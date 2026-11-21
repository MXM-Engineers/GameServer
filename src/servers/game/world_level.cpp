#include "config.h"
#include "world.h"
#include "world_common.h"
#include <mxm/game_content.h>
#include <EASTL/sort.h>

static const u32 PVP_ANNOUNCE_MONSTER_KILL = 5;
static const u8 LEVEL_STAT_HEALTH = 0;
static const f64 TRAIT_RESET_COOLDOWN_MS = 20000.0;
static const u8 LEVEL_STAT_ATTACK = 6;
static const u8 LEVEL_STAT_SKILLPOWER = 8;

static bool KillRewarded(const World::ActorNpc& npc, const World::Player& killer)
{
	return npc.faction != (Faction)killer.team;
}

void World::StartIngameLevels()
{
	if(!map || !map->ingameLevels) return;
	const GameXmlContent::IngameLevelInfo& info = GetGameXmlContent().ingameLevel;
	foreach(p, players) {
		p->ingameLevel = 1;
		p->ingameExp = 0;
		p->Main().traits = {};
		p->Main().traitPoints = 0;
		p->Sub().traits = {};
		p->Sub().traitPoints = 0;
		replication->SendLevelIngameToAll(p->userID, info.ExpToLevelUp(1), 1);
	}
	if(Config().DevStartLevel > 1) {
		foreach(p, players) {
			while(p->ingameLevel < MIN(Config().DevStartLevel, info.maxLevel)) GrantIngameExp(*p, MAX(info.ExpToLevelUp(p->ingameLevel) - p->ingameExp, 1), ActorUID::INVALID);
		}
	}
	nextPassiveExpAt = info.passivePeriod > 0 ? TimeAdd(localTime, TimeMsToTime(info.passivePeriod * 1000.0)) : Time::ZERO;
}

void World::GrantIngameExp(Player& player, i32 gain, ActorUID target)
{
	if(!map || !map->ingameLevels || gain <= 0) return;
	const GameXmlContent::IngameLevelInfo& info = GetGameXmlContent().ingameLevel;
	if(player.ingameLevel >= info.maxLevel) return;
	player.ingameExp += gain;
	bool levelUp = false;
	while(player.ingameLevel < info.maxLevel && player.ingameExp >= info.ExpToLevelUp(player.ingameLevel)) {
		player.ingameExp -= info.ExpToLevelUp(player.ingameLevel);
		player.ingameLevel++;
		levelUp = true;
		GrantSkillPoint(player);
		if(info.propertyPointsEvery > 0 && player.ingameLevel % info.propertyPointsEvery == 0) {
			player.Main().traitPoints += info.propertyPointsStep;
			player.Sub().traitPoints += info.propertyPointsStep;
		}
	}
	if(player.ingameLevel >= info.maxLevel) player.ingameExp = 0;
	if(player.clientHd != ClientHandle::INVALID) replication->SendExpIngame(player.clientHd, player.userID, gain, player.ingameExp, target);
	if(!levelUp) return;
	replication->SendLevelIngameToAll(player.userID, info.ExpToLevelUp(player.ingameLevel), player.ingameLevel);
	ApplyLevelStats(player);
	if(player.clientHd != ClientHandle::INVALID) {
		if(player.Main().traitPoints > 0) replication->SendCharacterPropertyPoint(player.clientHd, player.Main().UID, player.Main().traitPoints);
		if(player.Sub().traitPoints > 0) replication->SendCharacterPropertyPoint(player.clientHd, player.Sub().UID, player.Sub().traitPoints);
	}
	LOG("Level :: %ls reaches level %d", player.name.data(), player.ingameLevel);
}

void World::GrantMonsterKillExp(const ActorNpc& npc, ActorUID killerUID)
{
	if(!map || !map->ingameLevels) return;
	const GameXmlContent::IngameLevelInfo& info = GetGameXmlContent().ingameLevel;
	auto found = info.monsters.find((i32)npc.docID);
	if(found == info.monsters.end()) return;
	const GameXmlContent::IngameLevelInfo::MonsterExp& rule = found->second;
	ActorMaster* killerMaster = FindMasterActor(killerUID);
	Player* killer = killerMaster ? killerMaster->parent : nullptr;
	if(killer && !KillRewarded(npc, *killer)) return;
	i32 team = killer ? (i32)killer->team : (npc.faction == Faction::RED ? 1 : (npc.faction == Faction::BLUE ? 0 : -1));
	if(team < 0) return;
	i32 base = rule.exp;
	if(rule.growth > 0 && rule.growthPeriod > 0 && gameStartedAt != Time::ZERO) {
		base += rule.growth * (i32)(TimeDiffSec(TimeDiff(gameStartedAt, localTime)) / rule.growthPeriod);
	}
	switch(rule.mode) {
		case GameXmlContent::IngameLevelInfo::MonsterExp::KILLER: {
			if(killer) GrantIngameExp(*killer, base, npc.UID);
		} break;
		case GameXmlContent::IngameLevelInfo::MonsterExp::TEAM: {
			foreach(p, players) if((i32)p->team == team) GrantIngameExp(*p, base + (&*p == killer ? rule.killerBonus : 0), npc.UID);
		} break;
		default: {
			eastl::fixed_vector<Player*,10,false> sharers;
			foreach(p, players) {
				if((i32)p->team != team || p->dead || !p->body) continue;
				if(&*p == killer || glm::length(vec2(p->body->GetWorldPos()) - vec2(npc.pos)) <= info.shareRadius) sharers.push_back(&*p);
			}
			if(sharers.empty()) return;
			const i32 each = MAX((base - rule.killerBonus) / (i32)sharers.size(), 0);
			foreach(p, sharers) GrantIngameExp(**p, each + (*p == killer ? rule.killerBonus : 0), npc.UID);
		} break;
	}
}

