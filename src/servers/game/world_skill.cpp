#include "config.h"
#include "world.h"
#include "world_common.h"
#include <mxm/game_content.h>

static const i32 STAT_STAMINAREGEN = 5;
static const i32 STAT_MANAREGEN = 39;
static const i32 STAT_ULTIMATEREGEN = 42;

void World::UpdateUltimateGauges(f32 delta)
{
	foreach(it, players) {
		if(it->clientHd == ClientHandle::INVALID) continue;
		const ClassType cls = it->Main().classType;

		UpdateWeaponHeat(*it);

		{
			const f32 base = MasterStat(cls, STAT_MOVESPEED);
			const f32 speed = MAX(base * (1.0f + StatusModMulSum(*it, StatusMod::MOVESPEED) / 100.0f), 0.0f);
			if(base > 0 && fabsf(speed - it->moveSpeedSent) > 0.01f) {
				it->moveSpeedSent = speed;
				replication->SendStatToAll(it->Main().UID, STAT_MOVESPEED, speed, speed);
				if(fabsf(speed - base) > 0.01f) LOG("MoveSpeed :: %ls %.0f%%", it->name.data(), speed);
			}
		}

		const f32 ultMax = MAX(MasterStat(cls, STAT_ULTIMATE), 1.0f);
		if(it->ultGauge < 0) it->ultGauge = ultMax;
		it->ultGauge = MIN(it->ultGauge + MasterStat(cls, STAT_ULTIMATEREGEN) * delta, ultMax);

		const f32 staminaMax = MasterStat(cls, STAT_STAMINA);
		if(staminaMax > 0) {
			if(it->stamina < 0) it->stamina = staminaMax;
			it->stamina = MIN(it->stamina + MasterStat(cls, STAT_STAMINAREGEN) * delta, staminaMax);
			if(localTime >= it->tNextStaminaSend) {
				it->tNextStaminaSend = TimeAdd(localTime, TimeMsToTime(250));
				replication->SendStatToAll(it->Main().UID, STAT_STAMINA, it->stamina, staminaMax);
			}
		}

		bool resChanged = false;
		for(int r = 0; r < 3; r++) {
			const f32 resMax = MasterStat(cls, RESOURCES[r].maxStat);
			if(resMax <= 0) continue;
			if(it->res[r] < 0) it->res[r] = RESOURCES[r].startFull ? resMax : 0;
			const ResourceGainRule* charging = it->weaponState == WEAPON_STATE_CHARGE ? FindResourceGainRule(cls, ResourceEvent::CHARGING_PER_SEC) : nullptr;
			const f32 chargingGain = charging && charging->slot == r ? charging->amount : 0.0f;
			it->res[r] = MIN(MAX(it->res[r] + (MasterStat(cls, RESOURCES[r].regenStat) + chargingGain) * delta, 0.0f), resMax);
			if(fabsf(it->res[r] - it->resSent[r]) >= 0.01f) resChanged = true;
		}
		if(localTime >= it->tNextResRefresh) {
			it->tNextResRefresh = TimeAdd(localTime, TimeMsToTime(1000));
			resChanged = true;
			it->moveSpeedSent = -1.0f;
		}
		if(resChanged && localTime >= it->tNextResSend) {
			it->tNextResSend = TimeAdd(localTime, TimeMsToTime(250));
			for(int r = 0; r < 3; r++) {
				const f32 resMax = MasterStat(cls, RESOURCES[r].maxStat);
				if(resMax <= 0) continue;
				it->resSent[r] = it->res[r];
				replication->SendStatToAll(it->Main().UID, RESOURCES[r].maxStat, it->res[r], resMax);
			}
		}

		const f32 manaMax = MasterStat(cls, STAT_MANA);
		if(manaMax > 0) {
			if(it->mana < 0) it->mana = manaMax;
			it->mana = MIN(it->mana + MasterStat(cls, STAT_MANAREGEN) * delta, manaMax);
			if(localTime >= it->tNextManaSend) {
				it->tNextManaSend = TimeAdd(localTime, TimeMsToTime(250));
				replication->SendStatToAll(it->Main().UID, STAT_MANA, it->mana, manaMax);
			}
		}

		{
			const ClassType benchCls = it->Sub().classType;
			const f32 benchManaMax = MasterStat(benchCls, STAT_MANA);
			if(benchManaMax > 0) {
				if(it->benchMana < 0) it->benchMana = benchManaMax;
				it->benchMana = MIN(it->benchMana + MasterStat(benchCls, STAT_MANAREGEN) * delta, benchManaMax);
			}
			for(int r = 0; r < 3; r++) {
				const f32 resMax = MasterStat(benchCls, RESOURCES[r].maxStat);
				if(resMax <= 0) continue;
				if(it->benchRes[r] < 0) it->benchRes[r] = RESOURCES[r].startFull ? resMax : 0;
				it->benchRes[r] = MIN(MAX(it->benchRes[r] + MasterStat(benchCls, RESOURCES[r].regenStat) * delta, 0.0f), resMax);
			}
		}

		if(localTime < it->tNextUltSend) continue;
		it->tNextUltSend = TimeAdd(localTime, TimeMsToTime(1000));
		it->ultGaugeSent = it->ultGauge;
		foreach(c, it->characters) {
			replication->SendStatToAll((*c)->UID, (i32)StatType::STAT_TYPE_ULTIMATE, it->ultGauge, ultMax);
		}
	}
}

