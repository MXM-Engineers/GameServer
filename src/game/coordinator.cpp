#include "coordinator.h"
#include "game.h"
#include <zlib.h>
#include <EAThread/eathread_thread.h>

intptr_t ThreadCoordinator(void* pData)
{
	Coordinator& coordinator = *(Coordinator*)pData;

	while(coordinator.server->running) {
		coordinator.Update();
	}
	return 0;
}

void Coordinator::Init(Server* server_, Game* game_)
{
	server = server_;
	game = game_;
	recvDataBuff.Init(10 * (1024*1024)); // 10 MB

	EA::Thread::Thread Thread;
	Thread.Begin(ThreadCoordinator, this);
}

void Coordinator::Update()
{
	server->TransferAllReceivedData(&recvDataBuff);

	ConstBuffer buff(recvDataBuff.data, recvDataBuff.size);
	while(buff.CanRead(sizeof(Server::ReceiveBufferHeader))) {
		const Server::ReceiveBufferHeader& header = buff.Read<Server::ReceiveBufferHeader>();
		const u8* data = buff.ReadRaw(header.len);
		ClientHandleReceivedChunk(header.clientID, data, header.len);
	}

	recvDataBuff.Clear();

	Sleep(16); // TODO: do a time accumulator scheme to wait a precise time
}


void Coordinator::ClientHandlePacket(i32 clientID, const NetHeader& header, const u8* packetData)
{
	DBG_ASSERT(clientID >= 0 && clientID < Server::MAX_CLIENTS);
	const i32 packetSize = header.size - sizeof(NetHeader);

#define HANDLE_CASE(PACKET) case Cl::PACKET::NET_ID: { HandlePacket_##PACKET(clientID, header, packetData, packetSize); } break

	switch(header.netID) {
		HANDLE_CASE(CQ_FirstHello);
		HANDLE_CASE(CQ_Authenticate);
		HANDLE_CASE(CQ_GetGuildProfile);
		HANDLE_CASE(CQ_GetGuildMemberList);
		HANDLE_CASE(CQ_GetGuildHistoryList);
		HANDLE_CASE(CQ_GetGuildRankingSeasonList);
		HANDLE_CASE(CQ_TierRecord);

		default: {
			// TODO: dispatch packets to games depending on clients playing on them
			game->CoordinatorClientHandlePacket(clientID, header, packetData);
		} break;
	}

#undef HANDLE_CASE
}

void Coordinator::ClientHandleReceivedChunk(i32 clientID, const u8* data, const i32 dataSize)
{
	if(dataSize < sizeof(NetHeader)) {
		LOG("ERROR(ClientHandleReceivedChunk): received invalid data (%d < %d)", dataSize, (i32)sizeof(NetHeader));
		server->DisconnectClient(clientID);
		return;
	}

	ConstBuffer buff(data, dataSize);
	while(buff.CanRead(sizeof(NetHeader))) {
#ifdef CONF_DEBUG
		const u8* data = buff.cursor;
#endif
		const NetHeader& header = buff.Read<NetHeader>();
		const u8* packetData = buff.ReadRaw(header.size - sizeof(NetHeader));

#ifdef CONF_DEBUG
		fileSaveBuff(FMT("trace\\game_%d_cl_%d.raw", server->packetCounter, header.netID), data, header.size);
		server->packetCounter++;
#endif
		ClientHandlePacket(clientID, header, packetData);
	}
}

void Coordinator::HandlePacket_CQ_FirstHello(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	LOG("[client%03d] Client :: CQ_FirstHello", clientID);

	// TODO: verify version, protocol, etc
	const Server::ClientInfo& info = server->clientInfo[clientID];

	Sv::SA_FirstHello hello;
	hello.dwProtocolCRC = 0x28845199;
	hello.dwErrorCRC    = 0x93899e2c;
	hello.serverType    = 1;
	memmove(hello.clientIp, info.ip, sizeof(hello.clientIp));
	STATIC_ASSERT(sizeof(hello.clientIp) == sizeof(info.ip));
	hello.clientPort = info.port;
	hello.tqosWorldId = 1;

	LOG("[client%03d] Server :: SA_FirstHello :: protocolCrc=%x errorCrc=%x serverType=%d clientIp=(%s) clientPort=%d tqosWorldId=%d", clientID, hello.dwProtocolCRC, hello.dwErrorCRC, hello.serverType, IpToString(hello.clientIp), hello.clientPort, hello.tqosWorldId);
	SendPacket(clientID, hello);
}

void Coordinator::HandlePacket_CQ_Authenticate(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	ConstBuffer request(packetData, packetSize);
	const u16 nickLen = request.Read<u16>();
	const wchar* nick = (wchar*)request.ReadRaw(nickLen * sizeof(wchar));
	i32 var = request.Read<i32>();
	LOG("[client%03d] Client :: RequestConnectGame :: %.*S var=%d", clientID, nickLen, nick, var);

	const Server::ClientInfo& info = server->clientInfo[clientID];

	// TODO: check authentication

	// send authentication result
	Sv::SA_AuthResult auth;
	auth.result = 91;
	LOG("[client%03d] Server :: SA_AuthResult :: result=%d", clientID, auth.result);
	SendPacket(clientID, auth);


	// TODO: fetch account data
	AccountData& account = accountData[clientID];
	account = {};
	account.nickname.assign(nick, nickLen);
	account.guildTag = L"XMX";

	// send account data
	ClientSendAccountData(clientID);

	// register new player to the game
	game->CoordinatorRegisterNewPlayer(clientID, &account);
}

