#include "renderer.h"
#include "shaders.h"
#include "sokol_imgui.h"
#include <game/config.h>

sg_buffer LineBuffer::GetUpdatedBuffer()
{
	if(needsUpdate) {
		if(vramBuffer.id == 0xFFFFFFFF) {
			sg_buffer_desc desc = {0};
			desc.size = sizeof(Line) * ramBuffer.capacity();
			desc.data.ptr = nullptr;
			desc.data.size = 0;
			desc.usage = SG_USAGE_STREAM;
			vramBuffer = sg_make_buffer(&desc);
		}

		sg_update_buffer(vramBuffer, { ramBuffer.data(), ramBuffer.size() * sizeof(Line) });
		needsUpdate = false;
	}
	return vramBuffer;
}

void LineBuffer::Push(const Line& line)
{
	ramBuffer.push_back(line);
	needsUpdate = true;
}

void LineBuffer::Clear()
{
	ramBuffer.clear();
	needsUpdate = true;
}

void MeshBuffer::Push(const FixedStr64& name, const MeshBuffer::Vertex* vertices, const u32 vertexCount, const u16* indices, const u32 indexCount)
{
	ASSERT(meshRefMap.find(name) == meshRefMap.end());

	const u32 vertexStartIndex = vertexBuffer.size();

	MeshRef ref;
	ref.indexStart = indexBuffer.size();
	ref.indexCount = indexCount;
	vertexBuffer.insert(vertexBuffer.end(), vertices, vertices + vertexCount);

	for(int i = 0; i < indexCount; i++) {
		indexBuffer.push_back(vertexStartIndex + indices[i]);
	}

	meshRefMap[name] = ref;
	needsUpdate = true;
}

void MeshBuffer::UpdateAndBind()
{
	if(needsUpdate) {
		if(gpuVertexBuff.id == 0xFFFFFFFF) {
			{
				sg_buffer_desc desc = {0};
				desc.size = sizeof(Vertex) * vertexBuffer.capacity();
				desc.data.ptr = vertexBuffer.data();
				desc.data.size = desc.size;
				gpuVertexBuff = sg_make_buffer(&desc);
			}

			{
				sg_buffer_desc desc = {0};
				desc.type = SG_BUFFERTYPE_INDEXBUFFER;
				desc.size = sizeof(u16) * indexBuffer.capacity();
				desc.data.ptr = indexBuffer.data();
				desc.data.size = desc.size;
				gpuIndexBuff = sg_make_buffer(&desc);
			}
		}
		else {
			sg_update_buffer(gpuVertexBuff, { vertexBuffer.data(), vertexBuffer.capacity() * sizeof(Vertex) });
			sg_update_buffer(gpuIndexBuff, { indexBuffer.data(), indexBuffer.capacity() * sizeof(u16) });
		}
		needsUpdate = false;
	}

	sg_bindings binds = {0};
	binds.vertex_buffers[0] = gpuVertexBuff;
	binds.index_buffer = gpuIndexBuff;
	sg_apply_bindings(&binds);
}

void MeshBuffer::DrawMesh(const FixedStr64& name)
{
	const MeshRef& ref = meshRefMap.at(name);
	sg_draw(ref.indexStart, ref.indexCount, 1);
}

bool MeshBuffer::HasMesh(const FixedStr64& name) const
{
	return meshRefMap.find(name) != meshRefMap.end();
}

sg_buffer TriangleBuffer::GetUpdatedBuffer()
{
	if(needsUpdate) {
		if(vramBuffer.id == 0xFFFFFFFF) {
			sg_buffer_desc desc = {0};
			desc.size = sizeof(TriangleMesh) * triangleList.capacity();
			desc.data.ptr = nullptr;
			desc.data.size = 0;
			desc.usage = SG_USAGE_STREAM;
			vramBuffer = sg_make_buffer(&desc);
		}

		sg_update_buffer(vramBuffer, { triangleList.data(), triangleList.size() * sizeof(TriangleMesh) });
		needsUpdate = false;
	}
	return vramBuffer;
}

void TriangleBuffer::Push(const TriangleMesh& triangle)
{
	triangleList.push_back(triangle);
	needsUpdate = true;
}

void TriangleBuffer::Clear()
{
	triangleList.clear();
	needsUpdate = true;
}

void Camera::Save()
{
	CConfig& cfg = ConfigMutable();
	cfg.DbgCamPosX = eye.x;
	cfg.DbgCamPosY = eye.y;
	cfg.DbgCamPosZ = eye.z;
	cfg.DbgCamDirX = dir.x;
	cfg.DbgCamDirY = dir.y;
	cfg.DbgCamDirZ = dir.z;
}

void Camera::Reset()
{
	const CConfig& cfg = Config();
	eye = vec3(cfg.DbgCamPosX, cfg.DbgCamPosY, cfg.DbgCamPosZ);
	dir = vec3(cfg.DbgCamDirX, cfg.DbgCamDirY, cfg.DbgCamDirZ);
}

