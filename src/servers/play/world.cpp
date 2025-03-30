#include "world.h"
#include <mxm/game_content.h>

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

	const f64 tdelta = TimeDurationSec(localTime, localTime_);
	localTime = localTime_;
	physics.localTime = localTime;

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

		// cast skills
		if(p.input.cast.skillID != SkillID::INVALID) {
			// TODO: check for requirements in general
			// TODO: check for skill
			// TODO: check for cost
			const auto& cast = p.input.cast;
			PlayerCastSkill(p, cast.skillID, cast.pos, Slice<const ActorUID>(cast.targetList.data(), cast.targetList.size()));
			p.input.cast.skillID = SkillID::INVALID;
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

	// execute skill programs
	for(auto it = skillProgramList.begin(); it != skillProgramList.end(); ) {
		if(it->IsDoneExecuting()) {
			it = skillProgramList.erase_unsorted(it);
		}
		else {
			ExecuteSkillProgram(*it);
			++it;
		}
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
		rfl.action = actor.action;

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
	player.body = physics.CreateDynamicBody(110, 70, pos); // radius 100 is found in files but 110 (_AILength) matches better

	// clear input
	player.input.moveTo = pos;
	player.input.speed = 0;
	player.input.rot = rot;
	player.input.tag = 0;
	player.input.jump = 0;
	player.input.action = ActionStateID::INVALID;

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
	actor.faction = Faction::DYNAMIC;
	actor.action = ActionStateID::DYNAMIC_NORMAL_STAND;
	actor.tLastActionChange = localTime;

	actorDynamicMap.emplace(actorUID, --actorDynamicList.end());
	return actor;
}

World::Player& World::GetPlayer(u32 playerIndex)
{
	return players[playerIndex];
}

World::ActorMaster* World::FindMasterActor(ActorUID actorUID) const
{
	auto it = actorMasterMap.find(actorUID);
	if(it == actorMasterMap.end()) return nullptr;
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

ActorUID World::NewActorUID()
{
	return (ActorUID)nextActorUID++;
}

World::ActorMasterHandle World::MasterInvalidHandle()
{
	return actorMasterList.end();
}

void World::PlayerCastSkill(Player& player, SkillID skillID, const vec3& castPos, Slice<const ActorUID> targets)
{
	// TODO: check if can cast

	// access method is kinda convoluted
	const auto& content = GetGameXmlContent();
	const auto& skill = content.skillMap.at(skillID);
	const ActionStateID actionState = skill.action;

	const f32 angle = player.input.rot.upperYaw;
	const vec2 dir = vec2(cosf(angle), sinf(angle));

	Replication::SkillCast rpCast;
	rpCast.clientHd = player.clientHd;
	rpCast.casterUID = player.Main().UID;
	rpCast.skillID = skillID;
	rpCast.castPos = castPos;
	rpCast.actionID = actionState;

	rpCast.casterPos = player.body->GetWorldPos();
	rpCast.casterMoveDir = dir;
	rpCast.casterRot = { angle, 0, angle };
	rpCast.casterSpeed = player.input.speed; // FIXME: should not come from input

	eastl::copy(targets.begin(), targets.end(), eastl::back_inserter(rpCast.targetList));

	replication->FramePushSkillCast(rpCast);


	/*
	 * After the cast, we start executing a "skill program"
	 * It is a set of simple instructions in the world of MxM
	 * Each "program" is described in ActionBase.xml
	 */

	// Trigger new skill execution
	player.Main().actionState = actionState;

	SkillProgram prog;
	prog.skillID = skillID;
	prog.actionID = actionState;
	prog.castPos = castPos;
	prog.castAngle = angle;
	prog.casterUID = player.Main().UID;
	eastl::copy(targets.begin(), targets.end(), eastl::back_inserter(prog.targetList));
	prog.startTime = localTime;
	prog.commandID = 0;
	skillProgramList.push_back(prog);

	// go through the skill program to find out how much the master moves
	f32 distance = 0;
	f32 moveDuration = 0;

	const auto& action = content.GetSkillAction(player.Main().classType, actionState);

	foreach_const(cmd, action.commands) {
		switch(cmd->type) {
			case ActionCommand::Type::GRAPH_MOVE_HORZ: {
				distance = cmd->graphMoveHorz.distance;
				moveDuration = action.seqLength;
			} break;

			case ActionCommand::Type::MOVE: {
				switch(cmd->move.preset) {
					case ActionCommand::MovePreset::WARP: {
						distance = (f32)cmd->move.param2;
						moveDuration = 0.01f; // warping
					} break;
				}
			} break;
		}
	}

	Replication::SkillExec rpExec;
	rpExec.casterUID = player.Main().UID;
	rpExec.skillID = skillID;
	rpExec.castPos = castPos;
	rpExec.actionID = actionState;
	eastl::copy(targets.begin(), targets.end(), eastl::back_inserter(rpExec.targetList));

	rpExec.moveDuration = moveDuration;
	rpExec.startPos = player.body->GetWorldPos();
	rpExec.moveDir = dir;
	rpExec.rot = { angle, 0, angle };
	rpExec.speed = player.input.speed; // FIXME: should not come from input

	if(distance != 0) {
		const vec3 endPos = physics.FindMovePos(player.body, vec3(dir * distance, 0), moveDuration);
		rpExec.endPos = endPos;
	}

	replication->FramePushSkillExec(rpExec);
}

void World::ExecuteSkillProgram(SkillProgram& prog)
{
	// TODO: each skill is executed following a list of commands from ActionBase.xml
	// process them at runtime for now
	// but since they never change, produce logic code from ActionBase.xml

	ActorMaster* caster = FindMasterActor(prog.casterUID);

	const vec2 dir = vec2(cosf(prog.castAngle), sinf(prog.castAngle));

	// access method is kinda convoluted
	const auto& content = GetGameXmlContent();
	const auto& action = content.GetSkillAction(caster->classType, prog.actionID);

	if(TimeDiffSec(TimeDiff(prog.startTime, localTime)) > action.commands[prog.commandID].relativeEndTimeFromStart) {
		prog.commandID++;

		// program is done
		if(prog.commandID >= action.commands.size()) {
			prog.Finish();
			return;
		}
	}
	else {
		// we have not changed command / instruction, nothing to be done
		return;
	}

	bool running = true;
	while(running) {
		running = false;
		const auto& cmd = action.commands[prog.commandID];

		f32 distance = 0;
		f32 moveDuration = 0;

		switch(cmd.type) {
			case ActionCommand::Type::STATE_BLOCK: {
				// lock WASD input type movement during skill execution
				caster->parent->body->lockedMoveUntil = TimeAddSec(localTime, cmd.delay);
			} break
				;
			case ActionCommand::Type::GRAPH_MOVE_HORZ: {
				distance = cmd.graphMoveHorz.distance;
				moveDuration = action.seqLength;
			} break;

			case ActionCommand::Type::MOVE: {
				switch(cmd.move.preset) {
					case ActionCommand::MovePreset::WARP: {
						distance = (f32)cmd.move.param2;
						moveDuration = 0.01f; // warping
					} break;
				}
			} break;
		}

		if(distance != 0) {
			caster->parent->input.moveTo = caster->parent->body->GetWorldPos() + vec3(dir * distance, 0);
			caster->parent->body->vel = vec3(0);
			physics.Move(caster->parent->body, vec3(dir * distance, 0), moveDuration);
		}

		if(cmd.delay == 0) {
			running = true;
			prog.commandID++;

			// program is done
			if(prog.commandID >= action.commands.size()) {
				prog.Finish();
				return;
			}
		}
	}
}
