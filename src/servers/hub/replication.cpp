#include "replication.h"

#include <common/protocol.h>
#include <common/packet_serialize.h>
#include <EASTL/algorithm.h>
#include <EASTL/fixed_hash_map.h>
#include <EAStdC/EAString.h>

#include "config.h"
#include "coordinator.h" // AccountData
#include <mxm/game_content.h>


void ReplicationHub::Frame::Clear()
{
	playerList.clear();
	npcList.clear();
	playerMap.clear();
	npcMap.clear();
	actorUIDSet.clear();
	actorType.clear();
	transformMap.clear();
	actionStateMap.clear();
}

void ReplicationHub::PlayerLocalInfo::Reset()
{
	localActorIDMap.clear();
	actorUIDSet.clear();
	nextPlayerLocalActorID = LocalActorID::FIRST_OTHER_PLAYER;
	nextNpcLocalActorID = LocalActorID::FIRST_NPC;
	nextMonsterLocalActorID = LocalActorID::INVALID;
	isFirstLoad = true;
}

void ReplicationHub::Init(Server* server_)
{
	server = server_;
	memset(&playerState, 0, sizeof(playerState));

	framePrev = &frames[0];
	frameCur = &frames[1];
}

void ReplicationHub::FrameEnd()
{
	ProfileFunction();

	UpdatePlayersLocalState();

	FrameDifference();

	// send SN_ScanEnd if requested
	for(int clientID = 0; clientID < Server::MAX_CLIENTS; clientID++) {
		if(playerState[clientID] != PlayerState::IN_GAME) continue;

		if(playerLocalInfo[clientID].isFirstLoad) {
			playerLocalInfo[clientID].isFirstLoad = false;

			SendInitialFrame(clientID);
		}
	}

	eastl::swap(frameCur, framePrev);
	frameCur->Clear(); // clear frame
}

void ReplicationHub::FramePushPlayerActor(const ActorPlayer& actor)
{
	ASSERT(frameCur->playerMap.find(actor.actorUID) == frameCur->playerMap.end());
	ASSERT(frameCur->actorUIDSet.find(actor.actorUID) == frameCur->actorUIDSet.end());

	frameCur->playerList.emplace_back(actor);
	frameCur->playerMap.emplace(actor.actorUID, --frameCur->playerList.end());
	frameCur->actorUIDSet.insert(actor.actorUID);
	frameCur->actorType.emplace(actor.actorUID, actor.Type());

	Frame::Transform tf;
	tf.pos = actor.pos;
	tf.dir = actor.dir;
	tf.eye = actor.eye;
	tf.rotate = actor.rotate;
	tf.speed = actor.speed;
	frameCur->transformMap.emplace(actor.actorUID, tf);

	Frame::ActionState at;
	at.actionState = actor.actionState;
	at.actionParam1 = actor.actionParam1;
	at.actionParam2 = actor.actionParam2;
	at.rotate = actor.rotate;
	at.upperRotate = actor.upperRotate;
	frameCur->actionStateMap.emplace(actor.actorUID, at);
}

void ReplicationHub::FramePushNpcActor(const ReplicationHub::ActorNpc& actor)
{
	ASSERT(frameCur->playerMap.find(actor.actorUID) == frameCur->playerMap.end());
	ASSERT(frameCur->actorUIDSet.find(actor.actorUID) == frameCur->actorUIDSet.end());

	frameCur->npcList.emplace_back(actor);
	frameCur->npcMap.emplace(actor.actorUID, --frameCur->npcList.end());
	frameCur->actorUIDSet.insert(actor.actorUID);
	frameCur->actorType.emplace(actor.actorUID, actor.Type());

	Frame::Transform tf;
	tf.pos = actor.pos;
	tf.dir = actor.dir;
	tf.eye = vec3(0, 0, 0);
	tf.rotate = 0;
	tf.speed = 0;
	frameCur->transformMap.emplace(actor.actorUID, tf);

	/*
	Frame::ActionState at;
	at.actionState = actor.actionState;
	at.actionParam1 = actor.actionParam1;
	at.actionParam2 = actor.actionParam2;
	at.rotate = actor.rotate;
	at.upperRotate = actor.upperRotate;
	frameCur->actionStateMap.emplace(actor.actorUID, at);
	*/
}

void ReplicationHub::FramePushJukebox(const ReplicationHub::ActorJukebox& actor)
{
	ASSERT(frameCur->actorUIDSet.find(actor.actorUID) == frameCur->actorUIDSet.end());

	frameCur->jukebox = actor;

	frameCur->actorUIDSet.insert(actor.actorUID);
	frameCur->actorType.emplace(actor.actorUID, actor.Type());
}

void ReplicationHub::EventPlayerConnect(i32 clientID)
{
	playerState[clientID] = PlayerState::CONNECTED;
	playerLocalInfo[clientID].Reset();
}

