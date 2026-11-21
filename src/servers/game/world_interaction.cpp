#include "config.h"
#include "world.h"
#include "world_common.h"
#include <mxm/game_content.h>
#include <EASTL/algorithm.h>

static const f64 RON_ITEM_LIFETIME_SEC = 120.0;
static const f32 TELEPORT_SPREAD = 80.0f;
static const i32 INTERACTION_NOT_OCCUPIED = -1;
static const u32 PLAYER_FACTION_BASE = 3;
static const f64 MEASURED_OCCUPY_TICK_MS = 5000.0;
static const i32 MEASURED_OCCUPY_TICK_POINTS = 1;
static const i32 MEASURED_OCCUPY_CAPTURE_POINTS = 3;
static const i32 MEASURED_OCCUPY_MID_CAPTURE_POINTS = 10;
static const i32 MEASURED_OCCUPY_MID_POINTS[] = { 110045552, 110045557 };
static const u32 MEASURED_ANNOUNCE_OCCUPY_CAPTURE = 9;
static const u32 MEASURED_ANNOUNCE_OCCUPY_TICK = 10;
static const f32 HYP_BOT_OCCUPY_REACH = 0.8f;
static const f32 CARRY_SYNC_SEC = 0.1f;
static const f32 TOOLTIP_INTERACTION_OPEN_SEC = 3.0f;
static const i32 INTERACTION_OBJECT_LOCAL_BASE = 41000;
static const f64 INTERACTION_ANNOUNCE_DELAY_MS = 200.0;
static const i32 GROUP_TELEPORT_SLOTS = 5;
static const f32 LEVEL_TELEPORT_LIFT = 30.0f;

void World::GroupTeleport(Player& caster, SkillID skillID, const vec3& from)
{
	const ActiveStatus* passive = FindGroupTeleportPassive(caster, skillID);
	if(!passive || caster.dead) return;
	const StatusInfo& rule = *passive->info;
	const vec3 dest = caster.body->GetWorldPos();
	i32 moved = 0;
	foreach(it, players) {
		if(it->index == caster.index || it->dead || it->team != caster.team) continue;
		if(glm::length(vec2(it->body->GetWorldPos()) - vec2(from)) > rule.groupTeleportRadius) continue;
		if(HasStatusFromSkill(*it, caster.index, (i32)skillID)) continue;
		const f32 a = moved * PI * 2.0f / GROUP_TELEPORT_SLOTS;
		if(rule.groupTeleportStatus != 0) ApplyStatus(caster, *it, rule.groupTeleportStatus);
		MovePlayer(*it, dest + vec3(cosf(a) * TELEPORT_SPREAD, sinf(a) * TELEPORT_SPREAD, 0), it->input.rot.upperYaw, true);
		moved++;
	}
	LOG("Interaction :: %ls group teleport moved %d allies from (%.0f, %.0f) radius %.0f", caster.name.data(), moved, from.x, from.y, rule.groupTeleportRadius);
}

void World::OpenInteractionObject(Player& owner, i32 interactionID)
{
	const GameXmlContent::InteractionInfo* info = GetGameXmlContent().FindInteraction(interactionID);
	if(!info || info->receiver == CreatureIndex::Invalid) {
		LOG("Interaction :: %ls interaction %d has no receiver object", owner.name.data(), interactionID);
		return;
	}
	for(auto it = openInteractions.begin(); it != openInteractions.end(); ) {
		if(it->ownerIndex == owner.index && it->interactionID == interactionID) {
			RemoveDynamic(it->objectUID);
			it = openInteractions.erase_unsorted(it);
		}
		else ++it;
	}
	if(openInteractions.full()) return;
	ActorDynamic& object = SpawnDynamic(info->receiver, INTERACTION_OBJECT_LOCAL_BASE + nextInteractionLocal++);
	object.pos = owner.body->GetWorldPos();
	object.rot = vec3(0, 0, owner.input.rot.upperYaw);
	OpenInteraction open;
	open.objectUID = object.UID;
	open.ownerIndex = owner.index;
	open.interactionID = interactionID;
	open.until = TimeAdd(localTime, TimeMsToTime(TOOLTIP_INTERACTION_OPEN_SEC * 1000.0));
	open.announceAt = TimeAdd(localTime, TimeMsToTime(INTERACTION_ANNOUNCE_DELAY_MS));
	openInteractions.push_back(open);
	LOG("Interaction :: %ls opens interaction %d for %.0fs", owner.name.data(), interactionID, TOOLTIP_INTERACTION_OPEN_SEC);
}

ActorUID World::InteractionObjectOf(const Player& owner) const
{
	foreach_const(it, openInteractions) {
		if(it->ownerIndex == owner.index) return it->objectUID;
	}
	return ActorUID::INVALID;
}

void World::UpdateDynamicRespawns()
{
	for(auto it = actorDynamicList.begin(); it != actorDynamicList.end(); ++it) {
		if(!it->destroyed || it->respawnAt == Time::ZERO || localTime < it->respawnAt) continue;
		const ActorDynamic old = *it;
		ActorDynamic& fresh = SpawnDynamic(old.docID, old.localID);
		fresh.pos = old.pos;
		fresh.rot = old.rot;
		fresh.faction = old.faction;
		fresh.tagID = old.tagID;
		fresh.entityType = old.entityType;
		fresh.seed = (i32)RandUint();
		fresh.collider = old.collider;
		if(fresh.collider) physics.SetStaticEnabled(fresh.collider, true);
		LOG("LevelEvent :: dynamic %d (doc %d) respawns", old.localID, (i32)old.docID);
		RemoveDynamic(old.UID);
		return;
	}
}

void World::RemoveDynamic(ActorUID uid)
{
	auto found = actorDynamicMap.find(uid);
	if(found == actorDynamicMap.end()) return;
	actorDynamicList.erase(found->second);
	actorDynamicMap.erase(found);
}

void World::PlayerInteract(Player& player, ActorUID targetUID)
{
	if(player.dead || player.carriedBy >= 0 || targetUID == ActorUID::INVALID) return;
	if(StartMapInteraction(player, targetUID)) return;
	const OpenInteraction* open = nullptr;
	foreach_const(it, openInteractions) {
		if(it->objectUID == targetUID || players[it->ownerIndex].Main().UID == targetUID) open = &*it;
	}
	if(!open) return;
	Player& owner = players[open->ownerIndex];
	if(owner.index == player.index || owner.team != player.team || owner.dead) return;
	const GameXmlContent::InteractionInfo* info = GetGameXmlContent().FindInteraction(open->interactionID);
	ASSERT(info);
	const f32 dist = glm::length(vec2(owner.body->GetWorldPos()) - vec2(player.body->GetWorldPos()));
	if(info->activeDistance > 0 && dist > info->activeDistance + HitRadius(owner)) {
		LOG("Interaction :: %ls too far from %ls (%.0f > %.0f)", player.name.data(), owner.name.data(), dist, info->activeDistance);
		return;
	}
	if(info->carry) BoardCarrier(player, owner, open->interactionID);
}

void World::BoardCarrier(Player& passenger, Player& carrier, i32 interactionID)
{
	const ActiveStatus* passive = FindCarrierPassive(carrier);
	if(!passive) return;
	foreach_const(p, passive->info->params) {
		if(p->name == "DATA") ApplyStatus(carrier, passenger, (i32)p->value);
	}
	passenger.carriedBy = (i32)carrier.index;
	passenger.carrierCharaID = carrier.mainCharaID;
	passenger.untargetable = true;
	replication->SendInteractionExecuteToAll(passenger.Main().UID, carrier.Main().UID, (u32)interactionID, 0);
	LOG("Interaction :: %ls boards %ls", passenger.name.data(), carrier.name.data());
}

void World::LeaveCarrier(Player& passenger)
{
	if(passenger.carriedBy < 0) return;
	Player& carrier = players[passenger.carriedBy];
	passenger.carriedBy = -1;
	passenger.untargetable = false;
	if(const ActiveStatus* passive = FindCarrierPassive(carrier)) {
		foreach_const(p, passive->info->params) {
			if(p->name != "DATA") continue;
			const i32 id = (i32)p->value;
			const StatusInfo* info = GetGameXmlContent().FindStatus(id);
			foreach(st, activeStatuses) {
				if(st->targetIndex == passenger.index && st->statusID == id) st->until = localTime;
				if(info && st->targetIndex == carrier.index && st->statusID == info->casterStatus) st->until = localTime;
			}
		}
	}
	if(!passenger.dead) {
		MovePlayer(passenger, carrier.body->GetWorldPos() + vec3(TELEPORT_SPREAD, 0, 0), passenger.input.rot.upperYaw, true);
	}
	LOG("Interaction :: %ls leaves %ls", passenger.name.data(), carrier.name.data());
}

void World::UpdateInteractions()
{
	for(auto it = pendingInteractions.begin(); it != pendingInteractions.end(); ) {
		if(localTime < it->time) {
			++it;
			continue;
		}
		Player& owner = players[it->ownerIndex];
		if(!owner.dead && it->teleportTarget != ActorUID::INVALID) TeleportNextToTarget(owner, it->teleportTarget);
		else if(!owner.dead) OpenInteractionObject(owner, it->interactionID);
		it = pendingInteractions.erase_unsorted(it);
	}
	for(auto it = openInteractions.begin(); it != openInteractions.end(); ) {
		Player& owner = players[it->ownerIndex];
		if(it->announceAt != Time::ZERO && localTime >= it->announceAt) {
			it->announceAt = Time::ZERO;
			replication->SendInteractionStatusToAll(it->objectUID, 1u << (PLAYER_FACTION_BASE + owner.team), it->interactionID, INTERACTION_NOT_OCCUPIED, owner.clientHd);
		}
		if(localTime >= it->until || owner.dead) {
			RemoveDynamic(it->objectUID);
			it = openInteractions.erase_unsorted(it);
		}
		else ++it;
	}
	foreach(it, players) {
		if(it->carriedBy < 0) continue;
		Player& carrier = players[it->carriedBy];
		if(it->dead || carrier.dead || carrier.Main().stance == 0 || carrier.mainCharaID != it->carrierCharaID) {
			LeaveCarrier(*it);
			continue;
		}
		TeleportPlayer(*it, carrier.body->GetWorldPos(), carrier.input.rot.upperYaw);
		if(it->clientHd != ClientHandle::INVALID && localTime >= it->nextCarrySync) {
			it->nextCarrySync = TimeAdd(localTime, TimeMsToTime(CARRY_SYNC_SEC * 1000.0));
			replication->SendSelfMove(it->clientHd, it->Main().UID, carrier.body->GetWorldPos(), carrier.movement.moveDir, carrier.movement.moveSpeed, carrier.input.rot.upperYaw);
		}
	}
	foreach(it, players) {
		if(it->dead || it->clientHd != ClientHandle::INVALID || !it->body) continue;
		const Player* holder = AttachHolder(*it);
		if(!holder || holder->dead || !holder->body) continue;
		const vec3 at = holder->body->GetWorldPos();
		it->body->collider->setFootPosition(PxExtendedVec3(at.x, at.y, at.z));
		it->body->vel = vec3(0);
		it->input.moveTo = at;
	}
	foreach(it, players) {
		if(it->dead || !it->body || !HasStatusType(*it, StatusType::BURROW) || !HasStatusType(*it, StatusType::TARGET)) continue;
		bool found = false;
		vec3 at = vec3(0);
		Time latest = Time::ZERO;
		foreach_const(st, activeStatuses) {
			if(st->casterIndex != it->index || st->targetIndex == it->index || !st->info || st->info->type != StatusType::CASTER || st->until <= localTime || (found && st->appliedAt <= latest)) continue;
			const Player& victim = players[st->targetIndex];
			if(victim.dead || !victim.body) continue;
			at = victim.body->GetWorldPos();
			latest = st->appliedAt;
			found = true;
		}
		foreach_const(ns, npcStatuses) {
			if(ns->casterUID != it->Main().UID || ns->until <= localTime || (found && ns->appliedAt <= latest)) continue;
			const StatusInfo* info = GetGameXmlContent().FindStatus(ns->statusID);
			if(!info || info->type != StatusType::CASTER) continue;
			const ActorNpc* npc = FindNpcActor(ns->npcUID);
			if(!npc || npc->hp <= 0 || npc->removeAt != Time::ZERO) continue;
			at = npc->pos;
			latest = ns->appliedAt;
			found = true;
		}
		if(!found) continue;
		if(localTime < it->dashUntil) {
			it->dashDone = it->dashDist;
			it->dashUntil = localTime;
		}
		it->body->collider->setFootPosition(PxExtendedVec3(at.x, at.y, at.z));
		it->body->vel = vec3(0);
		it->input.moveTo = at;
	}
}

static const u32 STAT_REASON_ITEM = 3;
static const u8 MONSTER_DEFENCE_STAT = 7;
static const i32 DYNAMIC_INDEX_RANGE = 110;
static const f32 MONSTER_STATUS_FOREVER_SEC = 1000000.0f;
static const f32 BOT_ITEM_PICKUP_RADIUS = 100.0f;

void World::StartLevelEvents()
{
	if(levelEventsStarted) return;
	levelEventsStarted = true;
	static const u32 coreTag = StatusTypeKey("Core");
	if(map->ingameLevels) foreach(npc, actorNpcList) if(npc->monsterAi && npc->monsterAi->tagKey == coreTag) StartCore(*npc);
	foreach_const(n, map->levelEvents) {
		if(n->kind != GameXmlContent::LevelEventNode::LEVEL_START || !NodeInDifficulty(*n)) continue;
		foreach_const(o, n->outputs) FireLevelEvent(*o, -1, n->id);
	}
	if(defence.active) SendMinerals();
}

i32 World::AreaOwnerTeam(const GameXmlContent::Map::Area& area) const
{
	f32 best = FLT_MAX;
	i32 team = -1;
	foreach_const(s, map->creatures) {
		if(!s->IsSpawnPoint() || (s->faction != Faction::RED && s->faction != Faction::BLUE)) continue;
		const f32 d = glm::length(vec2(s->pos) - vec2(area.pos));
		if(d < best) { best = d; team = (i32)s->faction; }
	}
	return team;
}

void World::SetLevelTriggerActive(u32 nameKey, bool active)
{
	if(active && map) {
		for(i32 slot = 0; slot < (i32)map->levelEvents.size(); slot++) {
			if(map->levelEvents[slot].nameKey != nameKey) continue;
			deadCountsDone.erase(slot);
			boundTriggerFires.erase(slot);
		}
	}
	foreach(t, levelTriggerOverrides) {
		if(t->first != nameKey) continue;
		t->second = active;
		return;
	}
	levelTriggerOverrides.push_back(eastl::make_pair(nameKey, active));
}

