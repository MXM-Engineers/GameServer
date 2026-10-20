#include "config.h"
#include "world.h"
#include "world_common.h"
#include <mxm/game_content.h>

static const f32 WEAPON_NODE_MAX_FORWARD = 250.0f;
static const i32 ULTIMATE_SKILL_SUFFIX = 50;
static const f32 REPEAT_HIT_REDUCTION = 0.1f;
static const f32 REPEAT_HIT_REDUCTION_CAP = 0.4f;
static const f32 ORBIT_RADIUS = 150.0f;
struct WorldRemoteSpawner
{
	static World::RemoteInst* Spawn(World& w, u32 ownerIndex, RemoteIdx idx, const vec3& pos, const vec2& dir, ActorUID target, SkillID skillID, u32 castID, i32 damage, bool broadcast, bool casterLocal)
	{
		return w.SpawnRemote(ownerIndex, idx, pos, dir, target, skillID, castID, damage, broadcast, nullptr, 0, casterLocal);
	}
};

bool World::RemoteCanAffect(const RemoteInst& inst, const Player& p, bool includeOwner) const
{
	if(p.dead || p.untargetable) return false;
	if(inst.remote->monsterOnly) return false;
	if(inst.excludeIndex >= 0 && (i32)p.index == inst.excludeIndex) return false;
	const Player& owner = players[inst.ownerIndex];
	const Remote& remote = *inst.remote;
	const auto group = remote.damageGroup;
	const bool friendContact = group == Remote::DamageGroup::eFRIEND || group == Remote::DamageGroup::eALL || remote.contactFriend;
	if(p.index == owner.index) {
		if(!includeOwner && !remote.vsOwner) return false;
		return remote.vsOwner || group == Remote::DamageGroup::eFRIEND || group == Remote::DamageGroup::eALL;
	}
	const bool enemy = p.team != owner.team;
	if(enemy) return group == Remote::DamageGroup::eENEMY || group == Remote::DamageGroup::eALL;
	return friendContact;
}

static f32 WeaponNodeForward(const GameXmlContent& content, ClassType cls, SkillID skillID, const Remote& remote, u16 nodeName)
{
	if(nodeName == 0 || remote.maxSpeed > 0 || remote.attachFireObject || remote.boundType != Remote::BoundType::E_BOUND_BOX) return 0;
	auto f = content.skillMap.find(skillID);
	if(f == content.skillMap.end()) return 0;
	SkillNormalModel& skill = const_cast<SkillNormalModel&>(f->second);
	const f32 indicator = skill.getSkillNormalLevelByIndex(0)->getSkillRangeLengthY();
	if(indicator <= 0) return 0;
	const GameXmlContent::Action* action = content.FindSkillAction(cls, skill.action);
	if(!action) return 0;
	foreach_const(cmd, action->commands) {
		if(cmd->type == ActionCommand::Type::GRAPH_MOVE_HORZ && cmd->graphMoveHorz.distance > 0) return 0;
		if(cmd->type == ActionCommand::Type::MOVE && cmd->move.preset != ActionCommand::MovePreset::MOVE_NONE) return 0;
	}
	const f32 forward = indicator - (remote.offsetY + remote.boundSize[1] * 0.5f);
	return forward > 0 && forward <= WEAPON_NODE_MAX_FORWARD ? forward : 0;
}

