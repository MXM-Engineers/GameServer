#include "replication.h"

#include <common/protocol.h>
#include <common/packet_serialize.h>
#include <EASTL/algorithm.h>
#include <EASTL/fixed_hash_map.h>
#include <EAStdC/EAString.h>
#include <game/coordinator.h> // AccountData
#include <game/game_content.h>
#include <game/config.h>

void Replication::Frame::Clear()
{
	playerList.clear();
	playerCharaList.clear();
	npcList.clear();

	playerMap.clear();
	playerCharaMap.clear();
	npcMap.clear();

	actorUIDSet.clear();
	actorType.clear();
}

void Replication::PlayerLocalInfo::Reset()
{
	localActorIDMap.clear();
	actorUIDSet.clear();
	nextPlayerLocalActorID = LocalActorID::FIRST_OTHER_PLAYER;
	nextNpcLocalActorID = LocalActorID::FIRST_NPC;
	nextMonsterLocalActorID = LocalActorID::INVALID;
}

void Replication::Init(Server* server_)
{
	server = server_;
	memset(&playerState, 0, sizeof(playerState));

	framePrev = &frames[0];
	frameCur = &frames[1];
}

void Replication::FrameEnd()
{
	ProfileFunction();

	UpdatePlayersLocalState();

	FrameDifference();

	for(int clientID = 0; clientID < Server::MAX_CLIENTS; clientID++) {
		PlayerStatePair& state = playerState[clientID];

		if(state.prev == PlayerState::CONNECTED && state.cur == PlayerState::IN_GAME) {
			SendInitialFrame(clientID);
		}
		else if(state.prev == PlayerState::IN_GAME && state.cur == PlayerState::LOADED) {
			SendPvpLoadingComplete(clientID);
		}

		state.prev = state.cur;
	}

	eastl::swap(frameCur, framePrev);
	frameCur->Clear(); // clear frame
}

void Replication::FramePushPlayerActor(const ActorPlayer& actor)
{
	ASSERT(frameCur->playerMap.find(actor.actorUID) == frameCur->playerMap.end());
	ASSERT(frameCur->actorUIDSet.find(actor.actorUID) == frameCur->actorUIDSet.end());

	frameCur->playerList.emplace_back(actor);
	frameCur->playerMap.emplace(actor.actorUID, --frameCur->playerList.end());

	// ActorPlayers are not replicated as such, only their characters are
	// So we don't push their actorUID to the actorUIDSet
	// TODO: should they be called "Actor" then?

	//frameCur->actorUIDSet.insert(actor.actorUID);
	//frameCur->actorType.emplace(actor.actorUID, actor.Type());
}

void Replication::FramePushPlayerCharacterActor(const Replication::ActorPlayerCharacter& actor)
{
	ASSERT(frameCur->playerCharaMap.find(actor.actorUID) == frameCur->playerCharaMap.end());
	ASSERT(frameCur->actorUIDSet.find(actor.actorUID) == frameCur->actorUIDSet.end());

	frameCur->playerCharaList.emplace_back(actor);
	frameCur->playerCharaMap.emplace(actor.actorUID, --frameCur->playerCharaList.end());

	frameCur->actorUIDSet.insert(actor.actorUID);
	frameCur->actorType.emplace(actor.actorUID, actor.Type());
}

void Replication::FramePushNpcActor(const Replication::ActorNpc& actor)
{
	ASSERT(frameCur->playerMap.find(actor.actorUID) == frameCur->playerMap.end());
	ASSERT(frameCur->actorUIDSet.find(actor.actorUID) == frameCur->actorUIDSet.end());

	frameCur->npcList.emplace_back(actor);
	frameCur->npcMap.emplace(actor.actorUID, --frameCur->npcList.end());
	frameCur->actorUIDSet.insert(actor.actorUID);
	frameCur->actorType.emplace(actor.actorUID, actor.Type());
}

void Replication::OnPlayerConnect(i32 clientID)
{
	playerState[clientID].cur = PlayerState::CONNECTED;
	playerLocalInfo[clientID].Reset();
}

void Replication::SendLoadLobby(i32 clientID, StageIndex stageIndex)
{
	// SN_LoadCharacterStart
	LOG("[client%03d] Server :: SN_LoadCharacterStart :: ", clientID);
	SendPacketData(clientID, Sv::SN_LoadCharacterStart::NET_ID, 0, nullptr);

	// SN_SetGameGvt
	{
		Sv::SN_SetGameGvt gameGvt;
		// Possible loss of information here? Not sure what the time format should be
		u32 time = (u32)TimeDiffMs(TimeRelNow());
		gameGvt.sendTime = time;
		gameGvt.virtualTime = time;
		LOG("[client%03d] Server :: SN_SetGameGvt :: ", clientID);
		SendPacket(clientID, gameGvt);
	}

	// SN_SummaryInfoAll
	{
		u8 sendData[128];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(0);

		LOG("[client%03d] Server :: SN_SummaryInfoAll :: ", clientID);
		SendPacketData(clientID, Sv::SN_SummaryInfoAll::NET_ID, packet.size, packet.data);
	}

	// SN_AvailableSummaryRewardCountList
	{
		u8 sendData[128];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(8);

		const i32 rewardCount[8] = {
			220004100,
			2,
			220002200,
			3,
			220005100,
			3,
			220003300,
			2
		};

		packet.WriteRaw(rewardCount, sizeof(rewardCount));

		LOG("[client%03d] Server :: SN_AvailableSummaryRewardCountList :: ", clientID);
		SendPacketData(clientID, Sv::SN_AvailableSummaryRewardCountList::NET_ID, packet.size, packet.data);
	}


	// SN_SummaryInfoLatest
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(10);

		const Sv::SN_SummaryInfoLatest::Info infoList[10] = {
			{ 220002200, 200002201, 3, 220002201, 0, (i64)0xa6605c00 },
			{ 220002200, 200002201, 3, 220002204, 0, (i64)0xa6605c00 },
			{ 220002200, 200002201, 3, 220002205, 0, (i64)0xa6605c00 },
			{ 220005100, 200005101, 3, 220005101, 0, (i64)0x6a2bfc00 },
			{ 220005100, 200005101, 3, 220005102, 0, (i64)0x6a2bfc00 },
			{ 220005100, 200005101, 3, 220005105, 0, (i64)0x6a2bfc00 },
			{ 220003300, 200003301, 3, 220003301, 0, (i64)0xe0c7fa00 },
			{ 220003300, 200003301, 3, 220003305, 0, (i64)0xe0c7fa00 },
			{ 220004100, 200004101, 3, 220004101, 0, (i64)0x9eea8400 },
			{ 220004100, 200004101, 3, 220004105, 0, (i64)0x9eea8400 },
		};

		packet.WriteRaw(infoList, sizeof(infoList));

		LOG("[client%03d] Server :: SN_SummaryInfoLatest :: ", clientID);
		SendPacketData(clientID, Sv::SN_SummaryInfoLatest::NET_ID, packet.size, packet.data);
	}

	// SN_AchieveInfo
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u8>(1); // packetNum
		packet.Write<i32>(800); // achievementScore
		packet.Write<u16>(0); // achList_count

		LOG("[client%03d] Server :: SN_AchieveInfo :: ", clientID);
		SendPacketData(clientID, Sv::SN_AchieveInfo::NET_ID, packet.size, packet.data);
	}

	// SN_AchieveLatest
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(0); // achList_count

		LOG("[client%03d] Server :: SN_AchieveLatest :: ", clientID);
		SendPacketData(clientID, Sv::SN_AchieveLatest::NET_ID, packet.size, packet.data);
	}

	// SN_CityMapInfo
	Sv::SN_CityMapInfo cityMapInfo;
	cityMapInfo.cityMapID = (StageIndex)Config().LobbyMap;
	LOG("[client%03d] Server :: SN_CityMapInfo :: cityMapID=%d", clientID, (i32)cityMapInfo.cityMapID);
	SendPacket(clientID, cityMapInfo);

	// SQ_CityLobbyJoinCity
	LOG("[client%03d] Server :: SQ_CityLobbyJoinCity :: ", clientID);
	SendPacketData(clientID, Sv::SQ_CityLobbyJoinCity::NET_ID, 0, nullptr);

	// SN_SetGameGvt
	{
		Sv::SN_SetGameGvt gameGvt;
		gameGvt.sendTime = 0;
		gameGvt.virtualTime = 0;
		LOG("[client%03d] Server :: SN_SetGameGvt :: ", clientID);
		SendPacket(clientID, gameGvt);
	}
}

void Replication::SendLoadPvpMap(i32 clientID, StageIndex stageIndex)
{
	Sv::SN_UpdateGameOwner owner;
	owner.userID = 1;
	LOG("[client%03d] Server :: SN_UpdateGameOwner :: { userID=%d }", clientID, owner.userID);
	SendPacket(clientID, owner);

	Sv::SN_LobbyStartGame lobby;
	lobby.stageType = StageType::GAME_INSTANCE;
	LOG("[client%03d] Server :: SN_LobbyStartGame :: { stageType=GAME_INSTANCE }", clientID);
	SendPacket(clientID, lobby);

	// SN_CityMapInfo
	Sv::SN_CityMapInfo cityMapInfo;
	cityMapInfo.cityMapID = stageIndex;
	LOG("[client%03d] Server :: SN_CityMapInfo :: { cityMapID=%d }", clientID, (i32)cityMapInfo.cityMapID);
	SendPacket(clientID, cityMapInfo);

	/*
	// SN_WeaponState
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<LocalActorID>((LocalActorID)21035); // ownerID
		packet.Write<i32>(90); // weaponID
		packet.Write<i32>(0); // state
		packet.Write<u8>(0); // chargeLevel
		packet.Write<u8>(0); // firingCombo
		packet.Write<i32>(-1); // result

		LOG("[client%03d] Server :: %s", clientID, PacketSerialize<Sv::SN_WeaponState>(packet.data, packet.size));
		SendPacketData(clientID, Sv::SN_WeaponState::NET_ID, packet.size, packet.data);
	}

	playerState[clientID].cur = PlayerState::IN_GAME;*/
}

void Replication::SetPlayerAsInGame(i32 clientID)
{
	playerState[clientID].cur = PlayerState::IN_GAME;
}

void Replication::SetPlayerLoaded(i32 clientID)
{
	playerState[clientID].cur = PlayerState::LOADED;
}

void Replication::SendCharacterInfo(i32 clientID, ActorUID actorUID, CreatureIndex docID, ClassType classType, i32 health, i32 healthMax)
{
	ASSERT(clientID >= 0 && clientID < Server::MAX_CLIENTS);

	if(playerState[clientID].cur < PlayerState::IN_GAME) {
		LOG("WARNING(SendCharacterInfo): player not in game (clientID=%d, state=%d)", clientID, (i32)playerState[clientID].cur);
		return;
	}

	// SA_GetCharacterInfo
	Sv::SA_GetCharacterInfo info;
	info.characterID = GetLocalActorID(clientID, actorUID);
	info.docIndex = docID;
	info.classType = classType;
	info.hp = health;
	info.maxHp = healthMax;
	LOG("[client%03d] Server :: SA_GetCharacterInfo :: ", clientID);
	SendPacket(clientID, info);
}

