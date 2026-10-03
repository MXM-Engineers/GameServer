#include "world.h"
#include <mxm/game_content.h>
#include <EASTL/sort.h>
#include <eathread/eathread_atomic.h>
#include "velqor_trace.h"
#include <cmath>

static EA::Thread::AtomicUint32 g_velqorUpdateSeq = 0;

static JumpDirection GetJumpDirection(const vec2& direction, f32 facing)
{
	if(direction.x == 0.0f && direction.y == 0.0f) return JumpDirection::Stand;
	const f32 angle = std::remainder(std::atan2(direction.y, direction.x) - facing, 2.0f * (f32)PI);
	if(std::fabs(angle) <= (f32)PI * 0.25f) return JumpDirection::Front;
	if(std::fabs(angle) > (f32)PI * 0.75f) return JumpDirection::Back;
	return angle > 0.0f ? JumpDirection::Left : JumpDirection::Right;
}

static HorizontalMoveType JumpDirToMoveType(JumpDirection dir)
{
	switch(dir) {
	case JumpDirection::Stand: return HorizontalMoveType::Stand;
	case JumpDirection::Front: return HorizontalMoveType::Front;
	case JumpDirection::Left: return HorizontalMoveType::Left;
	case JumpDirection::Right: return HorizontalMoveType::Right;
	case JumpDirection::Back: return HorizontalMoveType::Back;
	default: return HorizontalMoveType::Stand;
	}
}

static u32 HorizontalRandomRoll()
{
	return (u32)RandInt(0, 100);
}

static const HorizontalMotionVariant* PickRandomVariant(const HorizontalMotion& motion, HorizontalMoveType want, u32 roll)
{
	const HorizontalMotionVariant* last = nullptr;
	for(u8 i = 0; i < motion.variantCount; ++i) {
		const HorizontalMotionVariant& v = motion.variants[i];
		if(!v.hasRandom) continue;
		if(v.moveType != HorizontalMoveType::Any && v.moveType != want) continue;
		last = &v;
		if(roll <= (u32)v.randomCaseValue) return &v;
	}
	return last;
}

static const HorizontalMotionVariant* SelectHorizontalVariant(const HorizontalMotion& motion, HorizontalMoveType want)
{
	ASSERT(motion.variantCount > 0);
	const HorizontalMotionVariant* exactNonRandom = nullptr;
	const HorizontalMotionVariant* anyNonRandom = nullptr;
	bool hasRandom = false;
	for(u8 i = 0; i < motion.variantCount; ++i) {
		const HorizontalMotionVariant& v = motion.variants[i];
		if(v.hasRandom) {
			hasRandom = true;
			continue;
		}
		if(v.moveType == HorizontalMoveType::Any) {
			if(!anyNonRandom) anyNonRandom = &v;
			continue;
		}
		if(v.moveType == want && !exactNonRandom) exactNonRandom = &v;
	}
	if(exactNonRandom) return exactNonRandom;
	if(anyNonRandom) return anyNonRandom;
	if(hasRandom) return PickRandomVariant(motion, want, HorizontalRandomRoll());
	return nullptr;
}

static vec2 SkillMoveInputDir(const World::Player& player)
{
	if(player.input.moveDir.x != 0.f || player.input.moveDir.y != 0.f) {
		return player.input.moveDir;
	}
	if(player.clientHd == ClientHandle::INVALID) {
		const vec2 delta = vec2(player.input.moveTo - player.body->GetWorldPos());
		const f32 deltaLen = glm::length(delta);
		if(deltaLen > 1.0f && player.input.speed > 0.f) {
			return NormalizeSafe(delta);
		}
	}
	return vec2(0);
}

static void ApplyIgnoreUnitCollision(World::Player& player, bool ignore)
{
	if(player.graphIgnoreCollision == ignore) {
		return;
	}
	player.graphIgnoreCollision = ignore;
	player.body->SetIgnoreUnitCollision(ignore);
}

static void ClearIgnoreUnitCollisionNow(World::Player& player)
{
	player.graphIgnoreClearAfterStep = false;
	ApplyIgnoreUnitCollision(player, false);
}

static void ClearJumpMotion(PhysicsDynamicBody& body)
{
	body.jumpMotion = nullptr;
	body.jumpElapsed = 0.0f;
	body.jumpHeight = 0.0f;
}



static void ApplyHorizontalGraph(World& world, World::SkillProgram& prog, World::Player& player, f32 elapsed)
{
	if(!prog.horizontalVariant || prog.graphDone) {
		return;
	}
	if(elapsed < prog.graphExecuteAt) {
		return;
	}
	if(prog.graphApplyUpdate != world.SimulationUpdate()) {
		prog.graphApplyUpdate = world.SimulationUpdate();
		prog.graphElapsed += (f32)UPDATE_RATE;
	}
	const HorizontalMotionVariant& variant = *prog.horizontalVariant;
	const f32 graphT = prog.graphElapsed;
	const f32 t = eastl::min(graphT, prog.moveDuration);
	const f32 endSample = variant.samples[variant.sampleCount - 1];
	const f32 rawSample = variant.Sample(t);
	const f32 sampled = endSample < 0.f ? eastl::max(rawSample, endSample) : eastl::min(rawSample, endSample);
	const f32 delta = sampled - prog.moveSampled;
	prog.moveSampled = sampled;
	player.body->horizontalGraphDisplacement += prog.moveHorizDir * delta;
	player.body->horizontalGraphActive = true;
	prog.graphDone = graphT >= prog.moveDuration;
	player.movement.forcedMove = true;
}

