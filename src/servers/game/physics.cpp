#include "physics.h"
#include <mxm/game_content.h>
#include <NxStream.h>
#include <NxCooking.h>
#include <NxTriangleMesh.h>
#include <NxConvexMesh.h>
#include <new>
#include <limits>
#include <cmath>
#include <cstring>
#include "velqor_trace.h"

static constexpr f32 RigidTimestep = 1.0f / 30.0f;

class NxMemoryReadStream: public NxStream
{
	const void* data;
	const u32 size;
	mutable u32 cur = 0;

public:
	NxMemoryReadStream(const void* data_, u32 size_):
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
		ASSERT(cur <= size && count <= size - cur);
		memcpy(dest, (const u8*)data + cur, count);
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

class NxMemoryWriteStream: public NxStream
{
public:
	GrowableBuffer data{1024};

	NxU8 readByte() const override { ASSERT(false); return 0; }
	NxU16 readWord() const override { ASSERT(false); return 0; }
	NxU32 readDword() const override { ASSERT(false); return 0; }
	NxF32 readFloat() const override { ASSERT(false); return 0; }
	NxF64 readDouble() const override { ASSERT(false); return 0; }
	void readBuffer(void*, NxU32) const override { ASSERT(false); }
	NxStream& storeByte(NxU8 value) override { return storeBuffer(&value, sizeof(value)); }
	NxStream& storeWord(NxU16 value) override { return storeBuffer(&value, sizeof(value)); }
	NxStream& storeDword(NxU32 value) override { return storeBuffer(&value, sizeof(value)); }
	NxStream& storeFloat(NxF32 value) override { return storeBuffer(&value, sizeof(value)); }
	NxStream& storeDouble(NxF64 value) override { return storeBuffer(&value, sizeof(value)); }
	NxStream& storeBuffer(const void* bytes, NxU32 count) override
	{
		ASSERT(count <= u32(std::numeric_limits<i32>::max() - data.size));
		if(i32(count) > data.capacity - data.size) {
			data.Reserve(MAX(data.size + i32(count), data.capacity * 2));
		}
		data.Append(bytes, i32(count));
		return *this;
	}
};

static NxMat34 CollisionTransform(const CollisionAsset::Transform& transform, f32 scale)
{
	NxMat34 result;
	result.M.setRowMajor(transform.rotation);
	result.t = NxVec3(transform.translation[0] * scale, transform.translation[1] * scale, transform.translation[2] * scale);
	return result;
}

static void ReleaseCollisionMesh(NxPhysicsSDK& sdk, const PhysicsCollisionMesh& mesh)
{
	if(mesh.triangle) sdk.releaseTriangleMesh(*mesh.triangle);
	else {
		ASSERT(mesh.convex);
		sdk.releaseConvexMesh(*mesh.convex);
	}
}

static constexpr f32 CCT_SKIN_WIDTH = 10.0f;
static constexpr f32 CCT_STEP_OFFSET = 70.0f;

static NxGroupsMask ControllerGroupsMask(const PhysicsDynamicBody& body)
{
	NxGroupsMask mask;
	mask.bits0 = 1u << static_cast<u32>(PhysicsCollisionGroup::ControllerPc);
	mask.bits1 = body.collisionTeam;
	mask.bits2 = body.collisionGroups;
	mask.bits3 = 0;
	return mask;
}

struct PhysicsControllerHitReport: NxUserControllerHitReport
{
	NxControllerAction onShapeHit(const NxControllerShapeHit& hit) override
	{
		auto* body = static_cast<PhysicsDynamicBody*>(hit.controller->getUserData());
		ASSERT(body);
		if(body->suppressContactReport) return NX_ACTION_NONE;
		const NxCollisionGroup group = hit.shape->getGroup();
		if(group == 9 || group == 11) return NX_ACTION_NONE;
		if(group != 18 && group != 19) {
			ASSERT(std::isfinite(hit.worldNormal.x) && std::isfinite(hit.worldNormal.y) && std::isfinite(hit.worldNormal.z));
			body->groundNormal = vec3(hit.worldNormal.x, hit.worldNormal.y, hit.worldNormal.z);
		}
		const f32 centerZ = f32(hit.controller->getDebugPosition().z);
		const f32 belowCenter = (centerZ - body->height * 0.5f) - 0.05f;
		if(hit.worldPos.z < Extended(belowCenter)) body->vel.z = 0.0f;
		return NX_ACTION_NONE;
	}

