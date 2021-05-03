#pragma once
#include <common/vector_math.h>
#include <EASTL/array.h>

struct PhysSphere
{
	vec3 center;
	f32 radius;
};

struct PhysCapsule
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

struct PhysTriangle
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
	vec2 min;
	vec2 max;
	vec3 pos;
	vec3 normal; // defines the plane
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

inline vec3 Lerp(const vec3& v1, const vec3& v2, f32 a)
{
	ASSERT(a >= 0 && a <= 1);
	return (v1 * (1.0f - a)) + (v2 * a);
}

bool TestIntersection(const PhysSphere& A, const PhysSphere& B, PhysPenetrationVector* pen);
bool TestIntersection(const PhysSphere& A, const PhysTriangle& B, PhysPenetrationVector* pen);
bool TestIntersection(const PhysCapsule& A, const PhysCapsule& B);
bool TestIntersectionUpright(const PhysCapsule& A, const PhysCapsule& B, PhysPenetrationVector* pen);
bool TestIntersection(const PhysCapsule& A, const PhysTriangle& B, PhysPenetrationVector* pen, vec3* sphereCenter);
bool TestIntersectionUpright(const PhysCapsule& A, const PhysRect& B, PhysPenetrationVector* pen);
