#include <common/utils.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EASprintf.h>
#include <tinyxml2.h>

#include "core.h"
#include "game_content.h"

using namespace tinyxml2;
constexpr eastl::hash<const char*> strHash;

static GameXmlContent* g_GameXmlContent = nullptr;

static Path gameDataDir = L"gamedata";

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
			const char* unlock = pSkillElt->Attribute("_UnLock");
			u8 unlocked = 1;
			if(unlock && (unlock[0] == 'F' || unlock[0] == 'f')) unlocked = 0;
			master.skillUnlocked.push_back(unlocked);
			pSkillElt = pSkillElt->NextSiblingElement();
		} while(pSkillElt);

		XMLElement* pEquip = pNodeMaster->FirstChildElement("EquipComData");
		if(pEquip) {
			for(XMLElement* pDef = pEquip->FirstChildElement("_DefaultWeaponIndex"); pDef; pDef = pDef->NextSiblingElement("_DefaultWeaponIndex")) {
				i32 wid = 0;
				if(pDef->QueryIntAttribute("DATA", &wid) == XML_SUCCESS && master.defaultWeaponIDs.size() < master.defaultWeaponIDs.capacity()) {
					master.defaultWeaponIDs.push_back((WeaponIndex)wid);
				}
			}
			for(XMLElement* pFair = pEquip->FirstChildElement("_FairPvPWeaponIndex"); pFair; pFair = pFair->NextSiblingElement("_FairPvPWeaponIndex")) {
				i32 wid = 0;
				if(pFair->QueryIntAttribute("DATA", &wid) == XML_SUCCESS && master.fairPvpWeaponIDs.size() < master.fairPvpWeaponIDs.capacity()) {
					master.fairPvpWeaponIDs.push_back((WeaponIndex)wid);
				}
			}
		}

		// save master data
		master.ID = (CreatureIndex)masterID;
		master.classType = (ClassType)(masterID - 100000000);
		master.className = className;
		masterClassStringMap.emplace(strHash(master.className.data()), &master);
		DBG_ASSERT(masterClassStringMap.find(strHash("CLASS_TYPE_STRIKER")) != masterClassStringMap.end());

		masterClassTypeMap.emplace(master.classType, &master);
		masterIdMap.emplace(master.ID, &master);

	}

	return true;
}

