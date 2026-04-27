#include "world.h"
#include "config.h"
#include <mxm/game_content.h>
#include <mxm/hero_stats.h>
#include <tinyxml2.h>
#include <EAStdC/EAString.h>

void World::Init(Replication* replication_)
{
	replication = replication_;
	nextActorUID = 1;

	auto& ctx = PhysContext();
	ctx.CreateScene(&physics);
}

void World::Cleanup()
{
	physics.Destroy();
}

void World::Update(Time localTime_)
{
	ProfileFunction();

	const f64 tdelta = TimeDurationSec(localTime, localTime_);
	localTime = localTime_;
	physics.localTime = localTime;

	if(players.empty()) return;

	vec3 prevPos = players.front().body->GetWorldPos();

	// players: handle input
	foreach(it, players) {
		Player& p = *it;

		PhysicsDynamicBody& body = *p.body;

		p.movement.rot = p.input.rot;

		// tag (with cooldown)
		if(p.input.tag) {
			p.input.tag = 0;

			// Block swap during skill execution or tag cooldown
			bool canSwap = true;
			if(localTime < p.tagCooldownUntil) canSwap = false;
			if(p.body && p.body->lockedMoveUntil > localTime) canSwap = false; // in skill animation
			if(p.isStunned) canSwap = false; // CC'd

			if(canSwap) {
				// Save current hero state before swapping
				p.SaveActiveHero();

				// Swap active hero
				p.mainCharaID ^= 1;

				// Load new hero state from saved values
				ClassType newClass = (p.mainCharaID == 0) ? p.mainClass : p.subClass;
				const HeroStats& newStats = GetHeroStats(newClass);
				p.LoadActiveHero(newStats);

				// Set tag cooldown (official: always 14 seconds)
				f32 cooldownSec = p.tagCooldown > 0 ? p.tagCooldown : 14.0f;
				p.tagCooldownUntil = TimeAddSec(localTime, cooldownSec);
				p.tagCooldownActive = true;

				// 62114 sent in replication frame, right before SN_GamePlayerTag (62112)
			}
		}

		// cast skills (blocked by stun/silence)
		if(p.input.cast.skillID != SkillID::INVALID) {
			const auto& cast = p.input.cast;
			const auto& content = GetGameXmlContent();

			// Check and consume skill cost from XML
			bool canCast = true;
			if(p.isStunned || p.isSilenced || p.inputBlocked.BlocksActions()) canCast = false;
			f32 resourceCost = 0;
			f32 staminaCost = 0;
			f32 hpCost = 0;
			f32 coolTime = 0;
			bool isShirk = false;
			auto skillIt = content.skillMap.find(cast.skillID);
			if(skillIt != content.skillMap.end()) {
				isShirk = (skillIt->second.type == SkillType::SHIRK);
				SkillNormalLevelModel* lvl = const_cast<SkillNormalModel&>(skillIt->second).getSkillNormalLevelByIndex(0);
				if(lvl) {
					// Use the correct cost based on hero's resource type
					if(p.resourceStatType == 17) // ENERGY
						resourceCost = lvl->getConsumeEP();
					else if(p.resourceStatType == 36) // RAGE
						resourceCost = lvl->getConsumeRP();
					else
						resourceCost = lvl->getConsumeMP(); // MANA

					staminaCost = lvl->getConsumeStamina();
					hpCost = lvl->getConsumeHP();
					coolTime = lvl->getCoolTime();
				}
				if(resourceCost > 0 && p.mana < resourceCost) canCast = false;

				// Check UG cost (ultimates)
				f32 ugCost = skillIt->second.consumeUG;
				if(ugCost > 0 && p.ultimateGauge < ugCost) canCast = false;

				// Check stamina cost (dodge/shirk uses stamina)
				if(staminaCost > 0 && p.stamina < staminaCost) canCast = false;

				// Check HP cost (some skills like Deathknight's consume HP)
				if(hpCost > 0 && p.hp <= hpCost) canCast = false;

				// Check dodge cooldown (SHIRK skills have per-hero cooldown)
				if(isShirk && p.tDodgeCooldownUntil > localTime) canCast = false;

				// Also block shirk if movement is blocked by CC
				if(isShirk && (p.isRooted || p.inputBlocked.movingShirk || p.inputBlocked.standingShirk)) canCast = false;

				if(canCast) {
					// Consume resource cost
					if(resourceCost > 0) {
						p.mana -= resourceCost;
						if(p.mana < 0) p.mana = 0;
						replication->SendUpdateStatToAll(p.Main().UID, p.resourceStatType, (f32)p.maxMana, (f32)p.mana);
					}

					// Consume stamina cost
					if(staminaCost > 0) {
						p.stamina -= staminaCost;
						if(p.stamina < 0) p.stamina = 0;
						replication->SendUpdateStatToAll(p.Main().UID, 64, p.maxStamina, p.stamina);
					}

					// Consume HP cost
					if(hpCost > 0) {
						p.hp -= hpCost;
						if(p.hp < 1) p.hp = 1; // don't let HP cost kill you
						replication->SendUpdateStatToAll(p.Main().UID, 0, p.maxHp, p.hp);
					}

					// Consume UG cost (ultimates)
					if(ugCost > 0) {
						p.ultimateGauge -= ugCost;
						if(p.ultimateGauge < 0) p.ultimateGauge = 0;
						replication->SendUpdateStatToAll(p.Main().UID, 37, p.maxUltimateGauge, p.ultimateGauge);
						LOG("[SKILL] UG consumed: %.0f (remaining: %.0f/%.0f)", ugCost, p.ultimateGauge, p.maxUltimateGauge);
					}

					// Set dodge cooldown
					if(isShirk && coolTime > 0) {
						p.tDodgeCooldownUntil = TimeAddSec(localTime, coolTime);
					}
				}
			}

			if(canCast) {
				PlayerCastSkill(p, cast.skillID, cast.pos, Slice<const ActorUID>(cast.targetList.data(), cast.targetList.size()));
			}
			p.input.cast.skillID = SkillID::INVALID;
		}

		// move (blocked by stun/root CC)
		vec2 delta = vec2(p.input.moveTo - body.GetWorldPos());
		f32 deltaLen = glm::length(delta);
		if(deltaLen > 1.0f && p.input.speed > 0.f && !p.isStunned && !p.isRooted && !p.inputBlocked.BlocksMovement()) {
			vec2 dir = NormalizeSafe(delta);
			p.movement.moveDir = dir;

			// we're close enough that we might miss the point by going full speed in a step, slow down
			if(deltaLen < (p.input.speed * UPDATE_RATE)) {
				p.movement.moveSpeed = f32(deltaLen/UPDATE_RATE);
			}
			else {
				p.movement.moveSpeed = p.input.speed;
			}
		}
		else {
			p.movement.moveDir = vec2(0);
			p.movement.moveSpeed = 0.0f;
		}

		// jump (blocked by stun/root CC)
		p.movement.hasJumped = false;
		if(p.input.jump && !p.isStunned && !p.isRooted && !p.inputBlocked.jump) {
			p.input.jump = 0;
			p.movement.hasJumped = true;
			body.vel.z = GetGlobalTweakableVars().jumpForce;
		}
		else if(p.input.jump) {
			p.input.jump = 0; // consume the input even if blocked
		}

		// apply 2D velocity
		const f32 s = p.movement.moveSpeed;
		body.vel.x = p.movement.moveDir.x * s;
		body.vel.y = p.movement.moveDir.y * s;

		// Titan Ruins (noGravity): lock Z to spawn height, zero Z velocity
		if(physics.noGravity) {
			body.vel.z = 0;
		}
	}

	// execute skill programs
	for(auto it = skillProgramList.begin(); it != skillProgramList.end(); ) {
		if(it->IsDoneExecuting()) {
			// Send SN_CancelSkill to unlock the caster's skill bar
			replication->SendCancelSkill(it->casterUID, (i32)it->skillID);
			it = skillProgramList.erase_unsorted(it);
		}
		else {
			ExecuteSkillProgram(*it);
			++it;
		}
	}

	physics.Step();

	/*
	static f64 accumulatedDiff = 0.0;
	if(players.front().movement.moveDir == vec2(0)) {
		accumulatedDiff = 0.0;
	}

	vec3 deltaPos = players.front().body->pos - prevPos;
	f32 moveDiff = players.front().input.speed * UPDATE_RATE - glm::length(vec2(deltaPos));
	if(moveDiff != 0 && players.front().movement.moveDir != vec2(0)) {
		accumulatedDiff += moveDiff;
		LOG("Move diff = %f  |  accumulatedDiff = %g", moveDiff, accumulatedDiff);
	}
	*/

	Replicate();
}

