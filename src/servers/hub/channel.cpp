#include "channel.h"

#include "coordinator.h"
#include <mxm/game_content.h>
#include "game.h"

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
		HANDLE_CASE(CQ_GetGuildProfile);
		HANDLE_CASE(CQ_GetGuildMemberList);
		HANDLE_CASE(CQ_GetGuildHistoryList);
		HANDLE_CASE(CQ_GetGuildRankingSeasonList);
		HANDLE_CASE(CQ_TierRecord);

		HANDLE_CASE(CN_ReadyToLoadCharacter);
		HANDLE_CASE(CN_ReadyToLoadGameMap);
		HANDLE_CASE(CA_SetGameGvt);
		HANDLE_CASE(CN_MapIsLoaded);
		HANDLE_CASE(CQ_GetCharacterInfo);
		HANDLE_CASE(CN_UpdatePosition);
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

		case In::MN_MatchFound::NET_ID: {
			const In::MN_MatchFound resp = SafeCast<In::MN_MatchFound>(packetData, packetSize);
			game->MmOnMatchFound(resp.partyUID, resp.sortieUID);
		} break;
	}
}

void HubPacketHandler::HandlePacket_CQ_GetGuildProfile(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_GetGuildProfile>(packetData, packetSize));

	// SA_GetGuildProfile
	{
		PacketWriter<Sv::SA_GetGuildProfile,512> packet;

		packet.Write<i32>(0); // result;
		packet.WriteStringObj(L"Alpha testers"); // guildName
		packet.WriteStringObj(L"Alpha"); // guildTag
		packet.Write<i32>(100203); // emblemIndex
		packet.Write<u8>(10); // guildLvl
		packet.Write<u8>(120); // memberMax
		packet.WriteStringObj(L"Malachi"); // ownerNickname
		packet.Write<i64>(131474874000000000); // createdDate
		packet.Write<i64>(0); // dissolutionDate
		packet.Write<u8>(0); // joinType

		Sv::SA_GetGuildProfile::ST_GuildInterest guildInterest;
		guildInterest.likePveStage = 1;
		guildInterest.likeDefence = 1;
		guildInterest.likePvpNormal = 1;
		guildInterest.likePvpOccupy = 1;
		guildInterest.likePvpGot = 1;
		guildInterest.likePvpRank = 1;
		guildInterest.likeOlympic = 1;
		packet.Write(guildInterest);

		packet.WriteStringObj(L"This is a great intro"); // guildIntro
		packet.WriteStringObj(L"Notice: this game is dead! (for now)"); // guildNotice
		packet.Write<i32>(460281); // guildPoint
		packet.Write<i32>(9999); // guildFund

		Sv::SA_GetGuildProfile::ST_GuildPvpRecord guildPvpRecord;
		guildPvpRecord.rp = 5;
		guildPvpRecord.win = 4;
		guildPvpRecord.draw = 3;
		guildPvpRecord.lose = 2;
		packet.Write(guildPvpRecord);

		packet.Write<i32>(-1); // guildRankNo

		packet.Write<u16>(1); // guildMemberClassList_count
		// guildMemberClassList[0]
		packet.Write<i32>(12456); // id
		packet.Write<u8>(3); // type
		packet.Write<u8>(2); // iconIndex
		packet.WriteStringObj(L"Malachi");

		Sv::SA_GetGuildProfile::ST_GuildMemberRights rights;
		rights.hasInviteRight = 1;
		rights.hasExpelRight = 1;
		rights.hasMembershipChgRight = 1;
		rights.hasClassAssignRight = 1;
		rights.hasNoticeChgRight = 1;
		rights.hasIntroChgRight = 1;
		rights.hasInterestChgRight = 1;
		rights.hasFundManageRight = 1;
		rights.hasJoinTypeRight = 1;
		rights.hasEmblemRight = 1;
		packet.Write(rights);

		packet.Write<u16>(1); // guildSkills_count
		// guildSkills[0]
		packet.Write<u8>(1); // type
		packet.Write<u8>(9); // level
		packet.Write<i64>(0); // expiryDate
		packet.Write<u16>(0); // extensionCount

		packet.Write<i32>(7); // curDailyStageGuildPoint
		packet.Write<i32>(500); // maxDailyStageGuildPoint
		packet.Write<i32>(2); // curDailyArenaGuildPoint
		packet.Write<i32>(450); // maxDailyArenaGuildPoint
		packet.Write<u8>(1); // todayRollCallCount

		SendPacket(clientHd, packet);
	}
}

