#include <common/base.h>
#include <common/protocol.h>
#include <common/network.h>
#include <mutex>

// TODO:

#define LISTEN_PORT "11900"

DWORD ThreadNetwork(void* pData);
DWORD ThreadGame(void* pData);

struct Server
{
	enum {
		MAX_CLIENTS=256,
		RECV_BUFF_LEN=8192,
		SEND_BUFF_LEN=8192,
	};

	struct ClientNet
	{
		sockaddr addr;
		GrowableBuffer recvPendingProcessingBuff;
		GrowableBuffer pendingSendBuff;
		GrowableBuffer sendingBuff;
		u8 recvBuffID;
		u8 sendBuffID;
		std::mutex mutexRecv;
		std::mutex mutexSend;
		OVERLAPPED recvOverlapped;
		OVERLAPPED sendOverlapped;
		HANDLE hEventRecv;
		HANDLE hEventSend;
		char recvBuff[RECV_BUFF_LEN];
		char sendBuff[SEND_BUFF_LEN];
	};

	struct ClientInfo
	{
		u8 ip[4];
		u16 port;
	};

	struct ReceiveBufferHeader
	{
		i32 clientID;
		i32 len;
	};

	bool running;
	SOCKET clientSocket[MAX_CLIENTS];
	ClientNet clientNet[MAX_CLIENTS];
	ClientInfo clientInfo[MAX_CLIENTS];

	void Init()
	{
		for(int i = 0; i < MAX_CLIENTS; i++) {
			clientSocket[i] = INVALID_SOCKET;
			ClientNet& client = clientNet[i];
			client.hEventRecv = WSA_INVALID_EVENT;
			client.hEventSend = WSA_INVALID_EVENT;
		}

		CreateThread(NULL, 0, ThreadNetwork, this, 0, NULL);
		running = true;
	}

	i32 AddClient(SOCKET s, const sockaddr& addr_)
	{
		for(int clientID = 0; clientID < MAX_CLIENTS; clientID++) {
			if(clientSocket[clientID] == INVALID_SOCKET) {
				// set non blocking
				u_long NonBlocking = true;
				int ior = ioctlsocket(s, FIONBIO, &NonBlocking);
				if(ior != NO_ERROR) {
					LOG("ERROR(socket=%x): failed to change io mode (%d)", (u32)s, WSAGetLastError());
					closesocket(s);
					ASSERT(0); // we want to catch that error
					return -1;
				}

				clientSocket[clientID] = s;
				ClientNet& client = clientNet[clientID];
				client.addr = addr_;

				if(client.recvPendingProcessingBuff.data == nullptr) {
					client.recvPendingProcessingBuff.Init(RECV_BUFF_LEN * 4);
				}
				client.recvPendingProcessingBuff.Clear();

				if(client.pendingSendBuff.data == nullptr) {
					client.pendingSendBuff.Init(SEND_BUFF_LEN * 4);
				}
				client.pendingSendBuff.Clear();

				if(client.sendingBuff.data == nullptr) {
					client.sendingBuff.Init(SEND_BUFF_LEN);
				}
				client.sendingBuff.Clear();

				client.recvBuffID = 0;
				client.sendBuffID = 0;

				if(client.hEventRecv != WSA_INVALID_EVENT) {
					client.hEventRecv = WSACreateEvent();
				}
				if(client.hEventSend != WSA_INVALID_EVENT) {
					client.hEventSend = WSACreateEvent();
				}

				memset(&client.recvOverlapped, 0, sizeof(client.recvOverlapped));
				client.recvOverlapped.hEvent = client.hEventRecv;
				memset(&client.sendOverlapped, 0, sizeof(client.sendOverlapped));
				client.sendOverlapped.hEvent = client.hEventSend;

				// start receiving
				WSABUF buff;
				buff.len = RECV_BUFF_LEN;
				buff.buf = client.recvBuff;

				DWORD len;
				DWORD flags = 0;
				int r = WSARecv(s, &buff, 1, &len, &flags, &client.recvOverlapped, NULL);
				if(r == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
					LOG("ERROR: receive failed (%d)", WSAGetLastError());
					DisconnectClient(clientID);
					continue;
				}

				ClientInfo& info = clientInfo[clientID];
				struct sockaddr_in& sin = *(struct sockaddr_in*)&client.addr;
				const u8* clIp = (u8*)&sin.sin_addr;
				SetIp(info.ip, clIp[0], clIp[1], clIp[2], clIp[3]);
				info.port = htons(sin.sin_port);
				return clientID;
			}
		}

		ASSERT(0); // no space
		return -1;
	}

