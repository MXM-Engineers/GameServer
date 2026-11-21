#include "config.h"
#include "world.h"
#include "world_common.h"
#include <mxm/game_content.h>

static const u32 MEASURED_TRAP_REMOTE_ID_BASE = 0xC0000000u;
static const u32 TRAP_REMOTE_ID_MASK = 0x3FFFFFFFu;
static const f32 MEASURED_TRAP_NODE_YAW_OFFSET = (f32)PI;
static const f32 MEASURED_TRAP_CREATE_YAW_OFFSET = 0.0f;
static const i32 MEASURED_TRAP_FIRE_YAW_CENTIDEG_OFFSET = -18000;
static const i32 CENTIDEG_HALF_TURN = 18000;
static const i32 CENTIDEG_TURN = 36000;
static const f32 HYP_TRAP_NPC_HEIGHT = 200.0f;
static const f32 HYP_TRAP_MIN_LOOP_SEC = 0.05f;
static const i32 HYP_TRAP_MAX_SUBSTEPS = 16;
static const i32 TRAP_ROLL_STEPS = 10000;
static const i32 STATUS_RATE_ALWAYS = 100;
static const f32 TRAP_EPSILON = 1e-4f;
static const u32 TRAP_MAX_PLAYER_BITS = 16;
static const i32 TRAP_KEEP_TRACK = -2;

static vec2 TrapRotate(f32 yaw, const vec2& v)
{
	const f32 c = cosf(yaw);
	const f32 s = sinf(yaw);
	return vec2(v.x * c + v.y * s, -v.x * s + v.y * c);
}

static vec3 TrapTrackPoint(const GameXmlContent::DynamicTrapTrack& track, f32 step, f32 t)
{
	const i32 count = (i32)track.points.size();
	if(count == 1 || step <= 0) return track.points[0];
	const f32 f = MAX(t, 0.0f) / step;
	const i32 i = (i32)f;
	if(i >= count - 1) return track.points[count - 1];
	return track.points[i] + (track.points[i + 1] - track.points[i]) * (f - (f32)i);
}

static vec2 TrapTrackDir(const GameXmlContent::DynamicTrapTrack& track, f32 step, f32 t)
{
	const i32 count = (i32)track.dirs.size();
	if(count == 0) return vec2(0, 1);
	vec2 d = track.dirs[0];
	if(count > 1 && step > 0) {
		const f32 f = MAX(t, 0.0f) / step;
		const i32 i = (i32)f;
		d = i >= count - 1 ? track.dirs[count - 1] : track.dirs[i] + (track.dirs[i + 1] - track.dirs[i]) * (f - (f32)i);
	}
	const f32 len = glm::length(d);
	return len > TRAP_EPSILON ? d / len : vec2(0, 1);
}

static f32 TrapAnimTime(const GameXmlContent::DynamicTrapState& st, f32 elapsed)
{
	if(!st.loop) return MIN(elapsed, st.length);
	return fmodf(MAX(elapsed, 0.0f), MAX(st.length, HYP_TRAP_MIN_LOOP_SEC));
}

static vec3 TrapNodeLocal(const World::ActorDynamic& dyn, i32 track, f32 animTime)
{
	if(track < 0) return vec3(0);
	return TrapTrackPoint(dyn.trap->model->tracks[track], GetGameXmlContent().dynamicTrapStep, animTime);
}

static vec2 TrapNodeDir(const World::ActorDynamic& dyn, i32 track, f32 animTime)
{
	if(track < 0) return vec2(0, 1);
	return TrapTrackDir(dyn.trap->model->tracks[track], GetGameXmlContent().dynamicTrapStep, animTime);
}

static bool TrapRemoved(const GameXmlContent::DynamicTrapState& st, const Remote& remote)
{
	foreach_const(d, st.removes) if(*d == (i32)remote.ID) return true;
	return false;
}

static vec3 TrapNodeWorld(const World::ActorDynamic& dyn, const vec3& local, f32 yawOffset)
{
	const vec2 offset = TrapRotate(dyn.rot.z + yawOffset, vec2(local));
	return dyn.pos + vec3(offset.x, offset.y, local.z);
}

