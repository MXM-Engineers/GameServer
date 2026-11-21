#include "config.h"
#include "world.h"
#include "world_common.h"
#include <mxm/game_content.h>
#include <chrono>

static const f32 BOT_TITAN_MELEE_SKILL_DIST = 400.0f;

f32 World::BotTitanSkillDistance(i32 skill) const
{
	const GameXmlContent::SkillInput* in = GetGameXmlContent().FindSkillInput(skill);
	return in && in->maxDistance > 0 ? in->maxDistance : BOT_TITAN_MELEE_SKILL_DIST;
}

static const i32 BOT_STAGE_RECOVERY = 0;
static const i32 BOT_STAGE_SHIELD = 1;
static const i32 BOT_STAGE_TELEPORT = 2;
static const i32 BOT_STAGE_WARD = 4;
static const f32 DATA_BOT_WARD_CHECK_DISTANCE = 6000.0f;
static const f32 HYP_BOT_TELEPORT_FROM_BASE = 2500.0f;
static const f32 HYP_BOT_TELEPORT_LANE_DIST = 1500.0f;

static f32 HpRatio(i32 hp, i32 hpMax)
{
	return hpMax > 0 ? (f32)hp / (f32)hpMax : 1.0f;
}

bool World::BotStageSkill(Player& bot)
{
	const auto& content = GetGameXmlContent();
	const i32 skill = (i32)bot.stageSkills[1];
	if(skill <= 0 || bot.titanForm >= 0 || localTime < bot.botStageSkillReady) return false;
	const vec3 pos = bot.body->GetWorldPos();
	ActorUID target = ActorUID::INVALID;
	vec3 at = pos;
	switch(content.BotStageSkillKind(skill)) {
		case BOT_STAGE_RECOVERY: {
			bool use = HpRatio(bot.Main().hp, bot.Main().hpMax) < content.BotConstant("BOTLOGIC_CONSTANT_KEY_MY_LESS_HP_RATIO_FOR_RECOVERY_STAGE_SKILL");
			i32 hurt = 0;
			const f32 radius = content.BotConstant("BOTLOGIC_CONSTANT_KEY_FRIENDLY_CHECK_RADIUS_FOR_RECOVERY_STAGE_SKILL");
			foreach_const(p, players) {
				if(p->index == bot.index || p->team != bot.team || p->dead || glm::length(vec2(p->body->GetWorldPos()) - vec2(pos)) > radius) continue;
				if(HpRatio(p->Main().hp, p->Main().hpMax) < content.BotConstant("BOTLOGIC_CONSTANT_KEY_FRIENDLY_HP_RATIO_FOR_RECOVERY_STAGE_SKILL")) hurt++;
			}
			if(hurt >= (i32)content.BotConstant("BOTLOGIC_CONSTANT_KEY_NUM_MIN_FRIENDLY_FOR_RECOVERY_STAGE_SKILL")) use = true;
			if(!use) return false;
		} break;
		case BOT_STAGE_SHIELD: {
			static const u32 towerCenterTag = StatusTypeKey("Tower_Center");
			static const u32 towerSideTag = StatusTypeKey("Tower_Side");
			const f32 radius = content.BotConstant("BOTLOGIC_CONSTANT_KEY_TOWER_CHECK_RADIUS_FOR_SHIELD_STAGE_SKILL");
			const f32 ratio = content.BotConstant("BOTLOGIC_CONSTANT_KEY_TOWER_HP_RATIO_FOR_SHIELD_STAGE_SKILL");
			foreach_const(npc, actorNpcList) {
				if(!npc->monsterAi || (npc->monsterAi->tagKey != towerCenterTag && npc->monsterAi->tagKey != towerSideTag) || !npc->monsterAi->byTarget[GameXmlContent::MonsterAttack::MASTER] || npc->hp <= 0) continue;
				if((i32)npc->faction != (i32)bot.team || glm::length(vec2(npc->pos) - vec2(pos)) > radius || HpRatio(npc->hp, npc->hpMax) >= ratio) continue;
				target = npc->UID;
				at = npc->pos;
				break;
			}
			if(target == ActorUID::INVALID) return false;
		} break;
		case BOT_STAGE_TELEPORT: {
			if(glm::length(vec2(pos) - vec2(bot.spawnPos)) > HYP_BOT_TELEPORT_FROM_BASE) return false;
			vec2 laneGoal;
			if(!BotLaneGoal(bot, &laneGoal) || bot.botLane < 0 || botLanes[bot.team].empty()) return false;
			const auto& lane = botLanes[bot.team][bot.botLane % (i32)botLanes[bot.team].size()];
			static const u32 towerCenterTag = StatusTypeKey("Tower_Center");
			static const u32 towerSideTag = StatusTypeKey("Tower_Side");
			const f32 enemyRadius = content.BotConstant("BOTLOGIC_CONSTANT_KEY_ENEMY_CHECK_RADIUS_FOR_TELEPORT_STAGE_SKILL");
			f32 farthest = HYP_BOT_TELEPORT_FROM_BASE;
			foreach_const(npc, actorNpcList) {
				if(!npc->monsterAi || (npc->monsterAi->tagKey != towerCenterTag && npc->monsterAi->tagKey != towerSideTag) || !npc->monsterAi->byTarget[GameXmlContent::MonsterAttack::MASTER] || npc->hp <= 0 || (i32)npc->faction != (i32)bot.team) continue;
				f32 laneDist = 1e30f;
				foreach_const(pt, lane) laneDist = MIN(laneDist, glm::length(*pt - vec2(npc->pos)));
				const f32 fromBase = glm::length(vec2(npc->pos) - vec2(bot.spawnPos));
				if(laneDist > HYP_BOT_TELEPORT_LANE_DIST || fromBase <= farthest) continue;
				bool enemyNear = false;
				foreach_const(p, players) if(p->team != bot.team && !p->dead && glm::length(vec2(p->body->GetWorldPos()) - vec2(npc->pos)) < enemyRadius) enemyNear = true;
				if(enemyNear) continue;
				farthest = fromBase;
				target = npc->UID;
				at = npc->pos;
			}
			if(target == ActorUID::INVALID) return false;
		} break;
		case BOT_STAGE_WARD: {
			if(glm::length(vec2(pos) - vec2(bot.spawnPos)) <= HYP_BOT_TELEPORT_FROM_BASE) return false;
			foreach_const(npc, actorNpcList) {
				if(!OneHitSummon((i32)npc->docID) || npc->hp <= 0 || npc->ownerUID == ActorUID::INVALID || players[npc->ownerIndex].team != bot.team) continue;
				if(glm::length(vec2(npc->pos) - vec2(pos)) < DATA_BOT_WARD_CHECK_DISTANCE) return false;
			}
		} break;
		default: return false;
	}
	bot.botStageSkillReady = TimeAdd(localTime, TimeMsToTime(MAX(content.GetSkillCoolTime(skill), 1.0f) * 1000.0));
	LOG("BotSkill :: %ls uses stage skill %d", bot.name.data(), skill);
	PlayerCastSkill(bot, SkillID(skill), at, Slice<const ActorUID>(&target, target != ActorUID::INVALID ? 1 : 0), 0);
	return true;
}

static const f64 MEASURED_BOT_RETURN_AFTER_DAMAGE_SEC = 2.7;

bool World::BotReturnSkill(Player& bot)
{
	const auto& content = GetGameXmlContent();
	const i32 skill = (i32)bot.stageSkills[0];
	static const i32 HYP_BOT_TRAPPED_EPISODES = 3;
	const bool trapped = bot.botStuckEpisodes >= HYP_BOT_TRAPPED_EPISODES && localTime < bot.botStuckAvoidUntil;
	if(skill <= 0 || skill != content.BotReturnSkill() || !(bot.botRetreat || trapped) || bot.titanForm >= 0 || bot.devGod || localTime < bot.botReturnReady) return false;
	if(bot.lastDamagedAt != Time::ZERO && TimeDiffSec(TimeDiff(bot.lastDamagedAt, localTime)) < MEASURED_BOT_RETURN_AFTER_DAMAGE_SEC) return false;
	const vec3 pos = bot.body->GetWorldPos();
	if(glm::length(vec2(pos) - vec2(bot.spawnPos)) <= content.BotConstant("BOTLOGIC_STRATEGY_CONST_KEY_IN_BASE_CAMP_RANGE")) return false;
	vec2 item;
	if(BotItemGoal(bot, &item)) return false;
	bot.botReturnReady = TimeAdd(localTime, TimeMsToTime(content.BotConstant("BOTLOGIC_STRATEGY_CONST_KEY_USE_RETURN_SKILL_STRATEGY_DISABLE_TIME") * 1000.0));
	bot.input.moveTo = pos;
	LOG("BotSkill :: %ls uses return skill %d at %.0f from base%s", bot.name.data(), skill, glm::length(vec2(pos) - vec2(bot.spawnPos)), trapped ? " (trapped)" : "");
	bot.botStuckEpisodes = 0;
	const ActorUID none = ActorUID::INVALID;
	PlayerCastSkill(bot, SkillID(skill), pos, Slice<const ActorUID>(&none, (size_t)0), 0);
	return true;
}

static const f32 HYP_BOT_EVADE_AIM_DISTANCE = 500.0f;
static const f32 MEASURED_BOT_EVADE_CONTACT_DIST = 306.0f;

bool World::BotEvadeZone(const Player& bot, vec2* away, f32* depth) const
{
	const vec2 at = vec2(bot.body->GetWorldPos());
	foreach_const(mr, monsterRemotes) {
		const Remote* r = mr->remote;
		if(!r || r->evadeBound == 0 || r->evadeLengthX <= 0 || localTime >= mr->expire) continue;
		const ActorNpc* owner = FindNpcActor(mr->owner);
		if(!owner || !NpcHittableBy(*owner, bot)) continue;
		const vec2 rel = at - vec2(mr->pos);
		const f32 dist = glm::length(rel);
		const vec2 dir = glm::length(mr->dir) > 0.5f ? glm::normalize(mr->dir) : vec2(1, 0);
		const vec2 lateral = vec2(-dir.y, dir.x);
		if(r->evadeBound == 1) {
			const f32 radius = r->evadeLengthX * 0.5f;
			if(dist > radius) continue;
			*away = dist >= 1.0f ? rel / dist : lateral;
			*depth = radius - dist;
			if(r->evadeAngle > 0 && dist >= 1.0f) {
				const f32 half = MIN(r->evadeAngle * 0.5f, 180.0f) * PI / 180.0f;
				const f32 off = acosf(glm::clamp(glm::dot(rel / dist, dir), -1.0f, 1.0f));
				if(off > half) continue;
				const f32 toEdge = dist * sinf(half - off);
				if(half < PI * 0.5f && toEdge < *depth) {
					const vec2 perp = vec2(-rel.y, rel.x) / dist;
					*away = glm::dot(perp, lateral) * glm::dot(rel, lateral) >= 0.0f ? perp : -perp;
					*depth = toEdge;
				}
			}
		}
		else {
			const f32 along = glm::dot(rel, dir);
			const f32 side = glm::dot(rel, lateral);
			if(along < 0 || along > r->evadeLengthY || fabsf(side) > r->evadeLengthX * 0.5f) continue;
			*away = side >= 0 ? lateral : -lateral;
			*depth = r->evadeLengthX * 0.5f - fabsf(side);
		}
		return true;
	}
	return false;
}

bool World::BotEvade(Player& bot)
{
	const auto& content = GetGameXmlContent();
	if(localTime < bot.botEvadeReady) return false;
	const f32 nearby = content.BotConstant("BOTLOGIC_CONSTANT_KEY_NEARBY_REMOTE_DISTANCE_FOR_GOOD_EVADE_STATE_IN_BOTHELPER");
	const f32 cooltime = content.BotConstant("BOTLOGIC_CONSTANT_KEY_EVADE_COOLTIME_IN_PVP_AI_ENTITY_BASE");
	if(nearby <= 0 || cooltime <= 0) return false;
	const vec2 at = vec2(bot.body->GetWorldPos());
	vec2 incoming = vec2(0);
	vec2 forcedSide = vec2(0);
	foreach_const(pr, projectileList) {
		if(players[pr->ownerIndex].team == bot.team || pr->damage <= 0) continue;
		if(glm::length(vec2(pr->pos) - at) > nearby) continue;
		incoming = pr->dir;
		break;
	}
	if(glm::length(incoming) < 0.5f) {
		foreach_const(ri, remoteInsts) {
			if(!ri->remote || players[ri->ownerIndex].team == bot.team || ri->remote->attackMultiplier <= 0) continue;
			if(ri->remote->damageGroup != Remote::DamageGroup::eENEMY && ri->remote->damageGroup != Remote::DamageGroup::eALL) continue;
			if(glm::length(vec2(ri->pos) - at) > nearby) continue;
			incoming = glm::length(ri->dir) > 0.5f ? ri->dir : glm::normalize(at - vec2(ri->pos) + vec2(0.01f, 0.0f));
			break;
		}
	}
	f32 zoneDepth = 0;
	if(glm::length(incoming) < 0.5f && BotEvadeZone(bot, &forcedSide, &zoneDepth)) {
		incoming = vec2(-forcedSide.y, forcedSide.x);
		LOG("BotEvade :: %ls inside a monster evade zone, %.0f from its edge", bot.name.data(), zoneDepth);
	}
	if(glm::length(incoming) < 0.5f && FindPassiveOfType(bot, StatusType::STATUS_ON_EVADE)) {
		foreach_const(e, players) {
			if(e->dead || e->untargetable || e->team == bot.team || !(e->fogSeenBy & (1u << bot.team))) continue;
			const vec2 rel = at - vec2(e->body->GetWorldPos());
			if(glm::length(rel) > MEASURED_BOT_EVADE_CONTACT_DIST) continue;
			incoming = glm::normalize(rel + vec2(0.01f, 0.0f));
			break;
		}
	}
	if(glm::length(incoming) < 0.5f) return false;
	const GameXmlContent::Master& master = content.GetMaster(bot.Main().classType);
	SkillID shirk = master.shirkSkill;
	if(bot.titanForm >= 0 && bot.titanForm < (i32)content.titan.forms.size()) {
		const i32 family = content.titan.forms[bot.titanForm].skill1 / 1000;
		shirk = SkillID::INVALID;
		foreach_const(sk, content.skillMap) if(sk->second.type == SkillType::SHIRK && (i32)sk->first / 1000 == family) { shirk = sk->first; break; }
	}
	else if(shirk == SkillID::INVALID) foreach_const(sid, master.skillIDs) {
		auto f = content.skillMap.find(*sid);
		if(f != content.skillMap.end() && f->second.type == SkillType::SHIRK) { shirk = *sid; break; }
	}
	if(shirk == SkillID::INVALID || !SkillCostAffordable(bot, (i32)shirk, PlayerSkillConsumeMP(bot, (i32)shirk))) return false;
	bot.botEvadeReady = TimeAdd(localTime, TimeMsToTime(cooltime * 1000.0));
	const vec2 side = glm::length(forcedSide) > 0.5f ? forcedSide : ((RandUint() & 1) ? vec2(-incoming.y, incoming.x) : vec2(incoming.y, -incoming.x));
	const vec3 to = vec3(at + side * HYP_BOT_EVADE_AIM_DISTANCE, bot.body->GetWorldPos().z);
	const f32 yaw = atan2f(side.y, side.x);
	bot.input.rot.upperYaw = yaw;
	bot.input.rot.bodyYaw = yaw;
	LOG("BotEvade :: %ls evades with %d", bot.name.data(), (i32)shirk);
	const ActorUID none = ActorUID::INVALID;
	PlayerCastSkill(bot, shirk, to, Slice<const ActorUID>(&none, (size_t)0), 0);
	return true;
}

