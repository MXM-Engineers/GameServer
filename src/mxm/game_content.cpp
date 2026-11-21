#include <common/utils.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EASprintf.h>
#include <EASTL/sort.h>
#include <EASTL/string.h>
#include <tinyxml2.h>

#include "core.h"
#include "game_content.h"

using namespace tinyxml2;
constexpr eastl::hash<const char*> strHash;

static GameXmlContent* g_GameXmlContent = nullptr;

static Path gameDataDir = L"gamedata";

static bool XmlAttrTrue(XMLElement* pElt, const char* name)
{
	const char* v = nullptr;
	pElt->QueryStringAttribute(name, &v);
	return v && EA::StdC::Stricmp(v, "true") == 0;
}


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
			const char* skillSlot = pSkillElt->Attribute("_SkillSlot");
			if(skillSlot && EA::StdC::Strcmp(skillSlot, "SKILL_SLOT_SHIRK") == 0) master.shirkSkill = (SkillID)skillID;
			const char* unlock = pSkillElt->Attribute("_UnLock");
			u8 unlocked = 1;
			if(unlock && (unlock[0] == 'F' || unlock[0] == 'f')) unlocked = 0;
			master.skillUnlocked.push_back(unlocked);
			master.skillUnlockOrder.push_back((u8)pSkillElt->IntAttribute("_UnlockOrder", 0));
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
		if(XMLElement* pWeapon = pWeapElt->FirstChildElement("WeaponComData")) {
			for(XMLElement* r = pWeapon->FirstChildElement("_RequiredIngredients"); r; r = r->NextSiblingElement("_RequiredIngredients")) {
				auto& list = weaponIngredients[ID];
				if(!list.full()) list.push_back(eastl::make_pair(r->IntAttribute("_Index"), r->IntAttribute("_Count")));
			}
		}

		pWeapElt = pWeapElt->NextSiblingElement();
	} while(pWeapElt);

	if(!LoadWeaponModelDefinitions()) {
		LOG("ERROR(LoadWeaponModelDefinitions): failed");
		return false;
	}

	return true;
}

bool GameXmlContent::LoadXMLFile(const wchar* fileName, tinyxml2::XMLDocument& xmlData, bool optional)
{
	// Find a better way to check this
	// Simple path hash map?
	ASSERT_MSG(xmlData.NoChildren(), "xml already parsed");

	Path filePath = gameDataDir;
	PathAppend(filePath, fileName);

	i32 fileSize;
	u8* fileData = FileOpenAndReadAll(filePath.data(), &fileSize);
	if (!fileData) {
		if(!optional) LOG("ERROR(LoadXMLFile): failed to open '%ls'", filePath.data());
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
		master->moveSpeed = moveSpeed;
		pEntityComData->QueryFloatAttribute("_RotateSpeed", &rotateSpeed);
		pEntityComData->QueryFloatAttribute("_Scale", &scale);
		pEntityComData->QueryIntAttribute("_DensityCount", &master->densityCount);

		
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
		for(XMLElement* pR = pNodeCommonSkill->FirstChildElement("_RandomAction"); pR; pR = pR->NextSiblingElement("_RandomAction")) {
			RandomActionList& list = skillRandomActions[skillID];
			if(const char* a = pR->Attribute("DATA")) if(!list.full()) list.push_back(ActionStateFromString(a));
		}
		{
			i32 baseCombo = 0;
			pNodeCommonSkill->QueryIntAttribute("_BaseComboSkill", &baseCombo);
			if(baseCombo != 0 && type == SkillType::COMBO && firstComboSkill.find(baseCombo) == firstComboSkill.end()) firstComboSkill[baseCombo] = skillID;
			i32 nextCombo = 0;
			pNodeCommonSkill->QueryIntAttribute("_NextComboSkill", &nextCombo);
			if(nextCombo != 0) nextComboSkill[skillID] = nextCombo;
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
				in.minHpFriend = selectType && EA::StdC::Strcmp(selectType, "SKILL_TARGET_FRIENDLY_PC_MIN_HP") == 0;
				const char* casterType = pTarget->Attribute("_CasterType");
				in.petCaster = casterType && EA::StdC::Strcmp(casterType, "SKILL_CASTER_TYPE_PET") == 0;
				in.takesTarget = in.pickedEntity || in.lockOn || in.teleportEntity || (selectType && (EA::StdC::Strcmp(selectType, "SKILL_TARGET_USE_HIT_EVENT_TARGET") == 0 || EA::StdC::Strcmp(selectType, "SKILL_TARGET_PICKED_MINIMAP") == 0));
				if(selectType && EA::StdC::Strcmp(selectType, "SKILL_TARGET_STATUS") == 0) pTarget->QueryAttribute("_CheckStatusIndex", &in.checkStatus);
				if(const char* om = pTarget->Attribute("_TestObstructMove")) in.obstructMove = EA::StdC::Strcmp(om, "SKILL_OBSTRUCTMOVE_TYPE_JUMPABLE") == 0 ? 1 : 2;
				if(const char* os = pTarget->Attribute("_TestObstructSight")) in.obstructSight = EA::StdC::Strcmp(os, "SKILL_OBSTRUCTSIGHT_TYPE_ALL") == 0;
				pTarget->QueryFloatAttribute("_MinDistance", &in.minDistance);
				pTarget->QueryFloatAttribute("_MaxDistance", &in.maxDistance);
				in.friendlyOnly = XmlAttrTrue(pTarget, "_Friendly") && pTarget->Attribute("_UnFriendly") && !XmlAttrTrue(pTarget, "_UnFriendly");
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
			pNodeCommonSkill->QueryFloatAttribute("_GlobalCoolTime", &in.globalCoolTime);
			pNodeCommonSkill->QueryIntAttribute("_ResetCoolTimeID", &in.resetCoolTimeID);
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
			pCast->QueryBoolAttribute("_InterruptByLostTarget", &sc.interruptByLostTarget);
			pCast->QueryBoolAttribute("_InterruptByAttacked", &sc.interruptByAttacked);
			pCast->QueryBoolAttribute("_InterruptByOtherSkill", &sc.interruptByOtherSkill);
			if(XMLElement* pCharge = pCast->FirstChildElement("_Charge")) {
				pCharge->QueryFloatAttribute("_MaxChargingTime", &sc.maxChargingTime);
				pCharge->QueryFloatAttribute("_MinSkillChargeRatio", &sc.minChargeRatio);
				pCharge->QueryFloatAttribute("_MaxSkillChargeRatio", &sc.maxChargeRatio);
			}
			sc.action = castAction ? ActionStateFromString(castAction) : ActionStateID::INVALID;
			if(castType && sc.time > 0 && (sc.action != ActionStateID::INVALID || sc.type == SkillCast::LIMIT)) skillCast[skillID] = sc;
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
				if(id > 0) { ASSERT(!tg.remotes.full()); tg.remotes.push_back(id); }
			}
			for(XMLElement* p = pLists->FirstChildElement("_StatusList"); p; p = p->NextSiblingElement("_StatusList")) {
				i32 id = 0;
				p->QueryAttribute("DATA", &id);
				if(id > 0) { ASSERT(!tg.statuses.full()); tg.statuses.push_back(id); }
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

		static const char* MEASURED_PVP_SAVE_TYPES[] = { "PVP_SAVE_TYPE_INVALID", "PVP_SAVE_TYPE_NORMAL", "PVP_SAVE_TYPE_GOT", "PVP_SAVE_TYPE_OCCUPY" };
		if(const char* save = pMapElt->Attribute("_PVPSaveType")) {
			for(u8 t = 0; t < (u8)ARRAY_COUNT(MEASURED_PVP_SAVE_TYPES); t++) if(EA::StdC::Strcmp(save, MEASURED_PVP_SAVE_TYPES[t]) == 0) mapList.pvpSaveType = t;
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
		pSpawnElt->QueryAttribute("fScale", &spawn.scale);

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
		spawn.difficultyMask = (u8)pSpawnElt->IntAttribute("stageDifficultyMask", 0xff);
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
		pSpawnElt->QueryAttribute("fScale", &spawn.scale);

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
		spawn.difficultyMask = (u8)pSpawnElt->IntAttribute("stageDifficultyMask", 0xff);
		const char* actionStr = nullptr;
		if(pSpawnElt->QueryStringAttribute("strActionState", &actionStr) == XML_SUCCESS && actionStr) {
			spawn.actionState = ActionStateFromString(actionStr);
		}


		map->dynamic.push_back(spawn);
	}


	XMLDocument levelXml;
	if(LoadXMLFile(LFMT(L"/%s/Level.xml", tempPathData), levelXml, true) && levelXml.FirstChildElement()) {
		for(XMLElement* pInfo = levelXml.FirstChildElement()->FirstChildElement(); pInfo; pInfo = pInfo->NextSiblingElement()) {
			if(XMLElement* pLevel = pInfo->FirstChildElement("_LevelData")) {
				map->killZ = pLevel->FloatAttribute("KillZ", map->killZ);
				break;
			}
		}
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

	if(!LoadLevelEvents(map, tempPathData)) return false;

	XMLDocument pathDoc;
	if(LoadXMLFile(LFMT(L"/%s/Path.xml", tempPathData), pathDoc, true) && pathDoc.FirstChildElement()) {
		XMLElement* pList = pathDoc.FirstChildElement()->FirstChildElement("MAP_PATH_LIST");
		for(XMLElement* pPath = pList ? pList->FirstChildElement("MAP_PATH_INFO") : nullptr; pPath; pPath = pPath->NextSiblingElement("MAP_PATH_INFO")) {
			auto& points = map->paths[pPath->IntAttribute("DATAKEY")];
			for(XMLElement* pPoint = pPath->FirstChildElement("_PathList"); pPoint; pPoint = pPoint->NextSiblingElement("_PathList")) {
				points.push_back(vec3(pPoint->FloatAttribute("kTranslate_x"), pPoint->FloatAttribute("kTranslate_y"), pPoint->FloatAttribute("kTranslate_z")));
				if(pPoint->IntAttribute("RoundTrip") == 1) map->roundTripPaths.insert(pPath->IntAttribute("DATAKEY"));
			}
		}
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

static bool FileLoad(FileBuffer* out, const wchar* path)
{
	out->data = FileOpenAndReadAll(path, &out->size);
	if(!out->data) {
		LOG("ERROR(FileLoad): failed to open '%ls'", path);
		return false;
	}

	return true;
}

bool GameXmlContent::LoadPvpMaps()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/MapCollision.xml", doc)) return false;
	XMLElement* pRoot = doc.FirstChildElement();
	if(!pRoot) return false;
	eastl::hash_set<u32> loadedFiles;
	for(XMLElement* pMap = pRoot->FirstChildElement("_Map"); pMap; pMap = pMap->NextSiblingElement("_Map")) {
		const MapIndex index = (MapIndex)pMap->IntAttribute("_Index");
		pvpMaps.emplace_back();
		Map& map = pvpMaps.back();
		map.index = index;
		map.ingameLevels = pMap->IntAttribute("_IngameLevel") == 1;
		map.levelStartLead = pMap->FloatAttribute("_LevelStartLead", map.levelStartLead);
		map.groundPlane = pMap->IntAttribute("_GroundPlane", 1) != 0;
		if(!LoadMapByID(&map, index)) {
			LOG("ERROR(LoadPvpMaps): map %d failed to load", (i32)index);
			return false;
		}
		for(XMLElement* pFile = pMap->FirstChildElement("_File"); pFile; pFile = pFile->NextSiblingElement("_File")) {
			const char* file = pFile->Attribute("_Path");
			if(!file) return false;
			if(!loadedFiles.insert(eastl::hash<const char*>{}(file)).second) continue;
			Path path = gameDataDir;
			wchar_t fileWide[256] = {0};
			wchar_t* fileWideOut = fileWide;
			eastl::DecodePart(file, file + EA::StdC::Strlen(file), fileWideOut, fileWide + ARRAY_COUNT(fileWide) - 1);
			PathAppend(path, L"/");
			PathAppend(path, fileWide);
			FileBuffer buffer;
			if(!FileLoad(&buffer, path.data())) {
				LOG("ERROR(LoadPvpMaps): collision file '%s' not found", file);
				return false;
			}
			collisionFiles.push_back(buffer);
		}
		for(XMLElement* pCollider = pMap->FirstChildElement("_Collider"); pCollider; pCollider = pCollider->NextSiblingElement("_Collider")) {
			Map::Collider collider;
			const char* mesh = pCollider->Attribute("_Mesh");
			if(!mesh) return false;
			collider.mesh = mesh;
			collider.pos = vec3(pCollider->FloatAttribute("x"), pCollider->FloatAttribute("y"), pCollider->FloatAttribute("z"));
			collider.rot = vec3(pCollider->FloatAttribute("angx"), pCollider->FloatAttribute("angy"), pCollider->FloatAttribute("angz"));
			collider.scale = pCollider->FloatAttribute("scale", 1.0f);
			collider.walk = pCollider->IntAttribute("_Walk", 1) != 0;
			map.colliders.push_back(collider);
		}
		LOG("PvP map %d: %d creatures, %d dynamics, %d areas, %d level events, %d colliders", (i32)index, (i32)map.creatures.size(), (i32)map.dynamic.size(), (i32)map.areas.size(), (i32)map.levelEvents.size(), (i32)map.colliders.size());
	}
	for(XMLElement* pAlias = pRoot->FirstChildElement("_Alias"); pAlias; pAlias = pAlias->NextSiblingElement("_Alias")) {
		pvpMapAliases[pAlias->IntAttribute("_Index")] = pAlias->IntAttribute("_Map");
	}
	LOG("Loaded %d map aliases", (i32)pvpMapAliases.size());
	return FindPvpMap(MapIndex::PVP_DEATHMATCH) != nullptr;
}

bool GameXmlContent::LoadMasterTraits()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/MasterTraits.xml", doc)) return false;
	traitMaxPoints = doc.FirstChildElement()->IntAttribute("maxPointsPerTrait", traitMaxPoints);
	for(XMLElement* pInfo = doc.FirstChildElement()->FirstChildElement("INFO"); pInfo; pInfo = pInfo->NextSiblingElement("INFO")) {
		auto& traits = masterTraits[pInfo->IntAttribute("class")];
		for(XMLElement* pTrait = pInfo->FirstChildElement("TRAIT"); pTrait; pTrait = pTrait->NextSiblingElement("TRAIT")) {
			const i32 index = pTrait->IntAttribute("index");
			if(index < 0 || index >= 4) continue;
			for(XMLElement* st = pTrait->FirstChildElement("STAT"); st && !traits[index].full(); st = st->NextSiblingElement("STAT")) {
				traits[index].push_back({ (u8)st->IntAttribute("type"), st->FloatAttribute("perPoint") });
			}
		}
	}
	LOG("Loaded traits of %d masters", (i32)masterTraits.size());
	return true;
}

bool GameXmlContent::LoadMasterGrowth()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/MasterGrowth.xml", doc)) return false;
	for(XMLElement* pInfo = doc.FirstChildElement()->FirstChildElement("INFO"); pInfo; pInfo = pInfo->NextSiblingElement("INFO")) {
		auto& stats = masterGrowth[pInfo->IntAttribute("class")];
		for(XMLElement* st = pInfo->FirstChildElement("STAT"); st && !stats.full(); st = st->NextSiblingElement("STAT")) {
			MasterGrowthStat g;
			g.type = (u8)st->IntAttribute("type");
			g.linear = st->FloatAttribute("linear");
			g.quadratic = st->FloatAttribute("quadratic");
			stats.push_back(g);
		}
	}
	LOG("Loaded in-game growth of %d masters", (i32)masterGrowth.size());
	return true;
}

bool GameXmlContent::LoadIngameLevel()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/IngameLevel.xml", doc)) return false;
	XMLElement* pRoot = doc.FirstChildElement();
	if(!pRoot) return false;
	IngameLevelInfo& info = ingameLevel;
	info.maxLevel = pRoot->IntAttribute("maxLevel", 1);
	info.passiveExp = pRoot->IntAttribute("passiveExp");
	info.passivePeriod = pRoot->FloatAttribute("passivePeriod");
	info.shareRadius = pRoot->FloatAttribute("shareRadius");
	info.propertyPointsEvery = pRoot->IntAttribute("propertyPointsEvery");
	info.propertyPointsStep = pRoot->IntAttribute("propertyPointsStep");
	info.respawnBaseMs = pRoot->FloatAttribute("respawnBaseMs");
	info.respawnPerLevelMs = pRoot->FloatAttribute("respawnPerLevelMs");
	info.respawnPerSecMs = pRoot->FloatAttribute("respawnPerSecMs");
	info.killPoints = pRoot->IntAttribute("killPoints", 1);
	for(XMLElement* e = pRoot->FirstChildElement("LEVEL"); e && !info.levelExp.full(); e = e->NextSiblingElement("LEVEL")) info.levelExp.push_back(e->IntAttribute("exp"));
	for(XMLElement* e = pRoot->FirstChildElement("MONSTER"); e; e = e->NextSiblingElement("MONSTER")) {
		IngameLevelInfo::MonsterExp m;
		m.exp = e->IntAttribute("exp");
		m.growth = e->IntAttribute("growth");
		m.growthPeriod = e->FloatAttribute("growthPeriod");
		m.points = e->IntAttribute("points");
		m.killerBonus = e->IntAttribute("killerBonus");
		const char* mode = e->Attribute("mode");
		if(mode && EA::StdC::Strcmp(mode, "team") == 0) m.mode = IngameLevelInfo::MonsterExp::TEAM;
		else if(mode && EA::StdC::Strcmp(mode, "killer") == 0) m.mode = IngameLevelInfo::MonsterExp::KILLER;
		info.monsters[e->IntAttribute("ID")] = m;
	}
	if(XMLElement* e = pRoot->FirstChildElement("MASTER_KILL")) {
		info.masterKillBase = e->FloatAttribute("base");
		info.masterKillPerLevel = e->FloatAttribute("perVictimLevel");
		info.masterKillAssistPool = e->FloatAttribute("assistPool");
		info.masterKillAssistWindowSec = e->FloatAttribute("assistWindowSec");
	}
	for(XMLElement* e = pRoot->FirstChildElement("SHARE"); e && !info.shares.full(); e = e->NextSiblingElement("SHARE")) info.shares.push_back(eastl::make_pair(e->FloatAttribute("killer"), e->FloatAttribute("helper")));
	LOG("Loaded in-game levels (max %d, %d monsters)", info.maxLevel, (i32)info.monsters.size());
	return true;
}

template<typename List>
static void ParseIdList(const char* text, List& out)
{
	if(!text) return;
	const char* at = text;
	while(*at && !out.full()) {
		while(*at == ' ') at++;
		if(!*at) break;
		out.push_back(EA::StdC::AtoI32(at));
		while(*at && *at != ' ') at++;
	}
}

static void ApplyMonsterScript(GameXmlContent::MonsterAi& ai, XMLElement* pScript, i32 id)
{
	ai.scriptStart = pScript->FloatAttribute("start");
	ai.scriptPeriod = pScript->FloatAttribute("period");
	if(const char* pause = pScript->Attribute("pauseOnState")) ai.scriptPause = ActionStateFromString(pause);
	if(pScript->Attribute("groggyChance")) ai.groggyChance = pScript->FloatAttribute("groggyChance");
	if(pScript->Attribute("groggyTime")) ai.groggyTime = pScript->FloatAttribute("groggyTime");
	if(const char* gs = pScript->Attribute("groggyStatus")) { ai.groggyStatuses.clear(); ParseIdList(gs, ai.groggyStatuses); }
	for(XMLElement* pAttack = pScript->FirstChildElement("Attack"); pAttack; pAttack = pAttack->NextSiblingElement("Attack")) {
		const char* stateName = pAttack->Attribute("state");
		if(!stateName) continue;
		const ActionStateID state = ActionStateFromString(stateName);
		GameXmlContent::MonsterAttack* attack = nullptr;
		foreach(a, ai.attacks) if(a->state == state) attack = &*a;
		if(!attack) {
			const char* like = pAttack->Attribute("like");
			const ActionStateID likeState = like ? ActionStateFromString(like) : ActionStateID::INVALID;
			const GameXmlContent::MonsterAttack* model = nullptr;
			foreach_const(a, ai.attacks) if(a->state == likeState) model = &*a;
			if(!model || ai.attacks.full()) {
				LOG("WARNING(MonsterScripts): monster %d has no attack %s to copy for %s", id, like ? like : "-", stateName);
				continue;
			}
			GameXmlContent::MonsterAttack copy = *model;
			copy.state = state;
			copy.special = true;
			copy.share = 0;
			copy.cooldown = 1000000.0f;
			ai.attacks.push_back(copy);
			attack = &ai.attacks.back();
		}
		if(pAttack->Attribute("anim")) attack->anim = pAttack->FloatAttribute("anim");
		if(pAttack->Attribute("weakAt")) attack->weakAt = pAttack->FloatAttribute("weakAt");
		if(pAttack->Attribute("weakTime")) attack->weakTime = pAttack->FloatAttribute("weakTime");
		if(pAttack->Attribute("weakStatus")) attack->weakStatus = pAttack->IntAttribute("weakStatus");
		if(pAttack->Attribute("evade")) attack->evade = pAttack->FloatAttribute("evade");
		if(pAttack->Attribute("multiplier")) attack->multiplier = pAttack->FloatAttribute("multiplier");
		if(pAttack->Attribute("radius")) attack->radius = pAttack->FloatAttribute("radius");
		if(pAttack->Attribute("hits")) attack->hits = (u8)MAX(pAttack->IntAttribute("hits"), 1);
		if(pAttack->Attribute("interval")) attack->interval = pAttack->FloatAttribute("interval");
		if(pAttack->Attribute("delay")) attack->delay = pAttack->FloatAttribute("delay");
		if(pAttack->Attribute("hitAt")) attack->hitAt = pAttack->FloatAttribute("hitAt");
		if(pAttack->Attribute("speed")) attack->speed = pAttack->FloatAttribute("speed");
		if(pAttack->Attribute("status")) attack->status = pAttack->IntAttribute("status");
		if(pAttack->Attribute("selfCentered")) attack->selfCentered = pAttack->BoolAttribute("selfCentered");
	}
	ai.script.clear();
	for(XMLElement* pStep = pScript->FirstChildElement("Step"); pStep && !ai.script.full(); pStep = pStep->NextSiblingElement("Step")) {
		GameXmlContent::MonsterAi::ScriptStep step;
		step.t = pStep->FloatAttribute("t");
		step.hpMin = pStep->FloatAttribute("hpMin", 0.0f) / 100.0f;
		step.hpMax = pStep->FloatAttribute("hpMax", 200.0f) / 100.0f;
		step.aiTrigger = pStep->IntAttribute("aiTrigger", -1);
		if(const char* pick = pStep->Attribute("pick")) step.pick = EA::StdC::Strcmp(pick, "random") == 0 ? 1 : (EA::StdC::Strcmp(pick, "alternate") == 0 ? 2 : 0);
		const char* states = pStep->Attribute("state");
		while(states && *states && !step.attacks.full()) {
			eastl::fixed_string<char,64,false> name;
			while(*states && *states != '|') name.push_back(*states++);
			if(*states == '|') states++;
			const ActionStateID state = ActionStateFromString(name.c_str());
			i32 index = -1;
			for(i32 a = 0; a < (i32)ai.attacks.size(); a++) if(ai.attacks[a].state == state) index = a;
			if(index < 0) { LOG("WARNING(MonsterScripts): monster %d has no attack %s", id, name.c_str()); continue; }
			step.attacks.push_back((u8)index);
		}
		if(!step.attacks.empty() || step.aiTrigger >= 0) ai.script.push_back(step);
	}
}

void GameXmlContent::LoadMonsterScripts()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/MonsterScripts.xml", doc, true) || !doc.FirstChildElement()) return;
	i32 loaded = 0;
	for(XMLElement* pScript = doc.FirstChildElement()->FirstChildElement("Script"); pScript; pScript = pScript->NextSiblingElement("Script")) {
		const i32 id = pScript->IntAttribute("ID");
		auto found = monsterAi.find(id);
		if(found == monsterAi.end()) { LOG("WARNING(MonsterScripts): no AI for monster %d", id); continue; }
		XMLElement* source = pScript;
		if(const i32 copyOf = pScript->IntAttribute("copyOf")) {
			source = nullptr;
			for(XMLElement* s = doc.FirstChildElement()->FirstChildElement("Script"); s; s = s->NextSiblingElement("Script")) if(s->IntAttribute("ID") == copyOf) source = s;
			if(!source) { LOG("WARNING(MonsterScripts): monster %d copies missing script %d", id, copyOf); continue; }
		}
		ApplyMonsterScript(found->second, source, id);
		if(found->second.scriptPeriod > 0 && !found->second.script.empty()) loaded++;
	}
	LOG("Loaded %d monster scripts", loaded);
}

void GameXmlContent::LoadMonsterFires()
{
	static const wchar* FILES[] = { L"/client/ActionBase_Monster.xml", L"/client/ActionBase_Monster_MOB_A.xml", L"/client/ActionBase_Monster_MOB_B.xml", L"/client/ActionBase_Monster_MOB_C.xml", L"/client/ActionBase_Monster_MOB_D.xml", L"/client/ActionBase_Monster_MOB_PUBLIC.xml", L"/client/ActionBase_Monster_MOB_PvP.xml" };
	eastl::hash_map<u64, eastl::fixed_vector<MonsterFire,24,false>> fires;
	struct Dash { f32 dist = 0; f32 time = 0; i32 pass = -1; eastl::fixed_vector<i32,4,false> anims; eastl::fixed_vector<i32,4,false> loops; };
	eastl::hash_map<u64, Dash> dashes;
	for(i32 f = 0; f < (i32)ARRAY_COUNT(FILES); f++) {
		XMLDocument doc;
		if(!LoadXMLFile(FILES[f], doc, true) || !doc.FirstChildElement()) continue;
		for(XMLElement* e = doc.FirstChildElement()->FirstChildElement("ActionBase"); e; e = e->NextSiblingElement("ActionBase")) {
			const char* type = e->Attribute("CommandType");
			const char* cls = e->Attribute("Class");
			const char* state = e->Attribute("BehaviorState");
			if(type && cls && state) {
				Dash& dash = dashes[((u64)StatusTypeKey(cls) << 32) | (u32)ActionStateFromString(state)];
				const i32 pass = e->IntAttribute("AnimationIndex");
				if(pass >= 0 && pass < 4) {
					while((i32)dash.anims.size() <= pass) { dash.anims.push_back(0); dash.loops.push_back(1); }
					dash.anims[pass] = e->IntAttribute("AnimationId");
					if(e->IntAttribute("AnimationLoop") > 1) dash.loops[pass] = e->IntAttribute("AnimationLoop");
				}
				if(EA::StdC::Strcmp(type, "GRAPH_MOVE_HORZ") == 0) {
					const char* param = e->Attribute("Param1");
					if(param && EA::StdC::Sscanf(param, "%f|%f", &dash.dist, &dash.time) >= 1) dash.pass = pass;
				}
			}
			if(!type || !cls || !state || EA::StdC::Strcmp(type, "REMOTE") != 0 || e->IntAttribute("RemoteIndex") <= 0) continue;
			const char* rowParam2 = e->Attribute("Param2");
			if(rowParam2 && EA::StdC::Strcmp(rowParam2, "Remove") == 0) continue;
			auto& list = fires[((u64)StatusTypeKey(cls) << 32) | (u32)ActionStateFromString(state)];
			MonsterFire fire;
			fire.remote = e->IntAttribute("RemoteIndex");
			const char* preset = e->Attribute("TargetPreset");
			fire.atTarget = preset && EA::StdC::Strncmp(preset, "TARGET_POS", 10) == 0;
			fire.pass = (u8)MAX(MIN(e->IntAttribute("AnimationIndex"), 3), 0);
			const char* node = e->Attribute("NodeName");
			for(const char* c = node ? node : "Scene Root"; *c && !fire.node.full(); c++) fire.node.push_back((wchar)*c);
			const char* param = e->Attribute("Param1");
			const char* times = param ? EA::StdC::Strchr(param, '|') : nullptr;
			if(times) {
				const char* at = times + 1;
				while(*at && !list.full()) {
					f32 t = 0;
					if(EA::StdC::Sscanf(at, "%f", &t) != 1) break;
					fire.at = t;
					list.push_back(fire);
					while(*at && *at != ',') at++;
					if(*at == ',') at++;
				}
			}
			else if(!list.full()) {
				fire.at = e->FloatAttribute("Delay");
				list.push_back(fire);
			}
		}
	}
	i32 attached = 0;
	for(auto& ai : monsterAi) {
		const u64 classKey = (u64)StatusTypeKey(ai.second.className.c_str()) << 32;
		foreach(attack, ai.second.attacks) {
			auto dash = dashes.find(classKey | (u32)attack->state);
			if(dash != dashes.end()) {
				for(i32 p = 0; p < (i32)dash->second.anims.size(); p++) attack->passes.push_back({ dash->second.anims[p], dash->second.loops[p] });
			}
			if(dash != dashes.end() && dash->second.pass >= 0 && dash->second.dist > 0) {
				attack->dashDist = dash->second.dist;
				attack->dashTime = dash->second.time;
				for(i32 p = 0; p <= dash->second.pass && p < (i32)dash->second.anims.size(); p++) attack->dashPassAnims.push_back(dash->second.anims[p]);
			}
			auto found = fires.find(classKey | (u32)attack->state);
			if(found == fires.end()) continue;
			attack->fires = found->second;
			attached++;
		}
		if(ai.second.walkState != ActionStateID::INVALID) {
			auto walkFires = fires.find(classKey | (u32)ai.second.walkState);
			if(walkFires != fires.end()) for(const MonsterFire& f : walkFires->second) if(!ai.second.walkFires.full()) ai.second.walkFires.push_back(f);
		}
	}
	LOG("Loaded %d monster fire states, attached to %d monster attacks", (i32)fires.size(), attached);
}

