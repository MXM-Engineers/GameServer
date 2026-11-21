#include "config.h"
#include "world.h"
#include "world_common.h"
#include <mxm/game_content.h>

static f32 MovespeedMul(f32 percentSum, f32 statPercent)
{
	static const f32 MEASURED_MOVESPEED_MUL_MIN = 0.1f;
	static const f32 MEASURED_MOVESPEED_STAT_MAX = 160.0f;
	const f32 maxMul = statPercent > 0 ? MAX(MEASURED_MOVESPEED_STAT_MAX / statPercent, 1.0f) : 1.6f;
	return MIN(MAX(1.0f + percentSum / 100.0f, MEASURED_MOVESPEED_MUL_MIN), maxMul);
}

static const i32 STAT_STAMINAREGEN = 5;
static const i32 ACTION_FULL_MOVESPEED_PERCENT = 100;
static const i32 STAT_MANAREGEN = 39;
static const i32 STAT_ULTIMATEREGEN = 42;
static const u8 STAT_SKILL_COOLTIME_DISCOUNT = 53;
static const u8 STAT_SKILLDAMAGE_REDUCE_RATIO = 58;
static const f32 MEASURED_NPC_SPEED_MUL_MIN = 0.1f;
static const f64 LAST_CHILD_STATUS_DELAY_MS = 50.0;
static const i32 STAT_TAG_HEALTHREGEN_VALUE = 23;
static const i32 STAT_TAG_COOLTIME = 31;
static const f32 TAG_COOLTIME_AUX = 3.75f;
static const f32 TAG_COOLTIME_MIN = 5.0f;
static const i32 STAT_TAG_STAMINAREGEN_VALUE = 25;
static const i32 STAT_TAG_HEALTHREGEN_RATIO = 27;
static const i32 STAT_TAG_STAMINAREGEN_RATIO = 29;
static const i32 STAT_TAG_MANAREGEN_VALUE = 44;
static const i32 STAT_TAG_MANAREGEN_RATIO = 47;
static const f64 REGEN_INTERVAL_MS = 1000.0;
static const f64 RAGE_DECAY_DELAY_MS = 6000.0;
static const f32 HIDE_DETECT_RADIUS_UNIT = 100.0f;
static const f64 SKILL_POINT_INTERVAL_MS = 35000.0;
static const f64 CHASE_UPDATE_INTERVAL_MS = 200.0;
static const f64 BATTLE_STAGE_SEC = 1.0;
static const f32 BATTLE_SPEED_RATIO_ACTIVE = 535.0f / 620.0f;
static const f32 BATTLE_SPEED_RATIO_RECENT = 578.0f / 620.0f;
static const i32 SKILL_UPGRADE_SLOT_ULTIMATE = 12;
static const f32 STANCE_EXIT_DURATION = -1.0f;
static const f32 MOVEMENT_YAW_MIN_DIST = 10.0f;
static const i32 ULTIMATE_SKILL_ID_SUFFIX = 50;

static f32 MovementYaw(const World::Player& player, f32 fallback)
{
	const vec2 moving = vec2(player.input.moveTo) - vec2(player.body->GetWorldPos());
	return glm::length(moving) > MOVEMENT_YAW_MIN_DIST ? atan2f(moving.y, moving.x) : fallback;
}

void World::UpdateUltimateGauges(f32 delta)
{
	foreach(it, players) {
		if(it->clientHd == ClientHandle::INVALID && (Config().DevShowcase || Config().DevMirror)) continue;
		const ClassType cls = it->Main().classType;
		const ClassType benchCls = it->Sub().classType;

		UpdateWeaponHeat(*it);

		if(localTime >= it->tNextSpeedRefresh) {
			it->tNextSpeedRefresh = TimeAdd(localTime, TimeMsToTime(1000));
			it->moveSpeedSent = -1.0f;
		}
		{
			const f32 base = MasterStat(cls, STAT_MOVESPEED) + StatBonus(*it, STAT_MOVESPEED);
			const f32 firingMul = localTime < it->firingStatUntil || it->weaponState == WEAPON_STATE_CHARGE ? it->firingMoveSpeedMul : 0.0f;
			const f32 speed = base * MovespeedMul(StatusModMulSum(*it, StatusMod::MOVESPEED) + firingMul, base);
			if(base > 0 && fabsf(speed - it->moveSpeedSent) > 0.01f) {
				it->moveSpeedSent = speed;
				replication->SendStatToAll(it->Main().UID, STAT_MOVESPEED, speed, speed);
				if(firingMul == 0.0f && fabsf(speed - base) > 0.01f) LOG("MoveSpeed :: %ls %.0f%%", it->name.data(), speed);
			}
		}

		const f32 ultMax = MAX(MasterStat(cls, STAT_ULTIMATE), 1.0f);
		const f32 staminaMax = MasterStat(cls, STAT_STAMINA);
		const f32 manaMax = MasterStat(cls, STAT_MANA);
		eastl::array<f32,3> resMax;
		for(int r = 0; r < 3; r++) resMax[r] = MasterStat(cls, RESOURCES[r].maxStat);
		if(it->ultGauge < 0) { it->ultGauge = 0; it->ultDirty = true; }
		if(staminaMax > 0 && it->stamina < 0) { it->stamina = staminaMax; it->staminaDirty = true; }
		if(manaMax > 0 && it->mana < 0) { it->mana = manaMax; it->manaDirty = true; }
		for(int r = 0; r < 3; r++) {
			if(resMax[r] > 0 && it->res[r] < 0) { it->res[r] = ResourceStartsFull(cls, r, RESOURCES[r].startFull) ? resMax[r] : 0; it->resDirty = true; }
		}

		const ActorMaster& sub = it->Sub();
		const f32 benchStaminaMax = MasterStat(benchCls, STAT_STAMINA);
		const f32 benchManaMax = MasterStat(benchCls, STAT_MANA);
		eastl::array<f32,3> benchResMax;
		for(int r = 0; r < 3; r++) benchResMax[r] = MasterStat(benchCls, RESOURCES[r].maxStat);
		if(benchStaminaMax > 0 && it->benchStamina < 0) {
			it->benchStamina = benchStaminaMax;
			replication->SendStatToAll(sub.UID, STAT_STAMINA, it->benchStamina, benchStaminaMax);
		}
		if(benchManaMax > 0 && it->benchMana < 0) {
			it->benchMana = benchManaMax;
			replication->SendStatToAll(sub.UID, STAT_MANA, it->benchMana, benchManaMax);
		}
		for(int r = 0; r < 3; r++) {
			if(benchResMax[r] <= 0 || it->benchRes[r] >= 0) continue;
			it->benchRes[r] = ResourceStartsFull(benchCls, r, RESOURCES[r].startFull) ? benchResMax[r] : 0;
			replication->SendStatToAll(sub.UID, RESOURCES[r].maxStat, it->benchRes[r], benchResMax[r]);
		}

		const ResourceGainRule* charging = it->weaponState == WEAPON_STATE_CHARGE ? FindResourceGainRule(cls, ResourceEvent::CHARGING_PER_SEC) : nullptr;
		if(charging && charging->slot >= 0 && charging->slot < 3 && resMax[charging->slot] > 0) {
			f32& pool = it->res[charging->slot];
			pool = MIN(MAX(pool + charging->amount * delta, 0.0f), resMax[charging->slot]);
			if(localTime >= it->tNextChargeSend) {
				it->tNextChargeSend = TimeAdd(localTime, TimeMsToTime(250));
				it->resDirty = true;
			}
		}

		const f32 staminaRegen = (MasterStat(cls, STAT_STAMINAREGEN) + StatBonus(*it, STAT_STAMINAREGEN) + StatusRegenBonus(*it, StatusMod::STAMINAREGEN, false)) * MAX(1.0f + StatusRegenBonus(*it, StatusMod::STAMINA, true) / 100.0f, 0.0f);
		const f32 manaRegen = MasterStat(cls, STAT_MANAREGEN) + StatBonus(*it, STAT_MANAREGEN) + StatusRegenBonus(*it, StatusMod::MANAREGEN, false);
		const f32 rageRegen = MasterStat(cls, RESOURCES[1].regenStat) + StatusRegenBonus(*it, StatusMod::RAGEREGEN, false);
		const f32 energyRegen = MasterStat(cls, RESOURCES[0].regenStat) + StatusRegenBonus(*it, StatusMod::ENERGYREGEN, false);
		{
			const f32 rates[4] = { staminaRegen, manaRegen, rageRegen, energyRegen };
			const i32 statIDs[4] = { STAT_STAMINAREGEN, STAT_MANAREGEN, RESOURCES[1].regenStat, RESOURCES[0].regenStat };
			const bool used[4] = { staminaMax > 0, manaMax > 0, resMax[1] > 0, resMax[0] > 0 };
			for(int k = 0; k < 4; k++) {
				if(!used[k]) continue;
				if(it->regenSent[k] < 0 && !it->regenResend) it->regenSent[k] = rates[k];
				if(!it->regenResend && fabsf(rates[k] - it->regenSent[k]) < 0.001f) continue;
				it->regenSent[k] = rates[k];
				replication->SendStatToAll(it->Main().UID, statIDs[k], 0.0f, rates[k]);
			}
			it->regenResend = false;
		}
		{
			const f32 tagCoolMul = MAX(1.0f + StatusRegenBonus(*it, StatusMod::TAG_COOLTIME, true) / 100.0f, 0.0f);
			const f32 tagHealthBonus = StatusRegenBonus(*it, StatusMod::TAG_HEALTHREGEN, false);
			if(tagCoolMul != it->tagStatSent[0] || tagHealthBonus != it->tagStatSent[1]) {
				const bool first = it->tagStatSent[0] < 0;
				it->tagStatSent[0] = tagCoolMul;
				it->tagStatSent[1] = tagHealthBonus;
				if(!first) {
					for(i32 c = 0; c < 2; c++) {
						const ActorMaster& m = *it->characters[c];
						replication->SendStatToAll(m.UID, STAT_TAG_HEALTHREGEN_VALUE, 0.0f, MasterStat(m.classType, STAT_TAG_HEALTHREGEN_VALUE) + tagHealthBonus);
						replication->SendStatToAll(m.UID, STAT_TAG_COOLTIME, TAG_COOLTIME_AUX * tagCoolMul, MAX(MasterStat(m.classType, STAT_TAG_COOLTIME) * tagCoolMul, TAG_COOLTIME_MIN));
					}
				}
			}
		}

		{
			static const StatusMod::Stat STATUS_STAT_MODS[2] = { StatusMod::SKILL_COOLTIME_DISCOUNT, StatusMod::SKILLDAMAGE_REDUCE };
			static const u8 STATUS_STAT_TYPES[2] = { STAT_SKILL_COOLTIME_DISCOUNT, STAT_SKILLDAMAGE_REDUCE_RATIO };
			const ActorMaster& main = it->Main();
			const bool mainChanged = it->statusStatMain != main.UID;
			for(i32 k = 0; k < 2; k++) {
				const f32 sum = StatusModSum(*it, STATUS_STAT_MODS[k]);
				if(mainChanged && it->statusStatMain != ActorUID::INVALID && it->statusStatSent[k] != 0) {
					for(u32 m = 0; m < it->characters.size(); m++) {
						const ActorMaster& old = *it->characters[m];
						if(old.UID != it->statusStatMain || old.classType == ClassType::NONE) continue;
						replication->SendStatToAll(old.UID, STATUS_STAT_TYPES[k], 0.0f, MasterStat(old.classType, STATUS_STAT_TYPES[k]) + GetGameXmlContent().MasterTraitBonus(old.classType, old.traits, STATUS_STAT_TYPES[k]));
					}
				}
				if(sum == it->statusStatSent[k] && !(mainChanged && sum != 0)) continue;
				it->statusStatSent[k] = sum;
				replication->SendStatToAll(main.UID, STATUS_STAT_TYPES[k], 0.0f, MasterStat(main.classType, STATUS_STAT_TYPES[k]) + StatBonus(*it, STATUS_STAT_TYPES[k]) + sum);
			}
			it->statusStatMain = main.UID;
		}

		{
			static const StatusMod::Stat COMBAT_STAT_MODS[2] = { StatusMod::ATTACK, StatusMod::DEFENCE };
			static const u8 COMBAT_STAT_TYPES[2] = { (u8)StatType::STAT_TYPE_ATTACK, (u8)StatType::STAT_TYPE_DEFENCE };
			const ActorMaster& main = it->Main();
			const bool mainChanged = it->combatStatMain != main.UID;
			for(i32 k = 0; k < 2; k++) {
				if(mainChanged && it->combatStatMain != ActorUID::INVALID) {
					for(u32 m = 0; m < it->characters.size(); m++) {
						const ActorMaster& old = *it->characters[m];
						if(old.UID != it->combatStatMain || old.classType == ClassType::NONE) continue;
						replication->SendStatToAll(old.UID, COMBAT_STAT_TYPES[k], 0.0f, CombatStatBase(*it, old, COMBAT_STAT_TYPES[k]));
					}
				}
				const f32 value = (CombatStatBase(*it, main, COMBAT_STAT_TYPES[k]) + StatusModSum(*it, COMBAT_STAT_MODS[k])) * MAX(1.0f + StatusModMulSum(*it, COMBAT_STAT_MODS[k]) / 100.0f, 0.0f);
				if(!mainChanged && fabsf(value - it->combatStatSent[k]) <= 0.01f) continue;
				it->combatStatSent[k] = value;
				replication->SendStatToAll(main.UID, COMBAT_STAT_TYPES[k], 0.0f, value);
			}
			it->combatStatMain = main.UID;
		}

		if(localTime >= it->tNextRegen) {
			it->tNextRegen = TimeAdd(localTime, TimeMsToTime(REGEN_INTERVAL_MS));
			auto regen = [&](ActorUID uid, i32 stat, f32& value, f32 max, f32 amount) {
				if(value < 0 || max <= 0 || amount == 0) return;
				const f32 next = MIN(MAX(value + amount, 0.0f), max);
				if(next == value) return;
				value = next;
				replication->SendStatToAll(uid, stat, value, max, STAT_REASON_REGEN);
			};
			if(!it->dead) {
				const ActorUID mainUID = it->Main().UID;
				regen(mainUID, (i32)StatType::STAT_TYPE_ULTIMATE, it->ultGauge, ultMax, MasterStat(cls, STAT_ULTIMATEREGEN));
				regen(mainUID, STAT_STAMINA, it->stamina, staminaMax, staminaRegen);
				regen(mainUID, STAT_MANA, it->mana, manaMax, manaRegen);
				ActorMaster& mainChara = it->Main();
				const i32 hpRegen = (i32)StatusRegenBonus(*it, StatusMod::HEALTHREGEN, false);
				if(hpRegen > 0 && mainChara.hp > 0 && mainChara.hp < mainChara.hpMax) {
					mainChara.hp = MIN(mainChara.hp + hpRegen, mainChara.hpMax);
					replication->SendStatToAll(mainUID, (i32)StatType::STAT_TYPE_HEALTH, (f32)mainChara.hp, (f32)mainChara.hpMax, STAT_REASON_REGEN);
				}
				for(int r = 0; r < 3; r++) {
					f32 amount = r == 1 ? rageRegen : r == 0 ? energyRegen : MasterStat(cls, RESOURCES[r].regenStat);
					if(r == 1 && amount < 0 && localTime < TimeAdd(it->rageGainAt, TimeMsToTime(RAGE_DECAY_DELAY_MS))) amount = 0;
					regen(mainUID, RESOURCES[r].maxStat, it->res[r], resMax[r], amount);
				}

				ActorMaster& bench = it->Sub();
				if(bench.hp > 0 && bench.hp < bench.hpMax) {
					const i32 hpGain = (i32)(MasterStat(benchCls, STAT_TAG_HEALTHREGEN_VALUE) + StatusRegenBonus(*it, StatusMod::TAG_HEALTHREGEN, false) + MasterStat(benchCls, STAT_TAG_HEALTHREGEN_RATIO) * bench.hpMax / 100.0f);
					if(hpGain > 0) {
						bench.hp = MIN(bench.hp + hpGain, bench.hpMax);
						replication->SendStatToAll(bench.UID, (i32)StatType::STAT_TYPE_HEALTH, (f32)bench.hp, (f32)bench.hpMax, STAT_REASON_REGEN);
					}
				}
				regen(bench.UID, STAT_STAMINA, it->benchStamina, benchStaminaMax, MasterStat(benchCls, STAT_TAG_STAMINAREGEN_VALUE) + MasterStat(benchCls, STAT_TAG_STAMINAREGEN_RATIO) * benchStaminaMax / 100.0f);
				regen(bench.UID, STAT_MANA, it->benchMana, benchManaMax, MasterStat(benchCls, STAT_TAG_MANAREGEN_VALUE) + MasterStat(benchCls, STAT_TAG_MANAREGEN_RATIO) * benchManaMax / 100.0f);
				for(int r = 0; r < 3; r++) {
					regen(bench.UID, RESOURCES[r].maxStat, it->benchRes[r], benchResMax[r], MasterStat(benchCls, RESOURCES[r].tagRegenValueStat) + MasterStat(benchCls, RESOURCES[r].tagRegenRatioStat) * benchResMax[r] / 100.0f);
				}
			}
		}

		if(ShowcaseKeepsFullRes(*it)) {
			for(int r = 0; r < 3; r++) {
				if(resMax[r] > 0 && it->res[r] != resMax[r]) { it->res[r] = resMax[r]; it->resDirty = true; }
			}
		}

		const ActorUID mainUID = it->Main().UID;
		if(it->ultDirty) replication->SendStatToAll(mainUID, (i32)StatType::STAT_TYPE_ULTIMATE, it->ultGauge, ultMax);
		if(replication->titanMode) {
			SkillID ult = SkillID::INVALID;
			foreach_const(sid, GetGameXmlContent().GetMaster(it->Main().classType).skillIDs) if((i32)*sid % 100 == ULTIMATE_SKILL_ID_SUFFIX) ult = *sid;
			auto learned = it->skillLevels.find((i32)ult);
			const i8 ready = (learned != it->skillLevels.end() && (learned->second > 0 || !ingameSkillPoints) && localTime >= it->ultCooldownUntil && localTime >= it->castLockUntil) ? 1 : 0;
			if(ready != it->ultReadySent) {
				it->ultReadySent = ready;
				foreach_const(mate, players) {
					if(mate->clientHd != ClientHandle::INVALID && mate->team == it->team) replication->SendUltReady(mate->clientHd, mainUID, ready != 0);
				}
			}
		}
		if(it->staminaDirty && staminaMax > 0) replication->SendStatToAll(mainUID, STAT_STAMINA, it->stamina, staminaMax);
		if(it->manaDirty && manaMax > 0) replication->SendStatToAll(mainUID, STAT_MANA, it->mana, manaMax);
		if(it->resDirty) {
			for(int r = 0; r < 3; r++) {
				if(resMax[r] > 0) replication->SendStatToAll(mainUID, RESOURCES[r].maxStat, it->res[r], resMax[r]);
			}
		}
		it->ultDirty = false;
		it->staminaDirty = false;
		it->manaDirty = false;
		it->resDirty = false;
	}
}

