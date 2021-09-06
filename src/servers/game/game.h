#pragma once
#include <common/base.h>
#include <common/protocol.h>
#include <common/network.h>
#include <common/utils.h>
#include <EASTL/fixed_list.h>
#include <EASTL/fixed_hash_map.h>
#include "world.h"
#include "game.h"
#include "debug/window.h"

struct AccountData;

struct Game
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
		ActorUID actorUID = ActorUID::INVALID;
		ActorUID cloneActorUID = ActorUID::INVALID; // TODO: remove

		Player(): clientHd(ClientHandle::INVALID) {}

		Player(ClientHandle clientHd_):
			clientHd(clientHd_) {}
	};

	eastl::array<const AccountData*,MAX_PLAYERS> playerAccountData;

	World world;
	Replication replication;

	ClientLocalMapping plidMap;

	eastl::fixed_list<Player,MAX_PLAYERS> playerList;
	typedef decltype(playerList) TypePlayerList; // just so it plays well with my tools...
	eastl::array<TypePlayerList::iterator,MAX_PLAYERS> playerMap;

	eastl::array<eastl::fixed_vector<SpawnPoint,128,false>, (i32)TeamID::_COUNT> mapSpawnPoints;

	Time localTime;

	Dbg::GameUID dbgGameUID;

	World::Player* lego = nullptr;
	vec2 legoDir = vec2(1, 0);
	i32 legoAngle = 0;
	u32 legoLastStep = 0;

	World::Player* clone = nullptr;


	void Init(Server* server_);
	void Update(Time localTime_);

	bool LoadMap();

	void OnPlayerConnect(ClientHandle clientHd, const AccountData* accountData);
	void OnPlayerDisconnect(ClientHandle clientHd);
	void OnPlayerReadyToLoad(ClientHandle clientHd);
	void OnPlayerGetCharacterInfo(ClientHandle clientHd, ActorUID actorUID);
	void OnPlayerUpdatePosition(ClientHandle clientHd, ActorUID actorUID, const vec3& pos, const vec2& dir, const RotationHumanoid& rot, f32 speed, ActionStateID state, i32 actionID, f32 clientTime);
	void OnPlayerUpdateRotation(ClientHandle clientHd, ActorUID actorUID, const RotationHumanoid& rot);
	void OnPlayerChatMessage(ClientHandle clientHd, i32 chatType, const wchar* msg, i32 msglen);
	void OnPlayerChatWhisper(ClientHandle clientHd, const wchar* destNick, const wchar* msg);
	void OnPlayerSetLeaderCharacter(ClientHandle clientHd, LocalActorID characterID, SkinIndex skinIndex);
	void OnPlayerSyncActionState(ClientHandle clientHd, ActorUID actorUID, ActionStateID state, i32 param1, i32 param2, f32 rotate, f32 upperRotate);
	void OnPlayerLoadingComplete(ClientHandle clientHd);
	void OnPlayerGameIsReady(ClientHandle clientHd);
	void OnPlayerGameMapLoaded(ClientHandle clientHd);
	void OnPlayerTag(ClientHandle clientHd, LocalActorID toLocalActorID);
	void OnPlayerJump(ClientHandle clientHd, LocalActorID toLocalActorID, f32 rotate, f32 moveDirX, f32 moveDirY);
	void OnPlayerCastSkill(ClientHandle clientHd, const PlayerCastSkill& cast);

	bool ParseChatCommand(ClientHandle clientHd, const wchar* msg, const i32 len);
	void SendDbgMsg(ClientHandle clientHd, const wchar* msg);

	World::ActorNpc& SpawnNPC(CreatureIndex docID, i32 localID, const vec3& pos, const vec3& dir);
};
