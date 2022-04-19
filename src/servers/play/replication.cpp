#include "replication.h"

#include <common/protocol.h>
#include <EASTL/algorithm.h>
#include <EASTL/fixed_hash_map.h>
#include <EAStdC/EAString.h>
#include <mxm/game_content.h>
#include "config.h"

void Replication::Frame::Clear()
{
	playerList.clear();
	masterList.clear();
	npcList.clear();
	dynamicList.clear();

	playerMap.fill(playerList.end());
	masterMap.clear();
	npcMap.clear();
	dynamicMap.clear();

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

	clientHandle.fill(ClientHandle::INVALID);
	playerState.fill({ PlayerState::DISCONNECTED, PlayerState::DISCONNECTED });
	foreach(p, playerLocalInfo) p->Reset();

	framePrev = &frames[0];
	frameCur = &frames[1];

	framePrev->Clear();
	frameCur->Clear();
}

void Replication::FrameEnd()
{
	ProfileFunction();

	UpdatePlayersLocalState();

	FrameDifference();

	for(int pi = 0; pi < MAX_PLAYERS; pi++) {
		PlayerStatePair& state = playerState[pi];

		const ClientHandle clienHd = clientHandle[pi];
		if(state.prev == PlayerState::DISCONNECTED && state.cur == PlayerState::CONNECTED) {
			SendAccountDataPvp(clienHd);
		}
		if(state.prev == PlayerState::CONNECTED && state.cur == PlayerState::IN_GAME) {
			SendInitialFrame(clienHd);
		}
		else if(state.prev == PlayerState::IN_GAME && state.cur == PlayerState::LOADED) {
			SendPvpLoadingComplete(clienHd);
		}

		state.prev = state.cur;
	}

	eastl::swap(frameCur, framePrev);
	frameCur->Clear(); // clear frame
}

void Replication::FramePushPlayer(const Player& player)
{
	ASSERT(frameCur->playerMap[player.index] == frameCur->playerList.end());

	frameCur->playerList.emplace_back(player);
	frameCur->playerMap[player.index] = --frameCur->playerList.end();

#ifdef CONF_DEBUG
	if(player.clientHd != ClientHandle::INVALID) {
		ASSERT(playerMap.at(player.clientHd) == player.index);
	}
#endif
}

void Replication::FramePushMasterActors(const Replication::ActorMaster* actorList, const i32 count)
{
	forarr(actor, actorList, count) {
		ASSERT(frameCur->masterMap.find(actor->actorUID) == frameCur->masterMap.end());
		ASSERT(frameCur->actorUIDSet.find(actor->actorUID) == frameCur->actorUIDSet.end());

		frameCur->masterList.emplace_back(*actor);
		frameCur->masterMap.emplace(actor->actorUID, --frameCur->masterList.end());

		frameCur->actorUIDSet.insert(actor->actorUID);
		frameCur->actorType.emplace(actor->actorUID, actor->Type());
	}
}

void Replication::FramePushNpcActor(const Replication::ActorNpc& actor)
{
	ASSERT(frameCur->npcMap.find(actor.actorUID) == frameCur->npcMap.end());
	ASSERT(frameCur->actorUIDSet.find(actor.actorUID) == frameCur->actorUIDSet.end());

	frameCur->npcList.emplace_back(actor);
	frameCur->npcMap.emplace(actor.actorUID, --frameCur->npcList.end());
	frameCur->actorUIDSet.insert(actor.actorUID);
	frameCur->actorType.emplace(actor.actorUID, actor.Type());
}

void Replication::FramePushDynamicActor(const ActorDynamic& actor)
{
	ASSERT(frameCur->dynamicMap.find(actor.actorUID) == frameCur->dynamicMap.end());
	ASSERT(frameCur->actorUIDSet.find(actor.actorUID) == frameCur->actorUIDSet.end());

	frameCur->dynamicList.emplace_back(actor);
	frameCur->dynamicMap.emplace(actor.actorUID, --frameCur->dynamicList.end());
	frameCur->actorUIDSet.insert(actor.actorUID);
	frameCur->actorType.emplace(actor.actorUID, actor.Type());
}

void Replication::OnPlayerConnect(ClientHandle clientHd, u32 playerIndex)
{
	playerMap[clientHd] = playerIndex;
	playerState[playerIndex].cur = PlayerState::CONNECTED;
	playerLocalInfo[playerIndex].Reset();
	clientHandle[playerIndex] = clientHd;
}

void Replication::SendLoadPvpMap(ClientHandle clientHd, MapIndex stageIndex)
{
	Sv::SN_UpdateGameOwner owner;
	owner.userID = 1;
	SendPacket(clientHd, owner);

	Sv::SN_LobbyStartGame lobby;
	lobby.stageType = StageType::PVP_GAME;
	SendPacket(clientHd, lobby);

	// SN_CityMapInfo
	Sv::SN_CityMapInfo cityMapInfo;
	cityMapInfo.cityMapID = stageIndex;
	SendPacket(clientHd, cityMapInfo);

	/*
	// SN_WeaponState
	{
		PacketWriter<> packet;

		packet.Write<LocalActorID>((LocalActorID)21035); // ownerID
		packet.Write<i32>(90); // weaponID
		packet.Write<i32>(0); // state
		packet.Write<u8>(0); // chargeLevel
		packet.Write<u8>(0); // firingCombo
		packet.Write<i32>(-1); // result

		SendPacketData<>(clientID, Sv::SN_WeaponState::NET_ID, packet.size, packet.data);
	}

	playerState[clientID].cur = PlayerState::IN_GAME;*/
}

void Replication::SetPlayerAsInGame(ClientHandle clientHd)
{
	const i32 clientID = playerMap.at(clientHd);
	playerState[clientID].cur = PlayerState::IN_GAME;
}

void Replication::SetPlayerLoaded(ClientHandle clientHd)
{
	const i32 clientID = playerMap.at(clientHd);
	playerState[clientID].cur = PlayerState::LOADED;
}

