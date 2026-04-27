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

struct AccountData;

struct Game
{
	enum {
		MAX_PLAYERS = 16,
		KILLS_TO_WIN = 20,
		MATCH_TIME_SEC = 240,
		TITAN_RUINS_MATCH_TIME_SEC = 1200, // 20 min for MOBA mode
		MAX_TOWERS_PER_TEAM = 8,
	};

	static constexpr f32 BASE_MOVE_SPEED = 620.0f;

	struct SpawnPoint
	{
		vec3 pos;
		vec3 rot;
	};

	struct Player
	{
		const ClientHandle clientHd;
		const AccountUID accountUID;
		const WideString& name;
		const u32 playerIndex;

		Player(ClientHandle clientHd_, AccountUID accountUID_, const WideString& name_, u32 playerIndex_):
			clientHd(clientHd_),
			accountUID(accountUID_),
			name(name_),
			playerIndex(playerIndex_)
		{

		}

		inline bool IsBot() const { return clientHd == ClientHandle::INVALID; }
	};

	// Lane IDs for Titan Ruins MOBA
	enum class Lane: u8 { TOP = 0, MID = 1, BOT = 2 };

	struct Bot
	{
		enum class AIState: u8 {
			Laning,     // walk along lane waypoints toward enemy base
			Chasing,    // chasing a nearby enemy
			Attacking,  // in attack range, auto-attacking
			Retreating, // low HP, running back to base
			Dead,       // waiting for respawn
			Unsticking,
		};

		const u32 playerIndex;
		Time tNextAction = Time::ZERO;
		Time tNextAttack = Time::ZERO;
		Time tNextSkill = Time::ZERO;
		Time tNextTag = Time::ZERO;
		Time tStateChange = Time::ZERO;
		AIState aiState = AIState::Laning;
		u32 targetIndex = 0xFFFFFFFF;
		vec3 flankTarget = vec3(0);
		i32 stuckCounter = 0;
		vec3 lastPos = vec3(0);

		// Lane assignment (MOBA)
		Lane lane = Lane::MID;
		i32 waypointIdx = 0; // current waypoint in lane path

		explicit Bot(u32 playerIndex_): playerIndex(playerIndex_) {}
	};

	enum class Phase: u8
	{
		WaitingForFirstPlayer,
		WaitingForReady,
		PreGame,
		Game,
		GameOver
	};

	enum class PickupType: u8 {
		UG,
		HP,
		Mana,
		Recharge,
	};

	struct PickupSpot {
		PickupType type;
		vec3 pos;
		f32 respawnSec;
		f32 pickupRadius;
		i32 groundItemID;
		Time tNextSpawn;
		bool alive;
	};

	// Speed pad zones (Titan Ruins)
	enum { MAX_SPEED_PADS = 4 };
	struct SpeedPad {
		vec3 pos;
		f32 radius;
		ActorUID barrierUID = ActorUID::INVALID; // linked spawn barrier entity
		bool triggered = false;
	};
	eastl::fixed_vector<SpeedPad, MAX_SPEED_PADS, false> speedPads;

	World world;
	Replication replication;

	eastl::fixed_list<Player,MAX_PLAYERS,false> playerList;
	hash_map<ClientHandle, decltype(playerList)::iterator,MAX_PLAYERS> playerMap;

	eastl::array<eastl::fixed_vector<SpawnPoint,128,false>, (i32)Faction::_COUNT> mapSpawnPoints;

	Time startTime;
	Time localTime;

	Time phaseTime = Time::ZERO;
	Phase phase = Phase::WaitingForFirstPlayer;

	// Match state
	i32 teamKills[2] = {0, 0};
	bool matchEnded = false;
	Time matchEndTime = Time::ZERO;
	Time matchStartTime = Time::ZERO;
	i32 winningTeam = -1;

	// PreGame countdown
	i32 countdownSent = 0;       // how many NotifyTimestamp sent (0..5)
	Time tNextCountdown = Time::ZERO;

	// Gate delayed destroy (fallback collision removal after open animation)
	Time gateDestroyTime = Time::ZERO;

	// Flags
	bool dummyMode = false;
	bool freezeMode = false;

	// Titan Ruins: terrain heightmap (sampled from human player, saved to disk)
	enum { HMAP_CELL = 500, HMAP_W = 64, HMAP_H = 60 }; // 500 unit cells, covers 32000x30000
	f32 heightmap[HMAP_W][HMAP_H] = {};
	bool heightmapSet[HMAP_W][HMAP_H] = {};
	bool heightmapDirty = false;
	void LoadHeightmap();
	void SaveHeightmap();


	// NPC spawning
	static i32 npcLocalID;

	// Map
	MapIndex mapIndex = MapIndex::PVP_DEATHMATCH;

	// Pickups
	eastl::fixed_vector<PickupSpot, 32, false> pickupSpots;
	i32 nextGroundItemID = 1;

	// Titan Ruins tower/core system
	struct TowerInfo {
		ActorUID uid = ActorUID::INVALID;
		u8 team = 0;        // 0=red, 1=blue
		bool destroyed = false;
		Time tNextAttack = Time::ZERO;
		u32 targetPlayerIdx = 0xFFFFFFFF; // current target
		i32 hitCount = 0; // consecutive hits on same target (for escalating damage)
	};

