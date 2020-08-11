#include <common/base.h>
#include <common/protocol.h>
#include <common/network.h>
#include <mutex>

// TODO:
// - Handle instant receive

#define LISTEN_PORT "11900"

DWORD ThreadNetwork(void* pData);
DWORD ThreadGame(void* pData);

#ifdef CONF_DEBUG
static i32 packetCounter = 0;
#endif

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
	SOCKET serverSocket;
	SOCKET clientSocket[MAX_CLIENTS];
	ClientNet clientNet[MAX_CLIENTS];
	ClientInfo clientInfo[MAX_CLIENTS];

	bool Init()
	{
		// Initialize Winsock
		WSADATA wsaData;
		int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
		if(iResult != 0) {
			LOG("ERROR: WSAStartup failed: %d", iResult);
			return false;
		}

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
			return false;
		}
		defer(freeaddrinfo(result));

		serverSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if(serverSocket == INVALID_SOCKET) {
			LOG("ERROR(socket): %ld", WSAGetLastError());
			return false;
		}

		// Setup the TCP listening socket
		iResult = bind(serverSocket, result->ai_addr, (int)result->ai_addrlen);
		if(iResult == SOCKET_ERROR) {
			LOG("ERROR(bind): failed with error: %d", WSAGetLastError());
			return false;
		}

		// listen
		if(listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
			LOG("ERROR(listen): failed with error: %ld", WSAGetLastError());
			return false;
		}

		for(int i = 0; i < MAX_CLIENTS; i++) {
			clientSocket[i] = INVALID_SOCKET;
			ClientNet& client = clientNet[i];
			client.hEventRecv = WSA_INVALID_EVENT;
			client.hEventSend = WSA_INVALID_EVENT;
		}

		CreateThread(NULL, 0, ThreadNetwork, this, 0, NULL);
		running = true;
		return true;
	}

	void Cleanup()
	{
		LOG("Server shutting down...");

		closesocket(serverSocket);
		WSACleanup();
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

Server* g_Server = nullptr;

DWORD ThreadNetwork(void* pData)
{
	Server& server = *(Server*)pData;

	while(server.running) {
		server.Update();
	}

	server.Cleanup();
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
			fileSaveBuff(FMT("trace\\game_%d_cl_%d.raw", packetCounter, header.netID), data, header.size);
			packetCounter++;
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
					chara.characterID = 21013;
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
					weap.characterID = 21013;
					weap.weaponType = 1;
					weap.weaponIndex = 131101011;
					weap.grade = 0;
					weap.isUnlocked = 0;
					weap.isActivated = 0;

					packet.Write(weap);

					LOG("[client%03d] Server :: SN_ProfileWeapons :: ", clientID);
					SendPacketData(clientID, Sv::SN_ProfileWeapons::NET_ID, packet.size, packet.data);
				}

				// SN_LeaderCharacter
				Sv::SN_LeaderCharacter leader;
				leader.leaderID = 21013;
				leader.skinIndex = 0;
				SendPacket(clientID, leader);

				// SN_AccountInfo
				{
					u8 sendData[2048];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.WriteStringObj(L"LordSk"); // nick
					packet.Write<i32>(4); // inventoryLineCountTab0
					packet.Write<i32>(4); // inventoryLineCountTab1
					packet.Write<i32>(4); // inventoryLineCountTab2
					packet.Write<i32>(320080005); // displayTitlteIndex
					packet.Write<i32>(320080005); // statTitleIndex
					packet.Write<i32>(1); // warehouseLineCount
					packet.Write<i32>(-1); // tutorialState
					packet.Write<i32>(3600); // masterGearDurability
					packet.Write<u8>(0); // badgeType

					LOG("[client%03d] Server :: SN_AccountInfo :: ", clientID);
					SendPacketData(clientID, Sv::SN_AccountInfo::NET_ID, packet.size, packet.data);
				}

				// TODO: send SN_AccountExtraInfo
				// TODO: send SN_AccountEquipmentList
				// TODO: send SN_GuildChannelEnter

				// SN_ClientSettings
				// TODO: this is surely encrypted, find out what it represents
				{
					u8 sendData[2048];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u8>(0); // settingType
					packet.Write<u16>(1127); // dat_len

					const u8 settings0[1127] = {
						0x78, 0xDA, 0xAD, 0x9A, 0x5D, 0x73, 0xDA, 0x38, 0x14, 0x86, 0xAF, 0x37, 0xBF, 0x82, 0xF1, 0x7D, 0xB7, 0x18, 0x27, 0x7C, 0xCC, 0x84, 0x74, 0x84, 0x2D, 0x1B, 0x15, 0xC9, 0x72, 0x65, 0x09, 0x4A, 0x6E, 0x3C, 0xEC, 0x42, 0x5B, 0x66, 0x9B, 0xB0, 0x93, 0x90, 0xDD, 0xED, 0xBF, 0x5F, 0x91, 0xA4, 0x93, 0x63, 0x0C, 0x2E, 0xF8, 0x88, 0x3B, 0x18, 0x78, 0xF4, 0xBE, 0xE7, 0x48, 0x47, 0x3A, 0x32, 0xD7, 0x1F, 0xFE, 0xBB, 0xFB, 0xDE, 0xFA, 0x67, 0xF5, 0xF0, 0xB8, 0xDE, 0xDC, 0x0F, 0x3D, 0xFF, 0xF7, 0xB6, 0xD7, 0x5A, 0xDD, 0xFF, 0xB9, 0x59, 0xAE, 0xEF, 0xBF, 0x0E, 0xBD, 0xA7, 0xED, 0x97, 0x77, 0x7D, 0xEF, 0xC3, 0xCD, 0xC5, 0xF5, 0x84, 0xCE, 0x8B, 0x88, 0x68, 0xD2, 0xFA, 0xB6, 0xFE, 0xFA, 0x2D, 0x5A, 0x6C, 0x57, 0x7A, 0x7D, 0xB7, 0x1A, 0x7A, 0x41, 0xBB, 0x1B, 0x04, 0xED, 0xC0, 0xF7, 0x5A, 0xDF, 0x37, 0xFF, 0x82, 0x8F, 0x07, 0xFD, 0x6E, 0xB7, 0xDF, 0xF6, 0xFB, 0x1D, 0xAF, 0xB5, 0x7E, 0x0C, 0x9F, 0x1E, 0xB7, 0x9B, 0xBB, 0xA1, 0x67, 0xD1, 0x7F, 0xAD, 0x7E, 0xFC, 0xB1, 0x59, 0x3C, 0x2C, 0xF9, 0xE2, 0xC7, 0xE6, 0x69, 0x9B, 0x2E, 0x76, 0x5F, 0x6E, 0xEF, 0x5E, 0x7D, 0x3F, 0xF0, 0x6E, 0x2E, 0x7E, 0xBB, 0x5E, 0x2E, 0xB6, 0x8B, 0xD6, 0xFA, 0xFE, 0xEF, 0xA7, 0xED, 0xD0, 0x63, 0x69, 0x66, 0x74, 0x61, 0x18, 0x8D, 0x98, 0x16, 0x32, 0xA2, 0xCF, 0x3F, 0x1F, 0x7A, 0x62, 0x12, 0x16, 0xA9, 0xB4, 0x82, 0xBC, 0xD6, 0x9D, 0xD5, 0xF9, 0x65, 0xBD, 0x7A, 0x78, 0xF9, 0x30, 0x94, 0xA9, 0x56, 0x92, 0xEF, 0x86, 0x5C, 0x2C, 0x97, 0xCF, 0xDF, 0xB5, 0x43, 0xBE, 0x3F, 0x8C, 0xCD, 0x58, 0x9A, 0xFC, 0x0A, 0x28, 0x68, 0x6A, 0x4E, 0xA3, 0x71, 0xA2, 0x35, 0x09, 0x27, 0x00, 0xC8, 0x47, 0x46, 0x6B, 0x99, 0xEE, 0x23, 0x5F, 0xC7, 0x39, 0x85, 0x99, 0x10, 0x41, 0xF7, 0x54, 0x1E, 0x81, 0x9E, 0xAE, 0x33, 0x1F, 0x33, 0x05, 0x55, 0x2A, 0xB4, 0x4A, 0x55, 0x71, 0x7E, 0x84, 0x79, 0x56, 0x76, 0x46, 0x16, 0xB9, 0xE7, 0x5D, 0x61, 0xBD, 0x47, 0x24, 0x1F, 0x8F, 0x24, 0x51, 0x11, 0x80, 0x6A, 0x32, 0x42, 0x78, 0x0F, 0xC7, 0xE4, 0x59, 0xD1, 0x9B, 0x46, 0xAA, 0x8D, 0x4A, 0x91, 0xC4, 0x82, 0x70, 0x9E, 0x71, 0x32, 0xA7, 0xAA, 0x90, 0x69, 0x48, 0x7F, 0x89, 0xB7, 0x39, 0x8D, 0xF5, 0x69, 0x78, 0x9A, 0x87, 0x00, 0x67, 0xDF, 0x91, 0x8C, 0x22, 0xD4, 0x7E, 0x34, 0x22, 0x2B, 0x72, 0x12, 0xD3, 0xD8, 0x4A, 0x06, 0xE0, 0x3C, 0x23, 0x21, 0x86, 0xAB, 0xE8, 0xCE, 0x7F, 0x91, 0x48, 0x2D, 0x0B, 0xCE, 0xA6, 0x30, 0x04, 0x6D, 0x64, 0x70, 0x05, 0x09, 0x95, 0x2C, 0xDA, 0x75, 0xC4, 0xB3, 0x26, 0xEA, 0x8C, 0xA8, 0x58, 0x26, 0xBB, 0xEA, 0x54, 0xF8, 0x00, 0xEA, 0x3B, 0x91, 0x59, 0x4B, 0x3C, 0x4B, 0x26, 0x15, 0x52, 0x33, 0x99, 0x96, 0x8C, 0xFB, 0xCD, 0x97, 0x12, 0x70, 0xDD, 0x01, 0xC4, 0x8E, 0x13, 0xD7, 0xB5, 0xC4, 0x46, 0xAE, 0xFD, 0x3A, 0x62, 0x23, 0xD7, 0x01, 0x20, 0x06, 0x4E, 0x5C, 0xD7, 0x12, 0x1B, 0xB9, 0xEE, 0xD4, 0x11, 0x1B, 0xB9, 0xBE, 0x04, 0xC4, 0x4B, 0x27, 0xAE, 0x6B, 0x89, 0x67, 0xB9, 0x7E, 0x63, 0x5E, 0x01, 0xE6, 0x15, 0x8A, 0xA9, 0x65, 0x92, 0x70, 0x5A, 0x68, 0x26, 0xE8, 0xCF, 0x1F, 0x72, 0xAA, 0x00, 0xBE, 0xEB, 0x24, 0x08, 0xDD, 0x3A, 0x62, 0xC3, 0x20, 0xF4, 0x00, 0xB3, 0x87, 0x62, 0xBE, 0x16, 0x63, 0x21, 0xA7, 0x74, 0x54, 0xDE, 0xE8, 0xFB, 0x4E, 0xDC, 0xF7, 0xEB, 0x88, 0x4D, 0x94, 0x66, 0xC4, 0xE4, 0xD4, 0xBE, 0xC9, 0x8D, 0x80, 0x3B, 0xC7, 0xC0, 0x89, 0xDA, 0x41, 0x1D, 0xF1, 0x2C, 0xB5, 0x9F, 0x0C, 0x0B, 0x27, 0x39, 0x97, 0xBA, 0x54, 0x9E, 0x08, 0x42, 0x65, 0x3E, 0x61, 0x9C, 0x9B, 0xAC, 0x9E, 0xD7, 0x54, 0xA3, 0x15, 0xA2, 0x59, 0x3C, 0xAF, 0x43, 0x9F, 0x5E, 0x53, 0xDE, 0xB8, 0x32, 0xD3, 0x00, 0x89, 0x39, 0x8C, 0xBD, 0x31, 0x61, 0x29, 0x0D, 0x31, 0x59, 0xCF, 0xC7, 0x32, 0xAB, 0x63, 0x9D, 0x71, 0xF2, 0xCA, 0x35, 0x49, 0xE8, 0x4B, 0x86, 0xAA, 0x91, 0x0C, 0x9B, 0x47, 0x52, 0xB1, 0x64, 0x0C, 0x23, 0x18, 0x39, 0x89, 0x20, 0xDC, 0x3A, 0xA8, 0x83, 0x19, 0x59, 0xCB, 0x6B, 0x38, 0x23, 0x3B, 0xD5, 0x38, 0xD2, 0xE6, 0x71, 0x24, 0xE1, 0x6E, 0xCF, 0x04, 0xAC, 0x18, 0xD3, 0xB9, 0x19, 0xC6, 0x61, 0x97, 0x91, 0x20, 0x58, 0xDA, 0x68, 0xA9, 0x18, 0x81, 0xC7, 0xEB, 0x31, 0x02, 0xC7, 0xD2, 0x29, 0x4D, 0x2D, 0x11, 0x86, 0x8D, 0x21, 0x78, 0x82, 0xE5, 0xB9, 0x0D, 0x1C, 0x67, 0x39, 0x9C, 0x85, 0x1F, 0x11, 0xC4, 0xD4, 0xB6, 0xBD, 0x45, 0x44, 0x43, 0x09, 0x78, 0x13, 0x04, 0x2F, 0x56, 0x8C, 0xA6, 0xD1, 0x9E, 0x40, 0x8E, 0x99, 0xD4, 0xE1, 0x98, 0x46, 0x86, 0xC3, 0xBD, 0x45, 0x60, 0xFC, 0x42, 0xA3, 0x98, 0xDE, 0xD1, 0x96, 0x52, 0x9B, 0x89, 0x19, 0x4B, 0x23, 0x39, 0x03, 0x48, 0x89, 0xDB, 0xF7, 0x94, 0x5D, 0x19, 0xA5, 0x33, 0x4F, 0x86, 0xE0, 0x65, 0xB2, 0x94, 0x85, 0x0C, 0x51, 0x50, 0x39, 0x8D, 0x21, 0xEA, 0x93, 0x93, 0xBA, 0x67, 0x4A, 0x17, 0x0E, 0x0E, 0x0A, 0x5F, 0x3D, 0xB0, 0x61, 0xE5, 0x33, 0x49, 0xB5, 0xF4, 0xA9, 0xE6, 0xA5, 0x6F, 0xEF, 0x54, 0x97, 0x23, 0x6C, 0x13, 0xAD, 0x15, 0x1B, 0x19, 0x0D, 0xD7, 0x86, 0xC6, 0x14, 0x3F, 0xA6, 0x49, 0x5A, 0x90, 0x29, 0xD1, 0x44, 0x55, 0x3D, 0x6B, 0xC4, 0x3D, 0x98, 0x11, 0x82, 0x94, 0x8A, 0xA0, 0xC1, 0x4C, 0x6B, 0x62, 0x8B, 0xE0, 0x94, 0x56, 0x15, 0x4E, 0x9B, 0x2B, 0x84, 0xD6, 0x01, 0x71, 0x86, 0x90, 0x69, 0xD8, 0x6B, 0x47, 0x03, 0x78, 0x9F, 0x51, 0x73, 0x72, 0x4E, 0x73, 0xC0, 0x9A, 0xA3, 0xAA, 0xB4, 0x1D, 0x17, 0xB0, 0x6E, 0x51, 0xDB, 0x2F, 0x51, 0x11, 0xB3, 0xD1, 0x0B, 0x89, 0x80, 0xB5, 0xD5, 0x88, 0x8C, 0x44, 0x98, 0x0B, 0xA4, 0x19, 0xBB, 0xB5, 0xE4, 0x83, 0x58, 0xCC, 0x85, 0x0F, 0x89, 0x88, 0x38, 0xA2, 0x16, 0x73, 0xA3, 0xA2, 0x0C, 0x99, 0x1C, 0x84, 0x06, 0xE8, 0x15, 0x39, 0xE2, 0x86, 0xEE, 0xD0, 0xA5, 0x56, 0xE3, 0x05, 0x7E, 0xE9, 0x0E, 0xDE, 0xA9, 0xC0, 0xAF, 0xDC, 0xC1, 0x83, 0x0A, 0xBC, 0x8B, 0x86, 0x2B, 0x1A, 0x1D, 0x89, 0x4A, 0xCF, 0x19, 0xBB, 0x1A, 0x94, 0xBE, 0x33, 0x76, 0x35, 0x26, 0x03, 0xFC, 0xC1, 0x6B, 0x5E, 0x09, 0x49, 0xEC, 0xBB, 0xA2, 0xC2, 0x60, 0xC4, 0x1D, 0x57, 0x54, 0x18, 0x86, 0x38, 0x70, 0x45, 0x85, 0x57, 0x5D, 0xF1, 0xA5, 0x2B, 0x2A, 0xBC, 0xEC, 0x8A, 0x31, 0xCB, 0x83, 0xA6, 0x54, 0x1C, 0x48, 0x55, 0xD7, 0x09, 0xB2, 0x94, 0xA7, 0x9E, 0x13, 0x64, 0x29, 0x49, 0x7D, 0x27, 0xC8, 0x52, 0x86, 0x06, 0x4E, 0x90, 0xA5, 0xF4, 0xF8, 0xA8, 0x87, 0x17, 0x96, 0x66, 0x77, 0x09, 0x53, 0xEA, 0x04, 0x63, 0xDF, 0x47, 0x22, 0x89, 0x6D, 0x2F, 0x4B, 0x40, 0xCC, 0x2A, 0x9A, 0xB2, 0x88, 0x4A, 0x5B, 0x4D, 0x42, 0x69, 0xF7, 0xDF, 0xD2, 0xC3, 0xBB, 0x03, 0xDC, 0xF3, 0x2E, 0x36, 0xAD, 0xD4, 0x5B, 0x29, 0x45, 0xF5, 0x00, 0x23, 0xA9, 0x40, 0x3D, 0x6D, 0xD1, 0x8A, 0x44, 0x54, 0xD9, 0xBE, 0x38, 0xB1, 0x55, 0xD0, 0x2E, 0x2C, 0xBD, 0xC7, 0xCE, 0xB8, 0xC9, 0xF1, 0x78, 0xDB, 0x0F, 0xA4, 0xFB, 0x60, 0xAA, 0x98, 0x8C, 0xF0, 0x68, 0x41, 0xA3, 0xD2, 0x9C, 0xD8, 0xB1, 0x31, 0x49, 0xD4, 0x04, 0x26, 0x6E, 0x36, 0xA6, 0x94, 0xDB, 0x86, 0x32, 0xC5, 0x11, 0xAB, 0x47, 0xE3, 0xA3, 0xE0, 0x73, 0x1A, 0x17, 0xAD, 0x39, 0xE5, 0x32, 0xD9, 0x73, 0x8F, 0xDA, 0xC8, 0x5F, 0xAE, 0xFC, 0xED, 0x74, 0xD8, 0x5D, 0x43, 0xB0, 0x34, 0x96, 0x4A, 0x90, 0xBD, 0xAB, 0xA1, 0xDD, 0x18, 0x3D, 0xC7, 0x5D, 0xD2, 0xC1, 0xFF, 0x1F, 0x54, 0x98, 0xFE, 0x39, 0x59, 0x33, 0xD9, 0x89, 0xBC, 0xEB, 0xF7, 0x3F, 0xFF, 0xDD, 0x71, 0x73, 0xF1, 0x3F, 0xDA, 0x87, 0x40, 0x6C,
					};
					packet.WriteRaw(settings0, sizeof(settings0)); // dat_len

					LOG("[client%03d] Server :: SN_ClientSettings :: ", clientID);
					SendPacketData(clientID, Sv::SN_ClientSettings::NET_ID, packet.size, packet.data);
				}

				// TODO: send SN_ClientSettings 1 and 2
				// TODO: send SN_FriendList
				// TODO: send SN_PveComradeInfo
				// TODO: send SN_AchieveUpdate x5
				// TODO: send SN_FriendRequestList
				// TODO: send SN_BlockList
				// TODO: send SN_PveComradeInfo
				// TODO: send SN_MailUnreadNotice
				// TODO: send SA_TierRecord
				// TODO: send SN_WarehouseItems
				// TODO: send SN_MutualFriendList
				// TODO: send SA_GetGuildProfile
				// TODO: send SA_GetGuildMemberList
				// TODO: send SA_GetGuildHistoryList
				// TODO: send SN_GuildMemberStatus
				// TODO: send SA_GetGuildRankingSeasonList

			} break;

			case Cl::ReadyToLoadCharacter::NET_ID: {
				LOG("[client%03d] Client :: ReadyToLoadCharacter ::", clientID);

				// SN_Money
				Sv::SN_Money money;
				money.nMoney = 1;
				money.nReason = 1;
				LOG("[client%03d] Server :: SN_Money :: ", clientID);
				SendPacket(clientID, money);

				// SN_LoadCharacterStart
				LOG("[client%03d] Server :: SN_LoadCharacterStart :: ", clientID);
				SendPacketData(clientID, Sv::SN_LoadCharacterStart::NET_ID, 0, nullptr);

				// SN_PlayerSkillSlot
				{
					u8 sendData[2048];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<i32>(21013);

					packet.Write<u16>(7); // slotList_count

					// slotList[0]
					packet.Write<i32>(180350010); // skillIndex
					packet.Write<i32>(0); // coolTime
					packet.Write<u8>(1);  // unlocked
					packet.Write<u16>(1); // propList_count
					packet.Write<i32>(280351001); // skillPropertyIndex
					packet.Write<i32>(1); // level
					packet.Write<u8>(1); // isUnlocked
					packet.Write<u8>(1); // isActivated

					// slotList[1]
					packet.Write<i32>(180350030); // skillIndex
					packet.Write<i32>(0); // coolTime
					packet.Write<u8>(1);  // unlocked
					packet.Write<u16>(1); // propList_count
					packet.Write<i32>(280353001); // skillPropertyIndex
					packet.Write<i32>(1); // level
					packet.Write<u8>(1); // isUnlocked
					packet.Write<u8>(1); // isActivated

					// slotList[2]
					packet.Write<i32>(180350040); // skillIndex
					packet.Write<i32>(0); // coolTime
					packet.Write<u8>(1);  // unlocked
					packet.Write<u16>(0); // propList_count

					packet.Write<u8>(1); // isUnlocked
					packet.Write<u8>(1); // isActivated

					// slotList[3]
					packet.Write<i32>(180350020); // skillIndex
					packet.Write<i32>(0); // coolTime
					packet.Write<u8>(1);  // unlocked
					packet.Write<u16>(0); // propList_count
					packet.Write<u8>(0); // isUnlocked
					packet.Write<u8>(0); // isActivated

					// slotList[4]
					packet.Write<i32>(180350050); // skillIndex
					packet.Write<i32>(0); // coolTime
					packet.Write<u8>(1);  // unlocked
					packet.Write<u16>(0); // propList_count
					packet.Write<u8>(1); // isUnlocked
					packet.Write<u8>(1); // isActivated

					// slotList[5]
					packet.Write<i32>(180350000); // skillIndex
					packet.Write<i32>(0); // coolTime
					packet.Write<u8>(1);  // unlocked
					packet.Write<u16>(0); // propList_count
					packet.Write<u8>(1); // isUnlocked
					packet.Write<u8>(1); // isActivated

					// slotList[6]
					packet.Write<i32>(180350002); // skillIndex
					packet.Write<i32>(0); // coolTime
					packet.Write<u8>(1);  // unlocked
					packet.Write<u16>(0); // propList_count
					packet.Write<u8>(1); // isUnlocked
					packet.Write<u8>(1); // isActivated

					// slotList[7]
					packet.Write<i32>(-1);
					packet.Write<i32>(-1);
					packet.Write<i32>(-1);
					packet.Write<i32>(-1);
					packet.Write<i32>(-1);

					ASSERT(packet.size == 133); // TODO: remove

					LOG("[client%03d] Server :: SN_PlayerSkillSlot :: ", clientID);
					SendPacketData(clientID, Sv::SN_PlayerSkillSlot::NET_ID, packet.size, packet.data);
				}

				// SN_SetGameGvt
				Sv::SN_SetGameGvt gameGvt;
				gameGvt.sendTime = 0x6c3ed30;
				gameGvt.virtualTime = 0x6c3ed30;
				LOG("[client%03d] Server :: SN_SetGameGvt :: ", clientID);
				SendPacket(clientID, gameGvt);

				// SN_SummaryInfoAll
				{
					u8 sendData[128];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u16>(0);

					LOG("[client%03d] Server :: SN_SummaryInfoAll :: ", clientID);
					SendPacketData(clientID, Sv::SN_SummaryInfoAll::NET_ID, packet.size, packet.data);
				}

				// SN_AvailableSummaryRewardCountList
				{
					u8 sendData[128];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u16>(8);

					const i32 rewardCount[8] = {
						220004100,
						2,
						220002200,
						3,
						220005100,
						3,
						220003300,
						2
					};

					packet.WriteRaw(rewardCount, sizeof(rewardCount));

					LOG("[client%03d] Server :: SN_AvailableSummaryRewardCountList :: ", clientID);
					SendPacketData(clientID, Sv::SN_AvailableSummaryRewardCountList::NET_ID, packet.size, packet.data);
				}


				// SN_SummaryInfoLatest
				{
					u8 sendData[1024];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u16>(10);

					const Sv::SN_SummaryInfoLatest::Info infoList[10] = {
						{ 220002200, 200002201, 3, 220002201, 0, (i64)0xa6605c00 },
						{ 220002200, 200002201, 3, 220002204, 0, (i64)0xa6605c00 },
						{ 220002200, 200002201, 3, 220002205, 0, (i64)0xa6605c00 },
						{ 220005100, 200005101, 3, 220005101, 0, (i64)0x6a2bfc00 },
						{ 220005100, 200005101, 3, 220005102, 0, (i64)0x6a2bfc00 },
						{ 220005100, 200005101, 3, 220005105, 0, (i64)0x6a2bfc00 },
						{ 220003300, 200003301, 3, 220003301, 0, (i64)0xe0c7fa00 },
						{ 220003300, 200003301, 3, 220003305, 0, (i64)0xe0c7fa00 },
						{ 220004100, 200004101, 3, 220004101, 0, (i64)0x9eea8400 },
						{ 220004100, 200004101, 3, 220004105, 0, (i64)0x9eea8400 },
					};

					packet.WriteRaw(infoList, sizeof(infoList));

					LOG("[client%03d] Server :: SN_SummaryInfoLatest :: ", clientID);
					SendPacketData(clientID, Sv::SN_SummaryInfoLatest::NET_ID, packet.size, packet.data);
				}

				// SN_AchieveInfo
				{
					u8 sendData[1024];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u8>(1); // packetNum
					packet.Write<i32>(0); // achievementScore
					packet.Write<u16>(0); // achList_count

					LOG("[client%03d] Server :: SN_AchieveInfo :: ", clientID);
					SendPacketData(clientID, Sv::SN_AchieveInfo::NET_ID, packet.size, packet.data);
				}

				// SN_AchieveLatest
				{
					u8 sendData[1024];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u16>(0); // achList_count

					LOG("[client%03d] Server :: SN_AchieveLatest :: ", clientID);
					SendPacketData(clientID, Sv::SN_AchieveLatest::NET_ID, packet.size, packet.data);
				}

				// SN_CityMapInfo
				Sv::SN_CityMapInfo cityMapInfo;
				cityMapInfo.cityMapID = 160000042;
				SendPacket(clientID, cityMapInfo);

				// SQ_CityLobbyJoinCity
				SendPacketData(clientID, Sv::SQ_CityLobbyJoinCity::NET_ID, 0, nullptr);

			} break;

			case Cl::MapIsLoaded::NET_ID: {
				LOG("[client%03d] Client :: MapIsLoaded ::", clientID);

				// SN_GameCreateActor
				{
					u8 sendData[1024];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<i32>(21013); // objectID
					packet.Write<i32>(1); // nType
					packet.Write<i32>(100000017); // nIDX
					packet.Write<i32>(-1); // dwLocalID
					packet.Write(Vec3(11959.4f, 6451.76f, 3012)); // p3nPos
					packet.Write(Vec3(0, 0, 2.68874f)); // p3nDir
					packet.Write<i32>(0); // spawnType
					packet.Write<i32>(-1); // actionState
					packet.Write<i32>(0); // ownerID
					packet.Write<u8>(0); // bDirectionToNearPC
					packet.Write<i32>(-1); // AiWanderDistOverride
					packet.Write<i32>(-1); // tagID
					packet.Write<i32>(0); // faction
					packet.Write<i32>(17); // classType
					packet.Write<i32>(2); // skinIndex
					packet.Write<i32>(0); // seed

					typedef Sv::SN_GameCreateActor::BaseStat::Stat Stat;

					// initStat ------------------------
					packet.Write<u16>(53); // maxStats_count
					packet.Write(Stat{ 0, 2400 });
					packet.Write(Stat{ 35, 1000 });
					packet.Write(Stat{ 17, 0 });
					packet.Write(Stat{ 36, 0 });
					packet.Write(Stat{ 56, 0 });
					packet.Write(Stat{ 2, 200 });
					packet.Write(Stat{ 37, 120 });
					packet.Write(Stat{ 3, 0 });
					packet.Write(Stat{ 39, 5 });
					packet.Write(Stat{ 41, 0 });
					packet.Write(Stat{ 40, 0 });
					packet.Write(Stat{ 57, 0 });
					packet.Write(Stat{ 50, 0 });
					packet.Write(Stat{ 51, 0 });
					packet.Write(Stat{ 5, 5 });
					packet.Write(Stat{ 42, 0.6f });
					packet.Write(Stat{ 6, 0 });
					packet.Write(Stat{ 7, 93.75f });
					packet.Write(Stat{ 8, 0 });
					packet.Write(Stat{ 9, 3 });
					packet.Write(Stat{ 10, 150 });
					packet.Write(Stat{ 12, 0 });
					packet.Write(Stat{ 20, 0 });
					packet.Write(Stat{ 21, 0 });
					packet.Write(Stat{ 18, 100 });
					packet.Write(Stat{ 13, 100 });
					packet.Write(Stat{ 14, 98 });
					packet.Write(Stat{ 15, 100 });
					packet.Write(Stat{ 52, 100 });
					packet.Write(Stat{ 16, 1 });
					packet.Write(Stat{ 27, 0 });
					packet.Write(Stat{ 47, 0 });
					packet.Write(Stat{ 49, 0 });
					packet.Write(Stat{ 48, 0 });
					packet.Write(Stat{ 29, 20 });
					packet.Write(Stat{ 23, 9 });
					packet.Write(Stat{ 44, 15 });
					packet.Write(Stat{ 46, 0 });
					packet.Write(Stat{ 45, 0 });
					packet.Write(Stat{ 26, 0 });
					packet.Write(Stat{ 25, 0 });
					packet.Write(Stat{ 31, 14 });
					packet.Write(Stat{ 22, 2 });
					packet.Write(Stat{ 54, 15 });
					packet.Write(Stat{ 60, 0 });
					packet.Write(Stat{ 61, 0 });
					packet.Write(Stat{ 62, 0 });
					packet.Write(Stat{ 63, 3 });
					packet.Write(Stat{ 64, 150 });
					packet.Write(Stat{ 53, 0 });
					packet.Write(Stat{ 58, 0 });
					packet.Write(Stat{ 65, 0 });
					packet.Write(Stat{ 55, 15 });

					packet.Write<u16>(2); // curStats_count
					packet.Write(Stat{ 0, 2400 });
					packet.Write(Stat{ 2, 200 });
					// ------------------------------------

					packet.Write<u8>(1); // isInSight
					packet.Write<u8>(0); // isDead
					packet.Write<i64>((i64)0x6cf3fe57); // serverTime

					packet.Write<u16>(0); // meshChangeActionHistory_count

					ASSERT(packet.size == 368); // TODO: remove

					LOG("[client%03d] Server :: SN_GameCreateActor :: ", clientID);
					SendPacketData(clientID, Sv::SN_GameCreateActor::NET_ID, packet.size, packet.data);
				}

				// SN_GamePlayerStock
				{
					u8 sendData[1024];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<i32>(21013); // playerID
					packet.WriteStringObj(L"LordSk"); // name
					packet.Write<i32>(35); // class_
					packet.Write<i32>(320080005); // displayTitleIDX
					packet.Write<i32>(320080005); // statTitleIDX
					packet.Write<u8>(0); // badgeType
					packet.Write<u8>(0); // badgeTierLevel
					packet.WriteStringObj(L"XMX"); // guildTag
					packet.Write<u8>(0); // vipLevel
					packet.Write<u8>(0); // staffType
					packet.Write<u8>(0); // isSubstituted

					LOG("[client%03d] Server :: SN_GamePlayerStock :: ", clientID);
					SendPacketData(clientID, Sv::SN_GamePlayerStock::NET_ID, packet.size, packet.data);
				}

				// SN_PlayerStateInTown
				{
					u8 sendData[1024];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<i32>(21013); // playerID
					packet.Write<u8>(0); // playerStateInTown
					packet.Write<u16>(0); // matchingGameModes_count

					LOG("[client%03d] Server :: SN_PlayerStateInTown :: ", clientID);
					SendPacketData(clientID, Sv::SN_PlayerStateInTown::NET_ID, packet.size, packet.data);
				}

				// SN_GamePlayerEquipWeapon
				{
					u8 sendData[1024];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<i32>(21013); // characterID
					packet.Write<i32>(131135012); // weaponDocIndex
					packet.Write<i32>(0); // additionnalOverHeatGauge
					packet.Write<i32>(0); // additionnalOverHeatGaugeRatio

					LOG("[client%03d] Server :: SN_GamePlayerEquipWeapon :: ", clientID);
					SendPacketData(clientID, Sv::SN_GamePlayerEquipWeapon::NET_ID, packet.size, packet.data);
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

#ifdef CONF_DEBUG
			fileSaveBuff(FMT("trace\\game_%d_sv_%d.raw", packetCounter, header.netID), sendBuff, header.size);
			packetCounter++;
#endif
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
	bool r = server.Init();
	if(!r) {
		LOG("ERROR: failed to initialize server");
		return 1;
	}
	g_Server = &server;

	static Game game;
	game.Init(&server);

	// accept thread
	while(server.running) {
		// Accept a client socket
		LOG("Waiting for a connection...");
		struct sockaddr clientAddr;
		int addrLen = sizeof(sockaddr);
		SOCKET clientSocket = accept(server.serverSocket, &clientAddr, &addrLen);
		if(clientSocket == INVALID_SOCKET) {
			LOG("ERROR(accept): failed: %d", WSAGetLastError());
			return 1;
		}

		LOG("New connection (%s)", GetIpString(clientAddr));
		server.AddClient(clientSocket, clientAddr);
	}

	LOG("Done.");

	return 0;
}
