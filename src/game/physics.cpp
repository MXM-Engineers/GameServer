#include "physics.h"
#include <EASTL/sort.h>

#define DBG_ASSERT_NONNAN(X) DBG_ASSERT(!isnan(X))

bool TestIntersection(const ShapeSphere& A, const ShapeSphere& B, PhysPenetrationVector* pen)
{
	f32 len = glm::length(B.center - A.center);
	if(len < (A.radius + B.radius)) {
		pen->depth = (A.radius + B.radius) - len;
		pen->dir = glm::normalize(B.center - A.center);
		return true;
	}
	return false;
}

bool TestIntersection(const ShapeSphere& A, const ShapeTriangle& B, PhysPenetrationVector* pen)
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
		if(glm::length(delta) > PHYS_EPSILON) {
			pen->depth = A.radius - glm::length(delta);
			pen->dir = glm::normalize(delta);
		}
		else {
			pen->dir = planeNorm;
			pen->depth = A.radius;
		}
		return true;
	}

	const f32 radiusSq = A.radius * A.radius;
	bool intersects = false;

	// project center on all edges
	const f32 PHYS_EPSILON_BIG = PHYS_EPSILON * 1000;
	const vec3 point1 = ClosestPointOnLineSegment(B.p[0], B.p[1], A.center);
	intersects |= LengthSq(A.center - point1) + PHYS_EPSILON_BIG < radiusSq;
	const vec3 point2 = ClosestPointOnLineSegment(B.p[1], B.p[2], A.center);
	intersects |= LengthSq(A.center - point2) + PHYS_EPSILON_BIG < radiusSq;
	const vec3 point3 = ClosestPointOnLineSegment(B.p[2], B.p[0], A.center);
	intersects |= LengthSq(A.center - point3) + PHYS_EPSILON_BIG < radiusSq;

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
		pen->depth = A.radius - glm::length(delta);
		pen->dir = glm::normalize(delta);
		return true;
	}

	return false;
}

// This is inaccurate
// We need to find the closest distance (2 points) between the 2 capsule normal segments
#if 0
bool TestIntersection(const PhysCapsule& A, const PhysCapsule& B, PhysPenetrationVector* pen)
{
	// capsule A
	const vec3 a_Normal = glm::normalize(A.tip - A.base);
	const vec3 a_LineEndOffset = a_Normal * A.radius;
	const vec3 a_A = A.base + a_LineEndOffset;
	const vec3 a_B = A.tip - a_LineEndOffset;

	// capsule B
	const vec3 b_Normal = glm::normalize(B.tip - B.base);
	const vec3 b_LineEndOffset = b_Normal * B.radius;
	const vec3 b_A = B.base + b_LineEndOffset;
	const vec3 b_B = B.tip - b_LineEndOffset;

	// select best potential endpoint on capsule A:
	vec3 bestPotentialA;
	{
		// vectors between line endpoints
		const vec3 v0 = b_A - a_A;
		const vec3 v1 = b_B - a_A;
		const vec3 v2 = b_A - a_B;
		const vec3 v3 = b_B - a_B;

		// squared distances
		const f32 d0 = LengthSq(v0);
		const f32 d1 = LengthSq(v1);
		const f32 d2 = LengthSq(v2);
		const f32 d3 = LengthSq(v3);

		if(d2 < d0 || d2 < d1 || d3 < d0 || d3 < d1) {
			bestPotentialA = a_B;
		}
		else {
			bestPotentialA = a_A;
		}
	}

	// select point on capsule B line segment nearest to best potential endpoint on A capsule:
	vec3 bestB = ClosestPointOnLineSegment(b_A, b_B, bestPotentialA);

	// now do the same for capsule A segment:
	vec3 bestA = ClosestPointOnLineSegment(a_A, a_B, bestB);

	vec3 delta = bestA - bestB;
	f32 len = glm::length(delta);
	pen->depth = A.radius + B.radius - len;
	pen->dir = -glm::normalize(delta);
	return pen->depth > 0;
}
#endif

