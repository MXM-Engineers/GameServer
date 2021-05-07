#ifdef CONF_WINDOWS
#include "window.h"

#include <common/vector_math.h>
#include <game/config.h>
#include <eathread/eathread_thread.h>
#include <imgui.h>


#define SOKOL_NO_ENTRY
#define SOKOL_D3D11

#define SG_DEFAULT_CLEAR_RED 0.2f
#define SG_DEFAULT_CLEAR_GREEN 0.2f
#define SG_DEFAULT_CLEAR_BLUE 0.2f

#define SOKOL_ASSERT ASSERT
#define SOKOL_LOG LOG

#include "renderer.h"

#define SOKOL_IMPL
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_imgui.h"

#include <game/physics.h>
#include <glm/gtx/vector_angle.hpp>

struct GameState
{
	eastl::fixed_vector<Dbg::Entity,2048,true> entityList;
	Mutex mutex;

	void NewFrame()
	{
		const LockGuard lock(mutex);
		entityList.clear();
	}

	void PushEntity(const Dbg::Entity& entity)
	{
		const LockGuard lock(mutex);
		entityList.push_back(entity);
	}
};

struct CollisionTest
{
	Renderer& rdr;
	Time localTime;

	CollisionTest(Renderer& rdr_): rdr(rdr_)
	{
		localTime = TimeRelNow();
	}

	inline void Draw(const PhysSphere& sphere, const vec3& color)
	{
		rdr.PushMesh(Pipeline::Wireframe, "Sphere", sphere.center + vec3(0, 0, -sphere.radius), vec3(0), vec3(sphere.radius), color);
	}

	inline void Draw(const PhysCapsule& capsule, const vec3& color)
	{
		f32 height = glm::length(capsule.tip - capsule.base);
		vec3 dir = glm::normalize(capsule.tip - capsule.base);
		f32 yaw = atan2(dir.y, dir.x);
		f32 pitch = asinf(dir.z) - PI/2.0;

		rdr.PushCapsule(Pipeline::Wireframe, capsule.base, vec3(yaw, pitch, 0), capsule.radius, height, color);
	}

	inline void Draw(const PhysTriangle& triangle, const vec3& color)
	{
		rdr.triangleBuffer.Push({
			TrianglePoint{ triangle.p[0], CU3(color.x, color.y, color.z) },
			TrianglePoint{ triangle.p[1], CU3(color.x, color.y, color.z) },
			TrianglePoint{ triangle.p[2], CU3(color.x, color.y, color.z) }
		});
	}

