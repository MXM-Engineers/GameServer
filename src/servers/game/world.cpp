#include "world.h"
#include <mxm/game_content.h>
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
	const vec2 delta = vec2(player.input.moveTo - player.body->GetWorldPos());
	const f32 deltaLen = glm::length(delta);
	if(deltaLen > 1.0f && player.input.speed > 0.f) {
		return NormalizeSafe(delta);
	}
	return vec2(0);
}

static void ApplyHorizontalGraph(World& world, World::SkillProgram& prog, World::Player& player, f32 elapsed)
{
	if(!prog.horizontalVariant || prog.graphDone) {
		return;
	}
	if(elapsed < prog.graphExecuteAt) {
		return;
	}
	const HorizontalMotionVariant& variant = *prog.horizontalVariant;
	const f32 graphT = elapsed - prog.graphExecuteAt;
	const f32 t = eastl::min(graphT, prog.moveDuration);
	const f32 sampled = variant.Sample(t);
	const f32 delta = sampled - prog.moveSampled;
	prog.moveSampled = sampled;
	const vec3 displacement = vec3(prog.moveHorizDir * delta, 0);
	if(glm::dot(displacement, displacement) > 0) {
		world.physics.Move(player.body, displacement, UPDATE_RATE);
	}
	prog.graphDone = graphT >= prog.moveDuration;
	player.input.moveTo = player.body->GetWorldPos();
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

	localTime = localTime_;
	physics.localTime = localTime;

	const bool velqorTrace = VelqorTrace::Enabled();
	if(velqorTrace) g_velqorUpdateSeq.Increment();

	foreach(it, players) {
		Player& p = *it;
		p.movement.rot = p.input.rot;
		p.movement.hasJumped = false;
		p.movement.forcedMove = false;

		if(p.input.tag) {
			p.input.tag = 0;
			p.mainCharaID ^= 1;
			const auto& character = GetGameXmlContent().GetMaster(p.Main().classType).character;
			physics.ResizeDynamicBody(p.body, (f32)character.getActorRadius(), (f32)character.getActorHeight());
			CancelPlayerPrograms(p);
			if(velqorTrace) {
				char f[640];
				size_t len = 0;
				VelqorTrace::Catf(f, sizeof(f), len, "\"game_id\":%d,\"player_index\":%u,\"actor_uid\":%u,\"main_chara_id\":%u,\"class_type\":%d",
					(i32)replication->inGameID, p.index, (u32)p.Main().UID, (u32)p.mainCharaID, (i32)p.Main().classType);
				VelqorTrace::Catf(f, sizeof(f), len, ",\"pos\":");
				const vec3 pos = p.body->GetWorldPos();
				VelqorTrace::CatVec(f, sizeof(f), len, (const float*)&pos.x, 3);
				VelqorTrace::Catf(f, sizeof(f), len, ",\"sim_t\":%.4f", TimeDurationSec(Time::ZERO, localTime));
				VelqorTrace::Emit("tag_apply", f);
			}
		}

		if(p.input.cast.skillID != SkillID::INVALID) {
			const auto& cast = p.input.cast;
			PlayerCastSkill(p, cast.skillID, cast.pos, Slice<const ActorUID>(cast.targetList.data(), cast.targetList.size()), cast.clientTime);
			p.input.cast.skillID = SkillID::INVALID;
		}
	}

	for(auto it = skillProgramList.begin(); it != skillProgramList.end(); ) {
		if(!it->IsDoneExecuting()) {
			ExecuteSkillProgram(*it);
		}
		if(it->IsDoneExecuting()) {
			it = skillProgramList.erase_unsorted(it);
		}
		else {
			++it;
		}
	}

	foreach(it, players) {
		Player& p = *it;
		PhysicsDynamicBody& body = *p.body;
		const bool inputBlocked = p.movement.forcedMove;
		const vec2 delta = vec2(p.input.moveTo - body.GetWorldPos());
		const f32 deltaLen = glm::length(delta);
		if(!inputBlocked && deltaLen > 1.0f && p.input.speed > 0.f) {
			p.movement.moveDir = NormalizeSafe(delta);
			p.movement.moveSpeed = eastl::min(p.input.speed, f32(deltaLen / UPDATE_RATE));
		}
		else {
			p.movement.moveDir = vec2(0);
			p.movement.moveSpeed = 0.0f;
		}

		if(p.input.jump) {
			p.input.jump = 0;
			bool velqorJumpStarted = false;
			if(!inputBlocked) {
				const auto& motion = GetGameXmlContent().GetJumpMotion(p.Main().classType,
					GetJumpDirection(p.input.jumpMoveDir, p.input.jumpRotate));
				if(physics.StartJump(&body, motion)) {
					p.movement.hasJumped = true;
					p.movement.rot.bodyYaw = p.input.jumpRotate;
					p.input.rot.bodyYaw = p.input.jumpRotate;
					velqorJumpStarted = true;
				}
			}
			if(velqorTrace) {
				char f[1024];
				size_t len = 0;
				VelqorTrace::Catf(f, sizeof(f), len, "\"game_id\":%d,\"player_index\":%u,\"actor_uid\":%u,\"started\":%d,\"has_jumped\":%d",
					(i32)replication->inGameID, p.index, (u32)p.Main().UID, velqorJumpStarted ? 1 : 0, p.movement.hasJumped ? 1 : 0);
				VelqorTrace::Catf(f, sizeof(f), len, ",\"jump_rotate\":");
				VelqorTrace::CatF32(f, sizeof(f), len, p.input.jumpRotate);
				VelqorTrace::Catf(f, sizeof(f), len, ",\"move_dir\":");
				VelqorTrace::CatVec(f, sizeof(f), len, (const float*)&p.input.jumpMoveDir.x, 2);
				VelqorTrace::Catf(f, sizeof(f), len, ",\"pos\":");
				const vec3 pos = body.GetWorldPos();
				VelqorTrace::CatVec(f, sizeof(f), len, (const float*)&pos.x, 3);
				VelqorTrace::Catf(f, sizeof(f), len, ",\"vel\":");
				VelqorTrace::CatVec(f, sizeof(f), len, (const float*)&body.vel.x, 3);
				VelqorTrace::Catf(f, sizeof(f), len, ",\"grounded\":%d", body.grounded ? 1 : 0);
				VelqorTrace::Catf(f, sizeof(f), len, ",\"sim_t\":%.4f", TimeDurationSec(Time::ZERO, localTime));
				VelqorTrace::Emit("jump_apply", f);
			}
		}

		body.vel.x = p.movement.moveDir.x * p.movement.moveSpeed;
		body.vel.y = p.movement.moveDir.y * p.movement.moveSpeed;
	}

	physics.Step();

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
			VelqorTrace::Catf(f, sizeof(f), len, ",\"main_chara_id\":%u,\"has_jumped\":%d,\"forced_move\":%d,\"sim_t\":%.4f,\"update_seq\":%u",
				(u32)p.mainCharaID, p.movement.hasJumped ? 1 : 0, p.movement.forcedMove ? 1 : 0, velqorSimTime, g_velqorUpdateSeq.GetValue());
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

	if(VelqorTrace::Enabled()) {
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

	replication->FramePushSkillCast(rpCast);


	player.Main().actionState = actionState;

	CancelPlayerPrograms(player);

	SkillProgram prog;
	prog.skillID = skillID;
	prog.actionID = actionState;
	prog.castPos = castPos;
	prog.castAngle = angle;
	prog.casterUID = player.Main().UID;
	eastl::copy(targets.begin(), targets.end(), eastl::back_inserter(prog.targetList));
	prog.startTime = localTime;
	prog.moveStartPos = player.body->GetWorldPos();
	prog.moveEndPos = prog.moveStartPos;

	const auto& action = content.GetSkillAction(player.Main().classType, actionState);
	prog.completeAt = action.seqLength;
	f32 graphExecuteAt = 0.0f;
	bool hasGraph = false;
	foreach_const(cmd, action.commands) {
		prog.completeAt = eastl::max(prog.completeAt, cmd->completeAt);
		if(cmd->type == ActionCommand::Type::GRAPH_MOVE_HORZ) {
			hasGraph = true;
			graphExecuteAt = cmd->executeAt;
		}
	}

	if(hasGraph) {
		const auto& motion = action.horizontalMotion;
		const HorizontalMoveType want = JumpDirToMoveType(GetJumpDirection(SkillMoveInputDir(player), player.input.rot.bodyYaw));
		const HorizontalMotionVariant* variant = SelectHorizontalVariant(motion, want);
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

	if(VelqorTrace::Enabled()) {
		char f[1280];
		size_t len = 0;
		VelqorTrace::Catf(f, sizeof(f), len, "\"game_id\":%d,\"player_index\":%u,\"actor_uid\":%u,\"skill_id\":%d,\"action_state\":%d",
			(i32)replication->inGameID, player.index, (u32)prog.casterUID, (i32)prog.skillID, (i32)prog.actionID);
		VelqorTrace::Catf(f, sizeof(f), len, ",\"complete_at\":");
		VelqorTrace::CatF32(f, sizeof(f), len, prog.completeAt);
		VelqorTrace::Catf(f, sizeof(f), len, ",\"has_graph\":%d", hasGraph ? 1 : 0);
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

	if(hasGraph && graphExecuteAt > 0.0f) {
		EmitSkillExec(*this, prog, player, false);
	}

	else if(hasGraph) {
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
	foreach(it, skillProgramList) {
		ActorMaster* caster = FindMasterActor(it->casterUID);
		if(caster && caster->parent == &player) {
			if(velqorTrace) {
				char f[640];
				size_t len = 0;
				VelqorTrace::Catf(f, sizeof(f), len, "\"game_id\":%d,\"player_index\":%u,\"actor_uid\":%u,\"skill_id\":%d,\"action_state\":%d,\"sim_t\":%.4f",
					(i32)replication->inGameID, player.index, (u32)it->casterUID, (i32)it->skillID, (i32)it->actionID, TimeDurationSec(Time::ZERO, localTime));
				VelqorTrace::Emit("skill_cancel", f);
			}
			it->Finish();
		}
	}
}


void World::ExecuteSkillProgram(SkillProgram& prog)
{
	const bool velqorTrace = VelqorTrace::Enabled();
	ActorMaster* caster = FindMasterActor(prog.casterUID);
	if(!caster || caster->parent->Main().UID != prog.casterUID) {
		if(velqorTrace) {
			char f[768];
			size_t len = 0;
			VelqorTrace::Catf(f, sizeof(f), len, "\"game_id\":%d,\"player_index\":%d,\"actor_uid\":%u,\"skill_id\":%d,\"action_state\":%d,\"reason\":\"invalid_caster\",\"elapsed\":null,\"complete_at\":null,\"cmd_index\":%d,\"command_count\":-1,\"sim_t\":%.4f",
				(i32)replication->inGameID, caster ? (i32)caster->parent->index : -1, (u32)prog.casterUID, (i32)prog.skillID, (i32)prog.actionID, (i32)prog.commandID, TimeDurationSec(Time::ZERO, localTime));
			VelqorTrace::Emit("skill_done", f);
		}
		prog.Finish();
		return;
	}
	Player& player = *caster->parent;
	const f32 elapsed = (f32)TimeDurationSec(prog.startTime, localTime);


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
					physics.Move(body, endPos - startPos, UPDATE_RATE);
					player.input.moveTo = body->GetWorldPos();
					player.movement.forcedMove = true;
					if(player.clientHd != ClientHandle::INVALID) {
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
			default:
				break;
		}
		++prog.commandID;

		if(velqorTrace) {
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


	const bool graphPending = prog.horizontalVariant && !prog.graphDone && elapsed < prog.graphExecuteAt + prog.moveDuration;
	if(prog.commandID == action.commands.size() && !graphPending && elapsed >= prog.completeAt) {
		caster->actionState = ActionStateID::INVALID;
		if(velqorTrace) {
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
		prog.Finish();
	}
}
