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

	localTime = localTime_;
	physics.localTime = localTime;

	foreach(it, players) {
		Player& p = *it;
		p.movement.rot = p.input.rot;
		p.movement.hasJumped = false;
		p.movement.forcedMove = false;

		if(p.input.tag) {
			p.input.tag = 0;
			p.mainCharaID ^= 1;
			const auto& character = GetGameXmlContent().GetMaster(p.Main().classType).character;
			physics.ResizeDynamicBody(p.body, (f32)character.getActorRadius(), (f32)character.getActorHeight());
		}

		if(p.input.cast.skillID != SkillID::INVALID) {
			const auto& cast = p.input.cast;
			PlayerCastSkill(p, cast.skillID, cast.pos, Slice<const ActorUID>(cast.targetList.data(), cast.targetList.size()), cast.clientTime);
			p.input.cast.skillID = SkillID::INVALID;
		}
	}

	for(auto it = skillProgramList.begin(); it != skillProgramList.end(); ) {
		if(!it->IsDoneExecuting()) {
			ExecuteSkillProgram(*it);
		}
		if(it->IsDoneExecuting()) {
			it = skillProgramList.erase_unsorted(it);
		}
		else {
			++it;
		}
	}

	foreach(it, players) {
		Player& p = *it;
		PhysicsDynamicBody& body = *p.body;
		const bool inputBlocked = localTime < p.movement.lockedMoveUntil || p.movement.forcedMove;
		const vec2 delta = vec2(p.input.moveTo - body.GetWorldPos());
		const f32 deltaLen = glm::length(delta);
		if(!inputBlocked && deltaLen > 1.0f && p.input.speed > 0.f) {
			p.movement.moveDir = NormalizeSafe(delta);
			p.movement.moveSpeed = eastl::min(p.input.speed, f32(deltaLen / UPDATE_RATE));
		}
		else {
			p.movement.moveDir = vec2(0);
			p.movement.moveSpeed = 0.0f;
		}

		if(p.input.jump) {
			p.input.jump = 0;
			if(body.grounded && !inputBlocked) {
				p.movement.hasJumped = true;
				body.grounded = false;
				body.vel.z = GetGlobalTweakableVars().jumpForce;
			}
		}

		body.vel.x = p.movement.moveDir.x * p.movement.moveSpeed;
		body.vel.y = p.movement.moveDir.y * p.movement.moveSpeed;
	}

	physics.Step();
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
		rep.skills = player.skills;
		rep.mainWeapon = player.mainWeapon;
		rep.subWeapon = player.subWeapon;
		rep.mainWeaponGrade = player.mainWeaponGrade;
		rep.subWeaponGrade = player.subWeaponGrade;
		rep.mainMasterGearNo = player.mainMasterGearNo;
		rep.subMasterGearNo = player.subMasterGearNo;
		rep.mainCharacterType = player.mainCharacterType;
		rep.subCharacterType = player.subCharacterType;

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
			rch.hp = chara.hp;
			rch.hpMax = chara.hpMax;
			rch.seed = chara.seed;
			rch.spawnAnim = chara.spawnAnim;
			rch.ownerID = chara.ownerID;
			rch.dirToNearPC = chara.dirToNearPC;
			rch.wanderDist = chara.wanderDist;
			rch.tagID = chara.tagID;




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
			rch.hp = chara.hp;
			rch.hpMax = chara.hpMax;
			rch.seed = chara.seed;
			rch.spawnAnim = chara.spawnAnim;
			rch.ownerID = chara.ownerID;
			rch.dirToNearPC = chara.dirToNearPC;
			rch.wanderDist = chara.wanderDist;
			rch.tagID = chara.tagID;




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
		rfl.spawnAnim = actor.spawnAnim;
		rfl.ownerID = actor.ownerID;
		rfl.dirToNearPC = actor.dirToNearPC;
		rfl.wanderDist = actor.wanderDist;
		rfl.tagID = actor.tagID;
		rfl.actionState = actor.actionState;
		rfl.seed = actor.seed;
		rfl.entityType = actor.entityType;


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
		rfl.spawnAnim = actor.spawnAnim;
		rfl.ownerID = actor.ownerID;
		rfl.dirToNearPC = actor.dirToNearPC;
		rfl.wanderDist = actor.wanderDist;
		rfl.tagID = actor.tagID;
		rfl.seed = actor.seed;
		rfl.entityType = actor.entityType;


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
	const auto& character = GetGameXmlContent().GetMaster(player.mainClass).character;
	player.body = physics.CreateDynamicBody((f32)character.getActorRadius(), (f32)character.getActorHeight(), pos);

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
	main.hp = 2400;
	main.hpMax = 2400;
	main.seed = (i32)RandUint();




	sub.parent = &player;
	sub.classType = player.subClass;
	sub.skinIndex = player.subSkin;
	sub.hp = 2400;
	sub.hpMax = 2400;
	sub.seed = (i32)RandUint();



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