void Replication::SendPlayerSetLeaderMaster(i32 clientID, ActorUID masterActorUID, ClassType classType, SkinIndex skinIndex)
{
	LocalActorID laiLeader = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)classType);
	ASSERT(laiLeader >= LocalActorID::FIRST_SELF_MASTER && laiLeader < LocalActorID::LAST_SELF_MASTER);

	PlayerForceLocalActorID(clientID, masterActorUID, laiLeader);

	if(playerState[clientID].cur < PlayerState::IN_GAME) {
		// SN_LeaderCharacter
		Sv::SN_LeaderCharacter leader;
		leader.leaderID = laiLeader;
		leader.skinIndex = skinIndex;
		LOG("[client%03d] Server :: SN_LeaderCharacter :: actorUID=%d localActorID=%d", clientID, (u32)masterActorUID, (u32)laiLeader);
		SendPacket(clientID, leader);
	}
	else {
		// NOTE: only seems to close the master window
		// SA_LeaderCharacter
		Sv::SA_SetLeader leader;
		leader.result = 0;
		leader.leaderID = laiLeader;
		leader.skinIndex = skinIndex;
		LOG("[client%03d] Server :: SA_SetLeader :: actorUID=%d localActorID=%d", clientID, (u32)masterActorUID, (u32)laiLeader);
		SendPacket(clientID, leader);
	}
}

void Replication::SendChatMessageToAll(const wchar* senderName, i32 chatType, const wchar* msg, i32 msgLen)
{
	// TODO: restrict message length

	u8 sendData[2048];
	PacketWriter packet(sendData, sizeof(sendData));

	packet.Write<i32>(chatType); // chatType
	packet.WriteStringObj(senderName);
	packet.Write<u8>(0); // senderStaffType
	packet.WriteStringObj(msg, msgLen);

	for(int clientID= 0; clientID < Server::MAX_CLIENTS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;

		LOG("[client%03d] Server :: SN_ChatChannelMessage :: sender='%ls' msg='%.*S'", clientID, senderName, msgLen, msg);
		SendPacketData(clientID, Sv::SN_ChatChannelMessage::NET_ID, packet.size, packet.data);
	}
}

void Replication::SendChatMessageToClient(i32 toClientID, const wchar* senderName, i32 chatType, const wchar* msg, i32 msgLen)
{
	ASSERT(toClientID >= 0 && toClientID < Server::MAX_CLIENTS);
	if(playerState[toClientID].cur < PlayerState::IN_GAME) return;

	if(msgLen == -1) msgLen = EA::StdC::Strlen(msg);

	u8 sendData[2048];
	PacketWriter packet(sendData, sizeof(sendData));

	packet.Write<i32>(chatType); // chatType
	packet.WriteStringObj(senderName);
	packet.Write<u8>(0); // senderStaffType
	packet.WriteStringObj(msg, msgLen);

	LOG("[client%03d] Server :: SN_ChatChannelMessage :: sender='%ls' msg='%.*S'", toClientID, senderName, msgLen, msg);
	SendPacketData(toClientID, Sv::SN_ChatChannelMessage::NET_ID, packet.size, packet.data);
}

void Replication::SendChatWhisperConfirmToClient(i32 senderClientID, const wchar* destNick, const wchar* msg)
{
	u8 sendData[2048];
	PacketWriter packet(sendData, sizeof(sendData));

	packet.Write<i32>(0); // result
	packet.WriteStringObj(destNick);
	packet.WriteStringObj(msg);

	LOG("[client%03d] Server :: SA_WhisperSend :: destNick='%ls' msg='%ls'", senderClientID, destNick, msg);
	SendPacketData(senderClientID, Sv::SA_WhisperSend::NET_ID, packet.size, packet.data);
}

void Replication::SendChatWhisperToClient(i32 destClientID, const wchar* senderName, const wchar* msg)
{
	u8 sendData[2048];
	PacketWriter packet(sendData, sizeof(sendData));

	packet.WriteStringObj(senderName); // senderNick
	packet.Write<u8>(0); // staffType
	packet.WriteStringObj(msg); // msg

	LOG("[client%03d] Server :: SN_WhisperReceive :: senderName='%ls' msg='%ls'", destClientID, senderName, msg);
	SendPacketData(destClientID, Sv::SN_WhisperReceive::NET_ID, packet.size, packet.data);
}

void Replication::SendAccountDataLobby(i32 clientID, const AccountData& account)
{
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

	const GameXmlContent& content = GetGameXmlContent();

	// SN_ProfileCharacters
	{
		u8 sendData[2048];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(content.masters.size()); // charaList_count

		foreach(it, content.masters) {
			Sv::SN_ProfileCharacters::Character chara;
			chara.characterID = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)it->classType);
			chara.creatureIndex = it->ID;
			chara.skillShot1 = it->skillIDs[0];
			chara.skillShot2 = it->skillIDs[1];
			chara.classType = it->classType;
			chara.x = 0;
			chara.y = 0;
			chara.z = 0;
			chara.characterType = 1;
			chara.skinIndex = 0;
			chara.weaponIndex = it->weaponIDs[0];
			chara.masterGearNo = 1;
			packet.Write(chara);
		}

		LOG("[client%03d] Server :: SN_ProfileCharacters :: ", clientID);
		SendPacketData(clientID, Sv::SN_ProfileCharacters::NET_ID, packet.size, packet.data);
	}

	// TODO: send weapons
	/*
	// SN_ProfileWeapons
	{
		u8 sendData[2048];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(3); // weaponList_count

		Sv::SN_ProfileWeapons::Weapon weap;
		weap.characterID = LocalActorID::FIRST_SELF_MASTER;
		weap.weaponType = 1;
		weap.weaponIndex = 131135012;
		weap.grade = 1;
		weap.isUnlocked = 1;
		weap.isActivated = 1;
		packet.Write(weap);

		weap.characterID = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + 1);
		weap.weaponType = 1;
		weap.weaponIndex = 131135012;
		weap.grade = 1;
		weap.isUnlocked = 1;
		weap.isActivated = 1;
		packet.Write(weap);

		weap.characterID = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + 2);
		weap.weaponType = 1;
		weap.weaponIndex = 131135012;
		weap.grade = 1;
		weap.isUnlocked = 1;
		weap.isActivated = 1;
		packet.Write(weap);

		LOG("[client%03d] Server :: SN_ProfileWeapons :: ", clientID);
		SendPacketData(clientID, Sv::SN_ProfileWeapons::NET_ID, packet.size, packet.data);
	}
	*/

	// SN_MyGuild
	{
		u8 sendData[256];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.WriteStringObj(L"Alpha");
		packet.Write<i64>(0);
		packet.Write<u8>(0);

		LOG("[client%03d] Server :: SN_MyGuild :: ", clientID);
		SendPacketData(clientID, Sv::SN_MyGuild::NET_ID, packet.size, packet.data);
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

		packet.Write<u8>(1); // packetNum
		packet.Write<u16>(1); // items_count

		// jukebox coins
		packet.Write<i32>(1073741864); // itemID
		packet.Write<u8>(0); // invenType
		packet.Write<i32>(200); // slot
		packet.Write<i32>(137120001); // itemIndex -> actual jukebox coin identifier
		packet.Write<i32>(1337); // count
		packet.Write<i32>(-1); // propertyGroupIndex
		packet.Write<u8>(0); // isLifeTimeAbsolute
		packet.Write<i64>(0); // lifeEndTimeUTC
		packet.Write<u16>(0); // properties_count

		LOG("[client%03d] Server :: SN_ProfileItems :: ", clientID);
		SendPacketData(clientID, Sv::SN_ProfileItems::NET_ID, packet.size, packet.data);
	}

	// SN_ProfileSkills
	{
		u8 sendData[4096];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u8>(1); // packetNum

		i32 skillCount = 0;
		foreach(it, content.masters) {
			foreach(skill, it->skillIDs) {
				skillCount++;
			}
		}
		packet.Write<u16>(skillCount); // skills_count

		foreach(it, content.masters) {
			foreach(skill, it->skillIDs) {
				packet.Write<LocalActorID>((LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)it->classType)); // characterID
				packet.Write<SkillID>(*skill);
				packet.Write<u8>(1); // isUnlocked
				packet.Write<u8>(1); // isActivated
				packet.Write<u16>(0); // properties_count
			}
		}

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
		u8 sendData[4096];
		PacketWriter packet(sendData, sizeof(sendData));

		i32 skinCount = 0;
		foreach(it, content.masters) {
			skinCount += it->skinIDs.size();
		}

		packet.Write<u16>(skinCount); // skins_count

		foreach(it, content.masters) {
			const ClassType classType = it->classType;

			foreach(s, it->skinIDs) {
				packet.Write<ClassType>(classType); // classType
				packet.Write<SkinIndex>(*s); // skinIndex
				packet.Write<i32>(0); // bufCount
				packet.Write<i64>(0); // expireDateTime
			}
		}

		LOG("[client%03d] Server :: SN_ProfileCharacterSkinList :: ", clientID);
		SendPacketData(clientID, Sv::SN_ProfileCharacterSkinList::NET_ID, packet.size, packet.data);
	}

	// SN_AccountInfo
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.WriteStringObj(account.nickname.data()); // nick
		packet.Write<i32>(4); // inventoryLineCountTab0
		packet.Write<i32>(4); // inventoryLineCountTab1
		packet.Write<i32>(4); // inventoryLineCountTab2
#if 0
		packet.Write<i32>(320080005); // displayTitlteIndex
		packet.Write<i32>(320080005); // statTitleIndex
#else // disable title
		packet.Write<i32>(0); // displayTitlteIndex
		packet.Write<i32>(0); // statTitleIndex
#endif
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
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.WriteStringObj(L"Alpha"); // guildName
		packet.WriteStringObj(account.nickname.data()); // nick
		packet.Write<u8>(0); // onlineStatus

		LOG("[client%03d] Server :: SN_GuildChannelEnter :: ", clientID);
		SendPacketData(clientID, Sv::SN_GuildChannelEnter::NET_ID, packet.size, packet.data);
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

