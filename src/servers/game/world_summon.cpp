#include "config.h"
#include "world.h"
#include "world_common.h"
#include <mxm/game_content.h>

static bool SummonStatusClientOnly(const StatusInfo& st)
{
	return st.type == StatusType::HIDE && (st.instant || st.durationSec <= 0);
}

static const i32 RELAY_MODE_STATE_OFFSET = 9;
static const i32 RELAY_IDS_PER_MODE = 3;
static const i32 RELAY_ARTILLERY_MODE = 2;
static const f32 RELAY_LEAP_SPEED = 2000.0f;
static const i32 RELAY_PRISM_MODE = 0;
static const f32 WALL_HEIGHT = 300.0f;
static const struct { i32 creature; f32 width; f32 depth; } WALL_FOOTPRINTS[] = { { 100033400, 300.0f, 80.0f }, { 100033401, 500.0f, 80.0f }, { 100033402, 500.0f, 80.0f } };
static const i32 RELAY_BRUISER_MODE = 1;
static const f32 RELAY_LIFE_SEC = 3600.0f;
static const f32 RELAY_PASSIVE_SEC = 3.2f;
static const f64 SUMMON_DIE_STAND_MS = 90.0;
static const i32 RELAY_STEALTH_STATUS = 1230210030;

void World::SpawnCreatureFromRemote(const RemoteInst& parent, i32 creature, const vec3& pos)
{
	if(pendingSummons.full() || parent.ownerIndex >= (u32)players.size()) return;
	PendingSummon ps;
	ps.time = localTime;
	ps.creature = creature;
	ps.ownerIndex = parent.ownerIndex;
	ps.pos = vec3(vec2(pos), players[parent.ownerIndex].body->GetWorldPos().z);
	ps.rot = vec3(0, 0, atan2f(parent.dir.y, parent.dir.x));
	ps.target = parent.target;
	pendingSummons.push_back(ps);
	LOG("Remote :: %d creates creature %d at (%.0f, %.0f)", (i32)parent.remote->ID, creature, pos.x, pos.y);
}

static f32 SummonPassStart(const SummonInfo& info, i32 state, i32 pass)
{
	f32 start = 0;
	for(i32 k = 0; k < pass; k++) {
		foreach_const(c, info.skillStates[state]) {
			if(c->animIndex != k || c->animId == 0) continue;
			start += GetGameXmlContent().GetAniSeqTime(info.className.c_str(), c->animId);
			break;
		}
	}
	return start;
}

void World::RunSummonState(ActorNpc& npc, i32 state)
{
	const SummonInfo* info = GetGameXmlContent().FindSummon((i32)npc.docID);
	if(!info || state < 0 || state > SUMMON_STATE_LIFTED) return;
	const bool lifted = state == SUMMON_STATE_LIFTED;
	const auto& commands = lifted ? info->onLifted : info->skillStates[state];
	Player& owner = players[npc.ownerIndex];
	Player* current = &owner;
	if(npc.ai == SummonInfo::AI_CHASE) {
		ActorMaster* m = FindMasterActor(npc.chaseTarget);
		current = m ? m->parent : nullptr;
	}
	else if(npc.skillTarget != ActorUID::INVALID) {
		ActorMaster* m = FindMasterActor(npc.skillTarget);
		if(m && !m->parent->dead) current = m->parent;
	}
	f32 yaw = owner.input.rot.upperYaw;
	const ActorNpc* targetNpc = npc.skillTarget != ActorUID::INVALID && (!current || current == &owner) ? FindNpcActor(npc.skillTarget) : nullptr;
	if(current && current != &owner) {
		const vec2 rel = vec2(current->body->GetWorldPos()) - vec2(npc.pos);
		if(glm::length(rel) > 1.0f) yaw = atan2f(rel.y, rel.x);
	}
	else if(targetNpc) {
		const vec2 rel = vec2(targetNpc->pos) - vec2(npc.pos);
		if(glm::length(rel) > 1.0f) yaw = atan2f(rel.y, rel.x);
	}
	if(!lifted) {
		eastl::fixed_string<char,48,false> name;
		name.sprintf("SKILL_%d_BEHAVIORSTATE", state);
		const ActionStateID action = ActionStateFromString(name.c_str());
		const vec3 targetPos = targetNpc ? targetNpc->pos : (current ? current->body->GetWorldPos() : npc.pos);
		if(npc.skillTarget != ActorUID::INVALID && current && current != &owner) {
			f32 speed = 0;
			foreach_const(c, info->skillStates[state]) {
				if(c->kind != SummonCommand::REMOTE) continue;
				const Remote* r = GetGameXmlContent().FindRemote(RemoteIdx(c->index));
				if(r && r->behaviorType == Remote::BehaviorType::TARGET && r->maxSpeed > 0) speed = MAX(speed, r->maxSpeed);
			}
			if(speed > 0) {
				const vec3 goal = current->body->GetWorldPos();
				replication->SendNpcMoveToAll(npc.UID, npc.pos, goal, yaw, speed);
				LOG("Summon :: creature=%d moves to its target at %.0f", (i32)npc.docID, speed);
				npc.pos = goal;
			}
		}
		if(action != ActionStateID::INVALID) {
			npc.actionState = action;
			npc.pendingMotion = action;
			npc.motionAt = TimeAdd(localTime, TimeMsToTime(50.0));
			npc.motionTarget = targetPos;
			npc.motionTargetUID = targetNpc ? targetNpc->UID : (current ? current->Main().UID : ActorUID::INVALID);
			npc.motionYaw = yaw;
			const f32 skillSec = MAX(GetGameXmlContent().GetAniSeqTime(info->className.c_str(), info->stateAnim[state]), SummonPassStart(*info, state, 8));
			npc.skillEndAt = skillSec > 0 ? TimeAdd(npc.motionAt, TimeMsToTime(skillSec * 1000.0)) : Time::ZERO;
		}
		LOG("Summon :: creature=%d plays %s", (i32)npc.docID, name.c_str());
	}
	f32 passStart = 0;
	i32 pass = 0;
	foreach_const(c, commands) {
		if(!lifted && c->animIndex > pass) {
			pass = c->animIndex;
			passStart = SummonPassStart(*info, state, pass);
		}
		const f32 t = passStart + c->delay;
		if(c->kind == SummonCommand::REMOTE) {
			if(pendingSummonCommands.full()) continue;
			PendingSummonCommand pc;
			pc.time = TimeAdd(localTime, TimeMsToTime(t * 1000.0));
			pc.ownerIndex = npc.ownerIndex;
			pc.kind = c->kind;
			pc.index = c->index;
			pc.nodeName = c->nodeName;
			pc.pos = npc.pos;
			pc.yaw = yaw;
			pc.skill = info->skills.empty() ? 0 : info->skills[0];
			pc.until = npc.despawnAt;
			pc.target = current && current != &owner ? current->Main().UID : (npc.ai == SummonInfo::AI_CHASE && FindNpcActor(npc.chaseTarget) ? npc.chaseTarget : ActorUID::INVALID);
			pc.source = npc.UID;
			pc.aim = npc.aimPos;
			pendingSummonCommands.push_back(pc);
		}
		else if(c->kind == SummonCommand::STATUS) {
			if(c->onCurrent) {
				if(current && !current->dead) {
					statusCasterOverride = npc.UID;
					ApplyStatus(owner, *current, c->index);
					statusCasterOverride = ActorUID::INVALID;
				}
				continue;
			}
			const StatusInfo* st = GetGameXmlContent().FindStatus(c->index);
			if(!st) continue;
			const f32 duration = st->durationSec > 0 ? st->durationSec : 1.0f;
			if(!SummonStatusClientOnly(*st)) replication->SendAddStatusToAll(c->index, npc.UID, npc.UID, StatusSentDuration(*st), npc.pos, npc.pos, 0, 1, 1.0f, -1.0f, false);
			TrackNpcSelfStatus(npc.UID, *st);
			if(st->type == StatusType::SUICIDE || st->type == StatusType::COMA) {
				const Time at = TimeAdd(localTime, TimeMsToTime(t * 1000.0));
				if(npc.despawnAt == Time::ZERO || at < npc.despawnAt) npc.despawnAt = at;
			}
			LOG("Summon :: creature=%d self status %d (%s)", (i32)npc.docID, c->index, st->typeName.c_str());
			if(st->type == StatusType::TOGGLE && st->auraStatus == 0 && st->targetingRange > 0) {
				foreach(p, players) {
					if(p->dead) continue;
					bool attached = false;
					foreach_const(as, activeStatuses) if(as->targetIndex == p->index && as->casterUID == npc.UID && as->info && as->info->attachToCaster && as->until > localTime) attached = true;
					if(!attached && glm::length(vec2(p->body->GetWorldPos()) - vec2(npc.pos)) > st->targetingRange) continue;
					statusCasterOverride = npc.UID;
					ApplyStatus(owner, *p, c->index);
					statusCasterOverride = ActorUID::INVALID;
				}
			}
			if(st->auraStatus != 0 && st->targetingRange > 0 && !groundAuras.full()) {
				GroundAura ga;
				ga.ownerIndex = npc.ownerIndex;
				ga.auraStatus = st->ID;
				ga.range = st->targetingRange;
				ga.pos = npc.pos;
				ga.until = TimeAdd(localTime, TimeMsToTime(duration * 1000.0));
				ga.nextPulse = localTime;
				ga.sourceUID = npc.UID;
				groundAuras.push_back(ga);
			}
		}
		else if(c->kind == SummonCommand::DIE) {
			npc.dieMode = c->dieMode;
		}
	}
}

