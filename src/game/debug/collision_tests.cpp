#include "collision_tests.h"
#include <imgui.h>
#include <glm/gtx/vector_angle.hpp>

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

	// test7 -- cylinder upright triangle
	{
		static bool bFixedCylinder = false;
		static bool bAutoMove = false;
		static bool bCenter = false;
		static bool bCapsule = false;
		static f32 fOffsetX = 166.520f;
		static f32 fOffsetY = 0;
		static f32 fTipOff = 0;
		static f32 fTriY = 0;
		static f32 fTriZ = 0;

		if(ImGui::Begin("Test 7")) {
			ImGui::Checkbox("Fixed cylinder", &bFixedCylinder);
			ImGui::Checkbox("Auto move", &bAutoMove);
			ImGui::Checkbox("Center", &bCenter);
			ImGui::Checkbox("Capsule", &bCapsule);
			ImGui::SliderFloat("OffsetX", &fOffsetX, 0, 300);
			ImGui::SliderFloat("OffsetY", &fOffsetY, -200, 200);
			ImGui::SliderFloat("TipOff", &fTipOff, -200, 200);
			ImGui::SliderFloat("TriY", &fTriY, -200, 200);
			ImGui::SliderFloat("TriZ", &fTriZ, -200, 200);
		}

		ShapeCylinder cylinderA;
		ShapeCapsule capsuleA;
		ShapeTriangle triangleB;

		const vec3 off = bAutoMove ? vec3(210 + saw(a * .5) * -210, 0, 0) : vec3(fOffsetX, 0, 0);
		cylinderA.base = vec3(100, 1300, 0) + off;
		cylinderA.tip = vec3(100, 1300, 100) + off;
		cylinderA.radius = 20;

		capsuleA.base = cylinderA.base;
		capsuleA.tip = cylinderA.tip;
		capsuleA.radius = cylinderA.radius;

		const vec3 triOff = bAutoMove ? vec3(0, 0, -100 + saw(a) * 200) : vec3(0, 0, fOffsetY);
		triangleB.p = {
			vec3(200, 1250 + fTriY, 0 + fTriZ) + triOff,
			vec3(200 + fTipOff, 1300 + fTriY, 80 + fTriZ) + triOff,
			vec3(200, 1350 + fTriY, 0 + fTriZ) + triOff
		};

		rdr.PushLine(cylinderA.base - vec3(0, 0, 1000.f), cylinderA.base + vec3(0, 0, 1000.f), vec3(1));


		do {
			const ShapeCylinder& A = cylinderA;
			const ShapeTriangle& B = triangleB;

			const vec3 cylNorm = glm::normalize(A.tip - A.base);
			const vec3 planeNorm = B.Normal();
			DBG_ASSERT(glm::dot(cylNorm, vec3(0, 0, 1)) == 1); // upright

			eastl::fixed_vector<vec3,4> points;

			foreach_const(p, B.p) {
				if(p->z < A.tip.z && p->z >= A.base.z) {
					points.push_back(*p);
				}
			}

			// top plane
			vec3 ti0, ti1, ti2;
			bool rt0 = SegmentPlaneIntersection(B.p[0], B.p[1], vec3(0, 0, 1), A.tip, &ti0);
			bool rt1 = SegmentPlaneIntersection(B.p[0], B.p[2], vec3(0, 0, 1), A.tip, &ti1);
			bool rt2 = SegmentPlaneIntersection(B.p[1], B.p[2], vec3(0, 0, 1), A.tip, &ti2);
			bool intersectsTopPlane = rt0 | rt1 | rt2;

			if(rt0) {
				//Draw(ti0, vec3(1, 0, 0));
				points.push_back(ti0);
			}
			if(rt1) {
				//Draw(ti1, vec3(1, 0, 0));
				points.push_back(ti1);
			}
			if(rt2) {
				//Draw(ti2, vec3(1, 0, 0));
				points.push_back(ti2);
			}

			// bottom plane
			vec3 bi0, bi1, bi2;
			bool rb0 = SegmentPlaneIntersection(B.p[0], B.p[1], vec3(0, 0, 1), A.base, &bi0);
			bool rb1 = SegmentPlaneIntersection(B.p[0], B.p[2], vec3(0, 0, 1), A.base, &bi1);
			bool rb2 = SegmentPlaneIntersection(B.p[1], B.p[2], vec3(0, 0, 1), A.base, &bi2);
			bool intersectsBotPlane = rb0 | rb1 | rb2;

			if(rb0) {
				//Draw(bi0, vec3(1, 0.5, 0.5));
				points.push_back(bi0);
			}
			if(rb1) {
				//Draw(bi1, vec3(1, 0.5, 0.5));
				points.push_back(bi1);
			}
			if(rb2) {
				//Draw(bi2, vec3(1, 0.5, 0.5));
				points.push_back(bi2);
			}

			if(points.empty()) break;

			// sort points
			{
				vec3 center = vec3(0);
				foreach_const(p, points) {
					center += *p;
				}
				center /= (f32)points.size();

				//Draw(center, vec3(1));

				bool sort = true;
				while(sort) {
					sort = false;

					for(int i = 2; i < points.size(); i++) {
						vec3 d0 = glm::normalize(points[i-2] - center);
						vec3 d1 = glm::normalize(points[i-1] - center);
						vec3 d2 = glm::normalize(points[i] - center);
						f32 a0 = glm::angle(d0, d1);
						f32 a1 = glm::angle(d0, d2);
						if(a0 > a1) {
							eastl::swap(points[i], points[i-1]);
							sort = true;
						}
					}
				}
			}

			ImGui::Text("intersectsTopPlane: %d", intersectsTopPlane);
			ImGui::Text("intersectsBotPlane: %d", intersectsBotPlane);
			ImGui::Text("Points: %d", points.size());

			/*
			for(int i = 0; i < points.size(); i++) {
				Draw(points[i], vec3(1, (f32)i/points.size(), 0));
			}
			*/

			// triangulate
			vec3 anchor = points[0];
			points.erase(points.begin());

			eastl::fixed_vector<ShapeTriangle,2> tris;

			for(int i = 0; i < points.size() - 1; i++) {
				ShapeTriangle t;
				t.p = {
					anchor,
					points[i],
					points[i+1],
				};

				// remove 0 surface triangles
				if(LengthSq(t.p[0] - t.p[1]) < 1.0f) break;
				if(LengthSq(t.p[0] - t.p[2]) < 1.0f) break;
				if(LengthSq(t.p[1] - t.p[2]) < 1.0f) break;

				tris.push_back(t);
			}

			ImGui::Text("Triangles: %d", tris.size());

			foreach_const(t, tris) {
				rdr.PushLine(t->p[0], t->p[1], ColorV3(0xfc0283));
				rdr.PushLine(t->p[0], t->p[2], ColorV3(0xfc0283));
				rdr.PushLine(t->p[1], t->p[2], ColorV3(0xfc0283));
			}

			foreach(t, tris) {
				t->p[0].z = 0;
				t->p[1].z = 0;
				t->p[2].z = 0;
			}

			foreach_const(t, tris) {
				#define DRAW_NORM(P0, P1) DrawVec(glm::cross(P1 - P0, vec3(0, 0, 1)), (P0 + P1) * 0.5f, vec3(1), 0.5f)

				rdr.PushLine(t->p[0], t->p[1], ColorV3(0xfc0283));
				rdr.PushLine(t->p[1], t->p[2], ColorV3(0xfc0283));
				rdr.PushLine(t->p[2], t->p[0], ColorV3(0xfc0283));

				//DRAW_NORM(t->p[0], t->p[1]);
				//DRAW_NORM(t->p[1], t->p[2]);
				//DRAW_NORM(t->p[2], t->p[0]);

				#undef DRAW_NORM
			}

			const vec2 triDir = glm::normalize(vec2(planeNorm));
			const f32 triMinZ = MIN(MIN(B.p[0].z, B.p[1].z), B.p[2].z);
			const f32 triMaxZ = MAX(MAX(B.p[0].z, B.p[1].z), B.p[2].z);
			const vec2 center = vec2(A.base);
			const f32 r = A.radius;
			bool intersects = false;

			foreach_const(t, tris) {
				// TODO: make 2D version of this
				ShapeTriangle tri2D = *t;
				tri2D.p[0].z = 0;
				tri2D.p[1].z = 0;
				tri2D.p[2].z = 0;

				if(IsPointInsideTriangle(vec3(center, 0), tri2D)) {
					intersects = true;
					break;
				}

				eastl::array<vec2,3> closestList;
				closestList[0] = ClosestPointOnLineSegment(vec2(t->p[0]), vec2(t->p[1]), center);
				closestList[1] = ClosestPointOnLineSegment(vec2(t->p[1]), vec2(t->p[2]), center);
				closestList[2] = ClosestPointOnLineSegment(vec2(t->p[2]), vec2(t->p[0]), center);

				vec2 closest = closestList[0];
				f32 bestDist = LengthSq(center - closest);

				f32 dist = LengthSq(closestList[1] - center);
				if(dist < bestDist) {
					closest = closestList[1];
					bestDist = dist;
				}

				dist = LengthSq(closestList[2] - center);
				if(dist < bestDist) {
					closest = closestList[2];
					bestDist = dist;
				}

				vec2 p0 = closest;
				if(bestDist < r*r) {
					vec3 inters;
					LinePlaneIntersection(vec3(p0, 0), vec3(p0, 1), planeNorm, B.p[0], &inters);

					f32 sign = 1;
					if(glm::dot(planeNorm, cylNorm) > 0) {
						sign = -1;
					}

					f32 planeDot = glm::dot(cylNorm, planeNorm);
					if(abs(planeDot) > 1.0f-PHYS_EPSILON) {
						// collinear
					}
					else {
						intersects = true;
						break;
					}
				}
			}

			if(intersects) {
				vec3 projCenter;
				LinePlaneIntersection(vec3(center, A.base.z), vec3(center, A.tip.z), planeNorm, B.p[0], &projCenter);
				projCenter.z = clamp(projCenter.z, triMinZ, triMaxZ);

				f32 baseZ = A.base.z;
				if(glm::dot(cylNorm, planeNorm) < 0) {
					baseZ = A.tip.z;
				}
				vec3 farthestPoint = vec3(center + -triDir * r, projCenter.z);
				vec3 projFarthestPoint = ProjectPointOnPlane(farthestPoint, planeNorm, B.p[0]);

				vec3 delta = farthestPoint - projFarthestPoint + vec3(0, 0, baseZ - projCenter.z);
				//pen->dir = glm::normalize(delta);
				//pen->depth = glm::length(delta);

				Draw(projCenter, vec3(1));
				Draw(farthestPoint, vec3(0, 0, 1));
				Draw(projFarthestPoint, ColorV3(0x8b57bf));

				// X push
				vec2 pushX = vec2(0);
				f32 pushXLenSq = 0;
				const vec2 fp = vec2(farthestPoint);



				foreach_const(t, tris) {
					vec2 t0 = vec2(t->p[0]);
					vec2 t1 = vec2(t->p[1]);
					vec2 t2 = vec2(t->p[2]);

					vec2 p0, p1, p2;
					bool r0 = LineSegmentIntersection(fp, fp + triDir * 100.f, t0, t1, &p0);
					bool r1 = LineSegmentIntersection(fp, fp + triDir * 100.f, t0, t2, &p1);
					bool r2 = LineSegmentIntersection(fp, fp + triDir * 100.f, t1, t2, &p2);

					if(r0) {
						f32 d0 = glm::dot(p0 - fp, triDir);
						if(d0 > pushXLenSq) {
							pushXLenSq = d0;
							pushX = d0 * triDir;
						}
					}
					if(r1) {
						f32 d1 = glm::dot(p1 - fp, triDir);
						if(d1 > pushXLenSq) {
							pushXLenSq = d1;
							pushX = d1 * triDir;
						}
					}
					if(r2) {
						f32 d2 = glm::dot(p2 - fp, triDir);
						if(d2 > pushXLenSq) {
							pushXLenSq = d2;
							pushX = d2 * triDir;
						}
					}

					Draw(vec3(p0, 0), ColorV3(0x00b724));
					Draw(vec3(p1, 0), ColorV3(0x00b724));
					Draw(vec3(p2, 0), ColorV3(0x00b724));
				}

				DrawVec(vec3(pushX, 0), vec3(fp, 0), ColorV3(0x62b6c1), 0.2f);
			}

		} while(0);

		vec2 p0;
		vec2 la0 = vec2(-300, 10);
		vec2 la1 = vec2(300, 10);
		vec2 lb0 = vec2(150, -300);
		vec2 lb1 = vec2(150, 0);
		bool r = LineLineIntersection(la0, la1, lb0, lb1, &p0);

		rdr.PushLine(vec3(la0, 0), vec3(la1, 0), vec3(1, 0.2, 0));
		rdr.PushLine(vec3(lb0, 0), vec3(lb1, 0), vec3(1, 0.2, 0));
		if(r) Draw(vec3(p0, 0), vec3(1));

		rdr.PushLine(cylinderA.base + vec3(0, 0, -1000), cylinderA.tip + vec3(0, 0, 1000), vec3(1));

		PhysResolutionCylinderTriangle pen;
		bool intersect = TestIntersectionUpright(cylinderA, triangleB, &pen);

		DrawVec(triangleB.Normal() * 20.f, triangleB.Center(), vec3(1));

		if(intersect) {
			Draw(cylinderA, vec3(1, 0.5, 0.8));
			Draw(triangleB, vec3(1, 0.2, 1));
			DrawVec(-pen.slide, cylinderA.base + glm::normalize(pen.slide) * cylinderA.radius, vec3(1, 1, 0));

			PhysPenetrationVector capsPen = { vec3(1), 0.f };
			vec3 capsSphereCenter;
			if(bCapsule) {
				bool capsInt = TestIntersection(capsuleA, triangleB, &capsPen, &capsSphereCenter);
				Draw(capsuleA, vec3(1, 0.5, 0.8));
			}

			if(bFixedCylinder) {
				ShapeCylinder fixed = cylinderA;
				fixed.base += -pen.slide;
				fixed.tip += -pen.slide;
				Draw(fixed, vec3(0.5, 1, 0.5));

				fixed = cylinderA;
				fixed.base += pen.pushX;
				fixed.tip += pen.pushX;
				Draw(fixed, ColorV3(0xfcd276));

				if(bCapsule) {
					ShapeSphere sphere;
					sphere.center = capsSphereCenter;
					sphere.radius = capsuleA.radius;
					Draw(sphere, vec3(1));

					vec3 triPen = FixCapsuleAlongTriangleNormal(capsuleA, capsSphereCenter, capsPen, triangleB.Normal());
					ShapeCapsule capsFixed = capsuleA;
					capsFixed.base += triPen;
					capsFixed.tip += triPen;
					Draw(capsFixed, vec3(0.5, 1, 0.5));
				}
			}
		}
		else {
			Draw(cylinderA, vec3(0, 0.5, 0.8));
			Draw(triangleB, vec3(0, 0.2, 1));
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
