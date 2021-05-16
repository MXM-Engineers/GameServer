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
struct ChannelPvP;

// Responsible for managing Account data and dispatching client to game channels/instances
struct Coordinator
{
	enum class LaneID: i32 {
		INVALID = -1,
		FIRST = 0,
		HUB = 0,
		PVP = 1,
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
	eastl::array<Lane, (i32)LaneID::_COUNT> laneList;
	ChannelHub* channelHub;
	ChannelPvP* channelPvP;
	eastl::array<AccountData, Server::MAX_CLIENTS> accountData;
	eastl::array<LaneID, Server::MAX_CLIENTS> associatedChannel;
	GrowableBuffer recvDataBuff;
	EA::Thread::Thread thread;
	Time localTime;

	void Init(Server* server_);
	void Cleanup();

	void Update(f64 delta);

private:
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
