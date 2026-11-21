#include "replication.h"
#include <mxm/item_writer.h>
#include <mxm/achievements.h>

#include <common/protocol.h>
#include <EASTL/algorithm.h>
#include <EASTL/fixed_hash_map.h>
#include <EASTL/sort.h>
#include <EAStdC/EAString.h>
#include <mxm/game_content.h>
#include "config.h"

static const u32 PACKET_DAMAGE_TYPE_STATUS = 1;

void Replication::Frame::Clear()
{
	playerList.clear();
	masterList.clear();
	npcList.clear();
	dynamicList.clear();

	playerMap.fill(playerList.end());
	masterMap.clear();
	npcMap.clear();
	dynamicMap.clear();

	actorUIDSet.clear();
	actorType.clear();

	skillCastList.clear();
	skillExecList.clear();
}

void Replication::PlayerLocalInfo::Reset()
{
	localActorIDMap.clear();
	departedActorIDMap.clear();
	actorUIDSet.clear();
	hiddenActors.clear();
	shownMain.clear();
	monsterSpeedSent.clear();
	nextPlayerLocalActorID = LocalActorID::FIRST_OTHER_PLAYER;
	nextNpcLocalActorID = LocalActorID::FIRST_NPC;
	nextMonsterLocalActorID = LocalActorID::INVALID;
}

void Replication::Init(Server* server_)
{
	server = server_;

	clientHandle.fill(ClientHandle::INVALID);
	playerState.fill({ PlayerState::DISCONNECTED, PlayerState::DISCONNECTED });
	foreach(p, playerLocalInfo) p->Reset();

	framePrev = &frames[0];
	frameCur = &frames[1];

	framePrev->Clear();
	frameCur->Clear();
}

void Replication::FrameEnd()
{
	ProfileFunction();

	UpdatePlayersLocalState();

	FrameDifference();

	for(int pi = 0; pi < MAX_PLAYERS; pi++) {
		PlayerStatePair& state = playerState[pi];

		const ClientHandle clienHd = clientHandle[pi];
		if(state.prev == PlayerState::DISCONNECTED && state.cur == PlayerState::CONNECTED) {
			SendAccountDataPvp(clienHd);
		}
		if(state.prev == PlayerState::CONNECTED && state.cur == PlayerState::IN_GAME) {
			SendInitialFrame(clienHd);
		}
		else if(state.prev == PlayerState::IN_GAME && state.cur == PlayerState::LOADED) {
			SendPvpLoadingComplete(clienHd);
		}

		state.prev = state.cur;
	}

	eastl::swap(frameCur, framePrev);
	frameCur->Clear(); // clear frame
}

void Replication::FramePushPlayer(const Player& player)
{
	ASSERT(frameCur->playerMap[player.index] == frameCur->playerList.end());

	frameCur->playerList.emplace_back(player);
	frameCur->playerMap[player.index] = --frameCur->playerList.end();

#ifdef CONF_DEBUG
	if(player.clientHd != ClientHandle::INVALID) {
		ASSERT(playerMap.at(player.clientHd) == player.index);
	}
#endif
}

void Replication::FramePushMasterActors(const Replication::ActorMaster* actorList, const i32 count)
{
	forarr(actor, actorList, count) {
		ASSERT(frameCur->masterMap.find(actor->actorUID) == frameCur->masterMap.end());
		ASSERT(frameCur->actorUIDSet.find(actor->actorUID) == frameCur->actorUIDSet.end());

		frameCur->masterList.emplace_back(*actor);
		frameCur->masterMap.emplace(actor->actorUID, --frameCur->masterList.end());

		frameCur->actorUIDSet.insert(actor->actorUID);
		frameCur->actorType.emplace(actor->actorUID, actor->Type());
	}
}

void Replication::FramePushNpcActor(const Replication::ActorNpc& actor)
{
	ASSERT(frameCur->npcMap.find(actor.actorUID) == frameCur->npcMap.end());
	ASSERT(frameCur->actorUIDSet.find(actor.actorUID) == frameCur->actorUIDSet.end());

	frameCur->npcList.emplace_back(actor);
	frameCur->npcMap.emplace(actor.actorUID, --frameCur->npcList.end());
	frameCur->actorUIDSet.insert(actor.actorUID);
	frameCur->actorType.emplace(actor.actorUID, actor.Type());
}

void Replication::FramePushDynamicActor(const ActorDynamic& actor)
{
	ASSERT(frameCur->dynamicMap.find(actor.actorUID) == frameCur->dynamicMap.end());
	ASSERT(frameCur->actorUIDSet.find(actor.actorUID) == frameCur->actorUIDSet.end());

	frameCur->dynamicList.emplace_back(actor);
	frameCur->dynamicMap.emplace(actor.actorUID, --frameCur->dynamicList.end());
	frameCur->actorUIDSet.insert(actor.actorUID);
	frameCur->actorType.emplace(actor.actorUID, actor.Type());
}

void Replication::FramePushSkillCast(const SkillCast& skillCast)
{
	frameCur->skillCastList.push_back(skillCast);
}

void Replication::FramePushSkillExec(const SkillExec& skillExec)
{
	frameCur->skillExecList.push_back(skillExec);
}

void Replication::OnPlayerConnect(ClientHandle clientHd, u32 playerIndex)
{
	playerMap[clientHd] = playerIndex;
	playerState[playerIndex].prev = PlayerState::DISCONNECTED;
	playerState[playerIndex].cur = PlayerState::CONNECTED;
	playerLocalInfo[playerIndex].Reset();
	clientHandle[playerIndex] = clientHd;
}

static const u8 MEASURED_REWARD_NEXT_ACTION = 1;
static const i32 MEASURED_PVE_REMAIN_LIFE = -1;
static const u32 MEASURED_DROP_TYPE_ITEM = 0;
static const u32 MEASURED_DROP_TYPE_RON = 1;
static const u8 MEASURED_PRIVATE_DROP_POSITION_TYPE = 2;
static const char MEASURED_DAMAGE_HIT_NODE[] = "__$body";

void Replication::SendStageClearResult(ClientHandle clientHd, const StageClear& clear, const StageMember* members, i32 memberCount)
{
	PacketWriter<Sv::SN_StageClearResult,1024> packet;
	packet.Write<LocalActorID>(clear.player);
	packet.Write<u16>((u16)memberCount);
	for(i32 i = 0; i < memberCount; i++) {
		packet.Write<u32>((u32)members[i].mainClass);
		packet.Write<u32>((u32)members[i].subClass);
		packet.WriteStringObj(members[i].name);
	}
	packet.Write<u8>(0);
	packet.Write<i32>(clear.stage);
	packet.Write<i32>(clear.kills);
	packet.Write<i32>(clear.deaths);
	packet.Write<u32>(clear.playTimeMs);
	packet.Write<u8>(clear.rank);
	packet.Write<u32>(0);
	packet.Write<u32>(clear.baseRon);
	packet.Write<u16>((u16)clear.bonusRon.size());
	foreach_const(b, clear.bonusRon) { packet.Write<u32>(b->type); packet.Write<u32>(b->amount); }
	packet.Write<u32>(clear.baseExp);
	packet.Write<u16>((u16)clear.bonusExp.size());
	foreach_const(b, clear.bonusExp) { packet.Write<u32>(b->type); packet.Write<u32>(b->amount); }
	packet.Write<u32>(clear.attackScore);
	packet.Write<u32>(clear.defenseScore);
	packet.Write<u32>(clear.timeScore);
	packet.Write<u32>(0);
	packet.Write<u32>(clear.totalScore);
	packet.Write<u32>(0);
	packet.Write<u8>(clear.technicGrade);
	packet.Write<u32>(0);
	packet.Write<u32>(0);
	packet.Write<u8>(1);
	packet.Write<u16>(0);
	packet.Write<u16>(0);
	packet.Write<u16>(0);
	SendPacket(clientHd, packet);
}

static const i32 MEASURED_PC_CAFE_ITEM_NONE = -1;

void Replication::SendDefenceClearResult(ClientHandle clientHd, const DefenceClear& clear, const StageMember* members, i32 memberCount)
{
	PacketWriter<Sv::SN_DefenceClearResult,1024> packet;
	packet.Write<LocalActorID>(clear.player);
	packet.Write<u16>((u16)memberCount);
	for(i32 i = 0; i < memberCount; i++) {
		packet.Write<u32>((u32)members[i].mainClass);
		packet.Write<u32>((u32)members[i].subClass);
		packet.WriteStringObj(members[i].name);
	}
	packet.Write<i32>(clear.stage);
	packet.Write<i32>(clear.clearStep);
	packet.Write<u32>(clear.clearMs);
	packet.Write<u32>(0);
	packet.Write<u32>(clear.baseRon);
	packet.Write<u16>((u16)clear.bonusRon.size());
	foreach_const(b, clear.bonusRon) { packet.Write<u32>(b->type); packet.Write<u32>(b->amount); }
	packet.Write<u32>(clear.baseExp);
	packet.Write<u16>((u16)clear.bonusExp.size());
	foreach_const(b, clear.bonusExp) { packet.Write<u32>(b->type); packet.Write<u32>(b->amount); }
	packet.Write<u16>((u16)clear.items.size());
	foreach_const(it, clear.items) { packet.Write<u32>(it->type); packet.Write<i32>(it->index); packet.Write<u32>(it->amount); }
	packet.Write<u32>(clear.guildBase);
	packet.Write<u32>(clear.guildBonus);
	packet.Write<u8>(clear.guildMembers);
	packet.Write<u16>(clear.boostGold);
	packet.Write<u16>(clear.boostExp);
	packet.Write<u16>(0);
	packet.Write<u32>(0);
	packet.Write<i32>(MEASURED_PC_CAFE_ITEM_NONE);
	packet.Write<u32>(0);
	packet.Write<u16>(0);
	SendPacket(clientHd, packet);
}

void Replication::SendDefenceBestRecord(ClientHandle clientHd, i32 personalStep, u32 personalMs, i32 worldStep, u32 worldMs)
{
	Sv::SA_DefenceBestRecord packet;
	packet.personalClearStepIndex = (u32)personalStep;
	packet.personalClearTime = personalMs;
	packet.worldClearStepIndex = (u32)worldStep;
	packet.worldClearTime = worldMs;
	SendPacket(clientHd, packet);
}

void Replication::SendTeamScoreToAll(u32 type, i32 value)
{
	PacketWriter<Sv::SN_ScoreUpdate> packet;
	packet.Write<u32>(0);
	packet.Write<u32>(0);
	packet.Write<u16>(1);
	packet.Write<u32>(type);
	packet.Write<u32>((u32)MAX(value, 0));
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		SendPacket(clientHandle[clientID], packet);
	}
}

void Replication::SendPhaseStartToAll(i32 step)
{
	Sv::SN_PhaseStart packet;
	packet.step = (u32)step;
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		SendPacket(clientHandle[clientID], packet);
	}
}

void Replication::SendPhaseEndToAll(i32 step, u32 reason, u32 clearMs)
{
	Sv::SN_PhaseEnd packet;
	packet.step = (u32)step;
	packet.endReason = reason;
	packet.clearTime = clearMs;
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		SendPacket(clientHandle[clientID], packet);
	}
}

void Replication::SendExpGain(ClientHandle clientHd, u32 exp)
{
	PacketWriter<Sv::SN_ExpGain,16> packet;
	packet.Write<u32>(exp);
	SendPacket(clientHd, packet);
}

void Replication::SendClearedStage(ClientHandle clientHd, i32 stage)
{
	PacketWriter<Sv::SN_LoadClearedStages,16> packet;
	packet.Write<u16>(1);
	packet.Write<i32>(stage);
	SendPacket(clientHd, packet);
}

void Replication::SendRewardItemList(ClientHandle clientHd, const RewardCard* cards, i32 count)
{
	static const u8 MEASURED_REWARD_VIP_SLOT_COUNT = 1;
	PacketWriter<Sv::SN_RewardItemList,1024> packet;
	packet.Write<u16>((u16)count);
	for(i32 i = 0; i < count; i++) {
		packet.Write<u32>(cards[i].type);
		packet.Write<i32>(cards[i].index);
		packet.Write<u32>(cards[i].amount);
	}
	packet.Write<u32>(0);
	packet.Write<u8>(MEASURED_REWARD_VIP_SLOT_COUNT);
	packet.Write<u8>(0);
	packet.Write<u8>(0);
	packet.Write<u16>(0);
	SendPacket(clientHd, packet);
}

void Replication::SendRewardStageStart(ClientHandle clientHd, u32 waitMs, u8 slotCount)
{
	PacketWriter<Sv::SN_RewardStageStartNp,16> packet;
	packet.Write<u32>(waitMs);
	packet.Write<u8>(MEASURED_REWARD_NEXT_ACTION);
	packet.Write<u8>(slotCount);
	packet.Write<u32>(0);
	SendPacket(clientHd, packet);
}

void Replication::SendRewardSelectSlot(ClientHandle clientHd, u32 result, u32 slot, const RewardCard& card)
{
	PacketWriter<Sv::SA_RewardSelectSlot,32> packet;
	packet.Write<u32>(result);
	packet.Write<u32>(slot);
	packet.Write<u32>(card.type);
	packet.Write<i32>(card.index);
	packet.Write<u32>(card.amount);
	packet.Write<u8>(0);
	SendPacket(clientHd, packet);
}

void Replication::SendRewardRequestFinish(ClientHandle clientHd)
{
	PacketWriter<Sv::SA_RewardRequestFinish,8> packet;
	packet.Write<u32>(0);
	SendPacket(clientHd, packet);
}

void Replication::SendRewardStageEnd(ClientHandle clientHd, const RewardCard* cards, i32 count)
{
	PacketWriter<Sv::SN_RewardStageEnd,1024> packet;
	packet.Write<u16>((u16)count);
	for(i32 i = 0; i < count; i++) {
		packet.Write<u32>(cards[i].type);
		packet.Write<i32>(cards[i].index);
		packet.Write<u32>(cards[i].amount);
	}
	packet.Write<u16>(0);
	packet.Write<u32>(0);
	SendPacket(clientHd, packet);
}

void Replication::SendPrepareHandover(ClientHandle clientHd)
{
	SendPacketData<Sv::SQ_PrepareHandoverToChannel>(clientHd, 0, nullptr);
}

void Replication::SendUserLifeInfo(ClientHandle clientHd, i32 players)
{
	Sv::SN_NotifyUserLifeInfo lifeInfo;
	lifeInfo.usn = players;
	lifeInfo.lifeCount = 0;
	lifeInfo.maxLifeCount = 0;
	lifeInfo.remainLifeCount = MEASURED_PVE_REMAIN_LIFE;
	SendPacket(clientHd, lifeInfo);
}

void Replication::SendStageRank(ClientHandle clientHd, u32 rank, u32 timeScoreRank)
{
	Sv::SN_StageRank stageRank;
	stageRank.rank = rank;
	stageRank.timeScoreRank = timeScoreRank;
	SendPacket(clientHd, stageRank);
}

void Replication::SendWarnPveGameEnd(ClientHandle clientHd, u32 remainMs)
{
	PacketWriter<Sv::SN_WarnPveGameend,8> packet;
	packet.Write<u32>(remainMs);
	SendPacket(clientHd, packet);
}

void Replication::SendGameAreaMove(ClientHandle clientHd, MapIndex map)
{
	Sv::SN_GameAreaMove packet;
	packet.dwAreaIDX = (u32)map;
	SendPacket(clientHd, packet);
}

void Replication::ResetClientForAreaMove(ClientHandle clientHd)
{
	const i32 clientID = playerMap.at(clientHd);
	playerState[clientID].cur = PlayerState::CONNECTED;
	playerState[clientID].prev = PlayerState::CONNECTED;
	playerLocalInfo[clientID].actorUIDSet.clear();
	playerLocalInfo[clientID].hiddenActors.clear();
	playerLocalInfo[clientID].monsterSpeedSent.clear();
}

void Replication::SendMasterTrainingEvent(ClientHandle clientHd, u8 eventType, u32 ret, u32 entityID, u32 param1, u32 param2)
{
	Sv::SA_MasterTrainingEvent packet;
	packet.eventType = eventType;
	packet.ret = ret;
	packet.entityID = entityID;
	packet.param1 = param1;
	packet.param2 = param2;
	SendPacket(clientHd, packet);
}

void Replication::SendLoadPvpMap(ClientHandle clientHd, MapIndex stageIndex)
{
	Sv::SN_UpdateGameOwner owner;
	owner.userId = 1;
	SendPacket(clientHd, owner);

	Sv::SN_LobbyStartGame lobby;
	lobby.stageType = StageType::PVP_GAME;
	SendPacket(clientHd, lobby);

	// SN_CityMapInfo
	Sv::SN_CityMapInfo cityMapInfo;
	cityMapInfo.CityMapID = stageIndex;
	SendPacket(clientHd, cityMapInfo);

	/*
	// SN_WeaponState
	{
		PacketWriter<> packet;

		packet.Write<LocalActorID>((LocalActorID)21035); // ownerID
		packet.Write<i32>(90); // weaponID
		packet.Write<i32>(0); // state
		packet.Write<u8>(0); // chargeLevel
		packet.Write<u8>(0); // firingCombo
		packet.Write<i32>(-1); // result

		SendPacketData<>(clientID, Sv::SN_WeaponState::NET_ID, packet.size, packet.data);
	}

	playerState[clientID].cur = PlayerState::IN_GAME;*/
}

void Replication::SetPlayerAsInGame(ClientHandle clientHd)
{
	const i32 clientID = playerMap.at(clientHd);
	playerState[clientID].cur = PlayerState::IN_GAME;
}

void Replication::SetPlayerLoaded(ClientHandle clientHd)
{
	const i32 clientID = playerMap.at(clientHd);
	playerState[clientID].cur = PlayerState::LOADED;
}

void Replication::SendCharacterInfo(ClientHandle clientHd, ActorUID actorUID, CreatureIndex docID, ClassType classType, i32 health, i32 healthMax)
{
	const i32 clientID = playerMap.at(clientHd);

	if(playerState[clientID].cur < PlayerState::IN_GAME) {
		LOG("WARNING(SendCharacterInfo): player not in game (clientID=%d, state=%d)", clientID, (i32)playerState[clientID].cur);
		return;
	}

	// SA_GetCharacterInfo
	Sv::SA_GetCharacterInfo info;
	info.characterID = GetLocalActorID(clientHd, actorUID);
	info.docIndex = docID;
	info.classType = classType;
	info.hp = health;
	info.maxHp = healthMax;
	SendPacket(clientHd, info);
}

void Replication::SendPlayerSetLeaderMaster(ClientHandle clientHd, ActorUID masterActorUID, ClassType classType, SkinIndex skinIndex)
{
	LocalActorID laiLeader = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)classType);
	ASSERT(laiLeader >= LocalActorID::FIRST_SELF_MASTER && laiLeader < LocalActorID::LAST_SELF_MASTER);

	const i32 clientID = playerMap.at(clientHd);
	PlayerForceLocalActorID(clientID, masterActorUID, laiLeader);

	if(playerState[clientID].cur < PlayerState::IN_GAME) {
		// SN_LeaderCharacter
		Sv::SN_LeaderCharacter leader;
		leader.leaderID = laiLeader;
		leader.skinIndex = skinIndex;
		SendPacket(clientHd, leader);
	}
	else {
		// NOTE: only seems to close the master window
		// SA_LeaderCharacter
		Sv::SA_SetLeader leader;
		leader.result = 0;
		leader.leaderID = laiLeader;
		leader.skinIndex = skinIndex;
		SendPacket(clientHd, leader);
	}
}

void Replication::SendChatMessageToAll(const wchar* senderName, i32 chatType, const wchar* msg, i32 msgLen)
{
	// TODO: restrict message length
	PacketWriter<Sv::SN_ChatChannelMessage> packet;

	packet.Write<i32>(chatType); // chatType
	packet.WriteStringObj(senderName);
	packet.Write<u8>(0); // senderStaffType
	packet.WriteStringObj(msg, msgLen);

	for(int clientID= 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;

		SendPacket(clientHandle[clientID], packet);
	}
}

void Replication::SendChatMessageToClient(ClientHandle toClientHd, const wchar* senderName, EChatType chatType, const wchar* msg, i32 msgLen)
{
	const i32 toClientID = playerMap.at(toClientHd);
	if(playerState[toClientID].cur < PlayerState::IN_GAME) return;

	if(msgLen == -1) msgLen = EA::StdC::Strlen(msg);

	PacketWriter<Sv::SN_ChatChannelMessage> packet;

	packet.Write<EChatType>(chatType); // chatType
	packet.WriteStringObj(senderName);
	packet.Write<u8>(0); // senderStaffType
	packet.WriteStringObj(msg, msgLen);

	SendPacket(toClientHd, packet);
}

void Replication::SendClientLevelEvent(ClientHandle clientHd, i32 eventID, ActorUID caller)
{
	Sv::SN_RunClientLevelEvent event;
	event.eventID = eventID;
	const LocalActorID callerLocal = caller != ActorUID::INVALID ? GetLocalActorID(clientHd, caller) : LocalActorID::INVALID;
	event.caller = callerLocal != LocalActorID::INVALID ? (u32)callerLocal : 0;
	event.serverTime = (i64)TimeDiffMs(TimeRelNow());
	SendPacket(clientHd, event);
}

void Replication::SendClientLevelEventToAll(i32 eventID, ActorUID caller)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		SendClientLevelEvent(clientHandle[clientID], eventID, caller);
	}
}

void Replication::SendCreateGroundItemToAll(u32 itemUID, i32 itemIndex, const vec3& pos, i32 gettableType, i32 factionType)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		PacketWriter<Sv::SN_CreateGroundItem,64> packet;
		packet.Write<u8>(0);
		packet.Write<float3>(v2f(pos));
		packet.Write<float3>(v2f(pos));
		packet.Write<u32>(itemUID);
		packet.Write<u32>(0);
		packet.Write<u32>((u32)itemIndex);
		packet.Write<u32>(1);
		packet.Write<u32>(0);
		packet.Write<i32>(gettableType);
		packet.Write<i32>(factionType);
		packet.Write<u8>(0);
		packet.Write<u8>(1);
		SendPacket(clientHandle[clientID], packet);
	}
}

void Replication::SendCreatePrivateGroundItem(ClientHandle clientHd, ActorUID ownerUID, u32 itemUID, bool ron, i32 itemIndex, i32 count, const vec3& pos)
{
	PacketWriter<Sv::SN_CreateGroundItem,64> packet;
	packet.Write<u8>(0);
	packet.Write<float3>(v2f(pos));
	packet.Write<float3>(v2f(pos));
	packet.Write<u32>(itemUID);
	packet.Write<u32>(ron ? MEASURED_DROP_TYPE_RON : MEASURED_DROP_TYPE_ITEM);
	packet.Write<i32>(ron ? -1 : itemIndex);
	packet.Write<i32>(count);
	packet.Write<LocalActorID>(GetLocalActorID(clientHd, ownerUID));
	packet.Write<i32>(0);
	packet.Write<i32>(-1);
	packet.Write<u8>(1);
	packet.Write<u8>(MEASURED_PRIVATE_DROP_POSITION_TYPE);
	SendPacket(clientHd, packet);
}

void Replication::SendMoney(ClientHandle clientHd, i64 gold, u32 reason)
{
	Sv::SN_Money money;
	money.nMoney = gold;
	money.nReason = reason;
	SendPacket(clientHd, money);
}

void Replication::SendItemMerge(ClientHandle clientHd, u32 itemUID, i32 count)
{
	Sv::SA_ItemMerge merge;
	merge.nEquipKey = 0;
	merge.nCount = 0;
	merge.nEquipKey2 = itemUID;
	merge.nCount2 = (u32)count;
	merge.nErrorType = 0;
	SendPacket(clientHd, merge);
}

void Replication::SendItemUpdate(ClientHandle clientHd, const AccountItem& item, u32 reason)
{
	PacketWriter<Sv::SN_ItemUpdate,64> packet;
	packet.Write<u16>(1);
	packet.Write<u32>(item.uid);
	packet.Write<u8>(item.invenType);
	packet.Write<i32>(item.slot);
	packet.Write<i32>(item.docIndex);
	packet.Write<i32>(item.count);
	packet.Write<i32>(-1);
	packet.Write<u8>(0);
	packet.Write<i64>(0);
	packet.Write<u16>(0);
	packet.Write<u32>(reason);
	SendPacket(clientHd, packet);
}

void Replication::SendGroundItemTaken(ClientHandle clientHd, ActorUID takerUID, u32 itemUID)
{
	SendGroundItemTaken(clientHd, takerUID, itemUID, 0);
}

void Replication::SendGroundItemTaken(ClientHandle clientHd, ActorUID takerUID, u32 itemUID, u32 itemISN)
{
	SendGroundItemAnswer(clientHd, takerUID, itemUID, itemISN);
	SendGroundItemAnswer(clientHd, takerUID, itemUID, itemISN);
}

void Replication::SendGroundItemAnswer(ClientHandle clientHd, ActorUID takerUID, u32 itemUID, u32 itemISN)
{
	Sv::SA_GetGroundItem answer;
	answer.playerID = (u32)GetLocalActorID(clientHd, takerUID);
	answer.itemID = itemUID;
	answer.itemISN = itemISN;
	answer.nErrorType = 0;
	SendPacket(clientHd, answer);
}

void Replication::SendPublicGroundItemTaken(ClientHandle takerHd, ActorUID takerUID, u32 itemUID)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID local = GetLocalActorID(clientHd, takerUID);
		if(local == LocalActorID::INVALID) continue;
		if(clientHd == takerHd) SendGroundItemAnswer(clientHd, takerUID, itemUID, 0);
		Sv::SN_GetPublicGroundItem notify;
		notify.playerID = (u32)local;
		notify.itemID = (GroundItemUID)itemUID;
		SendPacket(clientHd, notify);
		if(clientHd == takerHd) SendGroundItemAnswer(clientHd, takerUID, itemUID, 0);
	}
}

void Replication::SendDestroyGroundItemToAll(u32 itemUID)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		Sv::SN_DestroyGroundItem destroy;
		destroy.itemID = (GroundItemUID)itemUID;
		SendPacket(clientHandle[clientID], destroy);
	}
}

