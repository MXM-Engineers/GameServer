#ifdef CONF_WINDOWS
#include "window.h"

#include "config.h"
#include "render/shaders.h"
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <EASTL/fixed_map.h>

#define SOKOL_IMPL
#define SOKOL_NO_ENTRY
#define SOKOL_D3D11

#define SG_DEFAULT_CLEAR_RED 0.2f
#define SG_DEFAULT_CLEAR_GREEN 0.2f
#define SG_DEFAULT_CLEAR_BLUE 0.2f

#define SOKOL_ASSERT ASSERT
#define SOKOL_LOG LOG

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"

typedef glm::vec3 vec3;
typedef glm::vec4 vec4;
typedef glm::mat4 mat4;

constexpr f64 PI = glm::pi<f64>();

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
	eastl::fixed_vector<Line,2048,false> ramBuffer;
	sg_buffer vramBuffer = { 0xFFFFFFFF };
	bool needsUpdate = false;

public:

	sg_buffer GetUpdatedBuffer()
	{
		if(needsUpdate) {
			if(vramBuffer.id == 0xFFFFFFFF) {
				sg_buffer_desc desc = {0};
				desc.size = sizeof(Line) * ramBuffer.capacity();
				desc.content = ramBuffer.data();
				vramBuffer = sg_make_buffer(&desc);
			}
			else {
				sg_update_buffer(vramBuffer, ramBuffer.data(), ramBuffer.capacity() * sizeof(Line));
			}
			needsUpdate = false;
		}
		return vramBuffer;
	}

	void Push(const Line& line)
	{
		ramBuffer.push_back(line);
		needsUpdate = true;
	}

	void Clear()
	{
		ramBuffer.clear();
		needsUpdate = true;
	}

	inline i32 GetLineCount() const
	{
		return ramBuffer.size();
	}
};

typedef eastl::fixed_string<char,32,false> FixedStr32;

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

	eastl::fixed_vector<Vertex,8192,false> vertexBuffer;
	eastl::fixed_vector<u16,8192,false> indexBuffer;
	sg_buffer gpuVertexBuff = { 0xFFFFFFFF };
	sg_buffer gpuIndexBuff = { 0xFFFFFFFF };
	eastl::fixed_map<FixedStr32, MeshRef, 2048, false> meshRefMap;
	bool needsUpdate = false;

	void Push(const FixedStr32& name, const Vertex* vertices, const u32 vertexCount, const u16* indices, const u32 indexCount)
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

	void UpdateAndBind()
	{
		if(needsUpdate) {
			if(gpuVertexBuff.id == 0xFFFFFFFF) {
				{
					sg_buffer_desc desc = {0};
					desc.size = sizeof(Vertex) * vertexBuffer.capacity();
					desc.content = vertexBuffer.data();
					gpuVertexBuff = sg_make_buffer(&desc);
				}

				{
					sg_buffer_desc desc = {0};
					desc.type = SG_BUFFERTYPE_INDEXBUFFER;
					desc.size = sizeof(u16) * indexBuffer.capacity();
					desc.content = indexBuffer.data();
					gpuIndexBuff = sg_make_buffer(&desc);
				}
			}
			else {
				sg_update_buffer(gpuVertexBuff, vertexBuffer.data(), vertexBuffer.capacity() * sizeof(Vertex));
				sg_update_buffer(gpuIndexBuff, indexBuffer.data(), indexBuffer.capacity() * sizeof(u16));
			}
			needsUpdate = false;
		}

		sg_bindings binds = {0};
		binds.vertex_buffers[0] = gpuVertexBuff;
		binds.index_buffer = gpuIndexBuff;
		sg_apply_bindings(&binds);
	}

	void DrawMesh(const FixedStr32& name)
	{
		const MeshRef& ref = meshRefMap.at(name);
		sg_draw(ref.indexStart, ref.indexCount, 1);
	}
};

struct Window
{
	const i32 winWidth;
	const i32 winHeight;
	sg_pipeline pipeMeshShaded;
	sg_pipeline pipeMeshUnlit;
	sg_pipeline pipeLine;
	sg_shader shaderMeshShaded;
	sg_shader shaderMeshUnlit;
	sg_shader shaderLine;
	Time startTime = Time::ZERO;
	Time localTime = Time::ZERO;