static void EmitSkillExec(World& world, World::SkillProgram& prog, World::Player& player, bool applyGraph)
{
	vec3 start = prog.moveStartPos;
	vec3 end = player.body->GetWorldPos();
	f32 duration = 0.0f;
	if(applyGraph && prog.horizontalVariant) {
		start = player.body->GetWorldPos();
		const f32 authoredEnd = prog.horizontalVariant->Sample(prog.moveDuration);
		end = start + vec3(prog.moveHorizDir * authoredEnd, 0);
		prog.moveStartPos = start;
		prog.moveEndPos = end;
		duration = prog.moveDuration;
	}
	if(!world.EmitInputNotification(player, prog.inputSequence, 2)) return;
	Replication::SkillExec rpExec;
	rpExec.casterUID = prog.casterUID;
	rpExec.skillID = prog.skillID;
	rpExec.castPos = prog.castPos;
	rpExec.actionID = prog.actionID;
	eastl::copy(prog.targetList.begin(), prog.targetList.end(), eastl::back_inserter(rpExec.targetList));
	rpExec.moveDuration = duration;
	rpExec.startPos = start;
	rpExec.endPos = end;
	rpExec.moveDir = vec2(cosf(prog.castAngle), sinf(prog.castAngle));
	rpExec.rot = { prog.castAngle, 0, prog.castAngle };
	rpExec.speed = player.movement.moveSpeed;
	if(VelqorTrace::Enabled()) {
		char f[1280];
		size_t len = 0;
		VelqorTrace::Catf(f, sizeof(f), len, "\"game_id\":%d,\"player_index\":%u,\"actor_uid\":%u,\"skill_id\":%d,\"action_state\":%d,\"apply_graph\":%d",
			(i32)world.replication->inGameID, player.index, (u32)prog.casterUID, (i32)prog.skillID, (i32)prog.actionID, applyGraph ? 1 : 0);
		VelqorTrace::Catf(f, sizeof(f), len, ",\"cast_pos\":");
		VelqorTrace::CatVec(f, sizeof(f), len, (const float*)&prog.castPos.x, 3);
		VelqorTrace::Catf(f, sizeof(f), len, ",\"exec_start\":");
		VelqorTrace::CatVec(f, sizeof(f), len, (const float*)&start.x, 3);
		VelqorTrace::Catf(f, sizeof(f), len, ",\"exec_end\":");
		VelqorTrace::CatVec(f, sizeof(f), len, (const float*)&end.x, 3);
		VelqorTrace::Catf(f, sizeof(f), len, ",\"move_dir\":");
		VelqorTrace::CatVec(f, sizeof(f), len, (const float*)&rpExec.moveDir.x, 2);
		VelqorTrace::Catf(f, sizeof(f), len, ",\"speed\":");
		VelqorTrace::CatF32(f, sizeof(f), len, rpExec.speed);
		VelqorTrace::Catf(f, sizeof(f), len, ",\"move_duration\":");
		VelqorTrace::CatF32(f, sizeof(f), len, duration);
		VelqorTrace::Catf(f, sizeof(f), len, ",\"sim_t\":%.4f", TimeDurationSec(Time::ZERO, world.localTime));
		VelqorTrace::Emit("skill_exec", f);
	}
	world.replication->FramePushSkillExec(rpExec);
}




void World::Init(Replication* replication_)
{
	replication = replication_;
	replication->ownerWorld = this;
	nextActorUID = 1;
	ASSERT(!history);
	history = new SimulationHistory();

	auto& ctx = PhysContext();
	ctx.CreateScene(&physics);
}

void World::Cleanup()
{
	physics.Destroy();
	if(replication->ownerWorld == this) replication->ownerWorld = nullptr;
	delete history;
	history = nullptr;
}

void World::BeginPlayerTimeline(ClientHandle clientHd, ActorUID actorUID, f64 serverTime)
{
	ActorMaster* actor = FindMasterActor(actorUID);
	if(!actor || actor->parent->clientHd != clientHd) return;
	Player& player = *actor->parent;
	if(player.Main().UID != actorUID) return;
	auto& timeline = history->players[player.index];
	if(timeline.enabled || !std::isfinite(serverTime)) return;
	timeline.clockStartTime[player.mainCharaID] = serverTime;
	timeline.admittedMaster = player.mainCharaID;
	timeline.enabled = true;
	if(VelqorTrace::Enabled()) {
		char fields[384];
		size_t length = 0;
		VelqorTrace::Catf(fields, sizeof(fields), length,
			"\"game_id\":%d,\"player_index\":%u,\"actor_uid\":%u,\"master_slot\":%u,\"server_t\":%.9f",
			(i32)replication->inGameID, player.index, (u32)actorUID, (u32)player.mainCharaID, serverTime);
		VelqorTrace::Emit("input_timeline_begin", fields);
	}
}

bool World::QueuePlayerInput(Player& player, ActorUID actorUID, InputKind kind, const Player::Input& input,
	f64 receivedTime, bool timestamped)
{
	if(!std::isfinite(receivedTime)) return false;
	u8 masterSlot = PLAYER_CHARACTER_COUNT;
	for(u8 i = 0; i < PLAYER_CHARACTER_COUNT; ++i) {
		if(player.characters[i]->UID == actorUID) masterSlot = i;
	}
	if(masterSlot == PLAYER_CHARACTER_COUNT) return false;
	auto& timeline = history->players[player.index];
	if(!timeline.enabled) return false;
	constexpr f64 historySeconds = f64(InputHistoryFrames) / UPDATE_TICK_RATE;
	if(kind == InputKind::Tag && actorUID != player.characters[timeline.admittedMaster]->UID) return false;
	InputClock candidate = timeline.clocks[masterSlot];
	bool mappingRevised = false;
	f64 serverTime = receivedTime;
	if(timestamped) {
		if(input.clientTime < timeline.minimumClientTime[masterSlot] ||
			!candidate.Observe(input.clientTime, receivedTime, historySeconds, &mappingRevised)) return false;
		serverTime = candidate.ToServerTime(input.clientTime);
		if(serverTime > receivedTime || serverTime < timeline.clockStartTime[masterSlot] ||
			(timeline.clockEndTime[masterSlot] >= 0.0 && serverTime > timeline.clockEndTime[masterSlot])) return false;
	}
	const f64 oldestTime = timeline.frames.empty() ? receivedTime - historySeconds :
		timeline.frames[timeline.oldestFrame].previousServerTime;
	if(!timeline.frames.empty() && serverTime <= oldestTime) return false;
	u32 retainedEvents = 0;
	for(const auto& existing: timeline.events) {
		if(existing.serverTime <= oldestTime && existing.keepUntil < receivedTime) continue;
		++retainedEvents;
	}
	if(retainedEvents == timeline.events.max_size()) return false;
	const GameXmlContent::Action* castAction = nullptr;
	if(kind == InputKind::Cast) {
		const auto& content = GetGameXmlContent();
		const auto found = content.skillMap.find(input.cast.skillID);
		if(found == content.skillMap.end()) return false;
		const auto actions = content.actionListMap.find(player.characters[masterSlot]->classType);
		if(actions == content.actionListMap.end()) return false;
		for(const auto& action: actions->second) {
			if(action.ID == found->second.action) {
				castAction = &action;
				break;
			}
		}
		if(!castAction) return false;
	}
	for(auto it = timeline.events.begin(); it != timeline.events.end();) {
		if(it->serverTime <= oldestTime && it->keepUntil < receivedTime) it = timeline.events.erase(it);
		else ++it;
	}
	if(timestamped) {
		timeline.clocks[masterSlot] = candidate;
		timeline.lastClientTime[masterSlot] = eastl::max(timeline.lastClientTime[masterSlot], input.clientTime);
		if(mappingRevised) {
			for(auto& existing: timeline.events) {
				if(!existing.timestamped || existing.masterSlot != masterSlot ||
					existing.clockGeneration != timeline.clockGeneration[masterSlot]) continue;
				const f64 revisedTime = candidate.ToServerTime(existing.clientTime);
				if(existing.serverTime <= oldestTime || revisedTime <= oldestTime ||
					revisedTime < timeline.clockStartTime[masterSlot]) continue;
				existing.serverTime = revisedTime;
				if(existing.serverTime <= timeline.lastStepTime) {
					timeline.replayFrom = timeline.replayFrom < 0.0 ? existing.serverTime :
						eastl::min(timeline.replayFrom, existing.serverTime);
				}
			}
		}
	}
	InputEvent event;
	event.sequence = timeline.nextSequence++;
	event.kind = kind;
	event.actorUID = actorUID;
	event.data = input;
	event.receivedTime = receivedTime;
	event.serverTime = serverTime;
	event.clientTime = input.clientTime;
	event.timestamped = timestamped;
	event.masterSlot = masterSlot;
	event.clockGeneration = timeline.clockGeneration[masterSlot];
	event.keepUntil = serverTime + historySeconds;
	if(castAction) {
		event.keepUntil += castAction->seqLength;
		for(const auto& command: castAction->commands) {
			event.keepUntil = eastl::max(event.keepUntil, serverTime + command.completeAt + historySeconds);
			if(command.type == ActionCommand::Type::GRAPH_MOVE_HORZ) {
				for(u8 i = 0; i < castAction->horizontalMotion.variantCount; ++i) {
					event.keepUntil = eastl::max(event.keepUntil, serverTime + command.executeAt +
						castAction->horizontalMotion.variants[i].duration + historySeconds);
				}
			}
		}
	}
	if(kind == InputKind::Tag) {
		const u8 previous = timeline.admittedMaster;
		timeline.clockEndTime[previous] = receivedTime;
		timeline.admittedMaster ^= 1;
		const u8 next = timeline.admittedMaster;
		timeline.minimumClientTime[next] = timeline.lastClientTime[next];
		timeline.clocks[next].Reset();
		++timeline.clockGeneration[next];
		timeline.clockStartTime[next] = receivedTime;
		timeline.clockEndTime[next] = -1.0;
	}
	timeline.events.push_back(event);
	eastl::sort(timeline.events.begin(), timeline.events.end(), [](const InputEvent& a, const InputEvent& b) {
		return a.serverTime < b.serverTime || (a.serverTime == b.serverTime && a.sequence < b.sequence);
	});
	if(serverTime <= timeline.lastStepTime) {
		timeline.replayFrom = timeline.replayFrom < 0.0 ? serverTime : eastl::min(timeline.replayFrom, serverTime);
	}
	return true;
}