void World::SetLevelGroupActive(u32 groupKey, bool active)
{
	if(!map || groupKey == 0) return;
	i32 members = 0;
	for(i32 slot = 0; slot < (i32)map->levelEvents.size(); slot++) {
		const GameXmlContent::LevelEventNode& n = map->levelEvents[slot];
		if(n.groupKey != groupKey) continue;
		members++;
		if(active) {
			deadCountsDone.erase(slot);
			boundTriggerFires.erase(slot);
		}
		for(i32 i = 0; i < (i32)levelTriggerOverrides.size(); ) {
			if(levelTriggerOverrides[i].first == n.nameKey) levelTriggerOverrides.erase(levelTriggerOverrides.begin() + i);
			else i++;
		}
	}
	LOG("LevelEvent :: group %u (%d nodes) %s", groupKey, members, active ? "activated" : "deactivated");
	foreach(g, levelGroupOverrides) {
		if(g->first != groupKey) continue;
		g->second = active;
		return;
	}
	levelGroupOverrides.push_back(eastl::make_pair(groupKey, active));
}

i32 World::LevelNodeOverride(const GameXmlContent::LevelEventNode& node) const
{
	foreach_const(t, levelTriggerOverrides) if(t->first == node.nameKey) return t->second ? 1 : 0;
	if(node.groupKey != 0) foreach_const(g, levelGroupOverrides) if(g->first == node.groupKey) return g->second ? 1 : 0;
	return -1;
}

static f32 LevelStatusDuration(i32 statusID)
{
	const StatusInfo* info = GetGameXmlContent().FindStatus(statusID);
	return info && info->durationSec < 0 ? STATUS_INFINITE_DURATION_SEC : 0.0f;
}

void World::ApplyMonsterStatus(ActorNpc& npc, i32 statusID, bool apply)
{
	for(i32 i = 0; i < (i32)npcStatuses.size(); ) {
		if(npcStatuses[i].npcUID == npc.UID && npcStatuses[i].statusID == statusID) npcStatuses.erase(npcStatuses.begin() + i);
		else i++;
	}
	if(!apply) {
		replication->SendRemoveStatusToAll(statusID, npc.UID, npc.UID);
		LOG("Status :: creature=%d (local %d) loses status %d", (i32)npc.docID, npc.localID, statusID);
		return;
	}
	const StatusInfo* info = GetGameXmlContent().FindStatus(statusID);
	const f32 duration = info && info->durationSec > 0 ? info->durationSec : MONSTER_STATUS_FOREVER_SEC;
	replication->SendAddStatusToAll(statusID, npc.UID, npc.UID, info && info->permanent ? StatusSentDuration(*info) : info && info->durationSec < 0 ? STATUS_INFINITE_DURATION_SEC : duration, npc.pos, npc.pos, 0, 1, 1.0f, -1.0f, false, false);
	if(!npcStatuses.full()) {
		NpcStatus ns;
		ns.npcUID = npc.UID;
		ns.statusID = statusID;
		ns.casterUID = npc.UID;
		ns.until = TimeAdd(localTime, TimeMsToTime((f64)duration * 1000.0));
		ns.appliedAt = localTime;
		ns.linked = false;
		if(info && info->type == StatusType::SHIELD) ns.shieldHP = GetGameXmlContent().StatusShieldAmount(*info);
		npcStatuses.push_back(ns);
	}
	LOG("Status :: creature=%d (local %d) gets status %d", (i32)npc.docID, npc.localID, statusID);
}

static const f32 NPC_DEFEAT_PUSH_SEC = 1.0f;
static const f64 MEASURED_NPC_PUSH_RECOVER_MS = 270.0;
static const f64 MEASURED_NPC_LIE_RECOVER_MS = 300.0;
static const f64 MEASURED_NPC_STUN_RECOVER_MS = 140.0;
static const f32 MEASURED_NPC_DEFEAT_AIRBORNE_SEC = 1.5f;
static const f32 MEASURED_NPC_AIRBORNE_GRAPH_SEC = 1.0f;
static const f32 MEASURED_NPC_LIFT_BASE_HEIGHT = 400.0f;
static const f64 MEASURED_NPC_CC_FIRST_MOVE_MS = 47.0;
static const f64 MEASURED_NPC_CC_MOVE_MS = 93.0;
static const f32 MEASURED_NPC_HOOK_OFFSET = 200.0f;
static const f32 NPC_DEFEAT_KNOCKDOWN_SEC = 1.5f;
static const f32 NPC_DEFEAT_FLY_SEC = 1.8f;
static const f32 NPC_DEFEAT_SYNC_SEC = 1.0f;
static const f32 NPC_LIE_SEC = 1.0f;
static const f32 MEASURED_NPC_PUSH_BASE_DISTANCE = 500.0f;
static const f32 NPC_PUSH_RAY_HEIGHT = 100.0f;
static const f32 MEASURED_NPC_AIRBORNE_LIFT[] = { 0.0f, 298.0f, 371.0f, 391.0f, 356.0f, 322.0f, 284.0f, 210.0f, 84.0f, 0.0f };
static const f64 MEASURED_NPC_AIRBORNE_STEP_MS = 100.0;

static bool HypPveMonsterTakesCc(const World::ActorNpc& npc)
{
	const GameXmlContent& content = GetGameXmlContent();
	const u8 rank = content.FindMonsterRank((i32)npc.docID);
	return rank >= content.rankNames.size() || !(content.rankNames[rank] == "BOSS");
}

void World::CancelNpcPendingHits(const ActorNpc& npc)
{
	for(i32 i = 0; i < (i32)monsterHits.size(); ) {
		if(monsterHits[i].attacker == npc.UID && !monsterHits[i].ally && localTime < monsterHits[i].at) monsterHits.erase(monsterHits.begin() + i);
		else i++;
	}
	for(i32 i = 0; i < (i32)monsterFires.size(); ) {
		if(monsterFires[i].npc == npc.UID && localTime < monsterFires[i].at) monsterFires.erase(monsterFires.begin() + i);
		else i++;
	}
}

f32 World::NpcDefeatReaction(ActorNpc& npc, const StatusInfo* info)
{
	if(!info || !npc.monsterAi) return 0.0f;
	if(info->type == StatusType::DRAGGED || info->type == StatusType::DRAGGEDBYFORCE) {
		const bool takes = eastl::find(npc.monsterAi->ccTypes.begin(), npc.monsterAi->ccTypes.end(), info->type) != npc.monsterAi->ccTypes.end();
		return takes || (hostileMonsters && HypPveMonsterTakesCc(npc)) ? 0.0f : -1.0f;
	}
	ActionStateID state;
	f32 sec = 0;
	bool lie = false;
	const f32 timeRatio = info->timeRatio > 0 ? info->timeRatio : 1.0f;
	switch(info->type) {
		case StatusType::PUSH: state = ActionStateID::DEFEAT_PUSH_BEHAVIORSTATE; sec = NPC_DEFEAT_PUSH_SEC * timeRatio; break;
		case StatusType::AIRBORNE: state = ActionStateID::DEFEAT_AIRBORNE_BEHAVIORSTATE; sec = MEASURED_NPC_DEFEAT_AIRBORNE_SEC * timeRatio; lie = true; break;
		case StatusType::KNOCKDOWN: state = ActionStateID::DEFEAT_KNOCKDOWN_BEHAVIORSTATE; sec = NPC_DEFEAT_KNOCKDOWN_SEC * timeRatio; lie = true; break;
		case StatusType::FLY: state = ActionStateID::DEFEAT_FLY_BEHAVIORSTATE; sec = info->durationSec > 0 ? info->durationSec : NPC_DEFEAT_FLY_SEC; lie = true; break;
		case StatusType::STUN:
		case StatusType::SHOCKED:
		case StatusType::FREEZE:
		case StatusType::DANCE: state = ActionStateID::DEFEAT_SYNC_BEHAVIORSTATE; sec = info->durationSec > 0 ? info->durationSec : NPC_DEFEAT_SYNC_SEC; break;
		default: return 0.0f;
	}
	if(eastl::find(npc.monsterAi->ccTypes.begin(), npc.monsterAi->ccTypes.end(), info->type) == npc.monsterAi->ccTypes.end() && !(hostileMonsters && HypPveMonsterTakesCc(npc))) return -1.0f;
	if(npc.removeAt != Time::ZERO) {
		if(npc.diedAt == localTime) {
			replication->SendMonsterStateToAll(npc.UID, state, npc.pos, npc.monsterAi->run * npc.speedMul);
			replication->SendAiActionStateToAll(npc.UID, ActionStateID::DIE_BEHAVIORSTATE, npc.rot.z, npc.pos, false);
		}
		return 0.0f;
	}
	if(const i32 armor = npc.monsterAi->ccArmorStatus) {
		foreach_const(ns, npcStatuses) if(ns->npcUID == npc.UID && ns->statusID == armor && localTime < ns->until) return -1.0f;
		ApplyMonsterStatus(npc, armor, true);
	}
	const f32 total = sec + (lie ? NPC_LIE_SEC : 0.0f);
	if(npc.monsterAi->tagKey == StatusTypeKey("Minion")) CancelNpcPendingHits(npc);
	npc.monsterSpreading = false;
	npc.actionState = state;
	replication->SendMonsterStateToAll(npc.UID, state, npc.pos, npc.monsterAi->run * npc.speedMul);
	if(info->type == StatusType::DANCE) {
		npc.actionState = ActionStateID::NONE_BEHAVIORSTATE;
		replication->SendMonsterStateToAll(npc.UID, npc.actionState, npc.pos, npc.monsterAi->run * npc.speedMul);
	}
	npc.defeatLieAt = lie ? TimeAdd(localTime, TimeMsToTime(sec * 1000.0)) : Time::ZERO;
	npc.airborneAt = info->type == StatusType::AIRBORNE ? localTime : Time::ZERO;
	npc.airborneStep = 0;
	npc.stateEndAt = TimeAdd(localTime, TimeMsToTime(total * 1000.0));
	const f64 recoverMs = info->type == StatusType::PUSH ? MEASURED_NPC_PUSH_RECOVER_MS : (info->type == StatusType::AIRBORNE || info->type == StatusType::KNOCKDOWN) ? MEASURED_NPC_LIE_RECOVER_MS : info->type == StatusType::STUN ? MEASURED_NPC_STUN_RECOVER_MS : 0.0;
	const Time holdEnd = npc.monsterAi->tagKey == StatusTypeKey("Minion") ? TimeAdd(npc.stateEndAt, TimeMsToTime(recoverMs)) : npc.stateEndAt;
	if(npc.monsterHoldUntil < holdEnd) npc.monsterHoldUntil = holdEnd;
	npc.monsterChasing = false;
	npc.monsterWalking = false;
	return total;
}

static const f32 MEASURED_NPC_FORCEBACK_HOLD_SEC = 0.2f;

World::NpcCcGraph World::StartNpcCc(ActorNpc& npc, const StatusInfo* info, const vec3& source, const vec2& dragCenter, const vec3& casterPos, f32 hold)
{
	NpcCcGraph cc;
	cc.to = npc.pos;
	cc.duration = info && info->durationSec > 0 ? info->durationSec : hold;
	if(!info) return cc;
	const f32 timeRatio = info->timeRatio > 0 ? info->timeRatio : 1.0f;
	const bool knockdown = info->type == StatusType::KNOCKDOWN && npc.monsterAi;
	const bool forceBack = info->type == StatusType::FORCEBACK && info->distance > 0 && info->durationSec > 0;
	if(info->type == StatusType::PUSH || knockdown || forceBack) {
		vec2 dir = vec2(npc.pos) - vec2(source);
		const f32 len = glm::length(dir);
		dir = len > 1.0f ? dir / len : vec2(cosf(npc.rot.z), sinf(npc.rot.z));
		const f32 signedDist = forceBack ? info->distance : MEASURED_NPC_PUSH_BASE_DISTANCE * (info->distanceRatio != 0 ? info->distanceRatio : 1.0f);
		if(signedDist < 0) dir = -dir;
		f32 dist = fabsf(signedDist);
		cc.nominal = dist;
		cc.graphSec = forceBack ? info->durationSec : (knockdown ? NPC_DEFEAT_KNOCKDOWN_SEC : NPC_DEFEAT_PUSH_SEC) * timeRatio;
		cc.duration = cc.graphSec;
		cc.graph = true;
		const f32 radius = npc.monsterAi ? npc.monsterAi->radius : 0.0f;
		f32 wall = 0;
		const vec3 rayFrom = vec3(vec2(npc.pos), npc.pos.z + NPC_PUSH_RAY_HEIGHT);
		if(dist > 1.0f && physics.RaycastStatic(rayFrom, rayFrom + vec3(dir * (dist + radius), 0), &wall)) dist = MAX(wall - radius, 0.0f);
		cc.to = vec3(vec2(npc.pos) + dir * dist, npc.pos.z);
		npc.dashFrom = vec2(npc.pos);
		npc.dashDir = dir;
		npc.dashDist = dist;
		npc.dashStart = localTime;
		npc.dashEnd = TimeAdd(localTime, TimeMsToTime(cc.graphSec * 1000.0));
		npc.dashCurve = NPC_DASH_CURVE_PUSH;
		npc.ccNextMove = TimeAdd(localTime, TimeMsToTime(MEASURED_NPC_CC_FIRST_MOVE_MS));
		if(forceBack && npc.monsterAi) {
			const Time holdEnd = TimeAdd(localTime, TimeMsToTime(MEASURED_NPC_FORCEBACK_HOLD_SEC * 1000.0));
			const bool attacking = npc.actionState == ActionStateID::ATTACK_BEHAVIORSTATE || (npc.actionState >= ActionStateID::SKILL_1_BEHAVIORSTATE && npc.actionState <= ActionStateID::SKILL_30_BEHAVIORSTATE);
			if(attacking && npc.monsterAi->tagKey == StatusTypeKey("Minion") && npc.stateEndAt != Time::ZERO && localTime < npc.stateEndAt) {
				CancelNpcPendingHits(npc);
				npc.stateEndAt = localTime;
				npc.monsterHoldUntil = holdEnd;
			}
			if(npc.monsterHoldUntil < holdEnd) npc.monsterHoldUntil = holdEnd;
		}
		return cc;
	}
	if(npc.monsterAi && info->type == StatusType::DRAGGED && cc.duration > 0) {
		const vec2 rel = vec2(npc.pos) - vec2(casterPos);
		const f32 len = glm::length(rel);
		const vec2 dir = len > 1.0f ? rel / len : vec2(cosf(npc.rot.z), sinf(npc.rot.z));
		const vec2 goal = vec2(casterPos) + dir * (info->dragOffset > 0 ? info->dragOffset : MEASURED_NPC_HOOK_OFFSET);
		const vec2 travel = goal - vec2(npc.pos);
		const f32 dist = glm::length(travel);
		cc.to = vec3(goal, npc.pos.z);
		cc.graphSec = cc.duration;
		cc.graph = true;
		if(dist > 1.0f) {
			npc.dashFrom = vec2(npc.pos);
			npc.dashDir = travel / dist;
			npc.dashDist = dist;
			npc.dashStart = localTime;
			npc.dashEnd = TimeAdd(localTime, TimeMsToTime(cc.graphSec * 1000.0));
			npc.dashCurve = NPC_DASH_CURVE_HOOK;
			npc.ccNextMove = TimeAdd(localTime, TimeMsToTime(MEASURED_NPC_CC_FIRST_MOVE_MS));
		}
		return cc;
	}
	if(npc.monsterAi && (info->type == StatusType::AIRBORNE || info->type == StatusType::FLY)) {
		cc.to = vec3(vec2(npc.pos), npc.pos.z + MEASURED_NPC_LIFT_BASE_HEIGHT * (info->heightRatio > 0 ? info->heightRatio : 1.0f));
		cc.graphSec = MEASURED_NPC_AIRBORNE_GRAPH_SEC * timeRatio;
		cc.nominal = 0.0f;
		if(info->type == StatusType::AIRBORNE) cc.duration = MEASURED_NPC_DEFEAT_AIRBORNE_SEC * timeRatio;
		cc.graph = true;
		return cc;
	}
	if(info->type == StatusType::DRAGGEDBYFORCE && cc.duration > 0) {
		npc.dragCenter = dragCenter;
		npc.dragGoal = NpcPullGoal(npc, dragCenter);
		npc.dragMin = info->dragForceMin;
		npc.dragMax = info->dragForceMax;
		npc.dragDuration = cc.duration;
		npc.dragElapsed = 0;
		npc.ccNextMove = TimeAdd(localTime, TimeMsToTime(MEASURED_NPC_CC_FIRST_MOVE_MS));
		if(npc.monsterAi) {
			cc.to = vec3(dragCenter, npc.pos.z);
			cc.graphSec = cc.duration;
			cc.graph = true;
		}
		return cc;
	}
	if(StatusSendsGraph(info) && StatusGraphAnchorOnly(info)) {
		cc.to = vec3(0);
		cc.graph = true;
	}
	return cc;
}

