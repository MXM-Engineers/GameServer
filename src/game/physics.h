#pragma once
#include <common/vector_math.h>
#include <common/utils.h>
#include <EASTL/array.h>
#include <EASTL/fixed_list.h>

constexpr f32 PHYS_EPSILON = 0.0001f;

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
	vec3 tip;
	f32 radius;

	inline vec3 Normal() const
	{
		return glm::normalize(tip - base);
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

// on needs to be normalized
inline vec3 ProjectVecNorm(const vec3& v, const vec3& on)
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
	if(LengthSq(v) > 0.001f) return glm::normalize(v);
	return vec2(0);
}

inline vec3 NormalizeSafe(vec3 v)
{
	if(LengthSq(v) > 0.001f) return glm::normalize(v);
	return vec3(0);
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

inline vec2 ProjectPointOnSegment(const vec2& point, const vec2& l0, const vec2& l1, const vec2& projDir)
{
	const vec2 lineDir = glm::normalize(l1 - l0);
	const vec2 norm = vec2(-lineDir.y, lineDir.x);
	const f32 dot = glm::dot(norm, projDir);
	if(abs(dot) < PHYS_EPSILON) {
		return l0; // parallel
	}
	
	const f32 projD = glm::dot(point - l0, lineDir);
	const vec2 proj = l0 + projD * lineDir;
	const vec2 A = proj - point;
	const f32 d = glm::length(A) / dot;
	//f32 lineD = clamp(glm::dot(projDir * d, lineDir), 0.f, glm::length(l0 - l1));
	const f32 lineD = clamp(glm::dot(projDir * d, lineDir) + projD, 0.0f, glm::length(l0 - l1));
	return l0 + lineDir * lineD;
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

bool TestIntersection(const ShapeSphere& A, const ShapeSphere& B, PhysPenetrationVector* pen);
bool TestIntersection(const ShapeSphere& A, const ShapeTriangle& B, PhysPenetrationVector* pen);
bool TestIntersection(const ShapeCapsule& A, const ShapeCapsule& B);
bool TestIntersectionUpright(const ShapeCapsule& A, const ShapeCapsule& B, PhysPenetrationVector* pen);
bool TestIntersection(const ShapeCapsule& A, const ShapeTriangle& B, PhysPenetrationVector* pen, vec3* sphereCenter);
bool TestIntersectionUpright(const ShapeCylinder& A, const ShapeTriangle& B, PhysPenetrationVector* pen, vec3* diskCenter);
bool TestIntersection(const ShapeSphere& A, const PhysRect& B, PhysPenetrationVector* pen);

struct ShapeMesh
{
	eastl::vector<ShapeTriangle> triangleList;
};

struct PhysWorld
{
	enum Flags: u32 {
		Disabled = 0x1
	};

	struct MoveComp
	{
		dvec3 pos;
		dvec3 force;
		dvec3 vel;
	};

	struct BodyCapsule
	{
		u32 flags;
		f32 radius;
		f32 height;
		vec3 pos;
		vec3 force;
		vec3 vel;
	};

	typedef ListItT<BodyCapsule> BodyHandle;

	eastl::fixed_vector<ShapeTriangle, 4096, false> staticMeshTriangleList;
	eastl::fixed_list<BodyCapsule, 4096, false> dynCapsuleBodyList;

	// temp data used for compute
	struct Collision
	{
		PhysPenetrationVector pen;
		dvec3 triangleNormal;
		dvec3 fix;
		f32 fixLenSq;
	};

	eastl::fixed_vector<MoveComp, 4096, false> bodyList;
	eastl::fixed_vector<ShapeCapsule, 4096, false> shapeCapsuleList;
	eastl::fixed_vector<ShapeCapsule, 4096, false> movedShapeCapsuleList;
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
		ShapeCapsule capsule;
		ShapeTriangle triangle;
		vec3 fix;
		vec3 fix2;
		vec3 vel;
		vec3 fixedVel;
		PhysPenetrationVector pen;
	};

	struct PositionRec
	{
		u64 step;
		u16 capsuleID;
		u8 ssi;
		u8 cri;
		ShapeCapsule capsule;
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
};
