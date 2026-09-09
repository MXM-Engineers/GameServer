#include "physics.h"
#include <mxm/game_content.h>
#include <PxPhysicsAPI.h> // lazy but oh well
#include <pvd/PxPvd.h>

#define PVD_HOST "127.0.0.1"


inline PxTolerancesScale TolerancesScale()
{
	PxTolerancesScale s;
	s.length = 100.f;
	s.speed = 100.f;
	return s;
}

class PhysxReadBuffer: public PxInputStream
{
	const void* data;
	const u32 size;
	i32 cur = 0;

public:
	PhysxReadBuffer(void* data_, u32 size_):
		data(data_),
		size(size_)
	{

	}

	virtual uint32_t read(void* dest, uint32_t count) override
	{
		ASSERT(cur + count <= size);
		memmove(dest, (u8*)data + cur, count);
		cur += count;
		return count;
	}
};

static constexpr u32 PC_COLLISION_MASK = 0x1c7f06;
static const PxFilterData g_pcQueryFilterData(PC_COLLISION_MASK, 0, 0, 0);

struct CCT_CollisionFilterCallback: PxControllerFilterCallback
{
	virtual bool filter(const PxController& a, const PxController& b) override
	{
		const auto* filterA = static_cast<const PxFilterData*>(a.getUserData());
		const auto* filterB = static_cast<const PxFilterData*>(b.getUserData());
		ASSERT(filterA && filterB);
		return (filterA->word1 & filterB->word0) != 0 && (filterB->word1 & filterA->word0) != 0;
	}
};

static CCT_CollisionFilterCallback g_cctCollisionFilterCallback;

bool PhysicsContext::Init()
{
	foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocatorCallback, errorCallback);
	if(!foundation) {
		LOG("[PhysX] ERROR: PxCreateFoundation failed");
		return false;
	}

	bool recordMemoryAllocations = true;

	pvd = PxCreatePvd(*foundation);
	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
	bool connected = pvd->connect(*transport, PxPvdInstrumentationFlag::eALL);
	if(!connected) {
		LOG("[PhysX] WARNING: failed to connect to PVD Client");
	}

	physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, TolerancesScale(), recordMemoryAllocations, pvd);
	if(!physics) {
		LOG("[PhysX] ERROR: PxCreatePhysics failed");
		return false;
	}

	// TODO: does this work to make the simulation be executed on the same thread??
	dispatcher = PxDefaultCpuDispatcherCreate(0);
	if(!dispatcher) {
		LOG("[PhysX] ERROR: PxDefaultCpuDispatcherCreate failed");
		return false;
	}

	// collision meshes
	matMapSurface = physics->createMaterial(1.0f, 1.0f, 0.0f);

	const GameXmlContent& gc = GetGameXmlContent();
	if(!LoadCollisionMeshes(gc.filePvpDeathmatch01Collision)) return false;
	if(!LoadCollisionMeshes(gc.filePvpDeathmatch01CollisionWalls)) return false;
	if(!LoadCollisionMeshes(gc.filePvpDeathNmWall04)) return false;

	LOG("PhysicsContext initialised");
	return true;
}

void PhysicsContext::Shutdown()
{
	physics->release();

	PxPvdTransport* transport = pvd->getTransport();
	pvd->release();
	transport->release();

	foundation->release();
	LOG("PhysicsContext shutdown");
}

bool PhysicsContext::LoadCollisionMeshes(const FileBuffer& file)
{
	ConstBuffer buff(file.data, file.size);

	const u32 magic = buff.Read<u32>();
	if(magic != 0x58594850) { // PHYSX
		LOG("[PhysX] ERROR(LoadCollisionMeshes): file format not recognised");
		return false;
	}

	const u16 version = buff.Read<u16>();
	if(version != 1) {
		LOG("[PhysX] ERROR(LoadCollisionMeshes): version not supported");
		return false;
	}

	const u16 meshCount = buff.Read<u16>();
	for(u16 i = 0; i < meshCount; i++) {
		const i32 nameLen = buff.Read<i32>();
		const char* name = (char*)buff.ReadRaw(nameLen);

		const u32 meshDataSize = buff.Read<u32>();
		void* meshData = buff.ReadRaw(meshDataSize);

		PhysxReadBuffer readBuff(meshData, meshDataSize);
		PxTriangleMesh* tri = physics->createTriangleMesh(readBuff);
		if(!tri) {
			LOG("[PhysX] ERROR(LoadCollisionMeshes): createTriangleMesh failed (meshIdx=%d)", i);
			return false;
		}

		const size_t key = eastl::hash<const char*>{}(FixedStr64(name, nameLen).data());
		ASSERT(triangleMeshMap.find(key) == triangleMeshMap.end());
		triangleMeshMap.emplace(key, tri);

		LOG("[PhysX] '%.*s' loaded", nameLen, name);
	}

	return true;
}

