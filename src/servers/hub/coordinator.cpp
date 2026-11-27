#include <common/packet_serialize.h>
#include <common/packet_validator.h>
#include <common/inner_protocol.h>
#include <mxm/game_content.h>
#include <zlib.h>
#include <time.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EASprintf.h>

#include "coordinator.h"
#include "config.h"
#include "channel.h"
#include "instance.h"
#include "default_settings.h"

static GameStore* g_GameStore = nullptr;

intptr_t ThreadLane(void* pData)
{
	InstancePool::Lane& lane = *(InstancePool::Lane*)pData;
	ProfileSetThreadName(FMT("Lane_%d", lane.laneIndex));
	const i32 cpuID = (i32)CoreAffinity::LANES + lane.laneIndex;
	EA::Thread::SetThreadAffinityMask((EA::Thread::ThreadAffinityMask)1 << cpuID);

	const f64 UPDATE_RATE_MS = (1.0 / UPDATE_TICK_RATE) * 1000.0;
	const Time startTime = TimeNow();
	Time t0 = startTime;

	char name[256];
	snprintf(name, sizeof(name), "Lane_%d", lane.laneIndex);

	while (lane.server->running)
	{
		Time t1 = TimeNow();
		lane.localTime = TimeDiff(startTime, t1);
		f64 delta = TimeDiffMs(TimeDiff(t0, t1));

		if (delta >= UPDATE_RATE_MS) {
			ProfileNewFrame(name);
			lane.Update();
			t0 = Time((u64)t0 + (u64)TimeMsToTime(UPDATE_RATE_MS));
		}
		else {
			// Calculate sleep time in milliseconds
			EA::Thread::ThreadTime sleepTime = (EA::Thread::ThreadTime)(UPDATE_RATE_MS - delta);

			if (sleepTime > 1.0) {
				// Sleep for most of the remaining time
				EA::Thread::ThreadSleep(sleepTime - 1);
			}
			// Yield for the final millisecond to improve timing accuracy
			EA::Thread::ThreadSleep(EA::Thread::kTimeoutYield);
		}
	}

	LOG("[Lane%d] Closing...", lane.laneIndex);
	lane.Cleanup();
	return 0;
}

intptr_t ThreadCoordinator(void* pData)
{
	ProfileSetThreadName("Coordinator");
	ThreadSetCoreAffinity((i32)CoreAffinity::COORDINATOR);

	Coordinator& coordinator = *(Coordinator*)pData;

	const f64 UPDATE_RATE_MS = (1.0 / 120.0) * 1000.0;
	const Time startTime = TimeNow();
	Time t0 = startTime;

	while (coordinator.server->running)
	{
		Time t1 = TimeNow();
		coordinator.localTime = TimeDiff(startTime, t1);
		f64 delta = TimeDiffMs(TimeDiff(t0, t1));

		if (delta >= UPDATE_RATE_MS) {
			ProfileNewFrame("Coordinator");
			coordinator.Update(delta / 1000.0);
			t0 = t1;
		}
		else {
			// Calculate sleep time in milliseconds 
			EA::Thread::ThreadTime sleepTime = (EA::Thread::ThreadTime)(UPDATE_RATE_MS - delta);

			if (sleepTime > 1.0) {
				// Sleep for most of the remaining time
				EA::Thread::ThreadSleep(sleepTime - 1);
			}
			// Yield for the final millisecond to improve timing accuracy
			EA::Thread::ThreadSleep(EA::Thread::kTimeoutYield);
		}
	}
	return 0;
}

static EA::Thread::AtomicUint32 g_NextInstanceUID = 1;

