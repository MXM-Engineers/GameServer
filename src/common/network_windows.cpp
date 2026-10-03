#ifdef CONF_WINDOWS
#include "network.h"

bool NetworkInit()
{
	// Initialize Winsock
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if(iResult != 0) {
		LOG("ERROR(NetworkInit): WSAStartup failed: %d", iResult);
		return false;
	}
	return true;
}

void NetworkCleanup()
{
	WSACleanup();
}

int NetworkGetLastError()
{
	return WSAGetLastError();
}

void AsyncConnection::Init()
{
	sock = INVALID_SOCKET;
	hEventRecv = WSA_INVALID_EVENT;
	hEventSend = WSA_INVALID_EVENT;
}

void AsyncConnection::Reset()
{
	sock = INVALID_SOCKET;
	WSAResetEvent(hEventRecv);
	WSAResetEvent(hEventSend);
}

bool AsyncConnection::ConnectTo(const u8* ip, u16 port)
{
	Init();

	sockaddr_in addr = {0};
	memmove(&addr.sin_addr.s_addr, ip, 4);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	ASSERT(s != INVALID_SOCKET);

	int r = connect(s, (sockaddr*)&addr, sizeof(addr));
	if(r == SOCKET_ERROR) return false;

	PostConnectionInit(s);
	return true;
}

void AsyncConnection::PostConnectionInit(SOCKET s)
{
	sock = s;

	// set non blocking
	u_long NonBlocking = true;
	int ior = ioctlsocket(s, FIONBIO, &NonBlocking);
	if(ior != NO_ERROR) {
		LOG("ERROR(socket=%x): failed to change io mode (%d)", (u32)s, WSAGetLastError());
		closesocket(s);
		return;
	}

	if(hEventRecv != WSA_INVALID_EVENT) {
		hEventRecv = WSACreateEvent();
	}
	if(hEventSend != WSA_INVALID_EVENT) {
		hEventSend = WSACreateEvent();
	}

	memset(&sendOverlapped, 0, sizeof(sendOverlapped));
	sendOverlapped.hEvent = hEventSend;

	if(sendingBuff.data == nullptr) {
		sendingBuff.Init(SEND_BUFF_LEN);
	}
	sendingBuff.Clear();
}

bool AsyncConnection::StartReceiving()
{
	memset(&recvOverlapped, 0, sizeof(recvOverlapped));
	WSAResetEvent(hEventRecv);
	recvOverlapped.hEvent = hEventRecv;

	WSABUF buff;
	buff.len = RECV_BUFF_LEN;
	buff.buf = recvBuff;

	DWORD len;
	DWORD flags = 0;
	int r = WSARecv(sock, &buff, 1, &len, &flags, &recvOverlapped, NULL);
	if(r == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
		LOG("ERROR(StartReceiving): receive failed (%d)", WSAGetLastError());
		return false;
	}
	return true;
}

bool AsyncConnection::StartSending()
{
	WSABUF buff;
	buff.len = sendingBuff.size;
	buff.buf = (char*)sendingBuff.data;

	DWORD len;
	DWORD flags = 0;
	int err;
	int r = WSASend(sock, &buff, 1, &len, flags, &sendOverlapped, NULL);
	if(r == SOCKET_ERROR && (err = WSAGetLastError()) != WSA_IO_PENDING) {
		LOG("ERROR(StartSending): send failed (%d)", err);
		return false;
	}
	return true;
}

void AsyncConnection::PushSendData(const void* data, const int dataSize)
{
	sendingBuff.Append(data, dataSize);
}

const char* AsyncConnection::GetReceivedData()
{
	return recvBuff;
}

NetPollResult AsyncConnection::PollReceive(int* outRecvLen)
{
	DWORD len;
	DWORD flags = 0;
	i32 r = WSAGetOverlappedResult(sock, &recvOverlapped, &len, FALSE, &flags);
	if(r == FALSE) {
		int err = WSAGetLastError();
		if(err != WSA_IO_INCOMPLETE) {
			LOG("ERROR(PollReceive): Recv WSAGetOverlappedResult failed (%d)", err);
			sock = INVALID_SOCKET;
			return NetPollResult::POLL_ERROR;
		}
		return NetPollResult::PENDING;
	}

	if(len == 0) {
		sock = INVALID_SOCKET;
		return NetPollResult::POLL_ERROR;
	}

	*outRecvLen = len;
	return NetPollResult::SUCCESS;
}

NetPollResult AsyncConnection::PollSend()
{
	DWORD len;
	DWORD flags = 0;
	DWORD r = WSAGetOverlappedResult(sock, &sendOverlapped, &len, FALSE, &flags);
	if(r == FALSE) {
		int err = WSAGetLastError();
		if(err != WSA_IO_INCOMPLETE) {
			LOG("ERROR(PollSend): Send WSAGetOverlappedResult failed (%d)", err);
			sock = INVALID_SOCKET;
			return NetPollResult::POLL_ERROR;
		}
		return NetPollResult::PENDING;
	}

	if(len > 0) {
#ifdef CONF_LOG_TRAFFIC_BYTELEN
		LOG("Sent %d bytes", len);
#endif
		ASSERT(len == sendingBuff.size);
		sendingBuff.Clear();
	}

	memset(&sendOverlapped, 0, sizeof(sendOverlapped));
	sendOverlapped.hEvent = hEventSend;

	return NetPollResult::SUCCESS;
}

void Server::PostControl(ClientHandle handle, DWORD kind)
{
	ASSERT(completionPort);
	const BOOL posted = PostQueuedCompletionStatus(completionPort, kind, ULONG_PTR((u32)handle), nullptr);
	ASSERT(posted);
}