void World::Replicate()
{
	eastl::fixed_vector<Replication::ActorMaster, 64, false> repMasterList;

	// players
	foreach_const(it, players) {
		const Player& player = *it;

		Replication::Player rep;
		rep.index = player.index;
		rep.userID = player.userID;
		rep.clientHd = player.clientHd;
		rep.name = player.name;
		rep.guildTag = player.guildTag;
		rep.team = player.team;

		rep.mainClass = player.mainClass;
		rep.mainSkin = player.mainSkin;
		rep.subClass = player.subClass;
		rep.subSkin = player.subSkin;

		rep.masters = {
			player.characters[0]->UID,
			player.characters[1]->UID
		};
		rep.mainCharaID = player.mainCharaID;
		rep.hasJumped = player.movement.hasJumped;

		// Combat stats for tag enter packet
		rep.hp = player.hp;
		rep.maxHp = player.maxHp;
		rep.mana = player.mana;
		rep.maxMana = player.maxMana;
		rep.stamina = player.stamina;
		rep.maxStamina = player.maxStamina;
		rep.atk = player.atk;
		rep.defense = player.defense;
		rep.moveSpeed = player.moveSpeed;
		rep.resourceStatType = player.resourceStatType;
		rep.ultimateGauge = player.ultimateGauge;
		rep.maxUltimateGauge = player.maxUltimateGauge;

		replication->FramePushPlayer(rep);

		// main
		{
			const ActorMaster& chara = player.Main();
			Replication::ActorMaster& rch = repMasterList.push_back();
			rch.actorUID = chara.UID;
			rch.clientHd = player.clientHd;
			rch.playerIndex = player.index;
			rch.classType = chara.classType;
			rch.skinIndex = chara.skinIndex;

			rch.pos = player.body->GetWorldPos();
			rch.moveDir = player.movement.moveDir;
			rch.speed = player.input.speed;
			rch.rotation = player.movement.rot;

			rch.actionState = chara.actionState;
			rch.actionParam1 = chara.actionParam1;
			rch.actionParam2 = chara.actionParam2;

			rch.taggedOut = false;
		}

		// sub
		{
			const ActorMaster& chara = player.Sub();
			Replication::ActorMaster& rch = repMasterList.push_back();
			rch.actorUID = chara.UID;
			rch.clientHd = player.clientHd;
			rch.playerIndex = player.index;
			rch.classType = chara.classType;
			rch.skinIndex = chara.skinIndex;

			rch.pos = player.body->GetWorldPos();
			rch.moveDir = player.movement.moveDir;
			rch.speed = player.input.speed;
			rch.rotation = player.movement.rot;

			rch.actionState = chara.actionState;
			rch.actionParam1 = chara.actionParam1;
			rch.actionParam2 = chara.actionParam2;

			rch.taggedOut = true;
		}
	}

	replication->FramePushMasterActors(repMasterList.data(), repMasterList.size());

	// clear action state
	foreach(it, actorMasterList) {
		it->actionState = ActionStateID::INVALID;
		it->actionParam1 = -1;
		it->actionParam2 = -1;
	}

	// npcs
	foreach_const(it, actorNpcList) {
		const ActorNpc& actor = *it;

		Replication::ActorNpc rfl;
		rfl.actorUID = actor.UID;
		rfl.docID = actor.docID;
		rfl.pos = actor.pos;
		rfl.dir = actor.rot;
		rfl.localID = actor.localID;
		rfl.faction = actor.faction;
		rfl.hp = actor.hp;
		rfl.maxHp = actor.maxHp;
		rfl.hasHP = actor.hasHP;

		replication->FramePushNpcActor(rfl);
	}

	// dynamic
	foreach_const(it, actorDynamicList) {
		const ActorDynamic& actor = *it;

		Replication::ActorDynamic rfl;
		rfl.actorUID = actor.UID;
		rfl.docID = actor.docID;
		rfl.pos = actor.pos;
		rfl.rot = actor.rot;
		rfl.localID = actor.localID;
		rfl.faction = actor.faction;
		rfl.action = actor.action;

		replication->FramePushDynamicActor(rfl);
	}
}

World::Player& World::CreatePlayer(const PlayerDescription& desc, const vec3& pos, const RotationHumanoid& rot)
{
	players.emplace_back(players.size(), desc);

	Player& player = players.back();
	player.mainCharaID = 0;
	player.level = 1;
	player.experience = 0;
	player.body = physics.CreateDynamicBody(110, 70, pos); // radius 100 is found in files but 110 (_AILength) matches better

	// clear input
	player.input.moveTo = pos;
	player.input.speed = 0;
	player.input.rot = rot;
	player.input.tag = 0;
	player.input.jump = 0;
	player.input.action = ActionStateID::INVALID;

	const ActorUID mainUID = NewActorUID();
	const ActorUID subUID = NewActorUID();

	actorMasterList.emplace_back(mainUID);
	ActorMaster& main = actorMasterList.back();
	ActorMasterHandle hMain = --actorMasterList.end();
	actorMasterMap.emplace(mainUID, hMain);

	actorMasterList.emplace_back(subUID);
	ActorMaster& sub = actorMasterList.back();
	ActorMasterHandle hSub = --actorMasterList.end();
	actorMasterMap.emplace(subUID, hSub);

	player.characters = {
		hMain,
		hSub
	};

	main.parent = &player;
	main.classType = player.mainClass;
	main.skinIndex = player.mainSkin;

	sub.parent = &player;
	sub.classType = player.subClass;
	sub.skinIndex = player.subSkin;

	// Apply hero stats from XMLs
	{
		const HeroStats& mainStats = GetHeroStats(player.mainClass);
		const HeroStats& subStats = GetHeroStats(player.subClass);
		player.heroStats = &mainStats;

		// Main hero stats
		player.hp = mainStats.HP;
		player.maxHp = mainStats.HP;
		player.atk = mainStats.ATTACK;
		player.defense = mainStats.DEFENSE;
		player.attackRange = mainStats.ATTACK_RANGE;
		player.mana = mainStats.MANA;
		player.maxMana = mainStats.MANA;
		player.manaRegen = mainStats.MANA_PER_SECOND;
		player.stamina = mainStats.STAMINA;
		player.maxStamina = mainStats.STAMINA;
		player.staminaRegen = mainStats.STAMINA_PER_SECOND;
		player.hpRegen = mainStats.HP_REGEN;
		player.critChance = mainStats.CRIT_CHANCE;
		player.critDamage = mainStats.CRIT_DAMAGE;
		player.weaponDamage = mainStats.WEAPON_DAMAGE;
		player.shieldDefense = mainStats.SHIELD_DEFENSE;
		player.lifeSteal = mainStats.LIFE_STEAL;
		player.skillPower = mainStats.SKILL_POWER;
		player.skillCritChance = mainStats.SKILL_CRIT_CHANCE;
		player.skillCritDamage = mainStats.SKILL_CRIT_DAMAGE;
		player.skillAttScaling = mainStats.SKILL_ATT_SCALING;
		player.skillPowerPenetration = mainStats.SKILL_POWER_PENETRATION;
		player.defensePenetration = mainStats.DEFENSE_PENETRATION;
		player.cooldownReduction = mainStats.COOLDOWN_REDUCTION;
		player.attunementScaling = mainStats.ATTUNEMENT_SCALING;
		player.tagCooldown = mainStats.TAG_COOLDOWN;
		player.tagOutHealing = mainStats.TAG_OUT_HEALING;
		player.rclickMoveRatio = mainStats.RCLICK_MOVE_SPEED_RATIO;
		player.rotateSpeed = mainStats.ROTATE_SPEED;
		player.itemPickupRange = mainStats.ITEM_PICKUP_RANGE;
		player.resourceStatType = mainStats.resourceStatType;
		player.moveSpeed = mainStats.BASE_MOVE_SPEED * (mainStats.MOVE_SPEED / 100.0f);

		// Set resource (mana/energy/rage) based on hero's resource type
		if(mainStats.resourceStatType == 17) { // ENERGY
			f32 e = mainStats.ENERGY > 0 ? mainStats.ENERGY : 100; // default 100 if not set
			player.mana = e;
			player.maxMana = e;
		} else if(mainStats.resourceStatType == 36) { // RAGE
			player.mana = 100;
			player.maxMana = 100;
		} else if(mainStats.resourceStatType == 56) { // BUBBLE
			player.mana = 0; // bubble starts empty
			player.maxMana = 100;
		} else if(mainStats.resourceStatType == 0) { // NONE
			player.mana = 0;
			player.maxMana = 0;
		}
		// else: MANA (35) -- already set from mainStats.MANA above

		// Save initial hero states for tag system (main hero)
		player.heroState[0].hp = player.hp;
		player.heroState[0].mana = player.mana;
		player.heroState[0].maxMana = player.maxMana;
		player.heroState[0].stamina = player.stamina;
		player.heroState[0].ultimateGauge = 0;
		player.heroState[0].resourceStatType = mainStats.resourceStatType;

		// Sub hero: compute resource based on sub hero's resource type
		{
			f32 subMana = subStats.MANA;
			f32 subMaxMana = subStats.MANA;
			if(subStats.resourceStatType == 17) { // ENERGY
				subMana = subStats.ENERGY > 0 ? subStats.ENERGY : 100;
				subMaxMana = subStats.ENERGY > 0 ? subStats.ENERGY : 100;
			} else if(subStats.resourceStatType == 36) { // RAGE
				subMana = 100;
				subMaxMana = 100;
			} else if(subStats.resourceStatType == 56) { // BUBBLE
				subMana = 0;
				subMaxMana = 100;
			} else if(subStats.resourceStatType == 0) { // NONE
				subMana = 0;
				subMaxMana = 0;
			}
			player.heroState[1].hp = subStats.HP;
			player.heroState[1].mana = subMana;
			player.heroState[1].maxMana = subMaxMana;
			player.heroState[1].stamina = subStats.STAMINA;
			player.heroState[1].ultimateGauge = 0;
			player.heroState[1].resourceStatType = subStats.resourceStatType;
		}

		player.ultimateGauge = 0;
		player.maxUltimateGauge = 120;
	}

	return player;
}

