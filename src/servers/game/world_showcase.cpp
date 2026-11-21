#include "config.h"
#include "world.h"
#include "world_common.h"
#include <mxm/game_content.h>
#include <chrono>
#include <cstdio>
#include <EASTL/hash_map.h>

static const i32 GX_DEATH_TEST_MODE = 4;
static const f32 VALIDATE_CASTER_HP = 0.6f;
static const f32 VALIDATE_EXTRA_SIDE = 180.0f;
static const f32 VALIDATE_ALLY_SIDE = 250.0f;
static const i32 VALIDATE_FOLLOW_SHOT_MS = 1500;
static const i32 VALIDATE_COMBO_DELAY_MS = 700;
static const i32 VALIDATE_COMBO_MAX_DEPTH = 4;

struct ValidateAim { f32 dist; i32 expect; };

static const eastl::hash_map<i32, ValidateAim>& ValidateAims()
{
	static eastl::hash_map<i32, ValidateAim> aims;
	static bool loaded = false;
	if(!loaded) {
		loaded = true;
		if(FILE* f = fopen("validate_aim.txt", "r")) {
			i32 skill = 0, expect = 0;
			f32 dist = 0;
			while(fscanf(f, "%d %f %d", &skill, &dist, &expect) == 3) aims[skill] = { dist, expect };
			fclose(f);
		}
	}
	return aims;
}
static const f32 SHOWCASE_MIN_MEASURE_SEC = 2.5f;
static const f32 SHOWCASE_QUIET_SEC = 0.5f;
static const f32 MIRROR_POS_TOLERANCE = 60.0f;
static const f64 MIRROR_STUCK_STATE_SEC = 3.0;
static const i32 MIRROR_POS_MAX_WAITS = 12;
static const f64 MIRROR_POS_RETRY_MS = 400.0;
static const i32 MIRROR_CHARGE_HOLD_MARGIN_MS = 300;
static const i32 SHOWCASE_STANCE_SHOTS = 3;
static const f32 SHOWCASE_STANCE_WINDOW_SEC = 4.0f;
static const i32 SHOWCASE_STANCE_FIRST_SHOT_MS = 900;
static const i32 SHOWCASE_EVADE_HIT_MS = 150;
static const i32 SHOWCASE_EVADE_COUNTER_MS = 600;
static const f32 GX_TEST_BOARD_GAP = 250.0f;
static const f32 GX_TEST_DRIVE_STEP = 150.0f;
static const i32 GX_TEST_DRIVE_STEPS = 7;
static const f32 GX_TEST_RECALL_DIST = 1500.0f;
static const f32 GX_TEST_ENEMY_VIEW_DIST = 350.0f;
static const f32 GX_TEST_BASE_MOVESPEED = 620.0f;
static const i32 GX_PREVIEW_MODE = 2;
static const i32 GX_EMP_TEST_MODE = 3;
static const i32 GX_TEST_PASSENGER_HIT = 500;
static const f32 GX_TEST_EMP_DIST = 200.0f;
static const i32 GX_TEST_EMP_SKILL = 180410020;
static const f32 GX_PREVIEW_ENEMY_DIST = 450.0f;
static const f32 GX_PREVIEW_CLOSE_DIST = 220.0f;
static const f32 GX_PREVIEW_SPREAD = 90.0f;
static const f32 GX_PREVIEW_RIDER_GAP = 250.0f;
static const f32 GX_PREVIEW_RECALL_DIST = 750.0f;
static const f32 GATLING_PREVIEW_NEAR_DIST = 160.0f;

static const GameXmlContent::Action& ShowcaseSkillAction(const GameXmlContent& content, ClassType cls, ActionStateID action, i8 stance = 0)
{
	static const GameXmlContent::Action none = {};
	const GameXmlContent::Action* found = content.FindSkillAction(cls, action, stance);
	return found ? *found : none;
}

static f32 RemoteDamageRadius(const GameXmlContent& content, RemoteIdx idx, i32 depth)
{
	const Remote* r = content.FindRemote(idx);
	if(!r || depth > 5) return 0;
	const bool vsEnemy = r->damageGroup == Remote::DamageGroup::eENEMY || r->damageGroup == Remote::DamageGroup::eALL;
	const f32 extent = r->boundType == Remote::BoundType::E_BOUND_BOX ? MAX(r->boundSize[0], r->boundSize[1]) * 0.5f : r->boundSize[0] * 0.5f;
	f32 radius = (vsEnemy && (r->attackMultiplier > 0 || !r->statuses.empty() || r->changeCasterAction != ActionStateID::INVALID)) ? extent : 0;
	const Remote::SpawnRules* lists[4] = { &r->onDestroy, &r->onTime, &r->onActivate, &r->onHitCharacter };
	for(const Remote::SpawnRules* l : lists) {
		for(const Remote::SpawnRule& rule : *l) radius = MAX(radius, RemoteDamageRadius(content, rule.idx, depth + 1));
	}
	return radius;
}

static f32 RemoteReach(const GameXmlContent& content, RemoteIdx idx, i32 depth)
{
	const Remote* r = content.FindRemote(idx);
	if(!r || depth > 5) return 0;
	const f32 travel = r->maxDistance > 0 ? r->maxDistance : (r->maxSpeed > 0 ? r->lifeTime * r->maxSpeed : 0.0f);
	const bool vsEnemy = r->damageGroup == Remote::DamageGroup::eENEMY || r->damageGroup == Remote::DamageGroup::eALL;
	const bool offensive = vsEnemy && (r->attackMultiplier > 0 || !r->statuses.empty() || r->changeCasterAction != ActionStateID::INVALID);
	f32 front = 0;
	if(offensive) front = r->boundType == Remote::BoundType::E_BOUND_BOX ? MAX(r->offsetY + RemoteBoundOffset(*r) + r->boundSize[1] * 0.5f, r->boundSize[0] * 0.5f) : r->boundSize[0] * 0.5f + RemoteBoundOffset(*r);
	f32 children = 0;
	const Remote::SpawnRules* lists[4] = { &r->onDestroy, &r->onTime, &r->onActivate, &r->onHitCharacter };
	for(const Remote::SpawnRules* l : lists) {
		for(const Remote::SpawnRule& rule : *l) children = MAX(children, RemoteReach(content, rule.idx, depth + 1));
	}
	const f32 own = MAX(front, children);
	return own > 0 ? travel + own : 0.0f;
}

static f32 SkillReach(const GameXmlContent& content, ClassType cls, SkillID skillID)
{
	auto f = content.skillMap.find(skillID);
	if(f == content.skillMap.end()) return 0;
	const auto& action = ShowcaseSkillAction(content, cls, f->second.action);
	f32 reach = 0;
	foreach_const(cmd, action.commands) {
		const bool atCaster = cmd->remote.targetPreset == ActionCommand::TargetPreset::SELF_FFF || cmd->remote.targetPreset == ActionCommand::TargetPreset::SELF_FFF_1 ||
			cmd->remote.targetPreset == ActionCommand::TargetPreset::SELF_TFF || cmd->remote.targetPreset == ActionCommand::TargetPreset::SELF_TTF || cmd->remote.targetPreset == ActionCommand::TargetPreset::SELF_TTT;
		if((cmd->type == ActionCommand::Type::REMOTE || cmd->type == ActionCommand::Type::REMOTE_SKILL_TARGET) && cmd->remote.idx != RemoteIdx::INVALID && atCaster)
			reach = MAX(reach, RemoteReach(content, cmd->remote.idx, 0));
	}
	return reach;
}

static bool IsSelfPreset(ActionCommand::TargetPreset p)
{
	return p == ActionCommand::TargetPreset::SELF_FFF || p == ActionCommand::TargetPreset::SELF_FFF_1 || p == ActionCommand::TargetPreset::SELF_TFF ||
		p == ActionCommand::TargetPreset::SELF_TTF || p == ActionCommand::TargetPreset::SELF_TTT;
}

static f32 SkillDamageRadius(const GameXmlContent& content, ClassType cls, SkillID skillID)
{
	auto f = content.skillMap.find(skillID);
	if(f == content.skillMap.end()) return 0;
	const auto& action = ShowcaseSkillAction(content, cls, f->second.action);
	f32 radius = 0;
	bool anyRemote = false;
	bool selfOnly = true;
	foreach_const(cmd, action.commands) {
		if((cmd->type == ActionCommand::Type::REMOTE || cmd->type == ActionCommand::Type::REMOTE_SKILL_TARGET) && cmd->remote.idx != RemoteIdx::INVALID) {
			anyRemote = true;
			radius = MAX(radius, RemoteDamageRadius(content, cmd->remote.idx, 0));
		}
		if(cmd->type == ActionCommand::Type::ENTITY || cmd->type == ActionCommand::Type::CALL_CHILD_SKILL || cmd->type == ActionCommand::Type::REGCOMBO ||
		   cmd->type == ActionCommand::Type::AIOBJECT || cmd->type == ActionCommand::Type::STATUS_SKILL_TARGET) selfOnly = false;
		if(cmd->type == ActionCommand::Type::STATUS && !IsSelfPreset(cmd->status.targetPreset)) selfOnly = false;
	}
	if(!anyRemote && selfOnly) return 0;
	return anyRemote ? radius : -1.0f;
}

static f32 SkillDashDistance(const GameXmlContent& content, ClassType cls, SkillID skillID)
{
	auto f = content.skillMap.find(skillID);
	if(f == content.skillMap.end()) return 0;
	const auto& action = ShowcaseSkillAction(content, cls, f->second.action);
	f32 dash = 0;
	foreach_const(cmd, action.commands) {
		if(cmd->type == ActionCommand::Type::MOVE && (cmd->move.preset == ActionCommand::MovePreset::TARGET_POS || cmd->move.preset == ActionCommand::MovePreset::TARGET_POS_LEAP)) return 0;
		if(cmd->type == ActionCommand::Type::GRAPH_MOVE_HORZ) dash = MAX(dash, cmd->graphMoveHorz.distance);
	}
	return dash;
}

static bool RemoteShapeUnknown(const GameXmlContent& content, RemoteIdx idx, i32 depth)
{
	const Remote* r = content.FindRemote(idx);
	if(!r || depth > 5) return false;
	if(r->boundType == Remote::BoundType::E_BOUND_BEAM_NIF) return true;
	const Remote::SpawnRules* lists[4] = { &r->onDestroy, &r->onTime, &r->onActivate, &r->onHitCharacter };
	for(const Remote::SpawnRules* l : lists) {
		for(const Remote::SpawnRule& rule : *l) if(RemoteShapeUnknown(content, rule.idx, depth + 1)) return true;
	}
	return false;
}

static bool SkillReachUnknown(const GameXmlContent& content, ClassType cls, SkillID skillID)
{
	auto f = content.skillMap.find(skillID);
	if(f == content.skillMap.end()) return false;
	const auto& action = ShowcaseSkillAction(content, cls, f->second.action);
	foreach_const(cmd, action.commands) {
		if(cmd->type == ActionCommand::Type::ENTITY || cmd->type == ActionCommand::Type::AIOBJECT || cmd->type == ActionCommand::Type::CALL_CHILD_SKILL) return true;
		if((cmd->type == ActionCommand::Type::REMOTE || cmd->type == ActionCommand::Type::REMOTE_SKILL_TARGET) && RemoteShapeUnknown(content, cmd->remote.idx, 0)) return true;
	}
	return false;
}

