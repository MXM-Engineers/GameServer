#pragma once
#include <common/vector_math.h>
#include <common/utils.h>
#include <mxm/jump_motion.h>
#include <common/collision_asset.h>
#include <NxPhysics.h>
#include <NxCapsuleController.h>
#include <NxControllerManager.h>
#include <malloc.h>

struct PhysicsAllocatorCallback: NxUserAllocator
{
	virtual void* mallocDEBUG(size_t size, const char*, int) override
	{
		void* ptr = _aligned_malloc(size ? size : 1, 16);
		ASSERT(ptr);
		DBG_ASSERT((((intptr_t)ptr) & 15) == 0);
		return ptr;
	}

	virtual void* malloc(size_t size) override
	{
		void* ptr = _aligned_malloc(size ? size : 1, 16);
		ASSERT(ptr);
		DBG_ASSERT((((intptr_t)ptr) & 15) == 0);
		return ptr;
	}

	virtual void* realloc(void* memory, size_t size) override
	{
		void* ptr = _aligned_realloc(memory, size ? size : 1, 16);
		ASSERT(ptr);
		DBG_ASSERT((((intptr_t)ptr) & 15) == 0);
		return ptr;
	}

	virtual void free(void* memory) override
	{
		_aligned_free(memory);
	}
};

struct PhysicsOutputStream: NxUserOutputStream
{
	virtual void reportError(NxErrorCode code, const char* message, const char* file, int line) override
	{
		const char* errorCode = "UNKNOWN";
		switch(code) {
			case NXE_NO_ERROR: { errorCode = "NO_ERROR"; } break;
			case NXE_INVALID_PARAMETER: { errorCode = "INVALID_PARAMETER"; } break;
			case NXE_INVALID_OPERATION: { errorCode = "INVALID_OPERATION"; } break;
			case NXE_OUT_OF_MEMORY: { errorCode = "OUT_OF_MEMORY"; } break;
			case NXE_INTERNAL_ERROR: { errorCode = "INTERNAL_ERROR"; } break;
			case NXE_ASSERTION: { errorCode = "ASSERTION"; } break;
			case NXE_DB_INFO: { errorCode = "DEBUG_INFO"; } break;
			case NXE_DB_WARNING: { errorCode = "DEBUG_WARNING"; } break;
			case NXE_DB_PRINT: { errorCode = "DEBUG_PRINT"; } break;
		}

		LOG("[PhysX] ERROR(%s) '%s' @(%s : %d)", errorCode, message, file, line);
	}

	virtual NxAssertResponse reportAssertViolation(const char* message, const char* file, int line) override
	{
		LOG("[PhysX] ASSERT '%s' @(%s : %d)", message, file, line);
		ASSERT(false);
		return NX_AR_CONTINUE;
	}

	virtual void print(const char* message) override
	{
		LOG("[PhysX] %s", message);
	}
};

enum class PhysicsCollisionGroup : u32
{
	Static = 1,
	Prop = 2,
	ControllerPc = 3,
	Dynamic = 11,
	StartDelay = 12,
	Fence = 18,
	FenceAll = 19,
	Aim = 22,
};

struct PhysicsDynamicBody
{
	NxCapsuleController* collider = nullptr;
	vec3 vel = vec3(0); // actual velocity
	vec2 moveDir = vec2(0);
	f32 moveSpeed = 0.0f;
	vec3 groundNormal = vec3(0, 0, 1);
	u32 collisionGroups = 0x1c7f06;
	u32 collisionTeam = 1;
	bool grounded = false;
	const JumpMotion* jumpMotion = nullptr;
	f32 jumpElapsed = 0.0f;
	f32 jumpHeight = 0.0f;
	vec2 horizontalGraphDisplacement = vec2(0);
	bool horizontalGraphActive = false;

	inline vec3 GetWorldPos() const
	{
		const NxExtendedVec3& center = collider->getDebugPosition();
		return { f32(center.x), f32(center.y), f32(center.z) - feetOffset };
	}
	inline vec2 GetBoundSize() const { return { radius, height + radius * 2 }; }
	void SetIgnoreUnitCollision(bool ignore);

private:
	f32 radius;
	f32 height;
	f32 feetOffset;
	bool suppressContactReport = false;

	friend struct PhysicsScene;
	friend struct PhysicsControllerHitReport;
};