World::Player* World::FindPlayerByActorUID(ActorUID actorUID)
{
	foreach(it, players) {
		foreach_const(chit, it->characters) {
			if((*chit)->UID == actorUID) return &(*it);
		}
	}
	return nullptr;
}

World::ActorNpc& World::SpawnNpcActor(CreatureIndex docID, i32 localID)
{
	ActorUID actorUID = NewActorUID();

	actorNpcList.emplace_back(actorUID);
	ActorNpc& actor = actorNpcList.back();
	actor.docID = (CreatureIndex)docID;
	actor.localID = localID;

	actorNpcMap.emplace(actorUID, --actorNpcList.end());
	return actor;
}

World::ActorDynamic& World::SpawnDynamic(CreatureIndex docID, i32 localID)
{
	ActorUID actorUID = NewActorUID();

	actorDynamicList.emplace_back(actorUID);
	auto& actor = actorDynamicList.back();
	actor.docID = (CreatureIndex)docID;
	actor.localID = localID;
	actor.faction = Faction::DYNAMIC;
	actor.action = ActionStateID::DYNAMIC_NORMAL_STAND;
	actor.tLastActionChange = localTime;

	actorDynamicMap.emplace(actorUID, --actorDynamicList.end());
	return actor;
}

void World::DestroyDynamic(ActorUID uid)
{
	auto found = actorDynamicMap.find(uid);
	if(found == actorDynamicMap.end()) return;
	actorDynamicList.erase(found->second);
	actorDynamicMap.erase(found);
}

World::Player& World::GetPlayer(u32 playerIndex)
{
	return players[playerIndex];
}

World::ActorMaster* World::FindMasterActor(ActorUID actorUID) const
{
	auto it = actorMasterMap.find(actorUID);
	if(it == actorMasterMap.end()) return nullptr;
	return &(*it->second);
}

World::ActorNpc* World::FindNpcActor(ActorUID actorUID) const
{
	auto it = actorNpcMap.find(actorUID);
	if(it == actorNpcMap.end()) return nullptr;
	return &(*it->second);
}

World::ActorNpc* World::FindNpcActorByCreatureID(CreatureIndex docID)
{
	foreach(it, actorNpcList) {
		if(it->docID == docID) {
			return &(*it);
		}
	}
	return nullptr;
}

ActorUID World::NewActorUID()
{
	return (ActorUID)nextActorUID++;
}

World::ActorMasterHandle World::MasterInvalidHandle()
{
	return actorMasterList.end();
}

void World::PlayerCastSkill(Player& player, SkillID skillID, const vec3& castPos, Slice<const ActorUID> targets)
{
	// TODO: check if can cast

	// access method is kinda convoluted
	const auto& content = GetGameXmlContent();
	auto skillIt = content.skillMap.find(skillID);
	if(skillIt == content.skillMap.end()) {
		LOG("WARNING: Skill %d not found in skillMap", (i32)skillID);
		return;
	}
	const auto& skill = skillIt->second;
	const ActionStateID actionState = skill.action;

	// Determine skill direction from castPos (cursor) or upperYaw (body facing)
	// castPos=(0,0,0) means the client didn't send a target position (dodge/shirk)
	const vec3 casterWorldPos = player.body->GetWorldPos();
	f32 angle;
	bool castPosValid = (castPos.x != 0 || castPos.y != 0 || castPos.z != 0);
	if(castPosValid) {
		vec2 toCastPos = vec2(castPos - casterWorldPos);
		if(glm::length(toCastPos) > 1.0f) {
			vec2 castDir = glm::normalize(toCastPos);
			angle = atan2f(castDir.y, castDir.x);
		} else {
			angle = player.input.rot.upperYaw;
		}
	} else {
		// No castPos — use player's facing direction (follows mouse cursor)
		angle = player.input.rot.upperYaw;
	}
	const vec2 dir = vec2(cosf(angle), sinf(angle));

	vec2 dbgToCast = vec2(castPos - casterWorldPos);
	LOG("[SKILL_AIM] castPos=(%.0f,%.0f,%.0f) casterPos=(%.0f,%.0f,%.0f) len=%.1f angle=%.3f dir=(%.2f,%.2f) upperYaw=%.3f valid=%d",
		castPos.x, castPos.y, castPos.z,
		casterWorldPos.x, casterWorldPos.y, casterWorldPos.z,
		glm::length(dbgToCast), angle, dir.x, dir.y, player.input.rot.upperYaw, castPosValid);
	LOG("[VFX_DIAG] CAST: skillID=%d player=%d castPos=(%.1f,%.1f,%.1f) casterPos=(%.1f,%.1f,%.1f) castAngle=%.4f upperYaw=%.4f angleDeg=%.1f",
		(i32)skillID, player.index,
		castPos.x, castPos.y, castPos.z,
		casterWorldPos.x, casterWorldPos.y, casterWorldPos.z,
		angle, player.input.rot.upperYaw, angle * 180.0f / PI);


	Replication::SkillCast rpCast;
	rpCast.clientHd = player.clientHd;
	rpCast.casterUID = player.Main().UID;
	rpCast.skillID = skillID;
	rpCast.castPos = castPos;
	rpCast.actionID = actionState;

	rpCast.casterPos = casterWorldPos;
	rpCast.casterMoveDir = dir;
	rpCast.casterRot = { angle, 0, angle };
	rpCast.casterSpeed = player.input.speed; // FIXME: should not come from input

	eastl::copy(targets.begin(), targets.end(), eastl::back_inserter(rpCast.targetList));

	replication->FramePushSkillCast(rpCast);


	/*
	 * After the cast, we start executing a "skill program"
	 * It is a set of simple instructions in the world of MxM
	 * Each "program" is described in ActionBase.xml
	 */

	// Trigger new skill execution
	player.Main().actionState = actionState;

	SkillProgram prog;
	prog.skillID = skillID;
	prog.actionID = actionState;
	prog.castPos = castPos;
	prog.castAngle = angle; // aim direction toward cursor for everything
	prog.casterUID = player.Main().UID;
	eastl::copy(targets.begin(), targets.end(), eastl::back_inserter(prog.targetList));
	prog.startTime = localTime;
	prog.commandID = 0;
	skillProgramList.push_back(prog);

	// go through the skill program to find out how much the master moves
	f32 distance = 0;
	f32 moveDuration = 0;

	const auto& action = content.GetSkillAction(player.Main().classType, actionState);

	foreach_const(cmd, action.commands) {
		switch(cmd->type) {
			case ActionCommand::Type::GRAPH_MOVE_HORZ: {
				distance = cmd->graphMoveHorz.distance;
				moveDuration = action.seqLength;
			} break;

			case ActionCommand::Type::MOVE: {
				switch(cmd->move.preset) {
					case ActionCommand::MovePreset::WARP: {
						distance = (f32)cmd->move.param2;
						moveDuration = 0.01f; // warping
					} break;
				}
			} break;
		}
	}

	Replication::SkillExec rpExec;
	rpExec.casterUID = player.Main().UID;
	rpExec.skillID = skillID;
	rpExec.castPos = castPos;
	rpExec.actionID = actionState;
	eastl::copy(targets.begin(), targets.end(), eastl::back_inserter(rpExec.targetList));

	rpExec.moveDuration = moveDuration;
	rpExec.startPos = player.body->GetWorldPos();
	rpExec.moveDir = dir;
	rpExec.rot = { angle, 0, angle };
	rpExec.speed = player.input.speed; // FIXME: should not come from input

	if(distance != 0) {
		const vec3 endPos = physics.FindMovePos(player.body, vec3(dir * distance, 0), moveDuration);
		rpExec.endPos = endPos;
	}

	replication->FramePushSkillExec(rpExec);
}