bool GameXmlContent::LoadMonsterAi()
{
	static const wchar* AI_FILES[] = { L"/MonsterAi.xml", L"/MonsterAiEstimated.xml" };
	for(i32 fileIndex = 0; fileIndex < (i32)ARRAY_COUNT(AI_FILES); fileIndex++) {
		const bool estimated = fileIndex > 0;
		XMLDocument doc;
		if(!LoadXMLFile(AI_FILES[fileIndex], doc, estimated)) {
			if(estimated) continue;
			return false;
		}
		for(XMLElement* pInfo = doc.FirstChildElement()->FirstChildElement("INFO"); pInfo; pInfo = pInfo->NextSiblingElement("INFO")) {
			if(estimated && monsterAi.find(pInfo->IntAttribute("ID")) != monsterAi.end()) continue;
			MonsterAi& ai = monsterAi[pInfo->IntAttribute("ID")];
			ai.className = pInfo->Attribute("class") ? pInfo->Attribute("class") : "";
			ai.walk = pInfo->FloatAttribute("walk");
			ai.run = pInfo->FloatAttribute("run");
			ai.perception = pInfo->FloatAttribute("perception");
			ai.lost = pInfo->FloatAttribute("lost");
			ai.destroy = pInfo->FloatAttribute("destroy");
			ai.radius = pInfo->FloatAttribute("radius");
			ai.tagKey = StatusTypeKey(pInfo->Attribute("tag") ? pInfo->Attribute("tag") : "");
			if(const char* idle = pInfo->Attribute("idle")) ai.idle = ActionStateFromString(idle);
			if(const char* awake = pInfo->Attribute("awake")) if(*awake) ai.awake = ActionStateFromString(awake);
			ai.awakeTime = pInfo->FloatAttribute("awakeTime");
			ai.groggyChance = pInfo->FloatAttribute("groggyChance");
			ai.groggyTime = pInfo->FloatAttribute("groggyTime");
			ai.spawnTime = pInfo->FloatAttribute("spawnTime");
			ai.chaseRing = pInfo->FloatAttribute("chaseRing");
			ai.chaseArc = pInfo->FloatAttribute("chaseArc") * PI / 180.0f;
			ai.turnLock = pInfo->BoolAttribute("turnLock");
			ai.turnEvery = pInfo->FloatAttribute("turnEvery");
			ai.retargetGain = pInfo->FloatAttribute("retargetGain");
			ai.retargetRate = pInfo->FloatAttribute("retargetRate");
			ai.masterMark = pInfo->IntAttribute("masterMark");
			ai.teamBuff = pInfo->IntAttribute("teamBuff");
			if(const char* team = pInfo->Attribute("team")) ai.playerSide = EA::StdC::Strcmp(team, "player") == 0;
			ai.mineral = pInfo->BoolAttribute("mineral");
			if(const char* walkState = pInfo->Attribute("walkState")) ai.walkState = ActionStateFromString(walkState);
			if(const char* preAttack = pInfo->Attribute("preAttack")) ai.preAttack = ActionStateFromString(preAttack);
			ai.lowHpRatio = pInfo->FloatAttribute("lowHp");
			if(const char* lowHpState = pInfo->Attribute("lowHpState")) ai.lowHpState = ActionStateFromString(lowHpState);
			ai.lowHpTime = pInfo->FloatAttribute("lowHpTime");
			ai.hpBuffSpeedStep = pInfo->FloatAttribute("hpBuffSpeedStep");
			ai.ccArmorStatus = pInfo->IntAttribute("ccArmorStatus");
			ai.dieStand = pInfo->FloatAttribute("dieStand");
			ai.allyBuffRadius = pInfo->FloatAttribute("allyBuffRadius");
			ai.allyBuffInterval = pInfo->FloatAttribute("allyBuffInterval");
			if(const char* allyBuffs = pInfo->Attribute("allyBuffs")) {
				const char* at = allyBuffs;
				while(*at && !ai.allyBuffs.full()) {
					char* end = nullptr;
					const long v = strtol(at, &end, 10);
					if(end == at) break;
					ai.allyBuffs.push_back((i32)v);
					at = end;
				}
			}
			if(const char* hpBuffs = pInfo->Attribute("hpBuffs")) {
				const char* at = hpBuffs;
				while(*at && !ai.hpBuffs.full()) {
					f32 ratio = 0;
					i32 status = 0;
					if(EA::StdC::Sscanf(at, "%f:%d", &ratio, &status) != 2) break;
					ai.hpBuffs.push_back(eastl::make_pair(ratio, status));
					while(*at && *at != ' ') at++;
					while(*at == ' ') at++;
				}
			}
			if(const char* victory = pInfo->Attribute("victory")) {
				char first[64] = {}, second[64] = {};
				if(EA::StdC::Sscanf(victory, "%63s %63s", first, second) == 2) {
					ai.victoryStates[0] = ActionStateFromString(first);
					ai.victoryStates[1] = ActionStateFromString(second);
				}
				if(const char* times = pInfo->Attribute("victoryTimes")) EA::StdC::Sscanf(times, "%f %f", &ai.victoryTimes[0], &ai.victoryTimes[1]);
				ParseIdList(pInfo->Attribute("victoryStatus"), ai.victoryStatuses);
				ai.victoryStatusDelay = pInfo->FloatAttribute("victoryStatusDelay");
				ai.victoryHeal = pInfo->FloatAttribute("victoryHeal");
			}
			ParseIdList(pInfo->Attribute("groggyStatus"), ai.groggyStatuses);
			ParseIdList(pInfo->Attribute("seek"), ai.seekDocs);
			ParseIdList(pInfo->Attribute("hideStatuses"), ai.hideStatuses);
			ai.reactionImmune = pInfo->BoolAttribute("reactionImmune");
			ParseIdList(pInfo->Attribute("wakeStatuses"), ai.wakeStatuses);
			ai.seekReach = pInfo->FloatAttribute("seekReach");
			if(const char* cc = pInfo->Attribute("cc")) {
				const char* at = cc;
				while(*at && !ai.ccTypes.full()) {
					while(*at == ' ') at++;
					if(!*at) break;
					eastl::fixed_string<char,48,false> name;
					while(*at && *at != ' ') name.push_back(*at++);
					ai.ccTypes.push_back(StatusTypeFromString(name.c_str()));
				}
			}
			if(const char* enemies = pInfo->Attribute("enemies")) {
				const char* at = enemies;
				while(*at && !ai.enemyTags.full()) {
					const char* end = at;
					while(*end && *end != '|') end++;
					eastl::fixed_string<char,32,false> tag(at, end);
					ai.enemyTags.push_back(StatusTypeKey(tag.c_str()));
					at = *end ? end + 1 : end;
				}
			}
			for(XMLElement* pAttack = pInfo->FirstChildElement("ATTACK"); pAttack && !ai.attacks.full(); pAttack = pAttack->NextSiblingElement("ATTACK")) {
				MonsterAttack attack;
				const char* target = pAttack->Attribute("target");
				if(!target) continue;
				if(EA::StdC::Strcmp(target, "master") == 0) attack.target = MonsterAttack::MASTER;
				else if(EA::StdC::Strcmp(target, "minion") == 0) attack.target = MonsterAttack::MINION;
				else if(EA::StdC::Strcmp(target, "titan") == 0) attack.target = MonsterAttack::TITAN;
				else if(EA::StdC::Strcmp(target, "ally") == 0) attack.target = MonsterAttack::ALLY;
				else attack.target = MonsterAttack::STRUCTURE;
				if(const char* state = pAttack->Attribute("state")) attack.state = ActionStateFromString(state);
				attack.range = pAttack->FloatAttribute("range");
				attack.cooldown = pAttack->FloatAttribute("cooldown");
				attack.remote = pAttack->IntAttribute("remote");
				attack.hitRemote = pAttack->IntAttribute("hitRemote", attack.remote);
				attack.executeMinion = pAttack->BoolAttribute("executeMinion");
				if(attack.executeMinion && attack.remote > 0) executeMinionRemotes.insert(attack.remote);
				if(attack.executeMinion && attack.hitRemote > 0) executeMinionRemotes.insert(attack.hitRemote);
				attack.anim = pAttack->FloatAttribute("anim");
				attack.noDamage = pAttack->BoolAttribute("noDamage");
				attack.selfDie = pAttack->BoolAttribute("selfDie");
				attack.allyStatus = pAttack->IntAttribute("allyStatus");
				attack.allyRadius = pAttack->FloatAttribute("allyRadius");
				attack.allyDelay = pAttack->FloatAttribute("allyDelay");
				attack.delay = pAttack->FloatAttribute("delay");
				attack.multiplier = pAttack->FloatAttribute("multiplier");
				attack.speed = pAttack->FloatAttribute("speed");
				attack.status = pAttack->IntAttribute("status");
				const char* kind = pAttack->Attribute("kind");
				attack.special = kind && EA::StdC::Strcmp(kind, "special") == 0;
				attack.radius = pAttack->FloatAttribute("radius");
				static const i32 MEASURED_AREA_REACH_MONSTERS[] = { 105001403 };
				for(i32 id : MEASURED_AREA_REACH_MONSTERS) if(id == pInfo->IntAttribute("ID") && !attack.special && attack.speed <= 0 && attack.radius * 0.5f > attack.range) attack.range = attack.radius * 0.5f;
				attack.hits = (u8)MAX(pAttack->IntAttribute("hits", 1), 1);
				attack.interval = pAttack->FloatAttribute("interval");
				attack.hpMin = pAttack->FloatAttribute("hpMin", 0.0f);
				attack.hpMax = pAttack->FloatAttribute("hpMax", 1.0f);
				attack.weakTime = pAttack->FloatAttribute("weakTime");
				attack.weakAt = pAttack->FloatAttribute("weakAt");
				attack.firstAt = pAttack->FloatAttribute("firstAt", -1.0f);
				attack.after = pAttack->FloatAttribute("after", 0.0f);
				attack.minRange = pAttack->FloatAttribute("minRange");
				attack.weakStatus = pAttack->IntAttribute("weakStatus");
				attack.evade = pAttack->FloatAttribute("evade");
				attack.selfCentered = pAttack->BoolAttribute("selfCentered");
				attack.share = pAttack->FloatAttribute("share", 1.0f);
				ParseIdList(pAttack->Attribute("selfStatus"), attack.selfStatuses);
				ParseIdList(pAttack->Attribute("allyStatus"), attack.allyStatuses);
				ai.attacks.push_back(attack);
			}
			foreach_const(a, ai.attacks) if(!a->special && !ai.byTarget[a->target]) ai.byTarget[a->target] = &*a;
			if(!ai.byTarget[MonsterAttack::TITAN]) ai.byTarget[MonsterAttack::TITAN] = ai.byTarget[MonsterAttack::STRUCTURE];
		}
	}
	XMLDocument xmlPvp;
	if(LoadXMLFile(L"/client/CREATURE_MONSTER_PvP.xml", xmlPvp)) {
		for(XMLElement* pInfo = xmlPvp.FirstChildElement()->FirstChildElement(); pInfo; pInfo = pInfo->NextSiblingElement()) {
			auto found = monsterAi.find(pInfo->IntAttribute("ID"));
			if(found == monsterAi.end()) continue;
			if(XMLElement* p = pInfo->FirstChildElement("Ai_ComData")) p->QueryFloatAttribute("_ReturnToSpawnPosSpeedRate", &found->second.returnSpeedRate);
			const XMLElement* entity = pInfo->FirstChildElement("EntityComData");
			const char* sizeType = entity ? entity->Attribute("_SizeType") : nullptr;
			if(entity) entity->QueryIntAttribute("_DensityCount", &found->second.density);
			if(sizeType) found->second.sizeRank = EA::StdC::Strstr(sizeType, "BIG") ? 3 : EA::StdC::Strstr(sizeType, "MEDIUM") ? 2 : EA::StdC::Strstr(sizeType, "SMALL") ? 1 : 0;
			const XMLElement* mesh = pInfo->FirstChildElement("PhysXMeshComData");
			const char* group = mesh ? mesh->Attribute("_ActorGroup") : nullptr;
			if(entity && group && EA::StdC::Strcmp(group, "GROUP_BODY_BOSSNPC") == 0 && entity->FloatAttribute("_MoveSpeed") > 0) {
				found->second.movingBodyRadius = mesh->FloatAttribute("_ColliderRadius");
				found->second.movingBodyHeight = mesh->FloatAttribute("_ColliderHeight");
			}
			if(entity && group && EA::StdC::Strcmp(group, "GROUP_BODY_BOSSNPC") == 0 && entity->FloatAttribute("_MoveSpeed") <= 0) {
				found->second.bodyBlockRadius = mesh->FloatAttribute("_ColliderRadius");
				found->second.bodyBlockHeight = mesh->FloatAttribute("_ColliderHeight");
			}
		}
	}
	XMLDocument xmlPvpActions;
	i32 spawnStatusCount = 0, dieRemoteCount = 0;
	if(LoadXMLFile(L"/client/ActionBase_Monster_MOB_PvP.xml", xmlPvpActions)) {
		for(XMLElement* pA = xmlPvpActions.FirstChildElement()->FirstChildElement(); pA; pA = pA->NextSiblingElement()) {
			const char* cls = pA->Attribute("Class");
			const char* state = pA->Attribute("BehaviorState");
			const char* type = pA->Attribute("CommandType");
			const char* preset = pA->Attribute("TargetPreset");
			if(!cls || !state || !type || pA->Attribute("VariationType")) continue;
			const char* param2 = pA->Attribute("Param2");
			const ActionStateID rowState = ActionStateFromString(state);
			if(rowState == ActionStateID::EXTRAMOTION_1_BEHAVIORSTATE || rowState == ActionStateID::EXTRAMOTION_2_BEHAVIORSTATE || rowState == ActionStateID::EXTRAMOTION_3_BEHAVIORSTATE) {
				const bool selfRow = EA::StdC::Strcmp(type, "STATUS") == 0 && preset && EA::StdC::Strncmp(preset, "TARGET_SELF", 11) == 0;
				const bool removeRow = param2 && EA::StdC::Strcmp(param2, "Remove") == 0;
				const i32 animID = pA->IntAttribute("AnimationId");
				foreach(it, monsterAi) {
					MonsterAi& ai = it->second;
					if(ai.className != cls) continue;
					if(selfRow && (!param2 || removeRow) && pA->IntAttribute("StatusIndex") != 0 && !ai.stateStatuses.full()) ai.stateStatuses.push_back({ rowState, pA->IntAttribute("StatusIndex"), removeRow });
					bool known = false;
					foreach_const(a, ai.stateAnims) if(a->state == rowState) known = true;
					if(animID != 0 && !known && !ai.stateAnims.full()) ai.stateAnims.push_back({ rowState, animID });
				}
			}
			const bool remoteCmd = EA::StdC::Strcmp(type, "REMOTE") == 0 && preset && EA::StdC::Strncmp(preset, "TARGET_SELF", 11) == 0;
			if(param2 && remoteCmd && EA::StdC::Strcmp(param2, "Remove") == 0) {
				const i32 removed = pA->IntAttribute("RemoteIndex");
				const ActionStateID removeState = ActionStateFromString(state);
				foreach(it, monsterAi) if(it->second.className == cls && removeState != ActionStateID::INVALID && removeState == it->second.lowHpState && !it->second.lowHpRemoveRemotes.full()) it->second.lowHpRemoveRemotes.push_back(removed);
				continue;
			}
			if(param2) continue;
			const bool selfStatus = EA::StdC::Strcmp(type, "STATUS") == 0 && preset && EA::StdC::Strncmp(preset, "TARGET_SELF", 11) == 0;
			const bool spawnStatus = selfStatus && EA::StdC::Strcmp(state, "SPAWN_BEHAVIORSTATE") == 0;
			const bool dieRemote = EA::StdC::Strcmp(state, "DIE_BEHAVIORSTATE") == 0 && EA::StdC::Strcmp(type, "REMOTE") == 0;
			const bool spawnRemote = remoteCmd && EA::StdC::Strcmp(state, "SPAWN_BEHAVIORSTATE") == 0;
			if(spawnRemote) {
				const i32 remoteIndex = pA->IntAttribute("RemoteIndex");
				foreach(it, monsterAi) if(it->second.className == cls && remoteIndex != 0 && !it->second.spawnRemotes.full()) it->second.spawnRemotes.push_back(remoteIndex);
				continue;
			}
			if(!selfStatus && !dieRemote) continue;
			const i32 index = pA->IntAttribute(dieRemote ? "RemoteIndex" : "StatusIndex");
			if(index == 0) continue;
			const ActionStateID stateID = ActionStateFromString(state);
			foreach(it, monsterAi) {
				MonsterAi& ai = it->second;
				if(ai.className != cls) continue;
				if(spawnStatus && !ai.spawnStatuses.full()) { ai.spawnStatuses.push_back(index); spawnStatusCount++; }
				if(dieRemote && !ai.dieRemotes.full()) { ai.dieRemotes.push_back(index); dieRemoteCount++; }
				if(selfStatus && stateID != ActionStateID::INVALID && stateID == ai.walkState && !ai.walkStatuses.full()) ai.walkStatuses.push_back(index);
				if(selfStatus && stateID != ActionStateID::INVALID && stateID == ai.lowHpState && !ai.lowHpStatuses.full()) ai.lowHpStatuses.push_back(index);
			}
		}
	}
	LOG("Loaded AI of %d monsters (%d spawn statuses, %d death remotes)", (i32)monsterAi.size(), spawnStatusCount, dieRemoteCount);
	return true;
}

bool GameXmlContent::LoadTitan()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/TitanTransform.xml", doc, true)) return true;
	XMLElement* root = doc.FirstChildElement();
	titan.cost = root->IntAttribute("cost");
	titan.votePeriod = root->FloatAttribute("votePeriod");
	titan.teamCoolTime = root->FloatAttribute("teamCoolTime");
	titan.failPlayerCool = root->IntAttribute("failPlayerCool");
	titan.endTeamCool = root->IntAttribute("endTeamCool");
	titan.endPlayerCool = root->IntAttribute("endPlayerCool");
	if(const char* giants = root->Attribute("giants")) {
		for(const char* c = giants; *c && !titan.giants.full(); ) {
			char* end = nullptr;
			const long v = strtol(c, &end, 10);
			if(end == c) break;
			titan.giants.push_back((i32)v);
			c = end;
		}
	}
	for(XMLElement* f = root->FirstChildElement("FORM"); f && !titan.forms.full(); f = f->NextSiblingElement("FORM")) {
		TitanForm form;
		form.doc = f->IntAttribute("doc");
		form.duration = f->FloatAttribute("duration");
		form.weapon = f->IntAttribute("weapon");
		form.skill1 = (i32)f->UnsignedAttribute("skill1");
		form.skill2 = (i32)f->UnsignedAttribute("skill2");
		form.ug = (i32)f->UnsignedAttribute("ug");
		form.moveSpeed = f->FloatAttribute("moveSpeed");
		for(XMLElement* st = f->FirstChildElement("STAT"); st && !form.stats.full(); st = st->NextSiblingElement("STAT")) form.stats.push_back({ (u8)st->IntAttribute("type"), st->FloatAttribute("value") });
		titan.forms.push_back(form);
	}
	LOG("Loaded %d titan forms (cost %d)", (i32)titan.forms.size(), titan.cost);
	return true;
}

bool GameXmlContent::LoadMonsterStats()
{
	static const wchar* STAT_FILES[] = { L"/MonsterStats.xml", L"/MonsterStatsEstimated.xml" };
	for(i32 fileIndex = 0; fileIndex < (i32)ARRAY_COUNT(STAT_FILES); fileIndex++) {
		const bool estimated = fileIndex > 0;
		XMLDocument doc;
		if(!LoadXMLFile(STAT_FILES[fileIndex], doc, estimated)) {
			if(estimated) continue;
			return false;
		}
		if(!estimated) {
			monsterGrowthPeriod = doc.FirstChildElement()->FloatAttribute("growthPeriod");
			monsterGrowthCap = doc.FirstChildElement()->IntAttribute("growthCap");
		}
		for(XMLElement* pInfo = doc.FirstChildElement()->FirstChildElement("INFO"); pInfo; pInfo = pInfo->NextSiblingElement("INFO")) {
			const i32 id = pInfo->IntAttribute("ID");
			if(estimated && monsterStats.find(id) != monsterStats.end()) continue;
			auto& stats = monsterStats[id];
			if(const i32 drop = pInfo->IntAttribute("drop")) monsterDrop[id] = drop;
			if(const f32 life = pInfo->FloatAttribute("lifeTime")) monsterLifeTime[id] = life;
			if(const char* spawns = pInfo->Attribute("dieSpawn")) {
				auto& list = monsterDieSpawn[id];
				for(const char* c = spawns; *c && !list.full(); ) {
					char* end = nullptr;
					const long v = strtol(c, &end, 10);
					if(end == c) break;
					list.push_back((i32)v);
					c = end;
				}
			}
			for(XMLElement* st = pInfo->FirstChildElement("STAT"); st && !stats.full(); st = st->NextSiblingElement("STAT")) {
				stats.push_back({ (u8)st->IntAttribute("type"), st->FloatAttribute("value") });
				const f32 growth = st->FloatAttribute("growth");
				if(growth != 0 && !monsterGrowth[id].full()) monsterGrowth[id].push_back({ (u8)st->IntAttribute("type"), growth });
			}
			if(!estimated) continue;
			static const StatValue MEASURED_SPAWN_STAT_DEFAULTS[] = { { 7, 113.333f }, { 9, 5.0f }, { 10, 150.0f }, { 13, 100.0f }, { 14, 100.0f }, { 15, 100.0f }, { 52, 100.0f }, { 64, 150.0f } };
			for(const StatValue& def : MEASURED_SPAWN_STAT_DEFAULTS) {
				bool present = false;
				foreach_const(s, stats) if(s->type == def.type) present = true;
				if(!present && !stats.full()) stats.push_back(def);
			}
		}
	}
	LOG("Loaded stats of %d monsters", (i32)monsterStats.size());
	return true;
}

const GameXmlContent::Map* GameXmlContent::FindPvpMap(MapIndex index) const
{
	auto alias = pvpMapAliases.find((i32)index);
	if(alias != pvpMapAliases.end()) index = (MapIndex)alias->second;
	foreach_const(m, pvpMaps) if(m->index == index) return &*m;
	return nullptr;
}

struct MissionSubTypeName
{
	const char* name;
	GameXmlContent::Mission::SubType subType;
};

static const MissionSubTypeName MISSION_SUBTYPE_NAMES[] = {
	{ "MISSION_SUBTYPE_MISSION_ACHIEVE_MISSION", GameXmlContent::Mission::SubType::ACHIEVE_MISSION },
	{ "MISSION_SUBTYPE_ALL_GET_GOLD", GameXmlContent::Mission::SubType::GET_GOLD },
	{ "MISSION_SUBTYPE_ALL_GET_EXP", GameXmlContent::Mission::SubType::GET_EXP },
	{ "MISSION_SUBTYPE_ALL_MASTER_SORTIE", GameXmlContent::Mission::SubType::MASTER_SORTIE },
	{ "MISSION_SUBTYPE_PVP_MISSION_PLAY", GameXmlContent::Mission::SubType::PVP_PLAY },
	{ "MISSION_SUBTYPE_PVP_WIN_TOTAL", GameXmlContent::Mission::SubType::PVP_WIN },
	{ "MISSION_SUBTYPE_PVP_KILL_TOTAL", GameXmlContent::Mission::SubType::PVP_KILL },
	{ "MISSION_SUBTYPE_PVP_ASSIST_COUNT", GameXmlContent::Mission::SubType::PVP_ASSIST },
	{ "MISSION_SUBTYPE_PVP_KILL_STREAK", GameXmlContent::Mission::SubType::PVP_KILL_STREAK },
	{ "MISSION_SUBTYPE_PVP_FIRST_BLOOD", GameXmlContent::Mission::SubType::PVP_FIRST_BLOOD },
	{ "MISSION_SUBTYPE_PVP_GIVE_DAMAGE_TOTAL", GameXmlContent::Mission::SubType::PVP_DAMAGE },
	{ "MISSION_SUBTYPE_PVP_TITAN_SCORE", GameXmlContent::Mission::SubType::PVP_TITAN_SCORE },
	{ "MISSION_SUBTYPE_PVP_TITAN_SCORE_TOTAL", GameXmlContent::Mission::SubType::PVP_TITAN_SCORE_TOTAL },
	{ "MISSION_SUBTYPE_PVE_MISSION_RANK", GameXmlContent::Mission::SubType::PVE_MISSION_RANK },
	{ "MISSION_SUBTYPE_PVE_STAGE_CLEAR_NO_DIE", GameXmlContent::Mission::SubType::PVE_STAGE_CLEAR_NO_DIE },
	{ "MISSION_SUBTYPE_PVE_KILL_TOTAL", GameXmlContent::Mission::SubType::PVE_KILL_TOTAL },
	{ "MISSION_SUBTYPE_PVE_RANDOM_STAGE_CLEAR", GameXmlContent::Mission::SubType::PVE_RANDOM_STAGE_CLEAR },
	{ "MISSION_SUBTYPE_PVE_STAGE_CLEAR_WITH_COMRADE", GameXmlContent::Mission::SubType::PVE_STAGE_CLEAR_WITH_COMRADE },
};

static const char* GAME_TYPE_NAMES[] = {
	"GAME_TYPE_INVALID", "GAME_TYPE_PVE_NORMAL", "GAME_TYPE_PVE_RANDOM", "GAME_TYPE_PVE_TICKET", "GAME_TYPE_PVP_NORMAL",
	"GAME_TYPE_PVP_RANK", "GAME_TYPE_PVP_AI", "GAME_TYPE_PVP_NORMAL_FFA", "GAME_TYPE_PVP_GUILD", "GAME_TYPE_PVP_FAKE_AI",
};

static GameXmlContent::Mission::Category MissionCategoryFromName(const char* cat)
{
	if(cat && EA::StdC::Strcmp(cat, "DAILY") == 0) return GameXmlContent::Mission::Category::DAILY;
	if(cat && EA::StdC::Strcmp(cat, "WEEKLY") == 0) return GameXmlContent::Mission::Category::WEEKLY;
	if(cat && EA::StdC::Strcmp(cat, "MONTHLY") == 0) return GameXmlContent::Mission::Category::MONTHLY;
	if(cat && EA::StdC::Strcmp(cat, "GUILD") == 0) return GameXmlContent::Mission::Category::GUILD;
	return GameXmlContent::Mission::Category::OTHER;
}

bool GameXmlContent::LoadMissions()
{
	struct RewardInfo
	{
		eastl::fixed_string<char,32,false> key;
		Mission::RewardType type;
		i64 amount;
		i32 item;
	};
	eastl::fixed_vector<RewardInfo,128,false> rewards;
	XMLDocument docReward;
	if(!LoadXMLFile(L"/Design/GAMEINFO/ServiceReward.xml", docReward)) return false;
	XMLElement* pRewardRoot = docReward.FirstChildElement()->FirstChildElement("ST_SERVICEREWARD");
	ASSERT(pRewardRoot);
	XMLElement* pMissionRewards = pRewardRoot->FirstChildElement("_MissionRewardInfo");
	ASSERT(pMissionRewards);
	for(XMLElement* pReward = pMissionRewards->FirstChildElement("_RewardInfo"); pReward; pReward = pReward->NextSiblingElement("_RewardInfo")) {
		RewardInfo info;
		info.item = pReward->IntAttribute("_ItemIndex", 0);
		const char* key = pReward->Attribute("DATAKEY");
		const char* type = pReward->Attribute("_Type");
		if(!key || !type) continue;
		info.key = key;
		info.type = Mission::RewardType::NONE;
		if(EA::StdC::Strcmp(type, "REWARD_TYPE_GOLD") == 0) info.type = Mission::RewardType::GOLD;
		else if(EA::StdC::Strcmp(type, "REWARD_TYPE_EXP") == 0) info.type = Mission::RewardType::EXP;
		else if(EA::StdC::Strcmp(type, "REWARD_TYPE_CCOIN") == 0) info.type = Mission::RewardType::CCOIN;
		else if(EA::StdC::Strcmp(type, "REWARD_TYPE_ITEM") == 0) info.type = Mission::RewardType::ITEM;
		else if(EA::StdC::Strcmp(type, "REWARD_TYPE_GUILDFUND") == 0) info.type = Mission::RewardType::GUILDFUND;
		info.amount = pReward->Int64Attribute("_Amount", 0);
		ASSERT(!rewards.full());
		rewards.push_back(info);
	}

	if(XMLElement* pActivity = pRewardRoot->FirstChildElement("_ActivityPointInfo")) {
		activityMaxPoint = pActivity->IntAttribute("_MaxPoint", 0);
		for(XMLElement* pRatio = pActivity->FirstChildElement("_PointRatioInfo"); pRatio; pRatio = pRatio->NextSiblingElement("_PointRatioInfo")) {
			const i32 tier = pRatio->IntAttribute("DATAKEY", 0);
			if(tier <= 0 || tier > (i32)activityTiers.capacity()) continue;
			if((i32)activityTiers.size() < tier) activityTiers.resize(tier);
			activityTiers[tier - 1].ratio = pRatio->FloatAttribute("DATA", 0);
		}
		for(XMLElement* pTier = pActivity->FirstChildElement("_RewardInfo"); pTier; pTier = pTier->NextSiblingElement("_RewardInfo")) {
			const i32 tier = pTier->IntAttribute("DATAKEY", 0);
			if(tier <= 0 || tier > (i32)activityTiers.size()) continue;
			activityTiers[tier - 1].item = pTier->IntAttribute("_ItemIndex", 0);
			activityTiers[tier - 1].amount = pTier->IntAttribute("_Amount", 0);
		}
	}

	XMLDocument docPrice;
	if(!LoadXMLFile(L"/Design/GAMEINFO/ServicePrice.xml", docPrice)) return false;
	XMLElement* pPrice = docPrice.FirstChildElement()->FirstChildElement("ST_SERVICEPRICE");
	ASSERT(pPrice);
	for(XMLElement* pCost = pPrice->FirstChildElement("_InvenUnlockCost"); pCost; pCost = pCost->NextSiblingElement("_InvenUnlockCost")) {
		const i32 step = pCost->IntAttribute("DATAKEY", 0);
		if(step <= 0 || step > (i32)inventoryExpandCosts.capacity()) continue;
		if((i32)inventoryExpandCosts.size() < step) inventoryExpandCosts.resize(step);
		ServiceCost& cost = inventoryExpandCosts[step - 1];
		const char* type = pCost->Attribute("_Type");
		cost.buyType = type && EA::StdC::Strcmp(type, "BUY_TYPE_CCOIN") == 0 ? 2 : 1;
		cost.amount = pCost->IntAttribute("_Amount", 0);
	}

	LoadSections();
	constexpr eastl::hash<const char*> hashSection;
	static const char* DIFFICULTY_NAMES[] = { "EASY", "NORMAL", "HARD", "EXTREME", "HELL" };
	XMLDocument doc;
	if(!LoadXMLFile(L"/Design/DOCUMENT/MISSION.xml", doc)) return false;
	for(XMLElement* pInfo = doc.FirstChildElement()->FirstChildElement("INFO"); pInfo; pInfo = pInfo->NextSiblingElement("INFO")) {
		XMLElement* pMission = pInfo->FirstChildElement("ST_MISSION");
		if(!pMission) continue;
		Mission mission;
		if(pInfo->QueryIntAttribute("ID", &mission.ID) != XML_SUCCESS) continue;
		mission.step = pMission->IntAttribute("_Step", 0);
		mission.grade = (mission.ID / 10000) % 10;
		mission.dayRestricted = false;
		if(XMLElement* pData = pMission->FirstChildElement("_MISSION_DATA")) {
			for(i32 day = 1; day <= 7; day++) {
				char attr[8];
				EA::StdC::Snprintf(attr, sizeof(attr), "_Day%d", day);
				const char* value = pData->Attribute(attr);
				if(value && EA::StdC::Stricmp(value, "FALSE") == 0) mission.dayRestricted = true;
			}
		}
		mission.category = MissionCategoryFromName(pMission->Attribute("_Category"));
		const char* subType = pMission->Attribute("_SubType");
		for(const MissionSubTypeName& name : MISSION_SUBTYPE_NAMES) {
			if(subType && EA::StdC::Strcmp(subType, name.name) == 0) mission.subType = name.subType;
		}
		if(XMLElement* pData = pMission->FirstChildElement("_MISSION_DATA")) {
			mission.count = pData->Int64Attribute("_count", 0);
			mission.value = pData->Int64Attribute("_value", 0);
			const char* section = pData->Attribute("_SectionKey");
			if(section && EA::StdC::Strcmp(section, "AREA_ALL") == 0) mission.section = Mission::Section::ALL;
			else if(section && EA::StdC::Strcmp(section, "AREA_PVP_ALL") == 0) mission.section = Mission::Section::PVP_ALL;
			else if(section && EA::StdC::Strcmp(section, "AREA_PVP_TITAN") == 0) mission.section = Mission::Section::PVP_TITAN;
			else if(section) {
				auto found = sectionByName.find(hashSection(section));
				if(found != sectionByName.end()) {
					mission.section = Mission::Section::STAGES;
					mission.sectionIndex = found->second;
				}
			}
			mission.rank = ClearRankFromName(pData->Attribute("_Rank"));
			if(const char* difficulty = pData->Attribute("_Difficulty")) {
				for(u8 d = 0; d < ARRAY_COUNT(DIFFICULTY_NAMES); d++) if(EA::StdC::Strcmp(difficulty, DIFFICULTY_NAMES[d]) == 0) mission.minDifficultyBit = (u8)(1 << d);
			}
			const char* gameType = pData->Attribute("_GameType");
			for(i32 t = 0; t < (i32)ARRAY_COUNT(GAME_TYPE_NAMES); t++) {
				if(gameType && EA::StdC::Strcmp(gameType, GAME_TYPE_NAMES[t]) == 0) mission.gameType = (GameType)t;
			}
			const char* reward = pData->Attribute("_Reward");
			foreach_const(r, rewards) {
				if(reward && r->key.compare(reward) == 0) {
					mission.rewardType = r->type;
					mission.rewardAmount = r->amount;
					mission.rewardItem = r->item;
				}
			}
			for(XMLElement* pReq = pData->FirstChildElement("_RequiredList"); pReq; pReq = pReq->NextSiblingElement("_RequiredList")) {
				const char* req = pReq->Attribute("DATA");
				if(!req) continue;
				auto found = masterClassStringMap.find(strHash(req));
				if(found != masterClassStringMap.end()) {
					if(!mission.requiredClasses.full()) mission.requiredClasses.push_back(found->second->classType);
				}
				else mission.requiredCategory = MissionCategoryFromName(req);
			}
		}
		ASSERT(!missions.full());
		missions.push_back(mission);
	}
	LOG("Loaded %d missions", (i32)missions.size());
	return true;
}

const GameXmlContent::ItemInfo* GameXmlContent::FindItemInfo(i32 ID) const
{
	auto found = itemInfoMap.find(ID);
	return found == itemInfoMap.end() ? nullptr : &found->second;
}

const GameXmlContent::Mission* GameXmlContent::FindMission(i32 ID) const
{
	foreach_const(m, missions) if(m->ID == ID) return &*m;
	return nullptr;
}

bool GameXmlContent::LoadExpInfo()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/Design/GAMEINFO/ExpInfo.xml", doc)) return false;
	XMLElement* pTable = doc.FirstChildElement()->FirstChildElement("ST_EXP_INFO");
	ASSERT(pTable);
	for(XMLElement* pLevel = pTable->FirstChildElement("EXP_INFO"); pLevel; pLevel = pLevel->NextSiblingElement("EXP_INFO")) {
		i32 level = 0;
		if(pLevel->QueryIntAttribute("DATAKEY", &level) != XML_SUCCESS || level <= 0) continue;
		ASSERT(level < (i32)expLevels.capacity());
		if((i32)expLevels.size() <= level) expLevels.resize(level + 1);
		ExpLevel& info = expLevels[level];
		pLevel->QueryUnsignedAttribute("_RequiredExp", &info.required);
		for(XMLElement* pReward = pLevel->FirstChildElement("_Reward"); pReward; pReward = pReward->NextSiblingElement("_Reward")) {
			const char* type = pReward->Attribute("_Type");
			if(type && EA::StdC::Strcmp(type, "GOLD") == 0) info.gold += pReward->UnsignedAttribute("_Amount", 0);
		}
	}
	LOG("Loaded %d account levels", (i32)expLevels.size() - 1);
	return true;
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
			static const char* STAGE_INDEX_ATTRIBUTES[] = { "_NormalStageIndex", "_EasyStageIndex", "_HardStageIndex", "_ExtremeStageIndex", "_HellStageIndex" };
			i32 stageID = 0;
			for(const char* attribute : STAGE_INDEX_ATTRIBUTES) {
				i32 difficultyStage = 0;
				if(pStage->QueryIntAttribute(attribute, &difficultyStage) != XML_SUCCESS || difficultyStage == 0) continue;
				if(!area.allStages.full()) area.allStages.push_back(StageIndex(difficultyStage));
				if(stageID == 0) stageID = difficultyStage;
			}
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

