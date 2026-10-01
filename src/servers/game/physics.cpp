#include "physics.h"
#include <mxm/game_content.h>
#include <NxStream.h>
#include <cmath>
#include <cstring>

class NxMemoryReadStream: public NxStream
{
	const void* data;
	const u32 size;
	mutable u32 cur = 0;

public:
	NxMemoryReadStream(void* data_, u32 size_):
		data(data_),
		size(size_)
	{

	}

	virtual NxU8 readByte() const override
	{
		NxU8 value;
		readBuffer(&value, sizeof(value));
		return value;
	}

	virtual NxU16 readWord() const override
	{
		NxU16 value;
		readBuffer(&value, sizeof(value));
		return value;
	}

	virtual NxU32 readDword() const override
	{
		NxU32 value;
		readBuffer(&value, sizeof(value));
		return value;
	}

	virtual NxF32 readFloat() const override
	{
		NxF32 value;
		readBuffer(&value, sizeof(value));
		return value;
	}

	virtual NxF64 readDouble() const override
	{
		NxF64 value;
		readBuffer(&value, sizeof(value));
		return value;
	}

	virtual void readBuffer(void* dest, NxU32 count) const override
	{
		ASSERT(cur + count <= size);
		memcpy(dest, (u8*)data + cur, count);
		cur += count;
	}

	virtual NxStream& storeByte(NxU8) override
	{
		ASSERT(false);
		return *this;
	}

	virtual NxStream& storeWord(NxU16) override
	{
		ASSERT(false);
		return *this;
	}

	virtual NxStream& storeDword(NxU32) override
	{
		ASSERT(false);
		return *this;
	}

	virtual NxStream& storeFloat(NxF32) override
	{
		ASSERT(false);
		return *this;
	}

	virtual NxStream& storeDouble(NxF64) override
	{
		ASSERT(false);
		return *this;
	}

	virtual NxStream& storeBuffer(const void*, NxU32) override
	{
		ASSERT(false);
		return *this;
	}
};

static constexpr u32 PC_COLLISION_MASK = 0x1c7f06;
static constexpr f32 CCT_SKIN_WIDTH = 10.0f;
static constexpr f32 CCT_STEP_OFFSET = 70.0f;

bool PhysicsContext::Init()
{
	NxSDKCreateError error = NXCE_NO_ERROR;
	physics = NxCreatePhysicsSDK(NX_PHYSICS_SDK_VERSION, &allocatorCallback, &outputStream, NxPhysicsSDKDesc(), &error);
	if(!physics) {
		LOG("[PhysX] ERROR: NxCreatePhysicsSDK failed (%d)", (int)error);
		return false;
	}

	const GameXmlContent& gc = GetGameXmlContent();
	if(!LoadCollisionMeshes(gc.filePvpDeathmatch01Collision)) return false;
	if(!LoadCollisionMeshes(gc.filePvpDeathmatch01CollisionWalls)) return false;
	if(!LoadCollisionMeshes(gc.filePvpDeathNmWall04)) return false;

	LOG("PhysicsContext initialised");
	return true;
}

