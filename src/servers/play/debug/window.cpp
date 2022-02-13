#include "window.h"

#include <config.h>
#include <common/vector_math.h>
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

#include "physics.h"
#include "collision_tests.h"

struct GameState
{
	eastl::fixed_vector<Dbg::Entity,2048,true> entityList;
	PhysWorld physics;

	void NewFrame()
	{
		entityList.clear();
	}

	void PushEntity(const Dbg::Entity& entity)
	{
		entityList.push_back(entity);
	}

	void PushPhysics(const PhysWorld& world)
	{
		physics = world;
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

	eastl::array<GameState, 2> gameStateList;
	GameState* gameStateFront = &gameStateList[0];
	GameState* gameStateBack = &gameStateList[1];
	ProfileMutex(Mutex, gameStateMutex);

	bool bGameStateRecording = false;
	struct Recording
	{
		ProfileMutex(Mutex, mutex);
		i32 filterStep = 1;
		i32 filterCapsuleID = 0;
		i32 selectedID = 0;
		eastl::fixed_vector<PhysWorld::CollisionEvent, 8192, true> events;
	};
	Recording gsRecording;

	CollisionTest collisionTest;

	ShapeMesh mapCollision;
	ShapeMesh mapWalls;

	PhysicsScene testScene;
	bool bFreezeTestPhysics = false;

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
			body->pos = vec3(5469, 3945, 1000);
			//body->pos = vec3(5820, 3795, 1000);
			facing = vec3(1, 0, 0);
			input = {0};
		}
	}
	testSubject;

	bool ui_bCollisionTests = false;
	bool ui_bMapWireframe = false;
	bool ui_bGameStates = true;
	bool ui_bPhysicsTest = true;

	Window(i32 width, i32 height):
		winWidth(width),
		winHeight(height),
		rdr{width, height},
		collisionTest(rdr)
	{

	}

	inline void Draw(const PhysWorld::BodyHandle& body, vec3 color)
	{
		vec3 pos = body->pos;
		ShapeCapsule shape;
		shape.radius = body->radius;
		shape.base = pos;
		shape.tip = pos + vec3(0, 0, body->height);

		collisionTest.Draw(shape, color);
	}

	bool Init();

	void WindowPhysicsWorld(PhysWorld& physicsTest, const char* name);
	void WindowGameStates();
	void WindowPhysicsTest();

	void NewFrame(Dbg::GameUID gameUID);
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
	MeshFile mfCylinder;
	r = OpenMeshFile("gamedata/PVP_DeathMatch01_Collision.msh", &mfCollision);
	if(!r) return false;
	r = OpenMeshFile("gamedata/PVP_DeathMatch01_Env.msh", &mfEnv);
	if(!r) return false;
	r = OpenMeshFile("gamedata/cylinder.msh", &mfCylinder);
	if(!r) return false;

	// drawable meshes
	const MeshFile::Mesh& mshCol = mfCollision.meshList.front();
	rdr.LoadMeshFile("PVP_DeathMatchCollision", mshCol);

	foreach_const(it, mfEnv.meshList) {
		rdr.LoadMeshFile(it->name.data(), *it);
	}

	rdr.LoadMeshFile("cylinder", mfCylinder.meshList.front());


	r = MakeMapCollisionMesh(mshCol, &mapCollision);
	if(!r) return false;
	r = MakeMapCollisionMesh(mfEnv.meshList.front(), &mapWalls);
	if(!r) return false;


	// create map scene, add ground and wall static meshes
	PhysContext().CreateScene(&testScene);

	PhysicsCollisionMesh pvpCollision1;
	PhysicsCollisionMesh pvpCollision2;

	const GameXmlContent& gc = GetGameXmlContent();
	r = PhysContext().LoadCollisionMesh(&pvpCollision1, gc.filePvpDeathmatch01Collision);
	if(!r) {
		LOG("ERROR: LoadCollisionMesh failed (pvpCollision1)");
		return false;
	}
	r = PhysContext().LoadCollisionMesh(&pvpCollision2, gc.filePvpDeathmatch01CollisionWalls);
	if(!r) {
		LOG("ERROR: LoadCollisionMesh failed (pvpCollision2)");
		return false;
	}

	testScene.AddStaticMesh(&pvpCollision1);
	testScene.AddStaticMesh(&pvpCollision2);

	return true;
}

