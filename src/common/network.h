#pragma once
#include "base.h"
#include "utils.h"
#include <EASTL/array.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/fixed_hash_map.h>
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
		GrowableBuffer sendingBuff;
		GrowableBuffer recvBuff; // TODO: make this a flat buffer (no malloc)?
		GrowableBuffer recvBuffProcessing; // TODO: make this a flat buffer (no malloc)?
		i32 sendCursor;

		void Init();
		void Reset();

		bool ConnectTo(const u8* ip, u16 port);
		void PostConnectionInit(SOCKET s);

		bool StartReceiving();
		bool StartSending();
		void PushSendData(const void* data, const int dataSize);
		const char* GetReceivedData();
		NetPollResult PollReceive(int* outRecvLen);
		NetPollResult PollSend();
		void CropPartialPackets();

		inline bool IsConnected() const { return sock != INVALID_SOCKET; }
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

		bool ConnectTo(const u8* ip, u16 port);
		void PostConnectionInit(SOCKET s);

		bool StartReceiving();
		bool StartSending();
		void PushSendData(const void* data, const int dataSize);
		const char* GetReceivedData();
		NetPollResult PollReceive(int* outRecvLen);
		NetPollResult PollSend();

		inline bool IsConnected() const { return sock != INVALID_SOCKET; }
	};
#endif

const char* IpToString(const u8* ip);
const void SetIp(u8* ip, u8 i0, u8 i1, u8 i2, u8 i3);
const char* GetIpString(const sockaddr& addr);

bool NetworkInit();
void NetworkCleanup();
i32 NetworkGetLastError();

enum
{
	MAX_CLIENTS = 256, // TODO: vastly increase this number
};

enum class ClientHandle: u32 {
	INVALID = 0,
};

typedef LocalMapping<i32, ClientHandle, 0, MAX_CLIENTS, -1> ClientLocalMapping;

struct Server
{
	// fixed non growing hash map
	template<typename T1, typename T2, int CAPACITY>
	using hash_map = eastl::fixed_hash_map<T1 ,T2, CAPACITY, CAPACITY+1, false>;

	struct ClientNet
	{
#ifdef CONF_WINDOWS
		OVERLAPPED recvOverlapped{};
		OVERLAPPED sendOverlapped{};
		u8 recvBuff[RECV_BUFF_LEN];
		GrowableBuffer sendingBuff;
		i32 sendCursor = 0;
		bool recvPending = false;
		bool sendPending = false;
		bool sendWakeQueued = false;
#else
		AsyncConnection async;
#endif
		ClientHandle handle = ClientHandle::INVALID;
		sockaddr addr;
		GrowableBuffer recvPendingProcessingBuff;
		GrowableBuffer pendingSendBuff;
		ProfileMutex(Mutex, mutexRecv);
		ProfileMutex(Mutex, mutexSend);
		Mutex mutexConnect;
	};

	struct ClientInfo
	{
		eastl::array<u8,4> ip;
		u16 port;
	};

	struct RecvChunkHeader
	{
		ClientHandle clientHd;
		i32 len;
	};

	bool initialized = false;
	EA::Thread::AtomicUint32 running = 0;
	eastl::array<u8,MAX_CLIENTS> clientIsConnected; // is guarded by ClientNet.mutexConnect
	eastl::array<u8,MAX_CLIENTS> clientDoDisconnect;
	eastl::array<SOCKET,MAX_CLIENTS> clientSocket;
	eastl::array<ClientNet,MAX_CLIENTS> clientNet;
	eastl::array<ClientInfo,MAX_CLIENTS> clientInfo;

	hash_map<ClientHandle,i32,MAX_CLIENTS*4> clientHandle2IDMap;
	ClientHandle nextclientHd = ClientHandle(1);
	mutable Mutex mutexClientMap;

	struct ConnectedClient
	{
		ClientHandle handle;
		ClientInfo info;
	};