static const u32 ERROR_TYPE_REQ_MANA = 5;
static const u32 ERROR_TYPE_REQ_ULTIMATE = 8;
static const u32 ERROR_TYPE_REQ_STAMINA = 10;
static const u32 ERROR_TYPE_SKILL_NOT_ENOUGH_RESOURCE = 562;
static const u32 RESOURCE_ERRORS[3] = { 7, 6, ERROR_TYPE_SKILL_NOT_ENOUGH_RESOURCE };

u32 World::SkillCostError(const Player& player, i32 skillID, f32 mp) const
{
	const f32 ug = PlayerSkillConsume(player, skillID, 4);
	const f32 st = GetGameXmlContent().GetSkillConsumeStamina(skillID);
	const f32 hpCost = PlayerSkillConsume(player, skillID, 3);
	if(mp > 0 && player.mana >= 0 && player.mana < mp) return ERROR_TYPE_REQ_MANA;
	if(ug > 0 && player.ultGauge >= 0 && player.ultGauge < ug) return ERROR_TYPE_REQ_ULTIMATE;
	if(st > 0 && player.stamina >= 0 && player.stamina < st) return ERROR_TYPE_REQ_STAMINA;
	if(hpCost > 0 && player.Main().hp <= hpCost) return ERROR_TYPE_SKILL_NOT_ENOUGH_RESOURCE;
	for(int r = 0; r < 3; r++) {
		const f32 cost = PlayerSkillConsume(player, skillID, r);
		if(cost > 0 && player.res[r] >= 0 && player.res[r] < cost) return RESOURCE_ERRORS[r];
	}
	return 0;
}

u32 World::PlayerPaySkillCost(Player& player, i32 skillID, bool pay)
{
	const auto& content = GetGameXmlContent();
	f32 mp = PlayerSkillConsumeMP(player, skillID);
	foreach(it, activeStatuses) {
		if(it->targetIndex != player.index || !it->info || it->info->type != StatusType::NOCOST || it->until <= localTime) continue;
		const bool listed = eastl::find(it->info->costSkills.begin(), it->info->costSkills.end(), skillID) != it->info->costSkills.end();
		bool ownSkill = false;
		if(!listed && player.Main().classType != ClassType::NONE && content.GetSkillConsumeStamina(skillID) <= 0) {
			foreach_const(sid, content.GetMaster(player.Main().classType).skillIDs) if((i32)*sid == skillID) ownSkill = true;
		}
		if(!listed && !ownSkill) continue;
		if(listed) mp = MAX(mp * (1.0f + it->info->costRatioMP / 100.0f), 0.0f);
		if(pay) {
			it->until = localTime;
			LOG("Passive :: %ls skill %d mana cost %.0f (status %d used)", player.name.data(), skillID, mp, it->statusID);
		}
		break;
	}
	if(const ActiveStatus* stacking = FindPassiveOfType(player, StatusType::DOUBLEGUN)) {
		const auto& list = stacking->info->costSkills;
		if(player.skillStacks > 0 && localTime < player.skillStacksUntil && eastl::find(list.begin(), list.end(), skillID) != list.end()) mp = MAX(mp * (1.0f + stacking->info->costRatioMP * player.skillStacks / 100.0f), 0.0f);
	}
	const f32 ug = PlayerSkillConsume(player, skillID, 4);
	const f32 st = content.GetSkillConsumeStamina(skillID);
	const f32 resCost[3] = { PlayerSkillConsume(player, skillID, 0), PlayerSkillConsume(player, skillID, 1), PlayerSkillConsume(player, skillID, 2) };
	const f32 hpCost = PlayerSkillConsume(player, skillID, 3);
	if(mp <= 0 && ug <= 0 && st <= 0 && resCost[0] <= 0 && resCost[1] <= 0 && resCost[2] <= 0 && hpCost <= 0) return 0;
	if(player.clientHd == ClientHandle::INVALID && (Config().DevShowcase || Config().DevMirror)) return 0;

	const u32 error = SkillCostError(player, skillID, mp);
	const bool ok = error == 0;
	if(resCost[0] > 0 || resCost[1] > 0 || resCost[2] > 0 || hpCost > 0) {
		LOG("Cost :: %ls skill=%d energy=%.0f/%.0f rage=%.0f/%.0f charges=%.0f/%.1f hp=%.0f", player.name.data(), skillID,
			resCost[0], player.res[0], resCost[1], player.res[1], resCost[2], player.res[2], hpCost);
	}
	LOG("Cost :: %ls skill=%d mp=%.0f/%.0f ug=%.0f/%.0f stamina=%.0f/%.0f %s", player.name.data(), skillID,
		mp, player.mana, ug, player.ultGauge, st, player.stamina, ok ? (pay ? "paid" : "payable") : "REFUSED");
	if(!ok) return error;
	if(!pay) return 0;

	if(mp > 0 && player.mana >= 0) { player.mana -= mp; player.manaDirty = true; }
	if(ug > 0 && player.ultGauge >= 0) { player.ultGauge -= ug; player.ultDirty = true; }
	if(st > 0 && player.stamina >= 0) { player.stamina -= st; player.staminaDirty = true; }
	for(int r = 0; r < 3; r++) {
		if(resCost[r] > 0 && player.res[r] >= 0) { player.res[r] -= resCost[r]; player.resDirty = true; }
	}
	if(hpCost > 0) {
		ActorMaster& chara = player.Main();
		chara.hp = MAX(chara.hp - (i32)hpCost, 1);
		replication->SendHpToAll(chara.UID, chara.hp, chara.hpMax);
	}
	return 0;
}

void World::RunStanceOut(Player& player, i8 oldStance)
{
	const auto& content = GetGameXmlContent();
	const GameXmlContent::Action* action = content.FindSkillAction(ActionClass(player), ActionStateID::STANCE_OUT_BEHAVIORSTATE, oldStance);
	if(!action) return;
	foreach_const(cmd, action->commands) {
		if(cmd->type == ActionCommand::Type::STATUS && cmd->status.statusIndex != 0) {
			if(cmd->status.remove) {
				foreach(it, activeStatuses) {
					if(it->targetIndex == player.index && it->statusID == cmd->status.statusIndex && localTime < it->until) it->until = localTime;
				}
			}
			else if(!player.dead) ApplyStatus(player, player, cmd->status.statusIndex);
		}
		else if(cmd->type == ActionCommand::Type::REMOTE && cmd->remote.remove) {
			foreach(ri, remoteInsts) {
				if(ri->ownerIndex == player.index && ri->remote && ri->remote->ID == cmd->remote.idx && !ri->killNow) ri->killNow = true;
			}
		}
	}
	LOG("Stance :: %ls stance %d out", player.name.data(), (i32)oldStance);
}

void World::SetStance(ActorMaster& master, i8 stance, f32 duration, i32 weaponFireCount, bool cancelByDefeat)
{
	const i8 oldStance = master.stance;
	master.stance = stance;
	master.stanceUntil = stance != 0 && duration > 0 ? TimeAdd(localTime, TimeMsToTime((i64)(duration * 1000.0f))) : Time::ZERO;
	master.stanceShotsLeft = stance != 0 && weaponFireCount > 0 ? weaponFireCount : -1;
	master.stanceCancelByDefeat = stance != 0 && cancelByDefeat;
	if(stance == 0 && oldStance != 0) replication->SendStanceToAll(master.UID, 0, STANCE_EXIT_DURATION);
	if(stance != oldStance && master.parent && &master.parent->Main() == &master && FindPassiveOfType(*master.parent, StatusType::DOUBLEGUN)) {
		const u32 m = master.parent->mainCharaID & 1;
		master.parent->heat[m] = 0;
		if(master.parent->overheatUntil[m] != Time::ZERO) master.parent->overheatUntil[m] = localTime;
	}
	LOG("Stance :: %ls stance=%d duration=%.1f weaponFireCount=%d", master.parent ? master.parent->name.data() : L"?", (i32)stance, duration, weaponFireCount);
	if(oldStance != 0 && stance == 0 && master.parent) RunStanceOut(*master.parent, oldStance);
}

void World::PlayerCancelStance(Player& player)
{
	if(player.dead || player.Main().stance == 0) return;
	LOG("Stance :: %ls cancels stance %d", player.name.data(), (i32)player.Main().stance);
	SetStance(player.Main(), 0, 0, 0, false);
}

void World::StanceWeaponUse(Player& player)
{
	ActorMaster& m = player.Main();
	if(m.stance == 0 || m.stanceShotsLeft <= 0) return;
	if(--m.stanceShotsLeft == 0) SetStance(m, 0, 0, 0, false);
}

void World::ToggleOn(Player& player, SkillID skillID, const vec3& castPos)
{
	const GameXmlContent::SkillToggle* tg = GetGameXmlContent().FindSkillToggle((i32)skillID);
	if(!tg) return;
	if(player.dead || frozen) return;
	const bool shirk = GetGameXmlContent().GetSkillConsumeStamina((i32)skillID) > 0;
	if(IsBlocked(player, false, !shirk, shirk)) {
		LOG("Blocked :: %ls toggle skill=%d (input blocked by a status)", player.name.data(), (i32)skillID);
		return;
	}
	if(PlayerPaySkillCost(player, (i32)skillID, false) != 0) return;
	if(player.toggleSkill != SkillID::INVALID) ToggleOff(player);
	player.toggleSkill = skillID;
	player.toggleCastID = nextCastID++;
	player.toggleNext = TimeAdd(localTime, TimeMsToTime(tg->interval * 1000.0));
	const vec3 pos = player.body->GetWorldPos();
	const f32 yaw = player.input.rot.upperYaw;
	const GameXmlContent::CostLevel* mods = UpgradeCastMods(player, skillID, nullptr);
	foreach_const(id, tg->remotes) {
		i32 rid = *id;
		if(mods) foreach_const(rr, mods->remoteReplace) if(rr->first == rid) rid = rr->second;
		SpawnRemote(player.index, RemoteIdx(rid), pos, vec2(cosf(yaw), sinf(yaw)), ActorUID::INVALID, skillID, player.toggleCastID, (i32)SkillDamage(skillID), true, nullptr, 0, player.clientHd != ClientHandle::INVALID);
	}
	player.toggleStatuses.clear();
	foreach_const(id, tg->statuses) {
		i32 sid = *id;
		f32 duration = -1.0f;
		if(mods) foreach_const(sr, mods->statusReplace) if(sr->statusID == sid) { sid = sr->replaceID; if(sr->durationSec > 0) duration = sr->durationSec; }
		const StatusInfo* info = GetGameXmlContent().FindStatus(sid);
		if(duration <= 0 && info && info->durationSec < 0) duration = STATUS_INFINITE_DURATION_SEC;
		ApplyStatus(player, player, sid, 0, duration);
		if(!player.toggleStatuses.full()) player.toggleStatuses.push_back(sid);
	}
	LOG("Toggle :: %ls skill=%d on (%d remotes, %d statuses)", player.name.data(), (i32)skillID, (i32)tg->remotes.size(), (i32)tg->statuses.size());
	auto sk = GetGameXmlContent().skillMap.find(skillID);
	replication->SendToggleSkill(player.Main().UID, player.clientHd, skillID, true, sk != GetGameXmlContent().skillMap.end() ? sk->second.action : ActionStateID::INVALID);
}

void World::ToggleOff(Player& player)
{
	if(player.toggleSkill == SkillID::INVALID) return;
	const GameXmlContent::SkillToggle* tg = GetGameXmlContent().FindSkillToggle((i32)player.toggleSkill);
	for(int i = 0; i < (i32)remoteInsts.size(); ) {
		RemoteInst& inst = remoteInsts[i];
		if(inst.ownerIndex == player.index && inst.castID == player.toggleCastID) {
			if(inst.clientRemoteID != 0) replication->SendDestroyRemoteToAll(inst.clientRemoteID);
			remoteInsts.erase_unsorted(remoteInsts.begin() + i);
		}
		else i++;
	}
	if(tg) {
		const GameXmlContent::CostLevel* mods = UpgradeCastMods(player, player.toggleSkill, nullptr);
		foreach(it, activeStatuses) {
			if(it->targetIndex != player.index) continue;
			bool own = eastl::find(tg->statuses.begin(), tg->statuses.end(), it->statusID) != tg->statuses.end() ||
				eastl::find(player.toggleStatuses.begin(), player.toggleStatuses.end(), it->statusID) != player.toggleStatuses.end();
			if(mods) foreach_const(sr, mods->statusReplace) if(sr->replaceID == it->statusID && eastl::find(tg->statuses.begin(), tg->statuses.end(), sr->statusID) != tg->statuses.end()) own = true;
			if(own) it->until = localTime;
		}
	}
	LOG("Toggle :: %ls skill=%d off", player.name.data(), (i32)player.toggleSkill);
	replication->SendToggleSkill(player.Main().UID, player.clientHd, player.toggleSkill, false, ActionStateID::INVALID);
	player.toggleSkill = SkillID::INVALID;
	player.toggleStatuses.clear();
}

void World::UpdateToggles()
{
	foreach(it, players) {
		Player& p = *it;
		if(p.toggleSkill == SkillID::INVALID) continue;
		if(p.dead) { ToggleOff(p); continue; }
		const GameXmlContent::SkillToggle* tg = GetGameXmlContent().FindSkillToggle((i32)p.toggleSkill);
		if(!tg || tg->interval <= 0 || localTime < p.toggleNext) continue;
		p.toggleNext = TimeAdd(p.toggleNext, TimeMsToTime(tg->interval * 1000.0));
		f32* pool = nullptr;
		if(tg->consume == GameXmlContent::SkillToggle::MANA) pool = &p.mana;
		else if(tg->consume == GameXmlContent::SkillToggle::RAGE) pool = &p.res[1];
		else if(tg->consume == GameXmlContent::SkillToggle::STAMINA) pool = &p.stamina;
		if(!pool || *pool < 0) continue;
		const GameXmlContent::SkillUpgrade* tu = LatestUpgradeWith(p, (i32)p.toggleSkill, [](const GameXmlContent::SkillUpgrade& u) { return u.toggleValue >= 0; });
		const f32 cost = tu ? tu->toggleValue : tg->value;
		if(*pool < cost) { ToggleOff(p); continue; }
		*pool -= cost;
		p.manaDirty = true;
		p.resDirty = true;
		p.staminaDirty = true;
	}
}

void World::ApplyNpcOwnerStatus(ActorUID npcUID, Player& carrier, i32 statusID)
{
	ApplyNpcCarrierStatus(npcUID, carrier.Main().UID, carrier.body->GetWorldPos(), statusID);
}

void World::ApplyNpcCarrierStatus(ActorUID npcUID, ActorUID carrierUID, const vec3& pos, i32 statusID)
{
	const StatusInfo* info = GetGameXmlContent().FindStatus(statusID);
	const f32 duration = info && info->durationSec > 0 ? info->durationSec : 0.0f;
	replication->SendAddStatusToAll(statusID, npcUID, carrierUID, duration, pos, pos, 0, 1, 1.0f, -1.0f, false, false);
	LOG("Status :: summon %u carried by %u status=%d duration=%.2f", (u32)npcUID, (u32)carrierUID, statusID, duration);
	if(ActorNpc* carried = info && info->npcMotion != ActionStateID::INVALID ? FindNpcActor(npcUID) : nullptr) {
		if(carried->pendingMotion != ActionStateID::INVALID) {
			const SummonInfo* motionInfo = GetGameXmlContent().FindSummon((i32)carried->docID);
			replication->SendNpcBehaviorToAll(carried->UID, carried->pendingMotion, carried->pos, carried->motionTarget, carried->motionTargetUID, motionInfo ? motionInfo->moveSpeed : 0.0f);
			carried->pendingMotion = ActionStateID::INVALID;
		}
		carried->actionState = info->npcMotion;
		if(info->npcMotion == ActionStateID::LIFTED_BEHAVIORSTATE) RunSummonState(*carried, SUMMON_STATE_LIFTED);
		replication->SendAiActionStateToAll(npcUID, info->npcMotion, carried->rot.z, pos, GetGameXmlContent().FindSummon((i32)carried->docID) && GetGameXmlContent().FindSummon((i32)carried->docID)->moveSpeed > 0);
	}
	if(npcStatuses.full()) return;
	NpcStatus ns;
	ns.npcUID = npcUID;
	ns.statusID = statusID;
	ns.casterUID = carrierUID;
	ns.until = TimeAdd(localTime, TimeMsToTime(duration * 1000.0));
	npcStatuses.push_back(ns);
}

void World::ApplySummonDrainLife(ActorNpc& npc, const StatusInfo& info)
{
	if(npc.ownerUID == ActorUID::INVALID || info.typeName != "STATUS_TYPE_PUREDOT" || info.durationSec > 0 || info.intervalSec <= 0 || info.paramRatio <= 0) return;
	static const f64 DRAINED_SUMMON_MAX_LIFE_SEC = 600.0;
	if(npc.hpMax <= 0) {
		const f32 life = ceilf(1.0f / info.paramRatio) * info.intervalSec;
		npc.despawnAt = TimeAdd(localTime, TimeMsToTime(life * 1000.0));
		LOG("Summon :: creature=%d drained by status %d, life %.1fs", (i32)npc.docID, info.ID, life);
		return;
	}
	npc.drainPerTick = MAX((i32)(npc.hpMax * info.paramRatio), 1);
	npc.drainInterval = info.intervalSec;
	npc.nextDrainAt = TimeAdd(localTime, TimeMsToTime(info.intervalSec * 1000.0));
	npc.despawnAt = TimeAdd(localTime, TimeMsToTime(DRAINED_SUMMON_MAX_LIFE_SEC * 1000.0));
	LOG("Summon :: creature=%d drained by status %d, %d hp every %.1fs", (i32)npc.docID, info.ID, npc.drainPerTick, info.intervalSec);
}