bool World::EmitInputNotification(Player& player, u32 sequence, u32 notification, i32 commandIndex)
{
	if(sequence == 0) return !replayingInputs;
	auto& timeline = history->players[player.index];
	for(auto& event: timeline.events) {
		if(event.sequence != sequence) continue;
		u32& flags = commandIndex < 0 ? event.notifications :
			(notification == 0 ? event.warpNotifications : event.commandNotifications);
		ASSERT(commandIndex < 32);
		const u32 bit = commandIndex < 0 ? notification : 1u << commandIndex;
		if(flags & bit) return false;
		flags |= bit;
		return true;
	}
	return false;
}

void World::CaptureSceneFrame(SceneFrame& frame)
{
	frame.update = updateIndex;
	frame.actors.clear();
	for(const auto& actor: actorDynamicList) {
		for(NxActor* collision: actor.collisionActors) {
			ASSERT(frame.actors.size() < frame.actors.max_size());
			NxActorDesc desc;
			collision->saveToDesc(desc);
			frame.actors.push_back({ collision, desc.flags });
		}
	}
}

void World::RestoreSceneFrame(const SceneFrame& frame)
{
	for(const auto& actor: actorDynamicList) {
		for(NxActor* collision: actor.collisionActors) {
			const SceneActorState* state = nullptr;
			for(const auto& saved: frame.actors) {
				if(saved.actor == collision) {
					state = &saved;
					break;
				}
			}
			if(state && !(state->flags & NX_AF_DISABLE_COLLISION)) collision->clearActorFlag(NX_AF_DISABLE_COLLISION);
			else collision->raiseActorFlag(NX_AF_DISABLE_COLLISION);
		}
	}
}

void World::SavePlayerFrame(Player& player, PlayerFrame& frame, u32 update, Time frameTime, f64 serverTime)
{
	frame.update = update;
	frame.localTime = frameTime;
	frame.serverTime = serverTime;
	frame.previousServerTime = history->players[player.index].lastStepTime;
	frame.body = physics.CaptureBodyState(*player.body);
	frame.input = player.input;
	frame.movement = player.movement;
	frame.mainCharaID = player.mainCharaID;
	for(u8 i = 0; i < PLAYER_CHARACTER_COUNT; ++i) frame.actions[i] = player.characters[i]->actionState;
	frame.graphIgnoreCollision = player.graphIgnoreCollision;
	frame.graphIgnoreClearAfterStep = player.graphIgnoreClearAfterStep;
	frame.programs.clear();
	for(const auto& program: skillProgramList) {
		const ActorMaster* caster = FindMasterActor(program.casterUID);
		if(caster && caster->parent == &player && !program.IsDoneExecuting()) {
			ASSERT(frame.programs.size() < frame.programs.max_size());
			frame.programs.push_back(program);
		}
	}
}

void World::RestorePlayerFrame(Player& player, const PlayerFrame& frame)
{
	physics.RestoreBodyState(*player.body, frame.body);
	player.input = frame.input;
	player.movement = frame.movement;
	player.mainCharaID = frame.mainCharaID;
	for(u8 i = 0; i < PLAYER_CHARACTER_COUNT; ++i) player.characters[i]->actionState = frame.actions[i];
	player.graphIgnoreCollision = frame.graphIgnoreCollision;
	player.graphIgnoreClearAfterStep = frame.graphIgnoreClearAfterStep;
	for(auto it = skillProgramList.begin(); it != skillProgramList.end();) {
		const ActorMaster* caster = FindMasterActor(it->casterUID);
		if(caster && caster->parent == &player) it = skillProgramList.erase_unsorted(it);
		else ++it;
	}
	for(const auto& program: frame.programs) {
		ASSERT(skillProgramList.size() < skillProgramList.max_size());
		skillProgramList.push_back(program);
	}
}

void World::ApplyPlayerInput(Player& player, InputEvent& event)
{
	switch(event.kind) {
		case InputKind::Movement: {
			player.input.moveDir = event.data.moveDir;
			player.input.speed = event.data.speed;
			player.input.rot = event.data.rot;
			player.input.action = event.data.action;
			player.input.clientTime = event.clientTime;
		} break;
		case InputKind::Rotation: player.input.rot = event.data.rot; break;
		case InputKind::Action: {
			player.input.rot.bodyYaw = event.data.rot.bodyYaw;
			player.input.rot.upperYaw = event.data.rot.upperYaw;
			player.input.action = event.data.action;
			player.input.actionParam1 = event.data.actionParam1;
			player.input.actionParam2 = event.data.actionParam2;
		} break;
		case InputKind::Jump: {
			player.input.jump = true;
			player.input.jumpMoveDir = event.data.jumpMoveDir;
			player.input.jumpRotate = event.data.jumpRotate;
			ApplyPlayerControls(player, &event);
		} break;
		case InputKind::Tag: {
			player.input.tag = true;
			ApplyPlayerControls(player, &event);
		} break;
		case InputKind::Cast: {
			player.input.moveDir = event.data.moveDir;
			player.input.speed = event.data.speed;
			player.input.rot = event.data.rot;
			player.input.action = event.data.action;
			player.input.clientTime = event.clientTime;
			simulationEvent = &event;
			const auto& cast = event.data.cast;
			PlayerCastSkill(player, cast.skillID, cast.pos,
				Slice<const ActorUID>(cast.targetList.data(), cast.targetList.size()), cast.clientTime);
			simulationEvent = nullptr;
		} break;
	}
}