void World::ChildSkillOnExisting(Player& owner, i32 child, const vec3& pos)
{
	ActorNpc* npc = nullptr;
	const SummonInfo* info = nullptr;
	foreach(it, actorNpcList) {
		if(it->ownerIndex != owner.index || (it->despawnAt != Time::ZERO && localTime >= it->despawnAt)) continue;
		const SummonInfo* si = GetGameXmlContent().FindSummon((i32)it->docID);
		if(si && child <= 20 && !si->skillStates[child].empty()) { npc = &*it; info = si; }
	}
	if(!npc) {
		LOG("Summon :: %ls child SKILL_%d has no creature to run it", owner.name.data(), child);
		return;
	}
	f32 travel = 0;
	foreach_const(c, info->skillStates[child]) if(c->kind == SummonCommand::REMOTE) { travel = c->delay; break; }
	const vec3 goal = vec3(vec2(pos), npc->pos.z);
	const f32 dist = glm::length(vec2(goal) - vec2(npc->pos));
	LOG("Summon :: %ls child SKILL_%d on its creature=%d, flies %.0f in %.2fs", owner.name.data(), child, (i32)npc->docID, dist, travel);
	if(dist > 1.0f) {
		replication->SendAiMoveTargetToAll(npc->UID, goal, (u32)ActionCommand::MovePreset::TARGET_POS);
		npc->rot.z = atan2f(goal.y - npc->pos.y, goal.x - npc->pos.x);
		replication->SendNpcTurnToAll(npc->UID, npc->rot.z);
	}
	foreach(st, activeStatuses) {
		if(st->casterIndex != owner.index || !st->info || st->info->type != StatusType::TARGET || localTime >= st->until) continue;
		static const f32 HYP_CARRY_ARRIVE_EARLY_SEC = 0.1f;
		Player& carried = players[st->targetIndex];
		const Time drop = TimeAdd(localTime, TimeMsToTime(travel * 1000.0));
		const f32 flight = MAX(travel - HYP_CARRY_ARRIVE_EARLY_SEC, 0.05f);
		carried.carryReleaseAt = drop;
		if(carried.clientHd == ClientHandle::INVALID && !carried.dead) {
			carried.pushTarget = vec3(vec2(pos), carried.body->GetWorldPos().z);
			carried.pushUntil = TimeAdd(localTime, TimeMsToTime(flight * 1000.0));
			carried.pushSpeed = glm::length(vec2(pos) - vec2(carried.body->GetWorldPos())) / flight;
			if(drop > carried.ccUntil) carried.ccUntil = drop;
		}
		LOG("Summon :: %ls carried to (%.0f, %.0f)", carried.name.data(), pos.x, pos.y);
	}
	const vec3 from = npc->pos;
	npc->pos = goal;
	npc->aimPos = pos;
	RunSummonState(*npc, child);
	if(dist > 1.0f && npc->pendingMotion != ActionStateID::INVALID) {
		npc->motionFrom = from;
		npc->motionFromSet = true;
	}
}

static bool NpcKeepsSpeed(const World::ActorNpc& npc)
{
	const SummonInfo* info = GetGameXmlContent().FindSummon((i32)npc.docID);
	return info && info->moveSpeed > 0;
}

void World::RelayWarp(ActorNpc& npc, const vec3& to)
{
	LOG("Summon :: relay robot warps from (%.0f, %.0f) to (%.0f, %.0f)", npc.pos.x, npc.pos.y, to.x, to.y);
	const SummonInfo* info = GetGameXmlContent().FindSummon((i32)npc.docID);
	replication->SendNpcBehaviorToAll(npc.UID, ActionStateID::EXTRAMOTION_2_BEHAVIORSTATE, npc.pos, to, ActorUID::INVALID, info ? info->moveSpeed : 0.0f);
	npc.pos = to;
}

