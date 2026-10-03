#include "scene_snapshot.h"
#include "velqor.h"

#include <NxPhysics.h>
#include <NxController.h>
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <float.h>

namespace {

NxScene* g_captured_scene = 0;

void RequireSnapshot(bool ok, const char* operation)
{
	if (ok)
		return;
	char fields[192];
	snprintf(fields, sizeof(fields), "\"operation\":\"%s\"", operation);
	velqor::Emit("scene_snapshot_error", fields);
	TerminateProcess(GetCurrentProcess(), 2);
}

void EncodeName(char* out, size_t capacity, const char* name)
{
	if (!name)
	{
		RequireSnapshot(capacity >= 5, "name_capacity");
		memcpy(out, "null", 5);
		return;
	}
	size_t n = 0;
	out[n++] = '"';
	for (const unsigned char* c = (const unsigned char*)name; *c; ++c)
	{
		RequireSnapshot(n + 8 < capacity, "name_capacity");
		if (*c < 32 || *c >= 127)
			n += snprintf(out + n, capacity - n, "\\u%04x", (unsigned int)*c);
		else
		{
			if (*c == '"' || *c == '\\')
				out[n++] = '\\';
			out[n++] = (char)*c;
		}
	}
	out[n++] = '"';
	out[n] = 0;
}

void EmitShapeDescription(uint32_t actor_id, uint32_t shape_id, const NxShapeDesc& description)
{
	char name[1024];
	EncodeName(name, sizeof(name), description.name);
	char fields[1600];
	snprintf(fields, sizeof(fields),
		"\"actor_id\":%u,\"shape_id\":%u,\"name\":%s,\"shape_flags\":%u,\"skin_width\":%.9g,\"density\":%.9g,\"mass\":%.9g,\"non_interacting\":%u",
		actor_id, shape_id, name, description.shapeFlags, (double)description.skinWidth,
		(double)description.density, (double)description.mass, description.nonInteractingCompartmentTypes);
	velqor::Emit("scene_shape_desc", fields);
}

void EmitPose(uint32_t actor_id, uint32_t shape_id, const char* kind, const NxMat34& pose)
{
	char fields[768];
	snprintf(fields, sizeof(fields),
		"\"actor_id\":%u,\"shape_id\":%u,\"kind\":\"%s\",\"translation\":[%.9g,%.9g,%.9g],\"rotation\":[%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g]",
		actor_id, shape_id, kind,
		(double)pose.t.x, (double)pose.t.y, (double)pose.t.z,
		(double)pose.M(0, 0), (double)pose.M(0, 1), (double)pose.M(0, 2),
		(double)pose.M(1, 0), (double)pose.M(1, 1), (double)pose.M(1, 2),
		(double)pose.M(2, 0), (double)pose.M(2, 1), (double)pose.M(2, 2));
	velqor::Emit("scene_pose", fields);
}

template<class MeshDesc>
void EmitMesh(uint32_t mesh_id, const char* kind, const MeshDesc& mesh)
{
	RequireSnapshot(mesh.numVertices != 0 && mesh.points != 0 && mesh.pointStrideBytes >= 12, "mesh_points");
	RequireSnapshot(mesh.numTriangles == 0 || mesh.triangles != 0, "mesh_triangles");
	char fields[256];
	snprintf(fields, sizeof(fields), "\"mesh_id\":%u,\"kind\":\"%s\",\"vertices\":%u,\"triangles\":%u,\"flags\":%u",
		mesh_id, kind, mesh.numVertices, mesh.numTriangles, mesh.flags);
	velqor::Emit("scene_mesh", fields);
	for (NxU32 start = 0; start < mesh.numVertices; start += 32)
	{
		char points[2560];
		int used = snprintf(points, sizeof(points), "\"mesh_id\":%u,\"start\":%u,\"points\":[", mesh_id, start);
		const NxU32 end = start + 32 < mesh.numVertices ? start + 32 : mesh.numVertices;
		for (NxU32 i = start; i < end; ++i)
		{
			float point[3];
			memcpy(point, (const uint8_t*)mesh.points + i * mesh.pointStrideBytes, sizeof(point));
			const int added = snprintf(points + used, sizeof(points) - used, "%s[%.9g,%.9g,%.9g]",
				i == start ? "" : ",", (double)point[0], (double)point[1], (double)point[2]);
			RequireSnapshot(added > 0 && (size_t)added < sizeof(points) - used, "points_buffer");
			used += added;
		}
		RequireSnapshot((size_t)used + 2 < sizeof(points), "points_close");
		points[used++] = ']';
		points[used] = 0;
		velqor::Emit("scene_mesh_points", points);
	}
	for (NxU32 start = 0; start < mesh.numTriangles; start += 64)
	{
		char triangles[2560];
		int used = snprintf(triangles, sizeof(triangles), "\"mesh_id\":%u,\"start\":%u,\"indices\":[", mesh_id, start);
		const NxU32 end = start + 64 < mesh.numTriangles ? start + 64 : mesh.numTriangles;
		for (NxU32 i = start; i < end; ++i)
		{
			NxU32 indices[3];
			const uint8_t* source = (const uint8_t*)mesh.triangles + i * mesh.triangleStrideBytes;
			if (mesh.flags & NX_MF_16_BIT_INDICES)
			{
				NxU16 small[3];
				memcpy(small, source, sizeof(small));
				for (int j = 0; j < 3; ++j)
					indices[j] = small[j];
			}
			else
				memcpy(indices, source, sizeof(indices));
			const int added = snprintf(triangles + used, sizeof(triangles) - used, "%s[%u,%u,%u]",
				i == start ? "" : ",", indices[0], indices[1], indices[2]);
			RequireSnapshot(added > 0 && (size_t)added < sizeof(triangles) - used, "triangles_buffer");
			used += added;
		}
		RequireSnapshot((size_t)used + 2 < sizeof(triangles), "triangles_close");
		triangles[used++] = ']';
		triangles[used] = 0;
		velqor::Emit("scene_mesh_triangles", triangles);
	}
}

}

