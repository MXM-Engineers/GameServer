#include "channel.h"

#include "coordinator.h"
#include <common/packet_validator.h>
#include <mxm/game_content.h>
#include <mxm/account_progress.h>
#include "game.h"
#include "town_rules.h"
#include "arsenal.h"

static const u16 MEASURED_CQ_MASTERGEAR_RENAME = 60066;
static const u16 MEASURED_CQ_PVP_RECORD = 60228;
static const u16 MEASURED_CQ_SUMMARY_REWARD = 60046;
static const u16 MEASURED_CQ_TITLE_SELECT = 60056;
static const u16 MEASURED_CQ_MASTERGEAR_ADD = 60068;
static const u16 MEASURED_CQ_SELECT_SUPPORT_KIT = 60057;
static const u16 MEASURED_CQ_REGAME = 60093;
static const u16 MEASURED_CQ_GIFT_FRIENDS = 60293;
static const u16 MEASURED_CQ_STAY_JOIN = 60095;
static const u16 MEASURED_CQ_STAY_LEAVE = 60096;
static const u16 MEASURED_CQ_SERVER_UTC_TIME = 60131;

bool HubPacketHandler::Init(HubGame* game_)
{
	game = game_;
	replication = &game->replication;
	server = game->replication.server;
	return true;
}

void HubPacketHandler::OnClientsConnected(const eastl::pair<ClientHandle, const Account*>* clientList, const i32 count)
{
	for(int i = 0; i < count; i++) {
		auto& it = clientList[i];
		game->OnPlayerConnect(it.first, it.second);
	}
}

void HubPacketHandler::OnClientsDisconnected(const ClientHandle* clientList, const i32 count)
{
	for(int i = 0; i < count; i++) {
		ClientHandle clientHd = clientList[i];
		game->OnPlayerDisconnect(clientHd);
	}
}

