#include <common/base.h>
#include <mxm/game_content.h>
#include <servers/game/physics.h>
#include <servers/game/world.h>
#include <common/packet_validator.h>
#include <NxPhysics.h>
#include <NxCooking.h>
#include <NxController.h>
#include <PhysXLoader.h>
#include <NxFoundationSDK.h>
#include <NxStream.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <initializer_list>
#include <memory>

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
	NxTriangleMesh* triangleMesh = nullptr;

	Scene() { PhysContext().CreateScene(&physics); }
	~Scene()
	{
		physics.Destroy();
		if(triangleMesh) PhysContext().physics->releaseTriangleMesh(*triangleMesh);
	}
	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;
};

NxActor* Box(PhysicsScene& scene, const NxVec3& halfSize, const NxVec3& position,
	PhysicsCollisionGroup group = PhysicsCollisionGroup::Static)
{
	NxBoxShapeDesc shape;
	shape.dimensions = halfSize;
	shape.group = static_cast<NxCollisionGroup>(group);
	shape.groupsMask.bits0 = 1u << u32(group);
	NxActorDesc description;
	description.globalPose.t = position;
	description.shapes.pushBack(&shape);
	NxActor* actor = scene.scene->createActor(description);
	ASSERT(actor);
	return actor;
}

void Floor(PhysicsScene& scene)
{
	Box(scene, NxVec3(5000, 5000, 100), NxVec3(0, 0, -100));
}

class CookedMeshStream: public NxStream
{
	GrowableBuffer data{4096};
	mutable NxU32 position = 0;

	template<class T> T Read() const
	{
		T value{};
		readBuffer(&value, sizeof(value));
		return value;
	}

public:
	CookedMeshStream() = default;
	CookedMeshStream(const CookedMeshStream&) = delete;
	CookedMeshStream& operator=(const CookedMeshStream&) = delete;

	void Rewind() { position = 0; }

	NxU8 readByte() const override { return Read<NxU8>(); }
	NxU16 readWord() const override { return Read<NxU16>(); }
	NxU32 readDword() const override { return Read<NxU32>(); }
	NxF32 readFloat() const override { return Read<NxF32>(); }
	NxF64 readDouble() const override { return Read<NxF64>(); }
	void readBuffer(void* buffer, NxU32 size) const override
	{
		ASSERT(position <= NxU32(data.size) && size <= NxU32(data.size) - position);
		std::memcpy(buffer, data.data + position, size);
		position += size;
	}
	NxStream& storeByte(NxU8 value) override { return storeBuffer(&value, sizeof(value)); }
	NxStream& storeWord(NxU16 value) override { return storeBuffer(&value, sizeof(value)); }
	NxStream& storeDword(NxU32 value) override { return storeBuffer(&value, sizeof(value)); }
	NxStream& storeFloat(NxF32 value) override { return storeBuffer(&value, sizeof(value)); }
	NxStream& storeDouble(NxF64 value) override { return storeBuffer(&value, sizeof(value)); }
	NxStream& storeBuffer(const void* buffer, NxU32 size) override
	{
		ASSERT(size <= NxU32(std::numeric_limits<i32>::max() - data.size));
		const i32 count = i32(size);
		if(count > data.capacity - data.size) {
			data.Reserve(MAX(data.size + count, data.capacity * 2));
		}
		data.Append(buffer, count);
		return *this;
	}
};

void MeshRamp(Scene& scene, float angle)
{
	auto& context = PhysContext();
	const float c = std::cos(angle);
	const float s = std::sin(angle);
	const NxVec3 vertices[] = {
		NxVec3(-40 * s, -500, 0),
		NxVec3(2000 * c - 40 * s, -500, 2000 * s),
		NxVec3(2000 * c - 40 * s, 500, 2000 * s),
		NxVec3(-40 * s, 500, 0),
	};
	const NxU32 indices[] = {0, 1, 2, 0, 2, 3};
	NxTriangleMeshDesc description;
	description.numVertices = 4;
	description.pointStrideBytes = sizeof(NxVec3);
	description.points = vertices;
	description.numTriangles = 2;
	description.triangleStrideBytes = sizeof(NxU32) * 3;
	description.triangles = indices;
	NxCookingInterface* cooking = NxGetCookingLib(NX_PHYSICS_SDK_VERSION);
	ASSERT(cooking);
	auto& foundation = context.physics->getFoundationSDK();
	const bool initialized = cooking->NxInitCooking(&foundation.getAllocator(), foundation.getErrorStream());
	ASSERT(initialized);
	CookedMeshStream stream;
	const bool cooked = cooking->NxCookTriangleMesh(description, stream);
	cooking->NxCloseCooking();
	ASSERT(cooked);
	stream.Rewind();
	ASSERT(!scene.triangleMesh);
	scene.triangleMesh = context.physics->createTriangleMesh(stream);
	ASSERT(scene.triangleMesh);
	NxTriangleMeshShapeDesc shape;
	shape.meshData = scene.triangleMesh;
	shape.group = static_cast<NxCollisionGroup>(PhysicsCollisionGroup::Static);
	shape.groupsMask.bits0 = 1u << u32(PhysicsCollisionGroup::Static);
	NxActorDesc actorDescription;
	actorDescription.shapes.pushBack(&shape);
	NxActor* actor = scene.physics.scene->createActor(actorDescription);
	ASSERT(actor);
}

void Settle(PhysicsScene& scene, int ticks = 90)
{
	for(int i = 0; i < ticks; ++i) scene.Step();
}

struct WorldScene
{
	World world;
	Replication* replica = nullptr;

	WorldScene()
	{
		replica = new Replication();
		replica->Init(nullptr);
		world.Init(replica);
		Floor(world.physics);
	}

	~WorldScene() { world.Cleanup(); delete replica; }
	WorldScene(const WorldScene&) = delete;
	WorldScene& operator=(const WorldScene&) = delete;

	World::Player& Player(ClientHandle clientHd = ClientHandle::INVALID, ClassType classType = ClassType(6))
	{
		World::PlayerDescription description{};
		description.clientHd = clientHd;
		description.masters = { classType, ClassType(5) };
		auto& player = world.CreatePlayer(description, vec3(0), RotationHumanoid{});
		Settle(world.physics);
		if(clientHd != ClientHandle::INVALID) {
			replica->OnPlayerConnect(clientHd, player.index);
			world.BeginPlayerTimeline(clientHd, player.Main().UID, 0.0);
		}
		return player;
	}

	void Step(f64 milliseconds)
	{
		world.replication->frameCur->Clear();
		world.Update(TimeMsToTime(milliseconds), milliseconds / 1000.0);
	}
};

