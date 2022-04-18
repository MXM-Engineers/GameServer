#include "world.h"

void World::Init(Replication* replication_)
{
	replication = replication_;
	nextActorUID = 1;

	auto& ctx = PhysContext();
	ctx.CreateScene(&physics);
}

void World::Cleanup()
{
	physics.Destroy();
}

void World::Update(Time localTime_)
{
	ProfileFunction();

	const f64 delta = TimeDurationSec(localTime, localTime_);
	localTime = localTime_;

	vec3 prevPos = players.front().body->GetWorldPos();

	// players: handle input
	foreach(it, players) {
		Player& p = *it;

		PhysicsDynamicBody& body = *p.body;

		p.movement.rot = p.input.rot;

		// tag
		if(p.input.tag) {
			p.input.tag = 0;
			p.mainCharaID ^= 1;
		}

		// cast skill
		p.cast.skill = SkillID::INVALID;
		if(p.input.castSkill != SkillID::INVALID) {
			// TODO: check for requirements in general
			// TODO: check for skill
			// TODO: check for cost
			p.cast.skill = p.input.castSkill;

			ActionStateID actionState;
			f32 distance = 0;
			f32 moveDuration = 0;

			switch(p.cast.skill) {
				// Lua dodge
				case (SkillID)180350002: {
					actionState = ActionStateID::SHIRK_BEHAVIORSTATE;
					distance = 500;
					moveDuration = 0.7333333f;
				} break;

				// Sizuka dodge
				case (SkillID)180030002: {
					actionState = ActionStateID::SHIRK_BEHAVIORSTATE;
					distance = 500;
					moveDuration = 0.01f;
				} break;

				case (SkillID)180350010: actionState = ActionStateID::SKILL_1_BEHAVIORSTATE; break;
				case (SkillID)180350030: actionState = (ActionStateID)30; break;
				case (SkillID)180030020: actionState = (ActionStateID)32; break;
				case (SkillID)180030030: actionState = (ActionStateID)33; break;
				case (SkillID)180030050: actionState = (ActionStateID)35; break;
			}

			if(distance > 0) {
				p.cast.startPos = body.GetWorldPos();
				const f32 angle = p.input.rot.upperYaw;
				vec2 dir = vec2(cosf(angle), sinf(angle));
				const vec3 endPos = physics.Move(p.body, vec3(dir * distance, 0), moveDuration);

				p.cast.endPos = endPos;
				p.input.moveTo = endPos;
				p.movement.moveDir = dir;
				p.movement.rot = { angle, 0, angle };
			}
			p.cast.moveDurationS = moveDuration;

			p.input.castSkill = SkillID::INVALID;
			p.Main().actionState = actionState;
		}

		// move
		vec2 delta = vec2(p.input.moveTo - body.GetWorldPos());
		f32 deltaLen = glm::length(delta);
		if(deltaLen > 1.0f && p.input.speed > 0.f) {
			vec2 dir = NormalizeSafe(delta);
			p.movement.moveDir = dir;

			// we're close enough that we might miss the point by going full speed in a step, slow down
			if(deltaLen < (p.input.speed * UPDATE_RATE)) {
				p.movement.moveSpeed = f32(deltaLen/UPDATE_RATE);
			}
			else {
				p.movement.moveSpeed = p.input.speed;
			}
		}
		else {
			p.movement.moveDir = vec2(0);
			p.movement.moveSpeed = 0.0f;
		}

		// jump
		p.movement.hasJumped = false;
		if(p.input.jump) {
			p.input.jump = 0;
			p.movement.hasJumped = true;
			body.vel.z = GetGlobalTweakableVars().jumpForce;
		}

		// apply 2D velocity
		const f32 s = p.movement.moveSpeed;
		body.vel.x = p.movement.moveDir.x * s;
		body.vel.y = p.movement.moveDir.y * s;
	}

	physics.Step();

	/*
	static f64 accumulatedDiff = 0.0;
	if(players.front().movement.moveDir == vec2(0)) {
		accumulatedDiff = 0.0;
	}

	vec3 deltaPos = players.front().body->pos - prevPos;
	f32 moveDiff = players.front().input.speed * UPDATE_RATE - glm::length(vec2(deltaPos));
	if(moveDiff != 0 && players.front().movement.moveDir != vec2(0)) {
		accumulatedDiff += moveDiff;
		LOG("Move diff = %f  |  accumulatedDiff = %g", moveDiff, accumulatedDiff);
	}
	*/

	Replicate();
}