void Replication::SendClientLevelEventSeqToAll(i32 eventID, ActorUID caller)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		Sv::SN_RunClientLevelEventSeq seq;
		seq.needCompleteTriggerAckID = -1;
		seq.rootEventID = eventID;
		const LocalActorID callerLocal = caller != ActorUID::INVALID ? GetLocalActorID(clientHd, caller) : LocalActorID::INVALID;
		seq.caller = callerLocal != LocalActorID::INVALID ? (u32)callerLocal : 0;
		seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
		SendPacket(clientHd, seq);
	}
}

void Replication::SendClientLevelEventSeq(ClientHandle clientHd, i32 eventID)
{
	Sv::SN_RunClientLevelEventSeq seq;
	seq.needCompleteTriggerAckID = -1;
	seq.rootEventID = eventID;
	seq.caller = 0;
	seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
	SendPacket(clientHd, seq);
}

void Replication::SendChatWhisperConfirmToClient(ClientHandle senderClientHd, const wchar* destNick, const wchar* msg, ErrorType retval)
{
	PacketWriter<Sv::SA_WhisperSend> packet;

	packet.Write<ErrorType>(retval);
	packet.WriteStringObj(destNick);
	packet.WriteStringObj(msg);

	SendPacket(senderClientHd, packet);
}

void Replication::SendChatWhisperToClient(ClientHandle destClientHd, const wchar* senderName, const wchar* msg)
{
	PacketWriter<Sv::SN_WhisperReceived> packet;

	packet.WriteStringObj(senderName); // senderNick
	packet.Write<u8>(0); // staffType
	packet.WriteStringObj(msg); // msg

	SendPacket(destClientHd, packet);
}

static const i32 PROFILE_WEAPON_TYPES = 3;
static const i32 MEASURED_TRAINING_BENCH_CHARACTER_TYPE = 5;
static const i32 PROFILE_WEAPON_STRIDE = 4;

void Replication::SendAccountDataPvp(ClientHandle clientHd)
{
	// SN_Money
	// SN_ProfileCharacters
	// SN_ProfileItems
	// SN_ProfileWeapons
	// SN_ProfileSkills
	// SN_ProfileMasterGears
	// SN_PlayerSkillSlot x2
	// SN_AccountEquipmentList
	// SN_GameFieldReady

	const GameXmlContent& content = GetGameXmlContent();

	const u32 playerIndex = playerMap.at(clientHd);
	const Player* player = frameCur->FindPlayer(playerIndex);
	ASSERT(player);

	const GameXmlContent::Master& masterMain = content.GetMaster(player->mainClass);
	const GameXmlContent::Master& masterSub = content.GetMaster(player->subClass);
	const GameXmlContent::StageMaps* stage = content.FindStageMaps(stageIndex);
	const bool fullRoster = stage && stage->gameMode == GameModeType::GAME_MODE_MASTER_TRAINING;

	// SN_AccountInfo
	{
		PacketWriter<Sv::SN_AccountInfo> packet;

		packet.WriteStringObj(player->name.data()); // nick
		packet.Write<i32>(4); // inventoryLineCountTab0
		packet.Write<i32>(4); // inventoryLineCountTab1
		packet.Write<i32>(4); // inventoryLineCountTab2
		packet.Write<i32>(player->displayTitle);
		packet.Write<i32>(player->statTitle);
		packet.Write<i32>(MEASURED_WAREHOUSE_LINES);
		packet.Write<i32>(-1); // tutorialState
		packet.Write<i32>(3600); // masterGearDurability
		packet.Write<u8>(player->badgeType);

		SendPacket(clientHd, packet);
	}

	// SN_ProfileCharacters
	{
		const ActorMaster* actorMain = frameCur->FindMaster(player->masters[0]);
		const ActorMaster* actorSub = frameCur->FindMaster(player->masters[1]);
		ASSERT(actorMain);
		ASSERT(actorSub);

		PacketWriter<Sv::SN_ProfileCharacters,4096> packet;

		packet.Write<u16>(fullRoster ? (u16)content.masters.size() : (u16)2);

		{
			Sv::SN_ProfileCharacters::Character chara;
			chara.characterID = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)player->mainClass);
			chara.creatureIndex = CreatureIndex(100000000 + (i32)player->mainClass);
			chara.skillSlot1 = player->skills[0];
			chara.skillSlot2 = player->skills[1];
			chara.classType = player->mainClass;
			chara.x = actorMain->pos.x;
			chara.y = actorMain->pos.y;
			chara.z = actorMain->pos.z;
			chara.characterType = player->mainCharacterType;
			chara.skinIndex = player->mainSkin;
			chara.weaponIndex = player->mainWeapon;
			chara.masterGearNo = player->mainMasterGearNo;
			packet.Write(chara);
		}

		{
			Sv::SN_ProfileCharacters::Character chara;
			chara.characterID = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)player->subClass);
			chara.creatureIndex = CreatureIndex(100000000 + (i32)player->subClass);
			chara.skillSlot1 = player->skills[2];
			chara.skillSlot2 = player->skills[3];
			chara.classType = player->subClass;
			chara.x = actorSub->pos.x;
			chara.y = actorSub->pos.y;
			chara.z = actorSub->pos.z;
			chara.characterType = player->subCharacterType;
			chara.skinIndex = player->subSkin;
			chara.weaponIndex = player->subWeapon;
			chara.masterGearNo = player->subMasterGearNo;
			packet.Write(chara);
		}

		if(fullRoster) {
			foreach_const(m, content.masters) {
				if(m->classType == player->mainClass || m->classType == player->subClass) continue;
				Sv::SN_ProfileCharacters::Character chara;
				chara.characterID = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)m->classType);
				chara.creatureIndex = m->ID;
				chara.skillSlot1 = m->skillIDs[0];
				chara.skillSlot2 = m->skillIDs[1];
				chara.classType = m->classType;
				chara.x = actorMain->pos.x;
				chara.y = actorMain->pos.y;
				chara.z = actorMain->pos.z;
				chara.characterType = MEASURED_TRAINING_BENCH_CHARACTER_TYPE;
				chara.skinIndex = SkinIndex::DEFAULT;
				chara.weaponIndex = m->defaultWeaponIDs.empty() ? m->weaponIDs[0] : m->defaultWeaponIDs[0];
				chara.masterGearNo = 1;
				packet.Write(chara);
			}
		}

		SendPacket(clientHd, packet);
	}

	// SN_ProfileWeapons
	{
		PacketWriter<Sv::SN_ProfileWeapons,8192> packet;

		packet.Write<u16>(fullRoster ? (u16)(2 + (content.masters.size() - 2) * PROFILE_WEAPON_TYPES) : (u16)2);

		Sv::SN_ProfileWeapons::Weapon weap;
		weap.characterID = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)player->mainClass);
		weap.weaponType = content.WeaponTypeOf(player->mainClass, player->mainWeapon);
		weap.weaponIndex = player->mainWeapon;
		weap.grade = player->mainWeaponGrade;
		weap.isUnlocked = 1;
		weap.isActivated = 1;
		packet.Write(weap);

		weap.characterID = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)player->subClass);
		weap.weaponType = content.WeaponTypeOf(player->subClass, player->subWeapon);
		weap.weaponIndex = player->subWeapon;
		weap.grade = player->subWeaponGrade;
		weap.isUnlocked = 1;
		weap.isActivated = 1;
		packet.Write(weap);

		if(fullRoster) {
			foreach_const(m, content.masters) {
				if(m->classType == player->mainClass || m->classType == player->subClass) continue;
				weap.characterID = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)m->classType);
				for(int wi = 0; wi < PROFILE_WEAPON_TYPES; wi++) {
					const u32 at = (u32)(1 + wi * PROFILE_WEAPON_STRIDE);
					weap.weaponType = wi + 1;
					weap.weaponIndex = at < m->weaponIDs.size() ? m->weaponIDs[at] : m->weaponIDs[0];
					weap.grade = 0;
					weap.isUnlocked = wi == 0;
					weap.isActivated = wi == 0;
					packet.Write(weap);
				}
			}
		}

		SendPacket(clientHd, packet);
	}

	// SN_ProfileItems
	{
		PacketWriter<Sv::SN_ProfileItems,8192> packet;
		const AccountInventory inventory = AccountInventoryLoad(player->name.data());
		packet.Write<u8>(1);
		packet.Write<u16>((u16)inventory.items.size());
		foreach_const(it, inventory.items) WriteInventoryItem(packet, *it);

		SendPacket(clientHd, packet);
	}

	// SN_ProfileSkills
	{
		PacketWriter<Sv::SN_ProfileSkills,16384> packet;

		packet.Write<u8>(1); // packetNum

		i32 skillCount = masterMain.skillIDs.size() + masterSub.skillIDs.size();
		if(fullRoster) {
			skillCount = 0;
			foreach_const(m, content.masters) skillCount += (i32)m->skillIDs.size();
		}
		packet.Write<u16>((u16)skillCount);

		auto writeSkills = [&](const GameXmlContent::Master& master) {
			const LocalActorID characterID = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)master.classType);
			for(int si = 0; si < (int)master.skillIDs.size(); si++) {
				packet.Write(characterID);
				packet.Write(master.skillIDs[si]);
				const u8 unlocked = (si < (int)master.skillUnlocked.size()) ? master.skillUnlocked[si] : (u8)1;
				packet.Write<u8>(unlocked);
				packet.Write<u8>(1);
				packet.Write<u16>(0);
			}
		};
		if(fullRoster) {
			foreach_const(m, content.masters) writeSkills(*m);
		}
		else {
			writeSkills(masterMain);
			writeSkills(masterSub);
		}

		SendPacket(clientHd, packet);
	}

	// SN_ProfileMasterGears
	{
		PacketWriter<Sv::SN_ProfileMasterGears,4096> packet;

		packet.Write<u16>(0);

		SendPacket(clientHd, packet);
	}

	// SN_AccountEquipmentList
	{
		PacketWriter<Sv::SN_AccountEquipmentList,4096> packet;
		packet.Write<i32>(player->supportKit);
		SendPacket(clientHd, packet);
	}

	// SN_Money
	{
		Sv::SN_Money money;
		money.nMoney = 0;
		money.nReason = 1;
		SendPacket(clientHd, money);
	}

	// SN_LoadingProgressData
	{
		foreach_const(pit, frameCur->playerList) {
			if(pit->clientHd == clientHd) continue; // skip self

			PacketWriter<Sv::SN_LoadingProgressData> packet;
			packet.Write(pit->userID); //userID
			packet.WriteStringObj(pit->name.data()); // nickname

			// bot
			if(pit->clientHd == ClientHandle::INVALID) {
				packet.Write<u8>(0); // progressData
			}
			else {
				packet.Write<u8>(0); // progressData
			}

			packet.Write<CreatureIndex>((CreatureIndex)(100000000 + (i32)pit->mainClass)); //activeCreatureIndex
			packet.Write<CreatureIndex>((CreatureIndex)(100000000 + (i32)pit->subClass)); //inactiveCreatureIndex
			packet.Write<u8>(pit->team == 2); // isSpectator
			SendPacket(clientHd, packet);
		}
	}

	{
		PacketWriter<Sv::SN_GameFieldReady,4096> packet;

		packet.Write<i32>(inGameID);
		packet.Write(gameType);
		packet.Write(areaIndex);
		packet.Write(stageIndex);
		packet.Write(GameDefinition::System);
		packet.Write<u8>((u8)frameCur->playerList.size());
		packet.Write<u8>(canEscape);
		packet.Write<u8>(isTrespass);
		packet.Write<u8>(player->team == 2);

		packet.Write<u16>(frameCur->playerList.size());

		foreach_const(pit, frameCur->playerList) {
			packet.Write(pit->userID);
			packet.WriteStringObj(pit->name.data());
			packet.Write<u8>(3 + pit->team);
			packet.Write<u8>(pit->clientHd == ClientHandle::INVALID);
		}

		packet.Write<u16>(frameCur->playerList.size());

		foreach_const(pit, frameCur->playerList) {
			const Player& p = *pit;

			packet.Write(p.userID);
			packet.Write<CreatureIndex>((CreatureIndex)(100000000 + (i32)p.mainClass));
			packet.Write<SkinIndex>(p.mainSkin);
			packet.Write<SkillID>(p.skills[0]);
			packet.Write<SkillID>(p.skills[1]);
			packet.Write<CreatureIndex>((CreatureIndex)(100000000 + (i32)p.subClass));
			packet.Write<SkinIndex>(p.subSkin);
			packet.Write<SkillID>(p.skills[2]);
			packet.Write<SkillID>(p.skills[3]);
			packet.Write<SkillID>(p.stageSkills[0]);
			packet.Write<SkillID>(p.stageSkills[1]);
			packet.Write<i32>(p.supportKit);
			packet.Write<u8>(p.clientHd == ClientHandle::INVALID);
		}

		packet.Write<u16>(0);
		packet.Write<i32>(surrenderAbleTime);

		SendPacket(clientHd, packet);
	}
}

void Replication::SendPvpLoadingComplete(ClientHandle clientHd)
{
	const u32 playerIndex = playerMap.at(clientHd);
	const Player* player = frameCur->FindPlayer(playerIndex);
	ASSERT(player);

	{
		PacketWriter<Sv::SN_NotifyPcDetailInfos,1024> packet;

		packet.Write<u16>((u16)frameCur->playerList.size());

		foreach_const(pl, frameCur->playerList) {
			const Player& p = *pl;
			const ActorMaster* main = frameCur->FindMaster(p.masters[0]);
			const ActorMaster* sub = frameCur->FindMaster(p.masters[1]);
			ASSERT(main);
			ASSERT(sub);

			packet.Write<UserID>(p.userID);
			packet.Write<LocalActorID>(GetLocalActorID(clientHd, main->actorUID));
			packet.Write<CreatureIndex>(CreatureIndex(100000000 + (i32)main->classType));
			packet.Write<ClassType>(main->classType);
			packet.Write<i32>(main->hp);
			packet.Write<i32>(main->hpMax);
			packet.Write<LocalActorID>(GetLocalActorID(clientHd, sub->actorUID));
			packet.Write<CreatureIndex>(CreatureIndex(100000000 + (i32)sub->classType));

			packet.Write<ClassType>(sub->classType);
			packet.Write<i32>(sub->hp);
			packet.Write<i32>(sub->hpMax);

			packet.Write<i32>(0);
			packet.Write<u8>(0);
		}


		SendPacket(clientHd, packet);
	}

	{
		PacketWriter<Sv::SN_InitScoreBoard,4096> packet;

		packet.Write<u16>(frameCur->playerList.size());

		foreach_const(pl, frameCur->playerList) {
			const Player& p = *pl;
			const ActorMaster* main = frameCur->FindMaster(p.masters[0]);
			const ActorMaster* sub = frameCur->FindMaster(p.masters[1]);
			ASSERT(main);
			ASSERT(sub);
			ASSERT(p.team == 0 || p.team == 1);

			packet.Write<UserID>(p.userID);
			packet.WriteStringObj(p.name.data());
			packet.Write<TeamType>((TeamType)PlayerTeamType(p.team));
			packet.Write<CreatureIndex>(CreatureIndex(100000000 + (i32)main->classType));
			packet.Write<CreatureIndex>(CreatureIndex(100000000 + (i32)sub->classType));
		}


		SendPacket(clientHd, packet);
	}


	SendPacketData<Sv::SA_LoadingComplete>(clientHd,  0, nullptr);
}


void Replication::SendGameReady(ClientHandle clientHd, i32 waitTime, i32 elapsed, UserID userId)
{
	Sv::SA_GameReady ready;
	ready.waitingTimeMS = waitTime;
	ready.serverTimestamp = (i64)TimeDiffMs(TimeRelNow());
	ready.readyElapsedMS = elapsed;
	SendPacket(clientHd, ready);

	if(pveTeams) return;
	Sv::SN_NotifyIngameSkillPoint notify;
	notify.userId = userId;

	notify.skillPoint = 1;
	SendPacket(clientHd, notify);
}

void Replication::SendActivatedSupportKit(ClientHandle clientHd, LocalActorID chara, i32 kit, i32 coolTimeID, f32 coolTime)
{
	PacketWriter<Sv::SN_ActivatedSupportkit,32> packet;
	packet.Write<LocalActorID>(chara);
	packet.Write<i32>(kit);
	packet.Write<i32>(coolTimeID);
	packet.Write<f32>(coolTime);
	SendPacket(clientHd, packet);
}

void Replication::SendKitConsumed(ClientHandle clientHd, u32 uid, u8 invenType, i32 slot, i32 doc, i32 count)
{
	static const u32 MEASURED_ITEM_UPDATE_REASON_SUPPORT_KIT = 202;
	PacketWriter<Sv::SN_ItemUpdate,64> packet;
	packet.Write<u16>(1);
	packet.Write<u32>(uid);
	packet.Write<u8>(count > 0 ? invenType : 0);
	packet.Write<i32>(count > 0 ? slot : 0);
	packet.Write<i32>(count > 0 ? doc : 0);
	packet.Write<i32>(count > 0 ? count : 0);
	packet.Write<i32>(count > 0 ? -1 : 0);
	packet.Write<u8>(0);
	packet.Write<i64>(0);
	packet.Write<u16>(0);
	packet.Write<u32>(MEASURED_ITEM_UPDATE_REASON_SUPPORT_KIT);
	SendPacket(clientHd, packet);
}

void Replication::SendSkillPoints(ClientHandle clientHd, UserID userId, i32 points)
{
	Sv::SN_NotifyIngameSkillPoint notify;
	notify.userId = userId;
	notify.skillPoint = points;
	SendPacket(clientHd, notify);
}

static void WriteProfileSkill(PacketWriter<Sv::SA_SkillUpgradeIngame,512>& packet, LocalActorID chara, SkillID skill, bool active, Slice<const i32> props)
{
	packet.Write<LocalActorID>(chara);
	packet.Write<LocalActorID>(chara);
	packet.Write<SkillID>(skill);
	packet.Write<u8>(active ? 1 : 0);
	packet.Write<u8>(1);
	packet.Write<u16>((u16)props.size());
	foreach_const(p, props) {
		packet.Write<u32>((u32)*p);
		packet.Write<u32>(1);
	}
}

void Replication::SendSkillUpgradeResult(ClientHandle clientHd, i32 result, ActorUID mainUID, SkillID mainSkill, Slice<const i32> mainProps, ActorUID subUID, SkillID subSkill, Slice<const i32> subProps)
{
	PacketWriter<Sv::SA_SkillUpgradeIngame,512> packet;
	packet.Write<i32>(result);
	WriteProfileSkill(packet, GetLocalActorID(clientHd, mainUID), mainSkill, true, mainProps);
	WriteProfileSkill(packet, GetLocalActorID(clientHd, subUID), subSkill, false, subProps);
	SendPacket(clientHd, packet);
}

void Replication::SendPlayerSkillUpdateToAll(ActorUID charaUID, SkillID skill, bool active, Slice<const i32> props)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID local = GetLocalActorID(clientHd, charaUID);
		if(local == LocalActorID::INVALID) continue;
		PacketWriter<Sv::SN_PlayerSkillUpdate,512> packet;
		packet.Write<LocalActorID>(local);
		packet.Write<SkillID>(skill);
		packet.Write<u32>(0);
		packet.Write<u8>(1);
		packet.Write<u16>((u16)props.size());
		foreach_const(p, props) {
			packet.Write<u32>((u32)*p);
			packet.Write<u32>(1);
		}
		packet.Write<u8>(active ? 1 : 0);
		packet.Write<u8>(1);
		SendPacket(clientHd, packet);
	}
}

static const i32 MEASURED_GAME_START_ACK_EVENT = 1000000;

void Replication::SendPreGameLevelEvents(ClientHandle clientHd)
{
	// countdown
	Sv::SN_NotifyTimestamp notifyTimestamp;
	notifyTimestamp.serverTimestamp = (i64)TimeDiffMs(TimeRelNow());
	notifyTimestamp.curCount = 0;
	notifyTimestamp.maxCount = 5;
	SendPacket(clientHd, notifyTimestamp);


	if(mapIndex != MapIndex::PVP_DEATHMATCH) {
		Sv::SN_RunClientLevelEventSeq seq;
		seq.needCompleteTriggerAckID = MEASURED_GAME_START_ACK_EVENT;
		seq.rootEventID = MEASURED_GAME_START_ACK_EVENT + 1;
		seq.caller = 21035;
		seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
		SendPacket(clientHd, seq);
		return;
	}

	// EVENT: Starts music?
	// Sv::SN_RunClientLevelEventSeq
	{
		Sv::SN_RunClientLevelEventSeq seq;
		seq.needCompleteTriggerAckID = -1;
		seq.rootEventID = 218;
		seq.caller = 0;
		seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
		SendPacket(clientHd, seq);
	}

	// EVENT: You have entered the Combat Arena, the match begins soon
	// Sv::SN_RunClientLevelEventSeq
	{
		Sv::SN_RunClientLevelEventSeq seq;
		seq.needCompleteTriggerAckID = -1;
		seq.rootEventID = 219;
		seq.caller = 0;
		seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
		SendPacket(clientHd, seq);
	}

	// EVENT: Locks skill casting
	// Sv::SN_RunClientLevelEventSeq
	{
		Sv::SN_RunClientLevelEventSeq seq;
		seq.needCompleteTriggerAckID = -1;
		seq.rootEventID = 274;
		seq.caller = 0;
		seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
		SendPacket(clientHd, seq);
	}

	// EVENT: Pings the middle of the map where a powerup is
	// Sv::SN_RunClientLevelEvent
	{
		Sv::SN_RunClientLevelEvent event;
		event.eventID = 48;
		event.caller = 0;
		event.serverTime = (i64)TimeDiffMs(TimeRelNow());
		SendPacket(clientHd, event);
	}

	// EVENT: ???
	// Sv::SN_RunClientLevelEventSeq
	{
		Sv::SN_RunClientLevelEventSeq seq;
		seq.needCompleteTriggerAckID = 1000000;
		seq.rootEventID = 1000001;
		seq.caller = 21035;
		seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
		SendPacket(clientHd, seq);
	}

	// EVENT: ???
	// Sv::SN_RunClientLevelEvent
	{
		Sv::SN_RunClientLevelEvent event;
		event.eventID = 150;
		event.caller = 21035;
		event.serverTime = (i64)TimeDiffMs(TimeRelNow());
		SendPacket(clientHd, event);
	}

	/*
	Sv::SN_NotifyIsInSafeZone safe;
	safe.userID = 1;
	safe.inSafeZone = 1;
	LOG("[client%03d] Server :: SN_NotifyIsInSafeZone", clientID);
	SendPacket(clientHd, safe);
	*/
}

void Replication::SendGameStart(ClientHandle clientHd)
{
	SendPacketData<Sv::SN_GameStart>(clientHd, 0, nullptr);

	Sv::SN_NotifyAasRestricted notify;
	notify.isRestrictedByAAS = 0;
	SendPacket(clientHd, notify);
	if(titanMode) return;

	// EVENT: Release input lock (Data\Design\Level\PVP\PVP_DeathMatch\EVENTNODES\LEVELEVENT_CLIENT.XML)
	// Sv::SN_RunClientLevelEventSeq
	{
		Sv::SN_RunClientLevelEventSeq seq;
		seq.needCompleteTriggerAckID = -1;
		seq.rootEventID = 163;
		seq.caller = 0;
		seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
		SendPacket(clientHd, seq);
	}


	// EVENT: Start the internal battle timer (240s)
	// -> will then call following sequence events (271, 272, 273, 274). "battle will end in 1min, 30s".
	// Sv::SN_RunClientLevelEventSeq
	{
		Sv::SN_RunClientLevelEventSeq seq;
		seq.needCompleteTriggerAckID = -1;
		seq.rootEventID = 270;
		seq.caller = 0;
		seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
		SendPacket(clientHd, seq);
	}
}

void Replication::SendPlayerTag(ClientHandle clientHd, ActorUID mainActorUID, ActorUID subActorUID)
{
	Sv::SN_GamePlayerTag tag;
	tag.result = 128;
	tag.mainID = GetLocalActorID(clientHd, mainActorUID);
	tag.subID = GetLocalActorID(clientHd, subActorUID);
	tag.attackerID = LocalActorID::INVALID;

	ASSERT(tag.mainID != LocalActorID::INVALID);
	ASSERT(tag.subID != LocalActorID::INVALID);

	SendPacket(clientHd, tag);
	auto found = playerMap.find(clientHd);
	if(found != playerMap.end()) playerLocalInfo[found->second].shownMain[MIN(mainActorUID, subActorUID)] = mainActorUID;
}

void Replication::SendPlayerJump(ClientHandle clientHd, ActorUID mainActorUID, f32 rotate, f32 moveDirX, f32 moveDirY)
{
	LocalActorID localActorID = GetLocalActorID(clientHd, mainActorUID);
	ASSERT(localActorID != LocalActorID::INVALID);

	PacketWriter<Sv::SA_ResultSpAction> packet;

	packet.Write<u8>(0x20); // excludedFieldBits
	packet.Write<i32>(0); // actionID
	packet.Write<LocalActorID>(GetLocalActorID(clientHd, mainActorUID));
	packet.Write<f32>(rotate);
	packet.Write<f32>(moveDirX);
	packet.Write<f32>(moveDirY);
	packet.Write<i32>(0); // errorType

	SendPacket(clientHd, packet);
}

void Replication::SendDamageToAll(ActorUID attackerUID, ActorUID targetUID, SkillID skillID, i32 damage, i32 hp, i32 hpMax, u32 damageType, i32 statusID)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;

		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID targetLocalID = GetLocalActorID(clientHd, targetUID);
		if(targetLocalID == LocalActorID::INVALID) continue;
		if(playerLocalInfo[clientID].hiddenActors.count(targetUID)) continue;
		const LocalActorID attackerLocalID = GetLocalActorID(clientHd, attackerUID);

		Sv::SN_BroadcastNonRemoteDamage dmg;
		dmg.defID = (u32)targetLocalID;
		dmg.attID = (u32)attackerLocalID;
		const StatusInfo* status = statusID != 0 ? GetGameXmlContent().FindStatus(statusID) : nullptr;
		dmg.statusDocID = (u32)statusID;
		dmg.skillDocID = status ? (u32)status->skillIndex : (skillID == SkillID::INVALID ? 0 : (u32)skillID);
		dmg.damageType = status ? PACKET_DAMAGE_TYPE_STATUS : damageType;
		dmg.damage = (u32)damage;
		dmg.masterGroupingDamage = status ? 0 : (u32)damage;
		SendPacket(clientHd, dmg);
	}

	SendHpToAll(targetUID, hp, hpMax);
}

