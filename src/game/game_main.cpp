#include "coordinator.h"
#include "game_content.h"
#include "config.h"
#include <game/debug/window.h>
#include <common/platform.h>

//#error
// TODO:
// - Fix how action states are checked for change. Currently if not invalid we send, then clear to invalid.
// However some action states seem to be more permanent, like NPC ones (99?). We could add a frameChangedID to designate the frame the state changed, and compare based on it.
// Also from a gameplay perspective, we want to know how long we jump for example. Simulate action states?

// - split different kind of actors in Replication

// NOTE: SN_GamePlayerEquipWeapon is needed for the player to rotate with the mouse

struct Listener
{
	Server& server;
	const ListenerType type;

	SOCKET listenSocket;
	i32 listenPort;

	Listener(ListenerType type_, Server* server_): type(type_), server(*server_) {}

	bool Init(i32 listenPort_)
	{
		listenPort = listenPort_;
		struct addrinfo *result = NULL, *ptr = NULL, hints;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;

		// Resolve the local address and port to be used by the server
		int iResult = getaddrinfo(NULL, FMT("%d", listenPort), &hints, &result);
		if (iResult != 0) {
			LOG("ERROR: getaddrinfo failed: %d", iResult);
			return false;
		}
		defer(freeaddrinfo(result));

		listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if(listenSocket == INVALID_SOCKET) {
			LOG("ERROR(socket): %d", NetworkGetLastError());
			return false;
		}

		// Setup the TCP listening socket
		iResult = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
		if(iResult == SOCKET_ERROR) {
			LOG("ERROR(bind): failed with error: %d", NetworkGetLastError());
			return false;
		}

		// listen
		if(listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
			LOG("ERROR(listen): failed with error: %d", NetworkGetLastError());
			return false;
		}
		return true;
	}

	void Stop()
	{
		closesocket(listenSocket);
		listenSocket = INVALID_SOCKET;
	}

	inline bool IsRunning() const { return listenSocket != INVALID_SOCKET; }

	void Listen()
	{
		while(IsRunning()) {
			// Accept a client socket
			LOG("[%d] Waiting for a connection...", listenPort);
			struct sockaddr clientAddr;
			AddrLen addrLen = sizeof(sockaddr);
			SOCKET clientSocket = accept(listenSocket, &clientAddr, &addrLen);
			if(clientSocket == INVALID_SOCKET) {
				if(IsRunning()) {
					LOG("[%d] ERROR(accept): failed: %d", listenPort, NetworkGetLastError());
					return;
				}
				else {
					break;
				}
			}

			LOG("[%d] New connection (%s)", listenPort, GetIpString(clientAddr));
			server.ListenerAddClient(clientSocket, clientAddr, type);
		}
	}
};

Server* g_Server = nullptr;
Listener* g_ListenerLobby = nullptr;
Listener* g_ListenerGame = nullptr;

intptr_t ThreadGameServerListen(void* pData)
{
	ProfileSetThreadName("GameServerListen");
	const i32 cpuID = 0;
	EA::Thread::SetThreadAffinityMask(1 << cpuID);

	Listener& listener = *(Listener*)pData;
	listener.Listen(); // blocking
	return 0;
}

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
		g_ListenerLobby->Stop();
		g_ListenerGame->Stop();
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

	static Server server;
	r = server.Init();
	if(!r) {
		LOG("ERROR: failed to initialize server");
		return 1;
	}
	g_Server = &server;
	server.doTraceNetwork = Config().TraceNetwork;

	Listener listenLobby(ListenerType::LOBBY, &server);
	static Listener listenGame(ListenerType::GAME, &server);
	g_ListenerLobby = &listenLobby;
	g_ListenerGame = &listenGame;

	r = listenLobby.Init(Config().ListenLobbyPort);
	if(!r) {
		LOG("ERROR: Could not init lobby listener");
		return 1;
	}
	r = listenGame.Init(Config().ListenGamePort);
	if(!r) {
		LOG("ERROR: Could not init game listener");
		return 1;
	}

#ifdef CONF_WINDOWS
	WindowCreate();
#endif

	static Coordinator coordinator;
	coordinator.Init(&server);

	// listen on another thread
	EA::Thread::Thread threadListenGame;
	threadListenGame.Begin(ThreadGameServerListen, &listenGame);

	// listen on main thread
	listenLobby.Listen();

	LOG("Cleaning up...");

#ifdef CONF_WINDOWS
	WindowWaitForCleanup();
#endif

	threadListenGame.WaitForEnd(); // wait for game listen thread to close
	coordinator.Cleanup();
	server.Cleanup();

	SaveConfig();
	LOG("Done.");
	return 0;
}
