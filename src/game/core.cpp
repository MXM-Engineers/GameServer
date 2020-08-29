#include "core.h"
#include <common/utils.h>
#include <tinyxml2.h>

static GameXmlContent* g_GameXmlContent = nullptr;

#ifdef CONF_DEBUG
	static Path gameDataDir = L"../gamedata";
#else
	static Path gameDataDir = L"gamedata";
#endif

bool GameXmlContent::LoadMasterDefinitions()
{
	Path creatureCharacterXml = gameDataDir;
	PathAppend(creatureCharacterXml, L"/CREATURE_CHARACTER.xml");

	i32 fileSize;
	u8* fileData = FileOpenAndReadAll(creatureCharacterXml.data(), &fileSize);
	if(!fileData) {
		LOG("ERROR(LoadMasterDefinitions): failed to open '%S'", creatureCharacterXml.data());
		return false;
	}
	defer(memFree(fileData));

	using namespace tinyxml2;
	XMLDocument doc;
	XMLError error = doc.Parse((char*)fileData, fileSize);
	if(error != XML_SUCCESS) {
		LOG("ERROR(LoadMasterDefinitions): error parsing '%S' > '%s'", creatureCharacterXml.data(), doc.ErrorStr());
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
			master.skillIDs.push_back(skillID);

			pSkillElt = pSkillElt->NextSiblingElement();
		} while(pSkillElt);

		// save master data
		master.ID = masterID;
		master.className = className;
		masterClassMap.emplace(strHash(master.className.data()), --masters.end());
		DBG_ASSERT(masterClassMap.find(strHash("CLASS_TYPE_STRIKER")) != masterClassMap.end());

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
		LOG("ERROR(LoadMasterSkinsDefinitions): failed to open '%S'", xmlPath.data());
		return false;
	}
	defer(memFree(fileData));

	using namespace tinyxml2;
	XMLDocument doc;
	XMLError error = doc.Parse((char*)fileData, fileSize);
	if(error != XML_SUCCESS) {
		LOG("ERROR(LoadMasterSkinsDefinitions): error parsing '%S' > '%s'", xmlPath.data(), doc.ErrorStr());
		return false;
	}

	XMLElement* pSkinElt = doc.FirstChildElement()->FirstChildElement()->FirstChildElement();
	do {
		i32 skinIndex;
		pSkinElt->QueryAttribute("_SkinIndex", &skinIndex);

		const char* classType;
		pSkinElt->QueryStringAttribute("_ClassType", &classType);

		auto found = masterClassMap.find(strHash(classType));
		if(found == masterClassMap.end()) {
			LOG("WARNING(LoadMasterSkinsDefinitions): class '%s' not found in masterClassMap, ignored", classType);
		}
		else {
			Master& master = *found->second;
			master.skinIDs.push_back(skinIndex);
		}

		pSkinElt = pSkinElt->NextSiblingElement();
	} while(pSkinElt);

	return true;
}

bool GameXmlContent::LoadMasterWeaponDefinitions()
{
	Path xmlPath = gameDataDir;
	PathAppend(xmlPath, L"/WEAPON.XML");

	i32 fileSize;
	u8* fileData = FileOpenAndReadAll(xmlPath.data(), &fileSize);
	if(!fileData) {
		LOG("ERROR(LoadMasterWeaponDefinitions): failed to open '%S'", xmlPath.data());
		return false;
	}
	defer(memFree(fileData));

	using namespace tinyxml2;
	XMLDocument doc;
	XMLError error = doc.Parse((char*)fileData, fileSize);
	if(error != XML_SUCCESS) {
		LOG("ERROR(LoadMasterWeaponDefinitions): error parsing '%S' > '%s'", xmlPath.data(), doc.ErrorStr());
		return false;
	}

	XMLElement* pWeapElt = doc.FirstChildElement()->FirstChildElement();
	do {
		i32 ID;
		pWeapElt->QueryAttribute("ID", &ID);

		XMLElement* pItemComData = pWeapElt->FirstChildElement("ItemComData");

		const char* classType;
		pItemComData->QueryStringAttribute("_RequireClass", &classType);

		auto found = masterClassMap.find(strHash(classType));
		if(found == masterClassMap.end()) {
			LOG("WARNING(LoadMasterWeaponDefinitions): class '%s' not found in masterClassMap, ignored", classType);
		}
		else {
			Master& master = *found->second;
			master.weaponIDs.push_back(ID);
		}

		pWeapElt = pWeapElt->NextSiblingElement();
	} while(pWeapElt);

	return true;
}

bool GameXmlContent::Load()
{
	bool r = LoadMasterDefinitions();
	if(!r) return false;

	r = LoadMasterSkinsDefinitions();
	if(!r) return false;

	r = LoadMasterWeaponDefinitions();
	if(!r) return false;

	eastl::fixed_string<char,1024> buff;
	foreach(it, masters) {
		LOG("%s: ID=%d", it->className.data(), it->ID);

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

	return true;
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
