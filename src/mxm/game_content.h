#pragma once
#include "core.h"
#include <tinyxml2.h>
#include <common/protocol.h>
#include <common/utils.h>
#include <common/stat.h>

#include <EASTL/hash_set.h>
#include <EASTL/fixed_list.h>
#include <EASTL/list.h>
#include <EASTL/fixed_hash_map.h>
#include <EASTL/fixed_map.h>
#include <EASTL/fixed_string.h>
#include "model/character_model.h"
#include "model/weapon_model.h"
#include "model/weapon_spec.h"

namespace ActionCommand {

enum class Type: i8
{
	INVALID = -1,
	STATE_BLOCK = 0,
	WEAPON_USE,
	MOVE,
	TELEPORT,
	ENTITY,
	STATUS,
	REMOTE,
	STATUS_SKILL_TARGET,
	REMOTE_SKILL_TARGET,
	GRAPH_MOVE_HORZ,
	FLY_MOVE,
	MOVESPEED,
	ROTATESPEED,
	REGCOMBO,
	SETSTANCE,
	PHYSICS,
	AIOBJECT,
	PUSH_OVERLAP,
	POLYMORPH,
	DIE_MODE,
	SET_FLAG,
	CLEAR_FLAG,
	FORCE_ROTATE,
	CALL_CHILD_SKILL,
	LONG_JUMP,
	TALK,
	EFFECT,
	AI_ADJUST,
	INTERACTION,
	DEATHWORM_BOUND,
	UI,
	CHANGE_MESH,
};

Type TypeFromString(const char* str);

enum class MovePreset: i8
{
	INVALID = -1,
	SEE_TARGET = 0,
	ROTATE,
	LIFTED,
	GRAPH,
	GRAPH_GROUND,
	MOVE_NONE,
	GOTO_CURRENT_TARGET,
	GOTO_CURRENT_TARGET_CHASE,
	SEE_CURRENT_TARGET,
	AFTERIMAGE_MOVE,
	TARGET_POS,
	TARGET_POS_LEAP,
	WARP,
	WARP_TARGET_POS,
	GOTO_CURRENT_TARGET_THROUGH,
	GOTO_CURRENT_TARGET_THROUGH_CHASE,
	SEE_MEMORIZED_TARGET,
	CHASE_TARGET,
	CHASE_TARGET_ATTACK,
};

MovePreset MovePresetFromString(const char* str);
const char* MovePresetToString(MovePreset p);


enum class TargetPreset: i8
{
	INVALID = -1,
	SELF_FFF = 0,
	SELF_FFF_1,
	SELF_TFF,
	SELF_TTF,
	SELF_TTT,
	POS_TTT_1,
	POS_FFF,
	POS_TFF,
	POS_TFF_1_800_1000,
	LAST_CHILD_FFF,
	LAST_CHILD_TFF,
	CHILD_INDEX_FFF,
	MEMORIZED_TFT,
	CURRENT_FFF,
	LOCK_TFF,
	MULTI_LOCKON_FFF,
};

TargetPreset TargetPresetFromString(const char* str);
const char* TargetPresetToString(TargetPreset p);

enum class StatusCondition: u8
{
	NONE = 0,
	FRIENDLY_HPBIGGER,
	FRIENDLY_HPLESS,
	UNFRIENDLY_HPBIGGER,
	UNFRIENDLY_HPLESS,
};

StatusCondition StatusConditionFromString(const char* str);

enum class StatusEnd: u8
{
	NONE = 0,
	ANIMATION,
	CLEANUP,
	ACTION,
};

}

struct SummonCommand
{
	enum Kind: u8 { REMOTE = 0, STATUS, DIE };
	Kind kind = REMOTE;
	bool onCurrent = false;
	bool remove = false;
	bool aniEnd = false;
	u8 dieMode = 0;
	i8 animIndex = -1;
	i32 animId = 0;
	i32 index = 0;
	f32 delay = 0;
	u16 nodeName = 0;
};

struct SummonInfo
{
	eastl::fixed_string<char,48,false> className;
	eastl::fixed_vector<StatValue,24,false> baseStats;
	f32 colliderRadius = 0;
	i32 density = 0;
	i32 clearComboOnDeath = 0;
	eastl::fixed_vector<SummonCommand,12,false> onSpawn;
	eastl::fixed_vector<SummonCommand,12,false> onDie;
	eastl::fixed_vector<SummonCommand,12,false> onLifted;
	eastl::fixed_vector<i32,6,false> dieRemovedRemotes;
	f32 lifeTime = 0;
	f32 moveSpeed = 0;
	f32 normalMoveSpeed = 0;
	f32 rotateSpeed = 0;
	f32 perception = 0;
	f32 targetLost = 0;
	f32 hideDetectRadius = 0;
	f32 destroyTime = 0;
	i32 publicDrop = 0;
	i32 spawnAnim = 0;
	i32 spawnInteraction = 0;
	bool targetable = false;
	bool aiTargetProof = false;
	eastl::fixed_vector<i32,4,false> skills;
	enum Ai: u8 { AI_NONE = 0, AI_RELAY, AI_CHASE };
	Ai ai = AI_NONE;
	i32 variation = 0;
	f32 aiDistance = 0;
	eastl::array<eastl::fixed_vector<SummonCommand,12,false>,21> skillStates;
	eastl::array<i32,21> stateAnim = {};
	eastl::array<eastl::fixed_vector<SummonCommand,4,false>,3> extraMotions;
};

struct StatusMod
{
	enum Stat: u8 { OTHER = 0, SHIELD_DEFENCE, HEALTH, SKILLPOWER, CRITICALRATE, SKILL_CRITICALRATE, ATTACK, MOVESPEED, REFLECT_DAMAGE_RATIO, DEFENCE, MANAREGEN, RAGEREGEN, STAMINA, REMOTESPEED, STAMINAREGEN, ENERGYREGEN, TAG_HEALTHREGEN, TAG_COOLTIME, HEALTHREGEN, LIFESTEAL, DAMAGE_TAKEN, RAGE, SKILL_COOLTIME_DISCOUNT, SKILLDAMAGE_REDUCE };
	Stat stat = OTHER;
	bool regen = false;
	bool mul = false;
	f32 value = 0;
	f32 count = 0;
	f32 intervalSec = 0;
};

struct MasterGrouping
{
	enum Kind: u8 { FRIENDLY = 0, ENEMY, PAIR, ALL };
	enum Apply: u8 { ALWAYS = 0, HIT, KILL_SUMMONEE, OTHER };
	enum Effect: u8 { NONE = 0, STAT, DAMAGE_BONUS, DAMAGE_REDUCE, CONSUMABLE_STAT, EXP };
	struct Mod { StatusMod::Stat stat = StatusMod::OTHER; i32 statType = -1; bool mul = false; f32 value = 0; };
	struct Unit { ClassType classType = ClassType::NONE; Effect effect = NONE; f32 value = 0; eastl::fixed_vector<Mod,4,false> mods; };
	i32 id = 0;
	Kind kind = FRIENDLY;
	Apply apply = ALWAYS;
	eastl::fixed_vector<Unit,4,false> units;
};

enum class Element: u8 { NONE, RED, GREEN, BLUE };

inline u32 StatusTypeKey(const char* name)
{
	u32 h = 2166136261u;
	for(const char* c = name; c && *c; c++) h = (h ^ (u8)*c) * 16777619u;
	return h;
}

enum class StatusType: u8
{
	OTHER,
	ACTIONBLOCK,
	ACTIVATE_UNDEAD,
	AIRBORNE,
	ARTILLERY,
	BURROW,
	CASTER,
	COMA,
	COUNTERSTATUS,
	DANCE,
	DEFEAT_CASTER_AURA,
	DEFENDER,
	DEFIANCE,
	DISPEL,
	DOUBLEGUN,
	DRAGGED,
	DRAGGEDBYFORCE,
	ELECTRO,
	EVADE,
	FEAR,
	FIREDELAY,
	FLY,
	FORCEBACK,
	FREEZE,
	GHOSTWIDOW,
	GODMODE,
	GX09,
	HEAL,
	HIDE,
	HIT_RECOVER,
	HIT_PLAGUE,
	HPTRIGGER,
	IMMUNESKILL,
	KNOCKDOWN,
	KROMEDE,
	LINK,
	MANABURN,
	MORPH,
	MOTION,
	NEARLY_IMMORTAL,
	NOCOST,
	PUSH,
	ROOT,
	SECOND_CHANCE,
	SHIELD,
	SHIELDBOMB,
	SHOCKED,
	SILENCE,
	SKILLSTAMINA,
	SKILLSTATUS,
	SLIME,
	SLOW,
	SNIPER,
	STACKATTACK,
	STACKBOMB,
	STAT,
	STATUS_ON_EVADE,
	STUN,
	SUICIDE,
	SUPERARMOR,
	TARGET,
	TOGGLE,
	UNDEAD,
	UNSHAKABLE,
	WEAK_ON_CRITICAL,
	WEAPON_GAUGE,
	WEAPON_GAUGE_RECOVERY,
};

StatusType StatusTypeFromString(const char* str);

struct StatusInfo
{
	enum class Kind: i8 {
		OTHER = 0,
		HARD_CC,
		ROOT,
		SLOW,
	};

	i32 ID = 0;
	f32 durationSec = 0;
	bool instant = false;
	f32 distanceRatio = 0;
	f32 timeRatio = 0;
	f32 heightRatio = 0;
	f32 lyingTime = 0;
	bool reaction = false;
	f32 dotDamage = 0;
	f32 dotMul = 0;
	f32 healMul = 0;
	f32 shieldMul = 0;
	bool attachToCaster = false;
	bool raiseStunByPush = false;
	f32 drainMultiplier = 0;
	i32 skillIndex = 0;
	ActionStateID motionAction = ActionStateID::INVALID;
	ActionStateID npcMotion = ActionStateID::INVALID;
	enum TagMode: u8 { TAG_STAY, TAG_MOVE, TAG_DROP };
	TagMode tagMode = TAG_STAY;
	f32 intervalSec = 0;
	f32 distance = 0;
	f32 paramRatio = 0;
	f32 dragOffset = 0;
	f32 dragForceMin = 0;
	f32 dragForceMax = 0;
	i32 maxOverlap = 1;
	Kind kind = Kind::OTHER;
	eastl::fixed_vector<StatusMod,6,false> mods;
	bool modsFromTT = false;
	enum class NextCondition: u8 { NONE = 0, ALL, TIMEOVER, REMOVE_ONHIT };
	i32 nextStatus = 0;
	bool blockAttack = false;
	bool blockSkill = false;
	bool blockShirk = false;
	bool blockMove = false;
	bool isDebuff = false;
	bool canDispel = false;
	i8 dispelEffect = -1;
	i32 dispelMaxCount = 0;
	f32 dispelDamage = 0;
	StatusType dispelStatusType = StatusType::OTHER;
	eastl::fixed_vector<StatusType,4,false> defenseTypes;
	i32 auraStatus = 0;
	eastl::fixed_vector<i32,8,false> auraStatuses;
	bool auraEnemies = false;
	i32 linkCasterStatus = 0;
	i32 linkTargetStatus = 0;
	f32 targetingRange = 0;
	bool bigAggro = false;
	bool coolTimeRecovery = false;
	eastl::fixed_vector<eastl::pair<i32,i32>,2,false> summonLimits;
	bool targetsFriendlyPc = false;
	bool targetsFriendlyNpc = false;
	bool bothCharacters = false;
	bool targetsLastChild = false;
	f32 explodeDamage = 0;
	f32 additionalDamageByCaster = 0;
	NextCondition nextCondition = NextCondition::NONE;
	StatusType type = StatusType::OTHER;
	eastl::fixed_string<char,40,false> typeName;
	u32 typeKey = 0;
	i32 shieldCount = 0;

	bool permanent = false;
	i32 customInit = 0;
	i32 customMax = 0;
	i32 hitEffectStatus = 0;
	f32 hitEffectValue = 0;
	f32 costRatioMP = 0;
	f32 stackDurationSec = 0;
	i32 stackMax = 0;
	i32 groupTeleportSkill = 0;
	f32 groupTeleportRadius = 0;
	i32 groupTeleportStatus = 0;
	bool carrier = false;
	eastl::fixed_vector<i32,8,false> costSkills;
	struct NamedValue { eastl::fixed_string<char,32,false> name; f64 value; };
	eastl::fixed_vector<NamedValue,64,false> params;
	f32 Param(const char* name, f32 fallback = 0) const
	{
		for(const NamedValue& p : params) if(p.name == name) return (f32)p.value;
		return fallback;
	}
	i64 ParamInt(const char* name, i64 fallback = 0) const
	{
		for(const NamedValue& p : params) if(p.name == name) return (i64)p.value;
		return fallback;
	}
	bool ListHas(const char* name, i32 value) const
	{
		for(const NamedValue& p : params) if(p.name == name && (i64)p.value == value) return true;
		return false;
	}
	i32 stackStatus = 0;
	i32 stackFullStatus = 0;
	eastl::fixed_vector<i32,8,false> stackRemotes;
	eastl::fixed_vector<i32,4,false> fullStackSkills;
	i32 casterStatus = 0;
	bool staminaBurn = false;
	f32 descDefenceDown = 0;
	f32 descHeal = 0;
	eastl::fixed_string<char,64,true> keyName;
	i32 flagEffectStatus = 0;
	i32 toggleRemoves = 0;
	bool endsGrab = false;
	f32 descHealPct = 0;
};

struct WeaponFiring
{
	RemoteIdx base = RemoteIdx::INVALID;
	eastl::fixed_vector<RemoteIdx,12,false> combo;
	eastl::fixed_vector<f32,10,false> comboValid;
	eastl::fixed_vector<f32,10,false> comboDelay;
	ActionStateID animation = ActionStateID::ATTACK_BEHAVIORSTATE;
	eastl::fixed_vector<ActionStateID,10,false> comboAnimation;
	eastl::fixed_vector<ActionStateID,4,false> chargeAnimation;
	ActionStateID AttackState(u8 firingCombo, u8 chargeLevel) const
	{
		if(chargeLevel > 0 && chargeLevel <= chargeAnimation.size()) return chargeAnimation[chargeLevel - 1];
		if(firingCombo > 0 && firingCombo <= comboAnimation.size()) return comboAnimation[firingCombo - 1];
		return animation;
	}
	f32 fireDelay = 0;
	i32 continuityCount = 0;
	f32 continuityDelay = 0;
	i32 firingMethod = 0;
	f32 firingStatDuration = 0;
	f32 firingMoveSpeedMul = 0;
	eastl::fixed_vector<RemoteIdx,4,false> charge;
	eastl::fixed_vector<f32,4,false> chargeTime;
	eastl::fixed_string<char,32,false> chargeDummy;
	i32 countRate = 0;
	RemoteIdx countRemote = RemoteIdx::INVALID;
	i32 replaceStatus = 0;
	RemoteIdx replaceRemote = RemoteIdx::INVALID;
	f32 maxDistance = 0;
	f32 minDistance = 0;
	eastl::fixed_string<char,32,false> fireDummy;
	bool spawnAtAim = false;
	i32 radiateNum = 1;
	f32 radiateAngle = 0;
	eastl::fixed_vector<RemoteIdx,4,false> extraRemotes;
	eastl::fixed_vector<i32,4,false> chargeParallel;
	f32 parallelOffset = 0;
	i32 ChargeParallel(u8 chargeLevel) const { return chargeLevel > 0 && chargeLevel <= chargeParallel.size() ? MAX(chargeParallel[chargeLevel - 1], 1) : 1; }
	i32 CreatedRemotes() const
	{
		i32 n = MAX(radiateNum, 1);
		for(i32 p : chargeParallel) n = MAX(n, p);
		return n + (i32)extraRemotes.size();
	}
};

struct Remote
{
	enum class BoundType: i8 {
		INVALID = -1,
		E_BOUND_NONE = 0,
		E_BOUND_RAY,
		E_BOUND_BOX,
		E_BOUND_CAPSULE,
		E_BOUND_SPHERE,
		E_BOUND_BEAM,
		E_BOUND_BEAM_NIF,
		E_BOUND_LASER,
		E_BOUND_PHYSXPROP,
		E_BOUND_HAMMER,
		E_BOUND_CASTER_MOVE_BOUND
	};

	enum class DamageGroup: i8 {
		INVALID = -1,
		eNONE = 0,
		eENEMY,
		eFRIEND,
		eALL,
	};

	enum VsFlagBit: u32 {
		VS_DYNAMIC = 0,
		VS_NPC_MONSTER,
		VS_PLAYER_CHARACTER,
	};

	enum class BehaviorType: i8 {
		INVALID = -1,
		NONETARGET = 0,
		HOMING,
		RETURN,
		CASTER_TARGET,
		BEAM,
		CHAIN,
		GRAPH,
		TARGET,
		FOLLOW_TARGET,
		ATTACH_TARGET,
		ATTACH_MUZZLE,
		TARGET_GROUNDPOSITION,
		ORBIT,
	};

	static BoundType BoundTypeFromString(const char* str);
	static const char* BoundTypeToString(BoundType t);
	static DamageGroup DamageGroupFromString(const char* str);
	static const char* DamageGroupToString(DamageGroup g);
	static BehaviorType BehaviourTypeFromString(const char* str);
	static const char* BehaviourTypeToString(BehaviorType t);

	RemoteIdx ID = RemoteIdx::INVALID;
	BoundType boundType = BoundType::INVALID;
	DamageGroup damageGroup = DamageGroup::INVALID;
	BehaviorType behaviorType = BehaviorType::INVALID;

	eastl::array<u16,3> boundSize = { 0, 0, 0 };
	u8 vs = 0;

