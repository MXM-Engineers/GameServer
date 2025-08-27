#include <common/utils.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EASprintf.h>
#include <tinyxml2.h>

#include "core.h"
#include "game_content.h"

using namespace tinyxml2;
constexpr eastl::hash<const char*> strHash;

static GameXmlContent* g_GameXmlContent = nullptr;

static Path gameDataDir = L"../gamedata";

bool GameXmlContent::LoadMasterDefinitions()
{
	// Parse CREATURE_CHARACTER.xml once
	if(!LoadXMLFile(L"/CREATURE_CHARACTER.xml", xmlCREATURECHARACTER)) return false;

	// get master IDs
	for(XMLElement* pNodeMaster = xmlCREATURECHARACTER.FirstChildElement()->FirstChildElement();
		pNodeMaster;
		pNodeMaster = pNodeMaster->NextSiblingElement()) {
		masters.push_back();
		Master& master = masters.back();

		i32 masterID;
		pNodeMaster->QueryAttribute("ID", &masterID);

		const char* className;
		XMLElement* pStatsCompData = pNodeMaster->FirstChildElement("StatsComData");
		pStatsCompData->QueryStringAttribute("_class", &className);

		// read skill ids
		XMLElement* pSkillElt = pNodeMaster->FirstChildElement("SkillComData")->FirstChildElement();
		do {
			i32 skillID;
			pSkillElt->QueryAttribute("_Index", &skillID);
			master.skillIDs.push_back((SkillID)skillID);

			pSkillElt = pSkillElt->NextSiblingElement();
		} while(pSkillElt);

		// save master data
		master.ID = (CreatureIndex)masterID;
		master.classType = (ClassType)(masterID - 100000000);
		master.className = className;
		masterClassStringMap.emplace(strHash(master.className.data()), &master);
		DBG_ASSERT(masterClassStringMap.find(strHash("CLASS_TYPE_STRIKER")) != masterClassStringMap.end());

		masterClassTypeMap.emplace(master.classType, &master);
	}

	return true;
}

bool GameXmlContent::LoadMasterSkinsDefinitions()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/CHARACTERSKIN.xml", doc)) return false;

	XMLElement* pSkinElt = doc.FirstChildElement()->FirstChildElement()->FirstChildElement();
	do {
		i32 skinIndex;
		pSkinElt->QueryAttribute("_SkinIndex", &skinIndex);

		const char* classType;
		pSkinElt->QueryStringAttribute("_ClassType", &classType);

		auto found = masterClassStringMap.find(strHash(classType));
		if(found == masterClassStringMap.end()) {
			WARN("class '%s' not found in masterClassMap, ignored", classType);
		}
		else {
			Master& master = *found->second;
			master.skinIDs.push_back((SkinIndex)skinIndex);
		}

		pSkinElt = pSkinElt->NextSiblingElement();
	} while(pSkinElt);

	return true;
}

bool GameXmlContent::LoadMasterWeaponDefinitions()
{
	// Parse WEAPON.xml once
	if(!LoadXMLFile(L"/WEAPON.xml", xmlWEAPON)) return false;

	XMLElement* pWeapElt = xmlWEAPON.FirstChildElement()->FirstChildElement();
	do {
		i32 ID;
		pWeapElt->QueryAttribute("ID", &ID);

		XMLElement* pItemComData = pWeapElt->FirstChildElement("ItemComData");

		const char* classType;
		pItemComData->QueryStringAttribute("_RequireClass", &classType);

		auto found = masterClassStringMap.find(strHash(classType));
		if(found == masterClassStringMap.end()) {
			WARN("class '%s' not found in masterClassMap, ignored", classType);
		}
		else {
			Master& master = *found->second;
			master.weaponIDs.push_back((WeaponIndex)ID);
		}

		pWeapElt = pWeapElt->NextSiblingElement();
	} while(pWeapElt);

	if(!LoadWeaponModelDefinitions()) {
		LOG("ERROR(LoadWeaponModelDefinitions): failed");
		return false;
	}

	return true;
}

bool GameXmlContent::LoadXMLFile(const wchar* fileName, tinyxml2::XMLDocument& xmlData)
{
	// Find a better way to check this
	// Simple path hash map?
	ASSERT_MSG(xmlData.NoChildren(), "xml already parsed");

	Path filePath = gameDataDir;
	PathAppend(filePath, fileName);

	i32 fileSize;
	u8* fileData = FileOpenAndReadAll(filePath.data(), &fileSize);
	if (!fileData) {
		LOG("ERROR(LoadXMLFile): failed to open '%ls'", filePath.data());
		return false;
	}
	defer(memFree(fileData));

	XMLError error = xmlData.Parse((char*)fileData, fileSize);
	if (error != XML_SUCCESS) {
		LOG("ERROR(LoadXMLFile): error parsing '%ls' > '%s'", filePath.data(), xmlData.ErrorStr());
		return false;
	}

	return true;
}

// FIXME: merge with LoadMasterDefinitions
bool GameXmlContent::LoadMasterDefinitionsModel()
{
	// Parse SKILLS.xml once
	if(!LoadXMLFile(L"/SKILL.xml", xmlSKILL)) return false;

	// Parse SKILL_PROPERTY.xml once
	if(!LoadXMLFile(L"/SKILL_PROPERTY.xml", xmlSKILLPROPERTY)) return false;

	LoadAllSkills(); // TODO: move

	// get master IDs
	XMLElement* pNodeMaster = xmlCREATURECHARACTER.FirstChildElement()->FirstChildElement();
	do {
		i32 masterID;
		pNodeMaster->QueryAttribute("ID", &masterID);

		auto found = masterClassTypeMap.find((ClassType)(masterID - 100000000));
		if(found == masterClassTypeMap.end());
		auto& master = found->second;

		CharacterModel &character = master->character;

		// Entity data
		XMLElement* pEntityComData = pNodeMaster->FirstChildElement("EntityComData");
		const char* entityTypeTemp;
		pEntityComData->QueryStringAttribute("_Type", &entityTypeTemp);

		// Creature data
		float moveSpeed;
		float rotateSpeed;
		float scale;

		pEntityComData->QueryFloatAttribute("_MoveSpeed", &moveSpeed);
		pEntityComData->QueryFloatAttribute("_RotateSpeed", &rotateSpeed);
		pEntityComData->QueryFloatAttribute("_Scale", &scale);

		
		XMLElement* pCreatureCompData = pNodeMaster->FirstChildElement("CreatureComData");

		const char* creatureTypeTemp;
		pCreatureCompData->QueryStringAttribute("_Type", &creatureTypeTemp);

		XMLElement* pMoveControllerComData = pNodeMaster->FirstChildElement("MoveController_ComData");

		i32 actorRadius, actorHeight;
		pMoveControllerComData->QueryIntAttribute("ActorRadius", &actorRadius);
		pMoveControllerComData->QueryIntAttribute("ActorHeight", &actorHeight);

		XMLElement* pPhysxMeshComData = pNodeMaster->FirstChildElement("PhysXMeshComData");

		i32 colliderRadius, colliderHeight;
		pPhysxMeshComData->QueryIntAttribute("_ColliderRadius", &colliderRadius);
		pPhysxMeshComData->QueryIntAttribute("_ColliderHeight", &colliderHeight);

		XMLElement* pStatsCompData = pNodeMaster->FirstChildElement("StatsComData");

		// Read character data: skills

		// read skill ids
		SkillsModel& _skillsModel = *character.getSkills();
		XMLElement* pSkillElt = pNodeMaster->FirstChildElement("SkillComData")->FirstChildElement();
		do {
			i32 skillID;
			const char* skillSlot;
			
			pSkillElt->QueryAttribute("_Index", &skillID);
			pSkillElt->QueryStringAttribute("_SkillSlot", &skillSlot);
			VERBOSE("SkillID: %d", skillID);

			//Yes this could be done in a shorter way perhaps but atleast it's not as dangerous as previous code.
			int slotIndex = GetSkillSlotIndex(skillSlot);
			if (slotIndex >= 0 && slotIndex < 4) {
				LoadMasterSkillWithID(*_skillsModel.getSkillByIndex(slotIndex), skillID);
			}
			else if (slotIndex == -2) {
				LoadMasterSkillWithID(*_skillsModel.getUltimate(), skillID);
			}
			else if (slotIndex >= -6 && slotIndex <= -3) {
				// Handle other slot types or leave empty for now
				VERBOSE("Skill slot '%s' recognized but not implemented", skillSlot);
			}
			else {
				LOG("UNSUPPORTED skillslot: %s\n", skillSlot);
			}

			pSkillElt = pSkillElt->NextSiblingElement();
		} while (pSkillElt);

		// save master data
		character.setID(masterID);
		// entity data
		character.setEntityType(StringToEntityType(entityTypeTemp));
		// creature data
		character.setCreatureType(StringToCreatureType(creatureTypeTemp));
		character.setMoveSpeed(moveSpeed);
		character.setRotateSpeed(rotateSpeed);
		character.setScale(scale);
		//MoveController data
		character.setActorHeight(actorHeight);
		character.setActorRadius(actorRadius);
		//Physx data
		character.setColliderHeight(colliderHeight);
		character.setColliderRadius(colliderRadius);
		
		pNodeMaster = pNodeMaster->NextSiblingElement();

		//LOG("DEBUG MASTER: Creature Type %d, ID %d, movement speed %f, normal movement speed %f, skill 1 ID %d", character.getCreatureType(), 
		//	character.getID(), character.getMoveSpeed(), character.getNormalMoveSpeed(), character.getSkills()->getSkillByIndex(0)->getID());
		character.Print();
	} while (pNodeMaster);

	return true;
}

