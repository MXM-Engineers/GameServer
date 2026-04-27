#include "hero_stats.h"
#include <common/base.h>
#include <common/platform.h>
#include <tinyxml2.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EASprintf.h>
#include <EAStdC/EAScanf.h>

using namespace tinyxml2;

static eastl::fixed_hash_map<ClassType, HeroStats, 50> g_heroStatsMap;
static HeroStats g_defaultStats;

bool HeroStats::SetStat(const char* name, f32 value)
{
	// Survival
	if(EA::StdC::Strcmp(name, "HP") == 0) { HP = value; return true; }
	if(EA::StdC::Strcmp(name, "HP_REGEN") == 0) { HP_REGEN = value; return true; }
	if(EA::StdC::Strcmp(name, "TAG_OUT_HEALING") == 0) { TAG_OUT_HEALING = value; return true; }
	if(EA::StdC::Strcmp(name, "DEFENSE") == 0) { DEFENSE = value; return true; }
	if(EA::StdC::Strcmp(name, "CC_REDUCTION") == 0) { CC_REDUCTION = value; return true; }
	if(EA::StdC::Strcmp(name, "SHIELD_DEFENSE") == 0) { SHIELD_DEFENSE = value; return true; }
	if(EA::StdC::Strcmp(name, "STAMINA") == 0) { STAMINA = value; return true; }
	if(EA::StdC::Strcmp(name, "STAMINA_PER_SECOND") == 0) { STAMINA_PER_SECOND = value; return true; }
	if(EA::StdC::Strcmp(name, "STAMINA_WHEN_STRUCK") == 0) { STAMINA_WHEN_STRUCK = value; return true; }
	if(EA::StdC::Strcmp(name, "SKILL_DAMAGE_REDUCTION") == 0) { SKILL_DAMAGE_REDUCTION = value; return true; }
	if(EA::StdC::Strcmp(name, "LIFE_STEAL") == 0) { LIFE_STEAL = value; return true; }

	// Weapon
	if(EA::StdC::Strcmp(name, "ATTACK") == 0) { ATTACK = value; return true; }
	if(EA::StdC::Strcmp(name, "WEAPON_DAMAGE") == 0) { WEAPON_DAMAGE = value; return true; }
	if(EA::StdC::Strcmp(name, "CRIT_CHANCE") == 0) { CRIT_CHANCE = value; return true; }
	if(EA::StdC::Strcmp(name, "CRIT_DAMAGE") == 0) { CRIT_DAMAGE = value; return true; }
	if(EA::StdC::Strcmp(name, "WEAK_POINT_CRIT") == 0) { WEAK_POINT_CRIT = value; return true; }
	if(EA::StdC::Strcmp(name, "ATTACK_RANGE") == 0) { ATTACK_RANGE = value; return true; }
	if(EA::StdC::Strcmp(name, "DEFENSE_PENETRATION") == 0) { DEFENSE_PENETRATION = value; return true; }
	if(EA::StdC::Strcmp(name, "ATTUNEMENT_SCALING") == 0) { ATTUNEMENT_SCALING = value; return true; }
	if(EA::StdC::Strcmp(name, "RCLICK_MOVE_SPEED_RATIO") == 0) { RCLICK_MOVE_SPEED_RATIO = value; return true; }

	// Skills
	if(EA::StdC::Strcmp(name, "SKILL_POWER") == 0) { SKILL_POWER = value; return true; }
	if(EA::StdC::Strcmp(name, "SKILL_POWER_PENETRATION") == 0) { SKILL_POWER_PENETRATION = value; return true; }
	if(EA::StdC::Strcmp(name, "SKILL_CRIT_CHANCE") == 0) { SKILL_CRIT_CHANCE = value; return true; }
	if(EA::StdC::Strcmp(name, "SKILL_CRIT_DAMAGE") == 0) { SKILL_CRIT_DAMAGE = value; return true; }
	if(EA::StdC::Strcmp(name, "SKILL_ATT_SCALING") == 0) { SKILL_ATT_SCALING = value; return true; }
	if(EA::StdC::Strcmp(name, "SKILL_VAMP") == 0) { SKILL_VAMP = value; return true; }
	if(EA::StdC::Strcmp(name, "COOLDOWN_REDUCTION") == 0) { COOLDOWN_REDUCTION = value; return true; }
	if(EA::StdC::Strcmp(name, "MANA") == 0) { MANA = value; return true; }
	if(EA::StdC::Strcmp(name, "MANA_PER_SECOND") == 0) { MANA_PER_SECOND = value; return true; }
	if(EA::StdC::Strcmp(name, "RECOVER_MP_DURING_TAG_OUT") == 0) { RECOVER_MP_DURING_TAG_OUT = value; return true; }

	// Class-specific
	if(EA::StdC::Strcmp(name, "ENERGY") == 0) { ENERGY = value; return true; }
	if(EA::StdC::Strcmp(name, "RAGE_GAINED_ON_ATTACK") == 0) { RAGE_GAINED_ON_ATTACK = value; return true; }
	if(EA::StdC::Strcmp(name, "POWER_OF_SOL") == 0) { POWER_OF_SOL = value; return true; }

	// Usability
	if(EA::StdC::Strcmp(name, "MOVE_SPEED") == 0) { MOVE_SPEED = value; return true; }
	if(EA::StdC::Strcmp(name, "ROTATE_SPEED") == 0) { ROTATE_SPEED = value; return true; }
	if(EA::StdC::Strcmp(name, "TAG_COOLDOWN") == 0) { TAG_COOLDOWN = value; return true; }
	if(EA::StdC::Strcmp(name, "ITEM_PICKUP_RANGE") == 0) { ITEM_PICKUP_RANGE = value; return true; }
	if(EA::StdC::Strcmp(name, "RESPAWN_REDUCTION") == 0) { RESPAWN_REDUCTION = value; return true; }

	return false;
}