	f32 maxSpeed = 0;
	f32 maxDistance = 0;
	i32 skillIndex = 0;
	bool useOwnerRemoteSpeedRatio = false;
	f32 orbitRadius = 0;
	f32 lifeTime = 0;
	i32 activateCount = 1;
	i32 penetrationCount = 0;
	RemoteIdx destroyCreateRemote = RemoteIdx::INVALID;
	f32 attackMultiplier = 0;
	f32 offsetY = 0;
	f32 offsetX = 0;
	u8 polygonSides = 0;
	f32 fov = 0;
	bool forceTargetDir = false;
	f32 forceFixedAngle = 0;
	f32 offsetZ = 0;
	f32 groundMoveOffsetZ = -1;
	f32 damageDistStart = 0;
	f32 statusTypeMultiplier = 1;
	f32 hitDamageMultiplier = 0;
	bool targetOnly = false;
	bool vsRemote = false;
	bool canReflect = false;
	bool reflectUse = false;
	bool reflectBack = false;
	f32 reflectDamage = 1;
	f32 maxHitDamageMultiplier = 0;
	u32 statusTypeMultiplyKey = 0;
	bool statusTypeMultiplierToCaster = false;
	f32 damageOnBound = 100;
	f32 distPenaltyRate = 0;
	f32 penaltyApplyDistance = 0;
	f32 boundOffsetY = 0;
	f32 angle = 0;
	u8 evadeBound = 0;
	f32 evadeLengthX = 0;
	f32 evadeLengthY = 0;
	f32 evadeAngle = 0;
	f32 hitInvalidTime = 1;
	f32 multiHitTimeOut = 0;
	f32 fixedGraphLength = 0;
	bool bezier = false;
	f32 bezierUp[2] = { 0, 0 };
	f32 bezierRotate[2] = { 0, 0 };
	f32 bezierFront[2] = { 0, 0 };
	bool memorizeTarget = false;

	struct AppliedStatus
	{
		i32 statusID;
		i32 rate;
		i32 value;
	};
	eastl::fixed_vector<AppliedStatus,4,false> statuses;
	eastl::fixed_vector<AppliedStatus,4,false> friendlyStatuses;

	enum SpawnKind: u8 {
		SPAWN_SINGLE = 0,
		SPAWN_RADIATE,
		SPAWN_CIRCLE,
		SPAWN_DEFAULT,
	};
	struct SpawnRule
	{
		RemoteIdx idx = RemoteIdx::INVALID;
		i32 creature = 0;
		u8 kind = SPAWN_SINGLE;
		i32 maxNum = 1;
		f32 maxAngle = 0;
		f32 radius = 0;
		f32 angleY = 0;
		bool targetOwner = false;
		f32 offsetForward = 0;
		f32 offsetHeight = 0;
		bool hitInvalidParentLastTarget = false;
	};
	typedef eastl::fixed_vector<SpawnRule,4,false> SpawnRules;
	SpawnRules onDestroy;
	SpawnRules onTime;
	SpawnRules onActivate;
	SpawnRules onHitCharacter;
	u16 creatorOnDestroy = 0;
	u16 creatorOnTime = 0;
	u16 creatorOnActivate = 0;
	u16 creatorOnHitCharacter = 0;
	u16 spawnerCreator = 0;
	bool spawnerFromTime = false;
	f32 timeEventDelay = -1;
	bool timeEventRecycle = false;

	bool contactFriend = false;
	bool vsOwner = false;
	bool includeParty = false;
	bool monsterOnly = false;
	bool attachFireObject = false;
	bool ignoreGround = false;
	bool attachTargetObject = false;
	f32 timeChangeSpeed = 0;
	ActionStateID changeCasterAction = ActionStateID::INVALID;
	f32 sight = 0;
	f32 homingLength = 0;
	f32 homingAngle = 0;
	f32 homingTime = 0;
	bool homingLockOn = false;
	f32 angularVel = 0;
	f32 initSpeed = 0;
	f32 forceScale = 0;
	f32 forceDelay = 0;
	f32 criticalRate = 0;
	f32 hitCriticalRate = 0;
	f32 maxHitCriticalRate = 0;
	i32 onHitOwnerStatus = 0;
	f32 activateDelay = 0;
};

struct WeaponHeatInfo
{
	f32 maxGauge = 0;
	f32 consumption = 0;
	f32 autoRegen = 0;
	f32 autoRegenInterval = 0.1f;
	f32 autoRegenDelay = 0;
	f32 overheatDuration = 0;
	f32 overheatResetGauge = 0;
};

enum class PraiseType: u8
{
	INVALID = 0,
	KILL_ASSIST,
	KILL,
	DAMAGE_PC,
	MAX_CONTINUOUS_KILL,
	MAX_CONTINUOUS_KILL_TRANSFORM,
	REFLECTION_KILL,
	DOT_KILL,
	HEAL_OTHERS,
	HEAL_SELF,
	NEVER_DIE,
	TOTAL_SCORE,
	DAMAGE_CORE,
	DEFEAT_APPLY_COUNT,
	KILL_TOWER,
	OCCUPY_SIGHT,
	SAVE_PIECE,
	KILL_BOSS,
	STEAL_BOSS,
	KILL_TANIANKING,
	KILL_TRANSFORMCH,
	KILL_TRANSFORMREADYCH,
	COUNT
};

enum class PraiseRelationType: u8
{
	INVALID = 0,
	KILL_ASSIST,
	KILL_ASSIST_RATIO,
	DAMAGE_PC,
	DAMAGE_PC_RATIO,
	REFLECTION_COUNT,
	HEAL_OTHERS_PER_SECOND,
	HEAL_SELF_PER_SECOND,
	SCORE_RATIO,
	DAMAGE_CORE_RATIO,
	DEFEAT_APPLY_COUNT,
	DAMAGE_TOWER,
	SAVE_PIECE_RATIO,
	COUNT
};

struct GameXmlContent
{
	struct Master
	{
		CreatureIndex ID;
		ClassType classType;
		eastl::fixed_string<char,64,false> className;
		eastl::fixed_vector<SkillID,32,false> skillIDs;
		SkillID shirkSkill = SkillID::INVALID;
		eastl::fixed_vector<u8,32,false> skillUnlocked;
		eastl::fixed_vector<u8,32,false> skillUnlockOrder;
		eastl::fixed_vector<SkinIndex,20,false> skinIDs;
		eastl::fixed_vector<WeaponIndex,32,false> weaponIDs;
		eastl::fixed_vector<WeaponIndex,3,false> defaultWeaponIDs;
		eastl::fixed_vector<WeaponIndex,3,false> fairPvpWeaponIDs;
		eastl::fixed_vector<StatValue,32,false> baseStats;
		i32 densityCount = 0;
		f32 moveSpeed = 0;




		CharacterModel character;
	};

	struct GearItem
	{
		i32 slotType = -1;
		eastl::fixed_vector<StatValue,4,false> statMul;
		eastl::fixed_vector<StatValue,4,false> statAdd;
	};
	eastl::hash_map<i32, GearItem> gearItems;
	eastl::array<i32,(i32)GameType::MAX> gearPowerPercent;
	eastl::fixed_vector<eastl::pair<u8,i32>,16,false> achievementTierPoints;
	u8 AchievementTier(i32 score) const;
	i32 GearPowerPercent(GameType type) const { return type > GameType::INVALID && type < GameType::MAX ? gearPowerPercent[(i32)type] : 100; }
	bool LoadServiceRule();
	eastl::hash_map<i32, eastl::fixed_vector<eastl::pair<i32,i32>,6,false>> weaponIngredients;
	struct PriceCost { u8 buyType = 0; i32 amount = 0; };
	eastl::array<PriceCost,4> weaponUnlockCost = {};
	eastl::array<PriceCost,4> skillUnlockCost = {};
	eastl::fixed_vector<PriceCost,8,false> pveSkillUpgradeCost;
	eastl::fixed_vector<PriceCost,24,false> masterGearAddCost;
	eastl::array<eastl::fixed_vector<i32,4,false>,5> rewardBuyChanceSteps;
	i32 RewardBuyChanceCost(u8 difficultyBit, i32 bought) const;
	bool LoadServicePrice();
	enum class AchieveSub: u8 {
		NONE, PVE_KILL_MONSTER_TOTAL, PVE_SUFFER_FROM_TARGET, PVE_STAGE_CLEAR, PVE_STAGE_CLEAR_WITH_FRIENDS, PVE_DEATH_TOTAL, PVE_KILL_IN_CERTAIN_TIME,
		PVE_SURVIVAL, PVE_INTERACTION, PVE_BRING_COMRADE_COUNT, PVP_PLAY, PVP_WIN_TOTAL, PVP_LOSE_TOTAL, PVP_DRAW_TOTAL, PVP_DEATH_TOTAL,
		PVP_KILL_STREAK, PVP_KILL_STREAK_TOTAL, PVP_KDA, PVP_OCCUPY_ALL_CORES, PVP_SPORTS_AWARDED, COMMON_USE_SKILL, COMMON_SKILL_UPGRADE_COUNT,
		COMMON_MASTER_SORTIE, COMMON_MASTER_UNLOCK, COMMON_MASTER_TOTAL, COMMON_WEAPON_UPGRADE_COUNT, COMMON_ITEM, COMMON_MULTI, COMMON_LEVEL,
		COMMON_FRIEND, COMMON_ACHIEVEMENT_POINT, COMMON_ATTEND_ROLL_CALL, COMMON_FREE_CCOIN_ASSETS, COMMON_RON_ASSETS, COMMON_SALES,
		COMMON_TRADE_COUNT, COMMON_TITLE_COUNT, COMMON_MISSION_ACHIEVE_COUNT, COMMON_JOIN_GUILD, COMMON_CREATE_GUILD, COUNT
	};
	struct Achievement
	{
		i32 index = 0;
		u8 group = 0;
		AchieveSub sub = AchieveSub::NONE;
		i32 point = 0;
		i32 gainTitle = 0;
		i64 count = 0;
		i64 value = 0;
		f32 fvalue = 0;
		i32 section = -1;
		i32 gameType = -1;
		i32 classType = -1;
		u8 rank = 0;
		eastl::fixed_vector<i32,16,false> objects;
	};
	eastl::vector<Achievement> achievements;
	eastl::hash_map<i32,i32> achievementPos;
	eastl::array<eastl::vector<i32>,(i32)AchieveSub::COUNT> achievementsBySub;
	eastl::hash_map<i32,eastl::fixed_vector<i32,4,true>> achievementParents;
	eastl::vector<eastl::vector<i32>> sections;
	eastl::hash_map<size_t,i32> sectionByName;
	bool LoadSections();
	static u8 ClearRankFromName(const char* name);
	bool LoadAchievements();
	struct TitleStat { u8 type = 0; f32 value = 0; bool mul = false; };
	eastl::hash_map<i32,eastl::fixed_vector<TitleStat,4,false>> titleStats;
	const eastl::fixed_vector<TitleStat,4,false>* FindTitleStats(i32 title) const { auto f = titleStats.find(title); return f == titleStats.end() ? nullptr : &f->second; }
	struct SummaryReward { u8 type = 0; i32 amount = 0; i32 item = 0; };
	enum class SummaryChallenge: u8 { NONE, NODIE, STATUSTYPE, NOHITBYBOSS, KILL_IN_TIME, RANK_OVER, NOKILL, KILL_GREATER_EQUAL, STATUSINDEX, KILL_LESS };
	struct SummaryChallengeInfo
	{
		i32 index = 0;
		SummaryChallenge type = SummaryChallenge::NONE;
		i32 difficulty = -1;
		u8 rank = 0;
		i32 status = 0;
		f32 timeSec = 0;
		i32 totalCount = 0;
		eastl::fixed_vector<eastl::pair<i32,i32>,8,false> targets;
		SummaryReward reward;
	};
	struct Summary
	{
		i32 index = 0;
		i32 stage = 0;
		eastl::fixed_vector<eastl::pair<i32,SummaryReward>,16,false> rares;
		eastl::fixed_vector<SummaryChallengeInfo,16,false> challenges;
		eastl::fixed_vector<SummaryReward,8,false> progress;
	};
	eastl::vector<Summary> summaries;
	eastl::hash_map<i32,eastl::pair<i32,i32>> stageSummary;
	bool LoadSummaries();
	const Summary* FindSummary(i32 index) const { foreach_const(s, summaries) if(s->index == index) return &*s; return nullptr; }
	const Summary* FindStageSummary(i32 stage, i32* difficulty) const;
	bool SectionContains(i32 section, i32 stage) const;
	const Achievement* FindAchievement(i32 index) const { auto f = achievementPos.find(index); return f == achievementPos.end() ? nullptr : &achievements[f->second]; }
	i32 WeaponIndexFor(const Master& master, u32 type, u32 grade) const;
	const GearItem* FindGearItem(i32 doc) const { auto f = gearItems.find(doc); return f == gearItems.end() ? nullptr : &f->second; }

	struct GroundItemInfo
	{
		i32 id = 0;
		eastl::fixed_vector<StatValue,4,false> statMul;
		eastl::fixed_vector<StatValue,4,false> statAdd;
		eastl::fixed_vector<i32,4,false> statuses;
		f32 lifeTime = 0;
	};

	struct DropGroup
	{
		enum class Kind: u8 { ITEM, RON, GROUP, GEAR };
		struct Entry { i32 item = 0; i32 minCount = 1; i32 maxCount = 1; f32 rate = 100; Kind kind = Kind::ITEM; };
		bool piece = false;
		eastl::fixed_vector<Entry,32,false> entries;
	};

	struct DropRoll { i32 item = 0; i32 count = 0; bool ron = false; };
	typedef eastl::fixed_vector<DropRoll,64,false> DropRollList;

	static const u8 ALL_DIFFICULTIES = 0xff;

	struct LevelEventNode
	{
		enum Kind: u8 { OTHER = 0, LEVEL_START, TAKE_ITEM, DELAY, CALL_SFX, CALL_ITEM, BOUND_TRIGGER, STATUS_CHANGE, ACTIVATE_TRIGGER, ON_HIT_TRIGGER, ACTION_CHANGE, CLIENT_EVENT, CALL_MONSTER, DESPAWN_ENTITY, PASS_THROUGH, TIME_RECORD_START, DEAD_COUNT, HP_TRIGGER, OR_CONDITION, GAME_END_TRIGGER, SAFE_ZONE, TEAM_CONDITION, ADD_GOT_PIECE, SAVE_GOT_PIECE, AI_TRIGGER, INVOKE_AI_TRIGGER, NOTIFY_RESPAWN, ACTIVE_INTERACTION, INTERACTION_TRIGGER, ADD_CAST_TIME, TELEPORT_TO, RANDOM_CONDITION, PORTAL, KILL_TARGET, ACTIVE_AREA_TRIGGER, INVOKE_DIALOG, DIALOG_TRIGGER, SET_ACTIVATION, AND_CONDITION, START_PHASE, END_PHASE, CAMP_GUARD, ACTIVE_EVENTS, CREATURE_ENTER, SWITCH_CONDITION, RANDOM_CHOICE, AI_PAUSE, EXECUTE_LUA, PVP_TAKE_SCORE };
		enum ApplyType: u8 { APPLY_MY_CHAR = 0, APPLY_LOCAL_ID, APPLY_ALL_PARTY, APPLY_OTHER };
		i32 id = 0;
		Kind kind = OTHER;
		i32 tagID = -1;
		f32 delay = 0;
		f32 variance = 0;
		i32 dropIndex = 0;
		i32 areaID = 0;
		u32 nameKey = 0;
		u32 targetKey = 0;
		u32 groupKey = 0;
		bool enter = false;
		bool active = false;
		bool dialogLine = false;
		bool seqRoot = false;
		f32 retrigger = 0;
		i32 maxTriggers = 0;
		i32 statusIndex = 0;
		i32 localID = 0;
		i32 hitCount = 0;
		ActionStateID action = ActionStateID::INVALID;
		i32 monster = 0;
		i32 count = 1;
		i32 callCount = 1;
		f32 callDelay = 0;
		f32 callInterval = 0;
		i32 spawnAnim = 0;
		i32 wander = -1;
		Faction team = Faction::INVALID;
		ApplyType applyType = APPLY_OTHER;
		bool apply = true;
		bool resetAfter = false;
		bool initTagCool = false;
		bool clientDriven = false;
		u8 stageLevelBit = ALL_DIFFICULTIES;
		eastl::fixed_vector<eastl::pair<i32,u8>,8,false> inputs;
		bool ownTeamOnly = false;
		f32 hpPercent = 0;
		i32 aiIndex = 0;
		i32 phase = 0;
		u8 difficultyMask = ALL_DIFFICULTIES;
		bool relative = false;
		struct RandomEvent { u8 branch; u16 weight; u32 nameKey; };
		eastl::fixed_vector<RandomEvent,16,true> randomEvents;
		eastl::fixed_vector<i32,8,true> outputs;
	};

	struct Map
	{
		struct Spawn
		{
			enum class Type: i32 {
				NORMAL=0,
				SPAWN_POINT
			};

			CreatureIndex docID;
			i32 localID;
			Type type;
			Faction faction;
			vec3 pos;
			vec3 rot;
			f32 scale = 1.0f;
			i32 entityType = 1;
			i32 spawnAnim = 0;
			i32 wanderDist = -1;
			i32 tagID = -1;
			i32 ownerID = 0;
			u8 dirToNearPC = 0;
			ActionStateID actionState = ActionStateID::INVALID;
			u8 difficultyMask = 0xff;

			inline bool IsSpawnPoint() const { return type == Type::SPAWN_POINT; }
		};

		struct Area
		{
			i32 ID;
			i32 type;
			i32 layer;
			vec3 size;
			vec3 pos;
			vec3 rot;
		};

		struct Collider
		{
			eastl::fixed_string<char,128,true> mesh;
			vec3 pos;
			vec3 rot;
			f32 scale = 1;
			bool walk = true;
		};

		MapIndex index = MapIndex(0);
		eastl::vector<Collider> colliders;
		bool groundPlane = true;
		f32 killZ = -1000000.0f;
		bool ingameLevels = false;
		f32 levelStartLead = 5.0f;

		eastl::fixed_vector<Spawn,512> creatures;
		eastl::fixed_vector<Spawn,512> dynamic;
		eastl::fixed_vector<Area,512> areas;
		eastl::vector<LevelEventNode> levelEvents;
		eastl::hash_map<i32, eastl::fixed_vector<vec3,48,true>> paths;
		eastl::hash_set<i32> roundTripPaths;

