#include "channel.h"

#include "coordinator.h"
#include <mxm/game_content.h>
#include "game.h"


// TODO: replace listener type later on, this is just convenient for now
bool ChannelHub::Init(Server* server_)
{
	server = server_;
	replication.Init(server_);
	game = new GameHub();
	game->Init(&replication);
	return true;
}

void ChannelHub::Cleanup()
{
	LOG("ChannelHub cleanup...");
}

void ChannelHub::Update()
{
	ProfileFunction();

	game->Update(localTime);
	replication.FrameEnd();
}

void ChannelHub::OnNewClientsConnected(const eastl::pair<i32, const AccountData*>* clientList, const i32 count)
{
	for(int i = 0; i < count; i++) {
		auto& it = clientList[i];
		game->OnPlayerConnect(it.first, it.second);
		replication.EventPlayerConnect(it.first);
	}
}

void ChannelHub::OnNewClientsDisconnected(const i32* clientList, const i32 count)
{
	for(int i = 0; i < count; i++) {
		i32 clientID = clientList[i];
		game->OnPlayerDisconnect(clientID);
		replication.EventClientDisconnect(clientID);
	}
}

void ChannelHub::OnNewPacket(i32 clientID, const NetHeader& header, const u8* packetData)
{
	const i32 packetSize = header.size - sizeof(NetHeader);

#define HANDLE_CASE(PACKET) case Cl::PACKET::NET_ID: { HandlePacket_##PACKET(clientID, header, packetData, packetSize); } break

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

		default: {
			NT_LOG("[client%03d] Client :: Unknown packet :: size=%d netID=%d", clientID, header.size, header.netID);
		} break;
	}

#undef HANDLE_CASE
}

void ChannelHub::HandlePacket_CQ_GetGuildProfile(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%03d] Client :: %s", clientID, PacketSerialize<Cl::CQ_GetGuildProfile>(packetData, packetSize));

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

		SendPacket(clientID, packet);
	}
}

void ChannelHub::HandlePacket_CQ_GetGuildMemberList(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%03d] Client :: %s", clientID, PacketSerialize<Cl::CQ_GetGuildMemberList>(packetData, packetSize));

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

		SendPacket(clientID, packet);
	}
}

void ChannelHub::HandlePacket_CQ_GetGuildHistoryList(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%03d] Client :: %s", clientID, PacketSerialize<Cl::CQ_GetGuildHistoryList>(packetData, packetSize));

	// SA_GetGuildMemberList
	{
		PacketWriter<Sv::SA_GetGuildHistoryList> packet;

		packet.Write<i32>(0); // result

		packet.Write<u16>(0); // guildHistories_count

		SendPacket(clientID, packet);
	}
}

void ChannelHub::HandlePacket_CQ_GetGuildRankingSeasonList(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_GetGuildRankingSeasonList& rank = SafeCast<Cl::CQ_GetGuildRankingSeasonList>(packetData, packetSize);
	NT_LOG("[client%03d] Client :: %s", clientID, PacketSerialize<Cl::CQ_GetGuildRankingSeasonList>(packetData, packetSize));

	// SA_GetGuildRankingSeasonList
	{
		PacketWriter<Sv::SA_GetGuildRankingSeasonList> packet;

		packet.Write<i32>(0); // result
		packet.Write<u8>(rank.rankingType); // result

		packet.Write<u16>(0); // rankingSeasonList_count

		SendPacket(clientID, packet);
	}
}

void ChannelHub::HandlePacket_CQ_TierRecord(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%03d] Client :: %s", clientID, PacketSerialize<Cl::CQ_TierRecord>(packetData, packetSize));

	// SA_TierRecord
	{
		PacketWriter<Sv::SA_TierRecord> packet;

		packet.Write<u8>(1); // seasonId
		packet.Write<i32>(0); // allTierWin
		packet.Write<i32>(0); // allTierDraw
		packet.Write<i32>(0); // allTierLose
		packet.Write<i32>(0); // allTierLeave
		packet.Write<u16>(0); // stageRecordList_count

		SendPacket(clientID, packet);
	}
}

void ChannelHub::HandlePacket_CN_ReadyToLoadCharacter(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%03d] Client :: CN_ReadyToLoadCharacter ::", clientID);
	game->OnPlayerReadyToLoad(clientID);
}

void ChannelHub::HandlePacket_CN_ReadyToLoadGameMap(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%03d] Client :: CN_ReadyToLoadGame ::", clientID);
	game->OnPlayerReadyToLoad(clientID);
}