bool Camera::HandleEvent(const sapp_event& event)
{
	const vec3 right = glm::normalize(glm::cross(dir, up));

	if(event.type == SAPP_EVENTTYPE_KEY_DOWN) {
		switch(event.key_code) {
			case sapp_keycode::SAPP_KEYCODE_W: keyState[Controls::Forward] = 1; break;
			case sapp_keycode::SAPP_KEYCODE_S: keyState[Controls::Backward] = 1; break;
			case sapp_keycode::SAPP_KEYCODE_D: keyState[Controls::Right] = 1; break;
			case sapp_keycode::SAPP_KEYCODE_A: keyState[Controls::Left] = 1; break;
			case sapp_keycode::SAPP_KEYCODE_SPACE: keyState[Controls::Up] = 1; break;
			case sapp_keycode::SAPP_KEYCODE_LEFT_CONTROL: keyState[Controls::Down] = 1; break;
			case sapp_keycode::SAPP_KEYCODE_F1: {
				Reset();
			} break;

			case sapp_keycode::SAPP_KEYCODE_Z: {
				mouseLocked = !mouseLocked;
				sapp_lock_mouse(mouseLocked);
			} break;
		}
	}
	else if(event.type == SAPP_EVENTTYPE_KEY_UP) {
		switch(event.key_code) {
			case sapp_keycode::SAPP_KEYCODE_W: keyState[Controls::Forward] = 0; break;
			case sapp_keycode::SAPP_KEYCODE_S: keyState[Controls::Backward] = 0; break;
			case sapp_keycode::SAPP_KEYCODE_D: keyState[Controls::Right] = 0; break;
			case sapp_keycode::SAPP_KEYCODE_A: keyState[Controls::Left] = 0; break;
			case sapp_keycode::SAPP_KEYCODE_SPACE: keyState[Controls::Up] = 0; break;
			case sapp_keycode::SAPP_KEYCODE_LEFT_CONTROL: keyState[Controls::Down] = 0; break;
		}
	}
	else if(event.type == SAPP_EVENTTYPE_MOUSE_MOVE) {
		if(mouseLocked) {
			mat4 rot = glm::rotate(glm::identity<mat4>(), -event.mouse_dy / invSens, right); // pitch
			rot = glm::rotate(rot, -event.mouse_dx / invSens, up); // yaw
			dir = glm::normalize(rot * vec4(dir, 0));
		}
	}
	else if(event.type == SAPP_EVENTTYPE_MOUSE_SCROLL) {
		if(mouseLocked) {
			speed *= 1 + event.scroll_y * 0.05;
			speed = clamp(speed, 1.f, 100000.f);
		}
	}

	return false;
}

mat4 Camera::UpdateAndComputeMatrix(f32 delta)
{
	if(fabs(dir.z) > 0.9999f) {
		dir.y = 0.01f;
		dir = glm::normalize(dir);
	}

	const vec3 right = glm::normalize(glm::cross(dir, up));

	if(mouseLocked) {
		if(keyState[Controls::Forward]) {
			eye += dir * speed * delta;
		}
		else if(keyState[Controls::Backward]) {
			eye += dir * -speed * delta;
		}
		if(keyState[Controls::Left]) {
			eye += right * -speed * delta;
		}
		else if(keyState[Controls::Right]) {
			eye += right * speed * delta;
		}
		if(keyState[Controls::Up]) {
			eye += up * speed * delta;
		}
		else if(keyState[Controls::Down]) {
			eye += up * -speed * delta;
		}
	}

	return glm::lookAt(eye, eye + dir, up);
}

template<typename OutputIteratorVertices, typename OutputIteratorIndices>
void GenerateCapsuleMesh(const f32 height, const f32 radius, const i32 subDivisions, OutputIteratorVertices outVert, OutputIteratorIndices outInd)
{
	// base is 0,0,0

	typedef MeshBuffer::Vertex Vert;

	*outVert++ = Vert{ 0, 0, 0, 0, 0, -1 };

	for(i32 s = 0; s < subDivisions; s++) {
		*outInd++ = 0;
		*outInd++ = s+1;
		*outInd++ = (s+1) % (subDivisions+1) + 1;
	}

	i32 vertCount = 1;
	const f32 subHeight = height / subDivisions;
	for(f32 z = subHeight; z < height; z += subHeight) {
		for(i32 s = 0; s < subDivisions+1; s++) {
			const f32 a = (2 * PI) / subDivisions * s;
			f32 r = radius;
			vec3 n;
			if(radius - z > 0) {
				f32 d = radius - z;
				r = sqrtf(radius*radius - d*d);
				n = glm::normalize(vec3(cosf(a) * r, sinf(a) * r, -d));
			}
			else if((height - z) < radius) {
				f32 d = radius - (height - z);
				r = sqrtf(radius*radius - d*d);
				n = glm::normalize(vec3(cosf(a) * r, sinf(a) * r, d));
			}
			else {
				n = vec3(cosf(a), sinf(a), 0);
			}

			*outVert++ = Vert{ cosf(a) * r, sinf(a) * r, z, n.x, n.y, n.z };
			vertCount++;
		}

		if(z > subHeight) {
			for(i32 s = 0; s < subDivisions; s++) {
				i32 b0 = vertCount - ((subDivisions+1) * 2) + s;
				i32 b1 = vertCount - ((subDivisions+1) * 2) + s + 1;
				i32 t0 = vertCount - subDivisions + s;
				i32 t1 = vertCount - subDivisions + s + 1;

				if(s+1 == subDivisions) {
					b1 = vertCount - ((subDivisions+1) * 2);
					t1 = vertCount - subDivisions;
				}

				*outInd++ = b0;
				*outInd++ = t0;
				*outInd++ = b1;

				*outInd++ = b1;
				*outInd++ = t0;
				*outInd++ = t1;
			}
		}
	}

	const i32 last = vertCount;
	for(i32 s = 0; s < subDivisions; s++) {
		*outInd++ = last - 1 - subDivisions + s;
		*outInd++ = last;
		*outInd++ = last - 1 - subDivisions + (s+1) % (subDivisions+1);
	}

	*outVert++ = Vert{ 0, 0, height, 0, 0, 1 };
}

