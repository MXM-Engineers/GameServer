#include "config.h"
#include "world.h"
#include "world_common.h"
#include <mxm/game_content.h>
#include <EASTL/sort.h>

static const u16 SCENE_ROOT_NODE = 0;
static const wchar* CHAIN_CHILD_NODE = L"";
static const f64 CLIENT_SEED_BIND_MS = 1000.0;
static const f32 WEAPON_NODE_MAX_FORWARD = 250.0f;
static const i32 ULTIMATE_SKILL_SUFFIX = 50;
static const f32 REPEAT_HIT_REDUCTION = 0.1f;
static const f32 REPEAT_HIT_REDUCTION_CAP = 0.6f;
static const i32 BEZIER_ARC_SAMPLES = 24;
static const f32 REMOTE_DESTROY_CHILD_DELAY = 0.05f;
static const f32 MEASURED_CHAIN_HOP_SEC = 0.126f;
static const f32 REMOTE_MAX_AGE = 60.0f;

static bool RemoteCreateSendsEntity(ActionCommand::TargetPreset preset, Remote::BehaviorType behavior)
{
	switch(behavior) {
		case Remote::BehaviorType::FOLLOW_TARGET:
		case Remote::BehaviorType::ATTACH_TARGET:
		case Remote::BehaviorType::TARGET:
		case Remote::BehaviorType::CHAIN:
		case Remote::BehaviorType::HOMING:
			return true;
		default: break;
	}
	switch(preset) {
		case ActionCommand::TargetPreset::SELF_FFF:
		case ActionCommand::TargetPreset::SELF_FFF_1:
		case ActionCommand::TargetPreset::SELF_TFF:
		case ActionCommand::TargetPreset::SELF_TTF:
		case ActionCommand::TargetPreset::POS_FFF:
			return false;
		default: return true;
	}
}
struct WorldRemoteSpawner
{
	static World::RemoteInst* Spawn(World& w, u32 ownerIndex, RemoteIdx idx, const vec3& pos, const vec2& dir, ActorUID target, SkillID skillID, u32 castID, i32 damage, bool broadcast, bool casterLocal, u16 nodeName)
	{
		return w.SpawnRemote(ownerIndex, idx, pos, dir, target, skillID, castID, damage, broadcast, nullptr, nodeName, casterLocal, false, true);
	}
};

bool World::RemoteCanAffect(const RemoteInst& inst, const Player& p, bool includeOwner) const
{
	if(p.dead || p.untargetable) return false;
	if(inst.remote->monsterOnly) return false;
	if(inst.excludeIndex >= 0 && (i32)p.index == inst.excludeIndex) return false;
	const Player& owner = players[inst.ownerIndex];
	const Remote& remote = *inst.remote;
	const auto group = remote.damageGroup;
	const bool friendContact = group == Remote::DamageGroup::eFRIEND || group == Remote::DamageGroup::eALL || remote.contactFriend || (remote.includeParty && !remote.friendlyStatuses.empty());
	if(p.index == owner.index) {
		if(!includeOwner && !remote.vsOwner) return false;
		if(!remote.vsOwner && (remote.attachFireObject || remote.activateCount == 1)) return false;
		return remote.vsOwner || group == Remote::DamageGroup::eFRIEND || group == Remote::DamageGroup::eALL;
	}
	const bool enemy = p.team != owner.team;
	if(enemy) return group == Remote::DamageGroup::eENEMY || group == Remote::DamageGroup::eALL;
	return friendContact;
}

static f32 BezierArcLength(const Remote& remote, f32 range)
{
	vec3 c[4];
	c[0] = vec3(0);
	c[3] = vec3(range, 0, 0);
	for(int k = 0; k < 2; k++) {
		const f32 a = remote.bezierRotate[k] * PI / 180.0f;
		c[k + 1] = vec3(range * (k + 1) / 3.0f + remote.bezierFront[k], remote.bezierUp[k] * sinf(a), remote.bezierUp[k] * cosf(a));
	}
	f32 length = 0;
	vec3 prev = c[0];
	for(int i = 1; i <= BEZIER_ARC_SAMPLES; i++) {
		const f32 u = (f32)i / BEZIER_ARC_SAMPLES;
		const f32 v = 1.0f - u;
		const vec3 p = c[0] * (v * v * v) + c[1] * (3 * v * v * u) + c[2] * (3 * v * u * u) + c[3] * (u * u * u);
		length += glm::length(p - prev);
		prev = p;
	}
	return length;
}

static f32 WeaponNodeForward(const GameXmlContent& content, ClassType cls, SkillID skillID, const Remote& remote, u16 nodeName)
{
	if(nodeName == 0 || remote.maxSpeed > 0 || remote.attachFireObject || remote.boundType != Remote::BoundType::E_BOUND_BOX) return 0;
	auto f = content.skillMap.find(skillID);
	if(f == content.skillMap.end()) return 0;
	const SkillNormalModel& skill = f->second;
	const f32 indicator = skill.getSkillNormalLevelByIndex(0)->getSkillRangeLengthY();
	if(indicator <= 0) return 0;
	const GameXmlContent::Action* action = content.FindSkillAction(cls, skill.action);
	if(!action) return 0;
	foreach_const(cmd, action->commands) {
		if(cmd->type == ActionCommand::Type::GRAPH_MOVE_HORZ && cmd->graphMoveHorz.distance > 0) return 0;
		if(cmd->type == ActionCommand::Type::MOVE && cmd->move.preset != ActionCommand::MovePreset::MOVE_NONE) return 0;
	}
	const f32 forward = indicator - (remote.offsetY + RemoteBoundOffset(remote) + remote.boundSize[1] * 0.5f);
	return forward > 0 && forward <= WEAPON_NODE_MAX_FORWARD ? forward : 0;
}

World::RemoteInst* World::SpawnRemote(u32 ownerIndex, RemoteIdx idx, const vec3& pos, const vec2& dir, ActorUID target, SkillID skillID, u32 castID, i32 damage, bool broadcast, const vec3* aimPos, u16 nodeName, bool casterCreatesLocally, bool stopAtTarget, bool child, bool sendEntity)
{
	const RemoteIdx sendIdx = remoteSendIdxOverride != RemoteIdx::INVALID ? remoteSendIdxOverride : idx;
	remoteSendIdxOverride = RemoteIdx::INVALID;
	const Remote* remote = GetGameXmlContent().FindRemote(idx);
	if(!remote || remoteInsts.full()) return nullptr;
	if(const GameXmlContent::CostLevel* lv = spawnCostLevel ? spawnCostLevel : castCostLevel) {
		foreach_const(rs, lv->remoteResize) if(rs->first == (i32)idx) remote = rs->second;
	}

	RemoteInst inst;
	inst.instID = nextRemoteInstID++;
	inst.remote = remote;
	inst.ownerIndex = ownerIndex;
	inst.nodeName = nodeName;
	inst.casterMasterUID = players[ownerIndex].Main().UID;
	inst.casterClass = players[ownerIndex].Main().classType;
	inst.skillID = skillID;
	inst.castID = castID;
	inst.damage = damage;
	inst.pos = pos;
	if(child && remote->groundMoveOffsetZ >= 0) inst.pos.z = players[ownerIndex].body->GetWorldPos().z + remote->groundMoveOffsetZ;
	inst.dir = dir;
	inst.travelled = 0;
	inst.wallZ = players[ownerIndex].body->GetWorldPos().z + BodyMidHeight(players[ownerIndex]);
	inst.speed = remote->maxSpeed;
	inst.range = remote->maxDistance;
	if(remote->behaviorType == Remote::BehaviorType::GRAPH && remote->fixedGraphLength > 0) inst.range = remote->fixedGraphLength;
	if(inst.range <= 0 && remote->lifeTime > 0 && inst.speed > 0) inst.range = remote->lifeTime * inst.speed;
	if(remote->initSpeed > 0 && remote->angularVel > 0) inst.speed = remote->maxSpeed > 0 ? MIN(remote->initSpeed, remote->maxSpeed) : remote->initSpeed;
	if(remote->useOwnerRemoteSpeedRatio) inst.speed *= OwnerRemoteSpeedRatio(players[ownerIndex]);
	if(remote->behaviorType == Remote::BehaviorType::GRAPH && remote->fixedGraphLength <= 0 && aimPos && glm::length(vec2(*aimPos)) > 1.0f) {
		const f32 aimDist = glm::length(vec2(*aimPos) - vec2(pos));
		inst.range = inst.range > 0 ? MIN(aimDist, inst.range) : aimDist;
		inst.landZ = aimPos->z;
		inst.hasLandZ = true;
	}
	inst.stopAtFirstHit = false;
	if(stopAtTarget && remote->behaviorType != Remote::BehaviorType::GRAPH && aimPos && glm::length(vec2(*aimPos)) > 1.0f && inst.speed > 0) {
		const f32 aimDist = glm::length(vec2(*aimPos) - vec2(pos));
		inst.range = inst.range > 0 ? MIN(aimDist, inst.range) : aimDist;
	}
	inst.spawnTime = localTime;
	inst.firePath = GetGameXmlContent().FindFireNodePath(remote->ID);
	inst.firePathYaw = atan2f(dir.y, dir.x);
	inst.firePathDone = 0;
	if(target != ActorUID::INVALID && remote->behaviorType == Remote::BehaviorType::HOMING && remote->homingAngle > 0 && remote->homingAngle < 360 && glm::length(dir) > 0.5f) {
		const ActorMaster* tm = FindMasterActor(target);
		if(tm && tm->parent && tm->parent->body) {
			const vec2 rel = vec2(tm->parent->body->GetWorldPos() - pos);
			const f32 len = glm::length(rel);
			if(len > 1.0f && glm::dot(rel / len, glm::normalize(dir)) < cosf(remote->homingAngle * 0.5f * PI / 180.0f)) target = ActorUID::INVALID;
		}
	}
	inst.target = target;
	inst.hitsLeft = MAX(remote->activateCount, 1);
	inst.penetrationLeft = remote->penetrationCount > 0 ? remote->penetrationCount : PENETRATION_UNLIMITED;
	inst.areaDone = false;
	inst.casterLocal = casterCreatesLocally;
	if(casterCreatesLocally && players[ownerIndex].clientHd != ClientHandle::INVALID) {
		for(i32 k = 0; k < (i32)clientRemoteSeeds.size(); k++) {
			const ClientRemoteSeed& cs = clientRemoteSeeds[k];
			if(cs.playerIndex != ownerIndex || cs.doc != idx || localTime > TimeAdd(cs.at, TimeMsToTime(CLIENT_SEED_BIND_MS))) continue;
			inst.clientRemoteID = cs.seed + 1;
			inst.seedOwnerHd = players[ownerIndex].clientHd;
			clientRemoteSeeds.erase(clientRemoteSeeds.begin() + k);
			break;
		}
	}
	inst.graphStart = inst.pos;
	inst.graphDir = dir;
	inst.graphArc = remote->behaviorType == Remote::BehaviorType::GRAPH && remote->bezier && inst.range > 0 ? BezierArcLength(*remote, inst.range) : 0;
	if(remote->timeEventDelay >= 0) {
		inst.timeEventActive = true;
		inst.nextTimeEvent = TimeAdd(localTime, TimeMsToTime(remote->timeEventDelay * 1000.0));
	}

	if(!broadcast && child && !casterCreatesLocally) {
		inst.damageRemoteID = nextRemoteSeed + 1;
		nextRemoteSeed += 2;
	}
	if(broadcast) {
		const u32 seed = nextRemoteSeed;
		nextRemoteSeed += 2;
		inst.clientRemoteID = seed + 1;
		inst.sentByDoc = true;
		vec3 targetPos = pos + vec3(dir * (inst.range > 0 ? inst.range : 1000.0f), 0);
		const ActorMaster* targetMaster = target != ActorUID::INVALID ? FindMasterActor(target) : nullptr;
		if(targetMaster) targetPos = targetMaster->parent->body->GetWorldPos();
		if(child && (remote->behaviorType == Remote::BehaviorType::NONETARGET || remote->behaviorType == Remote::BehaviorType::HOMING)) {
			targetMaster = nullptr;
			targetPos = pos;
		}
		if(!sendEntity) {
			targetMaster = nullptr;
			if(remote->behaviorType != Remote::BehaviorType::GRAPH && aimPos && glm::length(vec2(*aimPos)) > 1.0f) targetPos = *aimPos;
		}
		if(remote->behaviorType == Remote::BehaviorType::GRAPH && aimPos && glm::length(vec2(*aimPos)) > 1.0f) {
			targetPos = pos + vec3(dir * inst.range, 0);
			targetPos.z = aimPos->z;
		}
		const bool plainStaticBox = inst.speed <= 0 && (remote->boundType == Remote::BoundType::E_BOUND_BOX || remote->boundType == Remote::BoundType::E_BOUND_SPHERE) && remote->angle <= 0;
		const f32 sendYaw = atan2f(dir.y, dir.x) + (remoteSendRootDir ? PI : 0.0f);
		vec3 sendPos = vec3(vec2(pos), inst.pos.z) - vec3(0, 0, remote->offsetZ);
		const bool atAimedPoint = aimPos && glm::length(vec2(*aimPos)) > 1.0f && glm::length(vec2(pos) - vec2(*aimPos)) < 1.0f;
		if(plainStaticBox && !atAimedPoint) sendPos -= vec3(dir * (remote->forceTargetDir ? -remote->offsetY : remote->offsetY) - Perp(dir) * remote->offsetX, 0);
		if(remoteTargetEntityOverride != ActorUID::INVALID) targetPos = pos;
		const ActorUID sendOwner = remoteParentOverride != ActorUID::INVALID ? remoteParentOverride : players[ownerIndex].Main().UID;
		const ActorUID sendTarget = remoteTargetEntityOverride != ActorUID::INVALID ? remoteTargetEntityOverride : (targetMaster ? target : ActorUID::INVALID);
		const wchar* sendNode = child && remote->behaviorType == Remote::BehaviorType::CHAIN ? CHAIN_CHILD_NODE : GetGameXmlContent().GetNodeName(nodeName);
		const ClientHandle sendExclude = casterCreatesLocally ? players[ownerIndex].clientHd : ClientHandle::INVALID;
		const vec3 sendFrom = remoteSendFromSet ? remoteSendFrom : sendPos;
		if(remoteSendParentID != 0) replication->SendChildRemoteCreateToAll(sendOwner, sendOwner, remoteSendParentID, remoteSendParentSeedHd, seed, sendIdx, 0, sendFrom, sendYaw, targetPos, sendTarget, ActorUID::INVALID, sendNode, MEASURED_REMOTE_CREATE_FLAG_PARENT_REMOTE, sendExclude);
		else replication->SendRemoteCreateToAll(sendOwner, seed, sendIdx, sendFrom, sendYaw, targetPos, sendTarget, sendNode, sendExclude);
	}

	remoteInsts.push_back(inst);

	LOG("Remote :: spawn %d at (%.0f, %.0f) dir=(%.2f, %.2f) speed=%.0f range=%.0f", (i32)idx, pos.x, pos.y, dir.x, dir.y, inst.speed, inst.range);
	return &remoteInsts.back();
}