void World::ApplyPlayerControls(Player& player, InputEvent* event)
{
	const u32 sequence = event ? event->sequence : 0;
	if(player.input.tag) {
		player.input.tag = false;
		player.mainCharaID ^= 1;
		const auto& character = GetGameXmlContent().GetMaster(player.Main().classType).character;
		physics.ResizeDynamicBody(player.body, f32(character.getActorRadius()), f32(character.getActorHeight()));
		CancelPlayerPrograms(player);
		ClearJumpMotion(*player.body);
		if(EmitInputNotification(player, sequence, 4) && VelqorTrace::Enabled()) {
			char fields[512];
			size_t length = 0;
			VelqorTrace::Catf(fields, sizeof(fields), length,
				"\"game_id\":%d,\"player_index\":%u,\"actor_uid\":%u,\"main_chara_id\":%u,\"class_type\":%d,\"sim_t\":%.4f",
				(i32)replication->inGameID, player.index, (u32)player.Main().UID, (u32)player.mainCharaID,
				(i32)player.Main().classType, TimeDurationSec(Time::ZERO, localTime));
			VelqorTrace::Emit("tag_apply", fields);
		}
	}
	if(player.input.jump) {
		player.input.jump = false;
		const auto& motion = GetGameXmlContent().GetJumpMotion(player.Main().classType,
			GetJumpDirection(player.input.jumpMoveDir, player.input.jumpRotate));
		const bool started = physics.StartJump(player.body, motion);
		if(started) {
			CancelPlayerPrograms(player);
			player.movement.hasJumped = true;
			player.movement.rot.bodyYaw = player.input.jumpRotate;
			player.input.rot.bodyYaw = player.input.jumpRotate;
		}
		if(EmitInputNotification(player, sequence, 8) && VelqorTrace::Enabled()) {
			char fields[768];
			size_t length = 0;
			VelqorTrace::Catf(fields, sizeof(fields), length,
				"\"game_id\":%d,\"player_index\":%u,\"actor_uid\":%u,\"started\":%d,\"has_jumped\":%d,\"rotate\":%.9g,\"move_dir\":[%.9g,%.9g],\"pos\":[%.9g,%.9g,%.9g],\"sim_t\":%.4f",
				(i32)replication->inGameID, player.index, (u32)player.Main().UID, started ? 1 : 0,
				player.movement.hasJumped ? 1 : 0, player.input.jumpRotate, player.input.jumpMoveDir.x,
				player.input.jumpMoveDir.y, player.body->GetWorldPos().x, player.body->GetWorldPos().y,
				player.body->GetWorldPos().z, TimeDurationSec(Time::ZERO, localTime));
			VelqorTrace::Emit("jump_apply", fields);
		}
	}
}

void World::SimulatePlayer(Player& player, u32 update, Time frameTime, f64 serverTime)
{
	simulationPlayer = &player;
	simulationUpdate = update;
	localTime = frameTime;
	physics.localTime = frameTime;
	player.movement.hasJumped = false;
	player.movement.forcedMove = false;
	auto& timeline = history->players[player.index];
	for(auto& event: timeline.events) {
		if(event.serverTime <= timeline.lastStepTime || event.serverTime > serverTime) continue;
		if(player.Main().UID != event.actorUID) continue;
		ApplyPlayerInput(player, event);
	}
	ApplyPlayerControls(player, nullptr);
	player.movement.rot = player.input.rot;
	if(player.input.cast.skillID != SkillID::INVALID) {
		const auto& cast = player.input.cast;
		PlayerCastSkill(player, cast.skillID, cast.pos,
			Slice<const ActorUID>(cast.targetList.data(), cast.targetList.size()), cast.clientTime);
		player.input.cast.skillID = SkillID::INVALID;
	}
	for(auto it = skillProgramList.begin(); it != skillProgramList.end();) {
		const ActorMaster* caster = FindMasterActor(it->casterUID);
		if(caster && caster->parent == &player) {
			if(!it->IsDoneExecuting()) {
				if(it->bornUpdate != update) ++it->simTicks;
				ExecuteSkillProgram(*it);
			}
			if(it->IsDoneExecuting()) {
				it = skillProgramList.erase_unsorted(it);
				continue;
			}
		}
		++it;
	}
	if(player.movement.forcedMove) {
		player.movement.moveDir = vec2(0);
		player.movement.moveSpeed = 0.0f;
	}
	else if(player.input.moveDir != vec2(0)) {
		player.movement.moveDir = player.input.moveDir;
		player.movement.moveSpeed = player.input.speed;
	}
	else if(player.clientHd == ClientHandle::INVALID) {
		const vec2 delta = vec2(player.input.moveTo - player.body->GetWorldPos());
		const f32 length = glm::length(delta);
		player.movement.moveDir = length > 1.0f && player.input.speed > 0.0f ? NormalizeSafe(delta) : vec2(0);
		player.movement.moveSpeed = length > 1.0f && player.input.speed > 0.0f ?
			eastl::min(player.input.speed, f32(length / UPDATE_RATE)) : 0.0f;
	}
	else {
		player.movement.moveDir = vec2(0);
		player.movement.moveSpeed = player.input.speed;
	}
	player.body->moveDir = player.movement.moveDir;
	player.body->moveSpeed = player.movement.moveSpeed;
	physics.StepBody(*player.body);
	if(player.movement.forcedMove) player.input.moveTo = player.body->GetWorldPos();
	if(player.graphIgnoreClearAfterStep) {
		player.graphIgnoreClearAfterStep = false;
		ApplyIgnoreUnitCollision(player, false);
	}
	timeline.lastStepTime = serverTime;
	simulationPlayer = nullptr;
}

