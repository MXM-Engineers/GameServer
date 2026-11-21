#include "config.h"
#include "world.h"
#include "world_common.h"
#include <mxm/game_content.h>

static const i32 STAT_TOUGHNESS = 65;
static const f32 TOOLTIP_DEFENDER_CC_IMMUNE_SEC = 1.0f;
static const f32 TOOLTIP_DEATHKNIGHT_REVIVE_HP = 0.30f;
static const f64 SELF_DRAG_LINK_SEC = 0.6;
static const f32 STATUS_PUSH_BASE_DISTANCE = 500.0f;
static const f32 STATUS_LIFT_BASE_HEIGHT = 400.0f;
static const f32 FLY_PULL_MAX_SCALE = 2.0f;

bool StatusSendsGraph(const StatusInfo* info)
{
	if(!info) return false;
	if(info->blockMove) return true;
	switch(info->type) {
		case StatusType::MOTION:
		case StatusType::ROOT:
		case StatusType::STUN:
		case StatusType::FREEZE:
		case StatusType::AIRBORNE:
		case StatusType::PUSH:
		case StatusType::TARGET:
		case StatusType::FORCEBACK:
		case StatusType::DRAGGEDBYFORCE:
		case StatusType::FLY:
		case StatusType::DRAGGED:
		case StatusType::KNOCKDOWN:
		case StatusType::SHOCKED:
		case StatusType::UNDEAD:
		case StatusType::DANCE:
			return true;
		default: return false;
	}
}

static bool StatusGraphTimed(const StatusInfo* info)
{
	if(!info) return false;
	switch(info->type) {
		case StatusType::ROOT:
		case StatusType::STUN:
		case StatusType::FREEZE:
		case StatusType::SHOCKED:
		case StatusType::TARGET:
		case StatusType::UNDEAD:
		case StatusType::DANCE:
			return false;
		default: return true;
	}
}

bool StatusGraphAnchorOnly(const StatusInfo* info)
{
	if(!info) return false;
	switch(info->type) {
		case StatusType::ROOT:
		case StatusType::STUN:
		case StatusType::FREEZE:
		case StatusType::SHOCKED:
		case StatusType::TARGET:
		case StatusType::DANCE:
			return true;
		default: return false;
	}
}
static const i32 STATUS_PC_DEFEAT_SUPERARMOR = 1230000101;
static const i32 STATUS_PC_DEFEAT_AND_LIE_SUPERARMOR = 1230000102;
static const f32 DEFEAT_PUSH_SEC = 1.0f;
static const f32 DEFEAT_KNOCKDOWN_SEC = 1.5f;
static const f32 DEFEAT_FLY_START_SEC = 1.0f;
static const f32 DEFEAT_FLY_END_SEC = 0.5f;

static i32 DefeatGuardStatus(const StatusInfo* info)
{
	if(!info) return 0;
	switch(info->type) {
		case StatusType::PUSH: return STATUS_PC_DEFEAT_SUPERARMOR;
		case StatusType::KNOCKDOWN:
		case StatusType::AIRBORNE:
		case StatusType::FLY: return STATUS_PC_DEFEAT_AND_LIE_SUPERARMOR;
		default: return 0;
	}
}

static bool DefeatGraphTimes(const StatusInfo* info, f32* duration, f32* graph)
{
	if(!info || info->timeRatio <= 0) return false;
	switch(info->type) {
		case StatusType::PUSH: *duration = *graph = DEFEAT_PUSH_SEC * info->timeRatio; return true;
		case StatusType::KNOCKDOWN: *duration = *graph = DEFEAT_KNOCKDOWN_SEC * info->timeRatio; return true;
		case StatusType::FLY:
		case StatusType::AIRBORNE:
			*graph = DEFEAT_FLY_START_SEC * info->timeRatio;
			*duration = (DEFEAT_FLY_START_SEC + DEFEAT_FLY_END_SEC) * info->timeRatio;
			return true;
		default: return false;
	}
}

const World::Player* World::AttachHolder(const Player& p) const
{
	foreach_const(it, activeStatuses) {
		if(it->targetIndex != p.index || !it->info || !it->info->attachToCaster || it->until <= localTime) continue;
		const ActorMaster* caster = FindMasterActor(it->casterUID);
		if(caster && caster->parent && caster->parent != &p) return caster->parent;
	}
	return nullptr;
}

bool World::AttachedToCaster(const Player& p) const
{
	foreach_const(it, activeStatuses) {
		if(it->targetIndex == p.index && it->info && it->info->attachToCaster && it->until > localTime) return true;
	}
	return false;
}

bool World::HasStatusType(const Player& p, StatusType type) const
{
	foreach_const(it, activeStatuses) {
		if(it->targetIndex == p.index && it->info && it->info->type == type) return true;
	}
	return false;
}

World::ActiveStatus* World::FindActiveStatus(u32 targetIndex, i32 statusID)
{
	foreach(it, activeStatuses) {
		if(it->targetIndex == targetIndex && it->statusID == statusID && it->until > localTime) return &*it;
	}
	return nullptr;
}

void World::ConsumeStatus(u32 targetIndex, i32 statusID)
{
	ActiveStatus* st = FindActiveStatus(targetIndex, statusID);
	if(st) st->until = localTime;
	LOG("Status :: %ls status %d consumed (%s)", players[targetIndex].name.data(), statusID, st ? "found" : "not active");
}

void World::ApplyPassives(Player& player)
{
	if(player.dead) return;
	const auto& content = GetGameXmlContent();
	foreach(c, player.characters) {
		const auto* list = content.FindPassiveStatuses((*c)->classType);
		if(!list) continue;
		for(i32 id : *list) {
			if(FindActiveStatus(player.index, id)) continue;
			statusTargetOverride = (*c)->UID;
			statusNoCaster = true;
			ApplyStatus(player, player, id);
			statusNoCaster = false;
			statusTargetOverride = ActorUID::INVALID;
			ActiveStatus* st = FindActiveStatus(player.index, id);
			if(st && st->info) st->custom = st->info->customInit;
			LOG("Passive :: %ls status %d", player.name.data(), id);
		}
	}
	player.passivesApplied = true;
}

void World::ApplyAllPassives()
{
	foreach(it, players) ApplyPassives(*it);
}

static bool PassiveOfActiveMaster(const World::Player& player, i32 statusID)
{
	const auto* list = GetGameXmlContent().FindPassiveStatuses(player.Main().classType);
	return list && eastl::find(list->begin(), list->end(), statusID) != list->end();
}

void World::PassiveCooldownsOnKill(Player& killer)
{
	const auto& content = GetGameXmlContent();
	const auto* list = content.FindPassiveStatuses(killer.Main().classType);
	if(!list) return;
	for(i32 id : *list) {
		const StatusInfo* info = content.FindStatus(id);
		if(!info || !info->coolTimeRecovery) continue;
		foreach(t, killer.botSkillReady) *t = Time::ZERO;
		killer.ultCooldownUntil = Time::ZERO;
		LOG("Passive :: %ls master kill resets cooldowns (status %d)", killer.name.data(), id);
		return;
	}
}

const World::ActiveStatus* World::FindPassiveOfType(const Player& p, StatusType type) const
{
	foreach_const(it, activeStatuses) {
		if(it->targetIndex != p.index || !it->info || it->info->type != type) continue;
		if(PassiveOfActiveMaster(p, it->statusID)) return &*it;
	}
	return nullptr;
}

World::ActiveStatus* World::FindPassiveOfType(const Player& p, StatusType type)
{
	foreach(it, activeStatuses) {
		if(it->targetIndex != p.index || !it->info || it->info->type != type) continue;
		if(PassiveOfActiveMaster(p, it->statusID)) return &*it;
	}
	return nullptr;
}

const World::ActiveStatus* World::FindCarrierPassive(const Player& carrier) const
{
	foreach_const(it, activeStatuses) {
		if(it->targetIndex == carrier.index && it->info && it->info->carrier && PassiveOfActiveMaster(carrier, it->statusID)) return &*it;
	}
	return nullptr;
}

const World::ActiveStatus* World::FindGroupTeleportPassive(const Player& caster, SkillID skillID) const
{
	foreach_const(it, activeStatuses) {
		if(it->targetIndex == caster.index && it->info && it->info->groupTeleportSkill == (i32)skillID && PassiveOfActiveMaster(caster, it->statusID)) return &*it;
	}
	return nullptr;
}

bool World::HasStatusFromSkill(const Player& target, u32 casterIndex, i32 skillID) const
{
	foreach_const(it, activeStatuses) {
		if(it->targetIndex == target.index && it->casterIndex == casterIndex && it->info && it->info->skillIndex == skillID && localTime < it->until) return true;
	}
	return false;
}

void World::SendPassiveCounter(const ActiveStatus& st, i32 value)
{
	replication->SendModifyStatusToAll(st.statusID, st.targetUID, st.casterUID, (u8)MAX(st.overlap, 1), (u8)MAX(value, 0));
}

void World::SyncPassivesOnDeath(const Player& target)
{
	foreach_const(it, activeStatuses) {
		if(it->targetIndex != target.index || !it->info || !PassiveOfActiveMaster(target, it->statusID)) continue;
		SendPassiveCounter(*it, it->info->type == StatusType::UNSHAKABLE && target.cleanseCharges >= 0 ? target.cleanseCharges : it->custom);
	}
}

bool World::PassiveBlocksCC(Player& target, const StatusInfo& info, f32* ccScale)
{
	if(info.kind != StatusInfo::Kind::HARD_CC && info.kind != StatusInfo::Kind::ROOT) return false;
	if(ShowcaseIsDummy(target)) return false;
	if(localTime < target.ccImmuneUntil) {
		LOG("Passive :: %ls immune to status %d", target.name.data(), info.ID);
		replication->SendResistStatusToAll(target.Main().UID, target.Main().UID, STATUS_RESIST_EVENT_PASSIVE);
		return true;
	}
	const f32 toughness = MasterStat(target.Main().classType, STAT_TOUGHNESS);
	if(toughness > 0 && ccScale) *ccScale = MAX(1.0f - toughness / 100.0f, 0.0f);
	return false;
}

void World::PassiveOnCC(Player& target, const StatusInfo& info)
{
	if(info.kind != StatusInfo::Kind::HARD_CC && info.kind != StatusInfo::Kind::ROOT) return;
	if(ShowcaseIsDummy(target)) return;
	const ActiveStatus* st = FindPassiveOfType(target, StatusType::UNSHAKABLE);
	if(!st) return;
	if(target.cleanseCharges < 0) target.cleanseCharges = st->info->customInit;
	const i32 defiance = (i32)st->info->ParamInt("_CustomAddStatusIndex");
	if(target.cleanseCharges <= 0 || defiance == 0) return;
	target.cleanseCharges--;
	SendPassiveCounter(*st, target.cleanseCharges);
	ActiveStatus* held = FindActiveStatus(target.index, defiance);
	if(held && held->until > localTime) {
		held->custom = MIN(held->custom + 1, MAX(held->info->customMax, 1));
		SendPassiveCounter(*held, held->custom);
	}
	else {
		ApplyStatus(target, target, defiance, 0, STATUS_INFINITE_DURATION_SEC);
		held = FindActiveStatus(target.index, defiance);
		if(held && held->info) held->custom = MAX(held->info->customInit, 1);
	}
	LOG("Passive :: %ls struck by status %d gains defiance %d (charges left %d)", target.name.data(), info.ID, defiance, target.cleanseCharges);
}

