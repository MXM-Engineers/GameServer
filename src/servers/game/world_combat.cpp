#include "config.h"
#include "world.h"
#include "world_common.h"
#include <mxm/game_content.h>

static const i32 STAT_CRITICALRATE = 9;
static const i32 STAT_CRITICALDAMAGE = 10;
static const i32 STAT_RAGE_ATTACK_RECOVERY_VALUE = 51;
static const i32 STAT_SKILL_CRITICALRATE = 63;
static const i32 STAT_SKILL_CRITICALDAMAGE = 64;
static const u32 PACKET_DAMAGE_TYPE_NORMAL = 0;
static const u32 PACKET_DAMAGE_TYPE_CRITICAL = 2;
static const i32 STAT_ELEMENT_DAMAGE_BONUS = 54;
static const i32 STAT_ELEMENT_SKILL_DAMAGE_BONUS = 55;
static const i32 STAT_DEFENCE = 7;
static const i32 STAT_DEFENCE_PENETRATION = 20;
static const i32 STAT_SKILL_PENETRATION = 21;
static const i32 STAT_SKILLDAMAGE_REDUCE_RATIO = 58;
static const f32 DEFENCE_SCALE = 100.0f;
static const f32 PENETRATION_CAP = 50.0f;
static const f32 SKILLDAMAGE_REDUCE_CAP = 80.0f;
static const f32 TOOLTIP_SNIPER_CRIT_PER_MARK = 33.0f;
static const i32 STATUS_ELECTRO_PASSIVE_BUFF_MPREGEN = 1230040011;
static const i32 RESPAWN_DELAY_MS = 5000;

static bool ElementBeats(i32 a, i32 b)
{
	return (a == 1 && b == 2) || (a == 2 && b == 3) || (a == 3 && b == 1);
}

static f32 ElementMultiplier(const World::Player& attacker, const World::Player& target, bool skill)
{
	const auto& content = GetGameXmlContent();
	const ClassType ac = attacker.Main().classType;
	const i32 ea = content.GetCreatureElement((i32)content.GetMaster(ac).ID);
	const i32 et = content.GetCreatureElement((i32)content.GetMaster(target.Main().classType).ID);
	const i32 stat = skill ? STAT_ELEMENT_SKILL_DAMAGE_BONUS : STAT_ELEMENT_DAMAGE_BONUS;
	if(ElementBeats(ea, et)) return 1.0f + MasterStat(ac, stat) / 100.0f;
	if(ElementBeats(et, ea)) return 1.0f - MasterStat(target.Main().classType, stat) / 100.0f;
	return 1.0f;
}

void World::HealPlayer(Player& healer, Player& target, i32 amount)
{
	if(target.dead || amount <= 0) return;
	ActorMaster& chara = target.Main();
	chara.hp = MIN(chara.hp + amount, chara.hpMax);
	LOG("Heal :: %ls -> %ls amount=%d hp=%d/%d", healer.name.data(), target.name.data(), amount, chara.hp, chara.hpMax);
	if(healer.index != target.index && healer.Main().classType == ClassType::ELECTRO && FindPassiveOfType(healer, StatusType::ELECTRO)) ApplyStatus(healer, healer, STATUS_ELECTRO_PASSIVE_BUFF_MPREGEN);
	replication->SendHpToAll(chara.UID, chara.hp, chara.hpMax);
}

f32 World::HitRadius(const Player& p) const
{
	if(p.Main().classType == ClassType::NONE) return 0;
	return (f32)GetGameXmlContent().GetMaster(p.Main().classType).character.getColliderRadius();
}

f32 World::BodyMidHeight(const Player& p) const
{
	if(p.Main().classType == ClassType::NONE) return 0;
	return (f32)GetGameXmlContent().GetMaster(p.Main().classType).character.getColliderHeight() * 0.5f;
}

