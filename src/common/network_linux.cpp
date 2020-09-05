#ifdef CONF_LINUX
#include "network.h"
#include <fcntl.h>
#include <errno.h>
#include <poll.h>

bool NetworkInit()
{
	return true;
}

void NetworkCleanup()
{
}

int NetworkGetLastError()
{
	return 0;
}

void AsyncConnection::Init()
{
	sock = INVALID_SOCKET;
}

void AsyncConnection::Reset()
{
}

void AsyncConnection::PrepareForNewConnection(SOCKET s)
{
	sock = s;

	// set non blocking
	int ior = fcntl(s, F_SETFL, O_NONBLOCK);
	if(ior == -1) {
		LOG("ERROR(socket=%x): failed to change io mode (%d)", (u32)s, errno);
		closesocket(s);
		return;
	}

	if(recvBuff.data == nullptr) {
		recvBuff.Init(RECV_BUFF_LEN * 4);
	}
	recvBuff.Clear();

	if(sendingBuff.data == nullptr) {
		sendingBuff.Init(SEND_BUFF_LEN);
	}
	sendingBuff.Clear();
	sendCursor = 0;
	return;
}

bool AsyncConnection::StartReceiving()
{
	recvBuff.Clear();

	ssize_t len = recv(sock, recvTempBuff, RECV_BUFF_LEN, 0);
	if(len == -1) {
		if(errno == EAGAIN || errno == EWOULDBLOCK) {
			return true;
		}
		LOG("ERROR(StartReceiving): recv failed (%d)", errno);
		return false;
	}

	if(len > 0) {
		recvBuff.Append(recvTempBuff, len);
	}
	else if(len == 0) {
		return false;
	}
	return true;
}

bool AsyncConnection::StartSending()
{
	sendCursor = 0;

	ssize_t len = send(sock, sendingBuff.data, sendingBuff.size, 0);
	if(len == -1) {
		if(errno == EAGAIN || errno == EWOULDBLOCK) {
			return true;
		}
		LOG("ERROR(StartSending): send failed (%d)", errno);
		return false;
	}

	sendCursor += len;
	return true;
}

void AsyncConnection::PushSendData(const void* data, const int dataSize)
{
	sendingBuff.Append(data, dataSize);
}

const char* AsyncConnection::GetReceivedData()
{
	return (char*)recvBuff.data;
}

NetPollResult AsyncConnection::PollReceive(int* outRecvLen)
{
	struct pollfd pollInfo;
	memset(&pollInfo, 0, sizeof(pollInfo));
	pollInfo.fd = sock;
	pollInfo.events = POLLIN;

	int r = poll(&pollInfo, 1, 0);
	if(r == -1) {
		LOG("ERROR(PollReceive): poll failed (%d)", errno);
		return NetPollResult::POLL_ERROR;
	}

	if(pollInfo.revents & POLLIN) {
		ssize_t len = recv(sock, recvTempBuff, RECV_BUFF_LEN, 0);
		if(len == -1) {
			if(errno == EAGAIN || errno == EWOULDBLOCK) {
				return NetPollResult::PENDING;
			}
			LOG("ERROR(PollReceive): recv failed (%d)", errno);
			return NetPollResult::POLL_ERROR;
		}

		if(len > 0) {
			recvBuff.Append(recvTempBuff, len);
		}
	}

	if(recvBuff.size > 0) {
		*outRecvLen = recvBuff.size;
		LOG("Received %d bytes", recvBuff.size);
		return NetPollResult::SUCCESS;
	}

	return NetPollResult::PENDING;
}

NetPollResult AsyncConnection::PollSend()
{
	ASSERT(sendCursor <= sendingBuff.size);
	if(sendCursor == sendingBuff.size) {
		sendingBuff.Clear();
		sendCursor = 0;
		return NetPollResult::SUCCESS;
	}

	struct pollfd pollInfo;
	memset(&pollInfo, 0, sizeof(pollInfo));
	pollInfo.fd = sock;
	pollInfo.events = POLLOUT;

	int r = poll(&pollInfo, 1, 0);
	if(r == -1) {
		LOG("ERROR(PollSend): poll failed (%d)", errno);
		return NetPollResult::POLL_ERROR;
	}

	if(pollInfo.revents & POLLOUT) {
		ssize_t len = send(sock, sendingBuff.data + sendCursor, sendingBuff.size - sendCursor, 0);
		if(len == -1) {
			if(errno == EAGAIN || errno == EWOULDBLOCK) {
				return NetPollResult::PENDING;
			}
			LOG("ERROR(PollSend): write failed (%d)", errno);
			return NetPollResult::POLL_ERROR;
		}

		if(len > 0) {
			sendCursor += len;
			LOG("Sent %ld bytes", len);

			ASSERT(sendCursor <= sendingBuff.size);
			if(sendCursor == sendingBuff.size) {
				sendingBuff.Clear();
				sendCursor = 0;
				return NetPollResult::SUCCESS;
			}
		}
		else if(len == 0) {
			return NetPollResult::POLL_ERROR;
		}
	}

	return NetPollResult::PENDING;
}

#endif