template<typename OutputIteratorVertices, typename OutputIteratorIndices>
void GenerateSphereMesh(const f32 radius, const i32 subDivisions, OutputIteratorVertices outVert, OutputIteratorIndices outInd)
{
	// base is 0,0,0

	typedef MeshBuffer::Vertex Vert;

	*outVert++ = Vert{ 0, 0, 0, 0, 0, -1 };

	for(i32 s = 0; s < subDivisions; s++) {
		*outInd++ = 0;
		*outInd++ = s+1;
		*outInd++ = (s+1) % (subDivisions+1) + 1;
	}

	const f32 height = radius * 2;

	i32 vertCount = 1;
	const f32 subHeight = height / subDivisions;
	for(f32 z = subHeight; z < height; z += subHeight) {
		for(i32 s = 0; s < subDivisions+1; s++) {
			const f32 a = (2 * PI) / subDivisions * s;
			f32 r = radius;
			vec3 n;
			if(radius - z > 0) {
				f32 d = radius - z;
				r = sqrtf(radius*radius - d*d);
				n = glm::normalize(vec3(cosf(a) * r, sinf(a) * r, -d));
			}
			else if((height - z) < radius) {
				f32 d = radius - (height - z);
				r = sqrtf(radius*radius - d*d);
				n = glm::normalize(vec3(cosf(a) * r, sinf(a) * r, d));
			}
			else {
				n = vec3(cosf(a), sinf(a), 0);
			}

			*outVert++ = Vert{ cosf(a) * r, sinf(a) * r, z, n.x, n.y, n.z };
			vertCount++;
		}

		if(z > subHeight) {
			for(i32 s = 0; s < subDivisions; s++) {
				i32 b0 = vertCount - ((subDivisions+1) * 2) + s;
				i32 b1 = vertCount - ((subDivisions+1) * 2) + s + 1;
				i32 t0 = vertCount - subDivisions + s;
				i32 t1 = vertCount - subDivisions + s + 1;

				if(s+1 == subDivisions) {
					b1 = vertCount - ((subDivisions+1) * 2);
					t1 = vertCount - subDivisions;
				}

				*outInd++ = b0;
				*outInd++ = t0;
				*outInd++ = b1;

				*outInd++ = b1;
				*outInd++ = t0;
				*outInd++ = t1;
			}
		}
	}

	const i32 last = vertCount;
	for(i32 s = 0; s < subDivisions; s++) {
		*outInd++ = last - 1 - subDivisions + s;
		*outInd++ = last;
		*outInd++ = last - 1 - subDivisions + (s+1) % (subDivisions+1);
	}

	*outVert++ = Vert{ 0, 0, height, 0, 0, 1 };
}

template<typename OutputIteratorVertices, typename OutputIteratorIndices>
void GenerateCylinderMesh(const f32 height, const f32 radius, const i32 subDivisions, OutputIteratorVertices outVert, OutputIteratorIndices outInd)
{
	// base is 0,0,0

	typedef MeshBuffer::Vertex Vert;

	*outVert++ = Vert{ 0, 0, 0, 0, 0, -1 };

	for(i32 s = 0; s < subDivisions; s++) {
		*outInd++ = 0;
		*outInd++ = s+1;
		*outInd++ = (s+1) % (subDivisions+1) + 1;
	}

	i32 vertCount = 1;
	const f32 subHeight = height / subDivisions;
	for(f32 z = subHeight; z < height; z += subHeight) {
		for(i32 s = 0; s < subDivisions+1; s++) {
			const f32 a = (2 * PI) / subDivisions * s;
			f32 r = radius;
			vec3 n = vec3(cosf(a), sinf(a), 0);

			*outVert++ = Vert{ cosf(a) * r, sinf(a) * r, z, n.x, n.y, n.z };
			vertCount++;
		}

		if(z > subHeight) {
			for(i32 s = 0; s < subDivisions; s++) {
				i32 b0 = vertCount - ((subDivisions+1) * 2) + s;
				i32 b1 = vertCount - ((subDivisions+1) * 2) + s + 1;
				i32 t0 = vertCount - subDivisions + s;
				i32 t1 = vertCount - subDivisions + s + 1;

				if(s+1 == subDivisions) {
					b1 = vertCount - ((subDivisions+1) * 2);
					t1 = vertCount - subDivisions;
				}

				*outInd++ = b0;
				*outInd++ = t0;
				*outInd++ = b1;

				*outInd++ = b1;
				*outInd++ = t0;
				*outInd++ = t1;
			}
		}
	}

	const i32 last = vertCount;
	for(i32 s = 0; s < subDivisions; s++) {
		*outInd++ = last - 1 - subDivisions + s;
		*outInd++ = last;
		*outInd++ = last - 1 - subDivisions + (s+1) % (subDivisions+1);
	}

	*outVert++ = Vert{ 0, 0, height, 0, 0, 1 };
}

