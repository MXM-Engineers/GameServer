#ifdef CONF_WINDOWS
#include "window.h"

#include "config.h"
#include "render/shaders.h"
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define SOKOL_IMPL
#define SOKOL_NO_ENTRY
#define SOKOL_D3D11

#define SG_DEFAULT_CLEAR_RED 0.2f
#define SG_DEFAULT_CLEAR_GREEN 0.2f
#define SG_DEFAULT_CLEAR_BLUE 0.2f

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"

typedef glm::vec3 vec3;
typedef glm::vec4 vec4;
typedef glm::mat4 mat4;

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

struct Window
{
	const i32 winWidth;
	const i32 winHeight;
	sg_buffer vbuf;
	sg_buffer ibuf;
	sg_pipeline pip;
	sg_pipeline pipeLine;
	sg_shader shaderBaseShaded;
	Time startTime = Time::ZERO;
	Time localTime = Time::ZERO;

	LineBuffer lineBuffer;

	void Init()
	{
		startTime = TimeNow();

		// setup sokol_gfx
		sg_desc gfxDesc = {0};
		gfxDesc.context = sapp_sgcontext();
		sg_setup(&gfxDesc);

		// a vertex buffer
		struct Vertex
		{
			f32 x, y ,z;
			u32 color;
		};
		STATIC_ASSERT(sizeof(Vertex) == 16);

		const Vertex vertices[] = {
			{ -0.5, -0.5, -0.5, 0xFF0000FF },
			{ 0.5, -0.5, -0.5,  0xFF0000FF },
			{ 0.5,  0.5, -0.5,  0xFF0000FF },
			{ -0.5,  0.5, -0.5, 0xFF0000FF },

			{ -0.5, -0.5,  0.5, 0xFF00FF00 },
			{ 0.5, -0.5,  0.5,  0xFF00FF00 },
			{ 0.5,  0.5,  0.5,  0xFF00FF00 },
			{ -0.5,  0.5,  0.5, 0xFF00FF00 },

			{ -0.5, -0.5, -0.5, 0xFFFF0000 },
			{ -0.5,  0.5, -0.5, 0xFFFF0000 },
			{ -0.5,  0.5,  0.5, 0xFFFF0000 },
			{ -0.5, -0.5,  0.5, 0xFFFF0000 },

			{ 0.5, -0.5, -0.5,  0xFF007FFF },
			{ 0.5,  0.5, -0.5,  0xFF007FFF },
			{ 0.5,  0.5,  0.5,  0xFF007FFF },
			{ 0.5, -0.5,  0.5,  0xFF007FFF },

			{ -0.5, -0.5, -0.5, 0xFFFF7F00 },
			{ -0.5, -0.5,  0.5, 0xFFFF7F00 },
			{ 0.5, -0.5,  0.5,  0xFFFF7F00 },
			{ 0.5, -0.5, -0.5,  0xFFFF7F00 },

			{ -0.5,  0.5, -0.5, 0xFF7F00FF },
			{ -0.5,  0.5,  0.5, 0xFF7F00FF },
			{ 0.5,  0.5,  0.5,  0xFF7F00FF },
			{ 0.5,  0.5, -0.5,  0xFF7F00FF },
		};

		sg_buffer_desc vertexBuffDesc = {0};
		vertexBuffDesc.size = sizeof(vertices);
		vertexBuffDesc.content = vertices;
		vbuf = sg_make_buffer(&vertexBuffDesc);

		const u16 indices[] = {
			0, 1, 2,  0, 2, 3,
			6, 5, 4,  7, 6, 4,
			8, 9, 10,  8, 10, 11,
			14, 13, 12,  15, 14, 12,
			16, 17, 18,  16, 18, 19,
			22, 21, 20, 23, 22, 20
		};

		sg_buffer_desc indiceBuffDesc = {0};
		indiceBuffDesc.type = SG_BUFFERTYPE_INDEXBUFFER;
		indiceBuffDesc.size = sizeof(indices);
		indiceBuffDesc.content = indices;
		ibuf = sg_make_buffer(&indiceBuffDesc);

		shaderBaseShaded = sg_make_shader(&ShaderBaseShaded());

		// basic mesh pipeline
		sg_pipeline_desc pipeDesc = {0};
		pipeDesc.shader = shaderBaseShaded;
		pipeDesc.layout.buffers[0].stride = sizeof(Vertex);
		pipeDesc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
		pipeDesc.layout.attrs[1].format = SG_VERTEXFORMAT_UBYTE4N;
		pipeDesc.index_type = SG_INDEXTYPE_UINT16;
		pipeDesc.depth_stencil.depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL;
		pipeDesc.depth_stencil.depth_write_enabled = true;
		pipeDesc.rasterizer.cull_mode = SG_CULLMODE_BACK;
		pip = sg_make_pipeline(&pipeDesc);

		// line pipeline
		sg_pipeline_desc linePipeDesc = {0};
		linePipeDesc.shader = shaderBaseShaded;
		linePipeDesc.primitive_type = SG_PRIMITIVETYPE_LINES;
		linePipeDesc.layout.buffers[0].stride = sizeof(Vertex);
		linePipeDesc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
		linePipeDesc.layout.attrs[1].format = SG_VERTEXFORMAT_UBYTE4N;
		linePipeDesc.depth_stencil.depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL;
		linePipeDesc.depth_stencil.depth_write_enabled = true;
		pipeLine = sg_make_pipeline(&linePipeDesc);

		// push a simple grid
		const u32 lineColor = 0xFF7F7F7F;
		for(int i = -500; i < 500; i++) {
			lineBuffer.Push({vec3(10000, i * 5, 0), lineColor, vec3(-10000, i * 5, 0), lineColor});
			lineBuffer.Push({vec3(i * 5, 10000, 0), lineColor, vec3(i * 5, -10000, 0), lineColor});
		}
	}

