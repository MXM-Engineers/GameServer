#pragma once
#include "renderer.h"
#include <mxm/physics.h>

struct CollisionTest
{
	Renderer& rdr;
	Time localTime;

	CollisionTest(Renderer& rdr_): rdr(rdr_)
	{
		localTime = TimeRelNow();
	}

	inline void Draw(const ShapeSphere& sphere, const vec3& color)
	{
		rdr.PushMesh(Pipeline::Wireframe, "Sphere", sphere.center + vec3(0, 0, -sphere.radius), vec3(0), vec3(sphere.radius), color);
	}

	inline void Draw(const ShapeCapsule& capsule, const vec3& color)
	{
		f32 height = glm::length(capsule.tip - capsule.base);
		vec3 dir = glm::normalize(capsule.tip - capsule.base);
		f32 yaw = atan2(dir.y, dir.x);
		f32 pitch = asinf(dir.z) - PI/2.0;

		rdr.PushCapsule(Pipeline::Wireframe, capsule.base, vec3(yaw, pitch, 0), capsule.radius, height, color);
	}

	inline void Draw(const ShapeCylinder& cylinder, const vec3& color, Pipeline pipe = Pipeline::Wireframe)
	{
		f32 height = glm::length(cylinder.height);
		vec3 dir = cylinder.Normal();
		f32 yaw = atan2(dir.y, dir.x);
		f32 pitch = asinf(dir.z) - PI/2.0;

		rdr.PushCylinder(pipe, cylinder.base, vec3(yaw, pitch, 0), cylinder.radius, height, color);
	}

	inline void Draw(const ShapeTriangle& triangle, const vec3& color)
	{
		rdr.triangleBuffer.Push({
			TrianglePoint{ triangle.p[0], ColorU3(color.x, color.y, color.z) },
			TrianglePoint{ triangle.p[1], ColorU3(color.x, color.y, color.z) },
			TrianglePoint{ triangle.p[2], ColorU3(color.x, color.y, color.z) }
		});
	}

	inline void DrawSimpleQuad(const vec3& bl, const vec3& br, const vec3& tr, const vec3& tl, const vec3& color)
	{
		const u32 c = ColorU3(color.x, color.y, color.z);

		rdr.lineBuffer.Push({ bl, c, br, c });
		rdr.lineBuffer.Push({ bl, c, tl, c });
		rdr.lineBuffer.Push({ tl, c, tr, c });
		rdr.lineBuffer.Push({ tr, c, br, c });
	}

	inline void Draw(const PhysRect& rect, const vec3& color, const int quadCount = 3)
	{
		const vec3 up = vec3(0, 0, 1);
		const vec3 vx = -glm::normalize(glm::cross(up, rect.normal));
		const vec3 vy = glm::normalize(glm::cross(vx, rect.normal));
		//DrawVec(vx * 20.f, rect.pos, vec3(1, 0, 0));
		//DrawVec(vy * 20.f, rect.pos, vec3(0, 1, 0));

		const vec3 bl = rect.pos;
		const vec3 br = rect.pos + vx * rect.size.x;
		const vec3 tr = rect.pos + vx * rect.size.x + vy * rect.size.y;
		const vec3 tl = rect.pos + vy * rect.size.y;

		const u32 c = ColorU3(color.x, color.y, color.z);

		DrawSimpleQuad(bl, br, tr, tl, color);

		const vec3 dx = (br - bl) / (quadCount*2.f+1);
		const vec3 dy = (tl - bl) / (quadCount*2.f+1);

		for(int yi = 0; yi < quadCount; yi++) {
			for(int xi = 0; xi < quadCount; xi++) {
				vec3 qbl = rect.pos + dx * (xi*2.f + 1) + dy * (yi*2.f + 1);
				vec3 qbr = rect.pos + dx * (xi*2.f + 2) + dy * (yi*2.f + 1);
				vec3 qtl = rect.pos + dx * (xi*2.f + 1) + dy * (yi*2.f + 2);
				vec3 qtr = rect.pos + dx * (xi*2.f + 2) + dy * (yi*2.f + 2);
				DrawSimpleQuad(qbl, qbr, qtr, qtl, color);
			}
		}
	}

	inline void DrawVec(const vec3& v, const vec3& at, const vec3& color, f32 size = 2)
	{
		rdr.PushArrow(Pipeline::Unlit, at, at + v, color, size);
	}

	inline void Draw(const vec3& at, const vec3& color)
	{
		rdr.PushArrow(Pipeline::Unlit, at + vec3(1, 0, 0), at, color, 1);
	}

	void DoCollisionTests();
	void Render();
};