void Replication::SendCharacterInfo(ClientHandle clientHd, ActorUID actorUID, CreatureIndex docID, ClassType classType, i32 health, i32 healthMax)
{
	const i32 clientID = playerMap.at(clientHd);

	if(playerState[clientID].cur < PlayerState::IN_GAME) {
		LOG("WARNING(SendCharacterInfo): player not in game (clientID=%d, state=%d)", clientID, (i32)playerState[clientID].cur);
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

void Replication::SendPlayerSetLeaderMaster(ClientHandle clientHd, ActorUID masterActorUID, ClassType classType, SkinIndex skinIndex)
{
	LocalActorID laiLeader = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)classType);
	ASSERT(laiLeader >= LocalActorID::FIRST_SELF_MASTER && laiLeader < LocalActorID::LAST_SELF_MASTER);

	const i32 clientID = playerMap.at(clientHd);
	PlayerForceLocalActorID(clientID, masterActorUID, laiLeader);

	if(playerState[clientID].cur < PlayerState::IN_GAME) {
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

void Replication::SendChatMessageToAll(const wchar* senderName, i32 chatType, const wchar* msg, i32 msgLen)
{
	// TODO: restrict message length
	PacketWriter<Sv::SN_ChatChannelMessage> packet;

	packet.Write<i32>(chatType); // chatType
	packet.WriteStringObj(senderName);
	packet.Write<u8>(0); // senderStaffType
	packet.WriteStringObj(msg, msgLen);

	for(int clientID= 0; clientID < MAX_PLAYERS; clientID++) {
		if(playerState[clientID].cur < PlayerState::IN_GAME) continue;

		SendPacket(clientHandle[clientID], packet);
	}
}

void Replication::SendChatMessageToClient(ClientHandle toClientHd, const wchar* senderName, i32 chatType, const wchar* msg, i32 msgLen)
{
	const i32 toClientID = playerMap.at(toClientHd);
	if(playerState[toClientID].cur < PlayerState::IN_GAME) return;

	if(msgLen == -1) msgLen = EA::StdC::Strlen(msg);

	PacketWriter<Sv::SN_ChatChannelMessage> packet;

	packet.Write<i32>(chatType); // chatType
	packet.WriteStringObj(senderName);
	packet.Write<u8>(0); // senderStaffType
	packet.WriteStringObj(msg, msgLen);

	SendPacket(toClientHd, packet);
}

void Replication::SendChatWhisperConfirmToClient(ClientHandle senderClientHd, const wchar* destNick, const wchar* msg)
{
	PacketWriter<Sv::SA_WhisperSend> packet;

	packet.Write<i32>(0); // result
	packet.WriteStringObj(destNick);
	packet.WriteStringObj(msg);

	SendPacket(senderClientHd, packet);
}

void Replication::SendChatWhisperToClient(ClientHandle destClientHd, const wchar* senderName, const wchar* msg)
{
	PacketWriter<Sv::SN_WhisperReceive> packet;

	packet.WriteStringObj(senderName); // senderNick
	packet.Write<u8>(0); // staffType
	packet.WriteStringObj(msg); // msg

	SendPacket(destClientHd, packet);
}

void Replication::SendAccountDataPvp(ClientHandle clientHd)
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

	const u32 playerIndex = playerMap.at(clientHd);
	const Player* player = frameCur->FindPlayer(playerIndex);
	ASSERT(player);

	const GameXmlContent::Master& masterMain = content.GetMaster(player->mainClass);
	const GameXmlContent::Master& masterSub = content.GetMaster(player->subClass);

	// SN_AccountInfo
	{
		PacketWriter<Sv::SN_AccountInfo> packet;

		packet.WriteStringObj(player->name.data()); // nick
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

		SendPacket(clientHd, packet);
	}

	// SN_ProfileCharacters
	{
		const ActorMaster* actorMain = frameCur->FindMaster(player->masters[0]);
		const ActorMaster* actorSub = frameCur->FindMaster(player->masters[1]);
		ASSERT(actorMain);
		ASSERT(actorSub);

		PacketWriter<Sv::SN_ProfileCharacters> packet;

		packet.Write<u16>(2); // charaList_count

		{
			Sv::SN_ProfileCharacters::Character chara;
			chara.characterID = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)player->mainClass);
			chara.creatureIndex = CreatureIndex(100000000 + (i32)player->mainClass);
			chara.skillShot1 = masterMain.skillIDs[0];
			chara.skillShot2 = masterMain.skillIDs[1];
			chara.classType = player->mainClass;
			chara.x = actorMain->pos.x;
			chara.y = actorMain->pos.y;
			chara.z = actorMain->pos.z;
			chara.characterType = 1;
			chara.skinIndex = player->mainSkin;
			chara.weaponIndex = masterMain.weaponIDs[1];
			chara.masterGearNo = 1;
			packet.Write(chara);
		}

		{
			Sv::SN_ProfileCharacters::Character chara;
			chara.characterID = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)player->subClass);
			chara.creatureIndex = CreatureIndex(100000000 + (i32)player->subClass);
			chara.skillShot1 = masterSub.skillIDs[0];
			chara.skillShot2 = masterSub.skillIDs[1];
			chara.classType = player->subClass;
			chara.x = actorSub->pos.x;
			chara.y = actorSub->pos.y;
			chara.z = actorSub->pos.z;
			chara.characterType = 1;
			chara.skinIndex = player->subSkin;
			chara.weaponIndex = masterSub.weaponIDs[1];
			chara.masterGearNo = 1;
			packet.Write(chara);
		}

		SendPacket(clientHd, packet);
	}

	// SN_ProfileWeapons
	{
		PacketWriter<Sv::SN_ProfileWeapons,4096> packet;

		packet.Write<u16>(2); // weaponList_count

		Sv::SN_ProfileWeapons::Weapon weap;
		weap.characterID = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)player->mainClass);
		weap.weaponType = 1;
		weap.weaponIndex = masterMain.weaponIDs[1];
		weap.grade = 0;
		weap.isUnlocked = 1;
		weap.isActivated = 1;
		packet.Write(weap);

		weap.characterID = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)player->subClass);
		weap.weaponType = 1;
		weap.weaponIndex = masterSub.weaponIDs[1];
		weap.grade = 0;
		weap.isUnlocked = 1;
		weap.isActivated = 1;
		packet.Write(weap);

		SendPacket(clientHd, packet);
	}

	// SN_ProfileItems
	{
		PacketWriter<Sv::SN_ProfileItems,4096> packet;

		packet.Write<u8>(1); // packetNum
		packet.Write<u16>(0); // items_count

		SendPacket(clientHd, packet);
	}

	// SN_ProfileSkills
	{
		PacketWriter<Sv::SN_ProfileSkills,4096> packet;

		packet.Write<u8>(1); // packetNum

		const i32 skillCount = masterMain.skillIDs.size() + masterSub.skillIDs.size();
		packet.Write<u16>(skillCount); // skills_count

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

		i32 skillStatusID = 0;
		foreach(skill, masterMain.skillIDs) {
			packet.Write<LocalActorID>((LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)masterMain.classType)); // characterID
			packet.Write<SkillID>(*skill);
			packet.Write<u8>(skillStatusList[skillStatusID].isUnlocked); // isUnlocked
			packet.Write<u8>(skillStatusList[skillStatusID].isActivated); // isActivated
			packet.Write<u16>(0); // properties_count

			skillStatusID++;
		}

		skillStatusID = 0;
		foreach(skill, masterSub.skillIDs) {
			packet.Write<LocalActorID>((LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)masterSub.classType)); // characterID
			packet.Write<SkillID>(*skill);
			packet.Write<u8>(skillStatusList[skillStatusID].isUnlocked); // isUnlocked
			packet.Write<u8>(skillStatusList[skillStatusID].isActivated); // isActivated
			packet.Write<u16>(0); // properties_count

			skillStatusID++;
		}

		SendPacket(clientHd, packet);
	}

	// SN_ProfileMasterGears
	{
		PacketWriter<Sv::SN_ProfileMasterGears,4096> packet;

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

		SendPacket(clientHd, packet);
	}

	// SN_AccountEquipmentList
	{
		PacketWriter<Sv::SN_AccountEquipmentList,4096> packet;
		packet.Write<i32>(-1); // supportKitDocIndex
		SendPacket(clientHd, packet);
	}

	// SN_Money
	{
		Sv::SN_Money money;
		money.nMoney = 666;
		money.nReason = 1;
		SendPacket(clientHd, money);
	}

	// SN_LoadingProgressData
	{
		foreach_const(pit, frameCur->playerList) {
			if(pit->clientHd == clientHd) continue; // skip self

			PacketWriter<Sv::SN_LoadingProgressData> packet;
			packet.Write(pit->userID); //userID
			packet.WriteStringObj(pit->name.data()); // nickname

			// bot
			if(pit->clientHd == ClientHandle::INVALID) {
				packet.Write<u8>(0); // progressData
			}
			else {
				packet.Write<u8>(0); // progressData
			}

			packet.Write<CreatureIndex>((CreatureIndex)(100000000 + (i32)pit->mainClass)); //activeCreatureIndex
			packet.Write<CreatureIndex>((CreatureIndex)(100000000 + (i32)pit->subClass)); //inactiveCreatureIndex
			packet.Write<u8>(pit->team == 2); // isSpectator
			SendPacket(clientHd, packet);
		}
	}

	// SN_GameFieldReady
	{
		PacketWriter<Sv::SN_GameFieldReady,4096> packet;

		packet.Write<i32>(1); // InGameID=449
		packet.Write(GameType::PVP_Tutorial); // GameType=
		packet.Write<i32>(190002102); // AreaIndex=190002202
		packet.Write<StageIndex>(StageIndex(200020104)); // StageIndex
		packet.Write(GameDefinition::System); // GameDefinitionType=
		packet.Write<u8>(6); // initPlayerCount=6
		packet.Write<u8>(1); // CanEscape=1
		packet.Write<u8>(0); // IsTrespass=0
		packet.Write<u8>(0); // IsSpectator=0

		// InGameUsers
		packet.Write<u16>(frameCur->playerList.size());

		foreach_const(pit, frameCur->playerList) {
			packet.Write(pit->userID); //userID
			packet.WriteStringObj(pit->name.data()); // nickname
			packet.Write<u8>(3 + pit->team); // team
			packet.Write<u8>(pit->clientHd == ClientHandle::INVALID); // isBot
		}

		// IngamePlayers
		packet.Write<u16>(frameCur->playerList.size());

		foreach_const(pit, frameCur->playerList) {
			const bool self = pit->clientHd == clientHd;
			const Player& p = *pit;

			const GameXmlContent::Master& pmmain = content.GetMaster(p.mainClass);
			const GameXmlContent::Master& pmsub = content.GetMaster(p.subClass);

			packet.Write(pit->userID); //userID
			packet.Write<CreatureIndex>((CreatureIndex)(100000000 + (i32)p.mainClass)); //mainCreatureIndex
			packet.Write<SkinIndex>(p.mainSkin); //mainSkinIndex

			if(self) {
				packet.Write<SkillID>(pmmain.skillIDs[0]); //mainSkillindex1
				packet.Write<SkillID>(pmmain.skillIDs[1]); //mainSkillIndex2
			}
			else {
				packet.Write<SkillID>(SkillID::INVALID); //mainSkillindex1
				packet.Write<SkillID>(SkillID::INVALID); //mainSkillIndex2
			}


			packet.Write<CreatureIndex>((CreatureIndex)(100000000 + (i32)p.subClass)); //subCreatureIndex
			packet.Write<SkinIndex>(p.subSkin); //subSkinIndex

			if(self) {
				packet.Write<SkillID>(pmsub.skillIDs[0]); //subSkillIndex1
				packet.Write<SkillID>(pmsub.skillIDs[1]); //subSkillIndex2
			}
			else {
				packet.Write<SkillID>(SkillID::INVALID); //mainSkillindex1
				packet.Write<SkillID>(SkillID::INVALID); //mainSkillIndex2
			}

			packet.Write<i32>(-1); //stageSkillIndex1
			packet.Write<i32>(-1); //stageSkillIndex2
			packet.Write<i32>(-1); //supportKitIndex
			packet.Write<u8>(pit->clientHd == ClientHandle::INVALID); //isBot
		}

		// IngameGuilds
		packet.Write<u16>(0);
		packet.Write<u32>(180000); // surrenderAbleTime

		SendPacket(clientHd, packet);
	}
}