static i16 TrapFireYawCentiDeg(f32 yaw)
{
	i32 cd = (i32)floorf(yaw * 180.0f / (f32)PI * 100.0f + 0.5f) + MEASURED_TRAP_FIRE_YAW_CENTIDEG_OFFSET;
	while(cd < -CENTIDEG_HALF_TURN) cd += CENTIDEG_TURN;
	while(cd >= CENTIDEG_HALF_TURN) cd -= CENTIDEG_TURN;
	return (i16)cd;
}

static bool TrapClipAxis(f32 a0, f32 a1, f32 half, f32& lo, f32& hi)
{
	const f32 d = a1 - a0;
	if(fabsf(d) < TRAP_EPSILON) return fabsf(a0) <= half;
	const f32 s0 = (-half - a0) / d;
	const f32 s1 = (half - a0) / d;
	lo = MAX(lo, MIN(s0, s1));
	hi = MIN(hi, MAX(s0, s1));
	return lo <= hi;
}

static bool TrapShapeHit(const Remote& remote, const vec3& from, const vec3& to, const vec2& dir, const vec3& target, f32 radius, f32 height)
{
	const bool sphere = remote.boundType == Remote::BoundType::E_BOUND_SPHERE;
	const f32 halfZ = sphere ? remote.boundSize[0] * 0.5f : remote.boundSize[2] > 0 ? remote.boundSize[2] * 0.5f : WEAPON_PROJECTILE_HIT_HEIGHT;
	if(target.z > MAX(from.z, to.z) + halfZ || target.z + height < MIN(from.z, to.z) - halfZ) return false;
	const vec2 offset = dir * RemoteBoundOffset(remote);
	const vec2 p0 = vec2(from) + offset;
	const vec2 p1 = vec2(to) + offset;
	const vec2 t = vec2(target);
	if(sphere) {
		const vec2 seg = p1 - p0;
		const f32 len2 = glm::dot(seg, seg);
		const f32 s = len2 > TRAP_EPSILON ? MIN(MAX(glm::dot(t - p0, seg) / len2, 0.0f), 1.0f) : 0.0f;
		return glm::length(t - (p0 + seg * s)) <= remote.boundSize[0] * 0.5f + radius;
	}
	const vec2 side = Perp(dir);
	f32 lo = 0.0f;
	f32 hi = 1.0f;
	if(!TrapClipAxis(glm::dot(t - p0, dir), glm::dot(t - p1, dir), remote.boundSize[1] * 0.5f + radius, lo, hi)) return false;
	return TrapClipAxis(glm::dot(t - p0, side), glm::dot(t - p1, side), remote.boundSize[0] * 0.5f + radius, lo, hi);
}

static i32 TrapRemoteFor(const GameXmlContent::DynamicTrapState& st, u8 node, u32 activeRemotes, u32 vsBit)
{
	for(i32 i = 0; i < (i32)st.remotes.size(); i++) {
		if(!(activeRemotes & (1u << i)) || st.remotes[i].node != node) continue;
		if(st.remotes[i].remote->vs & (1u << vsBit)) return i;
	}
	return -1;
}

static bool TrapHitPending(const World::ActorDynamic& dyn, u8 node, ActorUID target)
{
	foreach_const(h, dyn.trapHits) if(h->node == node && h->target == target) return true;
	return false;
}

void World::UpdateDynamicTraps()
{
	if(!hostileMonsters || gameStartedAt == Time::ZERO) return;
	const GameXmlContent& content = GetGameXmlContent();
	if(content.dynamicTraps.empty()) return;
	eastl::fixed_vector<PendingTrapHit,32,true> hits;
	foreach(it, actorDynamicList) {
		ActorDynamic& dyn = *it;
		if(!dyn.trapChecked) {
			dyn.trapChecked = true;
			dyn.trap = content.FindDynamicTrap((i32)dyn.docID);
		}
		if(dyn.trap) UpdateDynamicTrap(dyn, hits);
	}
	foreach_const(h, hits) ApplyDynamicTrapHit(*h);
}