void World::DamagePlayer(Player& attacker, Player& target, SkillID skillID, i32 damage, const Projectile* proj, u32 remoteID, RemoteIdx remoteDoc, vec2 remoteDir)
{
	if(target.dead || target.untargetable) return;
	if(attacker.index != target.index) {
		attacker.lastHitTargetIndex = (i32)target.index;
		attacker.lastHitAt = localTime;
	}
	if(HasStatusType(target, StatusType::GODMODE)) return;
	const i32 damageIn = damage;
	f32 elementMul = 1.0f;
	if(HasStatusType(target, StatusType::EVADE) && attacker.team != target.team) {
		LOG("Evade :: %ls evaded %ls", target.name.data(), attacker.name.data());
		return;
	}

	{
		const RemoteIdx doc = proj ? proj->remoteDoc : remoteDoc;
		const Remote* pr = doc != RemoteIdx::INVALID ? GetGameXmlContent().FindRemote(doc) : nullptr;
		if(pr && pr->distPenaltyRate > 0 && pr->penaltyApplyDistance > 0 &&
		   glm::length(vec2(target.body->GetWorldPos()) - vec2(attacker.body->GetWorldPos())) < pr->penaltyApplyDistance) {
			damage = (i32)(damage * (1.0f - pr->distPenaltyRate / 100.0f));
		}
	}

	if(attacker.index != target.index) {
		const bool skillHit = !proj && skillID != SkillID::INVALID;
		const f32 def = MasterStat(target.Main().classType, STAT_DEFENCE) * MAX(1.0f + StatusModMulSum(target, StatusMod::DEFENCE) / 100.0f, 0.0f);
		const f32 pen = MIN(MAX(MasterStat(attacker.Main().classType, skillHit ? STAT_SKILL_PENETRATION : STAT_DEFENCE_PENETRATION), 0.0f), PENETRATION_CAP) / 100.0f;
		f32 mul = DEFENCE_SCALE / (DEFENCE_SCALE + MAX(def * (1.0f - pen), 0.0f));
		if(skillHit) mul *= 1.0f - MIN(MAX(MasterStat(target.Main().classType, STAT_SKILLDAMAGE_REDUCE_RATIO), 0.0f), SKILLDAMAGE_REDUCE_CAP) / 100.0f;
		damage = MAX((i32)(damage * mul + 0.5f), damage > 0 ? 1 : 0);
	}

	u32 damageType = PACKET_DAMAGE_TYPE_NORMAL;
	if(proj || skillID != SkillID::INVALID) {
		const f32 mul = ElementMultiplier(attacker, target, !proj);
		if(mul != 1.0f) damage = (i32)(damage * mul);
		elementMul = mul;

		const ClassType ac = attacker.Main().classType;
		const RemoteIdx doc = proj ? proj->remoteDoc : remoteDoc;
		const Remote* critRemote = doc != RemoteIdx::INVALID ? GetGameXmlContent().FindRemote(doc) : nullptr;
		const f32 rate = MasterStat(ac, proj ? STAT_CRITICALRATE : STAT_SKILL_CRITICALRATE) + (critRemote ? critRemote->criticalRate : 0) +
			StatusModSum(attacker, proj ? StatusMod::CRITICALRATE : StatusMod::SKILL_CRITICALRATE);
		f32 bonus = 0;
		foreach_const(st, activeStatuses) {
			if(st->targetIndex != target.index || !st->info || localTime >= st->until) continue;
			if(st->info->type == StatusType::WEAK_ON_CRITICAL && attacker.team != target.team) bonus = 100.0f;
			if(proj && st->info->type == StatusType::STACKBOMB && st->casterIndex == attacker.index && FindPassiveOfType(attacker, StatusType::SNIPER)) bonus += TOOLTIP_SNIPER_CRIT_PER_MARK * st->overlap;
		}
		if(rate + bonus > 0 && (RandUint() % 10000) < (u32)((rate + bonus) * 100.0f)) {
			const f32 critDamage = MasterStat(ac, proj ? STAT_CRITICALDAMAGE : STAT_SKILL_CRITICALDAMAGE);
			damage = (i32)(damage * (critDamage > 0 ? critDamage : 100.0f) / 100.0f);
			damageType = PACKET_DAMAGE_TYPE_CRITICAL;
		}
	}

	foreach(it, activeStatuses) {
		if(it->targetIndex == target.index && it->info && it->info->nextCondition == StatusInfo::NextCondition::REMOVE_ONHIT) it->removeOnHit = true;
	}

	foreach(it, activeStatuses) {
		if(damage <= 0) break;
		if(it->targetIndex != target.index || it->shieldHP <= 0) continue;
		const f32 absorbed = MIN(it->shieldHP, (f32)damage);
		it->shieldHP -= absorbed;
		it->absorbed += absorbed;
		damage -= (i32)absorbed;
	}
	if(damage <= 0) return;

	ActorMaster& chara = target.Main();
	target.damageTaken += damage;
	target.hitsTaken++;
	chara.hp = MAX(chara.hp - damage, (HasStatusType(target, StatusType::NEARLY_IMMORTAL) || HasStatusType(target, StatusType::UNDEAD)) ? 1 : 0);
	if(chara.hp == 0) PassiveOnLethal(target);
	if(!reflecting && attacker.team != target.team && attacker.index != target.index && (proj || skillID != SkillID::INVALID)) {
		const f32 ratio = StatusModSum(target, StatusMod::REFLECT_DAMAGE_RATIO);
		if(ratio > 0 && !attacker.dead) {
			reflecting = true;
			LOG("Status :: %ls reflects %d to %ls", target.name.data(), (i32)(damage * ratio / 100.0f), attacker.name.data());
			DamagePlayer(target, attacker, SkillID::INVALID, (i32)(damage * ratio / 100.0f));
			reflecting = false;
		}
	}

	if(attacker.res[1] >= 0) {
		const ClassType ac = attacker.Main().classType;
		const f32 rageMax = MasterStat(ac, RESOURCES[1].maxStat);
		if(rageMax > 0) attacker.res[1] = MIN(attacker.res[1] + MasterStat(ac, STAT_RAGE_ATTACK_RECOVERY_VALUE), rageMax);
	}

	LOG("Damage :: %ls -> %ls skill=%d damage=%d%s hp=%d/%d in=%d remote=%u element=%d", attacker.name.data(), target.name.data(), (i32)skillID, damage, damageType == PACKET_DAMAGE_TYPE_CRITICAL ? " CRIT" : "", chara.hp, chara.hpMax,
		damageIn, (u32)(proj ? proj->remoteDoc : remoteDoc), (i32)(elementMul * 100.0f));
	if(proj && proj->remoteID != 0) {
		replication->SendRemoteDamageToAll(proj->remoteID, proj->remoteDoc, attacker.Main().UID, chara.UID, target.body->GetWorldPos(), proj->dir, damage, chara.hp, chara.hpMax, damageType);
	}
	else if(remoteID != 0) {
		replication->SendRemoteDamageToAll(remoteID, remoteDoc, attacker.Main().UID, chara.UID, target.body->GetWorldPos(), remoteDir, damage, chara.hp, chara.hpMax, damageType);
	}
	else {
		replication->SendDamageToAll(attacker.Main().UID, chara.UID, skillID, damage, chara.hp, chara.hpMax, damageType);
	}

	if(chara.hp > 0) PassiveOnDamaged(target);
	if(chara.hp == 0) {
		target.dead = true;
		target.tRespawn = TimeAdd(localTime, TimeMsToTime(RESPAWN_DELAY_MS));
		LOG("Death :: %ls killed by %ls", target.name.data(), attacker.name.data());
		attacker.kills++;
		target.deaths++;
		i32 teamKills = 0;
		foreach_const(it, players) {
			if(it->team == attacker.team) teamKills += it->kills;
		}
		replication->SendScoreToAll(attacker.userID, attacker.team, attacker.kills, teamKills, target.userID, target.deaths);
		replication->SendDeathToAll(attacker.Main().UID, chara.UID, CreatureIndex(100000000 + (i32)chara.classType), target.userID, RESPAWN_DELAY_MS);
	}
}
