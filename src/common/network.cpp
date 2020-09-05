#include "network.h"
#include "protocol.h"

const char* IpToString(const u8* ip)
{
	return FMT("%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}

const void SetIp(u8* ip, u8 i0, u8 i1, u8 i2, u8 i3)
{
	ip[0] = i0;
	ip[1] = i1;
	ip[2] = i2;
	ip[3] = i3;
}

const char* GetIpString(const sockaddr& addr)
{
	const sockaddr_in& in = *(sockaddr_in*)&addr;
	return FMT("%s:%d", inet_ntoa(in.sin_addr), in.sin_port);
}

intptr_t ThreadNetwork(void* pData)
{
	Server& server = *(Server*)pData;

	while(server.running) {
		server.Update();
	}

	return 0;
}

bool Server::Init(const char* listenPort)
{
	if(!NetworkInit()) return false;

	struct addrinfo *result = NULL, *ptr = NULL, hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the local address and port to be used by the server
	int iResult = getaddrinfo(NULL, listenPort, &hints, &result);
	if (iResult != 0) {
		LOG("ERROR: getaddrinfo failed: %d", iResult);
		return false;
	}
	defer(freeaddrinfo(result));

	serverSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if(serverSocket == INVALID_SOCKET) {
		LOG("ERROR(socket): %d", NetworkGetLastError());
		return false;
	}

	// Setup the TCP listening socket
	iResult = bind(serverSocket, result->ai_addr, (int)result->ai_addrlen);
	if(iResult == SOCKET_ERROR) {
		LOG("ERROR(bind): failed with error: %d", NetworkGetLastError());
		return false;
	}

	// listen
	if(listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
		LOG("ERROR(listen): failed with error: %d", NetworkGetLastError());
		return false;
	}

	memset(&clientIsConnected, 0, sizeof(clientIsConnected));

	for(int i = 0; i < MAX_CLIENTS; i++) {
		clientSocket[i] = INVALID_SOCKET;
		ClientNet& client = clientNet[i];
		client.async.Init();
	}

	thread.Begin(ThreadNetwork, this);
	running = true;
	return true;
}

void Server::Cleanup()
{
	NetworkCleanup();
}

// NOTE: this is called from the Main thread (listen thread)
i32 Server::AddClient(SOCKET s, const sockaddr& addr_)
{
	for(int clientID = 0; clientID < MAX_CLIENTS; clientID++) {
		if(clientIsConnected[clientID] == 0) {
			ClientNet& client = clientNet[clientID];
			const LockGuard lock(client.mutexConnect);

			ASSERT(clientSocket[clientID] == INVALID_SOCKET);

			clientSocket[clientID] = s;

			client.addr = addr_;

			if(client.recvPendingProcessingBuff.data == nullptr) {
				client.recvPendingProcessingBuff.Init(RECV_BUFF_LEN * 4);
			}
			client.recvPendingProcessingBuff.Clear();

			if(client.pendingSendBuff.data == nullptr) {
				client.pendingSendBuff.Init(SEND_BUFF_LEN * 4);
			}
			client.pendingSendBuff.Clear();

			client.async.PrepareForNewConnection(s);

			// start receiving
			bool r = ClientStartReceiving(clientID);
			if(!r) {
				continue;
			}

			ClientInfo& info = clientInfo[clientID];
			struct sockaddr_in& sin = *(struct sockaddr_in*)&client.addr;
			const u8* clIp = (u8*)&sin.sin_addr;
			SetIp(info.ip, clIp[0], clIp[1], clIp[2], clIp[3]);
			info.port = htons(sin.sin_port);

			clientIsConnected[clientID] = 1; // register the socket at the end, when everything is initialized
			return clientID;
		}
	}

	ASSERT(0); // no space
	return -1;
}

void Server::DisconnectClient(i32 clientID)
{
	ASSERT(clientID >= 0 && clientID < MAX_CLIENTS);
	if(clientIsConnected[clientID] == 0) return;

	mutexClientDisconnectedList.Lock();
	clientDisconnectedList.push_back(clientID);
	mutexClientDisconnectedList.Unlock();

	ClientNet& client = clientNet[clientID];
	const LockGuard lock(client.mutexConnect);

	closesocket(clientSocket[clientID]);
	clientSocket[clientID] = INVALID_SOCKET;

	client.async.Reset();

	clientIsConnected[clientID] = 0;
	LOG("[client%03d] disconnected", clientID);
}

void Server::ClientSend(i32 clientID, const void* data, i32 dataSize)
{
	ASSERT(clientID >= 0 && clientID < MAX_CLIENTS);
	if(clientSocket[clientID] == INVALID_SOCKET) return;

	ClientNet& client = clientNet[clientID];
	const LockGuard lock(client.mutexSend);
	client.pendingSendBuff.Append(data, dataSize);
}

// NOTE: this is called from the Poller thread
void Server::Update()
{
	for(int clientID = 0; clientID < MAX_CLIENTS; clientID++) {
		if(clientIsConnected[clientID] == 0) continue;

		SOCKET sock = clientSocket[clientID];
		ASSERT(sock != INVALID_SOCKET);

		ClientNet& client = clientNet[clientID];

		i32 len = 0;
		NetPollResult r = client.async.PollReceive(&len);
		if(r == NetPollResult::POLL_ERROR) {
			DisconnectClient(clientID);
			continue;
		}
		else if(r == NetPollResult::SUCCESS) {
			bool r = ClientHandleReceivedData(clientID, len);
			if(!r) {
				continue;
			}

			// start receiving again
			r = ClientStartReceiving(clientID);
			if(!r) {
				continue;
			}
		}

		r = client.async.PollSend();
		if(r == NetPollResult::POLL_ERROR) {
			DisconnectClient(clientID);
			continue;
		}
		else if(r == NetPollResult::SUCCESS) {
			if(client.pendingSendBuff.size > 0) {
				{
					LockGuard lock(client.mutexSend);
					client.async.PushSendData(client.pendingSendBuff.data, client.pendingSendBuff.size);
					client.pendingSendBuff.Clear();
				}

				bool r = client.async.StartSending();
				if(!r) {
					LOG("[client%03d] ERROR: send failed", clientID);
					DisconnectClient(clientID);
				}
			}
		}
	}
}

void Server::TransferAllReceivedData(GrowableBuffer* out)
{
	for(int clientID = 0; clientID < MAX_CLIENTS; clientID++) {
		if(clientSocket[clientID] == INVALID_SOCKET) continue;
		ClientNet& client = clientNet[clientID];

		{
			const LockGuard lock(client.mutexRecv);

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

bool Server::ClientStartReceiving(i32 clientID)
{
	DBG_ASSERT(clientID >= 0 && clientID < MAX_CLIENTS);
	SOCKET sock = clientSocket[clientID];
	ClientNet& client = clientNet[clientID];

	bool r = client.async.StartReceiving();
	if(!r) {
		LOG("[client%03d]  ERROR(ClientStartReceiving): receive failed", clientID);
		DisconnectClient(clientID);
		return false;
	}

	return true;
}

bool Server::ClientHandleReceivedData(i32 clientID, i32 dataLen)
{
	DBG_ASSERT(clientID >= 0 && clientID < MAX_CLIENTS);
	ClientNet& client = clientNet[clientID];

#ifdef CONF_LOG_TRAFFIC_BYTELEN
	LOG("[client%03d] Received %d bytes", clientID, dataLen);
#endif
	if(dataLen == 0) {
		DisconnectClient(clientID);
		return false;
	}

	// append to pending processing buffer
	const LockGuard lock(client.mutexRecv);
	client.recvPendingProcessingBuff.Append(client.async.GetReceivedData(), dataLen);
	return true;
}

void Server::SendPacketData(i32 clientID, u16 netID, u16 packetSize, const void* packetData)
{
	const i32 packetTotalSize = packetSize+sizeof(NetHeader);
	u8 sendBuff[8192];
	ASSERT(packetTotalSize <= sizeof(sendBuff));

	NetHeader header;
	header.size = packetTotalSize;
	header.netID = netID;
	memmove(sendBuff, &header, sizeof(header));
	memmove(sendBuff+sizeof(NetHeader), packetData, packetSize);

	ClientSend(clientID, sendBuff, packetTotalSize);

#ifdef CONF_DEBUG
	static Mutex mutexFile;
	mutexFile.Lock();
	fileSaveBuff(FMT("trace\\game_%d_sv_%d.raw", packetCounter, header.netID), sendBuff, header.size);
	packetCounter++;
	mutexFile.Unlock();
#endif
}
