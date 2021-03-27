#ifdef CONF_WINDOWS
#include "window.h"

#include <common/vector_math.h>
#include <game/config.h>
#include <eathread/eathread_thread.h>
#include <imgui.h>


#define SOKOL_NO_ENTRY
#define SOKOL_D3D11

#define SG_DEFAULT_CLEAR_RED 0.2f
#define SG_DEFAULT_CLEAR_GREEN 0.2f
#define SG_DEFAULT_CLEAR_BLUE 0.2f

#define SOKOL_ASSERT ASSERT
#define SOKOL_LOG LOG

#include "renderer.h"

#define SOKOL_IMPL
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_imgui.h"

#include <game/physics.h>

struct GameState
{
	eastl::fixed_vector<Dbg::Entity,2048,true> entityList;
	Mutex mutex;

	void NewFrame()
	{
		const LockGuard lock(mutex);
		entityList.clear();
	}

	void PushEntity(const Dbg::Entity& entity)
	{
		const LockGuard lock(mutex);
		entityList.push_back(entity);
	}
};

struct CollisionTest
{
	PhysSphere sphere;
	PhysCapsule capsule;
	PhysTriangle triangle;

	CollisionTest()
	{
		sphere.pos = vec3(1100, 150, 0);
		sphere.radius = 50;

		capsule.pos = vec3(100, 150, 0);
		capsule.height = 100;
		capsule.radius = 20;

		triangle.points = {
			vec3(600, 100, 0),
			vec3(600, 200, 0),
			vec3(600, 150, 100),
		};
	}

	void Render(Renderer& rdr)
	{
		rdr.PushMesh(Pipeline::Unlit, "Sphere", sphere.pos, vec3(0), vec3(sphere.radius), vec3(0, 0.5, 0.8));
		rdr.PushCapsule(Pipeline::Unlit, capsule.pos, vec3(0), capsule.radius, capsule.height, vec3(0, 0.5, 0.8));

		rdr.triangleBuffer.Push({
			TrianglePoint{ triangle.points[0], CU3(0, 0.5, 0) },
			TrianglePoint{ triangle.points[1], CU3(0, 0.5, 0) },
			TrianglePoint{ triangle.points[2], CU3(0, 0.5, 0) }
		});
	}
};

struct Window
{
	const i32 winWidth;
	const i32 winHeight;
	EA::Thread::Thread thread;

	Time startTime = Time::ZERO;
	Time localTime = Time::ZERO;

	Renderer rdr;

	GameState game;
	GameState lastGame;

	CollisionTest collisionTest;

	Window(i32 width, i32 height):
		winWidth(width),
		winHeight(height),
		rdr{width, height}
	{

	}

	bool Init();
	void Update(f64 delta);
	void Frame();
	void OnEvent(const sapp_event& event);
	void Cleanup();
};

bool Window::Init()
{
	startTime = TimeNow();

	bool r = rdr.Init();
	if(!r) {
		return false;
	}

	simgui_desc_t imguiDesc = {0};
	imguiDesc.ini_filename = "gameserver_imgui.ini";
	simgui_setup(&imguiDesc);
	return true;
}