void HubPacketHandler::OnNewPacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData)
{
	const i32 packetSize = header.size - sizeof(NetHeader);

#define HANDLE_CASE(PACKET) case Cl::PACKET::NET_ID: { HandlePacket_##PACKET(clientHd, header, packetData, packetSize); } break

	switch(header.netID) {
		HANDLE_CASE(CQ_GetGuildRankingSeasonList);
		HANDLE_CASE(CQ_TierRecord);

		HANDLE_CASE(CN_ReadyToLoadCharacter);
		HANDLE_CASE(CN_ReadyToLoadGameMap);
		HANDLE_CASE(CN_EnterCityScene);
		HANDLE_CASE(CQ_ReturnToCity);
		HANDLE_CASE(CQ_ReturnToGame);
		HANDLE_CASE(CQ_PartyInvite);
		HANDLE_CASE(CQ_PartyInviteAccept);
		HANDLE_CASE(CQ_BreakUpPartyRoom);
		HANDLE_CASE(CQ_LeavePartyRoom);
		HANDLE_CASE(CQ_CancelMatching);
		HANDLE_CASE(CQ_FriendRequest);
		HANDLE_CASE(CQ_FriendRequestRespond);
		HANDLE_CASE(CQ_FriendRemove);
		HANDLE_CASE(CQ_FriendSetFavorite);
		HANDLE_CASE(CQ_FriendUnsetFavorite);
		HANDLE_CASE(CQ_CommunityBlockAdd);
		HANDLE_CASE(CQ_CommunityBlockRemove);
		HANDLE_CASE(CQ_CommunityStatusMessage);
		HANDLE_CASE(CQ_Praise);
		HANDLE_CASE(CQ_Honor);
		HANDLE_CASE(CQ_NpBuyCshopItem);
		HANDLE_CASE(CQ_BuyCShopItem);
		HANDLE_CASE(CQ_BuyShopItem);
		HANDLE_CASE(CQ_InventoryExpand);
		HANDLE_CASE(CA_SetGameGvt);
		HANDLE_CASE(CA_CityLobbyJoinCity);
		HANDLE_CASE(CQ_GetCharacterInfo);
		HANDLE_CASE(CN_GamePlayerSyncByInt);
		HANDLE_CASE(CN_ChannelChatMessage);
		HANDLE_CASE(CQ_SetLeaderCharacter);
		HANDLE_CASE(CN_GamePlayerSyncActionStateOnly);
		HANDLE_CASE(CQ_JukeboxQueueSong);
		HANDLE_CASE(CQ_WhisperSend);
		HANDLE_CASE(CQ_PartyCreate);
		HANDLE_CASE(CQ_RTT_Time);
		HANDLE_CASE(CQ_LoadingProgressData);
		HANDLE_CASE(CQ_RequestCalendar);
		HANDLE_CASE(CQ_RequestAreaPopularity);
		HANDLE_CASE(CQ_PartyModify);
		HANDLE_CASE(CQ_PartyOptionModify);
		HANDLE_CASE(CQ_EnqueueGame);
		HANDLE_CASE(CA_SortieRoomFound);
		HANDLE_CASE(CN_SortieRoomConfirm);
		HANDLE_CASE(CQ_GetUserInfo);
		HANDLE_CASE(CQ_ItemTrade);
		HANDLE_CASE(CQ_ItemUse);
		HANDLE_CASE(CQ_SkinChipDissolve);
		HANDLE_CASE(CQ_GearEquip);
		HANDLE_CASE(CQ_PvpDetailRecord);
		HANDLE_CASE(CQ_UNKNOWN_60118);
		HANDLE_CASE(CQ_QuickRunArena);
		case MEASURED_CQ_PVP_RECORD: HandlePvpRecord(clientHd); break;
		case MEASURED_CQ_WEAPON_UNLOCK: case MEASURED_CQ_WEAPON_EQUIP: case MEASURED_CQ_SKILL_UNLOCK: case MEASURED_CQ_SKILL_EQUIP: case MEASURED_CQ_SKILL_UPGRADE: case MEASURED_CQ_WEAPON_UPGRADE: HandleArsenal(clientHd, header.netID, packetData, packetSize); break;
		HANDLE_CASE(CQ_GearUnequip);
		HANDLE_CASE(CQ_MasterGearRepair);
		case MEASURED_CQ_MASTERGEAR_RENAME: HandleMasterGearRename(clientHd, packetData, packetSize); break;
		HANDLE_CASE(CQ_BuyGift);
		HANDLE_CASE(CQ_PveRanking);
		HANDLE_CASE(CQ_SaveSettings);
		HANDLE_CASE(CQ_RequestSummaryInfoEach);
		case MEASURED_CQ_SUMMARY_REWARD: HandleSummaryReward(clientHd, packetData, packetSize); break;
		case MEASURED_CQ_TITLE_SELECT: HandleTitleSelect(clientHd, packetData, packetSize); break;
		case Cl::CN_SceneEnter::NET_ID: {
			static const u8 MEASURED_UNRANKED_TIER_STAGE_RECORD[] = { 0x01, 0x02, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x01, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
			static const u8 MEASURED_SCENE_STAGE_RECORDS = 2;
			NT_LOG("[client%x] Client :: CN_SceneEnter :: size=%d", clientHd, packetSize);
			if(packetSize != 2 || packetData[1] != MEASURED_SCENE_STAGE_RECORDS) break;
			PacketWriter<Sv::SA_TierStageRecord,64> answer;
			for(u8 b : MEASURED_UNRANKED_TIER_STAGE_RECORD) answer.Write<u8>(b);
			game->replication.SendWriter(clientHd, answer);
		} break;
		case Cl::CQ_PartyInviteRecommendResponse::NET_ID: {
			if(packetSize < 3) break;
			ConstBuffer buff(packetData, packetSize);
			const u16 len = buff.Read<u16>();
			if(packetSize != 2 + len * 2 + 1) break;
			WideString invitee;
			invitee.assign((const wchar*)buff.ReadRaw(len * 2), len);
			const bool accept = buff.Read<u8>() != 0;
			NT_LOG("[client%x] Client :: CQ_PartyInviteRecommendResponse :: %ls accept=%d", clientHd, invitee.data(), (i32)accept);
			game->OnPartyInviteRecommendResponse(clientHd, invitee, accept);
		} break;
		case Cl::CQ_SelectBadge::NET_ID: {
			if(packetSize != 1) break;
			NT_LOG("[client%x] Client :: CQ_SelectBadge :: %u", clientHd, (u32)packetData[0]);
			game->OnSelectBadge(clientHd, packetData[0]);
		} break;
		case MEASURED_CQ_SERVER_UTC_TIME: {
			static const i64 FILETIME_UNIX_EPOCH = 116444736000000000LL;
			static const i64 FILETIME_PER_SEC = 10000000LL;
			PacketWriter<Sv::SN_ServerUtcTime,16> answer;
			answer.Write<i64>(FILETIME_UNIX_EPOCH + (i64)time(nullptr) * FILETIME_PER_SEC);
			game->replication.SendWriter(clientHd, answer);
		} break;
		case MEASURED_CQ_STAY_JOIN:
		case MEASURED_CQ_STAY_LEAVE: {
			NT_LOG("[client%x] Client :: stay as a team :: %s", clientHd, header.netID == MEASURED_CQ_STAY_JOIN ? "join" : "leave");
			game->OnStayAsTeam(clientHd, header.netID == MEASURED_CQ_STAY_JOIN);
		} break;
		case MEASURED_CQ_GIFT_FRIENDS: {
			NT_LOG("[client%x] Client :: gift friends", clientHd);
			game->OnGiftFriends(clientHd);
		} break;
		case MEASURED_CQ_REGAME: {
			NT_LOG("[client%x] Client :: regame :: size=%d", clientHd, packetSize);
			game->OnRegame(clientHd);
		} break;
		case MEASURED_CQ_SELECT_SUPPORT_KIT: {
			if(packetSize != 5) break;
			ConstBuffer buff(packetData, packetSize);
			const u8 etcType = buff.Read<u8>();
			const i32 doc = buff.Read<i32>();
			NT_LOG("[client%x] Client :: select support kit :: type=%u doc=%d", clientHd, (u32)etcType, doc);
			game->OnSelectSupportKit(clientHd, etcType, doc);
		} break;
		case MEASURED_CQ_MASTERGEAR_ADD: NT_LOG("[client%x] Client :: master gear add", clientHd); game->OnMasterGearAdd(clientHd); break;
		HANDLE_CASE(CQ_RestartVote);
		HANDLE_CASE(CQ_PartySwapTeam);
		HANDLE_CASE(CQ_PartyAddBot);
		HANDLE_CASE(CQ_UNKNOWN_60082);
		HANDLE_CASE(CQ_MailShowList);
		HANDLE_CASE(CQ_MailRead);
		HANDLE_CASE(CQ_RequestMissionReward);
		HANDLE_CASE(CQ_DailyMissionChange);
		HANDLE_CASE(CQ_MailWrite);
		HANDLE_CASE(CQ_MailMove);
		HANDLE_CASE(CQ_MailGetAttachment);
		HANDLE_CASE(CQ_MailGetAttachmentMulti);

		case Cl::CQ_GetGuildProfile::NET_ID:
		case Cl::CQ_GetGuildMemberList::NET_ID:
		case Cl::CQ_GetGuildHistoryList::NET_ID:
		case Cl::CQ_GetGuildMemberStatusList::NET_ID:
		case Cl::CQ_CreateGuild::NET_ID:
		case Cl::CQ_DissolveGuild::NET_ID:
		case MEASURED_CQ_CANCEL_GUILD_DISSOLUTION:
		case Cl::CQ_QuitGuild::NET_ID:
		case Cl::CQ_KickGuildMember::NET_ID:
		case MEASURED_CQ_JOIN_GUILD:
		case Cl::CQ_GetGuildJoinRequestList::NET_ID:
		case Cl::CQ_RespondGuildJoinRequest::NET_ID:
		case Cl::CQ_InviteGuildMember::NET_ID:
		case Cl::CQ_GetGuildInvitationList::NET_ID:
		case Cl::CQ_RespondGuildInvitation::NET_ID:
		case Cl::CQ_GetGuildPublicProfile::NET_ID:
		case MEASURED_CQ_GET_GUILD_PUBLIC_PROFILE_FROM_USER:
		case Cl::CQ_EditGuildNotice::NET_ID:
		case Cl::CQ_EditGuildIntro::NET_ID:
		case Cl::CQ_SetGuildInterest::NET_ID:
		case Cl::CQ_SetGuildJoinType::NET_ID:
		case MEASURED_CQ_DONATE_TO_GUILD:
		case MEASURED_CQ_EDIT_GUILD_TAG:
		case MEASURED_CQ_SET_GUILD_EMBLEM:
		case MEASURED_CQ_SET_GUILD_MEMBER_CLASS:
		case MEASURED_CQ_SET_GUILD_MASTER:
		case MEASURED_CQ_CREATE_GUILD_MEMBERSHIP:
		case MEASURED_CQ_DELETE_GUILD_MEMBERSHIP:
		case MEASURED_CQ_EDIT_GUILD_MEMBERSHIP:
		case MEASURED_CQ_GET_GUILD_SKILL_LIST:
		case MEASURED_CQ_GET_GUILD_RANKING_LIST:
		case MEASURED_CQ_GET_GUILD_RANKING:
		case MEASURED_CQ_GET_GUILD_RANK_REWARD_INFO:
		case MEASURED_CQ_GET_PLAYING_GUILD_PVP_LIST:
		case MEASURED_CQ_BUY_GUILD_SKILL:
		case MEASURED_CQ_EXTEND_GUILD_LIMITED_SKILL:
		case Cl::CQ_GetRecommendedGuildList::NET_ID:
		case Cl::CQ_UNKNOWN_60172::NET_ID:
			game->OnGuildPacket(clientHd, header.netID, packetData, packetSize);
			break;

		default: {
			NT_LOG("[client%x] Client :: Unknown packet :: size=%d netID=%d", clientHd, header.size, header.netID);
		} break;
	}

#undef HANDLE_CASE
}

void HubPacketHandler::OnMatchmakerPacket(const NetHeader& header, const u8* packetData)
{
	const i32 packetSize = header.size - sizeof(NetHeader);

	switch(header.netID) {
		// TODO: move this one to connector
		case In::MR_Handshake::NET_ID: {
			const In::MR_Handshake resp = SafeCast<In::MR_Handshake>(packetData, packetSize);
			if(resp.result == 1) {
				LOG("[MM] Connected to Matchmaker server");
			}
			else {
				WARN("[MM] handshake failed (%d)", resp.result);
				ASSERT_MSG(0, "mm handshake failed"); // should not happen
			}
		} break;

		case In::MR_PartyCreated::NET_ID: {
			const In::MR_PartyCreated resp = SafeCast<In::MR_PartyCreated>(packetData, packetSize);
			game->MmOnPartyCreated(resp.partyUID, resp.leader);
		} break;

		case In::MR_PartyEnqueued::NET_ID: {
			const In::MR_PartyEnqueued resp = SafeCast<In::MR_PartyEnqueued>(packetData, packetSize);
			game->MmOnPartyEnqueued(resp.partyUID);
		} break;

		case In::MN_MatchingPartyFound::NET_ID: {
			const In::MN_MatchingPartyFound resp = SafeCast<In::MN_MatchingPartyFound>(packetData, packetSize);
			game->MmOnMatchFound(resp);
		} break;

		case In::MN_GameEnded::NET_ID: {
			const In::MN_GameEnded resp = SafeCast<In::MN_GameEnded>(packetData, packetSize);
			game->MmOnGameEnded(resp.sortieUID);
		} break;

		case In::MN_RoomCancelled::NET_ID: {
			const In::MN_RoomCancelled resp = SafeCast<In::MN_RoomCancelled>(packetData, packetSize);
			game->MmOnRoomCancelled(resp.sortieUID);
		} break;
	}
}

void HubPacketHandler::HandlePacket_CQ_GetGuildRankingSeasonList(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_GetGuildRankingSeasonList>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_GetGuildRankingSeasonList (size=%d)", clientHd, packetSize);
		return;
	}
	const Cl::CQ_GetGuildRankingSeasonList& rank = SafeCast<Cl::CQ_GetGuildRankingSeasonList>(packetData, packetSize);
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_GetGuildRankingSeasonList>(packetData, packetSize));

	// SA_GetGuildRankingSeasonList
	{
		PacketWriter<Sv::SA_GetGuildRankingSeasonList> packet;

		packet.Write<i32>(0); // result
		packet.Write<u8>(rank.rankingType); // result

		packet.Write<u16>(0); // rankingSeasonList_count

		SendPacket(clientHd, packet);
	}
}

