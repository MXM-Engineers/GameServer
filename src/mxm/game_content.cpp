#include <common/utils.h>
#include <EAStdC/EAString.h>
#include <tinyxml2.h>

#include "core.h"
#include "game_content.h"

using namespace tinyxml2;

static GameXmlContent* g_GameXmlContent = nullptr;

static Path gameDataDir = L"gamedata";

bool GameXmlContent::LoadMasterDefinitions()
{
	Path creatureCharacterXml = gameDataDir;
	PathAppend(creatureCharacterXml, L"/CREATURE_CHARACTER.xml");

	i32 fileSize;
	u8* fileData = FileOpenAndReadAll(creatureCharacterXml.data(), &fileSize);
	if(!fileData) {
		LOG("ERROR(LoadMasterDefinitions): failed to open '%ls'", creatureCharacterXml.data());
		return false;
	}
	defer(memFree(fileData));

	using namespace tinyxml2;
	XMLDocument doc;
	XMLError error = doc.Parse((char*)fileData, fileSize);
	if(error != XML_SUCCESS) {
		LOG("ERROR(LoadMasterDefinitions): error parsing '%ls' > '%s'", creatureCharacterXml.data(), doc.ErrorStr());
		return false;
	}

	// get master IDs
	XMLElement* pNodeMaster = doc.FirstChildElement()->FirstChildElement();
	do {
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

		pNodeMaster = pNodeMaster->NextSiblingElement();
	} while(pNodeMaster);

	return true;
}