// Map stat to protocol stat type IDs used in SN_GameCreateActor maxStats/curStats
f32 HeroStats::GetStatByTypeID(i32 typeID) const
{
	switch(typeID) {
		case 0:  return HP;                    // STAT_TYPE_HEALTH
		case 2:  return ATTACK;                // STAT_TYPE_STAMINA -> actually ATK in packets
		case 3:  return HP_REGEN;              // STAT_TYPE_HEALTHREGEN
		case 5:  return STAMINA_PER_SECOND;    // STAT_TYPE_STAMINAREGEN
		case 6:  return ATTACK;                // STAT_TYPE_ATTACK
		case 7:  return SHIELD_DEFENSE;        // STAT_TYPE_DEFENCE -> shieldDef in packets
		case 9:  return CRIT_CHANCE;           // STAT_TYPE_CRITICALRATE
		case 10: return CRIT_DAMAGE;           // STAT_TYPE_CRITICALDAMAGE -> lifesteal in old code
		case 13: return LIFE_STEAL;            // STAT_TYPE_LIFESTEAL
		case 14: return TAG_COOLDOWN;          // STAT_TYPE_MOVESPEED -> tagCooldown in packets
		case 15: return MOVE_SPEED;            // STAT_TYPE_ATTACKSPEED -> moveSpeed%
		case 16: return 1;                     // STAT_TYPE_ABILITYPOINT
		case 17: return ENERGY;                // STAT_TYPE_ENERGY
		case 18: return ROTATE_SPEED;          // STAT_TYPE_ROTATESPEED
		case 22: return ROTATE_SPEED;          // STAT_TYPE_SKILL_DEFENCE_PENETRATION -> rotateSpeed
		case 23: return ITEM_PICKUP_RANGE;     // STAT_TYPE_PICKUP_RADIUS
		case 29: return DEFENSE_PENETRATION;   // STAT_TYPE_TAG_HEALTHREGEN_VALUE -> defPen
		case 31: return SKILL_POWER_PENETRATION; // STAT_TYPE_TAG_STAMINAREGEN_VALUE -> skillDefPen
		case 35: return MANA;                  // STAT_TYPE_MANA
		case 37: return DEFENSE;               // STAT_TYPE_MANAREGEN -> defense in packets
		case 39: return HP_REGEN;              // STAT_TYPE_ENERGYREGEN -> hpRegen
		case 42: return RCLICK_MOVE_SPEED_RATIO; // STAT_TYPE_RAGE_DAMAGE_RECOVERY_RATIO
		case 44: return TAG_OUT_HEALING;       // STAT_TYPE_REMOTESPEED -> tagOutHealing
		case 52: return SKILL_POWER;           // STAT_TYPE_BUBBLE -> skillPower
		case 54: return SKILL_CRIT_CHANCE;     // STAT_TYPE_SKILLDAMAGE_REDUCE_RATIO
		case 55: return SKILL_CRIT_DAMAGE;     // STAT_TYPE_SKILLDAMAGE_RATIO
		case 63: return COOLDOWN_REDUCTION;    // STAT_TYPE_SKILL_CRITICALRATE
		case 64: return STAMINA;               // STAT_TYPE_SKILL_CRITICALDAMAGE -> stamina
		default: return 0;
	}
}