void ChannelHub::HandlePacket_CA_SetGameGvt(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CA_SetGameGvt& gvt = SafeCast<Cl::CA_SetGameGvt>(packetData, packetSize);
	NT_LOG("[client%03d] Client :: CA_SetGameGvt :: sendTime=%d virtualTime=%d unk=%d", clientID, gvt.sendTime, gvt.virtualTime, gvt.unk);
}

void ChannelHub::HandlePacket_CN_MapIsLoaded(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%03d] Client :: CN_MapIsLoaded ::", clientID);
	replication.SetPlayerAsInGame(clientID);
}

void ChannelHub::HandlePacket_CQ_GetCharacterInfo(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_GetCharacterInfo& req = SafeCast<Cl::CQ_GetCharacterInfo>(packetData, packetSize);
	NT_LOG("[client%03d] Client :: CQ_GetCharacterInfo :: characterID=%d", clientID, (u32)req.characterID);

	ActorUID actorUID = replication.GetWorldActorUID(clientID, req.characterID);
	if(actorUID == ActorUID::INVALID) {
		WARN("Client sent an invalid actor (localActorID=%d)", req.characterID);
		return;
	}

	game->OnPlayerGetCharacterInfo(clientID, actorUID);
}

void ChannelHub::HandlePacket_CN_UpdatePosition(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CN_UpdatePosition& update = SafeCast<Cl::CN_UpdatePosition>(packetData, packetSize);
	NT_LOG("[client%03d] Client :: CN_UpdatePosition :: { characterID=%d p3nPos=(%g, %g, %g) p3nDir=(%g, %g, %g) p3nEye=(%g, %g, %g) nRotate=%g nSpeed=%g nState=%d nActionIDX=%d", clientID, (u32)update.characterID, update.p3nPos.x, update.p3nPos.y, update.p3nPos.z, update.p3nDir.x, update.p3nDir.y, update.p3nDir.z, update.p3nEye.x, update.p3nEye.y, update.p3nEye.z, update.nRotate, update.nSpeed, (i32)update.nState, update.nActionIDX);

	ActorUID actorUID = replication.GetWorldActorUID(clientID, update.characterID);
	if(actorUID == ActorUID::INVALID) {
		WARN("Client sent an invalid actor (localActorID=%d)", update.characterID);
		return;
	}

	game->OnPlayerUpdatePosition(clientID, actorUID, f2v(update.p3nPos), f2v(update.p3nDir), f2v(update.p3nEye), update.nRotate, update.nSpeed, update.nState, update.nActionIDX);
}

void ChannelHub::HandlePacket_CN_ChannelChatMessage(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	ConstBuffer buff(packetData, packetSize);
	i32 chatType = buff.Read<i32>();
	const u16 msgLen = buff.Read<u16>();
	const wchar* msg = (wchar*)buff.ReadRaw(msgLen * 2);

	NT_LOG("[client%03d] Client :: CN_ChannelChatMessage :: chatType=%d msg='%.*S'", clientID, chatType, msgLen, msg);

	game->OnPlayerChatMessage(clientID, chatType, msg, msgLen);
}

void ChannelHub::HandlePacket_CQ_SetLeaderCharacter(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_SetLeaderCharacter& leader = SafeCast<Cl::CQ_SetLeaderCharacter>(packetData, packetSize);
	NT_LOG("[client%03d] Client :: CQ_SetLeaderCharacter :: characterID=%d skinIndex=%d", clientID, (u32)leader.characterID, (i32)leader.skinIndex);

	game->OnPlayerSetLeaderCharacter(clientID, leader.characterID, leader.skinIndex);
}

void ChannelHub::HandlePacket_CN_GamePlayerSyncActionStateOnly(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CN_GamePlayerSyncActionStateOnly& sync = SafeCast<Cl::CN_GamePlayerSyncActionStateOnly>(packetData, packetSize);

	i32 state = (i32)sync.state;
	const char* stateStr = g_ActionStateInvalidString;
	if(state >= 0 && state < ARRAY_COUNT(g_ActionStateString)) {
		stateStr = g_ActionStateString[state];
	}

	NT_LOG("[client%03d] Client :: CN_GamePlayerSyncActionStateOnly :: {", clientID);
	NT_LOG("	characterID=%d", (u32)sync.characterID);
	NT_LOG("	nState=%d (%s)", (i32)sync.state, stateStr);
	NT_LOG("	bApply=%d", sync.bApply);
	NT_LOG("	param1=%d", sync.param1);
	NT_LOG("	param2=%d", sync.param2);
	NT_LOG("	i4=%d", sync.i4);
	NT_LOG("	rotate=%g", sync.rotate);
	NT_LOG("	upperRotate=%g", sync.upperRotate);
	NT_LOG("}");

	ActorUID actorUID = replication.GetWorldActorUID(clientID, sync.characterID);
	if(actorUID == ActorUID::INVALID) {
		WARN("Client sent an invalid actor (localActorID=%d)", sync.characterID);
		return;
	}

	game->OnPlayerSyncActionState(clientID, actorUID, sync.state, sync.param1, sync.param2, sync.rotate, sync.upperRotate);
}

