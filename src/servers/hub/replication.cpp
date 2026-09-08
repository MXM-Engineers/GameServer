#include "replication.h"

#include <common/protocol.h>
#include <common/packet_serialize.h>
#include <EASTL/algorithm.h>
#include <EASTL/fixed_hash_map.h>
#include <EAStdC/EAString.h>

#include "config.h"
#include "coordinator.h" // AccountData
#include <mxm/game_content.h>


void HubReplication::Frame::Clear()
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

void HubReplication::PlayerLocalInfo::Reset()
{
	localActorIDMap.clear();
	actorUIDSet.clear();
	nextPlayerLocalActorID = LocalActorID::FIRST_OTHER_PLAYER;
	nextNpcLocalActorID = LocalActorID::FIRST_NPC;
	nextMonsterLocalActorID = LocalActorID::INVALID;
	isFirstLoad = true;
}

void HubReplication::Init(Server* server_)
{
	server = server_;
	memset(&playerState, 0, sizeof(playerState));

	framePrev = &frames[0];
	frameCur = &frames[1];
}

void HubReplication::FrameEnd()
{
	ProfileFunction();

	UpdatePlayersLocalState();

	FrameDifference();

	// send SN_ScanEnd if requested
	for(int clientID = 0; clientID < MAX_CLIENTS; clientID++) {
		if(playerState[clientID] != PlayerState::IN_GAME) continue;

		if(playerLocalInfo[clientID].isFirstLoad) {
			playerLocalInfo[clientID].isFirstLoad = false;

			SendInitialFrame(playerClientHd[clientID]);
		}
	}

	eastl::swap(frameCur, framePrev);
	frameCur->Clear(); // clear frame
}

void HubReplication::FramePushPlayerActor(const ActorPlayer& actor)
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

void HubReplication::FramePushNpcActor(const HubReplication::ActorNpc& actor)
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

void HubReplication::FramePushJukebox(const HubReplication::ActorJukebox& actor)
{
	ASSERT(frameCur->actorUIDSet.find(actor.actorUID) == frameCur->actorUIDSet.end());

	frameCur->jukebox = actor;

	frameCur->actorUIDSet.insert(actor.actorUID);
	frameCur->actorType.emplace(actor.actorUID, actor.Type());
}

void HubReplication::OnPlayerConnect(ClientHandle clientHd)
{
	const i32 clientID = plidMap->Get(clientHd);

	playerClientHd[clientID] = clientHd;
	playerState[clientID] = PlayerState::CONNECTED;
	playerLocalInfo[clientID].Reset();
}

void HubReplication::SendLoadLobby(ClientHandle clientHd, MapIndex stageIndex)
{
	// SN_LoadCharacterStart
	SendPacketData<Sv::SN_LoadCharacterStart>(clientHd, 0, nullptr);

	// SN_SetGameGvt
	{
		Sv::SN_SetGameGvt gameGvt;
		// Possible loss of information here? Not sure what the time format should be
		u32 time = (u32)TimeDiffMs(TimeRelNow());
		gameGvt.sendTime = time;
		gameGvt.virtualTime = time;
		SendPacket(clientHd, gameGvt);
	}

	// SN_SummaryInfoAll
	{
		PacketWriter<Sv::SN_SummaryInfoAll> packet;

		packet.Write<u16>(0);

		SendPacket(clientHd, packet);
	}

	// SN_AvailableSummaryRewardCountList
	{
		PacketWriter<Sv::SN_AvailableSummaryRewardCountList> packet;

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

		SendPacket(clientHd, packet);
	}


	// SN_SummaryInfoLatest
	{
		PacketWriter<Sv::SN_SummaryInfoLatest> packet;

		packet.Write<u16>(10);

		const Sv::SN_SummaryInfoLatest::Info infoList[10] = {
			{ 220002200, StageIndex(200002201), 3, 220002201, 0, (i64)0xa6605c00 },
			{ 220002200, StageIndex(200002201), 3, 220002204, 0, (i64)0xa6605c00 },
			{ 220002200, StageIndex(200002201), 3, 220002205, 0, (i64)0xa6605c00 },
			{ 220005100, StageIndex(200005101), 3, 220005101, 0, (i64)0x6a2bfc00 },
			{ 220005100, StageIndex(200005101), 3, 220005102, 0, (i64)0x6a2bfc00 },
			{ 220005100, StageIndex(200005101), 3, 220005105, 0, (i64)0x6a2bfc00 },
			{ 220003300, StageIndex(200003301), 3, 220003301, 0, (i64)0xe0c7fa00 },
			{ 220003300, StageIndex(200003301), 3, 220003305, 0, (i64)0xe0c7fa00 },
			{ 220004100, StageIndex(200004101), 3, 220004101, 0, (i64)0x9eea8400 },
			{ 220004100, StageIndex(200004101), 3, 220004105, 0, (i64)0x9eea8400 },
		};

		packet.WriteRaw(infoList, sizeof(infoList));

		SendPacket(clientHd, packet);
	}

	// SN_AchieveInfo
	{
		PacketWriter<Sv::SN_AchieveInfo> packet;

		packet.Write<u8>(1); // packetNum
		packet.Write<i32>(800); // achievementScore
		packet.Write<u16>(0); // achList_count

		SendPacket(clientHd, packet);
	}

	// SN_AchieveLatest
	{
		PacketWriter<Sv::SN_AchieveLatest> packet;

		packet.Write<u16>(0); // achList_count

		SendPacket(clientHd, packet);
	}

	// SN_CityMapInfo
	Sv::SN_CityMapInfo cityMapInfo;
	cityMapInfo.CityMapID = (MapIndex)Config().LobbyMap;
	SendPacket(clientHd, cityMapInfo);

	// SQ_CityLobbyJoinCity
	SendPacketData<Sv::SQ_CityLobbyJoinCity>(clientHd, 0, nullptr);

	// SN_SetGameGvt
	{
		Sv::SN_SetGameGvt gameGvt;
		gameGvt.sendTime = 0;
		gameGvt.virtualTime = 0;
		SendPacket(clientHd, gameGvt);
	}
}

void HubReplication::SetPlayerAsInGame(ClientHandle clientHd)
{
	const i32 clientID = plidMap->Get(clientHd);
	playerState[clientID] = PlayerState::IN_GAME;
}

void HubReplication::SendCharacterInfo(ClientHandle clientHd, ActorUID actorUID, CreatureIndex docID, ClassType classType, i32 health, i32 healthMax)
{
	const i32 clientID = plidMap->Get(clientHd);

	if(playerState[clientID] != PlayerState::IN_GAME) {
		LOG("WARNING(EventPlayerRequestCharacterInfo): player not in game (clientID=%d, state=%d)", clientID, (i32)playerState[clientID]);
		return;
	}

	// SA_GetCharacterInfo
	Sv::SA_GetCharacterInfo info;
	info.characterID = GetLocalActorID(clientHd, actorUID);
	info.docIndex = docID;
	info.classType = classType;
	info.hp = health;
	info.maxHp = healthMax;
	SendPacket(clientHd, info);
}

