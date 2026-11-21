#pragma once
#include <common/base.h>
#include <common/vector_math.h>
#include <mxm/game_content.h>
#include "world.h"

enum EndDamageStat: u8 { END_DMG_MAX_DEALT = 0, END_DMG_NORMAL_DEALT, END_DMG_SKILL_DEALT, END_DMG_BUILDING_DEALT, END_DMG_TAKEN, END_DMG_MAX_TAKEN, END_DMG_NORMAL_TAKEN, END_DMG_SKILL_TAKEN, END_DMG_HEAL };

static const f32 FIRE_NODE_HEIGHT = 158.0f;
static const f32 MEASURED_JUMPWALL_MAX_HP = 1491.0f;
static const f32 DEFENCE_SCALE = 100.0f;
static const f32 COMBAT_ATTACK_STAT = 0.0f;
static const f32 WEAPON_TARGET_RADIUS = 110.0f;
static const f32 PROJECTILE_HIT_TOLERANCE = 20.0f;
static const f32 WEAPON_PROJECTILE_HIT_HEIGHT = 300.0f;
static const f64 MEASURED_MONSTER_SPAWN_STATE_MIN_MS = 100.0;
static const i32 SUMMON_STATE_LIFTED = 21;
static const i32 STAT_STAMINA = 2;
static const i32 STAT_MOVESPEED = 14;
static const i32 STAT_MANA = 35;
static const u32 STATUS_RESIST_EVENT_PASSIVE = 0;
static const u32 STATUS_RESIST_EVENT_PROTECTED = 2;
static const i32 STAT_SKILLPOWER = 8;
static const i32 STAT_ATTACK = 6;
struct ResourceDef { i32 maxStat; i32 regenStat; bool startFull; i32 tagRegenValueStat; i32 tagRegenRatioStat; };
static const ResourceDef RESOURCES[3] = {
	{ 17, 41, true, 46, 49 },
	{ 36, 40, false, 45, 48 },
	{ 56, 57, true, 26, 30 },
};
static const i32 STAT_ULTIMATE = 37;
static const f32 WEAPON_HIT_TOLERANCE = 50.0f;
static const f32 STATUS_INFINITE_DURATION_SEC = 4294967.295f;

static inline f32 StatusSentDuration(const StatusInfo& st)
{
	return st.durationSec > 0 ? st.durationSec : (st.durationSec < 0 ? STATUS_INFINITE_DURATION_SEC : 0.0f);
}
static const f32 OBSTRUCT_JUMPABLE_HEIGHT = 120.0f;
static const u32 STAT_REASON_REGEN = 4;
static const f32 OBSTRUCT_SIDE = 50.0f;

static inline f32 SkillMoveDuration(const GameXmlContent& content, const char* className, const GameXmlContent::Action& action, i32 animIndex)
{
	foreach_const(st, action.anims) {
		if(st->index != animIndex) continue;
		const GameXmlContent::MoveCurve* curve = content.GetMoveCurve(className, st->animationId);
		if(curve && curve->time > 0) return curve->time;
		const f32 animLen = content.GetAniSeqTime(className, st->animationId) * (f32)MAX(st->loop, 1);
		if(animLen > 0) return animLen;
	}
	return action.seqLength;
}

static inline bool ProjectileConeHolds(const World::Projectile& proj, const vec2& at, f32 radius)
{
	if(proj.coneHalfCos < -1.5f) return true;
	const vec2 rel = at - vec2(proj.start);
	const f32 dist = glm::length(rel);
	if(dist > proj.coneRadius + radius) return false;
	return dist <= radius || glm::dot(rel / dist, proj.dir) >= proj.coneHalfCos;
}

static inline f32 CapsuleOrSphereRadius(const Remote& remote)
{
	return remote.boundType == Remote::BoundType::E_BOUND_CAPSULE ? remote.boundSize[0] : remote.boundSize[0] * 0.5f;
}

static inline f32 RemoteBoundOffset(const Remote& remote)
{
	return remote.forceTargetDir ? -remote.boundOffsetY : remote.boundOffsetY;
}

static inline f32 RemoteDistanceFalloff(const Remote& remote, f32 dist)
{
	if(remote.damageDistStart <= 0 || remote.boundType != Remote::BoundType::E_BOUND_SPHERE) return 1.0f;
	const f32 inner = remote.damageDistStart * 0.5f;
	const f32 outer = remote.boundSize[0] * 0.5f;
	if(dist <= inner || outer <= inner) return 1.0f;
	const f32 edge = remote.damageOnBound / 100.0f;
	return MAX(edge, 1.0f - (1.0f - edge) * (dist - inner) / (outer - inner));
}