void Replication::SendPvpLoadingComplete(ClientHandle clientHd)
{
	const u32 playerIndex = playerMap.at(clientHd);
	const Player* player = frameCur->FindPlayer(playerIndex);
	ASSERT(player);

	// SN_NotifyPcDetailInfos
	{
		PacketWriter<Sv::SN_NotifyPcDetailInfos,1024> packet;

		u16 playerCount = 0;
		foreach_const(pl, frameCur->playerList) {
			const Player& p = *pl;
			if(p.team == player->team) {
				playerCount++;
			}
		}

		packet.Write<u16>(playerCount); // pcList_count

		foreach_const(pl, frameCur->playerList) {
			const Player& p = *pl;
			if(p.team == player->team) {
				const ActorMaster* main = frameCur->FindMaster(p.masters[0]);
				const ActorMaster* sub = frameCur->FindMaster(p.masters[1]);
				ASSERT(main);
				ASSERT(sub);

				packet.Write<UserID>(p.userID); // userID
				// mainPC
				packet.Write<LocalActorID>(GetLocalActorID(clientHd, main->actorUID)); // characterID
				packet.Write<CreatureIndex>(CreatureIndex(100000000 + (i32)main->classType)); // docID
				packet.Write<ClassType>(main->classType); // classType
				packet.Write<i32>(2400); // hp
				packet.Write<i32>(2400); // maxHp
				// subPC
				packet.Write<LocalActorID>(GetLocalActorID(clientHd, sub->actorUID)); // characterID
				packet.Write<CreatureIndex>(CreatureIndex(100000000 + (i32)sub->classType)); // docID
				packet.Write<ClassType>(sub->classType); // classType
				packet.Write<i32>(2400); // hp
				packet.Write<i32>(2400); // maxHp

				packet.Write<i32>(0); // remainTagCooltimeMS
				packet.Write<u8>(0); // canCastSkillSlotUG
			}
		}


		SendPacket(clientHd, packet);
	}

	// SN_InitScoreBoard
	{
		PacketWriter<Sv::SN_InitScoreBoard,4096> packet;

		packet.Write<u16>(frameCur->playerList.size()); // userInfos_count

		foreach_const(pl, frameCur->playerList) {
			const Player& player = *pl;
			const ActorMaster* main = frameCur->FindMaster(player.masters[0]);
			const ActorMaster* sub = frameCur->FindMaster(player.masters[1]);
			ASSERT(main);
			ASSERT(sub);

			packet.Write<UserID>(player.userID); // usn
			packet.WriteStringObj(player.name.data());
			packet.Write<i32>(3 + player.team); // teamType | TODO: team
			packet.Write<CreatureIndex>(CreatureIndex(100000000 + (i32)main->classType)); // mainCreatureIndex
			packet.Write<CreatureIndex>(CreatureIndex(100000000 + (i32)sub->classType)); // subCreatureIndex
		}


		SendPacket(clientHd, packet);
	}

	// SA_LoadingComplete
	SendPacketData<Sv::SA_LoadingComplete>(clientHd,  0, nullptr);
}