void HubPacketHandler::HandlePacket_CQ_TierRecord(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_TierRecord>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_TierRecord (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_TierRecord>(packetData, packetSize));

	// SA_TierRecord
	{
		static const u8 MEASURED_TIER_SEASON = 1;
		static const u8 MEASURED_TIER_STAGE_TYPE = 2;
		static const u16 MEASURED_TIER_SECTOR = 11;
		static const u8 HYP_TIER_SECTOR_RANKING = 3;
		const AccountTier tier = AccountTierLoad(game->AccountNickname(clientHd));
		PacketWriter<Sv::SA_TierRecord> packet;
		packet.Write<u8>(MEASURED_TIER_SEASON);
		packet.Write<i32>(tier.win);
		packet.Write<i32>(tier.draw);
		packet.Write<i32>(tier.lose);
		packet.Write<i32>(tier.leave);
		packet.Write<u16>(1);
		packet.Write<u8>(MEASURED_TIER_STAGE_TYPE);
		packet.Write<u8>((u8)tier.tierType);
		packet.Write<u16>((u16)tier.points);
		packet.Write<u16>(MEASURED_TIER_SECTOR);
		packet.Write<u8>(HYP_TIER_SECTOR_RANKING);
		packet.Write<i32>(tier.win);
		packet.Write<i32>(tier.draw);
		packet.Write<i32>(tier.lose);
		packet.Write<i32>(tier.leave);

		SendPacket(clientHd, packet);
	}
}

void HubPacketHandler::HandlePacket_CN_ReadyToLoadCharacter(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CN_ReadyToLoadCharacter>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CN_ReadyToLoadCharacter (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: CN_ReadyToLoadCharacter ::", clientHd);
	game->OnPlayerReadyToLoad(clientHd);
}

void HubPacketHandler::HandlePacket_CN_EnterCityScene(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CN_EnterCityScene>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CN_EnterCityScene (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: CN_EnterCityScene ::", clientHd);
	game->OnPlayerEnterCity(clientHd);
}

void HubPacketHandler::HandlePacket_CQ_ReturnToCity(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%x] Client :: CQ_ReturnToCity ::", clientHd);
	game->OnReturnToCity(clientHd);
}

void HubPacketHandler::HandlePacket_CQ_ReturnToGame(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%x] Client :: CQ_ReturnToGame ::", clientHd);
	game->OnReturnToGame(clientHd);
}

