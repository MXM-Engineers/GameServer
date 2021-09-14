#pragma once
#include "game.h"
#include <common/packet_serialize.h>

// TODO: this should not send anything
// This class is just used to validate and transform packets to server them to Game
struct HubPacketHandler
{
	Server* server;
	HubGame* game;
	HubReplication* replication;

	bool Init(HubGame* game_);

	void OnNewClientsConnected(const eastl::pair<ClientHandle, const AccountData*>* clientList, const i32 count);
	void OnNewClientsDisconnected(const ClientHandle* clientList, const i32 count);
	void OnNewPacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData);

private:
	void HandlePacket_CQ_GetGuildProfile(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_GetGuildMemberList(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_GetGuildHistoryList(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_GetGuildRankingSeasonList(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_TierRecord(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);

	void HandlePacket_CN_ReadyToLoadCharacter(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_ReadyToLoadGameMap(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CA_SetGameGvt(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_MapIsLoaded(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_GetCharacterInfo(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_UpdatePosition(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_ChannelChatMessage(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_SetLeaderCharacter(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_GamePlayerSyncActionStateOnly(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_JukeboxQueueSong(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_WhisperSend(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);

	void HandlePacket_CQ_RTT_Time(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_LoadingProgressData(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_RequestCalendar(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_RequestAreaPopularity(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);

	void HandlePacket_CQ_PartyCreate(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_PartyModify(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_PartyOptionModify(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_EnqueueGame(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CA_SortieRoomFound(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_SortieRoomConfirm(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);

	template<typename Packet>
	inline void SendPacket(ClientHandle clientHd, const Packet& packet)
	{
		SendPacketData<Packet>(clientHd, sizeof(packet), &packet);
	}

	template<typename Packet, u32 CAPACITY>
	inline void SendPacket(ClientHandle clientHd, const PacketWriter<Packet,CAPACITY>& writer)
	{
		SendPacketData<Packet>(clientHd, writer.size, writer.data);
	}

	template<typename Packet>
	inline void SendPacketData(ClientHandle clientHd, u16 packetSize, const void* packetData)
	{
		NT_LOG("[client%x] Hub :: %s", clientHd, PacketSerialize<Packet>(packetData, packetSize));
		server->SendPacketData(clientHd, Packet::NET_ID, packetSize, packetData);
	}
};
