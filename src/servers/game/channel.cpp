#include "channel.h"
#include <common/packet_serialize.h>
#include <common/packet_validator.h>


#include "coordinator.h"
#include <mxm/game_content.h>
#include "game.h"

bool GamePacketHandler::Init(Game* game_)
{
	game = game_;
	replication = &game->replication;
	server = replication->server;

	clientTime.fill({});
	return true;
}

void GamePacketHandler::Cleanup()
{
	LOG("Channel cleanup...");
}

void GamePacketHandler::OnClientsConnected(const eastl::pair<ClientHandle,AccountUID>* clientList, const i32 count)
{
	for(int i = 0; i < count; i++) {
		auto& it = clientList[i];
		plidMap.Push(it.first);
	}
}

void GamePacketHandler::OnClientsDisconnected(const ClientHandle* clientList, const i32 count)
{
	for(int i = 0; i < count; i++) {
		const ClientHandle clientHd = clientList[i];
		game->OnPlayerDisconnect(clientHd);
		plidMap.Pop(clientHd);
	}
}

void GamePacketHandler::OnNewPacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData)
{
	const i32 packetSize = header.size - sizeof(NetHeader);

#define CASE(PACKET) case Cl::PACKET::NET_ID: { HandlePacket_##PACKET(clientHd, header, packetData, packetSize); } break

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
			NT_LOG("[client%x] Client :: Unknown packet :: size=%d netID=%d", clientHd, header.size, header.netID);
		} break;
	}

#undef CASE
}

void GamePacketHandler::HandlePacket_CN_ReadyToLoadGameMap(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CN_ReadyToLoadGameMap>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CN_ReadyToLoadGameMap (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CN_ReadyToLoadGameMap>(packetData, packetSize));
	game->OnPlayerReadyToLoad(clientHd);
}

void GamePacketHandler::HandlePacket_CA_SetGameGvt(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CA_SetGameGvt>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CA_SetGameGvt (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CA_SetGameGvt>(packetData, packetSize));
	const Cl::CA_SetGameGvt& gvt = SafeCast<Cl::CA_SetGameGvt>(packetData, packetSize);
	(void)gvt;
}

void GamePacketHandler::HandlePacket_CN_GameMapLoaded(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CN_GameMapLoaded>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CN_GameMapLoaded (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CN_GameMapLoaded>(packetData, packetSize));
	game->OnPlayerGameMapLoaded(clientHd);
	replication->SetPlayerAsInGame(clientHd);
}

void GamePacketHandler::HandlePacket_CQ_GetCharacterInfo(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_GetCharacterInfo>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_GetCharacterInfo (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_GetCharacterInfo>(packetData, packetSize));
	const Cl::CQ_GetCharacterInfo& req = SafeCast<Cl::CQ_GetCharacterInfo>(packetData, packetSize);

	ActorUID actorUID = replication->GetWorldActorUID(clientHd, req.characterID);
	if(actorUID == ActorUID::INVALID) {
		WARN("Client sent an invalid actor (localActorID=%d)", req.characterID);
		return;
	}

	game->OnPlayerGetCharacterInfo(clientHd, actorUID);
}

