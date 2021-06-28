#pragma once
#include <common/base.h>
#include <common/protocol.h>
#include <common/vector_math.h>

enum class ActorUID: u32
{
	INVALID = 0,
};

enum class PlayerID: u32
{
	INVALID = 0xFFFFFFFF
};

// used in skill.xml, botskillconditiondata.xml
// xml tag _NormalConds attribute _ConditionType
// values are from decompile
enum class BotSkillCondition : i32
{
	UNKNOWN_BOT_SKILL_CONDITION_TYPE = -1,
	BOT_SKILL_CONDITION_NO_CONDITION = 0,
	BOT_SKILL_CONDITION_DISTANCE,
	BOT_SKILL_CONDITION_HPRATIO,
	BOT_SKILL_CONDITION_SPRATIO,
	BOT_SKILL_CONDITION_UGRATIO,
	BOT_SKILL_CONDITION_OVERHEADRATIO,
	BOT_SKILL_CONDITION_FRIENDLY_PC_IN_RANGE,
	BOT_SKILL_CONDITION_ENEMY_PC_IN_RANGE,
	BOT_SKILL_CONDITION_FRIENDLY_CREATURE_IN_RANGE,
	BOT_SKILL_CONDITION_ENEMY_CREATURE_IN_RANGE,
	BOT_SKILL_CONDITION_MAX
};

// used in skill.xml, botskillconditiondata.xml
// xml tag _StatusTypeConds attribute _StatusCategory
// values are from decompile
enum class BotSkillStatustypeCategory : i32
{
	UNKNOWN_BOT_SKILL_STATUSTYPE_CATEGORY_TYPE = -1,
	BOT_SKILL_STATUSTYPE_CATEGORY_UNKNOWN = 0,
	BOT_SKILL_STATUSTYPE_CATEGORY_EACH_TYPE,
	BOT_SKILL_STATUSTYPE_CATEGORY_EACH_INDEX,
	BOT_SKILL_STATUSTYPE_CATEGORY_CC_BLOCKALL,
	BOT_SKILL_STATUSTYPE_CATEGORY_CC_RESTRAIN,
	BOT_SKILL_STATUSTYPE_CATEGORY_CC_DISTURB,
	BOT_SKILL_STATUSTYPE_CATEGORY_DEBUFF,
	BOT_SKILL_STATUSTYPE_CATEGORY_INVINCIBLE,
	BOT_SKILL_STATUSTYPE_CATEGORY_MAX
};

enum class CoreAffinity: i32
{
	MAIN = 0,
	NETWORK = 1,
	COORDINATOR = 2,
	CHANNELS = 3,
};

// used in CREATURE_MONSTER_BOT.xml, CREATURE_MONSTER_PVE.xml
// xml tag CreatureComData attribute _Rank
// values are from decompile
enum class CreatureRank : i32
{
	UNKNOWN_CREATURE_RANK = -1, // UNKNOWN_CREATURE_RANK default for switch case
	CREATURE_RANK_INVALID = 0,
	CREATURE_RANK_COMMON,
	CREATURE_RANK_ELITE,
	CREATURE_RANK_ELITENAMED,
	CREATURE_RANK_HERO,
	CREATURE_RANK_BOSS,
	CREATURE_RANK_EX,
	CREATURE_RANK_NODROP,
	CREATURE_RANK_NO_MONSTER,
	CREATURE_RANK_NO_MAX
};