void World::RewardMasterKill(Player* killer, const Player& victim)
{
	if(!map || !map->ingameLevels) return;
	const f32 window = GetGameXmlContent().ingameLevel.masterKillAssistWindowSec;
	Player* credited = killer && killer->team != victim.team ? killer : nullptr;
	Time latest = Time::ZERO;
	if(!credited) {
		foreach_const(d, victim.recentDamage) {
			Player& a = players[d->attackerIndex];
			if(a.team == victim.team || d->damage <= 0 || TimeDiffSec(TimeDiff(d->at, localTime)) > window) continue;
			if(!credited || d->at > latest) { credited = &a; latest = d->at; }
		}
	}
	if(!credited) return;
	eastl::fixed_vector<Player*,10,false> assisters;
	foreach_const(d, victim.recentDamage) {
		Player& a = players[d->attackerIndex];
		if(&a == credited || a.team == victim.team || d->damage <= 0 || TimeDiffSec(TimeDiff(d->at, localTime)) > window) continue;
		if(eastl::find(assisters.begin(), assisters.end(), &a) == assisters.end() && !assisters.full()) assisters.push_back(&a);
	}
	GrantMasterKillExp(*credited, victim, assisters);
}

void World::GrantMasterKillExp(Player& killer, const Player& victim, const eastl::fixed_vector<Player*,10,false>& assisters)
{
	if(!map || !map->ingameLevels || killer.team == victim.team) return;
	const GameXmlContent::IngameLevelInfo& info = GetGameXmlContent().ingameLevel;
	if(info.masterKillBase <= 0) return;
	const i32 reward = (i32)(info.masterKillBase + info.masterKillPerLevel * (f32)victim.ingameLevel + 0.5f);
	GrantIngameExp(killer, reward, victim.Main().UID);
	if(assisters.empty()) return;
	const i32 each = MAX((i32)(reward * info.masterKillAssistPool / (f32)assisters.size()), 1);
	foreach_const(a, assisters) GrantIngameExp(**a, each, victim.Main().UID);
}

i32 World::IngameRespawnDelayMs(const Player& player) const
{
	const GameXmlContent::IngameLevelInfo& info = GetGameXmlContent().ingameLevel;
	const f64 elapsed = gameStartedAt != Time::ZERO ? TimeDiffSec(TimeDiff(gameStartedAt, localTime)) : 0.0;
	const f64 ms = info.respawnBaseMs + info.respawnPerLevelMs * player.ingameLevel + info.respawnPerSecMs * elapsed;
	return (i32)(floor(ms / 1000.0 + 0.5) * 1000.0);
}

f32 World::LevelBonus(const Player& player, ClassType classType, u8 statType) const
{
	if(!map || !map->ingameLevels) return 0;
	return GetGameXmlContent().MasterLevelBonus(classType, statType, player.ingameLevel);
}

void World::ApplyLevelStats(Player& player)
{
	for(i32 i = 0; i < 2; i++) {
		ActorMaster& chara = i == 0 ? player.Main() : player.Sub();
		if(i == 0 && player.titanForm >= 0) continue;
		RefreshMasterHealth(player, chara);
		const WeaponIndex weapon = (chara.UID == player.Main().UID) == ((player.mainCharaID & 1) == 0) ? player.mainWeapon : player.subWeapon;
		const auto& content = GetGameXmlContent();
		replication->SendStatToAll(chara.UID, LEVEL_STAT_ATTACK, 0, content.GetWeaponAttack((i32)weapon) + LevelBonus(player, chara.classType, LEVEL_STAT_ATTACK) + content.MasterTraitBonus(chara.classType, chara.traits, LEVEL_STAT_ATTACK) + chara.GearBonus(LEVEL_STAT_ATTACK));
		replication->SendStatToAll(chara.UID, LEVEL_STAT_SKILLPOWER, 0, MasterStat(chara.classType, LEVEL_STAT_SKILLPOWER) + LevelBonus(player, chara.classType, LEVEL_STAT_SKILLPOWER) + content.MasterTraitBonus(chara.classType, chara.traits, LEVEL_STAT_SKILLPOWER) + chara.GearBonus(LEVEL_STAT_SKILLPOWER));
	}
}

void World::RefreshMasterHealth(Player& player, ActorMaster& chara)
{
	if(player.titanForm >= 0 && chara.UID == player.Main().UID) return;
	const f32 groupingMul = chara.UID == player.Main().UID ? 1.0f + GroupingStatMod(player, StatusMod::HEALTH, true) / 100.0f : 1.0f;
	const i32 hpMax = (i32)((MasterStat(chara.classType, LEVEL_STAT_HEALTH) + LevelBonus(player, chara.classType, LEVEL_STAT_HEALTH) + GetGameXmlContent().MasterTraitBonus(chara.classType, chara.traits, LEVEL_STAT_HEALTH) + chara.GearBonus(LEVEL_STAT_HEALTH)) * groupingMul);
	if(chara.hp > 0) chara.hp = MIN(chara.hp + MAX(hpMax - chara.hpMax, 0), hpMax);
	chara.hpMax = hpMax;
	replication->SendStatToAll(chara.UID, LEVEL_STAT_HEALTH, (f32)chara.hp, (f32)chara.hpMax);
}

f32 World::CombatStatBase(const Player& player, const ActorMaster& chara, u8 statType) const
{
	const auto& content = GetGameXmlContent();
	const WeaponIndex weapon = (chara.UID == player.Main().UID) == ((player.mainCharaID & 1) == 0) ? player.mainWeapon : player.subWeapon;
	const f32 base = statType == LEVEL_STAT_ATTACK ? content.GetWeaponAttack((i32)weapon) : MasterStat(chara.classType, statType);
	return base + LevelBonus(player, chara.classType, statType) + content.MasterTraitBonus(chara.classType, chara.traits, statType) + chara.GearBonus(statType);
}

f32 World::StatBonus(const Player& player, u8 statType) const
{
	const ActorMaster& chara = player.Main();
	return LevelBonus(player, chara.classType, statType) + GetGameXmlContent().MasterTraitBonus(chara.classType, chara.traits, statType) + chara.GearBonus(statType);
}