void World::UpdateBotSkills()
{
	const auto& content = GetGameXmlContent();
	foreach(it, players) {
		Player& bot = *it;
		if(bot.clientHd != ClientHandle::INVALID || bot.dead) continue;
		if(bot.casting.skillID != SkillID::INVALID && (i32)bot.casting.skillID == content.BotReturnSkill()) bot.botReturnReady = TimeAdd(localTime, TimeMsToTime(content.BotConstant("BOTLOGIC_STRATEGY_CONST_KEY_USE_RETURN_SKILL_STRATEGY_DISABLE_TIME") * 1000.0));
		if(localTime >= bot.tNextBotSkill && BotReleaseHold(bot)) continue;
		if(localTime < bot.tNextBotSkill || localTime < bot.ccUntil || localTime < bot.dashUntil || localTime < bot.botSkillStillUntil || bot.botShotAt == localTime) continue;
		if(bot.casting.skillID == SkillID::INVALID && !Config().DevShowcase && !IsBlocked(bot, false, false, true) && BotEvade(bot)) {
			bot.tNextBotSkill = TimeAdd(localTime, TimeMsToTime(500));
			continue;
		}
		vec2 zoneAway;
		f32 zoneDepth = 0;
		if(IsBlocked(bot, false, true, false) || BotHoldsHide(bot) || BotEvadeZone(bot, &zoneAway, &zoneDepth)) continue;
		bot.tNextBotSkill = TimeAdd(localTime, TimeMsToTime(500));
		if(bot.casting.skillID != SkillID::INVALID || BotReturnSkill(bot) || BotStageSkill(bot)) continue;

		const ActorMaster& master = bot.Main();
		const bool titan = bot.titanForm >= 0 && bot.titanForm < (i32)content.titan.forms.size();
		const auto* skills = content.FindBotSkills(titan ? content.titan.forms[bot.titanForm].doc : 100000000 + (i32)master.classType);
		if(!skills) continue;

		const vec3 pos = bot.body->GetWorldPos();
		Player* target = nullptr;
		f32 best = 1e9f;
		foreach(e, players) {
			if(e->dead || e->untargetable || e->team == bot.team || !(e->fogSeenBy & (1u << bot.team))) continue;
			const f32 d = glm::length(vec2(e->body->GetWorldPos()) - vec2(pos));
			if(d < best) { best = d; target = &*e; }
		}
		ActorNpc* monster = nullptr;
		if(Config().DevBotMonsterSkills) {
			f32 bestMonster = best;
			foreach(n, actorNpcList) {
				if(!n->monsterAi || n->hp <= 0 || n->removeAt != Time::ZERO || !NpcHittableBy(*n, bot)) continue;
				const f32 d = glm::length(vec2(n->pos) - vec2(pos));
				if(d < bestMonster) { bestMonster = d; monster = &*n; }
			}
			if(monster) best = bestMonster;
		}
		if(!target && !monster) continue;

		i32 skill = 0;
		f32 cd = 0;
		if(bot.botComboSkill != SkillID::INVALID) {
			const i32 combo = (i32)bot.botComboSkill;
			const f32 comboDist = content.GetBotSkillMaxDistance(combo);
			if(localTime >= bot.botComboUntil || bot.botComboClass != bot.Main().classType) bot.botComboSkill = SkillID::INVALID;
			else if((comboDist <= 0 || best <= comboDist) && (Config().DevShowcase || Config().DevMirror || SkillCostAffordable(bot, combo, PlayerSkillConsumeMP(bot, combo)))) skill = combo;
		}
		if(skill == 0) {
			const i32 base = (i32)bot.mainCharaID * 2;
			eastl::fixed_vector<i32,5,false> ready;
			for(i32 i = 0; i < 5; i++) {
				const i32 skill = titan && i == 4 ? content.titan.forms[bot.titanForm].ug : (*skills)[i];
				if(skill == 0 || localTime < bot.botSkillReady[i]) continue;
				if(!titan && i != 4 && !Config().DevShowcase && (i32)bot.skills[base] != skill && (i32)bot.skills[base + 1] != skill) continue;
				const f32 maxDist = titan ? BotTitanSkillDistance(skill) : content.GetBotSkillMaxDistance(skill);
				if(maxDist <= 0 || best > maxDist) continue;
				if(!Config().DevShowcase && !Config().DevMirror && !SkillCostAffordable(bot, skill, PlayerSkillConsumeMP(bot, skill))) continue;
				ready.push_back(i);
			}
			if(ready.empty()) continue;
			const i32 slot = ready[RandUint() % ready.size()];
			skill = titan && slot == 4 ? content.titan.forms[bot.titanForm].ug : (*skills)[slot];

			const f32 botCd = slot == 4 ? content.BotUgSkillCoolTime() : content.BotNormalSkillCoolTime();
			cd = MAX(SkillCoolTime(bot, skill), botCd);
			bot.botSkillReady[slot] = TimeAdd(localTime, TimeMsToTime(cd * 1000.0));
		}
		const GameXmlContent::SkillInput* in = content.FindSkillInput(skill);
		bot.tNextBotSkill = TimeAdd(localTime, TimeMsToTime(MAX(MAX(content.BotAfterSkillDelay(), 0.5f), in ? in->globalCoolTime : 0.0f) * 1000.0));

		if(in && in->friendlyOnly && in->takesTarget) {
			const f32 maxDist = content.GetBotSkillMaxDistance(skill);
			Player* ally = nullptr;
			f32 lowest = 2.0f;
			foreach(f, players) {
				if(&*f == &bot || f->dead || f->untargetable || f->team != bot.team || !f->body) continue;
				if(glm::length(vec2(f->body->GetWorldPos()) - vec2(pos)) > maxDist) continue;
				const f32 ratio = (f32)f->Main().hp / (f32)MAX(f->Main().hpMax, 1);
				if(ratio < lowest) { lowest = ratio; ally = &*f; }
			}
			if(!ally) continue;
			target = ally;
			monster = nullptr;
			best = glm::length(vec2(ally->body->GetWorldPos()) - vec2(pos));
		}
		const vec3 tpos = monster ? monster->pos : target->body->GetWorldPos();
		const f32 yaw = atan2f(tpos.y - pos.y, tpos.x - pos.x);
		bot.input.rot.upperYaw = yaw;
		bot.input.rot.bodyYaw = yaw;
		bot.weaponAimYaw = yaw;
		const ActorUID targetUID = monster ? monster->UID : target->Main().UID;
		bot.botAimTarget = targetUID;
		if(monster) LOG("BotSkill :: %ls casts %d at creature=%d (dist %.0f, cd %.0f)", bot.name.data(), skill, (i32)monster->docID, best, cd);
		else LOG("BotSkill :: %ls casts %d at %ls (dist %.0f, cd %.0f)", bot.name.data(), skill, target->name.data(), best, cd);
		PlayerCastSkill(bot, SkillID(skill), tpos, Slice<const ActorUID>(&targetUID, in && in->takesTarget ? 1 : 0), 0);
		i32 next = ComboSkillFor(bot, skill);
		if(next == 0) next = content.GetNextComboSkill(skill);
		const i32 windowMs = next != 0 && next != skill && content.FindSkillHitEventRemote(skill) == 0 ? content.GetComboDurationMs(next) : 0;
		const i32 autoMs = windowMs > 0 ? content.GetComboBotAutoCastMs(next) : 0;
		if(autoMs > 0) {
			bot.comboSkill = SkillID(next);
			bot.comboCastPos = tpos;
			bot.comboAutoAt = TimeAdd(localTime, TimeMsToTime((f64)autoMs));
		}
		bot.botComboSkill = windowMs > 0 && autoMs <= 0 ? SkillID(next) : SkillID::INVALID;
		bot.botComboUntil = windowMs > 0 ? TimeAdd(localTime, TimeMsToTime((f64)windowMs)) : Time::ZERO;
		bot.botComboClass = bot.Main().classType;
	}
}

static const f32 BOT_ENGAGE_RANGE = 2500.0f;
static const f32 BOT_MELEE_RANGE = 150.0f;
static const f32 BOT_MIN_FIRE_DELAY = 0.15f;
static const f32 BOT_RANGE_FRACTION = 0.7f;
static const f32 BOT_RANGED_MAX = 800.0f;
static const f32 BOT_STRAFE_STEP = 0.6f;
static const f32 BOT_REPATH_DIST = 150.0f;
static const f32 BOT_STUCK_DIST = 20.0f;
static const f32 BOT_ROAM_STEP = 800.0f;
static const f32 BOT_PIECE_SEARCH_RANGE = 4000.0f;
static const f32 MEASURED_BOT_ITEM_REACH = 1500.0f;
static const i32 BOT_TITAN_PIECE_ITEM = 130100904;

static const f32 BOT_FARM_RANGE = 4000.0f;

static const i32 HYP_BOT_JUNGLE_ROLE_EVERY = 5;
static const f32 HYP_BOT_JUNGLE_RANGE = 4000.0f;

bool World::BotFarmTarget(Player& bot, vec2* goal, f32 range)
{
	static const f32 HYP_BOT_FARM_PROGRESS_SEC = 8.0f;
	static const f32 HYP_BOT_FARM_PROGRESS_DIST = 300.0f;
	static const f32 HYP_BOT_FARM_SKIP_SEC = 30.0f;
	static const f32 HYP_BOT_FARM_KEEP_RANGE_MUL = 1.5f;
	static const f32 HYP_BOT_FARM_ROUTE_GOAL_SLACK = 1300.0f;
	if(!map || !map->ingameLevels) return false;
	const vec2 here = vec2(bot.body->GetWorldPos());
	const bool jungler = bot.clientHd == ClientHandle::INVALID && bot.index % HYP_BOT_JUNGLE_ROLE_EVERY == 2;
	const f32 neutralRange = jungler ? MAX(range, HYP_BOT_JUNGLE_RANGE) : range;
	f32 bestMinion = range, bestNeutral = neutralRange;
	const ActorNpc* minion = nullptr;
	const ActorNpc* neutral = nullptr;
	const ActorNpc* kept = nullptr;
	for(i32 s = (i32)bot.botFarmSkips.size() - 1; s >= 0; s--) if(localTime >= bot.botFarmSkips[s].second) bot.botFarmSkips.erase(bot.botFarmSkips.begin() + s);
	foreach_const(npc, actorNpcList) {
		if(!npc->monsterAi || npc->ownerUID != ActorUID::INVALID || IsStructure(*npc) || !NpcHittableBy(*npc, bot) || NpcHasStatusType(*npc, StatusType::GODMODE)) continue;
		bool skipped = false;
		foreach_const(s, bot.botFarmSkips) if(s->first == npc->UID) skipped = true;
		if(skipped) continue;
		const f32 d = glm::length(vec2(npc->pos) - here);
		if(npc->faction == Faction::RED || npc->faction == Faction::BLUE) {
			if(d < range * HYP_BOT_FARM_KEEP_RANGE_MUL && !BotDangerAt(bot, vec2(npc->pos), nullptr, nullptr)) {
				if(npc->UID == bot.botFarmUID) kept = &*npc;
				if(d < bestMinion) { bestMinion = d; minion = &*npc; }
			}
		}
		else if((npc->faction == Faction::ORANGE || npc->faction == Faction::MONSTER) && d < neutralRange * HYP_BOT_FARM_KEEP_RANGE_MUL) {
			if(npc->UID == bot.botFarmUID) kept = &*npc;
			if(d < bestNeutral) { bestNeutral = d; neutral = &*npc; }
		}
	}
	if(bestMinion >= range) minion = nullptr;
	if(bestNeutral >= neutralRange) neutral = nullptr;
	const ActorNpc* chosen = kept ? kept : (jungler && neutral) ? neutral : minion ? minion : (neutral && bestNeutral < range) ? neutral : nullptr;
	if(!chosen) {
		const ActorNpc* dropped = bot.botFarmUID != ActorUID::INVALID ? FindNpcActor(bot.botFarmUID) : nullptr;
		if(dropped && dropped->hp > 0 && dropped->removeAt == Time::ZERO && (dropped->faction == Faction::ORANGE || dropped->faction == Faction::MONSTER)) {
			LOG("BotFarm :: %ls drops creature=%d at %.0f, out of reach", bot.name.data(), (i32)dropped->docID, glm::length(vec2(dropped->pos) - here));
			static const f32 HYP_BOT_CAMP_RADIUS = 1000.0f;
			foreach_const(npc, actorNpcList) {
				if(&*npc != dropped && (npc->faction != dropped->faction || glm::length(vec2(npc->home) - vec2(dropped->home)) > HYP_BOT_CAMP_RADIUS)) continue;
				if(bot.botFarmSkips.full()) bot.botFarmSkips.erase(bot.botFarmSkips.begin());
				bot.botFarmSkips.push_back(eastl::make_pair(npc->UID, TimeAdd(localTime, TimeMsToTime(HYP_BOT_FARM_SKIP_SEC * 1000.0))));
			}
		}
		bot.botFarmUID = ActorUID::INVALID;
		return false;
	}
	const f32 dist = glm::length(vec2(chosen->pos) - here);
	const bool chosenNeutral = chosen->faction == Faction::ORANGE || chosen->faction == Faction::MONSTER;
	const f32 reach = (chosenNeutral ? neutralRange : range) * HYP_BOT_FARM_KEEP_RANGE_MUL;
	if(chosen->UID == bot.botFarmUID && !bot.botRoute.empty() && glm::length(bot.botRouteGoal - vec2(chosen->pos)) <= HYP_BOT_FARM_ROUTE_GOAL_SLACK && bot.botRouteCost > reach) {
		LOG("BotFarm :: %ls skips creature=%d, route %.0f longer than %.0f", bot.name.data(), (i32)chosen->docID, bot.botRouteCost, reach);
		static const f32 HYP_BOT_CAMP_RADIUS_ROUTE = 1500.0f;
		foreach_const(npc, actorNpcList) {
			if(&*npc != chosen && (!chosenNeutral || npc->faction != chosen->faction || glm::length(vec2(npc->home) - vec2(chosen->home)) > HYP_BOT_CAMP_RADIUS_ROUTE)) continue;
			if(bot.botFarmSkips.full()) bot.botFarmSkips.erase(bot.botFarmSkips.begin());
			bot.botFarmSkips.push_back(eastl::make_pair(npc->UID, TimeAdd(localTime, TimeMsToTime(HYP_BOT_FARM_SKIP_SEC * 1000.0))));
		}
		bot.botFarmUID = ActorUID::INVALID;
		return false;
	}
	if(chosen->UID != bot.botFarmUID) {
		bot.botFarmUID = chosen->UID;
		bot.botFarmCheckAt = TimeAdd(localTime, TimeMsToTime(HYP_BOT_FARM_PROGRESS_SEC * 1000.0));
		bot.botFarmCheckDist = dist;
		bot.botFarmCheckHp = chosen->hp;
	}
	else if(localTime >= bot.botFarmCheckAt) {
		if(dist > bot.botFarmCheckDist - HYP_BOT_FARM_PROGRESS_DIST && (dist > BOT_RANGED_MAX + HYP_BOT_FARM_PROGRESS_DIST || chosen->hp >= bot.botFarmCheckHp)) {
			LOG("BotFarm :: %ls gives up creature=%d at %.0f (was %.0f %.0f s ago)", bot.name.data(), (i32)chosen->docID, dist, bot.botFarmCheckDist, HYP_BOT_FARM_PROGRESS_SEC);
			if(bot.botFarmSkips.full()) bot.botFarmSkips.erase(bot.botFarmSkips.begin());
			bot.botFarmSkips.push_back(eastl::make_pair(chosen->UID, TimeAdd(localTime, TimeMsToTime(HYP_BOT_FARM_SKIP_SEC * 1000.0))));
			bot.botFarmUID = ActorUID::INVALID;
			return false;
		}
		bot.botFarmCheckAt = TimeAdd(localTime, TimeMsToTime(HYP_BOT_FARM_PROGRESS_SEC * 1000.0));
		bot.botFarmCheckDist = dist;
		bot.botFarmCheckHp = chosen->hp;
	}
	*goal = vec2(chosen->pos);
	return true;
}