void Replication::SendGameReady(ClientHandle clientHd, i32 waitTime, i32 elapsed)
{
	Sv::SA_GameReady ready;
	ready.waitingTimeMs = waitTime;
	ready.serverTimestamp = (i64)TimeDiffMs(TimeRelNow());
	ready.readyElapsedMs = elapsed;
	SendPacket(clientHd, ready);

	Sv::SN_NotifyIngameSkillPoint notify;
	notify.userID = 1;
	notify.skillPoint = 1;
	SendPacket(clientHd, notify);
}

void Replication::SendPreGameLevelEvents(ClientHandle clientHd)
{
	// countdown
	Sv::SN_NotifyTimestamp notifyTimestamp;
	notifyTimestamp.serverTimestamp = (i64)TimeDiffMs(TimeRelNow());
	notifyTimestamp.curCount = 0;
	notifyTimestamp.maxCount = 5;
	SendPacket(clientHd, notifyTimestamp);

	// Sv::SN_RunClientLevelEventSeq
	{
		Sv::SN_RunClientLevelEventSeq seq;
		seq.needCompleteTriggerAckID = -1;
		seq.rootEventID = 218;
		seq.caller = 0;
		seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
		SendPacket(clientHd, seq);
	}
	// Sv::SN_RunClientLevelEventSeq
	{
		Sv::SN_RunClientLevelEventSeq seq;
		seq.needCompleteTriggerAckID = -1;
		seq.rootEventID = 219;
		seq.caller = 0;
		seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
		SendPacket(clientHd, seq);
	}
	// Sv::SN_RunClientLevelEventSeq
	{
		Sv::SN_RunClientLevelEventSeq seq;
		seq.needCompleteTriggerAckID = -1;
		seq.rootEventID = 274;
		seq.caller = 0;
		seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
		SendPacket(clientHd, seq);
	}
	// Sv::SN_RunClientLevelEvent
	{
		Sv::SN_RunClientLevelEvent event;
		event.eventID = 48;
		event.caller = 0;
		event.serverTime = (i64)TimeDiffMs(TimeRelNow());
		SendPacket(clientHd, event);
	}
	// Sv::SN_RunClientLevelEventSeq
	{
		Sv::SN_RunClientLevelEventSeq seq;
		seq.needCompleteTriggerAckID = 1000000;
		seq.rootEventID = 1000001;
		seq.caller = 21035;
		seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
		SendPacket(clientHd, seq);
	}
	// Sv::SN_RunClientLevelEvent
	{
		Sv::SN_RunClientLevelEvent event;
		event.eventID = 150;
		event.caller = 21035;
		event.serverTime = (i64)TimeDiffMs(TimeRelNow());
		SendPacket(clientHd, event);
	}

	/*
	Sv::SN_NotifyIsInSafeZone safe;
	safe.userID = 1;
	safe.inSafeZone = 1;
	LOG("[client%03d] Server :: SN_NotifyIsInSafeZone", clientID);
	SendPacket(clientHd, safe);
	*/
}

void Replication::SendGameStart(ClientHandle clientHd)
{
	SendPacketData<Sv::SN_GameStart>(clientHd, 0, nullptr);

	Sv::SN_NotifyAasRestricted notify;
	notify.isRestrictedByAAS = 0;
	SendPacket(clientHd, notify);

	// Release input lock (Data\Design\Level\PVP\PVP_DeathMatch\EVENTNODES\LEVELEVENT_CLIENT.XML)

	// Sv::SN_RunClientLevelEventSeq
	{
		Sv::SN_RunClientLevelEventSeq seq;
		seq.needCompleteTriggerAckID = -1;
		seq.rootEventID = 163;
		seq.caller = 0;
		seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
		SendPacket(clientHd, seq);
	}

	// Timer related event

	// Sv::SN_RunClientLevelEventSeq
	{
		Sv::SN_RunClientLevelEventSeq seq;
		seq.needCompleteTriggerAckID = -1;
		seq.rootEventID = 270;
		seq.caller = 0;
		seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
		SendPacket(clientHd, seq);
	}
}

void Replication::SendPlayerTag(ClientHandle clientHd, ActorUID mainActorUID, ActorUID subActorUID)
{
	Sv::SN_GamePlayerTag tag;
	tag.result = 128;
	tag.mainID = GetLocalActorID(clientHd, mainActorUID);
	tag.subID = GetLocalActorID(clientHd, subActorUID);
	tag.attackerID = LocalActorID::INVALID;

	ASSERT(tag.mainID != LocalActorID::INVALID);
	ASSERT(tag.subID != LocalActorID::INVALID);

	SendPacket(clientHd, tag);
}

void Replication::SendPlayerJump(ClientHandle clientHd, ActorUID mainActorUID, f32 rotate, f32 moveDirX, f32 moveDirY)
{
	LocalActorID localActorID = GetLocalActorID(clientHd, mainActorUID);
	ASSERT(localActorID != LocalActorID::INVALID);

	PacketWriter<Sv::SA_ResultSpAction> packet;

	packet.Write<u8>(0x20); // excludedFieldBits
	packet.Write<i32>(0); // actionID
	packet.Write<LocalActorID>(GetLocalActorID(clientHd, mainActorUID));
	packet.Write<f32>(rotate);
	packet.Write<f32>(moveDirX);
	packet.Write<f32>(moveDirY);
	packet.Write<i32>(0); // errorType

	SendPacket(clientHd, packet);
}

