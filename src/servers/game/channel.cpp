#include "channel.h"
#include <common/packet_serialize.h>

#include "coordinator.h"
#include <mxm/game_content.h>
#include "game.h"

bool ChannelPvP::Init(Server* server_)
{
	server = server_;
	replication.Init(server_);

	game = new Game();
	game->Init(&replication);

	clientTime.fill({});
	return true;
}

void ChannelPvP::Cleanup()
{
	LOG("Channel cleanup...");
}

void ChannelPvP::Update()
{
	ProfileFunction();

	// clients disconnected
	if(!lane->clientDisconnectedList.empty()) {
		LOCK_MUTEX(lane->mutexClientDisconnectedList);
		foreach(it, lane->clientDisconnectedList) {
			const i32 clientID = *it;
			game->OnPlayerDisconnect(clientID);
			replication.EventClientDisconnect(clientID);
		}
		lane->clientDisconnectedList.clear();
	}

	// clients connected
	if(!lane->newPlayerQueue.empty()) {
		LOCK_MUTEX(lane->mutexNewPlayerQueue);
		foreach(it, lane->newPlayerQueue) {
			game->OnPlayerConnect(it->clientID, it->accountData);
			replication.OnPlayerConnect(it->clientID, it->accountData);
		}
		lane->newPlayerQueue.clear();
	}

	// process packets
	lane->processPacketQueue.Clear();
	if(lane->packetDataQueue.size > 0) {
		LOCK_MUTEX(lane->mutexPacketDataQueue);
		lane->processPacketQueue.Append(lane->packetDataQueue.data, lane->packetDataQueue.size);
		lane->packetDataQueue.Clear();
	}

	ConstBuffer buff(lane->processPacketQueue.data, lane->processPacketQueue.size);
	while(buff.CanRead(4)) {
		const i32 clientID = buff.Read<i32>();
		const NetHeader& header = buff.Read<NetHeader>();
		const u8* packetData = buff.ReadRaw(header.size - sizeof(NetHeader));
		ClientHandlePacket(clientID, header, packetData);
	}

	game->Update(localTime);
	replication.FrameEnd();
}

void ChannelPvP::ClientHandlePacket(i32 clientID, const NetHeader& header, const u8* packetData)
{
	const i32 packetSize = header.size - sizeof(NetHeader);

#define CASE(PACKET) case Cl::PACKET::NET_ID: { HandlePacket_##PACKET(clientID, header, packetData, packetSize); } break

	switch(header.netID) {
		CASE(CN_ReadyToLoadGameMap);
		CASE(CA_SetGameGvt);
		CASE(CN_GameMapLoaded);
		CASE(CQ_GetCharacterInfo);
		CASE(CN_GameUpdatePosition);
		CASE(CN_GameUpdateRotation);
		CASE(CN_ChannelChatMessage);
		CASE(CQ_SetLeaderCharacter);
		CASE(CN_GamePlayerSyncActionStateOnly);
		CASE(CQ_WhisperSend);
		CASE(CQ_RTT_Time);
		CASE(CQ_LoadingProgressData);
		CASE(CQ_LoadingComplete);
		CASE(CQ_GameIsReady);
		CASE(CQ_GamePlayerTag);
		CASE(CQ_PlayerJump);
		CASE(CQ_PlayerCastSkill);

		default: {
			NT_LOG("[client%03d] Client :: Unknown packet :: size=%d netID=%d", clientID, header.size, header.netID);
		} break;
	}

#undef HANDLE_CASE
}

void ChannelPvP::HandlePacket_CN_ReadyToLoadGameMap(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%03d] Client :: CN_ReadyToLoadGame ::", clientID);
	game->OnPlayerReadyToLoad(clientID);
}