void HubReplication::SendPlayerSetLeaderMaster(ClientHandle clientHd, ActorUID masterActorUID, ClassType classType, SkinIndex skinIndex)
{
	LocalActorID laiLeader = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)classType);
	ASSERT(laiLeader >= LocalActorID::FIRST_SELF_MASTER && laiLeader < LocalActorID::LAST_SELF_MASTER);

	PlayerForceLocalActorID(clientHd, masterActorUID, laiLeader);

	const i32 clientID = plidMap->Get(clientHd);
	if(playerState[clientID] < PlayerState::IN_GAME) {
		// SN_LeaderCharacter
		Sv::SN_LeaderCharacter leader;
		leader.leaderID = laiLeader;
		leader.skinIndex = skinIndex;
		SendPacket(clientHd, leader);
	}
	else {
		// NOTE: only seems to close the master window
		// SA_LeaderCharacter
		Sv::SA_SetLeader leader;
		leader.result = 0;
		leader.leaderID = laiLeader;
		leader.skinIndex = skinIndex;
		SendPacket(clientHd, leader);
	}
}

void HubReplication::SendChatMessageToAll(const wchar* senderName, i32 chatType, const wchar* msg, i32 msgLen)
{
	// TODO: restrict message length

	PacketWriter<Sv::SN_ChatChannelMessage> packet;

	packet.Write<i32>(chatType); // chatType
	packet.WriteStringObj(senderName);
	packet.Write<u8>(0); // senderStaffType
	packet.WriteStringObj(msg, msgLen);

	for(int clientID= 0; clientID < MAX_CLIENTS; clientID++) {
		if(playerState[clientID] != PlayerState::IN_GAME) continue;

		SendPacket(playerClientHd[clientID], packet);
	}
}

void HubReplication::SendChatMessageToClient(ClientHandle toClientHd, const wchar* senderName, i32 chatType, const wchar* msg, i32 msgLen)
{
	const i32 clientID = plidMap->Get(toClientHd);
	if(playerState[clientID] != PlayerState::IN_GAME) return;

	if(msgLen == -1) msgLen = EA::StdC::Strlen(msg);

	PacketWriter<Sv::SN_ChatChannelMessage> packet;

	packet.Write<i32>(chatType); // chatType
	packet.WriteStringObj(senderName);
	packet.Write<u8>(0); // senderStaffType
	packet.WriteStringObj(msg, msgLen);

	SendPacket(toClientHd, packet);
}

void HubReplication::SendChatWhisperConfirmToClient(ClientHandle senderClientHd, const wchar* destNick, const wchar* msg, ErrorType retval)
{
	PacketWriter<Sv::SA_WhisperSend> packet;

	packet.Write<ErrorType>(retval);
	packet.WriteStringObj(destNick);
	packet.WriteStringObj(msg);

	SendPacket(senderClientHd, packet);
}

void HubReplication::SendChatWhisperToClient(ClientHandle destClientHd, const wchar* senderName, const wchar* msg)
{
	PacketWriter<Sv::SN_WhisperReceived> packet;

	packet.WriteStringObj(senderName); // senderNick
	packet.Write<u8>(0); // staffType
	packet.WriteStringObj(msg); // msg

	SendPacket(destClientHd, packet);
}