void World::StartDynamicTrap(ActorDynamic& dyn)
{
	const GameXmlContent::DynamicTrapState& st = *dyn.trapState;
	dyn.trapStart = dyn.tLastActionChange > gameStartedAt ? dyn.tLastActionChange : gameStartedAt;
	dyn.trapPrevElapsed = -1.0f;
	dyn.trapSlots.clear();
	dyn.trapSlots.resize(st.remotes.size());
	dyn.trapHits.clear();
	dyn.trapMotion = nullptr;
	dyn.trapCarried = 0;
	dyn.trapMotionTracks.clear();
	LOG("Trap :: dynamic %d (doc %d) at (%.0f, %.0f, %.0f) rot %.3f starts state %d (%d node(s), %d remote(s), loop %.3f s) t=%.2f",
		dyn.localID, (i32)dyn.docID, dyn.pos.x, dyn.pos.y, dyn.pos.z, dyn.rot.z, (i32)st.action, (i32)st.nodes.size(), (i32)st.remotes.size(), st.length,
		TimeDiffSec(TimeDiff(gameStartedAt, dyn.trapStart)));
}

void World::StopDynamicTrap(ActorDynamic& dyn)
{
	const GameXmlContent::DynamicTrapState& st = *dyn.trapState;
	for(i32 i = 0; i < (i32)dyn.trapSlots.size() && i < (i32)st.remotes.size(); i++) {
		if(dyn.trapSlots[i].seed != 0 && st.remotes[i].life <= 0) replication->SendDestroyRemoteToAll(dyn.trapSlots[i].seed);
	}
	LOG("Trap :: dynamic %d (doc %d) leaves state %d", dyn.localID, (i32)dyn.docID, (i32)st.action);
	dyn.trapSlots.clear();
	dyn.trapHits.clear();
	dyn.trapState = nullptr;
	dyn.trapMotion = nullptr;
	dyn.trapCarried = 0;
	dyn.trapMotionTracks.clear();
}

bool World::CarryDynamicTrap(ActorDynamic& dyn, const GameXmlContent::DynamicTrapState& next)
{
	const GameXmlContent::DynamicTrapState& st = *dyn.trapState;
	const u32 alive = dyn.trapMotion ? dyn.trapCarried : ~0u;
	const i32 count = MIN((i32)st.remotes.size(), (i32)dyn.trapSlots.size());
	u32 carried = 0;
	i32 carriedCount = 0;
	for(i32 i = 0; i < count; i++) {
		const GameXmlContent::DynamicTrapRemote& r = st.remotes[i];
		if(!(alive & (1u << i)) || r.life > 0 || dyn.trapSlots[i].seed == 0 || TrapRemoved(next, *r.remote)) continue;
		carried |= 1u << i;
		carriedCount++;
	}
	if(carried == 0) return false;
	for(i32 i = 0; i < count; i++) {
		if(!(alive & (1u << i)) || (carried & (1u << i)) || st.remotes[i].life > 0 || dyn.trapSlots[i].seed == 0) continue;
		replication->SendDestroyRemoteToAll(dyn.trapSlots[i].seed);
		dyn.trapSlots[i].seed = 0;
	}
	const Time start = dyn.tLastActionChange > dyn.trapStart ? dyn.tLastActionChange : dyn.trapStart;
	dyn.trapMotion = &next;
	dyn.trapCarried = carried;
	dyn.trapMotionOffset = (f32)TimeDiffSec(TimeDiff(dyn.trapStart, start));
	dyn.trapPrevElapsed = -1.0f;
	dyn.trapMotionTracks.clear();
	for(i32 n = 0; n < (i32)st.nodes.size(); n++) {
		i32 track = TRAP_KEEP_TRACK;
		foreach_const(m, next.nodes) if(m->name == st.nodes[n].name) track = m->track;
		dyn.trapMotionTracks.push_back(track);
	}
	LOG("Trap :: dynamic %d (doc %d) carries %d remote(s) of state %d into state %d (loop %.3f s) t=%.2f",
		dyn.localID, (i32)dyn.docID, carriedCount, (i32)st.action, (i32)next.action, next.length, dyn.trapMotionOffset);
	return true;
}

