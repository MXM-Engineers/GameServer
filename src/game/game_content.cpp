#include "game_content.h"
#include "config.h"
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
		masterClassMap.emplace(strHash(master.className.data()), &master);
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

		auto found = masterClassMap.find(strHash(classType));
		if(found == masterClassMap.end()) {
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

		auto found = masterClassMap.find(strHash(classType));
		if(found == masterClassMap.end()) {
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

		pMapElt->QueryStringAttribute("_LevelFile", &mapList.levelFile);

		const char* mapTypeXml;
		pMapElt->QueryStringAttribute("_MapType", &mapTypeXml);
		
		mapList.gameSubModeType = GAME_SUB_MODE_INVALID;
		if (strcmp("E_MAP_TYPE_CITY", mapTypeXml) == 0)
		{
			mapList.mapType = MAP_CITY;
		}
		else if (strcmp("E_MAP_TYPE_INGAME", mapTypeXml) == 0)
		{
			const char* gameSubModeTypeXml;

			mapList.mapType = MAP_INGAME;
		
			if (pMapElt->QueryStringAttribute("_GameSubModeType", &gameSubModeTypeXml) != XML_SUCCESS)
			{
				// training room doesn't have a gamesubmode
			}
			else if (strcmp("GAME_SUB_MODE_DEATH_MATCH_NORMAL", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GAME_SUB_MODE_DEATH_MATCH_NORMAL;
			}
			else if (strcmp("GAME_SUB_MODE_OCCUPY_CORE", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GAME_SUB_MODE_OCCUPY_CORE;
			}
			else if (strcmp("GAME_SUB_MODE_OCCUPY_BUSH", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GAME_SUB_MODE_OCCUPY_BUSH;
			}
			else if (strcmp("GAME_SUB_MODE_GOT_AUTHENTIC", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GAME_SUB_MODE_GOT_AUTHENTIC;
			}
			else if (strcmp("GAME_SUB_MODE_GOT_TUTORIAL_BASIC", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GAME_SUB_MODE_GOT_TUTORIAL_BASIC;
			}
			else if (strcmp("GAME_SUB_MODE_GOT_TUTORIAL_EXPERT", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GAME_SUB_MODE_GOT_TUTORIAL_EXPERT;
			}
			else if (strcmp("GAME_SUB_MODE_GOT_FIRE_POWER", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GAME_SUB_MODE_GOT_FIRE_POWER;
			}
			else if (strcmp("GAME_SUB_MODE_GOT_ULTIMATE_TITAN", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GAME_SUB_MODE_GOT_ULTIMATE_TITAN;
			}
			else if (strcmp("GAME_SUB_MODE_SPORTS_RUN", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GAME_SUB_MODE_SPORTS_RUN;
			}
			else if (strcmp("GAME_SUB_MODE_SPORTS_SURVIVAL", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GAME_SUB_MODE_SPORTS_SURVIVAL;
			}
			else if (strcmp("GAME_SUB_MODE_STAGE_TUTORIAL", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GAME_SUB_MODE_STAGE_TUTORIAL;
			}
			else if (strcmp("GAME_SUB_MODE_STAGE_NORMAL", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GAME_SUB_MODE_STAGE_NORMAL;
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

bool GameXmlContent::LoadLobbyNormal()
{
	LOG("Info(LoadLobbyNormal)");
	return LoadLobby(L"/Lobby_Normal/Spawn.xml");
}

bool GameXmlContent::LoadLobbyHalloween()
{
	LOG("Info(LoadLobbyHalloween)");
	return LoadLobby(L"/Lobby_Halloween/Spawn.xml");
}

bool GameXmlContent::LoadLobby(WideString levelPath)
{
	Path xmlPath = gameDataDir;
	PathAppend(xmlPath, levelPath.data());

	i32 fileSize;
	u8* fileData = FileOpenAndReadAll(xmlPath.data(), &fileSize);
	if(!fileData) {
		LOG("ERROR(LoadLobby): failed to open '%ls'", xmlPath.data());
		return false;
	}
	defer(memFree(fileData));

	using namespace tinyxml2;
	XMLDocument doc;
	XMLError error = doc.Parse((char*)fileData, fileSize);
	if(error != XML_SUCCESS) {
		LOG("ERROR(LoadLobby): error parsing '%ls' > '%s'", xmlPath.data(), doc.ErrorStr());
		return false;
	}

	// TODO: load spawns from "MAP_ENTITY_TYPE_DYNAMIC" as well
	XMLElement* pSpawnElt = doc.FirstChildElement()->FirstChildElement()->FirstChildElement();
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
		if(pSpawnElt->QueryAttribute("ReturnPoint", &returnPoint) == XML_SUCCESS) {
			spawn.type = Spawn::Type::SPAWN_POINT;
		}

		mapLobby.spawns.push_back(spawn);

		pSpawnElt = pSpawnElt->NextSiblingElement();
	} while(pSpawnElt);

	return true;
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
	bool r = LoadMasterDefinitions();
	if(!r) return false;

	r = LoadMasterSkinsDefinitions();
	if(!r) return false;

	r = LoadMasterWeaponDefinitions();
	if(!r) return false;

	r = LoadMapList();
	if (!r) return false;

	if (Config().lobbyMap == 160000042)
	{
		r = LoadLobbyNormal();
		if (!r) return false;
	}
	else if (Config().lobbyMap == 160000043)
	{
		r = LoadLobbyHalloween();
		if (!r) return false;
	}
	else
		return false;

	r = LoadJukeboxSongs();
	if(!r) return false;

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
	foreach(it, mapLobby.spawns) {
		LOG("Spawn :: docID=%d localID=%d pos=(%g, %g, %g) rot=(%g, %g, %g)", (i32)it->docID, it->localID, it->pos.x, it->pos.y, it->pos.z, it->rot.x, it->rot.y, it->rot.z);
	}

	LOG("Jukebox songs:");
	foreach(it, jukeboxSongs) {
		LOG("ID=%d length=%d", (i32)it->ID, it->length);
	}

	return true;
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
