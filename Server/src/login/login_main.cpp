#include <common/base.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <common/protocol.h>

// TODO:
// - finish login sequence
// - connect to bridge server

#define LISTEN_PORT "10900"

struct Client
{
	i32 clientID;
	SOCKET sock;
	sockaddr addr;

	u8 recvBuff[8192];
	bool running = true;

	void Run()
	{
		while(running) {
			Receive();
		}
	}
	void Receive()
	{
		int len;
		do {
			len = recv(sock, (char*)recvBuff, sizeof(recvBuff), 0);
			if(len > 0) {
				LOG("Bytes received: %d", len);
				HandleReveivedChunk(len);
			}
			else if(len == 0) {
				LOG("Connection closing...");
				Disconnect();
			}
			else {
				LOG("ERROR(recv): failed: %d", WSAGetLastError());
				Disconnect();
			}
		} while (len > 0);
	}

	void Disconnect()
	{
		closesocket(sock);
		running = false;
	}

	void HandleReveivedChunk(const i32 recvLen)
	{
		if(recvLen < sizeof(NetHeader)) {
			LOG("ERROR(HandleReveivedChunk): received invalid data");
			Disconnect();
			return;
		}

		ConstBuffer buff(recvBuff, recvLen);
		while(buff.CanRead(sizeof(NetHeader))) {
			const NetHeader& header = buff.Read<NetHeader>();
			const u8* packetData = buff.ReadRaw(header.size - sizeof(NetHeader));
			HandlePacket(header, packetData);
		}
	}

	void HandlePacket(const NetHeader& header, const u8* packetData)
	{
		switch(header.netID) {
			case Cl_Hello::NET_ID: {
				LOG("Packet :: Cl_Hello");

				LOG(">> Send :: Sv_Hello");
				Sv_Hello hello;
				const char* key = "\x99\x51\x84\x28\x2c\x9e\x89\x93\x00\xd7\xe9\x41\x57\x19\xc5\x01";
				memmove(hello.key, key, sizeof(hello.key));
				SendPacket(hello);
			} break;

			case Cl_Login::NET_ID: {
				ConstBuffer data(packetData, header.size);

				u16 loginStrSize = data.Read<u16>();
				wchar* loginStr = (wchar*)data.ReadRaw(sizeof(wchar) * loginStrSize);
				u16 unkSize = data.Read<u16>();
				wchar* unkStr = (wchar*)data.ReadRaw(sizeof(wchar) * unkSize);
				u16 typeSize = data.Read<u16>();
				wchar* typeStr = (wchar*)data.ReadRaw(sizeof(wchar) * typeSize);

				LOG("Packet :: Cl_Login :: '%.*ws' '%.*ws' '%.*ws'", loginStrSize, loginStr, unkSize, unkStr, typeSize, typeStr);

				LOG(">> Send :: Sv_AcceptLogin");
				Sv_AcceptLoginType0 accept;
				accept.ping = 0x33;
				SendPacket(accept);
			} break;

			case Cl_ConfirmLogin::NET_ID: {
				LOG("Packet :: Cl_ConfirmLogin ::");

				LOG(">> Send :: Sv_AccountData");
				u8 accountData[203];
				i32 fileSize;
				u8* fileData = fileOpenAndReadAll("sv_account_data.raw", &fileSize);
				ASSERT(fileSize == sizeof(accountData));
				memmove(accountData, fileData, sizeof(accountData));
				SendPacketData(Sv_AccountData::NET_ID, sizeof(accountData), accountData);
			} break;
		}
	}

	template<typename Packet>
	void SendPacket(const Packet& packet)
	{
		SendPacketData(Packet::NET_ID, sizeof(packet), &packet);
	}

	void SendPacketData(u16 netID, u16 packetSize, const void* packetData)
	{
		const i32 packetTotalSize = packetSize+sizeof(NetHeader);
		u8 sendBuff[8192];
		ASSERT(packetTotalSize <= sizeof(sendBuff));

		NetHeader header;
		header.size = packetTotalSize;
		header.netID = netID;
		memmove(sendBuff, &header, sizeof(header));
		memmove(sendBuff+sizeof(NetHeader), packetData, packetSize);

		int r = send(sock, (char*)sendBuff, packetTotalSize, 0);
		if(r == SOCKET_ERROR) {
			Disconnect();
		}
	}
};

DWORD ThreadClient(void* pData)
{
	Client client = *(Client*)pData;
	LOG("Client_%d thread :: init", client.clientID);

	client.Run();

	LOG("Client_%d thread :: close", client.clientID);
	return 0;
}

const char* GetIpString(const sockaddr& addr)
{
	const sockaddr_in& in = *(sockaddr_in*)&addr;
	return FMT("%s:%d", inet_ntoa(in.sin_addr), in.sin_port);
}

int main(int argc, char** argv)
{
	LogInit("login_server.log");
	LOG(".: Login server :.");

	// Initialize Winsock
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if(iResult != 0) {
		LOG("ERROR: WSAStartup failed: %d", iResult);
		return 1;
	}
	defer(WSACleanup());

	struct addrinfo *result = NULL, *ptr = NULL, hints;

	ZeroMemory(&hints, sizeof (hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the local address and port to be used by the server
	iResult = getaddrinfo(NULL, LISTEN_PORT, &hints, &result);
	if (iResult != 0) {
		LOG("ERROR: getaddrinfo failed: %d", iResult);
		return 1;
	}
	defer(freeaddrinfo(result));

	SOCKET serverSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if(serverSocket == INVALID_SOCKET) {
		LOG("ERROR(socket): %ld", WSAGetLastError());
		return 1;
	}
	defer(closesocket(serverSocket));

	// Setup the TCP listening socket
	iResult = bind(serverSocket, result->ai_addr, (int)result->ai_addrlen);
	if(iResult == SOCKET_ERROR) {
		LOG("ERROR(bind): failed with error: %d", WSAGetLastError());
		return 1;
	}

	// listen
	if(listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
		LOG("ERROR(listen): failed with error: %ld", WSAGetLastError());
		return 1;
	}

	while(true) {
		// Accept a client socket
		LOG("Waiting for a connection...");
		struct sockaddr clientAddr;
		int addrLen = sizeof(sockaddr);
		SOCKET clientSocket = accept(serverSocket, &clientAddr, &addrLen);
		if(clientSocket == INVALID_SOCKET) {
			LOG("ERROR(accept): failed: %d", WSAGetLastError());
			return 1;
		}

		// TODO: handle multiple clients
		static Client client;
		client.clientID = 0;
		client.sock = clientSocket;
		client.addr = clientAddr;

		LOG("New connection (%s)", GetIpString(clientAddr));
		HANDLE hThread = CreateThread(NULL, 0, ThreadClient, &client, 0, NULL);
		ASSERT(hThread != NULL);
	}

	return 0;
}
