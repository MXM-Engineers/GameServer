#pragma once
#include <eathread/eathread_thread.h>
#include <common/network.h>
#include <common/utils.h>
#include <common/protocol.h>

// TODO: move this
struct AccountData
{
	WideString nickname;
	WideString guildTag;
	i32 leaderMasterID;

	// TODO: add to this
};

struct ChannelHub;

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

	i32 ID;
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

// Responsible for managing Account data and dispatching client to game channels/instances
struct Coordinator
{
	enum class LaneID: i32 {
		INVALID = -1,
		FIRST = 0,
		HUB = 0,
		_COUNT,
	};

	struct Lane
	{
		struct EventOnClientConnect
		{
			i32 clientID;
			const AccountData* accountData;
		};

		Time localTime;

		EA::Thread::Thread thread;

		GrowableBuffer packetDataQueue;
		GrowableBuffer processPacketQueue;
		eastl::fixed_vector<i32,128> clientDisconnectedList;
		ProfileMutex(Mutex, mutexPacketDataQueue);
		ProfileMutex(Mutex, mutexClientDisconnectedList);
		eastl::fixed_vector<EventOnClientConnect,128> newPlayerQueue;
		ProfileMutex(Mutex, mutexNewPlayerQueue);

		void Init();
		void CoordinatorRegisterNewPlayer(i32 clientID, const AccountData* accountData);
		void CoordinatorClientHandlePacket(i32 clientID, const NetHeader& header, const u8* packetData);
		void CoordinatorHandleDisconnectedClients(i32* clientIDList, const i32 count);
	};

	Server* server;
	eastl::fixed_vector<InnerConnection, 16, false> connGameSrv;

	eastl::array<Lane, (i32)LaneID::_COUNT> laneList;
	ChannelHub* channelHub;

	eastl::array<AccountData, Server::MAX_CLIENTS> accountData;
	eastl::array<LaneID, Server::MAX_CLIENTS> associatedChannel;
	GrowableBuffer recvDataBuff;
	EA::Thread::Thread thread;
	Time localTime;

	bool Init(Server* server_);
	void Cleanup();

	void Update(f64 delta);

private:
	bool ConnectToInfrastructure();

	void ClientHandlePacket(i32 clientID, const NetHeader& header, const u8* packetData);
	void ClientHandleReceivedChunk(i32 clientID, const u8* data, const i32 dataSize);

	void HandlePacket_CQ_FirstHello(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_Authenticate(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_AuthenticateGameServer(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_GetGuildProfile(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_GetGuildMemberList(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_GetGuildHistoryList(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_GetGuildRankingSeasonList(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_TierRecord(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);

	void ClientSendAccountData(i32 clientID);

	template<typename Packet>
	inline void SendPacket(i32 clientID, const Packet& packet)
	{
		SendPacketData(clientID, Packet::NET_ID, sizeof(packet), &packet);
	}
	inline void SendPacketData(i32 clientID, u16 netID, u16 packetSize, const void* packetData)
	{
		server->SendPacketData(clientID, netID, packetSize, packetData);
	}
};
