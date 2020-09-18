#pragma once
#include <common/base.h>
#include <common/protocol.h>
#include <common/network.h>
#include <common/utils.h>
#include <EASTL/fixed_list.h>
#include "world.h"

struct AccountData;

struct Game3v3
{
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

	eastl::array<const AccountData*,MAX_PLAYERS> playerAccountData;

	World world;
	Replication* replication;

	eastl::array<ActorUID,MAX_PLAYERS> playerActorUID;
	eastl::fixed_list<Player,MAX_PLAYERS> playerList;
	eastl::array<decltype(playerList)::iterator,MAX_PLAYERS> playerClientIDMap;

	eastl::fixed_vector<SpawnPoint,128> mapSpawnPoints;

	Time localTime;

	void Init(Replication* replication_);
	void Update(f64 delta, Time localTime_);

	bool JukeboxQueueSong(i32 clientID, SongID songID);

	bool LoadMap();

	void OnPlayerConnect(i32 clientID, const AccountData* accountData);
	void OnPlayerDisconnect(i32 clientID);
	void OnPlayerReadyToLoad(i32 clientID);
	void OnPlayerGetCharacterInfo(i32 clientID, LocalActorID characterID);
	void OnPlayerUpdatePosition(i32 clientID, LocalActorID characterID, const Vec3& pos, const Vec3& dir, const Vec3& eye, f32 rotate, f32 speed, ActionStateID state, i32 actionID);
	void OnPlayerChatMessage(i32 clientID, i32 chatType, const wchar* msg, i32 msglen);
	void OnPlayerChatWhisper(i32 clientID, const wchar* destNick, const wchar* msg);
	void OnPlayerSetLeaderCharacter(i32 clientID, LocalActorID characterID, SkinIndex skinIndex);
	void OnPlayerSyncActionState(i32 clientID, LocalActorID characterID, ActionStateID state, i32 param1, i32 param2, f32 rotate, f32 upperRotate);
	void OnPlayerJukeboxQueueSong(i32 clientID, SongID songID);

	bool ParseChatCommand(i32 clientID, const wchar* msg, const i32 len);
	void SendDbgMsg(i32 clientID, const wchar* msg);

	void SpawnNPC(CreatureIndex docID, i32 localID, const Vec3& pos, const Vec3& dir);
};