void World::BuildTitanSaveAreas()
{
	if(titanSaveAreasBuilt || !map) return;
	titanSaveAreasBuilt = true;
	{
		foreach_const(n, map->levelEvents) {
			if(n->kind != GameXmlContent::LevelEventNode::BOUND_TRIGGER) continue;
			const GameXmlContent::Map::Area* area = map->FindArea(n->areaID);
			if(!area) continue;
			foreach_const(o, n->outputs) {
				const GameXmlContent::LevelEventNode* cond = map->FindLevelEvent(*o);
				if(!cond || cond->kind != GameXmlContent::LevelEventNode::TEAM_CONDITION || (cond->team != Faction::RED && cond->team != Faction::BLUE)) continue;
				foreach_const(s, cond->outputs) {
					const GameXmlContent::LevelEventNode* save = map->FindLevelEvent(*s);
					if(save && save->kind == GameXmlContent::LevelEventNode::STATUS_CHANGE && save->apply && save->statusIndex != 0) {
						const StatusInfo* heal = GetGameXmlContent().FindStatus(save->statusIndex);
						if(heal && heal->type == StatusType::HEAL) {
							titanHealAreas[(i32)cond->team] = vec2(area->pos);
							titanHealAreaKnown[(i32)cond->team] = true;
						}
					}
					if(!save || save->kind != GameXmlContent::LevelEventNode::SAVE_GOT_PIECE) continue;
					titanSaveAreas[(i32)cond->team] = vec2(area->pos);
					titanSaveAreaKnown[(i32)cond->team] = true;
				}
			}
		}
	}
}

static const i32 MEASURED_CHIEF_MASTERS_PER_TEAM = 4;

bool World::BotChiefGoal(const Player& bot, vec2* goal) const
{
	if(bot.titanForm >= 0) return false;
	static const u32 chiefTag = StatusTypeKey("Chief");
	const vec2 here = vec2(bot.body->GetWorldPos());
	foreach_const(npc, actorNpcList) {
		if(!npc->monsterAi || npc->monsterAi->tagKey != chiefTag || npc->hp <= 0 || npc->removeAt != Time::ZERO || !NpcHittableBy(*npc, bot) || NpcHasStatusType(*npc, StatusType::GODMODE)) continue;
		bool skipped = false;
		foreach_const(s, bot.botFarmSkips) if(s->first == npc->UID && localTime < s->second) skipped = true;
		if(skipped) continue;
		const vec2 at = vec2(npc->pos);
		if(BotDangerAt(bot, at, nullptr, nullptr)) continue;
		const f32 mine = glm::length(at - here);
		i32 closer = 0;
		foreach_const(p, players) {
			if(&*p == &bot || p->team != bot.team || p->dead || p->removed || !p->body || p->titanForm >= 0) continue;
			if(glm::length(at - vec2(p->body->GetWorldPos())) < mine) closer++;
		}
		if(closer >= MEASURED_CHIEF_MASTERS_PER_TEAM) continue;
		const f32 range = WeaponIsMelee(bot) ? BOT_MELEE_RANGE : MIN(WeaponRange(bot) * BOT_RANGE_FRACTION, BOT_RANGED_MAX);
		const vec2 away = here - at;
		const f32 len = glm::length(away);
		*goal = at + (len > 1.0f ? away / len : vec2(1, 0)) * MIN(range, len);
		botLastBossUID = npc->UID;
		return true;
	}
	return false;
}

static const i32 MEASURED_COMPONENT_FIRST_MASTER_HIT_SEC[] = { 15, 24, 27, 232, 233, 278, 311, 313, 449, 569 };
static const i32 MEASURED_KING_FIRST_MASTER_HIT_SEC[] = { 34, 46, 49, 78, 78, 132, 155, 166, 183, 233 };
static const i32 MEASURED_ROBOT_BUFF_FIRST_MASTER_HIT_SEC[] = { 3, 6, 13, 21, 24, 37, 51, 59, 72, 93 };
static const i32 MEASURED_ROBOT_EXP_FIRST_MASTER_HIT_SEC[] = { 1, 2, 10, 20, 30, 57, 82, 144, 255, 387 };
static const i32 MEASURED_ROBOT_BUFF2_FIRST_MASTER_HIT_SEC[] = { 1, 8, 13, 14, 23, 31, 48, 73, 84, 115 };
struct BotBossRule
{
	i32 docID;
	const i32* firstHitSec;
	i32 firstHitCount;
	i32 masters;
};
static const BotBossRule MEASURED_BOT_BOSS_RULES[] = {
	{ 105001402, MEASURED_COMPONENT_FIRST_MASTER_HIT_SEC, (i32)ARRAY_COUNT(MEASURED_COMPONENT_FIRST_MASTER_HIT_SEC), 1 },
	{ 105001040, MEASURED_KING_FIRST_MASTER_HIT_SEC, (i32)ARRAY_COUNT(MEASURED_KING_FIRST_MASTER_HIT_SEC), 3 },
	{ 105001400, MEASURED_ROBOT_BUFF_FIRST_MASTER_HIT_SEC, (i32)ARRAY_COUNT(MEASURED_ROBOT_BUFF_FIRST_MASTER_HIT_SEC), 1 },
	{ 105001401, MEASURED_ROBOT_EXP_FIRST_MASTER_HIT_SEC, (i32)ARRAY_COUNT(MEASURED_ROBOT_EXP_FIRST_MASTER_HIT_SEC), 1 },
	{ 105001403, MEASURED_ROBOT_BUFF2_FIRST_MASTER_HIT_SEC, (i32)ARRAY_COUNT(MEASURED_ROBOT_BUFF2_FIRST_MASTER_HIT_SEC), 2 },
};

static const f32 HYP_BOT_BOSS_ENGAGED_MARGIN = 300.0f;

bool World::BotBossGoal(const Player& bot, vec2* goal)
{
	if(bot.titanForm >= 0) return false;
	const vec2 here = vec2(bot.body->GetWorldPos());
	const ActorNpc* chosen = nullptr;
	f32 chosenDist = 0.0f;
	foreach(npc, actorNpcList) {
		if(!npc->monsterAi || npc->faction != Faction::ORANGE || npc->hp <= 0 || npc->removeAt != Time::ZERO || NpcHasStatusType(*npc, StatusType::GODMODE)) continue;
		bool skipped = false;
		foreach_const(s, bot.botFarmSkips) if(s->first == npc->UID && localTime < s->second) skipped = true;
		if(skipped) continue;
		const BotBossRule* rule = nullptr;
		for(i32 r = 0; r < (i32)ARRAY_COUNT(MEASURED_BOT_BOSS_RULES); r++) if((i32)npc->docID == MEASURED_BOT_BOSS_RULES[r].docID) rule = &MEASURED_BOT_BOSS_RULES[r];
		if(!rule) continue;
		if(npc->botBossFrom == Time::ZERO) {
			npc->botBossFrom = TimeAdd(localTime, TimeMsToTime(rule->firstHitSec[RandUint() % (u32)rule->firstHitCount] * 1000.0));
			LOG("Bot :: boss creature=%d contested from %.0f s", (i32)npc->docID, TimeDiffSec(TimeDiff(localTime, npc->botBossFrom)));
		}
		if(localTime < npc->botBossFrom) continue;
		const vec2 at = vec2(npc->pos);
		if(BotDangerAt(bot, at, nullptr, nullptr)) continue;
		const f32 mine = glm::length(at - here);
		const f32 reach = WeaponIsMelee(bot) ? BOT_MELEE_RANGE : MIN(WeaponRange(bot) * BOT_RANGE_FRACTION, BOT_RANGED_MAX);
		const bool engaged = npc->monsterTarget == bot.Main().UID || mine <= reach + HYP_BOT_BOSS_ENGAGED_MARGIN;
		i32 closer = 0;
		foreach_const(p, players) {
			if(&*p == &bot || p->dead || p->removed || !p->body || p->titanForm >= 0) continue;
			if(glm::length(at - vec2(p->body->GetWorldPos())) < mine) closer++;
		}
		if(!engaged && closer >= rule->masters) continue;
		if(!chosen || mine < chosenDist) {
			chosen = &*npc;
			chosenDist = mine;
		}
	}
	if(!chosen) return false;
	botLastBossUID = chosen->UID;
	if(!BotShotClear(bot, chosen->pos, NpcHitRadius(*chosen))) {
		*goal = vec2(chosen->pos);
		return true;
	}
	const f32 range = WeaponIsMelee(bot) ? BOT_MELEE_RANGE : MIN(WeaponRange(bot) * BOT_RANGE_FRACTION, BOT_RANGED_MAX);
	const vec2 away = here - vec2(chosen->pos);
	const f32 len = glm::length(away);
	*goal = vec2(chosen->pos) + (len > 1.0f ? away / len : vec2(1, 0)) * MIN(range, len);
	return true;
}

bool World::BotObjective(const Player& bot, vec2* goal)
{
	if(!map) return false;
	BuildTitanSaveAreas();
	const vec2 here = vec2(bot.body->GetWorldPos());
	if(bot.titanPieces > 0 && titanSaveAreaKnown[bot.team]) {
		*goal = titanSaveAreas[bot.team];
		botObjectiveKind = 's';
		return true;
	}
	botObjectiveKind = 'c';
	if(BotChiefGoal(bot, goal)) return true;
	botObjectiveKind = 'b';
	if(BotBossGoal(bot, goal)) return true;
	botObjectiveKind = 'p';
	f32 best = BOT_PIECE_SEARCH_RANGE;
	bool found = false;
	foreach_const(g, groundItems) {
		if(g->itemIndex != BOT_TITAN_PIECE_ITEM) continue;
		const f32 d = glm::length(vec2(g->pos) - here);
		if(d < best) { best = d; *goal = vec2(g->pos); found = true; }
	}
	return found;
}

bool World::BotItemGoal(const Player& bot, vec2* goal) const
{
	const vec2 here = vec2(bot.body->GetWorldPos());
	f32 best = MEASURED_BOT_ITEM_REACH;
	bool found = false;
	foreach_const(g, groundItems) {
		if(g->owner >= 0 || g->itemIndex == BOT_TITAN_PIECE_ITEM || (g->team >= 0 && g->team != (i32)bot.team)) continue;
		const f32 d = glm::length(vec2(g->pos) - here);
		if(d < best) { best = d; *goal = vec2(g->pos); found = true; }
	}
	return found;
}
static const u32 BOT_DECISION_MIN_MS = 400;
static const f32 HYP_BOT_OCCUPY_FIGHT_RANGE = 600.0f;
static const f32 HYP_BOT_TITAN_ENGAGE_RANGE = 1000.0f;
enum BotMode: u8 { BOT_MODE_FIGHT = 0, BOT_MODE_OCCUPY, BOT_MODE_OBJECTIVE, BOT_MODE_FARM, BOT_MODE_LANE, BOT_MODE_ROAM };
static const char* const BOT_MODE_NAMES[] = { "fight", "occupy", "objective", "farm", "lane", "roam" };
static const f32 HYP_BOT_LANE_FARM_RANGE = 1500.0f;
static const f32 HYP_BOT_RETREAT_HP = 0.25f;
static const f32 HYP_BOT_RETREAT_DONE_HP = 0.9f;
static const i32 HYP_BOT_SIEGE_TANKS = 2;
static const i32 HYP_BOT_SIEGE_GIANT_WEIGHT = 3;
static const f32 MEASURED_FIRST_MASTER_TOWER_HIT_SEC[] = { 113.0f, 125.0f, 171.0f, 205.0f, 207.0f, 209.0f, 235.0f };
static const i32 BOT_LANE_MIN_POINTS = 10;
static const f32 BOT_LANE_END_DIST = 3000.0f;
static const f32 BOT_LANE_SAME_DIST = 1500.0f;
static const i32 BOT_LANE_LOOKAHEAD = 2;
static const f32 HYP_BOT_WAVE_LANE_DIST = 800.0f;
static const f32 BOT_NAV_MERGE_DIST = 200.0f;
static const f32 BOT_NAV_LINK_RANGE = 1500.0f;
static const f32 BOT_NAV_REACHED = 200.0f;
static const f32 BOT_NAV_REROUTE_DIST = 500.0f;
static const f32 BOT_NAV_RAY_HEIGHT = 100.0f;
static const f32 BOT_NAV_RAY_SIDE = 40.0f;
static const f32 HYP_BOT_NAV_JUMP_RANGE = 400.0f;
static const f32 HYP_BOT_NAV_JUMP_MARGIN = 0.85f;
static const f32 HYP_BOT_NAV_JUMP_COST = 300.0f;
static const f32 HYP_BOT_NAV_LEDGE = 150.0f;
static const f32 HYP_BOT_NAV_JUMP_TRIGGER = 300.0f;

f32 World::BotJumpRise() const
{
	const f32 v = GetGlobalTweakableVars().jumpForce;
	return v * v / (2.0f * GetGlobalTweakableVars().gravity) * HYP_BOT_NAV_JUMP_MARGIN;
}