void World::SendNpcCcMove(ActorNpc& npc, bool ended)
{
	if(!ended && localTime < npc.ccNextMove) return;
	replication->SendMonsterLiftToAll(npc.UID, npc.pos);
	npc.ccNextMove = TimeAdd(localTime, TimeMsToTime(MEASURED_NPC_CC_MOVE_MS));
}

void World::UpdateNpcDefeat(ActorNpc& npc)
{
	if(npc.airborneAt != Time::ZERO) {
		const i32 step = (i32)(TimeDurationMs(npc.airborneAt, localTime) / MEASURED_NPC_AIRBORNE_STEP_MS);
		const i32 last = (i32)(sizeof(MEASURED_NPC_AIRBORNE_LIFT) / sizeof(MEASURED_NPC_AIRBORNE_LIFT[0])) - 1;
		if(step > npc.airborneStep) {
			npc.airborneStep = MIN(step, last);
			const vec3 lifted = vec3(vec2(npc.pos), npc.pos.z + MEASURED_NPC_AIRBORNE_LIFT[npc.airborneStep]);
			replication->SendMonsterLiftToAll(npc.UID, lifted);
			if(npc.airborneStep >= last) npc.airborneAt = Time::ZERO;
		}
	}
	if(npc.defeatLieAt != Time::ZERO && localTime >= npc.defeatLieAt) {
		npc.defeatLieAt = Time::ZERO;
		npc.actionState = ActionStateID::LIE_BEHAVIORSTATE;
		replication->SendMonsterStateToAll(npc.UID, ActionStateID::LIE_BEHAVIORSTATE, npc.pos, npc.monsterAi ? npc.monsterAi->run * npc.speedMul : 0.0f);
		if(npc.monsterAi && npc.monsterAi->ccArmorStatus) ApplyMonsterStatus(npc, npc.monsterAi->ccArmorStatus, true);
	}
	if(npc.stateEndAt != Time::ZERO && localTime >= npc.stateEndAt && npc.actionState != ActionStateID::SPAWN_BEHAVIORSTATE) {
		npc.stateEndAt = Time::ZERO;
		const bool ccArmorAgain = npc.monsterAi && npc.monsterAi->ccArmorStatus && (npc.actionState == ActionStateID::DEFEAT_PUSH_BEHAVIORSTATE || npc.actionState == ActionStateID::DEFEAT_SYNC_BEHAVIORSTATE);
		npc.actionState = ActionStateID::NONE_BEHAVIORSTATE;
		const f32 endSpeed = npc.monsterStopSpeed > 0 ? npc.monsterStopSpeed : (npc.monsterAi ? npc.monsterAi->run * npc.speedMul : 0.0f);
		npc.monsterStopSpeed = 0;
		replication->SendMonsterStateToAll(npc.UID, ActionStateID::NONE_BEHAVIORSTATE, npc.pos, endSpeed);
		if(ccArmorAgain) ApplyMonsterStatus(npc, npc.monsterAi->ccArmorStatus, true);
	}
}

vec2 World::NpcPullGoal(const ActorNpc& npc, const vec2& center) const
{
	const vec2 from = vec2(npc.pos);
	const vec2 rel = center - from;
	const f32 len = glm::length(rel);
	if(len <= 1.0f) return center;
	const vec2 dir = rel / len;
	const f32 selfRadius = npc.monsterAi ? npc.monsterAi->radius : 0.0f;
	f32 stop = len;
	foreach_const(o, actorNpcList) {
		if(&*o == &npc || !o->monsterAi || o->hp <= 0 || o->removeAt != Time::ZERO || o->dragDuration <= 0) continue;
		if(glm::length(o->dragCenter - center) >= 1.0f) continue;
		const f32 reach = selfRadius + o->monsterAi->radius;
		const vec2 toOther = (o->dragElapsed < o->dragDuration ? o->dragGoal : vec2(o->pos)) - from;
		const f32 along = glm::dot(toOther, dir);
		const f32 side2 = glm::dot(toOther, toOther) - along * along;
		if(side2 >= reach * reach) continue;
		const f32 t = along - sqrtf(reach * reach - side2);
		if(t < stop) stop = MAX(t, 0.0f);
	}
	return from + dir * stop;
}

void World::UpdateNpcDrag(ActorNpc& npc, f32 delta)
{
	if(npc.dragElapsed >= npc.dragDuration) {
		if(npc.dragTrailing && localTime >= npc.ccNextMove) {
			npc.dragTrailing = false;
			replication->SendMonsterLiftToAll(npc.UID, npc.pos);
		}
		return;
	}
	const f32 dt = MIN(delta, npc.dragDuration - npc.dragElapsed);
	const f32 accel = (npc.dragMax - npc.dragMin) / npc.dragDuration;
	const f32 t0 = npc.dragElapsed;
	const f32 t1 = t0 + dt;
	const f32 step = (t1 * t1 - t0 * t0) * accel * 0.5f + npc.dragMin * dt;
	const vec2 rel = npc.dragGoal - vec2(npc.pos);
	const f32 dist = glm::length(rel);
	npc.dragElapsed = t1;
	if(dist > 0.001f && dist <= step) npc.pos = vec3(npc.dragGoal, npc.pos.z);
	else if(dist > 0.001f) npc.pos = vec3(vec2(npc.pos) + rel / dist * step, npc.pos.z);
	SendNpcCcMove(npc, false);
	if(npc.dragElapsed >= npc.dragDuration) npc.dragTrailing = true;
}

void World::ApplyStatusToNpcFromNpc(const ActorNpc& caster, ActorNpc& npc, i32 statusID)
{
	const StatusInfo* info = GetGameXmlContent().FindStatus(statusID);
	if(info && ((info->reaction && npc.monsterAi && npc.monsterAi->reactionImmune) || NpcResistsDebuff(npc, info, caster.faction != npc.faction))) {
		replication->SendResistStatusToAll(npc.UID, caster.UID, STATUS_RESIST_EVENT_PASSIVE);
		return;
	}
	foreach_const(ns, npcStatuses) if(ns->npcUID == npc.UID && ns->statusID == statusID && ns->casterUID == caster.UID && ns->appliedAt == localTime && (!info || info->maxOverlap <= 1)) return;
	const f32 hold = NpcDefeatReaction(npc, info);
	if(hold < 0) {
		replication->SendResistStatusToAll(npc.UID, caster.UID, STATUS_RESIST_EVENT_PASSIVE);
		return;
	}
	if(npc.removeAt != Time::ZERO) return;
	const vec3 from = npc.pos;
	const NpcCcGraph cc = StartNpcCc(npc, info, caster.pos, npcHitCenterSet ? vec2(npcHitCenter) : vec2(caster.pos), caster.pos, hold);
	const f32 duration = cc.duration;
	if(duration <= 0) return;
	for(i32 i = 0; i < (i32)npcStatuses.size(); ) {
		if(npcStatuses[i].npcUID == npc.UID && npcStatuses[i].statusID == statusID) npcStatuses.erase(npcStatuses.begin() + i);
		else i++;
	}
	replication->SendAddStatusToAll(statusID, npc.UID, caster.UID, duration, from, cc.to, cc.graphSec, 1, 1.0f, cc.nominal, cc.graph, false);
	if(npcStatuses.full()) return;
	NpcStatus ns;
	ns.npcUID = npc.UID;
	ns.statusID = statusID;
	ns.appliedAt = localTime;
	ns.casterUID = caster.UID;
	ns.until = TimeAdd(localTime, TimeMsToTime((f64)duration * 1000.0));
	ns.linked = false;
	if(info && info->type == StatusType::SHIELD) ns.shieldHP = GetGameXmlContent().StatusShieldAmount(*info);
	npcStatuses.push_back(ns);
}

f32 World::NpcStatusMul(const ActorNpc& npc, i32 stat) const
{
	f32 sum = 0;
	foreach_const(ns, npcStatuses) {
		if(ns->npcUID != npc.UID || localTime >= ns->until) continue;
		const StatusInfo* info = GetGameXmlContent().FindStatus(ns->statusID);
		if(!info) continue;
		foreach_const(m, info->mods) if((i32)m->stat == stat && !m->regen && m->mul) sum += m->value;
	}
	return MAX(1.0f + sum / 100.0f, 0.0f);
}

bool World::NpcHasStatusType(const ActorNpc& npc, StatusType type) const
{
	foreach_const(ns, npcStatuses) {
		if(ns->npcUID != npc.UID || localTime >= ns->until) continue;
		const StatusInfo* info = GetGameXmlContent().FindStatus(ns->statusID);
		if(info && info->type == type) return true;
	}
	return false;
}

void World::CountTagDeath(i32 tagID, Faction killerTeam, i32 killerIndex)
{
	if(tagID < 0) return;
	foreach_const(n, map->levelEvents) {
		if(n->kind != GameXmlContent::LevelEventNode::DEAD_COUNT || n->tagID != tagID || !LevelTriggerActive(*n)) continue;
		if(n->team != Faction::INVALID && n->team != Faction::MONSTER && n->team != killerTeam) continue;
		const i32 slot = (i32)(n - map->levelEvents.begin());
		if(deadCountsDone.find(slot) != deadCountsDone.end()) continue;
		i32& count = deadCounts[slot];
		if(++count < n->count) continue;
		count = 0;
		deadCountsDone[slot] = 1;
		LOG("LevelEvent :: dead count of tag %d reached (node %d)", tagID, n->id);
		foreach_const(o, n->outputs) FireLevelEvent(*o, killerIndex, n->id);
	}
}

void World::DestroyDynamic(ActorDynamic& dynamic, i32 killerIndex)
{
	static const u32 DYNAMIC_DEAD_NO_KILLER_DOC = 0;
	if(dynamic.destroyed) return;
	dynamic.destroyed = true;
	if(dynamic.collider) physics.SetStaticEnabled(dynamic.collider, false);
	const auto respawn = GetGameXmlContent().dynamicRespawnSec.find((i32)dynamic.docID);
	if(respawn != GetGameXmlContent().dynamicRespawnSec.end()) dynamic.respawnAt = TimeAdd(localTime, TimeMsToTime(respawn->second * 1000.0));
	replication->SendStatToAll(dynamic.UID, (i32)StatType::STAT_TYPE_HEALTH, 0, 1);
	replication->SendNpcDeadAckToAll(dynamic.UID, ActorUID::INVALID, DYNAMIC_DEAD_NO_KILLER_DOC);
	LOG("LevelEvent :: dynamic %d (doc %d, tag %d) destroyed after %d hits", dynamic.localID, (i32)dynamic.docID, dynamic.tagID, dynamic.destroyHits);
	const Faction killerTeam = killerIndex >= 0 ? (Faction)players[killerIndex].team : Faction::INVALID;
	CountTagDeath(dynamic.tagID, killerTeam, killerIndex);
}

void World::OnMonsterDeath(ActorNpc& npc, ActorUID killerUID)
{
	Faction killerTeam = Faction::INVALID;
	i32 killerIndex = -1;
	if(ActorMaster* m = FindMasterActor(killerUID)) {
		killerTeam = m->parent ? (Faction)m->parent->team : Faction::INVALID;
		killerIndex = m->parent ? (i32)m->parent->index : -1;
	}
	else {
		auto killer = actorNpcMap.find(killerUID);
		if(killer != actorNpcMap.end()) killerTeam = killer->second->faction;
	}
	CountTagDeath(npc.tagID, killerTeam, killerIndex);
	if(!stageCommonDrop.empty()) SpawnPrivateDrops(npc);
	else if(const i32 drop = GetGameXmlContent().FindMonsterDrop((i32)npc.docID)) SpawnDropGroupAt(drop, npc.pos, -1);
	static const u32 coreTag = StatusTypeKey("Core");
	if(npc.monsterAi && npc.monsterAi->tagKey == coreTag && winnerTeam < 0 && (npc.faction == Faction::RED || npc.faction == Faction::BLUE)) {
		winnerTeam = npc.faction == Faction::RED ? 1 : 0;
		LOG("Game :: core of team %d destroyed, team %d wins", (i32)npc.faction, winnerTeam);
	}
}

static const u32 PVP_ANNOUNCE_CORE_ATTACKED = 15;
static const f64 CORE_ANNOUNCE_INTERVAL_MS = 1000.0;

