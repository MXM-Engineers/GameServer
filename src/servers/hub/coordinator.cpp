#include <common/packet_serialize.h>
#include <common/inner_protocol.h>
#include <mxm/game_content.h>
#include <zlib.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EASprintf.h>

#include "coordinator.h"
#include "config.h"
#include "channel.h"
#include "instance.h"

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
		client.accountUID = hp->accountUID;
		client.instanceType = InstanceType::HUB;
		client.instanceUID = hub.UID;
		clientMap.emplace(client.clientHd, --clientList.end());
		clientHandleSet.insert(client.clientHd);

		HubInstance::NewUser nu;
		nu.clientHd = hp->clientHd;
		nu.accountUID = hp->accountUID;
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

void InstancePool::QueuePushPlayerToHub(ClientHandle clientHd, AccountUID accountUID)
{
	// TODO: choose a lane based on capacity
	Lane& l = lanes.front();

	const i32 clientID = plidMap.Push(clientHd);
	clientHandle[clientID] = clientHd;
	clientLocation[clientID].lane = l.laneIndex;

	Lane::HubPlayerEntry player;
	player.clientHd = clientHd;
	player.accountUID = accountUID;

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

	bool r = matchmaker.Init();
	if(!r) return false;

	r = instancePool.Init(server);
	if(!r) return false;

	thread.Begin(ThreadCoordinator, this);
	return true;
}

void Coordinator::Cleanup()
{
	LOG("Coordinator cleanup...");

	instancePool.Cleanup();
	thread.WaitForEnd();
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
		}
		clientAccountUID[clientID] = AccountUID::INVALID;
	}

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
			ASSERT(instancePool.IsClientInsideAnInstance(clientHd));
			instancePool.QueueRogueCoordinatorPacket(clientHd, header, packetData);
		} break;
	}

#undef HANDLE_CASE
}

void Coordinator::PushClientToHubInstance(ClientHandle clientHd)
{
	const i32 clientID = plidMap.Get(clientHd);
	instancePool.QueuePushPlayerToHub(clientHd, clientAccountUID[clientID]);
}

void Coordinator::HandlePacket_CQ_FirstHello(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_FirstHello& clHello = SafeCast<Cl::CQ_FirstHello>(packetData, packetSize);
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_FirstHello>(packetData, packetSize));

	// TODO: verify version, protocol, etc
	const i32 clientID = plidMap.Get(clientHd);
	const Server::ClientInfo& info = server->clientInfo[clientID];

	Sv::SA_FirstHello hello;
	hello.dwProtocolCRC = 0x28845199;
	hello.dwErrorCRC    = 0x93899e2c;
	hello.serverType    = 1;
	hello.clientIp[0] = info.ip[3];
	hello.clientIp[1] = info.ip[2];
	hello.clientIp[2] = info.ip[1];
	hello.clientIp[3] = info.ip[0];
	STATIC_ASSERT(sizeof(hello.clientIp) == sizeof(info.ip));
	hello.clientPort = info.port;
	hello.tqosWorldId = 1;

	SendPacket(clientHd, hello);
}

void Coordinator::HandlePacket_CQ_Authenticate(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	ConstBuffer request(packetData, packetSize);
	const u16 nickLen = request.Read<u16>();
	const wchar* nick = (wchar*)request.ReadRaw(nickLen * sizeof(wchar));
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_Authenticate>(packetData, packetSize));

	const i32 clientID = plidMap.Get(clientHd);
	const Server::ClientInfo& info = server->clientInfo[clientID];

	// TODO: check authentication

	// send authentication result
	Sv::SA_AuthResult auth;
	auth.result = 91;
	SendPacket(clientHd, auth);

	// TODO: fetch account data
	AccountManager& am = GetAccountManager();
	AccountUID accountUID = AccountUID((u32)clientHd); // FIXME: hack, actually get the accountID

	if(Config().DevMode && Config().DevQuickConnect) accountUID = AccountUID(0x1337);

	am.accountList.emplace_back(accountUID);
	am.accountMap.emplace(accountUID, --am.accountList.end());

	Account& account = *(--am.accountList.end());
	account.nickname.assign(nick, nickLen);
	account.guildTag = L"Alpha";
	account.leaderMasterID = 0; // Lua

	// remove the @plaync... part
	i64 f = account.nickname.find(L'@');
	if(f != -1) {
		account.nickname = account.nickname.left(f);
	}

	accChdMap.emplace(accountUID, clientHd);
	clientAccountUID[clientID] = accountUID;

	// send account data
	ClientSendAccountData(clientHd); // TODO: move this to hub instance

	PushClientToHubInstance(clientHd);
}