void Window::WindowPhysicsWorld(PhysWorld& physics, const char* name)
{
	if(ImGui::Begin(name)) {
		const ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable;

		ImGui::BeginTable("postable", 7, flags);
		ImGui::TableSetupColumn("UID");
		ImGui::TableSetupColumn("PosX");
		ImGui::TableSetupColumn("PosY");
		ImGui::TableSetupColumn("PosZ");
		ImGui::TableSetupColumn("VelX");
		ImGui::TableSetupColumn("VelY");
		ImGui::TableSetupColumn("VelZ");
		ImGui::TableHeadersRow();

		int uid = 0;
		foreach_const(it, physics.dynBodyList) {
			vec3 pos = it->pos;
			vec3 vel = it->vel;
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("%d", uid);
			ImGui::TableNextColumn();
			ImGui::Text("%.2f", pos.x);
			ImGui::TableNextColumn();
			ImGui::Text("%.2f", pos.y);
			ImGui::TableNextColumn();
			ImGui::Text("%.2f", pos.z);
			ImGui::TableNextColumn();
			ImGui::Text("%.2f", vel.x);
			ImGui::TableNextColumn();
			ImGui::Text("%.2f", vel.y);
			ImGui::TableNextColumn();
			ImGui::Text("%.2f", vel.z);
			uid++;
		}

		ImGui::EndTable();

		foreach_const(b, physics.dynBodyList) {
			vec3 pos = b->pos;
			vec3 vel = b->vel;

			ShapeCylinder shape;
			shape.radius = b->radius;
			shape.height = b->height;
			shape.base = pos;

			vec3 color = vec3(1, 0, 1);
			if(b->flags & PhysWorld::Flags::Disabled) color = vec3(0.5);

			collisionTest.Draw(shape, color);
			collisionTest.DrawVec(vel, shape.base + vec3(0, 0, shape.radius), vec3(1, 0.5, 0.8));
		}
	}
	ImGui::End();
}

