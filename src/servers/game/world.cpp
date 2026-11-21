#include "config.h"
#include "world.h"
#include <mxm/game_content.h>
#include "world_common.h"

static const i32 STAT_HEALTH = 0;
static const f64 REMOVED_PLAYER_RESPAWN_MS = 1.0e12;
static const i32 STATUS_PC_RESPAWN_INVICIBLE_3S = 1230000010;
static const i32 STATUS_PC_TAG_GODMODE = 1230000020;
static const f64 MEASURED_TAG_FIRST_ATTACK_MS = 110.0;
static const f64 MEASURED_GRAPH_END_SYNC_DELAY_MS = 190.0;
static const f64 MEASURED_UNSYNCED_GRAPH_MAX_SEC = 0.5;
static const f64 WEAPON_HIT_INTERVAL_MS = 80.0;
static const i32 MEASURED_COMBO_SLOT_FIRST = 0;
static const f32 OBSTRUCT_LOW_HEIGHT = 30.0f;
static const i32 MEASURED_COMBO_SLOT_UG = 2;
static const i32 MEASURED_COMBO_SLOT_SHIRK = 3;

static void AddTitleBonus(World::ActorMaster& chara, i32 title, WeaponIndex weapon)
{
	const auto* stats = GetGameXmlContent().FindTitleStats(title);
	if(!stats) return;
	for(const auto& t : *stats) {
		const f32 base = t.type == STAT_ATTACK ? GetGameXmlContent().GetWeaponAttack((i32)weapon) : MasterStat(chara.classType, t.type);
		const f32 value = t.mul ? base * t.value / 100.0f : t.value;
		bool merged = false;
		foreach(g, chara.gearBonus) if(g->first == t.type) { g->second += value; merged = true; }
		if(!merged && !chara.gearBonus.full()) chara.gearBonus.push_back(eastl::make_pair(t.type, value));
	}
}

static void ComputeGearBonus(World::ActorMaster& chara, const eastl::fixed_vector<i32,6,false>& docs, WeaponIndex weapon, i32 powerPercent)
{
	chara.gearBonus.clear();
	auto add = [&chara](u8 type, f32 value) {
		foreach(g, chara.gearBonus) if(g->first == type) { g->second += value; return; }
		if(!chara.gearBonus.full()) chara.gearBonus.push_back(eastl::make_pair(type, value));
	};
	foreach_const(d, docs) {
		const GameXmlContent::GearItem* gear = GetGameXmlContent().FindGearItem(*d);
		if(!gear) continue;
		foreach_const(m, gear->statMul) {
			const f32 base = m->type == STAT_ATTACK ? GetGameXmlContent().GetWeaponAttack((i32)weapon) : MasterStat(chara.classType, m->type);
			add(m->type, base * m->value / 100.0f * (f32)powerPercent / 100.0f);
		}
		foreach_const(a, gear->statAdd) add(a->type, a->value * (f32)powerPercent / 100.0f);
	}
}

static void AddMeasuredBotGear(World::ActorMaster& chara, WeaponIndex weapon, i32 powerPercent, const GameXmlContent::MeasuredBotGear* gear)
{
	if(!gear || powerPercent <= 0) return;
	const f32 k = (f32)powerPercent / 100.0f;
	const i32 doc = (i32)weapon;
	const f32 tierAttack = GetGameXmlContent().GetWeaponAttack(doc - doc % 10 + gear->weaponTier);
	const f32 weaponAttack = GetGameXmlContent().GetWeaponAttack(doc);
	const f32 tierBonus = doc % 10 == 1 && tierAttack > weaponAttack ? tierAttack - weaponAttack : 0.0f;
	const eastl::pair<u8,f32> adds[4] = {
		eastl::make_pair((u8)STAT_ATTACK, (weaponAttack + tierBonus) * gear->attack * k + tierBonus),
		eastl::make_pair((u8)StatType::STAT_TYPE_DEFENCE, MasterStat(chara.classType, (i32)StatType::STAT_TYPE_DEFENCE) * gear->defence * k),
		eastl::make_pair((u8)STAT_HEALTH, gear->health * k),
		eastl::make_pair((u8)StatType::STAT_TYPE_CRITICALRATE, gear->critical * k),
	};
	for(const auto& a : adds) {
		if(a.second <= 0) continue;
		bool merged = false;
		foreach(g, chara.gearBonus) if(g->first == a.first) { g->second += a.second; merged = true; break; }
		if(!merged && !chara.gearBonus.full()) chara.gearBonus.push_back(a);
	}
}

