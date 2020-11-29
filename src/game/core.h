#pragma once
#include <common/base.h>

enum class ActorUID: u32
{
	INVALID = 0,
};

enum class CoreAffinity: i32
{
	MAIN = 0,
	NETWORK = 1,
	COORDINATOR = 2,
	CHANNELS = 3,
};

// Possible CreatureComData _Type from the xml files
// These types can be used when the _Type in EntityComData is CREATURE
// ToDo get correct values from decompile
// ToDo test how the game react if the EntityComData Type isn't CREATURE
enum class CreatureType: i32
{
	CREATURE_INVALID = 0,
	CREATURE_NPC = 1,
	CREATURE_BOT = 2,
	CREATURE_PC = 3,
	CREATURE_MONSTER = 4,
	CREATURE_ALLY = 5
};

// _Type in EntityComData
// ToDo get correct values from decompile
enum class EntityType : i32
{
	ENTITY_INVALID = 0,
	ENTITY_CREATURE = 1,
	ENTITY_DYNAMIC = 2,
	ENTITY_SFX = 3,
	ENTITY_ITEM = 4,
	ENTITY_TERRAIN = 5 // haven't found this one in EntityComData was in an attribute CaseValue1 in tag <StatePlay>
};

// Has ItemComData _Type ITEM_TYPE_ETC
// Most of these types don't have a subtype 
enum class EtcType : i32
{
	ETC_INVALID = 0,
	ETC_SUPPORTKIT = 1,
	ETC_GOLD = 2,
	ETC_SOUL = 3,
	ETC_INGREDIENT = 4,
	ETC_TICKET = 5,
	ETC_EATER = 6,
	ETC_TITLE = 7,
	ETC_EXPBOOSTER = 8,
	ETC_RONBOOSTER = 9,
	ETC_REVIVAL_BY_COIN = 10,
	ETC_OTHERS = 11,
	ETC_OPEN_REWARD_ONEMORE = 12,
	ETC_OPEN_REWARD_ALL = 13,
	ETC_PROMOTIONGOLD = 14,
	ETC_SKIN = 15,
	ETC_SKINSTONE = 16,
	ETC_FOOD = 17,
	ETC_GUILD_EMBLEM = 18,
	ETC_JUKEBOX = 19,
	ETC_LEVELUPCOIN = 20,
	ETC_LIFETIME_SKIN = 21,
	ETC_LIFECHARGER = 22,
	ETC_MEDALBOOSTER = 23,
	ETC_CCOINTOGOLD = 24,
	ETC_CHARACTER = 25,
	ETC_BOOTY = 26,
	ETC_BOX = 27,
	ETC_NONE = 28
};

// Has EntityComData _Type ENTITY_TYPE_ITEM
enum class ItemType : i32
{
	ITEM_INVALID = 0,
	ITEM_WEAPON = 1,
	ITEM_ETC = 2,
	ITEM_GEAR = 3
};

// Has ItemComData _Type ITEM_TYPE_GEAR
enum class GearType : i32
{
	GEAR_INVALID = 0,
	GEAR_ATT = 1,
	GEAR_DEF = 2,
	GEAR_UTIL = 3
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

enum class MapType: i32
{
	MAP_INVALID = 0,
	MAP_CITY = 1, //LOBBY
	MAP_INGAME = 2
};

// Has ETC_TYPE_SUPPORTKIT _Type in EtcItemComData
enum class SupportKitType : i32
{
	SUPPORTKIT_INVALID = 0,
	SUPPORTKIT_HP = 1
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

constexpr i32 JUKEBOX_MAX_TRACKS = 8;

enum class TeamID: i32
{
	INVALID = -1,
	RED = 0,
	BLUE = 1,
	_COUNT = 2
};