void GameXmlContent::LoadAllSkills()
{
	XMLElement* pNodeSkill = xmlSKILL.FirstChildElement()->FirstChildElement();

	static const eastl::hash_map<eastl::string, SkillType> skillTypeMap = {
		{"SKILL_TYPE_PASSIVE", SkillType::PASSIVE},
		{"SKILL_TYPE_NORMAL", SkillType::NORMAL},
		{"SKILL_TYPE_TOGGLE", SkillType::TOGGLE},
		{"SKILL_TYPE_SUMMON", SkillType::SUMMON},
		{"SKILL_TYPE_STANCE", SkillType::STANCE},
		{"SKILL_TYPE_SHIRK", SkillType::SHIRK},
		{"SKILL_TYPE_COMBO", SkillType::COMBO},
		{"SKILL_TYPE_BREAKFALL", SkillType::BREAKFALL},
	};

	do {
		SkillNormalModel skill;

		i32 skillID;
		pNodeSkill->QueryAttribute("ID", &skillID);
		skill.setID(skillID);

		XMLElement* pNodeCommonSkill = pNodeSkill->FirstChildElement("ST_COMMONSKILL");

		// parse type
		const char* typeStr;
		pNodeCommonSkill->QueryStringAttribute("_Type", &typeStr);

		auto it = skillTypeMap.find(typeStr);
		SkillType type = (it != skillTypeMap.end()) ? it->second : SkillType::INVALID;

		skill.type = type;

		// parse action
		const char* actionStr = 0;
		pNodeCommonSkill->QueryStringAttribute("_Action", &actionStr);
		if(actionStr) {
			skill.action = ActionStateFromString(actionStr);
		}

		for(int i = 0; i < 6; i++) {
			SkillNormalLevelModel& _skillNormalLevelModel = *skill.getSkillNormalLevelByIndex(i);
			SetValuesSkillNormalLevel(*pNodeCommonSkill, _skillNormalLevelModel);
		}

		// TODO: slow
		LoadMasterSkillPropertyWithID(skill, skillID);

		skillMap.emplace(SkillID(skillID), skill);

		pNodeSkill = pNodeSkill->NextSiblingElement();
	} while (pNodeSkill);
}

bool GameXmlContent::LoadMasterSkillWithID(SkillNormalModel& SkillNormal, i32 skillID)
{
	XMLElement* pNodeSkill = xmlSKILL.FirstChildElement()->FirstChildElement();

	do {
		i32 _skillID;
		pNodeSkill->QueryAttribute("ID", &_skillID);
		if (_skillID == skillID)
		{
			XMLElement* pNodeCommonSkill = pNodeSkill->FirstChildElement("ST_COMMONSKILL");
			VERBOSE("Skill Match");
			const char* SkillTypeTemp;
			pNodeCommonSkill->QueryStringAttribute("_Type", &SkillTypeTemp);
			{
				SkillNormal.setID(_skillID);

				for (int i = 0; i < 6; i++)
				{
					SkillNormalLevelModel& _skillNormalLevelModel = *SkillNormal.getSkillNormalLevelByIndex(i);
					SetValuesSkillNormalLevel(*pNodeCommonSkill, _skillNormalLevelModel);
				}

				LoadMasterSkillPropertyWithID(SkillNormal, _skillID);
			}
			break;
		}

		pNodeSkill = pNodeSkill->NextSiblingElement();
	} while (pNodeSkill);
	
	return true;
}

bool GameXmlContent::LoadMasterSkillPropertyWithID(SkillNormalModel& SkillNormal, i32 skillID)
{
	XMLElement* pNodeInfo = xmlSKILLPROPERTY.FirstChildElement()->FirstChildElement();

	do {
		i32 _skillID;
		XMLElement * pNodeSkillProperty = pNodeInfo->FirstChildElement("ST_SKILL_PROPERTY");
		pNodeSkillProperty->QueryAttribute("_SkillIndex", &_skillID);

		if (_skillID == skillID)
		{
			int level = 0;
			float _temp = 0.0f;

			//Probably add check to see if this attribute exist which it should
			pNodeSkillProperty->QueryIntAttribute("_Priority", &level);

			//LOG("Skill Match");
			XMLElement* pNodePropertyLevel = pNodeSkillProperty->FirstChildElement("_PROPERTY_LEVEL");
			if(pNodePropertyLevel) { // some skills don't have a level property (180000010 for example)
				//Need -1 to correct index
				for (int i = level-1; i < 6; i++)
				{
					SkillNormalLevelModel& _skillNormalLevelModel = *SkillNormal.getSkillNormalLevelByIndex(i);
					_skillNormalLevelModel.setLevel(level);
					SetValuesSkillNormalLevel(*pNodePropertyLevel, _skillNormalLevelModel);
				}
			}
		}
	
		pNodeInfo = pNodeInfo->NextSiblingElement();
	} while (pNodeInfo);

	return true;
}

bool GameXmlContent::LoadWeaponModelDefinitions()
{
	//get weapon IDS
	XMLElement* pNodeWeapon = xmlWEAPON.FirstChildElement()->FirstChildElement();
	do {
		//weaponModel.push_back();
		//WeaponModel& weapon = weaponModel.back();

		i32 weaponID;
		pNodeWeapon->QueryAttribute("ID", &weaponID);

		// WeaponComData
		XMLElement* pWeaponComData = pNodeWeapon->FirstChildElement("WeaponComData");
		float attack, pvpattack;
		
		pWeaponComData->QueryFloatAttribute("_Attack", &attack);
		pWeaponComData->QueryFloatAttribute("_PvPAttack", &pvpattack);

		// save weapon data
		//weapon.setAttack(attack);

		pNodeWeapon = pNodeWeapon->NextSiblingElement();
	} while (pNodeWeapon);

	return true;
}

void GameXmlContent::SetValuesSkillNormalLevel(XMLElement& pNodeCommonSkill, SkillNormalLevelModel& _skillNormalLevelModel)
{
	f32 _temp;
	i64 ival;

	if (pNodeCommonSkill.QueryFloatAttribute("_AddGroggy", &_temp) == XML_SUCCESS)
	{
		_skillNormalLevelModel.setAddGroggy(_temp);
	}
	if (pNodeCommonSkill.QueryFloatAttribute("_AttackMultiplier", &_temp) == XML_SUCCESS)
	{
		_skillNormalLevelModel.setAttackMultiplier(_temp);
	}
	if (pNodeCommonSkill.QueryFloatAttribute("_BaseDamage", &_temp) == XML_SUCCESS)
	{
		_skillNormalLevelModel.setBaseDamage(_temp);
	}
	if (pNodeCommonSkill.QueryFloatAttribute("_ConsumeEP", &_temp) == XML_SUCCESS)
	{
		_skillNormalLevelModel.setConsumeEP(_temp);
	}
	if (pNodeCommonSkill.QueryFloatAttribute("_ConsumeMP", &_temp) == XML_SUCCESS)
	{
		_skillNormalLevelModel.setConsumeMP(_temp);
	}
	if (pNodeCommonSkill.QueryFloatAttribute("_ConsumeUG", &_temp) == XML_SUCCESS)
	{
		_skillNormalLevelModel.setConsumeUG(_temp);
	}
	if (pNodeCommonSkill.QueryFloatAttribute("_CoolTime", &_temp) == XML_SUCCESS)
	{
		_skillNormalLevelModel.setCoolTime(_temp);
	}
	if (pNodeCommonSkill.QueryInt64Attribute("_SkillIndex", &ival) == XML_SUCCESS)
	{
		_skillNormalLevelModel.setSkillIndex(ival);
	}
	if (pNodeCommonSkill.QueryFloatAttribute("_SkillRangeLengthX", &_temp) == XML_SUCCESS)
	{
		_skillNormalLevelModel.setSkillRangeLengthX(_temp);
	}
	if (pNodeCommonSkill.QueryFloatAttribute("_SkillRangeLengthY", &_temp) == XML_SUCCESS)
	{
		_skillNormalLevelModel.setSkillRangeLengthY(_temp);
	}
	if (pNodeCommonSkill.QueryFloatAttribute("_TargetMaxDistance", &_temp) == XML_SUCCESS)
	{
		_skillNormalLevelModel.setTargetMaxDistance(_temp);
	}
	if (pNodeCommonSkill.QueryFloatAttribute("_TargetRangeLengthX", &_temp) == XML_SUCCESS)
	{
		_skillNormalLevelModel.setTargetRangeLengthX(_temp);
	}
	if (pNodeCommonSkill.QueryFloatAttribute("_TargetRangeLengthY", &_temp) == XML_SUCCESS)
	{
		_skillNormalLevelModel.setTargetRangeLengthY(_temp);
	}
}