static void BuildInitStats(World::ActorMaster& chara, WeaponIndex weapon)
{
	chara.initStats.clear();
	bool hasAttack = false;
	foreach_const(st, GetGameXmlContent().GetMaster(chara.classType).baseStats) {
		if(chara.initStats.full()) break;
		const f32 value = st->type == (u8)StatType::STAT_TYPE_HEALTH ? (f32)chara.hpMax : st->value + chara.GearBonus(st->type);
		hasAttack |= st->type == (u8)StatType::STAT_TYPE_ATTACK;
		chara.initStats.push_back({ st->type, value });
	}
	const f32 attack = GetGameXmlContent().GetWeaponAttack((i32)weapon);
	if(!hasAttack && attack > 0 && !chara.initStats.full()) chara.initStats.push_back({ (u8)StatType::STAT_TYPE_ATTACK, attack + chara.GearBonus((u8)StatType::STAT_TYPE_ATTACK) });
}

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
			if(p.dashFly && (p.dashCurve ? fabsf(ahead) > 0.01f : ahead > 0)) {
				const vec3 at = body.GetWorldPos() + vec3(p.dashDir * ahead, 0);
				body.collider->setFootPosition(PxExtendedVec3(at.x, at.y, at.z));
			}
			else if(p.dashCurve ? fabsf(ahead) > 0.01f : ahead > 0) physics.Move(&body, vec3(p.dashDir * ahead, 0), (f32)tdelta);
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

		if(p.body) p.body->passWallTeam = !p.dead && p.clientHd == ClientHandle::INVALID && localTime < p.pushUntil && p.pushPassWallTeam >= 0 ? p.pushPassWallTeam : -1;
		if(!p.dead && p.clientHd == ClientHandle::INVALID && localTime < p.ccUntil) {
			const ActiveStatus* fear = nullptr;
			foreach_const(st, activeStatuses) {
				if(st->targetIndex == p.index && st->info && localTime < st->until && st->info->type == StatusType::FEAR) { fear = &*st; break; }
			}
			if(localTime < p.pushUntil) {
				p.input.moveTo = p.pushTarget;
				p.input.speed = p.pushSpeed;
				const f32 total = (f32)TimeDiffSec(TimeDiff(p.pushStart, p.pushUntil));
				if(p.pushSilent && total > 0) {
					const f32 next = PushCurveProgress(((f32)TimeDiffSec(TimeDiff(p.pushStart, localTime)) + (f32)UPDATE_RATE) / total);
					const vec2 want = vec2(p.pushFrom) + (vec2(p.pushTarget) - vec2(p.pushFrom)) * next;
					const vec2 line = vec2(p.pushTarget) - vec2(p.pushFrom);
					const f32 lineLen = glm::length(line);
					const f32 step = lineLen > 1.0f ? glm::dot(want - vec2(body.GetWorldPos()), line / lineLen) : 0.0f;
					p.input.speed = MAX(step, 0.0f) / (f32)UPDATE_RATE;
				}
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
		if(!p.dead && p.clientHd != ClientHandle::INVALID && p.clientPushSnap && localTime < p.clientPushUntil && p.clientPosCount == p.clientPushPosCount) {
			const f32 total = (f32)TimeDiffSec(TimeDiff(p.clientPushStart, p.clientPushUntil));
			const f32 k = total > 0.001f ? MIN((f32)TimeDiffSec(TimeDiff(p.clientPushStart, localTime)) / total, 1.0f) : 1.0f;
			f32 progress = PushCurveProgress(k);
			if(p.clientPushDragMax > 0) {
				const f32 len = glm::length(vec2(p.pushTarget) - vec2(p.clientPushFrom));
				const f32 t = (f32)TimeDiffSec(TimeDiff(p.clientPushStart, localTime));
				const f32 accel = total > 0.001f ? (p.clientPushDragMax - p.clientPushDragMin) / total : 0.0f;
				progress = len > 1.0f ? MIN((p.clientPushDragMin * t + 0.5f * accel * t * t) / len, 1.0f) : 1.0f;
			}
			const vec2 at2 = glm::mix(vec2(p.clientPushFrom), vec2(p.pushTarget), progress);
			const vec3 at = vec3(at2, body.GetWorldPos().z);
			body.collider->setFootPosition(PxExtendedVec3(at.x, at.y, at.z));
			body.vel = vec3(0);
			p.input.moveTo = at;
		}
		if(!p.dead && p.clientHd != ClientHandle::INVALID && p.clientPushSnap && localTime >= p.clientPushUntil) {
			p.clientPushSnap = false;
			if(p.clientPosCount == p.clientPushPosCount) {
				const vec3 at = vec3(vec2(p.pushTarget), body.GetWorldPos().z);
				body.collider->setFootPosition(PxExtendedVec3(at.x, at.y, at.z));
				body.vel = vec3(0);
				p.input.moveTo = at;
				LOG("Push :: %ls silent client, body set to push end (%.0f, %.0f)", p.name.data(), at.x, at.y);
			}
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
		if(p.input.tag && p.clientHd == ClientHandle::INVALID) {
			if(p.casting.skillID != SkillID::INVALID || localTime < p.dashUntil) p.input.tag = 0;
			foreach_const(pr, pendingRemotes) {
				if(!pr->remove && pr->ownerIndex == p.index) { p.input.tag = 0; break; }
			}
		}
		if(p.input.tag) {
			p.input.tag = 0;
			if(p.casting.skillID != SkillID::INVALID) CancelCast(p, "tag");
			if(p.comboSlot >= 0) {
				replication->SendChangeSkillSlot(p.clientHd, p.Main().UID, p.comboSlot, p.comboBase, 0);
				LOG("Combo :: %ls slot %d cleared (tag)", p.name.data(), p.comboSlot);
				p.comboSlot = -1;
			}
			p.comboPending = SkillID::INVALID;
			p.comboSkill = SkillID::INVALID;
			p.comboAutoAt = Time::ZERO;
			p.lastComboSkill = SkillID::INVALID;
			p.pendingChildSkill = 0;
			SwapMainMaster(p);
			if(!p.dead) {
				statusNoCaster = true;
				ApplyStatus(p, p, STATUS_PC_TAG_GODMODE);
				statusNoCaster = false;
			}
			SendTagCooltimeToTeam(p, TagCooltimeMs(p));
			if(p.clientHd == ClientHandle::INVALID) {
				const Time ready = TimeAdd(localTime, TimeMsToTime(MEASURED_TAG_FIRST_ATTACK_MS));
				if(p.tNextBotShot < ready) p.tNextBotShot = ready;
			}
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
			if(p.clientHd != ClientHandle::INVALID && p.casting.skillID != SkillID::INVALID && p.casting.type == GameXmlContent::SkillCast::LIMIT) {
				if(p.lastCastStart != localTime) ReleaseCast(p);
				else LOG("Cast :: %ls weapon fire sent before skill=%d, not a release", p.name.data(), (i32)p.casting.skillID);
			}
			else if(WeaponIsMelee(p) && localTime > p.tNextWeaponHit && !(p.clientHd == ClientHandle::INVALID && (p.lastCastStart == localTime || localTime < p.dashUntil || p.casting.skillID != SkillID::INVALID))) {
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
	UpdateHideVisibility();
	UpdateFog();
	UpdateMasterGroupings();
	UpdateNpcStatuses();
	UpdateLevelEvents();
	UpdateToggles();
	UpdateChases();
	UpdateBattleStates();
	UpdateWeaponCharges();
	UpdateSkillPoints();
	UpdateSummons();
	UpdateSummonAi((f32)tdelta);
	UpdateMonsterAi((f32)tdelta);
	UpdateDynamicTraps();
	UpdateDynamicRespawns();
	UpdateDefence();
	UpdateIngameLevels();
	if(!frozen) UpdatePassives();
	if(!frozen) UpdateStances();
	if(!frozen) UpdatePushBounds();
	if(!frozen) UpdateCasts();
	if(!frozen) UpdateInteractions();
	BuildTeamGates();
	if(!frozen && !combatLocked) UpdateBotMovement();
	if(!frozen && !combatLocked) UpdateBotCombat();
	if(!frozen && !combatLocked) UpdateBotSkills();

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
		if(!it->body || it->dead) continue;
		if(it->body->grounded) {
			it->lastGroundPos = it->body->GetWorldPos();
			it->hasGroundPos = true;
		}
		else if(it->hasGroundPos && it->body->GetWorldPos().z < killZ) {
			LOG("Fall :: %ls below KillZ %.0f, back to (%.0f, %.0f, %.0f)", it->name.data(), killZ, it->lastGroundPos.x, it->lastGroundPos.y, it->lastGroundPos.z);
			TeleportPlayer(*it, it->lastGroundPos, it->input.rot.bodyYaw);
		}
	}

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

	UpdateStatuses();
	Replicate();
}

void World::Replicate()
{
	eastl::fixed_vector<Replication::ActorMaster, 64, false> repMasterList;

	// players
	foreach_const(it, players) {
		const Player& player = *it;
		if(player.removed) continue;

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
		rep.stageSkills = player.stageSkills;
		rep.skillLevels.clear();
		foreach_const(l, player.skillLevels) if(!rep.skillLevels.full()) rep.skillLevels.push_back(eastl::make_pair(l->first, l->second));
		rep.supportKit = player.supportKit;
		rep.badgeType = player.badgeType;
		rep.badgeTier = player.badgeTier;
		rep.displayTitle = player.displayTitle;
		rep.statTitle = player.statTitle;
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
			rch.initStats = chara.initStats.data();
			rch.initStatCount = (u16)chara.initStats.size();
			rch.spawnAnim = chara.spawnAnim;
			rch.ownerID = chara.ownerID;
			rch.dirToNearPC = chara.dirToNearPC;
			rch.wanderDist = chara.wanderDist;
			rch.tagID = chara.tagID;




			rch.pos = player.body->GetWorldPos();
			rch.moveDir = player.movement.moveDir;
			rch.speed = player.input.speed;
			rch.rotation = player.movement.rot;
			const bool weaponLunge = player.weaponLungeUntil != Time::ZERO && player.weaponLungeUntil == player.dashUntil;
			rch.skillMoving = (localTime < player.dashUntil && !weaponLunge) || AttachedToCaster(player);
			rch.graphMoving = (localTime < player.dashUntil && !weaponLunge && !AttachedToCaster(player)) || localTime < player.pushUntil || player.dragElapsed < player.dragDuration;
			rch.statusGraph = localTime < player.pushUntil && player.pushSilent;
			const Time holdEnd = TimeDiffSec(TimeDiff(player.dashStartAt, player.dashUntil)) <= MEASURED_UNSYNCED_GRAPH_MAX_SEC || player.dashBlockUntil >= player.dashUntil ? TimeAdd(player.dashUntil, TimeMsToTime(MEASURED_GRAPH_END_SYNC_DELAY_MS)) : player.dashBlockUntil;
			rch.graphHold = player.clientHd == ClientHandle::INVALID && player.dashUntil != Time::ZERO && localTime < holdEnd && !AttachedToCaster(player) && !weaponLunge;

			const bool idle = chara.actionState == ActionStateID::INVALID || chara.actionState == ActionStateID::NONE_BEHAVIORSTATE;
			const bool walking = player.movement.moveSpeed > 0 && localTime >= player.dashUntil;
			const bool holdState = localTime < player.botAttackStateUntil && !(player.botAttackStateSkill && walking);
			rch.actionState = idle && player.clientHd == ClientHandle::INVALID && holdState ? player.botAttackState : chara.actionState;
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
			rch.initStats = chara.initStats.data();
			rch.initStatCount = (u16)chara.initStats.size();
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
		rfl.createRot = actor.createRot;
		rfl.hasCreateRot = actor.hasCreateRot;
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
		rfl.ownerSkillPower = actor.ownerClass >= 0 ? MasterStat((ClassType)actor.ownerClass, STAT_SKILLPOWER) : 0.0f;
		rfl.hp = actor.hp;
		rfl.growthLevel = actor.growthLevel;
		rfl.statScale = actor.statScale;
		rfl.stateSpeed = actor.monsterAi ? actor.monsterAi->run * actor.speedMul : 0.0f;
		if(actor.dashEnd != Time::ZERO && actor.dashCurve == NPC_DASH_CURVE_LINEAR) {
			const vec2 goal = actor.dashFrom + actor.dashDir * actor.dashDist;
			const f32 left = (f32)TimeDiffSec(TimeDiff(localTime > actor.dashStart ? localTime : actor.dashStart, actor.dashEnd));
			const f32 togo = glm::length(goal - vec2(actor.pos));
			if(left > 0 && togo > 1.0f) {
				rfl.dashGoal = goal;
				rfl.dashSpeed = togo / left;
			}
		}


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
	player.devGod = Config().DevAutoStage != 0 || (Config().DevGodPlayers != 0 && desc.clientHd != ClientHandle::INVALID);
	if(!ingameSkillPoints) {
		player.skillPoints = 0;
		const GameXmlContent& content = GetGameXmlContent();
		for(const ClassType cls : { player.mainClass, player.subClass }) {
			if(cls == ClassType::NONE) continue;
			foreach_const(sk, content.GetMaster(cls).skillIDs) {
				const auto* ups = content.FindSkillUpgrades((i32)*sk);
				if(!ups || ups->empty() || (player.skillLevels.find((i32)*sk) == player.skillLevels.end() && player.skillLevels.size() >= player.skillLevels.max_size())) continue;
				u8 level = 0;
				foreach_const(l, desc.stageSkillLevels) if(l->first == (i32)*sk) level = (u8)MIN((size_t)l->second, ups->size());
				player.skillLevels[(i32)*sk] = level;
			}
		}
	}
	player.mainCharaID = 0;
	player.level = 1;
	player.experience = 0;
	player.body = physics.CreateDynamicBody(PLAYER_BODY_RADIUS, 70, pos); // radius 100 is found in files but 110 (_AILength) matches better
	player.spawnPos = pos;

	// clear input
	player.input.moveTo = pos;
	player.input.speed = 0;
	player.input.rot = rot;
	player.input.tag = 0;
	player.input.jump = 0;
	player.input.action = ActionStateID::INVALID;

	CreatePlayerMasters(player);
	return player;
}

void World::CreatePlayerMasters(Player& player)
{
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
	ComputeGearBonus(main, player.gearDocs[0], player.mainWeapon, GetGameXmlContent().GearPowerPercent(replication->gameType));
	AddTitleBonus(main, player.statTitle, player.mainWeapon);
	const bool botGear = player.botGear[0] || player.botGear[1];
	AddMeasuredBotGear(main, player.mainWeapon, GetGameXmlContent().GearPowerPercent(replication->gameType), player.botGear[0]);
	if(!player.gearDocs[0].empty()) LOG("Gear :: %ls power %d%% attack +%.1f skill power +%.1f health +%.1f defence +%.1f", player.name.data(), GetGameXmlContent().GearPowerPercent(replication->gameType), main.GearBonus(STAT_ATTACK), main.GearBonus(STAT_SKILLPOWER), main.GearBonus(STAT_HEALTH), main.GearBonus((u8)StatType::STAT_TYPE_DEFENCE));
	main.hpMax = (i32)(MasterStat(main.classType, STAT_HEALTH) + main.GearBonus(STAT_HEALTH));
	main.hp = main.hpMax;
	main.seed = (i32)RandUint();




	sub.parent = &player;
	sub.classType = player.subClass;
	sub.skinIndex = player.subSkin;
	ComputeGearBonus(sub, player.gearDocs[1], player.subWeapon, GetGameXmlContent().GearPowerPercent(replication->gameType));
	AddTitleBonus(sub, player.statTitle, player.subWeapon);
	AddMeasuredBotGear(sub, player.subWeapon, GetGameXmlContent().GearPowerPercent(replication->gameType), player.botGear[1]);
	if(botGear) LOG("Gear :: bot %ls measured gear attack +%.1f/+%.1f defence +%.1f/+%.1f health +%.0f/+%.0f", player.name.data(), main.GearBonus(STAT_ATTACK), sub.GearBonus(STAT_ATTACK), main.GearBonus((u8)StatType::STAT_TYPE_DEFENCE), sub.GearBonus((u8)StatType::STAT_TYPE_DEFENCE), main.GearBonus(STAT_HEALTH), sub.GearBonus(STAT_HEALTH));
	sub.hpMax = (i32)(MasterStat(sub.classType, STAT_HEALTH) + sub.GearBonus(STAT_HEALTH));
	sub.hp = sub.hpMax;
	ASSERT(main.hpMax > 0 && sub.hpMax > 0);
	sub.seed = (i32)RandUint();
	BuildInitStats(main, player.mainWeapon);
	BuildInitStats(sub, player.subWeapon);
}

void World::ReplacePlayerMasters(Player& player, const PlayerDescription& desc)
{
	ToggleOff(player);
	const u32 index = player.index;
	for(auto it = activeStatuses.begin(); it != activeStatuses.end();) {
		if(it->targetIndex != index) { ++it; continue; }
		if(it->info && it->info->auraStatus != 0) EndAura(index, *it->info);
		it = activeStatuses.erase_unsorted(it);
	}
	foreach_const(c, player.characters) {
		actorMasterMap.erase((*c)->UID);
		actorMasterList.erase(*c);
	}
	PhysicsDynamicBody* body = player.body;
	const vec3 spawnPos = player.spawnPos;
	const Player::Input input = player.input;
	const u8 level = player.level;
	const u32 experience = player.experience;
	const i32 ingameLevel = player.ingameLevel;
	const i32 ingameExp = player.ingameExp;
	const i32 skillPoints = player.skillPoints + player.skillPointsSpent;
	const bool devGod = player.devGod;
	const bool devVision = player.devVision;
	player.~Player();
	new(&player) Player(index, desc);
	player.body = body;
	player.spawnPos = spawnPos;
	player.input = input;
	player.input.action = ActionStateID::INVALID;
	player.level = level;
	player.experience = experience;
	player.ingameLevel = ingameLevel;
	player.ingameExp = ingameExp;
	player.skillPoints = skillPoints;
	player.devGod = devGod;
	player.devVision = devVision;
	CreatePlayerMasters(player);
	if(ingameSkillPoints && player.clientHd != ClientHandle::INVALID) replication->SendSkillPoints(player.clientHd, player.userID, player.skillPoints);
	ApplyPassives(player);
	LOG("Loadout :: %ls masters=(%d, %d) skills=(%d, %d, %d, %d)", player.name.data(), (i32)player.mainClass, (i32)player.subClass, (i32)player.skills[0], (i32)player.skills[1], (i32)player.skills[2], (i32)player.skills[3]);
}

World::ActorNpc& World::SpawnNpcActor(CreatureIndex docID, i32 localID)
{
	ActorUID actorUID = NewActorUID();

	actorNpcList.emplace_back(actorUID);
	ActorNpc& actor = actorNpcList.back();
	actor.docID = (CreatureIndex)docID;
	actor.localID = localID;
	actor.spawnedAt = localTime;

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

static const f64 CAST_LOCK_MS = 500.0;
static const u32 ERROR_TYPE_SKILL_INVALID_STATE_BLOCK = 563;
static const i32 ULTIMATE_SKILL_SUFFIX = 50;

void World::PlayerCastSkill(Player& player, SkillID skillIn, const vec3& castPosIn, Slice<const ActorUID> targets, f32 clientTime, ActionStateID forcedAction)
{
	const SkillID skillID = ResolveComboRecast(player, skillIn, forcedAction);
	const vec3 castPos = ResolveCastPos(player, skillID, castPosIn, forcedAction);
	if(frozen || combatLocked || !SkillTargetsAllowed(player, skillID, targets)) {
		if(forcedAction == ActionStateID::INVALID && player.clientHd != ClientHandle::INVALID) {
			LOG("Skill :: %ls cast %d refused (frozen %d, locked %d)", player.name.data(), (i32)skillID, (i32)frozen, (i32)combatLocked);
			replication->SendCastSkillRefused(player.clientHd, player.Main().UID, skillID, ERROR_TYPE_SKILL_INVALID_STATE_BLOCK, player.body->GetWorldPos(), player.input.rot.upperYaw, player.input.speed, clientTime);
		}
		return;
	}
	if(forcedAction == ActionStateID::INVALID) TitanOnCast(player, skillIn);
	MarkInBattle(player);
	const ActorUID hitEventTarget = player.memorizedTarget;
	if(targets.size() == 0 && hitEventTarget != ActorUID::INVALID && GetGameXmlContent().UsesHitEventTarget((i32)skillID)) targets = Slice<const ActorUID>(&hitEventTarget, 1);
	eastl::fixed_vector<ActorUID,10,false> selectedTargets;
	if(const GameXmlContent::SkillInput* selIn = GetGameXmlContent().FindSkillInput((i32)skillID)) {
		if(selIn->allEnemies) {
			foreach_const(e, players) if(e->team != player.team && !e->dead) selectedTargets.push_back(e->Main().UID);
			targets = Slice<const ActorUID>(selectedTargets.data(), selectedTargets.size());
		}
		if(selIn->minHpFriend) {
			const Player* weakest = nullptr;
			foreach_const(f, players) if(f->team == player.team && f->index != player.index && !f->dead && (!weakest || f->Main().hp < weakest->Main().hp)) weakest = &*f;
			if(weakest) selectedTargets.push_back(weakest->Main().UID);
			targets = Slice<const ActorUID>(selectedTargets.data(), selectedTargets.size());
			LOG("Skill :: %ls skill=%d picks the weakest ally %ls", player.name.data(), (i32)skillID, weakest ? weakest->name.data() : L"(none)");
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
	const bool castRelease = followUp && forcedAction == skill.action;
	if(!followUp && content.FindSkillToggle((i32)skillID)) {
		if(player.toggleSkill == skillID) {
			ToggleOff(player);
			return;
		}
		ToggleOn(player, skillID, castPos);
	}
	if(!followUp && player.casting.skillID == skillID && player.casting.type != GameXmlContent::SkillCast::DELAY) {
		ReleaseCast(player);
		return;
	}
	if(!followUp && player.casting.skillID != SkillID::INVALID && player.casting.skillID != skillID && player.casting.interruptByOtherSkill) CancelCast(player, "other skill");
	const GameXmlContent::SkillCast* cast = followUp ? nullptr : content.FindSkillCast((i32)skillID);
	castCostLevel = followUp ? nullptr : PickCostLevel(player, skillID);
	const GameXmlContent::SkillUpgrade* actionUpgrade = followUp ? nullptr : LatestUpgradeWith(player, (i32)skillID, [](const GameXmlContent::SkillUpgrade& u) { return u.action != ActionStateID::INVALID; });
	ActionStateID actionState = skill.action;
	ActionStateID remoteAction = ActionStateID::INVALID;
	const GameXmlContent::RandomActionList* randomActions = nullptr;
	if(followUp) actionState = forcedAction;
	else if(castCostLevel && castCostLevel->action != ActionStateID::INVALID) { actionState = castCostLevel->action; randomActions = &castCostLevel->randomActions; remoteAction = castCostLevel->action; }
	else if(actionUpgrade) { actionState = actionUpgrade->action; randomActions = &actionUpgrade->randomActions; remoteAction = actionUpgrade->action; }
	else if(cast) actionState = cast->action;
	else randomActions = content.FindSkillRandomActions((i32)skillID);
	if(randomActions && !randomActions->empty()) {
		const u32 pick = RandUint() % (u32)(randomActions->size() + 1);
		if(pick > 0) { actionState = (*randomActions)[pick - 1]; remoteAction = actionState; }
	}

	if(!followUp && !CheckAndPayCast(player, skillID, castPos, targets, clientTime)) return;
	if(!followUp) {
		AchieveEvent ev;
		ev.sub = GameXmlContent::AchieveSub::COMMON_USE_SKILL;
		ev.object = (i32)skillID;
		ev.stage = (i32)replication->stageIndex;
		ev.gameType = (i32)replication->gameType;
		AchieveLive(player, ev);
	}
	if(!followUp) DropPendingActions(player);
	if(!followUp) BreakHide(player);
	BushRevealOnFire(player);

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
	const auto* costLevels = castCostLevel ? GetGameXmlContent().FindSkillCostLevels((i32)skillID) : nullptr;
	const i8 costLevelIndex = costLevels ? (i8)(castCostLevel - costLevels->data() + 1) : 0;
	castCostLevel = UpgradeCastMods(player, skillID, castCostLevel);
	rpCast.actionID = actionState;
	rpCast.costLevel = costLevelIndex;
	rpCast.sendAction = cast != nullptr;

	rpCast.casterPos = player.body->GetWorldPos();
	rpCast.casterMoveDir = dir;
	rpCast.casterRot = { angle, 0, angle };
	rpCast.casterSpeed = player.input.speed; // FIXME: should not come from input
	rpCast.clientTime = clientTime;

	eastl::copy(targets.begin(), targets.end(), eastl::back_inserter(rpCast.targetList));

	const bool actionOnly = hitActionOnly && followUp;
	hitActionOnly = false;
	if(!castRelease && !actionOnly) replication->FramePushSkillCast(rpCast);
	if(!followUp) PassiveOnSkillCast(player, (i32)skillID);
	if(cast && cast->action == ActionStateID::INVALID) {
		static const GameXmlContent::Action noCastAction = {};
		castCostLevel = nullptr;
		StartCasting(player, skillID, *cast, castPos, targets, noCastAction);
		if(player.clientHd != ClientHandle::INVALID) UpdateComboSlot(player, skillID, skill.type, castPos);
		return;
	}


	/*
	 * After the cast, we start executing a "skill program"
	 * It is a set of simple instructions in the world of MxM
	 * Each "program" is described in ActionBase.xml
	 */

	// Trigger new skill execution
	if(!content.SkillActionKnown(ActionClass(player), actionState, player.Main().stance)) {
		WARN("PlayerCastSkill :: %ls skill=%d action %d unknown for action class %d", player.name.data(), (i32)skillID, (i32)actionState, (i32)ActionClass(player));
		return;
	}
	player.Main().actionState = actionState;

	SkillProgram prog;
	prog.skillID = skillID;
	prog.actionID = actionState;
	prog.actionClass = ActionClass(player);
	prog.stance = player.Main().stance;
	prog.castPos = castPos;
	prog.castAngle = angle;
	prog.casterUID = player.Main().UID;
	eastl::copy(targets.begin(), targets.end(), eastl::back_inserter(prog.targetList));
	prog.startTime = localTime;
	prog.commandID = 0;
	skillProgramList.push_back(prog);
	player.lastCastSkill = skillID;
	player.lastCastAction = actionState;
	player.lastCastStance = player.Main().stance;
	player.lastCastStart = localTime;

	// go through the skill program to find out how much the master moves
	f32 distance = 0;
	f32 moveDuration = 0;
	vec2 gotoDir = vec2(0);
	ActorUID chaseTarget = ActorUID::INVALID;
	f32 gotoDistance = -1.0f;

	const auto& action = content.GetSkillAction(ActionClass(player), actionState, player.Main().stance);

	if(player.clientHd == ClientHandle::INVALID) {
		f32 actionLen = 0;
		foreach_const(st, action.anims) actionLen += content.GetAniSeqTime(ActionClassName(player), st->animationId) * (f32)MAX(st->loop, 1);
		if(actionLen <= 0) actionLen = action.seqLength;
		player.botAttackState = actionState;
		player.botAttackStateSkill = true;
		player.botAttackStateUntil = TimeAdd(localTime, TimeMsToTime(actionLen * 1000.0));
	}

	foreach_const(cmd, action.commands) {
		switch(cmd->type) {
			case ActionCommand::Type::GRAPH_MOVE_HORZ: {
				distance = cmd->graphMoveHorz.distance;
				moveDuration = SkillMoveDuration(content, ActionClassName(player), action, cmd->animIndex);
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
						vec2 targetPos;
						f32 targetRadius = 0;
						if(targets.size() > 0 && TargetBody(targets[0], &targetPos, &targetRadius)) {
							const vec2 rel = targetPos - vec2(player.body->GetWorldPos());
							const f32 len = glm::length(rel);
							if(len > 1.0f) {
								const bool through = cmd->move.preset == ActionCommand::MovePreset::GOTO_CURRENT_TARGET_THROUGH || cmd->move.preset == ActionCommand::MovePreset::GOTO_CURRENT_TARGET_THROUGH_CHASE;
								gotoDir = rel / len;
								gotoDistance = through ? len + targetRadius + HitRadius(player) : MAX(len - targetRadius - HitRadius(player), 0.0f);
								if(distance == 0) {
									distance = gotoDistance;
									moveDuration = SkillMoveDuration(content, ActionClassName(player), action, cmd->animIndex);
								}
								if(cmd->move.preset == ActionCommand::MovePreset::GOTO_CURRENT_TARGET_CHASE || cmd->move.preset == ActionCommand::MovePreset::GOTO_CURRENT_TARGET_THROUGH_CHASE) chaseTarget = targets[0];
							}
						}
					} break;
				}
			} break;
		}
	}

	if(gotoDistance >= 0 && distance > gotoDistance) distance = gotoDistance;

	Replication::SkillExec rpExec;
	rpExec.casterUID = player.Main().UID;
	rpExec.skillID = skillID;
	rpExec.castPos = castPos;
	rpExec.actionID = actionState;
	rpExec.costLevel = costLevelIndex;
	eastl::copy(targets.begin(), targets.end(), eastl::back_inserter(rpExec.targetList));

	rpExec.moveDuration = moveDuration;
	rpExec.startPos = player.body->GetWorldPos();
	rpExec.endPos = rpExec.startPos;
	rpExec.moveDir = dir;
	rpExec.rot = { angle, 0, angle };
	rpExec.speed = player.input.speed; // FIXME: should not come from input

	if(distance > 0 && glm::length(gotoDir) < 0.5f && glm::length(vec2(castPos)) > 1.0f) TargetPositionMove(player, skillID, action, castPos, &distance, &gotoDir);
	if(distance != 0) {
		const vec2 moveDir = glm::length(gotoDir) > 0.5f ? gotoDir : dir;
		const vec3 endPos = physics.FindMovePos(player.body, vec3(moveDir * distance, 0), moveDuration);
		rpExec.moveDir = moveDir;
		rpExec.endPos = endPos;
	}

	if(actionOnly) replication->SendActionStateOnlyToAll(player.Main().UID, actionState, WorldYawToMxmYaw(angle), distance != 0, rpExec.startPos, distance != 0 ? rpExec.endPos : rpExec.startPos, moveDuration);
	else if(!cast) replication->FramePushSkillExec(rpExec);
	if(chaseTarget != ActorUID::INVALID && player.clientHd != ClientHandle::INVALID && moveDuration > 0) {
		player.chaseTarget = chaseTarget;
		player.chaseAction = actionState;
		player.chaseUntil = TimeAdd(localTime, TimeMsToTime(moveDuration * 1000.0));
		player.chaseNext = localTime;
	}

	if(castRelease && player.lastComboSkill == skillID) player.lastComboSkill = SkillID::INVALID;
	ScheduleSkillRemotes(player, skillID, castPos, targets, angle, followUp && !castRelease ? 0 : -1, remoteAction != ActionStateID::INVALID ? remoteAction : cast ? cast->action : forcedAction);
	castCostLevel = nullptr;
	if(cast) StartCasting(player, skillID, *cast, castPos, targets, action);
	skillProgramList.back().passes = lastAnimPasses;

	if(!followUp && player.clientHd != ClientHandle::INVALID) UpdateComboSlot(player, skillID, skill.type, castPos);
	if(!followUp) {
		if(const GameXmlContent::SkillStance* st = content.FindSkillStance((i32)skillID)) {
			const GameXmlContent::SkillUpgrade* su = LatestUpgradeWith(player, (i32)skillID, [](const GameXmlContent::SkillUpgrade& u) { return u.stanceDuration > 0; });
			const GameXmlContent::SkillUpgrade* fu = LatestUpgradeWith(player, (i32)skillID, [](const GameXmlContent::SkillUpgrade& u) { return u.weaponFireCount > 0; });
			SetStance(player.Main(), st->stance, su ? su->stanceDuration : st->duration, fu ? fu->weaponFireCount : st->weaponFireCount, st->cancelByDefeat);
			player.Main().stanceSkill = skillID;
		}
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
			const f32 maxDistance = SkillMaxDistance(player, (i32)skillID);
			if(in->minDistance > 0) clamped = MAX(clamped, in->minDistance);
			if(maxDistance > 0) clamped = MIN(clamped, maxDistance);
			if(clamped != len) castPos = vec3(from + rel / len * clamped, castPos.z);
		}
		else if(in && in->minDistance > 0) castPos = vec3(from + vec2(cosf(player.input.rot.upperYaw), sinf(player.input.rot.upperYaw)) * in->minDistance, castPos.z);
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


bool World::CheckAndPayCast(Player& player, SkillID skillID, const vec3& castPos, Slice<const ActorUID> targets, f32 clientTime)
{
	const auto& content = GetGameXmlContent();
	const bool shirk = GetGameXmlContent().GetSkillConsumeStamina((i32)skillID) > 0;
	if(IsBlocked(player, false, !shirk, shirk)) {
		LOG("Blocked :: %ls skill=%d (input blocked by a status)", player.name.data(), (i32)skillID);
		if(player.clientHd != ClientHandle::INVALID) replication->SendCastSkillRefused(player.clientHd, player.Main().UID, skillID, ERROR_TYPE_SKILL_INVALID_STATE_BLOCK, player.body->GetWorldPos(), player.input.rot.upperYaw, player.input.speed, clientTime);
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
	if(const u32 costError = PlayerPaySkillCost(player, (i32)skillID)) {
		if(player.clientHd != ClientHandle::INVALID) replication->SendCastSkillRefused(player.clientHd, player.Main().UID, skillID, costError, player.body->GetWorldPos(), player.input.rot.upperYaw, player.input.speed, clientTime);
		return false;
	}
	player.castLockUntil = TimeAdd(localTime, TimeMsToTime(CAST_LOCK_MS));
	if((i32)skillID % 100 == ULTIMATE_SKILL_SUFFIX) player.ultCooldownUntil = TimeAdd(localTime, TimeMsToTime(SkillCoolTime(player, (i32)skillID) * 1000.0));
	if(castCostLevel) {
		for(i32 r = 0; r < 3; r++) {
			if(castCostLevel->consume[r] > 0 && player.res[r] >= 0) { player.res[r] -= castCostLevel->consume[r]; player.resDirty = true; }
		}
		LOG("Cost :: %ls skill=%d empowered level (consumed rp=%.0f bb=%.0f, %d remotes / %d statuses replaced, damage x%.3f)", player.name.data(), (i32)skillID,
			castCostLevel->consume[1], castCostLevel->consume[2], (i32)castCostLevel->remoteReplace.size(), (i32)castCostLevel->statusReplace.size(), castCostLevel->damageRatio);
	}
	return true;
}

void World::TargetPositionMove(const Player& player, SkillID skillID, const GameXmlContent::Action& action, const vec3& castPos, f32* distance, vec2* gotoDir) const
{
	const f32 maxDistance = SkillMaxDistance(player, (i32)skillID);
	foreach_const(cmd, action.commands) {
		if(cmd->type == ActionCommand::Type::MOVE && (cmd->move.preset == ActionCommand::MovePreset::TARGET_POS || cmd->move.preset == ActionCommand::MovePreset::TARGET_POS_LEAP)) {
			const vec2 rel = vec2(castPos) - vec2(player.body->GetWorldPos());
			const f32 len = glm::length(rel);
			*distance = maxDistance > 0 ? MIN(len, maxDistance) : len;
			if(len > 1.0f) *gotoDir = rel / len;
		}
	}
}

void World::StartCasting(Player& player, SkillID skillID, const GameXmlContent::SkillCast& cast, const vec3& castPos, Slice<const ActorUID> targets, const GameXmlContent::Action& action)
{
	const bool bot = player.clientHd == ClientHandle::INVALID;
	f32 wait = cast.time;
	if(bot && cast.type != GameXmlContent::SkillCast::DELAY) wait = MIN(wait, action.seqLength);
	if(bot && cast.type == GameXmlContent::SkillCast::CHARGING && cast.maxChargingTime > 0) {
		const ChargeHoldRule* hold = FindChargeHoldRule(skillID);
		wait = hold ? MIN(hold->holdSec, cast.maxChargingTime) : cast.maxChargingTime;
	}
	if(bot && cast.type == GameXmlContent::SkillCast::LIMIT) {
		if(const ChargeHoldRule* hold = FindChargeHoldRule(skillID)) wait = cast.time > 0 ? MIN(hold->holdSec, cast.time) : hold->holdSec;
	}
	player.casting.skillID = skillID;
	player.casting.castPos = castPos;
	player.casting.target = targets.size() > 0 ? targets[0] : ActorUID::INVALID;
	player.casting.startPos = vec2(player.body->GetWorldPos());
	player.casting.executeAt = TimeAdd(localTime, TimeMsToTime(wait * 1000.0));
	player.casting.startedAt = localTime;
	player.casting.type = (u8)cast.type;
	player.casting.interruptByMove = cast.interruptByMove;
	player.casting.interruptByLostTarget = cast.interruptByLostTarget;
	player.casting.interruptByAttacked = cast.interruptByAttacked;
	player.casting.interruptByOtherSkill = cast.interruptByOtherSkill;
	player.casting.lifted = false;
	player.casting.casterClass = player.Main().classType;
	foreach_const(cmd, action.commands) if(cmd->type == ActionCommand::Type::MOVE && cmd->move.preset == ActionCommand::MovePreset::LIFTED) player.casting.lifted = true;
	LOG("Cast :: %ls skill=%d start action=%d type=%d wait=%.2f", player.name.data(), (i32)skillID, (i32)cast.action, (i32)cast.type, wait);
}

void World::UpdateComboSlot(Player& player, SkillID skillID, SkillType skillType, const vec3& castPos, bool onHit)
{
	const auto& content = GetGameXmlContent();
	const i32 chainSlot = skillType == SkillType::COMBO ? player.comboSlot : -1;
	const SkillID chainBase = player.comboBase;
	if(player.comboSlot >= 0 && skillType == SkillType::COMBO) {
		replication->SendChangeSkillSlot(player.clientHd, player.Main().UID, player.comboSlot, player.comboBase, 0);
		LOG("Combo :: %ls slot %d back to %d", player.name.data(), player.comboSlot, (i32)player.comboBase);
		player.comboSlot = -1;
	}
	if(content.FindSkillHitEventRemote((i32)skillID) != 0 && !onHit) return;
	i32 combo = ComboSkillFor(player, (i32)skillID);
	if(combo == 0 && (onHit || skillType == SkillType::COMBO)) combo = content.GetNextComboSkill((i32)skillID);
	if(combo != 0) {
		const i32 base = (i32)player.mainCharaID * 2;
		i32 slot = -1;
		const i32 order = ((i32)skillID / 10) % 10;
		if(player.skills[base] == skillID && order >= 1 && order <= 4) slot = MEASURED_COMBO_SLOT_FIRST;
		else if(player.skills[base + 1] == skillID && order >= 1 && order <= 4) slot = MEASURED_COMBO_SLOT_FIRST + 1;
		else if(order == 5 && (i32)skillID % 10 == 0) slot = MEASURED_COMBO_SLOT_UG;
		else if(order == 0 && (i32)skillID % 10 != 0) slot = MEASURED_COMBO_SLOT_SHIRK;
		const bool chained = slot < 0 && chainSlot >= 0;
		if(chained) slot = chainSlot;
		if(slot >= 0) {
			player.comboPending = SkillID(combo);
			player.comboSkill = SkillID(combo);
			player.comboSendAt = TimeAdd(localTime, TimeMsToTime(50.0));
			player.comboSlot = slot;
			player.comboBase = chained ? chainBase : skillID;
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
	const ClassType actionClass = ActionClass(player);
	const i32 child = content.GetChildSkill(actionClass, actionState);
	if(child > 0 && content.IsChildOnExisting(actionClass, actionState)) {
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

void World::WarpPlayer(Player& p, const vec3& pos, f32 yaw)
{
	TeleportPlayer(p, pos, yaw);
	if(p.clientHd != ClientHandle::INVALID) replication->SendTeleportRequest(p.clientHd, p.Main().UID, pos);
	else replication->SendPlayerSyncTeleportToAll(p.Main().UID, pos);
}

void World::SpawnRespawnFlyRemote(Player& player)
{
	const i32 doc = GetGameXmlContent().respawnFlyRemote;
	if(doc == 0 || pendingRemotes.full() || !GetGameXmlContent().FindRemote(RemoteIdx(doc))) return;
	PendingRemote pr;
	pr.casterMasterUID = player.Main().UID;
	pr.time = localTime;
	pr.idx = RemoteIdx(doc);
	pr.preset = (i32)ActionCommand::TargetPreset::POS_TTT_1;
	pr.offsetSide = 0;
	pr.offsetFront = 0;
	pr.ownerIndex = player.index;
	pr.skillID = SkillID::INVALID;
	pr.castID = nextCastID++;
	pr.damage = 0;
	pr.castPos = player.body->GetWorldPos();
	pr.angle = player.input.rot.upperYaw;
	pr.target = ActorUID::INVALID;
	pendingRemotes.push_back(pr);
}

void World::SwapMainMaster(Player& p)
{
	AccumulateMasterScore(p);
	const ActorUID oldMain = p.Main().UID;
	p.mainCharaID ^= 1;
	p.firingCombo = 0;
	p.botComboShotAt = Time::ZERO;
	RetargetMonsters(oldMain, p.Main().UID);
	eastl::swap(p.mana, p.benchMana);
	eastl::swap(p.stamina, p.benchStamina);
	eastl::swap(p.res, p.benchRes);
	p.manaDirty = true;
	p.staminaDirty = true;
	p.resDirty = true;
	p.ultDirty = true;
	p.regenResend = true;
	ShowTagToEnemies(p);
	const ActorUID newMain = p.Main().UID;
	foreach(st, activeStatuses) {
		if(st->targetIndex != p.index || st->targetUID != oldMain || !st->info || localTime >= st->until) continue;
		if(st->info->tagMode == StatusInfo::TAG_DROP) st->until = localTime;
		else if(st->info->tagMode == StatusInfo::TAG_MOVE) {
			replication->SendRemoveStatusToAll(st->statusID, oldMain, st->casterUID);
			st->targetUID = newMain;
			const f32 remain = st->info->permanent ? StatusSentDuration(*st->info) : MIN((f32)TimeDiffSec(TimeDiff(localTime, st->until)), STATUS_INFINITE_DURATION_SEC);
			const vec3 at = p.body ? p.body->GetWorldPos() : vec3(0);
			replication->SendAddStatusToAll(st->statusID, newMain, st->casterUID, remain, at, at, 0, st->overlap, 1.0f, -1.0f, false, false);
		}
	}
	foreach(st, activeStatuses) {
		if(st->casterUID != oldMain || st->targetIndex == p.index || !st->info || st->info->type != StatusType::LINK || localTime >= st->until) continue;
		const f32 remain = MIN((f32)TimeDiffSec(TimeDiff(localTime, st->until)), STATUS_INFINITE_DURATION_SEC);
		const vec3 at = p.body ? p.body->GetWorldPos() : vec3(0);
		replication->linkMirror = false;
		replication->SendModifyStatusToAll(st->statusID, st->targetUID, oldMain, (u8)MAX(st->overlap, 1), (u8)MAX(st->custom, 0), newMain);
		replication->SendRemoveStatusToAll(st->statusID, oldMain, st->targetUID);
		replication->SendAddStatusToAll(st->statusID, newMain, st->targetUID, remain, at, at, 0, st->overlap, 1.0f, -1.0f, false, false);
		replication->linkMirror = true;
		st->casterUID = newMain;
		LOG("Tag :: %ls link %d on %d now cast by the new master", p.name.data(), st->statusID, (i32)st->targetUID);
	}
}

void World::RetargetMonsters(ActorUID from, ActorUID to)
{
	foreach(npc, actorNpcList) {
		if(npc->monsterTarget != from) continue;
		npc->monsterTarget = to;
		replication->SendAiSetTargetToAll(npc->UID, to);
	}
	foreach(h, monsterHits) if(h->target == from) h->target = to;
	foreach(f, monsterFires) if(f->target == from) f->target = to;
	foreach(r, monsterRemotes) if(r->target == from) r->target = to;
	foreach(c, monsterChains) if(c->target == from) c->target = to;
}

void World::ResetMainMasterForAreaMove(Player& p)
{
	if(p.mainCharaID == 0) return;
	if(p.casting.skillID != SkillID::INVALID) CancelCast(p, "area move");
	SwapMainMaster(p);
	LOG("Stage :: %ls leads with the first master again after the area move", p.name.data());
}

void World::ClearMapContent()
{
	actorNpcList.clear();
	actorNpcMap.clear();
	actorDynamicList.clear();
	actorDynamicMap.clear();
	remoteInsts.clear();
	pendingRemotes.clear();
	deadStructureFootprints.clear();
	groundAuras.clear();
	projectileList.clear();
	pendingSummonCommands.clear();
	groundItems.clear();
	monsterCalls.clear();
	monsterHits.clear();
	monsterChains.clear();
	monsterRemotes.clear();
	deathHits.clear();
	boundTriggerStates.clear();
	levelTriggerOverrides.clear();
	levelGroupOverrides.clear();
	switchSteps.clear();
	levelEventTimers.clear();
	dialogEnds.clear();
	npcStatuses.clear();
	mapInteractions.clear();
	deadCounts.clear();
	deadCountsDone.clear();
	boundTriggerFires.clear();
	hpTriggersFired.clear();
	andInputsFired.clear();
	levelEventsStarted = false;
	mapInteractionsBuilt = false;
	portalRequested = false;
	physics.ClearMapStatics();
}

void World::RemovePlayer(Player& player)
{
	player.removed = true;
	player.dead = true;
	player.tRespawn = TimeAdd(localTime, TimeMsToTime(REMOVED_PLAYER_RESPAWN_MS));
	TeleportPlayer(player, player.spawnPos, 0.f);
}

void World::RespawnPlayer(Player& player)
{
	player.dead = false;
	player.mana = -1.0f;
	player.benchMana = -1.0f;
	player.stamina = -1.0f;
	player.benchStamina = -1.0f;
	player.res = { -1, -1, -1 };
	player.benchRes = { -1, -1, -1 };
	player.manaDirty = true;
	player.staminaDirty = true;
	foreach(c, player.characters) {
		(*c)->hp = (*c)->hpMax;
	}

	player.body->collider->setFootPosition(PxExtendedVec3(player.spawnPos.x, player.spawnPos.y, player.spawnPos.z));
	player.body->vel = vec3(0);
	player.input.moveTo = player.spawnPos;

	LOG("Respawn :: %ls", player.name.data());
	replication->SendReviveToAll(player.userID, player.Main().UID, player.Sub().UID, player.spawnPos);
	SpawnRespawnFlyRemote(player);
	foreach(c, player.characters) {
		replication->SendHpToAll((*c)->UID, (*c)->hp, (*c)->hpMax);
	}
	if(player.passivesApplied) ApplyPassives(player);
	ApplyStatus(player, player, STATUS_PC_RESPAWN_INVICIBLE_3S);
	foreach(npc, actorNpcList) {
		if(npc->hp <= 0 || npc->removeAt != Time::ZERO || !npc->monsterAi || npc->monsterAi->teamBuff == 0 || npc->faction != (Faction)player.team) continue;
		ApplyStatusFromNpc(*npc, player, npc->monsterAi->teamBuff, STATUS_INFINITE_DURATION_SEC);
	}
}

void World::ExecuteSkillProgram(SkillProgram& prog)
{
	ActorMaster* caster = FindMasterActor(prog.casterUID);
	if(!caster) {
		prog.Finish();
		return;
	}
	const vec2 dir = vec2(cosf(prog.castAngle), sinf(prog.castAngle));
	const auto& action = GetGameXmlContent().GetSkillAction(prog.actionClass, prog.actionID, prog.stance);
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
			if(cmd.type == ActionCommand::Type::MOVE || cmd.type == ActionCommand::Type::GRAPH_MOVE_HORZ) LOG("SkillProgram :: action %d anim %d not played, move command skipped", (i32)prog.actionID, (i32)cmd.animIndex);
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

void World::AccumulateMasterScore(Player& player)
{
	Player::MasterScore& score = player.masterScores[player.mainCharaID];
	Player::MasterScore& snap = player.scoreSnapshot;
	score.kills += player.kills - snap.kills;
	score.deaths += player.deaths - snap.deaths;
	score.assists += player.assists - snap.assists;
	score.damageDealt += player.damageDealt - snap.damageDealt;
	for(i32 d = 0; d < (i32)player.damageStats.size(); d++) {
		const bool peak = d == END_DMG_MAX_DEALT || d == END_DMG_MAX_TAKEN;
		if(peak) { if(player.damageStats[d] > snap.damageStats[d]) score.damageStats[d] = MAX(score.damageStats[d], player.damageStats[d]); }
		else score.damageStats[d] += player.damageStats[d] - snap.damageStats[d];
	}
	const Time from = player.scoreSnapshotAt != Time::ZERO ? player.scoreSnapshotAt : gameStartedAt;
	if(from != Time::ZERO && localTime > from) score.activeSec += TimeDiffSec(TimeDiff(from, localTime));
	snap.kills = player.kills;
	snap.deaths = player.deaths;
	snap.assists = player.assists;
	snap.damageDealt = player.damageDealt;
	snap.damageStats = player.damageStats;
	player.scoreSnapshotAt = localTime;
}
