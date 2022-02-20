#pragma once
#include <common/vector_math.h>
#include <common/utils.h>
#include <EASTL/array.h>
#include <EASTL/fixed_list.h>

#include <foundation/PxAllocatorCallback.h>
#include <foundation/PxErrorCallback.h>
#include <PxFoundation.h>
#include <PxPhysics.h>
#include <task/PxCpuDispatcher.h>
using namespace physx;

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

struct PhysicsCollisionMesh
{
	PxTriangleMesh* mesh;
	PxTriangleMeshGeometry geometry;
	PxShape* shape;
};

struct PhysicsScene
{
	PxScene* scene;

	void Step();
	void Destroy();

	void CreateStaticCollider(PxTriangleMesh* mesh);
	void CreateEntityCollider(f32 radius, f32 height);
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
	PxConvexMesh* cylinderMesh;

	bool Init();
	void Shutdown();

	bool LoadCollisionMesh(PxTriangleMesh** out, const FileBuffer& file);
	bool LoadCollisionMesh(PxConvexMesh** out, const FileBuffer& file);
	void CreateScene(PhysicsScene* out);
};

bool PhysicsInit();
PhysicsContext& PhysContext();

constexpr f32 PHYS_EPSILON = 0.0001f; // Warning: NEVER change this value

struct ShapeSphere
{
	vec3 center;
	f32 radius;
};

struct ShapeCapsule
{
	vec3 base;
	vec3 tip;
	f32 radius;

	inline vec3 Normal() const
	{
		return glm::normalize(tip - base);
	}

	inline vec3 InnerBase() const
	{
		return base + Normal() * radius;
	}

	inline vec3 InnerTip() const
	{
		return tip - Normal() * radius;
	}
};

struct ShapeTriangle
{
	eastl::array<vec3,3> p;

	inline vec3 Normal() const
	{
		return glm::normalize(glm::cross(p[1] - p[0], p[2] - p[0]));
	}

	inline vec3 Center() const
	{
		return (p[0] + p[1] + p[2]) / 3.f;
	}
};

struct ShapeCylinder
{
	vec3 base;
	f32 height;
	f32 radius;

	inline vec3 Normal() const
	{
		return vec3(0, 0, 1);
	}
};

struct PhysRect
{
	vec2 size;
	vec3 pos;
	vec3 normal; // defines the plane

	inline vec3 Center() const
	{
		const vec3 up = vec3(0, 0, 1);
		const vec3 vx = -glm::normalize(glm::cross(up, normal));
		const vec3 vy = glm::normalize(glm::cross(vx, normal));
		return pos + vx * (size.x * 0.5f) + vy * (size.y * 0.5f);
	}
};

struct PhysPenetrationVector
{
	vec3 dir;
	f32 depth;
};

struct PhysResolutionCylinderTriangle
{
	vec3 slide = vec3(0);
	vec3 pushX = vec3(0);
	vec3 pushZ = vec3(0);
};

inline f32 LengthSq(const vec2& v)
{
	return glm::dot(v, v);
}

inline f32 LengthSq(const vec3& v)
{
	return glm::dot(v, v);
}

// Project point on segment
inline vec3 ClosestPointOnLineSegment(const vec3& la, const vec3& lb, const vec3& point)
{
	vec3 delta = lb - la;
	f32 t = glm::dot(point - la, delta) / glm::dot(delta, delta);
	return la + clamp(t, 0.f, 1.f) * delta;
}

inline vec2 ClosestPointOnLineSegment(const vec2& la, const vec2& lb, const vec2& point)
{
	vec2 delta = lb - la;
	f32 t = glm::dot(point - la, delta) / glm::dot(delta, delta);
	return la + clamp(t, 0.f, 1.f) * delta;
}

inline vec3 ProjectVec(const vec3& v, const vec3& on)
{
	vec3 n = glm::normalize(on);
	return glm::dot(v, n) * n;
}

inline vec2 ProjectVec2(const vec2& v, const vec2& on)
{
	vec2 n = glm::normalize(on);
	return glm::dot(v, n) * n;
}

// on needs to be normalized
inline vec3 ProjectVecNorm(const vec3& v, const vec3& on)
{
	return glm::dot(v, on) * on;
}
inline vec2 ProjectVec2Norm(const vec2& v, const vec2& on)
{
	return glm::dot(v, on) * on;
}

inline vec3 Lerp(const vec3& v1, const vec3& v2, f32 a)
{
	ASSERT(a >= 0 && a <= 1);
	return (v1 * (1.0f - a)) + (v2 * a);
}

inline f32 SignedEpsilon(f32 v)
{
	return v > 0.0f ? PHYS_EPSILON : -PHYS_EPSILON;
}

inline vec2 NormalizeSafe(vec2 v)
{
	if(LengthSq(v) > FLT_EPSILON) return glm::normalize(v);
	return vec2(0);
}

inline vec3 NormalizeSafe(vec3 v)
{
	if(LengthSq(v) > FLT_EPSILON) return glm::normalize(v);
	return vec3(0);
}