bool GameXmlContent::LoadMasterSkinsDefinitions()
{
	Path xmlPath = gameDataDir;
	PathAppend(xmlPath, L"/CHARACTERSKIN.XML");

	i32 fileSize;
	u8* fileData = FileOpenAndReadAll(xmlPath.data(), &fileSize);
	if(!fileData) {
		LOG("ERROR(LoadMasterSkinsDefinitions): failed to open '%ls'", xmlPath.data());
		return false;
	}
	defer(memFree(fileData));

	using namespace tinyxml2;
	XMLDocument doc;
	XMLError error = doc.Parse((char*)fileData, fileSize);
	if(error != XML_SUCCESS) {
		LOG("ERROR(LoadMasterSkinsDefinitions): error parsing '%ls' > '%s'", xmlPath.data(), doc.ErrorStr());
		return false;
	}

	XMLElement* pSkinElt = doc.FirstChildElement()->FirstChildElement()->FirstChildElement();
	do {
		i32 skinIndex;
		pSkinElt->QueryAttribute("_SkinIndex", &skinIndex);

		const char* classType;
		pSkinElt->QueryStringAttribute("_ClassType", &classType);

		auto found = masterClassStringMap.find(strHash(classType));
		if(found == masterClassStringMap.end()) {
			LOG("WARNING(LoadMasterSkinsDefinitions): class '%s' not found in masterClassMap, ignored", classType);
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
	Path xmlPath = gameDataDir;
	PathAppend(xmlPath, L"/WEAPON.xml");

	i32 fileSize;
	u8* fileData = FileOpenAndReadAll(xmlPath.data(), &fileSize);
	if(!fileData) {
		LOG("ERROR(LoadMasterWeaponDefinitions): failed to open '%ls'", xmlPath.data());
		return false;
	}
	defer(memFree(fileData));

	using namespace tinyxml2;
	XMLDocument doc;
	XMLError error = doc.Parse((char*)fileData, fileSize);
	if(error != XML_SUCCESS) {
		LOG("ERROR(LoadMasterWeaponDefinitions): error parsing '%ls' > '%s'", xmlPath.data(), doc.ErrorStr());
		return false;
	}

	XMLElement* pWeapElt = doc.FirstChildElement()->FirstChildElement();
	do {
		i32 ID;
		pWeapElt->QueryAttribute("ID", &ID);

		XMLElement* pItemComData = pWeapElt->FirstChildElement("ItemComData");

		const char* classType;
		pItemComData->QueryStringAttribute("_RequireClass", &classType);

		auto found = masterClassStringMap.find(strHash(classType));
		if(found == masterClassStringMap.end()) {
			LOG("WARNING(LoadMasterWeaponDefinitions): class '%s' not found in masterClassMap, ignored", classType);
		}
		else {
			Master& master = *found->second;
			master.weaponIDs.push_back((WeaponIndex)ID);
		}

		pWeapElt = pWeapElt->NextSiblingElement();
	} while(pWeapElt);

	return true;
}

bool GameXmlContent::LoadMasterDefinitionsModel()
{
	return true; // TODO: remove

	// Parse SKILLS.xml once
	{
		Path SkillXml = gameDataDir;
		PathAppend(SkillXml, L"/SKILL.xml");

		i32 fileSize;
		u8* fileData = FileOpenAndReadAll(SkillXml.data(), &fileSize);
		if (!fileData) {
			LOG("ERROR(LoadMasterDefinitions): failed to open '%ls'", SkillXml.data());
			return false;
		}
		defer(memFree(fileData));

		using namespace tinyxml2;
		XMLError error = xmlSKILL.Parse((char*)fileData, fileSize);
		if (error != XML_SUCCESS) {
			LOG("ERROR(LoadMasterDefinitions): error parsing '%ls' > '%s'", SkillXml.data(), xmlSKILL.ErrorStr());
			return false;
		}
	}

	// Parse SKILL_PROPERTY.xml once
	{
		Path SkillPropertyXml = gameDataDir;
		PathAppend(SkillPropertyXml, L"/SKILL_PROPERTY.xml");

		i32 fileSize;
		u8* fileData = FileOpenAndReadAll(SkillPropertyXml.data(), &fileSize);
		if (!fileData) {
			LOG("ERROR(LoadMasterDefinitions): failed to open '%ls'", SkillPropertyXml.data());
			return false;
		}
		defer(memFree(fileData));

		using namespace tinyxml2;
		XMLError error = xmlSKILLPROPERTY.Parse((char*)fileData, fileSize);
		if (error != XML_SUCCESS) {
			LOG("ERROR(LoadMasterDefinitions): error parsing '%ls' > '%s'", SkillPropertyXml.data(), xmlSKILLPROPERTY.ErrorStr());
			return false;
		}
	}

	Path creatureCharacterXml = gameDataDir;
	PathAppend(creatureCharacterXml, L"/CREATURE_CHARACTER.xml");

	i32 fileSize;
	u8* fileData = FileOpenAndReadAll(creatureCharacterXml.data(), &fileSize);
	if (!fileData) {
		LOG("ERROR(LoadMasterDefinitions): failed to open '%ls'", creatureCharacterXml.data());
		return false;
	}
	defer(memFree(fileData));

	using namespace tinyxml2;
	XMLDocument doc;
	XMLError error = doc.Parse((char*)fileData, fileSize);
	if (error != XML_SUCCESS) {
		LOG("ERROR(LoadMasterDefinitions): error parsing '%ls' > '%s'", creatureCharacterXml.data(), doc.ErrorStr());
		return false;
	}


	// get master IDs
	XMLElement* pNodeMaster = doc.FirstChildElement()->FirstChildElement();
	do {
		mastersModel.push_back();
		CharacterModel &character = mastersModel.back();

		LOG("DEBUG: CharacterModel addresss %llx", (intptr_t)&character);

		i32 masterID;
		pNodeMaster->QueryAttribute("ID", &masterID);

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
			LOG("SkillID: %d", skillID);

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
		
		pNodeMaster = pNodeMaster->NextSiblingElement();

		//LOG("DEBUG MASTER: Creature Type %d, ID %d, movement speed %f, normal movement speed %f, skill 1 ID %d", character.getCreatureType(), 
		//	character.getID(), character.getMoveSpeed(), character.getNormalMoveSpeed(), character.getSkills()->getSkillByIndex(0)->getID());
		character.Print();
	} while (pNodeMaster);

	return true;
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
			LOG("Skill Match");
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
					
			//Need -1 to correct index
			for (int i = level-1; i < 6; i++)
			{
				SkillNormalLevelModel& _skillNormalLevelModel = *SkillNormal.getSkillNormalLevelByIndex(i);
				_skillNormalLevelModel.setLevel(level);
				SetValuesSkillNormalLevel(*pNodePropertyLevel, _skillNormalLevelModel);
			}
		}
	
		pNodeInfo = pNodeInfo->NextSiblingElement();
	} while (pNodeInfo);

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

bool GameXmlContent::LoadMapList()
{
	Path xmlPath = gameDataDir;
	PathAppend(xmlPath, L"/MAPLIST.xml");

	i32 fileSize;
	u8* fileData = FileOpenAndReadAll(xmlPath.data(), &fileSize);
	if (!fileData) {
		LOG("ERROR(LoadMapList): failed to open '%ls'", xmlPath.data());
		return false;
	}
	defer(memFree(fileData));

	using namespace tinyxml2;
	XMLDocument doc;
	XMLError error = doc.Parse((char*)fileData, fileSize);
	if (error != XML_SUCCESS) {
		LOG("ERROR(LoadMapList): error parsing '%ls' > '%s'", xmlPath.data(), doc.ErrorStr());
		return false;
	}

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

	Path xmlPath = gameDataDir;

	wchar_t tempPathData[256] = {0}; //ToDO: replace with define
	wchar_t* tempPath = tempPathData;
	const char* levelFileString = mapList->levelFile.data();

	eastl::DecodePart(levelFileString, mapList->levelFile.data() + EA::StdC::Strlen(mapList->levelFile.data()), tempPath, tempPathData + sizeof(tempPathData));
	PathAppend(xmlPath, L"/");
	PathAppend(xmlPath, tempPathData);
	PathAppend(xmlPath, L"/Spawn.xml");

	i32 fileSize;
	u8* fileData = FileOpenAndReadAll(xmlPath.data(), &fileSize);
	if (!fileData) {
		LOG("ERROR(LoadMapByID): failed to open '%ls'", xmlPath.data());
		return false;
	}
	defer(memFree(fileData));

	using namespace tinyxml2;
	XMLDocument doc;
	XMLError error = doc.Parse((char*)fileData, fileSize);
	if (error != XML_SUCCESS) {
		LOG("ERROR(LoadMapByID): error parsing '%ls' > '%s'", xmlPath.data(), doc.ErrorStr());
		return false;
	}

	XMLElement* pMapInfo = doc.FirstChildElement();
	XMLElement* pMapEntityCreature = pMapInfo->FirstChildElement();
	XMLElement* pSpawnElt = pMapEntityCreature->FirstChildElement();
	do {
		Spawn spawn;
		pSpawnElt->QueryAttribute("dwDoc", (i32*)&spawn.docID);
		pSpawnElt->QueryAttribute("dwID", (i32*)&spawn.localID);
		pSpawnElt->QueryAttribute("kTranslate_x", &spawn.pos.x);
		pSpawnElt->QueryAttribute("kTranslate_y", &spawn.pos.y);
		pSpawnElt->QueryAttribute("kTranslate_z", &spawn.pos.z);
		pSpawnElt->QueryAttribute("kRotation_x", &spawn.rot.x);
		pSpawnElt->QueryAttribute("kRotation_y", &spawn.rot.y);
		pSpawnElt->QueryAttribute("kRotation_z", &spawn.rot.z);

		spawn.type = Spawn::Type::NPC_SPAWN;
		bool returnPoint;
		if (pSpawnElt->QueryAttribute("ReturnPoint", &returnPoint) == XML_SUCCESS) {
			spawn.type = Spawn::Type::SPAWN_POINT;
		}

		spawn.team = TeamID::INVALID;
		const char* teamString;
		if(pSpawnElt->QueryStringAttribute("team", &teamString) == XML_SUCCESS) {
			if(EA::StdC::Strncmp(teamString, "TEAM_RED", 8) == 0) spawn.team = TeamID::RED;
			else if(EA::StdC::Strncmp(teamString, "TEAM_BLUE", 9) == 0) spawn.team = TeamID::BLUE;
		}

		map->creatures.push_back(spawn);

		pSpawnElt = pSpawnElt->NextSiblingElement();
	} while (pSpawnElt);

	XMLElement* pMapEntityDynamic = pMapEntityCreature->NextSiblingElement();
	pSpawnElt = pMapEntityDynamic->FirstChildElement();
	do {
		Spawn spawn;
		pSpawnElt->QueryAttribute("dwDoc", (i32*)&spawn.docID);
		pSpawnElt->QueryAttribute("dwID", (i32*)&spawn.localID);
		pSpawnElt->QueryAttribute("kTranslate_x", &spawn.pos.x);
		pSpawnElt->QueryAttribute("kTranslate_y", &spawn.pos.y);
		pSpawnElt->QueryAttribute("kTranslate_z", &spawn.pos.z);
		pSpawnElt->QueryAttribute("kRotation_x", &spawn.rot.x);
		pSpawnElt->QueryAttribute("kRotation_y", &spawn.rot.y);
		pSpawnElt->QueryAttribute("kRotation_z", &spawn.rot.z);

		spawn.type = Spawn::Type::NPC_SPAWN;

		map->dynamic.push_back(spawn);

		pSpawnElt = pSpawnElt->NextSiblingElement();
	} while(pSpawnElt);

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
	Path xmlPath = gameDataDir;
	PathAppend(xmlPath, L"/JUKEBOX.xml");

	i32 fileSize;
	u8* fileData = FileOpenAndReadAll(xmlPath.data(), &fileSize);
	if(!fileData) {
		LOG("ERROR(LoadJukeboxSongs): failed to open '%ls'", xmlPath.data());
		return false;
	}
	defer(memFree(fileData));

	using namespace tinyxml2;
	XMLDocument doc;
	XMLError error = doc.Parse((char*)fileData, fileSize);
	if(error != XML_SUCCESS) {
		LOG("ERROR(LoadJukeboxSongs): error parsing '%ls' > '%s'", xmlPath.data(), doc.ErrorStr());
		return false;
	}

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
		return EntityType::ENTITY_CREATURE;
	}
	else if (EA::StdC::Strcmp("ENTITY_TYPE_DYNAMIC", s) == 0)
	{
		return EntityType::ENTITY_DYNAMIC;
	}
	else if (EA::StdC::Strcmp("ENTITY_TYPE_ITEM", s) == 0)
	{
		return EntityType::ENTITY_ITEM;
	}
	else if (EA::StdC::Strcmp("ENTITY_TYPE_SFX", s) == 0)
	{
		return EntityType::ENTITY_SFX;
	}
	else if (EA::StdC::Strcmp("ENTITY_TYPE_TERRAIN", s) == 0)
	{
		return EntityType::ENTITY_TERRAIN;
	}
	else
	{
		LOG("Unknown EntityType: %s", s);
		return EntityType::ENTITY_INVALID;
	}
}