void Replication::SendDoConnectChannel(ClientHandle clientHd, const wchar* nickname, const i32 hubIP[4], u16 hubPort)
{
	Sv::SA_ReturnToCity ret;
	ret.errcode = 0;
	SendPacket(clientHd, ret);

	PacketWriter<Sv::SN_DoConnectChannelServer,256> packet;
	packet.Write<u16>(1);
	const u8 ip[4] = { (u8)hubIP[0], (u8)hubIP[1], (u8)hubIP[2], (u8)hubIP[3] };
	packet.Write<u8[4]>(ip);
	packet.Write<u16>(hubPort);
	const wchar* serverName = L"XMX_SERVER";
	packet.Write<u16>(10);
	packet.WriteRaw(serverName, 10 * sizeof(wchar));
	packet.WriteStringObj(nickname, (i32)EA::StdC::Strlen(nickname));
	packet.Write<i32>(536);
	packet.Write<i32>(1);
	LOG("[client%x] Replication :: SendDoConnectChannel (return to bridge)", clientHd);
	SendPacket(clientHd, packet);
}

void Replication::SendGameEnd(const EndPlayer* players, i32 count, i32 redScore, i32 blueScore, u32 playTimeMs, i32 winnerTeam, const EndGot* got)
{
	u32 redResult = redScore > blueScore ? 1 : (redScore < blueScore ? 2 : 3);
	u32 blueResult = redScore < blueScore ? 1 : (redScore > blueScore ? 2 : 3);
	if(winnerTeam == 0 || winnerTeam == 1) {
		redResult = winnerTeam == 0 ? 1 : 2;
		blueResult = winnerTeam == 1 ? 1 : 2;
	}

	PacketWriter<Sv::SN_PvpResultScoreGot,8192> scoreGot;
	if(got) {
		scoreGot.Write<u16>(2);
		for(i32 team = 0; team < 2; team++) {
			scoreGot.Write<TeamType>(team == 0 ? TeamType::RED : TeamType::BLUE);
			scoreGot.Write<u32>((u32)(team == 0 ? redScore : blueScore));
			scoreGot.Write<u16>((u16)got->summons[team].size());
			foreach_const(s, got->summons[team]) scoreGot.Write<u32>((u32)*s);
		}
		scoreGot.Write<u16>((u16)count);
		for(i32 i = 0; i < count; i++) {
			const EndPlayer& p = players[i];
			scoreGot.Write<u32>((u32)p.userID);
			scoreGot.WriteStringObj(p.name, (i32)EA::StdC::Strlen(p.name));
			scoreGot.Write<TeamType>(p.team == 0 ? TeamType::RED : TeamType::BLUE);
			scoreGot.Write<u8>(0);
			scoreGot.Write<u8>(p.ingameLevel);
			scoreGot.Write<u32>((u32)p.points);
			for(int m = 0; m < 2; m++) {
				u16 n = 0;
				for(u8 lv : p.traits[m]) if(lv > 0) n++;
				scoreGot.Write<u16>(n);
				for(u8 t = 0; t < 4; t++) {
					if(p.traits[m][t] == 0) continue;
					scoreGot.Write<u8>(t);
					scoreGot.Write<u8>(p.traits[m][t]);
				}
			}
			scoreGot.Write<u16>(2);
			for(int m = 0; m < 2; m++) {
				const bool main = m == 0;
				scoreGot.Write<u32>((u32)p.masters[m]);
				scoreGot.Write<u32>(p.masterScores[m].playMs);
				scoreGot.Write<u8>((u8)p.masterScores[m].kills);
				scoreGot.Write<u8>((u8)(main ? p.bestKillStreak : 0));
				scoreGot.Write<u8>(0);
				scoreGot.Write<u8>((u8)p.masterScores[m].deaths);
				scoreGot.Write<u8>((u8)p.masterScores[m].assists);
				scoreGot.Write<u32>((u32)p.masterScores[m].damageDealt);
				for(int d = 0; d < 9; d++) scoreGot.Write<u32>(p.masterScores[m].damageStats[d]);
				scoreGot.Write<u8>(main ? p.monsterKills[0] : 0);
				scoreGot.Write<u8>(main ? p.monsterKills[1] : 0);
				scoreGot.Write<u8>(main ? p.statueCaptures : 0);
				scoreGot.Write<u8>((u8)(main ? p.savedPieces : 0));
				scoreGot.Write<u8>(main ? p.visionCaptures : 0);
				scoreGot.Write<u8>(main ? p.monsterKills[2] : 0);
				scoreGot.Write<u8>(main ? p.monsterKills[3] : 0);
			}
			scoreGot.Write<u16>(0);
			scoreGot.Write<u16>(0);
		}
	}

	static const u32 MEASURED_PRAISE_INFO_DURATION_MS = 45000;
	PacketWriter<Sv::SN_PraiseInfo,2048> praiseInfo;
	if(got) {
		praiseInfo.Write<u32>(MEASURED_PRAISE_INFO_DURATION_MS);
		praiseInfo.Write<u16>((u16)got->praises.size());
		foreach_const(pr, got->praises) {
			praiseInfo.Write<u32>((u32)pr->userID);
			praiseInfo.WriteStringObj(pr->name, (i32)EA::StdC::Strlen(pr->name));
			praiseInfo.Write<TeamType>(pr->team == 0 ? TeamType::RED : TeamType::BLUE);
			praiseInfo.Write<CreatureIndex>(pr->activeMaster);
			praiseInfo.Write<CreatureIndex>(pr->inactiveMaster);
			praiseInfo.Write<u32>((u32)pr->type);
			praiseInfo.Write<u32>(pr->value);
			praiseInfo.Write<u32>((u32)pr->relation);
			praiseInfo.Write<u32>(pr->relationValue);
		}
	}

	PacketWriter<Sv::SN_PvpResultScoreOccupy,8192> scoreOccupy;
	if(occupyMode) {
		scoreOccupy.Write<u16>(2);
		scoreOccupy.Write<TeamType>(TeamType::RED);
		scoreOccupy.Write<u32>((u32)redScore);
		scoreOccupy.Write<TeamType>(TeamType::BLUE);
		scoreOccupy.Write<u32>((u32)blueScore);
		scoreOccupy.Write<u16>((u16)count);
		for(i32 i = 0; i < count; i++) {
			const EndPlayer& p = players[i];
			scoreOccupy.Write<u32>((u32)p.userID);
			scoreOccupy.WriteStringObj(p.name, (i32)EA::StdC::Strlen(p.name));
			scoreOccupy.Write<TeamType>(p.team == 0 ? TeamType::RED : TeamType::BLUE);
			scoreOccupy.Write<u8>(0);
			scoreOccupy.Write<u16>(2);
			for(int m = 0; m < 2; m++) {
				const bool main = m == 0;
				scoreOccupy.Write<u32>((u32)p.masters[m]);
				scoreOccupy.Write<u32>(p.masterScores[m].playMs);
				scoreOccupy.Write<u8>((u8)p.masterScores[m].kills);
				scoreOccupy.Write<u8>((u8)(main ? p.bestKillStreak : 0));
				scoreOccupy.Write<u8>(0);
				scoreOccupy.Write<u8>((u8)p.masterScores[m].deaths);
				scoreOccupy.Write<u8>((u8)p.masterScores[m].assists);
				scoreOccupy.Write<u32>((u32)p.masterScores[m].damageDealt);
				for(int d = 0; d < 7; d++) scoreOccupy.Write<u32>(0);
				scoreOccupy.Write<u8>(main ? p.occupyCaptures : 0);
				scoreOccupy.Write<u16>((u16)(main ? p.occupyScore : 0));
			}
		}
	}

	PacketWriter<Sv::SN_PvpResultScoreDeathmatch,4096> score;
	score.Write<u16>(2);
	score.Write<TeamType>(TeamType::RED);
	score.Write<u32>((u32)redScore);
	score.Write<TeamType>(TeamType::BLUE);
	score.Write<u32>((u32)blueScore);
	score.Write<u16>((u16)count);
	for(i32 i = 0; i < count; i++) {
		const EndPlayer& p = players[i];
		score.Write<u32>((u32)p.userID);
		score.WriteStringObj(p.name, (i32)EA::StdC::Strlen(p.name));
		score.Write<TeamType>(p.team == 0 ? TeamType::RED : TeamType::BLUE);
		score.Write<u8>(0);
		score.Write<u32>((u32)p.kills);
		score.Write<u16>(2);
		for(int m = 0; m < 2; m++) {
			score.Write<u32>((u32)p.masters[m]);
			score.Write<u32>(p.masterScores[m].playMs);
			score.Write<u8>((u8)p.masterScores[m].kills);
			score.Write<u8>(0);
			score.Write<u8>(0);
			score.Write<u8>((u8)p.masterScores[m].deaths);
			score.Write<u8>(0);
			for(int d = 0; d < 8; d++) score.Write<u32>(0);
		}
	}

	static const u32 MEASURED_REWARD_ITEM_TYPE_ITEM = 2;
	static const u8 MEASURED_TIER_GAME_RESULT_UNRANKED = 6;
	for(i32 i = 0; i < count; i++) {
		const ClientHandle clientHd = players[i].clientHd;
		if(clientHd == ClientHandle::INVALID) continue;
		PacketWriter<Sv::SN_PvpResult,512> res;
		res.Write<u32>(0);
		res.Write<u32>(playTimeMs);
		res.Write<u16>(2);
		res.Write<TeamType>(TeamType::RED);
		res.Write<u32>(redResult);
		res.Write<TeamType>(TeamType::BLUE);
		res.Write<u32>(blueResult);
		res.Write<u32>((u32)players[i].userID);
		res.Write<u32>(players[i].gold);
		res.Write<u16>((u16)players[i].bonusRon.size());
		foreach_const(b, players[i].bonusRon) { res.Write<u32>(b->type); res.Write<u32>(b->amount); }
		res.Write<u32>(players[i].exp);
		res.Write<u16>((u16)players[i].bonusExp.size());
		foreach_const(b, players[i].bonusExp) { res.Write<u32>(b->type); res.Write<u32>(b->amount); }
		res.Write<u16>(players[i].medals > 0 ? 1 : 0);
		if(players[i].medals > 0) {
			res.Write<u32>(MEASURED_REWARD_ITEM_TYPE_ITEM);
			res.Write<u32>((u32)GetGameXmlContent().pvpMedalItem);
			res.Write<u32>(players[i].medals);
		}
		res.Write<u32>(players[i].guildBase);
		res.Write<u32>(players[i].guildBonus);
		res.Write<u8>(players[i].guildMembers);
		res.Write<u16>(players[i].guildBoost[0]);
		res.Write<u16>(players[i].guildBoost[1]);
		res.Write<u16>(0);
		for(int z = 0; z < 12; z++) res.Write<u8>(0);
		res.Write<u8>(MEASURED_TIER_GAME_RESULT_UNRANKED);
		res.Write<u8>(players[i].tierType);
		res.Write<u16>(players[i].tierPoints);
		res.Write<u16>(0);
		res.Write<u16>(0);
		res.Write<u16>(0);
		SendPacket(clientHd, res);
		if(got) {
			SendPacket(clientHd, scoreGot);
			if(!got->praises.empty()) SendPacket(clientHd, praiseInfo);
		}
		else if(occupyMode) SendPacket(clientHd, scoreOccupy);
		else SendPacket(clientHd, score);
	}
	for(i32 i = 0; i < count; i++) {
		const ClientHandle clientHd = players[i].clientHd;
		if(clientHd == ClientHandle::INVALID) continue;
		Sv::SA_RestartPvpEntryLeave leave;
		leave.errcode = 0;
		SendPacket(clientHd, leave);
	}
	LOG("Replication :: game end (red %d - blue %d)", redScore, blueScore);
}

void Replication::SendGauge(ClientHandle clientHd, UserID userID, u8 updateType, bool overheated, f32 overheatTime, u16 gauge, u16 maxGauge)
{
	if(clientHd == ClientHandle::INVALID) return;
	PacketWriter<Sv::SN_Gauge> packet;
	packet.Write<u32>((u32)userID);
	packet.Write<u8>(updateType);
	packet.Write<u8>(overheated ? 1 : 0);
	packet.Write<f32>(overheatTime);
	packet.Write<u16>(gauge);
	packet.Write<u16>(maxGauge);
	SendPacket(clientHd, packet);
}

void Replication::SendScoreToAll(UserID killerID, u8 team, i32 kills, i32 score, i32 teamKills, i32 teamScore, UserID victimID, u8 victimTeam, i32 victimDeaths, i32 victimTeamKills)
{
	enum : u32 { SCORE = 0, KILL_COUNT = 1, DEAD_COUNT = 14 };

	PacketWriter<Sv::SN_ScoreUpdate> killerPacket;
	killerPacket.Write<u32>((u32)killerID);
	killerPacket.Write<TeamType>(TeamType::INVALID);
	killerPacket.Write<u16>(2);
	killerPacket.Write<u32>(KILL_COUNT); killerPacket.Write<u32>((u32)kills);
	killerPacket.Write<u32>(SCORE);      killerPacket.Write<u32>((u32)score);

	PacketWriter<Sv::SN_ScoreUpdate> teamPacket;
	teamPacket.Write<u32>(0);
	teamPacket.Write<TeamType>(team == 0 ? TeamType::RED : TeamType::BLUE);
	teamPacket.Write<u16>(2);
	teamPacket.Write<u32>(KILL_COUNT); teamPacket.Write<u32>((u32)teamKills);
	teamPacket.Write<u32>(SCORE);      teamPacket.Write<u32>((u32)teamScore);

	PacketWriter<Sv::SN_ScoreUpdate> victimPacket;
	victimPacket.Write<u32>((u32)victimID);
	victimPacket.Write<TeamType>(TeamType::INVALID);
	victimPacket.Write<u16>(1);
	victimPacket.Write<u32>(DEAD_COUNT); victimPacket.Write<u32>((u32)victimDeaths);

	PacketWriter<Sv::SN_ScoreUpdate> victimTeamPacket;
	victimTeamPacket.Write<u32>(0);
	victimTeamPacket.Write<TeamType>(victimTeam == 0 ? TeamType::RED : TeamType::BLUE);
	victimTeamPacket.Write<u16>(1);
	victimTeamPacket.Write<u32>(DEAD_COUNT); victimTeamPacket.Write<u32>((u32)victimTeamKills);

	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		SendPacket(clientHandle[clientID], killerPacket);
		SendPacket(clientHandle[clientID], teamPacket);
		SendPacket(clientHandle[clientID], victimPacket);
		SendPacket(clientHandle[clientID], victimTeamPacket);
	}
}

void Replication::SendOccupyScoreToAll(UserID userID, u8 team, i32 captures, i32 occupyScore, i32 score, i32 teamHeld, i32 teamScore, i32 otherHeld, bool capture)
{
	enum : u32 { SCORE = 0, OCCUPY_COUNT = 27, OCCUPY_SCORE = 28 };
	const TeamType teamType = team == 0 ? TeamType::RED : TeamType::BLUE;
	PacketWriter<Sv::SN_ScoreUpdate> playerPacket;
	playerPacket.Write<u32>((u32)userID);
	playerPacket.Write<TeamType>(TeamType::INVALID);
	playerPacket.Write<u16>(capture ? 3 : 2);
	if(capture) { playerPacket.Write<u32>(OCCUPY_COUNT); playerPacket.Write<u32>((u32)captures); }
	playerPacket.Write<u32>(OCCUPY_SCORE); playerPacket.Write<u32>((u32)occupyScore);
	playerPacket.Write<u32>(SCORE); playerPacket.Write<u32>((u32)score);
	PacketWriter<Sv::SN_ScoreUpdate> teamPacket;
	teamPacket.Write<u32>(0);
	teamPacket.Write<TeamType>(teamType);
	teamPacket.Write<u16>(capture ? 2 : 1);
	if(capture) { teamPacket.Write<u32>(OCCUPY_COUNT); teamPacket.Write<u32>((u32)teamHeld); }
	teamPacket.Write<u32>(SCORE); teamPacket.Write<u32>((u32)teamScore);
	PacketWriter<Sv::SN_ScoreUpdate> otherPacket;
	otherPacket.Write<u32>(0);
	otherPacket.Write<TeamType>(team == 0 ? TeamType::BLUE : TeamType::RED);
	otherPacket.Write<u16>(1);
	otherPacket.Write<u32>(OCCUPY_COUNT); otherPacket.Write<u32>((u32)otherHeld);
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		SendPacket(clientHandle[clientID], playerPacket);
		SendPacket(clientHandle[clientID], teamPacket);
		if(capture) SendPacket(clientHandle[clientID], otherPacket);
	}
}

void Replication::SendPlayerScoreToAll(UserID userID, u32 type, i32 value)
{
	PacketWriter<Sv::SN_ScoreUpdate> packet;
	packet.Write<u32>((u32)userID);
	packet.Write<TeamType>(TeamType::INVALID);
	packet.Write<u16>(1);
	packet.Write<u32>(type);
	packet.Write<u32>((u32)MAX(value, 0));
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		SendPacket(clientHandle[clientID], packet);
	}
}

void Replication::SendAssistScoreToAll(UserID userID, u8 team, i32 assists, i32 score, i32 teamScore)
{
	enum : u32 { SCORE = 0, ASSIST_COUNT = 2 };

	PacketWriter<Sv::SN_ScoreUpdate> playerPacket;
	playerPacket.Write<u32>((u32)userID);
	playerPacket.Write<TeamType>(TeamType::INVALID);
	playerPacket.Write<u16>(2);
	playerPacket.Write<u32>(ASSIST_COUNT); playerPacket.Write<u32>((u32)assists);
	playerPacket.Write<u32>(SCORE);        playerPacket.Write<u32>((u32)score);

	PacketWriter<Sv::SN_ScoreUpdate> teamPacket;
	teamPacket.Write<u32>(0);
	teamPacket.Write<TeamType>(team == 0 ? TeamType::RED : TeamType::BLUE);
	teamPacket.Write<u16>(1);
	teamPacket.Write<u32>(SCORE); teamPacket.Write<u32>((u32)teamScore);

	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		SendPacket(clientHandle[clientID], playerPacket);
		SendPacket(clientHandle[clientID], teamPacket);
	}
}

void Replication::SendDeadDamageInfo(ClientHandle clientHd, u32 killerIndex, const DamageTaken* records, i32 count, i32 durationMs)
{
	if(clientHd == ClientHandle::INVALID) return;
	struct Entry { u32 attackerIndex; UserID userID; LocalActorID object; SkillID skill; i32 status; i32 damage; u8 damageType; };
	eastl::fixed_vector<Entry,48,false> entries;
	struct Group { u32 attackerIndex; UserID userID; i32 total; };
	eastl::fixed_vector<Group,MAX_PLAYERS,false> groups;
	for(i32 k = 0; k < count; k++) {
		const DamageTaken& d = records[k];
		const LocalActorID object = GetLocalActorID(clientHd, d.objectUID);
		bool merged = false;
		foreach(e, entries) {
			if(e->attackerIndex == d.attackerIndex && e->object == object && e->skill == d.skill && e->status == d.status) {
				e->damage += d.damage;
				merged = true;
				break;
			}
		}
		if(!merged && !entries.full()) entries.push_back({ d.attackerIndex, d.attackerUserID, object, d.skill, d.status, d.damage, d.damageType });
		bool grouped = false;
		foreach(g, groups) {
			if(g->attackerIndex == d.attackerIndex) {
				g->total += d.damage;
				grouped = true;
				break;
			}
		}
		if(!grouped && !groups.full()) groups.push_back({ d.attackerIndex, d.attackerUserID, d.damage });
	}
	eastl::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) { return a.damage > b.damage; });
	eastl::sort(groups.begin(), groups.end(), [killerIndex](const Group& a, const Group& b) {
		if((a.attackerIndex == killerIndex) != (b.attackerIndex == killerIndex)) return a.attackerIndex == killerIndex;
		return a.total > b.total;
	});

	static const u16 MEASURED_DEAD_DAMAGE_ITEMS_PER_ATTACKER = 3;
	static const u16 MEASURED_DEAD_DAMAGE_OTHER_ATTACKERS = 2;
	PacketWriter<Sv::SN_DeadDamageInfo> packet;
	auto writeList = [&](const Group& g) {
		u16 n = 0;
		foreach_const(e, entries) if(e->attackerIndex == g.attackerIndex) n++;
		n = MIN(n, MEASURED_DEAD_DAMAGE_ITEMS_PER_ATTACKER);
		packet.Write<u16>(n);
		u16 written = 0;
		foreach_const(e, entries) {
			if(e->attackerIndex != g.attackerIndex || written >= n) continue;
			written++;
			packet.Write<u32>((u32)e->object);
			packet.Write<u32>(e->skill != SkillID::INVALID ? (u32)e->skill : 0xFFFFFFFF);
			packet.Write<u32>(e->status != 0 ? (u32)e->status : 0xFFFFFFFF);
			packet.Write<u32>((u32)e->damage);
			packet.Write<u8>(0);
			packet.Write<u8>(e->status != 0 ? 1 : 0);
		}
		packet.Write<u32>((u32)g.userID);
	};
	const bool hasKiller = !groups.empty() && groups[0].attackerIndex == killerIndex;
	if(hasKiller) writeList(groups[0]);
	else {
		packet.Write<u16>(0);
		packet.Write<u32>(0);
	}
	const i32 first = hasKiller ? 1 : 0;
	const i32 others = MIN((i32)groups.size() - first, (i32)MEASURED_DEAD_DAMAGE_OTHER_ATTACKERS);
	packet.Write<u16>((u16)others);
	for(i32 g = first; g < first + others; g++) writeList(groups[g]);
	packet.Write<u32>((u32)durationMs);
	SendPacket(clientHd, packet);
}

static const f32 REMOTE_FIRE_HEIGHT = 158.0f;
static const u16 MAX_FIRE_OBJECT_LEN = 64;

static const u8 NPC_MOVE_FULL = 0x02;
static const u8 NPC_MOVE_YAW_AND_POSITIONS = 0x32;
static const u8 NPC_MOVE_POSITIONS_ONLY = 0x3e;
static const u8 NPC_MOVE_YAW_AND_SPEED = 0x12;
static const u8 NPC_MOVE_FLAG_STOPPED = 31;
static const u8 NPC_MOVE_FLAG_MOVING = 32;
static const u8 MEASURED_NPC_MOVE_FLAG_ACTION_STOP = 63;
static const f32 SUMMON_SPAWN_HEIGHT = 50.0f;
static const f32 NPC_SPEED_SCALE = 5.0f;
static const u8 NPC_STATE_KEEP_SPEED = 0x08;
static const u8 MONSTER_STATE_NO_YAW = 0x04;
static const f32 MONSTER_SPEED_SCALE = 5.0f;
static const u8 SUMMON_OWNER_SKILLPOWER_STAT = 8;
static const f32 REFLECT_TARGET_DISTANCE = 10000.0f;
static const u8 REFLECT_CREATE_FLAGS = 2;

static i16 WorldYawToMonsterDeciDegrees(f32 worldYaw)
{
	f32 deciDeg = (worldYaw * 180.0f / PI + 90.0f) * 10.0f;
	while(deciDeg >= 3600.0f) deciDeg -= 3600.0f;
	while(deciDeg < 0.0f) deciDeg += 3600.0f;
	return (i16)deciDeg;
}

static i16 WorldYawToCentiDegrees(f32 worldYaw)
{
	f32 yawDeg = 90.0f - worldYaw * 180.0f / PI;
	while(yawDeg > 180.0f) yawDeg -= 360.0f;
	while(yawDeg < -180.0f) yawDeg += 360.0f;
	return (i16)(yawDeg * 100.0f);
}

void Replication::SendReflectedShotToAll(ActorUID shooterUID, ActorUID reflectorUID, u32 seed, const vec3& firePos, f32 worldYaw, const wchar* fireObject, u32 groupSeed, RemoteIdx doc)
{
	RemoteCreate remote;
	remote.parentUID = shooterUID;
	remote.ownerUID = reflectorUID;
	remote.seed = seed;
	remote.firePos = firePos;
	remote.fireRotation[2] = (u16)WorldYawToCentiDegrees(worldYaw);
	remote.targetPos = firePos + vec3(cosf(worldYaw), sinf(worldYaw), 0) * REFLECT_TARGET_DISTANCE;
	remote.fireObject = fireObject;
	remote.fireObjectLen = fireObject ? (u16)EA::StdC::Strlen(fireObject) : 0;
	remote.hitInvalidTarget = groupSeed;
	remote.doc = doc;
	remote.flags = REFLECT_CREATE_FLAGS;
	SendRemoteCreate(remote, ClientHandle::INVALID);
}

void Replication::SendRemoteCreate(const RemoteCreate& remote, ClientHandle exclude)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME || clientHandle[clientID] == exclude) continue;
		SendRemoteCreateToClient(clientID, remote);
	}
}

bool Replication::SendRemoteCreateTo(ClientHandle clientHd, const RemoteCreate& remote)
{
	auto found = playerMap.find(clientHd);
	return found != playerMap.end() && SendRemoteCreateToClient(found->second, remote);
}