void InstancePool::Lane::Update()
{
	// on disconnected clients
	decltype(clientDisconnectQueue) disconnectedList;
	{ LOCK_MUTEX(mutexClientDisconnectQueue);
		disconnectedList = clientDisconnectQueue;
		clientDisconnectQueue.clear();
	}

	foreach_const(tr, disconnectedList) {
		auto client = clientMap.at(*tr);

		switch(client->instanceType) {
			case InstanceType::HUB: {
				HubInstance& instance = *instanceHubMap.at(client->instanceUID);
				instance.OnClientsDisconnected(&client->clientHd, 1); // TODO: group client handles by instance
			} break;

			case InstanceType::ROOM: {
				// TODO: we can probably transfer out of a room when a match is canceled (someone leaves)
				ASSERT_MSG(0, "can't transfer out from room");
			} break;

			default: {
				ASSERT_MSG(0, "case not handled");
			}
		}

		clientList.erase(client);
		clientMap.erase(*tr);
		clientHandleSet.erase(*tr);
		LOG("[Lane_%d][client%x] client disconnected", laneIndex, *tr);
	}

	// transfer clients out of instances
	decltype(clientTransferOutQueue) transferOutList;
	{ LOCK_MUTEX(mutexClientTransferOutQueue);
		transferOutList = clientTransferOutQueue;
		clientTransferOutQueue.clear();
	}

	foreach_const(tr, transferOutList) {
		auto client = clientMap.at(*tr);

		switch(client->instanceType) {
			case InstanceType::HUB: {
				HubInstance& instance = *instanceHubMap.at(client->instanceUID);
				instance.OnClientsTransferOut(&client->clientHd, 1); // TODO: group client handles by instance
			} break;

			case InstanceType::ROOM: {
				// TODO: we can probably transfer out of a room when a match is canceled (someone leaves)
				ASSERT_MSG(0, "can't transfer out from room");
			} break;

			default: {
				ASSERT_MSG(0, "case not handled");
			}
		}

		client->instanceType = InstanceType::NONE;
		client->instanceUID = HubInstanceUID::INVALID;
		LOG("[Lane_%d][client%x] client transfered out", laneIndex, *tr);
	}

	// create rooms
	decltype(createRoomQueue) createRoomList;
	{ LOCK_MUTEX(mutexCreateRoomQueue);
		createRoomList = createRoomQueue;
		createRoomQueue.clear();
	}

	foreach_const(cr, createRoomList) {
		const HubInstanceUID instUID = HubInstanceUID(g_NextInstanceUID++);
		instanceRoomList.emplace_back(instUID, cr->sortieUID);
		instanceRoomMap.emplace(instUID, --instanceRoomList.end());
		RoomInstance& room = *(--instanceRoomList.end());

		foreach_const(u, cr->users) {
			if(u->clientHd != ClientHandle::INVALID) {
				Client& client = *clientMap.at(u->clientHd);
				client.instanceType = InstanceType::ROOM;
				client.instanceUID = instUID;
			}
		}

		room.Init(server, cr->users.data(), cr->users.size());
	}

	// push players to hubs
	decltype(hubPushPlayerQueue) hubPlayerList;
	{ LOCK_MUTEX(mutexHubPushPlayerQueue);
		hubPlayerList = hubPushPlayerQueue;
		hubPushPlayerQueue.clear();
	}

	// TODO: only one hub ever?
	if(instanceHubList.empty()) {
		const HubInstanceUID instUID = HubInstanceUID(g_NextInstanceUID++);
		instanceHubList.emplace_back(instUID);
		instanceHubMap.emplace(instUID, --instanceHubList.end());

		HubInstance& hub = *(--instanceHubList.end());
		hub.Init(server);
	}

	HubInstance& hub = *(--instanceHubList.end());

	eastl::fixed_vector<HubInstance::NewUser,128> hubNewUsers;
	foreach_const(hp, hubPlayerList) {
		// create client entry
		clientList.push_back();
		Client& client = *(--clientList.end());
		client.clientHd = hp->clientHd;
		client.accountUID = hp->account->UID;
		client.instanceType = InstanceType::HUB;
		client.instanceUID = hub.UID;
		clientMap.emplace(client.clientHd, --clientList.end());
		clientHandleSet.insert(client.clientHd);

		hp->account->ApplyLogin(hp->login);

		HubInstance::NewUser nu;
		nu.clientHd = hp->clientHd;
		nu.account = hp->account;
		hubNewUsers.push_back(nu);

		LOG("[Lande_%d][client%x] client joins hub", laneIndex, client.clientHd);
	}
	hub.OnClientsConnected(hubNewUsers.data(), hubNewUsers.size());

	// handle client packets
	{ LOCK_MUTEX(mutexRoguePacketsQueue);
		recvDataBuff.Append(roguePacketsQueue.data, roguePacketsQueue.size);
		roguePacketsQueue.Clear();
	}

	eastl::fixed_vector<ClientHandle,MAX_CLIENTS> clientList;
	eastl::copy(clientHandleSet.begin(), clientHandleSet.end(), eastl::back_inserter(clientList));

	server->TransferReceivedData(&recvDataBuff, clientList.data(), clientList.size());

	ClientHandle curClientHd = ClientHandle::INVALID;
	HubInstance* curHubInstance = nullptr;
	RoomInstance* curRoomInstance = nullptr;

	// TODO: move base packet validation to server
	ConstBuffer buff(recvDataBuff.data, recvDataBuff.size);
	while(buff.CanRead(sizeof(Server::RecvChunkHeader))) {
		const Server::RecvChunkHeader& chunkInfo = buff.Read<Server::RecvChunkHeader>();
		const u8* data = buff.ReadRaw(chunkInfo.len);

		// handle each packet in chunk
		ConstBuffer reader(data, chunkInfo.len);
		if(!reader.CanRead(sizeof(NetHeader))) {
			WARN("Packet too small (clientHd=%u size=%d)", chunkInfo.clientHd, chunkInfo.len);
			server->DisconnectClient(chunkInfo.clientHd);
			continue;
		}

		while(reader.CanRead(sizeof(NetHeader))) {
			const NetHeader& header = reader.Read<NetHeader>();
			const i32 packetDataSize = header.size - sizeof(NetHeader);

			if(!reader.CanRead(packetDataSize)) {
				WARN("Packet header size differs from actual data size (clientHd=%u size=%d)", chunkInfo.clientHd, header.size);
				server->DisconnectClient(chunkInfo.clientHd);
				break;
			}

			if(Config().TraceNetwork) {
				fileSaveBuff(FormatPath(FMT("trace/lane_%d_cl_%d.raw", server->packetCounter, header.netID)), &header, header.size);
				server->packetCounter++;
			}

			const u8* packetData = reader.ReadRaw(packetDataSize);

			if(curClientHd != chunkInfo.clientHd) {
				curClientHd = chunkInfo.clientHd;
				curHubInstance = nullptr;
				curRoomInstance = nullptr;
				const Client& client = *clientMap.at(curClientHd);
				switch(client.instanceType) {
					case InstanceType::HUB: {
						curHubInstance = &*instanceHubMap.at(client.instanceUID);
					} break;

					case InstanceType::ROOM: {
						curRoomInstance = &*instanceRoomMap.at(client.instanceUID);
					} break;

					default: {
						ASSERT_MSG(0, "case not handled");
					}
				}
			}

			if(curHubInstance) {
				curHubInstance->OnClientPacket(curClientHd, header, packetData);
			}
			else if(curRoomInstance) {
				curRoomInstance->OnClientPacket(curClientHd, header, packetData);
			}
			else {
				ASSERT_MSG(0, "case not handled");
			}
		}
	}

	recvDataBuff.Clear();

	// matchmaker packets
	GrowableBuffer& backQueue = mmPacketQueues[!mmPacketQueueFront];
	if(backQueue.size > 0) {
		ConstBuffer reader(backQueue.data, backQueue.size);
		while(reader.CanRead(sizeof(NetHeader))) {
			const NetHeader& header = reader.Read<NetHeader>();
			const i32 packetDataSize = header.size - sizeof(NetHeader);
			ASSERT(reader.CanRead(packetDataSize));
			const u8* packetData = reader.ReadRaw(packetDataSize);

			// TODO: filter packets so we don't send everything everywhere
			foreach(hub, instanceHubList) {
				hub->OnMatchmakerPacket(header, packetData);
			}
			foreach(room, instanceRoomList) {
				room->OnMatchmakerPacket(header, packetData);
			}
		}
		backQueue.Clear();
	}

	{ LOCK_MUTEX(mutexMatchmakerPacketsQueue);
		mmPacketQueueFront ^= 1;
	}

	// update instances
	foreach(hub, instanceHubList) {
		hub->Update(localTime);
	}

	// delete rooms
	for(auto room = instanceRoomList.begin(); room != instanceRoomList.end(); ) {
		if(room->markedAsRemove) {
			LOG("[InstancePool] deleting room (sortieUID=%llu)", room->sortieUID);
			room = instanceRoomList.erase(room);
		}
		else {
			++room;
		}
	}

	// update rooms
	foreach(room, instanceRoomList) {
		room->Update(localTime);
	}
}

