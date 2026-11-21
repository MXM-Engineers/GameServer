#include "config.h"
#include "world.h"
#include "world_common.h"
#include <mxm/game_content.h>

static const f32 WALL_HIT_SEARCH_RADIUS = 300.0f;
static const f32 REFLECT_SAMPLE_STEP = 25.0f;
static const f64 BOT_ATTACK_STATE_MS = 300.0;
static const f32 WEAPON_PROJECTILE_SPEED = 4000.0f;
static const f32 WEAPON_MELEE_MAX_RANGE = 600.0f;
static const f32 PROJECTILE_TARGET_RADIUS = 70.0f;
static const f32 WEAPON_SHOT_RELIABLE_DISTANCE = 100.0f;
static const f32 WEAPON_SHOT_MAX_DISTANCE = 2500.0f;

static f32 WeaponReach(const Remote& remote, const WeaponFiring& wf)
{
	if(remote.behaviorType == Remote::BehaviorType::GRAPH && wf.maxDistance > 0) return wf.maxDistance;
	if(remote.maxDistance > 0) return remote.maxDistance;
	if(remote.maxSpeed > 0 && remote.lifeTime > 0) return remote.lifeTime * remote.maxSpeed;
	if(remote.boundType == Remote::BoundType::E_BOUND_SPHERE) return remote.offsetY + RemoteBoundOffset(remote) + remote.boundSize[0] * 0.5f;
	if(remote.boundType != Remote::BoundType::E_BOUND_NONE) return remote.offsetY + RemoteBoundOffset(remote) + remote.boundSize[1] * 0.5f;
	return wf.maxDistance;
}

static const Remote* WeaponDelayedHit(const Remote& base)
{
	if(base.boundType != Remote::BoundType::E_BOUND_NONE || base.onTime.empty() || base.onTime[0].idx == RemoteIdx::INVALID) return nullptr;
	return GetGameXmlContent().FindRemote(base.onTime[0].idx);
}

static vec3 ProjectileContact(const World::Projectile& proj, const vec2& target, f32 radius)
{
	const vec2 contact = target - proj.dir * radius;
	return vec3(glm::dot(contact - vec2(proj.start), proj.dir) < 0 ? vec2(proj.start) : contact, proj.pos.z);
}

static ActionStateID WeaponAttackState(const World::Player& player, const WeaponFiring& wf)
{
	return wf.AttackState(player.firingCombo, player.chargeLevel);
}

static void BotWeaponAnimation(const World::Player& player, const WeaponFiring& wf, Replication* replication)
{
	if(player.clientHd != ClientHandle::INVALID) return;
	replication->SendAttackActionToOthers(ClientHandle::INVALID, player.Main().UID, WorldYawToMxmYaw(player.weaponAimYaw), WeaponAttackState(player, wf));
}

void World::PlayerWeaponFire(Player& player)
{
	if(player.dead || frozen) return;
	if(IsBlocked(player, true, false, false)) {
		LOG("Blocked :: %ls melee swing ignored (attack blocked by a status)", player.name.data());
		return;
	}
	BreakHide(player);
	BushRevealOnFire(player);
	const vec2 dir = vec2(cosf(player.weaponAimYaw), sinf(player.weaponAimYaw));
	const vec3 origin = player.body->GetWorldPos();

	if(player.clientHd != ClientHandle::INVALID && player.Main().stance == 0 && player.casting.skillID == SkillID::INVALID) DropPendingActions(player);
	if(player.clientHd != ClientHandle::INVALID && player.casting.skillID != SkillID::INVALID && player.casting.type == GameXmlContent::SkillCast::LIMIT) {
		ReleaseCast(player);
		return;
	}
	if(WeaponIsMelee(player)) {
		const i8 swingStance = player.Main().stance;
		StanceWeaponUse(player);
		const WeaponFiring& wf = ActiveWeaponFiring(player, swingStance);
		if(player.chargeLevel > 0) ApplyResourceGain(player, ResourceEvent::CHARGED_SHOT);
		const RemoteIdx swing = SelectWeaponRemote(player, wf);
		BotWeaponAnimation(player, wf, replication);
		RunWeaponActionStatuses(player, swingStance);
		const Remote* swingRemote = GetGameXmlContent().FindRemote(swing);
		ASSERT(swingRemote);
		LOG("Shot :: %ls melee swing remote %d (stance %d)", player.name.data(), (i32)swing, (i32)swingStance);
		const GameXmlContent::MeasuredSwingNode* node = GetGameXmlContent().FindMeasuredSwingNode(swing);
		const vec2 at = dir * (swingRemote->offsetY + (node ? node->forward : 0)) + Perp(dir) * (node ? node->side : 0);
		SpawnRemote(player.index, swing, origin + vec3(at, 0), dir, ActorUID::INVALID, SkillID::INVALID, nextCastID++, (i32)WeaponHitDamage(player, nullptr), true, nullptr, 0, player.clientHd != ClientHandle::INVALID);
		if(player.clientHd == ClientHandle::INVALID) BotWeaponLunge(player, GetGameXmlContent().GetSkillAction(ActionClass(player), WeaponAttackState(player, wf), swingStance), dir);
		else if(player.chargeLevel > 0) HumanWeaponLunge(player, GetGameXmlContent().GetSkillAction(ActionClass(player), WeaponAttackState(player, wf), swingStance), WeaponAttackState(player, wf), dir);
	}
}

void World::HumanWeaponLunge(Player& player, const GameXmlContent::Action& action, ActionStateID state, const vec2& dir)
{
	foreach_const(cmd, action.commands) {
		if(cmd->type != ActionCommand::Type::GRAPH_MOVE_HORZ || cmd->graphMoveHorz.distance <= 0) continue;
		i32 animID = 0;
		foreach_const(st, action.anims) if(st->index == MAX(cmd->animIndex, 0)) animID = st->animationId;
		const f32 sec = animID != 0 ? GetGameXmlContent().GetAniSeqTime(ActionClassName(player), animID) : action.seqLength;
		if(sec <= 0) return;
		const vec3 from = player.body->GetWorldPos();
		const vec3 end = from + vec3(dir * cmd->graphMoveHorz.distance, 0);
		replication->SendActionStateOnlyToAll(player.Main().UID, state, WorldYawToMxmYaw(atan2f(dir.y, dir.x)), true, from, end, sec);
		LOG("Dash :: %ls charged lunge %.0f over %.2fs sent", player.name.data(), cmd->graphMoveHorz.distance, sec);
		return;
	}
}

void World::BotWeaponLunge(Player& player, const GameXmlContent::Action& action, const vec2& dir)
{
	foreach_const(cmd, action.commands) {
		if(cmd->type != ActionCommand::Type::GRAPH_MOVE_HORZ || cmd->graphMoveHorz.distance <= 0) continue;
		f32 duration = SkillMoveDuration(GetGameXmlContent(), ActionClassName(player), action, cmd->animIndex);
		const GameXmlContent::MoveCurve* curve = nullptr;
		foreach_const(st, action.anims) {
			if(st->index != cmd->animIndex) continue;
			curve = GetGameXmlContent().GetMoveCurve(ActionClassName(player), st->animationId);
			if(curve && curve->time > 0) duration = curve->time;
			break;
		}
		if(duration <= 0.02f) return;
		const vec3 from = player.body->GetWorldPos();
		const vec3 end = physics.FindMovePos(player.body, vec3(dir * cmd->graphMoveHorz.distance, 0), duration);
		player.input.moveTo = from;
		player.body->vel = vec3(0);
		player.dashDir = dir;
		player.dashDist = glm::length(vec2(end) - vec2(from));
		player.dashSpeed = player.dashDist / duration;
		player.dashStartAt = localTime;
		player.dashOrigin = from;
		player.dashDone = 0;
		player.dashCurve = curve;
		player.dashFly = false;
		player.dashAwaitClientStart = false;
		player.dashUntil = TimeAdd(localTime, TimeMsToTime(duration * 1000.0));
		player.dashBlockUntil = player.dashUntil;
		player.weaponLungeUntil = player.dashUntil;
		LOG("Dash :: %ls weapon lunge %.0f over %.2fs%s", player.name.data(), player.dashDist, duration, curve ? " (move graph)" : "");
		return;
	}
}