void HubPacketHandler::HandlePacket_CQ_PartyInvite(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	ConstBuffer buff(packetData, packetSize);
	if(!buff.CanRead(2)) {
		WARN("[client%x] WARNING: invalid CQ_PartyInvite (size=%d)", clientHd, packetSize);
		return;
	}
	const u16 len = buff.Read<u16>();
	if(len == 0 || len > 32 || packetSize != 2 + len * 2 + 4) {
		WARN("[client%x] WARNING: invalid CQ_PartyInvite (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_PartyInvite>(packetData, packetSize));
	game->OnPartyInvite(clientHd, (const wchar*)buff.ReadRaw(len * 2), len);
}

void HubPacketHandler::HandlePacket_CQ_PartyInviteAccept(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	ConstBuffer buff(packetData, packetSize);
	if(!buff.CanRead(10)) {
		WARN("[client%x] WARNING: invalid CQ_PartyInviteAccept (size=%d)", clientHd, packetSize);
		return;
	}
	const i32 partyID = buff.Read<i32>();
	buff.Read<u32>();
	const u16 len = buff.Read<u16>();
	if(len > 32 || packetSize != 10 + len * 2) {
		WARN("[client%x] WARNING: invalid CQ_PartyInviteAccept (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_PartyInviteAccept>(packetData, packetSize));
	game->OnPartyInviteAccept(clientHd, partyID);
}

void HubPacketHandler::HandlePacket_CQ_BreakUpPartyRoom(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%x] Client :: CQ_BreakUpPartyRoom :: size=%d", clientHd, packetSize);
	game->OnPartyBreakUp(clientHd);
}

void HubPacketHandler::HandlePacket_CQ_LeavePartyRoom(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%x] Client :: CQ_LeavePartyRoom :: size=%d", clientHd, packetSize);
	game->OnPartyLeave(clientHd);
}

void HubPacketHandler::HandlePacket_CQ_CancelMatching(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%x] Client :: CQ_CancelMatching :: size=%d", clientHd, packetSize);
	game->OnCancelMatching(clientHd);
}

static bool ReadNickname(const u8* packetData, i32 packetSize, i32 trailing, WideString* out)
{
	ConstBuffer buff(packetData, packetSize);
	if(!buff.CanRead(2)) return false;
	const u16 len = buff.Read<u16>();
	if(len == 0 || len > 32 || packetSize != 2 + len * 2 + trailing) return false;
	out->assign((const wchar*)buff.ReadRaw(len * 2), len);
	return true;
}

void HubPacketHandler::HandlePacket_CQ_FriendRequest(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	WideString nickname;
	if(!ReadNickname(packetData, packetSize, 0, &nickname)) {
		WARN("[client%x] WARNING: invalid CQ_FriendRequest (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_FriendRequest>(packetData, packetSize));
	game->OnFriendRequest(clientHd, nickname);
}

void HubPacketHandler::HandlePacket_CQ_FriendRequestRespond(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	WideString nickname;
	if(!ReadNickname(packetData, packetSize, 1, &nickname)) {
		WARN("[client%x] WARNING: invalid CQ_FriendRequestRespond (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_FriendRequestRespond>(packetData, packetSize));
	game->OnFriendRespond(clientHd, nickname, packetData[packetSize - 1] != 0);
}

void HubPacketHandler::HandlePacket_CQ_FriendRemove(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	WideString nickname;
	if(!ReadNickname(packetData, packetSize, 0, &nickname)) {
		WARN("[client%x] WARNING: invalid CQ_FriendRemove (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_FriendRemove>(packetData, packetSize));
	game->OnFriendRemove(clientHd, nickname);
}

void HubPacketHandler::HandlePacket_CQ_FriendSetFavorite(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	WideString nickname;
	if(!ReadNickname(packetData, packetSize, 0, &nickname)) {
		WARN("[client%x] WARNING: invalid CQ_FriendSetFavorite (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: CQ_FriendSetFavorite :: nickname='%ls'", clientHd, nickname.data());
	game->OnFriendFavorite(clientHd, nickname, true);
}

void HubPacketHandler::HandlePacket_CQ_FriendUnsetFavorite(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	WideString nickname;
	if(!ReadNickname(packetData, packetSize, 0, &nickname)) {
		WARN("[client%x] WARNING: invalid CQ_FriendUnsetFavorite (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: CQ_FriendUnsetFavorite :: nickname='%ls'", clientHd, nickname.data());
	game->OnFriendFavorite(clientHd, nickname, false);
}

void HubPacketHandler::HandlePacket_CQ_CommunityBlockAdd(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	WideString nickname;
	if(!ReadNickname(packetData, packetSize, 0, &nickname)) {
		WARN("[client%x] WARNING: invalid CQ_CommunityBlockAdd (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: CQ_CommunityBlockAdd :: nickname='%ls'", clientHd, nickname.data());
	game->OnBlock(clientHd, nickname, true);
}

void HubPacketHandler::HandlePacket_CQ_CommunityBlockRemove(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	WideString nickname;
	if(!ReadNickname(packetData, packetSize, 0, &nickname)) {
		WARN("[client%x] WARNING: invalid CQ_CommunityBlockRemove (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: CQ_CommunityBlockRemove :: nickname='%ls'", clientHd, nickname.data());
	game->OnBlock(clientHd, nickname, false);
}

void HubPacketHandler::HandlePacket_CQ_CommunityStatusMessage(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	ConstBuffer buff(packetData, packetSize);
	if(!buff.CanRead(2)) {
		WARN("[client%x] WARNING: invalid CQ_CommunityStatusMessage (size=%d)", clientHd, packetSize);
		return;
	}
	const u16 len = buff.Read<u16>();
	if(len > 60 || packetSize != 2 + len * 2) {
		WARN("[client%x] WARNING: invalid CQ_CommunityStatusMessage (size=%d)", clientHd, packetSize);
		return;
	}
	WideString message;
	message.assign((const wchar*)buff.ReadRaw(len * 2), len);
	NT_LOG("[client%x] Client :: CQ_CommunityStatusMessage :: message='%ls'", clientHd, message.data());
	game->OnStatusMessage(clientHd, message);
}

void HubPacketHandler::HandlePacket_CQ_InventoryExpand(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_InventoryExpand>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_InventoryExpand (size=%d)", clientHd, packetSize);
		return;
	}
	const Cl::CQ_InventoryExpand& packet = SafeCast<Cl::CQ_InventoryExpand>(packetData, packetSize);
	NT_LOG("[client%x] Client :: CQ_InventoryExpand :: tab=%u", clientHd, packet.field1_id_id);
	game->OnInventoryExpand(clientHd, (i32)packet.field1_id_id);
}

void HubPacketHandler::HandlePacket_CQ_BuyCShopItem(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(packetSize != 8) {
		WARN("[client%x] WARNING: invalid CQ_BuyCShopItem (size=%d)", clientHd, packetSize);
		return;
	}
	ConstBuffer buff(packetData, packetSize);
	const i32 itemIndex = buff.Read<i32>();
	const i32 count = buff.Read<i32>();
	NT_LOG("[client%x] Client :: CQ_BuyCShopItem :: item=%d count=%d", clientHd, itemIndex, count);
	game->OnShopBuyItem(clientHd, (u32)itemIndex, count, itemIndex, 1, false);
}

void HubPacketHandler::HandlePacket_CQ_BuyShopItem(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_BuyShopItem>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_BuyShopItem (size=%d)", clientHd, packetSize);
		return;
	}
	const Cl::CQ_BuyShopItem& packet = SafeCast<Cl::CQ_BuyShopItem>(packetData, packetSize);
	LOG("[client%x] Client :: CQ_BuyShopItem :: player=%u item=%u op=%u", clientHd, packet.playerId, packet.itemId, packet.opFlag);
	game->OnGoldShopBuyItem(clientHd, (i32)packet.itemId);
}

void HubPacketHandler::HandlePacket_CQ_NpBuyCshopItem(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	ConstBuffer buff(packetData, packetSize);
	if(!buff.CanRead(44)) {
		WARN("[client%x] WARNING: invalid CQ_NpBuyCshopItem (size=%d)", clientHd, packetSize);
		return;
	}
	const u32 goodsID = buff.Read<u32>();
	const i32 quantity = buff.Read<i32>();
	buff.ReadRaw(34);
	const u16 textLen = buff.Read<u16>();
	if(textLen > 64 || !buff.CanRead(textLen * 2 + 2)) {
		WARN("[client%x] WARNING: invalid CQ_NpBuyCshopItem (size=%d)", clientHd, packetSize);
		return;
	}
	buff.ReadRaw(textLen * 2);
	const u16 entries = buff.Read<u16>();
	if(entries != 1 || !buff.CanRead(14)) {
		WARN("[client%x] WARNING: unsupported CQ_NpBuyCshopItem (size=%d entries=%u)", clientHd, packetSize, (u32)entries);
		return;
	}
	const i32 itemIndex = buff.Read<i32>();
	const i32 itemCount = buff.Read<i32>();
	NT_LOG("[client%x] Client :: CQ_NpBuyCshopItem :: goods=%u quantity=%d item=%d count=%d", clientHd, goodsID, quantity, itemIndex, itemCount);
	game->OnShopBuyItem(clientHd, goodsID, quantity, itemIndex, itemCount, true);
}

void HubPacketHandler::HandlePacket_CQ_Honor(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	WideString nickname;
	if(!ReadNickname(packetData, packetSize, 0, &nickname)) {
		WARN("[client%x] WARNING: invalid CQ_Honor (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: CQ_Honor :: nickname='%ls'", clientHd, nickname.data());
	game->OnHonor(clientHd, nickname);
}

void HubPacketHandler::HandlePacket_CQ_Praise(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	ConstBuffer buff(packetData, packetSize);
	if(!buff.CanRead(2)) {
		WARN("[client%x] WARNING: invalid CQ_Praise (size=%d)", clientHd, packetSize);
		return;
	}
	const u16 len = buff.Read<u16>();
	if(len == 0 || len > 32 || !buff.CanRead(len * 2) || packetSize != 2 + len * 2) {
		WARN("[client%x] WARNING: invalid CQ_Praise (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_Praise>(packetData, packetSize));
	game->OnPraise(clientHd, (const wchar*)buff.ReadRaw(len * 2), len);
}

void HubPacketHandler::HandlePacket_CN_ReadyToLoadGameMap(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CN_ReadyToLoadGameMap>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CN_ReadyToLoadGameMap (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: CN_ReadyToLoadGame ::", clientHd);
	game->OnPlayerReadyToLoad(clientHd);
}

void HubPacketHandler::HandlePacket_CA_SetGameGvt(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CA_SetGameGvt>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CA_SetGameGvt (size=%d)", clientHd, packetSize);
		return;
	}
	const Cl::CA_SetGameGvt& gvt = SafeCast<Cl::CA_SetGameGvt>(packetData, packetSize);
	NT_LOG("[client%x] Client :: CA_SetGameGvt :: sendTime=%d virtualTime=%d unk=%d", clientHd, gvt.sendTime, gvt.virtualTime, gvt.unk);
}

void HubPacketHandler::HandlePacket_CA_CityLobbyJoinCity(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CA_CityLobbyJoinCity>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CA_CityLobbyJoinCity (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: CA_CityLobbyJoinCity ::", clientHd);
	replication->SetPlayerAsInGame(clientHd);
}

void HubPacketHandler::HandlePacket_CQ_GetCharacterInfo(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_GetCharacterInfo>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_GetCharacterInfo (size=%d)", clientHd, packetSize);
		return;
	}
	const Cl::CQ_GetCharacterInfo& req = SafeCast<Cl::CQ_GetCharacterInfo>(packetData, packetSize);
	NT_LOG("[client%x] Client :: CQ_GetCharacterInfo :: characterID=0x%08x", clientHd, (u32)req.characterID);

	ActorUID actorUID = replication->GetWorldActorUID(clientHd, req.characterID);
	if(actorUID == ActorUID::INVALID) {
		WARN("Client sent an invalid actor (localActorID=%d)", req.characterID);
		return;
	}

	game->OnPlayerGetCharacterInfo(clientHd, actorUID);
}

void HubPacketHandler::HandlePacket_CN_GamePlayerSyncByInt(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CN_GamePlayerSyncByInt>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CN_GamePlayerSyncByInt (size=%d)", clientHd, packetSize);
		return;
	}
	const Cl::CN_GamePlayerSyncByInt& update = SafeCast<Cl::CN_GamePlayerSyncByInt>(packetData, packetSize);
	NT_LOG("[client%x] Client :: CN_GamePlayerSyncByInt :: { characterID=0x%08x p3nPos=(%g, %g, %g) p3nDir=(%g, %g, %g) p3nEye=(%g, %g, %g) nRotate=%g nSpeed=%g nState=%d nActionIDX=%d", clientHd, (u32)update.characterID, update.p3nPos.x, update.p3nPos.y, update.p3nPos.z, update.p3nDir.x, update.p3nDir.y, update.p3nDir.z, update.p3nEye.x, update.p3nEye.y, update.p3nEye.z, update.nRotate, update.nSpeed, (i32)update.nState, update.nActionIDX);

	ActorUID actorUID = replication->GetWorldActorUID(clientHd, update.characterID);
	if(actorUID == ActorUID::INVALID) {
		WARN("Client sent an invalid actor (localActorID=%d)", update.characterID);
		return;
	}

	game->OnPlayerUpdatePosition(clientHd, actorUID, f2v(update.p3nPos), f2v(update.p3nDir), f2v(update.p3nEye), update.nRotate, update.nSpeed, update.nState, update.nActionIDX);
}

void HubPacketHandler::HandlePacket_CN_ChannelChatMessage(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CN_ChannelChatMessage>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CN_ChannelChatMessage (size=%d)", clientHd, packetSize);
		return;
	}
	ConstBuffer buff(packetData, packetSize);
	i32 chatType = buff.Read<i32>();
	const u16 msgLen = buff.Read<u16>();
	const wchar* msg = (wchar*)buff.ReadRaw(msgLen * 2);

	NT_LOG("[client%x] Client :: CN_ChannelChatMessage :: chatType=%d msg='%.*S'", clientHd, chatType, msgLen, msg);

	game->OnPlayerChatMessage(clientHd, chatType, msg, msgLen);
}

void HubPacketHandler::HandlePacket_CQ_SetLeaderCharacter(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_SetLeaderCharacter>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_SetLeaderCharacter (size=%d)", clientHd, packetSize);
		return;
	}
	const Cl::CQ_SetLeaderCharacter& leader = SafeCast<Cl::CQ_SetLeaderCharacter>(packetData, packetSize);
	NT_LOG("[client%x] Client :: CQ_SetLeaderCharacter :: characterID=0x%08x skinIndex=%d", clientHd, (u32)leader.characterID, (i32)leader.skinIndex);

	game->OnPlayerSetLeaderCharacter(clientHd, leader.characterID, leader.skinIndex);
}

void HubPacketHandler::HandlePacket_CN_GamePlayerSyncActionStateOnly(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CN_GamePlayerSyncActionStateOnly>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CN_GamePlayerSyncActionStateOnly (size=%d)", clientHd, packetSize);
		return;
	}
	const Cl::CN_GamePlayerSyncActionStateOnly& sync = SafeCast<Cl::CN_GamePlayerSyncActionStateOnly>(packetData, packetSize);

	const char* stateStr = ActionStateToString(sync.state);

	NT_LOG("[client%x] Client :: CN_GamePlayerSyncActionStateOnly :: {", clientHd);
	NT_LOG("	characterID=0x%08x", (u32)sync.characterID);
	NT_LOG("	nState=%d (%s)", (i32)sync.state, stateStr);
	NT_LOG("	bApply=%d", sync.bApply);
	NT_LOG("	param1=%d", sync.param1);
	NT_LOG("	param2=%d", sync.param2);
	NT_LOG("	i4=%d", sync.i4);
	NT_LOG("	rotate=%g", sync.rotate);
	NT_LOG("	upperRotate=%g", sync.upperRotate);
	NT_LOG("}");

	ActorUID actorUID = replication->GetWorldActorUID(clientHd, sync.characterID);
	if(actorUID == ActorUID::INVALID) {
		WARN("Client sent an invalid actor (localActorID=%d)", sync.characterID);
		return;
	}

	game->OnPlayerSyncActionState(clientHd, actorUID, sync.state, sync.param1, sync.param2, sync.rotate, sync.upperRotate);
}

void HubPacketHandler::HandlePacket_CQ_JukeboxQueueSong(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_JukeboxQueueSong>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_JukeboxQueueSong (size=%d)", clientHd, packetSize);
		return;
	}
	const Cl::CQ_JukeboxQueueSong& queue = SafeCast<Cl::CQ_JukeboxQueueSong>(packetData, packetSize);
	NT_LOG("[client%x] Client :: CQ_JukeboxQueueSong :: { songID=%d }", clientHd, (i32)queue.songID);

	game->OnPlayerJukeboxQueueSong(clientHd, queue.songID);
}

void HubPacketHandler::HandlePacket_CQ_WhisperSend(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_WhisperSend>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_WhisperSend (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_WhisperSend>(packetData, packetSize));
	ConstBuffer buff(packetData, packetSize);
	WideString destNick;
	eastl::fixed_string<wchar,256,true> msg;

	const u16 destNick_len = buff.Read<u16>();
	const wchar* destNick_str = (wchar*)buff.ReadRaw(destNick_len * sizeof(wchar));
	const u16 msg_len = buff.Read<u16>();
	const wchar* msg_str = (wchar*)buff.ReadRaw(msg_len * sizeof(wchar));

	destNick.assign(destNick_str, destNick_len);
	msg.assign(msg_str, msg_len);

	game->OnPlayerChatWhisper(clientHd, destNick.data(), msg.data());
}

void HubPacketHandler::HandlePacket_CQ_RTT_Time(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_RTT_Time>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_RTT_Time (size=%d)", clientHd, packetSize);
		return;
	}
	const Cl::CQ_RTT_Time& rtt = SafeCast<Cl::CQ_RTT_Time>(packetData, packetSize);
	NT_LOG("[client%x] Client :: CQ_RTT_Time :: { time=%u }", clientHd, rtt.time);

	Sv::SA_RTT_Time answer;
	answer.clientTimestamp = rtt.time;
	answer.serverTimestamp = (i64)TimeDiffMs(TimeRelNow());
	SendPacket(clientHd, answer);
}

void HubPacketHandler::HandlePacket_CQ_LoadingProgressData(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_LoadingProgressData>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_LoadingProgressData (size=%d)", clientHd, packetSize);
		return;
	}
	const Cl::CQ_LoadingProgressData& loading = SafeCast<Cl::CQ_LoadingProgressData>(packetData, packetSize);
	NT_LOG("[client%x] Client :: CQ_LoadingProgressData :: { progress=%u }", clientHd, loading.progress);
}

void HubPacketHandler::HandlePacket_CQ_RequestCalendar(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_RequestCalendar>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_RequestCalendar (size=%d)", clientHd, packetSize);
		return;
	}
	const Cl::CQ_RequestCalendar& req = SafeCast<Cl::CQ_RequestCalendar>(packetData, packetSize);
	NT_LOG("[client%x] Client :: CQ_RequestCalendar :: { filetimeUTC=%llu }", clientHd, req.filetimeUTC);

	replication->SendCalendar(clientHd);
}

void HubPacketHandler::HandlePacket_CQ_RequestAreaPopularity(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_RequestAreaPopularity>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_RequestAreaPopularity (size=%d)", clientHd, packetSize);
		return;
	}
	const Cl::CQ_RequestAreaPopularity& req = SafeCast<Cl::CQ_RequestAreaPopularity>(packetData, packetSize);
	NT_LOG("[client%x] Client :: CQ_RequestAreaPopularity :: { area=%d }", clientHd, (i32)req.areaID);

	replication->SendAreaPopularity(clientHd, req.areaID);
}

void HubPacketHandler::HandlePacket_CQ_PartyCreate(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_PartyCreate>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_PartyCreate (size=%d)", clientHd, packetSize);
		return;
	}
	const Cl::CQ_PartyCreate& create = SafeCast<Cl::CQ_PartyCreate>(packetData, packetSize);
	NT_LOG("[client%x] Client :: CQ_PartyCreate :: { entrySysID=%d stageType=%d }", clientHd, create.entrySysID, create.stageType);

	game->OnCreateParty(clientHd, create.entrySysID, create.stageType);
}

void HubPacketHandler::HandlePacket_CQ_PartyModify(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_PartyModify>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_PartyModify (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_PartyModify>(packetData, packetSize));

	ConstBuffer buff(packetData, packetSize);
	const u16 stageCount = buff.Read<u16>();
	if(buff.CanRead((i32)stageCount * 4 + 4)) {
		const StageIndex stage = stageCount > 0 ? (StageIndex)buff.Read<i32>() : StageIndex(0);
		if(stageCount > 1) buff.ReadRaw((i32)(stageCount - 1) * 4);
		const GameType gameType = (GameType)buff.Read<i32>();
		const GameDefinition gameDefinition = buff.CanRead(4) ? (GameDefinition)buff.Read<i32>() : GameDefinition::System;
		const u8 selectMasterType = buff.CanRead(1) ? buff.Read<u8>() : 0;
		game->OnPartyModify(clientHd, stage, gameType, gameDefinition == GameDefinition::User ? GameDefinition::User : GameDefinition::System, selectMasterType);
	}

	Sv::SA_PartyModify packet;
	packet.retval = 0;
	SendPacket(clientHd, packet);
}

void HubPacketHandler::HandlePacket_CQ_PartyOptionModify(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_PartyOptionModify>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_PartyOptionModify (size=%d)", clientHd, packetSize);
		return;
	}
	const Cl::CQ_PartyOptionModify& req = SafeCast<Cl::CQ_PartyOptionModify>(packetData, packetSize);
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_PartyOptionModify>(packetData, packetSize));

	Sv::SA_PartyOptionModify packet;
	packet.retval = 0;
	packet.option = req.option;
	packet.enable = req.enable;
	SendPacket(clientHd, packet);
}

