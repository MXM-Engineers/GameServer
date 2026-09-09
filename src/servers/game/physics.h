#pragma once
#include <common/vector_math.h>
#include <common/utils.h>
#include <EASTL/array.h>
#include <EASTL/fixed_list.h>

#include <mxm/jump_motion.h>
#include <foundation/PxAllocatorCallback.h>
#include <foundation/PxErrorCallback.h>
#include <PxFoundation.h>
#include <PxPhysics.h>
#include <PxRigidDynamic.h>
#include <task/PxCpuDispatcher.h>
#include <characterkinematic/PxBoxController.h>
#include <characterkinematic/PxCapsuleController.h>
#include <characterkinematic/PxControllerManager.h>
using namespace physx;

// should be no-op
inline vec3 tov3(const PxVec3& v)
{
	return { v.x, v.y, v.z };
}
inline vec3 tov3(const PxExtendedVec3& v)
{
	return { v.x, v.y, v.z };
}

// TODO: actual allocator
struct PhysicsAllocatorCallback: PxAllocatorCallback
{
	virtual ~PhysicsAllocatorCallback() override
	{

	}

	virtual void* allocate(size_t size, const char* typeName, const char* filename, int line) override
	{
		void* ptr = memAlloc(size);
		DBG_ASSERT((((intptr_t)ptr) & 15) == 0); // 16 aligned
		return ptr;
	}

	virtual void deallocate(void* ptr) override
	{
		memFree(ptr);
	}
};

struct PhysicsErrorCallback: PxErrorCallback
{
	virtual void reportError(PxErrorCode::Enum code, const char* message, const char* file, int line) override
	{
		const char* errorCode = "UNKNOWN";
		switch(code) {
			case PxErrorCode::eNO_ERROR: { errorCode = "NO_ERROR"; } break;
			case PxErrorCode::eDEBUG_INFO: { errorCode = "DEBUG_INFO"; } break;
			case PxErrorCode::eDEBUG_WARNING: { errorCode = "DEBUG_WARNING"; } break;
			case PxErrorCode::eINVALID_PARAMETER: { errorCode = "INVALID_PARAMETER"; } break;
			case PxErrorCode::eINVALID_OPERATION: { errorCode = "INVALID_OPERATION"; } break;
			case PxErrorCode::eOUT_OF_MEMORY: { errorCode = "OUT_OF_MEMORY"; } break;
			case PxErrorCode::eINTERNAL_ERROR: { errorCode = "INTERNAL_ERROR"; } break;
			case PxErrorCode::eABORT: { errorCode = "ABORT"; } break;
			case PxErrorCode::ePERF_WARNING: { errorCode = "PERF_WARNING"; } break;
			case PxErrorCode::eMASK_ALL: { errorCode = "MASK_ALL"; } break;
		}

		LOG("[PhysX] ERROR(%s) '%s' @(%s : %d)", errorCode, message, file, line);
	}
};

enum class PhysicsCollisionGroup : u32
{
	Static = 1,
	ControllerPc = 3,
	Fence = 18,
	FenceAll = 19,
	Aim = 22,
};

struct PhysicsDynamicBody
{
	PxCapsuleController* collider = nullptr;
	vec3 vel = vec3(0); // actual velocity
	bool grounded = false;
	const JumpMotion* jumpMotion = nullptr;
	f32 jumpElapsed = 0.0f;
	f32 jumpHeight = 0.0f;

	inline vec3 GetWorldPos() const { return tov3(collider->getFootPosition()); }
	inline vec2 GetBoundSize() const { return { radius, height + radius * 2 }; }

private:
	f32 radius;
	f32 height;
	PxFilterData collisionFilterData;

	friend struct PhysicsScene;
};

struct PhysicsScene
{
	Time localTime = Time::ZERO;
    PxScene* scene = nullptr;
    PxControllerManager* controllerMngr = nullptr;
	eastl::fixed_vector<PhysicsDynamicBody,256,false> colliderList; // doesn't grow so we don't invalidate pointer

	void Step();
	void Destroy();

	void CreateStaticCollider(const char* meshName, const vec3& pos, const vec3& rot, PhysicsCollisionGroup group);
	PhysicsDynamicBody* CreateDynamicBody(f32 radius, f32 height, const vec3& pos);
	void ResizeDynamicBody(PhysicsDynamicBody* body, f32 radius, f32 height);
	bool StartJump(PhysicsDynamicBody* body, const JumpMotion& motion);
	vec3 Move(PhysicsDynamicBody* body, const vec3& disp, f32 time /* seconds */);
	vec3 FindMovePos(PhysicsDynamicBody* body, const vec3& disp, f32 time);
};

struct PhysicsContext
{
	PhysicsAllocatorCallback allocatorCallback;
	PhysicsErrorCallback errorCallback;
	PxFoundation* foundation;
	PxCpuDispatcher* dispatcher;
	PxPhysics* physics;
	PxPvd* pvd;

	ProfileMutex(Mutex, mutexSceneCreate);

	PxMaterial* matMapSurface;

	eastl::fixed_hash_map<size_t, PxTriangleMesh*, 64> triangleMeshMap;

	bool Init();
	void Shutdown();

	bool LoadCollisionMeshes(const FileBuffer& file);
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