void DeathMatch(PhysicsScene& scene)
{
	scene.CreateCollisionAsset("PVP_DeathMatch01_Collision", vec3(0), vec3(0), 1.0f, PhysicsCollisionGroup::Prop);
	scene.CreateCollisionAsset("PVP_DeathMatch01_Env", vec3(0), vec3(0), 1.0f, PhysicsCollisionGroup::Prop);
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
	body->moveDir = vec2(1, 1);
	body->moveSpeed = 447.15f;
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

bool RigidSdkStepBoundaries()
{
	Scene scene;
	NxBoxShapeDesc shape;
	shape.dimensions = NxVec3(10, 10, 10);
	NxBodyDesc body;
	body.linearDamping = 0.0f;
	NxActorDesc description;
	description.body = &body;
	description.density = 1.0f;
	description.globalPose.t = NxVec3(0, 0, 500);
	description.shapes.pushBack(&shape);
	NxActor* actor = scene.physics.scene->createActor(description);
	ASSERT(actor);
	scene.physics.Step();
	const f32 first = actor->getGlobalPosition().z;
	std::printf("SDK_RIGID first=%.6f\n", first);
	REQUIRE(Near(first, 498.0f, 0.005f));
	scene.physics.Step();
	REQUIRE(Near(actor->getGlobalPosition().z, first, 0.005f));
	scene.physics.Step();
	REQUIRE(Near(actor->getGlobalPosition().z, 494.0f, 0.005f));
	scene.physics.FinishStep(1.0f);
	REQUIRE(Near(actor->getGlobalPosition().z, 390.0f, 0.005f));
	scene.physics.Step();
	REQUIRE(Near(actor->getGlobalPosition().z, 390.0f, 0.005f));
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
	body->moveDir = vec2(1, 0);
	body->moveSpeed = 420.85f;
	for(int i = 0; i < 30; ++i) scene.physics.Step();
	REQUIRE(Near(body->GetWorldPos().x, 1048786.f, 0.005f));
	body->moveSpeed = 0.6f;
	for(int i = 0; i < 30; ++i) scene.physics.Step();
	REQUIRE(Near(body->GetWorldPos().x, 1048786.f, 0.005f));
	return true;
}

bool Steps()
{
	for(float height: {50.0f, 120.0f}) {
		Scene scene;
		Floor(scene.physics);
		Box(scene.physics, NxVec3(1000, 500, height * 0.5f), NxVec3(1000, 0, height * 0.5f));
		auto* body = scene.physics.CreateDynamicBody(100, 70, vec3(-300, 0, 0));
		Settle(scene.physics);
		body->moveDir = vec2(1, 0);
		body->moveSpeed = 480;
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
			REQUIRE(end.x < 0 && Near(end.z, 0));
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
			Box(scene.physics, NxVec3(150, 500, height * 0.5f),
				NxVec3(float(150 + i * 300), 0, height * 0.5f));
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
		MeshRamp(scene, angle);
		auto* body = scene.physics.CreateDynamicBody(20, 80, vec3(-200, 0, 0));
		Settle(scene.physics);
		body->moveDir = vec2(1, 0);
		body->moveSpeed = 300;
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
	Box(scene.physics, NxVec3(1000, 1000, 50), NxVec3(0, 0, 400));
	auto* body = scene.physics.CreateDynamicBody(100, 70, vec3(0));
	Settle(scene.physics);
	scene.physics.InstallJumpGraph(body, GetGameXmlContent().GetJumpMotion(ClassType(6), JumpDirection::Stand));
	float ceilingFeet = 0.0f;
	for(int i = 0; i < 90; ++i) {
		scene.physics.Step();
		ceilingFeet = eastl::max(ceilingFeet, body->GetWorldPos().z);
		REQUIRE(body->GetWorldPos().z < 80.0f);
	}
	REQUIRE(ceilingFeet > 50.0f);
	REQUIRE(body->grounded && Near(body->GetWorldPos().z, 0));
	const vec3 feet = body->GetWorldPos();
	scene.physics.ResizeDynamicBody(body, 60, 100);
	REQUIRE(Near(body->GetWorldPos(), feet, 0.001f));
	scene.physics.Move(body, vec3(0, 0, 200), Dt, false);
	REQUIRE(body->GetWorldPos().z > 100 && body->GetWorldPos().z < 130);
	const vec3 airFeet = body->GetWorldPos();
	scene.physics.ResizeDynamicBody(body, 100, 70);
	REQUIRE(Near(body->GetWorldPos(), airFeet, 0.001f));
	NxExtendedVec3 center = body->collider->getDebugPosition();
	center.x = feet.x;
	center.y = feet.y;
	center.z += double(feet.z - body->GetWorldPos().z);
	body->collider->setPosition(center);
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
		Box(*scene, NxVec3(50, 500, 500), NxVec3(300, 0, 500));
	}
	auto* a = queried.physics.CreateDynamicBody(100, 70, vec3(0));
	auto* b = control.physics.CreateDynamicBody(100, 70, vec3(0));
	Settle(queried.physics);
	Settle(control.physics);
	a->moveDir = b->moveDir = vec2(1, 0);
	a->moveSpeed = b->moveSpeed = 120;
	a->vel.z = b->vel.z = -100;
	const vec3 start = a->GetWorldPos();
	const auto& motion = GetGameXmlContent().masters.front().jumpMotions[0];
	queried.physics.InstallJumpGraph(a, motion);
	control.physics.InstallJumpGraph(b, motion);
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
	moving->moveDir = vec2(1, 0);
	moving->moveSpeed = 600;
	for(int i = 0; i < 60; ++i) scene.physics.Step();
	REQUIRE(Near(moving->GetWorldPos(), vec3(300, 0, 0)));
	REQUIRE(Near(other->GetWorldPos(), vec3(0)));
	for(auto group: {PhysicsCollisionGroup::ControllerPc, PhysicsCollisionGroup::Static}) {
		Scene filtered;
		Floor(filtered.physics);
		Box(filtered.physics, NxVec3(50, 500, 500), NxVec3(0, 0, 500), group);
		auto* body = filtered.physics.CreateDynamicBody(100, 70, vec3(-300, 0, 0));
		Settle(filtered.physics);
		body->moveDir = vec2(1, 0);
		body->moveSpeed = 600;
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
			scene.physics.InstallJumpGraph(body, motion);
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
			}
			REQUIRE(rose && descended && maximum > 300);
			REQUIRE(body->grounded && !body->jumpMotion && Near(body->GetWorldPos().z, 0));
		}
	}
	return true;
}

