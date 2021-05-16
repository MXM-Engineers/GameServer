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

struct Game
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
	vec2 legoDir = vec2(1, 0);
	i32 legoAngle = 0;
	u32 legoLastStep = 0;

	eastl::fixed_hash_map<i32,ActorUID,MAX_PLAYERS> cloneMap;

	void Init(Replication* replication_);
	void Update(Time localTime_);

	bool LoadMap();

	void OnPlayerConnect(i32 clientID, const AccountData* accountData);
	void OnPlayerDisconnect(i32 clientID);
	void OnPlayerReadyToLoad(i32 clientID);
	void OnPlayerGetCharacterInfo(i32 clientID, ActorUID actorUID);
	void OnPlayerUpdatePosition(i32 clientID, ActorUID actorUID, const vec3& pos, const vec3& dir, const RotationHumanoid& rot, f32 speed, ActionStateID state, i32 actionID);
	void OnPlayerUpdateRotation(i32 clientID, ActorUID actorUID, const RotationHumanoid& rot);
	void OnPlayerChatMessage(i32 clientID, i32 chatType, const wchar* msg, i32 msglen);
	void OnPlayerChatWhisper(i32 clientID, const wchar* destNick, const wchar* msg);
	void OnPlayerSetLeaderCharacter(i32 clientID, LocalActorID characterID, SkinIndex skinIndex);
	void OnPlayerSyncActionState(i32 clientID, ActorUID actorUID, ActionStateID state, i32 param1, i32 param2, f32 rotate, f32 upperRotate);
	void OnPlayerJukeboxQueueSong(i32 clientID, SongID songID);
	void OnPlayerLoadingComplete(i32 clientID);
	void OnPlayerGameIsReady(i32 clientID);
	void OnPlayerGameMapLoaded(i32 clientID);
	void OnPlayerTag(i32 clientID, LocalActorID toLocalActorID);
	void OnPlayerJump(i32 clientID, LocalActorID toLocalActorID, f32 rotate, f32 moveDirX, f32 moveDirY);
	void OnPlayerCastSkill(i32 clientID, const PlayerCastSkill& cast);

	bool ParseChatCommand(i32 clientID, const wchar* msg, const i32 len);
	void SendDbgMsg(i32 clientID, const wchar* msg);

	World::ActorNpc& SpawnNPC(CreatureIndex docID, i32 localID, const vec3& pos, const vec3& dir);
};
