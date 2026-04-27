#pragma once
#include <common/base.h>
#include <common/network.h>
#include <common/vector_math.h>
#include <mxm/core.h>
#include <mxm/hero_stats.h>

#include <EASTL/array.h>
#include <EASTL/fixed_list.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/fixed_hash_map.h>

#include "replication.h"
#include "physics.h"

// ============================================================================
// Buff/Debuff System
// ============================================================================

// Status effect type enumeration (maps to _Type in STATUS_PC.xml)
enum class StatusType : i32
{
	INVALID = -1,

	// Hard CC
	STUN = 0,
	AIRBORNE,
	KNOCKDOWN,
	FLY,
	FEAR,
	DRAGGED,
	DRAGGEDBYFORCE,
	SHOCKED,
	FREEZE,

	// Soft CC
	SLOW,
	ROOT,
	SILENCE,
	ACTIONBLOCK,
	PUSH,
	FORCEBACK,

	// Defensive
	GODMODE,
	SUPERARMOR,
	SHIELD,
	EVADE,
	NEARLY_IMMORTAL,
	SECOND_CHANCE,
	BURROW,
	HIDE,

	// Stat modifier
	STAT,
	AURA,
	TOGGLE,
	FIREDELAY,
	COOLTIME,
	FASTRUN,

	// DoT / HoT
	POISON,
	BURNING,
	BLEEDING,
	PUREDOT,
	HIT_PLAGUE,
	HEAL,

	// Utility
	FLAG,
	EFFECT,
	MOTION,
	LINK,
	TARGET,
	CASTER,
	DISPEL,
	SUICIDE,
	COMA,
	ONEKILL,
	ZEROCOOLTIME,

	// System
	BUSH_HIDE,
	BUSH_REVEAL,

	TYPE_COUNT
};

enum class StatusEffectCategory : u8
{
	BUFF,
	DEBUFF,
	UNKNOWN
};

// Flags for which player inputs are blocked
struct InputBlockFlags
{
	u8 attack : 1;
	u8 skill : 1;
	u8 move : 1;
	u8 jump : 1;
	u8 movingShirk : 1;
	u8 standingShirk : 1;
	u8 hideSkill : 1;
	u8 moveSkill : 1;

	void Clear() { attack = skill = move = jump = movingShirk = standingShirk = hideSkill = moveSkill = 0; }
	bool BlocksAll() const { return attack && skill && move && jump; }
	bool BlocksMovement() const { return move != 0; }
	bool BlocksActions() const { return attack != 0 || skill != 0; }
};

// Static definition loaded from STATUS_PC.xml
struct StatusDefinition
{
	i32 statusID = 0;
	StatusType type = StatusType::INVALID;
	StatusEffectCategory effectType = StatusEffectCategory::UNKNOWN;

	i32 durationTimeMs = 0;       // milliseconds (-1 = indefinite, 0 = permanent/passive)
	bool canDispel = false;
	bool passive = false;
	bool reAction = false;         // triggers CC reaction animation
	i32 maxOverlapCount = 1;       // max stacks

	// DoT/HoT
	f32 dotBaseDamage = 0;
	i32 intervalMs = 0;

	// Displacement
	f32 distanceRatio = 0;
	f32 timeRatio = 0;
	f32 heightRatio = 0;
	f32 lyingTime = 0;

	// Input blocking
	InputBlockFlags inputBlock;

	// Chaining
	i32 nextStatusIndex = 0;       // 0 = none
};

// Active status effect instance on a player
struct ActiveStatus
{
	i32 statusID = 0;
	ActorUID casterUID = ActorUID::INVALID;
	ActorUID targetUID = ActorUID::INVALID;

	f32 durationTime = 0;          // total duration in seconds
	f32 elapsedTime = 0;           // seconds elapsed
	f32 tickTimer = 0;             // for DoT/HoT interval tracking