void Window::Update(f64 delta)
{
	ImGui::ShowDemoWindow();

	// test meshes
	// rdr.PushMesh(Pipeline::Shaded, { "Capsule", vec3(0, 0, 0), vec3(0), vec3(1), vec3(1, 0.5, 0) });
	// rdr.PushMesh(Pipeline::Shaded, { "Ring", vec3(0, 0, 0), vec3(0), vec3(1), vec3(1, 0.5, 0) });
	// rdr.PushMesh(Pipeline::Shaded, "Cone", vec3(0, 0, 0), vec3(0), vec3(100), vec3(1, 0.5, 0));
	// DrawArrow(vec3(200, 0, 0), vec3(300, 400, 500), vec3(1), 20);
	// rdr.PushMesh(Pipeline::Shaded, "Cylinder", vec3(0, 0, 0), vec3(0), vec3(100), vec3(1, 0.5, 0));
	// rdr.PushMesh(Pipeline::Shaded, "Sphere", vec3(0, 0, 0), vec3(0), vec3(100), vec3(1, 0.5, 0));
	// rdr.PushCapsule(Pipeline::Unlit, vec3(100, 100, 0), vec3(0), 50, 250, vec3(0.8, 0.2, 0.35));

	// map
	rdr.PushMesh(Pipeline::ShadedDoubleSided, "PVP_DeathMatchCollision", vec3(0, 0, 0), vec3(0, 0, 0), vec3(1), vec3(0.2, 0.3, 0.3));

	// @Speed: very innefficient locking
	// Also some blinking because sometimes the gamestate is empty when we render it (NewFrame before adding anything)
	decltype(GameState::entityList) entityList;
	{
		const LockGuard lock(game.mutex);
		entityList = game.entityList;
	}

	foreach_const(ent, entityList) {
		const Dbg::Entity& e = *ent;
		rdr.PushCapsule(Pipeline::Shaded, e.pos, vec3(0), 50, 250, e.color);
		rdr.PushMesh(Pipeline::Unlit, "Ring", e.pos, vec3(0), vec3(50), e.color);

		// this is updated when the player moves
		const vec3 upperStart = e.pos + vec3(0, 0, 200);
		const vec3 upperDir = glm::normalize(vec3(cosf(e.rot.upperYaw), sinf(e.rot.upperYaw), sinf(e.rot.upperPitch)));
		rdr.PushArrow(Pipeline::Unlit, upperStart, upperStart + upperDir * 150.f, vec3(1, 0.4, 0.1), 10);

		const vec3 bodyRotStart = e.pos + vec3(0, 0, 150);
		rdr.PushArrow(Pipeline::Unlit, bodyRotStart, bodyRotStart + vec3(cosf(e.rot.bodyYaw), sinf(e.rot.bodyYaw), 0) * 150.f, vec3(1, 0.4, 0.1), 10);
		const vec3 dirStart = e.pos + vec3(0, 0, 80);
		rdr.PushArrow(Pipeline::Unlit, dirStart, dirStart + glm::normalize(vec3(e.moveDir.x, e.moveDir.y, 0)) * 200.0f, vec3(0.2, 1, 0.2), 10);
	}

	collisionTest.Render(rdr);

	const ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable;


	if(ImGui::Begin("Entities")) {
		if(ImGui::BeginTable("EntityList", 8, flags))
		{
			ImGui::TableSetupColumn("UID");
			ImGui::TableSetupColumn("Name");
			ImGui::TableSetupColumn("PosX");
			ImGui::TableSetupColumn("PosY");
			ImGui::TableSetupColumn("PosZ");
			ImGui::TableSetupColumn("RotUpperYaw");
			ImGui::TableSetupColumn("RotUpperPitch");
			ImGui::TableSetupColumn("RotBodyYaw");
			ImGui::TableHeadersRow();

			foreach_const(ent, entityList) {
				ImGui::TableNextRow();

				ImGui::TableNextColumn();
				ImGui::Text("%u", ent->UID);
				ImGui::TableNextColumn();
				ImGui::Text("%ls", ent->name.data());

				ImGui::TableNextColumn();
				ImGui::Text("%.2f", ent->pos.x);
				ImGui::TableNextColumn();
				ImGui::Text("%.2f", ent->pos.y);
				ImGui::TableNextColumn();
				ImGui::Text("%.2f", ent->pos.z);

				ImGui::TableNextColumn();
				ImGui::Text("%.2f", ent->rot.upperYaw);
				ImGui::TableNextColumn();
				ImGui::Text("%.2f", ent->rot.upperPitch);
				ImGui::TableNextColumn();
				ImGui::Text("%.2f", ent->rot.bodyYaw);
			}

			ImGui::EndTable();
		}

		ImGui::End();
	}
}

void Window::Frame()
{
	Time lastLocalTime = localTime;
	Time now = TimeNow();
	localTime = TimeDiff(startTime, now);
	const f64 delta = TimeDiffSec(TimeDiff(lastLocalTime, localTime));
	lastLocalTime = localTime;

	simgui_new_frame(winWidth, winHeight, delta);

	Update(delta);
	rdr.Render(delta);
}

void Window::OnEvent(const sapp_event& event)
{
	// quit on escape
	if(event.type == SAPP_EVENTTYPE_KEY_DOWN) {
		if(event.key_code == sapp_keycode::SAPP_KEYCODE_ESCAPE) {
			sapp_request_quit();
			return;
		}
	}

	if(!rdr.camera.mouseLocked) {
		simgui_handle_event(&event);
	}

	rdr.OnEvent(event);
}

void Window::Cleanup()
{
	rdr.Cleanup();
	simgui_shutdown();
	sg_shutdown();
}

static Window* g_pWindow = nullptr;

void WindowInit()
{
	bool r = g_pWindow->Init();
	if(!r) {
		LOG("ERROR: failed to init window");
		sapp_quit();
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

	sapp_desc desc;
	memset(&desc, 0, sizeof(desc));
	desc.window_title = "MxM server";
	// MSAA
	desc.sample_count = 8;
	desc.width = g_pWindow->winWidth;
	desc.height = g_pWindow->winHeight;
	desc.init_cb = WindowInit;
	desc.frame_cb = WindowFrame;
	desc.event_cb = WindowEvent;
	desc.cleanup_cb = WindowCleanup;
	sapp_run(&desc);

	LOG("Window closed.");
	GenerateConsoleCtrlEvent(0, 0); // so the whole server shuts down when closing the window
	return 0;
}

void WindowCreate()
{
	static Window win = {Config().WindowWidth, Config().WindowHeight};
	g_pWindow = &win;

	win.thread.Begin(ThreadWindow, nullptr);
}

void WindowRequestClose()
{
	sapp_request_quit();
}

void WindowWaitForCleanup()
{
	g_pWindow->thread.WaitForEnd();
}

namespace Dbg {

GameUID PushNewGame(const FixedStr32& mapName)
{
	// ignore name for now, assume PVP map
	return GameUID::INVALID;
}

void PushNewFrame(GameUID gameUID)
{
	g_pWindow->game.NewFrame();
}

void PushEntity(GameUID gameUID, const Entity& entity)
{
	g_pWindow->game.PushEntity(entity);
}

void PopGame(GameUID gameUID)
{
	// does nothing for now
}

}

#endif
