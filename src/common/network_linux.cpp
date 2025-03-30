#ifdef CONF_LINUX
#include "network.h"
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <common/protocol.h>

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

//ToDo: finish this function
bool AsyncConnection::ConnectTo(const u8* ip, u16 port)
{
	Init();

	sockaddr_in addr = { 0 };
	memmove(&addr.sin_addr.s_addr, ip, 4);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	ASSERT(s != INVALID_SOCKET);

	int r = connect(s, (sockaddr*)&addr, sizeof(addr));
	if (r == SOCKET_ERROR) return false;

	PostConnectionInit(s);
	return true;
}

void AsyncConnection::PostConnectionInit(SOCKET s)
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

	if(recvBuffProcessing.data == nullptr) {
		recvBuffProcessing.Init(RECV_BUFF_LEN * 4);
	}
	recvBuffProcessing.Clear();

	if(sendingBuff.data == nullptr) {
		sendingBuff.Init(SEND_BUFF_LEN);
	}
	sendingBuff.Clear();
	sendCursor = 0;
	return;
}

bool AsyncConnection::StartReceiving()
{
	char recvTempBuff[RECV_BUFF_LEN];
	ssize_t len = recv(sock, recvTempBuff, sizeof(recvTempBuff), 0);
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
	else if(len == 0) { // disconnect
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
	return (char*)recvBuffProcessing.data;
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
		char recvTempBuff[RECV_BUFF_LEN];
		ssize_t len = recv(sock, recvTempBuff, sizeof(recvTempBuff), 0);
		if(len == -1) {
			if(errno == EAGAIN || errno == EWOULDBLOCK) {
				return NetPollResult::PENDING;
			}
			LOG("ERROR(PollReceive): recv failed (%d)", errno);
			sock = INVALID_SOCKET;
			return NetPollResult::POLL_ERROR;
		}

		if(len > 0) {
			recvBuff.Append(recvTempBuff, len);
		}
		else if(len == 0) { // disconnect
			sock = INVALID_SOCKET;
			return NetPollResult::POLL_ERROR;
		}
	}

	if(recvBuff.size > 0) {
		// only give whole packets, crop partial ones
		CropPartialPackets();

		if(recvBuffProcessing.size == 0) {
			return NetPollResult::PENDING;
		}

		*outRecvLen = recvBuffProcessing.size;
		LOG("Received %d bytes", recvBuffProcessing.size);
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
			sock = INVALID_SOCKET;
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
	}

	return NetPollResult::PENDING;
}

void AsyncConnection::CropPartialPackets()
{
	// TODO: Warning, this is easily exploitable, put a limit on packet sizes

	recvBuffProcessing.Clear();
	ConstBuffer buff(recvBuff.data, recvBuff.size);
	while(buff.CanRead(sizeof(NetHeader))) {
		const u8* wholePacket = buff.cursor;
		const NetHeader& header = buff.Read<NetHeader>();
		const i32 packetDataSize = header.size - sizeof(NetHeader);
		if(buff.CanRead(packetDataSize)) {
			buff.ReadRaw(packetDataSize);
			recvBuffProcessing.Append(wholePacket, header.size);
		}
		else {
			break; // partial packet
		}
	}

	ASSERT(recvBuffProcessing.size <= recvBuff.size);

	const i32 partialAt = recvBuffProcessing.size;
	const i32 partialSize = recvBuff.size - partialAt;

	if(partialSize > 0) {
		u8 tempBuff[8192];
		if(partialSize > sizeof(tempBuff)) {
			thread_local GrowableBuffer tempGrowBuff(sizeof(tempBuff) * 2);
			tempGrowBuff.Clear();
			tempGrowBuff.Append(recvBuff.data + partialAt, partialSize);
			recvBuff.Clear();
			recvBuff.Append(tempGrowBuff.data, tempGrowBuff.size);
		}
		else {
			memmove(tempBuff, recvBuff.data + partialAt, partialSize);
			recvBuff.Clear();
			recvBuff.Append(tempBuff, partialSize);
		}
	}
	else {
		recvBuff.Clear();
	}
}

// https://stackoverflow.com/a/4135003
/**
 * number of seconds from 1 Jan. 1601 00:00 to 1 Jan 1970 00:00 UTC
 */
#define EPOCH_DIFF 11644473600LL

uint64_t CurrentFiletimeTimestampUTC()
{
	struct timeval tv;
	uint64_t result = EPOCH_DIFF;
	gettimeofday(&tv, NULL);
	result += tv.tv_sec;
	result *= 10000000LL;
	result += tv.tv_usec * 10;
	return result;
}
#endif