void World::RelayPassive(ActorNpc& npc, const SummonInfo& info, Player& owner)
{
	const i32 state = npc.relayMode * 2 + 1;
	if(state > 20 || info.skillStates[state].empty()) return;
	f32 reach = info.perception;
	bool needsEnemy = false;
	foreach_const(c, info.skillStates[state]) {
		if(c->kind != SummonCommand::REMOTE) continue;
		needsEnemy = true;
		const Remote* r = GetGameXmlContent().FindRemote(RemoteIdx(c->index));
		if(r && r->maxDistance > 0) reach = MIN(reach, r->maxDistance);
	}
	npc.skillTarget = ActorUID::INVALID;
	npc.aimPos = vec3(0);
	if(needsEnemy) {
		Player* target = nullptr;
		f32 best = reach;
		foreach(e, players) {
			if(e->dead || e->team == owner.team) continue;
			const f32 d = glm::length(vec2(e->body->GetWorldPos()) - vec2(npc.pos));
			if(d < best) { best = d; target = &*e; }
		}
		if(!target) return;
		npc.aimPos = target->body->GetWorldPos();
		if(npc.relayMode == RELAY_BRUISER_MODE) npc.skillTarget = target->Main().UID;
	}
	LOG("Summon :: relay robot of %ls passive SKILL_%d", owner.name.data(), state);
	RunSummonState(npc, state);
	npc.skillTarget = ActorUID::INVALID;
}

static const f32 NPC_LEG_RETARGET_DIST = 100.0f;
static const i32 NPC_LEG_MIN_INTERVAL_MS = 100;
static const i32 NPC_FOLLOW_LEG_MIN_INTERVAL_MS = 800;
static const u8 SUMMON_DEFENCE_STAT = 7;
static const u32 NPC_DEAD_NO_KILLER = 0xFFFFFFFF;
static const f32 NPC_WANDER_MIN_RADIUS = 100.0f;
static const f32 NPC_WANDER_MAX_RADIUS = 300.0f;
static const f32 NPC_WANDER_MIN_PAUSE_MS = 250.0f;
static const f32 NPC_WANDER_MAX_PAUSE_MS = 900.0f;

void World::SyncNpcLeg(ActorNpc& npc, const vec3& goal, f32 speed)
{
	const i32 minInterval = npc.ai == SummonInfo::AI_RELAY ? NPC_FOLLOW_LEG_MIN_INTERVAL_MS : NPC_LEG_MIN_INTERVAL_MS;
	const bool retarget = npc.legMoving && glm::length(vec2(goal) - vec2(npc.legGoal)) > NPC_LEG_RETARGET_DIST && localTime >= TimeAdd(npc.legAt, TimeMsToTime(minInterval));
	if(npc.legMoving && !retarget) return;
	npc.legMoving = true;
	npc.legGoal = goal;
	npc.legAt = localTime;
	npc.legSpeed = speed;
	replication->SendNpcLegStartToAll(npc.UID, npc.pos, goal, speed);
	replication->SendNpcTurnToAll(npc.UID, atan2f(goal.y - npc.pos.y, goal.x - npc.pos.x));
}

void World::WanderNpc(ActorNpc& npc, f32 moveSpeed, f32 delta)
{
	if(!npc.legMoving) {
		if(localTime < npc.wanderResumeAt) return;
		const f32 angle = (f32)(RandUint() % 3600) * PI / 1800.0f;
		const f32 radius = NPC_WANDER_MIN_RADIUS + (f32)(RandUint() % 1000) / 1000.0f * (NPC_WANDER_MAX_RADIUS - NPC_WANDER_MIN_RADIUS);
		SyncNpcLeg(npc, vec3(vec2(npc.home) + vec2(cosf(angle), sinf(angle)) * radius, npc.pos.z), moveSpeed);
		return;
	}
	const vec2 rel = vec2(npc.legGoal) - vec2(npc.pos);
	const f32 dist = glm::length(rel);
	const f32 step = moveSpeed * delta;
	if(dist <= step) {
		npc.pos = vec3(vec2(npc.legGoal), npc.pos.z);
		StopNpcLeg(npc);
		const f32 pause = NPC_WANDER_MIN_PAUSE_MS + (f32)(RandUint() % 1000) / 1000.0f * (NPC_WANDER_MAX_PAUSE_MS - NPC_WANDER_MIN_PAUSE_MS);
		npc.wanderResumeAt = TimeAdd(localTime, TimeMsToTime(pause));
		return;
	}
	npc.pos += vec3(rel / dist * step, 0);
	npc.rot.z = atan2f(rel.y, rel.x);
}

void World::StopNpcLeg(ActorNpc& npc)
{
	if(!npc.legMoving) return;
	npc.legMoving = false;
	replication->SendNpcLegStopToAll(npc.UID, npc.pos, npc.rot.z);
}

void World::UpdateSummonAttack(ActorNpc& npc, f32 delta)
{
	const SummonInfo* info = GetGameXmlContent().FindSummon((i32)npc.docID);
	if(!info || npc.spawnedAt >= localTime) return;
	const Player& owner = players[npc.ownerIndex];
	ActorMaster* tm = FindMasterActor(npc.chaseTarget);
	Player* target = tm && !tm->parent->dead && tm->parent->team != owner.team ? tm->parent : nullptr;
	const f32 lost = info->targetLost > 0 ? info->targetLost : info->perception;
	if(target && glm::length(vec2(target->body->GetWorldPos()) - vec2(npc.pos)) > lost) target = nullptr;
	if(!target) {
		f32 best = info->perception;
		foreach(e, players) {
			if(e->dead || e->team == owner.team) continue;
			const f32 d = glm::length(vec2(e->body->GetWorldPos()) - vec2(npc.pos));
			if(d < best) { best = d; target = &*e; }
		}
	}
	ActorNpc* targetNpc = nullptr;
	if(!target) {
		f32 best = info->perception;
		foreach(e, actorNpcList) {
			if(&*e == &npc || !NpcHittableBy(*e, owner)) continue;
			const f32 d = glm::length(vec2(e->pos) - vec2(npc.pos));
			if(d < best) { best = d; targetNpc = &*e; }
		}
	}
	const ActorUID chosen = target ? target->Main().UID : (targetNpc ? targetNpc->UID : ActorUID::INVALID);
	if(chosen != npc.chaseTarget) {
		replication->SendAiSetTargetToAll(npc.UID, chosen);
		npc.chaseTarget = chosen;
	}
	if(!target && !targetNpc) { StopNpcLeg(npc); return; }
	if(npc.standAt != Time::ZERO && localTime < npc.standAt) return;
	const vec3 goal = target ? target->body->GetWorldPos() : targetNpc->pos;
	const vec2 rel = vec2(goal) - vec2(npc.pos);
	const f32 dist = glm::length(rel);
	const f32 yaw = dist > 1.0f ? atan2f(rel.y, rel.x) : npc.rot.z;
	if(dist > npc.attackRule->attackRange) {
		if(info->moveSpeed <= 0) return;
		const f32 step = MIN(info->moveSpeed * delta, dist - npc.attackRule->attackRange);
		SyncNpcLeg(npc, vec3(vec2(goal) - rel / dist * npc.attackRule->attackRange, goal.z), info->moveSpeed);
		npc.pos = vec3(vec2(npc.pos) + rel / dist * step, npc.pos.z);
		npc.rot.z = yaw;
		return;
	}
	StopNpcLeg(npc);
	if(localTime < npc.nextAttack) return;
	npc.nextAttack = TimeAdd(localTime, TimeMsToTime(npc.attackRule->cooldownMs));
	npc.rot.z = yaw;
	npc.skillTarget = chosen;
	npc.aimPos = goal;
	npc.actionState = ActionStateID::ATTACK_BEHAVIORSTATE;
	replication->SendAiActionStateToAll(npc.UID, ActionStateID::ATTACK_BEHAVIORSTATE, yaw, npc.pos, info->moveSpeed > 0);
	RunSummonState(npc, 0);
	const f32 animSec = MAX(GetGameXmlContent().GetAniSeqTime(info->className.c_str(), info->stateAnim[0]), SummonPassStart(*info, 0, 8));
	if(animSec > 0) npc.standAt = TimeAdd(localTime, TimeMsToTime(animSec * 1000.0));
	LOG("Summon :: creature=%d attacks %s %d at %.0f", (i32)npc.docID, target ? "master" : "creature", target ? (i32)target->index : (i32)targetNpc->docID, dist);
}

