#include "config.h"
#include "world.h"
#include "world_common.h"
#include <mxm/game_content.h>

static const i32 STAT_TOUGHNESS = 65;
static const f32 TOOLTIP_DEFENDER_CC_IMMUNE_SEC = 1.0f;
static const f32 TOOLTIP_DEATHKNIGHT_REVIVE_HP = 0.30f;
static const f64 SELF_DRAG_LINK_SEC = 0.6;
static const f32 STATUS_PUSH_BASE_DISTANCE = 500.0f;

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
			ApplyStatus(player, player, id);
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

const World::ActiveStatus* World::FindPassiveOfType(const Player& p, StatusType type) const
{
	foreach_const(it, activeStatuses) {
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

bool World::PassiveBlocksCC(Player& target, const StatusInfo& info, f32* duration)
{
	if(info.kind != StatusInfo::Kind::HARD_CC && info.kind != StatusInfo::Kind::ROOT) return false;
	if(localTime < target.ccImmuneUntil) {
		LOG("Passive :: %ls immune to status %d", target.name.data(), info.ID);
		return true;
	}
	if(const ActiveStatus* st = FindPassiveOfType(target, StatusType::UNSHAKABLE)) {
		if(target.cleanseCharges < 0) target.cleanseCharges = st->info->customInit;
		if(target.cleanseCharges > 0) {
			target.cleanseCharges--;
			if(target.cleanseRefillAt == Time::ZERO) target.cleanseRefillAt = TimeAdd(localTime, TimeMsToTime(st->info->Param("_RefillDuration")));
			LOG("Passive :: %ls blocked status %d (charges left %d)", target.name.data(), info.ID, target.cleanseCharges);
			return true;
		}
		target.cleanseCharges = MIN(target.cleanseCharges + 1, st->info->customMax);
	}
	const f32 toughness = MasterStat(target.Main().classType, STAT_TOUGHNESS);
	if(toughness > 0 && duration) *duration *= MAX(1.0f - toughness / 100.0f, 0.0f);
	return false;
}

bool World::PassiveOnLethal(Player& target)
{
	const ActiveStatus* st = FindPassiveOfType(target, StatusType::ACTIVATE_UNDEAD);
	if(!st || localTime < target.reviveReadyAt) return false;
	const i32 undead = (i32)st->info->ParamInt("_UndeadStatusIndex");
	const StatusInfo* undeadInfo = GetGameXmlContent().FindStatus(undead);
	target.Main().hp = 1;
	target.reviveReadyAt = TimeAdd(localTime, TimeMsToTime(st->info->Param("_DelayMS")));
	target.reviveAt = TimeAdd(localTime, TimeMsToTime((undeadInfo ? undeadInfo->durationSec : 0.0f) * 1000.0));
	target.reviveRatio = TOOLTIP_DEATHKNIGHT_REVIVE_HP;
	ApplyStatus(target, target, undead);
	LOG("Passive :: %ls cheated death (undead status %d)", target.name.data(), undead);
	return true;
}

void World::PassiveOnDamaged(Player& target)
{
	if(target.dead) return;
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

void World::PulseAura(u32 casterIndex, const StatusInfo& aura, const vec2& center, f32 range, i32 skipDebuffIndex)
{
	const u8 casterTeam = players[casterIndex].team;
	foreach_const(id, aura.auraStatuses) {
		const StatusInfo* child = GetGameXmlContent().FindStatus(*id);
		if(!child) continue;
		foreach(p, players) {
			if(p->dead) continue;
			const bool casterAlly = p->team == casterTeam;
			if(child->isDebuff ? casterAlly : (!casterAlly && !aura.auraAll)) continue;
			if(child->isDebuff && (i32)p->index == skipDebuffIndex) continue;
			if(glm::length(vec2(p->body->GetWorldPos()) - center) > range + HitRadius(*p)) continue;
			if(pendingStatuses.full()) return;
			PendingStatus ps;
			ps.time = localTime;
			ps.statusID = *id;
			ps.casterIndex = casterIndex;
			ps.targetUID = p->Main().UID;
			ps.duration = child->durationSec > 0 ? child->durationSec : 1.0f;
			ps.power = 0;
			pendingStatuses.push_back(ps);
		}
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
		if(it->cleanseRefillAt != Time::ZERO && localTime >= it->cleanseRefillAt) {
			const ActiveStatus* st = FindPassiveOfType(*it, StatusType::UNSHAKABLE);
			it->cleanseRefillAt = Time::ZERO;
			if(st && it->cleanseCharges < st->info->customMax) {
				it->cleanseCharges++;
				if(it->cleanseCharges < st->info->customMax) it->cleanseRefillAt = TimeAdd(localTime, TimeMsToTime(st->info->Param("_RefillDuration")));
			}
		}
	}
}

void World::PassiveOnSkillCast(Player& player, i32 skillID)
{
	if(GetGameXmlContent().GetSkillConsumeStamina(skillID) > 0) return;
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
		}
	}
	foreach_const(id, toApply) {
		LOG("Passive :: %ls counter full -> status %d", player.name.data(), *id);
		ApplyStatus(player, player, *id);
	}

	if(const ActiveStatus* st = FindPassiveOfType(player, StatusType::SKILLSTAMINA)) {
		ActiveStatus& counter = const_cast<ActiveStatus&>(*st);
		counter.custom++;
		if(st->info->customMax > 0 && counter.custom >= st->info->customMax) {
			counter.custom = 0;
			const f32 staminaMax = MasterStat(player.Main().classType, STAT_STAMINA);
			if(staminaMax > 0) {
				player.stamina = staminaMax;
				player.tNextStaminaSend = Time::ZERO;
			}
			LOG("Passive :: %ls stamina restored after %d skills", player.name.data(), st->info->customMax);
		}
	}
	if(const ActiveStatus* st = FindPassiveOfType(player, StatusType::DEFENDER)) {
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

void World::PassiveOnWeaponHit(Player& attacker, Player& target, const Projectile& proj)
{
	if(proj.remoteDoc != RemoteIdx::INVALID && !target.dead) {
		const Remote* remote = GetGameXmlContent().FindRemote(proj.remoteDoc);
		if(remote) {
			foreach_const(st, remote->statuses) {
				if(st->rate >= 100 || (i32)(RandUint() % 100) < st->rate) ApplyStatus(attacker, target, st->statusID);
			}
		}
	}

	if(const ActiveStatus* sniper = FindPassiveOfType(attacker, StatusType::SNIPER)) {
		const i32 mark = GetGameXmlContent().FindSkillStatusOfType(sniper->info->skillIndex, StatusType::STACKBOMB);
		if(mark != 0 && !target.dead && target.team != attacker.team) {
			foreach(st, activeStatuses) {
				if(st->statusID == mark && st->casterIndex == attacker.index && st->targetIndex != target.index) st->until = localTime;
			}
			ApplyStatus(attacker, target, mark);
		}
	}

	eastl::fixed_vector<i32,4,false> toApply;
	foreach(it, activeStatuses) {
		if(it->targetIndex != attacker.index || !it->info) continue;
		if(!PassiveOfActiveMaster(attacker, it->statusID)) continue;
		const StatusInfo& info = *it->info;
		if(info.type == StatusType::HIT_PLAGUE && info.customMax > 0) {
			it->custom++;
			if(it->custom >= info.customMax) {
				it->custom = info.customInit;
				if(info.hitEffectStatus && !toApply.full()) toApply.push_back(info.hitEffectStatus);
			}
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
				if(target.mana >= 0) { stolen = MIN(stolen, target.mana); target.mana -= stolen; target.tNextManaSend = Time::ZERO; }
				if(attacker.mana >= 0) { attacker.mana += stolen; attacker.tNextManaSend = Time::ZERO; }
				LOG("Passive :: %ls drained %.0f mana from %ls", attacker.name.data(), stolen, target.name.data());
			}
		}
	}
	foreach_const(id, toApply) {
		LOG("Passive :: %ls weapon hit -> status %d on %ls", attacker.name.data(), *id, target.name.data());
		ApplyStatus(attacker, target, *id);
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
			case WeaponBonus::OWNER_BELOW_HP_TRIGGER: {
				const ActiveStatus* st = FindPassiveOfType(attacker, StatusType::HPTRIGGER);
				if(st && attacker.Main().hp * 100 < attacker.Main().hpMax * st->info->Param("_CheckVal")) mul *= rule.mul;
			} break;
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

void World::ApplyStatus(Player& caster, Player& target, i32 statusID, f32 power, f32 durationOverride)
{
	if(target.dead || target.untargetable) return;
	if(caster.team != target.team) target.statusesTaken++;
	const StatusInfo* info = GetGameXmlContent().FindStatus(statusID);
	f32 duration = durationOverride > 0 ? durationOverride : (info && info->durationSec > 0 ? info->durationSec : 1.0f);
	if(info && caster.team != target.team && PassiveBlocksCC(target, *info, &duration)) return;
	const bool permanent = info && info->permanent && durationOverride <= 0;
	if(permanent) duration = 1000000.0f;
	const f32 sentDuration = permanent ? -1.0f : duration;
	if(info && info->casterStatus != 0 && info->casterStatus != statusID && !caster.dead && !applyingCasterStatus) {
		applyingCasterStatus = true;
		ApplyStatus(caster, caster, info->casterStatus);
		applyingCasterStatus = false;
	}
	const StatusDrainRule* drain = FindStatusDrainRule(statusID);
	if(drain && caster.team != target.team) {
		const f32 manaBefore = target.mana, energyBefore = target.res[0];
		target.mana = MAX(target.mana, 0.0f) * (1.0f - drain->fraction);
		if(target.res[0] > 0) target.res[0] *= 1.0f - drain->fraction;
		target.tNextManaSend = Time::ZERO;
		target.tNextResSend = Time::ZERO;
		LOG("Status :: %ls drained by %d (mana %.0f -> %.0f, energy %.0f -> %.0f)", target.name.data(), statusID, manaBefore, target.mana, energyBefore, target.res[0]);
	}
	else if(info && info->staminaBurn && info->type == StatusType::HEAL && target.stamina > 0) {
		LOG("Status :: %ls stamina burned by %d (%.0f -> 0)", target.name.data(), statusID, target.stamina);
		target.stamina = 0;
		target.tNextStaminaSend = Time::ZERO;
	}

	const bool dragForce = info && info->type == StatusType::DRAGGEDBYFORCE;
	const bool displacement = info && (info->type == StatusType::PUSH || info->type == StatusType::FORCEBACK ||
		info->type == StatusType::KNOCKDOWN || info->type == StatusType::AIRBORNE || info->type == StatusType::FLY || info->type == StatusType::DRAGGED || dragForce);
	if(displacement && HasStatusType(target, StatusType::SUPERARMOR)) {
		LOG("Status :: %ls resisted %d (superarmor)", target.name.data(), statusID);
		return;
	}

	foreach(it, activeStatuses) {
		if(it->statusID == statusID && it->targetIndex == target.index) {
			const Time until = TimeAdd(localTime, TimeMsToTime(duration * 1000.0));
			if(until > it->until) it->until = until;
			const bool stacked = info && it->overlap < info->maxOverlap;
			if(stacked) it->overlap++;
			LOG("Status :: %ls -> %ls status=%d refreshed overlap=%d", caster.name.data(), target.name.data(), statusID, it->overlap);
			const vec3 pos = target.body->GetWorldPos();
			if(!stacked) replication->SendRemoveStatusToAll(statusID, it->targetUID, it->casterUID);
			replication->SendAddStatusToAll(statusID, it->targetUID, it->casterUID, sentDuration, pos, pos, 0, it->overlap);
			if(info && it->overlap >= info->maxOverlap && info->maxOverlap > 1) {
				foreach_const(ps, activeStatuses) {
					if(ps->targetIndex != caster.index || !ps->info || ps->info->stackStatus != statusID || ps->info->stackFullStatus == 0) continue;
					const i32 full = ps->info->stackFullStatus;
					it->until = localTime;
					LOG("Passive :: %ls full stack of %d on %ls -> %d", caster.name.data(), statusID, target.name.data(), full);
					ApplyStatus(caster, target, full);
					break;
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
	st.targetUID = target.Main().UID;
	st.casterUID = statusCasterOverride != ActorUID::INVALID ? statusCasterOverride : caster.Main().UID;
	st.damageAtStart = target.damageTaken;
	st.until = TimeAdd(localTime, TimeMsToTime(MAX(duration, 0.1f) * 1000.0));
	st.appliedAt = localTime;
	st.nextTick = TimeAdd(localTime, TimeMsToTime((info && info->intervalSec > 0 ? info->intervalSec : 1.0f) * 1000.0));
	st.overlap = 1;
	st.nextHealTick = localTime;
	st.healTicksDone = 0;
	st.shieldHP = 0;
	if(info && (info->type == StatusType::SHIELD || info->type == StatusType::SHIELDBOMB)) {
		f32 dataShield = 0;
		foreach_const(m, info->mods) {
			if(m->stat == StatusMod::SHIELD_DEFENCE && !m->mul) dataShield += m->value;
		}
		st.shieldHP = dataShield > 0 ? dataShield : MAX(power, 100.0f) * (info->paramRatio > 0 ? info->paramRatio : 1.0f);
	}
	activeStatuses.push_back(st);

	if(info && info->type == StatusType::LINK && &caster != &target) {
		if(info->linkCasterStatus != 0) ApplyStatus(caster, caster, info->linkCasterStatus, power);
		if(info->linkTargetStatus != 0) ApplyStatus(caster, target, info->linkTargetStatus, power);
	}

	if(info && (info->kind == StatusInfo::Kind::HARD_CC || info->kind == StatusInfo::Kind::ROOT)) {
		Time ccEnd = st.until;
		if(info->type == StatusType::AIRBORNE || info->type == StatusType::FLY || info->type == StatusType::KNOCKDOWN) {
			const auto& content = GetGameXmlContent();
			const f32 recover = info->lyingTime + content.GetLieEndTime(content.GetMaster(target.Main().classType).className.c_str());
			ccEnd = TimeAdd(ccEnd, TimeMsToTime(recover * 1000.0));
		}
		if(ccEnd > target.ccUntil) target.ccUntil = ccEnd;
	}

	const bool chainedPush = target.clientHd == ClientHandle::INVALID ? localTime < target.pushUntil : localTime < target.clientPushUntil;
	const vec3 from = chainedPush ? vec3(vec2(target.pushTarget), target.body->GetWorldPos().z) : target.body->GetWorldPos();
	vec3 to = from;
	vec3 forceCenter = from;
	const f32 graphDuration = duration * (info && info->timeRatio > 0 ? info->timeRatio : 1.0f);
	const bool flyMove = info && info->type == StatusType::FLY && info->distanceRatio != 0;
	if(info && displacement && info->type != StatusType::AIRBORNE && (info->type != StatusType::FLY || flyMove)) {
		vec2 dir = vec2(from - caster.body->GetWorldPos());
		const f32 len = glm::length(dir);
		dir = len > 1.0f ? dir / len : vec2(cosf(caster.input.rot.upperYaw), sinf(caster.input.rot.upperYaw));
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
				STATUS_PUSH_BASE_DISTANCE * (info->distanceRatio != 0 ? info->distanceRatio : 1.0f);
			if(const PushToEdgeRule* edge = FindPushToEdgeRule(statusID)) {
				const Remote* wave = GetGameXmlContent().FindRemote(edge->remote);
				dist = wave ? MAX(wave->boundSize[0] * 0.5f - len, 0.0f) : dist;
			}
			to = from + vec3(dir * dist, 0);
		}
		LOG("Push :: %ls -> %ls status=%d sep=%.0f dir=(%.2f, %.2f) to=(%.0f, %.0f)", caster.name.data(), target.name.data(), statusID, len, dir.x, dir.y, to.x, to.y);
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
				target.dashOrigin = target.body->GetWorldPos();
				target.dashDist = rl;
				target.dashDone = 0;
				target.dashCurve = nullptr;
				target.dashUntil = TimeAdd(localTime, TimeMsToTime(graphDuration * 1000.0));
			}
		}
		else if(target.clientHd == ClientHandle::INVALID && graphDuration > 0) {
			const Time prevUntil = chainedPush ? target.pushUntil : localTime;
			const Time newUntil = TimeAdd(localTime, TimeMsToTime(graphDuration * 1000.0));
			target.pushUntil = newUntil > prevUntil ? newUntil : prevUntil;
			target.pushTarget = to;
			target.pushSpeed = glm::length(vec2(to - target.body->GetWorldPos())) / MAX((f32)TimeDiffSec(TimeDiff(localTime, target.pushUntil)), 0.05f);
			if(target.pushUntil > target.ccUntil) target.ccUntil = target.pushUntil;
		}
		else if(target.clientHd != ClientHandle::INVALID && graphDuration > 0) {
			target.clientPushUntil = TimeAdd(localTime, TimeMsToTime(graphDuration * 1000.0));
			target.pushTarget = to;
			const Time followUntil = TimeAdd(localTime, TimeMsToTime((graphDuration + 2.0f) * 1000.0));
			if(followUntil > target.ccUntil) target.ccUntil = followUntil;
		}
	}

	if(info && info->type == StatusType::DISPEL && info->dispelEffect >= 0) {
		foreach(it, activeStatuses) {
			if(it->targetIndex != target.index || !it->info || !it->info->canDispel || it->statusID == statusID) continue;
			if((it->info->isDebuff ? 1 : 0) != info->dispelEffect) continue;
			it->until = localTime;
			LOG("Dispel :: %ls removed status %d from %ls", caster.name.data(), it->statusID, target.name.data());
		}
	}

	LOG("Status :: %ls -> %ls status=%d type=%s duration=%.2f shield=%.0f", caster.name.data(), target.name.data(), statusID, info ? info->typeName.c_str() : "?", duration, st.shieldHP);
	replication->SendAddStatusToAll(statusID, st.targetUID, st.casterUID, sentDuration, from, dragForce ? forceCenter : to, graphDuration);
}

bool World::IsBlocked(const Player& p, bool attack, bool skill, bool shirk) const
{
	foreach_const(it, activeStatuses) {
		if(it->targetIndex != p.index || !it->info || localTime >= it->until) continue;
		if((attack && it->info->blockAttack) || (skill && it->info->blockSkill) || (shirk && it->info->blockShirk)) return true;
	}
	return false;
}

f32 World::StatusModMulSum(const Player& p, i32 stat) const
{
	f32 sum = 0;
	foreach_const(it, activeStatuses) {
		if(it->targetIndex != p.index || !it->info || localTime >= it->until) continue;
		foreach_const(m, it->info->mods) {
			if((i32)m->stat == stat && !m->regen && m->mul) sum += m->value * it->overlap;
		}
	}
	return sum;
}

f32 World::StatusModSum(const Player& p, i32 stat) const
{
	f32 sum = 0;
	foreach_const(it, activeStatuses) {
		if(it->targetIndex != p.index || !it->info) continue;
		foreach_const(m, it->info->mods) {
			if((i32)m->stat == stat && !m->regen && !m->mul) sum += m->value * it->overlap;
		}
	}
	return sum;
}

void World::UpdateStatuses()
{
	for(auto ps = pendingStatuses.begin(); ps != pendingStatuses.end(); ) {
		if(localTime < ps->time) { ++ps; continue; }
		Player& caster = players[ps->casterIndex];
		const ActorMaster* targetMaster = ps->targetUID != ActorUID::INVALID ? FindMasterActor(ps->targetUID) : nullptr;
		if(targetMaster && ps->remove) {
			foreach(it, activeStatuses) {
				if(it->targetIndex != targetMaster->parent->index || it->statusID != ps->statusID || localTime >= it->until) continue;
				it->until = localTime;
				LOG("Status :: %ls status=%d removed by action", targetMaster->parent->name.data(), ps->statusID);
			}
		}
		else if(!caster.dead && targetMaster) {
			ApplyStatus(caster, *targetMaster->parent, ps->statusID, ps->power, ps->duration);
		}
		ps = pendingStatuses.erase_unsorted(ps);
	}

	for(auto it = activeStatuses.begin(); it != activeStatuses.end(); ) {
		Player& target = players[it->targetIndex];
		if(it->info && it->info->dotDamage > 0 && it->info->intervalSec > 0) {
			while(localTime >= it->nextTick && it->nextTick <= it->until && !target.dead) {
				it->nextTick = TimeAdd(it->nextTick, TimeMsToTime(it->info->intervalSec * 1000.0));
				const i32 hpBefore = target.Main().hp;
				DamagePlayer(players[it->casterIndex], target, SkillID::INVALID, (i32)(it->info->dotDamage * it->overlap));
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
				if(!m->regen || m->stat != StatusMod::HEALTH) continue;
				const f32 ticks = MAX(m->count, 1.0f);
				const f32 interval = it->info->intervalSec > 0 ? it->info->intervalSec : MAX(it->info->durationSec, 0.1f) / ticks;
				if(it->healTicksDone >= (i32)ticks || localTime < it->nextHealTick) continue;
				it->nextHealTick = TimeAdd(localTime, TimeMsToTime(interval * 1000.0));
				it->healTicksDone++;
				const i32 amount = (i32)(m->mul ? target.Main().hpMax * m->value / 100.0f : m->value);
				HealPlayer(players[it->casterIndex], target, amount * it->overlap);
			}
		}

		if(it->info && it->info->auraStatus != 0 && !target.dead && localTime >= it->nextAuraPulse) {
			it->nextAuraPulse = TimeAdd(localTime, TimeMsToTime(500));
			const StatusInfo* child = GetGameXmlContent().FindStatus(it->info->auraStatus);
			const f32 range = it->info->targetingRange > 0 ? it->info->targetingRange : (child ? child->targetingRange : 0);
			if(child && range > 0) PulseAura(it->casterIndex, *it->info, vec2(target.body->GetWorldPos()), range, it->info->type == StatusType::DEFEAT_CASTER_AURA ? (i32)target.index : -1);
		}

		if(it->info && it->info->type == StatusType::LINK && localTime < it->until && it->casterIndex != it->targetIndex) {
			const GameXmlContent::SkillInput* in = GetGameXmlContent().FindSkillInput(it->info->skillIndex);
			const Player& linker = players[it->casterIndex];
			if(in && in->maxDistance > 0 && (linker.dead || glm::length(vec2(linker.body->GetWorldPos()) - vec2(target.body->GetWorldPos())) > in->maxDistance)) {
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
		const bool shieldBroken = it->info && ((it->info->type == StatusType::SHIELD && it->shieldHP <= 0) || (shieldBomb && it->shieldHP <= 0 && it->absorbed > 0));
		const bool timeOver = localTime >= it->until;
		if(timeOver || target.dead || shieldBroken || it->removeOnHit) {
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
			for(i32 k = 0; k < MAX(it->overlap, 1); k++) replication->SendRemoveStatusToAll(it->statusID, it->targetUID, it->casterUID);
			i32 next = 0;
			if(it->info && it->info->nextStatus != 0 && !target.dead) {
				const auto cond = it->info->nextCondition;
				if(cond == StatusInfo::NextCondition::ALL ||
				   (cond == StatusInfo::NextCondition::TIMEOVER && timeOver) ||
				   (cond == StatusInfo::NextCondition::REMOVE_ONHIT && it->removeOnHit)) next = it->info->nextStatus;
			}
			const u32 casterIndex = it->casterIndex;
			it = activeStatuses.erase_unsorted(it);
			if(next != 0 && !pendingStatuses.full()) {
				PendingStatus ps;
				ps.time = localTime;
				ps.statusID = next;
				ps.casterIndex = casterIndex;
				ps.targetUID = target.Main().UID;
				ps.duration = -1.0f;
				ps.power = 0;
				pendingStatuses.push_back(ps);
			}
		}
		else {
			++it;
		}
	}
}