void InstancePool::Lane::Cleanup()
{

}

bool InstancePool::Init(Server* server_)
{
	server = server_;

	int laneIndex = 0;
	foreach(l, lanes) {
		l->server = server_;
		l->laneIndex = laneIndex++;
		l->mmPacketQueues[0].Init(1 * (1024*1024)); // 1 MB
		l->mmPacketQueues[1].Init(1 * (1024*1024)); // 1 MB
		l->recvDataBuff.Init(10 * (1024*1024)); // 10MB
		l->thread.Begin(ThreadLane, &*l);
	}

	return true;
}

void InstancePool::Cleanup()
{
	foreach(l, lanes) {
		l->thread.WaitForEnd();
	}
}

void InstancePool::QueuePushPlayerToHub(ClientHandle clientHd, Account* account, const AccountLogin& login)
{
	// TODO: choose a lane based on capacity
	Lane& l = lanes.front();

	const i32 clientID = plidMap.Push(clientHd);
	clientHandle[clientID] = clientHd;
	clientLocation[clientID].lane = l.laneIndex;

	Lane::HubPlayerEntry player;
	player.clientHd = clientHd;
	player.account = account;
	player.login = login;

	LOCK_MUTEX(l.mutexHubPushPlayerQueue);
	l.hubPushPlayerQueue.push_back(player);
}

