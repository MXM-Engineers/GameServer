#pragma once
#include <common/base.h>
#include <common/network.h>
#include <common/vector_math.h>
#include <EASTL/array.h>
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
		i32 classType;
		Vec3 pos;
		Vec3 dir;
	};

	struct Player
	{
		i32 clientID;
	};

	Replication* replication;
	eastl::array<ActorUID,MAX_PLAYERS> playerActorUID;
	eastl::array<Player,MAX_PLAYERS> player;
	eastl::fixed_vector<ActorCore,2048> actorList;
	u32 nextPlayerActorUID;

	void Init(Replication* replication_);
	void Update(f64 delta);

	ActorUID RegisterNewPlayer(i32 clientID);
	void PlayerFirstSpawn(i32 clientID);
	ActorCore& SpawnActor(ActorUID actorUID);
};