void ReplicationHub::SendLoadLobby(i32 clientID, StageIndex stageIndex)
{
	// SN_LoadCharacterStart
	SendPacketData<Sv::SN_LoadCharacterStart>(clientID, 0, nullptr);

	// SN_SetGameGvt
	{
		Sv::SN_SetGameGvt gameGvt;
		// Possible loss of information here? Not sure what the time format should be
		u32 time = (u32)TimeDiffMs(TimeRelNow());
		gameGvt.sendTime = time;
		gameGvt.virtualTime = time;
		SendPacket(clientID, gameGvt);
	}

	// SN_SummaryInfoAll
	{
		u8 sendData[128];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(0);

		SendPacket<Sv::SN_SummaryInfoAll>(clientID, packet);
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

		SendPacket<Sv::SN_AvailableSummaryRewardCountList>(clientID, packet);
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

		SendPacket<Sv::SN_SummaryInfoLatest>(clientID, packet);
	}

	// SN_AchieveInfo
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u8>(1); // packetNum
		packet.Write<i32>(800); // achievementScore
		packet.Write<u16>(0); // achList_count

		SendPacket<Sv::SN_AchieveInfo>(clientID, packet);
	}

	// SN_AchieveLatest
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(0); // achList_count

		SendPacket<Sv::SN_AchieveLatest>(clientID, packet);
	}

	// SN_CityMapInfo
	Sv::SN_CityMapInfo cityMapInfo;
	cityMapInfo.cityMapID = (StageIndex)Config().LobbyMap;
	SendPacket(clientID, cityMapInfo);

	// SQ_CityLobbyJoinCity
	SendPacketData<Sv::SQ_CityLobbyJoinCity>(clientID, 0, nullptr);

	// SN_SetGameGvt
	{
		Sv::SN_SetGameGvt gameGvt;
		gameGvt.sendTime = 0;
		gameGvt.virtualTime = 0;
		SendPacket(clientID, gameGvt);
	}
}

void ReplicationHub::SetPlayerAsInGame(i32 clientID)
{
	playerState[clientID] = PlayerState::IN_GAME;
}

void ReplicationHub::SendCharacterInfo(i32 clientID, ActorUID actorUID, CreatureIndex docID, ClassType classType, i32 health, i32 healthMax)
{
	ASSERT(clientID >= 0 && clientID < Server::MAX_CLIENTS);

	if(playerState[clientID] != PlayerState::IN_GAME) {
		LOG("WARNING(EventPlayerRequestCharacterInfo): player not in game (clientID=%d, state=%d)", clientID, (i32)playerState[clientID]);
		return;
	}

	// SA_GetCharacterInfo
	Sv::SA_GetCharacterInfo info;
	info.characterID = GetLocalActorID(clientID, actorUID);
	info.docIndex = docID;
	info.classType = classType;
	info.hp = health;
	info.maxHp = healthMax;
	SendPacket(clientID, info);
}

void ReplicationHub::SendPlayerSetLeaderMaster(i32 clientID, ActorUID masterActorUID, ClassType classType, SkinIndex skinIndex)
{
	LocalActorID laiLeader = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)classType);
	ASSERT(laiLeader >= LocalActorID::FIRST_SELF_MASTER && laiLeader < LocalActorID::LAST_SELF_MASTER);

	PlayerForceLocalActorID(clientID, masterActorUID, laiLeader);

	if(playerState[clientID] < PlayerState::IN_GAME) {
		// SN_LeaderCharacter
		Sv::SN_LeaderCharacter leader;
		leader.leaderID = laiLeader;
		leader.skinIndex = skinIndex;
		SendPacket(clientID, leader);
	}
	else {
		// NOTE: only seems to close the master window
		// SA_LeaderCharacter
		Sv::SA_SetLeader leader;
		leader.result = 0;
		leader.leaderID = laiLeader;
		leader.skinIndex = skinIndex;
		SendPacket(clientID, leader);
	}
}

void ReplicationHub::SendChatMessageToAll(const wchar* senderName, i32 chatType, const wchar* msg, i32 msgLen)
{
	// TODO: restrict message length

	u8 sendData[2048];
	PacketWriter packet(sendData, sizeof(sendData));

	packet.Write<i32>(chatType); // chatType
	packet.WriteStringObj(senderName);
	packet.Write<u8>(0); // senderStaffType
	packet.WriteStringObj(msg, msgLen);

	for(int clientID= 0; clientID < Server::MAX_CLIENTS; clientID++) {
		if(playerState[clientID] != PlayerState::IN_GAME) continue;

		SendPacket<Sv::SN_ChatChannelMessage>(clientID, packet);
	}
}

void ReplicationHub::SendChatMessageToClient(i32 toClientID, const wchar* senderName, i32 chatType, const wchar* msg, i32 msgLen)
{
	ASSERT(toClientID >= 0 && toClientID < Server::MAX_CLIENTS);
	if(playerState[toClientID] != PlayerState::IN_GAME) return;

	if(msgLen == -1) msgLen = EA::StdC::Strlen(msg);

	u8 sendData[2048];
	PacketWriter packet(sendData, sizeof(sendData));

	packet.Write<i32>(chatType); // chatType
	packet.WriteStringObj(senderName);
	packet.Write<u8>(0); // senderStaffType
	packet.WriteStringObj(msg, msgLen);

	SendPacket<Sv::SN_ChatChannelMessage>(toClientID, packet);
}