		const Area* FindArea(i32 id) const { foreach_const(a, areas) if(a->ID == id) return &*a; return nullptr; }
		const LevelEventNode* FindLevelEvent(i32 id) const { foreach_const(n, levelEvents) if(n->id == id) return &*n; return nullptr; }
		const LevelEventNode* FindLevelEventByName(u32 nameKey) const { foreach_const(n, levelEvents) if(n->nameKey == nameKey) return &*n; return nullptr; }
	};

	struct MapList
	{
		MapIndex index;
		MapType mapType;
		GameSubModeType gameSubModeType;
		eastl::fixed_string<char, 256> levelFile;
		u8 pvpSaveType = 0;
	};

	struct Song
	{
		SongID ID;
		i32 length;
	};

	struct Action
	{
		struct Command
		{
			ActionCommand::Type type;
			f32 delay;
			f32 relativeEndTimeFromStart;
			i32 animIndex = 0;
			i32 animationId = 0;
			u16 nodeName = 0;
			u8 byDummyDir = 0;
			u8 rootDummyDir = 0;
			u8 lowerDummyDir = 0;

			union {
				struct {

				} stateBlock;

				struct {
					// not really useful for anything?
				} weaponUse;

				struct {
					ActionCommand::MovePreset preset;
					i32 param2;
				} move;

				struct {
					RemoteIdx idx;
					ActionCommand::TargetPreset targetPreset;
					f32 offsetSide;
					f32 offsetFront;
					u8 stopAtTarget;
					u8 remove;
					u8 removeAtAnimEnd;
				} remote;

				struct {
					i32 statusIndex;
					ActionCommand::TargetPreset targetPreset;
					u8 removeAtEnd;
					u8 remove;
					ActionCommand::StatusCondition condition;
					i32 childCreature;
				} status;

				struct {
					i32 entityIndex;
					ActionCommand::TargetPreset targetPreset;
					u8 remove;
					f32 offsetFront;
					f32 offsetSide;
				} entity;

				struct {
					f32 distance;
				} graphMoveHorz;

				struct {
					u8 toLastChild;
					u8 toSpawnPoint;
					u8 toTarget;
				} teleport;

				struct {
					i32 interactionID;
				} interaction;

				struct {
					i32 speed;
				} rotateSpeed;

				struct {
					i32 percent;
				} moveSpeed;
			};
		};

		ActionStateID ID;
		i8 stance = 0;
		f32 seqLength = 0.0f; // seconds
		struct AnimStep { i32 index = 0; i32 animationId = 0; i32 loop = 1; };
		eastl::fixed_vector<AnimStep,8,false> anims;
		eastl::fixed_vector<Command,16,false> commands;
	};

	eastl::fixed_vector<Master,100,false> masters;
	eastl::fixed_vector<WeaponModel, 100, false> weaponsModel;
	eastl::fixed_hash_map<size_t,Master*,100> masterClassStringMap;
	eastl::fixed_hash_map<ClassType,Master*,100> masterClassTypeMap;
	eastl::fixed_hash_map<CreatureIndex,Master*,100> masterIdMap;

	eastl::fixed_hash_map<ClassType,CreatureIndex,100> deathMatchBotIndex;

	eastl::fixed_vector<MapList, 500, false> maplists;
	eastl::fixed_hash_map<SkillID, SkillNormalModel, 500> skillMap;
	eastl::fixed_vector<Action, 2000, false> actionList;
	eastl::fixed_hash_map<ClassType, Slice<Action>, 800> actionListMap;

	Map mapLobby;
	eastl::list<Map> pvpMaps;

	eastl::fixed_vector<Song,60,false> jukeboxSongs;

	struct EntrySystem
	{
		i32 ID;
		eastl::fixed_string<char,32,false> entryType;
		eastl::fixed_vector<AreaIndex,8,false> areas;
		eastl::fixed_vector<AreaIndex,8,false> scheduleAreas;
	};

	struct AreaStages
	{
		AreaIndex ID;
		eastl::fixed_vector<StageIndex,16,false> stages;
		eastl::fixed_vector<StageIndex,64,false> allStages;
	};

	struct StageMaps
	{
		StageIndex ID;
		i32 joinMemberMax = 0;
		bool titan = false;
		GameModeType gameMode = GameModeType::GAME_MODE_INVALID;
		eastl::fixed_vector<MapIndex,8,false> maps;
		eastl::fixed_vector<i32,8,false> mapRates;
		eastl::fixed_vector<MapIndex,8,false> aiIgnoredMaps;
		MapIndex bossMap = MapIndex(0);
		u8 difficultyBit = ALL_DIFFICULTIES;
		i32 clearMonster = 0;
		i32 clearCount = 0;
		i32 defendTarget = 0;
		i32 moneyReward = 0;
		eastl::fixed_vector<i32,8,false> viewRewards;
		eastl::fixed_vector<eastl::pair<u8,i32>,32,false> commonDrop;
		SkillID stageSkill = SkillID::INVALID;
		eastl::fixed_vector<SkillID,8,false> stageSkillCandidates;
	};

	struct Mission
	{
		enum class Category: u8 { DAILY, WEEKLY, MONTHLY, GUILD, OTHER };
		enum class SubType: u8 {
			OTHER,
			ACHIEVE_MISSION,
			GET_GOLD,
			GET_EXP,
			MASTER_SORTIE,
			PVP_PLAY,
			PVP_WIN,
			PVP_KILL,
			PVP_ASSIST,
			PVP_KILL_STREAK,
			PVP_FIRST_BLOOD,
			PVP_DAMAGE,
			PVP_TITAN_SCORE,
			PVP_TITAN_SCORE_TOTAL,
			PVE_MISSION_RANK,
			PVE_STAGE_CLEAR_NO_DIE,
			PVE_KILL_TOTAL,
			PVE_RANDOM_STAGE_CLEAR,
			PVE_STAGE_CLEAR_WITH_COMRADE,
		};
		enum class Section: u8 { OTHER, ALL, PVP_ALL, PVP_TITAN, STAGES };
		enum class RewardType: u8 { NONE, GOLD, EXP, CCOIN, ITEM, GUILDFUND };
		i32 ID;
		Category category;
		i32 step;
		i32 grade;
		bool dayRestricted;
		SubType subType = SubType::OTHER;
		Section section = Section::OTHER;
		i64 count = 0;
		i64 value = 0;
		GameType gameType = GameType::INVALID;
		Category requiredCategory = Category::OTHER;
		eastl::fixed_vector<ClassType,4,false> requiredClasses;
		RewardType rewardType = RewardType::NONE;
		i64 rewardAmount = 0;
		i32 rewardItem = 0;
		i32 sectionIndex = -1;
		u8 rank = 0;
		u8 minDifficultyBit = 0;
	};

	struct ExpLevel
	{
		u32 required = 0;
		u32 gold = 0;
	};

	struct SupportKit
	{
		i32 hpPercent = 0;
		f32 coolTime = 0;
		i32 coolTimeID = 0;
		eastl::fixed_vector<i32,4,false> statuses;
	};
	eastl::hash_map<i32,SupportKit> supportKits;
	const SupportKit* FindSupportKit(i32 doc) const { auto f = supportKits.find(doc); return f == supportKits.end() ? nullptr : &f->second; }
	struct ItemInfo
	{
		i32 ID = 0;
		i32 maxStack = 1;
		i32 firstSlot = 200;
		u8 buyType = 0;
		i32 buyValue = 0;
		i32 sellValue = 0;
		i32 boxDrop = 0;
	};
	eastl::hash_map<i32,ItemInfo> itemInfoMap;
	struct ServiceCost { u8 buyType = 0; i32 amount = 0; };
	eastl::fixed_vector<ServiceCost,32,false> inventoryExpandCosts;
	struct ActivityTier { f32 ratio = 0; i32 item = 0; i32 amount = 0; };
	eastl::fixed_vector<ActivityTier,8,false> activityTiers;
	i32 activityMaxPoint = 0;
	const ItemInfo* FindItemInfo(i32 ID) const;
	struct TradeItem { i32 ID = 0; eastl::fixed_vector<eastl::pair<i32,i32>,4,false> required; i32 buyItem = 0; bool buyDrop = false; u8 costType = 0; i32 costAmount = 0; };
	eastl::hash_map<i32,TradeItem> tradeItems;
	const TradeItem* FindTradeItem(i32 ID) const { auto f = tradeItems.find(ID); return f == tradeItems.end() ? nullptr : &f->second; }
	bool LoadTradeItems();
	void LoadItemInfos(tinyxml2::XMLDocument& doc);

	eastl::fixed_vector<Mission,256,false> missions;
	const Mission* FindMission(i32 ID) const;
	eastl::fixed_vector<ExpLevel,1024,false> expLevels;
	eastl::fixed_vector<EntrySystem,32,false> entrySystems;
	eastl::fixed_vector<AreaStages,64,false> areaStages;
	eastl::fixed_vector<StageMaps,512,false> stageMaps;

	struct GuildLevelInfo
	{
		i32 level;
		i32 requirePoint;
		i32 medalGiftCount;
	};

	struct GuildSkillLevel
	{
		i32 level;
		i32 unlockGuildLevel;
		i32 cost;
		i32 value;
	};

	struct GuildSkillInfo
	{
		eastl::fixed_string<char,64,false> key;
		u8 type = 0;
		i32 limitedHours = 0;
		eastl::fixed_vector<GuildSkillLevel,12,false> levels;
	};

	eastl::fixed_vector<GuildLevelInfo,16,false> guildLevels;
	eastl::fixed_vector<GuildSkillInfo,16,false> guildSkills;
	eastl::fixed_vector<i32,64,false> validGuildEmblems;
	i32 guildActivityCapWeekday = 100;
	i32 guildFoundationLevel = 10;
	i32 guildFoundationGold = 18000;
	i32 guildDonationUnit = 100;
	i32 guildFundExchangeRate = 100;
	i32 guildRollcallPoint = 50;
	enum GuildBonusKind: u8 { GUILD_BONUS_ARENA = 0, GUILD_BONUS_STAGE, GUILD_BONUS_DEFENCE, GUILD_BONUS_COUNT };
	eastl::array<eastl::array<i32,6>,GUILD_BONUS_COUNT> guildMemberBonus = {};
	i32 GuildMemberBonus(GuildBonusKind kind, i32 members) const { return members >= 1 && members <= 5 ? guildMemberBonus[kind][members] : 0; }
	i32 guildActivityCapWeekend = 250;
	i32 guildTagChangeCost = 0;
	i32 guildMedalItem = 0;
	static const i32 MEASURED_PVP_MEDAL_ITEM = 138000391;
	i32 pvpMedalItem = MEASURED_PVP_MEDAL_ITEM;
	i32 guildMembershipCapacity = 0;
	const GuildSkillInfo* FindGuildSkill(u8 type) const { foreach_const(s, guildSkills) if(s->type == type) return &*s; return nullptr; }
	i32 GuildSkillValueByType(u8 type, i32 level) const { const GuildSkillInfo* s = FindGuildSkill(type); if(!s) return 0; foreach_const(l, s->levels) if(l->level == level) return l->value; return 0; }

	i32 GuildLevelForPoints(i32 points) const;
	i32 GuildSkillValue(const char* key, i32 level) const;

	bool FindQueueAreaStage(i32 entryID, AreaIndex* outAreaIndex, StageIndex* outStageIndex) const;
	bool FindEntryStageArea(i32 entryID, StageIndex stage, AreaIndex* outAreaIndex) const;
	bool FindStageMap(StageIndex stageID, MapIndex* outMapIndex) const;
	bool PickStageMap(StageIndex stageID, bool aiMode, MapIndex* outMapIndex) const;
	bool IsMapLoadable(MapIndex mapID) const;
	const StageMaps* FindStageMaps(StageIndex stageID) const;
	bool HasEntrySystem(i32 entryID) const;
	struct StageEntryStage { StageIndex stage; eastl::fixed_vector<u8,4,false> gameTypes; };
	struct StageEntryArea { AreaIndex area; eastl::fixed_vector<StageEntryStage,32,false> stages; };
	struct StageEntry { i32 ID; eastl::fixed_vector<StageEntryArea,8,false> areas; };
	eastl::fixed_vector<StageEntry,4,false> stageEntries;
	bool LoadStageEntries();
	eastl::hash_map<i32,f32> difficultyMonsterStat;
	bool LoadStageDifficulty();
	f32 DifficultyMonsterStat(u8 bit) const { auto f = difficultyMonsterStat.find((i32)bit); return f == difficultyMonsterStat.end() ? 1.0f : f->second; }
	CreatureIndex FindDeathMatchBotIndex(ClassType classType) const;
	eastl::hash_map<i32,eastl::fixed_vector<ClassType,16,false>> stageBotClasses;
	bool LoadStageBots();

	struct DefencePhase
	{
		i32 step = 0;
		f32 timeout = 0;
		f32 endDelay = 0;
		i32 killCount = 0;
		eastl::fixed_vector<i32,16,false> excluded;
		u32 gold = 0;
		u32 exp = 0;
		u32 guildPoint = 0;
		eastl::fixed_vector<eastl::pair<i32,i32>,4,false> items;
		bool Excludes(i32 doc) const { return eastl::find(excluded.begin(), excluded.end(), doc) != excluded.end(); }
	};
	eastl::vector<DefencePhase> defencePhases;
	bool LoadDefencePhases();

	struct PvpPraise
	{
		GameModeType gameMode = GameModeType::GAME_MODE_INVALID;
		i32 priority = 0;
		PraiseType type = PraiseType::INVALID;
		PraiseRelationType relation = PraiseRelationType::INVALID;
		f32 minValue = 0;
	};
	eastl::fixed_vector<PvpPraise,32,false> pvpPraises;
	bool LoadPvpPraises();
	const eastl::fixed_vector<ClassType,16,false>* FindStageBots(StageIndex stage) const { auto f = stageBotClasses.find((i32)stage); return f == stageBotClasses.end() ? nullptr : &f->second; }

	eastl::vector<FileBuffer> collisionFiles;
	FileBuffer fileCylinderCollision;

	bool Load();
	const MapList* FindMapListByID(MapIndex index) const;
	const Map* FindPvpMap(MapIndex index) const;
	eastl::hash_map<i32,i32> pvpMapAliases;

	struct MonsterFire { i32 remote = 0; eastl::fixed_string<wchar,48,true> node; f32 at = 0; u8 pass = 0; bool atTarget = false; };

	struct MonsterAttack
	{
		enum Target: u8 { MASTER = 0, MINION, STRUCTURE, TITAN, ALLY, _COUNT };
		Target target = MASTER;
		ActionStateID state = ActionStateID::INVALID;
		f32 range = 0;
		f32 cooldown = 0;
		i32 remote = 0;
		i32 hitRemote = 0;
		bool executeMinion = false;
		f32 anim = 0;
		bool noDamage = false;
		bool selfDie = false;
		i32 allyStatus = 0;
		f32 allyRadius = 0;
		f32 allyDelay = 0;
		f32 delay = 0;
		f32 multiplier = 0;
		f32 speed = 0;
		i32 status = 0;
		bool special = false;
		f32 radius = 0;
		u8 hits = 1;
		f32 interval = 0;
		f32 hpMin = 0;
		f32 hpMax = 1;
		f32 weakTime = 0;
		f32 weakAt = 0;
		f32 hitAt = -1.0f;
		f32 firstAt = -1.0f;
		f32 after = 0;
		f32 minRange = 0;
		i32 weakStatus = 0;
		f32 evade = 0;
		bool selfCentered = false;
		f32 share = 1;
		eastl::fixed_vector<i32,4,false> selfStatuses;
		eastl::fixed_vector<i32,4,false> allyStatuses;
		eastl::fixed_vector<MonsterFire,24,false> fires;
		f32 dashDist = 0;
		f32 dashTime = 0;
		eastl::fixed_vector<i32,4,false> dashPassAnims;
		eastl::fixed_vector<eastl::pair<i32,i32>,4,false> passes;
	};

	struct MonsterAi
	{
		eastl::fixed_string<char,64,true> className;
		f32 walk = 0;
		f32 run = 0;
		f32 perception = 0;
		f32 lost = 0;
		f32 destroy = 0;
		f32 radius = 0;
		f32 returnSpeedRate = 1;
		f32 bodyBlockRadius = 0;
		u8 sizeRank = 0;
		i32 density = 0;
		f32 bodyBlockHeight = 0;
		f32 movingBodyRadius = 0;
		f32 movingBodyHeight = 0;
		eastl::fixed_vector<i32,8,false> spawnStatuses;
		eastl::fixed_vector<i32,4,false> dieRemotes;
		eastl::fixed_vector<i32,4,false> spawnRemotes;
		eastl::fixed_vector<i32,4,false> lowHpRemoveRemotes;
		struct StateStatus { ActionStateID state; i32 statusID; bool remove; };
		eastl::fixed_vector<StateStatus,8,false> stateStatuses;
		struct StateAnim { ActionStateID state; i32 animID; };
		eastl::fixed_vector<StateAnim,8,false> stateAnims;
		u32 tagKey = 0;
		ActionStateID idle = ActionStateID::INVALID;
		ActionStateID awake = ActionStateID::INVALID;
		f32 awakeTime = 0;
		f32 groggyChance = 0;
		f32 groggyTime = 0;
		f32 spawnTime = 0;
		f32 chaseRing = 0;
		f32 chaseArc = 0;
		bool turnLock = false;
		f32 turnEvery = 0;
		f32 retargetGain = 0;
		f32 retargetRate = 0;
		i32 masterMark = 0;
		i32 teamBuff = 0;
		bool playerSide = false;
		bool mineral = false;
		eastl::array<ActionStateID,2> victoryStates = {{ ActionStateID::INVALID, ActionStateID::INVALID }};
		eastl::array<f32,2> victoryTimes = {{ 0, 0 }};
		eastl::fixed_vector<i32,4,false> victoryStatuses;
		f32 victoryStatusDelay = 0;
		f32 victoryHeal = 0;
		eastl::fixed_vector<eastl::pair<f32,i32>,8,false> hpBuffs;
		f32 hpBuffSpeedStep = 0;
		i32 ccArmorStatus = 0;
		f32 dieStand = 0;
		eastl::fixed_vector<i32,5,false> allyBuffs;
		f32 allyBuffRadius = 0;
		f32 allyBuffInterval = 0;
		eastl::fixed_vector<i32,4,false> groggyStatuses;
		eastl::fixed_vector<StatusType,8,false> ccTypes;
		eastl::fixed_vector<u32,10,false> enemyTags;
		eastl::array<const MonsterAttack*,MonsterAttack::_COUNT> byTarget = {};
		eastl::fixed_vector<MonsterAttack,24,false> attacks;
		struct ScriptStep { f32 t = 0; eastl::fixed_vector<u8,4,false> attacks; u8 pick = 0; f32 hpMin = 0; f32 hpMax = 2.0f; i32 aiTrigger = -1; };
		f32 scriptStart = 0;
		f32 scriptPeriod = 0;
		ActionStateID scriptPause = ActionStateID::INVALID;
		eastl::fixed_vector<ScriptStep,12,false> script;
		eastl::fixed_vector<i32,8,false> seekDocs;
		eastl::fixed_vector<i32,4,false> hideStatuses;
		bool reactionImmune = false;
		eastl::fixed_vector<i32,4,false> wakeStatuses;
		f32 seekReach = 0;
		ActionStateID walkState = ActionStateID::INVALID;
		ActionStateID preAttack = ActionStateID::INVALID;
		eastl::fixed_vector<i32,4,false> walkStatuses;
		eastl::fixed_vector<MonsterFire,4,false> walkFires;
		f32 lowHpRatio = 0;
		ActionStateID lowHpState = ActionStateID::INVALID;
		f32 lowHpTime = 0;
		eastl::fixed_vector<i32,4,false> lowHpStatuses;
		bool Hates(u32 tag) const { return eastl::find(enemyTags.begin(), enemyTags.end(), tag) != enemyTags.end(); }
	};