void Coordinator::HandlePacket_CQ_GetGuildProfile(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	LOG("[client%03d] Client :: CQ_GetGuildProfile ::", clientID);

	// SA_GetGuildProfile
	{
		u8 sendData[2048];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<i32>(0); // result;
		packet.WriteStringObj(L"The XMX dream"); // guildName
		packet.WriteStringObj(L"XMX"); // guildTag
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

		LOG("[client%03d] Server :: SA_GetGuildProfile :: ", clientID);
		SendPacketData(clientID, Sv::SA_GetGuildProfile::NET_ID, packet.size, packet.data);
	}
}

void Coordinator::HandlePacket_CQ_GetGuildMemberList(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	LOG("[client%03d] Client :: CQ_GetGuildMemberList ::", clientID);

	// SA_GetGuildMemberList
	{
		u8 sendData[2048];
		PacketWriter packet(sendData, sizeof(sendData));

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

		LOG("[client%03d] Server :: SA_GetGuildMemberList :: ", clientID);
		SendPacketData(clientID, Sv::SA_GetGuildMemberList::NET_ID, packet.size, packet.data);
	}
}

void Coordinator::HandlePacket_CQ_GetGuildHistoryList(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	LOG("[client%03d] Client :: CQ_GetGuildHistoryList ::", clientID);

	// SA_GetGuildMemberList
	{
		u8 sendData[2048];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<i32>(0); // result

		packet.Write<u16>(0); // guildHistories_count


		LOG("[client%03d] Server :: SA_GetGuildHistoryList :: ", clientID);
		SendPacketData(clientID, Sv::SA_GetGuildHistoryList::NET_ID, packet.size, packet.data);
	}
}

void Coordinator::HandlePacket_CQ_GetGuildRankingSeasonList(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_GetGuildRankingSeasonList& rank = SafeCast<Cl::CQ_GetGuildRankingSeasonList>(packetData, packetSize);
	LOG("[client%03d] Client :: CQ_GetGuildRankingSeasonList :: rankingType=%d", clientID, rank.rankingType);

	// SA_GetGuildRankingSeasonList
	{
		u8 sendData[2048];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<i32>(0); // result
		packet.Write<u8>(rank.rankingType); // result

		packet.Write<u16>(0); // rankingSeasonList_count


		LOG("[client%03d] Server :: SA_GetGuildRankingSeasonList :: ", clientID);
		SendPacketData(clientID, Sv::SA_GetGuildRankingSeasonList::NET_ID, packet.size, packet.data);
	}
}

void Coordinator::HandlePacket_CQ_TierRecord(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	LOG("[client%03d] Client :: CQ_TierRecord ::", clientID);

	// SA_TierRecord
	{
		u8 sendData[2048];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u8>(1); // seasonId
		packet.Write<i32>(0); // allTierWin
		packet.Write<i32>(0); // allTierDraw
		packet.Write<i32>(0); // allTierLose
		packet.Write<i32>(0); // allTierLeave
		packet.Write<u16>(0); // stageRecordList_count

		LOG("[client%03d] Server :: SA_TierRecord :: ", clientID);
		SendPacketData(clientID, Sv::SA_TierRecord::NET_ID, packet.size, packet.data);
	}
}