void HubReplication::SendAccountDataLobby(ClientHandle clientHd, const Account& account)
{
	// SN_RegionServicePolicy
	{
		PacketWriter<Sv::SN_RegionServicePolicy> packet;

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

		packet.Write<u16>((u16)Config().regionBanMaster.size());
		for(auto id : Config().regionBanMaster) packet.Write<i32>(id);

		packet.Write<u16>((u16)Config().regionNewMaster.size());
		for(auto id : Config().regionNewMaster) packet.Write<i32>(id);

		packet.Write<u16>((u16)Config().eventBanMaster.size());
		for(auto id : Config().eventBanMaster) packet.Write<i32>(id);

		packet.Write<i32>(0x50000); // checkPeriodSec
		packet.Write<i32>(0xA0000); // maxTalkCount
		packet.Write<i32>(0x780000); // blockPeriodSec

		packet.Write<u16>(0); // regionBanSkinList_count
		packet.Write<u16>(0); // pcCafeSkinList_count

		packet.Write<u8>(0); // useFatigueSystem

		SendPacket(clientHd, packet);
	}

	// SN_AllCharacterBaseData
	{
		PacketWriter<Sv::SN_AllCharacterBaseData,2048> packet;

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

		SendPacket(clientHd, packet);
	}

	const GameXmlContent& content = GetGameXmlContent();

	// SN_ProfileCharacters
	{
		PacketWriter<Sv::SN_ProfileCharacters,2048> packet;

		packet.Write<u16>(content.masters.size()); // charaList_count

		foreach(it, content.masters) {
			Sv::SN_ProfileCharacters::Character chara;
			chara.characterID = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)it->classType);
			chara.creatureIndex = it->ID;
			chara.skillSlot1 = it->skillIDs[0];
			chara.skillSlot2 = it->skillIDs[1];
			chara.classType = it->classType;
			chara.x = 0;
			chara.y = 0;
			chara.z = 0;
			chara.characterType = 1;
			chara.skinIndex = SkinIndex::DEFAULT;
			chara.weaponIndex = it->defaultWeaponIDs[0];
			chara.masterGearNo = 1;
			packet.Write(chara);
		}

		SendPacket(clientHd, packet);
	}

	// SN_ProfileWeapons
	{
		PacketWriter<Sv::SN_ProfileWeapons,4096> packet;

		u16 weaponCount = content.masters.size() * 3;
		packet.Write<u16>(weaponCount); // weaponList_count

		foreach_const(it, content.masters) {
			const GameXmlContent::Master& master = *it;

			Sv::SN_ProfileWeapons::Weapon weapon;
			weapon.characterID = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)master.classType);

			for(int wi = 0; wi < 3; wi++) {
				weapon.weaponType = wi+1;
				// FIXME: because we load weapons from WEAPON.xml instead of CREATURE_CHARACTER.xml
				// we don't get the default weapons. This can be fixed later when we have a real unlock system.
				// - LordSk (10/10/2021)
				weapon.weaponIndex = master.weaponIDs[1 + wi*4];
				weapon.grade = 0;
				weapon.isUnlocked = wi == 0;
				weapon.isActivated = wi == 0;
				packet.Write(weapon);
			}
		}

		SendPacket(clientHd, packet);
	}

	// SN_MyGuild
	{
		PacketWriter<Sv::SN_MyGuild> packet;

		packet.WriteStringObj(L"Alpha");
		packet.Write<i64>(0);
		packet.Write<u8>(0);

		SendPacket(clientHd, packet);
	}

	// SN_ProfileMasterGears
	{
		PacketWriter<Sv::SN_ProfileMasterGears> packet;

		packet.Write<u16>(0); // masterGears_count

		SendPacket(clientHd, packet);
	}

	// SN_ProfileItems
	{
		PacketWriter<Sv::SN_ProfileItems> packet;

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

		SendPacket(clientHd, packet);
	}

	// SN_ProfileSkills
	{
		PacketWriter<Sv::SN_ProfileSkills,8192> packet;

		packet.Write<u8>(1); // packetNum

		i32 skillCount = 0;
		foreach_const(it, content.masters) {
			foreach_const(skill, it->skillIDs) {
				skillCount++;
			}
		}
		packet.Write<u16>(skillCount); // skills_count

		foreach_const(it, content.masters) {
			const LocalActorID characterID = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)it->classType);
			for(int si = 0; si < (int)it->skillIDs.size(); si++) {
				packet.Write(characterID);
				packet.Write(it->skillIDs[si]);
				const u8 unlocked = (si < (int)it->skillUnlocked.size()) ? it->skillUnlocked[si] : (u8)1;
				packet.Write<u8>(unlocked);
				packet.Write<u8>(1);
				packet.Write<u16>(0);
			}
		}

		SendPacket(clientHd, packet);
	}

	// SN_ProfileTitles
	{
		PacketWriter<Sv::SN_ProfileTitles> packet;

		packet.Write<u16>(1); // titles_count
		packet.Write<i32>(320080004); // titles[0]

		SendPacket(clientHd, packet);
	}

	// SN_ProfileCharacterSkinList
	{
		PacketWriter<Sv::SN_ProfileCharacterSkinList,4096> packet;

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

		SendPacket(clientHd, packet);
	}

	// SN_AccountInfo
	{
		PacketWriter<Sv::SN_AccountInfo> packet;

		packet.WriteStringObj(account.nickname.data()); // nick
		packet.Write<i32>(4); // inventoryLineCountTab0
		packet.Write<i32>(4); // inventoryLineCountTab1
		packet.Write<i32>(4); // inventoryLineCountTab2
#if 0
		packet.Write<i32>(320080005);
		packet.Write<i32>(320080005);
#else
		packet.Write<i32>(-1);
		packet.Write<i32>(-1);
#endif
		packet.Write<i32>(1); // warehouseLineCount
		packet.Write<i32>(-1); // tutorialState
		packet.Write<i32>(3600); // masterGearDurability
		packet.Write<u8>(0); // badgeType

		SendPacket(clientHd, packet);
	}

	// SN_AccountExtraInfo
	{
		PacketWriter<Sv::SN_AccountExtraInfo> packet;

		// membership
		Sv::SN_AccountExtraInfo::UserGrade grade = {
			5,
			1,
			CurrentFiletimeTimestampUTC() + (1*24*3600*10000000ull),
			0,
			0,
			0
		};

		packet.WriteVec(&grade, 1);
		packet.Write<i32>(0); // activityPoint
		packet.Write<u8>(0); // activityRewaredState

		SendPacket(clientHd, packet);
	}

	// SN_AccountEquipmentList
	{
		PacketWriter<Sv::SN_AccountEquipmentList> packet;

		packet.Write<i32>(-1);

		SendPacket(clientHd, packet);
	}

	// SN_Unknown_62472
	{
		PacketWriter<Sv::SN_Unknown_62472> packet;

		packet.Write<u8>(1);

		SendPacket(clientHd, packet);
	}

	// SN_GuildChannelEnter
	{
		PacketWriter<Sv::SN_GuildChannelEnter> packet;

		packet.WriteStringObj(L"Alpha"); // guildName
		packet.WriteStringObj(account.nickname.data()); // nick
		packet.Write<u8>(0); // onlineStatus

		SendPacket(clientHd, packet);
	}

	// SN_FriendList
	{
		PacketWriter<Sv::SN_FriendList> packet;

		packet.Write<u16>(0); // friendList_count

		SendPacket(clientHd, packet);
	}

	// SN_PveComradeInfo
	{
		PacketWriter<Sv::SN_PveComradeInfo> packet;

		packet.Write<i32>(5); // availableComradeCount
		packet.Write<i32>(5); // maxComradeCount

		SendPacket(clientHd, packet);
	}

	// SN_AchieveUpdate
	{
		PacketWriter<Sv::SN_AchieveUpdate> packet;

		packet.Write<i32>(800); // achievementScore
		packet.Write<i32>(300190005); // index
		packet.Write<i32>(1); // type
		packet.Write<u8>(0); // isCleared
		packet.Write<u16>(0); // achievedList_count
		packet.Write<i64>(6); // progressInt64
		packet.Write<i64>(6); // date

		SendPacket(clientHd, packet);
	}

	// SN_FriendRequestList
	{
		PacketWriter<Sv::SN_FriendRequestList> packet;

		packet.Write<u16>(0); // friendRequestList_count

		SendPacket(clientHd, packet);
	}

	// SN_BlockList
	{
		PacketWriter<Sv::SN_BlockList> packet;

		packet.Write<u16>(0); // blocks_count

		SendPacket(clientHd, packet);
	}

	// SN_MailUnreadNotice
	{
		PacketWriter<Sv::SN_MailUnreadNotice> packet;

		packet.Write<u16>(1); // unreadInboxMailCount
		packet.Write<u16>(0); // unreadArchivedMailCount
		packet.Write<u16>(4); // unreadShopMailCount
		packet.Write<u16>(3); // inboxMailCount
		packet.Write<u16>(3); // archivedMailCount
		packet.Write<u16>(16); // shopMailCount
		packet.Write<u16>(0); // newAttachmentsPending_count

		SendPacket(clientHd, packet);
	}

	// SN_WarehouseItems
	{
		PacketWriter<Sv::SN_WarehouseItems> packet;

		packet.Write<u16>(0); // items_count

		SendPacket(clientHd, packet);
	}

	// SN_MutualFriendList
	{
		PacketWriter<Sv::SN_MutualFriendList> packet;

		packet.Write<u16>(0); // candidates_count

		SendPacket(clientHd, packet);
	}

	// SN_GuildMemberStatus
	{
		PacketWriter<Sv::SN_GuildMemberStatus> packet;

		packet.Write<u16>(0); // guildMemberStatusList_count

		SendPacket(clientHd, packet);
	}

	// SN_Money
	Sv::SN_Money money;
	money.nMoney = 116472;
	money.nReason = 1;
	SendPacket(clientHd, money);

	// SN_UpdateEntrySystem
	{
		PacketWriter<Sv::SN_UpdateEntrySystem,2048> packet;

		packet.Write<u16>(7); // entrySystemListCount

		{
			packet.Write<u32>(210036011); // entrySystemIndex

			// areaList
			const Sv::SN_UpdateEntrySystem::Area areaList[] = {
				{ 2, AreaIndex(190009205) }
			};
			packet.WriteVec(areaList, ARRAY_COUNT(areaList));

			// stageList
			packet.Write<u16>(1);
			packet.Write<u8>(2); // areaKey
			packet.Write<i32>(200101330); // stageIndex
			const u8 gametypes[] = { 4, 6 };
			packet.WriteVec(gametypes, ARRAY_COUNT(gametypes));
		}
		{
			packet.Write<u32>(210036010); // entrySystemIndex

			// areaList
			const Sv::SN_UpdateEntrySystem::Area areaList[] = {
				{ 3, AreaIndex(190009204) }
			};
			packet.WriteVec(areaList, ARRAY_COUNT(areaList));

			// stageList
			packet.Write<u16>(1);
			packet.Write<u8>(3); // areaKey
			packet.Write<i32>(200101320); // stageIndex
			const u8 gametypes[] = { 4, 6 };
			packet.WriteVec(gametypes, ARRAY_COUNT(gametypes));
		}
		{
			packet.Write<u32>(210037002); // entrySystemIndex

			// areaList
			const Sv::SN_UpdateEntrySystem::Area areaList[] = {
				{ 4, AreaIndex(190004000) }
			};
			packet.WriteVec(areaList, ARRAY_COUNT(areaList));

			// stageList
			packet.Write<u16>(1);
			packet.Write<u8>(4); // areaKey
			packet.Write<i32>(200000100); // stageIndex
			const u8 gametypes[] = { 1 };
			packet.WriteVec(gametypes, ARRAY_COUNT(gametypes));
		}
		{
			packet.Write<u32>(210037000); // entrySystemIndex

			// areaList
			const Sv::SN_UpdateEntrySystem::Area areaList[] = {
				{ 5, AreaIndex(190000006) },
				{ 6, AreaIndex(190000007) },
				{ 7, AreaIndex(190000008) },
				{ 8, AreaIndex(190000009) },
				{ 9, AreaIndex(190000010) },
				{ 10, AreaIndex(190000011) },
			};
			packet.WriteVec(areaList, ARRAY_COUNT(areaList));

			// stageList
			packet.Write<u16>(12);
			{
				packet.Write<u8>(5); // areaKey
				packet.Write<i32>(200007101); // stageIndex
				const u8 gametypes[] = { 1 };
				packet.WriteVec(gametypes, ARRAY_COUNT(gametypes));
			}
			{
				packet.Write<u8>(5); // areaKey
				packet.Write<i32>(200007103); // stageIndex
				const u8 gametypes[] = { 1 };
				packet.WriteVec(gametypes, ARRAY_COUNT(gametypes));
			}
			{
				packet.Write<u8>(6); // areaKey
				packet.Write<i32>(200007201); // stageIndex
				const u8 gametypes[] = { 1 };
				packet.WriteVec(gametypes, ARRAY_COUNT(gametypes));
			}
			{
				packet.Write<u8>(6); // areaKey
				packet.Write<i32>(200007203); // stageIndex
				const u8 gametypes[] = { 1 };
				packet.WriteVec(gametypes, ARRAY_COUNT(gametypes));
			}
			{
				packet.Write<u8>(7); // areaKey
				packet.Write<i32>(200007301); // stageIndex
				const u8 gametypes[] = { 1 };
				packet.WriteVec(gametypes, ARRAY_COUNT(gametypes));
			}
			{
				packet.Write<u8>(7); // areaKey
				packet.Write<i32>(200007303); // stageIndex
				const u8 gametypes[] = { 1 };
				packet.WriteVec(gametypes, ARRAY_COUNT(gametypes));
			}
			{
				packet.Write<u8>(8); // areaKey
				packet.Write<i32>(200007401); // stageIndex
				const u8 gametypes[] = { 1 };
				packet.WriteVec(gametypes, ARRAY_COUNT(gametypes));
			}
			{
				packet.Write<u8>(8); // areaKey
				packet.Write<i32>(200007403); // stageIndex
				const u8 gametypes[] = { 1 };
				packet.WriteVec(gametypes, ARRAY_COUNT(gametypes));
			}
			{
				packet.Write<u8>(9); // areaKey
				packet.Write<i32>(200007501); // stageIndex
				const u8 gametypes[] = { 1 };
				packet.WriteVec(gametypes, ARRAY_COUNT(gametypes));
			}
			{
				packet.Write<u8>(9); // areaKey
				packet.Write<i32>(200007503); // stageIndex
				const u8 gametypes[] = { 1 };
				packet.WriteVec(gametypes, ARRAY_COUNT(gametypes));
			}
			{
				packet.Write<u8>(10); // areaKey
				packet.Write<i32>(200007601); // stageIndex
				const u8 gametypes[] = { 1 };
				packet.WriteVec(gametypes, ARRAY_COUNT(gametypes));
			}
			{
				packet.Write<u8>(10); // areaKey
				packet.Write<i32>(200007603); // stageIndex
				const u8 gametypes[] = { 1 };
				packet.WriteVec(gametypes, ARRAY_COUNT(gametypes));
			}
		}
		{
			packet.Write<u32>(210037006); // entrySystemIndex

			// areaList
			const Sv::SN_UpdateEntrySystem::Area areaList[] = {
				{ 11, AreaIndex(190001000) }
			};
			packet.WriteVec(areaList, ARRAY_COUNT(areaList));

			// stageList
			packet.Write<u16>(1);
			packet.Write<u8>(11); // areaKey
			packet.Write<i32>(200011109); // stageIndex
			const u8 gametypes[] = { 1 };
			packet.WriteVec(gametypes, ARRAY_COUNT(gametypes));
		}
		{
			packet.Write<u32>(210036812); // entrySystemIndex

			// areaList
			const Sv::SN_UpdateEntrySystem::Area areaList[] = {
				{ 1, AreaIndex(190002101) }
			};
			packet.WriteVec(areaList, ARRAY_COUNT(areaList));

			// stageList
			// stageList
			packet.Write<u16>(3);
			{
				packet.Write<u8>(1); // areaKey
				packet.Write<i32>(200020102); // stageIndex
				const u8 gametypes[] = { 4, 6 };
				packet.WriteVec(gametypes, ARRAY_COUNT(gametypes));
			}
			{
				packet.Write<u8>(1); // areaKey
				packet.Write<i32>(200101000); // stageIndex
				const u8 gametypes[] = { 4, 6, 5 };
				packet.WriteVec(gametypes, ARRAY_COUNT(gametypes));
			}
			{
				packet.Write<u8>(1); // areaKey
				packet.Write<i32>(200006112); // stageIndex
				const u8 gametypes[] = { 4 };
				packet.WriteVec(gametypes, ARRAY_COUNT(gametypes));
			}
		}
		{
			packet.Write<u32>(210037003); // entrySystemIndex

			// areaList
			const Sv::SN_UpdateEntrySystem::Area areaList[] = {
				{ 18, AreaIndex(190002200) }
			};
			packet.WriteVec(areaList, ARRAY_COUNT(areaList));

			// stageList
			packet.Write<u16>(1);
			packet.Write<u8>(18); // areaKey
			packet.Write<i32>(200006203); // stageIndex
			const u8 gametypes[] = { 7 };
			packet.WriteVec(gametypes, ARRAY_COUNT(gametypes));
		}


		SendPacket(clientHd, packet);
	}
}