void World::SendDynamicTrapRemote(ActorDynamic& dyn, i32 index)
{
	ActorDynamic::TrapSlot& slot = dyn.trapSlots[index];
	u32 pending = 0;
	foreach_const(p, players) {
		if(p->clientHd == ClientHandle::INVALID || p->removed || p->index >= TRAP_MAX_PLAYER_BITS) continue;
		if(!(slot.sentMask & (1u << p->index))) pending |= 1u << p->index;
	}
	if(pending == 0) return;
	const GameXmlContent::DynamicTrapState& st = *dyn.trapState;
	const GameXmlContent::DynamicTrapRemote& r = st.remotes[index];
	const GameXmlContent::DynamicTrapNode& node = st.nodes[r.node];
	Replication::RemoteCreate create;
	create.parentUID = dyn.UID;
	create.ownerUID = dyn.UID;
	create.seed = slot.seed;
	create.firePos = TrapNodeWorld(dyn, TrapNodeLocal(dyn, node.track, TrapAnimTime(st, r.delay)), MEASURED_TRAP_CREATE_YAW_OFFSET);
	create.fireRotation[2] = (u16)TrapFireYawCentiDeg(dyn.rot.z);
	create.fireObject = node.name.c_str();
	create.fireObjectLen = (u16)node.name.size();
	create.lifeTime = r.life;
	create.doc = r.remote->ID;
	foreach_const(p, players) {
		if(!(pending & (1u << p->index))) continue;
		if(replication->SendRemoteCreateTo(p->clientHd, create)) slot.sentMask |= (u16)(1u << p->index);
	}
}

void World::UpdateDynamicTrap(ActorDynamic& dyn, eastl::fixed_vector<PendingTrapHit,32,true>& hits)
{
	const ActionStateID action = dyn.destroyed ? ActionStateID::INVALID : dyn.action;
	if(action != dyn.trapAction) {
		dyn.trapAction = action;
		const GameXmlContent::DynamicTrapState* next = action != ActionStateID::INVALID ? dyn.trap->model->FindState(action) : nullptr;
		if(!(next && next->remotes.empty() && dyn.trapState && CarryDynamicTrap(dyn, *next))) {
			if(dyn.trapState) StopDynamicTrap(dyn);
			dyn.trapState = next && !next->remotes.empty() ? next : nullptr;
			if(dyn.trapState) StartDynamicTrap(dyn);
		}
	}
	if(!dyn.trapState) return;
	const GameXmlContent::DynamicTrapState& st = *dyn.trapState;
	for(i32 i = 0; i < (i32)dyn.trapHits.size(); ) {
		if(localTime >= dyn.trapHits[i].until) dyn.trapHits.erase(dyn.trapHits.begin() + i);
		else i++;
	}
	const f32 elapsed = (f32)TimeDiffSec(TimeDiff(dyn.trapStart, localTime));
	const f32 loopLength = MAX(st.length, HYP_TRAP_MIN_LOOP_SEC);
	u32 activeRemotes = 0;
	u32 activeNodes = 0;
	for(i32 i = 0; i < (i32)st.remotes.size(); i++) {
		if(dyn.trapMotion && !(dyn.trapCarried & (1u << i))) continue;
		const GameXmlContent::DynamicTrapRemote& r = st.remotes[i];
		ActorDynamic::TrapSlot& slot = dyn.trapSlots[i];
		const f32 since = elapsed - r.delay;
		if(since < 0) continue;
		const bool persistent = r.life <= 0;
		const i32 activation = persistent || !st.loop ? 0 : (i32)(since / loopLength);
		if(!persistent && since - (f32)activation * loopLength > r.life) continue;
		if(slot.activation != activation) {
			slot.activation = activation;
			slot.seed = MEASURED_TRAP_REMOTE_ID_BASE | (nextMonsterRemoteID++ & TRAP_REMOTE_ID_MASK);
			slot.sentMask = 0;
			if(activation == 0) LOG("Trap :: dynamic %d (doc %d) remote %d on %ls seed %#x (life %.2f) t=%.2f", dyn.localID, (i32)dyn.docID, (i32)r.remote->ID, st.nodes[r.node].name.c_str(), slot.seed, r.life, elapsed);
		}
		SendDynamicTrapRemote(dyn, i);
		activeRemotes |= 1u << i;
		activeNodes |= 1u << r.node;
	}
	const f32 from = dyn.trapPrevElapsed >= 0 && dyn.trapPrevElapsed <= elapsed ? dyn.trapPrevElapsed : elapsed;
	dyn.trapPrevElapsed = elapsed;
	if(frozen || activeNodes == 0) return;
	const f32 step = GetGameXmlContent().dynamicTrapStep;
	for(i32 n = 0; n < (i32)st.nodes.size(); n++) {
		if(!(activeNodes & (1u << n))) continue;
		const bool follow = dyn.trapMotion && n < (i32)dyn.trapMotionTracks.size() && dyn.trapMotionTracks[n] != TRAP_KEEP_TRACK;
		const GameXmlContent::DynamicTrapState& anim = follow ? *dyn.trapMotion : st;
		const i32 track = follow ? dyn.trapMotionTracks[n] : st.nodes[n].track;
		const f32 base = follow ? dyn.trapMotionOffset : 0.0f;
		const i32 steps = track >= 0 && step > 0 ? MIN(MAX((i32)ceilf((elapsed - from) / step), 1), HYP_TRAP_MAX_SUBSTEPS) : 1;
		for(i32 s = 0; s < steps; s++) {
			const f32 a0 = TrapAnimTime(anim, from + (elapsed - from) * (f32)s / (f32)steps - base);
			const f32 a1 = TrapAnimTime(anim, from + (elapsed - from) * (f32)(s + 1) / (f32)steps - base);
			const vec3 p1 = TrapNodeWorld(dyn, TrapNodeLocal(dyn, track, a1), MEASURED_TRAP_NODE_YAW_OFFSET);
			const vec3 p0 = a0 <= a1 ? TrapNodeWorld(dyn, TrapNodeLocal(dyn, track, a0), MEASURED_TRAP_NODE_YAW_OFFSET) : p1;
			const vec2 dir = TrapRotate(dyn.rot.z + MEASURED_TRAP_NODE_YAW_OFFSET, TrapNodeDir(dyn, track, a1));
			CollectDynamicTrapHits(dyn, (u8)n, activeRemotes, p0, p1, dir, hits);
		}
	}
}

