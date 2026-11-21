#include "config.h"
#include "world.h"
#include "world_common.h"
#include <mxm/game_content.h>

static const f64 NO_RESPAWN_MS = 1.0e12;
static const i32 STAT_CRITICALRATE = 9;
static const i32 STAT_CRITICALDAMAGE = 10;
static const i32 STAT_RAGE_ATTACK_RECOVERY_VALUE = 51;
static const i32 STAT_RAGE_DAMAGE_RECOVERY_RATIO = 50;

static const i32 TITAN_FORM_FIRST_DOC = 100000900;

ClassType World::ActionClass(const Player& p) const
{
	const auto& forms = GetGameXmlContent().titan.forms;
	if(p.titanForm < 0 || p.titanForm >= (i32)forms.size()) return p.Main().classType;
	const i32 offset = forms[p.titanForm].doc - TITAN_FORM_FIRST_DOC;
	return offset == 1 ? ClassType::TITAN_SIEGE : ClassType::TITAN;
}

const char* World::ActionClassName(const Player& p) const
{
	const ClassType cls = ActionClass(p);
	if(cls == ClassType::TITAN) return "TITAN";
	if(cls == ClassType::TITAN_SIEGE) return "TITAN_SIEGE";
	return GetGameXmlContent().GetMaster(cls).className.c_str();
}

void World::SendNpcShieldStat(ActorUID npcUID)
{
	f32 left = 0;
	f32 total = 0;
	foreach_const(ns, npcStatuses) {
		if(ns->npcUID != npcUID || ns->shieldMax <= 0 || ns->until <= localTime) continue;
		left += MAX(ns->shieldHP, 0.0f);
		total += ns->shieldMax;
	}
	replication->SendStatToAll(npcUID, (i32)StatType::STAT_TYPE_SHIELD_DEFENCE, left, total);
}

void World::SendShieldStat(const Player& p)
{
	f32 left = 0;
	f32 total = 0;
	foreach_const(it, activeStatuses) {
		if(it->targetIndex != p.index || it->shieldMax <= 0 || it->until <= localTime) continue;
		left += MAX(it->shieldHP, 0.0f);
		total += it->shieldMax;
	}
	replication->SendStatToAll(p.Main().UID, (i32)StatType::STAT_TYPE_SHIELD_DEFENCE, left, total);
}

bool World::TargetBody(ActorUID uid, vec2* pos, f32* radius) const
{
	if(const ActorMaster* tm = FindMasterActor(uid)) {
		if(!tm->parent || !tm->parent->body) return false;
		*pos = vec2(tm->parent->body->GetWorldPos());
		*radius = HitRadius(*tm->parent);
		return true;
	}
	if(const ActorNpc* npc = FindNpcActor(uid)) {
		*pos = vec2(npc->pos);
		*radius = NpcHitRadius(*npc);
		return true;
	}
	return false;
}

void World::GainRageOnHit(Player& attacker)
{
	const f32 gain = MasterStat(attacker.Main().classType, STAT_RAGE_ATTACK_RECOVERY_VALUE) + StatBonus(attacker, (u8)STAT_RAGE_ATTACK_RECOVERY_VALUE);
	if(gain > 0) GainResource(attacker, 1, gain);
}

void World::GainRageOnDamage(Player& target, i32 damage)
{
	const f32 gain = damage > 0 ? floorf(sqrtf((f32)damage)) * MasterStat(target.Main().classType, STAT_RAGE_DAMAGE_RECOVERY_RATIO) / 100.0f : 0.0f;
	if(gain > 0) GainResource(target, 1, gain);
}
static const u32 STAT_REASON_HIT = 1;
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
static const u32 PVP_ANNOUNCE_PLAIN_KILL = 0;
static const u32 PVP_ANNOUNCE_KILL = 1;
static const u32 PVP_ANNOUNCE_FIRST_BLOOD = 2;
static const u32 PVP_ANNOUNCE_MULTI_KILL = 3;
static const u32 PVP_ANNOUNCE_KILL_STREAK = 4;
static const u32 PVP_ANNOUNCE_TEAM_ALL_DEAD = 6;
static const u32 PVP_ANNOUNCE_SHUTDOWN = 7;
static const u32 PVP_ANNOUNCE_KILLED_BY_MONSTER = 8;

static const f64 MULTI_KILL_WINDOW_MS = 15000.0;
static const f64 DEAD_DAMAGE_WINDOW_MS = 15000.0;
static const f64 MEASURED_MONSTER_KILL_CREDIT_MS = 10000.0;
static const u32 PVP_ANNOUNCE_MONSTER_KILL_CREDITED = 11;
static const f32 PENETRATION_CAP = 50.0f;
static const f32 SKILLDAMAGE_REDUCE_CAP = 80.0f;
static const f64 RESPAWN_BASE_UNTIL_SEC[] = { 50.0, 170.0 };
static const i32 RESPAWN_BASE_MS[] = { 3000, 4000, 5000 };
static const i32 RESPAWN_LEAD_MIN[] = { 1, 2, 4 };
static const i32 RESPAWN_LEAD_BONUS_MS[] = { 2000, 4000, 7000 };

static i32 RespawnDelayMs(f64 elapsedSec, i32 lead)
{
	i32 step = 0;
	while(step < (i32)ARRAY_COUNT(RESPAWN_BASE_UNTIL_SEC) && elapsedSec >= RESPAWN_BASE_UNTIL_SEC[step]) step++;
	i32 delay = RESPAWN_BASE_MS[step];
	for(i32 i = ARRAY_COUNT(RESPAWN_LEAD_MIN) - 1; i >= 0; i--) {
		if(lead >= RESPAWN_LEAD_MIN[i]) { delay += RESPAWN_LEAD_BONUS_MS[i]; break; }
	}
	return delay;
}

static bool ElementBeats(Element a, Element b)
{
	return (a == Element::RED && b == Element::GREEN) || (a == Element::GREEN && b == Element::BLUE) || (a == Element::BLUE && b == Element::RED);
}

static u32 ElementRelation(f32 elementMul)
{
	return elementMul > 1.0f ? 1 : (elementMul < 1.0f ? 2 : 0);
}