template<typename OutputIteratorVertices, typename OutputIteratorIndices>
void GenerateFlatRingMesh(const f32 height, const f32 outerRadius, const f32 innerRadius, const i32 subDivisions, OutputIteratorVertices outVert, OutputIteratorIndices outInd)
{
	ASSERT(outerRadius > innerRadius);

	typedef MeshBuffer::Vertex Vert;

	int vertCount = 0;
	for(int s = 0; s < subDivisions; s++) {
		f32 a0 = (2*PI / subDivisions) * s;
		f32 a1 = (2*PI / subDivisions) * (s + 1);
		f32 ca0 = cosf(a0);
		f32 ca1 = cosf(a1);
		f32 sa0 = sinf(a0);
		f32 sa1 = sinf(a1);

		vec3 pi0 = vec3(ca0 * innerRadius, sa0 * innerRadius, 0);
		vec3 pi1 = vec3(ca1 * innerRadius, sa1 * innerRadius, 0);
		vec3 pi2 = vec3(ca0 * innerRadius, sa0 * innerRadius, height);
		vec3 pi3 = vec3(ca1 * innerRadius, sa1 * innerRadius, height);
		vec3 po0 = vec3(ca0 * outerRadius, sa0 * outerRadius, 0);
		vec3 po1 = vec3(ca1 * outerRadius, sa1 * outerRadius, 0);
		vec3 po2 = vec3(ca0 * outerRadius, sa0 * outerRadius, height);
		vec3 po3 = vec3(ca1 * outerRadius, sa1 * outerRadius, height);
		vec3 n0 = glm::normalize(-pi0);
		vec3 n1 = glm::normalize(-pi1);

		// inner
		*outVert++ = Vert{ pi0.x, pi0.y, pi0.z, n0.x, n0.y, n0.z };
		*outVert++ = Vert{ pi1.x, pi1.y, pi1.z, n1.x, n1.y, n1.z };
		*outVert++ = Vert{ pi2.x, pi2.y, pi2.z, n0.x, n0.y, n0.z };
		*outVert++ = Vert{ pi3.x, pi3.y, pi3.z, n1.x, n1.y, n1.z };

		*outInd++ = vertCount + 0;
		*outInd++ = vertCount + 1;
		*outInd++ = vertCount + 2;

		*outInd++ = vertCount + 1;
		*outInd++ = vertCount + 3;
		*outInd++ = vertCount + 2;

		vertCount += 4;

		// outer
		*outVert++ = Vert{ po0.x, po0.y, po0.z, -n0.x, -n0.y, -n0.z };
		*outVert++ = Vert{ po1.x, po1.y, po1.z, -n1.x, -n1.y, -n1.z };
		*outVert++ = Vert{ po2.x, po2.y, po2.z, -n0.x, -n0.y, -n0.z };
		*outVert++ = Vert{ po3.x, po3.y, po3.z, -n1.x, -n1.y, -n1.z };

		*outInd++ = vertCount + 0;
		*outInd++ = vertCount + 2;
		*outInd++ = vertCount + 1;

		*outInd++ = vertCount + 1;
		*outInd++ = vertCount + 2;
		*outInd++ = vertCount + 3;

		vertCount += 4;

		// top
		*outVert++ = Vert{ pi2.x, pi2.y, pi2.z, 0, 0, 1 };
		*outVert++ = Vert{ pi3.x, pi3.y, pi3.z, 0, 0, 1 };
		*outVert++ = Vert{ po2.x, po2.y, po2.z, 0, 0, 1 };
		*outVert++ = Vert{ po3.x, po3.y, po3.z, 0, 0, 1 };

		*outInd++ = vertCount + 0;
		*outInd++ = vertCount + 1;
		*outInd++ = vertCount + 2;

		*outInd++ = vertCount + 1;
		*outInd++ = vertCount + 3;
		*outInd++ = vertCount + 2;

		vertCount += 4;

		// bottom
		*outVert++ = Vert{ pi0.x, pi0.y, pi0.z, 0, 0, -1 };
		*outVert++ = Vert{ pi1.x, pi1.y, pi1.z, 0, 0, -1 };
		*outVert++ = Vert{ po0.x, po0.y, po0.z, 0, 0, -1 };
		*outVert++ = Vert{ po1.x, po1.y, po1.z, 0, 0, -1 };

		*outInd++ = vertCount + 0;
		*outInd++ = vertCount + 2;
		*outInd++ = vertCount + 1;

		*outInd++ = vertCount + 1;
		*outInd++ = vertCount + 2;
		*outInd++ = vertCount + 3;

		vertCount += 4;
	}
}

