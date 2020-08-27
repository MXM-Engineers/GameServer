#include "crossnetwork.h"


int sockInit()
{
#ifdef _WIN32
	WSADATA wsaData;
	return WSAStartup(MAKEWORD(2, 2), &wsaData);
#else
	return 0;
#endif
}

int sockQuit()
{
#ifdef _WIN32
	return WSACleanup();
#else
	return 0;
#endif
}

int sockClose(SOCKET sock)
{
	int status = 0;

#ifdef _WIN32
	status = shutdown(sock, SD_BOTH);
	if (status == 0) { status = closesocket(sock); }
#else
	status = shutdown(sock, SHUT_RDWR);
	if (status == 0) { status = close(sock); }
#endif

	return status;
}

int getLastError()
{
#ifdef _WIN32
	return WSAGetLastError();
#else
	return 0; //ToDo implement
#endif;
}

HANDLE networkCreateEvent()
{
#ifdef _WIN32
	return WSACreateEvent();
#else
	return NULL;
#endif
}

void resetEvent(HANDLE event)
{
#ifdef _WIN32
	WSAResetEvent(event);
#else
	//ToDo implement
#endif
}