bool Replication::SendRemoteCreateToClient(i32 clientID, const RemoteCreate& remote)
{
	if(clientID < 0 || clientID >= MAX_PLAYERS || playerState[clientID].cur < PlayerState::IN_GAME) return false;
	const ClientHandle clientHd = clientHandle[clientID];

	const LocalActorID ownerLocal = GetLocalActorID(clientHd, remote.ownerUID);
	const bool throughFog = remote.doc != RemoteIdx::INVALID && (i32)remote.doc == GetGameXmlContent().respawnFlyRemote;
	if(ownerLocal == LocalActorID::INVALID || (!throughFog && playerLocalInfo[clientID].hiddenActors.count(remote.ownerUID))) return false;
	u32 parentField = 0;
	if(remote.parentRemoteID != 0) parentField = clientHd == remote.parentSeedOwnerHd ? remote.parentRemoteID - 1 : remote.parentRemoteID;
	else {
		const LocalActorID parentLocal = GetLocalActorID(clientHd, remote.parentUID);
		if(parentLocal == LocalActorID::INVALID || (!throughFog && playerLocalInfo[clientID].hiddenActors.count(remote.parentUID))) return false;
		parentField = (u32)parentLocal;
	}
	const LocalActorID hitLocal = remote.hitInvalidUID != ActorUID::INVALID ? GetLocalActorID(clientHd, remote.hitInvalidUID) : LocalActorID::INVALID;
	const u32 hitField = remote.hitInvalidUID == ActorUID::INVALID ? remote.hitInvalidTarget : (hitLocal != LocalActorID::INVALID ? (u32)hitLocal : 0);
	const LocalActorID targetLocal = remote.targetUID != ActorUID::INVALID ? GetLocalActorID(clientHd, remote.targetUID) : LocalActorID::INVALID;

	if(remote.creator != 0) {
		PacketWriter<Sv::SN_RemoteSyncCreateFromCreatorId,512> packet;
		packet.Write<u32>(parentField);
		packet.Write<u32>((u32)ownerLocal);
		packet.Write<u32>(remote.seed);
		packet.Write<float3>(v2f(remote.firePos));
		packet.Write<u16>(remote.fireRotation[0]);
		packet.Write<u16>(remote.fireRotation[1]);
		packet.Write<u16>(remote.fireRotation[2]);
		packet.Write<float3>(v2f(remote.targetPos));
		packet.Write<u16>(remote.fireObjectLen);
		packet.WriteRaw(remote.fireObject, remote.fireObjectLen * sizeof(wchar));
		packet.Write<u16>(remote.scale);
		packet.Write<f32>(remote.lifeTime);
		packet.Write<u32>(hitField);
		packet.Write<u16>(remote.creator);
		packet.Write<u16>(remote.errorAngle[0]);
		packet.Write<u16>(remote.errorAngle[1]);
		packet.Write<u8>(remote.chargingLevel);
		packet.Write<u8>(remote.gaugeLevel);
		packet.Write<u16>(remote.firingCount);
		packet.Write<u8>(remote.comboCount);
		packet.Write<u32>((u32)targetLocal);
		packet.Write<u8>(remote.flags);
		SendPacket(clientHd, packet);
		return true;
	}

	PacketWriter<Sv::SN_RemoteSyncCreateFromRemoteDoc,512> packet;
	packet.Write<u32>(parentField);
	packet.Write<u32>((u32)ownerLocal);
	packet.Write<u32>(remote.seed);
	packet.Write<float3>(v2f(remote.firePos));
	packet.Write<u16>(remote.fireRotation[0]);
	packet.Write<u16>(remote.fireRotation[1]);
	packet.Write<u16>(remote.fireRotation[2]);
	packet.Write<float3>(v2f(remote.targetPos));
	packet.Write<u16>(remote.fireObjectLen);
	packet.WriteRaw(remote.fireObject, remote.fireObjectLen * sizeof(wchar));
	packet.Write<u16>(remote.scale);
	packet.Write<f32>(remote.lifeTime);
	packet.Write<u32>(hitField);
	packet.Write<u32>((u32)remote.doc);
	packet.Write<u32>((u32)targetLocal);
	packet.Write<u8>(remote.flags);
	SendPacket(clientHd, packet);
	return true;
}

u16 Replication::RelayWeaponShot(ClientHandle senderHd, const u8* packetData, i32 packetSize, const vec3& target, RemoteIdx remoteDoc, bool groundTarget)
{
	ConstBuffer buff(packetData, packetSize);
	RemoteCreate remote;
	remote.parentUID = GetWorldActorUID(senderHd, buff.Read<LocalActorID>());
	remote.ownerUID = GetWorldActorUID(senderHd, buff.Read<LocalActorID>());
	remote.seed = buff.Read<u32>();
	const vec3 clientFirePos = f2v(buff.Read<float3>());
	buff.ReadRaw(offsetof(Cl::CQ_GamePlayerSyncPhy, charNameLen) - offsetof(Cl::CQ_GamePlayerSyncPhy, unkShort1));
	remote.fireObjectLen = buff.Read<u16>();
	if(remote.fireObjectLen > MAX_FIRE_OBJECT_LEN) return 0;
	if(packetSize < (i32)(offsetof(Cl::CQ_GamePlayerSyncPhy, charNameLen) + sizeof(u16) + remote.fireObjectLen * sizeof(wchar) + sizeof(u16))) return 0;
	remote.fireObject = (const wchar*)buff.ReadRaw(remote.fireObjectLen * sizeof(wchar));
	remote.scale = buff.Read<u16>();
	if(remote.parentUID == ActorUID::INVALID || remote.ownerUID == ActorUID::INVALID) return 0;
	if(buff.CanRead(sizeof(f32) + sizeof(u32) * 2 + sizeof(u16) * 4 + sizeof(u8) * 4)) {
		remote.lifeTime = buff.Read<f32>();
		buff.Read<u32>();
		remote.creator = buff.Read<u16>();
		remote.errorAngle[0] = buff.Read<u16>();
		remote.errorAngle[1] = buff.Read<u16>();
		remote.chargingLevel = buff.Read<u8>();
		remote.gaugeLevel = buff.Read<u8>();
		remote.firingCount = buff.Read<u16>();
		remote.comboCount = buff.Read<u8>();
		const LocalActorID shotTarget = buff.Read<LocalActorID>();
		remote.targetUID = shotTarget != LocalActorID::INVALID ? GetWorldActorUID(senderHd, shotTarget) : ActorUID::INVALID;
		remote.flags = buff.Read<u8>();
	}

	remote.fireRotation[2] = (u16)WorldYawToCentiDegrees(atan2f(target.y - clientFirePos.y, target.x - clientFirePos.x));
	remote.firePos = clientFirePos + vec3(0, 0, REMOTE_FIRE_HEIGHT);
	remote.targetPos = vec3(target.x, target.y, groundTarget ? target.z : remote.firePos.z);
	remote.doc = remoteDoc;
	SendRemoteCreate(remote, senderHd);
	return remote.creator;
}

void Replication::SendWeaponStateTo(ClientHandle clientHd, ActorUID ownerUID, i32 weaponID, i32 state, i32 result)
{
	const LocalActorID ownerLocal = GetLocalActorID(clientHd, ownerUID);
	if(ownerLocal == LocalActorID::INVALID) return;
	Sv::SN_WeaponState packet;
	packet.ownerID = ownerLocal;
	packet.weaponID = weaponID;
	packet.state = state;
	packet.chargeLevel = 0;
	packet.firingCombo = 0;
	packet.result = result;
	SendPacket(clientHd, packet);
}

void Replication::RelayWeaponState(ClientHandle senderHd, ActorUID ownerUID, i32 weaponID, i32 state, u8 chargeLevel, u8 firingCombo)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		if(clientHd == senderHd) continue;

		const LocalActorID ownerLocal = GetLocalActorID(clientHd, ownerUID);
		if(ownerLocal == LocalActorID::INVALID || playerLocalInfo[clientID].hiddenActors.count(ownerUID)) continue;

		Sv::SN_WeaponState packet;
		packet.ownerID = ownerLocal;
		packet.weaponID = weaponID;
		packet.state = state;
		packet.chargeLevel = chargeLevel;
		packet.firingCombo = firingCombo;
		packet.result = 0;
		SendPacket(clientHd, packet);
	}
}

void Replication::SendRemoteDamageToAll(const RemoteDamage& hit)
{
	const u32 seq = damageSeq++;
	const Remote* remote = GetGameXmlContent().FindRemote(hit.remoteDoc);
	const f32 fixedAngle = remote ? remote->forceFixedAngle * PI / 180.0f : 0.0f;
	const vec2 dir = fixedAngle != 0 ? vec2(hit.dir.x * cosf(fixedAngle) - hit.dir.y * sinf(fixedAngle), hit.dir.x * sinf(fixedAngle) + hit.dir.y * cosf(fixedAngle)) : vec2(hit.dir);
	const float3 dir3 = { dir.x, dir.y, 0 };
	const float3 back3 = { -dir.x, -dir.y, 0 };
	const float3 up3 = { 0, 0, 1 };
	const float3 remote3 = v2f(hit.remotePos);
	const float3 target3 = v2f(hit.targetPos);
	const float3 hitDir3 = remote && remote->maxSpeed > 0 ? back3 : up3;
	const u32 skillField = remote && remote->skillIndex != 0 ? (u32)remote->skillIndex : 0xFFFFFFFF;

	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;

		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID targetLocalID = GetLocalActorID(clientHd, hit.targetUID);
		if(targetLocalID == LocalActorID::INVALID) continue;
		if(playerLocalInfo[clientID].hiddenActors.count(hit.targetUID)) continue;

		PacketWriter<Sv::SN_BroadcastDamage,256> packet;
		packet.Write<u32>(seq);
		packet.Write<u32>(clientHd == hit.seedOwnerHd ? hit.remoteID - 1 : hit.remoteID);
		packet.Write<u32>((u32)targetLocalID);
		packet.Write<u32>((u32)GetLocalActorID(clientHd, hit.attackerUID));
		packet.Write<u32>((u32)hit.remoteDoc);
		packet.Write<float3>(remote3);
		packet.Write<float3>(back3);
		packet.Write<float3>(dir3);
		packet.Write<float3>(target3);
		packet.Write<float3>(hitDir3);
		packet.Write<u32>(hit.damageType);
		packet.Write<u32>(skillField);
		packet.Write<u32>(0);
		packet.Write<u32>(hit.elementRelation);
		packet.Write<u16>((u16)(sizeof(MEASURED_DAMAGE_HIT_NODE) - 1));
		packet.WriteRaw(MEASURED_DAMAGE_HIT_NODE, sizeof(MEASURED_DAMAGE_HIT_NODE) - 1);
		packet.Write<u32>((u32)hit.damage);
		packet.Write<u16>(0);
		packet.Write<u32>(0);
		packet.Write<u32>((u32)hit.damage);
		packet.Write<u32>(0);
		SendPacket(clientHd, packet);
	}

	SendHpToAll(hit.targetUID, hit.hp, hit.hpMax);
}

void Replication::RelayRemoteSyncCreateFromRemoteDoc(ClientHandle senderHd, const u8* packetData, i32 packetSize)
{
	ConstBuffer buff(packetData, packetSize);
	RemoteCreate remote;
	remote.parentUID = GetWorldActorUID(senderHd, buff.Read<LocalActorID>());
	remote.ownerUID = GetWorldActorUID(senderHd, buff.Read<LocalActorID>());
	remote.seed = buff.Read<u32>();
	remote.firePos = f2v(buff.Read<float3>());
	remote.fireRotation[0] = buff.Read<u16>();
	remote.fireRotation[1] = buff.Read<u16>();
	remote.fireRotation[2] = buff.Read<u16>();
	remote.targetPos = f2v(buff.Read<float3>());
	remote.fireObjectLen = buff.Read<u16>();
	if(remote.fireObjectLen > MAX_FIRE_OBJECT_LEN) return;
	remote.fireObject = (const wchar*)buff.ReadRaw(remote.fireObjectLen * sizeof(wchar));
	remote.scale = buff.Read<u16>();
	remote.lifeTime = buff.Read<f32>();
	remote.hitInvalidTarget = buff.Read<u32>();
	remote.doc = (RemoteIdx)buff.Read<u32>();
	const LocalActorID targetLocal = buff.Read<LocalActorID>();
	remote.targetUID = targetLocal != LocalActorID::INVALID ? GetWorldActorUID(senderHd, targetLocal) : ActorUID::INVALID;
	remote.flags = buff.Read<u8>();
	if(remote.parentUID == ActorUID::INVALID || remote.ownerUID == ActorUID::INVALID) return;
	SendRemoteCreate(remote, senderHd);
}
static const u8 REMOTE_SYNC_NO_MOVE_DIR = 8;
static const f32 MEASURED_SYNC_TURN_MIN_RAD = 0.087f;

void Replication::SendRemoteSyncTargetToAll(u32 remoteID, ActorUID targetUID, const vec3& pos, const vec2* moveDir, f32 moveDirZ)
{
	Sv::SN_RemoteSyncTarget packet;
	packet.excludedFieldBits = moveDir ? 0 : REMOTE_SYNC_NO_MOVE_DIR;
	packet.remoteID = remoteID;
	packet.DestPos = v2f(pos);
	const f32 flat = sqrtf(MAX(1.0f - moveDirZ * moveDirZ, 0.0f));
	packet.MoveDir = moveDir ? v2f(vec3(*moveDir * flat, moveDirZ)) : v2f(vec3(0));
	const u16 size = moveDir ? (u16)sizeof(Sv::SN_RemoteSyncTarget) : (u16)offsetof(Sv::SN_RemoteSyncTarget, MoveDir);
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID targetLocal = targetUID != ActorUID::INVALID ? GetLocalActorID(clientHd, targetUID) : LocalActorID::INVALID;
		packet.targetEntity = targetLocal != LocalActorID::INVALID ? (u32)targetLocal : 0;
		SendPacketData<Sv::SN_RemoteSyncTarget>(clientHd, size, &packet);
	}
}

void Replication::RelayRemoteSyncTarget(ClientHandle senderHd, const u8* packetData, i32 packetSize)
{
	const Cl::CQ_RemoteSyncTarget& sync = *(const Cl::CQ_RemoteSyncTarget*)packetData;
	const bool hasMoveDir = !(sync.excludedFieldBits & 8);
	const u16 relaySize = hasMoveDir ? sizeof(Sv::SN_RemoteSyncTarget) : offsetof(Sv::SN_RemoteSyncTarget, MoveDir);
	const ActorUID targetUID = sync.targetEntity != 0 ? GetWorldActorUID(senderHd, (LocalActorID)sync.targetEntity) : ActorUID::INVALID;

	Sv::SN_RemoteSyncTarget packet;
	packet.excludedFieldBits = sync.excludedFieldBits;
	packet.remoteID = sync.remoteID;
	packet.DestPos = sync.DestPos;
	packet.MoveDir = hasMoveDir ? sync.MoveDir : float3{ 0, 0, 0 };
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		if(clientHd == senderHd) continue;

		packet.targetEntity = targetUID != ActorUID::INVALID ? (u32)GetLocalActorID(clientHd, targetUID) : 0;
		SendPacketData<Sv::SN_RemoteSyncTarget>(clientHd, relaySize, &packet);
	}
}
void Replication::SendRemoteCreateToAll(ActorUID ownerUID, u32 seed, RemoteIdx doc, const vec3& firePos, f32 worldYaw, const vec3& targetPos, ActorUID targetUID, const wchar* fireObject, ClientHandle exclude)
{
	if(!frameCur->skillCastList.empty() || !frameCur->skillExecList.empty()) SendFrameSkillPackets();
	if(!fireObject || !fireObject[0]) fireObject = FIRE_OBJECT_SCENE_ROOT;
	RemoteCreate remote;
	remote.parentUID = ownerUID;
	remote.ownerUID = ownerUID;
	remote.seed = seed;
	remote.firePos = firePos;
	remote.fireRotation[2] = (u16)WorldYawToCentiDegrees(worldYaw);
	remote.targetPos = targetPos;
	remote.fireObject = fireObject;
	remote.fireObjectLen = (u16)EA::StdC::Strlen(fireObject);
	remote.doc = doc;
	remote.targetUID = targetUID;
	SendRemoteCreate(remote, exclude);
}
void Replication::SendChildRemoteCreateToAll(ActorUID ownerUID, ActorUID parentUID, u32 parentRemoteID, ClientHandle parentSeedOwnerHd, u32 seed, RemoteIdx doc, u16 creator, const vec3& firePos, f32 worldYaw, const vec3& targetPos, ActorUID targetUID, ActorUID hitInvalidUID, const wchar* fireObject, u8 flags, ClientHandle exclude)
{
	if(!fireObject || !fireObject[0]) fireObject = FIRE_OBJECT_SCENE_ROOT;
	RemoteCreate remote;
	remote.parentUID = parentUID;
	remote.ownerUID = ownerUID;
	remote.parentRemoteID = parentRemoteID;
	remote.parentSeedOwnerHd = parentSeedOwnerHd;
	remote.seed = seed;
	remote.firePos = firePos;
	remote.fireRotation[2] = (u16)WorldYawToCentiDegrees(worldYaw);
	remote.targetPos = targetPos;
	remote.fireObject = fireObject;
	remote.fireObjectLen = (u16)EA::StdC::Strlen(fireObject);
	remote.doc = doc;
	remote.creator = creator;
	remote.targetUID = targetUID;
	remote.hitInvalidUID = hitInvalidUID;
	remote.flags = flags;
	SendRemoteCreate(remote, exclude);
}

void Replication::SendWeaponCreatorShotToAll(ActorUID ownerUID, u32 seed, RemoteIdx doc, const vec3& firePos, f32 worldYaw, const vec3& targetPos, const wchar* fireObject, u16 creator, u8 chargingLevel, u8 comboCount, u16 firingCount)
{
	RemoteCreate remote;
	remote.parentUID = ownerUID;
	remote.ownerUID = ownerUID;
	remote.seed = seed;
	remote.firePos = firePos;
	remote.fireRotation[2] = (u16)WorldYawToCentiDegrees(worldYaw);
	remote.targetPos = targetPos;
	remote.fireObject = fireObject;
	remote.fireObjectLen = (u16)EA::StdC::Strlen(fireObject);
	remote.doc = doc;
	remote.creator = creator;
	remote.chargingLevel = chargingLevel;
	remote.comboCount = comboCount;
	remote.firingCount = firingCount;
	SendRemoteCreate(remote, ClientHandle::INVALID);
}
void Replication::SendAiActionStateToAll(ActorUID actorUID, ActionStateID action, f32 worldYaw, const vec3& pos, bool keepSpeed, f32 speed)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME || playerLocalInfo[clientID].hiddenActors.count(actorUID)) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID localID = GetLocalActorID(clientHd, actorUID);
		if(localID == LocalActorID::INVALID) continue;
		PacketWriter<Sv::SN_AiSetActionState,32> packet;
		packet.Write<u8>(keepSpeed ? NPC_STATE_KEEP_SPEED : 0);
		packet.Write<LocalActorID>(localID);
		packet.Write<i16>((i16)action);
		packet.Write<i16>(WorldYawToMonsterDeciDegrees(worldYaw));
		if(!keepSpeed) packet.Write<u16>((u16)MIN(speed * NPC_SPEED_SCALE, 65535.0f));
		packet.Write(pos);
		SendPacket(clientHd, packet);
	}
}

void Replication::SendAiSetTargetToAll(ActorUID actorUID, ActorUID targetUID)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID localID = GetLocalActorID(clientHd, actorUID);
		if(localID == LocalActorID::INVALID) continue;
		Sv::SN_AiSetTarget packet;
		packet.entityID = (u32)localID;
		const LocalActorID targetLocal = targetUID != ActorUID::INVALID ? GetLocalActorID(clientHd, targetUID) : LocalActorID::INVALID;
		packet.targetEntityID = targetLocal != LocalActorID::INVALID ? (u32)targetLocal : 0;
		SendPacketData<Sv::SN_AiSetTarget>(clientHd, sizeof(packet), &packet);
	}
}

void Replication::SendAiMoveTargetToAll(ActorUID actorUID, const vec3& goal, u32 movePreset)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID localID = GetLocalActorID(clientHd, actorUID);
		if(localID == LocalActorID::INVALID) continue;
		Sv::SN_AiSyncMonsterSkillTarget target;
		target.entityID = (u32)localID;
		target.targetID = 0;
		target.targetPos = v2f(goal);
		SendPacket(clientHd, target);
		Sv::SN_AiSetMovetargetpos move;
		move.entityID = (u32)localID;
		move.movePresetType = movePreset;
		move.moveTargetPos = v2f(goal);
		SendPacket(clientHd, move);
	}
}

void Replication::SendNpcMoveToAll(ActorUID actorUID, const vec3& pos, const vec3& goal, f32 worldYaw, f32 speed)
{
	const i16 yaw = WorldYawToMonsterDeciDegrees(worldYaw);
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME || playerLocalInfo[clientID].hiddenActors.count(actorUID)) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID localID = GetLocalActorID(clientHd, actorUID);
		if(localID == LocalActorID::INVALID) continue;
		PacketWriter<Sv::SN_AIMonsterSyncMove,48> packet;
		packet.Write<u8>(NPC_MOVE_YAW_AND_SPEED);
		packet.Write<u32>((u32)localID);
		packet.Write<i16>(yaw);
		packet.Write<i16>(yaw);
		packet.Write<u16>((u16)MIN(speed * 5.0f, 65535.0f));
		packet.Write<float3>(v2f(pos));
		packet.Write<float3>(v2f(goal));
		packet.Write<u8>(vec2(pos) == vec2(goal) ? NPC_MOVE_FLAG_STOPPED : 0);
		SendPacket(clientHd, packet);
	}
}

void Replication::SendMonsterStateToAll(ActorUID actorUID, ActionStateID action, const vec3& pos, f32 speed)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME || playerLocalInfo[clientID].hiddenActors.count(actorUID)) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID localID = GetLocalActorID(clientHd, actorUID);
		if(localID == LocalActorID::INVALID) continue;
		PacketWriter<Sv::SN_AiSetActionState,32> packet;
		packet.Write<u8>(MONSTER_STATE_NO_YAW);
		packet.Write<LocalActorID>(localID);
		packet.Write<i16>((i16)action);
		const u16 stateSpeed = (u16)MIN(speed * MONSTER_SPEED_SCALE, 65535.0f);
		packet.Write<u16>(stateSpeed);
		if(stateSpeed > 0) playerLocalInfo[clientID].monsterSpeedSent[actorUID] = stateSpeed;
		else playerLocalInfo[clientID].monsterSpeedSent.erase(actorUID);
		packet.Write(pos);
		SendPacket(clientHd, packet);
	}
}

void Replication::SendLevelIngameToAll(UserID userID, i32 expToLevelUp, i32 level)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		Sv::SN_LevelIngame packet;
		packet.usn = (u32)userID;
		packet.expForLevelUp = (u16)expToLevelUp;
		packet.currentLevel = (u8)level;
		SendPacketData<Sv::SN_LevelIngame>(clientHandle[clientID], sizeof(packet), &packet);
	}
}

void Replication::SendExpIngame(ClientHandle clientHd, UserID userID, i32 gain, i32 current, ActorUID target)
{
	Sv::SN_ExpIngame packet;
	packet.usn = (u32)userID;
	packet.acquisitionExp = (u16)gain;
	packet.currentExp = (u16)current;
	const LocalActorID local = target != ActorUID::INVALID ? GetLocalActorID(clientHd, target) : LocalActorID::INVALID;
	packet.target = local != LocalActorID::INVALID ? (u32)local : 0;
	SendPacketData<Sv::SN_ExpIngame>(clientHd, sizeof(packet), &packet);
}

void Replication::SendCharacterPropertyInfoToAll(ActorUID character, const eastl::array<u8,4>& levels)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		const LocalActorID local = GetLocalActorID(clientHandle[clientID], character);
		if(local == LocalActorID::INVALID) continue;
		PacketWriter<Sv::SN_CharacterPropertyInfo,32> packet;
		packet.Write<u32>((u32)local);
		u16 count = 0;
		for(u8 v : levels) if(v) count++;
		packet.Write<u16>(count);
		for(u8 i = 0; i < 4; i++) {
			if(!levels[i]) continue;
			packet.Write<u8>(i);
			packet.Write<u8>(levels[i]);
		}
		SendPacket(clientHandle[clientID], packet);
	}
}

void Replication::SendRequestVoting(ClientHandle clientHd, UserID requestor, u32 type, u32 periodMs, i32 voters, bool modify)
{
	Sv::SN_RequestVoting packet;
	packet.requestorUsn = (u32)requestor;
	packet.votingType = type;
	packet.period = periodMs;
	packet.voterCount = (u32)voters;
	packet.isModify = modify ? 1 : 0;
	SendPacket(clientHd, packet);
}

void Replication::SendVote(ClientHandle clientHd, u32 type, u32 choice)
{
	Sv::SN_Vote packet;
	packet.votingType = type;
	packet.voteType = choice;
	SendPacket(clientHd, packet);
}

void Replication::SendVotingResult(ClientHandle clientHd, UserID candidate, u32 type, u32 state, u32 reason, u32 playerCool, u32 teamCool)
{
	Sv::SN_VotingResult packet;
	packet.candidateUsn = (u32)candidate;
	packet.votingType = type;
	packet.votingStateType = state;
	packet.votingResultReasonType = reason;
	packet.playerCoolTime = playerCool;
	packet.teamCoolTime = teamCool;
	SendPacket(clientHd, packet);
}

void Replication::SendMonsterRespawnTimeToAll(u32 stringKey, u32 localID, u32 remainMs)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		Sv::SN_MonsterRespawnTime packet;
		packet.stringKey = stringKey;
		packet.localId = localID;
		packet.remainTime = remainMs;
		SendPacket(clientHandle[clientID], packet);
	}
}

void Replication::SendTransformAnswer(ClientHandle clientHd, ActorUID charUID, i32 doc)
{
	Sv::SA_TransformCharacter packet;
	packet.result = 0;
	packet.characterID = (u32)GetLocalActorID(clientHd, charUID);
	packet.docIndex = (u32)doc;
	packet.coolTime = 0;
	SendPacket(clientHd, packet);
}

