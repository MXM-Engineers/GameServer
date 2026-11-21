#pragma once
#include <common/base.h>
#include <common/protocol.h>
#include <common/network.h>
#include <common/utils.h>
#include <EASTL/fixed_list.h>
#include <EASTL/fixed_hash_map.h>
#include "world.h"
#include "game.h"
#include "debug/window.h"
#include <math.h>

inline bool IsFinite(f32 v)
{
	return isfinite(v);
}

struct AccountData;

struct Game
{
	enum {
		MAX_PLAYERS = 16
	};

	struct SpawnPoint
	{
		vec3 pos;
		vec3 rot;
	};

	struct Player
	{
		ClientHandle clientHd;
		const AccountUID accountUID;
		const WideString& name;
		const u32 playerIndex;
		bool ready = false;
		bool gameStartSent = false;
		bool left = false;
		bool returnSent = false;

		Player(ClientHandle clientHd_, AccountUID accountUID_, const WideString& name_, u32 playerIndex_):
			clientHd(clientHd_),
			accountUID(accountUID_),
			name(name_),
			playerIndex(playerIndex_)
		{

		}

		inline bool IsBot() const { return clientHd == ClientHandle::INVALID; }
	};

	struct Bot
	{
		const u32 playerIndex;
		Time tNextAction = Time::ZERO;

		explicit Bot(u32 playerIndex_): playerIndex(playerIndex_) {}
	};

	enum class Phase: u8
	{
		WaitingForFirstPlayer,
		WaitingForReady,
		PreGame,
		Game
	};

	World world;
	Replication replication;

	eastl::fixed_list<Player,MAX_PLAYERS,false> playerList;
	hash_map<ClientHandle, decltype(playerList)::iterator,MAX_PLAYERS> playerMap;

	eastl::array<eastl::fixed_vector<SpawnPoint,128,false>, (i32)Faction::_COUNT> mapSpawnPoints;

	Time startTime;
	Time tGameStarted = Time::ZERO;
	bool gameEnded = false;
	Time tGameEnded = Time::ZERO;
	bool sentBackToLobby = false;
	bool Finished() const { return gameEnded && sentBackToLobby; }
	bool Deserted() const { foreach_const(p, playerList) if(p->clientHd != ClientHandle::INVALID && !p->left) return false; return true; }
	void EndGame();
	f64 LevelStartLeadMs() const;
	Time localTime;

	Time phaseTime = Time::ZERO;
	Phase phase = Phase::WaitingForFirstPlayer;

	Dbg::GameUID dbgGameUID;

	World::Player* lego = nullptr;
	vec2 legoDir = vec2(1, 0);
	i32 legoAngle = 0;
	u32 legoLastStep = 0;

	World::Player* clone = nullptr;

	eastl::fixed_list<Bot,MAX_PLAYERS,false> botList;

	void Init(Server* server_, const In::MQ_CreateGame& gameInfo, const eastl::array<ClientHandle,MAX_PLAYERS>& playerClientHdList);
	void Cleanup();

	void Update(Time localTime_);

	bool LoadMap();

