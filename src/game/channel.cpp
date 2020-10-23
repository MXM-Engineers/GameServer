#include "channel.h"

#include "coordinator.h"
#include "game_content.h"
#include "gm_3v3.h"

intptr_t ThreadChannel(void* pData)
{
	Channel& channel = *(Channel*)pData;

	ProfileSetThreadName(FMT("Channel_%d", 0)); // TODO: channel ID / name
	const i32 cpuID = (i32)CoreAffinity::CHANNELS + 0; // TODO: increase this for each channel
	EA::Thread::SetThreadAffinityMask((EA::Thread::ThreadAffinityMask)1 << cpuID);

	const f64 UPDATE_RATE_MS = (1.0/60.0) * 1000.0;
	const Time startTime = TimeNow();
	Time t0 = startTime;

	while(channel.server->running)
	{
		Time t1 = TimeNow();
		channel.localTime = TimeDiff(startTime, t1);
		f64 delta = TimeDiffMs(TimeDiff(t0, t1));

		if(delta > UPDATE_RATE_MS) {
			channel.Update(delta / 1000.0);
			t0 = t1;
		}
		else {
			EA::Thread::ThreadSleep(UPDATE_RATE_MS - delta);
			// EA::Thread::ThreadSleep(EA::Thread::kTimeoutYield);
			// Sleep on windows is notoriously innacurate, we'll probably need to "just yield"
		}
	}

	return 0;
}

// TODO: replace listener type later on, this is just convenient for now
bool Channel::Init(Server* server_, ListenerType type)
{
	server = server_;
	replication.Init(server_);

	switch(type) {
		case ListenerType::LOBBY: game = new Game(); break;
		case ListenerType::GAME: game = new Game3v3(); break;
		default: ASSERT(0); //case not handled
	}

	game->Init(&replication);

	packetDataQueue.Init(10 * (1024*1024)); // 10 MB
	processPacketQueue.Init(10 * (1024*1024)); // 10 MB

	thread.Begin(ThreadChannel, this);
	return true;
}

void Channel::Cleanup()
{
	LOG("Channel cleanup...");
	thread.WaitForEnd();
}

void Channel::Update(f64 delta)
{
	ProfileFunction();

	// clients disconnected
	{
		const LockGuard lock(mutexClientDisconnectedList);
		foreach(it, clientDisconnectedList) {
			const i32 clientID = *it;
			game->OnPlayerDisconnect(clientID);
			replication.EventClientDisconnect(clientID);
		}
		clientDisconnectedList.clear();
	}

	// clients connected
	{
		const LockGuard lock(mutexNewPlayerQueue);
		foreach(it, newPlayerQueue) {
			game->OnPlayerConnect(it->clientID, it->accountData);
			replication.EventPlayerConnect(it->clientID);
		}
		newPlayerQueue.clear();
	}

	// process packets
	processPacketQueue.Clear();
	{
		const LockGuard lock(mutexPacketDataQueue);
		processPacketQueue.Append(packetDataQueue.data, packetDataQueue.size);
		packetDataQueue.Clear();
	}

	ConstBuffer buff(processPacketQueue.data, processPacketQueue.size);
	while(buff.CanRead(4)) {
		const i32 clientID = buff.Read<i32>();
		const NetHeader& header = buff.Read<NetHeader>();
		const u8* packetData = buff.ReadRaw(header.size - sizeof(NetHeader));
		ClientHandlePacket(clientID, header, packetData);
	}

	game->Update(delta, localTime);
	replication.FrameEnd();
}

void Channel::CoordinatorRegisterNewPlayer(i32 clientID, const AccountData* accountData)
{
	LOG("[client%03d] Channel:: New player :: '%ls'", clientID, accountData->nickname.data());

	const LockGuard lock(mutexNewPlayerQueue);
	newPlayerQueue.push_back(EventOnClientConnect{ clientID, accountData });
}

void Channel::CoordinatorClientHandlePacket(i32 clientID, const NetHeader& header, const u8* packetData)
{
	const LockGuard lock(mutexPacketDataQueue);
	packetDataQueue.Append(&clientID, sizeof(clientID));
	packetDataQueue.Append(&header, sizeof(header));
	packetDataQueue.Append(packetData, header.size - sizeof(NetHeader));
}

void Channel::CoordinatorHandleDisconnectedClients(i32* clientIDList, const i32 count)
{
	const LockGuard lock(mutexClientDisconnectedList);
	eastl::copy(clientIDList, clientIDList+count, eastl::back_inserter(clientDisconnectedList));
}

