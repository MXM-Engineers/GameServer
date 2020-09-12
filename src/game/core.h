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
	INVALID = 0,
	NPC = 1,
	BOT = 2,
	PC = 3,
	MONSTER = 4,
	ALLY = 5
};

// _Type in EntityComData
// ToDo get correct values from decompile
enum class EntityType : i32
{
	INVALID = 0,
	CREATURE = 1,
	DYNAMIC = 2

};

// NPC's that have shops or the jukebox have also _NPCType in CreatureComData
// These types can be used when _Type in CreatureComData is NPC
// ToDo get correct values from decompile
enum class NPCType: i32
{
	INVALID = 0,	// I don't know if these Types have an INVALID value --Delta-47
	JUKEBOX = 1,
	WAREHOUSE = 2,
	MEDAL_TRADER = 3,
	INGREDIENT_TRADER = 4,
	SKIN_TRADER = 5,
	GUILD = 6,
	PVE = 7,
	PVP = 8,
	MASTERTRAINING = 9,
	EVENTSHOP = 10
};

enum class MapType: i32
{
	INVALID = 0,
	CITY = 1, //LOBBY
	INGAME = 2
};

// When the maptype is INGAME it has a GameSubModeType
enum class GameSubModeType : i32
{
	INVALID = 0,
	DEATH_MATCH_NORMAL = 1,
	OCCUPY_CORE = 2,
	OCCUPY_BUSH = 3,
	GOT_AUTHENTIC = 4,
	GOT_TUTORIAL_BASIC = 5,
	GOT_TUTORIAL_EXPERT = 6,
	GOT_FIRE_POWER = 7,
	GOT_ULTIMATE_TITAN = 8,
	SPORTS_RUN = 9,
	SPORTS_SURVIVAL = 10,
	STAGE_TUTORIAL = 11,
	STAGE_NORMAL = 12
};

constexpr i32 JUKEBOX_MAX_TRACKS = 8;