void World::PlayerWeaponShot(Player& player, const vec3& origin, const vec2& dir, f32 distance)
{
	if(player.dead) return;
	BreakHide(player);
	BushRevealOnFire(player);
	MarkInBattle(player);
	const ClassType classType = player.Main().classType;
	if(WeaponIsMelee(player)) return;
	if(player.chargeLevel > 0) ApplyResourceGain(player, ResourceEvent::CHARGED_SHOT);
	if(player.clientHd != ClientHandle::INVALID && player.casting.skillID != SkillID::INVALID && player.casting.type == GameXmlContent::SkillCast::LIMIT) {
		ReleaseCast(player);
		return;
	}
	const i8 shotStance = player.Main().stance;
	StanceWeaponUse(player);
	if(projectileList.full()) return;

	const auto& content = GetGameXmlContent();
	const WeaponFiring& wf = ActiveWeaponFiring(player, shotStance);
	const f32 aimMax = wf.maxDistance > 0 ? wf.maxDistance : WEAPON_SHOT_MAX_DISTANCE;
	const f32 aimMin = wf.minDistance;
	const bool spawnAtAim = wf.spawnAtAim;
	const RemoteIdx idx = SelectWeaponRemote(player, wf);
	BotWeaponAnimation(player, wf, replication);
	if(wf.firingStatDuration > 0) {
		player.firingStatUntil = TimeAdd(localTime, TimeMsToTime(wf.firingStatDuration * 1000.0));
		player.firingMoveSpeedMul = wf.firingMoveSpeedMul;
	}
	if(player.clientHd == ClientHandle::INVALID) {
		player.botAttackState = WeaponAttackState(player, wf);
		player.botAttackStateSkill = false;
		player.botShotAt = localTime;
		player.botAttackStateUntil = TimeAdd(localTime, TimeMsToTime(BOT_ATTACK_STATE_MS));
	}
	const vec3 shotAim = vec3(vec2(origin) + dir * MAX(MIN(distance > 1.0f ? distance : aimMax, aimMax), aimMin), player.body->GetWorldPos().z);
	RunWeaponActionStatuses(player, shotStance, &shotAim);
	const Remote* remote = content.FindRemote(idx);
	ASSERT(remote);
	LOG("Shot :: %ls firing remote %d (combo %d charge %d count %d)", player.name.data(), (i32)idx, player.firingCombo, player.chargeLevel, player.shotCount[player.mainCharaID & 1]);

	const Remote* bomb = remote ? content.FindRemote(remote->destroyCreateRemote) : nullptr;
	const bool friendChainShot = remote && remote->behaviorType == Remote::BehaviorType::CHAIN && remote->includeParty && !remote->friendlyStatuses.empty();
	if((spawnAtAim || friendChainShot) && remote) {
		f32 reach = friendChainShot ? 0.0f : MAX(MIN(distance > 1.0f ? distance : WeaponReach(*remote, wf), aimMax), aimMin);
		const vec3 aimPos = vec3(vec2(origin) + dir * reach, player.body->GetWorldPos().z);
		const bool bot = player.clientHd == ClientHandle::INVALID;
		if(!bot) replication->SendAttackActionToOthers(player.clientHd, player.Main().UID, WorldYawToMxmYaw(atan2f(dir.y, dir.x)), WeaponAttackState(player, wf));
		u32 clientSeed = 0;
		if(!bot && player.pendingShotRawSize > 0) {
			const u16 creator = replication->RelayWeaponShot(player.clientHd, player.pendingShotRaw.data(), player.pendingShotRawSize, aimPos, remote->ID);
			LOG("WeaponShot :: %ls firing=%d creator=%u doc=%d", player.name.data(), wf.firingMethod, creator, (i32)remote->ID);
			if(player.pendingShotRawSize >= (i32)offsetof(Cl::CQ_GamePlayerSyncPhy, FirePosition)) clientSeed = ((const Cl::CQ_GamePlayerSyncPhy*)player.pendingShotRaw.data())->RemoteSeedID;
			player.pendingShotRawSize = 0;
		}
		RemoteInst* marker = SpawnRemote(player.index, remote->ID, aimPos, dir, ActorUID::INVALID, SkillID::INVALID, nextCastID++, (i32)WeaponHitDamage(player, nullptr), bot);
		if(marker && !bot && clientSeed != 0) {
			marker->clientRemoteID = clientSeed + 1;
			marker->seedOwnerHd = player.clientHd;
		}
		LOG("Shot :: %ls class=%d remote=%d at aimed point (%.0f, %.0f) reach=%.0f", player.name.data(), (i32)classType, (i32)remote->ID, aimPos.x, aimPos.y, reach);
		return;
	}

	const f32 classRange = WeaponReach(*remote, wf);
	const bool reliable = distance >= WEAPON_SHOT_RELIABLE_DISTANCE;
	const bool graph = remote && remote->behaviorType == Remote::BehaviorType::GRAPH;

	Projectile proj;
	proj.ownerIndex = player.index;
	proj.pos = reliable ? vec3(vec2(origin), player.body->GetWorldPos().z) : player.body->GetWorldPos();
	proj.dir = reliable ? dir : vec2(cosf(player.weaponAimYaw), sinf(player.weaponAimYaw));
	proj.travelled = 0;
	proj.start = proj.pos;
	proj.speed = (remote && remote->maxSpeed > 0) ? remote->maxSpeed : WEAPON_PROJECTILE_SPEED;

	f32 remoteRange = classRange;
	if(remote && remote->maxDistance > 0) remoteRange = remote->maxDistance;
	else if(remote && remote->lifeTime > 0) remoteRange = remote->lifeTime * proj.speed;

	proj.range = graph ? MAX(reliable ? MIN(distance, aimMax) : MIN(classRange, aimMax), aimMin) : remoteRange;
	if(remote && remote->useOwnerRemoteSpeedRatio) proj.speed *= OwnerRemoteSpeedRatio(player);
	proj.graph = graph;
	proj.ignoreWalls = !remote || remote->ignoreGround;
	proj.wallZ = player.body->GetWorldPos().z + BodyMidHeight(player);
	proj.halfWidth = remote ? remote->boundSize[0] * 0.5f : 20.0f;
	if(remote && remote->boundType == Remote::BoundType::E_BOUND_SPHERE && remote->angle > 0 && remote->angle < 180.0f && !graph) {
		proj.coneRadius = remote->boundSize[0] * 0.5f;
		proj.coneHalfCos = cosf(remote->angle * 0.5f * PI / 180.0f);
		proj.range = proj.coneRadius;
		proj.halfWidth = proj.coneRadius * sinf(remote->angle * 0.5f * PI / 180.0f);
	}
	const bool chainShot = remote && remote->behaviorType == Remote::BehaviorType::CHAIN;
	proj.hitsLeft = chainShot ? 1 : remote ? MAX(remote->activateCount, 1) : 1;
	proj.penetrationLeft = remote && remote->penetrationCount > 0 ? remote->penetrationCount : PENETRATION_UNLIMITED;
	proj.forwardReach = (remote && remote->boundType == Remote::BoundType::E_BOUND_BOX) ? MAX(remote->offsetY + RemoteBoundOffset(*remote) + remote->boundSize[1] * 0.5f, 0.0f) : 0.0f;
	proj.aoeRadius = bomb ? bomb->boundSize[0] * 0.5f : 0;
	proj.bombDoc = bomb ? bomb->ID : RemoteIdx::INVALID;
	if(remote && remote->behaviorType == Remote::BehaviorType::HOMING && remote->angularVel > 0) {
		proj.angularVel = remote->angularVel;
		proj.homingLength = remote->homingLength;
		proj.homingHalfCos = (remote->homingAngle > 0 && remote->homingAngle < 360) ? cosf(remote->homingAngle * 0.5f * PI / 180.0f) : -1.0f;
		proj.homingTime = remote->homingTime;
		proj.homingTimer = remote->homingTime;
		proj.homingLockOn = remote->homingLockOn;
	}

	proj.remoteDoc = remote ? remote->ID : RemoteIdx::INVALID;
	proj.nodeName = GetGameXmlContent().FindNodeName(wf.fireDummy.c_str());
	proj.pendingAdvance = player.shotLeadSec * proj.speed;
	if(player.pendingShotRawSize >= (i32)offsetof(Cl::CQ_GamePlayerSyncPhy, FirePosition)) {
		proj.remoteID = ((const Cl::CQ_GamePlayerSyncPhy*)player.pendingShotRaw.data())->RemoteSeedID + 1;
		proj.seedOwnerHd = player.clientHd;
	}
	replication->SendAttackActionToOthers(player.clientHd, player.Main().UID, WorldYawToMxmYaw(atan2f(proj.dir.y, proj.dir.x)), WeaponAttackState(player, wf));

	if(player.pendingShotRawSize > 0) {
		const vec3 target = vec3(vec2(origin) + proj.dir * proj.range, proj.graph ? player.body->GetWorldPos().z : origin.z);
		const u16 creator = replication->RelayWeaponShot(player.clientHd, player.pendingShotRaw.data(), player.pendingShotRawSize, target, remote ? remote->ID : RemoteIdx::INVALID, proj.graph);
		LOG("WeaponShot :: %ls firing=%d creator=%u doc=%d", player.name.data(), wf.firingMethod, creator, remote ? (i32)remote->ID : -1);
		player.pendingShotRawSize = 0;
	}
	else if(player.clientHd == ClientHandle::INVALID && remote) {
		const u32 seed = nextRemoteSeed;
		nextRemoteSeed += 1 + wf.CreatedRemotes();
		proj.remoteID = seed + 1;
		const vec3 firePos = proj.pos + vec3(0, 0, FIRE_NODE_HEIGHT);
		const vec3 target = vec3(vec2(firePos) + proj.dir * proj.range, proj.graph ? proj.pos.z : firePos.z);
		eastl::fixed_string<wchar,32,false> fireObject;
		for(char c : wf.fireDummy) fireObject.push_back((wchar)c);
		const u16 creator = DefaultWeaponCreator(player, wf);
		if(creator != 0 && !fireObject.empty()) replication->SendWeaponCreatorShotToAll(player.Main().UID, seed, remote->ID, firePos, atan2f(proj.dir.y, proj.dir.x), target, fireObject.c_str(), creator, player.chargeLevel, player.firingCombo, (u16)player.shotCount[player.mainCharaID & 1]);
		else replication->SendRemoteCreateToAll(player.Main().UID, seed, remote->ID, firePos, atan2f(proj.dir.y, proj.dir.x), target, ActorUID::INVALID, fireObject.empty() ? FIRE_OBJECT_SCENE_ROOT : fireObject.c_str());
	}
	proj.damage = bomb && remote->attackMultiplier <= 0 ? 0 : (i32)WeaponHitDamage(player, remote);
	proj.aoeDamage = (i32)WeaponHitDamage(player, bomb && bomb->attackMultiplier > 0 ? bomb : remote);
	if(bomb && bomb->behaviorType == Remote::BehaviorType::CHAIN) {
		proj.chainRemote = bomb->ID;
		proj.chainDamage = (i32)WeaponHitDamage(player, nullptr);
		proj.aoeRadius = 0;
		proj.bombDoc = RemoteIdx::INVALID;
	}
	if(bomb && bomb->timeEventDelay >= 0) {
		proj.zoneRemote = bomb->ID;
		proj.zoneDamage = (i32)WeaponHitDamage(player, nullptr);
	}
	if(remote && remote->behaviorType == Remote::BehaviorType::BEAM) {
		foreach(ri, remoteInsts) {
			if(ri->ownerIndex == player.index && ri->remote == remote && !ri->killNow) ri->killNow = true;
		}
		const SkillID beamSkill = remote->skillIndex != 0 ? SkillID(remote->skillIndex) : SkillID::INVALID;
		const i32 beamDamage = beamSkill != SkillID::INVALID ? (i32)PlayerSkillDamage(player, beamSkill) : proj.damage;
		RemoteInst* beam = SpawnRemote(player.index, remote->ID, player.body->GetWorldPos(), proj.dir, ActorUID::INVALID, beamSkill, nextCastID++, beamDamage, false);
		if(beam) {
			beam->clientRemoteID = proj.remoteID;
			beam->seedOwnerHd = proj.seedOwnerHd;
		}
		LOG("Shot :: %ls class=%d remote=%d beam held for %.1fs over %.0f", player.name.data(), (i32)classType, (i32)remote->ID, remote->lifeTime, beam ? beam->range : 0.0f);
		return;
	}
	if(remote && content.FindFireNodePath(remote->ID)) {
		RemoteInst* swing = SpawnRemote(player.index, remote->ID, player.body->GetWorldPos(), proj.dir, ActorUID::INVALID, SkillID::INVALID, nextCastID++, (i32)WeaponHitDamage(player, nullptr), false);
		if(swing) {
			swing->clientRemoteID = proj.remoteID;
			swing->seedOwnerHd = proj.seedOwnerHd;
		}
		LOG("Shot :: %ls class=%d remote=%d swings along its fire node path", player.name.data(), (i32)classType, (i32)remote->ID);
		return;
	}
	if(remote && !remote->timeEventRecycle && remote->timeEventDelay == 0.0f) {
		foreach_const(r, remote->onTime) if(r->idx != RemoteIdx::INVALID) SpawnRemote(player.index, r->idx, proj.pos, proj.dir, ActorUID::INVALID, SkillID::INVALID, nextCastID++, 0, true);
	}
	LOG("Shot :: %ls class=%d remote=%d graph=%d reliable=%d dist=%.0f range=%.0f speed=%.0f width=%.0f hits=%d aoe=%.0f", player.name.data(), (i32)classType, remote ? (i32)remote->ID : -1, graph, reliable, distance, proj.range, proj.speed, proj.halfWidth, proj.hitsLeft, proj.aoeRadius);
	if(!wf.extraRemotes.empty()) {
		const u32 firstID = proj.remoteID;
		projectileList.push_back(proj);
		for(i32 k = 0; k < (i32)wf.extraRemotes.size() && !projectileList.full(); k++) {
			const Remote* extra = content.FindRemote(wf.extraRemotes[k]);
			if(!extra) continue;
			const Remote* delayedChild = extra->boundType == Remote::BoundType::E_BOUND_NONE && extra->destroyCreateRemote != RemoteIdx::INVALID ? content.FindRemote(extra->destroyCreateRemote) : nullptr;
			if(delayedChild) {
				if(pendingRemotes.full()) continue;
				static const char* const LEFT_FIRE_DUMMY = "Attack_Fire_Dummy_l";
				PendingRemote pr;
				pr.casterMasterUID = player.Main().UID;
				pr.time = TimeAdd(localTime, TimeMsToTime(MAX(extra->lifeTime, 0.0f) * 1000.0));
				pr.idx = delayedChild->ID;
				pr.preset = (i32)ActionCommand::TargetPreset::SELF_FFF;
				pr.offsetSide = 0;
				pr.offsetFront = 0;
				pr.ownerIndex = player.index;
				pr.skillID = SkillID::INVALID;
				pr.castID = nextCastID++;
				pr.damage = (i32)WeaponHitDamage(player, nullptr);
				pr.castPos = proj.pos + vec3(proj.dir * proj.range, 0);
				pr.angle = atan2f(proj.dir.y, proj.dir.x);
				pr.target = ActorUID::INVALID;
				pr.nodeName = content.FindNodeName(LEFT_FIRE_DUMMY);
				pendingRemotes.push_back(pr);
				continue;
			}
			Projectile p = proj;
			p.remoteDoc = extra->ID;
			p.damage = (i32)WeaponHitDamage(player, extra);
			p.halfWidth = extra->boundSize[0] * 0.5f;
			if(firstID != 0) p.remoteID = firstID + (u32)k + 1;
			projectileList.push_back(p);
		}
		return;
	}
	const i32 parallel = wf.ChargeParallel(player.chargeLevel);
	if(parallel > 1) {
		const vec2 side = vec2(-proj.dir.y, proj.dir.x);
		const vec3 center = proj.pos;
		const u32 firstID = proj.remoteID;
		for(i32 k = 0; k < parallel && !projectileList.full(); k++) {
			proj.pos = center + vec3(side * (((f32)k - (f32)(parallel - 1) * 0.5f) * wf.parallelOffset), 0);
			proj.start = proj.pos;
			if(firstID != 0) proj.remoteID = firstID + (u32)k;
			projectileList.push_back(proj);
		}
		return;
	}
	if(wf.radiateNum <= 1) {
		projectileList.push_back(proj);
		return;
	}
	proj.volleyID = nextCastID++;
	const vec2 aim = proj.dir;
	const u32 firstID = proj.remoteID;
	for(i32 k = 0; k < wf.radiateNum && !projectileList.full(); k++) {
		const f32 a = ((f32)k + 0.5f - (f32)wf.radiateNum * 0.5f) * wf.radiateAngle / (f32)wf.radiateNum * PI / 180.0f;
		proj.dir = vec2(aim.x * cosf(a) - aim.y * sinf(a), aim.x * sinf(a) + aim.y * cosf(a));
		if(firstID != 0) proj.remoteID = firstID + (u32)k;
		projectileList.push_back(proj);
	}
}