void PhysicsContext::CreateScene(PhysicsScene* out)
{
	PxSceneDesc desc{TolerancesScale()};
	desc.cpuDispatcher = dispatcher;
	desc.filterShader = PxDefaultSimulationFilterShader;
	desc.gravity = PxVec3(0.0f, 0.0f, GetGameXmlContent().creatureGravity);

	PxScene* scene;
	{ LOCK_MUTEX(mutexSceneCreate);
		scene = physics->createScene(desc);
		if(!scene) {
			LOG("[PhysX] ERROR: Creating scene failed");
		}
		ASSERT(scene);

		out->scene = scene;
        PxPvdSceneClient* pvdClient = scene->getScenePvdClient();
		if(pvdClient) {
			pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
			pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
			pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
		}

		out->controllerMngr = PxCreateControllerManager(*scene);
		if(!out->controllerMngr) {
			LOG("[PhysX] ERROR: Creating controller manager failed");
		}
		ASSERT(out->controllerMngr);
	}
}

void PhysicsScene::Step()
{
	ProfileFunction();

	const f32 dt = (f32)UPDATE_RATE;
	const f32 gravity = scene->getGravity().z;
	foreach(c, colliderList) {
		c->vel.z += gravity * dt;
		vec3 displacement = c->vel * dt;
		if(c->jumpMotion) {
			c->jumpElapsed = eastl::min(c->jumpElapsed + dt, c->jumpMotion->duration);
			const f32 height = c->jumpMotion->Sample(c->jumpElapsed);
			displacement.z = height - c->jumpHeight;
			c->jumpHeight = height;
			if(c->jumpElapsed >= c->jumpMotion->duration) {
				c->jumpMotion = nullptr;
			}
		}
		Move(c, displacement, dt);
	}

	// we don't need to actually *simulate* anything?
#if 1
	// FIXME: find out how to simulate on the same thread
	scene->simulate((f32)UPDATE_RATE);
	// here we do nothing but wait...
	scene->fetchResults(true);
#endif
}

void PhysicsScene::Destroy()
{
	if(controllerMngr) {
		controllerMngr->purgeControllers();
		controllerMngr->release();
		controllerMngr = nullptr;
	}
	colliderList.clear();
	if(scene) {
		const PxActorTypeFlags types = PxActorTypeFlag::eRIGID_STATIC | PxActorTypeFlag::eRIGID_DYNAMIC;
		PxActor* actors[32];
		while(scene->getNbActors(types) != 0) {
			const PxU32 count = scene->getActors(types, actors, ARRAY_COUNT(actors));
			ASSERT(count != 0);
			for(PxU32 i = 0; i < count; i++) actors[i]->release();
		}
		scene->release();
		scene = nullptr;
	}
}

void PhysicsScene::CreateStaticCollider(const char* meshName, const vec3& pos, const vec3& rot, PhysicsCollisionGroup group)
{
	auto& ctx = PhysContext();

	PxTriangleMeshGeometry geometry = PxTriangleMeshGeometry(ctx.triangleMeshMap.at(eastl::hash<const char*>{}(meshName)));

	PxShape* shape = ctx.physics->createShape(geometry, *ctx.matMapSurface);
	ASSERT(shape); // createShape failed
	ASSERT(group == PhysicsCollisionGroup::Static || group == PhysicsCollisionGroup::Fence ||
		group == PhysicsCollisionGroup::FenceAll || group == PhysicsCollisionGroup::Aim);
	shape->setQueryFilterData(PxFilterData(1u << static_cast<u32>(group), 0, 0, 0));

	PxRigidStatic* ground = ctx.physics->createRigidStatic(PxTransform{PxIdentity});
	ASSERT(ground);
	const bool attached = ground->attachShape(*shape);
	ASSERT(attached);
	shape->release();

	glm::quat quat(vec3(rot.x, rot.y, -rot.z)); // quaternion from euler angles
	ground->setGlobalPose(PxTransform(PxVec3(pos.x, pos.y, pos.z), PxQuat(quat.x, quat.y, quat.z, quat.w)));
	scene->addActor(*ground);
}

