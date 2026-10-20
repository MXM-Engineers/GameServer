#include "config.h"
#include "world.h"
#include "world_common.h"
#include <mxm/game_content.h>

static const f32 TELEPORT_SPREAD = 80.0f;
static const i32 INTERACTION_NOT_OCCUPIED = -1;
static const u32 PLAYER_FACTION_BASE = 3;
static const f32 CARRY_SYNC_SEC = 0.1f;
static const f32 TOOLTIP_INTERACTION_OPEN_SEC = 3.0f;
static const i32 INTERACTION_OBJECT_LOCAL_BASE = 41000;
static const f64 INTERACTION_ANNOUNCE_DELAY_MS = 200.0;
static const i32 GROUP_TELEPORT_SLOTS = 5;

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
		if(!owner.dead) OpenInteractionObject(owner, it->interactionID);
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
}