	eastl::hash_map<i32, MonsterAi> monsterAi;

	struct IngameLevelInfo
	{
		struct MonsterExp { enum Mode: u8 { SHARE, TEAM, KILLER }; Mode mode = SHARE; i32 exp = 0; i32 growth = 0; f32 growthPeriod = 0; i32 points = 0; i32 killerBonus = 0; };
		i32 maxLevel = 1;
		i32 passiveExp = 0;
		f32 passivePeriod = 0;
		f32 shareRadius = 0;
		i32 propertyPointsEvery = 0;
		i32 propertyPointsStep = 0;
		f32 respawnBaseMs = 0;
		f32 respawnPerLevelMs = 0;
		f32 respawnPerSecMs = 0;
		i32 killPoints = 1;
		f32 masterKillBase = 0;
		f32 masterKillPerLevel = 0;
		f32 masterKillAssistPool = 0;
		f32 masterKillAssistWindowSec = 0;
		eastl::fixed_vector<i32,32,false> levelExp;
		eastl::hash_map<i32, MonsterExp> monsters;
		eastl::fixed_vector<eastl::pair<f32,f32>,8,false> shares;
		i32 ExpToLevelUp(i32 level) const { return level >= 1 && level <= (i32)levelExp.size() ? levelExp[level - 1] : 0; }
	};
	IngameLevelInfo ingameLevel;
	bool LoadIngameLevel();

	struct MasterGrowthStat { u8 type = 0; f32 linear = 0; f32 quadratic = 0; };
	eastl::hash_map<i32, eastl::fixed_vector<MasterGrowthStat,4,false>> masterGrowth;
	bool LoadMasterGrowth();

	eastl::hash_map<i32, eastl::array<eastl::fixed_vector<StatValue,4,false>,4>> masterTraits;
	i32 traitMaxPoints = 12;
	bool LoadMasterTraits();
	f32 MasterTraitBonus(ClassType classType, const eastl::array<u8,4>& levels, u8 statType) const
	{
		auto f = masterTraits.find((i32)classType);
		if(f == masterTraits.end()) return 0;
		f32 sum = 0;
		for(i32 i = 0; i < 4; i++) {
			if(levels[i] == 0) continue;
			for(const StatValue& st : f->second[i]) if(st.type == statType) sum += st.value * levels[i];
		}
		return sum;
	}
	bool MasterHasTrait(ClassType classType, i32 index) const
	{
		auto f = masterTraits.find((i32)classType);
		return f != masterTraits.end() && index >= 0 && index < 4 && !f->second[index].empty();
	}
	f32 MasterLevelBonus(ClassType classType, u8 statType, i32 level) const
	{
		auto f = masterGrowth.find((i32)classType);
		if(f == masterGrowth.end() || level <= 1) return 0;
		for(const MasterGrowthStat& g : f->second) {
			if(g.type != statType) continue;
			f32 best = 0;
			for(i32 l = 2; l <= level; l++) {
				const f32 x = (f32)(l - 1);
				best = MAX(best, g.linear * x + g.quadratic * x * x);
			}
			return best;
		}
		return 0;
	}
	const MonsterAi* FindMonsterAi(i32 creatureID) const { auto f = monsterAi.find(creatureID); return f == monsterAi.end() ? nullptr : &f->second; }
	bool LoadMonsterAi();
	void LoadMonsterScripts();
	void LoadMonsterFires();
	const eastl::fixed_vector<StatValue,24,false>* FindMonsterStats(i32 creatureID) const { auto f = monsterStats.find(creatureID); return f == monsterStats.end() ? nullptr : &f->second; }
	eastl::hash_map<i32, eastl::fixed_vector<StatValue,24,false>> monsterStats;
	eastl::hash_map<i32, eastl::fixed_vector<StatValue,4,false>> monsterGrowth;
	eastl::hash_map<i32, i32> monsterDrop;
	i32 FindMonsterDrop(i32 creatureID) const { auto f = monsterDrop.find(creatureID); return f == monsterDrop.end() ? 0 : f->second; }
	eastl::hash_map<i32, u8> monsterRank;
	eastl::fixed_vector<eastl::fixed_string<char,32,false>,32,false> rankNames;
	u8 RankID(const char* name);
	u8 FindMonsterRank(i32 creatureID) const { auto f = monsterRank.find(creatureID); return f == monsterRank.end() ? NO_RANK : f->second; }
	static const u8 NO_RANK = 0xff;
	bool LoadMonsterRanks();
	struct GameEvent
	{
		i32 index = 0;
		i32 expBonusPercent = 0;
		i32 ronBonusPercent = 0;
		eastl::fixed_vector<GameType,8,false> gameTypes;
		eastl::fixed_vector<i32,32,true> stages;
	};
	eastl::fixed_vector<GameEvent,16,false> activeGameEvents;
	bool LoadGameEvents();
	void ActiveEventBonus(GameType type, StageIndex stage, i32* expPercent, i32* ronPercent) const;
	eastl::hash_map<i32, eastl::fixed_vector<i32,4,false>> monsterDieSpawn;
	eastl::hash_map<i32, f32> monsterLifeTime;
	const eastl::fixed_vector<i32,4,false>* FindMonsterDieSpawn(i32 creatureID) const { auto f = monsterDieSpawn.find(creatureID); return f == monsterDieSpawn.end() ? nullptr : &f->second; }
	struct DeathRemote { i32 remote = 0; f32 delay = 0; f32 radius = 0; f32 multiplier = 0; eastl::fixed_vector<i32,4,false> statuses; bool ally = false; bool pc = false; bool npc = false; };
	eastl::hash_map<i32, eastl::fixed_vector<DeathRemote,4,false>> deathRemotes;
	const eastl::fixed_vector<DeathRemote,4,false>* FindDeathRemotes(i32 creatureID) const { auto f = deathRemotes.find(creatureID); return f == deathRemotes.end() ? nullptr : &f->second; }
	bool LoadDeathRemotes();
	struct DynamicTrapTrack { eastl::vector<vec3> points; eastl::vector<vec2> dirs; };
	struct DynamicTrapRemote { const Remote* remote = nullptr; u8 node = 0; f32 delay = 0; f32 life = 0; };
	struct DynamicTrapNode { eastl::fixed_string<wchar,48,true> name; i32 track = -1; bool vsPlayer = false; bool vsMonster = false; };
	struct DynamicTrapState
	{
		ActionStateID action = ActionStateID::INVALID;
		f32 length = 0;
		bool loop = false;
		eastl::fixed_vector<DynamicTrapNode,8,false> nodes;
		eastl::fixed_vector<DynamicTrapRemote,16,false> remotes;
		eastl::fixed_vector<i32,8,false> removes;
	};
	struct DynamicTrapModel
	{
		eastl::fixed_string<char,64,true> kfm;
		eastl::vector<DynamicTrapTrack> tracks;
		eastl::fixed_vector<DynamicTrapState,12,false> states;
		const DynamicTrapState* FindState(ActionStateID action) const
		{
			for(i32 i = 0; i < (i32)states.size(); i++) if(states[i].action == action) return &states[i];
			return nullptr;
		}
	};
	struct DynamicTrap { const DynamicTrapModel* model = nullptr; f32 ratio = 1; };
	eastl::vector<DynamicTrapModel> dynamicTrapModels;
	eastl::hash_map<i32, DynamicTrap> dynamicTraps;
	f32 dynamicTrapBaseAttack = 0;
	f32 dynamicTrapSpread = 0;
	f32 dynamicTrapStep = 0;
	const DynamicTrap* FindDynamicTrap(i32 docID) const { auto f = dynamicTraps.find(docID); return f == dynamicTraps.end() ? nullptr : &f->second; }
	bool LoadDynamicTraps();
	f32 FindMonsterLifeTime(i32 creatureID) const { auto f = monsterLifeTime.find(creatureID); return f == monsterLifeTime.end() ? 0.0f : f->second; }
	f32 monsterGrowthPeriod = 0;
	i32 monsterGrowthCap = 0;
	struct TitanForm
	{
		i32 doc = 0;
		f32 duration = 0;
		i32 weapon = 0;
		i32 skill1 = 0;
		i32 skill2 = 0;
		i32 ug = 0;
		f32 moveSpeed = 0;
		eastl::fixed_vector<StatValue,32,false> stats;
		f32 Stat(u8 type) const { for(const StatValue& st : stats) if(st.type == type) return st.value; return 0; }
	};
	struct TitanInfo
	{
		i32 cost = 0;
		f32 votePeriod = 0;
		f32 teamCoolTime = 0;
		i32 failPlayerCool = 0;
		i32 endTeamCool = 0;
		i32 endPlayerCool = 0;
		eastl::fixed_vector<i32,4,false> giants;
		eastl::fixed_vector<TitanForm,4,false> forms;
	};
	TitanInfo titan;
	bool LoadTitan();
	f32 MonsterGrowth(i32 creatureID, u8 type) const
	{
		auto f = monsterGrowth.find(creatureID);
		if(f == monsterGrowth.end()) return 0;
		for(const StatValue& st : f->second) if(st.type == type) return st.value;
		return 0;
	}
	const Song* FindJukeboxSongByID(SongID songID) const;
	bool LoadCharacterBaseStats();

