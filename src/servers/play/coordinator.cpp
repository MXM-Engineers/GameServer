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
	Lane& lane = *(Lane*)pData;
	ProfileSetThreadName(FMT("Lane_%d", lane.laneIndex));
	const i32 cpuID = (i32)CoreAffinity::LANES + lane.laneIndex;
	EA::Thread::SetThreadAffinityMask((EA::Thread::ThreadAffinityMask)1 << cpuID);

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
	EA::Thread::SetThreadAffinityMask(1 << (i32)CoreAffinity::COORDINATOR);

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
			coordinator.Update(delta / 1000.0);
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

void Lane::Init(Server* server_)
{
	server = server_;

	recvDataBuff.Init(10 * (1024*1024)); // 10 MB
	packetDataQueue.Init(10 * (1024*1024)); // 10 MB
	processPacketQueue.Init(10 * (1024*1024)); // 10 MB

	// TODO: several of these
	instance = new PvpInstance();
	instance->Init(server);
}

void Lane::Update()
{
	// TODO: handle multiple instances

	eastl::fixed_vector<ClientHandle,MAX_CLIENTS> disconnectedClientList;
	eastl::fixed_vector<eastl::pair<ClientHandle, const AccountData*>,MAX_CLIENTS> newClientList;

	{ LOCK_MUTEX(mutexClientDisconnectedList);
		foreach_const(n, clientDisconnectedList) {
			clientSet.erase(*n);
			disconnectedClientList.push_back(*n);
		}
		clientDisconnectedList.clear();
	}

	{ LOCK_MUTEX(mutexNewPlayerQueue);
		foreach_const(n, newPlayerQueue) {
			ASSERT(clientSet.find(n->clientHd) == clientSet.end());
			clientSet.insert(n->clientHd);
			newClientList.push_back({ n->clientHd, n->accountData });
		}
		newPlayerQueue.clear();
	}

	// TODO: move this to game probably?
	instance->OnNewClientsDisconnected(disconnectedClientList.data(), disconnectedClientList.size());
	instance->OnNewClientsConnected(newClientList.data(), newClientList.size());

	{ LOCK_MUTEX(mutexPacketDataQueue);
		recvDataBuff.Append(packetDataQueue.data, packetDataQueue.size);
		packetDataQueue.Clear();
	}

	eastl::fixed_vector<ClientHandle,MAX_CLIENTS> clientList;
	eastl::copy(clientSet.begin(), clientSet.end(), eastl::back_inserter(clientList));

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

			const u8* packetData = reader.ReadRaw(packetDataSize);
			instance->OnNewPacket(chunkInfo.clientHd, header, packetData);
		}
	}

	recvDataBuff.Clear();

	instance->Update(localTime);
}

void Lane::Cleanup()
{
	// instance->game.Cleanup();
}

void Lane::CoordinatorRegisterNewPlayer(ClientHandle clientHd, const AccountData* accountData)
{
	LOG("[Lane%d][client%x] New player", laneIndex, clientHd);

	LOCK_MUTEX(mutexNewPlayerQueue);
	newPlayerQueue.push_back(PlayerTransit{ clientHd, accountData });
}

void Lane::CoordinatorClientHandlePacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData)
{
	LOCK_MUTEX(mutexPacketDataQueue);

	Server::RecvChunkHeader chunkInfo;
	chunkInfo.clientHd = clientHd;
	chunkInfo.len = header.size;
	packetDataQueue.Append(&chunkInfo, sizeof(chunkInfo));
	packetDataQueue.Append(&header, sizeof(header));
	packetDataQueue.Append(packetData, header.size - sizeof(NetHeader));
}

void Lane::CoordinatorHandleDisconnectedClients(ClientHandle* clientIDList, const i32 count)
{
	LOCK_MUTEX(mutexClientDisconnectedList);
	eastl::copy(clientIDList, clientIDList+count, eastl::back_inserter(clientDisconnectedList));
}

bool Matchmaker::Init()
{
	// TODO: load this from somewhere
	const u8 ip[4] = { 127, 0, 0, 1 };
	const u16 port = 13900;
	bool r = conn.async.ConnectTo(ip, port);
	if(!r) {
		LOG("ERROR: Failed to connect to matchmaker server");
		return false;
	}

	conn.async.StartReceiving(); // TODO: move this to ConnectTo()?

	In::PQ_Handshake handshake;
	handshake.magic = In::MagicHandshake;
	conn.SendPacket(handshake);
	return true;
}