void World::TrackNpcSelfStatus(ActorUID npcUID, const StatusInfo& info)
{
	if(ActorNpc* drained = FindNpcActor(npcUID)) ApplySummonDrainLife(*drained, info);
	if(info.durationSec <= 0) return;
	const Time until = TimeAdd(localTime, TimeMsToTime(info.durationSec * 1000.0));
	foreach(ns, npcStatuses) {
		if(ns->npcUID != npcUID || ns->statusID != info.ID || ns->linked) continue;
		ns->until = until;
		ns->appliedAt = localTime;
		return;
	}
	if(npcStatuses.full()) return;
	NpcStatus ns;
	ns.npcUID = npcUID;
	ns.statusID = info.ID;
	ns.casterUID = npcUID;
	ns.until = until;
	ns.appliedAt = localTime;
	ns.linked = false;
	npcStatuses.push_back(ns);
}

bool World::NpcResistsDebuff(const ActorNpc& npc, const StatusInfo* info, bool hostile) const
{
	static const i32 MEASURED_SLOW_IMMUNE_DEFENCE_STATUSES[] = { 400000140, 400000160 };
	if(!info || !(info->type == StatusType::SLOW || (info->type == StatusType::STAT && (info->isDebuff || hostile)))) return false;
	const i32 defence = GetGameXmlContent().CreatureDefenceStatus((i32)npc.docID);
	for(i32 i = 0; i < (i32)ARRAY_COUNT(MEASURED_SLOW_IMMUNE_DEFENCE_STATUSES); i++) if(defence == MEASURED_SLOW_IMMUNE_DEFENCE_STATUSES[i]) return true;
	return false;
}

void World::ApplyStatusToNpc(Player& caster, ActorNpc& npc, i32 statusID, bool remove)
{
	i32 prevOverlap = 0;
	Time prevTick = Time::ZERO;
	const StatusInfo* known = GetGameXmlContent().FindStatus(statusID);
	if(!remove && known && ((known->reaction && npc.monsterAi && npc.monsterAi->reactionImmune) || NpcResistsDebuff(npc, known, npc.faction != (caster.team == 0 ? Faction::RED : Faction::BLUE)))) {
		replication->SendResistStatusToAll(npc.UID, caster.Main().UID, STATUS_RESIST_EVENT_PASSIVE);
		return;
	}
	const ActorUID casterField = statusCasterOverride != ActorUID::INVALID ? statusCasterOverride : caster.Main().UID;
	for(i32 i = 0; i < (i32)npcStatuses.size(); ) {
		if(npcStatuses[i].npcUID != npc.UID || npcStatuses[i].statusID != statusID || npcStatuses[i].linked) { i++; continue; }
		if(!remove && npcStatuses[i].appliedAt == localTime && npcStatuses[i].casterUID == casterField && (!known || known->maxOverlap <= 1)) return;
		if(!remove && localTime >= npcStatuses[i].until) {
			replication->SendRemoveStatusToAll(statusID, npc.UID, npcStatuses[i].casterUID);
			npcStatuses.erase(npcStatuses.begin() + i);
			continue;
		}
		if(localTime < npcStatuses[i].until) { prevOverlap = npcStatuses[i].dotOverlap; prevTick = npcStatuses[i].dotNextTick; }
		if(remove) npcStatuses[i].until = localTime;
		i++;
	}
	if(remove) return;
	const StatusInfo* info = GetGameXmlContent().FindStatus(statusID);
	if(info && (info->type == StatusType::SUICIDE || info->type == StatusType::COMA) && npc.ownerIndex == caster.index && npc.ownerUID != ActorUID::INVALID) {
		LOG("Status :: %ls ends its creature=%d with status %d", caster.name.data(), (i32)npc.docID, statusID);
		replication->SendAddStatusToAll(statusID, npc.UID, caster.Main().UID, 0.0f, npc.pos, npc.pos, 0, 1, 1.0f, -1.0f, false);
		replication->SendRemoveStatusToAll(statusID, npc.UID, caster.Main().UID);
		if(npc.despawnAt == Time::ZERO || localTime < npc.despawnAt) npc.despawnAt = localTime;
		return;
	}
	if(info && info->type == StatusType::TOGGLE && info->durationSec <= 0 && info->skillIndex != 0) {
		foreach(ns, npcStatuses) {
			if(ns->npcUID != npc.UID || ns->statusID == statusID || localTime >= ns->until || ns->appliedAt >= localTime) continue;
			const StatusInfo* other = GetGameXmlContent().FindStatus(ns->statusID);
			if(!other || other->permanent || other->skillIndex != info->skillIndex) continue;
			if(info->toggleRemoves != 0 && ns->statusID != info->toggleRemoves) continue;
			ns->until = localTime;
			LOG("Status :: creature=%d toggle %d removes %d", (i32)npc.docID, statusID, ns->statusID);
		}
	}
	if(info) ApplySummonDrainLife(npc, *info);
	const f32 reaction = NpcDefeatReaction(npc, info);
	if(reaction < 0) {
		replication->SendResistStatusToAll(npc.UID, caster.Main().UID, STATUS_RESIST_EVENT_PASSIVE);
		return;
	}
	if(npc.removeAt != Time::ZERO) return;
	const ActorUID summonSource = statusCasterOverride != ActorUID::INVALID && FindNpcActor(statusCasterOverride) ? statusCasterOverride : ActorUID::INVALID;
	if(info && info->casterStatus != 0 && info->casterStatus != statusID && summonSource != ActorUID::INVALID && prevOverlap == 0) ApplyNpcCarrierStatus(summonSource, npc.UID, npc.pos, info->casterStatus);
	else if(info && info->casterStatus != 0 && info->casterStatus != statusID && !caster.dead && !applyingCasterStatus && prevOverlap == 0) {
		applyingCasterStatus = true;
		const ActorUID prevCaster = statusCasterOverride;
		statusCasterOverride = npc.UID;
		ApplyStatus(caster, caster, info->casterStatus);
		statusCasterOverride = prevCaster;
		applyingCasterStatus = false;
	}
	const vec3 from = npc.pos;
	const vec3 center = statusForceCenterSet ? statusForceCenter : caster.body->GetWorldPos();
	const NpcCcGraph cc = StartNpcCc(npc, info, center, vec2(center), caster.body->GetWorldPos(), 0.0f);
	const bool summonBound = info && info->durationSec < 0 && cc.graphSec <= 0 && npc.ownerUID != ActorUID::INVALID && info->dotDamage <= 0 && info->intervalSec <= 0;
	const f32 duration = summonBound ? STATUS_INFINITE_DURATION_SEC : cc.duration;
	const i32 overlap = prevOverlap > 0 ? MIN(prevOverlap + 1, MAX(info ? info->maxOverlap : 1, 1)) : 1;
	replication->SendAddStatusToAll(statusID, npc.UID, casterField, duration, from, cc.to, cc.graphSec, overlap, 1.0f, cc.nominal, cc.graph, prevOverlap > 0);
	if(info && info->raiseStunByPush && cc.graphSec > 0) AddPushBound(caster, npc.UID, *info, cc.graphSec);
	if(npcStatuses.full()) return;
	NpcStatus ns;
	ns.npcUID = npc.UID;
	ns.statusID = statusID;
	ns.casterUID = casterField;
	ns.until = TimeAdd(localTime, TimeMsToTime(duration * 1000.0));
	ns.appliedAt = localTime;
	ns.dotOverlap = overlap;
	ns.linked = false;
	if(info && info->type == StatusType::SHIELD) ns.shieldHP = GetGameXmlContent().StatusShieldAmount(*info);
	ns.shieldMax = ns.shieldHP;
	for(i32 i = 0; i < (i32)npcStatuses.size(); ) {
		if(npcStatuses[i].npcUID == npc.UID && npcStatuses[i].statusID == statusID && !npcStatuses[i].linked) npcStatuses.erase(npcStatuses.begin() + i);
		else i++;
	}
	if(info && info->dotDamage > 0) {
		ns.dotCaster = caster.index;
		ns.dotPower = PlayerSkillPower(caster);
		ns.dotOverlap = MIN(prevOverlap + 1, MAX(info->maxOverlap, 1));
		ns.dotNextTick = prevTick != Time::ZERO ? prevTick : TimeAdd(localTime, TimeMsToTime((info->intervalSec > 0 ? info->intervalSec : 0.0f) * 1000.0));
		const i32 ticks = info->intervalSec > 0 ? (i32)floorf(duration / info->intervalSec) : 0;
		if(prevTick != Time::ZERO && ticks >= 1) ns.until = TimeAdd(prevTick, TimeMsToTime((f64)(ticks - 1) * info->intervalSec * 1000.0));
	}
	if(info && info->type == StatusType::HEAL && info->intervalSec > 0) {
		foreach_const(m, info->mods) {
			if(m->stat != StatusMod::HEALTH) continue;
			ns.dotCaster = caster.index;
			ns.healValue = m->value;
			ns.healPct = m->mul;
			ns.dotNextTick = localTime;
			break;
		}
	}
	npcStatuses.push_back(ns);
	if(ns.shieldMax > 0) SendNpcShieldStat(npc.UID);
	if(info && info->maxOverlap > 1 && overlap >= info->maxOverlap) {
		i32 full = 0;
		foreach_const(ps, activeStatuses) {
			if(ps->targetIndex != caster.index || !ps->info || ps->info->stackStatus != statusID || ps->info->stackFullStatus == 0) continue;
			full = ps->info->stackFullStatus;
			break;
		}
		if(full != 0) {
			LOG("Passive :: %ls full stack of %d on creature=%d -> %d", caster.name.data(), statusID, (i32)npc.docID, full);
			for(i32 i = 0; i < (i32)npcStatuses.size(); ) {
				if(npcStatuses[i].npcUID == npc.UID && npcStatuses[i].statusID == statusID) {
					replication->SendRemoveStatusToAll(statusID, npc.UID, npcStatuses[i].casterUID);
					npcStatuses.erase(npcStatuses.begin() + i);
				}
				else i++;
			}
			ApplyStatusToNpc(caster, npc, full, false);
		}
	}
	LOG("Status :: %ls gives creature=%d status=%d for %.1fs shield=%.0f", caster.name.data(), (i32)npc.docID, statusID, duration, ns.shieldHP);
}

bool World::TeleportNextToTarget(Player& player, ActorUID target)
{
	vec3 targetPos;
	i32 targetDoc = 0;
	if(const ActorNpc* npc = FindNpcActor(target)) { targetPos = npc->pos; targetDoc = (i32)npc->docID; }
	else if(const ActorMaster* master = FindMasterActor(target)) {
		if(master->parent->dead) return false;
		targetPos = master->parent->body->GetWorldPos();
	}
	else {
		auto dyn = actorDynamicMap.find(target);
		if(dyn == actorDynamicMap.end()) return false;
		targetPos = dyn->second->pos;
		targetDoc = (i32)dyn->second->docID;
	}
	static const f32 TELEPORT_PROBE_DIST = 800.0f;
	static const f32 TELEPORT_PROBE_HEIGHT = 100.0f;
	vec2 dir = vec2(player.body->GetWorldPos()) - vec2(targetPos);
	const f32 len = glm::length(dir);
	dir = len > 1.0f ? dir / len : vec2(1, 0);
	const vec3 probe = vec3(vec2(targetPos) + dir * TELEPORT_PROBE_DIST, targetPos.z + TELEPORT_PROBE_HEIGHT);
	f32 hit = 0;
	vec3 dest = targetPos;
	f32 measured = GetGameXmlContent().FindStageTeleportDistance(targetDoc);
	const GameXmlContent::MonsterAi* targetAi = measured < 0 ? GetGameXmlContent().FindMonsterAi(targetDoc) : nullptr;
	if(targetAi && targetAi->radius > 0) foreach_const(m, GetGameXmlContent().stageTeleportDistance) {
		const GameXmlContent::MonsterAi* same = GetGameXmlContent().FindMonsterAi(m->first);
		if(same && m->second > 0 && same->radius == targetAi->radius && same->sizeRank == targetAi->sizeRank && same->tagKey != 0 && targetAi->tagKey != 0) { measured = m->second; break; }
	}
	if(measured >= 0) dest = vec3(vec2(targetPos) + dir * measured, targetPos.z);
	else if(physics.RaycastStatic(probe, vec3(vec2(targetPos), targetPos.z + TELEPORT_PROBE_HEIGHT), &hit)) dest = vec3(vec2(targetPos) + dir * (TELEPORT_PROBE_DIST - hit + HitRadius(player)), targetPos.z);
	LOG("Teleport :: %ls next to target %u at (%.0f, %.0f), %.0f from it", player.name.data(), (u32)target, dest.x, dest.y, glm::length(vec2(dest) - vec2(targetPos)));
	WarpPlayer(player, dest, atan2f(-dir.y, -dir.x));
	return true;
}

void World::UpdateNpcDots()
{
	for(i32 i = 0; i < (i32)npcStatuses.size(); i++) {
		if(npcStatuses[i].dotCaster == UINT32_MAX || npcStatuses[i].dotNextTick == Time::ZERO || localTime < npcStatuses[i].dotNextTick) continue;
		const StatusInfo* info = GetGameXmlContent().FindStatus(npcStatuses[i].statusID);
		ActorNpc* npc = FindNpcActor(npcStatuses[i].npcUID);
		if(!info || !npc || npcStatuses[i].dotCaster >= players.size()) { npcStatuses[i].dotNextTick = Time::ZERO; continue; }
		Player& caster = players[npcStatuses[i].dotCaster];
		const f32 interval = info->intervalSec;
		while(npcStatuses[i].dotNextTick != Time::ZERO && localTime >= npcStatuses[i].dotNextTick && npcStatuses[i].dotNextTick <= npcStatuses[i].until && npc->hp > 0 && npc->removeAt == Time::ZERO) {
			npcStatuses[i].dotNextTick = interval > 0 ? TimeAdd(npcStatuses[i].dotNextTick, TimeMsToTime(interval * 1000.0)) : Time::ZERO;
			if(npcStatuses[i].healValue > 0) {
				if(npcStatuses[i].dotNextTick > npcStatuses[i].until) break;
				const i32 amount = (i32)(npcStatuses[i].healPct ? npc->hpMax * npcStatuses[i].healValue / 100.0f : npcStatuses[i].healValue);
				if(npc->hp >= npc->hpMax) continue;
				npc->hp = MIN(npc->hp + amount, npc->hpMax);
				replication->SendHpToAll(npc->UID, npc->hp, npc->hpMax);
				LOG("Heal :: %ls -> creature=%d amount=%d hp=%d/%d", caster.name.data(), (i32)npc->docID, amount, npc->hp, npc->hpMax);
				continue;
			}
			const f32 perStack = info->dotDamage + npcStatuses[i].dotPower * info->dotMul;
			const i32 tick = MAX((i32)perStack, 1) * npcStatuses[i].dotOverlap;
			damageStatusID = npcStatuses[i].statusID;
			DamageNpc(caster, *npc, SkillID::INVALID, tick, nullptr);
			damageStatusID = 0;
		}
		if(npcStatuses[i].dotNextTick != Time::ZERO && npcStatuses[i].dotNextTick > npcStatuses[i].until) npcStatuses[i].dotNextTick = Time::ZERO;
	}
}

void World::UpdateNpcSpeedMods()
{
	eastl::fixed_vector<eastl::pair<ActorUID,f32>,64,false> sums;
	foreach_const(ns, npcStatuses) {
		if(localTime >= ns->until) continue;
		const StatusInfo* info = GetGameXmlContent().FindStatus(ns->statusID);
		if(!info) continue;
		foreach_const(m, info->mods) {
			if(m->stat != StatusMod::MOVESPEED || !m->mul || m->regen) continue;
			bool found = false;
			foreach(s, sums) if(s->first == ns->npcUID) { s->second += m->value; found = true; break; }
			if(!found && !sums.full()) sums.push_back({ ns->npcUID, m->value });
		}
	}
	foreach(npc, actorNpcList) {
		if(!npc->monsterAi) continue;
		f32 sum = 0;
		foreach_const(s, sums) if(s->first == npc->UID) { sum = s->second; break; }
		const f32 mul = MAX(1.0f + sum / 100.0f, MEASURED_NPC_SPEED_MUL_MIN);
		if(mul == npc->statusSpeedMul) continue;
		npc->speedMul *= mul / npc->statusSpeedMul;
		npc->statusSpeedMul = mul;
		npc->monsterNextSync = localTime;
		if(npc->hp > 0 && npc->removeAt == Time::ZERO) replication->SendStatToAll(npc->UID, (i32)StatType::STAT_TYPE_MOVESPEED, 0.0f, npc->speedMul * 100.0f);
	}
}

