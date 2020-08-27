#pragma once
#include "base.h"
#include <EASTL/array.h>
#include <EASTL/fixed_vector.h>

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
	typedef int SOCKET;
#endif

const char* IpToString(const u8* ip);
const void SetIp(u8* ip, u8 i0, u8 i1, u8 i2, u8 i3);
const char* GetIpString(const sockaddr& addr);

struct Server
{
	enum {
		MAX_CLIENTS=256,
		RECV_BUFF_LEN=8192,
		SEND_BUFF_LEN=8192,
	};

	struct ClientNet
	{
		sockaddr addr;
		GrowableBuffer recvPendingProcessingBuff;
		GrowableBuffer pendingSendBuff;
		GrowableBuffer sendingBuff;
		u8 recvBuffID;
		u8 sendBuffID;
		Mutex mutexRecv;
		Mutex mutexSend;
		Mutex mutexConnect;
#ifdef _WIN32
		OVERLAPPED recvOverlapped;
		OVERLAPPED sendOverlapped;
#endif
		HANDLE hEventRecv;
		HANDLE hEventSend;
		char recvBuff[RECV_BUFF_LEN];
		char sendBuff[SEND_BUFF_LEN];
	};

	struct ClientInfo
	{
		u8 ip[4];
		u16 port;
	};

	struct ReceiveBufferHeader
	{
		i32 clientID;
		i32 len;
	};

	bool running;
	SOCKET serverSocket;
	eastl::array<u8,MAX_CLIENTS> clientIsConnected; // is guarded by ClientNet.mutexConnect
	eastl::array<SOCKET,MAX_CLIENTS> clientSocket;
	eastl::array<ClientNet,MAX_CLIENTS> clientNet;
	eastl::array<ClientInfo,MAX_CLIENTS> clientInfo;

	eastl::fixed_vector<i32,MAX_CLIENTS> clientDisconnectedList;
	Mutex mutexClientDisconnectedList;

#ifdef CONF_DEBUG
	i32 packetCounter = 0;
#endif

	bool Init(const char* listenPort);
	void Cleanup();

	i32 AddClient(SOCKET s, const sockaddr& addr_);
	void DisconnectClient(i32 clientID);

	void Update();

	void TransferAllReceivedData(GrowableBuffer* out);

	template<class Array>
	void TransferDisconnectedClientList(Array* out)
	{
		if(clientDisconnectedList.size() > 0) {
			const LockGuard lock(mutexClientDisconnectedList);
			eastl::copy(clientDisconnectedList.begin(), clientDisconnectedList.end(), eastl::back_inserter(*out));
			clientDisconnectedList.clear();
		}
	}

	template<typename Packet>
	inline void SendPacket(i32 clientID, const Packet& packet)
	{
		SendPacketData(clientID, Packet::NET_ID, sizeof(packet), &packet);
	}
	void SendPacketData(i32 clientID, u16 netID, u16 packetSize, const void* packetData);

private:

	void ClientSend(i32 clientID, const void* data, i32 dataSize);
	bool ClientStartReceiving(i32 clientID);
	bool ClientHandleReceivedData(i32 clientID, i32 dataLen);
};
