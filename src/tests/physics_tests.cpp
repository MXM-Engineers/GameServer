#include <common/base.h>
#include <mxm/game_content.h>
#include <servers/game/physics.h>
#include <servers/game/world.h>
#include <common/packet_validator.h>
#include <PxPhysicsAPI.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <initializer_list>

namespace
{
constexpr float Dt = float(UPDATE_RATE);

bool Check(bool condition, const char* expression, int line)
{
	if(!condition) std::fprintf(stderr, "FAIL line %d: %s\n", line, expression);
	return condition;
}

#define REQUIRE(expression) do { if(!Check(bool(expression), #expression, __LINE__)) return false; } while(false)

bool Near(float a, float b, float tolerance = 0.5f)
{
	return std::isfinite(a) && std::fabs(a - b) <= tolerance;
}

bool Near(const vec3& a, const vec3& b, float tolerance = 0.5f)
{
	return Near(a.x, b.x, tolerance) && Near(a.y, b.y, tolerance) && Near(a.z, b.z, tolerance);
}

struct Scene
{
	PhysicsScene physics;

	Scene() { PhysContext().CreateScene(&physics); }
	~Scene() { physics.Destroy(); }
	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;
};

void Box(PhysicsScene& scene, const PxVec3& halfSize, const PxTransform& pose,
	PhysicsCollisionGroup group = PhysicsCollisionGroup::Static)
{
	auto& context = PhysContext();
	PxShape* shape = context.physics->createShape(PxBoxGeometry(halfSize), *context.matMapSurface);
	ASSERT(shape);
	shape->setQueryFilterData(PxFilterData(1u << static_cast<u32>(group), 0, 0, 0));
	PxRigidStatic* actor = context.physics->createRigidStatic(pose);
	ASSERT(actor);
	const bool attached = actor->attachShape(*shape);
	ASSERT(attached);
	shape->release();
	scene.scene->addActor(*actor);
}

void Floor(PhysicsScene& scene)
{
	Box(scene, PxVec3(5000, 5000, 100), PxTransform(PxVec3(0, 0, -100)));
}

void MeshRamp(PhysicsScene& scene, float angle)
{
	auto& context = PhysContext();
	const float c = std::cos(angle);
	const float s = std::sin(angle);
	const PxVec3 vertices[] = {
		PxVec3(-40 * s, -500, 0),
		PxVec3(2000 * c - 40 * s, -500, 2000 * s),
		PxVec3(2000 * c - 40 * s, 500, 2000 * s),
		PxVec3(-40 * s, 500, 0),
	};
	const PxU32 indices[] = {0, 1, 2, 0, 2, 3};
	PxTriangleMeshDesc description;
	description.points.count = 4;
	description.points.stride = sizeof(PxVec3);
	description.points.data = vertices;
	description.triangles.count = 2;
	description.triangles.stride = sizeof(PxU32) * 3;
	description.triangles.data = indices;
	PxCooking* cooking = PxCreateCooking(PX_PHYSICS_VERSION, *context.foundation,
		PxCookingParams(context.physics->getTolerancesScale()));
	ASSERT(cooking);
	PxTriangleMesh* mesh = cooking->createTriangleMesh(description, context.physics->getPhysicsInsertionCallback());
	cooking->release();
	ASSERT(mesh);
	PxShape* shape = context.physics->createShape(PxTriangleMeshGeometry(mesh), *context.matMapSurface);
	mesh->release();
	ASSERT(shape);
	shape->setQueryFilterData(PxFilterData(1u << static_cast<u32>(PhysicsCollisionGroup::Static), 0, 0, 0));
	PxRigidStatic* actor = context.physics->createRigidStatic(PxTransform(PxIdentity));
	ASSERT(actor);
	const bool attached = actor->attachShape(*shape);
	ASSERT(attached);
	shape->release();
	scene.scene->addActor(*actor);
}

void Settle(PhysicsScene& scene, int ticks = 90)
{
	for(int i = 0; i < ticks; ++i) scene.Step();
}

struct WorldScene
{
	World world;

