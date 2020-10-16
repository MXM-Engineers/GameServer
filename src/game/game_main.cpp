#include "game.h"
#include "coordinator.h"
#include "game_content.h"
#include "config.h"
#include <common/platform.h>

Server* g_Server = nullptr;

//#error
// TODO:
// - Fix how action states are checked for change. Currently if not invalid we send, then clear to invalid.
// However some action states seem to be more permanent, like NPC ones (99?). We could add a frameChangedID to designate the frame the state changed, and compare based on it.
// Also from a gameplay perspective, we want to know how long we jump for example. Simulate action states?

// - split different kind of actors in Replication

// NOTE: SN_GamePlayerEquipWeapon is needed for the player to rotate with the mouse

int main(int argc, char** argv)
{
	PlatformInit();
	LogInit("game_server.log");
	TimeInit();

	// TODO: linux thread affinity (sched_setaffinity)
	EA::Thread::SetThreadAffinityMask((EA::Thread::ThreadAffinityMask)1 << (i32)CoreAffinity::MAIN);

	LoadConfig();

	LOG(".: Game server :.");

	Config().Print();

	bool r = SetCloseSignalHandler([](){
		g_Server->running = false;
		closesocket(g_Server->serverSocket);
		g_Server->serverSocket = INVALID_SOCKET;
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
	r = server.Init(FMT("%d", Config().listenPort));
	if(!r) {
		LOG("ERROR: failed to initialize server");
		return 1;
	}
	g_Server = &server;
	server.doTraceNetwork = Config().traceNetwork;

	static Coordinator coordinator;
	coordinator.Init(&server);

	// accept thread
	while(server.running) {
		// Accept a client socket
		LOG("Waiting for a connection...");
		struct sockaddr clientAddr;
		AddrLen addrLen = sizeof(sockaddr);
		SOCKET clientSocket = accept(server.serverSocket, &clientAddr, &addrLen);
		if(clientSocket == INVALID_SOCKET) {
			if(server.running) {
				LOG("ERROR(accept): failed: %d", NetworkGetLastError());
				return 1;
			}
			else {
				break;
			}
		}

		LOG("New connection (%s)", GetIpString(clientAddr));
		server.AddClient(clientSocket, clientAddr);
	}

	LOG("Cleaning up...");

	coordinator.Cleanup();
	server.Cleanup();

	LOG("Done.");
	return 0;
}