static vec2 RotateDir(const vec2& d, f32 deg)
{
	const f32 a = deg * PI / 180.0f;
	const f32 c = cosf(a), s = sinf(a);
	return vec2(d.x * c - d.y * s, d.x * s + d.y * c);
}

static RemoteIdx CostReplacedRemote(const GameXmlContent::CostLevel* level, RemoteIdx idx)
{
	if(level) foreach_const(rr, level->remoteReplace) if(rr->first == (i32)idx) return RemoteIdx(rr->second);
	return idx;
}

static i32 ClientCreatorKindRank(u8 kind)
{
	switch(kind) {
		case Remote::SPAWN_DEFAULT: return 0;
		case Remote::SPAWN_RADIATE: return 1;
		case Remote::SPAWN_CIRCLE: return 2;
		default: return 3;
	}
}

static u32 CreatorChildCount(const Remote::SpawnRules& rules, bool& replicable)
{
	u32 count = 0;
	replicable = true;
	foreach_const(rule, rules) {
		if(rule->kind == Remote::SPAWN_SINGLE) continue;
		if(rule->kind == Remote::SPAWN_CIRCLE) replicable = false;
		count += (u32)MAX(rule->maxNum, 1);
	}
	if(count == 0) replicable = false;
	return count;
}

static void SpawnRemoteRules(World& world, const World::RemoteInst& parent, const Remote::SpawnRules& rules, u16 creator, const vec3& pos, i32 excludeIndex, RemoteIdx skipFirst, ActorUID spawnerActor = ActorUID::INVALID, const vec3& aimPos = vec3(0))
{
	bool replicable = false;
	const u32 childCount = CreatorChildCount(rules, replicable);
	const bool collapse = creator != 0 && replicable && (parent.clientRemoteID != 0 || spawnerActor != ActorUID::INVALID);
	bool hitInvalidParent = false;
	foreach_const(rule, rules) if(rule->kind != Remote::SPAWN_SINGLE && rule->hitInvalidParentLastTarget) hitInvalidParent = true;
	u32 childID = collapse ? world.SendRemoteCreator(parent, creator, pos, childCount, spawnerActor, aimPos, hitInvalidParent) : 0;
	eastl::fixed_vector<const Remote::SpawnRule*,4,false> ordered;
	foreach_const(rule, rules) ordered.push_back(&*rule);
	eastl::stable_sort(ordered.begin(), ordered.end(), [](const Remote::SpawnRule* a, const Remote::SpawnRule* b) { return ClientCreatorKindRank(a->kind) < ClientCreatorKindRank(b->kind); });
	bool skipped = false;
	foreach_const(it, ordered) {
		const Remote::SpawnRule* rule = *it;
		if(spawnerActor != ActorUID::INVALID && rule->kind == Remote::SPAWN_SINGLE) continue;
		if(!skipped && rule->creature <= 0 && rule->kind == Remote::SPAWN_SINGLE && rule->idx == skipFirst) {
			skipped = true;
			continue;
		}
		if(rule->creature > 0) {
			world.SpawnCreatureFromRemote(parent, rule->creature, pos);
			continue;
		}
		const bool fromCreator = collapse && rule->kind != Remote::SPAWN_SINGLE;
		const Remote* child = GetGameXmlContent().FindRemote(rule->idx);
		if(!child) {
			if(fromCreator) childID += (u32)MAX(rule->maxNum, 1);
			continue;
		}
		for(i32 n = 0; n < rule->maxNum; n++) {
			vec2 dir = parent.dir;
			vec3 at = pos;
			switch(rule->kind) {
				case Remote::SPAWN_SINGLE: break;
				case Remote::SPAWN_RADIATE: {
					dir = RotateDir(parent.dir, ((f32)n - (f32)(rule->maxNum - 1) * 0.5f) * rule->maxAngle / (f32)MAX(rule->maxNum, 1));
					at = pos + vec3(dir * rule->offsetForward, rule->offsetHeight);
				} break;
				case Remote::SPAWN_CIRCLE: {
					const f32 a = (RandUint() % 3600) * 0.1f * PI / 180.0f;
					const f32 r = rule->radius * sqrtf((RandUint() % 1000) / 1000.0f);
					at = pos + vec3(cosf(a) * r, sinf(a) * r, 0);
				} break;
				case Remote::SPAWN_DEFAULT: {
					dir = RotateDir(parent.dir, rule->angleY);
				} break;
			}
			ActorUID childTarget = parent.target;
			if(rule->targetOwner) {
				World::Player& owner = world.GetPlayer(parent.ownerIndex);
				childTarget = owner.Main().UID;
				const vec2 rel = vec2(owner.body->GetWorldPos()) - vec2(at);
				const bool turnsItself = child->behaviorType == Remote::BehaviorType::HOMING && child->angularVel > 0;
				if(glm::length(rel) > 1.0f && !turnsItself) dir = glm::normalize(rel);
			}
			if(parent.sourceUID != ActorUID::INVALID && world.FindNpcActor(parent.sourceUID)) world.remoteParentOverride = parent.sourceUID;
			const bool broadcast = !fromCreator && parent.clientRemoteID != 0;
			if(broadcast) {
				world.remoteSendParentID = parent.clientRemoteID - (parent.sentByDoc ? 1 : 0);
				world.remoteSendParentSeedHd = parent.seedOwnerHd;
			}
			World::RemoteInst* inst = WorldRemoteSpawner::Spawn(world, parent.ownerIndex, CostReplacedRemote(parent.costLevel, rule->idx), at + vec3(dir * child->offsetY, 0), dir, childTarget, parent.skillID, parent.castID, parent.damage, broadcast, false, parent.nodeName);
			world.remoteParentOverride = ActorUID::INVALID;
			world.remoteSendParentID = 0;
			world.remoteSendParentSeedHd = ClientHandle::INVALID;
			if(inst) {
				inst->excludeIndex = excludeIndex;
				inst->chainHitMask = parent.chainHitMask;
				inst->chainNpcHit = parent.chainNpcHit;
				inst->casterClass = parent.casterClass;
				inst->costLevel = parent.costLevel;
				inst->centeredBox = parent.centeredBox;
				inst->sourceUID = parent.sourceUID;
				if(fromCreator) {
					inst->clientRemoteID = childID;
					inst->damageRemoteID = 0;
					inst->seedOwnerHd = ClientHandle::INVALID;
				}
			}
			if(fromCreator) childID++;
		}
	}
}

u32 World::SendRemoteCreator(const RemoteInst& parent, u16 creator, const vec3& pos, u32 childCount, ActorUID spawnerActor, const vec3& aimPos, bool hitInvalidParent)
{
	const u32 seed = nextRemoteSeed;
	nextRemoteSeed += (childCount + 2) & ~1u;
	const bool fromRemote = spawnerActor == ActorUID::INVALID;
	const ActorUID ownerUID = parent.sourceUID != ActorUID::INVALID && FindNpcActor(parent.sourceUID) ? parent.sourceUID : players[parent.ownerIndex].Main().UID;
	const ActorMaster* targetMaster = parent.target != ActorUID::INVALID ? FindMasterActor(parent.target) : nullptr;
	const ActorUID hitInvalid = fromRemote && hitInvalidParent && parent.lastHitIndex >= 0 ? players[parent.lastHitIndex].Main().UID : ActorUID::INVALID;
	replication->SendChildRemoteCreateToAll(ownerUID, fromRemote ? ownerUID : spawnerActor, fromRemote ? parent.clientRemoteID - (parent.sentByDoc ? 1 : 0) : 0, fromRemote ? parent.seedOwnerHd : ClientHandle::INVALID, seed, RemoteIdx::INVALID, creator, pos, atan2f(parent.dir.y, parent.dir.x), fromRemote ? pos : aimPos,
		targetMaster ? parent.target : ActorUID::INVALID, hitInvalid, GetGameXmlContent().GetNodeName(parent.nodeName), fromRemote ? MEASURED_REMOTE_CREATE_FLAG_PARENT_REMOTE : 0, ClientHandle::INVALID);
	return seed + 1;
}

void World::SpawnProjectileHitRemotes(const Projectile& proj, const Remote& shot, const vec3& at, i32 hitIndex, ActorUID hitNpc)
{
	if(shot.onHitCharacter.empty()) return;
	RemoteInst parent = RemoteInst();
	parent.remote = &shot;
	parent.ownerIndex = proj.ownerIndex;
	parent.casterMasterUID = players[proj.ownerIndex].Main().UID;
	parent.casterClass = players[proj.ownerIndex].Main().classType;
	parent.skillID = SkillID::INVALID;
	parent.castID = nextCastID++;
	parent.damage = proj.damage;
	parent.pos = at;
	parent.dir = proj.dir;
	parent.spawnTime = localTime;
	parent.target = ActorUID::INVALID;
	parent.nodeName = proj.nodeName;
	parent.clientRemoteID = proj.remoteID;
	parent.seedOwnerHd = proj.seedOwnerHd;
	parent.lastHitIndex = hitIndex;
	const u32 before = (u32)remoteInsts.size();
	SpawnRemoteRules(*this, parent, shot.onHitCharacter, shot.creatorOnHitCharacter, at, hitIndex, RemoteIdx::INVALID);
	if(hitNpc == ActorUID::INVALID) return;
	for(u32 i = before; i < (u32)remoteInsts.size(); i++) {
		RemoteInst& child = remoteInsts[i];
		if(!child.npcHits.full()) child.npcHits.push_back({ hitNpc, TimeAdd(localTime, TimeMsToTime(MAX(child.remote->hitInvalidTime, 0.1f) * 1000.0)) });
	}
}

i32 World::PenetrationCost(const Player& p) const
{
	if(p.Main().classType == ClassType::NONE) return 0;
	return GetGameXmlContent().GetMaster(p.Main().classType).densityCount;
}

void World::BeginRemoteActivation(const RemoteInst& inst)
{
	remoteActivation.remoteID = inst.clientRemoteID - (inst.sentByDoc ? 1 : 0);
	remoteActivation.skipHd = inst.casterLocal ? players[inst.ownerIndex].clientHd : ClientHandle::INVALID;
	remoteActivation.targetUID = ActorUID::INVALID;
	remoteActivation.penetration = 0;
}

void World::AddRemoteActivation(const Player& target, f32 z)
{
	if(remoteActivation.remoteID == 0) return;
	if(remoteActivation.targetUID == ActorUID::INVALID) {
		remoteActivation.targetUID = target.Main().UID;
		remoteActivation.pos = vec3(vec2(target.body->GetWorldPos()), z);
	}
	remoteActivation.penetration += (u32)PenetrationCost(target);
}

void World::FlushRemoteActivation()
{
	if(remoteActivation.remoteID != 0 && remoteActivation.targetUID != ActorUID::INVALID)
		replication->SendRemoteActivatedToAll(remoteActivation.remoteID, ClientHandle::INVALID, remoteActivation.skipHd, remoteActivation.targetUID, remoteActivation.penetration, remoteActivation.pos);
	remoteActivation.remoteID = 0;
}

