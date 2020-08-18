#include <common/base.h>
#include <common/protocol.h>
#include <common/network.h>
#include <direct.h>

// TODO:
// - connect to bridge server

#define LISTEN_PORT "10900"

struct Client
{
	i32 clientID;
	SOCKET sock;
	sockaddr addr;
	u8 clientIp[4];
	u16 clientPort;

	u8 recvBuff[8192];
	bool running = true;

	void Run()
	{
		struct sockaddr_in& sin = *(struct sockaddr_in*)&addr;
		const u8* clIp = (u8*)&sin.sin_addr;
		SetIp(clientIp, clIp[0], clIp[1], clIp[2], clIp[3]);
		clientPort = htons(sin.sin_port);

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
			fileSaveBuff(FMT("trace\\login_%d_cl_%d.raw", counter, header.netID), data, header.size);
			counter++;
#endif
			HandlePacket(header, packetData);
		}
	}

	void HandlePacket(const NetHeader& header, const u8* packetData)
	{
		const i32 packetSize = header.size - sizeof(NetHeader);

		switch(header.netID) {
			case Cl::CQ_FirstHello::NET_ID: {
				LOG("Client :: Hello");

				Sv::SA_FirstHello hello;
				hello.dwProtocolCRC = 0x28845199;
				hello.dwErrorCRC    = 0x93899e2c;
				hello.serverType    = 0;
				memmove(hello.clientIp, clientIp, sizeof(hello.clientIp));
				STATIC_ASSERT(sizeof(hello.clientIp) == sizeof(clientIp));
				hello.clientPort = clientPort;
				hello.tqosWorldId = 1;

				LOG("Server :: SA_FirstHello :: protocolCrc=%x errorCrc=%x serverType=%d clientIp=(%s) clientPort=%d tqosWorldId=%d", hello.dwProtocolCRC, hello.dwErrorCRC, hello.serverType, IpToString(hello.clientIp), hello.clientPort, hello.tqosWorldId);
				SendPacket(hello);
			} break;

			case Cl::CQ_UserLogin::NET_ID: {
				ConstBuffer data(packetData, header.size);

				u16 loginStrSize = data.Read<u16>();
				wchar* loginStr = (wchar*)data.ReadRaw(sizeof(wchar) * loginStrSize);
				u16 unkSize = data.Read<u16>();
				wchar* unkStr = (wchar*)data.ReadRaw(sizeof(wchar) * unkSize);
				u16 typeSize = data.Read<u16>();
				wchar* typeStr = (wchar*)data.ReadRaw(sizeof(wchar) * typeSize);

				LOG("Client :: UserLogin :: '%.*ws' '%.*ws' '%.*ws'", loginStrSize, loginStr, unkSize, unkStr, typeSize, typeStr);

				LOG("Server :: SA_UserloginResult");
				Sv::SA_UserloginResult accept;
				accept.result = 0x33;
				SendPacket(accept);
			} break;

			case Cl::ConfirmLogin::NET_ID: {
				LOG("Client :: ConfirmLogin ::");

				LOG("Server :: SN_TgchatServerInfo");
				{
					u8 sendData[256];
					PacketWriter packet(sendData, sizeof(sendData));

					// host
					const wchar* host = L"127.0.0.1";
					packet.Write<u16>(0);
					//packet.WriteRaw(host, 9 * sizeof(wchar));

					packet.Write<u16>(255); // port
					packet.Write<i32>(61); // gameID
					packet.Write<i32>(0); // serverID
					packet.Write<u32>(424242); // userID

					packet.WriteStringObj(L"XMX"); // gamename
					packet.WriteStringObj(L"LordSk@0.XMX"); // chatname
					packet.WriteStringObj(L"LordSk"); // playncname

					u8 signature[128]; // garbage here. TODO: actually fill that in?
					packet.Write<u16>(sizeof(signature));
					packet.WriteRaw(signature, sizeof(signature));

					packet.Write<u8>(0); // serverType

					SendPacketData(Sv::SN_TgchatServerInfo::NET_ID, packet.size, packet.data);
				}

				LOG("Server :: SA_VersionInfo");
				{
					u8 gatewayData[256];
					PacketWriter packet(gatewayData, sizeof(gatewayData));
					const wchar* infoStr = L"Gateway Server CSP 1.17.1017.7954";
					const i32 infoStrLen = 33;
					packet.Write<u16>(infoStrLen);
					packet.WriteRaw(infoStr, infoStrLen*sizeof(wchar));
					SendPacketData(Sv::SA_VersionInfo::NET_ID, packet.size, packet.data);
				}
			} break;

			case Cl::ConfirmGatewayInfo::NET_ID: {
				const Cl::ConfirmGatewayInfo& confirm = SafeCast<Cl::ConfirmGatewayInfo>(packetData, packetSize);
				LOG("Client :: Cl::ConfirmGatewayInfo :: var=%d", confirm.var);

				LOG("Server :: Sv::SendStationList");
				u8 sendData[256];
				PacketWriter packet(sendData, sizeof(sendData));

				packet.Write<u16>(1); // count

				Sv::SN_StationList::PST_Station station;
				station.idc = 12345678;
				station.stations_count = 1;
				auto& addr = station.stations[0]; // alias
				addr.gameServerIp[0] = 127;
				addr.gameServerIp[1] = 0;
				addr.gameServerIp[2] = 0;
				addr.gameServerIp[3] = 1;
				addr.pingServerIp[0] = 127;
				addr.pingServerIp[1] = 0;
				addr.pingServerIp[2] = 0;
				addr.pingServerIp[3] = 1;
				addr.port = 12900;

				packet.Write(station); // station

				SendPacketData(Sv::SN_StationList::NET_ID, packet.size, packet.data);
			} break;

			case Cl::EnterQueue::NET_ID: {
				const Cl::EnterQueue& enter = SafeCast<Cl::EnterQueue>(packetData, packetSize);
				LOG("Client :: Cl::EnterQueue :: var1=%d gameIp=(%s) unk=%d pingIp=(%s) port=%d unk2=%d stationID=%d", enter.var1, IpToString(enter.gameIp), enter.unk, IpToString(enter.pingIp), enter.port, enter.unk2, enter.stationID);

				LOG("Server :: Sv::QueueStatus");
				Sv::QueueStatus status;
				memset(&status, 0, sizeof(status));
				status.var1 = 2;
				status.var1 = 5;
				SendPacket(status);

				LOG("Server :: Sv::Finish");
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

				SendPacketData(Sv::SN_DoConnectChannelServer::NET_ID, packet.size, packet.data);
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

int main(int argc, char** argv)
{
	LogInit("login_server.log");
	LOG(".: Login server :.");

	_mkdir("trace");

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