f32 World::VolleyLockout(const Projectile& proj) const
{
	if(proj.volleyID == 0) return 0.0f;
	const Remote* shot = GetGameXmlContent().FindRemote(proj.remoteDoc);
	return shot ? shot->multiHitTimeOut : 0.0f;
}

bool World::VolleyHitLocked(const Projectile& proj, u32 target)
{
	const f32 lockout = VolleyLockout(proj);
	if(lockout <= 0) return false;
	foreach(r, repeatHits) {
		if(r->castID != proj.volleyID || r->targetIndex != target) continue;
		if(TimeDiffSec(TimeDiff(r->at, localTime)) < lockout) return true;
		r->at = localTime;
		return false;
	}
	if(!repeatHits.full()) repeatHits.push_back({ proj.volleyID, target, 1, localTime });
	return false;
}

void World::Explode(Player& owner, const vec3& center, f32 radius, i32 damage, const Projectile* source)
{
	Projectile hitInfo;
	if(source) {
		hitInfo = *source;
		if(source->bombDoc != RemoteIdx::INVALID) hitInfo.remoteDoc = source->bombDoc;
		hitInfo.pos = center;
	}
	LOG("Explosion :: at (%.0f, %.0f) radius=%.0f", center.x, center.y, radius);
	const Remote* bombRemote = source && source->bombDoc != RemoteIdx::INVALID ? GetGameXmlContent().FindRemote(source->bombDoc) : nullptr;
	const bool bombHit = source && source->damage <= 0 && source->bombDoc != RemoteIdx::INVALID;
	const bool shotExplodes = source && source->bombDoc == RemoteIdx::INVALID;
	const Remote* statusRemote = bombRemote ? bombRemote : (shotExplodes ? GetGameXmlContent().FindRemote(source->remoteDoc) : nullptr);
	const vec2 zone = vec2(center) + (bombRemote ? source->dir * RemoteBoundOffset(*bombRemote) : vec2(0));
	i32 mastersHit = 0;
	i32 npcsHit = 0;
	const bool singleActivation = source && source->bombDoc == RemoteIdx::INVALID && source->hitsLeft == 1;
	const Player* onlyPlayer = nullptr;
	const ActorNpc* onlyNpc = nullptr;
	if(singleActivation) {
		f32 best = FLT_MAX;
		foreach_const(it, players) {
			if(it->team == owner.team || it->dead || it->untargetable || fabsf(it->body->GetWorldPos().z - center.z) > WEAPON_PROJECTILE_HIT_HEIGHT) continue;
			const f32 d = LagDistToPoint(*it, zone);
			if(d > radius + HitRadius(*it) + WEAPON_HIT_TOLERANCE || d >= best) continue;
			best = d;
			onlyPlayer = &*it;
		}
		foreach_const(npc, actorNpcList) {
			if(!NpcHittableBy(*npc, owner)) continue;
			const f32 d = glm::length(vec2(npc->pos) - zone);
			if(d > radius + NpcHitRadius(*npc) || d >= best) continue;
			best = d;
			onlyPlayer = nullptr;
			onlyNpc = &*npc;
		}
	}
	foreach(it, players) {
		if(it->team == owner.team || it->dead || it->untargetable) continue;
		if(singleActivation && &*it != onlyPlayer) continue;
		const vec3 pos = it->body->GetWorldPos();
		if(fabsf(pos.z - center.z) > WEAPON_PROJECTILE_HIT_HEIGHT) continue;
		if(LagDistToPoint(*it, zone) > radius + HitRadius(*it) + WEAPON_HIT_TOLERANCE) continue;
		const f32 falloff = bombRemote ? RemoteDistanceFalloff(*bombRemote, glm::length(vec2(pos) - zone)) : 1.0f;
		const f32 passive = bombHit ? PassiveWeaponDamageMul(owner, *it) : 1.0f;
		DamagePlayer(owner, *it, SkillID::INVALID, (i32)(damage * falloff * passive), source ? &hitInfo : nullptr);
		if(bombHit || shotExplodes) PassiveOnWeaponHit(owner, *it, hitInfo);
		mastersHit++;
	}
	foreach(npc, actorNpcList) {
		if(!NpcHittableBy(*npc, owner)) continue;
		if(singleActivation && &*npc != onlyNpc) continue;
		const f32 d = glm::length(vec2(npc->pos) - zone);
		if(d > radius + NpcHitRadius(*npc)) continue;
		Replication::RemoteDamage hit;
		if(source) {
			hit.remoteID = hitInfo.remoteID;
			hit.remoteDoc = hitInfo.remoteDoc;
			hit.remotePos = vec3(vec2(center), npc->pos.z);
			hit.dir = hitInfo.dir;
			hit.seedOwnerHd = hitInfo.seedOwnerHd;
		}
		DamageNpc(owner, *npc, SkillID::INVALID, (i32)(damage * (bombRemote ? RemoteDistanceFalloff(*bombRemote, d) : 1.0f)), source ? &hit : nullptr);
		npcsHit++;
		if(bombHit || shotExplodes) WeaponHitPlagueNpc(owner, *npc);
		if(!statusRemote || npc->hp <= 0) continue;
		foreach_const(st, statusRemote->statuses) if(st->rate >= 100 || (i32)(RandUint() % 100) < st->rate) ApplyStatusToNpc(owner, *npc, StanceUpgradedStatus(owner, st->statusID), false);
	}
	if(bombRemote && !bombRemote->friendlyStatuses.empty()) {
		foreach(it, players) {
			if(it->team != owner.team || it->dead || (it->index == owner.index && !bombRemote->vsOwner)) continue;
			const vec3 pos = it->body->GetWorldPos();
			if(fabsf(pos.z - center.z) > WEAPON_PROJECTILE_HIT_HEIGHT) continue;
			if(glm::length(vec2(pos) - zone) > radius + HitRadius(*it)) continue;
			ApplyFriendlyRemoteStatuses(owner, *bombRemote, &*it, nullptr);
		}
		const Faction ownFaction = owner.team == 0 ? Faction::RED : Faction::BLUE;
		foreach(npc, actorNpcList) {
			if(npc->faction != ownFaction || npc->hp <= 0 || npc->removeAt != Time::ZERO) continue;
			if(glm::length(vec2(npc->pos) - zone) > radius + NpcHitRadius(*npc)) continue;
			ApplyFriendlyRemoteStatuses(owner, *bombRemote, nullptr, &*npc);
		}
	}
	if(mastersHit + npcsHit >= 2 && owner.mana >= 0 && localTime >= owner.multiHitPassiveReady) {
		if(const ActiveStatus* st = FindPassiveOfType(owner, StatusType::ARTILLERY)) {
			owner.mana = MIN(owner.mana + st->info->Param(mastersHit > 0 ? "_AddValuePC" : "_AddValueNPC"), MasterStat(owner.Main().classType, STAT_MANA));
			owner.manaDirty = true;
			owner.multiHitPassiveReady = TimeAdd(localTime, TimeMsToTime(st->info->Param("_Duration")));
			LOG("Passive :: %ls hit %d masters %d creatures -> mana %.0f", owner.name.data(), mastersHit, npcsHit, owner.mana);
		}
	}
}