void World::OnMonsterDamaged(ActorNpc& npc, i32 hpBefore)
{
	if(npc.coreStage != 0 && npc.hp < hpBefore) {
		foreach(ns, npcStatuses) {
			if(ns->npcUID != npc.UID || localTime >= ns->until) continue;
			const StatusInfo* info = GetGameXmlContent().FindStatus(ns->statusID);
			if(info && info->nextCondition == StatusInfo::NextCondition::REMOVE_ONHIT) {
				ns->until = localTime;
				ns->removedOnHit = true;
			}
		}
	}
	static const u32 coreTag = StatusTypeKey("Core");
	if(npc.monsterAi && npc.monsterAi->tagKey == coreTag && npc.hp > 0 && npc.hp < hpBefore && localTime >= npc.coreAnnounceAt) {
		npc.coreAnnounceAt = TimeAdd(localTime, TimeMsToTime(CORE_ANNOUNCE_INTERVAL_MS));
		replication->SendPvpAnnouncementToAll(PVP_ANNOUNCE_CORE_ATTACKED, 0, npc.UID, ActorUID::INVALID, (u32)npc.hp);
	}
	if(npc.monsterAi && npc.hp > 0 && npc.hpMax > 0) {
		const auto& buffs = npc.monsterAi->hpBuffs;
		while(npc.hpBuffLevel < buffs.size() && (f32)npc.hp <= buffs[npc.hpBuffLevel].first * npc.hpMax) {
			if(npc.hpBuffLevel > 0) ApplyMonsterStatus(npc, buffs[npc.hpBuffLevel - 1].second, false);
			ApplyMonsterStatus(npc, buffs[npc.hpBuffLevel].second, true);
			const f32 step = npc.monsterAi->hpBuffSpeedStep;
			npc.speedMul *= (1.0f + step * (npc.hpBuffLevel + 1)) / (1.0f + step * npc.hpBuffLevel);
			npc.hpBuffLevel++;
		}
	}
	if(npc.hpMax <= 0) return;
	foreach_const(n, map->levelEvents) {
		if(n->kind != GameXmlContent::LevelEventNode::HP_TRIGGER) continue;
		if(n->tagID >= 0 ? n->tagID != npc.tagID : (n->localID == 0 || n->localID != npc.localID)) continue;
		if(!LevelTriggerActive(*n)) continue;
		const f32 threshold = npc.hpMax * n->hpPercent / 100.0f;
		if((f32)hpBefore <= threshold || (f32)npc.hp > threshold) continue;
		const i32 slot = (i32)(n - map->levelEvents.begin());
		if(eastl::find(hpTriggersFired.begin(), hpTriggersFired.end(), slot) != hpTriggersFired.end()) continue;
		hpTriggersFired.push_back(slot);
		LOG("LevelEvent :: tag %d local %d below %.0f%% hp (node %d)", npc.tagID, npc.localID, n->hpPercent, n->id);
		foreach_const(o, n->outputs) FireLevelEvent(*o, -1, n->id);
	}
}

void World::FireGameEndTriggers(i32 team)
{
	const Faction winner = team == 0 ? Faction::RED : Faction::BLUE;
	foreach_const(n, map->levelEvents) {
		if(n->kind != GameXmlContent::LevelEventNode::GAME_END_TRIGGER || n->team != winner) continue;
		foreach_const(o, n->outputs) FireLevelEvent(*o, -1, n->id);
	}
}

void World::StartTimeRecord()
{
	foreach_const(n, map->levelEvents) {
		if(n->kind != GameXmlContent::LevelEventNode::TIME_RECORD_START) continue;
		foreach_const(o, n->outputs) FireLevelEvent(*o, -1, n->id);
	}
}

void World::InitMonsterStats(ActorNpc& npc)
{
	npc.monsterAi = GetGameXmlContent().FindMonsterAi((i32)npc.docID);
	npc.statScale = monsterStatScale;
	auto path = map->paths.find(npc.localID);
	npc.path = path != map->paths.end() && !path->second.empty() ? &path->second : nullptr;
	npc.pathRoundTrip = npc.path && map->roundTripPaths.count(npc.localID) > 0;
	const GameXmlContent& content = GetGameXmlContent();
	const auto* stats = content.FindMonsterStats((i32)npc.docID);
	if(!stats) return;
	if(gameStartedAt != Time::ZERO && content.monsterGrowthPeriod > 0) {
		npc.growthLevel = MIN((i32)(TimeDiffSec(TimeDiff(gameStartedAt, localTime)) / content.monsterGrowthPeriod), content.monsterGrowthCap);
	}
	const auto& statList = *stats;
	foreach_const(st, statList) {
		const f32 value = st->value + content.MonsterGrowth((i32)npc.docID, st->type) * npc.growthLevel;
		if(st->type == (u8)StatType::STAT_TYPE_HEALTH) npc.hp = npc.hpMax = MAX((i32)(value * npc.statScale), 1);
		if(st->type == MONSTER_DEFENCE_STAT) npc.defence = value;
		if(st->type == (u8)StatType::STAT_TYPE_MOVESPEED && value > 0) npc.speedMul = value / 100.0f;
	}
}

World::ActorNpc& World::SpawnMonster(CreatureIndex docID, i32 localID, const vec3& pos, const vec3& rot, Faction faction, i32 tagID)
{
	ActorNpc& npc = SpawnNpcActor(docID, localID);
	npc.pos = pos;
	npc.home = pos;
	npc.rot = rot;
	npc.faction = faction;
	npc.tagID = tagID;
	npc.actionState = ActionStateID::SPAWN_BEHAVIORSTATE;
	npc.seed = (i32)RandUint();
	InitMonsterStats(npc);
	const f32 spawnSec = GetGameXmlContent().MonsterSpawnSec((i32)docID);
	npc.stateEndAt = TimeAdd(localTime, TimeMsToTime(spawnSec > 0 ? spawnSec * 1000.0 : MEASURED_MONSTER_SPAWN_STATE_MIN_MS));
	if(npc.monsterHoldUntil < npc.stateEndAt) npc.monsterHoldUntil = npc.stateEndAt;
	if(npc.monsterAi && npc.monsterAi->playerSide && (faction == Faction::MONSTER || faction == Faction::INVALID)) npc.faction = PlayerSideFaction();
	if(npc.monsterAi && npc.monsterAi->teamBuff != 0 && (faction == Faction::RED || faction == Faction::BLUE)) {
		foreach(p, players) {
			if(p->dead || !p->body || (Faction)p->team != faction || p->Main().classType == ClassType::NONE) continue;
			ApplyStatusFromNpc(npc, *p, npc.monsterAi->teamBuff, STATUS_INFINITE_DURATION_SEC);
		}
	}
	if(const f32 life = GetGameXmlContent().FindMonsterLifeTime((i32)docID)) npc.selfDestructAt = TimeAdd(localTime, TimeMsToTime(life * 1000.0));
	return npc;
}

void World::CallMonsters(const GameXmlContent::LevelEventNode& node, i32 playerIndex)
{
	GameXmlContent::Map::Area relativeArea;
	const GameXmlContent::Map::Area* area = map->FindArea(node.areaID);
	if(node.relative) {
		const Player* anchor = playerIndex >= 0 && playerIndex < (i32)players.size() && !players[playerIndex].dead ? &players[playerIndex] : nullptr;
		foreach_const(p, players) if(!anchor && !p->dead && p->body && !p->removed) anchor = &*p;
		if(!anchor || !anchor->body) return;
		relativeArea.pos = anchor->body->GetWorldPos();
		relativeArea.rot = vec3(0);
		area = &relativeArea;
	}
	if(!area) {
		WARN("LevelEvent :: node %d calls monster %d in unknown area %d", node.id, node.monster, node.areaID);
		return;
	}
	for(i32 i = 0; i < node.count; i++) {
		if(node.monster / 1000000 == DYNAMIC_INDEX_RANGE) {
			bool timed = false;
			if(GetGameXmlContent().dynamicRespawnSec.count(node.monster)) {
				foreach_const(d, actorDynamicList) if((i32)d->docID == node.monster && d->tagID == node.tagID && (!d->destroyed || d->respawnAt != Time::ZERO)) timed = true;
			}
			if(timed) {
				LOG("LevelEvent :: node %d skips dynamic %d (tag %d), its own respawn timer brings it back", node.id, node.monster, node.tagID);
				continue;
			}
			ActorDynamic& dyn = SpawnDynamic((CreatureIndex)node.monster, node.areaID);
			dyn.pos = area->pos;
			dyn.rot = area->rot;
			dyn.tagID = node.tagID;
			if(node.team != Faction::INVALID && node.team != Faction::MONSTER) dyn.faction = node.team;
			if(const i32 interaction = GetGameXmlContent().FindDynamicInteraction(node.monster)) AddDynamicInteraction(dyn, interaction);
			continue;
		}
		ActorNpc& npc = SpawnMonster((CreatureIndex)node.monster, node.areaID, area->pos, vec3(area->rot.x, area->rot.y, MxmYawToWorldYaw(area->rot.z)), node.team, node.tagID);
		npc.createRot = area->rot;
		npc.hasCreateRot = true;
		npc.spawnAnim = node.spawnAnim;
		npc.wanderDist = node.wander;
	}
	LOG("LevelEvent :: node %d calls %d x monster %d (tag %d) in area %d at t=%.1f", node.id, node.count, node.monster, node.tagID, node.areaID, GameTimeSec());
	if(node.tagID < 0) return;
	for(i32 slot = 0; slot < (i32)map->levelEvents.size(); slot++) {
		const GameXmlContent::LevelEventNode& n = map->levelEvents[slot];
		if(n.kind != GameXmlContent::LevelEventNode::DEAD_COUNT || n.tagID != node.tagID || deadCountsDone.find(slot) == deadCountsDone.end()) continue;
		deadCountsDone.erase(slot);
		deadCounts[slot] = 0;
		LOG("LevelEvent :: dead count of tag %d re-armed by a respawn (node %d)", node.tagID, n.id);
	}
}

void World::FireRandomCondition(const GameXmlContent::LevelEventNode& node, i32 playerIndex)
{
	i32 total = 0;
	u8 lastBranch = 0xff;
	foreach_const(e, node.randomEvents) {
		if(e->branch != lastBranch) total += e->weight;
		lastBranch = e->branch;
	}
	if(total <= 0) return;
	i32 roll = (i32)(RandUint() % (u32)total);
	i32 chosen = -1;
	lastBranch = 0xff;
	foreach_const(e, node.randomEvents) {
		if(e->branch == lastBranch) continue;
		lastBranch = e->branch;
		if(roll < e->weight) { chosen = e->branch; break; }
		roll -= e->weight;
	}
	foreach_const(e, node.randomEvents) {
		if(e->branch != chosen) continue;
		if(const GameXmlContent::LevelEventNode* target = map->FindLevelEventByName(e->nameKey)) FireLevelEvent(target->id, playerIndex);
	}
	LOG("LevelEvent :: random node %d picks branch %d", node.id, chosen);
}

void World::FireSwitchCondition(const GameXmlContent::LevelEventNode& node, i32 playerIndex)
{
	if(node.count <= 0) return;
	i32& step = switchSteps[node.id];
	if(step >= node.count) {
		if(!node.apply) return;
		step = 0;
	}
	const i32 index = step++;
	LOG("LevelEvent :: switch node %d step %d", node.id, index);
	foreach_const(e, node.randomEvents) {
		if(e->branch != index) continue;
		if(const GameXmlContent::LevelEventNode* target = map->FindLevelEventByName(e->nameKey)) FireLevelEvent(target->id, playerIndex, node.id);
	}
}

void World::FireRandomChoice(const GameXmlContent::LevelEventNode& node, i32 playerIndex)
{
	eastl::fixed_vector<i32,32,true> pool(node.outputs.begin(), node.outputs.end());
	const i32 picks = MIN(node.count, (i32)pool.size());
	LOG("LevelEvent :: random choice node %d picks %d of %d", node.id, picks, (i32)pool.size());
	for(i32 i = 0; i < picks; i++) {
		const i32 j = i + (i32)(RandUint() % (u32)(pool.size() - i));
		eastl::swap(pool[i], pool[j]);
	}
	for(i32 i = 0; i < picks; i++) FireLevelEvent(pool[i], playerIndex, node.id);
}

static const u32 LEVEL_EVENT_QUEUE_CAP = 4096;
static const i32 DIALOG_END_AI_TRIGGER = 9999;
static const f32 DIALOG_AUTO_SKIP_SEC = 4.0f;
static const f32 DIALOG_END_MARGIN_SEC = 1.0f;

f32 World::DialogChainSec(i32 nodeID, eastl::fixed_vector<i32,64,true>& visited) const
{
	if(eastl::find(visited.begin(), visited.end(), nodeID) != visited.end()) return 0.0f;
	visited.push_back(nodeID);
	const GameXmlContent::LevelEventNode* node = map->FindLevelEvent(nodeID);
	if(!node) return 0.0f;
	f32 self = 0.0f;
	if(node->kind == GameXmlContent::LevelEventNode::DELAY) self = node->delay;
	else if(node->dialogLine) self = DIALOG_AUTO_SKIP_SEC;
	else if(node->kind != GameXmlContent::LevelEventNode::CLIENT_EVENT) return 0.0f;
	f32 longest = 0.0f;
	foreach_const(o, node->outputs) longest = MAX(longest, DialogChainSec(*o, visited));
	return self + longest;
}

void World::FireDialogTrigger(i32 dialogID, i32 playerIndex)
{
	bool endTrigger = false;
	foreach_const(n, map->levelEvents) if(n->kind == GameXmlContent::LevelEventNode::AI_TRIGGER && n->aiIndex == DIALOG_END_AI_TRIGGER && NodeInDifficulty(*n)) endTrigger = true;
	foreach_const(n, map->levelEvents) {
		if(n->kind != GameXmlContent::LevelEventNode::DIALOG_TRIGGER || n->aiIndex != dialogID || !NodeInDifficulty(*n)) continue;
		foreach_const(o, n->outputs) FireLevelEvent(*o, playerIndex, n->id);
		if(!endTrigger || dialogEnds.full()) continue;
		eastl::fixed_vector<i32,64,true> visited;
		f32 sec = 0.0f;
		foreach_const(o, n->outputs) sec = MAX(sec, DialogChainSec(*o, visited));
		dialogEnds.push_back({ TimeAdd(localTime, TimeMsToTime((sec + DIALOG_END_MARGIN_SEC) * 1000.0)), playerIndex });
		LOG("LevelEvent :: dialog %d (node %d) ends in %.1f s", dialogID, n->id, sec + DIALOG_END_MARGIN_SEC);
	}
}