void World::UpdateNpcStatuses()
{
	UpdateNpcDots();
	static const u32 coreTag = StatusTypeKey("Core");
	eastl::fixed_vector<eastl::pair<ActorUID,i32>,8,false> nextStatuses;
	for(i32 i = 0; i < (i32)npcStatuses.size(); ) {
		const NpcStatus& ns = npcStatuses[i];
		bool linked = false;
		foreach_const(st, activeStatuses) {
			if(st->casterUID == ns.npcUID && st->targetUID == ns.casterUID && st->info && (st->info->type == StatusType::TARGET || st->info->casterStatus == ns.statusID) && st->until > localTime) { linked = true; break; }
		}
		if(!linked && ns.linked) {
			foreach_const(other, npcStatuses) {
				if(other->npcUID != ns.casterUID || other->casterUID != ns.npcUID || localTime >= other->until) continue;
				const StatusInfo* pair = GetGameXmlContent().FindStatus(other->statusID);
				if(pair && pair->casterStatus == ns.statusID) { linked = true; break; }
			}
		}
		bool pairCasterGone = false;
		if(!ns.linked) {
			const StatusInfo* own = GetGameXmlContent().FindStatus(ns.statusID);
			if(own && own->type == StatusType::CASTER && own->casterStatus != 0 && !FindMasterActor(ns.casterUID)) pairCasterGone = NpcPairCasterGone(ns.casterUID);
		}
		const ActorNpc* holder = FindNpcActor(ns.npcUID);
		if((linked || !ns.linked) && !pairCasterGone && localTime < ns.until && holder && (holder->hp > 0 || holder->hpMax <= 0)) { i++; continue; }
		replication->SendRemoveStatusToAll(ns.statusID, ns.npcUID, ns.casterUID);
		const StatusInfo* motionStatus = GetGameXmlContent().FindStatus(ns.statusID);
		if(motionStatus && motionStatus->nextStatus != 0 && holder && holder->hp > 0 && holder->monsterAi && holder->monsterAi->tagKey == coreTag && !nextStatuses.full()) {
			const auto cond = motionStatus->nextCondition;
			const bool timeOver = localTime >= ns.until && !ns.removedOnHit;
			if(cond == StatusInfo::NextCondition::ALL || (cond == StatusInfo::NextCondition::TIMEOVER && timeOver) || (cond == StatusInfo::NextCondition::REMOVE_ONHIT && ns.removedOnHit)) nextStatuses.push_back({ ns.npcUID, motionStatus->nextStatus });
		}
		ActorNpc* released = motionStatus && motionStatus->npcMotion != ActionStateID::INVALID ? FindNpcActor(ns.npcUID) : nullptr;
		if(released && released->ownerUID != ActorUID::INVALID && released->actionState == motionStatus->npcMotion && (released->hp > 0 || released->hpMax <= 0) && released->removeAt == Time::ZERO) {
			const SummonInfo* releasedInfo = GetGameXmlContent().FindSummon((i32)released->docID);
			released->actionState = ActionStateID::NONE_BEHAVIORSTATE;
			replication->SendAiActionStateToAll(released->UID, ActionStateID::NONE_BEHAVIORSTATE, released->rot.z, released->pos, false, releasedInfo ? releasedInfo->moveSpeed : 0.0f);
		}
		const ActorUID shieldedUID = ns.shieldMax > 0 ? ns.npcUID : ActorUID::INVALID;
		npcStatuses.erase(npcStatuses.begin() + i);
		if(shieldedUID != ActorUID::INVALID && FindNpcActor(shieldedUID)) SendNpcShieldStat(shieldedUID);
	}
	foreach_const(n, nextStatuses) if(ActorNpc* npc = FindNpcActor(n->first)) ApplyMonsterStatus(*npc, n->second, true);
	foreach(st, activeStatuses) {
		if(!st->info || st->info->type != StatusType::CASTER || st->info->casterStatus == 0 || st->until <= localTime || FindMasterActor(st->casterUID) || !NpcPairCasterGone(st->casterUID)) continue;
		st->until = localTime;
		LOG("Status :: status=%d ended with its creature caster %u", st->statusID, (u32)st->casterUID);
	}
	UpdateNpcSpeedMods();
}

bool World::NpcPairCasterGone(ActorUID casterUID) const
{
	if(casterUID == ActorUID::INVALID) return false;
	const ActorNpc* npc = FindNpcActor(casterUID);
	return !npc || npc->removeAt != Time::ZERO || (npc->hp <= 0 && npc->hpMax > 0);
}

bool World::IsCarrying(const Player& player) const
{
	foreach_const(st, activeStatuses) {
		if(st->casterIndex == player.index && st->info && st->info->type == StatusType::TARGET && st->until > localTime) return true;
	}
	return false;
}

void World::ReleaseCast(Player& player)
{
	const Player::Casting c = player.casting;
	player.casting.skillID = SkillID::INVALID;
	auto f = GetGameXmlContent().skillMap.find(c.skillID);
	if(f == GetGameXmlContent().skillMap.end() || player.dead) return;
	if(c.casterClass != player.Main().classType) return;
	LOG("Cast :: %ls skill=%d release", player.name.data(), (i32)c.skillID);
	EndCastStatuses(player, c);
	GroupTeleport(player, c.skillID, c.castPos);
	if(f->second.action == ActionStateID::INVALID) return;
	const GameXmlContent::SkillCast* sc = GetGameXmlContent().FindSkillCast((i32)c.skillID);
	if(sc && sc->type == GameXmlContent::SkillCast::CHARGING && sc->maxChargingTime > 0 && sc->maxChargeRatio != sc->minChargeRatio) {
		const f32 held = MIN((f32)TimeDiffSec(TimeDiff(c.startedAt, localTime)) / sc->maxChargingTime, 1.0f);
		player.castChargeRatio = sc->minChargeRatio + (sc->maxChargeRatio - sc->minChargeRatio) * held;
		LOG("Cast :: %ls skill=%d charge ratio %.2f", player.name.data(), (i32)c.skillID, player.castChargeRatio);
	}
	vec3 releasePos = c.castPos;
	if(player.clientHd == ClientHandle::INVALID && c.type != GameXmlContent::SkillCast::DELAY && !c.lifted && glm::length(vec2(c.castPos)) > 1.0f) {
		const ActorUID aim = c.target != ActorUID::INVALID ? c.target : player.botAimTarget;
		const ActorMaster* tm = aim != ActorUID::INVALID ? FindMasterActor(aim) : nullptr;
		if(tm && !tm->parent->dead && !tm->parent->untargetable) {
			releasePos = tm->parent->body->GetWorldPos();
			const f32 yaw = atan2f(releasePos.y - player.body->GetWorldPos().y, releasePos.x - player.body->GetWorldPos().x);
			player.input.rot.upperYaw = yaw;
			player.input.rot.bodyYaw = yaw;
			player.weaponAimYaw = yaw;
		}
	}
	PlayerCastSkill(player, c.skillID, releasePos, Slice<const ActorUID>(&c.target, c.target != ActorUID::INVALID ? 1 : 0), 0, f->second.action);
	player.castChargeRatio = 1.0f;
}

void World::EndActionStatuses(Player& player)
{
	foreach(it, activeStatuses) {
		if(it->targetIndex != player.index || it->casterIndex != player.index || !it->aniEnd || it->appliedAt < player.lastCastStart || it->until <= localTime) continue;
		it->until = localTime;
		LOG("Status :: %ls status=%d removed at client action end", player.name.data(), it->statusID);
	}
}

void World::EndCastStatuses(Player& player, const Player::Casting& c)
{
	foreach(it, activeStatuses) {
		if(it->targetIndex != player.index || it->casterIndex != player.index || !it->aniEnd || it->appliedAt < c.startedAt || it->until <= localTime) continue;
		it->until = localTime;
		LOG("Status :: %ls status=%d removed at cast release", player.name.data(), it->statusID);
	}
	if(const auto* cleanupList = GetGameXmlContent().FindComboCleanupStatuses((i32)c.skillID)) {
		const auto& cleanup = *cleanupList;
		foreach_const(cs, cleanup) {
			const StatusInfo* info = GetGameXmlContent().FindStatus(*cs);
			const i32 pair = info ? info->casterStatus : 0;
			foreach(it, activeStatuses) {
				if(it->statusID != *cs && (pair == 0 || it->statusID != pair)) continue;
				if((it->targetIndex != player.index && it->casterIndex != player.index) || it->until <= localTime) continue;
				it->until = localTime;
				LOG("Status :: %ls status=%d cleaned up at combo end", player.name.data(), it->statusID);
			}
			foreach(ns, npcStatuses) {
				if((ns->statusID != *cs && (pair == 0 || ns->statusID != pair)) || ns->casterUID != player.Main().UID || ns->until <= localTime) continue;
				ns->until = localTime;
				LOG("Status :: %ls status=%d on creature cleaned up at combo end", player.name.data(), ns->statusID);
			}
		}
	}
}

void World::DropPendingActions(Player& player)
{
	i32 dropped = 0;
	for(auto pr = pendingRemotes.begin(); pr != pendingRemotes.end(); ) {
		if(pr->ownerIndex == player.index && pr->sourceUID == ActorUID::INVALID && !pr->remove && localTime < pr->time) {
			pr = pendingRemotes.erase_unsorted(pr);
			dropped++;
		}
		else ++pr;
	}
	for(auto ps = pendingStatuses.begin(); ps != pendingStatuses.end(); ) {
		if(ps->casterIndex == player.index && ps->sourceUID == ActorUID::INVALID && !ps->remove && localTime < ps->time) {
			ps = pendingStatuses.erase_unsorted(ps);
			dropped++;
		}
		else ++ps;
	}
	if(dropped > 0) LOG("Skill :: %ls new action, %d pending commands dropped", player.name.data(), dropped);
}

void World::InterruptSkills(Player& player)
{
	i32 dropped = 0;
	for(auto pr = pendingRemotes.begin(); pr != pendingRemotes.end(); ) {
		if(pr->ownerIndex == player.index && pr->sourceUID == ActorUID::INVALID && !pr->remove && localTime < pr->time) {
			pr = pendingRemotes.erase_unsorted(pr);
			dropped++;
		}
		else ++pr;
	}
	i32 programs = 0;
	foreach(sp, skillProgramList) {
		if(sp->IsDoneExecuting()) continue;
		const ActorMaster* owner = FindMasterActor(sp->casterUID);
		if(!owner || owner->parent != &player) continue;
		sp->Finish();
		programs++;
	}
	if(dropped > 0 || programs > 0) LOG("Skill :: %ls interrupted, %d pending remotes and %d skill programs dropped", player.name.data(), dropped, programs);
	if(player.casting.skillID != SkillID::INVALID) CancelCast(player, "interrupted");
}

void World::CancelCast(Player& player, const char* reason, ClientHandle skipHd)
{
	LOG("Cast :: %ls skill=%d cancelled (%s)", player.name.data(), (i32)player.casting.skillID, reason);
	replication->SendCancelSkillToAll(player.Main().UID, player.casting.skillID, skipHd);
	const Player::Casting c = player.casting;
	player.casting.skillID = SkillID::INVALID;
	if(!player.dead) EndCastStatuses(player, c);
}

void World::UpdateCasts()
{
	foreach(it, players) {
		if(it->comboPending != SkillID::INVALID && it->comboSlot >= 0 && localTime >= it->comboSendAt) {
			replication->SendChangeSkillSlot(it->clientHd, it->Main().UID, it->comboSlot, it->comboPending, 0);
			it->comboPending = SkillID::INVALID;
		}
		if(it->comboAutoAt != Time::ZERO && localTime >= it->comboAutoAt) {
			it->comboAutoAt = Time::ZERO;
			if((it->comboSlot >= 0 || it->clientHd == ClientHandle::INVALID) && it->comboSkill != SkillID::INVALID && !it->dead) {
				const GameXmlContent::SkillInput* comboIn = GetGameXmlContent().FindSkillInput((i32)it->comboSkill);
				const bool petToCaster = it->clientHd == ClientHandle::INVALID && comboIn && comboIn->petCaster && comboIn->needsClick && IsCarrying(*it);
				const vec3 comboPos = petToCaster ? it->body->GetWorldPos() : it->comboCastPos;
				LOG("Combo :: %ls auto-cast %d at (%.0f, %.0f)", it->name.data(), (i32)it->comboSkill, comboPos.x, comboPos.y);
				PlayerCastSkill(*it, it->comboSkill, comboPos, Slice<const ActorUID>(), 0);
			}
		}
		if(it->comboSlot >= 0 && localTime >= it->comboUntil) {
			replication->SendChangeSkillSlot(it->clientHd, it->Main().UID, it->comboSlot, it->comboBase, 0);
			LOG("Combo :: %ls slot %d window ended", it->name.data(), it->comboSlot);
			it->comboSlot = -1;
		}
		if(it->hitCombo != SkillID::INVALID && localTime >= it->hitComboAt) {
			const SkillID combo = it->hitCombo;
			const ActorUID comboTarget = it->hitComboTarget;
			it->hitCombo = SkillID::INVALID;
			const ActorMaster* tm = FindMasterActor(comboTarget);
			const ActorNpc* tn = tm ? nullptr : FindNpcActor(comboTarget);
			if(!it->dead && tm && !tm->parent->dead) {
				LOG("Combo :: %ls hit auto-cast %d on %ls", it->name.data(), (i32)combo, tm->parent->name.data());
				PlayerCastSkill(*it, combo, tm->parent->body->GetWorldPos(), Slice<const ActorUID>(&comboTarget, 1), 0);
			}
			else if(!it->dead && tn && tn->hp > 0 && tn->removeAt == Time::ZERO) {
				LOG("Combo :: %ls hit auto-cast %d on creature=%d", it->name.data(), (i32)combo, (i32)tn->docID);
				PlayerCastSkill(*it, combo, tn->pos, Slice<const ActorUID>(&comboTarget, 1), 0);
			}
		}
		if(it->casting.skillID == SkillID::INVALID || !it->body) continue;
		if(it->dead) { CancelCast(*it, "dead"); continue; }
		const bool bot = it->clientHd == ClientHandle::INVALID;
		if(bot && (it->casting.lifted || it->casting.interruptByLostTarget)) {
			const ActorMaster* tm = FindMasterActor(it->casting.target);
			const ActorNpc* tn = tm ? nullptr : FindNpcActor(it->casting.target);
			const bool npcAlive = tn && tn->hp > 0 && tn->removeAt == Time::ZERO;
			const bool lost = tm ? (tm->parent->dead || tm->parent->untargetable) : !npcAlive;
			if(lost && it->casting.interruptByLostTarget) {
				LOG("Cast :: %ls skill=%d target lost", it->name.data(), (i32)it->casting.skillID);
				ReleaseCast(*it);
				continue;
			}
			if(!lost && it->casting.lifted) TeleportPlayer(*it, tm ? tm->parent->body->GetWorldPos() : tn->pos, it->input.rot.upperYaw);
		}
		if(localTime < it->casting.executeAt) continue;
		if(!bot && it->casting.type == GameXmlContent::SkillCast::LIMIT) CancelCast(*it, "window ended");
		else ReleaseCast(*it);
	}
}

void World::UpdateStances()
{
	foreach(it, players) {
		if(!it->body) continue;
		ActorMaster& m = it->Main();
		if(m.stance == 0) continue;
		if((m.stanceUntil != Time::ZERO && localTime >= m.stanceUntil) || (it->dead && m.stanceCancelByDefeat)) SetStance(m, 0, 0, 0, false);
	}
}