	const Master& GetMaster(ClassType classType) const;
	eastl::hash_map<i32,u16> firingCreators;
	u16 FindFiringCreator(i32 fm) const { auto f = firingCreators.find(fm); return f == firingCreators.end() ? 0 : f->second; }
	i32 GetFirstComboSkill(i32 baseSkill) const { auto f = firstComboSkill.find(baseSkill); return f == firstComboSkill.end() ? 0 : f->second; }
	i32 GetNextComboSkill(i32 skillID) const { auto f = nextComboSkill.find(skillID); return f == nextComboSkill.end() ? 0 : f->second; }
	typedef eastl::fixed_vector<ActionStateID,4,false> RandomActionList;
	const RandomActionList* FindSkillRandomActions(i32 skillID) const { auto f = skillRandomActions.find(skillID); return f == skillRandomActions.end() ? nullptr : &f->second; }
	i32 WeaponTypeOf(ClassType classType, WeaponIndex weaponIndex) const;
	const Action& GetSkillAction(ClassType classType, ActionStateID actionID, i8 stance = 0) const;
	const Action* FindSkillAction(ClassType classType, ActionStateID actionID, i8 stance = 0) const;
	bool SkillActionKnown(ClassType classType, ActionStateID actionID, i8 stance = 0) const;
	bool CheckSkillActions();
	eastl::hash_set<u64> skillActionGaps;
	struct SkillInput { bool needsClick = false; bool directionOnly = false; bool friendlyOnly = false; bool positionDirection = false; bool selfPosition = false; bool teleportEntity = false; bool allEnemies = false; bool pickedEntity = false; bool lockOn = false; bool minHpFriend = false; bool takesTarget = false; bool petCaster = false; i32 checkStatus = 0; u8 obstructMove = 0; bool obstructSight = false; i32 coolTimeID = 0; i32 resetCoolTimeID = 0; f32 coolTime = 0; f32 globalCoolTime = 0; f32 minDistance = 0; f32 maxDistance = 0; };
	const SkillInput* FindSkillInput(i32 skillID) const { auto f = skillInput.find(skillID); return f == skillInput.end() ? nullptr : &f->second; }
	struct SkillCast { enum Type: u8 { DELAY, LIMIT, CHARGING } type = DELAY; f32 time = 0; ActionStateID action = ActionStateID::INVALID; bool interruptByMove = false; bool interruptByLostTarget = false; bool interruptByAttacked = false; bool interruptByOtherSkill = false; f32 maxChargingTime = 0; f32 minChargeRatio = 1; f32 maxChargeRatio = 1; };
	const SkillCast* FindSkillCast(i32 skillID) const { auto f = skillCast.find(skillID); return f == skillCast.end() ? nullptr : &f->second; }
	bool IsCastAction(ActionStateID action) const { foreach_const(it, skillCast) if(it->second.action == action) return true; return false; }
	struct SkillToggle { enum Consume: u8 { NONE, MANA, RAGE, STAMINA } consume = NONE; f32 interval = 0; f32 value = 0; eastl::fixed_vector<i32,4,false> remotes; eastl::fixed_vector<i32,6,false> statuses; };
	const SkillToggle* FindSkillToggle(i32 skillID) const { auto f = skillToggle.find(skillID); return f == skillToggle.end() ? nullptr : &f->second; }
	struct SkillStance { i8 stance = 0; f32 duration = 0; i32 weaponFireCount = 0; bool cancelByDefeat = false; };
	const SkillStance* FindSkillStance(i32 skillID) const { auto f = skillStance.find(skillID); return f == skillStance.end() ? nullptr : &f->second; }
	static i8 StanceFromString(const char* str);
	const Remote& GetRemote(RemoteIdx remoteID) const;
	const Remote* FindRemote(RemoteIdx remoteID) const;
	const Remote* FindCasterMoveBoundRemote(i32 skillIndex) const;
	eastl::fixed_vector<eastl::fixed_string<wchar,48,false>,512,true> nodeNames;
	u16 InternNodeName(const char* name);
	u16 FindNodeName(const char* name) const;
	const wchar* GetNodeName(u16 idx) const { return idx < nodeNames.size() ? nodeNames[idx].c_str() : L"Scene Root"; }
	const StatusInfo* FindStatus(i32 statusID) const;
	i32 FindSkillStatusOfType(i32 skillIndex, StatusType type) const { foreach_const(it, statusMap) { if(it->second.skillIndex == skillIndex && it->second.type == type) return it->first; } return 0; }
	f32 GetWeaponFireDelay(i32 weaponID) const { auto f = weaponFireDelay.find(weaponID); return f == weaponFireDelay.end() ? 0 : f->second; }
	f32 GetSkillCoolTime(i32 skill) const { auto f = skillCoolTime.find(skill); return f == skillCoolTime.end() ? 0 : f->second; }
	f32 GetSkillTargetRange(i32 skill) const { auto f = skillTargetRangeX.find(skill); return f == skillTargetRangeX.end() ? 0 : f->second; }
	f32 GetSkillRange(i32 skill) const { auto f = skillRangeX.find(skill); return f == skillRangeX.end() ? 0 : f->second; }
	f32 GetBotSkillMaxDistance(i32 skill) const { auto f = botSkillMaxDistance.find(skill); return f == botSkillMaxDistance.end() ? 0 : f->second; }
	f32 BotNormalSkillCoolTime() const { return botNormalSkillCoolTime; }
	f32 BotUgSkillCoolTime() const { return botUgSkillCoolTime; }
	f32 BotAfterSkillDelay() const { return botAfterSkillDelay; }
	f32 BotConstant(const char* key) const { auto f = botConstants.find(StatusTypeKey(key)); return f == botConstants.end() ? 0 : f->second; }
	i32 BotReturnSkill() const { return botReturnSkill; }
	i32 BotStageSkillKind(i32 skill) const { for(i32 i = 0; i < (i32)botStageSkills.size(); i++) if(botStageSkills[i] == skill) return i; return -1; }
	struct MeasuredFireNode { f32 forward = 0; f32 side = 0; f32 up = 0; bool hasChild = false; f32 childForward = 0; f32 childSide = 0; f32 childUp = 0; f32 childRot = 0; f32 childDirZ = 0; };
	struct FireNodePath { f32 step = 0; eastl::vector<vec3> points; };
	eastl::vector<FireNodePath> fireNodePaths;
	eastl::hash_map<i32, i32> fireNodePathByRemote;
	const FireNodePath* FindFireNodePath(RemoteIdx remote) const { auto f = fireNodePathByRemote.find((i32)remote); return f == fireNodePathByRemote.end() ? nullptr : &fireNodePaths[f->second]; }
	eastl::hash_map<u64, MeasuredFireNode> measuredFireNodes;
	static u64 FireNodeKey(i32 skill, i32 remote, i32 index) { return ((u64)(u32)skill << 32) ^ ((u64)(u32)remote << 4) ^ (u64)(u32)index; }
	const MeasuredFireNode* FindMeasuredFireNode(i32 skill, i32 remote, i32 index) const { auto f = measuredFireNodes.find(FireNodeKey(skill, remote, index)); return f == measuredFireNodes.end() ? nullptr : &f->second; }
	struct MeasuredBotGear { i32 classType = 0; f32 attack = 0; f32 defence = 0; f32 health = 0; f32 critical = 0; i32 weaponTier = 1; i32 weapon = 0; };
	eastl::vector<MeasuredBotGear> measuredBotGear;
	struct MeasuredBotTraits { i32 classType = 0; eastl::array<u8,4> levels = {}; };
	eastl::vector<MeasuredBotTraits> measuredBotTraits;
	const MeasuredBotTraits* PickMeasuredBotTraits(ClassType classType, u32 roll) const
	{
		u32 count = 0;
		foreach_const(t, measuredBotTraits) if(t->classType == (i32)classType) count++;
		if(count == 0) return nullptr;
		u32 pick = roll % count;
		foreach_const(t, measuredBotTraits) if(t->classType == (i32)classType && pick-- == 0) return &*t;
		return nullptr;
	}
	const MeasuredBotGear* PickMeasuredBotGear(ClassType classType, u32 roll) const
	{
		u32 count = 0;
		foreach_const(g, measuredBotGear) if(g->classType == (i32)classType) count++;
		if(count == 0) return measuredBotGear.empty() ? nullptr : &measuredBotGear[roll % measuredBotGear.size()];
		u32 pick = roll % count;
		foreach_const(g, measuredBotGear) if(g->classType == (i32)classType && pick-- == 0) return &*g;
		return nullptr;
	}
	struct MeasuredSwingNode { f32 forward = 0; f32 side = 0; };
	eastl::hash_map<i32, MeasuredSwingNode> measuredSwingNodes;
	const MeasuredSwingNode* FindMeasuredSwingNode(RemoteIdx remote) const { auto f = measuredSwingNodes.find((i32)remote); return f == measuredSwingNodes.end() ? nullptr : &f->second; }
	const eastl::array<i32,5>* FindBotSkills(i32 creature) const { auto f = botSkills.find(creature); return f == botSkills.end() ? nullptr : &f->second; }
	bool IsChildOnExisting(ClassType c, ActionStateID a) const { return childOnExisting.find(((u32)c << 16) | ((u32)a & 0xFFFF)) != childOnExisting.end(); }
	i32 GetChildSkill(ClassType c, ActionStateID a) const { auto f = childSkill.find(((u32)c << 16) | ((u32)a & 0xFFFF)); return f == childSkill.end() ? 0 : f->second; }
	const WeaponFiring* FindWeaponFiring(i32 weaponID) const { auto f = weaponFiring.find(weaponID); return f == weaponFiring.end() ? nullptr : &f->second; }
	const WeaponFiring* FindWeaponFiring(i32 weaponID, i8 stance) const { if(stance > 0) { auto f = weaponStanceFiring.find(((u32)weaponID << 4) | (u32)stance); if(f != weaponStanceFiring.end()) return &f->second; } return FindWeaponFiring(weaponID); }
	f32 GetWeaponStanceFireDelay(i32 weaponID, i8 stance) const { auto f = weaponStanceFireDelay.find(((u32)weaponID << 4) | (u32)stance); return f == weaponStanceFireDelay.end() ? 0.0f : f->second; }
	bool IsRepeatHitReduced(i32 skillID) const { return repeatHitReduced.find(skillID) != repeatHitReduced.end(); }
	f32 GetWeaponAttack(i32 weaponID) const { auto f = weaponAttack.find(weaponID); return f == weaponAttack.end() ? 0.0f : f->second; }
	const eastl::fixed_vector<i32,4,false>* FindPassiveStatuses(ClassType c) const { auto f = passiveStatuses.find((i32)c); return f == passiveStatuses.end() ? nullptr : &f->second; }
	const WeaponHeatInfo* FindWeaponHeat(i32 weaponID) const { auto f = weaponHeat.find(weaponID); return f == weaponHeat.end() ? nullptr : &f->second; }
	f32 GetSkillBaseDamage(i32 skillID) const { auto f = skillBaseDamage.find(skillID); return f == skillBaseDamage.end() ? 0 : f->second; }
	f32 GetSkillBasePowerMul(i32 skillID) const { auto f = skillBasePowerMul.find(skillID); if(f != skillBasePowerMul.end()) return f->second; const i32 combo = GetComboBaseSkill(skillID); return combo != 0 && combo != skillID ? GetSkillBasePowerMul(combo) : 0.0f; }
	struct StatusReplace {
		i32 statusID = 0;
		i32 replaceID = 0;
		f32 durationSec = 0;
		f32 distanceRatio = 0;
	};
	typedef eastl::fixed_vector<StatusReplace,8,false> StatusReplaceList;
	struct CostLevel {
		f32 consume[3] = { 0, 0, 0 };
		ActionStateID action = ActionStateID::INVALID;
		eastl::fixed_vector<ActionStateID,4,false> randomActions;
		eastl::fixed_vector<eastl::pair<i32,i32>,8,false> remoteReplace;
		StatusReplaceList statusReplace;
		eastl::fixed_vector<eastl::pair<i32,const Remote*>,4,false> remoteResize;
		f32 damageRatio = 1.0f;
	};
	eastl::hash_map<i32, eastl::fixed_vector<CostLevel,4,false>> skillCostLevels;
	struct SkillUpgrade {
		enum class Type: u8 { COST, DAMAGE, EFFECT, RANGE };
		i32 propertyID = 0;
		Type type = Type::COST;
		f32 baseDamage = -1;
		f32 attackMultiplier = 0;
		eastl::fixed_vector<eastl::pair<f32,f32>,4,false> costLevelDamage;
		struct CostLists {
			eastl::fixed_vector<eastl::pair<i32,i32>,8,false> remoteReplace;
			StatusReplaceList statusReplace;
			eastl::fixed_vector<eastl::pair<i32,const Remote*>,4,false> remoteResize;
		};
		eastl::fixed_vector<CostLists,4,false> costLevelLists;
		f32 consumeMP = -1;
		eastl::fixed_vector<eastl::pair<i32,i32>,8,false> remoteReplace;
		StatusReplaceList statusReplace;
		eastl::fixed_vector<eastl::pair<i32,const Remote*>,4,false> remoteResize;
		ActionStateID action = ActionStateID::INVALID;
		eastl::fixed_vector<ActionStateID,4,false> randomActions;
		i32 nextCombo = 0;
		i32 summon = 0;
		eastl::array<f32,5> consume = { -1, -1, -1, -1, -1 };
		f32 stanceDuration = -1;
		f32 toggleValue = -1;
		f32 targetMaxDistance = -1;
		f32 coolTime = -1;
		i32 weaponFireCount = -1;
	};
	typedef eastl::fixed_vector<SkillUpgrade,8,false> SkillUpgradeList;
	eastl::hash_map<i32, Remote> remoteVariants;
	eastl::hash_map<i32, SkillUpgradeList> skillUpgrades;
	const SkillUpgradeList* FindSkillUpgrades(i32 skillID) const { auto f = skillUpgrades.find(skillID); return f == skillUpgrades.end() ? nullptr : &f->second; }
	eastl::hash_map<i32, eastl::vector<StatusReplaceList>> propertyStatusChain;
	i32 LevelStatus(i32 skillID, i32 level, i32 statusID) const {
		auto f = propertyStatusChain.find(skillID);
		if(f == propertyStatusChain.end() || f->second.empty() || level < 2) return statusID;
		foreach_const(sr, f->second[MIN(level - 2, (i32)f->second.size() - 1)]) if(sr->statusID == statusID) return sr->replaceID;
		return statusID;
	}
	const eastl::fixed_vector<CostLevel,4,false>* FindSkillCostLevels(i32 skillID) const { auto f = skillCostLevels.find(skillID); return f == skillCostLevels.end() ? nullptr : &f->second; }
	f32 GetSkillConsume(i32 skillID, i32 index) const { auto f = skillConsumeRes.find(skillID); return f == skillConsumeRes.end() ? 0 : f->second[index]; }
	eastl::hash_map<i32, i32> comboAutoCastMs;
	i32 GetComboAutoCastMs(i32 skillID) const { auto f = comboAutoCastMs.find(skillID); return f == comboAutoCastMs.end() ? 0 : f->second; }
	eastl::hash_map<i32, i32> comboBotAutoCastMs;
	i32 GetComboBotAutoCastMs(i32 skillID) const { auto f = comboBotAutoCastMs.find(skillID); return f == comboBotAutoCastMs.end() ? 0 : f->second; }
	eastl::hash_map<i32, i32> comboDurationMs;
	i32 GetComboDurationMs(i32 skillID) const { auto f = comboDurationMs.find(skillID); return f == comboDurationMs.end() ? 0 : f->second; }
	eastl::hash_map<i32, i32> hitTargetComboMs;
	i32 GetHitTargetComboMs(i32 skillID) const { auto f = hitTargetComboMs.find(skillID); return f == hitTargetComboMs.end() ? 0 : f->second; }
	bool UsesHitEventTarget(i32 skillID) const { return hitTargetComboMs.find(skillID) != hitTargetComboMs.end(); }
	eastl::hash_map<i32, eastl::fixed_vector<i32,2,false>> comboCleanupStatuses;
	const eastl::fixed_vector<i32,2,false>* FindComboCleanupStatuses(i32 skillID) const { auto f = comboCleanupStatuses.find(skillID); return f == comboCleanupStatuses.end() ? nullptr : &f->second; }
	eastl::hash_map<i32, i32> comboBaseSkill;
	i32 GetComboBaseSkill(i32 skillID) const { auto f = comboBaseSkill.find(skillID); return f == comboBaseSkill.end() ? 0 : f->second; }
	f32 GetSkillConsumeMP(i32 skillID) const { auto f = skillConsumeMP.find(skillID); return f == skillConsumeMP.end() ? 0 : f->second; }
	f32 GetSkillConsumeUG(i32 skillID) const { auto f = skillConsumeUG.find(skillID); return f == skillConsumeUG.end() ? 0 : f->second; }
	f32 GetSkillConsumeStamina(i32 skillID) const { auto f = skillConsumeStamina.find(skillID); return f == skillConsumeStamina.end() ? 0 : f->second; }
	static u64 AniKey(const char* className, i32 seqID)
	{
		const char* key = EA::StdC::Strncmp(className, "CLASS_TYPE_", 11) == 0 ? className + 11 : className;
		u32 h = 2166136261u;
		for(const char* c = key; *c; c++) { h ^= (u8)(*c >= 'a' && *c <= 'z' ? *c - 32 : *c); h *= 16777619u; }
		return ((u64)h << 32) | (u32)seqID;
	}
	struct InteractionInfo { enum Action: u8 { OTHER, TOTEM, OCCUPY }; Action action = OTHER; bool pvpOccupy = false; f32 activeDistance = 0; bool carry = false; bool individual = false; CreatureIndex receiver = CreatureIndex::Invalid; bool active = true; bool repeat = false; f32 repeatInterval = 0; i32 castMs = 0; u32 announcement = 0; i32 points = 0; eastl::array<u32,2> ownerMask = {{ 0, 0 }}; i32 requireMinerals = 0; };
	eastl::hash_map<i32, i32> dynamicInteraction;
	eastl::hash_map<i32, f32> stageTeleportDistance;
	eastl::hash_map<i32, f32> statusShield;
	eastl::hash_map<i32, f32> creatureSight;
	eastl::hash_set<i32> creatureBushPass;
	bool CreatureBushPass(i32 doc) const { return creatureBushPass.count(doc) != 0; }
	eastl::fixed_vector<MasterGrouping,32,false> masterGroupings;
	i32 respawnFlyRemote = 0;
	const MasterGrouping* FindMasterGrouping(i32 id) const { foreach_const(g, masterGroupings) if(g->id == id) return &*g; return nullptr; }
	eastl::hash_map<i32, eastl::fixed_string<char,48,false>> creatureKfm;
	eastl::hash_map<eastl::string, i32> monsterSpawnAnim;
	f32 MonsterSpawnSec(i32 doc) const { auto k = creatureKfm.find(doc); if(k == creatureKfm.end()) return 0.0f; auto a = monsterSpawnAnim.find(eastl::string(k->second.c_str())); return a == monsterSpawnAnim.end() ? 0.0f : GetAniSeqTime(k->second.c_str(), a->second); }
	eastl::hash_map<i32, vec2> bushSize;
	eastl::hash_set<i32> teamGateDocs;
	eastl::hash_set<i32> executeMinionRemotes;
	bool IsTeamGate(i32 doc) const { return teamGateDocs.find(doc) != teamGateDocs.end(); }
	f32 CreatureSight(i32 doc) const { auto f = creatureSight.find(doc); return f == creatureSight.end() ? 0.0f : f->second; }
	const vec2* FindBushSize(i32 doc) const { auto f = bushSize.find(doc); return f == bushSize.end() ? nullptr : &f->second; }
	f32 StatusShieldAmount(const StatusInfo& info) const
	{
		auto f = statusShield.find(info.ID);
		if(f != statusShield.end()) return f->second;
		f32 sum = 0;
		for(const StatusMod& m : info.mods) if(m.stat == StatusMod::SHIELD_DEFENCE && !m.mul) sum += m.value;
		return sum;
	}
	f32 FindStageTeleportDistance(i32 doc) const { auto f = stageTeleportDistance.find(doc); return f == stageTeleportDistance.end() ? -1.0f : f->second; }
	i32 FindDynamicInteraction(i32 doc) const { auto f = dynamicInteraction.find(doc); return f == dynamicInteraction.end() ? 0 : f->second; }
	eastl::hash_map<i32, eastl::array<i32,5>> dynamicDrop;
	i32 FindDynamicDrop(i32 doc, u8 difficultyBit) const;
	eastl::hash_map<i32, InteractionInfo> interactions;
	eastl::hash_map<i32, i32> dynamicDensity;
	eastl::hash_map<i32, i32> dynamicDestroyHits;
	eastl::hash_map<i32, f32> dynamicRespawnSec;
	eastl::hash_set<i32> dynamicJumpable;
	eastl::hash_set<i32> dynamicBlockSight;
	eastl::hash_set<i32> actionRemovedStatuses;
	eastl::hash_map<i32, eastl::vector<vec3>> measuredWalkCells;
	eastl::hash_map<i32, eastl::fixed_string<char,48,false>> dynamicMeshKey;
	struct LevelAITriggerSource { eastl::fixed_string<wchar,128,true> level; i32 node; u8 mask; i32 aiIndex; f32 delay; };
	eastl::vector<LevelAITriggerSource> levelAITriggerSources;
	bool levelAITriggerSourcesLoaded = false;
	const InteractionInfo* FindInteraction(i32 id) const { auto f = interactions.find(id); return f == interactions.end() ? nullptr : &f->second; }
	f32 GetAniSeqTime(const char* className, i32 seqID) const { auto f = aniSeqTime.find(AniKey(className, seqID)); return f == aniSeqTime.end() ? 0 : f->second; }
	struct MoveCurve
	{
		f32 time = 0;
		f32 maxAbs = 0;
		f32 peak = 0;
		eastl::fixed_vector<f32,24,false> h;

		f32 Eval(f32 u) const
		{
			const i32 n = (i32)h.size();
			if(n == 0 || maxAbs <= 0) return 0;
			if(n == 1) return h[0] / peak;
			const f32 x = MAX(MIN(u, 1.0f), 0.0f) * (f32)(n - 1);
			const i32 i = MIN((i32)x, n - 1);
			const i32 j = MIN(i + 1, n - 1);
			return (h[i] + (x - (f32)i) * (h[j] - h[i])) / peak;
		}
	};
	const MoveCurve* GetMoveCurve(const char* className, i32 seqID) const { auto f = moveCurves.find(AniKey(className, seqID)); return f == moveCurves.end() ? nullptr : &f->second; }
	f32 GetLieEndTime(const char* className) const
	{
		const char* key = EA::StdC::Strncmp(className, "CLASS_TYPE_", 11) == 0 ? className + 11 : className;
		foreach_const(it, lieEndTime) {
			if(it->first == key) return it->second;
		}
		return 0;
	}
	f32 GetStageTimeLimit(i32 stageID) const { auto f = stageTimeLimit.find(stageID); return f == stageTimeLimit.end() ? 0 : f->second; }
	i32 GetStageScoreLimit(i32 stageID) const { auto f = stageScoreLimit.find(stageID); return f == stageScoreLimit.end() ? 0 : f->second; }
	const GroundItemInfo* FindGroundItem(i32 id) const { auto f = groundItemMap.find(id); return f == groundItemMap.end() ? nullptr : &f->second; }
	const DropGroup* FindDropGroup(i32 id) const { auto f = dropGroupMap.find(id); return f == dropGroupMap.end() ? nullptr : &f->second; }
	void RollDrop(i32 dropIndex, DropRollList* out, i32 depth = 0) const;
	eastl::hash_map<i32, eastl::fixed_vector<i32,64,true>> gearDocs;
	static i32 GearKey(const char* gearType, const char* rank);
	const SummonInfo* FindSummon(i32 creatureID) const { auto f = summonMap.find(creatureID); return f == summonMap.end() ? nullptr : &f->second; }
	bool CreatureUntargetable(i32 creatureID) const { return creatureUntargetable.find(creatureID) != creatureUntargetable.end(); }
	i32 CreatureDefenceStatus(i32 creatureID) const { auto f = creatureDefenceStatus.find(creatureID); return f == creatureDefenceStatus.end() ? 0 : f->second; }
	f32 CreatureHideDetectRadius(i32 creatureID) const { auto f = creatureHideDetect.find(creatureID); return f == creatureHideDetect.end() ? 0.0f : f->second; }
	Element GetCreatureElement(i32 creatureID) const { auto f = creatureElement.find(creatureID); return f == creatureElement.end() ? Element::NONE : f->second; }
	i32 FindSkillHitEventRemote(i32 skillID) const { auto f = skillHitEventRemote.find(skillID); return f == skillHitEventRemote.end() ? 0 : f->second; }

private:
	bool LoadXMLFile(const wchar* fileName, tinyxml2::XMLDocument& xmlData, bool optional = false);

	bool LoadMasterDefinitions();
	bool LoadEntrySystems();
	bool LoadMissions();
	bool LoadExpInfo();
	bool LoadStageMaps();
	bool LoadBotCreatures();