void Replication::SendAccountDataPvp(i32 clientID, const AccountData& account)
{
	// SN_Money
	// SN_ProfileCharacters
	// SN_ProfileItems
	// SN_ProfileWeapons
	// SN_ProfileSkills
	// SN_ProfileMasterGears
	// SN_PlayerSkillSlot x2
	// SN_AccountEquipmentList
	// SN_GameFieldReady

	const GameXmlContent& content = GetGameXmlContent();

	// SN_ProfileCharacters
	{
		u8 sendData[2048];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(2); // charaList_count

		foreach(it, content.masters) {
			if(it->classType != ClassType::LUA) continue;

			Sv::SN_ProfileCharacters::Character chara;
			chara.characterID = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)it->classType);
			chara.creatureIndex = it->ID;
			chara.skillShot1 = it->skillIDs[0];
			chara.skillShot2 = it->skillIDs[1];
			chara.classType = it->classType;
			chara.x = 0;
			chara.y = 0;
			chara.z = 0;
			chara.characterType = 1;
			chara.skinIndex = 0;
			chara.weaponIndex = it->weaponIDs[1];
			chara.masterGearNo = 1;
			packet.Write(chara);
		}
		foreach(it, content.masters) {
			if(it->classType != ClassType::SIZUKA) continue;

			Sv::SN_ProfileCharacters::Character chara;
			chara.characterID = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)it->classType);
			chara.creatureIndex = it->ID;
			chara.skillShot1 = it->skillIDs[0];
			chara.skillShot2 = it->skillIDs[1];
			chara.classType = it->classType;
			chara.x = 0;
			chara.y = 0;
			chara.z = 0;
			chara.characterType = 1;
			chara.skinIndex = 0;
			chara.weaponIndex = it->weaponIDs[1];
			chara.masterGearNo = 1;
			packet.Write(chara);
		}

		LOG("[client%03d] Server :: %s", clientID, PacketSerialize<Sv::SN_ProfileCharacters>(packet.data, packet.size));
		SendPacketData(clientID, Sv::SN_ProfileCharacters::NET_ID, packet.size, packet.data);
	}

	// SN_ProfileWeapons
	{
		u8 sendData[2048];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(2); // weaponList_count

		Sv::SN_ProfileWeapons::Weapon weap;
		weap.characterID = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)ClassType::LUA);
		weap.weaponType = 1;
		weap.weaponIndex = 131135011;
		weap.grade = 0;
		weap.isUnlocked = 1;
		weap.isActivated = 1;
		packet.Write(weap);

		weap.characterID = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)ClassType::SIZUKA);
		weap.weaponType = 1;
		weap.weaponIndex = 131103011;
		weap.grade = 0;
		weap.isUnlocked = 1;
		weap.isActivated = 1;
		packet.Write(weap);

		LOG("[client%03d] Server :: SN_ProfileWeapons :: %s", clientID, PacketSerialize<Sv::SN_ProfileWeapons>(packet.data, packet.size));
		SendPacketData(clientID, Sv::SN_ProfileWeapons::NET_ID, packet.size, packet.data);
	}

	// SN_ProfileItems
	{
		u8 sendData[128];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u8>(1); // packetNum
		packet.Write<u16>(0); // items_count

		/*
		// jukebox coins
		packet.Write<i32>(1073741864); // itemID
		packet.Write<u8>(0); // invenType
		packet.Write<i32>(200); // slot
		packet.Write<i32>(137120001); // itemIndex -> actual jukebox coin identifier
		packet.Write<i32>(1337); // count
		packet.Write<i32>(-1); // propertyGroupIndex
		packet.Write<u8>(0); // isLifeTimeAbsolute
		packet.Write<i64>(0); // lifeEndTimeUTC
		packet.Write<u16>(0); // properties_count
		*/

		LOG("[client%03d] Server :: SN_ProfileItems :: ", clientID);
		SendPacketData(clientID, Sv::SN_ProfileItems::NET_ID, packet.size, packet.data);
	}

	// SN_ProfileSkills
	{
		u8 sendData[8192];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u8>(1); // packetNum

		i32 skillCount = 0;
		foreach(it, content.masters) {
			if(it->classType != ClassType::SIZUKA && it->classType != ClassType::LUA) continue;

			foreach(skill, it->skillIDs) {
				skillCount++;
			}
		}

		packet.Write<u16>(skillCount); // skills_count

		/*
		foreach(it, content.masters) {
			if(it->classType != ClassType::SIZUKA && it->classType != ClassType::LUA) continue;

			foreach(skill, it->skillIDs) {
				packet.Write<LocalActorID>((LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)it->classType)); // characterID
				packet.Write<SkillID>(*skill);
				packet.Write<u8>(1); // isUnlocked
				packet.Write<u8>(1); // isActivated
				packet.Write<u16>(0); // properties_count
			}
		}
		*/

		struct SkillStatus {
			u8 isUnlocked;
			u8 isActivated;
		};

		// NOTE: not having all skills enabled here is important. There are 5 skills per master selectable at a time.
		const SkillStatus skillStatusList[7] = {
			{ 1, 1 },
			{ 1, 1 },
			{ 0, 0 },
			{ 0, 0 },
			{ 1, 1 },
			{ 1, 1 },
			{ 1, 1 },
		};

		foreach(it, content.masters) {
			if(it->classType != ClassType::LUA) continue;

			i32 skillStatusID = 0;
			foreach(skill, it->skillIDs) {
				packet.Write<LocalActorID>((LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)it->classType)); // characterID
				packet.Write<SkillID>(*skill);
				packet.Write<u8>(skillStatusList[skillStatusID].isUnlocked); // isUnlocked
				packet.Write<u8>(skillStatusList[skillStatusID].isActivated); // isActivated
				packet.Write<u16>(0); // properties_count

				skillStatusID++;
			}
		}
		foreach(it, content.masters) {
			if(it->classType != ClassType::SIZUKA) continue;

			i32 skillStatusID = 0;
			foreach(skill, it->skillIDs) {
				packet.Write<LocalActorID>((LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)it->classType)); // characterID
				packet.Write<SkillID>(*skill);
				packet.Write<u8>(skillStatusList[skillStatusID].isUnlocked); // isUnlocked
				packet.Write<u8>(skillStatusList[skillStatusID].isActivated); // isActivated
				packet.Write<u16>(0); // properties_count

				skillStatusID++;
			}
		}

		LOG("[client%03d] Server :: %s", clientID, PacketSerialize<Sv::SN_ProfileSkills>(packet.data, packet.size));
		SendPacketData(clientID, Sv::SN_ProfileSkills::NET_ID, packet.size, packet.data);
	}

	// SN_ProfileMasterGears
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(1); // masterGears_count

		packet.Write<u8>(1); // masterGearNo
		packet.WriteStringObj(L""); // name
		packet.Write<u16>(6); // slots_count
		packet.Write<i32>(-1); // gearType
		packet.Write<i32>(0); // gearItemID
		packet.Write<i32>(-1); // gearType
		packet.Write<i32>(0); // gearItemID
		packet.Write<i32>(-1); // gearType
		packet.Write<i32>(0); // gearItemID
		packet.Write<i32>(-1); // gearType
		packet.Write<i32>(0); // gearItemID
		packet.Write<i32>(-1); // gearType
		packet.Write<i32>(0); // gearItemID
		packet.Write<i32>(80); // gearType
		packet.Write<i32>(1073741825); // gearItemID

		LOG("[client%03d] Server :: SN_ProfileMasterGears :: ", clientID);
		SendPacketData(clientID, Sv::SN_ProfileMasterGears::NET_ID, packet.size, packet.data);
	}


	// SN_AccountInfo
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.WriteStringObj(account.nickname.data()); // nick
		packet.Write<i32>(4); // inventoryLineCountTab0
		packet.Write<i32>(4); // inventoryLineCountTab1
		packet.Write<i32>(4); // inventoryLineCountTab2
#if 0
		packet.Write<i32>(320080005); // displayTitlteIndex
		packet.Write<i32>(320080005); // statTitleIndex
#else // disable title
		packet.Write<i32>(-1); // displayTitlteIndex
		packet.Write<i32>(-1); // statTitleIndex
#endif
		packet.Write<i32>(1); // warehouseLineCount
		packet.Write<i32>(-1); // tutorialState
		packet.Write<i32>(3600); // masterGearDurability
		packet.Write<u8>(0); // badgeType

		LOG("[client%03d] Server :: SN_AccountInfo :: ", clientID);
		SendPacketData(clientID, Sv::SN_AccountInfo::NET_ID, packet.size, packet.data);
	}

	// SN_AccountEquipmentList
	{
		u8 sendData[128];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<i32>(-1); // supportKitDocIndex

		LOG("[client%03d] Server :: SN_AccountEquipmentList :: ", clientID);
		SendPacketData(clientID, Sv::SN_AccountEquipmentList::NET_ID, packet.size, packet.data);
	}

	// SN_Money
	Sv::SN_Money money;
	money.nMoney = 666;
	money.nReason = 1;
	LOG("[client%03d] Server :: SN_Money :: ", clientID);
	SendPacket(clientID, money);

	// SN_GameFieldReady
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<i32>(449); // InGameID=449
		packet.Write<i32>(6); // GameType=6
		packet.Write<i32>(190002202); // AreaIndex=190002202
		packet.Write<i32>(200020104); // StageIndex=200020104
		packet.Write<i32>(1); // GameDefinitionType=1
		packet.Write<u8>(6); // initPlayerCount=6
		packet.Write<u8>(1); // CanEscape=1
		packet.Write<u8>(0); // IsTrespass=0
		packet.Write<u8>(0); // IsSpectator=0

		// InGameUsers
		packet.Write<u16>(6);
		packet.Write<i32>(1); //userID
		packet.WriteStringObj(account.nickname.data()); // nickname
		packet.Write<u8>(3); // team
		packet.Write<u8>(0); // isBot
		for(int i = 1; i < 6; i++) {
			packet.Write<i32>(i+1); //userID
			packet.WriteStringObj(LFMT(L"Player_%d", i)); // nickname
			packet.Write<u8>(3 + i/3); // team
			packet.Write<u8>(0); // isBot
		}

		// IngamePlayers
		packet.Write<u16>(6);
		for(int i = 0; i < 6; i++) {
			packet.Write<i32>(i+1); //userID
			packet.Write<CreatureIndex>((CreatureIndex)100000035); //mainCreatureIndex
			packet.Write<i32>(0); //mainSkinIndex

			if(i == 0) {
				packet.Write<i32>(180350010); //mainSkillindex1
				packet.Write<i32>(180350030); //mainSkillIndex2
			}
			else {
				packet.Write<i32>(-1); //mainSkillindex1
				packet.Write<i32>(-1); //mainSkillIndex2
			}

			packet.Write<CreatureIndex>((CreatureIndex)100000003); //subCreatureIndex
			packet.Write<i32>(0); //subSkinIndex

			if(i == 0) {
				packet.Write<i32>(180030020); //subSkillIndex1
				packet.Write<i32>(180030030); //subSkillIndex2
			}
			else {
				packet.Write<i32>(-1); //subSkillIndex1
				packet.Write<i32>(-1); //subSkillIndex2
			}
			packet.Write<i32>(-1); //stageSkillIndex1
			packet.Write<i32>(-1); //stageSkillIndex2
			packet.Write<i32>(-1); //supportKitIndex
			packet.Write<u8>(0); //isBot
		}

		// IngameGuilds
		packet.Write<u16>(0);
		packet.Write<u32>(180000); // surrenderAbleTime

		LOG("[client%03d] Server :: %s", clientID, PacketSerialize<Sv::SN_GameFieldReady>(packet.data, packet.size));
		SendPacketData(clientID, Sv::SN_GameFieldReady::NET_ID, packet.size, packet.data);
	}
}