World::RemoteInst* World::SpawnRemote(u32 ownerIndex, RemoteIdx idx, const vec3& pos, const vec2& dir, ActorUID target, SkillID skillID, u32 castID, i32 damage, bool broadcast, const vec3* aimPos, u16 nodeName, bool casterCreatesLocally, bool stopAtTarget)
{
	const Remote* remote = GetGameXmlContent().FindRemote(idx);
	if(!remote || remoteInsts.full()) return nullptr;

	RemoteInst inst;
	inst.instID = nextRemoteInstID++;
	inst.remote = remote;
	inst.ownerIndex = ownerIndex;
	inst.skillID = skillID;
	inst.castID = castID;
	inst.damage = damage;
	inst.pos = pos;
	inst.dir = dir;
	inst.travelled = 0;
	inst.wallZ = players[ownerIndex].body->GetWorldPos().z + BodyMidHeight(players[ownerIndex]);
	inst.speed = remote->maxSpeed;
	inst.range = remote->maxDistance;
	if(remote->behaviorType == Remote::BehaviorType::GRAPH && remote->fixedGraphLength > 0) inst.range = remote->fixedGraphLength;
	if(inst.range <= 0 && remote->lifeTime > 0 && inst.speed > 0) inst.range = remote->lifeTime * inst.speed;
	if(remote->behaviorType == Remote::BehaviorType::GRAPH && remote->fixedGraphLength <= 0 && aimPos && glm::length(vec2(*aimPos)) > 1.0f) {
		const f32 aimDist = glm::length(vec2(*aimPos) - vec2(pos));
		inst.range = inst.range > 0 ? MIN(aimDist, inst.range) : aimDist;
		inst.landZ = aimPos->z;
		inst.hasLandZ = true;
	}
	inst.stopAtFirstHit = false;
	if(stopAtTarget && remote->behaviorType != Remote::BehaviorType::GRAPH && aimPos && glm::length(vec2(*aimPos)) > 1.0f && inst.speed > 0) {
		const f32 aimDist = glm::length(vec2(*aimPos) - vec2(pos));
		inst.range = inst.range > 0 ? MIN(aimDist, inst.range) : aimDist;
	}
	inst.spawnTime = localTime;
	inst.target = target;
	inst.hitsLeft = MAX(remote->activateCount, 1);
	inst.areaDone = false;
	inst.casterLocal = casterCreatesLocally;
	inst.graphStart = pos;
	inst.graphDir = dir;
	if(remote->timeEventDelay >= 0) {
		inst.timeEventActive = true;
		inst.nextTimeEvent = TimeAdd(localTime, TimeMsToTime(remote->timeEventDelay * 1000.0));
	}

	if(broadcast) {
		const u32 seed = nextRemoteSeed;
		nextRemoteSeed += 2;
		inst.clientRemoteID = seed + 1;
		vec3 targetPos = pos + vec3(dir * (inst.range > 0 ? inst.range : 1000.0f), 0);
		const ActorMaster* targetMaster = target != ActorUID::INVALID ? FindMasterActor(target) : nullptr;
		if(targetMaster) targetPos = targetMaster->parent->body->GetWorldPos();
		if(remote->behaviorType == Remote::BehaviorType::GRAPH && aimPos && glm::length(vec2(*aimPos)) > 1.0f) {
			targetPos = pos + vec3(dir * inst.range, 0);
			targetPos.z = aimPos->z;
		}
		const bool plainStaticBox = inst.speed <= 0 && remote->boundType == Remote::BoundType::E_BOUND_BOX && remote->angle <= 0;
		const bool chained = remote->destroyCreateRemote != RemoteIdx::INVALID || !remote->onDestroy.empty() || !remote->onTime.empty() || !remote->onActivate.empty();
		const bool flip = plainStaticBox && (chained || nodeName == 0);
		const f32 sendYaw = atan2f(dir.y, dir.x) + (flip ? PI : 0.0f);
		vec3 sendPos = pos - vec3(0, 0, remote->offsetZ);
		const bool atAimedPoint = aimPos && glm::length(vec2(*aimPos)) > 1.0f && glm::length(vec2(pos) - vec2(*aimPos)) < 1.0f;
		if(plainStaticBox && !flip && !atAimedPoint) sendPos -= vec3(dir * (remote->forceTargetDir ? -remote->offsetY : remote->offsetY), 0);
		else if(plainStaticBox && flip && !atAimedPoint) sendPos += vec3(dir * (remote->forceTargetDir ? -remote->offsetY : remote->offsetY), 0);
		replication->SendRemoteCreateToAll(players[ownerIndex].Main().UID, seed, idx, sendPos, sendYaw, targetPos, targetMaster ? target : ActorUID::INVALID, GetGameXmlContent().GetNodeName(nodeName), casterCreatesLocally ? players[ownerIndex].clientHd : ClientHandle::INVALID);
	}

	remoteInsts.push_back(inst);

	LOG("Remote :: spawn %d at (%.0f, %.0f) dir=(%.2f, %.2f) speed=%.0f range=%.0f", (i32)idx, pos.x, pos.y, dir.x, dir.y, inst.speed, inst.range);
	return &remoteInsts.back();
}

static vec2 RotateDir(const vec2& d, f32 deg)
{
	const f32 a = deg * PI / 180.0f;
	const f32 c = cosf(a), s = sinf(a);
	return vec2(d.x * c - d.y * s, d.x * s + d.y * c);
}

static RemoteIdx CostReplacedRemote(const GameXmlContent::CostLevel* level, RemoteIdx idx)
{
	if(level) foreach_const(rr, level->remoteReplace) if(rr->first == (i32)idx) return RemoteIdx(rr->second);
	return idx;
}

static void SpawnRemoteRules(World& world, const World::RemoteInst& parent, const Remote::SpawnRules& rules, const vec3& pos, i32 excludeIndex, RemoteIdx skipFirst, bool clientChains = false)
{
	bool skipped = false;
	foreach_const(rule, rules) {
		if(!skipped && rule->creature <= 0 && rule->kind == Remote::SPAWN_SINGLE && rule->idx == skipFirst) {
			skipped = true;
			continue;
		}
		if(rule->creature > 0) {
			world.SpawnCreatureFromRemote(parent, rule->creature, pos);
			continue;
		}
		const Remote* child = GetGameXmlContent().FindRemote(rule->idx);
		if(!child) continue;

		for(i32 n = 0; n < rule->maxNum; n++) {
			vec2 dir = parent.dir;
			vec3 at = pos;
			switch(rule->kind) {
				case Remote::SPAWN_RADIATE: {
					if(rule->maxNum > 1 && rule->maxAngle > 0) {
						if(rule->maxAngle >= 360.0f) dir = RotateDir(parent.dir, 360.0f * n / rule->maxNum);
						else dir = RotateDir(parent.dir, -rule->maxAngle * 0.5f + rule->maxAngle * n / (rule->maxNum - 1));
					}
				} break;
				case Remote::SPAWN_CIRCLE: {
					const f32 a = (RandUint() % 3600) * 0.1f * PI / 180.0f;
					const f32 r = rule->radius * sqrtf((RandUint() % 1000) / 1000.0f);
					at = pos + vec3(cosf(a) * r, sinf(a) * r, 0);
				} break;
				case Remote::SPAWN_DEFAULT: {
					dir = RotateDir(parent.dir, rule->angleY);
				} break;
			}
			ActorUID childTarget = parent.target;
			if(rule->targetOwner) {
				World::Player& owner = world.GetPlayer(parent.ownerIndex);
				childTarget = owner.Main().UID;
				const vec2 rel = vec2(owner.body->GetWorldPos()) - vec2(at);
				if(glm::length(rel) > 1.0f) dir = glm::normalize(rel);
			}
			World::RemoteInst* inst = WorldRemoteSpawner::Spawn(world, parent.ownerIndex, CostReplacedRemote(parent.costLevel, rule->idx), at + vec3(dir * child->offsetY, 0), dir, childTarget, parent.skillID, parent.castID, parent.damage,
				parent.clientRemoteID != 0 && !(clientChains && rule->kind != Remote::SPAWN_SINGLE && !rule->targetOwner), parent.casterLocal);
			if(inst) {
				inst->excludeIndex = excludeIndex;
				inst->costLevel = parent.costLevel;
				inst->centeredBox = parent.centeredBox;
			}
		}
	}
}