void Matchmaker::Update()
{
	// handle inner communication
	conn.SendPendingData();

	u8 recvBuff[8192];
	i32 recvLen = 0;
	conn.RecvPendingData(recvBuff, sizeof(recvBuff), &recvLen);

	if(recvLen > 0) {
		ConstBuffer reader(recvBuff, recvLen);

		while(reader.CanRead(sizeof(NetHeader))) {
			const NetHeader& header = reader.Read<NetHeader>();
			const i32 packetDataSize = header.size - sizeof(NetHeader);
			ASSERT(reader.CanRead(packetDataSize));
			const u8* packetData = reader.ReadRaw(packetDataSize);

			HandlePacket(header, packetData);
		}
	}
}

void Matchmaker::HandlePacket(const NetHeader& header, const u8* packetData)
{
	const i32 packetSize = header.size - sizeof(NetHeader);

	switch(header.netID) {
		case In::MR_Handshake::NET_ID: {
			const In::MR_Handshake resp = SafeCast<In::MR_Handshake>(packetData, packetSize);
			if(resp.result == 1) {
				LOG("[MM] Connected to Matchmaker server");
			}
			else {
				WARN("[MM] handshake failed (%d)", resp.result);
				ASSERT_MSG(0, "mm handshake failed"); // should not happen
			}
		} break;

		case In::MQ_CreatePlaySession::NET_ID: {
			const In::MQ_CreatePlaySession& packet = SafeCast<In::MQ_CreatePlaySession>(packetData, packetSize);
			QueryCreateSession query;
			query.players = packet.players;
			queueQueryCreateSession.push_back(query);
		} break;

		default: {
			WARN("Unknown packet (netID=%u size=%u)", header.netID, header.size);
			ASSERT_MSG(0, "packet not handled");
		} break;
	}
}

void Coordinator::Init(Server* server_)
{
	matchmaker.Init();

	server = server_;
	recvDataBuff.Init(10 * (1024*1024)); // 10 MB

	associatedLane.fill(LaneID::NONE);
	clientHandle.fill(ClientHandle::INVALID);

	i32 laneIndex = 0;
	foreach(it, lanes) {
		it->Init(server);
		it->laneIndex = laneIndex++;
		it->thread.Begin(ThreadLane, &(*it));
	}

	thread.Begin(ThreadCoordinator, this);
}

void Coordinator::Cleanup()
{
	LOG("Coordinator cleanup...");

	foreach(it, lanes) {
		it->thread.WaitForEnd();
	}

	thread.WaitForEnd();
}

