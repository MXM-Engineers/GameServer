#include "collision_tests.h"
#include <imgui.h>

void CollisionTest::DoCollisionTests()
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

		ShapeSphere sphereA;
		ShapeSphere sphereB;

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
				ShapeSphere fixed = sphereA;
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


		ShapeSphere sphereA;
		ShapeTriangle triangleB;

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

				ShapeSphere fixed = sphereA;
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

		ShapeCapsule capsuleA;
		ShapeCapsule capsuleB;

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

		ShapeCapsule capsuleA;
		ShapeCapsule capsuleB;

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
				ShapeCapsule fixed = capsuleA;
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


		ShapeCapsule capsuleA;
		ShapeTriangle triangleB;

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
		const ShapeCapsule& A = capsuleA;
		const ShapeTriangle& B = triangleB;

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

		const ShapeSphere sphere = { center, A.radius };

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

				ShapeCapsule fixed = capsuleA;
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
		static bool bFixedCapsule = true;
		static bool bAutoMove = true;
		static f32 fOffsetX = 166.520f;
		static f32 fOffsetY = 0;

		if(ImGui::Begin("Test 5")) {
			ImGui::Checkbox("Fixed capsule", &bFixedCapsule);
			ImGui::Checkbox("Auto move", &bAutoMove);
			ImGui::SliderFloat("OffsetX", &fOffsetX, 0, 300);
			ImGui::SliderFloat("OffsetY", &fOffsetY, -200, 200);
		}
		ImGui::End();

		ShapeCapsule capsuleA;
		ShapeTriangle triangleB;

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

			if(bFixedCapsule) {
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


				ShapeCapsule fixed = capsuleA;
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

		ShapeSphere sphereA;
		PhysRect rectB;

		const vec3 off = bAutoMove ? vec3(210 + saw(a * .5) * -210, 0, 0) : vec3(fOffsetX, 0, 0);
		sphereA.center = vec3(100, 1200, 0) + off;
		sphereA.radius = 20;

		rectB.size = vec2(100, 100);
		rectB.pos = vec3(200, 1100 + fRectX, fRectY);
		rectB.normal = vec3(-1, 0, 0);

		do {
			const ShapeSphere& A = sphereA;
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


				ShapeSphere fixed = sphereA;
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

void CollisionTest::Render()
{
	Time lastLocalTime = localTime;
	localTime = TimeRelNow();
	const f64 delta = TimeDiffSec(TimeDiff(lastLocalTime, localTime));
	lastLocalTime = localTime;

	DoCollisionTests();
}