void World::SendTraitStats(Player& player, ActorMaster& chara)
{
	const GameXmlContent& content = GetGameXmlContent();
	auto f = content.masterTraits.find((i32)chara.classType);
	if(f == content.masterTraits.end()) return;
	for(i32 i = 0; i < 4; i++) {
		for(const StatValue& st : f->second[i]) {
			if(st.type == LEVEL_STAT_HEALTH) {
				RefreshMasterHealth(player, chara);
				continue;
			}
			const WeaponIndex weapon = (chara.UID == player.Main().UID) == ((player.mainCharaID & 1) == 0) ? player.mainWeapon : player.subWeapon;
			const f32 base = st.type == LEVEL_STAT_ATTACK ? content.GetWeaponAttack((i32)weapon) : MasterStat(chara.classType, st.type);
			const f32 value = base + LevelBonus(player, chara.classType, st.type) + content.MasterTraitBonus(chara.classType, chara.traits, st.type) + chara.GearBonus(st.type);
			replication->SendStatToAll(chara.UID, st.type, 0, value);
		}
	}
}

u32 World::UpgradeTraits(Player& player, ActorUID characterUID, const u8* indices, i32 count)
{
	static const u32 ERR_NOT_USING_MODE = 600;
	static const u32 ERR_NOT_EXIST_CHARACTER = 603;
	static const u32 ERR_NOT_ENOUGH_POINT = 604;
	static const u32 ERR_NOT_EXIST_DATA = 605;
	static const u32 ERR_COUNT_LIMIT = 606;
	const GameXmlContent& content = GetGameXmlContent();
	if(!map || !map->ingameLevels) return ERR_NOT_USING_MODE;
	static const u32 ERR_NOT_IN_SAFE_ZONE = 601;
	ActorMaster* chara = player.Main().UID == characterUID ? &player.Main() : player.Sub().UID == characterUID ? &player.Sub() : nullptr;
	if(!chara) return ERR_NOT_EXIST_CHARACTER;
	if(!player.inSafeZone) return ERR_NOT_IN_SAFE_ZONE;
	if(count <= 0 || count > chara->traitPoints) return ERR_NOT_ENOUGH_POINT;
	eastl::array<u8,4> levels = chara->traits;
	for(i32 i = 0; i < count; i++) {
		if(indices[i] >= 4) return ERR_NOT_EXIST_DATA;
		if(levels[indices[i]] >= content.traitMaxPoints) return ERR_COUNT_LIMIT;
		levels[indices[i]]++;
	}
	chara->traits = levels;
	chara->traitPoints -= count;
	SendTraitStats(player, *chara);
	if(player.clientHd != ClientHandle::INVALID) replication->SendCharacterPropertyPoint(player.clientHd, chara->UID, chara->traitPoints);
	replication->SendCharacterPropertyInfoToAll(chara->UID, chara->traits);
	LOG("Traits :: %ls master %d traits %d/%d/%d/%d (%d points left)", player.name.data(), (i32)chara->classType, chara->traits[0], chara->traits[1], chara->traits[2], chara->traits[3], chara->traitPoints);
	return 0;
}

void World::BotSpendTraits(Player& player)
{
	const i32 maxPoints = GetGameXmlContent().traitMaxPoints;
	for(i32 m = 0; m < 2; m++) {
		ActorMaster& chara = m == 0 ? player.Main() : player.Sub();
		eastl::fixed_vector<u8,64,false> picks;
		eastl::array<u8,4> levels = chara.traits;
		if(!chara.botTraitPlanSet) {
			chara.botTraitPlanSet = true;
			const GameXmlContent::MeasuredBotTraits* plan = GetGameXmlContent().PickMeasuredBotTraits(chara.classType, RandUint());
			if(plan) chara.botTraitPlan = plan->levels;
		}
		for(i32 k = 0; k < chara.traitPoints && !picks.full(); k++) {
			i32 idx = -1;
			for(i32 i = 0; i < 4; i++) {
				if(levels[i] >= maxPoints || levels[i] >= chara.botTraitPlan[i]) continue;
				if(idx < 0 || chara.botTraitPlan[i] > chara.botTraitPlan[idx]) idx = i;
			}
			if(idx < 0) {
				idx = 0;
				while(idx < 4 && levels[idx] >= maxPoints) idx++;
			}
			if(idx >= 4) break;
			levels[idx]++;
			picks.push_back((u8)idx);
		}
		if(!picks.empty()) UpgradeTraits(player, chara.UID, picks.data(), (i32)picks.size());
	}
}

u32 World::ResetTraits(Player& player, ActorUID characterUID)
{
	static const u32 ERR_NOT_USING_MODE = 600;
	static const u32 ERR_NOT_EXIST_CHARACTER = 603;
	if(!map || !map->ingameLevels) return ERR_NOT_USING_MODE;
	ActorMaster* chara = player.Main().UID == characterUID ? &player.Main() : player.Sub().UID == characterUID ? &player.Sub() : nullptr;
	static const u32 ERR_RESET_COOLTIME = 602;
	if(!chara) return ERR_NOT_EXIST_CHARACTER;
	if(chara->traitResetAt != Time::ZERO && localTime < TimeAdd(chara->traitResetAt, TimeMsToTime(TRAIT_RESET_COOLDOWN_MS))) return ERR_RESET_COOLTIME;
	chara->traitResetAt = localTime;
	for(u8 v : chara->traits) chara->traitPoints += v;
	chara->traits = {};
	SendTraitStats(player, *chara);
	replication->SendCharacterPropertyResetToAll(chara->UID);
	if(player.clientHd != ClientHandle::INVALID) replication->SendCharacterPropertyPoint(player.clientHd, chara->UID, chara->traitPoints);
	LOG("Traits :: %ls master %d reset (%d points)", player.name.data(), (i32)chara->classType, chara->traitPoints);
	return 0;
}

