#pragma once
#include <common/base.h>
#include <common/protocol.h>
#include <common/network.h>
#include <common/utils.h>
#include "world.h"

struct AccountData;

struct Game
{
	enum {
		MAX_PLAYERS = Server::MAX_CLIENTS
	};

	Server* server;
	GrowableBuffer packetDataQueue;
	GrowableBuffer processPacketQueue;
	Mutex mutexPacketDataQueue;

	const AccountData* playerAccountData[MAX_PLAYERS];

	World world;
	Replication replication;

	eastl::array<ActorUID,MAX_PLAYERS> playerActorUID;

	void Init(Server* server_);
	void Update(f64 delta);
	bool LoadMap();

	void CoordinatorRegisterNewPlayer(i32 clientID, const AccountData* accountData);
	void CoordinatorClientHandlePacket(i32 clientID, const NetHeader& header, const u8* packetData);

private:
	void ClientHandlePacket(i32 clientID, const NetHeader& header, const u8* packetData);

	void HandlePacket_CN_ReadyToLoadCharacter(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CA_SetGameGvt(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_MapIsLoaded(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_GetCharacterInfo(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_UpdatePosition(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_ChannelChatMessage(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);

	void SpawnNPC(i32 modelID, const Vec3& pos, const Vec3& dir);
};