void World::ExecuteSkillProgram(SkillProgram& prog)
{
	// TODO: each skill is executed following a list of commands from ActionBase.xml
	// process them at runtime for now
	// but since they never change, produce logic code from ActionBase.xml

	ActorMaster* caster = FindMasterActor(prog.casterUID);

	// All directions use castAngle (toward cursor) — dodge, skills, VFX all go toward cursor
	const vec2 dir = vec2(cosf(prog.castAngle), sinf(prog.castAngle));

	// access method is kinda convoluted
	const auto& content = GetGameXmlContent();
	const auto& action = content.GetSkillAction(caster->classType, prog.actionID);

	if(TimeDiffSec(TimeDiff(prog.startTime, localTime)) > action.commands[prog.commandID].relativeEndTimeFromStart) {
		prog.commandID++;

		// program is done
		if(prog.commandID >= action.commands.size()) {
			prog.Finish();
			return;
		}
	}
	else {
		// we have not changed command / instruction, nothing to be done
		return;
	}

	bool running = true;
	while(running) {
		running = false;
		const auto& cmd = action.commands[prog.commandID];

		f32 distance = 0;
		f32 moveDuration = 0;

		switch(cmd.type) {
			case ActionCommand::Type::STATE_BLOCK: {
				// lock WASD input type movement during skill execution
				caster->parent->body->lockedMoveUntil = TimeAddSec(localTime, cmd.delay);
			} break;

			case ActionCommand::Type::GRAPH_MOVE_HORZ: {
				distance = cmd.graphMoveHorz.distance;
				moveDuration = action.seqLength;
			} break;

			case ActionCommand::Type::MOVE: {
				switch(cmd.move.preset) {
					case ActionCommand::MovePreset::WARP: {
						distance = (f32)cmd.move.param2;
						moveDuration = 0.01f; // warping
					} break;
				}
			} break;

			case ActionCommand::Type::REMOTE: {
				// REMOTE command — damage + VFX (gated by EnableSkillVFX config)
				const RemoteIdx remoteIdx = cmd.remote.idx;
				if(remoteIdx != RemoteIdx::INVALID) {
					const Remote& remote = content.GetRemote(remoteIdx);
					const vec3 casterPos = caster->parent->body->GetWorldPos();

					LOG("[SKILL] REMOTE: remoteIdx=%d skillID=%d boundType=%s boundSize=(%d,%d,%d) angle=%d pos=(%.0f,%.0f,%.0f) yaw=%.2f",
						(i32)remoteIdx, (i32)prog.skillID,
						Remote::BoundTypeToString(remote.boundType),
						remote.boundSize[0], remote.boundSize[1], remote.boundSize[2], remote.boundAngle,
						casterPos.x, casterPos.y, casterPos.z, prog.castAngle);

					// VFX packets (62386 + 62237) — format from sv.py serialize_62386
					// Track the seedID for this remote so hit confirmation can reference it
					i32 currentSeedID = 0;
					if(Config().EnableSkillVFX) {
						currentSeedID = (i32)(0xC0000000 | remoteSeedCounter);
						remoteSeedCounter++;

						const ActorUID casterUID = caster->parent->Main().UID;
						// Use real cast position if available, otherwise fallback to direction-based
						const bool hasCastPos = (prog.castPos.x != 0 || prog.castPos.y != 0 || prog.castPos.z != 0);
						const vec3 vfxTargetPos = hasCastPos ? prog.castPos : (casterPos + vec3(dir * 500.0f, 0));

						LOG("[VFX] SEND 62386: docIdx=%d seedID=0x%X parent=%u fireObj=%d pos=(%.0f,%.0f,%.0f) yaw=%.2f",
							(i32)remoteIdx, currentSeedID,
							(u32)casterUID, cmd.remote.fireObjectType,
							casterPos.x, casterPos.y, casterPos.z, prog.castAngle);
						LOG("[VFX_DIAG] REMOTE: remoteIdx=%d seedID=0x%X casterPos=(%.1f,%.1f,%.1f) targetPos=(%.1f,%.1f,%.1f) castPos=(%.1f,%.1f,%.1f) vfxYaw=%.4f vfxYawDeg=%.1f fireObjType=%d hasCastPos=%d",
							(i32)remoteIdx, currentSeedID,
							casterPos.x, casterPos.y, casterPos.z,
							vfxTargetPos.x, vfxTargetPos.y, vfxTargetPos.z,
							prog.castPos.x, prog.castPos.y, prog.castPos.z,
							prog.castAngle, prog.castAngle * 180.0f / PI, cmd.remote.fireObjectType, hasCastPos ? 1 : 0);

						// Use castAngle (saved at cast time) — not current upperYaw which may have changed
						f32 vfxYaw = prog.castAngle;

						// Convert lifeTime from seconds to milliseconds for the packet
						i32 remoteLifeTimeMs = (remote.lifeTime > 0) ? (i32)(remote.lifeTime * 1000) : 0;

						replication->SendRemoteSyncCreateToAll(
							casterUID,
							(i32)remoteIdx,   // RemoteDocIndex (raw document ID)
							currentSeedID,     // RemoteSeedID (0xC0000000 + counter)
							casterPos,         // FirePosition
							vfxYaw,            // FireYaw — player's facing direction (follows mouse)
							vfxTargetPos,      // TargetPosition
							100,               // Scale (100 = 1.0x)
							cmd.remote.fireObjectType, // FireObject type from ActionBase.xml NodeName
							remoteLifeTimeMs   // LifeTime in ms from REMOTE_PC.xml
						);
					}

					// Skip remotes that don't damage enemies (VFX-only or friendly-only)
					if(remote.damageGroup == Remote::DamageGroup::eNONE) {
						break; // no damage — VFX only
					}

					// Look up skill damage — full formula:
					//   rawDamage = BaseDamage + (AttackMultiplier * ATK) + (SkillAttScaling/100 * SkillPower)
					// Defense/crit applied later in Game::ApplyDamage()
					f32 skillDamage = 0;
					f32 skillRangeLengthX = 0;
					f32 skillRangeLengthY = 0;
					f32 skillTargetMaxDist = 0;
					{
						auto skillIt2 = content.skillMap.find(prog.skillID);
						if(skillIt2 != content.skillMap.end()) {
							SkillNormalLevelModel* lvl = const_cast<SkillNormalModel&>(skillIt2->second).getSkillNormalLevelByIndex(0);
							if(lvl) {
								f32 baseDmg = lvl->getBaseDamage();
								f32 atkMult = lvl->getAttackMultiplier();
								f32 atkScaled = atkMult * caster->parent->atk;
								f32 spScaled = (caster->parent->skillAttScaling / 100.0f) * caster->parent->skillPower;
								skillDamage = baseDmg + atkScaled + spScaled;
								skillRangeLengthX = lvl->getSkillRangeLengthX();
								skillRangeLengthY = lvl->getSkillRangeLengthY();
								skillTargetMaxDist = lvl->getTargetMaxDistance();
							}
						}
					}
					if(skillDamage <= 0) skillDamage = caster->parent->atk * 0.5f;

					// ---- Hit detection based on bound type ----
					// Hit range priority: _SkillRangeLengthX (official SKILL.xml) > boundSize > TargetMaxDistance > attackRange
					// SPHERE = AoE centered on castPos (where player aimed)
					// BOX = cone/rectangle from caster in facing direction
					// RAY = line from caster in direction, width = boundSize[1]
					// NONE = VFX-only (already filtered above)

					vec3 hitOrigin;     // center point for hit check
					f32 hitRange;       // max distance from hitOrigin
					f32 coneHalfAngle;
					bool useConeCheck;

					// SPHERE AoE: hit check centered on castPos (where player aimed)
					// BOX/RAY: hit check from casterPos with cone direction
					const bool isSphereAoE = (remote.boundType == Remote::BoundType::E_BOUND_SPHERE);
					const bool hasCastPos2 = (prog.castPos.x != 0 || prog.castPos.y != 0 || prog.castPos.z != 0);
					hitOrigin = (isSphereAoE && hasCastPos2) ? prog.castPos : casterPos;

					// Use _SkillRangeLengthX as primary hit range (this is the actual skill range from SKILL.xml)
					if(skillRangeLengthX > 0) {
						hitRange = skillRangeLengthX;
					} else if(skillTargetMaxDist > 0) {
						hitRange = skillTargetMaxDist;
					} else if(caster->parent->attackRange > 0) {
						hitRange = caster->parent->attackRange;
					} else {
						hitRange = 800.0f;
					}

					// For BOX-type bounds, use boundSize if larger (e.g. Cannon Fist has 960 boundSize)
					if(remote.boundType == Remote::BoundType::E_BOUND_BOX && remote.boundSize[0] > 0) {
						if((f32)remote.boundSize[0] > hitRange) {
							hitRange = (f32)remote.boundSize[0];
						}
					}

					LOG("[SKILL] HIT_RANGE: skillID=%d rangeLenX=%.0f rangeLenY=%.0f targetMaxDist=%.0f boundSize=%d -> hitRange=%.0f",
						(i32)prog.skillID, skillRangeLengthX, skillRangeLengthY, skillTargetMaxDist, remote.boundSize[0], hitRange);

					// Cone angle from bound data, or generous default
					// SPHERE AoE = circular area check (no cone)
					useConeCheck = !isSphereAoE;
					if(remote.boundAngle > 0) {
						coneHalfAngle = (f32)remote.boundAngle * (f32)(PI / 360.0);
					} else {
						coneHalfAngle = 90.0f * (f32)(PI / 360.0); // 90 degree default cone
					}

					i32 hitCount = 0;
					foreach(it, players) {
						Player& target = *it;
						if(target.isDead) continue;
						if(target.team == caster->parent->team) continue;

						vec3 targetPos = target.body->GetWorldPos();
						vec2 toTarget = vec2(targetPos - hitOrigin);
						f32 targetDist = glm::length(toTarget);

						if(targetDist > hitRange) continue;

						// Cone check from caster direction (only for BOX/RAY, not SPHERE AoE)
						if(useConeCheck && targetDist > 1.0f) {
							vec2 dirNorm = NormalizeSafe(toTarget);
							// For BOX/RAY, direction is from caster (even if hitOrigin==casterPos)
							vec2 toTargetFromCaster = vec2(targetPos - casterPos);
							vec2 dirFromCaster = NormalizeSafe(toTargetFromCaster);
							f32 dot = glm::dot(dir, dirFromCaster);
							if(dot < cosf(coneHalfAngle)) continue;
						}

						LOG("[SKILL] HIT: %ls -> %ls damage=%.0f range=%.0f dist=%.0f type=%s",
							caster->parent->name.c_str(), target.name.c_str(),
							skillDamage, hitRange, targetDist,
							Remote::BoundTypeToString(remote.boundType));

						// Send hit VFX at target position (62237)
						if(Config().EnableSkillVFX && currentSeedID != 0) {
							replication->SendRemoteActivatedToAll(
								caster->parent->Main().UID,
								currentSeedID,  // use the seed from the 62386 spawn above
								target.Main().UID, // target being hit
								10,             // penetrationCount (10 = standard from official capture)
								targetPos
							);
						}
						hitCount++;

						// Queue damage for Game to process (handles death, assists, UG, etc)
						PendingSkillDamage pd;
						pd.attackerIndex = caster->parent->index;
						pd.targetIndex = target.index;
						pd.damage = skillDamage;
						pd.skillID = prog.skillID;
						pd.statusID = !remote.hitStatuses.empty() ? remote.hitStatuses[0].statusID : 0;
						pendingSkillDamage.push_back(pd);

						// Apply hit statuses (stun, slow, burn, etc.) from REMOTE _Status entries
						foreach_const(hs, remote.hitStatuses) {
							if(hs->rate >= 100 || (Randf01() * 100.0f) < (f32)hs->rate) {
								PendingStatus ps;
								ps.targetIndex = target.index;
								ps.statusID = hs->statusID;
								ps.casterUID = prog.casterUID;
								ps.isRemove = false;
								pendingStatuses.push_back(ps);
							}
						}
					}

					// Also check NPC targets (spawned dummies, monsters)
					foreach(npcIt, actorNpcList) {
						ActorNpc& npc = *npcIt;
						if(npc.isDead || !npc.hasHP) continue;
						// Check faction — only hit enemy NPCs
						if(npc.faction == Faction(3 + caster->parent->team)) continue;

						vec3 npcPos = npc.pos;
						vec2 toNpc = vec2(npcPos - hitOrigin);
						f32 npcDist = glm::length(toNpc);
						if(npcDist > hitRange) continue;

						if(useConeCheck && npcDist > 1.0f) {
							vec2 toNpcFromCaster = vec2(npcPos - casterPos);
							vec2 dirFromCaster = NormalizeSafe(toNpcFromCaster);
							f32 dot = glm::dot(dir, dirFromCaster);
							if(dot < cosf(coneHalfAngle)) continue;
						}

						// Apply damage to NPC
						npc.hp -= skillDamage;
						if(npc.hp < 0) npc.hp = 0;

						LOG("[SKILL] NPC HIT: %ls -> NPC(%d) damage=%.0f range=%.0f dist=%.0f hp=%.0f/%.0f",
							caster->parent->name.c_str(), (i32)npc.docID,
							skillDamage, hitRange, npcDist, npc.hp, npc.maxHp);

						// Send damage broadcast with skill info
						vec3 atkDir = vec3(dir, 0);
						replication->SendBroadcastDamage(
							caster->parent->Main().UID, npc.UID, (i32)skillDamage,
							casterPos, atkDir, npcPos, atkDir, 0, (i32)prog.skillID);

						// Send HP update
						replication->SendUpdateStatToAll(npc.UID, 0, npc.maxHp, npc.hp);

						// Send hit VFX
						if(Config().EnableSkillVFX && currentSeedID != 0) {
							replication->SendRemoteActivatedToAll(
								caster->parent->Main().UID,
								currentSeedID,
								npc.UID,        // NPC target being hit
								10,             // penetrationCount (standard)
								npcPos);
						}
						hitCount++;
					}
				}
			} break;

			case ActionCommand::Type::STATUS:
			case ActionCommand::Type::STATUS_SKILL_TARGET: {
				// STATUS command — apply or remove a status effect
				const i32 statusIndex = cmd.status.statusIndex;
				if(statusIndex != 0) {
					// Resolve target based on TargetPreset
					bool isSelf = false;
					switch(cmd.status.targetPreset) {
						case ActionCommand::TargetPreset::SELF_FFF:
						case ActionCommand::TargetPreset::SELF_FFF_1:
						case ActionCommand::TargetPreset::SELF_TFF:
						case ActionCommand::TargetPreset::SELF_TTF:
						case ActionCommand::TargetPreset::SELF_TTT:
							isSelf = true;
							break;
						default:
							break;
					}

					if(isSelf) {
						// Apply/remove on caster
						PendingStatus ps;
						ps.targetIndex = caster->parent->index;
						ps.statusID = statusIndex;
						ps.casterUID = prog.casterUID;
						ps.isRemove = cmd.status.isRemove != 0;
						pendingStatuses.push_back(ps);
					} else {
						// Apply/remove on all targets in the skill's target list
						foreach(targetIt, prog.targetList) {
							Player* tgt = FindPlayerByActorUID(*targetIt);
							if(tgt) {
								PendingStatus ps;
								ps.targetIndex = tgt->index;
								ps.statusID = statusIndex;
								ps.casterUID = prog.casterUID;
								ps.isRemove = cmd.status.isRemove != 0;
								pendingStatuses.push_back(ps);
							}
						}
						// If no explicit targets, apply to nearest enemy in cone (like REMOTE)
						if(prog.targetList.empty() && !cmd.status.isRemove) {
							const f32 coneHalfAngle = 145.0f * (f32)(PI / 360.0);
							const vec3 casterPos = caster->parent->body->GetWorldPos();
							foreach(it, players) {
								Player& target = *it;
								if(target.isDead) continue;
								if(target.team == caster->parent->team) continue;

								vec3 tPos = target.body->GetWorldPos();
								vec2 toTarget = vec2(tPos - casterPos);
								f32 dist = glm::length(toTarget);
								if(dist > 800.0f) continue;

								vec2 dirNorm = NormalizeSafe(toTarget);
								f32 dot = glm::dot(dir, dirNorm);
								if(dot < cosf(coneHalfAngle)) continue;

								PendingStatus ps;
								ps.targetIndex = target.index;
								ps.statusID = statusIndex;
								ps.casterUID = prog.casterUID;
								ps.isRemove = false;
								pendingStatuses.push_back(ps);
							}
						}
					}

					LOG("[SKILL] STATUS: idx=%d target=%s remove=%d", statusIndex, isSelf ? "SELF" : "TARGET", cmd.status.isRemove);
				}
			} break;
		}

		if(distance != 0) {
			caster->parent->input.moveTo = caster->parent->body->GetWorldPos() + vec3(dir * distance, 0);
			caster->parent->body->vel = vec3(0);
			physics.Move(caster->parent->body, vec3(dir * distance, 0), moveDuration);
		}

		if(cmd.delay == 0) {
			running = true;
			prog.commandID++;

			// program is done
			if(prog.commandID >= action.commands.size()) {
				prog.Finish();
				return;
			}
		}
	}
}