void ChannelPvP::HandlePacket_CA_SetGameGvt(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CA_SetGameGvt& gvt = SafeCast<Cl::CA_SetGameGvt>(packetData, packetSize);
	NT_LOG("[client%03d] Client :: CA_SetGameGvt :: sendTime=%d virtualTime=%d unk=%d", clientID, gvt.sendTime, gvt.virtualTime, gvt.unk);
}

void ChannelPvP::HandlePacket_CN_GameMapLoaded(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%03d] Client :: CN_GameMapLoaded ::", clientID);
	game->OnPlayerGameMapLoaded(clientID);
	replication.SetPlayerAsInGame(clientID);
}

void ChannelPvP::HandlePacket_CQ_GetCharacterInfo(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_GetCharacterInfo& req = SafeCast<Cl::CQ_GetCharacterInfo>(packetData, packetSize);
	NT_LOG("[client%03d] Client :: CQ_GetCharacterInfo :: characterID=%d", clientID, (u32)req.characterID);

	ActorUID actorUID = replication.GetWorldActorUID(clientID, req.characterID);
	if(actorUID == ActorUID::INVALID) {
		WARN("Client sent an invalid actor (localActorID=%d)", req.characterID);
		return;
	}

	game->OnPlayerGetCharacterInfo(clientID, actorUID);
}

void ChannelPvP::HandlePacket_CN_GameUpdatePosition(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	ProfileFunction();

	Cl::CN_GameUpdatePosition update = SafeCast<Cl::CN_GameUpdatePosition>(packetData, packetSize);
	NT_LOG("[client%03d] Client :: CN_GameUpdatePosition :: {", clientID);
	NT_LOG("	characterID=%d", (u32)update.characterID);
	NT_LOG("	p3nPos=(%g, %g, %g)", update.p3nPos.x, update.p3nPos.y, update.p3nPos.z);
	NT_LOG("	p3nDir=(%g, %g)", update.p3nDir.x, update.p3nDir.y);
	NT_LOG("	rot=(upperYaw=%g, upperPitch=%g, bodyYaw=%g)", update.upperYaw, update.upperPitch, update.bodyYaw);
	NT_LOG("	nSpeed=%g", update.nSpeed);
	NT_LOG("	unk1=%u", update.unk1);
	NT_LOG("	actionState=%s (%d)", ActionStateString(update.actionState), update.actionState);
	NT_LOG("	localTimeS=%g", update.localTimeS);
	NT_LOG("	unk2=%u", update.unk2);
	NT_LOG("}");

	ActorUID actorUID = replication.GetWorldActorUID(clientID, update.characterID);
	if(actorUID == ActorUID::INVALID) {
		WARN("Client sent an invalid actor (localActorID=%d)", update.characterID);
		return;
	}

	const f64 serverTime = TimeDiffSec(TimeRelNow());
	f64 clientDelta = (f64)update.localTimeS - clientTime[clientID].posClient;
	f64 serverDelta = serverTime - clientTime[clientID].posServer;

	clientTime[clientID].posClient = update.localTimeS;
	clientTime[clientID].posServer = serverTime;

	LOG("clientDelta=%g serverDelta=%g", clientDelta, serverDelta);

	// transform rotation for our coordinate system
	RotationHumanoid rot = RotConvertToWorld({ update.upperYaw, update.upperPitch, update.bodyYaw });

	game->OnPlayerUpdatePosition(clientID, actorUID, f2v(update.p3nPos), f2v(update.p3nDir), rot, update.nSpeed, ActionStateID::INVALID, 0, update.localTimeS);
}