void HubPacketHandler::HandlePacket_CQ_EnqueueGame(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_EnqueueGame>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_EnqueueGame (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: CQ_EnqueueGame :: { }", clientHd);

	game->OnEnqueueGame(clientHd);
}

void HubPacketHandler::HandlePacket_CA_SortieRoomFound(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CA_SortieRoomFound>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CA_SortieRoomFound (size=%d)", clientHd, packetSize);
		return;
	}
	const Cl::CA_SortieRoomFound& packet = SafeCast<Cl::CA_SortieRoomFound>(packetData, packetSize);
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CA_SortieRoomFound>(packetData, packetSize));

	game->OnSortieRoomFound(clientHd, packet.sortieID);
}

void HubPacketHandler::HandlePacket_CN_SortieRoomConfirm(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CN_SortieRoomConfirm>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CN_SortieRoomConfirm (size=%d)", clientHd, packetSize);
		return;
	}
	const Cl::CN_SortieRoomConfirm& packet = SafeCast<Cl::CN_SortieRoomConfirm>(packetData, packetSize);
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CN_SortieRoomConfirm>(packetData, packetSize));

	game->OnSortieRoomConfirm(clientHd, packet.confirm);
}

void HubPacketHandler::HandlePacket_CQ_SaveSettings(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	ConstBuffer buff(packetData, packetSize);
	if(!buff.CanRead(3)) {
		WARN("[client%x] WARNING: invalid CQ_SaveSettings (size=%d)", clientHd, packetSize);
		return;
	}
	const u8 settingType = buff.Read<u8>();
	const u16 len = buff.Read<u16>();
	if(!buff.CanRead(len)) {
		WARN("[client%x] WARNING: invalid CQ_SaveSettings length %d (size=%d)", clientHd, (i32)len, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: CQ_SaveSettings :: type=%u len=%u", clientHd, (u32)settingType, (u32)len);
	game->OnSaveSettings(clientHd, settingType, (const u8*)buff.ReadRaw(len), len);
}

void HubPacketHandler::HandlePacket_CQ_PveRanking(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(packetSize != 4) {
		WARN("[client%x] WARNING: invalid CQ_PveRanking (size=%d)", clientHd, packetSize);
		return;
	}
	ConstBuffer buff(packetData, packetSize);
	const u32 stage = buff.Read<u32>();
	NT_LOG("[client%x] Client :: CQ_PveRanking :: stage=%u", clientHd, stage);
	replication->SendPveRanking(clientHd, stage);
}

void HubPacketHandler::HandleTitleSelect(ClientHandle clientHd, const u8* packetData, const i32 packetSize)
{
	if(packetSize != 8) {
		WARN("[client%x] WARNING: invalid title select (size=%d)", clientHd, packetSize);
		return;
	}
	ConstBuffer buff(packetData, packetSize);
	const i32 displayTitle = buff.Read<i32>();
	const i32 statTitle = buff.Read<i32>();
	NT_LOG("[client%x] Client :: title select :: display=%d stat=%d", clientHd, displayTitle, statTitle);
	game->OnTitleSelect(clientHd, displayTitle, statTitle);
}

void HubPacketHandler::HandleSummaryReward(ClientHandle clientHd, const u8* packetData, const i32 packetSize)
{
	if(packetSize != 12) {
		WARN("[client%x] WARNING: invalid summary reward request (size=%d)", clientHd, packetSize);
		return;
	}
	ConstBuffer buff(packetData, packetSize);
	const i32 summary = buff.Read<i32>();
	const u32 type = buff.Read<u32>();
	const i32 data = buff.Read<i32>();
	NT_LOG("[client%x] Client :: summary reward :: summary=%d type=%u data=%d", clientHd, summary, type, data);
	game->OnSummaryReward(clientHd, summary, (u8)type, data);
}

void HubPacketHandler::HandlePacket_CQ_RequestSummaryInfoEach(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(packetSize != 4) {
		WARN("[client%x] WARNING: invalid CQ_RequestSummaryInfoEach (size=%d)", clientHd, packetSize);
		return;
	}
	ConstBuffer buff(packetData, packetSize);
	const u32 summary = buff.Read<u32>();
	NT_LOG("[client%x] Client :: CQ_RequestSummaryInfoEach :: summary=%u", clientHd, summary);
	game->OnSummaryInfoEach(clientHd, (i32)summary);
}

void HubPacketHandler::HandlePacket_CQ_PartySwapTeam(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_PartySwapTeam>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_PartySwapTeam (size=%d)", clientHd, packetSize);
		return;
	}
	const Cl::CQ_PartySwapTeam& packet = SafeCast<Cl::CQ_PartySwapTeam>(packetData, packetSize);
	NT_LOG("[client%x] Client :: CQ_PartySwapTeam :: (%u,%u) <-> (%u,%u)", clientHd, packet.originTeam, (u32)packet.originSlot, packet.targetTeam, (u32)packet.targetSlot);
	game->OnPartySwapTeam(clientHd, packet.originTeam, packet.originSlot, packet.targetTeam, packet.targetSlot);
}

void HubPacketHandler::HandlePacket_CQ_UNKNOWN_60082(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_UNKNOWN_60082>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_PartyRemoveBot (size=%d)", clientHd, packetSize);
		return;
	}
	const Cl::CQ_UNKNOWN_60082& packet = SafeCast<Cl::CQ_UNKNOWN_60082>(packetData, packetSize);
	NT_LOG("[client%x] Client :: CQ_PartyRemoveBot :: userId=%u", clientHd, packet.wideStringField);
	game->OnPartyRemoveBot(clientHd, packet.wideStringField);
}

