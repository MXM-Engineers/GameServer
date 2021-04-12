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
		if(glm::length(delta) > 0.001f) {
			pen->impact = A.center + glm::normalize(projSphereCenter - A.center) * A.radius;
			pen->depth = projSphereCenter - pen->impact;
		}
		else {
			pen->impact = A.center - planeNorm * A.radius;
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

bool TestIntersection(const PhysCapsule& A, const PhysTriangle& B, PhysPenetrationVector* pen)
{
	const vec3 capsuleNorm = normalize(A.tip - A.base);
	const vec3 lineEndOffset = capsuleNorm * A.radius;
	const vec3 pointA = A.base + lineEndOffset;
	const vec3 pointB = A.tip - lineEndOffset;

	// ray-plane intersection
	const vec3 planeNorm = B.Normal();
	const f32 t = glm::dot(planeNorm, (B.p[0] - A.base) / abs(glm::dot(planeNorm, capsuleNorm)));
	const vec3 linePlaneIntersection = A.base + capsuleNorm * t;

	vec3 refPoint;

	// Determine whether linePlaneIntersection is inside all triangle edges
	const vec3 c0 = glm::cross(linePlaneIntersection - B.p[0], B.p[1] - B.p[0]);
	const vec3 c1 = glm::cross(linePlaneIntersection - B.p[1], B.p[2] - B.p[1]);
	const vec3 c2 = glm::cross(linePlaneIntersection - B.p[2], B.p[0] - B.p[2]);
	bool inside = glm::dot(c0, planeNorm) <= 0 && glm::dot(c1, planeNorm) <= 0 && glm::dot(c2, planeNorm) <= 0;

	if(inside) {
		refPoint = linePlaneIntersection;
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
	}

	// The center of the best sphere candidate
	const vec3 center = ClosestPointOnLineSegment(pointA, pointB, refPoint);

	// inline sphere collision test
	// -------------------------------------------------------------------------------------------------------------
	f32 signedDistToPlane = glm::dot(center - B.p[0], planeNorm);

	// does not intersect plane
	if(signedDistToPlane < -A.radius || signedDistToPlane > A.radius) {
		return false;
	}

	vec3 projSphereCenter = center - planeNorm * signedDistToPlane; // projected sphere center on triangle plane

	// Now determine whether projSphereCenter is inside all triangle edges
	const vec3 d0 = cross(projSphereCenter - B.p[0], B.p[1] - B.p[0]);
	const vec3 d1 = cross(projSphereCenter - B.p[1], B.p[2] - B.p[1]);
	const vec3 d2 = cross(projSphereCenter - B.p[2], B.p[0] - B.p[2]);
	inside = glm::dot(d0, planeNorm) <= 0 && glm::dot(d1, planeNorm) <= 0 && glm::dot(d2, planeNorm) <= 0;
	if(inside) {
		vec3 delta = projSphereCenter - center;
		vec3 impact;
		vec3 depth;
		if(glm::length(delta) > 0.001f) {
			impact = center + glm::normalize(projSphereCenter - center) * A.radius;
			depth = projSphereCenter - impact;
		}
		else {
			impact  = center - planeNorm * A.radius;
			depth = center - impact;
		}

		vec3 depth1 = center + depth - pointA;
		vec3 depth2 = center - glm::normalize(depth) * (A.radius * 2 - glm::length(depth)) - pointB;

		if(LengthSq(depth1) < LengthSq(depth2)) {
			pen->depth = depth1;
			pen->impact = impact - (center - pointA);
		}
		else {
			pen->depth = depth2;
			pen->impact = center - depth2;
		}
		return true;
	}

	const f32 radiusSq = A.radius * A.radius;
	bool intersects = false;

	// project center on all edges
	const vec3 point1 = ClosestPointOnLineSegment(B.p[0], B.p[1], center);
	intersects |= LengthSq(center - point1) < radiusSq;
	const vec3 point2 = ClosestPointOnLineSegment(B.p[1], B.p[2], center);
	intersects |= LengthSq(center - point2) < radiusSq;
	const vec3 point3 = ClosestPointOnLineSegment(B.p[2], B.p[0], center);
	intersects |= LengthSq(center - point3) < radiusSq;

	if(intersects) {
		vec3 bestPoint = point1;
		f32 bestDist = LengthSq(center - point1);

		f32 dist2 = LengthSq(center - point2);
		if(dist2 < bestDist) {
			bestDist = dist2;
			bestPoint = point2;
		}

		f32 dist3 = LengthSq(center - point3);
		if(dist3 < bestDist) {
			bestPoint = point3;
		}

		vec3 delta = bestPoint - center;
		vec3 impact = center + glm::normalize(delta) * A.radius;
		vec3 depth = -(glm::normalize(delta) * A.radius - delta);

		pen->depth = depth;
		pen->impact = impact;
		return true;
	}

	return false;
}