bool World::RemoteHitRecord(RemoteInst& inst, Player& target, bool enemy)
{
	const Remote& remote = *inst.remote;
	const bool sharedZone = inst.speed <= 0 && remote.lifeTime > remote.hitInvalidTime && inst.castID != 0;
	foreach(h, castHits) {
		if(h->playerIndex != target.index || localTime >= h->until) continue;
		if(h->instID == inst.instID) return false;
		if(sharedZone && h->castID == inst.castID && h->remoteID == remote.ID) return false;
	}
	if(inst.speed <= 0 && enemy) inst.hitsLeft--;
	if(enemy) inst.singleHitDone = true;
	if(!castHits.full()) {
		CastHit h;
		h.instID = inst.instID;
		h.remoteID = remote.ID;
		h.castID = inst.castID;
		h.playerIndex = target.index;
		h.until = TimeAdd(localTime, TimeMsToTime(MAX(remote.hitInvalidTime, 0.1f) * 1000.0));
		castHits.push_back(h);
	}
	return true;
}

void World::RemoteHitDamage(RemoteInst& inst, Player& owner, Player& target, i32 amount, bool enemy, f32 falloff)
{
	const Remote& remote = *inst.remote;
	if(enemy && inst.skillID != SkillID::INVALID) PassiveOnSkillHit(owner, target);
	if(enemy) PassiveStackOnHit(owner, target, (i32)remote.ID, inst.skillID);
	if(enemy && inst.skillID == SkillID::INVALID) PassiveOnWeaponStatusHit(owner, target);
	i32 stackBonus = 0;
	if(enemy && (i32)inst.skillID % 100 == ULTIMATE_SKILL_SUFFIX) {
		foreach_const(st, activeStatuses) {
			if(st->targetIndex == target.index && st->casterIndex == owner.index && st->info && st->info->additionalDamageByCaster > 0 && localTime < st->until)
				stackBonus += (i32)(st->info->additionalDamageByCaster * MAX(st->overlap, 1));
		}
		if(stackBonus > 0) LOG("Passive :: %ls stack bonus +%d on %ls", owner.name.data(), stackBonus, target.name.data());
	}
	i32 dealt = amount + stackBonus;
	if(enemy) dealt = (i32)(dealt * RepeatHitMul(inst.castID, inst.ownerIndex, target.index, remote.ID, inst.skillID));
	if(enemy) dealt = (i32)(dealt * falloff);
	if(enemy && remote.hitDamageMultiplier != 0) {
		dealt = (i32)(dealt * MAX(1.0f + remote.hitDamageMultiplier * inst.enemiesHit, 1.0f + remote.maxHitDamageMultiplier));
		if(inst.enemiesHit < 255) inst.enemiesHit++;
	}
	damageAttackerOverride = inst.sourceUID;
	damageSeedOwnerOverride = inst.seedOwnerHd;
	if(enemy) DamagePlayer(owner, target, inst.skillID, dealt, nullptr, inst.clientRemoteID != 0 ? inst.clientRemoteID - (inst.sentByDoc ? 1 : 0) : inst.damageRemoteID, remote.ID, inst.dir);
	else if(remote.damageGroup == Remote::DamageGroup::eFRIEND) HealPlayer(owner, target, amount);
	damageAttackerOverride = ActorUID::INVALID;
	damageSeedOwnerOverride = ClientHandle::INVALID;
}

void World::RemoteHitStatuses(RemoteInst& inst, Player& owner, Player& target, bool enemy, f32 falloff)
{
	const Remote& remote = *inst.remote;
	if(target.dead) return;
	const bool friendGroup = remote.damageGroup == Remote::DamageGroup::eFRIEND;
	if(enemy || friendGroup) {
		foreach_const(st, remote.statuses) {
			i32 statusID = st->statusID;
			f32 durationOverride = 0;
			f32 distanceOverride = 0;
			if(inst.costLevel) {
				foreach_const(sr, inst.costLevel->statusReplace) {
					if(sr->statusID != st->statusID) continue;
					statusID = sr->replaceID;
					durationOverride = sr->durationSec;
					distanceOverride = sr->distanceRatio;
				}
			}
			if(st->rate >= 100 || (i32)(RandUint() % 100) < st->rate) {
				statusCasterOverride = inst.sourceUID;
				statusDistanceRatioOverride = distanceOverride;
				statusDurationScale = enemy ? falloff : 1.0f;
				ApplyStatus(owner, target, statusID, (f32)inst.damage, durationOverride);
				statusDurationScale = 1.0f;
				statusDistanceRatioOverride = 0;
				statusCasterOverride = ActorUID::INVALID;
			}
		}
	}
	if(!enemy) {
		foreach_const(st, remote.friendlyStatuses) {
			if(st->rate >= 100 || (i32)(RandUint() % 100) < st->rate) { statusCasterOverride = inst.sourceUID; ApplyStatus(owner, target, st->statusID, (f32)inst.damage); statusCasterOverride = ActorUID::INVALID; }
		}
		if(!remote.friendlyStatuses.empty() && &target != &owner) PassiveOnFriendlyHit(owner);
	}
}

bool World::RemoteHit(RemoteInst& inst, Player& target)
{
	const Remote& remote = *inst.remote;
	Player& owner = players[inst.ownerIndex];
	if(!RemoteCanAffect(inst, target, true)) return false;
	if(remote.targetOnly && inst.target != ActorUID::INVALID) {
		const ActorMaster* designated = FindMasterActor(inst.target);
		if(designated ? designated->parent != &target : FindNpcActor(inst.target) != nullptr) return false;
	}
	if(remote.activateDelay > 0 && TimeDiffSec(TimeDiff(inst.spawnTime, localTime)) < remote.activateDelay) return false;
	const bool enemy = target.team != owner.team;
	if(enemy && remote.activateCount == 1 && inst.singleHitDone) return false;
	if(enemy) ApplyRemoteHitResourceGain(owner, remote.ID, inst.castID, target.index);

	const i32 amount = (i32)(inst.damage * remote.attackMultiplier);
	if(enemy && remote.memorizeTarget) owner.memorizedTarget = target.Main().UID;
	if(enemy) ChangeCasterActionOnHit(inst, owner, target.Main().UID, target.body->GetWorldPos());
	if(amount <= 0 && remote.statuses.empty() && remote.friendlyStatuses.empty() && remote.onActivate.empty() && remote.onHitCharacter.empty() && remote.onHitOwnerStatus == 0) {
		AddRemoteActivation(target, inst.pos.z);
		if(enemy) TriggerSkillHitEvent(inst, owner, target.Main().UID, target.body->GetWorldPos());
		return true;
	}

	if(!RemoteHitRecord(inst, target, enemy)) return false;

	AddRemoteActivation(target, inst.pos.z);
	statusFromBehind = remote.offsetY < 0;
	statusForceCenter = inst.pos;
	statusForceCenterSet = true;
	statusPushDir = inst.dir;
	statusPushDirSet = glm::length(inst.dir) > 0.5f;
	const f32 falloff = RemoteDistanceFalloff(remote, glm::length(vec2(target.body->GetWorldPos()) - vec2(inst.pos)));

	if(amount > 0) RemoteHitDamage(inst, owner, target, amount, enemy, falloff);

	inst.lastHitIndex = (i32)target.index;
	if(enemy && HasStatusType(target, StatusType::EVADE)) return true;
	const bool npcOwnerStatus = enemy && remote.onHitOwnerStatus != 0 && inst.sourceUID != ActorUID::INVALID && FindNpcActor(inst.sourceUID);
	if(remote.onHitOwnerStatus != 0 && !owner.dead && !npcOwnerStatus) {
		const ActorUID prevCaster = statusCasterOverride;
		statusCasterOverride = target.Main().UID;
		ApplyStatus(owner, owner, remote.onHitOwnerStatus);
		statusCasterOverride = prevCaster;
	}
	RemoteHitStatuses(inst, owner, target, enemy, falloff);
	if(npcOwnerStatus) ApplyNpcOwnerStatus(inst.sourceUID, target, remote.onHitOwnerStatus);

	if(!remote.onActivate.empty() || !remote.onHitCharacter.empty()) {
		const RemoteInst copy = inst;
		const vec3 at = vec3(vec2(target.body->GetWorldPos()), inst.pos.z);
		SpawnRemoteRules(*this, copy, remote.onActivate, remote.creatorOnActivate, at, -1, RemoteIdx::INVALID);
		SpawnRemoteRules(*this, copy, remote.onHitCharacter, remote.creatorOnHitCharacter, at, (i32)target.index, RemoteIdx::INVALID);
	}

	if(enemy) TriggerSkillHitEvent(inst, owner, target.Main().UID, target.body->GetWorldPos());
	statusFromBehind = false;
	statusForceCenterSet = false;
	statusPushDirSet = false;
	return true;
}

void World::TriggerSkillHitEvent(const RemoteInst& inst, Player& owner, ActorUID targetUID, const vec3& targetPos)
{
	const auto& content = GetGameXmlContent();
	const i32 hitEventRemote = content.FindSkillHitEventRemote((i32)inst.skillID);
	if(hitEventRemote == 0 || hitEventRemote != (i32)inst.remote->ID || owner.Main().UID != inst.casterMasterUID) return;
	if(eastl::find(comboTriggeredCasts.begin(), comboTriggeredCasts.end(), inst.castID) != comboTriggeredCasts.end()) return;
	if(comboTriggeredCasts.full()) comboTriggeredCasts.erase(comboTriggeredCasts.begin());
	comboTriggeredCasts.push_back(inst.castID);
	owner.memorizedTarget = targetUID;
	const vec2 rel = vec2(targetPos - owner.body->GetWorldPos());
	LOG("ComboHit :: %ls skill=%d remote=%d -> next part", owner.name.data(), (i32)inst.skillID, (i32)inst.remote->ID);
	castCostLevel = inst.costLevel;
	schedulePassOffset = ComboPassOffset(owner, inst.skillID);
	ScheduleSkillRemotes(owner, inst.skillID, targetPos, Slice<const ActorUID>(&targetUID, 1), atan2f(rel.y, rel.x), 1, ActionStateID::INVALID, inst.casterClass);
	schedulePassOffset = 0;
	castCostLevel = nullptr;
	if(owner.clientHd != ClientHandle::INVALID) {
		auto sk = content.skillMap.find(inst.skillID);
		if(sk != content.skillMap.end()) UpdateComboSlot(owner, inst.skillID, sk->second.type, targetPos, true);
	}
	i32 combo = ComboSkillFor(owner, (i32)inst.skillID);
	if(combo == 0) combo = content.GetNextComboSkill((i32)inst.skillID);
	const i32 comboMs = combo != 0 ? content.GetHitTargetComboMs(combo) : 0;
	if(comboMs > 0) {
		owner.hitCombo = SkillID(combo);
		owner.hitComboTarget = targetUID;
		owner.hitComboAt = TimeAdd(localTime, TimeMsToTime((f64)comboMs));
	}
	else if(owner.clientHd == ClientHandle::INVALID && combo != 0 && combo != (i32)inst.skillID) {
		const i32 windowMs = content.GetComboDurationMs(combo);
		const i32 autoMs = windowMs > 0 ? content.GetComboBotAutoCastMs(combo) : 0;
		if(autoMs > 0) {
			owner.comboSkill = SkillID(combo);
			owner.comboCastPos = targetPos;
			owner.comboAutoAt = TimeAdd(localTime, TimeMsToTime((f64)autoMs));
		}
		else if(windowMs > 0) {
			owner.botComboSkill = SkillID(combo);
			owner.botComboUntil = TimeAdd(localTime, TimeMsToTime((f64)windowMs));
			owner.botComboClass = owner.Main().classType;
		}
	}
}

void World::RemoteHitAlongPath(RemoteInst& inst)
{
	const Remote& remote = *inst.remote;
	if(remote.damageGroup == Remote::DamageGroup::eNONE || remote.damageGroup == Remote::DamageGroup::INVALID) return;
	if(remote.attackMultiplier <= 0 && remote.statuses.empty()) return;
	const f32 radius = remote.boundSize[0] * 0.5f;
	const vec2 origin = vec2(inst.pos) + inst.dir * RemoteBoundOffset(remote);
	foreach(it, players) {
		if(!RemoteCanAffect(inst, *it, false)) continue;
		if(it->team == players[inst.ownerIndex].team) continue;
		if(glm::length(vec2(it->body->GetWorldPos()) - origin) > radius + HitRadius(*it)) continue;
		RemoteHit(inst, *it);
	}
	Player& owner = players[inst.ownerIndex];
	foreach(npc, actorNpcList) {
		if(!NpcHittableBy(*npc, owner)) continue;
		const f32 dist = glm::length(vec2(npc->pos) - origin);
		if(dist > radius + NpcHitRadius(*npc)) continue;
		RemoteDamageNpc(inst, *npc, dist);
	}
}