void World::CollectDynamicTrapHits(ActorDynamic& dyn, u8 node, u32 activeRemotes, const vec3& from, const vec3& to, const vec2& dir, eastl::fixed_vector<PendingTrapHit,32,true>& hits)
{
	const GameXmlContent::DynamicTrapState& st = *dyn.trapState;
	const i32 playerRemote = TrapRemoteFor(st, node, activeRemotes, Remote::VS_PLAYER_CHARACTER);
	const i32 monsterRemote = TrapRemoteFor(st, node, activeRemotes, Remote::VS_NPC_MONSTER);
	if(playerRemote >= 0) {
		const GameXmlContent::DynamicTrapRemote& r = st.remotes[playerRemote];
		foreach(p, players) {
			if(p->dead || p->untargetable || p->removed || !p->body) continue;
			const ActorUID uid = p->Main().UID;
			if(TrapHitPending(dyn, node, uid)) continue;
			if(!TrapShapeHit(*r.remote, from, to, dir, p->body->GetWorldPos(), HitRadius(*p), BodyMidHeight(*p) * 2.0f)) continue;
			dyn.trapHits.push_back({ uid, node, TimeAdd(localTime, TimeMsToTime((f64)r.remote->hitInvalidTime * 1000.0)) });
			hits.push_back({ dyn.UID, &r, dyn.trapSlots[playerRemote].seed, to, dir, uid, (i32)p->index });
		}
	}
	if(monsterRemote >= 0) {
		const GameXmlContent::DynamicTrapRemote& r = st.remotes[monsterRemote];
		foreach(o, actorNpcList) {
			if(o->faction != Faction::MONSTER || o->ownerUID != ActorUID::INVALID || !o->monsterAi || o->hp <= 0 || o->removeAt != Time::ZERO) continue;
			if(o->despawnAt != Time::ZERO && localTime >= o->despawnAt) continue;
			if(TrapHitPending(dyn, node, o->UID)) continue;
			if(!TrapShapeHit(*r.remote, from, to, dir, o->pos, NpcHitRadius(*o), HYP_TRAP_NPC_HEIGHT)) continue;
			dyn.trapHits.push_back({ o->UID, node, TimeAdd(localTime, TimeMsToTime((f64)r.remote->hitInvalidTime * 1000.0)) });
			hits.push_back({ dyn.UID, &r, dyn.trapSlots[monsterRemote].seed, to, dir, o->UID, -1 });
		}
	}
}