SkillType GameXmlContent::StringToSkillType(const char* s)
{
	if (EA::StdC::Strcmp("SKILL_TYPE_COMBO", s) == 0)
	{
		return SkillType::SKILL_COMBO;
	}
	else if (EA::StdC::Strcmp("SKILL_TYPE_NORMAL", s) == 0)
	{
		return SkillType::SKILL_NORMAL;
	}
	else if (EA::StdC::Strcmp("SKILL_TYPE_PASSIVE", s) == 0)
	{
		return SkillType::SKILL_PASSIVE;
	}
	else if (EA::StdC::Strcmp("SKILL_TYPE_SHIRK", s) == 0)
	{
		return SkillType::SKILL_SHIRK;
	}
	else if (EA::StdC::Strcmp("SKILL_TYPE_STANCE", s) == 0)
	{
		return SkillType::SKILL_STANCE;
	}
	else if (EA::StdC::Strcmp("SKILL_TYPE_SUMMON", s) == 0)
	{
		return SkillType::SKILL_SUMMON;
	}
	else if (EA::StdC::Strcmp("SKILL_TYPE_TOGGLE", s) == 0)
	{
		return SkillType::SKILL_TOGGLE;
	}
	else
	{
		LOG("Unknown SkillType: %s", s);
		return SkillType::SKILL_INVALID;
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

bool GameXmlContentLoad()
{
	static GameXmlContent content;
	g_GameXmlContent = &content;
	return content.Load();
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
