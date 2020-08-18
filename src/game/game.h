#pragma once
#include <common/base.h>
#include <common/protocol.h>
#include <common/network.h>
#include <common/utils.h>

// TODO: move this
struct AccountData
{
	WString nickname;

	// TODO: add to this
};



struct Game
{
	enum {
		MAX_PLAYERS = Server::MAX_CLIENTS
	};

	Server* server;
	GrowableBuffer packetDataQueue;
	GrowableBuffer processPacketQueue;
	std::mutex mutexPacketDataQueue;

	const AccountData* playerAccountData[MAX_PLAYERS];

	void Init(Server* server_);
	void Update();

	void CoordinatorClientEnter(i32 clientID, const AccountData* accountData);
	void CoordinatorClientHandlePacket(i32 clientID, const NetHeader& header, const u8* packetData);

	void ClientHandlePacket(i32 clientID, const NetHeader& header, const u8* packetData);
	void ClientEnter(i32 clientID, const AccountData* accountData);

	void PlayerSpawnInMap(i32 clientID);

	void SendNPCSpawn(i32 clientID, i32 objectID, i32 nIDX, const Vec3& pos, const Vec3& dir);

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