void World::SteerProjectile(Projectile& proj, f32 delta)
{
	const Player& owner = players[proj.ownerIndex];
	const auto targetable = [&](const Player& p) { return p.team != owner.team && !p.dead && !p.untargetable; };
	if(proj.homingTarget >= 0 && !targetable(players[proj.homingTarget])) proj.homingTarget = -1;
	const ActorNpc* lockedNpc = proj.homingNpc != ActorUID::INVALID ? FindNpcActor(proj.homingNpc) : nullptr;
	if(proj.homingNpc != ActorUID::INVALID && (!lockedNpc || !NpcHittableBy(*lockedNpc, owner))) { proj.homingNpc = ActorUID::INVALID; lockedNpc = nullptr; }
	if(proj.homingTarget >= 0 && glm::length(vec2(players[proj.homingTarget].body->GetWorldPos() - proj.pos)) > proj.homingLength) proj.homingTarget = -1;
	if(lockedNpc && glm::length(vec2(lockedNpc->pos - proj.pos)) > proj.homingLength) { proj.homingNpc = ActorUID::INVALID; lockedNpc = nullptr; }
	const bool hasTarget = proj.homingTarget >= 0 || lockedNpc;
	proj.homingTimer += delta;
	if(proj.homingTimer >= proj.homingTime && !(proj.homingLockOn && hasTarget)) {
		proj.homingTimer = 0;
		f32 bestDist = proj.homingLength;
		i32 found = -1;
		const ActorNpc* foundNpc = nullptr;
		foreach_const(it, players) {
			if(!targetable(*it)) continue;
			const vec2 rel = vec2(it->body->GetWorldPos() - proj.pos);
			const f32 d = glm::length(rel);
			if(d > bestDist) continue;
			if(d > 1.0f && glm::dot(rel / d, proj.dir) < proj.homingHalfCos) continue;
			bestDist = d;
			found = (i32)it->index;
		}
		foreach_const(npc, actorNpcList) {
			if(!NpcHittableBy(*npc, owner)) continue;
			const vec2 rel = vec2(npc->pos - proj.pos);
			const f32 d = glm::length(rel);
			if(d > bestDist) continue;
			if(d > 1.0f && glm::dot(rel / d, proj.dir) < proj.homingHalfCos) continue;
			bestDist = d;
			found = -1;
			foundNpc = &*npc;
		}
		if(found >= 0 && found != proj.homingTarget) LOG("Shot :: homing projectile locked %ls at %.0f", players[found].name.data(), bestDist);
		proj.homingTarget = found;
		proj.homingNpc = foundNpc ? foundNpc->UID : ActorUID::INVALID;
		lockedNpc = foundNpc;
	}
	if(proj.homingTarget < 0 && !lockedNpc) return;
	const vec2 rel = lockedNpc ? vec2(lockedNpc->pos - proj.pos) : vec2(players[proj.homingTarget].body->GetWorldPos() - proj.pos);
	if(glm::length(rel) < 1.0f) return;
	proj.dir = SteerToward(proj.dir, rel, proj.angularVel * delta);
}

void World::ApplyFriendlyRemoteStatuses(Player& owner, const Remote& remote, Player* target, ActorNpc* npc)
{
	foreach_const(st, remote.friendlyStatuses) {
		if(st->rate < 100 && (i32)(RandUint() % 100) >= st->rate) continue;
		if(target) {
			ApplyStatus(owner, *target, st->statusID);
			continue;
		}
		const StatusInfo* flagged = GetGameXmlContent().FindStatus(st->statusID);
		if(!flagged) continue;
		if(npc->monsterAi && (flagged->type == StatusType::FORCEBACK || flagged->type == StatusType::PUSH)) {
			ApplyStatusToNpc(owner, *npc, flagged->ID, false);
			continue;
		}
		ApplyStatusToNpc(owner, *npc, flagged->ID, false);
		const StatusInfo* info = flagged->flagEffectStatus != 0 ? GetGameXmlContent().FindStatus(flagged->flagEffectStatus) : nullptr;
		if(!info || npc->ownerUID == ActorUID::INVALID) continue;
		ApplyMonsterStatus(*npc, info->ID, true);
		if(info->descHealPct <= 0 || npc->hp >= npc->hpMax) continue;
		npc->hp = MIN(npc->hp + (i32)(npc->hpMax * info->descHealPct / 100.0f), npc->hpMax);
		replication->SendHpToAll(npc->UID, npc->hp, npc->hpMax);
		LOG("Summon :: creature=%d repaired %.0f%% by %ls (hp %d/%d)", (i32)npc->docID, info->descHealPct, owner.name.data(), npc->hp, npc->hpMax);
	}
}