void ChannelPvP::HandlePacket_CN_GameUpdateRotation(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	ProfileFunction();

	Cl::CN_GameUpdateRotation update = SafeCast<Cl::CN_GameUpdateRotation>(packetData, packetSize);
	NT_LOG("[client%03d] Client :: CN_GameUpdateRotation :: { characterID=%u upperYaw=%f upperPitch=%f bodyYaw=%f }", clientID, (u32)update.characterID, update.upperYaw, update.upperPitch, update.bodyYaw);

	ActorUID actorUID = replication.GetWorldActorUID(clientID, update.characterID);
	if(actorUID == ActorUID::INVALID) {
		WARN("Client sent an invalid actor (localActorID=%u)", update.characterID);
		return;
	}

	// transform rotation for our coordinate system
	RotationHumanoid rot;
	rot.upperYaw = MxmYawToWorldYaw(update.upperYaw);
	rot.upperPitch = MxmPitchToWorldPitch(update.upperPitch);
	rot.bodyYaw = MxmYawToWorldYaw(update.bodyYaw);
	game->OnPlayerUpdateRotation(clientID, actorUID, rot);
}

void ChannelPvP::HandlePacket_CN_ChannelChatMessage(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	ConstBuffer buff(packetData, packetSize);
	i32 chatType = buff.Read<i32>();
	const u16 msgLen = buff.Read<u16>();
	const wchar* msg = (wchar*)buff.ReadRaw(msgLen * 2);

	NT_LOG("[client%03d] Client :: CN_ChannelChatMessage :: chatType=%d msg='%.*S'", clientID, chatType, msgLen, msg);

	game->OnPlayerChatMessage(clientID, chatType, msg, msgLen);
}

void ChannelPvP::HandlePacket_CQ_SetLeaderCharacter(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_SetLeaderCharacter& leader = SafeCast<Cl::CQ_SetLeaderCharacter>(packetData, packetSize);
	NT_LOG("[client%03d] Client :: CQ_SetLeaderCharacter :: characterID=%d skinIndex=%d", clientID, (u32)leader.characterID, (i32)leader.skinIndex);

	game->OnPlayerSetLeaderCharacter(clientID, leader.characterID, leader.skinIndex);
}

void ChannelPvP::HandlePacket_CN_GamePlayerSyncActionStateOnly(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	Cl::CN_GamePlayerSyncActionStateOnly sync = SafeCast<Cl::CN_GamePlayerSyncActionStateOnly>(packetData, packetSize);

	i32 state = (i32)sync.state;
	const char* stateStr = g_ActionStateInvalidString;
	if(state >= 0 && state < ARRAY_COUNT(g_ActionStateString)) {
		stateStr = g_ActionStateString[state];
	}

	NT_LOG("[client%03d] Client :: CN_GamePlayerSyncActionStateOnly :: {", clientID);
	NT_LOG("	characterID=%d", (u32)sync.characterID);
	NT_LOG("	nState=%d (%s)", (i32)sync.state, stateStr);
	NT_LOG("	bApply=%d", sync.bApply);
	NT_LOG("	param1=%d", sync.param1);
	NT_LOG("	param2=%d", sync.param2);
	NT_LOG("	i4=%d", sync.i4);
	NT_LOG("	rotate=%g", sync.rotate);
	NT_LOG("	upperRotate=%g", sync.upperRotate);
	NT_LOG("}");

	ActorUID actorUID = replication.GetWorldActorUID(clientID, sync.characterID);
	if(actorUID == ActorUID::INVALID) {
		WARN("Client sent an invalid actor (localActorID=%d)", sync.characterID);
		return;
	}

	sync.rotate = MxmYawToWorldYaw(sync.rotate);
	sync.upperRotate = MxmYawToWorldYaw(sync.upperRotate);
	game->OnPlayerSyncActionState(clientID, actorUID, sync.state, sync.param1, sync.param2, sync.rotate, sync.upperRotate);
}