// ============================================================================
// Status Manager — loads STATUS_PC.xml definitions
// ============================================================================

static StatusManager g_StatusManager;

StatusType StatusTypeFromString(const char* str)
{
	if(!str) return StatusType::INVALID;

	struct Entry { const char* name; StatusType type; };
	static const Entry table[] = {
		{"STATUS_TYPE_STUN", StatusType::STUN},
		{"STATUS_TYPE_AIRBORNE", StatusType::AIRBORNE},
		{"STATUS_TYPE_KNOCKDOWN", StatusType::KNOCKDOWN},
		{"STATUS_TYPE_FLY", StatusType::FLY},
		{"STATUS_TYPE_FEAR", StatusType::FEAR},
		{"STATUS_TYPE_DRAGGED", StatusType::DRAGGED},
		{"STATUS_TYPE_DRAGGEDBYFORCE", StatusType::DRAGGEDBYFORCE},
		{"STATUS_TYPE_SHOCKED", StatusType::SHOCKED},
		{"STATUS_TYPE_FREEZE", StatusType::FREEZE},
		{"STATUS_TYPE_SLOW", StatusType::SLOW},
		{"STATUS_TYPE_ROOT", StatusType::ROOT},
		{"STATUS_TYPE_SILENCE", StatusType::SILENCE},
		{"STATUS_TYPE_ACTIONBLOCK", StatusType::ACTIONBLOCK},
		{"STATUS_TYPE_PUSH", StatusType::PUSH},
		{"STATUS_TYPE_FORCEBACK", StatusType::FORCEBACK},
		{"STATUS_TYPE_GODMODE", StatusType::GODMODE},
		{"STATUS_TYPE_SUPERARMOR", StatusType::SUPERARMOR},
		{"STATUS_TYPE_SHIELD", StatusType::SHIELD},
		{"STATUS_TYPE_EVADE", StatusType::EVADE},
		{"STATUS_TYPE_NEARLY_IMMORTAL", StatusType::NEARLY_IMMORTAL},
		{"STATUS_TYPE_SECOND_CHANCE", StatusType::SECOND_CHANCE},
		{"STATUS_TYPE_BURROW", StatusType::BURROW},
		{"STATUS_TYPE_HIDE", StatusType::HIDE},
		{"STATUS_TYPE_STAT", StatusType::STAT},
		{"STATUS_TYPE_AURA", StatusType::AURA},
		{"STATUS_TYPE_TOGGLE", StatusType::TOGGLE},
		{"STATUS_TYPE_FIREDELAY", StatusType::FIREDELAY},
		{"STATUS_TYPE_COOLTIME", StatusType::COOLTIME},
		{"STATUS_TYPE_POISON", StatusType::POISON},
		{"STATUS_TYPE_BURNING", StatusType::BURNING},
		{"STATUS_TYPE_BLEEDING", StatusType::BLEEDING},
		{"STATUS_TYPE_PUREDOT", StatusType::PUREDOT},
		{"STATUS_TYPE_HIT_PLAGUE", StatusType::HIT_PLAGUE},
		{"STATUS_TYPE_HEAL", StatusType::HEAL},
		{"STATUS_TYPE_FLAG", StatusType::FLAG},
		{"STATUS_TYPE_EFFECT", StatusType::EFFECT},
		{"STATUS_TYPE_MOTION", StatusType::MOTION},
		{"STATUS_TYPE_LINK", StatusType::LINK},
		{"STATUS_TYPE_TARGET", StatusType::TARGET},
		{"STATUS_TYPE_CASTER", StatusType::CASTER},
		{"STATUS_TYPE_DISPEL", StatusType::DISPEL},
		{"STATUS_TYPE_SUICIDE", StatusType::SUICIDE},
		{"STATUS_TYPE_COMA", StatusType::COMA},
		{"STATUS_TYPE_ONEKILL", StatusType::ONEKILL},
		{"STATUS_TYPE_ZEROCOOLTIME", StatusType::ZEROCOOLTIME},
		{"STATUS_TYPE_BUSH_HIDE", StatusType::BUSH_HIDE},
		{"STATUS_TYPE_BUSH_REVEAL", StatusType::BUSH_REVEAL},
		{"STATUS_TYPE_FASTRUN", StatusType::FASTRUN},
	};

	for(const auto& e : table) {
		if(EA::StdC::Strcmp(str, e.name) == 0) return e.type;
	}

	// Hero-specific types map to STAT (they are stat modifier types)
	if(EA::StdC::Strstr(str, "STATUS_TYPE_") != nullptr) {
		return StatusType::STAT;
	}

	return StatusType::INVALID;
}

