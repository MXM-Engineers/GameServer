#pragma once
#include "base.h"
#include <EASTL/array.h>
#include <EASTL/fixed_vector.h>
#include <eathread/eathread_thread.h>

enum class NetPollResult: int
{
	SUCCESS=0,
	PENDING=1,
	POLL_ERROR=2
};

enum {
	RECV_BUFF_LEN=8192,
	SEND_BUFF_LEN=8192,
};

#ifdef CONF_LINUX
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netinet/ip.h>
	#include <arpa/inet.h>
	#include <unistd.h>
	#include <netdb.h>

	typedef struct sockaddr sockaddr;
	typedef struct sockaddr_in sockaddr_in;
	typedef int SOCKET;
	#define INVALID_SOCKET (-1)
	#define SOCKET_ERROR (-1)

	typedef u32 AddrLen;

	struct AsyncConnection
	{
		SOCKET sock;
		char recvTempBuff[RECV_BUFF_LEN];
		GrowableBuffer sendingBuff;
		GrowableBuffer recvBuff; // TODO: make this a flat buffer (no malloc)?
		i32 sendCursor;

		void Init();
		void Reset();
		void PrepareForNewConnection(SOCKET s);

		bool StartReceiving();
		bool StartSending();
		void PushSendData(const void* data, const int dataSize);
		const char* GetReceivedData();
		NetPollResult PollReceive(int* outRecvLen);
		NetPollResult PollSend();
	};

	inline void closesocket(SOCKET s) { close(s); }
#endif

#ifdef CONF_WINDOWS
	#include <windows.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <iphlpapi.h>

	typedef i32 AddrLen;

	struct AsyncConnection
	{
		SOCKET sock;
		OVERLAPPED recvOverlapped;
		OVERLAPPED sendOverlapped;
		HANDLE hEventRecv;
		HANDLE hEventSend;
		char recvBuff[RECV_BUFF_LEN];
		GrowableBuffer sendingBuff;

		void Init();
		void Reset();
		void PrepareForNewConnection(SOCKET s);

		bool StartReceiving();
		bool StartSending();
		void PushSendData(const void* data, const int dataSize);
		const char* GetReceivedData();
		NetPollResult PollReceive(int* outRecvLen);
		NetPollResult PollSend();
	};
#endif

const char* IpToString(const u8* ip);
const void SetIp(u8* ip, u8 i0, u8 i1, u8 i2, u8 i3);
const char* GetIpString(const sockaddr& addr);

bool NetworkInit();
void NetworkCleanup();
i32 NetworkGetLastError();

struct Server
{
	enum {
		MAX_CLIENTS=256,
	};

	struct ClientNet
	{
		AsyncConnection async;
		sockaddr addr;
		GrowableBuffer recvPendingProcessingBuff;
		GrowableBuffer pendingSendBuff;
		Mutex mutexRecv;
		Mutex mutexSend;
		Mutex mutexConnect;
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

	EA::Thread::Thread thread;

	i32 packetCounter = 0;
	bool doTraceNetwork = false;

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