//Todo make this loop and finish it
void GameXmlContent::SetWeaponSpecRef(XMLElement& pNodeWeaponSpecRef, WeaponSpec& _weaponSpec)
{
	// Define a function pointer type for WeaponSpec setter methods
	using SetterFunction = void (WeaponSpec::*)(float);

	// Create a lookup table mapping weapon spec references to setter functions
	static const eastl::hash_map<eastl::string, SetterFunction> weaponSpecSetters = {
		{"WEAPONSPEC_REF_FIRINGMETHOD_CHARGING_TIME_LEVEL1", &WeaponSpec::setFiringMethodChargingTimeLevel1},
		{"WEAPONSPEC_REF_FIRINGMETHOD_CHARGING_TIME_LEVEL2", &WeaponSpec::setFiringMethodChargingTimeLevel2},
		{"WEAPONSPEC_REF_FIRINGMETHOD_CHARGING_TIME_LEVEL3", &WeaponSpec::setFiringMethodChargingTimeLevel3},
		{"WEAPONSPEC_REF_FIRINGMETHOD_COMBO_FIREDELAY", &WeaponSpec::setFiringMethodComboFireDelay},
		{"WEAPONSPEC_REF_FIRINGMETHOD_COMBO_VALIDTIME", &WeaponSpec::setFiringMethodComboValidTime},
		{"WEAPONSPEC_REF_FIRINGMETHOD_CONSUMPTION", &WeaponSpec::setFiringMethodConsumption},
		{"WEAPONSPEC_REF_FIRINGMETHOD_CONSUMPTION_BY_CHARGING", &WeaponSpec::setFiringMethodConsumptionByCharging},
		{"WEAPONSPEC_REF_FIRINGMETHOD_ERRORANGLE", &WeaponSpec::setFiringMethodErrorAngle},
		{"WEAPONSPEC_REF_FIRINGMETHOD_FIREDELAY", &WeaponSpec::setFiringMethodComboFireDelay},
		{"WEAPONSPEC_REF_FIRINGMETHOD_MAXDISTANCE", &WeaponSpec::setFiringMethodMaxDistance},
		{"WEAPONSPEC_REF_FIRINGMETHOD_MINDISTANCE", &WeaponSpec::setFiringMethodMinDistance},
		{"WEAPONSPEC_REF_FIRINGMETHOD_CHARGING_LEVEL_1_CREATE_RADIATE_MAXNUM", &WeaponSpec::setFiringMethodChargingLevel1CreateRadiateMaxNum},
		{"WEAPONSPEC_REF_FIRINGMETHOD_CHARGING_LEVEL_2_CREATE_RADIATE_MAXNUM", &WeaponSpec::setFiringMethodChargingLevel2CreateRadiateMaxNum},
		{"WEAPONSPEC_REF_FIRINGMETHOD_CREATE_RADIATE_MAXANGBLE", &WeaponSpec::setFiringMethodCreateRadiateMaxAngBle},
		{"WEAPONSPEC_REF_FIRINGMETHOD_CREATE_RADIATE_MAXNUM", &WeaponSpec::setFiringMethodCreateRadiateMaxNum},
		{"WEAPONSPEC_REF_FIRINGMETHOD_FIRINGSTAT_MOVEMENT_SPEED", &WeaponSpec::setFiringMethodFiringStatMovementSpeed},
		{"WEAPONSPEC_REF_FIRINGMETHOD_GAUGE_LEVEL", &WeaponSpec::setFiringMethodGaugeLevel},
		{"WEAPONSPEC_REF_FIRINGMETHOD_CHARGING_LEVEL_1_CREATE_PARALLEL_MAXNUM", &WeaponSpec::setFiringMethodChargingLevel1CreateParallelMaxNum},
		{"WEAPONSPEC_REF_FIRINGMETHOD_CHARGING_LEVEL_2_CREATE_PARALLEL_MAXNUM", &WeaponSpec::setFiringMethodChargingLevel2CreateParallelMaxNum},
		{"WEAPONSPEC_REF_FIRINGMETHOD_CHARGING_LEVEL_1_ANIMATION_MOVEHORIZON", &WeaponSpec::setFiringMethodChargingLevel1AnimationMoveHorizon},
		{"WEAPONSPEC_REF_FIRINGMETHOD_CHARGING_LEVEL_2_ANIMATION_MOVEHORIZON", &WeaponSpec::setFiringMethodChargingLevel2AnimationMoveHorizon},
		{"WEAPONSPEC_REF_FIRINGMETHOD_GAUGE_AUTOREGEN", &WeaponSpec::setFiringMethodGaugeAutoRegen},
		{"WEAPONSPEC_REF_REMOTE_ANGLE", &WeaponSpec::setRemoteAngle},
		{"WEAPONSPEC_REF_REMOTE_ATTACKMULTIPLIER", &WeaponSpec::setRemoteAttackMultiplier},
		{"WEAPONSPEC_REF_REMOTE_DAMAGEONBOUND", &WeaponSpec::setRemoteDamageOnBound},
		{"WEAPONSPEC_REF_REMOTE_FOV", &WeaponSpec::setRemoteFOV},
		{"WEAPONSPEC_REF_REMOTE_LENGTH_X", &WeaponSpec::setRemoteLengthX},
		{"WEAPONSPEC_REF_REMOTE_LENGTH_Y", &WeaponSpec::setRemoteLengthY},
		{"WEAPONSPEC_REF_REMOTE_MAXDISTANCE", &WeaponSpec::setRemoteMaxDistance},
		{"WEAPONSPEC_REF_REMOTE_MAXSCALE", &WeaponSpec::setRemoteMaxScale},
		{"WEAPONSPEC_REF_REMOTE_MINSCALE", &WeaponSpec::setRemoteMinScale},
		{"WEAPONSPEC_REF_REMOTE_MAXSPEED", &WeaponSpec::setRemoteMaxSpeed},
		{"WEAPONSPEC_REF_REMOTE_PENETRATIONCOUNT", &WeaponSpec::setRemotePenetrationCount},
		{"WEAPONSPEC_REF_REMOTE_SIGHT", &WeaponSpec::setRemoteSight},
		{"WEAPONSPEC_REF_REMOTE_STATUS_RATE", &WeaponSpec::setRemoteStatusRate},
		{"WEAPONSPEC_REF_REMOTE_CRITICALRATE", &WeaponSpec::setRemoteCriticalRate},
		{"WEAPONSPEC_REF_REMOTE_SUBBOUNDLENGTH", &WeaponSpec::setRemoteSubBoundLenght},
		{"WEAPONSPEC_REF_ACTIONBASE_PARAM_1", &WeaponSpec::setActionBaseParam1},
		{"WEAPONSPEC_REF_STATUS_DISTANCE", &WeaponSpec::setStatusDistance},
		{"WEAPONSPEC_REF_STATUS_REGEN_AND_HEALTH", &WeaponSpec::setStatusRegenAndHealth},
		{"WEAPONSPEC_REF_STATUS_STAT_MOVEMENT_SPEED", &WeaponSpec::setStatusStatMovementSpeed},
		{"WEAPONSPEC_REF_STATUS_STAT_DEFENCE", &WeaponSpec::setStatusStatDefence},
		{"WEAPONSPEC_REF_STATUS_DURATION_TIME", &WeaponSpec::setStatusDurationTime},
		{"WEAPONSPEC_REF_STATUS_MAX_OVERLAP_COUNT", &WeaponSpec::setStatusMaxOverlapCount},
		{"WEAPONSPEC_REF_STATUS_DOT_DAMAGE_MULTIPLIER", &WeaponSpec::setStatusDotDamageMultiplier},
		{"WEAPONSPEC_REF_ATTACK", &WeaponSpec::setAttack},
		{"WEAPONSPEC_REF_STAT_AND_CRITICALDAMAGE", &WeaponSpec::setStatAndCriticalDamage},
		{"WEAPONSPEC_REF_STAT_AND_CRITICALRATE", &WeaponSpec::setStatAndCriticalRate}
	};

	const char* weaponSpecREF;
	float _temp = 0.0f;

	pNodeWeaponSpecRef.QueryStringAttribute("ref", &weaponSpecREF);
	pNodeWeaponSpecRef.QueryFloatAttribute("value", &_temp);

	// Look up the setter function and call it
	auto it = weaponSpecSetters.find(weaponSpecREF);
	if (it != weaponSpecSetters.end()) {
		(_weaponSpec.*(it->second))(_temp);
	}
	else {
		LOG("ERROR(SetWeaponREFSet): Unsupported WEAPONSPEC_REF %s", weaponSpecREF);
	}
}

GameSubModeType GameXmlContent::StringToGameSubModeType(const char* s)
{
	static const eastl::hash_map<eastl::string, GameSubModeType> gameModeMap = {
		{"GAME_SUB_MODE_DEATH_MATCH_NORMAL", GameSubModeType::GAME_SUB_MODE_DEATH_MATCH_NORMAL},
		{"GAME_SUB_MODE_OCCUPY_CORE", GameSubModeType::GAME_SUB_MODE_OCCUPY_CORE},
		{"GAME_SUB_MODE_OCCUPY_BUSH", GameSubModeType::GAME_SUB_MODE_OCCUPY_BUSH},
		{"GAME_SUB_MODE_GOT_AUTHENTIC", GameSubModeType::GAME_SUB_MODE_GOT_AUTHENTIC},
		{"GAME_SUB_MODE_GOT_TUTORIAL_BASIC", GameSubModeType::GAME_SUB_MODE_GOT_TUTORIAL_BASIC},
		{"GAME_SUB_MODE_GOT_TUTORIAL_EXPERT", GameSubModeType::GAME_SUB_MODE_GOT_TUTORIAL_EXPERT},
		{"GAME_SUB_MODE_GOT_FIRE_POWER", GameSubModeType::GAME_SUB_MODE_GOT_FIRE_POWER},
		{"GAME_SUB_MODE_GOT_ULTIMATE_TITAN", GameSubModeType::GAME_SUB_MODE_GOT_ULTIMATE_TITAN},
		{"GAME_SUB_MODE_SPORTS_RUN", GameSubModeType::GAME_SUB_MODE_SPORTS_RUN},
		{"GAME_SUB_MODE_SPORTS_SURVIVAL", GameSubModeType::GAME_SUB_MODE_SPORTS_SURVIVAL},
		{"GAME_SUB_MODE_STAGE_TUTORIAL", GameSubModeType::GAME_SUB_MODE_STAGE_TUTORIAL},
		{"GAME_SUB_MODE_STAGE_NORMAL", GameSubModeType::GAME_SUB_MODE_STAGE_NORMAL}
	};

	auto it = gameModeMap.find(s);
	if (it != gameModeMap.end()) {
		return it->second;
	}

	LOG("ERROR(StringToGameSubModeType): Unsupported subGameMode %s", s);
	return GameSubModeType::GAME_SUB_MODE_INVALID;
}