void HubPacketHandler::HandlePacket_CQ_GetGuildMemberList(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_GetGuildMemberList>(packetData, packetSize));

	// SA_GetGuildMemberList
	{
		PacketWriter<Sv::SA_GetGuildMemberList,512> packet;

		packet.Write<i32>(0); // result

		packet.Write<u16>(3); // guildMemberProfileList_count

		// guildMemberProfileList[0]
		packet.WriteStringObj(L"Malachi");
		packet.Write<i32>(0);  // membershipID
		packet.Write<u16>(99); // lvl
		packet.Write<u16>(10); // leaderClassType
		packet.Write<u16>(27); // masterCount
		packet.Write<i32>(12455); // achievementScore
		packet.Write<u8>(0); // topPvpTierGrade
		packet.Write<u16>(0); // topPvpTierPoint
		packet.Write<i32>(16965); // contributedGuildPoint
		packet.Write<i32>(60047); // contributedGuildFund
		packet.Write<u16>(0); // guildPvpWin
		packet.Write<u16>(0); // guildPvpPlay
		packet.Write<i64>((i64)131568669600000000); // lastLogoutDate

		// guildMemberProfileList[1]
		packet.WriteStringObj(L"Delta-47");
		packet.Write<i32>(0);  // membershipID
		packet.Write<u16>(99); // lvl
		packet.Write<u16>(10); // leaderClassType
		packet.Write<u16>(27); // masterCount
		packet.Write<i32>(12455); // achievementScore
		packet.Write<u8>(0); // topPvpTierGrade
		packet.Write<u16>(0); // topPvpTierPoint
		packet.Write<i32>(16965); // contributedGuildPoint
		packet.Write<i32>(60047); // contributedGuildFund
		packet.Write<u16>(0); // guildPvpWin
		packet.Write<u16>(0); // guildPvpPlay
		packet.Write<i64>((i64)131568669600000000); // lastLogoutDate

		// guildMemberProfileList[2]
		packet.WriteStringObj(L"LordSk");
		packet.Write<i32>(0);  // membershipID
		packet.Write<u16>(99); // lvl
		packet.Write<u16>(10); // leaderClassType
		packet.Write<u16>(27); // masterCount
		packet.Write<i32>(12455); // achievementScore
		packet.Write<u8>(0); // topPvpTierGrade
		packet.Write<u16>(0); // topPvpTierPoint
		packet.Write<i32>(16965); // contributedGuildPoint
		packet.Write<i32>(60047); // contributedGuildFund
		packet.Write<u16>(0); // guildPvpWin
		packet.Write<u16>(0); // guildPvpPlay
		packet.Write<i64>((i64)131568669600000000); // lastLogoutDate

		SendPacket(clientHd, packet);
	}
}

void HubPacketHandler::HandlePacket_CQ_GetGuildHistoryList(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_GetGuildHistoryList>(packetData, packetSize));

	// SA_GetGuildMemberList
	{
		PacketWriter<Sv::SA_GetGuildHistoryList> packet;

		packet.Write<i32>(0); // result

		packet.Write<u16>(0); // guildHistories_count

		SendPacket(clientHd, packet);
	}
}

void HubPacketHandler::HandlePacket_CQ_GetGuildRankingSeasonList(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
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
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_TierRecord>(packetData, packetSize));

	// SA_TierRecord
	{
		PacketWriter<Sv::SA_TierRecord> packet;

		packet.Write<u8>(1); // seasonId
		packet.Write<i32>(0); // allTierWin
		packet.Write<i32>(0); // allTierDraw
		packet.Write<i32>(0); // allTierLose
		packet.Write<i32>(0); // allTierLeave
		packet.Write<u16>(0); // stageRecordList_count

		SendPacket(clientHd, packet);
	}
}