void World::RemoteHit(RemoteInst& inst, Player& target)
{
	const Remote& remote = *inst.remote;
	Player& owner = players[inst.ownerIndex];
	if(!RemoteCanAffect(inst, target, true)) return;
	if(remote.activateDelay > 0 && TimeDiffSec(TimeDiff(inst.spawnTime, localTime)) < remote.activateDelay) return;
	const bool enemy = target.team != owner.team;
	if(enemy) ApplyRemoteHitResourceGain(owner, remote.ID, inst.castID, target.index);

	const i32 amount = (i32)(inst.damage * remote.attackMultiplier);
	if(enemy && remote.memorizeTarget) owner.memorizedTarget = target.Main().UID;
	if(enemy && remote.changeCasterAction != ActionStateID::INVALID && !inst.actionChanged && !owner.dead) {
		inst.actionChanged = true;
		const ActorUID targetUID = target.Main().UID;
		LOG("Remote :: %ls hit %ls -> caster action %d", owner.name.data(), target.name.data(), (i32)remote.changeCasterAction);
		PlayerCastSkill(owner, inst.skillID, target.body->GetWorldPos(), Slice<const ActorUID>(&targetUID, 1), 0, remote.changeCasterAction);
	}
	if(amount <= 0 && remote.statuses.empty() && remote.friendlyStatuses.empty() && remote.onActivate.empty() && remote.onHitCharacter.empty()) return;

	const bool sharedZone = inst.speed <= 0 && remote.lifeTime > remote.hitInvalidTime && inst.castID != 0;
	foreach(h, castHits) {
		if(h->playerIndex != target.index || localTime >= h->until) continue;
		if(h->instID == inst.instID) return;
		if(sharedZone && h->castID == inst.castID && h->remoteID == remote.ID) return;
	}
	if(inst.speed <= 0 && enemy) inst.hitsLeft--;
	if(!castHits.full()) {
		CastHit h;
		h.instID = inst.instID;
		h.remoteID = remote.ID;
		h.castID = inst.castID;
		h.playerIndex = target.index;
		h.until = TimeAdd(localTime, TimeMsToTime(MAX(remote.hitInvalidTime, 0.1f) * 1000.0));
		castHits.push_back(h);
	}

	if(amount > 0) {
		if(enemy && inst.skillID != SkillID::INVALID) PassiveOnSkillHit(owner, target);
		i32 stackBonus = 0;
		if(enemy && (i32)inst.skillID % 100 == ULTIMATE_SKILL_SUFFIX) {
			foreach_const(st, activeStatuses) {
				if(st->targetIndex == target.index && st->casterIndex == owner.index && st->info && st->info->additionalDamageByCaster > 0 && localTime < st->until)
					stackBonus += (i32)(st->info->additionalDamageByCaster * MAX(st->overlap, 1));
			}
			if(stackBonus > 0) LOG("Passive :: %ls stack bonus +%d on %ls", owner.name.data(), stackBonus, target.name.data());
		}
		i32 dealt = amount + stackBonus;
		if(enemy && GetGameXmlContent().IsRepeatHitReduced((i32)inst.skillID)) {
			RepeatHit* rh = nullptr;
			foreach(r, repeatHits) if(r->castID == inst.castID && r->targetIndex == target.index) rh = &*r;
			if(!rh) {
				repeatHits.push_back({ inst.castID, target.index, 0, localTime });
				rh = &repeatHits.back();
			}
			dealt = (i32)(dealt * MAX(1.0f - REPEAT_HIT_REDUCTION * rh->count, 1.0f - REPEAT_HIT_REDUCTION_CAP));
			rh->count++;
			rh->at = localTime;
		}
		if(enemy) DamagePlayer(owner, target, inst.skillID, dealt, nullptr, inst.clientRemoteID, remote.ID, inst.dir);
		else if(remote.damageGroup == Remote::DamageGroup::eFRIEND) HealPlayer(owner, target, amount);
	}

	inst.lastHitIndex = (i32)target.index;
	statusFromBehind = remote.offsetY < 0;
	statusForceCenter = inst.pos;
	statusForceCenterSet = true;
	if(enemy && remote.onHitOwnerStatus != 0 && !owner.dead) ApplyStatus(owner, owner, remote.onHitOwnerStatus);
	if(!target.dead) {
		const bool friendGroup = remote.damageGroup == Remote::DamageGroup::eFRIEND;
		if(enemy || friendGroup) {
			foreach_const(st, remote.statuses) {
				i32 statusID = st->statusID;
				if(inst.costLevel) {
					foreach_const(sr, inst.costLevel->statusReplace) if(sr->first == statusID) statusID = sr->second;
				}
				if(st->rate >= 100 || (i32)(RandUint() % 100) < st->rate) { statusCasterOverride = inst.sourceUID; ApplyStatus(owner, target, statusID, (f32)inst.damage); statusCasterOverride = ActorUID::INVALID; }
			}
		}
		if(!enemy) {
			foreach_const(st, remote.friendlyStatuses) {
				if(st->rate >= 100 || (i32)(RandUint() % 100) < st->rate) { statusCasterOverride = inst.sourceUID; ApplyStatus(owner, target, st->statusID, (f32)inst.damage); statusCasterOverride = ActorUID::INVALID; }
			}
		}
	}

	if(!remote.onActivate.empty() || !remote.onHitCharacter.empty()) {
		const RemoteInst copy = inst;
		const vec3 at = vec3(vec2(target.body->GetWorldPos()), inst.pos.z);
		SpawnRemoteRules(*this, copy, remote.onActivate, at, -1, RemoteIdx::INVALID);
		SpawnRemoteRules(*this, copy, remote.onHitCharacter, at, (i32)target.index, RemoteIdx::INVALID);
	}

	if(enemy) {
		const i32 hitEventRemote = GetGameXmlContent().FindSkillHitEventRemote((i32)inst.skillID);
		if(hitEventRemote != 0 && hitEventRemote == (i32)remote.ID &&
		   eastl::find(comboTriggeredCasts.begin(), comboTriggeredCasts.end(), inst.castID) == comboTriggeredCasts.end()) {
			if(comboTriggeredCasts.full()) comboTriggeredCasts.erase(comboTriggeredCasts.begin());
			comboTriggeredCasts.push_back(inst.castID);
			owner.memorizedTarget = target.Main().UID;
			const ActorUID targetUID = target.Main().UID;
			const vec2 rel = vec2(target.body->GetWorldPos() - owner.body->GetWorldPos());
			LOG("ComboHit :: %ls skill=%d remote=%d -> next part", owner.name.data(), (i32)inst.skillID, (i32)remote.ID);
			castCostLevel = inst.costLevel;
			ScheduleSkillRemotes(owner, inst.skillID, target.body->GetWorldPos(), Slice<const ActorUID>(&targetUID, 1), atan2f(rel.y, rel.x), 1);
			castCostLevel = nullptr;
		}
	}
	statusFromBehind = false;
	statusForceCenterSet = false;
}