bool World::ScheduleSkillRemotes(Player& player, SkillID skillID, const vec3& castPos, Slice<const ActorUID> targets, f32 angle, i32 forceAnim, ActionStateID forcedAction, ClassType casterClass)
{
	const auto& content = GetGameXmlContent();
	auto skillFound = content.skillMap.find(skillID);
	if(skillFound == content.skillMap.end()) return false;
	const auto& skill = skillFound->second;
	const bool deferred = casterClass != ClassType::NONE && casterClass != player.Main().classType;
	if(!deferred) casterClass = player.Main().classType;
	const ClassType actionClass = deferred ? casterClass : ActionClass(player);
	const i8 casterStance = deferred ? player.Sub().stance : player.Main().stance;
	const char* casterClassName = deferred ? content.GetMaster(casterClass).className.c_str() : ActionClassName(player);
	const ActionStateID actionID = forcedAction != ActionStateID::INVALID ? forcedAction : skill.action;
	if(!content.SkillActionKnown(actionClass, actionID, casterStance)) {
		LOG("Skill :: %ls skill=%d action %d not on active master %d, dropped", player.name.data(), (i32)skillID, (i32)actionID, (i32)casterClass);
		return false;
	}
	const auto& action = content.GetSkillAction(actionClass, actionID, casterStance);

	i32 anim = 0;
	if(player.lastComboSkill == skillID && TimeDiffSec(TimeDiff(player.lastComboTime, localTime)) < 3.0) {
		foreach_const(cmd, action.commands) {
			if(cmd->animIndex == 1 && cmd->type == ActionCommand::Type::REMOTE) anim = 1;
		}
	}
	if(forceAnim >= 0) {
		anim = forceAnim;
	}
	else {
		player.lastComboSkill = anim == 0 ? skillID : SkillID::INVALID;
		player.lastComboTime = localTime;
	}

	const u32 castID = nextCastID++;
	const i32 damage = (i32)(PlayerSkillDamage(player, skillID) * player.castChargeRatio);
	eastl::fixed_vector<ActorUID,8,false> liveTargets;
	foreach_const(t, targets) if(!liveTargets.full() && (FindMasterActor(*t) || FindNpcActor(*t))) liveTargets.push_back(*t);
	const ActorUID currentTarget = liveTargets.size() > 0 ? liveTargets[0] : ActorUID::INVALID;
	u32 lockOnIndex = 0;
	const i32 upgradeSummon = UpgradeSummon(player, (i32)skillID);

	bool scheduled = false;
	bool teleportToChild = false;
	bool teleportToSpawn = false;
	bool teleportToTarget = false;
	eastl::fixed_vector<SkillAnimPass,8,false>& animPasses = lastAnimPasses;
	animPasses.clear();
	if(anim == 0 && forceAnim < 0) {
		const bool hitEventCombo = GetGameXmlContent().FindSkillHitEventRemote((i32)skillID) != 0;
		bool regCombo = false;
		bool acts[8] = {};
		bool visuals[8] = {};
		bool moves[8] = {};
		foreach_const(cmd, action.commands) {
			if(cmd->type == ActionCommand::Type::REGCOMBO) regCombo = true;
			if((cmd->type == ActionCommand::Type::MOVE || cmd->type == ActionCommand::Type::GRAPH_MOVE_HORZ) && cmd->animIndex >= 0 && cmd->animIndex < 8) moves[cmd->animIndex] = true;
			bool a = cmd->type == ActionCommand::Type::REMOTE || cmd->type == ActionCommand::Type::REMOTE_SKILL_TARGET ||
				cmd->type == ActionCommand::Type::STATUS || cmd->type == ActionCommand::Type::STATUS_SKILL_TARGET || cmd->type == ActionCommand::Type::ENTITY;
			if(cmd->type == ActionCommand::Type::REMOTE || cmd->type == ActionCommand::Type::REMOTE_SKILL_TARGET) {
				const Remote* r = content.FindRemote(cmd->remote.idx);
				const bool visualOnly = r && r->boundType == Remote::BoundType::E_BOUND_NONE && r->onDestroy.empty() && r->onTime.empty() && r->onActivate.empty() &&
					(r->damageGroup == Remote::DamageGroup::eNONE || r->damageGroup == Remote::DamageGroup::INVALID);
				if(visualOnly) {
					a = false;
					if(cmd->animIndex >= 0 && cmd->animIndex < 8) visuals[cmd->animIndex] = true;
				}
			}
			if(cmd->animIndex >= 0 && cmd->animIndex < 8) acts[cmd->animIndex] |= a;
		}
		const char* className = casterClassName;
		i32 lastActing = -1;
		for(i32 i = 0; i < 8; i++) if(acts[i] || moves[i]) lastActing = i;
		if(!hitEventCombo || !acts[0]) {
			f32 start = 0;
			for(i32 i = 0; i <= lastActing && (!regCombo || acts[0] || i <= 1); i++) {
				if((acts[i] || visuals[i] || moves[i]) && !animPasses.full()) animPasses.push_back({ i, start });
				i32 animId = 0, loop = 1;
				foreach_const(st, action.anims) if(st->index == i) { animId = st->animationId; loop = MAX(st->loop, 1); }
				start += content.GetAniSeqTime(className, animId) * (f32)loop;
				if(regCombo && !acts[0] && acts[i]) break;
			}
		}
	}
	if(animPasses.empty()) animPasses.push_back({ anim, schedulePassOffset });
	anim = animPasses[0].index;

	const i32 passes = (i32)animPasses.size();
	eastl::fixed_vector<eastl::pair<i32, u8>,16,true> docCounts;
	for(i32 pass = 0; pass < passes; pass++) {
	const i32 curAnim = animPasses[pass].index;
	foreach_const(cmd, action.commands) {
		if(cmd->animIndex != curAnim && (cmd->animIndex != -1 || pass > 0)) continue;
		const f32 t = animPasses[pass].start + cmd->delay;
		if((cmd->type == ActionCommand::Type::REMOTE || cmd->type == ActionCommand::Type::REMOTE_SKILL_TARGET) && !pendingRemotes.full()) {
			PendingRemote pr;
			pr.casterMasterUID = player.Main().UID;
			pr.time = TimeAdd(localTime, TimeMsToTime(t * 1000.0));
			pr.idx = cmd->remote.idx;
			pr.costLevel = castCostLevel;
			if(castCostLevel) {
				foreach_const(rr, castCostLevel->remoteReplace) if(rr->first == (i32)pr.idx) pr.idx = RemoteIdx(rr->second);
				if(pr.idx != cmd->remote.idx) pr.sendIdx = cmd->remote.idx;
			}
			pr.preset = (i32)cmd->remote.targetPreset;
			pr.offsetSide = cmd->remote.offsetSide;
			pr.offsetFront = cmd->remote.offsetFront;
			pr.ownerIndex = player.index;
			pr.skillID = skillID;
			pr.castID = castID;
			pr.damage = castCostLevel ? (i32)(damage * castCostLevel->damageRatio) : damage;
			pr.castPos = castPos;
			pr.angle = cmd->lowerDummyDir ? MovementYaw(player, angle) : angle;
			pr.target = cmd->remote.targetPreset == ActionCommand::TargetPreset::MEMORIZED_TFT ? player.memorizedTarget : currentTarget;
			if(cmd->remote.targetPreset == ActionCommand::TargetPreset::MULTI_LOCKON_FFF && liveTargets.size() > 0) pr.target = liveTargets[lockOnIndex++ % liveTargets.size()];
			pr.atTarget = cmd->type == ActionCommand::Type::REMOTE_SKILL_TARGET;
			pr.stopAtTarget = cmd->remote.stopAtTarget;
			pr.remove = cmd->remote.remove != 0;
			pr.nodeName = cmd->nodeName;
			pr.rootDummyDir = cmd->rootDummyDir != 0;
			pr.casterClass = casterClass;
			{
				u8* count = nullptr;
				foreach(dc, docCounts) if(dc->first == (i32)cmd->remote.idx) count = &dc->second;
				if(!count) { docCounts.push_back(eastl::make_pair((i32)cmd->remote.idx, (u8)0)); count = &docCounts.back().second; }
				pr.docIndex = (*count)++;
			}
			if(cmd->remote.removeAtAnimEnd) {
				i32 animId = 0, loop = 1;
				foreach_const(st, action.anims) if(st->index == curAnim) { animId = st->animationId; loop = MAX(st->loop, 1); }
				const f32 passEnd = animPasses[pass].start + content.GetAniSeqTime(casterClassName, animId) * (f32)loop;
				if(passEnd > t) pr.aniEndAt = TimeAdd(localTime, TimeMsToTime(passEnd * 1000.0));
			}
			pendingRemotes.push_back(pr);
			scheduled = true;
		}
		if(cmd->type == ActionCommand::Type::TELEPORT && cmd->teleport.toLastChild && t <= 0.001f) teleportToChild = true;
		if(cmd->type == ActionCommand::Type::TELEPORT && cmd->teleport.toSpawnPoint) teleportToSpawn = true;
		if(cmd->type == ActionCommand::Type::TELEPORT && cmd->teleport.toTarget) {
			if(t <= 0.001f || currentTarget == ActorUID::INVALID || pendingInteractions.full()) teleportToTarget = true;
			else {
				PendingInteraction pt;
				pt.time = TimeAdd(localTime, TimeMsToTime(t * 1000.0));
				pt.ownerIndex = player.index;
				pt.interactionID = 0;
				pt.teleportTarget = currentTarget;
				pendingInteractions.push_back(pt);
			}
		}
		if(cmd->type == ActionCommand::Type::INTERACTION && cmd->interaction.interactionID != 0 && !pendingInteractions.full()) {
			PendingInteraction pi;
			pi.time = TimeAdd(localTime, TimeMsToTime(t * 1000.0));
			pi.ownerIndex = player.index;
			pi.interactionID = cmd->interaction.interactionID;
			pendingInteractions.push_back(pi);
		}
		if(cmd->type == ActionCommand::Type::ENTITY && cmd->entity.entityIndex != 0 && !pendingSummons.full()) {
			PendingSummon ps;
			ps.time = TimeAdd(localTime, TimeMsToTime(t * 1000.0));
			ps.creature = cmd->entity.entityIndex;
			if(upgradeSummon != 0 && upgradeSummon / 10 == ps.creature / 10) ps.creature = upgradeSummon;
			ps.ownerIndex = player.index;
			ps.remove = cmd->entity.remove != 0;
			const auto preset = cmd->entity.targetPreset;
			vec3 pos = player.body->GetWorldPos();
			if(preset == ActionCommand::TargetPreset::POS_FFF || preset == ActionCommand::TargetPreset::POS_TFF ||
			   preset == ActionCommand::TargetPreset::POS_TTT_1 || preset == ActionCommand::TargetPreset::POS_TFF_1_800_1000) {
				if(glm::length(vec2(castPos)) > 1.0f) pos = vec3(vec2(castPos), pos.z);
			}
			if(cmd->entity.offsetFront != 0 || cmd->entity.offsetSide != 0) {
				const vec2 face = vec2(cosf(angle), sinf(angle));
				pos += vec3(face * cmd->entity.offsetFront + Perp(face) * cmd->entity.offsetSide, 0);
			}
			ps.pos = pos;
			ps.rot = vec3(0, 0, angle);
			if(preset == ActionCommand::TargetPreset::CURRENT_FFF) ps.target = currentTarget;
			pendingSummons.push_back(ps);
			scheduled = true;
		}
		if((cmd->type == ActionCommand::Type::STATUS || cmd->type == ActionCommand::Type::STATUS_SKILL_TARGET) && cmd->status.statusIndex != 0 && !pendingStatuses.full() &&
		   (cmd->status.condition == ActionCommand::StatusCondition::NONE || StatusConditionMet(player, currentTarget, cmd->status.condition))) {
			PendingStatus ps;
			ps.time = TimeAdd(localTime, TimeMsToTime(t * 1000.0));
			ps.statusID = cmd->status.statusIndex;
			f32 replaceDuration = -1.0f;
			if(castCostLevel) {
				foreach_const(sr, castCostLevel->statusReplace) {
					if(sr->statusID != cmd->status.statusIndex) continue;
					ps.statusID = sr->replaceID;
					if(sr->durationSec > 0) replaceDuration = sr->durationSec;
				}
			}
			if(ps.statusID == cmd->status.statusIndex) ps.statusID = PlayerUpgradedStatus(player, ps.statusID, &replaceDuration);
			if(skillID == player.stageSkills[0] || skillID == player.stageSkills[1]) ps.statusID = content.LevelStatus((i32)skillID, player.ingameLevel, ps.statusID);
			ps.casterIndex = player.index;
			const auto preset = cmd->status.targetPreset;
			const bool self = preset == ActionCommand::TargetPreset::SELF_FFF || preset == ActionCommand::TargetPreset::SELF_FFF_1 ||
				preset == ActionCommand::TargetPreset::SELF_TFF || preset == ActionCommand::TargetPreset::SELF_TTF || preset == ActionCommand::TargetPreset::SELF_TTT;
			const bool lastChild = preset == ActionCommand::TargetPreset::LAST_CHILD_FFF || preset == ActionCommand::TargetPreset::LAST_CHILD_TFF || preset == ActionCommand::TargetPreset::CHILD_INDEX_FFF;
			ps.targetUID = lastChild ? ActorUID::INVALID : self ? player.Main().UID : currentTarget;
			ps.duration = replaceDuration;
			if(cmd->status.removeAtEnd) {
				i32 animId = 0, loop = 1;
				foreach_const(st, action.anims) if(st->index == curAnim) { animId = st->animationId; loop = MAX(st->loop, 1); }
				f32 wholeLen = 0;
				if(animId == 0) foreach_const(st, action.anims) wholeLen += content.GetAniSeqTime(casterClassName, st->animationId) * (f32)MAX(st->loop, 1);
				const f32 passLen = animId != 0 ? content.GetAniSeqTime(casterClassName, animId) * (f32)loop : MAX(action.seqLength, wholeLen);
				f32 endAt = animPasses[pass].start + passLen;
				if(cmd->status.removeAtEnd == (u8)ActionCommand::StatusEnd::ACTION) {
					f32 total = 0;
					foreach_const(st, action.anims) total += content.GetAniSeqTime(casterClassName, st->animationId) * (f32)MAX(st->loop, 1);
					endAt = MAX(endAt, total);
				}
				ps.duration = MAX(endAt - t, 0.3f);
				const StatusInfo* capInfo = content.FindStatus(cmd->status.statusIndex);
				if(capInfo && capInfo->durationSec > 0) ps.duration = MIN(ps.duration, capInfo->durationSec);
			}
			ps.power = (f32)damage;
			ps.remove = cmd->status.remove != 0;
			ps.aniEnd = cmd->status.removeAtEnd != 0;
			ps.cleanupOnly = cmd->status.removeAtEnd == (u8)ActionCommand::StatusEnd::CLEANUP;
			if(cmd->type == ActionCommand::Type::STATUS_SKILL_TARGET && targets.size() > 1) {
				foreach_const(tg, targets) {
					if(pendingStatuses.full()) break;
					ps.targetUID = *tg;
					pendingStatuses.push_back(ps);
				}
			}
			else if(lastChild && !ps.remove) {
				ps.lastChild = true;
				ps.childCreature = cmd->status.childCreature;
				ps.time = TimeAdd(ps.time, TimeMsToTime(LAST_CHILD_STATUS_DELAY_MS));
				pendingStatuses.push_back(ps);
			}
			else if(ps.targetUID != ActorUID::INVALID) pendingStatuses.push_back(ps);
		}
	}
	}

	if(player.clientHd == ClientHandle::INVALID && !player.dead) {
		bool moves = false;
		bool freeMove = false;
		foreach_const(cmd, action.commands) {
			if(cmd->type == ActionCommand::Type::MOVESPEED && cmd->moveSpeed.percent >= ACTION_FULL_MOVESPEED_PERCENT) freeMove = true;
			if(cmd->type == ActionCommand::Type::GRAPH_MOVE_HORZ || cmd->type == ActionCommand::Type::TELEPORT) moves = true;
			if(cmd->type == ActionCommand::Type::MOVE && cmd->move.preset != ActionCommand::MovePreset::MOVE_NONE && cmd->move.preset != ActionCommand::MovePreset::SEE_TARGET &&
				cmd->move.preset != ActionCommand::MovePreset::SEE_CURRENT_TARGET && cmd->move.preset != ActionCommand::MovePreset::SEE_MEMORIZED_TARGET) moves = true;
		}
		{
			f32 still = 0;
			bool blocked = false;
			for(i32 pass = 0; pass < passes; pass++) {
				i32 animId = 0, loop = 1;
				foreach_const(st, action.anims) if(st->index == animPasses[pass].index) { animId = st->animationId; loop = MAX(st->loop, 1); }
				const f32 passEnd = animPasses[pass].start + content.GetAniSeqTime(casterClassName, animId) * (f32)loop;
				foreach_const(cmd, action.commands) {
					if(cmd->type != ActionCommand::Type::STATE_BLOCK || cmd->animIndex != animPasses[pass].index) continue;
					still = MAX(still, animPasses[pass].start + cmd->delay);
					blocked = true;
				}
				if(!blocked) still = MAX(still, passEnd);
			}
			if(still > 0 && !freeMove) {
				player.botSkillStillUntil = TimeAdd(localTime, TimeMsToTime(still * 1000.0));
				if(!moves) player.input.moveTo = player.body->GetWorldPos();
			}
		}
	}

	if(teleportToTarget && !player.dead && currentTarget != ActorUID::INVALID) TeleportNextToTarget(player, currentTarget);
	if(teleportToSpawn && !player.dead) {
		LOG("Teleport :: %ls back to spawn (%.0f, %.0f)", player.name.data(), player.spawnPos.x, player.spawnPos.y);
		WarpPlayer(player, player.spawnPos, player.input.rot.upperYaw);
	}
	if(teleportToChild) {
		const ActorNpc* child = FindNpcActor(player.lastChildUID);
		if(child && localTime < child->despawnAt) {
			const vec3 to = vec3(vec2(child->pos), MAX(child->pos.z, player.body->GetWorldPos().z));
			LOG("Teleport :: %ls to its last entity %d at (%.0f, %.0f)", player.name.data(), (i32)child->docID, to.x, to.y);
			TeleportPlayer(player, to, player.input.rot.upperYaw);
			replication->SendPlayerSyncTeleportToAll(player.Main().UID, to);
		}
		else LOG("Teleport :: %ls has no live entity to teleport to", player.name.data());
	}
	LOG("Skill :: %ls skill=%d anim=%d%s scheduled=%d damage=%d castPos=(%.0f, %.0f)", player.name.data(), (i32)skillID, anim, passes > 1 ? "+1" : "", scheduled, damage, castPos.x, castPos.y);
	return scheduled;
}