void HubPacketHandler::HandlePacket_CN_ReadyToLoadCharacter(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%x] Client :: CN_ReadyToLoadCharacter ::", clientHd);
	game->OnPlayerReadyToLoad(clientHd);
}

void HubPacketHandler::HandlePacket_CN_ReadyToLoadGameMap(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%x] Client :: CN_ReadyToLoadGame ::", clientHd);
	game->OnPlayerReadyToLoad(clientHd);
}

void HubPacketHandler::HandlePacket_CA_SetGameGvt(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CA_SetGameGvt& gvt = SafeCast<Cl::CA_SetGameGvt>(packetData, packetSize);
	NT_LOG("[client%x] Client :: CA_SetGameGvt :: sendTime=%d virtualTime=%d unk=%d", clientHd, gvt.sendTime, gvt.virtualTime, gvt.unk);
}

void HubPacketHandler::HandlePacket_CN_MapIsLoaded(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%x] Client :: CN_MapIsLoaded ::", clientHd);
	replication->SetPlayerAsInGame(clientHd);
}

void HubPacketHandler::HandlePacket_CQ_GetCharacterInfo(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_GetCharacterInfo& req = SafeCast<Cl::CQ_GetCharacterInfo>(packetData, packetSize);
	NT_LOG("[client%x] Client :: CQ_GetCharacterInfo :: characterID=%d", clientHd, (u32)req.characterID);

	ActorUID actorUID = replication->GetWorldActorUID(clientHd, req.characterID);
	if(actorUID == ActorUID::INVALID) {
		WARN("Client sent an invalid actor (localActorID=%d)", req.characterID);
		return;
	}

	game->OnPlayerGetCharacterInfo(clientHd, actorUID);
}

void HubPacketHandler::HandlePacket_CN_UpdatePosition(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CN_UpdatePosition& update = SafeCast<Cl::CN_UpdatePosition>(packetData, packetSize);
	NT_LOG("[client%x] Client :: CN_UpdatePosition :: { characterID=%d p3nPos=(%g, %g, %g) p3nDir=(%g, %g, %g) p3nEye=(%g, %g, %g) nRotate=%g nSpeed=%g nState=%d nActionIDX=%d", clientHd, (u32)update.characterID, update.p3nPos.x, update.p3nPos.y, update.p3nPos.z, update.p3nDir.x, update.p3nDir.y, update.p3nDir.z, update.p3nEye.x, update.p3nEye.y, update.p3nEye.z, update.nRotate, update.nSpeed, (i32)update.nState, update.nActionIDX);

	ActorUID actorUID = replication->GetWorldActorUID(clientHd, update.characterID);
	if(actorUID == ActorUID::INVALID) {
		WARN("Client sent an invalid actor (localActorID=%d)", update.characterID);
		return;
	}

	game->OnPlayerUpdatePosition(clientHd, actorUID, f2v(update.p3nPos), f2v(update.p3nDir), f2v(update.p3nEye), update.nRotate, update.nSpeed, update.nState, update.nActionIDX);
}

void HubPacketHandler::HandlePacket_CN_ChannelChatMessage(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	ConstBuffer buff(packetData, packetSize);
	i32 chatType = buff.Read<i32>();
	const u16 msgLen = buff.Read<u16>();
	const wchar* msg = (wchar*)buff.ReadRaw(msgLen * 2);

	NT_LOG("[client%x] Client :: CN_ChannelChatMessage :: chatType=%d msg='%.*S'", clientHd, chatType, msgLen, msg);

	game->OnPlayerChatMessage(clientHd, chatType, msg, msgLen);
}

void HubPacketHandler::HandlePacket_CQ_SetLeaderCharacter(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_SetLeaderCharacter& leader = SafeCast<Cl::CQ_SetLeaderCharacter>(packetData, packetSize);
	NT_LOG("[client%x] Client :: CQ_SetLeaderCharacter :: characterID=%d skinIndex=%d", clientHd, (u32)leader.characterID, (i32)leader.skinIndex);

	game->OnPlayerSetLeaderCharacter(clientHd, leader.characterID, leader.skinIndex);
}