void World::UpdateProjectiles(f32 delta)
{
	for(auto proj = projectileList.begin(); proj != projectileList.end(); ) {
		Player& owner = players[proj->ownerIndex];
		if(proj->angularVel > 0) SteerProjectile(*proj, delta);
		if(proj->angularVel > 0 && proj->remoteID != 0 && proj->seedOwnerHd == ClientHandle::INVALID) {
			const ActorUID target = proj->homingTarget >= 0 ? players[proj->homingTarget].Main().UID : proj->homingNpc;
			if(!proj->syncedInit) { proj->syncedInit = true; proj->syncedTarget = target; }
			proj->syncIn -= delta;
			const bool retarget = target != proj->syncedTarget;
			if(retarget || proj->syncIn <= 0) {
				proj->syncIn = retarget ? MEASURED_REMOTE_SYNC_PERIOD_SEC : proj->syncIn + MEASURED_REMOTE_SYNC_PERIOD_SEC;
				proj->syncedTarget = target;
				replication->SendRemoteSyncTargetToAll(proj->remoteID, target, proj->pos + vec3(0, 0, FIRE_NODE_HEIGHT), &proj->dir);
			}
		}
		f32 step = MIN(proj->speed * delta + proj->pendingAdvance, proj->range - proj->travelled);
		proj->pendingAdvance = 0;
		bool hitWall = false;
		const PxRigidActor* wallActor = nullptr;
		ActorDynamic* wallGate = nullptr;
		if(!proj->graph && !proj->ignoreWalls && step > 0) {
			f32 wallDist = 0;
			const vec2 a = vec2(proj->pos);
			if(RaycastShot(vec3(a, proj->wallZ), vec3(a + proj->dir * step, proj->wallZ), (i32)players[proj->ownerIndex].team, &wallDist, &wallActor, &wallGate)) {
				step = wallDist;
				hitWall = true;
			}
		}
		const bool lastStep = hitWall || proj->travelled + step >= proj->range - 0.01f;
		const vec2 from = vec2(proj->pos);
		const vec2 to = from + proj->dir * step;
		bool destroyed = false;

		if(!proj->graph) {
			const Remote* shotRemote = GetGameXmlContent().FindRemote(proj->remoteDoc);
			vec2 contact;
			RemoteInst* reflector = shotRemote && shotRemote->canReflect ? FindProjectileReflector(*proj, from, to, &contact) : nullptr;
			if(!reflector && shotRemote && shotRemote->canReflect) {
				if(const MonsterRemote* blocker = FindMonsterProjectileBlocker(*proj, from, to)) {
					LOG("Shot :: remote %d blocked by creature remote %d", (i32)proj->remoteDoc, (i32)blocker->remote->ID);
					proj = projectileList.erase_unsorted(proj);
					continue;
				}
			}
			if(reflector) {
				const Projectile src = *proj;
				proj = projectileList.erase_unsorted(proj);
				if(!src.reflected && (reflector->remote->reflectUse || IsMeasuredReflector(reflector->remote->ID))) ReflectProjectile(src, *reflector, contact);
				else LOG("Shot :: remote %d absorbed by %d", (i32)src.remoteDoc, (i32)reflector->remote->ID);
				continue;
			}
			u32 activationCost = 0;
			const Player* activationTarget = nullptr;
			if(shotRemote && !shotRemote->friendlyStatuses.empty() && (shotRemote->contactFriend || shotRemote->includeParty)) {
				foreach(it, players) {
					if(it->team != owner.team || it->dead || it->index == owner.index || proj->friendHitList.full()) continue;
					if(eastl::find(proj->friendHitList.begin(), proj->friendHitList.end(), it->index) != proj->friendHitList.end()) continue;
					const vec3 pos = it->body->GetWorldPos();
					if(fabsf(pos.z - proj->pos.z) > WEAPON_PROJECTILE_HIT_HEIGHT) continue;
					if(SegmentPointDist2D(from, to, vec2(pos)) > proj->halfWidth + HitRadius(*it)) continue;
					proj->friendHitList.push_back(it->index);
					ApplyFriendlyRemoteStatuses(owner, *shotRemote, &*it, nullptr);
				}
				const Faction ownFaction = owner.team == 0 ? Faction::RED : Faction::BLUE;
				foreach(npc, actorNpcList) {
					if(npc->faction != ownFaction || npc->hp <= 0 || npc->removeAt != Time::ZERO || proj->friendNpcHitList.full()) continue;
					if(eastl::find(proj->friendNpcHitList.begin(), proj->friendNpcHitList.end(), npc->UID) != proj->friendNpcHitList.end()) continue;
					if(SegmentPointDist2D(from, to, vec2(npc->pos)) > proj->halfWidth + NpcHitRadius(*npc)) continue;
					proj->friendNpcHitList.push_back(npc->UID);
					ApplyFriendlyRemoteStatuses(owner, *shotRemote, nullptr, &*npc);
				}
			}
			const bool singleActivation = proj->hitsLeft == 1;
			foreach(it, players) {
				if(it->team == owner.team || it->dead || it->untargetable) continue;
				if(eastl::find(proj->hitList.begin(), proj->hitList.end(), it->index) != proj->hitList.end()) continue;
				const vec3 pos = it->body->GetWorldPos();
				const f32 lagDist = LagDistToSegment(*it, from, to);
				proj->closest = MIN(proj->closest, lagDist);
				if(fabsf(pos.z - proj->pos.z) > WEAPON_PROJECTILE_HIT_HEIGHT) continue;
				if(!LagSegmentHits(*it, from, to, proj->halfWidth + HitRadius(*it) + PROJECTILE_HIT_TOLERANCE, lastStep ? proj->forwardReach + HitRadius(*it) : HitRadius(*it))) continue;
				if(!ProjectileConeHolds(*proj, vec2(pos), HitRadius(*it))) continue;

				if(!proj->hitList.full()) proj->hitList.push_back(it->index);
				if(VolleyHitLocked(*proj, it->index)) continue;
				f32 volleyMul = 1.0f;
				if(proj->volleyID != 0 && !VolleyLockout(*proj)) {
					bool repeated = false;
					foreach(r, repeatHits) if(r->castID == proj->volleyID && r->targetIndex == it->index) repeated = true;
					if(repeated) volleyMul = MEASURED_RADIATE_REPEAT_HIT_RATIO;
					else repeatHits.push_back({ proj->volleyID, it->index, 1, localTime });
				}
				if(proj->damage > 0) DamagePlayer(owner, *it, SkillID::INVALID, (i32)(proj->damage * volleyMul * PassiveWeaponDamageMul(owner, *it)), &*proj);
				if(proj->damage > 0 || proj->bombDoc == RemoteIdx::INVALID) PassiveOnWeaponHit(owner, *it, *proj);
				const i32 cost = PenetrationCost(*it);
				activationCost += (u32)cost;
				if(!activationTarget) activationTarget = &*it;
				proj->penetrationLeft -= cost;
				if(proj->penetrationLeft <= 0 || singleActivation) break;
			}
			if(activationTarget) {
				const vec3 hitPos = vec3(vec2(activationTarget->body->GetWorldPos()), proj->pos.z);
				if(proj->remoteID != 0) replication->SendRemoteActivatedToAll(proj->remoteID, proj->seedOwnerHd, ClientHandle::INVALID, activationTarget->Main().UID, activationCost, hitPos);
				if(shotRemote) SpawnProjectileHitRemotes(*proj, *shotRemote, hitPos, (i32)activationTarget->index, ActorUID::INVALID);
				if(proj->chainRemote != RemoteIdx::INVALID && !proj->chained) {
					proj->chained = true;
					RemoteInst* chain = SpawnRemote(proj->ownerIndex, proj->chainRemote, hitPos, proj->dir, ActorUID::INVALID, SkillID::INVALID, nextCastID++, proj->chainDamage, true, nullptr, proj->nodeName, false, false, true);
					if(chain) {
						chain->excludeIndex = (i32)activationTarget->index;
						chain->chainHitMask = 1u << activationTarget->index;
					}
				}
				proj->hitsLeft--;
				if(proj->hitsLeft <= 0 || proj->penetrationLeft <= 0) {
					proj->pos = ProjectileContact(*proj, vec2(hitPos), HitRadius(*activationTarget));
					destroyed = true;
				}
			}
			foreach(npc, actorNpcList) {
				if(destroyed || proj->npcHitList.full() || (proj->damage <= 0 && proj->bombDoc == RemoteIdx::INVALID) || !NpcHittableBy(*npc, owner)) continue;
				if(eastl::find(proj->npcHitList.begin(), proj->npcHitList.end(), npc->UID) != proj->npcHitList.end()) continue;
				if(SegmentPointDist2D(from, lastStep ? to + proj->dir * proj->forwardReach : to, vec2(npc->pos)) > proj->halfWidth + NpcHitRadius(*npc) + PROJECTILE_HIT_TOLERANCE) continue;
				if(!ProjectileConeHolds(*proj, vec2(npc->pos), NpcHitRadius(*npc))) continue;
				proj->npcHitList.push_back(npc->UID);
				if(VolleyHitLocked(*proj, (u32)npc->UID | VOLLEY_NPC_TARGET_BIT)) continue;
				f32 volleyMul = 1.0f;
				if(proj->volleyID != 0 && !VolleyLockout(*proj)) {
					const u32 key = (u32)npc->UID | VOLLEY_NPC_TARGET_BIT;
					bool repeated = false;
					foreach(r, repeatHits) if(r->castID == proj->volleyID && r->targetIndex == key) repeated = true;
					if(repeated) volleyMul = MEASURED_RADIATE_REPEAT_HIT_RATIO;
					else repeatHits.push_back({ proj->volleyID, key, 1, localTime });
				}
				Replication::RemoteDamage hit;
				hit.remoteID = proj->remoteID;
				hit.remoteDoc = proj->remoteDoc;
				hit.pierceHits = (u32)(proj->hitList.size() + proj->npcHitList.size() - 1);
				hit.remotePos = vec3(vec2(proj->pos), npc->pos.z);
				hit.dir = proj->dir;
				hit.seedOwnerHd = proj->seedOwnerHd;
				if(proj->damage > 0) DamageNpc(owner, *npc, SkillID::INVALID, (i32)(proj->damage * volleyMul), &hit);
				if(proj->remoteID != 0) {
					proj->npcActivation += NpcDensity(*npc);
					replication->SendRemoteActivatedToAll(proj->remoteID, proj->seedOwnerHd, ClientHandle::INVALID, npc->UID, proj->npcActivation, proj->pos);
				}
				if(shotRemote) SpawnProjectileHitRemotes(*proj, *shotRemote, vec3(vec2(npc->pos), proj->pos.z), -1, npc->UID);
				if(const Remote* shot = GetGameXmlContent().FindRemote(proj->remoteDoc)) {
					if(npc->hp > 0 && npc->ownerUID == ActorUID::INVALID) {
						foreach_const(st, shot->statuses) if(st->rate >= 100 || (i32)(RandUint() % 100) < st->rate) ApplyStatusToNpc(owner, *npc, StanceUpgradedStatus(owner, st->statusID), false);
					}
					WeaponHitPlagueNpc(owner, *npc);
				}
				if(proj->chainRemote != RemoteIdx::INVALID && !proj->chained) {
					proj->chained = true;
					RemoteInst* npcChain = SpawnRemote(proj->ownerIndex, proj->chainRemote, vec3(vec2(npc->pos), proj->pos.z), proj->dir, ActorUID::INVALID, SkillID::INVALID, nextCastID++, proj->chainDamage, true, nullptr, proj->nodeName, false, false, true);
					if(npcChain) npcChain->chainNpcHit = npc->UID;
				}
				proj->hitsLeft--;
				if(proj->hitsLeft <= 0) {
					proj->pos = ProjectileContact(*proj, vec2(npc->pos), NpcHitRadius(*npc));
					destroyed = true;
				}
			}
		}

		if(!destroyed) {
			proj->pos += vec3(proj->dir * step, 0);
			proj->travelled += step;
			destroyed = hitWall || proj->travelled >= proj->range - 0.01f;
			if(hitWall) {
				LOG("Shot :: projectile stopped by a wall after %.0f", proj->travelled);
				OnDynamicHit(wallActor, proj->remoteID, proj->seedOwnerHd, ClientHandle::INVALID, proj->pos, wallGate, (i32)proj->ownerIndex, proj->damage, proj->remoteDoc);
				ActorNpc* wallNpc = nullptr;
				f32 best = WALL_HIT_SEARCH_RADIUS;
				foreach(npc, actorNpcList) {
					if(!npc->wall || !NpcHittableBy(*npc, owner)) continue;
					const f32 d = glm::length(vec2(npc->pos) - vec2(proj->pos));
					if(d < best) { best = d; wallNpc = &*npc; }
				}
				if(wallNpc && proj->damage > 0) DamageNpc(owner, *wallNpc, SkillID::INVALID, proj->damage);
			}
		}

		if(destroyed) {
			const Player* nearPlayer = nullptr;
			f32 nearDist = 999999.0f;
			foreach_const(it, players) {
				if(it->team == owner.team || it->dead || it->untargetable) continue;
				const f32 d = SegmentPointDist2D(vec2(proj->start), vec2(proj->pos), vec2(it->body->GetWorldPos()));
				if(d < nearDist) {
					nearDist = d;
					nearPlayer = &*it;
				}
			}
			if(nearPlayer) {
				const vec2 rel = vec2(nearPlayer->body->GetWorldPos()) - vec2(proj->start);
				LOG("ProjectileEnd :: start=(%.0f, %.0f, %.0f) dir=(%.2f, %.2f) travelled=%.0f hit=%d nearest=%ls at (%.0f, %.0f, %.0f) along=%.0f side=%.0f lagClosest=%.0f", proj->start.x, proj->start.y, proj->start.z, proj->dir.x, proj->dir.y, proj->travelled, proj->hitsLeft <= 0, nearPlayer->name.data(), nearPlayer->body->GetWorldPos().x, nearPlayer->body->GetWorldPos().y, nearPlayer->body->GetWorldPos().z, glm::dot(rel, proj->dir), glm::dot(rel, vec2(-proj->dir.y, proj->dir.x)), proj->closest);
			}
			if(proj->zoneRemote != RemoteIdx::INVALID) {
				SpawnRemote(proj->ownerIndex, proj->zoneRemote, proj->pos, proj->dir, ActorUID::INVALID, SkillID::INVALID, nextCastID++, proj->zoneDamage, true, nullptr, proj->nodeName);
			}
			else if(proj->aoeRadius > 0) {
				if(proj->bombDoc != RemoteIdx::INVALID) {
					const u32 seed = nextRemoteSeed;
					nextRemoteSeed += 2;
					replication->SendRemoteCreateToAll(owner.Main().UID, seed, proj->bombDoc, proj->pos, atan2f(proj->dir.y, proj->dir.x), proj->pos, ActorUID::INVALID, GetGameXmlContent().GetNodeName(proj->nodeName));
				}
				Explode(owner, proj->pos, proj->aoeRadius, proj->aoeDamage, &*proj);
			}
			else if(proj->graph && proj->hitsLeft > 0) {
				Explode(owner, proj->pos, proj->halfWidth, proj->aoeDamage, &*proj);
			}
			proj = projectileList.erase_unsorted(proj);
		}
		else {
			++proj;
		}
	}
}