// used in CREATURE_MONSTER_BOT.xml, CREATURE_MONSTER_PVE.xml
// xml tag CreatureComData attribute _RankDetail
// values are from decompile
enum class CreatureRankDetail : i32
{
	UNKNOWN_CREATURE_RANK_DETAIL = -1, // UNKNOWN_CREATURE_RANK_DETAIL default for switch case
	CREATURE_RANK_DETAIL_INVALID = 0,
	CREATURE_RANK_DETAIL_COMMON_1,
	CREATURE_RANK_DETAIL_COMMON_2,
	CREATURE_RANK_DETAIL_COMMON_3,
	CREATURE_RANK_DETAIL_COMMON_4,
	CREATURE_RANK_DETAIL_COMMON_5,
	CREATURE_RANK_DETAIL_ELITE_1,
	CREATURE_RANK_DETAIL_ELITE_2,
	CREATURE_RANK_DETAIL_ELITE_3,
	CREATURE_RANK_DETAIL_ELITE_4,
	CREATURE_RANK_DETAIL_ELITE_5,
	CREATURE_RANK_DETAIL_ELITE_6,
	CREATURE_RANK_DETAIL_ELITE_7,
	CREATURE_RANK_DETAIL_ELITE_8,
	CREATURE_RANK_DETAIL_ELITE_9,
	CREATURE_RANK_DETAIL_ELITE_10,
	CREATURE_RANK_DETAIL_ELITENAMED_1,
	CREATURE_RANK_DETAIL_ELITENAMED_2,
	CREATURE_RANK_DETAIL_ELITENAMED_3,
	CREATURE_RANK_DETAIL_ELITENAMED_4,
	CREATURE_RANK_DETAIL_ELITENAMED_5,
	CREATURE_RANK_DETAIL_HERO,
	CREATURE_RANK_DETAIL_BOSS,
	CREATURE_RANK_DETAIL_EX,
	CREATURE_RANK_DETAIL_NO_MONSTER,
	CREATURE_RANK_DETAIL_MAX
};

// Possible CreatureComData _Type from the xml files
// These types can be used when the _Type in EntityComData is CREATURE
// values are from decompile
// ToDo test how the game react if the EntityComData Type isn't CREATURE
enum class CreatureType: i32
{
	CREATURE_TYPE_INVALID = -1, //UNKNOWN_CREATURE_TYPE default for switch case
	CREATURE_TYPE_NONE = 0,
	CREATURE_TYPE_PC,
	CREATURE_TYPE_MONSTER,
	CREATURE_TYPE_NPC,
	CREATURE_TYPE_ALLY,
	CREATURE_TYPE_BOT,
	CREATURE_TYPE_NONE_DYNAMIC,
	CREATURE_TYPE_MAX
};

// _Type in EntityComData
// got values from decompile
enum class EntityType : i32
{
	ENTITY_INVALID = -1, //UNKNOWN_ENTITY_TYPE default return
	ENTITY_TERRAIN = 0, // haven't found this one in EntityComData was in an attribute CaseValue1 in tag <StatePlay>
	ENTITY_CREATURE,
	ENTITY_ITEM,
	ENTITY_DYNAMIC,
	ENTITY_REMOTE,
	ENTITY_MARKER,
	ENTITY_EFFECT,
	ENTITY_SOUND, //not found in chinese data files check if this is changed to SFX in chinese release
	ENTITY_DEFAULT,
	ENTITY_MAX,
	ENTITY_SFX //found in chinese data files check if exist in West files
};

// Has ItemComData _Type ITEM_TYPE_ETC
// Most of these types don't have a subtype
// xml tag EtcItemComData attribute _Type
// got values from decompile
enum class EtcType : i32
{
	ETC_TYPE_INVALID = -1,
	ETC_TYPE_FOOD = 0,
	ETC_TYPE_GOLD,
	ETC_TYPE_INVEN_TAB_0_1,
	ETC_TYPE_LEVELUPCOIN,
	ETC_TYPE_EATER,
	ETC_TYPE_LIFECHARGER,
	ETC_TYPE_SUPPORTKIT,
	ETC_TYPE_RONBOOSTER,
	ETC_TYPE_EXPBOOSTER,
	ETC_TYPE_MEDALBOOSTER,
	ETC_TYPE_TICKET,
	ETC_TYPE_VIP,
	ETC_TYPE_BOX,
	ETC_TYPE_MISC,
	ETC_TYPE_OTHERS,
	ETC_TYPE_BOOTY,
	ETC_TYPE_SKINSTONE,
	ETC_TYPE_RENAMER,
	ETC_TYPE_LIFETIME_SKIN,
	ETC_TYPE_LIFETIME_MASTER,
	ETC_TYPE_JUKEBOX,
	ETC_TYPE_NONE,
	ETC_TYPE_INVEN_TAB_1_2,
	ETC_TYPE_INGREDIENT,
	ETC_TYPE_MATERIA_ESSENCE,
	ETC_TYPE_INVEN_TAB_END,
	ETC_TYPE_CHARACTER,	// default value in switch case if value is < 0x3e9