bool World::PassiveOnLethal(Player& target)
{
	foreach(it, activeStatuses) {
		if(it->targetIndex != target.index || !it->info || it->info->type != StatusType::SECOND_CHANCE || it->until <= localTime) continue;
		ActorMaster& chara = target.Main();
		chara.hp = MAX((i32)(chara.hpMax * it->info->paramRatio), 1);
		it->until = localTime;
		LOG("Status :: %ls second chance %d -> hp %d", target.name.data(), it->statusID, chara.hp);
		return true;
	}
	const ActiveStatus* st = FindPassiveOfType(target, StatusType::ACTIVATE_UNDEAD);
	if(!st || localTime < target.reviveReadyAt) return false;
	const i32 undead = (i32)st->info->ParamInt("_UndeadStatusIndex");
	const StatusInfo* undeadInfo = GetGameXmlContent().FindStatus(undead);
	target.Main().hp = 1;
	target.reviveReadyAt = TimeAdd(localTime, TimeMsToTime(st->info->Param("_DelayMS")));
	target.reviveAt = TimeAdd(localTime, TimeMsToTime((undeadInfo ? undeadInfo->durationSec : 0.0f) * 1000.0));
	target.reviveRatio = TOOLTIP_DEATHKNIGHT_REVIVE_HP;
	ApplyStatus(target, target, undead);
	if(st->info->skillIndex != 0 && GetGameXmlContent().SkillActionKnown(target.Main().classType, ActionStateID::EXTRAMOTION_3_BEHAVIORSTATE, target.Main().stance)) {
		PlayerCastSkill(target, SkillID(st->info->skillIndex), target.body->GetWorldPos(), Slice<const ActorUID>(), 0, ActionStateID::EXTRAMOTION_3_BEHAVIORSTATE);
	}
	LOG("Passive :: %ls cheated death (undead status %d)", target.name.data(), undead);
	return true;
}

void World::PassiveOnDamaged(Player& target)
{
	if(target.dead) return;
	if(ShowcaseIsDummy(target)) return;
	const ActiveStatus* st = FindPassiveOfType(target, StatusType::HPTRIGGER);
	if(!st) return;
	const ActorMaster& chara = target.Main();
	if(chara.hp * 100 >= chara.hpMax * st->info->Param("_CheckVal")) return;
	const i32 buff = (i32)st->info->ParamInt("_BuffStatus");
	const i32 creature = (i32)st->info->ParamInt("_SummonCreature");
	if(creature == 0 || buff == 0 || FindActiveStatus(target.index, buff) || pendingSummons.full()) return;
	ApplyStatus(target, target, buff);
	PendingSummon ps;
	ps.time = localTime;
	ps.creature = creature;
	ps.ownerIndex = target.index;
	ps.pos = target.body->GetWorldPos();
	ps.rot = vec3(0, 0, target.input.rot.upperYaw);
	pendingSummons.push_back(ps);
	LOG("Passive :: %ls low health -> summon %d", target.name.data(), creature);
}

void World::PulseAura(u32 casterIndex, const StatusInfo& aura, const vec2& center, f32 range, i32 skipDebuffIndex, ActorUID sourceUID)
{
	const u8 casterTeam = players[casterIndex].team;
	foreach_const(id, aura.auraStatuses) {
		const StatusInfo* child = GetGameXmlContent().FindStatus(*id);
		if(!child) continue;
		const bool bound = child->durationSec <= 0;
		foreach(p, players) {
			const bool casterAlly = p->team == casterTeam;
			if((child->isDebuff || aura.auraEnemies) == casterAlly) continue;
			if(child->isDebuff && (i32)p->index == skipDebuffIndex) continue;
			bool inside = !p->dead;
			if(inside && glm::length(vec2(p->body->GetWorldPos()) - center) > range + HitRadius(*p)) inside = false;
			ActiveStatus* cur = FindActiveStatus(p->index, *id);
			if(!inside) {
				if(cur && cur->casterIndex == casterIndex) cur->until = localTime;
				continue;
			}
			if(cur) continue;
			if(pendingStatuses.full()) return;
			PendingStatus ps;
			ps.time = localTime;
			ps.statusID = *id;
			ps.casterIndex = casterIndex;
			ps.targetUID = p->Main().UID;
			ps.duration = bound ? STATUS_INFINITE_DURATION_SEC : child->durationSec;
			ps.power = 0;
			ps.sourceUID = sourceUID;
			pendingStatuses.push_back(ps);
		}
		const Player& caster = players[casterIndex];
		const ActorUID npcCaster = sourceUID != ActorUID::INVALID ? sourceUID : caster.Main().UID;
		const Faction casterFaction = casterTeam == 0 ? Faction::RED : Faction::BLUE;
		const bool towardEnemies = child->isDebuff || aura.auraEnemies;
		foreach(npc, actorNpcList) {
			if(!(towardEnemies ? NpcHittableBy(*npc, caster) : npc->faction == casterFaction)) continue;
			bool inside = npc->hp > 0 && npc->removeAt == Time::ZERO;
			if(inside && glm::length(vec2(npc->pos) - center) > range + NpcHitRadius(*npc)) inside = false;
			NpcStatus* cur = nullptr;
			foreach(ns, npcStatuses) if(ns->npcUID == npc->UID && ns->statusID == *id && localTime < ns->until) cur = &*ns;
			if(!inside) {
				if(cur && (cur->casterUID == npcCaster || (sourceUID == ActorUID::INVALID && cur->casterUID == caster.Sub().UID))) cur->until = localTime;
				continue;
			}
			if(cur) continue;
			if(bound) {
				if(npcStatuses.full()) continue;
				replication->SendAddStatusToAll(*id, npc->UID, npcCaster, STATUS_INFINITE_DURATION_SEC, npc->pos, npc->pos, 0, 1, 1.0f, -1.0f, false, false);
				NpcStatus ns;
				ns.npcUID = npc->UID;
				ns.statusID = *id;
				ns.casterUID = npcCaster;
				ns.until = TimeAdd(localTime, TimeMsToTime((f64)STATUS_INFINITE_DURATION_SEC * 1000.0));
				ns.linked = false;
				npcStatuses.push_back(ns);
				continue;
			}
			if(pendingStatuses.full()) continue;
			PendingStatus ps;
			ps.time = localTime;
			ps.statusID = *id;
			ps.casterIndex = casterIndex;
			ps.targetUID = npc->UID;
			ps.duration = child->durationSec;
			ps.power = 0;
			ps.sourceUID = sourceUID;
			pendingStatuses.push_back(ps);
		}
	}
}

void World::EndAura(u32 casterIndex, const StatusInfo& aura, ActorUID sourceUID)
{
	foreach_const(id, aura.auraStatuses) {
		foreach(it, activeStatuses) {
			if(it->statusID == *id && it->casterIndex == casterIndex && it->info && it->info->durationSec <= 0) it->until = localTime;
		}
		const StatusInfo* child = GetGameXmlContent().FindStatus(*id);
		if(child && child->durationSec <= 0) foreach(ns, npcStatuses) if(ns->statusID == *id && (ns->casterUID == players[casterIndex].Main().UID || ns->casterUID == players[casterIndex].Sub().UID || (sourceUID != ActorUID::INVALID && ns->casterUID == sourceUID)) && !ns->linked) ns->until = localTime;
	}
}

void World::UpdatePassives()
{
	foreach(it, players) {
		if(it->reviveAt != Time::ZERO && localTime >= it->reviveAt) {
			it->reviveAt = Time::ZERO;
			if(!it->dead) {
				ActorMaster& chara = it->Main();
				chara.hp = MAX(chara.hp, (i32)(chara.hpMax * it->reviveRatio));
				replication->SendHpToAll(chara.UID, chara.hp, chara.hpMax);
				LOG("Passive :: %ls revived at %d hp", it->name.data(), chara.hp);
			}
		}
		if(it->cleanseCharges < 0 && !it->dead) {
			if(const ActiveStatus* st = FindPassiveOfType(*it, StatusType::UNSHAKABLE)) {
				it->cleanseCharges = st->info->customInit;
				it->cleanseRefillAt = TimeAdd(localTime, TimeMsToTime(st->info->Param("_RefillDuration")));
			}
		}
		if(it->cleanseRefillAt != Time::ZERO && localTime >= it->cleanseRefillAt) {
			const ActiveStatus* st = FindPassiveOfType(*it, StatusType::UNSHAKABLE);
			if(!st) it->cleanseRefillAt = Time::ZERO;
			else {
				it->cleanseRefillAt = TimeAdd(it->cleanseRefillAt, TimeMsToTime(st->info->Param("_RefillDuration")));
				if(it->cleanseCharges < st->info->customMax) {
					it->cleanseCharges++;
					SendPassiveCounter(*st, it->cleanseCharges);
				}
			}
		}
	}
}

