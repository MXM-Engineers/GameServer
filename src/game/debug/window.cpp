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

	void DrawArrow(const vec3& start, const vec3& end, const vec3& color, f32 thickness);
	void DrawArrowUnlit(const vec3& start, const vec3& end, const vec3& color, f32 thickness);
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
	// rdr.PushMesh({ "Capsule", vec3(0, 0, 0), vec3(0), vec3(1), vec3(1, 0.5, 0) });
	// rdr.PushMesh({ "Ring", vec3(0, 0, 0), vec3(0), vec3(1), vec3(1, 0.5, 0) });
	// rdr.PushMesh("Cone", vec3(0, 0, 0), vec3(0), vec3(100), vec3(1, 0.5, 0));
	// DrawArrow(vec3(200, 0, 0), vec3(300, 400, 500), vec3(1), 20);

	// map
	rdr.PushMeshDs("PVP_DeathMatchCollision", vec3(0, 0, 0), vec3(0, 0, 0), vec3(1), vec3(0.2, 0.3, 0.3));

	// @Speed: very innefficient locking
	// Also some blinking because sometimes the gamestate is empty when we render it (NewFrame before adding anything)
	decltype(GameState::entityList) entityList;
	{
		const LockGuard lock(game.mutex);
		entityList = game.entityList;
	}

	foreach_const(ent, entityList) {
		const Dbg::Entity& e = *ent;
		rdr.PushMesh("Capsule", e.pos, vec3(0), vec3(250), e.color);
		rdr.PushMeshUnlit("Ring", e.pos, vec3(0), vec3(50), e.color);

		// this is updated when the player moves
		const vec3 eyeStart1 = e.pos + vec3(0, 0, 200);
		const vec3 eyeStart2 = e.pos + vec3(0, 0, 180);
		f32 eyeA1 = -e.eye.x - PI/2;
		f32 eyeA2 = -e.eye.z - PI/2;
		DrawArrowUnlit(eyeStart1, eyeStart1 +  vec3(cosf(eyeA1), sinf(eyeA1), 0) * 150.f, vec3(0.2, 1, 1), 8);
		DrawArrowUnlit(eyeStart2, eyeStart2 +  vec3(cosf(eyeA2), sinf(eyeA2), 0) * 150.f, vec3(0.2, 1, 1), 8);
		const vec3 dirStart = e.pos + vec3(0, 0, 80);
		DrawArrowUnlit(dirStart, dirStart + glm::normalize(e.dir) * 100.0f, vec3(0.2, 1, 0.2), 10);

		// this is updated when the player aims
		const vec3 upperRotStart = e.pos + vec3(0, 0, 200);
		DrawArrowUnlit(upperRotStart, upperRotStart + vec3(cosf(e.upperRotate), sinf(e.upperRotate), 0) * 200.f, vec3(1, 0.4, 0.1), 10);
		const vec3 bodyRotStart = e.pos + vec3(0, 0, 80);
		DrawArrowUnlit(bodyRotStart, bodyRotStart + vec3(cosf(e.bodyRotate), sinf(e.bodyRotate), 0) * 200.f, vec3(1, 0.4, 0.1), 10);
	}

	const ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable;


	if(ImGui::Begin("Entities")) {
		if(ImGui::BeginTable("EntityList", 5, flags))
		{
			ImGui::TableSetupColumn("UID");
			ImGui::TableSetupColumn("Name");
			ImGui::TableSetupColumn("PosX");
			ImGui::TableSetupColumn("PosY");
			ImGui::TableSetupColumn("PosZ");
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
	simgui_shutdown();
	sg_shutdown();
}

void Window::DrawArrow(const vec3& start, const vec3& end, const vec3& color, f32 thickness)
{
	vec3 dir = glm::normalize(end - start);

	f32 yaw = atan2(dir.y, dir.x);
	f32 pitch = asinf(dir.z);
	f32 len = glm::length(end - start);
	f32 coneSize = thickness * 1.2f;
	f32 coneHeight = coneSize * 2;
	f32 armLen = len - coneHeight;
	f32 sizeY = thickness;
	rdr.PushMesh("CubeCentered", start + dir * (armLen/2), vec3(yaw, pitch, 0), vec3(armLen, sizeY, sizeY), color);
	//rdr.PushMesh("Cube", end + vec3(0, 0, 0), vec3(0), vec3(coneSize), color);
	rdr.PushMesh("Cone", end - dir * coneHeight, vec3(yaw, pitch - PI/2, 0), vec3(coneSize, coneSize, coneHeight), color);
}

void Window::DrawArrowUnlit(const vec3& start, const vec3& end, const vec3& color, f32 thickness)
{
	// code duplication
	vec3 dir = glm::normalize(end - start);

	f32 yaw = atan2(dir.y, dir.x);
	f32 pitch = asinf(dir.z);
	f32 len = glm::length(end - start);
	f32 coneSize = thickness * 1.2f;
	f32 coneHeight = coneSize * 2;
	f32 armLen = len - coneHeight;
	f32 sizeY = thickness;

	rdr.PushMeshUnlit("CubeCentered", start + dir * (armLen/2), vec3(yaw, pitch, 0), vec3(armLen, sizeY, sizeY), color);
	rdr.PushMeshUnlit("Cone", end - dir * coneHeight, vec3(yaw, pitch - PI/2, 0), vec3(coneSize, coneSize, coneHeight), color);
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