bool World::BotSees(const vec3& from, const vec3& to)
{
	const vec2 d = vec2(to) - vec2(from);
	const f32 len = glm::length(d);
	if(len < 1.0f) return true;
	foreach_const(it, actorNpcList) {
		if(!it->bodyBlocker || !it->monsterAi) continue;
		const vec2 c = vec2(it->pos) - vec2(from);
		const f32 along = MIN(MAX(glm::dot(c, d) / (len * len), 0.0f), 1.0f);
		if(glm::length(c - d * along) < it->monsterAi->bodyBlockRadius + PLAYER_BODY_RADIUS) return false;
	}
	const vec2 side = vec2(-d.y, d.x) / len * BOT_NAV_RAY_SIDE;
	for(i32 k = -1; k <= 1; k++) {
		const vec2 off = side * (f32)k;
		if(physics.RaycastStatic(vec3(vec2(from) + off, from.z + BOT_NAV_RAY_HEIGHT), vec3(vec2(to) + off, to.z + BOT_NAV_RAY_HEIGHT), nullptr)) return false;
	}
	return true;
}

bool World::BotWalkable(const vec3& from, const vec3& to)
{
	static const f32 HYP_BOT_NAV_WALK_SAMPLE = 50.0f;
	static const f32 HYP_BOT_NAV_GROUND_UP = 200.0f;
	static const f32 HYP_BOT_NAV_GROUND_DOWN = 600.0f;
	const f32 len = glm::length(vec2(to) - vec2(from));
	const i32 steps = MAX((i32)ceilf(len / HYP_BOT_NAV_WALK_SAMPLE), 1);
	const f32 rise = physics.MaxWalkRise(len / (f32)steps);
	f32 prevZ = 0;
	for(i32 k = 0; k <= steps; k++) {
		const vec3 at = glm::mix(from, to, (f32)k / (f32)steps);
		f32 down;
		if(!physics.RaycastStatic(at + vec3(0, 0, HYP_BOT_NAV_GROUND_UP), at - vec3(0, 0, HYP_BOT_NAV_GROUND_DOWN), &down)) return false;
		const f32 z = at.z + HYP_BOT_NAV_GROUND_UP - down;
		if(k > 0 && fabsf(z - prevZ) > rise) return false;
		prevZ = z;
	}
	return true;
}

void World::BuildBotNav()
{
	botNavBuilt = true;
	if(!map) return;
	const auto buildStart = std::chrono::steady_clock::now();
	BuildTitanSaveAreas();
	foreach_const(path, map->paths) {
		i32 prev = -1;
		foreach_const(pt, path->second) {
			i32 node = -1;
			for(i32 n = 0; n < (i32)botNav.size(); n++) {
				if(glm::length(vec2(botNav[n].pos) - vec2(*pt)) < BOT_NAV_MERGE_DIST) { node = n; break; }
			}
			if(node < 0) {
				node = (i32)botNav.size();
				botNav.push_back();
				botNav.back().pos = *pt;
			}
			if(prev >= 0 && prev != node && eastl::find(botNav[prev].links.begin(), botNav[prev].links.end(), (u16)node) == botNav[prev].links.end()) {
				botNav[prev].links.push_back((u16)node);
				botNav[node].links.push_back((u16)prev);
			}
			prev = node;
		}
	}
	const auto cells = GetGameXmlContent().measuredWalkCells.find((i32)map->index);
	if(cells != GetGameXmlContent().measuredWalkCells.end()) {
		foreach_const(c, cells->second) {
			if(InsideBodyBlocker(vec2(*c), PLAYER_BODY_RADIUS)) continue;
			botNav.push_back();
			botNav.back().pos = *c;
		}
	}
	for(i32 a = 0; a < (i32)botNav.size(); a++) {
		for(i32 b = a + 1; b < (i32)botNav.size(); b++) {
			if(glm::length(vec2(botNav[a].pos) - vec2(botNav[b].pos)) > BOT_NAV_LINK_RANGE) continue;
			if(eastl::find(botNav[a].links.begin(), botNav[a].links.end(), (u16)b) != botNav[a].links.end()) continue;
			if(!BotSees(botNav[a].pos, botNav[b].pos)) continue;
			if(fabsf(botNav[a].pos.z - botNav[b].pos.z) > physics.MaxWalkRise(0) && !BotWalkable(botNav[a].pos, botNav[b].pos)) continue;
			botNav[a].links.push_back((u16)b);
			botNav[b].links.push_back((u16)a);
		}
	}
	const f32 jumpRise = BotJumpRise();
	i32 jumps = 0;
	for(i32 a = 0; a < (i32)botNav.size(); a++) {
		for(i32 b = 0; b < (i32)botNav.size(); b++) {
			if(a == b || botNav[a].jumps.full()) continue;
			const vec3 from = botNav[a].pos;
			const vec3 to = botNav[b].pos;
			const f32 h = glm::length(vec2(to) - vec2(from));
			const f32 rise = to.z - from.z;
			if(h > HYP_BOT_NAV_JUMP_RANGE || rise <= physics.MaxWalkRise(h) || rise > jumpRise) continue;
			if(eastl::find(botNav[a].links.begin(), botNav[a].links.end(), (u16)b) != botNav[a].links.end()) continue;
			const f32 top = to.z + BOT_NAV_RAY_HEIGHT;
			if(physics.RaycastStatic(vec3(vec2(from), from.z + BOT_NAV_RAY_HEIGHT), vec3(vec2(from), top), nullptr)) continue;
			if(physics.RaycastStatic(vec3(vec2(from), top), vec3(vec2(to), top), nullptr)) continue;
			botNav[a].jumps.push_back((u16)b);
			jumps++;
		}
	}
	LOG("Bot :: nav %d jump links (rise up to %.0f)", jumps, jumpRise);
	for(i32 team = 0; team < 2; team++) {
		if(!titanSaveAreaKnown[team] || !titanSaveAreaKnown[1 - team]) continue;
		foreach_const(path, map->paths) {
			const auto& pts = path->second;
			if((i32)pts.size() < BOT_LANE_MIN_POINTS) continue;
			if(glm::length(vec2(pts.front()) - titanSaveAreas[team]) > BOT_LANE_END_DIST || glm::length(vec2(pts.back()) - titanSaveAreas[1 - team]) > BOT_LANE_END_DIST) continue;
			const vec2 mid = vec2(pts[pts.size() / 2]);
			bool same = false;
			foreach_const(lane, botLanes[team]) {
				if(glm::length((*lane)[lane->size() / 2] - mid) < BOT_LANE_SAME_DIST) same = true;
			}
			if(same) continue;
			botLanes[team].push_back();
			foreach_const(pt, pts) botLanes[team].back().push_back(vec2(*pt));
		}
	}
	i32 links = 0;
	foreach_const(n, botNav) links += (i32)n->links.size();
	LOG("Bot :: nav %d nodes, %d links, lanes %d/%d, built in %lld ms", (i32)botNav.size(), links / 2, (i32)botLanes[0].size(), (i32)botLanes[1].size(), (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - buildStart).count());
}

void World::BotPlanRoute(Player& bot, const vec2& goal)
{
	bot.botRoute.clear();
	bot.botRouteGoal = goal;
	bot.botRouteCost = 0;
	const i32 count = (i32)botNav.size();
	if(count == 0) return;
	const vec3 pos = bot.body->GetWorldPos();
	const vec3 goal3 = vec3(goal, pos.z);
	eastl::vector<f32> dist(count, 1e30f);
	eastl::vector<i32> prev(count, -1);
	eastl::vector<bool> done(count, false);
	i32 nearest = 0;
	f32 nearestDist = 1e30f;
	bool seen = false;
	for(i32 n = 0; n < count; n++) {
		const f32 d = glm::length(vec2(botNav[n].pos) - vec2(pos));
		if(d < nearestDist) { nearestDist = d; nearest = n; }
		if(d <= BOT_NAV_LINK_RANGE && BotSees(pos, botNav[n].pos) && BotWalkable(pos, botNav[n].pos) && (localTime >= bot.botRouteForceUntil || BotCanStepToward(bot, vec2(botNav[n].pos)))) { dist[n] = d; seen = true; }
	}
	if(!seen) dist[nearest] = nearestDist;
	for(;;) {
		i32 cur = -1;
		for(i32 n = 0; n < count; n++) {
			if(!done[n] && dist[n] < 1e29f && (cur < 0 || dist[n] < dist[cur])) cur = n;
		}
		if(cur < 0) break;
		done[cur] = true;
		foreach_const(l, botNav[cur].links) {
			const f32 d = dist[cur] + glm::length(vec2(botNav[*l].pos) - vec2(botNav[cur].pos));
			if(d < dist[*l]) { dist[*l] = d; prev[*l] = cur; }
		}
		if(bot.clientHd == ClientHandle::INVALID) foreach_const(l, botNav[cur].jumps) {
			const f32 d = dist[cur] + glm::length(vec2(botNav[*l].pos) - vec2(botNav[cur].pos)) + HYP_BOT_NAV_JUMP_COST;
			if(d < dist[*l]) { dist[*l] = d; prev[*l] = cur; }
		}
	}
	i32 last = -1;
	f32 lastCost = 1e30f;
	i32 fallback = -1;
	f32 fallbackDist = 1e30f;
	for(i32 n = 0; n < count; n++) {
		if(dist[n] >= 1e29f) continue;
		const f32 d = glm::length(vec2(botNav[n].pos) - goal);
		if(d < fallbackDist) { fallbackDist = d; fallback = n; }
		if(d > BOT_NAV_LINK_RANGE || dist[n] + d >= lastCost || BotPassesStuckSpot(bot, vec2(botNav[n].pos), goal) || !BotSees(botNav[n].pos, goal3) || !BotWalkable(botNav[n].pos, goal3)) continue;
		lastCost = dist[n] + d;
		last = n;
	}
	if(last < 0) last = fallback;
	if(last >= 0) bot.botRouteCost = last == fallback && lastCost >= 1e29f ? dist[last] + fallbackDist : lastCost;
	eastl::fixed_vector<u16,256,true> reversed;
	for(i32 n = last; n >= 0; n = prev[n]) reversed.push_back((u16)n);
	for(i32 k = (i32)reversed.size() - 1; k >= 0 && !bot.botRoute.full(); k--) bot.botRoute.push_back(reversed[k]);
}

void World::PlanNavRoute(const vec3& from, const vec3& to, eastl::fixed_vector<u16,24,false>& out)
{
	out.clear();
	const i32 count = (i32)botNav.size();
	if(count == 0) return;
	eastl::vector<f32> dist(count, 1e30f);
	eastl::vector<i32> prev(count, -1);
	eastl::vector<bool> done(count, false);
	for(i32 n = 0; n < count; n++) {
		const f32 d = glm::length(vec2(botNav[n].pos) - vec2(from));
		if(d <= BOT_NAV_LINK_RANGE && BotSees(from, botNav[n].pos) && BotWalkable(from, botNav[n].pos)) dist[n] = d;
	}
	for(;;) {
		i32 cur = -1;
		for(i32 n = 0; n < count; n++) {
			if(!done[n] && dist[n] < 1e29f && (cur < 0 || dist[n] < dist[cur])) cur = n;
		}
		if(cur < 0) break;
		done[cur] = true;
		foreach_const(l, botNav[cur].links) {
			const f32 d = dist[cur] + glm::length(vec2(botNav[*l].pos) - vec2(botNav[cur].pos));
			if(d < dist[*l]) { dist[*l] = d; prev[*l] = cur; }
		}
	}
	i32 last = -1;
	f32 lastCost = 1e30f;
	for(i32 n = 0; n < count; n++) {
		if(dist[n] >= 1e29f) continue;
		const f32 d = glm::length(vec2(botNav[n].pos) - vec2(to));
		if(d > BOT_NAV_LINK_RANGE || dist[n] + d >= lastCost || !BotSees(botNav[n].pos, to) || !BotWalkable(botNav[n].pos, to)) continue;
		lastCost = dist[n] + d;
		last = n;
	}
	eastl::fixed_vector<u16,64,true> reversed;
	for(i32 n = last; n >= 0; n = prev[n]) reversed.push_back((u16)n);
	for(i32 k = (i32)reversed.size() - 1; k >= 0 && !out.full(); k--) out.push_back(reversed[k]);
}

bool World::BotShotClear(const Player& bot, const vec3& aim, f32 bodyRadius)
{
	const vec3 pos = bot.body->GetWorldPos();
	const f32 z = pos.z + BodyMidHeight(bot);
	f32 dist = 0;
	const PxRigidActor* wall = nullptr;
	ActorDynamic* gate = nullptr;
	if(!RaycastShot(vec3(vec2(pos), z), vec3(vec2(aim), z), (i32)bot.team, &dist, &wall, &gate)) return true;
	return !gate && dist + 1.0f >= glm::length(vec2(aim) - vec2(pos)) - bodyRadius;
}

bool World::BotCanStepToward(Player& bot, const vec2& to)
{
	static const f32 HYP_BOT_STEP_PROBE = 100.0f;
	static const f32 HYP_BOT_STEP_MIN_PROGRESS = 0.5f;
	const vec2 here = vec2(bot.body->GetWorldPos());
	const vec2 d = to - here;
	const f32 len = glm::length(d);
	if(len < 1.0f) return true;
	const f32 probe = MIN(len, HYP_BOT_STEP_PROBE);
	const vec2 dir = d / len;
	const vec3 reach = physics.FindMovePos(bot.body, vec3(dir * probe, 0), 0.3f);
	return glm::dot(vec2(reach) - here, dir) >= probe * HYP_BOT_STEP_MIN_PROGRESS;
}

bool World::BotPassesStuckSpot(const Player& bot, const vec2& from, const vec2& to) const
{
	static const f32 HYP_BOT_STUCK_SPOT_RADIUS = 120.0f;
	return localTime < bot.botStuckAvoidUntil && SegmentPointDist2D(from, to, bot.botStuckPos) < HYP_BOT_STUCK_SPOT_RADIUS;
}

vec2 World::BotNavStep(Player& bot, const vec2& goal)
{
	if(!botNavBuilt) BuildBotNav();
	const vec3 pos = bot.body->GetWorldPos();
	const vec2 here = vec2(pos);
	if(botNav.empty() || (localTime >= bot.botRouteForceUntil && !BotPassesStuckSpot(bot, here, goal) && BotSees(pos, vec3(goal, pos.z)) && BotWalkable(pos, vec3(goal, pos.z)))) {
		bot.botRoute.clear();
		return goal;
	}
	if(bot.botRoute.empty() || glm::length(goal - bot.botRouteGoal) > BOT_NAV_REROUTE_DIST) {
		BotPlanRoute(bot, goal);
		if(localTime < bot.botRouteForceUntil) LOG("BotNav :: %ls forced route to (%.0f, %.0f): %d nodes, first (%.0f, %.0f)", bot.name.data(), goal.x, goal.y, (i32)bot.botRoute.size(), bot.botRoute.empty() ? 0.0f : botNav[bot.botRoute.front()].pos.x, bot.botRoute.empty() ? 0.0f : botNav[bot.botRoute.front()].pos.y);
	}
	static const f32 HYP_BOT_NAV_REACHED_STUCK = 60.0f;
	const f32 reached = localTime < bot.botRouteForceUntil ? HYP_BOT_NAV_REACHED_STUCK : BOT_NAV_REACHED;
	while(!bot.botRoute.empty() && ((glm::length(vec2(botNav[bot.botRoute.front()].pos) - here) < reached && botNav[bot.botRoute.front()].pos.z - pos.z < HYP_BOT_NAV_LEDGE) || InsideBodyBlocker(vec2(botNav[bot.botRoute.front()].pos), PLAYER_BODY_RADIUS))) bot.botRoute.erase(bot.botRoute.begin());
	if(!bot.botRoute.empty() && bot.clientHd == ClientHandle::INVALID && bot.body->grounded) {
		const vec3 next = botNav[bot.botRoute.front()].pos;
		if(next.z - pos.z >= HYP_BOT_NAV_LEDGE && glm::length(vec2(next) - here) < HYP_BOT_NAV_JUMP_TRIGGER) bot.input.jump = 1;
	}
	if(bot.botRoute.size() > 1 && BotSees(pos, botNav[bot.botRoute[1]].pos) && BotWalkable(pos, botNav[bot.botRoute[1]].pos)) bot.botRoute.erase(bot.botRoute.begin());
	if(bot.botRoute.empty()) return goal;
	return vec2(botNav[bot.botRoute.front()].pos);
}