bool HumanWalkingDirection()
{
	WorldScene scene;
	World::PlayerDescription description{};
	description.clientHd = ClientHandle(1);
	description.masters = { ClassType(6), ClassType(5) };
	auto& player = scene.world.CreatePlayer(description, vec3(0), RotationHumanoid{});
	scene.world.replication->OnPlayerConnect(description.clientHd, player.index);
	Settle(scene.world.physics);
	player.input.moveDir = vec2(0, 0.5f);
	player.input.moveTo = vec3(-1000, 0, 0);
	player.input.speed = 600;
	scene.Step(17);
	REQUIRE(Near(player.body->GetWorldPos(), vec3(0, 5, 0), 0.005f));
	player.input.moveDir = vec2(0);
	player.input.moveTo = vec3(0, 35, 0);
	scene.Step(34);
	scene.Step(51);
	REQUIRE(Near(player.body->GetWorldPos(), vec3(0, 5, 0), 0.005f));
	return true;
}

bool GateCollisionLifecycle()
{
	WorldScene scene;
	auto& gate = scene.world.SpawnDynamic(CreatureIndex(110040546), 15);
	gate.pos = vec3(0, 500, 0);
	auto& player = scene.Player();
	player.input.moveDir = vec2(0, 1);
	player.input.speed = 600;
	u32 tick = 0;
	for(int i = 0; i < 60; ++i) scene.Step(++tick * 17);
	REQUIRE(player.body->GetWorldPos().y > 380 && player.body->GetWorldPos().y < 400);
	const NxRay query(NxVec3(0, 0, 200), NxVec3(0, 1, 0));
	const NxU32 groups = ~NxU32(1u << u32(PhysicsCollisionGroup::ControllerPc));
	REQUIRE(scene.world.physics.scene->raycastAnyShape(query, NX_ALL_SHAPES, groups, 1000.0f));
	gate.action = ActionStateID::DYNAMIC_OPEN;
	for(int i = 0; i < 40; ++i) scene.Step(++tick * 17);
	REQUIRE(player.body->GetWorldPos().y > 780);
	REQUIRE(!scene.world.physics.scene->raycastAnyShape(query, NX_ALL_SHAPES, groups, 1000.0f));
	gate.action = ActionStateID::DYNAMIC_CLOSE;
	player.input.moveDir = vec2(0, -1);
	for(int i = 0; i < 40; ++i) scene.Step(++tick * 17);
	REQUIRE(player.body->GetWorldPos().y > 600 && player.body->GetWorldPos().y < 620);
	REQUIRE(scene.world.physics.scene->raycastAnyShape(query, NX_ALL_SHAPES, groups, 1000.0f));
	return true;
}

bool MatchClockIgnoresLaneUptime()
{
	WorldScene scene;
	std::unique_ptr<WorldScene> reference{new WorldScene()};
	auto& player = scene.Player();
	auto& expected = reference->Player();
	const f64 uptimeMs = 262144000.0;
	scene.Step(uptimeMs + 17);
	reference->Step(17);
	player.input.jump = true;
	expected.input.jump = true;
	for(u32 tick = 2; tick <= 12; ++tick) {
		scene.Step(uptimeMs + tick * 17);
		reference->Step(tick * 17);
		REQUIRE(Near(player.body->GetWorldPos(), expected.body->GetWorldPos(), 0.01f));
	}
	player.input.cast.skillID = SkillID(180060040);
	expected.input.cast.skillID = SkillID(180060040);
	for(u32 tick = 13; tick <= 240; ++tick) {
		scene.Step(uptimeMs + tick * 17);
		reference->Step(tick * 17);
		REQUIRE(Near(player.body->GetWorldPos(), expected.body->GetWorldPos(), 0.01f));
	}
	return true;
}

bool JumpRequestEntryPhases()
{
	WorldScene scene;
	auto& player = scene.Player();
	const f32 origin = player.body->GetWorldPos().z;
	player.input.jump = true;
	player.input.jumpMoveDir = vec2(0);
	player.input.jumpRotate = 0.f;
	scene.Step(17);
	REQUIRE(Near(player.body->GetWorldPos().z, origin, 0.005f));
	player.input.jump = true;
	scene.Step(34);
	REQUIRE(Near(player.body->GetWorldPos().z, origin, 0.005f));
	scene.Step(51);
	const auto& motion = GetGameXmlContent().GetJumpMotion(ClassType(6), JumpDirection::Stand);
	const f32 dt = scene.world.physics.StepDuration();
	REQUIRE(Near(player.body->GetWorldPos().z, origin + motion.Sample(dt), 0.01f));
	player.input.jump = true;
	scene.Step(68);
	REQUIRE(Near(player.body->GetWorldPos().z, origin + motion.Sample(dt + dt), 0.01f));
	return true;
}

bool RoundedClockJumpTail()
{
	Scene scene;
	auto* platform = Box(scene.physics, NxVec3(1000, 1000, 25), NxVec3(0, 0, 475));
	auto* body = scene.physics.CreateDynamicBody(100, 70, vec3(0, 0, 500));
	Settle(scene.physics);
	REQUIRE(body->grounded);
	scene.physics.localTime = TimeMsToTime(50000);
	scene.physics.InstallJumpGraph(body, GetGameXmlContent().GetJumpMotion(ClassType(6), JumpDirection::Stand));
	platform->raiseActorFlag(NX_AF_DISABLE_COLLISION);
	for(int i = 0; i < 36; ++i) scene.physics.Step();
	const f32 beforeTerminal = body->GetWorldPos().z;
	scene.physics.Step();
	const f32 terminal = body->GetWorldPos().z;
	REQUIRE(Near(terminal, 500.f, 0.005f));
	REQUIRE(beforeTerminal > terminal && beforeTerminal - terminal < 0.1f);
	scene.physics.Step();
	REQUIRE(body->GetWorldPos().z < terminal - 20.f && body->GetWorldPos().z > terminal - 24.f);
	return true;
}