void ReplicationHub::SendChatWhisperConfirmToClient(i32 senderClientID, const wchar* destNick, const wchar* msg)
{
	u8 sendData[2048];
	PacketWriter packet(sendData, sizeof(sendData));

	packet.Write<i32>(0); // result
	packet.WriteStringObj(destNick);
	packet.WriteStringObj(msg);

	SendPacket<Sv::SA_WhisperSend>(senderClientID, packet);
}

void ReplicationHub::SendChatWhisperToClient(i32 destClientID, const wchar* senderName, const wchar* msg)
{
	u8 sendData[2048];
	PacketWriter packet(sendData, sizeof(sendData));

	packet.WriteStringObj(senderName); // senderNick
	packet.Write<u8>(0); // staffType
	packet.WriteStringObj(msg); // msg

	SendPacket<Sv::SN_WhisperReceive>(destClientID, packet);
}

void ReplicationHub::SendAccountDataLobby(i32 clientID, const AccountData& account)
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

		SendPacket<Sv::SN_RegionServicePolicy>(clientID, packet);
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

		SendPacket<Sv::SN_AllCharacterBaseData>(clientID, packet);
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
			chara.skinIndex = SkinIndex::DEFAULT;
			chara.weaponIndex = it->weaponIDs[0];
			chara.masterGearNo = 1;
			packet.Write(chara);
		}

		SendPacket<Sv::SN_ProfileCharacters>(clientID, packet);
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
		SendPacket<>(clientID, Sv::SN_ProfileWeaponspacket);
	}
	*/

	// SN_MyGuild
	{
		u8 sendData[256];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.WriteStringObj(L"Alpha");
		packet.Write<i64>(0);
		packet.Write<u8>(0);

		SendPacket<Sv::SN_MyGuild>(clientID, packet);
	}

	// SN_ProfileMasterGears
	{
		u8 sendData[128];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(0); // masterGears_count

		SendPacket<Sv::SN_ProfileMasterGears>(clientID, packet);
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

		SendPacket<Sv::SN_ProfileItems>(clientID, packet);
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

		SendPacket<Sv::SN_ProfileSkills>(clientID, packet);
	}

	// SN_ProfileTitles
	{
		u8 sendData[128];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(1); // titles_count
		packet.Write<i32>(320080004); // titles[0]

		SendPacket<Sv::SN_ProfileTitles>(clientID, packet);
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

		SendPacket<Sv::SN_ProfileCharacterSkinList>(clientID, packet);
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

		SendPacket<Sv::SN_AccountInfo>(clientID, packet);
	}

	// SN_AccountExtraInfo
	{
		u8 sendData[128];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(0); // userGradeList_count
		packet.Write<i32>(0); // activityPoint
		packet.Write<u8>(0); // activityRewaredState

		SendPacket<Sv::SN_AccountExtraInfo>(clientID, packet);
	}

	// SN_AccountEquipmentList
	{
		u8 sendData[128];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<i32>(-1); // supportKitDocIndex

		SendPacket<Sv::SN_AccountEquipmentList>(clientID, packet);
	}

	/*// SN_Unknown_62472
	{
		u8 sendData[32];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u8>(1);

		LOG("[client%03d] Server :: SN_Unknown_62472 :: ", clientID);
		SendPacket<>(clientID, 62472, packet.size, packet.data);
	}*/

	// SN_GuildChannelEnter
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.WriteStringObj(L"Alpha"); // guildName
		packet.WriteStringObj(account.nickname.data()); // nick
		packet.Write<u8>(0); // onlineStatus

		SendPacket<Sv::SN_GuildChannelEnter>(clientID, packet);
	}

	// SN_FriendList
	{
		u8 sendData[32];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(0); // friendList_count

		SendPacket<Sv::SN_FriendList>(clientID, packet);
	}

	// SN_PveComradeInfo
	{
		u8 sendData[32];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<i32>(5); // availableComradeCount
		packet.Write<i32>(5); // maxComradeCount

		SendPacket<Sv::SN_PveComradeInfo>(clientID, packet);
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

		SendPacket<Sv::SN_AchieveUpdate>(clientID, packet);
	}

	// SN_FriendRequestList
	{
		u8 sendData[128];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(0); // friendRequestList_count

		SendPacket<Sv::SN_FriendRequestList>(clientID, packet);
	}

	// SN_BlockList
	{
		u8 sendData[128];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(0); // blocks_count

		SendPacket<Sv::SN_BlockList>(clientID, packet);
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

		SendPacket<Sv::SN_MailUnreadNotice>(clientID, packet);
	}

	// SN_WarehouseItems
	{
		u8 sendData[128];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(0); // items_count

		SendPacket<Sv::SN_WarehouseItems>(clientID, packet);
	}

	// SN_MutualFriendList
	{
		u8 sendData[128];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(0); // candidates_count

		SendPacket<Sv::SN_MutualFriendList>(clientID, packet);
	}

	// SN_GuildMemberStatus
	{
		u8 sendData[128];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(0); // guildMemberStatusList_count

		SendPacket<Sv::SN_GuildMemberStatus>(clientID, packet);
	}

	// SN_Money
	Sv::SN_Money money;
	money.nMoney = 116472;
	money.nReason = 1;
	SendPacket(clientID, money);
}