bool World::BotRetreat(Player& bot, vec2* goal)
{
	if(bot.devGod) {
		bot.botRetreat = false;
		return false;
	}
	BuildTitanSaveAreas();
	if(!titanSaveAreaKnown[bot.team]) return false;
	const f32 ratio = HpRatio(bot.Main().hp, bot.Main().hpMax);
	if(ratio < HYP_BOT_RETREAT_HP) bot.botRetreat = true;
	else if(ratio >= HYP_BOT_RETREAT_DONE_HP) bot.botRetreat = false;
	if(!bot.botRetreat || bot.titanForm >= 0) return false;
	*goal = titanHealAreaKnown[bot.team] ? titanHealAreas[bot.team] : vec2(bot.spawnPos);
	return true;
}

bool World::BotLaneGoal(Player& bot, vec2* goal)
{
	if(!botNavBuilt) BuildBotNav();
	const auto& lanes = botLanes[bot.team];
	if(lanes.empty()) return false;
	if(bot.botLane < 0) {
		i32 rank = 0;
		foreach_const(p, players) {
			if(p->team == bot.team && p->index < bot.index) rank++;
		}
		bot.botLane = (i8)(rank % (i32)lanes.size());
	}
	const auto& lane = lanes[bot.botLane % (i32)lanes.size()];
	const vec2 here = vec2(bot.body->GetWorldPos());
	i32 nearest = 0;
	f32 nearestDist = 1e30f;
	for(i32 k = 0; k < (i32)lane.size(); k++) {
		const f32 d = glm::length(lane[k] - here);
		if(d < nearestDist) { nearestDist = d; nearest = k; }
	}
	static const u32 minionTag = StatusTypeKey("Minion");
	i32 front = -1;
	foreach_const(npc, actorNpcList) {
		if(!npc->monsterAi || npc->monsterAi->tagKey != minionTag || npc->hp <= 0 || npc->removeAt != Time::ZERO || (i32)npc->faction != (i32)bot.team) continue;
		i32 at = -1;
		f32 atDist = HYP_BOT_WAVE_LANE_DIST;
		for(i32 k = 0; k < (i32)lane.size(); k++) {
			const f32 d = glm::length(lane[k] - vec2(npc->pos));
			if(d < atDist) { atDist = d; at = k; }
		}
		front = MAX(front, at);
	}
	if(front < 0) front = (i32)lane.size() / 2;
	i32 next = MIN(nearest + BOT_LANE_LOOKAHEAD, front);
	while(next < front && InsideBodyBlocker(lane[next], PLAYER_BODY_RADIUS + BOT_NAV_REACHED)) next++;
	*goal = lane[next];
	return true;
}

void World::RollBotSiegeFrom()
{
	const f32 sec = MEASURED_FIRST_MASTER_TOWER_HIT_SEC[RandUint() % ARRAY_COUNT(MEASURED_FIRST_MASTER_TOWER_HIT_SEC)];
	botSiegeFrom = TimeAdd(localTime, TimeMsToTime(sec * 1000.0));
	LOG("BotSiege :: bots may siege towers from %.0f s", sec);
}

bool World::BotSiegeOk(const Player& bot, const ActorNpc& tower) const
{
	if(!map || !map->ingameLevels || !IsStructure(tower)) return false;
	if(botSiegeFrom == Time::ZERO || localTime < botSiegeFrom) return false;
	if(bot.titanForm >= 0) return true;
	const GameXmlContent::MonsterAttack* attack = tower.monsterAi->byTarget[GameXmlContent::MonsterAttack::MINION];
	if(!attack) attack = tower.monsterAi->byTarget[GameXmlContent::MonsterAttack::MASTER];
	if(!attack || attack->range <= 0) return false;
	const auto& giants = GetGameXmlContent().titan.giants;
	i32 tanks = 0;
	foreach_const(npc, actorNpcList) {
		if(!npc->monsterAi || npc->ownerUID != ActorUID::INVALID || npc->faction != (Faction)bot.team || npc->hp <= 0 || npc->removeAt != Time::ZERO || IsStructure(*npc)) continue;
		if(glm::length(vec2(npc->pos) - vec2(tower.pos)) > attack->range) continue;
		tanks += eastl::find(giants.begin(), giants.end(), (i32)npc->docID) != giants.end() ? HYP_BOT_SIEGE_GIANT_WEIGHT : 1;
		if(tanks >= HYP_BOT_SIEGE_TANKS) return true;
	}
	return false;
}
static const u32 BOT_DECISION_SPAN_MS = 500;
static const u32 BOT_STUCK_CHECK_MS = 500;
static const f32 BOT_TOWER_MARGIN = 200.0f;

static vec2 RotateVec(const vec2& v, f32 a)
{
	return vec2(v.x * cosf(a) - v.y * sinf(a), v.x * sinf(a) + v.y * cosf(a));
}

bool World::BotDangerAt(const Player& bot, const vec2& at, vec2* towerPos, f32* towerRange) const
{
	foreach_const(npc, actorNpcList) {
		if(!npc->monsterAi || npc->monsterAi->run > 0 || npc->hp <= 0 || npc->removeAt != Time::ZERO) continue;
		if(npc->faction != Faction::RED && npc->faction != Faction::BLUE) continue;
		if(npc->faction == (Faction)bot.team) continue;
		const GameXmlContent::MonsterAttack* attack = npc->monsterAi->byTarget[GameXmlContent::MonsterAttack::MASTER];
		if(!attack) continue;
		const f32 range = attack->range + BOT_TOWER_MARGIN;
		if(glm::length(at - vec2(npc->pos)) >= range) continue;
		if(BotSiegeOk(bot, *npc)) continue;
		if(towerPos) *towerPos = vec2(npc->pos);
		if(towerRange) *towerRange = range;
		return true;
	}
	return false;
}

