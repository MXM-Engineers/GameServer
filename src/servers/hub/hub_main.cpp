#include <common/platform.h>
#include <mxm/game_content.h>

#include "coordinator.h"
#include "config.h"

Server* g_Server = nullptr;
Listener* g_Listener = nullptr;

int main(int argc, char** argv)
{
	PlatformInit();
	LogInit("hub_server.log");
	LogNetTrafficInit("hub_server_nt.log"/*, 0x0*/);
	TimeInit();

	// TODO: linux thread affinity (sched_setaffinity)
	EA::Thread::SetThreadAffinityMask((EA::Thread::ThreadAffinityMask)1 << (i32)CoreAffinity::MAIN);

	LoadConfig();

	LOG(".: Hub server :.");

	Config().Print();

	bool r = SetCloseSignalHandler([](){
		g_Server->running = false;
		g_Listener->Stop();
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

	static Server server;
	r = server.Init();
	if(!r) {
		LOG("ERROR: failed to initialize server");
		return 1;
	}
	g_Server = &server;
	server.doTraceNetwork = Config().TraceNetwork;

	Listener listenLobby(&server);
	g_Listener = &listenLobby;

	r = listenLobby.Init(Config().ListenPort);
	if(!r) {
		LOG("ERROR: Could not init lobby listener");
		return 1;
	}

	static Coordinator coordinator;
	r = coordinator.Init(&server);
	if(!r) {
		LOG("ERROR: Could not init coordinator");
		return 1;
	}

	// listen on main thread
	listenLobby.Listen();

	LOG("Cleaning up...");

	coordinator.Cleanup();
	server.Cleanup();

	SaveConfig();
	LOG("Done.");
	return 0;
}