void Channel::ClientHandlePacket(i32 clientID, const NetHeader& header, const u8* packetData)
{
	const i32 packetSize = header.size - sizeof(NetHeader);

#define HANDLE_CASE(PACKET) case Cl::PACKET::NET_ID: { HandlePacket_##PACKET(clientID, header, packetData, packetSize); } break

	switch(header.netID) {
		HANDLE_CASE(CN_ReadyToLoadCharacter);
		HANDLE_CASE(CN_ReadyToLoadGameMap);
		HANDLE_CASE(CA_SetGameGvt);
		HANDLE_CASE(CN_MapIsLoaded);
		HANDLE_CASE(CN_GameMapLoaded);
		HANDLE_CASE(CQ_GetCharacterInfo);
		HANDLE_CASE(CN_UpdatePosition);
		HANDLE_CASE(CN_ChannelChatMessage);
		HANDLE_CASE(CQ_SetLeaderCharacter);
		HANDLE_CASE(CN_GamePlayerSyncActionStateOnly);
		HANDLE_CASE(CQ_JukeboxQueueSong);
		HANDLE_CASE(CQ_WhisperSend);
		HANDLE_CASE(CQ_PartyCreate);
		HANDLE_CASE(CQ_RTT_Time);
		HANDLE_CASE(CQ_LoadingProgressData);
		HANDLE_CASE(CQ_LoadingComplete);
		HANDLE_CASE(CQ_GameIsReady);

		default: {
			LOG("[client%03d] Client :: Unknown packet :: size=%d netID=%d", clientID, header.size, header.netID);
		} break;
	}

#undef HANDLE_CASE
}

void Channel::HandlePacket_CN_ReadyToLoadCharacter(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	LOG("[client%03d] Client :: CN_ReadyToLoadCharacter ::", clientID);
	game->OnPlayerReadyToLoad(clientID);
}

void Channel::HandlePacket_CN_ReadyToLoadGameMap(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	LOG("[client%03d] Client :: CN_ReadyToLoadGame ::", clientID);
	game->OnPlayerReadyToLoad(clientID);
}

void Channel::HandlePacket_CA_SetGameGvt(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CA_SetGameGvt& gvt = SafeCast<Cl::CA_SetGameGvt>(packetData, packetSize);
	LOG("[client%03d] Client :: CA_SetGameGvt :: sendTime=%d virtualTime=%d unk=%d", clientID, gvt.sendTime, gvt.virtualTime, gvt.unk);
}

void Channel::HandlePacket_CN_MapIsLoaded(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	LOG("[client%03d] Client :: CN_MapIsLoaded ::", clientID);
	replication.SetPlayerAsInGame(clientID);
}

void Channel::HandlePacket_CN_GameMapLoaded(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	LOG("[client%03d] Client :: CN_GameMapLoaded ::", clientID);
	game->OnPlayerGameMapLoaded(clientID);
	replication.SetPlayerAsInGame(clientID);
}

void Channel::HandlePacket_CQ_GetCharacterInfo(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_GetCharacterInfo& req = SafeCast<Cl::CQ_GetCharacterInfo>(packetData, packetSize);
	LOG("[client%03d] Client :: CQ_GetCharacterInfo :: characterID=%d", clientID, (u32)req.characterID);

	ActorUID actorUID = replication.GetWorldActorUID(clientID, req.characterID);
	if(actorUID == ActorUID::INVALID) {
		WARN("Client sent an invalid actor (localActorID=%d)", req.characterID);
		return;
	}

	game->OnPlayerGetCharacterInfo(clientID, actorUID);
}

void Channel::HandlePacket_CN_UpdatePosition(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CN_UpdatePosition& update = SafeCast<Cl::CN_UpdatePosition>(packetData, packetSize);
	LOG("[client%03d] Client :: CN_UpdatePosition :: { characterID=%d p3nPos=(%g, %g, %g) p3nDir=(%g, %g, %g) p3nEye=(%g, %g, %g) nRotate=%g nSpeed=%g nState=%d nActionIDX=%d", clientID, (u32)update.characterID, update.p3nPos.x, update.p3nPos.y, update.p3nPos.z, update.p3nDir.x, update.p3nDir.y, update.p3nDir.z, update.p3nEye.x, update.p3nEye.y, update.p3nEye.z, update.nRotate, update.nSpeed, (i32)update.nState, update.nActionIDX);

	ActorUID actorUID = replication.GetWorldActorUID(clientID, update.characterID);
	if(actorUID == ActorUID::INVALID) {
		WARN("Client sent an invalid actor (localActorID=%d)", update.characterID);
		return;
	}

	game->OnPlayerUpdatePosition(clientID, actorUID, update.p3nPos, update.p3nDir, update.p3nEye, update.nRotate, update.nSpeed, update.nState, update.nActionIDX);
}

void Channel::HandlePacket_CN_ChannelChatMessage(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	ConstBuffer buff(packetData, packetSize);
	i32 chatType = buff.Read<i32>();
	const u16 msgLen = buff.Read<u16>();
	const wchar* msg = (wchar*)buff.ReadRaw(msgLen * 2);

	LOG("[client%03d] Client :: CN_ChannelChatMessage :: chatType=%d msg='%.*S'", clientID, chatType, msgLen, msg);

	game->OnPlayerChatMessage(clientID, chatType, msg, msgLen);
}

