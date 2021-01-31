#pragma once
#ifdef CONF_WINDOWS

#define SOKOL_D3D11

#include <common/vector_math.h>
#include <EASTL/fixed_map.h>
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"

typedef eastl::fixed_string<char,32,false> FixedStr32;

struct Line
{
	vec3 p0;
	u32 c0;
	vec3 p1;
	u32 c1;
};

struct LineBuffer
{
private:
	eastl::fixed_vector<Line,8192*1024,false> ramBuffer;
	sg_buffer vramBuffer = { 0xFFFFFFFF };
	bool needsUpdate = false;

public:

	sg_buffer GetUpdatedBuffer();
	void Push(const Line& line);
	void Clear();

	inline i32 GetLineCount() const
	{
		return ramBuffer.size();
	}
};

struct MeshBuffer
{
	struct Vertex
	{
		f32 px, py, pz;
		f32 nx, ny, nz;
	};
	STATIC_ASSERT(sizeof(Vertex) == 24);

	struct MeshRef
	{
		u32 indexStart;
		u32 indexCount;
	};

	// these need to be static
	eastl::fixed_vector<Vertex,8192*1024,false> vertexBuffer;
	eastl::fixed_vector<u16,8192*1024,false> indexBuffer;
	sg_buffer gpuVertexBuff = { 0xFFFFFFFF };
	sg_buffer gpuIndexBuff = { 0xFFFFFFFF };
	eastl::fixed_map<FixedStr32, MeshRef, 2048, false> meshRefMap;
	bool needsUpdate = false;

	void Push(const FixedStr32& name, const Vertex* vertices, const u32 vertexCount, const u16* indices, const u32 indexCount);
	void UpdateAndBind();
	void DrawMesh(const FixedStr32& name);
};

struct Camera
{
	enum Controls
	{
		Forward = 0,
		Backward,
		Left,
		Right,
		Up,
		Down,
		_COUNT
	};

	vec3 eye;
	vec3 dir;
	vec3 vel;
	f32 speed = 2000;
	const vec3 up = vec3(0, 0, 1);
	u8 keyState[Controls::_COUNT] = {0};
	bool mouseLocked = false;
	const f32 invSens = 200;

	void Reset();

	bool HandleEvent(const sapp_event& event);

	mat4 UpdateAndComputeMatrix(f32 delta);
};

struct Window;

struct Renderer
{
	const i32 winWidth;
	const i32 winHeight;
	sg_pipeline pipeMeshShaded;
	sg_pipeline pipeMeshUnlit;
	sg_pipeline pipeLine;
	sg_shader shaderMeshShaded;
	sg_shader shaderMeshUnlit;
	sg_shader shaderLine;

	LineBuffer lineBuffer;
	MeshBuffer meshBuffer;

	struct InstanceMesh
	{
		FixedStr32 meshName;
		vec3 pos;
		vec3 rot = vec3(0);
		vec3 scale = vec3(1);
		vec3 color = vec3(1);
	};

	eastl::fixed_vector<InstanceMesh, 1024, true> drawQueueMesh;
	eastl::fixed_vector<InstanceMesh, 1024, true> drawQueueMeshUnlit;

	Camera camera;

	bool Init();
	bool OpenAndLoadMeshFile(const char* name, const char* path);

	inline void PushMesh(const InstanceMesh& mesh)
	{
		drawQueueMesh.push_back(mesh);
	}

	inline void PushMeshUnlit(const InstanceMesh& mesh)
	{
		drawQueueMeshUnlit.push_back(mesh);
	}

	void Render(f64 delta);
	void OnEvent(const sapp_event& event);
};

#endif