void GamePacketHandler::HandlePacket_CN_GameUpdatePosition(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	ProfileFunction();

	if(!ValidatePacket<Cl::CN_GameUpdatePosition>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CN_GameUpdatePosition (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CN_GameUpdatePosition>(packetData, packetSize));
	Cl::CN_GameUpdatePosition update = SafeCast<Cl::CN_GameUpdatePosition>(packetData, packetSize);

	ActorUID actorUID = replication->GetWorldActorUID(clientHd, update.characterID);
	if(actorUID == ActorUID::INVALID) {
		WARN("Client sent an invalid actor (localActorID=%d)", update.characterID);
		return;
	}

	RotationHumanoid rot = RotConvertToWorld({ update.upperYaw, update.upperPitch, update.bodyYaw });

	game->OnPlayerUpdatePosition(clientHd, actorUID, f2v(update.p3nPos), f2v(update.p3nDir), rot, update.nSpeed, ActionStateID::INVALID, update.localTimeS);
}

void GamePacketHandler::HandlePacket_CN_GameUpdateRotation(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	ProfileFunction();

	if(!ValidatePacket<Cl::CN_GameUpdateRotation>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CN_GameUpdateRotation (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CN_GameUpdateRotation>(packetData, packetSize));
	Cl::CN_GameUpdateRotation update = SafeCast<Cl::CN_GameUpdateRotation>(packetData, packetSize);

	ActorUID actorUID = replication->GetWorldActorUID(clientHd, update.characterID);
	if(actorUID == ActorUID::INVALID) {
		WARN("Client sent an invalid actor (localActorID=%u)", update.characterID);
		return;
	}

	RotationHumanoid rot;
	rot.upperYaw = MxmYawToWorldYaw(update.upperYaw);
	rot.upperPitch = MxmPitchToWorldPitch(update.upperPitch);
	rot.bodyYaw = MxmYawToWorldYaw(update.bodyYaw);
	game->OnPlayerUpdateRotation(clientHd, actorUID, rot);
}

void GamePacketHandler::HandlePacket_CN_ChannelChatMessage(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CN_ChannelChatMessage>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CN_ChannelChatMessage (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CN_ChannelChatMessage>(packetData, packetSize));
	ConstBuffer buff(packetData, packetSize);
	i32 chatType = buff.Read<i32>();
	const u16 msgLen = buff.Read<u16>();
	const wchar* msg = (wchar*)buff.ReadRaw(msgLen * 2);

	game->OnPlayerChatMessage(clientHd, chatType, msg, msgLen);
}

void GamePacketHandler::HandlePacket_CQ_SetLeaderCharacter(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_SetLeaderCharacter>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_SetLeaderCharacter (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_SetLeaderCharacter>(packetData, packetSize));
	const Cl::CQ_SetLeaderCharacter& leader = SafeCast<Cl::CQ_SetLeaderCharacter>(packetData, packetSize);

	game->OnPlayerSetLeaderCharacter(clientHd, leader.characterID, leader.skinIndex);
}

void GamePacketHandler::HandlePacket_CN_GamePlayerSyncActionStateOnly(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CN_GamePlayerSyncActionStateOnly>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CN_GamePlayerSyncActionStateOnly (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CN_GamePlayerSyncActionStateOnly>(packetData, packetSize));
	Cl::CN_GamePlayerSyncActionStateOnly sync = SafeCast<Cl::CN_GamePlayerSyncActionStateOnly>(packetData, packetSize);

	ActorUID actorUID = replication->GetWorldActorUID(clientHd, sync.characterID);
	if(actorUID == ActorUID::INVALID) {
		WARN("Client sent an invalid actor (localActorID=%d)", sync.characterID);
		return;
	}

	sync.rotate = MxmYawToWorldYaw(sync.rotate);
	sync.upperRotate = MxmYawToWorldYaw(sync.upperRotate);
	game->OnPlayerSyncActionState(clientHd, actorUID, sync.state, sync.param1, sync.param2, sync.rotate, sync.upperRotate);
}

void GamePacketHandler::HandlePacket_CQ_WhisperSend(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_WhisperSend>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_WhisperSend (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_WhisperSend>(packetData, packetSize));
	ConstBuffer buff(packetData, packetSize);
	WideString destNick;
	eastl::fixed_string<wchar,256,true> msg;

	const u16 destNick_len = buff.Read<u16>();
	const wchar* destNick_str = (wchar*)buff.ReadRaw(destNick_len * sizeof(wchar));
	const u16 msg_len = buff.Read<u16>();
	const wchar* msg_str = (wchar*)buff.ReadRaw(msg_len * sizeof(wchar));

	destNick.assign(destNick_str, destNick_len);
	msg.assign(msg_str, msg_len);

	game->OnPlayerChatWhisper(clientHd, destNick.data(), msg.data());
}

void GamePacketHandler::HandlePacket_CQ_RTT_Time(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_RTT_Time>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_RTT_Time (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_RTT_Time>(packetData, packetSize));
	const Cl::CQ_RTT_Time& rtt = SafeCast<Cl::CQ_RTT_Time>(packetData, packetSize);

	Sv::SA_RTT_Time answer;
	answer.clientTimestamp = rtt.time;
	answer.serverTimestamp = (i64)TimeDiffMs(TimeRelNow());
	SendPacket(clientHd, answer);
}

void GamePacketHandler::HandlePacket_CQ_LoadingProgressData(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_LoadingProgressData>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_LoadingProgressData (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_LoadingProgressData>(packetData, packetSize));
}

void GamePacketHandler::HandlePacket_CQ_LoadingComplete(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_LoadingComplete>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_LoadingComplete (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_LoadingComplete>(packetData, packetSize));
	game->OnPlayerLoadingComplete(clientHd);
	replication->SetPlayerLoaded(clientHd);
}

void GamePacketHandler::HandlePacket_CQ_GameIsReady(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_GameIsReady>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_GameIsReady (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_GameIsReady>(packetData, packetSize));
	game->OnPlayerGameIsReady(clientHd);
}

void GamePacketHandler::HandlePacket_CQ_GamePlayerTag(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_GamePlayerTag>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_GamePlayerTag (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_GamePlayerTag>(packetData, packetSize));
	const Cl::CQ_GamePlayerTag& tag = SafeCast<Cl::CQ_GamePlayerTag>(packetData, packetSize);
	game->OnPlayerTag(clientHd, replication->GetWorldActorUID(clientHd, tag.characterID));
}

void GamePacketHandler::HandlePacket_CQ_PlayerJump(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_PlayerJump>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_PlayerJump (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_PlayerJump>(packetData, packetSize));
	ConstBuffer buff(packetData, packetSize);
	const u8 excludedFieldBits = buff.Read<u8>();
	const i32 actionID = buff.Read<i32>();
	(void)excludedFieldBits;
	(void)actionID;
	const LocalActorID actorID = buff.Read<LocalActorID>();
	const f32 rotate = buff.Read<f32>();
	const f32 moveDirX = buff.Read<f32>();
	const f32 moveDirY = buff.Read<f32>();

	game->OnPlayerJump(clientHd, replication->GetWorldActorUID(clientHd, actorID), rotate, moveDirX, moveDirY);
}

void GamePacketHandler::HandlePacket_CQ_PlayerCastSkill(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_PlayerCastSkill>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_PlayerCastSkill (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_PlayerCastSkill>(packetData, packetSize));

	PlayerInputCastSkill cast;
	Cl::CQ_PlayerCastSkill::PosStruct posInfo;

	ConstBuffer buff(packetData, packetSize);
	const ActorUID actorUID = replication->GetWorldActorUID(clientHd, buff.Read<LocalActorID>());

	cast.skillID = buff.Read<SkillID>();
	cast.pos = f2v(buff.Read<float3>());

	const u16 count = buff.Read<u16>();
	for(int i = 0; i < count; i++) {
		cast.targetList.push_back(replication->GetWorldActorUID(clientHd, buff.Read<LocalActorID>()));
	}

	posInfo = buff.Read<Cl::CQ_PlayerCastSkill::PosStruct>();

	RotationHumanoid rot = RotConvertToWorld({ posInfo.rot.x, posInfo.rot.y, posInfo.rot.z });
	posInfo.rot = { rot.upperYaw, rot.upperPitch, rot.bodyYaw };

	game->OnPlayerCastSkill(clientHd, actorUID, cast, posInfo);
}