void Replication::SendConnectToServer(i32 clientID, const AccountData& account, const u8 ip[4], u16 port)
{
	u8 sendData[1024];
	PacketWriter packet(sendData, sizeof(sendData));

	packet.Write<u16>(port);
	packet.WriteRaw(ip, 4);
	packet.Write<i32>(449); // gameID
	packet.Write<u32>(3490298546); // idcHash
	packet.WriteStringObj(account.nickname.data(), account.nickname.size());
	packet.Write<i32>(340); // instantKey

	LOG("[client%03d] Server :: %s", clientID, PacketSerialize<Sv::SN_DoConnectGameServer>(packet.data, packet.size));
	SendPacketData(clientID, Sv::SN_DoConnectGameServer::NET_ID, packet.size, packet.data);

	// NOTE: client will disconnect on reception
}

void Replication::SendPvpLoadingComplete(i32 clientID)
{
	const i32 playerCount = frameCur->playerList.size();

	// SN_NotifyPcDetailInfos
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));
		packet.Write<u16>(playerCount); // pcList_count

		int i = 0;
		foreach_const(pl, frameCur->playerList) {
			const ActorPlayer& player = *pl;
			const ActorPlayerCharacter* main = frameCur->FindPlayerChara(player.characters[0]);
			const ActorPlayerCharacter* sub = frameCur->FindPlayerChara(player.characters[1]);
			ASSERT(main);
			ASSERT(sub);

			packet.Write<i32>(i); // userID
			// mainPC
			packet.Write<LocalActorID>(GetLocalActorID(clientID, main->actorUID)); // characterID
			packet.Write<CreatureIndex>(CreatureIndex(100000000 + (i32)main->classType)); // docID
			packet.Write<ClassType>(main->classType); // classType
			packet.Write<i32>(2400); // hp
			packet.Write<i32>(2400); // maxHp
			// subPC
			packet.Write<LocalActorID>(GetLocalActorID(clientID, sub->actorUID)); // characterID
			packet.Write<CreatureIndex>(CreatureIndex(100000000 + (i32)sub->classType)); // docID
			packet.Write<ClassType>(sub->classType); // classType
			packet.Write<i32>(2400); // hp
			packet.Write<i32>(2400); // maxHp

			packet.Write<i32>(0); // remainTagCooltimeMS
			packet.Write<u8>(0); // canCastSkillSlotUG
			i++;
		}


		LOG("[client%03d] Server :: %s", clientID, PacketSerialize<Sv::SN_NotifyPcDetailInfos>(packet.data, packet.size));
		SendPacketData(clientID, Sv::SN_NotifyPcDetailInfos::NET_ID, packet.size, packet.data);
	}

	// SN_InitScoreBoard
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(playerCount); // userInfos_count

		int i = 0;
		foreach_const(pl, frameCur->playerList) {
			const ActorPlayer& player = *pl;
			const ActorPlayerCharacter* main = frameCur->FindPlayerChara(player.characters[0]);
			const ActorPlayerCharacter* sub = frameCur->FindPlayerChara(player.characters[1]);
			ASSERT(main);
			ASSERT(sub);

			packet.Write<i32>(i); // usn
			packet.WriteStringObj(player.name.data());
			packet.Write<i32>(3 + i/3); // teamType | TODO: team
			packet.Write<CreatureIndex>(CreatureIndex(100000000 + (i32)main->classType)); // mainCreatureIndex
			packet.Write<CreatureIndex>(CreatureIndex(100000000 + (i32)sub->classType)); // subCreatureIndex
			i++;
		}


		LOG("[client%03d] Server :: %s", clientID, PacketSerialize<Sv::SN_InitScoreBoard>(packet.data, packet.size));
		SendPacketData(clientID, Sv::SN_InitScoreBoard::NET_ID, packet.size, packet.data);
	}

	// SA_LoadingComplete
	LOG("[client%03d] Server :: SA_LoadingComplete ::", clientID);
	SendPacketData(clientID, Sv::SA_LoadingComplete::NET_ID, 0, nullptr);
}

void Replication::SendGameReady(i32 clientID)
{
	Sv::SA_GameReady ready;
	ready.waitingTimeMs = 3000;
	ready.serverTimestamp = (i64)TimeDiffMs(TimeRelNow());
	ready.readyElapsedMs = 0;
	LOG("[client%03d] Server :: SA_GameReady ::", clientID);
	SendPacket(clientID, ready);

	Sv::SN_NotifyIngameSkillPoint notify;
	notify.userID = 1;
	notify.skillPoint = 1;
	LOG("[client%03d] Server :: SN_NotifyIngameSkillPoint", clientID);
	SendPacket(clientID, notify);

	Sv::SN_NotifyTimestamp notifyTimestamp;
	notifyTimestamp.serverTimestamp = (i64)TimeDiffMs(TimeRelNow());
	notifyTimestamp.curCount = 4;
	notifyTimestamp.maxCount = 5;
	LOG("[client%03d] Server :: SN_NotifyTimestamp", clientID);
	SendPacket(clientID, notifyTimestamp);

	/*
	// Sv::SN_RunClientLevelEventSeq
	{
		Sv::SN_RunClientLevelEventSeq seq;
		seq.needCompleteTriggerAckID = -1;
		seq.rootEventID = 218;
		seq.caller = 0;
		seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
		LOG("[client%03d] Server :: SN_RunClientLevelEventSeq", clientID);
		SendPacket(clientID, seq);
	}
	// Sv::SN_RunClientLevelEventSeq
	{
		Sv::SN_RunClientLevelEventSeq seq;
		seq.needCompleteTriggerAckID = -1;
		seq.rootEventID = 219;
		seq.caller = 0;
		seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
		LOG("[client%03d] Server :: SN_RunClientLevelEventSeq", clientID);
		SendPacket(clientID, seq);
	}
	// Sv::SN_RunClientLevelEventSeq
	{
		Sv::SN_RunClientLevelEventSeq seq;
		seq.needCompleteTriggerAckID = -1;
		seq.rootEventID = 274;
		seq.caller = 0;
		seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
		LOG("[client%03d] Server :: SN_RunClientLevelEventSeq", clientID);
		SendPacket(clientID, seq);
	}
	// Sv::SN_RunClientLevelEvent
	{
		Sv::SN_RunClientLevelEvent event;
		event.eventID = 48;
		event.caller = 0;
		event.serverTime = (i64)TimeDiffMs(TimeRelNow());
		LOG("[client%03d] Server :: SN_RunClientLevelEvent", clientID);
		SendPacket(clientID, event);
	}
	// Sv::SN_RunClientLevelEventSeq
	{
		Sv::SN_RunClientLevelEventSeq seq;
		seq.needCompleteTriggerAckID = 1000000;
		seq.rootEventID = 1000001;
		seq.caller = 21035;
		seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
		LOG("[client%03d] Server :: SN_RunClientLevelEventSeq", clientID);
		SendPacket(clientID, seq);
	}
	// Sv::SN_RunClientLevelEvent
	{
		Sv::SN_RunClientLevelEvent event;
		event.eventID = 150;
		event.caller = 21035;
		event.serverTime = (i64)TimeDiffMs(TimeRelNow());
		LOG("[client%03d] Server :: SN_RunClientLevelEvent", clientID);
		SendPacket(clientID, event);
	}
	*/

	/*
	Sv::SN_NotifyIsInSafeZone safe;
	safe.userID = 1;
	safe.inSafeZone = 1;
	LOG("[client%03d] Server :: SN_NotifyIsInSafeZone", clientID);
	SendPacket(clientID, safe);
	*/
}

void Replication::SendGameStart(i32 clientID)
{
	LOG("[client%03d] Server :: SN_GameStart", clientID);
	SendPacketData(clientID, Sv::SN_GameStart::NET_ID, 0, nullptr);

	Sv::SN_NotifyAasRestricted notify;
	notify.isRestrictedByAAS = 0;
	LOG("[client%03d] Server :: SN_NotifyAasRestricted :: isRestrictedByAAS=%d", clientID, notify.isRestrictedByAAS);
	SendPacket(clientID, notify);

	// Release input lock (Data\Design\Level\PVP\PVP_DeathMatch\EVENTNODES\LEVELEVENT_CLIENT.XML)

	// Sv::SN_RunClientLevelEventSeq
	{
		Sv::SN_RunClientLevelEventSeq seq;
		seq.needCompleteTriggerAckID = -1;
		seq.rootEventID = 163;
		seq.caller = 0;
		seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
		LOG("[client%03d] Server :: SN_RunClientLevelEventSeq", clientID);
		SendPacket(clientID, seq);
	}

	// Timer related event

	// Sv::SN_RunClientLevelEventSeq
	{
		Sv::SN_RunClientLevelEventSeq seq;
		seq.needCompleteTriggerAckID = -1;
		seq.rootEventID = 270;
		seq.caller = 0;
		seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
		LOG("[client%03d] Server :: SN_RunClientLevelEventSeq", clientID);
		SendPacket(clientID, seq);
	}
}

void Replication::SendPlayerTag(i32 clientID, ActorUID mainActorUID, ActorUID subActorUID)
{
	Sv::SN_GamePlayerTag tag;
	tag.result = 128;
	tag.mainID = GetLocalActorID(clientID, mainActorUID);
	tag.subID = GetLocalActorID(clientID, subActorUID);
	tag.attackerID = LocalActorID::INVALID;

	ASSERT(tag.mainID != LocalActorID::INVALID);
	ASSERT(tag.subID != LocalActorID::INVALID);

	LOG("[client%03d] Server :: SN_GamePlayerTag", clientID);
	SendPacket(clientID, tag);
}

void Replication::SendPlayerJump(i32 clientID, ActorUID mainActorUID, f32 rotate, f32 moveDirX, f32 moveDirY)
{
	LocalActorID localActorID = GetLocalActorID(clientID, mainActorUID);
	ASSERT(localActorID != LocalActorID::INVALID);

	u8 sendData[1024];
	PacketWriter packet(sendData, sizeof(sendData));

	packet.Write<u8>(0x20); // excludedFieldBits
	packet.Write<i32>(0); // actionID
	packet.Write<LocalActorID>(GetLocalActorID(clientID, mainActorUID));
	packet.Write<f32>(rotate);
	packet.Write<f32>(moveDirX);
	packet.Write<f32>(moveDirY);
	packet.Write<i32>(0); // errorType

	LOG("[client%03d] Server :: SA_ResultSpAction", clientID);
	SendPacketData(clientID, Sv::SA_ResultSpAction::NET_ID, packet.size, packet.data);
}