void World::PassiveOnSkillCast(Player& player, i32 skillID)
{
	if(GetGameXmlContent().GetSkillConsumeStamina(skillID) > 0) return;
	bool ownSkill = false;
	if(player.Main().classType != ClassType::NONE) {
		foreach_const(sid, GetGameXmlContent().GetMaster(player.Main().classType).skillIDs) if((i32)*sid == skillID) ownSkill = true;
	}
	if(!ownSkill) return;
	if(const ActiveStatus* stacking = FindPassiveOfType(player, StatusType::DOUBLEGUN)) {
		const StatusInfo& info = *stacking->info;
		if(info.stackMax > 0) {
			player.skillStacks = localTime < player.skillStacksUntil ? (u8)MIN((i32)player.skillStacks + 1, info.stackMax) : 1;
			player.skillStacksUntil = TimeAdd(localTime, TimeMsToTime(info.stackDurationSec * 1000.0));
			const i32 passiveID = stacking->statusID;
			LOG("Passive :: %ls skill stack %d/%d", player.name.data(), (i32)player.skillStacks, info.stackMax);
			for(i32 k = 1; k <= (i32)player.skillStacks; k++) ApplyStatus(player, player, passiveID + k);
		}
	}
	eastl::fixed_vector<i32,4,false> toApply;
	foreach(it, activeStatuses) {
		if(it->targetIndex != player.index || !it->info || it->info->customMax <= 0) continue;
		if(!PassiveOfActiveMaster(player, it->statusID)) continue;
		if(it->info->type == StatusType::SKILLSTATUS) {
			it->custom++;
			LOG("Passive :: %ls skill counter %d/%d", player.name.data(), it->custom, it->info->customMax);
			if(it->custom >= it->info->customMax) {
				it->custom = it->info->customInit;
				foreach_const(id, it->info->auraStatuses) if(!toApply.full()) toApply.push_back(*id);
			}
			SendPassiveCounter(*it, it->custom);
		}
	}
	foreach_const(id, toApply) {
		LOG("Passive :: %ls counter full -> status %d", player.name.data(), *id);
		ApplyStatus(player, player, *id);
	}

	if(ActiveStatus* st = FindPassiveOfType(player, StatusType::SKILLSTAMINA)) {
		ActiveStatus& counter = *st;
		counter.custom++;
		if(st->info->customMax > 0 && counter.custom >= st->info->customMax) counter.custom = 0;
		SendPassiveCounter(counter, counter.custom);
		if(counter.custom == 0) {
			const f32 staminaMax = MasterStat(player.Main().classType, STAT_STAMINA);
			if(staminaMax > 0) {
				player.stamina = staminaMax;
				player.staminaDirty = true;
			}
			LOG("Passive :: %ls stamina restored after %d skills", player.name.data(), st->info->customMax);
		}
	}
	if(const ActiveStatus* st = FindPassiveOfType(player, StatusType::DEFENDER)) {
		const auto used = GetGameXmlContent().skillMap.find(SkillID(skillID));
		if(used != GetGameXmlContent().skillMap.end() && used->second.type == SkillType::SHIRK && localTime >= player.dodgeRageReadyAt && st->info->Param("_DodgeRecoveryRP") > 0) {
			GainResource(player, 1, st->info->Param("_DodgeRecoveryRP"));
			player.dodgeRageReadyAt = TimeAdd(localTime, TimeMsToTime(st->info->Param("_DodgeRecoveryRPCooltime")));
		}
		if(st->info->ListHas("DATA", skillID)) {
			HealPlayer(player, player, (i32)(player.Main().hpMax * st->info->Param("_RecoveryFactorHP") / 100.0f));
			player.ccImmuneUntil = TimeAdd(localTime, TimeMsToTime(TOOLTIP_DEFENDER_CC_IMMUNE_SEC * 1000.0));
		}
	}
	for(const PassiveStatusRule& rule : PASSIVE_STATUS_RULES) {
		if(rule.master == player.Main().classType && rule.trigger == PassiveTrigger::SKILL_USE) ApplyPassiveStatusRule(rule, player, nullptr);
	}
}

void World::ApplyPassiveStatusRule(const PassiveStatusRule& rule, Player& owner, Player* hitTarget)
{
	if(rule.cooldownStatus != 0 && FindActiveStatus(owner.index, rule.cooldownStatus)) return;
	switch(rule.target) {
		case PassiveTarget::SELF: ApplyStatus(owner, owner, rule.status); break;
		case PassiveTarget::HIT_TARGET: ASSERT(hitTarget); ApplyStatus(owner, *hitTarget, rule.status); break;
		case PassiveTarget::LOWEST_HP_ALLY_IN_STATUS_RANGE: {
			const StatusInfo* info = GetGameXmlContent().FindStatus(rule.status);
			ASSERT(info);
			const f32 range = info->Param("_TargetingRange");
			Player* lowest = nullptr;
			foreach(it, players) {
				if(it->team != owner.team || it->dead) continue;
				if(glm::length(vec2(it->body->GetWorldPos()) - vec2(owner.body->GetWorldPos())) > range) continue;
				const f32 ratio = (f32)it->Main().hp / MAX(it->Main().hpMax, 1);
				if(!lowest || ratio < (f32)lowest->Main().hp / MAX(lowest->Main().hpMax, 1)) lowest = &*it;
			}
			if(lowest) HealPlayer(owner, *lowest, rule.heal);
		} break;
	}
	if(rule.cooldownStatus != 0) ApplyStatus(owner, owner, rule.cooldownStatus);
}

void World::PassiveStackOnHit(Player& attacker, Player& target, i32 remoteID, SkillID skillID)
{
	if(target.dead || target.team == attacker.team) return;
	i32 apply = 0;
	foreach_const(it, activeStatuses) {
		if(it->targetIndex != attacker.index || !it->info || it->info->type != StatusType::STACKATTACK) continue;
		if(!PassiveOfActiveMaster(attacker, it->statusID)) continue;
		const StatusInfo& info = *it->info;
		if(skillID != SkillID::INVALID && eastl::find(info.fullStackSkills.begin(), info.fullStackSkills.end(), (i32)skillID) != info.fullStackSkills.end()) apply = info.stackFullStatus;
		else if(eastl::find(info.stackRemotes.begin(), info.stackRemotes.end(), remoteID) != info.stackRemotes.end()) apply = info.stackStatus;
		break;
	}
	if(apply != 0) ApplyStatus(attacker, target, apply);
}

void World::OnEvade(Player& target)
{
	eastl::fixed_vector<i32,4,false> toApply;
	foreach_const(st, activeStatuses) {
		if(st->targetIndex != target.index || !st->info || st->info->type != StatusType::STATUS_ON_EVADE || !st->info->hitEffectStatus) continue;
		if(st->info->permanent && !PassiveOfActiveMaster(target, st->statusID)) continue;
		if(!toApply.full()) toApply.push_back(st->info->hitEffectStatus);
	}
	foreach_const(s, toApply) {
		LOG("Passive :: %ls evaded -> status %d", target.name.data(), *s);
		ApplyStatus(target, target, *s);
	}
}

void World::PassiveOnWeaponStatusHit(Player& attacker, Player& target)
{
	eastl::fixed_vector<i32,4,false> toApply;
	foreach(it, activeStatuses) {
		if(it->targetIndex != attacker.index || !it->info || it->until <= localTime) continue;
		const StatusInfo& info = *it->info;
		if(info.type == StatusType::HIT_PLAGUE && info.customMax <= 0 && !info.permanent) {
			if(info.hitEffectStatus && !toApply.full()) toApply.push_back(info.hitEffectStatus);
			continue;
		}
		if(!PassiveOfActiveMaster(attacker, it->statusID)) continue;
		if(info.type == StatusType::HIT_PLAGUE && info.customMax > 0) {
			it->custom++;
			if(it->custom >= info.customMax) {
				it->custom = info.customInit;
				if(info.hitEffectStatus && !toApply.full()) toApply.push_back(info.hitEffectStatus);
			}
			SendPassiveCounter(*it, it->custom);
		}
		else if(info.type == StatusType::KROMEDE && info.hitEffectStatus) {
			if(HasStatusType(target, StatusType::STUN) || HasStatusType(target, StatusType::ROOT) || HasStatusType(target, StatusType::SHOCKED)) {
				if(!toApply.full()) toApply.push_back(info.hitEffectStatus);
			}
		}
		else if(info.type == StatusType::MANABURN && info.hitEffectStatus) {
			if(FindActiveStatus(target.index, info.hitEffectStatus)) {
				ConsumeStatus(target.index, info.hitEffectStatus);
				f32 stolen = info.hitEffectValue;
				if(target.mana >= 0) { stolen = MIN(stolen, target.mana); target.mana -= stolen; target.manaDirty = true; }
				if(attacker.mana >= 0) { attacker.mana += stolen; attacker.manaDirty = true; }
				LOG("Passive :: %ls drained %.0f mana from %ls", attacker.name.data(), stolen, target.name.data());
			}
		}
	}
	foreach_const(id, toApply) {
		LOG("Passive :: %ls weapon hit -> status %d on %ls", attacker.name.data(), *id, target.name.data());
		ApplyStatus(attacker, target, *id);
	}
}

void World::WeaponHitPlagueNpc(Player& attacker, ActorNpc& npc)
{
	if(npc.hp <= 0 || npc.ownerUID != ActorUID::INVALID) return;
	bool controlled = false;
	foreach_const(ns, npcStatuses) {
		if(ns->npcUID != npc.UID || localTime >= ns->until) continue;
		const StatusInfo* held = GetGameXmlContent().FindStatus(ns->statusID);
		if(held && (held->type == StatusType::STUN || held->type == StatusType::ROOT || held->type == StatusType::SHOCKED)) controlled = true;
	}
	eastl::fixed_vector<i32,4,false> toApply;
	foreach(it, activeStatuses) {
		if(it->targetIndex != attacker.index || !it->info || it->until <= localTime) continue;
		const StatusInfo& info = *it->info;
		if(info.type == StatusType::HIT_PLAGUE && info.customMax <= 0 && !info.permanent) {
			if(info.hitEffectStatus && !toApply.full()) toApply.push_back(info.hitEffectStatus);
			continue;
		}
		if(!PassiveOfActiveMaster(attacker, it->statusID)) continue;
		if(info.type == StatusType::HIT_PLAGUE && info.customMax > 0) {
			it->custom++;
			if(it->custom >= info.customMax) {
				it->custom = info.customInit;
				if(info.hitEffectStatus && !toApply.full()) toApply.push_back(info.hitEffectStatus);
			}
			SendPassiveCounter(*it, it->custom);
		}
		else if(info.type == StatusType::KROMEDE && info.hitEffectStatus && controlled && !toApply.full()) toApply.push_back(info.hitEffectStatus);
	}
	foreach_const(id, toApply) ApplyStatusToNpc(attacker, npc, *id, false);
}

i32 World::StanceUpgradedStatus(Player& player, i32 statusID)
{
	const ActorMaster& m = player.Main();
	if(m.stance == 0 || m.stanceSkill == SkillID::INVALID) return statusID;
	const GameXmlContent::CostLevel* mods = UpgradeCastMods(player, m.stanceSkill, nullptr);
	if(mods) foreach_const(sr, mods->statusReplace) if(sr->statusID == statusID) return sr->replaceID;
	return statusID;
}

void World::PassiveOnWeaponHit(Player& attacker, Player& target, const Projectile& proj)
{
	PassiveStackOnHit(attacker, target, (i32)proj.remoteDoc, SkillID::INVALID);
	PassiveOnWeaponStatusHit(attacker, target);
	if(proj.remoteDoc != RemoteIdx::INVALID && !target.dead) {
		const Remote* remote = GetGameXmlContent().FindRemote(proj.remoteDoc);
		if(remote) {
			foreach_const(st, remote->statuses) {
				if(st->rate >= 100 || (i32)(RandUint() % 100) < st->rate) ApplyStatus(attacker, target, StanceUpgradedStatus(attacker, st->statusID));
			}
		}
	}

	if(target.dead) return;
	for(const WeaponBonusRule& rule : WEAPON_BONUS_RULES) {
		if(rule.master != attacker.Main().classType || rule.condition != WeaponBonus::LAST_OF_CHAIN) continue;
		attacker.weaponHitCounter = attacker.weaponHitCounter % rule.count + 1;
		if(attacker.weaponHitCounter < rule.count) ApplyStatus(attacker, target, rule.status);
	}
}

