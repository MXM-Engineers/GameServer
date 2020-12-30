#ifdef CONF_WINDOWS
#include "window.h"

#define SOKOL_IMPL
#define SOKOL_NO_ENTRY
#define SOKOL_D3D11

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"

#include "config.h"
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Window
{
	const i32 winWidth;
	const i32 winHeight;
	sg_buffer vbuf;
	sg_buffer ibuf;
	sg_pipeline pip;
	Time startTime = Time::ZERO;
	Time localTime = Time::ZERO;

	void Init()
	{
		startTime = TimeNow();

		// setup sokol_gfx
		sg_desc gfxDesc = {0};
		gfxDesc.context = sapp_sgcontext();
		sg_setup(&gfxDesc);

		// a vertex buffer
		const float vertices[] = {
			// positions            // colors
			-1.0, -1.0, -1.0,  1.0, 0.0, 0.0, 1.0,
			1.0, -1.0, -1.0,   1.0, 0.0, 0.0, 1.0,
			1.0,  1.0, -1.0,   1.0, 0.0, 0.0, 1.0,
		   -1.0,  1.0, -1.0,   1.0, 0.0, 0.0, 1.0,

		   -1.0, -1.0,  1.0,   0.0, 1.0, 0.0, 1.0,
			1.0, -1.0,  1.0,   0.0, 1.0, 0.0, 1.0,
			1.0,  1.0,  1.0,   0.0, 1.0, 0.0, 1.0,
		   -1.0,  1.0,  1.0,   0.0, 1.0, 0.0, 1.0,

		   -1.0, -1.0, -1.0,   0.0, 0.0, 1.0, 1.0,
		   -1.0,  1.0, -1.0,   0.0, 0.0, 1.0, 1.0,
		   -1.0,  1.0,  1.0,   0.0, 0.0, 1.0, 1.0,
		   -1.0, -1.0,  1.0,   0.0, 0.0, 1.0, 1.0,

		   1.0, -1.0, -1.0,   1.0, 0.5, 0.0, 1.0,
		   1.0,  1.0, -1.0,   1.0, 0.5, 0.0, 1.0,
		   1.0,  1.0,  1.0,   1.0, 0.5, 0.0, 1.0,
		   1.0, -1.0,  1.0,   1.0, 0.5, 0.0, 1.0,

		   -1.0, -1.0, -1.0,   0.0, 0.5, 1.0, 1.0,
		   -1.0, -1.0,  1.0,   0.0, 0.5, 1.0, 1.0,
			1.0, -1.0,  1.0,   0.0, 0.5, 1.0, 1.0,
			1.0, -1.0, -1.0,   0.0, 0.5, 1.0, 1.0,

		   -1.0,  1.0, -1.0,   1.0, 0.0, 0.5, 1.0,
		   -1.0,  1.0,  1.0,   1.0, 0.0, 0.5, 1.0,
			1.0,  1.0,  1.0,   1.0, 0.0, 0.5, 1.0,
			1.0,  1.0, -1.0,   1.0, 0.0, 0.5, 1.0
		};

		sg_buffer_desc vertexBuffDesc = {0};
		vertexBuffDesc.size = sizeof(vertices);
		vertexBuffDesc.content = vertices;
		vbuf = sg_make_buffer(&vertexBuffDesc);

		/* cube indices */
		u16 indices[] = {
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

		// a shader
		sg_shader_desc shaderDesc = {0};
		shaderDesc.attrs[0].sem_name = "POSITION";
		shaderDesc.attrs[1].sem_name = "COLOR";
		shaderDesc.attrs[1].sem_index = 1;

		shaderDesc.vs.uniform_blocks[0].size = sizeof(glm::mat4);
		shaderDesc.vs.source = R"RAW(
				cbuffer params: register(b0) {
				   float4x4 mvp;
				};

				struct vs_in {
				   float4 pos: POSITION;
				   float4 color: COLOR1;
				};

				struct vs_out {
				   float4 color: COLOR0;
				   float4 pos: SV_Position;
				};

				vs_out main(vs_in inp) {
				   vs_out outp;
				   outp.pos = mul(mvp, inp.pos);
				   outp.color = inp.color;
				   return outp;
				};
			)RAW";
		shaderDesc.fs.source = R"RAW(
				float4 main(float4 color: COLOR0): SV_Target0
				{
					return color;
				}
			)RAW";

		sg_shader shd = sg_make_shader(&shaderDesc);

		// a pipeline state object (default render states are fine for triangle)
		sg_pipeline_desc pipeDesc = {0};
		pipeDesc.shader = shd;
		pipeDesc.layout.buffers[0].stride = 28;
		pipeDesc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
		pipeDesc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT4;
		pipeDesc.index_type = SG_INDEXTYPE_UINT16;
		pipeDesc.depth_stencil.depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL;
		pipeDesc.depth_stencil.depth_write_enabled = true;
		pipeDesc.rasterizer.cull_mode = SG_CULLMODE_BACK;
		pip = sg_make_pipeline(&pipeDesc);
	}

	void Frame()
	{
		Time now = TimeNow();
		localTime = TimeDiff(startTime, now);

		// resource bindings
		sg_bindings binds = {0};
		binds.vertex_buffers[0] = vbuf;
		binds.index_buffer = ibuf;

		// default pass action (clear to grey)
		sg_pass_action pass_action = {0};

		f32 a = TimeDiffSec(localTime)*0.2 * glm::pi<f32>() * 2.0f;
		const f32 viewDist = 5.0f;

		glm::mat4 proj = glm::perspective(glm::radians(60.0f), (float)winWidth/(float)winHeight, 0.01f, 50.0f);
		glm::mat4 view = glm::lookAt(glm::vec3(cosf(a) * viewDist, sinf(a) * viewDist, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		glm::mat4 view_proj = proj * view;

		sg_begin_default_pass(&pass_action, winWidth, winHeight);
		sg_apply_pipeline(pip);
		sg_apply_bindings(&binds);
		sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &view_proj, sizeof(view_proj));
		sg_draw(0, 36, 1);
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
