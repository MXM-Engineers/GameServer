#include "world.h"

void World::Init(Replication* replication_)
{
	replication = replication_;
	nextPlayerActorUID = 21013;
	nextNpcActorUID = 5000;
}

void World::Update(f64 delta)
{
	const i32 actorCount = actorList.size();
	for(int i = 0; i < actorCount; i++) {
		const ActorCore& actor = actorList[i];

		Replication::Actor rfl;

		// Player ?
		// TODO: split players from npc from monsters?
		if(actor.classType != -1) {
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
		}
		else {
			rfl.UID = (u32)actor.UID;
			rfl.type = actor.type;
			rfl.modelID = (i32)actor.modelID;
			rfl.pos = actor.pos;
			rfl.dir = actor.dir;
			rfl.spawnType = 0;
			rfl.actionState = 99;
			rfl.ownerID = 0;
			rfl.faction = -1;
			rfl.classType = actor.classType;
			rfl.skinIndex = 0;
		}

		replication->FramePushActor(rfl);
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

World::ActorCore& World::SpawnActor(ActorUID actorUID)
{
	ActorCore& actor = actorList.push_back();
	actor.UID = actorUID;
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
	// TODO: UID -> Actor map
	for(auto it = actorList.begin(), itEnd = actorList.end(); it != itEnd; ++it) {
		if(it->UID == actorUID) {
			return it;
		}
	}
	return nullptr;
}