inline f32 VecAngle(vec2 v)
{
	f32 a = atan2(v.y, v.x);
	if(a < 0.0f) a += 2 * (f32)PI;
	return a;
}

inline f32 Vec2Cross(vec2 v1, vec2 v2)
{
	return v1.x * v2.y - v1.y * v2.x;
}

inline f32 Vec2Dot(vec2 v1, vec2 v2)
{
	return v1.x * v2.x + v1.y * v2.y;
}

inline bool LineLineIntersection(vec2 la0, vec2 la1, vec2 lb0, vec2 lb1, vec2* tu)
{
	// t = (q − p) × s / (r × s)
	// u = (q − p) × r / (r × s)

	vec2 p = la0;
	vec2 r = la1 - la0;
	vec2 q = lb0;
	vec2 s = lb1 - lb0;

	const f32 c0 = Vec2Cross(q - p, r);
	const f32 c1 = Vec2Cross(r, s);

	if(abs(c0) < PHYS_EPSILON) {
		if(abs(c1) < PHYS_EPSILON) { // collinear
			*tu = vec2(0);
			return true;
		}
		return false; // parallel
	}

	f32 u = c0 / c1;
	f32 t = Vec2Cross(q - p, s) / c1;
	*tu = vec2(t, u);
	return true;
}

inline bool LineSegmentIntersection(vec2 l0, vec2 l1, vec2 s0, vec2 s1, vec2* inters)
{
	vec2 tu;
	bool r = LineLineIntersection(l0, l1, s0, s1, &tu);
	if(!r) return false;

	if(tu.y < 0.0 || tu.y > 1.0) return false;
	*inters = l0 + ((l1 - l0) * tu.x);
	return true;
}

// NOTE: pn needs to be normalized
bool SegmentPlaneIntersection(const vec3& s0, const vec3& s1, const vec3& planeNorm, const vec3& planePoint, vec3* intersPoint);
inline bool LinePlaneIntersection(const vec3& s0, const vec3& s1, const vec3& planeNorm, const vec3& planePoint, vec3* intersPoint)
{
	const vec3 lineDir = glm::normalize(s1 - s0);
	f32 dot = glm::dot(planeNorm, lineDir);
	if(abs(dot) < PHYS_EPSILON) {
		return false; // parallel
	}

	const f32 t = glm::dot(planeNorm, (planePoint - s0) / dot);
	*intersPoint = s0 + lineDir * t;
	return true;
}

inline vec2 ProjectPointOnSegment(const vec2& point, vec2 l0, vec2 l1, const vec2& projDir)
{
	//if(VecAngle(l0 - l1) > VecAngle(l1 - l0)) eastl::swap(l0, l1);
	
	const vec2 lineDir = glm::normalize(l1 - l0);
	const vec2 norm = vec2(glm::cross(vec3(lineDir, 0), vec3(0, 0, 1)));
	f32 dot = glm::dot(norm, projDir);
	if(abs(dot) < PHYS_EPSILON) {
		return l0; // parallel
	}
	
	const f32 projD = glm::dot(point - l0, lineDir);
	const vec2 proj = l0 + projD * lineDir;
	const vec2 A = proj - point;
	const f32 d = glm::length(A) / dot;
	//const f32 lineD = clamp(glm::dot(projDir * d, lineDir) + projD, 0.0f, glm::length(l0 - l1));
	const f32 lineD = glm::dot(projDir * d, lineDir);
	return proj + lineDir * lineD;
}

inline bool IsPointInsideTriangle(const vec3& point, const ShapeTriangle& tri)
{
	const vec3 planeNorm = tri.Normal();
	const vec3 c0 = glm::cross(point - tri.p[0], tri.p[1] - tri.p[0]);
	const vec3 c1 = glm::cross(point - tri.p[1], tri.p[2] - tri.p[1]);
	const vec3 c2 = glm::cross(point - tri.p[2], tri.p[0] - tri.p[2]);
	bool inside = glm::dot(c0, planeNorm) <= 0 && glm::dot(c1, planeNorm) <= 0 && glm::dot(c2, planeNorm) <= 0;
	return inside;
}

// NOTE: planeNorm needs to be normalized
inline vec3 ProjectPointOnPlane(const vec3& point, const vec3& planeNorm, const vec3& planePoint)
{
	vec3 delta = planePoint - point;
	f32 dot = glm::dot(delta, -planeNorm);
	return -planeNorm * dot + point;
}