bool Server::SubmitReceive(i32 clientID)
{
	ClientNet& client = clientNet[clientID];
	ASSERT(!client.recvPending && clientIsConnected[clientID] == 1);
	memset(&client.recvOverlapped, 0, sizeof(client.recvOverlapped));
	WSABUF buffer;
	buffer.buf = (char*)client.recvBuff;
	buffer.len = RECV_BUFF_LEN;
	DWORD flags = 0;
	client.recvPending = true;
	pendingIO.Increment();
	const int result = WSARecv(clientSocket[clientID], &buffer, 1, nullptr, &flags, &client.recvOverlapped, nullptr);
	if(result != SOCKET_ERROR || WSAGetLastError() == WSA_IO_PENDING) return true;
	client.recvPending = false;
	pendingIO.Decrement();
	return false;
}

bool Server::SubmitSend(i32 clientID)
{
	ClientNet& client = clientNet[clientID];
	if(client.sendPending) return true;
	if(client.sendingBuff.size == 0) {
		LOCK_MUTEX(client.mutexSend);
		if(client.pendingSendBuff.size == 0) return true;
		eastl::swap(client.sendingBuff.data, client.pendingSendBuff.data);
		eastl::swap(client.sendingBuff.size, client.pendingSendBuff.size);
		eastl::swap(client.sendingBuff.capacity, client.pendingSendBuff.capacity);
		client.pendingSendBuff.Clear();
		client.sendCursor = 0;
	}
	ASSERT(client.sendCursor < client.sendingBuff.size);
	memset(&client.sendOverlapped, 0, sizeof(client.sendOverlapped));
	WSABUF buffer;
	buffer.buf = (char*)client.sendingBuff.data + client.sendCursor;
	buffer.len = ULONG(client.sendingBuff.size - client.sendCursor);
	client.sendPending = true;
	pendingIO.Increment();
	const int result = WSASend(clientSocket[clientID], &buffer, 1, nullptr, 0, &client.sendOverlapped, nullptr);
	if(result != SOCKET_ERROR || WSAGetLastError() == WSA_IO_PENDING) return true;
	client.sendPending = false;
	pendingIO.Decrement();
	return false;
}

void Server::RetireClient(i32 clientID)
{
	ClientNet& client = clientNet[clientID];
	if(clientIsConnected[clientID] != 2 || client.recvPending || client.sendPending) return;
	{
		LOCK_MUTEX(mutexClientDisconnectedList);
		clientDisconnectedList.push_back(client.handle);
	}
	client.sendingBuff.Clear();
	client.pendingSendBuff.Clear();
	client.recvPendingProcessingBuff.Clear();
	client.sendCursor = 0;
	client.sendWakeQueued = false;
	client.handle = ClientHandle::INVALID;
	clientIsConnected[clientID] = 0;
}

void Server::Update()
{
	DWORD transferred = 0;
	ULONG_PTR key = 0;
	OVERLAPPED* operation = nullptr;
	const BOOL completed = GetQueuedCompletionStatus(completionPort, &transferred, &key, &operation, INFINITE);
	if(!operation) {
		ASSERT(completed);
		if(transferred == 2) {
			completionStopping = true;
			for(int i = 0; i < MAX_CLIENTS; ++i) {
				ClientNet& client = clientNet[i];
				LOCK_MUTEX(client.mutexConnect);
				DisconnectClient(i);
			}
			return;
		}
		const ClientHandle handle = ClientHandle((u32)key);
		const i32 clientID = TryGetClientID(handle);
		if(clientID == -1) return;
		ClientNet& client = clientNet[clientID];
		LOCK_MUTEX(client.mutexConnect);
		if(clientIsConnected[clientID] != 1 || client.handle != handle) return;
		if(!running || transferred == 1 || clientDoDisconnect[clientID]) {
			DisconnectClient(clientID);
			return;
		}
		ASSERT(transferred == 0);
		client.sendWakeQueued = false;
		if(!SubmitSend(clientID)) DisconnectClient(clientID);
		return;
	}
	ASSERT(key > 0 && key <= MAX_CLIENTS);
	const i32 clientID = i32(key - 1);
	ClientNet& client = clientNet[clientID];
	LOCK_MUTEX(client.mutexConnect);
	const bool receiving = operation == &client.recvOverlapped;
	ASSERT(receiving || operation == &client.sendOverlapped);
	if(receiving) {
		ASSERT(client.recvPending);
		client.recvPending = false;
	}
	else {
		ASSERT(client.sendPending);
		client.sendPending = false;
	}
	ASSERT(pendingIO != 0);
	pendingIO.Decrement();
	if(clientIsConnected[clientID] == 2) {
		RetireClient(clientID);
		return;
	}
	ASSERT(clientIsConnected[clientID] == 1);
	if(!completed || transferred == 0 || !running || clientDoDisconnect[clientID]) {
		DisconnectClient(clientID);
		return;
	}
	if(receiving) {
		ASSERT(transferred <= RECV_BUFF_LEN);
		ClientHandleReceivedData(clientID, i32(transferred));
		if(!SubmitReceive(clientID)) DisconnectClient(clientID);
	}
	else {
		ASSERT(transferred <= DWORD(client.sendingBuff.size - client.sendCursor));
		client.sendCursor += i32(transferred);
		if(client.sendCursor == client.sendingBuff.size) {
			client.sendingBuff.Clear();
			client.sendCursor = 0;
		}
		if(!SubmitSend(clientID)) DisconnectClient(clientID);
	}
}

#endif
