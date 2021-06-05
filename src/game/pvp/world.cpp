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

	// players: handle input
	foreach(it, players) {
		Player& p = *it;

		// tag
		if(p.input.tag) {
			p.input.tag = 0;
			p.mainCharaID ^= 1;

			p.Main().body->pos = p.Sub().body->pos;
		}

		// move
		PhysWorld::BodyCapsule& bodyMain = *p.Main().body;
		PhysWorld::BodyCapsule& bodySub = *p.Sub().body;
		bodyMain.flags &= ~PhysWorld::Flags::Disabled; // enable current body
		bodySub.flags |= PhysWorld::Flags::Disabled; // disable tagged out body

		vec2 delta = vec2(p.input.moveTo - bodyMain.pos);
		f32 deltaLen = glm::length(delta);
		if(deltaLen > 1.0f && p.input.speed > 0.f) {
			vec2 dir = NormalizeSafe(delta);
			p._moveDir = dir;

			// we're close enough that we might miss the point by going full speed in a step, slow down
			if(deltaLen < (p.input.speed * UPDATE_RATE)) {
				bodyMain.vel = vec3(dir.x, dir.y, 0) * f32(deltaLen/UPDATE_RATE);
			}
			else {
				bodyMain.vel = vec3(dir.x, dir.y, 0) * p.input.speed;
			}
		}
		else {
			p._moveDir = vec2(0);
			bodyMain.vel = vec3(0);
		}
	}

	physics.Step();

	Replicate();
}

void World::Replicate()
{
	// players
	foreach_const(it, players) {
		const Player& player = *it;

		Replication::Player rep;
		rep.playerID = player.playerID;
		rep.clientID = player.clientID;
		rep.name = player.name;
		rep.guildTag = player.guildTag;

		rep.mainClass = player.mainClass;
		rep.mainSkin = player.mainSkin;
		rep.subClass = player.subClass;
		rep.subSkin = player.subSkin;

		rep.characters = {
			player.characters[0]->UID,
			player.characters[1]->UID
		};
		rep.mainCharaID = player.mainCharaID;

		replication->FramePushPlayer(rep);

		foreach_const(chit, player.characters) {
			const ActorMaster& chara = **chit;
			Replication::ActorMaster rch;
			rch.actorUID = chara.UID;
			rch.clientID = player.clientID;
			rch.playerID = player.playerID;
			rch.classType = chara.classType;
			rch.skinIndex = chara.skinIndex;
			rch.pos = chara.body->pos;

			rch.moveDir = player._moveDir;
			rch.speed = player.input.speed;

			rch.rotation = player.input.rot; // TODO: compute this?

			rch.actionState = chara.actionState;
			rch.actionParam1 = chara.actionParam1;
			rch.actionParam2 = chara.actionParam2;

			replication->FramePushMasterActor(rch);
		}
	}

	// clear action state
	foreach(it, actorMasterList) {
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

World::Player& World::CreatePlayer(i32 clientID, const wchar* name, const wchar* guildTag, ClassType mainClass, SkinIndex mainSkin, ClassType subClass, SkinIndex subSkin)
{
	players.emplace_back((PlayerID)players.size(), clientID, name, guildTag, mainClass, mainSkin, subClass, subSkin);
	Player& player = players.back();
	player.level = 1;
	player.experience = 0;

	player.characters.fill(MasterInvalidHandle());
	return player;
}

World::ActorMaster& World::SpawnPlayerMasters(Player& player, const vec3& pos)
{
	ASSERT(player.characters[0] == MasterInvalidHandle());
	ASSERT(player.characters[1] == MasterInvalidHandle());

	const ActorUID mainUID = NewActorUID();
	const ActorUID subUID = NewActorUID();

	actorMasterList.emplace_back(mainUID);
	ActorMaster& main = actorMasterList.back();
	ActorMasterHandle hMain = --actorMasterList.end();
	actorMasterMap.emplace(mainUID, hMain);

	actorMasterList.emplace_back(subUID);
	ActorMaster& sub = actorMasterList.back();
	ActorMasterHandle hSub = --actorMasterList.end();
	actorMasterMap.emplace(subUID, hSub);

	player.characters = {
		hMain,
		hSub
	};

	main.parent = &player;
	main.classType = player.mainClass;
	main.skinIndex = player.mainSkin;
	main.body = physics.CreateBody(100, 270, pos);

	sub.parent = &player;
	sub.classType = player.subClass;
	sub.skinIndex = player.subSkin;
	sub.body = physics.CreateBody(100, 270, pos);

	return main;
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

World::Player* World::FindPlayer(PlayerID playerID)
{
	if((i32)playerID >= players.size()) return nullptr;
	return &(players[(u32)playerID]);
}

World::Player& World::GetPlayer(PlayerID playerID)
{
	return players[(u32)playerID];
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

ActorUID World::NewActorUID()
{
	return (ActorUID)nextActorUID++;
}

World::ActorMasterHandle World::MasterInvalidHandle()
{
	return actorMasterList.end();
}
