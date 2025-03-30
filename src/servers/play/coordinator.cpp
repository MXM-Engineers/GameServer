#include <mxm/game_content.h>
#include <common/packet_serialize.h>
#include <common/inner_protocol.h>
#include <zlib.h>

#include "coordinator.h"
#include "config.h"
#include "channel.h"
#include "instance.h"

intptr_t ThreadLane(void* pData)
{
	InstancePool::Lane& lane = *(InstancePool::Lane*)pData;
	ProfileSetThreadName(FMT("Lane_%d", lane.laneIndex));
	const i32 cpuID = (i32)CoreAffinity::LANES + lane.laneIndex;
    ThreadSetCoreAffinity(cpuID);

	const f64 UPDATE_RATE_MS = (1.0/UPDATE_TICK_RATE) * 1000.0;
	const Time startTime = TimeNow();
	Time t0 = startTime;

	char name[256];
	snprintf(name, sizeof(name), "Lane_%d", lane.laneIndex);

	f64 accumulator = 0.0f;

	while(lane.server->running)
	{
		Time t1 = TimeNow();
		lane.localTime = TimeDiff(startTime, t1);
		f64 delta = TimeDiffMs(TimeDiff(t0, t1));

		if(delta > UPDATE_RATE_MS) {
			ProfileNewFrame(name);
			lane.Update();
			t0 = Time((u64)t0 + (u64)TimeMsToTime(UPDATE_RATE_MS));
		}
		/*else {
			EA::Thread::ThreadSleep((EA::Thread::ThreadTime)(UPDATE_RATE_MS - delta));
			// EA::Thread::ThreadSleep(EA::Thread::kTimeoutYield);
			// Sleep on windows is notoriously innacurate, we'll probably need to "just yield"
		}*/
		else {
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

	const f64 UPDATE_RATE_MS = (1.0/120.0) * 1000.0;
	const Time startTime = TimeNow();
	Time t0 = startTime;

	while(coordinator.server->running)
	{
		Time t1 = TimeNow();
		coordinator.localTime = TimeDiff(startTime, t1);
		f64 delta = TimeDiffMs(TimeDiff(t0, t1));

		if(delta > UPDATE_RATE_MS) {
			ProfileNewFrame("Coordinator");
			coordinator.Update();
			t0 = t1;
		}
		else {
			EA::Thread::ThreadSleep((EA::Thread::ThreadTime)(UPDATE_RATE_MS - delta)); // yield
			// EA::Thread::ThreadSleep(EA::Thread::kTimeoutYield);
			// Sleep on windows is notoriously innacurate, we'll probably need to "just yield"
		}
	}
	return 0;
}

static EA::Thread::AtomicUint32 g_NextInstanceUID = 1;

void InstancePool::Lane::Update()
{
	// create games
	decltype(createGameQueue) gameList;
	{ LOCK_MUTEX(mutexCreateGameQueue);
		gameList = createGameQueue;
		createGameQueue.clear();
	}

	foreach_const(g, gameList) {
		instancePvpList.emplace_back(g->sortieUID, *g, server);
		auto inst = --instancePvpList.end();
		instancePvpMap.emplace(inst->sortieUID, inst);

		LOG("[Lane_%d] Created game (sortieUID=%llu)", laneIndex, inst->sortieUID);
	}

	// on disconnected clients
	decltype(clientDisconnectQueue) disconnectedList;
	{ LOCK_MUTEX(mutexClientDisconnectQueue);
		disconnectedList = clientDisconnectQueue;
		clientDisconnectQueue.clear();
	}

	foreach_const(tr, disconnectedList) {
		auto client = clientMap.at(*tr);

		switch(client->instanceType) {
			case InstanceType::PVP_3V3: {
				PvpInstance& instance = *instancePvpMap.at(client->sortieUID);
				instance.OnClientsDisconnected(&client->clientHd, 1); // TODO: group client handles by instance
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

	// on connected clients
	decltype(clientConnectQueue) connectedList;
	{ LOCK_MUTEX(mutexClientConnectQueue);
		connectedList = clientConnectQueue;
		clientConnectQueue.clear();
	}

	foreach_const(e, connectedList) {
		clientList.push_back();
		auto cit = --clientList.end();
		Client& client = *cit;
		clientMap.emplace(e->clientHd, cit);
		clientHandleSet.insert(e->clientHd);

		client.clientHd = e->clientHd;
		client.instanceType = InstanceType::PVP_3V3;
		client.sortieUID = e->sortieUID;

		PvpInstance& instance = *instancePvpMap.at(client.sortieUID);
		eastl::pair<ClientHandle,AccountUID> list(e->clientHd, e->accountUID);
		instance.OnClientsConnected(&list, 1); // TODO: group by instance

		LOG("[Lane_%d][client%x] client connected to sortie (sortieUID=%llu)", laneIndex, client.clientHd, client.sortieUID);
	}

	// handle client packets
	{ LOCK_MUTEX(mutexRoguePacketsQueue);
		recvDataBuff.Append(roguePacketsQueue.data, roguePacketsQueue.size);
		roguePacketsQueue.Clear();
	}

	eastl::fixed_vector<ClientHandle,MAX_CLIENTS> clientList;
	eastl::copy(clientHandleSet.begin(), clientHandleSet.end(), eastl::back_inserter(clientList));

	server->TransferReceivedData(&recvDataBuff, clientList.data(), clientList.size());

	ClientHandle curClientHd = ClientHandle::INVALID;
	PvpInstance* curPvpInstance = nullptr;

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
				curPvpInstance = nullptr;
				const Client& client = *clientMap.at(curClientHd);
				switch(client.instanceType) {
					case InstanceType::PVP_3V3: {
						curPvpInstance = &*instancePvpMap.at(client.sortieUID);
					} break;

					default: {
						ASSERT_MSG(0, "case not handled");
					}
				}
			}

			if(curPvpInstance) {
				curPvpInstance->OnClientPacket(curClientHd, header, packetData);
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
			foreach(room, instancePvpList) {
				room->OnMatchmakerPacket(header, packetData);
			}
		}
		backQueue.Clear();
	}

	{ LOCK_MUTEX(mutexMatchmakerPacketsQueue);
		mmPacketQueueFront ^= 1;
	}

	// update instances
	foreach(room, instancePvpList) {
		room->Update(localTime);
	}
}

void InstancePool::Lane::Cleanup()
{
	foreach(inst, instancePvpList) {
		inst->game.Cleanup();
	}
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

void InstancePool::QueuePushPlayerToGame(ClientHandle clientHd, AccountUID accountUID, SortieUID sortieUID)
{
	const i32 clientID = plidMap.Push(clientHd);
	clientHandle[clientID] = clientHd;

	u8 laneID = sortieLocation.at(sortieUID);
	clientLocation[clientID].lane = laneID;

	Lane& l = lanes[laneID];

	Lane::ClientConnectEntry entry;
	entry.clientHd = clientHd;
	entry.accountUID = accountUID;
	entry.sortieUID = sortieUID;

	LOCK_MUTEX(l.mutexClientConnectQueue);
	l.clientConnectQueue.push_back(entry);
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

void InstancePool::QueueCreateGame(const In::MQ_CreateGame& gameInfo)
{
	// TODO: choose lane based on load
	Lane& l = lanes.front();

	sortieLocation.emplace(gameInfo.sortieUID, 0);

	LOCK_MUTEX(l.mutexCreateGameQueue);
	l.createGameQueue.push_back(gameInfo);
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

	if(Config().DevMode && Config().DevQuickConnect) {
		CreateDevGame();
	}

	thread.Begin(ThreadCoordinator, this);
	return true;
}

void Coordinator::Cleanup()
{
	LOG("Coordinator cleanup...");

	instancePool.Cleanup();
	thread.WaitForEnd();
}

void Coordinator::Update()
{
	ProfileFunction();

	matchmaker.Update();
	ProcessMatchmakerPackets();

	// handle client connections
	eastl::fixed_vector<ClientHandle,128> clientConnectedList;
	server->TransferConnectedClientList(&clientConnectedList);

	foreach_const(cl, clientConnectedList) {
		const i32 clientID = plidMap.Push(*cl);
		clientHandle[clientID] = *cl;
	}


	// handle client disconnections
	eastl::fixed_vector<ClientHandle,128> clientDisconnectedList;
	server->TransferDisconnectedClientList(&clientDisconnectedList);

	// clear client data
	foreach_const(cl, clientDisconnectedList) {
		const i32 clientID = plidMap.Get(*cl);
		plidMap.Pop(*cl);
		clientHandle[clientID] = ClientHandle::INVALID;
	}

	// handle received data
	eastl::fixed_vector<ClientHandle,MAX_CLIENTS> clientList;
	for(i32 clientID = 0; clientID < MAX_CLIENTS; clientID++) {
		if(clientHandle[clientID] != ClientHandle::INVALID && !instancePool.IsClientInsideAnInstance(clientHandle[clientID])) {
			clientList.push_back(clientHandle[clientID]);
		}
	}

	server->TransferReceivedData(&recvDataBuff, clientList.data(), clientList.size());

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
				fileSaveBuff(FormatPath(FMT("trace/game_%d_cl_%d.raw", server->packetCounter, header.netID)), &header, header.size);
				server->packetCounter++;
			}

			const u8* packetData = reader.ReadRaw(packetDataSize);
			ClientHandlePacket(chunkInfo.clientHd, header, packetData);
		}
	}

	recvDataBuff.Clear();
}


void Coordinator::ClientHandlePacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData)
{
	const i32 clientID = plidMap.Get(clientHd);
	const i32 packetSize = header.size - sizeof(NetHeader);

#define CASE_CL(PACKET) case Cl::PACKET::NET_ID: { HandlePacket_##PACKET(clientHd, header, packetData, packetSize); } break

	switch(header.netID) {
		CASE_CL(CQ_FirstHello);
		CASE_CL(CQ_AuthenticateGameServer);

		default: {
			WARN("Unhandled packet (netID=%d size=%d)", header.netID, header.size);
			ASSERT(instancePool.IsClientInsideAnInstance(clientHd));
			instancePool.QueueRogueCoordinatorPacket(clientHd, header, packetData);
		} break;
	}

#undef CASE_CL
}

void Coordinator::ProcessMatchmakerPackets()
{
	instancePool.QueueMatchmakerPackets(matchmaker.packetQueue.data, matchmaker.packetQueue.size);

	// parse some matchmaker packets
	ConstBuffer reader(matchmaker.packetQueue.data, matchmaker.packetQueue.size);
	while(reader.CanRead(sizeof(NetHeader))) {
		const NetHeader& header = reader.Read<NetHeader>();
		const i32 packetDataSize = header.size - sizeof(NetHeader);
		ASSERT(reader.CanRead(packetDataSize));
		const u8* packetData = reader.ReadRaw(packetDataSize);

		HandleMatchmakerPacket(header, packetData, packetDataSize);
	}
	matchmaker.packetQueue.Clear();
}

void Coordinator::HandleMatchmakerPacket(const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	switch(header.netID) {
		case In::MR_Handshake::NET_ID: {
			NT_LOG("[MM] %s", PacketSerialize<In::MR_Handshake>(packetData, packetSize));
			const In::MR_Handshake resp = SafeCast<In::MR_Handshake>(packetData, packetSize);
			if(resp.result == 1) {
				LOG("[MM] Connected to Matchmaker server");
			}
			else {
				WARN("[MM] handshake failed (%d)", resp.result);
				ASSERT_MSG(0, "mm handshake failed"); // should not happen
			}
		} break;

		case In::MQ_CreateGame::NET_ID: {
			NT_LOG("[MM] %s", PacketSerialize<In::MQ_CreateGame>(packetData, packetSize));
			const In::MQ_CreateGame& packet = SafeCast<In::MQ_CreateGame>(packetData, packetSize);

			for(auto* p = packet.players.begin(); p != packet.players.begin()+packet.playerCount; ++p) {
				PendingClientEntry entry;
				entry.accountUID = p->accountUID;
				entry.sortieUID = packet.sortieUID;
				entry.time = localTime;
				entry.instantKey = In::ProduceInstantKey(p->accountUID, packet.sortieUID);
				pendingClientQueue.push_back(entry);
			}

			instancePool.QueueCreateGame(packet);
			matchmaker.QueryGameCreated(packet.sortieUID);
		} break;

		default: {
			WARN("Unknown packet (netID=%u size=%u)", header.netID, header.size);
			DBG_ASSERT(0); // packet not handled;
		} break;
	}
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
	hello.serverType = 2;
	hello.clientIp[0] = info.ip[3];
	hello.clientIp[1] = info.ip[2];
	hello.clientIp[2] = info.ip[1];
	hello.clientIp[3] = info.ip[0];
	STATIC_ASSERT(sizeof(hello.clientIp) == sizeof(info.ip));
	hello.clientPort = info.port;
	hello.tqosWorldId = 1;

	SendPacket(clientHd, hello);
}

void Coordinator::HandlePacket_CQ_AuthenticateGameServer(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_AuthenticateGameServer>(packetData, packetSize));

	ConstBuffer request(packetData, packetSize);
	const u16 nickLen = request.Read<u16>();
	const wchar* nick = (wchar*)request.ReadRaw(nickLen * sizeof(wchar));
	const u32 instantKey = request.Read<u32>();
	//i32 var2 = request.Read<i32>();
	//u8 b1 = request.Read<u8>();

	// check authentication
	AccountUID accountUID = AccountUID::INVALID;
	SortieUID sortieUID = SortieUID::INVALID;
	foreach(e, pendingClientQueue) {
		if(e->instantKey == instantKey) {
			accountUID = e->accountUID;
			sortieUID = e->sortieUID;
			pendingClientQueue.erase(e);
			break;
		}
	}

	// failed to authenticate
	if(accountUID == AccountUID::INVALID) {
		WARN("[client%x] Client failed to authenticate", clientHd);

		Sv::SA_AuthResult auth;
		auth.result = 0;
		SendPacket(clientHd, auth);
		server->DisconnectClient(clientHd);
		return;
	}

	// authentication success
	Sv::SA_AuthResult auth;
	auth.result = 91;
	SendPacket(clientHd, auth);

	LOG("[client%x] Client authenticated (accountuID=%u sortieUID=%llu)", clientHd, accountUID, sortieUID);
	instancePool.QueuePushPlayerToGame(clientHd, accountUID, sortieUID);
}