static bool SegmentReachesPoint(const vec2& a, const vec2& b, const vec2& p, f32 lateral, f32 forwardReach)
{
	const vec2 ab = b - a;
	const f32 len = glm::length(ab);
	if(len < 0.001f) return glm::length(p - a) <= lateral;
	const vec2 dir = ab / len;
	const vec2 rel = p - a;
	const f32 along = glm::dot(rel, dir);
	if(along < -PROJECTILE_TARGET_RADIUS || along > len + forwardReach) return false;
	return fabsf(rel.x * dir.y - rel.y * dir.x) <= lateral;
}

bool World::LagSegmentHits(const Player& p, const vec2& a, const vec2& b, f32 lateral, f32 forwardReach) const
{
	if(SegmentReachesPoint(a, b, vec2(p.body->GetWorldPos()), lateral, forwardReach)) return true;
	for(u32 i = 0; i < p.posHistoryCount; i++) {
		if(SegmentReachesPoint(a, b, vec2(p.posHistory[i]), lateral, forwardReach)) return true;
	}
	return false;
}

f32 World::LagDistToSegment(const Player& p, const vec2& a, const vec2& b) const
{
	f32 best = SegmentPointDist2D(a, b, vec2(p.body->GetWorldPos()));
	for(u32 i = 0; i < p.posHistoryCount; i++) {
		best = MIN(best, SegmentPointDist2D(a, b, vec2(p.posHistory[i])));
	}
	return best;
}

f32 World::LagDistToPoint(const Player& p, const vec2& point) const
{
	return glm::length(LagClosestPos(p, point) - point);
}

vec2 World::LagClosestPos(const Player& p, const vec2& point) const
{
	vec2 best = vec2(p.body->GetWorldPos());
	f32 bestDist = glm::length(best - point);
	for(u32 i = 0; i < p.posHistoryCount; i++) {
		const vec2 h = vec2(p.posHistory[i]);
		const f32 d = glm::length(h - point);
		if(d < bestDist) {
			bestDist = d;
			best = h;
		}
	}
	return best;
}

void World::UpdateWeaponHeat(Player& player)
{
	const WeaponHeatInfo* h = ActiveWeaponHeat(player);
	if(!h) return;
	const u32 m = player.mainCharaID & 1;

	if(player.overheatUntil[m] != Time::ZERO && localTime >= player.overheatUntil[m]) {
		player.overheatUntil[m] = Time::ZERO;
		player.heat[m] = h->overheatResetGauge;
		player.heatLastShot[m] = TimeAdd(localTime, TimeMsToTime(-h->autoRegenDelay * 1000.0));
		replication->SendGauge(player.clientHd, player.userID, 1, false, 0, (u16)h->maxGauge, (u16)h->maxGauge);
		LOG("Overheat :: %ls weapon cooled down", player.name.data());
	}
}

void World::OnWeaponShotPacket(Player& player, const vec3& origin, const vec2& dir, f32 distance, const u8* rawPacket, i32 rawSize)
{
	if(frozen || combatLocked) return;
	bool blocked = false;
	foreach_const(st, activeStatuses) {
		if(st->targetIndex != player.index || !st->info || !st->info->blockAttack || localTime >= st->until) continue;
		bool ownAniEnd = false;
		if(st->casterIndex == player.index) foreach_const(id, player.weaponAniEndStatuses) if(*id == st->statusID) ownAniEnd = true;
		if(!ownAniEnd) blocked = true;
	}
	if(blocked) {
		LOG("Blocked :: %ls shot ignored (attack blocked by a status)", player.name.data());
		return;
	}
	if(!WeaponHeatShot(player, localTime, replication, WeaponHeatScale(player))) {
		LOG("Overheat :: %ls shot ignored (weapon overheated)", player.name.data());
		return;
	}
	BreakHide(player);

	player.pendingShotRawSize = MIN(rawSize, (i32)player.pendingShotRaw.size());
	memmove(player.pendingShotRaw.data(), rawPacket, player.pendingShotRawSize);

	if(player.pendingAim) {
		player.pendingAim = false;
		PlayerWeaponShot(player, origin, vec2(cosf(player.pendingAimYaw), sinf(player.pendingAimYaw)), distance);
		return;
	}
	player.pendingShot = true;
	player.pendingShotOrigin = origin;
	player.pendingShotDir = dir;
	player.pendingShotDistance = distance;
	player.pendingShotTime = localTime;
}

void World::OnWeaponAimPacket(Player& player, f32 aimYaw)
{
	if(player.pendingShot) {
		player.pendingShot = false;
		PlayerWeaponShot(player, player.pendingShotOrigin, vec2(cosf(aimYaw), sinf(aimYaw)), player.pendingShotDistance);
		return;
	}
	player.pendingAim = true;
	player.pendingAimYaw = aimYaw;
	player.pendingAimTime = localTime;
}

WeaponIndex World::ActiveWeapon(const Player& p) const
{
	if(p.titanForm >= 0) {
		const WeaponIndex titanWeapon = WeaponIndex(GetGameXmlContent().titan.forms[p.titanForm].weapon);
		if(GetGameXmlContent().FindWeaponFiring((i32)titanWeapon, 0) && GetGameXmlContent().GetWeaponAttack((i32)titanWeapon) > 0) return titanWeapon;
	}
	return (p.mainCharaID & 1) == 0 ? p.mainWeapon : p.subWeapon;
}

const WeaponFiring& World::ActiveWeaponFiring(const Player& p, i8 stance) const
{
	const WeaponIndex weapon = ActiveWeapon(p);
	const WeaponFiring* wf = GetGameXmlContent().FindWeaponFiring((i32)weapon, stance);
	ASSERT(wf);
	return *wf;
}