void World::PassiveOnSkillHit(Player& attacker, Player& target)
{
	for(const PassiveStatusRule& rule : PASSIVE_STATUS_RULES) {
		if(rule.master == attacker.Main().classType && rule.trigger == PassiveTrigger::SKILL_HIT) ApplyPassiveStatusRule(rule, attacker, &target);
	}
	foreach_const(it, activeStatuses) {
		if(it->targetIndex != attacker.index || !it->info) continue;
		if(it->info->type == StatusType::MANABURN && it->info->hitEffectStatus && PassiveOfActiveMaster(attacker, it->statusID)) {
			const i32 mark = it->info->hitEffectStatus;
			ApplyStatus(attacker, target, mark);
			return;
		}
	}
}

f32 World::PassiveWeaponDamageMul(Player& attacker, Player& target)
{
	f32 mul = 1.0f;
	foreach_const(it, activeStatuses) {
		if(it->targetIndex != attacker.index || !it->info) continue;
		if(it->info->type == StatusType::GHOSTWIDOW && PassiveOfActiveMaster(attacker, it->statusID)) {
			i32 debuffs = 0;
			foreach_const(d, activeStatuses) {
				if(d->targetIndex == target.index && d->info && d->info->isDebuff && d->until > localTime) debuffs++;
			}
			mul *= 1.0f + it->info->hitEffectValue * debuffs;
		}
	}
	for(const WeaponBonusRule& rule : WEAPON_BONUS_RULES) {
		if(rule.master != attacker.Main().classType) continue;
		switch(rule.condition) {
			case WeaponBonus::TARGET_HAS_OWN_STATUS: {
				foreach_const(st, activeStatuses) {
					if(st->statusID != rule.status || st->targetIndex != target.index || st->casterIndex != attacker.index || st->until <= localTime) continue;
					mul *= rule.mul;
					break;
				}
			} break;
			case WeaponBonus::EVERY_NTH_HIT: {
				attacker.weaponHitCounter++;
				if(attacker.weaponHitCounter >= rule.count) {
					attacker.weaponHitCounter = 0;
					mul *= rule.mul;
					ApplyStatus(attacker, target, rule.status);
				}
			} break;
			case WeaponBonus::LAST_OF_CHAIN: {
				if(attacker.weaponHitCounter + 1 == rule.count) mul *= rule.mul;
			} break;
		}
	}
	return mul;
}

static f32 StatusShieldLayer(const StatusInfo& info, f32 skillPower, f32 power)
{
	f32 dataShield = 0;
	foreach_const(m, info.mods) {
		if(m->stat == StatusMod::SHIELD_DEFENCE && !m->mul) dataShield += m->value;
	}
	return dataShield > 0 ? (f32)(i32)(dataShield + skillPower * info.shieldMul) : MAX(power, 100.0f) * (info.paramRatio > 0 ? info.paramRatio : 1.0f);
}

void World::ApplyStatusControl(Player& caster, Player& target, const StatusInfo* info, i32 statusID, f32 duration, f32 graphDuration, f32 ccScale, Time until, bool displacement, vec3* moveFrom, vec3* moveTo, f32* moveDist)
{
	if(info && (info->kind == StatusInfo::Kind::HARD_CC || info->kind == StatusInfo::Kind::ROOT)) {
		if(caster.team != target.team) caster.ccApplied++;
		Time ccEnd = until;
		if(info->type == StatusType::AIRBORNE || info->type == StatusType::FLY || info->type == StatusType::KNOCKDOWN) {
			const auto& content = GetGameXmlContent();
			const f32 recover = info->lyingTime + content.GetLieEndTime(content.GetMaster(target.Main().classType).className.c_str());
			ccEnd = TimeAdd(ccEnd, TimeMsToTime(recover * 1000.0));
		}
		foreach(a, activeStatuses) {
			if(a->statusID != statusID || a->targetIndex != target.index) continue;
			a->ccStatusEnd = until;
			a->ccEnd = ccEnd;
			break;
		}
		if(ccEnd > target.ccUntil) target.ccUntil = ccEnd;
		const ActorNpc* npcCaster = &caster == &target && statusCasterOverride != ActorUID::INVALID ? FindNpcActor(statusCasterOverride) : nullptr;
		const bool hostile = &caster != &target ? caster.team != target.team : npcCaster && NpcHittableBy(*npcCaster, target);
		if(info->kind == StatusInfo::Kind::HARD_CC && hostile && localTime < target.dashUntil && target.dashDone < target.dashDist) {
			target.dashDone = target.dashDist;
			target.dashUntil = localTime;
			LOG("Status :: %ls dash stopped by cc %d", target.name.data(), statusID);
		}
	}

	const bool chainedPush = target.clientHd == ClientHandle::INVALID ? localTime < target.pushUntil : localTime < target.clientPushUntil;
	const bool dragForce = info && info->type == StatusType::DRAGGEDBYFORCE;
	const vec3 from = chainedPush ? vec3(vec2(target.pushTarget), target.body->GetWorldPos().z) : target.body->GetWorldPos();
	vec3 to = from;
	f32 nominalDist = -1.0f;
	vec3 forceCenter = from;
	const f32 distanceRatio = statusDistanceRatioOverride != 0 ? statusDistanceRatioOverride : (info ? info->distanceRatio : 0);
	const bool flyMove = info && info->type == StatusType::FLY && distanceRatio != 0;
	if(info && displacement && info->type != StatusType::AIRBORNE && (info->type != StatusType::FLY || flyMove)) {
		vec2 dir = vec2(from - caster.body->GetWorldPos());
		const f32 len = glm::length(dir);
		dir = len > 1.0f ? dir / len : vec2(cosf(caster.input.rot.upperYaw), sinf(caster.input.rot.upperYaw));
		if((info->type == StatusType::PUSH || info->type == StatusType::FLY) && statusPushDirSet) dir = glm::normalize(statusPushDir);
		if(statusForceCenterSet && (info->type == StatusType::KNOCKDOWN || (&caster == &target && !statusPushDirSet && info->type != StatusType::DRAGGED))) {
			const vec2 radial = vec2(from) - vec2(statusForceCenter);
			if(glm::length(radial) > 1.0f) dir = glm::normalize(radial);
		}
		if(info->type == StatusType::FLY && statusFromBehind) dir = -dir;
		if(dragForce) {
			forceCenter = statusForceCenterSet ? statusForceCenter : caster.body->GetWorldPos();
			const vec2 rel = vec2(forceCenter) - vec2(from);
			const f32 centerDist = glm::length(rel);
			const f32 travel = (info->dragForceMin + info->dragForceMax) * 0.5f * duration;
			dir = centerDist > 0.001f ? rel / centerDist : vec2(0);
			to = from + vec3(dir * (travel > centerDist ? centerDist : travel), 0);
		}
		else if(info->type == StatusType::DRAGGED && &caster == &target) {
			const bool hooked = caster.lastHitTargetIndex >= 0 && caster.lastHitTargetIndex != (i32)caster.index && TimeDiffSec(TimeDiff(caster.lastHitAt, localTime)) <= SELF_DRAG_LINK_SEC;
			if(hooked) {
				const Player& hookedPlayer = players[caster.lastHitTargetIndex];
				const vec3 hp = hookedPlayer.body->GetWorldPos();
				const vec2 rel = vec2(hp - from);
				const f32 rl = glm::length(rel);
				if(rl > 1.0f) dir = rel / rl;
				to = hp - vec3(dir * (info->dragOffset > 0 ? info->dragOffset : HitRadius(hookedPlayer) + HitRadius(target)), 0);
			}
			else to = from;
		}
		else if(info->type == StatusType::DRAGGED) {
			to = caster.body->GetWorldPos() + vec3(dir * (info->dragOffset > 0 ? info->dragOffset : 200.0f), 0);
		}
		else {
			f32 dist = info->type == StatusType::FORCEBACK && info->distance > 0 ? info->distance :
				STATUS_PUSH_BASE_DISTANCE * (distanceRatio != 0 ? distanceRatio : 1.0f) * ccScale;
			if(const PushToEdgeRule* edge = FindPushToEdgeRule(statusID)) {
				const Remote* wave = GetGameXmlContent().FindRemote(edge->remote);
				dist = wave ? MAX(wave->boundSize[0] * 0.5f - len, 0.0f) : dist;
			}
			to = from + vec3(dir * dist, 0);
			nominalDist = fabsf(dist);
			f32 wallDist = 0;
			const vec3 mid = vec3(0, 0, BodyMidHeight(target));
			if(nominalDist > 1.0f && physics.RaycastStatic(from + mid, to + mid, &wallDist, info->type == StatusType::FORCEBACK ? (i32)caster.team : (i32)target.team)) to = from + vec3(glm::normalize(vec2(to - from)) * wallDist, 0);
		}
		LOG("Push :: %ls -> %ls status=%d sep=%.0f dir=(%.2f, %.2f) to=(%.0f, %.0f)", caster.name.data(), target.name.data(), statusID, len, dir.x, dir.y, to.x, to.y);
		if(info->raiseStunByPush && graphDuration > 0) AddPushBound(caster, target.Main().UID, *info, graphDuration);
		if(dragForce && target.clientHd == ClientHandle::INVALID) {
			target.dragCenter = vec2(forceCenter);
			target.dragMin = info->dragForceMin;
			target.dragMax = info->dragForceMax;
			target.dragDuration = MAX(duration, 0.001f);
			target.dragElapsed = 0;
			const Time dragEnd = TimeAdd(localTime, TimeMsToTime(duration * 1000.0));
			if(dragEnd > target.ccUntil) target.ccUntil = dragEnd;
		}
		else if(target.clientHd == ClientHandle::INVALID && graphDuration > 0 && &caster == &target) {
			const vec2 rel = vec2(to - from);
			const f32 rl = glm::length(rel);
			if(rl > 1.0f) {
				target.dashDir = rel / rl;
				target.dashSpeed = rl / graphDuration;
				target.dashStartAt = localTime;
				target.dashBlockUntil = Time::ZERO;
				target.dashOrigin = target.body->GetWorldPos();
				target.dashDist = rl;
				target.dashDone = 0;
				target.dashCurve = nullptr;
				target.dashFly = false;
				target.dashUntil = TimeAdd(localTime, TimeMsToTime(graphDuration * 1000.0));
			}
		}
		else if(target.clientHd == ClientHandle::INVALID && graphDuration > 0) {
			if(localTime < target.dashUntil) {
				target.dashDone = target.dashDist;
				target.dashUntil = localTime;
			}
			const Time prevUntil = chainedPush ? target.pushUntil : localTime;
			const Time newUntil = TimeAdd(localTime, TimeMsToTime(graphDuration * 1000.0));
			target.pushUntil = newUntil > prevUntil ? newUntil : prevUntil;
			target.pushSilent = info && (info->type == StatusType::PUSH || info->type == StatusType::FLY || info->type == StatusType::AIRBORNE || info->type == StatusType::KNOCKDOWN);
			target.pushTarget = to;
			target.pushFrom = target.body->GetWorldPos();
			target.pushStart = localTime;
			target.pushPassWallTeam = info && info->type == StatusType::FORCEBACK ? (i32)caster.team : -1;
			target.pushSpeed = glm::length(vec2(to - target.body->GetWorldPos())) / MAX((f32)TimeDiffSec(TimeDiff(localTime, target.pushUntil)), 0.05f);
			if(target.pushUntil > target.ccUntil) target.ccUntil = target.pushUntil;
		}
		else if(target.clientHd != ClientHandle::INVALID && graphDuration > 0) {
			target.clientPushUntil = TimeAdd(localTime, TimeMsToTime(graphDuration * 1000.0));
			target.pushTarget = to;
			target.clientPushPosCount = target.clientPosCount;
			target.clientPushSnap = info->type == StatusType::PUSH || info->type == StatusType::FORCEBACK || info->type == StatusType::FLY || info->type == StatusType::AIRBORNE || info->type == StatusType::KNOCKDOWN || info->type == StatusType::DRAGGED || dragForce;
			target.clientPushFrom = target.body->GetWorldPos();
			target.clientPushStart = localTime;
			target.clientPushDragMin = dragForce ? info->dragForceMin : 0.0f;
			target.clientPushDragMax = dragForce ? info->dragForceMax : 0.0f;
			const Time followUntil = TimeAdd(localTime, TimeMsToTime((graphDuration + 2.0f) * 1000.0));
			if(followUntil > target.ccUntil) target.ccUntil = followUntil;
		}
	}
	*moveFrom = from;
	*moveTo = dragForce ? forceCenter : to;
	*moveDist = nominalDist;
}