void World::UpdateBotMovement()
{
	if(Config().DevShowcase) return;
	foreach(it, players) {
		Player& bot = *it;
		if(bot.clientHd != ClientHandle::INVALID || bot.dead || bot.Main().classType == ClassType::NONE) continue;
		const char* held = localTime < bot.ccUntil ? "cc" : localTime < bot.dashUntil ? "dash" : bot.casting.skillID != SkillID::INVALID ? "casting" : AttachedToCaster(bot) ? "attached" : bot.interactSlot >= 0 ? "interaction" : localTime < bot.botSkillStillUntil ? "skill still" : nullptr;
		if(held) {
			if(bot.botHeldSince == Time::ZERO) bot.botHeldSince = localTime;
			if(TimeDiffSec(TimeDiff(bot.botHeldSince, localTime)) > 10.0 && localTime >= bot.botIdleLogAt) {
				bot.botIdleLogAt = TimeAdd(localTime, TimeMsToTime(10000.0));
				LOG("BotHeld :: %ls held by %s for %.0fs (skill %d, slot %d, cc %.1fs, dash %.1fs, still %.1fs)", bot.name.data(), held, TimeDiffSec(TimeDiff(bot.botHeldSince, localTime)), (i32)bot.casting.skillID, (i32)bot.interactSlot,
					localTime < bot.ccUntil ? TimeDiffSec(TimeDiff(localTime, bot.ccUntil)) : 0.0, localTime < bot.dashUntil ? TimeDiffSec(TimeDiff(localTime, bot.dashUntil)) : 0.0, localTime < bot.botSkillStillUntil ? TimeDiffSec(TimeDiff(localTime, bot.botSkillStillUntil)) : 0.0);
			}
			continue;
		}
		bot.botHeldSince = Time::ZERO;
		const vec3 pos = bot.body->GetWorldPos();
		const vec2 here = vec2(pos);
		bool stuck = false;
		if(localTime >= bot.botStuckCheck) {
			stuck = bot.botStuckCheck != Time::ZERO && glm::length(here - bot.botLastPos) < BOT_STUCK_DIST && glm::length(vec2(bot.input.moveTo) - here) > BOT_REPATH_DIST;
			bot.botStillChecks = bot.botStuckCheck != Time::ZERO && glm::length(here - bot.botLastPos) < BOT_STUCK_DIST ? bot.botStillChecks + 1 : 0;
			if((f64)bot.botStillChecks * BOT_STUCK_CHECK_MS > 15000.0 && localTime >= bot.botIdleLogAt && localTime >= bot.botAttackStateUntil) {
				bot.botIdleLogAt = TimeAdd(localTime, TimeMsToTime(10000.0));
				LOG("BotStill :: %ls at (%.0f, %.0f, %.0f) mode %s moveTo %.0f away speed %.0f lock %.1fs stuck %d grounded %d", bot.name.data(), pos.x, pos.y, pos.z, bot.botMode < 6 ? BOT_MODE_NAMES[bot.botMode] : "-", glm::length(vec2(bot.input.moveTo) - here), bot.input.speed,
					bot.body->lockedMoveUntil > localTime ? TimeDiffSec(TimeDiff(localTime, bot.body->lockedMoveUntil)) : 0.0, stuck ? 1 : 0, bot.body->grounded ? 1 : 0);
			}
			bot.botLastPos = here;
			bot.botStuckCheck = TimeAdd(localTime, TimeMsToTime(BOT_STUCK_CHECK_MS));
		}
		const bool arrived = glm::length(vec2(bot.input.moveTo) - here) < BOT_REPATH_DIST;
		vec2 zoneDir;
		f32 zoneDeep = 0;
		if(!stuck && !arrived && localTime < bot.botMoveAt && !BotEvadeZone(bot, &zoneDir, &zoneDeep)) continue;
		bot.botMoveAt = TimeAdd(localTime, TimeMsToTime(BOT_DECISION_MIN_MS + RandUint() % BOT_DECISION_SPAN_MS));
		if(stuck || RandUint() % 6 == 0) bot.botStrafe = -bot.botStrafe;

		const Player* target = nullptr;
		f32 best = 1e9f;
		foreach_const(e, players) {
			if(e->dead || e->team == bot.team || e->untargetable || e->Main().classType == ClassType::NONE || !(e->fogSeenBy & (1u << bot.team))) continue;
			if(BotDangerAt(bot, vec2(e->body->GetWorldPos()), nullptr, nullptr)) continue;
			const f32 d = glm::length(vec2(e->body->GetWorldPos()) - here);
			if(d < best) { best = d; target = &*e; }
		}
		vec2 goal;
		vec2 occupyPoint;
		ActorUID occupyObject = ActorUID::INVALID;
		f32 occupyReach = 0;
		const bool occupyGoal = OccupyTarget(bot, &occupyPoint, &occupyObject, &occupyReach);
		if(occupyGoal && glm::length(occupyPoint - here) <= occupyReach && (!target || best > HYP_BOT_OCCUPY_FIGHT_RANGE)) {
			bot.input.moveTo = pos;
			StartMapInteraction(bot, occupyObject);
			if(bot.interactSlot < 0 && localTime >= bot.botIdleLogAt) {
				bot.botIdleLogAt = TimeAdd(localTime, TimeMsToTime(10000.0));
				LOG("BotIdle :: %ls at (%.0f, %.0f) %.0f from occupy point (%.0f, %.0f), interaction not started", bot.name.data(), here.x, here.y, glm::length(occupyPoint - here), occupyPoint.x, occupyPoint.y);
			}
			continue;
		}
		const bool titanNav = map && map->ingameLevels;
		vec2 objective;
		const bool retreatGoal = titanNav && BotRetreat(bot, &objective);
		const bool itemGoal = titanNav && BotItemGoal(bot, &objective);
		const bool objectiveGoal = itemGoal || retreatGoal || (titanNav && BotObjective(bot, &objective));
		bot.botGoalKind = itemGoal ? 'i' : retreatGoal ? 'r' : objectiveGoal ? botObjectiveKind : '-';
		const f32 engage = occupyGoal || objectiveGoal ? HYP_BOT_OCCUPY_FIGHT_RANGE : (titanNav ? HYP_BOT_TITAN_ENGAGE_RANGE : BOT_ENGAGE_RANGE);
		bool fighting = false;
		bool farm = false;
		u8 mode = BOT_MODE_ROAM;
		if(target && best <= engage) {
			fighting = true;
			mode = BOT_MODE_FIGHT;
			const vec2 tp = vec2(target->body->GetWorldPos());
			const f32 range = WeaponIsMelee(bot) ? BOT_MELEE_RANGE : MIN(WeaponRange(bot) * BOT_RANGE_FRACTION, BOT_RANGED_MAX);
			const vec2 from = here - tp;
			const f32 a = atan2f(from.y, from.x) + bot.botStrafe * BOT_STRAFE_STEP;
			goal = tp + vec2(cosf(a), sinf(a)) * range;
			const f32 face = atan2f(tp.y - here.y, tp.x - here.x);
			bot.input.rot.upperYaw = face;
			bot.input.rot.bodyYaw = face;
			bot.input.rot.upperPitch = 0;
		}
		else if(occupyGoal) {
			static const f32 HYP_BOT_OCCUPY_PROGRESS_SEC = 8.0f;
			static const f32 HYP_BOT_OCCUPY_PROGRESS_DIST = 300.0f;
			static const f32 HYP_BOT_OCCUPY_SKIP_SEC = 30.0f;
			const vec2 d = occupyPoint - here;
			const f32 len = glm::length(d);
			if(occupyObject != bot.botFarmUID) {
				bot.botFarmUID = occupyObject;
				bot.botFarmCheckAt = TimeAdd(localTime, TimeMsToTime(HYP_BOT_OCCUPY_PROGRESS_SEC * 1000.0));
				bot.botFarmCheckDist = len;
			}
			else if(localTime >= bot.botFarmCheckAt) {
				if(len > bot.botFarmCheckDist - HYP_BOT_OCCUPY_PROGRESS_DIST) {
					LOG("BotOccupy :: %ls gives up object at (%.0f, %.0f), %.0f away (was %.0f)", bot.name.data(), occupyPoint.x, occupyPoint.y, len, bot.botFarmCheckDist);
					if(bot.botFarmSkips.full()) bot.botFarmSkips.erase(bot.botFarmSkips.begin());
					bot.botFarmSkips.push_back(eastl::make_pair(occupyObject, TimeAdd(localTime, TimeMsToTime(HYP_BOT_OCCUPY_SKIP_SEC * 1000.0))));
					bot.botFarmUID = ActorUID::INVALID;
				}
				else {
					bot.botFarmCheckAt = TimeAdd(localTime, TimeMsToTime(HYP_BOT_OCCUPY_PROGRESS_SEC * 1000.0));
					bot.botFarmCheckDist = len;
				}
			}
			goal = len > BOT_ROAM_STEP ? here + d / len * BOT_ROAM_STEP : occupyPoint;
			mode = BOT_MODE_OCCUPY;
		}
		else if(objectiveGoal) {
			goal = objective;
			mode = BOT_MODE_OBJECTIVE;
		}
		else if(titanNav && !(farm = BotFarmTarget(bot, &goal, HYP_BOT_LANE_FARM_RANGE)) && BotLaneGoal(bot, &goal)) mode = BOT_MODE_LANE;
		else if(!titanNav && BotObjective(bot, &goal)) {
			const vec2 d = goal - here;
			const f32 len = glm::length(d);
			if(len > BOT_ROAM_STEP) goal = here + d / len * BOT_ROAM_STEP;
		}
		else if(titanNav ? farm : BotFarmTarget(bot, &goal, BOT_FARM_RANGE)) {
			const f32 range = WeaponIsMelee(bot) ? BOT_MELEE_RANGE : MIN(WeaponRange(bot) * BOT_RANGE_FRACTION, BOT_RANGED_MAX);
			const vec2 d = here - goal;
			const f32 len = glm::length(d);
			const f32 face = atan2f(goal.y - here.y, goal.x - here.x);
			bot.input.rot.upperYaw = face;
			bot.input.rot.bodyYaw = face;
			goal = goal + (len > 1.0f ? d / len : vec2(1, 0)) * range;
			mode = BOT_MODE_FARM;
		}
		else if(target) {
			const vec2 toward = glm::normalize(vec2(target->body->GetWorldPos()) - here);
			goal = here + RotateVec(toward, bot.botStrafe * 0.3f) * BOT_ROAM_STEP;
		}
		else {
			goal = here + vec2(cosf(bot.input.rot.bodyYaw), sinf(bot.input.rot.bodyYaw)) * BOT_ROAM_STEP;
		}
		static const f32 MEASURED_PLAYER_JUMPS_PER_MIN = 0.43f;
		if(bot.botJumpRollAt == Time::ZERO) bot.botJumpRollAt = localTime;
		if(titanNav && bot.body->grounded) {
			const f32 sinceRollSec = (f32)TimeDiffSec(TimeDiff(bot.botJumpRollAt, localTime));
			bot.botJumpRollAt = localTime;
			if((f32)Randf01() < MEASURED_PLAYER_JUMPS_PER_MIN * sinceRollSec / 60.0f) bot.input.jump = 1;
		}
		if(titanNav && mode != bot.botMode) {
			const ActorNpc* farmNpc = mode == BOT_MODE_FARM ? FindNpcActor(bot.botFarmUID) : nullptr;
			LOG("BotMode :: %ls %s at (%.0f, %.0f) hp %d/%d pieces %d farm creature=%d at (%.0f, %.0f)", bot.name.data(), BOT_MODE_NAMES[mode], here.x, here.y, bot.Main().hp, bot.Main().hpMax, bot.titanPieces,
				farmNpc ? (i32)farmNpc->docID : 0, farmNpc ? farmNpc->pos.x : 0.0f, farmNpc ? farmNpc->pos.y : 0.0f);
			bot.botMode = mode;
		}
		const vec2 preNavGoal = goal;
		if(titanNav && !fighting) {
			goal = BotNavStep(bot, goal);
			const vec2 d = goal - here;
			const f32 len = glm::length(d);
			if(len > BOT_ROAM_STEP) goal = here + d / len * BOT_ROAM_STEP;
		}
		if(stuck) {
			static const i32 HYP_BOT_STUCK_ROUTE_CHECKS = 6;
			static const f64 HYP_BOT_STUCK_ROUTE_MS = 5000.0;
			if(titanNav && bot.botStillChecks >= HYP_BOT_STUCK_ROUTE_CHECKS && localTime >= bot.botRouteForceUntil) {
				static const f64 HYP_BOT_STUCK_SPOT_MS = 20000.0;
				bot.botRouteForceUntil = TimeAdd(localTime, TimeMsToTime(HYP_BOT_STUCK_ROUTE_MS));
				static const f32 HYP_BOT_TRAP_SAME_SPOT = 300.0f;
				bot.botStuckEpisodes = localTime < bot.botStuckAvoidUntil && glm::length(here - bot.botStuckPos) < HYP_BOT_TRAP_SAME_SPOT ? bot.botStuckEpisodes + 1 : 1;
				bot.botStuckAvoidUntil = TimeAdd(localTime, TimeMsToTime(HYP_BOT_STUCK_SPOT_MS));
				bot.botStuckPos = here;
				eastl::fixed_string<char,128,false> free;
				for(i32 k = 0; k < 8; k++) {
					const vec2 d8 = vec2(cosf((f32)k * PI / 4.0f), sinf((f32)k * PI / 4.0f));
					const vec3 reach = physics.FindMovePos(bot.body, vec3(d8 * 200.0f, 0), 0.3f);
					free.append_sprintf(" %.0f", glm::dot(vec2(reach) - here, d8));
				}
				LOG("BotNav :: %ls stuck at (%.0f, %.0f, %.0f), follows the walk graph, free%s, route %d", bot.name.data(), here.x, here.y, pos.z, free.c_str(), (i32)bot.botRoute.size());
			}
			bot.botRoute.clear();
			const vec2 d = goal - here;
			const f32 len = glm::length(d);
			goal = here + RotateVec(len > 1.0f ? d / len : vec2(1, 0), bot.botStrafe * PI * 0.6f) * BOT_ROAM_STEP;
		}
		vec2 tower;
		f32 towerRange = 0;
		bool dangerHeld = false;
		if(BotDangerAt(bot, goal, &tower, &towerRange)) {
			const vec2 away = here - tower;
			const f32 len = glm::length(away);
			goal = tower + (len > 1.0f ? away / len : vec2(1, 0)) * towerRange;
			static const f32 HYP_BOT_DANGER_HOLD_DIST = 60.0f;
			dangerHeld = mode == BOT_MODE_OBJECTIVE && (bot.botGoalKind == 'b' || bot.botGoalKind == 'c') && glm::length(goal - here) < HYP_BOT_DANGER_HOLD_DIST;
		}
		if(!dangerHeld) bot.botDangerHoldSince = Time::ZERO;
		else if(bot.botDangerHoldSince == Time::ZERO) bot.botDangerHoldSince = localTime;
		else {
			static const f64 HYP_BOT_DANGER_HOLD_SEC = 5.0;
			static const f64 HYP_BOT_BOSS_SKIP_SEC = 30.0;
			if(TimeDiffSec(TimeDiff(bot.botDangerHoldSince, localTime)) > HYP_BOT_DANGER_HOLD_SEC && botLastBossUID != ActorUID::INVALID) {
				if(bot.botFarmSkips.full()) bot.botFarmSkips.erase(bot.botFarmSkips.begin());
				bot.botFarmSkips.push_back(eastl::make_pair(botLastBossUID, TimeAdd(localTime, TimeMsToTime(HYP_BOT_BOSS_SKIP_SEC * 1000.0))));
				LOG("BotNav :: %ls held at a tower edge on the way to a boss, skips it for %.0f s", bot.name.data(), HYP_BOT_BOSS_SKIP_SEC);
				bot.botDangerHoldSince = Time::ZERO;
			}
		}
		static const f32 HYP_BOT_EVADE_ZONE_MARGIN = 150.0f;
		vec2 zoneAway = vec2(0);
		f32 zoneDepth = 0;
		if(BotEvadeZone(bot, &zoneAway, &zoneDepth)) goal = here + zoneAway * (zoneDepth + HYP_BOT_EVADE_ZONE_MARGIN);
		const vec2 rawGoal = goal;
		BotGateBlocks(bot, here, &goal);
		BotKeepOutOfBlockers(bot, &goal);
		if(glm::length(goal - here) < BOT_REPATH_DIST && !fighting && localTime >= bot.botIdleLogAt) {
			bot.botIdleLogAt = TimeAdd(localTime, TimeMsToTime(10000.0));
			LOG("BotIdle :: %ls at (%.0f, %.0f) mode %s goal (%.0f, %.0f) raw (%.0f, %.0f) kind %c target (%.0f, %.0f) route %d next (%.0f, %.0f, %.0f) z %.0f", bot.name.data(), here.x, here.y, BOT_MODE_NAMES[mode], goal.x, goal.y, rawGoal.x, rawGoal.y, bot.botGoalKind, preNavGoal.x, preNavGoal.y, (i32)bot.botRoute.size(), bot.botRoute.empty() ? 0.0f : botNav[bot.botRoute.front()].pos.x, bot.botRoute.empty() ? 0.0f : botNav[bot.botRoute.front()].pos.y, bot.botRoute.empty() ? 0.0f : botNav[bot.botRoute.front()].pos.z, pos.z);
		}
		bot.input.moveTo = vec3(goal, pos.z);
		bot.input.speed = PlayerMoveSpeed(bot);
	}
}

void World::PilotTourEvent(const char* what)
{
	FILE* f = fopen("pilot_tour.jsonl", "ab");
	if(!f) return;
	const i64 wall = (i64)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	u32 local = 0;
	foreach_const(p, players) if(p->clientHd != ClientHandle::INVALID && !p->removed && pilotTour.npc != ActorUID::INVALID) local = (u32)replication->GetLocalActorID(p->clientHd, pilotTour.npc);
	fprintf(f, "{\"w\":%lld,\"kind\":%d,\"id\":%d,\"local\":%u,\"warped\":%d,\"what\":\"%s\"}\n", wall, (i32)pilotTour.kind, pilotTour.kind == 1 ? pilotTour.localID : pilotTour.doc, local, pilotTour.warped ? 1 : 0, what);
	fclose(f);
}

void World::PilotTourFinish(const char* result)
{
	if(pilotTour.kind) PilotTourEvent(result);
	if(pilotTour.kind == 1) pilotTourInteractions.push_back(pilotTour.localID);
	if(pilotTour.kind == 2) pilotTourNpcs.push_back(pilotTour.npc);
	if(pilotTour.kind) LOG("PilotTour :: %s %d %s after %.1f s%s", pilotTour.kind == 1 ? "interaction" : "monster", pilotTour.kind == 1 ? pilotTour.localID : pilotTour.doc, result, TimeDiffSec(TimeDiff(pilotTour.since, localTime)), pilotTour.warped ? " (warped)" : "");
	pilotTour = PilotTour();
}

bool World::PilotTourStep(Player& p, PilotDecision* out, vec2* goal)
{
	static const f64 HYP_TOUR_STOP_TIMEOUT_SEC = 150.0;
	static const f32 HYP_TOUR_INTERACT_REACH = 500.0f;
	const vec2 here = vec2(p.body->GetWorldPos());
	if(pilotTour.kind && TimeDiffSec(TimeDiff(pilotTour.since, localTime)) > HYP_TOUR_STOP_TIMEOUT_SEC) PilotTourFinish(pilotTour.kind == 2 && pilotTour.reacted ? "FAILED (reacted but not killed)" : (pilotTour.kind == 2 && pilotTour.hit ? "FAILED (hit but never targeted the player)" : "FAILED (not reached or not completed)"));
	if(pilotTour.kind == 1) {
		bool still = false;
		foreach_const(m, mapInteractions) if(m->localID == pilotTour.localID && m->active) still = true;
		if(!still) PilotTourFinish("skipped (inactive)");
	}
	if(pilotTour.kind == 2) {
		ActorNpc* npc = FindNpcActor(pilotTour.npc);
		if(!npc || npc->hp <= 0) PilotTourFinish(!pilotTour.hit ? "skipped (gone)" : (pilotTour.reacted ? "done (reacted, killed)" : "FAILED (killed but never targeted the player)"));
		else if(pilotTour.hit && !pilotTour.reacted && npc->monsterTarget == p.Main().UID) {
			pilotTour.reacted = true;
			PilotTourEvent("reacted");
		}
	}
	if(!pilotTour.kind) {
		f32 best = 1e12f;
		foreach_const(m, mapInteractions) {
			const GameXmlContent::InteractionInfo* info = GetGameXmlContent().FindInteraction(m->interactionID);
			if(!m->active || m->creature || !info || info->pvpOccupy || info->action == GameXmlContent::InteractionInfo::OCCUPY) continue;
			if(eastl::find(pilotTourInteractions.begin(), pilotTourInteractions.end(), m->localID) != pilotTourInteractions.end()) continue;
			vec3 at;
			if(!MapInteractionPos(*m, &at)) continue;
			const f32 d = glm::length(vec2(at) - here);
			if(d < best) { best = d; pilotTour = PilotTour(); pilotTour.kind = 1; pilotTour.localID = m->localID; }
		}
		foreach_const(npc, actorNpcList) {
			if(!npc->monsterAi || npc->hp <= 0 || npc->ownerUID != ActorUID::INVALID || IsStructure(*npc) || !(npc->faction == Faction::ORANGE || (npc->faction == Faction::MONSTER && !hostileMonsters)) || !NpcHittableBy(*npc, p)) continue;
			if(eastl::find(pilotTourNpcs.begin(), pilotTourNpcs.end(), npc->UID) != pilotTourNpcs.end()) continue;
			const f32 d = Config().DevPilotTourFirst == (i32)npc->docID ? 0.0f : glm::length(vec2(npc->pos) - here);
			if(d < best) { best = d; pilotTour = PilotTour(); pilotTour.kind = 2; pilotTour.npc = npc->UID; pilotTour.doc = (i32)npc->docID; }
		}
		if(!pilotTour.kind) return false;
		pilotTour.since = localTime;
		LOG("PilotTour :: next %s %d at %.0f", pilotTour.kind == 1 ? "interaction" : "monster", pilotTour.kind == 1 ? pilotTour.localID : pilotTour.doc, best);
	}
	static const f64 HYP_TOUR_WARP_AFTER_SEC = 20.0;
	static const f32 HYP_TOUR_WARP_DIST = 700.0f;
	static const f32 HYP_TOUR_WARP_STANDOFF = 350.0f;
	static const f32 HYP_TOUR_WARP_LIFT = 30.0f;
	if(!pilotTour.warped && TimeDiffSec(TimeDiff(pilotTour.since, localTime)) > HYP_TOUR_WARP_AFTER_SEC) {
		vec3 at = vec3(0);
		bool found = false;
		if(pilotTour.kind == 1) foreach_const(m, mapInteractions) if(m->localID == pilotTour.localID && MapInteractionPos(*m, &at)) found = true;
		if(pilotTour.kind == 2) if(ActorNpc* npc = FindNpcActor(pilotTour.npc)) { at = npc->pos; found = true; }
		const vec2 rel = here - vec2(at);
		const f32 len = glm::length(rel);
		if(found && len > HYP_TOUR_WARP_DIST) {
			const vec3 dest = vec3(vec2(at) + rel / len * HYP_TOUR_WARP_STANDOFF, at.z + HYP_TOUR_WARP_LIFT);
			pilotTour.warped = true;
			WarpPlayer(p, dest, atan2f(-rel.y, -rel.x));
			LOG("PilotTour :: stuck %.0f from the stop after %.0f s, warped next to it", len, HYP_TOUR_WARP_AFTER_SEC);
			PilotTourEvent("warped");
		}
	}
	if(pilotTour.kind == 1) {
		foreach_const(m, mapInteractions) {
			vec3 at;
			if(m->localID != pilotTour.localID || !MapInteractionPos(*m, &at)) continue;
			*goal = vec2(at);
			if(glm::length(vec2(at) - here) < HYP_TOUR_INTERACT_REACH) {
				out->hasInteract = true;
				out->interact = at;
			}
		}
		return true;
	}
	ActorNpc* npc = FindNpcActor(pilotTour.npc);
	*goal = vec2(npc->pos);
	out->hasTarget = true;
	out->target = npc->pos;
	out->hp = npc->hp;
	out->hpMax = npc->hpMax;
	out->doc = (i32)npc->docID;
	return true;
}

