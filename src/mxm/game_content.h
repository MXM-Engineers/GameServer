#pragma once
#include "core.h"
#include <tinyxml2.h>
#include <common/protocol.h>
#include <common/utils.h>
#include <common/stat.h>

#include <EASTL/hash_set.h>
#include <EASTL/fixed_list.h>
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

}

struct SummonCommand
{
	enum Kind: u8 { REMOTE = 0, STATUS, DIE };
	Kind kind = REMOTE;
	bool onCurrent = false;
	u8 dieMode = 0;
	i32 index = 0;
	f32 delay = 0;
};

struct SummonInfo
{
	eastl::fixed_string<char,48,false> className;
	eastl::fixed_vector<SummonCommand,12,false> onSpawn;
	eastl::fixed_vector<SummonCommand,12,false> onDie;
	f32 lifeTime = 0;
	f32 moveSpeed = 0;
	f32 perception = 0;
	bool targetable = false;
	eastl::fixed_vector<i32,4,false> skills;
	enum Ai: u8 { AI_NONE = 0, AI_RELAY, AI_CHASE };
	Ai ai = AI_NONE;
	i32 variation = 0;
	f32 aiDistance = 0;
	eastl::array<eastl::fixed_vector<SummonCommand,12,false>,21> skillStates;
	eastl::array<i32,21> stateAnim = {};
};

struct StatusMod
{
	enum Stat: u8 { OTHER = 0, SHIELD_DEFENCE, HEALTH, SKILLPOWER, CRITICALRATE, SKILL_CRITICALRATE, ATTACK, MOVESPEED, REFLECT_DAMAGE_RATIO, DEFENCE };
	Stat stat = OTHER;
	bool regen = false;
	bool mul = false;
	f32 value = 0;
	f32 count = 0;
};

enum class StatusType: u8
{
	OTHER,
	ACTIONBLOCK,
	ACTIVATE_UNDEAD,
	AIRBORNE,
	ARTILLERY,
	DEFEAT_CASTER_AURA,
	DEFENDER,
	DISPEL,
	DRAGGED,
	DRAGGEDBYFORCE,
	ELECTRO,
	EVADE,
	FEAR,
	FLY,
	FORCEBACK,
	GHOSTWIDOW,
	GODMODE,
	GX09,
	HEAL,
	HIDE,
	HIT_PLAGUE,
	HPTRIGGER,
	KNOCKDOWN,
	KROMEDE,
	LINK,
	MANABURN,
	NEARLY_IMMORTAL,
	NOCOST,
	PUSH,
	ROOT,
	SHIELD,
	SHIELDBOMB,
	SHOCKED,
	SKILLSTAMINA,
	SKILLSTATUS,
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
	UNDEAD,
	UNSHAKABLE,
	WEAK_ON_CRITICAL,
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
	f32 distanceRatio = 0;
	f32 timeRatio = 0;
	f32 heightRatio = 0;
	f32 lyingTime = 0;
	f32 dotDamage = 0;
	f32 drainMultiplier = 0;
	i32 skillIndex = 0;
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
	bool isDebuff = false;
	bool canDispel = false;
	i8 dispelEffect = -1;
	i32 auraStatus = 0;
	eastl::fixed_vector<i32,8,false> auraStatuses;
	bool auraAll = false;
	i32 linkCasterStatus = 0;
	i32 linkTargetStatus = 0;
	f32 targetingRange = 0;
	f32 explodeDamage = 0;
	f32 additionalDamageByCaster = 0;
	NextCondition nextCondition = NextCondition::NONE;
	StatusType type = StatusType::OTHER;
	eastl::fixed_string<char,40,false> typeName;

	bool permanent = false;
	i32 customInit = 0;
	i32 customMax = 0;
	i32 hitEffectStatus = 0;
	f32 hitEffectValue = 0;
	f32 costRatioMP = 0;
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
};