	inline void DrawSimpleQuad(const vec3& bl, const vec3& br, const vec3& tr, const vec3& tl, const vec3& color)
	{
		const u32 c = CU3(color.x, color.y, color.z);

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

		const u32 c = CU3(color.x, color.y, color.z);

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

	inline void DrawVec(const vec3& v, const vec3& at, const vec3& color)
	{
		rdr.PushArrow(Pipeline::Unlit, at, at + v, color, 2);
	}

	inline void Draw(const vec3& at, const vec3& color)
	{
		rdr.PushArrow(Pipeline::Unlit, at + vec3(5, 0, 0), at, color, 2);
	}

	void DoCollisionTests()
	{
		f64 a = (u64)localTime / 10000000000.0;
		f64 s = saw(a);

		// test 1
		{
			static bool bShowFixedSphere = true;
			static bool bAutoMove = true;
			static f32 fOffsetX = 0;

			if(ImGui::Begin("Test 1")) {
				ImGui::Checkbox("Fixed sphere", &bShowFixedSphere);
				ImGui::Checkbox("Auto move", &bAutoMove);
				ImGui::SliderFloat("OffsetX", &fOffsetX, 0, 300);
			}
			ImGui::End();

			PhysSphere sphereA;
			PhysSphere sphereB;

			const vec3 off = bAutoMove ? vec3(220 + saw(a * .5) * -220, 0, 0) : vec3(fOffsetX, 0, 0);

			sphereA.center = vec3(200, 150, 50) + off;
			sphereA.radius = 50;
			sphereB.center = vec3(310, 150, 50);
			sphereB.radius = 50;

			PhysPenetrationVector pen;
			bool intersect = TestIntersection(sphereA, sphereB, &pen);
			if(intersect) {
				Draw(sphereA, vec3(1, 0.5, 0.8));
				Draw(sphereB, vec3(1, 0.2, 1));
				DrawVec(-pen.dir * pen.depth, sphereA.center + pen.dir * sphereA.radius, vec3(1, 1, 0));

				if(bShowFixedSphere) {
					PhysSphere fixed = sphereA;
					fixed.center -= pen.dir * pen.depth;
					Draw(fixed, vec3(1, 0.5, 0));
				}
			}
			else {
				Draw(sphereA, vec3(0, 0.5, 0.8));
				Draw(sphereB, vec3(0, 0.2, 1)  );
			}
		}


		// test 2
		{
			static bool bShowFixedSphere = true;
			static bool bAutoMove = true;
			static f32 fOffsetX = 166.520f;
			static f32 fOffsetY = 0;

			const bool open = ImGui::Begin("Test 2");
			if(open) {
				ImGui::Checkbox("Fixed sphere", &bShowFixedSphere);
				ImGui::Checkbox("Auto move", &bAutoMove);
				ImGui::SliderFloat("OffsetX", &fOffsetX, 0, 300);
				ImGui::SliderFloat("OffsetY", &fOffsetY, -150, 150);
			}


			PhysSphere sphereA;
			PhysTriangle triangleB;

			sphereA.center = vec3(100, 349, 50);
			sphereA.radius = 50;
			triangleB.p = {
				vec3(200, 380, 30),
				vec3(300, 350, 40),
				vec3(250, 350, 100),
			};

			const vec3 off = bAutoMove ? vec3(saw(a * .5) * -300, 0, 0) : vec3(-fOffsetX, fOffsetY, 0);
			foreach(p, triangleB.p) {
				*p += off;
			}

			PhysPenetrationVector pen;
			bool intersect = TestIntersection(sphereA, triangleB, &pen);

			if(open) {
				ImGui::Text("%g", pen.depth);
				vec3 center = triangleB.Center();
				DrawVec(triangleB.Normal() * 20.0f, center, vec3(1, 1, 1));
			}

			if(intersect) {
				Draw(sphereA, vec3(1, 0.5, 0.8));
				Draw(triangleB, vec3(1, 0.2, 1));
				DrawVec(-pen.dir * pen.depth, sphereA.center + pen.dir * sphereA.radius, vec3(1, 1, 0));

				if(bShowFixedSphere) {
					vec3 p = sphereA.center + pen.dir * (sphereA.radius - pen.depth);
					Draw(p, vec3(1, 0.8, 0));
					vec3 pp = glm::dot(p - sphereA.center, triangleB.Normal()) * triangleB.Normal();
					DrawVec(pp, sphereA.center, vec3(0.5, 1, 0.5));
					vec3 pp2 = triangleB.Normal() * sphereA.radius + pp;
					DrawVec(pp2, sphereA.center + -triangleB.Normal() * sphereA.radius, vec3(0.5, 0.5, 1));

					PhysSphere fixed = sphereA;
					fixed.center += pp2;
					Draw(fixed, vec3(0.5, 1, 0.5));
				}
			}
			else {
				Draw(sphereA, vec3(0, 0.5, 0.8));
				Draw(triangleB, vec3(0, 0.2, 1));
			}

			ImGui::End();
		}

		// test3
		{
			static bool bAutoMove = true;
			static f32 fOffsetX = 166.520f;

			const bool open = ImGui::Begin("Test 3");
			if(open) {
				ImGui::Checkbox("Auto move", &bAutoMove);
				ImGui::SliderFloat("OffsetX", &fOffsetX, 0, 300);
			}
			ImGui::End();

			PhysCapsule capsuleA;
			PhysCapsule capsuleB;

			capsuleA.base = vec3(100, 500, 0);
			capsuleA.tip = vec3(120, 500, 100);
			capsuleA.radius = 20;

			const vec3 off = bAutoMove ? vec3(0, 0, (100 + saw(a * .5) * -100)) : vec3(0, 0, fOffsetX);
			capsuleB.base = vec3(100, 450, 0) + off;
			capsuleB.tip = vec3(100, 600, 0);
			capsuleB.radius = 30;

			bool intersect = TestIntersection(capsuleA, capsuleB);

			if(intersect) {
				Draw(capsuleA, vec3(1, 0.5, 0.8));
				Draw(capsuleB, vec3(1, 0.2, 1));
			}
			else {
				Draw(capsuleA, vec3(0, 0.5, 0.8));
				Draw(capsuleB, vec3(0, 0.2, 1));
			}
		}

		// test3.1
		{
			static bool bFixedSphere = true;
			static bool bAutoMove = true;
			static f32 fOffsetX = 0;
			static f32 fOffsetZ = 0;

			const bool open = ImGui::Begin("Test 3.1");
			if(open) {
				ImGui::Checkbox("Fixed capsule", &bFixedSphere);
				ImGui::Checkbox("Auto move", &bAutoMove);
				ImGui::SliderFloat("OffsetX", &fOffsetX, 0, 300);
				ImGui::SliderFloat("OffsetZ", &fOffsetZ, 0, 300);
			}
			ImGui::End();

			PhysCapsule capsuleA;
			PhysCapsule capsuleB;

			capsuleA.base = vec3(300, 500, 0);
			capsuleA.tip = vec3(300, 500, 100);
			capsuleA.radius = 20;

			const vec3 off = bAutoMove ? vec3(saw(a * .5) * -200, 0, 0) : vec3(-fOffsetX, 0, -fOffsetZ);
			capsuleB.base = vec3(400, 500, 0) + off;
			capsuleB.tip = vec3(400, 500, 100) + off;
			capsuleB.radius = 30;

			PhysPenetrationVector pen;
			bool intersect = TestIntersectionUpright(capsuleA, capsuleB, &pen);

			if(intersect) {
				Draw(capsuleA, vec3(1, 0.5, 0.8));
				Draw(capsuleB, vec3(1, 0.2, 1));
				DrawVec(-pen.dir * pen.depth, capsuleA.base, vec3(1, 1, 0));

				if(bFixedSphere) {
					PhysCapsule fixed = capsuleA;
					fixed.base += -pen.dir * pen.depth;
					fixed.tip += -pen.dir * pen.depth;
					Draw(fixed, vec3(1, 0.5, 0));
				}
			}
			else {
				Draw(capsuleA, vec3(0, 0.5, 0.8));
				Draw(capsuleB, vec3(0, 0.2, 1));
			}
		}

		// test4
		{
			static bool bShowLinePlaneInter = true;
			static bool bShowRefPoint = true;
			static bool bShowSphere = true;
			static bool bFixedSphere = true;
			static bool bAutoMove = true;
			static f32 fOffsetX = 40;
			static f32 fTriangleOffset = 150;

			if(ImGui::Begin("Test 4")) {
				ImGui::Checkbox("Line plane intersection", &bShowLinePlaneInter);
				ImGui::Checkbox("Reference point", &bShowRefPoint);
				ImGui::Checkbox("Collision sphere", &bShowSphere);
				ImGui::Checkbox("Fixed capsule", &bFixedSphere);
				ImGui::Checkbox("Auto move", &bAutoMove);
				ImGui::SliderFloat("Offset", &fOffsetX, 0, 300);
				ImGui::SliderFloat("Triangle Offset", &fTriangleOffset, 0, 300);
			}
			ImGui::End();


			PhysCapsule capsuleA;
			PhysTriangle triangleB;

			const vec3 off = bAutoMove ? vec3(210 + saw(a * .5) * -210, 0, 0) : vec3(fOffsetX, 0, 0);
			capsuleA.base = vec3(100, 750, 0) + off;
			capsuleA.tip = vec3(100, 750, 100) + off;
			capsuleA.radius = 20;

			// clipping
			/*
			triangleB.p = {
				vec3(200, 680, 0),
				vec3(300, 730, 80),
				vec3(200, 750, 0)
			};
			*/

			// inside
			triangleB.p = {
				vec3(200, 680, 0),
				vec3(300 - fTriangleOffset, 750, 80),
				vec3(200, 780, 0)
			};

			// --------------------------------------------------------------------------------------
			const PhysCapsule& A = capsuleA;
			const PhysTriangle& B = triangleB;

			const vec3 capsuleNorm = glm::normalize(A.tip - A.base);
			const vec3 lineEndOffset = capsuleNorm * A.radius;
			const vec3 pointA = A.base + lineEndOffset;
			const vec3 pointB = A.tip - lineEndOffset;

			// ray-plane intersection
			const vec3 planeNorm = B.Normal();
			f32 planeCapsuleDot = glm::dot(planeNorm, capsuleNorm);
			const bool parallel = (planeCapsuleDot == 0);
			if(parallel) {
				planeCapsuleDot = 1;
			}
			const f32 t = glm::dot(planeNorm, (B.p[0] - A.base) / planeCapsuleDot);
			const vec3 linePlaneIntersection = A.base + capsuleNorm * t;

			if(bShowLinePlaneInter) {
				Draw(linePlaneIntersection, vec3(1, 1, 1));
			}

			vec3 refPoint;

			// Determine whether linePlaneIntersection is inside all triangle edges
			const vec3 c0 = glm::cross(linePlaneIntersection - B.p[0], B.p[1] - B.p[0]);
			const vec3 c1 = glm::cross(linePlaneIntersection - B.p[1], B.p[2] - B.p[1]);
			const vec3 c2 = glm::cross(linePlaneIntersection - B.p[2], B.p[0] - B.p[2]);
			bool inside = glm::dot(c0, planeNorm) <= 0 && glm::dot(c1, planeNorm) <= 0 && glm::dot(c2, planeNorm) <= 0;

			if(inside) {
				refPoint = linePlaneIntersection;

				if(bShowRefPoint) {
					Draw(refPoint, vec3(1, 0.5, 0.5));
				}
			}
			else {
				// Edge 1
				vec3 point1 = ClosestPointOnLineSegment(B.p[0], B.p[1], linePlaneIntersection);
				vec3 v1 = linePlaneIntersection - point1;
				f32 distsq = LengthSq(v1);
				f32 bestDist = distsq;
				refPoint = point1;

				// Edge 2
				vec3 point2 = ClosestPointOnLineSegment(B.p[1], B.p[2], linePlaneIntersection);
				vec3 v2 = linePlaneIntersection - point2;
				distsq = LengthSq(v2);
				if(distsq < bestDist) {
					refPoint = point2;
					bestDist = distsq;
				}

				// Edge 3
				vec3 point3 = ClosestPointOnLineSegment(B.p[2], B.p[0], linePlaneIntersection);
				vec3 v3 = linePlaneIntersection - point3;
				distsq = LengthSq(v3);
				if(distsq < bestDist) {
					refPoint = point3;
					bestDist = distsq;
				}

				if(bShowRefPoint) {
					Draw(refPoint, vec3(1, 0, 0));
				}
			}

			// The center of the best sphere candidate
			vec3 center = ClosestPointOnLineSegment(pointA, pointB, refPoint);

			const PhysSphere sphere = { center, A.radius };

			rdr.PushLine(capsuleA.base - capsuleNorm * 1000.f, capsuleA.base + capsuleNorm * 1000.f, vec3(1));

			if(bShowSphere) {
				Draw(sphere, vec3(1));
			}

			PhysPenetrationVector pen;
			vec3 sphereCenter;
			bool intersect = TestIntersection(capsuleA, triangleB, &pen, &sphereCenter);

			DrawVec(triangleB.Normal() * 20.0f, triangleB.Center(), vec3(1, 1, 1));

			if(intersect) {
				Draw(capsuleA, vec3(1, 0.5, 0.8));
				Draw(triangleB, vec3(1, 0.2, 1));
				DrawVec(-pen.dir * pen.depth, sphereCenter, vec3(1, 1, 0));

				if(bFixedSphere) {
					vec3 p = sphereCenter + pen.dir * (capsuleA.radius - pen.depth);
					//Draw(p, vec3(1, 0.8, 0));
					vec3 pp = glm::dot(p - sphereCenter, triangleB.Normal()) * triangleB.Normal();
					//DrawVec(pp, sphereCenter, vec3(0.5, 1, 0.5));
					vec3 pp2 = triangleB.Normal() * capsuleA.radius + pp;
					//DrawVec(pp2, sphereCenter + -triangleB.Normal() * capsuleA.radius, vec3(0.5, 0.5, 1));

					f32 d = glm::dot(triangleB.Normal(), capsuleNorm);
					if(d > 0) {
						pp2 -= capsuleA.InnerBase() - sphereCenter;
					}
					else if(d < 0) {
						pp2 -= capsuleA.InnerTip() - sphereCenter;
					}

					PhysCapsule fixed = capsuleA;
					fixed.base += pp2;
					fixed.tip += pp2;
					Draw(fixed, vec3(0.5, 1, 0.5));
				}
			}
			else {
				Draw(capsuleA, vec3(0, 0.5, 0.8));
				Draw(triangleB, vec3(0, 0.2, 1));
			}
		}

		// test5
		{
			static bool bFixedSphere = true;
			static bool bAutoMove = true;
			static f32 fOffsetX = 166.520f;
			static f32 fOffsetY = 0;

			if(ImGui::Begin("Test 5")) {
				ImGui::Checkbox("Fixed capsule", &bFixedSphere);
				ImGui::Checkbox("Auto move", &bAutoMove);
				ImGui::SliderFloat("OffsetX", &fOffsetX, 0, 300);
				ImGui::SliderFloat("OffsetY", &fOffsetY, -200, 200);
			}
			ImGui::End();

			PhysCapsule capsuleA;
			PhysTriangle triangleB;

			const vec3 off = bAutoMove ? vec3(210 + saw(a * .5) * -210, 0, 0) : vec3(fOffsetX, 0, 0);
			capsuleA.base = vec3(100, 900, 0) + off;
			capsuleA.tip = vec3(100, 900, 100) + off;
			capsuleA.radius = 20;

			const vec3 triOff = bAutoMove ? vec3(0, 0, -100 + saw(a) * 200) : vec3(0, 0, fOffsetY);
			triangleB.p = {
				vec3(200, 850, 0) + triOff,
				vec3(200, 900, 80) + triOff,
				vec3(200, 950, 0) + triOff
			};

			PhysPenetrationVector pen;
			vec3 sphereCenter;
			bool intersect = TestIntersection(capsuleA, triangleB, &pen, &sphereCenter);

			if(intersect) {
				Draw(capsuleA, vec3(1, 0.5, 0.8));
				Draw(triangleB, vec3(1, 0.2, 1));
				DrawVec(-pen.dir * pen.depth, sphereCenter, vec3(1, 1, 0));

				if(bFixedSphere) {
					vec3 p = sphereCenter + pen.dir * (capsuleA.radius - pen.depth);
					Draw(p, vec3(1, 0.8, 0));
					vec3 pp = glm::dot(p - sphereCenter, triangleB.Normal()) * triangleB.Normal();
					DrawVec(pp, sphereCenter, vec3(0.5, 1, 0.5));
					vec3 pp2 = triangleB.Normal() * capsuleA.radius + pp;
					DrawVec(pp2, sphereCenter + -triangleB.Normal() * capsuleA.radius, vec3(0.5, 0.5, 1));

					f32 d = glm::dot(triangleB.Normal(), capsuleA.Normal());
					if(d > 0) {
						pp2 -= capsuleA.InnerBase() - sphereCenter;
					}
					else if(d < 0) {
						pp2 -= capsuleA.InnerTip() - sphereCenter;
					}


					PhysCapsule fixed = capsuleA;
					fixed.base += pp2;
					fixed.tip += pp2;
					Draw(fixed, vec3(0.5, 1, 0.5));
				}
			}
			else {
				Draw(capsuleA, vec3(0, 0.5, 0.8));
				Draw(triangleB, vec3(0, 0.2, 1));
			}
		}

		// test6
		{
			static bool bFixedSphere = true;
			static bool bAutoMove = true;
			static f32 fOffsetX = 113.077f;
			static f32 fRectX = -15.f;
			static f32 fRectY = -59.423f;

			const bool open = ImGui::Begin("Test 6");
			if(open) {
				ImGui::Checkbox("Fixed sphere", &bFixedSphere);
				ImGui::Checkbox("Auto move", &bAutoMove);
				ImGui::SliderFloat("OffsetX", &fOffsetX, 0, 300);
				ImGui::SliderFloat("RectX", &fRectX, -150, 150);
				ImGui::SliderFloat("RectY", &fRectY, -150, 150);
			}

			PhysSphere sphereA;
			PhysRect rectB;

			const vec3 off = bAutoMove ? vec3(210 + saw(a * .5) * -210, 0, 0) : vec3(fOffsetX, 0, 0);
			sphereA.center = vec3(100, 1200, 0) + off;
			sphereA.radius = 20;

			rectB.size = vec2(100, 100);
			rectB.pos = vec3(200, 1100 + fRectX, fRectY);
			rectB.normal = vec3(-1, 0, 0);

			do {
				const PhysSphere& A = sphereA;
				const PhysRect& B = rectB;

				const vec3 planeNorm = B.normal;
				f32 signedDistToPlane = glm::dot(A.center - B.pos, planeNorm);

				// does not intersect plane
				if(signedDistToPlane < -A.radius || signedDistToPlane > A.radius) {
					break;
				}

				// projected sphere center on plane
				const vec3 projSphereCenter = A.center - planeNorm * signedDistToPlane;

				Draw(projSphereCenter, vec3(1, 0, 0));

				// project on 2D coordinates and check if inside rectangle
				const vec3 up = vec3(0, 0, 1);
				const vec3 vx = -glm::normalize(glm::cross(up, B.normal));
				const vec3 vy = glm::normalize(glm::cross(vx, B.normal));

				const f32 halfW = B.size.x * 0.5f;
				const f32 halfH = B.size.y * 0.5f;
				const f32 dotx = abs(glm::dot(projSphereCenter - B.pos, vx) - halfW);
				const f32 doty = abs(glm::dot(projSphereCenter - B.pos, vy) - halfH);

				// compute projected circle radius onto 2D rectangle plane
				const f32 dist = abs(signedDistToPlane);
				const f32 sb = sqrtf(A.radius*A.radius - dist*dist);

				if(open) {
					ImGui::Text("dotx = %g", dotx);
					ImGui::Text("doty = %g", doty);
					ImGui::Text("dist = %g", dist);
					ImGui::Text("sb = %g", sb);
					rdr.PushLine(projSphereCenter, projSphereCenter + -vx * sb, vec3(0.5, 1, 0.5));
					rdr.PushLine(projSphereCenter, projSphereCenter + -vy * sb, vec3(0.5, 1, 0.5));
				}

			} while(0);


			DrawVec(rectB.normal * 20.f, rectB.Center(), vec3(1, 1, 1));

			PhysPenetrationVector pen;
			bool intersect = TestIntersection(sphereA, rectB, &pen);

			if(intersect) {
				Draw(sphereA, vec3(1, 0.5, 0.8));
				Draw(rectB, vec3(1, 0.2, 1), 12);
				DrawVec(-pen.dir * pen.depth, sphereA.center, vec3(1, 1, 0));

				if(bFixedSphere) {
					vec3 p = sphereA.center + pen.dir * (sphereA.radius - pen.depth);
					//Draw(p, vec3(1, 0.8, 0));
					vec3 pp = glm::dot(p - sphereA.center, rectB.normal) * rectB.normal;
					//DrawVec(pp, sphereA.center, vec3(0.5, 1, 0.5));
					vec3 pp2 = rectB.normal * sphereA.radius + pp;
					//DrawVec(pp2, sphereA.center + -rectB.normal * sphereA.radius, vec3(0.5, 0.5, 1));


					PhysSphere fixed = sphereA;
					fixed.center += pp2;
					Draw(fixed, vec3(0.5, 1, 0.5));
				}
			}
			else {
				Draw(sphereA, vec3(0, 0.5, 0.8));
				Draw(rectB, vec3(0, 0.2, 1), 12);
			}

			ImGui::End();
		}
	}

	void Render()
	{
		Time lastLocalTime = localTime;
		localTime = TimeRelNow();
		const f64 delta = TimeDiffSec(TimeDiff(lastLocalTime, localTime));
		lastLocalTime = localTime;

		DoCollisionTests();
	}
};

bool MakeMapCollisionMesh(const MeshFile::Mesh& mesh, PhysMapMesh* out)
{
	out->triangleList.reserve(mesh.indexCount/3);

	// triangles
	for(int i = 0; i < mesh.indexCount; i += 3) {
		const MeshFile::Vertex& vert0 = mesh.vertices[mesh.indices[i]];
		const MeshFile::Vertex& vert1 = mesh.vertices[mesh.indices[i+1]];
		const MeshFile::Vertex& vert2 = mesh.vertices[mesh.indices[i+2]];
		const vec3 v0(vert0.px, vert0.py, vert0.pz);
		const vec3 v1(vert1.px, vert1.py, vert1.pz);
		const vec3 v2(vert2.px, vert2.py, vert2.pz);

		PhysTriangle tri;
		tri.p = { v0, v2, v1 };
		out->triangleList.push_back(tri);
	}

	return true;
}

struct Window
{
	const i32 winWidth;
	const i32 winHeight;
	EA::Thread::Thread thread;