static f32 ElementMultiplier(const World::Player& attacker, const World::Player& target, bool skill)
{
	const auto& content = GetGameXmlContent();
	const ClassType ac = attacker.Main().classType;
	const Element ea = content.GetCreatureElement((i32)content.GetMaster(ac).ID);
	const Element et = content.GetCreatureElement((i32)content.GetMaster(target.Main().classType).ID);
	const i32 stat = skill ? STAT_ELEMENT_SKILL_DAMAGE_BONUS : STAT_ELEMENT_DAMAGE_BONUS;
	if(ElementBeats(ea, et)) return 1.0f + MasterStat(ac, stat) / 100.0f;
	if(ElementBeats(et, ea)) return 1.0f - MasterStat(target.Main().classType, stat) / 100.0f;
	return 1.0f;
}

f32 World::TitanDefencePenetration(const Player& p) const
{
	const auto& forms = GetGameXmlContent().titan.forms;
	if(p.titanForm < 0 || p.titanForm >= (i32)forms.size()) return -1.0f;
	return MIN(MAX(forms[p.titanForm].Stat((u8)StatType::STAT_TYPE_DEFENCE_PENETRATION), 0.0f), 100.0f) / 100.0f;
}

f32 World::DefenceMultiplier(const Player& attacker, const Player& target, bool skillHit) const
{
	const f32 def = (MasterStat(target.Main().classType, STAT_DEFENCE) + StatBonus(target, STAT_DEFENCE)) * MAX(1.0f + StatusModMulSum(target, StatusMod::DEFENCE) / 100.0f, 0.0f);
	const i32 penStat = skillHit ? STAT_SKILL_PENETRATION : STAT_DEFENCE_PENETRATION;
	const f32 titanPen = skillHit ? -1.0f : TitanDefencePenetration(attacker);
	const f32 pen = titanPen >= 0 ? titanPen : MIN(MAX(MasterStat(attacker.Main().classType, penStat) + StatBonus(attacker, (u8)penStat), 0.0f), PENETRATION_CAP) / 100.0f;
	f32 mul = DEFENCE_SCALE / (DEFENCE_SCALE + MAX(def * (1.0f - pen), 0.0f));
	if(skillHit) mul *= 1.0f - MIN(MAX(MasterStat(target.Main().classType, STAT_SKILLDAMAGE_REDUCE_RATIO) + StatBonus(target, STAT_SKILLDAMAGE_REDUCE_RATIO) + StatusModSum(target, StatusMod::SKILLDAMAGE_REDUCE), 0.0f), SKILLDAMAGE_REDUCE_CAP) / 100.0f;
	mul *= MAX(1.0f + StatusModMulSum(target, StatusMod::DAMAGE_TAKEN) / 100.0f, 0.0f);
	return mul;
}

i32 World::StatusDotTickDamage(const Player& caster, const Player& target, const ActiveStatus& st) const
{
	f32 perStack = st.info->dotDamage + st.skillPower * st.info->dotMul;
	if(caster.index != target.index) perStack *= DefenceMultiplier(caster, target, true) * ElementMultiplier(caster, target, true);
	return MAX((i32)perStack, perStack > 0 ? 1 : 0) * st.overlap;
}

void World::PassiveOnWeaponHit(Player& attacker, bool npcTarget)
{
	if(attacker.dead || localTime < attacker.hitRecoverReadyAt) return;
	const ActiveStatus* st = FindPassiveOfType(attacker, StatusType::HIT_RECOVER);
	const HitRecoverRule* rule = st ? FindHitRecoverRule(st->statusID) : nullptr;
	ActorMaster& chara = attacker.Main();
	if(!rule || chara.hp >= chara.hpMax) return;
	attacker.hitRecoverReadyAt = TimeAdd(localTime, TimeMsToTime(rule->cooldownMs));
	const i32 amount = (i32)(chara.hpMax * (npcTarget ? rule->npcHpRatio : rule->hpRatio));
	chara.hp = MIN(chara.hp + amount, chara.hpMax);
	replication->SendStatToAll(chara.UID, (i32)StatType::STAT_TYPE_HEALTH, (f32)chara.hp, (f32)chara.hpMax, STAT_REASON_HIT);
	LOG("Passive :: %ls weapon hit recovers %d hp (status %d)", attacker.name.data(), amount, st->statusID);
}

void World::PassiveOnFriendlyHit(Player& owner)
{
	if(owner.dead || owner.mana < 0) return;
	const ActiveStatus* st = FindPassiveOfType(owner, StatusType::ELECTRO);
	if(!st || !st->info || st->info->hitEffectValue <= 0) return;
	const f32 manaMax = MasterStat(owner.Main().classType, STAT_MANA);
	if(manaMax <= 0 || owner.mana >= manaMax) return;
	owner.mana = MIN(owner.mana + st->info->hitEffectValue, manaMax);
	owner.manaDirty = true;
}