bool World::PlayerPaySkillCost(Player& player, i32 skillID)
{
	const auto& content = GetGameXmlContent();
	f32 mp = content.GetSkillConsumeMP(skillID);
	foreach(it, activeStatuses) {
		if(it->targetIndex != player.index || !it->info || it->info->type != StatusType::NOCOST || it->until <= localTime) continue;
		if(eastl::find(it->info->costSkills.begin(), it->info->costSkills.end(), skillID) == it->info->costSkills.end()) continue;
		mp = MAX(mp * (1.0f + it->info->costRatioMP / 100.0f), 0.0f);
		it->until = localTime;
		LOG("Passive :: %ls skill %d mana cost %.0f (status %d used)", player.name.data(), skillID, mp, it->statusID);
		break;
	}
	const f32 ug = content.GetSkillConsumeUG(skillID);
	const f32 st = content.GetSkillConsumeStamina(skillID);
	const f32 resCost[3] = { content.GetSkillConsume(skillID, 0), content.GetSkillConsume(skillID, 1), content.GetSkillConsume(skillID, 2) };
	const f32 hpCost = content.GetSkillConsume(skillID, 3);
	if(mp <= 0 && ug <= 0 && st <= 0 && resCost[0] <= 0 && resCost[1] <= 0 && resCost[2] <= 0 && hpCost <= 0) return true;
	if(player.clientHd == ClientHandle::INVALID) return true;

	bool ok = (mp <= 0 || player.mana < 0 || player.mana >= mp) &&
		(ug <= 0 || player.ultGauge < 0 || player.ultGauge >= ug) &&
		(st <= 0 || player.stamina < 0 || player.stamina >= st) &&
		(hpCost <= 0 || player.Main().hp > hpCost);
	for(int r = 0; r < 3; r++) {
		if(resCost[r] > 0 && player.res[r] >= 0 && player.res[r] < resCost[r]) ok = false;
	}
	if(resCost[0] > 0 || resCost[1] > 0 || resCost[2] > 0 || hpCost > 0) {
		LOG("Cost :: %ls skill=%d energy=%.0f/%.0f rage=%.0f/%.0f charges=%.0f/%.1f hp=%.0f", player.name.data(), skillID,
			resCost[0], player.res[0], resCost[1], player.res[1], resCost[2], player.res[2], hpCost);
	}
	LOG("Cost :: %ls skill=%d mp=%.0f/%.0f ug=%.0f/%.0f stamina=%.0f/%.0f %s", player.name.data(), skillID,
		mp, player.mana, ug, player.ultGauge, st, player.stamina, ok ? "paid" : "REFUSED");
	if(!ok) return false;

	if(mp > 0 && player.mana >= 0) { player.mana -= mp; player.tNextManaSend = Time::ZERO; }
	if(ug > 0 && player.ultGauge >= 0) { player.ultGauge -= ug; player.tNextUltSend = Time::ZERO; }
	if(st > 0 && player.stamina >= 0) { player.stamina -= st; player.tNextStaminaSend = Time::ZERO; }
	for(int r = 0; r < 3; r++) {
		if(resCost[r] > 0 && player.res[r] >= 0) { player.res[r] -= resCost[r]; player.tNextResSend = Time::ZERO; }
	}
	if(hpCost > 0) {
		ActorMaster& chara = player.Main();
		chara.hp = MAX(chara.hp - (i32)hpCost, 1);
		replication->SendHpToAll(chara.UID, chara.hp, chara.hpMax);
	}
	return true;
}