void Channel::HandlePacket_CQ_SetLeaderCharacter(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_SetLeaderCharacter& leader = SafeCast<Cl::CQ_SetLeaderCharacter>(packetData, packetSize);
	LOG("[client%03d] Client :: CQ_SetLeaderCharacter :: characterID=%d skinIndex=%d", clientID, (u32)leader.characterID, (i32)leader.skinIndex);

	game->OnPlayerSetLeaderCharacter(clientID, leader.characterID, leader.skinIndex);
}

void Channel::HandlePacket_CN_GamePlayerSyncActionStateOnly(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CN_GamePlayerSyncActionStateOnly& sync = SafeCast<Cl::CN_GamePlayerSyncActionStateOnly>(packetData, packetSize);

	i32 state = (i32)sync.state;
	const char* stateStr = g_ActionStateInvalidString;
	if(state >= 0 && state < ARRAY_COUNT(g_ActionStateString)) {
		stateStr = g_ActionStateString[state];
	}

	LOG("[client%03d] Client :: CN_GamePlayerSyncActionStateOnly :: {", clientID);
	LOG("	characterID=%d", (u32)sync.characterID);
	LOG("	nState=%d (%s)", (i32)sync.state, stateStr);
	LOG("	bApply=%d", sync.bApply);
	LOG("	param1=%d", sync.param1);
	LOG("	param2=%d", sync.param2);
	LOG("	i4=%d", sync.i4);
	LOG("	rotate=%g", sync.rotate);
	LOG("	upperRotate=%g", sync.upperRotate);
	LOG("}");

	ActorUID actorUID = replication.GetWorldActorUID(clientID, sync.characterID);
	if(actorUID == ActorUID::INVALID) {
		WARN("Client sent an invalid actor (localActorID=%d)", sync.characterID);
		return;
	}

	game->OnPlayerSyncActionState(clientID, actorUID, sync.state, sync.param1, sync.param2, sync.rotate, sync.upperRotate);
}

void Channel::HandlePacket_CQ_JukeboxQueueSong(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_JukeboxQueueSong& queue = SafeCast<Cl::CQ_JukeboxQueueSong>(packetData, packetSize);

	LOG("[client%03d] Client :: CQ_JukeboxQueueSong :: { songID=%d }", clientID, (i32)queue.songID);

	game->OnPlayerJukeboxQueueSong(clientID, queue.songID);
}

void Channel::HandlePacket_CQ_WhisperSend(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	ConstBuffer buff(packetData, packetSize);
	WideString destNick;
	eastl::fixed_string<wchar,256,true> msg;

	const u16 destNick_len = buff.Read<u16>();
	const wchar* destNick_str = (wchar*)buff.ReadRaw(destNick_len * sizeof(wchar));
	const u16 msg_len = buff.Read<u16>();
	const wchar* msg_str = (wchar*)buff.ReadRaw(msg_len * sizeof(wchar));

	destNick.assign(destNick_str, destNick_len);
	msg.assign(msg_str, msg_len);

	game->OnPlayerChatWhisper(clientID, destNick.data(), msg.data());
}

void Channel::HandlePacket_CQ_PartyCreate(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_PartyCreate& create = SafeCast<Cl::CQ_PartyCreate>(packetData, packetSize);

	LOG("[client%03d] Client :: CQ_PartyCreate :: { someID=%d stageType=%d }", clientID, create.someID, create.stageType);

	// we don't support creating parties right now, send back an error

	u8 sendData[2048];
	PacketWriter packet(sendData, sizeof(sendData));

	packet.Write<i32>(175); // retval (ERROR_TYPE_PARTY_CREATE_PENALTY_TIME) <- this one is silent
	packet.Write<i32>(0); // ownerUserID
	packet.Write<i32>(create.stageType); // stageType

	LOG("[client%03d] Server :: SA_PartyCreate :: NO", clientID);
	server->SendPacketData(clientID, Sv::SA_PartyCreate::NET_ID, packet.size, packet.data);
}

void Channel::HandlePacket_CQ_RTT_Time(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_RTT_Time& rtt = SafeCast<Cl::CQ_RTT_Time>(packetData, packetSize);
	LOG("[client%03d] Client :: CQ_RTT_Time :: { time=%u }", clientID, rtt.time);

	Sv::SA_RTT_Time answer;
	answer.clientTimestamp = rtt.time;
	answer.serverTimestamp = (i64)TimeDiffMs(TimeRelNow());
	LOG("[client%03d] Server :: SA_RTT_Time ::", clientID);
	server->SendPacket(clientID, answer);
}

void Channel::HandlePacket_CQ_LoadingProgressData(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_LoadingProgressData& loading = SafeCast<Cl::CQ_LoadingProgressData>(packetData, packetSize);
	LOG("[client%03d] Client :: CQ_LoadingProgressData :: { progress=%u }", clientID, loading.progress);
}

void Channel::HandlePacket_CQ_LoadingComplete(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	LOG("[client%03d] Client :: CQ_LoadingComplete", clientID);
	game->OnPlayerLoadingComplete(clientID);
}

void Channel::HandlePacket_CQ_GameIsReady(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	LOG("[client%03d] Client :: CQ_GameIsReady", clientID);
	game->OnPlayerGameIsReady(clientID);
}