void Replication::SendTitanFormToAll(bool modeChange, ActorUID charUID, i32 doc, u32 durationMs, i32 weapon, i32 skill1, i32 skill2, i32 ug, const StatValue* stats, u16 count, f32 curHp, f32 curStamina, f32 curUlt, f32 moveSpeed)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		auto write = [&](auto& packet) {
			packet.Write<u32>((u32)GetLocalActorID(clientHd, charUID));
			packet.Write<i32>(doc);
			if(!modeChange) packet.Write<u32>(durationMs);
			packet.Write<i32>(weapon);
			packet.Write<i32>(skill1);
			packet.Write<i32>(skill2);
			packet.Write<i32>(ug);
			packet.Write<u16>(count);
			for(u16 i = 0; i < count; i++) {
				packet.Write<u8>(stats[i].type);
				packet.Write<f32>(stats[i].value);
			}
			packet.Write<u16>(3);
			packet.Write<u8>((u8)StatType::STAT_TYPE_HEALTH);
			packet.Write<f32>(curHp);
			packet.Write<u8>((u8)StatType::STAT_TYPE_ULTIMATE);
			packet.Write<f32>(curUlt);
			packet.Write<u8>((u8)StatType::STAT_TYPE_STAMINA);
			packet.Write<f32>(curStamina);
			packet.Write<f32>(moveSpeed);
			packet.Write<u8>(1);
			SendPacket(clientHd, packet);
		};
		if(modeChange) { PacketWriter<Sv::SN_ChangeCharacterMode,512> packet; write(packet); }
		else { PacketWriter<Sv::SN_TransformCharacter,512> packet; write(packet); }
	}
}

void Replication::SendPvpAnnouncementRaw(ClientHandle clientHd, u32 type, u32 data, u32 score, u32 attacker)
{
	Sv::SN_PvpEventAnnouncement packet;
	packet.stAnnounceMent.announcementType = type;
	packet.stAnnounceMent.announcementData = data;
	packet.stAnnounceMent.score = score;
	packet.stAnnounceMent.attackerID = attacker;
	packet.stAnnounceMent.attackeeID = 0;
	packet.stAnnounceMent.isAllDead = 0;
	SendPacket(clientHd, packet);
}

void Replication::SendSafeZone(ClientHandle clientHd, UserID userID, bool inside)
{
	Sv::SN_NotifyIsInSafeZone packet;
	packet.userID = (i32)userID;
	packet.inSafeZone = inside ? 1 : 0;
	SendPacket(clientHd, packet);
}

void Replication::SendTraitAnswer(ClientHandle clientHd, bool reset, u32 error)
{
	if(reset) {
		Sv::SA_CharacterPropertyReset packet;
		packet.errorType = error;
		SendPacket(clientHd, packet);
		return;
	}
	Sv::SA_CharacterPropertyUpgrade packet;
	packet.errorType = error;
	SendPacket(clientHd, packet);
}

void Replication::SendCharacterPropertyResetToAll(ActorUID character)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		const LocalActorID local = GetLocalActorID(clientHandle[clientID], character);
		if(local == LocalActorID::INVALID) continue;
		Sv::SN_CharacterPropertyReset packet;
		packet.characterId = (u32)local;
		SendPacketData<Sv::SN_CharacterPropertyReset>(clientHandle[clientID], sizeof(packet), &packet);
	}
}

void Replication::SendCharacterPropertyPoint(ClientHandle clientHd, ActorUID character, i32 points)
{
	const LocalActorID local = GetLocalActorID(clientHd, character);
	if(local == LocalActorID::INVALID) return;
	Sv::SN_CharacterPropertyPoint packet;
	packet.characterId = (u32)local;
	packet.propertyPoint = (u8)points;
	SendPacketData<Sv::SN_CharacterPropertyPoint>(clientHd, sizeof(packet), &packet);
}

void Replication::SendMonsterMoveToAll(ActorUID actorUID, const vec3& pos, const vec3& goal, f32 worldYaw, f32 speed, bool beforeAction)
{
	const i16 yaw = WorldYawToMonsterDeciDegrees(worldYaw);
	NoteNpcMoveFlag(actorUID, beforeAction && vec2(pos) == vec2(goal), yaw);
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME || playerLocalInfo[clientID].hiddenActors.count(actorUID)) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID localID = GetLocalActorID(clientHd, actorUID);
		if(localID == LocalActorID::INVALID) continue;
		const bool stopped = vec2(pos) == vec2(goal);
		const u16 speedSent = (u16)MIN(speed * MONSTER_SPEED_SCALE, 65535.0f);
		auto& told = playerLocalInfo[clientID].monsterSpeedSent;
		auto known = told.find(actorUID);
		const bool withSpeed = !stopped && (known == told.end() || known->second != speedSent);
		if(withSpeed) told[actorUID] = speedSent;
		PacketWriter<Sv::SN_AIMonsterSyncMove,48> packet;
		packet.Write<u8>(withSpeed ? NPC_MOVE_YAW_AND_SPEED : NPC_MOVE_YAW_AND_POSITIONS);
		packet.Write<u32>((u32)localID);
		packet.Write<i16>(yaw);
		packet.Write<i16>(yaw);
		if(withSpeed) packet.Write<u16>(speedSent);
		packet.Write<float3>(v2f(pos));
		packet.Write<float3>(v2f(goal));
		packet.Write<u8>(stopped ? (beforeAction ? MEASURED_NPC_MOVE_FLAG_ACTION_STOP : NPC_MOVE_FLAG_STOPPED) : 0);
		SendPacket(clientHd, packet);
	}
}

void Replication::SendMonsterStopWithSpeedToAll(ActorUID actorUID, const vec3& pos, f32 speed)
{
	NoteNpcMoveFlag(actorUID, false, 0);
	static const u8 NPC_MOVE_SPEED_AND_POSITIONS = 0x1e;
	const u16 speedSent = (u16)MIN(speed * MONSTER_SPEED_SCALE, 65535.0f);
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME || playerLocalInfo[clientID].hiddenActors.count(actorUID)) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID localID = GetLocalActorID(clientHd, actorUID);
		if(localID == LocalActorID::INVALID) continue;
		playerLocalInfo[clientID].monsterSpeedSent[actorUID] = speedSent;
		PacketWriter<Sv::SN_AIMonsterSyncMove,48> packet;
		packet.Write<u8>(NPC_MOVE_SPEED_AND_POSITIONS);
		packet.Write<u32>((u32)localID);
		packet.Write<u16>(speedSent);
		packet.Write<float3>(v2f(pos));
		packet.Write<float3>(v2f(pos));
		packet.Write<u8>(NPC_MOVE_FLAG_STOPPED);
		SendPacket(clientHd, packet);
	}
}

void Replication::SendMonsterStopTurnToAll(ActorUID actorUID, const vec3& pos, f32 fromYaw, f32 toYaw, f32 speed)
{
	const u16 speedSent = (u16)MIN(speed * MONSTER_SPEED_SCALE, 65535.0f);
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME || playerLocalInfo[clientID].hiddenActors.count(actorUID)) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID localID = GetLocalActorID(clientHd, actorUID);
		if(localID == LocalActorID::INVALID) continue;
		playerLocalInfo[clientID].monsterSpeedSent[actorUID] = speedSent;
		PacketWriter<Sv::SN_AIMonsterSyncMove,48> packet;
		packet.Write<u8>(NPC_MOVE_YAW_AND_SPEED);
		packet.Write<u32>((u32)localID);
		packet.Write<i16>(WorldYawToMonsterDeciDegrees(fromYaw));
		packet.Write<i16>(WorldYawToMonsterDeciDegrees(toYaw));
		packet.Write<u16>(speedSent);
		packet.Write<float3>(v2f(pos));
		packet.Write<float3>(v2f(pos));
		packet.Write<u8>(NPC_MOVE_FLAG_STOPPED);
		SendPacket(clientHd, packet);
	}
}

void Replication::NoteNpcMoveFlag(ActorUID actorUID, bool actionStop, i16 yaw)
{
	if(actionStop) npcActionStopYaw[actorUID] = yaw;
	else npcActionStopYaw.erase(actorUID);
}

void Replication::SendMonsterLiftToAll(ActorUID actorUID, const vec3& pos)
{
	auto pending = npcActionStopYaw.find(actorUID);
	const bool clearActionStop = pending != npcActionStopYaw.end();
	const i16 pendingYaw = clearActionStop ? pending->second : 0;
	if(clearActionStop) npcActionStopYaw.erase(pending);
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME || playerLocalInfo[clientID].hiddenActors.count(actorUID)) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID localID = GetLocalActorID(clientHd, actorUID);
		if(localID == LocalActorID::INVALID) continue;
		if(clearActionStop) {
			PacketWriter<Sv::SN_AIMonsterSyncMove,48> stop;
			stop.Write<u8>(NPC_MOVE_YAW_AND_POSITIONS);
			stop.Write<u32>((u32)localID);
			stop.Write<i16>(pendingYaw);
			stop.Write<i16>(pendingYaw);
			stop.Write<float3>(v2f(pos));
			stop.Write<float3>(v2f(pos));
			stop.Write<u8>(NPC_MOVE_FLAG_STOPPED);
			SendPacket(clientHd, stop);
		}
		PacketWriter<Sv::SN_AIMonsterSyncMove,48> packet;
		packet.Write<u8>(NPC_MOVE_POSITIONS_ONLY);
		packet.Write<u32>((u32)localID);
		packet.Write<float3>(v2f(pos));
		packet.Write<float3>(v2f(pos));
		packet.Write<u8>(NPC_MOVE_FLAG_STOPPED);
		SendPacket(clientHd, packet);
	}
}

void Replication::SendNpcTurnToAll(ActorUID actorUID, f32 worldYaw)
{
	const i16 yaw = WorldYawToMonsterDeciDegrees(worldYaw);
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME || playerLocalInfo[clientID].hiddenActors.count(actorUID)) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID localID = GetLocalActorID(clientHd, actorUID);
		if(localID == LocalActorID::INVALID) continue;
		PacketWriter<Sv::SN_AIMonsterSyncTurn,32> packet;
		packet.Write<u8>(0x12);
		packet.Write<u32>((u32)localID);
		packet.Write<i16>(yaw);
		packet.Write<i16>(yaw);
		SendPacket(clientHd, packet);
	}
}

void Replication::SendNpcLegStartToAll(ActorUID actorUID, const vec3& pos, const vec3& goal, f32 speed)
{
	NoteNpcMoveFlag(actorUID, false, 0);
	static const u8 NPC_MOVE_SPEED_AND_POSITIONS = 0x1e;
	const u16 speedSent = (u16)MIN(MAX(speed, 0.0f) * MONSTER_SPEED_SCALE, 65535.0f);
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME || playerLocalInfo[clientID].hiddenActors.count(actorUID)) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID localID = GetLocalActorID(clientHd, actorUID);
		if(localID == LocalActorID::INVALID) continue;
		auto& told = playerLocalInfo[clientID].monsterSpeedSent;
		auto known = told.find(actorUID);
		const bool withSpeed = speedSent > 0 && (known == told.end() || known->second != speedSent);
		if(withSpeed) told[actorUID] = speedSent;
		PacketWriter<Sv::SN_AIMonsterSyncMove,48> packet;
		packet.Write<u8>(withSpeed ? NPC_MOVE_SPEED_AND_POSITIONS : NPC_MOVE_POSITIONS_ONLY);
		packet.Write<u32>((u32)localID);
		if(withSpeed) packet.Write<u16>(speedSent);
		packet.Write<float3>(v2f(pos));
		packet.Write<float3>(v2f(goal));
		packet.Write<u8>(withSpeed ? 0 : NPC_MOVE_FLAG_MOVING);
		SendPacket(clientHd, packet);
	}
}

void Replication::SendNpcLegStopToAll(ActorUID actorUID, const vec3& pos, f32 worldYaw, bool beforeAction)
{
	const i16 yaw = WorldYawToMonsterDeciDegrees(worldYaw);
	NoteNpcMoveFlag(actorUID, beforeAction, yaw);
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME || playerLocalInfo[clientID].hiddenActors.count(actorUID)) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID localID = GetLocalActorID(clientHd, actorUID);
		if(localID == LocalActorID::INVALID) continue;
		PacketWriter<Sv::SN_AIMonsterSyncMove,48> packet;
		packet.Write<u8>(NPC_MOVE_YAW_AND_POSITIONS);
		packet.Write<u32>((u32)localID);
		packet.Write<i16>(yaw);
		packet.Write<i16>(yaw);
		packet.Write<float3>(v2f(pos));
		packet.Write<float3>(v2f(pos));
		packet.Write<u8>(beforeAction ? MEASURED_NPC_MOVE_FLAG_ACTION_STOP : NPC_MOVE_FLAG_STOPPED);
		SendPacket(clientHd, packet);
	}
}

void Replication::SendRemoteActivatedToAll(u32 remoteID, ClientHandle seedOwnerHd, ClientHandle skipHd, ActorUID targetUID, u32 penetration, const vec3& pos)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		if(clientHd == skipHd) continue;
		const LocalActorID targetLocal = GetLocalActorID(clientHd, targetUID);
		if(targetLocal == LocalActorID::INVALID) continue;
		PacketWriter<Sv::SN_RemoteActivated,32> packet;
		packet.Write<u32>(clientHd == seedOwnerHd ? remoteID - 1 : remoteID);
		packet.Write<u32>((u32)targetLocal);
		packet.Write<u32>(penetration);
		packet.Write<float3>(v2f(pos));
		SendPacket(clientHd, packet);
	}
}

void Replication::SendResistStatusToAll(ActorUID targetUID, ActorUID casterUID, u32 event)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID targetLocal = GetLocalActorID(clientHd, targetUID);
		if(targetLocal == LocalActorID::INVALID) continue;
		const LocalActorID casterLocal = GetLocalActorID(clientHd, casterUID);
		PacketWriter<Sv::SN_ResistStatus,16> packet;
		packet.Write<u32>((u32)targetLocal);
		packet.Write<u32>(casterLocal != LocalActorID::INVALID ? (u32)casterLocal : 0);
		packet.Write<u32>(event);
		SendPacket(clientHd, packet);
	}
}

void Replication::SendMasterGroupingEffect(ClientHandle clientHd, ActorUID masterUID, i32 index, ClassType classType, bool activate)
{
	const LocalActorID localID = GetLocalActorID(clientHd, masterUID);
	if(localID == LocalActorID::INVALID) return;
	Sv::SN_UpdateMasterGroupingEffect packet;
	packet.instanceID = localID;
	packet.index = index;
	packet.classType = classType;
	packet.activate = activate ? 1 : 0;
	SendPacket(clientHd, packet);
}

void Replication::SendBattleStateToAll(ActorUID actorUID, bool inBattle, f32 baseMoveSpeed)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID localID = GetLocalActorID(clientHd, actorUID);
		if(localID == LocalActorID::INVALID || playerLocalInfo[clientID].hiddenActors.count(actorUID)) continue;
		PacketWriter<Sv::SN_ChangeBattleState,16> packet;
		packet.Write<u32>((u32)localID);
		packet.Write<u8>(inBattle ? 1 : 0);
		packet.Write<f32>(baseMoveSpeed);
		SendPacket(clientHd, packet);
		playerLocalInfo[clientID].monsterSpeedSent.erase(actorUID);
	}
}

void Replication::SendTargetGraphMoveToAll(ActorUID casterUID, const vec3& casterPos, ActorUID targetUID, const vec3& targetPos, ActionStateID actionID)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID casterLocal = GetLocalActorID(clientHd, casterUID);
		if(casterLocal == LocalActorID::INVALID) continue;
		const LocalActorID targetLocal = GetLocalActorID(clientHd, targetUID);
		PacketWriter<Sv::SN_UpdateTargetGraphMove,48> packet;
		packet.Write<u32>((u32)casterLocal);
		packet.Write<float3>(v2f(casterPos));
		packet.Write<u32>(targetLocal != LocalActorID::INVALID ? (u32)targetLocal : 0);
		packet.Write<float3>(v2f(targetPos));
		packet.Write<u32>((u32)actionID);
		SendPacket(clientHd, packet);
	}
}

void Replication::SendEvadeToAll(ActorUID attackerUID, ActorUID defenderUID, u32 remoteID, RemoteIdx remoteDoc, ClientHandle seedOwnerHd)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID defLocal = GetLocalActorID(clientHd, defenderUID);
		if(defLocal == LocalActorID::INVALID) continue;
		const LocalActorID attLocal = GetLocalActorID(clientHd, attackerUID);
		PacketWriter<Sv::SN_BroadcastEvade,32> packet;
		packet.Write<u32>(attLocal != LocalActorID::INVALID ? (u32)attLocal : 0);
		packet.Write<u32>((u32)defLocal);
		packet.Write<u32>(remoteID != 0 && clientHd == seedOwnerHd ? remoteID - 1 : remoteID);
		packet.Write<u32>((u32)remoteDoc);
		SendPacket(clientHd, packet);
	}
}

void Replication::SendModifyStatusToAll(i32 statusID, ActorUID targetUID, ActorUID casterUID, u8 overlap, u8 customValue, ActorUID replaceCasterUID)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID targetLocal = GetLocalActorID(clientHd, targetUID);
		if(targetLocal == LocalActorID::INVALID) continue;
		const LocalActorID casterLocal = GetCasterLocalID(clientHd, casterUID);
		PacketWriter<Sv::SN_ModifyStatus,32> packet;
		packet.Write<u32>((u32)statusID);
		packet.Write<u8>(1);
		packet.Write<u32>((u32)targetLocal);
		packet.Write<u32>(casterLocal != LocalActorID::INVALID ? (u32)casterLocal : 0);
		const LocalActorID replaceLocal = replaceCasterUID != ActorUID::INVALID ? GetLocalActorID(clientHd, replaceCasterUID) : LocalActorID::INVALID;
		packet.Write<u32>(replaceLocal != LocalActorID::INVALID ? (u32)replaceLocal : 0);
		packet.Write<u8>(overlap);
		packet.Write<u8>(customValue);
		packet.Write<u8>(0);
		SendPacket(clientHd, packet);
	}
}

void Replication::SendDestroyRemoteToAll(u32 remoteID)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		Sv::SN_DestroyEntity packet;
		packet.objectID = (LocalActorID)remoteID;
		SendPacket(clientHandle[clientID], packet);
	}
}

void Replication::SendAttackActionToOthers(ClientHandle senderHd, ActorUID actorUID, f32 mxmYaw, ActionStateID state)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		if(clientHd == senderHd) continue;

		const LocalActorID localID = GetLocalActorID(clientHd, actorUID);
		if(localID == LocalActorID::INVALID || playerLocalInfo[clientID].hiddenActors.count(actorUID)) continue;

		Sv::SN_PlayerSyncActionStateOnly packet;
		memset(&packet, 0, sizeof(packet));
		packet.characterID = localID;
		packet.state = state;
		packet.param1 = 0;
		packet.param2 = 0;
		packet.rotate = mxmYaw;
		packet.upperRotate = mxmYaw;
		packet.graphMove.bApply = 0;
		SendPacket(clientHd, packet);
	}
}

void Replication::SendActionStateOnlyToAll(ActorUID actorUID, ActionStateID state, f32 mxmYaw, bool move, const vec3& startPos, const vec3& endPos, f32 durationSec)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID localID = GetLocalActorID(clientHd, actorUID);
		if(localID == LocalActorID::INVALID || playerLocalInfo[clientID].hiddenActors.count(actorUID)) continue;

		Sv::SN_PlayerSyncActionStateOnly packet;
		memset(&packet, 0, sizeof(packet));
		packet.characterID = localID;
		packet.state = state;
		packet.rotate = mxmYaw;
		packet.upperRotate = mxmYaw;
		packet.graphMove.bApply = move ? 1 : 0;
		if(move) {
			packet.graphMove.startPos = v2f(startPos);
			packet.graphMove.endPos = v2f(endPos);
			packet.graphMove.durationTimeS = durationSec;
			packet.graphMove.originDistance = glm::distance(startPos, endPos);
		}
		SendPacket(clientHd, packet);
	}
}

void Replication::SendAddStatusToAll(i32 statusID, ActorUID targetUID, ActorUID casterUID, f32 durationSec, const vec3& from, const vec3& to, f32 graphDuration, i32 overlap, f32 scale, f32 nominalDist, bool withGraph, bool refresh, u8 customValue)
{
	if(!frameCur->skillCastList.empty() || !frameCur->skillExecList.empty()) SendFrameSkillPackets();
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID targetLocal = GetLocalActorID(clientHd, targetUID);
		if(targetLocal == LocalActorID::INVALID) continue;
		const LocalActorID casterLocal = GetCasterLocalID(clientHd, casterUID);

		const f32 dist = to == vec3(0) ? 0.0f : glm::length(vec2(to - from));
		PacketWriter<Sv::SN_AddStatus,128> packet;
		packet.Write<u8>(withGraph ? 0 : 1);
		if(withGraph) {
			packet.Write<f32>(scale);
			packet.Write<float3>(v2f(from));
			packet.Write<float3>(v2f(to));
			packet.Write<f32>(dist);
			packet.Write<f32>(nominalDist >= 0 ? nominalDist : dist);
			packet.Write<f32>(graphDuration);
		}
		packet.Write<u32>((u32)statusID);
		packet.Write<u8>(1);
		packet.Write<u32>((u32)targetLocal);
		packet.Write<u32>(casterLocal != LocalActorID::INVALID ? (u32)casterLocal : 0);
		packet.Write<u8>(refresh ? 1 : 0);
		packet.Write<u8>((u8)MAX(overlap, 1));
		packet.Write<u8>(customValue);
		packet.Write<f32>(durationSec);
		packet.Write<f32>(0);
		SendPacket(clientHd, packet);
	}
	if(linkMirror && casterUID != ActorUID::INVALID && casterUID != targetUID) {
		const StatusInfo* info = GetGameXmlContent().FindStatus(statusID);
		if(info && info->type == StatusType::LINK) {
			linkMirror = false;
			SendAddStatusToAll(statusID, casterUID, targetUID, durationSec, from, to, graphDuration, overlap, scale, nominalDist, false, refresh, customValue);
			linkMirror = true;
		}
	}
}

void Replication::SendRemoveStatusToAll(i32 statusID, ActorUID targetUID, ActorUID casterUID)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID targetLocal = GetLocalActorID(clientHd, targetUID);
		if(targetLocal == LocalActorID::INVALID) continue;
		const LocalActorID casterLocal = GetCasterLocalID(clientHd, casterUID);

		Sv::SN_RemoveStatus packet;
		packet.statusID = (u32)statusID;
		packet.targetID = (u32)targetLocal;
		packet.casterID = casterLocal != LocalActorID::INVALID ? (u32)casterLocal : 0;
		SendPacket(clientHd, packet);
	}
	if(linkMirror && casterUID != ActorUID::INVALID && casterUID != targetUID) {
		const StatusInfo* info = GetGameXmlContent().FindStatus(statusID);
		if(info && info->type == StatusType::LINK) {
			linkMirror = false;
			SendRemoveStatusToAll(statusID, casterUID, targetUID);
			linkMirror = true;
		}
	}
}

void Replication::SendBushCurStateToAll(ActorUID bushUID, u32 state)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		const LocalActorID local = GetLocalActorID(clientHandle[clientID], bushUID);
		if(local == LocalActorID::INVALID) continue;
		Sv::SN_BushState packet;
		packet.bushID = (u32)local;
		packet.curState = state;
		SendPacket(clientHandle[clientID], packet);
	}
}

void Replication::SendDespawnMonsterToAll(ActorUID actorUID)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID localID = GetLocalActorID(clientHd, actorUID);
		if(localID == LocalActorID::INVALID) continue;
		Sv::SN_DespawnMonster packet;
		packet.objectID = (u32)localID;
		SendPacket(clientHd, packet);
	}
}

void Replication::SendStatToAll(ActorUID actorUID, i32 statType, f32 cur, f32 max, u32 reason)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;

		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID localID = GetLocalActorID(clientHd, actorUID);
		if(localID == LocalActorID::INVALID || playerLocalInfo[clientID].hiddenActors.count(actorUID)) continue;

		Sv::SN_UpdateStat stat;
		stat.characterID = (u32)localID;
		stat._STAT_TYPE = (u32)statType;
		stat._CUR = cur;
		stat._MAX = max;
		stat._ReasonCode = reason;
		SendPacket(clientHd, stat);
	}
}

void Replication::SendCooltimeReset(ClientHandle clientHd, ActorUID characterUID, Slice<const i32> coolTimeIDs, u32 leftMs, u32 totalMs)
{
	PacketWriter<Sv::SN_NotifyCooltime,256> packet;
	packet.Write<u32>((u32)GetLocalActorID(clientHd, characterUID));
	packet.Write<u16>((u16)coolTimeIDs.size());
	foreach_const(id, coolTimeIDs) {
		packet.Write<u32>(1);
		packet.Write<u32>((u32)*id);
		packet.Write<u32>(leftMs);
		packet.Write<u32>(totalMs);
	}
	SendPacket(clientHd, packet);
}

void Replication::SendHideDetect(ClientHandle clientHd, ActorUID detectorUID, ActorUID targetUID, u8 flag)
{
	Sv::SN_HideDetect packet;
	packet.hideDetectorOwner = (u32)GetLocalActorID(clientHd, detectorUID);
	packet.targetID = (u32)GetLocalActorID(clientHd, targetUID);
	packet.flag = flag;
	SendPacket(clientHd, packet);
}

void Replication::SendUltReady(ClientHandle clientHd, ActorUID actorUID, bool ready)
{
	Sv::SN_UpdateCanCastSkillSlotUg packet;
	packet.playerID = (u32)GetLocalActorID(clientHd, actorUID);
	packet.canCastSkillSlotUG = ready ? 1 : 0;
	SendPacket(clientHd, packet);
}

void Replication::SendTagCooltime(ClientHandle clientHd, ActorUID actorUID, u32 ms)
{
	Sv::SN_UpdateGamePlayerTagCooltime packet;
	packet.playerID = (u32)GetLocalActorID(clientHd, actorUID);
	packet.tagCooltimeMS = ms;
	SendPacket(clientHd, packet);
}

void Replication::SendLightArea(ClientHandle clientHd, bool on, u32 cell, u8 team, u8 radius)
{
	if(on) {
		Sv::SN_LightOnArea packet;
		packet.cellArrayIndex = cell;
		packet.team = team;
		packet.radius = radius;
		SendPacket(clientHd, packet);
	}
	else {
		Sv::SN_LightOffArea packet;
		packet.cellArrayIndex = cell;
		packet.team = team;
		packet.radius = radius;
		SendPacket(clientHd, packet);
	}
}