void World::UpdateSummonAi(f32 delta)
{
	const auto& content = GetGameXmlContent();
	foreach(it, actorNpcList) {
		ActorNpc& npc = *it;
		if(npc.pendingMotion != ActionStateID::INVALID && localTime >= npc.motionAt) {
			const SummonInfo* motionInfo = content.FindSummon((i32)npc.docID);
			replication->SendNpcBehaviorToAll(npc.UID, npc.pendingMotion, npc.motionFromSet ? npc.motionFrom : npc.pos, npc.motionTarget, npc.motionTargetUID, motionInfo ? motionInfo->moveSpeed : 0.0f);
			npc.pendingMotion = ActionStateID::INVALID;
			npc.motionFromSet = false;
		}
		if(npc.queuedState >= 0 && localTime >= npc.queuedStateAt && npc.pendingMotion == ActionStateID::INVALID) {
			if(glm::length(vec2(npc.queuedGoal)) > 1.0f) {
				replication->SendNpcMoveToAll(npc.UID, npc.pos, npc.queuedGoal, atan2f(npc.queuedGoal.y - npc.pos.y, npc.queuedGoal.x - npc.pos.x), RELAY_LEAP_SPEED);
				npc.leapEndAt = TimeAdd(localTime, TimeMsToTime(glm::length(vec2(npc.queuedGoal) - vec2(npc.pos)) / RELAY_LEAP_SPEED * 1000.0));
				npc.pos = npc.queuedGoal;
			}
			const i32 state = npc.queuedState;
			npc.queuedState = -1;
			RunSummonState(npc, state);
		}
		if(npc.spawnedAt >= localTime) continue;
		if(npc.attackRule && npc.removeAt == Time::ZERO && (npc.despawnAt == Time::ZERO || localTime < npc.despawnAt)) {
			UpdateSummonAttack(npc, delta);
			continue;
		}
		if(npc.ai == SummonInfo::AI_NONE) continue;
		if(npc.despawnAt != Time::ZERO && localTime >= npc.despawnAt) continue;
		const SummonInfo* info = content.FindSummon((i32)npc.docID);
		if(!info) continue;
		Player& owner = players[npc.ownerIndex];
		vec3 goal = npc.pos;
		f32 stopDist = 0;

		if(npc.ai == SummonInfo::AI_RELAY) {
			if(owner.dead || (i32)owner.Main().classType != npc.ownerClass) {
				LOG("Summon :: relay robot of %ls leaves with its master", owner.name.data());
				npc.despawnAt = localTime;
				continue;
			}
			if(npc.skillEndAt != Time::ZERO && localTime < npc.skillEndAt) {
				StopNpcLeg(npc);
				continue;
			}
			goal = owner.body->GetWorldPos();
			stopDist = info->aiDistance;
			if(info->perception > 0 && glm::length(vec2(goal) - vec2(npc.pos)) > info->perception) RelayWarp(npc, goal);
			if(npc.relayMode >= 0 && localTime >= npc.nextPassive && localTime >= npc.passiveBlockedUntil) {
				npc.nextPassive = TimeAdd(localTime, TimeMsToTime(RELAY_PASSIVE_SEC * 1000.0));
				RelayPassive(npc, *info, owner);
			}
		}
		else {
			if(npc.standAt != Time::ZERO && localTime < npc.standAt) continue;
			ActorMaster* tm = FindMasterActor(npc.chaseTarget);
			Player* target = tm ? tm->parent : nullptr;
			const ActorNpc* npcTarget = target ? nullptr : FindNpcActor(npc.chaseTarget);
			if(npcTarget && (npcTarget->hp <= 0 || npcTarget->removeAt != Time::ZERO)) npcTarget = nullptr;
			if(npc.stuck) {
				if((!target || target->dead) && !npcTarget) {
					npc.stuck = false;
					if(npc.chaseTarget != ActorUID::INVALID) {
						replication->SendAiSetTargetToAll(npc.UID, ActorUID::INVALID);
						replication->SendBattleStateToAll(npc.UID, false, info->normalMoveSpeed);
					}
					npc.chaseTarget = ActorUID::INVALID;
					continue;
				}
				npc.legMoving = false;
				if(npc.pendingMotion == ActionStateID::INVALID) npc.pos = target ? target->body->GetWorldPos() : npcTarget->pos;
				continue;
			}
			const f32 keepRange = info->targetLost > 0 ? info->targetLost : info->perception;
			const bool lost = target ? (target->dead || glm::length(vec2(target->body->GetWorldPos()) - vec2(npc.pos)) > keepRange) : (!npcTarget || glm::length(vec2(npcTarget->pos) - vec2(npc.pos)) > keepRange);
			if(lost) {
				target = nullptr;
				npcTarget = nullptr;
				f32 best = info->perception;
				ActorUID chosen = ActorUID::INVALID;
				foreach(e, players) {
					if(e->dead || e->team == owner.team) continue;
					bool taken = false;
					foreach_const(o, actorNpcList) {
						if(&*o != &npc && o->ownerIndex == npc.ownerIndex && o->docID == npc.docID && o->chaseTarget == e->Main().UID) taken = true;
					}
					if(taken) continue;
					const f32 d = glm::length(vec2(e->body->GetWorldPos()) - vec2(npc.pos));
					if(d < best) { best = d; target = &*e; chosen = e->Main().UID; }
				}
				foreach_const(o, actorNpcList) {
					if(&*o == &npc || o->hp <= 0 || o->removeAt != Time::ZERO || !NpcHittableBy(*o, owner)) continue;
					const f32 d = glm::length(vec2(o->pos) - vec2(npc.pos));
					if(d < best) { best = d; target = nullptr; npcTarget = &*o; chosen = o->UID; }
				}
				if(chosen != npc.chaseTarget) {
					if(chosen != ActorUID::INVALID) StopNpcLeg(npc);
					replication->SendAiSetTargetToAll(npc.UID, chosen);
					if((chosen != ActorUID::INVALID) != (npc.chaseTarget != ActorUID::INVALID)) replication->SendBattleStateToAll(npc.UID, chosen != ActorUID::INVALID, chosen != ActorUID::INVALID ? info->moveSpeed : info->normalMoveSpeed);
					if(chosen != ActorUID::INVALID && (npc.extraMotion == 2 || npc.extraMotion == 3)) PlaySummonExtraMotion(npc, *info, 1);
					else if(chosen == ActorUID::INVALID && npc.extraMotion == 1) PlaySummonExtraMotion(npc, *info, 3);
				}
				npc.chaseTarget = chosen;
			}
			if(!target && !npcTarget) {
				if(npc.extraMotion == 0 && localTime >= npc.spawnedAt) PlaySummonExtraMotion(npc, *info, 2);
				if(info->moveSpeed > 0 && info->normalMoveSpeed > 0) WanderNpc(npc, info->normalMoveSpeed, delta);
				else StopNpcLeg(npc);
				continue;
			}
			goal = target ? target->body->GetWorldPos() : npcTarget->pos;
			const f32 d = glm::length(vec2(goal) - vec2(npc.pos));
			if(d <= WEAPON_TARGET_RADIUS && !info->skillStates[1].empty()) {
				LOG("Summon :: creature=%d reached its target", (i32)npc.docID);
				RunSummonState(npc, 1);
				if(npc.despawnAt == Time::ZERO || npc.despawnAt > localTime) npc.stuck = true;
				continue;
			}
		}

		if(info->moveSpeed <= 0) continue;
		const vec2 rel = vec2(goal) - vec2(npc.pos);
		const f32 dist = glm::length(rel);
		if(dist <= stopDist + 1.0f || (!npc.legMoving && dist <= stopDist + NPC_LEG_RETARGET_DIST)) { StopNpcLeg(npc); continue; }
		const vec2 dir = rel / dist;
		f32 chaseSpeed = info->moveSpeed;
		if(const SummonChaseSpeedRule* chase = npc.ai == SummonInfo::AI_RELAY ? nullptr : FindSummonChaseSpeedRule((i32)npc.docID)) {
			const bool fresh = TimeDiffSec(TimeDiff(npc.spawnedAt, localTime)) < chase->spawnSec;
			chaseSpeed = fresh || dist - stopDist >= chase->nearDist ? chase->farSpeed : chase->nearSpeed;
		}
		SyncNpcLeg(npc, vec3(vec2(goal) - dir * stopDist, goal.z), chaseSpeed);
		const vec2 legRel = vec2(npc.legGoal) - vec2(npc.pos);
		const f32 legDist = glm::length(legRel);
		if(legDist > 1.0f) {
			const vec2 legDir = legRel / legDist;
			const f32 legStep = (npc.legSpeed > 0 ? npc.legSpeed : info->moveSpeed) * delta;
			npc.pos = vec3(vec2(npc.pos) + legDir * MIN(legStep, legDist), goal.z);
			npc.rot.z = atan2f(legDir.y, legDir.x);
			if(legStep >= legDist) StopNpcLeg(npc);
		}
		else StopNpcLeg(npc);
	}
}

