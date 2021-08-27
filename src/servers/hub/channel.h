#pragma once
#include "game.h"
#include "coordinator.h"
#include <common/packet_serialize.h>

struct ChannelHub
{
	i32 threadID;
	Time localTime;
	Server* server;
	Coordinator::Lane* lane;

	GameHub* game;
	ReplicationHub replication;

	bool Init(Server* server_);
	void Cleanup();

	void Update();

private:
	void ClientHandlePacket(i32 clientID, const NetHeader& header, const u8* packetData);

	void HandlePacket_CN_ReadyToLoadCharacter(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_ReadyToLoadGameMap(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CA_SetGameGvt(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_MapIsLoaded(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
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
	void HandlePacket_CQ_RequestCalendar(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_RequestAreaPopularity(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize);

	template<typename Packet>
	inline void SendPacket(i32 clientID, const Packet& packet)
	{
		SendPacketData<Packet>(clientID, sizeof(packet), &packet);
	}

	template<typename Packet>
	inline void SendPacket(i32 clientID, const PacketWriter& writer)
	{
		SendPacketData<Packet>(clientID, writer.size, writer.data);
	}

	template<typename Packet>
	inline void SendPacketData(i32 clientID, u16 packetSize, const void* packetData)
	{
		NT_LOG("[client%03d] Hub :: %s", clientID, PacketSerialize<Packet>(packetData, packetSize));
		server->SendPacketData(clientID, Packet::NET_ID, packetSize, packetData);
	}
};