	eastl::fixed_vector<ConnectedClient,MAX_CLIENTS> clientConnectedList;
	eastl::fixed_vector<ClientHandle,MAX_CLIENTS> clientDisconnectedList;
	ProfileMutex(Mutex, mutexClientConnectedList);
	ProfileMutex(Mutex, mutexClientDisconnectedList);

	EA::Thread::Thread thread;
#ifdef CONF_WINDOWS
	HANDLE completionPort = nullptr;
	EA::Thread::AtomicUint32 pendingIO = 0;
	bool completionStopping = false;
#endif

	i32 packetCounter = 0;
	bool doTraceNetwork = false;

	bool Init();
	void Cleanup();
	void Stop();

	ClientHandle ListenerAddClient(SOCKET s, const sockaddr& addr_);
	void DisconnectClient(ClientHandle clientHd);

	void Update();

	void TransferAllReceivedData(GrowableBuffer* out);
	void TransferReceivedData(GrowableBuffer* out, const ClientHandle* clientList, const u32 clientCount);

	template<class Array>
	void TransferConnectedClientList(Array* out)
	{
		LOCK_MUTEX(mutexClientConnectedList);
		for(const auto& client: clientConnectedList) out->push_back(client.handle);
		clientConnectedList.clear();
	}

	// has client info as well
	template<class Array>
	void TransferConnectedClientListEx(Array* out)
	{
		LOCK_MUTEX(mutexClientConnectedList);
		for(const auto& client: clientConnectedList) {
			out->push_back(eastl::pair<ClientHandle,ClientInfo>(client.handle, client.info));
		}
		clientConnectedList.clear();
	}

	template<class Array>
	void TransferDisconnectedClientList(Array* out)
	{
		LOCK_MUTEX(mutexClientDisconnectedList);
		eastl::copy(clientDisconnectedList.begin(), clientDisconnectedList.end(), eastl::back_inserter(*out));
		clientDisconnectedList.clear();
	}

	template<typename Packet>
	inline void SendPacket(ClientHandle clientHd, const Packet& packet)
	{
		SendPacketData(clientHd, Packet::NET_ID, sizeof(packet), &packet);
	}
	void SendPacketData(ClientHandle clientHd, u16 netID, u16 packetSize, const void* packetData);

private:

	inline i32 TryGetClientID(ClientHandle clientHd) const
	{
		LOCK_MUTEX(mutexClientMap);
		auto f = clientHandle2IDMap.find(clientHd);
		if(f == clientHandle2IDMap.end()) return -1;
		return f->second;
	}


	void DisconnectClient(i32 clientID);

	void ClientSend(i32 clientID, ClientHandle handle, const void* data, i32 dataSize);
#ifndef CONF_WINDOWS
	bool ClientStartReceiving(i32 clientID);
#endif
	void ClientHandleReceivedData(i32 clientID, i32 dataLen);
	void TransferClientReceivedData(GrowableBuffer* out, i32 clientID, ClientHandle expected);
#ifdef CONF_WINDOWS
	void PostControl(ClientHandle handle, DWORD kind);
	bool SubmitReceive(i32 clientID);
	bool SubmitSend(i32 clientID);
	void RetireClient(i32 clientID);
#endif
};

struct Listener
{
	Server& server;
	SOCKET listenSocket;
	i32 listenPort;

	Listener(Server* server_): server(*server_) {}

	bool Init(i32 listenPort_);
	void Stop();
	void Listen();

	inline bool IsRunning() const { return listenSocket != INVALID_SOCKET; }
};

struct InnerConnection
{
	AsyncConnection async;

	struct SendQueue
	{
		enum {
			QUEUE_CAPACITY = 1024 * 1024 // 1MB
		};

		u8 data[QUEUE_CAPACITY];
		i32 size = 0;
	};

	SendQueue sendQ;

	template<typename Packet>
	inline void SendPacket(const Packet& packet)
	{
		SendPacketData(Packet::NET_ID, sizeof(packet), &packet);
	}

	void SendPacketData(u16 netID, u16 packetSize, const void* packetData);

	void SendPendingData();
	void RecvPendingData(u8* buff, const i32 buffCapacity, i32* size);
};