void World::UpdatePlayerTimeline(Player& player, f64 serverTime)
{
	auto& timeline = history->players[player.index];
	const Time publishedTime = localTime;
	if(player.clientHd == ClientHandle::INVALID) {
		SimulatePlayer(player, updateIndex, publishedTime, serverTime);
		localTime = publishedTime;
		physics.localTime = publishedTime;
		return;
	}
	if(timeline.enabled && timeline.replayFrom >= 0.0 && !timeline.frames.empty()) {
		u32 first = 0;
		const u32 count = u32(timeline.frames.size());
		while(first < count && timeline.frames[(timeline.oldestFrame + first) % count].serverTime < timeline.replayFrom) ++first;
		if(first < count) {
			SceneFrame currentScene;
			CaptureSceneFrame(currentScene);
			const auto& before = timeline.frames[(timeline.oldestFrame + first) % count];
			RestorePlayerFrame(player, before);
			timeline.lastStepTime = before.previousServerTime;
			replayingInputs = true;
			for(u32 i = first; i < count; ++i) {
				auto& frame = timeline.frames[(timeline.oldestFrame + i) % count];
				for(const auto& sceneFrame: history->scenes) {
					if(sceneFrame.update == frame.update) {
						RestoreSceneFrame(sceneFrame);
						break;
					}
				}
				const u32 frameUpdate = frame.update;
				const Time frameTime = frame.localTime;
				const f64 stepTime = frame.serverTime;
				SavePlayerFrame(player, frame, frameUpdate, frameTime, stepTime);
				SimulatePlayer(player, frameUpdate, frameTime, stepTime);
			}
			replayingInputs = false;
			RestoreSceneFrame(currentScene);
			if(VelqorTrace::Enabled()) {
				char fields[384];
				size_t length = 0;
				VelqorTrace::Catf(fields, sizeof(fields), length,
					"\"game_id\":%d,\"player_index\":%u,\"ticks\":%u,\"from_server_t\":%.9f,\"to_server_t\":%.9f",
					(i32)replication->inGameID, player.index, count - first, timeline.replayFrom, timeline.lastStepTime);
				VelqorTrace::Emit("input_catchup", fields);
			}
		}
	}
	timeline.replayFrom = -1.0;
	localTime = publishedTime;
	PlayerFrame* frame = nullptr;
	if(timeline.frames.size() < timeline.frames.max_size()) frame = &timeline.frames.push_back();
	else {
		frame = &timeline.frames[timeline.oldestFrame];
		timeline.oldestFrame = (timeline.oldestFrame + 1) % u32(timeline.frames.size());
	}
	SavePlayerFrame(player, *frame, updateIndex, publishedTime, serverTime);
	SimulatePlayer(player, updateIndex, publishedTime, serverTime);
	localTime = publishedTime;
	physics.localTime = publishedTime;
}