void World::RemoteApplyShape(RemoteInst& inst)
{
	const Remote& remote = *inst.remote;
	f32 halfWidth = remote.boundSize[0] * 0.5f;
	f32 start = 0;
	f32 length = inst.range > 0 ? inst.range : remote.boundSize[1];
	switch(remote.boundType) {
		case Remote::BoundType::E_BOUND_CAPSULE: {
			length = remote.boundSize[1] + remote.boundSize[0];
			start = -remote.boundSize[0] * 0.5f;
		} break;
		case Remote::BoundType::E_BOUND_HAMMER:
		case Remote::BoundType::E_BOUND_BEAM:
		case Remote::BoundType::E_BOUND_BEAM_NIF:
		case Remote::BoundType::E_BOUND_LASER:
		case Remote::BoundType::E_BOUND_RAY: {
			if(length <= 0) length = MAX((f32)remote.boundSize[0], 300.0f);
		} break;
		default: return;
	}
	if(length <= 0) return;
	if(remote.boundType == Remote::BoundType::E_BOUND_BEAM_NIF && remote.polygonSides >= 3) {
		RemoteApplyPolygonOutline(inst);
		return;
	}
	RemoteHitWalls(inst, start + length);

	foreach(it, players) {
		const vec3 pos = it->body->GetWorldPos();
		if(fabsf(pos.z - (inst.pos.z - inst.hitZOffset)) > WEAPON_PROJECTILE_HIT_HEIGHT) continue;
		const vec2 center = vec2(inst.pos) + inst.dir * (start + length * 0.5f);
		const vec2 rel = LagClosestPos(*it, center) - vec2(inst.pos);
		const f32 along = glm::dot(rel, inst.dir);
		const f32 side = fabsf(glm::dot(rel, Perp(inst.dir)));
		if(along < start - HitRadius(*it) || along > start + length + HitRadius(*it)) continue;
		if(side > halfWidth + HitRadius(*it)) continue;
		RemoteHit(inst, *it);
	}
	Player& owner = players[inst.ownerIndex];
	foreach(npc, actorNpcList) {
		const bool hittable = NpcHittableBy(*npc, owner);
		if(!hittable && remote.friendlyStatuses.empty()) continue;
		const f32 r = NpcHitRadius(*npc);
		const vec2 rel = vec2(npc->pos) - vec2(inst.pos);
		const f32 along = glm::dot(rel, inst.dir);
		const f32 side = fabsf(glm::dot(rel, Perp(inst.dir)));
		if(along < start - r || along > start + length + r || side > halfWidth + r) continue;
		if(hittable) RemoteDamageNpc(inst, *npc, along);
		else RemoteFriendNpc(inst, *npc);
	}
}

void World::RemoteHitWalls(RemoteInst& inst, f32 farEdge)
{
	const Remote& remote = *inst.remote;
	if(inst.wallHitDone || inst.damage <= 0 || remote.attackMultiplier <= 0 || farEdge <= 0) return;
	if(remote.damageGroup != Remote::DamageGroup::eENEMY && remote.damageGroup != Remote::DamageGroup::eALL) return;
	const Player& owner = players[inst.ownerIndex];
	if(!owner.body) return;
	const vec2 from = vec2(owner.body->GetWorldPos());
	const vec2 to = vec2(inst.pos) + inst.dir * farEdge;
	if(glm::length(to - from) < 1.0f) return;
	f32 wallDist = 0;
	const PxRigidActor* wallActor = nullptr;
	ActorDynamic* wallGate = nullptr;
	if(!RaycastShot(vec3(from, inst.wallZ), vec3(to, inst.wallZ), (i32)owner.team, &wallDist, &wallActor, &wallGate) || (!wallActor && !wallGate)) return;
	inst.wallHitDone = true;
	OnDynamicHit(wallActor, inst.clientRemoteID, ClientHandle::INVALID, inst.casterLocal ? owner.clientHd : ClientHandle::INVALID, vec3(from + glm::normalize(to - from) * wallDist, inst.wallZ), wallGate, (i32)inst.ownerIndex, (i32)(inst.damage * remote.attackMultiplier), remote.ID);
}

static const f32 MEASURED_POLYGON_BLADE_RADIUS = 470.0f;

void World::RemoteApplyPolygonOutline(RemoteInst& inst)
{
	const Remote& remote = *inst.remote;
	const f32 halfWidth = remote.boundSize[0] * 0.5f;
	const f32 inner = MEASURED_POLYGON_BLADE_RADIUS * cosf(PI / (f32)remote.polygonSides);
	foreach(it, players) {
		const vec3 pos = it->body->GetWorldPos();
		if(fabsf(pos.z - (inst.pos.z - inst.hitZOffset)) > WEAPON_PROJECTILE_HIT_HEIGHT) continue;
		const f32 d = glm::length(LagClosestPos(*it, vec2(inst.pos)) - vec2(inst.pos));
		if(d < inner - halfWidth - HitRadius(*it) || d > MEASURED_POLYGON_BLADE_RADIUS + halfWidth + HitRadius(*it)) continue;
		RemoteHit(inst, *it);
	}
	Player& owner = players[inst.ownerIndex];
	foreach(npc, actorNpcList) {
		if(!NpcHittableBy(*npc, owner)) continue;
		const f32 r = NpcHitRadius(*npc);
		const f32 d = glm::length(vec2(npc->pos) - vec2(inst.pos));
		if(d < inner - halfWidth - r || d > MEASURED_POLYGON_BLADE_RADIUS + halfWidth + r) continue;
		RemoteDamageNpc(inst, *npc, d);
	}
}

void World::RemoteApplyArea(RemoteInst& inst)
{
	const Remote& remote = *inst.remote;
	const bool capsule = remote.boundType == Remote::BoundType::E_BOUND_CAPSULE;
	if(remote.boundType != Remote::BoundType::E_BOUND_SPHERE && remote.boundType != Remote::BoundType::E_BOUND_BOX && !capsule) {
		RemoteApplyShape(inst);
		return;
	}
	const f32 roundRadius = CapsuleOrSphereRadius(remote);
	RemoteHitWalls(inst, RemoteBoundOffset(remote) + (remote.boundType == Remote::BoundType::E_BOUND_BOX ? remote.boundSize[1] * 0.5f : roundRadius));
	const bool fanBox = remote.boundType == Remote::BoundType::E_BOUND_BOX && remote.angle > 0;
	const vec2 origin = capsule && remote.attachFireObject ? vec2(players[inst.ownerIndex].body->GetWorldPos()) : vec2(inst.pos) + inst.dir * RemoteBoundOffset(remote);
	if(remote.boundType == Remote::BoundType::E_BOUND_SPHERE || fanBox || capsule) {
		const bool cone = remote.angle > 0;
		const f32 radius = roundRadius;
		foreach(it, players) {
			const vec3 pos = it->body->GetWorldPos();
			if(fabsf(pos.z - (inst.pos.z - inst.hitZOffset)) > WEAPON_PROJECTILE_HIT_HEIGHT) continue;
			const vec2 rel = LagClosestPos(*it, origin) - origin;
			const f32 dist = glm::length(rel);
			if(dist > radius + HitRadius(*it)) continue;
			if(remote.angle > 0 && dist > HitRadius(*it)) {
				if(glm::dot(rel / dist, inst.dir) < cosf(MIN(remote.angle * 0.5f, 180.0f) * PI / 180.0f)) continue;
			}
			RemoteHit(inst, *it);
		}
	}
	else if(remote.boundType == Remote::BoundType::E_BOUND_BOX) {
		const f32 halfWidth = remote.boundSize[0] * 0.5f;
		const f32 length = remote.boundSize[1];
		foreach(it, players) {
			const vec3 pos = it->body->GetWorldPos();
			if(fabsf(pos.z - (inst.pos.z - inst.hitZOffset)) > WEAPON_PROJECTILE_HIT_HEIGHT) continue;
			const f32 back = length * 0.5f;
			const vec2 center = origin + inst.dir * (length * 0.5f - back);
			const vec2 rel = LagClosestPos(*it, center) - origin + inst.dir * back;
			const f32 along = glm::dot(rel, inst.dir);
			const f32 side = fabsf(glm::dot(rel, Perp(inst.dir)));
			if(along < -HitRadius(*it) || along > length + HitRadius(*it)) continue;
			if(side > halfWidth + HitRadius(*it)) continue;
			RemoteHit(inst, *it);
		}
	}
	RemoteHitNpcs(inst);
}

void World::RemoteHitNpcs(RemoteInst& inst)
{
	const Remote& remote = *inst.remote;
	const i32 base = (i32)(inst.damage * remote.attackMultiplier);
	const bool enemies = !(base <= 0 && remote.statuses.empty()) && (remote.damageGroup == Remote::DamageGroup::eENEMY || remote.damageGroup == Remote::DamageGroup::eALL);
	if(!enemies && remote.friendlyStatuses.empty()) return;
	Player& owner = players[inst.ownerIndex];
	const bool sphere = remote.boundType == Remote::BoundType::E_BOUND_SPHERE || remote.boundType == Remote::BoundType::E_BOUND_CAPSULE || remote.angle > 0;
	const f32 roundRadius = CapsuleOrSphereRadius(remote);
	foreach(npc, actorNpcList) {
		const bool hittable = NpcHittableBy(*npc, owner);
		if(hittable ? !enemies : remote.friendlyStatuses.empty()) continue;
		const f32 r = NpcHitRadius(*npc);
		const vec2 rel = remote.boundType == Remote::BoundType::E_BOUND_CAPSULE && remote.attachFireObject ? vec2(npc->pos) - vec2(owner.body->GetWorldPos()) : vec2(npc->pos) - vec2(inst.pos) - inst.dir * RemoteBoundOffset(remote);
		const f32 dist = glm::length(rel);
		if(sphere) {
			if(dist > roundRadius + r) continue;
			if(remote.angle > 0 && dist > r && glm::dot(rel / dist, inst.dir) < cosf(MIN(remote.angle * 0.5f, 180.0f) * PI / 180.0f)) continue;
		}
		else {
			const f32 length = remote.boundSize[1];
			const vec2 local = rel + inst.dir * (length * 0.5f);
			const f32 along = glm::dot(local, inst.dir);
			if(along < -r || along > length + r || fabsf(glm::dot(local, Perp(inst.dir))) > remote.boundSize[0] * 0.5f + r) continue;
		}
		if(hittable) RemoteDamageNpc(inst, *npc, dist);
		else RemoteFriendNpc(inst, *npc);
	}
}

bool World::RemoteFriendNpc(RemoteInst& inst, ActorNpc& npc)
{
	const Remote& remote = *inst.remote;
	if(remote.friendlyStatuses.empty() || !(remote.includeParty || remote.contactFriend)) return false;
	Player& owner = players[inst.ownerIndex];
	const Faction ownFaction = owner.team == 0 ? Faction::RED : Faction::BLUE;
	if(npc.faction != ownFaction || npc.hp <= 0 || npc.removeAt != Time::ZERO || npc.UID == inst.sourceUID || GetGameXmlContent().CreatureUntargetable((i32)npc.docID)) return false;
	const Time until = TimeAdd(localTime, TimeMsToTime(MAX(remote.hitInvalidTime, 0.1f) * 1000.0));
	bool recorded = false;
	foreach(h, inst.npcHits) {
		if(h->uid != npc.UID) continue;
		if(localTime < h->until) return false;
		h->until = until;
		recorded = true;
		break;
	}
	if(!recorded) inst.npcHits.push_back({ npc.UID, until });
	ApplyFriendlyRemoteStatuses(owner, remote, nullptr, &npc);
	return true;
}

void World::ChangeCasterActionOnHit(RemoteInst& inst, Player& owner, ActorUID targetUID, const vec3& targetPos)
{
	const Remote& remote = *inst.remote;
	if(remote.changeCasterAction == ActionStateID::INVALID || inst.actionChanged || owner.dead || owner.Main().UID != inst.casterMasterUID) return;
	inst.actionChanged = true;
	LOG("Remote :: %ls hit -> caster action %d", owner.name.data(), (i32)remote.changeCasterAction);
	foreach(pg, skillProgramList) if(pg->casterUID == owner.Main().UID && pg->skillID == inst.skillID && pg->actionID != remote.changeCasterAction) pg->Finish();
	owner.dashDone = owner.dashDist;
	owner.dashUntil = localTime;
	hitActionOnly = true;
	PlayerCastSkill(owner, inst.skillID, targetPos, Slice<const ActorUID>(&targetUID, 1), 0, remote.changeCasterAction);
	hitActionOnly = false;
}

f32 World::RepeatHitMul(u32 castID, u32 ownerIndex, u32 targetKey, RemoteIdx remoteID, SkillID skillID)
{
	static const u32 REPEAT_HIT_CASTER_KEY = 0xC0000000u;
	const RepeatHitDecayRule* decay = FindRepeatHitDecay(remoteID);
	if(!decay && !GetGameXmlContent().IsRepeatHitReduced((i32)skillID)) return 1.0f;
	if(decay && decay->perCaster) castID = REPEAT_HIT_CASTER_KEY | ownerIndex;
	RepeatHit* rh = nullptr;
	foreach(r, repeatHits) if(r->castID == castID && r->targetIndex == targetKey) rh = &*r;
	if(!rh) {
		repeatHits.push_back({ castID, targetKey, 0, localTime });
		rh = &repeatHits.back();
	}
	const f64 idle = TimeDiffSec(TimeDiff(rh->at, localTime));
	if(decay && decay->perCaster && decay->resetSec > 0) {
		const i32 cap = (i32)ceilf((1.0f - decay->floor) / decay->stepPerHit - 0.001f);
		rh->count = MAX(MIN(rh->count, cap) - (i32)(idle / decay->resetSec), 0);
	}
	else if(decay && decay->resetSec > 0 && idle > decay->resetSec) rh->count = 0;
	const f32 mul = decay ? MAX(1.0f - decay->stepPerHit * rh->count, decay->floor) : MAX(1.0f - REPEAT_HIT_REDUCTION * rh->count, 1.0f - REPEAT_HIT_REDUCTION_CAP);
	rh->count++;
	rh->at = localTime;
	return mul;
}