bool GraphLifecycle(SkillID skill, ClassType classType = ClassType(6))
{
	WorldScene scene;
	auto& player = scene.Player(ClientHandle::INVALID, classType);
	player.input.cast.skillID = skill;
	player.input.cast.pos = vec3(0);
	player.input.moveDir = vec2(1, 0);
	player.input.speed = 0.f;
	scene.Step(17);
	REQUIRE(!scene.world.skillProgramList.empty());
	const auto* variant = scene.world.skillProgramList.front().horizontalVariant;
	REQUIRE(variant);
	const float terminal = variant->samples[variant->sampleCount - 1];
	REQUIRE(Near(player.body->GetWorldPos().x, 0.f, 0.005f));
	REQUIRE(!player.movement.forcedMove);
	const auto& action = GetGameXmlContent().GetSkillAction(player.Main().classType,
		scene.world.skillProgramList.front().actionID);
	f32 nominalStart = 0.f;
	for(const auto& command: action.commands) {
		if(command.type == ActionCommand::Type::GRAPH_MOVE_HORZ) nominalStart = command.executeAt;
	}
	u32 tick = 1;
	const u32 prefixTicks = u32(std::ceil(nominalStart / float(Dt)));
	for(u32 i = 0; i < prefixTicks; ++i) {
		scene.Step(++tick * 17);
		REQUIRE(Near(player.body->GetWorldPos().x, 0.f, 0.005f));
	}
	while(!player.movement.forcedMove && tick < prefixTicks + 16) scene.Step(++tick * 17);
	REQUIRE(player.movement.forcedMove);
	const vec3 first = player.body->GetWorldPos();
	REQUIRE(Near(first.x, variant->Sample(Dt), 0.005f));
	REQUIRE(player.movement.forcedMove);
	player.input.speed = 480.f;
	scene.Step(++tick * 17);
	REQUIRE(Near(player.body->GetWorldPos().x, variant->Sample(Dt + Dt), 0.005f));
	player.input.moveDir = vec2(0);
	player.input.speed = 0.f;
	for(int i = 0; i < 240 && !scene.world.skillProgramList.empty(); ++i) {
		scene.Step(++tick * 17);
	}
	REQUIRE(scene.world.skillProgramList.empty());
	REQUIRE(Near(player.body->GetWorldPos().x, terminal, 0.005f));
	std::printf("GRAPH skill=%d first=%.6f terminal=%.6f actual=%.6f\n",
		int(skill), first.x, terminal, player.body->GetWorldPos().x);
	return true;
}

bool DelayedGraphLifecycle() { return GraphLifecycle(SkillID(180060040)); }
bool FirstClipGraphLifecycle() { return GraphLifecycle(SkillID(180060002)); }
bool OmittedIndexGraphLifecycle() { return GraphLifecycle(SkillID(180090002), ClassType(9)); }

bool WarpDirectionAndLifecycle()
{
	const struct { vec2 direction; f32 yaw; vec3 destination; } cases[] = {
		{ vec2(0, 0.5f), 0.f, vec3(450, 0, 0) },
		{ vec2(0), glm::radians(90.f), vec3(0, 450, 0) },
	};
	for(const auto& test: cases) {
		WorldScene scene;
		auto& player = scene.Player(ClientHandle::INVALID, ClassType(3));
		player.input.moveDir = test.direction;
		player.input.rot.bodyYaw = test.yaw;
		player.input.cast.skillID = SkillID(180030002);
		scene.Step(17);
		REQUIRE(Near(player.body->GetWorldPos(), vec3(0), 0.005f));
		scene.Step(34);
		REQUIRE(Near(player.body->GetWorldPos(), test.destination, 0.005f));
		REQUIRE(player.graphIgnoreCollision && player.body->destActive);
		u32 tick = 2;
		while(!scene.world.skillProgramList.empty() && tick < 120) {
			scene.Step(++tick * 17);
			REQUIRE(Near(player.body->GetWorldPos(), test.destination, 0.005f));
		}
		REQUIRE(!player.body->destActive && !player.graphIgnoreCollision);
		player.input.moveDir = vec2(0, 0.5f);
		player.input.speed = 600.f;
		scene.Step(++tick * 17);
		REQUIRE(Near(player.body->GetWorldPos(), test.destination + vec3(0, 5, 0), 0.005f));
	}
	return true;
}

bool WarpBlockedDestinationRestore()
{
	Scene scene;
	Box(scene.physics, NxVec3(50, 1000, 1500), NxVec3(300, 0, 1500));
	auto* body = scene.physics.CreateDynamicBody(100, 70, vec3(0, 0, 500));
	body->vel.z = -123.f;
	scene.physics.InstallMoveDest(body, vec3(450, 0, 500), true);
	const auto state = scene.physics.CaptureBodyState(*body);
	scene.physics.StepBody(*body, vec2(0, 10), Dt);
	const vec3 blocked = body->GetWorldPos();
	REQUIRE(blocked.x > 130.f && blocked.x < 150.f);
	REQUIRE(Near(blocked.y, 0.f, 0.005f) && Near(blocked.z, 500.f, 0.005f));
	REQUIRE(Near(body->vel.z, -123.f, 0.005f));
	scene.physics.StepBody(*body, vec2(0, 10), Dt);
	REQUIRE(Near(body->GetWorldPos(), blocked, 0.005f));
	REQUIRE(Near(body->vel.z, -123.f, 0.005f));
	scene.physics.CancelMoveDest(body);
	scene.physics.StepBody(*body, vec2(0, 10), Dt);
	REQUIRE(Near(body->GetWorldPos().y, 10.f, 0.005f));
	REQUIRE(body->GetWorldPos().z < blocked.z - 2.f);
	scene.physics.RestoreBodyState(*body, state);
	scene.physics.StepBody(*body, vec2(0, 10), Dt);
	REQUIRE(Near(body->GetWorldPos(), blocked, 0.005f));
	REQUIRE(Near(body->vel.z, -123.f, 0.005f));
	return true;
}

bool WarpSegmentedTraversal()
{
	Scene scene;
	Floor(scene.physics);
	for(int i = 0; i < 3; ++i) {
		const f32 height = f32((i + 1) * 50);
		Box(scene.physics, NxVec3(150, 500, height * 0.5f),
			NxVec3(f32(150 + i * 300), 0, height * 0.5f));
	}
	auto* body = scene.physics.CreateDynamicBody(20, 80, vec3(-100, 0, 0));
	Settle(scene.physics);
	scene.physics.InstallMoveDest(body, vec3(900, 0, 0), true);
	scene.physics.StepBody(*body, vec2(0), Dt);
	const vec3 end = body->GetWorldPos();
	std::printf("WARP steps end %.3f %.3f\n", end.x, end.z);
	REQUIRE(Near(end, vec3(900, 0, 150)));
	return true;
}