void HubReplication::SendGameReady(ClientHandle clientHd)
{
	Sv::SA_GameReady ready;
	ready.waitingTimeMS = 3000;
	ready.serverTimestamp = (i64)TimeDiffMs(TimeRelNow());
	ready.readyElapsedMS = 0;
	SendPacket(clientHd, ready);

	Sv::SN_NotifyIngameSkillPoint notify;
	notify.userId = UserID(1);
	notify.skillPoint = 1;
	SendPacket(clientHd, notify);

	Sv::SN_NotifyTimestamp notifyTimestamp;
	notifyTimestamp.serverTimestamp = (i64)TimeDiffMs(TimeRelNow());
	notifyTimestamp.curCount = 4;
	notifyTimestamp.maxCount = 5;
	SendPacket(clientHd, notifyTimestamp);

	/*
	// Sv::SN_RunClientLevelEventSeq
	{
		Sv::SN_RunClientLevelEventSeq seq;
		seq.needCompleteTriggerAckID = -1;
		seq.rootEventID = 218;
		seq.caller = 0;
		seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
		LOG("[client%03d] Server :: SN_RunClientLevelEventSeq", clientID);
		SendPacket(clientHd, seq);
	}
	// Sv::SN_RunClientLevelEventSeq
	{
		Sv::SN_RunClientLevelEventSeq seq;
		seq.needCompleteTriggerAckID = -1;
		seq.rootEventID = 219;
		seq.caller = 0;
		seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
		LOG("[client%03d] Server :: SN_RunClientLevelEventSeq", clientID);
		SendPacket(clientHd, seq);
	}
	// Sv::SN_RunClientLevelEventSeq
	{
		Sv::SN_RunClientLevelEventSeq seq;
		seq.needCompleteTriggerAckID = -1;
		seq.rootEventID = 274;
		seq.caller = 0;
		seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
		LOG("[client%03d] Server :: SN_RunClientLevelEventSeq", clientID);
		SendPacket(clientHd, seq);
	}
	// Sv::SN_RunClientLevelEvent
	{
		Sv::SN_RunClientLevelEvent event;
		event.eventID = 48;
		event.caller = 0;
		event.serverTime = (i64)TimeDiffMs(TimeRelNow());
		LOG("[client%03d] Server :: SN_RunClientLevelEvent", clientID);
		SendPacket(clientHd, event);
	}
	// Sv::SN_RunClientLevelEventSeq
	{
		Sv::SN_RunClientLevelEventSeq seq;
		seq.needCompleteTriggerAckID = 1000000;
		seq.rootEventID = 1000001;
		seq.caller = 21035;
		seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
		LOG("[client%03d] Server :: SN_RunClientLevelEventSeq", clientID);
		SendPacket(clientHd, seq);
	}
	// Sv::SN_RunClientLevelEvent
	{
		Sv::SN_RunClientLevelEvent event;
		event.eventID = 150;
		event.caller = 21035;
		event.serverTime = (i64)TimeDiffMs(TimeRelNow());
		LOG("[client%03d] Server :: SN_RunClientLevelEvent", clientID);
		SendPacket(clientHd, event);
	}
	*/

	/*
	Sv::SN_NotifyIsInSafeZone safe;
	safe.userID = 1;
	safe.inSafeZone = 1;
	LOG("[client%03d] Server :: SN_NotifyIsInSafeZone", clientID);
	SendPacket(clientHd, safe);
	*/
}