	bool LoadGuildData();
	bool LoadMasterSkinsDefinitions();
	bool LoadMasterWeaponDefinitions();
	bool LoadMasterDefinitionsModel();
	void LoadAllSkills();
	bool LoadMasterSkillWithID(SkillNormalModel& SkillNormal, i32 skillID);
	bool LoadMasterSkillPropertyWithID(SkillNormalModel& SkillNormal, i32 skillID);
	bool LoadWeaponModelDefinitions();
	void SetValuesSkillNormalLevel(tinyxml2::XMLElement& pNodeCommonSkill, SkillNormalLevelModel& _skillNormalLevelModel);
	void SetWeaponSpecRef(tinyxml2::XMLElement& pNodeWeaponSpecRef, WeaponSpec& _weaponSpec);
	bool LoadMapList();
	bool LoadMapByID(Map* map, MapIndex index);
	bool LoadLobby(MapIndex index);
	bool LoadPvpMaps();
	bool LoadMonsterStats();
	bool LoadJukeboxSongs();
	bool LoadCollisionMeshes();
	bool LoadAnimationData();
	bool LoadRemoteData(); // Any object created by skills (projectiles, explosions, etc): a "remote"

	// helper functions
	CreatureType StringToCreatureType(const char* s);
	EntityType StringToEntityType(const char* s);
	int GetSkillSlotIndex(const char* skillSlot);
	Faction StringToFaction(const char* s);
	GameSubModeType StringToGameSubModeType(const char* s);
	MapType StringToMapType(const char* s);
	SkillType StringToSkillType(const char* s);
	
	tinyxml2::XMLDocument xmlSKILL;
	tinyxml2::XMLDocument xmlSKILLPROPERTY;
	tinyxml2::XMLDocument xmlCREATURECHARACTER;
	tinyxml2::XMLDocument xmlWEAPON;
	tinyxml2::XMLDocument xmlWEAPONTT;

	eastl::fixed_hash_map<RemoteIdx, Remote, 1500> remoteMap;
	eastl::hash_map<i32, StatusInfo> statusMap;
	eastl::hash_map<i32, i32> skillHitEventRemote;
	eastl::hash_map<i32, WeaponHeatInfo> weaponHeat;
	eastl::hash_map<i32, f32> weaponFireDelay;
	eastl::hash_map<i32, WeaponFiring> weaponFiring;
	eastl::hash_map<u32, WeaponFiring> weaponStanceFiring;
	eastl::hash_map<u32, f32> weaponStanceFireDelay;
	eastl::hash_map<i32, f32> weaponAttack;
	eastl::hash_set<i32> repeatHitReduced;
	eastl::hash_map<u32, i32> childSkill;
	eastl::hash_set<u32> childOnExisting;
	eastl::hash_map<i32, eastl::array<i32,5>> botSkills;
	eastl::hash_map<i32, f32> skillCoolTime;
	eastl::hash_map<i32, i32> firstComboSkill;
	eastl::hash_map<i32, i32> nextComboSkill;
	eastl::hash_map<i32, RandomActionList> skillRandomActions;
	eastl::hash_map<i32, SkillStance> skillStance;
	eastl::hash_map<i32, SkillCast> skillCast;
	eastl::hash_map<i32, SkillToggle> skillToggle;
	eastl::hash_map<i32, SkillInput> skillInput;
	eastl::hash_map<i32, f32> botSkillMaxDistance;
	eastl::hash_map<i32, f32> skillRangeX;
	eastl::hash_map<i32, f32> skillTargetRangeX;
	f32 botNormalSkillCoolTime = 0;
	f32 botUgSkillCoolTime = 0;
	f32 botAfterSkillDelay = 0;
	eastl::hash_map<u32, f32> botConstants;
	eastl::array<i32,5> botStageSkills = {{ 0, 0, 0, 0, 0 }};
	i32 botReturnSkill = 0;
	eastl::hash_map<i32, eastl::fixed_vector<i32,4,false>> passiveStatuses;
	eastl::hash_map<i32, f32> skillConsumeStamina;
	eastl::hash_map<i32, f32> skillConsumeMP;
	eastl::hash_map<i32, eastl::array<f32,4>> skillConsumeRes;
	eastl::hash_map<i32, f32> skillBaseDamage;
	eastl::hash_map<i32, f32> skillTooltipDamage;
	eastl::hash_map<i32, f32> skillBasePowerMul;
	eastl::hash_map<i32, f32> skillConsumeUG;
	eastl::hash_map<i32, SummonInfo> summonMap;
	eastl::hash_map<i32, f32> creatureHideDetect;
	eastl::hash_map<i32, i32> creatureDefenceStatus;
	eastl::hash_map<i32, bool> creatureUntargetable;
	eastl::hash_map<i32, GroundItemInfo> groundItemMap;
	eastl::hash_map<i32, DropGroup> dropGroupMap;
	eastl::hash_map<u64, f32> aniSeqTime;
	eastl::hash_map<u64, MoveCurve> moveCurves;
	eastl::hash_map<i32, f32> stageTimeLimit;
	eastl::hash_map<i32, i32> stageScoreLimit;
	eastl::fixed_vector<eastl::pair<eastl::fixed_string<char,48,false>, f32>,128,true> lieEndTime;
	eastl::hash_map<i32, Element> creatureElement;
	bool LoadStatusData();
	bool LoadSkillTooltips();
	bool LoadStatusStatMods();
	bool LoadMeasuredFireNodes();
	bool LoadMeasuredSwingNodes();
	bool LoadMeasuredBotGear();
	bool LoadMeasuredBotTraits();
	bool LoadMeasuredWalkCells();
	bool LoadFireNodePaths();
	bool LoadClientRemoteCreators();
	bool LoadBalanceMultipliers();
	bool LoadSkillCosts();
	bool LoadSkillUpgrades();
	void ParseUpgradeLists(tinyxml2::XMLElement* pLevel, eastl::fixed_vector<eastl::pair<i32,i32>,8,false>& remoteReplace, StatusReplaceList& statusReplace, eastl::fixed_vector<eastl::pair<i32,const Remote*>,4,false>& remoteResize);
	bool LoadCharacterElements();
	bool LoadSummons();
	bool LoadSightData();
	bool LoadMasterGroupings();
	bool LoadFixedData();
	bool LoadTagStatuses();
	void LoadSummonInfo(tinyxml2::XMLElement* pInfo);
	bool LoadStageTimeLimits();
	bool LoadAnimationSequenceTimes();
	bool LoadMoveGraphs();
	bool LoadSummonActions();
	bool LoadSummonBaseStats();
	bool LoadGroundItems();
	bool LoadLevelEvents(Map* map, const wchar* levelDir);
	void AddLevelAITriggerSources(Map* map, const wchar* levelDir);
	bool LoadChildSkills();
	bool LoadWeaponFiring();
	bool LoadSkillTargeting();
	bool LoadInteractions();
	bool LoadInteractionReceivers();
	bool LoadBotBrain();
};

bool GameXmlContentLoad();
const GameXmlContent& GetGameXmlContent();

bool OpenMeshFile(const char* path, MeshFile* out);

