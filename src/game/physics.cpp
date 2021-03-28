#include "physics.h"

bool TestIntersection(const PhysSphere& A, const PhysSphere& B, PhysPenetrationVector* pen)
{
	vec3 delta = B.pos - A.pos;
	f32 deltaLen = glm::length(delta);
	if(deltaLen < (A.radius + B.radius)) {
		pen->impact = A.pos + glm::normalize(delta) * A.radius;
		pen->depth = -glm::normalize(delta) * (A.radius + B.radius - deltaLen);
		return true;
	}
	return glm::length(delta) < (A.radius + B.radius);
}

bool TestIntersection(const PhysSphere& A, const PhysTriangle& B, PhysPenetrationVector* pen)
{
	vec3 planeNorm = B.Normal();
	f32 signedDistToPlane = glm::dot(A.pos - B.p[0], planeNorm);

	if(signedDistToPlane < -A.radius || signedDistToPlane > A.radius) {
		return false;
	}

	return true;
}