void World::Replicate()
{
	eastl::fixed_vector<Replication::ActorMaster, 64, false> repMasterList;

	// players
	foreach_const(it, players) {
		const Player& player = *it;

		Replication::Player rep;
		rep.index = player.index;
		rep.userID = player.userID;
		rep.clientHd = player.clientHd;
		rep.name = player.name;
		rep.guildTag = player.guildTag;
		rep.team = player.team;

		rep.mainClass = player.mainClass;
		rep.mainSkin = player.mainSkin;
		rep.subClass = player.subClass;
		rep.subSkin = player.subSkin;

		rep.masters = {
			player.characters[0]->UID,
			player.characters[1]->UID
		};
		rep.mainCharaID = player.mainCharaID;
		rep.hasJumped = player.movement.hasJumped;

		replication->FramePushPlayer(rep);

		// main
		{
			const ActorMaster& chara = player.Main();
			Replication::ActorMaster& rch = repMasterList.push_back();
			rch.actorUID = chara.UID;
			rch.clientHd = player.clientHd;
			rch.playerIndex = player.index;
			rch.classType = chara.classType;
			rch.skinIndex = chara.skinIndex;

			rch.pos = player.body->GetWorldPos();
			rch.moveDir = player.movement.moveDir;
			rch.speed = player.input.speed;
			rch.rotation = player.movement.rot;

			rch.actionState = chara.actionState;
			rch.actionParam1 = chara.actionParam1;
			rch.actionParam2 = chara.actionParam2;

			rch.castSkill = player.cast.skill;
			rch.skillStartPos = player.cast.startPos;
			rch.skillEndPos = player.cast.endPos;
			rch.skillMoveDurationS = player.cast.moveDurationS;

			rch.taggedOut = false;
		}

		// sub
		{
			const ActorMaster& chara = player.Sub();
			Replication::ActorMaster& rch = repMasterList.push_back();
			rch.actorUID = chara.UID;
			rch.clientHd = player.clientHd;
			rch.playerIndex = player.index;
			rch.classType = chara.classType;
			rch.skinIndex = chara.skinIndex;

			rch.pos = player.body->GetWorldPos();
			rch.moveDir = player.movement.moveDir;
			rch.speed = player.input.speed;
			rch.rotation = player.movement.rot;

			rch.actionState = chara.actionState;
			rch.actionParam1 = chara.actionParam1;
			rch.actionParam2 = chara.actionParam2;

			rch.castSkill = player.cast.skill;
			rch.skillStartPos = player.cast.startPos;
			rch.skillEndPos = player.cast.endPos;
			rch.skillMoveDurationS = player.cast.moveDurationS;

			rch.taggedOut = true;
		}
	}

	replication->FramePushMasterActors(repMasterList.data(), repMasterList.size());

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
		rfl.docID = actor.docID;
		rfl.pos = actor.pos;
		rfl.dir = actor.rot;
		rfl.localID = actor.localID;
		rfl.faction = actor.faction;

		replication->FramePushNpcActor(rfl);
	}

	// dynamic
	foreach_const(it, actorDynamicList) {
		const ActorDynamic& actor = *it;

		Replication::ActorDynamic rfl;
		rfl.actorUID = actor.UID;
		rfl.docID = actor.docID;
		rfl.pos = actor.pos;
		rfl.rot = actor.rot;
		rfl.localID = actor.localID;
		rfl.faction = actor.faction;

		replication->FramePushDynamicActor(rfl);
	}
}

World::Player& World::CreatePlayer(const PlayerDescription& desc, const vec3& pos, const RotationHumanoid& rot)
{
	players.emplace_back(players.size(), desc);

	Player& player = players.back();
	player.mainCharaID = 0;
	player.level = 1;
	player.experience = 0;
	player.body = physics.CreateDynamicBody(90, 270, pos); // radius 100 is found in files but 90 matches better

	// clear input
	player.input.moveTo = pos;
	player.input.speed = 0;
	player.input.rot = rot;
	player.input.tag = 0;
	player.input.jump = 0;
	player.input.actionState = ActionStateID::INVALID;

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

	sub.parent = &player;
	sub.classType = player.subClass;
	sub.skinIndex = player.subSkin;
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

World::ActorDynamic& World::SpawnDynamic(CreatureIndex docID, i32 localID)
{
	ActorUID actorUID = NewActorUID();

	actorDynamicList.emplace_back(actorUID);
	auto& actor = actorDynamicList.back();
	actor.docID = (CreatureIndex)docID;
	actor.localID = localID;

	actorDynamicMap.emplace(actorUID, --actorDynamicList.end());
	return actor;
}

World::Player& World::GetPlayer(u32 playerIndex)
{
	return players[playerIndex];
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