	ETC_TYPE_EXPAND_INVENTORY_GEAR = 1001,
	ETC_TYPE_EXPAND_INVENTORY_MATERIAL,
	ETC_TYPE_EXPAND_INVENTORY_NORMAL,
	ETC_TYPE_EXPAND_MASTERGEAR,
	ETC_TYPE_EXPAND_WAREHOUSE,
	ETC_TYPE_OPEN_REWARD_ALL,
	ETC_TYPE_OPEN_REWARD_ONEMORE,
	ETC_TYPE_REFILL_STAGE_BATTERY,
	ETC_TYPE_REVIVAL_BY_COIN,
	ETC_TYPE_SKIN,
	ETC_TYPE_TITLE,
	ETC_TYPE_CCOINTOGOLD,
	ETC_TYPE_NCOINTOGOLD,
	ETC_TYPE_PROMOTIONGOLD,
	ETC_TYPE_GUILD_EMBLEM,
	ETC_TYPE_MAX,

	ETC_TYPE_SOUL // not in decomp but it's in ITEM_RON.xml in chinese dataset ToDo: check if this exist in west data
};

// When the maptype is INGAME it has a GameSubModeType except the training room
enum class GameSubModeType : i32
{
	GAME_SUB_MODE_INVALID = 0,
	GAME_SUB_MODE_DEATH_MATCH_NORMAL = 1,
	GAME_SUB_MODE_OCCUPY_CORE = 2,
	GAME_SUB_MODE_OCCUPY_BUSH = 3,
	GAME_SUB_MODE_GOT_AUTHENTIC = 4,
	GAME_SUB_MODE_GOT_TUTORIAL_BASIC = 5,
	GAME_SUB_MODE_GOT_TUTORIAL_EXPERT = 6,
	GAME_SUB_MODE_GOT_FIRE_POWER = 7,
	GAME_SUB_MODE_GOT_ULTIMATE_TITAN = 8,
	GAME_SUB_MODE_SPORTS_RUN = 9,
	GAME_SUB_MODE_SPORTS_SURVIVAL = 10,
	GAME_SUB_MODE_STAGE_TUTORIAL = 11,
	GAME_SUB_MODE_STAGE_NORMAL = 12
};

// Has ItemComData _Type ITEM_TYPE_GEAR
enum class GearType : i32
{
	GEAR_INVALID = 0,
	GEAR_ATT = 1,
	GEAR_DEF = 2,
	GEAR_UTIL = 3
};

enum class InvenTab : i32
{
	INVEN_TAB_INVALID = -1, // UNKNOWN_E_INVEN_TAB idk the value
	INVEN_TAB_NONE = 0,
	INVEN_TAB_0,
	INVEN_TAB_1,
	INVEN_TAB_2,
	INVEN_TAB_MAX,
	INVEN_TAB_SYSTEM = 7,
	INVEN_TAB_STORAGE,
	INVEN_TAB_TEMP
};

// Has EntityComData _Type ENTITY_TYPE_ITEM
enum class ItemType : i32
{
	ITEM_INVALID = 0,
	ITEM_WEAPON = 1,
	ITEM_ETC = 2,
	ITEM_GEAR = 3
};

enum class MapType : i32
{
	MAP_INVALID = 0,
	MAP_INGAME,
	MAP_CITY, //LOBBY
	MAP_PLATFORM,
	MAP_TUTORIAL,
	MAP_MAX // Anything else is UNKNOWN_E_MAP_TYPE
};