void Window::WindowGameStates()
{
	static GameState gameState;
	{
		const LockGuard lock(gameStateMutex);
		gameState = *gameStateBack;
	}

	WindowPhysicsWorld(gameState.physics, "GameState :: Physics");

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

			foreach_const(ent, gameState.entityList) {
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

		foreach_const(ent, gameState.entityList) {
			const Dbg::Entity& e = *ent;
			//rdr.PushCapsule(Pipeline::Shaded, e.pos, vec3(0), 45, 210, e.color);
			//rdr.PushMesh(Pipeline::Unlit, "Ring", e.pos, vec3(0), vec3(50), e.color);

			// this is updated when the player moves
			const vec3 upperStart = e.pos + vec3(0, 0, 200);
			const vec3 upperDir = glm::normalize(vec3(cosf(e.rot.upperYaw), sinf(e.rot.upperYaw), sinf(e.rot.upperPitch)));
			rdr.PushArrow(Pipeline::Unlit, upperStart, upperStart + upperDir * 150.f, vec3(1, 0.4, 0.1), 10);

			const vec3 bodyRotStart = e.pos + vec3(0, 0, 150);
			rdr.PushArrow(Pipeline::Unlit, bodyRotStart, bodyRotStart + vec3(cosf(e.rot.bodyYaw), sinf(e.rot.bodyYaw), 0) * 150.f, vec3(1, 0.4, 0.1), 10);
			const vec3 dirStart = e.pos + vec3(0, 0, 80);
			rdr.PushArrow(Pipeline::Unlit, dirStart, dirStart + glm::normalize(vec3(e.moveDir.x, e.moveDir.y, 0)) * 200.0f, vec3(0.2, 1, 0.2), 10);

			rdr.PushArrow(Pipeline::Unlit, e.moveDest + vec3(0, 0, 15), e.moveDest, ColorV3(0x3405b5), 5);
		}
	}
	ImGui::End();

	if(ImGui::Begin("Recorder")) {
		if(!bGameStateRecording) {
			if(ImGui::Button("Start Recording")) {
				bGameStateRecording = true;
				const LockGuard lock(gsRecording.mutex);
				gsRecording.events.clear();
			}
		}
		else {
			ImGui::TextColored({1, 0, 0, 1}, "Recording..."); ImGui::SameLine();
			if(ImGui::Button("Stop")) {
				bGameStateRecording = false;
			}
		}

		ImGui::InputInt("Filter capsule ID", &gsRecording.filterCapsuleID);

		i32 minStep = 1;
		i32 maxStep = 1;
		if(!gsRecording.events.empty()) {
			minStep = gsRecording.events.front().step;
			maxStep = gsRecording.events.back().step;
			gsRecording.filterStep = clamp(gsRecording.filterStep, minStep, maxStep);
		}
		ImGui::SliderInt("Filter step", &gsRecording.filterStep, minStep, maxStep);
		ImGui::InputInt("##Filter step", &gsRecording.filterStep);

		bool wasSelected = false;
		i32 lastFilteredID = -1;

		if(ImGui::BeginListBox("Events")) {
			for(int n = 0; n < gsRecording.events.size(); n++) {
				const PhysWorld::CollisionEvent& event = gsRecording.events[n];
				if(event.step != gsRecording.filterStep) continue;
				if(event.capsuleID != gsRecording.filterCapsuleID) continue;

				const bool isSelected = (gsRecording.selectedID == n);

				ImGui::PushID(FMT("recording_event_%d", n));
				if(ImGui::Selectable(FMT("ssi=%d cri=%d len=%g", event.ssi, event.cri, glm::length(event.fix2)), isSelected)) {
					gsRecording.selectedID = n;
				}
				ImGui::PopID();

				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if(isSelected) {
					ImGui::SetItemDefaultFocus();
				}

				wasSelected |= (gsRecording.selectedID == n);
				lastFilteredID = n;
			}
			ImGui::EndListBox();
		}

		// select id within filtered events
		if(!wasSelected && lastFilteredID != -1) {
			gsRecording.selectedID = lastFilteredID;
		}

		if(gsRecording.selectedID >= 0 && gsRecording.selectedID < gsRecording.events.size()) {
			const PhysWorld::CollisionEvent& event = gsRecording.events[gsRecording.selectedID];
			const ShapeTriangle& tri = event.triangle;

			collisionTest.Draw(event.cylinder, vec3(0, 0, 1));
			const vec3 vorg = event.cylinder.base + vec3(0, 0, event.cylinder.radius);

			if(/*gsRecording.bShowPen*/ true) {
				collisionTest.DrawVec(event.pen.slide * 10.0f, vorg, vec3(0.5, 1, 0.5));
			}

			if(/*gsRecording.bShowFixed*/ true) {
				ShapeCylinder fixed = event.cylinder;
				fixed.base += event.fix;
				collisionTest.Draw(fixed, vec3(1, 1, 0));
				fixed = event.cylinder;
				fixed.base += event.fix2;
				collisionTest.Draw(fixed, ColorV3(0xfc4137));

				collisionTest.DrawVec(event.fix2 * 20.f, vorg, vec3(1, 1, 0));
				collisionTest.DrawVec(event.fix * 20.f, vorg, vec3(1, 0, 0));
				collisionTest.DrawVec(event.vel, vorg, vec3(0.5, 0.5, 1));
				collisionTest.DrawVec(event.fixedVel, vorg, vec3(1.0, 0.5, 0.2));
				//vec3 rv = ProjectVec(event.vel, tri.Normal());
				//collisionTest.DrawVec(rv, event.capsule.base + vec3(0, 0, 50), vec3(1.0, 0, 0.5));
			}

			const vec3 color = vec3(1, 1, 1);
			rdr.PushLine(tri.p[0], tri.p[1], color);
			rdr.PushLine(tri.p[0], tri.p[2], color);
			rdr.PushLine(tri.p[1], tri.p[2], color);
			rdr.PushArrow(Pipeline::Unlit, tri.Center(), tri.Center() + tri.Normal() * 100.f, color, 5);


			ImGui::Text("Dot: %f", glm::dot(event.triangle.Normal(), event.cylinder.Normal()));
		}
	}
	ImGui::End();
}

