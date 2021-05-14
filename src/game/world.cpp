#include "world.h"

void World::Init(Replication* replication_)
{
	replication = replication_;
	nextActorUID = 1;
}

void World::Update(f64 delta, Time localTime_)
{
	ProfileFunction();

	delta = TimeDurationSec(localTime, localTime_);
	localTime = localTime_;

	/*
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
		p.dir = glm::normalize(vel);
		p.speed = glm::length(vel);
	}
	*/

	// update jukebox
	if(jukebox.UID != ActorUID::INVALID) {
		if(jukebox.currentSong.songID != SongID::INVALID) {
			if(TimeDiffSec(TimeDiff(jukebox.playStartTime, localTime)) >= jukebox.currentSong.lengthInSec) {
				jukebox.currentSong = {};
			}
		}

		if(jukebox.currentSong.songID == SongID::INVALID) {
			if(!jukebox.queue.empty()) {
				jukebox.currentSong = jukebox.queue.front();
				jukebox.queue.pop_front();
				jukebox.playStartTime = localTime;
			}
		}
	}

	Replicate();
}

void World::Replicate()
{
	if(jukebox.UID != ActorUID::INVALID) {
		// replicate
		Replication::ActorJukebox rjb;
		rjb.actorUID = jukebox.UID;
		rjb.docID = jukebox.docID;
		rjb.pos = jukebox.pos;
		rjb.dir = jukebox.dir;
		rjb.localID = jukebox.localID;

		if(jukebox.currentSong.songID != SongID::INVALID) {
			rjb.playPosition = (i32)round(TimeDiffSec(TimeDiff(jukebox.playStartTime, localTime)));
			rjb.playStartTime = jukebox.playStartTime;
			rjb.currentSong = { jukebox.currentSong.songID, jukebox.currentSong.requesterNick };
		}
		else {
			rjb.playPosition = 0;
			rjb.playStartTime = Time::ZERO;
			rjb.currentSong = { SongID::INVALID };
		}

		foreach(it, jukebox.queue) {
			rjb.tracks.push_back({ it->songID, it->requesterNick });
		}

		replication->FramePushJukebox(rjb);
	}

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

World::ActorJukebox& World::SpawnJukeboxActor(CreatureIndex docID, i32 localID, const vec3& pos, const vec3& dir)
{
	ASSERT(jukebox.UID == ActorUID::INVALID);

	new(&jukebox) ActorJukebox(NewActorUID());
	jukebox.type = 1;
	jukebox.docID = (CreatureIndex)docID;
	jukebox.pos = pos;
	jukebox.dir = dir;
	jukebox.rotation = {0};
	jukebox.speed = 0;
	jukebox.actionState = ActionStateID::INVALID;
	jukebox.actionParam1 = -1;
	jukebox.actionParam2 = -1;
	jukebox.localID = localID;
	return jukebox;
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