// NPC's that have shops or the jukebox have also _NPCType in CreatureComData
// These types can be used when _Type in CreatureComData is NPC
// ToDo get correct values from decompile
enum class NPCType: i32
{
	NPC_INVALID = 0,	// I don't know if these Types have an INVALID value --Delta-47
	NPC_JUKEBOX = 1,
	NPC_WAREHOUSE = 2,
	NPC_MEDAL_TRADER = 3,
	NPC_INGREDIENT_TRADER = 4,
	NPC_SKIN_TRADER = 5,
	NPC_GUILD = 6,
	NPC_PVE = 7,
	NPC_PVP = 8,
	NPC_MASTERTRAINING = 9,
	NPC_EVENTSHOP = 10
};

// _Type values for <ST_COMMONSKILL> in SKILL.xml
enum class SkillType : i32
{
	SKILL_INVALID = 0,
	SKILL_PASSIVE,
	SKILL_SHIRK,
	SKILL_NORMAL,
	SKILL_STANCE,
	SKILL_SUMMON,
	SKILL_COMBO,
	SKILL_TOGGLE
};

// Has ETC_TYPE_SUPPORTKIT _Type in EtcItemComData
enum class SupportKitType : i32
{
	SUPPORTKIT_INVALID = 0,
	SUPPORTKIT_HP = 1
};

// values from decompile
// doesn't seem to be used by the client side
enum class TriggerType : i32
{
	TRIGGER_TYPE_INVALID = -1, //UNKNOWN_ LevelTriggerType default return
	TRIGGER_TYPE_BOUND = 0,
	TRIGGER_TYPE_PLAYER_BOUND,
	TRIGGER_TYPE_CREATURE_ENTER,
	TRIGGER_TYPE_CREATURE_OUT,
	TRIGGER_TYPE_HP,
	TRIGGER_TYPE_DEAD,
	TRIGGER_TYPE_HIT,
	TRIGGER_TYPE_LEVELSTART,
	TRIGGER_TYPE_START_TIME_RECORD,
	TRIGGER_TYPE_INTERACTION,
	TRIGGER_TYPE_PHASE_START,
	TRIGGER_TYPE_PHASE_END,
	TRIGGER_TYPE_AI,
	TRIGGER_TYPE_TUTORIAL,
	TRIGGER_TYPE_TAG,
	TRIGGER_TYPE_GAME_END,
	TRIGGER_TYPE_DIALOG,
	TRIGGER_TYPE_TAKE_ITEM
};

constexpr i32 JUKEBOX_MAX_TRACKS = 8;

enum class TeamID: i32
{
	INVALID = -1,
	RED = 0,
	BLUE = 1,
	_COUNT = 2
};

// TODO: weird place for this
struct PlayerCastSkill
{
	ActorUID playerActorUID;
	SkillID skillID;
	vec3 p3nPos;
	eastl::fixed_vector<ActorUID,32,false> targetList;
	Cl::CQ_PlayerCastSkill::PosStruct posStruct;
};

struct RotationHumanoid
{
	f32 upperYaw = 0;
	f32 upperPitch = 0;
	f32 bodyYaw = 0;
};

inline f32 MxmYawToWorldYaw(f32 a)
{
	return -a - PI/2;
}

inline f32 WorldYawToMxmYaw(f32 a)
{
	return -(a + PI/2);
}

inline f32 MxmPitchToWorldPitch(f32 a)
{
	return a; // TODO: reverse this
}

inline f32 WorldPitchToMxmPitch(f32 a)
{
	return a; // TODO: reverse this
}

constexpr i32 PLAYER_CHARACTER_COUNT = 2;


const char* ActionStateString(ActionStateID state);

struct GlobalTweakableVariables
{
	f32 jumpForce = 2000;
	f32 gravity = 5000;
	f32 stepHeight = 200;
};

GlobalTweakableVariables& GetGlobalTweakableVars();