bool World::RemoteDamageNpc(RemoteInst& inst, ActorNpc& npc, f32 dist)
{
	const Remote& remote = *inst.remote;
	const i32 base = (i32)(inst.damage * remote.attackMultiplier);
	if(remote.damageGroup != Remote::DamageGroup::eENEMY && remote.damageGroup != Remote::DamageGroup::eALL) return false;
	if((remote.vs & (1u << Remote::VS_PLAYER_CHARACTER)) && !(remote.vs & (1u << Remote::VS_NPC_MONSTER))) return false;
	if(remote.targetOnly && inst.target != ActorUID::INVALID && inst.target != npc.UID) return false;
	if(remote.activateCount == 1 && inst.singleHitDone) return false;
	const Time until = TimeAdd(localTime, TimeMsToTime(MAX(remote.hitInvalidTime, 0.1f) * 1000.0));
	bool recorded = false;
	foreach(h, inst.npcHits) {
		if(h->uid != npc.UID) continue;
		if(localTime < h->until) return false;
		h->until = until;
		recorded = true;
		break;
	}
	if(!recorded) inst.npcHits.push_back({ npc.UID, until });
	if(inst.speed <= 0) inst.hitsLeft--;
	inst.singleHitDone = true;
	if(base > 0) {
		Replication::RemoteDamage hit;
		hit.remoteID = inst.clientRemoteID != 0 ? inst.clientRemoteID - (inst.sentByDoc ? 1 : 0) : inst.damageRemoteID;
		hit.remoteDoc = remote.ID;
		hit.remotePos = inst.pos;
		hit.dir = inst.dir;
		hit.seedOwnerHd = inst.seedOwnerHd;
		f32 mul = RemoteDistanceFalloff(remote, dist) * RepeatHitMul(inst.castID, inst.ownerIndex, (u32)npc.UID | VOLLEY_NPC_TARGET_BIT, remote.ID, inst.skillID);
		if(remote.hitDamageMultiplier != 0) {
			mul *= MAX(1.0f + remote.hitDamageMultiplier * inst.enemiesHit, 1.0f + remote.maxHitDamageMultiplier);
			if(inst.enemiesHit < 255) inst.enemiesHit++;
		}
		DamageNpc(players[inst.ownerIndex], npc, inst.skillID, (i32)(base * mul), &hit);
	}
	Player& owner = players[inst.ownerIndex];
	if(inst.clientRemoteID != 0) {
		inst.npcActivation += NpcDensity(npc);
		replication->SendRemoteActivatedToAll(inst.clientRemoteID - (inst.sentByDoc ? 1 : 0), inst.seedOwnerHd, inst.casterLocal ? owner.clientHd : ClientHandle::INVALID, npc.UID, inst.npcActivation, inst.pos);
	}
	if(remote.onHitOwnerStatus != 0 && !owner.dead && (inst.sourceUID == ActorUID::INVALID || !FindNpcActor(inst.sourceUID))) {
		const ActorUID prevCaster = statusCasterOverride;
		statusCasterOverride = npc.UID;
		ApplyStatus(owner, owner, remote.onHitOwnerStatus);
		statusCasterOverride = prevCaster;
	}
	if(npc.hp > 0 && !owner.dead) TriggerSkillHitEvent(inst, owner, npc.UID, npc.pos);
	if(inst.skillID == SkillID::INVALID && base > 0) WeaponHitPlagueNpc(owner, npc);
	if(npc.hp > 0) ChangeCasterActionOnHit(inst, owner, npc.UID, npc.pos);
	if(npc.hp <= 0 || npc.ownerUID != ActorUID::INVALID) return true;
	const ActorUID prevCaster = statusCasterOverride;
	if(inst.sourceUID != ActorUID::INVALID && FindNpcActor(inst.sourceUID)) statusCasterOverride = inst.sourceUID;
	foreach_const(st, remote.statuses) {
		if(st->rate >= 100 || (i32)(RandUint() % 100) < st->rate) ApplyStatusToNpc(players[inst.ownerIndex], npc, st->statusID, false);
	}
	statusCasterOverride = prevCaster;
	return true;
}

void World::UpdateRemotes(f32 delta)
{
	SpawnPendingRemotes();

	for(int i = 0; i < (i32)remoteInsts.size(); ) {
		RemoteInst& inst = remoteInsts[i];
		const Remote& remote = *inst.remote;
		const f32 elapsed = (f32)TimeDiffSec(TimeDiff(inst.spawnTime, localTime));
		bool destroyed = inst.killNow;
		Player& instOwner = players[inst.ownerIndex];
		BeginRemoteActivation(inst);

		if(remote.attachTargetObject && inst.target != ActorUID::INVALID) {
			const ActorMaster* targetMaster = FindMasterActor(inst.target);
			if(!targetMaster || targetMaster->parent->dead) destroyed = true;
			else inst.pos = vec3(vec2(targetMaster->parent->body->GetWorldPos()), inst.pos.z);
		}
		if(remote.behaviorType == Remote::BehaviorType::CASTER_TARGET && !instOwner.dead) MoveCasterTargetRemote(inst, instOwner, delta);
		const bool attached = remote.attachFireObject || remote.behaviorType == Remote::BehaviorType::ORBIT ||
			remote.behaviorType == Remote::BehaviorType::BEAM;
		const ActorNpc* fireNpc = attached && inst.sourceUID != ActorUID::INVALID ? FindNpcActor(inst.sourceUID) : nullptr;
		if(attached && inst.sourceUID != ActorUID::INVALID) {
			if(!fireNpc || (fireNpc->removeAt != Time::ZERO && localTime >= fireNpc->removeAt)) destroyed = true;
			else inst.pos = fireNpc->pos;
		}
		else if(attached) {
			if(instOwner.dead) {
				destroyed = true;
			}
			else {
				const f32 yaw = inst.firePath ? inst.firePathYaw : instOwner.input.rot.upperYaw;
				inst.dir = vec2(cosf(yaw), sinf(yaw));
				const f32 attachOffset = remote.forceTargetDir || inst.nodeName == SCENE_ROOT_NODE ? -remote.offsetY : remote.offsetY;
				inst.pos = instOwner.body->GetWorldPos() + vec3(inst.dir * (attachOffset + inst.attachForward) + Perp(inst.dir) * inst.attachSide, FIRE_NODE_HEIGHT);
			}
		}

		if(!destroyed && inst.aniEndAt != Time::ZERO && localTime >= inst.aniEndAt) destroyed = true;
		if(!destroyed && inst.endWithSource) {
			const ActorNpc* source = FindNpcActor(inst.sourceUID);
			if(!source || source->removeAt != Time::ZERO || (source->despawnAt != Time::ZERO && localTime >= source->despawnAt) || (source->hpMax > 0 && source->hp <= 0)) destroyed = true;
		}
		if(!destroyed && inst.timeEventActive && localTime >= inst.nextTimeEvent) RunRemoteTimeEvent(inst, elapsed);

		if(!destroyed) {
			if(remote.behaviorType == Remote::BehaviorType::CHAIN) destroyed = UpdateChainRemote(inst);
			else if(attached) {
				if(remote.lifeTime <= 0 || elapsed < remote.lifeTime) ApplyAttachedRemote(inst, instOwner);
				if(inst.hitsLeft <= 0 && (remote.behaviorType == Remote::BehaviorType::ORBIT || remote.lifeTime > 0)) destroyed = true;
			}
			else switch(remote.behaviorType) {
				case Remote::BehaviorType::CASTER_TARGET: break;
				case Remote::BehaviorType::TARGET:
				case Remote::BehaviorType::FOLLOW_TARGET:
				case Remote::BehaviorType::HOMING: destroyed = UpdateTargetRemote(inst, delta, elapsed); break;
				case Remote::BehaviorType::GRAPH: destroyed = UpdateGraphRemote(inst, delta); break;
				default: destroyed = UpdateLinearRemote(inst, delta, elapsed); break;
			}
		}

		if(!destroyed && remote.homingLength > 0 && inst.speed > 0 && inst.clientRemoteID != 0 && inst.seedOwnerHd == ClientHandle::INVALID) {
			if(!inst.syncedInit) { inst.syncedInit = true; inst.syncedTarget = inst.target; }
			inst.syncIn -= delta;
			const bool retarget = inst.target != inst.syncedTarget;
			if(retarget || inst.syncIn <= 0) {
				inst.syncIn = retarget ? MEASURED_REMOTE_SYNC_PERIOD_SEC : inst.syncIn + MEASURED_REMOTE_SYNC_PERIOD_SEC;
				inst.syncedTarget = inst.target;
				replication->SendRemoteSyncTargetToAll(inst.clientRemoteID, inst.target, inst.pos, &inst.dir, inst.dirZ);
			}
		}
		const bool chainHop = remote.behaviorType == Remote::BehaviorType::CHAIN && remote.destroyCreateRemote != RemoteIdx::INVALID;
		const f32 destroyDelay = !chainHop && inst.speed <= 0 && (remote.destroyCreateRemote != RemoteIdx::INVALID || !remote.onDestroy.empty()) ? REMOTE_DESTROY_CHILD_DELAY : 0.0f;
		const f32 lifeEnd = chainHop && !(remote.includeParty && !remote.friendlyStatuses.empty()) && remote.lifeTime > MEASURED_CHAIN_HOP_SEC ? MEASURED_CHAIN_HOP_SEC : remote.lifeTime;
		if(!destroyed && remote.lifeTime > 0 && elapsed >= lifeEnd + destroyDelay) destroyed = true;
		if(!destroyed && remote.lifeTime <= 0 && inst.speed <= 0 && inst.areaDone && elapsed >= destroyDelay) destroyed = true;
		const bool toggleRemote = instOwner.toggleSkill != SkillID::INVALID && inst.castID == instOwner.toggleCastID;
		const bool boundRemote = attached || remote.attachTargetObject || inst.endWithSource || inst.aniEndAt != Time::ZERO;
		if(!destroyed && remote.lifeTime <= 0 && !toggleRemote && !boundRemote && elapsed >= REMOTE_MAX_AGE) {
			WARN("Remote :: %d (inst %u) is older than %.0fs, destroyed", (i32)remote.ID, inst.instID, REMOTE_MAX_AGE);
			destroyed = true;
		}
		if(destroyed && !inst.killNow && inst.timeEventActive && ((remote.lifeTime > 0 && elapsed + delta >= remote.lifeTime) || (inst.speed > 0 && inst.range > 0 && inst.travelled >= inst.range - 0.01f))) {
			const f32 endSec = MIN(remote.lifeTime > 0 ? remote.lifeTime : FLT_MAX, inst.speed > 0 && inst.range > 0 ? inst.range / inst.speed : FLT_MAX);
			if(TimeDiffSec(TimeDiff(inst.spawnTime, inst.nextTimeEvent)) <= endSec + 0.001f) RunRemoteTimeEvent(inst, elapsed);
		}

		FlushRemoteActivation();

		if(destroyed) DestroyRemote(i);
		else i++;
	}

	ExpireHitRecords();
}