	LineBuffer lineBuffer;
	MeshBuffer meshBuffer;

	struct InstanceMesh
	{
		FixedStr32 meshName;
		vec3 pos;
		vec3 rot;
		vec3 scale;
		vec3 color;
	};

	eastl::fixed_vector<InstanceMesh, 1024, true> drawQueueMesh;
	eastl::fixed_vector<InstanceMesh, 1024, true> drawQueueMeshUnlit;

	vec3 cameraEye = vec3(0, 0, 6000);

	bool Init()
	{
		startTime = TimeNow();

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

			{ -0.5, -0.5, -0.5, 1, 0, 0 },
			{ -0.5,  0.5, -0.5, 1, 0, 0 },
			{ -0.5,  0.5,  0.5, 1, 0, 0 },
			{ -0.5, -0.5,  0.5, 1, 0, 0 },

			{  0.5, -0.5, -0.5, -1, 0, 0 },
			{  0.5,  0.5, -0.5, -1, 0, 0 },
			{  0.5,  0.5,  0.5, -1, 0, 0 },
			{  0.5, -0.5,  0.5, -1, 0, 0 },

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

			{ 0.0, 0.0, 0.0, 1, 0, 0 },
			{ 0.0, 1.0, 0.0, 1, 0, 0 },
			{ 0.0, 1.0, 1.0, 1, 0, 0 },
			{ 0.0, 0.0, 1.0, 1, 0, 0 },

			{ 1.0, 0.0, 0.0, -1, 0, 0 },
			{ 1.0, 1.0, 0.0, -1, 0, 0 },
			{ 1.0, 1.0, 1.0, -1, 0, 0 },
			{ 1.0, 0.0, 1.0, -1, 0, 0 },

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
		bool r = OpenAndLoadMeshFile("PVP_DeathMatchCollision", "gamedata/PVP_DeathMatchCollision.msh");
		if(!r) return false;

		shaderMeshShaded = sg_make_shader(&ShaderBaseMeshShaded());
		shaderMeshUnlit = sg_make_shader(&ShaderBaseMeshUnlit());
		shaderLine = sg_make_shader(&ShaderLine());

		// basic mesh pipeline
		sg_pipeline_desc pipeDesc;
		memset(&pipeDesc, 0, sizeof(pipeDesc));
		pipeDesc.shader = shaderMeshShaded;
		pipeDesc.layout.buffers[0].stride = sizeof(MeshBuffer::Vertex);
		pipeDesc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
		pipeDesc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT3;
		pipeDesc.index_type = SG_INDEXTYPE_UINT16;
		pipeDesc.depth_stencil.depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL;
		pipeDesc.depth_stencil.depth_write_enabled = true;
		pipeDesc.rasterizer.cull_mode = SG_CULLMODE_BACK;
		pipeMeshShaded = sg_make_pipeline(&pipeDesc);

		memset(&pipeDesc, 0, sizeof(pipeDesc));
		pipeDesc.shader = shaderMeshUnlit;
		pipeDesc.layout.buffers[0].stride = sizeof(MeshBuffer::Vertex);
		pipeDesc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
		pipeDesc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT3;
		pipeDesc.index_type = SG_INDEXTYPE_UINT16;
		pipeDesc.depth_stencil.depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL;
		pipeDesc.depth_stencil.depth_write_enabled = true;
		pipeDesc.rasterizer.cull_mode = SG_CULLMODE_BACK;
		pipeMeshUnlit = sg_make_pipeline(&pipeDesc);

		// line pipeline
		sg_pipeline_desc linePipeDesc = {0};
		linePipeDesc.shader = shaderLine;
		linePipeDesc.primitive_type = SG_PRIMITIVETYPE_LINES;
		linePipeDesc.layout.buffers[0].stride = sizeof(Line)/2;
		linePipeDesc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
		linePipeDesc.layout.attrs[1].format = SG_VERTEXFORMAT_UBYTE4N;
		linePipeDesc.depth_stencil.depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL;
		linePipeDesc.depth_stencil.depth_write_enabled = true;
		pipeLine = sg_make_pipeline(&linePipeDesc);

		// push a simple grid
		const u32 lineColor = 0xFF7F7F7F;
		const f32 lineSpacing = 100.0f;
		const f32 lineLength = 100000.0f;
		for(int i = -500; i < 500; i++) {
			// color x, y axis
			if(i == 0) {
				lineBuffer.Push({vec3(lineLength, 0, 0), 0xFF0000FF, vec3(-lineLength, 0, 0), 0xFF0000FF});
				lineBuffer.Push({vec3(0, lineLength, 0), 0xFF00FF00, vec3(0, -lineLength, 0), 0xFF00FF00});
				lineBuffer.Push({vec3(0, 0, 10000), 0xFFFF0000, vec3(0, 0, -10000), 0xFFFF0000});
				continue;
			}
			lineBuffer.Push({vec3(lineLength, i * lineSpacing, 0), lineColor, vec3(-lineLength, i * lineSpacing, 0), lineColor});
			lineBuffer.Push({vec3(i * lineSpacing, lineLength, 0), lineColor, vec3(i * lineSpacing, -lineLength, 0), lineColor});
		}

		return true;
	}