void HubPacketHandler::HandlePacket_CQ_PartyAddBot(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%x] Client :: CQ_PartyAddBot", clientHd);
	game->OnPartyAddBot(clientHd);
}

void HubPacketHandler::HandlePacket_CQ_RestartVote(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%x] Client :: CQ_RestartVote (start game) :: size=%d", clientHd, packetSize);
	game->OnStartGame(clientHd);
}

void HubPacketHandler::HandlePacket_CQ_BuyGift(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_BuyGift>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_BuyGift (size=%d)", clientHd, packetSize);
		return;
	}
	const Cl::CQ_BuyGift& packet = SafeCast<Cl::CQ_BuyGift>(packetData, packetSize);
	NT_LOG("[client%x] Client :: CQ_BuyGift (activity reward) :: tier=%u", clientHd, (u32)packet.amount_u8);
	game->OnActivityReward(clientHd, (i32)packet.amount_u8);
}

static const i64 PVP_FILETIME_UNIX_EPOCH = 116444736000000000LL;
static const i64 PVP_FILETIME_PER_SEC = 10000000LL;

void HubPacketHandler::HandlePacket_CQ_UNKNOWN_60118(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(packetSize != 4) return;
	const u32 state = SafeCast<Cl::CQ_UNKNOWN_60118>(packetData, packetSize).percentValue;
	NT_LOG("[client%x] Client :: friend state change :: %u", clientHd, state);
	game->OnFriendStateChange(clientHd, state);
}

