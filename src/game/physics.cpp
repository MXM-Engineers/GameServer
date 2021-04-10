#include "physics.h"

bool TestIntersection(const PhysSphere& A, const PhysSphere& B, PhysPenetrationVector* pen)
{
	vec3 delta = B.center - A.center;
	f32 deltaLen = glm::length(delta);
	if(deltaLen < (A.radius + B.radius)) {
		pen->impact = A.center + glm::normalize(delta) * A.radius;
		pen->depth = -glm::normalize(delta) * (A.radius + B.radius - deltaLen);
		return true;
	}
	return glm::length(delta) < (A.radius + B.radius);
}

inline f32 LengthSq(const vec3& v)
{
	return glm::dot(v, v);
}

inline vec3 ClosestPointOnLineSegment(const vec3& la, const vec3& lb, const vec3& point)
{
	vec3 delta = lb - la;
	f32 t = glm::dot(point - la, delta) / glm::dot(delta, delta);
	return la + clamp(t, 0.f, 1.f) * delta;
}

bool TestIntersection(const PhysSphere& A, const PhysTriangle& B, PhysPenetrationVector* pen)
{
	vec3 planeNorm = B.Normal();
	f32 signedDistToPlane = glm::dot(A.center - B.p[0], planeNorm);

	// does not intersect plane
	if(signedDistToPlane < -A.radius || signedDistToPlane > A.radius) {
		return false;
	}

	vec3 projSphereCenter = A.center - planeNorm * signedDistToPlane; // projected sphere center on triangle plane

	// Now determine whether projSphereCenter is inside all triangle edges
	vec3 c0 = cross(projSphereCenter - B.p[0], B.p[1] - B.p[0]);
	vec3 c1 = cross(projSphereCenter - B.p[1], B.p[2] - B.p[1]);
	vec3 c2 = cross(projSphereCenter - B.p[2], B.p[0] - B.p[2]);
	bool inside = glm::dot(c0, planeNorm) <= 0 && glm::dot(c1, planeNorm) <= 0 && glm::dot(c2, planeNorm) <= 0;
	if(inside) {
		vec3 delta = projSphereCenter - A.center;
		if(glm::length(delta) > 0) {
			pen->impact = A.center + glm::normalize(projSphereCenter - A.center) * A.radius;
			pen->depth = projSphereCenter - pen->impact;
		}
		else {
			pen->impact = A.center + planeNorm * A.radius;
			pen->depth = A.center - pen->impact;
		}
		return true;
	}

	const f32 radiusSq = A.radius * A.radius;
	bool intersects = false;

	// project center on all edges
	vec3 point1 = ClosestPointOnLineSegment(B.p[0], B.p[1], A.center);
	intersects |= LengthSq(A.center - point1) < radiusSq;
	vec3 point2 = ClosestPointOnLineSegment(B.p[1], B.p[2], A.center);
	intersects |= LengthSq(A.center - point2) < radiusSq;
	vec3 point3 = ClosestPointOnLineSegment(B.p[2], B.p[0], A.center);
	intersects |= LengthSq(A.center - point3) < radiusSq;

	if(intersects) {
		vec3 bestPoint = point1;
		f32 bestDist = LengthSq(A.center - point1);

		f32 dist2 = LengthSq(A.center - point2);
		if(dist2 < bestDist) {
			bestDist = dist2;
			bestPoint = point2;
		}

		f32 dist3 = LengthSq(A.center - point3);
		if(dist3 < bestDist) {
			bestPoint = point3;
		}

		vec3 delta = bestPoint - A.center;
		pen->impact = A.center + glm::normalize(delta) * A.radius;
		pen->depth = -(glm::normalize(delta) * A.radius - delta);
		return true;
	}

	return false;
}

bool TestIntersection(const PhysCapsule& A, const PhysCapsule& B, PhysPenetrationVector* pen)
{
	// capsule A
	vec3 a_Normal = glm::normalize(A.tip - A.base);
	vec3 a_LineEndOffset = a_Normal * A.radius;
	vec3 a_A = A.base + a_LineEndOffset;
	vec3 a_B = A.tip - a_LineEndOffset;

	// capsule B
	vec3 b_Normal = glm::normalize(B.tip - B.base);
	vec3 b_LineEndOffset = b_Normal * B.radius;
	vec3 b_A = B.base + b_LineEndOffset;
	vec3 b_B = B.tip - b_LineEndOffset;

	// vectors between line endpoints
	vec3 v0 = b_A - a_A;
	vec3 v1 = b_B - a_A;
	vec3 v2 = b_A - a_B;
	vec3 v3 = b_B - a_B;

	// squared distances
	f32 d0 = LengthSq(v0);
	f32 d1 = LengthSq(v1);
	f32 d2 = LengthSq(v2);
	f32 d3 = LengthSq(v3);

	// select best potential endpoint on capsule A:
	vec3 bestA;
	if(d2 < d0 || d2 < d1 || d3 < d0 || d3 < d1) {
		bestA = a_B;
	}
	else {
		bestA = a_A;
	}

	// select point on capsule B line segment nearest to best potential endpoint on A capsule:
	vec3 bestB = ClosestPointOnLineSegment(b_A, b_B, bestA);

	// now do the same for capsule A segment:
	bestA = ClosestPointOnLineSegment(a_A, a_B, bestB);

	const PhysSphere sphereA = {bestA, A.radius};
	const PhysSphere sphereB = {bestB, B.radius};
	return TestIntersection(sphereA, sphereB, pen);
}
