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

	void OnClientsConnected(const eastl::pair<ClientHandle, const Account*>* clientList, const i32 count);
	void OnClientsDisconnected(const ClientHandle* clientList, const i32 count);
	void OnNewPacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData);
	void OnMatchmakerPacket(const NetHeader& header, const u8* packetData);

private:
	void HandlePacket_CQ_GetGuildRankingSeasonList(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_TierRecord(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);

	void HandlePacket_CN_ReadyToLoadCharacter(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_ReadyToLoadGameMap(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_EnterCityScene(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_ReturnToCity(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_ReturnToGame(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_PartyInvite(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_PartyInviteAccept(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_BreakUpPartyRoom(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_LeavePartyRoom(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_CancelMatching(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_FriendRequest(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_FriendRequestRespond(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_FriendRemove(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_FriendSetFavorite(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_FriendUnsetFavorite(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_CommunityBlockAdd(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_CommunityBlockRemove(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_CommunityStatusMessage(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_Praise(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_Honor(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_NpBuyCshopItem(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_BuyCShopItem(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_BuyShopItem(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_InventoryExpand(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CA_SetGameGvt(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CA_CityLobbyJoinCity(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_GetCharacterInfo(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_GamePlayerSyncByInt(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_ChannelChatMessage(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_SetLeaderCharacter(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_GamePlayerSyncActionStateOnly(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_JukeboxQueueSong(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_WhisperSend(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);

	void HandlePacket_CQ_RTT_Time(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_LoadingProgressData(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_RequestCalendar(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_MailShowList(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_MailRead(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_RequestMissionReward(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_DailyMissionChange(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_MailWrite(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_MailMove(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_MailGetAttachment(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_MailGetAttachmentMulti(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_RequestAreaPopularity(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);

	void HandlePacket_CQ_PartyCreate(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_PartyModify(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_PartyOptionModify(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_EnqueueGame(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CA_SortieRoomFound(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CN_SortieRoomConfirm(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_GetUserInfo(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_ItemTrade(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_SkinChipDissolve(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_ItemUse(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_GearEquip(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_PvpDetailRecord(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePvpRecord(ClientHandle clientHd);
	void HandleSummaryReward(ClientHandle clientHd, const u8* packetData, const i32 packetSize);
	void HandleTitleSelect(ClientHandle clientHd, const u8* packetData, const i32 packetSize);
	void HandleArsenal(ClientHandle clientHd, u16 netID, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_QuickRunArena(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_UNKNOWN_60118(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_GearUnequip(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_MasterGearRepair(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandleMasterGearRename(ClientHandle clientHd, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_BuyGift(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_SaveSettings(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_PveRanking(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_RequestSummaryInfoEach(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_RestartVote(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_PartySwapTeam(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_PartyAddBot(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_UNKNOWN_60082(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);

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