bool TestIntersection(const ShapeCapsule& A, const ShapeCapsule& B)
{
	// capsule A
	const vec3 a_Normal = glm::normalize(A.tip - A.base);
	const vec3 a_LineEndOffset = a_Normal * A.radius;
	const vec3 a_A = A.base + a_LineEndOffset;
	const vec3 a_B = A.tip - a_LineEndOffset;

	// capsule B
	const vec3 b_Normal = glm::normalize(B.tip - B.base);
	const vec3 b_LineEndOffset = b_Normal * B.radius;
	const vec3 b_A = B.base + b_LineEndOffset;
	const vec3 b_B = B.tip - b_LineEndOffset;

	// select best potential endpoint on capsule A:
	vec3 bestPotentialA;
	{
		// vectors between line endpoints
		const vec3 v0 = b_A - a_A;
		const vec3 v1 = b_B - a_A;
		const vec3 v2 = b_A - a_B;
		const vec3 v3 = b_B - a_B;

		// squared distances
		const f32 d0 = LengthSq(v0);
		const f32 d1 = LengthSq(v1);
		const f32 d2 = LengthSq(v2);
		const f32 d3 = LengthSq(v3);

		if(d2 < d0 || d2 < d1 || d3 < d0 || d3 < d1) {
			bestPotentialA = a_B;
		}
		else {
			bestPotentialA = a_A;
		}
	}

	// select point on capsule B line segment nearest to best potential endpoint on A capsule:
	vec3 bestB = ClosestPointOnLineSegment(b_A, b_B, bestPotentialA);

	// now do the same for capsule A segment:
	vec3 bestA = ClosestPointOnLineSegment(a_A, a_B, bestB);

	vec3 delta = bestA - bestB;
	f32 len = glm::length(delta);
	return (A.radius + B.radius - len) > 0;
}

// @Speed: optimizable
bool TestIntersectionUpright(const ShapeCapsule& A, const ShapeCapsule& B, PhysPenetrationVector* pen)
{
	// capsule A
	const vec3 lineA = A.tip - A.base;
	const vec3 normalA = glm::normalize(lineA);
	const f32 lenA = glm::length(lineA);
	DBG_ASSERT(glm::dot(normalA, vec3(0, 0, 1)) == 1); // upright

	// capsule B
	DBG_ASSERT(glm::dot(glm::normalize(B.tip - B.base), vec3(0, 0, 1)) == 1); // upright

	bool intersect = true;
	const f32 d1 = glm::dot(B.base - A.base, normalA) / lenA;
	const f32 d2 = glm::dot(B.tip - A.base, normalA) / lenA;
	if(d1 > 1) intersect = false;
	if(d2 < 0) intersect = false;

	if(intersect) {
		vec3 projBtoA = ClosestPointOnLineSegment(A.base, A.tip, B.base);
		vec3 delta = B.base - projBtoA;
		delta.z = 0; // 2D length since both capsules are upright
		f32 len = glm::length(delta);
		if(len < (A.radius + B.radius)) {
			if(intersect) {
				pen->depth = (A.radius + B.radius) - len;
				pen->dir = glm::normalize(delta);
				return true;
			}
		}
	}
	return false;
}

bool TestIntersection(const ShapeCapsule& A, const ShapeTriangle& B, PhysPenetrationVector* pen, vec3* sphereCenter)
{
	const vec3 capsuleNorm = glm::normalize(A.tip - A.base);
	const vec3 lineEndOffset = capsuleNorm * A.radius;
	const vec3 pointA = A.base + lineEndOffset;
	const vec3 pointB = A.tip - lineEndOffset;

	// ray-plane intersection
	const vec3 planeNorm = B.Normal();
	f32 planeCapsuleDot = glm::dot(planeNorm, capsuleNorm);
	if(abs(planeCapsuleDot) < PHYS_EPSILON) {
		planeCapsuleDot = 0.5f;
	}
	const f32 t = glm::dot(planeNorm, (B.p[0] - A.base) / planeCapsuleDot);
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
	*sphereCenter = center;

	return TestIntersection(ShapeSphere{ center, A.radius }, B, pen);
}

bool TestIntersection(const ShapeSphere& A, const PhysRect& B, PhysPenetrationVector* pen)
{
	const vec3 planeNorm = B.normal;
	const f32 signedDistToPlane = glm::dot(A.center - B.pos, planeNorm);

	// does not intersect plane
	if(signedDistToPlane < -A.radius || signedDistToPlane > A.radius) {
		return false;
	}

	// projected sphere center on plane
	const vec3 projSphereCenter = A.center - planeNorm * signedDistToPlane;

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
	const f32 circleR = sqrtf(A.radius*A.radius - dist*dist);

	// 2D circle rectangle intersection
	if(dotx >= (halfW + circleR) || doty >= (halfH + circleR)) return false;

	const f32 cornerDistSq = (dotx - halfW)*(dotx - halfW) + (doty - halfH)*(doty - halfH);

	bool inside = dotx < halfW || doty < halfH || cornerDistSq < (circleR * circleR);
	if(inside) {
		vec3 delta = projSphereCenter - A.center;
		if(glm::length(delta) > PHYS_EPSILON) {
			pen->depth = A.radius - glm::length(delta);
			pen->dir = glm::normalize(delta);
		}
		else {
			pen->dir = planeNorm;
			pen->depth = A.radius;
		}
		return true;
	}

	return false;
}