void ChannelPvP::HandlePacket_CQ_WhisperSend(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
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

void ChannelPvP::HandlePacket_CQ_RTT_Time(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_RTT_Time& rtt = SafeCast<Cl::CQ_RTT_Time>(packetData, packetSize);
	NT_LOG("[client%03d] Client :: CQ_RTT_Time :: { time=%u }", clientID, rtt.time);

	const i64 serverTime = (i64)TimeDiffMs(TimeRelNow());
	i64 clientDelta = (i64)rtt.time - (i64)clientTime[clientID].rttClient;
	i64 serverDelta = serverTime - clientTime[clientID].rttServer;

	clientTime[clientID].rttClient = rtt.time;
	clientTime[clientID].rttServer = serverTime;

	LOG("clientDelta=%lld serverDelta=%lld", clientDelta, serverDelta);

	Sv::SA_RTT_Time answer;
	answer.clientTimestamp = rtt.time;
	answer.serverTimestamp = serverTime;
	LOG("[client%03d] Server :: %s", clientID, PacketSerialize<Sv::SA_RTT_Time>(&answer, sizeof(answer)));
	server->SendPacket(clientID, answer);
}

void ChannelPvP::HandlePacket_CQ_LoadingProgressData(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_LoadingProgressData& loading = SafeCast<Cl::CQ_LoadingProgressData>(packetData, packetSize);
	NT_LOG("[client%03d] Client :: CQ_LoadingProgressData :: { progress=%u }", clientID, loading.progress);
}

void ChannelPvP::HandlePacket_CQ_LoadingComplete(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%03d] Client :: CQ_LoadingComplete", clientID);
	game->OnPlayerLoadingComplete(clientID);
	replication.SetPlayerLoaded(clientID);
}

void ChannelPvP::HandlePacket_CQ_GameIsReady(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%03d] Client :: CQ_GameIsReady", clientID);
	game->OnPlayerGameIsReady(clientID);
}

void ChannelPvP::HandlePacket_CQ_GamePlayerTag(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_GamePlayerTag& tag = SafeCast<Cl::CQ_GamePlayerTag>(packetData, packetSize);

	NT_LOG("[client%03d] Client :: CQ_GamePlayerTag :: localActorID=%d", clientID, tag.characterID);
	game->OnPlayerTag(clientID, tag.characterID);
}

void ChannelPvP::HandlePacket_CQ_PlayerJump(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	ConstBuffer buff(packetData, packetSize);
	const u8 excludedFieldBits = buff.Read<u8>();
	const i32 actionID = buff.Read<i32>();
	const LocalActorID actorID = buff.Read<LocalActorID>();
	const f32 rotate = buff.Read<f32>();
	const f32 moveDirX = buff.Read<f32>();
	const f32 moveDirY = buff.Read<f32>();

	NT_LOG("[client%03d] Client :: CQ_PlayerJump :: localActorID", clientID, actorID);
	game->OnPlayerJump(clientID, actorID, rotate, moveDirX, moveDirY);
}

void ChannelPvP::HandlePacket_CQ_PlayerCastSkill(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%03d] Client :: %s", clientID, PacketSerialize<Cl::CQ_PlayerCastSkill>(packetData, packetSize));

	PlayerCastSkill cast;
	ReadPacket(&cast, clientID, packetData, packetSize);

	game->OnPlayerCastSkill(clientID, cast);
}

void ChannelPvP::ReadPacket(PlayerCastSkill* cast, i32 clientID, const u8* packetData, const i32 packetSize)
{
	ConstBuffer buff(packetData, packetSize);

	cast->playerActorUID = replication.GetWorldActorUID(clientID, buff.Read<LocalActorID>());
	cast->skillID = buff.Read<SkillID>();
	cast->p3nPos = buff.Read<vec3>();

	const u16 count = buff.Read<u16>();
	for(int i = 0; i < count; i++) {
		cast->targetList.push_back(replication.GetWorldActorUID(clientID, buff.Read<LocalActorID>()));
	}

	cast->posStruct.pos = buff.Read<float3>();
	cast->posStruct.destPos = buff.Read<float3>();
	cast->posStruct.moveDir = buff.Read<float2>();
	cast->posStruct.rotateStruct= buff.Read<float3>();
	cast->posStruct.speed = buff.Read<f32>();
	cast->posStruct.clientTime = buff.Read<i32>();
}