void World::PlayerCastSkill(Player& player, SkillID skillID, const vec3& castPos, Slice<const ActorUID> targets, f32 clientTime)
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
	rpCast.clientTime = clientTime;

	eastl::copy(targets.begin(), targets.end(), eastl::back_inserter(rpCast.targetList));

	replication->FramePushSkillCast(rpCast);


	player.Main().actionState = actionState;

	foreach(it, skillProgramList) {
		ActorMaster* caster = FindMasterActor(it->casterUID);
		if(caster && caster->parent == &player) {
			it->Finish();
		}
	}
	player.movement.lockedMoveUntil = Time::ZERO;

	SkillProgram prog;
	prog.skillID = skillID;
	prog.actionID = actionState;
	prog.castPos = castPos;
	prog.castAngle = angle;
	prog.casterUID = player.Main().UID;
	eastl::copy(targets.begin(), targets.end(), eastl::back_inserter(prog.targetList));
	prog.startTime = localTime;
	prog.moveStartPos = player.body->GetWorldPos();
	prog.moveEndPos = prog.moveStartPos;

	const auto& action = content.GetSkillAction(player.Main().classType, actionState);
	f32 distance = 0;
	bool hasGraph = false;
	foreach_const(cmd, action.commands) {
		if(cmd->type == ActionCommand::Type::GRAPH_MOVE_HORZ) {
			distance = cmd->graphMoveHorz.distance;
			hasGraph = true;
		}
	}

	if(hasGraph && distance != 0) {
		ASSERT(action.seqLength >= 0);
		prog.moveDuration = action.seqLength;
		prog.moveEndPos = physics.FindMovePos(player.body, vec3(dir * distance, 0), UPDATE_RATE);
		prog.moving = true;
	}

	ExecuteSkillProgram(prog);

	Replication::SkillExec rpExec;
	rpExec.casterUID = player.Main().UID;
	rpExec.skillID = skillID;
	rpExec.castPos = castPos;
	rpExec.actionID = actionState;
	eastl::copy(targets.begin(), targets.end(), eastl::back_inserter(rpExec.targetList));

	rpExec.moveDuration = prog.moveDuration;
	rpExec.startPos = prog.moveStartPos;
	rpExec.endPos = prog.moveDuration > 0 ? prog.moveEndPos : player.body->GetWorldPos();
	rpExec.moveDir = dir;
	rpExec.rot = { angle, 0, angle };
	rpExec.speed = player.movement.moveSpeed;

	if(!prog.IsDoneExecuting()) {
		skillProgramList.push_back(prog);
	}

	replication->FramePushSkillExec(rpExec);
}

void World::ExecuteSkillProgram(SkillProgram& prog)
{
	ActorMaster* caster = FindMasterActor(prog.casterUID);
	if(!caster || caster->parent->Main().UID != prog.casterUID) {
		prog.Finish();
		return;
	}

	Player& player = *caster->parent;
	PhysicsDynamicBody* body = player.body;
	const auto& action = GetGameXmlContent().GetSkillAction(caster->classType, prog.actionID);
	const f32 elapsed = (f32)TimeDurationSec(prog.startTime, localTime);

	if(prog.moving) {
		const f32 progress = prog.moveDuration > 0 ? eastl::min(elapsed / prog.moveDuration, 1.0f) : 1.0f;
		const vec3 target = prog.moveStartPos + (prog.moveEndPos - prog.moveStartPos) * progress;
		const vec3 current = body->GetWorldPos();
		const vec3 displacement = vec3(vec2(target - current), 0);
		if(glm::dot(displacement, displacement) > 0) {
			physics.Move(body, displacement, UPDATE_RATE);
		}
		player.movement.forcedMove = true;
		player.input.moveTo = body->GetWorldPos();
		prog.moving = progress < 1.0f;
	}

	while(prog.commandID < action.commands.size()) {
		const auto& cmd = action.commands[prog.commandID];
		const f32 commandTime = cmd.type == ActionCommand::Type::STATE_BLOCK
			? (prog.commandID == 0 ? 0.0f : action.commands[prog.commandID - 1].relativeEndTimeFromStart)
			: cmd.relativeEndTimeFromStart;
		if(elapsed < commandTime) {
			break;
		}

		switch(cmd.type) {
			case ActionCommand::Type::STATE_BLOCK: {
				const Time until = TimeAddSec(prog.startTime, cmd.relativeEndTimeFromStart);
				player.movement.lockedMoveUntil = eastl::max(player.movement.lockedMoveUntil, until);
			} break;

			case ActionCommand::Type::MOVE: {
				if(cmd.move.preset == ActionCommand::MovePreset::WARP) {
					const vec2 dir = vec2(cosf(prog.castAngle), sinf(prog.castAngle));
					const vec3 startPos = body->GetWorldPos();
					const vec3 endPos = physics.FindMovePos(body, vec3(dir * (f32)cmd.move.param2, 0), UPDATE_RATE);
					physics.Move(body, endPos - startPos, UPDATE_RATE);
					player.input.moveTo = body->GetWorldPos();
					player.movement.forcedMove = true;
					if(player.clientHd != ClientHandle::INVALID) {
						replication->FrameRequestPositionCorrection(player.clientHd, caster->UID);
					}
				}
			} break;
		}
		++prog.commandID;
	}

	const f32 commandEnd = action.commands.empty() ? 0.0f : action.commands.back().relativeEndTimeFromStart;
	if(prog.commandID == action.commands.size() && !prog.moving && elapsed >= eastl::max(action.seqLength, commandEnd)) {
		caster->actionState = ActionStateID::INVALID;
		prog.Finish();
	}
}