void HubPacketHandler::HandlePacket_CN_GamePlayerSyncActionStateOnly(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CN_GamePlayerSyncActionStateOnly& sync = SafeCast<Cl::CN_GamePlayerSyncActionStateOnly>(packetData, packetSize);

	i32 state = (i32)sync.state;
	const char* stateStr = g_ActionStateInvalidString;
	if(state >= 0 && state < ARRAY_COUNT(g_ActionStateString)) {
		stateStr = g_ActionStateString[state];
	}

	NT_LOG("[client%x] Client :: CN_GamePlayerSyncActionStateOnly :: {", clientHd);
	NT_LOG("	characterID=%d", (u32)sync.characterID);
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
	const Cl::CQ_JukeboxQueueSong& queue = SafeCast<Cl::CQ_JukeboxQueueSong>(packetData, packetSize);
	NT_LOG("[client%x] Client :: CQ_JukeboxQueueSong :: { songID=%d }", clientHd, (i32)queue.songID);

	game->OnPlayerJukeboxQueueSong(clientHd, queue.songID);
}

void HubPacketHandler::HandlePacket_CQ_WhisperSend(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
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
	const Cl::CQ_RTT_Time& rtt = SafeCast<Cl::CQ_RTT_Time>(packetData, packetSize);
	NT_LOG("[client%x] Client :: CQ_RTT_Time :: { time=%u }", clientHd, rtt.time);

	Sv::SA_RTT_Time answer;
	answer.clientTimestamp = rtt.time;
	answer.serverTimestamp = (i64)TimeDiffMs(TimeRelNow());
	SendPacket(clientHd, answer);
}

void HubPacketHandler::HandlePacket_CQ_LoadingProgressData(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_LoadingProgressData& loading = SafeCast<Cl::CQ_LoadingProgressData>(packetData, packetSize);
	NT_LOG("[client%x] Client :: CQ_LoadingProgressData :: { progress=%u }", clientHd, loading.progress);
}

void HubPacketHandler::HandlePacket_CQ_RequestCalendar(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_RequestCalendar& req = SafeCast<Cl::CQ_RequestCalendar>(packetData, packetSize);
	NT_LOG("[client%x] Client :: CQ_RequestCalendar :: { filetimeUTC=%llu }", clientHd, req.filetimeUTC);

	replication->SendCalendar(clientHd);
}

void HubPacketHandler::HandlePacket_CQ_RequestAreaPopularity(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_RequestAreaPopularity& req = SafeCast<Cl::CQ_RequestAreaPopularity>(packetData, packetSize);
	NT_LOG("[client%x] Client :: CQ_RequestAreaPopularity :: { area=%u }", clientHd, req.areaID);

	replication->SendAreaPopularity(clientHd, req.areaID);
}

void HubPacketHandler::HandlePacket_CQ_PartyCreate(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_PartyCreate& create = SafeCast<Cl::CQ_PartyCreate>(packetData, packetSize);
	NT_LOG("[client%x] Client :: CQ_PartyCreate :: { entrySysID=%d stageType=%d }", clientHd, create.entrySysID, create.stageType);

	game->OnCreateParty(clientHd, create.entrySysID, create.stageType);
}

void HubPacketHandler::HandlePacket_CQ_PartyModify(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_PartyModify>(packetData, packetSize));

	Sv::SA_PartyModify packet;
	packet.retval = 0;
	SendPacket(clientHd, packet);
}

void HubPacketHandler::HandlePacket_CQ_PartyOptionModify(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
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
	NT_LOG("[client%x] Client :: CQ_EnqueueGame :: { }", clientHd);

	game->OnEnqueueGame(clientHd);
}

void HubPacketHandler::HandlePacket_CA_SortieRoomFound(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CA_SortieRoomFound& packet = SafeCast<Cl::CA_SortieRoomFound>(packetData, packetSize);
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CA_SortieRoomFound>(packetData, packetSize));

	game->OnSortieRoomFound(clientHd, packet.sortieID);
}

void HubPacketHandler::HandlePacket_CN_SortieRoomConfirm(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CN_SortieRoomConfirm& packet = SafeCast<Cl::CN_SortieRoomConfirm>(packetData, packetSize);
	NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CN_SortieRoomConfirm>(packetData, packetSize));

	game->OnSortieRoomConfirm(clientHd, packet.confirm);
}