bool WarpDestinationInputReplay()
{
	WorldScene scene;
	std::unique_ptr<WorldScene> reference{new WorldScene()};
	auto& player = scene.Player(ClientHandle(1), ClassType(3));
	auto& expected = reference->Player(ClientHandle(1), ClassType(3));
	World::Player::Input input{};
	REQUIRE(scene.world.QueuePlayerInput(player, player.Main().UID, World::InputKind::Movement, input, 0.0, true));
	REQUIRE(reference->world.QueuePlayerInput(expected, expected.Main().UID, World::InputKind::Movement, input, 0.0, true));
	input.moveDir = vec2(1, 0);
	input.clientTime = 0.100f;
	input.cast.skillID = SkillID(180030002);
	input.cast.clientTime = input.clientTime;
	REQUIRE(scene.world.QueuePlayerInput(player, player.Main().UID, World::InputKind::Cast, input, 0.101, true));
	REQUIRE(reference->world.QueuePlayerInput(expected, expected.Main().UID, World::InputKind::Cast, input, 0.101, true));
	input.moveDir = vec2(0, 0.5f);
	input.speed = 600.f;
	input.clientTime = 0.130f;
	input.cast.skillID = SkillID::INVALID;
	input.action = ActionStateID(18);
	REQUIRE(reference->world.QueuePlayerInput(expected, expected.Main().UID, World::InputKind::Movement, input, 0.131, true));
	for(u32 tick = 1; tick <= 13; ++tick) {
		scene.Step(tick * 17);
		reference->Step(tick * 17);
	}
	REQUIRE(player.body->destActive);
	REQUIRE(scene.world.QueuePlayerInput(player, player.Main().UID, World::InputKind::Movement, input, 0.230, true));
	for(u32 tick = 14; tick <= 40; ++tick) {
		scene.Step(tick * 17);
		reference->Step(tick * 17);
		REQUIRE(Near(player.body->GetWorldPos(), expected.body->GetWorldPos(), 0.01f));
		REQUIRE(scene.replica->frameCur->skillCastList.empty() && scene.replica->frameCur->skillExecList.empty());
		if(tick == 20) {
			REQUIRE(!player.body->destActive && !player.graphIgnoreCollision);
			REQUIRE(player.body->GetWorldPos().y > 5.f);
		}
	}
	REQUIRE(Near(player.body->GetWorldPos().x, 450.f, 0.005f));
	REQUIRE(!player.body->destActive && !player.graphIgnoreCollision);
	const vec3 resumed = player.body->GetWorldPos();
	scene.Step(41 * 17);
	REQUIRE(Near(player.body->GetWorldPos(), resumed + vec3(0, 5.1f, 0), 0.01f));
	World::Player::Input refinement{};
	refinement.clientTime = 0.050f;
	const vec3 beforeReplay = player.body->GetWorldPos();
	REQUIRE(scene.world.QueuePlayerInput(player, player.Main().UID, World::InputKind::Rotation, refinement, 0.710, true));
	scene.Step(42 * 17);
	REQUIRE(Near(player.body->GetWorldPos(), beforeReplay + vec3(0, 5.1f, 0), 0.01f));
	const auto* published = scene.replica->frameCur->FindMaster(player.Main().UID);
	REQUIRE(published && published->actionState == ActionStateID::INVALID);
	return true;
}

bool AirborneHorizontalGraph()
{
	WorldScene scene;
	World::PlayerDescription description{};
	description.clientHd = ClientHandle::INVALID;
	description.masters = { ClassType(6), ClassType(5) };
	auto& player = scene.world.CreatePlayer(description, vec3(0, 0, 500), RotationHumanoid{});
	player.input.cast.skillID = SkillID(180060002);
	bool resumedFall = false;
	bool sawGraph = false;
	f32 graphHeight = 0.f;
	for(int i = 1; i <= 90; ++i) {
		const f32 before = player.body->GetWorldPos().z;
		scene.Step(u32(i * 17));
		if(player.movement.forcedMove) {
			if(!sawGraph) {
				sawGraph = true;
				graphHeight = before;
			}
			REQUIRE(Near(player.body->GetWorldPos().z, graphHeight, 0.005f));
			REQUIRE(!player.body->grounded && player.body->vel.z < 0.f);
		}
		else if(sawGraph) {
			REQUIRE(player.body->GetWorldPos().z < graphHeight);
			resumedFall = true;
			break;
		}
	}
	REQUIRE(sawGraph && resumedFall);
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
	update.nSpeed = 600;
	update.p3nDir = { 300, 0 };
	REQUIRE(ValidatePacket<Cl::CN_GameUpdatePosition>(&update, sizeof(update)));
	update.p3nDir = { 321, 0 };
	REQUIRE(!ValidatePacket<Cl::CN_GameUpdatePosition>(&update, sizeof(update)));
	update.p3nDir = { std::numeric_limits<float>::infinity(), 0 };
	REQUIRE(!ValidatePacket<Cl::CN_GameUpdatePosition>(&update, sizeof(update)));

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
	pos.speed = 600;
	pos.moveDir = { 0, 300 };
	std::memcpy(castData + prefixSize, &pos, sizeof(pos));
	REQUIRE(ValidatePacket<Cl::CQ_PlayerCastSkill>(castData, sizeof(castData)));
	pos.moveDir = { 0, 321 };
	std::memcpy(castData + prefixSize, &pos, sizeof(pos));
	REQUIRE(!ValidatePacket<Cl::CQ_PlayerCastSkill>(castData, sizeof(castData)));
	pos.moveDir = { 0, std::numeric_limits<float>::quiet_NaN() };
	std::memcpy(castData + prefixSize, &pos, sizeof(pos));
	REQUIRE(!ValidatePacket<Cl::CQ_PlayerCastSkill>(castData, sizeof(castData)));
	return true;
}

bool TimestampedMoveStop()
{
	WorldScene scene;
	auto& player = scene.Player(ClientHandle(1));
	World::Player::Input baseline{};
	REQUIRE(scene.world.QueuePlayerInput(player, player.Main().UID, World::InputKind::Movement, baseline, 0.0, true));
	for(u32 tick = 1; tick <= 12; ++tick) scene.Step(tick * 17);
	REQUIRE(Near(player.body->GetWorldPos().x, 0.0f, 0.005f));
	World::Player::Input movement{};
	movement.moveDir = vec2(1, 0);
	movement.speed = 600;
	movement.clientTime = 0.100f;
	movement.moveTo = vec3(100000, 100000, 100000);
	REQUIRE(scene.world.QueuePlayerInput(player, player.Main().UID, World::InputKind::Movement, movement, 0.220, true));
	scene.Step(221);
	for(u32 tick = 14; tick < 20; ++tick) scene.Step(tick * 17);
	movement.moveDir = vec2(0);
	movement.clientTime = 0.200f;
	REQUIRE(scene.world.QueuePlayerInput(player, player.Main().UID, World::InputKind::Movement, movement, 0.340, true));
	scene.Step(340);
	REQUIRE(Near(player.body->GetWorldPos().x, 60.0f, 0.01f));
	scene.Step(357);
	REQUIRE(Near(player.body->GetWorldPos().x, 60.0f, 0.01f));
	return true;
}