bool GameXmlContent::FindEntryStageArea(i32 entryID, StageIndex stage, AreaIndex* outAreaIndex) const
{
	foreach_const(entry, entrySystems) {
		if(entry->ID != entryID) continue;
		for(int pass = 0; pass < 2; pass++) {
			const auto& areas = pass == 0 ? entry->areas : entry->scheduleAreas;
			foreach_const(areaID, areas) {
				foreach_const(area, areaStages) {
					if(area->ID != *areaID) continue;
					if(eastl::find(area->allStages.begin(), area->allStages.end(), stage) == area->allStages.end()) continue;
					*outAreaIndex = area->ID;
					return true;
				}
			}
		}
		return false;
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

static GameModeType GameModeFromString(const char* name)
{
	static const char* GAME_MODE_NAMES[] = {
		"GAME_MODE_STAGE", "GAME_MODE_DEFENCE", "GAME_MODE_DEATH_MATCH", "GAME_MODE_GOT", "GAME_MODE_OCCUPY",
		"GAME_MODE_SPORTS", "GAME_MODE_TEST", "GAME_MODE_MASTER_TRAINING", "GAME_MODE_RANDOM_ARENA",
	};
	for(i32 m = 0; m < (i32)ARRAY_COUNT(GAME_MODE_NAMES); m++) {
		if(name && EA::StdC::Strcmp(name, GAME_MODE_NAMES[m]) == 0) return (GameModeType)m;
	}
	return GameModeType::GAME_MODE_INVALID;
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
		const char* gameMode = pStage->Attribute("_GameModeType");
		stage.titan = gameMode && EA::StdC::Strcmp(gameMode, "GAME_MODE_GOT") == 0;
		stage.gameMode = GameModeFromString(gameMode);
		i32 stageSkill = 0;
		if(pStage->QueryIntAttribute("_StageSkillIndex", &stageSkill) == XML_SUCCESS && stageSkill != 0) stage.stageSkill = SkillID(stageSkill);
		for(XMLElement* pC = pStage->FirstChildElement("_StageSkillCandidateList"); pC; pC = pC->NextSiblingElement("_StageSkillCandidateList")) {
			const i32 candidate = pC->IntAttribute("DATA");
			if(candidate != 0 && !stage.stageSkillCandidates.full()) stage.stageSkillCandidates.push_back(SkillID(candidate));
		}
		static const char* DIFFICULTY_NAMES[] = { "WORLD_STAGE_DIFFICULT_EASY", "WORLD_STAGE_DIFFICULT_NORMAL", "WORLD_STAGE_DIFFICULT_HARD", "WORLD_STAGE_DIFFICULT_EXTREME", "WORLD_STAGE_DIFFICULT_HELL" };
		const char* difficulty = pStage->Attribute("_Difficult");
		for(i32 d = 0; d < (i32)ARRAY_COUNT(DIFFICULTY_NAMES); d++) {
			if(difficulty && EA::StdC::Strcmp(difficulty, DIFFICULTY_NAMES[d]) == 0) stage.difficultyBit = (u8)(1 << d);
		}
		if(XMLElement* pSuccess = pStage->FirstChildElement("_SUCCESS_CONDITION_LIST")) {
			for(XMLElement* pSet = pSuccess->FirstChildElement("_MCASESET"); pSet; pSet = pSet->NextSiblingElement("_MCASESET")) {
				for(XMLElement* pTrig = pSet->FirstChildElement("_MTRIGGER"); pTrig; pTrig = pTrig->NextSiblingElement("_MTRIGGER")) {
					const char* type = pTrig->Attribute("_Type");
					if(type && EA::StdC::Strcmp(type, "E_TRIGGER_TYPE_MONEY_ADD") == 0) stage.moneyReward = pTrig->IntAttribute("_Value");
				}
				for(XMLElement* pCond = pSet->FirstChildElement("_MCONDITION"); pCond; pCond = pCond->NextSiblingElement("_MCONDITION")) {
					const char* type = pCond->Attribute("_Type");
					if(type && EA::StdC::Strcmp(type, "E_CONDITION_TYPE_DYNAMIC_KILL") == 0) stage.defendTarget = pCond->IntAttribute("_Index");
					if(!type || EA::StdC::Strcmp(type, "E_CONDITION_TYPE_MONSTER_KILL") != 0) continue;
					stage.clearMonster = pCond->IntAttribute("_Index");
					stage.clearCount = pCond->IntAttribute("_Value", 1);
				}
			}
		}
		if(XMLElement* pDrop = pStage->FirstChildElement("_COMMON_DROP")) {
			static const char* RANK_PREFIX = "CREATURE_RANK_DETAIL_";
			const size_t prefixLen = EA::StdC::Strlen(RANK_PREFIX);
			for(XMLElement* d = pDrop->FirstChildElement("_DROPINDEX"); d && !stage.commonDrop.full(); d = d->NextSiblingElement("_DROPINDEX")) {
				const char* rank = d->Attribute("_CreatureRankDetail");
				if(!rank || EA::StdC::Strncmp(rank, RANK_PREFIX, prefixLen) != 0) continue;
				stage.commonDrop.push_back(eastl::make_pair(RankID(rank + prefixLen), d->IntAttribute("_DropItemIndex")));
			}
		}
		if(XMLElement* pView = pStage->FirstChildElement("_VIEW_REWARD")) {
			for(XMLElement* v = pView->FirstChildElement("_VIEWINDEX"); v && !stage.viewRewards.full(); v = v->NextSiblingElement("_VIEWINDEX")) stage.viewRewards.push_back(v->IntAttribute("_ViewItemIndex"));
		}
		if(XMLElement* pPvp = pStage->FirstChildElement("_PVP")) {
			XMLElement* pAi = pPvp->FirstChildElement("_MODE") ? pPvp->FirstChildElement("_MODE")->FirstChildElement("_AI") : nullptr;
			XMLElement* pDefault = pAi ? pAi->FirstChildElement("_Default") : nullptr;
			for(XMLElement* pIgnore = pDefault ? pDefault->FirstChildElement("_IgnoreMapIndex") : nullptr; pIgnore && !stage.aiIgnoredMaps.full(); pIgnore = pIgnore->NextSiblingElement("_IgnoreMapIndex")) {
				stage.aiIgnoredMaps.push_back(MapIndex(pIgnore->IntAttribute("DATA")));
			}
		}
		XMLElement* pWorldMap = pStage->FirstChildElement("_WORLD_STAGE_MAP");
		if(!pWorldMap) continue;
		for(XMLElement* pGroup = pWorldMap->FirstChildElement("_WORLD_STAGE_MAP_GROUP");
			pGroup;
			pGroup = pGroup->NextSiblingElement("_WORLD_STAGE_MAP_GROUP")) {
			if(stage.bossMap == MapIndex(0)) stage.bossMap = MapIndex(pGroup->IntAttribute("_BossMapIndex"));
			for(XMLElement* pMap = pGroup->FirstChildElement("_WORLD_STAGE_MAPINFO");
				pMap;
				pMap = pMap->NextSiblingElement("_WORLD_STAGE_MAPINFO")) {
				i32 mapID = 0;
				if(pMap->QueryIntAttribute("_Index", &mapID) != XML_SUCCESS) continue;
				if(mapID != 0 && stage.maps.size() < stage.maps.capacity()) {
					stage.maps.push_back(MapIndex(mapID));
					stage.mapRates.push_back(pMap->IntAttribute("_Rate", 1));
				}
			}
		}
		if(!stage.maps.empty() && stageMaps.size() < stageMaps.capacity()) stageMaps.push_back(stage);
	}

	LOG("Loaded %d stage maps", (i32)stageMaps.size());
	return true;
}

bool GameXmlContent::PickStageMap(StageIndex stageID, bool aiMode, MapIndex* outMapIndex) const
{
	for(auto& stage : stageMaps) {
		if(stage.ID != stageID) continue;
		eastl::fixed_vector<eastl::pair<MapIndex,i32>,8,false> usable;
		i32 total = 0;
		for(i32 i = 0; i < (i32)stage.maps.size(); i++) {
			const MapIndex m = stage.maps[i];
			bool ignored = false;
			if(aiMode) foreach_const(ig, stage.aiIgnoredMaps) if(*ig == m) ignored = true;
			if(ignored || (stage.gameMode != GameModeType::GAME_MODE_STAGE && stage.gameMode != GameModeType::GAME_MODE_DEFENCE && !FindPvpMap(m))) continue;
			if(stage.gameMode == GameModeType::GAME_MODE_STAGE) { *outMapIndex = m; return true; }
			const i32 rate = MAX(i < (i32)stage.mapRates.size() ? stage.mapRates[i] : 1, 1);
			usable.push_back(eastl::make_pair(m, rate));
			total += rate;
		}
		if(usable.empty()) return FindStageMap(stageID, outMapIndex);
		i32 roll = (i32)(RandUint() % (u32)total);
		foreach_const(u, usable) {
			if(roll < u->second) { *outMapIndex = u->first; return true; }
			roll -= u->second;
		}
		*outMapIndex = usable.back().first;
		return true;
	}
	return false;
}

bool GameXmlContent::FindStageMap(StageIndex stageID, MapIndex* outMapIndex) const
{
	for(auto& stage : stageMaps) {
		if(stage.ID != stageID) continue;
		for(auto mapID : stage.maps) {
			if(!IsMapLoadable(mapID)) continue;
			*outMapIndex = mapID;
			return true;
		}
		if(stage.maps.empty()) return false;
		*outMapIndex = stage.maps[0];
		return true;
	}
	return false;
}

bool GameXmlContent::IsMapLoadable(MapIndex mapID) const
{
	return FindPvpMap(mapID) != nullptr;
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

i32 GameXmlContent::WeaponIndexFor(const Master& master, u32 type, u32 grade) const
{
	static const i32 MEASURED_WEAPON_INDEX_BASE = 131100000;
	if(type < 1 || type > 9) return -1;
	const i32 doc = MEASURED_WEAPON_INDEX_BASE + (i32)master.classType * 1000 + (i32)type * 10 + (i32)grade + 1;
	foreach_const(w, master.weaponIDs) if((i32)*w == doc) return doc;
	return -1;
}

bool GameXmlContent::LoadServicePrice()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/Design/GAMEINFO/ServicePrice.xml", doc)) return true;
	XMLElement* price = doc.FirstChildElement() ? doc.FirstChildElement()->FirstChildElement("ST_SERVICEPRICE") : nullptr;
	if(!price) return true;
	auto readCost = [](XMLElement* e) {
		PriceCost c;
		const char* type = e ? e->Attribute("_Type") : nullptr;
		c.buyType = type && EA::StdC::Strcmp(type, "BUY_TYPE_CCOIN") == 0 ? 2 : 1;
		c.amount = e ? e->IntAttribute("_Amount") : 0;
		return c;
	};
	for(XMLElement* e = price->FirstChildElement("_SkillUnlockInfo"); e; e = e->NextSiblingElement("_SkillUnlockInfo")) {
		const i32 order = e->IntAttribute("_UnlockOrder");
		if(order >= 0 && order < (i32)skillUnlockCost.size()) skillUnlockCost[order] = readCost(e->FirstChildElement("_Cost"));
	}
	for(XMLElement* e = price->FirstChildElement("_WeaponUnlockInfo"); e; e = e->NextSiblingElement("_WeaponUnlockInfo")) {
		const char* type = e->Attribute("_WeaponType");
		i32 t = 0;
		if(type) EA::StdC::Sscanf(type, "WEAPON_TYPE_%d", &t);
		if(t >= 0 && t < (i32)weaponUnlockCost.size()) weaponUnlockCost[t] = readCost(e->FirstChildElement("_Cost"));
	}
	for(XMLElement* e = price->FirstChildElement("_PvESkillUpgradeCost"); e; e = e->NextSiblingElement("_PvESkillUpgradeCost")) {
		const i32 level = e->IntAttribute("DATAKEY");
		if(level < 1 || level > (i32)pveSkillUpgradeCost.capacity()) continue;
		if((i32)pveSkillUpgradeCost.size() < level) pveSkillUpgradeCost.resize(level);
		pveSkillUpgradeCost[level - 1] = readCost(e);
	}
	static const char* DIFFICULTY_NAMES[] = { "WORLD_STAGE_DIFFICULT_EASY", "WORLD_STAGE_DIFFICULT_NORMAL", "WORLD_STAGE_DIFFICULT_HARD", "WORLD_STAGE_DIFFICULT_EXTREME", "WORLD_STAGE_DIFFICULT_HELL" };
	for(XMLElement* e = price->FirstChildElement("_RewardBuyChanceCostSteps"); e; e = e->NextSiblingElement("_RewardBuyChanceCostSteps")) {
		const char* key = e->Attribute("DATAKEY");
		const char* data = e->Attribute("DATA");
		for(i32 d = 0; d < (i32)ARRAY_COUNT(DIFFICULTY_NAMES) && key && data; d++) {
			if(EA::StdC::Strcmp(key, DIFFICULTY_NAMES[d]) != 0) continue;
			i32 value = 0, read = 0;
			while(*data && !rewardBuyChanceSteps[d].full() && EA::StdC::Sscanf(data, "%d%n", &value, &read) == 1) {
				rewardBuyChanceSteps[d].push_back(value);
				data += read;
				if(*data == ',') data++;
			}
		}
	}
	if(XMLElement* gear = price->FirstChildElement("_MasterGearCostInfo")) {
		for(XMLElement* e = gear->FirstChildElement("_AddCost"); e && !masterGearAddCost.full(); e = e->NextSiblingElement("_AddCost")) masterGearAddCost.push_back(readCost(e));
	}
	return true;
}

i32 GameXmlContent::RewardBuyChanceCost(u8 difficultyBit, i32 bought) const
{
	for(i32 d = 0; d < (i32)rewardBuyChanceSteps.size(); d++) {
		if(difficultyBit != (u8)(1 << d)) continue;
		return bought >= 0 && bought < (i32)rewardBuyChanceSteps[d].size() ? rewardBuyChanceSteps[d][bought] : -1;
	}
	return -1;
}

u8 GameXmlContent::AchievementTier(i32 score) const
{
	u8 tier = 0;
	foreach_const(t, achievementTierPoints) if(score >= t->second && t->first > tier) tier = t->first;
	return tier;
}

bool GameXmlContent::LoadServiceRule()
{
	static const struct { const char* name; GameType type; } GAME_TYPES[] = {
		{ "PVE_NORMAL", GameType::PVE_NORMAL }, { "PVE_RANDOM", GameType::PVE_RANDOM }, { "PVE_TICKET", GameType::PVE_TICKET },
		{ "PVP_NORMAL", GameType::PVP_NORMAL }, { "PVP_RANK", GameType::PVP_RANK }, { "PVP_AI", GameType::PVP_AI },
		{ "PVP_NORMAL_FFA", GameType::PVP_NORMAL_FFA }, { "PVP_GUILD", GameType::PVP_GUILD }, { "PVP_FAKE_AI", GameType::PVP_FAKE_AI },
	};
	gearPowerPercent.fill(100);
	XMLDocument doc;
	if(!LoadXMLFile(L"/client/ServiceRule.xml", doc)) return true;
	XMLElement* rule = doc.FirstChildElement() ? doc.FirstChildElement()->FirstChildElement("ST_SERVICERULE") : nullptr;
	XMLElement* gear = rule ? rule->FirstChildElement("_MasterGearInfo") : nullptr;
	for(XMLElement* p = rule ? rule->FirstChildElement("_AchievementTierPoint") : nullptr; p && !achievementTierPoints.full(); p = p->NextSiblingElement("_AchievementTierPoint")) achievementTierPoints.push_back({ (u8)p->IntAttribute("_Tier"), p->IntAttribute("_MinPoint") });
	for(XMLElement* p = gear ? gear->FirstChildElement("_PvPGearPowerPercent") : nullptr; p; p = p->NextSiblingElement("_PvPGearPowerPercent")) {
		const char* type = p->Attribute("_GameType");
		for(const auto& t : GAME_TYPES) if(type && EA::StdC::Strcmp(type, t.name) == 0) gearPowerPercent[(i32)t.type] = p->IntAttribute("DATA", 100);
	}
	return true;
}

bool GameXmlContent::LoadDefencePhases()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/client/Phase.xml", doc)) return true;
	XMLElement* pInfo = doc.FirstChildElement() ? doc.FirstChildElement()->FirstChildElement("INFO") : nullptr;
	XMLElement* pList = pInfo ? pInfo->FirstChildElement("ST_PHASE_INFO") : nullptr;
	for(XMLElement* p = pList ? pList->FirstChildElement("_Phase") : nullptr; p; p = p->NextSiblingElement("_Phase")) {
		DefencePhase phase;
		phase.step = p->IntAttribute("_StepIndex");
		phase.timeout = p->FloatAttribute("_PhaseTimeOut");
		phase.endDelay = p->FloatAttribute("_PhaseEndDelay");
		phase.killCount = p->IntAttribute("_PhaseKillCount");
		for(XMLElement* e = p->FirstChildElement("_ExcludeKillCount"); e && !phase.excluded.full(); e = e->NextSiblingElement("_ExcludeKillCount")) phase.excluded.push_back(e->IntAttribute("_Index"));
		if(XMLElement* r = p->FirstChildElement("_Reward")) {
			phase.gold = r->UnsignedAttribute("_Gold");
			phase.exp = r->UnsignedAttribute("_Exp");
			phase.guildPoint = r->UnsignedAttribute("_GuildPoint");
			for(XMLElement* it = r->FirstChildElement("_Item"); it && !phase.items.full(); it = it->NextSiblingElement("_Item")) phase.items.push_back(eastl::make_pair(it->IntAttribute("_Index"), it->IntAttribute("_Value", 1)));
		}
		defencePhases.push_back(phase);
	}
	LOG("Loaded %d defence phases", (i32)defencePhases.size());
	return true;
}

bool GameXmlContent::LoadPvpPraises()
{
	static const char* PRAISE_TYPE_NAMES[] = {
		"PRAISE_TYPE_INVALID", "PRAISE_TYPE_KILL_ASSIST", "PRAISE_TYPE_KILL", "PRAISE_TYPE_DAMAGE_PC", "PRAISE_TYPE_MAX_CONTINUOUS_KILL",
		"PRAISE_TYPE_MAX_CONTINUOUS_KILL_TRANSFORM", "PRAISE_TYPE_REFLECTION_KILL", "PRAISE_TYPE_DOT_KILL", "PRAISE_TYPE_HEAL_OTHERS",
		"PRAISE_TYPE_HEAL_SELF", "PRAISE_TYPE_NEVER_DIE", "PRAISE_TYPE_TOTAL_SCORE", "PRAISE_TYPE_DAMAGE_CORE", "PRAISE_TYPE_DEFEAT_APPLY_COUNT",
		"PRAISE_TYPE_KILL_TOWER", "PRAISE_TYPE_OCCUPY_SIGHT", "PRAISE_TYPE_SAVE_PIECE", "PRAISE_TYPE_KILL_BOSS", "PRAISE_TYPE_STEAL_BOSS",
		"PRAISE_TYPE_KILL_TANIANKING", "PRAISE_TYPE_KILL_TRANSFORMCH", "PRAISE_TYPE_KILL_TRANSFORMREADYCH",
	};
	static const char* PRAISE_RELATION_NAMES[] = {
		"PRAISE_RELATION_TYPE_INVALID", "PRAISE_RELATION_TYPE_KILL_ASSIST", "PRAISE_RELATION_TYPE_KILL_ASSIST_RATIO", "PRAISE_RELATION_TYPE_DAMAGE_PC",
		"PRAISE_RELATION_TYPE_DAMAGE_PC_RATIO", "PRAISE_RELATION_TYPE_REFLECTION_COUNT", "PRAISE_RELATION_TYPE_HEAL_OTHERS_PER_SECOND",
		"PRAISE_RELATION_TYPE_HEAL_SELF_PER_SECOND", "PRAISE_RELATION_TYPE_SCORE_RATIO", "PRAISE_RELATION_TYPE_DAMAGE_CORE_RATIO",
		"PRAISE_RELATION_TYPE_DEFEAT_APPLY_COUNT", "PRAISE_RELATION_TYPE_DAMAGE_TOWER", "PRAISE_RELATION_TYPE_SAVE_PIECE_RATIO",
	};
	static_assert(ARRAY_COUNT(PRAISE_TYPE_NAMES) == (size_t)PraiseType::COUNT, "praise type names");
	static_assert(ARRAY_COUNT(PRAISE_RELATION_NAMES) == (size_t)PraiseRelationType::COUNT, "praise relation names");
	XMLDocument doc;
	if(!LoadXMLFile(L"/client/PvpPraise.xml", doc)) return true;
	XMLElement* pInfo = doc.FirstChildElement() ? doc.FirstChildElement()->FirstChildElement("INFO") : nullptr;
	XMLElement* pTable = pInfo ? pInfo->FirstChildElement("ST_PVP_PRAISE_TABLE") : nullptr;
	for(XMLElement* p = pTable ? pTable->FirstChildElement("_PVP_PRAISE_INFO") : nullptr; p && !pvpPraises.full(); p = p->NextSiblingElement("_PVP_PRAISE_INFO")) {
		PvpPraise praise;
		praise.gameMode = GameModeFromString(p->Attribute("gameModeType"));
		praise.priority = p->IntAttribute("priority");
		praise.minValue = p->FloatAttribute("minValue");
		const char* type = p->Attribute("praiseType");
		const char* relation = p->Attribute("relationType");
		for(i32 t = 0; t < (i32)ARRAY_COUNT(PRAISE_TYPE_NAMES); t++) if(type && EA::StdC::Strcmp(type, PRAISE_TYPE_NAMES[t]) == 0) praise.type = (PraiseType)t;
		for(i32 r = 0; r < (i32)ARRAY_COUNT(PRAISE_RELATION_NAMES); r++) if(relation && EA::StdC::Strcmp(relation, PRAISE_RELATION_NAMES[r]) == 0) praise.relation = (PraiseRelationType)r;
		if(praise.type == PraiseType::INVALID) {
			WARN("PvpPraise :: unknown praise type '%s'", type ? type : "");
			continue;
		}
		auto at = pvpPraises.begin();
		while(at != pvpPraises.end() && at->priority <= praise.priority) ++at;
		pvpPraises.insert(at, praise);
	}
	LOG("Loaded %d pvp praises", (i32)pvpPraises.size());
	return true;
}

bool GameXmlContent::LoadStageBots()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/Design/DOCUMENT/BOT.xml", doc)) return true;
	for(XMLElement* pInfo = doc.FirstChildElement()->FirstChildElement("INFO"); pInfo; pInfo = pInfo->NextSiblingElement("INFO")) {
		XMLElement* pTable = pInfo->FirstChildElement("ST_BOT_TABLE");
		if(!pTable) continue;
		auto& classes = stageBotClasses[pTable->IntAttribute("_StageIndex")];
		for(XMLElement* pBot = pTable->FirstChildElement("_BOT_INFO"); pBot && !classes.full(); pBot = pBot->NextSiblingElement("_BOT_INFO")) {
			const char* className = pBot->Attribute("_Class");
			if(!className) continue;
			auto found = masterClassStringMap.find(strHash(className));
			if(found != masterClassStringMap.end()) classes.push_back(found->second->classType);
		}
	}
	LOG("Loaded %d stage bot tables", (i32)stageBotClasses.size());
	return true;
}

CreatureIndex GameXmlContent::FindDeathMatchBotIndex(ClassType classType) const
{
	auto found = deathMatchBotIndex.find(classType);
	if(found == deathMatchBotIndex.end()) return CreatureIndex::Invalid;
	return found->second;
}


static u8 MeasuredGuildSkillType(const char* key)
{
	static const struct { const char* key; u8 type; } TYPES[] = {
		{ "GUILD_MEMBER_MAX_UP", 1 },
		{ "STAGE_POINT_LIMIT_UP", 2 },
		{ "ARENA_POINT_LIMIT_UP", 3 },
		{ "GAIN_GOLD_UP", 4 },
		{ "GAIN_XP_UP", 5 },
		{ "GUILD_MISSION", 6 },
		{ "GUILD_EMBLEM", 7 },
		{ "LIMITED_REWARD_SLOT_ADD", 21 },
		{ "LIMITED_GAIN_GOLD_UP", 22 },
		{ "LIMITED_GAIN_XP_UP", 23 },
	};
	for(const auto& t : TYPES) if(strcmp(t.key, key) == 0) return t.type;
	return 0;
}

bool GameXmlContent::LoadGuildData()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/Design/GAMEINFO/Guild.xml", doc)) return false;

	XMLElement* pInfo = doc.FirstChildElement()->FirstChildElement("ST_GUILDINFO");
	if(!pInfo) return false;
	guildFoundationLevel = pInfo->IntAttribute("_RequiredLevelForGuildFoundation", guildFoundationLevel);
	guildFoundationGold = pInfo->IntAttribute("_GuildFoundationGold", guildFoundationGold);
	guildTagChangeCost = pInfo->IntAttribute("_GuildTagChangeCost", guildTagChangeCost);
	guildMedalItem = pInfo->IntAttribute("_GuildMedalItemDocIndex", guildMedalItem);
	if(XMLElement* pMembership = pInfo->FirstChildElement("_GuildMembership")) guildMembershipCapacity = pMembership->IntAttribute("_Capacity", guildMembershipCapacity);
	if(XMLElement* pDonation = pInfo->FirstChildElement("_GuildDonation")) {
		guildDonationUnit = MAX(1, pDonation->IntAttribute("_Unit", guildDonationUnit));
		guildFundExchangeRate = MAX(1, pDonation->IntAttribute("_ExchangeRateForOneFund", guildFundExchangeRate));
	}
	if(XMLElement* pReward = pInfo->FirstChildElement("_GuildPointReward")) guildRollcallPoint = pReward->IntAttribute("_RollcallPoint", guildRollcallPoint);
	if(XMLElement* pBonus = pInfo->FirstChildElement("_GuildMemberBonus")) {
		static const char* KINDS[GUILD_BONUS_COUNT] = { "_Arena", "_Stage", "_Defence" };
		for(i32 k = 0; k < GUILD_BONUS_COUNT; k++) {
			for(XMLElement* e = pBonus->FirstChildElement(KINDS[k]); e; e = e->NextSiblingElement(KINDS[k])) {
				const i32 members = e->IntAttribute("DATAKEY");
				if(members >= 1 && members <= 5) guildMemberBonus[k][members] = e->IntAttribute("DATA");
			}
		}
	}

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
			skill.type = MeasuredGuildSkillType(key);
			skill.limitedHours = pSkill->IntAttribute("_LimitedHours", 0);
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
			pStage->QueryIntAttribute("_Weekend", &guildActivityCapWeekend);
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

bool GameXmlContent::LoadCollisionMeshes()
{
	Path path = gameDataDir;
	PathAppend(path, L"/cylinder.physx_dynamic");
	bool r = FileLoad(&fileCylinderCollision, path.data());
	if(!r) return false;

	return true;
}