void World::RunSkillProgramCommand(SkillProgram& prog, ActorMaster* caster, i32 cmdIndex, const vec2& dir)
{
	const auto& content = GetGameXmlContent();
	const auto& action = content.GetSkillAction(prog.actionClass, prog.actionID, prog.stance);
	const auto& cmd = action.commands[cmdIndex];
	f32 distance = 0;
	f32 moveDuration = 0;
	const GameXmlContent::MoveCurve* curve = nullptr;

	switch(cmd.type) {
		case ActionCommand::Type::STATE_BLOCK: {
			i32 movePercent = -1;
			foreach_const(c2, action.commands) if(c2->type == ActionCommand::Type::MOVESPEED) movePercent = MAX(movePercent, c2->moveSpeed.percent);
			const bool freeMove = caster->parent->clientHd != ClientHandle::INVALID || movePercent >= ACTION_FULL_MOVESPEED_PERCENT;
			if(!freeMove) caster->parent->body->lockedMoveUntil = TimeAddSec(localTime, cmd.delay);
		} break
			;
		case ActionCommand::Type::GRAPH_MOVE_HORZ: {
			distance = cmd.graphMoveHorz.distance;
			moveDuration = SkillMoveDuration(content, caster->parent ? ActionClassName(*caster->parent) : content.GetMaster(caster->classType).className.c_str(), action, cmd.animIndex);
			foreach_const(st, action.anims) {
				if(st->index != cmd.animIndex) continue;
				curve = content.GetMoveCurve(content.GetMaster(caster->classType).className.c_str(), st->animationId);
				if(!curve) continue;
				const f32 animLen = content.GetAniSeqTime(content.GetMaster(caster->classType).className.c_str(), st->animationId) * (f32)MAX(st->loop, 1);
				const f32 curveTime = curve->time > 0 ? curve->time : animLen;
				if(curveTime > 0) moveDuration = curveTime;
				else curve = nullptr;
				break;
			}
		} break;

		case ActionCommand::Type::MOVE: {
			switch(cmd.move.preset) {
				case ActionCommand::MovePreset::WARP: {
					distance = (f32)cmd.move.param2;
					moveDuration = 0.01f;
				} break;
				case ActionCommand::MovePreset::TARGET_POS:
				case ActionCommand::MovePreset::TARGET_POS_LEAP:
				case ActionCommand::MovePreset::GOTO_CURRENT_TARGET:
				case ActionCommand::MovePreset::GOTO_CURRENT_TARGET_CHASE:
				case ActionCommand::MovePreset::GOTO_CURRENT_TARGET_THROUGH:
				case ActionCommand::MovePreset::GOTO_CURRENT_TARGET_THROUGH_CHASE: {
					bool graphMove = false;
					foreach_const(c2, action.commands) if(c2->type == ActionCommand::Type::GRAPH_MOVE_HORZ) graphMove = true;
					if(!graphMove) {
						distance = 999999.0f;
						moveDuration = action.seqLength;
					}
				} break;
			}
		} break;

		default: break;
	}

	bool longJump = false;
	foreach_const(c2, action.commands) if(c2->type == ActionCommand::Type::LONG_JUMP) longJump = true;
	if(distance != 0) {
		Player& mover = *caster->parent;
		const vec2 from = vec2(mover.body->GetWorldPos());
		vec2 moveDir = distance < 0 ? -dir : dir;
		f32 moveDist = fabsf(distance);
		bool throughMove = false;
		foreach_const(c2, action.commands) {
			if(c2->type != ActionCommand::Type::MOVE) continue;
			const auto preset = c2->move.preset;
			if((preset == ActionCommand::MovePreset::TARGET_POS || preset == ActionCommand::MovePreset::TARGET_POS_LEAP) && glm::length(vec2(prog.castPos)) > 1.0f) {
				moveDist = glm::length(vec2(prog.castPos) - from);
			}
			const bool gotoTarget = preset == ActionCommand::MovePreset::GOTO_CURRENT_TARGET || preset == ActionCommand::MovePreset::GOTO_CURRENT_TARGET_CHASE ||
				preset == ActionCommand::MovePreset::GOTO_CURRENT_TARGET_THROUGH || preset == ActionCommand::MovePreset::GOTO_CURRENT_TARGET_THROUGH_CHASE;
			if(gotoTarget && !prog.targetList.empty()) {
				vec2 targetPos;
				f32 targetRadius = 0;
				if(TargetBody(prog.targetList[0], &targetPos, &targetRadius)) {
					const vec2 rel = targetPos - from;
					const f32 len = glm::length(rel);
					if(len > 1.0f) moveDir = rel / len;
					const bool through = preset == ActionCommand::MovePreset::GOTO_CURRENT_TARGET_THROUGH || preset == ActionCommand::MovePreset::GOTO_CURRENT_TARGET_THROUGH_CHASE;
					if(through) throughMove = true;
					if(!through) moveDist = MIN(moveDist, MAX(len - targetRadius - HitRadius(mover), 0.0f));
					else if(moveDist > 999990.0f) moveDist = len + targetRadius + HitRadius(mover);
				}
			}
		}
		if(moveDist > 999990.0f) moveDist = 0;
		const bool fly = longJump && mover.clientHd != ClientHandle::INVALID;
		if(moveDuration > 0.02f && moveDist > 0 && !fly) {
			const vec3 reach = physics.FindMovePos(mover.body, vec3(moveDir * moveDist, 0), moveDuration);
			const f32 along = glm::dot(vec2(reach) - from, moveDir);
			if(along >= 0 && along < moveDist - 1.0f) {
				LOG("Dash :: %ls blocked after %.0f of %.0f", mover.name.data(), along, moveDist);
				moveDist = along;
			}
			if(!throughMove && mover.clientHd != ClientHandle::INVALID) {
				const f32 fromZ = mover.body->GetWorldPos().z;
				foreach_const(it, actorNpcList) {
					if(!it->monsterAi || it->monsterAi->movingBodyRadius <= 0 || it->hp <= 0 || it->removeAt != Time::ZERO) continue;
					if(fromZ < it->pos.z - HitRadius(mover) || fromZ > it->pos.z + it->monsterAi->movingBodyHeight) continue;
					const vec2 rel = vec2(it->pos) - from;
					const f32 reach = it->monsterAi->movingBodyRadius + HitRadius(mover);
					const f32 ahead = glm::dot(rel, moveDir);
					if(ahead <= 0) continue;
					const f32 side2 = glm::dot(rel, rel) - ahead * ahead;
					if(side2 >= reach * reach) continue;
					const f32 stop = MAX(ahead - sqrtf(reach * reach - side2), 0.0f);
					if(stop >= moveDist) continue;
					LOG("Dash :: %ls stopped by the body of creature %d after %.0f of %.0f", mover.name.data(), (i32)it->docID, stop, moveDist);
					moveDist = stop;
				}
			}
		}
		mover.input.moveTo = mover.body->GetWorldPos();
		mover.body->vel = vec3(0);
		if(moveDuration <= 0.02f) {
			physics.Move(mover.body, vec3(moveDir * moveDist, 0), moveDuration);
			mover.input.moveTo = mover.body->GetWorldPos();
			LOG("Warp :: %ls warps %.0f, moved %.0f", mover.name.data(), moveDist, glm::length(vec2(mover.body->GetWorldPos()) - from));
		}
		else {
			mover.dashDir = moveDir;
			mover.dashSpeed = moveDist / moveDuration;
			mover.dashStartAt = localTime;
			mover.dashOrigin = mover.body->GetWorldPos();
			mover.dashDist = moveDist;
			mover.dashDone = 0;
			mover.dashCurve = curve;
			mover.dashFly = fly;
			if(fly) LOG("Dash :: %ls long jump flies %.0f over obstacles", mover.name.data(), moveDist);
			mover.dashAwaitClientStart = mover.clientHd != ClientHandle::INVALID;
			mover.dashUntil = TimeAdd(localTime, TimeMsToTime(moveDuration * 1000.0));
			f32 block = 0;
			foreach_const(c2, action.commands) if(c2->type == ActionCommand::Type::STATE_BLOCK && c2->animIndex == cmd.animIndex) block = MAX(block, c2->delay);
			mover.dashBlockUntil = block > 0 && block < moveDuration ? TimeAdd(localTime, TimeMsToTime(block * 1000.0)) : mover.dashUntil;
			if(curve) LOG("Dash :: %ls follows move graph over %.2fs for %.0f", mover.name.data(), moveDuration, moveDist);
		}
	}
}

void World::GainResource(Player& player, i32 slot, f32 amount)
{
	const f32 resMax = MasterStat(player.Main().classType, RESOURCES[slot].maxStat);
	if(resMax <= 0) return;
	player.res[slot] = MIN(MAX(MAX(player.res[slot], 0.0f) + amount, 0.0f), resMax);
	player.resDirty = true;
	if(slot == 1 && amount > 0) player.rageGainAt = localTime;
	LOG("Resource :: %ls slot %d +%.0f -> %.0f", player.name.data(), slot, amount, player.res[slot]);
}

void World::ApplyResourceGain(Player& player, ResourceEvent event)
{
	if(const ResourceGainRule* rule = FindResourceGainRule(player.Main().classType, event)) GainResource(player, rule->slot, rule->amount);
}

void World::ApplyRemoteHitResourceGain(Player& owner, RemoteIdx remote, u32 castID, u32 targetIndex)
{
	const ResourceGainRule* rule = FindResourceGainRule(owner.Main().classType, ResourceEvent::REMOTE_HIT_ENEMY, remote);
	if(!rule) return;
	if(owner.resourceGainCast != castID) {
		owner.resourceGainCast = castID;
		owner.resourceGainTargets.clear();
	}
	if(owner.resourceGainTargets.full() || eastl::find(owner.resourceGainTargets.begin(), owner.resourceGainTargets.end(), targetIndex) != owner.resourceGainTargets.end()) return;
	owner.resourceGainTargets.push_back(targetIndex);
	GainResource(owner, rule->slot, rule->amount);
}

void World::UpdateChases()
{
	foreach(it, players) {
		if(it->chaseTarget == ActorUID::INVALID) continue;
		const ActorMaster* tm = FindMasterActor(it->chaseTarget);
		if(it->dead || !tm || tm->parent->dead || localTime >= it->chaseUntil) {
			it->chaseTarget = ActorUID::INVALID;
			continue;
		}
		if(localTime < it->chaseNext) continue;
		it->chaseNext = TimeAdd(localTime, TimeMsToTime(CHASE_UPDATE_INTERVAL_MS));
		replication->SendTargetGraphMoveToAll(it->Main().UID, it->body->GetWorldPos(), it->chaseTarget, tm->parent->body->GetWorldPos(), it->chaseAction);
	}
}

void World::MarkInBattle(Player& player)
{
	player.battleActionAt = localTime;
}

f32 World::PlayerMoveSpeed(const Player& p) const
{
	const f32 baseSpeed = const_cast<CharacterModel&>(GetGameXmlContent().GetMaster(p.Main().classType).character).getMoveSpeed();
	f32 mul = StatusModMulSum(p, StatusMod::MOVESPEED);
	if(localTime < p.firingStatUntil || p.weaponState == WEAPON_STATE_CHARGE) mul += p.firingMoveSpeedMul;
	return baseSpeed * (p.battleStage == 2 ? BATTLE_SPEED_RATIO_ACTIVE : (p.battleStage == 1 ? BATTLE_SPEED_RATIO_RECENT : 1.0f)) * MovespeedMul(mul, MasterStat(p.Main().classType, STAT_MOVESPEED) + StatBonus(p, STAT_MOVESPEED));
}

void World::UpdateBattleStates()
{
	foreach(it, players) {
		if(it->Main().classType == ClassType::NONE) continue;
		u8 stage = 0;
		if(it->battleActionAt != Time::ZERO && !it->dead) {
			const f64 since = TimeDiffSec(TimeDiff(it->battleActionAt, localTime));
			stage = since < BATTLE_STAGE_SEC ? 2 : (since < 2 * BATTLE_STAGE_SEC ? 1 : 0);
		}
		if(stage == it->battleStage) continue;
		it->battleStage = stage;
		const f32 baseSpeed = const_cast<CharacterModel&>(GetGameXmlContent().GetMaster(it->Main().classType).character).getMoveSpeed();
		const f32 speed = baseSpeed * (stage == 2 ? BATTLE_SPEED_RATIO_ACTIVE : (stage == 1 ? BATTLE_SPEED_RATIO_RECENT : 1.0f));
		replication->SendBattleStateToAll(it->Main().UID, stage > 0, speed);
	}
}

void World::BreakHide(Player& player)
{
	foreach(it, activeStatuses) {
		if(it->targetIndex != player.index || !it->info || it->info->type != StatusType::HIDE || it->appliedAt >= localTime || it->until <= localTime) continue;
		it->until = localTime;
		LOG("Hide :: %ls cast ends status %d", player.name.data(), it->statusID);
	}
}

void World::UpdateHideVisibility()
{
	foreach(it, players) {
		ActorUID hidden = ActorUID::INVALID;
		if(!it->dead) {
			foreach_const(st, activeStatuses) {
				if(st->targetIndex == it->index && st->info && st->info->type == StatusType::HIDE && st->until > localTime) {
					hidden = st->targetUID;
					break;
				}
			}
		}
		eastl::fixed_vector<ActorUID,4,false> detectors;
		if(hidden != ActorUID::INVALID) {
			const vec2 at = vec2(it->body->GetWorldPos());
			const Faction mine = it->team == 0 ? Faction::RED : Faction::BLUE;
			foreach_const(npc, actorNpcList) {
				if(npc->faction == mine || (npc->despawnAt != Time::ZERO && localTime >= npc->despawnAt) || detectors.full()) continue;
				f32 radius = 0;
				if(npc->ownerUID != ActorUID::INVALID) {
					const SummonInfo* info = GetGameXmlContent().FindSummon((i32)npc->docID);
					radius = info ? info->hideDetectRadius : 0.0f;
				}
				else if(npc->hp > 0 && (npc->faction == Faction::RED || npc->faction == Faction::BLUE)) radius = GetGameXmlContent().CreatureHideDetectRadius((i32)npc->docID);
				if(radius > 0 && glm::length(vec2(npc->pos) - at) <= radius * HIDE_DETECT_RADIUS_UNIT) detectors.push_back(npc->UID);
			}
		}
		const ActorUID concealed = detectors.empty() ? hidden : ActorUID::INVALID;
		if(concealed != it->hiddenUID) {
			const ActorUID was = it->hiddenUID;
			it->hiddenUID = concealed;
			ShowHiddenToEnemies(*it, was, concealed);
		}
		foreach_const(d, it->hideDetectors) {
			if(eastl::find(detectors.begin(), detectors.end(), *d) != detectors.end() && hidden == it->hideDetectedUID) continue;
			SendHideDetectToAll(*d, it->hideDetectedUID, 0);
		}
		foreach_const(d, detectors) {
			if(eastl::find(it->hideDetectors.begin(), it->hideDetectors.end(), *d) != it->hideDetectors.end() && hidden == it->hideDetectedUID) continue;
			SendHideDetectToAll(*d, hidden, 1);
			LOG("Hide :: %ls detected by %u", it->name.data(), (u32)*d);
		}
		it->hideDetectors = detectors;
		it->hideDetectedUID = hidden;
	}
}

void World::SendHideDetectToAll(ActorUID detectorUID, ActorUID targetUID, u8 flag)
{
	foreach(viewer, players) {
		if(viewer->clientHd == ClientHandle::INVALID) continue;
		replication->SendHideDetect(viewer->clientHd, detectorUID, targetUID, flag);
	}
}

void World::ShowHiddenToEnemies(Player& player, ActorUID was, ActorUID hidden)
{
	(void)was;
	LOG("Hide :: %ls %s from enemies", player.name.data(), hidden != ActorUID::INVALID ? "hidden" : "visible");
}

static const f64 FOG_UPDATE_MS = 200.0;
static const f64 FOG_REVEAL_MS = 3000.0;
static const f32 FOG_EYE_HEIGHT = 150.0f;
static const f32 FOG_DEFAULT_SIGHT = 1600.0f;

void World::RevealToTeam(Player& player, i32 team)
{
	if(!replication->titanMode || team < 0 || team > 1 || team == (i32)player.team) return;
	player.fogRevealUntil[team] = TimeAdd(localTime, TimeMsToTime(FOG_REVEAL_MS));
}

static const i32 MEASURED_BUSH_HIDE_STATUS = 1200000010;
static const i32 MEASURED_BUSH_REVEAL_STATUS = 1200000011;

bool World::PlayerHasActiveStatus(const Player& player, i32 statusID) const
{
	foreach_const(it, activeStatuses) if(it->targetIndex == player.index && it->statusID == statusID && localTime < it->until) return true;
	return false;
}

void World::EndPlayerStatus(Player& player, i32 statusID)
{
	foreach(it, activeStatuses) if(it->targetIndex == player.index && it->statusID == statusID && localTime < it->until) it->until = localTime;
}

static const u32 MEASURED_BUSH_STATE_NONE = 0;
static const u32 MEASURED_BUSH_STATE_REVEALED = 1;
static const u32 MEASURED_BUSH_STATE_CONTESTED = 2;

void World::RefreshBushStates()
{
	const auto& content = GetGameXmlContent();
	foreach(d, actorDynamicList) {
		if(!content.FindBushSize((i32)d->docID)) continue;
		u32 teams = 0;
		bool revealed = false;
		foreach_const(p, players) {
			if(p->bushUID != d->UID || p->dead) continue;
			teams |= 1u << p->team;
			if(PlayerHasActiveStatus(*p, MEASURED_BUSH_REVEAL_STATUS)) revealed = true;
		}
		const u32 state = teams == 3u ? MEASURED_BUSH_STATE_CONTESTED : (revealed ? MEASURED_BUSH_STATE_REVEALED : MEASURED_BUSH_STATE_NONE);
		if(state == d->bushState) continue;
		d->bushState = (u8)state;
		replication->SendBushCurStateToAll(d->UID, state);
	}
}

void World::BushRevealOnFire(Player& player)
{
	if(!replication->titanMode || player.bushUID == ActorUID::INVALID) return;
	EndPlayerStatus(player, MEASURED_BUSH_HIDE_STATUS);
	ApplyStatus(player, player, MEASURED_BUSH_REVEAL_STATUS);
	RevealToTeam(player, player.team == 0 ? 1 : 0);
	RefreshBushStates();
}

static const f32 LIGHT_CELL_SIZE = 100.0f;
static const i32 LIGHT_GRID_WIDTH = 512;
static const u8 LIGHT_RADIUS_CELLS = 2;
static const f64 LIGHT_DURATION_MS = 3000.0;
static const u8 NETWORK_TEAM_BASE = 3;

static const f32 TAG_COOLTIME_FLOOR_SEC = 5.0f;

u32 World::TagCooltimeMs(const Player& owner) const
{
	const f32 mul = MAX(1.0f + StatusRegenBonus(owner, StatusMod::TAG_COOLTIME, true) / 100.0f, 0.0f);
	return (u32)(MAX(MasterStat(owner.Sub().classType, STAT_TAG_COOLTIME) * mul, TAG_COOLTIME_FLOOR_SEC) * 1000.0f + 0.5f);
}

void World::SendTagCooltimeToTeam(const Player& owner, u32 ms)
{
	if(!replication->titanMode) return;
	foreach_const(p, players) {
		if(p->clientHd != ClientHandle::INVALID && p->team == owner.team) replication->SendTagCooltime(p->clientHd, owner.Sub().UID, ms);
	}
}

void World::LightAttacker(const vec3& attackerPos, i32 victimTeam)
{
	if(!replication->titanMode || victimTeam < 0 || victimTeam > 1) return;
	const i32 cx = (i32)(attackerPos.x / LIGHT_CELL_SIZE);
	const i32 cy = (i32)(attackerPos.y / LIGHT_CELL_SIZE);
	if(cx < 0 || cy < 0 || cx >= LIGHT_GRID_WIDTH) return;
	const u32 cell = (u32)(cx + cy * LIGHT_GRID_WIDTH);
	const Time until = TimeAdd(localTime, TimeMsToTime(LIGHT_DURATION_MS));
	foreach(it, litCells) {
		if(it->cell == cell && it->team == (u8)victimTeam) { it->until = until; return; }
	}
	if(litCells.full()) return;
	litCells.push_back({ cell, (u8)victimTeam, until });
	foreach_const(p, players) {
		if(p->clientHd != ClientHandle::INVALID && p->team == (u8)victimTeam) replication->SendLightArea(p->clientHd, true, cell, (u8)(NETWORK_TEAM_BASE + victimTeam), LIGHT_RADIUS_CELLS);
	}
}

void World::UpdateLitCells()
{
	for(i32 i = 0; i < (i32)litCells.size(); ) {
		if(localTime < litCells[i].until) { i++; continue; }
		const LitCell lit = litCells[i];
		litCells.erase(litCells.begin() + i);
		foreach_const(p, players) {
			if(p->clientHd != ClientHandle::INVALID && p->team == lit.team) replication->SendLightArea(p->clientHd, false, lit.cell, (u8)(NETWORK_TEAM_BASE + lit.team), LIGHT_RADIUS_CELLS);
		}
	}
}