static const u32 VOTE_TYPE_SURRENDER = 0;
static const u32 VOTE_TYPE_TITAN = 1;
static const i32 TITAN_READY_STATUS = 1239000002;
static const f64 VOTE_SURRENDER_PERIOD_MS = 30000.0;
static const f64 VOTE_SURRENDER_FAIL_COOLDOWN_MS = 90000.0;
static const u32 VOTE_STATE_PASSED = 1;
static const u32 VOTE_STATE_FAILED = 2;
static const u32 VOTE_REASON_PASSED = 1;
static const u32 VOTE_REASON_REJECTED = 2;
static const i8 VOTE_YES = 0;
static const i8 VOTE_NONE = -1;

void World::RequestVote(Player& player, u32 type)
{
	if(winnerTeam >= 0 || gameStartedAt == Time::ZERO) return;
	const GameXmlContent::TitanInfo& titan = GetGameXmlContent().titan;
	f32 periodMs = 0;
	if(type == VOTE_TYPE_SURRENDER) {
		if(TimeDiffMs(TimeDiff(gameStartedAt, localTime)) < (f64)replication->surrenderAbleTime) return;
		periodMs = (f32)VOTE_SURRENDER_PERIOD_MS;
	}
	else if(type == VOTE_TYPE_TITAN) {
		if(titan.forms.empty() || player.dead || player.titanForm >= 0 || teamTitanPieces[player.team] < titan.cost || localTime < player.titanVoteBlockedUntil) return;
		periodMs = titan.votePeriod;
	}
	else return;
	TeamVote& vote = teamVotes[player.team];
	if(vote.active || localTime < vote.cooldownUntil) return;
	vote.active = true;
	vote.type = type;
	vote.periodMs = periodMs;
	vote.requestor = (i32)player.index;
	vote.endAt = TimeAdd(localTime, TimeMsToTime(periodMs));
	vote.choices.fill(VOTE_NONE);
	i32 voters = 0;
	foreach(p, players) {
		if(p->team != player.team || (type == VOTE_TYPE_TITAN && p->index == player.index)) continue;
		voters++;
	}
	foreach(p, players) {
		if(p->team == player.team && (p->index == player.index || p->clientHd == ClientHandle::INVALID)) vote.choices[p->index] = VOTE_YES;
	}
	foreach(p, players) {
		if(p->team == player.team && p->clientHd != ClientHandle::INVALID) replication->SendRequestVoting(p->clientHd, player.userID, type, (u32)periodMs, voters, false);
	}
	foreach(voter, players) {
		if(voter->team != player.team || vote.choices[voter->index] != VOTE_YES) continue;
		if(type == VOTE_TYPE_TITAN && voter->index == player.index) continue;
		foreach(p, players) {
			if(p->team == player.team && p->clientHd != ClientHandle::INVALID) replication->SendVote(p->clientHd, type, (u32)VOTE_YES);
		}
	}
	LOG("Vote :: %ls starts vote %d for team %d", player.name.data(), type, (i32)player.team);
	if(type == VOTE_TYPE_TITAN) ApplyStatus(player, player, TITAN_READY_STATUS, 0, periodMs / 1000.0f);
	UpdateVotes();
}

void World::CastVote(Player& player, u32 choice)
{
	TeamVote& vote = teamVotes[player.team];
	if(!vote.active || vote.choices[player.index] != VOTE_NONE) return;
	vote.choices[player.index] = (i8)choice;
	foreach(p, players) {
		if(p->team == player.team && p->clientHd != ClientHandle::INVALID) replication->SendVote(p->clientHd, vote.type, choice);
	}
	UpdateVotes();
}

void World::FinishVote(i32 team, bool passed)
{
	TeamVote& vote = teamVotes[team];
	vote.active = false;
	const UserID requestor = vote.requestor >= 0 ? players[vote.requestor].userID : UserID(0);
	u32 reason = passed ? VOTE_REASON_PASSED : VOTE_REASON_REJECTED;
	u32 playerCool = 0;
	u32 teamCool = 0;
	if(vote.type == VOTE_TYPE_TITAN) {
		reason = VOTE_REASON_PASSED;
		if(passed) teamCool = (u32)GetGameXmlContent().titan.teamCoolTime;
		else playerCool = (u32)GetGameXmlContent().titan.failPlayerCool;
	}
	else if(!passed) teamCool = (u32)VOTE_SURRENDER_FAIL_COOLDOWN_MS;
	if(teamCool > 0) vote.cooldownUntil = TimeAdd(localTime, TimeMsToTime((f64)teamCool));
	if(playerCool > 0 && vote.requestor >= 0) players[vote.requestor].titanVoteBlockedUntil = TimeAdd(localTime, TimeMsToTime((f64)playerCool));
	foreach(p, players) {
		if(p->team == team && p->clientHd != ClientHandle::INVALID) replication->SendVotingResult(p->clientHd, requestor, vote.type, passed ? VOTE_STATE_PASSED : VOTE_STATE_FAILED, reason, playerCool, teamCool);
	}
	LOG("Vote :: team %d vote %d %s", team, vote.type, passed ? "passed" : "rejected");
	if(vote.type == VOTE_TYPE_TITAN && vote.requestor >= 0) ConsumeStatus((u32)vote.requestor, TITAN_READY_STATUS);
	if(!passed) return;
	if(vote.type == VOTE_TYPE_SURRENDER && winnerTeam < 0) winnerTeam = team == 0 ? 1 : 0;
	if(vote.type == VOTE_TYPE_TITAN && vote.requestor >= 0) TransformTitan(players[vote.requestor], 0, true);
}

void World::UpdateVotes()
{
	for(i32 team = 0; team < 2; team++) {
		TeamVote& vote = teamVotes[team];
		if(!vote.active) continue;
		i32 voters = 0, yes = 0, no = 0;
		foreach_const(p, players) {
			if(p->team != team) continue;
			voters++;
			if(vote.choices[p->index] == VOTE_YES) yes++;
			else if(vote.choices[p->index] != VOTE_NONE) no++;
		}
		const i32 needed = vote.type == VOTE_TYPE_TITAN ? voters / 2 + 1 : voters <= 3 ? voters : voters - 1;
		if(yes >= needed) FinishVote(team, true);
		else if(voters - no < needed || localTime >= vote.endAt) FinishVote(team, false);
	}
}