// ClassType from string
static ClassType ClassTypeFromString(const char* str)
{
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_STRIKER") == 0) return ClassType::STRIKER;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_ARTILLERY") == 0) return ClassType::ARTILLERY;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_ASSASSIN") == 0) return ClassType::ASSASSIN;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_ELECTRO") == 0) return ClassType::ELECTRO;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_DEFENDER") == 0) return ClassType::DEFENDER;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_SNIPER") == 0) return ClassType::SNIPER;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_DEATHKNIGHT") == 0) return ClassType::DEATHKNIGHT;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_DESTROYER") == 0) return ClassType::DESTROYER;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_MECHANIC") == 0) return ClassType::MECHANIC;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_SOULMASTER") == 0) return ClassType::SOULMASTER;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_DOUBLEGUN") == 0) return ClassType::DOUBLEGUN;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_JINSEOYEON") == 0) return ClassType::JINSEOYEON;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_KROMEDE") == 0) return ClassType::KROMEDE;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_RODMASTER") == 0) return ClassType::RODMASTER;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_PHOTOG") == 0) return ClassType::PHOTOG;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_ICEQUEEN") == 0) return ClassType::ICEQUEEN;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_MAGICGIRL") == 0) return ClassType::MAGICGIRL;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_POHWARAN") == 0) return ClassType::POHWARAN;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_BOOMERANG") == 0) return ClassType::BOOMERANG;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_SLIME") == 0) return ClassType::SLIME;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_MONDOZAX") == 0) return ClassType::MONDOZAX;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_LILU") == 0) return ClassType::LILU;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_EFREET") == 0) return ClassType::EFREET;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_SHADOWHUNTER") == 0) return ClassType::SHADOWHUNTER;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_RYTLOCK") == 0) return ClassType::RYTLOCK;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_CATTHECAT") == 0) return ClassType::CATTHECAT;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_RNB") == 0) return ClassType::RNB;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_BATTER") == 0) return ClassType::BATTER;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_ANDROA") == 0) return ClassType::ANDROA;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_ANDROB") == 0) return ClassType::ANDROB;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_ATTACKER") == 0) return ClassType::ATTACKER;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_BOXER") == 0) return ClassType::BOXER;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_SHUGOTRADER") == 0) return ClassType::SHUGOTRADER;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_NAGA") == 0) return ClassType::NAGA;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_LAUNCHER") == 0) return ClassType::LAUNCHER;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_STATESMAN") == 0) return ClassType::STATESMAN;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_YURI") == 0) return ClassType::YURI;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_ESPER") == 0) return ClassType::ESPER;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_PRIEST") == 0) return ClassType::PRIEST;
	if(EA::StdC::Strcmp(str, "CLASS_TYPE_GHOSTWIDOW") == 0) return ClassType::GHOSTWIDOW;
	return ClassType::NONE;
}

