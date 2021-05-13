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

bool TestIntersection(const ShapeSphere& A, const ShapeSphere& B, PhysPenetrationVector* pen);
bool TestIntersection(const ShapeSphere& A, const ShapeTriangle& B, PhysPenetrationVector* pen);
bool TestIntersection(const ShapeCapsule& A, const ShapeCapsule& B);
bool TestIntersectionUpright(const ShapeCapsule& A, const ShapeCapsule& B, PhysPenetrationVector* pen);
bool TestIntersection(const ShapeCapsule& A, const ShapeTriangle& B, PhysPenetrationVector* pen, vec3* sphereCenter);
bool TestIntersection(const ShapeSphere& A, const PhysRect& B, PhysPenetrationVector* pen);

struct ShapeMesh
{
	eastl::vector<ShapeTriangle> triangleList;
};

struct PhysBody
{
	vec3 pos;
	vec3 vel;
};

struct PhysWorld
{
	struct DynBodyCapsule
	{
		ShapeCapsule shape;
		PhysBody dyn;
	};

	typedef ListItT<DynBodyCapsule> BodyHandle;

	eastl::fixed_vector<ShapeTriangle, 4096, false> staticMeshTriangleList;
	eastl::fixed_list<DynBodyCapsule, 4096, false> dynCapsuleBodyList;

	// temp data used for compute
	struct Collision
	{
		vec3 triangleNormal;
		vec3 fix;
		f32 fixLenSq;
	};

	eastl::fixed_vector<PhysBody, 4096, false> bodyList;
	eastl::fixed_vector<ShapeCapsule, 4096, false> shapeCapsuleList;
	eastl::fixed_vector<ShapeCapsule, 4096, false> movedShapeCapsuleList;
	eastl::fixed_vector<eastl::fixed_vector<Collision,16,false>, 4096, false> collisionList;

#ifdef CONF_DEBUG
	struct CollisionEvent
	{
		u16 capsuleID;
		u8 ssi;
		u8 cri;
		ShapeCapsule capsule;
		ShapeTriangle triangle;
		vec3 disp;
	};

	eastl::fixed_vector<CollisionEvent, 8192, false> lastStepEvents;
#endif

	void PushStaticMeshes(const ShapeMesh* meshList, const int count);
	BodyHandle CreateBody(f32 radius, f32 height, vec3 pos);
	void DeleteBody(BodyHandle handle);

	void Step();
};