	WorldScene()
	{
		static Replication replica;
		replica.Init(nullptr);
		world.Init(&replica);
		Floor(world.physics);
	}

	~WorldScene() { world.Cleanup(); }
	WorldScene(const WorldScene&) = delete;
	WorldScene& operator=(const WorldScene&) = delete;

	World::Player& Player()
	{
		World::PlayerDescription description{};
		description.clientHd = ClientHandle::INVALID;
		description.masters = { ClassType(6), ClassType(5) };
		auto& player = world.CreatePlayer(description, vec3(0), RotationHumanoid{});
		Settle(world.physics);
		return player;
	}

	void Step(u32 milliseconds)
	{
		world.replication->frameCur->Clear();
		world.Update(TimeMsToTime(milliseconds));
	}
};

void DeathMatch(PhysicsScene& scene)
{
	scene.CreateStaticCollider("PVP_DeathMatch01_Collision", vec3(0), vec3(0), PhysicsCollisionGroup::Static);
	scene.CreateStaticCollider("PVP_Deathmatch01_GuardrailMob", vec3(0), vec3(0), PhysicsCollisionGroup::FenceAll);
	scene.CreateStaticCollider("PVP_Deathmatch01_Aim", vec3(0), vec3(0), PhysicsCollisionGroup::Aim);
	scene.CreateStaticCollider("PVP_Deathmatch01_sidewall_GuardrailMob", vec3(0), vec3(0), PhysicsCollisionGroup::FenceAll);
}

bool LoadedMapWalk()
{
	Scene scene;
	DeathMatch(scene.physics);
	auto* body = scene.physics.CreateDynamicBody(100, 70, vec3(2451.241943f, 3951.921631f, 1115));
	Settle(scene.physics);
	REQUIRE(body->grounded);
	REQUIRE(Near(body->GetWorldPos(), vec3(2451.241943f, 3951.921631f, 1115)));
	for(int i = 0; i < 120; ++i) {
		scene.physics.Step();
		REQUIRE(Near(body->GetWorldPos().z, 1115));
	}
	body->vel.x = body->vel.y = 447.15f;
	bool falling = false;
	bool landed = false;
	int fallTicks = 0;
	float slideStart = 0;
	for(int i = 0; i < 150; ++i) {
		const vec3 before = body->GetWorldPos();
		scene.physics.Step();
		const vec3 after = body->GetWorldPos();
		const float travel = glm::length(vec2(after.x - before.x, after.y - before.y));
		REQUIRE(travel <= glm::length(vec2(body->vel.x, body->vel.y)) * Dt + 1.0f);
		REQUIRE(after.z <= 1115.5f && after.z >= 549.5f);
		REQUIRE(after.z <= before.z + 0.5f);
		if(after.z < 1114) falling = true;
		if(falling && !landed) {
			++fallTicks;
			const float maximumDrop = -GetGameXmlContent().creatureGravity * Dt * Dt * float(fallTicks + 1);
			REQUIRE(before.z - after.z <= maximumDrop + 1.0f);
			if(body->grounded) {
				landed = true;
				REQUIRE(Near(after.z, 550.602844f));
			}
		}
		if(after.y > 4100 && slideStart == 0) slideStart = after.x;
	}
	const vec3 end = body->GetWorldPos();
	REQUIRE(falling && landed && body->grounded);
	REQUIRE(Near(end.z, 550.602844f));
	REQUIRE(Near(end.y, 4101.5f, 1.0f));
	REQUIRE(slideStart != 0 && end.x - slideStart > 400);
	REQUIRE(end.x > 3300);
	std::printf("DeathMatch diagonal end %.3f %.3f %.3f, fall ticks %d\n", end.x, end.y, end.z, fallTicks);
	return true;
}

bool FreeFall()
{
	Scene scene;
	Floor(scene.physics);
	auto* body = scene.physics.CreateDynamicBody(100, 70, vec3(0, 0, 600));
	const float gravity = GetGameXmlContent().creatureGravity;
	float velocity = 0;
	float expected = 600;
	bool landed = false;
	for(int i = 0; i < 90; ++i) {
		const float before = body->GetWorldPos().z;
		velocity += gravity * Dt;
		expected += velocity * Dt;
		scene.physics.Step();
		const float after = body->GetWorldPos().z;
		REQUIRE(after <= before + 0.5f && after >= -0.5f);
		if(expected > 1) {
			REQUIRE(!body->grounded);
			REQUIRE(Near(after, expected));
		} else {
			REQUIRE(body->grounded && Near(after, 0));
			REQUIRE(Near(body->vel.z, 0, 0.001f));
			landed = true;
		}
	}
	REQUIRE(landed);
	return true;
}

bool FloatPositionRequests()
{
	Scene scene;
	auto* body = scene.physics.CreateDynamicBody(100, 70, vec3(1048576, 0, 100000));
	body->vel.x = 420.85f;
	for(int i = 0; i < 30; ++i) scene.physics.Step();
	REQUIRE(Near(body->GetWorldPos().x, 1048786.f, 0.005f));
	body->vel.x = 0.6f;
	for(int i = 0; i < 30; ++i) scene.physics.Step();
	REQUIRE(Near(body->GetWorldPos().x, 1048786.f, 0.005f));
	return true;
}

bool Steps()
{
	for(float height: {50.0f, 120.0f}) {
		Scene scene;
		Floor(scene.physics);
		Box(scene.physics, PxVec3(1000, 500, height * 0.5f), PxTransform(PxVec3(1000, 0, height * 0.5f)));
		auto* body = scene.physics.CreateDynamicBody(100, 70, vec3(-300, 0, 0));
		Settle(scene.physics);
		body->vel.x = 480;
		for(int i = 0; i < 100; ++i) {
			const vec3 before = body->GetWorldPos();
			scene.physics.Step();
			const vec3 after = body->GetWorldPos();
			REQUIRE(after.x - before.x <= 8.5f && after.x >= before.x - 0.5f);
			REQUIRE(after.z <= height + 0.5f);
		}
		const vec3 end = body->GetWorldPos();
		std::printf("Step %.0f end %.3f %.3f\n", height, end.x, end.z);
		if(height == 50) {
			REQUIRE(end.x > 400 && Near(end.z, 50));
		} else {
			REQUIRE(end.x < -90 && end.x > -120 && Near(end.z, 0));
		}
		REQUIRE(body->grounded);
	}
	return true;
}

bool SegmentedTraversal()
{
	for(bool segmented: {false, true}) {
		Scene scene;
		Floor(scene.physics);
		for(int i = 0; i < 3; ++i) {
			const float height = float((i + 1) * 50);
			Box(scene.physics, PxVec3(150, 500, height * 0.5f),
				PxTransform(PxVec3(float(150 + i * 300), 0, height * 0.5f)));
		}
		auto* body = scene.physics.CreateDynamicBody(20, 80, vec3(-100, 0, 0));
		Settle(scene.physics);
		const vec3 before = body->GetWorldPos();
		scene.physics.Move(body, vec3(0), Dt, segmented);
		REQUIRE(body->grounded && Near(body->GetWorldPos(), before, 0.005f));
		const vec3 end = scene.physics.Move(body, vec3(1000, 0, -0.5833333f), Dt, segmented);
		std::printf("Segmented %d end %.3f %.3f\n", segmented ? 1 : 0, end.x, end.z);
		if(segmented) {
			REQUIRE(Near(end, vec3(900, 0, 150)));
		}
		else {
			REQUIRE(end.x < 300 && end.z < 100);
		}
		scene.physics.Step();
		REQUIRE(body->grounded && Near(body->GetWorldPos(), end, 0.005f));
	}
	return true;
}

bool Slopes()
{
	for(float degrees: {60.0f, 70.0f}) {
		Scene scene;
		Floor(scene.physics);
		const float angle = glm::radians(degrees);
		MeshRamp(scene.physics, angle);
		auto* body = scene.physics.CreateDynamicBody(20, 80, vec3(-200, 0, 0));
		Settle(scene.physics);
		body->vel.x = 300;
		for(int i = 0; i < 120; ++i) scene.physics.Step();
		const vec3 end = body->GetWorldPos();
		std::printf("Slope %.0f end %.3f %.3f\n", degrees, end.x, end.z);
		if(degrees == 60) {
			REQUIRE(end.x > 150 && end.z > 250);
		} else {
			REQUIRE(end.x < 50 && end.z < 150);
		}
	}
	return true;
}

bool CeilingAndResize()
{
	Scene scene;
	Floor(scene.physics);
	Box(scene.physics, PxVec3(1000, 1000, 50), PxTransform(PxVec3(0, 0, 400)));
	auto* body = scene.physics.CreateDynamicBody(100, 70, vec3(0));
	Settle(scene.physics);
	body->vel.z = 10000;
	scene.physics.Step();
	REQUIRE(body->GetWorldPos().z > 50 && body->GetWorldPos().z < 80);
	REQUIRE(!body->grounded && Near(body->vel.z, 0, 0.001f));
	const float ceilingFeet = body->GetWorldPos().z;
	scene.physics.Step();
	REQUIRE(body->GetWorldPos().z < ceilingFeet && body->vel.z < 0);
	Settle(scene.physics);
	REQUIRE(body->grounded && Near(body->GetWorldPos().z, 0));
	const vec3 feet = body->GetWorldPos();
	scene.physics.ResizeDynamicBody(body, 60, 100);
	REQUIRE(Near(body->GetWorldPos(), feet, 0.001f));
	scene.physics.Move(body, vec3(0, 0, 200), Dt, false);
	REQUIRE(body->GetWorldPos().z > 100 && body->GetWorldPos().z < 130);
	const vec3 airFeet = body->GetWorldPos();
	scene.physics.ResizeDynamicBody(body, 100, 70);
	REQUIRE(Near(body->GetWorldPos(), airFeet, 0.001f));
	body->collider->setFootPosition(PxExtendedVec3(feet.x, feet.y, feet.z));
	Settle(scene.physics);
	scene.physics.Move(body, vec3(0, 0, 200), Dt, false);
	REQUIRE(Near(body->GetWorldPos().z, ceilingFeet));
	return true;
}

bool QueryRestoration()
{
	Scene queried;
	Scene control;
	for(auto* scene: {&queried.physics, &control.physics}) {
		Floor(*scene);
		Box(*scene, PxVec3(50, 500, 500), PxTransform(PxVec3(300, 0, 500)));
	}
	auto* a = queried.physics.CreateDynamicBody(100, 70, vec3(0));
	auto* b = control.physics.CreateDynamicBody(100, 70, vec3(0));
	Settle(queried.physics);
	Settle(control.physics);
	a->vel = b->vel = vec3(120, 0, -100);
	const vec3 start = a->GetWorldPos();
	const auto& motion = GetGameXmlContent().masters.front().jumpMotions[0];
	REQUIRE(queried.physics.StartJump(a, motion));
	REQUIRE(control.physics.StartJump(b, motion));
	a->vel.z = b->vel.z = -100;
	const bool grounded = a->grounded;
	const JumpMotion* jump = a->jumpMotion;
	for(int i = 0; i < 10; ++i) {
		const vec3 result = queried.physics.FindMovePos(a, vec3(600, 0, -50), Dt);
		REQUIRE(result.x > 130 && result.x < 150 && Near(result.z, 0));
		REQUIRE(Near(a->GetWorldPos(), start, 0.001f));
		REQUIRE(a->grounded == grounded && Near(a->vel, b->vel, 0.001f));
		REQUIRE(a->jumpMotion == jump && Near(a->jumpElapsed, 0, 0.001f) && Near(a->jumpHeight, 0, 0.001f));
	}
	for(int i = 0; i < 60; ++i) {
		queried.physics.Step();
		control.physics.Step();
		REQUIRE(Near(a->GetWorldPos(), b->GetWorldPos(), 0.01f));
		REQUIRE(Near(a->vel, b->vel, 0.01f) && a->grounded == b->grounded);
	}
	return true;
}

bool ControllerFiltering()
{
	Scene scene;
	Floor(scene.physics);
	auto* moving = scene.physics.CreateDynamicBody(100, 70, vec3(-300, 0, 0));
	auto* other = scene.physics.CreateDynamicBody(100, 70, vec3(0));
	Settle(scene.physics);
	moving->vel.x = 600;
	for(int i = 0; i < 60; ++i) scene.physics.Step();
	REQUIRE(Near(moving->GetWorldPos(), vec3(300, 0, 0)));
	REQUIRE(Near(other->GetWorldPos(), vec3(0)));
	for(auto group: {PhysicsCollisionGroup::ControllerPc, PhysicsCollisionGroup::Static}) {
		Scene filtered;
		Floor(filtered.physics);
		Box(filtered.physics, PxVec3(50, 500, 500), PxTransform(PxVec3(0, 0, 500)), group);
		auto* body = filtered.physics.CreateDynamicBody(100, 70, vec3(-300, 0, 0));
		Settle(filtered.physics);
		body->vel.x = 600;
		for(int i = 0; i < 60; ++i) filtered.physics.Step();
		if(group == PhysicsCollisionGroup::ControllerPc) {
			REQUIRE(Near(body->GetWorldPos().x, 300));
		} else {
			REQUIRE(body->GetWorldPos().x < -150 && body->GetWorldPos().x > -170);
		}
	}
	return true;
}

bool JumpProfiles()
{
	for(const auto& master: GetGameXmlContent().masters) {
		for(const auto& motion: master.jumpMotions) {
			Scene scene;
			Floor(scene.physics);
			auto* body = scene.physics.CreateDynamicBody(100, 70, vec3(0));
			Settle(scene.physics);
			REQUIRE(scene.physics.StartJump(body, motion));
			REQUIRE(!scene.physics.StartJump(body, motion));
			float elapsed = 0;
			float maximum = 0;
			bool rose = false;
			float authoredMaximum = 0;
			for(float height: motion.heights) authoredMaximum = eastl::max(authoredMaximum, height);
			bool descended = false;
			const int ticks = int(std::ceil(motion.duration / Dt)) + 3;
			for(int i = 0; i < ticks; ++i) {
				const float before = body->GetWorldPos().z;
				scene.physics.Step();
				elapsed = eastl::min(elapsed + Dt, motion.duration);
				const float height = body->GetWorldPos().z;
				REQUIRE(Near(height, motion.Sample(elapsed)));
				REQUIRE(height >= -0.5f && height <= authoredMaximum + 0.5f);
				if(height > before + 1) rose = true;
				if(height < before - 1) descended = true;
				maximum = eastl::max(maximum, height);
				if(body->jumpMotion) REQUIRE(!scene.physics.StartJump(body, motion));
			}
			REQUIRE(rose && descended && maximum > 300);
			REQUIRE(body->grounded && !body->jumpMotion && Near(body->GetWorldPos().z, 0));
			REQUIRE(scene.physics.StartJump(body, motion));
		}
	}
	return true;
}

bool HumanWalkingDirection()
{
	WorldScene scene;
	auto& player = scene.Player();
	player.input.moveDir = vec2(0, 1);
	player.input.moveTo = vec3(-1000, 0, 0);
	player.input.speed = 600;
	scene.Step(17);
	REQUIRE(Near(player.body->GetWorldPos(), vec3(0, 10, 0), 0.005f));
	player.input.moveDir = vec2(0);
	player.input.moveTo = vec3(0, 35, 0);
	scene.Step(34);
	REQUIRE(Near(player.body->GetWorldPos(), vec3(0, 20, 0), 0.005f));
	scene.Step(51);
	scene.Step(68);
	REQUIRE(Near(player.body->GetWorldPos(), vec3(0, 35, 0), 0.005f));
	scene.Step(85);
	REQUIRE(Near(player.body->GetWorldPos(), vec3(0, 35, 0), 0.005f));
	return true;
}

bool GraphLifecycle(SkillID skill, int deferredTicks)
{
	WorldScene scene;
	auto& player = scene.Player();
	player.input.cast.skillID = skill;
	player.input.cast.pos = vec3(0);
	player.input.moveDir = vec2(1, 0);
	player.input.speed = deferredTicks ? 0.f : 480.f;
	scene.Step(17);
	REQUIRE(!scene.world.skillProgramList.empty());
	const auto* variant = scene.world.skillProgramList.front().horizontalVariant;
	REQUIRE(variant);
	const float terminal = variant->samples[variant->sampleCount - 1];
	for(int i = 1; i <= deferredTicks; ++i) {
		scene.Step(u32((i + 1) * 17));
		REQUIRE(Near(player.body->GetWorldPos().x, 0.f, 0.005f));
	}
	if(deferredTicks) {
		player.input.speed = 480.f;
		scene.Step(20000);
	}
	const vec3 first = player.body->GetWorldPos();
	REQUIRE(Near(first.x, variant->Sample(Dt), 0.005f));
	REQUIRE(player.movement.forcedMove);
	scene.Step(20017);
	REQUIRE(Near(player.body->GetWorldPos().x, variant->Sample(Dt + Dt), 0.005f));
	player.input.moveDir = vec2(0);
	player.input.speed = 0.f;
	for(int i = 0; i < 240 && !scene.world.skillProgramList.empty(); ++i) {
		scene.Step(u32(20034 + i * 17));
	}
	REQUIRE(scene.world.skillProgramList.empty());
	REQUIRE(Near(player.body->GetWorldPos().x, terminal, 0.005f));
	std::printf("GRAPH skill=%d first=%.6f terminal=%.6f actual=%.6f\n",
		int(skill), first.x, terminal, player.body->GetWorldPos().x);
	return true;
}

bool DelayedGraphLifecycle() { return GraphLifecycle(SkillID(180060040), 60); }
bool ImmediateGraphLifecycle() { return GraphLifecycle(SkillID(180060002), 0); }

bool AirborneHorizontalGraph()
{
	WorldScene scene;
	World::PlayerDescription description{};
	description.clientHd = ClientHandle::INVALID;
	description.masters = { ClassType(6), ClassType(5) };
	auto& player = scene.world.CreatePlayer(description, vec3(0, 0, 500), RotationHumanoid{});
	player.input.cast.skillID = SkillID(180060002);
	bool resumedFall = false;
	for(int i = 1; i <= 90; ++i) {
		scene.Step(u32(i * 17));
		if(player.movement.forcedMove) {
			REQUIRE(Near(player.body->GetWorldPos().z, 500.f, 0.005f));
			REQUIRE(!player.body->grounded && player.body->vel.z < 0.f);
		}
		else {
			REQUIRE(player.body->GetWorldPos().z < 499.f);
			resumedFall = true;
			break;
		}
	}
	REQUIRE(resumedFall);
	return true;
}

bool HostileMovementPackets()
{
	Cl::CN_GameUpdatePosition update{};
	update.nSpeed = 2001.f;
	REQUIRE(ValidatePacket<Cl::CN_GameUpdatePosition>(&update, sizeof(update)));
	update.nSpeed = MAX_MOVEMENT_INPUT_SPEED;
	REQUIRE(ValidatePacket<Cl::CN_GameUpdatePosition>(&update, sizeof(update)));
	update.nSpeed = std::nextafter(MAX_MOVEMENT_INPUT_SPEED, std::numeric_limits<float>::infinity());
	REQUIRE(!ValidatePacket<Cl::CN_GameUpdatePosition>(&update, sizeof(update)));
	update.nSpeed = std::numeric_limits<float>::max();
	REQUIRE(!ValidatePacket<Cl::CN_GameUpdatePosition>(&update, sizeof(update)));
	update.nSpeed = std::numeric_limits<float>::infinity();
	REQUIRE(!ValidatePacket<Cl::CN_GameUpdatePosition>(&update, sizeof(update)));
	update.nSpeed = std::numeric_limits<float>::quiet_NaN();
	REQUIRE(!ValidatePacket<Cl::CN_GameUpdatePosition>(&update, sizeof(update)));
	update.nSpeed = -1.f;
	REQUIRE(!ValidatePacket<Cl::CN_GameUpdatePosition>(&update, sizeof(update)));
	update.nSpeed = 0.f;
	REQUIRE(ValidatePacket<Cl::CN_GameUpdatePosition>(&update, sizeof(update)));

	constexpr size_t prefixSize = sizeof(LocalActorID) + sizeof(SkillID) + sizeof(float3) + sizeof(u16);
	u8 castData[prefixSize + sizeof(Cl::CQ_PlayerCastSkill::PosStruct)]{};
	Cl::CQ_PlayerCastSkill::PosStruct pos{};
	pos.speed = 2001.f;
	std::memcpy(castData + prefixSize, &pos, sizeof(pos));
	REQUIRE(ValidatePacket<Cl::CQ_PlayerCastSkill>(castData, sizeof(castData)));
	pos.speed = MAX_MOVEMENT_INPUT_SPEED;
	std::memcpy(castData + prefixSize, &pos, sizeof(pos));
	REQUIRE(ValidatePacket<Cl::CQ_PlayerCastSkill>(castData, sizeof(castData)));
	pos.speed = std::nextafter(MAX_MOVEMENT_INPUT_SPEED, std::numeric_limits<float>::infinity());
	std::memcpy(castData + prefixSize, &pos, sizeof(pos));
	REQUIRE(!ValidatePacket<Cl::CQ_PlayerCastSkill>(castData, sizeof(castData)));
	pos.speed = std::numeric_limits<float>::max();
	std::memcpy(castData + prefixSize, &pos, sizeof(pos));
	REQUIRE(!ValidatePacket<Cl::CQ_PlayerCastSkill>(castData, sizeof(castData)));
	pos.speed = std::numeric_limits<float>::quiet_NaN();
	std::memcpy(castData + prefixSize, &pos, sizeof(pos));
	REQUIRE(!ValidatePacket<Cl::CQ_PlayerCastSkill>(castData, sizeof(castData)));
	return true;
}
}

