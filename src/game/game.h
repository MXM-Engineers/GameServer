#pragma once
#include <common/base.h>
#include <common/protocol.h>
#include <common/network.h>
#include <common/crossnetwork.h>
#include <common/utils.h>
#include "world.h"

struct AccountData;

struct Game
{
	enum {
		MAX_PLAYERS = Server::MAX_CLIENTS
	};

	struct NewPlayerEntry
	{
		i32 clientID;
		const AccountData* accountData;
	};

	Server* server;
	GrowableBuffer packetDataQueue;
	GrowableBuffer processPacketQueue;
	eastl::fixed_vector<i32,128> clientDisconnectedList;
	Mutex mutexPacketDataQueue;
	Mutex mutexClientDisconnectedList;
	eastl::fixed_vector<NewPlayerEntry,128> newPlayerQueue;
	Mutex mutexNewPlayerQueue;

	const AccountData* playerAccountData[MAX_PLAYERS];

	World world;
	Replication replication;

	eastl::array<ActorUID,MAX_PLAYERS> playerActorUID;

	void Init(Server* server_);
	void Update(f64 delta);
	bool LoadMap();

	void CoordinatorRegisterNewPlayer(i32 clientID, const AccountData* accountData);
	void CoordinatorClientHandlePacket(i32 clientID, const NetHeader& header, const u8* packetData);
	void CoordinatorHandleDisconnectedClients(i32* clientIDList, const i32 count);

private:
	void ClientHandlePacket(i32 clientID, const NetHeader& header, const u8* packetData);

	void HandlePacket_CN_ReadyToLoadCharacter(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CA_SetGameGvt(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_MapIsLoaded(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_GetCharacterInfo(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_UpdatePosition(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_ChannelChatMessage(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_SetLeaderCharacter(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);

	void OnClientConnect(i32 clientID, const AccountData* accountData);
	void OnClientDisconnect(i32 clientID);

	bool ParseChatCommand(i32 clientID, const wchar* msg, const i32 len);
	void SendDbgMsg(i32 clientID, const wchar* msg);

	void SpawnNPC(i32 modelID, const Vec3& pos, const Vec3& dir);
};