void World::EnforceSummonLimit(const Player& owner, u32 ownerIndex, i32 creature)
{
	i32 limit = 0;
	i32 groupMin = creature;
	i32 groupMax = creature;
	foreach_const(st, activeStatuses) {
		if(st->targetIndex != owner.index || !st->info || localTime >= st->until) continue;
		foreach_const(l, st->info->summonLimits) if(l->first == creature) limit = l->second;
	}
	for(const SummonGroupLimitRule& rule : MEASURED_SUMMON_GROUP_LIMITS) {
		if(rule.master != owner.Main().classType || creature < rule.creatureMin || creature > rule.creatureMax) continue;
		limit = rule.limit;
		groupMin = rule.creatureMin;
		groupMax = rule.creatureMax;
	}
	if(limit <= 0) return;
	i32 alive = 0;
	ActorNpc* oldest = nullptr;
	foreach(npc, actorNpcList) {
		if(npc->ownerIndex != ownerIndex || (i32)npc->docID < groupMin || (i32)npc->docID > groupMax || npc->removeAt != Time::ZERO || (npc->despawnAt != Time::ZERO && localTime >= npc->despawnAt)) continue;
		alive++;
		if(!oldest || npc->spawnedAt < oldest->spawnedAt) oldest = &*npc;
	}
	if(alive >= limit && oldest) {
		LOG("Summon :: creature=%d limit %d reached, oldest removed", creature, limit);
		oldest->hp = 0;
		oldest->despawnAt = localTime;
	}
}

void World::PlaySummonExtraMotion(ActorNpc& npc, const SummonInfo& info, i32 motion)
{
	if(motion < 1 || motion > 3 || info.extraMotions[motion - 1].empty()) return;
	static const ActionStateID MOTIONS[3] = { ActionStateID::EXTRAMOTION_1_BEHAVIORSTATE, ActionStateID::EXTRAMOTION_2_BEHAVIORSTATE, ActionStateID::EXTRAMOTION_3_BEHAVIORSTATE };
	npc.extraMotion = (u8)motion;
	replication->SendMonsterStateToAll(npc.UID, MOTIONS[motion - 1], npc.pos, 0.0f);
	foreach_const(c, info.extraMotions[motion - 1]) {
		if(pendingSummonCommands.full()) continue;
		PendingSummonCommand pc;
		pc.time = TimeAdd(localTime, TimeMsToTime(c->delay * 1000.0));
		pc.remove = c->remove;
		pc.ownerIndex = npc.ownerIndex;
		pc.kind = SummonCommand::STATUS;
		pc.index = c->index;
		pc.pos = npc.pos;
		pc.yaw = npc.rot.z;
		pc.skill = 0;
		pc.source = npc.UID;
		pc.until = npc.despawnAt;
		pendingSummonCommands.push_back(pc);
	}
	LOG("Summon :: creature=%d plays extra motion %d", (i32)npc.docID, motion);
}