void Replication::SendPlayerAcceptCast(i32 clientID, const PlayerCastSkill& cast)
{
	LocalActorID localActorID = GetLocalActorID(clientID, cast.playerActorUID);
	ASSERT(localActorID != LocalActorID::INVALID);

	// SA_CastSkill
	{
		Sv::SA_CastSkill accept;
		accept.characterID = localActorID;
		accept.ret = 0;
		accept.skillIndex = cast.skillID;

		LOG("[client%03d] Server :: %s", clientID, PacketSerialize<Sv::SA_CastSkill>(&accept, sizeof(accept)));
		SendPacket(clientID, accept);
	}

	// SN_CastSkill
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<LocalActorID>(localActorID); // entityID
		packet.Write<i32>(0); // ret
		packet.Write<SkillID>(cast.skillID);
		packet.Write<u8>(0); // costLevel
		packet.Write<ActionStateID>(ActionStateID::INVALID);
		packet.Write<float3>(v2f(cast.p3nPos));

		packet.Write<u16>(0); // targetList_count

		packet.Write<u8>(1); // bSyncMyPosition
		packet.Write<float3>(cast.posStruct.pos);
		packet.Write<float3>(cast.posStruct.destPos);
		packet.Write<float2>(cast.posStruct.moveDir);
		packet.Write<float3>(cast.posStruct.rotateStruct);
		packet.Write<f32>(cast.posStruct.speed);
		packet.Write<i32>(cast.posStruct.clientTime);

		LOG("[client%03d] Server :: %s", clientID, PacketSerialize<Sv::SN_CastSkill>(packet.data, packet.size));
		SendPacketData(clientID, Sv::SN_CastSkill::NET_ID, packet.size, packet.data);
	}

	// SN_ExecuteSkill
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<LocalActorID>(localActorID); // entityID
		packet.Write<i32>(0); // ret
		packet.Write<SkillID>(cast.skillID);
		packet.Write<u8>(0); // costLevel

		ActionStateID actionState = ActionStateID::INVALID;
		switch(cast.skillID) {
			case (SkillID)180350002: actionState = (ActionStateID)103; break;
			case (SkillID)180350010: actionState = (ActionStateID)31; break;
			case (SkillID)180350030: actionState = (ActionStateID)30; break;
			case (SkillID)180030020: actionState = (ActionStateID)32; break;
			case (SkillID)180030030: actionState = (ActionStateID)33; break;
			case (SkillID)180030050: actionState = (ActionStateID)35; break;
		}
		packet.Write<ActionStateID>(actionState);
		packet.Write<float3>(v2f(cast.p3nPos));

		packet.Write<u16>(0); // targetList_count

		packet.Write<u8>(0); // bSyncMyPosition
		packet.Write<float3>(float3());
		packet.Write<float3>(float3());
		packet.Write<float2>(float2());
		packet.Write<float3>(float3());
		packet.Write<f32>(0);
		packet.Write<i32>(0);

		packet.Write<f32>(0); // fSkillChargeDamageMultiplier

		// graphMove
		packet.Write<u8>(0); // bApply
		packet.Write<float3>(cast.posStruct.pos); // startPos
		packet.Write<float3>(cast.posStruct.destPos); // endPos
		packet.Write<f32>(0); // durationTimeS
		packet.Write<f32>(0); // originDistance

		LOG("[client%03d] Server :: %s", clientID, PacketSerialize<Sv::SN_ExecuteSkill>(packet.data, packet.size));
		SendPacketData(clientID, Sv::SN_ExecuteSkill::NET_ID, packet.size, packet.data);
	}
}

void Replication::EventClientDisconnect(i32 clientID)
{
	playerState[clientID].cur = PlayerState::DISCONNECTED;
}

void Replication::PlayerRegisterMasterActor(i32 clientID, ActorUID masterActorUID, ClassType classType)
{
	LocalActorID laiLeader = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)classType);
	ASSERT(laiLeader >= LocalActorID::FIRST_SELF_MASTER && laiLeader < LocalActorID::LAST_SELF_MASTER);

	PlayerForceLocalActorID(clientID, masterActorUID, laiLeader);
}

void Replication::PlayerForceLocalActorID(i32 clientID, ActorUID actorUID, LocalActorID localActorID)
{
	DBG_ASSERT(actorUID != ActorUID::INVALID);

	auto& map = playerLocalInfo[clientID].localActorIDMap;
	ASSERT(map.find(actorUID) == map.end());
	map.emplace(actorUID, localActorID);
}

LocalActorID Replication::GetLocalActorID(i32 clientID, ActorUID actorUID) const
{
	const auto& map = playerLocalInfo[clientID].localActorIDMap;
	auto found = map.find(actorUID);
	if(found != map.end()) {
		return found->second;
	}
	return LocalActorID::INVALID;
}

ActorUID Replication::GetWorldActorUID(i32 clientID, LocalActorID localActorID) const
{
	// TODO: second map, for this reverse lookup
	const auto& map = playerLocalInfo[clientID].localActorIDMap;
	foreach(it, map) {
		if(it->second == localActorID) {
			return it->first;
		}
	}

	return ActorUID::INVALID;
}

void Replication::UpdatePlayersLocalState()
{
	for(int clientID = 0; clientID < Server::MAX_CLIENTS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;

		PlayerLocalInfo& localInfo = playerLocalInfo[clientID];
		auto& playerActorUIDSet = localInfo.actorUIDSet; // replicated actors UID set

		eastl::fixed_vector<ActorUID,2048,true> removedList;
		eastl::fixed_vector<ActorUID,2048,true> addedList;
		eastl::set_difference(playerActorUIDSet.begin(), playerActorUIDSet.end(), frameCur->actorUIDSet.begin(), frameCur->actorUIDSet.end(), eastl::back_inserter(removedList));
		eastl::set_difference(frameCur->actorUIDSet.begin(), frameCur->actorUIDSet.end(), playerActorUIDSet.begin(), playerActorUIDSet.end(), eastl::back_inserter(addedList));

		// send destroy entity for deleted actors
		foreach(setIt, removedList) {
			const ActorUID actorUID = *setIt;

#ifdef CONF_DEBUG // we don't actually need to verify the actor was in the previous frame, but do it in debug mode anyway
			const auto actorIt = framePrev->actorUIDSet.find(actorUID);
			ASSERT(actorIt != framePrev->actorUIDSet.end());
			auto type = framePrev->actorType.find(actorUID);
			ASSERT(type != framePrev->actorType.end());
			switch(type->second) {
				case ActorType::PlayerCharacter: {
					auto pm = framePrev->playerMap.find(actorUID);
					ASSERT(pm != framePrev->playerMap.end());
					ASSERT(pm->second->actorUID == actorUID);
				} break;

				case ActorType::Npc: {
					auto pm = framePrev->npcMap.find(actorUID);
					ASSERT(pm != framePrev->npcMap.end());
					ASSERT(pm->second->actorUID == actorUID);
				} break;

				default: {
					ASSERT_MSG(0, "case not handled");
				} break;
			}
#endif

			SendActorDestroy(clientID, actorUID);

			// Remove LocalActorID link
			DeleteLocalActorID(clientID, actorUID);
		}

		// send new spawns
		foreach(setIt, addedList) {
			const ActorUID actorUID = *setIt;

			// Create a LocalActorID link if none exists already
			// If one exists already, we have pre-allocated it (like with leader master)
			if(GetLocalActorID(clientID, actorUID) == LocalActorID::INVALID) {
				CreateLocalActorID(clientID, actorUID);
			}

			auto type = frameCur->actorType.find(actorUID);
			ASSERT(type != frameCur->actorType.end());

			switch(type->second) {
				case ActorType::PlayerCharacter: {
					const ActorPlayerCharacter* chara = frameCur->FindPlayerChara(actorUID);
					ASSERT(chara);
					const ActorPlayer* parent = frameCur->FindPlayer(chara->parentActorUID);
					ASSERT(parent);
					SendActorPlayerCharacterSpawn(clientID, *chara, *parent);
				} break;

				case ActorType::Npc: {
					const auto pm = frameCur->npcMap.find(actorUID);
					ASSERT(pm != frameCur->npcMap.end());
					ASSERT(pm->second->actorUID == actorUID);
					SendActorNpcSpawn(clientID, *pm->second);
				} break;

				default: {
					ASSERT_MSG(0, "case not handled");
				} break;
			}
		}

		playerActorUIDSet = frameCur->actorUIDSet;

		// TODO: remove, extra checks
#ifdef CONF_DEBUG
		auto& localActorIDMap = localInfo.localActorIDMap;
		foreach(it, playerActorUIDSet) {
			ASSERT(localActorIDMap.find(*it) != localActorIDMap.end());
		}
		foreach(it, localActorIDMap) {
			ASSERT(playerActorUIDSet.find(it->first) != playerActorUIDSet.end());
		}
		eastl::fixed_set<LocalActorID,2048> laiSet;
		foreach(it, localActorIDMap) {
			ASSERT(laiSet.find(it->second) == laiSet.end());
			laiSet.emplace(it->second);
		}
#endif
	}
}