void ReplicationHub::SendConnectToServer(i32 clientID, const AccountData& account, const u8 ip[4], u16 port)
{
	u8 sendData[1024];
	PacketWriter packet(sendData, sizeof(sendData));

	packet.Write<u16>(port);
	packet.WriteRaw(ip, 4);
	packet.Write<i32>(449); // gameID
	packet.Write<u32>(3490298546); // idcHash
	packet.WriteStringObj(account.nickname.data(), account.nickname.size());
	packet.Write<i32>(340); // instantKey

	SendPacket<Sv::SN_DoConnectGameServer>(clientID, packet);

	// NOTE: client will disconnect on reception
}

void ReplicationHub::SendGameReady(i32 clientID)
{
	Sv::SA_GameReady ready;
	ready.waitingTimeMs = 3000;
	ready.serverTimestamp = (i64)TimeDiffMs(TimeRelNow());
	ready.readyElapsedMs = 0;
	SendPacket(clientID, ready);

	Sv::SN_NotifyIngameSkillPoint notify;
	notify.userID = 1;
	notify.skillPoint = 1;
	SendPacket(clientID, notify);

	Sv::SN_NotifyTimestamp notifyTimestamp;
	notifyTimestamp.serverTimestamp = (i64)TimeDiffMs(TimeRelNow());
	notifyTimestamp.curCount = 4;
	notifyTimestamp.maxCount = 5;
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

void ReplicationHub::SendCalendar(i32 clientID)
{
	// SA_CalendarDetail
	{
		u8 sendData[4096];
		PacketWriter packet(sendData, sizeof(sendData));

		const u64 now = CurrentFiletimeTimestampUTC();
		const u64 before = now - (2*24*3600*10000000ull); // 2 days before
		const u64 after = now + (2*24*3600*10000000ull); // 2 days after

		packet.Write<u64>(now); // todayUTCDateTime

		const Sv::SA_CalendarDetail::Event events[] = {
			{
				4,
				80602001,
				before,
				after
			},
			{
				4,
				80602002,
				before,
				after
			},
			{
				4,
				80602003,
				before,
				after
			},
			{
				4,
				80602004,
				before,
				after
			},
			{
				4,
				80602006,
				before,
				after
			},
			{
				4,
				80602007,
				before,
				after
			},
			{
				4,
				80602005,
				before,
				after
			},
			{
				4,
				70600005,
				before,
				after
			},
			{
				4,
				70600006,
				before,
				after
			},
			{
				4,
				70600008,
				before,
				after
			},
			{
				4,
				70600009,
				before,
				after
			},
			{
				4,
				70600010,
				before,
				after
			},
			{
				4,
				70600012,
				before,
				after
			},
			{
				4,
				70600002,
				before,
				after
			},
			{
				6,
				980000714,
				before,
				after
			},
			{
				6,
				980000168,
				before,
				after
			},
			{
				6,
				980000147,
				before,
				after
			},
			{
				6,
				980000735,
				before,
				after
			},
			{
				6,
				980000756,
				before,
				after
			},
			{
				6,
				980000777,
				before,
				after
			},
			{
				6,
				980000798,
				before,
				after
			},
			{
				6,
				980000819,
				before,
				after
			},
			{
				6,
				980000840,
				before,
				after
			},
			{
				6,
				980000861,
				before,
				after
			},
			{
				4,
				80602001,
				before,
				after
			},
			{
				4,
				80602002,
				before,
				after
			},
			{
				4,
				80602003,
				before,
				after
			},
			{
				4,
				80602004,
				before,
				after
			},
			{
				4,
				80602006,
				before,
				after
			},
			{
				4,
				80602007,
				before,
				after
			},
			{
				4,
				80602005,
				before,
				after
			},
			{
				4,
				70600005,
				before,
				after
			},
			{
				4,
				70600006,
				before,
				after
			},
			{
				4,
				70600008,
				before,
				after
			},
			{
				4,
				70600009,
				before,
				after
			},
			{
				4,
				70600010,
				before,
				after
			},
			{
				4,
				70600012,
				before,
				after
			},
			{
				4,
				70600002,
				before,
				after
			},
			{
				6,
				980000714,
				before,
				after
			},
			{
				6,
				980000168,
				before,
				after
			},
			{
				6,
				980000147,
				before,
				after
			},
			{
				6,
				980000735,
				before,
				after
			},
			{
				6,
				980000756,
				before,
				after
			},
			{
				6,
				980000777,
				before,
				after
			},
			{
				6,
				980000798,
				before,
				after
			},
			{
				6,
				980000819,
				before,
				after
			},
			{
				6,
				980000840,
				before,
				after
			},
			{
				6,
				980000861,
				before,
				after
			},
			{
				4,
				80602001,
				before,
				after
			},
			{
				4,
				80602002,
				before,
				after
			},
			{
				4,
				80602003,
				before,
				after
			},
			{
				4,
				80602004,
				before,
				after
			},
			{
				4,
				80602006,
				before,
				after
			},
			{
				4,
				80602007,
				before,
				after
			},
			{
				4,
				80602005,
				before,
				after
			},
			{
				4,
				70600005,
				before,
				after
			},
			{
				4,
				70600006,
				before,
				after
			},
			{
				4,
				70600008,
				before,
				after
			},
			{
				4,
				70600009,
				before,
				after
			},
			{
				4,
				70600010,
				before,
				after
			},
			{
				4,
				70600012,
				before,
				after
			},
			{
				4,
				70600002,
				before,
				after
			},
			{
				6,
				980000714,
				before,
				after
			},
			{
				6,
				980000168,
				before,
				after
			},
			{
				6,
				980000147,
				before,
				after
			},
			{
				6,
				980000735,
				before,
				after
			},
			{
				6,
				980000756,
				before,
				after
			},
			{
				6,
				980000777,
				before,
				after
			},
			{
				6,
				980000798,
				before,
				after
			},
			{
				6,
				980000819,
				before,
				after
			},
			{
				6,
				980000840,
				before,
				after
			},
			{
				6,
				980000861,
				before,
				after
			}
		};

		packet.WriteVec(events, ARRAY_COUNT(events));

		SendPacket<Sv::SA_CalendarDetail>(clientID, packet);
	}
}

void ReplicationHub::SendAreaPopularity(i32 clientID, u32 areaID)
{
	// SA_AreaPopularity
	{
		Sv::SA_AreaPopularity packet;
		packet.errCode = 0;
		SendPacket(clientID, packet);
	}

	// SN_AreaPopularity
	{
		u8 sendData[256];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u32>(areaID);
		packet.WriteVec((Sv::SN_AreaPopularity*)nullptr, 0);

		SendPacket<Sv::SN_AreaPopularity>(clientID, packet);
	}
}

void ReplicationHub::EventClientDisconnect(i32 clientID)
{
	playerState[clientID] = PlayerState::DISCONNECTED;
}

void ReplicationHub::PlayerRegisterMasterActor(i32 clientID, ActorUID masterActorUID, ClassType classType)
{
	LocalActorID laiLeader = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)classType);
	ASSERT(laiLeader >= LocalActorID::FIRST_SELF_MASTER && laiLeader < LocalActorID::LAST_SELF_MASTER);

	PlayerForceLocalActorID(clientID, masterActorUID, laiLeader);
}