constexpr const char* g_ResultErrorString[] = {
	"ERROR_TYPE_SUCCESS",
	"ERROR_TYPE_REQ_CLASS",
	"ERROR_TYPE_REQ_MINLEVEL",
	"ERROR_TYPE_REQ_MONEY",
	"ERROR_TYPE_REQ_HEALTH",
	"ERROR_TYPE_REQ_MANA",
	"ERROR_TYPE_REQ_RAGE",
	"ERROR_TYPE_REQ_ENERGY",
	"ERROR_TYPE_REQ_ULTIMATE",
	"ERROR_TYPE_REQ_BUBBLE",
	"ERROR_TYPE_REQ_STAMINA",
	"ERROR_TYPE_REQ_TEAM_SCORE",
	"ERROR_TYPE_REQ_ITEM",
	"ERROR_TYPE_EQUIPSLOT",
	"ERROR_TYPE_INVENFULL",
	"ERROR_TYPE_CANTUSE",
	"ERROR_TYPE_INVIALD_ACTION_STATE",
	"ERROR_TYPE_INVIALD_ACTION_STATE_",
	"ERROR_TYPE_RUNNING_COOLTIME",
	"ERROR_TYPE_GET_GROUND_ITEM_NOT_E",
	"ERROR_TYPE_GET_GROUND_ITEM_INVAL",
	"ERROR_TYPE_GET_GROUND_ITEM_NOT_L",
	"ERROR_TYPE_GET_GROUND_ITEM_NOT_E",
	"ERROR_TYPE_GET_GROUND_ITEM_NOT_R",
	"ERROR_TYPE_GET_GROUND_ITEM_NOT_R",
	"ERROR_TYPE_GET_GROUND_ITEM_REQUI",
	"ERROR_TYPE_GET_GROUND_ITEM_FAR_A",
	"ERROR_TYPE_INVALID",
	"ERROR_TYPE_INVALID_DATA",
	"ERROR_TYPE_SKILL_NOT_FOUND",
	"ERROR_TYPE_MAXHP",
	"ERROR_TYPE_MAXSP",
	"ERROR_TYPE_MAXST",
	"ERROR_TYPE_MAXUG",
	"ERROR_TYPE_MAXMP",
	"ERROR_TYPE_MAXRP",
	"ERROR_TYPE_MAXEP",
	"ERROR_YOU_DIED",
	"ERROR_TYPE_TOO_FAR_AWAY",
	"ERROR_TYPE_TOO_CLOSE",
	"ERROR_TYPE_NOT_FOUND_TARGET",
	"ERROR_TYPE_TOO_FEW_TARGET",
	"ERROR_TYPE_OBSTRUCTED_TARGET",
	"ERROR_TYPE_OBSTRUCTED_BY_FENCE_T",
	"ERROR_TYPE_INVALID_TARGET_POSITI",
	"ERROR_TYPE_INVALID_TARGET_ENTITY",
	"ERROR_TYPE_SKILL_CANT_USE_SAME_M",
	"ERROR_TYPE_DB_ERROR",
	"ERROR_TYPE_NOT_ENOUGH_BATTERY",
	"ERROR_TYPE_NOT_ENOUGH_BATTERY_SO",
	"ERROR_TYPE_NOT_ENOUGH_BATTERY_ON",
	"ERROR_TYPE_LOGIN_SUCCESS",
	"ERROR_TYPE_LOGIN_DATABASE_FAILUR",
	"ERROR_TYPE_LOGIN_AUTHSVR_FAILURE",
	"ERROR_TYPE_LOGIN_PRESENCE_FAILUR",
	"ERROR_TYPE_LOGIN_WORLDSVR_FAILUR",
	"ERROR_TYPE_LOGIN_ALREADY_LOGIN",
	"ERROR_TYPE_LOGIN_NOT_REGISTERED",
	"ERROR_TYPE_LOGIN_INCORRECT_PASSW",
	"ERROR_TYPE_LOGIN_BLOCK_TEMPORARY",
	"ERROR_TYPE_LOGIN_BLOCK_PERSISTEN",
	"ERROR_TYPE_LOGIN_NOT_SUPPORT_MET",
	"ERROR_TYPE_LOGIN_TIMEOUT",
	"ERROR_TYPE_LOGIN_CHANNEL_ABSENCE",
	"ERROR_TYPE_LOGIN_USER_BLOCKED",
	"ERROR_TYPE_LOGIN_NEED_EMAIL_VALI",
	"ERROR_TYPE_LOGIN_USER_OF_WITHDRA",
	"ERROR_TYPE_LOGIN_TENTATIVE_USER",
	"ERROR_TYPE_LOGIN_DORMANT_USER",
	"ERROR_TYPE_CONNECTION_TIMEOUT",
	"ERROR_TYPE_LOGIN_AUTHSVR_SOCKET_",
	"ERROR_TYPE_LOGIN_GATE_EXPIRED",
	"ERROR_TYPE_LOGIN_AUTHSVR_IN_PROG",
	"ERROR_TYPE_LOGIN_BANNED",
	"ERROR_TYPE_LOGIN_CHANGE_CREDENTI",
	"ERROR_TYPE_LOGIN_INVALID_SECONDP",
	"ERROR_TYPE_LOGIN_TRANSIENT_NP_ER",
	"ERROR_TYPE_LOGIN_NO_SALT_VERIFIE",
	"ERROR_TYPE_LOGIN_HOME_DC_LOGIN_R",
	"ERROR_TYPE_LOGIN_CHANGING_NICKNA",
	"ERROR_TYPE_LOGIN_OTP_MISMATCH",
	"ERROR_TYPE_LOGIN_OTP_NOT_SUPPORT",
	"ERROR_TYPE_LOGIN_OTP_STATUS_INVA",
	"ERROR_SHUTDOWN_CANT_PLAY",
	"ERROR_SHUTDOWN_TIME_60",
	"ERROR_SHUTDOWN_TIME_50",
	"ERROR_SHUTDOWN_TIME_40",
	"ERROR_SHUTDOWN_TIME_30",
	"ERROR_SHUTDOWN_TIME_20",
	"ERROR_SHUTDOWN_TIME_10",
	"ERROR_SHUTDOWN_TIME_NOW",
	"ERROR_TYPE_AUTH_SUCCESS",
	"ERROR_TYPE_AUTH_NOT_HELLO",
	"ERROR_TYPE_AUTH_NOT_LOGIN",
	"ERROR_TYPE_AUTH_NOT_ON_HANDOVER",
	"ERROR_TYPE_AUTH_WRONG_KEY",
	"ERROR_TYPE_AUTH_WRONG_CONNECTION",
	"ERROR_TYPE_AUTH_UNKNOWN",
	"ERROR_TYPE_WEAPON_UPGRADE_ALREAD",
	"ERROR_TYPE_WEAPON_UPGRADE_INVALI",
	"ERROR_TYPE_WEAPON_UPGRADE_NOT_EN",
	"ERROR_TYPE_SKILL_PROPERTY_UPGRAD",
	"ERROR_TYPE_SKILL_SELECT_FAILURE",
	"ERROR_TYPE_TITLE_SELECT_FAIL",
	"ERROR_TYPE_TITLE_SELECT_FAIL_TOO",
	"ERROR_TYPE_TITLE_SELECT_FAIL_ON_",
	"ERROR_TYPE_CHARACTER_UNLOCK_FAIL",
	"ERROR_TYPE_CHARACTER_UNLOCK_NOT_",
	"ERROR_TYPE_CHARACTER_UNLOCK_NOT_",
	"ERROR_TYPE_CHARACTER_UNLOCK_NOT_",
	"ERROR_TYPE_CHARACTER_UNLOCK_TUTO",
	"ERROR_TYPE_USEITEM_CANNOT_USE",
	"ERROR_TYPE_USEITEM_INVALID_ITEM",
	"ERROR_TYPE_USEITEM_INVENTORY_FUL",
	"ERROR_TYPE_RANDOMBOX_NOT_ENOUGH_",
	"ERROR_TYPE_RANDOMBOX_NOT_EMPTY_T",
	"ERROR_TYPE_RANDOMBOX_CANNOT_USE_",
	"ERROR_TYPE_SKINTOKEN_NO_MASTER",
	"ERROR_TYPE_SKINTOKEN_NO_EXCHANGE",
	"ERROR_TYPE_SKINTOKEN_INVENTORY_F",
	"ERROR_TYPE_MASTERTOKEN_HAS_MASTE",
	"ERROR_TYPE_INVENTORY_EXPAND_ALRE",
	"ERROR_TYPE_INVENTORY_EXPAND_NOT_",
	"ERROR_TYPE_INVENTORY_EXPAND_NOT_",
	"ERROR_TYPE_INVENTORY_EXPAND_NOT_",
	"ERROR_TYPE_INVENTORY_EXPAND_UNKN",
	"ERROR_TYPE_TAG_SUCCESS_BY_DEAD",
	"ERROR_TYPE_TAG_SUCCESS_BY_REVIVE",
	"ERROR_TYPE_TAG_SUCCESS_BY_TAG",
	"ERROR_TYPE_TAG_COOLTIME",
	"ERROR_TYPE_TAG_OUTER_NOT_FOUND",
	"ERROR_TYPE_TAG_OUTER_NOT_DEFAULT",
	"ERROR_TYPE_TAG_OUTER_NOT_NONE_BE",
	"ERROR_TYPE_TAG_INNER_NOT_FOUND",
	"ERROR_TYPE_TAG_INNER_DIED",
	"ERROR_TYPE_TAG_INNER_OUT_BEHAVIO",
	"ERROR_TYPE_TAG_TRANSFORMATION",
	"ERROR_TYPE_TAG_TRANSFORMATION_VO",
	"ERROR_TYPE_ITEM_BUY_INVALID_ITEM",
	"ERROR_TYPE_ITEM_BUY_CANNOT_BUY",
	"ERROR_TYPE_ITEM_BUY_INVEN_FULL",
	"ERROR_TYPE_ITEM_BUY_PROGRESS",
	"ERROR_TYPE_ITEM_BUY_NOT_ENOUGH_M",
	"ERROR_TYPE_ITEM_SELL_DONOT_HAVE",
	"ERROR_TYPE_ITEM_SELL_INVALID_ITE",
	"ERROR_TYPE_ITEM_SELL_CANNOT_SELL",
	"ERROR_TYPE_ITEM_SELL_FAIL_TO_DEL",
	"ERROR_TYPE_GAME_START_NOT_OWNER",
	"ERROR_TYPE_GAME_START_ONE_PLAYER",
	"ERROR_TYPE_GAME_START_INVALID_ST",
	"ERROR_TYPE_GAME_START_INVALID_DI",
	"ERROR_TYPE_GAME_START_EMPTY_TEAM",
	"ERROR_TYPE_GAME_START_NOT_SORTIE",
	"ERROR_TYPE_GAME_START_NOT_CLEARE",
	"ERROR_TYPE_GAME_START_NOT_ENOUGH",
	"ERROR_TYPE_GAME_START_NOT_ENOUGH",
	"ERROR_TYPE_GAME_START_NOT_CLEARE",
	"ERROR_TYPE_GAME_START_NOT_ENOUGH",
	"ERROR_TYPE_GAME_START_NO_TICKET",
	"ERROR_TYPE_GAME_START_NOT_AVAILA",
	"ERROR_TYPE_CANT_SPECTATE_BY_AI",
	"ERROR_TYPE_CHAT_INVALID_CHANNEL",
	"ERROR_TYPE_CHAT_CHANNEL_INVITE_N",
	"ERROR_TYPE_CHAT_CHANNEL_INVITE_I",
	"ERROR_TYPE_CHAT_CHANNEL_INVITE_I",
	"ERROR_TYPE_CHAT_CHANNEL_INVITE_Y",
	"ERROR_TYPE_CHAT_CHANNEL_INVITE_R",
	"ERROR_TYPE_CHAT_CHANNEL_INVITE_R",
	"ERROR_TYPE_CHANNEL_NAME_ALREADY_",
	"ERROR_TYPE_CHANNEL_NAME_NOT_EXIS",
	"ERROR_TYPE_CHANNEL_NAME_INVALID",
	"ERROR_TYPE_PARTY_CREATE_ALREADY_",
	"ERROR_TYPE_PARTY_CREATE_NOT_EMPT",
	"ERROR_TYPE_PARTY_CREATE_NOT_EMPT",
	"ERROR_TYPE_PARTY_CREATE_INVALID_CHANNEL", // 174
	"ERROR_TYPE_PARTY_CREATE_PENALTY_TIME", // 175
	"ERROR_TYPE_PARTY_CREATE_CANNOT_C",
	"ERROR_TYPE_PARTY_MODIFY_NOT_OWNE",
	"ERROR_TYPE_PARTY_MODIFY_NOT_AVAI",
	"ERROR_TYPE_PARTY_MODIFY_ALREADY_",
	"ERROR_TYPE_PARTY_MODIFY_INVALID_",
	"ERROR_TYPE_PARTY_MODIFY_LEVEL_LI",
	"ERROR_TYPE_PARTY_MODIFY_NOT_ENOU",
	"ERROR_TYPE_PARTY_MODIFY_NOT_GUIL",
	"ERROR_TYPE_PARTY_MODIFY_GUILD_LE",
	"ERROR_TYPE_PARTY_BREAK_NOT_FOUND",
	"ERROR_TYPE_PARTY_BREAK_NOT_OWNER",
	"ERROR_TYPE_PARTY_BREAK_INVALID_S",
	"ERROR_TYPE_STAGE_SKILL_NOT_AVAIL",
	"ERROR_TYPE_STAGE_SKILL_INVALID_S",
	"ERROR_TYPE_PARTY_INVITE_NOT_OWNE",
	"ERROR_TYPE_PARTY_INVITE_NOT_FOUN",
	"ERROR_TYPE_PARTY_INVITE_NOT_FRIE",
	"ERROR_TYPE_PARTY_INVITE_INVALID_",
	"ERROR_TYPE_PARTY_INVITE_ALREADY_",
	"ERROR_TYPE_PARTY_INVITE_ALREADY_",
	"ERROR_TYPE_PARTY_INVITE_NOT_CLEA",
	"ERROR_TYPE_PARTY_INVITE_NOT_ENOU",
	"ERROR_TYPE_PARTY_INVITE_NOT_ENOU",
	"ERROR_TYPE_PARTY_INVITE_NOT_ENOU",
	"ERROR_TYPE_PARTY_INVITE_NOT_EMPT",
	"ERROR_TYPE_PARTY_INVITE_NOT_ENOU",
	"ERROR_TYPE_PARTY_INVITE_LEVEL_LI",
	"ERROR_TYPE_PARTY_INVITE_NOT_SAME",
	"ERROR_TYPE_PARTY_RECOMMEND_NOT_F",
	"ERROR_TYPE_PARTY_RECOMMEND_NOT_A",
	"ERROR_TYPE_PARTY_RECOMMEND_ALREA",
	"ERROR_TYPE_PARTY_RECOMMEND_NOT_R",
	"ERROR_TYPE_PARTY_JOIN_ALREADY_JO",
	"ERROR_TYPE_PARTY_JOIN_ALREADY_FU",
	"ERROR_TYPE_PARTY_JOIN_NOT_FOUND_",
	"ERROR_TYPE_PARTY_JOIN_NOT_CLEARE",
	"ERROR_TYPE_PARTY_JOIN_NOT_ENOUGH",
	"ERROR_TYPE_PARTY_JOIN_NOT_ENOUGH",
	"ERROR_TYPE_PARTY_JOIN_NOT_EMPTY_",
	"ERROR_TYPE_PARTY_JOIN_PENALTY_TI",
	"ERROR_TYPE_PARTY_JOIN_NOT_SAME_G",
	"ERROR_TYPE_PARTY_ADD_BOT_WRONG_G",
	"ERROR_TYPE_PARTY_ADD_BOT_NOT_AVA",
	"ERROR_TYPE_PARTY_REMOVE_BOT_NOT_",
	"ERROR_TYPE_PARTY_LEAVE_NOT_FOUND",
	"ERROR_TYPE_PARTY_KICK_NOT_OWNER",
	"ERROR_TYPE_PARTY_KICK_INVALID",
	"ERROR_TYPE_GAME_ENQUEUE_NOT_FOUN",
	"ERROR_TYPE_GAME_ENQUEUE_NOT_OWNE",
	"ERROR_TYPE_GAME_ENQUEUE_INVALID_",
	"ERROR_TYPE_GAME_ENQUEUE_NOT_ENOU",
	"ERROR_TYPE_GAME_ENQUEUE_NO_TICKE",
	"ERROR_TYPE_GAME_ENQUEUE_ALREADY_",
	"ERROR_TYPE_GAME_ENQUEUE_NOT_ENOU",
	"ERROR_TYPE_GAME_ENQUEUE_NOT_CLEA",
	"ERROR_TYPE_GAME_ENQUEUE_NOT_ENOU",
	"ERROR_TYPE_GAME_ENQUEUE_DODGE_PE",
	"ERROR_TYPE_GAME_ENQUEUE_TROLL_PE",
	"ERROR_TYPE_GAME_ENQUEUE_TROLL_PE",
	"ERROR_TYPE_GAME_ENQUEUE_NOT_AVAI",
	"ERROR_TYPE_GAME_ENQUEUE_NOT_ENOU",
	"ERROR_TYPE_SORTIE_BREAK_NOT_FOUN",
	"ERROR_TYPE_SORTIE_BREAK_NOT_SOLO",
	"ERROR_TYPE_SORTIE_BREAK_USER_LEF",
	"ERROR_TYPE_SORTIE_BREAK_NO_WORLD",
	"ERROR_TYPE_SORTIE_BREAK_NO_GAME_",
	"ERROR_TYPE_SORTIE_BREAK_FAIL_CRE",
	"ERROR_TYPE_SORTIE_BREAK_DENY_CRE",
	"ERROR_TYPE_SORTIE_BREAK_TIMEOUT",
	"ERROR_TYPE_SORTIE_BREAK_INVALID_",
	"ERROR_TYPE_WORLD_MANAGER_NO_GAME",
	"ERROR_TYPE_WORLD_MANAGER_NO_CHAN",
	"ERROR_TYPE_WORLD_MANAGER_FAIL_CR",
	"ERROR_TYPE_WORLD_MANAGER_DENY_CR",
	"ERROR_TYPE_WORLD_MANAGER_FAIL_PR",
	"ERROR_TYPE_PARTY_SWAP_TEAM_NOT_F",
	"ERROR_TYPE_PARTY_SWAP_TEAM_NOT_O",
	"ERROR_TYPE_PARTY_SWAP_TEAM_INVAL",
	"ERROR_TYPE_CHARACTER_SKIN_SELECT",
	"ERROR_TYPE_CHARACTER_SKIN_SELECT",
	"ERROR_TYPE_CHARACTER_SKIN_SELECT",
	"ERROR_TYPE_CHARACTER_SKIN_UNLOCK",
	"ERROR_TYPE_CHARACTER_SKIN_UNLOCK",
	"ERROR_TYPE_CHARACTER_SKIN_UNLOCK",
	"ERROR_TYPE_CHARACTER_SKIN_UNLOCK",
	"ERROR_TYPE_CHARACTER_SKIN_UNLOCK",
	"ERROR_TYPE_USER_NOT_FOUND",
	"ERROR_TYPE_USER_NOT_FOUND_CHARAC",
	"ERROR_TYPE_USER_NOT_FOUND_SKILL",
	"ERROR_TYPE_USER_NOT_FOUND_WEAPON",
	"ERROR_TYPE_USER_NOT_FOUND_MASTER",
	"ERROR_TYPE_BLOCK_USER_YOURSELF",
	"ERROR_TYPE_BLOCK_USER_NOT_FOUND",
	"ERROR_TYPE_BLOCK_USER_DB_ERROR",
	"ERROR_TYPE_BLOCK_USER_ALREADY_BL",
	"ERROR_TYPE_BLOCK_USER_ALREADY_UN",
	"ERROR_TYPE_BLOCK_USER_NOT_BLOCKE",
	"ERROR_TYPE_BLOCK_USER_FRIEND_REQ",
	"ERROR_TYPE_BLOCK_USER_MAX_LIMIT",
	"ERROR_TYPE_FRIEND_REQUEST_NOT_FO",
	"ERROR_TYPE_FRIEND_REQUEST_YOURSE",
	"ERROR_TYPE_FRIEND_REQUEST_ALREAD",
	"ERROR_TYPE_FRIEND_REQUEST_ALREAD",
	"ERROR_TYPE_FRIEND_REQUEST_DB_ERR",
	"ERROR_TYPE_FRIEND_REQUEST_ALREAD",
	"ERROR_TYPE_FRIEND_REQUEST_OVERFL",
	"ERROR_TYPE_FRIEND_RESPONSE_MY_LI",
	"ERROR_TYPE_FRIEND_RESPONSE_TARGE",
	"ERROR_TYPE_FRIEND_RESPONSE_ALREA",
	"ERROR_TYPE_FRIEND_RESPONSE_NOT_F",
	"ERROR_TYPE_FRIEND_RESPONSE_DB_ER",
	"ERROR_TYPE_FRIEND_REMOVE_NOT_FOU",
	"ERROR_TYPE_FRIEND_REMOVE_DB_ERRO",
	"ERROR_TYPE_FRIEND_FAVORITE_PARAM",
	"ERROR_TYPE_FRIEND_FAVORITE_NOT_F",
	"ERROR_TYPE_FRIEND_FAVORITE_DB_ER",
	"ERROR_TYPE_FRIEND_MUTUALFRIEND_P",
	"ERROR_TYPE_FRIEND_MUTUALFRIEND_D",
	"ERROR_TYPE_WHISPER_SEND_NOT_FOUN",
	"ERROR_TYPE_WHISPER_SEND_CANNOT_W",
	"ERROR_TYPE_WHISPER_SEND_USER_INV",
	"ERROR_TYPE_WHISPER_SEND_USER_OFF",
	"ERROR_TYPE_WHISPER_SEND_BLOCKED",
	"ERROR_TYPE_WHISPER_INVITE_CHANNE",
	"ERROR_TYPE_WHISPER_INVITE_CANNOT",
	"ERROR_TYPE_WHISPER_INVITE_USER_I",
	"ERROR_TYPE_WHISPER_INVITE_USER_O",
	"ERROR_TYPE_REWARD_SELECT_SLOT_AL",
	"ERROR_TYPE_REWARD_SELECT_SLOT_AL",
	"ERROR_TYPE_REWARD_SELECT_SLOT_NO",
	"ERROR_TYPE_REWARD_SELECT_SLOT_OU",
	"ERROR_TYPE_REWARD_SELECT_SLOT_NO",
	"ERROR_TYPE_REWARD_SELECT_SLOT_IN",
	"ERROR_TYPE_REWARD_NOT_REWARD_STA",
	"ERROR_TYPE_REWARD_BUY_CHANCE_NOT",
	"ERROR_TYPE_REWARD_BUY_CHANCE_ALR",
	"ERROR_TYPE_REWARD_BUY_CHANCE_NOT",
	"ERROR_TYPE_REWARD_OPEN_ALL_SLOTS",
	"ERROR_TYPE_REWARD_OPEN_ALL_SLOTS",
	"ERROR_TYPE_REWARD_OPEN_ALL_SLOTS",
	"ERROR_TYPE_REWARD_OPEN_ALL_SLOTS",
	"ERROR_TYPE_REWARD_TURN_END_NOT_F",
	"ERROR_TYPE_REWARD_TURN_END_ALREA",
	"ERROR_TYPE_REWARD_TURN_END_NOT_R",
	"ERROR_TYPE_WAREHOUSE_EXPAND_ALRE",
	"ERROR_TYPE_WAREHOUSE_EXPAND_NOT_",
	"ERROR_TYPE_WAREHOUSE_EXPAND_NOT_",
	"ERROR_TYPE_WAREHOUSE_EXPAND_NOT_",
	"ERROR_TYPE_WAREHOUSE_EXPAND_UNKN",
	"ERROR_TYPE_WAREHOUSE_INTERNAL",
	"ERROR_TYPE_WAREHOUSE_NOT_FOUND",
	"ERROR_TYPE_WAREHOUSE_MERGE_TO_DI",
	"ERROR_TYPE_WAREHOUSE_MERGE_NOT_S",
	"ERROR_TYPE_WAREHOUSE_MERGE_COUNT",
	"ERROR_TYPE_WAREHOUSE_IS_FULL",
	"ERROR_TYPE_MAIL_WRITE_INVALID_NI",
	"ERROR_TYPE_MAIL_WRITE_BLOCKED",
	"ERROR_TYPE_MAIL_GET_ATTACHMENT_N",
	"ERROR_TYPE_MAIL_GET_ATTACHMENT_N",
	"ERROR_TYPE_MAIL_MOVE_ARCHIVE_FAI",
	"ERROR_TYPE_MAIL_MOVE_DELETE_FAIL",
	"ERROR_TYPE_MAIL_MOVE_ARCHIVE_FAI",
	"ERROR_TYPE_MAIL_EMPTY_NICKNAME",
	"ERROR_TYPE_MAIL_EMPTY_SUBJECT",
	"ERROR_TYPE_MAIL_EMPTY_CONTENTS",
	"ERROR_TYPE_MAIL_EXCESS_NICKNAME",
	"ERROR_TYPE_MAIL_EXCESS_SUBJECT",
	"ERROR_TYPE_MAIL_EXCESS_CONTENTS",
	"ERROR_TYPE_MAIL_WRITE_INVAILID_N",
	"ERROR_TYPE_MAIL_SEND_ARCHIVE_FAI",
	"ERROR_TYPE_REQUEST_LEADER_INFO_Y",
	"ERROR_TYPE_REQUEST_LEADER_INFO_N",
	"ERROR_TYPE_REQUEST_LEADER_INFO_D",
	"ERROR_TYPE_FRIEND_COMRADE_YOURSE",
	"ERROR_TYPE_FRIEND_COMRADE_NOT_FO",
	"ERROR_TYPE_FRIEND_COMRADE_NOT_FR",
	"ERROR_TYPE_FRIEND_COMRADE_MAX_LI",
	"ERROR_TYPE_FRIEND_COMRADE_DB_ERR",
	"ERROR_TYPE_CHARACTER_SELECT_NO_C",
	"ERROR_TYPE_CHARACTER_SELECT_ALRE",
	"ERROR_TYPE_CHARACTER_SELECT_ALRE",
	"ERROR_TYPE_CHARACTER_SELECT_INVA",
	"ERROR_TYPE_CHARACTER_SELECT_NOT_",
	"ERROR_TYPE_CHARACTER_SELECT_NOT_",
	"ERROR_TYPE_CHARACTER_SELECT_NOT_",
	"ERROR_TYPE_PVP_ASSIGN_BAN_NOT_BA",
	"ERROR_TYPE_PVP_ASSIGN_BAN_NOT_TE",
	"ERROR_TYPE_PVP_ASSIGN_BAN_INVALI",
	"ERROR_TYPE_PVP_ASSIGN_BAN_NOT_AV",
	"ERROR_TYPE_PVP_ASSIGN_BAN_LIMIT_",
	"ERROR_TYPE_CANT_EQUIP_IN_FAIRPVP",
	"ERROR_TYPE_CANT_UNEQUIP_IN_FAIRP",
	"ERROR_TYPE_NICKNAME_INCLUDES_BAN",
	"ERROR_TYPE_NICKNAME_USES_INVALID",
	"ERROR_TYPE_NICKNAME_USES_INVALID",
	"ERROR_TYPE_NICKNAME_OVERLAP",
	"ERROR_TYPE_NICKNAME_NO_ITEM",
	"ERROR_TYPE_NICKNAME_NOT_FOUND_US",
	"ERROR_TYPE_NICKNAME_PENDING",
	"ERROR_TYPE_NICKNAME_SYSTEM_ERROR",
	"ERROR_TYPE_STATE_INVALID_STATE",
	"ERROR_TYPE_STATE_TOO_OFTEN",
	"ERROR_TYPE_STATUS_MESSAGE_TOO_LO",
	"ERROR_TYPE_STATUS_MESSAGE_TOO_OF",
	"ERROR_TYPE_SET_LEADER_INVALID",
	"ERROR_TYPE_SET_LEADER_TOO_OFTEN",
	"ERROR_TYPE_CANT_EQUIP_SUPPORTKIT",
	"ERROR_TYPE_CANT_EQUIP_COUNT_NOT_",
	"ERROR_TYPE_INVENTORY_NOT_FOUND",
	"ERROR_TYPE_STANCE_REFLESH_SUCCES",
	"ERROR_TYPE_STANCE_REFLESH_NOT_EN",
	"ERROR_TYPE_STANCE_REFLESH_DOES_N",
	"ERROR_TYPE_STANCE_REFLESH_INVALI",
	"ERROR_TYPE_CONNECTION_LOST_GAMES",
	"ERROR_TYPE_ACCOUNTITEM_ALREADY_E",
	"ERROR_TYPE_ACCOUNTITEM_NOT_ACCOU",
	"ERROR_TYPE_MASTERGEAR_GEAR_EQUIP",
	"ERROR_TYPE_MASTERGEAR_NAME_EDIT_",
	"ERROR_TYPE_MASTERGEAR_ADD_NOT_EN",
	"ERROR_TYPE_MASTERGEAR_ADD_NOT_EN",
	"ERROR_TYPE_MASTERGEAR_ADD_NOT_EN",
	"ERROR_TYPE_MASTERGEAR_ADD_COUNT_",
	"ERROR_TYPE_GEAR_UNEQUIP_FAIL_NOT",
	"ERROE_TYPE_CANT_EQUIP_GEAR_NOT_E",
	"ERROR_TYPE_MATERIA_EQUIP_FAIL_NO",
	"ERROR_TYPE_MATERIA_EQUIP_FAIL_NO",
	"ERROR_TYPE_MATERIA_EQUIP_FAIL",
	"ERROR_TYPE_MATERIA_UNEQUIP_FAIL_",
	"ERROR_TYPE_MATERIA_UNEQUIP_FAIL_",
	"ERROR_TYPE_MATERIA_UNEQUIP_FAIL",
	"ERROR_TYPE_COMRADE_EXPAND_NOT_FO",
	"ERROR_TYPE_COMRADE_EXPAND_NOT_EN",
	"ERROR_TYPE_COMRADE_EXPAND_LIMIT_",
	"ERROR_TYPE_AFK_NOT_PLAYING",
	"ERROR_TYPE_AFK_DISCONNECT",
	"ERROR_TYPE_AREA_POPULARITY_INVAL",
	"ERROR_TYPE_TRADE_NOT_ENOUGH_ITEM",
	"ERROR_TYPE_TRADE_NOT_ENOUGH_GOLD",
	"ERROR_TYPE_TRADE_NOT_ENOUGH_SOL",
	"ERROR_TYPE_TRADE_NOT_ENOUGH_CCOI",
	"ERROR_TYPE_TRADE_NOT_ENOUGH_INVE",
	"ERROR_TYPE_TRADE_TRADER_NOT_SPAW",
	"ERROR_TYPE_TRADE_INVALID_DATA",
	"ERROR_TYPE_TRADE_NOT_EMPTY_TEMP_",
	"ERROR_TYPE_REFILL_BATTERY_NOT_FO",
	"ERROR_TYPE_REFILL_BATTERY_NOT_EN",
	"ERROR_TYPE_REFILL_BATTERY_NOT_EX",
	"ERROR_TYPE_FATIGUE_CLEAR_NOT_FOU",
	"ERROR_TYPE_FATIGUE_CLEAR_NOT_ENO",
	"ERROR_TYPE_FATIGUE_CLEAR_NOT_EXH",
	"ERROR_TYPE_FATIGUE_REWARD_PENALT",
	"ERROR_TYPE_RANDOM_MISSION_NOT_EN",
	"ERROR_TYPE_RANDOM_MISSION_NOT_EN",
	"ERROR_TYPE_RANDOM_MISSION_ALREAD",
	"ERROR_TYPE_RANDOM_MISSION_INVALI",
	"ERROR_TYPE_CHANGE_CHARACTER_MODE",
	"ERROR_TYPE_TRANSFORM_CHARACTER_I",
	"ERROR_TYPE_TRANSFORM_CHARACTER_D",
	"ERROR_TYPE_TRANSFORM_CHARACTER_N",
	"ERROR_TYPE_TRANSFORM_ALREADY",
	"ERROR_TYPE_TRANSFORM_ALREADY_IN_",
	"ERROR_TYPE_TRANSFORM_PLAYER_COOL",
	"ERROR_TYPE_FORBIDDEN_WORD",
	"ERROR_TYPE_REPORT_DUPLICATED",
	"ERROR_TYPE_REPORT_USER_NOT_FOUND",
	"ERROR_TYPE_REPORT_NO_TOKEN",
	"ERROR_TYPE_PRAISE_NOT_FOUND",
	"ERROR_TYPE_PRAISE_TIME_OVER",
	"ERROR_TYPE_PRAISE_NOT_FOUND_TARG",
	"ERROR_TYPE_PRAISE_ALREADY",
	"ERROR_TYPE_FOLLOW_USER_CHANNEL_I",
	"ERROR_TYPE_FOLLOW_USER_SAME_CHAN",
	"ERROR_TYPE_FOLLOW_USER_NOT_IN_CH",
	"ERROR_TYPE_FOLLOW_USER_TARGET_NO",
	"ERROR_TYPE_FOLLOW_USER_NOT_IN_ID",
	"ERROR_TYPE_WATCH_NOT_FOUND_GAME",
	"ERROR_TYPE_WATCH_USER_NOT_IN_IDL",
	"ERROR_TYPE_WATCH_INVALID_GAME_TY",
	"ERROR_TYPE_CANT_OBSERVE_BY_AI",
	"ERROR_TYPE_CANT_OBSERVE_NO_SERVE",
	"ERROR_TYPE_TIREDNESS_NOT_AVAILAB",
	"ERROR_TYPE_TIREDNESS_NOT_AVAILAB",
	"ERROR_TYPE_TIREDNESS_NOT_AVAILAB",
	"ERROR_TYPE_TIREDNESS_NOT_AVAILAB",
	"ERROR_TYPE_TIREDNESS_NOT_AVAILAB",
	"ERROR_TYPE_TITAN_ALREADY",
	"ERROR_TYPE_TITAN_NOT_ENOUGH_PART",
	"ERROR_TYPE_TITAN_NOT_AVAILABLE_T",
	"ERROR_TYPE_VOTING_UNABLE_MODE",
	"ERROR_TYPE_VOTING_YET_UNABLE",
	"ERROR_TYPE_VOTING_TEAM_COOLTIME_",
	"ERROR_TYPE_VOTING_TEAM_COOLTIME_",
	"ERROR_TYPE_VOTING_TEAM_COOLTIME_",
	"ERROR_TYPE_VOTING_PLAYER_COOLTIM",
	"ERROR_TYPE_VOTING_NOT_EXIST_VOTE",
	"ERROR_TYPE_VOTING_ALREADY_VOTING",
	"ERROR_TYPE_VOTING_INVALID_VOTING",
	"ERROR_TYPE_VOTING_ALREADY_TRANSF",
	"ERROR_TYPE_VOTING_ALREADY_TRANSF",
	"ERROR_TYPE_VOTING_PLAYER_DEAD",
	"ERROR_TYPE_VOTE_NOT_EXIST_VOTING",
	"ERROR_TYPE_VOTE_NOT_VOTER",
	"ERROR_TYPE_VOTE_ALREADY_VOTE",
	"ERROR_TYPE_VOTING_CANCEL_NOT_EXI",
	"ERROR_TYPE_VOTING_CANCEL_NOT_CAN",
	"ERROR_TYPE_GUILD_ALREADY_JOINED",
	"ERROR_TYPE_GUILD_NOT_JOINED_GUIL",
	"ERROR_TYPE_GUILD_NOT_FOUND_GUILD",
	"ERROR_TYPE_GUILD_CREATE_DUPLICAT",
	"ERROR_TYPE_GUILD_KICKOUT_OTHERS_",
	"ERROR_TYPE_GUILD_NOT_FOUND_INVIT",
	"ERROR_TYPE_GUILD_NO_ROOM_TO_JOIN",
	"ERROR_TYPE_GUILD_DUPLICATED_INVI",
	"ERROR_TYPE_GUILD_NOT_FOUND_INVIT",
	"ERROR_TYPE_GUILD_UNABLE_TO_INVIT",
	"ERROR_TYPE_GUILD_NO_PERMISSION_I",
	"ERROR_TYPE_GUILD_NO_PERMISSION_D",
	"ERROR_TYPE_GUILD_NO_PERMISSION_K",
	"ERROR_TYPE_GUILD_NO_PERMISSION_E",
	"ERROR_TYPE_GUILD_NO_PERMISSION_E",
	"ERROR_TYPE_GUILD_NO_PERMISSION_E",
	"ERROR_TYPE_GUILD_NO_PERMISSION_A",
	"ERROR_TYPE_GUILD_NO_PERMISSION_E",
	"ERROR_TYPE_GUILD_NO_PERMISSION_S",
	"ERROR_TYPE_GUILD_NO_PERMISSION_D",
	"ERROR_TYPE_GUILD_NO_PERMISSION_C",
	"ERROR_TYPE_GUILD_NO_PERMISSION_C",
	"ERROR_TYPE_GUILD_NO_PERMISSION_A",
	"ERROR_TYPE_GUILD_NO_PERMISSION_M",
	"ERROR_TYPE_GUILD_NO_PERMISSION_D",
	"ERROR_TYPE_GUILD_NO_PERMISSION_B",
	"ERROR_TYPE_GUILD_NO_PERMISSION_E",
	"ERROR_TYPE_GUILD_NOT_ENOUGH_GOLD",
	"ERROR_TYPE_GUILD_NOT_ALLOWED_LEV",
	"ERROR_TYPE_GUILD_ILLEGAL_LETTER_",
	"ERROR_TYPE_GUILD_ILLEGAL_LETTER_",
	"ERROR_TYPE_GUILD_ILLEGAL_LETTER_",
	"ERROR_TYPE_GUILD_ILLEGAL_LETTER_",
	"ERROR_TYPE_GUILD_ILLEGAL_LETTER_",
	"ERROR_TYPE_GUILD_NOT_FOUND_GUILD",
	"ERROR_TYPE_GUILD_INCORRECT_GUILD",
	"ERROR_TYPE_GUILD_DIRTY_GUILDMEMB",
	"ERROR_TYPE_GUILD_TOO_MANY_GUILDM",
	"ERROR_TYPE_GUILD_DUPLICATED_GUIL",
	"ERROR_TYPE_GUILD_NOT_DELETABLE_G",
	"ERROR_TYPE_GUILD_INCORRECT_GUILD",
	"ERROR_TYPE_GUILD_DUPLICATED_JOIN",
	"ERROR_TYPE_GUILD_TOO_MANY_JOINRE",
	"ERROR_TYPE_GUILD_NOT_FOUND_JOINR",
	"ERROR_TYPE_GUILD_WILL_BE_DISSOLV",
	"ERROR_TYPE_GUILD_NOT_DISSOLVING",
	"ERROR_TYPE_GUILD_DISSOLVED",
	"ERROR_TYPE_GUILD_DUPLICATED_SKIL",
	"ERROR_TYPE_GUILD_NOT_FOUND_SKILL",
	"ERROR_TYPE_GUILD_DUPLICATED_SKIL",
	"ERROR_TYPE_GUILD_NOT_ENOUGH_FUND",
	"ERROR_TYPE_GUILD_NOT_SATISFIED_R",
	"ERROR_TYPE_GUILD_LOCKED_GUILDEMB",
	"ERROR_TYPE_GUILD_DUPLICATED_GUIL",
	"ERROR_TYPE_GUILD_TOO_MANY_GUILDE",
	"ERROR_TYPE_GUILD_SERVERNETWORK",
	"ERROR_TYPE_GUILD_MYGUILD_MISMATC",
	"ERROR_TYPE_GUILD_NOT_SUPPORTED",
	"ERROR_TYPE_GUILD_PARAMETER_ERROR",
	"ERROR_TYPE_GUILD_GUILDUSER_DB_ER",
	"ERROR_TYPE_GUILD_GUILDREQUEST_DB",
	"ERROR_TYPE_GUILD_GUILDMEMBERSHIP",
	"ERROR_TYPE_GUILD_GUILD_DB_ERROR",
	"ERROR_TYPE_GUILD_GUILDDATA_DB_ER",
	"ERROR_TYPE_GUILD_RANK_DB_ERROR",
	"ERROR_TYPE_GUILD_GUILDHISTORY_DB",
	"ERROR_TYPE_GUILD_GUILDJOINREQUES",
	"ERROR_TYPE_GUILD_GUILDSKILL_DB_E",
	"ERROR_TYPE_GUILD_WRONG_DISSOLUTI",
	"ERROR_TYPE_GUILD_GUILDEMBLEM_DB_",
	"ERROR_TYPE_GUILD_DB_ERROR",
	"ERROR_TYPE_GUILD_REWARDMAIL_GUIL",
	"ERROR_TYPE_GUIDE_MISSION_CANNOT_",
	"ERROR_TYPE_GUIDE_MISSION_CANNOT_",
	"ERROR_TYPE_GUIDE_MISSION_CANNOT_",
	"ERROR_CANT_DESTROY_INDESTRUCTIBL",
	"ERROR_TYPE_SKILL_INVALID_SLOT",
	"ERROR_TYPE_SKILL_INVALID_INDEX",
	"ERROR_TYPE_SKILL_INVALID_STATE",
	"ERROR_TYPE_SKILL_INVALID_DATA",
	"ERROR_TYPE_SKILL_INVALID_CONDITI",
	"ERROR_TYPE_SKILL_NOT_ENOUGH_RESO",
	"ERROR_TYPE_SKILL_INVALID_STATE_B",
	"ERROR_TYPE_SKILL_COOLTIME_OUT",
	"ERROR_TYPE_SKILL_CAN_NOT_FIND_DA",
	"ERROR_TYPE_SKILL_CAN_NOT_FIND_TA",
	"ERROR_TYPE_SKILL_INVALID_TARGET",
	"ERROR_TYPE_SKILL_INVALID_TARGET_",
	"ERROR_TYPE_SKILL_INVALID_BEHAVIO",
	"ERROR_TYPE_SKILL_INACTIVE_CHARAC",
	"ERROR_TYPE_SKILL_INACTIVE_SKILL",
	"ERROR_TYPE_SKILL_RESERVED_CAST",
	"ERROR_TYPE_SKILL_EXECUTE_TIMEOUT",
	"ERROR_TYPE_SKILL_OTHER_SKILL_ALR",
	"ERROR_TYPE_SKILL_SERVER_INPUT_ST",
	"ERROR_TYPE_SKILL_LOCKED_TARGET",
	"ERROR_TYPE_SKILL_ETC",
	"ERROR_TYPE_ACTIVITY_GAUGE_REWARD",
	"ERROR_TYPE_ACTIVITY_GAUGE_REWARD",
	"ERROR_TYPE_ACTIVITY_GAUGE_REWARD",
	"ERROR_TYPE_UNLOCK_SKILL_NOT_ENOU",
	"ERROR_TYPE_UNLOCK_SKILL_ALREADY_",
	"ERROR_TYPE_UNLOCK_WEAPON_NO_MAST",
	"ERROR_TYPE_UNLOCK_WEAPON_NO_WEAP",
	"ERROR_TYPE_UNLOCK_WEAPON_NOT_ENO",
	"ERROR_TYPE_UNLOCK_WEAPON_NOT_ENO",
	"ERROR_TYPE_UNLOCK_WEAPON_ALREADY",
	"ERROR_TYPE_GIFT_SENDER_LEVEL",
	"ERROR_TYPE_GIFT_RECEIVER_LEVEL",
	"ERROR_TYPE_GIFT_RECEIVER_ADDEDDA",
	"ERROR_TYPE_GIFT_RECEIVER_NONE",
	"ERROR_TYPE_GIFT_RECEIVER_NOT_FOU",
	"ERROR_TYPE_GIFT_RECEIVER_ALREADY",
	"ERROR_TYPE_GIFT_RECEIVER_ALREADY",
	"ERROR_TYPE_GIFT_RECEIVER_ALREADY",
	"ERROR_TYPE_GIFT_RECEIVER_NO_MAST",
	"ERROR_TYPE_GIFT_NOT_GIVABLE",
	"ERROR_TYPE_GIFT_SENDER_NOT_ENOUG",
	"ERROR_TYPE_GIFT_SENDER_RECEIVER_",
	"ERROR_TYPE_CHARACTER_PROPERTY_NO",
	"ERROR_TYPE_CHARACTER_PROPERTY_NO",
	"ERROR_TYPE_CHARACTER_PROPERTY_RE",
	"ERROR_TYPE_CHARACTER_PROPERTY_NO",
	"ERROR_TYPE_CHARACTER_PROPERTY_NO",
	"ERROR_TYPE_CHARACTER_PROPERTY_NO",
	"ERROR_TYPE_CHARACTER_PROPERTY_CO",
	"ERROR_TYPE_JUKEBOX_NOT_IN_TOWN",
	"ERROR_TYPE_JUKEBOX_ALREADY_ENQUE",
	"ERROR_TYPE_JUKEBOX_QUEUE_FULL",
	"ERROR_TYPE_JUKEBOX_NOT_ENOUGH_CO",
	"ERROR_TYPE_COUPON_ALREADY_REGIST",
	"ERROR_TYPE_COUPON_NOT_FOUND",
	"ERROR_TYPE_COUPON_EXCEED_LIMIT",
	"ERROR_TYPE_COUPON_INVALID_EFFECT",
	"ERROR_TYPE_COUPON_REGISTER_FAIL",
	"ERROR_TYPE_USER_REPORT_INVALID_A",
	"ERROR_TYPE_USER_REPORT_NOT_FOUND",
	"ERROR_TYPE_USER_REPORT_ALREADY_R",
	"ERROR_TYPE_RESTART_PVP_INVALID",
	"ERROR_TYPE_RESTART_PVP_NO_ENTRY",
	"ERROR_TYPE_RESTART_PVP_ALREADY_J",
	"ERROR_TYPE_RESTART_PVP_ALREADY_L",
	"ERROR_TYPE_RETURN_TO_CITY_INVALI",
	"ERROR_TYPE_RETURN_TO_CITY_NOT_IN",
	"ERROR_TYPE_NO_PERMISSION_BY_BM",
	"ERROR_TYPE_CAN_NOT_PAUSE_RESUME_",
	"ERROR_TYPE_PAUSE_NOT_GAME_PLAYIN",
	"ERROR_TYPE_PAUSE_ALREADY_PAUSED",
	"ERROR_TYPE_PAUSE_NOT_ENOUGH_PAUS",
	"ERROR_TYPE_PAUSE_INVALID",
	"ERROR_TYPE_RESUME_NOT_GAME_PLAYI",
	"ERROR_TYPE_RESUME_NOT_PAUSED",
	"ERROR_TYPE_RESUME_ALREADY_RESUME",
	"ERROR_TYPE_RESUME_INVALID",
	"UNKNOWN_ERROR_TYPE",
};

constexpr const char* g_ResultErrorInvalidString = "UNKNOWN_ERROR_TYPE";
