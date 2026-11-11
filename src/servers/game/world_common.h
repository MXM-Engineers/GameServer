#pragma once
#include <common/base.h>
#include <common/vector_math.h>
#include <mxm/game_content.h>
#include "world.h"

static const f32 COMBAT_ATTACK_STAT = 0.0f;
static const f32 WEAPON_TARGET_RADIUS = 110.0f;
static const f32 PROJECTILE_HIT_TOLERANCE = 20.0f;
static const f32 WEAPON_PROJECTILE_HIT_HEIGHT = 300.0f;
static const i32 STAT_STAMINA = 2;
static const i32 STAT_MOVESPEED = 14;
static const i32 STAT_MANA = 35;
struct ResourceDef { i32 maxStat; i32 regenStat; bool startFull; };
static const ResourceDef RESOURCES[3] = {
	{ 17, 41, true },
	{ 36, 40, false },
	{ 56, 57, true },
};
static const i32 STAT_ULTIMATE = 37;
static const f32 WEAPON_HIT_TOLERANCE = 50.0f;
static const f32 OBSTRUCT_JUMPABLE_HEIGHT = 120.0f;
static const f32 OBSTRUCT_SIDE = 50.0f;

static inline f32 SkillMoveDuration(const GameXmlContent& content, ClassType cls, const GameXmlContent::Action& action, i32 animIndex)
{
	f32 animLen = 0;
	foreach_const(st, action.anims) {
		if(st->index != animIndex) continue;
		animLen = content.GetAniSeqTime(content.GetMaster(cls).className.c_str(), st->animationId) * (f32)MAX(st->loop, 1);
		if(animLen > 0) break;
	}
	f32 attachedLife = 0;
	foreach_const(cmd, action.commands) {
		if(cmd->type != ActionCommand::Type::REMOTE || (cmd->animIndex != animIndex && cmd->animIndex != -1)) continue;
		const Remote* r = content.FindRemote(cmd->remote.idx);
		if(!r || !r->attachFireObject || r->lifeTime <= 0) continue;
		const f32 life = (cmd->remote.removeAtAnimEnd && animLen > 0) ? MIN(r->lifeTime, animLen) : r->lifeTime;
		attachedLife = MAX(attachedLife, life);
	}
	if(attachedLife > 0) return attachedLife;
	if(animLen > 0) return animLen;
	return action.seqLength;
}

static inline f32 SkillDamage(SkillID skillID)
{
	const auto& content = GetGameXmlContent();
	auto found = content.skillMap.find(skillID);
	if(found == content.skillMap.end()) return 0;
	SkillNormalModel& skill = const_cast<SkillNormalModel&>(found->second);
	SkillNormalLevelModel& level = *skill.getSkillNormalLevelByIndex(0);
	const f32 ttBase = content.GetSkillBaseDamage((i32)skillID);
	const f32 base = ttBase > 0 ? ttBase : level.getBaseDamage();
	if(base <= 0) {
		const i32 comboBase = content.GetComboBaseSkill((i32)skillID);
		if(comboBase != 0) return SkillDamage(SkillID(comboBase));
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

static inline bool WeaponHeatShot(World::Player& player, Time now, Replication* replication)
{
	const WeaponHeatInfo* h = ActiveWeaponHeat(player);
	if(!h) return true;
	const u32 m = player.mainCharaID & 1;
	if(player.overheatUntil[m] != Time::ZERO) return false;

	player.heat[m] = CurrentHeat(player, *h, now) + h->consumption;
	player.heatLastShot[m] = now;
	if(player.heat[m] >= h->maxGauge) {
		player.heat[m] = h->maxGauge;
		player.overheatUntil[m] = TimeAdd(now, TimeMsToTime(h->overheatDuration * 1000.0));
		replication->SendGaugeToAll(player.userID, 1, true, h->overheatDuration, (u16)h->maxGauge, (u16)h->maxGauge);
		player.heatSent = (i32)h->maxGauge;
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