void HubReplication::SendCalendar(ClientHandle clientHd)
{
	// SA_CalendarDetail
	{
		PacketWriter<Sv::SA_CalendarDetail,2048> packet;

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

		SendPacket(clientHd, packet);
	}
}

void HubReplication::SendAreaPopularity(ClientHandle clientHd, AreaIndex areaID)
{
	const GameXmlContent& content = GetGameXmlContent();
	bool known = false;
	for(auto& area : content.areaStages) {
		if(area.ID == areaID) { known = true; break; }
	}

	{
		Sv::SA_AreaPopularity packet;
		packet.errCode = known ? 0 : 1;
		SendPacket(clientHd, packet);
	}

	{
		PacketWriter<Sv::SN_AreaPopularity> packet;

		packet.Write(areaID);
		if(!known) {
			packet.Write<u16>(0);
			SendPacket(clientHd, packet);
			return;
		}
		i32 gameType = 1;
		for(auto& entry : content.entrySystems) {
			for(auto id : entry.areas) {
				if(id != areaID) continue;
				if(entry.entryType == "ENTRY_TYPE_SPORTS") gameType = 7;
			}
			for(auto id : entry.scheduleAreas) {
				if(id != areaID) continue;
				if(entry.entryType == "ENTRY_TYPE_SPORTS") gameType = 7;
			}
		}
		u16 count = 0;
		for(auto& area : content.areaStages) {
			if(area.ID != areaID) continue;
			for(auto stageID : area.stages) {
				(void)stageID;
				count++;
			}
		}
		packet.Write<u16>(count);
		for(auto& area : content.areaStages) {
			if(area.ID != areaID) continue;
			for(auto stageID : area.stages) {
				packet.Write(stageID);
				packet.Write<i32>(gameType);
				packet.Write<i32>(1);
			}
		}

		SendPacket(clientHd, packet);
	}
}

void HubReplication::SendPartyCreateSucess(ClientHandle clientHd, UserID ownerUserID, StageType stageType)
{
	PacketWriter<Sv::SA_PartyCreate> packet;

	//packet.Write<i32>(175); // retval (ERROR_TYPE_PARTY_CREATE_PENALTY_TIME) <- this one is silent
	packet.Write<i32>(0); // retval: success
	packet.Write(ownerUserID); // ownerUserID
	packet.Write(stageType); // stageType

	SendPacket(clientHd, packet);
}

void HubReplication::SendPartyEnqueue(ClientHandle clientHd, StageIndex stageIndex)
{
	Sv::SA_EnqueueGame packet;
	packet.retval = 0;
	SendPacket(clientHd, packet);

	Sv::SN_EnqueueMatchingQueue matching;
	matching.stageIndex = stageIndex;
	matching.currentMatchingTimeMs = 0;
	matching.avgMatchingTimeMs = 121634;
	matching.disableMatchExpansion = 0;
	matching.isMatchingExpanded = 0;
	SendPacket(clientHd, matching);
}

