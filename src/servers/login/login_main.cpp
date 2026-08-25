#include <common/base.h>
#include <common/protocol.h>
#include <common/network.h>
#include <common/utils.h>
#include <common/packet_serialize.h>
#include <common/packet_validator.h>
#include <common/platform.h>
#include <EAStdC/EASprintf.h>

struct Config
{
	i32 listenPort = 10900;
	u8 gameServerIP[4] = { 127, 0, 0, 1 };
	i32 gameServerPort = 11900;
	i32 traceNetwork = 0;

	bool ParseLine(const char* line)
	{
		if(EA::StdC::Sscanf(line, "ListenPort=%d", &listenPort) == 1) return true;
		i32 ip[4];
		if(EA::StdC::Sscanf(line, "GameServerIP=%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4) {
			gameServerIP[0] = ip[0];
			gameServerIP[1] = ip[1];
			gameServerIP[2] = ip[2];
			gameServerIP[3] = ip[3];
			return true;
		}
		if(EA::StdC::Sscanf(line, "GameServerPort=%d", &gameServerPort) == 1) return true;
		if(EA::StdC::Sscanf(line, "TraceNetwork=%d", &traceNetwork) == 1) return true;
		return false;
	}

	// returns false on failing to open the config file
	bool LoadConfigFile()
	{
		i32 fileSize;
		u8* fileData = fileOpenAndReadAll("login.cfg", &fileSize);
		if(!fileData) {
			LOG("WARNING(LoadConfigFile): failed to open 'login.cfg', using default config");
			return false;
		}
		defer(memFree(fileData));

		eastl::fixed_string<char,2048,true> contents;
		contents.assign((char*)fileData, fileSize);

		auto endLine = contents.find('\n');

		// parse at least one line
		if(endLine == -1) {
			bool r = ParseLine(contents.data());
			if(!r) {
				LOG("WARNING(LoadConfigFile): failed to parse line '%s'", contents.data());
				return true;
			}
		}

		do {
			auto line = contents.left(endLine);
			bool r = ParseLine(line.data());
			if(!r) {
				LOG("WARNING(LoadConfigFile): failed to parse line '%s'", line.data());
			}

			contents = contents.substr(endLine+1, contents.size());
			endLine = contents.find('\n');
		} while(endLine != -1);

		// last line
		if(contents.size() > 0) {
			bool r = ParseLine(contents.data());
			if(!r) {
				LOG("WARNING(LoadConfigFile): failed to parse line '%s'", contents.data());
			}
		}

		return true;
	}

	void Print()
	{
		LOG("Config = {");
		LOG("	ListenPort=%d", listenPort);
		LOG("	GameServerIP=%d.%d.%d.%d", gameServerIP[0], gameServerIP[1], gameServerIP[2], gameServerIP[3]);
		LOG("	GameServerPort=%d", gameServerPort);
		LOG("	TraceNetwork=%d", traceNetwork);
		LOG("}");
	}
};

static Config g_Config;

struct Client
{
	i32 clientID;
	SOCKET sock;
	sockaddr addr;
	u8 clientIp[4];
	u16 clientPort;

	u8 recvBuff[8192];
	bool running = true;

	WideString nickname;

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
				LOG("ERROR(recv): failed: %d", NetworkGetLastError());
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
			const u8* data = buff.cursor;
			const NetHeader& header = buff.Read<NetHeader>();
			const u8* packetData = buff.ReadRaw(header.size - sizeof(NetHeader));

			if(g_Config.traceNetwork) {
				static i32 counter = 0;
				fileSaveBuff(FormatPath(FMT("trace/login_%d_cl_%d.raw", counter, header.netID)), data, header.size);
				counter++;
			}