void World::ApplyStatus(Player& caster, Player& target, i32 statusID, f32 power, f32 durationOverride)
{
	if(target.dead || target.untargetable) return;
	const ActorNpc* npcSource = &caster == &target && statusCasterOverride != ActorUID::INVALID ? FindNpcActor(statusCasterOverride) : nullptr;
	const bool hostileSource = &caster != &target ? caster.team != target.team : npcSource && NpcHittableBy(*npcSource, target);
	if(caster.team != target.team) target.statusesTaken++;
	const StatusInfo* info = GetGameXmlContent().FindStatus(statusID);
	f32 duration = info && info->durationSec > 0 ? info->durationSec : (info && (info->type == StatusType::MOTION || info->instant) ? 0.0f : 1.0f);
	if(info && info->durationSec < 0 && GetGameXmlContent().actionRemovedStatuses.count(statusID)) duration = STATUS_INFINITE_DURATION_SEC;
	f32 graphDuration = duration * (info && info->timeRatio > 0 ? info->timeRatio : 1.0f);
	const bool defeatTimed = DefeatGraphTimes(info, &duration, &graphDuration);
	if(durationOverride > 0) duration = durationOverride;
	if(caster.team != target.team) MarkInBattle(target);
	if(info && info->targetsFriendlyPc && info->targetingRange > 0 && caster.index == target.index && !spreadingStatus) {
		spreadingStatus = true;
		foreach(it, players) {
			if(it->index == caster.index || it->team != caster.team || it->dead) continue;
			if(glm::length(vec2(it->body->GetWorldPos()) - vec2(caster.body->GetWorldPos())) > info->targetingRange) continue;
			ApplyStatus(caster, *it, statusID, power, durationOverride);
		}
		if(info->targetsFriendlyNpc) {
			const Faction ownFaction = caster.team == 0 ? Faction::RED : Faction::BLUE;
			foreach(npc, actorNpcList) {
				if(npc->faction != ownFaction || npc->hp <= 0 || npc->removeAt != Time::ZERO) continue;
				if(glm::length(vec2(npc->pos) - vec2(caster.body->GetWorldPos())) > info->targetingRange + NpcHitRadius(*npc)) continue;
				ApplyStatusToNpc(caster, *npc, statusID, false);
			}
		}
		spreadingStatus = false;
	}
	if(hostileSource && HasStatusType(target, StatusType::BURROW)) {
		LOG("Status :: %ls ignored %d (burrow)", target.name.data(), statusID);
		return;
	}
	if(info && info->isDebuff && hostileSource && HasStatusType(target, StatusType::GODMODE)) {
		LOG("Status :: %ls resisted %d (godmode)", target.name.data(), statusID);
		replication->SendResistStatusToAll(target.Main().UID, caster.Main().UID, STATUS_RESIST_EVENT_PROTECTED);
		return;
	}
	if(info && info->type != StatusType::OTHER) {
		foreach_const(it, activeStatuses) {
			if(it->targetIndex != target.index || !it->info || it->until <= localTime) continue;
			if(eastl::find(it->info->defenseTypes.begin(), it->info->defenseTypes.end(), info->type) == it->info->defenseTypes.end()) continue;
			LOG("Status :: %ls refused %d (blocked by %d)", target.name.data(), statusID, it->statusID);
			replication->SendResistStatusToAll(target.Main().UID, caster.Main().UID, STATUS_RESIST_EVENT_PROTECTED);
			return;
		}
	}
	if(info && info->type == StatusType::FORCEBACK && info->skillIndex != 0) {
		foreach_const(it, activeStatuses) {
			if(it->targetIndex != target.index || it->casterIndex != caster.index || !it->info || it->info->type != StatusType::FLY || it->info->skillIndex != info->skillIndex || it->until <= localTime) continue;
			LOG("Status :: %ls ignores %d (already thrown by %d)", target.name.data(), statusID, it->statusID);
			return;
		}
	}
	f32 ccScale = 1.0f;
	if(info && hostileSource && (info->kind == StatusInfo::Kind::HARD_CC || info->kind == StatusInfo::Kind::ROOT)) {
		foreach(it, activeStatuses) {
			if(it->targetIndex != target.index || !it->info || it->info->type != StatusType::DEFIANCE || it->until <= localTime || it->appliedAt >= localTime) continue;
			if(it->custom > 1) {
				it->custom--;
				SendPassiveCounter(*it, it->custom);
			}
			else it->until = localTime;
			LOG("Status :: %ls resisted %d (defiance %d left)", target.name.data(), statusID, it->until > localTime ? it->custom : 0);
			replication->SendResistStatusToAll(target.Main().UID, caster.Main().UID, STATUS_RESIST_EVENT_PASSIVE);
			return;
		}
	}
	if(info && hostileSource && PassiveBlocksCC(target, *info, &ccScale)) return;
	if(info && caster.team != target.team && target.defeatGuardSource == statusID && localTime < target.defeatGuardUntil) {
		LOG("Status :: %ls resisted %d (defeat guard)", target.name.data(), statusID);
		replication->SendResistStatusToAll(target.Main().UID, caster.Main().UID, STATUS_RESIST_EVENT_PROTECTED);
		return;
	}
	if(info && info->type == StatusType::FLY) {
		const f32 ratio = statusDistanceRatioOverride != 0 ? statusDistanceRatioOverride : info->distanceRatio;
		if(ratio < 0) {
			const vec3 center = statusForceCenterSet ? statusForceCenter : caster.body->GetWorldPos();
			const f32 rc = glm::length(vec2(target.body->GetWorldPos()) - vec2(center));
			ccScale *= MIN(MAX(-ratio + rc / STATUS_PUSH_BASE_DISTANCE, 1.0f), FLY_PULL_MAX_SCALE);
		}
	}
	ccScale *= statusDurationScale;
	duration *= ccScale;
	graphDuration *= ccScale;
	ShowcaseOnStatus(caster, target, statusID);
	const bool permanent = info && info->permanent && durationOverride <= 0;
	if(permanent) duration = 1000000.0f;
	const f32 sentDuration = info && info->permanent ? StatusSentDuration(*info) : statusAniEnd && info ? (info->durationSec > 0 ? info->durationSec : info->durationSec < 0 ? STATUS_INFINITE_DURATION_SEC : 0.0f) : durationOverride > 0 || !info || info->durationSec > 0 || defeatTimed ? duration : (info->durationSec < 0 ? STATUS_INFINITE_DURATION_SEC : 0.0f);
	if(info && info->type == StatusType::TOGGLE && info->durationSec <= 0 && info->skillIndex != 0) {
		foreach(it, activeStatuses) {
			if(it->targetIndex != target.index || !it->info || it->info->type != StatusType::TARGET || it->info->skillIndex != info->skillIndex || it->until <= localTime) continue;
			const Time applied = it->appliedAt;
			const Time release = target.carryReleaseAt > localTime ? target.carryReleaseAt : localTime;
			foreach(w, activeStatuses) if(w->targetIndex == target.index && w->appliedAt == applied && w->until > release) w->until = release;
			LOG("Status :: %ls released from %d by %d", target.name.data(), it->statusID, statusID);
		}
		eastl::fixed_vector<i32,8,false> toggled;
		foreach_const(it, activeStatuses) {
			if(it->targetIndex != target.index || !it->info || it->statusID == statusID || it->info->permanent || it->info->skillIndex != info->skillIndex || it->until <= localTime || it->appliedAt >= localTime || toggled.full()) continue;
			if(info->toggleRemoves != 0 && it->statusID != info->toggleRemoves) continue;
			if(info->toggleRemoves == 0 && it->info->type == StatusType::TARGET) continue;
			if(eastl::find(toggled.begin(), toggled.end(), it->statusID) == toggled.end()) toggled.push_back(it->statusID);
		}
		foreach_const(t, toggled) {
			LOG("Status :: %ls toggle %d removes %d", target.name.data(), statusID, *t);
			ConsumeStatus(target.index, *t);
		}
	}
	if(info && info->endsGrab) {
		foreach(it, activeStatuses) {
			if(!it->info || it->info->skillIndex != info->skillIndex || it->until <= localTime) continue;
			const bool own = it->targetIndex == target.index && it->info->type == StatusType::CASTER;
			const bool held = it->casterIndex == target.index && it->targetIndex != target.index && it->info->type == StatusType::TARGET;
			if(!own && !held) continue;
			it->until = localTime;
			LOG("Status :: %ls grab remove %d ends %d on %ls", target.name.data(), statusID, it->statusID, players[it->targetIndex].name.data());
		}
	}
	const ActorUID npcCaster = statusCasterOverride != ActorUID::INVALID && FindNpcActor(statusCasterOverride) ? statusCasterOverride : ActorUID::INVALID;
	i32 npcCasterStatus = 0;
	if(info && info->casterStatus != 0 && info->casterStatus != statusID && npcCaster != ActorUID::INVALID) npcCasterStatus = info->casterStatus;
	else if(info && info->casterStatus != 0 && info->casterStatus != statusID && !caster.dead && !applyingCasterStatus) {
		bool refresh = false;
		foreach_const(it, activeStatuses) if(it->statusID == statusID && it->targetIndex == target.index) { refresh = true; break; }
		if(!refresh) {
			applyingCasterStatus = true;
			const ActorUID prevCaster = statusCasterOverride;
			statusCasterOverride = target.Main().UID;
			ApplyStatus(caster, caster, info->casterStatus);
			statusCasterOverride = prevCaster;
			applyingCasterStatus = false;
		}
	}
	const StatusDrainRule* drain = FindStatusDrainRule(statusID);
	if(drain && caster.team != target.team) {
		const f32 manaBefore = target.mana, energyBefore = target.res[0];
		target.mana = MAX(target.mana, 0.0f) * (1.0f - drain->fraction);
		if(target.res[0] > 0) target.res[0] *= 1.0f - drain->fraction;
		target.manaDirty = true;
		target.resDirty = true;
		LOG("Status :: %ls drained by %d (mana %.0f -> %.0f, energy %.0f -> %.0f)", target.name.data(), statusID, manaBefore, target.mana, energyBefore, target.res[0]);
	}
	else if(info && info->staminaBurn && info->type == StatusType::HEAL && target.stamina > 0) {
		LOG("Status :: %ls stamina burned by %d (%.0f -> 0)", target.name.data(), statusID, target.stamina);
		target.stamina = 0;
		target.staminaDirty = true;
	}
	if(const StatusStaminaRule* gain = FindStatusStaminaRule(statusID)) {
		const f32 staminaMax = MasterStat(target.Main().classType, STAT_STAMINA);
		if(caster.team == target.team && staminaMax > 0 && target.stamina < staminaMax) {
			const f32 before = target.stamina;
			target.stamina = MIN(target.stamina + gain->amount, staminaMax);
			target.staminaDirty = true;
			LOG("Status :: %ls stamina +%.0f by %d (%.0f -> %.0f)", target.name.data(), gain->amount, statusID, before, target.stamina);
		}
	}

	const bool dragForce = info && info->type == StatusType::DRAGGEDBYFORCE;
	const bool displacement = info && (info->type == StatusType::PUSH || info->type == StatusType::FORCEBACK ||
		info->type == StatusType::KNOCKDOWN || info->type == StatusType::AIRBORNE || info->type == StatusType::FLY || info->type == StatusType::DRAGGED || dragForce);
	const bool hostileHardCc = info && hostileSource && (info->kind == StatusInfo::Kind::HARD_CC || info->kind == StatusInfo::Kind::ROOT || info->type == StatusType::SILENCE) && info->type != StatusType::ACTIONBLOCK;
	if((displacement || hostileHardCc) && HasStatusType(target, StatusType::SUPERARMOR)) {
		LOG("Status :: %ls resisted %d (superarmor)", target.name.data(), statusID);
		replication->SendResistStatusToAll(target.Main().UID, npcSource ? npcSource->UID : caster.Main().UID, STATUS_RESIST_EVENT_PROTECTED);
		return;
	}
	if(info && hostileSource) PassiveOnCC(target, *info);

	if(info) foreach_const(m, info->mods) if(m->stat == StatusMod::RAGE && !m->regen) GainResource(target, 1, m->value);
	foreach(it, activeStatuses) {
		if(it->statusID == statusID && it->targetIndex == target.index) {
			if(it->appliedAt == localTime && it->casterIndex == caster.index && (!info || info->maxOverlap <= 1)) return;
			if(it->until <= localTime) {
				replication->SendRemoveStatusToAll(it->statusID, it->targetUID, it->casterUID);
				activeStatuses.erase_unsorted(it);
				break;
			}
			if(statusCleanupOnly) {
				it->aniEnd = true;
				LOG("Status :: %ls status=%d already active, cleaned up at action end", target.name.data(), statusID);
				return;
			}
			Time until = TimeAdd(localTime, TimeMsToTime(duration * 1000.0));
			const i32 ticks = info && info->dotDamage > 0 && info->intervalSec > 0 ? (i32)floorf(duration / info->intervalSec) : 0;
			if(ticks >= 1 && it->nextTick > localTime) until = TimeAdd(it->nextTick, TimeMsToTime((f64)(ticks - 1) * info->intervalSec * 1000.0));
			if(until > it->until) it->until = until;
			const bool stacked = info && it->overlap < info->maxOverlap;
			if(stacked) it->overlap++;
			it->skillPower = PlayerSkillPower(caster);
			if(!statusNoCaster) it->casterUID = statusCasterOverride != ActorUID::INVALID ? statusCasterOverride : caster.Main().UID;
			if(info && info->dotDamage > 0 && info->intervalSec <= 0) it->dotDone = false;
			if(info && info->type == StatusType::SHIELD && info->shieldCount <= 0) {
				it->shieldMax = StatusShieldLayer(*info, it->skillPower, power) * (f32)it->overlap;
				it->shieldHP = it->shieldMax;
				SendShieldStat(target);
			}
			LOG("Status :: %ls -> %ls status=%d refreshed overlap=%d", caster.name.data(), target.name.data(), statusID, it->overlap);
			const vec3 pos = target.body->GetWorldPos();
			replication->SendAddStatusToAll(statusID, it->targetUID, it->casterUID, sentDuration, pos, StatusGraphAnchorOnly(info) ? vec3(0) : pos, 0, it->overlap, 1.0f, -1.0f, StatusGraphAnchorOnly(info), true);
			if(info && it->overlap >= info->maxOverlap && info->maxOverlap > 1) {
				i32 full = 0;
				foreach_const(ps, activeStatuses) {
					if(ps->targetIndex != caster.index || !ps->info || ps->info->stackStatus != statusID || ps->info->stackFullStatus == 0) continue;
					full = ps->info->stackFullStatus;
					break;
				}
				if(full != 0) {
					LOG("Passive :: %ls full stack of %d on %ls -> %d", caster.name.data(), statusID, target.name.data(), full);
					replication->SendRemoveStatusToAll(it->statusID, it->targetUID, it->casterUID);
					activeStatuses.erase_unsorted(it);
					ApplyStatus(caster, target, full);
				}
			}
			return;
		}
	}
	if(activeStatuses.full()) return;

	ActiveStatus st;
	st.statusID = statusID;
	st.info = info;
	st.targetIndex = target.index;
	st.casterIndex = caster.index;
	st.targetUID = statusTargetOverride != ActorUID::INVALID ? statusTargetOverride : target.Main().UID;
	st.casterUID = statusNoCaster ? ActorUID::INVALID : (statusCasterOverride != ActorUID::INVALID ? statusCasterOverride : caster.Main().UID);
	st.damageAtStart = target.damageTaken;
	st.until = info && info->instant && durationOverride <= 0 ? localTime : TimeAdd(localTime, TimeMsToTime(MAX(duration, 0.1f) * 1000.0));
	st.appliedAt = localTime;
	st.aniEnd = statusAniEnd;
	st.nextTick =TimeAdd(localTime, TimeMsToTime((info && info->intervalSec > 0 ? info->intervalSec : 1.0f) * 1000.0));
	st.overlap = 1;
	st.nextHealTick = localTime;
	st.healTicksDone = 0;
	st.shieldHP = 0;
	st.skillPower = PlayerSkillPower(caster);
	if(info && (info->type == StatusType::SHIELD || info->type == StatusType::SHIELDBOMB)) st.shieldHP = StatusShieldLayer(*info, st.skillPower, power);
	st.shieldHits = info ? info->shieldCount : 0;
	if(st.shieldHits > 0) st.shieldHP = 0;
	st.shieldMax = st.shieldHP;
	activeStatuses.push_back(st);
	if(st.shieldMax > 0) SendShieldStat(target);

	if(info && info->type == StatusType::LINK && &caster != &target) {
		if(info->linkCasterStatus != 0) ApplyStatus(caster, caster, info->linkCasterStatus, power);
		if(info->linkTargetStatus != 0) ApplyStatus(caster, target, info->linkTargetStatus, power);
	}

	vec3 from, to;
	f32 nominalDist = -1.0f;
	ApplyStatusControl(caster, target, info, statusID, duration, graphDuration, ccScale, st.until, displacement, &from, &to, &nominalDist);

	if(info && info->type == StatusType::WEAPON_GAUGE_RECOVERY) RecoverWeaponHeat(target, info->paramRatio);
	i32 dispelled = 0;
	if(info && info->type == StatusType::DISPEL && info->dispelStatusType != StatusType::OTHER) {
		foreach(it, activeStatuses) {
			if(info->dispelMaxCount > 0 && dispelled >= info->dispelMaxCount) break;
			if(it->targetIndex != target.index || !it->info || !it->info->canDispel || it->info->type != info->dispelStatusType || it->until <= localTime) continue;
			it->until = localTime;
			dispelled++;
			LOG("Dispel :: %ls removed status %d (type %s) from %ls", caster.name.data(), it->statusID, it->info->typeName.c_str(), target.name.data());
		}
	}
	if(info && info->type == StatusType::DISPEL && info->dispelEffect >= 0) {
		foreach(it, activeStatuses) {
			if(it->targetIndex != target.index || !it->info || !it->info->canDispel || it->statusID == statusID) continue;
			if((it->info->isDebuff ? 1 : 0) != info->dispelEffect) continue;
			if(it->until <= localTime) continue;
			if(info->dispelMaxCount > 0 && dispelled >= info->dispelMaxCount) break;
			it->until = localTime;
			dispelled++;
			LOG("Dispel :: %ls removed status %d from %ls", caster.name.data(), it->statusID, target.name.data());
		}
	}

	LOG("Status :: %ls -> %ls status=%d type=%s duration=%.2f shield=%.0f", caster.name.data(), target.name.data(), statusID, info ? info->typeName.c_str() : "?", duration, st.shieldHP);
	if(caster.index == target.index) {
		if(const ResourceGainRule* rule = FindResourceGainRule(target.Main().classType, ResourceEvent::SELF_STATUS, RemoteIdx::INVALID, statusID)) GainResource(target, rule->slot, rule->amount);
	}
	vec3 sentTo = to;
	if(info && (info->type == StatusType::AIRBORNE || info->type == StatusType::FLY) && info->heightRatio > 0) sentTo.z = from.z + STATUS_LIFT_BASE_HEIGHT * info->heightRatio * ccScale;
	if(info && (info->attachToCaster || StatusGraphAnchorOnly(info))) sentTo = vec3(0);
	const bool motionGraph = info && info->type == StatusType::MOTION;
	if(motionGraph) sentTo = vec3(0);
	replication->SendAddStatusToAll(statusID, st.targetUID, st.casterUID, sentDuration, motionGraph ? vec3(0) : from, sentTo, StatusGraphTimed(info) ? graphDuration : 0.0f, 1, ccScale, nominalDist, StatusSendsGraph(info), false, info && info->permanent ? (u8)MAX(info->customInit, 0) : (u8)0);
	if(npcCasterStatus != 0) ApplyNpcOwnerStatus(npcCaster, target, npcCasterStatus);
	bool motionPlaying = false;
	if(info && info->motionAction != ActionStateID::INVALID) {
		foreach_const(pr, skillProgramList) if(pr->casterUID == target.Main().UID && pr->actionID == info->motionAction) motionPlaying = true;
		if(target.lastCastAction == info->motionAction && target.lastCastStart != Time::ZERO) {
			const f32 seq = GetGameXmlContent().GetSkillAction(ActionClass(target), info->motionAction, target.lastCastStance).seqLength;
			if(TimeDiffSec(TimeDiff(target.lastCastStart, localTime)) < seq) motionPlaying = true;
		}
	}
	if(motionPlaying) LOG("Status :: %ls motion status %d: action %d already playing", target.name.data(), statusID, (i32)info->motionAction);
	if(info && !motionPlaying && info->motionAction != ActionStateID::INVALID && info->skillIndex != 0 && !target.dead && (info->skillIndex / 10000) % 100 == (i32)target.Main().classType && GetGameXmlContent().SkillActionKnown(target.Main().classType, info->motionAction, target.Main().stance)) {
		LOG("Status :: %ls motion status %d plays action %d", target.name.data(), statusID, (i32)info->motionAction);
		PlayerCastSkill(target, SkillID(info->skillIndex), target.body->GetWorldPos(), Slice<const ActorUID>(), 0, info->motionAction);
	}
	if(info && info->blockSkill && hostileSource && !target.dead) InterruptSkills(target);
	const i32 guard = DefeatGuardStatus(info);
	if(guard != 0 && caster.team != target.team && !target.dead) {
		const StatusInfo* guardInfo = GetGameXmlContent().FindStatus(guard);
		target.defeatGuardSource = statusID;
		target.defeatGuardUntil = TimeAdd(localTime, TimeMsToTime((guardInfo ? guardInfo->durationSec : 0) * 1000.0));
		ApplyStatus(target, target, guard);
	}
	if(info && info->type == StatusType::DISPEL && info->dispelDamage > 0 && dispelled > 0 && caster.team != target.team && !target.dead) {
		LOG("Dispel :: %ls dispelled %d statuses from %ls, %.0f damage each", caster.name.data(), dispelled, target.name.data(), info->dispelDamage);
		DamagePlayer(caster, target, SkillID(info->skillIndex), (i32)(info->dispelDamage * dispelled));
	}
}

