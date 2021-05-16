#include "world.h"

void World::Init(Replication* replication_)
{
	replication = replication_;
	nextActorUID = 1;
}

void World::Update(Time localTime_)
{
	ProfileFunction();

	const f64 delta = TimeDurationSec(localTime, localTime_);
	localTime = localTime_;

	// players: assign body position and velocity
	foreach(it, actorPlayerList) {
		ActorPlayer& p = *it;
		p.body->dyn.pos = p.pos;
		p.body->dyn.vel = vec3(p.dir.x, p.dir.y, 0) * p.speed;
	}

	physics.Step();

	// players
	foreach(it, actorPlayerList) {
		ActorPlayer& p = *it;

		p.pos = p.body->dyn.pos;
		vec3 vel = p.body->dyn.vel;
		if(LengthSq(vel) > 0.0001f) {
			p.dir = glm::normalize(vel);
		}
		else {
			p.dir = vec3(0, 0, 0);
		}
		p.speed = glm::length(vel);
	}

	Replicate();
}

void World::Replicate()
{
	// players
	foreach_const(it, actorPlayerList) {
		const ActorPlayer& actor = *it;

		Replication::ActorPlayer rfl;
		rfl.clientID = actor.clientID;
		rfl.parentActorUID = actor.parentActorUID;
		rfl.actorUID = actor.UID;
		rfl.docID = actor.docID;
		rfl.pos = actor.pos;
		rfl.dir = actor.dir;
		rfl.rotation = actor.rotation;
		rfl.speed = actor.speed;
		rfl.actionState = actor.actionState;
		rfl.actionParam1 = actor.actionParam1;
		rfl.actionParam2 = actor.actionParam2;
		rfl.classType = actor.classType;
		rfl.skinIndex = actor.skinIndex;

		rfl.name = actor.name;
		rfl.guildTag = actor.guildTag;

		replication->FramePushPlayerActor(rfl);
	}

	// clear action state
	foreach(it, actorPlayerList) {
		it->actionState = ActionStateID::INVALID;
		it->actionParam1 = -1;
		it->actionParam2 = -1;
	}

	// npcs
	foreach_const(it, actorNpcList) {
		const ActorNpc& actor = *it;

		Replication::ActorNpc rfl;
		rfl.actorUID = actor.UID;
		rfl.type = actor.type;
		rfl.docID = actor.docID;
		rfl.pos = actor.pos;
		rfl.dir = actor.dir;
		rfl.localID = actor.localID;
		rfl.faction = actor.faction;

		replication->FramePushNpcActor(rfl);
	}
}

ActorUID World::NewActorUID()
{
	return (ActorUID)nextActorUID++;
}

World::ActorPlayer& World::SpawnPlayerActor(i32 clientID, ClassType classType, SkinIndex skinIndex, const wchar* name, const wchar* guildTag)
{
	ActorUID actorUID = NewActorUID();

	actorPlayerList.emplace_back(actorUID, ActorUID::INVALID);
	ActorPlayer& actor = actorPlayerList.back();
	actor.clientID = clientID;
	actor.type = 1;
	actor.docID = (CreatureIndex)(100000000 + (i32)classType);
	actor.rotation = {0};
	actor.speed = 0;
	actor.actionState = ActionStateID::INVALID;
	actor.actionParam1 = -1;
	actor.actionParam2 = -1;
	actor.classType = classType;
	actor.skinIndex = skinIndex;
	actor.name = name;
	actor.guildTag = guildTag;
	actor.body = physics.CreateBody(45, 210, vec3(0));

	actorPlayerMap.emplace(actorUID, --actorPlayerList.end());
	return actor;
}

World::ActorPlayer& World::SpawnPlayerSubActor(i32 clientID, ActorUID parentActorUID, ClassType classType, SkinIndex skinIndex)
{
	const ActorPlayer* parent = FindPlayerActor(parentActorUID);
	ASSERT(parent);

	// TODO: probably should deduplicate this code at some point
	ActorUID actorUID = NewActorUID();

	actorPlayerList.emplace_back(actorUID, parentActorUID);
	ActorPlayer& actor = actorPlayerList.back();
	actor.clientID = clientID;
	actor.type = 1;
	actor.docID = (CreatureIndex)(100000000 + (i32)classType);
	actor.rotation = {0};
	actor.speed = 0;
	actor.actionState = ActionStateID::INVALID;
	actor.actionParam1 = -1;
	actor.actionParam2 = -1;
	actor.classType = classType;
	actor.skinIndex = skinIndex;
	actor.name = parent->name;
	actor.guildTag = parent->guildTag;
	actor.body = physics.CreateBody(45, 210, vec3(0));

	actorPlayerMap.emplace(actorUID, --actorPlayerList.end());
	return actor;
}

World::ActorNpc& World::SpawnNpcActor(CreatureIndex docID, i32 localID)
{
	ActorUID actorUID = NewActorUID();

	actorNpcList.emplace_back(actorUID);
	ActorNpc& actor = actorNpcList.back();
	actor.type = 1;
	actor.docID = (CreatureIndex)docID;
	actor.rotation = {0};
	actor.speed = 0;
	actor.actionState = ActionStateID::INVALID;
	actor.actionParam1 = -1;
	actor.actionParam2 = -1;
	actor.localID = localID;
	actor.faction = 0;

	actorNpcMap.emplace(actorUID, --actorNpcList.end());
	return actor;
}

World::ActorPlayer* World::FindPlayerActor(ActorUID actorUID) const
{
	auto it = actorPlayerMap.find(actorUID);
	if(it == actorPlayerMap.end()) return nullptr;
	return &(*it->second);
}

World::ActorNpc* World::FindNpcActor(ActorUID actorUID) const
{
	auto it = actorNpcMap.find(actorUID);
	if(it == actorNpcMap.end()) return nullptr;
	return &(*it->second);
}

World::ActorNpc* World::FindNpcActorByCreatureID(CreatureIndex docID)
{
	foreach(it, actorNpcList) {
		if(it->docID == docID) {
			return &(*it);
		}
	}
	return nullptr;
}


bool World::DestroyPlayerActor(ActorUID actorUID)
{
	auto actorIt = actorPlayerMap.find(actorUID);
	if(actorIt == actorPlayerMap.end()) return false;

	actorPlayerList.erase(actorIt->second);
	actorPlayerMap.erase(actorIt);
	return true;
}
