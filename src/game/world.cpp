#include "world.h"

void World::Init(Replication* replication_)
{
	replication = replication_;
	nextActorUID = 1;
}

void World::Update(f64 delta)
{
	ProfileFunction();

	// players
	foreach(it, actorPlayerList) {
		const ActorPlayer& actor = *it;

		Replication::Actor rfl;
		rfl.UID = actor.UID;
		rfl.type = actor.type;
		rfl.docID = actor.docID;
		rfl.pos = actor.pos;
		rfl.dir = actor.dir;
		rfl.eye = actor.eye;
		rfl.rotate = actor.rotate;
		rfl.upperRotate = actor.upperRotate;
		rfl.speed = actor.speed;
		rfl.spawnType = 0;
		rfl.actionState = actor.actionState;
		rfl.actionParam1 = actor.actionParam1;
		rfl.actionParam2 = actor.actionParam2;
		rfl.ownerID = 0;
		rfl.faction = 0;
		rfl.classType = actor.classType;
		rfl.skinIndex = actor.skinIndex;
		rfl.localID = -1;

		Replication::ActorNameplate plate;
		plate.name = actor.name;
		plate.guildTag = actor.guildTag;

		Replication::ActorStats stats;
		// TODO: fill those

		Replication::ActorPlayerInfo playerInfo;
		// TODO: fill those

		replication->FramePushActor(rfl, &plate, &stats, &playerInfo);
	}

	// clear action state
	foreach(it, actorPlayerList) {
		it->actionState = ActionStateID::INVALID;
		it->actionParam1 = -1;
		it->actionParam2 = -1;
	}

	// npcs
	foreach(it, actorNpcList) {
		const ActorNpc& actor = *it;

		Replication::Actor rfl;
		rfl.UID = actor.UID;
		rfl.type = actor.type;
		rfl.docID = actor.docID;
		rfl.pos = actor.pos;
		rfl.dir = actor.dir;
		rfl.eye = actor.eye;
		rfl.rotate = actor.rotate;
		rfl.upperRotate = actor.upperRotate;
		rfl.speed = actor.speed;
		rfl.spawnType = 0;
		rfl.actionState = (ActionStateID)99; // TODO: what's this?
		rfl.actionParam1 = 0;
		rfl.actionParam2 = 0;
		rfl.ownerID = 0;
		rfl.faction = -1;
		rfl.classType = ClassType::NONE; // -1 for NPCs
		rfl.skinIndex = SkinIndex::DEFAULT;
		rfl.localID = actor.localID;

		replication->FramePushActor(rfl, nullptr, nullptr, nullptr);
	}
}

ActorUID World::NewActorUID()
{
	return (ActorUID)nextActorUID++;
}

World::ActorPlayer& World::SpawnPlayerActor(i32 clientID, ClassType classType, SkinIndex skinIndex, const wchar* name, const wchar* guildTag)
{
	ActorUID actorUID = NewActorUID();
	ASSERT(FindPlayerActor(actorUID) == nullptr);

	ActorPlayer& actor = actorPlayerList.push_back();
	actor.UID = actorUID;
	actor.type = 1;
	actor.docID = (CreatureIndex)(100000000 + (i32)classType);
	actor.rotate = 0;
	actor.speed = 0;
	actor.actionState = ActionStateID::INVALID;
	actor.actionParam1 = -1;
	actor.actionParam2 = -1;
	actor.classType = classType;
	actor.skinIndex = skinIndex;
	actor.name = name;
	actor.guildTag = guildTag;

	actorPlayerMap.emplace(actorUID, --actorPlayerList.end());
	return actor;
}

World::ActorNpc& World::SpawnNpcActor(CreatureIndex docID, i32 localID)
{
	ActorUID actorUID = NewActorUID();
	ASSERT(FindNpcActor(actorUID) == nullptr);

	ActorNpc& actor = actorNpcList.push_back();
	actor.UID = actorUID;
	actor.type = 1;
	actor.docID = (CreatureIndex)docID;
	actor.eye = Vec3(0, 0, 0);
	actor.rotate = 0;
	actor.speed = 0;
	actor.actionState = ActionStateID::INVALID;
	actor.actionParam1 = -1;
	actor.actionParam2 = -1;
	actor.localID = localID;

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

World::List<World::ActorNpc>::iterator World::FindNpcActorByCreatureID(CreatureIndex docID)
{
	foreach(it, actorNpcList) {
		if(it->docID == docID) {
			return it;
		}
	}
	return actorNpcList.end();
}

World::List<World::ActorNpc>::const_iterator World::InvalidNpcHandle() const
{
	return actorNpcList.end();
}

bool World::DestroyPlayerActor(ActorUID actorUID)
{
	auto actorIt = actorPlayerMap.find(actorUID);
	if(actorIt == actorPlayerMap.end()) return false;

	actorPlayerList.erase(actorIt->second);
	actorPlayerMap.erase(actorIt);
	return true;
}