void HubPacketHandler::HandlePacket_CQ_QuickRunArena(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%x] Client :: CQ_QuickRunArena", clientHd);
	game->OnQuickRunArena(clientHd);
}

void HubPacketHandler::HandleArsenal(ClientHandle clientHd, u16 netID, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%x] Client :: arsenal request %u (size=%d)", clientHd, (u32)netID, packetSize);
	game->OnArsenal(clientHd, netID, packetData, packetSize);
}

void HubPacketHandler::HandlePvpRecord(ClientHandle clientHd)
{
	const AccountPvpHistory history = AccountPvpHistoryLoad(game->AccountNickname(clientHd));
	NT_LOG("[client%x] Client :: CQ_PvpRecord", clientHd);
	PacketWriter<Sv::SA_PvpRecord,2048> packet;
	packet.Write<u16>((u16)history.stats.size());
	foreach_const(s, history.stats) {
		packet.Write<u8>(s->gameType);
		packet.Write<u8>(s->saveType);
		packet.Write<u32>(s->win);
		packet.Write<u32>(s->lose);
		packet.Write<u32>(s->draw);
		packet.Write<u32>(s->leave);
		packet.Write<u32>(s->kill);
		packet.Write<u32>(s->death);
		packet.Write<u32>(s->assist);
		packet.Write<u32>(s->score);
		packet.Write<u32>(s->captures);
	}
	packet.Write<u16>(0);
	SendPacket(clientHd, packet);
}

void HubPacketHandler::HandlePacket_CQ_PvpDetailRecord(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_PvpDetailRecord>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_PvpDetailRecord (size=%d)", clientHd, packetSize);
		return;
	}
	const Cl::CQ_PvpDetailRecord& req = SafeCast<Cl::CQ_PvpDetailRecord>(packetData, packetSize);
	NT_LOG("[client%x] Client :: CQ_PvpDetailRecord :: gameType=%u saveType=%u", clientHd, req.gameType, (u32)req.pvpSaveType);
	const AccountPvpHistory history = AccountPvpHistoryLoad(game->AccountNickname(clientHd));
	eastl::fixed_vector<eastl::pair<i32,eastl::array<u32,2>>,64,false> classes;
	auto count = [&classes](i32 cls, bool won) {
		foreach(c, classes) if(c->first == cls) { c->second[0]++; if(won) c->second[1]++; return; }
		if(!classes.full()) classes.push_back(eastl::make_pair(cls, eastl::array<u32,2>{{ 1u, won ? 1u : 0u }}));
	};
	eastl::fixed_vector<const AccountPvpRecent*,40,false> recent;
	foreach_const(r, history.recent) {
		if(r->gameType != (u8)req.gameType || r->saveType != req.pvpSaveType) continue;
		recent.push_back(&*r);
		for(i32 c : r->classes) count(c, r->result == 1);
	}
	u32 mostPlay = 0, mostPlayCount = 0, mostWin = 0, mostWinCount = 0;
	foreach_const(c, classes) {
		if(c->second[0] > mostPlayCount) { mostPlay = (u32)c->first; mostPlayCount = c->second[0]; }
		if(c->second[1] > mostWinCount) { mostWin = (u32)c->first; mostWinCount = c->second[1]; }
	}
	PacketWriter<Sv::SA_PvpDetailRecord,2048> packet;
	packet.Write<u32>(req.gameType);
	packet.Write<u8>(req.pvpSaveType);
	packet.Write<u32>(mostPlay);
	packet.Write<u32>(mostPlayCount);
	packet.Write<u32>(mostWin);
	packet.Write<u32>(mostWinCount);
	packet.Write<u16>((u16)recent.size());
	for(i32 i = (i32)recent.size() - 1; i >= 0; i--) {
		const AccountPvpRecent& r = *recent[i];
		packet.Write<u8>(r.result);
		packet.Write<u32>((u32)r.classes[0]);
		packet.Write<u32>((u32)r.classes[1]);
		packet.Write<u16>((u16)r.myScore);
		packet.Write<u16>((u16)r.myTeamScore);
		packet.Write<u16>((u16)r.enemyTeamScore);
		packet.Write<u8>(r.myCount);
		packet.Write<u8>(r.enemyCount);
		packet.Write<u8>(r.kill);
		packet.Write<u8>(r.death);
		packet.Write<u8>(r.assist);
		packet.Write<u8>(r.occupy);
		packet.Write<u32>((u32)r.myScore);
		packet.Write<i64>(r.beginTime * PVP_FILETIME_PER_SEC + PVP_FILETIME_UNIX_EPOCH);
	}
	packet.Write<u32>(0);
	packet.Write<u32>(0);
	packet.Write<u16>(0);
	SendPacket(clientHd, packet);
}

void HubPacketHandler::HandlePacket_CQ_GearEquip(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	ConstBuffer buff(packetData, packetSize);
	if(packetSize != 9) {
		WARN("[client%x] WARNING: invalid CQ_GearEquip (size=%d)", clientHd, packetSize);
		return;
	}
	const u8 page = buff.Read<u8>();
	const u32 uid = buff.Read<u32>();
	const u32 slot = buff.Read<u32>();
	NT_LOG("[client%x] Client :: CQ_GearEquip :: page=%d item=0x%x slot=%u", clientHd, (i32)page, uid, slot);
	game->OnGearEquip(clientHd, page, uid, slot);
}

void HubPacketHandler::HandlePacket_CQ_GearUnequip(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	ConstBuffer buff(packetData, packetSize);
	if(packetSize != 5) {
		WARN("[client%x] WARNING: unexpected CQ_GearUnequip (size=%d)", clientHd, packetSize);
		return;
	}
	const u8 page = buff.Read<u8>();
	const u32 slot = buff.Read<u32>();
	NT_LOG("[client%x] Client :: CQ_GearUnequip :: page=%d slot=%u", clientHd, (i32)page, slot);
	game->OnGearUnequip(clientHd, page, slot);
}