MapType GameXmlContent::StringToMapType(const char* s)
{
	static const eastl::hash_map<eastl::string, MapType> mapTypeMap = {
		{"E_MAP_TYPE_CITY", MapType::MAP_CITY},
		{"E_MAP_TYPE_INGAME", MapType::MAP_INGAME}
	};

	auto it = mapTypeMap.find(s);
	if (it != mapTypeMap.end()) {
		return it->second;
	}

	LOG("ERROR(StringToMapType): Unsupported map type %s", s);
	return MapType::MAP_INVALID; // Assuming this exists
}

bool GameXmlContent::LoadMapList()
{
	XMLDocument doc;
	if (!LoadXMLFile(L"/MAPLIST.xml", doc)) return false;

	XMLElement* pMapElt = doc.FirstChildElement()->FirstChildElement()->FirstChildElement()->FirstChildElement();
	do {
		MapList mapList;
		pMapElt->QueryAttribute("_Index", &mapList.index);

		const char* levelFileTemp;
		pMapElt->QueryStringAttribute("_LevelFile", &levelFileTemp);
		mapList.levelFile = levelFileTemp;

		const char* mapTypeXml;
		pMapElt->QueryStringAttribute("_MapType", &mapTypeXml);

		mapList.mapType = StringToMapType(mapTypeXml);
		mapList.gameSubModeType = GameSubModeType::GAME_SUB_MODE_INVALID;

		if (mapList.mapType == MapType::MAP_INGAME) {
			const char* gameSubModeTypeXml;
			if (pMapElt->QueryStringAttribute("_GameSubModeType", &gameSubModeTypeXml) == XML_SUCCESS) {
				mapList.gameSubModeType = StringToGameSubModeType(gameSubModeTypeXml);
			}
			// training room doesn't have a gamesubmode, so this is OK
		}

		maplists.push_back(mapList);
		pMapElt = pMapElt->NextSiblingElement();
	} while (pMapElt);

	return true;
}

bool GameXmlContent::LoadMapByID(Map* map, i32 index)
{
	const MapList* mapList = FindMapListByID(index);
	if (!mapList){
		LOG("ERROR(LoadMapByID): Map not found %d", index);
		return false;
	}

	wchar_t tempPathData[256] = {0};
	wchar_t* tempPath = tempPathData;
	const char* levelFileString = mapList->levelFile.data();

	eastl::DecodePart(levelFileString, mapList->levelFile.data() + EA::StdC::Strlen(mapList->levelFile.data()), tempPath, tempPathData + sizeof(tempPathData));

	XMLDocument doc;
	if(!LoadXMLFile(LFMT(L"/%s/Spawn.xml", tempPathData), doc)) return false;

	XMLElement* pMapInfo = doc.FirstChildElement();
	XMLElement* pMapEntityCreature = pMapInfo->FirstChildElement();
	XMLElement* pSpawnElt = pMapEntityCreature->FirstChildElement();
	for(pSpawnElt = pMapEntityCreature->FirstChildElement(); pSpawnElt; pSpawnElt = pSpawnElt->NextSiblingElement()) {
		Map::Spawn spawn;
		pSpawnElt->QueryAttribute("dwDoc", (i32*)&spawn.docID);
		pSpawnElt->QueryAttribute("dwID", (i32*)&spawn.localID);
		pSpawnElt->QueryAttribute("kTranslate_x", &spawn.pos.x);
		pSpawnElt->QueryAttribute("kTranslate_y", &spawn.pos.y);
		pSpawnElt->QueryAttribute("kTranslate_z", &spawn.pos.z);
		pSpawnElt->QueryAttribute("kRotation_x", &spawn.rot.x);
		pSpawnElt->QueryAttribute("kRotation_y", &spawn.rot.y);
		pSpawnElt->QueryAttribute("kRotation_z", &spawn.rot.z);

		spawn.type = Map::Spawn::Type::NORMAL;
		bool returnPoint;
		if (pSpawnElt->QueryAttribute("ReturnPoint", &returnPoint) == XML_SUCCESS) {
			spawn.type = Map::Spawn::Type::SPAWN_POINT;
		}

		spawn.faction = Faction::INVALID;
		const char* teamString;
		if(pSpawnElt->QueryStringAttribute("team", &teamString) == XML_SUCCESS) {
			spawn.faction = StringToFaction(teamString);
		}

		map->creatures.push_back(spawn);
	}

	XMLElement* pMapEntityDynamic = pMapEntityCreature->NextSiblingElement();
	for(XMLElement* pSpawnElt = pMapEntityDynamic->FirstChildElement(); pSpawnElt; pSpawnElt = pSpawnElt->NextSiblingElement()) {
		Map::Spawn spawn;
		pSpawnElt->QueryAttribute("dwDoc", (i32*)&spawn.docID);
		pSpawnElt->QueryAttribute("dwID", (i32*)&spawn.localID);
		pSpawnElt->QueryAttribute("kTranslate_x", &spawn.pos.x);
		pSpawnElt->QueryAttribute("kTranslate_y", &spawn.pos.y);
		pSpawnElt->QueryAttribute("kTranslate_z", &spawn.pos.z);
		pSpawnElt->QueryAttribute("kRotation_x", &spawn.rot.x);
		pSpawnElt->QueryAttribute("kRotation_y", &spawn.rot.y);
		pSpawnElt->QueryAttribute("kRotation_z", &spawn.rot.z);

		spawn.type = Map::Spawn::Type::NORMAL;

		spawn.faction = Faction::INVALID;
		const char* teamString;
		if(pSpawnElt->QueryStringAttribute("team", &teamString) == XML_SUCCESS) {
			spawn.faction = StringToFaction(teamString);
		}

		map->dynamic.push_back(spawn);
	}


	// Areas
	XMLDocument areasXml;
	if(!LoadXMLFile(LFMT(L"/%s/Area.xml", tempPathData), areasXml)) return false;

	pMapInfo = areasXml.FirstChildElement();
	XMLElement* pMapAreaInfo = pMapInfo->FirstChildElement();
	for(XMLElement* pAreaList = pMapAreaInfo->FirstChildElement(); pAreaList; pAreaList = pAreaList->NextSiblingElement()) {
		Map::Area area;
		pAreaList->QueryAttribute("dwID", &area.ID);
		pAreaList->QueryAttribute("dwType", &area.type);
		// TODO: hard to judge what width, height and length represent
		pAreaList->QueryAttribute("fWidth", &area.size.z);
		pAreaList->QueryAttribute("fHeight", &area.size.y);
		pAreaList->QueryAttribute("fLength", &area.size.x);
		pAreaList->QueryAttribute("kTranslate_x", &area.pos.x);
		pAreaList->QueryAttribute("kTranslate_y", &area.pos.y);
		pAreaList->QueryAttribute("kTranslate_z", &area.pos.z);
		pAreaList->QueryAttribute("kRotation_x", &area.rot.x);
		pAreaList->QueryAttribute("kRotation_y", &area.rot.y);
		pAreaList->QueryAttribute("kRotation_z", &area.rot.z);
		pAreaList->QueryAttribute("layer", &area.layer);
		map->areas.push_back(area);
	}

	return true;
}


bool GameXmlContent::LoadLobby(i32 index)
{
	const MapList* map = FindMapListByID(index);
	if (!map) {
		LOG("ERROR(LoadLobby): Map not found %d", index);
		return false;
	};

	if (map->mapType != MapType::MAP_CITY)
	{
		LOG("ERROR(LoadLobby): Map index: %d is not from MapType MAP_CITY", index);
		return false;
	}

	return LoadMapByID(&mapLobby, index);
}

bool GameXmlContent::LoadPvpDeathmach()
{
	const MapList* map = FindMapListByID(160000094);
	if (!map) {
		LOG("ERROR(LoadPvpDeathmach): Map not found %d", 160000094);
		return false;
	};

	return LoadMapByID(&mapPvpDeathMatch, 160000094);
}

bool GameXmlContent::LoadJukeboxSongs()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/JUKEBOX.xml", doc)) return false;

	// TODO: load spawns from "MAP_ENTITY_TYPE_DYNAMIC" as well
	XMLElement* pSongElt = doc.FirstChildElement()->FirstChildElement();
	do {
		Song song;
		pSongElt->QueryAttribute("ID", (i32*)&song.ID);

		XMLElement* pJukeboxElt = pSongElt->FirstChildElement();
		pJukeboxElt->QueryAttribute("_Time", &song.length);

		jukeboxSongs.push_back(song);

		pSongElt = pSongElt->NextSiblingElement();
	} while(pSongElt);

	return true;
}

static bool FileLoad(FileBuffer* out, const wchar* path)
{
	out->data = FileOpenAndReadAll(path, &out->size);
	if(!out->data) {
		LOG("ERROR(FileLoad): failed to open '%ls'", path);
		return false;
	}

	return true;
}