void World::ApplyDynamicTrapHit(const PendingTrapHit& hit)
{
	auto found = actorDynamicMap.find(hit.dynamicUID);
	if(found == actorDynamicMap.end()) return;
	const ActorDynamic& dyn = *found->second;
	if(!dyn.trap) return;
	const GameXmlContent& content = GetGameXmlContent();
	const Remote& info = *hit.remote->remote;
	const f32 roll = 1.0f + content.dynamicTrapSpread * ((f32)(RandUint() % (u32)(TRAP_ROLL_STEPS * 2 + 1)) / (f32)TRAP_ROLL_STEPS - 1.0f);
	const i32 damage = (i32)(content.dynamicTrapBaseAttack * roll * dyn.trap->ratio * info.attackMultiplier * monsterStatScale + 0.5f);
	if(damage <= 0) return;
	eastl::fixed_vector<i32,4,false> statuses;
	foreach_const(st, info.statuses) {
		if(st->rate >= STATUS_RATE_ALWAYS || (i32)(RandUint() % (u32)STATUS_RATE_ALWAYS) < st->rate) statuses.push_back(st->statusID);
	}
	const i32 firstStatus = statuses.empty() ? 0 : statuses[0];
	ActorNpc source(dyn.UID);
	source.docID = dyn.docID;
	source.localID = dyn.localID;
	source.faction = dyn.faction;
	source.pos = hit.nodePos;
	source.rot = vec3(0, 0, atan2f(hit.nodeDir.y, hit.nodeDir.x));
	source.hp = 1;
	source.hpMax = 1;
	if(hit.playerIndex >= 0) {
		if(hit.playerIndex >= (i32)players.size()) return;
		Player& target = players[hit.playerIndex];
		if(target.dead || target.removed) return;
		const i32 hpBefore = target.Main().hp;
		DamagePlayerFromNpc(source, target, damage, (i32)info.ID, firstStatus, hit.seed);
		for(i32 s = 1; s < (i32)statuses.size(); s++) if(!target.dead) ApplyStatusFromNpc(source, target, statuses[s]);
		LOG("Trap :: dynamic %d (doc %d) remote %d seed %#x hits %ls at (%.0f, %.0f) from (%.0f, %.0f, %.0f) raw %d hp %d -> %d",
			dyn.localID, (i32)dyn.docID, (i32)info.ID, hit.seed, target.name.data(), target.body ? target.body->GetWorldPos().x : 0.0f, target.body ? target.body->GetWorldPos().y : 0.0f,
			hit.nodePos.x, hit.nodePos.y, hit.nodePos.z, damage, hpBefore, target.Main().hp);
		return;
	}
	ActorNpc* npc = FindNpcActor(hit.targetUID);
	if(!npc || npc->hp <= 0 || npc->removeAt != Time::ZERO) return;
	const i32 hpBefore = npc->hp;
	DamageNpcFromNpc(source, *npc, damage, (i32)info.ID, hit.seed, firstStatus);
	for(i32 s = 1; s < (i32)statuses.size(); s++) if(npc->hp > 0) ApplyStatusToNpcFromNpc(source, *npc, statuses[s]);
	LOG("Trap :: dynamic %d (doc %d) remote %d seed %#x hits creature %d (local %d) at (%.0f, %.0f) from (%.0f, %.0f, %.0f) raw %d hp %d -> %d",
		dyn.localID, (i32)dyn.docID, (i32)info.ID, hit.seed, (i32)npc->docID, npc->localID, npc->pos.x, npc->pos.y, hit.nodePos.x, hit.nodePos.y, hit.nodePos.z, damage, hpBefore, npc->hp);
}