void HubPacketHandler::HandlePacket_CQ_MasterGearRepair(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	ConstBuffer buff(packetData, packetSize);
	if(packetSize != 5) {
		WARN("[client%x] WARNING: invalid master gear select (size=%d)", clientHd, packetSize);
		return;
	}
	const u32 characterID = buff.Read<u32>();
	const u8 page = buff.Read<u8>();
	NT_LOG("[client%x] Client :: master gear select :: character=0x%x page=%d", clientHd, characterID, (i32)page);
	game->OnMasterGearSelect(clientHd, characterID, page);
}

void HubPacketHandler::HandleMasterGearRename(ClientHandle clientHd, const u8* packetData, const i32 packetSize)
{
	ConstBuffer buff(packetData, packetSize);
	if(!buff.CanRead(3)) return;
	const u8 page = buff.Read<u8>();
	const u16 len = buff.Read<u16>();
	if(len > 32 || packetSize != 3 + len * 2) {
		WARN("[client%x] WARNING: unexpected master gear rename (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: master gear rename :: page=%d", clientHd, (i32)page);
	game->OnMasterGearRename(clientHd, page, (const wchar*)buff.ReadRaw(len * 2), len);
}

void HubPacketHandler::HandlePacket_CQ_ItemTrade(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_ItemTrade>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_ItemTrade (size=%d)", clientHd, packetSize);
		return;
	}
	const Cl::CQ_ItemTrade& packet = SafeCast<Cl::CQ_ItemTrade>(packetData, packetSize);
	NT_LOG("[client%x] Client :: CQ_ItemTrade :: trader=%u trade=%u count=%u", clientHd, packet.unkContextId, packet.itemDocId, (u32)packet.count);
	game->OnItemTrade(clientHd, (i32)packet.itemDocId, (i32)packet.count);
}

void HubPacketHandler::HandlePacket_CQ_SkinChipDissolve(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_SkinChipDissolve>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_SkinChipDissolve (size=%d)", clientHd, packetSize);
		return;
	}
	const Cl::CQ_SkinChipDissolve& packet = SafeCast<Cl::CQ_SkinChipDissolve>(packetData, packetSize);
	LOG("[client%x] Client :: CQ_SkinChipDissolve :: type=%u trade=%u count=%u", clientHd, (u32)packet.unkZero, packet.chipItemDocId, (u32)packet.craftCount);
	game->OnItemTrade(clientHd, (i32)packet.chipItemDocId, (i32)packet.craftCount, true, packet.unkZero);
}

void HubPacketHandler::HandlePacket_CQ_ItemUse(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_ItemUse>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_ItemUse (size=%d)", clientHd, packetSize);
		return;
	}
	const Cl::CQ_ItemUse& packet = SafeCast<Cl::CQ_ItemUse>(packetData, packetSize);
	NT_LOG("[client%x] Client :: CQ_ItemUse :: item=%u", clientHd, (u32)packet.itemInstanceUID);
	game->OnItemUse(clientHd, (u32)packet.itemInstanceUID);
}

void HubPacketHandler::HandlePacket_CQ_GetUserInfo(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_GetUserInfo>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_GetUserInfo (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_GetUserInfo>(packetData, packetSize));
	replication->SendProfileCharactersWeapons(clientHd);
	replication->SendProfileSkillsSkins(clientHd);
}

void HubPacketHandler::HandlePacket_CQ_MailShowList(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_MailShowList>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_MailShowList (size=%d)", clientHd, packetSize);
		return;
	}
	const Cl::CQ_MailShowList& packet = SafeCast<Cl::CQ_MailShowList>(packetData, packetSize);
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_MailShowList>(packetData, packetSize));

	game->OnMailShowList(clientHd, packet.mailboxType);
}

void HubPacketHandler::HandlePacket_CQ_MailRead(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_MailRead>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_MailRead (size=%d)", clientHd, packetSize);
		return;
	}
	const Cl::CQ_MailRead& packet = SafeCast<Cl::CQ_MailRead>(packetData, packetSize);
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_MailRead>(packetData, packetSize));

	game->OnMailRead(clientHd, packet.target_user_id_u64);
}

void HubPacketHandler::HandlePacket_CQ_RequestMissionReward(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_RequestMissionReward>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_RequestMissionReward (size=%d)", clientHd, packetSize);
		return;
	}
	const Cl::CQ_RequestMissionReward& packet = SafeCast<Cl::CQ_RequestMissionReward>(packetData, packetSize);
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_RequestMissionReward>(packetData, packetSize));

	game->OnMissionReward(clientHd, (i32)packet.missionIndex);
}

void HubPacketHandler::HandlePacket_CQ_DailyMissionChange(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_DailyMissionChange>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_DailyMissionChange (size=%d)", clientHd, packetSize);
		return;
	}
	const Cl::CQ_DailyMissionChange& packet = SafeCast<Cl::CQ_DailyMissionChange>(packetData, packetSize);
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_DailyMissionChange>(packetData, packetSize));

	game->OnDailyMissionChange(clientHd, (i32)packet.step);
}

void HubPacketHandler::HandlePacket_CQ_MailWrite(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_MailWrite>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_MailWrite (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_MailWrite>(packetData, packetSize));
	ConstBuffer buff(packetData, packetSize);
	const u16 toLen = buff.Read<u16>();
	const wchar* to = (const wchar*)buff.ReadRaw(toLen * sizeof(wchar));
	const u16 subjectLen = buff.Read<u16>();
	const wchar* subject = (const wchar*)buff.ReadRaw(subjectLen * sizeof(wchar));
	const u16 contentLen = buff.Read<u16>();
	const wchar* content = (const wchar*)buff.ReadRaw(contentLen * sizeof(wchar));

	WideString toNick;
	toNick.assign(to, toLen);
	game->OnMailWrite(clientHd, toNick.data(), subject, subjectLen, content, contentLen);
}

void HubPacketHandler::HandlePacket_CQ_MailGetAttachment(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_MailGetAttachment>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_MailGetAttachment (size=%d)", clientHd, packetSize);
		return;
	}
	const Cl::CQ_MailGetAttachment& packet = SafeCast<Cl::CQ_MailGetAttachment>(packetData, packetSize);
	NT_LOG("[client%x] Client :: CQ_MailGetAttachment :: mail=%llu", clientHd, (unsigned long long)packet.mailUID);
	game->OnMailGetAttachments(clientHd, &packet.mailUID, 1);
}

void HubPacketHandler::HandlePacket_CQ_MailGetAttachmentMulti(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_MailGetAttachmentMulti>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_MailGetAttachmentMulti (size=%d)", clientHd, packetSize);
		return;
	}
	ConstBuffer buff(packetData, packetSize);
	const u16 count = buff.Read<u16>();
	eastl::fixed_vector<u64,64,true> ids;
	for(u16 i = 0; i < count; i++) ids.push_back(buff.Read<u64>());
	NT_LOG("[client%x] Client :: CQ_MailGetAttachmentMulti :: %u mail(s)", clientHd, (u32)count);
	game->OnMailGetAttachments(clientHd, ids.data(), (i32)ids.size());
}

void HubPacketHandler::HandlePacket_CQ_MailMove(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	if(!ValidatePacket<Cl::CQ_MailMove>(packetData, packetSize)) {
		WARN("[client%x] WARNING: invalid CQ_MailMove (size=%d)", clientHd, packetSize);
		return;
	}
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_MailMove>(packetData, packetSize));
	ConstBuffer buff(packetData, packetSize);
	const u8 moveType = buff.Read<u8>();
	const u16 count = buff.Read<u16>();
	eastl::fixed_vector<u64,64,true> ids;
	for(u16 i = 0; i < count; i++) ids.push_back(buff.Read<u64>());

	game->OnMailMove(clientHd, moveType, ids.data(), (i32)ids.size());
}