void World::SetStance(ActorMaster& master, i8 stance, f32 duration, i32 weaponFireCount, bool cancelByDefeat)
{
	master.stance = stance;
	master.stanceUntil = stance != 0 && duration > 0 ? TimeAdd(localTime, TimeMsToTime((i64)(duration * 1000.0f))) : Time::ZERO;
	master.stanceShotsLeft = stance != 0 && weaponFireCount > 0 ? weaponFireCount : -1;
	master.stanceCancelByDefeat = stance != 0 && cancelByDefeat;
	replication->SendStanceToAll(master.UID, stance, stance != 0 ? duration : 0.0f);
	LOG("Stance :: %ls stance=%d duration=%.1f weaponFireCount=%d", master.parent ? master.parent->name.data() : L"?", (i32)stance, duration, weaponFireCount);
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
	if(player.toggleSkill != SkillID::INVALID) ToggleOff(player);
	player.toggleSkill = skillID;
	player.toggleCastID = nextCastID++;
	player.toggleNext = TimeAdd(localTime, TimeMsToTime(tg->interval * 1000.0));
	const vec3 pos = player.body->GetWorldPos();
	const f32 yaw = player.input.rot.upperYaw;
	foreach_const(id, tg->remotes) {
		SpawnRemote(player.index, RemoteIdx(*id), pos, vec2(cosf(yaw), sinf(yaw)), ActorUID::INVALID, skillID, player.toggleCastID, (i32)SkillDamage(skillID), true, nullptr, 0, player.clientHd != ClientHandle::INVALID);
	}
	foreach_const(id, tg->statuses) ApplyStatus(player, player, *id);
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
		foreach(it, activeStatuses) {
			if(it->targetIndex != player.index) continue;
			if(eastl::find(tg->statuses.begin(), tg->statuses.end(), it->statusID) != tg->statuses.end()) it->until = localTime;
		}
	}
	LOG("Toggle :: %ls skill=%d off", player.name.data(), (i32)player.toggleSkill);
	replication->SendToggleSkill(player.Main().UID, player.clientHd, player.toggleSkill, false, ActionStateID::INVALID);
	player.toggleSkill = SkillID::INVALID;
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
		if(*pool < tg->value) { ToggleOff(p); continue; }
		*pool -= tg->value;
		p.tNextManaSend = Time::ZERO;
		p.tNextResSend = Time::ZERO;
		p.tNextStaminaSend = Time::ZERO;
	}
}

void World::ComboBaseCooldown(Player& player)
{
	const GameXmlContent::SkillInput* in = GetGameXmlContent().FindSkillInput((i32)player.comboBase);
	if(!in || in->coolTimeID == 0 || in->coolTime <= 0) return;
	const i32 id = in->coolTimeID;
	const u32 ms = (u32)(in->coolTime * 1000.0f);
	replication->SendCooltimeReset(player.clientHd, player.Main().UID, Slice<const i32>(&id, 1), ms, ms);
}