void World::DespawnEntities(i32 localID, i32 tagID)
{
	foreach(npc, actorNpcList) {
		if(npc->ownerUID != ActorUID::INVALID || npc->removeAt != Time::ZERO) continue;
		if(tagID >= 0 ? npc->tagID != tagID : npc->localID != localID) continue;
		if(npc->despawnAt == Time::ZERO || npc->despawnAt > localTime) npc->despawnAt = localTime;
	}
	foreach(d, actorDynamicList) {
		if(tagID >= 0 ? d->tagID != tagID : d->localID != localID) continue;
		foreach(g, teamGates) if(g->localID == d->localID) g->open = true;
		if(d->destroyed) continue;
		d->destroyed = true;
		if(d->collider) physics.SetStaticEnabled(d->collider, false);
		replication->SendStatToAll(d->UID, (i32)StatType::STAT_TYPE_HEALTH, 0, 1);
		replication->SendDespawnMonsterToAll(d->UID);
	}
	LOG("LevelEvent :: despawn local %d tag %d", localID, tagID);
}

static const i32 AI_TRIGGER_SAVE_TITAN_PIECE = 3333;

void World::FireAITrigger(i32 index, i32 playerIndex)
{
	foreach_const(n, map->levelEvents) {
		if(n->kind != GameXmlContent::LevelEventNode::AI_TRIGGER || n->aiIndex != index) continue;
		LOG("LevelEvent :: AI trigger %d (node %d)", index, n->id);
		foreach_const(o, n->outputs) FireLevelEvent(*o, playerIndex, n->id);
	}
}

bool World::NodeDeactivated(const GameXmlContent::LevelEventNode& node) const
{
	if(node.kind == GameXmlContent::LevelEventNode::BOUND_TRIGGER || node.kind == GameXmlContent::LevelEventNode::OR_CONDITION) return false;
	return LevelNodeOverride(node) == 0;
}

static const i32 LEVEL_PASS_DEPTH_MAX = 32;

static bool LevelEventSource(GameXmlContent::LevelEventNode::Kind kind)
{
	switch(kind) {
		case GameXmlContent::LevelEventNode::LEVEL_START:
		case GameXmlContent::LevelEventNode::TAKE_ITEM:
		case GameXmlContent::LevelEventNode::BOUND_TRIGGER:
		case GameXmlContent::LevelEventNode::ON_HIT_TRIGGER:
		case GameXmlContent::LevelEventNode::DEAD_COUNT:
		case GameXmlContent::LevelEventNode::HP_TRIGGER:
		case GameXmlContent::LevelEventNode::GAME_END_TRIGGER:
		case GameXmlContent::LevelEventNode::AI_TRIGGER:
		case GameXmlContent::LevelEventNode::INTERACTION_TRIGGER:
		case GameXmlContent::LevelEventNode::DIALOG_TRIGGER:
		case GameXmlContent::LevelEventNode::START_PHASE:
		case GameXmlContent::LevelEventNode::END_PHASE:
		case GameXmlContent::LevelEventNode::CREATURE_ENTER:
			return true;
		default: return false;
	}
}

void World::FireLevelEvent(i32 nodeID, i32 playerIndex, i32 fromNode)
{
	const GameXmlContent::LevelEventNode* node = map->FindLevelEvent(nodeID);
	if(!node || NodeDeactivated(*node)) return;
	if(!NodeInDifficulty(*node)) {
		if(LevelEventSource(node->kind) || levelPassDepth >= LEVEL_PASS_DEPTH_MAX) return;
		levelPassDepth++;
		foreach_const(o, node->outputs) FireLevelEvent(*o, playerIndex, node->id);
		levelPassDepth--;
		return;
	}
	if(fromNode >= 0 && difficultyBit != GameXmlContent::ALL_DIFFICULTIES) {
		const GameXmlContent::LevelEventNode* from = map->FindLevelEvent(fromNode);
		bool fromInDifficulty = !from || NodeInDifficulty(*from);
		if(!fromInDifficulty) foreach_const(n, map->levelEvents) if(n->id == fromNode && NodeInDifficulty(*n)) { fromInDifficulty = true; break; }
		if(fromInDifficulty) {
			foreach_const(in, node->inputs) if(in->first == fromNode && !(in->second & difficultyBit)) return;
		}
	}
	struct SeqRestore { bool& flag; bool value; ~SeqRestore() { flag = value; } } seqRestore{ levelClientSeq, levelClientSeq };
	const GameXmlContent::LevelEventNode* trigger = node->seqRoot && fromNode >= 0 ? map->FindLevelEvent(fromNode) : nullptr;
	if(trigger && (LevelEventSource(trigger->kind) || trigger->kind == GameXmlContent::LevelEventNode::TIME_RECORD_START) && !levelClientSeq && replication->titanMode) {
		replication->SendClientLevelEventSeqToAll(node->id, playerIndex >= 0 ? players[playerIndex].Main().UID : ActorUID::INVALID);
		levelClientSeq = true;
	}
	switch(node->kind) {
		case GameXmlContent::LevelEventNode::SET_ACTIVATION: SetLevelTriggerActive(node->targetKey, node->active); break;
		case GameXmlContent::LevelEventNode::AND_CONDITION: {
			auto& fired = andInputsFired[node->id];
			if(fromNode >= 0 && eastl::find(fired.begin(), fired.end(), fromNode) == fired.end() && !fired.full()) fired.push_back(fromNode);
			foreach_const(in, node->inputs) {
				if(difficultyBit != GameXmlContent::ALL_DIFFICULTIES && !(in->second & difficultyBit)) continue;
				if(eastl::find(fired.begin(), fired.end(), in->first) == fired.end()) return;
			}
			if(node->resetAfter) fired.clear();
			LOG("LevelEvent :: and node %d complete", node->id);
		} break;
		case GameXmlContent::LevelEventNode::RANDOM_CONDITION: FireRandomCondition(*node, playerIndex); return;
		case GameXmlContent::LevelEventNode::PORTAL: {
			portalRequested = true;
			LOG("LevelEvent :: portal (node %d)", node->id);
		} break;
		case GameXmlContent::LevelEventNode::KILL_TARGET: {
			foreach(npc, actorNpcList) {
				if(npc->hp <= 0 || npc->removeAt != Time::ZERO || npc->ownerUID != ActorUID::INVALID) continue;
				if(node->tagID >= 0 ? npc->tagID != node->tagID : npc->localID != node->localID) continue;
				MonsterDie(*npc, ActorUID::INVALID, CreatureIndex(0));
			}
		} break;
		case GameXmlContent::LevelEventNode::ACTIVE_AREA_TRIGGER: {
			foreach_const(n, map->levelEvents) {
				if(n->kind == GameXmlContent::LevelEventNode::BOUND_TRIGGER && n->areaID == node->areaID) SetLevelTriggerActive(n->nameKey, node->active);
			}
		} break;
		case GameXmlContent::LevelEventNode::INVOKE_DIALOG: {
			if(!levelClientSeq) replication->SendClientLevelEventToAll(node->id, playerIndex >= 0 ? players[playerIndex].Main().UID : ActorUID::INVALID);
			FireDialogTrigger(node->aiIndex, playerIndex);
		} break;
		case GameXmlContent::LevelEventNode::DIALOG_TRIGGER: return;
		case GameXmlContent::LevelEventNode::DELAY: {
			const f32 jitter = node->variance > 0 ? ((f32)(RandUint() % 2001) / 1000.0f - 1.0f) * node->variance : 0.0f;
			const Time at = TimeAdd(localTime, TimeMsToTime(MAX(node->delay + jitter, 0.0f) * 1000.0));
			foreach_const(o, node->outputs) {
				if(levelEventTimers.size() < LEVEL_EVENT_QUEUE_CAP) levelEventTimers.push_back({ *o, at, playerIndex, node->id, levelClientSeq });
				else LOG("LevelEvent :: timer queue full, node %d dropped", *o);
			}
			return;
		}
		case GameXmlContent::LevelEventNode::CALL_SFX: if(!levelClientSeq) replication->SendClientLevelEventToAll(node->id); break;
		case GameXmlContent::LevelEventNode::CLIENT_EVENT:
			if(!node->clientDriven && !levelClientSeq) replication->SendClientLevelEventToAll(node->id, playerIndex >= 0 ? players[playerIndex].Main().UID : ActorUID::INVALID);
			if(node->initTagCool && playerIndex >= 0) SendTagCooltimeToTeam(players[playerIndex], 0);
			break;
		case GameXmlContent::LevelEventNode::CALL_ITEM: SpawnGroundItems(node->dropIndex, node->areaID, node->tagID); break;
		case GameXmlContent::LevelEventNode::STATUS_CHANGE: {
			if(node->applyType == GameXmlContent::LevelEventNode::APPLY_MY_CHAR) {
				if(node->apply && playerIndex >= 0 && node->statusIndex != 0 && !players[playerIndex].dead) {
					statusNoCaster = true;
					ApplyStatus(players[playerIndex], players[playerIndex], node->statusIndex, 0, LevelStatusDuration(node->statusIndex));
					statusNoCaster = false;
				}
				else if(!node->apply && playerIndex >= 0 && node->statusIndex != 0) ConsumeStatus((u32)playerIndex, node->statusIndex);
			}
			else if(node->applyType == GameXmlContent::LevelEventNode::APPLY_ALL_PARTY && node->statusIndex != 0) {
				foreach(p, players) {
					if(!p->body || p->removed || ((node->team == Faction::RED || node->team == Faction::BLUE) && (Faction)p->team != node->team)) continue;
					if(node->apply) { if(!p->dead) { statusNoCaster = true; ApplyStatus(*p, *p, node->statusIndex, 0, LevelStatusDuration(node->statusIndex)); statusNoCaster = false; } }
					else ConsumeStatus(p->index, node->statusIndex);
				}
			}
			else if(node->applyType == GameXmlContent::LevelEventNode::APPLY_LOCAL_ID && node->statusIndex != 0) {
				foreach(npc, actorNpcList) {
					if(npc->ownerUID != ActorUID::INVALID || npc->removeAt != Time::ZERO) continue;
					if(node->tagID >= 0 ? npc->tagID != node->tagID : npc->localID != node->localID) continue;
					ApplyMonsterStatus(*npc, node->statusIndex, node->apply);
				}
			}
		} break;
		case GameXmlContent::LevelEventNode::SAFE_ZONE: {
			if(playerIndex < 0) break;
			Player& p = players[playerIndex];
			p.inSafeZone = node->active;
			if(node->active && p.clientHd == ClientHandle::INVALID) BotSpendTraits(p);
			if(p.clientHd != ClientHandle::INVALID) replication->SendSafeZone(p.clientHd, p.userID, node->active);
		} break;
		case GameXmlContent::LevelEventNode::INVOKE_AI_TRIGGER: FireAITrigger(node->aiIndex, playerIndex); break;
		case GameXmlContent::LevelEventNode::INTERACTION_TRIGGER: return;
		case GameXmlContent::LevelEventNode::CAMP_GUARD: {
			if(playerIndex < 0) break;
			Player& p = players[playerIndex];
			if(node->apply) {
				p.campStartArea = node->localID;
				p.campDestArea = node->areaID;
				p.campTeleportAt = TimeAdd(localTime, TimeMsToTime((node->delay + node->callDelay) * 1000.0));
				LOG("LevelEvent :: %ls back in starting area %d, moved to area %d in %.0f s", p.name.data(), node->localID, node->areaID, node->delay + node->callDelay);
			}
			else if(p.campStartArea == node->localID) p.campTeleportAt = Time::ZERO;
		} break;
		case GameXmlContent::LevelEventNode::TELEPORT_TO: {
			const GameXmlContent::Map::Area* area = map->FindArea(node->areaID);
			if(!area) break;
			const vec3 dest = area->pos + vec3(0, 0, LEVEL_TELEPORT_LIFT);
			foreach(p, players) {
				if(p->dead || !p->body || (!node->apply && (i32)p->index != playerIndex)) continue;
				WarpPlayer(*p, dest, area->rot.z);
				LOG("LevelEvent :: %ls teleported to area %d", p->name.data(), node->areaID);
			}
		} break;
		case GameXmlContent::LevelEventNode::ACTIVE_INTERACTION: {
			if(!levelClientSeq) replication->SendClientLevelEventToAll(node->id, playerIndex >= 0 ? players[playerIndex].Main().UID : ActorUID::INVALID);
			BuildMapInteractions();
			foreach(m, mapInteractions) {
				if(m->localID != node->localID) continue;
				m->active = node->active;
				if(node->active) { m->occupiedTeam = INTERACTION_NOT_OCCUPIED; m->coolUntil = Time::ZERO; }
				SendMapInteractionStatus(*m);
			}
		} break;
		case GameXmlContent::LevelEventNode::ADD_CAST_TIME: {
			BuildMapInteractions();
			foreach(m, mapInteractions) {
				if(m->localID != node->localID) continue;
				for(i32 team = 0; team < 2; team++) {
					if(node->team == Faction::MONSTER || (i32)node->team == team) m->extraCastMs[team] += (i32)(node->delay * 1000.0f);
				}
			}
		} break;
		case GameXmlContent::LevelEventNode::NOTIFY_RESPAWN: replication->SendMonsterRespawnTimeToAll((u32)node->count, (u32)node->localID, (u32)(node->delay * 1000.0f)); break;
		case GameXmlContent::LevelEventNode::TEAM_CONDITION: {
			if(playerIndex < 0 || node->team == Faction::INVALID || (Faction)players[playerIndex].team != node->team) return;
		} break;
		case GameXmlContent::LevelEventNode::ADD_GOT_PIECE: {
			if(node->team == Faction::RED || node->team == Faction::BLUE) AddTeamPieces((i32)node->team, node->count);
		} break;
		case GameXmlContent::LevelEventNode::SAVE_GOT_PIECE: {
			if(playerIndex < 0) break;
			Player& p = players[playerIndex];
			if(p.titanPieces <= 0) break;
			const i32 saved = p.titanPieces;
			p.savedPieces += saved;
			static const u32 SCORE_GOT_SAVE_COUNT = 39;
			if(map && map->ingameLevels) replication->SendPlayerScoreToAll(p.userID, SCORE_GOT_SAVE_COUNT, p.savedPieces);
			SetCarriedPieces(p, 0);
			AddTeamPieces((i32)p.team, saved);
			LOG("Titan :: %ls saves %d piece(s)", p.name.data(), saved);
			FireAITrigger(AI_TRIGGER_SAVE_TITAN_PIECE, playerIndex);
		} break;
		case GameXmlContent::LevelEventNode::OR_CONDITION: {
			if(!LevelTriggerActive(*node)) return;
			if(node->resetAfter) SetLevelTriggerActive(node->nameKey, false);
		} break;
		case GameXmlContent::LevelEventNode::DEAD_COUNT:
		case GameXmlContent::LevelEventNode::HP_TRIGGER:
		case GameXmlContent::LevelEventNode::GAME_END_TRIGGER:
		case GameXmlContent::LevelEventNode::AI_TRIGGER: return;
		case GameXmlContent::LevelEventNode::ACTION_CHANGE: {
			foreach(d, actorDynamicList) {
				if(d->localID != node->localID || node->action == ActionStateID::INVALID) continue;
				d->action = node->action;
				d->tLastActionChange = localTime;
				d->hits.clear();
				physics.SetStaticEnabled(d->collider, node->action != ActionStateID::DYNAMIC_OPEN);
				foreach(g, teamGates) if(g->localID == d->localID) g->open = node->action == ActionStateID::DYNAMIC_OPEN || node->action == ActionStateID::DYNAMIC_DIE;
				LOG("LevelEvent :: dynamic %d action %d", d->localID, (i32)node->action);
			}
		} break;
		case GameXmlContent::LevelEventNode::CALL_MONSTER: {
			if(difficultyBit != GameXmlContent::ALL_DIFFICULTIES && !(node->stageLevelBit & difficultyBit)) break;
			const f32 jitter = node->variance > 0 ? ((f32)(RandUint() % 2001) / 1000.0f - 1.0f) * node->variance : 0.0f;
			if(monsterCalls.size() < LEVEL_EVENT_QUEUE_CAP) monsterCalls.push_back({ node->id, TimeAdd(localTime, TimeMsToTime(MAX(node->callDelay + jitter, 0.0f) * 1000.0)), node->callCount, playerIndex });
		} break;
		case GameXmlContent::LevelEventNode::DESPAWN_ENTITY: DespawnEntities(node->localID, node->tagID); break;
		case GameXmlContent::LevelEventNode::PASS_THROUGH: break;
		case GameXmlContent::LevelEventNode::ACTIVATE_TRIGGER: SetLevelTriggerActive(node->targetKey, node->active); break;
		case GameXmlContent::LevelEventNode::ACTIVE_EVENTS: SetLevelGroupActive(node->targetKey, node->active); break;
		case GameXmlContent::LevelEventNode::SWITCH_CONDITION: FireSwitchCondition(*node, playerIndex); return;
		case GameXmlContent::LevelEventNode::RANDOM_CHOICE: FireRandomChoice(*node, playerIndex); return;
		case GameXmlContent::LevelEventNode::CREATURE_ENTER: return;
		case GameXmlContent::LevelEventNode::AI_PAUSE: LOG("LevelEvent :: AI pause %d not supported, passing through (node %d)", node->apply ? 1 : 0, node->id); break;
		case GameXmlContent::LevelEventNode::EXECUTE_LUA: LOG("LevelEvent :: Lua script not supported, passing through (node %d, target %d, area %d)", node->id, node->localID, node->areaID); break;
		case GameXmlContent::LevelEventNode::PVP_TAKE_SCORE: LOG("LevelEvent :: team %d takes %d score not supported, passing through (node %d)", (i32)node->team, node->count, node->id); break;
		default: return;
	}
	foreach_const(o, node->outputs) FireLevelEvent(*o, playerIndex, node->id);
}

