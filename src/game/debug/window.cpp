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
#include <sokol_app.h>
#include <sokol_gfx.h>
#include <sokol_glue.h>
#include <sokol_imgui.h>

#include <game/physics.h>
#include "collision_tests.h"

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

	CollisionTest collisionTest;

	ShapeMesh mapCollision;
	ShapeMesh mapWalls;

	PhysWorld physics;
	bool bFreezeStep = false;
	bool bShowSubject = true;
	i32 iSelectedEvent = 0;

	struct TestSubject
	{
		PhysWorld::BodyHandle body;
		vec3 facing;

		enum Input {
			Forward = 0,
			Backward,
			Right,
			Left,

			_Count
		};

		eastl::array<u8,Input::_Count> input = {0};

		void Reset() {
			//body->dyn.pos = vec3(5469, 3945, 6000);
			body->dyn.pos = vec3(5820, 3795, 1000);
			facing = vec3(1, 0, 0);
			input = {0};
		}
	}
	testSubject;

	bool ui_bCollisionTests = false;
	bool ui_bMapWireframe = false;
	bool ui_bGameStates = false;

	Window(i32 width, i32 height):
		winWidth(width),
		winHeight(height),
		rdr{width, height},
		collisionTest(rdr)
	{

	}

	inline void Draw(const PhysWorld::BodyHandle& body, vec3 color)
	{
		ShapeCapsule shape = body->shape;
		vec3 pos = body->dyn.pos;
		shape.base += pos;
		shape.tip += pos;

		collisionTest.Draw(shape, color);
	}

	bool Init();
	void Update(f64 delta);
	void UpdatePhysics();
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

	MeshFile mfCollision;
	MeshFile mfEnv;
	r = OpenMeshFile("gamedata/PVP_DeathMatch01_Collision.msh", &mfCollision);
	if(!r) return false;
	r = OpenMeshFile("gamedata/PVP_DeathMatch01_Env.msh", &mfEnv);
	if(!r) return false;

	const MeshFile::Mesh& mshCol = mfCollision.meshList.front();
	rdr.LoadMeshFile("PVP_DeathMatchCollision", mshCol);

	foreach_const(it, mfEnv.meshList) {
		rdr.LoadMeshFile(it->name.data(), *it);
	}

	r = MakeMapCollisionMesh(mshCol, &mapCollision);
	if(!r) return false;
	r = MakeMapCollisionMesh(mfEnv.meshList.front(), &mapWalls);
	if(!r) return false;

	physics.PushStaticMeshes(&mapCollision, 1);
	physics.PushStaticMeshes(&mapWalls, 1);

	testSubject.body = physics.CreateBody(45, 210, vec3(2800, 3532, 530));
	testSubject.Reset();
	return true;
}