template<typename OutputIteratorVertices, typename OutputIteratorIndices>
void GenerateConeMesh(const f32 height, const f32 radius, const i32 subDivisions, OutputIteratorVertices outVert, OutputIteratorIndices outInd)
{
	typedef MeshBuffer::Vertex Vert;

	int vertCount = 0;

	enum VertID {
		Base = 0,
	};

	*outVert++ = Vert{ 0, 0, 0, 0, 0, -1 }; // base
	vertCount += 1;

	for(int s = 0; s < subDivisions; s++) {
		f32 a0 = (2*PI / subDivisions) * s;
		f32 a1 = (2*PI / subDivisions) * (s + 1);
		f32 ca0 = cosf(a0);
		f32 ca1 = cosf(a1);
		f32 sa0 = sinf(a0);
		f32 sa1 = sinf(a1);

		vec3 pi0 = vec3(ca0 * radius, sa0 * radius, 0);
		vec3 pi1 = vec3(ca1 * radius, sa1 * radius, 0);

		// base
		*outVert++ = Vert{ pi0.x, pi0.y, pi0.z, 0, 0, -1 };
		*outVert++ = Vert{ pi1.x, pi1.y, pi1.z, 0, 0, -1 };

		*outInd++ = VertID::Base;
		*outInd++ = vertCount + 0;
		*outInd++ = vertCount + 1;

		vertCount += 2;

		// tip
		// TODO: fix normals (cross product)
		vec3 tn = glm::normalize(vec3(0, 0, height) + ((pi0 + pi1) * 0.5f));
		vec3 n0 = glm::normalize(vec3(0, 0, height) + pi0);
		vec3 n1 = glm::normalize(vec3(0, 0, height) + pi1);
		*outVert++ = Vert{ 0    , 0   , height, tn.x, tn.y, tn.z };
		*outVert++ = Vert{ pi0.x, pi0.y, pi0.z, n0.x, n0.y, n0.z };
		*outVert++ = Vert{ pi1.x, pi1.y, pi1.z, n1.x, n1.y, n1.z };


		*outInd++ = vertCount + 0;
		*outInd++ = vertCount + 2;
		*outInd++ = vertCount + 1;

		vertCount += 3;
	}
}