bool TimestampedWalkDuration()
{
	for(f32 start: { 0.00238418579f, 0.0105432f, 0.0166701f }) {
		WorldScene scene;
		auto& player = scene.Player(ClientHandle(1));
		World::Player::Input input{};
		REQUIRE(scene.world.QueuePlayerInput(player, player.Main().UID, World::InputKind::Movement, input, 0.0, true));
		for(u32 tick = 1; tick <= 6; ++tick) scene.Step(f64(tick) * UPDATE_RATE * 1000.0);
		input.moveDir = vec2(0.70710665f, 0.707106829f);
		input.speed = 595.200012f;
		input.clientTime = start;
		input.moveTo = vec3(100000, 100000, 100000);
		REQUIRE(scene.world.QueuePlayerInput(player, player.Main().UID, World::InputKind::Movement, input, 0.110, true));
		for(u32 tick = 7; tick <= 61; ++tick) scene.Step(f64(tick) * UPDATE_RATE * 1000.0);
		const vec2 direction = input.moveDir;
		const f32 stop = start + 0.899992943f;
		input.moveDir = vec2(0);
		input.clientTime = stop;
		REQUIRE(scene.world.QueuePlayerInput(player, player.Main().UID, World::InputKind::Movement, input, 1.030, true));
		scene.Step(62.0 * UPDATE_RATE * 1000.0);
		const vec3 expected(direction * f32(f64(stop - start) * input.speed), 0);
		const vec3 stopped = player.body->GetWorldPos();
		std::printf("TIMED_WALK start=%.9g stop=%.9g actual=%.6f expected=%.6f\n", start, stop, glm::length(vec2(stopped)), glm::length(vec2(expected)));
		REQUIRE(Near(stopped, expected, 0.01f));
		scene.Step(63.0 * UPDATE_RATE * 1000.0);
		scene.Step(64.0 * UPDATE_RATE * 1000.0);
		REQUIRE(Near(player.body->GetWorldPos(), stopped, 0.005f));
	}
	return true;
}

bool ClockEnvelopeReplay()
{
	WorldScene scene;
	auto& player = scene.Player(ClientHandle(1));
	for(u32 tick = 1; tick <= 11; ++tick) scene.Step(tick * 17);
	World::Player::Input movement{};
	movement.moveDir = vec2(1, 0);
	movement.speed = 600;
	movement.clientTime = 0.100f;
	REQUIRE(scene.world.QueuePlayerInput(player, player.Main().UID, World::InputKind::Movement, movement, 0.200, true));
	scene.Step(204);
	World::Player::Input invalidCast = movement;
	invalidCast.clientTime = 0.200f;
	invalidCast.cast.skillID = SkillID::INVALID;
	REQUIRE(!scene.world.QueuePlayerInput(player, player.Main().UID, World::InputKind::Cast, invalidCast, 0.210, true));
	scene.Step(221);
	movement.moveDir = vec2(0);
	movement.clientTime = 0.200f;
	REQUIRE(scene.world.QueuePlayerInput(player, player.Main().UID, World::InputKind::Movement, movement, 0.230, true));
	scene.Step(238);
	REQUIRE(Near(player.body->GetWorldPos().x, 60.0f, 0.01f));
	scene.Step(255);
	REQUIRE(Near(player.body->GetWorldPos().x, 60.0f, 0.01f));
	return true;
}

bool LateCastOwnerIsolation()
{
	WorldScene scene;
	std::unique_ptr<WorldScene> reference{new WorldScene()};
	auto& referencePlayer = reference->Player(ClientHandle(1));
	auto& player = scene.Player(ClientHandle(1));
	World::PlayerDescription otherDescription{};
	otherDescription.clientHd = ClientHandle::INVALID;
	otherDescription.masters = { ClassType(6), ClassType(5) };
	auto& other = scene.world.CreatePlayer(otherDescription, vec3(1000, 1000, 0), RotationHumanoid{});
	Settle(scene.world.physics);
	other.input.cast.skillID = SkillID(180060040);
	World::Player::Input baseline{};
	REQUIRE(scene.world.QueuePlayerInput(player, player.Main().UID, World::InputKind::Movement, baseline, 0.0, true));
	REQUIRE(reference->world.QueuePlayerInput(referencePlayer, referencePlayer.Main().UID,
		World::InputKind::Movement, baseline, 0.0, true));
	World::Player::Input cast{};
	cast.moveDir = vec2(1, 0);
	cast.clientTime = 0.100f;
	cast.cast.skillID = SkillID(180060002);
	cast.cast.clientTime = cast.clientTime;
	REQUIRE(reference->world.QueuePlayerInput(referencePlayer, referencePlayer.Main().UID,
		World::InputKind::Cast, cast, 0.101, true));
	for(u32 tick = 1; tick <= 12; ++tick) {
		scene.Step(tick * 17);
		reference->Step(tick * 17);
	}
	REQUIRE(scene.world.QueuePlayerInput(player, player.Main().UID, World::InputKind::Cast, cast, 0.220, true));
	scene.Step(221);
	reference->Step(221);
	const World::SkillProgram* ownerProgram = nullptr;
	const World::SkillProgram* otherProgram = nullptr;
	for(const auto& program: scene.world.skillProgramList) {
		if(program.casterUID == player.Main().UID) ownerProgram = &program;
		if(program.casterUID == other.Main().UID) otherProgram = &program;
	}
	REQUIRE(ownerProgram && otherProgram && ownerProgram->horizontalVariant);
	REQUIRE(Near(player.body->GetWorldPos(), referencePlayer.body->GetWorldPos(), 0.01f));
	REQUIRE(Near(other.body->GetWorldPos(), vec3(1000, 1000, 0), 0.01f));
	REQUIRE(scene.replica->frameCur->skillCastList.size() == 1 && scene.replica->frameCur->skillExecList.size() == 1);
	for(u32 tick = 14; tick < 20; ++tick) {
		scene.Step(tick * 17);
		reference->Step(tick * 17);
	}
	baseline.clientTime = 0.050f;
	REQUIRE(scene.world.QueuePlayerInput(player, player.Main().UID, World::InputKind::Movement, baseline, 0.340, true));
	scene.Step(340);
	reference->Step(340);
	REQUIRE(Near(player.body->GetWorldPos(), referencePlayer.body->GetWorldPos(), 0.01f));
	REQUIRE(scene.replica->frameCur->skillCastList.empty() && scene.replica->frameCur->skillExecList.empty());
	const auto* replicated = scene.replica->frameCur->FindMaster(player.Main().UID);
	REQUIRE(replicated && replicated->actionState == ActionStateID::INVALID);
	return true;
}

