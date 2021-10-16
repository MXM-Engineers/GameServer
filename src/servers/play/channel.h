#pragma once
#include "game.h"

struct GamePacketHandler
{
	Time localTime;
	Server* server;

	Game* game;
	Replication* replication;

	struct ClientTime
	{
		f64 posClient = 0;
		f64 posServer = 0;
		u32 rttClient = 0;
		i64 rttServer = 0;
	};

	eastl::array<ClientTime, MAX_CLIENTS> clientTime;

	ClientLocalMapping plidMap;

	bool Init(Game* game_);
	void Cleanup();

	void OnClientsConnected(const eastl::pair<ClientHandle,AccountUID>* clientList, const i32 count);
	void OnClientsDisconnected(const ClientHandle* clientList, const i32 count);
	void OnNewPacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData);

private:
	void HandlePacket_CN_ReadyToLoadGameMap(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CA_SetGameGvt(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_GameMapLoaded(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_GetCharacterInfo(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_GameUpdatePosition(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_GameUpdateRotation(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_ChannelChatMessage(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_SetLeaderCharacter(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_GamePlayerSyncActionStateOnly(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_WhisperSend(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_RTT_Time(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_LoadingProgressData(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_LoadingComplete(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_GameIsReady(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_GamePlayerTag(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_PlayerJump(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_PlayerCastSkill(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);

	void ReadPacket(PlayerCastSkill* cast, ClientHandle clientHd, const u8* packetData, const i32 packetSize);
};