static StatusEffectCategory EffectTypeFromString(const char* str)
{
	if(!str) return StatusEffectCategory::UNKNOWN;
	if(EA::StdC::Strcmp(str, "STATUS_EFFECT_BUFF") == 0) return StatusEffectCategory::BUFF;
	if(EA::StdC::Strcmp(str, "STATUS_EFFECT_DEBUFF") == 0) return StatusEffectCategory::DEBUFF;
	return StatusEffectCategory::UNKNOWN;
}

bool StatusManager::LoadFromXML(const char* filePath)
{
	tinyxml2::XMLDocument doc;
	if(doc.LoadFile(filePath) != tinyxml2::XML_SUCCESS) {
		LOG("[Status] Failed to load %s: %s", filePath, doc.ErrorStr());
		return false;
	}

	tinyxml2::XMLElement* root = doc.FirstChildElement("ENTITY_FILE");
	if(!root) {
		LOG("[Status] No ENTITY_FILE root in %s", filePath);
		return false;
	}

	i32 count = 0;
	for(tinyxml2::XMLElement* info = root->FirstChildElement("INFO"); info; info = info->NextSiblingElement("INFO")) {
		i32 statusID = 0;
		info->QueryAttribute("ID", &statusID);
		if(statusID == 0) continue;

		tinyxml2::XMLElement* common = info->FirstChildElement("ST_COMMONSTATUS");
		if(!common) continue;

		StatusDefinition def;
		def.statusID = statusID;

		const char* typeStr = common->Attribute("_Type");
		def.type = StatusTypeFromString(typeStr);

		const char* effectStr = common->Attribute("_EffectType");
		def.effectType = EffectTypeFromString(effectStr);

		common->QueryAttribute("_DurationTime", &def.durationTimeMs);

		const char* canDispel = common->Attribute("_CanDispel");
		def.canDispel = canDispel && EA::StdC::Strcmp(canDispel, "true") == 0;

		const char* passive = common->Attribute("_Passive");
		def.passive = passive && EA::StdC::Strcmp(passive, "true") == 0;

		const char* reAction = common->Attribute("_ReAction");
		def.reAction = reAction && EA::StdC::Strcmp(reAction, "true") == 0;

		if(common->QueryAttribute("_MaxOverlapCount", &def.maxOverlapCount) != tinyxml2::XML_SUCCESS) {
			def.maxOverlapCount = 1;
		}

		common->QueryAttribute("_DotBaseDamage", &def.dotBaseDamage);
		common->QueryAttribute("_Interval", &def.intervalMs);
		common->QueryAttribute("_DistanceRatio", &def.distanceRatio);
		common->QueryAttribute("_TimeRatio", &def.timeRatio);
		common->QueryAttribute("_HeightRatio", &def.heightRatio);
		common->QueryAttribute("_LyingTime", &def.lyingTime);

		if(common->QueryAttribute("_NextStatusIndex", &def.nextStatusIndex) != tinyxml2::XML_SUCCESS) {
			def.nextStatusIndex = 0;
		}

		// Parse input blocks
		def.inputBlock.Clear();
		tinyxml2::XMLElement* inputBlock = common->FirstChildElement("_INPUT_STATE_BLOCK");
		if(inputBlock) {
			i32 v = 0;
			if(inputBlock->QueryAttribute("_Attack", &v) == tinyxml2::XML_SUCCESS) def.inputBlock.attack = v ? 1 : 0;
			if(inputBlock->QueryAttribute("_Skill", &v) == tinyxml2::XML_SUCCESS) def.inputBlock.skill = v ? 1 : 0;
			if(inputBlock->QueryAttribute("_Move", &v) == tinyxml2::XML_SUCCESS) def.inputBlock.move = v ? 1 : 0;
			if(inputBlock->QueryAttribute("_Jump", &v) == tinyxml2::XML_SUCCESS) def.inputBlock.jump = v ? 1 : 0;
			if(inputBlock->QueryAttribute("_MovingShirk", &v) == tinyxml2::XML_SUCCESS) def.inputBlock.movingShirk = v ? 1 : 0;
			if(inputBlock->QueryAttribute("_StandingShirk", &v) == tinyxml2::XML_SUCCESS) def.inputBlock.standingShirk = v ? 1 : 0;
			if(inputBlock->QueryAttribute("_HideSkill", &v) == tinyxml2::XML_SUCCESS) def.inputBlock.hideSkill = v ? 1 : 0;
			if(inputBlock->QueryAttribute("_MoveSkill", &v) == tinyxml2::XML_SUCCESS) def.inputBlock.moveSkill = v ? 1 : 0;
		}
		else {
			// Default input blocks by type for hard CC
			switch(def.type) {
				case StatusType::STUN:
				case StatusType::AIRBORNE:
				case StatusType::KNOCKDOWN:
				case StatusType::FLY:
				case StatusType::FEAR:
				case StatusType::SHOCKED:
				case StatusType::FREEZE:
				case StatusType::DRAGGED:
				case StatusType::DRAGGEDBYFORCE:
					def.inputBlock.attack = 1;
					def.inputBlock.skill = 1;
					def.inputBlock.move = 1;
					def.inputBlock.jump = 1;
					def.inputBlock.movingShirk = 1;
					def.inputBlock.standingShirk = 1;
					break;
				case StatusType::ROOT:
					def.inputBlock.move = 1;
					def.inputBlock.jump = 1;
					def.inputBlock.movingShirk = 1;
					break;
				case StatusType::SILENCE:
					def.inputBlock.skill = 1;
					def.inputBlock.movingShirk = 1;
					def.inputBlock.standingShirk = 1;
					break;
				default:
					break;
			}
		}

		definitions.emplace(statusID, def);

		// Build skill→status reverse mapping (first status per skill wins)
		i32 skillIdx = 0;
		common->QueryAttribute("_SkillIndex", &skillIdx);
		if(skillIdx != 0 && skillToStatus.find(skillIdx) == skillToStatus.end()) {
			skillToStatus.emplace(skillIdx, statusID);
		}

		count++;
	}

	LOG("[Status] Loaded %d status definitions, %d skill→status mappings", count, (i32)skillToStatus.size());
	return true;
}

