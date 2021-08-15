#pragma once
#include <common/base.h>
#include <common/protocol.h>
#include <common/network.h>
#include <common/utils.h>
#include <EASTL/fixed_list.h>
#include "world.h"

struct AccountData;

struct GameHub
{
	enum {
		MAX_PLAYERS = Server::MAX_CLIENTS
	};

	struct SpawnPoint
	{
		vec3 pos, dir;
	};

	struct Player
	{
		const i32 clientID;
		bool isJukeboxActorReplicated = false;

		Player(): clientID(-1) {}

		Player(i32 clientID_):
			clientID(clientID_) {}
	};

	eastl::array<const AccountData*,MAX_PLAYERS> playerAccountData;

	WorldHub world;
	ReplicationHub* replication;

	eastl::array<ActorUID,MAX_PLAYERS> playerActorUID;
	eastl::fixed_list<Player,MAX_PLAYERS> playerList;
	eastl::array<decltype(playerList)::iterator,MAX_PLAYERS> playerClientIDMap;

	eastl::fixed_vector<SpawnPoint,128> mapSpawnPoints;

	Time localTime;

	void Init(ReplicationHub* replication_);
	void Update(Time localTime_);

	bool JukeboxQueueSong(i32 clientID, SongID songID);

	bool LoadMap();

	void OnPlayerConnect(i32 clientID, const AccountData* accountData);
	void OnPlayerDisconnect(i32 clientID);
	void OnPlayerGetCharacterInfo(i32 clientID, ActorUID actorUID);
	void OnPlayerUpdatePosition(i32 clientID, ActorUID characterActorUID, const vec3& pos, const vec3& dir, const vec3& eye, f32 rotate, f32 speed, ActionStateID state, i32 actionID);
	void OnPlayerChatMessage(i32 clientID, i32 chatType, const wchar* msg, i32 msglen);
	void OnPlayerChatWhisper(i32 clientID, const wchar* destNick, const wchar* msg);
	void OnPlayerSetLeaderCharacter(i32 clientID, LocalActorID characterID, SkinIndex skinIndex);
	void OnPlayerSyncActionState(i32 clientID, ActorUID actorUID, ActionStateID state, i32 param1, i32 param2, f32 rotate, f32 upperRotate);
	void OnPlayerJukeboxQueueSong(i32 clientID, SongID songID);
	void OnPlayerReadyToLoad(i32 clientID);

	bool ParseChatCommand(i32 clientID, const wchar* msg, const i32 len);
	void SendDbgMsg(i32 clientID, const wchar* msg);

	void SpawnNPC(CreatureIndex docID, i32 localID, const vec3& pos, const vec3& dir);
};
