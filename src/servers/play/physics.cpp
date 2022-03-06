#include "physics.h"
#include <mxm/game_content.h>
#include <EASTL/sort.h>
#include <glm/gtx/vector_angle.hpp>
#include <PxPhysicsAPI.h> // lazy but oh well
#include <pvd/PxPvd.h>

#define PVD_HOST "127.0.0.1"

#define DBG_ASSERT_NONNAN(X) DBG_ASSERT(!isnan(X))

inline PxTolerancesScale TolerancesScale()
{
	PxTolerancesScale s;
	s.length = 100.f;
	s.speed = 100.f;
	return s;
}

class PhysxReadBuffer: public PxInputStream
{
	const void* data;
	const u32 size;
	i32 cur = 0;

public:
	PhysxReadBuffer(void* data_, u32 size_):
		data(data_),
		size(size_)
	{

	}

	virtual uint32_t read(void* dest, uint32_t count) override
	{
		ASSERT(cur + count <= size);
		memmove(dest, (u8*)data + cur, count);
		cur += count;
		return count;
	}
};

bool PhysicsContext::Init()
{
	foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocatorCallback, errorCallback);
	if(!foundation) {
		LOG("[PhysX] ERROR: PxCreateFoundation failed");
		return false;
	}

	bool recordMemoryAllocations = true;

	pvd = PxCreatePvd(*foundation);
	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
	bool connected = pvd->connect(*transport, PxPvdInstrumentationFlag::eALL);
	if(!connected) {
		LOG("[PhysX] WARNING: failed to connect to PVD Client");
	}

	physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, TolerancesScale(), recordMemoryAllocations, pvd);
	if(!physics) {
		LOG("[PhysX] ERROR: PxCreatePhysics failed");
		return false;
	}

	// TODO: does this work to make the simulation be executed on the same thread??
	dispatcher = PxDefaultCpuDispatcherCreate(0);
	if(!dispatcher) {
		LOG("[PhysX] ERROR: PxDefaultCpuDispatcherCreate failed");
		return false;
	}

	// collision meshes
	matMapSurface = physics->createMaterial(1.0f, 1.0f, 0.0f);

	const GameXmlContent& gc = GetGameXmlContent();
	bool r = LoadCollisionMesh(&cylinderMesh, gc.fileCylinderCollision);
	if(!r) {
		LOG("[PhysX] ERROR: LoadCollisionMesh(cylinder) failed");
		return false;
	}

	LOG("PhysicsContext initialised");
	return true;
}

void PhysicsContext::Shutdown()
{
	physics->release();

	PxPvdTransport* transport = pvd->getTransport();
	pvd->release();
	transport->release();

	foundation->release();
	LOG("PhysicsContext shutdown");
}

bool PhysicsContext::LoadCollisionMesh(PxTriangleMesh** out, const FileBuffer& file)
{
	// TODO: actually properly read this file instead of skipping the header
	PhysxReadBuffer readBuff(file.data + 12 , file.size - 12);

	*out = physics->createTriangleMesh(readBuff);
	if(!*out) {
		LOG("[PhysX] ERROR: createTriangleMesh failed");
		return false;
	}

	return true;
}

bool PhysicsContext::LoadCollisionMesh(PxConvexMesh** out, const FileBuffer& file)
{
	// TODO: actually properly read this file instead of skipping the header
	PhysxReadBuffer readBuff(file.data + 12 , file.size - 12);

	*out = physics->createConvexMesh(readBuff);
	if(!*out) {
		LOG("[PhysX] ERROR: createConvexMesh failed");
		return false;
	}

	return true;
}

void PhysicsContext::CreateScene(PhysicsScene* out)
{
	PxSceneDesc desc{TolerancesScale()};
	desc.cpuDispatcher = dispatcher;
	desc.filterShader = PxDefaultSimulationFilterShader;
	desc.gravity = PxVec3(0.0f, 0.0f, -9.81f);

	PxScene* scene;
	{ LOCK_MUTEX(mutexSceneCreate);
		scene = physics->createScene(desc);
		if(!scene) {
			LOG("[PhysX] ERROR: Creating scene failed");
		}
		ASSERT(scene);

		out->scene = scene;
		PxPvdSceneClient* pvdClient = scene->getScenePvdClient();
		if(pvdClient) {
			pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
			pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
			pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
		}

		PxMaterial* material = physics->createMaterial(1.0f, 1.0f, 0.0f);

		// ground plane to aid with visualization (pvd)
		PxRigidStatic* groundPlane = PxCreatePlane(*physics, PxPlane(0,0,1,0), *material);
		scene->addActor(*groundPlane);

		out->controllerMngr = PxCreateControllerManager(*scene);
		if(!out->controllerMngr) {
			LOG("[PhysX] ERROR: Creating controller manager failed");
		}
		ASSERT(out->controllerMngr);
	}
}