void World::RemoteHitAlongPath(RemoteInst& inst)
{
	const Remote& remote = *inst.remote;
	if(remote.damageGroup == Remote::DamageGroup::eNONE || remote.damageGroup == Remote::DamageGroup::INVALID) return;
	if(remote.attackMultiplier <= 0 && remote.statuses.empty()) return;
	const f32 radius = remote.boundSize[0] * 0.5f;
	foreach(it, players) {
		if(!RemoteCanAffect(inst, *it, false)) continue;
		if(it->team == players[inst.ownerIndex].team) continue;
		if(glm::length(vec2(it->body->GetWorldPos()) - vec2(inst.pos)) > radius + HitRadius(*it)) continue;
		RemoteHit(inst, *it);
	}
}

void World::RemoteApplyShape(RemoteInst& inst)
{
	const Remote& remote = *inst.remote;
	f32 halfWidth = remote.boundSize[0] * 0.5f;
	f32 start = 0;
	f32 length = inst.range > 0 ? inst.range : remote.boundSize[1];
	switch(remote.boundType) {
		case Remote::BoundType::E_BOUND_CAPSULE: {
			length = remote.boundSize[1] + remote.boundSize[0];
			start = -remote.boundSize[0] * 0.5f;
		} break;
		case Remote::BoundType::E_BOUND_HAMMER:
		case Remote::BoundType::E_BOUND_BEAM:
		case Remote::BoundType::E_BOUND_BEAM_NIF:
		case Remote::BoundType::E_BOUND_LASER:
		case Remote::BoundType::E_BOUND_RAY: {
			if(length <= 0) length = MAX((f32)remote.boundSize[0], 300.0f);
		} break;
		default: return;
	}
	if(length <= 0) return;

	foreach(it, players) {
		const vec3 pos = it->body->GetWorldPos();
		if(fabsf(pos.z - inst.pos.z) > WEAPON_PROJECTILE_HIT_HEIGHT) continue;
		const vec2 center = vec2(inst.pos) + inst.dir * (start + length * 0.5f);
		const vec2 rel = LagClosestPos(*it, center) - vec2(inst.pos);
		const f32 along = glm::dot(rel, inst.dir);
		const f32 side = fabsf(glm::dot(rel, Perp(inst.dir)));
		if(along < start - HitRadius(*it) || along > start + length + HitRadius(*it)) continue;
		if(side > halfWidth + HitRadius(*it)) continue;
		RemoteHit(inst, *it);
	}
}

void World::RemoteApplyArea(RemoteInst& inst)
{
	const Remote& remote = *inst.remote;
	if(remote.boundType != Remote::BoundType::E_BOUND_SPHERE && remote.boundType != Remote::BoundType::E_BOUND_BOX) {
		RemoteApplyShape(inst);
		return;
	}
	const bool fanBox = remote.boundType == Remote::BoundType::E_BOUND_BOX && remote.angle > 0;
	if(remote.boundType == Remote::BoundType::E_BOUND_SPHERE || fanBox) {
		const bool cone = remote.angle > 0;
		const f32 radius = remote.boundSize[0] * 0.5f;
		foreach(it, players) {
			const vec3 pos = it->body->GetWorldPos();
			if(fabsf(pos.z - inst.pos.z) > WEAPON_PROJECTILE_HIT_HEIGHT) continue;
			const vec2 rel = LagClosestPos(*it, vec2(inst.pos)) - vec2(inst.pos);
			const f32 dist = glm::length(rel);
			if(dist > radius + HitRadius(*it)) continue;
			if(remote.angle > 0 && dist > HitRadius(*it)) {
				if(glm::dot(rel / dist, inst.dir) < cosf(MIN(remote.angle * 0.5f, 180.0f) * PI / 180.0f)) continue;
			}
			RemoteHit(inst, *it);
		}
	}
	else if(remote.boundType == Remote::BoundType::E_BOUND_BOX) {
		const f32 halfWidth = remote.boundSize[0] * 0.5f;
		const f32 length = remote.boundSize[1];
		foreach(it, players) {
			const vec3 pos = it->body->GetWorldPos();
			if(fabsf(pos.z - inst.pos.z) > WEAPON_PROJECTILE_HIT_HEIGHT) continue;
			const f32 back = length * 0.5f;
			const vec2 center = vec2(inst.pos) + inst.dir * (length * 0.5f - back);
			const vec2 rel = LagClosestPos(*it, center) - vec2(inst.pos) + inst.dir * back;
			const f32 along = glm::dot(rel, inst.dir);
			const f32 side = fabsf(glm::dot(rel, Perp(inst.dir)));
			if(along < -HitRadius(*it) || along > length + HitRadius(*it)) continue;
			if(side > halfWidth + HitRadius(*it)) continue;
			RemoteHit(inst, *it);
		}
	}
}

