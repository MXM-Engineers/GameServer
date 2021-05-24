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
		//ActorPlayer& p = *it;
		//p.body->pos = p.pos;
		//p.body->vel = vec3(p.dir.x, p.dir.y, 0) * p.speed;
	}

	physics.Step();

	// players
	foreach(it, actorPlayerList) {
		ActorPlayer& p = *it;

		/*p.pos = p.body->pos;
		vec3 vel = p.body->vel;
		if(LengthSq(vel) > 0.0001f) {
			p.dir = glm::normalize(vel);
		}
		else {
			p.dir = vec3(0, 0, 0);
		}
		p.speed = glm::length(vel);*/
	}

	Replicate();
}

void World::Replicate()
{
	// players
	foreach_const(it, actorPlayerList) {
		const ActorPlayer& player = *it;

		Replication::ActorPlayer rep;
		rep.actorUID = player.UID;
		rep.clientID = player.clientID;
		rep.name = player.name;
		rep.guildTag = player.guildTag;
		rep.characters = {
			player.characters[PlayerCharaID::Main]->UID,
			player.characters[PlayerCharaID::Sub]->UID
		};
		rep.currentCharaID = player.currentCharaID;

		replication->FramePushPlayerActor(rep);

		foreach_const(chit, player.characters) {
			const ActorPlayerCharacter& chara = **chit;
			Replication::ActorPlayerCharacter rch;
			rch.actorUID = chara.UID;
			rch.clientID = player.clientID;
			rch.parentActorUID = player.UID;
			rch.classType = chara.classType;
			rch.skinIndex = chara.skinIndex;
			rch.pos = chara.body->pos;
			rch.dir = NormalizeSafe(chara.body->vel);
			rch.speed = glm::length(chara.body->vel);
			rch.rotation = player.input.rot; // TODO: compute this?

			rch.actionState = chara.actionState;
			rch.actionParam1 = chara.actionParam1;
			rch.actionParam2 = chara.actionParam2;

			replication->FramePushPlayerCharacterActor(rch);
		}
	}

	// clear action state
	foreach(it, actorPlayerCharacterList) {
		it->actionState = ActionStateID::INVALID;
		it->actionParam1 = -1;
		it->actionParam2 = -1;
	}

	// npcs
	foreach_const(it, actorNpcList) {
		const ActorNpc& actor = *it;

		Replication::ActorNpc rfl;
		rfl.actorUID = actor.UID;
		rfl.type = 1;
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

World::ActorPlayer& World::SpawnPlayer(i32 clientID, const wchar* name, const wchar* guildTag, ClassType mainClass, SkinIndex mainSkin, ClassType subClass, SkinIndex subSkin, const vec3& pos)
{
	const ActorUID playerUID = NewActorUID();
	const ActorUID mainUID = NewActorUID();
	const ActorUID subUID = NewActorUID();

	actorPlayerCharacterList.emplace_back(mainUID);
	ActorPlayerCharacter& main = actorPlayerCharacterList.back();
	ActorPlayerCharacterHandle hMain = --actorPlayerCharacterList.end();
	actorPlayerCharacterMap.emplace(mainUID, hMain);

	actorPlayerCharacterList.emplace_back(subUID);
	ActorPlayerCharacter& sub = actorPlayerCharacterList.back();
	ActorPlayerCharacterHandle hSub = --actorPlayerCharacterList.end();
	actorPlayerCharacterMap.emplace(subUID, hSub);

	actorPlayerList.emplace_back(playerUID, clientID, name, guildTag);
	ActorPlayer& player = actorPlayerList.back();
	PlayerHandle hPlayer = --actorPlayerList.end();
	actorPlayerMap.emplace(playerUID, hPlayer);

	player.characters = {
		hMain,
		hSub
	};

	main.parent = hPlayer;
	main.classType = mainClass;
	main.skinIndex = mainSkin;
	main.body = physics.CreateBody(45, 210, pos);

	sub.parent = hPlayer;
	sub.classType = subClass;
	sub.skinIndex = subSkin;
	sub.body = physics.CreateBody(45, 210, pos);

	return player;
}

World::ActorNpc& World::SpawnNpcActor(CreatureIndex docID, i32 localID)
{
	ActorUID actorUID = NewActorUID();

	actorNpcList.emplace_back(actorUID);
	ActorNpc& actor = actorNpcList.back();
	actor.docID = (CreatureIndex)docID;
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

	foreach_const(chit, actorIt->second->characters) {
		actorPlayerCharacterMap.erase((*chit)->UID);
		actorPlayerCharacterList.erase(*chit);
	}

	actorPlayerList.erase(actorIt->second);
	actorPlayerMap.erase(actorIt);
	return true;
}