u32 World::KillPoints() const
{
	return map && map->ingameLevels ? (u32)GetGameXmlContent().ingameLevel.killPoints : 1;
}

static const i32 GIANT_POINTS_STEP = 100;

static const i32 GIANT_KILL_POINTS_STEP = 3;

i32 World::TeamPoints(i32 team) const
{
	i32 total = team >= 0 && team < 2 ? teamBonusPoints[team] : 0;
	foreach_const(p, players) if((i32)p->team == team) total += p->points;
	return total;
}

void World::OnTeamPointsChanged(i32 team, i32 before, i32 after)
{
	for(i32 step = before / GIANT_POINTS_STEP + 1; step <= after / GIANT_POINTS_STEP; step++) SummonGiant(team);
}

void World::AddPoints(Player& player, i32 points)
{
	if(!map || !map->ingameLevels || points <= 0) return;
	player.points += points;
	const i32 teamPoints = TeamPoints(player.team);
	replication->SendPointsToAll(player.userID, player.team, player.points, teamPoints);
	OnTeamPointsChanged(player.team, teamPoints - points, teamPoints);
}

void World::AddTeamPoints(i32 team, i32 points)
{
	if(!map || !map->ingameLevels || points <= 0 || team < 0 || team > 1) return;
	teamBonusPoints[team] += points;
	const i32 teamPoints = TeamPoints(team);
	replication->SendTeamPointsToAll((u8)team, teamPoints);
	OnTeamPointsChanged(team, teamPoints - points, teamPoints);
}

bool World::IsStructure(const ActorNpc& npc) const
{
	static const u32 towerCenterTag = StatusTypeKey("Tower_Center");
	static const u32 towerSideTag = StatusTypeKey("Tower_Side");
	static const u32 coreTag = StatusTypeKey("Core");
	return npc.monsterAi && (npc.monsterAi->tagKey == towerCenterTag || npc.monsterAi->tagKey == towerSideTag || npc.monsterAi->tagKey == coreTag);
}

void World::GrantMonsterKillPoints(const ActorNpc& npc, ActorUID killerUID)
{
	if(!map || !map->ingameLevels) return;
	static const u32 minionTag = StatusTypeKey("Minion");
	static const u32 towerCenterTag = StatusTypeKey("Tower_Center");
	static const u32 towerSideTag = StatusTypeKey("Tower_Side");
	static const u32 chiefTag = StatusTypeKey("Chief");
	if(ActorMaster* master = FindMasterActor(killerUID)) {
		if(master->parent && npc.monsterAi) {
			const u32 tag = npc.monsterAi->tagKey;
			i32 kind = -1;
			if(tag == minionTag) kind = END_KILL_MINION;
			else if(tag == towerCenterTag || tag == towerSideTag) kind = END_KILL_TOWER;
			else if(tag == chiefTag || npc.faction == Faction::ORANGE) kind = END_KILL_BOSS;
			else if(npc.faction == Faction::MONSTER) kind = END_KILL_NEUTRAL;
			if(kind >= 0 && master->parent->monsterKills[kind] < 255) master->parent->monsterKills[kind]++;
			if(kind == END_KILL_MINION || kind == END_KILL_NEUTRAL || kind == END_KILL_BOSS) replication->SendMonsterKillCountsToAll(master->parent->userID, master->parent->monsterKills[END_KILL_MINION], master->parent->monsterKills[END_KILL_NEUTRAL] + master->parent->monsterKills[END_KILL_BOSS]);
		}
	}
	const auto& giants = GetGameXmlContent().titan.giants;
	if(eastl::find(giants.begin(), giants.end(), (i32)npc.docID) != giants.end() && (npc.faction == Faction::RED || npc.faction == Faction::BLUE)) {
		const i32 giantTeam = (i32)npc.faction;
		const i32 points = GIANT_KILL_POINTS_STEP * ++giantsKilled[giantTeam];
		replication->SendPvpAnnouncementToAll(PVP_ANNOUNCE_MONSTER_KILL, 0, killerUID, npc.UID, (u32)points);
		ActorMaster* giantKiller = FindMasterActor(killerUID);
		if(giantKiller && giantKiller->parent && (i32)giantKiller->parent->team != giantTeam) AddPoints(*giantKiller->parent, points);
		else AddTeamPoints(1 - giantTeam, points);
		LOG("Titan :: giant of team %d killed (%d), +%d points to team %d", giantTeam, giantsKilled[giantTeam], points, 1 - giantTeam);
		return;
	}
	const GameXmlContent::IngameLevelInfo& info = GetGameXmlContent().ingameLevel;
	auto found = info.monsters.find((i32)npc.docID);
	ActorMaster* killer = FindMasterActor(killerUID);
	if(found == info.monsters.end() || found->second.points <= 0 || !killer || !killer->parent || !KillRewarded(npc, *killer->parent)) return;
	replication->SendPvpAnnouncementToAll(PVP_ANNOUNCE_MONSTER_KILL, 0, killerUID, npc.UID, (u32)found->second.points);
	AddPoints(*killer->parent, found->second.points);
}