void ReplicationHub::PlayerForceLocalActorID(i32 clientID, ActorUID actorUID, LocalActorID localActorID)
{
	auto& map = playerLocalInfo[clientID].localActorIDMap;
	ASSERT(map.find(actorUID) == map.end());
	map.emplace(actorUID, localActorID);
}

LocalActorID ReplicationHub::GetLocalActorID(i32 clientID, ActorUID actorUID)
{
	const auto& map = playerLocalInfo[clientID].localActorIDMap;
	auto found = map.find(actorUID);
	if(found != map.end()) {
		return found->second;
	}
	return LocalActorID::INVALID;
}

ActorUID ReplicationHub::GetWorldActorUID(i32 clientID, LocalActorID localActorID)
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

void ReplicationHub::UpdatePlayersLocalState()
{
	for(int clientID = 0; clientID < Server::MAX_CLIENTS; clientID++) {
		if(playerState[clientID] != PlayerState::IN_GAME) continue;

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
				case ActorType::PLAYER: {
					auto pm = framePrev->playerMap.find(actorUID);
					ASSERT(pm != framePrev->playerMap.end());
					ASSERT(pm->second->actorUID == actorUID);
				} break;

				case ActorType::NPC: {
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
				case ActorType::PLAYER: {
					const auto pm = frameCur->playerMap.find(actorUID);
					ASSERT(pm != frameCur->playerMap.end());
					ASSERT(pm->second->actorUID == actorUID);
					SendActorPlayerSpawn(clientID, *pm->second);
				} break;

				case ActorType::NPC: {
					const auto pm = frameCur->npcMap.find(actorUID);
					ASSERT(pm != frameCur->npcMap.end());
					ASSERT(pm->second->actorUID == actorUID);
					SendActorNpcSpawn(clientID, *pm->second);
				} break;

				case ActorType::JUKEBOX: {
					ASSERT(actorUID == frameCur->jukebox.actorUID);
					SendJukeboxSpawn(clientID, frameCur->jukebox);
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

void ReplicationHub::FrameDifference()
{
	// send position update

	eastl::fixed_vector<eastl::pair<ActorUID,Frame::Transform>, 2048> tfToSendList;
	eastl::fixed_vector<eastl::pair<ActorUID,Frame::ActionState>, 2048> atToSendList;

	// find if the position has changed since last frame
	foreach(it, frameCur->actorUIDSet) {
		const ActorUID actorUID = *it;
		auto type = frameCur->actorType.find(actorUID);
		ASSERT(type != frameCur->actorType.end());

		switch(type->second) {
			case ActorType::PLAYER: {
				// was present in last frame
				if(framePrev->actorUIDSet.find(actorUID) != framePrev->actorUIDSet.end()) {
					// transform
					{
						auto pf = framePrev->transformMap.find(actorUID);
						ASSERT(pf != framePrev->transformMap.end());
						const Frame::Transform& prev = pf->second;

						auto cf = frameCur->transformMap.find(actorUID);
						ASSERT(cf != frameCur->transformMap.end());
						const Frame::Transform& cur = cf->second;

						if(!prev.HasNotChanged(cur)) {
							tfToSendList.emplace_back(actorUID, cur);
						}
					}
				}

				// action state
				{
					auto cf = frameCur->actionStateMap.find(actorUID);
					ASSERT(cf != frameCur->actionStateMap.end());
					const Frame::ActionState& cur = cf->second;

					if(cur.actionState != ActionStateID::INVALID) {
						atToSendList.emplace_back(actorUID, cur);
					}
				}
			} break;

			case ActorType::NPC: {
				// nothing for now, NPCs don't change
			} break;

			case ActorType::JUKEBOX: {
				// was present in last frame
				if(framePrev->actorUIDSet.find(actorUID) != framePrev->actorUIDSet.end()) {
					const ActorJukebox& prev = framePrev->jukebox;
					const ActorJukebox& cur = frameCur->jukebox;

					ASSERT(prev.actorUID == cur.actorUID); // actually the same jukebox

					if(cur.currentSong.songID != SongID::INVALID) {
						if(prev.currentSong.songID != cur.currentSong.songID || prev.playStartTime != cur.playStartTime) {

							for(int clientID= 0; clientID < Server::MAX_CLIENTS; clientID++) {
								if(playerState[clientID] != PlayerState::IN_GAME) continue;

								SendJukeboxPlay(clientID, cur.currentSong.songID, cur.currentSong.requesterNick.data(), cur.playPosition);
							}
						}
					}

					bool doSendTracks = false;
					if(prev.tracks.size() == cur.tracks.size()) {
						for(int t = 0; t < cur.tracks.size(); t++) {
							if(prev.tracks[t].songID != cur.tracks[t].songID ||
							   prev.tracks[t].requesterNick.compare(cur.tracks[t].requesterNick)) {
								doSendTracks = true;
								break;
							}
						}
					}
					else {
						doSendTracks = true;
					}

					if(doSendTracks) {
						for(int clientID= 0; clientID < Server::MAX_CLIENTS; clientID++) {
							if(playerState[clientID] != PlayerState::IN_GAME) continue;

							SendJukeboxQueue(clientID, cur.tracks.data(), cur.tracks.size());
						}
					}
				}
			} break;

			default: ASSERT_MSG(0, "case not handled"); break;
		}
	}

	// send updates
	foreach(it, tfToSendList) {
		const auto& e = *it;
		const Frame::Transform& tf = e.second;

		Sv::SN_GamePlayerSyncByInt sync;
		sync.p3nPos = v2f(tf.pos);
		sync.p3nDir = v2f(tf.dir);
		sync.p3nEye = v2f(tf.eye);
		sync.nRotate = tf.rotate;
		sync.nSpeed = tf.speed;
		sync.nState = -1;
		sync.nActionIDX = -1;

		for(int clientID = 0; clientID < Server::MAX_CLIENTS; clientID++) {
			if(playerState[clientID] != PlayerState::IN_GAME) continue;

			sync.characterID = GetLocalActorID(clientID, e.first);
			SendPacket(clientID, sync);
		}
	}

	foreach(it, atToSendList) {
		const auto& e = *it;
		const Frame::ActionState& at = e.second;

		Sv::SN_PlayerSyncActionStateOnly packet;
		memset(&packet, 0, sizeof(packet));
		packet.state = at.actionState;
		packet.param1 = at.actionParam1;
		packet.param2 = at.actionParam2;
		packet.rotate = at.rotate;
		packet.upperRotate = at.upperRotate;

		for(int clientID= 0; clientID < Server::MAX_CLIENTS; clientID++) {
			if(playerState[clientID] != PlayerState::IN_GAME) continue;

			packet.characterID = GetLocalActorID(clientID, e.first);
			SendPacket(clientID, packet);
		}
	}
}

void ReplicationHub::SendActorPlayerSpawn(i32 clientID, const ActorPlayer& actor)
{
	DBG_ASSERT(actor.actorUID != ActorUID::INVALID);
	const LocalActorID localActorID = GetLocalActorID(clientID, actor.actorUID);
	ASSERT(localActorID != LocalActorID::INVALID);

	// this is the main actor
	if(actor.parentActorUID == ActorUID::INVALID) {
		// SN_GameCreateActor
		{
			u8 sendData[1024];
			PacketWriter packet(sendData, sizeof(sendData));

			packet.Write<LocalActorID>(localActorID); // objectID
			packet.Write<i32>(1); // nType
			packet.Write<CreatureIndex>(actor.docID); // nIDX
			packet.Write<i32>(-1); // dwLocalID
			// TODO: localID?

			packet.Write(actor.pos); // p3nPos
			packet.Write(actor.dir); // p3nDir
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
			packet.Write(Stat{ 37, 1 });
			packet.Write(Stat{ 35, 1000 });
			packet.Write(Stat{ 2, 200 });
			// ------------------------------------

			packet.Write<u8>(1); // isInSight
			packet.Write<u8>(0); // isDead
			packet.Write<i64>((i64)TimeDiffMs(TimeRelNow())); // serverTime

			packet.Write<u16>(0); // meshChangeActionHistory_count

			SendPacket<Sv::SN_GameCreateActor>(clientID, packet);
		}
	}
	// this is the sub actor
	else {
		const LocalActorID parentLocalActorID = GetLocalActorID(clientID, actor.parentActorUID);
		ASSERT(parentLocalActorID != LocalActorID::INVALID);

		// SN_GameCreateSubActor
		{
			u8 sendData[1024];
			PacketWriter packet(sendData, sizeof(sendData));

			packet.Write<LocalActorID>(localActorID); // objectID
			packet.Write<LocalActorID>(parentLocalActorID); // mainEntityID
			packet.Write<i32>(1); // nType
			packet.Write<CreatureIndex>(actor.docID); // nIDX
			packet.Write<i32>(-1); // dwLocalID

			packet.Write(actor.pos); // p3nPos
			packet.Write(actor.dir); // p3nDir
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

			SendPacket<Sv::SN_GameCreateSubActor>(clientID, packet);
		}
	}

	// SN_SpawnPosForMinimap
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<LocalActorID>(localActorID); // objectID
		packet.Write(actor.pos); // p3nPos

		SendPacket<Sv::SN_SpawnPosForMinimap>(clientID, packet);
	}

	// SN_GamePlayerStock
	{
		u8 sendData[2048];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<LocalActorID>(localActorID); // playerID
		packet.WriteStringObj(actor.name.data()); // name
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
		packet.WriteStringObj(actor.guildTag.data()); // guildTag
		packet.Write<u8>(0); // vipLevel
		packet.Write<u8>(0); // staffType
		packet.Write<u8>(0); // isSubstituted

		SendPacket<Sv::SN_GamePlayerStock>(clientID, packet);
	}

	// SN_GamePlayerEquipWeapon
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<LocalActorID>(localActorID); // characterID
		packet.Write<i32>(131135011); // weaponDocIndex
		packet.Write<i32>(0); // additionnalOverHeatGauge
		packet.Write<i32>(0); // additionnalOverHeatGaugeRatio

		SendPacket<Sv::SN_GamePlayerEquipWeapon>(clientID, packet);
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
			SendPacket<>(clientID, Sv::SN_PlayerStateInTownpacket);
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
		SendPacket<>(clientID, Sv::SN_StatusSnapshotpacket);
	}
	*/
}

void ReplicationHub::SendActorNpcSpawn(i32 clientID, const ActorNpc& actor)
{
	DBG_ASSERT(actor.actorUID != ActorUID::INVALID);
	auto found = playerLocalInfo[clientID].localActorIDMap.find(actor.actorUID);
	ASSERT(found != playerLocalInfo[clientID].localActorIDMap.end());

	const LocalActorID localActorID = found->second;

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

		SendPacket<Sv::SN_GameCreateActor>(clientID, packet);
	}

	// SN_SpawnPosForMinimap
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<LocalActorID>(localActorID); // objectID
		packet.Write(actor.pos); // p3nPos

		SendPacket<Sv::SN_SpawnPosForMinimap>(clientID, packet);
	}
}