void World::HealPlayer(Player& healer, Player& target, i32 amount)
{
	if(target.dead || amount <= 0) return;
	ActorMaster& chara = target.Main();
	if(chara.hp >= chara.hpMax) return;
	const u32 healed = (u32)MIN(amount, chara.hpMax - chara.hp);
	healer.damageStats[END_DMG_HEAL] += healed;
	if(healer.index == target.index) healer.healSelf += healed;
	else healer.healOthers += healed;
	chara.hp = MIN(chara.hp + amount, chara.hpMax);
	LOG("Heal :: %ls -> %ls amount=%d hp=%d/%d", healer.name.data(), target.name.data(), amount, chara.hp, chara.hpMax);
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

void World::CounterAttack(Player& attacker, Player& target, bool weaponHit)
{
	i32 stun = 0;
	SkillID skill = SkillID::INVALID;
	foreach(it, activeStatuses) {
		if(it->targetIndex != target.index || !it->info || it->info->type != StatusType::COUNTERSTATUS || it->until <= localTime) continue;
		it->until = localTime;
		stun = (i32)it->info->ParamInt("_CustomAddStatusIndex");
		skill = SkillID(it->info->skillIndex);
		break;
	}
	if(skill == SkillID::INVALID || !weaponHit || attacker.dead || !WeaponIsMelee(attacker)) return;
	LOG("Status :: %ls counters %ls (skill %d, status %d)", target.name.data(), attacker.name.data(), (i32)skill, stun);
	const ActorUID attackerUID = attacker.Main().UID;
	PlayerCastSkill(target, skill, attacker.body->GetWorldPos(), Slice<const ActorUID>(&attackerUID, 1), 0, ActionStateID::EXTRAMOTION_3_BEHAVIORSTATE);
	if(stun != 0) ApplyStatus(target, attacker, stun);
}

void World::OnPlayerKilled(Player& attacker, Player& target, u32 remoteID, RemoteIdx remoteDoc)
{
	const ActorMaster& chara = target.Main();
	target.dead = true;
	if(attacker.team != target.team) {
		attacker.kills++;
		MissionLive(attacker, 1, 0);
	}
	target.deaths++;
	if(!map || !map->ingameLevels) GrantSkillPoint(target);
	DropTitanPieces(target);
	DropMasterSoul(target);
	const bool targetWasTitan = target.titanForm >= 0;
	RevertTitan(target);
	target.occupyStreak = 0;
	i32 teamKills = 0;
	i32 victimTeamKills = 0;
	foreach_const(it, players) {
		if(it->team == attacker.team) teamKills += it->kills;
		if(it->team == target.team) victimTeamKills += it->kills;
	}
	const i32 respawnMs = fixedRespawnMs > 0 ? fixedRespawnMs : map && map->ingameLevels ? IngameRespawnDelayMs(target) : gameStartedAt == Time::ZERO ? RESPAWN_BASE_MS[ARRAY_COUNT(RESPAWN_BASE_MS) - 1] : RespawnDelayMs(TimeDiffSec(TimeDiff(gameStartedAt, localTime)), victimTeamKills - teamKills);
	target.tRespawn = TimeAdd(localTime, TimeMsToTime(noRespawn ? NO_RESPAWN_MS : (f64)respawnMs));
	LOG("Death :: %ls killed by %ls (respawn %d ms)", target.name.data(), attacker.name.data(), respawnMs);
	if(target.hiddenUID != ActorUID::INVALID) {
		ShowHiddenToEnemies(target, target.hiddenUID, ActorUID::INVALID);
		target.hiddenUID = ActorUID::INVALID;
	}
	bool specialAnnounced = false;
	if(attacker.team != target.team) {
		if(!firstBloodDone) {
			firstBloodDone = true;
			attacker.firstBlood = true;
			specialAnnounced = true;
			replication->SendPvpAnnouncementToAll(PVP_ANNOUNCE_FIRST_BLOOD, 0, attacker.Main().UID, chara.UID, KillPoints());
		}
		const bool chained = attacker.lastKillAt != Time::ZERO && localTime < TimeAdd(attacker.lastKillAt, TimeMsToTime(MULTI_KILL_WINDOW_MS));
		attacker.multiKill = chained ? attacker.multiKill + 1 : 1;
		attacker.lastKillAt = localTime;
		attacker.killStreak++;
		attacker.bestKillStreak = MAX(attacker.bestKillStreak, attacker.killStreak);
		if(attacker.titanForm >= 0) {
			attacker.titanKillStreak++;
			attacker.bestTitanKillStreak = MAX(attacker.bestTitanKillStreak, attacker.titanKillStreak);
		}
		if(targetWasTitan) attacker.transformKills++;
		if(damageStatusID != 0) attacker.dotKills++;
		if(attacker.multiKill >= 2) replication->SendPvpAnnouncementToAll(PVP_ANNOUNCE_MULTI_KILL, (u32)attacker.multiKill, attacker.Main().UID, chara.UID, KillPoints());
		if(attacker.killStreak >= 2) replication->SendPvpAnnouncementToAll(PVP_ANNOUNCE_KILL_STREAK, (u32)attacker.killStreak, attacker.Main().UID, chara.UID, KillPoints());
		if(target.killStreak >= 2) replication->SendPvpAnnouncementToAll(PVP_ANNOUNCE_SHUTDOWN, 0, attacker.Main().UID, chara.UID, KillPoints());
		specialAnnounced = specialAnnounced || attacker.multiKill >= 2 || attacker.killStreak >= 2 || target.killStreak >= 2;
	}
	if(!specialAnnounced) replication->SendPvpAnnouncementToAll(PVP_ANNOUNCE_PLAIN_KILL, 0, attacker.Main().UID, chara.UID, KillPoints());
	target.killStreak = 0;
	target.titanKillStreak = 0;
	replication->SendPvpAnnouncementToAll(PVP_ANNOUNCE_KILL, 0, attacker.Main().UID, chara.UID, KillPoints());
	if(attacker.team != target.team) {
		bool allDead = true;
		foreach_const(it, players) if(it->team == target.team && !it->dead) allDead = false;
		if(allDead) replication->SendPvpAnnouncementToAll(PVP_ANNOUNCE_TEAM_ALL_DEAD, (u32)(target.team == 0 ? TeamType::RED : TeamType::BLUE), attacker.Main().UID, chara.UID, KillPoints());
	}
	const bool pointsMode = occupyMode || (map && map->ingameLevels);
	if(occupyMode) attacker.points += (i32)KillPoints();
	AddPoints(attacker, (i32)KillPoints());
	replication->SendScoreToAll(attacker.userID, attacker.team, attacker.kills, pointsMode ? attacker.points : attacker.kills, teamKills, pointsMode ? TeamPoints(attacker.team) : teamKills, target.userID, target.team, target.deaths, victimTeamKills);
	for(i32 k = 0; k < (i32)target.recentDamage.size(); ) {
		if(localTime > TimeAdd(target.recentDamage[k].at, TimeMsToTime(DEAD_DAMAGE_WINDOW_MS))) target.recentDamage.erase(target.recentDamage.begin() + k);
		else k++;
	}
	Time firstDamageAt = localTime;
	foreach_const(d, target.recentDamage) if(d->at < firstDamageAt) firstDamageAt = d->at;
	foreach(it, players) {
		if(it->index == attacker.index || it->team == target.team) continue;
		bool assisted = false;
		foreach_const(d, target.recentDamage) {
			if(d->attackerIndex == it->index && d->damage > 0) assisted = true;
		}
		if(!assisted) continue;
		it->assists++;
		MissionLive(*it, 0, 1);
		i32 teamScore = 0;
		foreach_const(p, players) if(p->team == it->team) teamScore += p->kills;
		if(pointsMode) teamScore = TeamPoints(it->team);
		replication->SendAssistScoreToAll(it->userID, it->team, it->assists, pointsMode ? it->points : it->kills, teamScore);
	}
	RewardMasterKill(&attacker, target);
	if(attacker.team != target.team) PassiveCooldownsOnKill(attacker);
	replication->SendDeadDamageInfo(target.clientHd, attacker.index, target.recentDamage.data(), (i32)target.recentDamage.size(), (i32)TimeDiffMs(TimeDiff(firstDamageAt, localTime)));
	target.recentDamage.clear();
	SyncPassivesOnDeath(target);
	replication->SendDeathToAll(attacker.Main().UID, chara.UID, CreatureIndex(100000000 + (i32)attacker.Main().classType), remoteID, remoteDoc, target.userID, respawnMs);
}

void World::DamagePlayer(Player& attacker, Player& target, SkillID skillID, i32 damage, const Projectile* proj, u32 remoteID, RemoteIdx remoteDoc, vec2 remoteDir)
{
	const bool preMitigated = damagePreMitigated;
	damagePreMitigated = false;
	if(frozen || target.dead || target.untargetable) return;
	if(attacker.index != target.index) {
		attacker.lastHitTargetIndex = (i32)target.index;
		attacker.lastHitAt = localTime;
		MarkInBattle(attacker);
		MarkInBattle(target);
		if(target.body) TowerCallForHelp(attacker, target.body->GetWorldPos(), (Faction)target.team, nullptr);
		if(attacker.team != target.team && !(attacker.fogSeenBy & (1u << target.team))) {
			RevealToTeam(attacker, (i32)target.team);
			nextFogUpdate = localTime;
		}
	}
	if(HasStatusType(target, StatusType::GODMODE)) return;
	if(attacker.team != target.team && HasStatusType(target, StatusType::BURROW)) return;
	if(attacker.team != target.team && !proj && skillID != SkillID::INVALID && HasStatusType(target, StatusType::IMMUNESKILL)) {
		LOG("Status :: %ls immune to skill %d", target.name.data(), (i32)skillID);
		return;
	}
	const i32 damageIn = damage;
	f32 elementMul = 1.0f;
	if(HasStatusType(target, StatusType::EVADE) && attacker.team != target.team) {
		LOG("Evade :: %ls evaded %ls", target.name.data(), attacker.name.data());
		replication->SendEvadeToAll(attacker.Main().UID, target.Main().UID, proj ? proj->remoteID : remoteID, proj ? proj->remoteDoc : remoteDoc, proj ? proj->seedOwnerHd : ClientHandle::INVALID);
		OnEvade(target);
		return;
	}

	if(attacker.team != target.team) damage = (i32)(damage * GroupingDamageMul(attacker, target));
	{
		const RemoteIdx doc = proj ? proj->remoteDoc : remoteDoc;
		damage = (i32)(damage * RemoteTargetDamageMul(attacker, doc, target.body->GetWorldPos(), &target, nullptr));
	}

	if(attacker.index != target.index && !preMitigated) {
		const bool skillHit = !proj && skillID != SkillID::INVALID;
		damage = MAX((i32)(damage * DefenceMultiplier(attacker, target, skillHit) + 0.5f), damage > 0 ? 1 : 0);
	}

	u32 damageType = PACKET_DAMAGE_TYPE_NORMAL;
	if(proj || skillID != SkillID::INVALID || remoteDoc != RemoteIdx::INVALID) {
		const bool weaponHit = proj || skillID == SkillID::INVALID;
		const f32 mul = ElementMultiplier(attacker, target, !weaponHit);
		if(mul != 1.0f) damage = (i32)(damage * mul);
		elementMul = mul;

		const RemoteIdx doc = proj ? proj->remoteDoc : remoteDoc;
		f32 bonus = 0;
		foreach_const(st, activeStatuses) {
			if(st->targetIndex != target.index || !st->info || localTime >= st->until) continue;
			if(st->info->type == StatusType::WEAK_ON_CRITICAL && attacker.team != target.team) bonus = 100.0f;
		}
		if(proj && !proj->hitList.empty()) bonus += PierceCriticalBonus(doc, (u32)(proj->hitList.size() - 1 + proj->npcHitList.size()));
		const f32 critMul = RollCritical(attacker, weaponHit, doc, bonus);
		if(critMul > 0) {
			damage = (i32)(damage * critMul);
			damageType = PACKET_DAMAGE_TYPE_CRITICAL;
		}
	}

	foreach(it, activeStatuses) {
		if(it->targetIndex == target.index && it->info && it->info->nextCondition == StatusInfo::NextCondition::REMOVE_ONHIT) it->removeOnHit = true;
	}

	foreach(it, activeStatuses) {
		if(it->targetIndex != target.index || it->shieldHits <= 0 || it->until <= localTime || attacker.team == target.team) continue;
		it->shieldHits--;
		LOG("Status :: %ls shield %d absorbs a hit (%d left)", target.name.data(), it->statusID, it->shieldHits);
		CounterAttack(attacker, target, proj != nullptr);
		return;
	}
	bool shieldHit = false;
	foreach(it, activeStatuses) {
		if(damage <= 0) break;
		if(it->targetIndex != target.index || it->shieldHP <= 0) continue;
		const f32 absorbed = MIN(it->shieldHP, (f32)damage);
		it->shieldHP -= absorbed;
		it->absorbed += absorbed;
		damage -= (i32)absorbed;
		shieldHit = true;
	}
	if(shieldHit) SendShieldStat(target);
	if(damage <= 0) return;

	ActorMaster& chara = target.Main();
	ShowcaseOnDamage(target, damage);
	target.damageTaken += damage;
	target.hitsTaken++;
	target.lastDamagedAt = localTime;
	chara.hp = MAX(chara.hp - damage, (HasStatusType(target, StatusType::NEARLY_IMMORTAL) || HasStatusType(target, StatusType::UNDEAD)) ? 1 : 0);
	if(ShowcaseKeepsAlive(target)) chara.hp = MAX(chara.hp, 1);
	if(chara.hp == 0) PassiveOnLethal(target);
	if(damage > 0 && target.casting.skillID != SkillID::INVALID && target.casting.interruptByAttacked) CancelCast(target, "attacked");
	if(!reflecting && attacker.team != target.team && attacker.index != target.index && (proj || skillID != SkillID::INVALID)) {
		const f32 ratio = StatusModSum(target, StatusMod::REFLECT_DAMAGE_RATIO);
		if(ratio > 0 && !attacker.dead) {
			reflecting = true;
			LOG("Status :: %ls reflects %d to %ls", target.name.data(), (i32)(damage * ratio / 100.0f), attacker.name.data());
			DamagePlayer(target, attacker, SkillID::INVALID, (i32)(damage * ratio / 100.0f));
			reflecting = false;
		}
	}

	GainRageOnHit(attacker);
	if(attacker.index != target.index) GainRageOnDamage(target, damage);

	const bool weaponHit = skillID == SkillID::INVALID && damageStatusID == 0 && damageAttackerOverride == ActorUID::INVALID && (proj || remoteDoc != RemoteIdx::INVALID);
	LOG("Damage :: %ls -> %ls skill=%d damage=%d%s hp=%d/%d in=%d remote=%u element=%d", attacker.name.data(), target.name.data(), (i32)skillID, damage, damageType == PACKET_DAMAGE_TYPE_CRITICAL ? " CRIT" : "", chara.hp, chara.hpMax,
		damageIn, (u32)(proj ? proj->remoteDoc : remoteDoc), (i32)(elementMul * 100.0f));
	if(attacker.team != target.team) {
		RevealToTeam(attacker, target.team);
		LightAttacker(attacker.body->GetWorldPos(), target.team);
		attacker.damageDealt += damage;
		const bool normalHit = weaponHit;
		if(normalHit) WeaponLifesteal(attacker, damage);
		attacker.damageStats[END_DMG_MAX_DEALT] = MAX(attacker.damageStats[END_DMG_MAX_DEALT], (u32)damage);
		attacker.damageStats[normalHit ? END_DMG_NORMAL_DEALT : END_DMG_SKILL_DEALT] += (u32)damage;
		target.damageStats[END_DMG_TAKEN] += (u32)damage;
		target.damageStats[END_DMG_MAX_TAKEN] = MAX(target.damageStats[END_DMG_MAX_TAKEN], (u32)damage);
		target.damageStats[normalHit ? END_DMG_NORMAL_TAKEN : END_DMG_SKILL_TAKEN] += (u32)damage;
		for(i32 k = 0; k < (i32)target.recentDamage.size(); ) {
			if(localTime > TimeAdd(target.recentDamage[k].at, TimeMsToTime(DEAD_DAMAGE_WINDOW_MS))) target.recentDamage.erase(target.recentDamage.begin() + k);
			else k++;
		}
		if(target.recentDamage.full()) target.recentDamage.erase(target.recentDamage.begin());
		target.recentDamage.push_back({ attacker.index, attacker.userID, damageAttackerOverride != ActorUID::INVALID ? damageAttackerOverride : attacker.Main().UID, proj ? SkillID::INVALID : skillID, damageStatusID, damage, (u8)(damageType == PACKET_DAMAGE_TYPE_CRITICAL ? 1 : 0), localTime });
	}
	f32 carryLift = 0;
	foreach_const(st, activeStatuses) {
		if(st->targetIndex != target.index || localTime >= st->until) continue;
		for(const CarryLiftRule& rule : MEASURED_CARRY_LIFTS) if(rule.status == st->statusID) carryLift = rule.lift;
	}
	if(proj && proj->remoteID != 0) {
		Replication::RemoteDamage hit;
		hit.remoteID = proj->remoteID;
		hit.remoteDoc = proj->remoteDoc;
		hit.attackerUID = attacker.Main().UID;
		hit.targetUID = chara.UID;
		hit.remotePos = vec3(vec2(proj->pos), target.body->GetWorldPos().z + carryLift);
		hit.targetPos = target.body->GetWorldPos() + vec3(0, 0, carryLift);
		hit.dir = proj->dir;
		hit.damageType = damageType;
		hit.elementRelation = ElementRelation(elementMul);
		hit.damage = damage;
		hit.hp = chara.hp;
		hit.hpMax = chara.hpMax;
		hit.seedOwnerHd = proj->seedOwnerHd;
		replication->SendRemoteDamageToAll(hit);
	}
	else if(remoteID != 0) {
		Replication::RemoteDamage hit;
		hit.remoteID = remoteID;
		hit.remoteDoc = remoteDoc;
		hit.attackerUID = damageAttackerOverride != ActorUID::INVALID ? damageAttackerOverride : attacker.Main().UID;
		hit.targetUID = chara.UID;
		hit.remotePos = statusForceCenterSet ? statusForceCenter : target.body->GetWorldPos() + vec3(0, 0, carryLift);
		hit.targetPos = target.body->GetWorldPos() + vec3(0, 0, carryLift);
		hit.dir = remoteDir;
		hit.damageType = damageType;
		hit.elementRelation = ElementRelation(elementMul);
		hit.damage = damage;
		hit.hp = chara.hp;
		hit.hpMax = chara.hpMax;
		hit.seedOwnerHd = damageSeedOwnerOverride;
		replication->SendRemoteDamageToAll(hit);
	}
	else {
		replication->SendDamageToAll(attacker.Main().UID, chara.UID, skillID, damage, chara.hp, chara.hpMax, damageType, damageStatusID);
	}
	if(weaponHit && attacker.index != target.index && attacker.team != target.team) PassiveOnWeaponHit(attacker, false);

	if(chara.hp > 0) PassiveOnDamaged(target);
	if(chara.hp == 0) OnPlayerKilled(attacker, target, proj ? proj->remoteID : remoteID, proj ? proj->remoteDoc : remoteDoc);
}

bool World::NpcHittableBy(const ActorNpc& npc, const Player& attacker) const
{
	if(npc.hp <= 0 || npc.removeAt != Time::ZERO) return false;
	if(npc.despawnAt != Time::ZERO && localTime >= npc.despawnAt) return false;
	if(npc.ownerUID == ActorUID::INVALID) {
		if(!npc.monsterAi) return false;
		if(GetGameXmlContent().CreatureUntargetable((i32)npc.docID)) return false;
		if(npc.faction == Faction::RED || npc.faction == Faction::BLUE) return npc.faction != (Faction)attacker.team;
		return npc.faction == Faction::ORANGE || npc.faction == Faction::MONSTER;
	}
	return players[npc.ownerIndex].team != attacker.team;
}

f32 World::NpcHitRadius(const ActorNpc& npc) const
{
	const SummonInfo* info = GetGameXmlContent().FindSummon((i32)npc.docID);
	if(info) return info->colliderRadius;
	return npc.monsterAi ? npc.monsterAi->radius : 0.0f;
}

u32 World::NpcDensity(const ActorNpc& npc) const
{
	const SummonInfo* info = GetGameXmlContent().FindSummon((i32)npc.docID);
	if(info) return (u32)info->density;
	return npc.monsterAi ? (u32)npc.monsterAi->density : 0;
}

void World::WeaponLifesteal(Player& attacker, i32 damage)
{
	const f32 ratio = StatusRegenBonus(attacker, StatusMod::LIFESTEAL, false);
	ActorMaster& chara = attacker.Main();
	if(ratio <= 0 || damage <= 0 || attacker.dead || chara.hp <= 0 || chara.hp >= chara.hpMax) return;
	const i32 hpBefore = chara.hp;
	chara.hp = MIN(chara.hp + MAX((i32)(damage * ratio / 100.0f), 1), chara.hpMax);
	attacker.healSelf += (u32)(chara.hp - hpBefore);
	replication->SendStatToAll(chara.UID, (i32)StatType::STAT_TYPE_HEALTH, (f32)chara.hp, (f32)chara.hpMax);
}

bool World::HasStatusTypeKey(const Player* player, const ActorNpc* npc, u32 key, const Player* caster) const
{
	if(player) {
		foreach_const(st, activeStatuses) if(st->targetIndex == player->index && st->info && st->info->typeKey == key && st->until > localTime && (!caster || st->casterIndex == caster->index)) return true;
	}
	if(npc) {
		foreach_const(ns, npcStatuses) {
			if(ns->npcUID != npc->UID || localTime >= ns->until) continue;
			if(caster && ns->casterUID != caster->Main().UID && ns->casterUID != caster->Sub().UID) continue;
			const StatusInfo* info = GetGameXmlContent().FindStatus(ns->statusID);
			if(info && info->typeKey == key) return true;
		}
	}
	return false;
}

f32 World::RemoteTargetDamageMul(const Player& attacker, RemoteIdx doc, const vec3& targetPos, const Player* targetPlayer, const ActorNpc* targetNpc) const
{
	const Remote* pr = doc != RemoteIdx::INVALID ? GetGameXmlContent().FindRemote(doc) : nullptr;
	if(!pr) return 1.0f;
	f32 mul = 1.0f;
	if(pr->distPenaltyRate > 0 && pr->penaltyApplyDistance > 0 && attacker.body && glm::length(vec2(targetPos) - vec2(attacker.body->GetWorldPos())) < pr->penaltyApplyDistance) mul *= 1.0f - pr->distPenaltyRate / 100.0f;
	if(pr->statusTypeMultiplyKey != 0 && pr->statusTypeMultiplier != 1.0f) {
		if(HasStatusTypeKey(targetPlayer, targetNpc, pr->statusTypeMultiplyKey, pr->statusTypeMultiplierToCaster ? &attacker : nullptr)) mul *= pr->statusTypeMultiplier;
	}
	return mul;
}

f32 World::PierceCriticalBonus(RemoteIdx doc, u32 earlierHits) const
{
	const Remote* r = doc != RemoteIdx::INVALID ? GetGameXmlContent().FindRemote(doc) : nullptr;
	if(!r || r->hitCriticalRate <= 0 || earlierHits == 0) return 0.0f;
	const f32 bonus = r->hitCriticalRate * (f32)earlierHits;
	return r->maxHitCriticalRate > 0 ? MIN(bonus, r->maxHitCriticalRate) : bonus;
}

f32 World::RollCritical(const Player& attacker, bool weapon, RemoteIdx doc, f32 bonus)
{
	const ClassType ac = attacker.Main().classType;
	const Remote* critRemote = doc != RemoteIdx::INVALID ? GetGameXmlContent().FindRemote(doc) : nullptr;
	const f32 rate = MasterStat(ac, weapon ? STAT_CRITICALRATE : STAT_SKILL_CRITICALRATE) + (weapon ? StatBonus(attacker, STAT_CRITICALRATE) : 0.0f) + (critRemote ? critRemote->criticalRate : 0) +
		StatusModSum(attacker, weapon ? StatusMod::CRITICALRATE : StatusMod::SKILL_CRITICALRATE);
	if(rate + bonus <= 0 || (RandUint() % 10000) >= (u32)((rate + bonus) * 100.0f)) return 0.0f;
	const f32 critDamage = MasterStat(ac, weapon ? STAT_CRITICALDAMAGE : STAT_SKILL_CRITICALDAMAGE);
	return (critDamage > 0 ? critDamage : 100.0f) / 100.0f;
}

void World::DamageNpc(Player& attacker, ActorNpc& npc, SkillID skillID, i32 damage, const Replication::RemoteDamage* remoteHit)
{
	if(damage <= 0 || !NpcHittableBy(npc, attacker)) return;
	if(npc.ownerUID != ActorUID::INVALID && OneHitSummon((i32)npc.docID)) {
		npc.hp = 0;
		replication->SendHpToAll(npc.UID, 0, npc.hpMax);
		npc.killerUID = attacker.Main().UID;
		npc.despawnAt = localTime;
		GrantMonsterKillExp(npc, attacker.Main().UID);
		LOG("Summon :: creature=%d destroyed by one hit from %ls", (i32)npc.docID, attacker.name.data());
		return;
	}
	if(npc.ownerUID == ActorUID::INVALID && NpcHasStatusType(npc, StatusType::GODMODE)) return;
	if(npc.monsterEvadeUntil != Time::ZERO && localTime < npc.monsterEvadeUntil) return;
	const f32 titanPen = skillID == SkillID::INVALID ? MAX(TitanDefencePenetration(attacker), 0.0f) : 0.0f;
	const f32 critMul = remoteHit ? RollCritical(attacker, skillID == SkillID::INVALID, remoteHit->remoteDoc, PierceCriticalBonus(remoteHit->remoteDoc, remoteHit->pierceHits)) : 0.0f;
	const f32 remoteMul = remoteHit ? RemoteTargetDamageMul(attacker, remoteHit->remoteDoc, npc.pos, nullptr, &npc) : 1.0f;
	const i32 dealt = MAX((i32)(damage * remoteMul * (critMul > 0 ? critMul : 1.0f) * DEFENCE_SCALE / (DEFENCE_SCALE + MAX(npc.defence * (1.0f - titanPen), 0.0f)) * NpcStatusMul(npc, StatusMod::DAMAGE_TAKEN) + 0.5f), 1);
	const i32 hpBefore = npc.hp;
	if(npc.faction == Faction::MONSTER && npc.ownerUID == ActorUID::INVALID) {
		static const f64 HYP_HIT_COMBO_WINDOW_MS = 3000.0;
		attacker.stageHitCombo = attacker.lastComboHit != Time::ZERO && TimeDurationMs(attacker.lastComboHit, localTime) <= HYP_HIT_COMBO_WINDOW_MS ? attacker.stageHitCombo + 1 : 1;
		attacker.lastComboHit = localTime;
		attacker.bestHitCombo = MAX(attacker.bestHitCombo, attacker.stageHitCombo);
	}
	npc.hp = MAX(npc.hp - AbsorbNpcShield(npc, dealt), 0);
	GainRageOnHit(attacker);
	if(npc.monsterWeakUntil != Time::ZERO && localTime < npc.monsterWeakUntil && localTime >= npc.monsterWeakFrom) npc.monsterWeakHit = true;
	if(IsStructure(npc)) attacker.damageStats[END_DMG_BUILDING_DEALT] += (u32)(hpBefore - npc.hp);
	static const u32 coreTag = StatusTypeKey("Core");
	if(npc.monsterAi && npc.monsterAi->tagKey == coreTag) attacker.coreDamage += hpBefore - npc.hp;
	if(skillID == SkillID::INVALID && remoteHit) WeaponLifesteal(attacker, hpBefore - npc.hp);
	if(npc.faction == Faction::RED || npc.faction == Faction::BLUE) {
		RevealToTeam(attacker, (i32)npc.faction);
		LightAttacker(attacker.body->GetWorldPos(), (i32)npc.faction);
	}
	if(npc.ownerUID == ActorUID::INVALID) OnMonsterDamaged(npc, hpBefore);
	if(pilotTour.kind == 2 && pilotTour.npc == npc.UID && !pilotTour.hit) {
		pilotTour.hit = true;
		PilotTourEvent("first hit");
	}
	if(npc.ownerUID == ActorUID::INVALID) MonsterAggroOnHit(npc, attacker);
	if(remoteHit && remoteHit->remoteID != 0) {
		Replication::RemoteDamage hit = *remoteHit;
		hit.attackerUID = damageAttackerOverride != ActorUID::INVALID ? damageAttackerOverride : attacker.Main().UID;
		hit.targetUID = npc.UID;
		hit.targetPos = npc.pos;
		hit.damageType = critMul > 0 ? PACKET_DAMAGE_TYPE_CRITICAL : hit.damageType;
		hit.damage = MIN(dealt, hpBefore);
		hit.hp = npc.hp;
		hit.hpMax = npc.hpMax;
		replication->SendRemoteDamageToAll(hit);
	}
	else replication->SendDamageToAll(damageAttackerOverride != ActorUID::INVALID ? damageAttackerOverride : attacker.Main().UID, npc.UID, skillID, MIN(dealt, hpBefore), npc.hp, npc.hpMax, 0, damageStatusID);
	if(skillID == SkillID::INVALID && remoteHit && damageAttackerOverride == ActorUID::INVALID) PassiveOnWeaponHit(attacker, true);
	LOG("Summon :: creature=%d takes %d from %ls hp=%d/%d", (i32)npc.docID, dealt, attacker.name.data(), npc.hp, npc.hpMax);
	if(npc.hp <= 0) {
		if(npc.ownerUID == ActorUID::INVALID) {
			MonsterDie(npc, attacker.Main().UID, CreatureIndex(100000000 + (i32)attacker.Main().classType));
			return;
		}
		npc.killerUID = attacker.Main().UID;
		npc.despawnAt = localTime;
		GroupingSummonKill(attacker, npc);
	}
}

i32 World::AbsorbNpcShield(const ActorNpc& npc, i32 dealt)
{
	i32 left = dealt;
	foreach(ns, npcStatuses) {
		if(left <= 0) break;
		if(ns->npcUID != npc.UID || ns->shieldHP <= 0 || localTime >= ns->until) continue;
		const f32 absorbed = MIN(ns->shieldHP, (f32)left);
		ns->shieldHP -= absorbed;
		left -= (i32)absorbed;
		LOG("Status :: creature=%d shield %d absorbs %.0f (%.0f left)", (i32)npc.docID, ns->statusID, absorbed, ns->shieldHP);
	}
	if(left != dealt) SendNpcShieldStat(npc.UID);
	return left;
}

void World::DamageNpcFromNpc(ActorNpc& attacker, ActorNpc& target, i32 damage, i32 remote, u32 remoteID, i32 status, bool exact)
{
	if(target.hp <= 0 || target.removeAt != Time::ZERO) return;
	if(NpcHasStatusType(target, StatusType::GODMODE)) return;
	if(damage <= 0) {
		if(status > 0) ApplyStatusToNpcFromNpc(attacker, target, status);
		return;
	}
	const i32 dealt = exact ? damage : MAX((i32)(damage * DEFENCE_SCALE / (DEFENCE_SCALE + MAX(target.defence, 0.0f)) * NpcStatusMul(target, StatusMod::DAMAGE_TAKEN) + 0.5f), 1);
	const i32 hpBefore = target.hp;
	target.hp = MAX(target.hp - AbsorbNpcShield(target, dealt), 0);
	if(target.ownerUID == ActorUID::INVALID) OnMonsterDamaged(target, hpBefore);
	if((target.faction == Faction::RED || target.faction == Faction::BLUE) && (attacker.faction == Faction::RED || attacker.faction == Faction::BLUE) && attacker.faction != target.faction) LightAttacker(attacker.pos, (i32)target.faction);
	SendNpcHit(attacker, target.UID, target.pos, remote, remoteID, MIN(dealt, hpBefore), target.hp, target.hpMax);
	if(status > 0) ApplyStatusToNpcFromNpc(attacker, target, status);
	if(target.hp > 0) return;
	if(target.ownerUID == ActorUID::INVALID) MonsterDie(target, attacker.UID, attacker.docID);
	else {
		target.killerUID = attacker.UID;
		target.despawnAt = localTime;
	}
}

void World::ApplyStatusFromNpc(const ActorNpc& caster, Player& target, i32 statusID, f32 durationOverride)
{
	const ActorUID prevCaster = statusCasterOverride;
	const bool prevCenterSet = statusForceCenterSet;
	const vec3 prevCenter = statusForceCenter;
	statusCasterOverride = caster.UID;
	statusForceCenterSet = true;
	statusForceCenter = caster.pos;
	{
		bool known = false;
		foreach_const(s, target.npcStatuses) if(*s == statusID) known = true;
		if(!known) target.npcStatuses.push_back(statusID);
	}
	ApplyStatus(target, target, statusID, 0, durationOverride);
	statusCasterOverride = prevCaster;
	statusForceCenterSet = prevCenterSet;
	statusForceCenter = prevCenter;
}

void World::DamagePlayerFromNpc(ActorNpc& attacker, Player& target, i32 damage, i32 remote, i32 status, u32 remoteID)
{
	if(frozen || target.dead || target.untargetable) return;
	if(HasStatusType(target, StatusType::GODMODE) || HasStatusType(target, StatusType::BURROW)) return;
	if(HasStatusType(target, StatusType::EVADE)) {
		LOG("Evade :: %ls evaded creature=%d", target.name.data(), (i32)attacker.docID);
		replication->SendEvadeToAll(attacker.UID, target.Main().UID, remoteID, RemoteIdx(remote), ClientHandle::INVALID);
		OnEvade(target);
		return;
	}
	if(damage <= 0) {
		if(status > 0) ApplyStatusFromNpc(attacker, target, status);
		return;
	}
	MarkInBattle(target);
	{
		bool known = false;
		foreach_const(d, target.hitByDocs) if(*d == (i32)attacker.docID) known = true;
		if(!known) target.hitByDocs.push_back((i32)attacker.docID);
	}
	const f32 def = (MasterStat(target.Main().classType, STAT_DEFENCE) + StatBonus(target, STAT_DEFENCE)) * MAX(1.0f + StatusModMulSum(target, StatusMod::DEFENCE) / 100.0f, 0.0f);
	damage = MAX((i32)(damage * DEFENCE_SCALE / (DEFENCE_SCALE + MAX(def, 0.0f)) * MAX(1.0f + StatusModMulSum(target, StatusMod::DAMAGE_TAKEN) / 100.0f, 0.0f) + 0.5f), 1);
	bool shieldHit = false;
	foreach(it, activeStatuses) {
		if(damage <= 0) break;
		if(it->targetIndex != target.index || it->shieldHP <= 0) continue;
		const f32 absorbed = MIN(it->shieldHP, (f32)damage);
		it->shieldHP -= absorbed;
		it->absorbed += absorbed;
		damage -= (i32)absorbed;
		shieldHit = true;
	}
	if(shieldHit) SendShieldStat(target);
	if(damage <= 0) {
		if(status > 0) ApplyStatusFromNpc(attacker, target, status);
		return;
	}
	GainRageOnDamage(target, damage);
	ActorMaster& chara = target.Main();
	target.damageTaken += damage;
	target.hitsTaken++;
	target.lastDamagedAt = localTime;
	chara.hp = MAX(chara.hp - damage, (HasStatusType(target, StatusType::NEARLY_IMMORTAL) || HasStatusType(target, StatusType::UNDEAD)) ? 1 : 0);
	if(ShowcaseKeepsAlive(target)) chara.hp = MAX(chara.hp, 1);
	if(chara.hp == 0) PassiveOnLethal(target);
	if(target.casting.skillID != SkillID::INVALID && target.casting.interruptByAttacked) CancelCast(target, "attacked");
	if((attacker.faction == Faction::RED || attacker.faction == Faction::BLUE) && (i32)attacker.faction != (i32)target.team) LightAttacker(attacker.pos, target.team);
	SendNpcHit(attacker, chara.UID, target.body->GetWorldPos(), remote, remoteID, damage, chara.hp, chara.hpMax);
	if(chara.hp > 0) {
		LOG("Damage :: creature %d -> %ls damage=%d hp=%d/%d", (i32)attacker.docID, target.name.data(), damage, chara.hp, chara.hpMax);
		PassiveOnDamaged(target);
		if(status > 0) ApplyStatusFromNpc(attacker, target, status);
		CheckSupportKit(target);
		return;
	}
	target.dead = true;
	target.deaths++;
	Player* credited = nullptr;
	Time creditedAt = Time::ZERO;
	foreach_const(d, target.recentDamage) {
		if(d->attackerIndex >= players.size() || players[d->attackerIndex].team == target.team) continue;
		if(localTime > TimeAdd(d->at, TimeMsToTime(MEASURED_MONSTER_KILL_CREDIT_MS)) || d->at < creditedAt) continue;
		credited = &players[d->attackerIndex];
		creditedAt = d->at;
	}
	if(!map || !map->ingameLevels) GrantSkillPoint(target);
	DropTitanPieces(target);
	DropMasterSoul(target);
	RewardMasterKill(credited, target);
	RevertTitan(target);
	target.occupyStreak = 0;
	i32 teamKills = 0;
	i32 victimTeamKills = 0;
	foreach_const(it, players) {
		if(it->team == target.team) victimTeamKills += it->kills;
		else teamKills += it->kills;
	}
	const i32 respawnMs = fixedRespawnMs > 0 ? fixedRespawnMs : map && map->ingameLevels ? IngameRespawnDelayMs(target) : gameStartedAt == Time::ZERO ? RESPAWN_BASE_MS[ARRAY_COUNT(RESPAWN_BASE_MS) - 1] : RespawnDelayMs(TimeDiffSec(TimeDiff(gameStartedAt, localTime)), victimTeamKills - teamKills);
	target.tRespawn = TimeAdd(localTime, TimeMsToTime(noRespawn ? NO_RESPAWN_MS : (f64)respawnMs));
	target.killStreak = 0;
	target.recentDamage.clear();
	LOG("Death :: %ls killed by creature %d (respawn %d ms)", target.name.data(), (i32)attacker.docID, respawnMs);
	SyncPassivesOnDeath(target);
	replication->SendDeathToAll(attacker.UID, chara.UID, attacker.docID, 0, (RemoteIdx)remote, target.userID, respawnMs);
	if(replication->titanMode && credited) {
		credited->kills++;
		replication->SendPvpAnnouncementToAll(PVP_ANNOUNCE_MONSTER_KILL_CREDITED, 0, credited->Main().UID, chara.UID, KillPoints());
		AddPoints(*credited, (i32)KillPoints());
		LOG("Death :: %ls kill credited to %ls", target.name.data(), credited->name.data());
	}
	else if(replication->titanMode) replication->SendPvpAnnouncementToAll(PVP_ANNOUNCE_KILLED_BY_MONSTER, 0, attacker.UID, chara.UID, KillPoints());
}