void Coordinator::Update(f64 delta)
{
	ProfileFunction();

	matchmaker.Update();

	// handle client connections
	eastl::fixed_vector<ClientHandle,128> clientConnectedList;
	server->TransferConnectedClientList(&clientConnectedList);

	foreach_const(cl, clientConnectedList) {
		const i32 clientID = plidMap.Push(*cl);
		associatedLane[clientID] = LaneID::NONE;
		clientHandle[clientID] = *cl;
	}


	// handle client disconnections
	eastl::fixed_vector<ClientHandle,128> clientDisconnectedList;
	server->TransferDisconnectedClientList(&clientDisconnectedList);

	for(i32 i = (i32)LaneID::FIRST; i < (i32)LaneID::_COUNT; i++) {
		eastl::fixed_vector<ClientHandle,128> chanDiscList;

		foreach_const(cl, clientDisconnectedList) {
			if(associatedLane[plidMap.Get(*cl)] == (LaneID)i) {
				chanDiscList.push_back(*cl);
			}
		}

		lanes[i].CoordinatorHandleDisconnectedClients(chanDiscList.data(), chanDiscList.size());
	}

	// clear client data
	foreach_const(cl, clientDisconnectedList) {
		const i32 clientID = plidMap.Get(*cl);
		plidMap.Pop(*cl);
		associatedLane[clientID] = LaneID::NONE;
		clientHandle[clientID] = ClientHandle::INVALID;

	}

	// handle received data
	eastl::fixed_vector<ClientHandle,MAX_CLIENTS> clientList;
	for(i32 clientID = 0; clientID < MAX_CLIENTS; clientID++) {
		if(clientHandle[clientID] != ClientHandle::INVALID && associatedLane[clientID] == LaneID::NONE) {
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
		CASE_CL(CQ_GetGuildProfile);
		CASE_CL(CQ_GetGuildMemberList);
		CASE_CL(CQ_GetGuildHistoryList);
		CASE_CL(CQ_GetGuildRankingSeasonList);
		CASE_CL(CQ_TierRecord);

		default: {
				ASSERT(associatedLane[clientID] != LaneID::NONE);
				lanes[(i32)associatedLane[clientID]].CoordinatorClientHandlePacket(clientHd, header, packetData);
		} break;
	}

#undef CASE_CL
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
	ConstBuffer request(packetData, packetSize);
	const u16 nickLen = request.Read<u16>();
	const wchar* nick = (wchar*)request.ReadRaw(nickLen * sizeof(wchar));
	i32 var = request.Read<i32>();
	i32 var2 = request.Read<i32>();
	u8 b1 = request.Read<u8>();
	NT_LOG("[client%x] Client :: CQ_AuthenticateGameServer :: %.*ls var=%d var2=%d b1=%d", clientHd, nickLen, nick, var, var2, b1);

	const i32 clientID = plidMap.Get(clientHd);
	const Server::ClientInfo& info = server->clientInfo[clientID];

	// TODO: check authentication

	// assign to a channel
	ASSERT(associatedLane[clientID] == LaneID::NONE);
	associatedLane[clientID] = LaneID::FIRST;

	// send authentication result
	Sv::SA_AuthResult auth;
	auth.result = 91;
	SendPacket(clientHd, auth);


	// TODO: fetch account data
	AccountData& account = accountData[clientID];
	account = {};
	account.nickname.assign(nick, nickLen);
	account.guildTag = L"Alpha";
	account.leaderMasterID = 0; // Lua

	// remove the @plaync... part
	i64 f = account.nickname.find(L'@');
	if(f != -1) {
		account.nickname = account.nickname.left(f);
	}

	// send account data
	ClientSendAccountData(clientHd);

	// register new player to the game
	ASSERT(associatedLane[clientID] != LaneID::NONE);
	lanes[(i32)associatedLane[clientID]].CoordinatorRegisterNewPlayer(clientHd, &account);
}

void Coordinator::HandlePacket_CQ_GetGuildProfile(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%x] Client :: CQ_GetGuildProfile ::", clientHd);

	// SA_GetGuildProfile
	{
		PacketWriter<Sv::SA_GetGuildProfile,1024> packet;

		packet.Write<i32>(0); // result;
		packet.WriteStringObj(L"Alpha testers"); // guildName
		packet.WriteStringObj(L"Alpha"); // guildTag
		packet.Write<i32>(100203); // emblemIndex
		packet.Write<u8>(10); // guildLvl
		packet.Write<u8>(120); // memberMax
		packet.WriteStringObj(L"Malachi"); // ownerNickname
		packet.Write<i64>(131474874000000000); // createdDate
		packet.Write<i64>(0); // dissolutionDate
		packet.Write<u8>(0); // joinType

		Sv::SA_GetGuildProfile::ST_GuildInterest guildInterest;
		guildInterest.likePveStage = 1;
		guildInterest.likeDefence = 1;
		guildInterest.likePvpNormal = 1;
		guildInterest.likePvpOccupy = 1;
		guildInterest.likePvpGot = 1;
		guildInterest.likePvpRank = 1;
		guildInterest.likeOlympic = 1;
		packet.Write(guildInterest);

		packet.WriteStringObj(L"This is a great intro"); // guildIntro
		packet.WriteStringObj(L"Notice: this game is dead! (for now)"); // guildNotice
		packet.Write<i32>(460281); // guildPoint
		packet.Write<i32>(9999); // guildFund

		Sv::SA_GetGuildProfile::ST_GuildPvpRecord guildPvpRecord;
		guildPvpRecord.rp = 5;
		guildPvpRecord.win = 4;
		guildPvpRecord.draw = 3;
		guildPvpRecord.lose = 2;
		packet.Write(guildPvpRecord);

		packet.Write<i32>(-1); // guildRankNo

		packet.Write<u16>(1); // guildMemberClassList_count
		// guildMemberClassList[0]
		packet.Write<i32>(12456); // id
		packet.Write<u8>(3); // type
		packet.Write<u8>(2); // iconIndex
		packet.WriteStringObj(L"Malachi");

		Sv::SA_GetGuildProfile::ST_GuildMemberRights rights;
		rights.hasInviteRight = 1;
		rights.hasExpelRight = 1;
		rights.hasMembershipChgRight = 1;
		rights.hasClassAssignRight = 1;
		rights.hasNoticeChgRight = 1;
		rights.hasIntroChgRight = 1;
		rights.hasInterestChgRight = 1;
		rights.hasFundManageRight = 1;
		rights.hasJoinTypeRight = 1;
		rights.hasEmblemRight = 1;
		packet.Write(rights);

		packet.Write<u16>(1); // guildSkills_count
		// guildSkills[0]
		packet.Write<u8>(1); // type
		packet.Write<u8>(9); // level
		packet.Write<i64>(0); // expiryDate
		packet.Write<u16>(0); // extensionCount

		packet.Write<i32>(7); // curDailyStageGuildPoint
		packet.Write<i32>(500); // maxDailyStageGuildPoint
		packet.Write<i32>(2); // curDailyArenaGuildPoint
		packet.Write<i32>(450); // maxDailyArenaGuildPoint
		packet.Write<u8>(1); // todayRollCallCount

		SendPacket(clientHd, packet);
	}
}

