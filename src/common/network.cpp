#include "network.h"
#include "protocol.h"
#include <EAThread/eathread_thread.h>

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

	server.Cleanup();
	return 0;
}

bool Server::Init(const char* listenPort)
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
	iResult = getaddrinfo(NULL, listenPort, &hints, &result);
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

	memset(clientIsConnected, 0, sizeof(clientIsConnected));

	for(int i = 0; i < MAX_CLIENTS; i++) {
		clientSocket[i] = INVALID_SOCKET;
		ClientNet& client = clientNet[i];
		client.hEventRecv = WSA_INVALID_EVENT;
		client.hEventSend = WSA_INVALID_EVENT;
	}

	EA::Thread::Thread Thread;
	Thread.Begin(ThreadNetwork, this);
	running = true;
	return true;
}

void Server::Cleanup()
{
	LOG("Server shutting down...");

	closesocket(serverSocket);
	WSACleanup();
}

// NOTE: this is called from the Main thread (listen thread)
i32 Server::AddClient(SOCKET s, const sockaddr& addr_)
{
	for(int clientID = 0; clientID < MAX_CLIENTS; clientID++) {
		if(clientIsConnected[clientID] == 0) {
			ClientNet& client = clientNet[clientID];
			const LockGuard lock(client.mutexConnect);

			ASSERT(clientSocket[clientID] == INVALID_SOCKET);

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

			memset(&client.sendOverlapped, 0, sizeof(client.sendOverlapped));
			client.sendOverlapped.hEvent = client.hEventSend;

			 // register the socket at the end, when everything is initialized
			// TODO: add a variable clientIsInitialized?

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

			clientIsConnected[clientID] = 1;
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
	const LockGuard lock(client.mutexConnect);

	closesocket(clientSocket[clientID]);
	clientSocket[clientID] = INVALID_SOCKET;

	WSAResetEvent(client.hEventRecv);
	WSAResetEvent(client.hEventSend);

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
#ifdef CONF_LOG_TRAFFIC_BYTELEN
				LOG("[client%03d] Sent %d bytes", clientID, len);
#endif
				ASSERT(len == client.sendingBuff.size);
				client.sendingBuff.Clear();
			}

			memset(&client.sendOverlapped, 0, sizeof(client.sendOverlapped));
			client.sendOverlapped.hEvent = client.hEventSend;

			if(client.pendingSendBuff.size > 0) {
				{
					LockGuard lock(client.mutexSend);
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

	memset(&client.recvOverlapped, 0, sizeof(client.recvOverlapped));
	WSAResetEvent(client.hEventRecv);
	client.recvOverlapped.hEvent = client.hEventRecv;

	WSABUF buff;
	buff.len = RECV_BUFF_LEN;
	buff.buf = client.recvBuff;

	DWORD len;
	DWORD flags = 0;
	int r = WSARecv(sock, &buff, 1, &len, &flags, &client.recvOverlapped, NULL);
	if(r == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
		LOG("ERROR(ClientStartReceiving): receive failed (%d)", WSAGetLastError());
		DisconnectClient(clientID);
		return false;
	}
	/*else if(r == 0) { // instant reception, handle it
		bool hr = ClientHandleReceivedData(clientID, len);
		if(!hr) return false;

		// start receiving again
		return ClientStartReceiving(clientID);
	}*/

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
	client.recvPendingProcessingBuff.Append(client.recvBuff, dataLen);
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