static i64 ShowcaseWallMs()
{
	return (i64)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void World::UpdateShowcase()
{
	if(botUpgrades) {
		botUpgrades = false;
		foreach(it, players) it->skillLevels.clear();
	}
	if(showcase.comboSkill != 0 && localTime >= showcase.comboAt) {
		Player& comboCaster = players[showcase.comboCaster];
		LOG("Showcase :: combo skill=%d", showcase.comboSkill);
		const GameXmlContent::SkillInput* comboIn = GetGameXmlContent().FindSkillInput(showcase.comboSkill);
		const vec3 comboPos = comboIn && comboIn->petCaster && comboIn->needsClick && comboCaster.body && IsCarrying(comboCaster) ? comboCaster.body->GetWorldPos() : showcase.comboPos;
		if(!comboCaster.dead) PlayerCastSkill(comboCaster, SkillID(showcase.comboSkill), comboPos, Slice<const ActorUID>(&showcase.comboTarget, showcase.comboTarget != ActorUID::INVALID ? 1 : 0), 0);
		const i32 nextCombo = Config().DevValidate && showcase.comboDepth < VALIDATE_COMBO_MAX_DEPTH ? ComboSkillFor(comboCaster, showcase.comboSkill) : 0;
		if(nextCombo != 0 && nextCombo != showcase.comboSkill && !comboCaster.dead) {
			showcase.comboSkill = nextCombo;
			showcase.comboAt = TimeAdd(localTime, TimeMsToTime(VALIDATE_COMBO_DELAY_MS));
			showcase.comboDepth++;
		}
		else showcase.comboSkill = 0;
	}
	if(showcase.followShooter >= 0 && localTime >= showcase.followShotAt) {
		Player& shooter = players[showcase.followShooter];
		showcase.followShooter = -1;
		if(!shooter.dead) {
			LOG("Showcase :: follow-up weapon shot");
			ShowcaseWeaponAt(shooter, players[showcase.prevDummy]);
		}
	}
	if(showcase.stanceShots > 0 && localTime >= showcase.stanceShotAt && showcase.stanceShooter >= 0) {
		Player& shooter = players[showcase.stanceShooter];
		const Player& aimAt = players[showcase.prevDummy];
		if(shooter.dead || shooter.Main().stance == 0) showcase.stanceShots = 0;
		else {
			LOG("Showcase :: stance %d weapon shot (%d left)", (i32)shooter.Main().stance, showcase.stanceShots - 1);
			ShowcaseWeaponAt(shooter, aimAt);
			const u32 wm = shooter.mainCharaID & 1;
			const WeaponIndex weapon = wm == 0 ? shooter.mainWeapon : shooter.subWeapon;
			const f32 stanceDelay = GetGameXmlContent().GetWeaponStanceFireDelay((i32)weapon, shooter.Main().stance);
			const f32 delay = MAX(stanceDelay > 0 ? stanceDelay : GetGameXmlContent().GetWeaponFireDelay((i32)weapon), 0.3f);
			showcase.stanceShotAt = TimeAdd(localTime, TimeMsToTime(delay * 1000.0));
			showcase.stanceShots--;
		}
	}
	if(showcase.evadeProbe > 0 && localTime >= showcase.evadeProbeAt && showcase.evadeCaster >= 0) {
		Player& evader = players[showcase.evadeCaster];
		Player& other = players[showcase.prevDummy];
		if(showcase.evadeProbe == 2) {
			bool armed = false;
			foreach_const(st, activeStatuses) if(st->targetIndex == evader.index && st->info && st->info->type == StatusType::STATUS_ON_EVADE) armed = true;
			if(!armed || !HasStatusType(evader, StatusType::EVADE) || evader.dead || other.dead) showcase.evadeProbe = 0;
			else {
				LOG("Showcase :: evade probe: %ls hits %ls during the evade", other.name.data(), evader.name.data());
				DamagePlayer(other, evader, SkillID::INVALID, (i32)WeaponHitDamage(other, nullptr));
				showcase.evadeProbe = 1;
				showcase.evadeProbeAt = TimeAdd(localTime, TimeMsToTime(SHOWCASE_EVADE_COUNTER_MS));
			}
		}
		else {
			LOG("Showcase :: evade probe: %ls weapon attack", evader.name.data());
			if(!evader.dead) ShowcaseWeaponAt(evader, other);
			showcase.evadeProbe = 0;
		}
	}
	if(showcase.done) return;
	if(localTime < showcase.next) {
		if(showcase.minNext == Time::ZERO || localTime < showcase.minNext) return;
		bool quiet = remoteInsts.empty() && pendingRemotes.empty() && pendingStatuses.empty() && pendingSummons.empty() &&
			pendingSummonCommands.empty() && skillProgramList.empty() && showcase.comboSkill == 0 && showcase.stanceShots == 0 && showcase.evadeProbe == 0 && showcase.followShooter < 0;
		const Player& qc = players[showcase.prevCaster];
		const Player& qd = players[showcase.prevDummy];
		if(qc.clientHd != ClientHandle::INVALID && !showcase.castSeen) quiet = false;
		if(qc.casting.skillID != SkillID::INVALID || localTime < qc.dashUntil || localTime < qd.pushUntil || localTime < qd.ccUntil) quiet = false;
		if(qc.Main().stance != 0 && TimeDiffSec(TimeDiff(showcase.castAt, localTime)) < SHOWCASE_STANCE_WINDOW_SEC) quiet = false;
		foreach_const(pj, projectileList) if(pj->ownerIndex == qc.index) quiet = false;
		foreach_const(ri, remoteInsts) if(ri->ownerIndex == qc.index && ri->remote && (ri->remote->damageGroup == Remote::DamageGroup::eENEMY || !ri->remote->onDestroy.empty() || !ri->remote->onTime.empty() || ri->remote->destroyCreateRemote != RemoteIdx::INVALID)) quiet = false;
		foreach_const(npc, actorNpcList) {
			if(npc->ownerIndex == qc.index && npc->ai != SummonInfo::AI_RELAY && npc->despawnAt != Time::ZERO && localTime < npc->despawnAt) { quiet = false; break; }
		}
		foreach_const(st, activeStatuses) {
			if(!quiet) break;
			if((st->targetIndex == qc.index || st->targetIndex == qd.index) && st->info && !st->info->permanent && localTime < st->until) quiet = false;
		}
		if(!quiet) { showcase.quietSince = Time::ZERO; return; }
		if(showcase.quietSince == Time::ZERO) { showcase.quietSince = localTime; return; }
		if(TimeDiffSec(TimeDiff(showcase.quietSince, localTime)) < SHOWCASE_QUIET_SEC) return;
		LOG("Showcase :: measure ended early (quiet)");
	}
	showcase.minNext = Time::ZERO;
	showcase.quietSince = Time::ZERO;
	const auto& content = GetGameXmlContent();
	eastl::fixed_vector<u32,16,false> bots;
	Player* human = nullptr;
	foreach(it, players) {
		if(it->clientHd == ClientHandle::INVALID) bots.push_back(it->index);
		else if(!human) human = &*it;
	}
	if(!human || bots.size() < 2) return;

	if(!showcase.started) {
		vec3 sum[2] = { vec3(0), vec3(0) };
		i32 cnt[2] = { 0, 0 };
		f32 topZ = -1e9f;
		foreach_const(it, players) {
			sum[it->team & 1] += it->spawnPos; cnt[it->team & 1]++;
			topZ = MAX(topZ, it->spawnPos.z);
		}
		if(cnt[0] == 0 || cnt[1] == 0) return;
		const vec3 mid = (sum[0] / (f32)cnt[0] + sum[1] / (f32)cnt[1]) * 0.5f;
		showcase.mid = vec3(mid.x, mid.y, topZ + 300.0f);
		showcase.started = true;
		showcase.phase = 0;
		showcase.yaw = 0;
		showcase.dummySlot = (i32)bots.size() - 1;
		TeleportPlayer(players[bots[0]], showcase.mid, 0);
		showcase.next = TimeAdd(localTime, TimeMsToTime(2500));
		LOG("Showcase :: start wall=%lld bots=%d probe=(%.0f, %.0f, %.0f) human=%ls", ShowcaseWallMs(), (i32)bots.size(), showcase.mid.x, showcase.mid.y, showcase.mid.z, human->name.data());
		return;
	}
	static const f32 diag = 0.70710678f;
	static const vec2 probeDirs[8] = { vec2(1, 0), vec2(-1, 0), vec2(0, 1), vec2(0, -1), vec2(diag, diag), vec2(-diag, diag), vec2(diag, -diag), vec2(-diag, -diag) };
	static const f32 probeDist[4] = { 600.0f, 1200.0f, 1800.0f, 2400.0f };
	const f32 flatLen = showcase.flatLen;
	const i32 probes = MIN((i32)bots.size(), 4);
	if(showcase.phase == 0) {
		if(showcase.candidates.empty()) showcase.candidates.push_back(vec2(showcase.mid));
		showcase.phase = 3;
		TeleportPlayer(players[bots[0]], vec3(showcase.candidates[showcase.candIdx], showcase.mid.z), 0);
		showcase.next = TimeAdd(localTime, TimeMsToTime(2500));
		return;
	}
	if(showcase.phase == 3) {
		showcase.groundZ = players[bots[0]].body->GetWorldPos().z;
		showcase.probeDir = 0;
		showcase.phase = 2;
		const vec2 c = showcase.candidates[showcase.candIdx];
		for(int k = 0; k < probes; k++) TeleportPlayer(players[bots[k]], vec3(c + probeDirs[0] * probeDist[k], showcase.mid.z), 0);
		showcase.next = TimeAdd(localTime, TimeMsToTime(2500));
		return;
	}
	if(showcase.phase == 2) {
		const vec2 c = showcase.candidates[showcase.candIdx];
		i32 flatCount = 0;
		bool broken = false;
		for(int k = 0; k < probes; k++) {
			const vec3 lp = players[bots[k]].body->GetWorldPos();
			if(fabsf(lp.z - showcase.groundZ) <= 15.0f && !broken) flatCount++;
			else broken = true;
			if(lp.z < showcase.groundZ - 50.0f && lp.z > showcase.groundZ - 400.0f && !showcase.candidates.full()) showcase.candidates.push_back(vec2(lp));
		}
		f32 len = flatCount > 0 ? probeDist[flatCount - 1] : 0.0f;
		if(len > 0) {
			const vec2 d = probeDirs[showcase.probeDir];
			const vec2 side = vec2(-d.y, d.x);
			for(f32 off : { 0.0f, -OBSTRUCT_SIDE, OBSTRUCT_SIDE }) {
				for(f32 h : { OBSTRUCT_JUMPABLE_HEIGHT }) {
					f32 hitDist = 0;
					const vec2 from = c + side * off;
					if(physics.RaycastStatic(vec3(from, showcase.groundZ + h), vec3(from + d * len, showcase.groundZ + h), &hitDist)) len = MIN(len, hitDist);
				}
			}
		}
		LOG("Showcase :: probe from (%.0f, %.0f) z=%.0f dir=(%.2f, %.2f) flat length=%.0f (ground %.0f)", c.x, c.y, showcase.groundZ, probeDirs[showcase.probeDir].x, probeDirs[showcase.probeDir].y, len, flatCount > 0 ? probeDist[flatCount - 1] : 0.0f);
		if(len > showcase.bestLen) {
			showcase.bestLen = len;
			showcase.bestDir = probeDirs[showcase.probeDir];
			showcase.bestCenter = c;
			showcase.bestZ = showcase.groundZ;
		}
		if(flatCount < probes) {
			if(showcase.probeDir < 7) {
				showcase.probeDir++;
				for(int k = 0; k < probes; k++) TeleportPlayer(players[bots[k]], vec3(c + probeDirs[showcase.probeDir] * probeDist[k], showcase.mid.z), 0);
				showcase.next = TimeAdd(localTime, TimeMsToTime(2500));
				return;
			}
			if(showcase.candIdx + 1 < (i32)showcase.candidates.size() && showcase.candIdx < 10) {
				showcase.candIdx++;
				TeleportPlayer(players[bots[0]], vec3(showcase.candidates[showcase.candIdx], showcase.mid.z), 0);
				showcase.phase = 3;
				showcase.next = TimeAdd(localTime, TimeMsToTime(2500));
				return;
			}
			LOG("Showcase :: no full flat line, best length=%.0f", showcase.bestLen);
		}
		showcase.dir = showcase.bestDir;
		showcase.groundZ = showcase.bestZ;
		showcase.mid = vec3(showcase.bestCenter, showcase.mid.z);
		showcase.flatLen = MAX(showcase.bestLen - 350.0f, 0.0f);
		showcase.phase = 0;
	}
	if(showcase.phase == 0) {
		const vec3 ground = vec3(0, 0, showcase.groundZ);
		showcase.anchor = vec3(vec2(showcase.mid) + showcase.dir * 350.0f, ground.z);
		showcase.phase = 1;
		if(Config().DevShowcaseSteps != 0) {
			while(showcase.step <= 7 && !(Config().DevShowcaseSteps & (1 << showcase.step))) showcase.step++;
		}
		foreach(it, players) {
			if(it->clientHd == ClientHandle::INVALID) TeleportPlayer(*it, it->spawnPos, 0);
		}
		const vec2 perp = vec2(-showcase.dir.y, showcase.dir.x);
		const vec3 humanPos = showcase.anchor + vec3(showcase.dir * -250.0f - perp * 250.0f, 0);
		TeleportPlayer(*human, humanPos, 0);
		replication->SendSelfTeleport(human->clientHd, human->Main().UID, humanPos, 0);
		LOG("Showcase :: ground z=%.0f anchor=(%.0f, %.0f, %.0f)", ground.z, showcase.anchor.x, showcase.anchor.y, showcase.anchor.z);
		showcase.next = TimeAdd(localTime, TimeMsToTime(2000));
		return;
	}

	if(showcase.casterSlot >= (i32)bots.size()) {
		showcase.done = true;
		LOG("Showcase :: done wall=%lld", ShowcaseWallMs());
		return;
	}

	if(Config().DevGxTest == GX_EMP_TEST_MODE) {
		UpdateGxEmpTest(*human);
		return;
	}
	if(Config().DevGxTest == GX_PREVIEW_MODE) {
		UpdateGxPreview(*human);
		return;
	}
	if(Config().DevGxTest) {
		foreach_const(b, bots) {
			if(players[*b].team == human->team && players[*b].Main().classType == ClassType::SHUTTLE) { UpdateGxTest(*human, players[*b]); return; }
		}
		LOG("GxTest :: no allied GX-09 bot");
		showcase.done = true;
		return;
	}

	const bool mirror = Config().DevMirror != 0;
	u32 botCasterIndex = bots[showcase.casterSlot];
	if(mirror) {
		foreach_const(b, bots) {
			if(players[*b].team == human->team) { botCasterIndex = *b; break; }
		}
	}
	Player& botCaster = players[botCasterIndex];
	u32 dummyIndex = bots[showcase.dummySlot];
	foreach_const(b, bots) {
		if(*b != botCaster.index && players[*b].team != botCaster.team) { dummyIndex = *b; break; }
	}
	const i32 turn = mirror ? showcase.mirrorTurn : 0;
	Player& caster = turn == 1 ? *human : turn == 2 ? players[dummyIndex] : botCaster;
	Player* dummyPtr = turn == 2 ? human : &players[dummyIndex];
	{
		const i32 stepNow = showcase.step;
		const GameXmlContent::SkillInput* preIn = stepNow >= 1 && stepNow <= 5 ? GetGameXmlContent().FindSkillInput(180000000 + (i32)botCaster.Main().classType * 10000 + stepNow * 10) : nullptr;
		if(mirror && preIn && preIn->friendlyOnly) {
			foreach_const(b, bots) {
				if(*b != caster.index && players[*b].team == caster.team) { dummyPtr = &players[*b]; break; }
			}
		}
	}
	Player& dummy = *dummyPtr;
	const vec3 casterPos = showcase.anchor;
	vec3 dummyPos = showcase.anchor + vec3(showcase.dir * 650.0f, 0);

	if(caster.dead || dummy.dead) {
		if(caster.dead) RespawnPlayer(caster);
		if(dummy.dead) RespawnPlayer(dummy);
		showcase.next = TimeAdd(localTime, TimeMsToTime(1500));
		return;
	}

	if(showcase.pendingResult) {
		showcase.pendingResult = false;
		const Player& pd = players[showcase.prevDummy];
		const i32 dealt = pd.damageTaken - showcase.lastDummyHp;
		const i32 hits = pd.hitsTaken - showcase.lastDummyHits;
		const i32 statusHits = pd.statusesTaken - showcase.lastDummyStatuses;
		const bool hit = hits > 0 || statusHits > 0;
		const bool ok = showcase.prevExpect < 0 || (showcase.prevExpect == 1) == hit;
		if(mirror && human->clientPosCount > 0 && glm::length(vec2(human->placedAt)) > 1.0f)
			LOG("Mirror :: human placed=(%.0f, %.0f) client=(%.0f, %.0f, %.0f) server=(%.0f, %.0f) diff=%.0f updates=%d", human->placedAt.x, human->placedAt.y,
				human->lastClientPos.x, human->lastClientPos.y, human->lastClientPos.z, human->body->GetWorldPos().x, human->body->GetWorldPos().y,
				glm::length(vec2(human->lastClientPos) - vec2(human->placedAt)), human->clientPosCount);
		human->placedAt = vec3(0);
		LOG("Showcase :: result hit=%d expect=%d %s damage=%d hits=%d dummyZ=%.0f groundZ=%.0f", hit ? 1 : 0, showcase.prevExpect, ok ? "OK" : "MISMATCH", dealt, hits, pd.body->GetWorldPos().z, showcase.groundZ);
		foreach(tp, players) ToggleOff(*tp);
		if(mirror && human->lastClientAction != ActionStateID::INVALID) {
			const f64 held = TimeDiffSec(TimeDiff(human->lastClientActionSince, localTime));
			const bool busy = GetGameXmlContent().IsCastAction(human->lastClientAction) && human->casting.skillID == SkillID::INVALID;
			LOG("Mirror :: human client state=%s for %.1fs casting=%d%s", ActionStateToString(human->lastClientAction), held, (i32)human->casting.skillID, busy && held > MIRROR_STUCK_STATE_SEC ? " STUCK" : "");
		}
		{
			const Player& pc = players[showcase.prevCaster];
			eastl::fixed_string<char,160,false> sts;
			for(i32 id : showcase.statusesSeen) sts.append_sprintf("%d,", id);
			LOG("Showcase :: measure casterMoved=%d dummyMoved=%d firstHitMs=%lld firstDamage=%d mana=%d stamina=%d statuses=%s",
				(i32)glm::length(vec2(pc.body->GetWorldPos()) - showcase.casterStart), (i32)glm::length(vec2(pd.body->GetWorldPos()) - showcase.dummyStart),
				showcase.firstHitMs, showcase.firstHitDamage, (i32)(pc.mana >= 0 && showcase.casterMana0 >= 0 ? showcase.casterMana0 - pc.mana : 0.0f),
				(i32)(pc.stamina >= 0 && showcase.casterStamina0 >= 0 ? showcase.casterStamina0 - pc.stamina : 0.0f), sts.c_str());
		}
	}

	const ClassType cls = caster.Main().classType;
	static const char* slotNames[] = { "WEAPON", "SKILL_1", "SKILL_2", "SKILL_3", "SKILL_4", "ULTIMATE", "SHIRK", "TAG" };
	const i32 step = showcase.step;

	foreach(r, remoteInsts) {
		if(r->clientRemoteID != 0) replication->SendDestroyRemoteToAll(r->clientRemoteID);
	}
	remoteInsts.clear();
	pendingRemotes.clear();
	groundAuras.clear();
	projectileList.clear();
	pendingSummonCommands.clear();
	foreach(c, dummy.characters) { (*c)->hp = (*c)->hpMax; replication->SendHpToAll((*c)->UID, (*c)->hp, (*c)->hpMax); }
	const f32 faceCaster = atan2f(-showcase.dir.y, -showcase.dir.x);
	const f32 faceDummy = atan2f(showcase.dir.y, showcase.dir.x);
	foreach(it, players) {
		if(it->index == dummy.index || it->index == caster.index || it->clientHd != ClientHandle::INVALID) continue;
		if(glm::length(vec2(it->body->GetWorldPos()) - vec2(it->spawnPos)) > 50.0f) TeleportPlayer(*it, it->spawnPos, 0);
	}
	TeleportPlayer(dummy, dummyPos, faceCaster);
	TeleportPlayer(caster, casterPos, faceDummy);
	if(&caster == human || &dummy == human) {
		const vec3 humanAt = &caster == human ? casterPos : dummyPos;
		human->placedAt = humanAt;
		if(&caster == human || !showcase.settled) {
			human->clientPosCount = 0;
			replication->SendSelfTeleport(human->clientHd, human->Main().UID, humanAt, &caster == human ? faceDummy : faceCaster, &human->lastClientPos);
		}
		foreach(c, human->characters) { (*c)->hp = (*c)->hpMax; replication->SendHpToAll((*c)->UID, (*c)->hp, (*c)->hpMax); }
	}
	foreach(it, activeStatuses) {
		if(it->targetIndex == dummy.index || it->targetIndex == caster.index) {
			if(!it->info || !it->info->permanent) it->until = localTime;
		}
	}
	dummy.ccUntil = Time::ZERO;
	caster.ccUntil = Time::ZERO;

	i32 skill = 0;
	if(step >= 1 && step <= 5) {
		const i32 byID = 180000000 + (i32)cls * 10000 + step * 10;
		auto f = content.skillMap.find(SkillID(byID));
		if(f != content.skillMap.end() && f->second.type != SkillType::PASSIVE && f->second.type != SkillType::SHIRK) skill = byID;
	}
	if(skill == 0 && step >= 1 && step <= 6) {
		const auto& master = content.GetMaster(cls);
		foreach_const(sid, master.skillIDs) {
			auto f = content.skillMap.find(*sid);
			if(f == content.skillMap.end()) continue;
			const auto& sk = f->second;
			if(step == 6) {
				if(sk.type == SkillType::SHIRK) { skill = (i32)*sid; break; }
				continue;
			}
			eastl::fixed_string<char,48,false> name;
			name.sprintf("SKILL_%d_BEHAVIORSTATE", step);
			if((sk.type == SkillType::NORMAL || sk.type == SkillType::STANCE) && sk.action == ActionStateFromString(name.c_str())) { skill = (i32)*sid; break; }
		}
		if(skill == 0 && step <= 5) {
			foreach_const(sid, master.skillIDs) {
				auto f = content.skillMap.find(*sid);
				if(f == content.skillMap.end()) continue;
				if(f->second.type == SkillType::STANCE && f->second.action == ActionStateID::STANCE_IN_BEHAVIORSTATE && ((i32)*sid / 10) % 10 == step) { skill = (i32)*sid; break; }
			}
		}
	}

	const f32 dummyRadius = HitRadius(dummy);
	f32 range = 0;
	vec3 castPos = dummyPos;
	const char* testName = "-";
	i32 expect = -1;
	i32 chargeHoldMs = 0;
	u8 chargeShotLevel = 0;
	bool countShot = false;
	bool statusShot = false;
	showcase.testCount = 1;
	if(skill != 0) {
		range = content.GetSkillRange(skill);
		if(const GameXmlContent::SkillInput* lock = content.FindSkillInput(skill)) if(lock->lockOn && lock->maxDistance > 0) range = lock->maxDistance;
		if(const GameXmlContent::SkillInput* in = content.FindSkillInput(skill)) if(in->minDistance > range) range = MAX(in->maxDistance, in->minDistance);
		const f32 zone = content.GetSkillTargetRange(skill) * 0.5f;
		const f32 effect = SkillDamageRadius(content, cls, SkillID(skill));
		const f32 reach = SkillReach(content, cls, SkillID(skill));
		struct T { f32 dist; f32 side; const char* name; i32 expect; };
		eastl::fixed_vector<T,8,false> tests;
		if(range > 0) {
			const GameXmlContent::SkillInput* nearIn = content.FindSkillInput(skill);
			const f32 minDist = nearIn ? nearIn->minDistance : 0.0f;
			tests.push_back({ MAX(MAX(range * 0.3f, 150.0f), minDist > 0 ? minDist + 40.0f : 0.0f), 0, "near", 1 });
			tests.push_back({ range - 40.0f, 0, "range_in", 1 });
			const f32 dash = SkillDashDistance(content, cls, SkillID(skill));
			f32 outDist = MAX(MAX(range + MAX(zone, effect), reach), dash > 0 ? dash + MAX(reach, effect) : 0.0f);
			bool outUnknown = SkillReachUnknown(content, cls, SkillID(skill));
			if(const i32 combo = content.GetFirstComboSkill(skill)) {
				const SkillID cs = SkillID(combo);
				const f32 comboDash = SkillDashDistance(content, cls, cs);
				outDist = MAX(outDist, dash + comboDash + MAX(SkillReach(content, cls, cs), SkillDamageRadius(content, cls, cs)));
				outUnknown = outUnknown || SkillReachUnknown(content, cls, cs);
			}
			tests.push_back({ outDist + dummyRadius + 60.0f, 0, "range_out", outUnknown ? -1 : 0 });
		}
		else {
			tests.push_back({ 300.0f, 0, "near", -1 });
		}
		if(zone > 0) {
			const f32 d = range > 0 ? range * 0.65f : 300.0f;
			tests.push_back({ d, zone - 40.0f, "zone_in", 1 });
			tests.push_back({ d, zone + dummyRadius + 60.0f, "zone_out", 0 });
		}
		if(Config().DevValidate) {
			const auto& aims = ValidateAims();
			const auto f = aims.find(skill);
			tests.clear();
			tests.push_back({ f != aims.end() && f->second.dist > 50.0f ? f->second.dist : 300.0f, 0, "official", f != aims.end() ? f->second.expect : -1 });
		}
		showcase.testCount = (i32)tests.size();
		const T& t = tests[MIN(showcase.rangeIdx, (i32)tests.size() - 1)];
		dummyPos = showcase.anchor + vec3(showcase.dir * MIN(t.dist, flatLen), 0);
		castPos = dummyPos + vec3(vec2(-showcase.dir.y, showcase.dir.x) * t.side, 0);
		if(range > 0) {
			const vec2 rel = vec2(castPos) - vec2(casterPos);
			const f32 len = glm::length(rel);
			if(len > range) castPos = vec3(vec2(casterPos) + rel / len * range, castPos.z);
		}
		const bool support = effect == 0 || SkillDamage(SkillID(skill)) <= 0;
		testName = t.dist > flatLen ? "skip_beyond_flat" : (support ? "support" : t.name);
		expect = (t.dist > flatLen || support) ? -1 : t.expect;
		TeleportPlayer(dummy, dummyPos, faceCaster);
	}
	else if(step == 0) {
		const u32 wm = caster.mainCharaID & 1;
		const WeaponIndex weapon = wm == 0 ? caster.mainWeapon : caster.subWeapon;
		const WeaponFiring* wf = content.FindWeaponFiring((i32)weapon);
		const Remote* wr = wf ? content.FindRemote(wf->base) : nullptr;
		f32 wrange = wr ? (wr->maxDistance > 0 ? wr->maxDistance : wr->lifeTime * wr->maxSpeed) : 0;
		if(wf && wf->maxDistance > 0 && wr && wr->behaviorType == Remote::BehaviorType::GRAPH) wrange = wf->maxDistance;
		const Remote* bomb = wr ? content.FindRemote(wr->destroyCreateRemote) : nullptr;
		const f32 splash = bomb ? bomb->boundSize[0] * 0.5f : (wr && wr->behaviorType == Remote::BehaviorType::GRAPH ? wr->boundSize[0] * 0.5f : 0);
		const f32 front = (wr && wr->boundType == Remote::BoundType::E_BOUND_BOX) ? MAX(wr->offsetY + RemoteBoundOffset(*wr) + wr->boundSize[1] * 0.5f, 0.0f) : 0.0f;
		range = wrange;
		struct T { f32 dist; f32 side; const char* name; i32 expect; bool shiftDummy = false; };
		eastl::fixed_vector<T,8,false> tests;
		if(wrange > 0) {
			tests.push_back({ wrange - 40.0f, 0, "range_in", 1 });
			tests.push_back({ wrange + MAX(front, splash) + dummyRadius + WEAPON_HIT_TOLERANCE + 60.0f, 0, "range_out", 0 });
		}
		else tests.push_back({ 300.0f, 0, "near", -1 });
		if(splash > 0) {
			const bool endBlast = wr && wr->maxSpeed > 0 && wr->behaviorType != Remote::BehaviorType::GRAPH && !wf->spawnAtAim;
			const f32 d = wrange > 0 ? (endBlast ? wrange : wrange * 0.6f) : 300.0f;
			tests.push_back({ d, splash - 40.0f, "splash_in", 1, endBlast });
			tests.push_back({ d, splash + dummyRadius + 60.0f, "splash_out", 0, endBlast });
		}
		const Remote* cr = wf && !wf->charge.empty() && wf->chargeTime.size() == wf->charge.size() ? content.FindRemote(wf->charge.back()) : nullptr;
		if(cr) {
			f32 crange = cr->maxDistance > 0 ? cr->maxDistance : cr->lifeTime * cr->maxSpeed;
			if(wf->maxDistance > 0 && cr->behaviorType == Remote::BehaviorType::GRAPH) crange = wf->maxDistance;
			tests.push_back({ crange > 0 ? crange - 40.0f : 300.0f, 0, "charge", crange > 0 ? 1 : -1 });
		}
		const Remote* countRemote = wf && wf->countRate > 0 ? content.FindRemote(wf->countRemote) : nullptr;
		if(countRemote) {
			f32 nrange = countRemote->maxDistance > 0 ? countRemote->maxDistance : countRemote->lifeTime * countRemote->maxSpeed;
			if(wf->maxDistance > 0 && countRemote->behaviorType == Remote::BehaviorType::GRAPH) nrange = wf->maxDistance;
			tests.push_back({ nrange > 0 ? nrange - 40.0f : 300.0f, 0, "count", nrange > 0 ? 1 : -1 });
		}
		const Remote* statusRemote = wf && wf->replaceStatus != 0 ? content.FindRemote(wf->replaceRemote) : nullptr;
		if(statusRemote) {
			f32 srange = statusRemote->maxDistance > 0 ? statusRemote->maxDistance : statusRemote->lifeTime * statusRemote->maxSpeed;
			if(wf->maxDistance > 0 && statusRemote->behaviorType == Remote::BehaviorType::GRAPH) srange = wf->maxDistance;
			tests.push_back({ srange > 0 ? MIN(srange, wrange > 0 ? wrange : srange) - 40.0f : 300.0f, 0, "status", srange > 0 ? 1 : -1 });
		}
		showcase.testCount = (i32)tests.size();
		const T& t = tests[MIN(showcase.rangeIdx, (i32)tests.size() - 1)];
		if(statusRemote && EA::StdC::Strcmp(t.name, "status") == 0) {
			statusShot = true;
			LOG("Showcase :: status shot: owner status %d -> remote %d mult=%.2f", wf->replaceStatus, (i32)wf->replaceRemote, statusRemote->attackMultiplier);
		}
		if(countRemote && EA::StdC::Strcmp(t.name, "count") == 0) {
			countShot = true;
			LOG("Showcase :: count shot every %d -> remote %d mult=%.2f", wf->countRate, (i32)wf->countRemote, countRemote->attackMultiplier);
		}
		if(cr && EA::StdC::Strcmp(t.name, "charge") == 0) {
			chargeShotLevel = (u8)wf->charge.size();
			chargeHoldMs = (i32)(wf->chargeTime.back() * 1000.0f) + MIRROR_CHARGE_HOLD_MARGIN_MS;
			LOG("Showcase :: charge level=%d remote=%d hold=%dms mult=%.2f", (i32)chargeShotLevel, (i32)wf->charge.back(), chargeHoldMs, cr->attackMultiplier);
		}
		dummyPos = showcase.anchor + vec3(showcase.dir * MIN(t.dist, flatLen), 0);
		castPos = dummyPos + vec3(vec2(-showcase.dir.y, showcase.dir.x) * t.side, 0);
		if(t.shiftDummy) eastl::swap(dummyPos, castPos);
		testName = t.dist > flatLen ? "skip_beyond_flat" : t.name;
		expect = t.dist > flatLen ? -1 : t.expect;
		TeleportPlayer(dummy, dummyPos, faceCaster);
	}
	if(Config().DevValidate && !mirror) {
		const vec2 side = Perp(showcase.dir);
		i32 extra = 0;
		bool ally = false;
		foreach(it, players) {
			if(it->index == dummy.index || it->index == caster.index || it->clientHd != ClientHandle::INVALID || it->dead) continue;
			if(it->team != caster.team && extra < 2) {
				TeleportPlayer(*it, dummyPos + vec3(side * (extra == 0 ? VALIDATE_EXTRA_SIDE : -VALIDATE_EXTRA_SIDE), 0), faceCaster);
				foreach(c, it->characters) { (*c)->hp = (*c)->hpMax; replication->SendHpToAll((*c)->UID, (*c)->hp, (*c)->hpMax); }
				extra++;
			}
			else if(it->team == caster.team && !ally) {
				TeleportPlayer(*it, casterPos + vec3(side * VALIDATE_ALLY_SIDE, 0), faceDummy);
				foreach(c, it->characters) { (*c)->hp = (i32)((*c)->hpMax * VALIDATE_CASTER_HP); replication->SendHpToAll((*c)->UID, (*c)->hp, (*c)->hpMax); }
				ally = true;
			}
		}
		foreach(c, caster.characters) { (*c)->hp = (i32)((*c)->hpMax * VALIDATE_CASTER_HP); replication->SendHpToAll((*c)->UID, (*c)->hp, (*c)->hpMax); }
		LOG("Validate :: step master=%d step=%d extra=%d ally=%d", (i32)caster.Main().classType, step, extra, ally ? 1 : 0);
	}
	if(&caster == human || &dummy == human) human->untargetable = false;
	else {
		human->untargetable = true;
		vec3 viewPos = vec3((vec2(casterPos) + vec2(dummyPos)) * 0.5f + Perp(showcase.dir) * 400.0f, showcase.anchor.z);
		const vec2 mid = (vec2(casterPos) + vec2(dummyPos)) * 0.5f;
		static const f32 viewOffsets[] = { 350.0f, -350.0f, 450.0f, -450.0f, 550.0f, -550.0f, 700.0f, -700.0f };
		for(f32 off : viewOffsets) {
			const vec3 c = vec3(mid + Perp(showcase.dir) * off, showcase.groundZ);
			f32 hit = 0;
			if(physics.RaycastStatic(c + vec3(0, 0, 300.0f), c - vec3(0, 0, 300.0f), &hit) && fabsf(300.0f - hit) < 60.0f && !physics.RaycastStatic(vec3(mid, showcase.groundZ + 100.0f), c + vec3(0, 0, 100.0f), nullptr)) {
				viewPos = c;
				break;
			}
		}
		if(glm::length(vec2(viewPos) - vec2(human->body->GetWorldPos())) > 50.0f) {
			TeleportPlayer(*human, viewPos, 0);
			replication->SendSelfTeleport(human->clientHd, human->Main().UID, viewPos, 0, &human->lastClientPos);
		}
	}
	if(!showcase.settled) {
		showcase.settled = true;
		showcase.next = TimeAdd(localTime, TimeMsToTime(&dummy == human ? 1500 : 500));
		showcase.posWaits = 0;
		return;
	}
	if((&caster == human || &dummy == human) && human->clientPosCount > 0 && showcase.posWaits < MIRROR_POS_MAX_WAITS) {
		const vec3 want = human->body->GetWorldPos();
		const f32 off = glm::length(vec2(human->lastClientPos) - vec2(want));
		if(off > MIRROR_POS_TOLERANCE) {
			showcase.posWaits++;
			LOG("Mirror :: waiting for the client at its test spot (%.0f away, try %d)", off, showcase.posWaits);
			replication->SendSelfTeleport(human->clientHd, human->Main().UID, want, &caster == human ? faceDummy : faceCaster, &human->lastClientPos);
			showcase.next = TimeAdd(localTime, TimeMsToTime(MIRROR_POS_RETRY_MS));
			return;
		}
	}
	if(&dummy == human) human->placedAt = human->body->GetWorldPos();
	showcase.settled = false;
	if(&caster == human) {
		const vec3 settledPos = human->body->GetWorldPos();
		human->placedAt = settledPos;
		replication->SendSelfTeleport(human->clientHd, human->Main().UID, settledPos, &caster == human ? faceDummy : faceCaster, &human->lastClientPos);
	}
	if(caster.Main().stance != 0) SetStance(caster.Main(), 0, 0, 0, false);
	for(i32 r = 0; r < 3; r++) {
		const f32 resMax = MasterStat(caster.Main().classType, RESOURCES[r].maxStat);
		if(resMax > 0) caster.res[r] = Config().DevShowcaseFullRes != 0 || resMax <= 1.0f ? resMax : resMax - 1.0f;
	}
	if(Config().DevShowcaseUpgrade > 0) {
		for(u32 m = 0; m < 2; m++) {
			if(caster.characters[m]->classType == ClassType::NONE) continue;
			foreach_const(sid, content.GetMaster(caster.characters[m]->classType).skillIDs) {
				const auto* ups = content.FindSkillUpgrades((i32)*sid);
				if(!ups || ups->empty()) continue;
				const u8 want = (u8)MIN(Config().DevShowcaseUpgrade, (i32)ups->size());
				u8& level = caster.skillLevels[(i32)*sid];
				if(level == want) continue;
				level = want;
				SendSkillLevel(caster, m, *sid);
			}
		}
	}
	i32 skillLevel = 0;
	if(skill != 0) {
		auto lv = caster.skillLevels.find(skill);
		if(lv != caster.skillLevels.end()) skillLevel = lv->second;
	}
	caster.resDirty = true;
	caster.ultGauge = MasterStat(caster.Main().classType, STAT_ULTIMATE);
	caster.ultDirty = true;
	{
		const f32 cd = glm::length(vec2(caster.body->GetWorldPos()) - vec2(casterPos));
		const f32 dd = glm::length(vec2(dummy.body->GetWorldPos()) - vec2(dummyPos));
		const f32 hd = (&caster == human || &dummy == human) && human->clientPosCount > 0 ? glm::length(vec2(human->lastClientPos) - vec2(human->placedAt)) : 0.0f;
		LOG("Showcase :: position check caster=%.0f dummy=%.0f humanClient=%.0f%s", cd, dd, hd, (cd > 60 || dd > 60 || hd > 60) ? " MISPLACED" : "");
	}
	LOG("Showcase :: placement caster=%ls (%.0f, %.0f) dummy=%ls (%.0f, %.0f) player=(%.0f, %.0f) cast=(%.0f, %.0f)", caster.name.data(), casterPos.x, casterPos.y, dummy.name.data(), dummyPos.x, dummyPos.y, human->body->GetWorldPos().x, human->body->GetWorldPos().y, castPos.x, castPos.y);
	showcase.lastDummyHp = dummy.damageTaken;
	showcase.lastDummyHits = dummy.hitsTaken;
	showcase.lastDummyStatuses = dummy.statusesTaken;
	showcase.casterStart = vec2(caster.body->GetWorldPos());
	showcase.dummyStart = vec2(dummy.body->GetWorldPos());
	showcase.castAt = localTime;
	showcase.castSeen = false;
	showcase.firstHitMs = -1;
	showcase.firstHitDamage = 0;
	showcase.statusesSeen.clear();
	showcase.casterMana0 = caster.mana;
	showcase.casterStamina0 = caster.stamina;
	showcase.pendingResult = step <= 6 && (step == 0 || skill != 0);
	showcase.prevExpect = expect;
	showcase.prevDummy = dummy.index;
	showcase.prevCaster = caster.index;
	LOG("Showcase :: wall=%lld master=%d slot=%s skill=%d caster=%ls range=%.0f dist=%.0f test=%s expect=%d level=%d", ShowcaseWallMs(), (i32)cls, slotNames[step], skill, caster.name.data(), range, glm::length(vec2(dummyPos) - vec2(casterPos)), testName, expect, skillLevel);
	if(mirror) LOG("Mirror :: turn=%d caster=%ls target=%ls", turn, caster.name.data(), dummy.name.data());
	if(skill != 0) {
		if(const GameXmlContent::SkillInput* need = content.FindSkillInput(skill)) {
			if(need->checkStatus != 0) {
				ApplyStatus(caster, dummy, need->checkStatus);
				LOG("Showcase :: target marked with status %d required by the skill", need->checkStatus);
			}
		}
	}
	if(Config().DevShowcaseKeepSummons == 0) {
		foreach(npc, actorNpcList) if(npc->ai == SummonInfo::AI_RELAY && npc->ownerIndex != caster.index && localTime < npc->despawnAt) npc->despawnAt = localTime;
	}
	const ActorUID targetUID = dummy.Main().UID;
	const char* mirrorKey = nullptr;
	if(mirror) {
		const i32 base = (i32)human->mainCharaID * 2;
		if(step == 0) mirrorKey = chargeHoldMs > 0 ? "lmbhold" : "lmb";
		else if(step == 5 && skill != 0) mirrorKey = "r";
		else if(step >= 1 && step <= 4 && skill != 0 && (i32)human->skills[base] == skill) mirrorKey = "q";
		else if(step >= 1 && step <= 4 && skill != 0 && (i32)human->skills[base + 1] == skill) mirrorKey = "e";
		else if(step == 6 && skill != 0) mirrorKey = "rmb";
		else if(step == 7) mirrorKey = "wheel";
		if(step == 0 && EA::StdC::Strncmp(testName, "splash", 6) == 0) mirrorKey = nullptr;
	}
	if(&caster == human) {
		const ClassType hc = human->Main().classType;
		human->mana = MasterStat(hc, STAT_MANA);
		human->manaDirty = true;
		human->stamina = MasterStat(hc, STAT_STAMINA);
		human->staminaDirty = true;
		human->ultGauge = MasterStat(hc, STAT_ULTIMATE);
		human->ultDirty = true;

		eastl::fixed_vector<i32,8,false> cools;
		foreach_const(sid, content.GetMaster(hc).skillIDs) {
			const GameXmlContent::SkillInput* in = content.FindSkillInput((i32)*sid);
			if(in && in->coolTimeID != 0 && !cools.full()) cools.push_back(in->coolTimeID);
		}
		replication->SendCooltimeReset(human->clientHd, human->Main().UID, Slice<const i32>(cools.data(), cools.size()));
		const GameXmlContent::SkillInput* in = skill != 0 ? content.FindSkillInput(skill) : nullptr;
		const i32 combo = skill != 0 ? content.GetFirstComboSkill(skill) : 0;
		const char* followKey = "none";
		if(const GameXmlContent::SkillStance* st = skill != 0 ? content.FindSkillStance(skill) : nullptr) {
			const i32 hb = (i32)human->mainCharaID * 2;
			foreach_const(sid, content.GetMaster(hc).skillIDs) {
				auto f = content.skillMap.find(*sid);
				if(f == content.skillMap.end() || f->second.type != SkillType::NORMAL) continue;
				if(ShowcaseSkillAction(content, hc, f->second.action, st->stance).stance != st->stance) continue;
				if(human->skills[hb] == *sid) { followKey = "q"; break; }
				if(human->skills[hb + 1] == *sid) { followKey = "e"; break; }
			}
		}
		LOG("Mirror :: follow=%s", followKey);
		LOG("Mirror :: prompt wall=%lld key=%s click=%d combo=%d skill=%d test=%s dist=%.0f aim=(%.0f, %.0f) dironly=%d posdir=%d comboclick=%d combotele=%d pick=%d stance=%d hold=%d", ShowcaseWallMs(), mirrorKey ? mirrorKey : "none",
			in && in->needsClick ? 1 : 0, combo != 0 && content.GetComboAutoCastMs(combo) == 0 ? 1 : 0, skill, testName, glm::length(vec2(dummyPos) - vec2(casterPos)), castPos.x - casterPos.x, castPos.y - casterPos.y, in && in->directionOnly ? 1 : 0, in && in->positionDirection ? 1 : 0, combo != 0 && content.FindSkillInput(combo) && content.FindSkillInput(combo)->needsClick ? 1 : 0, combo != 0 && content.FindSkillInput(combo) && content.FindSkillInput(combo)->teleportEntity ? 1 : 0, in && in->pickedEntity ? 1 : 0, skill != 0 && content.FindSkillStance(skill) ? 1 : 0, chargeHoldMs);
	}
	else if(step == 0) {
		const vec2 rel = vec2(castPos) - vec2(casterPos);
		const f32 len = glm::length(rel);
		const vec2 dir = len > 1.0f ? rel / len : vec2(1, 0);
		caster.weaponAimYaw = atan2f(dir.y, dir.x);
		caster.chargeLevel = chargeShotLevel;
		if(statusShot) ApplyStatus(caster, caster, ActiveWeaponFiring(caster, caster.Main().stance).replaceStatus);
		if(countShot) {
			const WeaponFiring& cwf = ActiveWeaponFiring(caster, caster.Main().stance);
			caster.shotCount[caster.mainCharaID & 1] = cwf.countRate - 1;
		}
		if(WeaponIsMelee(caster)) PlayerWeaponFire(caster);
		else PlayerWeaponShot(caster, casterPos, dir, len);
		caster.chargeLevel = 0;
	}
	else if(step >= 1 && step <= 6) {
		const bool targetInRange = range <= 0 || glm::length(vec2(dummyPos) - vec2(casterPos)) <= range;
		if(skill != 0) PlayerCastSkill(caster, SkillID(skill), castPos, Slice<const ActorUID>(&targetUID, targetInRange ? 1 : 0), 0);
		if(step == 6 && skill != 0 && caster.clientHd == ClientHandle::INVALID) {
			showcase.evadeProbe = 2;
			showcase.evadeProbeAt = TimeAdd(localTime, TimeMsToTime(SHOWCASE_EVADE_HIT_MS));
			showcase.evadeCaster = (i32)caster.index;
		}
		showcase.stanceShots = 0;
		if(skill != 0 && caster.clientHd == ClientHandle::INVALID && content.FindSkillStance(skill) && caster.Main().stance != 0) {
			showcase.stanceShots = SHOWCASE_STANCE_SHOTS;
			showcase.stanceShotAt = TimeAdd(localTime, TimeMsToTime(SHOWCASE_STANCE_FIRST_SHOT_MS));
			showcase.stanceShooter = (i32)caster.index;
		}
		i32 combo = skill != 0 ? ComboSkillFor(caster, skill) : 0;
		i32 comboDelayMs = 700;
		if(const GameXmlContent::SkillStance* st = skill != 0 ? content.FindSkillStance(skill) : nullptr) {
			const i32 hb = (i32)human->mainCharaID * 2;
			foreach_const(sid, content.GetMaster(cls).skillIDs) {
				auto f = content.skillMap.find(*sid);
				if(f == content.skillMap.end() || f->second.type != SkillType::NORMAL) continue;
				if(mirror && human->Main().classType == cls && human->skills[hb] != *sid && human->skills[hb + 1] != *sid) continue;
				if(ShowcaseSkillAction(content, cls, f->second.action, st->stance).stance == st->stance) { combo = (i32)*sid; comboDelayMs = 1000; break; }
			}
		}
		if(Config().DevValidate && skill != 0 && step >= 1 && step <= 5 && caster.clientHd == ClientHandle::INVALID) {
			showcase.followShotAt = TimeAdd(localTime, TimeMsToTime(VALIDATE_FOLLOW_SHOT_MS));
			showcase.followShooter = (i32)caster.index;
		}
		if(Config().DevValidate && combo != 0 && !content.FindSkillStance(skill) && (caster.index & 1)) combo = 0;
		if(combo != 0) {
			showcase.comboDepth = 0;
			showcase.comboSkill = combo;
			showcase.comboAt = TimeAdd(localTime, TimeMsToTime(comboDelayMs));
			showcase.comboPos = castPos;
			showcase.comboCaster = (i32)caster.index;
			showcase.comboTarget = targetInRange ? targetUID : ActorUID::INVALID;
		}
	}
	else {
		caster.input.tag = true;
	}

	i32 nextTurn = 0;
	if(mirror) {
		const bool enemyTurn = EA::StdC::Strcmp(testName, "support") != 0 && !(step == 0 && EA::StdC::Strncmp(testName, "splash", 6) == 0) && (step == 0 || step == 7 || skill != 0);
		if(turn == 0) nextTurn = mirrorKey ? 1 : enemyTurn ? 2 : 0;
		else if(turn == 1) nextTurn = enemyTurn ? 2 : 0;
	}
	showcase.mirrorTurn = nextTurn;
	if(nextTurn == 0) {
		if((step == 0 || (skill != 0 && step <= 6)) && showcase.rangeIdx < showcase.testCount - 1) showcase.rangeIdx++;
		else { showcase.rangeIdx = 0; showcase.step++; }
		const i32 stepMask = Config().DevShowcaseSteps;
		if(stepMask != 0) {
			while(showcase.step <= 7 && !(stepMask & (1 << showcase.step))) showcase.step++;
			if(showcase.step > 7) {
				showcase.step = 0;
				while(showcase.step <= 7 && !(stepMask & (1 << showcase.step))) showcase.step++;
				showcase.masterPass++;
				showcase.mastersDone++;
				if(Config().DevShowcaseCount > 0 && showcase.mastersDone >= Config().DevShowcaseCount) {
					showcase.done = true;
					LOG("Showcase :: done wall=%lld", ShowcaseWallMs());
				}
				if(showcase.masterPass >= 2) {
					showcase.masterPass = 0;
					showcase.casterSlot++;
				}
			}
		}
	}
	f32 wait = step == 7 ? 2.0f : 9.0f;
	if(mirror && step != 7) wait = &caster == human ? 10.0f : 8.0f;
	if(step >= 1 && step <= 6 && skill == 0) wait = 0.2f;
	if(showcase.step > 7) {
		showcase.step = 0;
		showcase.masterPass++;
		showcase.mastersDone++;
		if(Config().DevShowcaseCount > 0 && showcase.mastersDone >= Config().DevShowcaseCount) {
			showcase.done = true;
			LOG("Showcase :: done wall=%lld", ShowcaseWallMs());
		}
		if(showcase.masterPass >= 2) {
			showcase.masterPass = 0;
			showcase.casterSlot++;
		}
	}
	showcase.next = TimeAdd(localTime, TimeMsToTime(wait * 1000.0));
	if(step != 7 && wait > SHOWCASE_MIN_MEASURE_SEC) showcase.minNext = TimeAdd(localTime, TimeMsToTime(SHOWCASE_MIN_MEASURE_SEC * 1000.0));
}

void World::GxPreviewPlace(Player& human, f32 enemyDist)
{
	const vec3 base = showcase.anchor;
	const vec2 dir = showcase.dir;
	const vec2 side = vec2(-dir.y, dir.x);
	const f32 yaw = atan2f(dir.y, dir.x);
	MovePlayer(human, base, yaw, true);
	human.res[1] = MasterStat(human.Main().classType, RESOURCES[1].maxStat);
	human.resDirty = true;
	HealPlayer(human, human, human.Main().hpMax);
	i32 e = 0;
	i32 a = 0;
	foreach(it, players) {
		if(it->index == human.index || it->dead) continue;
		if(it->team != human.team) {
			const f32 lateral = (f32)(e - 1) * GX_PREVIEW_SPREAD;
			TeleportPlayer(*it, base + vec3(dir * (enemyDist + (e == 1 ? GX_PREVIEW_SPREAD * 0.5f : 0.0f)) + side * lateral, 0), yaw + PI);
			HealPlayer(*it, *it, it->Main().hpMax);
			e++;
		}
		else {
			TeleportPlayer(*it, base + vec3(-dir * 600.0f - side * (f32)(a + 1) * 500.0f, 0), yaw);
			a++;
		}
	}
}

void World::GxPreviewPrompt(Player& human, const char* key, i32 click, i32 holdMs, i32 count, const vec3& target, i32 enemy)
{
	const vec2 aim = vec2(target) - vec2(human.body->GetWorldPos());
	LOG("Preview :: prompt key=%s click=%d hold=%d count=%d aim=(%.0f, %.0f) wall=%lld enemy=%d", key, click, holdMs, count, aim.x, aim.y, ShowcaseWallMs(), enemy);
}

void World::UpdateGatlingPreview(Player& human)
{
	const vec3 base = showcase.anchor;
	const vec2 dir = showcase.dir;
	const vec3 enemies = base + vec3(dir * GX_PREVIEW_ENEMY_DIST, 0);
	const bool second = ((i32)human.skills[(i32)human.mainCharaID * 2] % 100) >= 30;
	auto target = [&]() {
		const Player* best = nullptr;
		f32 bestDist = 0;
		foreach_const(it, players) {
			if(it->team == human.team || it->dead) continue;
			const f32 d = glm::length(vec2(it->body->GetWorldPos()) - vec2(human.body->GetWorldPos()));
			if(!best || d < bestDist) { best = &*it; bestDist = d; }
		}
		return best ? best->body->GetWorldPos() : enemies;
	};
	auto clip = [&](const char* name, i32 phase, i32 ms) {
		LOG("Preview :: clip %s wall=%lld", name, ShowcaseWallMs());
		showcase.gxPhase = phase;
		showcase.next = TimeAdd(localTime, TimeMsToTime(ms));
	};
	switch(showcase.gxPhase) {
		case 0: {
			GxPreviewPlace(human, GX_PREVIEW_ENEMY_DIST);
			LOG("Preview :: start pass=%d skills=%d %d wall=%lld", second ? 1 : 0, (i32)human.skills[(i32)human.mainCharaID * 2], (i32)human.skills[(i32)human.mainCharaID * 2 + 1], ShowcaseWallMs());
			clip("intro", second ? 20 : 10, 3000);
		} break;
		case 10: GxPreviewPrompt(human, "lmb", 1, 0, 8, target(), 1); clip("weapon", 11, 5000); break;
		case 11: GxPreviewPlace(human, GX_PREVIEW_ENEMY_DIST); clip("reset", 12, 1500); break;
		case 12: GxPreviewPrompt(human, "rmb", 1, 0, 1, enemies); clip("dodge", 13, 3500); break;
		case 13: GxPreviewPlace(human, GX_PREVIEW_ENEMY_DIST); clip("reset", 14, 1500); break;
		case 14: GxPreviewPrompt(human, "q", 1, 0, 1, target(), 1); clip("skill1", 15, 5000); break;
		case 15: GxPreviewPlace(human, GX_PREVIEW_CLOSE_DIST); clip("reset", 16, 2000); break;
		case 16: GxPreviewPrompt(human, "e", 1, 0, 1, target(), 1); clip("skill2", 17, 5000); break;
		case 17: {
			GxPreviewPlace(human, GATLING_PREVIEW_NEAR_DIST);
			human.ultGauge = MasterStat(human.Main().classType, STAT_ULTIMATE);
			human.ultDirty = true;
			clip("reset", 18, 2500);
		} break;
		case 18: GxPreviewPrompt(human, "r", 1, 0, 1, target(), 1); clip("ultimate", 99, 7000); break;
		case 20: GxPreviewPlace(human, GATLING_PREVIEW_NEAR_DIST); clip("reset", 21, 1500); break;
		case 21: GxPreviewPrompt(human, "q", 1, 0, 1, target(), 1); clip("skill3", 22, 5500); break;
		case 22: GxPreviewPlace(human, GX_PREVIEW_ENEMY_DIST); clip("reset", 23, 2000); break;
		case 23: GxPreviewPrompt(human, "e", 1, 0, 1, target(), 1); clip("skill4", 99, 5500); break;
		default: {
			LOG("Showcase :: done wall=%lld", ShowcaseWallMs());
			showcase.done = true;
		} break;
	}
}

void World::UpdateGxPreview(Player& human)
{
	if(human.Main().classType == ClassType::GATLING) {
		UpdateGatlingPreview(human);
		return;
	}
	const vec3 base = showcase.anchor;
	const vec2 dir = showcase.dir;
	const vec2 side = vec2(-dir.y, dir.x);
	const vec3 enemies = base + vec3(dir * GX_PREVIEW_ENEMY_DIST, 0);
	const bool second = ((i32)human.skills[(i32)human.mainCharaID * 2] % 100) >= 30;
	Player* ally = nullptr;
	foreach(it, players) if(it->team == human.team && it->index != human.index && !ally) ally = &*it;
	auto clip = [&](const char* name, i32 phase, i32 ms) {
		LOG("Preview :: clip %s wall=%lld", name, ShowcaseWallMs());
		showcase.gxPhase = phase;
		showcase.next = TimeAdd(localTime, TimeMsToTime(ms));
	};
	switch(showcase.gxPhase) {
		case 0: {
			GxPreviewPlace(human, GX_PREVIEW_ENEMY_DIST);
			LOG("Preview :: start pass=%d skills=%d %d wall=%lld", second ? 1 : 0, (i32)human.skills[(i32)human.mainCharaID * 2], (i32)human.skills[(i32)human.mainCharaID * 2 + 1], ShowcaseWallMs());
			clip("intro", second ? 20 : 10, 3000);
		} break;
		case 10: GxPreviewPrompt(human, "lmb", 1, 0, 5, enemies); clip("weapon", 11, 4000); break;
		case 11: GxPreviewPlace(human, GX_PREVIEW_ENEMY_DIST); clip("reset", 12, 1500); break;
		case 12: GxPreviewPrompt(human, "lmbhold", 1, 1800, 1, enemies); clip("charged", 13, 4000); break;
		case 13: GxPreviewPlace(human, GX_PREVIEW_ENEMY_DIST); clip("reset", 14, 1500); break;
		case 14: GxPreviewPrompt(human, "rmb", 1, 0, 1, enemies + vec3(dir * 300.0f, 0)); clip("dodge", 15, 3500); break;
		case 15: GxPreviewPlace(human, GX_PREVIEW_ENEMY_DIST); clip("reset", 16, 2000); break;
		case 16: GxPreviewPrompt(human, "q", 1, 0, 1, enemies); clip("skill1", 17, 5000); break;
		case 17: GxPreviewPlace(human, GX_PREVIEW_CLOSE_DIST); clip("reset", 18, 2000); break;
		case 18: GxPreviewPrompt(human, "e", 1, 0, 1, base + vec3(dir * GX_PREVIEW_CLOSE_DIST, 0)); clip("skill2", 99, 5000); break;
		case 20: GxPreviewPlace(human, GX_PREVIEW_CLOSE_DIST); clip("reset", 21, 1500); break;
		case 21: GxPreviewPrompt(human, "q", 0, 0, 1, base); clip("skill3", 22, 4500); break;
		case 22: {
			GxPreviewPlace(human, GX_PREVIEW_ENEMY_DIST * 3.0f);
			if(ally) TeleportPlayer(*ally, base + vec3(-side * GX_PREVIEW_RIDER_GAP, 0), atan2f(dir.y, dir.x));
			clip("reset", 23, 2000);
		} break;
		case 23: GxPreviewPrompt(human, "e", 0, 0, 1, base); clip("skill4", 24, 1200); break;
		case 24: {
			if(ally) PlayerInteract(*ally, InteractionObjectOf(human));
			LOG("Preview :: ally boarded=%d wall=%lld", ally && ally->carriedBy >= 0 ? 1 : 0, ShowcaseWallMs());
			GxPreviewPrompt(human, "drive", 0, 2200, 1, base);
			clip("ride", 25, 1000);
		} break;
		case 25: {
			if(human.Main().stance != 0) {
				showcase.next = TimeAdd(localTime, TimeMsToTime(300));
				return;
			}
			clip("dropped", 26, 2500);
		} break;
		case 26: {
			GxPreviewPlace(human, GX_PREVIEW_ENEMY_DIST * 3.0f);
			if(ally) TeleportPlayer(*ally, base + vec3(side * GX_PREVIEW_RECALL_DIST, 0), atan2f(dir.y, dir.x));
			human.ultGauge = MasterStat(human.Main().classType, STAT_ULTIMATE);
			human.ultDirty = true;
			clip("reset", 27, 2500);
		} break;
		case 27: {
			GxPreviewPrompt(human, "r", 1, 0, 1, ally ? ally->body->GetWorldPos() : base);
			clip("ultimate", 28, 6000);
		} break;
		case 28: {
			LOG("Preview :: recall dist=%.0f", ally ? glm::length(vec2(ally->body->GetWorldPos()) - vec2(human.body->GetWorldPos())) : -1.0f);
			clip("end", 99, 1000);
		} break;
		default: {
			LOG("Showcase :: done wall=%lld", ShowcaseWallMs());
			showcase.done = true;
		} break;
	}
}

void World::UpdateGxEmpTest(Player& human)
{
	Player* gx = nullptr;
	foreach(it, players) if(!gx && it->team != human.team && it->Main().classType == ClassType::SHUTTLE) gx = &*it;
	if(!gx) {
		LOG("GxTest :: no enemy GX-09");
		LOG("Showcase :: done wall=%lld", ShowcaseWallMs());
		showcase.done = true;
		return;
	}
	const vec2 side = vec2(-showcase.dir.y, showcase.dir.x);
	switch(showcase.gxPhase) {
		case 0: {
			MovePlayer(human, showcase.anchor, 0, true);
			TeleportPlayer(*gx, showcase.anchor + vec3(side * GX_TEST_EMP_DIST, 0), atan2f(-side.y, -side.x));
			showcase.gxPhase = 1;
			showcase.next = TimeAdd(localTime, TimeMsToTime(3000));
		} break;
		case 1: {
			LOG("GxTest :: emp before human class=%d mana=%.0f energy=%.0f", (i32)human.Main().classType, human.mana, human.res[0]);
			PlayerCastSkill(*gx, SkillID(GX_TEST_EMP_SKILL), gx->body->GetWorldPos(), Slice<const ActorUID>(), 0);
			showcase.gxPhase = 2;
			showcase.next = TimeAdd(localTime, TimeMsToTime(1500));
		} break;
		case 2: {
			LOG("GxTest :: emp after human mana=%.0f energy=%.0f", human.mana, human.res[0]);
			showcase.gxPhase = 3;
			showcase.next = TimeAdd(localTime, TimeMsToTime(2000));
		} break;
		default: {
			LOG("Showcase :: done wall=%lld", ShowcaseWallMs());
			showcase.done = true;
		} break;
	}
}

void World::GxTestDrive(Player& gx, const vec2& dir)
{
	const f32 yaw = atan2f(dir.y, dir.x);
	gx.input.speed = GX_TEST_BASE_MOVESPEED * MasterStat(gx.Main().classType, STAT_MOVESPEED) / 100.0f * (1.0f + StatusModMulSum(gx, StatusMod::MOVESPEED) / 100.0f);
	gx.input.moveTo = gx.body->GetWorldPos() + vec3(dir * GX_TEST_DRIVE_STEP * 4.0f, 0);
	gx.input.rot.upperYaw = yaw;
	gx.input.rot.bodyYaw = yaw;
}

void World::UpdateGxTest(Player& human, Player& gx)
{
	const vec3 base = showcase.anchor;
	const vec2 dir = showcase.dir;
	const vec2 side = vec2(-dir.y, dir.x);
	const ActiveStatus* passive = FindPassiveOfType(gx, StatusType::GX09);
	if(!passive && showcase.gxPhase == 40) {
		LOG("GxTest :: after gx death human boarded=%d untargetable=%d", human.carriedBy >= 0 ? 1 : 0, human.untargetable ? 1 : 0);
		LOG("Showcase :: done wall=%lld", ShowcaseWallMs());
		showcase.done = true;
		return;
	}
	if(!passive) {
		LOG("GxTest :: GX-09 passive missing");
		showcase.done = true;
		return;
	}
	const SkillID transport = SkillID((i32)passive->info->ParamInt("_ShuttleSkillIndex"));
	const SkillID teleporter = SkillID((i32)passive->info->ParamInt("_MassiveTeleportIndex"));
	switch(showcase.gxPhase) {
		case 0: {
			TeleportPlayer(gx, base, atan2f(dir.y, dir.x));
			const vec3 humanPos = base + vec3(dir * GX_TEST_BOARD_GAP, 0);
			TeleportPlayer(human, humanPos, atan2f(-dir.y, -dir.x));
			replication->SendSelfTeleport(human.clientHd, human.Main().UID, humanPos, 0);
			gx.res[1] = MasterStat(gx.Main().classType, RESOURCES[1].maxStat);
			gx.resDirty = true;
			gx.ultGauge = MasterStat(gx.Main().classType, STAT_ULTIMATE);
			gx.ultDirty = true;
			LOG("GxTest :: phase board gx=%ls human=%ls gap=%.0f", gx.name.data(), human.name.data(), GX_TEST_BOARD_GAP);
			showcase.gxPhase = 1;
			showcase.next = TimeAdd(localTime, TimeMsToTime(2000));
		} break;
		case 1: {
			PlayerCastSkill(gx, transport, gx.body->GetWorldPos(), Slice<const ActorUID>(), 0);
			LOG("GxTest :: press F wall=%lld", ShowcaseWallMs());
			showcase.gxPhase = 2;
			showcase.gxStep = 0;
			showcase.next = TimeAdd(localTime, TimeMsToTime(2500));
		} break;
		case 2: {
			GxTestDrive(gx, dir);
			LOG("GxTest :: drive gx=(%.0f, %.0f) human=(%.0f, %.0f) boarded=%d speed=%.0f", gx.body->GetWorldPos().x, gx.body->GetWorldPos().y, human.body->GetWorldPos().x, human.body->GetWorldPos().y, human.carriedBy >= 0 ? 1 : 0, gx.input.speed);
			if(++showcase.gxStep >= GX_TEST_DRIVE_STEPS) showcase.gxPhase = 3;
			if(Config().DevGxTest == GX_DEATH_TEST_MODE && showcase.gxStep == GX_TEST_DRIVE_STEPS / 2) {
				foreach(it, players) {
					if(it->team == gx.team || it->dead) continue;
					const i32 hpBefore = human.Main().hp;
					DamagePlayer(*it, human, SkillID::INVALID, GX_TEST_PASSENGER_HIT, nullptr, 0, RemoteIdx::INVALID, vec2(0));
					LOG("GxTest :: passenger hit hp %d -> %d", hpBefore, human.Main().hp);
					DamagePlayer(*it, gx, SkillID::INVALID, gx.Main().hpMax * 2, nullptr, 0, RemoteIdx::INVALID, vec2(0));
					break;
				}
				LOG("GxTest :: gx killed mid ride dead=%d", gx.dead ? 1 : 0);
				showcase.gxPhase = 40;
			}
			showcase.next = TimeAdd(localTime, TimeMsToTime(500));
		} break;
		case 40: {
			LOG("GxTest :: after gx death human boarded=%d untargetable=%d dist=%.0f", human.carriedBy >= 0 ? 1 : 0, human.untargetable ? 1 : 0, glm::length(vec2(human.body->GetWorldPos()) - vec2(gx.body->GetWorldPos())));
			LOG("Showcase :: done wall=%lld", ShowcaseWallMs());
			showcase.done = true;
		} break;
		case 3: {
			if(gx.Main().stance != 0) {
				GxTestDrive(gx, dir);
				showcase.next = TimeAdd(localTime, TimeMsToTime(500));
				return;
			}
			gx.input.moveTo = gx.body->GetWorldPos();
			LOG("GxTest :: transport ended human boarded=%d dist=%.0f", human.carriedBy >= 0 ? 1 : 0, glm::length(vec2(human.body->GetWorldPos()) - vec2(gx.body->GetWorldPos())));
			const vec3 away = gx.body->GetWorldPos() + vec3(side * GX_TEST_RECALL_DIST, 0);
			TeleportPlayer(human, away, 0);
			replication->SendSelfTeleport(human.clientHd, human.Main().UID, away, 0);
			showcase.gxPhase = 4;
			showcase.next = TimeAdd(localTime, TimeMsToTime(2000));
		} break;
		case 4: {
			const ActorUID target = human.Main().UID;
			gx.ultGauge = MasterStat(gx.Main().classType, STAT_ULTIMATE);
			gx.ultDirty = true;
			PlayerCastSkill(gx, teleporter, human.body->GetWorldPos(), Slice<const ActorUID>(&target, 1), 0);
			LOG("GxTest :: teleporter cast on human at %.0f", glm::length(vec2(human.body->GetWorldPos()) - vec2(gx.body->GetWorldPos())));
			showcase.gxPhase = 5;
			showcase.next = TimeAdd(localTime, TimeMsToTime(5000));
		} break;
		case 5: {
			LOG("GxTest :: teleporter result dist=%.0f", glm::length(vec2(human.body->GetWorldPos()) - vec2(gx.body->GetWorldPos())));
			Player* enemyGx = nullptr;
			Player* rider = nullptr;
			foreach(it, players) {
				if(it->team == human.team || it->clientHd != ClientHandle::INVALID || it->dead) continue;
				if(!enemyGx && it->Main().classType == ClassType::SHUTTLE) enemyGx = &*it;
				else if(!rider && it->Main().classType != ClassType::SHUTTLE) rider = &*it;
			}
			if(!enemyGx || !rider || !FindPassiveOfType(*enemyGx, StatusType::GX09)) {
				LOG("GxTest :: no enemy GX-09 and rider");
				LOG("Showcase :: done wall=%lld", ShowcaseWallMs());
				showcase.done = true;
				return;
			}
			showcase.gxEnemy = (i32)enemyGx->index;
			showcase.gxRider = (i32)rider->index;
			const vec3 view = human.body->GetWorldPos();
			const vec3 start = view + vec3(side * GX_TEST_ENEMY_VIEW_DIST - dir * (GX_TEST_DRIVE_STEP * GX_TEST_DRIVE_STEPS * 0.5f), 0);
			TeleportPlayer(*enemyGx, start, atan2f(dir.y, dir.x));
			TeleportPlayer(*rider, start - vec3(dir * GX_TEST_BOARD_GAP, 0), atan2f(dir.y, dir.x));
			enemyGx->res[1] = MasterStat(enemyGx->Main().classType, RESOURCES[1].maxStat);
			enemyGx->resDirty = true;
			LOG("GxTest :: enemy phase gx=%ls rider=%ls", enemyGx->name.data(), rider->name.data());
			showcase.gxPhase = 6;
			showcase.next = TimeAdd(localTime, TimeMsToTime(2000));
		} break;
		case 6: {
			Player& enemyGx = players[showcase.gxEnemy];
			const ActiveStatus* enemyPassive = FindPassiveOfType(enemyGx, StatusType::GX09);
			PlayerCastSkill(enemyGx, SkillID((i32)enemyPassive->info->ParamInt("_ShuttleSkillIndex")), enemyGx.body->GetWorldPos(), Slice<const ActorUID>(), 0);
			LOG("GxTest :: enemy transport cast wall=%lld", ShowcaseWallMs());
			showcase.gxPhase = 7;
			showcase.next = TimeAdd(localTime, TimeMsToTime(1000));
		} break;
		case 7: {
			Player& enemyGx = players[showcase.gxEnemy];
			Player& rider = players[showcase.gxRider];
			PlayerInteract(rider, InteractionObjectOf(enemyGx));
			LOG("GxTest :: enemy rider boarded=%d wall=%lld", rider.carriedBy >= 0 ? 1 : 0, ShowcaseWallMs());
			showcase.gxPhase = 8;
			showcase.gxStep = 0;
			showcase.next = TimeAdd(localTime, TimeMsToTime(1500));
		} break;
		case 8: {
			Player& enemyGx = players[showcase.gxEnemy];
			Player& rider = players[showcase.gxRider];
			GxTestDrive(enemyGx, dir);
			LOG("GxTest :: enemy drive step %d gx=(%.0f, %.0f) rider=(%.0f, %.0f) boarded=%d", showcase.gxStep, enemyGx.body->GetWorldPos().x, enemyGx.body->GetWorldPos().y, rider.body->GetWorldPos().x, rider.body->GetWorldPos().y, rider.carriedBy >= 0 ? 1 : 0);
			if(++showcase.gxStep >= GX_TEST_DRIVE_STEPS) showcase.gxPhase = 9;
			showcase.next = TimeAdd(localTime, TimeMsToTime(500));
		} break;
		case 9: {
			Player& enemyGx = players[showcase.gxEnemy];
			Player& rider = players[showcase.gxRider];
			if(enemyGx.Main().stance != 0) {
				GxTestDrive(enemyGx, dir);
				showcase.next = TimeAdd(localTime, TimeMsToTime(500));
				return;
			}
			enemyGx.input.moveTo = enemyGx.body->GetWorldPos();
			LOG("GxTest :: enemy transport ended rider boarded=%d dist=%.0f wall=%lld", rider.carriedBy >= 0 ? 1 : 0, glm::length(vec2(rider.body->GetWorldPos()) - vec2(enemyGx.body->GetWorldPos())), ShowcaseWallMs());
			showcase.gxPhase = 10;
			showcase.next = TimeAdd(localTime, TimeMsToTime(2000));
		} break;
		default: {
			LOG("Showcase :: done wall=%lld", ShowcaseWallMs());
			showcase.done = true;
		} break;
	}
}

void World::ShowcaseWeaponAt(Player& shooter, const Player& target)
{
	const vec3 from = shooter.body->GetWorldPos();
	const vec2 rel = vec2(target.body->GetWorldPos()) - vec2(from);
	const f32 len = glm::length(rel);
	const vec2 dir = len > 1.0f ? rel / len : vec2(cosf(shooter.input.rot.upperYaw), sinf(shooter.input.rot.upperYaw));
	shooter.weaponAimYaw = atan2f(dir.y, dir.x);
	if(WeaponIsMelee(shooter)) PlayerWeaponFire(shooter);
	else PlayerWeaponShot(shooter, from, dir, len);
}

void World::ShowcaseOnCast(const Player& player)
{
	if(!Config().DevMirror || player.clientHd == ClientHandle::INVALID || player.index != showcase.prevCaster || showcase.castSeen) return;
	showcase.castSeen = true;
	showcase.castAt = localTime;
}

bool World::ShowcaseKeepsFullRes(const Player& player) const
{
	return Config().DevMirror && Config().DevShowcaseFullRes && player.clientHd != ClientHandle::INVALID && player.index == showcase.prevCaster && !showcase.castSeen;
}

bool World::ShowcaseIsDummy(const Player& target) const
{
	return Config().DevShowcase && target.index == showcase.prevDummy;
}

void World::ShowcaseOnStatus(const Player& caster, const Player& target, i32 statusID)
{
	if(!ShowcaseIsDummy(target) || caster.team == target.team || showcase.statusesSeen.full()) return;
	if(eastl::find(showcase.statusesSeen.begin(), showcase.statusesSeen.end(), statusID) == showcase.statusesSeen.end()) showcase.statusesSeen.push_back(statusID);
}

void World::ShowcaseOnDamage(const Player& target, i32 damage)
{
	if(!ShowcaseIsDummy(target) || showcase.firstHitMs >= 0) return;
	showcase.firstHitMs = (i64)(TimeDiffSec(TimeDiff(showcase.castAt, localTime)) * 1000.0);
	showcase.firstHitDamage = damage;
}

bool World::ShowcaseKeepsAlive(const Player& target) const
{
	if(target.devGod) return true;
	return Config().DevShowcase && (target.clientHd == ClientHandle::INVALID || Config().DevMirror) && Config().DevGxTest != GX_DEATH_TEST_MODE;
}

static const i64 SHOWCASE_PERF_WINDOW_US = 10000000;

static i64 ShowcaseNowUs()
{
	return (i64)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

ShowcaseTickTimer::ShowcaseTickTimer(ShowcaseState& state_):
	state(state_),
	startUs(Config().DevShowcase ? ShowcaseNowUs() : 0)
{
}

ShowcaseTickTimer::~ShowcaseTickTimer()
{
	if(!Config().DevShowcase) return;
	const i64 now = ShowcaseNowUs();
	const i64 spent = now - startUs;
	if(state.perfWindowStartUs == 0) state.perfWindowStartUs = now;
	state.perfTotalUs += spent;
	state.perfMaxUs = MAX(state.perfMaxUs, spent);
	state.perfTicks++;
	if(now - state.perfWindowStartUs < SHOWCASE_PERF_WINDOW_US) return;
	LOG("Perf :: world update avg=%.1fus max=%lldus ticks=%d", (f64)state.perfTotalUs / MAX(state.perfTicks, 1), state.perfMaxUs, state.perfTicks);
	state.perfWindowStartUs = now;
	state.perfTotalUs = 0;
	state.perfMaxUs = 0;
	state.perfTicks = 0;
}

void World::ShowcaseOnRemoteEnd(const RemoteInst& inst)
{
	if(!Config().DevShowcase) return;
	const Remote& remote = *inst.remote;
	f32 nearest = 999999.0f;
	const Player* nearestPlayer = nullptr;
	foreach_const(it, players) {
		if(it->team == players[inst.ownerIndex].team || it->dead) continue;
		const f32 d = glm::length(vec2(it->body->GetWorldPos()) - vec2(inst.pos));
		if(d < nearest) { nearest = d; nearestPlayer = &*it; }
	}
	const vec2 nrel = nearestPlayer ? vec2(nearestPlayer->body->GetWorldPos()) - vec2(inst.pos) : vec2(0);
	LOG("SkillRemoteEnd :: %d travelled=%.0f hitsLeft=%d nearest=%ls dist=%.0f along=%.0f side=%.0f dz=%.0f", (i32)remote.ID, inst.travelled, inst.hitsLeft, nearestPlayer ? nearestPlayer->name.data() : L"-", nearest,
		glm::dot(nrel, inst.dir), inst.dir.x * nrel.y - inst.dir.y * nrel.x, nearestPlayer ? nearestPlayer->body->GetWorldPos().z - inst.pos.z : 0.0f);
}