RemoteIdx World::SelectWeaponRemote(Player& player, const WeaponFiring& wf)
{
	const u32 m = player.mainCharaID & 1;
	if(player.clientHd == ClientHandle::INVALID) {
		const bool chained = player.firingCombo < wf.comboValid.size() && player.botComboShotAt != Time::ZERO && (wf.comboValid[player.firingCombo] <= 0 || localTime <= TimeAdd(player.botComboShotAt, TimeMsToTime(wf.comboValid[player.firingCombo] * 1000.0)));
		player.firingCombo = chained ? player.firingCombo + 1 : 0;
		player.botComboShotAt = wf.comboValid.empty() ? Time::ZERO : localTime;
	}
	RemoteIdx idx = wf.base;
	if(player.chargeLevel > 0 && player.chargeLevel <= wf.charge.size()) idx = wf.charge[player.chargeLevel - 1];
	else if(player.firingCombo > 0 && player.firingCombo <= wf.combo.size()) idx = wf.combo[player.firingCombo - 1];
	player.shotCount[m]++;
	if(wf.countRate > 0 && wf.countRemote != RemoteIdx::INVALID && player.shotCount[m] % wf.countRate == 0) idx = wf.countRemote;
	if(wf.replaceStatus != 0 && wf.replaceRemote != RemoteIdx::INVALID && FindActiveStatus(player.index, wf.replaceStatus)) {
		idx = wf.replaceRemote;
		ConsumeStatus(player.index, wf.replaceStatus);
		LOG("Passive :: %ls weapon status %d used -> remote %d", player.name.data(), wf.replaceStatus, (i32)idx);
	}
	return idx;
}

void World::OnWeaponState(Player& player, i32 state, u8 chargeLevel, u8 firingCombo)
{
	if(state != 0) MarkInBattle(player);
	if(state == WEAPON_STATE_CHARGE && player.weaponState != WEAPON_STATE_CHARGE) {
		player.chargeStart = localTime;
		player.chargeAnnounced = 0;
		player.chargeDelayedDone = 0;
		const WeaponFiring& chargeFiring = ActiveWeaponFiring(player, player.Main().stance);
		player.firingMoveSpeedMul = chargeFiring.firingStatDuration > 0 ? chargeFiring.firingMoveSpeedMul : 0.0f;
		const GameXmlContent::Action* charge = GetGameXmlContent().FindSkillAction(ActionClass(player), ActionStateID::ATTACKCHARGE_BEHAVIORSTATE, player.Main().stance);
		if(charge && !player.dead) foreach_const(cmd, charge->commands) {
			if(cmd->type != ActionCommand::Type::STATUS || cmd->status.statusIndex == 0 || cmd->status.remove || cmd->status.removeAtEnd || cmd->delay > 0) continue;
			if(cmd->status.targetPreset == ActionCommand::TargetPreset::SELF_FFF || cmd->status.targetPreset == ActionCommand::TargetPreset::SELF_TFF) ApplyStatus(player, player, cmd->status.statusIndex);
		}
	}
	if(state != WEAPON_STATE_CHARGE && player.weaponState == WEAPON_STATE_CHARGE) {
		const GameXmlContent::Action* charge = GetGameXmlContent().FindSkillAction(ActionClass(player), ActionStateID::ATTACKCHARGE_BEHAVIORSTATE, player.Main().stance);
		if(charge) foreach_const(cmd, charge->commands) {
			if(cmd->type != ActionCommand::Type::STATUS || cmd->status.statusIndex == 0 || !cmd->status.removeAtEnd) continue;
			foreach(st, activeStatuses) if(st->targetIndex == player.index && st->statusID == cmd->status.statusIndex && localTime < st->until) st->until = localTime;
		}
	}
	if(state == WEAPON_STATE_FIRE && player.weaponState == WEAPON_STATE_CHARGE) {
		if(chargeLevel != player.chargeAnnounced) LOG("Shot :: %ls client charge level %d, announced %d", player.name.data(), (i32)chargeLevel, (i32)player.chargeAnnounced);
		chargeLevel = player.chargeAnnounced;
		player.chargeAnnounced = 0;
	}
	player.firingCombo = firingCombo;
	player.chargeLevel = chargeLevel;
	player.weaponState = state;
}

void World::UpdateWeaponCharges()
{
	foreach(it, players) {
		if(it->weaponState != WEAPON_STATE_CHARGE || it->dead) continue;
		const WeaponFiring& wf = ActiveWeaponFiring(*it, it->Main().stance);
		const f64 held = TimeDiffSec(TimeDiff(it->chargeStart, localTime));
		const GameXmlContent::Action* charge = GetGameXmlContent().FindSkillAction(ActionClass(*it), ActionStateID::ATTACKCHARGE_BEHAVIORSTATE, it->Main().stance);
		if(charge) {
			u32 bit = 1;
			foreach_const(cmd, charge->commands) {
				const bool delayed = cmd->type == ActionCommand::Type::STATUS && cmd->status.statusIndex != 0 && !cmd->status.remove && cmd->delay > 0 && (cmd->status.targetPreset == ActionCommand::TargetPreset::SELF_FFF || cmd->status.targetPreset == ActionCommand::TargetPreset::SELF_TFF);
				if(delayed && !(it->chargeDelayedDone & bit) && held >= cmd->delay) {
					it->chargeDelayedDone |= bit;
					ApplyStatus(*it, *it, cmd->status.statusIndex);
				}
				bit <<= 1;
			}
		}
		while(it->chargeAnnounced < wf.chargeTime.size() && it->chargeAnnounced < wf.charge.size() && held >= wf.chargeTime[it->chargeAnnounced]) {
			it->chargeAnnounced++;
			replication->RelayWeaponState(ClientHandle::INVALID, it->Main().UID, MEASURED_WEAPON_STATE_WEAPON_ID, WEAPON_STATE_CHARGE, it->chargeAnnounced, it->firingCombo);
			LOG("Shot :: %ls charge level %d after %.2fs", it->name.data(), (i32)it->chargeAnnounced, held);
		}
	}
}

f32 World::WeaponRange(const Player& p) const
{
	const WeaponFiring& wf = ActiveWeaponFiring(p, p.Main().stance);
	const Remote* remote = GetGameXmlContent().FindRemote(wf.base);
	ASSERT(remote);
	if(const Remote* hit = WeaponDelayedHit(*remote)) return WeaponReach(*hit, wf);
	return WeaponReach(*remote, wf);
}

bool World::WeaponHitsLater(const Player& p) const
{
	const Remote* remote = GetGameXmlContent().FindRemote(ActiveWeaponFiring(p, p.Main().stance).base);
	return remote && WeaponDelayedHit(*remote) != nullptr;
}

bool World::WeaponIsMelee(const Player& p) const
{
	const WeaponFiring& wf = ActiveWeaponFiring(p, p.Main().stance);
	const Remote* remote = GetGameXmlContent().FindRemote(wf.base);
	ASSERT(remote);
	if(const Remote* hit = WeaponDelayedHit(*remote)) return remote->maxSpeed <= 0 && hit->maxSpeed <= 0 && hit->behaviorType == Remote::BehaviorType::NONETARGET && !wf.spawnAtAim && (hit->boundType == Remote::BoundType::E_BOUND_BOX || hit->boundType == Remote::BoundType::E_BOUND_SPHERE);
	if(remote->maxSpeed > 0 || remote->behaviorType != Remote::BehaviorType::NONETARGET || wf.spawnAtAim) return false;
	if(remote->boundType != Remote::BoundType::E_BOUND_BOX && remote->boundType != Remote::BoundType::E_BOUND_SPHERE) return false;
	return WeaponReach(*remote, wf) <= WEAPON_MELEE_MAX_RANGE;
}

f32 World::AugmentedWeaponAttack(WeaponIndex weapon) const
{
	return GetGameXmlContent().GetWeaponAttack((i32)weapon);
}

f32 World::WeaponHitDamage(const Player& player, const Remote* remote) const
{
	const WeaponIndex weapon = ActiveWeapon(player);
	const f32 attack = AugmentedWeaponAttack(weapon) + StatBonus(player, STAT_ATTACK);
	ASSERT(attack > 0);
	const f32 mult = remote && remote->attackMultiplier > 0 ? remote->attackMultiplier : 1.0f;
	if(remote && remote->skillIndex != 0 && remote->attackMultiplier > 0) return PlayerSkillDamage(player, SkillID(remote->skillIndex)) * mult;
	return attack * mult * MAX(1.0f + StatusModMulSum(player, StatusMod::ATTACK) / 100.0f, 0.0f);
}

