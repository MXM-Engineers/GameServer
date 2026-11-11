#include "config.h"
#include "world.h"
#include "world_common.h"
#include <mxm/game_content.h>

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
	if(remote.boundType == Remote::BoundType::E_BOUND_SPHERE) return remote.offsetY + remote.boundSize[0] * 0.5f;
	if(remote.boundType != Remote::BoundType::E_BOUND_NONE) return remote.offsetY + remote.boundSize[1] * 0.5f;
	return wf.maxDistance;
}

static f32 SegmentPointDist2D(const vec2& a, const vec2& b, const vec2& p)
{
	const vec2 ab = b - a;
	const f32 len2 = glm::dot(ab, ab);
	f32 t = len2 > 0 ? glm::dot(p - a, ab) / len2 : 0;
	t = clamp(t, 0.0f, 1.0f);
	return glm::length(p - (a + ab * t));
}

static void BotWeaponAnimation(const World::Player& player, Replication* replication)
{
	if(player.clientHd != ClientHandle::INVALID) return;
	const WeaponIndex weapon = (player.mainCharaID & 1) == 0 ? player.mainWeapon : player.subWeapon;
	replication->RelayWeaponState(ClientHandle::INVALID, player.Main().UID, (i32)weapon, 2, player.chargeLevel, player.firingCombo);
}

void World::PlayerWeaponFire(Player& player)
{
	BotWeaponAnimation(player, replication);
	const vec2 dir = vec2(cosf(player.weaponAimYaw), sinf(player.weaponAimYaw));
	const vec3 origin = player.body->GetWorldPos();

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
		const Remote* swingRemote = GetGameXmlContent().FindRemote(swing);
		ASSERT(swingRemote);
		LOG("Shot :: %ls melee swing remote %d (stance %d)", player.name.data(), (i32)swing, (i32)swingStance);
		SpawnRemote(player.index, swing, origin + vec3(dir * swingRemote->offsetY, 0), dir, ActorUID::INVALID, SkillID::INVALID, nextCastID++, (i32)WeaponHitDamage(player, nullptr), true, nullptr, 0, player.clientHd != ClientHandle::INVALID);
	}
}

