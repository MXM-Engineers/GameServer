#ifdef CONF_LINUX
#include "network.h"

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

bool SocketSetNonBlocking(SOCKET s)
{
	return true;
}

void AsyncConnection::Init()
{
}

void AsyncConnection::Reset()
{
}

void AsyncConnection::PrepareForNewConnection(SOCKET s)
{
}

bool AsyncConnection::StartReceiving()
{
	return true;
}

bool AsyncConnection::StartSending()
{
	return true;
}

void AsyncConnection::PushSendData(const void* data, const int dataSize)
{
}

const char* AsyncConnection::GetReceivedData()
{
	return nullptr;
}

NetPollResult AsyncConnection::PollReceive(int* outRecvLen)
{
	return NetPollResult::SUCCESS;
}

NetPollResult AsyncConnection::PollSend()
{
	return NetPollResult::SUCCESS;
}

#endif
