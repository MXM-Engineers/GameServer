#pragma once
#include <common/base.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

typedef glm::vec2 vec2;
typedef glm::vec3 vec3;
typedef glm::vec4 vec4;
typedef glm::dvec2 dvec2;
typedef glm::dvec3 dvec3;
typedef glm::dvec4 dvec4;
typedef glm::mat4 mat4;

constexpr f64 PI = glm::pi<f64>();

inline vec3 f2v(const float3& f3)
{
	return vec3(f3.x, f3.y, f3.z);
}

inline float3 v2f(const vec3& v3)
{
	return float3{ (f32)v3.x, (f32)v3.y, (f32)v3.z };
}

inline vec2 f2v(const float2& v2)
{
	return vec2(v2.x, v2.y);
}

inline float2 v2f(const vec2& v2)
{
	return float2{ (f32)v2.x, (f32)v2.y };
}