// Build fallback skill→status mappings for skills that lack _SkillIndex in STATUS_PC.xml.
// Strategy:
//   1. Primary: _SkillIndex from STATUS_PC.xml (already built in LoadFromXML)
//   2. Base skill: combo variant 18HHH00XY → base skill 18HHH00X0
//   3. Hero prefix: find first status 123HHH* for the same hero
void StatusManager::BuildFallbackMappings()
{
	const auto& content = GetGameXmlContent();
	i32 before = (i32)skillToStatus.size();

	// Build hero prefix → first status ID map from loaded definitions
	// Status IDs are 123HHH#### where HHH is the 3-digit hero code
	eastl::fixed_hash_map<i32, i32, 64, 64, true> heroFirstStatus; // heroPrefix → first statusID
	for(auto it = definitions.begin(); it != definitions.end(); ++it) {
		i32 statusID = it->first;
		// Hero statuses are 123HHHXXXX (10 digits, hero code at positions 3-5)
		// Skip generic/system statuses (hero code 000)
		if(statusID < 1230010000 || statusID >= 1230500000) continue;
		i32 heroPrefix = (statusID / 10000) % 1000; // extract HHH from 123HHH####
		if(heroPrefix == 0) continue;
		if(heroFirstStatus.find(heroPrefix) == heroFirstStatus.end()) {
			heroFirstStatus.emplace(heroPrefix, statusID);
		}
	}

	// Iterate all skills and fill gaps
	for(auto it = content.skillMap.begin(); it != content.skillMap.end(); ++it) {
		i32 skillID = (i32)it->first;
		if(skillID < 180010000 || skillID >= 180500000) continue; // hero skills only
		if(skillToStatus.find(skillID) != skillToStatus.end()) continue; // already mapped

		i32 heroPrefix = (skillID / 10000) % 1000; // extract HHH from 18HHH####
		i32 skillSuffix = skillID % 10000;          // last 4 digits

		// Tier 2: Try base skill (strip combo digit: 00XY → 00X0)
		// Only for skill slots 1+ (suffix >= 10). Suffixes 0-9 are standalone (passive, dodge).
		if(skillSuffix >= 10 && (skillSuffix % 10) != 0) {
			i32 baseSkillID = skillID - (skillSuffix % 10);
			auto base = skillToStatus.find(baseSkillID);
			if(base != skillToStatus.end()) {
				skillToStatus.emplace(skillID, base->second);
				continue;
			}
		}

		// Tier 3: Try converting skillID prefix 18→123 and check if that status exists
		// Skill: 18HHHSSSS → Status: 123HHHSSSS
		// But status suffix often differs, so try a range: 123HHH0SS0 through 123HHH0SS9
		i32 statusBase = 1230000000 + heroPrefix * 10000 + skillSuffix;
		if(definitions.find(statusBase) != definitions.end()) {
			skillToStatus.emplace(skillID, statusBase);
			continue;
		}
		// Try nearby: statusBase+1, +2, ... +9 (status variants for same skill slot)
		bool found = false;
		for(i32 d = 1; d <= 9; d++) {
			if(definitions.find(statusBase + d) != definitions.end()) {
				skillToStatus.emplace(skillID, statusBase + d);
				found = true;
				break;
			}
		}
		if(found) continue;

		// Tier 4: Use first status for this hero (at least shows something)
		auto heroIt = heroFirstStatus.find(heroPrefix);
		if(heroIt != heroFirstStatus.end()) {
			skillToStatus.emplace(skillID, heroIt->second);
			continue;
		}
	}

	i32 after = (i32)skillToStatus.size();
	LOG("[Status] Fallback mappings: %d new (was %d, now %d total)", after - before, before, after);
}

i32 StatusManager::FindStatusForSkill(i32 skillIndex) const
{
	auto it = skillToStatus.find(skillIndex);
	return (it != skillToStatus.end()) ? it->second : 0;
}

const StatusDefinition* StatusManager::Find(i32 statusID) const
{
	auto it = definitions.find(statusID);
	if(it != definitions.end()) return &it->second;
	return nullptr;
}

const StatusManager& GetStatusManager()
{
	return g_StatusManager;
}

StatusManager& GetStatusManagerMutable()
{
	return g_StatusManager;
}

bool LoadStatusDefinitions()
{
	if(g_StatusManager.LoadFromXML("../gamedata/STATUS_PC.xml")) {
		LOG("[Status] STATUS_PC.xml loaded: %d definitions", (i32)g_StatusManager.definitions.size());
		g_StatusManager.BuildFallbackMappings();
		return true;
	}
	LOG("[Status] STATUS_PC.xml failed to load (non-fatal)");
	return false;
}

// ============================================================================
// Buff/Debuff Lifecycle
// ============================================================================

// Helper: Is this a DoT type?
static bool IsDoTType(StatusType t)
{
	return t == StatusType::POISON || t == StatusType::BURNING ||
		   t == StatusType::BLEEDING || t == StatusType::PUREDOT ||
		   t == StatusType::HIT_PLAGUE;
}

// Helper: Is this a hard CC type?
static bool IsHardCC(StatusType t)
{
	return t == StatusType::STUN || t == StatusType::AIRBORNE ||
		   t == StatusType::KNOCKDOWN || t == StatusType::FLY ||
		   t == StatusType::FEAR || t == StatusType::SHOCKED ||
		   t == StatusType::FREEZE || t == StatusType::DRAGGED ||
		   t == StatusType::DRAGGEDBYFORCE;
}

void World::Player::RecalculateStatusFlags()
{
	// L2-style: declarative flag rebuild from all active statuses
	isStunned = false;
	isRooted = false;
	isSilenced = false;
	isInvulnerable = false;
	hasSuperArmor = false;
	isHidden = false;
	isBurrowed = false;
	slowFactor = 1.0f;
	speedFactor = 1.0f;
	shieldHp = 0;
	inputBlocked.Clear();

	foreach_const(it, activeStatuses) {
		if(!it->def) continue;
		const StatusDefinition& def = *it->def;

		switch(def.type) {
			case StatusType::STUN:
			case StatusType::AIRBORNE:
			case StatusType::KNOCKDOWN:
			case StatusType::FLY:
			case StatusType::SHOCKED:
			case StatusType::FREEZE:
				isStunned = true;
				break;
			case StatusType::FEAR:
			case StatusType::DRAGGED:
			case StatusType::DRAGGEDBYFORCE:
				isStunned = true; // movement controlled by effect, not player
				break;
			case StatusType::ROOT:
				isRooted = true;
				break;
			case StatusType::SILENCE:
			case StatusType::ACTIONBLOCK:
				isSilenced = true;
				break;
			case StatusType::GODMODE:
				isInvulnerable = true;
				break;
			case StatusType::SUPERARMOR:
			case StatusType::EVADE:
				hasSuperArmor = true;
				break;
			case StatusType::HIDE:
				isHidden = true;
				break;
			case StatusType::BURROW:
				isBurrowed = true;
				break;
			case StatusType::SLOW:
				// slowFactor is multiplicative; 50% slow from each source stacks multiplicatively
				slowFactor *= 0.5f;
				break;
			case StatusType::FASTRUN:
				// 30% speed boost (from STATUS_PC.xml description)
				speedFactor *= 1.3f;
				break;
			case StatusType::SHIELD:
				// Shield HP from DoT base damage field (or a fixed amount)
				shieldHp += (def.dotBaseDamage > 0) ? def.dotBaseDamage : 200.0f;
				break;
			default:
				break;
		}

		// Merge input block flags (OR all active)
		if(def.inputBlock.attack) inputBlocked.attack = 1;
		if(def.inputBlock.skill) inputBlocked.skill = 1;
		if(def.inputBlock.move) inputBlocked.move = 1;
		if(def.inputBlock.jump) inputBlocked.jump = 1;
		if(def.inputBlock.movingShirk) inputBlocked.movingShirk = 1;
		if(def.inputBlock.standingShirk) inputBlocked.standingShirk = 1;
		if(def.inputBlock.hideSkill) inputBlocked.hideSkill = 1;
		if(def.inputBlock.moveSkill) inputBlocked.moveSkill = 1;
	}
}

bool World::Player::HasStatus(i32 statusID) const
{
	foreach_const(it, activeStatuses) {
		if(it->statusID == statusID) return true;
	}
	return false;
}

bool World::Player::HasStatusType(StatusType type) const
{
	foreach_const(it, activeStatuses) {
		if(it->def && it->def->type == type) return true;
	}
	return false;
}

i32 World::Player::GetStatusStackCount(i32 statusID) const
{
	foreach_const(it, activeStatuses) {
		if(it->statusID == statusID) return it->overlapCount;
	}
	return 0;
}

