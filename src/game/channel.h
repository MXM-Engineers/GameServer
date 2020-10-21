#pragma once
#include "game.h"
#include <eathread/eathread_thread.h>

struct Channel
{
	struct EventOnClientConnect
	{
		i32 clientID;
		const AccountData* accountData;
	};

	Server* server;
	IGame* game;

	Replication replication;
	Time localTime;

	EA::Thread::Thread thread;

	GrowableBuffer packetDataQueue;
	GrowableBuffer processPacketQueue;
	eastl::fixed_vector<i32,128> clientDisconnectedList;
	ProfileMutex(Mutex, mutexPacketDataQueue);
	ProfileMutex(Mutex, mutexClientDisconnectedList);
	eastl::fixed_vector<EventOnClientConnect,128> newPlayerQueue;
	ProfileMutex(Mutex, mutexNewPlayerQueue);

	bool Init(Server* server_, ListenerType type);
	void Cleanup();

	void Update(f64 delta);

	void CoordinatorRegisterNewPlayer(i32 clientID, const AccountData* accountData);
	void CoordinatorClientHandlePacket(i32 clientID, const NetHeader& header, const u8* packetData);
	void CoordinatorHandleDisconnectedClients(i32* clientIDList, const i32 count);

private:
	void ClientHandlePacket(i32 clientID, const NetHeader& header, const u8* packetData);
	void HandlePacket_CN_ReadyToLoadCharacter(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_ReadyToLoadGameMap(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CA_SetGameGvt(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_MapIsLoaded(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_GameMapLoaded(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_GetCharacterInfo(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_UpdatePosition(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_ChannelChatMessage(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_SetLeaderCharacter(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_GamePlayerSyncActionStateOnly(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_JukeboxQueueSong(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_WhisperSend(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_PartyCreate(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_RTT_Time(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_LoadingProgressData(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_LoadingComplete(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_GameIsReady(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
};
