#pragma once
#include <common/base.h>
#include <mxm/core.h>
#include <EASTL/fixed_hash_map.h>
#include <EASTL/fixed_string.h>

// All hero stats loaded from gamedata/heroes/*.xml
struct HeroStats
{
	// Survival
	f32 HP = 2400;
	f32 HP_REGEN = 5;
	f32 TAG_OUT_HEALING = 9;
	f32 DEFENSE = 85;
	f32 CC_REDUCTION = 0;        // percent
	f32 SHIELD_DEFENSE = 0;
	f32 STAMINA = 200;
	f32 STAMINA_PER_SECOND = 5;
	f32 STAMINA_WHEN_STRUCK = 0;
	f32 SKILL_DAMAGE_REDUCTION = 0; // percent
	f32 LIFE_STEAL = 0;           // percent

	// Weapon
	f32 ATTACK = 200;
	f32 WEAPON_DAMAGE = 100;
	f32 CRIT_CHANCE = 3;          // percent
	f32 CRIT_DAMAGE = 150;        // percent
	f32 WEAK_POINT_CRIT = 0;      // percent
	f32 ATTACK_RANGE = 500;
	f32 DEFENSE_PENETRATION = 0;   // percent
	f32 ATTUNEMENT_SCALING = 15;   // percent
	f32 RCLICK_MOVE_SPEED_RATIO = 0.6f; // percent

	// Skills
	f32 SKILL_POWER = 0;
	f32 SKILL_POWER_PENETRATION = 0; // percent
	f32 SKILL_CRIT_CHANCE = 3;     // percent
	f32 SKILL_CRIT_DAMAGE = 150;   // percent
	f32 SKILL_ATT_SCALING = 15;    // percent
	f32 SKILL_VAMP = 0;            // percent
	f32 COOLDOWN_REDUCTION = 0;    // percent
	f32 MANA = 1000;
	f32 MANA_PER_SECOND = 5;
	f32 RECOVER_MP_DURING_TAG_OUT = 7;

	// Class-specific resources
	f32 ENERGY = 0;
	f32 RAGE_GAINED_ON_ATTACK = 0;
	f32 POWER_OF_SOL = 0;

	// Usability
	f32 MOVE_SPEED = 100;          // percent (base 620 in EntityComData)
	f32 ROTATE_SPEED = 700;
	f32 TAG_COOLDOWN = 14;
	f32 ITEM_PICKUP_RANGE = 2;
	f32 RESPAWN_REDUCTION = 0;     // percent

	// Raw move speed from EntityComData (not from hero XML stats)
	f32 BASE_MOVE_SPEED = 620;

	// Resource type: 35=MANA, 17=ENERGY, 36=RAGE, 56=BUBBLE, 0=NONE
	i32 resourceStatType = 35;
	bool hasResource = true; // false = no resource bar at all

	// Set a stat by name, returns true if found
	bool SetStat(const char* name, f32 value);

	// Get stat mapped to protocol StatType ID for spawn packets
	f32 GetStatByTypeID(i32 typeID) const;
};

// Load all hero stats from gamedata/heroes/ directory
// Maps ClassType -> HeroStats
bool LoadHeroStats();
const HeroStats& GetHeroStats(ClassType classType);
const HeroStats& GetDefaultHeroStats();