void World::SpawnPendingSummons()
{
	foreach(npc, actorNpcList) {
		if(npc->drainPerTick <= 0 || npc->hp <= 0 || localTime < npc->nextDrainAt || localTime >= npc->despawnAt) continue;
		npc->nextDrainAt = TimeAdd(npc->nextDrainAt, TimeMsToTime(npc->drainInterval * 1000.0));
		npc->hp = MAX(npc->hp - npc->drainPerTick, 0);
		replication->SendHpToAll(npc->UID, npc->hp, npc->hpMax);
		if(npc->hp <= 0) npc->despawnAt = localTime;
	}
	for(auto ps = pendingSummons.begin(); ps != pendingSummons.end(); ) {
		if(!ps->remove || localTime < ps->time) { ++ps; continue; }
		foreach(npc, actorNpcList) {
			if(npc->ownerIndex != ps->ownerIndex || npc->ownerUID == ActorUID::INVALID || (i32)npc->docID != ps->creature || localTime >= npc->despawnAt) continue;
			npc->despawnAt = localTime;
			LOG("Summon :: %ls removes creature=%d", players[ps->ownerIndex].name.data(), ps->creature);
		}
		ps = pendingSummons.erase_unsorted(ps);
	}
	for(auto ps = pendingSummons.begin(); ps != pendingSummons.end(); ) {
		if(localTime < ps->time) { ++ps; continue; }
		Player& owner = players[ps->ownerIndex];
		const SummonInfo* info = GetGameXmlContent().FindSummon(ps->creature);
		if(!owner.dead) {
			const bool relay = info && info->ai == SummonInfo::AI_RELAY;
			ActorNpc* reuse = nullptr;
			if(relay) {
				foreach(other, actorNpcList) {
					if(other->ownerIndex == ps->ownerIndex && other->ai == SummonInfo::AI_RELAY && localTime < other->despawnAt) reuse = &*other;
				}
			}
			if(reuse && (reuse->queuedState >= 0 || localTime < reuse->leapEndAt)) {
				if(ps->childSkill == 0 && owner.pendingChildSkill > 0 && localTime < owner.pendingChildSkillUntil) {
					ps->childSkill = owner.pendingChildSkill;
					ps->childPos = owner.pendingChildPos;
					ps->childTarget = owner.pendingChildTarget;
					owner.pendingChildSkill = 0;
				}
				ps->time = TimeAdd(reuse->queuedState >= 0 && localTime < reuse->queuedStateAt ? reuse->queuedStateAt : (localTime < reuse->leapEndAt ? reuse->leapEndAt : localTime), TimeMsToTime(50));
				LOG("Summon :: relay robot of %ls in flight, next order at landing", owner.name.data());
				++ps;
				continue;
			}
			if(!reuse) EnforceSummonLimit(owner, ps->ownerIndex, ps->creature);
			ActorNpc& actor = reuse ? *reuse : SpawnNpcActor(CreatureIndex(ps->creature), nextSummonLocalID++);
			const f32 life = relay ? RELAY_LIFE_SEC : info && info->lifeTime > 0 ? info->lifeTime : 5.0f;
			if(!reuse) {
				actor.pos = ps->pos;
				actor.home = ps->pos;
				actor.rot = ps->rot;
				actor.faction = owner.team == 0 ? Faction::RED : Faction::BLUE;
				actor.ownerUID = owner.Main().UID;
				actor.seed = (i32)RandUint();
				actor.despawnAt = TimeAdd(localTime, TimeMsToTime(life * 1000.0));
				actor.ownerIndex = ps->ownerIndex;
				actor.ai = info ? (u8)info->ai : 0;
				actor.ownerClass = (i32)owner.Main().classType;
				for(const auto& w : WALL_FOOTPRINTS) {
					if(w.creature != ps->creature) continue;
					actor.wall = physics.CreateWall(actor.pos, w.width, w.depth, WALL_HEIGHT, actor.rot.z, (i32)owner.team);
					LOG("Summon :: creature=%d is a wall %.0fx%.0f", ps->creature, w.width, w.depth);
				}
				actor.playerSummon = info && (info->targetable || actor.wall) && !info->aiTargetProof;
				if(info && (info->targetable || actor.wall || OneHitSummon(ps->creature))) {
					foreach_const(st, info->baseStats) {
						if(st->type == (u8)StatType::STAT_TYPE_HEALTH) actor.hp = actor.hpMax = (i32)st->value;
						if(st->type == SUMMON_DEFENCE_STAT) actor.defence = st->value;
					}
				}
				actor.nextPassive = TimeAdd(localTime, TimeMsToTime(RELAY_PASSIVE_SEC * 1000.0));
				if(actor.actionState == ActionStateID::INVALID) actor.actionState = ActionStateID::SPAWN_BEHAVIORSTATE;
				const f32 spawnSec = info && info->spawnAnim != 0 ? GetGameXmlContent().GetAniSeqTime(info->className.c_str(), info->spawnAnim) : 0.0f;
				if(spawnSec < life) actor.standAt = TimeAdd(localTime, TimeMsToTime(MAX(spawnSec, 0.0f) * 1000.0));
				actor.attackRule = info ? FindSummonAttackRule(info->className.c_str()) : nullptr;
				if(actor.attackRule) actor.nextAttack = TimeAdd(localTime, TimeMsToTime(actor.attackRule->firstDelayMs));
				if(info && info->spawnInteraction != 0) AddCreatureInteraction(actor, info->spawnInteraction, owner.team);
			}
			actor.skillTarget = ps->target;
			owner.lastChildUID = actor.UID;
			if(reuse && info->perception > 0 && glm::length(vec2(ps->pos) - vec2(actor.pos)) > info->perception) RelayWarp(actor, ps->pos);
			const bool carried = ps->childSkill > 0;
			const i32 child = carried ? ps->childSkill : (owner.pendingChildSkill > 0 && localTime < owner.pendingChildSkillUntil ? owner.pendingChildSkill : 0);
			const vec3 childPos = carried ? ps->childPos : owner.pendingChildPos;
			const ActorUID childTarget = carried ? ps->childTarget : owner.pendingChildTarget;
			if(info && child > 0) {
				const i32 modeState = child + RELAY_MODE_STATE_OFFSET;
				if(info->ai == SummonInfo::AI_RELAY && modeState <= 20 && !info->skillStates[modeState].empty()) {
					const i32 mode = (child - 1) / RELAY_IDS_PER_MODE;
					const i32 activeState = mode * 2 + 2;
					LOG("Summon :: %ls relay mode SKILL_%d then active SKILL_%d%s", owner.name.data(), modeState, activeState, reuse ? " (same robot)" : "");
					actor.relayMode = mode;
					actor.nextPassive = TimeAdd(localTime, TimeMsToTime(RELAY_PASSIVE_SEC * 1000.0));
					if(mode == RELAY_PRISM_MODE) {
						const StatusInfo* hide = GetGameXmlContent().FindStatus(RELAY_STEALTH_STATUS);
						actor.passiveBlockedUntil = TimeAdd(localTime, TimeMsToTime((hide ? hide->durationSec : 0.0f) * 1000.0));
					}
					RunSummonState(actor, modeState);
					if(glm::length(vec2(childPos)) > 1.0f && actor.pendingMotion != ActionStateID::INVALID) {
						actor.motionTarget = childPos;
						actor.motionTargetUID = ActorUID::INVALID;
					}
					actor.aimPos = childPos;
					actor.queuedGoal = mode != RELAY_ARTILLERY_MODE && glm::length(vec2(childPos)) > 1.0f ? vec3(vec2(childPos), actor.pos.z) : vec3(0);
					const f32 modeSec = GetGameXmlContent().GetAniSeqTime(info->className.c_str(), info->stateAnim[modeState]);
					actor.queuedState = activeState;
					actor.queuedStateAt = TimeAdd(localTime, TimeMsToTime((f64)modeSec * 1000.0 + 60.0));
					LOG("Summon :: relay mode animation %d lasts %.2fs", info->stateAnim[modeState], modeSec);
				}
				else {
					LOG("Summon :: %ls relay state SKILL_%d", owner.name.data(), child);
					if(childTarget != ActorUID::INVALID) actor.skillTarget = childTarget;
					RunSummonState(actor, child);
				}
				if(!carried) owner.pendingChildSkill = 0;
			}
			if(info && !reuse) {
				foreach_const(c, info->onSpawn) {
					if(c->kind == SummonCommand::DIE) {
						actor.dieMode = c->dieMode;
						continue;
					}
					if(!pendingSummonCommands.full()) {
						PendingSummonCommand pc;
						pc.time = TimeAdd(localTime, TimeMsToTime(c->delay * 1000.0));
						pc.ownerIndex = ps->ownerIndex;
						pc.kind = c->kind;
						pc.index = c->index;
						pc.nodeName = c->nodeName;
						pc.pos = ps->pos;
						pc.yaw = ps->rot.z;
						pc.source = actor.UID;
						pc.skill = info->skills.empty() ? 0 : info->skills[0];
						pc.until = actor.despawnAt;
						pc.aniEnd = c->aniEnd;
						pendingSummonCommands.push_back(pc);
					}
				}
			}
			LOG("Summon :: %ls creature=%d at (%.0f, %.0f) life=%.1fs known=%d", owner.name.data(), ps->creature, ps->pos.x, ps->pos.y, life, info != nullptr);
			if(Config().DevHideDetectTest > 0 && info && info->hideDetectRadius > 0 && !reuse) {
				Player* nearest = nullptr;
				f32 best = 0;
				foreach(e, players) {
					if(e->team == owner.team || e->dead) continue;
					const f32 d = glm::length(vec2(e->body->GetWorldPos()) - vec2(ps->pos));
					if(!nearest || d < best) { nearest = &*e; best = d; }
				}
				if(nearest) {
					LOG("Hide :: dev test hides %ls at %.0f from detector", nearest->name.data(), best);
					ApplyStatus(*nearest, *nearest, Config().DevHideDetectTest);
				}
			}
		}
		ps = pendingSummons.erase_unsorted(ps);
	}
}