void World::SpawnGroundItems(i32 dropIndex, i32 areaID, i32 tagID)
{
	const auto& content = GetGameXmlContent();
	const GameXmlContent::Map::Area* area = map->FindArea(areaID);
	if(!content.FindDropGroup(dropIndex) || !area) {
		LOG("GroundItem :: drop group %d or area %d not found", dropIndex, areaID);
		return;
	}
	SpawnDropGroupAt(dropIndex, area->pos, tagID);
}

void World::SpawnDropGroupAt(i32 dropIndex, const vec3& pos, i32 tagID)
{
	GameXmlContent::DropRollList rolls;
	GetGameXmlContent().RollDrop(dropIndex, &rolls);
	foreach_const(r, rolls) {
		if(r->ron) continue;
		for(i32 k = 0; k < r->count; k++) SpawnGroundItemAt(r->item, pos, tagID);
	}
}

void World::SpawnPrivateDrops(const ActorNpc& npc)
{
	const GameXmlContent& content = GetGameXmlContent();
	const u8 rank = content.FindMonsterRank((i32)npc.docID);
	i32 dropIndex = 0;
	foreach_const(d, stageCommonDrop) if(d->first == rank) { dropIndex = d->second; break; }
	if(dropIndex == 0) dropIndex = content.FindMonsterDrop((i32)npc.docID);
	if(dropIndex == 0) return;
	foreach_const(p, players) {
		if(p->clientHd == ClientHandle::INVALID || p->removed) continue;
		SpawnPrivateDropGroup(*p, dropIndex, npc.pos);
	}
}

void World::SpawnPrivateDropGroup(const Player& owner, i32 dropIndex, const vec3& pos)
{
	GameXmlContent::DropRollList rolls;
	GetGameXmlContent().RollDrop(dropIndex, &rolls);
	foreach_const(r, rolls) {
		if(groundItems.full()) return;
		GroundItem item;
		item.uid = nextGroundItemUID++;
		item.itemIndex = r->item;
		item.pos = pos;
		item.count = r->count;
		item.owner = (i32)owner.index;
		item.ron = r->ron;
		item.expireAt = TimeAdd(localTime, TimeMsToTime(RON_ITEM_LIFETIME_SEC * 1000.0));
		groundItems.push_back(item);
		replication->SendCreatePrivateGroundItem(owner.clientHd, owner.Main().UID, item.uid, item.ron, item.itemIndex, item.count, item.pos);
	}
	if(!rolls.empty()) LOG("GroundItem :: drop %d gives %d item(s) to %ls", dropIndex, (i32)rolls.size(), owner.name.data());
}

void World::SpawnGroundItemAt(i32 itemIndex, const vec3& pos, i32 tagID)
{
	const GameXmlContent::GroundItemInfo* info = GetGameXmlContent().FindGroundItem(itemIndex);
	if(!info || groundItems.full()) return;
	GroundItem item;
	item.uid = nextGroundItemUID++;
	item.itemIndex = itemIndex;
	item.pos = pos;
	item.tagID = tagID;
	item.expireAt = info->lifeTime > 0 ? TimeAdd(localTime, TimeMsToTime(info->lifeTime * 1000.0)) : Time::ZERO;
	groundItems.push_back(item);
	replication->SendCreateGroundItemToAll(item.uid, item.itemIndex, item.pos);
	LOG("GroundItem :: item %d spawned at (%.0f, %.0f) tag %d", item.itemIndex, item.pos.x, item.pos.y, tagID);
}

static const i32 TITAN_PIECE_ITEM = 130100904;
static const u32 PVP_ANNOUNCE_PIECE_CARRIED = 13;
static const u32 PVP_ANNOUNCE_TEAM_PIECES = 14;

void World::SetCarriedPieces(Player& player, i32 count)
{
	player.titanPieces = count;
	replication->SendPvpAnnouncementToAll(PVP_ANNOUNCE_PIECE_CARRIED, 0, player.Main().UID, ActorUID::INVALID, (u32)count);
}

void World::AddTeamPieces(i32 team, i32 count)
{
	if(team < 0 || team > 1) return;
	teamTitanPieces[team] = MAX(teamTitanPieces[team] + count, 0);
	const u32 teamType = (u32)(team == 0 ? TeamType::RED : TeamType::BLUE);
	foreach_const(p, players) {
		if(p->team == team && p->clientHd != ClientHandle::INVALID) replication->SendPvpAnnouncementRaw(p->clientHd, PVP_ANNOUNCE_TEAM_PIECES, 0, (u32)teamTitanPieces[team], teamType);
	}
	LOG("Titan :: team %d has %d piece(s)", team, teamTitanPieces[team]);
}

void World::DropTitanPieces(Player& player)
{
	if(player.titanPieces <= 0) return;
	const vec3 pos = player.body ? player.body->GetWorldPos() : vec3(0);
	for(i32 k = 0; k < player.titanPieces; k++) SpawnGroundItemAt(TITAN_PIECE_ITEM, pos, -1);
	LOG("Titan :: %ls drops %d piece(s)", player.name.data(), player.titanPieces);
	SetCarriedPieces(player, 0);
}

static const i32 MEASURED_MASTER_SOUL_ITEM = 130100903;
static const i32 MEASURED_GETTABLE_TEAM = 1;
static const i32 MEASURED_NETWORK_TEAM_BASE = 3;

void World::DropMasterSoul(const Player& dead)
{
	if(!replication->titanMode || !dead.body || groundItems.full()) return;
	const GameXmlContent::GroundItemInfo* info = GetGameXmlContent().FindGroundItem(MEASURED_MASTER_SOUL_ITEM);
	if(!info) return;
	GroundItem item;
	item.uid = nextGroundItemUID++;
	item.itemIndex = MEASURED_MASTER_SOUL_ITEM;
	item.pos = dead.body->GetWorldPos();
	item.team = (i32)dead.team;
	item.expireAt = info->lifeTime > 0 ? TimeAdd(localTime, TimeMsToTime(info->lifeTime * 1000.0)) : Time::ZERO;
	groundItems.push_back(item);
	replication->SendCreateGroundItemToAll(item.uid, item.itemIndex, item.pos, MEASURED_GETTABLE_TEAM, MEASURED_NETWORK_TEAM_BASE + (i32)dead.team);
	LOG("GroundItem :: %ls drops a master soul", dead.name.data());
}

static f32 ItemStatMax(const World::Player& player, u8 type)
{
	const ClassType cls = player.Main().classType;
	if(type == (u8)StatType::STAT_TYPE_HEALTH) return (f32)player.Main().hpMax;
	return MasterStat(cls, type);
}

bool World::TakeGroundItem(Player& player, u32 itemUID, GroundItem* taken)
{
	auto found = eastl::find_if(groundItems.begin(), groundItems.end(), [itemUID](const GroundItem& g) { return g.uid == itemUID; });
	if(found == groundItems.end() || player.dead) return false;
	if(found->owner >= 0 && found->owner != (i32)player.index) return false;
	if(found->team >= 0 && found->team != (i32)player.team) return false;
	const GroundItem item = *found;
	groundItems.erase(found);
	if(taken) *taken = item;
	const GameXmlContent::GroundItemInfo* info = item.ron ? nullptr : GetGameXmlContent().FindGroundItem(item.itemIndex);
	if(item.owner >= 0 && !info) {
		LOG("GroundItem :: %ls takes private drop %d x%d%s", player.name.data(), item.itemIndex, item.count, item.ron ? " (ron)" : "");
		return true;
	}
	if(info) {
		auto apply = [&](u8 type, f32 amount) {
			const f32 max = ItemStatMax(player, type);
			if(max <= 0 || amount == 0) return;
			const ActorUID uid = player.Main().UID;
			if(type == (u8)StatType::STAT_TYPE_HEALTH) {
				ActorMaster& chara = player.Main();
				chara.hp = MIN(chara.hp + (i32)amount, chara.hpMax);
				replication->SendStatToAll(uid, type, (f32)chara.hp, (f32)chara.hpMax, STAT_REASON_ITEM);
			}
			else if(type == (u8)StatType::STAT_TYPE_ULTIMATE) {
				player.ultGauge = MIN(MAX(player.ultGauge, 0.0f) + amount, max);
				replication->SendStatToAll(uid, type, player.ultGauge, max, STAT_REASON_ITEM);
			}
			else if(type == (u8)StatType::STAT_TYPE_MANA) {
				player.mana = MIN(MAX(player.mana, 0.0f) + amount, max);
				replication->SendStatToAll(uid, type, player.mana, max, STAT_REASON_ITEM);
			}
			else if(type == (u8)StatType::STAT_TYPE_STAMINA) {
				player.stamina = MIN(MAX(player.stamina, 0.0f) + amount, max);
				replication->SendStatToAll(uid, type, player.stamina, max, STAT_REASON_ITEM);
			}
			else {
				for(int r = 0; r < 3; r++) {
					if(RESOURCES[r].maxStat != (i32)type) continue;
					player.res[r] = MIN(MAX(player.res[r], 0.0f) + amount, max);
					replication->SendStatToAll(uid, type, player.res[r], max, STAT_REASON_ITEM);
				}
			}
		};
		foreach_const(m, info->statMul) apply(m->type, ItemStatMax(player, m->type) * m->value / 100.0f);
		foreach_const(a, info->statAdd) apply(a->type, a->value);
		foreach_const(st, info->statuses) ApplyStatus(player, player, *st);
	}
	if(item.owner < 0) replication->SendPublicGroundItemTaken(player.clientHd, player.Main().UID, itemUID);
	else if(player.clientHd != ClientHandle::INVALID) replication->SendGroundItemTaken(player.clientHd, player.Main().UID, itemUID);
	LOG("GroundItem :: %ls takes item %d (tag %d)", player.name.data(), item.itemIndex, item.tagID);
	if(item.itemIndex == TITAN_PIECE_ITEM) SetCarriedPieces(player, player.titanPieces + 1);
	foreach_const(n, map->levelEvents) {
		if(n->kind != GameXmlContent::LevelEventNode::TAKE_ITEM || n->tagID != item.tagID || item.tagID < 0) continue;
		foreach_const(o, n->outputs) FireLevelEvent(*o, -1, n->id);
	}
	return true;
}

void World::OnDynamicHit(const PxRigidActor* actor, u32 remoteID, ClientHandle seedOwnerHd, ClientHandle skipHd, const vec3& pos, ActorDynamic* known, i32 ownerIndex, i32 damage, RemoteIdx remoteDoc)
{
	ActorDynamic* hit = known;
	if(!hit && actor) foreach(d, actorDynamicList) {
		if(d->collider && (const void*)d->collider == (const void*)actor) { hit = &*d; break; }
	}
	if(!hit || hit->destroyed) return;
	const auto& destroyHits = GetGameXmlContent().dynamicDestroyHits;
	const auto destroyable = destroyHits.find((i32)hit->docID);
	if(destroyable != destroyHits.end() && GetGameXmlContent().dynamicJumpable.count((i32)hit->docID)) {
		const bool byPlayer = ownerIndex >= 0 && ownerIndex < (i32)players.size();
		if(damage > 0) DamageJumpWall(*hit, damage, false, byPlayer ? players[ownerIndex].Main().UID : ActorUID::INVALID, byPlayer ? ownerIndex : -1, remoteID, remoteDoc, pos, seedOwnerHd);
	}
	else if(destroyable != destroyHits.end() && ++hit->destroyHits >= destroyable->second) {
		i32 killerIndex = -1;
		foreach_const(p, players) if(p->clientHd == seedOwnerHd && seedOwnerHd != ClientHandle::INVALID) killerIndex = (i32)p->index;
		DestroyDynamic(*hit, killerIndex);
	}
	if(remoteID != 0) {
		const auto& densities = GetGameXmlContent().dynamicDensity;
		const auto density = densities.find((i32)hit->docID);
		replication->SendRemoteActivatedToAll(remoteID, seedOwnerHd, skipHd, hit->UID, density != densities.end() ? (u32)density->second : 0, pos);
	}
	foreach_const(n, map->levelEvents) {
		if(n->kind != GameXmlContent::LevelEventNode::ON_HIT_TRIGGER || n->localID != hit->localID || n->hitCount <= 0) continue;
		while(!hit->hits.empty() && TimeDiffSec(TimeDiff(hit->hits.front(), localTime)) > n->delay) hit->hits.erase(hit->hits.begin());
		if(hit->hits.full()) hit->hits.erase(hit->hits.begin());
		hit->hits.push_back(localTime);
		if((i32)hit->hits.size() < n->hitCount) return;
		hit->hits.clear();
		LOG("LevelEvent :: dynamic %d hit %d times in %.0f s", hit->localID, n->hitCount, n->delay);
		foreach_const(o, n->outputs) FireLevelEvent(*o, -1, n->id);
		return;
	}
}

