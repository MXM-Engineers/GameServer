#pragma once
#include "base.h"
#include "protocol.h"

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
# endif

#ifdef __linux__
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#define INVALID_SOCKET    -1
#define SOCKET_ERROR      -1
#define closesocket(fd)   close(fd)
#define WSA_INVALID_EVENT NULL //ToDo set linux equivallent value
#define WSA_IO_PENDING NULL //ToDo set linux equivallent value
#define WSA_IO_INCOMPLETE NULL //ToDo set linux equivallent value
#define NO_ERROR 0
typedef int SOCKET;

typedef struct _WSABUF {
	size_t len;     /* the length of the buffer */
	void *buf; /* the pointer to the buffer */
} WSABUF;
#endif

int sockInit(void);
int sockQuit(void);
int sockClose(SOCKET);
int getLastError(void);
HANDLE networkCreateEvent(void);
void resetEvent(HANDLE);
