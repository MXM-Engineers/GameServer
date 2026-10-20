#include "config.h"
#include "world.h"
#include "world_common.h"
#include <mxm/game_content.h>

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

void World::RunSummonState(ActorNpc& npc, i32 state)
{
	const SummonInfo* info = GetGameXmlContent().FindSummon((i32)npc.docID);
	if(!info || state < 0 || state > 20) return;
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
	if(current && current != &owner) {
		const vec2 rel = vec2(current->body->GetWorldPos()) - vec2(npc.pos);
		if(glm::length(rel) > 1.0f) yaw = atan2f(rel.y, rel.x);
	}
	{
		eastl::fixed_string<char,48,false> name;
		name.sprintf("SKILL_%d_BEHAVIORSTATE", state);
		const ActionStateID action = ActionStateFromString(name.c_str());
		const vec3 targetPos = current ? current->body->GetWorldPos() : npc.pos;
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
			npc.motionTargetUID = current ? current->Main().UID : ActorUID::INVALID;
			npc.motionYaw = yaw;
		}
		LOG("Summon :: creature=%d plays %s", (i32)npc.docID, name.c_str());
	}
	f32 t = 0;
	foreach_const(c, info->skillStates[state]) {
		t += c->delay;
		if(c->kind == SummonCommand::REMOTE) {
			if(pendingSummonCommands.full()) continue;
			PendingSummonCommand pc;
			pc.time = TimeAdd(localTime, TimeMsToTime(t * 1000.0));
			pc.ownerIndex = npc.ownerIndex;
			pc.kind = c->kind;
			pc.index = c->index;
			pc.pos = npc.pos;
			pc.yaw = yaw;
			pc.skill = info->skills.empty() ? 0 : info->skills[0];
			pc.until = npc.despawnAt;
			pc.target = current && current != &owner ? current->Main().UID : ActorUID::INVALID;
			pc.source = npc.UID;
			pc.aim = npc.aimPos;
			pendingSummonCommands.push_back(pc);
		}
		else if(c->kind == SummonCommand::STATUS) {
			if(c->onCurrent) {
				if(current && !current->dead) ApplyStatus(owner, *current, c->index);
				continue;
			}
			const StatusInfo* st = GetGameXmlContent().FindStatus(c->index);
			if(!st) continue;
			const f32 duration = st->durationSec > 0 ? st->durationSec : 1.0f;
			replication->SendAddStatusToAll(c->index, npc.UID, owner.Main().UID, duration, npc.pos, npc.pos, 0);
			if(st->type == StatusType::SUICIDE) {
				const Time at = TimeAdd(localTime, TimeMsToTime(t * 1000.0));
				if(npc.despawnAt == Time::ZERO || at < npc.despawnAt) npc.despawnAt = at;
			}
			LOG("Summon :: creature=%d self status %d (%s)", (i32)npc.docID, c->index, st->typeName.c_str());
			if(st->auraStatus != 0 && st->targetingRange > 0 && !groundAuras.full()) {
				GroundAura ga;
				ga.ownerIndex = npc.ownerIndex;
				ga.auraStatus = st->ID;
				ga.range = st->targetingRange;
				ga.pos = npc.pos;
				ga.until = TimeAdd(localTime, TimeMsToTime(duration * 1000.0));
				ga.nextPulse = localTime;
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
	if(dist > 1.0f) replication->SendNpcMoveToAll(npc->UID, npc->pos, goal, atan2f(goal.y - npc->pos.y, goal.x - npc->pos.x), travel > 0 ? dist / travel : dist);
	foreach(st, activeStatuses) {
		if(st->casterIndex != owner.index || !st->info || st->info->type != StatusType::TARGET || localTime >= st->until) continue;
		Player& carried = players[st->targetIndex];
		const Time drop = TimeAdd(localTime, TimeMsToTime(travel * 1000.0));
		foreach(with, activeStatuses) {
			if(with->targetIndex == st->targetIndex && with->casterIndex == owner.index && with->appliedAt == st->appliedAt && with->until > drop) with->until = drop;
		}
		if(carried.clientHd == ClientHandle::INVALID && !carried.dead) {
			carried.pushTarget = vec3(vec2(pos), carried.body->GetWorldPos().z);
			carried.pushUntil = drop;
			carried.pushSpeed = glm::length(vec2(pos) - vec2(carried.body->GetWorldPos())) / MAX(travel, 0.05f);
			if(drop > carried.ccUntil) carried.ccUntil = drop;
		}
		LOG("Summon :: %ls carried to (%.0f, %.0f)", carried.name.data(), pos.x, pos.y);
	}
	npc->pos = goal;
	npc->aimPos = pos;
	RunSummonState(*npc, child);
}

void World::RelayWarp(ActorNpc& npc, const vec3& to)
{
	LOG("Summon :: relay robot warps from (%.0f, %.0f) to (%.0f, %.0f)", npc.pos.x, npc.pos.y, to.x, to.y);
	replication->SendNpcBehaviorToAll(npc.UID, ActionStateID::EXTRAMOTION_2_BEHAVIORSTATE, npc.pos, npc.rot.z, to, ActorUID::INVALID);
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

void World::UpdateSummonAi(f32 delta)
{
	const auto& content = GetGameXmlContent();
	foreach(it, actorNpcList) {
		ActorNpc& npc = *it;
		if(npc.pendingMotion != ActionStateID::INVALID && localTime >= npc.motionAt) {
			replication->SendNpcBehaviorToAll(npc.UID, npc.pendingMotion, npc.pos, npc.motionYaw, npc.motionTarget, npc.motionTargetUID);
			npc.pendingMotion = ActionStateID::INVALID;
		}
		if(npc.queuedState >= 0 && localTime >= npc.queuedStateAt && npc.pendingMotion == ActionStateID::INVALID) {
			if(glm::length(vec2(npc.queuedGoal)) > 1.0f) {
				replication->SendNpcMoveToAll(npc.UID, npc.pos, npc.queuedGoal, atan2f(npc.queuedGoal.y - npc.pos.y, npc.queuedGoal.x - npc.pos.x), RELAY_LEAP_SPEED);
				npc.pos = npc.queuedGoal;
			}
			const i32 state = npc.queuedState;
			npc.queuedState = -1;
			RunSummonState(npc, state);
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
			goal = owner.body->GetWorldPos();
			stopDist = info->aiDistance;
			if(info->perception > 0 && glm::length(vec2(goal) - vec2(npc.pos)) > info->perception) RelayWarp(npc, goal);
			if(npc.relayMode >= 0 && localTime >= npc.nextPassive && localTime >= npc.passiveBlockedUntil) {
				npc.nextPassive = TimeAdd(localTime, TimeMsToTime(RELAY_PASSIVE_SEC * 1000.0));
				RelayPassive(npc, *info, owner);
			}
		}
		else {
			ActorMaster* tm = FindMasterActor(npc.chaseTarget);
			Player* target = tm ? tm->parent : nullptr;
			if(npc.stuck) {
				if(!target || target->dead) { npc.stuck = false; npc.chaseTarget = ActorUID::INVALID; continue; }
				npc.pos = target->body->GetWorldPos();
				continue;
			}
			if(!target || target->dead || glm::length(vec2(target->body->GetWorldPos()) - vec2(npc.pos)) > info->perception) {
				target = nullptr;
				f32 best = info->perception;
				foreach(e, players) {
					if(e->dead || e->team == owner.team) continue;
					bool taken = false;
					foreach_const(o, actorNpcList) {
						if(&*o != &npc && o->ownerIndex == npc.ownerIndex && o->docID == npc.docID && o->chaseTarget == e->Main().UID) taken = true;
					}
					if(taken) continue;
					const f32 d = glm::length(vec2(e->body->GetWorldPos()) - vec2(npc.pos));
					if(d < best) { best = d; target = &*e; }
				}
				npc.chaseTarget = target ? target->Main().UID : ActorUID::INVALID;
			}
			if(!target) continue;
			goal = target->body->GetWorldPos();
			const f32 d = glm::length(vec2(goal) - vec2(npc.pos));
			if(d <= WEAPON_TARGET_RADIUS) {
				LOG("Summon :: creature=%d reached %ls", (i32)npc.docID, target->name.data());
				RunSummonState(npc, 1);
				if(npc.despawnAt == Time::ZERO || npc.despawnAt > localTime) npc.stuck = true;
				continue;
			}
		}

		if(info->moveSpeed <= 0) continue;
		const vec2 rel = vec2(goal) - vec2(npc.pos);
		const f32 dist = glm::length(rel);
		if(dist <= stopDist + 1.0f) continue;
		const f32 step = MIN(info->moveSpeed * delta, dist - stopDist);
		const vec2 dir = rel / dist;
		npc.pos = vec3(vec2(npc.pos) + dir * step, goal.z);
		npc.rot.z = atan2f(dir.y, dir.x);
		if(localTime >= npc.nextMoveSync) {
			npc.nextMoveSync = TimeAdd(localTime, TimeMsToTime(200));
			replication->SendNpcMoveToAll(npc.UID, npc.pos, goal, npc.rot.z, info->moveSpeed);
		}
	}
}

void World::UpdateSummons()
{
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
			ActorNpc& actor = reuse ? *reuse : SpawnNpcActor(CreatureIndex(ps->creature), nextSummonLocalID++);
			const f32 life = relay ? RELAY_LIFE_SEC : info && info->lifeTime > 0 ? info->lifeTime : 5.0f;
			if(!reuse) {
				actor.pos = ps->pos;
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
				actor.nextPassive = TimeAdd(localTime, TimeMsToTime(RELAY_PASSIVE_SEC * 1000.0));
			}
			actor.skillTarget = ps->target;
			owner.lastChildUID = actor.UID;
			if(reuse && info->perception > 0 && glm::length(vec2(ps->pos) - vec2(actor.pos)) > info->perception) RelayWarp(actor, ps->pos);
			if(info && owner.pendingChildSkill > 0 && localTime < owner.pendingChildSkillUntil) {
				const i32 child = owner.pendingChildSkill;
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
					actor.aimPos = owner.pendingChildPos;
					actor.queuedGoal = mode != RELAY_ARTILLERY_MODE && glm::length(vec2(owner.pendingChildPos)) > 1.0f ? vec3(vec2(owner.pendingChildPos), actor.pos.z) : vec3(0);
					const f32 modeSec = GetGameXmlContent().GetAniSeqTime(info->className.c_str(), info->stateAnim[modeState]);
					actor.queuedState = activeState;
					actor.queuedStateAt = TimeAdd(localTime, TimeMsToTime((f64)modeSec * 1000.0 + 60.0));
					LOG("Summon :: relay mode animation %d lasts %.2fs", info->stateAnim[modeState], modeSec);
				}
				else {
					LOG("Summon :: %ls relay state SKILL_%d", owner.name.data(), child);
					if(owner.pendingChildTarget != ActorUID::INVALID) actor.skillTarget = owner.pendingChildTarget;
					RunSummonState(actor, child);
				}
				owner.pendingChildSkill = 0;
			}
			if(info && !reuse) {
				f32 t = 0;
				foreach_const(c, info->onSpawn) {
					if(!pendingSummonCommands.full()) {
						PendingSummonCommand pc;
						pc.time = TimeAdd(localTime, TimeMsToTime(t * 1000.0));
						pc.ownerIndex = ps->ownerIndex;
						pc.kind = c->kind;
						pc.index = c->index;
						pc.pos = ps->pos;
						pc.yaw = ps->rot.z;
						pc.skill = info->skills.empty() ? 0 : info->skills[0];
						pc.until = actor.despawnAt;
						pendingSummonCommands.push_back(pc);
					}
					t += c->delay;
				}
			}
			LOG("Summon :: %ls creature=%d at (%.0f, %.0f) life=%.1fs known=%d", owner.name.data(), ps->creature, ps->pos.x, ps->pos.y, life, info != nullptr);
		}
		ps = pendingSummons.erase_unsorted(ps);
	}

	for(auto pc = pendingSummonCommands.begin(); pc != pendingSummonCommands.end(); ) {
		if(localTime < pc->time) { ++pc; continue; }
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
				pr.damage = pc->skill != 0 ? (i32)SkillDamage(SkillID(pc->skill)) : 0;
				pr.castPos = aimedGraph ? pc->aim : pc->pos;
				pr.angle = aimedGraph ? atan2f(pc->aim.y - pc->pos.y, pc->aim.x - pc->pos.x) : pc->yaw;
				if(aimedGraph) pr.preset = (i32)ActionCommand::TargetPreset::SELF_FFF;
				pr.target = pc->target;
				pr.sourceUID = pc->source;
				pendingRemotes.push_back(pr);
			}
		}
		else {
			const StatusInfo* st = GetGameXmlContent().FindStatus(pc->index);
			if(st && st->auraStatus != 0 && !groundAuras.full()) {
				GroundAura ga;
				ga.ownerIndex = pc->ownerIndex;
				ga.auraStatus = st->ID;
				ga.range = st->targetingRange;
				ga.pos = pc->pos;
				const Time statusEnd = st->durationSec > 0 ? TimeAdd(localTime, TimeMsToTime(st->durationSec * 1000.0)) : pc->until;
				ga.until = pc->until < statusEnd ? pc->until : statusEnd;
				ga.nextPulse = localTime;
				groundAuras.push_back(ga);
				LOG("Summon :: %ls aura %d (gives %d statuses within %.0f%s)", owner.name.data(), pc->index, (i32)st->auraStatuses.size(), st->targetingRange, st->auraAll ? ", everyone" : "");
			}
		}
		pc = pendingSummonCommands.erase_unsorted(pc);
	}

	for(auto ga = groundAuras.begin(); ga != groundAuras.end(); ) {
		if(localTime >= ga->until) { ga = groundAuras.erase_unsorted(ga); continue; }
		if(localTime >= ga->nextPulse) {
			ga->nextPulse = TimeAdd(localTime, TimeMsToTime(500));
			const StatusInfo* aura = GetGameXmlContent().FindStatus(ga->auraStatus);
			if(aura && ga->range > 0) PulseAura(ga->ownerIndex, *aura, vec2(ga->pos), ga->range, -1);
		}
		++ga;
	}

	for(auto it = actorNpcList.begin(); it != actorNpcList.end(); ) {
		if(it->despawnAt != Time::ZERO && localTime >= it->despawnAt) {
			LOG("Summon :: creature=%d despawned", (i32)it->docID);
			physics.RemoveWall(it->wall);
			it->wall = nullptr;
			const SummonInfo* info = GetGameXmlContent().FindSummon((i32)it->docID);
			if(info) {
				f32 t = 0;
				u8 wanted = 0;
				foreach_const(c, info->onDie) if(c->dieMode == it->dieMode) wanted = it->dieMode;
				foreach_const(c, info->onDie) {
					if(c->dieMode != wanted) continue;
					if(!pendingSummonCommands.full()) {
						PendingSummonCommand pc;
						pc.time = TimeAdd(localTime, TimeMsToTime(t * 1000.0));
						pc.ownerIndex = it->ownerIndex;
						pc.kind = c->kind;
						pc.index = c->index;
						pc.pos = it->pos;
						pc.yaw = it->rot.z;
						pc.skill = info->skills.empty() ? 0 : info->skills[0];
						pc.until = localTime;
						pendingSummonCommands.push_back(pc);
					}
					t += c->delay;
				}
			}
			actorNpcMap.erase(it->UID);
			it = actorNpcList.erase(it);
		}
		else {
			++it;
		}
	}
}
