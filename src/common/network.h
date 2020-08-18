#pragma once
#include "base.h"

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#include <mutex>

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
		std::mutex mutexRecv;
		std::mutex mutexSend;
		std::mutex mutexConnect;
		OVERLAPPED recvOverlapped;
		OVERLAPPED sendOverlapped;
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
	u8 clientIsConnected[MAX_CLIENTS]; // is guarded by ClientNet.mutexConnect
	SOCKET clientSocket[MAX_CLIENTS];
	ClientNet clientNet[MAX_CLIENTS];
	ClientInfo clientInfo[MAX_CLIENTS];

#ifdef CONF_DEBUG
	i32 packetCounter = 0;
#endif

	bool Init(const char* listenPort);
	void Cleanup();

	i32 AddClient(SOCKET s, const sockaddr& addr_);
	void DisconnectClient(i32 clientID);

	void Update();

	void TransferAllReceivedData(GrowableBuffer* out);

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