static bool LoadSingleHeroXML(const char* filepath)
{
	XMLDocument doc;
	if(doc.LoadFile(filepath) != XML_SUCCESS) {
		LOG("WARNING: Failed to load hero XML '%s'", filepath);
		return false;
	}

	XMLElement* root = doc.FirstChildElement("Hero");
	if(!root) return false;

	const char* classTypeStr = root->Attribute("classType");
	if(!classTypeStr) return false;

	ClassType ct = ClassTypeFromString(classTypeStr);
	if(ct == ClassType::NONE) {
		LOG("WARNING: Unknown classType '%s' in '%s'", classTypeStr, filepath);
		return false;
	}

	HeroStats stats;

	// Parse all sections
	const char* sections[] = {"Survival", "Weapon", "Skills", "Usability"};
	for(const char* section : sections) {
		XMLElement* sec = root->FirstChildElement(section);
		if(!sec) continue;

		for(XMLElement* stat = sec->FirstChildElement("Stat"); stat; stat = stat->NextSiblingElement("Stat")) {
			const char* name = stat->Attribute("name");
			const char* valueStr = stat->Attribute("value");
			if(!name || !valueStr) continue;
			if(valueStr[0] == '?') continue; // skip unset values

			f32 value = 0;
			if(sscanf(valueStr, "%f", &value) != 1) continue;

			if(!stats.SetStat(name, value)) {
				LOG("WARNING: Unknown stat '%s' in '%s'", name, filepath);
			}
		}
	}

	// Read resource type from HeroMechanics/Resource
	XMLElement* mechanics = root->FirstChildElement("HeroMechanics");
	if(mechanics) {
		XMLElement* resource = mechanics->FirstChildElement("Resource");
		if(resource) {
			const char* resType = resource->Attribute("type");
			if(resType) {
				if(EA::StdC::Strcmp(resType, "NONE") == 0) {
					stats.hasResource = false;
					stats.resourceStatType = 0;
				}
				else if(EA::StdC::Strcmp(resType, "MANA") == 0 || EA::StdC::Strcmp(resType, "MP") == 0) {
					stats.hasResource = true;
					stats.resourceStatType = 35;
				}
				else if(EA::StdC::Strcmp(resType, "ENERGY") == 0 || EA::StdC::Strcmp(resType, "EP") == 0) {
					stats.hasResource = true;
					stats.resourceStatType = 17; // stat 17 = ENERGY (confirmed from official capture: 67x type=17, 0x type=18)
				}
				else if(EA::StdC::Strcmp(resType, "RAGE") == 0 || EA::StdC::Strcmp(resType, "RP") == 0 ||
						EA::StdC::Strcmp(resType, "WILL_RP") == 0) {
					stats.hasResource = true;
					stats.resourceStatType = 36;
				}
				else if(EA::StdC::Strcmp(resType, "BUBBLE") == 0) {
					stats.hasResource = true;
					stats.resourceStatType = 56; // stat 56 = BUBBLE (Jinsoyun, Sonid)
				}
			}
			// Allow override via statType attribute
			int st = 0;
			if(resource->QueryIntAttribute("statType", &st) == XML_SUCCESS && st > 0) {
				stats.resourceStatType = st;
			}
			// Remap statType 18 -> 17 (18=ROTATESPEED, 17=ENERGY in protocol)
			if(stats.resourceStatType == 18) {
				stats.resourceStatType = 17;
			}
		}
	} else {
		// No HeroMechanics section = default MANA
		stats.hasResource = true;
		stats.resourceStatType = 35;
	}

	const char* heroName = root->Attribute("name");
	g_heroStatsMap.emplace(ct, stats);
	const char* resName = "MANA";
	if(stats.resourceStatType == 17) resName = "ENERGY";
	else if(stats.resourceStatType == 36) resName = "RAGE";
	else if(stats.resourceStatType == 56) resName = "BUBBLE";
	else if(stats.resourceStatType == 0) resName = "NONE";
	LOG("  Loaded hero: %s (%s) HP=%.0f ATK=%.0f DEF=%.1f Range=%.0f Res=%s(%d)",
		heroName ? heroName : "?", classTypeStr,
		stats.HP, stats.ATTACK, stats.DEFENSE, stats.ATTACK_RANGE,
		resName, stats.resourceStatType);

	return true;
}

bool LoadHeroStats()
{
	LOG("Loading hero stats...");

	// Scan all .xml files in gamedata/heroes/
	const char* heroDir = "../gamedata/heroes";

#ifdef CONF_WINDOWS
	WIN32_FIND_DATAA findData;
	char searchPath[512];
	snprintf(searchPath, sizeof(searchPath), "%s/*.xml", heroDir);

	HANDLE hFind = FindFirstFileA(searchPath, &findData);
	if(hFind == INVALID_HANDLE_VALUE) {
		LOG("WARNING: No hero XMLs found in '%s'", heroDir);
		return false;
	}

	do {
		char fullPath[512];
		snprintf(fullPath, sizeof(fullPath), "%s/%s", heroDir, findData.cFileName);
		LoadSingleHeroXML(fullPath);
	} while(FindNextFileA(hFind, &findData));

	FindClose(hFind);
#else
	// Linux: use opendir/readdir
	LOG("WARNING: Hero stats loading not implemented for Linux");
	return false;
#endif

	LOG("Loaded %d hero stat definitions.", (i32)g_heroStatsMap.size());
	return g_heroStatsMap.size() > 0;
}

const HeroStats& GetHeroStats(ClassType classType)
{
	auto it = g_heroStatsMap.find(classType);
	if(it != g_heroStatsMap.end()) {
		return it->second;
	}
	return g_defaultStats;
}

const HeroStats& GetDefaultHeroStats()
{
	return g_defaultStats;
}
