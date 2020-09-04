#include "game.h"
#include "coordinator.h"
#include "game_content.h"
#include "config.h"

#define LISTEN_PORT "11900"
Server* g_Server = nullptr;

//#error
// TODO:
// - Replicate movement
// - Manage client local actorUID (map)
// - Reserve a local actorUID for each ProfileCharacter and wpan it on that UID when switching to it (leader)

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

	coordinator.Cleanup();

	LOG("Done.");
	return 0;
}