struct WeaponFiring
{
	RemoteIdx base = RemoteIdx::INVALID;
	eastl::fixed_vector<RemoteIdx,4,false> combo;
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
	f32 lifeTime = 0;
	i32 activateCount = 1;
	RemoteIdx destroyCreateRemote = RemoteIdx::INVALID;
	f32 attackMultiplier = 0;
	f32 offsetY = 0;
	bool forceTargetDir = false;
	f32 offsetZ = 0;
	f32 distPenaltyRate = 0;
	f32 penaltyApplyDistance = 0;
	f32 boundOffsetY = 0;
	f32 angle = 0;
	f32 hitInvalidTime = 1;
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
	};
	typedef eastl::fixed_vector<SpawnRule,4,false> SpawnRules;
	SpawnRules onDestroy;
	SpawnRules onTime;
	SpawnRules onActivate;
	SpawnRules onHitCharacter;
	f32 timeEventDelay = -1;
	bool timeEventRecycle = false;

	bool contactFriend = false;
	bool vsOwner = false;
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
	f32 criticalRate = 0;
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

struct GameXmlContent
{
	struct Master
	{
		CreatureIndex ID;
		ClassType classType;
		eastl::fixed_string<char,64,false> className;
		eastl::fixed_vector<SkillID,32,false> skillIDs;
		eastl::fixed_vector<u8,32,false> skillUnlocked;
		eastl::fixed_vector<SkinIndex,20,false> skinIDs;
		eastl::fixed_vector<WeaponIndex,32,false> weaponIDs;
		eastl::fixed_vector<WeaponIndex,3,false> defaultWeaponIDs;
		eastl::fixed_vector<WeaponIndex,3,false> fairPvpWeaponIDs;
		eastl::fixed_vector<StatValue,32,false> baseStats;




		CharacterModel character;
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
			i32 entityType = 1;
			i32 spawnAnim = 0;
			i32 wanderDist = -1;
			i32 tagID = -1;
			i32 ownerID = 0;
			u8 dirToNearPC = 0;
			ActionStateID actionState = ActionStateID::INVALID;


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

		eastl::fixed_vector<Spawn,512> creatures;
		eastl::fixed_vector<Spawn,512> dynamic;
		eastl::fixed_vector<Area,512> areas;
	};

	struct MapList
	{
		MapIndex index;
		MapType mapType;
		GameSubModeType gameSubModeType;
		eastl::fixed_string<char, 256> levelFile;
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
				} status;

				struct {
					i32 entityIndex;
					ActionCommand::TargetPreset targetPreset;
				} entity;

				struct {
					f32 distance;
				} graphMoveHorz;

				struct {
					u8 toLastChild;
				} teleport;

				struct {
					i32 interactionID;
				} interaction;

