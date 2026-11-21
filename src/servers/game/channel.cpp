#include "channel.h"
#include <common/packet_serialize.h>
#include <common/packet_validator.h>


#include "coordinator.h"
#include <mxm/game_content.h>
#include "game.h"

static const f32 SHOT_AIM_SCALE = 10.0f;

bool GamePacketHandler::Init(Game* game_)
{
	game = game_;
	replication = &game->replication;
	server = replication->server;

	clientTime.fill({});
	return true;
}

static const i32 REMOTE_CREATE_DOC_AFTER_NAME = 10;

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
		if(plidMap.TryGet(clientHd) == -1) continue;
		game->OnPlayerDisconnect(clientHd);
		plidMap.Pop(clientHd);
	}
}

static const u16 TRAIT_RESET_NET_ID = 60299;

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
		CASE(CQ_PlayerSyncTeleport);
		case Cl::CQ_MasterSupplyRequest::NET_ID: {
			NT_LOG("[client%x] Client :: trait upgrade size=%d", clientHd, packetSize);
			ConstBuffer buff(packetData, packetSize);
			if(!buff.CanRead(6)) break;
			const LocalActorID local = buff.Read<LocalActorID>();
			const u16 count = buff.Read<u16>();
			if(count > 32 || !buff.CanRead(count)) break;
			const u8* indices = (const u8*)buff.ReadRaw(count);
			game->OnTraitUpgrade(clientHd, replication->GetWorldActorUID(clientHd, local), indices, count);
		} break;
		case Cl::CQ_RequestVoting::NET_ID: {
			NT_LOG("[client%x] Client :: request voting size=%d", clientHd, packetSize);
			if(packetSize < 4) break;
			game->OnRequestVote(clientHd, *(const u32*)packetData);
		} break;
		case Cl::CQ_Vote::NET_ID: {
			NT_LOG("[client%x] Client :: vote size=%d", clientHd, packetSize);
			if(packetSize < 8) break;
			game->OnVote(clientHd, *(const u32*)(packetData + 4));
		} break;
		case TRAIT_RESET_NET_ID: {
			NT_LOG("[client%x] Client :: trait reset size=%d", clientHd, packetSize);
			ConstBuffer buff(packetData, packetSize);
			if(!buff.CanRead(4)) break;
			const LocalActorID local = buff.Read<LocalActorID>();
			game->OnTraitReset(clientHd, replication->GetWorldActorUID(clientHd, local));
		} break;
		case Cl::CA_PrepareHandoverToChannel::NET_ID: {
			NT_LOG("[client%x] Client :: CA_PrepareHandoverToChannel", clientHd);
			game->OnPlayerReturnToChannel(clientHd);
		} break;
		case Cl::CN_RewardStageReady::NET_ID: {
			NT_LOG("[client%x] Client :: CN_RewardStageReady", clientHd);
			game->OnRewardStageReady(clientHd);
		} break;
		case Cl::CQ_RewardSelectSlot::NET_ID: {
			if(packetSize < 4) break;
			NT_LOG("[client%x] Client :: CQ_RewardSelectSlot :: slot=%u", clientHd, *(const u32*)packetData);
			game->OnRewardSelectSlot(clientHd, *(const u32*)packetData);
		} break;
		case Cl::CQ_RewardSelectSlotAllRandom::NET_ID: {
			NT_LOG("[client%x] Client :: CQ_RewardSelectSlotAllRandom", clientHd);
			game->AutoPickRewards(clientHd);
		} break;
		case Cl::CQ_RewardRequestFinish::NET_ID: {
			NT_LOG("[client%x] Client :: CQ_RewardRequestFinish", clientHd);
			game->OnRewardRequestFinish(clientHd);
		} break;
		case Cl::CN_UNKNOWN_60260::NET_ID: {
			NT_LOG("[client%x] Client :: titan incarnate size=%d", clientHd, packetSize);
			if(packetSize < 8) break;
			game->OnTitanIncarnate(clientHd, *(const i32*)(packetData + 4));
		} break;
		case Cl::CQ_RoomEquipSkill::NET_ID: {
			if(packetSize < 12) break;
			ConstBuffer buff(packetData, packetSize);
			const LocalActorID chara = buff.Read<LocalActorID>();
			const SkillID skill = buff.Read<SkillID>();
			const u32 slot = buff.Read<u32>();
			NT_LOG("[client%x] Client :: equip skill in game :: chara=%u skill=%d slot=%u", clientHd, (u32)chara, (i32)skill, slot);
			game->OnTrainingEquipSkill(clientHd, chara, skill, slot);
		} break;
		case Cl::CN_UNKNOWN_60216::NET_ID:
		case Cl::CN_UNKNOWN_60291::NET_ID: {
			NT_LOG("[client%x] Client :: reward buy chance :: size=%d", clientHd, packetSize);
			game->OnRewardBuyChance(clientHd);
		} break;
		case Cl::CN_UNKNOWN_60233::NET_ID: {
			if(packetSize != 24) break;
			ConstBuffer buff(packetData, packetSize);
			const LocalActorID chara = buff.Read<LocalActorID>();
			const float3 pos = buff.Read<float3>();
			const LocalActorID target = buff.Read<LocalActorID>();
			const u32 ping = buff.Read<u32>();
			NT_LOG("[client%x] Client :: game ping :: chara=%u target=%u type=%u", clientHd, (u32)chara, (u32)target, ping);
			game->OnGamePing(clientHd, chara, pos, target, ping);
		} break;
		case Cl::CN_UNKNOWN_60296::NET_ID: {
			char hex[128] = {0};
			for(i32 i = 0; i < packetSize && i < 60; i++) snprintf(hex + i * 2, 3, "%02x", packetData[i]);
			NT_LOG("[client%x] Client :: master training event size=%d %s", clientHd, packetSize, hex);
			if(packetSize < 1) break;
			game->OnMasterTrainingEvent(clientHd, packetData, packetSize);
		} break;
		case Cl::CN_Ping::NET_ID: {
			if(packetSize != sizeof(Cl::CN_Ping)) break;
			LOG("[client%x] Network :: client RTT %u ms", clientHd, ((const Cl::CN_Ping*)packetData)->rttMs);
		} break;
		case Cl::CN_CommunityChat::NET_ID: {
			NT_LOG("[client%x] Client :: CN_CommunityChat size=%d (no community server)", clientHd, packetSize);
		} break;
		case Cl::CA_UNKNOWN_60212::NET_ID: {
			NT_LOG("[client%x] Client :: CA_UNKNOWN_60212 size=%d", clientHd, packetSize);
		} break;
		CASE(CQ_SkillUpgradeIngame);
		CASE(CQ_GetGroundItem);
		CASE(CQ_PlayerCastSkill);
		CASE(CQ_CancelCastSkill);
		CASE(CQ_PlayerCancelStance);
		CASE(CQ_WeaponState);
		CASE(CQ_InteractionCasting);
		CASE(CQ_InteractionCancel);
		CASE(CQ_GamePlayerSyncPhy);
		CASE(CQ_ReturnToCity);
		CASE(CQ_HandoverToChannel);
		CASE(CQ_PreCastSkill);
		CASE(CQ_ExecuteSkill);
		CASE(CQ_SwitchOnToggleSkill);
		CASE(CQ_SwitchOffToggleSkill);
		CASE(CQ_RemoteSyncCreateFromRemoteDoc);
		CASE(CQ_RemoteSyncTarget);

		default: {
			char hex[97] = {0};
			for(i32 i = 0; i < packetSize && i < 48; i++) snprintf(hex + i * 2, 3, "%02x", packetData[i]);
			NT_LOG("[client%x] Client :: Unknown packet :: size=%d netID=%d %s", clientHd, header.size, header.netID, hex);
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

	game->OnPlayerUpdatePosition(clientHd, actorUID, f2v(update.p3nPos), f2v(update.p3nDir), rot, update.nSpeed, update.actionState, update.localTimeS);
	replication->RelayPlayerMove(clientHd, actorUID, update);
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
	replication->RelayPlayerTurn(clientHd, actorUID, update);
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

void GamePacketHandler::HandlePacket_CQ_SkillUpgradeIngame(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_SkillUpgradeIngame>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_SkillUpgradeIngame (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_SkillUpgradeIngame>(packetData, packetSize));
	const Cl::CQ_SkillUpgradeIngame& upgrade = SafeCast<Cl::CQ_SkillUpgradeIngame>(packetData, packetSize);
	game->OnPlayerSkillUpgrade(clientHd, (i32)upgrade.slot);
}

void GamePacketHandler::HandlePacket_CQ_GetGroundItem(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_GetGroundItem>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_GetGroundItem (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_GetGroundItem>(packetData, packetSize));
	const Cl::CQ_GetGroundItem& request = SafeCast<Cl::CQ_GetGroundItem>(packetData, packetSize);
	game->OnPlayerTakeGroundItem(clientHd, request.targetEntityId);
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

void GamePacketHandler::HandlePacket_CQ_PlayerSyncTeleport(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_PlayerSyncTeleport>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_PlayerSyncTeleport (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_PlayerSyncTeleport>(packetData, packetSize));
	const Cl::CQ_PlayerSyncTeleport& packet = SafeCast<Cl::CQ_PlayerSyncTeleport>(packetData, packetSize);
	game->OnPlayerSyncTeleport(clientHd, replication->GetWorldActorUID(clientHd, (LocalActorID)packet.objectID), vec3(packet.pos.x, packet.pos.y, packet.pos.z));
}

void GamePacketHandler::HandlePacket_CQ_WeaponState(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_WeaponState>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_WeaponState (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_WeaponState>(packetData, packetSize));

	const Cl::CQ_WeaponState& weapon = SafeCast<Cl::CQ_WeaponState>(packetData, packetSize);
	game->OnPlayerWeaponStateRelay(clientHd, replication->GetWorldActorUID(clientHd, (LocalActorID)weapon.ownerID), (i32)weapon.state, weapon.chargeLevel, weapon.firingCombo);
	if((i32)weapon.state == WEAPON_STATE_FIRE) {
		game->OnPlayerWeaponFire(clientHd, MxmYawToWorldYaw(weapon.aimYaw));
	}
}

void GamePacketHandler::HandlePacket_CQ_InteractionCasting(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_InteractionCasting>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_InteractionCasting (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_InteractionCasting>(packetData, packetSize));
	ConstBuffer buff(packetData, packetSize);
	const LocalActorID self = buff.Read<LocalActorID>();
	const LocalActorID target = buff.Read<LocalActorID>();
	game->OnPlayerInteraction(clientHd, replication->GetWorldActorUID(clientHd, target), self);
}

void GamePacketHandler::HandlePacket_CQ_InteractionCancel(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_InteractionCancel>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_InteractionCancel (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_InteractionCancel>(packetData, packetSize));
	game->OnPlayerInteractionCancel(clientHd);
}

void GamePacketHandler::HandlePacket_CQ_GamePlayerSyncPhy(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_GamePlayerSyncPhy>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_GamePlayerSyncPhy (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_GamePlayerSyncPhy>(packetData, packetSize));

	const Cl::CQ_GamePlayerSyncPhy& phy = *(const Cl::CQ_GamePlayerSyncPhy*)packetData;
	const vec3 origin = f2v(phy.FirePosition);
	if(!IsFinite(origin.x) || !IsFinite(origin.y) || !IsFinite(origin.z)) {
		WARN("[client%x] WARNING: CQ_GamePlayerSyncPhy with a non finite FirePosition", clientHd);
		return;
	}
	const vec3 aimOffset = vec3((f32)phy.aimOffsetX, (f32)phy.aimOffsetY, (f32)phy.aimOffsetZ) / SHOT_AIM_SCALE;
	const u16 nameLen = phy.charNameLen;
	const wchar* name = phy.charName;
	const bool isWeapon = FireObjectStartsWith(name, nameLen, FIRE_OBJECT_ATTACK_PREFIX) || game->IsWeaponFireObject(clientHd, name, nameLen);
	if(!isWeapon) {
		NT_LOG("[client%x] SyncPhy :: non-weapon fire object '%.*ls'", clientHd, nameLen, name);
		return;
	}

	const vec2 dir = vec2(aimOffset);
	const bool aimless = glm::length(dir) < 0.1f;
	if(aimless && !FireObjectIs(name, nameLen, FIRE_OBJECT_SCENE_ROOT)) return;

	game->OnPlayerWeaponShot(clientHd, origin, aimless ? vec2(0) : glm::normalize(dir), aimless ? 0.0f : glm::length(dir), packetData, packetSize);
}

void GamePacketHandler::HandlePacket_CQ_ReturnToCity(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%x] Client :: CQ_ReturnToCity", clientHd);
	game->OnPlayerReturnToChannel(clientHd);
}

void GamePacketHandler::HandlePacket_CQ_HandoverToChannel(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%x] Client :: CQ_HandoverToChannel", clientHd);
	game->OnPlayerReturnToChannel(clientHd);
}