void InstancePool::QueueCreateRoom(SortieUID sortieUID, const RoomUser* userList, const i32 userCount)
{
	// TODO: choose a lane based on capacity
	Lane& roomLane = lanes.front();

	for(i32 i = 0; i < userCount; i++) {
		const RoomUser& user = userList[i];
		if(user.clientHd == ClientHandle::INVALID) continue;

		const i32 clientID = plidMap.TryGet(user.clientHd);
		if(clientID == -1) {
			WARN("ClientID not found in local mapping (clientHd=%u)", user.clientHd);
			continue;
		}

		ClientLocation& loc = clientLocation[clientID];

		Lane& l = lanes[loc.lane];
		loc.lane = roomLane.laneIndex;

		// TODO: very inneficient locking, we lock for EVERY client
		LOCK_MUTEX(l.mutexClientTransferOutQueue);
		l.clientTransferOutQueue.push_back(user.clientHd);
	}

	Lane::CreateRoomEntry create;
	create.sortieUID = sortieUID;

	for(i32 i = 0; i < userCount; i++) {
		const RoomUser& user = userList[i];
		if(user.clientHd != ClientHandle::INVALID) {
			const i32 clientID = plidMap.TryGet(user.clientHd);
			if(clientID == -1) {
				WARN("ClientID not found in local mapping (clientHd=%u)", user.clientHd);
				continue;
			}
		}

		create.users.push_back(user);
	}

	LOCK_MUTEX(roomLane.mutexCreateRoomQueue);
	roomLane.createRoomQueue.push_back(create);
}

void InstancePool::QueuePopPlayers(const ClientHandle* clientList, const i32 count)
{
	for(int i = 0; i < count; i++) {
		const ClientHandle clientHd = clientList[i];
		const i32 clientID = plidMap.Get(clientHd);
		plidMap.Pop(clientHd);

		clientHandle[clientID] = ClientHandle::INVALID;
		Lane& l = lanes[clientLocation[clientID].lane];
		clientLocation[clientID] = ClientLocation::Null();

		// TODO: very bad locking
		LOCK_MUTEX(l.mutexClientDisconnectQueue);
		l.clientDisconnectQueue.push_back(clientHd);
	}
}

void InstancePool::QueueRogueCoordinatorPacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData)
{
	const i32 clientID = plidMap.Get(clientHd);
	const ClientLocation& loc = clientLocation[clientID];
	Lane& l = lanes[loc.lane];

	LOCK_MUTEX(l.mutexRoguePacketsQueue);
	Server::RecvChunkHeader ch;
	ch.clientHd = clientHd;
	ch.len = header.size;
	l.roguePacketsQueue.Append(&ch, sizeof(ch));
	l.roguePacketsQueue.Append(&header, sizeof(header));
	l.roguePacketsQueue.Append(packetData, header.size - sizeof(header));
}

void InstancePool::QueueMatchmakerPackets(const u8* buffer, u32 bufferSize)
{
	// TODO: filter packets here as well?

	foreach(l, lanes) {
		Lane& lane = *l;
		LOCK_MUTEX(lane.mutexMatchmakerPacketsQueue);
		lane.mmPacketQueues[lane.mmPacketQueueFront].Append(buffer, bufferSize);
	}
}

bool Coordinator::Init(Server* server_)
{
	server = server_;
	recvDataBuff.Init(10 * (1024*1024)); // 10 MB

	clientHandle.fill(ClientHandle::INVALID);
	clientLoadingProfile.fill(false);

	bool r = matchmaker.Init();
	if(!r) return false;

	if(!BuildProfileSeed(&profileSeed)) return false;
	if(!BuildWalletSeed(&profileSeed)) return false;

	GameStoreOptions storeOptions;
	storeOptions.seed = profileSeed;
	if(!gameStore.Start(&gameStoreBackend, Config().GameDatabasePath, storeOptions)) {
		LOG("ERROR: failed to open game database '%s'", Config().GameDatabasePath);
		return false;
	}
	g_GameStore = &gameStore;

	r = instancePool.Init(server);
	if(!r) {
		gameStore.Shutdown();
		return false;
	}

	thread.Begin(ThreadCoordinator, this);
	return true;
}