	u8 overlapCount = 1;           // current stack count
	bool enabled = true;

	const StatusDefinition* def = nullptr;  // pointer to static definition
};

static const i32 MAX_ACTIVE_STATUSES = 32;

// Status definition manager (loaded from STATUS_PC.xml)
struct StatusManager
{
	eastl::fixed_hash_map<i32, StatusDefinition, 2048, 2048, true> definitions;
	eastl::fixed_hash_map<i32, i32, 2048, 2048, true> skillToStatus; // skillIndex → first statusID

	bool LoadFromXML(const char* filePath);
	void BuildFallbackMappings(); // Fill gaps for skills missing _SkillIndex in STATUS_PC
	const StatusDefinition* Find(i32 statusID) const;
	i32 FindStatusForSkill(i32 skillIndex) const;
};

StatusType StatusTypeFromString(const char* str);
const StatusManager& GetStatusManager();
StatusManager& GetStatusManagerMutable();
bool LoadStatusDefinitions(); // Call at server startup

// ============================================================================

struct ColliderSize
{
	u16 radius;
	u16 height;
};

struct PlayerInputCastSkill
{
	SkillID skillID = SkillID::INVALID;
	vec3 pos;
	eastl::fixed_vector<ActorUID,10,false> targetList;
};

struct World
{
	struct Player;
	struct ActorMaster;

	typedef ListItT<ActorMaster> ActorMasterHandle;

	struct PlayerDescription
	{
		UserID userID;
		ClientHandle clientHd;
		WideString name;
		WideString guildTag;
		u8 team;

		eastl::array<ClassType,2> masters;
		eastl::array<SkinIndex,2> skins;
		eastl::array<ColliderSize,2> colliderSize;
		eastl::array<SkillID,4> skills;
	};

	struct Player
	{
		struct Input
		{
			vec3 moveTo;
			f32 speed;
			RotationHumanoid rot;

			u8 tag: 1;
			u8 jump: 1;

			ActionStateID action;
			i32 actionParam1; // TODO: investigate these
			i32 actionParam2;

			PlayerInputCastSkill cast;
		};

		// Hero state for tag system (save/load active hero stats)
		struct HeroState
		{
			f32 hp = 0;
			f32 mana = 0;
			f32 maxMana = 0;
			f32 stamina = 0;
			f32 ultimateGauge = 0;
			i32 resourceStatType = 35; // 35=MP, 17=EP, 36=RP, 56=BUBBLE, 0=NONE
		};

		// Damage tracking entry for assist system
		struct DamageEntry
		{
			u32 attackerIndex;
			f32 damage;
			Time timestamp;
			SkillID skillID;
			i32 statusID = 0; // first hitStatus from REMOTE (for DDI icon)
		};

		const u32 index;
		const UserID userID;
		const ClientHandle clientHd;
		const WideString name;
		const WideString guildTag;
		const u8 team;

		const ClassType mainClass;
		const SkinIndex mainSkin;
		const ClassType subClass;
		const SkinIndex subSkin;

		const eastl::array<ColliderSize,2> colliderSize; // used when checking for gameplay collisions, not movement

		u8 level;
		u32 experience;

		eastl::array<ActorMasterHandle, PLAYER_CHARACTER_COUNT> characters;
		u8 mainCharaID = 0;

		Input input;
		PhysicsDynamicBody* body = nullptr;

		// book keeping
		struct {
			vec2 moveDir = vec2(0);
			f32 moveSpeed = 0;
			RotationHumanoid rot;
			bool hasJumped = false;
		} movement;