void HubReplication::SendMatchingPartyFound(ClientHandle clientHd, const In::MN_MatchingPartyFound& matchingParty, StageIndex stageIndex, const eastl::fixed_vector<UserID,16>& rowIDs)
{
	PacketWriter<Sv::SQ_MatchingPartyFound,512> packet;

	packet.Write(matchingParty.sortieUID); // sortieID
	packet.Write(stageIndex);
	packet.Write(matchingParty.gameType);
	packet.Write(GameDefinition::System); // gameDefinitionType
	packet.Write(StageRule::Unfair); // stageRule

	// allies
	i32 alliesCount = 0;
	for(int i = 0; i < matchingParty.playerCount; i++) {
		if(matchingParty.playerList[i].team == 0) alliesCount++;
	}
	packet.Write<u16>(alliesCount);

	for(int i = 0; i < matchingParty.playerCount; i++) {
		const auto& p = matchingParty.playerList[i];
		if(p.team == 0) {
			packet.Write(rowIDs[i]);
			packet.WriteStringObj(p.name.data, p.name.len); // nickname
			packet.Write<u8>(p.isBot); // isBot
			packet.Write<i32>(0); // tier
			packet.Write<i32>(0); // tierGroupRanking
			packet.Write<i32>(0); // tierSeriesFlag
			packet.Write<i32>(0); // pvpRate
		}
	}

	// enemies
	i32 enemiesCount = 0;
	for(int i = 0; i < matchingParty.playerCount; i++) {
		if(matchingParty.playerList[i].team == 1) enemiesCount++;
	}
	packet.Write<u16>(enemiesCount);

	for(int i = 0; i < matchingParty.playerCount; i++) {
		const auto& p = matchingParty.playerList[i];
		if(p.team == 1) {
			packet.Write(rowIDs[i]);
			packet.WriteStringObj(p.name.data, p.name.len); // nickname
			packet.Write<u8>(p.isBot); // isBot
			packet.Write<i32>(0); // tier
			packet.Write<i32>(0); // tierGroupRanking
			packet.Write<i32>(0); // tierSeriesFlag
			packet.Write<i32>(0); // pvpRate
		}
	}

	// spectators
	packet.Write<u16>(0);

	// NOTE: if timeToWaitInSec is 0 there is no popup to accept
	packet.Write<i32>(30); // timeToWaitInSec
	packet.Write<u8>(1); // elementMain
	packet.Write<u8>(0); // elementSub

	SendPacket(clientHd, packet);
}

void HubReplication::OnClientDisconnect(ClientHandle clientHd)
{
	const i32 clientID = plidMap->Get(clientHd);
	playerState[clientID] = PlayerState::DISCONNECTED;
	playerClientHd[clientID] = ClientHandle::INVALID;
}

void HubReplication::PlayerRegisterMasterActor(ClientHandle clientHd, ActorUID masterActorUID, ClassType classType)
{
	LocalActorID laiLeader = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)classType);
	ASSERT(laiLeader >= LocalActorID::FIRST_SELF_MASTER && laiLeader < LocalActorID::LAST_SELF_MASTER);

	PlayerForceLocalActorID(clientHd, masterActorUID, laiLeader);
}

void HubReplication::PlayerForceLocalActorID(ClientHandle clientHd, ActorUID actorUID, LocalActorID localActorID)
{
	const i32 clientID = plidMap->Get(clientHd);
	auto& map = playerLocalInfo[clientID].localActorIDMap;
	ASSERT(map.find(actorUID) == map.end());
	map.emplace(actorUID, localActorID);
}

LocalActorID HubReplication::GetLocalActorID(ClientHandle clientHd, ActorUID actorUID)
{
	const i32 clientID = plidMap->Get(clientHd);
	const auto& map = playerLocalInfo[clientID].localActorIDMap;
	auto found = map.find(actorUID);
	if(found != map.end()) {
		return found->second;
	}
	return LocalActorID::INVALID;
}

ActorUID HubReplication::GetWorldActorUID(ClientHandle clientHd, LocalActorID localActorID)
{
	const i32 clientID = plidMap->Get(clientHd);
	// TODO: second map, for this reverse lookup
	const auto& map = playerLocalInfo[clientID].localActorIDMap;
	foreach(it, map) {
		if(it->second == localActorID) {
			return it->first;
		}
	}

	return ActorUID::INVALID;
}