	void UpdateTowerAI();
	struct CoreInfo {
		ActorUID uid = ActorUID::INVALID;
		u8 team = 0;
		bool invulnerable = true; // starts invulnerable until all towers destroyed
	};

	eastl::fixed_vector<TowerInfo, MAX_TOWERS_PER_TEAM * 2, false> towers;
	CoreInfo cores[2]; // [0]=red, [1]=blue
	i32 towersAlive[2] = {0, 0}; // per team

	bool IsTitanRuins() const { return mapIndex == MapIndex::PVP_TITAN_RUINS; }

	// Tower/core creature IDs
	// 105000308 = SubTurret (main tower entity), 105000309 = vsTitan (co-located, excluded from tracking)
	static bool IsTowerCreature(CreatureIndex docID) {
		return (i32)docID == 105000308;
	}
	static bool IsCoreCreature(CreatureIndex docID) {
		i32 id = (i32)docID;
		return id == 105000304 || id == 105000305;
	}

	// Titan's Sight (capturable neutral buildings)
	enum { MAX_SIGHTS = 4, SIGHT_CAPTURE_TIME_MS = 3000, SIGHT_CAPTURE_POINTS = 3 };
	static constexpr f32 SIGHT_CAPTURE_RADIUS = 450.0f;
	static constexpr CreatureIndex CI_SIGHT_TOTEM = CreatureIndex(110043040);

	struct SightInfo {
		ActorUID uid = ActorUID::INVALID;
		vec3 pos;
		i8 ownerTeam = -1;     // -1=neutral, 0=red, 1=blue
		i32 capturingPlayer = -1; // player index currently capturing (-1=none)
		f32 captureProgress = 0;  // 0..1
	};
	eastl::fixed_vector<SightInfo, MAX_SIGHTS, false> sights;

	void UpdateSights();

	// NPC damage (towers/cores)
	void ApplyNpcDamage(World::Player& attacker, World::ActorNpc& npc, f32 damage);
	void OnTowerDestroyed(TowerInfo& tower);
	void OnCoreDestroyed(CoreInfo& core);
	void CheckCoreVulnerability(u8 team);

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
	void OnPlayerReadyToLoad(ClientHandle clientHd);
	void OnPlayerGetCharacterInfo(ClientHandle clientHd, ActorUID actorUID);
	void OnPlayerUpdatePosition(ClientHandle clientHd, ActorUID actorUID, const vec3& pos, const vec2& dir, const RotationHumanoid& rot, f32 speed, ActionStateID action, f32 clientTime);
	void OnPlayerUpdateRotation(ClientHandle clientHd, ActorUID actorUID, const RotationHumanoid& rot);
	void OnPlayerChatMessage(ClientHandle clientHd, i32 chatType, const wchar* msg, i32 msglen);
	void OnPlayerChatWhisper(ClientHandle clientHd, const wchar* destNick, const wchar* msg);
	void OnPlayerSetLeaderCharacter(ClientHandle clientHd, LocalActorID characterID, SkinIndex skinIndex);
	void OnPlayerSyncActionState(ClientHandle clientHd, ActorUID actorUID, ActionStateID state, i32 param1, i32 param2, f32 rotate, f32 upperRotate);
	void OnPlayerLoadingComplete(ClientHandle clientHd);
	void OnPlayerGameIsReady(ClientHandle clientHd);
	void OnPlayerGameMapLoaded(ClientHandle clientHd);
	void OnPlayerTag(ClientHandle clientHd, ActorUID actorUID);
	void OnPlayerJump(ClientHandle clientHd, ActorUID actorUID, f32 rotate, f32 moveDirX, f32 moveDirY);
	void OnPlayerCastSkill(ClientHandle clientHd, ActorUID actorUID, const PlayerInputCastSkill& cast, const Cl::CQ_PlayerCastSkill::PosStruct& posInfo);
	void OnPlayerWeaponFire(ClientHandle clientHd, ActorUID actorUID, const vec3& pos);
	void OnPlayerReturnToCity(ClientHandle clientHd);

	// Combat
	void ApplyDamage(World::Player& attacker, World::Player& victim, f32 damage, SkillID skillID, i32 statusID = 0);
	void KillPlayer(World::Player& victim, World::Player& killer);
	void RespawnPlayer(World::Player& player);
	void CheckMatchEnd();
	void TriggerGameOver(i32 winTeam);

	// Regen
	void UpdateRegen(World::Player& player);
	void UpdateUltimateGauge(World::Player& player);

	// Scoreboard
	void SendTeamScoreToAll();
	void SendScoreUpdateToAll(World::Player& player, i32 statID, i32 value);

	// Bot AI
	void UpdateBotAI(Bot& bot);

	// Pickups
	void InitPickupSpots();
	void UpdatePickups();
	void CollectPickup(World::Player& player, PickupSpot& spot);

	bool ParseChatCommand(ClientHandle clientHd, const wchar* msg, const i32 len);
	void SendDbgMsg(ClientHandle clientHd, const wchar* msg);
};
