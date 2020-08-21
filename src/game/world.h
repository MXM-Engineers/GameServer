#pragma once
#include <common/base.h>
#include <common/network.h>
#include <common/vector_math.h>
#include <EASTL/array.h>
#include <EASTL/fixed_list.h>
#include <EASTL/fixed_vector.h>
#include "replication.h"

enum class ActorUID: u32 {
	INVALID = 0
};
enum class ActorModelID: i32 {
	INVALID = 0
};

struct World
{
	enum {
		MAX_PLAYERS = Server::MAX_CLIENTS
	};

	struct ActorCore
	{
		ActorUID UID;
		i32 type;
		ActorModelID modelID;
		Vec3 pos;
		Vec3 dir;
		Vec3 eye;
		f32 rotate;
		f32 speed;
		i32 state;
		i32 actionID;
	};

	struct ActorPlayer: ActorCore
	{
		i32 classType;
		i32 clientID;
		WideString name;
		WideString guildTag;
	};

	struct ActorNpc: ActorCore
	{

	};

	struct ActorMonster: ActorCore
	{

	};

	Replication* replication;

	// TODO: enable overflow for those
	eastl::fixed_list<ActorPlayer,2048> actorPlayerList;
	eastl::fixed_list<ActorNpc,2048> actorNpcList;
	eastl::fixed_list<ActorMonster,2048> actorMonsterList;
	eastl::fixed_map<ActorUID,ActorCore*,2048> actorMap;

	u32 nextPlayerActorUID;
	u32 nextNpcActorUID;

	void Init(Replication* replication_);
	void Update(f64 delta);

	ActorUID NewPlayerActorUID();
	ActorUID NewNpcActorUID();

	ActorPlayer& SpawnPlayerActor(i32 clientID, i32 classType, const wchar* name, const wchar* guildTag);
	ActorNpc& SpawnNpcActor(ActorModelID modelID);

	void PlayerUpdatePosition(ActorUID actorUID, const Vec3& pos, const Vec3& dir, const Vec3& eye, f32 rotate, f32 speed, i32 state, i32 actionID);

	ActorCore* FindActor(ActorUID actorUID);
	ActorPlayer* FindPlayerActor(i32 clientID, ActorUID actorUID);
};