bool Renderer::Init()
{
	// setup sokol_gfx
	sg_desc gfxDesc = {0};
	gfxDesc.context = sapp_sgcontext();
	sg_setup(&gfxDesc);

	const MeshBuffer::Vertex verticesCubeCentered[] = {
		{ -0.5, -0.5, -0.5, 0, 0, -1 },
		{  0.5, -0.5, -0.5, 0, 0, -1 },
		{  0.5,  0.5, -0.5, 0, 0, -1 },
		{ -0.5,  0.5, -0.5, 0, 0, -1 },

		{ -0.5, -0.5,  0.5, 0, 0, 1 },
		{  0.5, -0.5,  0.5, 0, 0, 1 },
		{  0.5,  0.5,  0.5, 0, 0, 1 },
		{ -0.5,  0.5,  0.5, 0, 0, 1 },

		{ -0.5, -0.5, -0.5, -1, 0, 0 },
		{ -0.5,  0.5, -0.5, -1, 0, 0 },
		{ -0.5,  0.5,  0.5, -1, 0, 0 },
		{ -0.5, -0.5,  0.5, -1, 0, 0 },

		{  0.5, -0.5, -0.5, 1, 0, 0 },
		{  0.5,  0.5, -0.5, 1, 0, 0 },
		{  0.5,  0.5,  0.5, 1, 0, 0 },
		{  0.5, -0.5,  0.5, 1, 0, 0 },

		{ -0.5, -0.5, -0.5, 0, -1, 0 },
		{ -0.5, -0.5,  0.5, 0, -1, 0 },
		{  0.5, -0.5,  0.5, 0, -1, 0 },
		{  0.5, -0.5, -0.5, 0, -1, 0 },

		{ -0.5,  0.5, -0.5, 0, 1, 0 },
		{ -0.5,  0.5,  0.5, 0, 1, 0 },
		{  0.5,  0.5,  0.5, 0, 1, 0 },
		{  0.5,  0.5, -0.5, 0, 1, 0 },
	};

	const MeshBuffer::Vertex verticesCube[] = {
		{ 0.0, 0.0, 0.0, 0, 0, -1 },
		{ 1.0, 0.0, 0.0, 0, 0, -1 },
		{ 1.0, 1.0, 0.0, 0, 0, -1 },
		{ 0.0, 1.0, 0.0, 0, 0, -1 },

		{ 0.0, 0.0, 1.0, 0, 0, 1 },
		{ 1.0, 0.0, 1.0, 0, 0, 1 },
		{ 1.0, 1.0, 1.0, 0, 0, 1 },
		{ 0.0, 1.0, 1.0, 0, 0, 1 },

		{ 0.0, 0.0, 0.0, -1, 0, 0 },
		{ 0.0, 1.0, 0.0, -1, 0, 0 },
		{ 0.0, 1.0, 1.0, -1, 0, 0 },
		{ 0.0, 0.0, 1.0, -1, 0, 0 },

		{ 1.0, 0.0, 0.0, 1, 0, 0 },
		{ 1.0, 1.0, 0.0, 1, 0, 0 },
		{ 1.0, 1.0, 1.0, 1, 0, 0 },
		{ 1.0, 0.0, 1.0, 1, 0, 0 },

		{ 0.0, 0.0, 0.0, 0, -1, 0 },
		{ 0.0, 0.0, 1.0, 0, -1, 0 },
		{ 1.0, 0.0, 1.0, 0, -1, 0 },
		{ 1.0, 0.0, 0.0, 0, -1, 0 },

		{ 0.0, 1.0, 0.0, 0, 1, 0 },
		{ 0.0, 1.0, 1.0, 0, 1, 0 },
		{ 1.0, 1.0, 1.0, 0, 1, 0 },
		{ 1.0, 1.0, 0.0, 0, 1, 0 },
	};

	const u16 indices[] = {
		0, 1, 2,  0, 2, 3,
		6, 5, 4,  7, 6, 4,
		8, 9, 10,  8, 10, 11,
		14, 13, 12,  15, 14, 12,
		16, 17, 18,  16, 18, 19,
		22, 21, 20, 23, 22, 20
	};

	meshBuffer.Push("Cube", verticesCube, ARRAY_COUNT(verticesCube), indices, ARRAY_COUNT(indices));
	meshBuffer.Push("CubeCentered", verticesCubeCentered, ARRAY_COUNT(verticesCubeCentered), indices, ARRAY_COUNT(indices));

	eastl::fixed_vector<MeshBuffer::Vertex,1024,true> genVertList;
	eastl::fixed_vector<u16,1024,true> genIndList;

	/*
	GenerateCapsuleMesh(1, 0.2f, 32, eastl::back_inserter(genVertList), eastl::back_inserter(genIndList));
	meshBuffer.Push("Capsule", genVertList.data(), genVertList.size(), genIndList.data(), genIndList.size());
	genVertList.clear();
	genIndList.clear();
	*/

	GenerateFlatRingMesh(0.1f, 1, 0.8f, 32, eastl::back_inserter(genVertList), eastl::back_inserter(genIndList));
	meshBuffer.Push("Ring", genVertList.data(), genVertList.size(), genIndList.data(), genIndList.size());
	genVertList.clear();
	genIndList.clear();

	GenerateConeMesh(1, 1, 32, eastl::back_inserter(genVertList), eastl::back_inserter(genIndList));
	meshBuffer.Push("Cone", genVertList.data(), genVertList.size(), genIndList.data(), genIndList.size());
	genVertList.clear();
	genIndList.clear();

	GenerateCylinderMesh(1, 1, 32, eastl::back_inserter(genVertList), eastl::back_inserter(genIndList));
	meshBuffer.Push("Cylinder", genVertList.data(), genVertList.size(), genIndList.data(), genIndList.size());
	genVertList.clear();
	genIndList.clear();

	GenerateSphereMesh(1, 32, eastl::back_inserter(genVertList), eastl::back_inserter(genIndList));
	meshBuffer.Push("Sphere", genVertList.data(), genVertList.size(), genIndList.data(), genIndList.size());
	genVertList.clear();
	genIndList.clear();

	shaderMeshShaded = sg_make_shader(&ShaderBaseMeshShaded());
	shaderMeshUnlit = sg_make_shader(&ShaderBaseMeshUnlit());
	shaderLine = sg_make_shader(&ShaderLine());

	// basic mesh pipeline
	{
		sg_pipeline_desc pipeDesc = {0};
		pipeDesc.shader = shaderMeshShaded;
		pipeDesc.layout.buffers[0].stride = sizeof(MeshBuffer::Vertex);
		pipeDesc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
		pipeDesc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT3;
		pipeDesc.index_type = SG_INDEXTYPE_UINT16;
		pipeDesc.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
		pipeDesc.depth.write_enabled = true;
		pipeDesc.cull_mode = SG_CULLMODE_BACK;
		sg_pipeline_desc pipeDescDs = pipeDesc;
		pipeDescDs.cull_mode = SG_CULLMODE_NONE;
		pipeMeshShaded = sg_make_pipeline(&pipeDesc);
		pipeMeshShadedDoubleSided = sg_make_pipeline(&pipeDescDs);
	}

	// mesh unlit
	{
		sg_pipeline_desc pipeDesc = {0};
		pipeDesc.shader = shaderMeshUnlit;
		pipeDesc.layout.buffers[0].stride = sizeof(MeshBuffer::Vertex);
		pipeDesc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
		pipeDesc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT3;
		pipeDesc.index_type = SG_INDEXTYPE_UINT16;
		pipeDesc.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
		pipeDesc.depth.write_enabled = true;
		pipeDesc.cull_mode = SG_CULLMODE_BACK;
		//pipeDesc.rasterizer.cull_mode = SG_CULLMODE_NONE;
		pipeMeshUnlit = sg_make_pipeline(&pipeDesc);
	}

	// mesh wireframe
	{
		sg_pipeline_desc pipeDesc = {0};
		pipeDesc.shader = shaderMeshUnlit;
		pipeDesc.primitive_type = SG_PRIMITIVETYPE_LINES;
		pipeDesc.layout.buffers[0].stride = sizeof(MeshBuffer::Vertex);
		pipeDesc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
		pipeDesc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT3;
		pipeDesc.index_type = SG_INDEXTYPE_UINT16;
		pipeDesc.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
		pipeDesc.depth.write_enabled = true;
		pipeDesc.cull_mode = SG_CULLMODE_BACK;
		//pipeDesc.rasterizer.cull_mode = SG_CULLMODE_NONE;
		pipeMeshWire = sg_make_pipeline(&pipeDesc);
	}

	// line pipeline
	{
		sg_pipeline_desc linePipeDesc = {0};
		linePipeDesc.shader = shaderLine;
		linePipeDesc.primitive_type = SG_PRIMITIVETYPE_LINES;
		linePipeDesc.layout.buffers[0].stride = sizeof(Line)/2;
		linePipeDesc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
		linePipeDesc.layout.attrs[1].format = SG_VERTEXFORMAT_UBYTE4N;
		linePipeDesc.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
		linePipeDesc.depth.write_enabled = true;
		pipeLine = sg_make_pipeline(&linePipeDesc);
	}

	// triangle pipeline
	{
		sg_pipeline_desc trianglePipeDesc = {0};
		trianglePipeDesc.shader = shaderLine;
		trianglePipeDesc.layout.buffers[0].stride = sizeof(TriangleMesh)/3;
		trianglePipeDesc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
		trianglePipeDesc.layout.attrs[1].format = SG_VERTEXFORMAT_UBYTE4N;
		trianglePipeDesc.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
		trianglePipeDesc.depth.write_enabled = true;
		trianglePipeDesc.cull_mode = SG_CULLMODE_NONE; // double sided
		pipeTriangle = sg_make_pipeline(&trianglePipeDesc);
	}

	camera.Reset();
	return true;
}

