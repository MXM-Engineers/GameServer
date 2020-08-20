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
		Vec3 eye;
		f32 rotate;
		f32 speed;
		i32 state;
		i32 actionID;
	};

	// TODO: there are 3 main actor types
	// - Players
	// - Monsters
	// - NPCs



	Replication* replication;
	eastl::fixed_vector<ActorCore,2048> actorList;
	u32 nextPlayerActorUID;

	void Init(Replication* replication_);
	void Update(f64 delta);

	ActorUID NewPlayerActorUID();
	ActorCore& SpawnActor(ActorUID actorUID);

	void PlayerUpdatePosition(ActorUID actorUID, const Vec3& pos, const Vec3& dir, const Vec3& eye, f32 rotate, f32 speed, i32 state, i32 actionID);

	ActorCore* FindActor(ActorUID actorUID);
};