void World::SpawnPendingRemotes()
{
	for(auto pr = pendingRemotes.begin(); pr != pendingRemotes.end(); ) {
		if(localTime < pr->time) {
			++pr;
			continue;
		}

		Player& owner = players[pr->ownerIndex];
		if(pr->remove) {
			bool ended = false;
			bool sameCast = false;
			foreach_const(ri, remoteInsts) if(ri->ownerIndex == pr->ownerIndex && ri->remote && ri->remote->ID == pr->idx && !ri->killNow && ri->castID == pr->castID) sameCast = true;
			foreach(ri, remoteInsts) {
				if(ri->ownerIndex == pr->ownerIndex && ri->remote && ri->remote->ID == pr->idx && !ri->killNow && (!sameCast || ri->castID == pr->castID)) { ri->killNow = true; ended = true; }
			}
			if(ended) LOG("Remote :: %ls ends live remote %d (action Remove)", owner.name.data(), (i32)pr->idx);
			pr = pendingRemotes.erase_unsorted(pr);
			continue;
		}
		vec2 dir = vec2(cosf(pr->angle), sinf(pr->angle));
		const Remote* remote = GetGameXmlContent().FindRemote(pr->idx);
		const f32 offsetY = remote ? (remote->forceTargetDir ? -remote->offsetY : remote->offsetY) : 0;
		vec3 pos = owner.body->GetWorldPos() + vec3(dir * offsetY, pr->nodeName == SCENE_ROOT_NODE ? (remote ? remote->offsetZ : 0.0f) : FIRE_NODE_HEIGHT);
		const f32 ownerZ = owner.body->GetWorldPos().z;

		bool atAimedPoint = pr->atTarget;
		switch((ActionCommand::TargetPreset)pr->preset) {
			case ActionCommand::TargetPreset::POS_FFF:
			case ActionCommand::TargetPreset::POS_TFF:
			case ActionCommand::TargetPreset::POS_TTT_1:
			case ActionCommand::TargetPreset::POS_TFF_1_800_1000: {
				atAimedPoint = glm::length(vec2(pr->castPos)) > 1.0f;
				const vec3 base = glm::length(vec2(pr->castPos)) > 1.0f ? pr->castPos : owner.body->GetWorldPos();
				pos = vec3(vec2(base) + dir * (pr->offsetFront + (remote && remote->maxSpeed <= 0 && remote->initSpeed <= 0 && remote->angle <= 0 && (remote->boundType == Remote::BoundType::E_BOUND_BOX || remote->boundType == Remote::BoundType::E_BOUND_SPHERE) ? offsetY : 0.0f)) + Perp(dir) * pr->offsetSide, owner.body->GetWorldPos().z);
			} break;
			case ActionCommand::TargetPreset::CURRENT_FFF: {
				const ActorMaster* targetMaster = pr->target != ActorUID::INVALID ? FindMasterActor(pr->target) : nullptr;
				if(targetMaster && (!remote || (remote->maxSpeed <= 0 && !remote->attachFireObject))) pos = targetMaster->parent->body->GetWorldPos();
			} break;
			case ActionCommand::TargetPreset::LAST_CHILD_FFF:
			case ActionCommand::TargetPreset::LAST_CHILD_TFF: {
				const ActorNpc* child = FindNpcActor(owner.lastChildUID);
				if(child && localTime < child->despawnAt) {
					pos = vec3(vec2(child->pos), owner.body->GetWorldPos().z);
					remoteTargetEntityOverride = child->UID;
				}
			} break;
			default: {
				if(pr->offsetFront != 0 || pr->offsetSide != 0) pos = vec3(vec2(owner.body->GetWorldPos()) + dir * pr->offsetFront + Perp(dir) * pr->offsetSide, owner.body->GetWorldPos().z);
			} break;
		}
		if(pr->atTarget) {
			const ActorMaster* targetMaster = pr->target != ActorUID::INVALID ? FindMasterActor(pr->target) : nullptr;
			if(targetMaster) pos = targetMaster->parent->body->GetWorldPos();
			else if(glm::length(vec2(pr->castPos)) > 1.0f) pos = vec3(vec2(pr->castPos), owner.body->GetWorldPos().z);
		}

		if(const SummonFireRule* summonFire = remote && pr->sourceUID != ActorUID::INVALID ? FindSummonFireRule(remote->ID) : nullptr) {
			if(const ActorNpc* source = FindNpcActor(pr->sourceUID)) {
				const vec2 toOwner = vec2(owner.body->GetWorldPos()) - vec2(source->pos);
				const vec2 back = glm::length(toOwner) > 1.0f ? glm::normalize(toOwner) : -dir;
				pos = vec3(vec2(source->pos) + back * summonFire->towardOwner, source->pos.z + summonFire->up);
			}
		}
		const ActionCommand::TargetPreset firePreset = (ActionCommand::TargetPreset)pr->preset;
		const bool fromCaster = !atAimedPoint && !pr->atTarget && pr->sourceUID == ActorUID::INVALID && firePreset != ActionCommand::TargetPreset::CURRENT_FFF &&
			firePreset != ActionCommand::TargetPreset::LAST_CHILD_FFF && firePreset != ActionCommand::TargetPreset::LAST_CHILD_TFF;
		const FireNodeRule* fireNode = fromCaster && remote ? FindFireNodeRule(remote->ID) : nullptr;
		const RemoteIdx sentDoc = pr->sendIdx != RemoteIdx::INVALID ? pr->sendIdx : pr->idx;
		const GameXmlContent::MeasuredFireNode* measured = fromCaster && remote && !pr->remove ? GetGameXmlContent().FindMeasuredFireNode((i32)pr->skillID, (i32)sentDoc, pr->docIndex) : nullptr;
		if(measured) {
			pos = owner.body->GetWorldPos() + vec3(dir * (measured->forward + offsetY) + Perp(dir) * measured->side, measured->up + remote->offsetZ);
			const vec2 toAim = vec2(pr->castPos) - vec2(pos);
			if(remote->behaviorType == Remote::BehaviorType::GRAPH && glm::length(vec2(pr->castPos)) > 1.0f && glm::length(toAim) > 1.0f) dir = glm::normalize(toAim);
		}
		else if(fireNode) {
			pos += vec3(dir * fireNode->forward + Perp(dir) * fireNode->side, 0);
			pos.z = owner.body->GetWorldPos().z + fireNode->up;
		}

		if(remote && remote->offsetX != 0) pos -= vec3(Perp(dir) * remote->offsetX, 0);
		if(remote && IsChainedZoneRemote(remote->ID)) {
			const f32 length = (f32)remote->boundSize[1];
			foreach_const(ri, remoteInsts) {
				if(ri->castID != pr->castID || !ri->remote || ri->remote->ID != pr->idx || ri->killNow) continue;
				const f32 ahead = glm::dot(vec2(pos) - vec2(ri->pos), dir);
				if(ahead >= 0 && ahead < length) pos = vec3(vec2(ri->pos) + dir * length, pos.z);
			}
		}
		if(owner.dead) remoteTargetEntityOverride = ActorUID::INVALID;
		if(!owner.dead) {
			remoteParentOverride = pr->sourceUID;
			if(remoteTargetEntityOverride != ActorUID::INVALID) {
				remoteSendFrom = owner.body->GetWorldPos();
				remoteSendFromSet = true;
			}
			else if(fromCaster && remote && remote->maxSpeed > 0 && offsetY != 0 && pr->offsetFront == 0 && pr->offsetSide == 0) {
				remoteSendFrom = pos - vec3(dir * offsetY, remote->offsetZ);
				remoteSendFromSet = true;
			}
			spawnCostLevel = pr->costLevel;
			remoteSendIdxOverride = pr->sendIdx;
			remoteSendRootDir = pr->rootDummyDir;
			RemoteInst* spawned = SpawnRemote(pr->ownerIndex, pr->idx, pos, dir, pr->target, pr->skillID, pr->castID, pr->damage, true, &pr->castPos, pr->nodeName, false, pr->stopAtTarget, false, !remote || RemoteCreateSendsEntity((ActionCommand::TargetPreset)pr->preset, remote->behaviorType));
			remoteParentOverride = ActorUID::INVALID;
			spawnCostLevel = nullptr;
			remoteSendIdxOverride = RemoteIdx::INVALID;
			remoteSendRootDir = false;
			remoteTargetEntityOverride = ActorUID::INVALID;
			remoteSendFromSet = false;
			if(spawned) {
				if(remote && remote->attachFireObject && measured) {
					spawned->attachForward = measured->forward;
					spawned->attachSide = measured->side;
				}
				else if(remote && remote->attachFireObject && fireNode) {
					spawned->attachForward = fireNode->forward;
					spawned->attachSide = fireNode->side;
				}
				spawned->sourceUID = pr->sourceUID;
				if(pr->casterMasterUID != ActorUID::INVALID) spawned->casterMasterUID = pr->casterMasterUID;
				const ClassType casterClass = pr->casterClass != ClassType::NONE ? pr->casterClass : owner.Main().classType;
				spawned->casterClass = casterClass;
				spawned->costLevel = pr->costLevel;
				spawned->centeredBox = atAimedPoint;
				if(remote && remote->offsetZ != 0 && (measured || fabsf(pos.z - (ownerZ + remote->offsetZ)) < 1.0f)) spawned->hitZOffset = remote->offsetZ;
				spawned->aniEndAt = pr->aniEndAt;
				spawned->endWithSource = pr->endWithSource;
				if(measured) {
					spawned->measuredNode = measured;
					spawned->castOrigin = owner.body->GetWorldPos();
					spawned->castYaw = pr->angle;
				}
				const f32 nodeForward = atAimedPoint || fireNode || measured ? 0 : WeaponNodeForward(GetGameXmlContent(), casterClass, pr->skillID, *spawned->remote, pr->nodeName);
				if(nodeForward > 0) {
					spawned->pos += vec3(dir * nodeForward, 0);
					LOG("Remote :: %d weapon node forward %.0f (skill indicator)", (i32)pr->idx, nodeForward);
				}
			}
		}
		pr = pendingRemotes.erase_unsorted(pr);
	}
}

void World::SetCasterRemoteDest(u32 ownerIndex, u32 clientRemoteID, const vec3& dest)
{
	foreach(r, remoteInsts) {
		if(r->ownerIndex != ownerIndex || r->clientRemoteID != clientRemoteID || !r->remote || r->remote->behaviorType != Remote::BehaviorType::CASTER_TARGET) continue;
		r->casterDest = dest;
		r->casterDestSet = true;
	}
}

void World::MoveCasterTargetRemote(RemoteInst& inst, Player& instOwner, f32 delta)
{
	const Remote& remote = *inst.remote;
	const vec2 ownerPos = vec2(instOwner.body->GetWorldPos());
	if(inst.followDist < 0) inst.followDist = glm::length(vec2(inst.pos) - ownerPos);
	vec2 desired;
	const ActorMaster* tm = inst.target != ActorUID::INVALID ? FindMasterActor(inst.target) : nullptr;
	if(inst.casterDestSet) desired = vec2(inst.casterDest);
	else if(instOwner.clientHd == ClientHandle::INVALID && tm && !tm->parent->dead) desired = vec2(tm->parent->body->GetWorldPos());
	else {
		const f32 yaw = instOwner.input.rot.upperYaw;
		desired = ownerPos + vec2(cosf(yaw), sinf(yaw)) * inst.followDist;
	}
	const vec2 rel = desired - vec2(inst.pos);
	const f32 len = glm::length(rel);
	const f32 step = (remote.maxSpeed > 0 ? remote.maxSpeed : 600.0f) * delta;
	if(len > 1.0f) inst.pos += vec3(rel / len * MIN(step, len), 0);
	if(remote.boundType != Remote::BoundType::E_BOUND_NONE && remote.damageGroup != Remote::DamageGroup::eNONE) RemoteApplyArea(inst);
}

void World::RunRemoteTimeEvent(RemoteInst& inst, f32 elapsed)
{
	const Remote& remote = *inst.remote;
	if(remote.timeEventRecycle && remote.timeEventDelay > 0.01f) inst.nextTimeEvent = TimeAdd(inst.nextTimeEvent, TimeMsToTime(remote.timeEventDelay * 1000.0));
	else inst.timeEventActive = false;
	if(remote.timeChangeSpeed > 0) {
		inst.speed = remote.timeChangeSpeed;
		if(remote.maxDistance <= 0 && remote.lifeTime > 0) inst.range = inst.travelled + MAX(remote.lifeTime - elapsed, 0.0f) * inst.speed;
	}
	const RemoteInst copy = inst;
	SpawnRemoteRules(*this, copy, remote.onTime, remote.creatorOnTime, copy.pos, copy.excludeIndex, RemoteIdx::INVALID);
}