bool World::TeamSees(i32 team, const Player& target)
{
	if(target.fogRevealUntil[team] != Time::ZERO && localTime < target.fogRevealUntil[team]) return true;
	return TeamSeesAt(team, target.body->GetWorldPos(), target.bushUID);
}

bool World::TeamSeesAt(i32 team, const vec3& at, ActorUID bushUID, f32 slack, f32 sightMargin, const void* ignoreActor)
{
	const auto& content = GetGameXmlContent();
	const vec3 eye = at + vec3(0, 0, FOG_EYE_HEIGHT);
	f32 hit = 0;
	foreach_const(p, players) {
		if(p->team != (u8)team || p->dead || !p->body) continue;
		if(bushUID != ActorUID::INVALID && p->bushUID != bushUID) continue;
		const vec3 from = p->body->GetWorldPos();
		const f32 sight = content.CreatureSight(100000000 + (i32)p->Main().classType);
		if(glm::length(vec2(from) - vec2(at)) > (sight > 0 ? sight : FOG_DEFAULT_SIGHT) + sightMargin) continue;
		const PxRigidActor* blocker = nullptr;
		if(!physics.RaycastSight(from + vec3(0, 0, FOG_EYE_HEIGHT), eye, &hit, &blocker) || hit >= glm::length(vec2(from) - vec2(at)) - slack || (ignoreActor && (const void*)blocker == ignoreActor)) return true;
	}
	if(bushUID != ActorUID::INVALID) return false;
	foreach_const(lit, litCells) {
		if(lit->team != (u8)team) continue;
		const vec2 center = vec2(((lit->cell % LIGHT_GRID_WIDTH) + 0.5f) * LIGHT_CELL_SIZE, ((lit->cell / LIGHT_GRID_WIDTH) + 0.5f) * LIGHT_CELL_SIZE);
		if(glm::length(vec2(at) - center) <= LIGHT_RADIUS_CELLS * LIGHT_CELL_SIZE) return true;
	}
	foreach_const(npc, actorNpcList) {
		if(npc->hp <= 0 || npc->removeAt != Time::ZERO || (npc->despawnAt != Time::ZERO && localTime >= npc->despawnAt)) continue;
		const i32 npcTeam = npc->ownerUID != ActorUID::INVALID ? (i32)players[npc->ownerIndex].team : (npc->faction == Faction::RED ? 0 : npc->faction == Faction::BLUE ? 1 : -1);
		if(npcTeam != team) continue;
		const f32 sight = content.CreatureSight((i32)npc->docID);
		if(sight <= 0 || glm::length(vec2(npc->pos) - vec2(at)) > sight + sightMargin) continue;
		const PxRigidActor* npcBlocker = nullptr;
		if(!physics.RaycastSight(npc->pos + vec3(0, 0, FOG_EYE_HEIGHT), eye, &hit, &npcBlocker) || hit >= glm::length(vec2(npc->pos) - vec2(at)) - slack || (ignoreActor && (const void*)npcBlocker == ignoreActor)) return true;
	}
	return false;
}

void World::UpdateDynamicFog()
{
	static const f32 DYNAMIC_FOG_SLACK = 250.0f;
	static const f32 MEASURED_DYNAMIC_SIGHT_MARGIN = 400.0f;
	const auto& content = GetGameXmlContent();
	foreach(d, actorDynamicList) {
		bool sees[2];
		const i32 ownTeam = d->faction == Faction::RED ? 0 : d->faction == Faction::BLUE ? 1 : -1;
		for(i32 team = 0; team < 2; team++) sees[team] = team == ownTeam || TeamSeesAt(team, d->pos, ActorUID::INVALID, DYNAMIC_FOG_SLACK, MEASURED_DYNAMIC_SIGHT_MARGIN, d->collider);
		const bool silent = !d->fogInit;
		d->fogInit = true;
		const bool jumpWall = content.dynamicJumpable.count((i32)d->docID) != 0;
		const auto destroyable = content.dynamicDestroyHits.find((i32)d->docID);
		const f32 hpLeft = jumpWall && destroyable != content.dynamicDestroyHits.end() && destroyable->second > 0 ? MAX(1.0f - (f32)d->destroyHits / (f32)destroyable->second, 0.0f) : 1.0f;
		foreach(viewer, players) {
			if(viewer->clientHd == ClientHandle::INVALID || viewer->team > 1) continue;
			const u32 bit = 1u << viewer->index;
			const bool hidden = (d->fogHiddenFrom & bit) != 0;
			if(!sees[viewer->team] && !hidden && !viewer->devVision) {
				if(!silent) replication->SendLeaveActor(viewer->clientHd, d->UID);
				d->fogHiddenFrom |= bit;
			}
			else if((sees[viewer->team] || viewer->devVision) && hidden) {
				replication->SendEnterDynamic(viewer->clientHd, d->UID, d->pos, d->rot.z, d->action, jumpWall, MEASURED_JUMPWALL_MAX_HP * hpLeft, MEASURED_JUMPWALL_MAX_HP);
				d->fogHiddenFrom &= ~bit;
			}
		}
	}
}

ActorUID World::BushAt(const vec3& at) const
{
	const auto& content = GetGameXmlContent();
	foreach_const(d, actorDynamicList) {
		const vec2* size = content.FindBushSize((i32)d->docID);
		if(!size) continue;
		const vec2 rel = vec2(at) - vec2(d->pos);
		const f32 c = cosf(-d->rot.z), s = sinf(-d->rot.z);
		const vec2 local = vec2(rel.x * c - rel.y * s, rel.x * s + rel.y * c);
		if(fabsf(local.x) <= size->x * 0.5f && fabsf(local.y) <= size->y * 0.5f) return d->UID;
	}
	return ActorUID::INVALID;
}

void World::UpdateSummonBushes()
{
	const auto& content = GetGameXmlContent();
	foreach(npc, actorNpcList) {
		if(npc->ownerUID == ActorUID::INVALID || content.CreatureBushPass((i32)npc->docID)) continue;
		const bool alive = npc->hp > 0 && npc->removeAt == Time::ZERO;
		const ActorUID bush = alive ? BushAt(npc->pos) : ActorUID::INVALID;
		NpcStatus* hide = nullptr;
		NpcStatus* reveal = nullptr;
		foreach(ns, npcStatuses) {
			if(ns->npcUID != npc->UID || localTime >= ns->until) continue;
			if(ns->statusID == MEASURED_BUSH_HIDE_STATUS) hide = &*ns;
			if(ns->statusID == MEASURED_BUSH_REVEAL_STATUS) reveal = &*ns;
		}
		if(bush != npc->bushUID) {
			npc->bushUID = bush;
			replication->SendBushStateToAll(npc->UID, bush);
			if(bush == ActorUID::INVALID) {
				if(hide) hide->until = localTime;
				if(reveal) reveal->until = localTime;
				continue;
			}
		}
		if(bush == ActorUID::INVALID) continue;
		if(hide && reveal && reveal->appliedAt > hide->appliedAt) hide->until = localTime;
		if(!hide && !reveal && !npcStatuses.full()) {
			replication->SendAddStatusToAll(MEASURED_BUSH_HIDE_STATUS, npc->UID, ActorUID::INVALID, STATUS_INFINITE_DURATION_SEC, npc->pos, npc->pos, 0, 1, 1.0f, -1.0f, false, false);
			NpcStatus ns;
			ns.npcUID = npc->UID;
			ns.statusID = MEASURED_BUSH_HIDE_STATUS;
			ns.casterUID = ActorUID::INVALID;
			ns.until = TimeAdd(localTime, TimeMsToTime((f64)STATUS_INFINITE_DURATION_SEC * 1000.0));
			ns.appliedAt = localTime;
			ns.linked = false;
			npcStatuses.push_back(ns);
		}
	}
}

void World::UpdateFog()
{
	if(!replication->titanMode) return;
	UpdateLitCells();
	if(localTime < nextFogUpdate) return;
	nextFogUpdate = TimeAdd(localTime, TimeMsToTime(FOG_UPDATE_MS));
	const auto& content = GetGameXmlContent();
	foreach(p, players) {
		if(!p->body) continue;
		const vec3 at = p->body->GetWorldPos();
		const ActorUID bush = BushAt(at);
		if(bush != p->bushUID) {
			p->bushUID = bush;
			replication->SendBushStateToAll(p->Main().UID, bush);
			if(bush == ActorUID::INVALID) {
				EndPlayerStatus(*p, MEASURED_BUSH_HIDE_STATUS);
				EndPlayerStatus(*p, MEASURED_BUSH_REVEAL_STATUS);
			}
			else if(!p->dead && !PlayerHasActiveStatus(*p, MEASURED_BUSH_REVEAL_STATUS)) { statusNoCaster = true; ApplyStatus(*p, *p, MEASURED_BUSH_HIDE_STATUS, 0, STATUS_INFINITE_DURATION_SEC); statusNoCaster = false; }
		}
		else if(bush != ActorUID::INVALID && !p->dead && !PlayerHasActiveStatus(*p, MEASURED_BUSH_HIDE_STATUS) && !PlayerHasActiveStatus(*p, MEASURED_BUSH_REVEAL_STATUS)) {
			statusNoCaster = true;
			ApplyStatus(*p, *p, MEASURED_BUSH_HIDE_STATUS, 0, STATUS_INFINITE_DURATION_SEC);
			statusNoCaster = false;
		}
	}
	UpdateSummonBushes();
	RefreshBushStates();
	UpdateNpcFog();
	UpdateDynamicFog();
	foreach(p, players) {
		if(p->body && !p->dead && p->hiddenUID != ActorUID::INVALID) p->fogSeenBy = (u8)(p->team == 0 ? 1 : 2);
		if(!p->body || p->dead || p->hiddenUID != ActorUID::INVALID) continue;
		const ActorUID main = p->Main().UID;
		if(p->hiddenFrom != 0 && p->fogHiddenUID != ActorUID::INVALID && p->fogHiddenUID != main) {
			foreach(viewer, players) if(viewer->clientHd != ClientHandle::INVALID && (p->hiddenFrom & (1u << viewer->index))) replication->MarkHidden(viewer->clientHd, main);
			p->fogHiddenUID = main;
		}
		const bool sees[2] = { p->team == 0 || TeamSees(0, *p), p->team == 1 || TeamSees(1, *p) };
		p->fogSeenBy = (u8)((sees[0] ? 1 : 0) | (sees[1] ? 2 : 0));
		foreach(viewer, players) {
			if(viewer->clientHd == ClientHandle::INVALID || viewer->team == p->team) continue;
			const u32 bit = 1u << viewer->index;
			const bool hidden = (p->hiddenFrom & bit) != 0;
			if(!sees[viewer->team] && !hidden) {
				replication->SendLeaveActor(viewer->clientHd, main);
				p->hiddenFrom |= bit;
				p->fogHiddenUID = main;
				LOG("Fog :: %ls hidden for %ls (bush %u)", p->name.data(), viewer->name.data(), (u32)p->bushUID);
			}
			else if(sees[viewer->team] && hidden) {
				const ActorMaster& chara = p->Main();
				const f32 baseSpeed = const_cast<CharacterModel&>(content.GetMaster(chara.classType).character).getMoveSpeed();
				const f32 speed = baseSpeed * (p->battleStage == 2 ? BATTLE_SPEED_RATIO_ACTIVE : (p->battleStage == 1 ? BATTLE_SPEED_RATIO_RECENT : 1.0f));
				replication->SendEnterActor(viewer->clientHd, main, chara.classType, p->body->GetWorldPos(), p->input.rot.bodyYaw, p->input.rot.upperYaw, p->input.moveTo, p->battleStage > 0, speed, chara.actionState != ActionStateID::INVALID ? chara.actionState : ActionStateID::NONE_BEHAVIORSTATE, chara.hp, chara.hpMax);
				const ActorMaster& bench = p->Sub();
				replication->SendHp(viewer->clientHd, bench.UID, bench.hp, bench.hpMax);
				LOG("Fog :: %ls back for %ls (actor %u, action %d)", p->name.data(), viewer->name.data(), (u32)replication->GetLocalActorID(viewer->clientHd, main), (i32)chara.actionState);
				replication->SendPlayerSyncTeleport(viewer->clientHd, main, p->body->GetWorldPos());
				p->hiddenFrom &= ~bit;
			}
		}
	}
}

void World::ShowTagToEnemies(Player& p)
{
	if(!p.body) return;
	const ActorMaster& chara = p.Main();
	const ActorUID oldUID = p.Sub().UID;
	const f32 baseSpeed = const_cast<CharacterModel&>(GetGameXmlContent().GetMaster(chara.classType).character).getMoveSpeed();
	const f32 speed = baseSpeed * (p.battleStage == 2 ? BATTLE_SPEED_RATIO_ACTIVE : (p.battleStage == 1 ? BATTLE_SPEED_RATIO_RECENT : 1.0f));
	foreach_const(viewer, players) {
		if(viewer->clientHd == ClientHandle::INVALID || viewer->team == p.team || (p.hiddenFrom & (1u << viewer->index))) continue;
		if(replication->GetLocalActorID(viewer->clientHd, chara.UID) == LocalActorID::INVALID) continue;
		replication->SendPlayerTag(viewer->clientHd, chara.UID, oldUID);
		replication->SendLeaveActor(viewer->clientHd, oldUID);
		replication->SendEnterActor(viewer->clientHd, chara.UID, chara.classType, p.body->GetWorldPos(), p.input.rot.bodyYaw, p.input.rot.upperYaw, p.input.moveTo, p.battleStage > 0, speed, ActionStateID::TAG_IN_EXECUTE_BEHAVIORSTATE, chara.hp, chara.hpMax);
	}
}

void World::UpdateNpcFog()
{
	foreach(npc, actorNpcList) {
		if(npc->hp <= 0 || npc->removeAt != Time::ZERO || npc->wall) continue;
		const bool summon = npc->ownerUID != ActorUID::INVALID;
		if(!summon && (!npc->monsterAi || IsStructure(*npc))) continue;
		const i32 npcTeam = summon ? (i32)players[npc->ownerIndex].team : (npc->faction == Faction::RED ? 0 : npc->faction == Faction::BLUE ? 1 : -1);
		bool sees[2];
		for(i32 team = 0; team < 2; team++) sees[team] = team == npcTeam || TeamSeesAt(team, npc->pos, ActorUID::INVALID);
		foreach(viewer, players) {
			if(viewer->clientHd == ClientHandle::INVALID) continue;
			const u32 bit = 1u << viewer->index;
			const bool hidden = (npc->fogHiddenFrom & bit) != 0;
			if(!sees[viewer->team] && !hidden && !viewer->devVision) {
				replication->SendLeaveActor(viewer->clientHd, npc->UID);
				npc->fogHiddenFrom |= bit;
			}
			else if((sees[viewer->team] || viewer->devVision) && hidden) {
				const f32 speed = npc->monsterAi ? (npc->monsterWalking ? npc->monsterAi->walk : npc->monsterAi->run) * npc->speedMul : 0.0f;
				Replication::EnterGraph graph;
				const bool dashing = npc->dashEnd != Time::ZERO && localTime >= npc->dashStart && npc->dashCurve == World::NPC_DASH_CURVE_LINEAR;
				if(dashing) {
					graph.start = vec3(npc->dashFrom, npc->pos.z);
					graph.end = vec3(npc->dashFrom + npc->dashDir * npc->dashDist, npc->pos.z);
					graph.dist = npc->dashDist;
					graph.originDist = npc->dashOriginDist > 0 ? npc->dashOriginDist : npc->dashDist;
					graph.totalSec = (f32)TimeDiffSec(TimeDiff(npc->dashStart, npc->dashEnd));
					graph.curSec = (f32)TimeDiffSec(TimeDiff(npc->dashStart, localTime));
				}
				const f32 yaw = dashing ? atan2f(npc->dashDir.y, npc->dashDir.x) : npc->rot.z;
				replication->SendEnterNpc(viewer->clientHd, npc->UID, npc->docID, npc->pos, yaw, speed, npc->actionState != ActionStateID::INVALID ? npc->actionState : ActionStateID::NONE_BEHAVIORSTATE, npc->hp, npc->hpMax, (f32)npc->growthLevel, dashing ? &graph : nullptr);
				npc->fogHiddenFrom &= ~bit;
			}
		}
	}
}

void World::StartSkillPoints()
{
	if(!ingameSkillPoints || (map && map->ingameLevels)) return;
	foreach(it, players) it->nextSkillPointAt = TimeAdd(localTime, TimeMsToTime(SKILL_POINT_INTERVAL_MS));
}

void World::GrantSkillPoint(Player& player)
{
	if(!ingameSkillPoints) return;
	player.skillPoints++;
	if(player.clientHd != ClientHandle::INVALID) replication->SendSkillPoints(player.clientHd, player.userID, player.skillPoints);
	LOG("Upgrade :: %ls skill points %d", player.name.data(), player.skillPoints);
}