	NxControllerAction onControllerHit(const NxControllersHit&) override
	{
		return NX_ACTION_NONE;
	}
};

static PhysicsControllerHitReport g_ControllerHitReport;

void PhysicsDynamicBody::SetIgnoreUnitCollision(bool ignore)
{
	const u32 groups = ignore ? collisionGroups & ~0x6000u : collisionGroups | 0x6000u;
	if(groups == collisionGroups) return;
	collisionGroups = groups;
	NxActor* actor = collider->getActor();
	ASSERT(actor && actor->getNbShapes() == 1);
	NxShape* const* shapes = actor->getShapes();
	ASSERT(shapes && shapes[0]);
	shapes[0]->setGroupsMask(ControllerGroupsMask(*this));
}

bool PhysicsContext::Init()
{
	NxSDKCreateError error = NXCE_NO_ERROR;
	physics = NxCreatePhysicsSDK(NX_PHYSICS_SDK_VERSION, &allocatorCallback, &outputStream, NxPhysicsSDKDesc(), &error);
	if(!physics) {
		LOG("[PhysX] ERROR: NxCreatePhysicsSDK failed (%d)", (int)error);
		return false;
	}

	const GameXmlContent& gc = GetGameXmlContent();
	for(const auto& file: gc.collisionFiles) {
		if(!LoadCollisionAsset(file.name.data(), file.file)) return false;
	}

	LOG("PhysicsContext initialised");
	return true;
}

void PhysicsContext::Shutdown()
{
	if(physics) {
		for(const auto& scaled: scaledMeshes) ReleaseCollisionMesh(*physics, scaled.mesh);
		scaledMeshes.clear();
		for(const auto& entry: collisionAssetMap) {
			for(const auto& mesh: entry.second.meshes) ReleaseCollisionMesh(*physics, mesh);
		}
		collisionAssetMap.clear();
		NxReleasePhysicsSDK(physics);
		physics = nullptr;
	}
	LOG("PhysicsContext shutdown");
}

bool PhysicsContext::LoadCollisionAsset(const char* name, const FileBuffer& file)
{
	ConstBuffer buff(file.data, file.size);
	const auto& header = buff.Read<CollisionAsset::Header>();
	ASSERT(header.magic == 0x58594850 && header.version == CollisionAsset::VERSION);
	const size_t key = eastl::hash<const char*>{}(name);
	ASSERT(collisionAssetMap.size() < 32 && collisionAssetMap.find(key) == collisionAssetMap.end());
	auto& asset = collisionAssetMap[key];
	ASSERT(strlen(name) < asset.name.max_size());
	asset.name = name;
	asset.header = &header;
	ASSERT(header.meshCount <= asset.meshes.max_size() && header.actorCount <= asset.actors.max_size());
	ASSERT(header.actorCount != 0);
	asset.materials = reinterpret_cast<const CollisionAsset::Material*>(buff.ReadRaw(header.materialCount * sizeof(CollisionAsset::Material)));
	for(u16 i = 0; i < header.meshCount; ++i) {
		const i32 nameLen = buff.Read<i32>();
		ASSERT(nameLen >= 0 && nameLen < 64);
		buff.ReadRaw(nameLen);
		const u32 dataSize = buff.Read<u32>();
		ASSERT(dataSize >= 8 && dataSize <= u32(std::numeric_limits<i32>::max()));
		const u8* data = buff.ReadRaw(i32(dataSize));
		ASSERT(memcmp(data, "NXS", 3) == 0);
		NxMemoryReadStream stream(data, dataSize);
		auto& mesh = asset.meshes.push_back();
		if(memcmp(data + 4, "MESH", 4) == 0) mesh.triangle = physics->createTriangleMesh(stream);
		else {
			ASSERT(memcmp(data + 4, "CVXM", 4) == 0);
			mesh.convex = physics->createConvexMesh(stream);
		}
		ASSERT(mesh.triangle || mesh.convex);
	}
	for(u16 i = 0; i < header.actorCount; ++i) {
		auto& actor = asset.actors.push_back();
		actor.recipe = &buff.Read<CollisionAsset::Actor>();
		ASSERT(actor.recipe->hasBody <= 1 && actor.recipe->shapeCount == 1);
		if(actor.recipe->hasBody) actor.body = &buff.Read<CollisionAsset::Body>();
		actor.shape = &buff.Read<CollisionAsset::Shape>();
		const i32 meshIndex = actor.shape->meshIndex;
		if(actor.shape->type == NX_SHAPE_MESH || actor.shape->type == NX_SHAPE_CONVEX) {
			ASSERT(meshIndex >= 0 && meshIndex < header.meshCount);
		}
		else {
			ASSERT(meshIndex == -1 && (actor.shape->type == NX_SHAPE_BOX || actor.shape->type == NX_SHAPE_CAPSULE));
		}
	}
	ASSERT(buff.RemainingDataSize() == 0);
	LOG("[PhysX] '%s' loaded (%u meshes, %u actors)", name, header.meshCount, header.actorCount);
	return true;
}

const PhysicsCollisionMesh& PhysicsContext::GetCollisionMesh(size_t assetKey, u32 meshIndex, f32 scale)
{
	const auto& asset = collisionAssetMap.at(assetKey);
	ASSERT(meshIndex < asset.meshes.size());
	const auto& source = asset.meshes[meshIndex];
	if(scale == 1.0f) return source;
	ASSERT(std::isfinite(scale) && scale > 0.0f && source.triangle && !source.convex);
	for(const auto& entry: scaledMeshes) {
		if(entry.assetKey == assetKey && entry.meshIndex == meshIndex && entry.scale == scale) return entry.mesh;
	}
	ASSERT(scaledMeshes.size() < scaledMeshes.max_size());
	NxTriangleMeshDesc description;
	const bool saved = source.triangle->saveToDesc(description);
	ASSERT(saved && description.numVertices > 0 && description.pointStrideBytes >= sizeof(NxVec3));
	eastl::vector<NxVec3> points(description.numVertices);
	for(u32 i = 0; i < description.numVertices; ++i) {
		const auto& point = *reinterpret_cast<const NxVec3*>(static_cast<const u8*>(description.points) + i * description.pointStrideBytes);
		points[i].set(point.x * scale, point.y * scale, point.z * scale);
	}
	description.points = points.data();
	description.pointStrideBytes = sizeof(NxVec3);
	NxCookingInterface* cooking = NxGetCookingLib(NX_PHYSICS_SDK_VERSION);
	ASSERT(cooking);
	const bool initialized = cooking->NxInitCooking(&allocatorCallback, &outputStream);
	ASSERT(initialized);
	NxMemoryWriteStream cooked;
	const bool success = cooking->NxCookTriangleMesh(description, cooked);
	cooking->NxCloseCooking();
	ASSERT(success);
	NxMemoryReadStream stream(cooked.data.data, cooked.data.size);
	NxTriangleMesh* mesh = physics->createTriangleMesh(stream);
	ASSERT(mesh);
	auto& entry = scaledMeshes.push_back();
	entry.assetKey = assetKey;
	entry.meshIndex = meshIndex;
	entry.scale = scale;
	entry.mesh.triangle = mesh;
	return entry.mesh;
}

void PhysicsContext::CreateScene(PhysicsScene* out)
{
	NxSceneDesc desc;
	desc.simType = NX_SIMULATION_SW;
	desc.gravity = NxVec3(0.0f, 0.0f, -1800.0f);
	desc.maxTimestep = RigidTimestep;
	desc.maxIter = 1;
	desc.timeStepMethod = NX_TIMESTEP_FIXED;

	NxScene* scene;
	{ LOCK_MUTEX(mutexSceneCreate);
		scene = physics->createScene(desc);
		if(!scene) {
			LOG("[PhysX] ERROR: Creating scene failed");
		}
		ASSERT(scene);

		out->scene = scene;
		out->rigidClock = 0.0f;
		out->rigidSimulatedThrough = 0.0f;
		NxMaterial* material = scene->getMaterialFromIndex(0);
		ASSERT(material);
		material->setStaticFriction(0.5f);
		material->setDynamicFriction(0.5f);
		material->setRestitution(0.5f);
		material->setDirOfAnisotropy(NxVec3(0, 0, 0));
		scene->setFilterOps(NX_FILTEROP_OR, NX_FILTEROP_OR, NX_FILTEROP_SWAP_AND);
		scene->setFilterBool(true);
		NxGroupsMask constant;
		constant.bits0 = constant.bits1 = constant.bits2 = constant.bits3 = 0;
		scene->setFilterConstant0(constant);
		scene->setFilterConstant1(constant);

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
	const f32 dt = StepDuration();
	foreach(c, colliderList) {
		StepBody(*c, c->moveDir * (c->moveSpeed * dt), dt);
	}
	FinishStep(dt);
}

f32 PhysicsScene::StepDuration() const
{
	const f32 clock = f32(TimeDurationSec(Time::ZERO, localTime));
	const f32 nextClock = clock + f32(UPDATE_RATE);
	const f32 dt = nextClock - clock;
	ASSERT(dt > 0.f);
	return dt;
}

PhysicsBodyState PhysicsScene::CaptureBodyState(const PhysicsDynamicBody& body) const
{
	ASSERT(body.collider);
	PhysicsBodyState state;
	state.center = body.collider->getDebugPosition();
	state.radius = body.radius;
	state.height = body.height;
	state.feetOffset = body.feetOffset;
	state.vel = body.vel;
	state.moveDir = body.moveDir;
	state.moveSpeed = body.moveSpeed;
	state.groundNormal = body.groundNormal;
	state.collisionGroups = body.collisionGroups;
	state.collisionTeam = body.collisionTeam;
	state.grounded = body.grounded;
	state.jumpMotion = body.jumpMotion;
	state.jumpElapsed = body.jumpElapsed;
	state.jumpHeight = body.jumpHeight;
	state.horizontalGraphDisplacement = body.horizontalGraphDisplacement;
	state.horizontalGraphActive = body.horizontalGraphActive;
	state.destPos = body.destPos;
	state.destActive = body.destActive;
	state.destForced = body.destForced;
	state.suppressContactReport = body.suppressContactReport;
	return state;
}

void PhysicsScene::RestoreBodyState(PhysicsDynamicBody& body, const PhysicsBodyState& state)
{
	ASSERT(body.collider);
	ASSERT(body.collider->getUserData() == &body);
	ASSERT(state.radius > 0.0f && state.height > 0.0f && CCT_STEP_OFFSET <= state.height + state.radius * 2.0f);
	if(body.radius != state.radius) {
		const bool resized = body.collider->setRadius(state.radius);
		ASSERT(resized);
		body.radius = state.radius;
	}
	if(body.height != state.height) {
		const bool resized = body.collider->setHeight(state.height);
		ASSERT(resized);
		body.height = state.height;
	}
	body.feetOffset = state.feetOffset;
	const bool positioned = body.collider->setPosition(state.center);
	ASSERT(positioned);
	body.vel = state.vel;
	body.moveDir = state.moveDir;
	body.moveSpeed = state.moveSpeed;
	body.groundNormal = state.groundNormal;
	body.collisionGroups = state.collisionGroups;
	body.collisionTeam = state.collisionTeam;
	body.grounded = state.grounded;
	body.jumpMotion = state.jumpMotion;
	body.jumpElapsed = state.jumpElapsed;
	body.jumpHeight = state.jumpHeight;
	body.horizontalGraphDisplacement = state.horizontalGraphDisplacement;
	body.horizontalGraphActive = state.horizontalGraphActive;
	body.destPos = state.destPos;
	body.destActive = state.destActive;
	body.destForced = state.destForced;
	body.suppressContactReport = state.suppressContactReport;
	NxActor* actor = body.collider->getActor();
	ASSERT(actor && actor->getNbShapes() == 1);
	NxShape* const* shapes = actor->getShapes();
	ASSERT(shapes && shapes[0]);
	shapes[0]->setGroupsMask(ControllerGroupsMask(body));
	body.collider->reportSceneChanged();
}

void PhysicsScene::StepBody(PhysicsDynamicBody& body, const vec2& walkingDisplacement, f32 dt)
{
	ASSERT(dt > 0.f);
	if(body.destActive && body.destForced) {
		const vec3 position = body.GetWorldPos();
		const vec3 delta = body.destPos - position;
		if(delta.x != 0.0f || delta.y != 0.0f || delta.z != 0.0f) {
			const bool segmented = (delta.x != 0.f || delta.y != 0.f) &&
				std::sqrt(delta.y * delta.y + delta.x * delta.x) / dt > 2000.f;
			Move(&body, delta, dt, segmented);
			body.horizontalGraphDisplacement = vec2(0);
			body.horizontalGraphActive = false;
			return;
		}
	}
	const f32 gravity = GetGameXmlContent().creatureGravity;
	body.vel.x = body.moveDir.x * body.moveSpeed;
	body.vel.y = body.moveDir.y * body.moveSpeed;
	body.vel.z += gravity * dt;
	vec3 displacement(walkingDisplacement, body.vel.z * dt);
	const bool ordinaryVertical = !body.horizontalGraphActive && !body.jumpMotion;
	if(body.horizontalGraphActive) {
		displacement = vec3(body.horizontalGraphDisplacement, 0);
	}
	if(body.jumpMotion) {
		body.jumpElapsed = eastl::min(body.jumpElapsed + dt, body.jumpMotion->duration);
		const f32 height = body.jumpMotion->Sample(body.jumpElapsed);
		displacement.z = height - body.jumpHeight;
		body.jumpHeight = height;
		if(body.jumpElapsed >= body.jumpMotion->duration) {
			body.jumpMotion = nullptr;
		}
	}
	const vec3 position = body.GetWorldPos();
	vec3 target = position + displacement;
	if(ordinaryVertical && body.groundNormal.z > 0.7071f && body.groundNormal.z < 0.93f &&
		walkingDisplacement.x * walkingDisplacement.x + walkingDisplacement.y * walkingDisplacement.y > 0.0f) {
		const f32 projection = body.groundNormal.x * walkingDisplacement.x + body.groundNormal.y * walkingDisplacement.y;
		target.z -= (projection / body.groundNormal.z) * 0.7f;
	}
	displacement = target - position;
	const bool segmented = (displacement.x != 0.f || displacement.y != 0.f) &&
		std::sqrt(displacement.y * displacement.y + displacement.x * displacement.x) / dt > 2000.f;
	Move(&body, displacement, dt, segmented);
	body.horizontalGraphDisplacement = vec2(0);
	body.horizontalGraphActive = false;
}

void PhysicsScene::FinishStep(f32 dt)
{
	ASSERT(dt > 0.f);
	rigidClock += dt;
	if(rigidClock <= rigidSimulatedThrough) return;
	const u32 elapsedSteps = u32(std::ceil((rigidClock - rigidSimulatedThrough - 1e-6f) / RigidTimestep));
	rigidSimulatedThrough += f32(elapsedSteps) * RigidTimestep;
	const u32 steps = eastl::min(elapsedSteps, 8u);
	scene->setTiming(RigidTimestep, steps, NX_TIMESTEP_FIXED);
	scene->simulate(f32(steps) * RigidTimestep + 1.0f);
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
	materials.clear();
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

static constexpr CollisionAsset::Material DynamicCollisionMaterial = {
	0, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, { 1.0f, 0.0f, 0.0f }, 0, 0, 0
};

static NxMaterialIndex CollisionMaterial(PhysicsScene& physicsScene, const CollisionAsset::Material& recipe)
{
	for(const auto& entry: physicsScene.materials) {
		if(memcmp(&entry.recipe->dynamicFriction, &recipe.dynamicFriction, sizeof(recipe) - sizeof(recipe.index)) == 0) return entry.index;
	}
	ASSERT(physicsScene.materials.size() < physicsScene.materials.max_size());
	NxMaterialDesc desc;
	desc.dynamicFriction = recipe.dynamicFriction;
	desc.staticFriction = recipe.staticFriction;
	desc.restitution = recipe.restitution;
	desc.dynamicFrictionV = recipe.dynamicFrictionV;
	desc.staticFrictionV = recipe.staticFrictionV;
	desc.dirOfAnisotropy = NxVec3(recipe.anisotropy[0], recipe.anisotropy[1], recipe.anisotropy[2]);
	desc.flags = recipe.flags;
	desc.frictionCombineMode = NxCombineMode(recipe.frictionCombine);
	desc.restitutionCombineMode = NxCombineMode(recipe.restitutionCombine);
	ASSERT(desc.isValid());
	NxMaterial* material = physicsScene.scene->createMaterial(desc);
	ASSERT(material);
	physicsScene.materials.push_back({ &recipe, material->getMaterialIndex() });
	return material->getMaterialIndex();
}

static NxMat34 CollisionPlacement(const vec3& pos, const vec3& rot)
{
	const f32 cx = cosf(-rot.x), sx = sinf(-rot.x);
	const f32 cy = cosf(-rot.y), sy = sinf(-rot.y);
	const f32 cz = cosf(-rot.z), sz = sinf(-rot.z);
	NxMat33 x, y, z;
	x.id(); y.id(); z.id();
	x(1, 1) = cx; x(1, 2) = -sx; x(2, 1) = sx; x(2, 2) = cx;
	y(0, 0) = cy; y(0, 2) = sy; y(2, 0) = -sy; y(2, 2) = cy;
	z(0, 0) = cz; z(0, 1) = -sz; z(1, 0) = sz; z(1, 1) = cz;
	NxMat34 placement;
	placement.M = x * y * z;
	placement.t = NxVec3(pos.x, pos.y, pos.z);
	return placement;
}

static NxCollisionGroup CollisionNameGroup(const char* name, PhysicsCollisionGroup fallback)
{
	if(strstr(name, "GuardrailMob")) return NxCollisionGroup(PhysicsCollisionGroup::FenceAll);
	if(strstr(name, "Guardrail")) return NxCollisionGroup(PhysicsCollisionGroup::Fence);
	if(strstr(name, "Aim")) return NxCollisionGroup(PhysicsCollisionGroup::Aim);
	return NxCollisionGroup(fallback);
}

union CollisionShapeStorage
{
	NxBoxShapeDesc box;
	NxCapsuleShapeDesc capsule;
	NxConvexShapeDesc convex;
	NxTriangleMeshShapeDesc triangle;
	CollisionShapeStorage() {}
	~CollisionShapeStorage() {}
};

void PhysicsScene::CreateCollisionAsset(const char* assetName, const vec3& pos, const vec3& rot, f32 scale, PhysicsCollisionGroup group, eastl::fixed_vector<NxActor*,4,false>* actors)
{
	ASSERT(scene && std::isfinite(scale) && scale > 0.0f);
	auto& ctx = PhysContext();
	LOCK_MUTEX(ctx.mutexSceneCreate);
	const size_t assetKey = eastl::hash<const char*>{}(assetName);
	const auto& asset = ctx.collisionAssetMap.at(assetKey);
	ASSERT(strcmp(asset.name.data(), assetName) == 0);
	const NxMat34 placement = CollisionPlacement(pos, rot);
	NxActorDesc& desc = creationDesc;
	for(const auto& actor: asset.actors) {
		const auto& recipe = *actor.recipe;
		const auto& source = *actor.shape;
		ASSERT(recipe.name[63] == 0 && source.name[63] == 0);
		CollisionShapeStorage storage;
		NxShapeDesc* shape = nullptr;
		if(source.type == NX_SHAPE_BOX) {
			shape = new(&storage.box) NxBoxShapeDesc;
			storage.box.dimensions = NxVec3(source.dimensions[0] * scale, source.dimensions[1] * scale, source.dimensions[2] * scale);
		}
		else if(source.type == NX_SHAPE_CAPSULE) {
			shape = new(&storage.capsule) NxCapsuleShapeDesc;
			storage.capsule.radius = source.dimensions[0] * scale;
			storage.capsule.height = source.dimensions[1] * scale;
		}
		else {
			const auto& mesh = ctx.GetCollisionMesh(assetKey, u32(source.meshIndex), scale);
			if(source.type == NX_SHAPE_CONVEX) {
				ASSERT(mesh.convex);
				shape = new(&storage.convex) NxConvexShapeDesc;
				storage.convex.meshData = mesh.convex;
			}
			else {
				ASSERT(source.type == NX_SHAPE_MESH && mesh.triangle);
				shape = new(&storage.triangle) NxTriangleMeshShapeDesc;
				storage.triangle.meshData = mesh.triangle;
			}
		}
		shape->name = source.name;
		shape->localPose = CollisionTransform(source.localPose, scale);
		shape->shapeFlags = source.flags & ~NX_SF_VISUALIZATION;
		shape->group = CollisionNameGroup(source.name, group);
		shape->groupsMask.bits0 = 1u << shape->group;
		shape->groupsMask.bits1 = source.groupsMask[1];
		shape->groupsMask.bits2 = group == PhysicsCollisionGroup::Dynamic ? 0xc7e06u :
			group == PhysicsCollisionGroup::StartDelay ? 0xa06u : source.groupsMask[2];
		shape->groupsMask.bits3 = source.groupsMask[3];
		shape->density = source.density;
		shape->mass = source.mass;
		shape->skinWidth = source.skinWidth;
		shape->nonInteractingCompartmentTypes = source.nonInteracting;
		if(group == PhysicsCollisionGroup::Dynamic) shape->materialIndex = CollisionMaterial(*this, DynamicCollisionMaterial);
		else {
			const CollisionAsset::Material* material = nullptr;
			for(u16 i = 0; i < asset.header->materialCount; ++i) {
				if(asset.materials[i].index == source.materialIndex) {
					material = &asset.materials[i];
					break;
				}
			}
			ASSERT(material || source.materialIndex == 0);
			shape->materialIndex = material ? CollisionMaterial(*this, *material) : 0;
		}
		ASSERT(shape->isValid());

		desc.shapes.clear();
		desc.body = nullptr;
		desc.name = recipe.name;
		desc.globalPose = placement * CollisionTransform(recipe.globalPose, scale);
		desc.flags = recipe.flags;
		desc.group = shape->group;
		desc.dominanceGroup = NxDominanceGroup(recipe.dominanceGroup);
		desc.contactReportFlags = recipe.contactReportFlags;
		desc.forceFieldMaterial = NxForceFieldMaterial(recipe.forceFieldMaterial);
		desc.density = recipe.density;
		NxBodyDesc& body = creationBody;
		if(actor.body) {
			ASSERT(scale == 1.0f && (actor.body->flags & NX_BF_KINEMATIC));
			const auto& b = *actor.body;
			body.massLocalPose = CollisionTransform(b.massLocalPose, scale);
			body.massSpaceInertia = NxVec3(b.inertia[0], b.inertia[1], b.inertia[2]);
			body.mass = b.mass;
			body.linearVelocity = NxVec3(b.linearVelocity[0], b.linearVelocity[1], b.linearVelocity[2]);
			body.angularVelocity = NxVec3(b.angularVelocity[0], b.angularVelocity[1], b.angularVelocity[2]);
			body.flags = b.flags;
			body.wakeUpCounter = b.wakeUpCounter;
			body.linearDamping = b.linearDamping;
			body.angularDamping = b.angularDamping;
			body.maxAngularVelocity = b.maxAngularVelocity;
			body.CCDMotionThreshold = b.ccdMotionThreshold;
			body.sleepLinearVelocity = b.sleepLinearVelocity;
			body.sleepAngularVelocity = b.sleepAngularVelocity;
			body.solverIterationCount = b.solverIterationCount;
			body.sleepEnergyThreshold = b.sleepEnergyThreshold;
			body.sleepDamping = b.sleepDamping;
			body.contactReportThreshold = b.contactReportThreshold;
			ASSERT(body.isValid());
			desc.body = &body;
			if(body.mass != 0.0f) desc.density = 0.0f;
		}
		desc.shapes.pushBack(shape);
		ASSERT(desc.isValid());
		NxActor* created = scene->createActor(desc);
		ASSERT(created);
		if(actors) {
			ASSERT(actors->size() < actors->max_size());
			actors->push_back(created);
		}
		shape->~NxShapeDesc();
	}
	desc.body = nullptr;
	desc.shapes.clear();
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
	desc.callback = &g_ControllerHitReport;
	desc.userData = &collider;
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
	shapes[0]->setGroupsMask(ControllerGroupsMask(collider));
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

void PhysicsScene::InstallJumpGraph(PhysicsDynamicBody* body, const JumpMotion& motion)
{
	ASSERT(motion.duration > 0.0f && !body->jumpMotion);
	body->jumpMotion = &motion;
	body->jumpElapsed = 0.0f;
	body->jumpHeight = 0.0f;
	body->grounded = false;
}

void PhysicsScene::InstallMoveDest(PhysicsDynamicBody* body, const vec3& dest, bool forced)
{
	ASSERT(body && body->collider);
	if(body->destActive) return;
	body->destPos = dest;
	body->destActive = true;
	body->destForced = forced;
}

void PhysicsScene::CancelMoveDest(PhysicsDynamicBody* body)
{
	ASSERT(body);
	body->destPos = vec3(0);
	body->destActive = false;
	body->destForced = false;
}

vec3 PhysicsScene::Move(PhysicsDynamicBody* body, const vec3& disp, f32 time, bool segmented)
{
	if(std::fabs(disp.x) < 1.1920929e-7f && std::fabs(disp.y) < 1.1920929e-7f && std::fabs(disp.z) < 1.1920929e-7f) {
		return body->GetWorldPos();
	}
	const bool trace = VelqorTrace::Enabled();
	NxExtendedVec3 centerBefore;
	if(trace) centerBefore = body->collider->getDebugPosition();
	body->collider->reportSceneChanged();
	NxU32 collisionFlags = 0;
	const NxGroupsMask mask = ControllerGroupsMask(*body);
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
			body->collider->move(NxVec3(step.x, step.y, step.z), body->collisionGroups, 0.0f, collisionFlags, 0.0f, &mask);
		}
		const vec3 step = direction * remaining;
		body->collider->move(NxVec3(step.x, step.y, step.z), body->collisionGroups, 0.0f, collisionFlags, 0.0f, &mask);
	}
	else {
		body->collider->move(NxVec3(disp.x, disp.y, disp.z), body->collisionGroups, 0.0f, collisionFlags, 0.0f, &mask);
	}
	body->grounded = (collisionFlags & NXCC_COLLISION_DOWN) != 0;
	const vec3 end = body->GetWorldPos();
	if(trace) {
		const NxExtendedVec3& centerAfter = body->collider->getDebugPosition();
		char fields[1536];
		size_t length = 0;
		VelqorTrace::Catf(fields, sizeof(fields), length,
			"\"controller_id\":%llu,\"requested\":[%.9g,%.9g,%.9g],\"center_before\":[%.17g,%.17g,%.17g],\"center_after\":[%.17g,%.17g,%.17g],\"feet\":[%.9g,%.9g,%.9g],\"flags\":%u,\"active_groups\":%u,\"groups_mask\":[%u,%u,%u,%u],\"segmented\":%s,\"dt\":%.9g,\"radius\":%.9g,\"height\":%.9g,\"feet_offset\":%.9g,\"move_dir\":[%.9g,%.9g],\"move_speed\":%.9g,\"vertical_velocity\":%.9g,\"jump_elapsed\":%.9g,\"grounded\":%s",
			(unsigned long long)(size_t)body->collider, disp.x, disp.y, disp.z,
			centerBefore.x, centerBefore.y, centerBefore.z, centerAfter.x, centerAfter.y, centerAfter.z,
			end.x, end.y, end.z, collisionFlags, body->collisionGroups,
			mask.bits0, mask.bits1, mask.bits2, mask.bits3, segmented ? "true" : "false", time,
			body->radius, body->height, body->feetOffset, body->moveDir.x, body->moveDir.y, body->moveSpeed,
			body->vel.z, body->jumpElapsed, body->grounded ? "true" : "false");
		VelqorTrace::Emit("physics_move", fields);
	}
	return end;
}

vec3 PhysicsScene::FindMovePos(PhysicsDynamicBody* body, const vec3& disp, f32 time)
{
	const NxExtendedVec3 start = body->collider->getDebugPosition();
	const bool grounded = body->grounded;
	const vec3 velocity = body->vel;
	const vec3 position = body->GetWorldPos();
	const vec3 displacement = (position + disp) - position;
	ASSERT(!body->suppressContactReport);
	body->suppressContactReport = true;
	const vec3 end = Move(body, displacement, time, false);
	const bool positioned = body->collider->setPosition(start);
	ASSERT(positioned);
	body->suppressContactReport = false;
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
