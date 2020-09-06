#include "game.h"
#include "coordinator.h"
#include "game_content.h"
#include "config.h"

Server* g_Server = nullptr;

//#error
// TODO:

// NOTE: SN_GamePlayerEquipWeapon is needed for the player to rotate with the mouse

int main(int argc, char** argv)
{
	LogInit("game_server.log");
	TimeInit();

	LoadConfig();
	Config().Print();

	LOG(".: Game server :.");

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
