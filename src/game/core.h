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
enum CreatureType: i32
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
enum EntityType : i32
{
	ENTITY_INVALID = 0,
	ENTITY_CREATURE = 1,
	ENTITY_DYNAMIC = 2

};

// NPC's that have shops or the jukebox have also _NPCType in CreatureComData
// These types can be used when _Type in CreatureComData is NPC
// ToDo get correct values from decompile
enum NPCType: i32
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

enum MapType: i32
{
	MAP_INVALID = 0,
	MAP_CITY = 1, //LOBBY
	MAP_INGAME = 2
};

// When the maptype is INGAME it has a GameSubModeType
enum GameSubModeType : i32
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