void GamePacketHandler::HandlePacket_CQ_PreCastSkill(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_PreCastSkill>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_PreCastSkill (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_PreCastSkill>(packetData, packetSize));
}

void GamePacketHandler::HandlePacket_CQ_ExecuteSkill(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_ExecuteSkill>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_ExecuteSkill (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_ExecuteSkill>(packetData, packetSize));

	ConstBuffer buff(packetData, packetSize);
	buff.Read<u32>();
	const SkillID skillID = SkillID(buff.Read<u32>());
	const vec3 targetPos = f2v(buff.Read<float3>());
	const u16 targetCount = buff.Read<u16>();
	const LocalActorID target = targetCount > 0 ? buff.Read<LocalActorID>() : LocalActorID::INVALID;
	game->OnPlayerExecuteSkill(clientHd, skillID, targetPos, target);
}

void GamePacketHandler::HandlePacket_CQ_SwitchOnToggleSkill(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_SwitchOnToggleSkill>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_SwitchOnToggleSkill (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_SwitchOnToggleSkill>(packetData, packetSize));
	const Cl::CQ_SwitchOnToggleSkill& toggle = SafeCast<Cl::CQ_SwitchOnToggleSkill>(packetData, packetSize);
	game->OnPlayerToggleSkill(clientHd, SkillID(toggle.skillID), true);
}