	void Update()
	{
		for(int clientID = 0; clientID < MAX_CLIENTS; clientID++) {
			SOCKET sock = clientSocket[clientID];
			if(sock == INVALID_SOCKET) continue;

			ClientNet& client = clientNet[clientID];

			DWORD len;
			DWORD flags = 0;
			i32 r = WSAGetOverlappedResult(sock, &client.recvOverlapped, &len, FALSE, &flags);
			if(r == FALSE) {
				if(WSAGetLastError() != WSA_IO_INCOMPLETE) {
					LOG("[client%03d] Recv WSAGetOverlappedResult failed (%d)", clientID, WSAGetLastError());
					DisconnectClient(clientID);
					continue;
				}
			}
			else {
				LOG("[client%03d] Received %d bytes", clientID, len);
				if(len == 0) {
					DisconnectClient(clientID);
					continue;
				}

				// append to pending processing buffer
				{
					const std::lock_guard<std::mutex> lock(client.mutexRecv);
					client.recvPendingProcessingBuff.Append(client.recvBuff, len);
				}

				// start receiving again
				memset(&client.recvOverlapped, 0, sizeof(client.recvOverlapped));
				client.recvOverlapped.hEvent = client.hEventRecv;

				WSABUF buff;
				buff.len = RECV_BUFF_LEN;
				buff.buf = client.recvBuff;

				DWORD len;
				DWORD flags = 0;
				int r = WSARecv(sock, &buff, 1, &len, &flags, &client.recvOverlapped, NULL);
				if(r == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
					LOG("ERROR: receive failed (%d)", WSAGetLastError());
					DisconnectClient(clientID);
					continue;
				}
			}

			len = 0;
			flags = 0;
			r = WSAGetOverlappedResult(sock, &client.sendOverlapped, &len, FALSE, &flags);
			if(r == FALSE) {
				if(WSAGetLastError() != WSA_IO_INCOMPLETE) {
					LOG("[client%03d] Send WSAGetOverlappedResult failed (%d)", clientID, WSAGetLastError());
					DisconnectClient(clientID);
				}
			}
			else {
				if(len > 0) {
					LOG("[client%03d] Sent %d bytes", clientID, len);
					ASSERT(len == client.sendingBuff.size);
					client.sendingBuff.Clear();
				}

				memset(&client.sendOverlapped, 0, sizeof(client.sendOverlapped));
				client.sendOverlapped.hEvent = client.hEventSend;

				if(client.pendingSendBuff.size > 0) {
					{
						const std::lock_guard<std::mutex> lock(client.mutexSend);
						client.sendingBuff.Append(client.pendingSendBuff.data, client.pendingSendBuff.size);
						client.pendingSendBuff.Clear();
					}

					WSABUF buff;
					buff.len = client.sendingBuff.size;
					buff.buf = (char*)client.sendingBuff.data;

					DWORD len;
					DWORD flags = 0;
					int err;
					int r = WSASend(sock, &buff, 1, &len, flags, &client.sendOverlapped, NULL);
					if(r == SOCKET_ERROR && (err = WSAGetLastError()) != WSA_IO_PENDING) {
						LOG("[client%03d] ERROR: send failed (%d)", clientID, err);
						DisconnectClient(clientID);
					}
				}
			}
		}
	}

	void DisconnectClient(i32 clientID)
	{
		ASSERT(clientID >= 0 && clientID < MAX_CLIENTS);
		if(clientSocket[clientID] == INVALID_SOCKET) return;

		closesocket(clientSocket[clientID]);
		clientSocket[clientID] = INVALID_SOCKET;
		ClientNet& client = clientNet[clientID];
		WSAResetEvent(client.hEventRecv);
		WSAResetEvent(client.hEventSend);

		LOG("[client%03d] disconnected", clientID);
	}

	void TransferAllReceivedData(GrowableBuffer* out)
	{
		for(int clientID = 0; clientID < MAX_CLIENTS; clientID++) {
			if(clientSocket[clientID] == INVALID_SOCKET) continue;
			ClientNet& client = clientNet[clientID];

			{
				const std::lock_guard<std::mutex> lock(client.mutexRecv);

				if(client.recvPendingProcessingBuff.size > 0) {
					ReceiveBufferHeader header;
					header.clientID = clientID;
					header.len = client.recvPendingProcessingBuff.size;

					out->Append(&header, sizeof(header));
					out->Append(client.recvPendingProcessingBuff.data, client.recvPendingProcessingBuff.size);

					client.recvPendingProcessingBuff.Clear();
				}
			}
		}
	}

	void ClientSend(i32 clientID, const void* data, i32 dataSize)
	{
		ASSERT(clientID >= 0 && clientID < MAX_CLIENTS);
		if(clientSocket[clientID] == INVALID_SOCKET) return;

		ClientNet& client = clientNet[clientID];
		const std::lock_guard<std::mutex> lock(client.mutexSend);
		client.pendingSendBuff.Append(data, dataSize);
	}
};

DWORD ThreadNetwork(void* pData)
{
	Server& server = *(Server*)pData;

	while(server.running) {
		server.Update();
	}
	return 0;
}

