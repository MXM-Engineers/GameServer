#include "world.h"

void World::Init(Replication* replication_)
{
	replication = replication_;
	nextPlayerActorUID = 21013;
	nextNpcActorUID = 5000;
}

void World::Update(f64 delta)
{
	foreach(it, actorPlayerList) {
		const ActorPlayer& actor = *it;

		Replication::Actor rfl;
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

		Replication::ActorNameplate plate;
		plate.name = actor.name;
		plate.guildTag = actor.guildTag;

		Replication::ActorStats stats;
		// TODO: fill those

		Replication::ActorPlayerInfo playerInfo;
		// TODO: fill those

		replication->FramePushActor(rfl, &plate, &stats, &playerInfo);
	}

	foreach(it, actorNpcList) {
		const ActorNpc& actor = *it;

		Replication::Actor rfl;
		rfl.UID = (u32)actor.UID;
		rfl.type = actor.type;
		rfl.modelID = (i32)actor.modelID;
		rfl.pos = actor.pos;
		rfl.dir = actor.dir;
		rfl.spawnType = 0;
		rfl.actionState = 99;
		rfl.ownerID = 0;
		rfl.faction = -1;
		rfl.classType = -1; // -1 for NPCs
		rfl.skinIndex = 0;
		replication->FramePushActor(rfl, nullptr, nullptr, nullptr);
	}
}

ActorUID World::NewPlayerActorUID()
{
	return (ActorUID)nextPlayerActorUID++;
}

ActorUID World::NewNpcActorUID()
{
	return (ActorUID)nextNpcActorUID++;
}

World::ActorPlayer& World::SpawnPlayerActor(i32 clientID, i32 classType, const wchar* name, const wchar* guildTag)
{
	ActorUID actorUID = NewPlayerActorUID();
	ASSERT(FindActor(actorUID) == nullptr);

	ActorPlayer& actor = actorPlayerList.push_back();
	actor.UID = actorUID;
	actor.type = 1;
	actor.modelID = (ActorModelID)(100000000 + classType);
	actor.classType = classType;
	actor.rotate = 0;
	actor.speed = 0;
	actor.state = 0;
	actor.actionID = 0;
	actor.name = name;
	actor.guildTag = guildTag;

	actorMap.emplace(actorUID, &actor);
	return actor;
}

World::ActorNpc& World::SpawnNpcActor(ActorModelID modelID)
{
	ActorUID actorUID = NewNpcActorUID();
	ASSERT(FindActor(actorUID) == nullptr);

	ActorNpc& actor = actorNpcList.push_back();
	actor.UID = actorUID;
	actor.type = 1;
	actor.modelID = (ActorModelID)modelID;
	actor.eye = Vec3(0, 0, 0);
	actor.rotate = 0;
	actor.speed = 0;
	actor.state = -1;
	actor.actionID = -1;

	actorMap.emplace(actorUID, &actor);
	return actor;
}

void World::PlayerUpdatePosition(ActorUID actorUID, const Vec3& pos, const Vec3& dir, const Vec3& eye, f32 rotate, f32 speed, i32 state, i32 actionID)
{
	// TODO: frame delta position update
	/*
	// SA_GetCharacterInfo
	Sv::SN_GamePlayerSyncByInt sync;
	sync.characterID = update.characterID;
	sync.p3nPos = update.p3nPos;
	sync.p3nDir = update.p3nDir;
	sync.p3nEye = update.p3nEye;
	sync.nRotate = update.nRotate;
	sync.nSpeed = update.nSpeed;
	sync.nState = update.nState;
	sync.nActionIDX = update.nActionIDX;
	LOG("[client%03d] Server :: SN_GamePlayerSyncByInt :: ", clientID);
	SendPacket(clientID, sync);
	*/

	ActorCore* actor = FindActor(actorUID);

	// TODO: check for movement hacking
	actor->pos = pos;
	actor->dir = dir;
	actor->eye = eye;
	actor->rotate = rotate;
	actor->speed = speed;
	actor->state = state;
	actor->actionID = actionID;
}

World::ActorCore* World::FindActor(ActorUID actorUID)
{
	auto it = actorMap.find(actorUID);
	if(it != actorMap.end()) return it->second;
	return nullptr;
}

World::ActorPlayer* World::FindPlayerActor(i32 clientID, ActorUID actorUID)
{
	ActorCore* core = FindActor(actorUID);
	if(!core) return nullptr;
	// TODO: check if actor is a player
	return (ActorPlayer*)core;
}