PhysicsDynamicBody* PhysicsScene::CreateDynamicBody(f32 radius, f32 height, const vec3& pos)
{
	auto& ctx = PhysContext();

	ASSERT(colliderList.size() < colliderList.max_size());
	colliderList.emplace_back();
	PhysicsDynamicBody& collider = colliderList.back();
	collider.height = height;
	collider.radius = radius;
	collider.collisionFilterData = PxFilterData(1u << static_cast<u32>(PhysicsCollisionGroup::ControllerPc), PC_COLLISION_MASK, 0, 0);

	PxCapsuleControllerDesc desc;
	desc.height = height;
	desc.radius = radius;
	desc.contactOffset = 10.0f;
	desc.stepOffset = 70.0f;
	desc.slopeLimit = cosf(glm::radians(65.0f));
	desc.climbingMode = PxCapsuleClimbingMode::eCONSTRAINED;
	desc.upDirection = PxVec3(0.0f, 0.0f, 1.0f);
	desc.position = PxExtendedVec3(pos.x, pos.y, PxExtended(pos.z) + height * 0.5f + radius + desc.contactOffset);
	desc.material = ctx.matMapSurface;
	desc.userData = &collider.collisionFilterData;
	ASSERT(desc.isValid());

	collider.collider = static_cast<PxCapsuleController*>(controllerMngr->createController(desc));
	ASSERT(collider.collider);
	PxRigidDynamic* actor = collider.collider->getActor();
	ASSERT(actor && actor->getNbShapes() == 1);
	PxShape* shape;
	const PxU32 shapeCount = actor->getShapes(&shape, 1);
	ASSERT(shapeCount == 1);
	shape->setQueryFilterData(PxFilterData(collider.collisionFilterData.word0, 0, 0, 0));
	return &collider;
}

void PhysicsScene::ResizeDynamicBody(PhysicsDynamicBody* body, f32 radius, f32 height)
{
	if(body->radius == radius && body->height == height) return;
	ASSERT(radius > 0.0f && height > 0.0f && body->collider->getStepOffset() <= height + radius * 2.0f);
	const PxExtendedVec3 feet = body->collider->getFootPosition();
	if(body->radius != radius) {
		const bool resized = body->collider->setRadius(radius);
		ASSERT(resized);
		body->radius = radius;
	}
	if(body->height != height) {
		const bool resized = body->collider->setHeight(height);
		ASSERT(resized);
		body->height = height;
	}
	const bool positioned = body->collider->setFootPosition(feet);
	ASSERT(positioned);
}

bool PhysicsScene::StartJump(PhysicsDynamicBody* body, const JumpMotion& motion)
{
	ASSERT(motion.duration > 0.0f);
	if(!body->grounded || body->jumpMotion) return false;
	body->jumpMotion = &motion;
	body->jumpElapsed = 0.0f;
	body->jumpHeight = 0.0f;
	body->vel.z = 0.0f;
	body->grounded = false;
	return true;
}

vec3 PhysicsScene::Move(PhysicsDynamicBody* body, const vec3& disp, f32 time)
{
	PxControllerFilters filter(&g_pcQueryFilterData, nullptr, &g_cctCollisionFilterCallback);
	filter.mFilterFlags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC;
	const PxControllerCollisionFlags collisionFlags = body->collider->move(PxVec3(disp.x, disp.y, disp.z), 0, time, filter);
	body->grounded = bool(collisionFlags & PxControllerCollisionFlag::eCOLLISION_DOWN);
	if((body->grounded && body->vel.z < 0.0f) ||
		((collisionFlags & PxControllerCollisionFlag::eCOLLISION_UP) && body->vel.z > 0.0f)) {
		body->vel.z = 0.0f;
	}
	return body->GetWorldPos();
}

vec3 PhysicsScene::FindMovePos(PhysicsDynamicBody* body, const vec3& disp, f32 time)
{
	const PxExtendedVec3 start = body->collider->getPosition();
	const bool grounded = body->grounded;
	const vec3 velocity = body->vel;
	const vec3 end = Move(body, disp, time);
	const bool positioned = body->collider->setPosition(start);
	ASSERT(positioned);
	body->grounded = grounded;
	body->vel = velocity;
	return end;
}

static PhysicsContext* g_Context;

bool PhysicsInit()
{
	static PhysicsContext context;
	g_Context = &context;
	return context.Init();
}

PhysicsContext& PhysContext()
{
	return *g_Context;
}
