#pragma once
#include <common/base.h>
#include <common/protocol.h>
#include <common/network.h>
#include <common/utils.h>
#include "world.h"

struct AccountData;

struct Game
{
	typedef World::List<World::ActorNpc>::iterator NpcHandle;

	enum {
		MAX_PLAYERS = Server::MAX_CLIENTS
	};

	struct SpawnPoint
	{
		Vec3 pos, dir;
	};

	struct Player
	{
		const i32 clientID;
		bool isJukeboxActorReplicated = false;

		Player(): clientID(-1) {}

		Player(i32 clientID_):
			clientID(clientID_) {}
	};

	const AccountData* playerAccountData[MAX_PLAYERS];

	World world;
	Replication* replication;

	eastl::array<ActorUID,MAX_PLAYERS> playerActorUID;
	eastl::fixed_list<Player,MAX_PLAYERS> playerList;
	eastl::array<decltype(playerList)::iterator,MAX_PLAYERS> playerClientIDMap;

	eastl::fixed_vector<SpawnPoint,128> mapSpawnPoints;

	NpcHandle npcJukeBox;

	void Init(Replication* replication_);
	void Update(f64 delta);
	bool LoadMap();

	void OnPlayerConnect(i32 clientID, const AccountData* accountData);
	void OnPlayerDisconnect(i32 clientID);
	void OnPlayerGetCharacterInfo(i32 clientID, LocalActorID characterID);
	void OnPlayerUpdatePosition(i32 clientID, LocalActorID characterID, const Vec3& pos, const Vec3& dir, const Vec3& eye, f32 rotate, f32 speed, i32 state, i32 actionID);
	void OnPlayerChatMessage(i32 clientID, i32 chatType, const wchar* msg, i32 msglen);
	void OnPlayerSetLeaderCharacter(i32 clientID, LocalActorID characterID, SkinIndex skinIndex);
	void OnPlayerSyncActionState(i32 clientID, const Cl::CN_GamePlayerSyncActionStateOnly& sync);

	bool ParseChatCommand(i32 clientID, const wchar* msg, const i32 len);
	void SendDbgMsg(i32 clientID, const wchar* msg);

	void SpawnNPC(CreatureIndex docID, i32 localID, const Vec3& pos, const Vec3& dir);
};
