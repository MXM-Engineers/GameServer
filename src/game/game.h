#pragma once
#include <common/base.h>
#include <common/protocol.h>
#include <common/network.h>
#include <common/utils.h>
#include <EASTL/fixed_list.h>
#include "world.h"

struct AccountData;

struct IGame
{
	virtual void Init(Replication* replication_) = 0;
	virtual void Update(f64 delta, Time localTime_) = 0;

	virtual void OnPlayerConnect(i32 clientID, const AccountData* accountData) = 0;
	virtual void OnPlayerDisconnect(i32 clientID) = 0;
	virtual void OnPlayerGetCharacterInfo(i32 clientID, ActorUID actorUID) = 0;
	virtual void OnPlayerUpdatePosition(i32 clientID, ActorUID characterActorUID, const Vec3& pos, const Vec3& dir, const Vec3& eye, f32 rotate, f32 speed, ActionStateID state, i32 actionID) = 0;
	virtual void OnPlayerChatMessage(i32 clientID, i32 chatType, const wchar* msg, i32 msglen) = 0;
	virtual void OnPlayerChatWhisper(i32 clientID, const wchar* destNick, const wchar* msg) = 0;
	virtual void OnPlayerSetLeaderCharacter(i32 clientID, LocalActorID characterID, SkinIndex skinIndex) = 0;
	virtual void OnPlayerSyncActionState(i32 clientID, ActorUID actorUID, ActionStateID state, i32 param1, i32 param2, f32 rotate, f32 upperRotate) = 0;
	virtual void OnPlayerJukeboxQueueSong(i32 clientID, SongID songID) = 0;
	virtual void OnPlayerReadyToLoad(i32 clientID) = 0;
	virtual void OnPlayerLoadingComplete(i32 clientID) {}
	virtual void OnPlayerGameIsReady(i32 clientID) {}
	virtual void OnPlayerGameMapLoaded(i32 clientID) {}
	virtual void OnPlayerTag(i32 clientID, LocalActorID toLocalActorID) {}
	virtual void OnPlayerJump(i32 clientID, LocalActorID toLocalActorID, f32 rotate, f32 moveDirX, f32 moveDirY) {}
	virtual void OnPlayerCastSkill(i32 clientID, const PlayerCastSkill& cast) {}
};

struct Game: IGame
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

	void Init(Replication* replication_) override;
	void Update(f64 delta, Time localTime_) override;

	bool JukeboxQueueSong(i32 clientID, SongID songID);

	bool LoadMap();

	void OnPlayerConnect(i32 clientID, const AccountData* accountData) override;
	void OnPlayerDisconnect(i32 clientID) override;
	void OnPlayerGetCharacterInfo(i32 clientID, ActorUID actorUID) override;
	void OnPlayerUpdatePosition(i32 clientID, ActorUID characterActorUID, const Vec3& pos, const Vec3& dir, const Vec3& eye, f32 rotate, f32 speed, ActionStateID state, i32 actionID) override;
	void OnPlayerChatMessage(i32 clientID, i32 chatType, const wchar* msg, i32 msglen) override;
	void OnPlayerChatWhisper(i32 clientID, const wchar* destNick, const wchar* msg) override;
	void OnPlayerSetLeaderCharacter(i32 clientID, LocalActorID characterID, SkinIndex skinIndex) override;
	void OnPlayerSyncActionState(i32 clientID, ActorUID actorUID, ActionStateID state, i32 param1, i32 param2, f32 rotate, f32 upperRotate) override;
	void OnPlayerJukeboxQueueSong(i32 clientID, SongID songID) override;
	void OnPlayerReadyToLoad(i32 clientID) override;

	bool ParseChatCommand(i32 clientID, const wchar* msg, const i32 len);
	void SendDbgMsg(i32 clientID, const wchar* msg);

	void SpawnNPC(CreatureIndex docID, i32 localID, const Vec3& pos, const Vec3& dir);
};