bool World::UpdateChainRemote(RemoteInst& inst)
{
	const Remote& remote = *inst.remote;
	const bool hopWait = !(remote.includeParty && !remote.friendlyStatuses.empty()) && (remote.destroyCreateRemote != RemoteIdx::INVALID || inst.chainNpcHit != ActorUID::INVALID || inst.chainHitMask != 0);
	if(hopWait && TimeDiffSec(TimeDiff(inst.spawnTime, localTime)) < MEASURED_CHAIN_HOP_SEC) return false;
	if(!inst.areaDone) {
		inst.areaDone = true;
		const f32 sight = remote.sight > 0 ? remote.sight : 1450.0f;
		Player* best = nullptr;
		const u32 ownerTeam = players[inst.ownerIndex].team;
		const f32 halfCos = remote.fov > 0 && remote.fov < 360 ? cosf(remote.fov * 0.5f * PI / 180.0f) : -2.0f;
		const auto inCone = [&](const vec2& at) {
			const vec2 rel = at - vec2(inst.pos);
			const f32 len = glm::length(rel);
			return len < 1.0f || glm::dot(rel / len, inst.dir) >= halfCos;
		};
		ActorNpc* bestNpc = nullptr;
		const bool friendChain = remote.includeParty && !remote.friendlyStatuses.empty();
		const bool firstHop = inst.chainHitMask == 0 && inst.chainNpcHit == ActorUID::INVALID;
		for(i32 pass = 0; pass < (friendChain ? 1 : 4) && !best && !bestNpc; pass++) {
			f32 bestDist = sight + WEAPON_TARGET_RADIUS;
			foreach(it, players) {
				if(friendChain ? (firstHop && it->index == inst.ownerIndex) : (it->team != ownerTeam) != (pass < 2)) continue;
				if(!RemoteCanAffect(inst, *it, remote.vsOwner)) continue;
				if(!friendChain && pass % 2 == 0 && ((inst.chainHitMask >> it->index) & 1)) continue;
				if(!inCone(vec2(it->body->GetWorldPos()))) continue;
				const f32 d = glm::length(vec2(it->body->GetWorldPos()) - vec2(inst.pos));
				if(d < bestDist) { bestDist = d; best = &*it; }
			}
			if(pass < 2) {
				const Faction ownFaction = ownerTeam == 0 ? Faction::RED : Faction::BLUE;
				foreach(npc, actorNpcList) {
					const bool ally = friendChain && npc->faction == ownFaction && npc->removeAt == Time::ZERO && !GetGameXmlContent().CreatureUntargetable((i32)npc->docID);
					if(npc->hp <= 0 || !(ally || NpcHittableBy(*npc, players[inst.ownerIndex])) || npc->UID == inst.chainNpcHit || !inCone(vec2(npc->pos))) continue;
					const f32 d = glm::length(vec2(npc->pos) - vec2(inst.pos));
					if(d < bestDist) { bestDist = d; bestNpc = &*npc; best = nullptr; }
				}
			}
		}
		const vec2 beamFrom = vec2(inst.pos);
		const ActorUID prevNpc = inst.chainNpcHit;
		if(bestNpc) {
			inst.dir = glm::length(vec2(bestNpc->pos - inst.pos)) > 1.0f ? glm::normalize(vec2(bestNpc->pos - inst.pos)) : inst.dir;
			inst.pos = vec3(vec2(bestNpc->pos), inst.pos.z);
			if(NpcHittableBy(*bestNpc, players[inst.ownerIndex])) RemoteDamageNpc(inst, *bestNpc, 0.0f);
			else RemoteFriendNpc(inst, *bestNpc);
			inst.chainNpcHit = bestNpc->UID;
			if(friendChain) inst.excludeIndex = -1;
			ChainBeamPassThrough(inst, beamFrom, vec2(bestNpc->pos), bestNpc->UID, prevNpc);
		}
		else if(best) {
			inst.dir = glm::length(vec2(best->body->GetWorldPos() - inst.pos)) > 1.0f ? glm::normalize(vec2(best->body->GetWorldPos() - inst.pos)) : inst.dir;
			inst.pos = vec3(vec2(best->body->GetWorldPos()), inst.pos.z);
			if(remote.targetOnly) inst.target = best->Main().UID;
			RemoteHit(inst, *best);
			inst.excludeIndex = (i32)best->index;
			inst.chainHitMask |= 1u << best->index;
			if(friendChain) inst.chainNpcHit = ActorUID::INVALID;
			ChainBeamPassThrough(inst, beamFrom, vec2(best->body->GetWorldPos()), ActorUID::INVALID, prevNpc);
		}
		else {
			inst.chainEnded = true;
			return true;
		}
	}
	return false;
}

void World::ChainBeamPassThrough(RemoteInst& inst, const vec2& from, const vec2& to, ActorUID skipNpc, ActorUID prevNpc)
{
	if(inst.remote->penetrationCount <= 1) return;
	const f32 halfWidth = inst.remote->boundSize[0] * 0.5f;
	Player& owner = players[inst.ownerIndex];
	foreach(it, players) {
		if(((inst.chainHitMask >> it->index) & 1) || it->team == owner.team || !RemoteCanAffect(inst, *it, false)) continue;
		if(SegmentPointDist2D(from, to, vec2(it->body->GetWorldPos())) > halfWidth + HitRadius(*it)) continue;
		LOG("Chain :: remote %d beam passes through %ls", (i32)inst.remote->ID, it->name.data());
		inst.chainHitMask |= 1u << it->index;
		RemoteHit(inst, *it);
	}
	foreach(npc, actorNpcList) {
		if(npc->UID == skipNpc || npc->UID == prevNpc || npc->hp <= 0 || !NpcHittableBy(*npc, owner)) continue;
		const f32 dist = SegmentPointDist2D(from, to, vec2(npc->pos));
		if(dist > halfWidth + NpcHitRadius(*npc)) continue;
		LOG("Chain :: remote %d beam passes through creature %d", (i32)inst.remote->ID, (i32)npc->docID);
		RemoteDamageNpc(inst, *npc, dist);
	}
}

void World::SweepFireNodePath(RemoteInst& inst, Player& instOwner)
{
	const Remote& remote = *inst.remote;
	const GameXmlContent::FireNodePath& path = *inst.firePath;
	const f32 elapsed = (f32)TimeDiffSec(TimeDiff(inst.spawnTime, localTime));
	const f32 pathEnd = path.step * (f32)(path.points.size() - 1);
	const f32 end = MIN(MIN(elapsed, remote.lifeTime > 0 ? remote.lifeTime : pathEnd), pathEnd);
	f32 t = MAX(inst.firePathDone, MAX(remote.activateDelay, 0.0f));
	if(t > end) return;
	const vec2 fwd = vec2(cosf(inst.firePathYaw), sinf(inst.firePathYaw));
	const vec2 left = vec2(-fwd.y, fwd.x);
	const vec3 base = instOwner.body->GetWorldPos();
	const f32 radius = (f32)remote.boundSize[0];
	for(; t <= end; t += path.step) {
		const f32 f = t / path.step;
		const i32 i = MIN((i32)f, (i32)path.points.size() - 2);
		const vec3 p = glm::mix(path.points[i], path.points[i + 1], MIN(f - (f32)i, 1.0f));
		const vec2 center = vec2(base) + fwd * p.x + left * p.y;
		inst.pos = vec3(center, base.z + p.z);
		foreach(it, players) {
			if(it->dead || !it->body || !RemoteCanAffect(inst, *it, false)) continue;
			if(glm::length(vec2(it->body->GetWorldPos()) - center) <= radius + HitRadius(*it)) RemoteHit(inst, *it);
		}
		{
			foreach(npc, actorNpcList) {
				if(!NpcHittableBy(*npc, instOwner)) continue;
				const f32 dist = glm::length(vec2(npc->pos) - center);
				if(dist <= radius + NpcHitRadius(*npc)) RemoteDamageNpc(inst, *npc, dist);
			}
		}
	}
	inst.firePathDone = t;
}

void World::ApplyAttachedRemote(RemoteInst& inst, Player& instOwner)
{
	const Remote& remote = *inst.remote;
	if(inst.firePath) {
		SweepFireNodePath(inst, instOwner);
		return;
	}
	if(remote.behaviorType == Remote::BehaviorType::ORBIT) {
		const f32 radius = remote.boundSize[0] * 0.5f + remote.orbitRadius;
		foreach(it, players) {
			if(!RemoteCanAffect(inst, *it, false)) continue;
			if(glm::length(vec2(it->body->GetWorldPos()) - vec2(instOwner.body->GetWorldPos())) <= radius + HitRadius(*it)) RemoteHit(inst, *it);
		}
		{
			foreach(npc, actorNpcList) {
				if(!NpcHittableBy(*npc, instOwner)) continue;
				const f32 dist = glm::length(vec2(npc->pos) - vec2(instOwner.body->GetWorldPos()));
				if(dist <= radius + NpcHitRadius(*npc)) RemoteDamageNpc(inst, *npc, dist);
			}
		}
	}
	else if(remote.boundType != Remote::BoundType::E_BOUND_NONE && remote.damageGroup != Remote::DamageGroup::eNONE) {
		RemoteApplyArea(inst);
	}
}

static const f32 HYP_NPC_REMOTE_AIM_HEIGHT = 100.0f;

bool World::SteerRemote3D(RemoteInst& inst, const vec3& aim, f32 groundZ, f32 hitReach, f32 delta, bool* reached)
{
	const Remote& remote = *inst.remote;
	const f32 flat = sqrtf(MAX(1.0f - inst.dirZ * inst.dirZ, 0.0f));
	vec3 d = vec3(inst.dir * flat, inst.dirZ);
	const vec3 rel = aim - inst.pos;
	const f32 len = glm::length(rel);
	const f32 step = inst.speed * delta;
	*reached = len <= step + hitReach;
	if(*reached) return true;
	const vec3 want = rel / len;
	const f32 angle = acosf(MAX(MIN(glm::dot(d, want), 1.0f), -1.0f));
	const f32 turn = remote.angularVel * delta;
	if(angle > 0.0001f) {
		const f32 t = MIN(turn / angle, 1.0f);
		const f32 s = sinf(angle);
		d = s > 0.0001f ? (d * sinf((1.0f - t) * angle) + want * sinf(t * angle)) / s : want;
		d = glm::normalize(d);
	}
	const vec2 h = vec2(d);
	if(glm::length(h) > 0.001f) inst.dir = glm::normalize(h);
	inst.dirZ = d.z;
	f32 moved = step;
	if(inst.range > 0) moved = MAX(MIN(moved, inst.range - inst.travelled), 0.0f);
	inst.pos += d * moved;
	inst.travelled += moved;
	if(inst.pos.z <= groundZ) {
		inst.pos.z = groundZ;
		return true;
	}
	return inst.range > 0 && inst.travelled >= inst.range - 0.01f;
}

bool World::UpdateTargetRemote(RemoteInst& inst, f32 delta, f32 elapsed)
{
	const Remote& remote = *inst.remote;
	const bool accelerating = remote.initSpeed > 0 && remote.angularVel > 0;
	if(accelerating) {
		const f32 grown = remote.initSpeed + remote.forceScale * (f32)TimeDiffSec(TimeDiff(inst.spawnTime, localTime));
		inst.speed = remote.maxSpeed > 0 ? MIN(grown, remote.maxSpeed) : grown;
	}
	if(remote.behaviorType == Remote::BehaviorType::TARGET && remote.attachTargetObject && inst.target != ActorUID::INVALID) return false;
	if(inst.target == ActorUID::INVALID && remote.behaviorType == Remote::BehaviorType::HOMING && remote.forceDelay > 0 && TimeDiffSec(TimeDiff(inst.spawnTime, localTime)) < remote.forceDelay) {
		const f32 forced = (inst.speed > 0 ? inst.speed : 0.0f) * delta;
		inst.pos += vec3(inst.dir * forced, 0);
		inst.travelled += forced;
		return false;
	}
	if(inst.target == ActorUID::INVALID && remote.behaviorType == Remote::BehaviorType::HOMING && remote.homingLength > 0) {
		f32 bestDist = remote.homingLength;
		const f32 halfCos = cosf(remote.homingAngle * 0.5f * PI / 180.0f);
		foreach(it, players) {
			if(!RemoteCanAffect(inst, *it, false) || it->team == players[inst.ownerIndex].team) continue;
			const vec2 rel = vec2(it->body->GetWorldPos() - inst.pos);
			const f32 d = glm::length(rel);
			if(d > bestDist) continue;
			if(d > 1.0f && remote.homingAngle > 0 && remote.homingAngle < 360 && glm::dot(rel / d, inst.dir) < halfCos) continue;
			bestDist = d;
			inst.target = it->Main().UID;
		}
		foreach(npc, actorNpcList) {
			if(npc->hp <= 0 || !NpcHittableBy(*npc, players[inst.ownerIndex])) continue;
			const vec2 rel = vec2(npc->pos - inst.pos);
			const f32 d = glm::length(rel);
			if(d > bestDist) continue;
			if(d > 1.0f && remote.homingAngle > 0 && remote.homingAngle < 360 && glm::dot(rel / d, inst.dir) < halfCos) continue;
			bestDist = d;
			inst.target = npc->UID;
		}
		if(inst.target != ActorUID::INVALID) LOG("Remote :: %d homing acquired a target at %.0f", (i32)remote.ID, bestDist);
	}
	const bool steered = remote.behaviorType == Remote::BehaviorType::HOMING && remote.angularVel > 0;
	const ActorMaster* targetMaster = inst.target != ActorUID::INVALID ? FindMasterActor(inst.target) : nullptr;
	if(!targetMaster) {
		ActorNpc* targetNpc = inst.target != ActorUID::INVALID ? FindNpcActor(inst.target) : nullptr;
		if(!targetNpc || targetNpc->hp <= 0) {
			if(!steered) {
				if(remote.behaviorType != Remote::BehaviorType::HOMING || inst.speed <= 0) return true;
				inst.target = ActorUID::INVALID;
				return UpdateLinearRemote(inst, delta, elapsed);
			}
			inst.target = ActorUID::INVALID;
			return AdvanceSteeredRemote(inst, delta);
		}
		const vec2 rel = vec2(targetNpc->pos - inst.pos);
		const f32 dist = glm::length(rel);
		const f32 step = (inst.speed > 0 ? inst.speed : 3000.0f) * delta;
		if(dist <= step + remote.boundSize[0] * 0.5f + NpcHitRadius(*targetNpc)) {
			inst.pos = vec3(vec2(targetNpc->pos), inst.pos.z);
			RemoteDamageNpc(inst, *targetNpc, 0.0f);
			return true;
		}
		if(steered && accelerating) {
			bool reached = false;
			const bool done = SteerRemote3D(inst, targetNpc->pos + vec3(0, 0, HYP_NPC_REMOTE_AIM_HEIGHT), targetNpc->pos.z, remote.boundSize[0] * 0.5f + NpcHitRadius(*targetNpc), delta, &reached);
			if(reached) RemoteDamageNpc(inst, *targetNpc, 0.0f);
			return done;
		}
		if(steered) {
			inst.dir = SteerToward(inst.dir, rel, remote.angularVel * delta);
			return AdvanceSteeredRemote(inst, delta);
		}
		inst.dir = rel / dist;
		inst.pos += vec3(inst.dir * step, 0);
		return false;
	}
	if(targetMaster->parent->dead) {
		if(!steered) {
			if(remote.behaviorType != Remote::BehaviorType::HOMING || inst.speed <= 0) return true;
			inst.target = ActorUID::INVALID;
			return UpdateLinearRemote(inst, delta, elapsed);
		}
		inst.target = ActorUID::INVALID;
		return AdvanceSteeredRemote(inst, delta);
	}
	Player& target = *targetMaster->parent;
	const vec3 tpos = target.body->GetWorldPos();
	if(steered && accelerating) {
		bool reached = false;
		const bool done = SteerRemote3D(inst, tpos + vec3(0, 0, BodyMidHeight(target)), tpos.z, remote.boundSize[0] * 0.5f + HitRadius(target), delta, &reached);
		if(reached) RemoteHit(inst, target);
		return done;
	}
	const vec2 rel = vec2(tpos - inst.pos);
	const f32 dist = glm::length(rel);
	const f32 step = (inst.speed > 0 ? inst.speed : 3000.0f) * delta;
	if(dist <= step + remote.boundSize[0] * 0.5f + HitRadius(target)) {
		inst.pos = vec3(vec2(tpos), inst.pos.z);
		RemoteHit(inst, target);
		return true;
	}
	else if(steered) {
		inst.dir = SteerToward(inst.dir, rel, remote.angularVel * delta);
		return AdvanceSteeredRemote(inst, delta);
	}
	else {
		inst.dir = rel / dist;
		inst.pos += vec3(inst.dir * step, 0);
		inst.travelled += step;
		RemoteHitAlongPath(inst);
	}
	return false;
}

