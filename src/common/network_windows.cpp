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

bool SocketSetNonBlocking(SOCKET s)
{
	// set non blocking
	u_long NonBlocking = true;
	int ior = ioctlsocket(s, FIONBIO, &NonBlocking);
	if(ior != NO_ERROR) {
		LOG("ERROR(socket=%x): failed to change io mode (%d)", (u32)s, WSAGetLastError());
		closesocket(s);
		return false;
	}

	return true;
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

void AsyncConnection::PrepareForNewConnection(SOCKET s)
{
	sock = s;

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
		if(WSAGetLastError() != WSA_IO_INCOMPLETE) {
			LOG("ERROR(PollReceive): Recv WSAGetOverlappedResult failed (%d)", WSAGetLastError());
			return NetPollResult::POLL_ERROR;
		}
		return NetPollResult::PENDING;
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
		if(WSAGetLastError() != WSA_IO_INCOMPLETE) {
			LOG("ERROR(PollSend): Send WSAGetOverlappedResult failed (%d)", WSAGetLastError());
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

#endif
