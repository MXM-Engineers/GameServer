#pragma once
#include <common/vector_math.h>
#include <EASTL/array.h>

struct PhysSphere
{
	vec3 pos;
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
	eastl::array<vec3,3> points;
};

struct PhysPenetrationVector
{
	vec3 impact;
	vec3 depth;
};

bool TestIntersection(const PhysSphere& A, const PhysSphere& B, PhysPenetrationVector* pen);
