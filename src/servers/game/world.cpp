#include "config.h"
#include "world.h"
#include <mxm/game_content.h>
#include "world_common.h"

static const i32 STAT_HEALTH = 0;
static const f64 WEAPON_HIT_INTERVAL_MS = 80.0;
static const i32 SKILL_SLOT_ENUM_FIRST = 2;
static const f32 OBSTRUCT_LOW_HEIGHT = 30.0f;
static const i32 SKILL_SLOT_ENUM_UG = 6;

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

	foreach(it, players) {
		it->posHistory[it->posHistoryHead] = it->body->GetWorldPos();
		it->posHistoryHead = (it->posHistoryHead + 1) % it->posHistory.size();
		it->posHistoryCount = MIN(it->posHistoryCount + 1, (u32)it->posHistory.size());
	}

	// players: handle input
	foreach(it, players) {
		Player& p = *it;

		PhysicsDynamicBody& body = *p.body;

		p.movement.rot = p.input.rot;

		if(!p.dead && p.dashDone < p.dashDist) {
			const f32 total = (f32)TimeDiffSec(TimeDiff(p.dashStartAt, p.dashUntil));
			const f32 frac = total > 0 ? MIN((f32)TimeDiffSec(TimeDiff(p.dashStartAt, localTime)) / total, 1.0f) : 1.0f;
			const f32 target = p.dashCurve ? p.dashDist * p.dashCurve->Eval(frac) : p.dashDist * frac * frac;
			const f32 ahead = glm::dot(vec2(p.dashOrigin) + p.dashDir * target - vec2(body.GetWorldPos()), p.dashDir);
			if(p.dashCurve ? fabsf(ahead) > 0.01f : ahead > 0) physics.Move(&body, vec3(p.dashDir * ahead, 0), (f32)tdelta);
			p.dashDone = frac >= 1.0f ? p.dashDist : (p.dashCurve ? MIN(fabsf(target), p.dashDist - 0.01f) : target);
			p.input.moveTo = body.GetWorldPos();
		}

		if(!p.dead && p.clientHd == ClientHandle::INVALID && p.dragElapsed < p.dragDuration) {
			const f32 dt = MIN((f32)tdelta, p.dragDuration - p.dragElapsed);
			const f32 accel = (p.dragMax - p.dragMin) / p.dragDuration;
			const f32 t0 = p.dragElapsed;
			const f32 t1 = t0 + dt;
			const f32 step = (t1 * t1 - t0 * t0) * accel * 0.5f + p.dragMin * dt;
			const vec2 rel = p.dragCenter - vec2(body.GetWorldPos());
			const f32 dist = glm::length(rel);
			p.dragElapsed = t1;
			if(dist > 0.001f) {
				if(dist * dist < step * step) {
					physics.Move(&body, vec3(rel, 0), (f32)tdelta);
					p.dragElapsed = p.dragDuration;
				}
				else physics.Move(&body, vec3(rel / dist * step, 0), (f32)tdelta);
			}
			p.input.moveTo = body.GetWorldPos();
		}

		if(!p.dead && p.clientHd == ClientHandle::INVALID && localTime < p.ccUntil) {
			const ActiveStatus* fear = nullptr;
			foreach_const(st, activeStatuses) {
				if(st->targetIndex == p.index && st->info && localTime < st->until && st->info->type == StatusType::FEAR) { fear = &*st; break; }
			}
			if(localTime < p.pushUntil) {
				p.input.moveTo = p.pushTarget;
				p.input.speed = p.pushSpeed;
			}
			else if(fear) {
				if(localTime >= p.fearNextTurn) {
					const f32 a = (f32)(RandUint() % 3600) * PI / 1800.0f;
					p.fearDir = vec2(cosf(a), sinf(a));
					p.fearNextTurn = TimeAdd(localTime, TimeMsToTime(MAX(fear->info->intervalSec, 0.1f) * 1000.0));
				}
				p.input.moveTo = body.GetWorldPos() + vec3(p.fearDir * 500.0f, 0);
				p.input.speed = MasterStat(p.Main().classType, STAT_MOVESPEED);
				const f32 yaw = atan2f(p.fearDir.y, p.fearDir.x);
				p.input.rot.upperYaw = yaw;
				p.input.rot.bodyYaw = yaw;
			}
			else {
				p.input.moveTo = body.GetWorldPos();
			}
			p.input.tag = 0;
			p.input.jump = 0;
		}

		if(p.dead) {
			p.input.moveTo = body.GetWorldPos();
			p.input.tag = 0;
			p.input.jump = 0;
			p.input.cast.skillID = SkillID::INVALID;
			p.weaponFired = false;
			if(localTime > p.tRespawn) {
				RespawnPlayer(p);
			}
		}

		// tag
		if(p.input.tag) {
			p.input.tag = 0;
			p.mainCharaID ^= 1;
			eastl::swap(p.mana, p.benchMana);
			eastl::swap(p.res, p.benchRes);
			p.tNextManaSend = Time::ZERO;
			p.resSent = { -1, -1, -1 };
			p.tNextResSend = Time::ZERO;
		}

		// cast skills
		if(p.input.cast.skillID != SkillID::INVALID) {
			// TODO: check for requirements in general
			// TODO: check for skill
			// TODO: check for cost
			const auto& cast = p.input.cast;
			PlayerCastSkill(p, cast.skillID, cast.pos, Slice<const ActorUID>(cast.targetList.data(), cast.targetList.size()), cast.clientTime);
			p.input.cast.skillID = SkillID::INVALID;
		}

		if(p.weaponFired) {
			p.weaponFired = false;
			if(p.clientHd != ClientHandle::INVALID && p.casting.skillID != SkillID::INVALID && p.casting.type == GameXmlContent::SkillCast::LIMIT) ReleaseCast(p);
			else if(WeaponIsMelee(p) && localTime > p.tNextWeaponHit) {
				p.tNextWeaponHit = TimeAdd(localTime, TimeMsToTime(WEAPON_HIT_INTERVAL_MS));
				PlayerWeaponFire(p);
			}
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

	foreach(it, players) {
		if(it->pendingShot && TimeDiffSec(TimeDiff(it->pendingShotTime, localTime)) > 0.15) {
			it->pendingShot = false;
			const vec2 shotDir = glm::length(it->pendingShotDir) < 0.1f ? vec2(cosf(it->weaponAimYaw), sinf(it->weaponAimYaw)) : it->pendingShotDir;
			PlayerWeaponShot(*it, it->pendingShotOrigin, shotDir, it->pendingShotDistance);
		}
		if(it->pendingAim && TimeDiffSec(TimeDiff(it->pendingAimTime, localTime)) > 0.15) {
			it->pendingAim = false;
		}
	}

	UpdateProjectiles((f32)tdelta);
	UpdateRemotes((f32)tdelta);
	UpdateUltimateGauges((f32)tdelta);
	UpdateStatuses();
	UpdateToggles();
	UpdateSummons();
	UpdateSummonAi((f32)tdelta);
	if(!frozen) UpdatePassives();
	if(!frozen) UpdateStances();
	if(!frozen) UpdateCasts();
	if(!frozen) UpdateInteractions();
	if(!frozen) UpdateBotCombat();
	if(!frozen) UpdateBotSkills();

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

	foreach(it, players) {
		if(it->clientHd != ClientHandle::INVALID || it->dead || it->posHistoryCount == 0) continue;
		const f32 expected = it->movement.moveSpeed * (f32)tdelta;
		if(expected < 1.0f) continue;
		const vec3 prev = it->posHistory[(it->posHistoryHead + it->posHistory.size() - 1) % it->posHistory.size()];
		const f32 moved = glm::length(vec2(it->body->GetWorldPos() - prev));
		if(moved < expected * 0.3f) {
			it->input.moveTo = it->body->GetWorldPos();
		}
	}

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
			rch.skillMoving = localTime < player.dashUntil;

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
		rfl.ownerUID = actor.ownerUID;


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
	player.body = physics.CreateDynamicBody(110, 70, pos); // radius 100 is found in files but 110 (_AILength) matches better
	player.spawnPos = pos;

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
	main.hpMax = (i32)MasterStat(main.classType, STAT_HEALTH);
	main.hp = main.hpMax;
	main.seed = (i32)RandUint();




	sub.parent = &player;
	sub.classType = player.subClass;
	sub.skinIndex = player.subSkin;
	sub.hpMax = (i32)MasterStat(sub.classType, STAT_HEALTH);
	sub.hp = sub.hpMax;
	ASSERT(main.hpMax > 0 && sub.hpMax > 0);
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

void World::PlayerCastSkill(Player& player, SkillID skillIn, const vec3& castPosIn, Slice<const ActorUID> targets, f32 clientTime, ActionStateID forcedAction)
{
	const SkillID skillID = ResolveComboRecast(player, skillIn, forcedAction);
	const vec3 castPos = ResolveCastPos(player, skillID, castPosIn, forcedAction);
	if(frozen) return;
	if(!SkillTargetsAllowed(player, skillID, targets)) return;
	eastl::fixed_vector<ActorUID,10,false> allEnemyTargets;
	if(const GameXmlContent::SkillInput* selIn = GetGameXmlContent().FindSkillInput((i32)skillID)) {
		if(selIn->allEnemies) {
			foreach_const(e, players) if(e->team != player.team && !e->dead) allEnemyTargets.push_back(e->Main().UID);
			targets = Slice<const ActorUID>(allEnemyTargets.data(), allEnemyTargets.size());
		}
	}
	// TODO: check if can cast

	// access method is kinda convoluted
	const auto& content = GetGameXmlContent();
	auto skillFound = content.skillMap.find(skillID);
	if(skillFound == content.skillMap.end()) {
		WARN("PlayerCastSkill :: unknown skill %d", (i32)skillID);
		return;
	}
	const auto& skill = skillFound->second;
	const bool followUp = forcedAction != ActionStateID::INVALID;
	if(!followUp && content.FindSkillToggle((i32)skillID)) {
		if(player.toggleSkill == skillID) {
			ToggleOff(player);
			return;
		}
		ToggleOn(player, skillID, castPos);
	}
	if(!followUp && player.casting.skillID == skillID) {
		ReleaseCast(player);
		return;
	}
	const GameXmlContent::SkillCast* cast = followUp ? nullptr : content.FindSkillCast((i32)skillID);
	castCostLevel = followUp ? nullptr : PickCostLevel(player, skillID);
	const ActionStateID actionState = followUp ? forcedAction : castCostLevel && castCostLevel->action != ActionStateID::INVALID ? castCostLevel->action : (cast ? cast->action : skill.action);

	if(!followUp && !CheckAndPayCast(player, skillID, castPos, targets)) return;

	f32 angle = player.input.rot.upperYaw;
	if(skill.type == SkillType::SHIRK) {
		const vec2 moving = vec2(player.input.moveTo) - vec2(player.body->GetWorldPos());
		if(glm::length(moving) > 10.0f) angle = atan2f(moving.y, moving.x);
	}
	const vec2 dir = vec2(cosf(angle), sinf(angle));

	Replication::SkillCast rpCast;
	rpCast.clientHd = player.clientHd;
	rpCast.casterUID = player.Main().UID;
	rpCast.skillID = skillID;
	rpCast.castPos = castPos;
	rpCast.actionID = actionState;
	rpCast.sendAction = cast != nullptr;

	rpCast.casterPos = player.body->GetWorldPos();
	rpCast.casterMoveDir = dir;
	rpCast.casterRot = { angle, 0, angle };
	rpCast.casterSpeed = player.input.speed; // FIXME: should not come from input
	rpCast.clientTime = clientTime;

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
	prog.stance = player.Main().stance;
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
	vec2 gotoDir = vec2(0);

	const auto& action = content.GetSkillAction(player.Main().classType, actionState, player.Main().stance);

	foreach_const(cmd, action.commands) {
		switch(cmd->type) {
			case ActionCommand::Type::GRAPH_MOVE_HORZ: {
				distance = cmd->graphMoveHorz.distance;
				moveDuration = SkillMoveDuration(content, player.Main().classType, action, cmd->animIndex);
			} break;

			case ActionCommand::Type::MOVE: {
				switch(cmd->move.preset) {
					case ActionCommand::MovePreset::WARP: {
						distance = (f32)cmd->move.param2;
						moveDuration = 0.01f; // warping
					} break;
					case ActionCommand::MovePreset::GOTO_CURRENT_TARGET:
					case ActionCommand::MovePreset::GOTO_CURRENT_TARGET_CHASE:
					case ActionCommand::MovePreset::GOTO_CURRENT_TARGET_THROUGH:
					case ActionCommand::MovePreset::GOTO_CURRENT_TARGET_THROUGH_CHASE: {
						const ActorMaster* tm = targets.size() > 0 ? FindMasterActor(targets[0]) : nullptr;
						if(tm && distance == 0) {
							const vec2 rel = vec2(tm->parent->body->GetWorldPos()) - vec2(player.body->GetWorldPos());
							const f32 len = glm::length(rel);
							if(len > 1.0f) {
								const bool through = cmd->move.preset == ActionCommand::MovePreset::GOTO_CURRENT_TARGET_THROUGH || cmd->move.preset == ActionCommand::MovePreset::GOTO_CURRENT_TARGET_THROUGH_CHASE;
								gotoDir = rel / len;
								distance = through ? len : MAX(len - HitRadius(*tm->parent) - HitRadius(player), 0.0f);
								moveDuration = action.seqLength;
							}
						}
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
	rpExec.casterClientHd = player.clientHd;
	rpExec.castStart = cast != nullptr;
	rpExec.startPos = player.body->GetWorldPos();
	rpExec.moveDir = dir;
	rpExec.rot = { angle, 0, angle };
	rpExec.speed = player.input.speed; // FIXME: should not come from input

	if(distance > 0 && glm::length(gotoDir) < 0.5f && glm::length(vec2(castPos)) > 1.0f) TargetPositionMove(player, action, castPos, &distance, &gotoDir);
	if(distance != 0) {
		const vec2 moveDir = glm::length(gotoDir) > 0.5f ? gotoDir : dir;
		const vec3 endPos = physics.FindMovePos(player.body, vec3(moveDir * distance, 0), moveDuration);
		rpExec.moveDir = moveDir;
		rpExec.endPos = endPos;
	}

	replication->FramePushSkillExec(rpExec);

	const bool castRelease = followUp && forcedAction == skill.action;
	if(castRelease && player.lastComboSkill == skillID) player.lastComboSkill = SkillID::INVALID;
	ScheduleSkillRemotes(player, skillID, castPos, targets, angle, followUp && !castRelease ? 0 : -1, cast ? cast->action : forcedAction);
	castCostLevel = nullptr;
	if(cast) StartCasting(player, skillID, *cast, castPos, targets, action);
	skillProgramList.back().passes = lastAnimPasses;

	if(!followUp && player.clientHd != ClientHandle::INVALID) UpdateComboSlot(player, skillID, skill.type, castPos);
	if(!followUp) {
		if(const GameXmlContent::SkillStance* st = content.FindSkillStance((i32)skillID)) SetStance(player.Main(), st->stance, st->duration, st->weaponFireCount, st->cancelByDefeat);
	}

	QueueChildSkill(player, actionState, castPos, targets);
}

SkillID World::ResolveComboRecast(Player& player, SkillID skillIn, ActionStateID forcedAction)
{
	if(player.clientHd != ClientHandle::INVALID && forcedAction == ActionStateID::INVALID && player.comboSlot >= 0 && skillIn == player.comboBase && localTime < player.comboUntil && player.comboSkill != SkillID::INVALID) {
		LOG("Combo :: %ls recast %d -> combo %d", player.name.data(), (i32)skillIn, (i32)player.comboSkill);
		return player.comboSkill;
	}
	return skillIn;
}

vec3 World::ResolveCastPos(const Player& player, SkillID skillID, const vec3& castPosIn, ActionStateID forcedAction) const
{
	vec3 castPos = castPosIn;
	if(player.clientHd != ClientHandle::INVALID && forcedAction == ActionStateID::INVALID && player.comboSlot >= 0 && skillID == player.comboSkill && glm::length(vec2(player.comboCastPos)) > 1.0f) {
		const GameXmlContent::SkillInput* comboIn = GetGameXmlContent().FindSkillInput((i32)skillID);
		if(comboIn && comboIn->selfPosition) castPos = player.comboCastPos;
	}
	if(player.clientHd == ClientHandle::INVALID && forcedAction == ActionStateID::INVALID && glm::length(vec2(castPos)) > 1.0f) {
		const GameXmlContent::SkillInput* in = GetGameXmlContent().FindSkillInput((i32)skillID);
		const vec2 from = vec2(player.body->GetWorldPos());
		const vec2 rel = vec2(castPos) - from;
		const f32 len = glm::length(rel);
		if(in && len > 1.0f) {
			f32 clamped = len;
			if(in->minDistance > 0) clamped = MAX(clamped, in->minDistance);
			if(in->maxDistance > 0) clamped = MIN(clamped, in->maxDistance);
			if(clamped != len) castPos = vec3(from + rel / len * clamped, castPos.z);
		}
	}
	return castPos;
}

bool World::SkillTargetsAllowed(const Player& player, SkillID skillID, Slice<const ActorUID> targets) const
{
	if(player.Main().classType == ClassType::SLIME && targets.size() > 0) {
		const GameXmlContent::SkillInput* pickIn = GetGameXmlContent().FindSkillInput((i32)skillID);
		const ActorMaster* picked = pickIn && pickIn->pickedEntity ? FindMasterActor(targets[0]) : nullptr;
		if(picked && picked->parent != &player && picked->classType == ClassType::SLIME) {
			LOG("Skill :: %ls skill=%d refused: cannot cling to another Bumaro (ERROR_TYPE_SKILL_CANT_USE_SAME_MASTER)", player.name.data(), (i32)skillID);
			return false;
		}
	}
	return true;
}

const GameXmlContent::CostLevel* World::PickCostLevel(const Player& player, SkillID skillID) const
{
	const GameXmlContent::CostLevel* picked = nullptr;
	if(const auto* levels = GetGameXmlContent().FindSkillCostLevels((i32)skillID)) {
		for(const GameXmlContent::CostLevel& lvl : *levels) {
			bool affordable = lvl.consume[0] > 0 || lvl.consume[1] > 0 || lvl.consume[2] > 0;
			for(i32 r = 0; r < 3; r++) if(lvl.consume[r] > 0 && player.res[r] < lvl.consume[r]) affordable = false;
			if(affordable) picked = &lvl;
		}
	}
	return picked;
}

bool World::CheckAndPayCast(Player& player, SkillID skillID, const vec3& castPos, Slice<const ActorUID> targets)
{
	const auto& content = GetGameXmlContent();
	const bool shirk = GetGameXmlContent().GetSkillConsumeStamina((i32)skillID) > 0;
	if(IsBlocked(player, false, !shirk, shirk)) {
		LOG("Blocked :: %ls skill=%d (input blocked by a status)", player.name.data(), (i32)skillID);
		return false;
	}
	if(player.clientHd == ClientHandle::INVALID) {
		const GameXmlContent::SkillInput* obIn = content.FindSkillInput((i32)skillID);
		if(obIn && (obIn->obstructMove != 0 || obIn->obstructSight)) {
			vec3 to = castPos;
			if(targets.size() > 0) { if(const ActorMaster* tm = FindMasterActor(targets[0])) to = tm->parent->body->GetWorldPos(); }
			const vec3 from = player.body->GetWorldPos();
			if(glm::length(vec2(to)) > 1.0f && glm::length(vec2(to) - vec2(from)) > 1.0f) {
				const f32 height = obIn->obstructMove == 1 ? OBSTRUCT_JUMPABLE_HEIGHT : OBSTRUCT_LOW_HEIGHT;
				const vec2 along = glm::normalize(vec2(to) - vec2(from));
				const vec2 side = vec2(-along.y, along.x);
				bool blocked = false;
				for(f32 off : { 0.0f, -OBSTRUCT_SIDE, OBSTRUCT_SIDE }) {
					f32 hitDist = 0;
					const bool hit = physics.RaycastStatic(vec3(vec2(from) + side * off, from.z + height), vec3(vec2(to) + side * off, from.z + height), &hitDist);
					if(hit) blocked = true;
				}
				if(blocked) {
					LOG("Blocked :: %ls skill=%d obstacle between caster and target", player.name.data(), (i32)skillID);
					return false;
				}
			}
		}
	}
	if(!PlayerPaySkillCost(player, (i32)skillID)) return false;
	if(castCostLevel) {
		for(i32 r = 0; r < 3; r++) {
			if(castCostLevel->consume[r] > 0 && player.res[r] >= 0) { player.res[r] -= castCostLevel->consume[r]; player.tNextResSend = Time::ZERO; }
		}
		LOG("Cost :: %ls skill=%d empowered level (consumed rp=%.0f bb=%.0f, %d remotes / %d statuses replaced, damage x%.3f)", player.name.data(), (i32)skillID,
			castCostLevel->consume[1], castCostLevel->consume[2], (i32)castCostLevel->remoteReplace.size(), (i32)castCostLevel->statusReplace.size(), castCostLevel->damageRatio);
	}
	PassiveOnSkillCast(player, (i32)skillID);
	return true;
}

void World::TargetPositionMove(const Player& player, const GameXmlContent::Action& action, const vec3& castPos, f32* distance, vec2* gotoDir) const
{
	foreach_const(cmd, action.commands) {
		if(cmd->type == ActionCommand::Type::MOVE && (cmd->move.preset == ActionCommand::MovePreset::TARGET_POS || cmd->move.preset == ActionCommand::MovePreset::TARGET_POS_LEAP)) {
			const vec2 rel = vec2(castPos) - vec2(player.body->GetWorldPos());
			const f32 len = glm::length(rel);
			*distance = len;
			if(len > 1.0f) *gotoDir = rel / len;
		}
	}
}

void World::StartCasting(Player& player, SkillID skillID, const GameXmlContent::SkillCast& cast, const vec3& castPos, Slice<const ActorUID> targets, const GameXmlContent::Action& action)
{
	const bool bot = player.clientHd == ClientHandle::INVALID;
	f32 wait = cast.time;
	if(bot && cast.type != GameXmlContent::SkillCast::DELAY) wait = MIN(wait, action.seqLength);
	player.casting.skillID = skillID;
	player.casting.castPos = castPos;
	player.casting.target = targets.size() > 0 ? targets[0] : ActorUID::INVALID;
	player.casting.startPos = vec2(player.body->GetWorldPos());
	player.casting.executeAt = TimeAdd(localTime, TimeMsToTime(wait * 1000.0));
	player.casting.type = (u8)cast.type;
	player.casting.interruptByMove = cast.interruptByMove;
	LOG("Cast :: %ls skill=%d start action=%d type=%d wait=%.2f", player.name.data(), (i32)skillID, (i32)cast.action, (i32)cast.type, wait);
}

void World::UpdateComboSlot(Player& player, SkillID skillID, SkillType skillType, const vec3& castPos)
{
	const auto& content = GetGameXmlContent();
	if(player.comboSlot >= 0 && skillType == SkillType::COMBO) {
		replication->SendChangeSkillSlot(player.clientHd, player.Main().UID, player.comboSlot, player.comboBase, 0);
		ComboBaseCooldown(player);
		LOG("Combo :: %ls slot %d back to %d", player.name.data(), player.comboSlot, (i32)player.comboBase);
		player.comboSlot = -1;
	}
	const i32 combo = content.GetFirstComboSkill((i32)skillID);
	if(combo != 0) {
		const i32 base = (i32)player.mainCharaID * 2;
		i32 slot = -1;
		const i32 order = ((i32)skillID / 10) % 10;
		if((player.skills[base] == skillID || player.skills[base + 1] == skillID) && order >= 1 && order <= 4) slot = SKILL_SLOT_ENUM_FIRST + order - 1;
		else if(order == 5 && (i32)skillID % 10 == 0) slot = SKILL_SLOT_ENUM_UG;
		if(slot >= 0) {
			const GameXmlContent::SkillInput* baseIn = content.FindSkillInput((i32)skillID);
			player.comboPending = SkillID(combo);
			player.comboSkill = SkillID(combo);
			player.comboSendAt = TimeAdd(localTime, TimeMsToTime(50.0));
			player.comboResetCooltime = baseIn ? baseIn->coolTimeID : 0;
			player.comboSlot = slot;
			player.comboBase = skillID;
			player.comboCastPos = castPos;
			const i32 windowMs = content.GetComboDurationMs(combo);
			ASSERT(windowMs > 0);
			player.comboUntil = TimeAdd(localTime, TimeMsToTime((f64)windowMs));
			LOG("Combo :: %ls slot %d -> %d", player.name.data(), slot, combo);
			const i32 autoMs = content.GetComboAutoCastMs(combo);
			player.comboAutoAt = autoMs > 0 ? TimeAdd(localTime, TimeMsToTime((f64)autoMs)) : Time::ZERO;
		}
	}
}

void World::QueueChildSkill(Player& player, ActionStateID actionState, const vec3& castPos, Slice<const ActorUID> targets)
{
	const auto& content = GetGameXmlContent();
	const i32 child = content.GetChildSkill(player.Main().classType, actionState);
	if(child > 0 && content.IsChildOnExisting(player.Main().classType, actionState)) {
		ChildSkillOnExisting(player, child, castPos);
	}
	else if(child > 0) {
		player.pendingChildSkill = child;
		player.pendingChildSkillUntil = TimeAdd(localTime, TimeMsToTime(2000));
		player.pendingChildPos = castPos;
		player.pendingChildTarget = targets.size() > 0 ? targets[0] : ActorUID::INVALID;
	}
}

void World::TeleportPlayer(Player& p, const vec3& pos, f32 yaw)
{
	p.body->collider->setFootPosition(PxExtendedVec3(pos.x, pos.y, pos.z));
	p.body->vel = vec3(0);
	p.posHistoryCount = 0;
	p.posHistoryHead = 0;
	p.dashDone = p.dashDist;
	p.dashUntil = Time::ZERO;
	p.input.moveTo = pos;
	p.input.rot.upperYaw = yaw;
	p.input.rot.bodyYaw = yaw;
	p.weaponAimYaw = yaw;
}

void World::MovePlayer(Player& p, const vec3& pos, f32 yaw, bool snap)
{
	TeleportPlayer(p, pos, yaw);
	if(p.clientHd == ClientHandle::INVALID) return;
	replication->SendSelfTeleport(p.clientHd, p.Main().UID, pos, yaw, snap ? &p.lastClientPos : nullptr);
}

void World::RespawnPlayer(Player& player)
{
	player.dead = false;
	player.mana = -1.0f;
	player.benchMana = -1.0f;
	player.stamina = -1.0f;
	player.res = { -1, -1, -1 };
	player.benchRes = { -1, -1, -1 };
	player.resSent = { -1, -1, -1 };
	player.tNextManaSend = Time::ZERO;
	player.tNextStaminaSend = Time::ZERO;
	foreach(c, player.characters) {
		(*c)->hp = (*c)->hpMax;
	}

	player.body->collider->setFootPosition(PxExtendedVec3(player.spawnPos.x, player.spawnPos.y, player.spawnPos.z));
	player.body->vel = vec3(0);
	player.input.moveTo = player.spawnPos;

	LOG("Respawn :: %ls", player.name.data());
	replication->SendReviveToAll(player.userID, player.Main().UID, player.Sub().UID, player.spawnPos);
	foreach(c, player.characters) {
		replication->SendHpToAll((*c)->UID, (*c)->hp, (*c)->hpMax);
	}
	if(player.passivesApplied) ApplyPassives(player);
}

void World::ExecuteSkillProgram(SkillProgram& prog)
{
	ActorMaster* caster = FindMasterActor(prog.casterUID);
	if(!caster) {
		prog.Finish();
		return;
	}
	const vec2 dir = vec2(cosf(prog.castAngle), sinf(prog.castAngle));
	const auto& action = GetGameXmlContent().GetSkillAction(caster->classType, prog.actionID, prog.stance);
	const f32 elapsed = (f32)TimeDiffSec(TimeDiff(prog.startTime, localTime));
	bool pending = false;
	for(i32 i = 0; i < (i32)action.commands.size() && i < 64; i++) {
		if(prog.done & (1ull << i)) continue;
		const auto& cmd = action.commands[i];
		f32 start = -1.0f;
		for(const SkillAnimPass& pass : prog.passes) {
			if(pass.index == cmd.animIndex || (cmd.animIndex == -1 && start < 0)) start = pass.start;
		}
		if(prog.passes.empty() && (cmd.animIndex == 0 || cmd.animIndex == -1)) start = 0;
		if(start < 0) {
			prog.done |= 1ull << i;
			continue;
		}
		if(elapsed < start + (cmd.type == ActionCommand::Type::STATE_BLOCK ? 0.0f : cmd.delay)) {
			pending = true;
			continue;
		}
		prog.done |= 1ull << i;
		RunSkillProgramCommand(prog, caster, i, dir);
	}
	if(!pending) prog.Finish();
}