		// Combat stats (per-hero from XMLs)
		f32 hp = 2400;
		f32 maxHp = 2400;
		f32 atk = 200;
		f32 defense = 85;
		f32 attackRange = 500;
		f32 mana = 1000;
		f32 maxMana = 1000;
		f32 manaRegen = 5;
		f32 stamina = 200;
		f32 maxStamina = 200;
		f32 staminaRegen = 5;
		f32 hpRegen = 5;
		f32 moveSpeed = 620;
		f32 critChance = 3;
		f32 critDamage = 150;
		f32 weaponDamage = 100;
		f32 shieldDefense = 0;
		f32 lifeSteal = 0;
		f32 skillPower = 0;
		f32 skillCritChance = 3;
		f32 skillCritDamage = 150;
		f32 skillAttScaling = 15;
		f32 skillPowerPenetration = 0;
		f32 defensePenetration = 0;
		f32 cooldownReduction = 0;
		f32 attunementScaling = 15;
		f32 tagCooldown = 14;
		f32 tagOutHealing = 9;
		f32 rclickMoveRatio = 0.6f;
		f32 rotateSpeed = 700;
		f32 itemPickupRange = 2;
		i32 resourceStatType = 35; // 35=MP, 17=EP, 36=RP
		f32 ultimateGauge = 0;
		f32 maxUltimateGauge = 120;
		f32 ugSkillCost = 120;
		const HeroStats* heroStats = nullptr;

		// Hero state for tag system
		HeroState heroState[2]; // main, sub

		// Death/respawn
		bool isDead = false;
		bool inBattle = false;
		i32 kills = 0;
		i32 deaths = 0;
		i32 assists = 0;
		i32 score = 0;

		// Battle log stats (tracked per-game for PVP result)
		i32 totalDamageDealt = 0;
		i32 totalDamageReceived = 0;
		i32 highestKillStreak = 0;
		i32 currentKillStreak = 0;
		Time tRespawnTime = Time::ZERO;
		Time tInvulnUntil = Time::ZERO;
		Time tagCooldownUntil = Time::ZERO;
		bool tagCooldownActive = false; // true when 62114 with cooltime>0 was sent, reset when cooltime=0 sent
		Time tLastRegenTick = Time::ZERO;
		Time tLastUGTick = Time::ZERO;
		Time tDodgeCooldownUntil = Time::ZERO; // per-hero dodge cooldown from SKILL.xml _CoolTime

		// Damage tracking for assists (20s window)
		eastl::fixed_vector<DamageEntry, 64, false> damageLog;

		// Sight interaction (F key)
		bool interacting = false; // true when F key is held (refreshed by CQ_PlayerCastSkill with IT_ACTION_OCCUPY)
		Time tInteractStart = Time::ZERO;

		// Combat log toggle
		bool combatLogEnabled = false;

		// Disconnected flag for PVP result
		bool disconnected = false;

		// Titan Ruins: speed pad state per zone
		Time tSpeedPadCooldown[8] = {};
		u8 inSpeedPad = 0; // bitfield: bit i = currently inside pad i

		// Active status effects (buff/debuff system)
		eastl::fixed_vector<ActiveStatus, MAX_ACTIVE_STATUSES, true> activeStatuses;

		// Computed status flags (recalculated when statuses change)
		bool isStunned = false;
		bool isRooted = false;
		bool isSilenced = false;
		bool isInvulnerable = false;     // GODMODE
		bool hasSuperArmor = false;      // CC immune
		bool isHidden = false;           // stealth
		bool isBurrowed = false;         // underground
		f32  slowFactor = 1.0f;          // 1.0 = normal, 0.5 = 50% slow
	f32  speedFactor = 1.0f;         // 1.0 = normal, 1.3 = 30% speed boost
		f32  shieldHp = 0;               // shield absorption pool
		InputBlockFlags inputBlocked;    // merged from all active CC

		void RecalculateStatusFlags();
		bool HasStatus(i32 statusID) const;
		bool HasStatusType(StatusType type) const;
		i32  GetStatusStackCount(i32 statusID) const;

