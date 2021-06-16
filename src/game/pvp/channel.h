#pragma once
#include "game.h"
#include <game/coordinator.h>
#include <eathread/eathread_thread.h>

struct ChannelPvP
{
	i32 threadID;
	Time localTime;
	Server* server;
	Coordinator::Lane* lane;

	Game* game;
	Replication replication;

	struct ClientTime
	{
		f64 posClient = 0;
		f64 posServer = 0;
		u32 rttClient = 0;
		i64 rttServer = 0;
	};

	eastl::array<ClientTime, Server::MAX_CLIENTS> clientTime;

	bool Init(Server* server_);
	void Cleanup();

	void Update();

private:
	void ClientHandlePacket(i32 clientID, const NetHeader& header, const u8* packetData);
	void HandlePacket_CN_ReadyToLoadGameMap(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CA_SetGameGvt(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_GameMapLoaded(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_GetCharacterInfo(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_GameUpdatePosition(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_GameUpdateRotation(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_ChannelChatMessage(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_SetLeaderCharacter(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_GamePlayerSyncActionStateOnly(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_WhisperSend(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_PartyCreate(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_RTT_Time(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_LoadingProgressData(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_LoadingComplete(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_GameIsReady(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_GamePlayerTag(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_PlayerJump(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_PlayerCastSkill(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);

	void ReadPacket(PlayerCastSkill* cast, i32 clientID, const u8* packetData, const i32 packetSize);
};