void World::RunPendingSummonCommands()
{
	for(auto pc = pendingSummonCommands.begin(); pc != pendingSummonCommands.end(); ) {
		if(localTime < pc->time) { ++pc; continue; }
		if(pc->source != ActorUID::INVALID) {
			const ActorNpc* fresh = FindNpcActor(pc->source);
			if(fresh && fresh->spawnedAt >= localTime) { ++pc; continue; }
		}
		Player& owner = players[pc->ownerIndex];
		if(pc->kind == SummonCommand::REMOTE) {
			if(!pendingRemotes.full()) {
				PendingRemote pr;
				pr.time = localTime;
				pr.idx = RemoteIdx(pc->index);
				pr.preset = (i32)ActionCommand::TargetPreset::POS_FFF;
				const Remote* summonRemote = GetGameXmlContent().FindRemote(RemoteIdx(pc->index));
				const bool aimedGraph = summonRemote && summonRemote->behaviorType == Remote::BehaviorType::GRAPH && glm::length(vec2(pc->aim)) > 1.0f;
				pr.offsetSide = 0;
				pr.offsetFront = 0;
				pr.ownerIndex = pc->ownerIndex;
				pr.skillID = pc->skill != 0 ? SkillID(pc->skill) : SkillID::INVALID;
				pr.castID = nextCastID++;
				const ActorNpc* sourceNpc = pc->source != ActorUID::INVALID ? FindNpcActor(pc->source) : nullptr;
				const SummonInfo* sourceInfo = sourceNpc ? GetGameXmlContent().FindSummon((i32)sourceNpc->docID) : nullptr;
				f32 sourceAttack = 0;
				if(sourceInfo) foreach_const(st, sourceInfo->baseStats) if(st->type == STAT_ATTACK) sourceAttack = st->value;
				if(summonRemote && summonRemote->skillIndex != 0) pr.damage = (i32)PlayerSkillDamage(owner, SkillID(summonRemote->skillIndex));
				else if(summonRemote && sourceAttack > 0) pr.damage = (i32)sourceAttack;
				else pr.damage = pc->skill != 0 ? (i32)PlayerSkillDamage(owner, SkillID(pc->skill)) : 0;
				pr.castPos = aimedGraph ? pc->aim : pc->pos;
				pr.angle = aimedGraph ? atan2f(pc->aim.y - pc->pos.y, pc->aim.x - pc->pos.x) : pc->yaw;
				if(aimedGraph) pr.preset = (i32)ActionCommand::TargetPreset::SELF_FFF;
				pr.target = pc->target;
				pr.sourceUID = pc->source;
				pr.nodeName = pc->nodeName;
				const bool dieRemoved = sourceInfo && sourceNpc->hp > 0 && sourceNpc->removeAt == Time::ZERO && eastl::find(sourceInfo->dieRemovedRemotes.begin(), sourceInfo->dieRemovedRemotes.end(), pc->index) != sourceInfo->dieRemovedRemotes.end();
				pr.endWithSource = (pc->aniEnd || dieRemoved) && pc->source != ActorUID::INVALID;
				pendingRemotes.push_back(pr);
			}
		}
		else if(pc->remove) {
			const StatusInfo* removed = GetGameXmlContent().FindStatus(pc->index);
			if(pc->source != ActorUID::INVALID && FindNpcActor(pc->source) && !(removed && SummonStatusClientOnly(*removed))) {
				foreach(ns, npcStatuses) if(ns->npcUID == pc->source && ns->statusID == pc->index) ns->until = localTime;
				replication->SendRemoveStatusToAll(pc->index, pc->source, pc->source);
			}
		}
		else {
			const StatusInfo* st = GetGameXmlContent().FindStatus(pc->index);
			const ActorNpc* source = pc->source != ActorUID::INVALID ? FindNpcActor(pc->source) : nullptr;
			if(st && source && (localTime < source->despawnAt || source->removeAt != Time::ZERO) && !SummonStatusClientOnly(*st)) {
				replication->SendAddStatusToAll(pc->index, source->UID, source->UID, StatusSentDuration(*st), source->pos, source->pos, 0, 1, 1.0f, -1.0f, false);
				TrackNpcSelfStatus(source->UID, *st);
			}
			if(st && st->auraStatus != 0 && !groundAuras.full()) {
				GroundAura ga;
				ga.ownerIndex = pc->ownerIndex;
				ga.auraStatus = st->ID;
				ga.range = st->targetingRange;
				ga.pos = pc->pos;
				const Time statusEnd = st->durationSec > 0 ? TimeAdd(localTime, TimeMsToTime(st->durationSec * 1000.0)) : pc->until;
				ga.until = pc->until != Time::ZERO && pc->until < statusEnd ? pc->until : statusEnd;
				ga.nextPulse = localTime;
				ga.sourceUID = pc->source;
				groundAuras.push_back(ga);
				LOG("Summon :: %ls aura %d (gives %d statuses within %.0f%s)", owner.name.data(), pc->index, (i32)st->auraStatuses.size(), st->targetingRange, st->auraEnemies ? ", enemies" : "");
			}
		}
		pc = pendingSummonCommands.erase_unsorted(pc);
	}
}