bool World::Pilot(Player& p, PilotDecision* out)
{
	static const f32 HYP_PILOT_ENGAGE_RANGE = 1300.0f;
	if(!map || !p.body || p.dead || p.Main().classType == ClassType::NONE) return false;
	const vec3 pos = p.body->GetWorldPos();
	const vec2 here = vec2(pos);
	if(!map->ingameLevels) {
		const Player* nearest = nullptr;
		f32 nearestDist = 0;
		foreach_const(e, players) {
			if(e->dead || e->untargetable || e->team == p.team || !e->body || e->Main().classType == ClassType::NONE || !(e->fogSeenBy & (1u << p.team))) continue;
			const f32 d = glm::length(vec2(e->body->GetWorldPos()) - here);
			if(nearest && d >= nearestDist) continue;
			nearest = &*e;
			nearestDist = d;
		}
		if(!nearest) return false;
		const vec3 at = nearest->body->GetWorldPos();
		if(nearestDist < HYP_PILOT_ENGAGE_RANGE) {
			out->hasTarget = true;
			out->master = true;
			out->target = at;
			out->hp = nearest->Main().hp;
			out->hpMax = nearest->Main().hpMax;
			out->doc = 100000000 + (i32)nearest->Main().classType;
		}
		out->finalGoal = at;
		out->goal = vec3(BotNavStep(p, vec2(at)), pos.z);
		return true;
	}
	vec2 goal = here;
	const bool touring = Config().DevPilotTour && PilotTourStep(p, out, &goal);
	const bool retreat = !touring && BotRetreat(p, &goal);
	f32 best = retreat || touring ? 0.0f : HYP_PILOT_ENGAGE_RANGE;
	foreach_const(e, players) {
		if(e->dead || e->untargetable || e->team == p.team || !e->body || e->Main().classType == ClassType::NONE || !(e->fogSeenBy & (1u << p.team))) continue;
		if(BotDangerAt(p, vec2(e->body->GetWorldPos()), nullptr, nullptr)) continue;
		const f32 d = glm::length(vec2(e->body->GetWorldPos()) - here);
		if(d >= best) continue;
		best = d;
		out->hasTarget = true;
		out->master = true;
		out->target = e->body->GetWorldPos();
		out->hp = e->Main().hp;
		out->hpMax = e->Main().hpMax;
		out->doc = 100000000 + (i32)e->Main().classType;
	}
	if(!out->hasTarget) {
		static const f32 HYP_PILOT_TARGET_PROGRESS_SEC = 8.0f;
		static const f32 HYP_PILOT_TARGET_PROGRESS_DIST = 300.0f;
		static const f32 HYP_PILOT_TARGET_SKIP_SEC = 30.0f;
		for(i32 s = (i32)p.botFarmSkips.size() - 1; s >= 0; s--) if(localTime >= p.botFarmSkips[s].second) p.botFarmSkips.erase(p.botFarmSkips.begin() + s);
		const ActorNpc* chosen = nullptr;
		foreach_const(npc, actorNpcList) {
			if(!npc->monsterAi || !NpcHittableBy(*npc, p) || NpcHasStatusType(*npc, StatusType::GODMODE)) continue;
			if(IsStructure(*npc) && !BotSiegeOk(p, *npc)) continue;
			if(!IsStructure(*npc) && BotDangerAt(p, vec2(npc->pos), nullptr, nullptr)) continue;
			bool skipped = false;
			foreach_const(s, p.botFarmSkips) if(s->first == npc->UID) skipped = true;
			if(skipped) continue;
			const f32 d = glm::length(vec2(npc->pos) - here);
			if(d >= best) continue;
			best = d;
			chosen = &*npc;
		}
		if(chosen && chosen->UID != p.botFarmUID) {
			p.botFarmUID = chosen->UID;
			p.botFarmCheckAt = TimeAdd(localTime, TimeMsToTime(HYP_PILOT_TARGET_PROGRESS_SEC * 1000.0));
			p.botFarmCheckDist = best;
			p.botFarmCheckHp = chosen->hp;
		}
		else if(chosen && localTime >= p.botFarmCheckAt) {
			if(best > p.botFarmCheckDist - HYP_PILOT_TARGET_PROGRESS_DIST && chosen->hp >= p.botFarmCheckHp) {
				LOG("Pilot :: gives up creature=%d at %.0f (no progress, hp %d)", (i32)chosen->docID, best, chosen->hp);
				if(p.botFarmSkips.full()) p.botFarmSkips.erase(p.botFarmSkips.begin());
				p.botFarmSkips.push_back(eastl::make_pair(chosen->UID, TimeAdd(localTime, TimeMsToTime(HYP_PILOT_TARGET_SKIP_SEC * 1000.0))));
				p.botFarmUID = ActorUID::INVALID;
				chosen = nullptr;
			}
			else {
				p.botFarmCheckAt = TimeAdd(localTime, TimeMsToTime(HYP_PILOT_TARGET_PROGRESS_SEC * 1000.0));
				p.botFarmCheckDist = best;
				p.botFarmCheckHp = chosen->hp;
			}
		}
		if(chosen) {
			out->hasTarget = true;
			out->target = chosen->pos;
			out->hp = chosen->hp;
			out->hpMax = chosen->hpMax;
			out->doc = (i32)chosen->docID;
		}
	}
	vec2 occupyPoint;
	ActorUID occupyObject = ActorUID::INVALID;
	f32 occupyReach = 0;
	if(touring) {}
	else if(!retreat && !out->hasTarget && OccupyTarget(p, &occupyPoint, &occupyObject, &occupyReach)) {
		out->hasInteract = true;
		out->interact = vec3(occupyPoint, pos.z);
		goal = occupyPoint;
	}
	else if(!retreat && !BotObjective(p, &goal) && !BotLaneGoal(p, &goal)) goal = here;
	out->finalGoal = vec3(goal, pos.z);
	goal = BotNavStep(p, goal);
	vec2 tower;
	f32 towerRange = 0;
	if(BotDangerAt(p, goal, &tower, &towerRange)) {
		const vec2 away = here - tower;
		const f32 len = glm::length(away);
		goal = tower + (len > 1.0f ? away / len : vec2(1, 0)) * towerRange;
		out->finalGoal = vec3(goal, pos.z);
	}
	if(BotDangerAt(p, vec2(out->finalGoal), nullptr, nullptr)) out->finalGoal = vec3(goal, pos.z);
	out->goal = vec3(goal, pos.z);
	return true;
}

void World::BuildTeamGates()
{
	static const f32 GATE_PROBE_HEIGHT = 100.0f;
	static const f32 GATE_PROBE_REACH = 1500.0f;
	if(teamGatesBuilt || !map || !map->ingameLevels) return;
	teamGatesBuilt = true;
	const auto& content = GetGameXmlContent();
	i32 built = 0;
	foreach(d, actorDynamicList) {
		if(!content.IsTeamGate((i32)d->docID) || (d->faction != Faction::RED && d->faction != Faction::BLUE)) continue;
		const vec3 at = d->pos + vec3(0, 0, GATE_PROBE_HEIGHT);
		f32 bestWidth = 1e9f;
		vec2 bestAxis = vec2(0);
		vec2 bestCenter = vec2(d->pos);
		for(i32 k = 0; k < 2; k++) {
			const f32 a = d->rot.z + (k ? PI * 0.5f : 0.0f);
			const vec2 axis = vec2(cosf(a), sinf(a));
			f32 left = 0, right = 0;
			if(!physics.RaycastStatic(at, at + vec3(axis * GATE_PROBE_REACH, 0), &left)) continue;
			if(!physics.RaycastStatic(at, at - vec3(axis * GATE_PROBE_REACH, 0), &right)) continue;
			if(left + right >= bestWidth) continue;
			bestWidth = left + right;
			bestAxis = axis;
			bestCenter = vec2(d->pos) + axis * (left - right) * 0.5f;
		}
		if(bestWidth > GATE_PROBE_REACH * 2.0f) {
			LOG("Gate :: dynamic %d (doc %d) opening not found", d->localID, (i32)d->docID);
			continue;
		}
		if(!teamGates.full()) teamGates.push_back({ d->localID, bestCenter - bestAxis * bestWidth * 0.5f, bestCenter + bestAxis * bestWidth * 0.5f, (i32)d->faction, d->action == ActionStateID::DYNAMIC_OPEN || d->action == ActionStateID::DYNAMIC_DIE });
		LOG("Gate :: dynamic %d (doc %d, team %d) wall %.0f wide at (%.0f, %.0f)", d->localID, (i32)d->docID, (i32)d->faction, bestWidth, bestCenter.x, bestCenter.y);
		built++;
	}
	LOG("Gate :: %d team gate walls", built);
}

bool World::InsideBodyBlocker(const vec2& at, f32 margin) const
{
	foreach_const(it, actorNpcList) {
		if(it->bodyBlocker && it->monsterAi && glm::length(at - vec2(it->pos)) < it->monsterAi->bodyBlockRadius + margin) return true;
	}
	return false;
}

void World::BotKeepOutOfBlockers(const Player& bot, vec2* goal) const
{
	static const f32 HYP_BOT_BLOCKER_MARGIN = 20.0f;
	foreach_const(it, actorNpcList) {
		if(!it->bodyBlocker || !it->monsterAi) continue;
		const vec2 center = vec2(it->pos);
		const f32 keep = it->monsterAi->bodyBlockRadius + bot.body->GetBoundSize().x + HYP_BOT_BLOCKER_MARGIN;
		const vec2 d = *goal - center;
		const f32 len = glm::length(d);
		if(len >= keep) continue;
		const vec2 out = len > 1.0f ? d / len : glm::normalize(vec2(bot.body->GetWorldPos()) - center + vec2(0.001f, 0));
		*goal = center + out * keep;
	}
	const vec2 here = vec2(bot.body->GetWorldPos());
	foreach_const(it, actorNpcList) {
		if(!it->bodyBlocker || !it->monsterAi) continue;
		const vec2 center = vec2(it->pos);
		const f32 keep = it->monsterAi->bodyBlockRadius + bot.body->GetBoundSize().x + HYP_BOT_BLOCKER_MARGIN;
		const vec2 d = *goal - here;
		const f32 len = glm::length(d);
		if(len < 1.0f) continue;
		const f32 along = glm::dot(center - here, d) / (len * len);
		if(along <= 0.0f || along >= 1.0f) continue;
		const vec2 side = vec2(-d.y, d.x) / len;
		const f32 offset = glm::dot(here - center, side);
		if(fabsf(offset) >= it->monsterAi->bodyBlockRadius + bot.body->GetBoundSize().x) continue;
		*goal = center + side * (offset >= 0.0f ? keep : -keep);
	}
}

bool World::BotGateBlocks(const Player& bot, const vec2& from, vec2* goal) const
{
	static const f32 GATE_STOP_DIST = 150.0f;
	foreach_const(g, teamGates) {
		if(g->open || g->team == (i32)bot.team) continue;
		const vec2 d = *goal - from;
		const vec2 s = g->b - g->a;
		const f32 denom = Vec2Cross(d, s);
		if(fabsf(denom) < 1e-3f) continue;
		const f32 t = Vec2Cross(g->a - from, s) / denom;
		const f32 u = Vec2Cross(g->a - from, d) / denom;
		if(t < 0.0f || t > 1.0f || u < 0.0f || u > 1.0f) continue;
		const vec2 hit = from + d * t;
		const f32 len = glm::length(d);
		*goal = len > 1.0f ? hit - d / len * GATE_STOP_DIST : from;
		return true;
	}
	return false;
}

bool World::RaycastShot(const vec3& from, const vec3& to, i32 team, f32* dist, const PxRigidActor** wallActor, ActorDynamic** gate)
{
	*gate = nullptr;
	bool blocked = physics.RaycastStatic(from, to, dist, team, wallActor);
	const vec2 d = vec2(to) - vec2(from);
	const f32 len = glm::length(to - from);
	foreach_const(g, teamGates) {
		if(g->open || g->team == team) continue;
		const vec2 s = g->b - g->a;
		const f32 denom = Vec2Cross(d, s);
		if(fabsf(denom) < 1e-3f) continue;
		const f32 t = Vec2Cross(g->a - vec2(from), s) / denom;
		const f32 u = Vec2Cross(g->a - vec2(from), d) / denom;
		if(t < 0.0f || t > 1.0f || u < 0.0f || u > 1.0f) continue;
		if(blocked && t * len >= *dist) continue;
		foreach(a, actorDynamicList) {
			if(a->localID != g->localID) continue;
			*gate = &*a;
			*dist = t * len;
			*wallActor = nullptr;
			blocked = true;
			break;
		}
	}
	return blocked;
}