void HubReplication::UpdatePlayersLocalState()
{
	for(int clientID = 0; clientID < MAX_CLIENTS; clientID++) {
		if(playerState[clientID] != PlayerState::IN_GAME) continue;

		PlayerLocalInfo& localInfo = playerLocalInfo[clientID];
		auto& playerActorUIDSet = localInfo.actorUIDSet; // replicated actors UID set

		eastl::fixed_vector<ActorUID,2048,true> removedList;
		eastl::fixed_vector<ActorUID,2048,true> addedList;
		eastl::set_difference(playerActorUIDSet.begin(), playerActorUIDSet.end(), frameCur->actorUIDSet.begin(), frameCur->actorUIDSet.end(), eastl::back_inserter(removedList));
		eastl::set_difference(frameCur->actorUIDSet.begin(), frameCur->actorUIDSet.end(), playerActorUIDSet.begin(), playerActorUIDSet.end(), eastl::back_inserter(addedList));

		const ClientHandle clientHd = playerClientHd[clientID];

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

			SendActorDestroy(clientHd, actorUID);

			// Remove LocalActorID link
			DeleteLocalActorID(clientHd, actorUID);
		}

		// send new spawns
		foreach(setIt, addedList) {
			const ActorUID actorUID = *setIt;

			// Create a LocalActorID link if none exists already
			// If one exists already, we have pre-allocated it (like with leader master)
			if(GetLocalActorID(clientHd, actorUID) == LocalActorID::INVALID) {
				CreateLocalActorID(clientHd, actorUID);
			}

			auto type = frameCur->actorType.find(actorUID);
			ASSERT(type != frameCur->actorType.end());

			switch(type->second) {
				case ActorType::PLAYER: {
					const auto pm = frameCur->playerMap.find(actorUID);
					ASSERT(pm != frameCur->playerMap.end());
					ASSERT(pm->second->actorUID == actorUID);
					SendActorPlayerSpawn(clientHd, *pm->second);
				} break;

				case ActorType::NPC: {
					const auto pm = frameCur->npcMap.find(actorUID);
					ASSERT(pm != frameCur->npcMap.end());
					ASSERT(pm->second->actorUID == actorUID);
					SendActorNpcSpawn(clientHd, *pm->second);
				} break;

				case ActorType::JUKEBOX: {
					ASSERT(actorUID == frameCur->jukebox.actorUID);
					SendJukeboxSpawn(clientHd, frameCur->jukebox);
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

void HubReplication::FrameDifference()
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

							for(int clientID= 0; clientID < MAX_CLIENTS; clientID++) {
								if(playerState[clientID] != PlayerState::IN_GAME) continue;

								SendJukeboxPlay(playerClientHd[clientID], cur.currentSong.songID, cur.currentSong.requesterNick.data(), cur.playPosition);
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
						for(int clientID= 0; clientID < MAX_CLIENTS; clientID++) {
							if(playerState[clientID] != PlayerState::IN_GAME) continue;

							SendJukeboxQueue(playerClientHd[clientID], cur.tracks.data(), cur.tracks.size());
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

		for(int clientID = 0; clientID < MAX_CLIENTS; clientID++) {
			if(playerState[clientID] != PlayerState::IN_GAME) continue;

			const ClientHandle clientHd = playerClientHd[clientID];
			sync.characterID = GetLocalActorID(clientHd, e.first);
			SendPacket(clientHd, sync);
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

		for(int clientID= 0; clientID < MAX_CLIENTS; clientID++) {
			if(playerState[clientID] != PlayerState::IN_GAME) continue;

			const ClientHandle clientHd = playerClientHd[clientID];
			packet.characterID = GetLocalActorID(clientHd, e.first);
			SendPacket(clientHd, packet);
		}
	}
}

void HubReplication::SendActorPlayerSpawn(ClientHandle clientHd, const ActorPlayer& actor)
{
	DBG_ASSERT(actor.actorUID != ActorUID::INVALID);
	const LocalActorID localActorID = GetLocalActorID(clientHd, actor.actorUID);
	ASSERT(localActorID != LocalActorID::INVALID);

	// this is the main actor
	if(actor.parentActorUID == ActorUID::INVALID) {
		// SN_GameCreateActor
		{
			PacketWriter<Sv::SN_GameCreateActor> packet;

			packet.Write<LocalActorID>(localActorID); // objectID
			packet.Write<i32>(1);
			packet.Write<CreatureIndex>(actor.docID);
			packet.Write<i32>(-1);

			packet.Write(actor.pos);
			packet.Write(actor.dir);
			packet.Write<i32>(actor.spawnAnim);
			packet.Write<ActionStateID>(actor.actionState);
			packet.Write<i32>(actor.ownerID);
			packet.Write<u8>(actor.dirToNearPC);
			packet.Write<i32>(actor.wanderDist);
			packet.Write<i32>(actor.tagID);
			packet.Write<i32>(actor.faction);
			packet.Write<ClassType>(actor.classType);
			packet.Write<SkinIndex>(actor.skinIndex);
			packet.Write<i32>(actor.seed);


			WriteInitStat(packet, GetGameXmlContent().GetMaster(actor.classType).baseStats);




			packet.Write<u8>(1); // isInSight
			packet.Write<u8>(0); // isDead
			packet.Write<i64>((i64)TimeDiffMs(TimeRelNow())); // serverTime

			packet.Write<u16>(0); // meshChangeActionHistory_count

			SendPacket(clientHd, packet);
		}
	}
	// this is the sub actor
	else {
		const LocalActorID parentLocalActorID = GetLocalActorID(clientHd, actor.parentActorUID);
		ASSERT(parentLocalActorID != LocalActorID::INVALID);

		// SN_GameCreateSubActor
		{
			PacketWriter<Sv::SN_GameCreateSubActor> packet;

			packet.Write<LocalActorID>(localActorID); // objectID
			packet.Write<LocalActorID>(parentLocalActorID); // mainEntityID
			packet.Write<i32>(1);
			packet.Write<CreatureIndex>(actor.docID);
			packet.Write<i32>(-1);

			packet.Write(actor.pos);
			packet.Write(actor.dir);
			packet.Write<i32>(actor.spawnAnim);
			packet.Write<ActionStateID>(actor.actionState);
			packet.Write<i32>(actor.ownerID);
			packet.Write<i32>(actor.tagID);
			packet.Write<i32>(actor.faction);
			packet.Write<ClassType>(actor.classType);
			packet.Write<SkinIndex>(actor.skinIndex);
			packet.Write<i32>(actor.seed);


			WriteInitStat(packet, GetGameXmlContent().GetMaster(actor.classType).baseStats);




			packet.Write<u16>(0); // meshChangeActionHistory_count

			SendPacket(clientHd, packet);
		}
	}

	// SN_SpawnPosForMinimap
	{
		PacketWriter<Sv::SN_SpawnPosForMinimap> packet;

		packet.Write<LocalActorID>(localActorID); // objectID
		packet.Write(actor.pos); // p3nPos

		SendPacket(clientHd, packet);
	}

	// SN_GamePlayerStock
	{
		PacketWriter<Sv::SN_GamePlayerStock> packet;

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

		SendPacket(clientHd, packet);
	}

	// SN_GamePlayerEquipWeapon
	{
		PacketWriter<Sv::SN_GamePlayerEquipWeapon> packet;

		packet.Write<LocalActorID>(localActorID); // characterID
		packet.Write<WeaponIndex>(GetGameXmlContent().GetMaster(actor.classType).defaultWeaponIDs[0]);
		packet.Write<f32>(0.f);
		packet.Write<f32>(0.f);

		SendPacket(clientHd, packet);
	}

	/*
	if(stageType == StageType::CITY) {
		// SN_PlayerStateInTown
		{
			PacketWriter<> packet;

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
		PacketWriter<> packet;

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

void HubReplication::SendActorNpcSpawn(ClientHandle clientHd, const ActorNpc& actor)
{
	const i32 clientID = plidMap->Get(clientHd);

	DBG_ASSERT(actor.actorUID != ActorUID::INVALID);
	auto found = playerLocalInfo[clientID].localActorIDMap.find(actor.actorUID);
	ASSERT(found != playerLocalInfo[clientID].localActorIDMap.end());

	const LocalActorID localActorID = found->second;

	// SN_GameCreateActor
	{
		PacketWriter<Sv::SN_GameCreateActor> packet;

		packet.Write<LocalActorID>(localActorID); // objectID
		packet.Write<i32>(actor.type);
		packet.Write<CreatureIndex>(actor.docID);
		packet.Write<i32>(actor.localID);
		packet.Write(actor.pos);
		packet.Write(actor.dir);
		packet.Write<i32>(actor.spawnAnim);
		packet.Write<ActionStateID>(actor.actionState);
		packet.Write<i32>(actor.ownerID);
		packet.Write<u8>(actor.dirToNearPC);
		packet.Write<i32>(actor.wanderDist);
		packet.Write<i32>(actor.tagID);
		packet.Write<i32>(actor.faction);
		packet.Write<ClassType>(ClassType::NONE);
		packet.Write<SkinIndex>(SkinIndex::DEFAULT);
		packet.Write<i32>(actor.seed);


		packet.Write<u16>(0);
		packet.Write<u16>(0);




		packet.Write<u8>(1); // isInSight
		packet.Write<u8>(0); // isDead
		packet.Write<i64>((i64)TimeDiffMs(TimeRelNow())); // serverTime

		packet.Write<u16>(0); // meshChangeActionHistory_count

		SendPacket(clientHd, packet);
	}

	// SN_SpawnPosForMinimap
	{
		PacketWriter<Sv::SN_SpawnPosForMinimap> packet;

		packet.Write<LocalActorID>(localActorID); // objectID
		packet.Write(actor.pos); // p3nPos

		SendPacket(clientHd, packet);
	}
}

void HubReplication::SendJukeboxSpawn(ClientHandle clientHd, const HubReplication::ActorJukebox& actor)
{
	const i32 clientID = plidMap->Get(clientHd);

	DBG_ASSERT(actor.actorUID != ActorUID::INVALID);
	auto found = playerLocalInfo[clientID].localActorIDMap.find(actor.actorUID);
	ASSERT(found != playerLocalInfo[clientID].localActorIDMap.end());

	const LocalActorID localActorID = found->second;

	// SN_GameCreateActor
	{
		PacketWriter<Sv::SN_GameCreateActor> packet;

		packet.Write<LocalActorID>(localActorID); // objectID
		packet.Write<i32>(actor.type);
		packet.Write<CreatureIndex>(actor.docID);
		packet.Write<i32>(actor.localID);
		packet.Write(actor.pos);
		packet.Write(actor.dir);
		packet.Write<i32>(actor.spawnAnim);
		packet.Write<ActionStateID>(actor.actionState);
		packet.Write<i32>(actor.ownerID);
		packet.Write<u8>(actor.dirToNearPC);
		packet.Write<i32>(actor.wanderDist);
		packet.Write<i32>(actor.tagID);
		packet.Write<i32>(actor.faction);
		packet.Write<ClassType>(ClassType::NONE);
		packet.Write<SkinIndex>(SkinIndex::DEFAULT);
		packet.Write<i32>(actor.seed);


		packet.Write<u16>(0);
		packet.Write<u16>(0);




		packet.Write<u8>(1); // isInSight
		packet.Write<u8>(0); // isDead
		packet.Write<i64>((i64)TimeDiffMs(TimeRelNow())); // serverTime

		packet.Write<u16>(0); // meshChangeActionHistory_count

		SendPacket(clientHd, packet);
	}

	/*
	// SN_JukeboxHotTrackList
	{
		PacketWriter<> packet;

		packet.Write<u16>(0); // trackList_count

		LOG("[client%03d] Server :: SN_JukeboxHotTrackList ::", clientID);
		SendPacket<>(clientID, Sv::SN_JukeboxHotTrackListpacket);
	}
	*/

	SongID songID = actor.currentSong.songID;
	if(songID == SongID::INVALID) {
		songID = SongID::Default; // send lobby song by default
	}

	SendJukeboxPlay(clientHd, songID, actor.currentSong.requesterNick.data(), actor.playPosition);
	SendJukeboxQueue(clientHd, actor.tracks.data(), actor.tracks.size());
}

void HubReplication::SendActorDestroy(ClientHandle clientHd, ActorUID actorUID)
{
	const i32 clientID = plidMap->Get(clientHd);

	auto found = playerLocalInfo[clientID].localActorIDMap.find(actorUID);
	ASSERT(found != playerLocalInfo[clientID].localActorIDMap.end());
	const LocalActorID localActorID = found->second;

	Sv::SN_DestroyEntity packet;
	packet.objectID = localActorID;
	SendPacket(clientHd, packet);
}

void HubReplication::SendJukeboxPlay(ClientHandle clientHd, SongID songID, const wchar* requesterNick, i32 playPosInSec)
{
	PacketWriter<Sv::SN_JukeboxPlay> packet;

	packet.Write<i32>(0); // result
	packet.Write<SongID>(songID); // trackID
	packet.WriteStringObj(requesterNick); // nickname
	packet.Write<u16>(playPosInSec); // playPositionSec

	SendPacket(clientHd, packet);
}

void HubReplication::SendJukeboxQueue(ClientHandle clientHd, const ActorJukebox::Track* tracks, const i32 trackCount)
{
	PacketWriter<Sv::SN_JukeboxEnqueuedList> packet;

	packet.Write<u16>(trackCount); // trackList_count
	for(int i = 0; i < trackCount; i++) {
		packet.Write<SongID>(tracks[i].songID);
		packet.WriteStringObj(tracks[i].requesterNick.data(), tracks[i].requesterNick.size());
	}

	SendPacket(clientHd, packet);
}

void HubReplication::SendMasterSkillSlots(ClientHandle clientHd, const HubReplication::ActorPlayer& actor)
{
	DBG_ASSERT(actor.actorUID != ActorUID::INVALID);
	const LocalActorID localActorID = GetLocalActorID(clientHd, actor.actorUID);
	ASSERT(localActorID != LocalActorID::INVALID);

	const GameXmlContent& content = GetGameXmlContent();

	// SN_PlayerSkillSlot
	{
		PacketWriter<Sv::SN_PlayerSkillSlot> packet;

		packet.Write<LocalActorID>(localActorID); // characterID

		const GameXmlContent::Master& master = content.GetMaster(actor.classType);
		ASSERT(master.skillUnlocked.size() == master.skillIDs.size());

		packet.Write<u16>(master.skillIDs.size());
		for(size_t i = 0; i < master.skillIDs.size(); i++) {
			packet.Write<SkillID>(master.skillIDs[i]);
			packet.Write<u32>(0);
			packet.Write<u8>(1);
			packet.Write<u16>(0);
			packet.Write<u8>(master.skillUnlocked[i]);
			packet.Write<u8>(1);
		}

		packet.Write<SkillID>(SkillID::INVALID); // stageSkillIndex1
		packet.Write<SkillID>(SkillID::INVALID); // stageSkillIndex2
		packet.Write<SkillID>(master.skillIDs[0]); // currentSkillSlot1
		packet.Write<SkillID>(master.skillIDs[1]); // currentSkillSlot2
		packet.Write<SkillID>(master.skillIDs.back()); // shirkSkillSlot

		SendPacket(clientHd, packet);
	}
}

void HubReplication::SendInitialFrame(ClientHandle clientHd)
{
	// SN_ScanEnd
	SendPacketData<Sv::SN_ScanEnd>(clientHd, 0, nullptr);

	// SN_TownHudStatistics
	{
		PacketWriter<Sv::SN_TownHudStatistics> packet;

		packet.Write<u8>(0); // gameModeType
		packet.Write<u8>(0); // gameType
		packet.Write<u16>(3); // argList_count

		// arglist
		packet.Write<i32>(479);
		packet.Write<i32>(0);
		packet.Write<i32>(16);

		SendPacket(clientHd, packet);
	}
}

void HubReplication::CreateLocalActorID(ClientHandle clientHd, ActorUID actorUID)
{
	const i32 clientID = plidMap->Get(clientHd);

	PlayerLocalInfo& localInfo = playerLocalInfo[clientID];
	auto& localActorIDMap = localInfo.localActorIDMap;
	localActorIDMap.emplace(actorUID, localInfo.nextPlayerLocalActorID);
	localInfo.nextPlayerLocalActorID = (LocalActorID)((u32)localInfo.nextPlayerLocalActorID + 1);
	// TODO: find first free LocalActorID

	// TODO: start at 5000 for NPCs? Does it even matter?
}

void HubReplication::DeleteLocalActorID(ClientHandle clientHd, ActorUID actorUID)
{
	const i32 clientID = plidMap->Get(clientHd);

	auto& localActorIDMap = playerLocalInfo[clientID].localActorIDMap;
	localActorIDMap.erase(localActorIDMap.find(actorUID));
}

bool HubReplication::Frame::Transform::HasNotChanged(const Frame::Transform& other) const
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

bool HubReplication::Frame::ActionState::HasNotChanged(const HubReplication::Frame::ActionState& other) const
{
	if(other.actionState == ActionStateID::INVALID) return true;
	if(actionState != other.actionState) return false;
	if(actionParam1 != other.actionParam1) return false;
	if(actionParam2 != other.actionParam2) return false;
	return true;
}
