#include "coordinator.h"

#define LISTEN_PORT "11900"
Server* g_Server = nullptr;

//#error
// TODO:
// - replication: send player info (name, status etc)

// NOTE: SN_GamePlayerEquipWeapon is needed for the player to rotate with the mouse

BOOL WINAPI ConsoleHandler(DWORD signal)
{
	if(signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT) {
		LOG(">> Exit signal");
		g_Server->running = false;
	}

	return TRUE;
}

int main(int argc, char** argv)
{
	LogInit("game_server.log");
	LOG(".: Game server :.");

	if(!SetConsoleCtrlHandler(ConsoleHandler, TRUE)) {
		LOG("ERROR: Could not set control handler");
		return 1;
	}

	static Server server;
	bool r = server.Init(LISTEN_PORT);
	if(!r) {
		LOG("ERROR: failed to initialize server");
		return 1;
	}
	g_Server = &server;

	// TODO: the coordinator should create games
	static Game game;
	game.Init(&server);

	static Coordinator coordinator;
	coordinator.Init(&server, &game);

	// accept thread
	while(server.running) {
		// Accept a client socket
		LOG("Waiting for a connection...");
		struct sockaddr clientAddr;
		int addrLen = sizeof(sockaddr);
		SOCKET clientSocket = accept(server.serverSocket, &clientAddr, &addrLen);
		if(clientSocket == INVALID_SOCKET) {
			if(server.running) {
				LOG("ERROR(accept): failed: %d", WSAGetLastError());
				return 1;
			}
			else {
				break;
			}
		}

		LOG("New connection (%s)", GetIpString(clientAddr));
		server.AddClient(clientSocket, clientAddr);
	}

	LOG("Done.");
	return 0;
}