bool Coordinator::BuildProfileSeed(ProfileSeed* out)
{
	const GameXmlContent& content = GetGameXmlContent();
	*out = ProfileSeed();
	out->dev = Config().DevSeedProfile != 0;

	if(out->dev) {
		foreach_const(it, content.masters) {
			out->masters.push_back(it->classType);
			foreach_const(s, it->skinIDs) {
				out->skins.push_back(ProfileSeed::Skin{ it->classType, *s });
			}
		}
		out->leaderClass = content.masterClassTypeMap.find(ClassType::LAUNCHER) != content.masterClassTypeMap.end() ? ClassType::LAUNCHER : out->masters.front();
		LOG("Coordinator :: new profiles own every master (%d) and skin (%d) (DevSeedProfile=1)", (i32)out->masters.size(), (i32)out->skins.size());
		return true;
	}

	foreach_const(it, Config().serviceRuleDefaultUnlockClass) {
		const ClassType classType = (ClassType)*it;
		if(content.masterClassTypeMap.find(classType) == content.masterClassTypeMap.end()) {
			LOG("ERROR: ServiceRuleDefaultUnlockClass names class %d, which is not a known master", *it);
			return false;
		}
		out->masters.push_back(classType);
		out->skins.push_back(ProfileSeed::Skin{ classType, SkinIndex::DEFAULT });
	}

	if(out->masters.empty()) {
		LOG("ERROR: ServiceRuleDefaultUnlockClass is empty, new profiles would own no master");
		return false;
	}

	out->leaderClass = out->masters.front();
	LOG("Coordinator :: new profiles own %d starter master(s), leader class %d", (i32)out->masters.size(), (i32)out->leaderClass);
	return true;
}

bool Coordinator::BuildWalletSeed(ProfileSeed* out)
{
	const CConfigHub& cfg = Config();

	if(cfg.ServiceRuleDefaultMoney < 0 || cfg.ServiceRuleDefaultMoneyBM < 0) {
		LOG("ERROR: ServiceRuleDefaultMoney and ServiceRuleDefaultMoneyBM must not be negative");
		return false;
	}
	if(cfg.serviceRuleDefaultLineCount.size() != INVENTORY_TABS) {
		LOG("ERROR: ServiceRuleDefaultLineCount needs %d values (E_INVEN_TAB_0..2)", (i32)INVENTORY_TABS);
		return false;
	}
	for(i32 t = 0; t < INVENTORY_TABS; t++) {
		out->lines[t] = cfg.serviceRuleDefaultLineCount[t];
	}
	out->lines[CONTAINER_WAREHOUSE] = cfg.ServiceRuleWarehouseDefaultLineCount;
	for(i32 c = 0; c < INVENTORY_CONTAINERS; c++) {
		if(out->lines[c] < 1 || out->lines[c] > ContainerMaxLines(c)) {
			LOG("ERROR: default line count %d of container %d is outside 1..%d", out->lines[c], c, ContainerMaxLines(c));
			return false;
		}
	}
	foreach_const(it, cfg.servicePriceInvenUnlockGold) {
		if(*it <= 0) {
			LOG("ERROR: ServicePriceInvenUnlockGold has a price %d, prices must be positive", *it);
			return false;
		}
	}
	if(cfg.FixedDataJukeboxCoinIndex <= 0) {
		LOG("ERROR: FixedDataJukeboxCoinIndex=%d is not an item index", cfg.FixedDataJukeboxCoinIndex);
		return false;
	}

	out->gold = cfg.ServiceRuleDefaultMoney;
	out->ccoin = cfg.ServiceRuleDefaultMoneyBM;

	if(out->dev) {
		out->gold = 10000000;
		out->ccoin = 1000000;
		for(i32 c = 0; c < INVENTORY_CONTAINERS; c++) {
			out->lines[c] = ContainerMaxLines(c);
		}
	}

	out->items.clear();
	if(cfg.ServiceRuleDefaultGearCount < 0 || cfg.ServiceRuleDefaultGearCount > out->lines[0] * INVENTORY_SLOTS_PER_LINE) {
		LOG("ERROR: ServiceRuleDefaultGearCount=%d does not fit the gear tab", cfg.ServiceRuleDefaultGearCount);
		return false;
	}
	if(cfg.ServiceRuleDefaultGearCount > 0 && cfg.ServiceRuleDefaultGearIndex <= 0) {
		LOG("ERROR: ServiceRuleDefaultGearIndex must name an item");
		return false;
	}
	for(i32 i = 0; i < cfg.ServiceRuleDefaultGearCount; i++) {
		out->items.push_back(ProfileSeed::Item{ (ItemDocIndex)cfg.ServiceRuleDefaultGearIndex, 1, cfg.ItemBasicDefaultGearOptionGroup, 0 });
	}

	LOG("Coordinator :: new profiles start with %lld gold, %lld ccoin, %d x item %d, lines %d/%d/%d warehouse %d%s",
		(long long)out->gold, (long long)out->ccoin, (i32)out->items.size(), cfg.ServiceRuleDefaultGearIndex,
		out->lines[0], out->lines[1], out->lines[2], out->lines[CONTAINER_WAREHOUSE], out->dev ? " (DevSeedProfile=1)" : "");
	return true;
}

GameStore& GetGameStore()
{
	ASSERT(g_GameStore);
	return *g_GameStore;
}

void Coordinator::Cleanup()
{
	LOG("Coordinator cleanup...");

	instancePool.Cleanup();
	thread.WaitForEnd();
	gameStore.Shutdown();
	accountDB.Close();
}