void Coordinator::HandlePacket_CQ_GetGuildMemberList(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%x] Client :: CQ_GetGuildMemberList ::", clientHd);

	// SA_GetGuildMemberList
	{
		PacketWriter<Sv::SA_GetGuildMemberList,512> packet;

		packet.Write<i32>(0); // result

		packet.Write<u16>(3); // guildMemberProfileList_count

		// guildMemberProfileList[0]
		packet.WriteStringObj(L"Malachi");
		packet.Write<i32>(0);  // membershipID
		packet.Write<u16>(99); // lvl
		packet.Write<u16>(10); // leaderClassType
		packet.Write<u16>(27); // masterCount
		packet.Write<i32>(12455); // achievementScore
		packet.Write<u8>(0); // topPvpTierGrade
		packet.Write<u16>(0); // topPvpTierPoint
		packet.Write<i32>(16965); // contributedGuildPoint
		packet.Write<i32>(60047); // contributedGuildFund
		packet.Write<u16>(0); // guildPvpWin
		packet.Write<u16>(0); // guildPvpPlay
		packet.Write<i64>((i64)131568669600000000); // lastLogoutDate

		// guildMemberProfileList[1]
		packet.WriteStringObj(L"Delta-47");
		packet.Write<i32>(0);  // membershipID
		packet.Write<u16>(99); // lvl
		packet.Write<u16>(10); // leaderClassType
		packet.Write<u16>(27); // masterCount
		packet.Write<i32>(12455); // achievementScore
		packet.Write<u8>(0); // topPvpTierGrade
		packet.Write<u16>(0); // topPvpTierPoint
		packet.Write<i32>(16965); // contributedGuildPoint
		packet.Write<i32>(60047); // contributedGuildFund
		packet.Write<u16>(0); // guildPvpWin
		packet.Write<u16>(0); // guildPvpPlay
		packet.Write<i64>((i64)131568669600000000); // lastLogoutDate

		// guildMemberProfileList[2]
		packet.WriteStringObj(L"LordSk");
		packet.Write<i32>(0);  // membershipID
		packet.Write<u16>(99); // lvl
		packet.Write<u16>(10); // leaderClassType
		packet.Write<u16>(27); // masterCount
		packet.Write<i32>(12455); // achievementScore
		packet.Write<u8>(0); // topPvpTierGrade
		packet.Write<u16>(0); // topPvpTierPoint
		packet.Write<i32>(16965); // contributedGuildPoint
		packet.Write<i32>(60047); // contributedGuildFund
		packet.Write<u16>(0); // guildPvpWin
		packet.Write<u16>(0); // guildPvpPlay
		packet.Write<i64>((i64)131568669600000000); // lastLogoutDate

		SendPacket(clientHd, packet);
	}
}

void Coordinator::HandlePacket_CQ_GetGuildHistoryList(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%x] Client :: CQ_GetGuildHistoryList ::", clientHd);

	// SA_GetGuildMemberList
	{
		PacketWriter<Sv::SA_GetGuildHistoryList> packet;

		packet.Write<i32>(0); // result
		packet.Write<u16>(0); // guildHistories_count

		SendPacket(clientHd, packet);
	}
}

void Coordinator::HandlePacket_CQ_GetGuildRankingSeasonList(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_GetGuildRankingSeasonList& rank = SafeCast<Cl::CQ_GetGuildRankingSeasonList>(packetData, packetSize);
	NT_LOG("[client%x] Client :: CQ_GetGuildRankingSeasonList :: rankingType=%d", clientHd, rank.rankingType);

	// SA_GetGuildRankingSeasonList
	{
		PacketWriter<Sv::SA_GetGuildRankingSeasonList> packet;

		packet.Write<i32>(0); // result
		packet.Write<u8>(rank.rankingType); // result
		packet.Write<u16>(0); // rankingSeasonList_count


		SendPacket(clientHd, packet);
	}
}

void Coordinator::HandlePacket_CQ_TierRecord(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%x] Client :: CQ_TierRecord ::", clientHd);

	// SA_TierRecord
	{
		PacketWriter<Sv::SA_TierRecord> packet;

		packet.Write<u8>(1); // seasonId
		packet.Write<i32>(0); // allTierWin
		packet.Write<i32>(0); // allTierDraw
		packet.Write<i32>(0); // allTierLose
		packet.Write<i32>(0); // allTierLeave
		packet.Write<u16>(0); // stageRecordList_count

		SendPacket(clientHd, packet);
	}
}

void Coordinator::ClientSendAccountData(ClientHandle clientHd)
{
	// Send account data
	const i32 clientID = plidMap.Get(clientHd);
	const AccountData& account = accountData[clientID];

	// SN_ClientSettings
	{
		PacketWriter<Sv::SN_ClientSettings,4096> packet;

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
		PacketWriter<Sv::SN_ClientSettings,4096> packet;

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
		PacketWriter<Sv::SN_ClientSettings,4096> packet;

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
