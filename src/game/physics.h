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
	vec3 pos;
	f32 radius;
	f32 height;
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

struct PhysPenetrationVector
{
	vec3 impact;
	vec3 depth;
};

bool TestIntersection(const PhysSphere& A, const PhysSphere& B, PhysPenetrationVector* pen);
bool TestIntersection(const PhysSphere& A, const PhysTriangle& B, PhysPenetrationVector* pen);