void Replication::FrameDifference()
{
	// send position update
	struct UpdatePosition
	{
		i32 clientID;
		ActorUID actorUID;
		vec3 pos;
		vec3 dir;
		RotationHumanoid rotation;
		f32	speed;
	};

	struct UpdateRotation
	{
		i32 clientID;
		ActorUID actorUID;
		RotationHumanoid rot;
	};

	struct UpdateTag
	{
		i32 clientID;
		ActorUID mainUID;
		ActorUID subUID;
		vec3 pos;
	};

	eastl::fixed_vector<UpdatePosition,1024> listUpdatePosition;
	eastl::fixed_vector<UpdateRotation,1024> listUpdateRotation;
	eastl::fixed_vector<UpdateTag,1024> listUpdateTag;

	foreach_const(it, frameCur->playerList) {
		const ActorPlayer& cur = *it;
		const ActorPlayer* found = framePrev->FindPlayer(cur.actorUID);
		if(!found) continue;
		const ActorPlayer& prev = *found;

		// tagging
		if(cur.mainCharaID != prev.mainCharaID) {
			UpdateTag update;
			update.clientID = cur.clientID;
			update.mainUID = cur.characters[cur.mainCharaID];
			update.subUID = cur.characters[cur.mainCharaID ^ 1];
			const ActorPlayerCharacter* chara = frameCur->FindPlayerChara(cur.characters[cur.mainCharaID]);
			ASSERT(chara);
			update.pos = chara->pos;
			listUpdateTag.push_back(update);
		}
	}

	// TODO: don't update everything here if tagged out (such as position)
	// find if the position has changed since last frame
	foreach_const(it, frameCur->playerCharaList) {
		const ActorPlayerCharacter& cur = *it;
		auto found = framePrev->playerCharaMap.find(cur.actorUID);
		if(found == framePrev->playerCharaMap.end()) continue; // previous not found, can't diff
		const ActorPlayerCharacter& prev = *found->second;

		bool rotationUpdated = false;

		// position
		const f32 posEpsilon = 0.1f;
		const f32 dirEpsilon = 0.01f;
		const f32 speedEpsilon = 0.1f;
		if(fabs(cur.pos.x - prev.pos.x) > posEpsilon ||
		   fabs(cur.pos.y - prev.pos.y) > posEpsilon ||
		   fabs(cur.pos.z - prev.pos.z) > posEpsilon ||
		   fabs(cur.dir.x - prev.dir.x) > dirEpsilon ||
		   fabs(cur.dir.y - prev.dir.y) > dirEpsilon ||
		   fabs(cur.dir.z - prev.dir.z) > dirEpsilon ||
		   fabs(cur.speed - prev.speed) > speedEpsilon)
		{
			UpdatePosition update;
			update.clientID = cur.clientID;
			update.actorUID = cur.actorUID;
			update.pos = cur.pos;
			update.dir = cur.dir;
			update.rotation = cur.rotation;
			update.speed = cur.speed;
			listUpdatePosition.push_back(update);

			rotationUpdated = true;
		}

		const f32 rotEpsilon = 0.1f;
		if(!rotationUpdated &&
		   (fabs(cur.rotation.upperYaw - prev.rotation.upperYaw) > rotEpsilon ||
		   fabs(cur.rotation.upperPitch - prev.rotation.upperPitch) > rotEpsilon ||
		   fabs(cur.rotation.bodyYaw - prev.rotation.bodyYaw) > rotEpsilon))
		{
			UpdateRotation update;
			update.clientID = cur.clientID;
			update.actorUID = cur.actorUID;
			update.rot = cur.rotation;
			listUpdateRotation.push_back(update);
		}
	}

	// send updates
	foreach_const(up, listUpdatePosition) {
		Sv::SN_PlayerSyncMove sync;
		sync.destPos = v2f(up->pos);
		sync.moveDir = { up->dir.x, up->dir.y };
		sync.upperDir = { WorldYawToMxmYaw(up->rotation.upperYaw), WorldPitchToMxmPitch(up->rotation.upperPitch) };
		sync.nRotate = WorldYawToMxmYaw(up->rotation.bodyYaw);
		sync.nSpeed = up->speed;
		sync.flags = 0;
		sync.state = ActionStateID::NONE_BEHAVIORSTATE;

		for(int clientID = 0; clientID < Server::MAX_CLIENTS; clientID++) {
			if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
			if(clientID == up->clientID) continue; // ignore self

			sync.characterID = GetLocalActorID(clientID, up->actorUID);
			// LOG("[client%03d] Server :: SN_PlayerSyncMove :: actorUID=%u", clientID, (u32)up->actorUID);
			SendPacket(clientID, sync);
		}
	}

	foreach_const(up, listUpdateRotation) {
		Sv::SN_PlayerSyncTurn sync;
		sync.upperDir = { WorldYawToMxmYaw(up->rot.upperYaw), WorldPitchToMxmPitch(up->rot.upperPitch) };
		sync.nRotate = WorldYawToMxmYaw(up->rot.bodyYaw);

		for(int clientID = 0; clientID < Server::MAX_CLIENTS; clientID++) {
			if(playerState[clientID].cur < PlayerState::IN_GAME) continue;
			if(clientID == up->clientID) continue; // ignore self

			sync.characterID = GetLocalActorID(clientID, up->actorUID);
			LOG("[client%03d] Server :: SN_PlayerSyncTurn :: actorUID=%u", clientID, (u32)up->actorUID);
			SendPacket(clientID, sync);
		}
	}

	foreach_const(up, listUpdateTag) {
		Sv::SN_GamePlayerTag tag;
		tag.result = 128;
		tag.attackerID = LocalActorID::INVALID;

		for(int clientID = 0; clientID < Server::MAX_CLIENTS; clientID++) {
			if(playerState[clientID].cur < PlayerState::IN_GAME) continue;

			tag.mainID = GetLocalActorID(clientID, up->mainUID);
			tag.subID = GetLocalActorID(clientID, up->subUID);
			ASSERT(tag.mainID != LocalActorID::INVALID);
			ASSERT(tag.subID != LocalActorID::INVALID);

			LOG("[client%03d] Server :: %s", clientID, PacketSerialize<Sv::SN_GamePlayerTag>(&tag, sizeof(tag)));
			SendPacket(clientID, tag);

			if(clientID != up->clientID) { // ignore self
				Sv::SN_GameLeaveActor leave;
				leave.objectID = tag.subID;
				LOG("[client%03d] Server :: %s", clientID, PacketSerialize<Sv::SN_GameLeaveActor>(&leave, sizeof(leave)));
				SendPacket(clientID, leave);

				// Sv::SN_GameEnterActor
				{
					u8 sendData[1024];
					PacketWriter packet(sendData, sizeof(sendData));
					packet.Write<u8>(0x1); // excludedBits
					packet.Write<LocalActorID>(tag.mainID); // objectID
					packet.Write<float3>(v2f(up->pos)); //  p3nPos
					packet.Write<float3>(v2f(vec3(1, 0, 0))); //  p3nDir
					packet.Write<float2>(v2f(vec2(1, 0))); //  p2nMoveDir
					packet.Write<float2>(v2f(vec2(1, 0))); //  p2nMoveUpperDir
					packet.Write<float3>(v2f(vec3(0))); //  p3nMoveTargetPos
					packet.Write<u8>(0); //  isBattleState
					packet.Write<f32>(620.0f); //  baseMoveSpeed
					packet.Write<ActionStateID>(ActionStateID::TAG_IN_EXECUTE_BEHAVIORSTATE); //  actionState
					packet.Write<i32>(0); //  aiTargetID

					// statSnapshot
					typedef Sv::SN_GameEnterActor::ST_StatData Stat;
					eastl::array<Stat, 9> curStats = {
						Stat{ 0, 1270 },
						Stat{ 37, 13.2f },
						Stat{ 35, 1000 },
						Stat{ 2, 200 },
						Stat{ 6, 0 },
						Stat{ 10, 0 },
						Stat{ 64, 0 },
						Stat{ 7, 0 },
						Stat{ 14, 10 },
					};
					eastl::array<Stat, 9> maxStats = {
						Stat{ 0, 1270 },
						Stat{ 37, 120 },
						Stat{ 35, 1000 },
						Stat{ 2, 200 },
						Stat{ 6, 49.0077f },
						Stat{ 10, 150 },
						Stat{ 64, 150 },
						Stat{ 7, 76.5 },
						Stat{ 14, 100 },
					};
					eastl::array<Stat, 0> addPrivate;
					eastl::array<Stat, 0> mulPrivate;

					packet.WriteVec(curStats.data(), curStats.size());
					packet.WriteVec(maxStats.data(), maxStats.size());
					packet.WriteVec(addPrivate.data(), addPrivate.size());
					packet.WriteVec(mulPrivate.data(), mulPrivate.size());

					LOG("[client%03d] Server :: %s", clientID, PacketSerialize<Sv::SN_GameEnterActor>(packet.data, packet.size));
					SendPacketData(clientID, Sv::SN_GameEnterActor::NET_ID, packet.size, packet.data);
				}

				Sv::SN_PlayerSyncMove sync;
				sync.destPos = v2f(up->pos);
				sync.moveDir = { 1, 0 };
				sync.upperDir = { 0, 0 };
				sync.nRotate = WorldYawToMxmYaw(0);
				sync.nSpeed = 620;
				sync.flags = 0;
				sync.state = ActionStateID::TAG_IN_EXECUTE_BEHAVIORSTATE;
				sync.characterID = tag.mainID;
				// LOG("[client%03d] Server :: SN_PlayerSyncMove :: actorUID=%u", clientID, (u32)up->actorUID);
				SendPacket(clientID, sync);
			}
		}
	}
}