bool World::IsBlocked(const Player& p, bool attack, bool skill, bool shirk) const
{
	foreach_const(it, activeStatuses) {
		if(it->targetIndex != p.index || !it->info || localTime >= it->until) continue;
		if((attack && it->info->blockAttack) || (skill && it->info->blockSkill) || (shirk && it->info->blockShirk)) return true;
	}
	return false;
}

static const f32 MEASURED_SLIME_MASS_BAND_START[4] = { 25.0f, 45.0f, 65.0f, 85.0f };
static const f32 MEASURED_SLIME_POWER_PCT[5] = { -50.0f, -25.0f, 0.0f, 25.0f, 50.0f };
static const f32 MEASURED_SLIME_SPEED_PCT[5] = { 25.0f, 10.0f, 0.0f, -10.0f, -25.0f };
static const f32 MEASURED_SLIME_SKILLPOWER[5] = { -75.0f, -37.5f, 0.0f, 37.5f, 75.0f };

f32 World::SlimeMassMod(const Player& p, i32 stat, bool mul) const
{
	if(stat != StatusMod::ATTACK && stat != StatusMod::DEFENCE && stat != StatusMod::MOVESPEED && stat != StatusMod::SKILLPOWER) return 0;
	if(!FindPassiveOfType(p, StatusType::SLIME)) return 0;
	i32 band = 0;
	for(i32 i = 0; i < 4; i++) if(p.res[1] >= MEASURED_SLIME_MASS_BAND_START[i]) band = i + 1;
	if(!mul) return stat == StatusMod::SKILLPOWER ? MEASURED_SLIME_SKILLPOWER[band] : 0;
	if(stat == StatusMod::MOVESPEED) return MEASURED_SLIME_SPEED_PCT[band];
	return stat == StatusMod::SKILLPOWER ? 0 : MEASURED_SLIME_POWER_PCT[band];
}