	void Frame()
	{
		Time now = TimeNow();
		localTime = TimeDiff(startTime, now);

		// default pass action (clear to grey)
		sg_pass_action pass_action = {0};

		f32 a = fmod(TimeDiffSec(localTime)*0.2 * glm::pi<f32>() * 2.0f, 4.0f * glm::pi<f32>());
		const f32 viewDist = 5.0f;

		mat4 model = glm::rotate(glm::translate(glm::identity<mat4>(), vec3(0, 0, 0.5)), a, glm::vec3(0, 0, 1));
		mat4 proj = glm::perspective(glm::radians(60.0f), (float)winWidth/(float)winHeight, 0.01f, 500.0f);
		mat4 view = glm::lookAt(vec3(viewDist, 0, sinf(a) * 2 + 2), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f));
		mat4 mvp = proj * view * model;

		// draw cube
		sg_bindings binds = {0};
		binds.vertex_buffers[0] = vbuf;
		binds.index_buffer = ibuf;

		sg_begin_default_pass(&pass_action, winWidth, winHeight);
		sg_apply_pipeline(pip);
		sg_apply_bindings(&binds);
		sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &mvp, sizeof(mvp));
		sg_draw(0, 36, 1);


		// draw lines
		mvp = proj * view;

		sg_bindings bindsLine = {0};
		bindsLine.vertex_buffers[0] = lineBuffer.GetUpdatedBuffer();

		sg_apply_pipeline(pipeLine);
		sg_apply_bindings(&bindsLine);
		sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &mvp, sizeof(mvp));
		sg_draw(0, lineBuffer.GetLineCount() * 2, 1);

		sg_end_pass();
		sg_commit();
	}

	void OnEvent(const sapp_event& event)
	{
		// quit on escape
		if(event.key_code == sapp_keycode::SAPP_KEYCODE_ESCAPE) {
			sapp_request_quit();
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
	g_pWindow->Init();
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