void GamePacketHandler::HandlePacket_CQ_SwitchOffToggleSkill(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_SwitchOffToggleSkill>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_SwitchOffToggleSkill (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_SwitchOffToggleSkill>(packetData, packetSize));
	const Cl::CQ_SwitchOffToggleSkill& toggle = SafeCast<Cl::CQ_SwitchOffToggleSkill>(packetData, packetSize);
	game->OnPlayerToggleSkill(clientHd, SkillID(toggle.skillID), false);
}

void GamePacketHandler::HandlePacket_CQ_RemoteSyncCreateFromRemoteDoc(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_RemoteSyncCreateFromRemoteDoc>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_RemoteSyncCreateFromRemoteDoc (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_RemoteSyncCreateFromRemoteDoc>(packetData, packetSize));
	replication->RelayRemoteSyncCreateFromRemoteDoc(clientHd, packetData, packetSize);
	const Cl::CQ_RemoteSyncCreateFromRemoteDoc& create = SafeCast<Cl::CQ_RemoteSyncCreateFromRemoteDoc>(packetData, packetSize);
	const i32 docOffset = (i32)offsetof(Cl::CQ_RemoteSyncCreateFromRemoteDoc, FireObject) + create.FireObject_len * (i32)sizeof(wchar_t) + REMOTE_CREATE_DOC_AFTER_NAME;
	if(packetSize >= docOffset + (i32)sizeof(u32)) game->OnClientRemoteCreated(clientHd, create.RemoteSeedID, (RemoteIdx)*(const u32*)(packetData + docOffset));
}

void GamePacketHandler::HandlePacket_CQ_RemoteSyncTarget(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_RemoteSyncTarget>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_RemoteSyncTarget (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_RemoteSyncTarget>(packetData, packetSize));
	replication->RelayRemoteSyncTarget(clientHd, packetData, packetSize);
	const Cl::CQ_RemoteSyncTarget& sync = *(const Cl::CQ_RemoteSyncTarget*)packetData;
	game->OnRemoteSyncTarget(clientHd, sync.remoteID, f2v(sync.DestPos));
}

void GamePacketHandler::HandlePacket_CQ_CancelCastSkill(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_CancelCastSkill>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_CancelCastSkill (size=%d)", clientHd, packetSize);
		return;
	}
	const Cl::CQ_CancelCastSkill& packet = SafeCast<Cl::CQ_CancelCastSkill>(packetData, packetSize);
	game->OnPlayerCancelCast(clientHd, packet.skillID);
}

void GamePacketHandler::HandlePacket_CQ_PlayerCancelStance(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_PlayerCancelStance>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_PlayerCancelStance (size=%d)", clientHd, packetSize);
		return;
	}
	game->OnPlayerCancelStance(clientHd);
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
