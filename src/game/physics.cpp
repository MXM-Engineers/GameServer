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
