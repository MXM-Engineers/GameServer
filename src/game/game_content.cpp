#include "game_content.h"
#include <common/utils.h>
#include <tinyxml2.h>
#include <EAStdC/EAString.h>

#include "config.h"
#include "core.h"
#include "physics.h"


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
		
		mapList.gameSubModeType = GAME_SUB_MODE_INVALID;
		if (EA::StdC::Strcmp("E_MAP_TYPE_CITY", mapTypeXml) == 0)
		{
			mapList.mapType = MAP_CITY;
		}
		else if (EA::StdC::Strcmp("E_MAP_TYPE_INGAME", mapTypeXml) == 0)
		{
			const char* gameSubModeTypeXml;

			mapList.mapType = MAP_INGAME;
		
			if (pMapElt->QueryStringAttribute("_GameSubModeType", &gameSubModeTypeXml) != XML_SUCCESS)
			{
				// training room doesn't have a gamesubmode
			}
			else if (EA::StdC::Strcmp("GAME_SUB_MODE_DEATH_MATCH_NORMAL", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GAME_SUB_MODE_DEATH_MATCH_NORMAL;
			}
			else if (EA::StdC::Strcmp("GAME_SUB_MODE_OCCUPY_CORE", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GAME_SUB_MODE_OCCUPY_CORE;
			}
			else if (EA::StdC::Strcmp("GAME_SUB_MODE_OCCUPY_BUSH", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GAME_SUB_MODE_OCCUPY_BUSH;
			}
			else if (EA::StdC::Strcmp("GAME_SUB_MODE_GOT_AUTHENTIC", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GAME_SUB_MODE_GOT_AUTHENTIC;
			}
			else if (EA::StdC::Strcmp("GAME_SUB_MODE_GOT_TUTORIAL_BASIC", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GAME_SUB_MODE_GOT_TUTORIAL_BASIC;
			}
			else if (EA::StdC::Strcmp("GAME_SUB_MODE_GOT_TUTORIAL_EXPERT", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GAME_SUB_MODE_GOT_TUTORIAL_EXPERT;
			}
			else if (EA::StdC::Strcmp("GAME_SUB_MODE_GOT_FIRE_POWER", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GAME_SUB_MODE_GOT_FIRE_POWER;
			}
			else if (EA::StdC::Strcmp("GAME_SUB_MODE_GOT_ULTIMATE_TITAN", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GAME_SUB_MODE_GOT_ULTIMATE_TITAN;
			}
			else if (EA::StdC::Strcmp("GAME_SUB_MODE_SPORTS_RUN", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GAME_SUB_MODE_SPORTS_RUN;
			}
			else if (EA::StdC::Strcmp("GAME_SUB_MODE_SPORTS_SURVIVAL", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GAME_SUB_MODE_SPORTS_SURVIVAL;
			}
			else if (EA::StdC::Strcmp("GAME_SUB_MODE_STAGE_TUTORIAL", gameSubModeTypeXml) == 0)
			{
				mapList.gameSubModeType = GAME_SUB_MODE_STAGE_TUTORIAL;
			}
			else if (EA::StdC::Strcmp("GAME_SUB_MODE_STAGE_NORMAL", gameSubModeTypeXml) == 0)
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

	if (map->mapType != MAP_CITY)
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

	r = LoadMapList();
	if (!r) return false;

	r = LoadLobby(Config().LobbyMap);
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

bool MakeMapCollisionMesh(const MeshFile::Mesh& mesh, ShapeMesh* out)
{
	out->triangleList.reserve(mesh.indexCount/3);

	// triangles
	for(int i = 0; i < mesh.indexCount; i += 3) {
		const MeshFile::Vertex& vert0 = mesh.vertices[mesh.indices[i]];
		const MeshFile::Vertex& vert1 = mesh.vertices[mesh.indices[i+1]];
		const MeshFile::Vertex& vert2 = mesh.vertices[mesh.indices[i+2]];
		const vec3 v0(vert0.px, vert0.py, vert0.pz);
		const vec3 v1(vert1.px, vert1.py, vert1.pz);
		const vec3 v2(vert2.px, vert2.py, vert2.pz);

		ShapeTriangle tri;
		tri.p = { v0, v2, v1 };
		out->triangleList.push_back(tri);
	}

	return true;
}
