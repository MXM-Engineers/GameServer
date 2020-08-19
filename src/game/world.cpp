#include "world.h"

void World::Init(Reflection* reflection_)
{
	reflection = reflection_;
	memset(&playerActorUID, 0, sizeof(playerActorUID));
	nextPlayerActorUID = 21013;
}

void World::Update(f64 delta)
{
	const i32 actorCount = actorList.size();
	for(int i = 0; i < actorCount; i++) {
		const ActorCore& actor = actorList[i];

		Reflection::Actor rfl;
		rfl.UID = (u32)actor.UID;
		rfl.type = actor.type;
		rfl.modelID = (i32)actor.modelID;
		rfl.pos = actor.pos;
		rfl.dir = actor.dir;
		rfl.spawnType = 0;
		rfl.actionState = -1;
		rfl.ownerID = 0;
		rfl.faction = 0;
		rfl.classType = actor.classType;
		rfl.skinIndex = 0;
		reflection->FramePushActor(rfl);
	}
}

ActorUID World::RegisterNewPlayer(i32 clientID)
{
	ASSERT(clientID >= 0 && clientID < MAX_PLAYERS);
	ASSERT(playerActorUID[clientID] == ActorUID::INVALID);

	playerActorUID[clientID] = (ActorUID)nextPlayerActorUID++;

	reflection->EventPlayerConnect(clientID, (u32)playerActorUID[clientID]);
	return playerActorUID[clientID];
}

void World::PlayerFirstSpawn(i32 clientID)
{
	ActorCore& actor = SpawnActor(playerActorUID[clientID]);
	actor.type = 1;
	actor.modelID = (ActorModelID)100000017;
	actor.classType = 17;
	actor.pos = Vec3(11959.4f, 6451.76f, 3012);
	actor.dir = Vec3(0, 0, 2.68874f);

	reflection->EventPlayerGameEnter(clientID);
}

World::ActorCore& World::SpawnActor(ActorUID actorUID)
{
	ActorCore& actor = actorList.push_back();
	actor.UID = actorUID;
	return actor;
}
