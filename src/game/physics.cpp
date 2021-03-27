#include "physics.h"

bool TestIntersection(const PhysSphere& A, const PhysSphere& B, PhysPenetrationVector* pen)
{
	return glm::length(A.pos - B.pos) < (A.radius + B.radius);
}