void World::UpdateObserver()
{
	static const f64 OBSERVER_MOVE_MS = 4000.0;
	static const f64 OBSERVER_FIGHT_RECENT_SEC = 3.0;
	static const f32 OBSERVER_DISTANCE = 450.0f;
	if(!Config().DevObserver || gameStartedAt == Time::ZERO || localTime < observerNextMove) return;
	observerNextMove = TimeAdd(localTime, TimeMsToTime(OBSERVER_MOVE_MS));
	Player* human = nullptr;
	foreach(p, players) if(p->clientHd != ClientHandle::INVALID && !p->removed && p->body) { human = &*p; break; }
	if(!human || human->dead) return;
	human->untargetable = true;
	human->devGod = true;
	const Player* best = nullptr;
	f64 bestAge = 1e9;
	foreach_const(p, players) {
		if(p->clientHd != ClientHandle::INVALID || p->dead || !p->body || p->Main().classType == ClassType::NONE) continue;
		const f64 age = p->battleActionAt == Time::ZERO ? 1e6 : TimeDiffSec(TimeDiff(p->battleActionAt, localTime));
		const f64 score = age + (p->index == (u32)observerTarget && age < OBSERVER_FIGHT_RECENT_SEC ? -OBSERVER_FIGHT_RECENT_SEC : 0.0);
		if(score < bestAge) { bestAge = score; best = &*p; }
	}
	if(!best) return;
	observerTarget = (i32)best->index;
	const vec3 at = best->body->GetWorldPos();
	const f32 yaw = best->input.rot.bodyYaw;
	const vec3 spot = at - vec3(cosf(yaw), sinf(yaw), 0) * OBSERVER_DISTANCE;
	MovePlayer(*human, vec3(spot.x, spot.y, at.z), yaw, true);
	LOG("Observer :: watches %ls at (%.0f, %.0f) last action %.1fs ago", best->name.data(), at.x, at.y, bestAge);
}

static const f32 MEASURED_SLIME_FIGHT_START_MASS[11] = { 0.0f, 0.0f, 0.0f, 5.0f, 20.0f, 35.0f, 60.0f, 70.0f, 80.0f, 90.0f, 90.0f };

void World::BotIdleCharge(Player& bot)
{
	const WeaponFiring& wf = ActiveWeaponFiring(bot, bot.Main().stance);
	if(wf.charge.empty() || wf.chargeTime.empty()) return;
	const f32 resMax = MasterStat(bot.Main().classType, RESOURCES[1].maxStat);
	if(resMax <= 0) return;
	f32 goal = resMax;
	if(FindPassiveOfType(bot, StatusType::SLIME)) {
		if(bot.botIdleResourceGoal < 0) {
			const f32 r = (f32)Randf01() * 10.0f;
			const i32 i = MIN((i32)r, 9);
			bot.botIdleResourceGoal = MEASURED_SLIME_FIGHT_START_MASS[i] + (MEASURED_SLIME_FIGHT_START_MASS[i + 1] - MEASURED_SLIME_FIGHT_START_MASS[i]) * (r - (f32)i);
		}
		goal = MIN(bot.botIdleResourceGoal, resMax);
	}
	if(bot.weaponState == WEAPON_STATE_CHARGE) {
		if(goal < resMax && bot.res[1] >= goal) OnWeaponState(bot, 0, 0, bot.firingCombo);
		return;
	}
	if(bot.res[1] >= goal) return;
	bool gains = FindResourceGainRule(bot.Main().classType, ResourceEvent::CHARGING_PER_SEC) != nullptr;
	const GameXmlContent::Action* charge = GetGameXmlContent().FindSkillAction(ActionClass(bot), ActionStateID::ATTACKCHARGE_BEHAVIORSTATE, bot.Main().stance);
	if(charge && !gains) foreach_const(cmd, charge->commands) {
		if(cmd->type != ActionCommand::Type::STATUS || cmd->status.statusIndex == 0 || cmd->status.remove) continue;
		const StatusInfo* info = GetGameXmlContent().FindStatus(cmd->status.statusIndex);
		if(info) foreach_const(m, info->mods) if(m->regen && m->stat == StatusMod::RAGE) gains = true;
	}
	if(!gains) return;
	bot.botChargeGoal = BotRollChargeLevel(bot, wf);
	OnWeaponState(bot, WEAPON_STATE_CHARGE, 0, bot.firingCombo);
	replication->RelayWeaponState(ClientHandle::INVALID, bot.Main().UID, MEASURED_WEAPON_STATE_WEAPON_ID, WEAPON_STATE_CHARGE, 0, bot.firingCombo);
}

struct BotRelease { i32 status; f64 ms; i32 skill; };
static const BotRelease MEASURED_BOT_RELEASES[] = {
	{ 1230200021, 2130.0, 180200011 },
};

bool World::BotReleaseHold(Player& bot)
{
	foreach_const(st, activeStatuses) {
		if(st->targetIndex != bot.index || st->until <= localTime) continue;
		for(const BotRelease& r : MEASURED_BOT_RELEASES) {
			if(r.status != st->statusID || localTime < TimeAdd(st->appliedAt, TimeMsToTime(r.ms))) continue;
			LOG("BotSkill :: %ls releases %d after %.1fs", bot.name.data(), r.status, TimeDiffSec(TimeDiff(st->appliedAt, localTime)));
			PlayerCastSkill(bot, SkillID(r.skill), bot.body->GetWorldPos(), Slice<const ActorUID>(), 0);
			bot.tNextBotSkill = TimeAdd(localTime, TimeMsToTime(500.0));
			return true;
		}
	}
	return false;
}

struct HideHold { i32 status; f64 ms; };
static const HideHold MEASURED_HIDE_HOLDS[] = {
	{ 1230030030, 2450.0 },
	{ 1230060030, 1010.0 },
	{ 1230100060, 1420.0 },
	{ 1230200028, 1080.0 },
	{ 1230210030, 3980.0 },
	{ 1230240000, 460.0 },
};

bool World::BotHoldsHide(const Player& bot) const
{
	foreach_const(st, activeStatuses) {
		if(st->targetIndex != bot.index || !st->info || st->info->type != StatusType::HIDE || st->until <= localTime) continue;
		for(const HideHold& h : MEASURED_HIDE_HOLDS) {
			if(h.status == st->statusID && localTime < TimeAdd(st->appliedAt, TimeMsToTime(h.ms))) return true;
		}
	}
	return false;
}

const World::ActorNpc* World::BotRepairTarget(const Player& bot, f32 range) const
{
	const WeaponFiring& wf = ActiveWeaponFiring(bot, bot.Main().stance);
	const Remote* weapon = GetGameXmlContent().FindRemote(wf.base);
	if(!weapon || weapon->friendlyStatuses.empty()) return nullptr;
	const ActorNpc* best = nullptr;
	f32 bestRatio = 1.0f;
	foreach_const(npc, actorNpcList) {
		if(npc->ownerIndex != bot.index || npc->ownerUID == ActorUID::INVALID || npc->hpMax <= 0 || npc->hp <= 0 || npc->removeAt != Time::ZERO) continue;
		if(npc->despawnAt != Time::ZERO && localTime >= npc->despawnAt) continue;
		if(glm::length(vec2(npc->pos) - vec2(bot.body->GetWorldPos())) > range) continue;
		const f32 ratio = (f32)npc->hp / (f32)npc->hpMax;
		if(ratio < bestRatio) { bestRatio = ratio; best = &*npc; }
	}
	return best;
}

void World::UpdateBotCombat()
{
	foreach(it, players) {
		Player& bot = *it;
		if(bot.clientHd != ClientHandle::INVALID || bot.dead) continue;
		if(localTime < bot.tNextBotShot || localTime < bot.ccUntil) continue;
		vec2 zoneAway;
		f32 zoneDepth = 0;
		if(BotHoldsHide(bot) || BotEvadeZone(bot, &zoneAway, &zoneDepth)) continue;
		if(localTime < bot.dashUntil || localTime < bot.botSkillStillUntil || bot.casting.skillID != SkillID::INVALID) continue;
		if(IsBlocked(bot, true, false, false)) continue;
		if(WeaponIsMelee(bot) && Config().DevShowcase) continue;
		if(WeaponIsMelee(bot)) {
			bool swing = false;
			const bool lateHit = WeaponHitsLater(bot);
			const f32 meleeReach = lateHit ? WeaponRange(bot) : BOT_MELEE_RANGE;
			vec2 swingAt = vec2(0);
			foreach_const(e, players) {
				if(e->dead || e->untargetable || e->team == bot.team || !(e->fogSeenBy & (1u << bot.team))) continue;
				if(glm::length(vec2(e->body->GetWorldPos()) - vec2(bot.body->GetWorldPos())) <= meleeReach + HitRadius(*e) + HitRadius(bot)) { swing = true; swingAt = vec2(e->body->GetWorldPos()); break; }
			}
			if(!swing && !Config().DevShowcase) {
				foreach_const(npc, actorNpcList) {
					if(!NpcHittableBy(*npc, bot) || (IsStructure(*npc) && !BotSiegeOk(bot, *npc))) continue;
					if(glm::length(vec2(npc->pos) - vec2(bot.body->GetWorldPos())) <= meleeReach + NpcHitRadius(*npc) + HitRadius(bot)) { swing = true; swingAt = vec2(npc->pos); break; }
				}
			}
			if(swing) {
				const vec2 rel = swingAt - vec2(bot.body->GetWorldPos());
				if(glm::length(rel) > 1.0f) {
					bot.weaponAimYaw = atan2f(rel.y, rel.x);
					bot.input.rot.upperYaw = bot.weaponAimYaw;
					bot.input.rot.bodyYaw = bot.weaponAimYaw;
				}
			}
			if(swing) {
				const WeaponFiring& swingWf = ActiveWeaponFiring(bot, bot.Main().stance);
				if(!swingWf.charge.empty() && !swingWf.chargeTime.empty()) {
					if(bot.weaponState != WEAPON_STATE_CHARGE) {
						bot.botChargeGoal = BotRollChargeLevel(bot, swingWf);
						OnWeaponState(bot, WEAPON_STATE_CHARGE, 0, bot.firingCombo);
						replication->RelayWeaponState(ClientHandle::INVALID, bot.Main().UID, MEASURED_WEAPON_STATE_WEAPON_ID, WEAPON_STATE_CHARGE, 0, bot.firingCombo);
					}
					if(bot.chargeAnnounced < bot.botChargeGoal) { bot.tNextBotShot = localTime; continue; }
					OnWeaponState(bot, WEAPON_STATE_FIRE, bot.chargeAnnounced, bot.firingCombo);
				}
				bot.weaponFired = true;
				const bool chained = bot.firingCombo < swingWf.comboValid.size() && bot.botComboShotAt != Time::ZERO && (swingWf.comboValid[bot.firingCombo] <= 0 || localTime <= TimeAdd(bot.botComboShotAt, TimeMsToTime(swingWf.comboValid[bot.firingCombo] * 1000.0)));
				bot.tNextBotShot = TimeAdd(localTime, TimeMsToTime(BotNextFireDelay(bot, chained ? bot.firingCombo + 1 : 0) * 1000.0));
			}
			continue;
		}

		const f32 range = WeaponRange(bot);

		const vec3 pos = bot.body->GetWorldPos();
		Player* target = nullptr;
		f32 best = range;
		foreach(e, players) {
			if(e->dead || e->untargetable || e->team == bot.team || !(e->fogSeenBy & (1u << bot.team))) continue;
			const f32 d = glm::length(vec2(e->body->GetWorldPos()) - vec2(pos));
			if(d < best) { best = d; target = &*e; }
		}
		bot.tNextBotShot = TimeAdd(localTime, TimeMsToTime(BotNextFireDelay(bot, bot.firingCombo) * 1000.0));
		vec3 aim = target ? target->body->GetWorldPos() : vec3(0);
		const ActorNpc* repair = !Config().DevShowcase ? BotRepairTarget(bot, range) : nullptr;
		if(repair) {
			target = nullptr;
			aim = repair->pos;
			best = glm::length(vec2(repair->pos) - vec2(pos));
		}
		if(!target && !repair && !Config().DevShowcase) {
			f32 bestNpc = range;
			foreach_const(npc, actorNpcList) {
				if(!NpcHittableBy(*npc, bot) || (IsStructure(*npc) && !BotSiegeOk(bot, *npc))) continue;
				const f32 d = glm::length(vec2(npc->pos) - vec2(pos));
				if(d < bestNpc && BotShotClear(bot, npc->pos, NpcHitRadius(*npc))) { bestNpc = d; best = d; aim = npc->pos; }
			}
			if(bestNpc >= range) { BotIdleCharge(bot); continue; }
		}
		else if(!target && !repair) { BotIdleCharge(bot); continue; }

		const WeaponFiring& chargeWf = ActiveWeaponFiring(bot, bot.Main().stance);
		if(!chargeWf.charge.empty() && !chargeWf.chargeTime.empty()) {
			if(bot.weaponState != WEAPON_STATE_CHARGE) {
				bot.botChargeGoal = BotRollChargeLevel(bot, chargeWf);
				OnWeaponState(bot, WEAPON_STATE_CHARGE, 0, bot.firingCombo);
				replication->RelayWeaponState(ClientHandle::INVALID, bot.Main().UID, MEASURED_WEAPON_STATE_WEAPON_ID, WEAPON_STATE_CHARGE, 0, bot.firingCombo);
			}
			if(bot.chargeAnnounced < bot.botChargeGoal) { bot.tNextBotShot = localTime; continue; }
			OnWeaponState(bot, WEAPON_STATE_FIRE, bot.chargeAnnounced, bot.firingCombo);
		}
		if(!WeaponHeatShot(bot, localTime, replication, WeaponHeatScale(bot))) continue;
		const vec2 rel = vec2(aim) - vec2(pos);
		const vec2 dir = glm::length(rel) > 1.0f ? glm::normalize(rel) : vec2(1, 0);
		bot.weaponAimYaw = atan2f(dir.y, dir.x);
		PlayerWeaponShot(bot, pos, dir, best);
		bot.botIdleResourceGoal = -1;
		bot.tNextBotShot = TimeAdd(localTime, TimeMsToTime(BotNextFireDelay(bot, bot.firingCombo) * 1000.0));
	}
}

struct BotChargeShare { i32 classType; f32 share[4]; };
static const BotChargeShare MEASURED_BOT_CHARGE_SHARES[] = {
	{ 6, { 0.38f, 0.09f, 0.21f, 0.32f } },
	{ 20, { 0.76f, 0.24f, 0.0f, 0.0f } },
	{ 36, { 0.94f, 0.06f, 0.0f, 0.0f } },
	{ 9, { 0.99f, 0.01f, 0.0f, 0.0f } },
};

u8 World::BotRollChargeLevel(const Player& bot, const WeaponFiring& wf) const
{
	const i32 levels = (i32)MIN(wf.charge.size(), wf.chargeTime.size());
	for(const BotChargeShare& row : MEASURED_BOT_CHARGE_SHARES) {
		if(row.classType != (i32)bot.Main().classType) continue;
		f32 r = (f32)Randf01();
		for(i32 lv = 0; lv < 4; lv++) {
			r -= row.share[lv];
			if(r < 0) return (u8)MIN(lv, levels);
		}
	}
	return 0;
}

f32 World::BotNextFireDelay(const Player& bot, u32 combo) const
{
	const WeaponFiring& wf = ActiveWeaponFiring(bot, bot.Main().stance);
	f32 delay = wf.fireDelay;
	if(combo < wf.comboDelay.size() && wf.comboDelay[combo] > 0 && wf.comboDelay[combo] <= wf.comboValid[combo]) delay = wf.comboDelay[combo];
	if(wf.continuityCount > 0 && (i32)combo + 1 >= wf.continuityCount) delay = MAX(delay, wf.continuityDelay);
	return MAX(delay, BOT_MIN_FIRE_DELAY) * WeaponFireDelayScale(bot);
}