void Replication::SendActorPlayerCharacterSpawn(i32 clientID, const ActorPlayerCharacter& actor, const ActorPlayer& parent)
{
	DBG_ASSERT(actor.actorUID != ActorUID::INVALID);
	DBG_ASSERT(parent.actorUID != ActorUID::INVALID);

	const LocalActorID localActorID = GetLocalActorID(clientID, actor.actorUID);
	ASSERT(localActorID != LocalActorID::INVALID);

	LOG("[client%03d] Replication :: SendActorSpawn :: actorUID=%u localActorID=%u", clientID, (u32)actor.actorUID, (u32)localActorID);

	// this is the main actor
	if(actor.actorUID == parent.characters[0]) {
		// SN_GameCreateActor
		{
			u8 sendData[1024];
			PacketWriter packet(sendData, sizeof(sendData));

			packet.Write<LocalActorID>(localActorID); // objectID
			packet.Write<i32>(1); // nType
			packet.Write<CreatureIndex>((CreatureIndex)(100000000 + (i32)actor.classType)); // nIDX
			packet.Write<i32>(-1); // dwLocalID
			// TODO: localID?

			packet.Write(actor.pos); // p3nPos
			packet.Write(WorldYawToMxmYaw(actor.rotation.upperYaw));
			packet.Write(WorldPitchToMxmPitch(actor.rotation.upperPitch));
			packet.Write(WorldYawToMxmYaw(actor.rotation.bodyYaw));
			packet.Write<i32>(-1); // spawnType
			packet.Write<ActionStateID>(actor.actionState); // actionState
			packet.Write<i32>(0); // ownerID
			packet.Write<u8>(0); // bDirectionToNearPC
			packet.Write<i32>(-1); // AiWanderDistOverride
			packet.Write<i32>(-1); // tagID
			packet.Write<i32>(3); // faction
			packet.Write<ClassType>(actor.classType); // classType
			packet.Write<SkinIndex>(actor.skinIndex); // skinIndex
			packet.Write<i32>(0); // seed

			typedef Sv::SN_GameCreateActor::BaseStat::Stat Stat;

			// initStat ------------------------
			/*
			packet.Write<u16>(53); // maxStats_count

			packet.Write(Stat{ 0, 2400 });
			packet.Write(Stat{ 2, 200 });
			packet.Write(Stat{ 3, 0 }); //
			packet.Write(Stat{ 5, 5 });
			packet.Write(Stat{ 6, 124 });
			packet.Write(Stat{ 7, 93.75f });
			packet.Write(Stat{ 8, 0 }); //
			packet.Write(Stat{ 9, 3 });
			packet.Write(Stat{ 10, 150 });
			packet.Write(Stat{ 12, 0 }); //
			packet.Write(Stat{ 13, 100 });
			packet.Write(Stat{ 14, 100.5 });
			packet.Write(Stat{ 15, 100 });
			packet.Write(Stat{ 16, 1 });
			packet.Write(Stat{ 17, 0 }); //
			packet.Write(Stat{ 18, 100 });
			packet.Write(Stat{ 20, 0 }); //
			packet.Write(Stat{ 21, 0 }); //
			packet.Write(Stat{ 22, 2 });
			packet.Write(Stat{ 23, 9 });
			packet.Write(Stat{ 29, 20 });
			packet.Write(Stat{ 31, 14 });
			packet.Write(Stat{ 35, 1000 });
			packet.Write(Stat{ 36, 0 }); //
			packet.Write(Stat{ 37, 120 });
			packet.Write(Stat{ 39, 5 });
			packet.Write(Stat{ 40, 0 }); //
			packet.Write(Stat{ 41, 0 }); //
			packet.Write(Stat{ 42, 0.6f });
			packet.Write(Stat{ 44, 15 });
			packet.Write(Stat{ 52, 100 });
			packet.Write(Stat{ 54, 15 });
			packet.Write(Stat{ 55, 15 });
			packet.Write(Stat{ 56, 0 }); //
			packet.Write(Stat{ 57, 0 });
			packet.Write(Stat{ 50, 0 });
			packet.Write(Stat{ 51, 0 });
			packet.Write(Stat{ 63, 3 });
			packet.Write(Stat{ 64, 150 });



			packet.Write(Stat{ 27, 0 });
			packet.Write(Stat{ 47, 0 });
			packet.Write(Stat{ 49, 0 });
			packet.Write(Stat{ 48, 0 });

			packet.Write(Stat{ 46, 0 });
			packet.Write(Stat{ 45, 0 });
			packet.Write(Stat{ 26, 0 });
			packet.Write(Stat{ 25, 0 });

			packet.Write(Stat{ 60, 0 });
			packet.Write(Stat{ 61, 0 });
			packet.Write(Stat{ 62, 0 });

			packet.Write(Stat{ 53, 0 });
			packet.Write(Stat{ 58, 0 });
			packet.Write(Stat{ 65, 0 });


			packet.Write<u16>(4); // curStats_count
			packet.Write(Stat{ 0, 2400 });
			packet.Write(Stat{ 2, 200 });
			packet.Write(Stat{ 35, 1000 });
			packet.Write(Stat{ 37, 0 });*/

			packet.Write<u16>(26); // maxStats_count
			packet.Write(Stat{ 0, 2400 });
			packet.Write(Stat{ 2, 200 });
			packet.Write(Stat{ 5, 5 });
			packet.Write(Stat{ 6, 124 });
			packet.Write(Stat{ 7, 93.7846f });
			packet.Write(Stat{ 9, 3 });
			packet.Write(Stat{ 10, 150 });
			packet.Write(Stat{ 13, 100 });
			packet.Write(Stat{ 14, 101 });
			packet.Write(Stat{ 15, 100 });
			packet.Write(Stat{ 16, 1 });
			packet.Write(Stat{ 18, 100 });
			packet.Write(Stat{ 22, 2 });
			packet.Write(Stat{ 23, 9 });
			packet.Write(Stat{ 29, 20 });
			packet.Write(Stat{ 31, 14 });
			packet.Write(Stat{ 35, 1000 });
			packet.Write(Stat{ 37, 120 });
			packet.Write(Stat{ 39, 5 });
			packet.Write(Stat{ 42, 0.6f });
			packet.Write(Stat{ 44, 15 });
			packet.Write(Stat{ 52, 100 });
			packet.Write(Stat{ 54, 15 });
			packet.Write(Stat{ 55, 15 });
			packet.Write(Stat{ 63, 3 });
			packet.Write(Stat{ 64, 150 });

			packet.Write<u16>(4); // curStats_count
			packet.Write(Stat{ 0, 2400 });
			//packet.Write(Stat{ 37, 0 });
			packet.Write(Stat{ 37, 120 });
			packet.Write(Stat{ 35, 1000 });
			packet.Write(Stat{ 2, 200 });
			// ------------------------------------

			packet.Write<u8>(1); // isInSight
			packet.Write<u8>(0); // isDead
			packet.Write<i64>((i64)TimeDiffMs(TimeRelNow())); // serverTime

			packet.Write<u16>(0); // meshChangeActionHistory_count

			LOG("[client%03d] Server :: %s", clientID, PacketSerialize<Sv::SN_GameCreateActor>(packet.data, packet.size));
			SendPacketData(clientID, Sv::SN_GameCreateActor::NET_ID, packet.size, packet.data);
		}
	}
	// this is the sub actor
	else {
		const LocalActorID parentLocalActorID = GetLocalActorID(clientID, parent.characters[0]);
		ASSERT(parentLocalActorID != LocalActorID::INVALID);

		// SN_GameCreateSubActor
		{
			u8 sendData[1024];
			PacketWriter packet(sendData, sizeof(sendData));

			packet.Write<LocalActorID>(localActorID); // objectID
			packet.Write<LocalActorID>(parentLocalActorID); // mainEntityID
			packet.Write<i32>(1); // nType
			packet.Write<CreatureIndex>((CreatureIndex)(100000000 + (i32)actor.classType)); // nIDX
			packet.Write<i32>(-1); // dwLocalID

			packet.Write(actor.pos); // p3nPos
			packet.Write(WorldYawToMxmYaw(actor.rotation.upperYaw));
			packet.Write(WorldPitchToMxmPitch(actor.rotation.upperPitch));
			packet.Write(WorldYawToMxmYaw(actor.rotation.bodyYaw));
			packet.Write<i32>(-1); // spawnType
			packet.Write<ActionStateID>(actor.actionState); // actionState
			packet.Write<i32>(0); // ownerID
			packet.Write<i32>(-1); // tagID
			packet.Write<i32>(3); // faction
			packet.Write<ClassType>(actor.classType); // classType
			packet.Write<SkinIndex>(actor.skinIndex); // skinIndex
			packet.Write<i32>(0); // seed

			typedef Sv::SN_GameCreateActor::BaseStat::Stat Stat;

			// initStat ------------------------
			packet.Write<u16>(26); // maxStats_count
			packet.Write(Stat{ 0, 1764 });
			packet.Write(Stat{ 2, 200 });
			packet.Write(Stat{ 5, 5 });
			packet.Write(Stat{ 6, 192 });
			packet.Write(Stat{ 7, 85.05f });
			packet.Write(Stat{ 9, 3 });
			packet.Write(Stat{ 10, 150 });
			packet.Write(Stat{ 13, 100 });
			packet.Write(Stat{ 14, 104.5 });
			packet.Write(Stat{ 15, 100 });
			packet.Write(Stat{ 16, 1 });
			packet.Write(Stat{ 17, 100 });
			packet.Write(Stat{ 18, 100 });
			packet.Write(Stat{ 22, 2 });
			packet.Write(Stat{ 23, 9 });
			packet.Write(Stat{ 29, 20 });
			packet.Write(Stat{ 31, 14 });
			packet.Write(Stat{ 37, 120 });
			packet.Write(Stat{ 41, 6 });
			packet.Write(Stat{ 42, 0.6f });
			packet.Write(Stat{ 46, 5 });
			packet.Write(Stat{ 52, 100 });
			packet.Write(Stat{ 54, 15 });
			packet.Write(Stat{ 55, 15 });
			packet.Write(Stat{ 63, 3 });
			packet.Write(Stat{ 64, 15 });

			packet.Write<u16>(4); // curStats_count
			packet.Write(Stat{ 0, 1764 });
			packet.Write(Stat{ 37, 0 });
			packet.Write(Stat{ 2, 200 });
			packet.Write(Stat{ 17, 100 });
			// ------------------------------------

			packet.Write<u16>(0); // meshChangeActionHistory_count

			LOG("[client%03d] Server :: %s", clientID, PacketSerialize<Sv::SN_GameCreateSubActor>(packet.data, packet.size));
			SendPacketData(clientID, Sv::SN_GameCreateSubActor::NET_ID, packet.size, packet.data);
		}
	}

	// SN_SpawnPosForMinimap
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<LocalActorID>(localActorID); // objectID
		packet.Write(actor.pos); // p3nPos

		LOG("[client%03d] Server :: SN_SpawnPosForMinimap :: actorUID=%d", clientID, (u32)actor.actorUID);
		SendPacketData(clientID, Sv::SN_SpawnPosForMinimap::NET_ID, packet.size, packet.data);
	}

	// SN_GamePlayerStock
	{
		u8 sendData[2048];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<LocalActorID>(localActorID); // playerID
		packet.WriteStringObj(parent.name.data()); // name
		packet.Write<ClassType>(actor.classType); // class_
#if 0
		packet.Write<i32>(320080005); // displayTitleIDX
		packet.Write<i32>(320080005); // statTitleIDX
#else // disable titles
		packet.Write<i32>(-1); // displayTitleIDX
		packet.Write<i32>(-1); // statTitleIDX
#endif
		packet.Write<u8>(0); // badgeType
		packet.Write<u8>(0); // badgeTierLevel
		packet.WriteStringObj(parent.guildTag.data()); // guildTag
		packet.Write<u8>(0); // vipLevel
		packet.Write<u8>(0); // staffType
		packet.Write<u8>(0); // isSubstituted

		LOG("[client%03d] Server :: %s", clientID, PacketSerialize<Sv::SN_GamePlayerStock>(packet.data, packet.size));
		SendPacketData(clientID, Sv::SN_GamePlayerStock::NET_ID, packet.size, packet.data);
	}

	// SN_GamePlayerEquipWeapon
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<LocalActorID>(localActorID); // characterID
		packet.Write<i32>(131135011); // weaponDocIndex
		packet.Write<i32>(0); // additionnalOverHeatGauge
		packet.Write<i32>(0); // additionnalOverHeatGaugeRatio

		LOG("[client%03d] Server :: %s", clientID, PacketSerialize<Sv::SN_GamePlayerEquipWeapon>(packet.data, packet.size));
		SendPacketData(clientID, Sv::SN_GamePlayerEquipWeapon::NET_ID, packet.size, packet.data);
	}

	/*
	if(stageType == StageType::CITY) {
		// SN_PlayerStateInTown
		{
			u8 sendData[1024];
			PacketWriter packet(sendData, sizeof(sendData));

			packet.Write<LocalActorID>(localActorID); // playerID
			packet.Write<u8>(0); // playerStateInTown
			packet.Write<u16>(0); // matchingGameModes_count

			LOG("[client%03d] Server :: SN_PlayerStateInTown :: state=%d", clientID, -1);
			SendPacketData(clientID, Sv::SN_PlayerStateInTown::NET_ID, packet.size, packet.data);
		}
	}
	*/

	/*
	// SN_StatusSnapshot
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<LocalActorID>(localActorID); // objectID
		packet.Write<u16>(1); // statusArray_count

		packet.Write<i32>(1230350000); // statusIndex
		packet.Write<u8>(1); // bEnabled
		packet.Write<i32>(0); // caster
		packet.Write<u8>(1); // overlapCount
		packet.Write<u8>(0); // customValue
		packet.Write<i32>(0); // durationTimeMs
		packet.Write<i32>(0); // remainTimeMs

		LOG("[client%03d] Server :: %s", clientID, PacketSerialize<Sv::SN_StatusSnapshot>(packet.data, packet.size));
		SendPacketData(clientID, Sv::SN_StatusSnapshot::NET_ID, packet.size, packet.data);
	}
	*/

	SendMasterSkillSlots(clientID, actor);
}