void Replication::SendBushStateToAll(ActorUID actorUID, ActorUID bushUID)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME || playerLocalInfo[clientID].hiddenActors.count(actorUID)) continue;
		const LocalActorID local = GetLocalActorID(clientHandle[clientID], actorUID);
		if(local == LocalActorID::INVALID) continue;
		Sv::SN_BushObjectState packet;
		packet.objectID = (u32)local;
		packet.bushID = bushUID != ActorUID::INVALID ? (u32)GetLocalActorID(clientHandle[clientID], bushUID) : 0;
		SendPacket(clientHandle[clientID], packet);
	}
}

void Replication::SendEnterNpc(ClientHandle clientHd, ActorUID actorUID, CreatureIndex docID, const vec3& pos, f32 yaw, f32 speed, ActionStateID action, i32 hp, i32 hpMax, f32 growthLevel, const EnterGraph* graph)
{
	const LocalActorID localID = GetLocalActorID(clientHd, actorUID);
	if(localID == LocalActorID::INVALID) return;
	auto found = playerMap.find(clientHd);
	if(found != playerMap.end()) playerLocalInfo[found->second].hiddenActors.erase(actorUID);
	static const f32 GRAPH_DISTANCE_UNIT = 100.0f;
	PacketWriter<Sv::SN_GameEnterActor,512> packet;
	packet.Write<u8>(graph ? 0 : 1);
	if(graph) {
		packet.Write<u8>(1);
		packet.Write<f32>(graph->dist / GRAPH_DISTANCE_UNIT);
		packet.Write<f32>(graph->totalSec);
		packet.Write<f32>(graph->curSec);
		packet.Write<float3>(v2f(graph->start));
		packet.Write<float3>(v2f(graph->end));
		packet.Write<f32>(graph->originDist / GRAPH_DISTANCE_UNIT);
		packet.Write<u8>(0);
		packet.Write<float2>(float2{ 0, 0 });
		packet.Write<f32>(0);
		packet.Write<float2>(float2{ 0, 0 });
		packet.Write<f32>(0);
	}
	packet.Write<LocalActorID>(localID);
	packet.Write<float3>(v2f(pos));
	packet.Write<float3>(float3{ cosf(yaw), sinf(yaw), 0 });
	packet.Write<float2>(float2{ 0, 0 });
	packet.Write<float2>(float2{ cosf(yaw), sinf(yaw) });
	packet.Write<float3>(v2f(pos));
	packet.Write<u8>(graph ? 1 : 0);
	packet.Write<f32>(graph ? 0.0f : speed);
	packet.Write<ActionStateID>(action);
	packet.Write<i32>(0);
	const auto* stats = GetGameXmlContent().FindMonsterStats((i32)docID);
	for(int list = 0; list < 2; list++) {
		packet.Write<u16>(stats ? (u16)stats->size() : 0);
		if(!stats) continue;
		for(const auto& st : *stats) {
			packet.Write<u8>(st.type);
			const f32 value = st.value + GetGameXmlContent().MonsterGrowth((i32)docID, st.type) * growthLevel;
			packet.Write<f32>(list == 1 && st.type == (u8)StatType::STAT_TYPE_HEALTH ? (f32)hp : value);
		}
	}
	packet.Write<u16>(0);
	packet.Write<u16>(0);
	SendPacket(clientHd, packet);
	Sv::SN_UpdateStat stat;
	stat.characterID = (u32)localID;
	stat._STAT_TYPE = 0;
	stat._CUR = (f32)hp;
	stat._MAX = (f32)hpMax;
	stat._ReasonCode = 0;
	SendPacket(clientHd, stat);
}

void Replication::SendEnterDynamic(ClientHandle clientHd, ActorUID actorUID, const vec3& pos, f32 yaw, ActionStateID action, bool jumpWall, f32 curHp, f32 maxHp)
{
	const LocalActorID localID = GetLocalActorID(clientHd, actorUID);
	if(localID == LocalActorID::INVALID) return;
	auto found = playerMap.find(clientHd);
	if(found != playerMap.end()) playerLocalInfo[found->second].hiddenActors.erase(actorUID);
	PacketWriter<Sv::SN_GameEnterActor,512> packet;
	packet.Write<u8>(1);
	packet.Write<LocalActorID>(localID);
	packet.Write<float3>(v2f(pos));
	packet.Write<float3>(float3{ cosf(yaw), sinf(yaw), 0 });
	packet.Write<float2>(float2{ 0, 0 });
	packet.Write<float2>(float2{ 0, 0 });
	packet.Write<float3>(float3{ 0, 0, 0 });
	packet.Write<u8>(0);
	packet.Write<f32>(0);
	packet.Write<ActionStateID>(action);
	packet.Write<i32>(0);
	static const f32 MEASURED_DYNAMIC_ENTER_HEALTH_CUR = -1.0f;
	static const f32 MEASURED_DYNAMIC_ENTER_HEALTH_MAX = 1.0f;
	static const f32 MEASURED_DYNAMIC_ENTER_DEFENCE_MAX = 113.33f;
	static const f32 MEASURED_JUMPWALL_ENTER_ATTACK_MAX = 192.0f;
	const u8 health = (u8)StatType::STAT_TYPE_HEALTH;
	const u8 attack = (u8)StatType::STAT_TYPE_ATTACK;
	const u8 defence = (u8)StatType::STAT_TYPE_DEFENCE;
	packet.Write<u16>(jumpWall ? 3 : 2);
	packet.Write<u8>(health);
	packet.Write<f32>(jumpWall ? curHp : MEASURED_DYNAMIC_ENTER_HEALTH_CUR);
	if(jumpWall) {
		packet.Write<u8>(attack);
		packet.Write<f32>(0);
	}
	packet.Write<u8>(defence);
	packet.Write<f32>(0);
	packet.Write<u16>(jumpWall ? 3 : 2);
	packet.Write<u8>(health);
	packet.Write<f32>(jumpWall ? maxHp : MEASURED_DYNAMIC_ENTER_HEALTH_MAX);
	if(jumpWall) {
		packet.Write<u8>(attack);
		packet.Write<f32>(MEASURED_JUMPWALL_ENTER_ATTACK_MAX);
	}
	packet.Write<u8>(defence);
	packet.Write<f32>(MEASURED_DYNAMIC_ENTER_DEFENCE_MAX);
	packet.Write<u16>(0);
	packet.Write<u16>(0);
	SendPacket(clientHd, packet);
}

void Replication::SendLeaveActor(ClientHandle clientHd, ActorUID actorUID)
{
	const LocalActorID localID = GetLocalActorID(clientHd, actorUID);
	if(localID == LocalActorID::INVALID) return;
	Sv::SN_GameLeaveActor packet;
	packet.objectID = localID;
	SendPacket(clientHd, packet);
	auto found = playerMap.find(clientHd);
	if(found != playerMap.end()) {
		playerLocalInfo[found->second].hiddenActors.insert(actorUID);
		playerLocalInfo[found->second].monsterSpeedSent.erase(actorUID);
	}
}

void Replication::MarkHidden(ClientHandle clientHd, ActorUID actorUID)
{
	auto found = playerMap.find(clientHd);
	if(found != playerMap.end()) playerLocalInfo[found->second].hiddenActors.insert(actorUID);
}

void Replication::SendEnterActor(ClientHandle clientHd, ActorUID actorUID, ClassType classType, const vec3& pos, f32 bodyYaw, f32 upperYaw, const vec3& moveTo, bool battle, f32 speed, ActionStateID action, i32 hp, i32 hpMax)
{
	const LocalActorID localID = GetLocalActorID(clientHd, actorUID);
	if(localID == LocalActorID::INVALID) return;
	auto found = playerMap.find(clientHd);
	if(found != playerMap.end()) playerLocalInfo[found->second].hiddenActors.erase(actorUID);
	PacketWriter<Sv::SN_GameEnterActor,512> packet;
	packet.Write<u8>(1);
	packet.Write<LocalActorID>(localID);
	packet.Write<float3>(v2f(pos));
	packet.Write<float3>(float3{ cosf(bodyYaw), sinf(bodyYaw), 0 });
	const vec2 moving = vec2(moveTo) - vec2(pos);
	const vec2 moveDir = glm::length(moving) > 1.0f ? glm::normalize(moving) : vec2(0);
	packet.Write<float2>(float2{ moveDir.x, moveDir.y });
	packet.Write<float2>(float2{ cosf(upperYaw), sinf(upperYaw) });
	packet.Write<float3>(v2f(moveTo));
	packet.Write<u8>(battle ? 1 : 0);
	packet.Write<f32>(speed);
	packet.Write<ActionStateID>(action);
	packet.Write<i32>(0);
	const auto& baseStats = GetGameXmlContent().GetMaster(classType).baseStats;
	for(int list = 0; list < 2; list++) {
		packet.Write<u16>((u16)baseStats.size());
		foreach_const(st, baseStats) {
			packet.Write<u8>(st->type);
			packet.Write<f32>(list == 1 && st->type == (u8)StatType::STAT_TYPE_HEALTH ? (f32)hp : st->value);
		}
	}
	packet.Write<u16>(0);
	packet.Write<u16>(0);
	SendPacket(clientHd, packet);

	Sv::SN_UpdateStat stat;
	stat.characterID = (u32)localID;
	stat._STAT_TYPE = 0;
	stat._CUR = (f32)hp;
	stat._MAX = (f32)hpMax;
	stat._ReasonCode = 0;
	SendPacket(clientHd, stat);
}

void Replication::SendChangeSkillSlot(ClientHandle clientHd, ActorUID actorUID, i32 slot, SkillID skill, i32 resetCooltimeID)
{
	Sv::SN_ChangeSkillSlot packet;
	packet.entity = (u32)GetLocalActorID(clientHd, actorUID);
	packet.slotIndex = (u32)slot;
	packet.skillIndex = (u32)skill;
	packet.resetBaseComboCooltimeID = (u32)resetCooltimeID;
	packet.param = 0;
	SendPacket(clientHd, packet);
}

static const f32 CLIENT_SELF_SNAP_DIST = 180.0f;
static const i32 SYNC_STOP_STILL_FRAMES = 4;
static const f32 SYNC_TURN_COS = 0.985f;
static const f32 SYNC_HEARTBEAT_SEC = 0.67f;
static const f32 HYP_SYNC_DRIFT_RESEND = 100.0f;
static const f32 MEASURED_SYNC_SKILL_MOVE_MIN_SEC = 0.1f;

void Replication::SendTeleportRequest(ClientHandle clientHd, ActorUID actorUID, const vec3& pos)
{
	Sv::SQ_TeleportObject packet;
	packet.objectID = (u32)GetLocalActorID(clientHd, actorUID);
	packet.pos = v2f(pos);
	SendPacketData<Sv::SQ_TeleportObject>(clientHd, sizeof(packet), &packet);
}

void Replication::SendPlayerSyncTeleport(ClientHandle clientHd, ActorUID actorUID, const vec3& pos)
{
	const LocalActorID local = GetLocalActorID(clientHd, actorUID);
	if(local == LocalActorID::INVALID) return;
	Sv::SN_PlayerSyncTeleport packet;
	packet.objectID = (u32)local;
	packet.pos = v2f(pos);
	SendPacketData<Sv::SN_PlayerSyncTeleport>(clientHd, sizeof(packet), &packet);
}

void Replication::SendPlayerSyncTeleportToAll(ActorUID actorUID, const vec3& pos)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		const LocalActorID local = GetLocalActorID(clientHandle[clientID], actorUID);
		if(local == LocalActorID::INVALID) continue;
		Sv::SN_PlayerSyncTeleport packet;
		packet.objectID = (u32)local;
		packet.pos = v2f(pos);
		SendPacketData<Sv::SN_PlayerSyncTeleport>(clientHandle[clientID], sizeof(packet), &packet);
	}
}

void Replication::SendSelfTeleport(ClientHandle clientHd, ActorUID actorUID, const vec3& pos, f32 worldYaw, const vec3* clientPos)
{
	if(clientPos && glm::length(vec2(*clientPos) - vec2(pos)) <= CLIENT_SELF_SNAP_DIST + 40.0f) {
		const vec3 detour = pos + vec3(CLIENT_SELF_SNAP_DIST * 2.2f, 0, 0);
		SendSelfTeleport(clientHd, actorUID, detour, worldYaw, nullptr);
	}
	Sv::SN_PlayerSyncMove sync;
	sync.entityID = GetLocalActorID(clientHd, actorUID);
	sync.DestPos = v2f(pos);
	sync.MoveDir = { 0, 0 };
	sync.UpperDir = { WorldYawToMxmYaw(worldYaw), 0 };
	sync.nRotate = WorldYawToMxmYaw(worldYaw);
	sync.nSpeed = 0;
	sync.flags = 0;
	sync.actionStateID = ActionStateID::NONE_BEHAVIORSTATE;
	SendPacket(clientHd, sync);
}

void Replication::SendSelfMove(ClientHandle clientHd, ActorUID actorUID, const vec3& pos, const vec2& moveDir, f32 speed, f32 worldYaw)
{
	Sv::SN_PlayerSyncMove sync;
	sync.entityID = GetLocalActorID(clientHd, actorUID);
	sync.DestPos = v2f(pos);
	sync.MoveDir = v2f(moveDir);
	sync.UpperDir = { WorldYawToMxmYaw(worldYaw), 0 };
	sync.nRotate = WorldYawToMxmYaw(worldYaw);
	sync.nSpeed = speed;
	sync.flags = 0;
	sync.actionStateID = ActionStateID::NONE_BEHAVIORSTATE;
	SendPacket(clientHd, sync);
}

void Replication::RelayPlayerMove(ClientHandle fromHd, ActorUID actorUID, const Cl::CN_GameUpdatePosition& update)
{
	Sv::SN_PlayerSyncMove sync;
	sync.DestPos = update.p3nPos;
	sync.MoveDir = update.p3nDir;
	sync.UpperDir = { update.upperYaw, update.upperPitch };
	sync.nRotate = update.bodyYaw;
	sync.nSpeed = update.nSpeed;
	sync.flags = update.unk1;
	sync.actionStateID = update.actionState;
	for(int pi = 0; pi < MAX_PLAYERS; pi++) {
		if(playerState[pi].cur < PlayerState::IN_GAME) continue;
		const ClientHandle clientHd = clientHandle[pi];
		if(clientHd == fromHd || playerLocalInfo[pi].hiddenActors.count(actorUID)) continue;
		sync.entityID = GetLocalActorID(clientHd, actorUID);
		if(sync.entityID == LocalActorID::INVALID) continue;
		SendPacket(clientHd, sync);
	}
}

void Replication::RelayPlayerTurn(ClientHandle fromHd, ActorUID actorUID, const Cl::CN_GameUpdateRotation& update)
{
	Sv::SN_PlayerSyncTurn sync;
	sync.UpperDir = { update.upperYaw, update.upperPitch };
	sync.nRotate = update.bodyYaw;
	for(int pi = 0; pi < MAX_PLAYERS; pi++) {
		if(playerState[pi].cur < PlayerState::IN_GAME) continue;
		const ClientHandle clientHd = clientHandle[pi];
		if(clientHd == fromHd || playerLocalInfo[pi].hiddenActors.count(actorUID)) continue;
		sync.entityID = GetLocalActorID(clientHd, actorUID);
		if(sync.entityID == LocalActorID::INVALID) continue;
		SendPacket(clientHd, sync);
	}
}

void Replication::SendExecuteSkillAck(ClientHandle clientHd, ActorUID casterUID, SkillID skillID)
{
	Sv::SA_ExecuteSkill ack;
	ack.entity = (u32)GetLocalActorID(clientHd, casterUID);
	ack.ret = 0;
	ack.skillIndex = (u32)skillID;
	SendPacket(clientHd, ack);
}

void Replication::SendToggleSkill(ActorUID casterUID, ClientHandle casterHd, SkillID skillID, bool on, ActionStateID action)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID localID = GetLocalActorID(clientHd, casterUID);
		if(localID == LocalActorID::INVALID) continue;
		u8 buf[24] = {};
		u32 words[3] = { (u32)localID, 0, (u32)skillID };
		memcpy(buf, words, sizeof(words));
		if(clientHd == casterHd) {
			const u32 act = (u32)action;
			memcpy(buf + 12, &act, 4);
			if(on) SendPacketData<Sv::SA_SwitchOnToggleSkill>(clientHd, 16, buf);
			else SendPacketData<Sv::SA_SwitchOffToggleSkill>(clientHd, 16, buf);
		}
		else if(on) SendPacketData<Sv::SN_SwitchOnToggleSkill>(clientHd, 18, buf);
		else SendPacketData<Sv::SN_SwitchOffToggleSkill>(clientHd, 16, buf);
	}
}

void Replication::SendCancelSkillToAll(ActorUID casterUID, SkillID skillID, ClientHandle skipHd)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		if(clientHd == skipHd) continue;
		const LocalActorID localID = GetLocalActorID(clientHd, casterUID);
		if(localID == LocalActorID::INVALID) continue;
		Sv::SN_CancelSkill cancel;
		cancel.entity = (u32)localID;
		cancel.ret = 0;
		cancel.skillIndex = (u32)skillID;
		SendPacket(clientHd, cancel);
	}
}

void Replication::SendCastSkillRefused(ClientHandle clientHd, ActorUID casterUID, SkillID skillID, u32 ret, const vec3& casterPos, f32 angle, f32 speed, f32 clientTime)
{
	const LocalActorID localActorID = GetLocalActorID(clientHd, casterUID);
	if(localActorID == LocalActorID::INVALID) return;
	Sv::SA_CastSkill refuse;
	refuse.entity = localActorID;
	refuse.ret = ret;
	refuse.skillIndex = skillID;
	SendPacket(clientHd, refuse);

	PacketWriter<Sv::SN_CastSkill,512> packet;
	packet.Write<LocalActorID>(localActorID);
	packet.Write<i32>((i32)ret);
	packet.Write<SkillID>(skillID);
	packet.Write<i8>(0);
	packet.Write<ActionStateID>(ActionStateID::INVALID);
	packet.Write<float3>(v2f(vec3(0)));
	packet.Write<u16>(0);
	packet.Write<u8>(1);
	packet.Write<float3>(v2f(casterPos));
	packet.Write<float3>(v2f(casterPos));
	packet.Write<float2>(v2f(vec2(cosf(angle), sinf(angle))));
	packet.Write<RotationHumanoid>(RotConvertToMxm(RotationHumanoid{ angle, 0, angle }));
	packet.Write<f32>(speed);
	packet.Write<f32>(clientTime);
	SendPacket(clientHd, packet);
}

void Replication::SendInteractionCastingToAll(ActorUID casterUID, ActorUID objectUID, u32 actionState, u32 castingTimeMS)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID casterID = GetLocalActorID(clientHd, casterUID);
		const LocalActorID objectID = GetLocalActorID(clientHd, objectUID);
		if(casterID == LocalActorID::INVALID || objectID == LocalActorID::INVALID) continue;
		Sv::SN_InteractionCasting packet;
		packet.objectID = (u32)casterID;
		packet.targetID = (u32)objectID;
		packet.actionState = actionState;
		packet.castingTimeMS = castingTimeMS;
		SendPacket(clientHd, packet);
	}
}

void Replication::SendInteractionExecuteOnlyToAll(ActorUID casterUID, ActorUID objectUID, u32 actionState)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID casterID = GetLocalActorID(clientHd, casterUID);
		const LocalActorID objectID = GetLocalActorID(clientHd, objectUID);
		if(casterID == LocalActorID::INVALID || objectID == LocalActorID::INVALID) continue;
		Sv::SN_InteractionExecute packet;
		packet.objectID = (u32)casterID;
		packet.targetID = (u32)objectID;
		packet.actionState = actionState;
		SendPacket(clientHd, packet);
	}
}

void Replication::SendInteractionCancelToAll(ActorUID casterUID, ActorUID objectUID)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID casterID = GetLocalActorID(clientHd, casterUID);
		const LocalActorID objectID = GetLocalActorID(clientHd, objectUID);
		if(casterID == LocalActorID::INVALID || objectID == LocalActorID::INVALID) continue;
		Sv::SN_InteractionCancel packet;
		packet.objectID = (u32)casterID;
		packet.targetID = (u32)objectID;
		SendPacket(clientHd, packet);
	}
}

void Replication::SendInteractionStatusToAll(ActorUID objectUID, u32 allowedTeamMask, i32 interactionID, i32 occupiedTeam, ClientHandle exclude, f32 coolTime)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		if(clientHd == exclude && exclude != ClientHandle::INVALID) continue;
		const LocalActorID localID = GetLocalActorID(clientHd, objectUID);
		if(localID == LocalActorID::INVALID) continue;
		Sv::SN_InteractionStatus packet;
		packet.allowedTeamMask = allowedTeamMask;
		packet.objectID = (u32)localID;
		packet.interactionID = (u32)interactionID;
		packet.occupiedTeam = occupiedTeam;
		packet.coolTime = coolTime;
		SendPacket(clientHd, packet);
	}
}

void Replication::SendInteractionExecuteToAll(ActorUID objectUID, ActorUID targetUID, u32 actionState, u32 castingTimeMS)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID objectID = GetLocalActorID(clientHd, objectUID);
		const LocalActorID targetID = GetLocalActorID(clientHd, targetUID);
		if(objectID == LocalActorID::INVALID || targetID == LocalActorID::INVALID) continue;
		Sv::SN_InteractionCasting casting;
		casting.objectID = (u32)objectID;
		casting.targetID = (u32)targetID;
		casting.actionState = actionState;
		casting.castingTimeMS = castingTimeMS;
		SendPacket(clientHd, casting);
		Sv::SN_InteractionExecute execute;
		execute.objectID = (u32)objectID;
		execute.targetID = (u32)targetID;
		execute.actionState = actionState;
		SendPacket(clientHd, execute);
	}
}

void Replication::SendNpcBehaviorToAll(ActorUID actorUID, ActionStateID action, const vec3& pos, const vec3& targetPos, ActorUID targetUID, f32 moveSpeed)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME || playerLocalInfo[clientID].hiddenActors.count(actorUID)) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID localID = GetLocalActorID(clientHd, actorUID);
		if(localID == LocalActorID::INVALID) continue;
		const LocalActorID targetLocal = targetUID != ActorUID::INVALID ? GetLocalActorID(clientHd, targetUID) : LocalActorID::INVALID;
		if(targetLocal != LocalActorID::INVALID) {
			Sv::SN_AiSetTarget target;
			target.entityID = (u32)localID;
			target.targetEntityID = (u32)targetLocal;
			SendPacketData<Sv::SN_AiSetTarget>(clientHd, sizeof(target), &target);
		}
		else if(glm::length(vec2(targetPos) - vec2(pos)) > 1.0f) {
			Sv::SN_AiSyncMonsterSkillTarget target;
			target.entityID = (u32)localID;
			target.targetID = 0;
			target.targetPos = v2f(targetPos);
			SendPacket(clientHd, target);
		}
		PacketWriter<Sv::SN_AiSetActionState,32> packet;
		packet.Write<u8>(0x04);
		packet.Write<u32>((u32)localID);
		packet.Write<i16>((i16)action);
		packet.Write<u16>((u16)MIN(moveSpeed * 5.0f, 65535.0f));
		packet.Write<float3>(v2f(pos));
		SendPacket(clientHd, packet);
	}
}

void Replication::SendStanceToAll(ActorUID actorUID, i32 stance, f32 durationSec)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;

		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID localID = GetLocalActorID(clientHd, actorUID);
		if(localID == LocalActorID::INVALID) continue;

		Sv::SN_SyncStanceType packet;
		packet.entityID = (u32)localID;
		packet.StanceType = (u32)stance;
		packet.durationtimeSec = durationSec;
		SendPacket(clientHd, packet);
	}
}

void Replication::SendHpToAll(ActorUID actorUID, i32 hp, i32 hpMax)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME || playerLocalInfo[clientID].hiddenActors.count(actorUID)) continue;

		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID localID = GetLocalActorID(clientHd, actorUID);
		if(localID == LocalActorID::INVALID || playerLocalInfo[clientID].hiddenActors.count(actorUID)) continue;

		Sv::SN_UpdateStat stat;
		stat.characterID = (u32)localID;
		stat._STAT_TYPE = 0;
		stat._CUR = (f32)hp;
		stat._MAX = (f32)hpMax;
		stat._ReasonCode = 0;
		SendPacket(clientHd, stat);
	}
}

void Replication::SendHp(ClientHandle clientHd, ActorUID actorUID, i32 hp, i32 hpMax)
{
	const LocalActorID localID = GetLocalActorID(clientHd, actorUID);
	if(localID == LocalActorID::INVALID) return;
	Sv::SN_UpdateStat stat;
	stat.characterID = (u32)localID;
	stat._STAT_TYPE = 0;
	stat._CUR = (f32)hp;
	stat._MAX = (f32)hpMax;
	stat._ReasonCode = 0;
	SendPacket(clientHd, stat);
}

void Replication::SendNpcDeadAckToAll(ActorUID npcUID, ActorUID killerUID, u32 killerDoc)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID npcLocal = GetLocalActorID(clientHd, npcUID);
		if(npcLocal == LocalActorID::INVALID) continue;
		Sv::SN_DeadAck dead;
		dead.sourceID = (u32)npcLocal;
		dead.targetID = killerUID != ActorUID::INVALID ? (u32)GetLocalActorID(clientHd, killerUID) : 0;
		dead.targetDocIndex = killerDoc;
		dead.dwRemoteID = 0;
		dead.dwRemoteDocIndex = killerUID != ActorUID::INVALID ? 0 : 0xFFFFFFFF;
		dead.isPast = 0;
		SendPacket(clientHd, dead);
	}
}

void Replication::SendPointsToAll(UserID userID, u8 team, i32 points, i32 teamPoints)
{
	enum : u32 { SCORE = 0 };
	PacketWriter<Sv::SN_ScoreUpdate> playerPacket;
	playerPacket.Write<u32>((u32)userID);
	playerPacket.Write<TeamType>(TeamType::INVALID);
	playerPacket.Write<u16>(1);
	playerPacket.Write<u32>(SCORE); playerPacket.Write<u32>((u32)points);
	PacketWriter<Sv::SN_ScoreUpdate> teamPacket;
	teamPacket.Write<u32>(0);
	teamPacket.Write<TeamType>(team == 0 ? TeamType::RED : TeamType::BLUE);
	teamPacket.Write<u16>(1);
	teamPacket.Write<u32>(SCORE); teamPacket.Write<u32>((u32)teamPoints);
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		SendPacket(clientHandle[clientID], playerPacket);
		SendPacket(clientHandle[clientID], teamPacket);
	}
}