void Coordinator::ClientSendAccountData(ClientHandle clientHd)
{
	// SN_ClientSettings
	{
		PacketWriter<Sv::SN_ClientSettings,2048> packet;

		const char* src = R"foo(
						  <?xml version="1.0" encoding="utf-8"?>
						  <KEY_DATA highDateTime="30633031" lowDateTime="3986680182" isCustom="0" keyboardLayoutName="00000813">
						  <data input="INPUT_UIEDITMODE" key="MKC_NOKEY" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_PING" key="MKC_NOKEY" modifier="MKC_MENU" isaddkey="0" />
						  <data input="INPUT_LATTACK" key="MKC_LBUTTON" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_GAMEPING" key="MKC_LBUTTON" modifier="MKC_MENU" isaddkey="0" />
						  <data input="INPUT_SHIRK" key="MKC_RBUTTON" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_RATTACK" key="MKC_RBUTTON" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_BACKPING" key="MKC_RBUTTON" modifier="MKC_MENU" isaddkey="0" />
						  <data input="INPUT_DASHBOARD" key="MKC_TAB" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_CHATON" key="MKC_RETURN" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_CHAT_ALLPLAYER_ONCE" key="MKC_RETURN" modifier="MKC_SHIFT" isaddkey="0" />
						  <data input="INPUT_ESC" key="MKC_ESCAPE" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_JUMP_SAFEFALL" key="MKC_SPACE" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_REPLAY_GOTO_LIVE" key="MKC_0" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_CHATMACRO_0" key="MKC_0" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_WARFOGMODE_1" key="MKC_1" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_CHATMACRO_1" key="MKC_1" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_EMOTION_0" key="MKC_1" modifier="MKC_MENU" isaddkey="0" />
						  <data input="INPUT_WARFOGMODE_2" key="MKC_2" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_CHATMACRO_2" key="MKC_2" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_EMOTION_1" key="MKC_2" modifier="MKC_MENU" isaddkey="0" />
						  <data input="INPUT_WARFOGMODE_3" key="MKC_3" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_CHATMACRO_3" key="MKC_3" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_EMOTION_2" key="MKC_3" modifier="MKC_MENU" isaddkey="0" />
						  <data input="INPUT_WARFOGMODE_4" key="MKC_4" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_CHATMACRO_4" key="MKC_4" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_CHATMACRO_5" key="MKC_5" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_TOGGLE_TIME_CONTROLLER" key="MKC_6" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_CHATMACRO_6" key="MKC_6" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_CHATMACRO_7" key="MKC_7" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_REPLAY_MOVEBACK" key="MKC_8" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_CHATMACRO_8" key="MKC_8" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_REPLAY_PAUSE_RESUME" key="MKC_9" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_CHATMACRO_9" key="MKC_9" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_QUICKSLOT_1" key="MKC_A" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_SKILLUP_1" key="MKC_A" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_QUICKSLOT_1_NOTIFY" key="MKC_A" modifier="MKC_MENU" isaddkey="0" />
						  <data input="INPUT_QUICKSLOT_OPT" key="MKC_B" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_QUICKSLOT_3" key="MKC_C" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_CSHOP" key="MKC_C" modifier="MKC_SHIFT" isaddkey="0" />
						  <data input="INPUT_STAGE_SKILL_NOTIFY" key="MKC_C" modifier="MKC_MENU" isaddkey="0" />
						  <data input="INPUT_RIGHT" key="MKC_D" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_QUICKSLOT_2" key="MKC_E" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_SKILLUP_2" key="MKC_E" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_QUICKSLOT_2_NOTIFY" key="MKC_E" modifier="MKC_MENU" isaddkey="0" />
						  <data input="INPUT_ACTION" key="MKC_F" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_GUILD" key="MKC_G" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_TUTORIAL" key="MKC_H" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_INVENTORY" key="MKC_I" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_MISSIONLIST" key="MKC_J" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_NAME_DECO" key="MKC_K" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_FRIENDLIST" key="MKC_L" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_SCHEDULE" key="MKC_M" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_NO" key="MKC_N" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_OPTIONWINDOW" key="MKC_O" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_CHARACTER" key="MKC_P" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_POST" key="MKC_P" modifier="MKC_SHIFT" isaddkey="0" />
						  <data input="INPUT_LEFT" key="MKC_Q" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_QUICKSLOT_UG" key="MKC_R" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_SKILLUP_UG" key="MKC_R" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_QUICKSLOT_UG_NOTIFY" key="MKC_R" modifier="MKC_MENU" isaddkey="0" />
						  <data input="INPUT_BACK" key="MKC_S" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_ATTRIBUTE" key="MKC_T" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_TITAN_AVATAR_NOTIFY" key="MKC_T" modifier="MKC_MENU" isaddkey="0" />
						  <data input="INPUT_SUMMARY" key="MKC_U" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_PASSIVE_NOTIFY" key="MKC_V" modifier="MKC_MENU" isaddkey="0" />
						  <data input="INPUT_TITAN_AVATAR" key="MKC_W" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_UI_TOGGLE" key="MKC_X" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_YES" key="MKC_Y" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_FRONT" key="MKC_Z" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_GUARDIAN_CAM" key="MKC_NUMPAD0" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_WIZARD_CAM" key="MKC_NUMPAD1" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_ADAMAN_CAM" key="MKC_NUMPAD2" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_RUAK_CAM" key="MKC_NUMPAD3" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_TITAN_BLUE_CAM_1" key="MKC_NUMPAD4" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_TITAN_BLUE_CAM_2" key="MKC_NUMPAD5" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_TITAN_BLUE_CAM_3" key="MKC_NUMPAD6" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_TITAN_RED_CAM_1" key="MKC_NUMPAD7" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_TITAN_RED_CAM_2" key="MKC_NUMPAD8" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_TITAN_RED_CAM_3" key="MKC_NUMPAD9" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_FRIENDLY_CAM_1" key="MKC_F1" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_FRIENDLY_CAM_2" key="MKC_F2" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_FRIENDLY_CAM_3" key="MKC_F3" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_FRIENDLY_CAM_4" key="MKC_F4" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_FRIENDLY_CAM_5" key="MKC_F5" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_ENEMY_CAM_1" key="MKC_F6" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_ENEMY_CAM_2" key="MKC_F7" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_ENEMY_CAM_3" key="MKC_F8" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_ENEMY_CAM_4" key="MKC_F9" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_ENEMY_CAM_5" key="MKC_F10" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_CAM_MANUAL" key="MKC_F11" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_CAM_AUTO" key="MKC_F12" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_VIDEO_RECORDING" key="MKC_F12" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_CAM_ZOOM_TOGGLE" key="MKC_OEM_1" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_TRADER_INGREDIENT" key="MKC_OEM_PLUS" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_TRADER_SKIN" key="MKC_OEM_PERIOD" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_TRADER_MEDAL" key="MKC_OEM_2" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_TAG" key="MKC_WHEELDOWN" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_TAG_NOTIFY" key="MKC_WHEELDOWN" modifier="MKC_MENU" isaddkey="0" />
						  <data input="INPUT_BATTLELOG" key="MKC_OEM_6" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_TOGGLE_INGAME_INFORMATION" key="MKC_OEM_7" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_ATTRIBUTE" key="MKC_NOKEY" modifier="MKC_NOKEY" isaddkey="1" />
						  <data input="INPUT_TAG" key="MKC_WHEELUP" modifier="MKC_NOKEY" isaddkey="1" />
						  </KEY_DATA>
						  )foo";

		u8 dest[2048];
		uLongf destLen = sizeof(dest);
		int r = compress((Bytef*)dest, &destLen, (Bytef*)src, strlen(src));
		if(r != Z_OK) {
			if(r == Z_MEM_ERROR) LOG("ERROR(compress) not enough memory");
			else if(r == Z_BUF_ERROR) LOG("ERROR(compress) not enough room in the output buffer");
			ASSERT_MSG(0, "compress failed");
		}

		packet.Write<u8>(0); // settingType
		packet.Write<u16>(destLen);
		packet.WriteRaw(dest, destLen);

		SendPacket(clientHd, packet);
	}

	// SN_ClientSettings
	{
		PacketWriter<Sv::SN_ClientSettings,2048> packet;

		const char* src =
				R"foo(
				<?xml version="1.0" encoding="utf-8"?>
				<userdata
				version="2">
				<useroption>
				<displayUserName
				version="0"
				value="1" />
				<displayNameOfUserTeam
				version="0"
				value="1" />
				<displayNameOfOtherTeam
				version="0"
				value="1" />
				<displayMonsterName
				version="0"
				value="1" />
				<displayNpcName
				version="0"
				value="1" />
				<displayUserTitle
				version="0"
				value="1" />
				<displayOtherTitle
				version="0"
				value="1" />
				<displayUserStatusBar
				version="0"
				value="1" />
				<displayStatusBarOfOtherTeam
				version="0"
				value="1" />
				<displayStatusBarOfUserTeam
				version="0"
				value="1" />
				<displayMonsterStatusBar
				version="0"
				value="1" />
				<displayDamage
				version="0"
				value="1" />
				<displayStatus
				version="0"
				value="1" />
				<displayMasterBigImageType
				version="0"
				value="0" />
				<displayCursorSFX
				version="0"
				value="1" />
				<displayTutorialInfos
				version="0"
				value="1" />
				<displayUserStat
				version="0"
				value="0" />
				<chatFiltering
				version="0"
				value="1" />
				<chatTimstamp
				version="0"
				value="0" />
				<useSmartCast
				version="0"
				value="0" />
				<useMouseSight
				version="0"
				value="0" />
				<alwaysActivateHUD
				version="0"
				value="1" />
				</useroption>
				</userdata>
				)foo";

		u8 dest[2048];
		uLongf destLen = sizeof(dest);
		int r = compress((Bytef*)dest, &destLen, (Bytef*)src, strlen(src));
		if(r != Z_OK) {
			if(r == Z_MEM_ERROR) LOG("ERROR(compress) not enough memory");
			else if(r == Z_BUF_ERROR) LOG("ERROR(compress) not enough room in the output buffer");
			ASSERT_MSG(0, "compress failed");
		}

		packet.Write<u8>(1); // settingType
		packet.Write<u16>(destLen);
		packet.WriteRaw(dest, destLen);

		SendPacket(clientHd, packet);
	}

	// SN_ClientSettings
	{
		PacketWriter<Sv::SN_ClientSettings,2048> packet;

		const char* src =
				R"foo(
				<?xml version="1.0" encoding="utf-8"?>
				<KEY_DATA highDateTime="30633030" lowDateTime="3827081041" isCustom="0" keyboardLayoutName="00000813">
				<data input="INPUT_SHIRK" key="MKC_NOKEY" modifier="MKC_SHIFT" isaddkey="0" />
				<data input="INPUT_UIEDITMODE" key="MKC_NOKEY" modifier="MKC_CONTROL" isaddkey="0" />
				<data input="INPUT_PING" key="MKC_NOKEY" modifier="MKC_MENU" isaddkey="0" />
				<data input="INPUT_LATTACK" key="MKC_LBUTTON" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_GAMEPING" key="MKC_LBUTTON" modifier="MKC_MENU" isaddkey="0" />
				<data input="INPUT_RATTACK" key="MKC_RBUTTON" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_BACKPING" key="MKC_RBUTTON" modifier="MKC_MENU" isaddkey="0" />
				<data input="INPUT_DASHBOARD" key="MKC_TAB" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_CHATON" key="MKC_RETURN" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_CHAT_ALLPLAYER_ONCE" key="MKC_RETURN" modifier="MKC_SHIFT" isaddkey="0" />
				<data input="INPUT_ESC" key="MKC_ESCAPE" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_JUMP_SAFEFALL" key="MKC_SPACE" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_REPLAY_GOTO_LIVE" key="MKC_0" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_CHATMACRO_0" key="MKC_0" modifier="MKC_CONTROL" isaddkey="0" />
				<data input="INPUT_WARFOGMODE_1" key="MKC_1" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_CHATMACRO_1" key="MKC_1" modifier="MKC_CONTROL" isaddkey="0" />
				<data input="INPUT_EMOTION_0" key="MKC_1" modifier="MKC_MENU" isaddkey="0" />
				<data input="INPUT_WARFOGMODE_2" key="MKC_2" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_CHATMACRO_2" key="MKC_2" modifier="MKC_CONTROL" isaddkey="0" />
				<data input="INPUT_EMOTION_1" key="MKC_2" modifier="MKC_MENU" isaddkey="0" />
				<data input="INPUT_WARFOGMODE_3" key="MKC_3" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_CHATMACRO_3" key="MKC_3" modifier="MKC_CONTROL" isaddkey="0" />
				<data input="INPUT_EMOTION_2" key="MKC_3" modifier="MKC_MENU" isaddkey="0" />
				<data input="INPUT_WARFOGMODE_4" key="MKC_4" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_CHATMACRO_4" key="MKC_4" modifier="MKC_CONTROL" isaddkey="0" />
				<data input="INPUT_CHATMACRO_5" key="MKC_5" modifier="MKC_CONTROL" isaddkey="0" />
				<data input="INPUT_TOGGLE_TIME_CONTROLLER" key="MKC_6" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_CHATMACRO_6" key="MKC_6" modifier="MKC_CONTROL" isaddkey="0" />
				<data input="INPUT_CHATMACRO_7" key="MKC_7" modifier="MKC_CONTROL" isaddkey="0" />
				<data input="INPUT_REPLAY_MOVEBACK" key="MKC_8" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_CHATMACRO_8" key="MKC_8" modifier="MKC_CONTROL" isaddkey="0" />
				<data input="INPUT_REPLAY_PAUSE_RESUME" key="MKC_9" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_CHATMACRO_9" key="MKC_9" modifier="MKC_CONTROL" isaddkey="0" />
				<data input="INPUT_QUICKSLOT_1" key="MKC_A" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_SKILLUP_1" key="MKC_A" modifier="MKC_CONTROL" isaddkey="0" />
				<data input="INPUT_QUICKSLOT_1_NOTIFY" key="MKC_A" modifier="MKC_MENU" isaddkey="0" />
				<data input="INPUT_QUICKSLOT_OPT" key="MKC_B" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_QUICKSLOT_3" key="MKC_C" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_CSHOP" key="MKC_C" modifier="MKC_SHIFT" isaddkey="0" />
				<data input="INPUT_STAGE_SKILL_NOTIFY" key="MKC_C" modifier="MKC_MENU" isaddkey="0" />
				<data input="INPUT_RIGHT" key="MKC_D" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_QUICKSLOT_2" key="MKC_E" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_SKILLUP_2" key="MKC_E" modifier="MKC_CONTROL" isaddkey="0" />
				<data input="INPUT_QUICKSLOT_2_NOTIFY" key="MKC_E" modifier="MKC_MENU" isaddkey="0" />
				<data input="INPUT_ACTION" key="MKC_F" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_GUILD" key="MKC_G" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_TUTORIAL" key="MKC_H" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_INVENTORY" key="MKC_I" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_MISSIONLIST" key="MKC_J" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_NAME_DECO" key="MKC_K" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_FRIENDLIST" key="MKC_L" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_SCHEDULE" key="MKC_M" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_NO" key="MKC_N" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_OPTIONWINDOW" key="MKC_O" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_CHARACTER" key="MKC_P" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_POST" key="MKC_P" modifier="MKC_SHIFT" isaddkey="0" />
				<data input="INPUT_LEFT" key="MKC_Q" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_QUICKSLOT_UG" key="MKC_R" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_SKILLUP_UG" key="MKC_R" modifier="MKC_CONTROL" isaddkey="0" />
				<data input="INPUT_QUICKSLOT_UG_NOTIFY" key="MKC_R" modifier="MKC_MENU" isaddkey="0" />
				<data input="INPUT_BACK" key="MKC_S" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_ATTRIBUTE" key="MKC_T" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_TITAN_AVATAR_NOTIFY" key="MKC_T" modifier="MKC_MENU" isaddkey="0" />
				<data input="INPUT_SUMMARY" key="MKC_U" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_PASSIVE_NOTIFY" key="MKC_V" modifier="MKC_MENU" isaddkey="0" />
				<data input="INPUT_TITAN_AVATAR" key="MKC_W" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_UI_TOGGLE" key="MKC_X" modifier="MKC_CONTROL" isaddkey="0" />
				<data input="INPUT_YES" key="MKC_Y" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_FRONT" key="MKC_Z" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_GUARDIAN_CAM" key="MKC_NUMPAD0" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_WIZARD_CAM" key="MKC_NUMPAD1" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_ADAMAN_CAM" key="MKC_NUMPAD2" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_RUAK_CAM" key="MKC_NUMPAD3" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_TITAN_BLUE_CAM_1" key="MKC_NUMPAD4" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_TITAN_BLUE_CAM_2" key="MKC_NUMPAD5" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_TITAN_BLUE_CAM_3" key="MKC_NUMPAD6" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_TITAN_RED_CAM_1" key="MKC_NUMPAD7" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_TITAN_RED_CAM_2" key="MKC_NUMPAD8" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_TITAN_RED_CAM_3" key="MKC_NUMPAD9" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_FRIENDLY_CAM_1" key="MKC_F1" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_FRIENDLY_CAM_2" key="MKC_F2" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_FRIENDLY_CAM_3" key="MKC_F3" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_FRIENDLY_CAM_4" key="MKC_F4" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_FRIENDLY_CAM_5" key="MKC_F5" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_ENEMY_CAM_1" key="MKC_F6" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_ENEMY_CAM_2" key="MKC_F7" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_ENEMY_CAM_3" key="MKC_F8" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_ENEMY_CAM_4" key="MKC_F9" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_ENEMY_CAM_5" key="MKC_F10" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_CAM_MANUAL" key="MKC_F11" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_CAM_AUTO" key="MKC_F12" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_VIDEO_RECORDING" key="MKC_F12" modifier="MKC_CONTROL" isaddkey="0" />
				<data input="INPUT_CAM_ZOOM_TOGGLE" key="MKC_OEM_1" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_TRADER_INGREDIENT" key="MKC_OEM_PLUS" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_TRADER_SKIN" key="MKC_OEM_PERIOD" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_TRADER_MEDAL" key="MKC_OEM_2" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_TAG" key="MKC_WHEELDOWN" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_TAG_NOTIFY" key="MKC_WHEELDOWN" modifier="MKC_MENU" isaddkey="0" />
				<data input="INPUT_BATTLELOG" key="MKC_OEM_6" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_TOGGLE_INGAME_INFORMATION" key="MKC_OEM_7" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_ATTRIBUTE" key="MKC_NOKEY" modifier="MKC_NOKEY" isaddkey="1" />
				<data input="INPUT_RATTACK" key="MKC_RBUTTON" modifier="MKC_CONTROL" isaddkey="1" />
				<data input="INPUT_TAG" key="MKC_WHEELUP" modifier="MKC_NOKEY" isaddkey="1" />
				</KEY_DATA>
				)foo";

		u8 dest[2048];
		uLongf destLen = sizeof(dest);
		int r = compress((Bytef*)dest, &destLen, (Bytef*)src, strlen(src));
		if(r != Z_OK) {
			if(r == Z_MEM_ERROR) LOG("ERROR(compress) not enough memory");
			else if(r == Z_BUF_ERROR) LOG("ERROR(compress) not enough room in the output buffer");
			ASSERT_MSG(0, "compress failed");
		}

		packet.Write<u8>(2); // settingType
		packet.Write<u16>(destLen);
		packet.WriteRaw(dest, destLen);

		SendPacket(clientHd, packet);
	}
}