void Coordinator::Update(f64 delta)
{
	ProfileFunction();

	matchmaker.Update();
	instancePool.QueueMatchmakerPackets(matchmaker.packetQueue.data, matchmaker.packetQueue.size);

	// parse some matchmaker packets
	ConstBuffer reader(matchmaker.packetQueue.data, matchmaker.packetQueue.size);
	while(reader.CanRead(sizeof(NetHeader))) {
		const NetHeader& header = reader.Read<NetHeader>();
		const i32 packetDataSize = header.size - sizeof(NetHeader);
		ASSERT(reader.CanRead(packetDataSize));
		const u8* packetData = reader.ReadRaw(packetDataSize);

		// A room has been created, detach clients from their lane & hub instance
		// We then push them all to the same room instance
		if(header.netID == In::MN_RoomCreated::NET_ID) {
			const In::MN_RoomCreated packet = SafeCast<In::MN_RoomCreated>(packetData, packetDataSize);

			eastl::fixed_vector<InstancePool::RoomUser,128> roomClientList;
			for(int i = 0; i < packet.playerCount; i++) {
				const In::RoomUser& pl = packet.playerList[i];

				if(pl.isBot) {
					InstancePool::RoomUser ru;
					ru.clientHd = ClientHandle::INVALID;
					ru.accountUID = AccountUID::INVALID;
					ru.name.assign(pl.name.data, pl.name.len);
					ru.team = pl.team;
					ru.userID = i;
					ru.isBot = true;
					roomClientList.push_back(ru);
				}
				else {
					auto it = accChdMap.find(pl.accountUID);

					ClientHandle clientHd = ClientHandle::INVALID;
					if(it != accChdMap.end()) { // on this server
						ASSERT(clientAccountUID[plidMap.Get(it->second)] == pl.accountUID); // sanity check
						clientHd = it->second;
					}

					InstancePool::RoomUser ru;
					ru.clientHd = clientHd;
					ru.accountUID = pl.accountUID;
					ru.name.assign(pl.name.data, pl.name.len);
					ru.team = pl.team;
					ru.userID = i;
					ru.isBot = false;
					roomClientList.push_back(ru);
				}
			}

			instancePool.QueueCreateRoom(packet.sortieUID, roomClientList.data(), roomClientList.size());
		}
	}
	matchmaker.packetQueue.Clear();

	// handle client connections
	eastl::fixed_vector<ClientHandle,128> clientConnectedList;
	server->TransferConnectedClientList(&clientConnectedList);

	foreach_const(cl, clientConnectedList) {
		const i32 clientID = plidMap.Push(*cl);
		clientHandle[clientID] = *cl;
		clientAccountUID[clientID] = AccountUID::INVALID;
		clientLoadingProfile[clientID] = false;
		clientLogin[clientID] = AccountLogin();
	}

	// handle client disconnections
	eastl::fixed_vector<ClientHandle,128> clientDisconnectedList;
	server->TransferDisconnectedClientList(&clientDisconnectedList);

	// clear client data
	foreach_const(cl, clientDisconnectedList) {
		const i32 clientID = plidMap.Get(*cl);
		plidMap.Pop(*cl);
		clientHandle[clientID] = ClientHandle::INVALID;
		if(clientAccountUID[clientID] != AccountUID::INVALID) {
			accChdMap.erase(clientAccountUID[clientID]);
			RecordLogout(*cl, clientAccountUID[clientID], clientLogin[clientID]);
		}
		clientAccountUID[clientID] = AccountUID::INVALID;
		clientLoadingProfile[clientID] = false;
		clientLogin[clientID] = AccountLogin();
	}

	HandleLoadedProfiles();

	// handle received data
	eastl::fixed_vector<ClientHandle,MAX_CLIENTS> clientList;
	for(i32 clientID = 0; clientID < MAX_CLIENTS; clientID++) {
		if(clientHandle[clientID] != ClientHandle::INVALID && !instancePool.IsClientInsideAnInstance(clientHandle[clientID])) {
			clientList.push_back(clientHandle[clientID]);
		}
	}

	server->TransferReceivedData(&recvDataBuff, clientList.data(), clientList.size());

	NetworkParseReceiveBuffer(this, server, recvDataBuff.data, recvDataBuff.size, "hub");

	recvDataBuff.Clear();
}