void Replication::SendMonsterKillCountsToAll(UserID userID, i32 minionKills, i32 neutralKills)
{
	enum : u32 { MINION_KILL_COUNT = 9, NEUTRAL_MONSTER_KILL_COUNT = 11 };
	PacketWriter<Sv::SN_ScoreUpdate> packet;
	packet.Write<u32>((u32)userID);
	packet.Write<TeamType>(TeamType::INVALID);
	packet.Write<u16>(2);
	packet.Write<u32>(MINION_KILL_COUNT); packet.Write<u32>((u32)minionKills);
	packet.Write<u32>(NEUTRAL_MONSTER_KILL_COUNT); packet.Write<u32>((u32)neutralKills);
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		SendPacket(clientHandle[clientID], packet);
	}
}

void Replication::SendTeamPointsToAll(u8 team, i32 teamPoints)
{
	enum : u32 { SCORE = 0 };
	PacketWriter<Sv::SN_ScoreUpdate> teamPacket;
	teamPacket.Write<u32>(0);
	teamPacket.Write<TeamType>(team == 0 ? TeamType::RED : TeamType::BLUE);
	teamPacket.Write<u16>(1);
	teamPacket.Write<u32>(SCORE); teamPacket.Write<u32>((u32)teamPoints);
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		SendPacket(clientHandle[clientID], teamPacket);
	}
}

void Replication::SendPvpAnnouncementToAll(u32 type, u32 data, ActorUID attackerUID, ActorUID victimUID, u32 score)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		const ClientHandle clientHd = clientHandle[clientID];
		Sv::SN_PvpEventAnnouncement packet;
		packet.stAnnounceMent.announcementType = type;
		packet.stAnnounceMent.announcementData = data;
		packet.stAnnounceMent.score = score;
		packet.stAnnounceMent.attackerID = (u32)GetLocalActorID(clientHd, attackerUID);
		packet.stAnnounceMent.attackeeID = (u32)GetLocalActorID(clientHd, victimUID);
		packet.stAnnounceMent.isAllDead = 0;
		SendPacket(clientHd, packet);
	}
}

void Replication::SendDeathToAll(ActorUID killerUID, ActorUID targetUID, CreatureIndex killerDocID, u32 remoteID, RemoteIdx remoteDoc, UserID targetUserID, i32 respawnDelayMs)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;

		const ClientHandle clientHd = clientHandle[clientID];
		const LocalActorID targetLocalID = GetLocalActorID(clientHd, targetUID);
		if(targetLocalID == LocalActorID::INVALID) continue;

		Sv::SN_DeadAck dead;
		dead.sourceID = (u32)targetLocalID;
		dead.targetID = (u32)GetLocalActorID(clientHd, killerUID);
		dead.targetDocIndex = (u32)killerDocID;
		dead.dwRemoteID = remoteID;
		dead.dwRemoteDocIndex = remoteDoc != RemoteIdx::INVALID ? (u32)remoteDoc : 0;
		dead.isPast = 0;
		SendPacket(clientHd, dead);

		Sv::SN_RespawnDelaytime delay;
		delay.usn = (u32)targetUserID;
		delay.respawnDelayTimeMS = (u32)respawnDelayMs;
		SendPacket(clientHd, delay);
	}
}

void Replication::SendReviveToAll(UserID userID, ActorUID activeUID, ActorUID inactiveUID, const vec3& pos)
{
	for(int clientID = 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
		SendRevive(clientHandle[clientID], userID, activeUID, inactiveUID, pos);
	}
}

void Replication::SendRevive(ClientHandle clientHd, UserID userID, ActorUID activeUID, ActorUID inactiveUID, const vec3& pos)
{
	Sv::SN_RevivePlayerAtStartingPoint revive;
	revive.usn = (u32)userID;
	revive.activeID = (u32)GetLocalActorID(clientHd, activeUID);
	revive.inactiveID = (u32)GetLocalActorID(clientHd, inactiveUID);
	revive.position = v2f(pos);
	SendPacket(clientHd, revive);
	auto found = playerMap.find(clientHd);
	if(found != playerMap.end()) playerLocalInfo[found->second].shownMain[MIN(activeUID, inactiveUID)] = activeUID;
}

void Replication::OnPlayerDisconnect(ClientHandle clientHd)
{
	auto found = playerMap.find(clientHd);
	if(found == playerMap.end()) return;
	const i32 clientID = found->second;
	playerState[clientID].cur = PlayerState::DISCONNECTED;
	clientHandle[clientID] = ClientHandle::INVALID;
}

void Replication::PlayerRegisterMasterActor(ClientHandle clientHd, ActorUID masterActorUID, ClassType classType)
{
	LocalActorID laiLeader = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)classType);
	ASSERT(laiLeader >= LocalActorID::FIRST_SELF_MASTER && laiLeader < LocalActorID::LAST_SELF_MASTER);

	const i32 clientID = playerMap.at(clientHd);
	PlayerForceLocalActorID(clientID, masterActorUID, laiLeader);
}

void Replication::PlayerForceLocalActorID(i32 clientID, ActorUID actorUID, LocalActorID localActorID)
{
	DBG_ASSERT(actorUID != ActorUID::INVALID);

	auto& map = playerLocalInfo[clientID].localActorIDMap;
	ASSERT(map.find(actorUID) == map.end());
	map.emplace(actorUID, localActorID);
}

LocalActorID Replication::GetLocalActorID(ClientHandle clientHd, ActorUID actorUID) const
{
	const i32 clientID = playerMap.at(clientHd);
	const auto& map = playerLocalInfo[clientID].localActorIDMap;
	auto found = map.find(actorUID);
	if(found != map.end()) {
		return found->second;
	}
	return LocalActorID::INVALID;
}

bool Replication::HiddenFor(ClientHandle clientHd, ActorUID actorUID) const
{
	return playerLocalInfo[playerMap.at(clientHd)].hiddenActors.count(actorUID) > 0;
}

ActorUID Replication::GetWorldActorUID(ClientHandle clientHd, LocalActorID localActorID) const
{
	ProfileFunction();

	// TODO: second map, for this reverse lookup
	const i32 clientID = playerMap.at(clientHd);
	const auto& map = playerLocalInfo[clientID].localActorIDMap;
	foreach(it, map) {
		if(it->second == localActorID) {
			return it->first;
		}
	}

	return ActorUID::INVALID;
}

void Replication::UpdatePlayersLocalState()
{
	for(int pi = 0; pi < MAX_PLAYERS; pi++) {
		if(playerState[pi].cur < PlayerState::IN_GAME) continue;

		PlayerLocalInfo& localInfo = playerLocalInfo[pi];
		const ClientHandle clientHd = clientHandle[pi];
		auto& playerActorUIDSet = localInfo.actorUIDSet; // replicated actors UID set

		eastl::fixed_vector<ActorUID,2048,true> removedList;
		eastl::fixed_vector<ActorUID,2048,true> addedList;
		eastl::set_difference(playerActorUIDSet.begin(), playerActorUIDSet.end(), frameCur->actorUIDSet.begin(), frameCur->actorUIDSet.end(), eastl::back_inserter(removedList));
		eastl::set_difference(frameCur->actorUIDSet.begin(), frameCur->actorUIDSet.end(), playerActorUIDSet.begin(), playerActorUIDSet.end(), eastl::back_inserter(addedList));

		// send destroy entity for deleted actors
		foreach(setIt, removedList) {
			const ActorUID actorUID = *setIt;

#ifdef CONF_DEBUG // we don't actually need to verify the actor was in the previous frame, but do it in debug mode anyway
			const auto actorIt = framePrev->actorUIDSet.find(actorUID);
			ASSERT(actorIt != framePrev->actorUIDSet.end());
			auto type = framePrev->actorType.find(actorUID);
			ASSERT(type != framePrev->actorType.end());
			switch(type->second) {
				case ActorType::Master: {
					auto pm = framePrev->masterMap.find(actorUID);
					ASSERT(pm != framePrev->masterMap.end());
					ASSERT(pm->second->actorUID == actorUID);
				} break;

				case ActorType::Npc: {
					auto pm = framePrev->npcMap.find(actorUID);
					ASSERT(pm != framePrev->npcMap.end());
					ASSERT(pm->second->actorUID == actorUID);
				} break;

				case ActorType::Dynamic: {
					auto pm = framePrev->dynamicMap.find(actorUID);
					ASSERT(pm != framePrev->dynamicMap.end());
					ASSERT(pm->second->actorUID == actorUID);
				} break;

				default: {
					ASSERT_MSG(0, "case not handled");
				} break;
			}
#endif

			const auto prevNpc = framePrev->npcMap.find(actorUID);
			const bool deadNpc = prevNpc != framePrev->npcMap.end() && (prevNpc->second->hp <= 0 || prevNpc->second->actionState == ActionStateID::DIE_BEHAVIORSTATE);
			const bool despawned = prevNpc != framePrev->npcMap.end() || framePrev->dynamicMap.find(actorUID) != framePrev->dynamicMap.end();
			if(!deadNpc) SendActorDestroy(clientHd, actorUID, despawned);

			// Remove LocalActorID link
			DeleteLocalActorID(pi, actorUID, deadNpc);
		}

		// send new spawns
		foreach(setIt, addedList) {
			const ActorUID actorUID = *setIt;

			// Create a LocalActorID link if none exists already
			// If one exists already, we have pre-allocated it (like with leader master)
			if(GetLocalActorID(clientHd, actorUID) == LocalActorID::INVALID) {
				CreateLocalActorID(pi, actorUID);
			}

			auto type = frameCur->actorType.find(actorUID);
			ASSERT(type != frameCur->actorType.end());

			switch(type->second) {
				case ActorType::Master: {
					const ActorMaster* chara = frameCur->FindMaster(actorUID);
					ASSERT(chara);
					const Player* parent = frameCur->FindPlayer(chara->playerIndex);
					ASSERT(parent);
					SendActorMasterSpawn(clientHd, *chara, *parent);
				} break;

				case ActorType::Npc: {
					const auto pm = frameCur->npcMap.find(actorUID);
					ASSERT(pm != frameCur->npcMap.end());
					ASSERT(pm->second->actorUID == actorUID);
					SendActorNpcSpawn(clientHd, *pm->second);
				} break;

				case ActorType::Dynamic: {
					const auto pm = frameCur->dynamicMap.find(actorUID);
					ASSERT(pm != frameCur->dynamicMap.end());
					ASSERT(pm->second->actorUID == actorUID);
					SendActorDynamicSpawn(clientHd, *pm->second);
				} break;

				default: {
					ASSERT_MSG(0, "case not handled");
				} break;
			}
		}

		playerActorUIDSet = frameCur->actorUIDSet;

		// TODO: remove, extra checks
#ifdef CONF_DEBUG
		auto& localActorIDMap = localInfo.localActorIDMap;
		foreach(it, playerActorUIDSet) {
			ASSERT(localActorIDMap.find(*it) != localActorIDMap.end());
		}
		foreach(it, localActorIDMap) {
			ASSERT(playerActorUIDSet.find(it->first) != playerActorUIDSet.end());
		}
		eastl::fixed_set<LocalActorID,2048> laiSet;
		foreach(it, localActorIDMap) {
			ASSERT(laiSet.find(it->second) == laiSet.end());
			laiSet.emplace(it->second);
		}
#endif
	}
}

void Replication::FrameDifference()
{
	foreach_const(it, frameCur->playerList) {
		const Player& cur = *it;
		const Player* found = framePrev->FindPlayer(cur.index);
		if(!found) continue;
		const Player& prev = *found;

		// tagging
		if(cur.mainCharaID != prev.mainCharaID) {
			const ActorUID upMainUID = cur.masters[cur.mainCharaID];
			const ActorUID upSubUID = cur.masters[cur.mainCharaID ^ 1];
			const ActorMaster* chara = frameCur->FindMaster(cur.masters[cur.mainCharaID]); // @Speed
			ASSERT(chara);

			Sv::SN_GamePlayerTag tag;
			tag.result = 128;
			tag.attackerID = LocalActorID::INVALID;

			for(int pi = 0; pi < MAX_PLAYERS; pi++) {
				if(playerState[pi].cur < PlayerState::IN_GAME) continue;
				const ClientHandle clientHd = clientHandle[pi];
				auto shown = playerLocalInfo[pi].shownMain.find(MIN(upMainUID, upSubUID));
				if(shown != playerLocalInfo[pi].shownMain.end() && shown->second == upMainUID) continue;

				tag.mainID = GetLocalActorID(clientHd, upMainUID);
				tag.subID = GetLocalActorID(clientHd, upSubUID);
				ASSERT(tag.mainID != LocalActorID::INVALID);
				ASSERT(tag.subID != LocalActorID::INVALID);

				SendPacket(clientHd, tag);
				playerLocalInfo[pi].shownMain[MIN(upMainUID, upSubUID)] = upMainUID;

				// Retail behavior (Titan Ruins capture): remote viewers receive ONLY the
				// SN_GamePlayerTag swap announcement - both masters already exist client-side.
				// Sending a destructive GameLeaveActor here removes the player's visible actor
				// without a valid re-bind, making tagged-out bots disappear.
			}
		}

		if(cur.hasJumped && !prev.hasJumped) {
			const ActorUID actorUID = cur.masters[cur.mainCharaID];
			const ActorMaster* chara = frameCur->FindMaster(cur.masters[cur.mainCharaID]);  // @Speed
			ASSERT(chara);
			const f32 rotate = WorldYawToMxmYaw(chara->rotation.bodyYaw);
			const vec2 moveDir = chara->moveDir;

			for(int pi = 0; pi < MAX_PLAYERS; pi++) {
				if(playerState[pi].cur < PlayerState::IN_GAME) continue;
				const ClientHandle clientHd = clientHandle[pi];
				if(playerLocalInfo[pi].hiddenActors.count(actorUID)) continue;

				PacketWriter<Sv::SA_ResultSpAction> packet;

				packet.Write<u8>(0x20); // excludedFieldBits
				packet.Write<i32>(0); // actionID
				packet.Write<LocalActorID>(GetLocalActorID(clientHd, actorUID));
				packet.Write<f32>(rotate);
				packet.Write<f32>(moveDir.x);
				packet.Write<f32>(moveDir.y);
				packet.Write<i32>(0); // errorType

				SendPacket(clientHd, packet);
			}
		}
	}

	if(!frameCur->skillCastList.empty() || !frameCur->skillExecList.empty()) SendFrameSkillPackets();

	// TODO: don't update everything here if tagged out (such as position)
	// find if the position has changed since last frame
	foreach_const(it, frameCur->masterList) {
		const ActorMaster& cur = *it;
		if(cur.taggedOut) continue;

		auto found = framePrev->masterMap.find(cur.actorUID);
		if(found == framePrev->masterMap.end()) continue; // previous not found, can't diff
		const ActorMaster& prev = *found->second;

		bool rotationUpdated = false;
		bool positionUpdated = false;

		// position
		const f32 posEpsilon = 0.5f;
		const f32 speedEpsilon = 0.001f;
		const bool moved = fabs(cur.pos.x - prev.pos.x) > posEpsilon || fabs(cur.pos.y - prev.pos.y) > posEpsilon || fabs(cur.pos.z - prev.pos.z) > posEpsilon;
		SentMove& sent = sentMoves[cur.actorUID];
		sent.stillFrames = moved ? 0 : sent.stillFrames + 1;
		const bool stopped = !moved && sent.stillFrames >= SYNC_STOP_STILL_FRAMES;
		const vec2 step = vec2(cur.pos - prev.pos);
		const f32 stepLen = glm::length(step);
		const vec2 realDir = stepLen > 0.01f ? step / stepLen : cur.moveDir;
		static const f64 MEASURE_WINDOW_SEC = 0.25;
		static const f32 HYP_BLOCKED_SPEED_RATIO = 0.35f;
		if(sent.probeAt == Time::ZERO) { sent.probeAt = TimeRelNow(); sent.probePos = cur.pos; }
		const f64 probeElapsed = TimeDiffSec(TimeDiff(sent.probeAt, TimeRelNow()));
		if(probeElapsed >= MEASURE_WINDOW_SEC) {
			sent.measuredSpeed = glm::length(vec2(cur.pos - sent.probePos)) / (f32)probeElapsed;
			sent.probePos = cur.pos;
			sent.probeAt = TimeRelNow();
		}
		const f32 nominalSpeed = cur.speed * MAX(glm::dot(cur.moveDir, realDir), 0.0f);
		const bool wasBlocked = sent.blocked;
		static const f32 MEASURED_SYNC_MIN_MOVING_SPEED = 200.0f;
		sent.blocked = cur.clientHd == ClientHandle::INVALID && !cur.skillMoving && moved && (nominalSpeed < MEASURED_SYNC_MIN_MOVING_SPEED || (sent.measuredSpeed >= 0.0f && sent.measuredSpeed < MAX(nominalSpeed * HYP_BLOCKED_SPEED_RATIO, MEASURED_SYNC_MIN_MOVING_SPEED)));
		const vec2 sendDir = stopped || sent.blocked ? vec2(0) : (moved ? realDir : sent.moveDir);
		const f32 sendSpeed = stopped || sent.blocked ? 0.0f : (moved ? nominalSpeed : sent.speed);
		const bool blockedChanged = sent.blocked != wasBlocked;
		const bool dirNow = glm::length(sendDir) > 0.01f;
		const bool dirSent = glm::length(sent.moveDir) > 0.01f;
		const bool turned = dirNow != dirSent || (dirNow && glm::dot(glm::normalize(sendDir), glm::normalize(sent.moveDir)) < SYNC_TURN_COS);
		const bool heartbeat = dirNow && TimeDiffSec(TimeDiff(sent.at, TimeRelNow())) >= SYNC_HEARTBEAT_SEC;
		const bool cadenceSend = Config().BotSyncCadence ? (turned || heartbeat || (prev.skillMoving && moved)) : (moved || turned);
		const f32 sinceSent = (f32)TimeDiffSec(TimeDiff(sent.at, TimeRelNow()));
		const bool skillSend = cur.skillMoving && (!prev.skillMoving || (moved && sinceSent >= MEASURED_SYNC_SKILL_MOVE_MIN_SEC));
		const bool landed = prev.statusGraph && !cur.statusGraph;
		const bool graphReleased = prev.graphHold && !cur.graphHold;
		const vec2 drawnGuess = vec2(sent.pos) + sent.moveDir * sent.speed * sinceSent;
		const bool drifted = cur.clientHd == ClientHandle::INVALID && !cur.skillMoving && glm::length(vec2(cur.pos) - drawnGuess) > HYP_SYNC_DRIFT_RESEND && sinceSent >= SYNC_HEARTBEAT_SEC;
		if(drifted && !positionUpdated && !cur.statusGraph && !(cur.graphHold && prev.graphHold)) LOG("Sync :: bot actor %u drifted %.0f from its last sync (%.2fs ago, blocked %d moved %d)", (u32)cur.actorUID, glm::length(vec2(cur.pos) - drawnGuess), sinceSent, sent.blocked ? 1 : 0, moved ? 1 : 0);
		if(cur.clientHd == ClientHandle::INVALID && !positionUpdated && !cur.statusGraph && !(cur.graphHold && prev.graphHold) &&
		   (drifted || (graphReleased || landed || cur.skillMoving ? (graphReleased || landed || skillSend || (stopped && glm::length(sent.moveDir) > 0.01f)) : (cadenceSend || blockedChanged || (!sent.blocked && fabs(sendSpeed - sent.speed) > speedEpsilon)))))
		{
			vec2 syncDir = sendDir;
			f32 syncSpeed = sendSpeed;
			if(cur.skillMoving) {
				const vec2 travel = vec2(cur.pos - sent.pos);
				const f32 travelLen = glm::length(travel);
				syncDir = !cur.graphMoving && prev.skillMoving && travelLen > posEpsilon ? travel / travelLen : vec2(0);
				syncSpeed = !cur.graphMoving && prev.skillMoving && sinceSent > 0 ? travelLen / sinceSent : 0.0f;
			}
			if(cur.graphMoving) {
				syncDir = vec2(0);
				syncSpeed = 0;
			}
			sent.moveDir = syncDir;
			sent.speed = syncSpeed;
			sent.at = TimeRelNow();
			sent.pos = cur.pos;
			sent.upperYaw = cur.rotation.upperYaw;
			sent.upperPitch = cur.rotation.upperPitch;
			sent.bodyYaw = cur.rotation.bodyYaw;
			ActionStateID action = cur.actionState;
			if(action == ActionStateID::INVALID) {
				action = ActionStateID::NONE_BEHAVIORSTATE;
			}

			Sv::SN_PlayerSyncMove sync;
			sync.DestPos = v2f(cur.pos);
			sync.MoveDir = v2f(syncDir);
			sync.UpperDir = { WorldYawToMxmYaw(cur.rotation.upperYaw), WorldPitchToMxmPitch(cur.rotation.upperPitch) };
			sync.nRotate = WorldYawToMxmYaw(cur.rotation.bodyYaw);
			sync.nSpeed = syncSpeed;
			sync.flags = 0;
			sync.actionStateID = action;

			for(int pi = 0; pi < MAX_PLAYERS; pi++) {
				if(playerState[pi].cur < PlayerState::IN_GAME) continue;
				const ClientHandle clientHd = clientHandle[pi];
				if(clientHd == cur.clientHd) continue; // ignore self
				if(playerLocalInfo[pi].hiddenActors.count(cur.actorUID)) continue;
				sync.entityID = GetLocalActorID(clientHd, cur.actorUID);
				SendPacket(clientHd, sync);
			}

			rotationUpdated = true;
		}

		// rotation
		SentMove& sentTurn = sentMoves[cur.actorUID];
		const auto angleGap = [](f32 a, f32 b) { f32 d = fmodf(a - b + PI, 2 * PI); if(d < 0) d += 2 * PI; return fabsf(d - PI); };
		const f32 turnGap = MAX(MAX(angleGap(cur.rotation.upperYaw, sentTurn.upperYaw), angleGap(cur.rotation.bodyYaw, sentTurn.bodyYaw)), fabsf(cur.rotation.upperPitch - sentTurn.upperPitch));
		if(cur.clientHd == ClientHandle::INVALID && !rotationUpdated && !cur.statusGraph && turnGap >= MEASURED_SYNC_TURN_MIN_RAD)
		{
			sentTurn.upperYaw = cur.rotation.upperYaw;
			sentTurn.upperPitch = cur.rotation.upperPitch;
			sentTurn.bodyYaw = cur.rotation.bodyYaw;
			Sv::SN_PlayerSyncTurn sync;
			sync.UpperDir = { WorldYawToMxmYaw(cur.rotation.upperYaw), WorldPitchToMxmPitch(cur.rotation.upperPitch) };
			sync.nRotate = WorldYawToMxmYaw(cur.rotation.bodyYaw);

			for(int pi = 0; pi < MAX_PLAYERS; pi++) {
				if(playerState[pi].cur < PlayerState::IN_GAME) continue;
				const ClientHandle clientHd = clientHandle[pi];
				if(clientHd == cur.clientHd) continue; // ignore self
				if(playerLocalInfo[pi].hiddenActors.count(cur.actorUID)) continue;

				sync.entityID = GetLocalActorID(clientHd, cur.actorUID);
				SendPacket(clientHd, sync);
			}
		}
	}


	// diff dynamics
	foreach_const(it, frameCur->dynamicList) {
		const ActorDynamic& cur = *it;
		const ActorDynamic* found = framePrev->FindDynamic(cur.actorUID);
		if(!found) continue;
		const ActorDynamic& prev = *found;

		// change action
		if(cur.action != prev.action) {
			ClientList list;
			GetPlayersInGame(&list);
			foreach_const(clientHd, list) {
				LocalActorID localActorID = GetLocalActorID(*clientHd, cur.actorUID);

				PacketWriter<Sv::SN_ActionChangeLevelEvent,64> packet;
				packet.WriteVec<LocalActorID>(&localActorID, 1);
				packet.Write(cur.action); // action
				packet.Write<i64>((i64)TimeDiffMs(TimeRelNow())); // serverTime

				SendPacket(*clientHd, packet);
			}
		}
	}

	SendFrameSkillPackets();
}