void World::BuildGiantTriggers()
{
	giantTriggersBuilt = true;
	const auto& content = GetGameXmlContent();
	foreach_const(n, map->levelEvents) {
		if(n->kind != GameXmlContent::LevelEventNode::AI_TRIGGER) continue;
		eastl::fixed_vector<i32,16,false> open;
		foreach_const(o, n->outputs) open.push_back(*o);
		for(i32 k = 0; k < (i32)open.size() && k < 16; k++) {
			const GameXmlContent::LevelEventNode* c = map->FindLevelEvent(open[k]);
			if(!c) continue;
			if(c->kind == GameXmlContent::LevelEventNode::CALL_MONSTER) {
				if((c->team == Faction::RED || c->team == Faction::BLUE) && content.FindMonsterDieSpawn(c->monster) && !giantTriggers.full()) giantTriggers.push_back({ n->aiIndex, (i32)c->team, c->monster, c->tagID });
				break;
			}
			if(c->kind != GameXmlContent::LevelEventNode::DELAY) continue;
			foreach_const(o, c->outputs) if(!open.full()) open.push_back(*o);
		}
	}
	eastl::sort(giantTriggers.begin(), giantTriggers.end(), [](const GiantTrigger& a, const GiantTrigger& b) { return a.monster < b.monster; });
	foreach_const(n, map->levelEvents) {
		if(n->kind != GameXmlContent::LevelEventNode::AI_TRIGGER) continue;
		bool giant = false;
		foreach_const(t, giantTriggers) if(t->aiIndex == n->aiIndex) giant = true;
		if(giant) continue;
		eastl::fixed_vector<i32,16,false> open;
		foreach_const(o, n->outputs) open.push_back(*o);
		for(i32 k = 0; k < (i32)open.size(); k++) {
			const GameXmlContent::LevelEventNode* c = map->FindLevelEvent(open[k]);
			if(!c) continue;
			if(c->kind == GameXmlContent::LevelEventNode::STATUS_CHANGE) {
				foreach_const(t, giantTriggers) if(t->tagID >= 0 && t->tagID == c->tagID) firstGiantTrigger[t->team] = n->aiIndex;
			}
			if(c->kind == GameXmlContent::LevelEventNode::DELAY) foreach_const(o, c->outputs) if(!open.full()) open.push_back(*o);
		}
	}
	LOG("Titan :: %d giant triggers, first giant triggers %d/%d", (i32)giantTriggers.size(), firstGiantTrigger[0], firstGiantTrigger[1]);
}

void World::SummonGiant(i32 team)
{
	if(!map || team < 0 || team > 1) return;
	if(!giantTriggersBuilt) BuildGiantTriggers();
	const auto& order = GetGameXmlContent().titan.giants;
	i32 variants = 0;
	foreach_const(t, giantTriggers) if(t->team == team) variants++;
	if(variants == 0) return;
	const u32 pick = giantCycle++;
	i32 rank = 0;
	foreach_const(t, giantTriggers) {
		if(t->team != team) continue;
		const bool chosen = order.empty() ? rank == (i32)(pick % (u32)variants) : t->monster == order[pick % (u32)order.size()];
		rank++;
		if(!chosen) continue;
		LOG("Titan :: giant %d for team %d", t->monster, team);
		const bool first = giantSummons[0].empty() && giantSummons[1].empty();
		if(!giantSummons[team].full()) giantSummons[team].push_back(t->monster);
		FireAITrigger(t->aiIndex, -1);
		if(first && firstGiantTrigger[team] >= 0) FireAITrigger(firstGiantTrigger[team], -1);
		return;
	}
}

static const i32 TITAN_SPAWN_STATUS = 1239000040;
static const i32 TITAN_SPAWN_SELF_STATUS = 1239000041;
static const i32 TITAN_SPAWN_REVEAL_STATUS = 1239000004;
static const i32 AI_TRIGGER_TRANSFORM_TITAN = 300;
static const f64 TITAN_MODE_SWITCH_DELAY_MS = 620.0;

void World::TransformTitan(Player& player, i32 formIndex, bool initial)
{
	const GameXmlContent::TitanInfo& titan = GetGameXmlContent().titan;
	if(formIndex < 0 || formIndex >= (i32)titan.forms.size()) return;
	const GameXmlContent::TitanForm& form = titan.forms[formIndex];
	ActorMaster& chara = player.Main();
	if(player.casting.skillID != SkillID::INVALID) CancelCast(player, "titan form");
	player.botComboSkill = SkillID::INVALID;
	if(initial) {
		AddTeamPieces((i32)player.team, -titan.cost);
		player.titanSavedHp = chara.hp;
		if(player.clientHd == ClientHandle::INVALID) player.botSkillReady.fill(Time::ZERO);
		player.titanUntil = TimeAdd(localTime, TimeMsToTime((f64)form.duration));
		chara.hpMax = (i32)form.Stat((u8)StatType::STAT_TYPE_HEALTH);
		chara.hp = chara.hpMax;
		if(player.clientHd != ClientHandle::INVALID) replication->SendTransformAnswer(player.clientHd, chara.UID, form.doc);
	}
	player.titanForm = formIndex;
	replication->SendTitanFormToAll(!initial, chara.UID, form.doc, initial ? (u32)form.duration : 0, form.weapon, form.skill1, form.skill2, form.ug, form.stats.data(), (u16)form.stats.size(), (f32)chara.hp, form.Stat((u8)StatType::STAT_TYPE_STAMINA), 0.0f, form.moveSpeed);
	LOG("Titan :: %ls becomes %d (%d hp)", player.name.data(), form.doc, chara.hp);
	if(!initial) return;
	ApplyStatus(player, player, TITAN_SPAWN_STATUS);
	ApplyStatus(player, player, TITAN_SPAWN_SELF_STATUS);
	if(const GameXmlContent::Action* landing = GetGameXmlContent().FindSkillAction(ActionClass(player), ActionStateID::EXTRAMOTION_1_BEHAVIORSTATE, chara.stance)) {
		const vec2 facing = vec2(cosf(player.input.rot.bodyYaw), sinf(player.input.rot.bodyYaw));
		foreach_const(cmd, landing->commands) {
			if(cmd->type != ActionCommand::Type::REMOTE || cmd->remote.idx == RemoteIdx::INVALID || cmd->remote.remove || cmd->delay > 0) continue;
			SpawnRemote(player.index, cmd->remote.idx, player.body->GetWorldPos(), facing, ActorUID::INVALID, SkillID::INVALID, nextCastID++, (i32)WeaponHitDamage(player, nullptr), true);
			LOG("Titan :: %ls landing remote %d", player.name.data(), (i32)cmd->remote.idx);
		}
	}
	const StatusInfo* reveal = GetGameXmlContent().FindStatus(TITAN_SPAWN_REVEAL_STATUS);
	const f64 revealMs = reveal && reveal->durationSec > 0 ? reveal->durationSec * 1000.0 : 0.0;
	foreach(e, players) {
		if(e->team == player.team || e->dead || e->Main().classType == ClassType::NONE) continue;
		if(revealMs > 0 && player.team < 2) {
			const Time until = TimeAdd(localTime, TimeMsToTime(revealMs));
			if(e->fogRevealUntil[player.team] < until) e->fogRevealUntil[player.team] = until;
		}
	}
	FireAITrigger(AI_TRIGGER_TRANSFORM_TITAN, (i32)player.index);
}