void Replication::SendPlayerAcceptCast(ClientHandle clientHd, const PlayerCastSkill& cast)
{
	LocalActorID localActorID = GetLocalActorID(clientHd, cast.playerActorUID);
	ASSERT(localActorID != LocalActorID::INVALID);

	// SA_CastSkill
	{
		Sv::SA_CastSkill accept;
		accept.characterID = localActorID;
		accept.ret = 0;
		accept.skillIndex = cast.skillID;

		SendPacket(clientHd, accept);
	}

	// SN_CastSkill
	{
		PacketWriter<Sv::SN_CastSkill> packet;

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

		SendPacket(clientHd, packet);
	}

	// SN_ExecuteSkill
	{
		PacketWriter<Sv::SN_ExecuteSkill,512> packet;

		packet.Write<LocalActorID>(localActorID); // entityID
		packet.Write<i32>(0); // ret
		packet.Write<SkillID>(cast.skillID);
		packet.Write<u8>(0); // costLevel

		ActionStateID actionState = ActionStateID::INVALID;
		f32 distance = 0.0f;
		f32 moveDuration = 1.0f; // WARNING: if this is 0 the character fall through the map (probably a division by 0 error)

		switch(cast.skillID) {
			// Lua dodge
			case (SkillID)180350002: {
				actionState = ActionStateID::SHIRK_BEHAVIORSTATE;
				distance = 500;
				moveDuration = 0.7333333f;
			} break;

			// Sizuka dodge
			case (SkillID)180030002: {
				actionState = ActionStateID::SHIRK_BEHAVIORSTATE;
				distance = 500;
				moveDuration = 0.01f;
			} break;

			case (SkillID)180350010: actionState = ActionStateID::SKILL_1_BEHAVIORSTATE; break;
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
		packet.Write<u8>(1); // bApply
		packet.Write<float3>(cast.posStruct.pos); // startPos
		vec3 endPos = f2v(cast.posStruct.pos) + vec3(distance, 0, 0);
		packet.Write<float3>(v2f(endPos)); // endPos
		packet.Write<f32>(moveDuration); // durationTimeS
		packet.Write<f32>(distance); // originDistance

		SendPacket(clientHd, packet);
	}
}

void Replication::OnPlayerDisconnect(ClientHandle clientHd)
{
	const i32 clientID = playerMap.at(clientHd);
	playerState[clientID].cur = PlayerState::DISCONNECTED;
	clientHandle[clientID] = ClientHandle::INVALID;
}

void Replication::PlayerRegisterMasterActor(ClientHandle clientHd, ActorUID masterActorUID, ClassType classType)
{
	LocalActorID laiLeader = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)classType);
	ASSERT(laiLeader >= LocalActorID::FIRST_SELF_MASTER && laiLeader < LocalActorID::LAST_SELF_MASTER);

	const i32 clientID = playerMap.at(clientHd);
	PlayerForceLocalActorID(clientID, masterActorUID, laiLeader);
}

void Replication::PlayerForceLocalActorID(i32 clientID, ActorUID actorUID, LocalActorID localActorID)
{
	DBG_ASSERT(actorUID != ActorUID::INVALID);

	auto& map = playerLocalInfo[clientID].localActorIDMap;
	ASSERT(map.find(actorUID) == map.end());
	map.emplace(actorUID, localActorID);
}

LocalActorID Replication::GetLocalActorID(ClientHandle clientHd, ActorUID actorUID) const
{
	const i32 clientID = playerMap.at(clientHd);
	const auto& map = playerLocalInfo[clientID].localActorIDMap;
	auto found = map.find(actorUID);
	if(found != map.end()) {
		return found->second;
	}
	return LocalActorID::INVALID;
}