void World::UpdateSkillPoints()
{
	static const i32 BOT_UPGRADE_ORDER[] = { SKILL_UPGRADE_SLOT_ULTIMATE, 0, 1 };
	static const i32 MEASURED_BOT_UPGRADE_PLAN_WEIGHTS[] = { 69, 53, 16 };
	foreach(it, players) {
		if(it->nextSkillPointAt != Time::ZERO && localTime >= it->nextSkillPointAt) {
			it->nextSkillPointAt = TimeAdd(it->nextSkillPointAt, TimeMsToTime(SKILL_POINT_INTERVAL_MS));
			GrantSkillPoint(*it);
		}
		if(!botUpgrades || (it->clientHd != ClientHandle::INVALID && Config().DevStartLevel <= 0 && Config().DevPilotUpgrades <= 0) || it->skillPoints <= 0 || it->dead) continue;
		if(it->botUpgradePlan < 0) {
			i32 total = 0;
			for(const i32 w : MEASURED_BOT_UPGRADE_PLAN_WEIGHTS) total += w;
			i32 roll = (i32)(RandUint() % (u32)total);
			it->botUpgradePlan = 0;
			while(roll >= MEASURED_BOT_UPGRADE_PLAN_WEIGHTS[it->botUpgradePlan]) roll -= MEASURED_BOT_UPGRADE_PLAN_WEIGHTS[it->botUpgradePlan++];
			it->botUpgradeFocus = (i8)(RandUint() % 2);
			LOG("Upgrade :: %ls plan %d focus slot %d", it->name.data(), (i32)it->botUpgradePlan, (i32)it->botUpgradeFocus);
		}
		for(i32 tries = 0; tries < MEASURED_BOT_SKILL_FIRST_UPGRADES * 3 && it->skillPoints > 0; tries++) {
			const i32 turn = it->botUpgradeTurn++;
			i32 slot = turn < MEASURED_BOT_SKILL_FIRST_UPGRADES ? turn % 2 : BOT_UPGRADE_ORDER[(turn - MEASURED_BOT_SKILL_FIRST_UPGRADES) % ARRAY_COUNT(BOT_UPGRADE_ORDER)];
			if(it->botUpgradePlan == 1) {
				const i32 stage = turn / MEASURED_BOT_SKILL_FIRST_UPGRADES;
				slot = stage == 0 ? it->botUpgradeFocus : stage == 1 ? SKILL_UPGRADE_SLOT_ULTIMATE : stage == 2 ? 1 - it->botUpgradeFocus : BOT_UPGRADE_ORDER[turn % ARRAY_COUNT(BOT_UPGRADE_ORDER)];
			}
			else if(it->botUpgradePlan == 2) slot = BOT_UPGRADE_ORDER[turn % ARRAY_COUNT(BOT_UPGRADE_ORDER)];
			if(UpgradeSkillSlot(*it, slot)) break;
		}
	}
}

static SkillID SlotSkill(const World::Player& player, u32 masterIndex, i32 slot)
{
	if(slot == SKILL_UPGRADE_SLOT_ULTIMATE) {
		foreach_const(sid, GetGameXmlContent().GetMaster(player.characters[masterIndex]->classType).skillIDs) {
			if((i32)*sid % 100 == ULTIMATE_SKILL_ID_SUFFIX) return *sid;
		}
		return SkillID::INVALID;
	}
	if(slot < 0 || slot > 1) return SkillID::INVALID;
	return player.skills[masterIndex * 2 + slot];
}

static void AcquiredProps(const World::Player& player, SkillID skill, eastl::fixed_vector<i32,8,false>& out)
{
	const auto* ups = GetGameXmlContent().FindSkillUpgrades((i32)skill);
	auto f = player.skillLevels.find((i32)skill);
	if(!ups || f == player.skillLevels.end()) return;
	for(u8 i = 0; i < f->second && i < ups->size(); i++) out.push_back((*ups)[i].propertyID);
}

void World::SendSkillLevel(const Player& player, u32 masterIndex, SkillID skill)
{
	eastl::fixed_vector<i32,8,false> props;
	AcquiredProps(player, skill, props);
	replication->SendPlayerSkillUpdateToAll(player.characters[masterIndex]->UID, skill, masterIndex == player.mainCharaID, Slice<const i32>(props.data(), props.size()));
}

void World::ResetSkillPoints(Player& player)
{
	const i32 slots[] = { 0, 1, SKILL_UPGRADE_SLOT_ULTIMATE };
	for(u32 m = 0; m < 2; m++) {
		for(const i32 slot : slots) {
			const SkillID skill = SlotSkill(player, m, slot);
			if(skill == SkillID::INVALID) continue;
			replication->SendPlayerSkillUpdateToAll(player.characters[m]->UID, skill, m == player.mainCharaID, Slice<const i32>());
		}
	}
	player.skillPoints += player.skillPointsSpent;
	player.skillPointsSpent = 0;
	player.skillLevels.clear();
	if(player.clientHd != ClientHandle::INVALID) replication->SendSkillPoints(player.clientHd, player.userID, player.skillPoints);
	LOG("Upgrade :: %ls skill points reset to %d", player.name.data(), player.skillPoints);
}

bool World::UpgradeSkillSlot(Player& player, i32 slot)
{
	if(player.skillPoints <= 0) return false;
	const u32 active = player.mainCharaID;
	bool upgraded = false;
	for(u32 m = 0; m < 2; m++) {
		const SkillID skill = SlotSkill(player, m, slot);
		const auto* ups = skill != SkillID::INVALID ? GetGameXmlContent().FindSkillUpgrades((i32)skill) : nullptr;
		if(!ups) continue;
		u8& level = player.skillLevels[(i32)skill];
		if(level >= ups->size()) continue;
		level++;
		upgraded = true;
		SendSkillLevel(player, m, skill);
		LOG("Upgrade :: %ls slot %d skill %d level %d (property %d)", player.name.data(), slot, (i32)skill, (i32)level, (*ups)[level - 1].propertyID);
	}
	if(!upgraded) return false;
	player.skillPoints--;
	player.skillPointsSpent++;
	if(player.clientHd != ClientHandle::INVALID) {
		eastl::fixed_vector<i32,8,false> mainProps, subProps;
		const SkillID mainSkill = SlotSkill(player, active, slot);
		const SkillID subSkill = SlotSkill(player, active ^ 1, slot);
		AcquiredProps(player, mainSkill, mainProps);
		AcquiredProps(player, subSkill, subProps);
		replication->SendSkillUpgradeResult(player.clientHd, 0, player.characters[active]->UID, mainSkill, Slice<const i32>(mainProps.data(), mainProps.size()),
			player.characters[active ^ 1]->UID, subSkill, Slice<const i32>(subProps.data(), subProps.size()));
		replication->SendSkillPoints(player.clientHd, player.userID, player.skillPoints);
	}
	return true;
}

const GameXmlContent::SkillUpgrade* World::LatestUpgrade(const Player& player, i32 skillID, GameXmlContent::SkillUpgrade::Type type) const
{
	auto f = player.skillLevels.find(skillID);
	const auto* ups = GetGameXmlContent().FindSkillUpgrades(skillID);
	if(f == player.skillLevels.end() || !ups) return nullptr;
	for(i32 i = MIN((i32)f->second, (i32)ups->size()) - 1; i >= 0; i--) {
		if((*ups)[i].type == type) return &(*ups)[i];
	}
	return nullptr;
}

ActionStateID World::UpgradeAction(const Player& player, i32 skillID) const
{
	auto f = player.skillLevels.find(skillID);
	const auto* ups = GetGameXmlContent().FindSkillUpgrades(skillID);
	if(f == player.skillLevels.end() || !ups) return ActionStateID::INVALID;
	for(i32 i = MIN((i32)f->second, (i32)ups->size()) - 1; i >= 0; i--) {
		if((*ups)[i].action != ActionStateID::INVALID) return (*ups)[i].action;
	}
	return ActionStateID::INVALID;
}

const GameXmlContent::SkillUpgrade* World::LatestUpgradeWith(const Player& player, i32 skillID, bool (*has)(const GameXmlContent::SkillUpgrade&)) const
{
	auto f = player.skillLevels.find(skillID);
	const auto* ups = GetGameXmlContent().FindSkillUpgrades(skillID);
	if(f == player.skillLevels.end() || !ups) return nullptr;
	for(i32 i = MIN((i32)f->second, (i32)ups->size()) - 1; i >= 0; i--) {
		if(has((*ups)[i])) return &(*ups)[i];
	}
	return nullptr;
}

f32 World::ComboPassOffset(const Player& player, SkillID skillID) const
{
	const auto& content = GetGameXmlContent();
	ActionStateID actionID = ActionStateID::INVALID;
	i8 stance = 0;
	Time start = Time::ZERO;
	for(i32 i = (i32)skillProgramList.size() - 1; i >= 0 && start == Time::ZERO; i--) {
		const SkillProgram& pg = skillProgramList[i];
		if(pg.casterUID != player.Main().UID || pg.skillID != skillID) continue;
		actionID = pg.actionID;
		stance = pg.stance;
		start = pg.startTime;
	}
	if(start == Time::ZERO && player.lastCastSkill == skillID) {
		actionID = player.lastCastAction;
		stance = player.lastCastStance;
		start = player.lastCastStart;
	}
	if(start == Time::ZERO) return 0;
	const GameXmlContent::Action* action = content.FindSkillAction(ActionClass(player), actionID, stance);
	if(!action) return 0;
	f32 first = 0;
	foreach_const(st, action->anims) if(st->index == 0) first = content.GetAniSeqTime(ActionClassName(player), st->animationId) * (f32)MAX(st->loop, 1);
	return MAX(first - TimeDiffSec(TimeDiff(start, localTime)), 0.0f);
}

i32 World::UpgradeSummon(const Player& player, i32 skillID) const
{
	auto f = player.skillLevels.find(skillID);
	const auto* ups = GetGameXmlContent().FindSkillUpgrades(skillID);
	if(f == player.skillLevels.end() || !ups) return 0;
	for(i32 i = MIN((i32)f->second, (i32)ups->size()) - 1; i >= 0; i--) {
		if((*ups)[i].summon != 0) return (*ups)[i].summon;
	}
	return 0;
}

bool World::StatusConditionMet(const Player& caster, ActorUID targetUID, ActionCommand::StatusCondition condition) const
{
	const ActorMaster* tm = FindMasterActor(targetUID);
	const ActorMaster& cm = caster.Main();
	if(!tm || tm->hpMax <= 0 || cm.hpMax <= 0) return false;
	const bool friendly = tm->parent->team == caster.team;
	const bool bigger = (f32)tm->hp * (f32)cm.hpMax >= (f32)cm.hp * (f32)tm->hpMax;
	switch(condition) {
		case ActionCommand::StatusCondition::FRIENDLY_HPBIGGER: return friendly && bigger;
		case ActionCommand::StatusCondition::FRIENDLY_HPLESS: return friendly && !bigger;
		case ActionCommand::StatusCondition::UNFRIENDLY_HPBIGGER: return !friendly && bigger;
		case ActionCommand::StatusCondition::UNFRIENDLY_HPLESS: return !friendly && !bigger;
		default: return true;
	}
}

i32 World::ComboSkillFor(const Player& player, i32 skillID) const
{
	auto f = player.skillLevels.find(skillID);
	const auto* ups = GetGameXmlContent().FindSkillUpgrades(skillID);
	if(f != player.skillLevels.end() && ups) {
		for(i32 i = MIN((i32)f->second, (i32)ups->size()) - 1; i >= 0; i--) {
			if((*ups)[i].nextCombo != 0) return (*ups)[i].nextCombo;
		}
	}
	return GetGameXmlContent().GetFirstComboSkill(skillID);
}

f32 World::PlayerSkillPower(const Player& player) const
{
	return MasterStat(player.Main().classType, STAT_SKILLPOWER) + StatBonus(player, STAT_SKILLPOWER) + StatusModSum(player, StatusMod::SKILLPOWER);
}

f32 World::SkillMaxDistance(const Player& player, i32 skillID) const
{
	if(const GameXmlContent::SkillUpgrade* up = LatestUpgradeWith(player, skillID, [](const GameXmlContent::SkillUpgrade& u) { return u.targetMaxDistance > 0; })) return up->targetMaxDistance;
	const GameXmlContent::SkillInput* in = GetGameXmlContent().FindSkillInput(skillID);
	return in ? in->maxDistance : 0.0f;
}

f32 World::SkillCoolTime(const Player& player, i32 skillID) const
{
	const GameXmlContent& content = GetGameXmlContent();
	const GameXmlContent::SkillUpgrade* up = LatestUpgradeWith(player, skillID, [](const GameXmlContent::SkillUpgrade& u) { return u.coolTime > 0; });
	const f32 base = up ? up->coolTime : content.GetSkillCoolTime(skillID);
	if(!map || !map->ingameLevels || skillID % 100 == ULTIMATE_SKILL_ID_SUFFIX) return base;
	for(u32 m = 0; m < 2; m++) {
		const ActorMaster& chara = *player.characters[m];
		if(chara.classType == ClassType::NONE) continue;
		foreach_const(sid, content.GetMaster(chara.classType).skillIDs) {
			if((i32)*sid != skillID) continue;
			const f32 discount = content.MasterTraitBonus(chara.classType, chara.traits, STAT_SKILL_COOLTIME_DISCOUNT) + StatusModSum(player, StatusMod::SKILL_COOLTIME_DISCOUNT);
			return base * MAX(1.0f - discount / 100.0f, 0.0f);
		}
	}
	return base;
}

f32 World::PlayerSkillDamage(const Player& player, SkillID skillID) const
{
	const f32 statusPower = StatusModSum(player, StatusMod::SKILLPOWER);
	if(const GameXmlContent::SkillUpgrade* up = LatestUpgrade(player, (i32)skillID, GameXmlContent::SkillUpgrade::Type::DAMAGE)) {
		if(up->baseDamage >= 0) return MAX(up->baseDamage + up->attackMultiplier * PlayerSkillPower(player), 0.0f);
	}
	f32 damage = SkillDamage(skillID);
	const f32 baseMul = GetGameXmlContent().GetSkillBasePowerMul((i32)skillID);
	if(baseMul > 0) return MAX(damage + baseMul * PlayerSkillPower(player), 0.0f);
	auto found = GetGameXmlContent().skillMap.find(skillID);
	const f32 extraPower = statusPower + StatBonus(player, STAT_SKILLPOWER);
	if(extraPower > 0 && found != GetGameXmlContent().skillMap.end()) {
		SkillNormalModel& sm = const_cast<SkillNormalModel&>(found->second);
		damage += extraPower * sm.getSkillNormalLevelByIndex(0)->getAttackMultiplier();
	}
	return damage;
}

f32 World::PlayerSkillConsume(const Player& player, i32 skillID, i32 index) const
{
	const auto* ups = GetGameXmlContent().FindSkillUpgrades(skillID);
	auto f = player.skillLevels.find(skillID);
	if(ups && f != player.skillLevels.end()) {
		for(i32 i = MIN((i32)f->second, (i32)ups->size()) - 1; i >= 0; i--) {
			if((*ups)[i].consume[index] >= 0) return (*ups)[i].consume[index];
		}
	}
	return index == 4 ? GetGameXmlContent().GetSkillConsumeUG(skillID) : GetGameXmlContent().GetSkillConsume(skillID, index);
}

f32 World::PlayerSkillConsumeMP(const Player& player, i32 skillID) const
{
	const auto* ups = GetGameXmlContent().FindSkillUpgrades(skillID);
	auto f = player.skillLevels.find(skillID);
	if(ups && f != player.skillLevels.end()) {
		for(i32 i = MIN((i32)f->second, (i32)ups->size()) - 1; i >= 0; i--) {
			if((*ups)[i].consumeMP >= 0) return (*ups)[i].consumeMP;
		}
	}
	return GetGameXmlContent().GetSkillConsumeMP(skillID);
}

static i32 ReplaceKey(const eastl::pair<i32,i32>& e) { return e.first; }
static i32 ReplaceKey(const GameXmlContent::StatusReplace& e) { return e.statusID; }
static i32 ReplaceKey(const eastl::pair<i32,const Remote*>& e) { return e.first; }

template<typename List>
static void MergeReplace(List& into, const List& from)
{
	foreach_const(r, from) {
		auto it = eastl::find_if(into.begin(), into.end(), [&](const auto& e) { return ReplaceKey(e) == ReplaceKey(*r); });
		if(it != into.end()) *it = *r;
		else if(!into.full()) into.push_back(*r);
	}
}

i32 World::PlayerUpgradedStatus(const Player& player, i32 statusID, f32* duration) const
{
	foreach_const(lv, player.skillLevels) {
		const auto* ups = GetGameXmlContent().FindSkillUpgrades(lv->first);
		if(!ups) continue;
		for(u8 i = 0; i < lv->second && i < ups->size(); i++) {
			foreach_const(sr, (*ups)[i].statusReplace) {
				if(sr->statusID != statusID) continue;
				if(duration && sr->durationSec > 0) *duration = sr->durationSec;
				return sr->replaceID;
			}
		}
	}
	return statusID;
}

const GameXmlContent::CostLevel* World::UpgradeCastMods(Player& player, SkillID skillID, const GameXmlContent::CostLevel* costLevel)
{
	auto f = player.skillLevels.find((i32)skillID);
	const auto* ups = GetGameXmlContent().FindSkillUpgrades((i32)skillID);
	if(f == player.skillLevels.end() || f->second == 0 || !ups) return costLevel;
	const auto* levels = costLevel ? GetGameXmlContent().FindSkillCostLevels((i32)skillID) : nullptr;
	const i32 costIndex = levels ? (i32)(costLevel - levels->data()) : -1;
	GameXmlContent::CostLevel& mods = player.castMods[((u32)skillID << 3) | (u32)(costIndex + 1)];
	mods = costLevel ? *costLevel : GameXmlContent::CostLevel();
	for(u8 i = 0; i < f->second && i < ups->size(); i++) {
		const GameXmlContent::SkillUpgrade& up = (*ups)[i];
		const GameXmlContent::SkillUpgrade::CostLists* cost = costIndex >= 0 && costIndex < (i32)up.costLevelLists.size() ? &up.costLevelLists[costIndex] : nullptr;
		MergeReplace(mods.remoteReplace, cost ? cost->remoteReplace : up.remoteReplace);
		MergeReplace(mods.statusReplace, cost ? cost->statusReplace : up.statusReplace);
		MergeReplace(mods.remoteResize, cost ? cost->remoteResize : up.remoteResize);
	}
	const GameXmlContent::SkillUpgrade* dmg = LatestUpgrade(player, (i32)skillID, GameXmlContent::SkillUpgrade::Type::DAMAGE);
	if(dmg && costIndex >= 0 && costIndex < (i32)dmg->costLevelDamage.size() && dmg->baseDamage > 0 && dmg->costLevelDamage[costIndex].first >= 0) {
		const f32 skillPower = PlayerSkillPower(player);
		const f32 normal = dmg->baseDamage + dmg->attackMultiplier * skillPower;
		const f32 empowered = dmg->costLevelDamage[costIndex].first + dmg->costLevelDamage[costIndex].second * skillPower;
		if(normal > 0) mods.damageRatio = empowered / normal;
	}
	return &mods;
}