void Window::Update(f64 delta)
{
	//ImGui::ShowDemoWindow();

	if(ImGui::BeginMainMenuBar()) {
		if(ImGui::BeginMenu("View")) {
			ImGui::MenuItem("Collision tests", "", &ui_bCollisionTests);
			ImGui::MenuItem("Map wireframe", "", &ui_bMapWireframe);
			ImGui::MenuItem("Game states", "", &ui_bGameStates);
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	const f64 t = TimeDiffSec(TimeDiff(startTime, localTime));

	// map
	rdr.PushMesh(Pipeline::Shaded, "PVP_DeathMatchCollision", vec3(0, 0, 0), vec3(0, 0, 0), vec3(1), vec3(0.2, 0.3, 0.3));
	rdr.PushMesh(Pipeline::Shaded, "PVP_Deathmatch01_GuardrailMob", vec3(0, 0, 0), vec3(0, 0, 0), vec3(1), vec3(0.2, 0.3, 0.5));

	if(ui_bMapWireframe) {
		foreach_const(it, mapCollision.triangleList) {
			const ShapeTriangle& tri = *it;
			const vec3 color = vec3(0.5, 1, 0.5);
			rdr.PushLine(tri.p[0], tri.p[1], color);
			rdr.PushLine(tri.p[0], tri.p[2], color);
			rdr.PushLine(tri.p[1], tri.p[2], color);
			rdr.PushArrow(Pipeline::Unlit, tri.Center(), tri.Center() + tri.Normal() * 100.f, color, 5);
		}
		foreach_const(it, mapWalls.triangleList) {
			const ShapeTriangle& tri = *it;
			const vec3 color = vec3(0.5, 0.5, 1.0);
			rdr.PushLine(tri.p[0], tri.p[1], color);
			rdr.PushLine(tri.p[0], tri.p[2], color);
			rdr.PushLine(tri.p[1], tri.p[2], color);
			rdr.PushArrow(Pipeline::Unlit, tri.Center(), tri.Center() + tri.Normal() * 100.f, color, 5);
		}
	}

	if(ui_bCollisionTests) {
		collisionTest.Render();
	}

	if(ui_bGameStates) {
		// @Speed: very innefficient locking
		// Also some blinking because sometimes the gamestate is empty when we render it (NewFrame before adding anything)
		decltype(GameState::entityList) entityList;
		{
			const LockGuard lock(game.mutex);
			entityList = game.entityList;
		}

		foreach_const(ent, entityList) {
			const Dbg::Entity& e = *ent;
			rdr.PushCapsule(Pipeline::Shaded, e.pos, vec3(0), 45, 210, e.color);
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
		}
		ImGui::End();
	}

	if(ImGui::Begin("Physics")) {
		ImGui::Checkbox("Freeze", &bFreezeStep);

		if(ImGui::Button("Step")) {
			UpdatePhysics();
		}
		ImGui::SameLine();
		if(ImGui::Button("Reset")) {
			testSubject.Reset();
		}

		ImGui::Checkbox("Show subject", &bShowSubject);

		#ifdef CONF_DEBUG
		ImGui::Text("LastFrameEvents = %d", physics.lastStepEvents.size());
		if(ImGui::BeginListBox("Events")) {
			for(int n = 0; n < physics.lastStepEvents.size(); n++) {
				const PhysWorld::CollisionEvent& event = physics.lastStepEvents[n];

				const bool isSelected = (iSelectedEvent == n);
				if(ImGui::Selectable(FMT("#%d capsule=%d ssi=%d cri=%d len=%g", n, event.capsuleID, event.ssi, event.cri, glm::length(event.disp)), isSelected)) {
					iSelectedEvent = n;
				}

				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if(isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndListBox();
		}
		#endif

		ImGui::BeginTable("testsubject_postable", 3);
		ImGui::TableSetupColumn("PosX");
		ImGui::TableSetupColumn("PosY");
		ImGui::TableSetupColumn("PosZ");
		ImGui::TableHeadersRow();

		vec3 pos = testSubject.body->dyn.pos;
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("%.2f", pos.x);
		ImGui::TableNextColumn();
		ImGui::Text("%.2f", pos.y);
		ImGui::TableNextColumn();
		ImGui::Text("%.2f", pos.z);

		ImGui::EndTable();


	}
	ImGui::End();

	if(!bFreezeStep) {
		UpdatePhysics();
	}

	if(bShowSubject) {
		Draw(testSubject.body, vec3(1, 0, 1));
	}

#ifdef CONF_DEBUG
	if(iSelectedEvent >= 0 && iSelectedEvent < physics.lastStepEvents.size()) {
		const PhysWorld::CollisionEvent& event = physics.lastStepEvents[iSelectedEvent];
		const ShapeTriangle& tri = event.triangle;

		collisionTest.Draw(event.capsule, vec3(0, 0, 1));
		ShapeCapsule fixed = event.capsule;
		fixed.base += event.disp;
		fixed.tip += event.disp;
		collisionTest.Draw(fixed, vec3(1, 1, 0));

		collisionTest.DrawVec(event.disp, event.capsule.base + vec3(0, 0, event.capsule.radius), vec3(1, 1, 0));
		collisionTest.DrawVec(event.vel, event.capsule.base + vec3(0, 0, event.capsule.radius), vec3(0.5, 0.5, 1));
		collisionTest.DrawVec(event.fixedVel, event.capsule.base + vec3(0, 0, event.capsule.radius), vec3(1.0, 0.5, 0.2));
		//vec3 rv = ProjectVec(event.vel, tri.Normal());
		//collisionTest.DrawVec(rv, event.capsule.base + vec3(0, 0, 50), vec3(1.0, 0, 0.5));


		const vec3 color = vec3(1, 1, 1);
		rdr.PushLine(tri.p[0], tri.p[1], color);
		rdr.PushLine(tri.p[0], tri.p[2], color);
		rdr.PushLine(tri.p[1], tri.p[2], color);
		rdr.PushArrow(Pipeline::Unlit, tri.Center(), tri.Center() + tri.Normal() * 100.f, color, 5);

	}
#endif
}

void Window::UpdatePhysics()
{
	f32 speed = 0;
	vec3 dir = vec3(0);
	const vec3 right = glm::cross(testSubject.facing, vec3(0, 0, 1));

	if(testSubject.input[TestSubject::Input::Forward]) {
		speed = 650;
		dir += testSubject.facing;
	}
	if(testSubject.input[TestSubject::Input::Backward]) {
		speed = 650;
		dir -= testSubject.facing;
	}
	if(testSubject.input[TestSubject::Input::Right]) {
		speed = 650;
		dir += right;
	}
	if(testSubject.input[TestSubject::Input::Left]) {
		speed = 650;
		dir += -right;
	}

	if(LengthSq(dir) > 0.001f) {
		dir = glm::normalize(dir);
	}

	testSubject.body->dyn.vel = dir * speed;

	physics.Step();
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
	if(event.type == SAPP_EVENTTYPE_KEY_DOWN) {
		// quit on escape
		if(event.key_code == sapp_keycode::SAPP_KEYCODE_ESCAPE) {
			sapp_request_quit();
			return;
		}

		if(!bFreezeStep) {
			switch(event.key_code) {
				case sapp_keycode::SAPP_KEYCODE_I: testSubject.input[TestSubject::Input::Forward] = 1; break;
				case sapp_keycode::SAPP_KEYCODE_K: testSubject.input[TestSubject::Input::Backward] = 1; break;
				case sapp_keycode::SAPP_KEYCODE_L: testSubject.input[TestSubject::Input::Right] = 1; break;
				case sapp_keycode::SAPP_KEYCODE_J: testSubject.input[TestSubject::Input::Left] = 1; break;
			}
		}
	}

	if(event.type == SAPP_EVENTTYPE_KEY_UP) {
		if(!bFreezeStep) {
			switch(event.key_code) {
				case sapp_keycode::SAPP_KEYCODE_I: testSubject.input[TestSubject::Input::Forward] = 0; break;
				case sapp_keycode::SAPP_KEYCODE_K: testSubject.input[TestSubject::Input::Backward] = 0; break;
				case sapp_keycode::SAPP_KEYCODE_L: testSubject.input[TestSubject::Input::Right] = 0; break;
				case sapp_keycode::SAPP_KEYCODE_J: testSubject.input[TestSubject::Input::Left] = 0; break;
			}
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