				struct {
					i32 speed;
				} rotateSpeed;
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
	Map mapPvpDeathMatch;

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
	};

	struct StageMaps
	{
		StageIndex ID;
		i32 joinMemberMax = 0;
		eastl::fixed_vector<MapIndex,8,false> maps;
	};

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
		eastl::fixed_vector<GuildSkillLevel,12,false> levels;
	};

	eastl::fixed_vector<GuildLevelInfo,16,false> guildLevels;
	eastl::fixed_vector<GuildSkillInfo,16,false> guildSkills;
	eastl::fixed_vector<i32,64,false> validGuildEmblems;
	i32 guildActivityCapWeekday = 100;

	i32 GuildLevelForPoints(i32 points) const;
	i32 GuildSkillValue(const char* key, i32 level) const;

	bool FindQueueAreaStage(i32 entryID, AreaIndex* outAreaIndex, StageIndex* outStageIndex) const;
	bool FindStageMap(StageIndex stageID, MapIndex* outMapIndex) const;
	const StageMaps* FindStageMaps(StageIndex stageID) const;
	bool HasEntrySystem(i32 entryID) const;
	CreatureIndex FindDeathMatchBotIndex(ClassType classType) const;

	FileBuffer filePvpDeathmatch01Collision;
	FileBuffer filePvpDeathmatch01CollisionWalls;
	FileBuffer filePvpDeathNmWall04;
	FileBuffer fileCylinderCollision;

	bool Load();
	const MapList* FindMapListByID(MapIndex index) const;
	const Song* FindJukeboxSongByID(SongID songID) const;
	bool LoadCharacterBaseStats();

	const Master& GetMaster(ClassType classType) const;
	i32 GetFirstComboSkill(i32 baseSkill) const { auto f = firstComboSkill.find(baseSkill); return f == firstComboSkill.end() ? 0 : f->second; }
	i32 WeaponTypeOf(ClassType classType, WeaponIndex weaponIndex) const;
	const Action& GetSkillAction(ClassType classType, ActionStateID actionID, i8 stance = 0) const;
	const Action* FindSkillAction(ClassType classType, ActionStateID actionID, i8 stance = 0) const;
	bool CheckSkillActions();
	eastl::hash_set<u64> skillActionGaps;
	struct SkillInput { bool needsClick = false; bool directionOnly = false; bool friendlyOnly = false; bool positionDirection = false; bool selfPosition = false; bool teleportEntity = false; bool allEnemies = false; bool pickedEntity = false; bool lockOn = false; i32 checkStatus = 0; u8 obstructMove = 0; bool obstructSight = false; i32 coolTimeID = 0; f32 coolTime = 0; f32 minDistance = 0; f32 maxDistance = 0; };
	const SkillInput* FindSkillInput(i32 skillID) const { auto f = skillInput.find(skillID); return f == skillInput.end() ? nullptr : &f->second; }
	struct SkillCast { enum Type: u8 { DELAY, LIMIT, CHARGING } type = DELAY; f32 time = 0; ActionStateID action = ActionStateID::INVALID; bool interruptByMove = false; };
	const SkillCast* FindSkillCast(i32 skillID) const { auto f = skillCast.find(skillID); return f == skillCast.end() ? nullptr : &f->second; }
	bool IsCastAction(ActionStateID action) const { foreach_const(it, skillCast) if(it->second.action == action) return true; return false; }
	struct SkillToggle { enum Consume: u8 { NONE, MANA, RAGE, STAMINA } consume = NONE; f32 interval = 0; f32 value = 0; eastl::fixed_vector<i32,4,false> remotes; eastl::fixed_vector<i32,6,false> statuses; };
	const SkillToggle* FindSkillToggle(i32 skillID) const { auto f = skillToggle.find(skillID); return f == skillToggle.end() ? nullptr : &f->second; }
	struct SkillStance { i8 stance = 0; f32 duration = 0; i32 weaponFireCount = 0; bool cancelByDefeat = false; };
	const SkillStance* FindSkillStance(i32 skillID) const { auto f = skillStance.find(skillID); return f == skillStance.end() ? nullptr : &f->second; }
	static i8 StanceFromString(const char* str);
	const Remote& GetRemote(RemoteIdx remoteID) const;
	const Remote* FindRemote(RemoteIdx remoteID) const;
	eastl::fixed_vector<eastl::fixed_string<wchar,48,false>,512,true> nodeNames;
	u16 InternNodeName(const char* name);
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
	struct CostLevel {
		f32 consume[3] = { 0, 0, 0 };
		ActionStateID action = ActionStateID::INVALID;
		eastl::fixed_vector<eastl::pair<i32,i32>,8,false> remoteReplace;
		eastl::fixed_vector<eastl::pair<i32,i32>,8,false> statusReplace;
		f32 damageRatio = 1.0f;
	};
	eastl::hash_map<i32, eastl::fixed_vector<CostLevel,4,false>> skillCostLevels;
	const eastl::fixed_vector<CostLevel,4,false>* FindSkillCostLevels(i32 skillID) const { auto f = skillCostLevels.find(skillID); return f == skillCostLevels.end() ? nullptr : &f->second; }
	f32 GetSkillConsume(i32 skillID, i32 index) const { auto f = skillConsumeRes.find(skillID); return f == skillConsumeRes.end() ? 0 : f->second[index]; }
	eastl::hash_map<i32, i32> comboAutoCastMs;
	i32 GetComboAutoCastMs(i32 skillID) const { auto f = comboAutoCastMs.find(skillID); return f == comboAutoCastMs.end() ? 0 : f->second; }
	eastl::hash_map<i32, i32> comboDurationMs;
	i32 GetComboDurationMs(i32 skillID) const { auto f = comboDurationMs.find(skillID); return f == comboDurationMs.end() ? 0 : f->second; }
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
	struct InteractionInfo { f32 activeDistance = 0; bool carry = false; CreatureIndex receiver = CreatureIndex::Invalid; };
	eastl::hash_map<i32, InteractionInfo> interactions;
	const InteractionInfo* FindInteraction(i32 id) const { auto f = interactions.find(id); return f == interactions.end() ? nullptr : &f->second; }
	f32 GetAniSeqTime(const char* className, i32 seqID) const { auto f = aniSeqTime.find(AniKey(className, seqID)); return f == aniSeqTime.end() ? 0 : f->second; }
	struct MoveCurve
	{
		f32 time = 0;
		f32 maxAbs = 0;
		eastl::fixed_vector<f32,24,false> h;

		f32 Eval(f32 u) const
		{
			const i32 n = (i32)h.size();
			if(n == 0 || maxAbs <= 0) return 0;
			if(n == 1) return h[0] / maxAbs;
			const f32 x = MAX(MIN(u, 1.0f), 0.0f) * (f32)(n - 1);
			const i32 i = MIN((i32)x, n - 1);
			const i32 j = MIN(i + 1, n - 1);
			return (h[i] + (x - (f32)i) * (h[j] - h[i])) / maxAbs;
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
	const SummonInfo* FindSummon(i32 creatureID) const { auto f = summonMap.find(creatureID); return f == summonMap.end() ? nullptr : &f->second; }
	i32 GetCreatureElement(i32 creatureID) const { auto f = creatureElement.find(creatureID); return f == creatureElement.end() ? 0 : f->second; }
	i32 FindSkillHitEventRemote(i32 skillID) const { auto f = skillHitEventRemote.find(skillID); return f == skillHitEventRemote.end() ? 0 : f->second; }

private:
	bool LoadXMLFile(const wchar* fileName, tinyxml2::XMLDocument& xmlData);

	bool LoadMasterDefinitions();
	bool LoadEntrySystems();
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
	bool LoadPvpDeathmach();
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
	eastl::hash_map<i32, eastl::fixed_vector<i32,4,false>> passiveStatuses;
	eastl::hash_map<i32, f32> skillConsumeStamina;
	eastl::hash_map<i32, f32> skillConsumeMP;
	eastl::hash_map<i32, eastl::array<f32,4>> skillConsumeRes;
	eastl::hash_map<i32, f32> skillBaseDamage;
	eastl::hash_map<i32, f32> skillConsumeUG;
	eastl::hash_map<i32, SummonInfo> summonMap;
	eastl::hash_map<u64, f32> aniSeqTime;
	eastl::hash_map<u64, MoveCurve> moveCurves;
	eastl::hash_map<i32, f32> stageTimeLimit;
	eastl::fixed_vector<eastl::pair<eastl::fixed_string<char,48,false>, f32>,128,true> lieEndTime;
	eastl::hash_map<i32, i32> creatureElement;
	bool LoadStatusData();
	bool LoadSkillTooltips();
	bool LoadSkillCosts();
	bool LoadCharacterElements();
	bool LoadSummons();
	bool LoadStageTimeLimits();
	bool LoadAnimationSequenceTimes();
	bool LoadMoveGraphs();
	bool LoadSummonActions();
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