void World::RevertTitan(Player& player)
{
	if(player.titanForm < 0) return;
	ActorMaster& chara = player.Main();
	player.titanForm = -1;
	player.titanUntil = Time::ZERO;
	player.titanModeAt = Time::ZERO;
	player.titanKillStreak = 0;
	RefreshMasterHealth(player, chara);
	chara.hp = player.dead ? 0 : MIN(MAX(player.titanSavedHp, 1), chara.hpMax);
	const auto& master = GetGameXmlContent().GetMaster(chara.classType);
	eastl::fixed_vector<StatValue,32,false> stats;
	foreach_const(st, master.baseStats) {
		StatValue v = *st;
		if(v.type == (u8)StatType::STAT_TYPE_HEALTH) v.value = (f32)chara.hpMax;
		if(!stats.full()) stats.push_back(v);
	}
	replication->SendTitanFormToAll(false, chara.UID, 100000000 + (i32)chara.classType, 0, 0, -1, -1, -1, stats.data(), (u16)stats.size(), (f32)chara.hp, MasterStat(chara.classType, (u8)StatType::STAT_TYPE_STAMINA), MAX(player.ultGauge, 0.0f), master.moveSpeed);
	replication->SendStatToAll(chara.UID, (u8)StatType::STAT_TYPE_HEALTH, (f32)chara.hp, (f32)chara.hpMax);
	LOG("Titan :: %ls back to master (%d/%d hp)", player.name.data(), chara.hp, chara.hpMax);
}

void World::TitanOnCast(Player& player, SkillID skill)
{
	if(player.titanForm < 0) return;
	const GameXmlContent::TitanInfo& titan = GetGameXmlContent().titan;
	if((i32)skill != titan.forms[player.titanForm].ug || titan.forms.size() < 2) return;
	player.titanModeAt = TimeAdd(localTime, TimeMsToTime(TITAN_MODE_SWITCH_DELAY_MS));
}

void World::UpdateTitans()
{
	foreach(p, players) {
		if(p->titanForm < 0) continue;
		if(p->titanUntil != Time::ZERO && localTime >= p->titanUntil) {
			RevertTitan(*p);
			continue;
		}
		if(p->titanModeAt != Time::ZERO && localTime >= p->titanModeAt) {
			p->titanModeAt = Time::ZERO;
			const i32 count = (i32)GetGameXmlContent().titan.forms.size();
			TransformTitan(*p, (p->titanForm + 1) % count, false);
		}
	}
}

static const f64 BOT_TITAN_HUMAN_GRACE_SEC = 20.0;

void World::UpdateBotTitanVotes()
{
	const GameXmlContent::TitanInfo& titan = GetGameXmlContent().titan;
	if(titan.forms.empty() || gameStartedAt == Time::ZERO || winnerTeam >= 0) return;
	for(i32 team = 0; team < 2; team++) {
		if(teamTitanPieces[team] < titan.cost) { teamTitanReadyAt[team] = Time::ZERO; continue; }
		if(teamTitanReadyAt[team] == Time::ZERO) teamTitanReadyAt[team] = localTime;
		const TeamVote& vote = teamVotes[team];
		if(vote.active || localTime < vote.cooldownUntil) continue;
		bool titanActive = false, human = false;
		Player* pick = nullptr;
		foreach(p, players) {
			if(p->team != team) continue;
			if(p->titanForm >= 0) titanActive = true;
			if(p->clientHd != ClientHandle::INVALID) human = true;
			else if(!p->dead && localTime >= p->titanVoteBlockedUntil && (!pick || p->Main().hp > pick->Main().hp)) pick = &*p;
		}
		if(titanActive || !pick) continue;
		if(human && TimeDiffSec(TimeDiff(teamTitanReadyAt[team], localTime)) < BOT_TITAN_HUMAN_GRACE_SEC) continue;
		LOG("Titan :: bot %ls asks for the Titan (team %d, %d pieces)", pick->name.data(), team, teamTitanPieces[team]);
		RequestVote(*pick, VOTE_TYPE_TITAN);
	}
}

void World::UpdateIngameLevels()
{
	UpdateVotes();
	UpdateBotTitanVotes();
	UpdateTitans();
	if(!map || !map->ingameLevels || frozen || nextPassiveExpAt == Time::ZERO || localTime < nextPassiveExpAt) return;
	const GameXmlContent::IngameLevelInfo& info = GetGameXmlContent().ingameLevel;
	nextPassiveExpAt = TimeAdd(nextPassiveExpAt, TimeMsToTime(info.passivePeriod * 1000.0));
	foreach(p, players) GrantIngameExp(*p, info.passiveExp, ActorUID::INVALID);
}

struct PraiseTeamTotals
{
	i64 kills = 0;
	i64 damage = 0;
	i64 points = 0;
	i64 core = 0;
	i64 pieces = 0;
};