void World::RunWeaponActionStatuses(Player& player, i8 stance, const vec3* aimPos)
{
	const auto& content = GetGameXmlContent();
	const ActionStateID state = ActiveWeaponFiring(player, stance).AttackState(player.firingCombo, player.chargeLevel);
	const GameXmlContent::Action* action = content.FindSkillAction(ActionClass(player), state, stance);
	if(!action) return;
	foreach_const(cmd, action->commands) {
		if(aimPos && cmd->type == ActionCommand::Type::REMOTE && cmd->remote.targetPreset == ActionCommand::TargetPreset::POS_TTT_1 && !cmd->remote.remove && !pendingRemotes.full()) {
			PendingRemote pr;
			pr.casterMasterUID = player.Main().UID;
			pr.time = TimeAdd(localTime, TimeMsToTime(cmd->delay * 1000.0));
			pr.idx = cmd->remote.idx;
			pr.preset = (i32)cmd->remote.targetPreset;
			pr.offsetSide = 0;
			pr.offsetFront = 0;
			pr.ownerIndex = player.index;
			pr.skillID = SkillID::INVALID;
			pr.castID = nextCastID++;
			pr.damage = 0;
			pr.castPos = *aimPos;
			pr.angle = atan2f(aimPos->y - player.body->GetWorldPos().y, aimPos->x - player.body->GetWorldPos().x);
			pr.target = ActorUID::INVALID;
			pendingRemotes.push_back(pr);
			continue;
		}
		if(cmd->type != ActionCommand::Type::STATUS || cmd->status.statusIndex == 0) continue;
		const auto preset = cmd->status.targetPreset;
		if(preset != ActionCommand::TargetPreset::SELF_FFF && preset != ActionCommand::TargetPreset::SELF_TFF) continue;
		if(cmd->status.remove) {
			if(cmd->delay > 0) continue;
			foreach(st, activeStatuses) if(st->targetIndex == player.index && st->statusID == cmd->status.statusIndex && localTime < st->until) st->until = localTime;
			continue;
		}
		f32 duration = -1.0f;
		if(cmd->status.removeAtEnd) {
			i32 animId = 0, loop = 1;
			foreach_const(st, action->anims) if(st->index == MAX(cmd->animIndex, 0)) { animId = st->animationId; loop = MAX(st->loop, 1); }
			duration = animId != 0 ? content.GetAniSeqTime(ActionClassName(player), animId) * (f32)loop : action->seqLength;
			if(duration <= 0) continue;
			const StatusInfo* capInfo = content.FindStatus(cmd->status.statusIndex);
			if(capInfo && capInfo->durationSec > 0) duration = MIN(duration, capInfo->durationSec);
		}
		if(!player.dead) ApplyStatus(player, player, cmd->status.statusIndex, 0, duration);
		if(cmd->status.removeAtEnd && eastl::find(player.weaponAniEndStatuses.begin(), player.weaponAniEndStatuses.end(), cmd->status.statusIndex) == player.weaponAniEndStatuses.end() && !player.weaponAniEndStatuses.full()) player.weaponAniEndStatuses.push_back(cmd->status.statusIndex);
	}
}

f32 World::WeaponHeatScale(const Player& player) const
{
	f32 scale = 1.0f;
	foreach_const(it, activeStatuses) {
		if(it->targetIndex != player.index || !it->info || it->info->type != StatusType::WEAPON_GAUGE || it->until <= localTime) continue;
		if(it->info->paramRatio > 0) scale *= it->info->paramRatio;
	}
	return scale;
}

void World::RecoverWeaponHeat(Player& player, f32 amount)
{
	const WeaponHeatInfo* h = ActiveWeaponHeat(player);
	if(!h || amount <= 0) return;
	const u32 m = player.mainCharaID & 1;
	player.heat[m] = MAX(CurrentHeat(player, *h, localTime) - amount, 0.0f);
	player.heatLastShot[m] = localTime;
	if(player.overheatUntil[m] != Time::ZERO && player.heat[m] < h->maxGauge) player.overheatUntil[m] = localTime;
	LOG("Overheat :: %ls weapon gauge recovered %.0f", player.name.data(), amount);
}

f32 World::WeaponFireDelayScale(const Player& player) const
{
	f32 scale = 1.0f;
	foreach_const(it, activeStatuses) {
		if(it->targetIndex != player.index || !it->info || it->info->type != StatusType::FIREDELAY || it->until <= localTime) continue;
		if(it->info->timeRatio > 0) scale *= it->info->timeRatio;
	}
	for(const LowHpFireRateRule& rule : MEASURED_LOW_HP_FIRE_RATES) {
		if(rule.master != player.Main().classType) continue;
		const ActiveStatus* st = FindPassiveOfType(player, StatusType::HPTRIGGER);
		if(st && player.Main().hp * 100 < player.Main().hpMax * st->info->Param("_CheckVal")) scale *= rule.fireDelayScale;
	}
	return scale;
}

World::RemoteInst* World::FindProjectileReflector(const Projectile& proj, const vec2& from, const vec2& to, vec2* contact)
{
	const Player& shooter = players[proj.ownerIndex];
	const f32 len = glm::length(to - from);
	const i32 steps = MAX((i32)(len / REFLECT_SAMPLE_STEP), 1);
	foreach(inst, remoteInsts) {
		if(!inst->remote || !inst->remote->vsRemote || inst->killNow) continue;
		const Player& owner = players[inst->ownerIndex];
		if(owner.team == shooter.team || owner.dead) continue;
		if(fabsf(inst->pos.z - proj.pos.z) > WEAPON_PROJECTILE_HIT_HEIGHT + FIRE_NODE_HEIGHT) continue;
		const Remote& r = *inst->remote;
		for(i32 k = 0; k <= steps; k++) {
			const vec2 p = from + (to - from) * ((f32)k / (f32)steps);
			const vec2 rel = p - vec2(inst->pos) - inst->dir * RemoteBoundOffset(r);
			bool inside;
			if(r.boundType == Remote::BoundType::E_BOUND_SPHERE) inside = glm::length(rel) <= r.boundSize[0] * 0.5f + proj.halfWidth;
			else {
				const f32 length = r.boundSize[1];
				const vec2 local = rel + inst->dir * (length * 0.5f);
				const f32 along = glm::dot(local, inst->dir);
				inside = along >= -proj.halfWidth && along <= length + proj.halfWidth && fabsf(glm::dot(local, Perp(inst->dir))) <= r.boundSize[0] * 0.5f + proj.halfWidth;
			}
			if(!inside) continue;
			*contact = p;
			return &*inst;
		}
	}
	return nullptr;
}

const World::MonsterRemote* World::FindMonsterProjectileBlocker(const Projectile& proj, const vec2& from, const vec2& to)
{
	const Player& shooter = players[proj.ownerIndex];
	const f32 len = glm::length(to - from);
	const i32 steps = MAX((i32)(len / REFLECT_SAMPLE_STEP), 1);
	foreach_const(mr, monsterRemotes) {
		if(!mr->remote || !mr->remote->vsRemote || localTime >= mr->expire) continue;
		const ActorNpc* owner = FindNpcActor(mr->owner);
		if(!owner || owner->hp <= 0 || (owner->faction != Faction::RED && owner->faction != Faction::BLUE) || owner->faction == (Faction)shooter.team) continue;
		const Remote& r = *mr->remote;
		for(i32 k = 0; k <= steps; k++) {
			const vec2 p = from + (to - from) * ((f32)k / (f32)steps);
			const vec2 rel = p - vec2(mr->pos) - mr->dir * RemoteBoundOffset(r);
			bool inside;
			if(r.boundType == Remote::BoundType::E_BOUND_SPHERE) inside = glm::length(rel) <= r.boundSize[0] * 0.5f + proj.halfWidth;
			else {
				const f32 length = r.boundSize[1];
				const vec2 local = rel + mr->dir * (length * 0.5f);
				const f32 along = glm::dot(local, mr->dir);
				inside = along >= -proj.halfWidth && along <= length + proj.halfWidth && fabsf(glm::dot(local, Perp(mr->dir))) <= r.boundSize[0] * 0.5f + proj.halfWidth;
			}
			if(inside) return &*mr;
		}
	}
	return nullptr;
}

void World::ReflectProjectile(const Projectile& src, RemoteInst& reflector, const vec2& contact)
{
	if(projectileList.full()) return;
	Player& r = players[reflector.ownerIndex];
	const Remote* doc = GetGameXmlContent().FindRemote(src.remoteDoc);
	if(!doc) return;
	const f32 yaw = reflector.remote->reflectBack ? atan2f(-src.dir.y, -src.dir.x) : r.input.rot.upperYaw;
	Projectile p = src;
	p.reflected = true;
	p.ownerIndex = r.index;
	p.dir = vec2(cosf(yaw), sinf(yaw));
	p.pos = vec3(contact, src.pos.z);
	p.start = p.pos;
	p.travelled = 0;
	p.range = doc->maxDistance > 0 ? doc->maxDistance : (doc->lifeTime > 0 ? doc->lifeTime * p.speed : src.range);
	p.wallZ = r.body->GetWorldPos().z + BodyMidHeight(r);
	p.hitList.clear();
	p.npcHitList.clear();
	p.hitsLeft = MAX(doc->activateCount, 1);
	p.penetrationLeft = doc->penetrationCount > 0 ? doc->penetrationCount : PENETRATION_UNLIMITED;
	p.damage = (i32)(WeaponHitDamage(r, doc) * reflector.remote->reflectDamage);
	p.aoeRadius = 0;
	p.bombDoc = RemoteIdx::INVALID;
	p.zoneRemote = RemoteIdx::INVALID;
	p.chainRemote = RemoteIdx::INVALID;
	p.pendingAdvance = 0;
	const u32 seed = nextRemoteSeed;
	nextRemoteSeed += 2;
	p.remoteID = seed + 1;
	p.seedOwnerHd = ClientHandle::INVALID;
	if(reflector.reflectGroupSeed == 0) reflector.reflectGroupSeed = seed;
	replication->SendReflectedShotToAll(players[src.ownerIndex].Main().UID, r.Main().UID, seed, p.pos + vec3(0, 0, FIRE_NODE_HEIGHT), yaw, GetGameXmlContent().GetNodeName(src.nodeName), reflector.reflectGroupSeed, src.remoteDoc);
	LOG("Shot :: %ls reflects remote %d with %d", r.name.data(), (i32)src.remoteDoc, (i32)reflector.remote->ID);
	projectileList.push_back(p);
}

u16 World::DefaultWeaponCreator(const Player& player, const WeaponFiring& wf) const
{
	return GetGameXmlContent().FindFiringCreator(wf.firingMethod);
}

f32 World::OwnerRemoteSpeedRatio(const Player& player) const
{
	return MAX(1.0f + StatusModSum(player, StatusMod::REMOTESPEED) / 100.0f, 0.1f);
}