ActorUID Replication::GetWorldActorUID(ClientHandle clientHd, LocalActorID localActorID) const
{
	ProfileFunction();

	// TODO: second map, for this reverse lookup
	const i32 clientID = playerMap.at(clientHd);
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
	for(int pi = 0; pi < MAX_PLAYERS; pi++) {
		if(playerState[pi].cur < PlayerState::IN_GAME) continue;

		PlayerLocalInfo& localInfo = playerLocalInfo[pi];
		const ClientHandle clientHd = clientHandle[pi];
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
				case ActorType::Master: {
					auto pm = framePrev->masterMap.find(actorUID);
					ASSERT(pm != framePrev->masterMap.end());
					ASSERT(pm->second->actorUID == actorUID);
				} break;

				case ActorType::Npc: {
					auto pm = framePrev->npcMap.find(actorUID);
					ASSERT(pm != framePrev->npcMap.end());
					ASSERT(pm->second->actorUID == actorUID);
				} break;

				case ActorType::Dynamic: {
					auto pm = framePrev->dynamicMap.find(actorUID);
					ASSERT(pm != framePrev->dynamicMap.end());
					ASSERT(pm->second->actorUID == actorUID);
				} break;

				default: {
					ASSERT_MSG(0, "case not handled");
				} break;
			}
#endif

			SendActorDestroy(clientHd, actorUID);

			// Remove LocalActorID link
			DeleteLocalActorID(pi, actorUID);
		}

		// send new spawns
		foreach(setIt, addedList) {
			const ActorUID actorUID = *setIt;

			// Create a LocalActorID link if none exists already
			// If one exists already, we have pre-allocated it (like with leader master)
			if(GetLocalActorID(clientHd, actorUID) == LocalActorID::INVALID) {
				CreateLocalActorID(pi, actorUID);
			}

			auto type = frameCur->actorType.find(actorUID);
			ASSERT(type != frameCur->actorType.end());

			switch(type->second) {
				case ActorType::Master: {
					const ActorMaster* chara = frameCur->FindMaster(actorUID);
					ASSERT(chara);
					const Player* parent = frameCur->FindPlayer(chara->playerIndex);
					ASSERT(parent);
					SendActorMasterSpawn(clientHd, *chara, *parent);
				} break;

				case ActorType::Npc: {
					const auto pm = frameCur->npcMap.find(actorUID);
					ASSERT(pm != frameCur->npcMap.end());
					ASSERT(pm->second->actorUID == actorUID);
					SendActorNpcSpawn(clientHd, *pm->second);
				} break;

				case ActorType::Dynamic: {
					const auto pm = frameCur->dynamicMap.find(actorUID);
					ASSERT(pm != frameCur->dynamicMap.end());
					ASSERT(pm->second->actorUID == actorUID);
					SendActorDynamicSpawn(clientHd, *pm->second);
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
		ClientHandle clientHd;
		ActorUID actorUID;
		vec3 pos;
		vec2 moveDir;
		RotationHumanoid rotation;
		f32	speed;
		ActionStateID action;
	};

	struct UpdateRotation
	{
		ClientHandle clientHd;
		ActorUID actorUID;
		RotationHumanoid rot;
	};

	struct UpdateTag
	{
		ClientHandle clientHd;
		ActorUID mainUID;
		ActorUID subUID;
		vec3 pos;
		RotationHumanoid rot;
	};

	struct UpdateJump
	{
		ActorUID actorUID;
		vec2 moveDir;
		f32 rotate;
	};

	struct UpdateSendCast
	{
		ClientHandle clientHd;
		ActorUID actorUID;
		SkillID skill;
		f32 distance;
		f32 moveDurationS;
		vec3 startPos;
		vec3 endPos;
		vec2 moveDir;
		RotationHumanoid rotation;
		f32 speed;
		ActionStateID action;
	};

	eastl::fixed_vector<UpdatePosition,1024> listUpdatePosition;
	eastl::fixed_vector<UpdateRotation,1024> listUpdateRotation;
	eastl::fixed_vector<UpdateTag,1024> listUpdateTag;
	eastl::fixed_vector<UpdateJump,1024> listUpdateJump;
	eastl::fixed_vector<UpdateSendCast,1024> listUpdateSendCast;

	foreach_const(it, frameCur->playerList) {
		const Player& cur = *it;
		const Player* found = framePrev->FindPlayer(cur.index);
		if(!found) continue;
		const Player& prev = *found;

		// tagging
		if(cur.mainCharaID != prev.mainCharaID) {
			UpdateTag update;
			update.clientHd = cur.clientHd;
			update.mainUID = cur.masters[cur.mainCharaID];
			update.subUID = cur.masters[cur.mainCharaID ^ 1];
			const ActorMaster* chara = frameCur->FindMaster(cur.masters[cur.mainCharaID]); // @Speed
			ASSERT(chara);
			update.pos = chara->pos;
			update.rot = chara->rotation;
			listUpdateTag.push_back(update);
		}

		if(cur.hasJumped && !prev.hasJumped) {
			UpdateJump update;
			update.actorUID = cur.masters[cur.mainCharaID];
			const ActorMaster* chara = frameCur->FindMaster(cur.masters[cur.mainCharaID]);  // @Speed
			ASSERT(chara);
			update.rotate = chara->rotation.bodyYaw;
			update.moveDir = chara->moveDir;
			listUpdateJump.push_back(update);
		}
	}

	// TODO: don't update everything here if tagged out (such as position)
	// find if the position has changed since last frame
	foreach_const(it, frameCur->masterList) {
		const ActorMaster& cur = *it;
		if(cur.taggedOut) continue;

		auto found = framePrev->masterMap.find(cur.actorUID);
		if(found == framePrev->masterMap.end()) continue; // previous not found, can't diff
		const ActorMaster& prev = *found->second;

		bool rotationUpdated = false;
		bool positionUpdated = false;

		const f32 rotEpsilon = 0.1f;
		if(!rotationUpdated &&
		   (fabs(cur.rotation.upperYaw - prev.rotation.upperYaw) > rotEpsilon ||
		   fabs(cur.rotation.upperPitch - prev.rotation.upperPitch) > rotEpsilon ||
		   fabs(cur.rotation.bodyYaw - prev.rotation.bodyYaw) > rotEpsilon))
		{
			UpdateRotation update;
			update.clientHd = cur.clientHd;
			update.actorUID = cur.actorUID;
			update.rot = cur.rotation;
			listUpdateRotation.push_back(update);
		}

		if(cur.castSkill != SkillID::INVALID) {
			UpdateSendCast update;
			update.clientHd = cur.clientHd;
			update.actorUID = cur.actorUID;
			update.skill = cur.castSkill;
			update.startPos = cur.skillStartPos;
			update.endPos = cur.skillEndPos;
			update.moveDir = cur.moveDir;
			update.rotation = cur.rotation;
			update.speed = cur.speed;
			update.distance = glm::length(cur.skillEndPos - cur.skillStartPos);
			update.moveDurationS = cur.skillMoveDurationS;
			update.action = cur.actionState;
			listUpdateSendCast.push_back(update);
			positionUpdated = true;
		}

		// position
		const f32 posEpsilon = 0.5f;
		const f32 dirEpsilon = 0.001f;
		const f32 speedEpsilon = 0.001f;
		if(!positionUpdated &&
		   fabs(cur.pos.x - prev.pos.x) > posEpsilon ||
		   fabs(cur.pos.y - prev.pos.y) > posEpsilon ||
		   fabs(cur.pos.z - prev.pos.z) > posEpsilon ||
		   fabs(cur.moveDir.x - prev.moveDir.x) > dirEpsilon ||
		   fabs(cur.moveDir.y - prev.moveDir.y) > dirEpsilon ||
		   fabs(cur.speed - prev.speed) > speedEpsilon)
		{
			UpdatePosition update;
			update.clientHd = cur.clientHd;
			update.actorUID = cur.actorUID;
			update.pos = cur.pos;
			update.moveDir = cur.moveDir;
			update.rotation = cur.rotation;
			update.speed = cur.speed;

			if(cur.actionState != ActionStateID::INVALID) {
				update.action = cur.actionState;
				//update.action = ActionStateID::JUMP_START_MOVESTATE;
			}
			else {
				update.action = ActionStateID::NONE_BEHAVIORSTATE;
			}

			listUpdatePosition.push_back(update);

			rotationUpdated = true;
		}
	}

	// send updates
	foreach_const(up, listUpdatePosition) {
		Sv::SN_PlayerSyncMove sync;
		sync.destPos = v2f(up->pos);
		sync.moveDir = v2f(up->moveDir);
		sync.upperDir = { WorldYawToMxmYaw(up->rotation.upperYaw), WorldPitchToMxmPitch(up->rotation.upperPitch) };
		sync.nRotate = WorldYawToMxmYaw(up->rotation.bodyYaw);
		sync.nSpeed = up->speed;
		sync.flags = 0;
		sync.state = up->action;

		for(int pi = 0; pi < MAX_PLAYERS; pi++) {
			if(playerState[pi].cur < PlayerState::IN_GAME) continue;
			const ClientHandle clientHd = clientHandle[pi];
			if(clientHd == up->clientHd) continue; // ignore self

			ProfileBlock("Send Sv::SN_PlayerSyncMove");

			sync.characterID = GetLocalActorID(clientHd, up->actorUID);
			SendPacket(clientHd, sync);
		}
	}

	foreach_const(up, listUpdateRotation) {
		Sv::SN_PlayerSyncTurn sync;
		sync.upperDir = { WorldYawToMxmYaw(up->rot.upperYaw), WorldPitchToMxmPitch(up->rot.upperPitch) };
		sync.nRotate = WorldYawToMxmYaw(up->rot.bodyYaw);

		for(int pi = 0; pi < MAX_PLAYERS; pi++) {
			if(playerState[pi].cur < PlayerState::IN_GAME) continue;
			const ClientHandle clientHd = clientHandle[pi];
			if(clientHd == up->clientHd) continue; // ignore self

			sync.characterID = GetLocalActorID(clientHd, up->actorUID);
			SendPacket(clientHd, sync);
		}
	}

	foreach_const(up, listUpdateTag) {
		Sv::SN_GamePlayerTag tag;
		tag.result = 128;
		tag.attackerID = LocalActorID::INVALID;

		for(int pi = 0; pi < MAX_PLAYERS; pi++) {
			if(playerState[pi].cur < PlayerState::IN_GAME) continue;
			const ClientHandle clientHd = clientHandle[pi];

			tag.mainID = GetLocalActorID(clientHd, up->mainUID);
			tag.subID = GetLocalActorID(clientHd, up->subUID);
			ASSERT(tag.mainID != LocalActorID::INVALID);
			ASSERT(tag.subID != LocalActorID::INVALID);

			SendPacket(clientHd, tag);

			if(clientHd != up->clientHd) { // ignore self
				Sv::SN_GameLeaveActor leave;
				leave.objectID = tag.subID;
				SendPacket(clientHd, leave);

				// Sv::SN_GameEnterActor
				{
					PacketWriter<Sv::SN_GameEnterActor,512> packet;
					packet.Write<u8>(0x1); // excludedBits
					packet.Write<LocalActorID>(tag.mainID); // objectID
					packet.Write<float3>(v2f(up->pos)); //  p3nPos
					packet.Write<RotationHumanoid>(RotConvertToMxm(up->rot)); //  p3nDir
					packet.Write<float2>(v2f(vec2(0, 0))); //  p2nMoveDir
					packet.Write<float2>(v2f(vec2(0, 0))); //  p2nMoveUpperDir
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

					SendPacket(clientHd, packet);
				}
			}
		}
	}

	foreach_const(up, listUpdateJump) {
		for(int pi = 0; pi < MAX_PLAYERS; pi++) {
			if(playerState[pi].cur < PlayerState::IN_GAME) continue;
			const ClientHandle clientHd = clientHandle[pi];

			PacketWriter<Sv::SA_ResultSpAction> packet;

			packet.Write<u8>(0x20); // excludedFieldBits
			packet.Write<i32>(0); // actionID
			packet.Write<LocalActorID>(GetLocalActorID(clientHd, up->actorUID));
			packet.Write<f32>(up->rotate);
			packet.Write<f32>(up->moveDir.x);
			packet.Write<f32>(up->moveDir.y);
			packet.Write<i32>(0); // errorType

			SendPacket(clientHd, packet);
		}
	}

	foreach_const(up, listUpdateSendCast) {
		if(up->clientHd != ClientHandle::INVALID) {
			LocalActorID localActorID = GetLocalActorID(up->clientHd, up->actorUID);

			// SA_CastSkill
			{
				Sv::SA_CastSkill accept;
				accept.characterID = localActorID;
				accept.ret = 0;
				accept.skillIndex = up->skill;

				SendPacket(up->clientHd, accept);
			}
		}

		for(int pi = 0; pi < MAX_PLAYERS; pi++) {
			if(playerState[pi].cur < PlayerState::IN_GAME) continue;
			const ClientHandle clientHd = clientHandle[pi];

			LocalActorID localActorID = GetLocalActorID(clientHd, up->actorUID);

			// SN_CastSkill
			{
				PacketWriter<Sv::SN_CastSkill,512> packet;

				packet.Write<LocalActorID>(localActorID); // entityID
				packet.Write<i32>(0); // ret
				packet.Write<SkillID>(up->skill);
				packet.Write<u8>(0); // costLevel
				packet.Write<ActionStateID>(ActionStateID::INVALID);
				packet.Write<float3>({});

				packet.Write<u16>(0); // targetList_count


#if 0
				packet.Write<u8>(0); // bSyncMyPosition
				packet.Write<float3>({});
				packet.Write<float3>({});
				packet.Write<float2>({});
				packet.Write<RotationHumanoid>({});
				packet.Write<f32>(0);
				packet.Write<i32>(0);

#else
				packet.Write<u8>(1); // bSyncMyPosition
				packet.Write<float3>(v2f(up->startPos));
				packet.Write<float3>(v2f(up->startPos));
				packet.Write<float2>(v2f(up->moveDir));
				packet.Write<RotationHumanoid>(RotConvertToMxm(up->rotation));
				packet.Write<f32>(up->speed);
				//packet.Write<i32>((i64)TimeDiffMs(TimeRelNow()));
				packet.Write<i32>(0);
#endif

				SendPacket(clientHd, packet);
			}

			// SN_ExecuteSkill
			{
				PacketWriter<Sv::SN_ExecuteSkill,512> packet;

				packet.Write<LocalActorID>(localActorID); // entityID
				packet.Write<i32>(0); // ret
				packet.Write<SkillID>(up->skill);
				packet.Write<u8>(0); // costLevel
				packet.Write<ActionStateID>(up->action);
				packet.Write<float3>({});

				packet.Write<u16>(0); // targetList_count


				packet.Write<u8>(0); // bSyncMyPosition
				packet.Write<float3>({});
				packet.Write<float3>({});
				packet.Write<float2>({});
				packet.Write<RotationHumanoid>({});
				packet.Write<f32>(0);
				packet.Write<i32>(0);

				/*
				packet.Write<u8>(1); // bSyncMyPosition
				packet.Write<float3>(v2f(up->actorPos));
				packet.Write<float3>(v2f(up->endPos));
				packet.Write<float2>(v2f(up->moveDir));
				packet.Write<RotationHumanoid>(up->rotation.ConvertToMxm());
				packet.Write<f32>(up->speed);
				packet.Write<i32>((i64)TimeDiffMs(TimeRelNow()));
				*/

				packet.Write<f32>(0); // fSkillChargeDamageMultiplier

				if(up->distance > 0) {
					// graphMove
					packet.Write<u8>(1); // bApply
					packet.Write<float3>(v2f(up->startPos)); // startPos
					packet.Write<float3>(v2f(up->endPos)); // endPos
					packet.Write<f32>(up->moveDurationS); // durationTimeS
					packet.Write<f32>(up->distance); // originDistance
				}
				else {
					// graphMove
					packet.Write<u8>(0); // bApply
					packet.Write<float3>(float3()); // startPos
					packet.Write<float3>(float3()); // endPos
					packet.Write<f32>(0); // durationTimeS
					packet.Write<f32>(0); // originDistance
				}

				SendPacket(clientHd, packet);
			}
		}
	}
}

void Replication::SendActorMasterSpawn(ClientHandle clientHd, const ActorMaster& actor, const Player& parent)
{
	DBG_ASSERT(actor.actorUID != ActorUID::INVALID);

	const LocalActorID localActorID = GetLocalActorID(clientHd, actor.actorUID);
	ASSERT(localActorID != LocalActorID::INVALID);

	LOG("[client%03d] Replication :: SendActorSpawn :: actorUID=%u localActorID=%u", clientHd, (u32)actor.actorUID, (u32)localActorID);

	// this is the main actor
	if(actor.actorUID == parent.masters[0]) {
		// SN_GameCreateActor
		{
			PacketWriter<Sv::SN_GameCreateActor,512> packet;

			packet.Write<LocalActorID>(localActorID); // objectID
			packet.Write<EntityType>(EntityType::CREATURE); // nType
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
			packet.Write<i32>(3 + parent.team); // faction
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

			SendPacket(clientHd, packet);
		}
	}
	// this is the sub actor
	else {
		const LocalActorID parentLocalActorID = GetLocalActorID(clientHd, parent.masters[0]);
		ASSERT(parentLocalActorID != LocalActorID::INVALID);

		// SN_GameCreateSubActor
		{
			PacketWriter<Sv::SN_GameCreateSubActor,512> packet;

			packet.Write<LocalActorID>(localActorID); // objectID
			packet.Write<LocalActorID>(parentLocalActorID); // mainEntityID
			packet.Write<EntityType>(EntityType::CREATURE); // nType
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

		SendPacket(clientHd, packet);
	}

	// SN_GamePlayerEquipWeapon
	{
		PacketWriter<Sv::SN_GamePlayerEquipWeapon> packet;

		packet.Write<LocalActorID>(localActorID); // characterID
		packet.Write<i32>(131135011); // weaponDocIndex
		packet.Write<i32>(0); // additionnalOverHeatGauge
		packet.Write<i32>(0); // additionnalOverHeatGaugeRatio

		SendPacket(clientHd, packet);
	}

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

		SendPacketData<>(clientID, Sv::SN_StatusSnapshot::NET_ID, packet.size, packet.data);
	}
	*/

	SendMasterSkillSlots(clientHd, actor);
}

void Replication::SendActorNpcSpawn(ClientHandle clientHd, const ActorNpc& actor)
{
	DBG_ASSERT(actor.actorUID != ActorUID::INVALID);

	const i32 clientID = playerMap.at(clientHd);
	auto found = playerLocalInfo[clientID].localActorIDMap.find(actor.actorUID);
	ASSERT(found != playerLocalInfo[clientID].localActorIDMap.end());

	const LocalActorID localActorID = found->second;

	LOG("[client%03d] Replication :: SendActorNpcSpawn :: actorUID=%u localActorID=%u", clientID, (u32)actor.actorUID, (u32)localActorID);

	// SN_GameCreateActor
	{
		PacketWriter<Sv::SN_GameCreateActor,512> packet;

		packet.Write<LocalActorID>(localActorID); // objectID
		packet.Write<EntityType>(EntityType::CREATURE); // nType
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
		packet.Write<Faction>(actor.faction); // faction
		packet.Write<ClassType>(ClassType::NONE); // classType
		packet.Write<SkinIndex>(SkinIndex::DEFAULT); // skinIndex
		packet.Write<i32>(0); // seed

		typedef Sv::SN_GameCreateActor::BaseStat::Stat Stat;

		// initStat ------------------------
		switch(actor.docID) {
			case (CreatureIndex)100010101: {
				const Stat maxStats[] = {
					{ 0, 15596.f },
					{ 6, 48.f },
					{ 7, 113.333f },
					{ 8, 10.f },
					{ 9, 5.f },
					{ 10, 150.f },
					{ 13, 100.f },
					{ 14, 80.f },
					{ 15, 100.f },
					{ 52, 70.f },
					{ 64, 150.f },
				};
				const Stat curStats[] = {
					{ 0, 15596.f },
				};

				packet.WriteVec(maxStats, ARRAY_COUNT(maxStats)); // maxStats
				packet.WriteVec(curStats, ARRAY_COUNT(curStats)); // curStats
			} break;

			case (CreatureIndex)110040546: {
				const Stat maxStats[] = {
					{ 0, -1 },
				};
				const Stat curStats[] = {
					{ 0, -1 },
				};

				packet.WriteVec(maxStats, ARRAY_COUNT(maxStats)); // maxStats
				packet.WriteVec(curStats, ARRAY_COUNT(curStats)); // curStats
			} break;

			default: {
				packet.Write<u16>(0); // maxStats
				packet.Write<u16>(0); // curStats
			}
		}
		// ------------------------------------

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

void Replication::SendActorDynamicSpawn(ClientHandle clientHd, const ActorDynamic& actor)
{
	DBG_ASSERT(actor.actorUID != ActorUID::INVALID);

	const i32 clientID = playerMap.at(clientHd);
	auto found = playerLocalInfo[clientID].localActorIDMap.find(actor.actorUID);
	ASSERT(found != playerLocalInfo[clientID].localActorIDMap.end());

	const LocalActorID localActorID = found->second;

	LOG("[client%03d] Replication :: SendActorNpcSpawn :: actorUID=%u localActorID=%u", clientID, (u32)actor.actorUID, (u32)localActorID);

	// SN_GameCreateActor
	{
		PacketWriter<Sv::SN_GameCreateActor,512> packet;

		packet.Write<LocalActorID>(localActorID); // objectID
		packet.Write<EntityType>(EntityType::DYNAMIC); // nType
		packet.Write<CreatureIndex>(actor.docID); // nIDX
		packet.Write<i32>(actor.localID); // dwLocalID

		packet.Write(actor.pos); // p3nPos
		packet.Write(actor.rot); // p3nDir
		packet.Write<i32>(0); // spawnType
		packet.Write<ActionStateID>(ActionStateID::DYNAMIC_NORMAL_STAND); // actionState
		packet.Write<i32>(0); // ownerID
		packet.Write<u8>(0); // bDirectionToNearPC
		packet.Write<i32>(-1); // AiWanderDistOverride
		packet.Write<i32>(-1); // tagID
		packet.Write<Faction>(actor.faction); // faction
		packet.Write<ClassType>(ClassType::NONE); // classType
		packet.Write<SkinIndex>(SkinIndex::DEFAULT); // skinIndex
		packet.Write<i32>(0); // seed

		typedef Sv::SN_GameCreateActor::BaseStat::Stat Stat;

		// initStat ------------------------
		switch(actor.docID) {
			case (CreatureIndex)110040546: {
				const Stat maxStats[] = {
					{ 0, -1 },
				};
				const Stat curStats[] = {
					{ 0, -1 },
				};

				packet.WriteVec(maxStats, ARRAY_COUNT(maxStats)); // maxStats
				packet.WriteVec(curStats, ARRAY_COUNT(curStats)); // curStats
			} break;

			default: {
				packet.Write<u16>(0); // maxStats
				packet.Write<u16>(0); // curStats
			}
		}
		// ------------------------------------

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

void Replication::SendActorDestroy(ClientHandle clientHd, ActorUID actorUID)
{
	const i32 clientID = playerMap.at(clientHd);
	auto found = playerLocalInfo[clientID].localActorIDMap.find(actorUID);
	ASSERT(found != playerLocalInfo[clientID].localActorIDMap.end());
	const LocalActorID localActorID = found->second;

	Sv::SN_DestroyEntity packet;
	packet.characterID = localActorID;
	LOG("[client%03d] Server :: SN_DestroyEntity :: actorUID=%u", clientID, (u32)actorUID);
	SendPacket(clientHd, packet);
}

void Replication::SendMasterSkillSlots(ClientHandle clientHd, const Replication::ActorMaster& actor)
{
	DBG_ASSERT(actor.actorUID != ActorUID::INVALID);
	const LocalActorID localActorID = GetLocalActorID(clientHd, actor.actorUID);
	ASSERT(localActorID != LocalActorID::INVALID);

	const GameXmlContent& content = GetGameXmlContent();

	// SN_PlayerSkillSlot
	{
		PacketWriter<Sv::SN_PlayerSkillSlot,4096> packet;

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

		SendPacket(clientHd, packet);
	}
}

void Replication::SendInitialFrame(ClientHandle clientHd)
{
	// TODO: move to earlier
	// SN_InitIngameModeInfo
	{
		PacketWriter<Sv::SN_InitIngameModeInfo> packet;

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

		SendPacket(clientHd, packet);
	}

	// SN_ScanEnd
	SendPacketData<Sv::SN_ScanEnd>(clientHd, 0, nullptr);

	// SN_LoadClearedStages
	{
		PacketWriter<Sv::SN_LoadClearedStages> packet;

		packet.Write<u16>(0); // count
		SendPacket(clientHd, packet);
	}

	/*
	Sv::SN_NotifyUserLifeInfo lifeInfo;
	lifeInfo.usn = 1;
	lifeInfo.lifeCount = 3;
	lifeInfo.maxLifeCount = 3;
	lifeInfo.remainLifeCount = 0;
	LOG("[client%03d] Server :: SN_NotifyUserLifeInfo ::", clientID);
	SendPacket(clientHd, lifeInfo);
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
