#include <common/platform.h>
#include <mxm/game_content.h>
#include "debug/window.h"
#include "coordinator.h"
#include "config.h"

// NOTE: SN_GamePlayerEquipWeapon is needed for the player to rotate with the mouse

Server* g_Server = nullptr;
Listener* g_Listener = nullptr;

int main(int argc, char** argv)
{
	PlatformInit();
	LogInit("game_server.log");
	LogNetTrafficInit("game_server_nt.log", 0x0);
	TimeInit();

	// TODO: linux thread affinity (sched_setaffinity)
	EA::Thread::SetThreadAffinityMask((EA::Thread::ThreadAffinityMask)1 << (i32)CoreAffinity::MAIN);

	LoadConfig();

	LOG(".: Game server :.");

	Config().Print();

	bool r = SetCloseSignalHandler([](){
		g_Server->running = false;
		g_Listener->Stop();
#ifdef CONF_WINDOWS
		WindowRequestClose();
#endif
	});

	if(!r) {
		LOG("ERROR: Could not set control handler");
		return 1;
	}

	r = GameXmlContentLoad();
	if(!r) {
		LOG("ERROR: failed to load game xml content");
		return 1;
	}

	r = PhysicsInit();
	if(!r) {
		LOG("ERROR: failed to init physics");
		return 1;
	}
	defer(PhysContext().Shutdown());

	static Server server;
	r = server.Init();
	if(!r) {
		LOG("ERROR: failed to initialize server");
		return 1;
	}
	g_Server = &server;
	server.doTraceNetwork = Config().TraceNetwork;

	Listener listen(&server);
	g_Listener = &listen;

	r = listen.Init(Config().ListenPort);
	if(!r) {
		LOG("ERROR: Could not init game listener");
		return 1;
	}

#ifdef CONF_WINDOWS
	WindowCreate();
#endif

	static Coordinator coordinator;
	r = coordinator.Init(&server);
	if(!r) {
		LOG("ERROR: Could not init coordinator");
		return 1;
	}

	// listen on main thread
	listen.Listen();

	LOG("Cleaning up...");

#ifdef CONF_WINDOWS
	WindowWaitForCleanup();
#endif

	coordinator.Cleanup();
	server.Cleanup();

	SaveConfig();
	LOG("Done.");
	return 0;
}
