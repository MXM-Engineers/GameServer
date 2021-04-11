#pragma once
#ifdef CONF_WINDOWS

#define SOKOL_D3D11

#include <common/vector_math.h>
#include <common/utils.h>
#include <EASTL/fixed_map.h>
#include <EASTL/fixed_list.h>
#include <EASTL/array.h>
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"

inline u32 CU3(f32 r, f32 g, f32 b)
{
	return 0xFF000000 | (u8(b * 255) << 16) | (u8(g * 255) << 8) | (u8(r * 255));
}

inline f64 saw(f64 a)
{
	f64 f = glm::fract(a);
	return f < 0.5 ? f * 2 : 1.0 - (f - 0.5) * 2;
}

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

	bool HasMesh(const FixedStr32& name) const;
};

struct TrianglePoint
{
	vec3 pos;
	u32 color;
};

typedef eastl::array<TrianglePoint,3> TriangleMesh;

struct TriangleBuffer
{
	eastl::fixed_vector<TriangleMesh,8192,false> triangleList;
	sg_buffer vramBuffer = { 0xFFFFFFFF };
	bool needsUpdate = false;

	sg_buffer GetUpdatedBuffer();
	void Push(const TriangleMesh& triangle);
	void Clear();

	inline i32 GetLineCount() const
	{
		return triangleList.size();
	}
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

	void Save();
	void Reset();

	bool HandleEvent(const sapp_event& event);

	mat4 UpdateAndComputeMatrix(f32 delta);
};

struct Window;

enum class Pipeline
{
	Shaded = 0,
	ShadedDoubleSided,
	Unlit,
	Wireframe,
	_Count
};

struct Renderer
{
	const i32 winWidth;
	const i32 winHeight;
	sg_pipeline pipeMeshShaded;
	sg_pipeline pipeMeshShadedDoubleSided;
	sg_pipeline pipeMeshUnlit;
	sg_pipeline pipeMeshWire;
	sg_pipeline pipeLine;
	sg_pipeline pipeTriangle;
	sg_shader shaderMeshShaded;
	sg_shader shaderMeshUnlit;
	sg_shader shaderLine;

	LineBuffer lineBuffer;
	MeshBuffer meshBuffer;
	TriangleBuffer triangleBuffer;

	struct InstanceMesh
	{
		const InstanceMesh* parent;
		FixedStr32 meshName;
		vec3 pos;
		vec3 rot = vec3(0); // yaw, pitch, roll
		vec3 scale = vec3(1);
		vec3 color = vec3(1);
	};

	eastl::array<eastl::fixed_list<InstanceMesh, 4096, true>, (i32)Pipeline::_Count> drawQueue;

	Camera camera;

	bool Init();
	void Cleanup();
	bool OpenAndLoadMeshFile(const char* name, const char* path);

	// return ID
	inline const InstanceMesh* PushMesh(
		Pipeline pipeline,
		const FixedStr32& meshName,
		const vec3& pos,
		const vec3& rot = vec3(0),
		const vec3& scale = vec3(1),
		const vec3& color = vec3(1),
		const InstanceMesh* parent = nullptr)
	{
		DBG_ASSERT(meshBuffer.HasMesh(meshName));
		drawQueue[(i32)pipeline].push_back(InstanceMesh{ parent, meshName, pos, rot, scale, color });
		return &drawQueue[(i32)pipeline].back();
	}

	// null mesh, only used for parent transforms
	inline const InstanceMesh* PushAnchor(
			const vec3& pos,
			const vec3& rot = vec3(0),
			const vec3& scale = vec3(1),
			const InstanceMesh* parent = nullptr)
	{
		drawQueue[0].push_back(InstanceMesh{ parent, "", pos, rot, scale, vec3(0) });
		return &drawQueue[0].back();
	}

	void PushArrow(Pipeline pipeline, const vec3& start, const vec3& end, const vec3& color, f32 thickness, const InstanceMesh* parent = nullptr);
	void PushCapsule(Pipeline pipeline, const vec3& pos, const vec3& rot, f32 radius, f32 height, const vec3& color, const InstanceMesh* parent = nullptr);

	void PushLine(const vec3& start, const vec3& end, const vec3& color);

	void Render(f64 delta);
	void OnEvent(const sapp_event& event);
};

#endif