void World::DamageJumpWall(ActorDynamic& wall, i32 rawDamage, bool exact, ActorUID attackerUID, i32 killerIndex, u32 remoteID, RemoteIdx remoteDoc, const vec3& pos, ClientHandle seedOwnerHd)
{
	if(wall.destroyed || rawDamage <= 0) return;
	const i32 damage = exact ? rawDamage : MAX((i32)(rawDamage * DEFENCE_SCALE / (DEFENCE_SCALE + MAX(LaneMinionDefence(), 0.0f)) + 0.5f), 1);
	const i32 hpBefore = MAX((i32)MEASURED_JUMPWALL_MAX_HP - wall.damageTaken, 0);
	const i32 hp = MAX(hpBefore - damage, 0);
	wall.damageTaken += damage;
	wall.destroyHits++;
	replication->SendStatToAll(wall.UID, (i32)StatType::STAT_TYPE_HEALTH, (f32)hp, MEASURED_JUMPWALL_MAX_HP);
	if(attackerUID != ActorUID::INVALID && remoteID != 0) {
		Replication::RemoteDamage dealt;
		dealt.remoteID = remoteID;
		dealt.remoteDoc = remoteDoc;
		dealt.attackerUID = attackerUID;
		dealt.targetUID = wall.UID;
		dealt.remotePos = pos;
		dealt.targetPos = wall.pos;
		dealt.damage = MIN(damage, hpBefore);
		dealt.hp = hp;
		dealt.hpMax = (i32)MEASURED_JUMPWALL_MAX_HP;
		dealt.seedOwnerHd = seedOwnerHd;
		replication->SendRemoteDamageToAll(dealt);
	}
	LOG("LevelEvent :: dynamic %d (doc %d) wall takes %d hp=%d", wall.localID, (i32)wall.docID, MIN(damage, hpBefore), hp);
	if(hp <= 0) DestroyDynamic(wall, killerIndex);
}

bool World::LevelTriggerActive(const GameXmlContent::LevelEventNode& node) const
{
	if(!NodeInDifficulty(node)) return false;
	const i32 overridden = LevelNodeOverride(node);
	return overridden < 0 ? node.active : overridden == 1;
}

static const u32 INTERACTION_ALL_TEAMS = 0xffffffff;

void World::BuildMapInteractions()
{
	if(mapInteractionsBuilt || !map) return;
	mapInteractionsBuilt = true;
	const auto& content = GetGameXmlContent();
	foreach_const(d, actorDynamicList) {
		if(d->localID >= INTERACTION_OBJECT_LOCAL_BASE || mapInteractions.full()) continue;
		const i32 interaction = content.FindDynamicInteraction((i32)d->docID);
		const GameXmlContent::InteractionInfo* info = interaction ? content.FindInteraction(interaction) : nullptr;
		if(!info) continue;
		mapInteractions.push_back({ d->UID, d->localID, interaction, info->active, INTERACTION_NOT_OCCUPIED, Time::ZERO, {{ 0, 0 }}, INTERACTION_ALL_TEAMS, false, false });
	}
	foreach_const(m, mapInteractions) SendMapInteractionStatus(*m);
	LOG("Interaction :: %d map objects", (i32)mapInteractions.size());
}

void World::SendMapInteractionStatus(const MapInteraction& m)
{
	const bool cooling = m.coolUntil != Time::ZERO && localTime < m.coolUntil;
	const GameXmlContent::InteractionInfo* info = GetGameXmlContent().FindInteraction(m.interactionID);
	const bool contestable = info && info->pvpOccupy && m.occupiedTeam != INTERACTION_NOT_OCCUPIED;
	const bool open = m.active && !cooling && (m.occupiedTeam == INTERACTION_NOT_OCCUPIED || contestable);
	const u32 mask = contestable ? m.openMask & ~(1u << (u32)m.occupiedTeam) : m.openMask;
	const f32 cool = cooling ? (f32)TimeDiffSec(TimeDiff(localTime, m.coolUntil)) : 0.0f;
	replication->SendInteractionStatusToAll(m.object, open ? mask : 0, m.interactionID, m.occupiedTeam, ClientHandle::INVALID, cool);
}

void World::AddCreatureInteraction(const ActorNpc& npc, i32 interactionID, u8 ownerTeam)
{
	const GameXmlContent::InteractionInfo* info = GetGameXmlContent().FindInteraction(interactionID);
	if(!info) return;
	BuildMapInteractions();
	const u32 owned = ownerTeam < 2 ? info->ownerMask[ownerTeam] : 0;
	const MapInteraction entry = { npc.UID, npc.localID, interactionID, true, INTERACTION_NOT_OCCUPIED, Time::ZERO, {{ 0, 0 }}, owned != 0 ? owned : INTERACTION_ALL_TEAMS, true, true };
	foreach(m, mapInteractions) {
		if(!m->creature || m->object != ActorUID::INVALID) continue;
		*m = entry;
		return;
	}
	if(!mapInteractions.full()) mapInteractions.push_back(entry);
}

void World::AddDynamicInteraction(const ActorDynamic& dyn, i32 interactionID)
{
	const GameXmlContent::InteractionInfo* info = GetGameXmlContent().FindInteraction(interactionID);
	if(!info) return;
	BuildMapInteractions();
	if(mapInteractions.full()) return;
	mapInteractions.push_back({ dyn.UID, dyn.localID, interactionID, info->active, INTERACTION_NOT_OCCUPIED, Time::ZERO, {{ 0, 0 }}, INTERACTION_ALL_TEAMS, false, true });
}

bool World::MapInteractionPos(const MapInteraction& m, vec3* pos) const
{
	if(m.creature) {
		const ActorNpc* npc = FindNpcActor(m.object);
		if(!npc) return false;
		*pos = npc->pos;
		return true;
	}
	auto obj = actorDynamicMap.find(m.object);
	if(obj == actorDynamicMap.end()) return false;
	*pos = obj->second->pos;
	return true;
}

bool World::StartMapInteraction(Player& player, ActorUID target)
{
	BuildMapInteractions();
	i32 slot = -1;
	for(i32 i = 0; i < (i32)mapInteractions.size(); i++) if(mapInteractions[i].object == target) slot = i;
	if(slot < 0) return false;
	const MapInteraction& m = mapInteractions[slot];
	const GameXmlContent::InteractionInfo* info = GetGameXmlContent().FindInteraction(m.interactionID);
	vec3 objPos;
	const bool contestable = info && info->pvpOccupy && m.occupiedTeam != INTERACTION_NOT_OCCUPIED && m.occupiedTeam != (i32)PLAYER_FACTION_BASE + (i32)player.team && (m.coolUntil == Time::ZERO || localTime >= m.coolUntil);
	if(!info || !MapInteractionPos(m, &objPos) || !m.active || (m.occupiedTeam != INTERACTION_NOT_OCCUPIED && !contestable) || player.interactSlot >= 0) return true;
	if(!(m.openMask & (1u << (PLAYER_FACTION_BASE + player.team)))) return true;
	if(!info->individual) foreach_const(p, players) if(p->interactSlot == slot) return true;
	if(info->requireMinerals > minerals) {
		LOG("Interaction :: %ls lacks minerals for object %d (%d/%d)", player.name.data(), m.localID, minerals, info->requireMinerals);
		return true;
	}
	const vec2 here = vec2(player.body->GetWorldPos());
	if(info->activeDistance > 0 && glm::length(here - vec2(objPos)) > info->activeDistance + HitRadius(player)) {
		LOG("Interaction :: %ls too far from object %d", player.name.data(), m.localID);
		return true;
	}
	const i32 castMs = MAX(info->castMs + m.extraCastMs[player.team], 0);
	player.interactSlot = slot;
	player.interactFrom = here;
	player.interactEndAt = TimeAdd(localTime, TimeMsToTime((f64)castMs));
	replication->SendInteractionCastingToAll(player.Main().UID, m.object, (u32)ActionStateID::INTERACTION_CAST_BEHAVIORSTATE, (u32)castMs);
	LOG("Interaction :: %ls casts object %d (%d ms)", player.name.data(), m.localID, castMs);
	return true;
}

void World::CancelMapInteraction(Player& player)
{
	if(player.interactSlot < 0) return;
	replication->SendInteractionCancelToAll(player.Main().UID, mapInteractions[player.interactSlot].object);
	LOG("Interaction :: %ls cancels object %d", player.name.data(), mapInteractions[player.interactSlot].localID);
	player.interactSlot = -1;
}

void World::CompleteMapInteraction(Player& player)
{
	MapInteraction& m = mapInteractions[player.interactSlot];
	player.interactSlot = -1;
	const GameXmlContent::InteractionInfo* info = GetGameXmlContent().FindInteraction(m.interactionID);
	if(!info) return;
	if(!info->individual) {
		m.occupiedTeam = PLAYER_FACTION_BASE + (i32)player.team;
		if(info->repeat && info->repeatInterval > 0) m.coolUntil = TimeAdd(localTime, TimeMsToTime(info->repeatInterval * 1000.0));
		else m.active = false;
		SendMapInteractionStatus(m);
	}
	replication->SendInteractionExecuteOnlyToAll(player.Main().UID, m.object, (u32)ActionStateID::INTERACTION_EXECUTE_BEHAVIORSTAT);
	LOG("Interaction :: %ls takes object %d (team %d)", player.name.data(), m.localID, (i32)player.team);
	if(pilotTour.kind == 1 && pilotTour.localID == m.localID) PilotTourFinish("done");
	if(info->requireMinerals > 0) {
		minerals = MAX(minerals - info->requireMinerals, 0);
		SendMinerals();
	}
	enum : u32 { SCORE_OCCUPY_STATUE_COUNT = 29, SCORE_OCCUPY_SIGHT_COUNT = 30 };
	const bool gotScores = map && map->ingameLevels && !occupyMode;
	if(info->action == GameXmlContent::InteractionInfo::TOTEM && player.statueCaptures < 255) {
		player.statueCaptures++;
		if(gotScores) replication->SendPlayerScoreToAll(player.userID, SCORE_OCCUPY_STATUE_COUNT, player.statueCaptures);
	}
	const bool visionCapture = info->action == GameXmlContent::InteractionInfo::OCCUPY && gotScores;
	if(info->action == GameXmlContent::InteractionInfo::OCCUPY && player.visionCaptures < 255) {
		player.visionCaptures++;
		if(visionCapture) replication->SendPlayerScoreToAll(player.userID, SCORE_OCCUPY_SIGHT_COUNT, player.visionCaptures);
	}
	if(info->pvpOccupy && occupyMode) {
		m.occupier = (i32)player.index;
		m.nextOccupyTick = TimeAdd(localTime, TimeMsToTime(MEASURED_OCCUPY_TICK_MS));
		const auto obj = actorDynamicMap.find(m.object);
		bool mid = false;
		if(obj != actorDynamicMap.end()) for(i32 doc : MEASURED_OCCUPY_MID_POINTS) if((i32)obj->second->docID == doc) mid = true;
		const i32 points = mid ? MEASURED_OCCUPY_MID_CAPTURE_POINTS : (info->points > 0 ? info->points : MEASURED_OCCUPY_CAPTURE_POINTS);
		replication->SendPvpAnnouncementToAll(MEASURED_ANNOUNCE_OCCUPY_CAPTURE, 1, player.Main().UID, m.object, (u32)points);
		OccupyScore(player, points, true);
	}
	if(info->announcement != 0 && !(info->pvpOccupy && occupyMode)) {
		if(!m.creature) player.occupyStreak++;
		replication->SendPvpAnnouncementToAll(info->announcement, m.creature ? 0 : player.occupyStreak, player.Main().UID, m.object, (u32)info->points);
		if(visionCapture && info->points > 0 && info->requireMinerals == 0) {
			player.points += info->points;
			player.visionPoints += info->points;
			const i32 teamPoints = TeamPoints(player.team);
			replication->SendOccupyScoreToAll(player.userID, player.team, player.visionCaptures, player.visionPoints, player.points, TeamHeldVision(player.team), teamPoints, TeamHeldVision(player.team ^ 1), true);
			OnTeamPointsChanged(player.team, teamPoints - info->points, teamPoints);
		}
		else if(info->points > 0 && info->requireMinerals == 0) AddPoints(player, info->points);
	}
	if(!m.creature && !stageCommonDrop.empty()) {
		auto obj = actorDynamicMap.find(m.object);
		const i32 drop = obj != actorDynamicMap.end() ? GetGameXmlContent().FindDynamicDrop((i32)obj->second->docID, difficultyBit) : 0;
		if(drop != 0) {
			const vec3 pos = obj->second->pos;
			replication->SendStatToAll(m.object, (i32)StatType::STAT_TYPE_HEALTH, 0, 1, 0);
			replication->SendNpcDeadAckToAll(m.object, player.Main().UID, (u32)CreatureIndex(100000000 + (i32)player.Main().classType));
			foreach_const(p, players) if(p->clientHd != ClientHandle::INVALID && !p->removed) SpawnPrivateDropGroup(*p, drop, pos);
			if(m.active) {
				m.active = false;
				SendMapInteractionStatus(m);
			}
			LOG("Interaction :: %ls opens dynamic %d (drop %d)", player.name.data(), m.localID, drop);
		}
	}
	if(m.creature) {
		if(ActorNpc* npc = FindNpcActor(m.object)) {
			GrantMonsterKillExp(*npc, player.Main().UID);
			npc->killerUID = player.Main().UID;
			npc->despawnAt = localTime;
			LOG("Interaction :: %ls destroys creature %d", player.name.data(), (i32)npc->docID);
		}
		m.object = ActorUID::INVALID;
		m.active = false;
	}
	foreach_const(n, map->levelEvents) {
		if(n->kind != GameXmlContent::LevelEventNode::INTERACTION_TRIGGER || n->localID != m.localID) continue;
		foreach_const(o, n->outputs) FireLevelEvent(*o, (i32)player.index, n->id);
	}
}


