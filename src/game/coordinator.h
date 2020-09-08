#pragma once
#include <common/network.h>
#include <common/utils.h>
#include <common/protocol.h>
#include <eathread/eathread_thread.h>

// TODO: move this
struct AccountData
{
	WideString nickname;
	WideString guildTag;
	i32 leaderMasterID;

	// TODO: add to this
};

struct Channel;

// Responsible for managing Account data and dispatching client to game channels/instances
struct Coordinator
{
	Server* server;
	Channel* channel;
	AccountData accountData[Server::MAX_CLIENTS];
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