f32 World::StatusModMulSum(const Player& p, i32 stat) const
{
	f32 sum = SlimeMassMod(p, stat, true) + GroupingStatMod(p, stat, true);
	foreach_const(it, activeStatuses) {
		if(it->targetIndex != p.index || !it->info || localTime >= it->until) continue;
		foreach_const(m, it->info->mods) {
			if((i32)m->stat == stat && !m->regen && m->mul) sum += m->value * it->overlap;
		}
	}
	return sum;
}

f32 World::StatusRegenBonus(const Player& p, i32 stat, bool mul) const
{
	f32 sum = 0;
	foreach_const(it, activeStatuses) {
		if(it->targetIndex != p.index || !it->info || localTime >= it->until) continue;
		foreach_const(m, it->info->mods) {
			if((i32)m->stat == stat && m->mul == mul) sum += m->value * it->overlap;
		}
	}
	return sum;
}

f32 World::StatusModSum(const Player& p, i32 stat) const
{
	f32 sum = SlimeMassMod(p, stat, false) + GroupingStatMod(p, stat, false);
	foreach_const(it, activeStatuses) {
		if(it->targetIndex != p.index || !it->info) continue;
		foreach_const(m, it->info->mods) {
			if((i32)m->stat == stat && !m->regen && !m->mul) sum += m->value * it->overlap;
		}
		for(const MeasuredStatusStat& ms : MEASURED_STATUS_STATS) {
			if(ms.status == it->statusID && (i32)ms.stat == stat) sum += ms.value * it->overlap;
		}
	}
	return sum;
}