bool World::AdvanceSteeredRemote(RemoteInst& inst, f32 delta)
{
	f32 step = (inst.speed > 0 ? inst.speed : 3000.0f) * delta;
	if(inst.range > 0) step = MAX(MIN(step, inst.range - inst.travelled), 0.0f);
	inst.pos += vec3(inst.dir * step, 0);
	inst.travelled += step;
	RemoteHitAlongPath(inst);
	return inst.range > 0 && inst.travelled >= inst.range - 0.01f;
}

bool World::UpdateGraphRemote(RemoteInst& inst, f32 delta)
{
	const Remote& remote = *inst.remote;
	const f32 total = inst.graphArc > 0 ? inst.graphArc : inst.range;
	const f32 step = MIN(inst.speed * delta, total - inst.travelled);
	inst.travelled += step;
	if(remote.bezier && inst.range > 0) {
		const vec2 f = inst.graphDir;
		const vec2 side = -Perp(f);
		const vec2 p0 = vec2(inst.graphStart);
		const vec2 p3 = p0 + f * inst.range;
		vec2 c[2];
		for(int k = 0; k < 2; k++) {
			const f32 a = remote.bezierRotate[k] * PI / 180.0f;
			c[k] = p0 + f * (inst.range * (k + 1) / 3.0f + remote.bezierFront[k]) + side * (remote.bezierUp[k] * sinf(a));
		}
		const f32 u = MIN(inst.travelled / total, 1.0f);
		const f32 v = 1.0f - u;
		const vec2 p = p0 * (v * v * v) + c[0] * (3 * v * v * u) + c[1] * (3 * v * u * u) + p3 * (u * u * u);
		const vec2 tangent = (c[0] - p0) * (3 * v * v) + (c[1] - c[0]) * (6 * v * u) + (p3 - c[1]) * (3 * u * u);
		if(glm::length(tangent) > 0.001f) inst.dir = glm::normalize(tangent);
		inst.pos = vec3(p, inst.pos.z);
	}
	else {
		inst.pos += vec3(inst.dir * step, 0);
	}
	RemoteHitAlongPath(inst);
	const bool arrived = (inst.speed <= 0 && remote.lifeTime <= 0) || inst.travelled >= total - 0.01f;
	if(arrived && inst.hasLandZ) inst.pos.z = inst.landZ;
	return arrived;
}

bool World::UpdateLinearRemote(RemoteInst& inst, f32 delta, f32 elapsed)
{
	const Remote& remote = *inst.remote;
	bool destroyed = false;
	if(inst.speed > 0) {
		f32 step = inst.range > 0 ? MIN(inst.speed * delta, inst.range - inst.travelled) : inst.speed * delta;
		bool hitWall = false;
		const PxRigidActor* wallActor = nullptr;
		ActorDynamic* wallGate = nullptr;
		if(!remote.ignoreGround && !remote.attachFireObject && step > 0) {
			f32 wallDist = 0;
			const vec2 a = vec2(inst.pos);
			if(RaycastShot(vec3(a, inst.wallZ), vec3(a + inst.dir * step, inst.wallZ), (i32)players[inst.ownerIndex].team, &wallDist, &wallActor, &wallGate)) {
				step = wallDist;
				hitWall = true;
			}
		}
		const vec2 from = vec2(inst.pos);
		const vec2 to = from + inst.dir * step;
		const f32 halfWidth = remote.boundSize[0] * 0.5f;
		const f32 boxFront = remote.boundType == Remote::BoundType::E_BOUND_BOX ? MAX(remote.offsetY + RemoteBoundOffset(remote) + remote.boundSize[1] * 0.5f, 0.0f) : 0.0f;
		const Player* stopPos = nullptr;
		if(remote.damageGroup != Remote::DamageGroup::eNONE && remote.damageGroup != Remote::DamageGroup::INVALID) {
			const bool singleActivation = inst.hitsLeft == 1;
			foreach(it, players) {
				if(!RemoteCanAffect(inst, *it, false)) continue;
				const vec3 pos = it->body->GetWorldPos();
				if(fabsf(pos.z - inst.pos.z) > WEAPON_PROJECTILE_HIT_HEIGHT) continue;
				if(!LagSegmentHits(*it, from, to, halfWidth + HitRadius(*it) + PROJECTILE_HIT_TOLERANCE, boxFront + HitRadius(*it))) continue;
				if(!RemoteHit(inst, *it)) continue;
				if(!stopPos) stopPos = &*it;
				inst.penetrationLeft -= PenetrationCost(*it);
				if(inst.stopAtFirstHit && it->team != players[inst.ownerIndex].team) inst.hitsLeft = 1;
				if(inst.penetrationLeft <= 0 || singleActivation) break;
			}
			bool npcStop = false;
			vec2 npcStopPos = vec2(0);
			if(inst.penetrationLeft > 0 && !(singleActivation && stopPos)) {
				Player& owner = players[inst.ownerIndex];
				const vec2 seg = to - from;
				const f32 segLen2 = glm::dot(seg, seg);
				foreach(npc, actorNpcList) {
					if(!NpcHittableBy(*npc, owner)) continue;
					const vec2 np = vec2(npc->pos);
					const f32 tt = segLen2 > 0 ? MIN(MAX(glm::dot(np - from, seg) / segLen2, 0.0f), 1.0f) : 0.0f;
					if(glm::length(np - (from + seg * tt)) > halfWidth + NpcHitRadius(*npc) + PROJECTILE_HIT_TOLERANCE) continue;
					if(!RemoteDamageNpc(inst, *npc, inst.travelled + step * tt)) continue;
					if(!npcStop) { npcStop = true; npcStopPos = np; }
					inst.penetrationLeft -= 1;
					if(inst.stopAtFirstHit) inst.hitsLeft = 1;
					if(inst.penetrationLeft <= 0 || singleActivation) break;
				}
			}
			if(stopPos || npcStop) {
				inst.hitsLeft--;
				if(inst.hitsLeft <= 0 || inst.penetrationLeft <= 0) {
					inst.pos = stopPos ? vec3(vec2(stopPos->body->GetWorldPos()), inst.pos.z) : vec3(npcStopPos, inst.pos.z);
					destroyed = true;
				}
			}
		}
		if(!destroyed) {
			inst.pos += vec3(inst.dir * step, 0);
			inst.travelled += step;
			if(hitWall || (inst.range > 0 && inst.travelled >= inst.range - 0.01f)) destroyed = true;
			if(hitWall) {
				LOG("Remote :: %d stopped by a wall after %.0f", (i32)remote.ID, inst.travelled);
				OnDynamicHit(wallActor, inst.clientRemoteID, ClientHandle::INVALID, inst.casterLocal ? players[inst.ownerIndex].clientHd : ClientHandle::INVALID, inst.pos, wallGate, (i32)inst.ownerIndex, (i32)(inst.damage * remote.attackMultiplier), remote.ID);
			}
		}
	}
	else if(!inst.areaDone) {
		if(remote.activateDelay <= 0 || elapsed >= remote.activateDelay) {
			inst.areaDone = true;
			RemoteApplyArea(inst);
		}
	}
	else if(remote.lifeTime > 0 && inst.hitsLeft > 0 && elapsed < remote.lifeTime) {
		RemoteApplyArea(inst);
	}
	if(inst.hitsLeft <= 0 && remote.lifeTime > 0) {
		destroyed = true;
	}
	return destroyed;
}

void World::DestroyRemote(i32 index)
{
	RemoteInst& inst = remoteInsts[index];
	const Remote& remote = *inst.remote;
	const i32 i = index;
	if(inst.hasLandZ) inst.pos.z = inst.landZ;
	{
		const RemoteIdx next = remote.destroyCreateRemote;
		const RemoteInst copy = inst;
		remoteInsts.erase_unsorted(remoteInsts.begin() + i);
		if(!copy.chainEnded) {
			const i32 childExclude = remote.behaviorType == Remote::BehaviorType::CHAIN ? copy.lastHitIndex : copy.excludeIndex;
			if(next != RemoteIdx::INVALID) {
				const Remote* nextRemote = GetGameXmlContent().FindRemote(next);
				const f32 nextOffset = nextRemote ? nextRemote->offsetY : 0;
				if(copy.sourceUID != ActorUID::INVALID && FindNpcActor(copy.sourceUID)) remoteParentOverride = copy.sourceUID;
				vec3 childPos = copy.pos + vec3(copy.dir * nextOffset, 0);
				vec2 childDir = copy.dir;
				if(copy.measuredNode && copy.measuredNode->hasChild) {
					const vec2 face = vec2(cosf(copy.castYaw), sinf(copy.castYaw));
					childPos = copy.castOrigin + vec3(face * copy.measuredNode->childForward + Perp(face) * copy.measuredNode->childSide, copy.measuredNode->childUp);
					childDir = RotateDir(face, copy.measuredNode->childRot);
				}
				remoteSendParentID = copy.clientRemoteID - (copy.sentByDoc ? 1 : 0);
				remoteSendParentSeedHd = copy.seedOwnerHd;
				RemoteInst* child = SpawnRemote(copy.ownerIndex, CostReplacedRemote(copy.costLevel, next), childPos, childDir, copy.target, copy.skillID, copy.castID, copy.damage, copy.clientRemoteID != 0, nullptr, copy.nodeName, false, false, true);
				if(child && copy.measuredNode && copy.measuredNode->hasChild) child->dirZ = copy.measuredNode->childDirZ;
				remoteParentOverride = ActorUID::INVALID;
				remoteSendParentID = 0;
				remoteSendParentSeedHd = ClientHandle::INVALID;
				if(child) {
					child->excludeIndex = childExclude;
					child->chainHitMask = copy.chainHitMask;
					child->chainNpcHit = copy.chainNpcHit;
					child->casterClass = copy.casterClass;
					child->costLevel = copy.costLevel;
					child->centeredBox = copy.centeredBox;
					child->sourceUID = copy.sourceUID;
				}
			}
			SpawnRemoteRules(*this, copy, remote.onDestroy, remote.creatorOnDestroy, copy.pos, childExclude, next);
		}
	}
}

void World::ExpireHitRecords()
{
	for(auto r = repeatHits.begin(); r != repeatHits.end(); ) {
		if(TimeDiffSec(TimeDiff(r->at, localTime)) > 10.0) r = repeatHits.erase_unsorted(r);
		else ++r;
	}
	for(auto h = castHits.begin(); h != castHits.end(); ) {
		if(localTime >= h->until) h = castHits.erase_unsorted(h);
		else ++h;
	}
}

void World::BindClientRemote(Player& player, RemoteIdx doc, u32 seed)
{
	RemoteInst* best = nullptr;
	foreach(inst, remoteInsts) {
		if(inst->ownerIndex != player.index || !inst->casterLocal || inst->clientRemoteID != 0 || !inst->remote || inst->remote->ID != doc) continue;
		if(!best || inst->instID > best->instID) best = &*inst;
	}
	if(best) {
		best->clientRemoteID = seed + 1;
		best->seedOwnerHd = player.clientHd;
		return;
	}
	for(i32 k = 0; k < (i32)clientRemoteSeeds.size(); ) {
		if(localTime > TimeAdd(clientRemoteSeeds[k].at, TimeMsToTime(CLIENT_SEED_BIND_MS))) clientRemoteSeeds.erase(clientRemoteSeeds.begin() + k);
		else k++;
	}
	if(!clientRemoteSeeds.full()) clientRemoteSeeds.push_back({ player.index, doc, seed, localTime });
}