		explicit Player(u32 index_, const PlayerDescription& desc):
			index(index_),
			userID(desc.userID),
			clientHd(desc.clientHd),
			name(desc.name),
			guildTag(desc.guildTag),
			team(desc.team),
			mainClass(desc.masters[0]),
			mainSkin(desc.skins[0]),
			subClass(desc.masters[1]),
			subSkin(desc.skins[1]),
			colliderSize(desc.colliderSize)
		{
			inputBlocked.Clear();
		}

		inline ActorMaster& Main() const { return *characters[mainCharaID]; }
		inline ActorMaster& Sub() const { return *characters[mainCharaID ^ 1]; }

		inline bool IsBot() const { return clientHd == ClientHandle::INVALID; }

		void SaveActiveHero()
		{
			heroState[mainCharaID].hp = hp;
			heroState[mainCharaID].mana = mana;
			heroState[mainCharaID].maxMana = maxMana;
			heroState[mainCharaID].stamina = stamina;
			heroState[mainCharaID].ultimateGauge = ultimateGauge;
			heroState[mainCharaID].resourceStatType = resourceStatType;
		}

		void LoadActiveHero(const HeroStats& stats)
		{
			hp = heroState[mainCharaID].hp;
			maxHp = stats.HP;
			mana = heroState[mainCharaID].mana;
			maxMana = heroState[mainCharaID].maxMana;
			stamina = heroState[mainCharaID].stamina;
			maxStamina = stats.STAMINA;
			ultimateGauge = heroState[mainCharaID].ultimateGauge;
			resourceStatType = heroState[mainCharaID].resourceStatType;
			heroStats = &stats;
			atk = stats.ATTACK;
			defense = stats.DEFENSE;
			attackRange = stats.ATTACK_RANGE;
			manaRegen = stats.MANA_PER_SECOND;
			staminaRegen = stats.STAMINA_PER_SECOND;
			hpRegen = stats.HP_REGEN;
			critChance = stats.CRIT_CHANCE;
			critDamage = stats.CRIT_DAMAGE;
			weaponDamage = stats.WEAPON_DAMAGE;
			shieldDefense = stats.SHIELD_DEFENSE;
			lifeSteal = stats.LIFE_STEAL;
			skillPower = stats.SKILL_POWER;
			skillCritChance = stats.SKILL_CRIT_CHANCE;
			skillCritDamage = stats.SKILL_CRIT_DAMAGE;
			skillAttScaling = stats.SKILL_ATT_SCALING;
			skillPowerPenetration = stats.SKILL_POWER_PENETRATION;
			defensePenetration = stats.DEFENSE_PENETRATION;
			cooldownReduction = stats.COOLDOWN_REDUCTION;
			attunementScaling = stats.ATTUNEMENT_SCALING;
			tagCooldown = stats.TAG_COOLDOWN;
			tagOutHealing = stats.TAG_OUT_HEALING;
			rclickMoveRatio = stats.RCLICK_MOVE_SPEED_RATIO;
			rotateSpeed = stats.ROTATE_SPEED;
			itemPickupRange = stats.ITEM_PICKUP_RANGE;
			moveSpeed = stats.BASE_MOVE_SPEED * (stats.MOVE_SPEED / 100.0f);
		}
	};

	struct ActorMaster
	{
		const ActorUID UID;
		Player* parent;
		ClassType classType;
		SkinIndex skinIndex;

		ActionStateID actionState;
		i32 actionParam1;
		i32 actionParam2;

		explicit ActorMaster(ActorUID UID_): UID(UID_) {}
	};

	struct ActorNpc
	{
		const ActorUID UID;
		CreatureIndex docID;
		i32 localID;
		Faction faction;
		vec3 pos;
		vec3 rot;
		f32 hp = 10000;
		f32 maxHp = 10000;
		bool isDead = false;
		bool hasHP = false; // true for spawned NPCs that can take damage

		explicit ActorNpc(ActorUID UID_): UID(UID_) {}
	};