void World::AddStatus(Player& target, i32 statusID, ActorUID casterUID, f32 durationOverride)
{
	const StatusManager& sm = GetStatusManager();
	const StatusDefinition* def = sm.Find(statusID);

	if(!def) {
		// Unknown status — still send the packet but don't track internally
		// This allows client VFX to display even for unregistered statuses
		f32 dur = (durationOverride >= 0) ? durationOverride : 3.0f;
		replication->SendAddStatusToAll(statusID, target.Main().UID, casterUID, dur);
		LOG("[STATUS] AddStatus %d to player %d — definition not found, sending packet only", statusID, target.index);
		return;
	}

	// Filter displacement CC (push/knockback) — requires client physics integration
	if(def->type == StatusType::PUSH || def->type == StatusType::FORCEBACK ||
	   def->type == StatusType::DRAGGED || def->type == StatusType::DRAGGEDBYFORCE) {
		LOG("[STATUS] Skipping displacement CC %d on player %d", statusID, target.index);
		return;
	}

	// Check super armor — blocks hard CC
	if(target.hasSuperArmor && IsHardCC(def->type)) {
		LOG("[STATUS] Player %d has super armor, blocking CC %d", target.index, statusID);
		return;
	}

	// Calculate duration
	f32 duration = 0;
	if(durationOverride >= 0) {
		duration = durationOverride;
	} else if(def->durationTimeMs > 0) {
		duration = (f32)def->durationTimeMs / 1000.0f;
	} else if(def->durationTimeMs == -1) {
		duration = 999999.0f; // indefinite
	} else {
		// durationTimeMs == 0: passive or unset
		if(def->passive) {
			duration = 999999.0f;
		} else {
			// Non-passive with 0 duration: use 5s default (prevents permanent debuffs)
			duration = 5.0f;
		}
	}

	// Refresh pattern (L2-style): Remove + Add to prevent visual accumulation
	bool refreshed = false;
	for(i32 i = 0; i < (i32)target.activeStatuses.size(); i++) {
		if(target.activeStatuses[i].statusID == statusID) {
			// Send remove first
			replication->SendRemoveStatusToAll(statusID, target.Main().UID, target.activeStatuses[i].casterUID);
			target.activeStatuses.erase_unsorted(target.activeStatuses.begin() + i);
			refreshed = true;
			break;
		}
	}

	// Add the new status
	if(target.activeStatuses.size() < MAX_ACTIVE_STATUSES) {
		ActiveStatus& status = target.activeStatuses.push_back();
		status.statusID = statusID;
		status.casterUID = casterUID;
		status.targetUID = target.Main().UID;
		status.durationTime = duration;
		status.elapsedTime = 0;
		status.tickTimer = 0;
		status.overlapCount = 1;
		status.enabled = true;
		status.def = def;
	}

	// Send add packet
	replication->SendAddStatusToAll(statusID, target.Main().UID, casterUID, duration);

	// Recalculate flags
	target.RecalculateStatusFlags();

	// If this is a slow or speed buff, update move speed via SN_ChangeBattleState
	if(def->type == StatusType::SLOW) {
		f32 newSpeed = target.moveSpeed * target.slowFactor;
		replication->SendChangeBattleState(target.Main().UID, true, newSpeed);
	}
	else if(def->type == StatusType::FASTRUN) {
		f32 newSpeed = target.moveSpeed * target.speedFactor;
		replication->SendChangeBattleState(target.Main().UID, false, newSpeed);
	}

	LOG("[STATUS] Added %d to player %d (type=%d dur=%.1fs refresh=%d)",
		statusID, target.index, (i32)def->type, duration, refreshed ? 1 : 0);
}

void World::RemoveStatus(Player& target, i32 statusID, ActorUID casterUID)
{
	bool found = false;
	for(i32 i = 0; i < (i32)target.activeStatuses.size(); i++) {
		if(target.activeStatuses[i].statusID == statusID) {
			const StatusDefinition* def = target.activeStatuses[i].def;

			// Send remove packet
			replication->SendRemoveStatusToAll(statusID, target.Main().UID, target.activeStatuses[i].casterUID);

			target.activeStatuses.erase_unsorted(target.activeStatuses.begin() + i);
			found = true;

			// Recalculate flags after removal
			target.RecalculateStatusFlags();

			// If was a slow or speed buff, restore speed
			if(def && def->type == StatusType::SLOW) {
				f32 newSpeed = target.moveSpeed * target.slowFactor * target.speedFactor;
				replication->SendChangeBattleState(target.Main().UID, target.slowFactor < 1.0f, newSpeed);
			}
			else if(def && def->type == StatusType::FASTRUN) {
				f32 newSpeed = target.moveSpeed * target.slowFactor * target.speedFactor;
				replication->SendChangeBattleState(target.Main().UID, false, newSpeed);
			}

			LOG("[STATUS] Removed %d from player %d", statusID, target.index);
			break;
		}
	}

	if(!found) {
		// Still send the remove packet in case client has it tracked
		replication->SendRemoveStatusToAll(statusID, target.Main().UID, casterUID);
	}
}

void World::RemoveAllStatuses(Player& target)
{
	// Send remove for each active status on BOTH heroes (main + sub)
	foreach(it, target.activeStatuses) {
		replication->SendRemoveStatusToAll(it->statusID, target.Main().UID, it->casterUID);
		replication->SendRemoveStatusToAll(it->statusID, target.Sub().UID, it->casterUID);
	}
	target.activeStatuses.clear();
	target.RecalculateStatusFlags();

	// Restore normal speed
	replication->SendChangeBattleState(target.Main().UID, false, target.moveSpeed);

	LOG("[STATUS] Cleared all statuses from player %d", target.index);
}

void World::TickStatuses(f32 dt)
{
	foreach(player, players) {
		if(player->isDead) continue;

		bool flagsDirty = false;

		for(i32 i = 0; i < (i32)player->activeStatuses.size(); ) {
			ActiveStatus& s = player->activeStatuses[i];
			s.elapsedTime += dt;

			// Check expiration
			if(s.durationTime > 0 && s.durationTime < 999999.0f && s.elapsedTime >= s.durationTime) {
				// Status expired
				replication->SendRemoveStatusToAll(s.statusID, player->Main().UID, s.casterUID);

				const StatusDefinition* def = s.def;

				// Chain to next status if defined
				i32 nextStatus = (def && def->nextStatusIndex != 0) ? def->nextStatusIndex : 0;

				player->activeStatuses.erase_unsorted(player->activeStatuses.begin() + i);
				flagsDirty = true;

				// If was slow or speed buff, restore speed
				if(def && (def->type == StatusType::SLOW || def->type == StatusType::FASTRUN)) {
					player->RecalculateStatusFlags();
					f32 newSpeed = player->moveSpeed * player->slowFactor * player->speedFactor;
					bool inBattle = player->slowFactor < 1.0f;
					replication->SendChangeBattleState(player->Main().UID, inBattle, newSpeed);
					flagsDirty = false; // already recalculated
				}

				// Apply chained status
				if(nextStatus != 0) {
					AddStatus(*player, nextStatus, ActorUID::INVALID);
				}

				continue; // don't increment, we erased
			}

			// DoT tick
			if(s.def && IsDoTType(s.def->type) && s.def->intervalMs > 0) {
				s.tickTimer += dt;
				f32 interval = (f32)s.def->intervalMs / 1000.0f;
				while(s.tickTimer >= interval) {
					s.tickTimer -= interval;

					// Apply DoT damage
					f32 dotDmg = s.def->dotBaseDamage;
					if(dotDmg > 0 && !player->isDead) {
						player->hp -= dotDmg;
						if(player->hp < 0) player->hp = 0;

						// Broadcast damage number (damageType=0 for DoT)
						vec3 pos = player->body->GetWorldPos();
						replication->SendBroadcastDamage(
							s.casterUID, player->Main().UID, (i32)dotDmg,
							pos, vec3(0,0,0), pos, vec3(0,0,0), 0, -1);

						replication->SendUpdateStatToAll(player->Main().UID, 0, player->maxHp, player->hp);

						// Death from DoT queued as pending damage
						if(player->hp <= 0) {
							PendingSkillDamage pd;
							pd.attackerIndex = 0; // DoT source — will be resolved by Game
							pd.targetIndex = player->index;
							pd.damage = 0; // already applied
							pd.skillID = SkillID::INVALID;
							// Find caster for kill credit
							Player* caster = FindPlayerByActorUID(s.casterUID);
							if(caster) pd.attackerIndex = caster->index;
							pendingSkillDamage.push_back(pd);
						}
					}
				}
			}

			// HoT tick
			if(s.def && s.def->type == StatusType::HEAL && s.def->intervalMs > 0) {
				s.tickTimer += dt;
				f32 interval = (f32)s.def->intervalMs / 1000.0f;
				while(s.tickTimer >= interval) {
					s.tickTimer -= interval;

					f32 healAmt = s.def->dotBaseDamage; // heal amount stored in dotBaseDamage field
					if(healAmt > 0 && !player->isDead) {
						player->hp += healAmt;
						if(player->hp > player->maxHp) player->hp = player->maxHp;
						replication->SendUpdateStatToAll(player->Main().UID, 0, player->maxHp, player->hp);
					}
				}
			}

			i++;
		}

		if(flagsDirty) {
			player->RecalculateStatusFlags();
		}
	}
}