void World::UpdateStatuses()
{
	for(auto ps = pendingStatuses.begin(); ps != pendingStatuses.end(); ) {
		if(localTime < ps->time) { ++ps; continue; }
		Player& caster = players[ps->casterIndex];
		if(ps->childCreature != 0) {
			foreach(npc, actorNpcList) {
				if(npc->ownerIndex != ps->casterIndex || (i32)npc->docID != ps->childCreature || localTime >= npc->despawnAt) continue;
				ApplyStatusToNpc(caster, *npc, ps->statusID, ps->remove);
			}
			ps = pendingStatuses.erase_unsorted(ps);
			continue;
		}
		if(ps->lastChild) ps->targetUID = caster.lastChildUID;
		const ActorMaster* targetMaster = ps->targetUID != ActorUID::INVALID ? FindMasterActor(ps->targetUID) : nullptr;
		if(targetMaster && ps->remove) {
			foreach(it, activeStatuses) {
				if(it->targetIndex != targetMaster->parent->index || it->statusID != ps->statusID || localTime >= it->until) continue;
				it->until = localTime;
				LOG("Status :: %ls status=%d removed by action", targetMaster->parent->name.data(), ps->statusID);
			}
		}
		else if(!targetMaster && ps->targetUID != ActorUID::INVALID) {
			if(ActorNpc* npc = FindNpcActor(ps->targetUID)) {
				statusCasterOverride = ps->sourceUID;
				ApplyStatusToNpc(caster, *npc, ps->statusID, ps->remove);
				statusCasterOverride = ActorUID::INVALID;
			}
		}
		else if(!caster.dead && targetMaster) {
			statusCasterOverride = ps->sourceUID;
			statusAniEnd = ps->aniEnd;
			statusCleanupOnly = ps->cleanupOnly;
			statusNoCaster = ps->noCaster;
			ApplyStatus(caster, *targetMaster->parent, ps->statusID, ps->power, ps->duration);
			statusNoCaster = false;
			statusAniEnd = false;
			statusCleanupOnly = false;
			statusCasterOverride = ActorUID::INVALID;
		}
		ps = pendingStatuses.erase_unsorted(ps);
	}

	for(auto it = activeStatuses.begin(); it != activeStatuses.end(); ) {
		Player& target = players[it->targetIndex];
		if(it->info && it->info->dotDamage > 0 && it->info->intervalSec <= 0 && !it->dotDone && !target.dead) {
			it->dotDone = true;
			const i32 dot = StatusDotTickDamage(players[it->casterIndex], target, *it);
			damageStatusID = it->statusID;
			damagePreMitigated = true;
			DamagePlayer(players[it->casterIndex], target, SkillID::INVALID, dot);
			damageStatusID = 0;
		}
		if(it->info && it->info->dotDamage > 0 && it->info->intervalSec > 0) {
			while(localTime >= it->nextTick && it->nextTick <= it->until && !target.dead) {
				it->nextTick = TimeAdd(it->nextTick, TimeMsToTime(it->info->intervalSec * 1000.0));
				const i32 hpBefore = target.Main().hp;
				const i32 dot = StatusDotTickDamage(players[it->casterIndex], target, *it);
				damageStatusID = it->statusID;
				damagePreMitigated = true;
				DamagePlayer(players[it->casterIndex], target, SkillID::INVALID, dot);
				damageStatusID = 0;
				Player& drainer = players[it->casterIndex];
				const i32 drained = hpBefore - target.Main().hp;
				if(it->info->drainMultiplier > 0 && drained > 0 && !drainer.dead) {
					ActorMaster& dm = drainer.Main();
					dm.hp = MIN(dm.hp + (i32)(drained * it->info->drainMultiplier), dm.hpMax);
					replication->SendHpToAll(dm.UID, dm.hp, dm.hpMax);
					LOG("Status :: %ls drains %d from %ls", drainer.name.data(), (i32)(drained * it->info->drainMultiplier), target.name.data());
				}
			}
		}

		if(it->info && !target.dead) {
			foreach_const(m, it->info->mods) {
				if(!m->regen || m->stat != StatusMod::RAGE || localTime < it->nextHealTick || it->healTicksDone >= (i32)m->count) continue;
				it->nextHealTick = TimeAdd(localTime, TimeMsToTime(MAX(m->intervalSec > 0 ? m->intervalSec : it->info->intervalSec, 0.1f) * 1000.0));
				it->healTicksDone++;
				GainResource(target, 1, m->value * it->overlap);
			}
			foreach_const(m, it->info->mods) {
				if(!m->regen || m->stat != StatusMod::HEALTH) continue;
				const f32 ticks = MAX(m->count, 1.0f);
				const f32 interval = it->info->intervalSec > 0 ? it->info->intervalSec : MAX(it->info->durationSec, 0.1f) / ticks;
				if(it->healTicksDone >= (i32)ticks || localTime < it->nextHealTick) continue;
				it->nextHealTick = TimeAdd(localTime, TimeMsToTime(interval * 1000.0));
				it->healTicksDone++;
				const i32 amount = (i32)(m->mul ? target.Main().hpMax * m->value / 100.0f : m->value + it->skillPower * it->info->healMul);
				HealPlayer(players[it->casterIndex], target, amount * it->overlap);
				ActorMaster& bench = target.Sub();
				if(it->info->bothCharacters && bench.hp > 0 && bench.hp < bench.hpMax) {
					const i32 benchAmount = (i32)(m->mul ? bench.hpMax * m->value / 100.0f : m->value + it->skillPower * it->info->healMul) * it->overlap;
					bench.hp = MIN(bench.hp + benchAmount, bench.hpMax);
					replication->SendStatToAll(bench.UID, (i32)StatType::STAT_TYPE_HEALTH, (f32)bench.hp, (f32)bench.hpMax, STAT_REASON_REGEN);
				}
			}
		}

		if(it->info && it->info->auraStatus != 0 && !target.dead && localTime >= it->nextAuraPulse) {
			it->nextAuraPulse = TimeAdd(localTime, TimeMsToTime(500));
			const StatusInfo* child = GetGameXmlContent().FindStatus(it->info->auraStatus);
			const f32 range = it->info->targetingRange > 0 ? it->info->targetingRange : (child ? child->targetingRange : 0);
			if(child && range > 0) PulseAura(it->casterIndex, *it->info, vec2(target.body->GetWorldPos()), range, it->info->type == StatusType::DEFEAT_CASTER_AURA ? (i32)target.index : -1);
		}

		if(it->info && it->info->type == StatusType::LINK && localTime < it->until && it->casterIndex != it->targetIndex) {
			const Player& linker = players[it->casterIndex];
			const f32 linkMax = SkillMaxDistance(linker, it->info->skillIndex);
			if(linkMax > 0 && (linker.dead || glm::length(vec2(linker.body->GetWorldPos()) - vec2(target.body->GetWorldPos())) > linkMax)) {
				LOG("Status :: link %d between %ls and %ls snapped", it->statusID, linker.name.data(), target.name.data());
				const i32 skillIndex = it->info->skillIndex;
				const u32 casterIdx = it->casterIndex;
				foreach(other, activeStatuses) {
					if(other->targetIndex == target.index && other->casterIndex == casterIdx && other->info && other->info->skillIndex == skillIndex) other->until = localTime;
					if(other->targetIndex == casterIdx && other->casterIndex == casterIdx && other->statusID == it->info->linkCasterStatus) other->until = localTime;
				}
			}
		}
		const bool shieldBomb = it->info && it->info->type == StatusType::SHIELDBOMB;
		const bool countShield = it->info && it->info->shieldCount > 0;
		const bool shieldBroken = it->info && (countShield ? it->shieldHits <= 0 : (shieldBomb && it->shieldHP <= 0 && it->absorbed > 0));
		const bool timeOver = localTime >= it->until;
		if(shieldBroken && it->info->skillIndex != 0) {
			foreach(sib, activeStatuses) {
				if(sib != it && sib->targetIndex == it->targetIndex && sib->info && sib->info->skillIndex == it->info->skillIndex && sib->appliedAt == it->appliedAt && sib->until > localTime) sib->until = localTime;
			}
		}
		if(timeOver || (target.dead && !(it->info && it->info->permanent && it->info->type != StatusType::DEFIANCE)) || shieldBroken || it->removeOnHit) {
			if(timeOver && !target.dead && it->info && it->info->type == StatusType::NEARLY_IMMORTAL && it->info->paramRatio > 0) {
				const i32 taken = MAX(target.damageTaken - it->damageAtStart, 0);
				const i32 extra = (i32)MIN(taken * it->info->Param("_RecoveryFactorHP") / 100.0f, it->info->Param("_MaxRecoverHP"));
				const i32 heal = (i32)(target.Main().hpMax * it->info->paramRatio) + extra;
				LOG("Status :: %ls undying %d ends: heal %d (+%d from %d taken)", target.name.data(), it->statusID, heal, extra, taken);
				HealPlayer(players[it->casterIndex], target, heal);
			}
			if(shieldBomb && !target.dead && it->info->targetingRange > 0) {
				const i32 boom = (i32)(it->info->explodeDamage + it->absorbed * it->info->paramRatio);
				const SkillID bombSkill = SkillID(it->info->skillIndex);
				Player& owner = players[it->casterIndex];
				LOG("Status :: %ls shield bomb %d explodes (absorbed %.0f) for %d within %.0f", target.name.data(), it->statusID, it->absorbed, boom, it->info->targetingRange);
				foreach(e, players) {
					if(e->dead || e->team == target.team) continue;
					if(glm::length(vec2(e->body->GetWorldPos()) - vec2(target.body->GetWorldPos())) > it->info->targetingRange + HitRadius(*e)) continue;
					DamagePlayer(owner, *e, bombSkill, boom);
				}
			}
			replication->SendRemoveStatusToAll(it->statusID, it->targetUID, it->casterUID);
			if(it->info && it->info->casterStatus != 0 && it->info->casterStatus != it->statusID) {
				foreach(o, activeStatuses) {
					if(o == it || o->statusID != it->info->casterStatus || o->until <= localTime) continue;
					const Player& holder = players[o->targetIndex];
					const bool own = o->targetIndex == it->casterIndex && (o->casterUID == target.Main().UID || o->casterUID == target.Sub().UID || o->casterIndex == o->targetIndex);
					const bool held = o->casterIndex == it->targetIndex && (it->casterUID == holder.Main().UID || it->casterUID == holder.Sub().UID || it->casterIndex == it->targetIndex);
					if(!own && !held) continue;
					o->until = localTime;
					LOG("Status :: %ls status %d ends with %d", holder.name.data(), o->statusID, it->statusID);
				}
			}
			i32 next = 0;
			if(it->info && it->info->nextStatus != 0 && !target.dead) {
				const auto cond = it->info->nextCondition;
				if(cond == StatusInfo::NextCondition::ALL ||
				   (cond == StatusInfo::NextCondition::TIMEOVER && timeOver) ||
				   (cond == StatusInfo::NextCondition::REMOVE_ONHIT && it->removeOnHit)) next = it->info->nextStatus;
			}
			const u32 casterIndex = it->casterIndex;
			if(it->info && it->info->auraStatus != 0) EndAura(casterIndex, *it->info);
			if(it->ccEnd != Time::ZERO && localTime < it->ccStatusEnd && target.ccUntil == it->ccEnd) {
				Time cc = TimeAdd(localTime, TimeDiff(it->ccStatusEnd, it->ccEnd));
				foreach_const(o, activeStatuses) {
					if(o == it || o->targetIndex != it->targetIndex || o->ccEnd == Time::ZERO) continue;
					const Time end = o->until < o->ccStatusEnd ? TimeAdd(o->until, TimeDiff(o->ccStatusEnd, o->ccEnd)) : o->ccEnd;
					if(end > cc) cc = end;
				}
				target.ccUntil = cc;
				LOG("Status :: %ls control %d ended early, control until +%.2f s", target.name.data(), it->statusID, TimeDiffSec(TimeDiff(localTime, cc)));
			}
			const bool hadShield = it->shieldMax > 0;
			it = activeStatuses.erase_unsorted(it);
			if(hadShield) SendShieldStat(target);
			if(next != 0 && !pendingStatuses.full()) {
				PendingStatus ps;
				ps.time = localTime;
				ps.statusID = next;
				ps.casterIndex = casterIndex;
				ps.targetUID = target.Main().UID;
				ps.duration = -1.0f;
				ps.power = 0;
				ps.noCaster = true;
				const StatusInfo* nextInfo = GetGameXmlContent().FindStatus(next);
				ps.lastChild = nextInfo && nextInfo->targetsLastChild;
				pendingStatuses.push_back(ps);
			}
		}
		else {
			++it;
		}
	}
}

void World::AddPushBound(Player& caster, ActorUID pushed, const StatusInfo& info, f32 duration)
{
	const Remote* remote = GetGameXmlContent().FindCasterMoveBoundRemote(info.skillIndex);
	if(!remote || pushBounds.full()) return;
	foreach(b, pushBounds) {
		if(b->pushed != pushed) continue;
		b->until = TimeAdd(localTime, TimeMsToTime(duration * 1000.0));
		b->remote = remote;
		b->casterIndex = caster.index;
		return;
	}
	PushBound b;
	b.pushed = pushed;
	b.casterIndex = caster.index;
	b.until = TimeAdd(localTime, TimeMsToTime(duration * 1000.0));
	b.remote = remote;
	pushBounds.push_back(b);
}

void World::UpdatePushBounds()
{
	for(i32 i = 0; i < (i32)pushBounds.size(); ) {
		if(localTime >= pushBounds[i].until) { pushBounds.erase(pushBounds.begin() + i); continue; }
		PushBound& b = pushBounds[i];
		i++;
		vec3 at;
		Faction faction;
		const ActorMaster* pm = FindMasterActor(b.pushed);
		const ActorNpc* pn = pm ? nullptr : FindNpcActor(b.pushed);
		if(pm && !pm->parent->dead && pm->parent->body) { at = pm->parent->body->GetWorldPos(); faction = (Faction)pm->parent->team; }
		else if(pn && pn->hp > 0) { at = pn->pos; faction = pn->faction; }
		else continue;
		Player& caster = players[b.casterIndex];
		const f32 bound = (f32)b.remote->boundSize[0];
		const ActorUID prevCaster = statusCasterOverride;
		foreach(p, players) {
			if(p->removed || p->dead || !p->body || (Faction)p->team != faction || p->Main().UID == b.pushed || b.hit.full()) continue;
			if(eastl::find(b.hit.begin(), b.hit.end(), p->Main().UID) != b.hit.end()) continue;
			if(glm::length(vec2(p->body->GetWorldPos() - at)) > bound + HitRadius(*p)) continue;
			b.hit.push_back(p->Main().UID);
			LOG("PushBound :: %ls hit by pushed actor %u", p->name.data(), (u32)b.pushed);
			statusCasterOverride = b.pushed;
			foreach_const(s, b.remote->friendlyStatuses) ApplyStatus(caster, *p, s->statusID);
			statusCasterOverride = prevCaster;
		}
		foreach(n, actorNpcList) {
			if(n->hp <= 0 || n->removeAt != Time::ZERO || n->faction != faction || n->UID == b.pushed || b.hit.full()) continue;
			if(eastl::find(b.hit.begin(), b.hit.end(), n->UID) != b.hit.end()) continue;
			if(glm::length(vec2(n->pos - at)) > bound + NpcHitRadius(*n)) continue;
			b.hit.push_back(n->UID);
			LOG("PushBound :: creature=%d hit by pushed actor %u", (i32)n->docID, (u32)b.pushed);
			statusCasterOverride = b.pushed;
			foreach_const(s, b.remote->friendlyStatuses) ApplyStatusToNpc(caster, *n, s->statusID, false);
			statusCasterOverride = prevCaster;
		}
	}
}