	struct ActorDynamic
	{
		const ActorUID UID;
		CreatureIndex docID;
		i32 localID;
		Faction faction;
		ActionStateID action;
		Time tLastActionChange;
		vec3 pos;
		vec3 rot;

		explicit ActorDynamic(ActorUID UID_): UID(UID_) {}
	};

	struct SkillProgram
	{
		SkillID skillID = SkillID::INVALID;
		ActionStateID actionID;
		vec3 castPos;
		f32 castAngle;
		ActorUID casterUID;
		eastl::fixed_vector<ActorUID,10,false> targetList;
		Time startTime;
		i32 commandID = 0;

		inline bool IsDoneExecuting() const { return skillID == SkillID::INVALID; }
		inline void Finish() { skillID = SkillID::INVALID; }
	};


	Replication* replication;

	eastl::fixed_vector<Player,10,false> players;
	eastl::fixed_list<ActorMaster,512,true> actorMasterList;
	eastl::fixed_list<ActorNpc,512,true> actorNpcList;
	eastl::fixed_list<ActorDynamic,512,true> actorDynamicList;

	typedef ListItT<ActorNpc> ActorNpcHandle;
	typedef ListItT<ActorDynamic> ActorDynamicHandle;

	// TODO: make those fixed_hash_maps
	eastl::fixed_map<ActorUID, ActorMasterHandle, 2048, true> actorMasterMap;
	eastl::fixed_map<ActorUID, ActorNpcHandle, 2048, true> actorNpcMap;
	eastl::fixed_map<ActorUID, ActorDynamicHandle, 2048, true> actorDynamicMap;

	eastl::fixed_vector<SkillProgram,40,false> skillProgramList;

	// Pending skill damage from ExecuteSkillProgram (processed by Game)
	struct PendingSkillDamage {
		u32 attackerIndex;
		u32 targetIndex;
		f32 damage;
		SkillID skillID;
		i32 statusID = 0; // first hitStatus from REMOTE (for DDI icon)
	};
	eastl::fixed_vector<PendingSkillDamage, 64, false> pendingSkillDamage;

	u32 nextActorUID;
	i32 remoteSeedCounter = 1; // VFX remote seed counter (used as 0xC0000000 + counter)
	Time localTime = Time::ZERO;

	PhysicsScene physics;

	void Init(Replication* replication_);
	void Cleanup();

	void Update(Time localTime_);
	void Replicate();

	Player& CreatePlayer(const PlayerDescription& desc, const vec3& pos, const RotationHumanoid& rot);
	ActorNpc& SpawnNpcActor(CreatureIndex docID, i32 localID);
	ActorDynamic& SpawnDynamic(CreatureIndex docID, i32 localID);
	void DestroyDynamic(ActorUID uid);

	Player& GetPlayer(u32 playerIndex);
	Player* FindPlayerByActorUID(ActorUID actorUID);
	ActorMaster* FindMasterActor(ActorUID actorUID) const;
	ActorNpc* FindNpcActor(ActorUID actorUID) const;
	ActorNpc* FindNpcActorByCreatureID(CreatureIndex docID); // Warning: slow!

	// Buff/Debuff system
	void AddStatus(Player& target, i32 statusID, ActorUID casterUID, f32 durationOverride = -1.0f);
	void RemoveStatus(Player& target, i32 statusID, ActorUID casterUID);
	void RemoveAllStatuses(Player& target);
	void TickStatuses(f32 dt);

	// Pending status applications from ExecuteSkillProgram
	struct PendingStatus {
		u32 targetIndex;
		i32 statusID;
		ActorUID casterUID;
		bool isRemove;
	};
	eastl::fixed_vector<PendingStatus, 64, false> pendingStatuses;

private:
	ActorUID NewActorUID();
	ActorMasterHandle MasterInvalidHandle();

	void PlayerCastSkill(Player& player, SkillID skill, const vec3& castPos, Slice<const ActorUID> targets);
	void ExecuteSkillProgram(SkillProgram& prog);
};