void Replication::SendActorNpcSpawn(i32 clientID, const ActorNpc& actor)
{
	DBG_ASSERT(actor.actorUID != ActorUID::INVALID);
	auto found = playerLocalInfo[clientID].localActorIDMap.find(actor.actorUID);
	ASSERT(found != playerLocalInfo[clientID].localActorIDMap.end());

	const LocalActorID localActorID = found->second;

	LOG("[client%03d] Replication :: SendActorNpcSpawn :: actorUID=%u localActorID=%u", clientID, (u32)actor.actorUID, (u32)localActorID);

	// SN_GameCreateActor
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<LocalActorID>(localActorID); // objectID
		packet.Write<i32>(actor.type); // nType
		packet.Write<CreatureIndex>(actor.docID); // nIDX
		packet.Write<i32>(actor.localID); // dwLocalID

		packet.Write(actor.pos); // p3nPos
		packet.Write(actor.dir); // p3nDir
		packet.Write<i32>(0); // spawnType
		packet.Write<ActionStateID>((ActionStateID)99); // actionState
		packet.Write<i32>(0); // ownerID
		packet.Write<u8>(0); // bDirectionToNearPC
		packet.Write<i32>(-1); // AiWanderDistOverride
		packet.Write<i32>(-1); // tagID
		packet.Write<i32>(actor.faction); // faction
		packet.Write<ClassType>(ClassType::NONE); // classType
		packet.Write<SkinIndex>(SkinIndex::DEFAULT); // skinIndex
		packet.Write<i32>(0); // seed

		typedef Sv::SN_GameCreateActor::BaseStat::Stat Stat;

		// initStat ------------------------
		/*packet.Write<u16>(11); // maxStats_count
		packet.Write(Stat{ 0, 24953 });
		packet.Write(Stat{ 6, 96 });
		packet.Write(Stat{ 7, 113.333 });
		packet.Write(Stat{ 8, 10 });
		packet.Write(Stat{ 9, 5 });
		packet.Write(Stat{ 10, 150 });
		packet.Write(Stat{ 13, 100 });
		packet.Write(Stat{ 14, 80 });
		packet.Write(Stat{ 15, 100 });
		packet.Write(Stat{ 52, 100 });
		packet.Write(Stat{ 64, 150 });
		packet.Write<u16>(1); // curStats_count
		packet.Write(Stat{ 0, 24953 });*/
		// ------------------------------------

		packet.Write<u16>(0); // maxStats_count
		packet.Write<u16>(0); // curStats_count

		packet.Write<u8>(1); // isInSight
		packet.Write<u8>(0); // isDead
		packet.Write<i64>((i64)TimeDiffMs(TimeRelNow())); // serverTime

		packet.Write<u16>(0); // meshChangeActionHistory_count

		LOG("[client%03d] Server :: %s", clientID, PacketSerialize<Sv::SN_GameCreateActor>(packet.data, packet.size));
		SendPacketData(clientID, Sv::SN_GameCreateActor::NET_ID, packet.size, packet.data);
	}

	// SN_SpawnPosForMinimap
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<LocalActorID>(localActorID); // objectID
		packet.Write(actor.pos); // p3nPos

		LOG("[client%03d] Server :: SN_SpawnPosForMinimap :: actorUID=%d", clientID, (u32)actor.actorUID);
		SendPacketData(clientID, Sv::SN_SpawnPosForMinimap::NET_ID, packet.size, packet.data);
	}
}

void Replication::SendActorDestroy(i32 clientID, ActorUID actorUID)
{
	auto found = playerLocalInfo[clientID].localActorIDMap.find(actorUID);
	ASSERT(found != playerLocalInfo[clientID].localActorIDMap.end());
	const LocalActorID localActorID = found->second;

	Sv::SN_DestroyEntity packet;
	packet.characterID = localActorID;
	LOG("[client%03d] Server :: SN_DestroyEntity :: actorUID=%u", clientID, (u32)actorUID);
	SendPacket(clientID, packet);
}

void Replication::SendJukeboxPlay(i32 clientID, SongID songID, const wchar* requesterNick, i32 playPosInSec)
{
	u8 sendData[256];
	PacketWriter packet(sendData, sizeof(sendData));

	packet.Write<i32>(0); // result
	packet.Write<SongID>(songID); // trackID
	packet.WriteStringObj(requesterNick); // nickname
	packet.Write<u16>(playPosInSec); // playPositionSec

	LOG("[client%03d] Server :: SN_JukeboxPlay :: songID=%d requester='%ls'", clientID, (i32)songID, requesterNick);
	SendPacketData(clientID, Sv::SN_JukeboxPlay::NET_ID, packet.size, packet.data);
}

void Replication::SendMasterSkillSlots(i32 clientID, const Replication::ActorPlayerCharacter& actor)
{
	DBG_ASSERT(actor.actorUID != ActorUID::INVALID);
	const LocalActorID localActorID = GetLocalActorID(clientID, actor.actorUID);
	ASSERT(localActorID != LocalActorID::INVALID);

	const GameXmlContent& content = GetGameXmlContent();

	// SN_PlayerSkillSlot
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<LocalActorID>(localActorID); // characterID

		auto masterIt = content.masterClassTypeMap.find(actor.classType);
		ASSERT(masterIt != content.masterClassTypeMap.end());
		GameXmlContent::Master& master = *masterIt->second;

		struct SkillStatus {
			u8 isUnlocked;
			u8 isActivated;
		};

		const SkillStatus skillStatusList[7] = {
			{ 1, 1 },
			{ 1, 1 },
			{ 0, 0 },
			{ 0, 0 },
			{ 1, 1 },
			{ 1, 1 },
			{ 1, 1 },
		};

		i32 skillStatusID = 0;

		packet.Write<u16>(master.skillIDs.size()); // slotList_count
		foreach(it, master.skillIDs) {
			packet.Write<SkillID>(*it); // skillIndex
			packet.Write<i32>(0); // coolTime
			packet.Write<u8>(1); // unlocked
			packet.Write<u16>(0); // propList_count
			packet.Write<u8>(skillStatusList[skillStatusID].isUnlocked); // isUnlocked
			packet.Write<u8>(skillStatusList[skillStatusID].isActivated); // isActivated

			skillStatusID++;
		}

		packet.Write<SkillID>(SkillID::INVALID); // stageSkillIndex1
		packet.Write<SkillID>(SkillID::INVALID); // stageSkillIndex2
		packet.Write<SkillID>(master.skillIDs[0]); // currentSkillSlot1
		packet.Write<SkillID>(master.skillIDs[1]); // currentSkillSlot2
		packet.Write<SkillID>(master.skillIDs.back()); // shirkSkillSlot

		LOG("[client%03d] Server :: %s", clientID, PacketSerialize<Sv::SN_PlayerSkillSlot>(packet.data, packet.size));
		SendPacketData(clientID, Sv::SN_PlayerSkillSlot::NET_ID, packet.size, packet.data);
	}
}

void Replication::SendInitialFrame(i32 clientID)
{
	// TODO: move to earlier
	// SN_InitIngameModeInfo
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<i32>(0); // transformationVotingPlayerCoolTimeByVotingFail
		packet.Write<i32>(0); // transformationVotingTeamCoolTimeByTransformationEnd
		packet.Write<i32>(0); // playerCoolTimeByTransformationEnd
		packet.Write<i32>(0); // currentTransformationVotingPlayerCoolTimeByVotingFail
		packet.Write<i32>(0); // currentTransformationVotingTeamCoolTimeByTransformationEnd
		packet.Write<i32>(0); // currentPlayerCoolTimeByTransformationEnd
		packet.Write<i32>(20); // chPropertyResetCoolTime
		packet.Write<u8>(0); // transformationPieceCount
		packet.Write<u16>(0); // titanDocIndexes_count
		packet.Write<u8>(0); // nextTitanIndex
		packet.Write<u16>(0); // listExceptionStat_count

		LOG("[client%03d] Server :: SN_InitIngameModeInfo :: ", clientID);
		SendPacketData(clientID, Sv::SN_InitIngameModeInfo::NET_ID, packet.size, packet.data);
	}

	// SN_ScanEnd
	LOG("[client%03d] Server :: SN_ScanEnd ::", clientID);
	SendPacketData(clientID, Sv::SN_ScanEnd::NET_ID, 0, nullptr);

	// SN_LoadClearedStages
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(0); // count
		LOG("[client%03d] Server :: %s", clientID, PacketSerialize<Sv::SN_LoadClearedStages>(packet.data, packet.size));
		SendPacketData(clientID, Sv::SN_NotifyPcDetailInfos::NET_ID, packet.size, packet.data);
	}

	/*
	Sv::SN_NotifyUserLifeInfo lifeInfo;
	lifeInfo.usn = 1;
	lifeInfo.lifeCount = 3;
	lifeInfo.maxLifeCount = 3;
	lifeInfo.remainLifeCount = 0;
	LOG("[client%03d] Server :: SN_NotifyUserLifeInfo ::", clientID);
	SendPacket(clientID, lifeInfo);
	*/
}

void Replication::CreateLocalActorID(i32 clientID, ActorUID actorUID)
{
	PlayerLocalInfo& localInfo = playerLocalInfo[clientID];
	auto& localActorIDMap = localInfo.localActorIDMap;
	localActorIDMap.emplace(actorUID, localInfo.nextPlayerLocalActorID);
	localInfo.nextPlayerLocalActorID = (LocalActorID)((u32)localInfo.nextPlayerLocalActorID + 1);
	// TODO: find first free LocalActorID

	// TODO: start at 5000 for NPCs? Does it even matter?
}

void Replication::DeleteLocalActorID(i32 clientID, ActorUID actorUID)
{
	auto& localActorIDMap = playerLocalInfo[clientID].localActorIDMap;
	localActorIDMap.erase(localActorIDMap.find(actorUID));
}
