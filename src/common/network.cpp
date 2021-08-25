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
	char buff[256];
	inet_ntop(AF_INET, &in.sin_addr, buff, sizeof(buff));
	return FMT("%s:%d", buff, in.sin_port);
}

intptr_t ThreadNetwork(void* pData)
{
	ProfileSetThreadName("NetworkPoller");
	const i32 cpuID = 1;
	EA::Thread::SetThreadAffinityMask(1 << cpuID);

	Server& server = *(Server*)pData;

	while(server.running) {
		server.Update();
	}

	return 0;
}

bool Server::Init()
{
	if(!NetworkInit()) return false;

	clientIsConnected.fill(0);

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

// NOTE: this is called from listeners
i32 Server::ListenerAddClient(SOCKET s, const sockaddr& addr_)
{
	for(int clientID = 0; clientID < MAX_CLIENTS; clientID++) {
		if(clientIsConnected[clientID] == 0) {
			ClientNet& client = clientNet[clientID];
			LOCK_MUTEX(client.mutexConnect);

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

			client.async.PostConnectionInit(s);

			// start receiving
			bool r = ClientStartReceiving(clientID);
			if(!r) {
				return -1;
			}

			ClientInfo& info = clientInfo[clientID];
			struct sockaddr_in& sin = *(struct sockaddr_in*)&client.addr;
			const u8* clIp = (u8*)&sin.sin_addr;
			SetIp(info.ip, clIp[0], clIp[1], clIp[2], clIp[3]);
			//info.port = htons(sin.sin_port);
			info.port = sin.sin_port;

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

	ClientNet& client = clientNet[clientID];
	LOCK_MUTEX(client.mutexConnect);

	{
		const LockGuard lock(mutexClientDisconnectedList);
		clientDisconnectedList.push_back(clientID);
	}

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
		if(clientIsConnected[clientID] == 0) continue; // first check for speed

		ClientNet& client = clientNet[clientID];
		LOCK_MUTEX(client.mutexConnect);
		if(clientIsConnected[clientID] == 0) continue; // second check to be certain

		SOCKET sock = clientSocket[clientID];
		ASSERT(sock != INVALID_SOCKET);

		i32 len = 0;
		NetPollResult r = client.async.PollReceive(&len);
		if(r == NetPollResult::POLL_ERROR) {
			DisconnectClient(clientID);
			continue;
		}
		else if(r == NetPollResult::SUCCESS) {
			ClientHandleReceivedData(clientID, len);

			// start receiving again
			bool r = ClientStartReceiving(clientID);
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
			LOCK_MUTEX(client.mutexRecv);

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

void Server::ClientHandleReceivedData(i32 clientID, i32 dataLen)
{
	ProfileFunction();

	DBG_ASSERT(clientID >= 0 && clientID < MAX_CLIENTS);
	ClientNet& client = clientNet[clientID];

#ifdef CONF_LOG_TRAFFIC_BYTELEN
	LOG("[client%03d] Received %d bytes", clientID, dataLen);
#endif

	// append to pending processing buffer
	LOCK_MUTEX(client.mutexRecv);
	client.recvPendingProcessingBuff.Append(client.async.GetReceivedData(), dataLen);
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

	if(doTraceNetwork) {
		static Mutex mutexFile;
		mutexFile.Lock();
		fileSaveBuff(FormatPath(FMT("trace/game_%d_sv_%d.raw", packetCounter, header.netID)), sendBuff, header.size);
		packetCounter++;
		mutexFile.Unlock();
	}
}

bool Listener::Init(i32 listenPort_)
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

void Listener::Stop()
{
	closesocket(listenSocket);
	listenSocket = INVALID_SOCKET;
}

void Listener::Listen()
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
		server.ListenerAddClient(clientSocket, clientAddr);
	}
}