bool GameXmlContent::LoadCharacterBaseStats()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/CHARACTER_BASE_STATS.xml", doc)) return false;

	XMLElement* root = doc.FirstChildElement();
	ASSERT(root);
	for(XMLElement* info = root->FirstChildElement(); info; info = info->NextSiblingElement()) {
		i32 id = 0;
		if(info->QueryAttribute("ID", &id) != XML_SUCCESS) continue;
		auto found = masterIdMap.find((CreatureIndex)id);
		if(found == masterIdMap.end()) {
			WARN("CHARACTER_BASE_STATS unknown master %d", id);
			continue;
		}
		Master& master = *found->second;

		for(XMLElement* st = info->FirstChildElement("STAT"); st; st = st->NextSiblingElement("STAT")) {
			i32 type = 0;
			f32 value = 0.f;
			st->QueryAttribute("type", &type);
			st->QueryAttribute("value", &value);
			ASSERT(master.baseStats.size() < master.baseStats.capacity());
			master.baseStats.push_back({ (u8)type, value });

		}
	}

	foreach(it, masters) {
		ASSERT(!it->baseStats.empty());
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
		ASSERT(found != masterClassTypeMap.end());
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
		{
			i32 baseCombo = 0;
			pNodeCommonSkill->QueryIntAttribute("_BaseComboSkill", &baseCombo);
			if(baseCombo != 0 && type == SkillType::COMBO && firstComboSkill.find(baseCombo) == firstComboSkill.end()) firstComboSkill[baseCombo] = skillID;
		}
		{
			SkillInput in;
			const char* smart = nullptr;
			const char* ux = nullptr;
			const char* select = nullptr;
			pNodeCommonSkill->QueryStringAttribute("_SmartCast", &smart);
			if(XMLElement* pTarget = pNodeCommonSkill->FirstChildElement("_Target")) {
				pTarget->QueryStringAttribute("_UXType", &ux);
				pTarget->QueryStringAttribute("_SelectStateType", &select);
				const char* selectType = pTarget->Attribute("_SelectType");
				in.teleportEntity = selectType && EA::StdC::Strcmp(selectType, "SKILL_TARGET_TELEPORT_ENTITY") == 0;
				in.allEnemies = selectType && EA::StdC::Strcmp(selectType, "SKILL_TARGET_ENEMY_PC_ALL") == 0;
				in.pickedEntity = selectType && EA::StdC::Strcmp(selectType, "SKILL_TARGET_PICKED_ENTITY") == 0;
				in.lockOn = selectType && EA::StdC::Strcmp(selectType, "SKILL_TARGET_MULTI_LOCKON") == 0;
				if(selectType && EA::StdC::Strcmp(selectType, "SKILL_TARGET_STATUS") == 0) pTarget->QueryAttribute("_CheckStatusIndex", &in.checkStatus);
				if(const char* om = pTarget->Attribute("_TestObstructMove")) in.obstructMove = EA::StdC::Strcmp(om, "SKILL_OBSTRUCTMOVE_TYPE_JUMPABLE") == 0 ? 1 : 2;
				if(const char* os = pTarget->Attribute("_TestObstructSight")) in.obstructSight = EA::StdC::Strcmp(os, "SKILL_OBSTRUCTSIGHT_TYPE_ALL") == 0;
				pTarget->QueryFloatAttribute("_MinDistance", &in.minDistance);
				pTarget->QueryFloatAttribute("_MaxDistance", &in.maxDistance);
				const char* friendly = pTarget->Attribute("_Friendly");
				const char* unfriendly = pTarget->Attribute("_UnFriendly");
				in.friendlyOnly = friendly && (friendly[0] == 'T' || friendly[0] == 't') && unfriendly && (unfriendly[0] == 'F' || unfriendly[0] == 'f');
			}
			const bool self = !ux || EA::StdC::Strcmp(ux, "SKILL_CAST_MY_POSITION") == 0;
			const bool smartCast = smart && EA::StdC::Strcmp(smart, "SKILL_SMARTCAST_TRUE") == 0;
			const bool selectBefore = select && EA::StdC::Strcmp(select, "SKILL_TARGET_SELECT_STATE_BEFORECAST") == 0;
			in.needsClick = (!self || selectBefore) && !smartCast;
			in.directionOnly = ux && EA::StdC::Strcmp(ux, "SKILL_CAST_MY_POSITION_DIRECTION") == 0;
			in.selfPosition = self;
			in.positionDirection = ux && EA::StdC::Strcmp(ux, "SKILL_CAST_TARGET_POSITION_DIRECTION") == 0;
			pNodeCommonSkill->QueryIntAttribute("_CoolTimeID", &in.coolTimeID);
			pNodeCommonSkill->QueryFloatAttribute("_CoolTime", &in.coolTime);
			skillInput[skillID] = in;
		}
		if(XMLElement* pCast = pNodeCommonSkill->FirstChildElement("_Cast")) {
			const char* castType = nullptr;
			const char* castAction = nullptr;
			pCast->QueryStringAttribute("_CastType", &castType);
			pCast->QueryStringAttribute("_CastAction", &castAction);
			SkillCast sc;
			if(castType && EA::StdC::Strcmp(castType, "SKILL_CAST_TIME_LIMIT") == 0) sc.type = SkillCast::LIMIT;
			else if(castType && EA::StdC::Strcmp(castType, "SKILL_CAST_TIME_CHARGING") == 0) sc.type = SkillCast::CHARGING;
			pCast->QueryFloatAttribute("_CastTime", &sc.time);
			pCast->QueryBoolAttribute("_InterruptByMove", &sc.interruptByMove);
			sc.action = castAction ? ActionStateFromString(castAction) : ActionStateID::INVALID;
			if(castType && sc.action != ActionStateID::INVALID && sc.time > 0) skillCast[skillID] = sc;
			if(type == SkillType::TOGGLE && skill.action == ActionStateID::INVALID) skill.action = sc.action;
		}
		if(type == SkillType::TOGGLE) {
			SkillToggle tg;
			if(XMLElement* pToggle = pNodeCommonSkill->FirstChildElement("_Toggle")) {
				f32 intervalMs = 0;
				pToggle->QueryFloatAttribute("_ConsumeInterval", &intervalMs);
				tg.interval = intervalMs / 1000.0f;
				pToggle->QueryFloatAttribute("_ConsumeValue", &tg.value);
				const char* ct = pToggle->Attribute("_ConsumeType");
				if(ct && EA::StdC::Strcmp(ct, "STAT_TYPE_MANA") == 0) tg.consume = SkillToggle::MANA;
				else if(ct && EA::StdC::Strcmp(ct, "STAT_TYPE_RAGE") == 0) tg.consume = SkillToggle::RAGE;
				else if(ct && EA::StdC::Strcmp(ct, "STAT_TYPE_STAMINA") == 0) tg.consume = SkillToggle::STAMINA;
			}
			XMLElement* pLists = pNodeCommonSkill->FirstChildElement("_Toggle");
			if(!pLists) pLists = pNodeCommonSkill;
			for(XMLElement* p = pLists->FirstChildElement("_RemoteList"); p; p = p->NextSiblingElement("_RemoteList")) {
				i32 id = 0;
				p->QueryAttribute("DATA", &id);
				if(id > 0 && !tg.remotes.full()) tg.remotes.push_back(id);
			}
			for(XMLElement* p = pLists->FirstChildElement("_StatusList"); p; p = p->NextSiblingElement("_StatusList")) {
				i32 id = 0;
				p->QueryAttribute("DATA", &id);
				if(id > 0 && !tg.statuses.full()) tg.statuses.push_back(id);
			}
			skillToggle[skillID] = tg;
		}
		if(XMLElement* pStance = pNodeCommonSkill->FirstChildElement("_STANCE")) {
			const char* stanceType = nullptr;
			pStance->QueryStringAttribute("_Type", &stanceType);
			SkillStance st;
			st.stance = StanceFromString(stanceType);
			pStance->QueryFloatAttribute("_DurationTime", &st.duration);
			pStance->QueryIntAttribute("_WeaponFireCount", &st.weaponFireCount);
			pStance->QueryBoolAttribute("_CancelByDefeat", &st.cancelByDefeat);
			if(st.stance > 0) skillStance[skillID] = st;
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
		i32 mapDocID = 0;
		pMapElt->QueryAttribute("_Index", &mapDocID);
		mapList.index = MapIndex(mapDocID);

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

bool GameXmlContent::LoadMapByID(Map* map, MapIndex index)
{
	const MapList* mapList = FindMapListByID(index);
	if (!mapList){
		LOG("ERROR(LoadMapByID): Map not found %d", (i32)index);
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

		pSpawnElt->QueryAttribute("dwType", &spawn.entityType);
		pSpawnElt->QueryAttribute("dwSpawnAnim", &spawn.spawnAnim);
		pSpawnElt->QueryAttribute("AIWanderDistOverride", &spawn.wanderDist);
		pSpawnElt->QueryAttribute("_TagID", &spawn.tagID);
		const char* actionStr = nullptr;
		if(pSpawnElt->QueryStringAttribute("strActionState", &actionStr) == XML_SUCCESS && actionStr) {
			spawn.actionState = ActionStateFromString(actionStr);
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

		pSpawnElt->QueryAttribute("dwType", &spawn.entityType);
		pSpawnElt->QueryAttribute("dwSpawnAnim", &spawn.spawnAnim);
		pSpawnElt->QueryAttribute("AIWanderDistOverride", &spawn.wanderDist);
		pSpawnElt->QueryAttribute("_TagID", &spawn.tagID);
		const char* actionStr = nullptr;
		if(pSpawnElt->QueryStringAttribute("strActionState", &actionStr) == XML_SUCCESS && actionStr) {
			spawn.actionState = ActionStateFromString(actionStr);
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


bool GameXmlContent::LoadLobby(MapIndex index)
{
	const MapList* map = FindMapListByID(index);
	if (!map) {
		LOG("ERROR(LoadLobby): Map not found %d", (i32)index);
		return false;
	};

	if (map->mapType != MapType::MAP_CITY)
	{
		LOG("ERROR(LoadLobby): Map index: %d is not from MapType MAP_CITY", (i32)index);
		return false;
	}

	return LoadMapByID(&mapLobby, index);
}

bool GameXmlContent::LoadPvpDeathmach()
{
	const MapList* map = FindMapListByID(MapIndex::PVP_DEATHMATCH);
	if (!map) {
		LOG("ERROR(LoadPvpDeathmach): Map not found %d", (i32)MapIndex::PVP_DEATHMATCH);
		return false;
	};

	return LoadMapByID(&mapPvpDeathMatch, MapIndex::PVP_DEATHMATCH);
}

bool GameXmlContent::LoadEntrySystems()
{
	XMLDocument docEntry;
	if(!LoadXMLFile(L"/Design/DOCUMENT/EntrySystemEX.xml", docEntry)) return false;

	for(XMLElement* pInfo = docEntry.FirstChildElement()->FirstChildElement();
		pInfo;
		pInfo = pInfo->NextSiblingElement()) {
		EntrySystem entry;
		if(pInfo->QueryIntAttribute("ID", &entry.ID) != XML_SUCCESS) continue;
		if(XMLElement* pSys = pInfo->FirstChildElement()) {
			const char* t = pSys->Attribute("_EntryType");
			if(t) entry.entryType = t;
		}
		for(XMLElement* pArea = pInfo->FirstChildElement()->FirstChildElement("_Area");
			pArea;
			pArea = pArea->NextSiblingElement("_Area")) {
			i32 areaID = 0;
			if(pArea->QueryIntAttribute("DATA", &areaID) != XML_SUCCESS) continue;
			if(entry.areas.size() < entry.areas.capacity()) entry.areas.push_back(AreaIndex(areaID));
		}
		if(entrySystems.size() < entrySystems.capacity()) entrySystems.push_back(entry);
	}

	XMLDocument docArea;
	if(!LoadXMLFile(L"/Design/DOCUMENT/AREALIST.xml", docArea)) return false;

	for(XMLElement* pInfo = docArea.FirstChildElement()->FirstChildElement();
		pInfo;
		pInfo = pInfo->NextSiblingElement()) {
		AreaStages area;
		i32 areaDocID = 0;
		if(pInfo->QueryIntAttribute("ID", &areaDocID) != XML_SUCCESS) continue;
		area.ID = AreaIndex(areaDocID);
		for(XMLElement* pStage = pInfo->FirstChildElement()->FirstChildElement("_StageList");
			pStage;
			pStage = pStage->NextSiblingElement("_StageList")) {
			i32 stageID = 0;
			if(pStage->QueryIntAttribute("_NormalStageIndex", &stageID) != XML_SUCCESS) continue;
			if(stageID != 0 && area.stages.size() < area.stages.capacity()) area.stages.push_back(StageIndex(stageID));
		}
		if(areaStages.size() < areaStages.capacity()) areaStages.push_back(area);
	}

	XMLDocument docSchedule;
	if(!LoadXMLFile(L"/Design/DOCUMENT/SCHEDULE.xml", docSchedule)) return false;

	for(XMLElement* pInfo = docSchedule.FirstChildElement()->FirstChildElement();
		pInfo;
		pInfo = pInfo->NextSiblingElement()) {
		XMLElement* pEvent = pInfo->FirstChildElement();
		if(!pEvent) continue;
		XMLElement* pEntry = pEvent->FirstChildElement("_ENTRYSYSTEM");
		if(!pEntry) continue;
		i32 entryID = 0;
		if(pEntry->QueryIntAttribute("_Index", &entryID) != XML_SUCCESS) continue;
		XMLElement* pAreaList = pEvent->FirstChildElement("_AREA_LIST");
		if(!pAreaList) continue;
		for(auto& entry : entrySystems) {
			if(entry.ID != entryID) continue;
			for(XMLElement* pArea = pAreaList->FirstChildElement("_AREA");
				pArea;
				pArea = pArea->NextSiblingElement("_AREA")) {
				i32 areaID = 0;
				if(pArea->QueryIntAttribute("_AreaIndex", &areaID) != XML_SUCCESS) continue;
				if(areaID != 0 && entry.scheduleAreas.size() < entry.scheduleAreas.capacity()) entry.scheduleAreas.push_back(AreaIndex(areaID));
			}
		}
	}

LOG("Loaded %d entry systems, %d areas", (i32)entrySystems.size(), (i32)areaStages.size());
return true;
}



bool GameXmlContent::HasEntrySystem(i32 entryID) const
{
	for(auto& entry : entrySystems) {
		if(entry.ID == entryID) return true;
	}
	return false;
}

bool GameXmlContent::FindQueueAreaStage(i32 entryID, AreaIndex* outAreaIndex, StageIndex* outStageIndex) const
{
	for(auto& entry : entrySystems) {
		if(entry.ID != entryID) continue;
		for(auto areaID : entry.areas) {
			for(auto& area : areaStages) {
				if(area.ID != areaID) continue;
				if(area.stages.empty()) continue;
				*outAreaIndex = area.ID;
				*outStageIndex = area.stages[0];
				return true;
			}
		}
		for(auto areaID : entry.scheduleAreas) {
			for(auto& area : areaStages) {
				if(area.ID != areaID) continue;
				if(area.stages.empty()) continue;
				*outAreaIndex = area.ID;
				*outStageIndex = area.stages[0];
				return true;
			}
		}
		return false;
	}
	return false;
}

bool GameXmlContent::LoadStageMaps()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/Design/DOCUMENT/STAGELIST.xml", doc)) return false;

	for(XMLElement* pInfo = doc.FirstChildElement()->FirstChildElement();
		pInfo;
		pInfo = pInfo->NextSiblingElement()) {
		StageMaps stage;
		i32 stageDocID = 0;
		if(pInfo->QueryIntAttribute("ID", &stageDocID) != XML_SUCCESS) continue;
		stage.ID = StageIndex(stageDocID);
		XMLElement* pStage = pInfo->FirstChildElement();
		if(!pStage) continue;
		pStage->QueryIntAttribute("_JoinMemberMax", &stage.joinMemberMax);
		XMLElement* pWorldMap = pStage->FirstChildElement("_WORLD_STAGE_MAP");
		if(!pWorldMap) continue;
		for(XMLElement* pGroup = pWorldMap->FirstChildElement("_WORLD_STAGE_MAP_GROUP");
			pGroup;
			pGroup = pGroup->NextSiblingElement("_WORLD_STAGE_MAP_GROUP")) {
			for(XMLElement* pMap = pGroup->FirstChildElement("_WORLD_STAGE_MAPINFO");
				pMap;
				pMap = pMap->NextSiblingElement("_WORLD_STAGE_MAPINFO")) {
				i32 mapID = 0;
				if(pMap->QueryIntAttribute("_Index", &mapID) != XML_SUCCESS) continue;
				if(mapID != 0 && stage.maps.size() < stage.maps.capacity()) stage.maps.push_back(MapIndex(mapID));
			}
		}
		if(!stage.maps.empty() && stageMaps.size() < stageMaps.capacity()) stageMaps.push_back(stage);
	}

	LOG("Loaded %d stage maps", (i32)stageMaps.size());
	return true;
}

bool GameXmlContent::FindStageMap(StageIndex stageID, MapIndex* outMapIndex) const
{
	for(auto& stage : stageMaps) {
		if(stage.ID != stageID) continue;
		if(stage.maps.empty()) return false;
		*outMapIndex = stage.maps[0];
		return true;
	}
	return false;
}

const GameXmlContent::StageMaps* GameXmlContent::FindStageMaps(StageIndex stageID) const
{
	for(auto& stage : stageMaps) {
		if(stage.ID == stageID) return &stage;
	}
	return nullptr;
}
bool GameXmlContent::LoadBotCreatures()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/Design/DOCUMENT/CREATURE_MONSTER_BOT.xml", doc)) return false;

	for(XMLElement* pInfo = doc.FirstChildElement()->FirstChildElement();
		pInfo;
		pInfo = pInfo->NextSiblingElement()) {
		XMLElement* pEntity = pInfo->FirstChildElement("EntityComData");
		if(!pEntity) continue;
		const char* keyName = pEntity->Attribute("KEYNAME");
		if(!keyName || !EA::StdC::Strstr(keyName, "DeathMatch")) continue;

		i32 id = 0;
		if(pInfo->QueryIntAttribute("ID", &id) != XML_SUCCESS) continue;

		XMLElement* pStats = pInfo->FirstChildElement("StatsComData");
		if(!pStats) continue;
		const char* className = nullptr;
		pStats->QueryStringAttribute("_class", &className);
		if(!className) continue;

		auto found = masterClassStringMap.find(strHash(className));
		if(found == masterClassStringMap.end()) continue;
		if(deathMatchBotIndex.find(found->second->classType) != deathMatchBotIndex.end()) continue;
		deathMatchBotIndex.emplace(found->second->classType, (CreatureIndex)id);
	}

	LOG("Loaded %d deathmatch bot creatures", (i32)deathMatchBotIndex.size());
	return true;
}

CreatureIndex GameXmlContent::FindDeathMatchBotIndex(ClassType classType) const
{
	auto found = deathMatchBotIndex.find(classType);
	if(found == deathMatchBotIndex.end()) return CreatureIndex::Invalid;
	return found->second;
}


bool GameXmlContent::LoadGuildData()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/Design/GAMEINFO/Guild.xml", doc)) return false;

	XMLElement* pInfo = doc.FirstChildElement()->FirstChildElement("ST_GUILDINFO");
	if(!pInfo) return false;

	for(XMLElement* pLvl = pInfo->FirstChildElement("_GuildLevel");
		pLvl;
		pLvl = pLvl->NextSiblingElement("_GuildLevel")) {
		GuildLevelInfo lvl;
		if(pLvl->QueryIntAttribute("DATAKEY", &lvl.level) != XML_SUCCESS) continue;
		pLvl->QueryIntAttribute("_RequireGuildPoint", &lvl.requirePoint);
		pLvl->QueryIntAttribute("_GuildMedalGiftCount", &lvl.medalGiftCount);
		if(guildLevels.size() < guildLevels.capacity()) guildLevels.push_back(lvl);
	}

	XMLElement* pSkillSet = pInfo->FirstChildElement("_GuildSkillSet");
	if(pSkillSet) {
		for(XMLElement* pSkill = pSkillSet->FirstChildElement("_GuildSkill");
			pSkill;
			pSkill = pSkill->NextSiblingElement("_GuildSkill")) {
			GuildSkillInfo skill;
			const char* key = pSkill->Attribute("DATAKEY");
			if(!key) continue;
			skill.key = key;
			for(XMLElement* pLvl = pSkill->FirstChildElement("_GuildSkillLevel");
				pLvl;
				pLvl = pLvl->NextSiblingElement("_GuildSkillLevel")) {
				GuildSkillLevel sl;
				if(pLvl->QueryIntAttribute("DATAKEY", &sl.level) != XML_SUCCESS) continue;
				pLvl->QueryIntAttribute("_UnlockGuildLevel", &sl.unlockGuildLevel);
				pLvl->QueryIntAttribute("_Cost", &sl.cost);
				pLvl->QueryIntAttribute("_Value", &sl.value);
				if(skill.levels.size() < skill.levels.capacity()) skill.levels.push_back(sl);
			}
			if(guildSkills.size() < guildSkills.capacity()) guildSkills.push_back(skill);
		}
	}

	for(XMLElement* pEmblem = pInfo->FirstChildElement("_ValidGuildEmblem");
		pEmblem;
		pEmblem = pEmblem->NextSiblingElement("_ValidGuildEmblem")) {
		i32 emblem = 0;
		if(pEmblem->QueryIntAttribute("DATA", &emblem) != XML_SUCCESS) continue;
		if(emblem != 0 && validGuildEmblems.size() < validGuildEmblems.capacity()) validGuildEmblems.push_back(emblem);
	}

	if(XMLElement* pAct = pInfo->FirstChildElement("_MaxGuildActivityPointPerDay")) {
		if(XMLElement* pStage = pAct->FirstChildElement("_Stage")) {
			pStage->QueryIntAttribute("_Weekday", &guildActivityCapWeekday);
		}
	}

	LOG("Loaded %d guild levels, %d skills, %d emblems", (i32)guildLevels.size(), (i32)guildSkills.size(), (i32)validGuildEmblems.size());
	return true;
}

i32 GameXmlContent::GuildLevelForPoints(i32 points) const
{
	i32 level = 1;
	for(auto& l : guildLevels) {
		if(points >= l.requirePoint && l.level > level) level = l.level;
	}
	return level;
}

i32 GameXmlContent::GuildSkillValue(const char* key, i32 level) const
{
	for(auto& s : guildSkills) {
		if(s.key != key) continue;
		for(auto& l : s.levels) {
			if(l.level == level) return l.value;
		}
	}
	return 0;
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

u16 GameXmlContent::InternNodeName(const char* name)
{
	if(nodeNames.empty()) nodeNames.push_back(L"Scene Root");
	if(!name || !name[0]) return 0;
	eastl::fixed_string<wchar,48,false> w;
	for(const char* c = name; *c && w.size() < 47; c++) w.push_back((wchar)*c);
	for(u16 i = 0; i < (u16)nodeNames.size(); i++) {
		if(nodeNames[i] == w) return i;
	}
	nodeNames.push_back(w);
	return (u16)(nodeNames.size() - 1);
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
	i8 prevStance = 0;
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
			VERBOSE("%s:", classStr.data());

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
		const char* StanceStr = nullptr;
		pActionBase->QueryStringAttribute("Stance", &StanceStr);
		const i8 stance = StanceFromString(StanceStr);

		if(actionID != prevActionID || stance != prevStance) {
			prevActionID = actionID;
			prevStance = stance;

			bool foundAction = false;
			for(int i = actionSliceStart; i < actionSliceStart+actionSliceCount; i++) {
				if(actionList[i].ID == actionID && actionList[i].stance == stance) {
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
				curAction->stance = stance;
				auto foundSeq = aniLenListMap.find(masterClassType);
				if(foundSeq != aniLenListMap.end()) {
					foreach_const(it, foundSeq->second) {
						if(it->ID == AnimationId) {
							curAction->seqLength = it->length;

							break;
						}
					}
				}

				VERBOSE("	ID='%s' (%d)", ActionStateToString(curAction->ID), curAction->ID);
				VERBOSE("		seqLength=%f", curAction->seqLength);

				accumulatedDelay = 0.0f;
			}
		}

		{
			i32 animIndex = 0, animId = 0, animLoop = 1;
			pActionBase->QueryAttribute("AnimationIndex", &animIndex);
			pActionBase->QueryAttribute("AnimationId", &animId);
			pActionBase->QueryAttribute("AnimationLoop", &animLoop);
			if(animIndex >= 0 && animId > 0) {
				Action::AnimStep* known = nullptr;
				for(auto& a : curAction->anims) if(a.index == animIndex) known = &a;
				if(!known && !curAction->anims.full()) {
					curAction->anims.push_back();
					known = &curAction->anims.back();
					known->index = animIndex;
					known->animationId = animId;
				}
				if(known && animLoop > known->loop) known->loop = animLoop;
			}
		}

		const char* CommandType = nullptr;
		if(pActionBase->QueryStringAttribute("CommandType", &CommandType) != XMLError::XML_SUCCESS) {
			continue;
		}

		Action::Command cmd;
		cmd.type = ActionCommand::TypeFromString(CommandType);
		pActionBase->QueryAttribute("AnimationIndex", &cmd.animIndex);
		pActionBase->QueryAttribute("AnimationId", &cmd.animationId);
		{
			const char* NodeName = nullptr;
			pActionBase->QueryStringAttribute("NodeName", &NodeName);
			cmd.nodeName = InternNodeName(NodeName);
			const char* P2 = nullptr;
			pActionBase->QueryStringAttribute("Param2", &P2);
			cmd.byDummyDir = P2 && EA::StdC::Strstr(P2, "ByDummyDir") ? 1 : 0;
		}
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

			case ActionCommand::Type::REMOTE:
			case ActionCommand::Type::REMOTE_SKILL_TARGET: {
				const char* TargetPreset = nullptr;
				//const char* Param2;
				i32 RemoteIndex;
				pActionBase->QueryStringAttribute("TargetPreset", &TargetPreset);
				//pActionBase->QueryStringAttribute("Param2", &Param2);
				pActionBase->QueryAttribute("RemoteIndex", &RemoteIndex);

				cmd.remote.idx = RemoteIdx(RemoteIndex);
				cmd.remote.targetPreset = TargetPreset ? ActionCommand::TargetPresetFromString(TargetPreset) : ActionCommand::TargetPreset::CURRENT_FFF;
				cmd.remote.offsetSide = 0;
				cmd.remote.offsetFront = 0;
				cmd.remote.removeAtAnimEnd = 0;
				{
					const char* P2 = nullptr;
					pActionBase->QueryStringAttribute("Param2", &P2);
					cmd.remote.stopAtTarget = P2 && EA::StdC::Strstr(P2, "DestroyAwayTarget") ? 1 : 0;
					cmd.remote.remove = P2 && (EA::StdC::Strcmp(P2, "Remove") == 0 || EA::StdC::Strcmp(P2, "RemoveAll") == 0) ? 1 : 0;
					cmd.remote.removeAtAnimEnd = P2 && EA::StdC::Strstr(P2, "RemoveWhenAniEnd") ? 1 : 0;
				}
				const char* TargetParam = nullptr;
				pActionBase->QueryStringAttribute("TargetParam", &TargetParam);
				if(TargetParam) {
					f32 p[7] = { 0 };
					if(EA::StdC::Sscanf(TargetParam, "%f|%f|%f|%f|%f|%f|%f", &p[0], &p[1], &p[2], &p[3], &p[4], &p[5], &p[6]) == 7) {
						cmd.remote.offsetSide = p[5];
						cmd.remote.offsetFront = p[6];
					}
				}
				ASSERT(cmd.remote.targetPreset != ActionCommand::TargetPreset::INVALID);
			} break;

			case ActionCommand::Type::GRAPH_MOVE_HORZ: {
				pActionBase->QueryAttribute("Param1", &cmd.graphMoveHorz.distance);
			} break;

			case ActionCommand::Type::TELEPORT: {
				const char* Param1 = nullptr;
				const char* TargetPreset = nullptr;
				pActionBase->QueryStringAttribute("Param1", &Param1);
				pActionBase->QueryStringAttribute("TargetPreset", &TargetPreset);
				cmd.teleport.toLastChild = Param1 && TargetPreset && EA::StdC::Strcmp(Param1, "TELEPORT_TARGET_ENTITY") == 0 &&
					ActionCommand::TargetPresetFromString(TargetPreset) == ActionCommand::TargetPreset::LAST_CHILD_FFF ? 1 : 0;
			} break;

			case ActionCommand::Type::INTERACTION: {
				cmd.interaction.interactionID = 0;
				pActionBase->QueryAttribute("Param1", &cmd.interaction.interactionID);
			} break;

			case ActionCommand::Type::ENTITY: {
				cmd.entity.entityIndex = 0;
				pActionBase->QueryAttribute("EntityIndex", &cmd.entity.entityIndex);
				const char* TargetPreset = nullptr;
				pActionBase->QueryStringAttribute("TargetPreset", &TargetPreset);
				cmd.entity.targetPreset = TargetPreset ? ActionCommand::TargetPresetFromString(TargetPreset) : ActionCommand::TargetPreset::SELF_FFF;
			} break;

			case ActionCommand::Type::STATUS:
			case ActionCommand::Type::STATUS_SKILL_TARGET: {
				cmd.status.statusIndex = 0;
				pActionBase->QueryAttribute("StatusIndex", &cmd.status.statusIndex);
				const char* TargetPreset = nullptr;
				pActionBase->QueryStringAttribute("TargetPreset", &TargetPreset);
				cmd.status.targetPreset = TargetPreset ? ActionCommand::TargetPresetFromString(TargetPreset) : ActionCommand::TargetPreset::CURRENT_FFF;
				if(cmd.type == ActionCommand::Type::STATUS_SKILL_TARGET) cmd.status.targetPreset = ActionCommand::TargetPreset::CURRENT_FFF;
				const char* Param2 = nullptr;
				pActionBase->QueryStringAttribute("Param2", &Param2);
				cmd.status.removeAtEnd = Param2 && EA::StdC::Strstr(Param2, "RemoveWhenAniEnd") ? 1 : 0;
				cmd.status.remove = Param2 && EA::StdC::Strcmp(Param2, "Remove") == 0 ? 1 : 0;
			} break;

			case ActionCommand::Type::ROTATESPEED: {
				pActionBase->QueryAttribute("Param1", &cmd.rotateSpeed.speed);
			} break;
		}

		curAction->commands.push_back(cmd);

		VERBOSE("		Command='%s' delay=%.2f relative=%.2f", CommandType, cmd.delay, accumulatedDelay);
		switch(cmd.type) {
			case ActionCommand::Type::STATE_BLOCK: {

			} break;

			case ActionCommand::Type::MOVE: {
				VERBOSE("		  preset='%s'", ActionCommand::MovePresetToString(cmd.move.preset));
				VERBOSE("		  param2=%d", cmd.move.param2);
			} break;

			case ActionCommand::Type::GRAPH_MOVE_HORZ: {
				VERBOSE("		  distance=%f", cmd.graphMoveHorz.distance);
			} break;

			case ActionCommand::Type::ROTATESPEED: {
				VERBOSE("		  speed=%d", cmd.rotateSpeed.speed);
			} break;
		}
	}

	if(actionSliceCount > 0) {
		actionListMap.emplace(prevMasterClassType, Slice<Action>(&actionList[actionSliceStart], actionSliceCount));
	}

	return true;
}

static void ParseRemoteSpawnRules(XMLElement* pEvent, Remote::SpawnRules& out)
{
	if(!pEvent) return;
	for(XMLElement* pAct = pEvent->FirstChildElement(); pAct; pAct = pAct->NextSiblingElement()) {
		if(EA::StdC::Strcmp(pAct->Name(), "_CreateEntity") == 0) {
			Remote::SpawnRule rule;
			i32 idx = -1;
			pAct->QueryAttribute("_RemoteIndex", &idx);
			pAct->QueryAttribute("_CreatureIndex", &rule.creature);
			if((idx <= 0 && rule.creature <= 0) || out.full()) continue;
			if(idx > 0) rule.idx = RemoteIdx(idx);
			out.push_back(rule);
		}
		else if(EA::StdC::Strcmp(pAct->Name(), "_CreateRemote") == 0) {
			const char* toOwner = nullptr;
			pAct->QueryStringAttribute("_TargetOwner", &toOwner);
			const bool targetOwner = toOwner && EA::StdC::Stricmp(toOwner, "true") == 0;
			for(XMLElement* pKind = pAct->FirstChildElement(); pKind; pKind = pKind->NextSiblingElement()) {
				Remote::SpawnRule rule;
				rule.targetOwner = targetOwner;
				i32 idx = -1;
				if(EA::StdC::Strcmp(pKind->Name(), "_Radiate") == 0) {
					rule.kind = Remote::SPAWN_RADIATE;
					pKind->QueryAttribute("_Index", &idx);
					pKind->QueryAttribute("_MaxNum", &rule.maxNum);
					pKind->QueryFloatAttribute("_MaxAngle", &rule.maxAngle);
				}
				else if(EA::StdC::Strcmp(pKind->Name(), "_CircleArea") == 0) {
					rule.kind = Remote::SPAWN_CIRCLE;
					pKind->QueryAttribute("_Index", &idx);
					pKind->QueryAttribute("_MaxNum", &rule.maxNum);
					pKind->QueryFloatAttribute("_Radius", &rule.radius);
				}
				else if(EA::StdC::Strcmp(pKind->Name(), "_Default") == 0) {
					rule.kind = Remote::SPAWN_DEFAULT;
					pKind->QueryFloatAttribute("_AngleY", &rule.angleY);
					XMLElement* pRemote = pKind->FirstChildElement("_Remote");
					if(pRemote) pRemote->QueryAttribute("_RemoteIndex", &idx);
				}
				if(idx <= 0 || out.full()) continue;
				rule.idx = RemoteIdx(idx);
				if(rule.maxNum < 1) rule.maxNum = 1;
				out.push_back(rule);
			}
		}
	}
}

static bool XmlAttrTrue(XMLElement* pElt, const char* name)
{
	const char* v = nullptr;
	pElt->QueryStringAttribute(name, &v);
	return v && EA::StdC::Stricmp(v, "true") == 0;
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

		VERBOSE("Remote: { ID=%u, KEYNAME='%s' }", ID, KEYNAME);

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

				remote.boundSize = { (u16)_LengthX, (u16)_LengthY, (u16)_LengthZ };
				pComp->QueryFloatAttribute("_OffsetY", &remote.boundOffsetY);
				pComp->QueryFloatAttribute("_Angle", &remote.angle);
				const char* _AddContactGroup = nullptr;
				pComp->QueryStringAttribute("_AddContactGroup", &_AddContactGroup);
				remote.contactFriend = _AddContactGroup && (EA::StdC::Strcmp(_AddContactGroup, "eFRIEND") == 0 || EA::StdC::Strcmp(_AddContactGroup, "eALL") == 0);
				remote.vsOwner = XmlAttrTrue(pComp, "_VsOwner");
				remote.monsterOnly = XmlAttrTrue(pComp, "_VsNPC_Monster") && !XmlAttrTrue(pComp, "_VsPC");
				remote.ignoreGround = XmlAttrTrue(pComp, "_IgnoreGround");
				pComp->QueryFloatAttribute("_ActivateDelayTime", &remote.activateDelay);
				remote.boundType = Remote::BoundTypeFromString(_Type);
				remote.damageGroup = Remote::DamageGroupFromString(_DamageGroup);
				remote.vs =
					(_VsDynamic << Remote::VS_DYNAMIC) |
					(_VsNPC_Monster << Remote::VS_NPC_MONSTER) |
					(_VsPC << Remote::VS_PLAYER_CHARACTER);

				VERBOSE("	_LengthX=%d _LengthY=%d _LengthZ=%d", _LengthX, _LengthY, _LengthZ);
				VERBOSE("	_DamageGroup=%s _Type=%s _VsX=%#x", Remote::DamageGroupToString(remote.damageGroup), Remote::BoundTypeToString(remote.boundType), remote.vs);
			}

			else if((EA::StdC::Strcmp("RemoteComData2", compName) == 0)) {
				ELT_GET(pComp, i32, _ActivateCount, 0);
				ELT_GET(pComp, i32, _AttackMultiplier, 0);
				ELT_GET(pComp, f32, _LifeTime, 0);
				for(XMLElement* pStatus = pComp->FirstChildElement("_Status"); pStatus; pStatus = pStatus->NextSiblingElement("_Status")) {
					Remote::AppliedStatus st;
					st.statusID = 0;
					st.rate = 100;
					st.value = 100;
					pStatus->QueryAttribute("DATAKEY", &st.statusID);
					pStatus->QueryAttribute("_Rate", &st.rate);
					pStatus->QueryAttribute("_Value", &st.value);
					if(st.statusID != 0 && !remote.statuses.full()) remote.statuses.push_back(st);
				}
				for(XMLElement* pStatus = pComp->FirstChildElement("_StatusFriendly"); pStatus; pStatus = pStatus->NextSiblingElement("_StatusFriendly")) {
					Remote::AppliedStatus st;
					st.statusID = 0;
					st.rate = 100;
					st.value = 100;
					pStatus->QueryAttribute("DATAKEY", &st.statusID);
					pStatus->QueryAttribute("_Rate", &st.rate);
					pStatus->QueryAttribute("_Value", &st.value);
					if(st.statusID != 0 && !remote.friendlyStatuses.full()) remote.friendlyStatuses.push_back(st);
				}
				remote.attachFireObject = XmlAttrTrue(pComp, "_AttachFireObject");
				remote.attachTargetObject = XmlAttrTrue(pComp, "_AttachTargetObject");
				pComp->QueryFloatAttribute("_CriticalRate", &remote.criticalRate);
				pComp->QueryAttribute("_OnHitOwnerStatusIndex", &remote.onHitOwnerStatus);
				remote.activateCount = _ActivateCount;
				remote.lifeTime = _LifeTime;
				pComp->QueryFloatAttribute("_AttackMultiplier", &remote.attackMultiplier);
				pComp->QueryFloatAttribute("_OffsetY", &remote.offsetY);
				if(XMLElement* pFd = pComp->FirstChildElement("_ForceDir")) {
					const char* fdType = nullptr;
					pFd->QueryStringAttribute("_Type", &fdType);
					remote.forceTargetDir = fdType && EA::StdC::Strcmp(fdType, "REMOTE_FORCE_DIRECTION_TYPE_TARGET_DIR") == 0;
				}
				pComp->QueryFloatAttribute("_OffsetZ", &remote.offsetZ);
				pComp->QueryFloatAttribute("_DistPenaltyRate", &remote.distPenaltyRate);
				pComp->QueryFloatAttribute("_PenaltyApplyDistance", &remote.penaltyApplyDistance);
				const char* _OnHitOwnerMemorizeTarget = nullptr;
				pComp->QueryStringAttribute("_OnHitOwnerMemorizeTarget", &_OnHitOwnerMemorizeTarget);
				remote.memorizeTarget = _OnHitOwnerMemorizeTarget && EA::StdC::Stricmp(_OnHitOwnerMemorizeTarget, "true") == 0;

				const char* _BehaviorType = 0;
				pComp->QueryStringAttribute("_BehaviorType", &_BehaviorType);
				if(_BehaviorType) {
					remote.behaviorType = Remote::BehaviourTypeFromString(_BehaviorType);
				}

				VERBOSE("	_ActivateCount=%d _ActivateMultiplier=%d", _ActivateCount, _AttackMultiplier);
				VERBOSE("	_BehaviorType=%s", Remote::BehaviourTypeToString(remote.behaviorType));
			}

			else if((EA::StdC::Strcmp("RemoteTargetComData", compName) == 0)) {
				pComp->QueryFloatAttribute("_HitInvalidTime", &remote.hitInvalidTime);
				pComp->QueryFloatAttribute("_Sight", &remote.sight);
				if(XMLElement* pHoming = pComp->FirstChildElement("_Homing")) {
					pHoming->QueryFloatAttribute("_ReactionLength", &remote.homingLength);
					pHoming->QueryFloatAttribute("_ReactionAngle", &remote.homingAngle);
					pHoming->QueryFloatAttribute("_ReactionTime", &remote.homingTime);
					pHoming->QueryBoolAttribute("_LockOn", &remote.homingLockOn);
				}
			}
			else if((EA::StdC::Strcmp("_Homing", compName) == 0)) {
				pComp->QueryFloatAttribute("_ReactionLength", &remote.homingLength);
				pComp->QueryFloatAttribute("_ReactionAngle", &remote.homingAngle);
				pComp->QueryFloatAttribute("_ReactionTime", &remote.homingTime);
				pComp->QueryBoolAttribute("_LockOn", &remote.homingLockOn);
			}

			else if((EA::StdC::Strcmp("RemoteGraphComData", compName) == 0)) {
				pComp->QueryFloatAttribute("_FixedGraphLength", &remote.fixedGraphLength);
				const char* graphType = nullptr;
				pComp->QueryStringAttribute("_Type", &graphType);
				remote.bezier = graphType && EA::StdC::Strcmp(graphType, "REMOTE_GRAPH_BEZIER_CURVE") == 0;
				i32 nUp = 0, nRot = 0, nFront = 0;
				for(XMLElement* p = pComp->FirstChildElement(); p; p = p->NextSiblingElement()) {
					f32 v = 0;
					p->QueryFloatAttribute("DATA", &v);
					if(EA::StdC::Strcmp(p->Name(), "_BezierUp") == 0 && nUp < 2) remote.bezierUp[nUp++] = v;
					else if(EA::StdC::Strcmp(p->Name(), "_BezierRotate") == 0 && nRot < 2) remote.bezierRotate[nRot++] = v;
					else if(EA::StdC::Strcmp(p->Name(), "_BezierFront") == 0 && nFront < 2) remote.bezierFront[nFront++] = v;
				}
			}

			else if((EA::StdC::Strcmp("SteeringBehaviorsComData", compName) == 0)) {
				ELT_GET(pComp, f32, _MaxSpeed, 0);
				ELT_GET(pComp, f32, _MaxDistance, 0);
				remote.maxSpeed = _MaxSpeed;
				remote.maxDistance = _MaxDistance;
				pComp->QueryFloatAttribute("_AngularVel", &remote.angularVel);
			}

			else if((EA::StdC::Strcmp("RemoteSystemComData", compName) == 0)) {
				XMLElement* pDestroy = pComp->FirstChildElement("_Destroy");
				XMLElement* pCreate = pDestroy ? pDestroy->FirstChildElement("_CreateEntity") : nullptr;
				if(pCreate) {
					i32 _RemoteIndex = -1;
					pCreate->QueryAttribute("_RemoteIndex", &_RemoteIndex);
					remote.destroyCreateRemote = RemoteIdx(_RemoteIndex);
				}
				ParseRemoteSpawnRules(pDestroy, remote.onDestroy);
				XMLElement* pTime = pComp->FirstChildElement("_Time");
				if(pTime) {
					pTime->QueryFloatAttribute("_Time", &remote.timeEventDelay);
					remote.timeEventRecycle = XmlAttrTrue(pTime, "_Recycle");
					ParseRemoteSpawnRules(pTime, remote.onTime);
					if(XMLElement* pCs = pTime->FirstChildElement("_ChangeMaxSpeed")) pCs->QueryFloatAttribute("_Speed", &remote.timeChangeSpeed);
					if(remote.onTime.empty() && remote.timeChangeSpeed <= 0) remote.timeEventDelay = -1;
				}
				ParseRemoteSpawnRules(pComp->FirstChildElement("_Activate"), remote.onActivate);
				if(XMLElement* pAct = pComp->FirstChildElement("_Activate")) {
					if(XMLElement* pCca = pAct->FirstChildElement("_ChangeCasterActionState")) {
						const char* state = nullptr;
						pCca->QueryStringAttribute("_ActionState", &state);
						if(state) remote.changeCasterAction = ActionStateFromString(state);
					}
				}
				ParseRemoteSpawnRules(pComp->FirstChildElement("_ActivateOnNotGround"), remote.onHitCharacter);
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

	r = LoadCharacterBaseStats();
	if(!r) return false;


	r = LoadMasterSkinsDefinitions();
	if(!r) return false;

	r = LoadMasterWeaponDefinitions();
	if(!r) return false;

	r = LoadMasterDefinitionsModel();
	if (!r) return false;

	r = LoadMapList();
	if (!r) return false;

	r = LoadLobby(MapIndex::LOBBY_NORMAL);
	if (!r) return false;

	r = LoadPvpDeathmach();
	if (!r) return false;

	r = LoadJukeboxSongs();
	if(!r) return false;

	r = LoadEntrySystems();
	if(!r) return false;

	r = LoadStageMaps();
	if(!r) return false;

	r = LoadBotCreatures();
	if(!r) return false;


	r = LoadGuildData();
	if(!r) return false;

	r = LoadCollisionMeshes();
	if(!r) return false;

	r = LoadAnimationData();
	if(!r) return false;

	r = LoadRemoteData();
	if(!r) return false;

	r = LoadStatusData();
	if(!r) return false;

	r = LoadSkillTooltips();
	if(!r) return false;

	r = LoadSkillCosts();
	if(!r) return false;

	r = LoadCharacterElements();
	if(!r) return false;

	r = LoadSummons();
	if(!r) return false;

	r = LoadStageTimeLimits();
	if(!r) return false;

	r = LoadAnimationSequenceTimes();
	if(!r) return false;

	r = LoadMoveGraphs();
	if(!r) return false;

	r = LoadSummonActions();
	if(!r) return false;

	r = LoadChildSkills();
	if(!r) return false;

	r = LoadWeaponFiring();
	if(!r) return false;

	r = LoadSkillTargeting();
	if(!r) return false;

	r = LoadInteractions();
	if(!r) return false;

	r = LoadInteractionReceivers();
	if(!r) return false;

	r = LoadBotBrain();
	if(!r) return false;

	r = CheckSkillActions();
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

const GameXmlContent::MapList* GameXmlContent::FindMapListByID(MapIndex index) const
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

i32 GameXmlContent::WeaponTypeOf(ClassType classType, WeaponIndex weaponIndex) const
{
	const Master& master = GetMaster(classType);
	const i32 fam = (i32)weaponIndex / 10;
	auto match = [&](const eastl::fixed_vector<WeaponIndex,3,false>& ids) -> i32 {
		for(int i = 0; i < (int)ids.size(); i++) {
			if(ids[i] == weaponIndex || (i32)ids[i] / 10 == fam) return i + 1;
		}
		return 0;
	};
	i32 t = match(master.fairPvpWeaponIDs);
	if(t != 0) return t;
	t = match(master.defaultWeaponIDs);
	if(t != 0) return t;
	return 1;
}

i8 GameXmlContent::StanceFromString(const char* str)
{
	if(!str || EA::StdC::Strncmp(str, "STANCE_TYPE_", 12) != 0) return 0;
	const char c = str[12];
	return (c >= 'A' && c <= 'Z' && str[13] == 0) ? (i8)(c - 'A' + 1) : 0;
}

static u64 SkillActionKey(ClassType classType, ActionStateID actionID)
{
	return ((u64)(u32)classType << 32) | (u64)(u32)actionID;
}

const GameXmlContent::Action* GameXmlContent::FindSkillAction(ClassType classType, ActionStateID actionID, i8 stance) const
{
	auto found = actionListMap.find(classType);
	if(found == actionListMap.end()) return nullptr;

	if(stance != 0) {
		foreach_const(it, found->second) {
			if(it->ID == actionID && it->stance == stance) return &*it;
		}
	}
	foreach_const(it, found->second) {
		if(it->ID == actionID && it->stance == 0) return &*it;
	}
	return nullptr;
}

const GameXmlContent::Action& GameXmlContent::GetSkillAction(ClassType classType, ActionStateID actionID, i8 stance) const
{
	static const Action noServerAction = {};
	const Action* action = FindSkillAction(classType, actionID, stance);
	if(action) return *action;
	if(skillActionGaps.find(SkillActionKey(classType, actionID)) == skillActionGaps.end()) {
		LOG("ERROR(GetSkillAction): action %d not found for class %d", (i32)actionID, (i32)classType);
		ASSERT_MSG(0, "action not found");
	}
	return noServerAction;
}

bool GameXmlContent::CheckSkillActions()
{
	foreach_const(m, masters) {
		foreach_const(sid, m->skillIDs) {
			auto found = skillMap.find(*sid);
			if(found == skillMap.end()) continue;
			eastl::fixed_vector<ActionStateID,6,false> actions;
			actions.push_back(found->second.action);
			if(const SkillCast* cast = FindSkillCast((i32)*sid)) actions.push_back(cast->action);
			if(const auto* levels = FindSkillCostLevels((i32)*sid)) {
				for(const CostLevel& lvl : *levels) if(!actions.full()) actions.push_back(lvl.action);
			}
			foreach_const(a, actions) {
				if(*a == ActionStateID::INVALID || FindSkillAction(m->classType, *a)) continue;
				if(skillActionGaps.insert(SkillActionKey(m->classType, *a)).second) {
					LOG("GameContent :: class %d skill %d action %d has no ActionBase entry (no server-side commands)", (i32)m->classType, (i32)*sid, (i32)*a);
				}
			}
		}
	}
	return true;
}

StatusType StatusTypeFromString(const char* str)
{
	static const char* names[] = {
		"STATUS_TYPE_ACTIONBLOCK",
		"STATUS_TYPE_ACTIVATE_UNDEAD",
		"STATUS_TYPE_AIRBORNE",
		"STATUS_TYPE_ARTILLERY",
		"STATUS_TYPE_DEFEAT_CASTER_AURA",
		"STATUS_TYPE_DEFENDER",
		"STATUS_TYPE_DISPEL",
		"STATUS_TYPE_DRAGGED",
		"STATUS_TYPE_DRAGGEDBYFORCE",
		"STATUS_TYPE_ELECTRO",
		"STATUS_TYPE_EVADE",
		"STATUS_TYPE_FEAR",
		"STATUS_TYPE_FLY",
		"STATUS_TYPE_FORCEBACK",
		"STATUS_TYPE_GHOSTWIDOW",
		"STATUS_TYPE_GODMODE",
		"STATUS_TYPE_GX09",
		"STATUS_TYPE_HEAL",
		"STATUS_TYPE_HIDE",
		"STATUS_TYPE_HIT_PLAGUE",
		"STATUS_TYPE_HPTRIGGER",
		"STATUS_TYPE_KNOCKDOWN",
		"STATUS_TYPE_KROMEDE",
		"STATUS_TYPE_LINK",
		"STATUS_TYPE_MANABURN",
		"STATUS_TYPE_NEARLY_IMMORTAL",
		"STATUS_TYPE_NOCOST",
		"STATUS_TYPE_PUSH",
		"STATUS_TYPE_ROOT",
		"STATUS_TYPE_SHIELD",
		"STATUS_TYPE_SHIELDBOMB",
		"STATUS_TYPE_SHOCKED",
		"STATUS_TYPE_SKILLSTAMINA",
		"STATUS_TYPE_SKILLSTATUS",
		"STATUS_TYPE_SLOW",
		"STATUS_TYPE_SNIPER",
		"STATUS_TYPE_STACKBOMB",
		"STATUS_TYPE_STAT",
		"STATUS_TYPE_STUN",
		"STATUS_TYPE_SUICIDE",
		"STATUS_TYPE_SUPERARMOR",
		"STATUS_TYPE_TARGET",
		"STATUS_TYPE_UNDEAD",
		"STATUS_TYPE_UNSHAKABLE",
		"STATUS_TYPE_WEAK_ON_CRITICAL",
	};
	if(!str) return StatusType::OTHER;
	for(i32 i = 0; i < (i32)ARRAY_COUNT(names); i++) {
		if(EA::StdC::Strcmp(str, names[i]) == 0) return (StatusType)(i + 1);
	}
	return StatusType::OTHER;
}

bool GameXmlContent::LoadStatusData()
{
	XMLDocument xml;
	if(!LoadXMLFile(L"/client/STATUS_PC.xml", xml)) {
		LOG("WARNING(LoadStatusData): client/STATUS_PC.xml missing, statuses disabled");
		return false;
	}

	for(XMLElement* pInfo = xml.FirstChildElement()->FirstChildElement(); pInfo; pInfo = pInfo->NextSiblingElement()) {
		StatusInfo st;
		pInfo->QueryAttribute("ID", &st.ID);
		XMLElement* pCommon = pInfo->FirstChildElement("ST_COMMONSTATUS");
		const char* statusKey = pInfo->Attribute("KEYNAME");
		st.staminaBurn = statusKey && EA::StdC::Strstr(statusKey, "STAMINABURN") != nullptr;
		const char* statusDesc = pCommon ? pCommon->Attribute("_Description") : nullptr;
		const char* defenceDown = statusKey && statusDesc && EA::StdC::Strstr(statusKey, "DEFENCEDOWN") ? EA::StdC::Strstr(statusDesc, "-") : nullptr;
		if(defenceDown && EA::StdC::Strchr(defenceDown, '%')) st.descDefenceDown = (f32)EA::StdC::AtoI32(defenceDown + 1);
		if(!pCommon) continue;

		f32 durationMs = 0;
		pCommon->QueryFloatAttribute("_DurationTime", &durationMs);
		st.durationSec = durationMs / 1000.0f;
		pCommon->QueryFloatAttribute("_DistanceRatio", &st.distanceRatio);
		pCommon->QueryFloatAttribute("_TimeRatio", &st.timeRatio);
		pCommon->QueryFloatAttribute("_HeightRatio", &st.heightRatio);
		pCommon->QueryFloatAttribute("_LyingTime", &st.lyingTime);
		pCommon->QueryFloatAttribute("_DotBaseDamage", &st.dotDamage);
		pCommon->QueryFloatAttribute("_DotDrainMultiplier", &st.drainMultiplier);
		pCommon->QueryIntAttribute("_SkillIndex", &st.skillIndex);
		f32 intervalMs = 0;
		pCommon->QueryFloatAttribute("_Interval", &intervalMs);
		st.intervalSec = intervalMs / 1000.0f;
		pCommon->QueryFloatAttribute("_Distance", &st.distance);
		pCommon->QueryFloatAttribute("_ParamRatio", &st.paramRatio);
		pCommon->QueryFloatAttribute("_OffsetDraggedDistanceFromTarget", &st.dragOffset);
		pCommon->QueryFloatAttribute("_DraggedByForceMin", &st.dragForceMin);
		pCommon->QueryFloatAttribute("_DraggedByForceMax", &st.dragForceMax);
		pCommon->QueryAttribute("_MaxOverlapCount", &st.maxOverlap);
		pCommon->QueryAttribute("_NextStatusIndex", &st.nextStatus);
		pCommon->QueryFloatAttribute("_TargetingRange", &st.targetingRange);
		if(XMLElement* pDef = pCommon->FirstChildElement("_DEFENDER")) pDef->QueryFloatAttribute("_ExplodeDmg", &st.explodeDamage);
		pCommon->QueryFloatAttribute("_AdditionalDamageByCaster", &st.additionalDamageByCaster);
		{
			const char* eff = nullptr;
			pCommon->QueryStringAttribute("_EffectType", &eff);
			st.isDebuff = eff && EA::StdC::Strcmp(eff, "STATUS_EFFECT_DEBUFF") == 0;
			const char* dis = nullptr;
			pCommon->QueryStringAttribute("_CanDispel", &dis);
			st.canDispel = dis && EA::StdC::Stricmp(dis, "true") == 0;
			const char* dispelType = nullptr;
			pCommon->QueryStringAttribute("_DispelEffectType", &dispelType);
			if(dispelType) st.dispelEffect = EA::StdC::Strcmp(dispelType, "STATUS_EFFECT_DEBUFF") == 0 ? 1 : 0;
			XMLElement* pBlock = pCommon->FirstChildElement("_INPUT_STATE_BLOCK");
			if(pBlock) {
				i32 v = 0;
				v = 0; pBlock->QueryAttribute("_Attack", &v); st.blockAttack = v != 0;
				v = 0; pBlock->QueryAttribute("_Skill", &v); st.blockSkill = v != 0;
				i32 m = 0, s2 = 0;
				pBlock->QueryAttribute("_MovingShirk", &m);
				pBlock->QueryAttribute("_StandingShirk", &s2);
				st.blockShirk = m != 0 || s2 != 0;
			}
			for(XMLElement* pAura = pCommon->FirstChildElement("_AuraStatusIndex"); pAura; pAura = pAura->NextSiblingElement("_AuraStatusIndex")) {
				i32 id = 0;
				pAura->QueryAttribute("DATA", &id);
				if(id == 0) continue;
				if(st.auraStatus == 0) st.auraStatus = id;
				if(!st.auraStatuses.full()) st.auraStatuses.push_back(id);
			}
			st.auraAll = eff && EA::StdC::Strcmp(eff, "STATUS_EFFECT_UNKNOWN") == 0;
			XMLElement* pLink = pCommon->FirstChildElement("_LINK");
			if(!pLink) pLink = pCommon->NextSiblingElement("_LINK");
			if(pLink) {
				pLink->QueryAttribute("_LINK_CASTER", &st.linkCasterStatus);
				pLink->QueryAttribute("_LINK_TARGET", &st.linkTargetStatus);
			}
		}
		{
			const char* cond = nullptr;
			pCommon->QueryStringAttribute("_NextStatusCondition", &cond);
			if(st.nextStatus != 0) {
				st.nextCondition = StatusInfo::NextCondition::ALL;
				if(cond && EA::StdC::Strcmp(cond, "E_NEXT_STATUS_CONDITION_TIMEOVER") == 0) st.nextCondition = StatusInfo::NextCondition::TIMEOVER;
				else if(cond && EA::StdC::Strcmp(cond, "E_NEXT_STATUS_CONDITION_REMOVE_ONHIT") == 0) st.nextCondition = StatusInfo::NextCondition::REMOVE_ONHIT;
			}
		}
		if(st.maxOverlap < 1) st.maxOverlap = 1;
		const char* type = nullptr;
		pCommon->QueryStringAttribute("_Type", &type);
		if(type) {
			st.typeName = type;
			st.type = StatusTypeFromString(type);
		}

		{
			const char* pas = nullptr;
			pCommon->QueryStringAttribute("_Passive", &pas);
			st.permanent = durationMs <= 0 && pas && EA::StdC::Stricmp(pas, "true") == 0;
			pCommon->QueryAttribute("_InitCustomValue", &st.customInit);
			pCommon->QueryAttribute("_MaxCustomValue", &st.customMax);
			pCommon->QueryAttribute("_CasterStatusIndex", &st.casterStatus);
			if(XMLElement* pHit = pCommon->FirstChildElement("_HITEFFECT")) {
				pHit->QueryAttribute("_StatusDocIndex", &st.hitEffectStatus);
				pHit->QueryFloatAttribute("_AddValue", &st.hitEffectValue);
			}
			if(XMLElement* pIce = pCommon->FirstChildElement("_ICEQUEEN")) {
				pIce->QueryAttribute("_Index", &st.stackStatus);
				pIce->QueryAttribute("_FreezeStatusIndex", &st.stackFullStatus);
			}
			if(st.type == StatusType::NOCOST) {
				if(XMLElement* pCost = pCommon->FirstChildElement("_DOUBLEGUN")) {
					pCost->QueryFloatAttribute("_AddRatioCostMP", &st.costRatioMP);
					for(XMLElement* pS = pCost->FirstChildElement("_SkillList"); pS; pS = pS->NextSiblingElement("_SkillList")) {
						i32 v = 0;
						pS->QueryAttribute("DATA", &v);
						if(v && !st.costSkills.full()) st.costSkills.push_back(v);
					}
				}
			}
		}

		switch(st.type) {
			case StatusType::STUN: case StatusType::PUSH: case StatusType::KNOCKDOWN: case StatusType::AIRBORNE: case StatusType::FEAR:
			case StatusType::FORCEBACK: case StatusType::DRAGGED: case StatusType::SHOCKED: case StatusType::ACTIONBLOCK: case StatusType::FLY:
				st.kind = StatusInfo::Kind::HARD_CC;
				break;
			case StatusType::ROOT: st.kind = StatusInfo::Kind::ROOT; break;
			case StatusType::SLOW: st.kind = StatusInfo::Kind::SLOW; break;
			default: break;
		}

		for(XMLElement* pChild = pInfo->FirstChildElement(); pChild; pChild = pChild->NextSiblingElement()) {
			for(XMLElement* pNode = pChild; pNode; pNode = pNode == pChild ? pChild->FirstChildElement() : pNode->NextSiblingElement()) {
				for(const XMLAttribute* a = pNode->FirstAttribute(); a; a = a->Next()) {
					f64 v = 0;
					if(a->QueryDoubleValue(&v) != XML_SUCCESS || st.params.full() || EA::StdC::Strlen(a->Name()) >= 31) continue;
					StatusInfo::NamedValue nv;
					nv.name = a->Name();
					nv.value = v;
					st.params.push_back(nv);
				}
			}
		}
		for(XMLElement* pChild = pInfo->FirstChildElement(); pChild; pChild = pChild->NextSiblingElement()) {
			for(XMLElement* pNode = pChild->FirstChildElement(); pNode; pNode = pNode->NextSiblingElement()) {
				for(XMLElement* pLeaf = pNode->FirstChildElement(); pLeaf; pLeaf = pLeaf->NextSiblingElement()) {
					for(const XMLAttribute* a = pLeaf->FirstAttribute(); a; a = a->Next()) {
						f64 v = 0;
						if(a->QueryDoubleValue(&v) != XML_SUCCESS || st.params.full() || EA::StdC::Strlen(a->Name()) >= 31) continue;
						StatusInfo::NamedValue nv;
						nv.name = a->Name();
						nv.value = v;
						st.params.push_back(nv);
					}
				}
			}
		}

		st.groupTeleportSkill = (i32)st.ParamInt("_MassiveTeleportIndex");
		st.groupTeleportRadius = (f32)st.Param("_MassiveTeleportRadius");
		st.groupTeleportStatus = (i32)st.ParamInt("_MassiveTeleportStatus");
		st.carrier = st.ParamInt("_ShuttleSelfStatusIndex") != 0;
		statusMap[st.ID] = st;
	}

	LOG("Loaded %d statuses", (i32)statusMap.size());







	return true;
}

bool GameXmlContent::LoadSkillTooltips()
{
	XMLDocument xmlTT;
	if(!LoadXMLFile(L"/client/skilltt.xml", xmlTT)) return false;
	i32 modCount = 0;
	eastl::hash_set<i32> touched;
	struct SkillMulStat { i32 skill; i32 status; u8 stat; f32 value; };
	eastl::fixed_vector<SkillMulStat,512,false> skillMulStats;
	for(XMLElement* pMaster = xmlTT.FirstChildElement()->FirstChildElement(); pMaster; pMaster = pMaster->NextSiblingElement()) {
		for(XMLElement* pSkill = pMaster->FirstChildElement("data"); pSkill; pSkill = pSkill->NextSiblingElement("data")) {
			XMLElement* pBase = pSkill->FirstChildElement("data");
			if(!pBase) continue;
			i32 skillID = 0;
			pSkill->QueryAttribute("DATAKEY", &skillID);
			for(XMLElement* pT = pBase->FirstChildElement("tableDatas"); pT; pT = pT->NextSiblingElement("tableDatas")) {
				const char* attrType = nullptr;
				pT->QueryStringAttribute("attrType", &attrType);
				if(!attrType || EA::StdC::Strcmp(attrType, "TTATTR_REMOTE_DAMAGE") != 0) continue;
				f32 dmg = 0;
				pT->QueryFloatAttribute("value_1", &dmg);
				if(dmg > 0 && skillBaseDamage.find(skillID) == skillBaseDamage.end()) skillBaseDamage[skillID] = dmg;
			}
			eastl::fixed_vector<eastl::pair<eastl::fixed_string<char,40,false>, i32>,16,false> keyStatus;
			i32 delegateID = 0;
			for(XMLElement* pAttr = pBase->FirstChildElement("attrMap"); pAttr; pAttr = pAttr->NextSiblingElement("attrMap")) {
				const char* key = nullptr;
				i32 data = 0;
				pAttr->QueryStringAttribute("DATAKEY", &key);
				pAttr->QueryAttribute("DATA", &data);
				if(!key) continue;
				if(EA::StdC::Strcmp(key, "TTATTR_STATUS_DELIGATE") == 0) delegateID = data;
				if(!keyStatus.full()) keyStatus.push_back(eastl::make_pair(eastl::fixed_string<char,40,false>(key), data));
			}
			for(XMLElement* pT = pBase->FirstChildElement("tableDatas"); pT; pT = pT->NextSiblingElement("tableDatas")) {
				const char* attrType = nullptr; const char* attrKey = nullptr; const char* proc = nullptr; const char* apply = nullptr; const char* stat = nullptr;
				pT->QueryStringAttribute("attrType", &attrType);
				if(!attrType || EA::StdC::Strcmp(attrType, "TTATTR_STATUS_STAT") != 0) continue;
				pT->QueryStringAttribute("attrKey", &attrKey);
				i32 statusID = delegateID;
				if(attrKey) {
					foreach_const(k, keyStatus) {
						if(k->first == attrKey) statusID = k->second;
					}
				}
				auto found = statusMap.find(statusID);
				if(found == statusMap.end() || found->second.modsFromTT) continue;
				pT->QueryStringAttribute("proc", &proc);
				pT->QueryStringAttribute("apply", &apply);
				pT->QueryStringAttribute("stat", &stat);
				StatusMod mod;
				pT->QueryFloatAttribute("value_1", &mod.value);
				pT->QueryFloatAttribute("value_2", &mod.count);
				if(mod.value == 0 || !stat) continue;
				mod.regen = proc && EA::StdC::Strcmp(proc, "STATPROC_REGEN") == 0;
				mod.mul = apply && EA::StdC::Strcmp(apply, "STATAPPLY_MUL") == 0;
				if(EA::StdC::Strcmp(stat, "STAT_TYPE_SHIELD_DEFENCE") == 0) mod.stat = StatusMod::SHIELD_DEFENCE;
				else if(EA::StdC::Strcmp(stat, "STAT_TYPE_HEALTH") == 0) mod.stat = StatusMod::HEALTH;
				else if(EA::StdC::Strcmp(stat, "STAT_TYPE_SKILLPOWER") == 0) mod.stat = StatusMod::SKILLPOWER;
				else if(EA::StdC::Strcmp(stat, "STAT_TYPE_CRITICALRATE") == 0) mod.stat = StatusMod::CRITICALRATE;
				else if(EA::StdC::Strcmp(stat, "STAT_TYPE_REFLECT_DAMAGE_RATIO") == 0) mod.stat = StatusMod::REFLECT_DAMAGE_RATIO;
				else if(EA::StdC::Strcmp(stat, "STAT_TYPE_SKILL_CRITICALRATE") == 0) mod.stat = StatusMod::SKILL_CRITICALRATE;
				else if(EA::StdC::Strcmp(stat, "STAT_TYPE_ATTACK") == 0) mod.stat = StatusMod::ATTACK;
				else if(EA::StdC::Strcmp(stat, "STAT_TYPE_MOVESPEED") == 0) mod.stat = StatusMod::MOVESPEED;
				else if(EA::StdC::Strcmp(stat, "STAT_TYPE_DEFENCE") == 0) mod.stat = StatusMod::DEFENCE;
				if(mod.mul && mod.stat != StatusMod::OTHER && !skillMulStats.full()) skillMulStats.push_back({ skillID, statusID, (u8)mod.stat, mod.value });
				if(!found->second.mods.full()) { found->second.mods.push_back(mod); modCount++; }
				touched.insert(statusID);
			}
		}
	}
	foreach(t, touched) {
		auto f = statusMap.find(*t);
		if(f != statusMap.end()) f->second.modsFromTT = true;
	}
	foreach(it, statusMap) {
		StatusInfo& st = it->second;
		if(st.descDefenceDown <= 0 || !st.mods.empty() || st.type != StatusType::STAT) continue;
		StatusMod mod;
		mod.stat = StatusMod::DEFENCE;
		mod.mul = true;
		mod.value = -st.descDefenceDown;
		mod.count = 1;
		st.mods.push_back(mod);
		modCount++;
	}
	LOG("Loaded %d status stat modifiers from skilltt", modCount);

	XMLDocument xmlDesc;
	if(LoadXMLFile(L"/client/string/skilldesc.xml", xmlDesc)) {
		i32 added = 0;
		XMLElement* pTable = xmlDesc.FirstChildElement()->FirstChildElement("StringTableData");
		for(XMLElement* pS = pTable ? pTable->FirstChildElement("StringData") : nullptr; pS; pS = pS->NextSiblingElement("StringData")) {
			const char* key = nullptr; const char* text = nullptr;
			pS->QueryStringAttribute("DATAKEY", &key);
			pS->QueryStringAttribute("data", &text);
			if(!key || !text || EA::StdC::Strncmp(key, "STR_SKILL_DESC_", 15) != 0) continue;
			if(EA::StdC::Stristr(text, "can be struck multiple times") || EA::StdC::Stristr(text, "can hit the same target") || EA::StdC::Stristr(text, "strike the same target")) repeatHitReduced.insert(EA::StdC::AtoI32(key + 15));
			if(!EA::StdC::Stristr(text, "movespeed") && !EA::StdC::Stristr(text, "movement speed")) continue;
			const char* ph = EA::StdC::Strstr(text, ":status:stat:mul:");
			if(!ph) continue;
			const char* statName = ph + 17;
			StatusMod::Stat stat = StatusMod::OTHER;
			if(EA::StdC::Strnicmp(statName, "attack:", 7) == 0) stat = StatusMod::ATTACK;
			if(stat == StatusMod::OTHER) continue;
			const i32 skillID = EA::StdC::AtoI32(key + 15);
			foreach_const(sm, skillMulStats) {
				if(sm->skill != skillID || sm->stat != (u8)stat) continue;
				auto found = statusMap.find(sm->status);
				if(found == statusMap.end()) continue;
				bool hasMove = false;
				foreach_const(m, found->second.mods) hasMove |= m->stat == StatusMod::MOVESPEED;
				if(hasMove || found->second.mods.full()) continue;
				StatusMod mod;
				mod.stat = StatusMod::MOVESPEED;
				mod.mul = true;
				mod.value = sm->value;
				mod.count = 1;
				found->second.mods.push_back(mod);
				added++;
			}
		}
		LOG("Loaded %d movespeed modifiers from skill descriptions", added);
	}
	return true;
}

bool GameXmlContent::LoadSkillCosts()
{
	XMLDocument xmlSkill;
	if(!LoadXMLFile(L"/SKILL.xml", xmlSkill)) return false;
	for(XMLElement* pInfo = xmlSkill.FirstChildElement()->FirstChildElement(); pInfo; pInfo = pInfo->NextSiblingElement()) {
		i32 skillID = 0;
		pInfo->QueryAttribute("ID", &skillID);
		XMLElement* pCommon = pInfo->FirstChildElement("ST_COMMONSKILL");
		if(pCommon) {
			f32 stamina = 0;
			if(pCommon->QueryFloatAttribute("_ConsumeStamina", &stamina) == XML_SUCCESS && stamina > 0) skillConsumeStamina[skillID] = stamina;
			f32 mp = 0, ug = 0;
			if(pCommon->QueryFloatAttribute("_ConsumeMP", &mp) == XML_SUCCESS && mp > 0) skillConsumeMP[skillID] = mp;
			if(pCommon->QueryFloatAttribute("_ConsumeUG", &ug) == XML_SUCCESS && ug > 0) skillConsumeUG[skillID] = ug;
			eastl::array<f32,4> res = { 0, 0, 0, 0 };
			pCommon->QueryFloatAttribute("_ConsumeEP", &res[0]);
			pCommon->QueryFloatAttribute("_ConsumeRP", &res[1]);
			pCommon->QueryFloatAttribute("_ConsumeBB", &res[2]);
			pCommon->QueryFloatAttribute("_ConsumeHP", &res[3]);
			if(res[0] > 0 || res[1] > 0 || res[2] > 0 || res[3] > 0) skillConsumeRes[skillID] = res;
			if(const i32 comboBase = pCommon->IntAttribute("_BaseComboSkill")) comboBaseSkill[skillID] = comboBase;
			if(XMLElement* pCombo = pCommon->FirstChildElement("_SKILLCOMBO")) {
				XMLElement* pComboTarget = pCommon->FirstChildElement("_Target");
				const char* selectState = pComboTarget ? pComboTarget->Attribute("_SelectStateType") : nullptr;
				const bool afterCast = selectState && EA::StdC::Strcmp(selectState, "SKILL_TARGET_SELECT_STATE_AFTERCAST") == 0;
				if(afterCast && pCombo->BoolAttribute("_AutoCasting")) comboAutoCastMs[skillID] = MAX(pCombo->IntAttribute("_ExecuteTimeMS"), 1);
				comboDurationMs[skillID] = pCombo->IntAttribute("_DurationTimeMS");
			}
			if(XMLElement* pCost = pCommon->FirstChildElement("_COST_PROPERTY")) {
				for(XMLElement* pLvl = pCost->FirstChildElement("_PROPERTY_LEVEL"); pLvl; pLvl = pLvl->NextSiblingElement("_PROPERTY_LEVEL")) {
					CostLevel lvl;
					pLvl->QueryFloatAttribute("_ConsumeEP", &lvl.consume[0]);
					pLvl->QueryFloatAttribute("_ConsumeRP", &lvl.consume[1]);
					pLvl->QueryFloatAttribute("_ConsumeBB", &lvl.consume[2]);
					if(const char* act = pLvl->Attribute("_Action")) lvl.action = ActionStateFromString(act);
					for(XMLElement* pR = pLvl->FirstChildElement("_REMOTELIST"); pR; pR = pR->NextSiblingElement("_REMOTELIST")) {
						i32 from = 0, to = 0;
						pR->QueryAttribute("_RemoteIndex", &from);
						pR->QueryAttribute("_ReplaceIndex", &to);
						if(from != 0 && to != 0 && !lvl.remoteReplace.full()) lvl.remoteReplace.push_back({ from, to });
					}
					for(XMLElement* pS = pLvl->FirstChildElement("_STATUSLIST"); pS; pS = pS->NextSiblingElement("_STATUSLIST")) {
						i32 from = 0, to = 0;
						pS->QueryAttribute("_StatusIndex", &from);
						pS->QueryAttribute("_ReplaceIndex", &to);
						if(from != 0 && to != 0 && !lvl.statusReplace.full()) lvl.statusReplace.push_back({ from, to });
					}
					for(XMLElement* pP = xmlSKILLPROPERTY.FirstChildElement()->FirstChildElement(); pP; pP = pP->NextSiblingElement()) {
						XMLElement* pSP = pP->FirstChildElement("ST_SKILL_PROPERTY");
						const char* spType = pSP ? pSP->Attribute("_Type") : nullptr;
						if(!spType || EA::StdC::Strcmp(spType, "E_SKILL_PROPERTY_DAMAGE") != 0 || pSP->IntAttribute("_SkillIndex") != skillID) continue;
						XMLElement* pNormal = pSP->FirstChildElement("_PROPERTY_LEVEL");
						XMLElement* pCostLevel = pSP->FirstChildElement("_COST_LEVEL");
						for(u32 k = skillCostLevels[skillID].size(); pCostLevel && k > 0; k--) pCostLevel = pCostLevel->NextSiblingElement("_COST_LEVEL");
						XMLElement* pEmpowered = pCostLevel ? pCostLevel->FirstChildElement("_PROPERTY_LEVEL") : nullptr;
						if(!pNormal || !pEmpowered) continue;
						const f32 normalBase = pNormal->FloatAttribute("_BaseDamage");
						const f32 empoweredBase = pEmpowered->FloatAttribute("_BaseDamage");
						if(normalBase > 0 && empoweredBase > 0) lvl.damageRatio = empoweredBase / normalBase;
						break;
					}
					auto& levels = skillCostLevels[skillID];
					if(!levels.full()) levels.push_back(lvl);
				}
			}
		}
		XMLElement* pEvent = pCommon ? pCommon->FirstChildElement("_HitEvent") : nullptr;
		if(!pEvent) continue;
		const char* type = nullptr;
		pEvent->QueryStringAttribute("_HIT_EVENT_TYPE", &type);
		i32 remote = 0;
		pEvent->QueryAttribute("_HIT_EVENT_CONDITION", &remote);
		if(type && EA::StdC::Strstr(type, "ENABLE_NEXT_COMBO") && remote != 0) {
			skillHitEventRemote[skillID] = remote;
		}
	}
	LOG("Loaded %d skill hit events", (i32)skillHitEventRemote.size());
	return true;
}

bool GameXmlContent::LoadCharacterElements()
{
	XMLDocument xmlChara;
	if(!LoadXMLFile(L"/client/CREATURE_CHARACTER.xml", xmlChara)) return false;
	for(XMLElement* pInfo = xmlChara.FirstChildElement()->FirstChildElement(); pInfo; pInfo = pInfo->NextSiblingElement()) {
		i32 id = 0;
		pInfo->QueryAttribute("ID", &id);
		for(XMLElement* pComp = pInfo->FirstChildElement(); pComp; pComp = pComp->NextSiblingElement()) {
			const char* el = nullptr;
			if(pComp->QueryStringAttribute("_ElementType", &el) != XML_SUCCESS || !el) continue;
			i32 e = 0;
			if(EA::StdC::Strcmp(el, "CELEMENT_TYPE_RED") == 0) e = 1;
			else if(EA::StdC::Strcmp(el, "CELEMENT_TYPE_GREEN") == 0) e = 2;
			else if(EA::StdC::Strcmp(el, "CELEMENT_TYPE_BLUE") == 0) e = 3;
			creatureElement[id] = e;
			break;
		}
	}
	LOG("Loaded %d character elements", (i32)creatureElement.size());
	return true;
}

bool GameXmlContent::LoadSummons()
{
	XMLDocument xmlSummon;
	if(!LoadXMLFile(L"/client/CREATURE_MONSTER_SUMMON.xml", xmlSummon)) return false;
	for(XMLElement* pInfo = xmlSummon.FirstChildElement()->FirstChildElement(); pInfo; pInfo = pInfo->NextSiblingElement()) {
		i32 id = 0;
		pInfo->QueryAttribute("ID", &id);
		SummonInfo si;
		if(XMLElement* p = pInfo->FirstChildElement("EntityComData")) {
			p->QueryFloatAttribute("_MoveSpeed", &si.moveSpeed);
			const char* t = nullptr;
			p->QueryStringAttribute("_IsTargetable", &t);
			si.targetable = t && EA::StdC::Stricmp(t, "true") == 0;
		}
		if(XMLElement* p = pInfo->FirstChildElement("LifeTime_ComData")) p->QueryFloatAttribute("LifeTime", &si.lifeTime);
		if(XMLElement* p = pInfo->FirstChildElement("Kfm_ComData")) {
			const char* key = nullptr;
			p->QueryStringAttribute("KeyName", &key);
			if(key) si.className = key;
		}
		if(XMLElement* p = pInfo->FirstChildElement("Ai_ComData")) {
			p->QueryFloatAttribute("_PerceptionLength", &si.perception);
			const char* ai = nullptr;
			p->QueryStringAttribute("_AiIndex", &ai);
			if(ai && EA::StdC::Strcmp(ai, "AI_PC_MONDOZAX_ROBOT") == 0) si.ai = SummonInfo::AI_RELAY;
			else if(ai && (EA::StdC::Strcmp(ai, "AI_PC_ARTILLERY_SPIDERMINE") == 0 || EA::StdC::Strcmp(ai, "AI_PC_SOULMASTER_SLOWSOUL") == 0 ||
				EA::StdC::Strcmp(ai, "AI_PC_ELECTRO_ENERGYBALL") == 0)) si.ai = SummonInfo::AI_CHASE;
		}
		if(XMLElement* p = pInfo->FirstChildElement("MoveController_ComData")) p->QueryFloatAttribute("_AIDistanceWidth", &si.aiDistance);
		if(XMLElement* p = pInfo->FirstChildElement("StatsComData")) p->QueryAttribute("_VariationType", &si.variation);
		if(XMLElement* p = pInfo->FirstChildElement("SkillComData")) {
			for(XMLElement* s = p->FirstChildElement("_SkillID"); s; s = s->NextSiblingElement("_SkillID")) {
				i32 sk = 0;
				s->QueryAttribute("_Index", &sk);
				if(sk != 0 && !si.skills.full()) si.skills.push_back(sk);
			}
		}
		summonMap[id] = si;
	}
	LOG("Loaded %d summon creatures", (i32)summonMap.size());
	return true;
}

bool GameXmlContent::LoadStageTimeLimits()
{
	XMLDocument xmlStage;
	if(!LoadXMLFile(L"/client/STAGELIST.xml", xmlStage)) return false;
	for(XMLElement* pInfo = xmlStage.FirstChildElement()->FirstChildElement(); pInfo; pInfo = pInfo->NextSiblingElement()) {
		i32 id = 0;
		pInfo->QueryAttribute("ID", &id);
		XMLElement* pStage = pInfo->FirstChildElement();
		XMLElement* pList = pStage ? pStage->FirstChildElement("_SUCCESS_CONDITION_LIST") : nullptr;
		XMLElement* pSet = pList ? pList->FirstChildElement("_MCASESET") : nullptr;
		for(XMLElement* pC = pSet ? pSet->FirstChildElement("_MCONDITION") : nullptr; pC; pC = pC->NextSiblingElement("_MCONDITION")) {
			const char* type = nullptr;
			pC->QueryStringAttribute("_Type", &type);
			if(!type || EA::StdC::Strcmp(type, "E_CONDITION_TYPE_TIME_COUNT") != 0) continue;
			f32 v = 0;
			pC->QueryFloatAttribute("_Value", &v);
			if(v > 0) stageTimeLimit[id] = v;
		}
	}
	LOG("Loaded %d stage time limits", (i32)stageTimeLimit.size());
	return true;
}

bool GameXmlContent::LoadAnimationSequenceTimes()
{
	XMLDocument xmlAni;
	if(!LoadXMLFile(L"/client/AniLength.xml", xmlAni)) return false;
	for(XMLElement* pType = xmlAni.FirstChildElement()->FirstChildElement("EntityType"); pType; pType = pType->NextSiblingElement("EntityType")) {
		const char* t = nullptr;
		pType->QueryStringAttribute("_Type", &t);
		if(!t) continue;
		const bool pcType = EA::StdC::Strcmp(t, "PC") == 0;
		for(XMLElement* pEnt = pType->FirstChildElement("Entity"); pEnt; pEnt = pEnt->NextSiblingElement("Entity")) {
			const char* key = nullptr;
			pEnt->QueryStringAttribute("_Key", &key);
			if(!key) continue;
			for(XMLElement* pAni = pEnt->FirstChildElement("AnimationInfo"); pAni; pAni = pAni->NextSiblingElement("AnimationInfo")) {
				i32 seqID = 0;
				f32 seqTime = 0;
				pAni->QueryAttribute("_SeqID", &seqID);
				pAni->QueryFloatAttribute("_SeqTime", &seqTime);
				if(seqID != 0 && (pcType || aniSeqTime.find(AniKey(key, seqID)) == aniSeqTime.end())) aniSeqTime[AniKey(key, seqID)] = seqTime;
				if(!pcType) continue;
				const char* name = nullptr;
				pAni->QueryStringAttribute("_AnimationName", &name);
				if(!name || EA::StdC::Strcmp(name, "LIE_END") != 0) continue;
				f32 len = 0;
				pAni->QueryFloatAttribute("_SeqTime", &len);
				lieEndTime.push_back(eastl::make_pair(eastl::fixed_string<char,48,false>(key), len));
			}
		}
	}
	LOG("Loaded %d stand-up animation lengths", (i32)lieEndTime.size());
	return true;
}

bool GameXmlContent::LoadMoveGraphs()
{
	XMLDocument xmlMoveGraph;
	if(!LoadXMLFile(L"/MoveGraph.xml", xmlMoveGraph)) return false;
	for(XMLElement* pClass = xmlMoveGraph.FirstChildElement()->FirstChildElement("Class"); pClass; pClass = pClass->NextSiblingElement("Class")) {
		const char* type = nullptr;
		pClass->QueryStringAttribute("Type", &type);
		if(!type) continue;
		for(XMLElement* pAnim = pClass->FirstChildElement("Anim"); pAnim; pAnim = pAnim->NextSiblingElement("Anim")) {
			i32 id = 0;
			const char* h = nullptr;
			MoveCurve curve;
			pAnim->QueryAttribute("Id", &id);
			pAnim->QueryFloatAttribute("Time", &curve.time);
			pAnim->QueryStringAttribute("H", &h);
			for(const char* c = h; c && *c && !curve.h.full(); ) {
				char* end = nullptr;
				const f32 v = EA::StdC::StrtoF32(c, &end);
				if(end == c) break;
				curve.h.push_back(v);
				curve.maxAbs = MAX(curve.maxAbs, fabsf(v));
				c = end;
			}
			if(id != 0 && !curve.h.empty()) moveCurves[AniKey(type, id)] = curve;
		}
	}
	LOG("Loaded %d move graphs", (i32)moveCurves.size());
	return true;
}

bool GameXmlContent::LoadSummonActions()
{
	XMLDocument xmlMobActions;
	if(!LoadXMLFile(L"/client/ActionBase_Monster_MOB_PvP.xml", xmlMobActions)) return false;
	i32 count = 0;
	for(XMLElement* pA = xmlMobActions.FirstChildElement()->FirstChildElement(); pA; pA = pA->NextSiblingElement()) {
		const char* cls = nullptr; const char* state = nullptr; const char* type = nullptr;
		pA->QueryStringAttribute("Class", &cls);
		pA->QueryStringAttribute("BehaviorState", &state);
		pA->QueryStringAttribute("CommandType", &type);
		if(!cls || !state || !type) continue;
		const bool spawn = EA::StdC::Strcmp(state, "SPAWN_BEHAVIORSTATE") == 0;
		const bool die = EA::StdC::Strcmp(state, "DIE_BEHAVIORSTATE") == 0;
		i32 skillState = 0;
		if(!spawn && !die && EA::StdC::Sscanf(state, "SKILL_%d_BEHAVIORSTATE", &skillState) != 1) continue;
		if(skillState < 0 || skillState > 20) continue;
		i32 stateAnimId = 0;
		pA->QueryAttribute("AnimationId", &stateAnimId);
		if(!spawn && !die && stateAnimId != 0) {
			foreach(it, summonMap) if(it->second.className == cls && it->second.stateAnim[skillState] == 0) it->second.stateAnim[skillState] = stateAnimId;
		}
		const char* p2 = nullptr;
		pA->QueryStringAttribute("Param2", &p2);
		if(p2 && EA::StdC::Strcmp(p2, "Remove") == 0) continue;
		SummonCommand cmd;
		if(EA::StdC::Strcmp(type, "REMOTE") == 0) { cmd.kind = SummonCommand::REMOTE; pA->QueryAttribute("RemoteIndex", &cmd.index); }
		else if(EA::StdC::Strcmp(type, "STATUS") == 0) { cmd.kind = SummonCommand::STATUS; pA->QueryAttribute("StatusIndex", &cmd.index); }
		else if(EA::StdC::Strcmp(type, "DIE_MODE") == 0) {
			cmd.kind = SummonCommand::DIE;
			const char* p1 = nullptr;
			pA->QueryStringAttribute("Param1", &p1);
			i32 mode = 0;
			if(p1 && EA::StdC::Sscanf(p1, "CUSTOM_DIE_MODE_%d", &mode) == 1) cmd.dieMode = (u8)mode;
		}
		else continue;
		if(cmd.kind != SummonCommand::DIE && cmd.index == 0) continue;
		pA->QueryFloatAttribute("Delay", &cmd.delay);
		const char* preset = nullptr;
		pA->QueryStringAttribute("TargetPreset", &preset);
		cmd.onCurrent = preset && EA::StdC::Strstr(preset, "CURRENT");
		const char* var = nullptr;
		pA->QueryStringAttribute("VariationType", &var);
		foreach(it, summonMap) {
			if(it->second.className != cls) continue;
			i32 dieMode = 0;
			if(var && die && EA::StdC::Sscanf(var, "CUSTOM_DIE_MODE_%d", &dieMode) == 1) {
				cmd.dieMode = (u8)dieMode;
			}
			else if(var) {
				i32 v = -1;
				if(EA::StdC::Sscanf(var, "STATS_VARIATION_TYPE_%d", &v) == 1) { if(v != it->second.variation) continue; }
				else if(EA::StdC::Sscanf(var, "SPAWN_VARIATION_TYPE_%d", &v) == 1) { if(v != 0) continue; }
				else if(EA::StdC::Strcmp(var, "STANCE_TYPE_DEFAULT") != 0) continue;
			}
			auto& list = spawn ? it->second.onSpawn : (die ? it->second.onDie : it->second.skillStates[skillState]);
			if(!list.full()) { list.push_back(cmd); count++; }
		}
	}
	LOG("Loaded %d summon spawn/die/skill commands", count);
	return true;
}

bool GameXmlContent::LoadChildSkills()
{
	XMLDocument xmlChild;
	if(!LoadXMLFile(L"/ActionBase.xml", xmlChild)) return false;
	eastl::hash_set<u32> spawningStates;
	for(XMLElement* pA = xmlChild.FirstChildElement()->FirstChildElement(); pA; pA = pA->NextSiblingElement()) {
		const char* type = nullptr;
		pA->QueryStringAttribute("CommandType", &type);
		if(type && EA::StdC::Strcmp(type, "ENTITY") == 0) {
			const char* cls = nullptr; const char* state = nullptr;
			pA->QueryStringAttribute("Class", &cls);
			pA->QueryStringAttribute("BehaviorState", &state);
			if(!cls || !state) continue;
			eastl::fixed_string<char,64,false> c = cls;
			c.make_upper();
			spawningStates.insert(((u32)ClassTypeFromString(c.data()) << 16) | ((u32)ActionStateFromString(state) & 0xFFFF));
			continue;
		}
		if(!type || EA::StdC::Strcmp(type, "CALL_CHILD_SKILL") != 0) continue;
		const char* cls = nullptr; const char* state = nullptr; const char* p1 = nullptr;
		pA->QueryStringAttribute("Class", &cls);
		pA->QueryStringAttribute("BehaviorState", &state);
		pA->QueryStringAttribute("Param1", &p1);
		i32 n = 0;
		if(!cls || !state || !p1 || EA::StdC::Sscanf(p1, "MONSTER_SKILL_ID_%d", &n) != 1) continue;
		eastl::fixed_string<char,64,false> c = cls;
		c.make_upper();
		const ClassType ct = ClassTypeFromString(c.data());
		const ActionStateID as = ActionStateFromString(state);
		childSkill[((u32)ct << 16) | ((u32)as & 0xFFFF)] = n;
	}
	foreach_const(cs, childSkill) if(spawningStates.find(cs->first) == spawningStates.end()) childOnExisting.insert(cs->first);
	LOG("Loaded %d child skill calls (%d to an existing creature)", (i32)childSkill.size(), (i32)childOnExisting.size());
	return true;
}

bool GameXmlContent::LoadWeaponFiring()
{
	XMLDocument xmlFiring, xmlWeapon;
	if(!LoadXMLFile(L"/client/FIRING_METHOD.xml", xmlFiring)) return false;
	if(!LoadXMLFile(L"/client/WEAPON.xml", xmlWeapon)) return false;
	eastl::hash_map<i32, WeaponHeatInfo> byFiring;
	eastl::hash_map<i32, f32> delayByFiring;
	eastl::hash_map<i32, WeaponFiring> firingRemotes;
	for(XMLElement* pInfo = xmlFiring.FirstChildElement()->FirstChildElement(); pInfo; pInfo = pInfo->NextSiblingElement()) {
		i32 id = 0;
		pInfo->QueryAttribute("ID", &id);
		XMLElement* pData = pInfo->FirstChildElement("FiringMethodData");
		XMLElement* pFire = pData ? pData->FirstChildElement("_FIREINFO") : nullptr;
		const char* fireDummy = pData ? pData->Attribute("_FiringDummy") : nullptr;
		f32 fireMaxDistance = 0;
		if(pFire) pFire->QueryFloatAttribute("_MaxDistance", &fireMaxDistance);
		f32 fireMinDistance = 0;
		if(pFire) pFire->QueryFloatAttribute("_MinDistance", &fireMinDistance);
		if(pFire) {
			f32 delay = 0;
			pFire->QueryFloatAttribute("_FireDelayTime", &delay);
			if(delay > 0) delayByFiring[id] = delay;
		}
		{
			WeaponFiring wf;
			if(fireDummy) wf.fireDummy = fireDummy;
			wf.spawnAtAim = pData && XmlAttrTrue(pData, "_RemoteSpawnAimPosition");
			if(XMLElement* pCharge = pData ? pData->FirstChildElement("_CHARGE") : nullptr) {
				for(XMLElement* pLevel = pCharge->FirstChildElement("_LEVEL"); pLevel && !wf.chargeTime.full(); pLevel = pLevel->NextSiblingElement("_LEVEL")) wf.chargeTime.push_back(pLevel->FloatAttribute("_ChargingTime"));
			}
			XMLElement* pFactory = pInfo->FirstChildElement("RemoteFactoryComData");
			XMLElement* pCreate = pFactory ? pFactory->FirstChildElement("_Create") : nullptr;
			XMLElement* pDef = pCreate ? pCreate->FirstChildElement("_Default") : nullptr;
			XMLElement* pRem = pDef ? pDef->FirstChildElement("_Remote") : nullptr;
			if(pRem) {
				i32 v = 0;
				pRem->QueryAttribute("_RemoteIndex", &v);
				wf.base = (RemoteIdx)v;
				for(XMLElement* pE = pRem->FirstChildElement(); pE; pE = pE->NextSiblingElement()) {
					i32 idx = 0;
					if(EA::StdC::Strcmp(pE->Name(), "_COMBO") == 0) {
						pE->QueryAttribute("_Index", &idx);
						if(idx && !wf.combo.full()) wf.combo.push_back((RemoteIdx)idx);
					}
					else if(EA::StdC::Strcmp(pE->Name(), "_CHARGINGREMOTE") == 0) {
						pE->QueryAttribute("_Index", &idx);
						if(idx && !wf.charge.full()) wf.charge.push_back((RemoteIdx)idx);
						if(const char* dummy = pE->Attribute("_FiringDummy")) wf.chargeDummy = dummy;
					}
					else if(EA::StdC::Strcmp(pE->Name(), "_FIRINGCOUNT") == 0) {
						pE->QueryAttribute("_Rate", &wf.countRate);
						pE->QueryAttribute("_Index", &idx);
						wf.countRemote = (RemoteIdx)idx;
					}
					else if(EA::StdC::Strcmp(pE->Name(), "_STATUS") == 0) {
						pE->QueryAttribute("_OwnerStatusToCheck", &wf.replaceStatus);
						pE->QueryAttribute("_RemoteIndexToReplace", &idx);
						wf.replaceRemote = (RemoteIdx)idx;
					}
				}
			}
			else if(XMLElement* pRad = pCreate ? pCreate->FirstChildElement("_Radiate") : nullptr) {
				i32 v = 0;
				pRad->QueryAttribute("_Index", &v);
				wf.base = (RemoteIdx)v;
			}
			wf.maxDistance = fireMaxDistance;
			wf.minDistance = fireMinDistance;
			if(wf.base != RemoteIdx::INVALID) firingRemotes[id] = wf;
		}
		XMLElement* pGauge = pData ? pData->FirstChildElement("_GAUGE") : nullptr;
		if(!pGauge) continue;
		const char* type = nullptr;
		pGauge->QueryStringAttribute("_Type", &type);
		if(!type || EA::StdC::Strcmp(type, "WEAPON_GAUGE_TYPE_INCREASE") != 0) continue;
		WeaponHeatInfo h;
		pGauge->QueryFloatAttribute("_MaxGauge", &h.maxGauge);
		pGauge->QueryFloatAttribute("_Consumption", &h.consumption);
		pGauge->QueryFloatAttribute("_AutoRegen", &h.autoRegen);
		pGauge->QueryFloatAttribute("_AutoRegenInterval", &h.autoRegenInterval);
		pGauge->QueryFloatAttribute("_AutoRegenDelayTime", &h.autoRegenDelay);
		pGauge->QueryFloatAttribute("_OverHeatDurationTime", &h.overheatDuration);
		pGauge->QueryFloatAttribute("_OverHeatResetGauge", &h.overheatResetGauge);
		if(h.maxGauge > 0 && h.consumption > 0) byFiring[id] = h;
	}
	for(XMLElement* pInfo = xmlWeapon.FirstChildElement()->FirstChildElement(); pInfo; pInfo = pInfo->NextSiblingElement()) {
		i32 id = 0;
		pInfo->QueryAttribute("ID", &id);
		for(XMLElement* pComp = pInfo->FirstChildElement(); pComp; pComp = pComp->NextSiblingElement()) {
			i32 firing = 0;
			if(pComp->QueryAttribute("_DefaultFiringIndex", &firing) != XML_SUCCESS) continue;
			f32 pvpAttack = 0;
			if(pComp->QueryFloatAttribute("_PvPAttack", &pvpAttack) == XML_SUCCESS && pvpAttack > 0) weaponAttack[id] = pvpAttack;
			auto f = byFiring.find(firing);
			if(f != byFiring.end()) weaponHeat[id] = f->second;
			auto d = delayByFiring.find(firing);
			if(d != delayByFiring.end()) weaponFireDelay[id] = d->second;
			auto r = firingRemotes.find(firing);
			if(r != firingRemotes.end()) weaponFiring[id] = r->second;
			if(XMLElement* pStances = pComp->FirstChildElement("_STANCE_FIRINGMETHOD")) {
				for(XMLElement* pSt = pStances->FirstChildElement("_STANCE"); pSt; pSt = pSt->NextSiblingElement("_STANCE")) {
					const i8 stance = StanceFromString(pSt->Attribute("DATAKEY"));
					i32 stanceFiring = 0;
					pSt->QueryAttribute("DATA", &stanceFiring);
					auto sr = firingRemotes.find(stanceFiring);
					if(stance <= 0 || sr == firingRemotes.end()) continue;
					weaponStanceFiring[((u32)id << 4) | (u32)stance] = sr->second;
					auto sd = delayByFiring.find(stanceFiring);
					if(sd != delayByFiring.end()) weaponStanceFireDelay[((u32)id << 4) | (u32)stance] = sd->second;
				}
			}
			break;
		}
	}
	LOG("Loaded %d weapon overheat gauges, %d weapon firing remotes, %d stance firing methods", (i32)weaponHeat.size(), (i32)weaponFiring.size(), (i32)weaponStanceFiring.size());
	return true;
}

bool GameXmlContent::LoadSkillTargeting()
{
	XMLDocument xmlSkillP;
	if(!LoadXMLFile(L"/client/SKILL.xml", xmlSkillP)) return false;
	i32 count = 0;
	for(XMLElement* pInfo = xmlSkillP.FirstChildElement()->FirstChildElement(); pInfo; pInfo = pInfo->NextSiblingElement()) {
		XMLElement* pSk = pInfo->FirstChildElement("ST_COMMONSKILL");
		if(!pSk) continue;
		const char* type = nullptr;
		const char* cls = nullptr;
		i32 status = 0;
		pSk->QueryStringAttribute("_Type", &type);
		pSk->QueryStringAttribute("_RequireClass", &cls);
		pSk->QueryAttribute("_Status", &status);
		{
			i32 skillID = 0;
			pInfo->QueryAttribute("ID", &skillID);
			f32 cd = 0;
			if(pSk->QueryFloatAttribute("_CoolTime", &cd) == XML_SUCCESS) skillCoolTime[skillID] = cd;
			f32 rx = 0;
			if(pSk->QueryFloatAttribute("_SkillRangeLengthX", &rx) == XML_SUCCESS && rx > 0) skillRangeX[skillID] = rx;
			{
				const char* sfx = nullptr;
				f32 ry = 0;
				pSk->QueryStringAttribute("_SkillRangeSFX", &sfx);
				if(sfx && EA::StdC::Strstr(sfx, "Box") && pSk->QueryFloatAttribute("_SkillRangeLengthY", &ry) == XML_SUCCESS && ry > 0) skillRangeX[skillID] = ry;
			}
			f32 trx = 0;
			if(pSk->QueryFloatAttribute("_TargetRangeLengthX", &trx) == XML_SUCCESS && trx > 0) skillTargetRangeX[skillID] = trx;
			if(XMLElement* pT = pSk->FirstChildElement("_Target")) {
				f32 d = 0;
				if(pT->QueryFloatAttribute("_BotSkillMaxDistance", &d) == XML_SUCCESS && d > 0) botSkillMaxDistance[skillID] = d;
			}
		}
		if(!type || !cls || !status || EA::StdC::Strcmp(type, "SKILL_TYPE_PASSIVE") != 0) continue;
		auto found = masterClassStringMap.find(strHash(cls));
		if(found == masterClassStringMap.end()) continue;
		auto& list = passiveStatuses[(i32)found->second->classType];
		if(!list.full()) { list.push_back(status); count++; }
	}
	LOG("Loaded %d passive statuses", count);
	return true;
}

bool GameXmlContent::LoadInteractions()
{
	XMLDocument xmlInteraction;
	if(!LoadXMLFile(L"/client/INTERACTION.xml", xmlInteraction)) return false;
	for(XMLElement* pI = xmlInteraction.FirstChildElement()->FirstChildElement(); pI; pI = pI->NextSiblingElement()) {
		XMLElement* pData = pI->FirstChildElement("ST_INTERACTION");
		i32 id = 0;
		if(!pData || pI->QueryAttribute("ID", &id) != XML_SUCCESS) continue;
		InteractionInfo& info = interactions[id];
		pData->QueryFloatAttribute("_ActiveDistance", &info.activeDistance);
		const char* action = pData->Attribute("_Action");
		info.carry = action && EA::StdC::Strcmp(action, "IT_ACTION_TELEPORT") == 0;
	}
	return true;
}

bool GameXmlContent::LoadInteractionReceivers()
{
	XMLDocument xmlDynamic;
	if(!LoadXMLFile(L"/client/DYNAMIC.xml", xmlDynamic)) return false;
	for(XMLElement* pInfo = xmlDynamic.FirstChildElement()->FirstChildElement(); pInfo; pInfo = pInfo->NextSiblingElement()) {
		XMLElement* pRecv = pInfo->FirstChildElement("InteractionRecvComData");
		i32 id = 0;
		i32 interaction = 0;
		if(!pRecv || pInfo->QueryAttribute("ID", &id) != XML_SUCCESS || pRecv->QueryAttribute("_Index", &interaction) != XML_SUCCESS) continue;
		auto found = interactions.find(interaction);
		if(found != interactions.end() && found->second.receiver == CreatureIndex::Invalid) found->second.receiver = CreatureIndex(id);
	}
	return true;
}

bool GameXmlContent::LoadBotBrain()
{
	XMLDocument xmlBrain;
	if(!LoadXMLFile(L"/client/BotBrainData.xml", xmlBrain)) return false;
	XMLElement* pRoot = xmlBrain.FirstChildElement()->FirstChildElement("ST_BOT_BRAIN_DATA");
	for(XMLElement* p = pRoot ? pRoot->FirstChildElement("_SkillIndexData") : nullptr; p; p = p->NextSiblingElement("_SkillIndexData")) {
		i32 creature = 0;
		p->QueryAttribute("DATAKEY", &creature);
		eastl::array<i32,5> s = { 0, 0, 0, 0, 0 };
		p->QueryAttribute("_Skill0", &s[0]);
		p->QueryAttribute("_Skill1", &s[1]);
		p->QueryAttribute("_Skill2", &s[2]);
		p->QueryAttribute("_Skill3", &s[3]);
		p->QueryAttribute("_UgSkill", &s[4]);
		botSkills[creature] = s;
	}
	for(XMLElement* p = pRoot ? pRoot->FirstChildElement("_StageLevelData") : nullptr; p; p = p->NextSiblingElement("_StageLevelData")) {
		const char* key = nullptr;
		p->QueryStringAttribute("DATAKEY", &key);
		if(!key || EA::StdC::Strcmp(key, "E_BOT_STAGE_LEVEL_NORMAL") != 0) continue;
		for(XMLElement* d = p->FirstChildElement("_Data"); d; d = d->NextSiblingElement("_Data")) {
			const char* k = nullptr;
			f32 v = 0;
			d->QueryStringAttribute("DATAKEY", &k);
			d->QueryFloatAttribute("DATA", &v);
			if(!k) continue;
			if(EA::StdC::Strcmp(k, "BOTLOGIC_STAGELEVEL_KEY_NORMALSKILL_COOLTIME") == 0) botNormalSkillCoolTime = v;
			else if(EA::StdC::Strcmp(k, "BOTLOGIC_STAGELEVEL_KEY_UGSKILL_COOLTIME") == 0) botUgSkillCoolTime = v;
			else if(EA::StdC::Strcmp(k, "BOTLOGIC_STAGELEVEL_KEY_AFTER_SKILL_DELAY") == 0) botAfterSkillDelay = v;
		}
	}
	LOG("Loaded bot skills for %d masters (skill cd %.0f, ult cd %.0f, after skill %.1f)", (i32)botSkills.size(), botNormalSkillCoolTime, botUgSkillCoolTime, botAfterSkillDelay);
	return true;
}

const StatusInfo* GameXmlContent::FindStatus(i32 statusID) const
{
	auto found = statusMap.find(statusID);
	if(found == statusMap.end()) return nullptr;
	return &found->second;
}

const Remote* GameXmlContent::FindRemote(RemoteIdx remoteID) const
{
	auto found = remoteMap.find(remoteID);
	if(found == remoteMap.end()) return nullptr;
	return &found->second;
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