	bool OpenAndLoadMeshFile(const char* name, const char* path)
	{
		struct MeshFileHeader
		{
			u32 magic;
			u32 vertexCount;
			u32 indexCount;
		};

		i32 fileSize;
		u8* fileBuff = fileOpenAndReadAll(path, &fileSize);
		if(!fileBuff) {
			LOG("ERROR: faield to open '%s'", path);
			return false;
		}
		defer(memFree(fileBuff));

		ConstBuffer buff(fileBuff, fileSize);
		const MeshFileHeader& header = buff.Read<MeshFileHeader>();
		const MeshBuffer::Vertex* vertices = (MeshBuffer::Vertex*)buff.ReadRaw(sizeof(MeshBuffer::Vertex) * header.vertexCount);
		const u16* indices = (u16*)buff.ReadRaw(sizeof(u16) * header.indexCount);

		meshBuffer.Push(name, vertices, header.vertexCount, indices, header.indexCount);
		return true;
	}

	void Frame()
	{
		Time now = TimeNow();
		localTime = TimeDiff(startTime, now);

		// origin
		const f32 orgnLen = 1000;
		const f32 orgnThick = 20;
		drawQueueMeshUnlit.push_back({ "CubeCentered", vec3(0), vec3(0), vec3(orgnThick*2), vec3(1) });
		drawQueueMeshUnlit.push_back({ "CubeCentered", vec3(orgnLen/2, 0, 0), vec3(0), vec3(orgnLen, orgnThick, orgnThick), vec3(1, 0, 0) });
		drawQueueMeshUnlit.push_back({ "CubeCentered", vec3(0, orgnLen/2, 0), vec3(0), vec3(orgnThick, orgnLen, orgnThick), vec3(0, 1, 0) });
		drawQueueMeshUnlit.push_back({ "CubeCentered", vec3(0, 0, orgnLen/2), vec3(0), vec3(orgnThick, orgnThick, orgnLen), vec3(0, 0, 1) });

		f32 a = fmod(TimeDiffSec(localTime)*0.2 * glm::pi<f32>() * 2.0f, 4.0f * glm::pi<f32>());

		// test map
		drawQueueMesh.push_back({ "PVP_DeathMatchCollision", vec3(0, 0, 0), vec3(0, 0, 0), vec3(1), vec3(1, 0, 1) });

		const f32 viewDist = 5.0f;
		mat4 proj = glm::perspective(glm::radians(60.0f), (float)winWidth/(float)winHeight, 1.0f, 10000.0f);
		//mat4 view = glm::lookAt(vec3(1, -viewDist, sinf(a) * 5), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f));
		vec3 center = vec3(cameraEye.x, cameraEye.y + 0.001, 0);
		mat4 view = glm::lookAt(cameraEye, center, vec3(0.0f, 0.0f, 1.0f));

		sg_pass_action pass_action = {0}; // default pass action (clear to grey)
		sg_begin_default_pass(&pass_action, winWidth, winHeight);

		// draw meshes
		{
			sg_apply_pipeline(pipeMeshShaded);
			meshBuffer.UpdateAndBind();

			foreach_const(it, drawQueueMesh) {
				mat4 model = glm::translate(glm::identity<mat4>(), it->pos);
				model = model * glm::eulerAngleYXZ(it->rot.x, it->rot.y, it->rot.z);
				model = glm::scale(model, it->scale);

				ShaderMeshShaded::VsUniform0 vsUni0 = { proj * view, model };
				ShaderMeshShaded::FsUniform0 fsUni0 = { it->color, vec3(5, 0, 0) };

				sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &vsUni0, sizeof(vsUni0));
				sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, &fsUni0, sizeof(fsUni0));

				meshBuffer.DrawMesh(it->meshName);
			}