void CaptureControllerScene(void* controller)
{
	NxActor* owner = ((NxController*)controller)->getActor();
	RequireSnapshot(owner != 0, "controller_actor");
	NxScene& scene = owner->getScene();
	if (g_captured_scene == &scene)
		return;
	NxFilterOp op0, op1, op2;
	scene.getFilterOps(op0, op1, op2);
	const NxGroupsMask filter0 = scene.getFilterConstant0();
	const NxGroupsMask filter1 = scene.getFilterConstant1();
	NxVec3 gravity;
	scene.getGravity(gravity);
	unsigned int fpu_control = 0;
	RequireSnapshot(_controlfp_s(&fpu_control, 0, 0) == 0, "fpu_control");
	const NxU32 count = scene.getNbActors();
	char state[640];
	snprintf(state, sizeof(state),
		"\"scene_id\":%u,\"actor_count\":%u,\"filter_ops\":[%u,%u,%u],\"filter_bool\":%u,\"filter0\":[%u,%u,%u,%u],\"filter1\":[%u,%u,%u,%u],\"gravity\":[%.9g,%.9g,%.9g],\"fpu_control\":%u",
		(uint32_t)(uintptr_t)&scene, count, (unsigned int)op0, (unsigned int)op1, (unsigned int)op2,
		scene.getFilterBool() ? 1u : 0u,
		filter0.bits0, filter0.bits1, filter0.bits2, filter0.bits3,
		filter1.bits0, filter1.bits1, filter1.bits2, filter1.bits3,
		(double)gravity.x, (double)gravity.y, (double)gravity.z, fpu_control);
	velqor::Emit("scene_snapshot", state);
	const NxU32 material_count = scene.getNbMaterials();
	RequireSnapshot(material_count <= 256, "material_count");
	NxMaterial* materials[256];
	NxU32 material_iterator = 0;
	const NxU32 material_written = scene.getMaterialArray(materials, material_count, material_iterator);
	RequireSnapshot(material_written == material_count, "material_array");
	for (NxU32 i = 0; i < material_count; ++i)
	{
		NxMaterialDesc description;
		materials[i]->saveToDesc(description);
		char fields[768];
		snprintf(fields, sizeof(fields),
			"\"index\":%u,\"dynamic_friction\":%.9g,\"static_friction\":%.9g,\"restitution\":%.9g,\"dynamic_friction_v\":%.9g,\"static_friction_v\":%.9g,\"anisotropy\":[%.9g,%.9g,%.9g],\"flags\":%u,\"friction_combine\":%u,\"restitution_combine\":%u",
			(unsigned int)materials[i]->getMaterialIndex(), (double)description.dynamicFriction,
			(double)description.staticFriction, (double)description.restitution,
			(double)description.dynamicFrictionV, (double)description.staticFrictionV,
			(double)description.dirOfAnisotropy.x, (double)description.dirOfAnisotropy.y, (double)description.dirOfAnisotropy.z,
			description.flags, (unsigned int)description.frictionCombineMode, (unsigned int)description.restitutionCombineMode);
		velqor::Emit("scene_material", fields);
	}
	const void* meshes[256];
	size_t mesh_count = 0;
	NxActor** actors = scene.getActors();
	RequireSnapshot(count == 0 || actors != 0, "scene_actors");
	for (NxU32 i = 0; i < count; ++i)
	{
		NxActor& actor = *actors[i];
		const uint32_t actor_id = (uint32_t)(uintptr_t)&actor;
		NxShape* const* shapes = actor.getShapes();
		EmitPose(actor_id, 0, "actor_global", actor.getGlobalPose());
		NxActorDesc actor_description;
		actor.saveToDesc(actor_description);
		char actor_name[1024];
		EncodeName(actor_name, sizeof(actor_name), actor_description.name);
		char actor_fields[1400];
		snprintf(actor_fields, sizeof(actor_fields),
			"\"actor_id\":%u,\"name\":%s,\"flags\":%u,\"group\":%u,\"density\":%.9g,\"contact_report\":%u",
			actor_id, actor_name, actor_description.flags, (unsigned int)actor_description.group,
			(double)actor_description.density, actor_description.contactReportFlags);
		velqor::Emit("scene_actor_desc", actor_fields);
		if (actor.isDynamic())
		{
			NxBodyDesc body;
			RequireSnapshot(actor.saveBodyToDesc(body), "body_description");
			EmitPose(actor_id, 0, "body_mass_local", body.massLocalPose);
			char fields[1200];
			snprintf(fields, sizeof(fields),
				"\"actor_id\":%u,\"mass\":%.9g,\"inertia\":[%.9g,%.9g,%.9g],\"linear_velocity\":[%.9g,%.9g,%.9g],\"angular_velocity\":[%.9g,%.9g,%.9g],\"flags\":%u,\"wake\":%.9g,\"linear_damping\":%.9g,\"angular_damping\":%.9g,\"max_angular_velocity\":%.9g,\"ccd_threshold\":%.9g,\"sleep_linear_velocity\":%.9g,\"sleep_angular_velocity\":%.9g,\"solver_iterations\":%u,\"sleep_energy\":%.9g,\"sleep_damping\":%.9g,\"contact_threshold\":%.9g",
				actor_id, (double)body.mass, (double)body.massSpaceInertia.x, (double)body.massSpaceInertia.y, (double)body.massSpaceInertia.z,
				(double)body.linearVelocity.x, (double)body.linearVelocity.y, (double)body.linearVelocity.z,
				(double)body.angularVelocity.x, (double)body.angularVelocity.y, (double)body.angularVelocity.z, body.flags,
				(double)body.wakeUpCounter, (double)body.linearDamping, (double)body.angularDamping, (double)body.maxAngularVelocity,
				(double)body.CCDMotionThreshold, (double)body.sleepLinearVelocity, (double)body.sleepAngularVelocity,
				(unsigned int)body.solverIterationCount, (double)body.sleepEnergyThreshold, (double)body.sleepDamping, (double)body.contactReportThreshold);
			velqor::Emit("scene_body_desc", fields);
		}
		for (NxU32 j = 0; j < actor.getNbShapes(); ++j)
		{
			NxShape& shape = *shapes[j];
			const uint32_t shape_id = (uint32_t)(uintptr_t)&shape;
			const NxGroupsMask mask = shape.getGroupsMask();
			const void* mesh_id = 0;
			if (shape.getType() == NX_SHAPE_MESH)
				mesh_id = &shape.isTriangleMesh()->getTriangleMesh();
			else if (shape.getType() == NX_SHAPE_CONVEX)
				mesh_id = &shape.isConvexMesh()->getConvexMesh();
			char properties[640];
			snprintf(properties, sizeof(properties),
				"\"actor_id\":%u,\"shape_id\":%u,\"mesh_id\":%u,\"type\":%u,\"group\":%u,\"groups_mask\":[%u,%u,%u,%u],\"material\":%u,\"dynamic\":%u,\"kinematic\":%u,\"disabled\":%u,\"frame\":%llu",
				actor_id, shape_id, (uint32_t)(uintptr_t)mesh_id, (unsigned int)shape.getType(), (unsigned int)shape.getGroup(),
				mask.bits0, mask.bits1, mask.bits2, mask.bits3, (unsigned int)shape.getMaterial(),
				actor.isDynamic() ? 1u : 0u, actor.isDynamic() && actor.readBodyFlag(NX_BF_KINEMATIC) ? 1u : 0u,
				shape.getFlag(NX_SF_DISABLE_COLLISION) ? 1u : 0u, (unsigned long long)velqor::CurrentFrame());
			velqor::Emit("scene_shape", properties);
			EmitPose(actor_id, shape_id, "shape_local", shape.getLocalPose());
			EmitPose(actor_id, shape_id, "shape_global", shape.getGlobalPose());
			if (shape.getType() == NX_SHAPE_BOX)
			{
				NxBoxShapeDesc description;
				shape.isBox()->saveToDesc(description);
				EmitShapeDescription(actor_id, shape_id, description);
				char fields[192];
				snprintf(fields, sizeof(fields), "\"shape_id\":%u,\"dimensions\":[%.9g,%.9g,%.9g]", shape_id,
					(double)description.dimensions.x, (double)description.dimensions.y, (double)description.dimensions.z);
				velqor::Emit("scene_box", fields);
			}
			else if (shape.getType() == NX_SHAPE_CAPSULE)
			{
				NxCapsuleShapeDesc description;
				shape.isCapsule()->saveToDesc(description);
				EmitShapeDescription(actor_id, shape_id, description);
				char fields[192];
				snprintf(fields, sizeof(fields), "\"shape_id\":%u,\"radius\":%.9g,\"height\":%.9g", shape_id,
					(double)description.radius, (double)description.height);
				velqor::Emit("scene_capsule", fields);
			}
			else if (shape.getType() == NX_SHAPE_MESH)
			{
				NxTriangleMeshShapeDesc description;
				shape.isTriangleMesh()->saveToDesc(description);
				EmitShapeDescription(actor_id, shape_id, description);
			}
			else
			{
				RequireSnapshot(shape.getType() == NX_SHAPE_CONVEX, "shape_type");
				NxConvexShapeDesc description;
				shape.isConvexMesh()->saveToDesc(description);
				EmitShapeDescription(actor_id, shape_id, description);
			}
			if (mesh_id == 0)
				continue;
			bool seen = false;
			for (size_t k = 0; k < mesh_count; ++k)
				if (meshes[k] == mesh_id)
					seen = true;
			if (seen)
				continue;
			RequireSnapshot(mesh_count < sizeof(meshes) / sizeof(meshes[0]), "mesh_capacity");
			meshes[mesh_count++] = mesh_id;
			if (shape.getType() == NX_SHAPE_MESH)
			{
				NxTriangleMeshDesc description;
				RequireSnapshot(shape.isTriangleMesh()->getTriangleMesh().saveToDesc(description), "triangle_description");
				EmitMesh((uint32_t)(uintptr_t)mesh_id, "triangle", description);
			}
			else
			{
				NxConvexMeshDesc description;
				RequireSnapshot(shape.isConvexMesh()->getConvexMesh().saveToDesc(description), "convex_description");
				EmitMesh((uint32_t)(uintptr_t)mesh_id, "convex", description);
			}
		}
	}
	g_captured_scene = &scene;
	velqor::Emit("scene_snapshot_complete", state);
}