void ChannelHub::HandlePacket_CQ_JukeboxQueueSong(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_JukeboxQueueSong& queue = SafeCast<Cl::CQ_JukeboxQueueSong>(packetData, packetSize);
	NT_LOG("[client%03d] Client :: CQ_JukeboxQueueSong :: { songID=%d }", clientID, (i32)queue.songID);

	game->OnPlayerJukeboxQueueSong(clientID, queue.songID);
}

void ChannelHub::HandlePacket_CQ_WhisperSend(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
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

	game->OnPlayerChatWhisper(clientID, destNick.data(), msg.data());
}

void ChannelHub::HandlePacket_CQ_PartyCreate(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_PartyCreate& create = SafeCast<Cl::CQ_PartyCreate>(packetData, packetSize);
	NT_LOG("[client%03d] Client :: CQ_PartyCreate :: { entrySysID=%d stageType=%d }", clientID, create.entrySysID, create.stageType);

	// we don't support creating parties right now, send back an error

	PacketWriter<Sv::SA_PartyCreate> packet;

	//packet.Write<i32>(175); // retval (ERROR_TYPE_PARTY_CREATE_PENALTY_TIME) <- this one is silent
	packet.Write<i32>(0); // retval: success
	packet.Write<i32>(1); // ownerUserID
	packet.Write<i32>(create.stageType); // stageType

	SendPacket(clientID, packet);
}

void ChannelHub::HandlePacket_CQ_RTT_Time(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_RTT_Time& rtt = SafeCast<Cl::CQ_RTT_Time>(packetData, packetSize);
	NT_LOG("[client%03d] Client :: CQ_RTT_Time :: { time=%u }", clientID, rtt.time);

	Sv::SA_RTT_Time answer;
	answer.clientTimestamp = rtt.time;
	answer.serverTimestamp = (i64)TimeDiffMs(TimeRelNow());
	SendPacket(clientID, answer);
}

void ChannelHub::HandlePacket_CQ_LoadingProgressData(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_LoadingProgressData& loading = SafeCast<Cl::CQ_LoadingProgressData>(packetData, packetSize);
	NT_LOG("[client%03d] Client :: CQ_LoadingProgressData :: { progress=%u }", clientID, loading.progress);
}

void ChannelHub::HandlePacket_CQ_RequestCalendar(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_RequestCalendar& req = SafeCast<Cl::CQ_RequestCalendar>(packetData, packetSize);
	NT_LOG("[client%03d] Client :: CQ_RequestCalendar :: { filetimeUTC=%llu }", clientID, req.filetimeUTC);

	replication.SendCalendar(clientID);
}

void ChannelHub::HandlePacket_CQ_RequestAreaPopularity(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_RequestAreaPopularity& req = SafeCast<Cl::CQ_RequestAreaPopularity>(packetData, packetSize);
	NT_LOG("[client%03d] Client :: CQ_RequestAreaPopularity :: { area=%u }", clientID, req.areaID);

	replication.SendAreaPopularity(clientID, req.areaID);
}

void ChannelHub::HandlePacket_CQ_PartyModify(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%03d] Client :: %s", clientID, PacketSerialize<Cl::CQ_PartyModify>(packetData, packetSize));

	Sv::SA_PartyModify packet;
	packet.retval = 0;
	SendPacket(clientID, packet);
}

void ChannelHub::HandlePacket_CQ_PartyOptionModify(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_PartyOptionModify& req = SafeCast<Cl::CQ_PartyOptionModify>(packetData, packetSize);
	NT_LOG("[client%03d] Client :: %s", clientID, PacketSerialize<Cl::CQ_PartyOptionModify>(packetData, packetSize));

	Sv::SA_PartyOptionModify packet;
	packet.retval = 0;
	packet.option = req.option;
	packet.enable = req.enable;
	SendPacket(clientID, packet);
}

void ChannelHub::HandlePacket_CQ_EnqueueGame(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	NT_LOG("[client%03d] Client :: CQ_EnqueueGame :: { }", clientID);

	Sv::SA_EnqueueGame packet;
	packet.retval = 0;
	SendPacket(clientID, packet);

	Sv::SN_EnqueueMatchingQueue matching;
	matching.stageIndex = 200020102;
	matching.currentMatchingTimeMs = 0;
	matching.avgMatchingTimeMs = 121634;
	matching.disableMatchExpansion = 0;
	matching.isMatchingExpanded = 0;
	SendPacket(clientID, matching);
}
