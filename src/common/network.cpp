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
#ifdef CONF_WINDOWS
	ProfileSetThreadName("NetworkCompletion");
#else
	ProfileSetThreadName("NetworkPoller");
#endif
#ifndef CONF_WINDOWS
	ThreadSetCoreAffinity((i32)CoreAffinity::NETWORK);
#endif

	Server& server = *(Server*)pData;

#ifdef CONF_WINDOWS
	while(!server.completionStopping || server.pendingIO != 0) server.Update();
#else
	while(server.running) server.Update();
#endif

	return 0;
}

bool Server::Init()
{
	ASSERT(!initialized);
	if(!NetworkInit()) return false;
	initialized = true;
	clientIsConnected.fill(0);
	clientDoDisconnect.fill(false);
	for(int i = 0; i < MAX_CLIENTS; ++i) {
		clientSocket[i] = INVALID_SOCKET;
		ClientNet& client = clientNet[i];
		client.handle = ClientHandle::INVALID;
#ifndef CONF_WINDOWS
		client.async.Init();
#endif
	}
#ifdef CONF_WINDOWS
	completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);
	if(!completionPort) {
		NetworkCleanup();
		initialized = false;
		return false;
	}
	pendingIO = 0;
	completionStopping = false;
#endif
	running = 1;
	thread.Begin(ThreadNetwork, this);
	return true;
}

void Server::Stop()
{
	if(!initialized) return;
	running = 0;
#ifdef CONF_WINDOWS
	PostControl(ClientHandle::INVALID, 2);
#endif
}

void Server::Cleanup()
{
	if(!initialized) return;
	Stop();
	thread.WaitForEnd();
#ifdef CONF_WINDOWS
	CloseHandle(completionPort);
	completionPort = nullptr;
#else
	for(int i = 0; i < MAX_CLIENTS; ++i) {
		ClientNet& client = clientNet[i];
		LOCK_MUTEX(client.mutexConnect);
		DisconnectClient(i);
	}
#endif
	NetworkCleanup();
	initialized = false;
}

ClientHandle Server::ListenerAddClient(SOCKET s, const sockaddr& addr_)
{
	for(int clientID = 0; clientID < MAX_CLIENTS; ++clientID) {
		ClientNet& client = clientNet[clientID];
		LOCK_MUTEX(client.mutexConnect);
		if(!running) break;
		if(clientIsConnected[clientID] != 0) continue;
		ASSERT(clientSocket[clientID] == INVALID_SOCKET);
		clientSocket[clientID] = s;
		client.addr = addr_;
		if(!client.recvPendingProcessingBuff.data) client.recvPendingProcessingBuff.Init(RECV_BUFF_LEN * 4);
		client.recvPendingProcessingBuff.Clear();
		if(!client.pendingSendBuff.data) client.pendingSendBuff.Init(SEND_BUFF_LEN * 4);
		client.pendingSendBuff.Clear();
#ifdef CONF_WINDOWS
		if(!client.sendingBuff.data) client.sendingBuff.Init(SEND_BUFF_LEN);
		client.sendingBuff.Clear();
		client.sendCursor = 0;
		client.recvPending = false;
		client.sendPending = false;
		client.sendWakeQueued = false;
		if(!CreateIoCompletionPort((HANDLE)s, completionPort, ULONG_PTR(clientID + 1), 0)) {
			closesocket(s);
			clientSocket[clientID] = INVALID_SOCKET;
			return ClientHandle::INVALID;
		}
#else
		client.async.PostConnectionInit(s);
#endif
		ClientInfo& info = clientInfo[clientID];
		const auto& address = (const sockaddr_in&)client.addr;
		const u8* ip = (const u8*)&address.sin_addr;
		SetIp(info.ip.data(), ip[0], ip[1], ip[2], ip[3]);
		info.port = address.sin_port;
		ClientHandle handle;
		{
			LOCK_MUTEX(mutexClientMap);
			handle = nextclientHd;
			nextclientHd = ClientHandle((u32)nextclientHd + 1);
			ASSERT(handle != ClientHandle::INVALID);
			clientHandle2IDMap[handle] = clientID;
		}
		client.handle = handle;
		clientDoDisconnect[clientID] = false;
		clientIsConnected[clientID] = 1;
		{
			LOCK_MUTEX(mutexClientConnectedList);
			clientConnectedList.push_back({ handle, info });
		}
#ifdef CONF_WINDOWS
		if(!SubmitReceive(clientID)) DisconnectClient(clientID);
#else
		ClientStartReceiving(clientID);
#endif
		return handle;
	}
	closesocket(s);
	return ClientHandle::INVALID;
}

void Server::DisconnectClient(ClientHandle handle)
{
	const i32 clientID = TryGetClientID(handle);
	if(clientID == -1) return;
	ClientNet& client = clientNet[clientID];
	LOCK_MUTEX(client.mutexConnect);
	if(clientIsConnected[clientID] != 1 || client.handle != handle) return;
	clientDoDisconnect[clientID] = true;
#ifdef CONF_WINDOWS
	PostControl(handle, 1);
#endif
}

void Server::ClientSend(i32 clientID, ClientHandle handle, const void* data, i32 dataSize)
{
	ClientNet& client = clientNet[clientID];
	LOCK_MUTEX(client.mutexConnect);
	if(!running || clientIsConnected[clientID] != 1 || client.handle != handle) return;
	{
		LOCK_MUTEX(client.mutexSend);
		client.pendingSendBuff.Append(data, dataSize);
	}
#ifdef CONF_WINDOWS
	if(!client.sendPending && !client.sendWakeQueued) {
		client.sendWakeQueued = true;
		PostControl(handle, 0);
	}
#endif
}