void World::ReleaseCast(Player& player)
{
	const Player::Casting c = player.casting;
	player.casting.skillID = SkillID::INVALID;
	auto f = GetGameXmlContent().skillMap.find(c.skillID);
	if(f == GetGameXmlContent().skillMap.end() || player.dead) return;
	LOG("Cast :: %ls skill=%d release", player.name.data(), (i32)c.skillID);
	GroupTeleport(player, c.skillID, c.castPos);
	if(f->second.action == ActionStateID::INVALID) return;
	PlayerCastSkill(player, c.skillID, c.castPos, Slice<const ActorUID>(&c.target, c.target != ActorUID::INVALID ? 1 : 0), 0, f->second.action);
}

void World::CancelCast(Player& player, const char* reason)
{
	LOG("Cast :: %ls skill=%d cancelled (%s)", player.name.data(), (i32)player.casting.skillID, reason);
	replication->SendCancelSkillToAll(player.Main().UID, player.casting.skillID);
	player.casting.skillID = SkillID::INVALID;
}

void World::UpdateCasts()
{
	foreach(it, players) {
		if(it->comboPending != SkillID::INVALID && it->comboSlot >= 0 && localTime >= it->comboSendAt) {
			replication->SendChangeSkillSlot(it->clientHd, it->Main().UID, it->comboSlot, it->comboPending, it->comboResetCooltime);
			if(it->comboResetCooltime != 0) replication->SendCooltimeReset(it->clientHd, it->Main().UID, Slice<const i32>(&it->comboResetCooltime, 1));
			it->comboPending = SkillID::INVALID;
		}
		if(it->comboAutoAt != Time::ZERO && localTime >= it->comboAutoAt) {
			it->comboAutoAt = Time::ZERO;
			if(it->comboSlot >= 0 && it->comboSkill != SkillID::INVALID && !it->dead) {
				LOG("Combo :: %ls auto-cast %d", it->name.data(), (i32)it->comboSkill);
				PlayerCastSkill(*it, it->comboSkill, it->comboCastPos, Slice<const ActorUID>(), 0);
			}
		}
		if(it->comboSlot >= 0 && localTime >= it->comboUntil) {
			replication->SendChangeSkillSlot(it->clientHd, it->Main().UID, it->comboSlot, it->comboBase, 0);
			ComboBaseCooldown(*it);
			LOG("Combo :: %ls slot %d window ended", it->name.data(), it->comboSlot);
			it->comboSlot = -1;
		}
		if(it->casting.skillID == SkillID::INVALID || !it->body) continue;
		if(it->dead) { CancelCast(*it, "dead"); continue; }
		const bool bot = it->clientHd == ClientHandle::INVALID;
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

bool World::ScheduleSkillRemotes(Player& player, SkillID skillID, const vec3& castPos, Slice<const ActorUID> targets, f32 angle, i32 forceAnim, ActionStateID forcedAction)
{
	const auto& content = GetGameXmlContent();
	auto skillFound = content.skillMap.find(skillID);
	if(skillFound == content.skillMap.end()) return false;
	const auto& skill = skillFound->second;
	const auto& action = content.GetSkillAction(player.Main().classType, forcedAction != ActionStateID::INVALID ? forcedAction : skill.action, player.Main().stance);

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

	{
		SkillNormalModel& skillModel = const_cast<SkillNormalModel&>(skill);
		const f32 consumeUG = skillModel.getSkillNormalLevelByIndex(0)->getConsumeUG();
		(void)consumeUG;
	}

	const u32 castID = nextCastID++;
	i32 damage = (i32)SkillDamage(skillID);
	{
		const f32 skillPower = StatusModSum(player, StatusMod::SKILLPOWER);
		if(skillPower > 0) {
			SkillNormalModel& sm = const_cast<SkillNormalModel&>(skill);
			damage += (i32)(skillPower * sm.getSkillNormalLevelByIndex(0)->getAttackMultiplier());
		}
	}
	const ActorUID currentTarget = targets.size() > 0 ? targets[0] : ActorUID::INVALID;

	bool scheduled = false;
	bool teleportToChild = false;
	eastl::fixed_vector<SkillAnimPass,8,false>& animPasses = lastAnimPasses;
	animPasses.clear();
	if(anim == 0 && forceAnim < 0) {
		const bool hitEventCombo = GetGameXmlContent().FindSkillHitEventRemote((i32)skillID) != 0;
		bool regCombo = false;
		bool acts[8] = {};
		foreach_const(cmd, action.commands) {
			if(cmd->type == ActionCommand::Type::REGCOMBO) regCombo = true;
			bool a = cmd->type == ActionCommand::Type::REMOTE || cmd->type == ActionCommand::Type::REMOTE_SKILL_TARGET ||
				cmd->type == ActionCommand::Type::STATUS || cmd->type == ActionCommand::Type::STATUS_SKILL_TARGET || cmd->type == ActionCommand::Type::ENTITY;
			if(cmd->type == ActionCommand::Type::REMOTE || cmd->type == ActionCommand::Type::REMOTE_SKILL_TARGET) {
				const Remote* r = content.FindRemote(cmd->remote.idx);
				const bool visualOnly = r && r->boundType == Remote::BoundType::E_BOUND_NONE && r->onDestroy.empty() && r->onTime.empty() && r->onActivate.empty() &&
					(r->damageGroup == Remote::DamageGroup::eNONE || r->damageGroup == Remote::DamageGroup::INVALID);
				if(visualOnly) a = false;
			}
			if(cmd->animIndex >= 0 && cmd->animIndex < 8) acts[cmd->animIndex] |= a;
		}
		const char* className = content.GetMaster(player.Main().classType).className.c_str();
		i32 lastActing = -1;
		for(i32 i = 0; i < 8; i++) if(acts[i]) lastActing = i;
		if((!regCombo || !acts[0]) && (!hitEventCombo || !acts[0])) {
			f32 start = 0;
			for(i32 i = 0; i <= lastActing && (!regCombo || i <= 1); i++) {
				if(acts[i] && !animPasses.full()) animPasses.push_back({ i, start });
				i32 animId = 0, loop = 1;
				foreach_const(st, action.anims) if(st->index == i) { animId = st->animationId; loop = MAX(st->loop, 1); }
				start += content.GetAniSeqTime(className, animId) * (f32)loop;
				if(regCombo && acts[i]) break;
			}
		}
	}
	if(animPasses.empty()) animPasses.push_back({ anim, 0.0f });
	anim = animPasses[0].index;

	const i32 passes = (i32)animPasses.size();
	for(i32 pass = 0; pass < passes; pass++) {
	const i32 curAnim = animPasses[pass].index;
	foreach_const(cmd, action.commands) {
		if(cmd->animIndex != curAnim && (cmd->animIndex != -1 || pass > 0)) continue;
		const f32 t = animPasses[pass].start + cmd->delay;
		if((cmd->type == ActionCommand::Type::REMOTE || cmd->type == ActionCommand::Type::REMOTE_SKILL_TARGET) && !pendingRemotes.full()) {
			PendingRemote pr;
			pr.time = TimeAdd(localTime, TimeMsToTime(t * 1000.0));
			pr.idx = cmd->remote.idx;
			pr.costLevel = castCostLevel;
			if(castCostLevel) {
				foreach_const(rr, castCostLevel->remoteReplace) if(rr->first == (i32)pr.idx) pr.idx = RemoteIdx(rr->second);
			}
			pr.preset = (i32)cmd->remote.targetPreset;
			pr.offsetSide = cmd->remote.offsetSide;
			pr.offsetFront = cmd->remote.offsetFront;
			pr.ownerIndex = player.index;
			pr.skillID = skillID;
			pr.castID = castID;
			pr.damage = castCostLevel ? (i32)(damage * castCostLevel->damageRatio) : damage;
			pr.castPos = castPos;
			pr.angle = angle;
			pr.target = cmd->remote.targetPreset == ActionCommand::TargetPreset::MEMORIZED_TFT ? player.memorizedTarget : currentTarget;
			pr.atTarget = cmd->type == ActionCommand::Type::REMOTE_SKILL_TARGET;
			pr.stopAtTarget = cmd->remote.stopAtTarget;
			pr.remove = cmd->remote.remove != 0;
			pr.nodeName = cmd->nodeName;
			pendingRemotes.push_back(pr);
			scheduled = true;
		}
		if(cmd->type == ActionCommand::Type::TELEPORT && cmd->teleport.toLastChild && t <= 0.001f) teleportToChild = true;
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
			ps.ownerIndex = player.index;
			const auto preset = cmd->entity.targetPreset;
			vec3 pos = player.body->GetWorldPos();
			if(preset == ActionCommand::TargetPreset::POS_FFF || preset == ActionCommand::TargetPreset::POS_TFF ||
			   preset == ActionCommand::TargetPreset::POS_TTT_1 || preset == ActionCommand::TargetPreset::POS_TFF_1_800_1000) {
				if(glm::length(vec2(castPos)) > 1.0f) pos = vec3(vec2(castPos), pos.z);
			}
			ps.pos = pos;
			ps.rot = vec3(0, 0, angle);
			if(preset == ActionCommand::TargetPreset::CURRENT_FFF) ps.target = currentTarget;
			pendingSummons.push_back(ps);
			scheduled = true;
		}
		if((cmd->type == ActionCommand::Type::STATUS || cmd->type == ActionCommand::Type::STATUS_SKILL_TARGET) && cmd->status.statusIndex != 0 && !pendingStatuses.full()) {
			PendingStatus ps;
			ps.time = TimeAdd(localTime, TimeMsToTime(t * 1000.0));
			ps.statusID = cmd->status.statusIndex;
			ps.casterIndex = player.index;
			const auto preset = cmd->status.targetPreset;
			const bool self = preset == ActionCommand::TargetPreset::SELF_FFF || preset == ActionCommand::TargetPreset::SELF_FFF_1 ||
				preset == ActionCommand::TargetPreset::SELF_TFF || preset == ActionCommand::TargetPreset::SELF_TTF || preset == ActionCommand::TargetPreset::SELF_TTT;
			const bool lastChild = preset == ActionCommand::TargetPreset::LAST_CHILD_FFF || preset == ActionCommand::TargetPreset::LAST_CHILD_TFF || preset == ActionCommand::TargetPreset::CHILD_INDEX_FFF;
			ps.targetUID = lastChild ? ActorUID::INVALID : self ? player.Main().UID : currentTarget;
			ps.duration = cmd->status.removeAtEnd ? MAX(action.seqLength - t, 0.3f) : -1.0f;
			ps.power = (f32)damage;
			ps.remove = cmd->status.remove != 0;
			if(cmd->type == ActionCommand::Type::STATUS_SKILL_TARGET && targets.size() > 1) {
				foreach_const(tg, targets) {
					if(pendingStatuses.full()) break;
					ps.targetUID = *tg;
					pendingStatuses.push_back(ps);
				}
			}
			else if(ps.targetUID != ActorUID::INVALID) pendingStatuses.push_back(ps);
		}
	}
	}

	if(teleportToChild) {
		const ActorNpc* child = FindNpcActor(player.lastChildUID);
		if(child && localTime < child->despawnAt) {
			const vec3 to = vec3(vec2(child->pos), player.body->GetWorldPos().z);
			LOG("Teleport :: %ls to its last entity %d at (%.0f, %.0f)", player.name.data(), (i32)child->docID, to.x, to.y);
			TeleportPlayer(player, to, player.input.rot.upperYaw);
		}
		else LOG("Teleport :: %ls has no live entity to teleport to", player.name.data());
	}
	LOG("Skill :: %ls skill=%d anim=%d%s scheduled=%d damage=%d castPos=(%.0f, %.0f)", player.name.data(), (i32)skillID, anim, passes > 1 ? "+1" : "", scheduled, damage, castPos.x, castPos.y);
	return scheduled;
}

void World::RunSkillProgramCommand(SkillProgram& prog, ActorMaster* caster, i32 cmdIndex, const vec2& dir)
{
	const auto& content = GetGameXmlContent();
	const auto& action = content.GetSkillAction(caster->classType, prog.actionID, prog.stance);
	const auto& cmd = action.commands[cmdIndex];
	f32 distance = 0;
	f32 moveDuration = 0;
	const GameXmlContent::MoveCurve* curve = nullptr;

	switch(cmd.type) {
		case ActionCommand::Type::STATE_BLOCK: {
			caster->parent->body->lockedMoveUntil = TimeAddSec(localTime, cmd.delay);
		} break
			;
		case ActionCommand::Type::GRAPH_MOVE_HORZ: {
			distance = cmd.graphMoveHorz.distance;
			moveDuration = SkillMoveDuration(content, caster->classType, action, cmd.animIndex);
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
	}

	if(distance != 0) {
		Player& mover = *caster->parent;
		const vec2 from = vec2(mover.body->GetWorldPos());
		vec2 moveDir = distance < 0 ? -dir : dir;
		f32 moveDist = fabsf(distance);
		foreach_const(c2, action.commands) {
			if(c2->type != ActionCommand::Type::MOVE) continue;
			const auto preset = c2->move.preset;
			if((preset == ActionCommand::MovePreset::TARGET_POS || preset == ActionCommand::MovePreset::TARGET_POS_LEAP) && glm::length(vec2(prog.castPos)) > 1.0f) {
				moveDist = glm::length(vec2(prog.castPos) - from);
			}
			const bool gotoTarget = preset == ActionCommand::MovePreset::GOTO_CURRENT_TARGET || preset == ActionCommand::MovePreset::GOTO_CURRENT_TARGET_CHASE ||
				preset == ActionCommand::MovePreset::GOTO_CURRENT_TARGET_THROUGH || preset == ActionCommand::MovePreset::GOTO_CURRENT_TARGET_THROUGH_CHASE;
			if(gotoTarget && !prog.targetList.empty()) {
				const ActorMaster* tm = FindMasterActor(prog.targetList[0]);
				if(tm) {
					const vec2 rel = vec2(tm->parent->body->GetWorldPos()) - from;
					const f32 len = glm::length(rel);
					if(len > 1.0f) moveDir = rel / len;
					const bool through = preset == ActionCommand::MovePreset::GOTO_CURRENT_TARGET_THROUGH || preset == ActionCommand::MovePreset::GOTO_CURRENT_TARGET_THROUGH_CHASE;
					if(!through) moveDist = MIN(moveDist, MAX(len - HitRadius(*tm->parent) - HitRadius(mover), 0.0f));
					else if(moveDist > 999990.0f) moveDist = len;
				}
			}
		}
		if(moveDist > 999990.0f) moveDist = 0;
		mover.input.moveTo = mover.body->GetWorldPos();
		mover.body->vel = vec3(0);
		if(moveDuration <= 0.02f) physics.Move(mover.body, vec3(moveDir * moveDist, 0), moveDuration);
		else {
			mover.dashDir = moveDir;
			mover.dashSpeed = moveDist / moveDuration;
			mover.dashStartAt = localTime;
			mover.dashOrigin = mover.body->GetWorldPos();
			mover.dashDist = moveDist;
			mover.dashDone = 0;
			mover.dashCurve = curve;
			mover.dashUntil = TimeAdd(localTime, TimeMsToTime(moveDuration * 1000.0));
			if(curve) LOG("Dash :: %ls follows move graph over %.2fs for %.0f", mover.name.data(), moveDuration, moveDist);
		}
	}
}

void World::GainResource(Player& player, i32 slot, f32 amount)
{
	const f32 resMax = MasterStat(player.Main().classType, RESOURCES[slot].maxStat);
	if(resMax <= 0) return;
	player.res[slot] = MIN(MAX(player.res[slot], 0.0f) + amount, resMax);
	player.tNextResSend = Time::ZERO;
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