void World::Update(Time localTime_, f64 serverTime)
{
	ProfileFunction();

	localTime = localTime_;
	physics.localTime = localTime;
	updateIndex++;

	const bool velqorTrace = VelqorTrace::Enabled();
	if(velqorTrace) g_velqorUpdateSeq.Increment();

	for(auto& actor: actorDynamicList) {
		if(!actor.collisionProfile) continue;
		const auto& profile = *actor.collisionProfile;
		if(actor.collisionActors.empty()) {
			physics.CreateCollisionAsset(profile.assetName.data(), actor.pos, actor.rot, 1.0f,
				PhysicsCollisionGroup(profile.group), &actor.collisionActors);
		}
		if(actor.collisionAction != actor.action) {
			actor.collisionAction = actor.action;
			actor.collisionActionStart = updateIndex;
			actor.collisionActionDone = false;
		}
		if(actor.collisionActionDone) continue;
		const DynamicCollisionProfile::Action* command = nullptr;
		for(const auto& action: profile.actions) {
			if(action.state == actor.action) {
				command = &action;
				break;
			}
		}
		ASSERT(command);
		const f32 elapsed = f32(updateIndex - actor.collisionActionStart) / f32(UPDATE_TICK_RATE);
		if(elapsed < command->delay) continue;
		for(NxActor* collision: actor.collisionActors) {
			if(command->enabled) collision->clearActorFlag(NX_AF_DISABLE_COLLISION);
			else collision->raiseActorFlag(NX_AF_DISABLE_COLLISION);
		}
		actor.collisionActionDone = true;
	}

	SceneFrame* sceneFrame = nullptr;
	if(history->scenes.size() < history->scenes.max_size()) sceneFrame = &history->scenes.push_back();
	else {
		sceneFrame = &history->scenes[history->oldestScene];
		history->oldestScene = (history->oldestScene + 1) % u32(history->scenes.size());
	}
	CaptureSceneFrame(*sceneFrame);
	for(auto& player: players) UpdatePlayerTimeline(player, serverTime);
	physics.FinishStep();

	if(velqorTrace) {
		const f64 velqorSimTime = TimeDurationSec(Time::ZERO, localTime);
		foreach_const(it, players) {
			const Player& p = *it;
			const PhysicsDynamicBody& body = *p.body;
			const vec3 posServer = body.GetWorldPos();
			const vec3 rot = vec3(p.movement.rot.upperYaw, p.movement.rot.upperPitch, p.movement.rot.bodyYaw);
			char f[1536];
			size_t len = 0;
			VelqorTrace::Catf(f, sizeof(f), len, "\"game_id\":%d,\"player_index\":%u,\"user_id\":%u,\"client_hd\":%u,\"actor_uid\":%u,\"master_slot\":%u",
				(i32)replication->inGameID, p.index, (u32)p.userID, (u32)p.clientHd, (u32)p.Main().UID, (u32)p.mainCharaID);
			VelqorTrace::Catf(f, sizeof(f), len, ",\"pos_server\":");
			VelqorTrace::CatVec(f, sizeof(f), len, (const float*)&posServer.x, 3);
			VelqorTrace::Catf(f, sizeof(f), len, ",\"vel\":");
			VelqorTrace::CatVec(f, sizeof(f), len, (const float*)&body.vel.x, 3);
			VelqorTrace::Catf(f, sizeof(f), len, ",\"grounded\":%d", body.grounded ? 1 : 0);
			VelqorTrace::Catf(f, sizeof(f), len, ",\"move_dir\":");
			VelqorTrace::CatVec(f, sizeof(f), len, (const float*)&p.movement.moveDir.x, 2);
			VelqorTrace::Catf(f, sizeof(f), len, ",\"move_speed\":");
			VelqorTrace::CatF32(f, sizeof(f), len, p.movement.moveSpeed);
			VelqorTrace::Catf(f, sizeof(f), len, ",\"input_speed\":");
			VelqorTrace::CatF32(f, sizeof(f), len, p.input.speed);
			VelqorTrace::Catf(f, sizeof(f), len, ",\"move_to\":");
			VelqorTrace::CatVec(f, sizeof(f), len, (const float*)&p.input.moveTo.x, 3);
			VelqorTrace::Catf(f, sizeof(f), len, ",\"rot\":");
			VelqorTrace::CatVec(f, sizeof(f), len, (const float*)&rot.x, 3);
			VelqorTrace::Catf(f, sizeof(f), len, ",\"main_chara_id\":%u,\"has_jumped\":%d,\"forced_move\":%d,\"sim_t\":%.4f,\"update_seq\":%u,\"controller_id\":%llu",
				(u32)p.mainCharaID, p.movement.hasJumped ? 1 : 0, p.movement.forcedMove ? 1 : 0, velqorSimTime, g_velqorUpdateSeq.GetValue(), (unsigned long long)(size_t)body.collider);
			VelqorTrace::Emit("post_sim", f);
		}
	}

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
		rep.skills = player.skills;
		rep.mainWeapon = player.mainWeapon;
		rep.subWeapon = player.subWeapon;
		rep.mainWeaponGrade = player.mainWeaponGrade;
		rep.subWeaponGrade = player.subWeaponGrade;
		rep.mainMasterGearNo = player.mainMasterGearNo;
		rep.subMasterGearNo = player.subMasterGearNo;
		rep.mainCharacterType = player.mainCharacterType;
		rep.subCharacterType = player.subCharacterType;

		rep.masters = {
			player.characters[0]->UID,
			player.characters[1]->UID
		};
		rep.mainCharaID = player.mainCharaID;
		rep.hasJumped = player.movement.hasJumped;

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
			rch.hp = chara.hp;
			rch.hpMax = chara.hpMax;
			rch.seed = chara.seed;
			rch.spawnAnim = chara.spawnAnim;
			rch.ownerID = chara.ownerID;
			rch.dirToNearPC = chara.dirToNearPC;
			rch.wanderDist = chara.wanderDist;
			rch.tagID = chara.tagID;




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
			rch.hp = chara.hp;
			rch.hpMax = chara.hpMax;
			rch.seed = chara.seed;
			rch.spawnAnim = chara.spawnAnim;
			rch.ownerID = chara.ownerID;
			rch.dirToNearPC = chara.dirToNearPC;
			rch.wanderDist = chara.wanderDist;
			rch.tagID = chara.tagID;




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
		rfl.spawnAnim = actor.spawnAnim;
		rfl.ownerID = actor.ownerID;
		rfl.dirToNearPC = actor.dirToNearPC;
		rfl.wanderDist = actor.wanderDist;
		rfl.tagID = actor.tagID;
		rfl.actionState = actor.actionState;
		rfl.seed = actor.seed;
		rfl.entityType = actor.entityType;


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
		rfl.spawnAnim = actor.spawnAnim;
		rfl.ownerID = actor.ownerID;
		rfl.dirToNearPC = actor.dirToNearPC;
		rfl.wanderDist = actor.wanderDist;
		rfl.tagID = actor.tagID;
		rfl.seed = actor.seed;
		rfl.entityType = actor.entityType;


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
	const auto& character = GetGameXmlContent().GetMaster(player.mainClass).character;
	player.body = physics.CreateDynamicBody((f32)character.getActorRadius(), (f32)character.getActorHeight(), pos);

	// clear input
	player.input.moveTo = pos;
	player.input.moveDir = vec2(0);
	player.input.speed = 0;
	player.input.rot = rot;
	player.input.tag = 0;
	player.input.jump = 0;
	player.input.action = ActionStateID::INVALID;
	player.input.cast.skillID = SkillID::INVALID;

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
	main.hp = 2400;
	main.hpMax = 2400;
	main.seed = (i32)RandUint();




	sub.parent = &player;
	sub.classType = player.subClass;
	sub.skinIndex = player.subSkin;
	sub.hp = 2400;
	sub.hpMax = 2400;
	sub.seed = (i32)RandUint();



	return player;
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
	for(const auto& profile: GetGameXmlContent().dynamicCollisionProfiles) {
		if(profile.docID == docID) {
			actor.collisionProfile = &profile;
			break;
		}
	}

	actorDynamicMap.emplace(actorUID, --actorDynamicList.end());
	return actor;
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

void World::PlayerCastSkill(Player& player, SkillID skillID, const vec3& castPos, Slice<const ActorUID> targets, f32 clientTime)
{
	// TODO: check if can cast

	// access method is kinda convoluted
	const auto& content = GetGameXmlContent();
	const auto& skill = content.skillMap.at(skillID);
	const u32 inputSequence = simulationEvent ? simulationEvent->sequence : 0;
	const bool emitCast = EmitInputNotification(player, inputSequence, 1);
	const ActionStateID actionState = skill.action;

	const f32 angle = player.input.rot.upperYaw;
	const vec2 dir = vec2(cosf(angle), sinf(angle));

	Replication::SkillCast rpCast;
	rpCast.clientHd = player.clientHd;
	rpCast.casterUID = player.Main().UID;
	rpCast.skillID = skillID;
	rpCast.castPos = castPos;
	rpCast.actionID = actionState;

	rpCast.casterPos = player.body->GetWorldPos();
	rpCast.casterMoveDir = dir;
	rpCast.casterRot = { angle, 0, angle };
	rpCast.casterSpeed = player.input.speed; // FIXME: should not come from input
	rpCast.clientTime = clientTime;

	eastl::copy(targets.begin(), targets.end(), eastl::back_inserter(rpCast.targetList));

	if(emitCast && VelqorTrace::Enabled()) {
		uint32_t traceTargets[16];
		const size_t traceTargetCount = targets.size() < 16 ? targets.size() : 16;
		for(size_t i = 0; i < traceTargetCount; ++i) traceTargets[i] = (uint32_t)targets[i];

		char f[1536];
		size_t len = 0;
		VelqorTrace::Catf(f, sizeof(f), len, "\"game_id\":%d,\"player_index\":%u,\"user_id\":%u,\"client_hd\":%u,\"actor_uid\":%u,\"master_slot\":%u",
			(i32)replication->inGameID, player.index, (u32)player.userID, (u32)player.clientHd, (u32)rpCast.casterUID, (u32)player.mainCharaID);
		VelqorTrace::Catf(f, sizeof(f), len, ",\"skill_id\":%d,\"action_state\":%d", (i32)skillID, (i32)actionState);
		VelqorTrace::Catf(f, sizeof(f), len, ",\"cast_pos\":");
		VelqorTrace::CatVec(f, sizeof(f), len, (const float*)&castPos.x, 3);
		VelqorTrace::Catf(f, sizeof(f), len, ",\"caster_pos\":");
		VelqorTrace::CatVec(f, sizeof(f), len, (const float*)&rpCast.casterPos.x, 3);
		VelqorTrace::Catf(f, sizeof(f), len, ",\"caster_dir\":");
		VelqorTrace::CatVec(f, sizeof(f), len, (const float*)&rpCast.casterMoveDir.x, 2);
		VelqorTrace::Catf(f, sizeof(f), len, ",\"caster_rot\":");
		VelqorTrace::CatVec(f, sizeof(f), len, (const float*)&rpCast.casterRot.upperYaw, 3);
		VelqorTrace::Catf(f, sizeof(f), len, ",\"caster_speed\":");
		VelqorTrace::CatF32(f, sizeof(f), len, rpCast.casterSpeed);
		VelqorTrace::Catf(f, sizeof(f), len, ",\"client_t\":");
		VelqorTrace::CatF32(f, sizeof(f), len, clientTime);
		VelqorTrace::Catf(f, sizeof(f), len, ",\"target_count\":%u,\"targets\":", (u32)traceTargetCount);
		VelqorTrace::CatUids(f, sizeof(f), len, traceTargets, traceTargetCount);
		VelqorTrace::Catf(f, sizeof(f), len, ",\"sim_t\":%.4f", TimeDurationSec(Time::ZERO, localTime));
		VelqorTrace::Emit("cast_accept", f);
	}

	if(emitCast) replication->FramePushSkillCast(rpCast);


	if(emitCast) player.Main().actionState = actionState;

	CancelPlayerPrograms(player);
	ClearJumpMotion(*player.body);

	SkillProgram prog;
	prog.skillID = skillID;
	prog.actionID = actionState;
	prog.castPos = castPos;
	prog.castAngle = angle;
	prog.casterUID = player.Main().UID;
	eastl::copy(targets.begin(), targets.end(), eastl::back_inserter(prog.targetList));
	prog.simTicks = 0;
	prog.bornUpdate = SimulationUpdate();
	prog.graphApplyUpdate = SimulationUpdate() - 1;
	prog.inputSequence = inputSequence;
	prog.moveStartPos = player.body->GetWorldPos();
	prog.moveEndPos = prog.moveStartPos;

	const auto& action = content.GetSkillAction(player.Main().classType, actionState);
	prog.completeAt = action.seqLength;
	f32 graphExecuteAt = 0.0f;
	bool hasGraph = false;
	u8 graphIgnoreCollision = 0;
	foreach_const(cmd, action.commands) {
		prog.completeAt = eastl::max(prog.completeAt, cmd->completeAt);
		if(cmd->type == ActionCommand::Type::GRAPH_MOVE_HORZ) {
			hasGraph = true;
			graphExecuteAt = cmd->executeAt;
			if(cmd->graphMoveHorz.ignoreCollision) graphIgnoreCollision = 1;
		}
	}

	if(hasGraph) {
		const auto& motion = action.horizontalMotion;
		const HorizontalMoveType want = JumpDirToMoveType(GetJumpDirection(SkillMoveInputDir(player), player.input.rot.bodyYaw));
		const HorizontalMotionVariant* variant = simulationEvent && simulationEvent->selectedVariant ?
			simulationEvent->selectedVariant : SelectHorizontalVariant(motion, want);
		if(simulationEvent) simulationEvent->selectedVariant = variant;
		ASSERT(variant);
		ASSERT(variant->duration > 0);
		const f32 authoredEnd = variant->Sample(variant->duration);
		const f32 yaw = player.input.rot.bodyYaw - glm::radians(variant->horizonRotate);
		const vec2 slide = vec2(cosf(yaw), sinf(yaw));
		prog.horizontalVariant = variant;
		prog.moveDuration = variant->duration;
		prog.moveHorizDir = slide;
		prog.moveSampled = 0.0f;
		prog.graphExecuteAt = graphExecuteAt;
		prog.moveEndPos = prog.moveStartPos + vec3(slide * authoredEnd, 0);
	}

	if(emitCast && VelqorTrace::Enabled()) {
		char f[1280];
		size_t len = 0;
		VelqorTrace::Catf(f, sizeof(f), len, "\"game_id\":%d,\"player_index\":%u,\"actor_uid\":%u,\"skill_id\":%d,\"action_state\":%d",
			(i32)replication->inGameID, player.index, (u32)prog.casterUID, (i32)prog.skillID, (i32)prog.actionID);
		VelqorTrace::Catf(f, sizeof(f), len, ",\"complete_at\":");
		VelqorTrace::CatF32(f, sizeof(f), len, prog.completeAt);
		VelqorTrace::Catf(f, sizeof(f), len, ",\"has_graph\":%d,\"ignore_collision\":%u", hasGraph ? 1 : 0, (u32)graphIgnoreCollision);
		VelqorTrace::Catf(f, sizeof(f), len, ",\"graph_execute_at\":");
		VelqorTrace::CatF32(f, sizeof(f), len, graphExecuteAt);
		VelqorTrace::Catf(f, sizeof(f), len, ",\"move_duration\":");
		VelqorTrace::CatF32(f, sizeof(f), len, prog.moveDuration);
		VelqorTrace::Catf(f, sizeof(f), len, ",\"move_dir\":");
		VelqorTrace::CatVec(f, sizeof(f), len, (const float*)&prog.moveHorizDir.x, 2);
		VelqorTrace::Catf(f, sizeof(f), len, ",\"move_start\":");
		VelqorTrace::CatVec(f, sizeof(f), len, (const float*)&prog.moveStartPos.x, 3);
		VelqorTrace::Catf(f, sizeof(f), len, ",\"move_end\":");
		VelqorTrace::CatVec(f, sizeof(f), len, (const float*)&prog.moveEndPos.x, 3);
		VelqorTrace::Catf(f, sizeof(f), len, ",\"command_count\":%u,\"sim_t\":%.4f", (u32)action.commands.size(), TimeDurationSec(Time::ZERO, localTime));
		VelqorTrace::Emit("skill_program_start", f);
	}

	if(hasGraph) {
		EmitSkillExec(*this, prog, player, true);
	}

	ExecuteSkillProgram(prog);

	if(!hasGraph) {
		EmitSkillExec(*this, prog, player, false);
	}

	if(!prog.IsDoneExecuting()) {
		skillProgramList.push_back(prog);
	}
}

void World::CancelPlayerPrograms(Player& player)
{
	const bool velqorTrace = VelqorTrace::Enabled();
	bool cancelled = false;
	for(auto it = skillProgramList.begin(); it != skillProgramList.end();) {
		ActorMaster* caster = FindMasterActor(it->casterUID);
		if(caster && caster->parent == &player) {
			if(velqorTrace && EmitInputNotification(player, it->inputSequence, 32)) {
				char f[640];
				size_t len = 0;
				VelqorTrace::Catf(f, sizeof(f), len, "\"game_id\":%d,\"player_index\":%u,\"actor_uid\":%u,\"skill_id\":%d,\"action_state\":%d,\"sim_t\":%.4f",
					(i32)replication->inGameID, player.index, (u32)it->casterUID, (i32)it->skillID, (i32)it->actionID, TimeDurationSec(Time::ZERO, localTime));
				VelqorTrace::Emit("skill_cancel", f);
			}
			it = skillProgramList.erase_unsorted(it);
			cancelled = true;
		}
		else ++it;
	}
	player.movement.forcedMove = false;
	player.body->horizontalGraphDisplacement = vec2(0);
	player.body->horizontalGraphActive = false;
	if(cancelled) {
		ClearIgnoreUnitCollisionNow(player);
	}
}


void World::ExecuteSkillProgram(SkillProgram& prog)
{
	const bool velqorTrace = VelqorTrace::Enabled();
	ActorMaster* caster = FindMasterActor(prog.casterUID);
	if(!caster || caster->parent->Main().UID != prog.casterUID) {
		if(velqorTrace && (caster ? EmitInputNotification(*caster->parent, prog.inputSequence, 16) : !replayingInputs)) {
			char f[768];
			size_t len = 0;
			VelqorTrace::Catf(f, sizeof(f), len, "\"game_id\":%d,\"player_index\":%d,\"actor_uid\":%u,\"skill_id\":%d,\"action_state\":%d,\"reason\":\"invalid_caster\",\"elapsed\":null,\"complete_at\":null,\"cmd_index\":%d,\"command_count\":-1,\"sim_t\":%.4f",
				(i32)replication->inGameID, caster ? (i32)caster->parent->index : -1, (u32)prog.casterUID, (i32)prog.skillID, (i32)prog.actionID, (i32)prog.commandID, TimeDurationSec(Time::ZERO, localTime));
			VelqorTrace::Emit("skill_done", f);
		}
		if(caster) {
			ClearIgnoreUnitCollisionNow(*caster->parent);
		}
		prog.Finish();
		return;
	}
	Player& player = *caster->parent;
	const f32 elapsed = (f32)prog.simTicks / (f32)UPDATE_TICK_RATE;


	PhysicsDynamicBody* body = player.body;
	const auto& action = GetGameXmlContent().GetSkillAction(caster->classType, prog.actionID);
	while(prog.commandID < action.commands.size()) {
		const auto& cmd = action.commands[prog.commandID];
		if(elapsed < cmd.executeAt) {
			break;
		}

		const i32 velqorCmdIndex = (i32)prog.commandID;
		vec3 velqorPosBefore = vec3(0);
		if(velqorTrace) velqorPosBefore = body->GetWorldPos();

		switch(cmd.type) {
			case ActionCommand::Type::MOVE: {
				if(cmd.move.preset == ActionCommand::MovePreset::WARP) {
					const vec2 dir = vec2(cosf(prog.castAngle), sinf(prog.castAngle));
					const vec3 startPos = body->GetWorldPos();
					const vec3 endPos = physics.FindMovePos(body, vec3(dir * (f32)cmd.move.param2, 0), UPDATE_RATE);
					physics.Move(body, endPos - startPos, UPDATE_RATE, false);
					player.input.moveTo = body->GetWorldPos();
					player.movement.forcedMove = true;
					if(player.clientHd != ClientHandle::INVALID &&
						EmitInputNotification(player, prog.inputSequence, 0, prog.commandID)) {
						replication->FrameRequestPositionCorrection(player.clientHd, caster->UID);
						if(velqorTrace) {
							char f[896];
							size_t len = 0;
							VelqorTrace::Catf(f, sizeof(f), len, "\"game_id\":%d,\"player_index\":%u,\"client_hd\":%u,\"actor_uid\":%u,\"skill_id\":%d,\"move_param2\":%d",
								(i32)replication->inGameID, player.index, (u32)player.clientHd, (u32)caster->UID, (i32)prog.skillID, cmd.move.param2);
							VelqorTrace::Catf(f, sizeof(f), len, ",\"pos_before\":");
							VelqorTrace::CatVec(f, sizeof(f), len, (const float*)&startPos.x, 3);
							const vec3 velqorCorrectionPos = body->GetWorldPos();
							VelqorTrace::Catf(f, sizeof(f), len, ",\"pos_after\":");
							VelqorTrace::CatVec(f, sizeof(f), len, (const float*)&velqorCorrectionPos.x, 3);
							VelqorTrace::Catf(f, sizeof(f), len, ",\"sim_t\":%.4f", TimeDurationSec(Time::ZERO, localTime));
							VelqorTrace::Emit("position_correction", f);
						}
					}
				}
			} break;
			case ActionCommand::Type::GRAPH_MOVE_HORZ: {
				if(cmd.graphMoveHorz.ignoreCollision) {
					player.graphIgnoreClearAfterStep = false;
					ApplyIgnoreUnitCollision(player, true);
				}
			} break;
			default:
				break;
		}
		++prog.commandID;

		if(velqorTrace && EmitInputNotification(player, prog.inputSequence, 1, velqorCmdIndex)) {
			const i32 velqorMovePreset = (cmd.type == ActionCommand::Type::MOVE) ? (i32)cmd.move.preset : -1;
			const i32 velqorMoveParam2 = (cmd.type == ActionCommand::Type::MOVE) ? cmd.move.param2 : 0;
			const vec3 velqorPosAfter = body->GetWorldPos();
			char f[1152];
			size_t len = 0;
			VelqorTrace::Catf(f, sizeof(f), len, "\"game_id\":%d,\"player_index\":%u,\"actor_uid\":%u,\"skill_id\":%d,\"action_state\":%d,\"cmd_index\":%d,\"cmd_type\":%d",
				(i32)replication->inGameID, player.index, (u32)prog.casterUID, (i32)prog.skillID, (i32)prog.actionID, velqorCmdIndex, (i32)cmd.type);
			VelqorTrace::Catf(f, sizeof(f), len, ",\"cmd_execute_at\":");
			VelqorTrace::CatF32(f, sizeof(f), len, cmd.executeAt);
			VelqorTrace::Catf(f, sizeof(f), len, ",\"cmd_complete_at\":");
			VelqorTrace::CatF32(f, sizeof(f), len, cmd.completeAt);
			VelqorTrace::Catf(f, sizeof(f), len, ",\"move_preset\":%d,\"move_param2\":%d", velqorMovePreset, velqorMoveParam2);
			VelqorTrace::Catf(f, sizeof(f), len, ",\"pos_before\":");
			VelqorTrace::CatVec(f, sizeof(f), len, (const float*)&velqorPosBefore.x, 3);
			VelqorTrace::Catf(f, sizeof(f), len, ",\"pos_after\":");
			VelqorTrace::CatVec(f, sizeof(f), len, (const float*)&velqorPosAfter.x, 3);
			VelqorTrace::Catf(f, sizeof(f), len, ",\"elapsed\":");
			VelqorTrace::CatF32(f, sizeof(f), len, elapsed);
			VelqorTrace::Catf(f, sizeof(f), len, ",\"sim_t\":%.4f", TimeDurationSec(Time::ZERO, localTime));
			VelqorTrace::Emit("skill_command", f);
		}
	}
	ApplyHorizontalGraph(*this, prog, player, elapsed);


	const bool graphPending = prog.horizontalVariant && !prog.graphDone;
	if(prog.commandID == action.commands.size() && !graphPending && elapsed >= prog.completeAt) {
		caster->actionState = ActionStateID::INVALID;
		if(velqorTrace && EmitInputNotification(player, prog.inputSequence, 16)) {
			char f[896];
			size_t len = 0;
			VelqorTrace::Catf(f, sizeof(f), len, "\"game_id\":%d,\"player_index\":%u,\"actor_uid\":%u,\"skill_id\":%d,\"action_state\":%d,\"reason\":\"complete\"",
				(i32)replication->inGameID, player.index, (u32)prog.casterUID, (i32)prog.skillID, (i32)prog.actionID);
			VelqorTrace::Catf(f, sizeof(f), len, ",\"elapsed\":");
			VelqorTrace::CatF32(f, sizeof(f), len, elapsed);
			VelqorTrace::Catf(f, sizeof(f), len, ",\"complete_at\":");
			VelqorTrace::CatF32(f, sizeof(f), len, prog.completeAt);
			VelqorTrace::Catf(f, sizeof(f), len, ",\"cmd_index\":%d,\"command_count\":%u,\"sim_t\":%.4f", (i32)prog.commandID, (u32)action.commands.size(), TimeDurationSec(Time::ZERO, localTime));
			VelqorTrace::Emit("skill_done", f);
		}
		if(player.graphIgnoreCollision) {
			player.graphIgnoreClearAfterStep = true;
		}
		prog.Finish();
	}
}