//const f32 GRAVITY = 1800;
// FIXME: remove
#include <game/core.h>
#define GRAVITY GetGlobalTweakableVars().gravity

const int SUB_STEP_COUNT = 1;
const int COLLISION_RESOLUTION_STEP_COUNT = 4;

void PhysWorld::PushStaticMeshes(const ShapeMesh* meshList, const int count)
{
	for(int i = 0; i < count; i++) {
		foreach_const(tri, meshList[i].triangleList) {
			staticMeshTriangleList.push_back(*tri);
		}
	}
}

PhysWorld::BodyHandle PhysWorld::CreateBody(f32 radius, f32 height, vec3 pos)
{
	BodyCapsule body;

	body.flags = 0x0;
	body.radius = radius;
	body.height = height;
	body.pos = pos;
	body.vel = vec3(0);

	dynCapsuleBodyList.push_back(body);
	return --dynCapsuleBodyList.end();
}

void PhysWorld::DeleteBody(BodyHandle handle)
{
	dynCapsuleBodyList.erase(handle);
}

inline vec3 FixAlongTriangleNormal(const ShapeCapsule& s, const vec3& sphereCenter, const PhysPenetrationVector& pen, const vec3& norm)
{
	const vec3 point = sphereCenter + pen.dir * (s.radius - (pen.depth + SignedEpsilon(pen.depth) * 5.0f));
	const vec3 projPoint = ProjectVecNorm(point - sphereCenter, norm);
	vec3 triPen = norm * s.radius + projPoint;

	f32 d = glm::dot(norm, s.Normal());
	if(d > PHYS_EPSILON) {
		triPen -= s.InnerBase() - sphereCenter;
	}
	else if(d < -PHYS_EPSILON) {
		triPen -= s.InnerTip() - sphereCenter;
	}

	return triPen;
}