void Coordinator::ClientHandlePacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData)
{
	const i32 clientID = plidMap.Get(clientHd);
	const i32 packetSize = header.size - sizeof(NetHeader);

#define HANDLE_CASE(PACKET) case Cl::PACKET::NET_ID: { HandlePacket_##PACKET(clientHd, header, packetData, packetSize); } break

	switch(header.netID) {
		HANDLE_CASE(CQ_FirstHello);
		HANDLE_CASE(CQ_Authenticate);

		default: {
			WARN("Unhandled packet (netID=%d size=%d)", header.netID, header.size);
			if(!instancePool.IsClientInsideAnInstance(clientHd)) break;
			instancePool.QueueRogueCoordinatorPacket(clientHd, header, packetData);
		} break;
	}

#undef HANDLE_CASE
}

void Coordinator::PushClientToHubInstance(ClientHandle clientHd)
{
	const i32 clientID = plidMap.Get(clientHd);
	instancePool.QueuePushPlayerToHub(clientHd, &FindOrAddAccount(clientAccountUID[clientID]), clientLogin[clientID]);
}

void Coordinator::HandlePacket_CQ_FirstHello(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_FirstHello>(packetData, packetSize)) {
		WARN("WARNING: invalid CQ_FirstHello (size=%d)", packetSize);
		return;
	}
	const Cl::CQ_FirstHello& clHello = SafeCast<Cl::CQ_FirstHello>(packetData, packetSize);
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_FirstHello>(packetData, packetSize));

	// TODO: verify version, protocol, etc
	const i32 clientID = plidMap.Get(clientHd);
	const Server::ClientInfo& info = server->clientInfo[clientID];

	Sv::SA_FirstHello hello;
	hello.dwProtocolCRC = 0x28845199;
	hello.dwErrorCRC    = 0x93899e2c;
	hello.serverType    = Sv::ServerType::Hub;
	memmove(hello.clientIp, info.ip.data(), sizeof(hello.clientIp));
	STATIC_ASSERT(sizeof(hello.clientIp) == sizeof(info.ip));
	hello.clientPort = htons(info.port);
	hello.tqosWorldId = 1;

	SendPacket(clientHd, hello);
}

void Coordinator::HandlePacket_CQ_Authenticate(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_Authenticate>(packetData, packetSize)) {
		WARN("WARNING: invalid CQ_Authenticate (size=%d)", packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_Authenticate>(packetData, packetSize));

	ConstBuffer request(packetData, packetSize);
	const u16 nickStrLen = request.Read<u16>();
	const wchar* nick = (wchar*)request.ReadRaw(nickStrLen * sizeof(wchar));
	const u32 instantKey = request.Read<u32>();

	const i32 clientID = plidMap.Get(clientHd);
	if(clientAccountUID[clientID] != AccountUID::INVALID) {
		WARN("client %x is already authenticated", clientHd);
		return;
	}

	i32 nickLen = 0;
	while(nickLen < nickStrLen && nick[nickLen] != L'@') nickLen++;
	if(nickLen < 1 || nickLen > AccountDB::MAX_LOGIN_NAME_LEN) {
		RefuseClient(clientHd, ErrorType::AUTH_NOT_LOGIN);
		return;
	}

	LoginSession session;
	ErrorType error = ErrorType::AUTH_NOT_LOGIN;
	const bool hasSession = ResolveLogin(nick, nickLen, instantKey, &session, &error);
	if(!hasSession && Config().RequireLoginSession) {
		RefuseClient(clientHd, error);
		return;
	}

	AccountUID accountUID = session.accountUID;
	if(!hasSession) {
		accountUID = AccountUID((u32)clientHd);
		if(Config().DevMode && Config().DevQuickConnect) accountUID = AccountUID(0x1337);
	}

	if(accChdMap.find(accountUID) != accChdMap.end()) {
		RefuseClient(clientHd, ErrorType::AUTH_WRONG_CONNECTION);
		return;
	}

	FindOrAddAccount(accountUID);
	AccountLogin& login = clientLogin[clientID];
	login = AccountLogin();
	login.guildTag = L"Alpha";
	login.profile.accountUID = accountUID;
	if(hasSession) login.nickname = session.nickname;
	else login.nickname.assign(nick, nickLen);

	accChdMap.emplace(accountUID, clientHd);
	clientAccountUID[clientID] = accountUID;

	if(hasSession) {
		clientLoadingProfile[clientID] = true;
		gameStore.QueueLoadProfile((u32)clientHd, accountUID);
		return;
	}

	SeedProfileInMemory(profileSeed, gameStore.ItemUIDs(), &login.profile);

	Sv::SA_AuthResult auth;
	auth.nResult = (i32)ErrorType::AUTH_SUCCESS;
	SendPacket(clientHd, auth);

	EnterHub(clientHd);
}

void Coordinator::RecordLogout(ClientHandle clientHd, AccountUID accountUID, const AccountLogin& login)
{
	if(!login.profileStored) return;
	if(IsAccountSentToGame(login.nickname)) {
		LOG("[client%x] account %u left for a game server, last logout not written", (u32)clientHd, (u32)accountUID);
		return;
	}

	const i64 now = (i64)time(nullptr);
	gameStore.QueueWrite("SetLastLogout", [accountUID, now](GameStoreBackend& b) {
		return b.SetLastLogout(accountUID, now);
	});
}