#ifndef CONF_WINDOWS
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

		if(clientDoDisconnect[clientID]) {
			DisconnectClient(clientID);
			continue;
		}

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
#endif

void Server::TransferClientReceivedData(GrowableBuffer* out, i32 clientID, ClientHandle expected)
{
	ClientNet& client = clientNet[clientID];
	LOCK_MUTEX(client.mutexConnect);
	if(clientIsConnected[clientID] != 1 || (expected != ClientHandle::INVALID && client.handle != expected)) return;
	bool malformed = false;
	{
		LOCK_MUTEX(client.mutexRecv);
		const u8* data = client.recvPendingProcessingBuff.data;
		const i32 size = client.recvPendingProcessingBuff.size;
		i32 complete = 0;
		while(size - complete >= i32(sizeof(NetHeader))) {
			u16 packetSize;
			memmove(&packetSize, data + complete, sizeof(packetSize));
			if(packetSize < sizeof(NetHeader)) { malformed = true; break; }
			if(packetSize > size - complete) break;
			complete += packetSize;
		}
		if(complete != 0) {
			RecvChunkHeader header{ client.handle, complete };
			out->Append(&header, sizeof(header));
			out->Append(data, complete);
			const i32 remaining = size - complete;
			if(remaining != 0) memmove(client.recvPendingProcessingBuff.data, data + complete, remaining);
			client.recvPendingProcessingBuff.size = remaining;
		}
	}
	if(malformed) DisconnectClient(clientID);
}

void Server::TransferAllReceivedData(GrowableBuffer* out)
{
	for(int clientID = 0; clientID < MAX_CLIENTS; ++clientID) {
		TransferClientReceivedData(out, clientID, ClientHandle::INVALID);
	}
}

void Server::TransferReceivedData(GrowableBuffer* out, const ClientHandle* clientList, u32 clientCount)
{
	for(u32 i = 0; i < clientCount; ++i) {
		const i32 clientID = TryGetClientID(clientList[i]);
		if(clientID != -1) TransferClientReceivedData(out, clientID, clientList[i]);
	}
}

#ifndef CONF_WINDOWS
bool Server::ClientStartReceiving(i32 clientID)
{
	if(clientNet[clientID].async.StartReceiving()) return true;
	DisconnectClient(clientID);
	return false;
}
#endif

void Server::ClientHandleReceivedData(i32 clientID, i32 dataLen)
{
	ClientNet& client = clientNet[clientID];
	LOCK_MUTEX(client.mutexRecv);
#ifdef CONF_WINDOWS
	client.recvPendingProcessingBuff.Append(client.recvBuff, dataLen);
#else
	client.recvPendingProcessingBuff.Append(client.async.GetReceivedData(), dataLen);
#endif
}

void Server::SendPacketData(ClientHandle clientHd, u16 netID, u16 packetSize, const void* packetData)
{
	const i32 clientID = TryGetClientID(clientHd);
	if(clientID == -1) return;

	const i32 packetTotalSize = packetSize+sizeof(NetHeader);
	u8 sendBuff[8192];
	ASSERT(packetTotalSize <= sizeof(sendBuff));

	NetHeader header;
	header.size = packetTotalSize;
	header.netID = netID;
	memmove(sendBuff, &header, sizeof(header));
	memmove(sendBuff+sizeof(NetHeader), packetData, packetSize);

	ClientSend(clientID, clientHd, sendBuff, packetTotalSize);

	if(doTraceNetwork) {
		static Mutex mutexFile;
		mutexFile.Lock();
		fileSaveBuff(FormatPath(FMT("trace/game_%d_sv_%d.raw", packetCounter, header.netID)), sendBuff, header.size);
		packetCounter++;
		mutexFile.Unlock();
	}
}

void Server::DisconnectClient(i32 clientID)
{
	if(clientIsConnected[clientID] != 1) return;
	ClientNet& client = clientNet[clientID];
	const ClientHandle handle = client.handle;
	clientIsConnected[clientID] = 2;
	{
		LOCK_MUTEX(mutexClientMap);
		clientHandle2IDMap.erase(handle);
	}
	closesocket(clientSocket[clientID]);
	clientSocket[clientID] = INVALID_SOCKET;
#ifdef CONF_WINDOWS
	RetireClient(clientID);
#else
	{
		LOCK_MUTEX(mutexClientDisconnectedList);
		clientDisconnectedList.push_back(handle);
	}
	client.async.Reset();
	client.handle = ClientHandle::INVALID;
	clientIsConnected[clientID] = 0;
#endif
	LOG("[client%x] disconnected", handle);
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

void InnerConnection::SendPacketData(u16 netID, u16 packetSize, const void *packetData)
{
	const i32 packetTotalSize = packetSize + sizeof(NetHeader);
	ASSERT(packetTotalSize + sendQ.size <= InnerConnection::SendQueue::QUEUE_CAPACITY);

	NetHeader header;
	header.size = packetTotalSize;
	header.netID = netID;

	u8* at = sendQ.data + sendQ.size;
	memmove(at, &header, sizeof(header));
	memmove(at+sizeof(NetHeader), packetData, packetSize);
	sendQ.size += packetTotalSize;
}

void InnerConnection::SendPendingData()
{
	NetPollResult r = async.PollSend();
	if(r == NetPollResult::SUCCESS) {
		async.PushSendData(sendQ.data, sendQ.size);
		async.StartSending();
		sendQ.size = 0;
	}
}

void InnerConnection::RecvPendingData(u8* buff, const i32 buffCapacity, i32* size)
{
	*size = 0;

	i32 len = 0;
	NetPollResult r = async.PollReceive(&len);
	if(r == NetPollResult::SUCCESS) {
		ASSERT(len <= buffCapacity);
		memmove(buff, async.GetReceivedData(), len);
		*size = len;
		async.StartReceiving();
	}
}