void PhysicsContext::Shutdown()
{
	if(physics) {
		for(auto& entry: triangleMeshMap) {
			physics->releaseTriangleMesh(*entry.second);
		}
		triangleMeshMap.clear();
		NxReleasePhysicsSDK(physics);
		physics = nullptr;
	}
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

		NxMemoryReadStream readBuff(meshData, meshDataSize);
		NxTriangleMesh* tri = physics->createTriangleMesh(readBuff);
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
	NxSceneDesc desc;
	desc.simType = NX_SIMULATION_SW;
	desc.gravity = NxVec3(0.0f, 0.0f, GetGameXmlContent().creatureGravity);

	NxScene* scene;
	{ LOCK_MUTEX(mutexSceneCreate);
		scene = physics->createScene(desc);
		if(!scene) {
			LOG("[PhysX] ERROR: Creating scene failed");
		}
		ASSERT(scene);

		out->scene = scene;
		NxMaterial* material = scene->getMaterialFromIndex(0);
		ASSERT(material);
		material->setStaticFriction(1.0f);
		material->setDynamicFriction(1.0f);
		material->setRestitution(0.0f);

		out->controllerMngr = NxCreateControllerManager(&allocatorCallback);
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
	NxVec3 gravity;
	scene->getGravity(gravity);
	foreach(c, colliderList) {
		c->vel.z += gravity.z * dt;
		vec3 displacement = c->vel * dt;
		if(c->horizontalGraphActive) {
			displacement = vec3(c->horizontalGraphDisplacement, 0);
		}
		if(c->jumpMotion) {
			c->jumpElapsed = eastl::min(c->jumpElapsed + dt, c->jumpMotion->duration);
			const f32 height = c->jumpMotion->Sample(c->jumpElapsed);
			displacement.z = height - c->jumpHeight;
			c->jumpHeight = height;
			if(c->jumpElapsed >= c->jumpMotion->duration) {
				c->jumpMotion = nullptr;
			}
		}
		const vec3 position = c->GetWorldPos();
		displacement = (position + displacement) - position;
		const bool segmented = (displacement.x != 0.f || displacement.y != 0.f) &&
			std::sqrt(displacement.y * displacement.y + displacement.x * displacement.x) / dt > 2000.f;
		Move(c, displacement, dt, segmented);
		c->horizontalGraphDisplacement = vec2(0);
		c->horizontalGraphActive = false;
	}

	scene->simulate((f32)UPDATE_RATE);
	scene->flushStream();
	scene->fetchResults(NX_RIGID_BODY_FINISHED, true);
}

void PhysicsScene::Destroy()
{
	if(controllerMngr) {
		controllerMngr->purgeControllers();
		NxReleaseControllerManager(controllerMngr);
		controllerMngr = nullptr;
	}
	colliderList.clear();
	if(scene) {
		while(scene->getNbActors() != 0) {
			NxActor** actors = scene->getActors();
			ASSERT(actors && actors[0]);
			scene->releaseActor(*actors[0]);
		}
		PhysContext().physics->releaseScene(*scene);
		scene = nullptr;
	}
}

void PhysicsScene::CreateStaticCollider(const char* meshName, const vec3& pos, const vec3& rot, PhysicsCollisionGroup group)
{
	auto& ctx = PhysContext();

	ASSERT(group == PhysicsCollisionGroup::Static || group == PhysicsCollisionGroup::Fence ||
		group == PhysicsCollisionGroup::FenceAll || group == PhysicsCollisionGroup::Aim);

	NxTriangleMeshShapeDesc shapeDesc;
	shapeDesc.meshData = ctx.triangleMeshMap.at(eastl::hash<const char*>{}(meshName));
	shapeDesc.group = static_cast<NxCollisionGroup>(group);
	shapeDesc.materialIndex = 0;
	ASSERT(shapeDesc.isValid());

	NxActorDesc actorDesc;
	actorDesc.shapes.pushBack(&shapeDesc);
	glm::quat quat(vec3(rot.x, rot.y, -rot.z));
	NxQuat nxQuat;
	nxQuat.setXYZW(quat.x, quat.y, quat.z, quat.w);
	actorDesc.globalPose.t = NxVec3(pos.x, pos.y, pos.z);
	actorDesc.globalPose.M.fromQuat(nxQuat);
	ASSERT(actorDesc.isValid());

	NxActor* actor = scene->createActor(actorDesc);
	ASSERT(actor);
}

PhysicsDynamicBody* PhysicsScene::CreateDynamicBody(f32 radius, f32 height, const vec3& pos)
{
	ASSERT(colliderList.size() < colliderList.max_size());
	colliderList.emplace_back();
	PhysicsDynamicBody& collider = colliderList.back();
	collider.height = height;
	collider.radius = radius;

	NxCapsuleControllerDesc desc;
	desc.height = height;
	desc.radius = radius;
	desc.skinWidth = CCT_SKIN_WIDTH;
	desc.stepOffset = CCT_STEP_OFFSET;
	desc.slopeLimit = cosf(glm::radians(65.0f));
	desc.climbingMode = CLIMB_CONSTRAINED;
	desc.upDirection = NX_Z;
	desc.interactionFlag = NXIF_INTERACTION_USE_FILTER;
	collider.feetOffset = height * 0.5f + radius + desc.skinWidth;
	desc.position = NxExtendedVec3(pos.x, pos.y, Extended(pos.z) + collider.feetOffset);
	ASSERT(desc.isValid());

	collider.collider = static_cast<NxCapsuleController*>(controllerMngr->createController(scene, desc));
	ASSERT(collider.collider);
	NxActor* actor = collider.collider->getActor();
	ASSERT(actor && actor->getNbShapes() == 1);
	NxShape* const* shapes = actor->getShapes();
	ASSERT(shapes && shapes[0]);
	shapes[0]->setGroup(static_cast<NxCollisionGroup>(PhysicsCollisionGroup::ControllerPc));
	return &collider;
}

void PhysicsScene::ResizeDynamicBody(PhysicsDynamicBody* body, f32 radius, f32 height)
{
	if(body->radius == radius && body->height == height) return;
	ASSERT(radius > 0.0f && height > 0.0f && CCT_STEP_OFFSET <= height + radius * 2.0f);
	const NxExtendedVec3 center = body->collider->getDebugPosition();
	const NxExtendedVec3 feet(center.x, center.y, center.z - body->feetOffset);
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
	body->feetOffset = height * 0.5f + radius + CCT_SKIN_WIDTH;
	const bool positioned = body->collider->setPosition(NxExtendedVec3(feet.x, feet.y, feet.z + body->feetOffset));
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

vec3 PhysicsScene::Move(PhysicsDynamicBody* body, const vec3& disp, f32, bool segmented)
{
	if(std::fabs(disp.x) < 1.1920929e-7f && std::fabs(disp.y) < 1.1920929e-7f && std::fabs(disp.z) < 1.1920929e-7f) {
		return body->GetWorldPos();
	}
	body->collider->reportSceneChanged();
	NxU32 collisionFlags = 0;
	if(segmented) {
		f32 remaining = std::sqrt(disp.y * disp.y + disp.x * disp.x + disp.z * disp.z);
		vec3 direction(0);
		if(remaining <= 1.e-6f) {
			remaining = 0.f;
		}
		else {
			direction = disp * (1.f / remaining);
		}
		while(remaining > 50.f) {
			remaining -= 50.f;
			const vec3 step = direction * 50.f;
			body->collider->move(NxVec3(step.x, step.y, step.z), PC_COLLISION_MASK, 0.0f, collisionFlags, 0.0f);
		}
		const vec3 step = direction * remaining;
		body->collider->move(NxVec3(step.x, step.y, step.z), PC_COLLISION_MASK, 0.0f, collisionFlags, 0.0f);
	}
	else {
		body->collider->move(NxVec3(disp.x, disp.y, disp.z), PC_COLLISION_MASK, 0.0f, collisionFlags, 0.0f);
	}
	body->grounded = (collisionFlags & NXCC_COLLISION_DOWN) != 0;
	if((body->grounded && body->vel.z < 0.0f) ||
		((collisionFlags & NXCC_COLLISION_UP) && body->vel.z > 0.0f)) {
		body->vel.z = 0.0f;
	}
	return body->GetWorldPos();
}

vec3 PhysicsScene::FindMovePos(PhysicsDynamicBody* body, const vec3& disp, f32 time)
{
	const NxExtendedVec3 start = body->collider->getDebugPosition();
	const bool grounded = body->grounded;
	const vec3 velocity = body->vel;
	const vec3 position = body->GetWorldPos();
	const vec3 displacement = (position + disp) - position;
	const vec3 end = Move(body, displacement, time, false);
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
