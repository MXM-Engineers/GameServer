#include "game.h"

#define LISTEN_PORT "11900"
Server* g_Server = nullptr;

//#error
// TODO:
// - replication: send player info (name, status etc)

// NOTE: SN_GamePlayerEquipWeapon is needed for the player to rotate with the mouse

DWORD ThreadCoordinator(void* pData);

// Responsible for managing Account data and dispatching client to game channels/instances
struct Coordinator
{
	Server* server;
	Game* game;
	AccountData accountData[Server::MAX_CLIENTS];
	GrowableBuffer recvDataBuff;

	void Init(Server* server_, Game* game_)
	{
		server = server_;
		game = game_;
		recvDataBuff.Init(10 * (1024*1024)); // 10 MB

		CreateThread(NULL, 0, ThreadCoordinator, this, 0, NULL);
	}

	void ClientHandlePacket(i32 clientID, const NetHeader& header, const u8* packetData)
	{
		DBG_ASSERT(clientID >= 0 && clientID < Server::MAX_CLIENTS);
		const i32 packetSize = header.size - sizeof(NetHeader);

		switch(header.netID) {
			case Cl::CQ_FirstHello::NET_ID: {
				LOG("[client%03d] Client :: CQ_FirstHello", clientID);

				// TODO: verify version, protocol, etc
				const Server::ClientInfo& info = server->clientInfo[clientID];

				Sv::SA_FirstHello hello;
				hello.dwProtocolCRC = 0x28845199;
				hello.dwErrorCRC    = 0x93899e2c;
				hello.serverType    = 1;
				memmove(hello.clientIp, info.ip, sizeof(hello.clientIp));
				STATIC_ASSERT(sizeof(hello.clientIp) == sizeof(info.ip));
				hello.clientPort = info.port;
				hello.tqosWorldId = 1;

				LOG("[client%03d] Server :: SA_FirstHello :: protocolCrc=%x errorCrc=%x serverType=%d clientIp=(%s) clientPort=%d tqosWorldId=%d", clientID, hello.dwProtocolCRC, hello.dwErrorCRC, hello.serverType, IpToString(hello.clientIp), hello.clientPort, hello.tqosWorldId);
				server->SendPacket(clientID, hello);
			} break;

			case Cl::CQ_Authenticate::NET_ID: {
				ConstBuffer request(packetData, packetSize);
				const u16 nickLen = request.Read<u16>();
				const wchar* nick = (wchar*)request.ReadRaw(nickLen * sizeof(wchar));
				i32 var = request.Read<i32>();
				LOG("[client%03d] Client :: RequestConnectGame :: %.*S var=%d", clientID, nickLen, nick, var);

				const Server::ClientInfo& info = server->clientInfo[clientID];

				Sv::SA_AuthResult auth;
				auth.result = 91;
				LOG("[client%03d] Server :: SA_AuthResult :: result=%d", clientID, auth.result);
				server->SendPacket(clientID, auth);

				// TODO: confirm authentication
				// TODO: fetch account data
				AccountData& account = accountData[clientID];
				account = {};
				account.nickname.Set(nick, nickLen);

				// TODO: make the client enter the BRIDGE game
				game->CoordinatorClientEnter(clientID, &account);
			} break;

			default: {

				// TODO: dispatch packets to games depending on clients playing to them
				game->CoordinatorClientHandlePacket(clientID, header, packetData);
			} break;
		}
	}

	void ClientHandleReceivedChunk(i32 clientID, const u8* data, const i32 dataSize)
	{
		if(dataSize < sizeof(NetHeader)) {
			LOG("ERROR(ClientHandleReceivedChunk): received invalid data (%d < %d)", dataSize, (i32)sizeof(NetHeader));
			server->DisconnectClient(clientID);
			return;
		}

		ConstBuffer buff(data, dataSize);
		while(buff.CanRead(sizeof(NetHeader))) {
#ifdef CONF_DEBUG
			const u8* data = buff.cursor;
#endif
			const NetHeader& header = buff.Read<NetHeader>();
			const u8* packetData = buff.ReadRaw(header.size - sizeof(NetHeader));

#ifdef CONF_DEBUG
			fileSaveBuff(FMT("trace\\game_%d_cl_%d.raw", server->packetCounter, header.netID), data, header.size);
			server->packetCounter++;
#endif
			ClientHandlePacket(clientID, header, packetData);
		}
	}

	void Update()
	{
		server->TransferAllReceivedData(&recvDataBuff);

		ConstBuffer buff(recvDataBuff.data, recvDataBuff.size);
		while(buff.CanRead(sizeof(Server::ReceiveBufferHeader))) {
			const Server::ReceiveBufferHeader& header = buff.Read<Server::ReceiveBufferHeader>();
			const u8* data = buff.ReadRaw(header.len);
			ClientHandleReceivedChunk(header.clientID, data, header.len);
		}

		recvDataBuff.Clear();

		Sleep(16); // TODO: do a time accumulator scheme to wait a precise time
	}
};

DWORD ThreadCoordinator(void* pData)
{
	Coordinator& coordinator = *(Coordinator*)pData;

	while(coordinator.server->running) {
		coordinator.Update();
	}
	return 0;
}


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