			sg_apply_pipeline(pipeMeshUnlit);
			meshBuffer.UpdateAndBind();

			foreach_const(it, drawQueueMeshUnlit) {
				mat4 model = glm::translate(glm::identity<mat4>(), it->pos);
				model = model * glm::eulerAngleYXZ(it->rot.x, it->rot.y, it->rot.z);
				model = glm::scale(model, it->scale);

				struct Uniform0
				{
					mat4 mvp;
				};

				Uniform0 uni0 = { proj * view * model };

				sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &uni0, sizeof(uni0));
				sg_apply_uniforms(SG_SHADERSTAGE_VS, 1, &it->color, sizeof(it->color));

				meshBuffer.DrawMesh(it->meshName);
			}
		}

		// draw lines
		{
			mat4 mvp = proj * view;

			sg_bindings bindsLine = {0};
			bindsLine.vertex_buffers[0] = lineBuffer.GetUpdatedBuffer();

			sg_apply_pipeline(pipeLine);
			sg_apply_bindings(&bindsLine);
			sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &mvp, sizeof(mvp));
			sg_draw(0, lineBuffer.GetLineCount() * 2, 1);
		}

		sg_end_pass();
		sg_commit();

		drawQueueMesh.clear();
		drawQueueMeshUnlit.clear();
	}

	void OnEvent(const sapp_event& event)
	{
		// quit on escape
		if(event.type == SAPP_EVENTTYPE_KEY_DOWN) {
			if(event.key_code == sapp_keycode::SAPP_KEYCODE_ESCAPE) {
				sapp_request_quit();
			}
			else if(event.key_code == sapp_keycode::SAPP_KEYCODE_W) {
				cameraEye.y += 100.0f;
			}
			else if(event.key_code == sapp_keycode::SAPP_KEYCODE_S) {
				cameraEye.y -= 100.0f;
			}
			else if(event.key_code == sapp_keycode::SAPP_KEYCODE_A) {
				cameraEye.x -= 100.0f;
			}
			else if(event.key_code == sapp_keycode::SAPP_KEYCODE_D) {
				cameraEye.x += 100.0f;
			}
		}
		else if(event.type == SAPP_EVENTTYPE_MOUSE_SCROLL) {
			cameraEye.z += -event.scroll_y * 50;
		}
	}

	void Cleanup()
	{
		sg_shutdown();
	}
};

static Window* g_pWindow = nullptr;

void WindowInit()
{
	bool r = g_pWindow->Init();
	if(!r) {
		LOG("ERROR: failed to init window");
		sapp_request_quit();
	}
}

void WindowFrame()
{
	g_pWindow->Frame();
}

void WindowEvent(const sapp_event* event)
{
	g_pWindow->OnEvent(*event);
}

void WindowCleanup()
{
	g_pWindow->Cleanup();
}

intptr_t ThreadWindow(void* pData)
{
	ProfileSetThreadName("Window");
	const i32 cpuID = 0;
	EA::Thread::SetThreadAffinityMask(1 << cpuID);

	static Window win = {Config().WindowWidth, Config().WindowHeight};
	g_pWindow = &win;

	sapp_desc desc;
	memset(&desc, 0, sizeof(desc));
	desc.window_title = "MxM server";
	// MSAA
	desc.sample_count = 8;
	desc.width = win.winWidth;
	desc.height = win.winHeight;
	desc.init_cb = WindowInit;
	desc.frame_cb = WindowFrame;
	desc.event_cb = WindowEvent;
	desc.cleanup_cb = WindowCleanup;
	sapp_run(&desc);

	LOG("Window closed.");
	GenerateConsoleCtrlEvent(0, 0); // so the whole server shuts down when closing the window
	return 0;
}

#endif

void WindowClose()
{
	sapp_request_quit();
}