void Window::WindowPhysicsTest()
{
	if(!bFreezeTestPhysics) {
		UpdatePhysics();
	}

	if(ImGui::Begin("Physics")) {
		ImGui::Checkbox("Freeze", &bFreezeTestPhysics);

		if(ImGui::Button("Step")) {
			UpdatePhysics();
		}
		ImGui::SameLine();
		if(ImGui::Button("Reset")) {
			testSubject.Reset();
		}

		/*
		ImGui::Checkbox("Subject", &physicsTest.bShowSubject); ImGui::SameLine();
		ImGui::Checkbox("Fixed", &physicsTest.bShowFixed); ImGui::SameLine();
		ImGui::Checkbox("Pen", &physicsTest.bShowPen);


		ImGui::Text("LastFrameEvents = %d", physicsTest.lastStepEvents.size());
		if(ImGui::BeginListBox("Events")) {
			for(int n = 0; n < physicsTest.lastStepEvents.size(); n++) {
				const PhysWorld::CollisionEvent& event = physicsTest.lastStepEvents[n];

				const bool isSelected = (physicsTest.iSelectedEvent == n);
				if(ImGui::Selectable(FMT("#%d capsule=%d ssi=%d cri=%d len=%g", n, event.capsuleID, event.ssi, event.cri, glm::length(event.fix2)), isSelected)) {
					physicsTest.iSelectedEvent = n;
				}

				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if(isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndListBox();
		}

		ImGui::BeginTable("testsubject_postable", 3);
		ImGui::TableSetupColumn("PosX");
		ImGui::TableSetupColumn("PosY");
		ImGui::TableSetupColumn("PosZ");
		ImGui::TableHeadersRow();

		vec3 pos = testSubject.body->pos;
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("%.2f", pos.x);
		ImGui::TableNextColumn();
		ImGui::Text("%.2f", pos.y);
		ImGui::TableNextColumn();
		ImGui::Text("%.2f", pos.z);

		ImGui::EndTable();
		*/
	}
	ImGui::End();

	/*
	if(physicsTest.bShowSubject) {
		Draw(testSubject.body, vec3(1, 0, 1));
	}

	if(physicsTest.iSelectedEvent >= 0 && physicsTest.iSelectedEvent < physicsTest.lastStepEvents.size()) {
		const PhysWorld::CollisionEvent& event = physicsTest.lastStepEvents[physicsTest.iSelectedEvent];
		const ShapeTriangle& tri = event.triangle;

		collisionTest.Draw(event.cylinder, vec3(0, 0, 1));


		const vec3 vorg = event.cylinder.base + vec3(0, 0, event.cylinder.radius);

		if(physicsTest.bShowPen) {
			collisionTest.DrawVec(event.pen.slide * 10.0f, vorg, vec3(0.5, 1, 0.5));
		}

		if(physicsTest.bShowFixed) {
			ShapeCylinder fixed = event.cylinder;
			fixed.base += event.fix2;
			collisionTest.Draw(fixed, vec3(1, 1, 0));

			collisionTest.DrawVec(event.fix2 * 20.f, vorg, vec3(1, 1, 0));
			collisionTest.DrawVec(event.fix * 20.f, vorg, vec3(1, 0, 0));
			collisionTest.DrawVec(event.vel, vorg, vec3(0.5, 0.5, 1));
			collisionTest.DrawVec(event.fixedVel, vorg, vec3(1.0, 0.5, 0.2));
			//vec3 rv = ProjectVec(event.vel, tri.Normal());
			//collisionTest.DrawVec(rv, event.capsule.base + vec3(0, 0, 50), vec3(1.0, 0, 0.5));
		}

		const vec3 color = vec3(1, 1, 1);
		rdr.PushLine(tri.p[0], tri.p[1], color);
		rdr.PushLine(tri.p[0], tri.p[2], color);
		rdr.PushLine(tri.p[1], tri.p[2], color);
		rdr.PushArrow(Pipeline::Unlit, tri.Center(), tri.Center() + tri.Normal() * 100.f, color, 5);
	}

	*/
}