void Replication::SendFrameSkillPackets()
{
	// skill casts
	foreach_const(it, frameCur->skillCastList) {
		const auto& cast = *it;

		if(cast.clientHd != ClientHandle::INVALID) {
			LocalActorID localActorID = GetLocalActorID(cast.clientHd, cast.casterUID);

			// SA_CastSkill
			{
				Sv::SA_CastSkill accept;
				accept.entity = localActorID;
				accept.ret = 0;
				accept.skillIndex = cast.skillID;

				SendPacket(cast.clientHd, accept);
			}
		}

		for(int pi = 0; pi < MAX_PLAYERS; pi++) {
			if(playerState[pi].cur < PlayerState::IN_GAME) continue;
			const ClientHandle clientHd = clientHandle[pi];

			if(playerLocalInfo[pi].hiddenActors.count(cast.casterUID)) continue;
			LocalActorID localCasterID = GetLocalActorID(clientHd, cast.casterUID);

			// SN_CastSkill
			{
				PacketWriter<Sv::SN_CastSkill,512> packet;

				packet.Write<LocalActorID>(localCasterID); // entityID
				packet.Write<i32>(0); // ret
				packet.Write<SkillID>(cast.skillID);
				packet.Write<i8>(cast.costLevel);
				packet.Write<ActionStateID>(cast.sendAction ? cast.actionID : ActionStateID::INVALID);
				packet.Write<float3>(v2f(cast.castPos));

				packet.Write<u16>(cast.targetList.size()); // targetList_count
				foreach_const(t, cast.targetList) {
					packet.Write<LocalActorID>(GetLocalActorID(clientHd, *t));
				}

				packet.Write<u8>(1); // bSyncMyPosition
				packet.Write<float3>(v2f(cast.casterPos));
				packet.Write<float3>(v2f(cast.casterPos));
				packet.Write<float2>(v2f(cast.casterMoveDir));
				packet.Write<RotationHumanoid>(RotConvertToMxm(cast.casterRot));
				packet.Write<f32>(cast.casterSpeed);
				packet.Write<f32>(cast.clientTime);

				SendPacket(clientHd, packet);
			}
		}
	}

	foreach_const(it, frameCur->skillExecList) {
		const auto& exec = *it;

		for(int pi = 0; pi < MAX_PLAYERS; pi++) {
			if(playerState[pi].cur < PlayerState::IN_GAME) continue;
			const ClientHandle clientHd = clientHandle[pi];

			if(playerLocalInfo[pi].hiddenActors.count(exec.casterUID)) continue;
			LocalActorID localCasterID = GetLocalActorID(clientHd, exec.casterUID);

			// SN_ExecuteSkill
			{
				PacketWriter<Sv::SN_ExecuteSkill,512> packet;

				packet.Write<LocalActorID>(localCasterID); // entityID
				packet.Write<i32>(0); // ret
				packet.Write<SkillID>(exec.skillID);
				packet.Write<i8>(exec.costLevel);
				packet.Write<ActionStateID>(exec.actionID);
				packet.Write<float3>(v2f(exec.castPos));

				packet.Write<u16>(exec.targetList.size()); // targetList_count
				foreach_const(t, exec.targetList) {
					packet.Write<LocalActorID>(GetLocalActorID(clientHd, *t));
				}

				packet.Write<u8>(0); // bSyncMyPosition
				packet.Write<float3>({});
				packet.Write<float3>({});
				packet.Write<float2>({});
				packet.Write<RotationHumanoid>({});
				packet.Write<f32>(0);
				packet.Write<f32>(0.f);

				packet.Write<f32>(0); // fSkillChargeDamageMultiplier

				if(exec.moveDuration != 0) {
					// graphMove
					packet.Write<u8>(1); // bApply
					packet.Write<float3>(v2f(exec.startPos)); // startPos
					packet.Write<float3>(v2f(exec.endPos)); // endPos
					packet.Write<f32>(exec.moveDuration); // durationTimeS
					packet.Write<f32>(glm::distance(exec.startPos, exec.endPos)); // originDistance
				}
				else {
					// graphMove
					packet.Write<u8>(0); // bApply
					packet.Write<float3>(float3()); // startPos
					packet.Write<float3>(float3()); // endPos
					packet.Write<f32>(0); // durationTimeS
					packet.Write<f32>(0); // originDistance
				}

				SendPacket(clientHd, packet);
			}
		}
	}
	frameCur->skillCastList.clear();
	frameCur->skillExecList.clear();
}

void Replication::SendActorMasterSpawn(ClientHandle clientHd, const ActorMaster& actor, const Player& parent)
{
	DBG_ASSERT(actor.actorUID != ActorUID::INVALID);

	const LocalActorID localActorID = GetLocalActorID(clientHd, actor.actorUID);
	ASSERT(localActorID != LocalActorID::INVALID);

	LOG("[client%03d] Replication :: SendActorSpawn :: actorUID=%u localActorID=%u", clientHd, (u32)actor.actorUID, (u32)localActorID);

	// this is the main actor
	if(actor.actorUID == parent.masters[0]) {
		// SN_GameCreateActor
		{
			PacketWriter<Sv::SN_GameCreateActor,512> packet;

			packet.Write<LocalActorID>(localActorID); // objectID
			packet.Write<EntityType>(EntityType::CREATURE); // nType
			packet.Write<CreatureIndex>((CreatureIndex)(100000000 + (i32)actor.classType)); // nIDX
			packet.Write<i32>(-1);
			packet.Write(actor.pos);
			packet.Write(WorldYawToMxmYaw(actor.rotation.upperYaw));
			packet.Write(WorldPitchToMxmPitch(actor.rotation.upperPitch));
			packet.Write(WorldYawToMxmYaw(actor.rotation.bodyYaw));
			packet.Write<i32>(actor.spawnAnim);
			packet.Write<ActionStateID>(actor.actionState);
			packet.Write<i32>(actor.ownerID);
			packet.Write<u8>(actor.dirToNearPC);
			packet.Write<i32>(actor.wanderDist);
			packet.Write<i32>(actor.tagID);

			packet.Write<i32>(PlayerTeamType(parent.team));
			packet.Write<ClassType>(actor.classType); // classType
			packet.Write<SkinIndex>(actor.skinIndex); // skinIndex
			packet.Write<i32>(actor.seed);


			if(actor.initStatCount > 0) WriteInitStat(packet, actor.initStats, actor.initStatCount);
			else WriteInitStat(packet, GetGameXmlContent().GetMaster(actor.classType).baseStats);





			packet.Write<u8>(1); // isInSight
			packet.Write<u8>(0); // isDead
			packet.Write<i64>((i64)TimeDiffMs(TimeRelNow())); // serverTime

			packet.Write<u16>(0); // meshChangeActionHistory_count

			SendPacket(clientHd, packet);
		}
	}
	// this is the sub actor
	else {
		const LocalActorID parentLocalActorID = GetLocalActorID(clientHd, parent.masters[0]);
		ASSERT(parentLocalActorID != LocalActorID::INVALID);

		// SN_GameCreateSubActor
		{
			PacketWriter<Sv::SN_GameCreateSubActor,512> packet;

			packet.Write<LocalActorID>(localActorID); // objectID
			packet.Write<LocalActorID>(parentLocalActorID); // mainEntityID
			packet.Write<EntityType>(EntityType::CREATURE); // nType
			packet.Write<CreatureIndex>((CreatureIndex)(100000000 + (i32)actor.classType)); // nIDX
			packet.Write<i32>(-1);
			packet.Write(actor.pos);
			packet.Write(WorldYawToMxmYaw(actor.rotation.upperYaw));
			packet.Write(WorldPitchToMxmPitch(actor.rotation.upperPitch));
			packet.Write(WorldYawToMxmYaw(actor.rotation.bodyYaw));
			packet.Write<i32>(actor.spawnAnim);
			packet.Write<ActionStateID>(actor.actionState);
			packet.Write<i32>(actor.ownerID);
			packet.Write<i32>(actor.tagID);

			packet.Write<i32>(PlayerTeamType(parent.team));
			packet.Write<ClassType>(actor.classType);
			packet.Write<SkinIndex>(actor.skinIndex);
			packet.Write<i32>(actor.seed);


			if(actor.initStatCount > 0) WriteInitStat(packet, actor.initStats, actor.initStatCount);
			else WriteInitStat(packet, GetGameXmlContent().GetMaster(actor.classType).baseStats);





			packet.Write<u16>(0); // meshChangeActionHistory_count

			SendPacket(clientHd, packet);
		}
	}

	// SN_SpawnPosForMinimap
	{
		PacketWriter<Sv::SN_SpawnPosForMinimap> packet;

		packet.Write<LocalActorID>(localActorID); // objectID
		packet.Write(actor.pos); // p3nPos

		SendPacket(clientHd, packet);
	}

	// SN_GamePlayerStock
	{
		PacketWriter<Sv::SN_GamePlayerStock> packet;

		packet.Write<LocalActorID>(localActorID); // playerID
		packet.WriteStringObj(parent.name.data()); // name
		packet.Write<ClassType>(actor.classType); // class_
		packet.Write<i32>(parent.displayTitle);
		packet.Write<i32>(parent.statTitle);
		packet.Write<u8>(parent.badgeType);
		packet.Write<u8>(parent.badgeTier);
		packet.WriteStringObj(parent.guildTag.data()); // guildTag
		packet.Write<u8>(0); // vipLevel
		packet.Write<u8>(0); // staffType
		packet.Write<u8>(0); // isSubstituted

		SendPacket(clientHd, packet);
	}

	// SN_GamePlayerEquipWeapon
	{
		PacketWriter<Sv::SN_GamePlayerEquipWeapon> packet;

		packet.Write<LocalActorID>(localActorID); // characterID
		packet.Write<WeaponIndex>(actor.actorUID == parent.masters[0] ? parent.mainWeapon : parent.subWeapon);
		packet.Write<f32>(0.f);
		packet.Write<f32>(0.f);

		SendPacket(clientHd, packet);
	}

	/*
	// SN_StatusSnapshot
	{
		PacketWriter<> packet;

		packet.Write<LocalActorID>(localActorID); // objectID
		packet.Write<u16>(1); // statusArray_count

		packet.Write<i32>(1230350000); // statusIndex
		packet.Write<u8>(1); // bEnabled
		packet.Write<i32>(0); // caster
		packet.Write<u8>(1); // overlapCount
		packet.Write<u8>(0); // customValue
		packet.Write<i32>(0); // durationTimeMs
		packet.Write<i32>(0); // remainTimeMs

		SendPacketData<>(clientID, Sv::SN_StatusSnapshot::NET_ID, packet.size, packet.data);
	}
	*/

	SendMasterSkillSlots(clientHd, actor, parent);
}

static const u8 MONSTER_ATTACK_STAT_TYPE = 6;

void Replication::SendActorNpcSpawn(ClientHandle clientHd, const ActorNpc& actor)
{
	DBG_ASSERT(actor.actorUID != ActorUID::INVALID);

	const i32 clientID = playerMap.at(clientHd);
	playerLocalInfo[clientID].monsterSpeedSent.erase(actor.actorUID);
	auto found = playerLocalInfo[clientID].localActorIDMap.find(actor.actorUID);
	ASSERT(found != playerLocalInfo[clientID].localActorIDMap.end());

	const LocalActorID localActorID = found->second;
	const vec3 spawnPos = actor.ownerUID != ActorUID::INVALID ? actor.pos + vec3(0, 0, SUMMON_SPAWN_HEIGHT) : actor.pos;

	LOG("[client%03d] Replication :: SendActorNpcSpawn :: actorUID=%u localActorID=%u", clientID, (u32)actor.actorUID, (u32)localActorID);

	// SN_GameCreateActor
	{
		PacketWriter<Sv::SN_GameCreateActor,512> packet;

		packet.Write<LocalActorID>(localActorID); // objectID
		packet.Write<i32>(actor.entityType);
		packet.Write<CreatureIndex>(actor.docID);
		packet.Write<i32>(actor.localID);
		packet.Write(spawnPos);
		packet.Write(actor.hasCreateRot ? actor.createRot : actor.dir);
		packet.Write<i32>(actor.spawnAnim);
		packet.Write<ActionStateID>(actor.ownerUID != ActorUID::INVALID ? ActionStateID::SPAWN_BEHAVIORSTATE : actor.actionState);
		packet.Write<i32>(actor.ownerUID != ActorUID::INVALID ? (i32)GetLocalActorID(clientHd, actor.ownerUID) : actor.ownerID);
		packet.Write<u8>(actor.dirToNearPC);
		packet.Write<i32>(actor.wanderDist);
		packet.Write<i32>(actor.tagID);
		packet.Write<TeamType>(ActorTeamType(actor.faction));
		packet.Write<ClassType>(ClassType::NONE);
		packet.Write<SkinIndex>(SkinIndex::DEFAULT);
		packet.Write<i32>(actor.seed);

		const SummonInfo* summon = actor.ownerUID != ActorUID::INVALID ? GetGameXmlContent().FindSummon((i32)actor.docID) : nullptr;
		const auto* monsterStats = summon ? nullptr : GetGameXmlContent().FindMonsterStats((i32)actor.docID);
		if(monsterStats && !monsterStats->empty()) {
			packet.Write<u16>((u16)monsterStats->size());
			const auto& statList = *monsterStats;
			foreach_const(st, statList) {
				const bool scaled = st->type == (u8)StatType::STAT_TYPE_HEALTH || st->type == MONSTER_ATTACK_STAT_TYPE;
				packet.Write<u8>(st->type);
				packet.Write<f32>((st->value + GetGameXmlContent().MonsterGrowth((i32)actor.docID, st->type) * actor.growthLevel) * (scaled ? actor.statScale : 1.0f));
			}
			packet.Write<u16>(1);
			packet.Write<u8>((u8)StatType::STAT_TYPE_HEALTH);
			packet.Write<f32>((f32)actor.hp);
		}
		else if(summon && !summon->baseStats.empty()) {
			f32 hp = 1.0f;
			packet.Write<u16>((u16)summon->baseStats.size());
			foreach_const(st, summon->baseStats) {
				if(st->type == (u8)StatType::STAT_TYPE_HEALTH) hp = st->value;
				packet.Write<u8>(st->type);
				packet.Write<f32>(st->type == SUMMON_OWNER_SKILLPOWER_STAT && actor.ownerSkillPower > 0 ? actor.ownerSkillPower : st->value);
			}
			packet.Write<u16>(1);
			packet.Write<u8>((u8)StatType::STAT_TYPE_HEALTH);
			packet.Write<f32>(hp);
		}
		else {
			const f32 hp = actor.ownerUID != ActorUID::INVALID ? 1.0f : 0.0f;
			packet.Write<u16>(hp > 0 ? 1 : 0);
			if(hp > 0) { packet.Write<u8>(0); packet.Write<f32>(hp); }
			packet.Write<u16>(hp > 0 ? 1 : 0);
			if(hp > 0) { packet.Write<u8>(0); packet.Write<f32>(hp); }
		}





		packet.Write<u8>(1); // isInSight
		packet.Write<u8>(0); // isDead
		packet.Write<i64>((i64)TimeDiffMs(TimeRelNow())); // serverTime

		packet.Write<u16>(0); // meshChangeActionHistory_count

		SendPacket(clientHd, packet);
	}

	// SN_SpawnPosForMinimap
	{
		PacketWriter<Sv::SN_SpawnPosForMinimap> packet;

		packet.Write<LocalActorID>(localActorID); // objectID
		packet.Write(actor.pos); // p3nPos

		SendPacket(clientHd, packet);
	}
	if(actor.ownerUID == ActorUID::INVALID && actor.actionState == ActionStateID::SPAWN_BEHAVIORSTATE) {
		PacketWriter<Sv::SN_AiSetActionState,32> packet;
		packet.Write<u8>(0);
		packet.Write<LocalActorID>(localActorID);
		packet.Write<i16>((i16)ActionStateID::SPAWN_BEHAVIORSTATE);
		packet.Write<i16>(WorldYawToMonsterDeciDegrees(actor.dir.z));
		packet.Write<u16>((u16)MIN(actor.stateSpeed * NPC_SPEED_SCALE, 65535.0f));
		packet.Write(spawnPos);
		SendPacket(clientHd, packet);
	}
	if(actor.ownerUID != ActorUID::INVALID) {
		const SummonInfo* info = GetGameXmlContent().FindSummon((i32)actor.docID);
		PacketWriter<Sv::SN_AiSetActionState,32> packet;
		packet.Write<u8>(0);
		packet.Write<LocalActorID>(localActorID);
		packet.Write<i16>((i16)ActionStateID::SPAWN_BEHAVIORSTATE);
		packet.Write<i16>(WorldYawToMonsterDeciDegrees(actor.dir.z));
		packet.Write<u16>((u16)MIN((info ? info->moveSpeed : 0.0f) * NPC_SPEED_SCALE, 65535.0f));
		packet.Write(spawnPos);
		SendPacket(clientHd, packet);
		if(info && (info->moveSpeed > 0 || info->rotateSpeed > 0)) {
			PacketWriter<Sv::SN_AIMonsterSyncMove,48> move;
			const i16 yaw = WorldYawToMonsterDeciDegrees(actor.dir.z);
			move.Write<u8>(NPC_MOVE_FULL);
			move.Write<LocalActorID>(localActorID);
			move.Write<i16>(yaw);
			move.Write<i16>(yaw);
			move.Write<i16>((i16)MIN(info->rotateSpeed * NPC_SPEED_SCALE, 32767.0f));
			move.Write<u16>((u16)MIN(info->moveSpeed * NPC_SPEED_SCALE, 65535.0f));
			move.Write(actor.pos);
			move.Write(actor.pos);
			move.Write<u8>(NPC_MOVE_FLAG_STOPPED);
			SendPacket(clientHd, move);
		}
	}
	if(actor.ownerUID == ActorUID::INVALID && actor.dashSpeed > 0) {
		static const u8 NPC_MOVE_SPEED_AND_POSITIONS = 0x1e;
		const u16 speedSent = (u16)MIN(actor.dashSpeed * MONSTER_SPEED_SCALE, 65535.0f);
		playerLocalInfo[clientID].monsterSpeedSent[actor.actorUID] = speedSent;
		PacketWriter<Sv::SN_AIMonsterSyncMove,48> dash;
		dash.Write<u8>(NPC_MOVE_SPEED_AND_POSITIONS);
		dash.Write<LocalActorID>(localActorID);
		dash.Write<u16>(speedSent);
		dash.Write<float3>(v2f(actor.pos));
		dash.Write<float3>(v2f(vec3(actor.dashGoal, actor.pos.z)));
		dash.Write<u8>(0);
		SendPacket(clientHd, dash);
		LOG("[client%03d] Replication :: monster %u spawned during a dash, sent to its end (%.0f, %.0f)", clientID, (u32)actor.actorUID, actor.dashGoal.x, actor.dashGoal.y);
	}
}

void Replication::SendActorDynamicSpawn(ClientHandle clientHd, const ActorDynamic& actor)
{
	DBG_ASSERT(actor.actorUID != ActorUID::INVALID);

	const i32 clientID = playerMap.at(clientHd);
	auto found = playerLocalInfo[clientID].localActorIDMap.find(actor.actorUID);
	ASSERT(found != playerLocalInfo[clientID].localActorIDMap.end());

	const LocalActorID localActorID = found->second;

	LOG("[client%03d] Replication :: SendActorNpcSpawn :: actorUID=%u localActorID=%u", clientID, (u32)actor.actorUID, (u32)localActorID);

	// SN_GameCreateActor
	{
		PacketWriter<Sv::SN_GameCreateActor,512> packet;

		packet.Write<LocalActorID>(localActorID); // objectID
		packet.Write<i32>(actor.entityType);
		packet.Write<CreatureIndex>(actor.docID);
		packet.Write<i32>(actor.localID);
		packet.Write(actor.pos);
		packet.Write(actor.rot);
		packet.Write<i32>(actor.spawnAnim);
		packet.Write<ActionStateID>(actor.action);
		packet.Write<i32>(actor.ownerID);
		packet.Write<u8>(actor.dirToNearPC);
		packet.Write<i32>(actor.wanderDist);
		packet.Write<i32>(actor.tagID);
		packet.Write<TeamType>(ActorTeamType(actor.faction));
		packet.Write<ClassType>(ClassType::NONE);
		packet.Write<SkinIndex>(SkinIndex::DEFAULT);
		packet.Write<i32>(actor.seed);


		packet.Write<u16>(0);
		packet.Write<u16>(0);





		packet.Write<u8>(1); // isInSight
		packet.Write<u8>(0); // isDead
		packet.Write<i64>((i64)TimeDiffMs(TimeRelNow())); // serverTime

		packet.Write<u16>(0); // meshChangeActionHistory_count

		SendPacket(clientHd, packet);
	}

	// SN_SpawnPosForMinimap
	{
		PacketWriter<Sv::SN_SpawnPosForMinimap> packet;

		packet.Write<LocalActorID>(localActorID); // objectID
		packet.Write(actor.pos); // p3nPos

		SendPacket(clientHd, packet);
	}
}

void Replication::SendActorDestroy(ClientHandle clientHd, ActorUID actorUID, bool despawned)
{
	const i32 clientID = playerMap.at(clientHd);
	auto found = playerLocalInfo[clientID].localActorIDMap.find(actorUID);
	ASSERT(found != playerLocalInfo[clientID].localActorIDMap.end());
	const LocalActorID localActorID = found->second;

	if(despawned) {
		Sv::SN_DespawnMonster packet;
		packet.objectID = (u32)localActorID;
		LOG("[client%03d] Server :: SN_DespawnMonster :: actorUID=%u", clientID, (u32)actorUID);
		SendPacket(clientHd, packet);
		return;
	}
	Sv::SN_DestroyEntity packet;
	packet.objectID = localActorID;
	LOG("[client%03d] Server :: SN_DestroyEntity :: actorUID=%u", clientID, (u32)actorUID);
	SendPacket(clientHd, packet);
}

void Replication::SendMasterSkillSlots(ClientHandle clientHd, const Replication::ActorMaster& actor, const Player& player)
{
	DBG_ASSERT(actor.actorUID != ActorUID::INVALID);
	const LocalActorID localActorID = GetLocalActorID(clientHd, actor.actorUID);
	ASSERT(localActorID != LocalActorID::INVALID);

	const GameXmlContent& content = GetGameXmlContent();

	// SN_PlayerSkillSlot
	{
		PacketWriter<Sv::SN_PlayerSkillSlot,4096> packet;

		packet.Write<LocalActorID>(localActorID); // characterID

		const GameXmlContent::Master& master = content.GetMaster(actor.classType);
		ASSERT(master.skillUnlocked.size() == master.skillIDs.size());

		packet.Write<u16>(master.skillIDs.size());
		for(size_t i = 0; i < master.skillIDs.size(); i++) {
			packet.Write<SkillID>(master.skillIDs[i]);
			packet.Write<u32>(0);
			packet.Write<u8>(1);
			u8 level = 0;
			if(pveTeams) foreach_const(l, player.skillLevels) if(l->first == (i32)master.skillIDs[i]) level = l->second;
			const auto* ups = level > 0 ? content.FindSkillUpgrades((i32)master.skillIDs[i]) : nullptr;
			const u16 props = ups ? (u16)MIN((size_t)level, ups->size()) : 0;
			packet.Write<u16>(props);
			for(u16 k = 0; k < props; k++) {
				packet.Write<i32>((*ups)[k].propertyID);
				packet.Write<u32>(1);
			}
			packet.Write<u8>(1);
			packet.Write<u8>(1);
		}

		const size_t skillOffset = actor.actorUID == player.masters[0] ? 0 : 2;
		packet.Write<SkillID>(player.stageSkills[0]);
		packet.Write<SkillID>(player.stageSkills[1]);
		packet.Write<SkillID>(player.skills[skillOffset]);
		packet.Write<SkillID>(player.skills[skillOffset + 1]);
		SkillID shirk = master.shirkSkill != SkillID::INVALID ? master.shirkSkill : master.skillIDs.back();
		if(master.shirkSkill == SkillID::INVALID) foreach_const(sid, master.skillIDs) {
			auto f = content.skillMap.find(*sid);
			if(f != content.skillMap.end() && f->second.type == SkillType::SHIRK) { shirk = *sid; break; }
		}
		packet.Write<SkillID>(shirk);

		SendPacket(clientHd, packet);
	}
}

void Replication::SendInitialFrame(ClientHandle clientHd)
{
	// TODO: move to earlier
	// SN_InitIngameModeInfo
	{
		PacketWriter<Sv::SN_InitIngameModeInfo> packet;

		const GameXmlContent::TitanInfo& titan = GetGameXmlContent().titan;
		packet.Write<i32>(titanMode ? titan.failPlayerCool : 0);
		packet.Write<i32>(titanMode ? titan.endTeamCool : 0);
		packet.Write<i32>(titanMode ? titan.endPlayerCool : 0);
		packet.Write<i32>(0); // currentTransformationVotingPlayerCoolTimeByVotingFail
		packet.Write<i32>(0); // currentTransformationVotingTeamCoolTimeByTransformationEnd
		packet.Write<i32>(0); // currentPlayerCoolTimeByTransformationEnd
		packet.Write<i32>(20); // chPropertyResetCoolTime
		packet.Write<u8>(titanMode ? (u8)titan.cost : 0);
		packet.Write<u16>(titanMode ? (u16)titan.giants.size() : 0);
		if(titanMode) foreach_const(g, titan.giants) packet.Write<i32>(*g);
		packet.Write<i8>(titanMode ? titanNextIndex : 0);
		packet.Write<u16>(0); // limitExceptionStat_count


		SendPacket(clientHd, packet);
	}

	// SN_ScanEnd
	SendPacketData<Sv::SN_ScanEnd>(clientHd, 0, nullptr);

	// SN_LoadClearedStages
	{
		PacketWriter<Sv::SN_LoadClearedStages> packet;

		packet.Write<u16>(0); // clearedStageList_count

		SendPacket(clientHd, packet);
	}

	/*
	Sv::SN_NotifyUserLifeInfo lifeInfo;
	lifeInfo.usn = 1;
	lifeInfo.lifeCount = 3;
	lifeInfo.maxLifeCount = 3;
	lifeInfo.remainLifeCount = 0;
	LOG("[client%03d] Server :: SN_NotifyUserLifeInfo ::", clientID);
	SendPacket(clientHd, lifeInfo);
	*/
}

void Replication::CreateLocalActorID(i32 clientID, ActorUID actorUID)
{
	PlayerLocalInfo& localInfo = playerLocalInfo[clientID];
	auto& localActorIDMap = localInfo.localActorIDMap;
	localActorIDMap.emplace(actorUID, localInfo.nextPlayerLocalActorID);
	localInfo.nextPlayerLocalActorID = (LocalActorID)((u32)localInfo.nextPlayerLocalActorID + 1);
	// TODO: find first free LocalActorID

	// TODO: start at 5000 for NPCs? Does it even matter?
}

void Replication::DeleteLocalActorID(i32 clientID, ActorUID actorUID, bool keepForCaster)
{
	auto& localActorIDMap = playerLocalInfo[clientID].localActorIDMap;
	auto& departed = playerLocalInfo[clientID].departedActorIDMap;
	auto found = localActorIDMap.find(actorUID);
	if(keepForCaster) {
		if(departed.size() >= departed.max_size()) departed.clear();
		departed[actorUID] = found->second;
	}
	localActorIDMap.erase(found);
}

LocalActorID Replication::GetCasterLocalID(ClientHandle clientHd, ActorUID actorUID) const
{
	const LocalActorID local = GetLocalActorID(clientHd, actorUID);
	if(local != LocalActorID::INVALID || actorUID == ActorUID::INVALID) return local;
	const auto& departed = playerLocalInfo[playerMap.at(clientHd)].departedActorIDMap;
	auto found = departed.find(actorUID);
	return found != departed.end() ? found->second : LocalActorID::INVALID;
}

void Replication::GetPlayersInGame(ClientList* list)
{
	for(int pi = 0; pi < MAX_PLAYERS; pi++) {
		if(playerState[pi].cur < PlayerState::IN_GAME) continue;
		const ClientHandle clientHd = clientHandle[pi];
		list->push_back(clientHd);
	}
}
