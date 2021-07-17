#include "physics.h"
#include <EASTL/sort.h>
#include <glm/gtx/vector_angle.hpp>

#define DBG_ASSERT_NONNAN(X) DBG_ASSERT(!isnan(X))

bool SegmentPlaneIntersection(const vec3& s0, const vec3& s1, const vec3& planeNorm, const vec3& planePoint, vec3* intersPoint)
{
	const vec3 segNorm = glm::normalize(s1 - s0);
	f32 dot = glm::dot(planeNorm, segNorm);
	if(abs(dot) < PHYS_EPSILON) {
		return false; // parallel
	}

	const f32 t = glm::dot(planeNorm, (planePoint - s0) / dot);
	if(t < 0 || t*t >= LengthSq(s1 - s0)) return false;

	*intersPoint = s0 + segNorm * t;
	return true;
}

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

bool TestIntersection(const ShapeCylinder& A, const ShapeTriangle& B, PhysResolutionCylinderTriangle* pen)
{
	/** PLAN
	 * Take the intersection between the infinite rect defined by the cylinder Z min and max
	 * -> get a triangle or polygon
	 * -> if polygon, make it 2 triangles
	 * -> project on 2D plane, do triangle circle intersection in 2D
	 **/

	const vec3 cylNorm = A.Normal();
	const vec3 planeNorm = B.Normal();
	const vec3 base = A.base;
	const vec3 tip = A.base + vec3(0, 0, A.height);

	eastl::fixed_vector<vec3,4> points;

	foreach_const(p, B.p) {
		if(p->z < tip.z && p->z >= A.base.z) {
			points.push_back(*p);
		}
	}

	// top plane
	vec3 ti0, ti1, ti2;
	bool rt0 = SegmentPlaneIntersection(B.p[0], B.p[1], vec3(0, 0, 1), tip, &ti0);
	bool rt1 = SegmentPlaneIntersection(B.p[0], B.p[2], vec3(0, 0, 1), tip, &ti1);
	bool rt2 = SegmentPlaneIntersection(B.p[1], B.p[2], vec3(0, 0, 1), tip, &ti2);

	if(rt0) {
		points.push_back(ti0);
	}
	if(rt1) {
		points.push_back(ti1);
	}
	if(rt2) {
		points.push_back(ti2);
	}

	// bottom plane
	vec3 bi0, bi1, bi2;
	bool rb0 = SegmentPlaneIntersection(B.p[0], B.p[1], vec3(0, 0, 1), A.base, &bi0);
	bool rb1 = SegmentPlaneIntersection(B.p[0], B.p[2], vec3(0, 0, 1), A.base, &bi1);
	bool rb2 = SegmentPlaneIntersection(B.p[1], B.p[2], vec3(0, 0, 1), A.base, &bi2);

	if(rb0) {
		points.push_back(bi0);
	}
	if(rb1) {
		points.push_back(bi1);
	}
	if(rb2) {
		points.push_back(bi2);
	}

	if(points.empty()) return false;

	// sort points
	{
		vec3 center = vec3(0);
		foreach_const(p, points) {
			center += *p;
		}
		center /= (f32)points.size();

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
		tris.push_back(t);
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

		if(bestDist < r*r) {
			intersects = true;
			break;
		}
	}

	if(intersects) {
		vec3 projCenter;
		LinePlaneIntersection(vec3(center, A.base.z), vec3(center, tip.z), planeNorm, B.p[0], &projCenter);
		projCenter.z = clamp(projCenter.z, triMinZ, triMaxZ);

		f32 baseZ = A.base.z;
		if(glm::dot(cylNorm, planeNorm) < 0) {
			baseZ = tip.z;
		}
		vec3 farthestPoint = vec3(center + -triDir * r, projCenter.z);
		vec3 projFarthestPoint = ProjectPointOnPlane(farthestPoint, planeNorm, B.p[0]);

		vec3 delta = farthestPoint - projFarthestPoint + vec3(0, 0, baseZ - projCenter.z);
		pen->slide = delta;

		// X push
		vec2 pushX = vec2(0);
		f32 pushXLenSq = 0;
		const vec2 fp = vec2(farthestPoint);
		const ShapeTriangle* farthestTri = &tris[0];

		foreach_const(t, tris) {
			vec2 t0 = vec2(t->p[0]);
			vec2 t1 = vec2(t->p[1]);
			vec2 t2 = vec2(t->p[2]);

			eastl::array<vec2, 3> pointList = {
				fp, fp, fp,
			};

			bool oneInters = false;
			oneInters |= LineSegmentIntersection(fp, fp + triDir * 100.f, t0, t1, &pointList[0]);
			oneInters |= LineSegmentIntersection(fp, fp + triDir * 100.f, t0, t2, &pointList[1]);
			oneInters |= LineSegmentIntersection(fp, fp + triDir * 100.f, t1, t2, &pointList[2]);

			if(!oneInters) {
				pointList = {
					LengthSq(center - t0) < r*r ? t0 : fp,
					LengthSq(center - t1) < r*r ? t1 : fp,
					LengthSq(center - t2) < r*r ? t2 : fp,
				};
			}

			foreach_const(p, pointList) {
				f32 d0 = glm::dot(*p - fp, triDir);
				if(d0 > pushXLenSq) {
					pushXLenSq = d0;
					pushX = d0 * triDir;
					farthestTri = &(*t);
				}
			}
		}

		vec2 displaced = vec2(A.base) + pushX;
		f32 dist;
		vec2 closest = ClosestPointToTriangle(displaced, *farthestTri, &dist);

		vec2 extra = NormalizeSafe(displaced - closest) * (A.radius - dist);
		if(Vec2Dot(extra, triDir) > 0) {
			pushX += extra;
		}

		pen->pushX = vec3(pushX, 0);

		const f32 cosTheta = glm::dot(glm::normalize(pen->slide), cylNorm);
		const f32 fix2Len = glm::length(pen->slide) / cosTheta;
		pen->pushZ = -cylNorm * (fix2Len);
		return true;
	}

	return false;
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
#define STEP_HEIGHT GetGlobalTweakableVars().stepHeight

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
						const vec3 triPen = FixCapsuleAlongTriangleNormal(s, sphereCenter, pen, triangleNormal);

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

					Collision* selected = &(*colList.begin());
					for(auto it = colList.rbegin(); it != colList.rend(); it++) {
						const Collision& col = *it;
						
						f32 deltaZ = col.fix.z;
						if(deltaZ > 0.f && deltaZ < STEP_HEIGHT) {
							selected = &(*it);
						}
					}

					const Collision& col = *selected;

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