Account& Coordinator::FindOrAddAccount(AccountUID accountUID)
{
	AccountManager& am = GetAccountManager();
	auto found = am.accountMap.find(accountUID);
	if(found == am.accountMap.end()) {
		am.accountList.emplace_back(accountUID);
		found = am.accountMap.emplace(accountUID, --am.accountList.end()).first;
	}
	return *found->second;
}

void Coordinator::HandleLoadedProfiles()
{
	gameStore.TransferLoaded(&loadedProfiles);

	foreach_const(it, loadedProfiles) {
		const ClientHandle clientHd = (ClientHandle)it->tag;
		const AccountUID accountUID = it->profile.accountUID;

		const i32 clientID = plidMap.TryGet(clientHd);
		if(clientID == -1 || !clientLoadingProfile[clientID] || clientAccountUID[clientID] != accountUID) {
			LOG("[client%x] profile of account %u loaded after the client left, dropped", (u32)clientHd, (u32)accountUID);
			continue;
		}
		clientLoadingProfile[clientID] = false;

		clientLogin[clientID].profile = it->profile;
		clientLogin[clientID].profileStored = true;
		if(it->profile.firstEntry) LOG("[client%x] account %u enters this server for the first time", (u32)clientHd, (u32)accountUID);

		Sv::SA_AuthResult auth;
		auth.nResult = (i32)ErrorType::AUTH_SUCCESS;
		SendPacket(clientHd, auth);

		EnterHub(clientHd);
	}
}

bool Coordinator::ResolveLogin(const wchar* name, i32 nameLen, u32 instantKey, LoginSession* outSession, ErrorType* outError)
{
	*outSession = LoginSession();
	*outError = ErrorType::AUTH_NOT_LOGIN;

	if(!accountDBOpen) accountDBOpen = accountDB.OpenReadOnly(Config().AccountsDatabasePath);
	if(!accountDBOpen) return false;

	const WideString asNickname(name, nameLen);
	if(IsAccountSentToGame(asNickname)) {
		return accountDB.FindSessionByNickname(name, nameLen, outSession);
	}

	LoginSession session;
	if(!accountDB.FindSession(name, nameLen, &session)) return false;
	if(session.instantKey != instantKey) {
		*outError = ErrorType::AUTH_WRONG_KEY;
		return false;
	}
	if(session.nickname.empty()) return false;

	*outSession = session;
	return true;
}

void Coordinator::RefuseClient(ClientHandle clientHd, ErrorType error)
{
	LOG("[client%x] authentication refused (%u)", clientHd, (u32)error);
	Sv::SA_AuthResult auth;
	auth.nResult = (i32)error;
	SendPacket(clientHd, auth);
	server->DisconnectClient(clientHd);
}

void Coordinator::EnterHub(ClientHandle clientHd)
{
	ClientSendAccountData(clientHd);
	PushClientToHubInstance(clientHd);
}

void Coordinator::SendClientSettings(ClientHandle clientHd, u8 settingType, const u8* data, i32 size)
{
	ASSERT(size > 0 && size <= PROFILE_SETTINGS_MAX_SIZE);
	PacketWriter<Sv::SN_ClientSettings,3 + PROFILE_SETTINGS_MAX_SIZE> packet;
	packet.Write<u8>(settingType);
	packet.Write<u16>((u16)size);
	packet.WriteRaw(data, size);
	SendPacket(clientHd, packet);
}

void Coordinator::ClientSendAccountData(ClientHandle clientHd)
{
	const PlayerProfile& profile = clientLogin[plidMap.Get(clientHd)].profile;

	for(i32 settingType = 0; settingType < PROFILE_SETTINGS_SLOTS; settingType++) {
		const ProfileSettings& stored = profile.settings[settingType];
		if(stored.present) {
			SendClientSettings(clientHd, (u8)settingType, stored.data.data(), (i32)stored.data.size());
			continue;
		}

		const char* src = DefaultClientSettingsXml((u8)settingType);
		if(!src) continue;

		u8 dest[PROFILE_SETTINGS_MAX_SIZE];
		uLongf destLen = sizeof(dest);
		const int r = compress((Bytef*)dest, &destLen, (const Bytef*)src, strlen(src));
		if(r != Z_OK) {
			if(r == Z_MEM_ERROR) LOG("ERROR(compress) not enough memory");
			else if(r == Z_BUF_ERROR) LOG("ERROR(compress) not enough room in the output buffer");
			ASSERT_MSG(0, "compress failed");
		}
		SendClientSettings(clientHd, (u8)settingType, dest, (i32)destLen);
	}
}
