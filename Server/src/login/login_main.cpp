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

const char* IpToString(const u8* ip)
{
	return FMT("%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}

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
#ifdef CONF_DEBUG
			const u8* data = buff.cursor;
#endif
			const NetHeader& header = buff.Read<NetHeader>();
			const u8* packetData = buff.ReadRaw(header.size - sizeof(NetHeader));

#ifdef CONF_DEBUG
			static i32 counter = 0;
			fileSaveBuff(FMT("cl_%d_%d.raw", header.netID, counter), data, header.size);
			counter++;
#endif
			HandlePacket(header, packetData);
		}
	}

	void HandlePacket(const NetHeader& header, const u8* packetData)
	{
		const i32 packetSize = header.size - sizeof(NetHeader);

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
				SendPacketData(Sv_AccountData::NET_ID, sizeof(accountData) - sizeof(NetHeader), accountData + sizeof(NetHeader));

				LOG(">> Send :: Sv_GatewayServerInfo");
				u8 gatewayData[256];
				PacketWriter packet(gatewayData, sizeof(gatewayData));
				const wchar* infoStr = L"Gateway Server CSP 1.17.1017.7954";
				const i32 infoStrLen = 33;
				packet.Write<u16>(infoStrLen);
				packet.WriteRaw(infoStr, infoStrLen*sizeof(wchar));
				SendPacketData(Sv_GatewayServerInfo::NET_ID, packet.size, packet.data);

			} break;

			case Cl_ConfirmGatewayInfo::NET_ID: {
				const Cl_ConfirmGatewayInfo& confirm = SafeCast<Cl_ConfirmGatewayInfo>(packetData, packetSize);
				LOG("Packet :: Cl_ConfirmGatewayInfo :: var=%d", confirm.var);


				LOG(">> Send :: Sv_SendStationList");
				u8 sendData[256];
				PacketWriter packet(sendData, sizeof(sendData));

				packet.Write<u16>(1); // count

				Sv_SendStationList::Station station;
				station.ID = 12345678;
				station.count = 1;
				auto& serverIP = station.serverIPs[0]; // alias
				serverIP.game[0] = 127;
				serverIP.game[1] = 0;
				serverIP.game[2] = 0;
				serverIP.game[3] = 1;
				serverIP.ping[0] = 127;
				serverIP.ping[1] = 0;
				serverIP.ping[2] = 0;
				serverIP.ping[3] = 1;
				serverIP.port = 12900;

				packet.Write(station); // station

				SendPacketData(Sv_SendStationList::NET_ID, packet.size, packet.data);
			} break;

			case Cl_EnterQueue::NET_ID: {
				const Cl_EnterQueue& enter = SafeCast<Cl_EnterQueue>(packetData, packetSize);
				LOG("Packet :: Cl_EnterQueue :: var1=%d gameIp=(%s) unk=%d pingIp=(%s) port=%d unk2=%d stationID=%d", enter.var1, IpToString(enter.gameIp), enter.unk, IpToString(enter.pingIp), enter.port, enter.unk2, enter.stationID);

				LOG(">> Send :: Sv_QueueStatus");
				Sv_QueueStatus status;
				memset(&status, 0, sizeof(status));
				status.var1 = 2;
				status.var1 = 5;
				SendPacket(status);

				LOG(">> Send :: Sv_Finish");
				u8 sendData[256];
				PacketWriter packet(sendData, sizeof(sendData));

				packet.Write<u16>(1); // count
				u8 ip[4] = { 127, 0, 0, 1};
				packet.Write<u8[4]>(ip); // ip
				packet.Write<u16>(11900); // port

				const wchar* serverName = L"XMX_SERVER";
				packet.Write<u16>(10); // serverNamelen
				packet.WriteRaw(serverName, 10 * sizeof(wchar)); // serverName

				const wchar* nick = L"LordSk";
				packet.Write<u16>(6); // nickLen
				packet.WriteRaw(nick, 6 * sizeof(wchar)); // nick

				packet.Write<i32>(536);
				packet.Write<i32>(1);

				SendPacketData(Sv_Finish::NET_ID, packet.size, packet.data);
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