i32 World::TeamHeldVision(i32 team) const
{
	i32 held = 0;
	foreach_const(m, mapInteractions) {
		const GameXmlContent::InteractionInfo* info = GetGameXmlContent().FindInteraction(m->interactionID);
		if(info && info->action == GameXmlContent::InteractionInfo::OCCUPY && m->occupiedTeam == (i32)PLAYER_FACTION_BASE + team) held++;
	}
	return held;
}

i32 World::TeamHeldPoints(i32 team) const
{
	i32 held = 0;
	foreach_const(m, mapInteractions) {
		const GameXmlContent::InteractionInfo* info = GetGameXmlContent().FindInteraction(m->interactionID);
		if(info && info->pvpOccupy && m->occupiedTeam == (i32)PLAYER_FACTION_BASE + team) held++;
	}
	return held;
}

void World::OccupyScore(Player& player, i32 points, bool capture)
{
	player.points += points;
	player.occupyScore += points;
	if(capture && player.occupyCaptures < 255) player.occupyCaptures++;
	replication->SendOccupyScoreToAll(player.userID, player.team, player.occupyCaptures, player.occupyScore, player.points, TeamHeldPoints(player.team), TeamPoints(player.team), TeamHeldPoints(player.team ^ 1), capture);
}

bool World::OccupyTarget(const Player& bot, vec2* point, ActorUID* object, f32* reach) const
{
	static const f32 HYP_BOT_TOTEM_SEARCH = 3500.0f;
	static const i32 HYP_BOT_TOTEM_ROLE_EVERY = 3;
	const bool titanTotems = !occupyMode && map && map->ingameLevels && bot.clientHd == ClientHandle::INVALID && bot.index % HYP_BOT_TOTEM_ROLE_EVERY == 0;
	if(!occupyMode && !titanTotems) return false;
	const vec2 here = vec2(bot.body->GetWorldPos());
	f32 best = titanTotems ? HYP_BOT_TOTEM_SEARCH : 1e9f;
	bool found = false;
	foreach_const(m, mapInteractions) {
		const GameXmlContent::InteractionInfo* info = GetGameXmlContent().FindInteraction(m->interactionID);
		if(!info || !m->active || m->occupiedTeam == (i32)PLAYER_FACTION_BASE + (i32)bot.team) continue;
		if(occupyMode ? !info->pvpOccupy : ((info->action != GameXmlContent::InteractionInfo::TOTEM && info->action != GameXmlContent::InteractionInfo::OCCUPY) || info->individual || m->creature)) continue;
		if(m->coolUntil != Time::ZERO && localTime < m->coolUntil) continue;
		if(!(m->openMask & (1u << (PLAYER_FACTION_BASE + bot.team)))) continue;
		bool skipped = false;
		foreach_const(s, bot.botFarmSkips) if(s->first == m->object && localTime < s->second) skipped = true;
		if(skipped) continue;
		if(m->occupiedTeam != INTERACTION_NOT_OCCUPIED && !info->pvpOccupy) continue;
		bool busy = false;
		if(!info->individual) foreach_const(p, players) if(p->interactSlot >= 0 && &mapInteractions[p->interactSlot] == &*m && &*p != &bot) busy = true;
		if(busy) continue;
		vec3 pos;
		if(!MapInteractionPos(*m, &pos)) continue;
		const f32 d = glm::length(vec2(pos) - here);
		if(d >= best) continue;
		best = d;
		*point = vec2(pos);
		*object = m->object;
		*reach = info->activeDistance * HYP_BOT_OCCUPY_REACH;
		found = true;
	}
	return found;
}

void World::UpdateOccupy()
{
	if(!occupyMode || winnerTeam >= 0) return;
	foreach(m, mapInteractions) {
		if(m->occupier < 0 || m->occupiedTeam == INTERACTION_NOT_OCCUPIED || localTime < m->nextOccupyTick) continue;
		Player& owner = players[m->occupier];
		m->nextOccupyTick = TimeAdd(m->nextOccupyTick, TimeMsToTime(MEASURED_OCCUPY_TICK_MS));
		if(owner.removed || (i32)PLAYER_FACTION_BASE + (i32)owner.team != m->occupiedTeam) continue;
		replication->SendPvpAnnouncementToAll(MEASURED_ANNOUNCE_OCCUPY_TICK, 0, owner.Main().UID, m->object, MEASURED_OCCUPY_TICK_POINTS);
		OccupyScore(owner, MEASURED_OCCUPY_TICK_POINTS, false);
	}
}

void World::UpdateMapInteractions()
{
	UpdateOccupy();
	if(!levelEventsStarted) return;
	BuildMapInteractions();
	static const f32 INTERACTION_MOVE_CANCEL_DIST = 60.0f;
	foreach(p, players) {
		if(p->interactSlot < 0) continue;
		if(p->dead || glm::length(vec2(p->body->GetWorldPos()) - p->interactFrom) > INTERACTION_MOVE_CANCEL_DIST) CancelMapInteraction(*p);
		else if(localTime >= p->interactEndAt) CompleteMapInteraction(*p);
	}
	for(i32 i = 0; i < (i32)mapInteractions.size(); i++) {
		MapInteraction& m = mapInteractions[i];
		if(!m.creature && m.statusPending && actorDynamicMap.find(m.object) != actorDynamicMap.end()) {
			bool known = true;
			foreach_const(p, players) if(p->clientHd != ClientHandle::INVALID && !p->removed && replication->GetLocalActorID(p->clientHd, m.object) == LocalActorID::INVALID) known = false;
			if(known) {
				m.statusPending = false;
				SendMapInteractionStatus(m);
			}
		}
		if(!m.creature || m.object == ActorUID::INVALID) continue;
		const ActorNpc* npc = FindNpcActor(m.object);
		if(npc && (npc->despawnAt == Time::ZERO || localTime < npc->despawnAt)) {
			if(m.statusPending) { m.statusPending = false; SendMapInteractionStatus(m); }
			continue;
		}
		foreach(p, players) if(p->interactSlot == i) CancelMapInteraction(*p);
		m.object = ActorUID::INVALID;
		m.active = false;
	}
	foreach(m, mapInteractions) {
		if(m->coolUntil == Time::ZERO || localTime < m->coolUntil) continue;
		m->coolUntil = Time::ZERO;
		const GameXmlContent::InteractionInfo* info = GetGameXmlContent().FindInteraction(m->interactionID);
		if(!info || !info->pvpOccupy) m->occupiedTeam = INTERACTION_NOT_OCCUPIED;
		SendMapInteractionStatus(*m);
	}
}

static const f32 HYP_AREA_MIN_HALF_EXTENT = 100.0f;
static const f32 MEASURED_AREA_SIZE_TO_HALF_EXTENT = 1.0f;

static bool AreaContains(const GameXmlContent::Map::Area& area, const vec3& pos)
{
	const vec2 d = vec2(pos) - vec2(area.pos);
	const f32 c = cosf(area.rot.z);
	const f32 s = sinf(area.rot.z);
	return fabsf(d.x * c - d.y * s) <= MAX(area.size.z * MEASURED_AREA_SIZE_TO_HALF_EXTENT, HYP_AREA_MIN_HALF_EXTENT) && fabsf(d.x * s + d.y * c) <= MAX(area.size.y * MEASURED_AREA_SIZE_TO_HALF_EXTENT, HYP_AREA_MIN_HALF_EXTENT);
}

static const u32 CREATURE_TRIGGER_STATE = 0xffffffff;

void World::UpdateCreatureEnterTrigger(const GameXmlContent::LevelEventNode& node, i32 slot, const GameXmlContent::Map::Area& area)
{
	if(node.maxTriggers > 0 && boundTriggerFires[slot] >= node.maxTriggers) return;
	bool inside = false;
	foreach_const(npc, actorNpcList) {
		if(npc->tagID != node.tagID || npc->hp <= 0 || npc->removeAt != Time::ZERO || !AreaContains(area, npc->pos)) continue;
		inside = true;
		break;
	}
	BoundTriggerState* state = nullptr;
	foreach(s, boundTriggerStates) if(s->node == slot && s->playerIndex == CREATURE_TRIGGER_STATE) state = &*s;
	if(!state) {
		boundTriggerStates.push_back({ slot, CREATURE_TRIGGER_STATE, false, Time::ZERO });
		state = &boundTriggerStates.back();
	}
	if(state->inside == inside) return;
	state->inside = inside;
	if(!inside) return;
	if(state->last != Time::ZERO && TimeDiffSec(TimeDiff(state->last, localTime)) < node.retrigger) return;
	state->last = localTime;
	if(node.maxTriggers > 0) boundTriggerFires[slot]++;
	LOG("LevelEvent :: creature tag %d enters area %d (node %d)", node.tagID, node.areaID, node.id);
	foreach_const(o, node.outputs) FireLevelEvent(*o, -1, node.id);
}

void World::UpdateBoundTriggers()
{
	if(!levelEventsStarted || Config().DevShowcase) return;
	const GameXmlContent::Map& levelMap = *map;
	foreach(p, players) {
		if(p->campTeleportAt == Time::ZERO || localTime < p->campTeleportAt) continue;
		p->campTeleportAt = Time::ZERO;
		const GameXmlContent::Map::Area* start = levelMap.FindArea(p->campStartArea);
		const GameXmlContent::Map::Area* dest = levelMap.FindArea(p->campDestArea);
		if(!start || !dest || p->dead || !p->body || !AreaContains(*start, p->body->GetWorldPos())) continue;
		WarpPlayer(*p, dest->pos + vec3(0, 0, LEVEL_TELEPORT_LIFT), dest->rot.z);
		LOG("LevelEvent :: %ls moved out of starting area %d to area %d", p->name.data(), p->campStartArea, p->campDestArea);
	}
	foreach_const(n, levelMap.levelEvents) {
		if((n->kind != GameXmlContent::LevelEventNode::BOUND_TRIGGER && n->kind != GameXmlContent::LevelEventNode::CREATURE_ENTER) || !LevelTriggerActive(*n)) continue;
		const GameXmlContent::Map::Area* area = levelMap.FindArea(n->areaID);
		if(!area) continue;
		if(n->kind == GameXmlContent::LevelEventNode::CREATURE_ENTER) {
			UpdateCreatureEnterTrigger(*n, (i32)(n - levelMap.levelEvents.begin()), *area);
			continue;
		}
		const i32 ownerTeam = n->ownTeamOnly ? AreaOwnerTeam(*area) : -1;
		const i32 slot = (i32)(n - levelMap.levelEvents.begin());
		foreach(p, players) {
			if(n->maxTriggers > 0 && boundTriggerFires[slot] >= n->maxTriggers) break;
			if(p->dead || !p->body) continue;
			if(ownerTeam >= 0 && (i32)p->team != ownerTeam) continue;
			const vec3 pos = p->body->GetWorldPos();
			const bool inside = AreaContains(*area, pos);
			BoundTriggerState* state = nullptr;
			foreach(s, boundTriggerStates) if(s->node == slot && s->playerIndex == p->index) state = &*s;
			if(!state) {
				boundTriggerStates.push_back({ slot, p->index, n->ownTeamOnly ? !inside : inside, Time::ZERO });
				if(!n->ownTeamOnly) continue;
				state = &boundTriggerStates.back();
			}
			if(state->inside == inside) continue;
			state->inside = inside;
			if(inside != n->enter) continue;
			if(state->last != Time::ZERO && TimeDiffSec(TimeDiff(state->last, localTime)) < n->retrigger) continue;
			state->last = localTime;
			LOG("LevelEvent :: %ls %s area %d (node %d)", p->name.data(), inside ? "enters" : "leaves", n->areaID, n->id);
			if(n->maxTriggers > 0) boundTriggerFires[slot]++;
			foreach_const(o, n->outputs) FireLevelEvent(*o, (i32)p->index, n->id);
		}
	}
}

void World::UpdateLevelEvents()
{
	for(i32 i = 0; i < (i32)levelEventTimers.size(); ) {
		if(localTime < levelEventTimers[i].at) { i++; continue; }
		const i32 node = levelEventTimers[i].node;
		const i32 playerIndex = levelEventTimers[i].playerIndex;
		const i32 fromNode = levelEventTimers[i].fromNode;
		const bool clientSeq = levelEventTimers[i].clientSeq;
		levelEventTimers.erase(levelEventTimers.begin() + i);
		levelClientSeq = clientSeq;
		FireLevelEvent(node, playerIndex, fromNode);
		levelClientSeq = false;
	}
	for(i32 i = 0; i < (i32)dialogEnds.size(); ) {
		if(localTime < dialogEnds[i].at) { i++; continue; }
		const i32 playerIndex = dialogEnds[i].playerIndex;
		dialogEnds.erase(dialogEnds.begin() + i);
		FireAITrigger(DIALOG_END_AI_TRIGGER, playerIndex);
	}
	for(i32 i = 0; i < (i32)monsterCalls.size(); ) {
		MonsterCall& call = monsterCalls[i];
		if(localTime < call.at) { i++; continue; }
		const GameXmlContent::LevelEventNode* node = map->FindLevelEvent(call.node);
		if(node) CallMonsters(*node, call.playerIndex);
		if(!node || --call.remaining <= 0) {
			monsterCalls.erase(monsterCalls.begin() + i);
			continue;
		}
		call.at = TimeAdd(call.at, TimeMsToTime(MAX(node->callInterval, 0.0f) * 1000.0));
		i++;
	}
	UpdateBoundTriggers();
	UpdateMapInteractions();
	for(i32 i = 0; i < (i32)groundItems.size(); ) {
		if(groundItems[i].expireAt != Time::ZERO && localTime >= groundItems[i].expireAt) {
			replication->SendDestroyGroundItemToAll(groundItems[i].uid);
			groundItems.erase(groundItems.begin() + i);
			continue;
		}
		i++;
	}
	foreach(p, players) {
		if(p->clientHd != ClientHandle::INVALID || p->dead || !p->body) continue;
		const vec2 at = vec2(p->body->GetWorldPos());
		for(i32 i = 0; i < (i32)groundItems.size(); i++) {
			if(glm::length(vec2(groundItems[i].pos) - at) > BOT_ITEM_PICKUP_RADIUS) continue;
			if(TakeGroundItem(*p, groundItems[i].uid)) break;
		}
	}
}