// WARNING: Threaded call
void Window::NewFrame(Dbg::GameUID gameUID)
{
	{ LOCK_MUTEX(gameStateMutex);
		eastl::swap(gameStateFront, gameStateBack);
	}

	gameStateFront->NewFrame();

	if(bGameStateRecording) {
		static GameState gameState;
		{ LOCK_MUTEX(gameStateMutex);
			gameState = *gameStateBack;
		}

		LOCK_MUTEX(gsRecording.mutex);
		eastl::copy(gameState.physics.lastStepEvents.begin(), gameState.physics.lastStepEvents.end(), eastl::back_inserter(gsRecording.events));
	}
}

void Window::Update(f64 delta)
{
	//ImGui::ShowDemoWindow();

	if(ImGui::BeginMainMenuBar()) {
		if(ImGui::BeginMenu("View")) {
			ImGui::MenuItem("Collision tests", "", &ui_bCollisionTests);
			ImGui::MenuItem("Physics tests", "", &ui_bPhysicsTest);
			ImGui::MenuItem("Map wireframe", "", &ui_bMapWireframe);
			ImGui::MenuItem("Game states", "", &ui_bGameStates);
			ImGui::EndMenu();
		}

		if(ImGui::BeginMenu("Tweak")) {
			auto& g = GetGlobalTweakableVars();
			ImGui::SliderFloat("JumpForce", &g.jumpForce, 0, 50000);
			ImGui::SliderFloat("Gravity", &g.gravity, 0, 2000);
			ImGui::SliderFloat("Step height", &g.stepHeight, 10, 2000);
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
		WindowGameStates();
	}

	if(ui_bPhysicsTest) {
		WindowPhysicsTest();
	}
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

	// testSubject.body->vel = dir * speed;

	testScene.Step();
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

		if(!bFreezeTestPhysics) {
			switch(event.key_code) {
				case sapp_keycode::SAPP_KEYCODE_I: testSubject.input[TestSubject::Input::Forward] = 1; break;
				case sapp_keycode::SAPP_KEYCODE_K: testSubject.input[TestSubject::Input::Backward] = 1; break;
				case sapp_keycode::SAPP_KEYCODE_L: testSubject.input[TestSubject::Input::Right] = 1; break;
				case sapp_keycode::SAPP_KEYCODE_J: testSubject.input[TestSubject::Input::Left] = 1; break;
			}
		}
	}

	if(event.type == SAPP_EVENTTYPE_KEY_UP) {
		if(!bFreezeTestPhysics) {
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
	g_pWindow->NewFrame(gameUID);
}

void PushEntity(GameUID gameUID, const Entity& entity)
{
	g_pWindow->gameStateFront->PushEntity(entity);
}

void PopGame(GameUID gameUID)
{
	// does nothing for now
}

void PushPhysics(GameUID gameUID, const PhysWorld& world)
{
	g_pWindow->gameStateFront->PushPhysics(world);
}

}