void World::PulseGroundAuras()
{
	for(auto ga = groundAuras.begin(); ga != groundAuras.end(); ) {
		if(localTime >= ga->until) {
			if(const StatusInfo* aura = GetGameXmlContent().FindStatus(ga->auraStatus)) EndAura(ga->ownerIndex, *aura, ga->sourceUID);
			ga = groundAuras.erase_unsorted(ga);
			continue;
		}
		if(localTime >= ga->nextPulse) {
			ga->nextPulse = TimeAdd(localTime, TimeMsToTime(500));
			const StatusInfo* aura = GetGameXmlContent().FindStatus(ga->auraStatus);
			if(aura && ga->range > 0) PulseAura(ga->ownerIndex, *aura, vec2(ga->pos), ga->range, -1, ga->sourceUID);
		}
		++ga;
	}
}

void World::DespawnSummons()
{
	for(auto it = actorNpcList.begin(); it != actorNpcList.end(); ) {
		if(it->removeAt != Time::ZERO) {
			if(it->dieStandAt != Time::ZERO && localTime >= it->dieStandAt) {
				it->dieStandAt = Time::ZERO;
				replication->SendAiActionStateToAll(it->UID, ActionStateID::NONE_BEHAVIORSTATE, it->rot.z, it->pos, NpcKeepsSpeed(*it));
			}
			if(localTime >= it->removeAt) {
				physics.RemoveWall(it->bodyBlocker);
				actorNpcMap.erase(it->UID);
				it = actorNpcList.erase(it);
			}
			else ++it;
			continue;
		}
		if(it->ownerUID == ActorUID::INVALID && it->actionState == ActionStateID::SPAWN_BEHAVIORSTATE && it->stateEndAt != Time::ZERO && localTime >= it->stateEndAt) {
			it->stateEndAt = Time::ZERO;
			it->actionState = ActionStateID::NONE_BEHAVIORSTATE;
			replication->SendMonsterStateToAll(it->UID, ActionStateID::NONE_BEHAVIORSTATE, it->pos, it->monsterAi ? it->monsterAi->run * it->speedMul : 0.0f);
		}
		if(it->skillEndAt != Time::ZERO && localTime >= it->skillEndAt) {
			it->skillEndAt = Time::ZERO;
			if(it->actionState >= ActionStateID::SKILL_1_BEHAVIORSTATE && it->actionState <= ActionStateID::SKILL_30_BEHAVIORSTATE && (it->hp > 0 || it->hpMax <= 0)) {
				it->actionState = ActionStateID::NONE_BEHAVIORSTATE;
				replication->SendAiActionStateToAll(it->UID, ActionStateID::NONE_BEHAVIORSTATE, it->rot.z, it->pos, NpcKeepsSpeed(*it));
			}
		}
		if(it->standAt != Time::ZERO && localTime >= it->standAt) {
			it->standAt = Time::ZERO;
			if(it->actionState == ActionStateID::SPAWN_BEHAVIORSTATE || it->actionState == ActionStateID::ATTACK_BEHAVIORSTATE) {
				it->actionState = ActionStateID::NONE_BEHAVIORSTATE;
				replication->SendAiActionStateToAll(it->UID, ActionStateID::NONE_BEHAVIORSTATE, it->rot.z, it->pos, NpcKeepsSpeed(*it));
			}
		}
		if(it->despawnAt != Time::ZERO && localTime >= it->despawnAt) {
			LOG("Summon :: creature=%d despawned", (i32)it->docID);
			if(it->hp <= 0) {
				const SummonInfo* dying = GetGameXmlContent().FindSummon((i32)it->docID);
				if(dying && dying->publicDrop != 0) SpawnDropGroupAt(dying->publicDrop, it->pos, -1);
			}
			physics.RemoveWall(it->wall);
			it->wall = nullptr;
			const SummonInfo* info = GetGameXmlContent().FindSummon((i32)it->docID);
			if(info && info->clearComboOnDeath != 0 && it->ownerUID != ActorUID::INVALID) {
				Player& owner = players[it->ownerIndex];
				if(owner.comboSlot >= 0 && (i32)owner.comboSkill == info->clearComboOnDeath) {
					replication->SendChangeSkillSlot(owner.clientHd, owner.Main().UID, owner.comboSlot, owner.comboBase, 0);
					LOG("Combo :: %ls combo %d cleared by the death of creature %d", owner.name.data(), info->clearComboOnDeath, (i32)it->docID);
					owner.comboSlot = -1;
					owner.comboAutoAt = Time::ZERO;
					owner.comboPending = SkillID::INVALID;
				}
			}
			if(info) {
				u8 wanted = 0;
				foreach_const(c, info->onDie) if(c->dieMode == it->dieMode) wanted = it->dieMode;
				if(!info->onDie.empty()) LOG("Summon :: creature=%d dies in mode %d (own %d) with %d die commands", (i32)it->docID, (i32)wanted, (i32)it->dieMode, (i32)info->onDie.size());
				foreach_const(c, info->onDie) {
					if(c->dieMode != wanted) continue;
					if(!pendingSummonCommands.full()) {
						PendingSummonCommand pc;
						pc.time = TimeAdd(localTime, TimeMsToTime(c->delay * 1000.0));
						pc.ownerIndex = it->ownerIndex;
						pc.kind = c->kind;
						pc.index = c->index;
						pc.nodeName = c->nodeName;
						pc.pos = it->pos;
						pc.yaw = it->rot.z;
						pc.skill = info->skills.empty() ? 0 : info->skills[0];
						pc.until = Time::ZERO;
						if(it->ownerUID != ActorUID::INVALID) pc.source = it->UID;
						pendingSummonCommands.push_back(pc);
					}
				}
			}
			if(it->ownerUID != ActorUID::INVALID) {
				if(it->hp > 0) {
					it->hp = 0;
					replication->SendHpToAll(it->UID, 0, it->hpMax);
				}
				it->actionState = ActionStateID::DIE_BEHAVIORSTATE;
				replication->SendAiActionStateToAll(it->UID, ActionStateID::DIE_BEHAVIORSTATE, it->rot.z, it->pos, NpcKeepsSpeed(*it));
				const ActorMaster* killer = it->killerUID != ActorUID::INVALID ? FindMasterActor(it->killerUID) : nullptr;
				replication->SendNpcDeadAckToAll(it->UID, killer ? it->killerUID : ActorUID::INVALID, killer ? (u32)(100000000 + (i32)killer->classType) : NPC_DEAD_NO_KILLER);
				it->dieStandAt = TimeAdd(localTime, TimeMsToTime(SUMMON_DIE_STAND_MS));
				it->removeAt = TimeAdd(localTime, TimeMsToTime(MAX((f64)(info ? info->destroyTime : 0.0f) * 1000.0, SUMMON_DIE_STAND_MS)));
				++it;
				continue;
			}
			physics.RemoveWall(it->bodyBlocker);
			actorNpcMap.erase(it->UID);
			it = actorNpcList.erase(it);
		}
		else {
			++it;
		}
	}
}

void World::UpdateSummons()
{
	SpawnPendingSummons();
	RunPendingSummonCommands();
	PulseGroundAuras();
	DespawnSummons();
}