void Coordinator::ClientSendAccountData(i32 clientID)
{
	// Send account data
	const AccountData& account = accountData[clientID];

	// SN_RegionServicePolicy
	{
		u8 sendData[256];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(1); // newMasterRestrict_count
		packet.Write<u8>(1); // newMasterRestrict[0]

		packet.Write<u16>(1); // userGradePolicy_count
		packet.Write<u8>(5); // userGradePolicy[0].userGrade
		packet.Write<u16>(1); // userGradePolicy[0].benefits_count
		packet.Write<u8>(9); // userGradePolicy[0].benefits[0]

		packet.Write<u8>(2); // purchaseCCoinMethod
		packet.Write<u8>(1); // exchangeCCoinForGoldMethod
		packet.Write<u8>(0); // rewardCCoinMethod
		packet.Write<u8>(1); // pveRewardSlotOpenBuyChanceMethod

		packet.Write<u16>(3); // regionBanMaster_count
		packet.Write<i32>(100000041); // regionBanMaster[0]
		packet.Write<i32>(100000042); // regionBanMaster[1]
		packet.Write<i32>(100000043); // regionBanMaster[2]

		packet.Write<u16>(1); // regionNewMaster_count
		packet.Write<i32>(100000038); // intList2[0]

		packet.Write<u16>(0); // eventBanMaster_count

		packet.Write<i32>(5);	// checkPeriodSec
		packet.Write<i32>(10);	// maxTalkCount
		packet.Write<i32>(120); // blockPeriodSec

		packet.Write<u16>(0); // regionBanSkinList_count
		packet.Write<u16>(0); // pcCafeSkinList_count

		packet.Write<u8>(1); // useFatigueSystem

		LOG("[client%03d] Server :: SN_RegionServicePolicy :: ", clientID);
		SendPacketData(clientID, Sv::SN_RegionServicePolicy::NET_ID, packet.size, packet.data);
	}

	// SN_AllCharacterBaseData
	{
		u8 sendData[4096];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(1); // charaList_count

		packet.Write<i32>(100000001); // charaList[0].masterID
		packet.Write<u16>(22); // charaList[0].baseStats_count

		// charaList[0].baseStats
		typedef Sv::SN_AllCharacterBaseData::Character::Stat Stat;
		packet.Write(Stat{ 0, 2400.f });
		packet.Write(Stat{ 2, 200.f });
		packet.Write(Stat{ 37, 120.f });
		packet.Write(Stat{ 5, 5.f });
		packet.Write(Stat{ 42, 0.6f });
		packet.Write(Stat{ 7, 92.3077f });
		packet.Write(Stat{ 9, 3.f });
		packet.Write(Stat{ 10, 150.f });
		packet.Write(Stat{ 18, 100.f });
		packet.Write(Stat{ 13, 100.f });
		packet.Write(Stat{ 14, 100.f });
		packet.Write(Stat{ 15, 100.f });
		packet.Write(Stat{ 52, 100.f });
		packet.Write(Stat{ 16, 1.f });
		packet.Write(Stat{ 29, 20.f });
		packet.Write(Stat{ 23, 9.f });
		packet.Write(Stat{ 31, 14.f });
		packet.Write(Stat{ 22, 2.f });
		packet.Write(Stat{ 54, 15.f });
		packet.Write(Stat{ 63, 3.f });
		packet.Write(Stat{ 64, 150.f });
		packet.Write(Stat{ 55, 15.f });

		packet.Write<u16>(7); // charaList[0].skillData_count

		// charaList[0].skillData
		typedef Sv::SN_AllCharacterBaseData::Character::SkillRatio SkillR;
		packet.Write(SkillR{ 180010020, 355.f, 0.42f, 0.f, 0.f, 0.f });
		packet.Write(SkillR{ 180010040, 995.f, 0.81f, 0.f, 0.f, 0.1f });
		packet.Write(SkillR{ 180010010, 550.f, 0.56f, 0.f, 0.f, 0.f });
		packet.Write(SkillR{ 180010030, 0.f, 0.f, 0.f, 0.f, 0.f });
		packet.Write(SkillR{ 180010050, 680.f, 0.37f, 0.f, 0.f, 0.f });
		packet.Write(SkillR{ 180010000, 0.f, 1.0f, 0.f, 0.f, 0.f });
		packet.Write(SkillR{ 180010002, 0.f, 1.0f, 0.f, 0.f, 0.f });

		packet.Write<i32>(1); // cur
		packet.Write<i32>(1); // max


		LOG("[client%03d] Server :: SN_AllCharacterBaseData :: ", clientID);
		SendPacketData(clientID, Sv::SN_AllCharacterBaseData::NET_ID, packet.size, packet.data);
	}

	// SN_MyGuild
	{
		u8 sendData[256];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.WriteStringObj(L"XMX");
		packet.Write<i64>(0);
		packet.Write<u8>(0);

		LOG("[client%03d] Server :: SN_MyGuild :: ", clientID);
		SendPacketData(clientID, Sv::SN_MyGuild::NET_ID, packet.size, packet.data);
	}

	// SN_ProfileCharacters
	{
		u8 sendData[2048];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(1); // charaList_count

		Sv::SN_ProfileCharacters::Character chara;
		chara.characterID = 21013;
		chara.creatureIndex = 100000035;
		chara.skillShot1 = 180350010;
		chara.skillShot2 = 180350030;
		chara.class_ = 35;
		chara.x = 12029;
		chara.y = 12622;
		chara.z = 3328.29f;
		chara.characterType = 1;
		chara.skinIndex = 2;
		chara.weaponIndex = 131135012;
		chara.masterGearNo = 1;

		packet.Write(chara);

		LOG("[client%03d] Server :: SN_ProfileCharacters :: ", clientID);
		SendPacketData(clientID, Sv::SN_ProfileCharacters::NET_ID, packet.size, packet.data);
	}

	// SN_ProfileWeapons
	{
		u8 sendData[2048];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(1); // weaponList_count

		Sv::SN_ProfileWeapons::Weapon weap;
		weap.characterID = 21013;
		weap.weaponType = 1;
		weap.weaponIndex = 131135012;
		weap.grade = 1;
		weap.isUnlocked = 1;
		weap.isActivated = 1;

		packet.Write(weap);

		LOG("[client%03d] Server :: SN_ProfileWeapons :: ", clientID);
		SendPacketData(clientID, Sv::SN_ProfileWeapons::NET_ID, packet.size, packet.data);
	}

	// SN_ProfileMasterGears
	{
		u8 sendData[128];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(0); // masterGears_count

		LOG("[client%03d] Server :: SN_ProfileMasterGears :: ", clientID);
		SendPacketData(clientID, Sv::SN_ProfileMasterGears::NET_ID, packet.size, packet.data);
	}

	// SN_ProfileItems
	{
		u8 sendData[128];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(1); // packetNum
		packet.Write<u16>(0); // items_count

		LOG("[client%03d] Server :: SN_ProfileItems :: ", clientID);
		SendPacketData(clientID, Sv::SN_ProfileItems::NET_ID, packet.size, packet.data);
	}

	// SN_ProfileSkills
	{
		u8 sendData[128];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(1); // packetNum
		packet.Write<u16>(0); // skills_count

		LOG("[client%03d] Server :: SN_ProfileSkills :: ", clientID);
		SendPacketData(clientID, Sv::SN_ProfileSkills::NET_ID, packet.size, packet.data);
	}

	// SN_ProfileTitles
	{
		u8 sendData[128];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(1); // titles_count
		packet.Write<i32>(320080004); // titles[0]

		LOG("[client%03d] Server :: SN_ProfileTitles :: ", clientID);
		SendPacketData(clientID, Sv::SN_ProfileTitles::NET_ID, packet.size, packet.data);
	}

	// SN_ProfileCharacterSkinList
	{
		u8 sendData[128];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(0); // skins_count

		LOG("[client%03d] Server :: SN_ProfileCharacterSkinList :: ", clientID);
		SendPacketData(clientID, Sv::SN_ProfileCharacterSkinList::NET_ID, packet.size, packet.data);
	}

	// SN_AccountInfo
	{
		u8 sendData[128];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.WriteStringObj(account.nickname.data()); // nick
		packet.Write<i32>(4); // inventoryLineCountTab0
		packet.Write<i32>(4); // inventoryLineCountTab1
		packet.Write<i32>(4); // inventoryLineCountTab2
		packet.Write<i32>(320080005); // displayTitlteIndex
		packet.Write<i32>(320080005); // statTitleIndex
		packet.Write<i32>(1); // warehouseLineCount
		packet.Write<i32>(-1); // tutorialState
		packet.Write<i32>(3600); // masterGearDurability
		packet.Write<u8>(0); // badgeType

		LOG("[client%03d] Server :: SN_AccountInfo :: ", clientID);
		SendPacketData(clientID, Sv::SN_AccountInfo::NET_ID, packet.size, packet.data);
	}

	// SN_AccountExtraInfo
	{
		u8 sendData[128];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(0); // userGradeList_count
		packet.Write<i32>(0); // activityPoint
		packet.Write<u8>(0); // activityRewaredState

		LOG("[client%03d] Server :: SN_AccountExtraInfo :: ", clientID);
		SendPacketData(clientID, Sv::SN_AccountExtraInfo::NET_ID, packet.size, packet.data);
	}

	// SN_AccountEquipmentList
	{
		u8 sendData[128];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<i32>(-1); // supportKitDocIndex

		LOG("[client%03d] Server :: SN_AccountEquipmentList :: ", clientID);
		SendPacketData(clientID, Sv::SN_AccountEquipmentList::NET_ID, packet.size, packet.data);
	}

	// SN_Unknown_62472
	{
		u8 sendData[32];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u8>(1);

		LOG("[client%03d] Server :: SN_Unknown_62472 :: ", clientID);
		SendPacketData(clientID, 62472, packet.size, packet.data);
	}

	// SN_GuildChannelEnter
	{
		u8 sendData[32];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.WriteStringObj(L"XMX"); // guildName
		packet.WriteStringObj(account.nickname.data()); // nick
		packet.Write<u8>(0); // onlineStatus

		LOG("[client%03d] Server :: SN_GuildChannelEnter :: ", clientID);
		SendPacketData(clientID, Sv::SN_GuildChannelEnter::NET_ID, packet.size, packet.data);
	}

	// SN_ClientSettings
	{
		u8 sendData[4096];
		PacketWriter packet(sendData, sizeof(sendData));

		const char* src = R"foo(
						  <?xml version="1.0" encoding="utf-8"?>
						  <KEY_DATA highDateTime="30633031" lowDateTime="3986680182" isCustom="0" keyboardLayoutName="00000813">
						  <data input="INPUT_UIEDITMODE" key="MKC_NOKEY" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_PING" key="MKC_NOKEY" modifier="MKC_MENU" isaddkey="0" />
						  <data input="INPUT_LATTACK" key="MKC_LBUTTON" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_GAMEPING" key="MKC_LBUTTON" modifier="MKC_MENU" isaddkey="0" />
						  <data input="INPUT_SHIRK" key="MKC_RBUTTON" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_RATTACK" key="MKC_RBUTTON" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_BACKPING" key="MKC_RBUTTON" modifier="MKC_MENU" isaddkey="0" />
						  <data input="INPUT_DASHBOARD" key="MKC_TAB" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_CHATON" key="MKC_RETURN" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_CHAT_ALLPLAYER_ONCE" key="MKC_RETURN" modifier="MKC_SHIFT" isaddkey="0" />
						  <data input="INPUT_ESC" key="MKC_ESCAPE" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_JUMP_SAFEFALL" key="MKC_SPACE" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_REPLAY_GOTO_LIVE" key="MKC_0" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_CHATMACRO_0" key="MKC_0" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_WARFOGMODE_1" key="MKC_1" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_CHATMACRO_1" key="MKC_1" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_EMOTION_0" key="MKC_1" modifier="MKC_MENU" isaddkey="0" />
						  <data input="INPUT_WARFOGMODE_2" key="MKC_2" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_CHATMACRO_2" key="MKC_2" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_EMOTION_1" key="MKC_2" modifier="MKC_MENU" isaddkey="0" />
						  <data input="INPUT_WARFOGMODE_3" key="MKC_3" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_CHATMACRO_3" key="MKC_3" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_EMOTION_2" key="MKC_3" modifier="MKC_MENU" isaddkey="0" />
						  <data input="INPUT_WARFOGMODE_4" key="MKC_4" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_CHATMACRO_4" key="MKC_4" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_CHATMACRO_5" key="MKC_5" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_TOGGLE_TIME_CONTROLLER" key="MKC_6" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_CHATMACRO_6" key="MKC_6" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_CHATMACRO_7" key="MKC_7" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_REPLAY_MOVEBACK" key="MKC_8" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_CHATMACRO_8" key="MKC_8" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_REPLAY_PAUSE_RESUME" key="MKC_9" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_CHATMACRO_9" key="MKC_9" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_QUICKSLOT_1" key="MKC_A" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_SKILLUP_1" key="MKC_A" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_QUICKSLOT_1_NOTIFY" key="MKC_A" modifier="MKC_MENU" isaddkey="0" />
						  <data input="INPUT_QUICKSLOT_OPT" key="MKC_B" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_QUICKSLOT_3" key="MKC_C" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_CSHOP" key="MKC_C" modifier="MKC_SHIFT" isaddkey="0" />
						  <data input="INPUT_STAGE_SKILL_NOTIFY" key="MKC_C" modifier="MKC_MENU" isaddkey="0" />
						  <data input="INPUT_RIGHT" key="MKC_D" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_QUICKSLOT_2" key="MKC_E" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_SKILLUP_2" key="MKC_E" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_QUICKSLOT_2_NOTIFY" key="MKC_E" modifier="MKC_MENU" isaddkey="0" />
						  <data input="INPUT_ACTION" key="MKC_F" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_GUILD" key="MKC_G" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_TUTORIAL" key="MKC_H" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_INVENTORY" key="MKC_I" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_MISSIONLIST" key="MKC_J" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_NAME_DECO" key="MKC_K" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_FRIENDLIST" key="MKC_L" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_SCHEDULE" key="MKC_M" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_NO" key="MKC_N" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_OPTIONWINDOW" key="MKC_O" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_CHARACTER" key="MKC_P" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_POST" key="MKC_P" modifier="MKC_SHIFT" isaddkey="0" />
						  <data input="INPUT_LEFT" key="MKC_Q" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_QUICKSLOT_UG" key="MKC_R" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_SKILLUP_UG" key="MKC_R" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_QUICKSLOT_UG_NOTIFY" key="MKC_R" modifier="MKC_MENU" isaddkey="0" />
						  <data input="INPUT_BACK" key="MKC_S" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_ATTRIBUTE" key="MKC_T" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_TITAN_AVATAR_NOTIFY" key="MKC_T" modifier="MKC_MENU" isaddkey="0" />
						  <data input="INPUT_SUMMARY" key="MKC_U" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_PASSIVE_NOTIFY" key="MKC_V" modifier="MKC_MENU" isaddkey="0" />
						  <data input="INPUT_TITAN_AVATAR" key="MKC_W" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_UI_TOGGLE" key="MKC_X" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_YES" key="MKC_Y" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_FRONT" key="MKC_Z" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_GUARDIAN_CAM" key="MKC_NUMPAD0" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_WIZARD_CAM" key="MKC_NUMPAD1" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_ADAMAN_CAM" key="MKC_NUMPAD2" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_RUAK_CAM" key="MKC_NUMPAD3" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_TITAN_BLUE_CAM_1" key="MKC_NUMPAD4" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_TITAN_BLUE_CAM_2" key="MKC_NUMPAD5" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_TITAN_BLUE_CAM_3" key="MKC_NUMPAD6" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_TITAN_RED_CAM_1" key="MKC_NUMPAD7" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_TITAN_RED_CAM_2" key="MKC_NUMPAD8" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_TITAN_RED_CAM_3" key="MKC_NUMPAD9" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_FRIENDLY_CAM_1" key="MKC_F1" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_FRIENDLY_CAM_2" key="MKC_F2" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_FRIENDLY_CAM_3" key="MKC_F3" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_FRIENDLY_CAM_4" key="MKC_F4" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_FRIENDLY_CAM_5" key="MKC_F5" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_ENEMY_CAM_1" key="MKC_F6" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_ENEMY_CAM_2" key="MKC_F7" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_ENEMY_CAM_3" key="MKC_F8" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_ENEMY_CAM_4" key="MKC_F9" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_ENEMY_CAM_5" key="MKC_F10" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_CAM_MANUAL" key="MKC_F11" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_CAM_AUTO" key="MKC_F12" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_VIDEO_RECORDING" key="MKC_F12" modifier="MKC_CONTROL" isaddkey="0" />
						  <data input="INPUT_CAM_ZOOM_TOGGLE" key="MKC_OEM_1" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_TRADER_INGREDIENT" key="MKC_OEM_PLUS" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_TRADER_SKIN" key="MKC_OEM_PERIOD" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_TRADER_MEDAL" key="MKC_OEM_2" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_TAG" key="MKC_WHEELDOWN" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_TAG_NOTIFY" key="MKC_WHEELDOWN" modifier="MKC_MENU" isaddkey="0" />
						  <data input="INPUT_BATTLELOG" key="MKC_OEM_6" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_TOGGLE_INGAME_INFORMATION" key="MKC_OEM_7" modifier="MKC_NOKEY" isaddkey="0" />
						  <data input="INPUT_ATTRIBUTE" key="MKC_NOKEY" modifier="MKC_NOKEY" isaddkey="1" />
						  <data input="INPUT_TAG" key="MKC_WHEELUP" modifier="MKC_NOKEY" isaddkey="1" />
						  </KEY_DATA>
						  )foo";

		u8 dest[2048];
		uLongf destLen = sizeof(dest);
		int r = compress((Bytef*)dest, &destLen, (Bytef*)src, strlen(src));
		if(r != Z_OK) {
			if(r == Z_MEM_ERROR) LOG("ERROR(compress) not enough memory");
			else if(r == Z_BUF_ERROR) LOG("ERROR(compress) not enough room in the output buffer");
			ASSERT_MSG(0, "compress failed");
		}

		packet.Write<u8>(0); // settingType
		packet.Write<u16>(destLen);
		packet.WriteRaw(dest, destLen);

		LOG("[client%03d] Server :: SN_ClientSettings :: dataLen=%d", clientID, destLen);
		SendPacketData(clientID, Sv::SN_ClientSettings::NET_ID, packet.size, packet.data);
	}

	// SN_ClientSettings
	{
		u8 sendData[4096];
		PacketWriter packet(sendData, sizeof(sendData));

		const char* src =
				R"foo(
				<?xml version="1.0" encoding="utf-8"?>
				<userdata
				version="2">
				<useroption>
				<displayUserName
				version="0"
				value="1" />
				<displayNameOfUserTeam
				version="0"
				value="1" />
				<displayNameOfOtherTeam
				version="0"
				value="1" />
				<displayMonsterName
				version="0"
				value="1" />
				<displayNpcName
				version="0"
				value="1" />
				<displayUserTitle
				version="0"
				value="1" />
				<displayOtherTitle
				version="0"
				value="1" />
				<displayUserStatusBar
				version="0"
				value="1" />
				<displayStatusBarOfOtherTeam
				version="0"
				value="1" />
				<displayStatusBarOfUserTeam
				version="0"
				value="1" />
				<displayMonsterStatusBar
				version="0"
				value="1" />
				<displayDamage
				version="0"
				value="1" />
				<displayStatus
				version="0"
				value="1" />
				<displayMasterBigImageType
				version="0"
				value="0" />
				<displayCursorSFX
				version="0"
				value="1" />
				<displayTutorialInfos
				version="0"
				value="1" />
				<displayUserStat
				version="0"
				value="0" />
				<chatFiltering
				version="0"
				value="1" />
				<chatTimstamp
				version="0"
				value="0" />
				<useSmartCast
				version="0"
				value="0" />
				<useMouseSight
				version="0"
				value="0" />
				<alwaysActivateHUD
				version="0"
				value="1" />
				</useroption>
				</userdata>
				)foo";

		u8 dest[2048];
		uLongf destLen = sizeof(dest);
		int r = compress((Bytef*)dest, &destLen, (Bytef*)src, strlen(src));
		if(r != Z_OK) {
			if(r == Z_MEM_ERROR) LOG("ERROR(compress) not enough memory");
			else if(r == Z_BUF_ERROR) LOG("ERROR(compress) not enough room in the output buffer");
			ASSERT_MSG(0, "compress failed");
		}

		packet.Write<u8>(1); // settingType
		packet.Write<u16>(destLen);
		packet.WriteRaw(dest, destLen);

		LOG("[client%03d] Server :: SN_ClientSettings :: dataLen=%d", clientID, destLen);
		SendPacketData(clientID, Sv::SN_ClientSettings::NET_ID, packet.size, packet.data);
	}

	// SN_ClientSettings
	{
		u8 sendData[4096];
		PacketWriter packet(sendData, sizeof(sendData));

		const char* src =
				R"foo(
				<?xml version="1.0" encoding="utf-8"?>
				<KEY_DATA highDateTime="30633030" lowDateTime="3827081041" isCustom="0" keyboardLayoutName="00000813">
				<data input="INPUT_SHIRK" key="MKC_NOKEY" modifier="MKC_SHIFT" isaddkey="0" />
				<data input="INPUT_UIEDITMODE" key="MKC_NOKEY" modifier="MKC_CONTROL" isaddkey="0" />
				<data input="INPUT_PING" key="MKC_NOKEY" modifier="MKC_MENU" isaddkey="0" />
				<data input="INPUT_LATTACK" key="MKC_LBUTTON" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_GAMEPING" key="MKC_LBUTTON" modifier="MKC_MENU" isaddkey="0" />
				<data input="INPUT_RATTACK" key="MKC_RBUTTON" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_BACKPING" key="MKC_RBUTTON" modifier="MKC_MENU" isaddkey="0" />
				<data input="INPUT_DASHBOARD" key="MKC_TAB" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_CHATON" key="MKC_RETURN" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_CHAT_ALLPLAYER_ONCE" key="MKC_RETURN" modifier="MKC_SHIFT" isaddkey="0" />
				<data input="INPUT_ESC" key="MKC_ESCAPE" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_JUMP_SAFEFALL" key="MKC_SPACE" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_REPLAY_GOTO_LIVE" key="MKC_0" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_CHATMACRO_0" key="MKC_0" modifier="MKC_CONTROL" isaddkey="0" />
				<data input="INPUT_WARFOGMODE_1" key="MKC_1" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_CHATMACRO_1" key="MKC_1" modifier="MKC_CONTROL" isaddkey="0" />
				<data input="INPUT_EMOTION_0" key="MKC_1" modifier="MKC_MENU" isaddkey="0" />
				<data input="INPUT_WARFOGMODE_2" key="MKC_2" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_CHATMACRO_2" key="MKC_2" modifier="MKC_CONTROL" isaddkey="0" />
				<data input="INPUT_EMOTION_1" key="MKC_2" modifier="MKC_MENU" isaddkey="0" />
				<data input="INPUT_WARFOGMODE_3" key="MKC_3" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_CHATMACRO_3" key="MKC_3" modifier="MKC_CONTROL" isaddkey="0" />
				<data input="INPUT_EMOTION_2" key="MKC_3" modifier="MKC_MENU" isaddkey="0" />
				<data input="INPUT_WARFOGMODE_4" key="MKC_4" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_CHATMACRO_4" key="MKC_4" modifier="MKC_CONTROL" isaddkey="0" />
				<data input="INPUT_CHATMACRO_5" key="MKC_5" modifier="MKC_CONTROL" isaddkey="0" />
				<data input="INPUT_TOGGLE_TIME_CONTROLLER" key="MKC_6" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_CHATMACRO_6" key="MKC_6" modifier="MKC_CONTROL" isaddkey="0" />
				<data input="INPUT_CHATMACRO_7" key="MKC_7" modifier="MKC_CONTROL" isaddkey="0" />
				<data input="INPUT_REPLAY_MOVEBACK" key="MKC_8" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_CHATMACRO_8" key="MKC_8" modifier="MKC_CONTROL" isaddkey="0" />
				<data input="INPUT_REPLAY_PAUSE_RESUME" key="MKC_9" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_CHATMACRO_9" key="MKC_9" modifier="MKC_CONTROL" isaddkey="0" />
				<data input="INPUT_QUICKSLOT_1" key="MKC_A" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_SKILLUP_1" key="MKC_A" modifier="MKC_CONTROL" isaddkey="0" />
				<data input="INPUT_QUICKSLOT_1_NOTIFY" key="MKC_A" modifier="MKC_MENU" isaddkey="0" />
				<data input="INPUT_QUICKSLOT_OPT" key="MKC_B" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_QUICKSLOT_3" key="MKC_C" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_CSHOP" key="MKC_C" modifier="MKC_SHIFT" isaddkey="0" />
				<data input="INPUT_STAGE_SKILL_NOTIFY" key="MKC_C" modifier="MKC_MENU" isaddkey="0" />
				<data input="INPUT_RIGHT" key="MKC_D" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_QUICKSLOT_2" key="MKC_E" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_SKILLUP_2" key="MKC_E" modifier="MKC_CONTROL" isaddkey="0" />
				<data input="INPUT_QUICKSLOT_2_NOTIFY" key="MKC_E" modifier="MKC_MENU" isaddkey="0" />
				<data input="INPUT_ACTION" key="MKC_F" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_GUILD" key="MKC_G" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_TUTORIAL" key="MKC_H" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_INVENTORY" key="MKC_I" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_MISSIONLIST" key="MKC_J" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_NAME_DECO" key="MKC_K" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_FRIENDLIST" key="MKC_L" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_SCHEDULE" key="MKC_M" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_NO" key="MKC_N" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_OPTIONWINDOW" key="MKC_O" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_CHARACTER" key="MKC_P" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_POST" key="MKC_P" modifier="MKC_SHIFT" isaddkey="0" />
				<data input="INPUT_LEFT" key="MKC_Q" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_QUICKSLOT_UG" key="MKC_R" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_SKILLUP_UG" key="MKC_R" modifier="MKC_CONTROL" isaddkey="0" />
				<data input="INPUT_QUICKSLOT_UG_NOTIFY" key="MKC_R" modifier="MKC_MENU" isaddkey="0" />
				<data input="INPUT_BACK" key="MKC_S" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_ATTRIBUTE" key="MKC_T" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_TITAN_AVATAR_NOTIFY" key="MKC_T" modifier="MKC_MENU" isaddkey="0" />
				<data input="INPUT_SUMMARY" key="MKC_U" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_PASSIVE_NOTIFY" key="MKC_V" modifier="MKC_MENU" isaddkey="0" />
				<data input="INPUT_TITAN_AVATAR" key="MKC_W" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_UI_TOGGLE" key="MKC_X" modifier="MKC_CONTROL" isaddkey="0" />
				<data input="INPUT_YES" key="MKC_Y" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_FRONT" key="MKC_Z" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_GUARDIAN_CAM" key="MKC_NUMPAD0" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_WIZARD_CAM" key="MKC_NUMPAD1" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_ADAMAN_CAM" key="MKC_NUMPAD2" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_RUAK_CAM" key="MKC_NUMPAD3" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_TITAN_BLUE_CAM_1" key="MKC_NUMPAD4" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_TITAN_BLUE_CAM_2" key="MKC_NUMPAD5" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_TITAN_BLUE_CAM_3" key="MKC_NUMPAD6" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_TITAN_RED_CAM_1" key="MKC_NUMPAD7" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_TITAN_RED_CAM_2" key="MKC_NUMPAD8" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_TITAN_RED_CAM_3" key="MKC_NUMPAD9" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_FRIENDLY_CAM_1" key="MKC_F1" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_FRIENDLY_CAM_2" key="MKC_F2" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_FRIENDLY_CAM_3" key="MKC_F3" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_FRIENDLY_CAM_4" key="MKC_F4" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_FRIENDLY_CAM_5" key="MKC_F5" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_ENEMY_CAM_1" key="MKC_F6" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_ENEMY_CAM_2" key="MKC_F7" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_ENEMY_CAM_3" key="MKC_F8" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_ENEMY_CAM_4" key="MKC_F9" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_ENEMY_CAM_5" key="MKC_F10" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_CAM_MANUAL" key="MKC_F11" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_CAM_AUTO" key="MKC_F12" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_VIDEO_RECORDING" key="MKC_F12" modifier="MKC_CONTROL" isaddkey="0" />
				<data input="INPUT_CAM_ZOOM_TOGGLE" key="MKC_OEM_1" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_TRADER_INGREDIENT" key="MKC_OEM_PLUS" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_TRADER_SKIN" key="MKC_OEM_PERIOD" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_TRADER_MEDAL" key="MKC_OEM_2" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_TAG" key="MKC_WHEELDOWN" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_TAG_NOTIFY" key="MKC_WHEELDOWN" modifier="MKC_MENU" isaddkey="0" />
				<data input="INPUT_BATTLELOG" key="MKC_OEM_6" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_TOGGLE_INGAME_INFORMATION" key="MKC_OEM_7" modifier="MKC_NOKEY" isaddkey="0" />
				<data input="INPUT_ATTRIBUTE" key="MKC_NOKEY" modifier="MKC_NOKEY" isaddkey="1" />
				<data input="INPUT_RATTACK" key="MKC_RBUTTON" modifier="MKC_CONTROL" isaddkey="1" />
				<data input="INPUT_TAG" key="MKC_WHEELUP" modifier="MKC_NOKEY" isaddkey="1" />
				</KEY_DATA>
				)foo";

		u8 dest[2048];
		uLongf destLen = sizeof(dest);
		int r = compress((Bytef*)dest, &destLen, (Bytef*)src, strlen(src));
		if(r != Z_OK) {
			if(r == Z_MEM_ERROR) LOG("ERROR(compress) not enough memory");
			else if(r == Z_BUF_ERROR) LOG("ERROR(compress) not enough room in the output buffer");
			ASSERT_MSG(0, "compress failed");
		}

		packet.Write<u8>(2); // settingType
		packet.Write<u16>(destLen);
		packet.WriteRaw(dest, destLen);

		LOG("[client%03d] Server :: SN_ClientSettings :: dataLen=%d", clientID, destLen);
		SendPacketData(clientID, Sv::SN_ClientSettings::NET_ID, packet.size, packet.data);
	}

	// SN_FriendList
	{
		u8 sendData[32];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(0); // friendList_count

		LOG("[client%03d] Server :: SN_FriendList :: ", clientID);
		SendPacketData(clientID, Sv::SN_FriendList::NET_ID, packet.size, packet.data);
	}

	// SN_PveComradeInfo
	{
		u8 sendData[32];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<i32>(5); // availableComradeCount
		packet.Write<i32>(5); // maxComradeCount

		LOG("[client%03d] Server :: SN_PveComradeInfo :: ", clientID);
		SendPacketData(clientID, Sv::SN_PveComradeInfo::NET_ID, packet.size, packet.data);
	}

	// SN_AchieveUpdate
	{
		u8 sendData[128];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<i32>(800); // achievementScore
		packet.Write<i32>(300190005); // index
		packet.Write<i32>(1); // type
		packet.Write<u8>(0); // isCleared
		packet.Write<u16>(0); // achievedList_count
		packet.Write<i64>(6); // progressInt64
		packet.Write<i64>(6); // date

		LOG("[client%03d] Server :: SN_AchieveUpdate :: ", clientID);
		SendPacketData(clientID, Sv::SN_AchieveUpdate::NET_ID, packet.size, packet.data);
	}

	// SN_FriendRequestList
	{
		u8 sendData[128];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(0); // friendRequestList_count

		LOG("[client%03d] Server :: SN_FriendRequestList :: ", clientID);
		SendPacketData(clientID, Sv::SN_FriendRequestList::NET_ID, packet.size, packet.data);
	}

	// SN_BlockList
	{
		u8 sendData[128];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(0); // blocks_count

		LOG("[client%03d] Server :: SN_BlockList :: ", clientID);
		SendPacketData(clientID, Sv::SN_BlockList::NET_ID, packet.size, packet.data);
	}

	// SN_MailUnreadNotice
	{
		u8 sendData[128];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(1); // unreadInboxMailCount
		packet.Write<u16>(0); // unreadArchivedMailCount
		packet.Write<u16>(4); // unreadShopMailCount
		packet.Write<u16>(3); // inboxMailCount
		packet.Write<u16>(3); // archivedMailCount
		packet.Write<u16>(16); // shopMailCount
		packet.Write<u16>(0); // newAttachmentsPending_count

		LOG("[client%03d] Server :: SN_MailUnreadNotice :: ", clientID);
		SendPacketData(clientID, Sv::SN_MailUnreadNotice::NET_ID, packet.size, packet.data);
	}

	// SN_WarehouseItems
	{
		u8 sendData[128];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(0); // items_count

		LOG("[client%03d] Server :: SN_WarehouseItems :: ", clientID);
		SendPacketData(clientID, Sv::SN_WarehouseItems::NET_ID, packet.size, packet.data);
	}

	// SN_MutualFriendList
	{
		u8 sendData[128];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(0); // candidates_count

		LOG("[client%03d] Server :: SN_MutualFriendList :: ", clientID);
		SendPacketData(clientID, Sv::SN_MutualFriendList::NET_ID, packet.size, packet.data);
	}

	// SN_GuildMemberStatus
	{
		u8 sendData[128];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(0); // guildMemberStatusList_count

		LOG("[client%03d] Server :: SN_GuildMemberStatus :: ", clientID);
		SendPacketData(clientID, Sv::SN_GuildMemberStatus::NET_ID, packet.size, packet.data);
	}

	// SN_Money
	Sv::SN_Money money;
	money.nMoney = 116472;
	money.nReason = 1;
	LOG("[client%03d] Server :: SN_Money :: ", clientID);
	SendPacket(clientID, money);
}