struct Game
{
	Server* server;
	GrowableBuffer recvDataBuff;

	void Init(Server* server_)
	{
		server = server_;
		CreateThread(NULL, 0, ThreadGame, this, 0, NULL);

		recvDataBuff.Init(10 * (1024*1024)); // 10 MB
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
			static i32 counter = 0;
			fileSaveBuff(FMT("game_cl_%d_%d.raw", header.netID, counter), data, header.size);
			counter++;
#endif
			ClientHandlePacket(clientID, header, packetData);
		}
	}

	void ClientHandlePacket(i32 clientID, const NetHeader& header, const u8* packetData)
	{
		const i32 packetSize = header.size - sizeof(NetHeader);

		switch(header.netID) {
			case Cl::Hello::NET_ID: {
				LOG("[client%03d] Client :: Hello", clientID);

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
				SendPacket(clientID, hello);
			} break;

			case Cl::RequestConnectGame::NET_ID: {
				ConstBuffer request(packetData, packetSize);
				u16 nickLen = request.Read<u16>();
				const wchar* nick = (wchar*)request.ReadRaw(nickLen * sizeof(wchar));
				i32 var = request.Read<i32>();
				LOG("[client%03d] Client :: RequestConnectGame :: %.*ws", clientID, nickLen, nick, var);

				const Server::ClientInfo& info = server->clientInfo[clientID];

				Sv::SA_AuthResult auth;
				auth.result = 91;
				LOG("[client%03d] Server :: SA_AuthResult :: result=%d", clientID, auth.result);
				SendPacket(clientID, auth);

				// SN_RegionServicePolicy
				{
					u8 sendData[256];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u16>(1); // newMasterRestrict_count
					packet.Write<u8>(1); // newMasterRestrict[0]

					packet.Write<u16>(1); // userGradePolicy_count
					packet.Write<u8>(5); // userGradePolicy[0].userGrade
					packet.Write<u16>(1); // userGradePolicy[0].benefits_count
					packet.Write<u8>(9); // userGradePolicy[0].benefits[0]

					packet.Write<u8>(2); // purchaseCCoinMethod
					packet.Write<u8>(1); // exchangeCCoinForGoldMethod
					packet.Write<u8>(0); // rewardCCoinMethod
					packet.Write<u8>(1); // pveRewardSlotOpenBuyChanceMethod

					packet.Write<u16>(3); // regionBanMaster_count
					packet.Write<i32>(100000041); // regionBanMaster[0]
					packet.Write<i32>(100000042); // regionBanMaster[1]
					packet.Write<i32>(100000043); // regionBanMaster[2]

					packet.Write<u16>(1); // regionNewMaster_count
					packet.Write<i32>(100000038); // intList2[0]

					packet.Write<u16>(0); // eventBanMaster_count

					packet.Write<i32>(5);	// checkPeriodSec
					packet.Write<i32>(10);	// maxTalkCount
					packet.Write<i32>(120); // blockPeriodSec

					packet.Write<u16>(0); // regionBanSkinList_count
					packet.Write<u16>(0); // pcCafeSkinList_count

					packet.Write<u8>(1); // useFatigueSystem

					LOG("[client%03d] Server :: SN_RegionServicePolicy :: ", clientID);
					SendPacketData(clientID, Sv::SN_RegionServicePolicy::NET_ID, packet.size, packet.data);
				}

				// SN_AllCharacterBaseData
				{
					u8 sendData[4096];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u16>(1); // charaList_count

					packet.Write<i32>(100000001); // charaList[0].masterID
					packet.Write<u16>(22); // charaList[0].baseStats_count

					// charaList[0].baseStats
					typedef Sv::SN_AllCharacterBaseData::Character::Stat Stat;
					packet.Write(Stat{ 0, 2400.f });
					packet.Write(Stat{ 2, 200.f });
					packet.Write(Stat{ 37, 120.f });
					packet.Write(Stat{ 5, 5.f });
					packet.Write(Stat{ 42, 0.6f });
					packet.Write(Stat{ 7, 92.3077f });
					packet.Write(Stat{ 9, 3.f });
					packet.Write(Stat{ 10, 150.f });
					packet.Write(Stat{ 18, 100.f });
					packet.Write(Stat{ 13, 100.f });
					packet.Write(Stat{ 14, 100.f });
					packet.Write(Stat{ 15, 100.f });
					packet.Write(Stat{ 52, 100.f });
					packet.Write(Stat{ 16, 1.f });
					packet.Write(Stat{ 29, 20.f });
					packet.Write(Stat{ 23, 9.f });
					packet.Write(Stat{ 31, 14.f });
					packet.Write(Stat{ 22, 2.f });
					packet.Write(Stat{ 54, 15.f });
					packet.Write(Stat{ 63, 3.f });
					packet.Write(Stat{ 64, 150.f });
					packet.Write(Stat{ 55, 15.f });

					packet.Write<u16>(7); // charaList[0].skillData_count

					// charaList[0].skillData
					typedef Sv::SN_AllCharacterBaseData::Character::SkillRatio SkillR;
					packet.Write(SkillR{ 180010020, 355.f, 0.42f, 0.f, 0.f, 0.f });
					packet.Write(SkillR{ 180010040, 995.f, 0.81f, 0.f, 0.f, 0.1f });
					packet.Write(SkillR{ 180010010, 550.f, 0.56f, 0.f, 0.f, 0.f });
					packet.Write(SkillR{ 180010030, 0.f, 0.f, 0.f, 0.f, 0.f });
					packet.Write(SkillR{ 180010050, 680.f, 0.37f, 0.f, 0.f, 0.f });
					packet.Write(SkillR{ 180010000, 0.f, 1.0f, 0.f, 0.f, 0.f });
					packet.Write(SkillR{ 180010002, 0.f, 1.0f, 0.f, 0.f, 0.f });

					packet.Write<i32>(1); // cur
					packet.Write<i32>(1); // max


					LOG("[client%03d] Server :: SN_AllCharacterBaseData :: ", clientID);
					SendPacketData(clientID, Sv::SN_AllCharacterBaseData::NET_ID, packet.size, packet.data);
				}

				// SN_MyGuild
				{
					u8 sendData[256];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.WriteStringObj(L"XMX");
					packet.Write<i64>(0);
					packet.Write<u8>(0);

					LOG("[client%03d] Server :: SN_MyGuild :: ", clientID);
					SendPacketData(clientID, Sv::SN_MyGuild::NET_ID, packet.size, packet.data);
				}
			} break;

			case Cl::Unknown_60148::NET_ID: {
				LOG("[client%03d] Client :: Unknown_60148 ::", clientID);

				// SN_ProfileCharacters
				{
					u8 sendData[2048];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u16>(1); // charaList_count

					Sv::SN_ProfileCharacters::Character chara;
					chara.characterID = 21003;
					chara.creatureIndex = 100000001;
					chara.skillShot1 = 180010020;
					chara.skillShot2 = 180010040;
					chara.class_ = 1;
					chara.x = 12029;
					chara.y = 12622;
					chara.z = 3328.29f;
					chara.characterType = 5;
					chara.skinIndex = 3;
					chara.weaponIndex = 131101011;
					chara.masterGearNo = 1;

					packet.Write(chara);

					LOG("[client%03d] Server :: SN_ProfileCharacters :: ", clientID);
					SendPacketData(clientID, Sv::SN_ProfileCharacters::NET_ID, packet.size, packet.data);
				}

				// SN_ProfileWeapons
				{
					u8 sendData[2048];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u16>(1); // weaponList_count

					Sv::SN_ProfileWeapons::Weapon weap;
					weap.characterID = 21003;
					weap.weaponType = 1;
					weap.weaponIndex = 131101011;
					weap.grade = 0;
					weap.isUnlocked = 0;
					weap.isActivated = 0;

					packet.Write(weap);

					LOG("[client%03d] Server :: SN_ProfileWeapons :: ", clientID);
					SendPacketData(clientID, Sv::SN_ProfileWeapons::NET_ID, packet.size, packet.data);
				}
			} break;

			default: {
				LOG("[client%03d] Client :: Unkown packet :: size=%d netID=%d", clientID, header.size, header.netID);
			} break;
		}
	}

	template<typename Packet>
	void SendPacket(i32 clientID, const Packet& packet)
	{
		SendPacketData(clientID, Packet::NET_ID, sizeof(packet), &packet);
	}

	void SendPacketData(i32 clientID, u16 netID, u16 packetSize, const void* packetData)
	{
		const i32 packetTotalSize = packetSize+sizeof(NetHeader);
		u8 sendBuff[8192];
		ASSERT(packetTotalSize <= sizeof(sendBuff));

		NetHeader header;
		header.size = packetTotalSize;
		header.netID = netID;
		memmove(sendBuff, &header, sizeof(header));
		memmove(sendBuff+sizeof(NetHeader), packetData, packetSize);

		server->ClientSend(clientID, sendBuff, packetTotalSize);
	}
};

DWORD ThreadGame(void* pData)
{
	Game& game = *(Game*)pData;

	while(game.server->running) {
		game.Update();
	}
	return 0;
}

int main(int argc, char** argv)
{
	LogInit("game_server.log");
	LOG(".: Game server :.");

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

	static Server server;
	server.Init();

	static Game game;
	game.Init(&server);

	// accept thread
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

		LOG("New connection (%s)", GetIpString(clientAddr));
		server.AddClient(clientSocket, clientAddr);
	}

	return 0;
}