void Renderer::Cleanup()
{
	camera.Save();
}

void Renderer::LoadMeshFile(const char* name, const MeshFile::Mesh& mesh)
{
	STATIC_ASSERT(sizeof(MeshBuffer::Vertex) == sizeof(MeshFile::Vertex)); // hacky
	meshBuffer.Push(name, (MeshBuffer::Vertex*)mesh.vertices, mesh.vertexCount, mesh.indices, mesh.indexCount);
}

void Renderer::PushArrow(Pipeline pipeline, const vec3& start, const vec3& end, const vec3& color, f32 thickness, const InstanceMesh* parent)
{
	vec3 dir = glm::normalize(end - start);

	f32 yaw = atan2(dir.y, dir.x);
	f32 pitch = asinf(dir.z);
	f32 len = glm::length(end - start);
	f32 coneSize = thickness * 1.2f;
	f32 coneHeight = coneSize * 2;
	f32 armLen = len - coneHeight;
	f32 sizeY = thickness;

	PushMesh(pipeline, "CubeCentered", start + dir * (armLen/2), vec3(yaw, pitch, 0), vec3(armLen, sizeY, sizeY), color, parent);
	PushMesh(pipeline, "Cone", end - dir * coneHeight, vec3(yaw, pitch - PI/2, 0), vec3(coneSize, coneSize, coneHeight), color, parent);
}

void Renderer::PushCapsule(Pipeline pipeline, const vec3& pos, const vec3& rot, f32 radius, f32 height, const vec3& color, const InstanceMesh* parent)
{
	auto base = PushAnchor(pos, rot, vec3(1), parent);
	PushMesh(pipeline, "Sphere", vec3(0), vec3(0), vec3(radius), color, base);
	PushMesh(pipeline, "Sphere", vec3(0, 0, height - radius * 2), vec3(0), vec3(radius), color, base);
	PushMesh(pipeline, "Cylinder", vec3(0, 0, radius), vec3(0), vec3(radius, radius, height - radius * 2), color, base);
}

void Renderer::PushLine(const vec3& start, const vec3& end, const vec3& color)
{
	u32 c = CU3(color.x, color.y, color.z);
	lineBuffer.Push({ start, c, end, c });
}

template<class Transform>
inline mat4 MakeModelMatrixFromTransform(const Transform& t)
{
	mat4 model = glm::identity<mat4>();
	model = glm::translate(model, t.pos);
	model = model * glm::eulerAngleZYX(t.rot.x, -t.rot.y, t.rot.z);
	model = glm::scale(model, t.scale);
	return model;
}

template<class Transform>
inline mat4 MakeModelMatrixFromTransformHierarchy(const Transform& t)
{
	eastl::fixed_list<const Transform*, 2048, false> parentHierarchy;
	const Transform* cur = &t;
	while(cur->parent) {
		parentHierarchy.push_front(cur->parent);
		cur = cur->parent;
	}

	mat4 model = glm::identity<mat4>();
	foreach_const(p, parentHierarchy) {
		model = model * MakeModelMatrixFromTransform(**p);
	}
	model = model * MakeModelMatrixFromTransform(t);
	return model;
}