void PhysWorld::Step()
{
	ProfileFunction();

	ASSERT(dynCapsuleBodyList.size() == dynCapsuleBodyList.size());

#if 1
	lastStepEvents.clear();
	lastStepPositions.clear();
	step++;
#endif

	// copy step data to continous buffers
	bodyList.clear();
	shapeCapsuleList.clear();
	foreach_const(b, dynCapsuleBodyList) {
		if(b->flags & Flags::Disabled) continue;

		ShapeCapsule shape;
		shape.radius = b->radius;
		shape.base = vec3(0);
		shape.tip = vec3(0, 0, b->height);

		shapeCapsuleList.push_back(shape);
		bodyList.push_back({ b->pos, b->force, b->vel + b->force});

		DBG_ASSERT_NONNAN(bodyList.back().pos.x);
		DBG_ASSERT_NONNAN(bodyList.back().pos.y);
		DBG_ASSERT_NONNAN(bodyList.back().pos.z);
		DBG_ASSERT_NONNAN(bodyList.back().vel.x);
		DBG_ASSERT_NONNAN(bodyList.back().vel.y);
		DBG_ASSERT_NONNAN(bodyList.back().vel.z);
	}

	// ASSUME NOTHING IS COLLIDING
	// TODO: make it so (STATIC -> DYNAMIC -> STATIC -> etc algo)

	const int dynCount = bodyList.size();

	for(int ssi = 0; ssi < SUB_STEP_COUNT; ssi++) {
		foreach(b, bodyList) {
			b->vel += dvec3(0, 0, -GRAVITY) * (UPDATE_RATE / SUB_STEP_COUNT);
			b->pos += b->vel * (UPDATE_RATE / SUB_STEP_COUNT);
		}

		for(int cri = 0; cri < COLLISION_RESOLUTION_STEP_COUNT; cri++) {
			bool collided = false;
			movedShapeCapsuleList.clear();
			eastl::copy(shapeCapsuleList.begin(), shapeCapsuleList.end(), eastl::back_inserter(movedShapeCapsuleList));

			for(int i = 0; i < dynCount; i++) {
				ShapeCapsule& s = movedShapeCapsuleList[i];
				const MoveComp& b = bodyList[i];
				s.base += b.pos;
				s.tip += b.pos;
			}

			collisionList.clear();
			collisionList.resize(dynCount);

			for(int i = 0; i < dynCount; i++) {
				const ShapeCapsule& s = movedShapeCapsuleList[i];
				auto& colList = collisionList[i];

				foreach_const(tri, staticMeshTriangleList) {
					PhysPenetrationVector pen;
					vec3 sphereCenter;

					bool intersects = TestIntersection(s, *tri, &pen, &sphereCenter);
					if(intersects) {
						const vec3 triangleNormal = tri->Normal();
						const vec3 triPen = FixAlongTriangleNormal(s, sphereCenter, pen, triangleNormal);

						vec3 fix = triPen;

						// move body to the ground without sliding in the XY plane when adequate (triangle is not nearly vertical)
						const vec3 gravityN = glm::normalize(vec3(0, 0, -GRAVITY));
						const f32 cosTheta = glm::dot(glm::normalize(triPen), -gravityN);
						if(cosTheta > 0.3) {
							const f32 fix2Len = glm::length(triPen) / cosTheta;
							fix = -gravityN * (fix2Len + PHYS_EPSILON);
							// TODO: grounded, remove all Z component
						}

						Collision col;
						col.pen = pen;
						col.triangleNormal = triangleNormal;
						col.fix = fix;
						col.fixLenSq = LengthSq(triPen);
						colList.push_back(col);
						collided = true;

						#if 1
						CollisionEvent event;
						event.step = step;
						event.capsuleID = i;
						event.ssi = ssi;
						event.cri = cri;
						event.capsule = s;
						event.triangle = *tri;
						event.fix = triPen;
						event.fix2 = fix;
						event.vel = bodyList[i].vel;
						event.fixedVel = bodyList[i].vel;
						if(glm::dot(glm::normalize(bodyList[i].vel), col.triangleNormal) < 0) {
							event.fixedVel -= ProjectVecNorm(event.fixedVel, col.triangleNormal);
						}
						event.pen = pen;
						lastStepEvents.push_back(event);
						#endif
					}
				}
			}


			for(int i = 0; i < dynCount; i++) {
				const ShapeCapsule& s = shapeCapsuleList[i];
				auto& colList = collisionList[i];
				auto& body = bodyList[i];

				if(!colList.empty()) {
					eastl::sort(colList.begin(), colList.end(), [](const Collision& a, const Collision& b) {
						return a.fixLenSq < b.fixLenSq;
					});

					const Collision& col = colList.front();

					body.pos += col.fix;
					const f32 len = glm::length(body.vel);
					if(glm::dot(glm::normalize(body.vel), col.triangleNormal) < 0) {
						body.vel -= ProjectVecNorm(body.vel, col.triangleNormal);
					}

					DBG_ASSERT_NONNAN(body.pos.x);
					DBG_ASSERT_NONNAN(body.pos.y);
					DBG_ASSERT_NONNAN(body.pos.z);
					DBG_ASSERT_NONNAN(body.vel.x);
					DBG_ASSERT_NONNAN(body.vel.y);
					DBG_ASSERT_NONNAN(body.vel.z);
				}

				#if 1
				PositionRec rec;
				rec.step = step;
				rec.capsuleID = i;
				rec.ssi = ssi;
				rec.cri = cri;
				rec.capsule = s;
				rec.pos = body.pos;
				rec.vel = body.vel;
				lastStepPositions.push_back(rec);
				#endif
			}

			if(!collided) break;
		}
	}

	// copy back
	int i = 0;
	foreach(b, dynCapsuleBodyList) {
		if(b->flags & Flags::Disabled) continue;
		const MoveComp& bl = bodyList[i];

		b->pos = bl.pos;
		b->vel = vec3(0, 0, bl.vel.z); // 100% ground friction, 0% air friction
		DBG_ASSERT_NONNAN(bl.pos.x);
		DBG_ASSERT_NONNAN(bl.pos.y);
		DBG_ASSERT_NONNAN(bl.pos.z);
		DBG_ASSERT_NONNAN(bl.vel.x);
		DBG_ASSERT_NONNAN(bl.vel.y);
		DBG_ASSERT_NONNAN(bl.vel.z);
		i++;
	}
}