bool OutOfOrderInputBounds()
{
	WorldScene scene;
	auto& player = scene.Player(ClientHandle(1));
	World::Player::Input movement{};
	REQUIRE(scene.world.QueuePlayerInput(player, player.Main().UID, World::InputKind::Movement, movement, 0.0, true));
	for(u32 tick = 1; tick <= 19; ++tick) scene.Step(tick * 17);
	movement.moveDir = vec2(1, 0);
	movement.speed = 600;
	movement.clientTime = 0.100f;
	REQUIRE(scene.world.QueuePlayerInput(player, player.Main().UID, World::InputKind::Movement, movement, 0.340, true));
	movement.clientTime = 0.300f;
	REQUIRE(scene.world.QueuePlayerInput(player, player.Main().UID, World::InputKind::Movement, movement, 0.340, true));
	movement.moveDir = vec2(0);
	movement.clientTime = 0.200f;
	REQUIRE(scene.world.QueuePlayerInput(player, player.Main().UID, World::InputKind::Movement, movement, 0.340, true));
	scene.Step(340);
	const f64 elapsed = f64(0.200f) - f64(0.100f) + 0.340 - f64(0.300f);
	REQUIRE(Near(player.body->GetWorldPos().x, f32(movement.speed * elapsed), 0.01f));
	const u32 endTick = World::InputHistoryFrames + 26;
	for(u32 tick = 21; tick <= endTick; ++tick) scene.Step(tick * 17);
	const f32 before = player.body->GetWorldPos().x;
	const f64 receivedTime = f64(endTick * 17) / 1000.0;
	REQUIRE(!scene.world.QueuePlayerInput(player, player.Main().UID, World::InputKind::Movement, movement, receivedTime, true));
	movement.clientTime = 1000;
	REQUIRE(!scene.world.QueuePlayerInput(player, player.Main().UID, World::InputKind::Movement, movement, receivedTime, true));
	scene.Step((endTick + 1) * 17);
	REQUIRE(Near(player.body->GetWorldPos().x - before, movement.speed * 0.017f, 0.01f));
	return true;
}

bool HistoryEdgeStop()
{
	WorldScene scene;
	auto& player = scene.Player(ClientHandle(1));
	World::Player::Input input{};
	REQUIRE(scene.world.QueuePlayerInput(player, player.Main().UID, World::InputKind::Movement, input, 0.200, true));
	for(u32 tick = 1; tick <= 27; ++tick) scene.Step(tick * 17);
	input.moveDir = vec2(1, 0);
	input.speed = 600;
	input.clientTime = 0.270f;
	REQUIRE(scene.world.QueuePlayerInput(player, player.Main().UID, World::InputKind::Movement, input, 0.470, true));
	const u32 endTick = World::InputHistoryFrames + 26;
	for(u32 tick = 28; tick <= endTick; ++tick) scene.Step(tick * 17);
	const vec3 beforeStop = player.body->GetWorldPos();
	input.moveDir = vec2(0);
	const f64 receivedTime = f64(endTick * 17) / 1000.0;
	input.clientTime = f32(receivedTime - 0.100);
	REQUIRE(scene.world.QueuePlayerInput(player, player.Main().UID, World::InputKind::Movement, input, receivedTime, true));
	scene.Step((endTick + 1) * 17);
	REQUIRE(Near(player.body->GetWorldPos().x, beforeStop.x, 0.01f) && Near(player.body->GetWorldPos().y, beforeStop.y, 0.01f));
	scene.Step((endTick + 2) * 17);
	REQUIRE(Near(player.body->GetWorldPos().x, beforeStop.x, 0.01f) && Near(player.body->GetWorldPos().y, beforeStop.y, 0.01f));
	return true;
}

bool TagDomainReplay()
{
	WorldScene scene;
	auto& player = scene.Player(ClientHandle(1));
	const ActorUID first = player.Main().UID;
	World::Player::Input input{};
	REQUIRE(scene.world.QueuePlayerInput(player, first, World::InputKind::Movement, input, 0.0, true));
	for(u32 tick = 1; tick <= 12; ++tick) scene.Step(tick * 17);
	input.moveDir = vec2(1, 0);
	input.speed = 600;
	input.clientTime = 0.100f;
	REQUIRE(scene.world.QueuePlayerInput(player, first, World::InputKind::Movement, input, 0.220, true));
	scene.Step(221);
	REQUIRE(scene.world.QueuePlayerInput(player, first, World::InputKind::Tag, {}, 0.230, false));
	scene.Step(238);
	const ActorUID second = player.Main().UID;
	REQUIRE(second != first);
	input.moveDir = vec2(0);
	input.clientTime = 0.200f;
	REQUIRE(scene.world.QueuePlayerInput(player, first, World::InputKind::Movement, input, 0.250, true));
	scene.Step(255);
	REQUIRE(player.Main().UID == second && Near(player.body->GetWorldPos(), vec3(60, 0, 0), 0.01f));
	const vec3 taggedPosition = player.body->GetWorldPos();
	input.moveDir = vec2(1, 0);
	input.clientTime = 0.016f;
	REQUIRE(scene.world.QueuePlayerInput(player, second, World::InputKind::Movement, input, 0.260, true));
	input.clientTime = 0.240f;
	REQUIRE(!scene.world.QueuePlayerInput(player, first, World::InputKind::Movement, input, 0.260, true));
	scene.Step(272);
	REQUIRE(player.Main().UID == second);
	REQUIRE(Near(player.body->GetWorldPos() - taggedPosition, vec3(input.speed * (0.272f - 0.260f), 0, 0), 0.01f));
	return true;
}