void Renderer::Render(f64 delta)
{
	// push a simple grid
	const u32 lineColor = 0xFF7F7F7F;
	const f32 lineSpacing = 100.0f;
	const f32 lineLength = 100000.0f;
	for(int i = -500; i < 500; i++) {
		// color x, y axis
		if(i == 0) {
			lineBuffer.Push({vec3(lineLength, 0, 0), 0xFFFF0000, vec3(-lineLength, 0, 0), 0xFFFF0000});
			lineBuffer.Push({vec3(0, lineLength, 0), 0xFF00FF00, vec3(0, -lineLength, 0), 0xFF00FF00});
			lineBuffer.Push({vec3(0, 0, lineLength), 0xFF0000FF, vec3(0, 0, -lineLength), 0xFF0000FF});
			continue;
		}
		lineBuffer.Push({vec3(lineLength, i * lineSpacing, 0), lineColor, vec3(-lineLength, i * lineSpacing, 0), lineColor});
		lineBuffer.Push({vec3(i * lineSpacing, lineLength, 0), lineColor, vec3(i * lineSpacing, -lineLength, 0), lineColor});
	}

	// origin
	const f32 orgnLen = 100;
	const f32 orgnThick = 2;
	PushMesh(Pipeline::Unlit, "CubeCentered", vec3(0), vec3(0), vec3(orgnThick*2), vec3(1));
	PushMesh(Pipeline::Unlit, "CubeCentered", vec3(orgnLen/2, 0, 0), vec3(0), vec3(orgnLen, orgnThick, orgnThick), vec3(0, 0, 1));
	PushMesh(Pipeline::Unlit, "CubeCentered", vec3(0, orgnLen/2, 0), vec3(0), vec3(orgnThick, orgnLen, orgnThick), vec3(0, 1, 0));
	PushMesh(Pipeline::Unlit, "CubeCentered", vec3(0, 0, orgnLen/2), vec3(0), vec3(orgnThick, orgnThick, orgnLen), vec3(1, 0, 0));

	//static Time lastTime = TimeNow();
	//Time curTime = TimeNow();
	//f32 a = sin((u64)curTime / 1000000000.0);
	const vec3 sunPos = vec3(5000, 5000, 2000);
	PushMesh(Pipeline::Unlit, "CubeCentered", sunPos, vec3(0), vec3(100), vec3(1));

	// rendering
	const f32 viewDist = 5.0f;
	mat4 proj = glm::perspective(glm::radians(60.0f), (f32)winWidth/(f32)winHeight, 1.0f, 20000.0f);
	mat4 view = camera.UpdateAndComputeMatrix(delta);

	sg_pass_action pass_action = {0}; // default pass action (clear to grey)
	sg_begin_default_pass(&pass_action, winWidth, winHeight);

	// draw meshes
	{
		// shaded
		sg_apply_pipeline(pipeMeshShaded);
		meshBuffer.UpdateAndBind();

		foreach_const(it, drawQueue[(i32)Pipeline::Shaded]) {
			if(it->meshName.empty()) continue; // ignore null meshes

			mat4 model = MakeModelMatrixFromTransformHierarchy(*it);

			mat4 normalMat = glm::transpose(glm::inverse(model));
			ShaderMeshShaded::VsUniform0 vsUni0 = { proj * view, model, normalMat };
			ShaderMeshShaded::FsUniform0 fsUni0 = { it->color, sunPos, vec3(1) };

			sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, { &vsUni0, sizeof(vsUni0) });
			sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, { &fsUni0, sizeof(fsUni0) });

			meshBuffer.DrawMesh(it->meshName);
		}

		// double sided
		sg_apply_pipeline(pipeMeshShadedDoubleSided);
		meshBuffer.UpdateAndBind();

		foreach_const(it, drawQueue[(i32)Pipeline::ShadedDoubleSided]) {
			mat4 model = MakeModelMatrixFromTransformHierarchy(*it);

			mat4 normalMat = glm::transpose(glm::inverse(model));
			ShaderMeshShaded::VsUniform0 vsUni0 = { proj * view, model, normalMat };
			ShaderMeshShaded::FsUniform0 fsUni0 = { it->color, sunPos, vec3(1) };

			sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, { &vsUni0, sizeof(vsUni0) });
			sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, { &fsUni0, sizeof(fsUni0) });

			meshBuffer.DrawMesh(it->meshName);
		}

		// unlit
		sg_apply_pipeline(pipeMeshUnlit);
		meshBuffer.UpdateAndBind();

		foreach_const(it, drawQueue[(i32)Pipeline::Unlit]) {
			mat4 model = MakeModelMatrixFromTransformHierarchy(*it);

			struct Uniform0
			{
				mat4 mvp;
			};

			Uniform0 uni0 = { proj * view * model };

			sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, { &uni0, sizeof(uni0) });
			sg_apply_uniforms(SG_SHADERSTAGE_VS, 1, { &it->color, sizeof(it->color) });

			meshBuffer.DrawMesh(it->meshName);
		}

		// wireframe
		sg_apply_pipeline(pipeMeshWire);
		meshBuffer.UpdateAndBind();

		foreach_const(it, drawQueue[(i32)Pipeline::Wireframe]) {
			mat4 model = MakeModelMatrixFromTransformHierarchy(*it);

			struct Uniform0
			{
				mat4 mvp;
			};

			Uniform0 uni0 = { proj * view * model };

			sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, { &uni0, sizeof(uni0) });
			sg_apply_uniforms(SG_SHADERSTAGE_VS, 1, { &it->color, sizeof(it->color) });

			meshBuffer.DrawMesh(it->meshName);
		}
	}

	// draw lines
	if(lineBuffer.GetLineCount() > 0) {
		mat4 mvp = proj * view;

		sg_bindings bindsLine = {0};
		bindsLine.vertex_buffers[0] = lineBuffer.GetUpdatedBuffer();

		sg_apply_pipeline(pipeLine);
		sg_apply_bindings(&bindsLine);
		sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, { &mvp, sizeof(mvp) });
		sg_draw(0, lineBuffer.GetLineCount() * 2, 1);
	}

	// draw triangles
	if(triangleBuffer.GetLineCount() > 0) {
		mat4 mvp = proj * view;

		sg_bindings binds = {0};
		binds.vertex_buffers[0] = triangleBuffer.GetUpdatedBuffer();

		sg_apply_pipeline(pipeTriangle);
		sg_apply_bindings(&binds);
		sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, { &mvp, sizeof(mvp) });
		sg_draw(0, triangleBuffer.GetLineCount() * 3, 1);
	}

	simgui_render();
	sg_end_pass();
	sg_commit();

	foreach(q, drawQueue) {
		q->clear();
	}

	lineBuffer.Clear();
	triangleBuffer.Clear();
}

void Renderer::OnEvent(const sapp_event& event)
{
	camera.HandleEvent(event);
}