void PhysicsScene::Step()
{
	foreach(c, colliderList) {
		PxControllerFilters filter;
		PxControllerCollisionFlags collisionFlags = c->actor->move(PxVec3(0.f, 0.f, -10.f), 0, (f32)UPDATE_RATE, filter, nullptr /* obstacles? */);
	}

	// FIXME: find out how to simulate on the same thread
	scene->simulate((f32)UPDATE_RATE);
	// here we do nothing but wait...
	scene->fetchResults(true);
}

void PhysicsScene::Destroy()
{
	scene->release();
}

void PhysicsScene::CreateStaticCollider(PxTriangleMesh* mesh)
{
	auto& ctx = PhysContext();

	PxTriangleMeshGeometry geometry = PxTriangleMeshGeometry(mesh);

	PxShape* shape = ctx.physics->createShape(geometry, *ctx.matMapSurface);
	ASSERT(shape); // createShape failed

	PxRigidStatic* ground = ctx.physics->createRigidStatic(PxTransform{PxIdentity});
	ground->attachShape(*shape);
	scene->addActor(*ground);
}

PhysicsEntityCollider PhysicsScene::CreateEntityCollider(f32 radius, f32 height)
{
	auto& ctx = PhysContext();

	PxBoxControllerDesc desc;
	desc.halfHeight = height/2.f;
	desc.halfForwardExtent = radius;
	desc.halfSideExtent = radius;
	desc.upDirection = PxVec3(0.0f, 0.0f, 1.0f);
	desc.material = ctx.matMapSurface;

	PxBoxController* box = (PxBoxController*)controllerMngr->createController(desc);
	ASSERT(box);

	PhysicsEntityCollider collider;
	collider.actor = box;
	collider.height = height;
	collider.radius = radius;

	colliderList.push_back(collider);
	return collider;
}

static PhysicsContext* g_Context;

bool PhysicsInit()
{
	static PhysicsContext context;
	g_Context = &context;
	return context.Init();
}

PhysicsContext& PhysContext()
{
	return *g_Context;
}



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

	if(points.size() < 3) return false;

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

	eastl::fixed_vector<ShapeTriangle,8,false> tris; // we should only need 2 triangles here, something is wrong

	for(int i = 0; i < points.size() - 1; i++) {
		ShapeTriangle t;
		t.p = {
			anchor,
			points[i],
			points[i+1],
		};

		if(LengthSq(t.p[0] - t.p[1]) < 1.f) continue;
		if(LengthSq(t.p[0] - t.p[2]) < 1.f) continue;
		if(LengthSq(t.p[1] - t.p[2]) < 1.f) continue;

		tris.push_back(t);
	}

	/*// parallel
	if(LengthSq(vec2(planeNorm)) < 0.001f) {
		return false; // FIXME: actually do this case
	}*/

	const vec2 triDir = NormalizeSafe(vec2(planeNorm));
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
		projCenter.z = clamp(projCenter.z, MAX(triMinZ, A.base.z), MIN(triMaxZ, tip.z));

		f32 baseZ = A.base.z;
		if(glm::dot(cylNorm, planeNorm) < 0) {
			baseZ = tip.z;
		}
		vec3 farthestPoint = vec3(center + -triDir * r, projCenter.z);
		vec3 projFarthestPoint = ProjectPointOnPlane(farthestPoint, planeNorm, B.p[0]);

		vec3 delta = farthestPoint - projFarthestPoint + vec3(0, 0, baseZ - projCenter.z);
		pen->slide = -delta;

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

		const f32 cosTheta = glm::dot(NormalizeSafe(pen->slide), cylNorm);
		if(abs(cosTheta) > PHYS_EPSILON) {
			const f32 fix2Len = glm::length(pen->slide) / cosTheta;
			pen->pushZ = cylNorm * (fix2Len);
		}
		else {
			pen->pushZ = pen->slide;
		}

		DBG_ASSERT_NONNAN(pen->slide.x); DBG_ASSERT_NONNAN(pen->slide.y); DBG_ASSERT_NONNAN(pen->slide.z);
		DBG_ASSERT_NONNAN(pen->pushX.x); DBG_ASSERT_NONNAN(pen->pushX.y); DBG_ASSERT_NONNAN(pen->pushX.z);
		DBG_ASSERT_NONNAN(pen->pushZ.x); DBG_ASSERT_NONNAN(pen->pushZ.y); DBG_ASSERT_NONNAN(pen->pushZ.z);
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
#include <mxm/core.h>
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
	Body body;

	body.flags = 0x0;
	body.radius = radius;
	body.height = height;
	body.pos = pos;
	body.vel = vec3(0);

	dynBodyList.push_back(body);
	return --dynBodyList.end();
}