bool ResumedMasterClock()
{
	WorldScene scene;
	auto& player = scene.Player(ClientHandle(1));
	const ActorUID first = player.Main().UID;
	World::Player::Input input{};
	input.speed = 600;
	input.clientTime = 0.100f;
	REQUIRE(scene.world.QueuePlayerInput(player, first, World::InputKind::Movement, input, 0.200, true));
	scene.Step(204);
	REQUIRE(scene.world.QueuePlayerInput(player, first, World::InputKind::Tag, {}, 0.230, false));
	scene.Step(238);
	const ActorUID second = player.Main().UID;
	input.clientTime = 0.010f;
	REQUIRE(scene.world.QueuePlayerInput(player, second, World::InputKind::Movement, input, 0.250, true));
	scene.Step(255);
	REQUIRE(scene.world.QueuePlayerInput(player, second, World::InputKind::Tag, {}, 1.500, false));
	scene.Step(1500);
	REQUIRE(player.Main().UID == first && Near(player.body->GetWorldPos(), vec3(0), 0.01f));
	input.clientTime = 0.200f;
	input.moveDir = vec2(1, 0);
	REQUIRE(scene.world.QueuePlayerInput(player, first, World::InputKind::Movement, input, 1.510, true));
	scene.Step(1517);
	const f32 resumedPosition = player.body->GetWorldPos().x;
	REQUIRE(resumedPosition > 0.0f && resumedPosition < input.speed * 0.017f);
	input.clientTime = 0.090f;
	input.moveDir = vec2(0);
	REQUIRE(!scene.world.QueuePlayerInput(player, first, World::InputKind::Movement, input, 1.520, true));
	scene.Step(1534);
	REQUIRE(player.Main().UID == first);
	REQUIRE(Near(player.body->GetWorldPos().x - resumedPosition, input.speed * 0.017f, 0.01f));
	return true;
}

bool StartupAccumulatedClock()
{
	WorldScene scene;
	auto& player = scene.Player(ClientHandle(1));
	World::Player::Input input{};
	input.speed = 600;
	input.clientTime = 1.35568047f;
	REQUIRE(scene.world.QueuePlayerInput(player, player.Main().UID, World::InputKind::Movement, input, 0.200, true));
	scene.Step(204);
	REQUIRE(Near(player.body->GetWorldPos(), vec3(0), 0.01f));
	input.clientTime = 1.550f;
	input.moveDir = vec2(1, 0);
	REQUIRE(scene.world.QueuePlayerInput(player, player.Main().UID, World::InputKind::Movement, input, 0.400, true));
	scene.Step(408);
	const f64 activeTime = 0.408 - (0.200 + f64(1.550f) - f64(1.35568047f));
	REQUIRE(Near(player.body->GetWorldPos().x, f32(input.speed * activeTime), 0.01f));
	const f32 beforeRejected = player.body->GetWorldPos().x;
	input.clientTime = 1000.0f;
	input.moveDir = vec2(0);
	REQUIRE(!scene.world.QueuePlayerInput(player, player.Main().UID, World::InputKind::Movement, input, 0.420, true));
	scene.Step(425);
	REQUIRE(Near(player.body->GetWorldPos().x - beforeRejected, input.speed * 0.017f, 0.01f));
	return true;
}

bool PhysicsHistoryRestore()
{
	Scene scene;
	Floor(scene.physics);
	auto* body = scene.physics.CreateDynamicBody(100, 70, vec3(0));
	Settle(scene.physics);
	scene.physics.InstallJumpGraph(body, GetGameXmlContent().GetJumpMotion(ClassType(6), JumpDirection::Stand));
	body->moveDir = vec2(0.7071067f, 0.7071068f);
	body->moveSpeed = 595.2f;
	for(int i = 0; i < 8; ++i) scene.physics.Step();
	const auto state = scene.physics.CaptureBodyState(*body);
	for(int i = 0; i < 12; ++i) scene.physics.StepBody(*body, body->moveDir * (body->moveSpeed * Dt), Dt);
	const vec3 expected = body->GetWorldPos();
	scene.physics.ResizeDynamicBody(body, 60, 100);
	scene.physics.RestoreBodyState(*body, state);
	for(int i = 0; i < 12; ++i) scene.physics.StepBody(*body, body->moveDir * (body->moveSpeed * Dt), Dt);
	REQUIRE(Near(body->GetWorldPos(), expected, 0.005f));
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
		{"native SDK ceiling, skipped frame and bounded rigid catch-up", RigidSdkStepBoundaries},
		{"float-position movement requests", FloatPositionRequests},
		{"step climb and block", Steps},
		{"segmented multi-step traversal and zero-motion state", SegmentedTraversal},
		{"slope limit", Slopes},
		{"ceiling and feet-preserving resize", CeilingAndResize},
		{"query state restoration", QueryRestoration},
		{"controller-PC filtering", ControllerFiltering},
		{"loaded ordinary jump profiles", JumpProfiles},
		{"rounded-clock jump remainder before gravity", RoundedClockJumpTail},
		{"jump request entry and first-playing phases", JumpRequestEntryPhases},
		{"match physics independent of dedicated lane uptime", MatchClockIgnoresLaneUptime},
		{"packet direction and zero-direction stop", HumanWalkingDirection},
		{"gate close/open collision lifecycle", GateCollisionLifecycle},
		{"clip-completed reverse graph activation and terminal clamp", DelayedGraphLifecycle},
		{"first-clip graph activation and walking ownership", FirstClipGraphLifecycle},
		{"omitted animation index first-playing graph ownership", OmittedIndexGraphLifecycle},
		{"authored WARP clears held direction and resumes walking after completion", WarpDirectionAndLifecycle},
		{"blocked WARP motor ownership, cancellation and restored destination", WarpBlockedDestinationRestore},
		{"forced WARP resolves successive climbable steps", WarpSegmentedTraversal},
		{"late input replay through an active WARP destination", WarpDestinationInputReplay},
		{"airborne horizontal graph and fall resumption", AirborneHorizontalGraph},
		{"hostile movement packet rejection", HostileMovementPackets},
		{"timestamped movement and STOP replay", TimestampedMoveStop},
		{"timestamped walk duration independent of tick phase", TimestampedWalkDuration},
		{"server history restore across jump and resize", PhysicsHistoryRestore},
		{"spawn clock envelope and rejected-cast atomicity", ClockEnvelopeReplay},
		{"late cast replay and other-owner isolation", LateCastOwnerIsolation},
		{"out-of-order timestamps and bounded history", OutOfOrderInputBounds},
		{"tag clock domains and old-master replay", TagDomainReplay},
		{"clock refinement at history edge preserves STOP", HistoryEdgeStop},
		{"resumed master clock without guessed paused timestamp", ResumedMasterClock},
		{"startup accumulated client clock", StartupAccumulatedClock},
	};
	int failed = 0;
	for(const auto& test: tests) {
		std::printf("RUN %s\n", test.name);
		const bool passed = test.run();
		std::printf("%s %s\n", passed ? "PASS" : "FAIL", test.name);
		if(!passed) ++failed;
	}
	PhysContext().Shutdown();
	std::printf("physics_tests: %d failed\n", failed);
	return failed ? 1 : 0;
}