struct PhysicsBodyState
{
	NxExtendedVec3 center = NxExtendedVec3(0.0, 0.0, 0.0);
	f32 radius = 0.0f;
	f32 height = 0.0f;
	f32 feetOffset = 0.0f;
	vec3 vel = vec3(0);
	vec2 moveDir = vec2(0);
	f32 moveSpeed = 0.0f;
	vec3 groundNormal = vec3(0, 0, 1);
	u32 collisionGroups = 0x1c7f06;
	u32 collisionTeam = 1;
	bool grounded = false;
	const JumpMotion* jumpMotion = nullptr;
	f32 jumpElapsed = 0.0f;
	f32 jumpHeight = 0.0f;
	vec2 horizontalGraphDisplacement = vec2(0);
	bool horizontalGraphActive = false;
	bool suppressContactReport = false;
};

struct PhysicsScene
{
	struct MaterialEntry
	{
		const CollisionAsset::Material* recipe;
		NxMaterialIndex index;
	};

	Time localTime = Time::ZERO;
    NxScene* scene = nullptr;
    NxControllerManager* controllerMngr = nullptr;
	eastl::fixed_vector<PhysicsDynamicBody,256,false> colliderList; // doesn't grow so we don't invalidate pointer
	eastl::fixed_vector<MaterialEntry,16,false> materials;
	NxActorDesc creationDesc;
	NxBodyDesc creationBody;

	void Step();
	PhysicsBodyState CaptureBodyState(const PhysicsDynamicBody& body) const;
	void RestoreBodyState(PhysicsDynamicBody& body, const PhysicsBodyState& state);
	void StepBody(PhysicsDynamicBody& body);
	void FinishStep();
	void Destroy();

	void CreateCollisionAsset(const char* assetName, const vec3& pos, const vec3& rot, f32 scale, PhysicsCollisionGroup group, eastl::fixed_vector<NxActor*,4,false>* actors = nullptr);
	PhysicsDynamicBody* CreateDynamicBody(f32 radius, f32 height, const vec3& pos);
	void ResizeDynamicBody(PhysicsDynamicBody* body, f32 radius, f32 height);
	bool StartJump(PhysicsDynamicBody* body, const JumpMotion& motion);
	vec3 Move(PhysicsDynamicBody* body, const vec3& disp, f32 time, bool segmented);
	vec3 FindMovePos(PhysicsDynamicBody* body, const vec3& disp, f32 time);
};
struct PhysicsCollisionActor
{
	const CollisionAsset::Actor* recipe = nullptr;
	const CollisionAsset::Body* body = nullptr;
	const CollisionAsset::Shape* shape = nullptr;
};


struct PhysicsCollisionMesh
{
	NxTriangleMesh* triangle = nullptr;
	NxConvexMesh* convex = nullptr;
};
struct PhysicsCollisionAsset
{
	FixedStr64 name;
	const CollisionAsset::Header* header = nullptr;
	const CollisionAsset::Material* materials = nullptr;
	eastl::fixed_vector<PhysicsCollisionMesh,8,false> meshes;
	eastl::fixed_vector<PhysicsCollisionActor,8,false> actors;
};

struct PhysicsScaledMesh
{
	size_t assetKey;
	u32 meshIndex;
	f32 scale;
	PhysicsCollisionMesh mesh;
};


struct PhysicsContext
{
	PhysicsAllocatorCallback allocatorCallback;
	PhysicsOutputStream outputStream;
	NxPhysicsSDK* physics = nullptr;

	ProfileMutex(Mutex, mutexSceneCreate);

	eastl::fixed_hash_map<size_t,PhysicsCollisionAsset,32,37,false> collisionAssetMap;
	eastl::fixed_vector<PhysicsScaledMesh,64,false> scaledMeshes;

	bool Init();
	void Shutdown();

	bool LoadCollisionAsset(const char* name, const FileBuffer& file);
	const PhysicsCollisionMesh& GetCollisionMesh(size_t assetKey, u32 meshIndex, f32 scale);
	void CreateScene(PhysicsScene* out);
};

bool PhysicsInit();
PhysicsContext& PhysContext();

inline f32 LengthSq(const vec2& v)
{
	return glm::dot(v, v);
}

inline f32 LengthSq(const vec3& v)
{
	return glm::dot(v, v);
}

inline vec2 NormalizeSafe(vec2 v)
{
	if(LengthSq(v) > FLT_EPSILON) return glm::normalize(v);
	return vec2(0);
}