void World::UpdateRemotes(f32 delta)
{
	SpawnPendingRemotes();

	for(int i = 0; i < (i32)remoteInsts.size(); ) {
		RemoteInst& inst = remoteInsts[i];
		const Remote& remote = *inst.remote;
		const f32 elapsed = (f32)TimeDiffSec(TimeDiff(inst.spawnTime, localTime));
		bool destroyed = inst.killNow;
		Player& instOwner = players[inst.ownerIndex];

		if(remote.attachTargetObject && inst.target != ActorUID::INVALID) {
			const ActorMaster* targetMaster = FindMasterActor(inst.target);
			if(!targetMaster || targetMaster->parent->dead) destroyed = true;
			else inst.pos = vec3(vec2(targetMaster->parent->body->GetWorldPos()), inst.pos.z);
		}
		if(remote.behaviorType == Remote::BehaviorType::CASTER_TARGET && !instOwner.dead) MoveCasterTargetRemote(inst, instOwner, delta);
		const bool attached = remote.attachFireObject || remote.behaviorType == Remote::BehaviorType::ORBIT ||
			remote.behaviorType == Remote::BehaviorType::BEAM;
		if(attached) {
			if(instOwner.dead) {
				destroyed = true;
			}
			else {
				const f32 yaw = instOwner.input.rot.upperYaw;
				inst.dir = vec2(cosf(yaw), sinf(yaw));
				inst.pos = instOwner.body->GetWorldPos() + vec3(inst.dir * (remote.forceTargetDir ? -remote.offsetY : remote.offsetY), 158.0f);
			}
		}

		if(!destroyed && inst.timeEventActive && localTime >= inst.nextTimeEvent) RunRemoteTimeEvent(inst, elapsed);

		if(!destroyed) {
			if(remote.behaviorType == Remote::BehaviorType::CHAIN) destroyed = UpdateChainRemote(inst);
			else if(attached) ApplyAttachedRemote(inst, instOwner);
			else switch(remote.behaviorType) {
				case Remote::BehaviorType::CASTER_TARGET: break;
				case Remote::BehaviorType::TARGET:
				case Remote::BehaviorType::FOLLOW_TARGET:
				case Remote::BehaviorType::HOMING: destroyed = UpdateTargetRemote(inst, delta); break;
				case Remote::BehaviorType::GRAPH: destroyed = UpdateGraphRemote(inst, delta); break;
				default: destroyed = UpdateLinearRemote(inst, delta, elapsed); break;
			}
		}

		if(!destroyed && remote.lifeTime > 0 && elapsed >= remote.lifeTime) destroyed = true;
		if(!destroyed && remote.lifeTime <= 0 && inst.speed <= 0 && inst.areaDone) destroyed = true;


		if(destroyed) DestroyRemote(i);
		else i++;
	}

	ExpireHitRecords();
}