int main()
{
	TimeInit();
	LogInit("physics_tests.log");
	if(!GameXmlContentLoad()) {
		std::fprintf(stderr, "GameXmlContentLoad failed; run from build/ with synced gamedata\n");
		return 1;
	}
	if(!PhysicsInit()) {
		std::fprintf(stderr, "PhysicsInit failed\n");
		return 1;
	}
	const struct { const char* name; bool (*run)(); } tests[] = {
		{"loaded DeathMatch walk/fall/wall", LoadedMapWalk},
		{"free fall and landing", FreeFall},
		{"float-position movement requests", FloatPositionRequests},
		{"step climb and block", Steps},
		{"segmented multi-step traversal and zero-motion state", SegmentedTraversal},
		{"slope limit", Slopes},
		{"ceiling and feet-preserving resize", CeilingAndResize},
		{"query state restoration", QueryRestoration},
		{"controller-PC filtering", ControllerFiltering},
		{"loaded ordinary jump profiles", JumpProfiles},
		{"human walking direction and goal-seeking fallback", HumanWalkingDirection},
		{"delayed reverse graph activation and terminal clamp", DelayedGraphLifecycle},
		{"immediate graph and same-update execution", ImmediateGraphLifecycle},
		{"airborne horizontal graph and fall resumption", AirborneHorizontalGraph},
		{"hostile movement packet rejection", HostileMovementPackets},
	};
	int failed = 0;
	for(const auto& test: tests) {
		std::printf("RUN %s\n", test.name);
		const bool passed = test.run();
		std::printf("%s %s\n", passed ? "PASS" : "FAIL", test.name);
		if(!passed) ++failed;
	}
	auto& context = PhysContext();
	for(auto& mesh: context.triangleMeshMap) mesh.second->release();
	context.triangleMeshMap.clear();
	context.matMapSurface->release();
	static_cast<PxDefaultCpuDispatcher*>(context.dispatcher)->release();
	context.Shutdown();
	std::printf("physics_tests: %d failed\n", failed);
	return failed ? 1 : 0;
}
