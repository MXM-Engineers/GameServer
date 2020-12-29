#ifdef CONF_WINDOWS
#include "window.h"

#define SOKOL_IMPL
#define SOKOL_NO_ENTRY
#define SOKOL_GLCORE33

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"

struct Window
{
	sg_buffer vbuf;
	sg_pipeline pip;
};

static Window win;

void WindowInit()
{
	// setup sokol_gfx
	sg_desc gfxDesc = {0};
	gfxDesc.context = sapp_sgcontext();
	sg_setup(&gfxDesc);

	// a vertex buffer
	const float vertices[] = {
		// positions            // colors
		 0.0f,  0.5f, 0.5f,     1.0f, 0.0f, 0.0f, 1.0f,
		 0.5f, -0.5f, 0.5f,     0.0f, 1.0f, 0.0f, 1.0f,
		-0.5f, -0.5f, 0.5f,     0.0f, 0.0f, 1.0f, 1.0f
	};

	sg_buffer_desc buffDesc = {0};
	buffDesc.size = sizeof(vertices);
	buffDesc.content = vertices;
	win.vbuf = sg_make_buffer(&buffDesc);

	// a shader
	sg_shader_desc shaderDesc = {0};
	shaderDesc.vs.source =
			"#version 330\n"
			"layout(location=0) in vec4 position;\n"
			"layout(location=1) in vec4 color0;\n"
			"out vec4 color;\n"
			"void main() {\n"
			"  gl_Position = position;\n"
			"  color = color0;\n"
			"}\n";
	shaderDesc.fs.source =
			"#version 330\n"
			"in vec4 color;\n"
			"out vec4 frag_color;\n"
			"void main() {\n"
			"  frag_color = color;\n"
			"}\n";

	sg_shader shd = sg_make_shader(&shaderDesc);

	// a pipeline state object (default render states are fine for triangle)
	sg_pipeline_desc pipeDesc = {0};
	pipeDesc.shader = shd;
	pipeDesc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
	pipeDesc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT4;
	win.pip = sg_make_pipeline(&pipeDesc);
}

void WindowFrame()
{
	// resource bindings
	sg_bindings binds = {0};
	binds.vertex_buffers[0] = win.vbuf;

	// default pass action (clear to grey)
	sg_pass_action pass_action = {0};

	sg_begin_default_pass(&pass_action, 1280, 720);
	sg_apply_pipeline(win.pip);
	sg_apply_bindings(&binds);
	sg_draw(0, 3, 1);
	sg_end_pass();
	sg_commit();
}

void WindowCleanup()
{
	sg_shutdown();
}

intptr_t ThreadWindow(void* pData)
{
	ProfileSetThreadName("Window");
	const i32 cpuID = 0;
	EA::Thread::SetThreadAffinityMask(1 << cpuID);

	sapp_desc desc;
	memset(&desc, 0, sizeof(desc));
	desc.width = 1280;
	desc.height = 720;
	desc.init_cb = WindowInit;
	desc.frame_cb = WindowFrame;
	desc.cleanup_cb = WindowCleanup;
	sapp_run(&desc);

	LOG("Window closed.");
	return 0;
}

#endif