void World::PlayerWeaponShot(Player& player, const vec3& origin, const vec2& dir, f32 distance)
{
	if(player.dead) return;
	const ClassType classType = player.Main().classType;
	if(WeaponIsMelee(player)) return;
	BotWeaponAnimation(player, replication);
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
	const Remote* remote = content.FindRemote(idx);
	ASSERT(remote);
	LOG("Shot :: %ls firing remote %d (combo %d charge %d count %d)", player.name.data(), (i32)idx, player.firingCombo, player.chargeLevel, player.shotCount[player.mainCharaID & 1]);

	const Remote* bomb = remote ? content.FindRemote(remote->destroyCreateRemote) : nullptr;
	if(spawnAtAim && remote) {
		f32 reach = MAX(MIN(distance > 1.0f ? distance : WeaponReach(*remote, wf), aimMax), aimMin);
		const vec3 aimPos = vec3(vec2(origin) + dir * reach, player.body->GetWorldPos().z);
		replication->SendAttackActionToOthers(player.clientHd, player.Main().UID, WorldYawToMxmYaw(atan2f(dir.y, dir.x)));
		const bool bot = player.clientHd == ClientHandle::INVALID;
		if(!bot && player.pendingShotRawSize > 0) {
			replication->RelayWeaponShot(player.clientHd, player.pendingShotRaw.data(), player.pendingShotRawSize, aimPos, remote->ID);
			player.pendingShotRawSize = 0;
		}
		SpawnRemote(player.index, remote->ID, aimPos, dir, ActorUID::INVALID, SkillID::INVALID, nextCastID++, (i32)WeaponHitDamage(player, nullptr), bot);
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

	proj.range = graph ? (reliable ? MIN(distance, aimMax) : MIN(classRange, aimMax)) : remoteRange;
	proj.graph = graph;
	proj.ignoreWalls = !remote || remote->ignoreGround;
	proj.wallZ = player.body->GetWorldPos().z + BodyMidHeight(player);
	proj.halfWidth = remote ? remote->boundSize[0] * 0.5f : 20.0f;
	proj.hitsLeft = remote ? MAX(remote->activateCount, 1) : 1;
	proj.forwardReach = (remote && remote->boundType == Remote::BoundType::E_BOUND_BOX) ? MAX(remote->offsetY + remote->boundSize[1] * 0.5f, 0.0f) : 0.0f;
	proj.aoeRadius = bomb ? bomb->boundSize[0] * 0.5f : 0;
	if(remote && remote->behaviorType == Remote::BehaviorType::HOMING && remote->angularVel > 0) {
		proj.angularVel = remote->angularVel;
		proj.homingLength = remote->homingLength;
		proj.homingHalfCos = (remote->homingAngle > 0 && remote->homingAngle < 360) ? cosf(remote->homingAngle * 0.5f * PI / 180.0f) : -1.0f;
		proj.homingTime = remote->homingTime;
		proj.homingTimer = remote->homingTime;
		proj.homingLockOn = remote->homingLockOn;
	}

	proj.remoteDoc = remote ? remote->ID : RemoteIdx::INVALID;
	proj.pendingAdvance = player.shotLeadSec * proj.speed;
	if(player.pendingShotRawSize >= (i32)offsetof(Cl::CQ_GamePlayerSyncPhy, FirePosition)) {
		proj.remoteID = ((const Cl::CQ_GamePlayerSyncPhy*)player.pendingShotRaw.data())->RemoteSeedID + 1;
	}
	replication->SendAttackActionToOthers(player.clientHd, player.Main().UID, WorldYawToMxmYaw(atan2f(proj.dir.y, proj.dir.x)));

	if(player.pendingShotRawSize > 0) {
		const vec3 target = vec3(vec2(origin) + proj.dir * proj.range, origin.z);
		replication->RelayWeaponShot(player.clientHd, player.pendingShotRaw.data(), player.pendingShotRawSize, target, remote ? remote->ID : RemoteIdx::INVALID);
		player.pendingShotRawSize = 0;
	}
	else if(player.clientHd == ClientHandle::INVALID && remote) {
		const u32 seed = nextRemoteSeed;
		nextRemoteSeed += 2;
		proj.remoteID = seed + 1;
		const vec3 firePos = proj.pos + vec3(0, 0, 158.0f);
		const vec3 target = firePos + vec3(proj.dir * proj.range, 0);
		replication->SendRemoteCreateToAll(player.Main().UID, seed, remote->ID, firePos, atan2f(proj.dir.y, proj.dir.x), target, ActorUID::INVALID, L"Attack_Fire_Dummy");
	}
	proj.damage = bomb && remote->attackMultiplier <= 0 ? 0 : (i32)WeaponHitDamage(player, remote);
	proj.aoeDamage = (i32)WeaponHitDamage(player, bomb && bomb->attackMultiplier > 0 ? bomb : remote);
	if(bomb && bomb->timeEventDelay >= 0) {
		proj.zoneRemote = bomb->ID;
		proj.zoneDamage = (i32)WeaponHitDamage(player, nullptr);
	}
	LOG("Shot :: %ls class=%d remote=%d graph=%d reliable=%d dist=%.0f range=%.0f speed=%.0f width=%.0f hits=%d aoe=%.0f", player.name.data(), (i32)classType, remote ? (i32)remote->ID : -1, graph, reliable, distance, proj.range, proj.speed, proj.halfWidth, proj.hitsLeft, proj.aoeRadius);
	projectileList.push_back(proj);
}

void World::Explode(Player& owner, const vec3& center, f32 radius, i32 damage)
{
	LOG("Explosion :: at (%.0f, %.0f) radius=%.0f", center.x, center.y, radius);
	i32 mastersHit = 0;
	foreach(it, players) {
		if(it->team == owner.team || it->dead || it->untargetable) continue;
		const vec3 pos = it->body->GetWorldPos();
		if(fabsf(pos.z - center.z) > WEAPON_PROJECTILE_HIT_HEIGHT) continue;
		if(LagDistToPoint(*it, vec2(center)) > radius + HitRadius(*it) + WEAPON_HIT_TOLERANCE) continue;
		DamagePlayer(owner, *it, SkillID::INVALID, damage);
		mastersHit++;
	}
	if(mastersHit >= 2 && owner.mana >= 0) {
		if(const ActiveStatus* st = FindPassiveOfType(owner, StatusType::ARTILLERY)) {
			owner.mana = MIN(owner.mana + st->info->Param("_AddValuePC"), MasterStat(owner.Main().classType, STAT_MANA));
			owner.tNextManaSend = Time::ZERO;
			LOG("Passive :: %ls hit %d masters -> mana %.0f", owner.name.data(), mastersHit, owner.mana);
		}
	}
}

void World::SteerProjectile(Projectile& proj, f32 delta)
{
	const Player& owner = players[proj.ownerIndex];
	const auto targetable = [&](const Player& p) { return p.team != owner.team && !p.dead && !p.untargetable; };
	if(proj.homingTarget >= 0 && !targetable(players[proj.homingTarget])) proj.homingTarget = -1;
	proj.homingTimer += delta;
	if(proj.homingTimer >= proj.homingTime && !(proj.homingLockOn && proj.homingTarget >= 0)) {
		proj.homingTimer = 0;
		f32 bestDist = proj.homingLength;
		i32 found = -1;
		foreach_const(it, players) {
			if(!targetable(*it)) continue;
			const vec2 rel = vec2(it->body->GetWorldPos() - proj.pos);
			const f32 d = glm::length(rel);
			if(d > bestDist) continue;
			if(d > 1.0f && glm::dot(rel / d, proj.dir) < proj.homingHalfCos) continue;
			bestDist = d;
			found = (i32)it->index;
		}
		if(found >= 0 && found != proj.homingTarget) LOG("Shot :: homing projectile locked %ls at %.0f", players[found].name.data(), bestDist);
		proj.homingTarget = found;
	}
	if(proj.homingTarget < 0) return;
	const vec2 rel = vec2(players[proj.homingTarget].body->GetWorldPos() - proj.pos);
	if(glm::length(rel) < 1.0f) return;
	f32 turn = atan2f(rel.y, rel.x) - atan2f(proj.dir.y, proj.dir.x);
	while(turn > PI) turn -= 2 * PI;
	while(turn < -PI) turn += 2 * PI;
	const f32 maxTurn = proj.angularVel * delta;
	turn = MAX(MIN(turn, maxTurn), -maxTurn);
	const f32 c = cosf(turn);
	const f32 sn = sinf(turn);
	proj.dir = vec2(proj.dir.x * c - proj.dir.y * sn, proj.dir.x * sn + proj.dir.y * c);
}

void World::UpdateProjectiles(f32 delta)
{
	for(auto proj = projectileList.begin(); proj != projectileList.end(); ) {
		Player& owner = players[proj->ownerIndex];
		if(proj->angularVel > 0) SteerProjectile(*proj, delta);
		f32 step = MIN(proj->speed * delta + proj->pendingAdvance, proj->range - proj->travelled);
		proj->pendingAdvance = 0;
		bool hitWall = false;
		if(!proj->graph && !proj->ignoreWalls && step > 0) {
			f32 wallDist = 0;
			const vec2 a = vec2(proj->pos);
			if(physics.RaycastStatic(vec3(a, proj->wallZ), vec3(a + proj->dir * step, proj->wallZ), &wallDist, (i32)players[proj->ownerIndex].team)) {
				step = wallDist;
				hitWall = true;
			}
		}
		const bool lastStep = hitWall || proj->travelled + step >= proj->range - 0.01f;
		const vec2 from = vec2(proj->pos);
		const vec2 to = from + proj->dir * step;
		bool destroyed = false;

		if(!proj->graph) {
			foreach(it, players) {
				if(it->team == owner.team || it->dead || it->untargetable) continue;
				if(eastl::find(proj->hitList.begin(), proj->hitList.end(), it->index) != proj->hitList.end()) continue;
				const vec3 pos = it->body->GetWorldPos();
				const f32 lagDist = LagDistToSegment(*it, from, to);
				proj->closest = MIN(proj->closest, lagDist);
				if(fabsf(pos.z - proj->pos.z) > WEAPON_PROJECTILE_HIT_HEIGHT) continue;
				if(!LagSegmentHits(*it, from, to, proj->halfWidth + HitRadius(*it) + PROJECTILE_HIT_TOLERANCE, lastStep ? proj->forwardReach + HitRadius(*it) : HitRadius(*it))) continue;

				if(!proj->hitList.full()) proj->hitList.push_back(it->index);
				if(proj->damage > 0) DamagePlayer(owner, *it, SkillID::INVALID, (i32)(proj->damage * PassiveWeaponDamageMul(owner, *it)), &*proj);
				PassiveOnWeaponHit(owner, *it, *proj);
				proj->hitsLeft--;
				if(proj->hitsLeft <= 0) {
					if(proj->remoteID != 0) replication->SendDestroyRemoteToAll(proj->remoteID);
					proj->pos = vec3(vec2(pos), proj->pos.z);
					destroyed = true;
					break;
				}
			}
		}

		if(!destroyed) {
			proj->pos += vec3(proj->dir * step, 0);
			proj->travelled += step;
			destroyed = hitWall || proj->travelled >= proj->range - 0.01f;
			if(hitWall) LOG("Shot :: projectile stopped by a wall after %.0f", proj->travelled);
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
				SpawnRemote(proj->ownerIndex, proj->zoneRemote, proj->pos, proj->dir, ActorUID::INVALID, SkillID::INVALID, nextCastID++, proj->zoneDamage, false);
			}
			else if(proj->aoeRadius > 0) {
				Explode(owner, proj->pos, proj->aoeRadius, proj->aoeDamage);
			}
			else if(proj->graph && proj->hitsLeft > 0) {
				Explode(owner, proj->pos, proj->halfWidth, proj->aoeDamage);
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
		replication->SendGaugeToAll(player.userID, 1, false, 0, (u16)player.heat[m], (u16)h->maxGauge);
		player.heatSent = (i32)player.heat[m];
		LOG("Overheat :: %ls weapon cooled down", player.name.data());
	}

	if(localTime < player.tNextHeatSend) return;
	player.tNextHeatSend = TimeAdd(localTime, TimeMsToTime(100));
	const i32 cur = (i32)CurrentHeat(player, *h, localTime);
	if(cur == player.heatSent) return;
	player.heatSent = cur;
	const bool overheated = player.overheatUntil[m] != Time::ZERO;
	replication->SendGaugeToAll(player.userID, 0, overheated, 0, (u16)cur, (u16)h->maxGauge);
}

void World::OnWeaponShotPacket(Player& player, const vec3& origin, const vec2& dir, f32 distance, const u8* rawPacket, i32 rawSize)
{
	if(frozen) return;
	if(IsBlocked(player, true, false, false)) {
		LOG("Blocked :: %ls shot ignored (attack blocked by a status)", player.name.data());
		return;
	}
	if(!WeaponHeatShot(player, localTime, replication)) {
		LOG("Overheat :: %ls shot ignored (weapon overheated)", player.name.data());
		return;
	}

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

const WeaponFiring& World::ActiveWeaponFiring(const Player& p, i8 stance) const
{
	const WeaponIndex weapon = (p.mainCharaID & 1) == 0 ? p.mainWeapon : p.subWeapon;
	const WeaponFiring* wf = GetGameXmlContent().FindWeaponFiring((i32)weapon, stance);
	ASSERT(wf);
	return *wf;
}

RemoteIdx World::SelectWeaponRemote(Player& player, const WeaponFiring& wf)
{
	const u32 m = player.mainCharaID & 1;
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
	if(state == WEAPON_STATE_CHARGE && player.weaponState != WEAPON_STATE_CHARGE) player.chargeStart = localTime;
	if(state == WEAPON_STATE_FIRE && player.weaponState == WEAPON_STATE_CHARGE && chargeLevel == 0) {
		const WeaponFiring& wf = ActiveWeaponFiring(player, player.Main().stance);
		const f64 held = TimeDiffSec(TimeDiff(player.chargeStart, localTime));
		while(chargeLevel < wf.chargeTime.size() && chargeLevel < wf.charge.size() && held >= wf.chargeTime[chargeLevel]) chargeLevel++;
		if(chargeLevel > 0) LOG("Shot :: %ls charged %.2fs -> level %d", player.name.data(), held, (i32)chargeLevel);
	}
	player.firingCombo = firingCombo;
	player.chargeLevel = chargeLevel;
	player.weaponState = state;
}

f32 World::WeaponRange(const Player& p) const
{
	const WeaponFiring& wf = ActiveWeaponFiring(p, p.Main().stance);
	const Remote* remote = GetGameXmlContent().FindRemote(wf.base);
	ASSERT(remote);
	return WeaponReach(*remote, wf);
}

bool World::WeaponIsMelee(const Player& p) const
{
	const WeaponFiring& wf = ActiveWeaponFiring(p, p.Main().stance);
	const Remote* remote = GetGameXmlContent().FindRemote(wf.base);
	ASSERT(remote);
	if(remote->maxSpeed > 0 || remote->behaviorType != Remote::BehaviorType::NONETARGET || wf.spawnAtAim) return false;
	if(remote->boundType != Remote::BoundType::E_BOUND_BOX && remote->boundType != Remote::BoundType::E_BOUND_SPHERE) return false;
	return WeaponReach(*remote, wf) <= WEAPON_MELEE_MAX_RANGE;
}

f32 World::WeaponHitDamage(const Player& player, const Remote* remote) const
{
	const u32 m = player.mainCharaID & 1;
	const WeaponIndex weapon = m == 0 ? player.mainWeapon : player.subWeapon;
	const f32 attack = GetGameXmlContent().GetWeaponAttack((i32)weapon);
	ASSERT(attack > 0);
	const f32 mult = remote && remote->attackMultiplier > 0 ? remote->attackMultiplier : 1.0f;
	return attack * mult * MAX(1.0f + StatusModMulSum(player, StatusMod::ATTACK) / 100.0f, 0.0f);
}