void Coordinator::CreateDevGame()
{
	// create a game to quickly connect to
	const GameXmlContent& content = GetGameXmlContent();

	const eastl::fixed_set<ClassType,100,false> allowedMastersSet = {
		ClassType::STRIKER,
		ClassType::ARTILLERY,
		ClassType::ASSASSIN,
		ClassType::ELECTRO,
		ClassType::DEFENDER,
		ClassType::SNIPER,
		ClassType::LAUNCHER,
	};

	eastl::array<eastl::array<u8,100>,2> teamMasterPickCount;
	memset(&teamMasterPickCount, 0x0, sizeof(teamMasterPickCount));

	In::MQ_CreateGame game;
	game.sortieUID = SortieUID(1);
	game.playerCount = 6;
	game.spectatorCount = 0;

	auto& p = game.players[0];
	p.name.Copy(WideString(L"LordSk")); // TODO: we really need an account system (sorry Delta)
	p.accountUID = AccountUID(0x1337);
	p.team = 0;
	p.isBot = 0;
	p.masters[0] = ClassType::LAUNCHER;
	p.masters[1] = ClassType::ASSASSIN;
	teamMasterPickCount[0][(i32)ClassType::LAUNCHER] = 1;
	teamMasterPickCount[0][(i32)ClassType::ASSASSIN] = 1;
	p.skins.fill(SkinIndex::DEFAULT);
	p.skills[0] = SkillID(180350010);
	p.skills[1] = SkillID(180350030);
	p.skills[2] = SkillID(180030020);
	p.skills[3] = SkillID(180030030);

	for(int bi = 1; bi < game.playerCount; bi++) {
		auto& bot = game.players[bi];
		bot.name.Copy(WideString(LFMT(L"Bot%d", bi)));
		bot.accountUID = AccountUID::INVALID;
		bot.team = bi > 2;
		bot.isBot = true;
		bot.masters[0] = ClassType::NONE;
		bot.masters[1] = ClassType::NONE;
		bot.skins.fill(SkinIndex::DEFAULT);

		for(int attempts = 0; attempts < 10000; attempts++) {
			const u32 r0 = RandUint() % allowedMastersSet.size();

			u32 i = 0;
			foreach_const(m, allowedMastersSet) {
				ClassType classType = *m;

				if(i == r0) {
					if(teamMasterPickCount[bot.team][(i32)classType] < 2) {
						teamMasterPickCount[bot.team][(i32)classType]++;

						const GameXmlContent::Master& master = *content.masterClassTypeMap.at(classType);
						if(bot.masters[0] == ClassType::NONE) {
							bot.masters[0] = classType;
							bot.skills[0] = master.skillIDs[0];
							bot.skills[1] = master.skillIDs[1];
						}
						else if(bot.masters[0] != classType){
							bot.masters[1] = classType;
							bot.skills[2] = master.skillIDs[0];
							bot.skills[3] = master.skillIDs[1];
						}
					}
					break;
				}
				i++;
			}

			if(bot.masters[0] != ClassType::NONE && bot.masters[1] != ClassType::NONE) {
				break;
			}
		}
	}

	NetHeader header;
	header.size = sizeof(NetHeader) + sizeof(game);
	header.netID = decltype(game)::NET_ID;
	matchmaker.packetQueue.Append(&header, sizeof(header));
	matchmaker.packetQueue.Append(&game, sizeof(game));
}