void PhysWorld::DeleteBody(BodyHandle handle)
{
	dynBodyList.erase(handle);
}

void PhysWorld::Step()
{
	ProfileFunction();

	ASSERT(dynBodyList.size() == dynBodyList.size());

#if 1
	lastStepEvents.clear();
	lastStepPositions.clear();
	step++;
#endif

	// copy step data to continous buffers
	bodyList.clear();
	shapeCylinderList.clear();
	foreach_const(b, dynBodyList) {
		if(b->flags & Flags::Disabled) continue;

		ShapeCylinder shape;
		shape.radius = b->radius;
		shape.base = vec3(0);
		shape.height = b->height;

		shapeCylinderList.push_back(shape);
		bodyList.push_back({ b->pos, b->vel + b->force});

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
			movedShapeCylinderList.clear();
			eastl::copy(shapeCylinderList.begin(), shapeCylinderList.end(), eastl::back_inserter(movedShapeCylinderList));

			for(int i = 0; i < dynCount; i++) {
				ShapeCylinder& s = movedShapeCylinderList[i];
				const MoveComp& b = bodyList[i];
				s.base += b.pos;
			}

			collisionList.clear();
			collisionList.resize(dynCount);

			for(int i = 0; i < dynCount; i++) {
				const ShapeCylinder& s = movedShapeCylinderList[i];
				auto& colList = collisionList[i];

				foreach_const(tri, staticMeshTriangleList) {
					PhysResolutionCylinderTriangle pen;

					bool intersects = TestIntersection(s, *tri, &pen);
					if(intersects) {
						vec3 triangleNormal = tri->Normal();

						//vec3 fix = pen.slide;
						vec3 fix = pen.pushZ; // this seems to work better

						// almost parallel to a wall
						const f32 cosTheta = glm::dot(triangleNormal, s.Normal());
						if(abs(cosTheta) < 0.1) {
							fix = pen.pushX;
							triangleNormal = NormalizeSafe(fix);
						}

						/*// move body to the ground without sliding in the XY plane when adequate
						if(glm::dot(triangleNormal, vec3(0, 0, 1)) > 0.8) {
							fix = pen.pushZ;
							triangleNormal = NormalizeSafe(fix);
						}*/

						fix += NormalizeSafe(fix) * (PHYS_EPSILON * 10.0f);

						Collision col;
						col.pen = pen;
						col.triangleNormal = triangleNormal;
						col.fix = fix;
						col.fixLenSq = LengthSq(fix);
						colList.push_back(col);
						collided = true;

						#if 1
						CollisionEvent event;
						event.step = step;
						event.capsuleID = i;
						event.ssi = ssi;
						event.cri = cri;
						event.cylinder = s;
						event.triangle = *tri;
						event.fix = fix;
						event.fix2 = pen.slide;
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
				const ShapeCylinder& s = shapeCylinderList[i];
				auto& colList = collisionList[i];
				auto& body = bodyList[i];

				if(!colList.empty()) {
					eastl::sort(colList.begin(), colList.end(), [](const Collision& a, const Collision& b) {
						return a.fixLenSq < b.fixLenSq;
					});


					// step
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

					// we are on the ground
					if(abs(col.triangleNormal.z) > PHYS_EPSILON) {
						body.flags |= Grounded;
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
				rec.cylinder = s;
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
	foreach(b, dynBodyList) {
		if(b->flags & Flags::Disabled) continue;
		const MoveComp& bl = bodyList[i];

		b->pos = bl.pos;
		b->vel = vec3(0, 0, bl.vel.z); // 100% ground friction, 0% air friction
		b->vel.z = (i32)(b->vel.z * 10000.f) / 10000.f; // quantize
		b->flags = bl.flags;
		DBG_ASSERT_NONNAN(bl.pos.x);
		DBG_ASSERT_NONNAN(bl.pos.y);
		DBG_ASSERT_NONNAN(bl.pos.z);
		DBG_ASSERT_NONNAN(bl.vel.x);
		DBG_ASSERT_NONNAN(bl.vel.y);
		DBG_ASSERT_NONNAN(bl.vel.z);
		i++;
	}
}

vec3 PhysWorld::MoveUntilWall(const PhysWorld::BodyHandle handle, const vec3& dest)
{
	ShapeCylinder shape;
	shape.radius = handle->radius;
	shape.height = handle->height;

	vec3 pos = FixCollision(shape, handle->pos);
	const vec2 dir = NormalizeSafe(vec2(dest - pos));
	i32 i = i32(glm::length(dest - pos) / 10.0f) + 1;
	const i32 count = i;
	const vec2 dest2 = vec2(dest);

	while(i && LengthSq(vec2(pos) - dest2) > 1.f) {
		vec3 prevPos = pos;

		f32 length = glm::length(dest2 - vec2(pos));
		const vec3 seg = vec3(NormalizeSafe(dest2 - vec2(pos)) * MIN(MAX(10.0f, length/i), length), 0);

		pos = FixCollision(shape, pos + seg);
		pos = SnapToGround(shape, pos);

		if(LengthSq(dest2 - vec2(pos)) > LengthSq(dest2 - vec2(prevPos))) {  // we moved backward
			pos = prevPos;
			break;
		}

		if(LengthSq(vec2(prevPos) - vec2(pos)) < 1.f) break; // we have not moved

		i--;
	}

	return pos;
}

vec3 PhysWorld::FixCollision(const ShapeCylinder& shape, vec3 pos)
{
	ShapeCylinder s;
	s.radius = shape.radius;
	s.base = pos;
	s.height = shape.height;

	for(int cri = 0; cri < COLLISION_RESOLUTION_STEP_COUNT; cri++) {
		eastl::fixed_vector<Collision,16,false> colList;
		bool collided = false;

		foreach_const(tri, staticMeshTriangleList) {
			PhysResolutionCylinderTriangle pen;

			bool intersects = TestIntersection(s, *tri, &pen);
			if(intersects) {
				vec3 triangleNormal = tri->Normal();
				vec3 fix = pen.pushZ; // this seems to work better

				// almost parallel to a wall
				const f32 cosTheta = glm::dot(triangleNormal, s.Normal());
				if(abs(cosTheta) < 0.1) {
					fix = pen.pushX;
					triangleNormal = NormalizeSafe(fix);
				}

				fix += NormalizeSafe(fix) * (PHYS_EPSILON * 10.0f);

				Collision col;
				col.pen = pen;
				col.triangleNormal = triangleNormal;
				col.fix = fix;
				col.fixLenSq = LengthSq(fix);
				colList.push_back(col);
				collided = true;
			}
		}

		if(!colList.empty()) {
			eastl::sort(colList.begin(), colList.end(), [](const Collision& a, const Collision& b) {
				return a.fixLenSq < b.fixLenSq;
			});


			// step
			Collision* selected = &(*colList.begin());
			for(auto it = colList.rbegin(); it != colList.rend(); it++) {
				const Collision& col = *it;

				f32 deltaZ = col.fix.z;
				if(deltaZ > 0.f && deltaZ < STEP_HEIGHT) {
					selected = &(*it);
				}
			}

			const Collision& col = *selected;

			pos += col.fix;

			DBG_ASSERT_NONNAN(pos.x);
			DBG_ASSERT_NONNAN(pos.y);
			DBG_ASSERT_NONNAN(pos.z);
		}

		if(!collided) break;
	}

	return pos;
}

vec3 PhysWorld::SnapToGround(const ShapeCylinder& shape, vec3 pos)
{
	const vec2 center = vec2(pos);
	const f32 r = shape.radius;

	eastl::fixed_vector<ShapeTriangle,32> tris;

	// project all triangles to 2D plane and check intersection
	foreach_const(t, staticMeshTriangleList) {
		// TODO: make 2D version of this
		ShapeTriangle tri2D = *t;
		tri2D.p[0].z = 0;
		tri2D.p[1].z = 0;
		tri2D.p[2].z = 0;

		if(IsPointInsideTriangle(vec3(center, 0), tri2D)) {
			tris.push_back(*t); // intersects
			continue;
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
			tris.push_back(*t); // intersects
		}
	}

	// intersect in 3D with all selected triangles and find max Z (highest triangle)
	f32 maxZ = 0;
	foreach_const(t, tris) {
		const f32 triMinZ = MIN(MIN(t->p[0].z, t->p[1].z), t->p[2].z);

		ShapeCylinder s;
		s.radius = shape.radius;
		s.height = shape.height;
		s.base = pos;
		s.base.z = triMinZ;

		PhysResolutionCylinderTriangle pen;

		bool intersects = TestIntersection(s, *t, &pen);
		if(intersects) {
			vec3 fix = pen.pushZ;
			fix += NormalizeSafe(fix) * (PHYS_EPSILON * 10.0f);
			maxZ = MAX(maxZ, s.base.z + fix.z);
		}
	}

	pos.z = maxZ;
	return pos;
}

bool MakeMapCollisionMesh(const MeshFile::Mesh& mesh, ShapeMesh* out)
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

		ShapeTriangle tri;
		tri.p = { v0, v2, v1 };
		out->triangleList.push_back(tri);
	}

	return true;
}