static i64 PraiseValue(const World::Player& p, PraiseType type)
{
	switch(type) {
		case PraiseType::KILL_ASSIST: return p.kills + p.assists;
		case PraiseType::KILL: return p.kills;
		case PraiseType::DAMAGE_PC: return p.damageDealt;
		case PraiseType::MAX_CONTINUOUS_KILL: return p.bestKillStreak;
		case PraiseType::MAX_CONTINUOUS_KILL_TRANSFORM: return p.bestTitanKillStreak;
		case PraiseType::DOT_KILL: return p.dotKills;
		case PraiseType::HEAL_OTHERS: return p.healOthers;
		case PraiseType::HEAL_SELF: return p.healSelf;
		case PraiseType::NEVER_DIE: return p.deaths;
		case PraiseType::TOTAL_SCORE: return p.points;
		case PraiseType::DAMAGE_CORE: return p.coreDamage;
		case PraiseType::DEFEAT_APPLY_COUNT: return p.ccApplied;
		case PraiseType::KILL_TOWER: return p.monsterKills[World::END_KILL_TOWER];
		case PraiseType::OCCUPY_SIGHT: return p.visionCaptures;
		case PraiseType::SAVE_PIECE: return p.savedPieces;
		case PraiseType::KILL_BOSS: return p.monsterKills[World::END_KILL_BOSS];
		case PraiseType::KILL_TRANSFORMCH: return p.transformKills;
		default: return -1;
	}
}

static u32 PraisePercent(i64 part, i64 total)
{
	return total > 0 && part > 0 ? (u32)(part * 100 / total) : 0;
}

static u32 PraiseRelationValue(const World::Player& p, PraiseRelationType relation, const PraiseTeamTotals& team, u32 playTimeMs)
{
	static const i64 MEASURED_PRAISE_RATE_PERIOD_MS = 60000;
	switch(relation) {
		case PraiseRelationType::KILL_ASSIST: return (u32)(p.kills + p.assists);
		case PraiseRelationType::KILL_ASSIST_RATIO: return PraisePercent(p.kills + p.assists, team.kills);
		case PraiseRelationType::DAMAGE_PC: return (u32)p.damageDealt;
		case PraiseRelationType::DAMAGE_PC_RATIO: return PraisePercent(p.damageDealt, team.damage);
		case PraiseRelationType::HEAL_OTHERS_PER_SECOND: return playTimeMs > 0 ? (u32)((i64)p.healOthers * MEASURED_PRAISE_RATE_PERIOD_MS / playTimeMs) : 0;
		case PraiseRelationType::HEAL_SELF_PER_SECOND: return playTimeMs > 0 ? (u32)((i64)p.healSelf * MEASURED_PRAISE_RATE_PERIOD_MS / playTimeMs) : 0;
		case PraiseRelationType::SCORE_RATIO: return PraisePercent(p.points, team.points);
		case PraiseRelationType::DAMAGE_CORE_RATIO: return PraisePercent(p.coreDamage, team.core);
		case PraiseRelationType::DEFEAT_APPLY_COUNT: return (u32)p.ccApplied;
		case PraiseRelationType::DAMAGE_TOWER: return p.damageStats[END_DMG_BUILDING_DEALT];
		case PraiseRelationType::SAVE_PIECE_RATIO: return PraisePercent(p.savedPieces, team.pieces);
		default: return 0;
	}
}

void World::BuildPraises(GameModeType mode, u32 playTimeMs, eastl::fixed_vector<Replication::EndPraise,10,false>* out)
{
	static const i32 HYP_PRAISE_MAX_ENTRIES = 5;
	const auto& rules = GetGameXmlContent().pvpPraises;
	eastl::array<PraiseTeamTotals,2> teams;
	foreach_const(p, players) {
		PraiseTeamTotals& t = teams[p->team & 1];
		t.kills += p->kills;
		t.damage += p->damageDealt;
		t.points += p->points;
		t.core += p->coreDamage;
		t.pieces += p->savedPieces;
	}
	u32 praisedMask = 0;
	struct Candidate { u32 index; Replication::EndPraise praise; };
	eastl::fixed_vector<Candidate,32,false> candidates;
	for(u32 r = 0; r < (u32)rules.size() && (i32)out->size() < HYP_PRAISE_MAX_ENTRIES && !out->full(); ) {
		const i32 priority = rules[r].priority;
		candidates.clear();
		for(; r < (u32)rules.size() && rules[r].priority == priority; r++) {
			const GameXmlContent::PvpPraise& rule = rules[r];
			if(rule.gameMode != mode) continue;
			const bool fewest = rule.type == PraiseType::NEVER_DIE;
			i32 best = -1;
			i64 bestValue = 0;
			u32 bestRelation = 0;
			for(u32 i = 0; i < (u32)players.size(); i++) {
				const Player& p = players[i];
				const i64 value = PraiseValue(p, rule.type);
				if(value < 0) break;
				const u32 relation = PraiseRelationValue(p, rule.relation, teams[p.team & 1], playTimeMs);
				const bool better = best < 0 || (fewest ? value < bestValue : value > bestValue) || (value == bestValue && relation > bestRelation);
				if(!better) continue;
				best = (i32)i;
				bestValue = value;
				bestRelation = relation;
			}
			if(best < 0 || (praisedMask & (1u << best)) != 0) continue;
			if(fewest ? (f32)bestValue > rule.minValue : (f32)bestValue < rule.minValue) continue;
			const Player& p = players[best];
			Candidate c;
			c.index = (u32)best;
			c.praise.userID = p.userID;
			c.praise.name = p.name.data();
			c.praise.team = p.team;
			c.praise.activeMaster = CreatureIndex(100000000 + (i32)p.Main().classType);
			c.praise.inactiveMaster = CreatureIndex(100000000 + (i32)p.Sub().classType);
			c.praise.type = (u8)rule.type;
			c.praise.value = (u32)bestValue;
			c.praise.relation = (u8)rule.relation;
			c.praise.relationValue = bestRelation;
			if(!candidates.full()) candidates.push_back(c);
		}
		if(candidates.empty()) continue;
		const Candidate& pick = candidates[RandUint() % (u32)candidates.size()];
		praisedMask |= 1u << pick.index;
		out->push_back(pick.praise);
		LOG("Praise :: %ls type %d value %u relation %d %u (priority %d, %d candidates)", pick.praise.name, (i32)pick.praise.type, pick.praise.value, (i32)pick.praise.relation, pick.praise.relationValue, priority, (i32)candidates.size());
	}
}