			HandlePacket(header, packetData);
		}
	}

	void HandlePacket(const NetHeader& header, const u8* packetData)
	{
		const i32 packetSize = header.size - sizeof(NetHeader);

		switch(header.netID) {
				case Cl::CQ_FirstHello::NET_ID: {
				if(!ValidatePacket<Cl::CQ_FirstHello>(packetData, packetSize)) {
					LOG("WARNING: invalid CQ_FirstHello (size=%d expected=%d)", packetSize, (i32)sizeof(Cl::CQ_FirstHello));
					break;
				}
				NT_LOG("[client%x] Client :: %s", clientID, PacketSerialize<Cl::CQ_FirstHello>(packetData, packetSize));
				const Cl::CQ_FirstHello& req = SafeCast<Cl::CQ_FirstHello>(packetData, packetSize);

				Sv::SA_FirstHello hello;
				hello.dwProtocolCRC = 0x28845199;
				hello.dwErrorCRC    = 0x93899e2c;
				hello.serverType    = Sv::ServerType::Login;
				memmove(hello.clientIp, clientIp, sizeof(hello.clientIp));
				STATIC_ASSERT(sizeof(hello.clientIp) == sizeof(clientIp));
				hello.clientPort = clientPort;
				hello.tqosWorldId = 1;

				NT_LOG("[client%x] Server :: %s", clientID, PacketSerialize<Sv::SA_FirstHello>(&hello, sizeof(hello)));
				SendPacket(hello);
			} break;

			case Cl::CQ_UserLogin::NET_ID: {
				if(!ValidatePacket<Cl::CQ_UserLogin>(packetData, packetSize)) {
					LOG("WARNING: invalid CQ_UserLogin (size=%d)", packetSize);
					break;
				}
				NT_LOG("[client%x] Client :: %s", clientID, PacketSerialize<Cl::CQ_UserLogin>(packetData, packetSize));

				ConstBuffer data(packetData, header.size);
				u16 loginStrSize = data.Read<u16>();
				wchar* loginStr = (wchar*)data.ReadRaw(sizeof(wchar) * loginStrSize);
				nickname.assign(loginStr, loginStrSize);

				Sv::SA_UserloginResult accept;
				accept.result = 0x33;
				NT_LOG("[client%x] Server :: %s", clientID, PacketSerialize<Sv::SA_UserloginResult>(&accept, sizeof(accept)));
				SendPacket(accept);
			} break;

			case Cl::CQ_ServerVersionInfo::NET_ID: {
				// header-only packet (no payload): nothing to validate, just log
				NT_LOG("[client%x] Client :: %s", clientID, PacketSerialize<Cl::CQ_ServerVersionInfo>(packetData, packetSize));

				{
					PacketWriter<Sv::SN_TgchatServerInfo> packet;

					// host (ANSI byte string, length in bytes)
					const char* host = "127.0.0.1";
					packet.Write<u16>(9);
					packet.WriteRaw(host, 9);

					// chat server: point at a dead port - the hub does not implement the NPChat
					// protocol, so the client chat connect must fail cleanly (no hub handshake).
					packet.Write<u16>(12999); // port (no chat server)
					packet.Write<i32>(61); // gameID
					packet.Write<i32>(1); // serverID
					packet.Write<u32>(424242); // userID

					packet.WriteStringObj(L"Alpha"); // gamename
					packet.WriteStringObj(LFMT(L"%ls@0.XMX", nickname.data())); // chatname
					packet.WriteStringObj(nickname.data(), nickname.size()); // playncname

					u8 signature[128]; // garbage here. TODO: actually fill that in?
					packet.Write<u16>(sizeof(signature));
					packet.WriteRaw(signature, sizeof(signature));

					packet.Write<u8>(0); // serverType

					NT_LOG("[client%x] Server :: %s", clientID, PacketSerialize<Sv::SN_TgchatServerInfo>(packet.data, packet.size));
					SendPacketData(Sv::SN_TgchatServerInfo::NET_ID, packet.size, packet.data);
				}

				{
					PacketWriter<Sv::SA_ServerVersionInfo> packet;
					const wchar* infoStr = L"Gateway Server CSP 1.17.1017.7954";
					const i32 infoStrLen = 33;
					packet.Write<u16>(infoStrLen);
					packet.WriteRaw(infoStr, infoStrLen*sizeof(wchar));
					NT_LOG("[client%x] Server :: %s", clientID, PacketSerialize<Sv::SA_ServerVersionInfo>(packet.data, packet.size));
					SendPacketData(Sv::SA_ServerVersionInfo::NET_ID, packet.size, packet.data);
				}
			} break;

			case Cl::CQ_SetIspName::NET_ID: {
				if(!ValidatePacket<Cl::CQ_SetIspName>(packetData, packetSize)) {
					LOG("WARNING: invalid CQ_SetIspName (size=%d)", packetSize);
					break;
				}
				NT_LOG("[client%x] Client :: %s", clientID, PacketSerialize<Cl::CQ_SetIspName>(packetData, packetSize));

				PacketWriter<Sv::SN_StationList> packet;

				packet.Write<u16>(1); // count

				Sv::SN_StationList::PST_Station station;
				station.idc = 12345678;
				station.stations_count = 1;
				auto& addr = station.stations[0]; // alias
				addr.gameServerIp[0] = g_Config.gameServerIP[0];
				addr.gameServerIp[1] = g_Config.gameServerIP[1];
				addr.gameServerIp[2] = g_Config.gameServerIP[2];
				addr.gameServerIp[3] = g_Config.gameServerIP[3];
				addr.pingServerIp[0] = g_Config.gameServerIP[0];
				addr.pingServerIp[1] = g_Config.gameServerIP[1];
				addr.pingServerIp[2] = g_Config.gameServerIP[2];
				addr.pingServerIp[3] = g_Config.gameServerIP[3];
				addr.port = 12900; // ping server port

				packet.Write(station); // station

				NT_LOG("[client%x] Server :: %s", clientID, PacketSerialize<Sv::SN_StationList>(packet.data, packet.size));
				SendPacketData(Sv::SN_StationList::NET_ID, packet.size, packet.data);
			} break;

			case Cl::CQ_EnterWaitingQueue::NET_ID: {
				if(!ValidatePacket<Cl::CQ_EnterWaitingQueue>(packetData, packetSize)) {
					LOG("WARNING: invalid CQ_EnterWaitingQueue (size=%d)", packetSize);
					break;
				}
				NT_LOG("[client%x] Client :: %s", clientID, PacketSerialize<Cl::CQ_EnterWaitingQueue>(packetData, packetSize));

				Sv::SA_EnterWaitingQueue status;
				memset(&status, 0, sizeof(status));
				status.var1 = 2;
				status.var1 = 5;
				NT_LOG("[client%x] Server :: %s", clientID, PacketSerialize<Sv::SA_EnterWaitingQueue>(&status, sizeof(status)));
				SendPacket(status);

				PacketWriter<Sv::SN_DoConnectChannelServer> packet;

				packet.Write<u16>(1); // count
				packet.Write<u8[4]>(g_Config.gameServerIP); // ip
				packet.Write<u16>(g_Config.gameServerPort); // port

				const wchar* serverName = L"XMX_SERVER";
				packet.Write<u16>(10); // serverNamelen
				packet.WriteRaw(serverName, 10 * sizeof(wchar)); // serverName

				packet.WriteStringObj(nickname.data(), nickname.size()); // nick

				packet.Write<i32>(536);
				packet.Write<i32>(1);

				NT_LOG("[client%x] Server :: %s", clientID, PacketSerialize<Sv::SN_DoConnectChannelServer>(packet.data, packet.size));
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

intptr_t ThreadClient(void* pData)
{
	Client client = *(Client*)pData;
	LOG("Client_%d thread :: init", client.clientID);

	client.Run();

	LOG("Client_%d thread :: close", client.clientID);
	return 0;
}

struct LoginServer
{
	SOCKET sock;
	bool running = true;

	bool Init()
	{
		if(!NetworkInit()) {
			return false;
		}

		struct addrinfo *result = NULL, *ptr = NULL, hints;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;

		// Resolve the local address and port to be used by the server
		i32 iResult = getaddrinfo(NULL, FMT("%d", g_Config.listenPort), &hints, &result);
		if (iResult != 0) {
			LOG("ERROR: getaddrinfo failed: %d", iResult);
			return 1;
		}
		defer(freeaddrinfo(result));

		sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if(sock == INVALID_SOCKET) {
			LOG("ERROR(socket): %d", NetworkGetLastError());
			return false;
		}

		// Setup the TCP listening socket
		iResult = bind(sock, result->ai_addr, (int)result->ai_addrlen);
		if(iResult == SOCKET_ERROR) {
			LOG("ERROR(bind): failed with error: %d", NetworkGetLastError());
			return false;
		}

		// listen
		if(listen(sock, SOMAXCONN) == SOCKET_ERROR) {
			LOG("ERROR(listen): failed with error: %d", NetworkGetLastError());
			return false;
		}

		return true;
	}

	void Cleanup()
	{
		closesocket(sock);
		NetworkCleanup();
	}
};

static LoginServer* g_LoginServer = nullptr;

int main(int argc, char** argv)
{
	PlatformInit();
	LogInit("login_server.log");
	LogNetTrafficInit("login_server_nt.log");
	LOG(".: Login server :.");

	g_Config.LoadConfigFile();
	g_Config.Print();

	bool r = SetCloseSignalHandler([]()
	{
		g_LoginServer->running = false;
		closesocket(g_LoginServer->sock);
		g_LoginServer->sock = INVALID_SOCKET;
	});

	if(!r) {
		LOG("ERROR: failed to set close signal handler");
		return 1;
	}

	MakeDirectory("trace");

	static LoginServer server;
	g_LoginServer = &server;

	if(!server.Init()) {
		LOG("ERROR: failed to init server");
		return 1;
	}

	while(server.running) {
		// Accept a client socket
		LOG("Waiting for a connection...");
		struct sockaddr clientAddr;
		AddrLen addrLen = sizeof(sockaddr);
		SOCKET clientSocket = accept(server.sock, &clientAddr, &addrLen);
		if(clientSocket == INVALID_SOCKET) {
			if(server.running) {
				LOG("ERROR(accept): failed: %d", NetworkGetLastError());
				return 1;
			}
			else {
				break;
			}
		}

		// TODO: handle multiple clients
		static Client client;
		client.clientID = 0;
		client.sock = clientSocket;
		client.addr = clientAddr;

		LOG("New connection (%s)", GetIpString(clientAddr));
		EA::Thread::Thread thread;
		thread.Begin(ThreadClient, &client);
	}

	server.Cleanup();
	LOG("Done.");
	return 0;
}