	void OnPlayerDisconnect(ClientHandle clientHd);
	void OnPlayerReconnect(ClientHandle clientHd, AccountUID accountUID);
	void OnPlayerReadyToLoad(ClientHandle clientHd);
	void OnPlayerGetCharacterInfo(ClientHandle clientHd, ActorUID actorUID);
	void OnPlayerUpdatePosition(ClientHandle clientHd, ActorUID actorUID, const vec3& pos, const vec2& dir, const RotationHumanoid& rot, f32 speed, ActionStateID action, f32 clientTime, bool force = false);
	void OnPlayerUpdateRotation(ClientHandle clientHd, ActorUID actorUID, const RotationHumanoid& rot);
	void OnPlayerChatMessage(ClientHandle clientHd, i32 chatType, const wchar* msg, i32 msglen);
	void OnPlayerChatWhisper(ClientHandle clientHd, const wchar* destNick, const wchar* msg);
	void OnPlayerSetLeaderCharacter(ClientHandle clientHd, LocalActorID characterID, SkinIndex skinIndex);
	void OnPlayerSyncActionState(ClientHandle clientHd, ActorUID actorUID, ActionStateID state, i32 param1, i32 param2, f32 rotate, f32 upperRotate);
	void OnPlayerLoadingComplete(ClientHandle clientHd);
	void OnPlayerGameIsReady(ClientHandle clientHd);
	void OnPlayerGameMapLoaded(ClientHandle clientHd);
	void OnPlayerTag(ClientHandle clientHd, ActorUID actorUID);
	void OnMasterTrainingEvent(ClientHandle clientHd, const u8* data, i32 size);
	void ChangeTrainingLoadout(ClientHandle clientHd, const Cl::CN_UNKNOWN_60296::SlotPairData& slots);
	void OnTrainingEquipSkill(ClientHandle clientHd, LocalActorID chara, SkillID skill, u32 slot);
	void OnGamePing(ClientHandle clientHd, LocalActorID chara, const float3& pos, LocalActorID target, u32 ping);
	void SpawnTrainingBot(ClassType mainClass, ClassType subClass, u8 team);
	eastl::fixed_vector<MapIndex,8,false> zoneMaps;
	i32 zoneIndex = 0;
	bool zoneLoading = false;
	void MoveToNextZone();
	void UpdateAutoStage();
	Time tAutoStageNext = Time::ZERO;
	Time tAutoStageWait = Time::ZERO;
	eastl::fixed_vector<i32,64,true> autoStageVisited;
	i32 autoStageZone = -1;
	i32 autoStageZoneKills = 0;
	i32 autoStagePendingWaits = 0;
	bool pveStage = false;
	bool defenceStage = false;
	i64 gameStartUnix = 0;
	bool stageEnded = false;
	eastl::fixed_vector<Replication::RewardCard,32,false> rewardCards;
	struct RewardState { ClientHandle clientHd; i32 picks; bool finished; Time startedAt; eastl::fixed_vector<u32,8,false> slots; i32 bought = 0; u8 slotCount = 0; };
	struct StageScore { u32 attack; u32 defense; u32 time; u32 total; u8 grade; u8 technic; };
	eastl::fixed_vector<RewardState,MAX_PLAYERS,false> rewardStates;
	void EndStage(bool cleared);
	void EndDefence();
	void BuildRewardCards(const GameXmlContent::StageMaps& stage);
	void OnRewardStageReady(ClientHandle clientHd);
	void OnRewardSelectSlot(ClientHandle clientHd, u32 slot);
	void OnRewardRequestFinish(ClientHandle clientHd);
	void OnRewardBuyChance(ClientHandle clientHd);
	void AutoPickRewards(ClientHandle clientHd);
	void PlacePlayersAtSpawn();
	bool AllHumansDead() const;
	StageScore ComputeStageScore(const World::Player& player, f64 playSec) const;
	struct PveLoot { i32 playerIndex = -1; i64 gold = 0; i64 picked = 0; AccountInventory inventory; };
	eastl::fixed_vector<PveLoot,MAX_PLAYERS,false> pveLoot;
	PveLoot& LootFor(const World::Player& player);
	i64 PickedGold(const World::Player& player) const;
	Time tNextStageRank = Time::ZERO;
	void OnTitanIncarnate(ClientHandle clientHd, i32 formDoc);
	eastl::fixed_vector<u32,MAX_PLAYERS,false> trainingBots;
	eastl::array<eastl::array<u16,2>,MAX_PLAYERS> guildBoost = {};
	void OnPlayerReturnToChannel(ClientHandle clientHd);
	void OnPlayerJump(ClientHandle clientHd, ActorUID actorUID, f32 rotate, f32 moveDirX, f32 moveDirY);
	void OnPlayerSyncTeleport(ClientHandle clientHd, ActorUID actorUID, const vec3& pos);
	void OnTraitUpgrade(ClientHandle clientHd, ActorUID characterUID, const u8* indices, i32 count);
	void OnTraitReset(ClientHandle clientHd, ActorUID characterUID);
	void OnRequestVote(ClientHandle clientHd, u32 type);
	void OnVote(ClientHandle clientHd, u32 choice);
	void OnPlayerSkillUpgrade(ClientHandle clientHd, i32 slot);
	void OnPlayerTakeGroundItem(ClientHandle clientHd, u32 itemUID);
	void OnClientRemoteCreated(ClientHandle clientHd, u32 seed, RemoteIdx doc);
	void OnPlayerWeaponFire(ClientHandle clientHd, f32 aimYaw);
	void OnPlayerWeaponStateRelay(ClientHandle clientHd, ActorUID ownerUID, i32 state, u8 chargeLevel, u8 firingCombo);
	void OnPlayerWeaponShot(ClientHandle clientHd, const vec3& origin, const vec2& dir, f32 distance, const u8* rawPacket, i32 rawSize);
	void OnPlayerCastSkill(ClientHandle clientHd, ActorUID actorUID, const PlayerInputCastSkill& cast, const Cl::CQ_PlayerCastSkill::PosStruct& posInfo);
	void OnPlayerToggleSkill(ClientHandle clientHd, SkillID skillID, bool on);
	void OnPlayerInteraction(ClientHandle clientHd, ActorUID targetUID, LocalActorID selfLocal);
	void OnPlayerInteractionCancel(ClientHandle clientHd);
	bool IsWeaponFireObject(ClientHandle clientHd, const wchar* name, u16 len);
	void OnPlayerExecuteSkill(ClientHandle clientHd, SkillID skillID, const vec3& targetPos, LocalActorID target);
	void OnPlayerCancelCast(ClientHandle clientHd, SkillID skillID);
	void OnPlayerCancelStance(ClientHandle clientHd);
	void OnRemoteSyncTarget(ClientHandle clientHd, u32 remoteID, const vec3& dest);

	bool ParseChatCommand(ClientHandle clientHd, const wchar* msg, const i32 len);
	void SendDbgMsg(ClientHandle clientHd, const wchar* msg);
};