void ReplicationHub::SendJukeboxSpawn(i32 clientID, const ReplicationHub::ActorJukebox& actor)
{
	DBG_ASSERT(actor.actorUID != ActorUID::INVALID);
	auto found = playerLocalInfo[clientID].localActorIDMap.find(actor.actorUID);
	ASSERT(found != playerLocalInfo[clientID].localActorIDMap.end());

	const LocalActorID localActorID = found->second;

	// SN_GameCreateActor
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<LocalActorID>(localActorID); // objectID
		packet.Write<i32>(1); // nType
		packet.Write<CreatureIndex>(actor.docID); // nIDX
		packet.Write<i32>(actor.localID); // dwLocalID

		packet.Write(actor.pos); // p3nPos
		packet.Write(actor.dir); // p3nDir
		packet.Write<i32>(0); // spawnType
		packet.Write<ActionStateID>(ActionStateID::INVALID); // actionState
		packet.Write<i32>(0); // ownerID
		packet.Write<u8>(0); // bDirectionToNearPC
		packet.Write<i32>(-1); // AiWanderDistOverride
		packet.Write<i32>(-1); // tagID
		packet.Write<i32>(-1); // faction
		packet.Write<ClassType>(ClassType::NONE); // classType
		packet.Write<SkinIndex>(SkinIndex::DEFAULT); // skinIndex
		packet.Write<i32>(0); // seed

		packet.Write<u16>(0); // maxStats_count
		packet.Write<u16>(0); // curStats_count

		packet.Write<u8>(1); // isInSight
		packet.Write<u8>(0); // isDead
		packet.Write<i64>((i64)TimeDiffMs(TimeRelNow())); // serverTime

		packet.Write<u16>(0); // meshChangeActionHistory_count

		SendPacket<Sv::SN_GameCreateActor>(clientID, packet);
	}

	/*
	// SN_JukeboxHotTrackList
	{
		u8 sendData[256];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(0); // trackList_count

		LOG("[client%03d] Server :: SN_JukeboxHotTrackList ::", clientID);
		SendPacket<>(clientID, Sv::SN_JukeboxHotTrackListpacket);
	}
	*/

	SongID songID = actor.currentSong.songID;
	if(songID == SongID::INVALID) {
		songID = SongID::Default; // send lobby song by default
	}

	SendJukeboxPlay(clientID, songID, actor.currentSong.requesterNick.data(), actor.playPosition);
	SendJukeboxQueue(clientID, actor.tracks.data(), actor.tracks.size());
}