void World::SpawnPendingRemotes()
{
	for(auto pr = pendingRemotes.begin(); pr != pendingRemotes.end(); ) {
		if(localTime < pr->time) {
			++pr;
			continue;
		}

		Player& owner = players[pr->ownerIndex];
		if(pr->remove) {
			bool ended = false;
			foreach(ri, remoteInsts) {
				if(ri->ownerIndex == pr->ownerIndex && ri->remote && ri->remote->ID == pr->idx && !ri->killNow) { ri->killNow = true; ended = true; }
			}
			if(ended) LOG("Remote :: %ls ends live remote %d (action Remove)", owner.name.data(), (i32)pr->idx);
			pr = pendingRemotes.erase_unsorted(pr);
			continue;
		}
		const vec2 dir = vec2(cosf(pr->angle), sinf(pr->angle));
		const Remote* remote = GetGameXmlContent().FindRemote(pr->idx);
		const f32 offsetY = remote ? (remote->forceTargetDir ? -remote->offsetY : remote->offsetY) : 0;
		vec3 pos = owner.body->GetWorldPos() + vec3(dir * offsetY, 158.0f);

		bool atAimedPoint = pr->atTarget;
		switch((ActionCommand::TargetPreset)pr->preset) {
			case ActionCommand::TargetPreset::POS_FFF:
			case ActionCommand::TargetPreset::POS_TFF:
			case ActionCommand::TargetPreset::POS_TTT_1:
			case ActionCommand::TargetPreset::POS_TFF_1_800_1000: {
				atAimedPoint = glm::length(vec2(pr->castPos)) > 1.0f;
				const vec3 base = glm::length(vec2(pr->castPos)) > 1.0f ? pr->castPos : owner.body->GetWorldPos();
				pos = vec3(vec2(base) + dir * pr->offsetFront + Perp(dir) * pr->offsetSide, owner.body->GetWorldPos().z);
			} break;
			case ActionCommand::TargetPreset::CURRENT_FFF: {
				const ActorMaster* targetMaster = pr->target != ActorUID::INVALID ? FindMasterActor(pr->target) : nullptr;
				if(targetMaster && (!remote || remote->maxSpeed <= 0)) pos = targetMaster->parent->body->GetWorldPos();
			} break;
			case ActionCommand::TargetPreset::LAST_CHILD_FFF:
			case ActionCommand::TargetPreset::LAST_CHILD_TFF: {
				const ActorNpc* child = FindNpcActor(owner.lastChildUID);
				if(child && localTime < child->despawnAt) pos = vec3(vec2(child->pos), owner.body->GetWorldPos().z);
			} break;
			default: {
				if(pr->offsetFront != 0 || pr->offsetSide != 0) pos = vec3(vec2(owner.body->GetWorldPos()) + dir * pr->offsetFront + Perp(dir) * pr->offsetSide, owner.body->GetWorldPos().z);
			} break;
		}
		if(pr->atTarget) {
			const ActorMaster* targetMaster = pr->target != ActorUID::INVALID ? FindMasterActor(pr->target) : nullptr;
			if(targetMaster) pos = targetMaster->parent->body->GetWorldPos();
			else if(glm::length(vec2(pr->castPos)) > 1.0f) pos = vec3(vec2(pr->castPos), owner.body->GetWorldPos().z);
		}

		if(remote && IsChainedZoneRemote(remote->ID)) {
			const f32 length = (f32)remote->boundSize[1];
			foreach_const(ri, remoteInsts) {
				if(ri->castID != pr->castID || !ri->remote || ri->remote->ID != pr->idx || ri->killNow) continue;
				const f32 ahead = glm::dot(vec2(pos) - vec2(ri->pos), dir);
				if(ahead >= 0 && ahead < length) pos = vec3(vec2(ri->pos) + dir * length, pos.z);
			}
		}
		if(!owner.dead) {
			bool casterLocal = false;
			{
				const char* name = ActionCommand::TargetPresetToString((ActionCommand::TargetPreset)pr->preset);
				for(const char* t = name; t && *t; ) {
					const char* e = t;
					while(*e && *e != '_') e++;
					if(e - t == 3 && (t[0] == 'T' || t[0] == 'F') && (t[1] == 'T' || t[1] == 'F') && (t[2] == 'T' || t[2] == 'F')) {
						casterLocal = t[0] == 'T';
						break;
					}
					t = *e ? e + 1 : e;
				}
			}
			if(ServerCreatesRemotes(players[pr->ownerIndex].Main().classType)) casterLocal = false;
			RemoteInst* spawned = SpawnRemote(pr->ownerIndex, pr->idx, pos, dir, pr->target, pr->skillID, pr->castID, pr->damage, true, &pr->castPos, pr->nodeName, casterLocal, pr->stopAtTarget);
			if(spawned) {
				spawned->sourceUID = pr->sourceUID;
				spawned->costLevel = pr->costLevel;
				spawned->centeredBox = atAimedPoint;
				const f32 nodeForward = atAimedPoint ? 0 : WeaponNodeForward(GetGameXmlContent(), owner.Main().classType, pr->skillID, *spawned->remote, pr->nodeName);
				if(nodeForward > 0) {
					spawned->pos += vec3(dir * nodeForward, 0);
					LOG("Remote :: %d weapon node forward %.0f (skill indicator)", (i32)pr->idx, nodeForward);
				}
			}
		}
		pr = pendingRemotes.erase_unsorted(pr);
	}
}

void World::MoveCasterTargetRemote(RemoteInst& inst, Player& instOwner, f32 delta)
{
	const Remote& remote = *inst.remote;
	const vec2 ownerPos = vec2(instOwner.body->GetWorldPos());
	if(inst.followDist < 0) inst.followDist = glm::length(vec2(inst.pos) - ownerPos);
	vec2 desired;
	const ActorMaster* tm = inst.target != ActorUID::INVALID ? FindMasterActor(inst.target) : nullptr;
	if(instOwner.clientHd == ClientHandle::INVALID && tm && !tm->parent->dead) desired = vec2(tm->parent->body->GetWorldPos());
	else {
		const f32 yaw = instOwner.input.rot.upperYaw;
		desired = ownerPos + vec2(cosf(yaw), sinf(yaw)) * inst.followDist;
	}
	const vec2 rel = desired - vec2(inst.pos);
	const f32 len = glm::length(rel);
	const f32 step = (remote.maxSpeed > 0 ? remote.maxSpeed : 600.0f) * delta;
	if(len > 1.0f) inst.pos += vec3(rel / len * MIN(step, len), 0);
	if(remote.boundType != Remote::BoundType::E_BOUND_NONE && remote.damageGroup != Remote::DamageGroup::eNONE) RemoteApplyArea(inst);
}

void World::RunRemoteTimeEvent(RemoteInst& inst, f32 elapsed)
{
	const Remote& remote = *inst.remote;
	if(remote.timeEventRecycle && remote.timeEventDelay > 0.01f) inst.nextTimeEvent = TimeAdd(inst.nextTimeEvent, TimeMsToTime(remote.timeEventDelay * 1000.0));
	else inst.timeEventActive = false;
	if(remote.timeChangeSpeed > 0) {
		inst.speed = remote.timeChangeSpeed;
		if(remote.maxDistance <= 0 && remote.lifeTime > 0) inst.range = inst.travelled + MAX(remote.lifeTime - elapsed, 0.0f) * inst.speed;
	}
	const RemoteInst copy = inst;
	SpawnRemoteRules(*this, copy, remote.onTime, copy.pos, copy.excludeIndex, RemoteIdx::INVALID);
}