u16 GameXmlContent::FindNodeName(const char* name) const
{
	if(!name || !name[0]) return 0;
	eastl::fixed_string<wchar,48,false> w;
	for(const char* c = name; *c && w.size() < 47; c++) w.push_back((wchar)*c);
	for(u16 i = 0; i < (u16)nodeNames.size(); i++) {
		if(nodeNames[i] == w) return i;
	}
	return 0;
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
	XMLDocument xmlClassActions;
	if(LoadXMLFile(L"/client/ActionBase_Class.xml", xmlClassActions)) {
		for(XMLElement* pShared = xmlClassActions.FirstChildElement()->FirstChildElement(); pShared; pShared = pShared->NextSiblingElement()) {
			XMLElement* copy = pShared->DeepClone(&xmlActionBase)->ToElement();
			copy->SetAttribute("Class", "COMMON");
			xmlActionBase.FirstChildElement()->InsertEndChild(copy);
		}
	}

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
				if(!known) {
					ASSERT(!curAction->anims.full());
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
			cmd.rootDummyDir = cmd.byDummyDir && NodeName && EA::StdC::Strcmp(NodeName, "Scene Root") == 0 ? 1 : 0;
			cmd.lowerDummyDir = P2 && EA::StdC::Strstr(P2, "ByLowerDummyDir") ? 1 : 0;
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
					if(EA::StdC::Sscanf(TargetParam, "%f|%f|%f|%f|%f|%f|%f", &p[0], &p[1], &p[2], &p[3], &p[4], &p[5], &p[6]) == 7 && p[3] <= 0) {
						cmd.remote.offsetSide = -p[5];
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
				cmd.teleport.toSpawnPoint = Param1 && EA::StdC::Strcmp(Param1, "TELEPORT_TARGET_SPAWN_POINT") == 0 ? 1 : 0;
				cmd.teleport.toTarget = Param1 && !TargetPreset && EA::StdC::Strcmp(Param1, "TELEPORT_TARGET_ENTITY") == 0 ? 1 : 0;
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
				const char* Param2 = nullptr;
				pActionBase->QueryStringAttribute("Param2", &Param2);
				cmd.entity.remove = Param2 && EA::StdC::Strcmp(Param2, "Remove") == 0 ? 1 : 0;
				cmd.entity.offsetFront = 0;
				cmd.entity.offsetSide = 0;
				if(Param2 && EA::StdC::Strchr(Param2, '|')) EA::StdC::Sscanf(Param2, "%f|%f", &cmd.entity.offsetFront, &cmd.entity.offsetSide);
			} break;

			case ActionCommand::Type::STATUS:
			case ActionCommand::Type::STATUS_SKILL_TARGET: {
				cmd.status.statusIndex = 0;
				pActionBase->QueryAttribute("StatusIndex", &cmd.status.statusIndex);
				const char* TargetPreset = nullptr;
				pActionBase->QueryStringAttribute("TargetPreset", &TargetPreset);
				cmd.status.targetPreset = TargetPreset ? ActionCommand::TargetPresetFromString(TargetPreset) : ActionCommand::TargetPreset::CURRENT_FFF;
				if(cmd.type == ActionCommand::Type::STATUS_SKILL_TARGET) cmd.status.targetPreset = ActionCommand::TargetPreset::CURRENT_FFF;
				cmd.status.childCreature = 0;
				if(cmd.status.targetPreset == ActionCommand::TargetPreset::CHILD_INDEX_FFF) {
					const char* TargetParam = nullptr;
					pActionBase->QueryStringAttribute("TargetParam", &TargetParam);
					const char* f = TargetParam;
					for(i32 i = 0; f && i < 4; i++) { f = EA::StdC::Strchr(f, '|'); if(f) f++; }
					if(f) cmd.status.childCreature = EA::StdC::AtoI32(f);
				}
				const char* Param2 = nullptr;
				pActionBase->QueryStringAttribute("Param2", &Param2);
				cmd.status.removeAtEnd = (u8)(!Param2 ? ActionCommand::StatusEnd::NONE : EA::StdC::Strstr(Param2, "CleanUpWhenActionEnd") ? ActionCommand::StatusEnd::CLEANUP :
					EA::StdC::Strstr(Param2, "RemoveWhenActionEnd") ? ActionCommand::StatusEnd::ACTION : EA::StdC::Strstr(Param2, "RemoveWhenAniEnd") ? ActionCommand::StatusEnd::ANIMATION : ActionCommand::StatusEnd::NONE);
				cmd.status.remove = Param2 && EA::StdC::Strcmp(Param2, "Remove") == 0 ? 1 : 0;
				if(cmd.status.remove || cmd.status.removeAtEnd) actionRemovedStatuses.insert(cmd.status.statusIndex);
				const char* Param1 = nullptr;
				pActionBase->QueryStringAttribute("Param1", &Param1);
				cmd.status.condition = ActionCommand::StatusConditionFromString(Param1);
			} break;

			case ActionCommand::Type::ROTATESPEED: {
				pActionBase->QueryAttribute("Param1", &cmd.rotateSpeed.speed);
			} break;

			case ActionCommand::Type::MOVESPEED: {
				cmd.moveSpeed.percent = 0;
				pActionBase->QueryAttribute("Param1", &cmd.moveSpeed.percent);
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

static const i32 MEASURED_CLIENT_RADIATE_DEFAULT_NUM = 10;
static const f32 MEASURED_CLIENT_RADIATE_DEFAULT_ANGLE = 360.0f;
static const f32 MEASURED_CLIENT_CIRCLE_DEFAULT_RADIUS = 100.0f;

static void ParseRemoteSpawnRules(XMLElement* pEvent, Remote::SpawnRules& out)
{
	if(!pEvent) return;
	for(XMLElement* pAct = pEvent->FirstChildElement(); pAct; pAct = pAct->NextSiblingElement()) {
		if(EA::StdC::Strcmp(pAct->Name(), "_CreateEntity") == 0) {
			Remote::SpawnRule rule;
			i32 idx = -1;
			pAct->QueryAttribute("_RemoteIndex", &idx);
			pAct->QueryAttribute("_CreatureIndex", &rule.creature);
			if(idx <= 0 && rule.creature <= 0) continue;
			ASSERT(!out.full());
			if(idx > 0) rule.idx = RemoteIdx(idx);
			out.push_back(rule);
		}
		else if(EA::StdC::Strcmp(pAct->Name(), "_CreateRemote") == 0) {
			const char* toOwner = nullptr;
			pAct->QueryStringAttribute("_TargetOwner", &toOwner);
			const bool targetOwner = toOwner && EA::StdC::Stricmp(toOwner, "true") == 0;
			const char* hitInvalidText = pAct->Attribute("_HitInvalidParentLastTarget");
			const bool hitInvalidParent = hitInvalidText && EA::StdC::Stricmp(hitInvalidText, "true") == 0;
			for(XMLElement* pKind = pAct->FirstChildElement(); pKind; pKind = pKind->NextSiblingElement()) {
				Remote::SpawnRule rule;
				rule.targetOwner = targetOwner;
				rule.hitInvalidParentLastTarget = hitInvalidParent;
				i32 idx = -1;
				if(EA::StdC::Strcmp(pKind->Name(), "_Radiate") == 0) {
					rule.kind = Remote::SPAWN_RADIATE;
					rule.maxNum = MEASURED_CLIENT_RADIATE_DEFAULT_NUM;
					rule.maxAngle = MEASURED_CLIENT_RADIATE_DEFAULT_ANGLE;
					pKind->QueryAttribute("_Index", &idx);
					pKind->QueryAttribute("_MaxNum", &rule.maxNum);
					pKind->QueryFloatAttribute("_MaxAngle", &rule.maxAngle);
					pKind->QueryFloatAttribute("_OffsetForward", &rule.offsetForward);
					pKind->QueryFloatAttribute("_OffsetHeight", &rule.offsetHeight);
				}
				else if(EA::StdC::Strcmp(pKind->Name(), "_CircleArea") == 0) {
					rule.kind = Remote::SPAWN_CIRCLE;
					rule.radius = MEASURED_CLIENT_CIRCLE_DEFAULT_RADIUS;
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
				if(idx <= 0) continue;
				ASSERT(!out.full());
				rule.idx = RemoteIdx(idx);
				if(rule.maxNum < 1) rule.maxNum = 1;
				out.push_back(rule);
			}
		}
	}
}

static void ParseAppliedStatuses(XMLElement* pParent, const char* name, eastl::fixed_vector<Remote::AppliedStatus,4,false>& out)
{
	for(XMLElement* pStatus = pParent->FirstChildElement(name); pStatus; pStatus = pStatus->NextSiblingElement(name)) {
		Remote::AppliedStatus st;
		st.statusID = 0;
		st.rate = 0;
		st.value = 100;
		pStatus->QueryAttribute("DATAKEY", &st.statusID);
		pStatus->QueryAttribute("_Rate", &st.rate);
		pStatus->QueryAttribute("_Value", &st.value);
		bool listed = false;
		foreach_const(o, out) if(o->statusID == st.statusID) listed = true;
		if(st.statusID != 0 && !listed) { ASSERT(!out.full()); out.push_back(st); }
	}
}

static void ParseHoming(XMLElement* pHoming, Remote& remote)
{
	pHoming->QueryFloatAttribute("_ReactionLength", &remote.homingLength);
	pHoming->QueryFloatAttribute("_ReactionAngle", &remote.homingAngle);
	pHoming->QueryFloatAttribute("_ReactionTime", &remote.homingTime);
	pHoming->QueryBoolAttribute("_LockOn", &remote.homingLockOn);
}

static void ParseReplaceList(XMLElement* pLevel, const char* name, const char* fromAttr, eastl::fixed_vector<eastl::pair<i32,i32>,8,false>& out)
{
	for(XMLElement* p = pLevel->FirstChildElement(name); p; p = p->NextSiblingElement(name)) {
		const i32 from = p->IntAttribute(fromAttr);
		const i32 to = p->IntAttribute("_ReplaceIndex");
		if(from != 0 && to != 0 && !out.full()) out.push_back({ from, to });
	}
}

static void ParseStatusReplaceList(XMLElement* pLevel, GameXmlContent::StatusReplaceList& out)
{
	for(XMLElement* p = pLevel->FirstChildElement("_STATUSLIST"); p; p = p->NextSiblingElement("_STATUSLIST")) {
		GameXmlContent::StatusReplace sr;
		sr.statusID = p->IntAttribute("_StatusIndex");
		sr.replaceID = p->IntAttribute("_ReplaceIndex", sr.statusID);
		sr.durationSec = p->FloatAttribute("_DurationTime") / 1000.0f;
		p->QueryFloatAttribute("_DistanceRatio", &sr.distanceRatio);
		if(sr.statusID != 0) { ASSERT(!out.full()); out.push_back(sr); }
	}
}

void GameXmlContent::ParseUpgradeLists(XMLElement* pLevel, eastl::fixed_vector<eastl::pair<i32,i32>,8,false>& remoteReplace, StatusReplaceList& statusReplace, eastl::fixed_vector<eastl::pair<i32,const Remote*>,4,false>& remoteResize)
{
	ParseReplaceList(pLevel, "_REMOTELIST", "_RemoteIndex", remoteReplace);
	for(XMLElement* p = pLevel->FirstChildElement("_REMOTELIST"); p && !remoteResize.full(); p = p->NextSiblingElement("_REMOTELIST")) {
		const i32 id = p->IntAttribute("_RemoteIndex");
		const Remote* base = id != 0 && !p->Attribute("_ReplaceIndex") && p->Attribute("_LengthX") ? FindRemote(RemoteIdx(id)) : nullptr;
		if(!base) continue;
		Remote& variant = remoteVariants[(i32)remoteVariants.size()];
		variant = *base;
		variant.boundSize = { (u16)p->IntAttribute("_LengthX"), (u16)p->IntAttribute("_LengthY", base->boundSize[1]), (u16)p->IntAttribute("_LengthZ", base->boundSize[2]) };
		remoteResize.push_back({ id, &variant });
	}
	ParseStatusReplaceList(pLevel, statusReplace);
}

struct MeasuredSkillPowerMul { i32 skillID; f32 mul; };
static const MeasuredSkillPowerMul MEASURED_SKILL_POWER_MUL[] = {
	{ 180030030, 0.307f },
	{ 180120040, 0.728f },
	{ 180050040, 0.517f },
	{ 180070010, 0.508f },
	{ 180070030, 0.288f },
	{ 180240010, 0.318f },
	{ 180250010, 0.395f },
	{ 180340040, 0.164f },
	{ 180220040, 0.143f },
	{ 180350030, 0.624f },
	{ 180020010, 0.409f },
	{ 180070020, 0.481f },
	{ 180400020, 0.573f },
	{ 180140030, 0.421f },
	{ 180220020, 0.234f },
	{ 180270010, 0.589f },
};

bool GameXmlContent::LoadSkillUpgrades()
{
	eastl::hash_map<i32, eastl::fixed_vector<eastl::pair<i32,SkillUpgrade>,8,false>> byPriority;
	eastl::hash_map<i32, eastl::vector<eastl::pair<i32,StatusReplaceList>>> chains;
	for(XMLElement* pInfo = xmlSKILLPROPERTY.FirstChildElement()->FirstChildElement(); pInfo; pInfo = pInfo->NextSiblingElement()) {
		XMLElement* pSP = pInfo->FirstChildElement("ST_SKILL_PROPERTY");
		if(!pSP) continue;
		const char* type = pSP->Attribute("_Type");
		if(!type) continue;
		SkillUpgrade up;
		up.propertyID = pSP->IntAttribute("_Index");
		if(EA::StdC::Strcmp(type, "E_SKILL_PROPERTY_DAMAGE") == 0) up.type = SkillUpgrade::Type::DAMAGE;
		else if(EA::StdC::Strcmp(type, "E_SKILL_PROPERTY_EFFECT") == 0) up.type = SkillUpgrade::Type::EFFECT;
		else if(EA::StdC::Strcmp(type, "E_SKILL_PROPERTY_RANGE") == 0) up.type = SkillUpgrade::Type::RANGE;
		if(XMLElement* pLevel = pSP->FirstChildElement("_PROPERTY_LEVEL")) {
			pLevel->QueryFloatAttribute("_BaseDamage", &up.baseDamage);
			pLevel->QueryFloatAttribute("_AttackMultiplier", &up.attackMultiplier);
			pLevel->QueryFloatAttribute("_ConsumeMP", &up.consumeMP);
			if(const char* act = pLevel->Attribute("_Action")) up.action = ActionStateFromString(act);
			for(XMLElement* pR = pLevel->FirstChildElement("_RandomAction"); pR && !up.randomActions.full(); pR = pR->NextSiblingElement("_RandomAction")) if(const char* a = pR->Attribute("DATA")) up.randomActions.push_back(ActionStateFromString(a));
			pLevel->QueryAttribute("_NextComboSkill", &up.nextCombo);
			pLevel->QueryAttribute("_SummonIndex", &up.summon);
			pLevel->QueryFloatAttribute("_ConsumeEP", &up.consume[0]);
			pLevel->QueryFloatAttribute("_ConsumeRP", &up.consume[1]);
			pLevel->QueryFloatAttribute("_ConsumeBB", &up.consume[2]);
			pLevel->QueryFloatAttribute("_ConsumeHP", &up.consume[3]);
			pLevel->QueryFloatAttribute("_ConsumeUG", &up.consume[4]);
			pLevel->QueryFloatAttribute("_StanceDuration", &up.stanceDuration);
			pLevel->QueryFloatAttribute("_ToggleConsumeValuePerInterval", &up.toggleValue);
			pLevel->QueryFloatAttribute("_TargetMaxDistance", &up.targetMaxDistance);
			pLevel->QueryFloatAttribute("_CoolTime", &up.coolTime);
			pLevel->QueryIntAttribute("_WeaponFireCount", &up.weaponFireCount);
			ParseUpgradeLists(pLevel, up.remoteReplace, up.statusReplace, up.remoteResize);
		}
		for(XMLElement* pCost = pSP->FirstChildElement("_COST_LEVEL"); pCost && !up.costLevelLists.full(); pCost = pCost->NextSiblingElement("_COST_LEVEL")) {
			XMLElement* pLevel = pCost->FirstChildElement("_PROPERTY_LEVEL");
			if(pLevel && !up.costLevelDamage.full()) up.costLevelDamage.push_back({ pLevel->FloatAttribute("_BaseDamage", -1.0f), pLevel->FloatAttribute("_AttackMultiplier") });
			SkillUpgrade::CostLists& lists = up.costLevelLists.push_back();
			if(pLevel) ParseUpgradeLists(pLevel, lists.remoteReplace, lists.statusReplace, lists.remoteResize);
		}
		if(up.type == SkillUpgrade::Type::EFFECT && !up.statusReplace.empty()) chains[pSP->IntAttribute("_SkillIndex")].push_back({ pSP->IntAttribute("_Priority"), up.statusReplace });
		auto& list = byPriority[pSP->IntAttribute("_SkillIndex")];
		if(!list.full()) list.push_back({ pSP->IntAttribute("_Priority"), up });
	}
	foreach(it, byPriority) {
		eastl::sort(it->second.begin(), it->second.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
		SkillUpgradeList& out = skillUpgrades[it->first];
		foreach_const(p, it->second) out.push_back(p->second);
	}
	foreach(it, chains) {
		eastl::sort(it->second.begin(), it->second.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
		auto& out = propertyStatusChain[it->first];
		foreach_const(p, it->second) out.push_back(p->second);
	}
	foreach_const(it, skillUpgrades) {
		auto base = skillTooltipDamage.find(it->first);
		if(base == skillTooltipDamage.end() || base->second <= 0) continue;
		eastl::fixed_vector<eastl::pair<f32,f32>,8,false> points;
		foreach_const(up, it->second) if(up->type == SkillUpgrade::Type::DAMAGE && up->baseDamage > 0 && up->attackMultiplier > 0 && !points.full()) points.push_back({ up->baseDamage, up->attackMultiplier });
		if(points.empty()) continue;
		f32 mul = points[0].second * base->second / points[0].first;
		if(points.size() >= 2 && points[1].first != points[0].first) {
			const f32 slope = (points[1].second - points[0].second) / (points[1].first - points[0].first);
			const f32 line = points[0].second + slope * (base->second - points[0].first);
			if(slope > 0 && line > 0) mul = line;
		}
		skillBasePowerMul[it->first] = mul;
	}
	for(const auto& m : MEASURED_SKILL_POWER_MUL) skillBasePowerMul[m.skillID] = m.mul;
	LOG("Loaded in-game upgrades for %d skills, base skill power multipliers for %d", (i32)skillUpgrades.size(), (i32)skillBasePowerMul.size());
	return true;
}

static XMLElement* NextEntityInfo(XMLDocument* docs, i32 count, i32& file, XMLElement* cur)
{
	if(cur) {
		if(XMLElement* next = cur->NextSiblingElement()) return next;
		file++;
	}
	for(; file < count; file++) {
		XMLElement* root = docs[file].FirstChildElement();
		if(XMLElement* first = root ? root->FirstChildElement() : nullptr) return first;
	}
	return nullptr;
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

	static const wchar* REMOTE_FILES[] = { L"/REMOTE_PC.xml", L"/client/REMOTE_DYNAMIC.xml", L"/client/REMOTE_DYNAMIC_EXTRA.xml", L"/client/REMOTE_MOB_A.xml", L"/client/REMOTE_MOB_B.xml", L"/client/REMOTE_MOB_C.xml", L"/client/REMOTE_MOB_PUBLIC.xml" };
	XMLDocument xmlFiles[ARRAY_COUNT(REMOTE_FILES)];
	if(!LoadXMLFile(REMOTE_FILES[0], xmlFiles[0])) return false;
	for(i32 f = 1; f < (i32)ARRAY_COUNT(REMOTE_FILES); f++) LoadXMLFile(REMOTE_FILES[f], xmlFiles[f], true);
	i32 remoteFile = 0;

	for(XMLElement* pEntityInfo = NextEntityInfo(xmlFiles, (i32)ARRAY_COUNT(REMOTE_FILES), remoteFile, nullptr);
		pEntityInfo;
		pEntityInfo = NextEntityInfo(xmlFiles, (i32)ARRAY_COUNT(REMOTE_FILES), remoteFile, pEntityInfo)) {

		Remote remote;

		const char* KEYNAME = nullptr;
		i32 ID = 0;
		pEntityInfo->QueryStringAttribute("KEYNAME", &KEYNAME);
		pEntityInfo->QueryAttribute("ID", &ID);

		remote.ID = RemoteIdx(ID);
		if(KEYNAME && EA::StdC::Strstr(KEYNAME, "_TRIANGLE")) remote.polygonSides = 3;
		else if(KEYNAME && EA::StdC::Strstr(KEYNAME, "_SQUARE")) remote.polygonSides = 4;
		else if(KEYNAME && EA::StdC::Strstr(KEYNAME, "_PENTAGON")) remote.polygonSides = 5;

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
				remote.includeParty = XmlAttrTrue(pComp, "_IncludeParty");
				remote.vsRemote = XmlAttrTrue(pComp, "_VsRemote");
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
				ParseAppliedStatuses(pComp, "_Status", remote.statuses);
				if(const char* ev = pComp->Attribute("_BoundTypeEvade")) remote.evadeBound = EA::StdC::Strcmp(ev, "E_BOUND_SPHERE") == 0 ? 1 : (EA::StdC::Strcmp(ev, "E_BOUND_BOX") == 0 ? 2 : 0);
				remote.evadeLengthX = pComp->FloatAttribute("_LengthXEvade", 0.0f);
				remote.evadeLengthY = pComp->FloatAttribute("_LengthYEvade", 0.0f);
				remote.evadeAngle = pComp->FloatAttribute("_AngleEvade", 0.0f);
				ParseAppliedStatuses(pComp, "_StatusFriendly", remote.friendlyStatuses);
				remote.attachFireObject = XmlAttrTrue(pComp, "_AttachFireObject");
				remote.attachTargetObject = XmlAttrTrue(pComp, "_AttachTargetObject");
				pComp->QueryFloatAttribute("_CriticalRate", &remote.criticalRate);
				pComp->QueryFloatAttribute("_HitCriticalRate", &remote.hitCriticalRate);
				pComp->QueryFloatAttribute("_MaxHitCriticalRate", &remote.maxHitCriticalRate);
				pComp->QueryAttribute("_OnHitOwnerStatusIndex", &remote.onHitOwnerStatus);
				pComp->QueryAttribute("_SkillIndex", &remote.skillIndex);
				remote.activateCount = _ActivateCount;
				pComp->QueryAttribute("_PenetrationCount", &remote.penetrationCount);
				remote.lifeTime = _LifeTime;
				pComp->QueryFloatAttribute("_AttackMultiplier", &remote.attackMultiplier);
				remote.useOwnerRemoteSpeedRatio = XmlAttrTrue(pComp, "_UseOwnerRemoteSpeedRatio");
				pComp->QueryFloatAttribute("_OffsetY", &remote.offsetY);
				pComp->QueryFloatAttribute("_OffsetX", &remote.offsetX);
				if(XMLElement* pFd = pComp->FirstChildElement("_ForceDir")) {
					const char* fdType = nullptr;
					pFd->QueryStringAttribute("_Type", &fdType);
					remote.forceTargetDir = fdType && EA::StdC::Strcmp(fdType, "REMOTE_FORCE_DIRECTION_TYPE_TARGET_DIR") == 0;
					if(fdType && EA::StdC::Strcmp(fdType, "REMOTE_FORCE_DIRECTION_TYPE_FIXED_ANGLE") == 0) remote.forceFixedAngle = pFd->FloatAttribute("_Angle");
				}
				pComp->QueryFloatAttribute("_OffsetZ", &remote.offsetZ);
				pComp->QueryFloatAttribute("_MultiHitTimeOut", &remote.multiHitTimeOut);
				pComp->QueryFloatAttribute("_DistPenaltyRate", &remote.distPenaltyRate);
				pComp->QueryFloatAttribute("_DamageDistStart", &remote.damageDistStart);
				pComp->QueryFloatAttribute("_StatusTypeMultiplier", &remote.statusTypeMultiplier);
				remote.canReflect = XmlAttrTrue(pComp, "_CanReflect");
				if(XMLElement* pReflect = pComp->FirstChildElement("_Reflect")) {
					remote.reflectUse = XmlAttrTrue(pReflect, "_Use");
					remote.reflectDamage = pReflect->FloatAttribute("_Damage", 1.0f);
					const char* rt = pReflect->Attribute("_ReflectType");
					remote.reflectBack = rt && EA::StdC::Strstr(rt, "180");
				}
				pComp->QueryFloatAttribute("_HitDamageMultiplier", &remote.hitDamageMultiplier);
				pComp->QueryFloatAttribute("_MaxHitDamageMultiplier", &remote.maxHitDamageMultiplier);
				if(const char* mt = pComp->Attribute("_StatusTypeToMultiplyDamage")) remote.statusTypeMultiplyKey = StatusTypeKey(mt);
				remote.statusTypeMultiplierToCaster = XmlAttrTrue(pComp, "_bStatusTypeMultiplierToCaster");
				pComp->QueryFloatAttribute("_DamageOnBound", &remote.damageOnBound);
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
				pComp->QueryFloatAttribute("_FOV", &remote.fov);
				remote.targetOnly = XmlAttrTrue(pComp, "_TargetOnly");
				if(XMLElement* pHoming = pComp->FirstChildElement("_Homing")) ParseHoming(pHoming, remote);
				if(XMLElement* pOrbit = pComp->FirstChildElement("_Orbit")) pOrbit->QueryFloatAttribute("_Radius", &remote.orbitRadius);
			}
			else if((EA::StdC::Strcmp("_Homing", compName) == 0)) {
				ParseHoming(pComp, remote);
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
				pComp->QueryFloatAttribute("_InitSpeed", &remote.initSpeed);
				pComp->QueryFloatAttribute("_ForceScale", &remote.forceScale);
				pComp->QueryFloatAttribute("_ForceDelay", &remote.forceDelay);
				if(pComp->Attribute("_GroundMoveType")) remote.groundMoveOffsetZ = pComp->FloatAttribute("_GroundMoveOffsetZ", 0.0f);
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

static const u16 MEASURED_CLIENT_CREATOR_BASE = 1000;
static const char* const CLIENT_CREATOR_SUB_TAGS[6] = { "_CHARGINGREMOTE", "_ADDREMOTE", "_OVERHEAT", "_FIRINGCOUNT", "_COMBO", "_GAUGE" };
static const char* const CLIENT_CREATOR_EVENTS[8] = { "_Activate", "_ActivateOnGround", "_ActivateOnNotGround", "_ActivateOnCantPierce", "_ActivateOnShield", "_Destroy", "_ReachedMaxDistance", "_Time" };
enum ClientCreatorSlot: u8 { CREATOR_SLOT_ACTIVATE = 0, CREATOR_SLOT_NOT_GROUND = 2, CREATOR_SLOT_DESTROY = 5, CREATOR_SLOT_TIME = 7, CREATOR_SLOT_FACTORY = 8, CREATOR_SLOT_CHARGE = 9 };

struct ClientCreatorSub { u32 rate = 100; u32 index = 0xFFFFFFFF; eastl::string dummy; };
struct ClientCreatorRemote { eastl::string dummy; u32 remoteIndex = 0xFFFFFFFF; eastl::array<eastl::vector<ClientCreatorSub>,6> subs; u32 tail[2] = { 0, 0 }; };
struct ClientCreatorArea { u32 field[6] = { 0, 0, 0, 0, 0, 0 }; u8 flag = 0; i32 count = 0; bool hasFlag = true; };
struct ClientCreation
{
	eastl::vector<ClientCreatorRemote> defaults;
	eastl::vector<ClientCreatorArea> radiate;
	eastl::vector<ClientCreatorArea> circleFall;
	eastl::vector<ClientCreatorArea> circle;
	eastl::vector<ClientCreatorArea> fall;
	eastl::vector<ClientCreatorRemote> parallel;
	i32 ownerID = 0;
	u8 slot = 0;
	bool firstTime = false;
	bool spawner = false;
};

static u32 ClientXmlU32(const XMLElement* e, const char* name, u32 def)
{
	double v = 0;
	if(e->QueryDoubleAttribute(name, &v) != XML_SUCCESS) return def;
	return v < 0 ? (u32)(i32)v : (u32)v;
}

static u32 ClientXmlI32(const XMLElement* e, const char* name, i32 def)
{
	double v = 0;
	if(e->QueryDoubleAttribute(name, &v) != XML_SUCCESS) return (u32)def;
	return (u32)(i32)v;
}

static u8 ClientXmlBool(const XMLElement* e, const char* name, u8 def)
{
	const char* v = e->Attribute(name);
	return v ? (u8)(EA::StdC::Stricmp(v, "true") == 0) : def;
}

static ClientCreatorRemote ParseClientCreatorRemote(XMLElement* pKind, u32 tail0, u32 tail1)
{
	ClientCreatorRemote r;
	r.tail[0] = tail0;
	r.tail[1] = tail1;
	XMLElement* pRemote = pKind->FirstChildElement("_Remote");
	if(!pRemote) return r;
	if(const char* d = pRemote->Attribute("_FiringDummy")) r.dummy = d;
	r.remoteIndex = ClientXmlU32(pRemote, "_RemoteIndex", 0xFFFFFFFF);
	for(i32 s = 0; s < 6; s++) {
		for(XMLElement* pSub = pRemote->FirstChildElement(CLIENT_CREATOR_SUB_TAGS[s]); pSub; pSub = pSub->NextSiblingElement(CLIENT_CREATOR_SUB_TAGS[s])) {
			ClientCreatorSub sub;
			sub.rate = ClientXmlU32(pSub, "_Rate", 100);
			sub.index = ClientXmlU32(pSub, "_Index", 0xFFFFFFFF);
			if(const char* d = pSub->Attribute("_FiringDummy")) sub.dummy = d;
			r.subs[s].push_back(sub);
		}
	}
	return r;
}

static ClientCreation ParseClientCreation(XMLElement* pCreate)
{
	ClientCreation c;
	for(XMLElement* p = pCreate->FirstChildElement(); p; p = p->NextSiblingElement()) {
		const char* tag = p->Name();
		if(EA::StdC::Strcmp(tag, "_Default") == 0) c.defaults.push_back(ParseClientCreatorRemote(p, ClientXmlI32(p, "_AngleX", 0), ClientXmlI32(p, "_AngleY", 0)));
		else if(EA::StdC::Strcmp(tag, "_Parallel") == 0) c.parallel.push_back(ParseClientCreatorRemote(p, ClientXmlU32(p, "_MaxNum", 0), ClientXmlI32(p, "_Offset", 0)));
		else if(EA::StdC::Strcmp(tag, "_Radiate") == 0) {
			ClientCreatorArea a;
			a.field[0] = ClientXmlU32(p, "_Index", 0xFFFFFFFF);
			a.field[1] = ClientXmlU32(p, "_MaxNum", 10);
			a.field[2] = ClientXmlI32(p, "_MaxAngle", 360);
			a.field[3] = ClientXmlI32(p, "_OffsetAngle", 0);
			a.field[4] = ClientXmlI32(p, "_OffsetForward", 0);
			a.field[5] = ClientXmlI32(p, "_OffsetHeight", 0);
			a.flag = ClientXmlBool(p, "_OnlyUseParentRotateZ", 1);
			a.count = 6;
			c.radiate.push_back(a);
		}
		else if(EA::StdC::Strcmp(tag, "_CircleArea") == 0 || EA::StdC::Strcmp(tag, "_CircleAreaFall") == 0) {
			ClientCreatorArea a;
			a.field[0] = ClientXmlU32(p, "_Index", 0xFFFFFFFF);
			a.field[1] = ClientXmlU32(p, "_MaxNum", 1);
			a.field[2] = ClientXmlI32(p, "_Radius", 100);
			a.flag = ClientXmlBool(p, "_CheckKynapse", 0);
			a.count = 3;
			if(EA::StdC::Strcmp(tag, "_CircleArea") == 0) c.circle.push_back(a);
			else c.circleFall.push_back(a);
		}
		else if(EA::StdC::Strcmp(tag, "_Fall") == 0) {
			ClientCreatorArea a;
			a.field[0] = ClientXmlU32(p, "_Index", 0xFFFFFFFF);
			a.field[1] = ClientXmlU32(p, "_MaxNum", 10);
			a.field[2] = ClientXmlI32(p, "_Height", 0);
			a.field[3] = ClientXmlI32(p, "_RandomOffset", 0);
			a.count = 4;
			a.hasFlag = false;
			c.fall.push_back(a);
		}
	}
	return c;
}

static i32 ClientBytes8(u32 a0, u32 a1, u32 b0, u32 b1)
{
	u8 a[8], b[8];
	memcpy(a, &a0, 4);
	memcpy(a + 4, &a1, 4);
	memcpy(b, &b0, 4);
	memcpy(b + 4, &b1, 4);
	return memcmp(a, b, 8);
}

static i32 ClientStrCmp(const eastl::string& a, const eastl::string& b)
{
	const i32 r = EA::StdC::Strcmp(a.c_str(), b.c_str());
	return r < 0 ? -1 : (r > 0 ? 1 : 0);
}

static i32 CmpClientCreatorRemote(const ClientCreatorRemote& a, const ClientCreatorRemote& b)
{
	i32 r = ClientStrCmp(a.dummy, b.dummy);
	if(r) return r;
	r = (i32)(a.remoteIndex - b.remoteIndex);
	if(r) return r;
	for(i32 s = 0; s < 6; s++) {
		r = (i32)a.subs[s].size() - (i32)b.subs[s].size();
		if(r) return r;
		for(i32 k = 0; k < (i32)a.subs[s].size(); k++) {
			r = ClientBytes8(a.subs[s][k].rate, a.subs[s][k].index, b.subs[s][k].rate, b.subs[s][k].index);
			if(!r) r = ClientStrCmp(a.subs[s][k].dummy, b.subs[s][k].dummy);
			if(r) return r;
		}
	}
	return ClientBytes8(a.tail[0], a.tail[1], b.tail[0], b.tail[1]);
}

static i32 CmpClientCreatorArea(const ClientCreatorArea& a, const ClientCreatorArea& b)
{
	for(i32 f = 0; f < a.count; f++) if(a.field[f] != b.field[f]) return (i32)(b.field[f] - a.field[f]);
	return a.hasFlag ? (i32)b.flag - (i32)a.flag : 0;
}

template<class T, class F> static i32 CmpClientList(const eastl::vector<T>& a, const eastl::vector<T>& b, F cmp)
{
	const i32 n = (i32)MIN(a.size(), b.size());
	for(i32 k = 0; k < n; k++) {
		const i32 r = cmp(a[k], b[k]);
		if(r) return r;
	}
	return (i32)a.size() - (i32)b.size();
}

static i32 CmpClientCreation(const ClientCreation& a, const ClientCreation& b)
{
	i32 r = CmpClientList(a.defaults, b.defaults, CmpClientCreatorRemote);
	if(!r) r = CmpClientList(a.radiate, b.radiate, CmpClientCreatorArea);
	if(!r) r = CmpClientList(a.circleFall, b.circleFall, CmpClientCreatorArea);
	if(!r) r = CmpClientList(a.circle, b.circle, CmpClientCreatorArea);
	if(!r) r = CmpClientList(a.fall, b.fall, CmpClientCreatorArea);
	if(!r) r = CmpClientList(a.parallel, b.parallel, CmpClientCreatorRemote);
	return r;
}

static XMLElement* ClientPureSpawnerEvent(XMLElement* pInfo)
{
	XMLElement* pSfx = pInfo->FirstChildElement("RemoteSFXComData");
	const char* res = pSfx ? pSfx->Attribute("_ResName") : nullptr;
	const char* resTarget = pSfx ? pSfx->Attribute("_ResNameTarget") : nullptr;
	if((res && *res) || (resTarget && *resTarget)) return nullptr;
	XMLElement* pBound = pInfo->FirstChildElement("RemoteBoundComData2");
	const char* boundType = pBound ? pBound->Attribute("_Type") : nullptr;
	if(boundType && EA::StdC::Strcmp(boundType, "E_BOUND_NONE") != 0) return nullptr;
	XMLElement* pSteer = pInfo->FirstChildElement("SteeringBehaviorsComData");
	if(pSteer && pSteer->FloatAttribute("_MaxSpeed") > 0) return nullptr;
	XMLElement* pSystem = pInfo->FirstChildElement("RemoteSystemComData");
	if(!pSystem) return nullptr;
	XMLElement* pTime = pSystem->FirstChildElement("_Time");
	if(pTime && !pTime->NextSiblingElement("_Time") && pTime->FirstChildElement("_CreateRemote") && pTime->FloatAttribute("_Time") <= 0) return pTime;
	XMLElement* pDestroy = pSystem->FirstChildElement("_Destroy");
	XMLElement* pCommon = pInfo->FirstChildElement("RemoteComData2");
	if(pDestroy && pDestroy->FirstChildElement("_CreateRemote") && (!pCommon || pCommon->FloatAttribute("_LifeTime") <= 0)) return pDestroy;
	return nullptr;
}

bool GameXmlContent::LoadClientRemoteCreators()
{
	static const wchar* FILES[] = { L"/REMOTE_PC.xml", L"/client/REMOTE_DYNAMIC.xml", L"/client/REMOTE_MOB_A.xml", L"/client/REMOTE_MOB_B.xml", L"/client/REMOTE_MOB_C.xml", L"/client/REMOTE_MOB_PUBLIC.xml", L"/client/FIRING_METHOD.xml" };
	eastl::vector<ClientCreation> creations;
	for(i32 f = 0; f < (i32)ARRAY_COUNT(FILES); f++) {
		XMLDocument doc;
		if(!LoadXMLFile(FILES[f], doc)) return false;
		for(XMLElement* pInfo = doc.FirstChildElement()->FirstChildElement(); pInfo; pInfo = pInfo->NextSiblingElement()) {
			const i32 id = pInfo->IntAttribute("ID");
			if(XMLElement* pFactory = pInfo->FirstChildElement("RemoteFactoryComData")) {
				XMLElement* pCreate = pFactory->FirstChildElement("_Create");
				ClientCreation c = pCreate ? ParseClientCreation(pCreate) : ClientCreation();
				c.ownerID = id;
				c.slot = CREATOR_SLOT_FACTORY;
				creations.push_back(c);
				for(XMLElement* pCharge = pFactory->FirstChildElement("_CHARGE"); pCharge; pCharge = pCharge->NextSiblingElement("_CHARGE")) {
					for(XMLElement* pLevel = pCharge->FirstChildElement("_Create"); pLevel; pLevel = pLevel->NextSiblingElement("_Create")) {
						ClientCreation cc = ParseClientCreation(pLevel);
						cc.ownerID = id;
						cc.slot = CREATOR_SLOT_CHARGE;
						creations.push_back(cc);
					}
				}
			}
			XMLElement* pSystem = pInfo->FirstChildElement("RemoteSystemComData");
			if(!pSystem) continue;
			XMLElement* pSpawnerEvent = ClientPureSpawnerEvent(pInfo);
			for(XMLElement* pEvent = pSystem->FirstChildElement(); pEvent; pEvent = pEvent->NextSiblingElement()) {
				i32 slot = -1;
				for(i32 e = 0; e < (i32)ARRAY_COUNT(CLIENT_CREATOR_EVENTS); e++) if(EA::StdC::Strcmp(pEvent->Name(), CLIENT_CREATOR_EVENTS[e]) == 0) slot = e;
				XMLElement* pCreate = slot >= 0 ? pEvent->FirstChildElement("_CreateRemote") : nullptr;
				if(!pCreate) continue;
				ClientCreation c = ParseClientCreation(pCreate);
				c.ownerID = id;
				c.slot = (u8)slot;
				c.firstTime = slot == CREATOR_SLOT_TIME && pEvent == pSystem->FirstChildElement("_Time");
				c.spawner = pEvent == pSpawnerEvent;
				creations.push_back(c);
			}
		}
	}
	eastl::vector<i32> order;
	for(i32 k = 0; k < (i32)creations.size(); k++) order.push_back(k);
	eastl::sort(order.begin(), order.end(), [&](i32 a, i32 b) { return CmpClientCreation(creations[a], creations[b]) < 0; });
	u16 creatorID = MEASURED_CLIENT_CREATOR_BASE;
	for(i32 k = 0; k < (i32)order.size(); k++) {
		const ClientCreation& c = creations[order[k]];
		if(k == 0 || CmpClientCreation(creations[order[k - 1]], c) < 0) creatorID++;
		if(c.slot == CREATOR_SLOT_FACTORY) { firingCreators[c.ownerID] = creatorID; continue; }
		auto found = remoteMap.find(RemoteIdx(c.ownerID));
		if(found == remoteMap.end()) continue;
		Remote& remote = found->second;
		if(c.slot == CREATOR_SLOT_DESTROY) remote.creatorOnDestroy = creatorID;
		else if(c.slot == CREATOR_SLOT_TIME && c.firstTime) remote.creatorOnTime = creatorID;
		else if(c.slot == CREATOR_SLOT_ACTIVATE) remote.creatorOnActivate = creatorID;
		else if(c.slot == CREATOR_SLOT_NOT_GROUND) remote.creatorOnHitCharacter = creatorID;
		if(c.spawner) {
			remote.spawnerCreator = creatorID;
			remote.spawnerFromTime = c.slot == CREATOR_SLOT_TIME;
		}
	}
	LOG("Loaded %d client remote creations, %d creator ids", (i32)creations.size(), (i32)(creatorID - MEASURED_CLIENT_CREATOR_BASE));
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

	r = LoadPvpMaps();
	if(!r) return false;

	r = LoadMonsterStats();
	if (!r) return false;

	r = LoadMonsterAi();
	LoadMonsterScripts();
	if (!r) return false;
	LoadMonsterFires();

	r = LoadTitan();
	if (!r) return false;

	r = LoadIngameLevel();
	if (!r) return false;

	r = LoadMasterGrowth();
	if (!r) return false;

	r = LoadMasterTraits();
	if (!r) return false;

	r = LoadJukeboxSongs();
	if(!r) return false;

	r = LoadEntrySystems();
	if(!r) return false;

	r = LoadMissions();
	if(!r) return false;

	r = LoadExpInfo();
	if(!r) return false;

	r = LoadStageMaps();
	if(!r) return false;

	r = LoadBotCreatures();
	if(!r) return false;

	r = LoadStageBots();
	if(!r) return false;

	r = LoadDefencePhases();
	if(!r) return false;

	r = LoadPvpPraises();
	if(!r) return false;

	r = LoadServiceRule();
	if(!r) return false;

	r = LoadServicePrice();
	if(!r) return false;

	r = LoadAchievements();
	if(!r) return false;

	r = LoadSummaries();
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

	r = LoadStatusStatMods();
	if(!r) return false;
	r = LoadMeasuredFireNodes();
	if(r) r = LoadMeasuredSwingNodes();
	if(r) r = LoadMeasuredBotGear();
	if(r) r = LoadMeasuredBotTraits();
	if(r) r = LoadMeasuredWalkCells();
	if(!r) return false;
	r = LoadFireNodePaths();
	if(!r) return false;

	r = LoadBalanceMultipliers();
	if(!r) return false;

	r = LoadSkillCosts();
	if(!r) return false;

	r = LoadSkillUpgrades();
	if(!r) return false;

	r = LoadCharacterElements();
	if(!r) return false;

	r = LoadSummons();
	if(!r) return false;

	r = LoadSightData();
	if(!r) return false;

	r = LoadMasterGroupings();
	if(!r) return false;

	r = LoadFixedData();
	if(!r) return false;

	r = LoadTagStatuses();
	if(!r) return false;

	r = LoadDeathRemotes();
	if(!r) return false;

	r = LoadDynamicTraps();
	if(!r) return false;

	r = LoadStageTimeLimits();
	if(!r) return false;

	r = LoadAnimationSequenceTimes();
	if(!r) return false;

	r = LoadMoveGraphs();
	if(!r) return false;

	r = LoadSummonActions();
	if(!r) return false;

	r = LoadSummonBaseStats();
	if(!r) return false;

	r = LoadTradeItems();
	if(!r) return false;

	r = LoadStageEntries();
	if(!r) return false;

	r = LoadStageDifficulty();
	if(!r) return false;

	r = LoadGroundItems();
	if(!r) return false;

	r = LoadMonsterRanks();
	if(!r) return false;

	r = LoadGameEvents();
	if(!r) return false;

	r = LoadChildSkills();
	if(!r) return false;

	r = LoadWeaponFiring();
	if(!r) return false;

	r = LoadClientRemoteCreators();
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
		{"TEAM_PLAYER", Faction::RED},
		{"TEAM_MONSTER", Faction::MONSTER},
		{"TEAM_DYNAMIC", Faction::DYNAMIC},
		{"TEAM_ORANGE", Faction::ORANGE},
		{"TEAM_TYPE_INVALID", Faction::MONSTER}
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
	if(classType != ClassType::NONE) return FindSkillAction(ClassType::NONE, actionID, 0);
	return nullptr;
}

bool GameXmlContent::SkillActionKnown(ClassType classType, ActionStateID actionID, i8 stance) const
{
	return FindSkillAction(classType, actionID, stance) || skillActionGaps.find(SkillActionKey(classType, actionID)) != skillActionGaps.end();
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
		foreach_const(st, stageMaps) {
			eastl::fixed_vector<SkillID,10,false> stageSkills;
			if(st->stageSkill != SkillID::INVALID) stageSkills.push_back(st->stageSkill);
			foreach_const(c, st->stageSkillCandidates) if(!stageSkills.full()) stageSkills.push_back(*c);
			foreach_const(sid, stageSkills) {
				auto found = skillMap.find(*sid);
				if(found == skillMap.end()) continue;
				eastl::fixed_vector<ActionStateID,2,false> actions;
				actions.push_back(found->second.action);
				if(const SkillCast* cast = FindSkillCast((i32)*sid)) actions.push_back(cast->action);
				foreach_const(a, actions) {
					if(*a == ActionStateID::INVALID || FindSkillAction(m->classType, *a)) continue;
					skillActionGaps.insert(SkillActionKey(m->classType, *a));
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
		"STATUS_TYPE_BURROW",
		"STATUS_TYPE_CASTER",
		"STATUS_TYPE_COMA",
		"STATUS_TYPE_COUNTERSTATUS",
		"STATUS_TYPE_DANCE",
		"STATUS_TYPE_DEFEAT_CASTER_AURA",
		"STATUS_TYPE_DEFENDER",
		"STATUS_TYPE_DEFIANCE",
		"STATUS_TYPE_DISPEL",
		"STATUS_TYPE_DOUBLEGUN",
		"STATUS_TYPE_DRAGGED",
		"STATUS_TYPE_DRAGGEDBYFORCE",
		"STATUS_TYPE_ELECTRO",
		"STATUS_TYPE_EVADE",
		"STATUS_TYPE_FEAR",
		"STATUS_TYPE_FIREDELAY",
		"STATUS_TYPE_FLY",
		"STATUS_TYPE_FORCEBACK",
		"STATUS_TYPE_FREEZE",
		"STATUS_TYPE_GHOSTWIDOW",
		"STATUS_TYPE_GODMODE",
		"STATUS_TYPE_GX09",
		"STATUS_TYPE_HEAL",
		"STATUS_TYPE_HIDE",
		"STATUS_TYPE_HIT_RECOVER",
		"STATUS_TYPE_HIT_PLAGUE",
		"STATUS_TYPE_HPTRIGGER",
		"STATUS_TYPE_IMMUNESKILL",
		"STATUS_TYPE_KNOCKDOWN",
		"STATUS_TYPE_KROMEDE",
		"STATUS_TYPE_LINK",
		"STATUS_TYPE_MANABURN",
		"STATUS_TYPE_MORPH",
		"STATUS_TYPE_MOTION",
		"STATUS_TYPE_NEARLY_IMMORTAL",
		"STATUS_TYPE_NOCOST",
		"STATUS_TYPE_PUSH",
		"STATUS_TYPE_ROOT",
		"STATUS_TYPE_SECOND_CHANCE",
		"STATUS_TYPE_SHIELD",
		"STATUS_TYPE_SHIELDBOMB",
		"STATUS_TYPE_SHOCKED",
		"STATUS_TYPE_SILENCE",
		"STATUS_TYPE_SKILLSTAMINA",
		"STATUS_TYPE_SKILLSTATUS",
		"STATUS_TYPE_SLIME",
		"STATUS_TYPE_SLOW",
		"STATUS_TYPE_SNIPER",
		"STATUS_TYPE_STACKATTACK",
		"STATUS_TYPE_STACKBOMB",
		"STATUS_TYPE_STAT",
		"STATUS_TYPE_STATUS_ON_EVADE",
		"STATUS_TYPE_STUN",
		"STATUS_TYPE_SUICIDE",
		"STATUS_TYPE_SUPERARMOR",
		"STATUS_TYPE_TARGET",
		"STATUS_TYPE_TOGGLE",
		"STATUS_TYPE_UNDEAD",
		"STATUS_TYPE_UNSHAKABLE",
		"STATUS_TYPE_WEAK_ON_CRITICAL",
		"STATUS_TYPE_WEAPON_GAUGE",
		"STATUS_TYPE_WEAPON_GAUGE_RECOVERY",
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
		LOG("ERROR(LoadStatusData): client/STATUS_PC.xml missing");
		return false;
	}

	XMLDocument xmlDynamic;
	XMLDocument xmlMob;
	XMLDocument xmlItem;
	XMLDocument* docs[4] = { &xml, LoadXMLFile(L"/client/STATUS_DYNAMIC.xml", xmlDynamic, true) ? &xmlDynamic : nullptr, LoadXMLFile(L"/client/STATUS_MOB.xml", xmlMob, true) ? &xmlMob : nullptr, LoadXMLFile(L"/client/STATUS_ITEM.xml", xmlItem, true) ? &xmlItem : nullptr };
	for(XMLDocument* doc : docs) {
		if(!doc) continue;
		for(XMLElement* pInfo = doc->FirstChildElement()->FirstChildElement(); pInfo; pInfo = pInfo->NextSiblingElement()) {
			StatusInfo st;
			pInfo->QueryAttribute("ID", &st.ID);
			XMLElement* pCommon = pInfo->FirstChildElement("ST_COMMONSTATUS");
			const char* statusKey = pInfo->Attribute("KEYNAME");
			st.staminaBurn = statusKey && EA::StdC::Strstr(statusKey, "STAMINABURN") != nullptr;
			if(statusKey) st.keyName = statusKey;
			const char* statusDesc = pCommon ? pCommon->Attribute("_Description") : nullptr;
			const char* defenceDown = statusKey && statusDesc && EA::StdC::Strstr(statusKey, "DEFENCEDOWN") ? EA::StdC::Strstr(statusDesc, "-") : nullptr;
			if(defenceDown && EA::StdC::Strchr(defenceDown, '%')) st.descDefenceDown = (f32)EA::StdC::AtoI32(defenceDown + 1);
			const char* descHp = statusDesc && !EA::StdC::Strstr(statusDesc, " X ") ? EA::StdC::Strstr(statusDesc, "HP ") : nullptr;
			if(descHp) {
				const char* digits = descHp + 3;
				const char* end = digits;
				while(*end >= '0' && *end <= '9') end++;
				if(end > digits && *end == ' ') st.descHeal = (f32)EA::StdC::AtoI32(digits);
				if(end > digits && *end == '%') st.descHealPct = (f32)EA::StdC::AtoI32(digits);
			}
			if(!pCommon) continue;

			f32 durationMs = 0;
			pCommon->QueryFloatAttribute("_DurationTime", &durationMs);
			st.durationSec = durationMs / 1000.0f;
			pCommon->QueryFloatAttribute("_DistanceRatio", &st.distanceRatio);
			pCommon->QueryFloatAttribute("_TimeRatio", &st.timeRatio);
			pCommon->QueryFloatAttribute("_HeightRatio", &st.heightRatio);
			pCommon->QueryFloatAttribute("_LyingTime", &st.lyingTime);
			st.reaction = XmlAttrTrue(pCommon, "_ReAction");
			pCommon->QueryFloatAttribute("_DotBaseDamage", &st.dotDamage);
			st.attachToCaster = pCommon->Attribute("_AttachNodeName") != nullptr;
			pCommon->QueryFloatAttribute("_DotDrainMultiplier", &st.drainMultiplier);
			pCommon->QueryIntAttribute("_SkillIndex", &st.skillIndex);
			if(const char* rs = pCommon->Attribute("_RaiseStunByPush")) st.raiseStunByPush = rs[0] == 'T' || rs[0] == 't' || rs[0] == '1';
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
			if(const char* tt = pCommon->Attribute("_TargetingType")) {
				st.targetsFriendlyNpc = EA::StdC::Strcmp(tt, "FRIENDLY") == 0;
				st.targetsFriendlyPc = st.targetsFriendlyNpc || EA::StdC::Strcmp(tt, "FRIENDLY_PC") == 0;
				st.bothCharacters = EA::StdC::Strcmp(tt, "BOTH_CHARACTER") == 0;
				st.targetsLastChild = EA::StdC::Strcmp(tt, "LAST_CHILD") == 0;
			}
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
				const char* dispelStatus = nullptr;
				pCommon->QueryStringAttribute("_DispelStatusType", &dispelStatus);
				if(dispelStatus) st.dispelStatusType = StatusTypeFromString(dispelStatus);
				st.dispelMaxCount = pCommon->IntAttribute("_MaxDispeDamageApplyCount", 0);
				st.dispelDamage = pCommon->FloatAttribute("_DispelDamage", 0.0f);
				for(XMLElement* pDefense = pCommon->FirstChildElement("_ADD_DF_STATUS"); pDefense; pDefense = pDefense->NextSiblingElement("_ADD_DF_STATUS")) {
					const char* defense = nullptr;
					pDefense->QueryStringAttribute("DATAKEY", &defense);
					if(defense && !st.defenseTypes.full()) st.defenseTypes.push_back(StatusTypeFromString(defense));
				}
				XMLElement* pBlock = pCommon->FirstChildElement("_INPUT_STATE_BLOCK");
				if(pBlock) {
					i32 v = 0;
					v = 0; pBlock->QueryAttribute("_Attack", &v); st.blockAttack = v != 0;
					v = 0; pBlock->QueryAttribute("_Skill", &v); st.blockSkill = v != 0;
					i32 m = 0, s2 = 0;
					pBlock->QueryAttribute("_MovingShirk", &m);
					pBlock->QueryAttribute("_StandingShirk", &s2);
					st.blockShirk = m != 0 || s2 != 0;
					v = 0; pBlock->QueryAttribute("_Move", &v); st.blockMove = v != 0;
				}
				for(XMLElement* pAura = pCommon->FirstChildElement("_AuraStatusIndex"); pAura; pAura = pAura->NextSiblingElement("_AuraStatusIndex")) {
					i32 id = 0;
					pAura->QueryAttribute("DATA", &id);
					if(id == 0) continue;
					if(st.auraStatus == 0) st.auraStatus = id;
					ASSERT(!st.auraStatuses.full());
					st.auraStatuses.push_back(id);
				}
				st.auraEnemies = eff && EA::StdC::Strcmp(eff, "STATUS_EFFECT_UNKNOWN") == 0;
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
				st.typeKey = StatusTypeKey(type);
				st.type = StatusTypeFromString(type);
			}
			st.bigAggro = type && EA::StdC::Strcmp(type, "STATUS_TYPE_BIGAGGRO") == 0;
			for(XMLElement* pGroup = pCommon->FirstChildElement(); pGroup; pGroup = pGroup->NextSiblingElement()) {
				if(pGroup->FirstChildElement("_CoolTimeRecovery")) st.coolTimeRecovery = true;
				for(XMLElement* pLimit = pGroup->FirstChildElement("_SummonLimit"); pLimit && !st.summonLimits.full(); pLimit = pLimit->NextSiblingElement("_SummonLimit")) {
					const i32 creature = pLimit->IntAttribute("DATAKEY");
					const i32 limit = pLimit->IntAttribute("DATA");
					if(creature != 0 && limit > 0) st.summonLimits.push_back(eastl::make_pair(creature, limit));
				}
			}
			if(const char* motion = pCommon->Attribute("_AnimationName")) {
				if(st.type == StatusType::MOTION) st.motionAction = ActionStateFromString(motion);
				st.npcMotion = ActionStateFromString(motion);
			}

			{
				const char* pas = nullptr;
				pCommon->QueryStringAttribute("_Passive", &pas);
				st.permanent = durationMs <= 0 && pas && EA::StdC::Stricmp(pas, "true") == 0;
				st.instant = (pCommon->Attribute("_DurationTime") || st.type == StatusType::TOGGLE) && durationMs == 0 && !(pas && EA::StdC::Stricmp(pas, "true") == 0);
				pCommon->QueryAttribute("_InitCustomValue", &st.customInit);
				pCommon->QueryAttribute("_MaxCustomValue", &st.customMax);
				pCommon->QueryAttribute("_ShieldCount", &st.shieldCount);
				pCommon->QueryAttribute("_CasterStatusIndex", &st.casterStatus);
				if(XMLElement* pHit = pCommon->FirstChildElement("_HITEFFECT")) {
					pHit->QueryAttribute("_StatusDocIndex", &st.hitEffectStatus);
					pHit->QueryFloatAttribute("_AddValue", &st.hitEffectValue);
				}
				if(XMLElement* pStack = pCommon->FirstChildElement("_STACKATTACK")) {
					pStack->QueryAttribute("_Index", &st.stackStatus);
					pStack->QueryAttribute("_NextStatusIndex", &st.stackFullStatus);
					for(XMLElement* pR = pStack->FirstChildElement("_RemoteIndexList"); pR; pR = pR->NextSiblingElement("_RemoteIndexList")) { ASSERT(!st.stackRemotes.full()); st.stackRemotes.push_back(pR->IntAttribute("DATA")); }
					for(XMLElement* pS = pStack->FirstChildElement("_FullStackSkillIndexList"); pS; pS = pS->NextSiblingElement("_FullStackSkillIndexList")) { ASSERT(!st.fullStackSkills.full()); st.fullStackSkills.push_back(pS->IntAttribute("DATA")); }
				}
				if(XMLElement* pIce = pCommon->FirstChildElement("_ICEQUEEN")) {
					pIce->QueryAttribute("_Index", &st.stackStatus);
					pIce->QueryAttribute("_FreezeStatusIndex", &st.stackFullStatus);
				}
				if(st.type == StatusType::NOCOST || st.type == StatusType::DOUBLEGUN) {
					if(XMLElement* pCost = pCommon->FirstChildElement("_DOUBLEGUN")) {
						pCost->QueryFloatAttribute("_AddRatioCostMP", &st.costRatioMP);
						st.stackDurationSec = pCost->FloatAttribute("_Duration") / 1000.0f;
						pCost->QueryAttribute("_MaxOverlapCount", &st.stackMax);
						for(XMLElement* pS = pCost->FirstChildElement("_SkillList"); pS; pS = pS->NextSiblingElement("_SkillList")) {
							i32 v = 0;
							pS->QueryAttribute("DATA", &v);
							if(v) { ASSERT(!st.costSkills.full()); st.costSkills.push_back(v); }
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
						if(a->QueryDoubleValue(&v) != XML_SUCCESS || EA::StdC::Strlen(a->Name()) >= 31) continue;
						ASSERT(!st.params.full());
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
							if(a->QueryDoubleValue(&v) != XML_SUCCESS || EA::StdC::Strlen(a->Name()) >= 31) continue;
							ASSERT(!st.params.full());
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
	}

	LOG("Loaded %d statuses", (i32)statusMap.size());







	return true;
}

static StatusMod::Stat StatusModStatFromName(const char* stat)
{
	static const struct { const char* name; StatusMod::Stat stat; } NAMES[] = {
		{ "STAT_TYPE_SHIELD_DEFENCE", StatusMod::SHIELD_DEFENCE },
		{ "STAT_TYPE_HEALTH", StatusMod::HEALTH },
		{ "STAT_TYPE_SKILLPOWER", StatusMod::SKILLPOWER },
		{ "STAT_TYPE_CRITICALRATE", StatusMod::CRITICALRATE },
		{ "STAT_TYPE_REFLECT_DAMAGE_RATIO", StatusMod::REFLECT_DAMAGE_RATIO },
		{ "STAT_TYPE_SKILL_CRITICALRATE", StatusMod::SKILL_CRITICALRATE },
		{ "STAT_TYPE_ATTACK", StatusMod::ATTACK },
		{ "STAT_TYPE_MOVESPEED", StatusMod::MOVESPEED },
		{ "STAT_TYPE_DEFENCE", StatusMod::DEFENCE },
		{ "STAT_TYPE_MANAREGEN", StatusMod::MANAREGEN },
		{ "STAT_TYPE_RAGEREGEN", StatusMod::RAGEREGEN },
		{ "STAT_TYPE_STAMINA", StatusMod::STAMINA },
		{ "STAT_TYPE_REMOTESPEED", StatusMod::REMOTESPEED },
		{ "STAT_TYPE_STAMINAREGEN", StatusMod::STAMINAREGEN },
		{ "STAT_TYPE_ENERGYREGEN", StatusMod::ENERGYREGEN },
		{ "STAT_TYPE_TAG_HEALTHREGEN_VALUE", StatusMod::TAG_HEALTHREGEN },
		{ "STAT_TYPE_TAG_COOLTIME", StatusMod::TAG_COOLTIME },
		{ "STAT_TYPE_HEALTHREGEN", StatusMod::HEALTHREGEN },
		{ "SERVER_DAMAGE_TAKEN_RATIO", StatusMod::DAMAGE_TAKEN },
		{ "STAT_TYPE_LIFESTEAL", StatusMod::LIFESTEAL },
		{ "STAT_TYPE_RAGE", StatusMod::RAGE },
		{ "STAT_TYPE_SKILL_COOLTIME_DISCOUNT", StatusMod::SKILL_COOLTIME_DISCOUNT },
		{ "STAT_TYPE_SKILLDAMAGE_REDUCE_RATIO", StatusMod::SKILLDAMAGE_REDUCE },
	};
	for(const auto& n : NAMES) {
		if(EA::StdC::Strcmp(stat, n.name) == 0) return n.stat;
	}
	return StatusMod::OTHER;
}

bool GameXmlContent::LoadStatusStatMods()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/StatusStatMods.xml", doc)) return true;
	i32 count = 0;
	for(XMLElement* pMod = doc.FirstChildElement()->FirstChildElement("MOD"); pMod; pMod = pMod->NextSiblingElement("MOD")) {
		auto found = statusMap.find(pMod->IntAttribute("status"));
		const char* stat = pMod->Attribute("stat");
		if(found == statusMap.end() || !stat || found->second.mods.full()) continue;
		StatusMod mod;
		mod.stat = StatusModStatFromName(stat);
		mod.value = pMod->FloatAttribute("value");
		mod.mul = pMod->BoolAttribute("mul");
		mod.count = pMod->FloatAttribute("count");
		mod.intervalSec = pMod->FloatAttribute("interval");
		mod.regen = mod.stat == StatusMod::HEALTH || (mod.stat == StatusMod::RAGE && mod.count > 1);
		if(mod.stat == StatusMod::OTHER || mod.value == 0) continue;
		found->second.mods.push_back(mod);
		count++;
	}
	static const struct { i32 status; StatusType type; } DESC_STATUS_RESISTS[] = {
		{ 1230410023, StatusType::SHOCKED },
	};
	for(const auto& r : DESC_STATUS_RESISTS) {
		auto found = statusMap.find(r.status);
		if(found != statusMap.end() && !found->second.defenseTypes.full()) found->second.defenseTypes.push_back(r.type);
	}
	LOG("Loaded %d measured status stat mods", count);
	return true;
}

bool GameXmlContent::LoadMeasuredFireNodes()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/MeasuredFireNodes.xml", doc)) return true;
	for(XMLElement* pFire = doc.FirstChildElement()->FirstChildElement("FIRE"); pFire; pFire = pFire->NextSiblingElement("FIRE")) {
		MeasuredFireNode node;
		node.forward = pFire->FloatAttribute("forward");
		node.side = pFire->FloatAttribute("side");
		node.up = pFire->FloatAttribute("up");
		node.hasChild = pFire->Attribute("childForward") != nullptr;
		node.childForward = pFire->FloatAttribute("childForward");
		node.childSide = pFire->FloatAttribute("childSide");
		node.childUp = pFire->FloatAttribute("childUp");
		node.childRot = pFire->FloatAttribute("childRot");
		node.childDirZ = pFire->FloatAttribute("childDirZ");
		measuredFireNodes[FireNodeKey(pFire->IntAttribute("skill"), pFire->IntAttribute("remote"), pFire->IntAttribute("index"))] = node;
	}
	LOG("Loaded %d measured fire nodes", (i32)measuredFireNodes.size());
	return true;
}

bool GameXmlContent::LoadMeasuredSwingNodes()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/MeasuredSwingNodes.xml", doc)) return true;
	for(XMLElement* pSwing = doc.FirstChildElement()->FirstChildElement("SWING"); pSwing; pSwing = pSwing->NextSiblingElement("SWING")) {
		MeasuredSwingNode node;
		node.forward = pSwing->FloatAttribute("forward");
		node.side = pSwing->FloatAttribute("side");
		measuredSwingNodes[pSwing->IntAttribute("remote")] = node;
	}
	LOG("Loaded %d measured swing nodes", (i32)measuredSwingNodes.size());
	return true;
}

bool GameXmlContent::LoadMeasuredBotGear()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/MeasuredBotGear.xml", doc, true) || !doc.FirstChildElement()) return true;
	for(XMLElement* pGear = doc.FirstChildElement()->FirstChildElement("GEAR"); pGear; pGear = pGear->NextSiblingElement("GEAR")) {
		MeasuredBotGear gear;
		gear.classType = pGear->IntAttribute("class");
		gear.attack = pGear->FloatAttribute("attack");
		gear.defence = pGear->FloatAttribute("defence");
		gear.health = pGear->FloatAttribute("health");
		gear.critical = pGear->FloatAttribute("critical");
		gear.weaponTier = pGear->IntAttribute("weaponTier", 1);
		gear.weapon = pGear->IntAttribute("weapon", 0);
		measuredBotGear.push_back(gear);
	}
	LOG("Loaded %d measured bot gear profiles", (i32)measuredBotGear.size());
	return true;
}

bool GameXmlContent::LoadMeasuredBotTraits()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/MeasuredBotTraits.xml", doc, true) || !doc.FirstChildElement()) return true;
	for(XMLElement* p = doc.FirstChildElement()->FirstChildElement("TRAITS"); p; p = p->NextSiblingElement("TRAITS")) {
		MeasuredBotTraits t;
		t.classType = p->IntAttribute("class");
		t.levels = { (u8)p->IntAttribute("t0"), (u8)p->IntAttribute("t1"), (u8)p->IntAttribute("t2"), (u8)p->IntAttribute("t3") };
		measuredBotTraits.push_back(t);
	}
	LOG("Loaded %d measured bot trait profiles", (i32)measuredBotTraits.size());
	return true;
}

bool GameXmlContent::LoadMeasuredWalkCells()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/MeasuredWalkCells.xml", doc, true) || !doc.FirstChildElement()) return true;
	i32 count = 0;
	for(XMLElement* pMap = doc.FirstChildElement()->FirstChildElement("MAP"); pMap; pMap = pMap->NextSiblingElement("MAP")) {
		auto& cells = measuredWalkCells[pMap->IntAttribute("index")];
		for(XMLElement* c = pMap->FirstChildElement("CELL"); c; c = c->NextSiblingElement("CELL")) {
			cells.push_back(vec3(c->FloatAttribute("x"), c->FloatAttribute("y"), c->FloatAttribute("z")));
			count++;
		}
	}
	LOG("Loaded %d measured walk cells", count);
	return true;
}

bool GameXmlContent::LoadFireNodePaths()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/FireNodePaths.xml", doc)) return true;
	for(XMLElement* pPath = doc.FirstChildElement()->FirstChildElement("PATH"); pPath; pPath = pPath->NextSiblingElement("PATH")) {
		FireNodePath path;
		path.step = pPath->FloatAttribute("step");
		for(XMLElement* p = pPath->FirstChildElement("P"); p; p = p->NextSiblingElement("P")) path.points.push_back(vec3(p->FloatAttribute("forward"), p->FloatAttribute("left"), p->FloatAttribute("up")));
		if(path.step <= 0 || path.points.size() < 2) continue;
		const i32 index = (i32)fireNodePaths.size();
		fireNodePaths.push_back(path);
		const char* at = pPath->Attribute("remotes");
		while(at && *at) {
			while(*at == ' ') at++;
			if(!*at) break;
			fireNodePathByRemote[EA::StdC::AtoI32(at)] = index;
			while(*at && *at != ' ') at++;
		}
	}
	LOG("Loaded %d fire node paths for %d remotes", (i32)fireNodePaths.size(), (i32)fireNodePathByRemote.size());
	return true;
}

bool GameXmlContent::LoadBalanceMultipliers()
{
	static const i32 BALANCE_INDEX_SKILL_OFFSET = 8000000;
	XMLDocument doc;
	if(!LoadXMLFile(L"/client/BALANCE_MULTIPLIER.xml", doc)) return false;
	struct Multipliers { f32 dot = 0; f32 heal = 0; f32 shield = 0; };
	eastl::hash_map<i32, Multipliers> balance;
	for(XMLElement* pInfo = doc.FirstChildElement()->FirstChildElement(); pInfo; pInfo = pInfo->NextSiblingElement()) {
		XMLElement* pBalance = pInfo->FirstChildElement("ST_BALANCE_MULTIPLIER");
		if(!pBalance) continue;
		Multipliers m;
		pBalance->QueryFloatAttribute("_DotMultiplier", &m.dot);
		pBalance->QueryFloatAttribute("_HealMultiplier", &m.heal);
		pBalance->QueryFloatAttribute("_ShieldMultiplier", &m.shield);
		balance[pBalance->IntAttribute("_Index")] = m;
	}
	i32 count = 0;
	foreach(it, statusMap) {
		StatusInfo& st = it->second;
		if(st.skillIndex == 0) continue;
		auto found = balance.find(st.skillIndex + BALANCE_INDEX_SKILL_OFFSET);
		if(found == balance.end()) found = balance.find(st.skillIndex / 10 * 10 + BALANCE_INDEX_SKILL_OFFSET);
		if(found == balance.end()) continue;
		st.dotMul = found->second.dot;
		st.healMul = found->second.heal;
		st.shieldMul = found->second.shield;
		if(st.dotMul > 0 || st.healMul > 0 || st.shieldMul > 0) count++;
	}
	LOG("Loaded %d balance multipliers, %d statuses scale with skill power", (i32)balance.size(), count);
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
			f32 remoteDelegateMul = 1.0f;
			for(XMLElement* pAttr = pBase->FirstChildElement("attrMap"); pAttr; pAttr = pAttr->NextSiblingElement("attrMap")) {
				const char* key = pAttr->Attribute("DATAKEY");
				if(!key || EA::StdC::Strcmp(key, "TTATTR_REMOTE_DELIGATE") != 0) continue;
				const Remote* delegate = FindRemote(RemoteIdx(pAttr->IntAttribute("DATA")));
				if(delegate && delegate->attackMultiplier > 0) remoteDelegateMul = delegate->attackMultiplier;
				break;
			}
			for(XMLElement* pT = pBase->FirstChildElement("tableDatas"); pT; pT = pT->NextSiblingElement("tableDatas")) {
				const char* attrType = nullptr;
				pT->QueryStringAttribute("attrType", &attrType);
				if(!attrType || EA::StdC::Strcmp(attrType, "TTATTR_REMOTE_DAMAGE") != 0) continue;
				f32 dmg = 0;
				pT->QueryFloatAttribute("value_1", &dmg);
				if(dmg > 0 && skillBaseDamage.find(skillID) == skillBaseDamage.end()) {
					skillBaseDamage[skillID] = dmg / remoteDelegateMul;
					skillTooltipDamage[skillID] = dmg;
				}
			}
			for(XMLElement* pLevel = pBase; pLevel; pLevel = pLevel->NextSiblingElement("data")) {
				eastl::fixed_set<i32,16,true> levelTouched;
				eastl::fixed_vector<eastl::pair<eastl::fixed_string<char,40,false>, i32>,16,false> keyStatus;
				i32 delegateID = 0;
				for(XMLElement* pAttr = pLevel->FirstChildElement("attrMap"); pAttr; pAttr = pAttr->NextSiblingElement("attrMap")) {
					const char* key = nullptr;
					i32 data = 0;
					pAttr->QueryStringAttribute("DATAKEY", &key);
					pAttr->QueryAttribute("DATA", &data);
					if(!key) continue;
					if(EA::StdC::Strcmp(key, "TTATTR_STATUS_DELIGATE") == 0) delegateID = data;
					ASSERT(!keyStatus.full());
					keyStatus.push_back(eastl::make_pair(eastl::fixed_string<char,40,false>(key), data));
				}
				for(XMLElement* pT = pLevel->FirstChildElement("tableDatas"); pT; pT = pT->NextSiblingElement("tableDatas")) {
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
					if(touched.count(statusID) && !levelTouched.count(statusID)) continue;
					pT->QueryStringAttribute("proc", &proc);
					pT->QueryStringAttribute("apply", &apply);
					pT->QueryStringAttribute("stat", &stat);
					StatusMod mod;
					pT->QueryFloatAttribute("value_1", &mod.value);
					pT->QueryFloatAttribute("value_2", &mod.count);
					if(mod.value == 0 || !stat) continue;
					mod.regen = proc && EA::StdC::Strcmp(proc, "STATPROC_REGEN") == 0;
					mod.mul = apply && EA::StdC::Strcmp(apply, "STATAPPLY_MUL") == 0;
					mod.stat = StatusModStatFromName(stat);
					if(mod.mul && mod.stat != StatusMod::OTHER && !skillMulStats.full()) skillMulStats.push_back({ skillID, statusID, (u8)mod.stat, mod.value });
					if(!found->second.mods.full()) { found->second.mods.push_back(mod); modCount++; }
					touched.insert(statusID);
					levelTouched.insert(statusID);
				}
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
	{
		eastl::hash_map<eastl::string,i32> statusByKey;
		foreach_const(it, statusMap) if(!it->second.keyName.empty()) statusByKey[eastl::string(it->second.keyName.c_str())] = it->second.ID;
		foreach(it, statusMap) {
			StatusInfo& st = it->second;
			const char* key = st.keyName.c_str();
			const char* flag = EA::StdC::Strstr(key, "_FLAG");
			if(!flag) continue;
			eastl::string effect(key, flag);
			effect += flag + 5;
			auto found = statusByKey.find(effect);
			if(found != statusByKey.end()) st.flagEffectStatus = found->second;
		}
		foreach(it, statusMap) {
			StatusInfo& st = it->second;
			const eastl_size_t len = st.keyName.size();
			if(st.type != StatusType::TOGGLE || len <= 7 || EA::StdC::Strcmp(st.keyName.c_str() + len - 7, "_REMOVE") != 0) continue;
			st.endsGrab = len > 12 && EA::StdC::Strcmp(st.keyName.c_str() + len - 12, "_GRAB_REMOVE") == 0;
			auto found = statusByKey.find(eastl::string(st.keyName.c_str(), st.keyName.c_str() + len - 7));
			if(found == statusByKey.end()) continue;
			st.toggleRemoves = found->second;
			auto removed = statusMap.find(found->second);
			if(removed != statusMap.end() && removed->second.durationSec < 0) actionRemovedStatuses.insert(found->second);
		}
	}
	static const f32 MEASURED_DESC_HEAL_SKILLPOWER_RATIO = 8.0f / 37.0f;
	foreach(it, statusMap) {
		StatusInfo& st = it->second;
		if(st.descHeal <= 0 || !st.mods.empty() || st.type != StatusType::HEAL) continue;
		StatusMod mod;
		mod.stat = StatusMod::HEALTH;
		mod.regen = true;
		mod.value = st.descHeal;
		mod.count = 1;
		st.mods.push_back(mod);
		if(st.healMul <= 0) st.healMul = MEASURED_DESC_HEAL_SKILLPOWER_RATIO;
		modCount++;
	}
	LOG("Loaded %d status stat modifiers from skilltt", modCount);

	XMLDocument xmlDesc;
	if(!LoadXMLFile(L"/client/string/skilldesc.xml", xmlDesc)) return false;
	{
		i32 added = 0;
		XMLElement* pTable = xmlDesc.FirstChildElement()->FirstChildElement("StringTableData");
		for(XMLElement* pS = pTable ? pTable->FirstChildElement("StringData") : nullptr; pS; pS = pS->NextSiblingElement("StringData")) {
			const char* key = nullptr; const char* text = nullptr;
			pS->QueryStringAttribute("DATAKEY", &key);
			pS->QueryStringAttribute("data", &text);
			if(!key || !text || EA::StdC::Strncmp(key, "STR_SKILL_DESC_", 15) != 0) continue;
			if(EA::StdC::Stristr(text, "can be struck multiple times") || EA::StdC::Stristr(text, "can hit the same target") || EA::StdC::Stristr(text, "strike the same target")) repeatHitReduced.insert(EA::StdC::AtoI32(key + 15));
			if(const char* restores = EA::StdC::Stristr(text, "restores ")) {
				const char* digits = restores + 9;
				const char* end = digits;
				while(*end >= '0' && *end <= '9') end++;
				const char* maxHp = EA::StdC::Stristr(end, "maximum HP");
				const char* sentence = EA::StdC::Strchr(end, '.');
				if(end > digits && *end == '%' && maxHp && (!sentence || maxHp < sentence)) {
					const i32 skillID = EA::StdC::AtoI32(key + 15);
					foreach(it, statusMap) {
						StatusInfo& st = it->second;
						if(st.skillIndex != skillID || st.type != StatusType::HEAL || !st.mods.empty()) continue;
						StatusMod mod;
						mod.stat = StatusMod::HEALTH;
						mod.regen = true;
						mod.mul = true;
						mod.value = (f32)EA::StdC::AtoI32(digits);
						mod.count = 1;
						st.mods.push_back(mod);
						LOG("Status %d heals %.0f%% of maximum HP (skill description %d)", st.ID, mod.value, skillID);
					}
				}
			}
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
				const char* selectType = pComboTarget ? pComboTarget->Attribute("_SelectType") : nullptr;
				const bool hitTarget = selectType && EA::StdC::Strcmp(selectType, "SKILL_TARGET_USE_HIT_EVENT_TARGET") == 0;
				if(!hitTarget && pCombo->BoolAttribute("_AutoCasting")) comboBotAutoCastMs[skillID] = MAX(pCombo->IntAttribute("_ExecuteTimeMS"), 1);
				if(selectType && EA::StdC::Strcmp(selectType, "SKILL_TARGET_USE_HIT_EVENT_TARGET") == 0) hitTargetComboMs[skillID] = pCombo->BoolAttribute("_AutoCasting") ? MAX(pCombo->IntAttribute("_ExecuteTimeMS"), 1) : 0;
				for(XMLElement* pC = pCombo->FirstChildElement("_CleanupStatus"); pC; pC = pC->NextSiblingElement("_CleanupStatus")) {
					auto& list = comboCleanupStatuses[skillID];
					const i32 st = pC->IntAttribute("DATA");
					if(st != 0 && !list.full()) list.push_back(st);
				}
				comboDurationMs[skillID] = pCombo->IntAttribute("_DurationTimeMS");
			}
			if(XMLElement* pCost = pCommon->FirstChildElement("_COST_PROPERTY")) {
				for(XMLElement* pLvl = pCost->FirstChildElement("_PROPERTY_LEVEL"); pLvl; pLvl = pLvl->NextSiblingElement("_PROPERTY_LEVEL")) {
					CostLevel lvl;
					pLvl->QueryFloatAttribute("_ConsumeEP", &lvl.consume[0]);
					pLvl->QueryFloatAttribute("_ConsumeRP", &lvl.consume[1]);
					pLvl->QueryFloatAttribute("_ConsumeBB", &lvl.consume[2]);
					if(const char* act = pLvl->Attribute("_Action")) lvl.action = ActionStateFromString(act);
					for(XMLElement* pR = pLvl->FirstChildElement("_RandomAction"); pR && !lvl.randomActions.full(); pR = pR->NextSiblingElement("_RandomAction")) if(const char* a = pR->Attribute("DATA")) lvl.randomActions.push_back(ActionStateFromString(a));
					for(XMLElement* pR = pLvl->FirstChildElement("_REMOTELIST"); pR; pR = pR->NextSiblingElement("_REMOTELIST")) {
						i32 from = 0, to = 0;
						pR->QueryAttribute("_RemoteIndex", &from);
						pR->QueryAttribute("_ReplaceIndex", &to);
						if(from != 0 && to != 0) { ASSERT(!lvl.remoteReplace.full()); lvl.remoteReplace.push_back({ from, to }); }
					}
					ParseStatusReplaceList(pLvl, lvl.statusReplace);
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
					ASSERT(!levels.full());
					levels.push_back(lvl);
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
	if(!LoadXMLFile(L"/CREATURE_CHARACTER.xml", xmlChara)) return false;
	for(XMLElement* pInfo = xmlChara.FirstChildElement()->FirstChildElement(); pInfo; pInfo = pInfo->NextSiblingElement()) {
		i32 id = 0;
		pInfo->QueryAttribute("ID", &id);
		for(XMLElement* pComp = pInfo->FirstChildElement(); pComp; pComp = pComp->NextSiblingElement()) {
			const char* el = nullptr;
			if(pComp->QueryStringAttribute("_ElementType", &el) != XML_SUCCESS || !el) continue;
			Element e = Element::NONE;
			if(EA::StdC::Strcmp(el, "CELEMENT_TYPE_RED") == 0) e = Element::RED;
			else if(EA::StdC::Strcmp(el, "CELEMENT_TYPE_GREEN") == 0) e = Element::GREEN;
			else if(EA::StdC::Strcmp(el, "CELEMENT_TYPE_BLUE") == 0) e = Element::BLUE;
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
	for(XMLElement* pInfo = xmlSummon.FirstChildElement()->FirstChildElement(); pInfo; pInfo = pInfo->NextSiblingElement()) LoadSummonInfo(pInfo);
	eastl::hash_set<i32> sharedEntities;
	XMLDocument xmlShared;
	if(LoadXMLFile(L"/client/ActionBase_Class.xml", xmlShared)) {
		for(XMLElement* pA = xmlShared.FirstChildElement()->FirstChildElement(); pA; pA = pA->NextSiblingElement()) {
			const char* type = pA->Attribute("CommandType");
			if(type && EA::StdC::Strcmp(type, "ENTITY") == 0) sharedEntities.insert(pA->IntAttribute("EntityIndex"));
		}
	}
	XMLDocument xmlPvp;
	if(LoadXMLFile(L"/client/CREATURE_MONSTER_PvP.xml", xmlPvp)) {
		for(XMLElement* pInfo = xmlPvp.FirstChildElement()->FirstChildElement(); pInfo; pInfo = pInfo->NextSiblingElement()) {
			const i32 id = pInfo->IntAttribute("ID");
			if(sharedEntities.find(id) != sharedEntities.end() && summonMap.find(id) == summonMap.end()) LoadSummonInfo(pInfo);
			XMLElement* creature = pInfo->FirstChildElement("CreatureComData");
			XMLElement* detector = creature ? creature->FirstChildElement("_HideDetector") : nullptr;
			if(detector) creatureHideDetect[id] = detector->FloatAttribute("_Radius", 0.0f);
			if(XMLElement* entity = pInfo->FirstChildElement("EntityComData")) {
				const char* targetable = entity->Attribute("_IsTargetable");
				if(targetable && EA::StdC::Stricmp(targetable, "false") == 0) creatureUntargetable[id] = true;
			}
			if(XMLElement* stats = pInfo->FirstChildElement("StatsComData")) {
				const i32 defence = stats->IntAttribute("_DFStatusIndex", 0);
				if(defence != 0) creatureDefenceStatus[id] = defence;
			}
		}
	}
	LOG("Loaded %d summon creatures, %d map hide detectors", (i32)summonMap.size(), (i32)creatureHideDetect.size());
	return true;
}

void GameXmlContent::LoadSummonInfo(XMLElement* pInfo)
{
	{
		i32 id = 0;
		pInfo->QueryAttribute("ID", &id);
		SummonInfo si;
		if(XMLElement* p = pInfo->FirstChildElement("EntityComData")) {
			p->QueryFloatAttribute("_MoveSpeed", &si.moveSpeed);
			si.normalMoveSpeed = p->FloatAttribute("_NormalMoveSpeed", si.moveSpeed);
			p->QueryFloatAttribute("_RotateSpeed", &si.rotateSpeed);
			p->QueryIntAttribute("_DensityCount", &si.density);
			const char* t = nullptr;
			p->QueryStringAttribute("_IsTargetable", &t);
			si.targetable = t && EA::StdC::Stricmp(t, "true") == 0;
		}
		if(XMLElement* p = pInfo->FirstChildElement("LifeTime_ComData")) p->QueryFloatAttribute("LifeTime", &si.lifeTime);
		if(XMLElement* p = pInfo->FirstChildElement("PhysXMeshComData")) p->QueryFloatAttribute("_ColliderRadius", &si.colliderRadius);
		if(XMLElement* p = pInfo->FirstChildElement("CreatureComData")) {
			p->QueryAttribute("_ClearComboOnDeath", &si.clearComboOnDeath);
			if(XMLElement* d = p->FirstChildElement("_HideDetector")) d->QueryFloatAttribute("_Radius", &si.hideDetectRadius);
		}
		if(XMLElement* p = pInfo->FirstChildElement("Kfm_ComData")) {
			const char* key = nullptr;
			p->QueryStringAttribute("KeyName", &key);
			if(key) si.className = key;
		}
		if(XMLElement* p = pInfo->FirstChildElement("Ai_ComData")) {
			p->QueryFloatAttribute("_PerceptionLength", &si.perception);
			p->QueryFloatAttribute("_TargetLostLength", &si.targetLost);
			const char* ai = nullptr;
			p->QueryStringAttribute("_AiIndex", &ai);
			if(ai && EA::StdC::Strcmp(ai, "AI_PC_MONDOZAX_ROBOT") == 0) si.ai = SummonInfo::AI_RELAY;
			else if(ai && (EA::StdC::Strcmp(ai, "AI_PC_ARTILLERY_SPIDERMINE") == 0 || EA::StdC::Strcmp(ai, "AI_PC_SOULMASTER_SLOWSOUL") == 0 ||
				EA::StdC::Strcmp(ai, "AI_PC_ELECTRO_ENERGYBALL") == 0)) si.ai = SummonInfo::AI_CHASE;
		}
		if(XMLElement* p = pInfo->FirstChildElement("MoveController_ComData")) p->QueryFloatAttribute("_AIDistanceWidth", &si.aiDistance);
		if(XMLElement* p = pInfo->FirstChildElement("StatsComData")) {
			const char* proof = p->Attribute("_AITargetProof");
			si.aiTargetProof = proof && EA::StdC::Stricmp(proof, "true") == 0;
			p->QueryAttribute("_VariationType", &si.variation);
			p->QueryFloatAttribute("_DestroyTime", &si.destroyTime);
			const char* dropType = p->Attribute("_DropType");
			if(dropType && EA::StdC::Strcmp(dropType, "DROP_TYPE_PUBLIC") == 0) p->QueryAttribute("_DropIndex", &si.publicDrop);
		}
		if(XMLElement* p = pInfo->FirstChildElement("SkillComData")) {
			for(XMLElement* s = p->FirstChildElement("_SkillID"); s; s = s->NextSiblingElement("_SkillID")) {
				i32 sk = 0;
				s->QueryAttribute("_Index", &sk);
				if(sk != 0) { ASSERT(!si.skills.full()); si.skills.push_back(sk); }
			}
		}
		summonMap[id] = si;
	}
}

bool GameXmlContent::LoadDeathRemotes()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/DeathRemotes.xml", doc)) return true;
	i32 count = 0;
	for(XMLElement* pD = doc.FirstChildElement()->FirstChildElement("DEATH"); pD; pD = pD->NextSiblingElement("DEATH")) {
		auto& list = deathRemotes[pD->IntAttribute("doc")];
		if(list.full()) continue;
		DeathRemote d;
		d.remote = pD->IntAttribute("remote");
		d.delay = pD->FloatAttribute("delay");
		d.radius = pD->FloatAttribute("radius");
		d.multiplier = pD->FloatAttribute("multiplier");
		ParseIdList(pD->Attribute("status"), d.statuses);
		const char* group = pD->Attribute("group");
		d.ally = group && EA::StdC::Strcmp(group, "ally") == 0;
		d.pc = pD->IntAttribute("pc") != 0;
		d.npc = pD->IntAttribute("npc") != 0;
		list.push_back(d);
		count++;
	}
	LOG("Loaded %d death remotes", count);
	return true;
}

static void ParseTrapTrack(const char* text, GameXmlContent::DynamicTrapTrack& out)
{
	const char* at = text;
	while(at && *at) {
		while(*at == ' ' || *at == '\n' || *at == '\r' || *at == '\t') at++;
		if(!*at) break;
		f32 x = 0, y = 0, z = 0, fx = 0, fy = 1;
		const i32 count = EA::StdC::Sscanf(at, "%f,%f,%f,%f,%f", &x, &y, &z, &fx, &fy);
		if(count != 3 && count != 5) break;
		out.points.push_back(vec3(x, y, z));
		if(count == 5) out.dirs.push_back(vec2(fx, fy));
		while(*at && *at != ' ' && *at != '\n' && *at != '\r' && *at != '\t') at++;
	}
	if(out.dirs.size() != out.points.size()) out.dirs.clear();
}

bool GameXmlContent::LoadDynamicTraps()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/DynamicTraps.xml", doc, true) || !doc.FirstChildElement()) return true;
	XMLElement* pRoot = doc.FirstChildElement();
	dynamicTrapBaseAttack = pRoot->FloatAttribute("baseAttack");
	dynamicTrapSpread = pRoot->FloatAttribute("spread");
	dynamicTrapStep = pRoot->FloatAttribute("step");
	i32 stateCount = 0;
	i32 carryCount = 0;
	i32 remoteCount = 0;
	i32 missing = 0;
	for(XMLElement* pModel = pRoot->FirstChildElement("MODEL"); pModel; pModel = pModel->NextSiblingElement("MODEL")) {
		dynamicTrapModels.push_back();
		DynamicTrapModel& model = dynamicTrapModels.back();
		model.kfm = pModel->Attribute("kfm") ? pModel->Attribute("kfm") : "";
		for(XMLElement* pTrack = pModel->FirstChildElement("TRACK"); pTrack; pTrack = pTrack->NextSiblingElement("TRACK")) {
			model.tracks.push_back();
			ParseTrapTrack(pTrack->GetText(), model.tracks.back());
		}
		for(XMLElement* pState = pModel->FirstChildElement("STATE"); pState && !model.states.full(); pState = pState->NextSiblingElement("STATE")) {
			DynamicTrapState st;
			const char* action = pState->Attribute("action");
			st.action = action ? ActionStateFromString(action) : ActionStateID::INVALID;
			st.length = pState->FloatAttribute("length");
			st.loop = pState->IntAttribute("loop") != 0;
			bool carry = true;
			for(XMLElement* pRemove = pState->FirstChildElement("REMOVE"); pRemove && !st.removes.full(); pRemove = pRemove->NextSiblingElement("REMOVE")) st.removes.push_back(pRemove->IntAttribute("doc"));
			for(XMLElement* pNode = pState->FirstChildElement("NODE"); pNode && !st.nodes.full(); pNode = pNode->NextSiblingElement("NODE")) {
				if(pNode->FirstChildElement("REMOTE")) carry = false;
				DynamicTrapNode node;
				for(const char* c = pNode->Attribute("name") ? pNode->Attribute("name") : "Scene Root"; *c && !node.name.full(); c++) node.name.push_back((wchar)*c);
				node.track = pNode->IntAttribute("track", -1);
				if(node.track >= (i32)model.tracks.size() || (node.track >= 0 && model.tracks[node.track].points.empty())) node.track = -1;
				for(XMLElement* pRemote = pNode->FirstChildElement("REMOTE"); pRemote && !st.remotes.full(); pRemote = pRemote->NextSiblingElement("REMOTE")) {
					DynamicTrapRemote r;
					r.remote = FindRemote((RemoteIdx)pRemote->IntAttribute("doc"));
					if(!r.remote) {
						missing++;
						continue;
					}
					r.node = (u8)st.nodes.size();
					r.delay = pRemote->FloatAttribute("delay");
					r.life = pRemote->FloatAttribute("life");
					if(r.remote->vs & (1 << Remote::VS_PLAYER_CHARACTER)) node.vsPlayer = true;
					if(r.remote->vs & (1 << Remote::VS_NPC_MONSTER)) node.vsMonster = true;
					st.remotes.push_back(r);
					remoteCount++;
				}
				st.nodes.push_back(node);
			}
			if(st.action == ActionStateID::INVALID || (st.remotes.empty() && !carry)) continue;
			model.states.push_back(st);
			if(carry) carryCount++;
			else stateCount++;
		}
	}
	for(XMLElement* pDoc = pRoot->FirstChildElement("DOC"); pDoc; pDoc = pDoc->NextSiblingElement("DOC")) {
		const char* kfm = pDoc->Attribute("kfm");
		if(!kfm) continue;
		foreach_const(m, dynamicTrapModels) {
			if(m->states.empty() || EA::StdC::Strcmp(m->kfm.c_str(), kfm) != 0) continue;
			DynamicTrap& trap = dynamicTraps[pDoc->IntAttribute("id")];
			trap.model = &*m;
			trap.ratio = pDoc->FloatAttribute("ratio", 1.0f);
			break;
		}
	}
	LOG("Loaded %d dynamic trap docs (%d models, %d states, %d carry states, %d remotes, %d unknown remotes), base attack %.0f", (i32)dynamicTraps.size(), (i32)dynamicTrapModels.size(), stateCount, carryCount, remoteCount, missing, dynamicTrapBaseAttack);
	return true;
}

bool GameXmlContent::LoadStageDifficulty()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/StageDifficulty.xml", doc, true)) return true;
	for(XMLElement* p = doc.FirstChildElement()->FirstChildElement("DIFFICULTY"); p; p = p->NextSiblingElement("DIFFICULTY")) {
		difficultyMonsterStat[p->IntAttribute("bit")] = p->FloatAttribute("monsterStat", 1.0f);
	}
	return true;
}

bool GameXmlContent::LoadStageEntries()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/StageEntries.xml", doc, true)) return true;
	for(XMLElement* pEntry = doc.FirstChildElement()->FirstChildElement("ENTRY"); pEntry && !stageEntries.full(); pEntry = pEntry->NextSiblingElement("ENTRY")) {
		stageEntries.push_back();
		StageEntry& entry = stageEntries.back();
		entry.ID = pEntry->IntAttribute("ID");
		for(XMLElement* pArea = pEntry->FirstChildElement("AREA"); pArea && !entry.areas.full(); pArea = pArea->NextSiblingElement("AREA")) {
			entry.areas.push_back();
			StageEntryArea& area = entry.areas.back();
			area.area = AreaIndex(pArea->IntAttribute("ID"));
			for(XMLElement* pStage = pArea->FirstChildElement("STAGE"); pStage && !area.stages.full(); pStage = pStage->NextSiblingElement("STAGE")) {
				area.stages.push_back();
				StageEntryStage& stage = area.stages.back();
				stage.stage = StageIndex(pStage->IntAttribute("ID"));
				const char* types = pStage->Attribute("gameTypes");
				i32 value = 0, read = 0;
				while(types && *types && !stage.gameTypes.full() && EA::StdC::Sscanf(types, "%d%n", &value, &read) == 1) {
					stage.gameTypes.push_back((u8)value);
					types += read;
				}
			}
		}
	}
	LOG("Loaded %d stage entries", (i32)stageEntries.size());
	return true;
}

bool GameXmlContent::LoadTradeItems()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/client/TRADEITEM.xml", doc)) return true;
	for(XMLElement* pInfo = doc.FirstChildElement()->FirstChildElement("INFO"); pInfo; pInfo = pInfo->NextSiblingElement("INFO")) {
		XMLElement* pTrade = pInfo->FirstChildElement("TRADEITEM");
		if(!pTrade) continue;
		TradeItem trade;
		trade.ID = pInfo->IntAttribute("ID");
		for(XMLElement* pReq = pTrade->FirstChildElement("_REQUIRED"); pReq && !trade.required.full(); pReq = pReq->NextSiblingElement("_REQUIRED")) {
			trade.required.push_back(eastl::make_pair(pReq->IntAttribute("_ItemIndex"), pReq->IntAttribute("_Amount")));
		}
		if(XMLElement* pBuy = pTrade->FirstChildElement("_BUY")) {
			trade.buyItem = pBuy->IntAttribute("_DataKey");
			const char* dataType = pBuy->Attribute("_DataType");
			trade.buyDrop = dataType && EA::StdC::Strcmp(dataType, "DropIndex") == 0;
		}
		if(XMLElement* pCost = pTrade->FirstChildElement("_COST")) {
			const char* type = pCost->Attribute("_Type");
			trade.costType = type && EA::StdC::Strcmp(type, "BUY_TYPE_CCOIN") == 0 ? 2 : 1;
			trade.costAmount = pCost->IntAttribute("_Amount");
		}
		tradeItems[trade.ID] = trade;
	}
	LOG("Loaded %d trade items", (i32)tradeItems.size());
	return true;
}

bool GameXmlContent::LoadSightData()
{
	static const wchar* CREATURE_FILES[] = { L"/CREATURE_CHARACTER.xml", L"/client/CREATURE_MONSTER_PvP.xml", L"/client/CREATURE_MONSTER_SUMMON.xml" };
	for(const wchar* file : CREATURE_FILES) {
		XMLDocument doc;
		if(!LoadXMLFile(file, doc, true)) continue;
		for(XMLElement* pInfo = doc.FirstChildElement()->FirstChildElement(); pInfo; pInfo = pInfo->NextSiblingElement()) {
			if(XMLElement* pKfm = pInfo->FirstChildElement("Kfm_ComData")) if(pKfm->Attribute("KeyName")) creatureKfm[pInfo->IntAttribute("ID")] = pKfm->Attribute("KeyName");
			XMLElement* pEntity = pInfo->FirstChildElement("EntityComData");
			const char* bushPass = pEntity ? pEntity->Attribute("_bBushPass") : nullptr;
			if(bushPass && EA::StdC::Stricmp(bushPass, "true") == 0) creatureBushPass.insert(pInfo->IntAttribute("ID"));
			const char* sight = pEntity ? pEntity->Attribute("_SightRadiusType") : nullptr;
			if(!sight) continue;
			i32 meters = 0;
			EA::StdC::Sscanf(sight, "SIGHT_RADIUS_TYPE_%dM", &meters);
			creatureSight[pInfo->IntAttribute("ID")] = (f32)meters * 100.0f;
		}
	}
	XMLDocument xmlDynamic;
	if(LoadXMLFile(L"/client/DYNAMIC.xml", xmlDynamic)) {
		for(XMLElement* pInfo = xmlDynamic.FirstChildElement()->FirstChildElement(); pInfo; pInfo = pInfo->NextSiblingElement()) {
			XMLElement* pDyn = pInfo->FirstChildElement("DynamicComData");
			XMLElement* pEntity = pInfo->FirstChildElement("EntityComData");
			const char* type = pDyn ? pDyn->Attribute("_Type") : nullptr;
			const char* key = pEntity ? pEntity->Attribute("KEYNAME") : nullptr;
			if(type && EA::StdC::Strcmp(type, "DYNAMIC_TYPE_GATE") == 0 && pDyn->BoolAttribute("_TeamPhysics")) teamGateDocs.insert(pInfo->IntAttribute("ID"));
			if(!type || !key || EA::StdC::Strcmp(type, "DYNAMIC_TYPE_BUSH") != 0) continue;
			const char* size = EA::StdC::Strrchr(key, '_');
			i32 w = 0, l = 0;
			if(size && EA::StdC::Sscanf(size + 1, "%dx%d", &w, &l) == 2) bushSize[pInfo->IntAttribute("ID")] = vec2((f32)w * 100.0f, (f32)l * 100.0f);
		}
	}
	LOG("Loaded sight radius of %d creatures, %d bush types, %d team gates", (i32)creatureSight.size(), (i32)bushSize.size(), (i32)teamGateDocs.size());
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
			if(!type) continue;
			if(EA::StdC::Strcmp(type, "E_CONDITION_TYPE_PVP_SCORE") == 0) {
				const i32 score = pC->IntAttribute("_Value");
				if(score > 0) stageScoreLimit[id] = score;
				continue;
			}
			if(EA::StdC::Strcmp(type, "E_CONDITION_TYPE_TIME_COUNT") != 0) continue;
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
	if(!LoadXMLFile(L"/AniLength.xml", xmlAni)) return false;
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
			for(const char* c = h; c && *c; ) {
				char* end = nullptr;
				const f32 v = EA::StdC::StrtoF32(c, &end);
				if(end == c) break;
				ASSERT(!curve.h.full());
				curve.h.push_back(v);
				if(fabsf(v) > curve.maxAbs) curve.peak = v;
				curve.maxAbs = MAX(curve.maxAbs, fabsf(v));
				c = end;
			}
			if(id != 0 && !curve.h.empty()) moveCurves[AniKey(type, id)] = curve;
		}
	}
	LOG("Loaded %d move graphs", (i32)moveCurves.size());
	return true;
}

static u8 ConsumableStatType(const char* name)
{
	static const struct { const char* name; StatType type; } STATS[] = {
		{ "STAT_TYPE_HEALTH", StatType::STAT_TYPE_HEALTH },
		{ "STAT_TYPE_STAMINA", StatType::STAT_TYPE_STAMINA },
		{ "STAT_TYPE_ENERGY", StatType::STAT_TYPE_ENERGY },
		{ "STAT_TYPE_MANA", StatType::STAT_TYPE_MANA },
		{ "STAT_TYPE_RAGE", StatType::STAT_TYPE_RAGE },
		{ "STAT_TYPE_ULTIMATE", StatType::STAT_TYPE_ULTIMATE },
	};
	if(!name) return 0xff;
	for(const auto& s : STATS) if(EA::StdC::Strcmp(name, s.name) == 0) return (u8)s.type;
	return 0xff;
}

void GameXmlContent::LoadItemInfos(XMLDocument& doc)
{
	for(XMLElement* pInfo = doc.FirstChildElement()->FirstChildElement("INFO"); pInfo; pInfo = pInfo->NextSiblingElement("INFO")) {
		XMLElement* common = pInfo->FirstChildElement("ItemComData");
		if(!common) continue;
		ItemInfo item;
		pInfo->QueryAttribute("ID", &item.ID);
		item.maxStack = MAX(1, common->IntAttribute("_MaxStack", 1));
		static const i32 MEASURED_GEAR_FIRST_SLOT = 100;
		if(pInfo->FirstChildElement("GearComData")) item.firstSlot = MEASURED_GEAR_FIRST_SLOT;
		static const i32 MEASURED_MATERIAL_FIRST_SLOT = 300;
		XMLElement* etc = pInfo->FirstChildElement("EtcItemComData");
		const char* etcType = etc ? etc->Attribute("_Type") : nullptr;
		if(etcType && EA::StdC::Strcmp(etcType, "ETC_TYPE_INGREDIENT") == 0) item.firstSlot = MEASURED_MATERIAL_FIRST_SLOT;
		if(etcType && EA::StdC::Strcmp(etcType, "ETC_TYPE_BOX") == 0) item.boxDrop = etc->IntAttribute("_DropIndex");
		const char* buyType = common->Attribute("_BuyType");
		if(buyType && EA::StdC::Strcmp(buyType, "BUY_TYPE_GOLD") == 0) item.buyType = 1;
		else if(buyType && EA::StdC::Strcmp(buyType, "BUY_TYPE_CCOIN") == 0) item.buyType = 2;
		item.buyValue = common->IntAttribute("_BuyValue", 0);
		item.sellValue = common->IntAttribute("_SellValue", 0);
		itemInfoMap[item.ID] = item;
	}
}

static const i32 MAX_DROP_DEPTH = 8;
static const i32 NORMAL_DIFFICULTY_INDEX = 1;

static u8 GearStatType(const char* name)
{
	static const struct { const char* name; StatType type; } STATS[] = {
		{ "STAT_TYPE_HEALTH", StatType::STAT_TYPE_HEALTH },
		{ "STAT_TYPE_HEALTHREGEN", StatType::STAT_TYPE_HEALTHREGEN },
		{ "STAT_TYPE_ATTACK", StatType::STAT_TYPE_ATTACK },
		{ "STAT_TYPE_DEFENCE", StatType::STAT_TYPE_DEFENCE },
		{ "STAT_TYPE_SKILLPOWER", StatType::STAT_TYPE_SKILLPOWER },
		{ "STAT_TYPE_ENERGY", StatType::STAT_TYPE_ENERGY },
		{ "STAT_TYPE_PICKUP_RADIUS", StatType::STAT_TYPE_PICKUP_RADIUS },
		{ "STAT_TYPE_MANA", StatType::STAT_TYPE_MANA },
		{ "STAT_TYPE_ULTIMATE", StatType::STAT_TYPE_ULTIMATE },
	};
	if(!name) return 0xff;
	for(const auto& s : STATS) if(EA::StdC::Strcmp(name, s.name) == 0) return (u8)s.type;
	return 0xff;
}

i32 GameXmlContent::GearKey(const char* gearType, const char* rank)
{
	static const char* GEAR_TYPES[] = { "GEAR_TYPE_ATT", "GEAR_TYPE_DEF", "GEAR_TYPE_UTIL" };
	static const char* ITEM_RANKS[] = { "ITEM_RANK_NORMAL", "ITEM_RANK_RARE", "ITEM_RANK_UNIQUE", "ITEM_RANK_HEROIC", "ITEM_RANK_LEGENDARY" };
	if(!gearType || !rank) return -1;
	i32 typeIndex = -1;
	i32 rankIndex = -1;
	for(i32 i = 0; i < (i32)ARRAY_COUNT(GEAR_TYPES); i++) if(EA::StdC::Strcmp(gearType, GEAR_TYPES[i]) == 0) typeIndex = i;
	for(i32 i = 0; i < (i32)ARRAY_COUNT(ITEM_RANKS); i++) if(EA::StdC::Strcmp(rank, ITEM_RANKS[i]) == 0) rankIndex = i;
	if(typeIndex < 0 || rankIndex < 0) return -1;
	return typeIndex * (i32)ARRAY_COUNT(ITEM_RANKS) + rankIndex;
}

u8 GameXmlContent::RankID(const char* name)
{
	for(i32 i = 0; i < (i32)rankNames.size(); i++) if(rankNames[i] == name) return (u8)i;
	if(rankNames.full()) return NO_RANK;
	rankNames.push_back(name);
	return (u8)(rankNames.size() - 1);
}

static GameType GameTypeFromString(const char* name)
{
	static const struct { const char* name; GameType type; } TYPES[] = {
		{ "GAME_TYPE_PVE_NORMAL", GameType::PVE_NORMAL },
		{ "GAME_TYPE_PVE_RANDOM", GameType::PVE_RANDOM },
		{ "GAME_TYPE_PVE_TICKET", GameType::PVE_TICKET },
		{ "GAME_TYPE_PVP_NORMAL", GameType::PVP_NORMAL },
		{ "GAME_TYPE_PVP_RANK", GameType::PVP_RANK },
		{ "GAME_TYPE_PVP_AI", GameType::PVP_AI },
		{ "GAME_TYPE_PVP_NORMAL_FFA", GameType::PVP_NORMAL_FFA },
		{ "GAME_TYPE_PVP_GUILD", GameType::PVP_GUILD },
		{ "GAME_TYPE_PVP_FAKE_AI", GameType::PVP_FAKE_AI },
	};
	for(const auto& t : TYPES) if(strcmp(t.name, name) == 0) return t.type;
	return GameType::INVALID;
}

bool GameXmlContent::LoadGameEvents()
{
	XMLDocument schedule;
	if(!LoadXMLFile(L"/GameEventSchedule.xml", schedule, true)) return true;
	eastl::fixed_set<i32,16,false> active;
	for(XMLElement* a = schedule.FirstChildElement()->FirstChildElement("_Active"); a; a = a->NextSiblingElement("_Active")) {
		if(active.size() < activeGameEvents.capacity()) active.insert(a->IntAttribute("_Index", 0));
	}
	if(active.empty()) return true;
	XMLDocument doc;
	if(!LoadXMLFile(L"/client/GAME_EVENT.xml", doc)) return false;
	for(XMLElement* pInfo = doc.FirstChildElement()->FirstChildElement("INFO"); pInfo; pInfo = pInfo->NextSiblingElement("INFO")) {
		XMLElement* ev = pInfo->FirstChildElement("ST_GAME_EVENT");
		if(!ev || !active.count(ev->IntAttribute("_Index", 0)) || activeGameEvents.full()) continue;
		GameEvent event;
		event.index = ev->IntAttribute("_Index", 0);
		for(XMLElement* e = ev->FirstChildElement("_EFFECT"); e; e = e->NextSiblingElement("_EFFECT")) {
			const char* type = e->Attribute("_Type");
			const char* calc = e->Attribute("_CalcType");
			if(!type || !calc || strcmp(calc, "ADD") != 0) continue;
			if(strcmp(type, "GAME_EVENT_TYPE_EXP_BONUS") == 0) event.expBonusPercent += e->IntAttribute("_Data", 0);
			if(strcmp(type, "GAME_EVENT_TYPE_RON_DROP_BONUS") == 0) event.ronBonusPercent += e->IntAttribute("_Data", 0);
		}
		if(XMLElement* cond = ev->FirstChildElement("_TRIGGER_CONDITION")) {
			for(XMLElement* t = cond->FirstChildElement("_TargetGameType"); t; t = t->NextSiblingElement("_TargetGameType")) {
				const char* name = t->Attribute("DATA");
				const GameType gt = name ? GameTypeFromString(name) : GameType::INVALID;
				if(gt != GameType::INVALID && !event.gameTypes.full()) event.gameTypes.push_back(gt);
			}
			for(XMLElement* s = cond->FirstChildElement("_TargetStage"); s; s = s->NextSiblingElement("_TargetStage")) event.stages.push_back(s->IntAttribute("DATA", 0));
		}
		activeGameEvents.push_back(event);
		LOG("GameContent :: active game event %d (exp +%d%%, gold +%d%%, %d game types, %d stages)", event.index, event.expBonusPercent, event.ronBonusPercent, (i32)event.gameTypes.size(), (i32)event.stages.size());
	}
	return true;
}

void GameXmlContent::ActiveEventBonus(GameType type, StageIndex stage, i32* expPercent, i32* ronPercent) const
{
	*expPercent = 0;
	*ronPercent = 0;
	foreach_const(ev, activeGameEvents) {
		if(!ev->gameTypes.empty() && eastl::find(ev->gameTypes.begin(), ev->gameTypes.end(), type) == ev->gameTypes.end()) continue;
		if(!ev->stages.empty() && eastl::find(ev->stages.begin(), ev->stages.end(), (i32)stage) == ev->stages.end()) continue;
		*expPercent += ev->expBonusPercent;
		*ronPercent += ev->ronBonusPercent;
	}
}

bool GameXmlContent::LoadMonsterRanks()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/MonsterRanks.xml", doc)) return false;
	for(XMLElement* m = doc.FirstChildElement()->FirstChildElement("M"); m; m = m->NextSiblingElement("M")) {
		const char* rank = m->Attribute("rank");
		if(!rank) continue;
		const i32 id = m->IntAttribute("ID");
		monsterRank[id] = RankID(rank);
		if(const i32 drop = m->IntAttribute("drop")) monsterDrop.insert(eastl::make_pair(id, drop));
	}
	LOG("Loaded %d monster ranks (%d rank names)", (i32)monsterRank.size(), (i32)rankNames.size());
	return true;
}

void GameXmlContent::RollDrop(i32 dropIndex, DropRollList* out, i32 depth) const
{
	const DropGroup* group = FindDropGroup(dropIndex);
	if(!group || depth > MAX_DROP_DEPTH) return;
	const DropGroup::Entry* picked = nullptr;
	if(group->piece) {
		const f32 roll = (f32)(RandUint() % 10000) / 100.0f;
		f32 cumulative = 0;
		foreach_const(e, group->entries) {
			cumulative += e->rate;
			if(roll < cumulative) { picked = &*e; break; }
		}
		if(!picked) return;
	}
	foreach_const(e, group->entries) {
		if(picked && &*e != picked) continue;
		const i32 count = e->minCount + (e->maxCount > e->minCount ? (i32)(RandUint() % (u32)(e->maxCount - e->minCount + 1)) : 0);
		if(count <= 0) continue;
		if(e->kind == DropGroup::Kind::GROUP) {
			for(i32 k = 0; k < count; k++) RollDrop(e->item, out, depth + 1);
			continue;
		}
		i32 item = e->kind == DropGroup::Kind::RON ? 0 : e->item;
		if(e->kind == DropGroup::Kind::GEAR) {
			auto docs = gearDocs.find(e->item);
			if(docs == gearDocs.end() || docs->second.empty()) continue;
			item = docs->second[RandUint() % (u32)docs->second.size()];
		}
		if(out->full()) return;
		DropRoll r;
		r.item = item;
		r.count = count;
		r.ron = e->kind == DropGroup::Kind::RON;
		out->push_back(r);
	}
}

bool GameXmlContent::LoadGroundItems()
{
	XMLDocument items;
	if(!LoadXMLFile(L"/client/ITEM_BASIC.xml", items)) return false;
	LoadItemInfos(items);
	const wchar* extraItemFiles[] = { L"/client/ITEM_SUPPORT.xml", L"/client/ITEM_RON.xml" };
	for(const wchar* file : extraItemFiles) {
		XMLDocument extra;
		if(!LoadXMLFile(file, extra)) return false;
		LoadItemInfos(extra);
		for(XMLElement* pInfo = extra.FirstChildElement()->FirstChildElement("INFO"); pInfo; pInfo = pInfo->NextSiblingElement("INFO")) {
			XMLElement* kit = pInfo->FirstChildElement("SupportKitItemComData");
			XMLElement* common = pInfo->FirstChildElement("ItemComData");
			const char* type = kit ? kit->Attribute("_Type") : nullptr;
			if(!kit || !common || !type) continue;
			SupportKit k;
			if(EA::StdC::Strcmp(type, "HP") == 0) k.hpPercent = kit->IntAttribute("_Value");
			k.coolTime = common->FloatAttribute("_CoolTime");
			k.coolTimeID = common->IntAttribute("_CoolTimeID");
			for(XMLElement* st = kit->FirstChildElement("_STATUS"); st && !k.statuses.full(); st = st->NextSiblingElement("_STATUS")) k.statuses.push_back(st->IntAttribute("DATA"));
			supportKits[pInfo->IntAttribute("ID")] = k;
		}
	}
	for(XMLElement* pInfo = items.FirstChildElement()->FirstChildElement("INFO"); pInfo; pInfo = pInfo->NextSiblingElement("INFO")) {
		XMLElement* gear = pInfo->FirstChildElement("GearComData");
		XMLElement* itemData = pInfo->FirstChildElement("ItemComData");
		if(gear && itemData) {
			const i32 key = GearKey(gear->Attribute("_Type"), itemData->Attribute("_Rank"));
			if(key >= 0) gearDocs[key].push_back(pInfo->IntAttribute("ID"));
			static const i32 MEASURED_GEAR_SLOT_TYPES[] = { 60, 70, 80 };
			GearItem& g = gearItems[pInfo->IntAttribute("ID")];
			if(key >= 0) g.slotType = MEASURED_GEAR_SLOT_TYPES[key / 5];
			if(XMLElement* effect = itemData->FirstChildElement("_Effect")) {
				for(XMLElement* e = effect->FirstChildElement(); e; e = e->NextSiblingElement()) {
					const u8 type = GearStatType(e->Attribute("DATAKEY"));
					if(type == 0xff) continue;
					StatValue v = { type, e->FloatAttribute("DATA") };
					if(EA::StdC::Strcmp(e->Name(), "_STAT_MUL") == 0 && !g.statMul.full()) g.statMul.push_back(v);
					else if(EA::StdC::Strcmp(e->Name(), "_STAT_ADD") == 0 && !g.statAdd.full()) g.statAdd.push_back(v);
				}
			}
			continue;
		}
		XMLElement* etc = pInfo->FirstChildElement("EtcItemComData");
		const char* etcType = etc ? etc->Attribute("_Type") : nullptr;
		if(!etcType || EA::StdC::Strcmp(etcType, "ETC_TYPE_FOOD") != 0) continue;
		GroundItemInfo info;
		pInfo->QueryAttribute("ID", &info.id);
		if(XMLElement* item = pInfo->FirstChildElement("ItemComData")) {
			if(XMLElement* effect = item->FirstChildElement("_Effect")) {
				for(XMLElement* e = effect->FirstChildElement(); e; e = e->NextSiblingElement()) {
					const u8 type = ConsumableStatType(e->Attribute("DATAKEY"));
					if(type == 0xff) continue;
					StatValue v = { type, e->FloatAttribute("DATA") };
					if(EA::StdC::Strcmp(e->Name(), "_STAT_MUL") == 0 && !info.statMul.full()) info.statMul.push_back(v);
					else if(EA::StdC::Strcmp(e->Name(), "_STAT_ADD") == 0 && !info.statAdd.full()) info.statAdd.push_back(v);
				}
			}
		}
		for(XMLElement* s = etc->FirstChildElement("_StatusIndex"); s; s = s->NextSiblingElement("_StatusIndex")) {
			if(!info.statuses.full()) info.statuses.push_back(s->IntAttribute("DATA"));
		}
		if(XMLElement* life = pInfo->FirstChildElement("LifeTime_ComData")) life->QueryFloatAttribute("LifeTime", &info.lifeTime);
		groundItemMap[info.id] = info;
	}

	XMLDocument drops;
	if(!LoadXMLFile(L"/client/DROPITEM.xml", drops)) return false;
	for(XMLElement* pInfo = drops.FirstChildElement()->FirstChildElement("INFO"); pInfo; pInfo = pInfo->NextSiblingElement("INFO")) {
		XMLElement* common = pInfo->FirstChildElement("ST_COMMONDROPITEMINFO");
		XMLElement* list = common ? common->FirstChildElement("_DROPITEMLIST") : nullptr;
		if(!list) continue;
		DropGroup group;
		const char* dropType = list->Attribute("_Droptype");
		group.piece = dropType && EA::StdC::Strcmp(dropType, "DROP_PIECE") == 0;
		for(XMLElement* e = list->FirstChildElement("_DROPITEMINFO"); e && !group.entries.full(); e = e->NextSiblingElement("_DROPITEMINFO")) {
			const char* dataType = e->Attribute("_DataType");
			if(!dataType) continue;
			DropGroup::Entry entry;
			if(EA::StdC::Strcmp(dataType, "ItemIndex") == 0) entry.kind = DropGroup::Kind::ITEM;
			else if(EA::StdC::Strcmp(dataType, "Ron") == 0) entry.kind = DropGroup::Kind::RON;
			else if(EA::StdC::Strcmp(dataType, "DropIndex") == 0) entry.kind = DropGroup::Kind::GROUP;
			else if(EA::StdC::Strcmp(dataType, "GearComData") == 0) entry.kind = DropGroup::Kind::GEAR;
			else continue;
			entry.item = entry.kind == DropGroup::Kind::GEAR ? GearKey(e->Attribute("_DataKey"), e->Attribute("_RankMin")) : e->IntAttribute("_DataKey");
			if(entry.item < 0) continue;
			entry.minCount = e->IntAttribute("_Min", 1);
			entry.maxCount = e->IntAttribute("_Max", 1);
			entry.rate = e->FloatAttribute("_DropRate", 100.0f);
			group.entries.push_back(entry);
		}
		if(!group.entries.empty()) dropGroupMap[pInfo->IntAttribute("ID")] = group;
	}
	LOG("Loaded %d ground items, %d drop groups", (i32)groundItemMap.size(), (i32)dropGroupMap.size());
	return true;
}

static XMLElement* FirstChildOf(XMLElement* parent, const char* a, const char* b)
{
	XMLElement* e = parent->FirstChildElement(a);
	return e ? e : parent->FirstChildElement(b);
}

static bool HasLevelEventInfo(XMLElement* node)
{
	for(XMLElement* e = node->FirstChildElement(); e; e = e->NextSiblingElement()) {
		if(EA::StdC::Strcmp(e->Name(), "_InputLink") != 0 && EA::StdC::Strcmp(e->Name(), "_OutputLink") != 0 && EA::StdC::Strcmp(e->Name(), "_LevelEventData") != 0) return true;
	}
	return false;
}

bool GameXmlContent::LoadLevelEvents(Map* map, const wchar* levelDir)
{
	XMLDocument doc;
	if(!LoadXMLFile(LFMT(L"/%s/EVENTNODES/LEVELEVENT_SERVER.XML", levelDir), doc, true)) return true;
	XMLElement* list = doc.FirstChildElement() ? doc.FirstChildElement()->FirstChildElement() : nullptr;
	if(!list) return true;
	for(XMLElement* pNode = list->FirstChildElement("_LevelEventData"); pNode; pNode = pNode->NextSiblingElement("_LevelEventData")) {
		LevelEventNode node;
		node.id = pNode->IntAttribute("_ConvertedEventDataID");
		node.nameKey = StatusTypeKey(pNode->Attribute("_EventDataID"));
		node.difficultyMask = (u8)pNode->IntAttribute("_StageDifficultyMask", ALL_DIFFICULTIES);
		if(const char* group = pNode->Attribute("_GroupTag")) node.groupKey = *group ? StatusTypeKey(group) : 0;
		for(XMLElement* in = pNode->FirstChildElement("_InputLink"); in && !node.inputs.full(); in = in->NextSiblingElement("_InputLink")) node.inputs.push_back(eastl::make_pair(in->IntAttribute("_ConvertedEventDataID"), (u8)in->IntAttribute("_StageDifficultyMask", ALL_DIFFICULTIES)));
		for(XMLElement* out = pNode->FirstChildElement("_OutputLink"); out; out = out->NextSiblingElement("_OutputLink")) node.outputs.push_back(out->IntAttribute("_ConvertedEventDataID"));
		if(pNode->FirstChildElement("_LevelStartTriggerData")) node.kind = LevelEventNode::LEVEL_START;
		else if(XMLElement* e = pNode->FirstChildElement("_TakeItemTriggerData")) { node.kind = LevelEventNode::TAKE_ITEM; node.tagID = e->IntAttribute("_TagID", -1); }
		else if(XMLElement* e = pNode->FirstChildElement("_DelaySeqInfo")) { node.kind = LevelEventNode::DELAY; node.seqRoot = true; node.delay = e->FloatAttribute("_DelayTime"); node.variance = e->FloatAttribute("_RandomDelayTimeVariance"); }
		else if(pNode->FirstChildElement("_CallSFXInfo")) node.kind = LevelEventNode::CALL_SFX;
		else if(XMLElement* e = pNode->FirstChildElement("_InitCoolTime")) { node.kind = LevelEventNode::CLIENT_EVENT; node.initTagCool = e->IntAttribute("_CharacterTag") != 0; }
		else if(XMLElement* e = pNode->FirstChildElement("_CallItemInfo")) { node.kind = LevelEventNode::CALL_ITEM; node.dropIndex = e->IntAttribute("_Index"); node.areaID = e->IntAttribute("_SpawnAreaID"); node.tagID = e->IntAttribute("_TagID", -1); }
		else if(XMLElement* e = FirstChildOf(pNode, "_BoundTriggerData", "_PlayerBoundTriggerData")) { node.kind = LevelEventNode::BOUND_TRIGGER; node.areaID = e->IntAttribute("_AreaID"); node.enter = e->IntAttribute("_IsEnter") == 1; node.active = e->IntAttribute("_Active") == 1; node.retrigger = e->FloatAttribute("_RetriggerDelay"); node.maxTriggers = e->IntAttribute("_MaxTriggerCount"); const char* trigger = e->Attribute("_TrigerType"); node.ownTeamOnly = trigger && EA::StdC::Strcmp(trigger, "TRIGER_MEMBERSHIP_OWN") == 0; }
		else if(XMLElement* e = pNode->FirstChildElement("_StatusChangeInfo")) {
			node.kind = LevelEventNode::STATUS_CHANGE;
			const char* applyType = e->Attribute("_ApplyType");
			node.applyType = !applyType ? LevelEventNode::APPLY_OTHER : EA::StdC::Strcmp(applyType, "APPLY_MY_CHAR") == 0 ? LevelEventNode::APPLY_MY_CHAR : EA::StdC::Strcmp(applyType, "APPLY_LOCAL_ID") == 0 ? LevelEventNode::APPLY_LOCAL_ID : EA::StdC::Strcmp(applyType, "APPLY_ALL_PARTY") == 0 ? LevelEventNode::APPLY_ALL_PARTY : LevelEventNode::APPLY_OTHER;
			if(node.applyType == LevelEventNode::APPLY_ALL_PARTY) { const char* teamType = e->Attribute("_TeamType"); node.team = teamType ? StringToFaction(teamType) : Faction::INVALID; }
			node.apply = e->IntAttribute("_bApply") == 1;
			node.localID = e->IntAttribute("_LocalID");
			node.tagID = e->IntAttribute("_TagID", -1);
			node.statusIndex = e->IntAttribute("_StatusIndex");
		}
		else if(XMLElement* e = pNode->FirstChildElement("_OnHitTriggerData")) { node.kind = LevelEventNode::ON_HIT_TRIGGER; node.localID = e->IntAttribute("_LocalID"); node.delay = e->FloatAttribute("_DelayTime"); node.hitCount = e->IntAttribute("_HitCount"); }
		else if(XMLElement* e = pNode->FirstChildElement("_ActionChangeInfo")) { node.kind = LevelEventNode::ACTION_CHANGE; node.localID = e->IntAttribute("_ID"); if(const char* a = e->Attribute("_ActionIndex")) node.action = ActionStateFromString(a); }
		else if(XMLElement* e = pNode->FirstChildElement("_ActiveEventTriggerInfo")) { node.kind = LevelEventNode::ACTIVATE_TRIGGER; node.targetKey = StatusTypeKey(e->Attribute("_TriggerID")); node.active = e->IntAttribute("_Active") == 1; }
		else if(XMLElement* e = FirstChildOf(pNode, "_CallMonsterInfo", "_RelativeCallMonsterInfo")) {
			node.kind = LevelEventNode::CALL_MONSTER;
			node.relative = pNode->FirstChildElement("_RelativeCallMonsterInfo") != nullptr;
			static const char* STAGE_LEVEL_NAMES[] = { "EASY", "NORMAL", "HARD", "EXTREME", "HELL" };
			if(const char* level = e->Attribute("_StageLevel")) {
				for(i32 d = 0; d < (i32)ARRAY_COUNT(STAGE_LEVEL_NAMES); d++) if(EA::StdC::Strcmp(level, STAGE_LEVEL_NAMES[d]) == 0) node.stageLevelBit = (u8)(1 << d);
			}
			node.monster = e->IntAttribute("_Index");
			node.count = MAX(e->IntAttribute("_Count", 1), 1);
			node.callCount = MAX(e->IntAttribute("_CallCount", 1), 1);
			node.callDelay = e->FloatAttribute("_CallDelayTime");
			node.callInterval = e->FloatAttribute("_CallIntervalTime");
			node.variance = e->FloatAttribute("_RandomDelayTimeVariance");
			node.areaID = e->IntAttribute("_SpawnAreaID");
			node.tagID = e->IntAttribute("_TagID", -1);
			node.spawnAnim = e->IntAttribute("_SpawnAnim");
			node.wander = e->IntAttribute("_AIWanderDistOverride", -1);
			if(const char* team = e->Attribute("_Team")) node.team = StringToFaction(team);
		}
		else if(XMLElement* e = pNode->FirstChildElement("_DespawnEntityInfo")) { node.kind = LevelEventNode::DESPAWN_ENTITY; node.localID = e->IntAttribute("_LocalID"); node.tagID = e->IntAttribute("_TagID", -1); }
		else if(pNode->FirstChildElement("_RepeatInfo")) node.kind = LevelEventNode::PASS_THROUGH;
		else if(XMLElement* e = pNode->FirstChildElement("_SafeZoneInfo")) { node.kind = LevelEventNode::SAFE_ZONE; node.active = e->IntAttribute("_Enable") == 1; }
		else if(XMLElement* e = pNode->FirstChildElement("_DeadCountTriggerData")) {
			node.kind = LevelEventNode::DEAD_COUNT;
			node.tagID = e->IntAttribute("_TagID", -1);
			node.count = MAX(e->IntAttribute("_Count", 1), 1);
			node.active = e->IntAttribute("_Active", 1) == 1;
			if(const char* team = e->Attribute("_Team")) node.team = StringToFaction(team);
		}
		else if(XMLElement* e = pNode->FirstChildElement("_HPTriggerData")) { node.kind = LevelEventNode::HP_TRIGGER; node.tagID = e->IntAttribute("_TagID", -1); node.localID = e->IntAttribute("_LocalID"); node.hpPercent = e->FloatAttribute("_HP"); node.active = true; }
		else if(XMLElement* e = pNode->FirstChildElement("_EventOrConditionInfo")) { node.kind = LevelEventNode::OR_CONDITION; node.resetAfter = e->IntAttribute("_ResetAfterEvent") == 1; node.active = true; }
		else if(XMLElement* e = pNode->FirstChildElement("_GameEndTriggerData")) { node.kind = LevelEventNode::GAME_END_TRIGGER; if(const char* team = e->Attribute("_Team")) node.team = StringToFaction(team); }
		else if(pNode->FirstChildElement("_TimeRecordStartTriggerData")) node.kind = LevelEventNode::TIME_RECORD_START;
		else if(XMLElement* e = pNode->FirstChildElement("_EventTeamConditionInfo")) { node.kind = LevelEventNode::TEAM_CONDITION; if(const char* team = e->Attribute("_Team")) node.team = StringToFaction(team); }
		else if(XMLElement* e = pNode->FirstChildElement("_AddGotPieceInfo")) { node.kind = LevelEventNode::ADD_GOT_PIECE; node.count = e->IntAttribute("_Count", 1); if(const char* team = e->Attribute("_TeamType")) node.team = StringToFaction(team); }
		else if(pNode->FirstChildElement("_SaveGotPieceInfo")) node.kind = LevelEventNode::SAVE_GOT_PIECE;
		else if(XMLElement* e = pNode->FirstChildElement("_AITriggerData")) { node.kind = LevelEventNode::AI_TRIGGER; node.aiIndex = e->IntAttribute("_INDEX"); }
		else if(XMLElement* e = pNode->FirstChildElement("_InvokeAITriggerInfo")) { node.kind = LevelEventNode::INVOKE_AI_TRIGGER; node.aiIndex = e->IntAttribute("_INDEX"); }
		else if(XMLElement* e = pNode->FirstChildElement("_ActiveInteractionInfo")) { node.kind = LevelEventNode::ACTIVE_INTERACTION; node.localID = e->IntAttribute("_LocalID"); node.active = e->IntAttribute("_Active") == 1; }
		else if(XMLElement* e = pNode->FirstChildElement("_TeleportFromStartingPointInfo")) {
			node.kind = LevelEventNode::CAMP_GUARD;
			node.localID = e->IntAttribute("_StartingAreaID");
			node.areaID = e->IntAttribute("_AreaID");
			node.delay = e->FloatAttribute("_WarningMessageDelayTime");
			node.callDelay = e->FloatAttribute("_TeleportDelayTime");
			node.apply = e->IntAttribute("_bApply") == 1;
		}
		else if(XMLElement* e = pNode->FirstChildElement("_TeleportToInfo")) {
			node.kind = LevelEventNode::TELEPORT_TO;
			node.areaID = e->IntAttribute("_AreaID");
			node.apply = false;
			if(XMLElement* t = e->FirstChildElement("_TargetEntityInfo")) node.apply = t->IntAttribute("_IsAllPlayer") == 1;
		}
		else if(XMLElement* e = pNode->FirstChildElement("_InteractionTriggerData")) { node.kind = LevelEventNode::INTERACTION_TRIGGER; node.localID = e->IntAttribute("_TargetID"); }
		else if(XMLElement* e = pNode->FirstChildElement("_AddInteractionCastingTimeInfo")) { node.kind = LevelEventNode::ADD_CAST_TIME; node.localID = e->IntAttribute("_LocalID"); node.delay = e->FloatAttribute("_Time"); if(const char* team = e->Attribute("_TeamType")) node.team = EA::StdC::Strcmp(team, "TEAM_GREEN") == 0 ? Faction::MONSTER : StringToFaction(team); }
		else if(XMLElement* e = pNode->FirstChildElement("_NotifyRespawnTime")) {
			node.kind = LevelEventNode::NOTIFY_RESPAWN;
			node.localID = e->IntAttribute("_LocalID");
			node.delay = e->FloatAttribute("_RemainTime");
			i32 message = 0;
			if(const char* key = e->Attribute("_StringKey")) EA::StdC::Sscanf(key, "MONSTER_RESPAWN_MESSAGE_%d", &message);
			node.count = MAX(message - 1, 0);
		}
		else if(XMLElement* e = pNode->FirstChildElement("_RandomConditionInfo")) {
			node.kind = LevelEventNode::RANDOM_CONDITION;
			u8 branch = 0;
			for(XMLElement* b = e->FirstChildElement("_RandomNodeList"); b; b = b->NextSiblingElement("_RandomNodeList"), branch++) {
				for(XMLElement* ev = b->FirstChildElement("_EvtList"); ev; ev = ev->NextSiblingElement("_EvtList")) {
					node.randomEvents.push_back({ branch, (u16)b->IntAttribute("_Probability"), StatusTypeKey(ev->Attribute("_EvtID")) });
				}
			}
		}
		else if(pNode->FirstChildElement("_PortalEntityInfo")) node.kind = LevelEventNode::PORTAL;
		else if(XMLElement* e = pNode->FirstChildElement("_SetLvEvtActivationInfo")) { node.kind = LevelEventNode::SET_ACTIVATION; node.targetKey = StatusTypeKey(e->Attribute("_LvEvtID")); node.active = e->IntAttribute("_Active") == 1; }
		else if(XMLElement* e = pNode->FirstChildElement("_EventAndConditionInfo")) { node.kind = LevelEventNode::AND_CONDITION; node.resetAfter = e->IntAttribute("_ResetAfterEvent") == 1; }
		else if(XMLElement* e = pNode->FirstChildElement("_StartPhaseTriggerData")) { node.kind = LevelEventNode::START_PHASE; node.phase = e->IntAttribute("_PhaseID"); }
		else if(XMLElement* e = pNode->FirstChildElement("_EndPhaseTriggerData")) { node.kind = LevelEventNode::END_PHASE; node.phase = e->IntAttribute("_PhaseID"); }
		else if(pNode->FirstChildElement("_SaveKillZSpawnArea")) node.kind = LevelEventNode::PASS_THROUGH;
		else if(pNode->FirstChildElement("_SceneAniStartInfo") || pNode->FirstChildElement("_SceneAniControlInfo") || pNode->FirstChildElement("_EntityHideObjInfo") || pNode->FirstChildElement("_EntityHighlightInfo") || pNode->FirstChildElement("_UIEventInfo") || pNode->FirstChildElement("_EventBgmStopInfo") || pNode->FirstChildElement("_BgmPlayInfo") || pNode->FirstChildElement("_HideObjInfo") || pNode->FirstChildElement("_LightActiveInfo")) node.kind = LevelEventNode::CLIENT_EVENT;
		else if(XMLElement* e = pNode->FirstChildElement("_KillTargetEntityInfo")) { node.kind = LevelEventNode::KILL_TARGET; node.localID = e->IntAttribute("_LocalID"); node.tagID = e->IntAttribute("_TagID", -1); }
		else if(XMLElement* e = pNode->FirstChildElement("_ActiveTriggerInfo")) { node.kind = LevelEventNode::ACTIVE_AREA_TRIGGER; node.areaID = e->IntAttribute("_AreaID"); node.active = e->IntAttribute("_Active") == 1; }
		else if(XMLElement* e = pNode->FirstChildElement("_InvokeDialogTriggerInfo")) { node.kind = LevelEventNode::INVOKE_DIALOG; node.aiIndex = e->IntAttribute("_DialogID"); }
		else if(XMLElement* e = pNode->FirstChildElement("_DialogTriggerData")) { node.kind = LevelEventNode::DIALOG_TRIGGER; node.aiIndex = e->IntAttribute("_ID"); }
		else if(pNode->FirstChildElement("_EventPartyConditionInfo") || pNode->FirstChildElement("_EventBehaviorInfo") || pNode->FirstChildElement("_CallRemoteInfo")) node.kind = LevelEventNode::PASS_THROUGH;
		else if(pNode->FirstChildElement("_UpdateObjectivePointInfo") || pNode->FirstChildElement("_ShowProgressMessageInfo") || pNode->FirstChildElement("_CameraWallInfo")) node.kind = LevelEventNode::CLIENT_EVENT;
		else if(pNode->FirstChildElement("_PrintMessageInfo") || pNode->FirstChildElement("_BGMEventInfo") || pNode->FirstChildElement("_ChangeCameraInfo") || pNode->FirstChildElement("_SetNameTagVisibillityInfo") || pNode->FirstChildElement("_InputLockInfo") || pNode->FirstChildElement("_ClientDrivenActionChangeInfo")) { node.kind = LevelEventNode::CLIENT_EVENT; node.dialogLine = pNode->FirstChildElement("_PrintMessageInfo") != nullptr; node.seqRoot = true; }
		else if(pNode->FirstChildElement("_PingBroadcastInfo") || pNode->FirstChildElement("_MaterialSetChange") || pNode->FirstChildElement("_EffectLightSwitch")) node.kind = LevelEventNode::CLIENT_EVENT;
		else if(XMLElement* e = pNode->FirstChildElement("_ActiveEvents")) {
			node.kind = LevelEventNode::ACTIVE_EVENTS;
			const char* group = e->Attribute("_InvokeGroupTag");
			node.targetKey = group && *group ? StatusTypeKey(group) : 0;
			node.active = e->IntAttribute("_Active") == 1;
		}
		else if(XMLElement* e = pNode->FirstChildElement("_CreatureEnterTriggerData")) {
			node.kind = LevelEventNode::CREATURE_ENTER;
			node.areaID = e->IntAttribute("_AreaID");
			node.tagID = e->IntAttribute("_TagID", -1);
			node.enter = true;
			node.active = e->IntAttribute("_Active", 1) == 1;
			node.retrigger = e->FloatAttribute("_RetriggerDelay");
			node.maxTriggers = e->IntAttribute("_MaxTriggerCount");
		}
		else if(XMLElement* e = pNode->FirstChildElement("_SwitchConditionInfo")) {
			node.kind = LevelEventNode::SWITCH_CONDITION;
			node.apply = e->IntAttribute("_Loop") == 1;
			node.count = 0;
			for(XMLElement* b = e->FirstChildElement("_SwitchNodeList"); b; b = b->NextSiblingElement("_SwitchNodeList")) {
				const i32 index = b->IntAttribute("_SwitchIdx", -1);
				if(index < 0 || index > 0xff) continue;
				node.randomEvents.push_back({ (u8)index, 0, StatusTypeKey(b->Attribute("_EventDataID")) });
				node.count = MAX(node.count, index + 1);
			}
		}
		else if(XMLElement* e = pNode->FirstChildElement("_RandomChoiceConditionInfo")) { node.kind = LevelEventNode::RANDOM_CHOICE; node.count = MAX(e->IntAttribute("_ChoiceCount", 1), 0); }
		else if(XMLElement* e = pNode->FirstChildElement("_AiPauseInfo")) { node.kind = LevelEventNode::AI_PAUSE; node.apply = e->IntAttribute("bApply") == 1; }
		else if(XMLElement* e = pNode->FirstChildElement("_ExecuteLuaFileInfo")) { node.kind = LevelEventNode::EXECUTE_LUA; node.localID = e->IntAttribute("_TargetID"); node.areaID = e->IntAttribute("_TargetAreaID"); }
		else if(XMLElement* e = pNode->FirstChildElement("_PVPTakeScoreInfo")) { node.kind = LevelEventNode::PVP_TAKE_SCORE; node.count = e->IntAttribute("_Point"); if(const char* team = e->Attribute("_TakeSocreTeam")) node.team = StringToFaction(team); }
		else if(!HasLevelEventInfo(pNode)) node.kind = LevelEventNode::PASS_THROUGH;
		if(node.kind == LevelEventNode::OTHER) WARN("LevelEvent :: %ls node %d has an unsupported type", levelDir, node.id);
		map->levelEvents.push_back(node);
	}
	i32 restored = 0;
	foreach_const(n, map->levelEvents) {
		foreach_const(in, n->inputs) {
			LevelEventNode* target = nullptr;
			bool linked = false;
			foreach(src, map->levelEvents) {
				if(src->id != in->first) continue;
				if(eastl::find(src->outputs.begin(), src->outputs.end(), n->id) != src->outputs.end()) { linked = true; break; }
				if(!target || (!(target->difficultyMask & in->second) && (src->difficultyMask & in->second))) target = &*src;
			}
			if(linked || !target) continue;
			target->outputs.push_back(n->id);
			restored++;
		}
	}
	if(restored > 0) LOG("LevelEvent :: %d output link(s) restored from input links", restored);
	eastl::vector<i32> pending;
	foreach_const(n, map->levelEvents) if(n->kind == LevelEventNode::DIALOG_TRIGGER) pending.push_back(n->id);
	while(!pending.empty()) {
		const i32 id = pending.back();
		pending.pop_back();
		foreach(n, map->levelEvents) {
			if(n->id != id || n->clientDriven) continue;
			n->clientDriven = true;
			foreach_const(o, n->outputs) pending.push_back(*o);
		}
	}
	AddLevelAITriggerSources(map, levelDir);
	for(bool changed = true; changed;) {
		changed = false;
		foreach(n, map->levelEvents) {
			if(!n->seqRoot) continue;
			foreach_const(o, n->outputs) {
				const LevelEventNode* next = map->FindLevelEvent(*o);
				if(next && next->seqRoot) continue;
				n->seqRoot = false;
				changed = true;
				break;
			}
		}
	}
	LOG("Loaded %d level event nodes", (i32)map->levelEvents.size());
	return true;
}

void GameXmlContent::AddLevelAITriggerSources(Map* map, const wchar* levelDir)
{
	if(!levelAITriggerSourcesLoaded) {
		levelAITriggerSourcesLoaded = true;
		XMLDocument doc;
		if(LoadXMLFile(L"/LevelAITriggers.xml", doc, true) && doc.FirstChildElement()) {
			for(XMLElement* e = doc.FirstChildElement()->FirstChildElement("Source"); e; e = e->NextSiblingElement("Source")) {
				const char* level = e->Attribute("Level");
				if(!level) continue;
				LevelAITriggerSource src;
				for(const char* c = level; *c; c++) src.level.push_back((wchar)*c);
				src.node = e->IntAttribute("Node");
				src.mask = (u8)e->IntAttribute("Mask", ALL_DIFFICULTIES);
				src.aiIndex = e->IntAttribute("AITrigger");
				src.delay = e->FloatAttribute("Delay");
				levelAITriggerSources.push_back(src);
			}
		}
	}
	foreach_const(src, levelAITriggerSources) {
		if(EA::StdC::Stricmp(src->level.c_str(), levelDir) != 0) continue;
		i32 nextID = 0;
		foreach_const(n, map->levelEvents) nextID = MAX(nextID, n->id + 1);
		LevelEventNode invoke;
		invoke.id = nextID;
		invoke.kind = LevelEventNode::INVOKE_AI_TRIGGER;
		invoke.aiIndex = src->aiIndex;
		invoke.difficultyMask = src->mask;
		i32 entry = invoke.id;
		if(src->delay > 0.0f) {
			LevelEventNode delay;
			delay.id = nextID + 1;
			delay.kind = LevelEventNode::DELAY;
			delay.delay = src->delay;
			delay.difficultyMask = src->mask;
			delay.inputs.push_back(eastl::make_pair(src->node, src->mask));
			delay.outputs.push_back(invoke.id);
			invoke.inputs.push_back(eastl::make_pair(delay.id, src->mask));
			entry = delay.id;
			map->levelEvents.push_back(delay);
		}
		else invoke.inputs.push_back(eastl::make_pair(src->node, src->mask));
		i32 linked = 0;
		foreach(n, map->levelEvents) {
			if(n->id != src->node || !(n->difficultyMask & src->mask)) continue;
			n->outputs.push_back(entry);
			linked++;
		}
		map->levelEvents.push_back(invoke);
		LOG("LevelEvent :: AI trigger %d sourced from node %d (%d cop%s, mask %d, delay %.1f s)", src->aiIndex, src->node, linked, linked == 1 ? "y" : "ies", (i32)src->mask, src->delay);
	}
}

bool GameXmlContent::LoadSummonBaseStats()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/SUMMON_BASE_STATS.xml", doc)) return false;
	i32 count = 0;
	for(XMLElement* info = doc.FirstChildElement()->FirstChildElement("INFO"); info; info = info->NextSiblingElement("INFO")) {
		auto found = summonMap.find(info->IntAttribute("ID"));
		if(found == summonMap.end()) continue;
		SummonInfo& summon = found->second;
		for(XMLElement* st = info->FirstChildElement("STAT"); st && !summon.baseStats.full(); st = st->NextSiblingElement("STAT")) {
			summon.baseStats.push_back({ (u8)st->IntAttribute("type"), st->FloatAttribute("value") });
		}
		count++;
	}
	LOG("Loaded base stats of %d summons", count);
	return true;
}

bool GameXmlContent::LoadSummonActions()
{
	static const wchar* FILES[] = { L"/client/ActionBase_Monster_MOB_PvP.xml", L"/client/ActionBase_Monster_MOB_PUBLIC.xml" };
	i32 count = 0;
	for(const wchar* file : FILES) {
	XMLDocument xmlMobActions;
	if(!LoadXMLFile(file, xmlMobActions, file != FILES[0])) {
		if(file == FILES[0]) return false;
		continue;
	}
	for(XMLElement* pA = xmlMobActions.FirstChildElement()->FirstChildElement(); pA; pA = pA->NextSiblingElement()) {
		const char* cls = nullptr; const char* state = nullptr; const char* type = nullptr;
		pA->QueryStringAttribute("Class", &cls);
		pA->QueryStringAttribute("BehaviorState", &state);
		pA->QueryStringAttribute("CommandType", &type);
		if(!cls || !state || !type) continue;
		const bool spawn = EA::StdC::Strcmp(state, "SPAWN_BEHAVIORSTATE") == 0;
		const bool die = EA::StdC::Strcmp(state, "DIE_BEHAVIORSTATE") == 0;
		const bool attack = EA::StdC::Strcmp(state, "ATTACK_BEHAVIORSTATE") == 0;
		i32 extraMotion = 0;
		if(EA::StdC::Sscanf(state, "EXTRAMOTION_%d_BEHAVIORSTATE", &extraMotion) == 1) {
			if(extraMotion < 1 || extraMotion > 3 || EA::StdC::Strcmp(type, "STATUS") != 0) continue;
			SummonCommand motionCmd;
			motionCmd.kind = SummonCommand::STATUS;
			pA->QueryAttribute("StatusIndex", &motionCmd.index);
			pA->QueryFloatAttribute("Delay", &motionCmd.delay);
			const char* removeParam = pA->Attribute("Param2");
			motionCmd.remove = removeParam && EA::StdC::Strcmp(removeParam, "Remove") == 0;
			if(motionCmd.index == 0) continue;
			foreach(it, summonMap) {
				auto& list = it->second.extraMotions[extraMotion - 1];
				if(it->second.className == cls && !list.full()) { list.push_back(motionCmd); count++; }
			}
			continue;
		}
		i32 skillState = 0;
		const bool lifted = EA::StdC::Strcmp(state, "LIFTED_BEHAVIORSTATE") == 0;
		if(!spawn && !die && !attack && !lifted && EA::StdC::Sscanf(state, "SKILL_%d_BEHAVIORSTATE", &skillState) != 1) continue;
		if(skillState < 0 || skillState > 20) continue;
		i32 stateAnimId = 0;
		pA->QueryAttribute("AnimationId", &stateAnimId);
		if(spawn && stateAnimId != 0) {
			foreach(it, summonMap) if(it->second.className == cls && it->second.spawnAnim == 0) it->second.spawnAnim = stateAnimId;
			if(monsterSpawnAnim.find(eastl::string(cls)) == monsterSpawnAnim.end()) monsterSpawnAnim[eastl::string(cls)] = stateAnimId;
		}
		if(!spawn && !die && !lifted && stateAnimId != 0) {
			foreach(it, summonMap) if(it->second.className == cls && it->second.stateAnim[skillState] == 0) it->second.stateAnim[skillState] = stateAnimId;
		}
		if(spawn && EA::StdC::Strcmp(type, "INTERACTION") == 0) {
			const i32 interaction = pA->IntAttribute("Param1");
			foreach(it, summonMap) if(it->second.className == cls) it->second.spawnInteraction = interaction;
			continue;
		}
		const char* p2 = nullptr;
		pA->QueryStringAttribute("Param2", &p2);
		if(p2 && EA::StdC::Strcmp(p2, "Remove") == 0) {
			i32 removed = 0;
			if(die && EA::StdC::Strcmp(type, "REMOTE") == 0 && pA->QueryAttribute("RemoteIndex", &removed) == XML_SUCCESS && removed != 0) {
				foreach(it, summonMap) if(it->second.className == cls && !it->second.dieRemovedRemotes.full() && eastl::find(it->second.dieRemovedRemotes.begin(), it->second.dieRemovedRemotes.end(), removed) == it->second.dieRemovedRemotes.end()) it->second.dieRemovedRemotes.push_back(removed);
			}
			continue;
		}
		SummonCommand cmd;
		cmd.aniEnd = p2 && EA::StdC::Strcmp(p2, "RemoveWhenAniEnd") == 0;
		if(EA::StdC::Strcmp(type, "REMOTE") == 0) { cmd.kind = SummonCommand::REMOTE; pA->QueryAttribute("RemoteIndex", &cmd.index); cmd.nodeName = InternNodeName(pA->Attribute("NodeName")); }
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
		cmd.animIndex = (i8)pA->IntAttribute("AnimationIndex", -1);
		cmd.animId = stateAnimId;
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
			auto& list = spawn ? it->second.onSpawn : (die ? it->second.onDie : (lifted ? it->second.onLifted : it->second.skillStates[skillState]));
			if(!list.full()) { list.push_back(cmd); count++; }
		}
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
	if(!LoadXMLFile(L"/WEAPON.xml", xmlWeapon)) return false;
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
			if(fireDummy) { wf.fireDummy = fireDummy; InternNodeName(fireDummy); }
			wf.spawnAtAim = pData && XmlAttrTrue(pData, "_RemoteSpawnAimPosition");
			wf.firingMethod = id;
			if(XMLElement* pStat = pData ? pData->FirstChildElement("_FIRING_STAT") : nullptr) {
				pStat->QueryFloatAttribute("_DurationTime", &wf.firingStatDuration);
				for(XMLElement* pMul = pStat->FirstChildElement("_STAT_MUL"); pMul; pMul = pMul->NextSiblingElement("_STAT_MUL")) {
					const char* key = pMul->Attribute("DATAKEY");
					if(key && EA::StdC::Strcmp(key, "STAT_TYPE_MOVESPEED") == 0) wf.firingMoveSpeedMul += pMul->FloatAttribute("DATA");
				}
			}
			if(pFire) pFire->QueryFloatAttribute("_FireDelayTime", &wf.fireDelay);
			if(pFire) {
				pFire->QueryIntAttribute("_ContinuityCount", &wf.continuityCount);
				wf.continuityDelay = pFire->FloatAttribute("_ContinuityDelayMS") / 1000.0f;
			}
			if(const char* anim = pData ? pData->Attribute("_Animation") : nullptr) wf.animation = ActionStateFromString(anim);
			if(XMLElement* pCombo = pData ? pData->FirstChildElement("_COMBO") : nullptr) {
				for(XMLElement* pLevel = pCombo->FirstChildElement("_LEVEL"); pLevel && !wf.comboValid.full(); pLevel = pLevel->NextSiblingElement("_LEVEL")) {
					wf.comboValid.push_back(pLevel->FloatAttribute("_ValidTime"));
					wf.comboDelay.push_back(pLevel->FloatAttribute("_FireDelayTime"));
					const char* anim = pLevel->Attribute("_Animation");
					wf.comboAnimation.push_back(anim ? ActionStateFromString(anim) : wf.animation);
				}
			}
			if(XMLElement* pCharge = pData ? pData->FirstChildElement("_CHARGE") : nullptr) {
				for(XMLElement* pLevel = pCharge->FirstChildElement("_LEVEL"); pLevel; pLevel = pLevel->NextSiblingElement("_LEVEL")) {
					ASSERT(!wf.chargeTime.full());
					wf.chargeTime.push_back(pLevel->FloatAttribute("_ChargingTime"));
					const char* anim = pLevel->Attribute("_Animation");
					wf.chargeAnimation.push_back(anim ? ActionStateFromString(anim) : wf.animation);
				}
			}
			XMLElement* pFactory = pInfo->FirstChildElement("RemoteFactoryComData");
			XMLElement* pCreate = pFactory ? pFactory->FirstChildElement("_Create") : nullptr;
			XMLElement* pDef = pCreate ? pCreate->FirstChildElement("_Default") : nullptr;
			XMLElement* pRem = pDef ? pDef->FirstChildElement("_Remote") : nullptr;
			for(XMLElement* pNext = pDef ? pDef->NextSiblingElement("_Default") : nullptr; pNext && !wf.extraRemotes.full(); pNext = pNext->NextSiblingElement("_Default")) {
				if(XMLElement* pExtra = pNext->FirstChildElement("_Remote")) wf.extraRemotes.push_back((RemoteIdx)pExtra->IntAttribute("_RemoteIndex"));
			}
			if(pRem) {
				i32 v = 0;
				pRem->QueryAttribute("_RemoteIndex", &v);
				wf.base = (RemoteIdx)v;
				for(XMLElement* pE = pRem->FirstChildElement(); pE; pE = pE->NextSiblingElement()) {
					i32 idx = 0;
					if(EA::StdC::Strcmp(pE->Name(), "_COMBO") == 0) {
						pE->QueryAttribute("_Index", &idx);
						if(idx) { ASSERT(!wf.combo.full()); wf.combo.push_back((RemoteIdx)idx); }
					}
					else if(EA::StdC::Strcmp(pE->Name(), "_CHARGINGREMOTE") == 0) {
						pE->QueryAttribute("_Index", &idx);
						if(idx) { ASSERT(!wf.charge.full()); wf.charge.push_back((RemoteIdx)idx); }
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
				pRad->QueryAttribute("_MaxNum", &wf.radiateNum);
				pRad->QueryFloatAttribute("_MaxAngle", &wf.radiateAngle);
			}
			XMLElement* pFactoryCharge = pFactory && wf.charge.empty() ? pFactory->FirstChildElement("_CHARGE") : nullptr;
			for(XMLElement* pLevel = pFactoryCharge ? pFactoryCharge->FirstChildElement("_Create") : nullptr; pLevel && !wf.charge.full(); pLevel = pLevel->NextSiblingElement("_Create")) {
				XMLElement* pPar = pLevel->FirstChildElement("_Parallel");
				XMLElement* pParRem = pPar ? pPar->FirstChildElement("_Remote") : nullptr;
				if(!pParRem) continue;
				wf.charge.push_back((RemoteIdx)pParRem->IntAttribute("_RemoteIndex"));
				wf.chargeParallel.push_back(pPar->IntAttribute("_MaxNum", 1));
				pPar->QueryFloatAttribute("_Offset", &wf.parallelOffset);
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
					ASSERT(stance >= 0 && stance < 16);
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
	if(!LoadXMLFile(L"/SKILL.xml", xmlSkillP)) return false;
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
		ASSERT(!list.full());
		list.push_back(status);
		count++;
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
		if(action && EA::StdC::Strcmp(action, "IT_ACTION_TOTEM") == 0) info.action = InteractionInfo::TOTEM;
		else if(action && EA::StdC::Strcmp(action, "IT_ACTION_OCCUPY") == 0) info.action = InteractionInfo::OCCUPY;
		const char* kind = pData->Attribute("_Type");
		info.individual = kind && EA::StdC::Strcmp(kind, "IT_TYPE_INDIVIDUAL") == 0;
		info.pvpOccupy = kind && EA::StdC::Strcmp(kind, "IT_TYPE_PVP_OCCUPY") == 0;
		const char* active = pData->Attribute("_Active");
		info.active = !active || EA::StdC::Stricmp(active, "False") != 0;
		const char* repeat = pData->Attribute("_Repeat");
		info.repeat = repeat && EA::StdC::Stricmp(repeat, "True") == 0;
		pData->QueryFloatAttribute("_RepeatInterval", &info.repeatInterval);
		for(XMLElement* pR = pData->FirstChildElement("_RequireResource"); pR; pR = pR->NextSiblingElement("_RequireResource")) {
			const char* type = pR->Attribute("_Type");
			if(type && EA::StdC::Strcmp(type, "SCORE_TYPE_MINERAL") == 0) info.requireMinerals = pR->IntAttribute("_Count");
		}
	}
	XMLDocument xmlCast;
	if(LoadXMLFile(L"/InteractionCast.xml", xmlCast, true)) {
		for(XMLElement* pC = xmlCast.FirstChildElement()->FirstChildElement("INFO"); pC; pC = pC->NextSiblingElement("INFO")) {
			auto found = interactions.find(pC->IntAttribute("ID"));
			if(found == interactions.end()) continue;
			found->second.castMs = pC->IntAttribute("castMs");
			found->second.announcement = pC->UnsignedAttribute("announcement");
			found->second.points = pC->IntAttribute("points");
			found->second.ownerMask[0] = pC->UnsignedAttribute("ownerMask3");
			found->second.ownerMask[1] = pC->UnsignedAttribute("ownerMask4");
		}
	}
	XMLDocument xmlShield;
	if(LoadXMLFile(L"/StatusShield.xml", xmlShield, true)) {
		for(XMLElement* pS = xmlShield.FirstChildElement()->FirstChildElement("STATUS"); pS; pS = pS->NextSiblingElement("STATUS")) {
			statusShield[pS->IntAttribute("ID")] = pS->FloatAttribute("shield");
		}
	}
	XMLDocument xmlTeleport;
	if(LoadXMLFile(L"/StageTeleport.xml", xmlTeleport, true)) {
		for(XMLElement* pT = xmlTeleport.FirstChildElement()->FirstChildElement("TARGET"); pT; pT = pT->NextSiblingElement("TARGET")) {
			stageTeleportDistance[pT->IntAttribute("ID")] = pT->FloatAttribute("distance");
		}
	}
	return true;
}

i32 GameXmlContent::FindDynamicDrop(i32 doc, u8 difficultyBit) const
{
	auto found = dynamicDrop.find(doc);
	if(found == dynamicDrop.end()) return 0;
	for(i32 d = 0; d < (i32)found->second.size(); d++) if(difficultyBit == (u8)(1 << d)) return found->second[d];
	return found->second[NORMAL_DIFFICULTY_INDEX];
}

bool GameXmlContent::LoadInteractionReceivers()
{
	XMLDocument xmlDynamic;
	if(!LoadXMLFile(L"/client/DYNAMIC.xml", xmlDynamic)) return false;
	for(XMLElement* pInfo = xmlDynamic.FirstChildElement()->FirstChildElement(); pInfo; pInfo = pInfo->NextSiblingElement()) {
		XMLElement* pRecv = pInfo->FirstChildElement("InteractionRecvComData");
		i32 id = 0;
		i32 interaction = 0;
		XMLElement* pEntity = pInfo->FirstChildElement("EntityComData");
		if(pEntity && pInfo->QueryAttribute("ID", &id) == XML_SUCCESS) pEntity->QueryAttribute("_DensityCount", &dynamicDensity[id]);
		if(XMLElement* pDyn = pInfo->FirstChildElement("DynamicComData")) {
			const char* dynType = pDyn->Attribute("_Type");
			XMLElement* pBalance = pDyn->FirstChildElement("_RankDetailBalanceData");
			const bool jumpable = pDyn->BoolAttribute("_Jumpable");
			if(id && pDyn->BoolAttribute("_IsBlockSight")) dynamicBlockSight.insert(id);
			if(id && ((dynType && EA::StdC::Strcmp(dynType, "DYNAMIC_TYPE_DESTROY") == 0) || jumpable) && pBalance && pBalance->IntAttribute("_BaseHitCount") > 0) {
				dynamicDestroyHits[id] = pBalance->IntAttribute("_BaseHitCount");
				if(jumpable) dynamicJumpable.insert(id);
				if(XMLElement* pStats = pInfo->FirstChildElement("StatsComData")) if(pStats->FloatAttribute("_DestroyTime") > 0) dynamicRespawnSec[id] = pStats->FloatAttribute("_DestroyTime");
			}
		}
		if(XMLElement* pKfm = pInfo->FirstChildElement("Kfm_ComData")) if(id && pKfm->Attribute("KeyName")) dynamicMeshKey[id] = pKfm->Attribute("KeyName");
		if(XMLElement* pStats = pInfo->FirstChildElement("StatsComData")) {
			if(const i32 baseDrop = pStats->IntAttribute("_DropIndex")) {
				static const char* DIFFICULTY_TAGS[] = { "_EASY", nullptr, "_HARD", "_EXTREME", "_HELL" };
				eastl::array<i32,5> drops;
				for(i32 d = 0; d < (i32)drops.size(); d++) {
					XMLElement* pDiff = DIFFICULTY_TAGS[d] ? pStats->FirstChildElement(DIFFICULTY_TAGS[d]) : nullptr;
					drops[d] = pDiff ? pDiff->IntAttribute("_DropIndex", baseDrop) : baseDrop;
				}
				dynamicDrop[pInfo->IntAttribute("ID")] = drops;
			}
		}
		if(!pRecv || pInfo->QueryAttribute("ID", &id) != XML_SUCCESS || pRecv->QueryAttribute("_Index", &interaction) != XML_SUCCESS) continue;
		auto found = interactions.find(interaction);
		if(found != interactions.end() && found->second.receiver == CreatureIndex::Invalid) found->second.receiver = CreatureIndex(id);
		if(found != interactions.end()) dynamicInteraction[id] = interaction;
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
	for(XMLElement* p = pRoot ? pRoot->FirstChildElement("_ConstantData") : nullptr; p; p = p->NextSiblingElement("_ConstantData")) {
		const char* key = p->Attribute("DATAKEY");
		if(key) botConstants[StatusTypeKey(key)] = p->FloatAttribute("DATA");
	}
	for(XMLElement* p = pRoot ? pRoot->FirstChildElement("_StrategyConstantData") : nullptr; p; p = p->NextSiblingElement("_StrategyConstantData")) {
		const char* key = p->Attribute("DATAKEY");
		if(key) botConstants[StatusTypeKey(key)] = p->FloatAttribute("DATA");
	}
	if(XMLElement* p = pRoot ? pRoot->FirstChildElement("_CommonSkillIndexData") : nullptr) {
		for(i32 i = 0; i < (i32)botStageSkills.size(); i++) {
			char attr[16];
			EA::StdC::Snprintf(attr, sizeof(attr), "_StageSkill%d", i);
			botStageSkills[i] = p->IntAttribute(attr);
		}
		botReturnSkill = p->IntAttribute("_ReturnSkill");
	}
	foreach_const(m, masters) {
		const i32 creature = (i32)m->ID;
		if(botSkills.find(creature) != botSkills.end()) continue;
		eastl::array<i32,5> slots = {{ 0, 0, 0, 0, 0 }};
		foreach_const(sid, m->skillIDs) {
			const i32 id = (i32)*sid;
			const i32 order = (id / 10) % 10;
			auto f = skillMap.find(*sid);
			if(f == skillMap.end() || id % 10 != 0 || order < 1 || order > 5) continue;
			if(f->second.type == SkillType::PASSIVE || f->second.type == SkillType::SHIRK) continue;
			slots[order - 1] = id;
		}
		botSkills[creature] = slots;
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

const Remote* GameXmlContent::FindCasterMoveBoundRemote(i32 skillIndex) const
{
	foreach_const(it, remoteMap) {
		if(it->second.skillIndex == skillIndex && it->second.boundType == Remote::BoundType::E_BOUND_CASTER_MOVE_BOUND) return &it->second;
	}
	return nullptr;
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

StatusCondition StatusConditionFromString(const char* str)
{
	if(!str) return StatusCondition::NONE;
	if(EA::StdC::Strcmp(str, "FRIENDLY_HPBIGGER") == 0) return StatusCondition::FRIENDLY_HPBIGGER;
	if(EA::StdC::Strcmp(str, "FRIENDLY_HPLESS") == 0) return StatusCondition::FRIENDLY_HPLESS;
	if(EA::StdC::Strcmp(str, "UNFRIENDLY_HPBIGGER") == 0) return StatusCondition::UNFRIENDLY_HPBIGGER;
	if(EA::StdC::Strcmp(str, "UNFRIENDLY_HPLESS") == 0) return StatusCondition::UNFRIENDLY_HPLESS;
	return StatusCondition::NONE;
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
	if(!str) return BoundType::INVALID;
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
	if(!str) return DamageGroup::INVALID;
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

bool GameXmlContent::LoadMasterGroupings()
{
	static const struct { const char* name; StatusMod::Stat stat; StatType type; } STATS[] = {
		{ "STAT_TYPE_HEALTH", StatusMod::HEALTH, StatType::STAT_TYPE_HEALTH },
		{ "STAT_TYPE_ATTACK", StatusMod::ATTACK, StatType::STAT_TYPE_ATTACK },
		{ "STAT_TYPE_DEFENCE", StatusMod::DEFENCE, StatType::STAT_TYPE_DEFENCE },
		{ "STAT_TYPE_SKILLPOWER", StatusMod::SKILLPOWER, StatType::STAT_TYPE_SKILLPOWER },
		{ "STAT_TYPE_CRITICALRATE", StatusMod::CRITICALRATE, StatType::STAT_TYPE_CRITICALRATE },
		{ "STAT_TYPE_MOVESPEED", StatusMod::MOVESPEED, StatType::STAT_TYPE_MOVESPEED },
		{ "STAT_TYPE_MANAREGEN", StatusMod::MANAREGEN, StatType::STAT_TYPE_MANAREGEN },
		{ "STAT_TYPE_MANA", StatusMod::OTHER, StatType::STAT_TYPE_MANA },
	};
	static const struct { const char* name; u8 value; } KINDS[] = { { "FRIENDLY", MasterGrouping::FRIENDLY }, { "ENEMY", MasterGrouping::ENEMY }, { "PAIR", MasterGrouping::PAIR }, { "ALL", MasterGrouping::ALL } };
	static const struct { const char* name; u8 value; } APPLIES[] = { { "ALWAYS", MasterGrouping::ALWAYS }, { "HIT", MasterGrouping::HIT }, { "KILL_SUMMONEE", MasterGrouping::KILL_SUMMONEE } };
	static const struct { const char* name; u8 value; } EFFECTS[] = { { "STAT", MasterGrouping::STAT }, { "DAMAGE_BONUS", MasterGrouping::DAMAGE_BONUS }, { "DAMAGE_REDUCE", MasterGrouping::DAMAGE_REDUCE }, { "CONSUMABLE_STAT", MasterGrouping::CONSUMABLE_STAT }, { "EXP", MasterGrouping::EXP } };
	XMLDocument doc;
	if(!LoadXMLFile(L"/client/MASTER_GROUPING.xml", doc, true)) return true;
	for(XMLElement* pInfo = doc.FirstChildElement()->FirstChildElement("INFO"); pInfo; pInfo = pInfo->NextSiblingElement("INFO")) {
		XMLElement* pG = pInfo->FirstChildElement("ST_MASTER_GROUPING");
		if(!pG || masterGroupings.full()) continue;
		MasterGrouping g;
		g.id = pInfo->IntAttribute("ID");
		g.apply = MasterGrouping::OTHER;
		if(const char* k = pG->Attribute("_Type")) for(const auto& e : KINDS) if(EA::StdC::Strcmp(k, e.name) == 0) g.kind = (MasterGrouping::Kind)e.value;
		if(const char* a = pG->Attribute("_ApplyEffectType")) for(const auto& e : APPLIES) if(EA::StdC::Strcmp(a, e.name) == 0) g.apply = (MasterGrouping::Apply)e.value;
		for(XMLElement* pU = pG->FirstChildElement("_ST_UNIT_MASTER"); pU && !g.units.full(); pU = pU->NextSiblingElement("_ST_UNIT_MASTER")) {
			MasterGrouping::Unit u;
			if(const char* c = pU->Attribute("_ClassType")) u.classType = ClassTypeFromString(EA::StdC::Strncmp(c, "CLASS_TYPE_", 11) == 0 ? c + 11 : c);
			if(const char* e = pU->Attribute("_EffectType")) for(const auto& x : EFFECTS) if(EA::StdC::Strcmp(e, x.name) == 0) u.effect = (MasterGrouping::Effect)x.value;
			u.value = pU->FloatAttribute("_EffectValue");
			for(XMLElement* pS = pU->FirstChildElement(); pS && !u.mods.full(); pS = pS->NextSiblingElement()) {
				const bool add = EA::StdC::Strcmp(pS->Name(), "_STAT_ADD") == 0;
				const bool mul = EA::StdC::Strcmp(pS->Name(), "_STAT_MUL") == 0;
				const char* key = pS->Attribute("DATAKEY");
				if((!add && !mul) || !key) continue;
				for(const auto& s : STATS) {
					if(EA::StdC::Strcmp(key, s.name) != 0) continue;
					MasterGrouping::Mod m;
					m.stat = s.stat;
					m.statType = (i32)s.type;
					m.mul = mul;
					m.value = pS->FloatAttribute("DATA");
					u.mods.push_back(m);
				}
			}
			if(u.classType != ClassType::NONE) g.units.push_back(u);
		}
		if(g.units.size() >= 2) masterGroupings.push_back(g);
	}
	LOG("Loaded %d master groupings", (i32)masterGroupings.size());
	return true;
}

bool GameXmlContent::LoadTagStatuses()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/TagStatus.xml", doc, true)) return true;
	i32 moved = 0, dropped = 0;
	for(XMLElement* pInfo = doc.FirstChildElement()->FirstChildElement("INFO"); pInfo; pInfo = pInfo->NextSiblingElement("INFO")) {
		auto found = statusMap.find(pInfo->IntAttribute("ID"));
		const char* mode = pInfo->Attribute("mode");
		if(found == statusMap.end() || !mode) continue;
		if(EA::StdC::Strcmp(mode, "MOVE") == 0) { found->second.tagMode = StatusInfo::TAG_MOVE; moved++; }
		else if(EA::StdC::Strcmp(mode, "DROP") == 0) { found->second.tagMode = StatusInfo::TAG_DROP; dropped++; }
	}
	LOG("Loaded tag statuses (%d follow the active master, %d end at tag)", moved, dropped);
	return true;
}

bool GameXmlContent::LoadFixedData()
{
	XMLDocument doc;
	if(!LoadXMLFile(L"/client/FixedData.xml", doc, true)) return true;
	for(XMLElement* pBlock = doc.FirstChildElement()->FirstChildElement(); pBlock; pBlock = pBlock->NextSiblingElement()) {
		for(XMLElement* pMap = pBlock->FirstChildElement("_IndexMap"); pMap; pMap = pMap->NextSiblingElement("_IndexMap")) {
			const char* key = pMap->Attribute("DATAKEY");
			if(key && EA::StdC::Strcmp(key, "E_FIXED_REMOTE_FLY_SPAWN") == 0) respawnFlyRemote = pMap->IntAttribute("DATA");
		}
	}
	LOG("Loaded fixed data (respawn fly remote %d)", respawnFlyRemote);
	return true;
}