void ReplicationHub::SendActorDestroy(i32 clientID, ActorUID actorUID)
{
	auto found = playerLocalInfo[clientID].localActorIDMap.find(actorUID);
	ASSERT(found != playerLocalInfo[clientID].localActorIDMap.end());
	const LocalActorID localActorID = found->second;

	Sv::SN_DestroyEntity packet;
	packet.characterID = localActorID;
	SendPacket(clientID, packet);
}

void ReplicationHub::SendJukeboxPlay(i32 clientID, SongID songID, const wchar* requesterNick, i32 playPosInSec)
{
	u8 sendData[256];
	PacketWriter packet(sendData, sizeof(sendData));

	packet.Write<i32>(0); // result
	packet.Write<SongID>(songID); // trackID
	packet.WriteStringObj(requesterNick); // nickname
	packet.Write<u16>(playPosInSec); // playPositionSec

	SendPacket<Sv::SN_JukeboxPlay>(clientID, packet);
}

void ReplicationHub::SendJukeboxQueue(i32 clientID, const ActorJukebox::Track* tracks, const i32 trackCount)
{
	u8 sendData[2048];
	PacketWriter packet(sendData, sizeof(sendData));

	packet.Write<u16>(trackCount); // trackList_count
	for(int i = 0; i < trackCount; i++) {
		packet.Write<SongID>(tracks[i].songID);
		packet.WriteStringObj(tracks[i].requesterNick.data(), tracks[i].requesterNick.size());
	}

	SendPacket<Sv::SN_JukeboxEnqueuedList>(clientID, packet);
}