bool GameXmlContent::LoadCollisionMeshes()
{
	Path path = gameDataDir;
	PathAppend(path, L"/PVP_DeathMatch01_Collision.physx_static");
	bool r = FileLoad(&filePvpDeathmatch01Collision, path.data());
	if(!r) return false;

	path = gameDataDir;
	PathAppend(path, L"/PVP_DeathMatch01_Env.physx_static");
	r = FileLoad(&filePvpDeathmatch01CollisionWalls, path.data());
	if(!r) return false;

	path = gameDataDir;
	PathAppend(path, L"/PvP_Death_NM_Wall04.physx_static");
	r = FileLoad(&filePvpDeathNmWall04, path.data());
	if(!r) return false;

	path = gameDataDir;
	PathAppend(path, L"/cylinder.physx_dynamic");
	r = FileLoad(&fileCylinderCollision, path.data());
	if(!r) return false;

	return true;
}

bool GameXmlContent::LoadAnimationData()
{
	XMLDocument xmlAniLength;
	if(!LoadXMLFile(L"/AniLength.xml", xmlAniLength)) return false;

	struct SequenceLength
	{
		i32 ID;
		f32 length;
	};

	eastl::fixed_vector<SequenceLength, 18000, false>* aniLenList;
	eastl::fixed_hash_map<ClassType, Slice<SequenceLength>, 800> aniLenListMap;

	aniLenList = new eastl::remove_reference<decltype(*aniLenList)>::type();
	defer(delete aniLenList);

	for(XMLElement* pEntityType = xmlAniLength.FirstChildElement()->FirstChildElement();
		pEntityType;
		pEntityType = pEntityType->NextSiblingElement()) {

		for(XMLElement* pEntity = pEntityType->FirstChildElement();
			pEntity;
			pEntity = pEntity->NextSiblingElement()) {

			const char* _Key;
			pEntity->QueryStringAttribute("_Key", &_Key);
			//LOG("_Key='%s'", _Key);

			const i32 startIdx = aniLenList->size();

			for(XMLElement* pAnimationInfo = pEntity->FirstChildElement();
				pAnimationInfo;
				pAnimationInfo = pAnimationInfo->NextSiblingElement()) {

				i32 _SeqID;
				f32 _SeqTime;
				pAnimationInfo->QueryAttribute("_SeqID", &_SeqID);
				pAnimationInfo->QueryAttribute("_SeqTime", &_SeqTime);
				//LOG("%d %f", _SeqID, _SeqTime);

				aniLenList->push_back({ _SeqID, _SeqTime });
			}

			aniLenListMap.emplace(ClassTypeFromString(_Key), Slice<SequenceLength>(&(*aniLenList)[startIdx], aniLenList->size() - startIdx));
		}
	}

	XMLDocument xmlActionBase;
	if(!LoadXMLFile(L"/ActionBase.xml", xmlActionBase)) return false;

	ClassType prevMasterClassType = ClassType::NONE;
	ActionStateID prevActionID = ActionStateID::INVALID;
	i32 actionSliceStart = 0;
	i32 actionSliceCount = 0;
	Action* curAction = nullptr;
	f32 accumulatedDelay = 0;

	for(XMLElement* pActionBase = xmlActionBase.FirstChildElement()->FirstChildElement();
		pActionBase;
		pActionBase = pActionBase->NextSiblingElement()) {

		const char* Class = nullptr;
		pActionBase->QueryStringAttribute("Class", &Class);
		eastl::fixed_string<char,64,false> classStr = Class;
		classStr.make_upper();

		const ClassType masterClassType = ClassTypeFromString(classStr.data());
		if(masterClassType != prevMasterClassType) {
			LOG("%s:", classStr.data());

			if(actionSliceCount > 0) {
				actionListMap.emplace(prevMasterClassType, Slice<Action>(&actionList[actionSliceStart], actionSliceCount));
			}

			prevMasterClassType = masterClassType;
			actionSliceStart = actionList.size();
			actionSliceCount = 0;
			prevActionID = ActionStateID::INVALID;
		}

		const char* BehaviorState = nullptr;
		i32 AnimationId = -1;
		pActionBase->QueryStringAttribute("BehaviorState", &BehaviorState);
		pActionBase->QueryAttribute("AnimationId", &AnimationId);

		const ActionStateID actionID = ActionStateFromString(BehaviorState);
		ASSERT(actionID != ActionStateID::INVALID);

		if(actionID != prevActionID) {
			prevActionID = actionID;

			bool foundAction = false;
			for(int i = actionSliceStart; i < actionSliceStart+actionSliceCount; i++) {
				if(actionList[i].ID == actionID) {
					curAction = &actionList[i];
					foundAction = true;
					break;
				}
			}

			if(!foundAction) {
				actionList.push_back();
				curAction = &actionList.back();
				actionSliceCount++;
				curAction->ID = actionID;
				auto foundSeq = aniLenListMap.find(masterClassType);
				if(foundSeq != aniLenListMap.end()) {
					foreach_const(it, foundSeq->second) {
						if(it->ID == AnimationId) {
							curAction->seqLength = it->length;

							break;
						}
					}
				}

				LOG("	ID='%s' (%d)", ActionStateToString(curAction->ID), curAction->ID);
				LOG("		seqLength=%f", curAction->seqLength);

				accumulatedDelay = 0.0f;
			}
		}

		const char* CommandType = nullptr;
		if(pActionBase->QueryStringAttribute("CommandType", &CommandType) != XMLError::XML_SUCCESS) {
			// FIXME: handle this case, it has AnimationLoop?
			continue;
		}

		Action::Command cmd;
		cmd.type = ActionCommand::TypeFromString(CommandType);
		if(pActionBase->QueryAttribute("Delay", &cmd.delay) != XMLError::XML_SUCCESS) {
			cmd.delay = 0.0f;
		}
		cmd.relativeEndTimeFromStart = accumulatedDelay + cmd.delay;
		accumulatedDelay += cmd.delay;

		switch(cmd.type) {
			case ActionCommand::Type::STATE_BLOCK: {

			} break;

			case ActionCommand::Type::MOVE: {
				const char* Param1 = 0;
				const char* Param2 = 0;
				pActionBase->QueryStringAttribute("Param1", &Param1);
				pActionBase->QueryStringAttribute("Param2", &Param2);

				cmd.move.preset = ActionCommand::MovePresetFromString(Param1);
				ASSERT(cmd.move.preset != ActionCommand::MovePreset::INVALID);

				if(Param2 != nullptr) {
					i32 tmp;
					i32 r = EA::StdC::Sscanf(Param2, "%d|%d", &cmd.move.param2, &tmp);
					ASSERT(r == 2);
				}
			} break;

			case ActionCommand::Type::REMOTE: {
				const char* TargetPreset;
				//const char* Param2;
				i32 RemoteIndex;
				pActionBase->QueryStringAttribute("TargetPreset", &TargetPreset);
				//pActionBase->QueryStringAttribute("Param2", &Param2);
				pActionBase->QueryAttribute("RemoteIndex", &RemoteIndex);

				cmd.remote.idx = RemoteIdx(RemoteIndex);
				cmd.remote.targetPreset = ActionCommand::TargetPresetFromString(TargetPreset);
				ASSERT(cmd.remote.targetPreset != ActionCommand::TargetPreset::INVALID);
			} break;

			case ActionCommand::Type::GRAPH_MOVE_HORZ: {
				pActionBase->QueryAttribute("Param1", &cmd.graphMoveHorz.distance);
			} break;

			case ActionCommand::Type::ROTATESPEED: {
				pActionBase->QueryAttribute("Param1", &cmd.rotateSpeed.speed);
			} break;
		}

		curAction->commands.push_back(cmd);

		LOG("		Command='%s' delay=%.2f relative=%.2f", CommandType, cmd.delay, accumulatedDelay);
		switch(cmd.type) {
			case ActionCommand::Type::STATE_BLOCK: {

			} break;

			case ActionCommand::Type::MOVE: {
				LOG("		  preset='%s'", ActionCommand::MovePresetToString(cmd.move.preset));
				LOG("		  param2=%d", cmd.move.param2);
			} break;

			case ActionCommand::Type::GRAPH_MOVE_HORZ: {
				LOG("		  distance=%f", cmd.graphMoveHorz.distance);
			} break;

			case ActionCommand::Type::ROTATESPEED: {
				LOG("		  speed=%d", cmd.rotateSpeed.speed);
			} break;
		}
	}

	if(actionSliceCount > 0) {
		actionListMap.emplace(prevMasterClassType, Slice<Action>(&actionList[actionSliceStart], actionSliceCount));
	}

	return true;
}