	Time startTime = Time::ZERO;
	Time localTime = Time::ZERO;

	Renderer rdr;

	GameState game;
	GameState lastGame;

	CollisionTest collisionTest;

	MeshFile mfCollision;
	MeshFile mfEnv;
	PhysMapMesh mapCollision;
	PhysMapMesh mapWalls;

	bool ui_bCollisionTests = false;
	bool ui_bMapWireframe = true;

	Window(i32 width, i32 height):
		winWidth(width),
		winHeight(height),
		rdr{width, height},
		collisionTest(rdr)
	{

	}

	bool Init();
	void Update(f64 delta);
	void Frame();
	void OnEvent(const sapp_event& event);
	void Cleanup();
};

bool Window::Init()
{
	startTime = TimeNow();

	bool r = rdr.Init();
	if(!r) {
		return false;
	}

	simgui_desc_t imguiDesc = {0};
	imguiDesc.ini_filename = "gameserver_imgui.ini";
	simgui_setup(&imguiDesc);

	r = OpenMeshFile("gamedata/PVP_DeathMatch01_Collision.msh", &mfCollision);
	if(!r) return false;
	r = OpenMeshFile("gamedata/PVP_DeathMatch01_Env.msh", &mfEnv);
	if(!r) return false;

	const MeshFile::Mesh& mshCol = mfCollision.meshList.front();
	rdr.LoadMeshFile("PVP_DeathMatchCollision", mshCol);

	foreach_const(it, mfEnv.meshList) {
		rdr.LoadMeshFile(it->name.data(), *it);
	}

	r = MakeMapCollisionMesh(mshCol, &mapCollision);
	if(!r) return false;
	r = MakeMapCollisionMesh(mfEnv.meshList.front(), &mapWalls);
	if(!r) return false;

	return true;
}

void Window::Update(f64 delta)
{
	//ImGui::ShowDemoWindow();

	if(ImGui::BeginMainMenuBar()) {
		if(ImGui::BeginMenu("View")) {
			ImGui::MenuItem("Collision tests", "", &ui_bCollisionTests);
			ImGui::MenuItem("Map wireframe", "", &ui_bMapWireframe);
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	const f64 t = TimeDiffSec(TimeDiff(startTime, localTime));
	// test meshes
	// rdr.PushMesh(Pipeline::Shaded, { "Capsule", vec3(0, 0, 0), vec3(0), vec3(1), vec3(1, 0.5, 0) });
	// rdr.PushMesh(Pipeline::Shaded, { "Ring", vec3(0, 0, 0), vec3(0), vec3(1), vec3(1, 0.5, 0) });
	// rdr.PushMesh(Pipeline::Shaded, "Cone", vec3(0, 0, 0), vec3(0), vec3(100), vec3(1, 0.5, 0));
	// DrawArrow(vec3(200, 0, 0), vec3(300, 400, 500), vec3(1), 20);
	// rdr.PushMesh(Pipeline::Shaded, "Cylinder", vec3(0, 0, 0), vec3(0), vec3(100), vec3(1, 0.5, 0));
	// rdr.PushMesh(Pipeline::Shaded, "Sphere", vec3(0, 0, 0), vec3(0), vec3(100), vec3(1, 0.5, 0));
	// rdr.PushCapsule(Pipeline::Unlit, vec3(100, 100, 0), vec3(0), 50, 250, vec3(0.8, 0.2, 0.35));
	// auto parent = rdr.PushAnchor(vec3(-100, -100, 0), vec3(0), vec3(10));
	// rdr.PushMesh(Pipeline::Shaded, "Sphere", vec3(0), vec3(0), vec3(1), vec3(1), parent);
	rdr.PushCapsule(Pipeline::Wireframe, vec3(-100, -100, 0), vec3(t, 0, 0), 20, 100, vec3(0.2, 0.5, 0.2));

	// map
	rdr.PushMesh(Pipeline::Shaded, "PVP_DeathMatchCollision", vec3(0, 0, 0), vec3(0, 0, 0), vec3(1), vec3(0.2, 0.3, 0.3));
	rdr.PushMesh(Pipeline::Shaded, "PVP_Deathmatch01_GuardrailMob", vec3(0, 0, 0), vec3(0, 0, 0), vec3(1), vec3(0.2, 0.3, 0.5));

	if(ui_bMapWireframe) {
		foreach_const(it, mapCollision.triangleList) {
			const PhysTriangle& tri = *it;
			const vec3 color = vec3(0.5, 1, 0.5);
			rdr.PushLine(tri.p[0], tri.p[1], color);
			rdr.PushLine(tri.p[0], tri.p[2], color);
			rdr.PushLine(tri.p[1], tri.p[2], color);
			rdr.PushArrow(Pipeline::Unlit, tri.Center(), tri.Center() + tri.Normal() * 100.f, color, 5);
		}
		foreach_const(it, mapWalls.triangleList) {
			const PhysTriangle& tri = *it;
			const vec3 color = vec3(0.5, 0.5, 1.0);
			rdr.PushLine(tri.p[0], tri.p[1], color);
			rdr.PushLine(tri.p[0], tri.p[2], color);
			rdr.PushLine(tri.p[1], tri.p[2], color);
			rdr.PushArrow(Pipeline::Unlit, tri.Center(), tri.Center() + tri.Normal() * 100.f, color, 5);
		}
	}

	// @Speed: very innefficient locking
	// Also some blinking because sometimes the gamestate is empty when we render it (NewFrame before adding anything)
	decltype(GameState::entityList) entityList;
	{
		const LockGuard lock(game.mutex);
		entityList = game.entityList;
	}

	foreach_const(ent, entityList) {
		const Dbg::Entity& e = *ent;
		rdr.PushCapsule(Pipeline::Shaded, e.pos, vec3(0), 50, 250, e.color);
		rdr.PushMesh(Pipeline::Unlit, "Ring", e.pos, vec3(0), vec3(50), e.color);

		// this is updated when the player moves
		const vec3 upperStart = e.pos + vec3(0, 0, 200);
		const vec3 upperDir = glm::normalize(vec3(cosf(e.rot.upperYaw), sinf(e.rot.upperYaw), sinf(e.rot.upperPitch)));
		rdr.PushArrow(Pipeline::Unlit, upperStart, upperStart + upperDir * 150.f, vec3(1, 0.4, 0.1), 10);

		const vec3 bodyRotStart = e.pos + vec3(0, 0, 150);
		rdr.PushArrow(Pipeline::Unlit, bodyRotStart, bodyRotStart + vec3(cosf(e.rot.bodyYaw), sinf(e.rot.bodyYaw), 0) * 150.f, vec3(1, 0.4, 0.1), 10);
		const vec3 dirStart = e.pos + vec3(0, 0, 80);
		rdr.PushArrow(Pipeline::Unlit, dirStart, dirStart + glm::normalize(vec3(e.moveDir.x, e.moveDir.y, 0)) * 200.0f, vec3(0.2, 1, 0.2), 10);
	}

	if(ui_bCollisionTests) {
		collisionTest.Render();
	}

	const ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable;


	if(ImGui::Begin("Entities")) {
		if(ImGui::BeginTable("EntityList", 8, flags))
		{
			ImGui::TableSetupColumn("UID");
			ImGui::TableSetupColumn("Name");
			ImGui::TableSetupColumn("PosX");
			ImGui::TableSetupColumn("PosY");
			ImGui::TableSetupColumn("PosZ");
			ImGui::TableSetupColumn("RotUpperYaw");
			ImGui::TableSetupColumn("RotUpperPitch");
			ImGui::TableSetupColumn("RotBodyYaw");
			ImGui::TableHeadersRow();

			foreach_const(ent, entityList) {
				ImGui::TableNextRow();

				ImGui::TableNextColumn();
				ImGui::Text("%u", ent->UID);
				ImGui::TableNextColumn();
				ImGui::Text("%ls", ent->name.data());

				ImGui::TableNextColumn();
				ImGui::Text("%.2f", ent->pos.x);
				ImGui::TableNextColumn();
				ImGui::Text("%.2f", ent->pos.y);
				ImGui::TableNextColumn();
				ImGui::Text("%.2f", ent->pos.z);

				ImGui::TableNextColumn();
				ImGui::Text("%.2f", ent->rot.upperYaw);
				ImGui::TableNextColumn();
				ImGui::Text("%.2f", ent->rot.upperPitch);
				ImGui::TableNextColumn();
				ImGui::Text("%.2f", ent->rot.bodyYaw);
			}

			ImGui::EndTable();
		}
	}
	ImGui::End();
}

void Window::Frame()
{
	Time lastLocalTime = localTime;
	Time now = TimeNow();
	localTime = TimeDiff(startTime, now);
	const f64 delta = TimeDiffSec(TimeDiff(lastLocalTime, localTime));
	lastLocalTime = localTime;

	simgui_new_frame(winWidth, winHeight, delta);

	Update(delta);
	rdr.Render(delta);
}

void Window::OnEvent(const sapp_event& event)
{
	// quit on escape
	if(event.type == SAPP_EVENTTYPE_KEY_DOWN) {
		if(event.key_code == sapp_keycode::SAPP_KEYCODE_ESCAPE) {
			sapp_request_quit();
			return;
		}
	}

	if(!rdr.camera.mouseLocked) {
		simgui_handle_event(&event);
	}

	rdr.OnEvent(event);
}

void Window::Cleanup()
{
	rdr.Cleanup();
	simgui_shutdown();
	sg_shutdown();
}

static Window* g_pWindow = nullptr;

void WindowInit()
{
	bool r = g_pWindow->Init();
	if(!r) {
		LOG("ERROR: failed to init window");
		sapp_quit();
	}
}

void WindowFrame()
{
	g_pWindow->Frame();
}

void WindowEvent(const sapp_event* event)
{
	g_pWindow->OnEvent(*event);
}

void WindowCleanup()
{
	g_pWindow->Cleanup();
}

intptr_t ThreadWindow(void* pData)
{
	ProfileSetThreadName("Window");
	const i32 cpuID = 0;
	EA::Thread::SetThreadAffinityMask(1 << cpuID);

	sapp_desc desc;
	memset(&desc, 0, sizeof(desc));
	desc.window_title = "MxM server";
	// MSAA
	desc.sample_count = 8;
	desc.width = g_pWindow->winWidth;
	desc.height = g_pWindow->winHeight;
	desc.init_cb = WindowInit;
	desc.frame_cb = WindowFrame;
	desc.event_cb = WindowEvent;
	desc.cleanup_cb = WindowCleanup;
	sapp_run(&desc);

	LOG("Window closed.");
	GenerateConsoleCtrlEvent(0, 0); // so the whole server shuts down when closing the window
	return 0;
}

void WindowCreate()
{
	static Window win = {Config().WindowWidth, Config().WindowHeight};
	g_pWindow = &win;

	win.thread.Begin(ThreadWindow, nullptr);
}

void WindowRequestClose()
{
	sapp_request_quit();
}

void WindowWaitForCleanup()
{
	g_pWindow->thread.WaitForEnd();
}

namespace Dbg {

GameUID PushNewGame(const FixedStr32& mapName)
{
	// ignore name for now, assume PVP map
	return GameUID::INVALID;
}

void PushNewFrame(GameUID gameUID)
{
	g_pWindow->game.NewFrame();
}

void PushEntity(GameUID gameUID, const Entity& entity)
{
	g_pWindow->game.PushEntity(entity);
}

void PopGame(GameUID gameUID)
{
	// does nothing for now
}

}

#endif
