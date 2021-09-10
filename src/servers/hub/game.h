#pragma once
#include <common/base.h>
#include <common/protocol.h>
#include <common/network.h>
#include <common/utils.h>
#include <EASTL/fixed_list.h>

#include "world.h"
#include "matchmaker_connector.h"

struct AccountData;

struct HubGame
{
	enum {
		MAX_PLAYERS = MAX_CLIENTS
	};

	struct SpawnPoint
	{
		vec3 pos, dir;
	};

	struct Player
	{
		const ClientHandle clientHd;
		bool isJukeboxActorReplicated = false;

		Player(): clientHd(ClientHandle::INVALID) {}

		Player(ClientHandle clientHd_):
			clientHd(clientHd_) {}
	};

	struct Party
	{
		PartyUID UID;
		ClientHandle leader;
		EntrySystemID entry;
		StageType stageType;
		// TODO: do fancy party stuff later on
	};

	const ClientLocalMapping* plidMap;
	eastl::array<const AccountData*,MAX_PLAYERS> playerAccountData;

	WorldHub world;
	HubReplication replication;

	eastl::array<ActorUID,MAX_PLAYERS> playerActorUID;
	eastl::fixed_list<Player,MAX_PLAYERS> playerList;
	eastl::array<decltype(playerList)::iterator,MAX_PLAYERS> playerMap;

	eastl::fixed_vector<SpawnPoint,128> mapSpawnPoints;

	Time localTime;
	MatchmakerConnector* matchmaker;

	void Init(Server* server_, const ClientLocalMapping* plidMap_);
	void Update(Time localTime_);
	void ProcessMatchmakerUpdates();

	bool JukeboxQueueSong(i32 userID, SongID songID);

	bool LoadMap();

	void OnPlayerConnect(ClientHandle clientHd, const AccountData* accountData);
	void OnPlayerDisconnect(ClientHandle clientHd);
	void OnPlayerGetCharacterInfo(ClientHandle clientHd, ActorUID actorUID);
	void OnPlayerUpdatePosition(ClientHandle clientHd, ActorUID characterActorUID, const vec3& pos, const vec3& dir, const vec3& eye, f32 rotate, f32 speed, ActionStateID state, i32 actionID);
	void OnPlayerChatMessage(ClientHandle clientHd, i32 chatType, const wchar* msg, i32 msglen);
	void OnPlayerChatWhisper(ClientHandle clientHd, const wchar* destNick, const wchar* msg);
	void OnPlayerSetLeaderCharacter(ClientHandle clientHd, LocalActorID characterID, SkinIndex skinIndex);
	void OnPlayerSyncActionState(ClientHandle clientHd, ActorUID actorUID, ActionStateID state, i32 param1, i32 param2, f32 rotate, f32 upperRotate);
	void OnPlayerJukeboxQueueSong(ClientHandle clientHd, SongID songID);
	void OnPlayerReadyToLoad(ClientHandle clientHd);
	void OnCreateParty(ClientHandle clientHd, EntrySystemID entry, StageType stageType);
	void OnEnqueueGame(ClientHandle clientHd);

	bool ParseChatCommand(ClientHandle clientHd, const wchar* msg, const i32 len);
	void SendDbgMsg(ClientHandle clientHd, const wchar* msg);

	void SpawnNPC(CreatureIndex docID, i32 localID, const vec3& pos, const vec3& dir);
};