bool GameXmlContent::LoadRemoteData()
{
	// TODO: use these everywhere?
#define ELT_GET_STR(COMP, STR)\
	const char* STR = 0;\
	pComp->QueryStringAttribute(#STR, &STR)\

#define ELT_GET(COMP, T, V, V_DEFAULT)\
	T V = V_DEFAULT;\
	pComp->QueryAttribute(#V, &V)\

	XMLDocument xml;
	if(!LoadXMLFile(L"/REMOTE_PC.xml", xml)) return false;

	for(XMLElement* pEntityInfo = xml.FirstChildElement()->FirstChildElement();
		pEntityInfo;
		pEntityInfo = pEntityInfo->NextSiblingElement()) {

		Remote remote;

		const char* KEYNAME = nullptr;
		i32 ID = 0;
		pEntityInfo->QueryStringAttribute("KEYNAME", &KEYNAME);
		pEntityInfo->QueryAttribute("ID", &ID);

		remote.ID = RemoteIdx(ID);

		LOG("Remote: { ID=%u, KEYNAME='%s' }", ID, KEYNAME);

		for(XMLElement* pComp = pEntityInfo->FirstChildElement();
			pComp;
			pComp = pComp->NextSiblingElement()) {
			const char* compName = pComp->Name();

			if((EA::StdC::Strcmp("RemoteBoundComData2", compName) == 0)) {
				ELT_GET(pComp, i32, _LengthX, 0);
				ELT_GET(pComp, i32, _LengthY, 0);
				ELT_GET(pComp, i32, _LengthZ, 0);
				ELT_GET_STR(pComp, _Type);
				ELT_GET_STR(pComp, _DamageGroup);
				ELT_GET(pComp, bool, _VsDynamic, false);
				ELT_GET(pComp, bool, _VsNPC_Monster, false);
				ELT_GET(pComp, bool, _VsPC, false);

				remote.boundType = Remote::BoundTypeFromString(_Type);
				remote.damageGroup = Remote::DamageGroupFromString(_DamageGroup);
				remote.vs =
					(_VsDynamic << Remote::VS_DYNAMIC) |
					(_VsNPC_Monster << Remote::VS_NPC_MONSTER) |
					(_VsPC << Remote::VS_PLAYER_CHARACTER);

				LOG("	_LengthX=%d _LengthY=%d _LengthZ=%d", _LengthX, _LengthY, _LengthZ);
				LOG("	_DamageGroup=%s _Type=%s _VsX=%#x", Remote::DamageGroupToString(remote.damageGroup), Remote::BoundTypeToString(remote.boundType), remote.vs);
			}

			else if((EA::StdC::Strcmp("RemoteComData2", compName) == 0)) {
				ELT_GET(pComp, i32, _ActivateCount, 0);
				ELT_GET(pComp, i32, _AttackMultiplier, 0);

				const char* _BehaviorType = 0;
				pComp->QueryStringAttribute("_BehaviorType", &_BehaviorType);
				if(_BehaviorType) {
					remote.behaviorType = Remote::BehaviourTypeFromString(_BehaviorType);
				}

				LOG("	_ActivateCount=%d _ActivateMultiplier=%d", _ActivateCount, _AttackMultiplier);
				LOG("	_BehaviorType=%s", Remote::BehaviourTypeToString(remote.behaviorType));
			}
		}

		remoteMap.emplace(remote.ID, remote);
	}

	return true;
}

bool GameXmlContent::Load()
{
	LOG("Loading GameContent...");

	bool r = LoadMasterDefinitions();
	if(!r) return false;

	r = LoadMasterSkinsDefinitions();
	if(!r) return false;

	r = LoadMasterWeaponDefinitions();
	if(!r) return false;

	r = LoadMasterDefinitionsModel();
	if (!r) return false;

	r = LoadMapList();
	if (!r) return false;

	r = LoadLobby(160000042);
	if (!r) return false;

	r = LoadPvpDeathmach();
	if (!r) return false;

	r = LoadJukeboxSongs();
	if(!r) return false;

	r = LoadCollisionMeshes();
	if(!r) return false;

	r = LoadAnimationData();
	if(!r) return false;

	r = LoadRemoteData();
	if(!r) return false;

	/*
	LOG("Masters:");
	eastl::fixed_string<char,1024> buff;
	foreach(it, masters) {
		LOG("%s: ID=%d", it->className.data(), (i32)it->ID);

		buff.clear();
		foreach(s, it->skillIDs) {
			buff.append(FMT("%d, ", *s));
		}
		LOG("	skills=[%s]", buff.data());

		buff.clear();
		foreach(s, it->skinIDs) {
			buff.append(FMT("%d, ", *s));
		}
		LOG("	skins=[%s]", buff.data());

		buff.clear();
		foreach(s, it->weaponIDs) {
			buff.append(FMT("%d, ", *s));
		}
		LOG("	weapons=[%s]", buff.data());
	}

	LOG("Lobby_Normal:");
	foreach(it, mapLobby.creatures) {
		LOG("Spawn :: docID=%d localID=%d pos=(%g, %g, %g) rot=(%g, %g, %g)", (i32)it->docID, it->localID, it->pos.x, it->pos.y, it->pos.z, it->rot.x, it->rot.y, it->rot.z);
	}

	LOG("Jukebox songs:");
	foreach(it, jukeboxSongs) {
		LOG("ID=%d length=%d", (i32)it->ID, it->length);
	}
	*/

	LOG("GameContent successfully loaded.");
	return true;
}

Faction GameXmlContent::StringToFaction(const char* s)
{
	static const eastl::hash_map<eastl::string, Faction> factionMap = {
		{"TEAM_RED", Faction::RED},
		{"TEAM_BLUE", Faction::BLUE},
		{"TEAM_DYNAMIC", Faction::DYNAMIC}
	};

	auto it = factionMap.find(s);
	return (it != factionMap.end()) ? it->second : Faction::INVALID;
}

// Returns CREATURE_TYPE_INVALID when it doesn't recognises the creature type.
CreatureType GameXmlContent::StringToCreatureType(const char* s)
{
	static const eastl::hash_map<eastl::string, CreatureType> creatureTypeMap = {
		{"CREATURE_TYPE_ALLY", CreatureType::CREATURE_TYPE_ALLY},
		{"CREATURE_TYPE_BOT", CreatureType::CREATURE_TYPE_BOT},
		{"CREATURE_TYPE_MONSTER", CreatureType::CREATURE_TYPE_MONSTER},
		{"CREATURE_TYPE_NPC", CreatureType::CREATURE_TYPE_NPC},
		{"CREATURE_TYPE_PC", CreatureType::CREATURE_TYPE_PC}
	};

	auto it = creatureTypeMap.find(s);
	if (it != creatureTypeMap.end()) {
		return it->second;
	}

	LOG("Unknown CreatureType: %s", s);
	return CreatureType::CREATURE_TYPE_INVALID;
}

// Returns ENTITY_INVALID when it doesn't recognises the entity type.
EntityType GameXmlContent::StringToEntityType(const char* s)
{
	static const eastl::hash_map<eastl::string, EntityType> entityTypeMap = {
		{"ENTITY_TYPE_CREATURE", EntityType::CREATURE},
		{"ENTITY_TYPE_DYNAMIC", EntityType::DYNAMIC},
		{"ENTITY_TYPE_ITEM", EntityType::ITEM},
		{"ENTITY_TYPE_SFX", EntityType::SFX},
		{"ENTITY_TYPE_TERRAIN", EntityType::TERRAIN}
	};

	auto it = entityTypeMap.find(s);
	if (it != entityTypeMap.end()) {
		return it->second;
	}

	LOG("Unknown EntityType: %s", s);
	return EntityType::INVALID;
}

int GameXmlContent::GetSkillSlotIndex(const char* skillSlot)
{
	static const eastl::hash_map<eastl::string, int> skillSlotMap = {
		{"SKILL_SLOT_1", 0},
		{"SKILL_SLOT_2", 1},
		{"SKILL_SLOT_3", 2},
		{"SKILL_SLOT_4", 3},
		{"SKILL_SLOT_UG", -2},         // Ultimate
		{"SKILL_SLOT_PASSIVE", -3},
		{"SKILL_SLOT_SHIRK", -4},
		{"SKILL_SLOT_BREAK_FALL", -5},
		{"SKILL_SLOT_COMBOSET", -6}
	};

	auto it = skillSlotMap.find(skillSlot);
	return (it != skillSlotMap.end()) ? it->second : -1;
}

SkillType GameXmlContent::StringToSkillType(const char* s)
{
	static const eastl::hash_map<eastl::string, SkillType> skillTypeMap = {
		{"SKILL_TYPE_COMBO", SkillType::COMBO},
		{"SKILL_TYPE_NORMAL", SkillType::NORMAL},
		{"SKILL_TYPE_PASSIVE", SkillType::PASSIVE},
		{"SKILL_TYPE_SHIRK", SkillType::SHIRK},
		{"SKILL_TYPE_STANCE", SkillType::STANCE},
		{"SKILL_TYPE_SUMMON", SkillType::SUMMON},
		{"SKILL_TYPE_TOGGLE", SkillType::TOGGLE},
		{"SKILL_TYPE_BREAKFALL", SkillType::BREAKFALL}
	};

	auto it = skillTypeMap.find(s);
	if (it != skillTypeMap.end()) {
		return it->second;
	}

	LOG("Unknown SkillType: %s", s);
	return SkillType::INVALID;
}

const GameXmlContent::MapList* GameXmlContent::FindMapListByID(i32 index) const
{
	foreach(it, maplists) {
		if (it->index == index) {
			return &(*it);
		}
	}
	return nullptr;
}

const GameXmlContent::Song* GameXmlContent::FindJukeboxSongByID(SongID songID) const
{
	foreach(it, jukeboxSongs) {
		if(it->ID == songID) {
			return &(*it);
		}
	}
	return nullptr;
}

const GameXmlContent::Master& GameXmlContent::GetMaster(ClassType classType) const
{
	auto found = masterClassTypeMap.find(classType);
	ASSERT(found != masterClassTypeMap.end());
	return *found->second;
}

const GameXmlContent::Action& GameXmlContent::GetSkillAction(ClassType classType, ActionStateID actionID) const
{
	auto found = actionListMap.find(classType);
	ASSERT(found != actionListMap.end());

	foreach_const(it, found->second) {
		if(it->ID == actionID) {
			return *it;
		}
	}

	ASSERT(0); // not found
	return actionListMap.cbegin()->second.front(); // unreachable
}

const Remote& GameXmlContent::GetRemote(RemoteIdx remoteID) const
{
	auto found = remoteMap.find(remoteID);
	ASSERT(found != remoteMap.end());
	return found->second;
}

bool GameXmlContentLoad()
{
	GameXmlContent* content = new GameXmlContent();
	g_GameXmlContent = content;
	return content->Load();
}

const GameXmlContent& GetGameXmlContent()
{
	return *g_GameXmlContent;
}

bool OpenMeshFile(const char* path, MeshFile* out)
{
	struct MeshFileHeader
	{
		u32 magic;
		u16 version;
		u16 count;
	};

	i32 fileSize;
	u8* fileBuff = fileOpenAndReadAll(path, &fileSize);
	if(!fileBuff) {
		LOG("ERROR: failed to open '%s'", path);
		return false;
	}

	out->fileData = fileBuff;

	ConstBuffer buff(fileBuff, fileSize);
	const MeshFileHeader& header = buff.Read<MeshFileHeader>();

	if(strncmp((char*)&header.magic, "MESH", 4) != 0) {
		WARN("Not a MESH file '%s'", path);
		return false;
	}

	if(header.version != 2) {
		WARN("Version not supported (%d) '%s'", header.version, path);
		return false;
	}

	for(int i = 0; i < header.count; i++) {
		const i32 nameLen = buff.Read<i32>();
		const char* name = (char*)buff.ReadRaw(nameLen);
		const u32 vertexCount = buff.Read<u32>();
		const u32 indexCount = buff.Read<u32>();
		const MeshFile::Vertex* vertices = (MeshFile::Vertex*)buff.ReadRaw(sizeof(MeshFile::Vertex) * vertexCount);
		const u16* indices = (u16*)buff.ReadRaw(sizeof(u16) * indexCount);

		MeshFile::Mesh& mesh = out->meshList.push_back();
		ASSERT(nameLen < 64);
		mesh.name.assign(name, nameLen);
		mesh.vertexCount = vertexCount;
		mesh.indexCount = indexCount;
		mesh.vertices = vertices;
		mesh.indices = indices;
	}
	return true;
}

namespace ActionCommand {

Type TypeFromString(const char* str)
{
	static const eastl::hash_map<eastl::string, Type> commandTypeMap = {
		{"STATE_BLOCK", Type::STATE_BLOCK},
		{"WEAPON_USE", Type::WEAPON_USE},
		{"MOVE", Type::MOVE},
		{"TELEPORT", Type::TELEPORT},
		{"ENTITY", Type::ENTITY},
		{"STATUS", Type::STATUS},
		{"REMOTE", Type::REMOTE},
		{"STATUS_SKILL_TARGET", Type::STATUS_SKILL_TARGET},
		{"REMOTE_SKILL_TARGET", Type::REMOTE_SKILL_TARGET},
		{"GRAPH_MOVE_HORZ", Type::GRAPH_MOVE_HORZ},
		{"FLY_MOVE", Type::FLY_MOVE},
		{"MOVESPEED", Type::MOVESPEED},
		{"ROTATESPEED", Type::ROTATESPEED},
		{"REGCOMBO", Type::REGCOMBO},
		{"SETSTANCE", Type::SETSTANCE},
		{"PHYSICS", Type::PHYSICS},
		{"AIOBJECT", Type::AIOBJECT},
		{"PUSH_OVERLAP", Type::PUSH_OVERLAP},
		{"POLYMORPH", Type::POLYMORPH},
		{"DIE_MODE", Type::DIE_MODE},
		{"SET_FLAG", Type::SET_FLAG},
		{"CLEAR_FLAG", Type::CLEAR_FLAG},
		{"FORCE_ROTATE", Type::FORCE_ROTATE},
		{"CALL_CHILD_SKILL", Type::CALL_CHILD_SKILL},
		{"LONG_JUMP", Type::LONG_JUMP},
		{"TALK", Type::TALK},
		{"EFFECT", Type::EFFECT},
		{"AI_ADJUST", Type::AI_ADJUST},
		{"INTERACTION", Type::INTERACTION},
		{"DEATHWORM_BOUND", Type::DEATHWORM_BOUND},
		{"UI", Type::UI},
		{"CHANGE_MESH", Type::CHANGE_MESH}
	};

	auto it = commandTypeMap.find(str);
	return (it != commandTypeMap.end()) ? it->second : Type::INVALID;
}

MovePreset MovePresetFromString(const char* str)
{
	static const eastl::hash_map<eastl::string, MovePreset> movePresetMap = {
		{"MOVE_PRESET_SEE_TARGET", MovePreset::SEE_TARGET},
		{"MOVE_PRESET_ROTATE", MovePreset::ROTATE},
		{"MOVE_PRESET_LIFTED", MovePreset::LIFTED},
		{"MOVE_PRESET_GRAPH", MovePreset::GRAPH},
		{"MOVE_PRESET_GRAPH_GROUND", MovePreset::GRAPH_GROUND},
		{"MOVE_PRESET_MOVE_NONE", MovePreset::MOVE_NONE},
		{"MOVE_PRESET_GOTO_CURRENT_TARGET", MovePreset::GOTO_CURRENT_TARGET},
		{"MOVE_PRESET_GOTO_CURRENT_TARGET_CHASE", MovePreset::GOTO_CURRENT_TARGET_CHASE},
		{"MOVE_PRESET_SEE_CURRENT_TARGET", MovePreset::SEE_CURRENT_TARGET},
		{"MOVE_PRESET_AFTERIMAGE_MOVE", MovePreset::AFTERIMAGE_MOVE},
		{"MOVE_PRESET_TARGET_POS", MovePreset::TARGET_POS},
		{"MOVE_PRESET_TARGET_POS_LEAP", MovePreset::TARGET_POS_LEAP},
		{"MOVE_PRESET_WARP", MovePreset::WARP},
		{"MOVE_PRESET_WARP_TARGET_POS", MovePreset::WARP_TARGET_POS},
		{"MOVE_PRESET_GOTO_CURRENT_TARGET_THROUGH", MovePreset::GOTO_CURRENT_TARGET_THROUGH},
		{"MOVE_PRESET_GOTO_CURRENT_TARGET_THROUGH_CHASE", MovePreset::GOTO_CURRENT_TARGET_THROUGH_CHASE},
		{"MOVE_PRESET_SEE_MEMORIZED_TARGET", MovePreset::SEE_MEMORIZED_TARGET},
		{"MOVE_PRESET_CHASE_TARGET", MovePreset::CHASE_TARGET},
		{"MOVE_PRESET_CHASE_TARGET_ATTACK", MovePreset::CHASE_TARGET_ATTACK}
	};


	auto it = movePresetMap.find(str);
	if (it != movePresetMap.end()) {
		return it->second;
	}
	return MovePreset::INVALID;
}

const char* MovePresetToString(MovePreset p)
{
	if(p == MovePreset::SEE_TARGET) { return "SEE_TARGET"; }
	if(p == MovePreset::ROTATE) { return "ROTATE"; }
	if(p == MovePreset::LIFTED) { return "LIFTED"; }
	if(p == MovePreset::GRAPH) { return "GRAPH"; }
	if(p == MovePreset::GRAPH_GROUND) { return "GRAPH_GROUND"; }
	if(p == MovePreset::MOVE_NONE) { return "MOVE_NONE"; }
	if(p == MovePreset::GOTO_CURRENT_TARGET) { return "GOTO_CURRENT_TARGET"; }
	if(p == MovePreset::GOTO_CURRENT_TARGET_CHASE) { return "GOTO_CURRENT_TARGET_CHASE"; }
	if(p == MovePreset::SEE_CURRENT_TARGET) { return "SEE_CURRENT_TARGET"; }
	if(p == MovePreset::AFTERIMAGE_MOVE) { return "AFTERIMAGE_MOVE"; }
	if(p == MovePreset::TARGET_POS) { return "TARGET_POS"; }
	if(p == MovePreset::TARGET_POS_LEAP) { return "TARGET_POS_LEAP"; }
	if(p == MovePreset::WARP) { return "WARP"; }
	if(p == MovePreset::WARP_TARGET_POS) { return "WARP_TARGET_POS"; }
	if(p == MovePreset::GOTO_CURRENT_TARGET_THROUGH) { return "GOTO_CURRENT_TARGET_THROUGH"; }
	if(p == MovePreset::GOTO_CURRENT_TARGET_THROUGH_CHASE) { return "GOTO_CURRENT_TARGET_THROUGH_CHASE"; }
	if(p == MovePreset::SEE_MEMORIZED_TARGET) { return "SEE_MEMORIZED_TARGET"; }
	if(p == MovePreset::CHASE_TARGET) { return "CHASE_TARGET"; }
	if(p == MovePreset::CHASE_TARGET_ATTACK) { return "CHASE_TARGET_ATTACK"; }
	return "INVALID";
}

TargetPreset TargetPresetFromString(const char* str)
{
	static const eastl::hash_map<eastl::string, TargetPreset> targetPresetMap = {
		{"TARGET_SELF_FFF", TargetPreset::SELF_FFF},
		{"TARGET_SELF_FFF_1", TargetPreset::SELF_FFF_1},
		{"TARGET_SELF_TFF", TargetPreset::SELF_TFF},
		{"TARGET_SELF_TTF", TargetPreset::SELF_TTF},
		{"TARGET_SELF_TTT", TargetPreset::SELF_TTT},
		{"TARGET_POS_TTT_1", TargetPreset::POS_TTT_1},
		{"TARGET_POS_FFF", TargetPreset::POS_FFF},
		{"TARGET_POS_TFF", TargetPreset::POS_TFF},
		{"TARGET_POS_TFF_1_800_1000", TargetPreset::POS_TFF_1_800_1000},
		{"TARGET_LAST_CHILD_FFF", TargetPreset::LAST_CHILD_FFF},
		{"TARGET_LAST_CHILD_TFF", TargetPreset::LAST_CHILD_TFF},
		{"TARGET_CHILD_INDEX_FFF", TargetPreset::CHILD_INDEX_FFF},
		{"TARGET_MEMORIZED_TFT", TargetPreset::MEMORIZED_TFT},
		{"TARGET_CURRENT_FFF", TargetPreset::CURRENT_FFF},
		{"TARGET_LOCK_TFF", TargetPreset::LOCK_TFF},
		{"TARGET_MULTI_LOCKON_FFF", TargetPreset::MULTI_LOCKON_FFF}
	};

	auto it = targetPresetMap.find(str);
	return (it != targetPresetMap.end()) ? it->second : TargetPreset::INVALID;
}

const char* TargetPresetToString(TargetPreset p)
{
	if(p == TargetPreset::SELF_FFF) { return "TARGET_SELF_FFF"; }
	if(p == TargetPreset::SELF_FFF_1) { return "TARGET_SELF_FFF_1"; }
	if(p == TargetPreset::SELF_TFF) { return "TARGET_SELF_TFF"; }
	if(p == TargetPreset::SELF_TTF) { return "TARGET_SELF_TTF"; }
	if(p == TargetPreset::SELF_TTT) { return "TARGET_SELF_TTT"; }
	if(p == TargetPreset::POS_TTT_1) { return "TARGET_POS_TTT_1"; }
	if(p == TargetPreset::POS_FFF) { return "TARGET_POS_FFF"; }
	if(p == TargetPreset::POS_TFF) { return "TARGET_POS_TFF"; }
	if(p == TargetPreset::POS_TFF_1_800_1000) { return "TARGET_POS_TFF_1_800_1000"; }
	if(p == TargetPreset::LAST_CHILD_FFF) { return "TARGET_LAST_CHILD_FFF"; }
	if(p == TargetPreset::LAST_CHILD_TFF) { return "TARGET_LAST_CHILD_TFF"; }
	if(p == TargetPreset::CHILD_INDEX_FFF) { return "TARGET_CHILD_INDEX_FFF"; }
	if(p == TargetPreset::MEMORIZED_TFT) { return "TARGET_MEMORIZED_TFT"; }
	if(p == TargetPreset::CURRENT_FFF) { return "TARGET_CURRENT_FFF"; }
	if(p == TargetPreset::LOCK_TFF) { return "TARGET_LOCK_TFF"; }
	if(p == TargetPreset::MULTI_LOCKON_FFF) { return "TARGET_MULTI_LOCKON_FFF"; }
	return "INVALID";
}

}

Remote::BoundType Remote::BoundTypeFromString(const char* str)
{
	static const eastl::hash_map<eastl::string, BoundType> boundTypeMap = {
        {"E_BOUND_NONE", BoundType::E_BOUND_NONE},
        {"E_BOUND_RAY", BoundType::E_BOUND_RAY},
        {"E_BOUND_BOX", BoundType::E_BOUND_BOX},
        {"E_BOUND_CAPSULE", BoundType::E_BOUND_CAPSULE},
        {"E_BOUND_SPHERE", BoundType::E_BOUND_SPHERE},
        {"E_BOUND_BEAM", BoundType::E_BOUND_BEAM},
        {"E_BOUND_BEAM_NIF", BoundType::E_BOUND_BEAM_NIF},
        {"E_BOUND_LASER", BoundType::E_BOUND_LASER},
        {"E_BOUND_PHYSXPROP", BoundType::E_BOUND_PHYSXPROP},
        {"E_BOUND_HAMMER", BoundType::E_BOUND_HAMMER},
        {"E_BOUND_CASTER_MOVE_BOUND", BoundType::E_BOUND_CASTER_MOVE_BOUND}
    };

    auto it = boundTypeMap.find(str);
    return (it != boundTypeMap.end()) ? it->second : BoundType::INVALID;

}	

const char* Remote::BoundTypeToString(BoundType t)
{
	if(t == BoundType::E_BOUND_NONE) { return "E_BOUND_NONE"; }
	if(t == BoundType::E_BOUND_RAY) { return "E_BOUND_RAY"; }
	if(t == BoundType::E_BOUND_BOX) { return "E_BOUND_BOX"; }
	if(t == BoundType::E_BOUND_CAPSULE) { return "E_BOUND_CAPSULE"; }
	if(t == BoundType::E_BOUND_SPHERE) { return "E_BOUND_SPHERE"; }
	if(t == BoundType::E_BOUND_BEAM) { return "E_BOUND_BEAM"; }
	if(t == BoundType::E_BOUND_BEAM_NIF) { return "E_BOUND_BEAM_NIF"; }
	if(t == BoundType::E_BOUND_LASER) { return "E_BOUND_LASER"; }
	if(t == BoundType::E_BOUND_PHYSXPROP) { return "E_BOUND_PHYSXPROP"; }
	if(t == BoundType::E_BOUND_HAMMER) { return "E_BOUND_HAMMER"; }
	if(t == BoundType::E_BOUND_CASTER_MOVE_BOUND) { return "E_BOUND_CASTER_MOVE_BOUND"; }
	return "INVALID";
}

Remote::DamageGroup Remote::DamageGroupFromString(const char* str)
{
	static const eastl::hash_map<eastl::string, DamageGroup> damageGroupMap = {
	   {"eNONE", DamageGroup::eNONE},
	   {"eENEMY", DamageGroup::eENEMY},
	   {"eFRIEND", DamageGroup::eFRIEND},
	   {"eALL", DamageGroup::eALL}
	};

	auto it = damageGroupMap.find(str);
	return (it != damageGroupMap.end()) ? it->second : DamageGroup::INVALID;

}

const char* Remote::DamageGroupToString(DamageGroup g)
{
	if(g == DamageGroup::eNONE) { return "eNONE"; }
	if(g == DamageGroup::eENEMY) { return "eENEMY"; }
	if(g == DamageGroup::eFRIEND) { return "eFRIEND"; }
	if(g == DamageGroup::eALL) { return "eALL"; }
	return "INVALID";
}

Remote::BehaviorType Remote::BehaviourTypeFromString(const char* str)
{
	static const eastl::hash_map<eastl::string, BehaviorType> behaviorTypeMap = {
		{"E_REMOTE_BEHAVIOR_TYPE_NONETARGET", BehaviorType::NONETARGET},
		{"E_REMOTE_BEHAVIOR_TYPE_HOMING", BehaviorType::HOMING},
		{"E_REMOTE_BEHAVIOR_TYPE_RETURN", BehaviorType::RETURN},
		{"E_REMOTE_BEHAVIOR_TYPE_CASTER_TARGET", BehaviorType::CASTER_TARGET},
		{"E_REMOTE_BEHAVIOR_TYPE_BEAM", BehaviorType::BEAM},
		{"E_REMOTE_BEHAVIOR_TYPE_CHAIN", BehaviorType::CHAIN},
		{"E_REMOTE_BEHAVIOR_TYPE_GRAPH", BehaviorType::GRAPH},
		{"E_REMOTE_BEHAVIOR_TYPE_TARGET", BehaviorType::TARGET},
		{"E_REMOTE_BEHAVIOR_TYPE_FOLLOW_TARGET", BehaviorType::FOLLOW_TARGET},
		{"E_REMOTE_BEHAVIOR_TYPE_ATTACH_TARGET", BehaviorType::ATTACH_TARGET},
		{"E_REMOTE_BEHAVIOR_TYPE_ATTACH_MUZZLE", BehaviorType::ATTACH_MUZZLE},
		{"E_REMOTE_BEHAVIOR_TYPE_TARGET_GROUNDPOSITION", BehaviorType::TARGET_GROUNDPOSITION},
		{"E_REMOTE_BEHAVIOR_TYPE_ORBIT", BehaviorType::ORBIT}
	};

	auto it = behaviorTypeMap.find(str);
	return (it != behaviorTypeMap.end()) ? it->second : BehaviorType::INVALID;

}

const char* Remote::BehaviourTypeToString(BehaviorType t)
{
	if(t == BehaviorType::NONETARGET) { return "E_REMOTE_BEHAVIOR_TYPE_NONETARGET"; }
	if(t == BehaviorType::HOMING) { return "E_REMOTE_BEHAVIOR_TYPE_HOMING"; }
	if(t == BehaviorType::RETURN) { return "E_REMOTE_BEHAVIOR_TYPE_RETURN"; }
	if(t == BehaviorType::CASTER_TARGET) { return "E_REMOTE_BEHAVIOR_TYPE_CASTER_TARGET"; }
	if(t == BehaviorType::BEAM) { return "E_REMOTE_BEHAVIOR_TYPE_BEAM"; }
	if(t == BehaviorType::CHAIN) { return "E_REMOTE_BEHAVIOR_TYPE_CHAIN"; }
	if(t == BehaviorType::GRAPH) { return "E_REMOTE_BEHAVIOR_TYPE_GRAPH"; }
	if(t == BehaviorType::TARGET) { return "E_REMOTE_BEHAVIOR_TYPE_TARGET"; }
	if(t == BehaviorType::FOLLOW_TARGET) { return "E_REMOTE_BEHAVIOR_TYPE_FOLLOW_TARGET"; }
	if(t == BehaviorType::ATTACH_TARGET) { return "E_REMOTE_BEHAVIOR_TYPE_ATTACH_TARGET"; }
	if(t == BehaviorType::ATTACH_MUZZLE) { return "E_REMOTE_BEHAVIOR_TYPE_ATTACH_MUZZLE"; }
	if(t == BehaviorType::TARGET_GROUNDPOSITION) { return "E_REMOTE_BEHAVIOR_TYPE_TARGET_GROUNDPOSITION"; }
	if(t == BehaviorType::ORBIT) { return "E_REMOTE_BEHAVIOR_TYPE_ORBIT"; }
	return "INVALID";
}