inline vec2 ClosestPointToTriangle(const vec2& point, const ShapeTriangle& triangle, f32* outDist)
{
	eastl::array<vec2,3> closestList;
	closestList[0] = ClosestPointOnLineSegment(vec2(triangle.p[0]), vec2(triangle.p[1]), point);
	closestList[1] = ClosestPointOnLineSegment(vec2(triangle.p[1]), vec2(triangle.p[2]), point);
	closestList[2] = ClosestPointOnLineSegment(vec2(triangle.p[2]), vec2(triangle.p[0]), point);

	vec2 closest = closestList[0];
	f32 bestDistSq = LengthSq(point - closest);

	f32 dist = LengthSq(closestList[1] - point);
	if(dist < bestDistSq) {
		closest = closestList[1];
		bestDistSq = dist;
	}

	dist = LengthSq(closestList[2] - point);
	if(dist < bestDistSq) {
		closest = closestList[2];
		bestDistSq = dist;
	}

	*outDist = sqrtf(bestDistSq);
	return closest;
}

bool TestIntersection(const ShapeSphere& A, const ShapeSphere& B, PhysPenetrationVector* pen);
bool TestIntersection(const ShapeSphere& A, const ShapeTriangle& B, PhysPenetrationVector* pen);
bool TestIntersection(const ShapeCapsule& A, const ShapeCapsule& B);
bool TestIntersectionUpright(const ShapeCapsule& A, const ShapeCapsule& B, PhysPenetrationVector* pen);
bool TestIntersection(const ShapeCapsule& A, const ShapeTriangle& B, PhysPenetrationVector* pen, vec3* sphereCenter);
bool TestIntersection(const ShapeCylinder& A, const ShapeTriangle& B, PhysResolutionCylinderTriangle* pen);
bool TestIntersection(const ShapeSphere& A, const PhysRect& B, PhysPenetrationVector* pen);

inline vec3 FixCapsuleAlongTriangleNormal(const ShapeCapsule& s, const vec3& sphereCenter, const PhysPenetrationVector& pen, const vec3& norm)
{
	const vec3 point = sphereCenter + pen.dir * (s.radius - (pen.depth + SignedEpsilon(pen.depth) * 5.0f));
	const vec3 projPoint = ProjectVecNorm(point - sphereCenter, norm);
	vec3 triPen = norm * s.radius + projPoint;

	f32 d = glm::dot(norm, s.Normal());
	if(d > PHYS_EPSILON) {
		triPen -= s.InnerBase() - sphereCenter;
	}
	else if(d < -PHYS_EPSILON) {
		triPen -= s.InnerTip() - sphereCenter;
	}

	return triPen;
}

struct ShapeMesh
{
	eastl::vector<ShapeTriangle> triangleList;
};

struct PhysWorld
{
	enum Flags: u32 {
		Disabled = 0x1,
		Grounded = 0x2 // Read Only
	};

	struct MoveComp
	{
		dvec3 pos;
		dvec3 vel;
		u32 flags;
	};

	struct Body
	{
		u32 flags;
		f32 radius;
		f32 height;
		vec3 pos;
		vec3 force;
		vec3 vel;
	};

	typedef ListItT<Body> BodyHandle;

	eastl::fixed_vector<ShapeTriangle, 4096, false> staticMeshTriangleList;
	eastl::fixed_list<Body, 4096, false> dynBodyList;

	// temp data used for compute
	struct Collision
	{
		PhysResolutionCylinderTriangle pen;
		dvec3 triangleNormal;
		dvec3 fix;
		f32 fixLenSq;
	};

	eastl::fixed_vector<MoveComp, 4096, false> bodyList;
	eastl::fixed_vector<ShapeCylinder, 4096, false> shapeCylinderList;
	eastl::fixed_vector<ShapeCylinder, 4096, false> movedShapeCylinderList;
	eastl::fixed_vector<eastl::fixed_vector<Collision,16,false>, 4096, false> collisionList;

#if 1
	u64 step = 0;
	bool bFreezeStep = false;
	bool bShowSubject = true;
	bool bShowFixed = true;
	bool bShowPen = true;
	i32 iSelectedEvent = 0;

	struct CollisionEvent
	{
		u64 step;
		u16 capsuleID;
		u8 ssi;
		u8 cri;
		ShapeCylinder cylinder;
		ShapeTriangle triangle;
		vec3 fix;
		vec3 fix2;
		vec3 vel;
		vec3 fixedVel;
		PhysResolutionCylinderTriangle pen;
	};

	struct PositionRec
	{
		u64 step;
		u16 capsuleID;
		u8 ssi;
		u8 cri;
		ShapeCylinder cylinder;
		vec3 pos;
		vec3 vel;
	};

	eastl::fixed_vector<CollisionEvent, 8192, false> lastStepEvents;
	eastl::fixed_vector<PositionRec, 8192, false> lastStepPositions;
#endif

	void PushStaticMeshes(const ShapeMesh* meshList, const int count);
	BodyHandle CreateBody(f32 radius, f32 height, vec3 pos);
	void DeleteBody(BodyHandle handle);

	void Step();

	vec3 MoveUntilWall(const BodyHandle handle, const vec3& dest);
	vec3 FixCollision(const ShapeCylinder& shape, vec3 pos);
	vec3 SnapToGround(const ShapeCylinder& shape, vec3 pos);
};

bool MakeMapCollisionMesh(const MeshFile::Mesh& mesh, ShapeMesh* out);
