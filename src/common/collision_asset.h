#pragma once

#include <common/platform.h>

namespace CollisionAsset
{
	static constexpr u16 VERSION = 2;

#pragma pack(push, 1)
	struct Header
	{
		u32 magic;
		u16 version;
		u16 meshCount;
		u16 materialCount;
		u16 actorCount;
	};

	struct Transform
	{
		f32 rotation[9];
		f32 translation[3];
	};

	struct Material
	{
		u32 index;
		f32 dynamicFriction;
		f32 staticFriction;
		f32 restitution;
		f32 dynamicFrictionV;
		f32 staticFrictionV;
		f32 anisotropy[3];
		u32 flags;
		u32 frictionCombine;
		u32 restitutionCombine;
	};

	struct Body
	{
		Transform massLocalPose;
		f32 inertia[3];
		f32 mass;
		f32 linearVelocity[3];
		f32 angularVelocity[3];
		u32 flags;
		f32 wakeUpCounter;
		f32 linearDamping;
		f32 angularDamping;
		f32 maxAngularVelocity;
		f32 ccdMotionThreshold;
		f32 sleepLinearVelocity;
		f32 sleepAngularVelocity;
		u32 solverIterationCount;
		f32 sleepEnergyThreshold;
		f32 sleepDamping;
		f32 contactReportThreshold;
	};

	struct Actor
	{
		char name[64];
		Transform globalPose;
		u32 flags;
		u32 group;
		u32 dominanceGroup;
		u32 contactReportFlags;
		u32 forceFieldMaterial;
		f32 density;
		u32 hasBody;
		u32 shapeCount;
	};

	struct Shape
	{
		char name[64];
		Transform localPose;
		u32 type;
		u32 flags;
		u32 group;
		u32 materialIndex;
		f32 density;
		f32 mass;
		f32 skinWidth;
		u32 nonInteracting;
		u32 groupsMask[4];
		i32 meshIndex;
		f32 dimensions[3];
	};
#pragma pack(pop)

	static_assert(sizeof(Header) == 12);
	static_assert(sizeof(Transform) == 48);
	static_assert(sizeof(Material) == 48);
	static_assert(sizeof(Body) == 136);
	static_assert(sizeof(Actor) == 144);
	static_assert(sizeof(Shape) == 176);
}