static inline f32 SkillDamage(SkillID skillID, i32 depth = 0)
{
	const auto& content = GetGameXmlContent();
	auto found = content.skillMap.find(skillID);
	if(found == content.skillMap.end()) return 0;
	const SkillNormalModel& skill = found->second;
	const SkillNormalLevelModel& level = *skill.getSkillNormalLevelByIndex(0);
	const f32 ttBase = content.GetSkillBaseDamage((i32)skillID);
	const f32 base = ttBase > 0 ? ttBase : level.getBaseDamage();
	if(base <= 0) {
		const i32 comboBase = content.GetComboBaseSkill((i32)skillID);
		if(comboBase != 0 && depth == 0) return SkillDamage(SkillID(comboBase), depth + 1);
	}
	const f32 damage = base + COMBAT_ATTACK_STAT * level.getAttackMultiplier();
	return MAX(damage, 0.0f);
}

static inline const WeaponHeatInfo* ActiveWeaponHeat(const World::Player& player)
{
	const u32 m = player.mainCharaID & 1;
	const WeaponIndex weapon = m == 0 ? player.mainWeapon : player.subWeapon;
	return GetGameXmlContent().FindWeaponHeat((i32)weapon);
}

static inline f32 CurrentHeat(const World::Player& player, const WeaponHeatInfo& h, Time now)
{
	const u32 m = player.mainCharaID & 1;
	if(player.overheatUntil[m] != Time::ZERO && now < player.overheatUntil[m]) return h.maxGauge;
	const f32 idle = player.heatLastShot[m] == Time::ZERO ? 999.0f : (f32)TimeDiffSec(TimeDiff(player.heatLastShot[m], now));
	if(idle <= h.autoRegenDelay || h.autoRegenInterval <= 0) return player.heat[m];
	return MIN(MAX(player.heat[m] + h.autoRegen * (idle - h.autoRegenDelay) / h.autoRegenInterval, 0.0f), h.maxGauge);
}

static inline vec2 SteerToward(const vec2& dir, const vec2& rel, f32 maxTurn)
{
	f32 turn = atan2f(rel.y, rel.x) - atan2f(dir.y, dir.x);
	while(turn > PI) turn -= 2 * PI;
	while(turn < -PI) turn += 2 * PI;
	turn = MAX(MIN(turn, maxTurn), -maxTurn);
	const f32 c = cosf(turn);
	const f32 sn = sinf(turn);
	return vec2(dir.x * c - dir.y * sn, dir.x * sn + dir.y * c);
}

static inline bool WeaponHeatShot(World::Player& player, Time now, Replication* replication, f32 consumptionScale)
{
	const WeaponHeatInfo* h = ActiveWeaponHeat(player);
	if(!h) return true;
	const u32 m = player.mainCharaID & 1;
	if(player.overheatUntil[m] != Time::ZERO) return false;

	player.heat[m] = CurrentHeat(player, *h, now) + h->consumption * consumptionScale;
	player.heatLastShot[m] = now;
	if(player.heat[m] >= h->maxGauge) {
		player.heat[m] = h->maxGauge;
		player.overheatUntil[m] = TimeAdd(now, TimeMsToTime(h->overheatDuration * 1000.0));
		replication->SendGauge(player.clientHd, player.userID, 1, true, h->overheatDuration, (u16)h->maxGauge, (u16)h->maxGauge);
		LOG("Overheat :: %ls locked %.1fs", player.name.data(), h->overheatDuration);
	}
	return true;
}

static inline f32 MasterStat(ClassType classType, i32 statType)
{
	const auto& master = GetGameXmlContent().GetMaster(classType);
	foreach_const(st, master.baseStats) {
		if(st->type == statType) return st->value;
	}
	return 0;
}

static inline vec2 Perp(const vec2& v)
{
	return vec2(-v.y, v.x);
}

static inline f32 SegmentPointDist2D(const vec2& a, const vec2& b, const vec2& p)
{
	const vec2 ab = b - a;
	const f32 len2 = glm::dot(ab, ab);
	f32 t = len2 > 0 ? glm::dot(p - a, ab) / len2 : 0;
	t = clamp(t, 0.0f, 1.0f);
	return glm::length(p - (a + ab * t));
}