void ReplicationHub::SendMasterSkillSlots(i32 clientID, const ReplicationHub::ActorPlayer& actor)
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

		SendPacket<Sv::SN_PlayerSkillSlot>(clientID, packet);
	}
}

void ReplicationHub::SendInitialFrame(i32 clientID)
{
	// SN_ScanEnd
	SendPacketData<Sv::SN_ScanEnd>(clientID, 0, nullptr);

	// SN_TownHudStatistics
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u8>(0); // gameModeType
		packet.Write<u8>(0); // gameType
		packet.Write<u16>(3); // argList_count

		// arglist
		packet.Write<i32>(479);
		packet.Write<i32>(0);
		packet.Write<i32>(16);

		SendPacket<Sv::SN_TownHudStatistics>(clientID, packet);
	}
}

void ReplicationHub::CreateLocalActorID(i32 clientID, ActorUID actorUID)
{
	PlayerLocalInfo& localInfo = playerLocalInfo[clientID];
	auto& localActorIDMap = localInfo.localActorIDMap;
	localActorIDMap.emplace(actorUID, localInfo.nextPlayerLocalActorID);
	localInfo.nextPlayerLocalActorID = (LocalActorID)((u32)localInfo.nextPlayerLocalActorID + 1);
	// TODO: find first free LocalActorID

	// TODO: start at 5000 for NPCs? Does it even matter?
}

void ReplicationHub::DeleteLocalActorID(i32 clientID, ActorUID actorUID)
{
	auto& localActorIDMap = playerLocalInfo[clientID].localActorIDMap;
	localActorIDMap.erase(localActorIDMap.find(actorUID));
}

bool ReplicationHub::Frame::Transform::HasNotChanged(const Frame::Transform& other) const
{
	const f32 posEpsilon = 0.1f;
	if(fabs(pos.x - other.pos.x) > posEpsilon) return false;
	if(fabs(pos.y - other.pos.y) > posEpsilon) return false;
	if(fabs(pos.z - other.pos.z) > posEpsilon) return false;
	if(fabs(dir.x - other.dir.x) > posEpsilon) return false;
	if(fabs(dir.y - other.dir.y) > posEpsilon) return false;
	if(fabs(dir.z - other.dir.z) > posEpsilon) return false;
	if(fabs(eye.x - other.eye.x) > posEpsilon) return false;
	if(fabs(eye.y - other.eye.y) > posEpsilon) return false;
	if(fabs(eye.z - other.eye.z) > posEpsilon) return false;
	/*const f32 rotEpsilon = 0.01f;
	if(fabs(rotate - other.rotate) > rotEpsilon) return false;*/
	const f32 speedEpsilon = 0.1f;
	if(fabs(speed - other.speed) > speedEpsilon) return false;
	return true;
}

bool ReplicationHub::Frame::ActionState::HasNotChanged(const ReplicationHub::Frame::ActionState& other) const
{
	if(other.actionState == ActionStateID::INVALID) return true;
	if(actionState != other.actionState) return false;
	if(actionParam1 != other.actionParam1) return false;
	if(actionParam2 != other.actionParam2) return false;
	return true;
}