bool World::UpdateChainRemote(RemoteInst& inst)
{
	const Remote& remote = *inst.remote;
	if(!inst.areaDone) {
		inst.areaDone = true;
		const f32 sight = remote.sight > 0 ? remote.sight : 1450.0f;
		Player* best = nullptr;
		f32 bestDist = sight + WEAPON_TARGET_RADIUS;
		foreach(it, players) {
			if(!RemoteCanAffect(inst, *it, remote.vsOwner)) continue;
			const f32 d = glm::length(vec2(it->body->GetWorldPos()) - vec2(inst.pos));
			if(d < bestDist) { bestDist = d; best = &*it; }
		}
		if(best) {
			inst.dir = glm::length(vec2(best->body->GetWorldPos() - inst.pos)) > 1.0f ? glm::normalize(vec2(best->body->GetWorldPos() - inst.pos)) : inst.dir;
			inst.pos = vec3(vec2(best->body->GetWorldPos()), inst.pos.z);
			RemoteHit(inst, *best);
			inst.excludeIndex = (i32)best->index;
		}
		else {
			inst.chainEnded = true;
			return true;
		}
	}
	return false;
}

void World::ApplyAttachedRemote(RemoteInst& inst, Player& instOwner)
{
	const Remote& remote = *inst.remote;
	if(remote.behaviorType == Remote::BehaviorType::ORBIT) {
		const f32 radius = remote.boundSize[0] * 0.5f + ORBIT_RADIUS;
		foreach(it, players) {
			if(!RemoteCanAffect(inst, *it, false)) continue;
			if(glm::length(vec2(it->body->GetWorldPos()) - vec2(instOwner.body->GetWorldPos())) <= radius + HitRadius(*it)) RemoteHit(inst, *it);
		}
	}
	else if(remote.boundType != Remote::BoundType::E_BOUND_NONE && remote.damageGroup != Remote::DamageGroup::eNONE) {
		RemoteApplyArea(inst);
	}
}

bool World::UpdateTargetRemote(RemoteInst& inst, f32 delta)
{
	const Remote& remote = *inst.remote;
	if(remote.behaviorType == Remote::BehaviorType::TARGET && remote.attachTargetObject && inst.target != ActorUID::INVALID) return false;
	if(inst.target == ActorUID::INVALID && remote.behaviorType == Remote::BehaviorType::HOMING && remote.homingLength > 0) {
		f32 bestDist = remote.homingLength;
		const f32 halfCos = cosf(remote.homingAngle * 0.5f * PI / 180.0f);
		foreach(it, players) {
			if(!RemoteCanAffect(inst, *it, false)) continue;
			const vec2 rel = vec2(it->body->GetWorldPos() - inst.pos);
			const f32 d = glm::length(rel);
			if(d > bestDist) continue;
			if(d > 1.0f && remote.homingAngle > 0 && remote.homingAngle < 360 && glm::dot(rel / d, inst.dir) < halfCos) continue;
			bestDist = d;
			inst.target = it->Main().UID;
		}
		if(inst.target != ActorUID::INVALID) LOG("Remote :: %d homing acquired a target at %.0f", (i32)remote.ID, bestDist);
	}
	const ActorMaster* targetMaster = inst.target != ActorUID::INVALID ? FindMasterActor(inst.target) : nullptr;
	if(!targetMaster || targetMaster->parent->dead) return true;
	Player& target = *targetMaster->parent;
	const vec3 tpos = target.body->GetWorldPos();
	const vec2 rel = vec2(tpos - inst.pos);
	const f32 dist = glm::length(rel);
	const f32 step = (inst.speed > 0 ? inst.speed : 3000.0f) * delta;
	if(dist <= step + remote.boundSize[0] * 0.5f + HitRadius(target)) {
		inst.pos = vec3(vec2(tpos), inst.pos.z);
		RemoteHit(inst, target);
		return true;
	}
	else {
		inst.dir = rel / dist;
		inst.pos += vec3(inst.dir * step, 0);
		RemoteHitAlongPath(inst);
	}
	return false;
}

bool World::UpdateGraphRemote(RemoteInst& inst, f32 delta)
{
	const Remote& remote = *inst.remote;
	const f32 step = MIN(inst.speed * delta, inst.range - inst.travelled);
	inst.travelled += step;
	if(remote.bezier && inst.range > 0) {
		const vec2 f = inst.graphDir;
		const vec2 side = -Perp(f);
		const vec2 p0 = vec2(inst.graphStart);
		const vec2 p3 = p0 + f * inst.range;
		vec2 c[2];
		for(int k = 0; k < 2; k++) {
			const f32 a = remote.bezierRotate[k] * PI / 180.0f;
			c[k] = p0 + f * (inst.range * (k + 1) / 3.0f + remote.bezierFront[k]) + side * (remote.bezierUp[k] * sinf(a));
		}
		const f32 u = MIN(inst.travelled / inst.range, 1.0f);
		const f32 v = 1.0f - u;
		const vec2 p = p0 * (v * v * v) + c[0] * (3 * v * v * u) + c[1] * (3 * v * u * u) + p3 * (u * u * u);
		const vec2 tangent = (c[0] - p0) * (3 * v * v) + (c[1] - c[0]) * (6 * v * u) + (p3 - c[1]) * (3 * u * u);
		if(glm::length(tangent) > 0.001f) inst.dir = glm::normalize(tangent);
		inst.pos = vec3(p, inst.pos.z);
	}
	else {
		inst.pos += vec3(inst.dir * step, 0);
	}
	RemoteHitAlongPath(inst);
	const bool arrived = inst.travelled >= inst.range - 0.01f;
	if(arrived && inst.hasLandZ) inst.pos.z = inst.landZ;
	return arrived;
}

