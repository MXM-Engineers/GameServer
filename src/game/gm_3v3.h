#pragma once
#include <common/base.h>
#include <common/protocol.h>
#include <common/network.h>
#include <common/utils.h>
#include <EASTL/fixed_list.h>
#include <EASTL/fixed_hash_map.h>
#include "world.h"
#include "game.h"
#include <game/debug/window.h>

struct AccountData;

struct Game3v3: IGame
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
		ActorUID mainActorUID = ActorUID::INVALID;
		ActorUID subActorUID = ActorUID::INVALID;
		ActorUID cloneActorUID = ActorUID::INVALID; // TODO: remove

		Player(): clientID(-1) {}

		Player(i32 clientID_):
			clientID(clientID_) {}
	};

	eastl::array<const AccountData*,MAX_PLAYERS> playerAccountData;

	World world;
	Replication* replication;

	eastl::fixed_list<Player,MAX_PLAYERS> playerList;
	typedef decltype(playerList) TypePlayerList; // just so it plays well with my tools...
	eastl::array<TypePlayerList::iterator,MAX_PLAYERS> playerMap;

	eastl::array<eastl::fixed_vector<SpawnPoint,128,false>, (i32)TeamID::_COUNT> mapSpawnPoints;

	Time localTime;

	Dbg::GameUID dbgGameUID;

	ActorUID legoUID = ActorUID::INVALID;
	i32 legoDir = 1;
	i32 legoAngle = 0;
	u32 legoLastStep = 0;

	eastl::fixed_hash_map<i32,ActorUID,MAX_PLAYERS> cloneMap;

	void Init(Replication* replication_) override;
	void Update(f64 delta, Time localTime_) override;

	bool LoadMap();

	void OnPlayerConnect(i32 clientID, const AccountData* accountData) override;
	void OnPlayerDisconnect(i32 clientID) override;
	void OnPlayerReadyToLoad(i32 clientID) override;
	void OnPlayerGetCharacterInfo(i32 clientID, ActorUID actorUID) override;
	void OnPlayerUpdatePosition(i32 clientID, ActorUID actorUID, const vec3& pos, const vec3& dir, const RotationHumanoid& rot, f32 speed, ActionStateID state, i32 actionID) override;
	void OnPlayerUpdateRotation(i32 clientID, ActorUID actorUID, const RotationHumanoid& rot) override;
	void OnPlayerChatMessage(i32 clientID, i32 chatType, const wchar* msg, i32 msglen) override;
	void OnPlayerChatWhisper(i32 clientID, const wchar* destNick, const wchar* msg) override;
	void OnPlayerSetLeaderCharacter(i32 clientID, LocalActorID characterID, SkinIndex skinIndex) override;
	void OnPlayerSyncActionState(i32 clientID, ActorUID actorUID, ActionStateID state, i32 param1, i32 param2, f32 rotate, f32 upperRotate) override;
	void OnPlayerJukeboxQueueSong(i32 clientID, SongID songID) override;
	void OnPlayerLoadingComplete(i32 clientID) override;
	void OnPlayerGameIsReady(i32 clientID) override;
	void OnPlayerGameMapLoaded(i32 clientID) override;
	void OnPlayerTag(i32 clientID, LocalActorID toLocalActorID) override;
	void OnPlayerJump(i32 clientID, LocalActorID toLocalActorID, f32 rotate, f32 moveDirX, f32 moveDirY) override;
	void OnPlayerCastSkill(i32 clientID, const PlayerCastSkill& cast) override;

	bool ParseChatCommand(i32 clientID, const wchar* msg, const i32 len);
	void SendDbgMsg(i32 clientID, const wchar* msg);

	World::ActorNpc& SpawnNPC(CreatureIndex docID, i32 localID, const vec3& pos, const vec3& dir);
};
