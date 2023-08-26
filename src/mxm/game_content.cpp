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
			if (EA::StdC::Strcmp("SKILL_SLOT_1", skillSlot) == 0)
			{
				LoadMasterSkillWithID(*_skillsModel.getSkillByIndex(0), skillID);
			}
			else if (EA::StdC::Strcmp("SKILL_SLOT_2", skillSlot) == 0)
			{
				LoadMasterSkillWithID(*_skillsModel.getSkillByIndex(1), skillID);
			}
			else if (EA::StdC::Strcmp("SKILL_SLOT_3", skillSlot) == 0)
			{
				LoadMasterSkillWithID(*_skillsModel.getSkillByIndex(2), skillID);
			}
			else if (EA::StdC::Strcmp("SKILL_SLOT_4", skillSlot) == 0)
			{
				LoadMasterSkillWithID(*_skillsModel.getSkillByIndex(3), skillID);
			}
			else if (EA::StdC::Strcmp("SKILL_SLOT_UG", skillSlot) == 0)
			{
				LoadMasterSkillWithID(*_skillsModel.getUltimate(), skillID);
			}
			else if (EA::StdC::Strcmp("SKILL_SLOT_PASSIVE", skillSlot) == 0)
			{

			}
			else if (EA::StdC::Strcmp("SKILL_SLOT_SHIRK", skillSlot) == 0)
			{

			}
			else if (EA::StdC::Strcmp("SKILL_SLOT_BREAK_FALL", skillSlot) == 0)
			{

			}
			else if (EA::StdC::Strcmp("SKILL_SLOT_COMBOSET", skillSlot) == 0)
			{

			}
			else
			{
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

	do {
		SkillNormalModel skill;

		i32 skillID;
		pNodeSkill->QueryAttribute("ID", &skillID);
		skill.setID(skillID);

		XMLElement* pNodeCommonSkill = pNodeSkill->FirstChildElement("ST_COMMONSKILL");

		// parse type
		const char* typeStr;
		pNodeCommonSkill->QueryStringAttribute("_Type", &typeStr);

		SkillType type = SkillType::INVALID;
		if(EA::StdC::Strcmp(typeStr, "SKILL_TYPE_PASSIVE")) {
			type = SkillType::PASSIVE;
		}
		else if(EA::StdC::Strcmp(typeStr, "SKILL_TYPE_NORMAL")) {
			type = SkillType::NORMAL;
		}
		else if(EA::StdC::Strcmp(typeStr, "SKILL_TYPE_TOGGLE")) {
			type = SkillType::TOGGLE;
		}
		else if(EA::StdC::Strcmp(typeStr, "SKILL_TYPE_SUMMON")) {
			type = SkillType::SUMMON;
		}
		else if(EA::StdC::Strcmp(typeStr, "SKILL_TYPE_STANCE")) {
			type = SkillType::STANCE;
		}
		else if(EA::StdC::Strcmp(typeStr, "SKILL_TYPE_SHIRK")) {
			type = SkillType::SHIRK;
		}
		else if(EA::StdC::Strcmp(typeStr, "SKILL_TYPE_COMBO")) {
			type = SkillType::COMBO;
		}
		else if(EA::StdC::Strcmp(typeStr, "SKILL_TYPE_BREAKFALL")) {
			type = SkillType::BREAKFALL;
		}
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
	const char *weaponSpecREF;
	float _temp = 0.0f;

	pNodeWeaponSpecRef.QueryStringAttribute("ref", &weaponSpecREF);
	pNodeWeaponSpecRef.QueryFloatAttribute("value", &_temp);

	if (EA::StdC::Strcmp("WEAPONSPEC_REF_FIRINGMETHOD_CHARGING_TIME_LEVEL1", weaponSpecREF) == 0)
	{
		_weaponSpec.setFiringMethodChargingTimeLevel1(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_FIRINGMETHOD_CHARGING_TIME_LEVEL2", weaponSpecREF) == 0)
	{
		_weaponSpec.setFiringMethodChargingTimeLevel2(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_FIRINGMETHOD_CHARGING_TIME_LEVEL3", weaponSpecREF) == 0)
	{
		_weaponSpec.setFiringMethodChargingTimeLevel3(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_FIRINGMETHOD_COMBO_FIREDELAY", weaponSpecREF) == 0)
	{
		_weaponSpec.setFiringMethodComboFireDelay(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_FIRINGMETHOD_COMBO_VALIDTIME", weaponSpecREF) == 0)
	{
		_weaponSpec.setFiringMethodComboValidTime(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_FIRINGMETHOD_CONSUMPTION", weaponSpecREF) == 0)
	{
		_weaponSpec.setFiringMethodConsumption(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_FIRINGMETHOD_CONSUMPTION_BY_CHARGING", weaponSpecREF) == 0)
	{
		_weaponSpec.setFiringMethodConsumptionByCharging(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_FIRINGMETHOD_ERRORANGLE", weaponSpecREF) == 0)
	{
		_weaponSpec.setFiringMethodErrorAngle(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_FIRINGMETHOD_FIREDELAY", weaponSpecREF) == 0)
	{
		_weaponSpec.setFiringMethodComboFireDelay(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_FIRINGMETHOD_MAXDISTANCE", weaponSpecREF) == 0)
	{
		_weaponSpec.setFiringMethodMaxDistance(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_FIRINGMETHOD_MINDISTANCE", weaponSpecREF) == 0)
	{
		_weaponSpec.setFiringMethodMinDistance(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_FIRINGMETHOD_CHARGING_LEVEL_1_CREATE_RADIATE_MAXNUM", weaponSpecREF) == 0)
	{
		_weaponSpec.setFiringMethodChargingLevel1CreateRadiateMaxNum(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_FIRINGMETHOD_CHARGING_LEVEL_2_CREATE_RADIATE_MAXNUM", weaponSpecREF) == 0)
	{
		_weaponSpec.setFiringMethodChargingLevel2CreateRadiateMaxNum(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_FIRINGMETHOD_CREATE_RADIATE_MAXANGBLE", weaponSpecREF) == 0)
	{
		_weaponSpec.setFiringMethodCreateRadiateMaxAngBle(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_FIRINGMETHOD_CREATE_RADIATE_MAXNUM", weaponSpecREF) == 0)
	{
		_weaponSpec.setFiringMethodCreateRadiateMaxNum(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_FIRINGMETHOD_FIRINGSTAT_MOVEMENT_SPEED", weaponSpecREF) == 0)
	{
		_weaponSpec.setFiringMethodFiringStatMovementSpeed(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_FIRINGMETHOD_GAUGE_LEVEL", weaponSpecREF) == 0)
	{
		_weaponSpec.setFiringMethodGaugeLevel(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_FIRINGMETHOD_CHARGING_LEVEL_1_CREATE_PARALLEL_MAXNUM", weaponSpecREF) == 0)
	{
		_weaponSpec.setFiringMethodChargingLevel1CreateParallelMaxNum(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_FIRINGMETHOD_CHARGING_LEVEL_2_CREATE_PARALLEL_MAXNUM", weaponSpecREF) == 0)
	{
		_weaponSpec.setFiringMethodChargingLevel2CreateParallelMaxNum(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_FIRINGMETHOD_CHARGING_LEVEL_1_ANIMATION_MOVEHORIZON", weaponSpecREF) == 0)
	{
		_weaponSpec.setFiringMethodChargingLevel1AnimationMoveHorizon(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_FIRINGMETHOD_CHARGING_LEVEL_2_ANIMATION_MOVEHORIZON", weaponSpecREF) == 0)
	{
		_weaponSpec.setFiringMethodChargingLevel2AnimationMoveHorizon(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_FIRINGMETHOD_GAUGE_AUTOREGEN", weaponSpecREF) == 0)
	{
		_weaponSpec.setFiringMethodGaugeAutoRegen(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_REMOTE_ANGLE", weaponSpecREF) == 0)
	{
		_weaponSpec.setRemoteAngle(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_REMOTE_ATTACKMULTIPLIER", weaponSpecREF) == 0)
	{
		_weaponSpec.setRemoteAttackMultiplier(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_REMOTE_DAMAGEONBOUND", weaponSpecREF) == 0)
	{
		_weaponSpec.setRemoteDamageOnBound(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_REMOTE_FOV", weaponSpecREF) == 0)
	{
		_weaponSpec.setRemoteFOV(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_REMOTE_LENGTH_X", weaponSpecREF) == 0)
	{
		_weaponSpec.setRemoteLengthX(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_REMOTE_LENGTH_Y", weaponSpecREF) == 0)
	{
		_weaponSpec.setRemoteLengthY(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_REMOTE_MAXDISTANCE", weaponSpecREF) == 0)
	{
		_weaponSpec.setRemoteMaxDistance(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_REMOTE_MAXSCALE", weaponSpecREF) == 0)
	{
		_weaponSpec.setRemoteMaxScale(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_REMOTE_MINSCALE", weaponSpecREF) == 0)
	{
		_weaponSpec.setRemoteMinScale(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_REMOTE_MAXSPEED", weaponSpecREF) == 0)
	{
		_weaponSpec.setRemoteMaxSpeed(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_REMOTE_PENETRATIONCOUNT", weaponSpecREF) == 0)
	{
		_weaponSpec.setRemotePenetrationCount(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_REMOTE_SIGHT", weaponSpecREF) == 0)
	{
		_weaponSpec.setRemoteSight(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_REMOTE_STATUS_RATE", weaponSpecREF) == 0)
	{
		_weaponSpec.setRemoteStatusRate(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_REMOTE_CRITICALRATE", weaponSpecREF) == 0)
	{
		_weaponSpec.setRemoteCriticalRate(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_REMOTE_SUBBOUNDLENGTH", weaponSpecREF) == 0)
	{
		_weaponSpec.setRemoteSubBoundLenght(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_ACTIONBASE_PARAM_1", weaponSpecREF) == 0)
	{
		_weaponSpec.setActionBaseParam1(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_STATUS_DISTANCE", weaponSpecREF) == 0)
	{
		_weaponSpec.setStatusDistance(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_STATUS_REGEN_AND_HEALTH", weaponSpecREF) == 0)
	{
		_weaponSpec.setStatusRegenAndHealth(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_STATUS_STAT_MOVEMENT_SPEED", weaponSpecREF) == 0)
	{
		_weaponSpec.setStatusStatMovementSpeed(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_STATUS_STAT_DEFENCE", weaponSpecREF) == 0)
	{
		_weaponSpec.setStatusStatDefence(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_STATUS_DURATION_TIME", weaponSpecREF) == 0)
	{
		_weaponSpec.setStatusDurationTime(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_STATUS_MAX_OVERLAP_COUNT", weaponSpecREF) == 0)
	{
		_weaponSpec.setStatusMaxOverlapCount(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_STATUS_DOT_DAMAGE_MULTIPLIER", weaponSpecREF) == 0)
	{
		_weaponSpec.setStatusDotDamageMultiplier(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_ATTACK", weaponSpecREF) == 0)
	{
		_weaponSpec.setAttack(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_STAT_AND_CRITICALDAMAGE", weaponSpecREF) == 0)
	{
		_weaponSpec.setStatAndCriticalDamage(_temp);
	}
	else if (EA::StdC::Strcmp("WEAPONSPEC_REF_STAT_AND_CRITICALRATE", weaponSpecREF) == 0)
	{
		_weaponSpec.setStatAndCriticalRate(_temp);
	}
	else
	{
		LOG("ERROR(SetWeaponREFSet): Unsupported WEAPONSPEC_REF %s", weaponSpecREF);
	}
}

bool GameXmlContent::LoadMapList()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/MAPLIST.xml", doc)) return false;

	XMLElement* pMapElt = doc.FirstChildElement()->FirstChildElement()->FirstChildElement()->FirstChildElement();
	do {
		MapList mapList;
		pMapElt->QueryAttribute("_Index", &mapList.index);

		const char* levelFileTemp;
		pMapElt->QueryStringAttribute("_LevelFile", &levelFileTemp);
		mapList.levelFile = levelFileTemp;

		const char* mapTypeXml;
		pMapElt->QueryStringAttribute("_MapType", &mapTypeXml);
		
		mapList.gameSubModeType = GameSubModeType::GAME_SUB_MODE_INVALID;
		if (EA::StdC::Strcmp("E_MAP_TYPE_CITY", mapTypeXml) == 0)
		{
			mapList.mapType = MapType::MAP_CITY;
		}
		else if (EA::StdC::Strcmp("E_MAP_TYPE_INGAME", mapTypeXml) == 0)
		{
			const char* gameSubModeTypeXml;

			mapList.mapType = MapType::MAP_INGAME;
		
			if (pMapElt->QueryStringAttribute("_GameSubModeType", &gameSubModeTypeXml) != XML_SUCCESS)
			{
				// training room doesn't have a gamesubmode
			}
			else if (EA::StdC::Strcmp("GAME_SUB_MODE_DEATH_MATCH_NORMAL", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GameSubModeType::GAME_SUB_MODE_DEATH_MATCH_NORMAL;
			}
			else if (EA::StdC::Strcmp("GAME_SUB_MODE_OCCUPY_CORE", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GameSubModeType::GAME_SUB_MODE_OCCUPY_CORE;
			}
			else if (EA::StdC::Strcmp("GAME_SUB_MODE_OCCUPY_BUSH", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GameSubModeType::GAME_SUB_MODE_OCCUPY_BUSH;
			}
			else if (EA::StdC::Strcmp("GAME_SUB_MODE_GOT_AUTHENTIC", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GameSubModeType::GAME_SUB_MODE_GOT_AUTHENTIC;
			}
			else if (EA::StdC::Strcmp("GAME_SUB_MODE_GOT_TUTORIAL_BASIC", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GameSubModeType::GAME_SUB_MODE_GOT_TUTORIAL_BASIC;
			}
			else if (EA::StdC::Strcmp("GAME_SUB_MODE_GOT_TUTORIAL_EXPERT", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GameSubModeType::GAME_SUB_MODE_GOT_TUTORIAL_EXPERT;
			}
			else if (EA::StdC::Strcmp("GAME_SUB_MODE_GOT_FIRE_POWER", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GameSubModeType::GAME_SUB_MODE_GOT_FIRE_POWER;
			}
			else if (EA::StdC::Strcmp("GAME_SUB_MODE_GOT_ULTIMATE_TITAN", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GameSubModeType::GAME_SUB_MODE_GOT_ULTIMATE_TITAN;
			}
			else if (EA::StdC::Strcmp("GAME_SUB_MODE_SPORTS_RUN", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GameSubModeType::GAME_SUB_MODE_SPORTS_RUN;
			}
			else if (EA::StdC::Strcmp("GAME_SUB_MODE_SPORTS_SURVIVAL", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GameSubModeType::GAME_SUB_MODE_SPORTS_SURVIVAL;
			}
			else if (EA::StdC::Strcmp("GAME_SUB_MODE_STAGE_TUTORIAL", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GameSubModeType::GAME_SUB_MODE_STAGE_TUTORIAL;
			}
			else if (EA::StdC::Strcmp("GAME_SUB_MODE_STAGE_NORMAL", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GameSubModeType::GAME_SUB_MODE_STAGE_NORMAL;
			}
			else
			{
				LOG("ERROR(LOADMAPLIST): Unsupported subGameMode %s", gameSubModeTypeXml);
			}
		}
		else
		{
			LOG("ERROR(LOADMAPLIST): Unsupported map type");
			return false;
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
			if(EA::StdC::Strncmp(teamString, "TEAM_RED", 8) == 0) spawn.faction = Faction::RED;
			else if(EA::StdC::Strncmp(teamString, "TEAM_BLUE", 9) == 0) spawn.faction = Faction::BLUE;
			else if(EA::StdC::Strncmp(teamString, "TEAM_DYNAMIC", 12) == 0) spawn.faction = Faction::DYNAMIC;
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
			if(EA::StdC::Strncmp(teamString, "TEAM_RED", 8) == 0) spawn.faction = Faction::RED;
			else if(EA::StdC::Strncmp(teamString, "TEAM_BLUE", 9) == 0) spawn.faction = Faction::BLUE;
			else if(EA::StdC::Strncmp(teamString, "TEAM_DYNAMIC", 12) == 0) spawn.faction = Faction::DYNAMIC;
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

// Returns CREATURE_TYPE_INVALID when it doesn't recognises the creature type.
CreatureType GameXmlContent::StringToCreatureType(const char* s)
{
	if (EA::StdC::Strcmp("CREATURE_TYPE_ALLY", s) == 0)
	{
		return CreatureType::CREATURE_TYPE_ALLY;
	}
	else if (EA::StdC::Strcmp("CREATURE_TYPE_BOT", s) == 0)
	{
		return CreatureType::CREATURE_TYPE_BOT;
	}
	else if (EA::StdC::Strcmp("CREATURE_TYPE_MONSTER", s) == 0)
	{
		return CreatureType::CREATURE_TYPE_MONSTER;
	}
	else if (EA::StdC::Strcmp("CREATURE_TYPE_NPC", s) == 0)
	{
		return CreatureType::CREATURE_TYPE_NPC;
	}
	else if (EA::StdC::Strcmp("CREATURE_TYPE_PC", s) == 0)
	{
		return CreatureType::CREATURE_TYPE_PC;
	}
	else
	{
		LOG("Unknown CreatureType: %s", s);
		return CreatureType::CREATURE_TYPE_INVALID;
	}
}

// Returns ENTITY_INVALID when it doesn't recognises the entity type.
EntityType GameXmlContent::StringToEntityType(const char* s)
{
	if (EA::StdC::Strcmp("ENTITY_TYPE_CREATURE", s) == 0)
	{
		return EntityType::CREATURE;
	}
	else if (EA::StdC::Strcmp("ENTITY_TYPE_DYNAMIC", s) == 0)
	{
		return EntityType::DYNAMIC;
	}
	else if (EA::StdC::Strcmp("ENTITY_TYPE_ITEM", s) == 0)
	{
		return EntityType::ITEM;
	}
	else if (EA::StdC::Strcmp("ENTITY_TYPE_SFX", s) == 0)
	{
		return EntityType::SFX;
	}
	else if (EA::StdC::Strcmp("ENTITY_TYPE_TERRAIN", s) == 0)
	{
		return EntityType::TERRAIN;
	}
	else
	{
		LOG("Unknown EntityType: %s", s);
		return EntityType::INVALID;
	}
}

SkillType GameXmlContent::StringToSkillType(const char* s)
{
	if (EA::StdC::Strcmp("SKILL_TYPE_COMBO", s) == 0)
	{
		return SkillType::COMBO;
	}
	else if (EA::StdC::Strcmp("SKILL_TYPE_NORMAL", s) == 0)
	{
		return SkillType::NORMAL;
	}
	else if (EA::StdC::Strcmp("SKILL_TYPE_PASSIVE", s) == 0)
	{
		return SkillType::PASSIVE;
	}
	else if (EA::StdC::Strcmp("SKILL_TYPE_SHIRK", s) == 0)
	{
		return SkillType::SHIRK;
	}
	else if (EA::StdC::Strcmp("SKILL_TYPE_STANCE", s) == 0)
	{
		return SkillType::STANCE;
	}
	else if (EA::StdC::Strcmp("SKILL_TYPE_SUMMON", s) == 0)
	{
		return SkillType::SUMMON;
	}
	else if (EA::StdC::Strcmp("SKILL_TYPE_TOGGLE", s) == 0)
	{
		return SkillType::TOGGLE;
	}
	else
	{
		LOG("Unknown SkillType: %s", s);
		return SkillType::INVALID;
	}
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
	if(StringEquals(str, "STATE_BLOCK")) {
		return Type::STATE_BLOCK;
	}
	if(StringEquals(str, "WEAPON_USE")) {
		return Type::WEAPON_USE;
	}
	if(StringEquals(str, "MOVE")) {
		return Type::MOVE;
	}
	if(StringEquals(str, "TELEPORT")) {
		return Type::TELEPORT;
	}
	if(StringEquals(str, "ENTITY")) {
		return Type::ENTITY;
	}
	if(StringEquals(str, "STATUS")) {
		return Type::STATUS;
	}
	if(StringEquals(str, "REMOTE")) {
		return Type::REMOTE;
	}
	if(StringEquals(str, "STATUS_SKILL_TARGET")) {
		return Type::STATUS_SKILL_TARGET;
	}
	if(StringEquals(str, "REMOTE_SKILL_TARGET")) {
		return Type::REMOTE_SKILL_TARGET;
	}
	if(StringEquals(str, "GRAPH_MOVE_HORZ")) {
		return Type::GRAPH_MOVE_HORZ;
	}
	if(StringEquals(str, "FLY_MOVE")) {
		return Type::FLY_MOVE;
	}
	if(StringEquals(str, "MOVESPEED")) {
		return Type::MOVESPEED;
	}
	if(StringEquals(str, "ROTATESPEED")) {
		return Type::ROTATESPEED;
	}
	if(StringEquals(str, "REGCOMBO")) {
		return Type::REGCOMBO;
	}
	if(StringEquals(str, "SETSTANCE")) {
		return Type::SETSTANCE;
	}
	if(StringEquals(str, "PHYSICS")) {
		return Type::PHYSICS;
	}
	if(StringEquals(str, "AIOBJECT")) {
		return Type::AIOBJECT;
	}
	if(StringEquals(str, "PUSH_OVERLAP")) {
		return Type::PUSH_OVERLAP;
	}
	if(StringEquals(str, "POLYMORPH")) {
		return Type::POLYMORPH;
	}
	if(StringEquals(str, "DIE_MODE")) {
		return Type::DIE_MODE;
	}
	if(StringEquals(str, "SET_FLAG")) {
		return Type::SET_FLAG;
	}
	if(StringEquals(str, "CLEAR_FLAG")) {
		return Type::CLEAR_FLAG;
	}
	if(StringEquals(str, "FORCE_ROTATE")) {
		return Type::FORCE_ROTATE;
	}
	if(StringEquals(str, "CALL_CHILD_SKILL")) {
		return Type::CALL_CHILD_SKILL;
	}
	if(StringEquals(str, "LONG_JUMP")) {
		return Type::LONG_JUMP;
	}
	if(StringEquals(str, "TALK")) {
		return Type::TALK;
	}
	if(StringEquals(str, "EFFECT")) {
		return Type::EFFECT;
	}
	if(StringEquals(str, "AI_ADJUST")) {
		return Type::AI_ADJUST;
	}
	if(StringEquals(str, "INTERACTION")) {
		return Type::INTERACTION;
	}
	if(StringEquals(str, "DEATHWORM_BOUND")) {
		return Type::DEATHWORM_BOUND;
	}
	if(StringEquals(str, "UI")) {
		return Type::UI;
	}
	if(StringEquals(str, "CHANGE_MESH")) {
		return Type::CHANGE_MESH;
	}

	return Type::INVALID;
}

MovePreset MovePresetFromString(const char* str)
{
	if(StringEquals(str, "MOVE_PRESET_SEE_TARGET")) {
		return MovePreset::SEE_TARGET;
	}
	if(StringEquals(str, "MOVE_PRESET_ROTATE")) {
		return MovePreset::ROTATE;
	}
	if(StringEquals(str, "MOVE_PRESET_LIFTED")) {
		return MovePreset::LIFTED;
	}
	if(StringEquals(str, "MOVE_PRESET_GRAPH")) {
		return MovePreset::GRAPH;
	}
	if(StringEquals(str, "MOVE_PRESET_GRAPH_GROUND")) {
		return MovePreset::GRAPH_GROUND;
	}
	if(StringEquals(str, "MOVE_PRESET_MOVE_NONE")) {
		return MovePreset::MOVE_NONE;
	}
	if(StringEquals(str, "MOVE_PRESET_GOTO_CURRENT_TARGET")) {
		return MovePreset::GOTO_CURRENT_TARGET;
	}
	if(StringEquals(str, "MOVE_PRESET_GOTO_CURRENT_TARGET_CHASE")) {
		return MovePreset::GOTO_CURRENT_TARGET_CHASE;
	}
	if(StringEquals(str, "MOVE_PRESET_SEE_CURRENT_TARGET")) {
		return MovePreset::SEE_CURRENT_TARGET;
	}
	if(StringEquals(str, "MOVE_PRESET_AFTERIMAGE_MOVE")) {
		return MovePreset::AFTERIMAGE_MOVE;
	}
	if(StringEquals(str, "MOVE_PRESET_TARGET_POS")) {
		return MovePreset::TARGET_POS;
	}
	if(StringEquals(str, "MOVE_PRESET_TARGET_POS_LEAP")) {
		return MovePreset::TARGET_POS_LEAP;
	}
	if(StringEquals(str, "MOVE_PRESET_WARP")) {
		return MovePreset::WARP;
	}
	if(StringEquals(str, "MOVE_PRESET_WARP_TARGET_POS")) {
		return MovePreset::WARP_TARGET_POS;
	}
	if(StringEquals(str, "MOVE_PRESET_GOTO_CURRENT_TARGET_THROUGH")) {
		return MovePreset::GOTO_CURRENT_TARGET_THROUGH;
	}
	if(StringEquals(str, "MOVE_PRESET_GOTO_CURRENT_TARGET_THROUGH_CHASE")) {
		return MovePreset::GOTO_CURRENT_TARGET_THROUGH_CHASE;
	}
	if(StringEquals(str, "MOVE_PRESET_SEE_MEMORIZED_TARGET")) {
		return MovePreset::SEE_MEMORIZED_TARGET;
	}
	if(StringEquals(str, "MOVE_PRESET_CHASE_TARGET")) {
		return MovePreset::CHASE_TARGET;
	}
	if(StringEquals(str, "MOVE_PRESET_CHASE_TARGET_ATTACK")) {
		return MovePreset::CHASE_TARGET_ATTACK;
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
	if(StringEquals(str, "TARGET_SELF_FFF")) { return TargetPreset::SELF_FFF; }
	if(StringEquals(str, "TARGET_SELF_FFF_1")) { return TargetPreset::SELF_FFF_1; }
	if(StringEquals(str, "TARGET_SELF_TFF")) { return TargetPreset::SELF_TFF; }
	if(StringEquals(str, "TARGET_SELF_TTF")) { return TargetPreset::SELF_TTF; }
	if(StringEquals(str, "TARGET_SELF_TTT")) { return TargetPreset::SELF_TTT; }
	if(StringEquals(str, "TARGET_POS_TTT_1")) { return TargetPreset::POS_TTT_1; }
	if(StringEquals(str, "TARGET_POS_FFF")) { return TargetPreset::POS_FFF; }
	if(StringEquals(str, "TARGET_POS_TFF")) { return TargetPreset::POS_TFF; }
	if(StringEquals(str, "TARGET_POS_TFF_1_800_1000")) { return TargetPreset::POS_TFF_1_800_1000; }
	if(StringEquals(str, "TARGET_LAST_CHILD_FFF")) { return TargetPreset::LAST_CHILD_FFF; }
	if(StringEquals(str, "TARGET_LAST_CHILD_TFF")) { return TargetPreset::LAST_CHILD_TFF; }
	if(StringEquals(str, "TARGET_CHILD_INDEX_FFF")) { return TargetPreset::CHILD_INDEX_FFF; }
	if(StringEquals(str, "TARGET_MEMORIZED_TFT")) { return TargetPreset::MEMORIZED_TFT; }
	if(StringEquals(str, "TARGET_CURRENT_FFF")) { return TargetPreset::CURRENT_FFF; }
	if(StringEquals(str, "TARGET_LOCK_TFF")) { return TargetPreset::LOCK_TFF; }
	if(StringEquals(str, "TARGET_MULTI_LOCKON_FFF")) { return TargetPreset::MULTI_LOCKON_FFF; }
	return TargetPreset::INVALID;
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
	if(StringEquals(str, "E_BOUND_NONE")) { return BoundType::E_BOUND_NONE; }
	if(StringEquals(str, "E_BOUND_RAY")) { return BoundType::E_BOUND_RAY; }
	if(StringEquals(str, "E_BOUND_BOX")) { return BoundType::E_BOUND_BOX; }
	if(StringEquals(str, "E_BOUND_CAPSULE")) { return BoundType::E_BOUND_CAPSULE; }
	if(StringEquals(str, "E_BOUND_SPHERE")) { return BoundType::E_BOUND_SPHERE; }
	if(StringEquals(str, "E_BOUND_BEAM")) { return BoundType::E_BOUND_BEAM; }
	if(StringEquals(str, "E_BOUND_BEAM_NIF")) { return BoundType::E_BOUND_BEAM_NIF; }
	if(StringEquals(str, "E_BOUND_LASER")) { return BoundType::E_BOUND_LASER; }
	if(StringEquals(str, "E_BOUND_PHYSXPROP")) { return BoundType::E_BOUND_PHYSXPROP; }
	if(StringEquals(str, "E_BOUND_HAMMER")) { return BoundType::E_BOUND_HAMMER; }
	if(StringEquals(str, "E_BOUND_CASTER_MOVE_BOUND")) { return BoundType::E_BOUND_CASTER_MOVE_BOUND; }
	return BoundType::INVALID;
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
	if(StringEquals(str, "eNONE")) { return DamageGroup::eNONE; }
	if(StringEquals(str, "eENEMY")) { return DamageGroup::eENEMY; }
	if(StringEquals(str, "eFRIEND")) { return DamageGroup::eFRIEND; }
	if(StringEquals(str, "eALL")) { return DamageGroup::eALL; }
	return DamageGroup::INVALID;
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
	if(StringEquals(str, "E_REMOTE_BEHAVIOR_TYPE_NONETARGET")) { return BehaviorType::NONETARGET; }
	if(StringEquals(str, "E_REMOTE_BEHAVIOR_TYPE_HOMING")) { return BehaviorType::HOMING; }
	if(StringEquals(str, "E_REMOTE_BEHAVIOR_TYPE_RETURN")) { return BehaviorType::RETURN; }
	if(StringEquals(str, "E_REMOTE_BEHAVIOR_TYPE_CASTER_TARGET")) { return BehaviorType::CASTER_TARGET; }
	if(StringEquals(str, "E_REMOTE_BEHAVIOR_TYPE_BEAM")) { return BehaviorType::BEAM; }
	if(StringEquals(str, "E_REMOTE_BEHAVIOR_TYPE_CHAIN")) { return BehaviorType::CHAIN; }
	if(StringEquals(str, "E_REMOTE_BEHAVIOR_TYPE_GRAPH")) { return BehaviorType::GRAPH; }
	if(StringEquals(str, "E_REMOTE_BEHAVIOR_TYPE_TARGET")) { return BehaviorType::TARGET; }
	if(StringEquals(str, "E_REMOTE_BEHAVIOR_TYPE_FOLLOW_TARGET")) { return BehaviorType::FOLLOW_TARGET; }
	if(StringEquals(str, "E_REMOTE_BEHAVIOR_TYPE_ATTACH_TARGET")) { return BehaviorType::ATTACH_TARGET; }
	if(StringEquals(str, "E_REMOTE_BEHAVIOR_TYPE_ATTACH_MUZZLE")) { return BehaviorType::ATTACH_MUZZLE; }
	if(StringEquals(str, "E_REMOTE_BEHAVIOR_TYPE_TARGET_GROUNDPOSITION")) { return BehaviorType::TARGET_GROUNDPOSITION; }
	if(StringEquals(str, "E_REMOTE_BEHAVIOR_TYPE_ORBIT")) { return BehaviorType::ORBIT; }
	return BehaviorType::INVALID;
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