bool World::UpdateLinearRemote(RemoteInst& inst, f32 delta, f32 elapsed)
{
	const Remote& remote = *inst.remote;
	bool destroyed = false;
	if(inst.speed > 0) {
		f32 step = inst.range > 0 ? MIN(inst.speed * delta, inst.range - inst.travelled) : inst.speed * delta;
		bool hitWall = false;
		if(!remote.ignoreGround && !remote.attachFireObject && step > 0) {
			f32 wallDist = 0;
			const vec2 a = vec2(inst.pos);
			if(physics.RaycastStatic(vec3(a, inst.wallZ), vec3(a + inst.dir * step, inst.wallZ), &wallDist, (i32)players[inst.ownerIndex].team)) {
				step = wallDist;
				hitWall = true;
			}
		}
		const vec2 from = vec2(inst.pos);
		const vec2 to = from + inst.dir * step;
		const f32 halfWidth = remote.boundSize[0] * 0.5f;
		const f32 boxFront = remote.boundType == Remote::BoundType::E_BOUND_BOX ? MAX(remote.offsetY + remote.boundSize[1] * 0.5f, 0.0f) : 0.0f;
		if(remote.damageGroup != Remote::DamageGroup::eNONE && remote.damageGroup != Remote::DamageGroup::INVALID) {
			foreach(it, players) {
				if(!RemoteCanAffect(inst, *it, false)) continue;
				const vec3 pos = it->body->GetWorldPos();
				if(fabsf(pos.z - inst.pos.z) > WEAPON_PROJECTILE_HIT_HEIGHT) continue;
				if(!LagSegmentHits(*it, from, to, halfWidth + HitRadius(*it) + PROJECTILE_HIT_TOLERANCE, boxFront + HitRadius(*it))) continue;
				RemoteHit(inst, *it);
				inst.hitsLeft--;
				if(inst.stopAtFirstHit && it->team != players[inst.ownerIndex].team) inst.hitsLeft = 0;
				if(inst.hitsLeft <= 0) {
					if(inst.clientRemoteID != 0) replication->SendDestroyRemoteToAll(inst.clientRemoteID);
					inst.destroyedByHit = true;
					inst.pos = vec3(vec2(pos), inst.pos.z);
					destroyed = true;
					break;
				}
			}
		}
		if(!destroyed) {
			inst.pos += vec3(inst.dir * step, 0);
			inst.travelled += step;
			if(hitWall || (inst.range > 0 && inst.travelled >= inst.range - 0.01f)) destroyed = true;
			if(hitWall) LOG("Remote :: %d stopped by a wall after %.0f", (i32)remote.ID, inst.travelled);
		}
	}
	else if(!inst.areaDone) {
		if(remote.activateDelay <= 0 || elapsed >= remote.activateDelay) {
			inst.areaDone = true;
			RemoteApplyArea(inst);
		}
	}
	else if(remote.lifeTime > 0 && inst.hitsLeft > 0) {
		RemoteApplyArea(inst);
	}
	if(inst.hitsLeft <= 0 && remote.lifeTime > 0) {
		if(inst.clientRemoteID != 0) replication->SendDestroyRemoteToAll(inst.clientRemoteID);
		inst.destroyedByHit = true;
		destroyed = true;
	}
	return destroyed;
}

void World::DestroyRemote(i32 index)
{
	RemoteInst& inst = remoteInsts[index];
	const Remote& remote = *inst.remote;
	const i32 i = index;
	if(inst.hasLandZ) inst.pos.z = inst.landZ;
	const bool destroySent = inst.clientRemoteID != 0 && (inst.destroyedByHit || inst.speed > 0 || inst.killNow);
	if(!inst.destroyedByHit && destroySent) replication->SendDestroyRemoteToAll(inst.clientRemoteID);
	{
		const RemoteIdx next = remote.destroyCreateRemote;
		const RemoteInst copy = inst;
		remoteInsts.erase_unsorted(remoteInsts.begin() + i);
		if(!copy.chainEnded) {
			const i32 childExclude = remote.behaviorType == Remote::BehaviorType::CHAIN ? copy.lastHitIndex : copy.excludeIndex;
			if(next != RemoteIdx::INVALID) {
				const Remote* nextRemote = GetGameXmlContent().FindRemote(next);
				const f32 nextOffset = nextRemote ? nextRemote->offsetY : 0;
				RemoteInst* child = SpawnRemote(copy.ownerIndex, CostReplacedRemote(copy.costLevel, next), copy.pos + vec3(copy.dir * nextOffset, 0), copy.dir, copy.target, copy.skillID, copy.castID, copy.damage, copy.clientRemoteID != 0, nullptr, 0, copy.casterLocal);
				if(child) {
					child->excludeIndex = childExclude;
					child->costLevel = copy.costLevel;
					child->centeredBox = copy.centeredBox;
				}
			}
			SpawnRemoteRules(*this, copy, remote.onDestroy, copy.pos, childExclude, next, !destroySent);
		}
	}
}

void World::ExpireHitRecords()
{
	for(auto r = repeatHits.begin(); r != repeatHits.end(); ) {
		if(TimeDiffSec(TimeDiff(r->at, localTime)) > 10.0) r = repeatHits.erase_unsorted(r);
		else ++r;
	}
	for(auto h = castHits.begin(); h != castHits.end(); ) {
		if(localTime >= h->until) h = castHits.erase_unsorted(h);
		else ++h;
	}
}
