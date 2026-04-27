#include "replication.h"

#include <common/protocol.h>
#include <mxm/hero_stats.h>
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

	skillCastList.clear();
	skillExecList.clear();
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

void Replication::FramePushSkillCast(const SkillCast& skillCast)
{
	frameCur->skillCastList.push_back(skillCast);
}

void Replication::FramePushSkillExec(const SkillExec& skillExec)
{
	frameCur->skillExecList.push_back(skillExec);
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

void Replication::SendChatMessageToClient(ClientHandle toClientHd, const wchar* senderName, EChatType chatType, const wchar* msg, i32 msgLen)
{
	const i32 toClientID = playerMap.at(toClientHd);
	if(playerState[toClientID].cur < PlayerState::IN_GAME) return;

	if(msgLen == -1) msgLen = EA::StdC::Strlen(msg);

	PacketWriter<Sv::SN_ChatChannelMessage> packet;

	packet.Write<EChatType>(chatType); // chatType
	packet.WriteStringObj(senderName);
	packet.Write<u8>(0); // senderStaffType
	packet.WriteStringObj(msg, msgLen);

	SendPacket(toClientHd, packet);
}

void Replication::SendClientLevelEvent(ClientHandle clientHd, i32 eventID)
{
	Sv::SN_RunClientLevelEvent event;
	event.eventID = eventID;
	event.caller = 0;
	event.serverTime = (i64)TimeDiffMs(TimeRelNow());
	SendPacket(clientHd, event);
}

void Replication::SendClientLevelEventSeq(ClientHandle clientHd, i32 eventID)
{
	Sv::SN_RunClientLevelEventSeq seq;
	seq.needCompleteTriggerAckID = -1;
	seq.rootEventID = eventID;
	seq.caller = 0;
	seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
	SendPacket(clientHd, seq);
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

		packet.Write<i32>(1); // InGameID
		if(mapIndex == MapIndex::PVP_TITAN_RUINS) {
			packet.Write(GameType::PVP_Rank);                    // GameType=4 (from new TR captures)
			packet.Write<i32>(190002214);                        // AreaIndex (official TR value)
			packet.Write<StageIndex>(StageIndex(200101000));     // StageIndex (GAME_MODE_GOT)
		} else {
			packet.Write(GameType::PVP_Tutorial);                // GameType=6 (DeathMatch)
			packet.Write<i32>(190002102);                        // AreaIndex (DeathMatch)
			packet.Write<StageIndex>(StageIndex(200020102));     // StageIndex (GAME_MODE_DEATH_MATCH)
		}
		packet.Write(GameDefinition::System); // GameDefinitionType=
		packet.Write<u8>((u8)frameCur->playerList.size()); // initPlayerCount
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

			// Stage skills: Titan Ruins has recall (B) and stage skill
			if(mapIndex == MapIndex::PVP_TITAN_RUINS) {
				packet.Write<i32>(180000010); //stageSkillIndex1 (stage skill)
				packet.Write<i32>(180000110); //stageSkillIndex2 (recall / B key)
			} else {
				packet.Write<i32>(-1); //stageSkillIndex1
				packet.Write<i32>(-1); //stageSkillIndex2
			}
			packet.Write<i32>(-1); //supportKitIndex
			packet.Write<u8>(pit->clientHd == ClientHandle::INVALID); //isBot
		}

		// IngameGuilds
		packet.Write<u16>(0);
		packet.Write<u32>(mapIndex == MapIndex::PVP_TITAN_RUINS ? 900000u : 180000u); // surrenderAbleTime (15min TR, 3min DM)

		SendPacket(clientHd, packet);
	}
}

void Replication::SendPvpLoadingComplete(ClientHandle clientHd)
{
	const u32 playerIndex = playerMap.at(clientHd);
	const Player* player = frameCur->FindPlayer(playerIndex);
	ASSERT(player);

	// SN_NotifyPcDetailInfos — send ALL players (both teams), matching official capture.
	// Official server sends all 10 players at load time. Client caches enemy portraits
	// for death damage info screen. Same-team players listed FIRST to preserve squad GFx.
	{
		PacketWriter<Sv::SN_NotifyPcDetailInfos,2048> packet;

		packet.Write<u16>((u16)frameCur->playerList.size()); // pcList_count (ALL players)

		// Pass 1: same-team players first (preserves SquadMasterSet GFx ordering)
		foreach_const(pl, frameCur->playerList) {
			const Player& p = *pl;
			if(p.team != player->team) continue;

			const ActorMaster* main = frameCur->FindMaster(p.masters[0]);
			const ActorMaster* sub = frameCur->FindMaster(p.masters[1]);
			ASSERT(main);
			ASSERT(sub);

			packet.Write<UserID>(p.userID); // userID
			// mainPC
			packet.Write<LocalActorID>(GetLocalActorID(clientHd, main->actorUID)); // characterID
			packet.Write<CreatureIndex>(CreatureIndex(100000000 + (i32)main->classType)); // docID
			packet.Write<ClassType>(main->classType); // classType
			packet.Write<i32>((i32)p.maxHp); // hp
			packet.Write<i32>((i32)p.maxHp); // maxHp
			// subPC
			packet.Write<LocalActorID>(GetLocalActorID(clientHd, sub->actorUID)); // characterID
			packet.Write<CreatureIndex>(CreatureIndex(100000000 + (i32)sub->classType)); // docID
			packet.Write<ClassType>(sub->classType); // classType
			packet.Write<i32>((i32)p.maxHp); // hp
			packet.Write<i32>((i32)p.maxHp); // maxHp

			packet.Write<i32>(0); // remainTagCooltimeMS
			packet.Write<u8>(0); // canCastSkillSlotUG
		}

		// Pass 2: enemy team players (for death damage info portraits)
		foreach_const(pl, frameCur->playerList) {
			const Player& p = *pl;
			if(p.team == player->team) continue;

			const ActorMaster* main = frameCur->FindMaster(p.masters[0]);
			const ActorMaster* sub = frameCur->FindMaster(p.masters[1]);
			ASSERT(main);
			ASSERT(sub);

			packet.Write<UserID>(p.userID); // userID
			// mainPC
			packet.Write<LocalActorID>(GetLocalActorID(clientHd, main->actorUID)); // characterID
			packet.Write<CreatureIndex>(CreatureIndex(100000000 + (i32)main->classType)); // docID
			packet.Write<ClassType>(main->classType); // classType
			packet.Write<i32>((i32)p.maxHp); // hp
			packet.Write<i32>((i32)p.maxHp); // maxHp
			// subPC
			packet.Write<LocalActorID>(GetLocalActorID(clientHd, sub->actorUID)); // characterID
			packet.Write<CreatureIndex>(CreatureIndex(100000000 + (i32)sub->classType)); // docID
			packet.Write<ClassType>(sub->classType); // classType
			packet.Write<i32>((i32)p.maxHp); // hp
			packet.Write<i32>((i32)p.maxHp); // maxHp

			packet.Write<i32>(0); // remainTagCooltimeMS
			packet.Write<u8>(0); // canCastSkillSlotUG
		}

		SendPacket(clientHd, packet);
	}

	// SN_InitScoreBoard — sent for both modes (needed for Tab and ally sidebar)
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

	// SN_InitIngameModeInfo (62576) — Titan Ruins only: activates Titan UI
	// Binary analysis: 7x i32 + 1x u8(pieceCount) + array(titanDocs) + 1x u8(nextTitan) + array(exceptionStats)
	if(mapIndex == MapIndex::PVP_TITAN_RUINS) {
		u8 buf[128];
		i32 off = 0;
		auto W32 = [&](i32 v){ memmove(buf+off, &v, 4); off += 4; };
		auto W16 = [&](u16 v){ memmove(buf+off, &v, 2); off += 2; };
		auto W8  = [&](u8  v){ buf[off++] = v; };
		W32(60000);  // transformationVotingPlayerCoolTimeByVotingFail
		W32(10000);  // transformationVotingTeamCoolTimeByTransformationEnd
		W32(0);      // playerCoolTimeByTransformationEnd
		W32(0);      // currentTransformationVotingPlayerCoolTimeByVotingFail
		W32(0);      // currentTransformationVotingTeamCoolTimeByTransformationEnd
		W32(0);      // currentPlayerCoolTimeByTransformationEnd
		W32(20);     // chPropertyResetCoolTime
		W8(5);       // transformationPieceCount (u8!)
		// titanDocIndexes array
		W16(3);      // count
		W32(105001102); W32(105001100); W32(105001101);
		W8(0);       // nextTitanIndex (u8!)
		// listExceptionStat array
		W16(0);      // count
		server->SendPacketData(clientHd, 62576, (u16)off, buf);
		LOG("[client] SN_InitIngameModeInfo (62576) sent, payload=%d", off);
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

void Replication::SendCountdown(ClientHandle clientHd, i32 curCount, i32 maxCount)
{
	Sv::SN_NotifyTimestamp notifyTimestamp;
	notifyTimestamp.serverTimestamp = (i64)TimeDiffMs(TimeRelNow());
	notifyTimestamp.curCount = curCount;
	notifyTimestamp.maxCount = maxCount;
	SendPacket(clientHd, notifyTimestamp);
}

void Replication::SendPreGameLevelEvents(ClientHandle clientHd)
{
	if(mapIndex == MapIndex::PVP_TITAN_RUINS) {
		// Titan Ruins pregame events (from official ranked capture)
		{
			Sv::SN_RunClientLevelEventSeq seq;
			seq.needCompleteTriggerAckID = -1;
			seq.rootEventID = 269;
			seq.caller = 0;
			seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
			SendPacket(clientHd, seq);
		}
		{
			Sv::SN_RunClientLevelEventSeq seq;
			seq.needCompleteTriggerAckID = -1;
			seq.rootEventID = 283;
			seq.caller = 0;
			seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
			SendPacket(clientHd, seq);
		}
		{
			Sv::SN_RunClientLevelEventSeq seq;
			seq.needCompleteTriggerAckID = -1;
			seq.rootEventID = 404;
			seq.caller = 0;
			seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
			SendPacket(clientHd, seq);
		}
		return;
	}

	// EVENT: Starts music? (DM only)
	{
		Sv::SN_RunClientLevelEventSeq seq;
		seq.needCompleteTriggerAckID = -1;
		seq.rootEventID = 218;
		seq.caller = 0;
		seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
		SendPacket(clientHd, seq);
	}

	// EVENT: You have entered the Combat Arena, the match begins soon (DM only)
	{
		Sv::SN_RunClientLevelEventSeq seq;
		seq.needCompleteTriggerAckID = -1;
		seq.rootEventID = 219;
		seq.caller = 0;
		seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
		SendPacket(clientHd, seq);
	}

	// EVENT: Locks skill casting
	// Sv::SN_RunClientLevelEventSeq
	{
		Sv::SN_RunClientLevelEventSeq seq;
		seq.needCompleteTriggerAckID = -1;
		seq.rootEventID = 274;
		seq.caller = 0;
		seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
		SendPacket(clientHd, seq);
	}

	// EVENT: Pings the middle of the map where a powerup is
	// Sv::SN_RunClientLevelEvent
	{
		Sv::SN_RunClientLevelEvent event;
		event.eventID = 48;
		event.caller = 0;
		event.serverTime = (i64)TimeDiffMs(TimeRelNow());
		SendPacket(clientHd, event);
	}

	// EVENT: ???
	// Sv::SN_RunClientLevelEventSeq
	{
		Sv::SN_RunClientLevelEventSeq seq;
		seq.needCompleteTriggerAckID = 1000000;
		seq.rootEventID = 1000001;
		seq.caller = 21035;
		seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
		SendPacket(clientHd, seq);
	}

	// EVENT: ???
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
	if(mapIndex == MapIndex::PVP_TITAN_RUINS) {
		// EVENT 837: Pre-game start setup
		{
			Sv::SN_RunClientLevelEvent event;
			event.eventID = 837;
			event.caller = 0;
			event.serverTime = (i64)TimeDiffMs(TimeRelNow());
			SendPacket(clientHd, event);
		}
		// EVENT 838: Game start trigger
		{
			Sv::SN_RunClientLevelEvent event;
			event.eventID = 838;
			event.caller = 0;
			event.serverTime = (i64)TimeDiffMs(TimeRelNow());
			SendPacket(clientHd, event);
		}
		// EVENT 85: TimeRecordStart — triggers client's gate opening chain
		// (from LevelEvent_Server.xml: opens entities 4,14,114,123 + unlocks input)
		{
			Sv::SN_RunClientLevelEventSeq seq;
			seq.needCompleteTriggerAckID = -1;
			seq.rootEventID = 85;
			seq.caller = 0;
			seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
			SendPacket(clientHd, seq);
		}
	}

	SendPacketData<Sv::SN_GameStart>(clientHd, 0, nullptr);

	Sv::SN_NotifyAasRestricted notify;
	notify.isRestrictedByAAS = 0;
	SendPacket(clientHd, notify);

	if(mapIndex == MapIndex::PVP_TITAN_RUINS) {
		// EVENT 90: Release input lock
		{
			Sv::SN_RunClientLevelEventSeq seq;
			seq.needCompleteTriggerAckID = -1;
			seq.rootEventID = 90;
			seq.caller = 0;
			seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
			SendPacket(clientHd, seq);
		}
		// EVENT 270: Start battle timer
		{
			Sv::SN_RunClientLevelEventSeq seq;
			seq.needCompleteTriggerAckID = -1;
			seq.rootEventID = 270;
			seq.caller = 0;
			seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
			SendPacket(clientHd, seq);
		}
		// EVENT 401
		{
			Sv::SN_RunClientLevelEventSeq seq;
			seq.needCompleteTriggerAckID = -1;
			seq.rootEventID = 401;
			seq.caller = 0;
			seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
			SendPacket(clientHd, seq);
		}
		// EVENT 399
		{
			Sv::SN_RunClientLevelEventSeq seq;
			seq.needCompleteTriggerAckID = -1;
			seq.rootEventID = 399;
			seq.caller = 0;
			seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
			SendPacket(clientHd, seq);
		}
	}
	else {
		// DeathMatch game start events
		// EVENT: Release input lock
		{
			Sv::SN_RunClientLevelEventSeq seq;
			seq.needCompleteTriggerAckID = -1;
			seq.rootEventID = 163;
			seq.caller = 0;
			seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
			SendPacket(clientHd, seq);
		}
		// EVENT: Start the internal battle timer
		{
			Sv::SN_RunClientLevelEventSeq seq;
			seq.needCompleteTriggerAckID = -1;
			seq.rootEventID = 270;
			seq.caller = 0;
			seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
			SendPacket(clientHd, seq);
		}
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
	foreach_const(it, frameCur->playerList) {
		const Player& cur = *it;
		const Player* found = framePrev->FindPlayer(cur.index);
		if(!found) continue;
		const Player& prev = *found;

		// tagging
		if(cur.mainCharaID != prev.mainCharaID) {
			const ClientHandle upClientHd = cur.clientHd;
			const ActorUID upMainUID = cur.masters[cur.mainCharaID];
			const ActorUID upSubUID = cur.masters[cur.mainCharaID ^ 1];
			const ActorMaster* chara = frameCur->FindMaster(cur.masters[cur.mainCharaID]); // @Speed
			ASSERT(chara);
			const vec3 upPos = chara->pos;
			const RotationHumanoid upRot = chara->rotation;

			Sv::SN_GamePlayerTag tag;
			tag.result = 128;
			tag.attackerID = LocalActorID::INVALID;

			for(int pi = 0; pi < MAX_PLAYERS; pi++) {
				if(playerState[pi].cur < PlayerState::IN_GAME) continue;
				const ClientHandle clientHd = clientHandle[pi];

				tag.mainID = GetLocalActorID(clientHd, upMainUID);
				tag.subID = GetLocalActorID(clientHd, upSubUID);
				ASSERT(tag.mainID != LocalActorID::INVALID);
				ASSERT(tag.subID != LocalActorID::INVALID);

				SendPacket(clientHd, tag);

				if(clientHd != upClientHd) { // ignore self
					Sv::SN_GameLeaveActor leave;
					leave.objectID = tag.subID;
					SendPacket(clientHd, leave);

					// Sv::SN_GameEnterActor
					{
						PacketWriter<Sv::SN_GameEnterActor,512> packet;
						packet.Write<u8>(0x1); // excludedBits
						packet.Write<LocalActorID>(tag.mainID); // objectID
						packet.Write<float3>(v2f(upPos)); //  p3nPos
						packet.Write<RotationHumanoid>(RotConvertToMxm(upRot)); //  p3nDir
						packet.Write<float2>(v2f(vec2(0, 0))); //  p2nMoveDir
						packet.Write<float2>(v2f(vec2(0, 0))); //  p2nMoveUpperDir
						packet.Write<float3>(v2f(vec3(0))); //  p3nMoveTargetPos
						packet.Write<u8>(0); //  isBattleState
						packet.Write<f32>(620.0f); //  baseMoveSpeed (original hardcoded)
						packet.Write<ActionStateID>(ActionStateID::TAG_IN_EXECUTE_BEHAVIORSTATE); //  actionState
						packet.Write<i32>(0); //  aiTargetID

						// statSnapshot — use ORIGINAL hardcoded values (dynamic values break tag cooldown UI)
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
							Stat{ 14, 10 },    // original
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
							Stat{ 14, 100 },   // original
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

			// Minimal tag swap — same as original code (no extra packets)
		}

		if(cur.hasJumped && !prev.hasJumped) {
			const ActorUID actorUID = cur.masters[cur.mainCharaID];
			const ActorMaster* chara = frameCur->FindMaster(cur.masters[cur.mainCharaID]);  // @Speed
			ASSERT(chara);
			const f32 rotate = chara->rotation.bodyYaw;
			const vec2 moveDir = chara->moveDir;

			for(int pi = 0; pi < MAX_PLAYERS; pi++) {
				if(playerState[pi].cur < PlayerState::IN_GAME) continue;
				const ClientHandle clientHd = clientHandle[pi];

				PacketWriter<Sv::SA_ResultSpAction> packet;

				packet.Write<u8>(0x20); // excludedFieldBits
				packet.Write<i32>(0); // actionID
				packet.Write<LocalActorID>(GetLocalActorID(clientHd, actorUID));
				packet.Write<f32>(rotate);
				packet.Write<f32>(moveDir.x);
				packet.Write<f32>(moveDir.y);
				packet.Write<i32>(0); // errorType

				SendPacket(clientHd, packet);
			}
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
			ActionStateID action = cur.actionState;
			if(action == ActionStateID::INVALID) {
				action = ActionStateID::NONE_BEHAVIORSTATE;
			}

			Sv::SN_PlayerSyncMove sync;
			sync.destPos = v2f(cur.pos);
			sync.moveDir = v2f(cur.moveDir);
			sync.upperDir = { WorldYawToMxmYaw(cur.rotation.upperYaw), WorldPitchToMxmPitch(cur.rotation.upperPitch) };
			sync.nRotate = WorldYawToMxmYaw(cur.rotation.bodyYaw);
			sync.nSpeed = cur.speed;
			sync.flags = 0;
			sync.state = action;

			for(int pi = 0; pi < MAX_PLAYERS; pi++) {
				if(playerState[pi].cur < PlayerState::IN_GAME) continue;
				const ClientHandle clientHd = clientHandle[pi];
				if(clientHd == cur.clientHd) continue; // ignore self
				sync.characterID = GetLocalActorID(clientHd, cur.actorUID);
				SendPacket(clientHd, sync);
			}

			rotationUpdated = true;
		}

		// rotation
		const f32 rotEpsilon = 0.1f;
		if(!rotationUpdated &&
		   (fabs(cur.rotation.upperYaw - prev.rotation.upperYaw) > rotEpsilon ||
		   fabs(cur.rotation.upperPitch - prev.rotation.upperPitch) > rotEpsilon ||
		   fabs(cur.rotation.bodyYaw - prev.rotation.bodyYaw) > rotEpsilon))
		{
			Sv::SN_PlayerSyncTurn sync;
			sync.upperDir = { WorldYawToMxmYaw(cur.rotation.upperYaw), WorldPitchToMxmPitch(cur.rotation.upperPitch) };
			sync.nRotate = WorldYawToMxmYaw(cur.rotation.bodyYaw);

			for(int pi = 0; pi < MAX_PLAYERS; pi++) {
				if(playerState[pi].cur < PlayerState::IN_GAME) continue;
				const ClientHandle clientHd = clientHandle[pi];
				if(clientHd == cur.clientHd) continue; // ignore self

				sync.characterID = GetLocalActorID(clientHd, cur.actorUID);
				SendPacket(clientHd, sync);
			}
		}
	}


	// diff dynamics
	foreach_const(it, frameCur->dynamicList) {
		const ActorDynamic& cur = *it;
		const ActorDynamic* found = framePrev->FindDynamic(cur.actorUID);
		if(!found) continue;
		const ActorDynamic& prev = *found;

		// change action
		if(cur.action != prev.action) {
			// Client enum skips MAXTYPE_BEHAVIORSTATE — shift DYNAMIC_ states by -1
			const i32 clientAction = (i32)cur.action - 1;
			LOG("[Replication] Dynamic action change: UID=%u docID=%d action %d -> %d (client=%d)",
				(u32)cur.actorUID, (i32)cur.docID, (i32)prev.action, (i32)cur.action, clientAction);
			ClientList list;
			GetPlayersInGame(&list);
			foreach_const(clientHd, list) {
				LocalActorID localActorID = GetLocalActorID(*clientHd, cur.actorUID);
				if(localActorID == LocalActorID::INVALID) continue;

				PacketWriter<Sv::SN_ActionChangeLevelEvent,64> packet;
				packet.WriteVec<LocalActorID>(&localActorID, 1);
				packet.Write<i32>(clientAction); // action (adjusted for client enum)
				packet.Write<i64>((i64)TimeDiffMs(TimeRelNow())); // serverTime

				SendPacket(*clientHd, packet);
			}
		}
	}

	// skill casts
	foreach_const(it, frameCur->skillCastList) {
		const auto& cast = *it;

		if(cast.clientHd != ClientHandle::INVALID) {
			LocalActorID localActorID = GetLocalActorID(cast.clientHd, cast.casterUID);

			// SA_CastSkill
			{
				Sv::SA_CastSkill accept;
				accept.characterID = localActorID;
				accept.ret = 0;
				accept.skillIndex = cast.skillID;

				SendPacket(cast.clientHd, accept);
			}
		}

		for(int pi = 0; pi < MAX_PLAYERS; pi++) {
			if(playerState[pi].cur < PlayerState::IN_GAME) continue;
			const ClientHandle clientHd = clientHandle[pi];

			LocalActorID localCasterID = GetLocalActorID(clientHd, cast.casterUID);

			// SN_CastSkill
			{
				PacketWriter<Sv::SN_CastSkill,512> packet;

				packet.Write<LocalActorID>(localCasterID); // entityID
				packet.Write<i32>(0); // ret
				packet.Write<SkillID>(cast.skillID);
				packet.Write<u8>(0); // costLevel
				packet.Write<ActionStateID>(ActionStateID::INVALID); // TODO: is it always invalid?
				packet.Write<float3>(v2f(cast.castPos));

				packet.Write<u16>(cast.targetList.size()); // targetList_count
				foreach_const(t, cast.targetList) {
					packet.Write<LocalActorID>(GetLocalActorID(clientHd, *t));
				}

				packet.Write<u8>(1); // bSyncMyPosition
				packet.Write<float3>(v2f(cast.casterPos));
				packet.Write<float3>(v2f(cast.casterPos));
				packet.Write<float2>(v2f(cast.casterMoveDir));
				packet.Write<RotationHumanoid>(RotConvertToMxm(cast.casterRot));
				packet.Write<f32>(cast.casterSpeed);
				packet.Write<i32>((i64)TimeDiffMs(TimeRelNow()));

				SendPacket(clientHd, packet);
			}
		}
	}

	foreach_const(it, frameCur->skillExecList) {
		const auto& exec = *it;

		for(int pi = 0; pi < MAX_PLAYERS; pi++) {
			if(playerState[pi].cur < PlayerState::IN_GAME) continue;
			const ClientHandle clientHd = clientHandle[pi];

			LocalActorID localCasterID = GetLocalActorID(clientHd, exec.casterUID);

			// SN_ExecuteSkill
			{
				PacketWriter<Sv::SN_ExecuteSkill,512> packet;

				packet.Write<LocalActorID>(localCasterID); // entityID
				packet.Write<i32>(0); // ret
				packet.Write<SkillID>(exec.skillID);
				packet.Write<u8>(0); // costLevel
				packet.Write<ActionStateID>(exec.actionID);
				packet.Write<float3>(v2f(exec.castPos));

				packet.Write<u16>(exec.targetList.size()); // targetList_count
				foreach_const(t, exec.targetList) {
					packet.Write<LocalActorID>(GetLocalActorID(clientHd, *t));
				}

				packet.Write<u8>(0); // bSyncMyPosition
				packet.Write<float3>({});
				packet.Write<float3>({});
				packet.Write<float2>({});
				packet.Write<RotationHumanoid>({});
				packet.Write<f32>(0);
				packet.Write<i32>(0);

				packet.Write<f32>(0); // fSkillChargeDamageMultiplier

				if(exec.moveDuration != 0) {
					// graphMove
					packet.Write<u8>(1); // bApply
					packet.Write<float3>(v2f(exec.startPos)); // startPos
					packet.Write<float3>(v2f(exec.endPos)); // endPos
					packet.Write<f32>(exec.moveDuration); // durationTimeS
					packet.Write<f32>(glm::distance(exec.startPos, exec.endPos)); // originDistance
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

	LOG("[client%03d] Replication :: SendActorSpawn :: actorUID=%u localActorID=%u classType=%d masters0=%u masters1=%u",
		clientHd, (u32)actor.actorUID, (u32)localActorID, (i32)actor.classType, (u32)parent.masters[0], (u32)parent.masters[1]);

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
			// Resource stats based on THIS actor's hero class
			{
				const GameXmlContent& xml = GetGameXmlContent();
				i32 actorResType = 35; // default MANA
				f32 actorResMax = 1000;
				const HeroStats& actorStats = GetHeroStats(actor.classType);
				actorResType = actorStats.resourceStatType;
				if(actorResType == 17) actorResMax = actorStats.ENERGY > 0 ? actorStats.ENERGY : 100;
				else if(actorResType == 36) actorResMax = 100; // RAGE
				else if(actorResType == 56) actorResMax = 100; // BUBBLE
				else if(actorResType == 0) actorResMax = 0; // NONE
				else actorResMax = actorStats.MANA; // MANA

				LOG("[SPAWN] classType=%d resType=%d resMax=%.0f (17=%.0f 35=%.0f 36=%.0f)",
					(i32)actor.classType, actorResType, actorResMax,
					(actorResType == 17) ? actorResMax : 0.f,
					(actorResType == 35) ? actorResMax : 0.f,
					(actorResType == 36) ? actorResMax : 0.f);
				packet.Write(Stat{ 17, 0 }); // original
				packet.Write(Stat{ 18, 100 });
				packet.Write(Stat{ 20, 0 });
				packet.Write(Stat{ 21, 0 });
				packet.Write(Stat{ 22, 2 });
				packet.Write(Stat{ 23, 9 });
				packet.Write(Stat{ 29, 20 });
				packet.Write(Stat{ 31, 14 });
				packet.Write(Stat{ 35, 1000 }); // original
				packet.Write(Stat{ 36, 0 }); // original
				packet.Write(Stat{ 37, 120 });
			}
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

			// Use actual hero stats for main hero spawn
			{
				const HeroStats& heroStats = GetHeroStats(actor.classType);
				i32 resType = heroStats.resourceStatType;
				f32 resMax = heroStats.MANA;
				if(resType == 17) resMax = heroStats.ENERGY > 0 ? heroStats.ENERGY : 100;
				else if(resType == 36) resMax = 100;
				else if(resType == 56) resMax = 100;
				else if(resType == 0) resMax = 0;

				LOG("[SPAWN_STATS] classType=%d resType=%d resMax=%.0f HP=%.0f ATK=%.0f DEF=%.0f",
					(i32)actor.classType, resType, resMax, heroStats.HP, heroStats.ATTACK, heroStats.DEFENSE);

				packet.Write<u16>(26); // maxStats_count (MUST be 26 — 28 breaks client parsing)
				// maxStats: ORIGINAL hardcoded values (changing these breaks tag cooldown)
				packet.Write(Stat{ 0, heroStats.HP });       // HP from hero XML
				packet.Write(Stat{ 2, heroStats.ATTACK });   // ATK from hero XML
				packet.Write(Stat{ 5, 5 });           // original
				packet.Write(Stat{ 6, 124 });         // original
				packet.Write(Stat{ 7, 93.7846f });    // original
				packet.Write(Stat{ 9, 3 });           // original
				packet.Write(Stat{ 10, 150 });        // original
				packet.Write(Stat{ 13, 100 });        // original
				packet.Write(Stat{ 14, 101 });        // original
				packet.Write(Stat{ 15, 100 });        // original
				packet.Write(Stat{ 16, 1 });          // original
				packet.Write(Stat{ 18, 100 });        // original
				packet.Write(Stat{ 22, 2 });          // original
				packet.Write(Stat{ 23, 9 });          // original
				packet.Write(Stat{ 29, 20 });         // original
				packet.Write(Stat{ 31, 14 });         // original
				// Resource stat: use hero's ACTUAL resource type ID (35=MANA, 17=ENERGY, 36=RAGE, 56=BUBBLE)
				if(resType > 0) {
					packet.Write(Stat{ (u8)resType, resMax });
				} else {
					packet.Write(Stat{ 35, 0 }); // NONE hero: placeholder stat 35 with 0
				}
				packet.Write(Stat{ 37, 120 });        // original
				packet.Write(Stat{ 39, 5 });          // original
				packet.Write(Stat{ 42, 0.6f });       // original
				packet.Write(Stat{ 44, 15 });         // original
				packet.Write(Stat{ 52, 100 });        // original
				packet.Write(Stat{ 54, 15 });         // original
				packet.Write(Stat{ 55, 15 });         // original
				packet.Write(Stat{ 63, 3 });          // original
				packet.Write(Stat{ 64, 150 });        // original

				// curStats: 4 stats, same IDs as original
				packet.Write<u16>(4); // curStats_count
				packet.Write(Stat{ 0, heroStats.HP }); // HP
				packet.Write(Stat{ 37, 120 });          // UG
				if(resType > 0) {
					packet.Write(Stat{ (u8)resType, resMax }); // resource cur = full at spawn
				} else {
					packet.Write(Stat{ 35, 0 }); // NONE placeholder
				}
				packet.Write(Stat{ 2, heroStats.ATTACK }); // ATK
			}
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
			packet.Write<i32>(3 + parent.team); // faction
			packet.Write<ClassType>(actor.classType); // classType
			packet.Write<SkinIndex>(actor.skinIndex); // skinIndex
			packet.Write<i32>(0); // seed

			typedef Sv::SN_GameCreateActor::BaseStat::Stat Stat;

			// initStat — use actual hero stats for sub hero
			const HeroStats& subHeroStats2 = GetHeroStats(actor.classType);
			i32 srt2 = subHeroStats2.resourceStatType;
			f32 srm2 = subHeroStats2.MANA;
			if(srt2 == 17) srm2 = subHeroStats2.ENERGY > 0 ? subHeroStats2.ENERGY : 100;
			else if(srt2 == 36) srm2 = 100;
			else if(srt2 == 56) srm2 = 100;
			else if(srt2 == 0) srm2 = 0;

			LOG("[SPAWN_STATS_SUB] classType=%d resType=%d resMax=%.0f HP=%.0f ATK=%.0f",
				(i32)actor.classType, srt2, srm2, subHeroStats2.HP, subHeroStats2.ATTACK);

			packet.Write<u16>(26); // maxStats_count (MUST be 26)
			// Sub hero maxStats: ORIGINAL hardcoded (different layout from main!)
			packet.Write(Stat{ 0, subHeroStats2.HP });   // HP from hero XML
			packet.Write(Stat{ 2, subHeroStats2.ATTACK }); // ATK from hero XML
			packet.Write(Stat{ 5, 5 });           // original
			packet.Write(Stat{ 6, 192 });         // original
			packet.Write(Stat{ 7, 85.05f });      // original
			packet.Write(Stat{ 9, 3 });           // original
			packet.Write(Stat{ 10, 150 });        // original
			packet.Write(Stat{ 13, 100 });        // original
			packet.Write(Stat{ 14, 104.5 });      // original
			packet.Write(Stat{ 15, 100 });        // original
			packet.Write(Stat{ 16, 1 });          // original
			// Resource stat: use hero's ACTUAL resource type ID
			if(srt2 > 0) {
				packet.Write(Stat{ (u8)srt2, srm2 });
			} else {
				packet.Write(Stat{ 17, 0 }); // NONE hero: placeholder stat 17 with 0
			}
			packet.Write(Stat{ 18, 100 });        // original
			packet.Write(Stat{ 22, 2 });          // original
			packet.Write(Stat{ 23, 9 });          // original
			packet.Write(Stat{ 29, 20 });         // original
			packet.Write(Stat{ 31, 14 });         // original
			packet.Write(Stat{ 37, 120 });        // original
			packet.Write(Stat{ 41, 6 });          // original
			packet.Write(Stat{ 42, 0.6f });       // original
			packet.Write(Stat{ 46, 5 });          // original
			packet.Write(Stat{ 52, 100 });        // original
			packet.Write(Stat{ 54, 15 });         // original
			packet.Write(Stat{ 55, 15 });         // original
			packet.Write(Stat{ 63, 3 });          // original
			packet.Write(Stat{ 64, 15 });         // original

			f32 subResCur = srm2;
			if(srt2 == 56) subResCur = 0; // BUBBLE starts empty

			// curStats: ORIGINAL format (4 stats, fixed IDs)
			packet.Write<u16>(4); // curStats_count
			packet.Write(Stat{ 0, subHeroStats2.HP }); // HP
			packet.Write(Stat{ 37, 0 });     // UG = 0 for sub
			packet.Write(Stat{ 2, subHeroStats2.ATTACK }); // ATK
			if(srt2 > 0) {
				packet.Write(Stat{ (u8)srt2, subResCur }); // resource cur
			} else {
				packet.Write(Stat{ 17, 0 }); // NONE placeholder
			}
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

		// Look up per-class weapon from WEAPON.xml (first weaponID = base/normal tier)
		const GameXmlContent& content = GetGameXmlContent();
		const GameXmlContent::Master& master = content.GetMaster(actor.classType);
		i32 weaponDocIndex = 131135011; // fallback: LAUNCHER base weapon
		if(master.weaponIDs.size() > 0) {
			weaponDocIndex = (i32)master.weaponIDs[0];
			LOG("[client%03d] Replication :: EquipWeapon :: classType=%d weaponDocIndex=%d", clientHd, (i32)actor.classType, weaponDocIndex);
		}
		else {
			WARN("[client%03d] Replication :: EquipWeapon :: no weaponIDs for classType=%d, using fallback", clientHd, (i32)actor.classType);
		}

		packet.Write<LocalActorID>(localActorID); // characterID
		packet.Write<i32>(weaponDocIndex); // weaponDocIndex
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

	LOG("[client%03d] Replication :: SendActorNpcSpawn :: UID=%u local=%u doc=%d faction=%d pos=(%.0f,%.0f,%.0f)",
		clientID, (u32)actor.actorUID, (u32)localActorID, (i32)actor.docID, (i32)actor.faction,
		actor.pos.x, actor.pos.y, actor.pos.z);

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
				if(actor.hasHP) {
					const Stat defMax[] = { { 0, actor.maxHp } };
					const Stat defCur[] = { { 0, actor.hp } };
					packet.WriteVec(defMax, ARRAY_COUNT(defMax));
					packet.WriteVec(defCur, ARRAY_COUNT(defCur));
				} else {
					packet.Write<u16>(0); // maxStats
					packet.Write<u16>(0); // curStats
				}
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
		// Client enum skips MAXTYPE_BEHAVIORSTATE — all DYNAMIC_ states are shifted by -1
		packet.Write<i32>((i32)actor.action - 1); // actionState (adjusted for client enum)
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
				if(actor.hasHP) {
					const Stat defMax[] = { { 0, actor.maxHp } };
					const Stat defCur[] = { { 0, actor.hp } };
					packet.WriteVec(defMax, ARRAY_COUNT(defMax));
					packet.WriteVec(defCur, ARRAY_COUNT(defCur));
				} else {
					packet.Write<u16>(0); // maxStats
					packet.Write<u16>(0); // curStats
				}
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

void Replication::DbgSendActionChange(ClientHandle clientHd, ActorUID uid, i32 actionID)
{
	LocalActorID localActorID = GetLocalActorID(clientHd, uid);
	if(localActorID == LocalActorID::INVALID) return;

	PacketWriter<Sv::SN_ActionChangeLevelEvent,64> packet;
	packet.WriteVec<LocalActorID>(&localActorID, 1);
	packet.Write<i32>(actionID);
	packet.Write<i64>((i64)TimeDiffMs(TimeRelNow()));
	SendPacket(clientHd, packet);
}

void Replication::DbgSendDestroy(ClientHandle clientHd, ActorUID uid)
{
	SendActorDestroy(clientHd, uid);
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

		if(mapIndex == MapIndex::PVP_TITAN_RUINS) {
			packet.Write<SkillID>(SkillID(180000010)); // stageSkillIndex1 (stage skill)
			packet.Write<SkillID>(SkillID(180000110)); // stageSkillIndex2 (recall / B key)
		} else {
			packet.Write<SkillID>(SkillID::INVALID); // stageSkillIndex1
			packet.Write<SkillID>(SkillID::INVALID); // stageSkillIndex2
		}
		packet.Write<SkillID>(master.skillIDs[0]); // currentSkillSlot1
		packet.Write<SkillID>(master.skillIDs[1]); // currentSkillSlot2
		packet.Write<SkillID>(master.skillIDs.back()); // shirkSkillSlot

		SendPacket(clientHd, packet);
	}
}

void Replication::SendInitialFrame(ClientHandle clientHd)
{
	// SN_InitIngameModeInfo — must match map mode from the start
	{
		PacketWriter<Sv::SN_InitIngameModeInfo> packet;

		if(mapIndex == MapIndex::PVP_TITAN_RUINS) {
			packet.Write<i32>(60000); // transformationVotingPlayerCoolTimeByVotingFail
			packet.Write<i32>(10000); // transformationVotingTeamCoolTimeByTransformationEnd
			packet.Write<i32>(0);     // playerCoolTimeByTransformationEnd
			packet.Write<i32>(0);     // currentTransformationVotingPlayerCoolTimeByVotingFail
			packet.Write<i32>(0);     // currentTransformationVotingTeamCoolTimeByTransformationEnd
			packet.Write<i32>(0);     // currentPlayerCoolTimeByTransformationEnd
			packet.Write<i32>(20);    // chPropertyResetCoolTime
			packet.Write<u8>(5);      // transformationPieceCount
			packet.Write<u16>(3);     // titanDocIndexes_count
			packet.Write<i32>(105001102); packet.Write<i32>(105001100); packet.Write<i32>(105001101);
			packet.Write<u8>(0);      // nextTitanIndex
			packet.Write<u16>(0);     // listExceptionStat_count
		}
		else {
			packet.Write<i32>(0); // transformationVotingPlayerCoolTimeByVotingFail
			packet.Write<i32>(0); // transformationVotingTeamCoolTimeByTransformationEnd
			packet.Write<i32>(0); // playerCoolTimeByTransformationEnd
			packet.Write<i32>(0); // currentTransformationVotingPlayerCoolTimeByVotingFail
			packet.Write<i32>(0); // currentTransformationVotingTeamCoolTimeByTransformationEnd
			packet.Write<i32>(0); // currentPlayerCoolTimeByTransformationEnd
			packet.Write<i32>(20); // chPropertyResetCoolTime
			packet.Write<u8>(0);  // transformationPieceCount
			packet.Write<u16>(0); // titanDocIndexes_count
			packet.Write<u8>(0);  // nextTitanIndex
			packet.Write<u16>(0); // listExceptionStat_count
		}

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

void Replication::GetPlayersInGame(ClientList* list)
{
	for(int pi = 0; pi < MAX_PLAYERS; pi++) {
		if(playerState[pi].cur < PlayerState::IN_GAME) continue;
		const ClientHandle clientHd = clientHandle[pi];
		list->push_back(clientHd);
	}
}

// --- Combat system send functions ---

void Replication::SendUpdateStatToAll(ActorUID actorUID, u8 statType, f32 maxValue, f32 curValue)
{
	ClientList list;
	GetPlayersInGame(&list);
	foreach_const(clientHd, list) {
		SendUpdateStatToClient(*clientHd, actorUID, statType, maxValue, curValue);
	}
}

void Replication::SendUpdateStatToClient(ClientHandle clientHd, ActorUID actorUID, u8 statType, f32 maxValue, f32 curValue)
{
	LocalActorID localID = GetLocalActorID(clientHd, actorUID);
	if(localID == LocalActorID::INVALID) return;

	// SN_UpdateStat (62056) - 20 bytes
	// Client reads: characterID, _STAT_TYPE, _CUR, _MAX, _ReasonCode
	PacketWriter<Sv::SN_UpdateStat> packet;
	packet.Write<LocalActorID>(localID);  // characterID 4
	packet.Write<u8>(statType);            // _STAT_TYPE 1
	packet.Write<u8>(0);                   // pad
	packet.Write<u8>(0);                   // pad
	packet.Write<u8>(0);                   // pad
	packet.Write<f32>(curValue);           // _CUR 4 (current value FIRST)
	packet.Write<f32>(maxValue);           // _MAX 4 (max value SECOND)
	packet.Write<i32>(0);                  // param 4 = total 20
	SendPacket(clientHd, packet);
}

void Replication::SendBroadcastDamage(ActorUID attackerUID, ActorUID targetUID, i32 damage,
	const vec3& attackerPos, const vec3& attackerDir,
	const vec3& hitPos, const vec3& hitDir,
	i32 damageType, i32 skillDocID)
{
	// SN_BroadcastDamage (62235) — 123 bytes payload (127 on wire)
	// Format from client binary handler analysis (capture_ranked_test.log):
	//   damageSeqNum(i32), remoteID(i32), defID(i32), attID(i32), remoteDocIndex(i32),
	//   remotePos(vec3), remoteDir(vec3), remoteForceDir(vec3), hitPos(vec3), hitDir(vec3),
	//   damageType(i32), skillDocID(i32), nRagePoint(i32), nRelativeElement(i32),
	//   hitNodeName(str), damage(i32), partName(str), partDamage(i32),
	//   masterGroupingDamage(i32), optionalResultOfHit(i32)

	static i32 damageSeqCounter = 0;

	// remoteForceDir is opposite of attackerDir (knockback direction)
	vec3 forceDir = { -attackerDir.x, -attackerDir.y, -attackerDir.z };

	// hitNodeName = "__$body" (7 bytes UTF-8)
	static const char hitNodeName[] = "__$body";
	static const u16 hitNodeNameLen = 7;

	// partName = "" (empty)
	static const u16 partNameLen = 0;

	ClientList list;
	GetPlayersInGame(&list);
	foreach_const(clientHd, list) {
		LocalActorID defLocal = GetLocalActorID(*clientHd, targetUID);
		LocalActorID attLocal = GetLocalActorID(*clientHd, attackerUID);
		if(defLocal == LocalActorID::INVALID) continue;

		PacketWriter<Sv::SN_BroadcastDamage, 256> packet;
		packet.Write<i32>(damageSeqCounter);           // damageSeqNum
		packet.Write<i32>(0);                           // remoteID (0 for direct attacks, negative for projectiles)
		packet.Write<i32>((i32)defLocal);               // defID (victim LocalActorID)
		packet.Write<i32>((i32)attLocal);               // attID (attacker LocalActorID)
		packet.Write<i32>(0);                           // remoteDocIndex (0 for direct)
		packet.Write<float3>(v2f(attackerPos));          // remotePos
		packet.Write<float3>(v2f(attackerDir));          // remoteDir
		packet.Write<float3>(v2f(forceDir));             // remoteForceDir
		packet.Write<float3>(v2f(hitPos));               // hitPos
		packet.Write<float3>(v2f(hitDir));               // hitDir
		packet.Write<i32>(damageType);                  // damageType (0=normal, 2=crit)
		packet.Write<i32>(skillDocID);                  // skillDocID (-1 for auto-attack)
		packet.Write<i32>(0);                           // nRagePoint
		packet.Write<i32>(0);                           // nRelativeElement
		packet.Write<u16>(hitNodeNameLen);               // hitNodeName length
		packet.WriteRaw(hitNodeName, hitNodeNameLen);    // hitNodeName bytes (UTF-8)
		packet.Write<i32>(damage);                      // damage value
		packet.Write<u16>(partNameLen);                  // partName length (empty)
		packet.Write<i32>(0);                           // partDamage
		packet.Write<i32>(0);                           // masterGroupingDamage
		packet.Write<i32>(0);                           // optionalResultOfHit
		SendPacket(*clientHd, packet);
	}

	damageSeqCounter++;
}

void Replication::SendDeadAck(ActorUID victimUID, ActorUID killerUID, i32 victimDocIndex)
{
	ClientList list;
	GetPlayersInGame(&list);
	foreach_const(clientHd, list) {
		LocalActorID victimLocal = GetLocalActorID(*clientHd, victimUID);
		LocalActorID killerLocal = GetLocalActorID(*clientHd, killerUID);
		if(victimLocal == LocalActorID::INVALID) continue;
		if(killerLocal == LocalActorID::INVALID) {
			LOG("WARNING(SendDeadAck): killer LocalActorID not found for client %x, using victim as self-kill", *clientHd);
			killerLocal = victimLocal; // fallback: treat as self-kill so death screen still shows
		}

		// SN_DeadAck (62062) — 21 bytes payload, 25 on wire
		// Fields: sourceID, targetID, targetDocIndex, dwRemoteID, dwRemoteDocIndex, isPast
		// NOTE: sourceID and targetID may need to be swapped per client expectation
		// Official format: sourceID=killer, targetID=victim
		Sv::SN_DeadAck packet;
		// Use victimLocal as sourceID to always trigger death screen on the victim's client
		// TODO: figure out the correct sourceID for bot kills
		packet.sourceID = (i32)victimLocal;       // victim (self) — ensures death screen shows
		packet.targetID = (i32)victimLocal;       // victim
		packet.targetDocIndex = victimDocIndex;   // creature doc index (100000000 + classType)
		packet.dwRemoteID = 0;                    // not used for basic kills
		packet.dwRemoteDocIndex = 0;              // not used for basic kills
		packet.isPast = 0;
		SendPacket(*clientHd, packet);
	}
}

void Replication::SendDeadDamageInfo(ClientHandle victimClientHd, ActorUID victimUID,
	const DeadDamageAttacker& killer,
	const eastl::fixed_vector<DeadDamageAttacker, 8, false>& others,
	i32 durationTimeMs)
{
	// SN_DEAD_DAMAGE_INFO (62063) — variable-size, sent only to victim
	// Format: PST_DEAD_DAMAGE_INFO_LIST(killer) + u16(othersCount) + PST_DEAD_DAMAGE_INFO_LIST[others] + i32(durationTime)
	// Each PST_DEAD_DAMAGE_INFO_LIST: u16(vecCount) + entries[vecCount] + i32(attackerKey)
	// Each entry (18 bytes): i32(objectID) + i32(skillDocIndex) + i32(statusDocIndex) + i32(damage) + u8(isMon) + u8(dmgType)

	if(victimClientHd == ClientHandle::INVALID) return;

	PacketWriter<Sv::SN_DEAD_DAMAGE_INFO, 512> packet;

	// Helper lambda to write one PST_DEAD_DAMAGE_INFO_LIST
	auto writeAttackerList = [&](const DeadDamageAttacker& attacker) {
		u16 vecCount = (u16)attacker.entries.size();
		packet.Write<u16>(vecCount);
		foreach_const(e, attacker.entries) {
			// Resolve ActorUID to LocalActorID for this client
			LocalActorID localID = GetLocalActorID(victimClientHd, e->actorUID);
			i32 objectID = (localID != LocalActorID::INVALID) ? (i32)localID : e->objectID;
			packet.Write<i32>(objectID);
			packet.Write<i32>(e->skillDocIndex);      // skillDocIndex
			packet.Write<i32>(e->statusDocIndex);     // statusDocIndex (status icon)
			packet.Write<i32>(e->damage);
			packet.Write<u8>(e->attackerIsMonster);
			packet.Write<u8>(e->damageType);
		}
		packet.Write<i32>(attacker.attackerKey);
	};

	// Killer list
	writeAttackerList(killer);

	// Others
	u16 othersCount = (u16)others.size();
	packet.Write<u16>(othersCount);
	foreach_const(other, others) {
		writeAttackerList(*other);
	}

	// Duration time
	packet.Write<i32>(durationTimeMs);

	// Log each entry for debugging
	foreach_const(e, killer.entries) {
		LocalActorID lid = GetLocalActorID(victimClientHd, e->actorUID);
		LOG("[DDI] killer entry: objID=%d skillDoc=%d dmgType=%d damage=%d",
			(i32)lid, e->skillDocIndex, e->damageType, e->damage);
	}
	LOG("[DDI] SN_DEAD_DAMAGE_INFO: killerKey=%d, killerEntries=%d, others=%d, duration=%dms, totalSize=%d bytes",
		killer.attackerKey, (i32)killer.entries.size(), (i32)others.size(), durationTimeMs, packet.size);

	// Log packet size for comparison with official (official: 94-136 bytes)
	LOG("[DDI] packet payload size: %d bytes (official range: 94-136)", packet.size - 4);

	SendPacket(victimClientHd, packet);
}

void Replication::SendRespawnDelaytime(UserID usn, i32 delayMs)
{
	// SN_RespawnDelaytime (62399) — 8 bytes payload (12 on wire)
	// Fields: usn (UserID), respawnDelayTimeMS
	// Broadcast to ALL players (official capture shows all clients receive this)
	Sv::SN_RespawnDelaytime packet;
	packet.usn = (i32)usn;
	packet.delaytimeMs = delayMs;

	ClientList list;
	GetPlayersInGame(&list);
	foreach_const(clientHd, list) {
		SendPacket(*clientHd, packet);
	}
}

void Replication::SendRevivePlayerAtStartingPoint(UserID userID, ActorUID activeUID, ActorUID inactiveUID, const vec3& pos)
{
	ClientList list;
	GetPlayersInGame(&list);
	foreach_const(clientHd, list) {
		LocalActorID activeLocal = GetLocalActorID(*clientHd, activeUID);
		LocalActorID inactiveLocal = GetLocalActorID(*clientHd, inactiveUID);
		if(activeLocal == LocalActorID::INVALID) continue;

		Sv::SN_RevivePlayerAtStartingPoint packet;
		packet.usn = (i32)userID;
		packet.activeID = (i32)activeLocal;
		packet.inactiveID = (inactiveLocal != LocalActorID::INVALID) ? (i32)inactiveLocal : 0;
		packet.posX = pos.x;
		packet.posY = pos.y;
		packet.posZ = pos.z;
		packet.param = 0;
		SendPacket(*clientHd, packet);
	}
}

void Replication::SendPlayerSyncTeleport(ActorUID actorUID, const vec3& pos, const vec3& rot)
{
	ClientList list;
	GetPlayersInGame(&list);
	foreach_const(clientHd, list) {
		LocalActorID localID = GetLocalActorID(*clientHd, actorUID);
		if(localID == LocalActorID::INVALID) continue;

		Sv::SN_PlayerSyncTeleport packet;
		packet.objectID = localID;
		packet.pos = v2f(pos);
		packet.rot = v2f(rot);
		SendPacket(*clientHd, packet);
	}
}

void Replication::SendKillNotify(ActorUID killerUID, ActorUID victimUID, i32 param)
{
	ClientList list;
	GetPlayersInGame(&list);
	foreach_const(clientHd, list) {
		LocalActorID victimLocal = GetLocalActorID(*clientHd, victimUID);
		if(victimLocal == LocalActorID::INVALID) continue;

		// SN_KillNotify (62114) — 8 bytes payload (12 on wire)
		Sv::SN_KillNotify packet;
		packet.actorID = victimLocal;
		packet.param = param;
		SendPacket(*clientHd, packet);
	}
}

void Replication::SendPvpEventAnnouncement(i32 type, i32 param1, i32 param2, ActorUID killerUID, ActorUID victimUID)
{
	ClientList list;
	GetPlayersInGame(&list);
	foreach_const(clientHd, list) {
		LocalActorID killerLocal = GetLocalActorID(*clientHd, killerUID);
		LocalActorID victimLocal = GetLocalActorID(*clientHd, victimUID);

		// SN_PvpEventAnnouncement (62488) — 21 bytes payload (25 on wire)
		Sv::SN_PvpEventAnnouncement packet;
		packet.type = type;
		packet.param1 = param1;
		packet.param2 = param2;
		packet.killerActorID = (i32)killerLocal;
		packet.victimActorID = (i32)victimLocal;
		packet.extra = 0;
		SendPacket(*clientHd, packet);
	}
}

void Replication::SendCancelSkill(ActorUID actorUID, i32 skillIndex)
{
	ClientList list;
	GetPlayersInGame(&list);
	foreach_const(clientHd, list) {
		LocalActorID localID = GetLocalActorID(*clientHd, actorUID);
		if(localID == LocalActorID::INVALID) continue;

		Sv::SN_CancelSkill packet;
		packet.objectID = localID;
		packet.skillIndex = skillIndex;
		packet.unk = 0;
		SendPacket(*clientHd, packet);
	}
}

void Replication::SendTagCooltime(ActorUID actorUID, i32 cooltimeMs)
{
	// SN_UPDATE_GAME_PLAYER_TAG_COOLTIME (62114) — 8 bytes payload
	// Fields: playerID (LocalActorID) + tagCooltimeMS (milliseconds)
	ClientList list;
	GetPlayersInGame(&list);
	foreach_const(clientHd, list) {
		LocalActorID localID = GetLocalActorID(*clientHd, actorUID);
		if(localID == LocalActorID::INVALID) continue;

		Sv::SN_KillNotify packet;
		packet.actorID = localID;
		packet.param = cooltimeMs;
		SendPacket(*clientHd, packet);

		LOG("[TAG_COOL] Sent 62114 to client%x: localID=%u cooltime=%dms",
			*clientHd, (u32)localID, cooltimeMs);
	}
}

void Replication::SendRemoteSyncCreateToAll(ActorUID casterUID, i32 remoteDocIndex, i32 remoteSeedID,
	const vec3& firePos, f32 fireYaw, const vec3& targetPos, i32 scale, u8 fireObjectType,
	i32 lifeTimeMs, ActorUID targetUID)
{
	// SN_RemoteSyncCreateFromRemoteDoc (62386) — spawns a VFX entity
	// Format from client binary handler (sv.py serialize_62386):
	//   ParentEntity(i32), OwnerEntity(i32), RemoteSeedID(i32),
	//   FirePosition(vec3), FireRotation(3x u16 compressed angle),
	//   TargetPosition(vec3), FireObject(wstr), Scale(u16),
	//   LifeTime(i32), HitInvalidTarget(i32), RemoteDocIndex(i32),
	//   TargetEntity(i32), RemoteCreateFlags(u8)

	// Wide string for FireObject (UTF-16LE) — from ActionBase.xml NodeName
	static const wchar fireObj_Skill[] = L"Skill_Fire_Dummy";
	static const wchar fireObj_Attack[] = L"Attack_Fire_Dummy";
	static const wchar fireObj_Scene[] = L"Scene Root";
	static const wchar fireObj_Skill1[] = L"Skill_1_Fire_Dummy"; // tower/NPC skill

	const wchar* fireObject = fireObj_Skill;
	u16 fireObjectLen = 16; // "Skill_Fire_Dummy" = 16 chars
	if(fireObjectType == 1) { fireObject = fireObj_Attack; fireObjectLen = 17; }
	else if(fireObjectType == 2) { fireObject = fireObj_Scene; fireObjectLen = 10; }
	else if(fireObjectType == 3) { fireObject = fireObj_Skill1; fireObjectLen = 18; }

	// VFX angle: use same conversion as movement (WorldYawToMxmYaw)
	const f32 PI2 = 6.2831853f;
	f32 vfxYaw = WorldYawToMxmYaw(fireYaw);
	while(vfxYaw < 0) vfxYaw += PI2;
	while(vfxYaw >= PI2) vfxYaw -= PI2;
	u16 compressedYaw = (u16)(vfxYaw / PI2 * 65535.0f);

	LOG("[VFX_DIAG] PACKET62386: caster=%u fireYaw=%.4f vfxYaw=%.4f compressedYaw=%u firePos=(%.1f,%.1f,%.1f) targetPos=(%.1f,%.1f,%.1f) fireObj=%d",
		(u32)casterUID, fireYaw, vfxYaw, (u32)compressedYaw, firePos.x, firePos.y, firePos.z, targetPos.x, targetPos.y, targetPos.z, fireObjectType);

	ClientList list;
	GetPlayersInGame(&list);
	foreach_const(clientHd, list) {
		LocalActorID casterLocal = GetLocalActorID(*clientHd, casterUID);
		if(casterLocal == LocalActorID::INVALID) continue;

		PacketWriter<Sv::SN_RemoteSyncCreateFromRemoteDoc, 256> packet;
		packet.Write<i32>((i32)casterLocal);                     // ParentEntity
		packet.Write<i32>((i32)casterLocal);                     // OwnerEntity (same as parent for self-cast)
		packet.Write<i32>(remoteSeedID);                         // RemoteSeedID (0xC0000000 + counter)
		packet.Write<f32>(firePos.x);                            // FirePosition.x
		packet.Write<f32>(firePos.y);                            // FirePosition.y
		packet.Write<f32>(firePos.z);                            // FirePosition.z
		packet.Write<u16>(0);                                    // FireRotationX (pitch, 0)
		packet.Write<u16>(0);                                    // FireRotationY (roll, 0)
		packet.Write<u16>(compressedYaw);                        // FireRotationZ (yaw, compressed)
		packet.Write<f32>(targetPos.x);                          // TargetPosition.x
		packet.Write<f32>(targetPos.y);                          // TargetPosition.y
		packet.Write<f32>(targetPos.z);                          // TargetPosition.z
		packet.WriteStringObj(fireObject, fireObjectLen);         // FireObject (wstr: u16 len + UTF-16LE)
		packet.Write<u16>((u16)scale);                           // Scale (100 = 1.0x)
		packet.Write<i32>(lifeTimeMs);                           // LifeTime in ms (0 = infinite/default)
		packet.Write<i32>(0);                                    // HitInvalidTarget (0)
		packet.Write<i32>(remoteDocIndex);                       // RemoteDocIndex (raw doc ID)
		// TargetEntity — needed for homing projectiles (FOLLOW_TARGET behavior)
		if(targetUID != ActorUID::INVALID) {
			LocalActorID targetLocal = GetLocalActorID(*clientHd, targetUID);
			packet.Write<i32>(targetLocal != LocalActorID::INVALID ? (i32)targetLocal : 0);
		} else {
			packet.Write<i32>(0);
		}
		packet.Write<u8>(0);                                     // RemoteCreateFlags (0)
		SendPacket(*clientHd, packet);
	}
}

void Replication::SendRemoteActivatedToAll(ActorUID casterUID, i32 remoteSeedID,
	ActorUID targetUID, i32 penetrationCount, const vec3& hitPos)
{
	// SN_RemoteActivated (62237) — hit confirmation for remote VFX
	// Official format from sv.py + capture_ranked_test.log:
	//   remoteID(i32)          = RemoteSeedID (0xC0000000 | counter)
	//   defID(i32)             = target LocalActorID being hit
	//   penetrationCount(i32)  = penetration/hit counter (10 for player hits in capture)
	//   remotePos(vec3)        = hit position
	// Total: 24 bytes payload = 28 bytes with header

	ClientList list;
	GetPlayersInGame(&list);
	foreach_const(clientHd, list) {
		LocalActorID casterLocal = GetLocalActorID(*clientHd, casterUID);
		if(casterLocal == LocalActorID::INVALID) continue;

		// Resolve target to local actor ID for this client
		i32 targetLocal = 0;
		if(targetUID != ActorUID::INVALID) {
			LocalActorID tl = GetLocalActorID(*clientHd, targetUID);
			targetLocal = (tl != LocalActorID::INVALID) ? (i32)tl : 0;
		}

		PacketWriter<Sv::SN_RemoteActivated, 64> packet;
		packet.Write<i32>(remoteSeedID);                         // remoteID (seed)
		packet.Write<i32>(targetLocal);                          // defID (target entity)
		packet.Write<i32>(penetrationCount);                     // penetrationCount
		packet.Write<f32>(hitPos.x);                             // remotePos.x
		packet.Write<f32>(hitPos.y);                             // remotePos.y
		packet.Write<f32>(hitPos.z);                             // remotePos.z
		SendPacket(*clientHd, packet);
	}
}

void Replication::SendChangeBattleState(ActorUID actorUID, bool inBattle, f32 baseMoveSpeed)
{
	ClientList list;
	GetPlayersInGame(&list);
	foreach_const(clientHd, list) {
		LocalActorID localID = GetLocalActorID(*clientHd, actorUID);
		if(localID == LocalActorID::INVALID) continue;

		Sv::SN_ChangeBattleState packet;
		packet.objectID = localID;
		packet.isBattleState = inBattle ? 1 : 0;
		packet.baseMoveSpeed = baseMoveSpeed;
		SendPacket(*clientHd, packet);
	}
}

void Replication::SendAddStatusToAll(i32 statusID, ActorUID targetUID, ActorUID casterUID,
	f32 durationTime, u8 overlapCount, u8 customValue)
{
	// SN_AddStatus (62230) — 69 bytes (excludedFieldBits=0, with defeat fields zeroed)
	// Official format: defeat fields with dist=0 prevents client displacement
	ClientList list;
	GetPlayersInGame(&list);

	// Get target position for defeat fields
	vec3 targetPos = vec3(0);
	Frame* frame = framePrev->playerList.empty() ? frameCur : framePrev;
	const ActorMaster* targetMaster = frame->FindMaster(targetUID);
	if(targetMaster) targetPos = targetMaster->pos;

	foreach_const(clientHd, list) {
		LocalActorID targetLocal = GetLocalActorID(*clientHd, targetUID);
		if(targetLocal == LocalActorID::INVALID) continue;

		i32 casterLocal = 0;
		if(casterUID != ActorUID::INVALID) {
			LocalActorID cl = GetLocalActorID(*clientHd, casterUID);
			casterLocal = (cl != LocalActorID::INVALID) ? (i32)cl : 0;
		}

		PacketWriter<Sv::SN_AddStatus, 80> packet;
		packet.Write<u8>(0);                          // excludedFieldBits = 0 (include defeat fields)
		packet.Write<f32>(1.0f);                      // totalRatio
		packet.Write<f32>(targetPos.x);               // defeatFrom.x
		packet.Write<f32>(targetPos.y);               // defeatFrom.y
		packet.Write<f32>(targetPos.z);               // defeatFrom.z
		packet.Write<f32>(targetPos.x);               // defeatTo.x (same = no displacement)
		packet.Write<f32>(targetPos.y);               // defeatTo.y
		packet.Write<f32>(targetPos.z);               // defeatTo.z
		packet.Write<f32>(0);                         // defeatDist (0 = no movement)
		packet.Write<f32>(0);                         // defeatOriginDist
		packet.Write<f32>(0);                         // defeatDurationTime
		packet.Write<i32>(statusID);                  // statusID
		packet.Write<u8>(1);                          // bEnabled
		packet.Write<i32>((i32)targetLocal);          // targetID
		packet.Write<i32>(casterLocal);               // casterID
		packet.Write<u8>(0);                          // isOverlap
		packet.Write<u8>(overlapCount);               // overlapCount
		packet.Write<u8>(customValue);                // customValue
		packet.Write<f32>(durationTime);              // durationTime (seconds)
		packet.Write<i32>(0);                         // elapsedTime
		SendPacket(*clientHd, packet);
	}
}

void Replication::SendNotifyPcDetailInfosForClient(ClientHandle clientHd, i32 excludeTeam)
{
	// Send enemy player info to a SINGLE client (for death info screen)
	Frame* frame = framePrev->playerList.empty() ? frameCur : framePrev;

	PacketWriter<Sv::SN_NotifyPcDetailInfos, 2048> packet;

	u16 enemyCount = 0;
	foreach_const(pl, frame->playerList) {
		if(pl->team != excludeTeam) enemyCount++;
	}
	packet.Write<u16>(enemyCount);

	foreach_const(pl, frame->playerList) {
		const Player& p = *pl;
		if(p.team == excludeTeam) continue;

		const ActorMaster* main = frame->FindMaster(p.masters[0]);
		const ActorMaster* sub = frame->FindMaster(p.masters[1]);
		if(!main || !sub) continue;

		packet.Write<UserID>(p.userID);
		packet.Write<LocalActorID>(GetLocalActorID(clientHd, main->actorUID));
		packet.Write<CreatureIndex>(CreatureIndex(100000000 + (i32)main->classType));
		packet.Write<ClassType>(main->classType);
		packet.Write<i32>((i32)p.hp);
		packet.Write<i32>((i32)p.maxHp);
		packet.Write<LocalActorID>(GetLocalActorID(clientHd, sub->actorUID));
		packet.Write<CreatureIndex>(CreatureIndex(100000000 + (i32)sub->classType));
		packet.Write<ClassType>(sub->classType);
		packet.Write<i32>((i32)p.hp);
		packet.Write<i32>((i32)p.maxHp);
		packet.Write<i32>(0); // remainTagCooltimeMS
		packet.Write<u8>(0); // canCastSkillSlotUG
	}

	if(enemyCount > 0) {
		SendPacket(clientHd, packet);
	}
}

void Replication::SendNotifyPcDetailInfosAllForClient(ClientHandle clientHd)
{
	// Send ALL players (same-team + enemies) to a SINGLE client
	// Same-team included first so squad GFx state is correct
	Frame* frame = framePrev->playerList.empty() ? frameCur : framePrev;

	PacketWriter<Sv::SN_NotifyPcDetailInfos, 2048> packet;
	packet.Write<u16>((u16)frame->playerList.size());

	foreach_const(pl, frame->playerList) {
		const Player& p = *pl;
		const ActorMaster* main = frame->FindMaster(p.masters[0]);
		const ActorMaster* sub = frame->FindMaster(p.masters[1]);
		if(!main || !sub) continue;

		packet.Write<UserID>(p.userID);
		packet.Write<LocalActorID>(GetLocalActorID(clientHd, main->actorUID));
		packet.Write<CreatureIndex>(CreatureIndex(100000000 + (i32)main->classType));
		packet.Write<ClassType>(main->classType);
		packet.Write<i32>((i32)p.hp);
		packet.Write<i32>((i32)p.maxHp);
		packet.Write<LocalActorID>(GetLocalActorID(clientHd, sub->actorUID));
		packet.Write<CreatureIndex>(CreatureIndex(100000000 + (i32)sub->classType));
		packet.Write<ClassType>(sub->classType);
		packet.Write<i32>((i32)p.hp);
		packet.Write<i32>((i32)p.maxHp);
		packet.Write<i32>(0); // remainTagCooltimeMS
		packet.Write<u8>(0); // canCastSkillSlotUG
	}

	SendPacket(clientHd, packet);
}

void Replication::SendRemoveStatusToAll(i32 statusID, ActorUID targetUID, ActorUID casterUID)
{
	// SN_RemoveStatus (62232) — always 12 bytes
	ClientList list;
	GetPlayersInGame(&list);
	foreach_const(clientHd, list) {
		LocalActorID targetLocal = GetLocalActorID(*clientHd, targetUID);
		if(targetLocal == LocalActorID::INVALID) continue;

		i32 casterLocal = 0;
		if(casterUID != ActorUID::INVALID) {
			LocalActorID cl = GetLocalActorID(*clientHd, casterUID);
			casterLocal = (cl != LocalActorID::INVALID) ? (i32)cl : 0;
		}

		Sv::SN_RemoveStatus packet;
		packet.statusID = statusID;
		packet.targetID = (i32)targetLocal;
		packet.casterID = casterLocal;
		SendPacket(*clientHd, packet);
	}
}

void Replication::SendNotifyPcDetailInfos()
{
	ClientList list;
	GetPlayersInGame(&list);

	// Use framePrev since frameCur may be empty (cleared at start of frame)
	Frame* frame = framePrev->playerList.empty() ? frameCur : framePrev;

	foreach_const(clientHd, list) {
		PacketWriter<Sv::SN_NotifyPcDetailInfos, 2048> packet;

		// Send ALL players (both teams)
		packet.Write<u16>((u16)frame->playerList.size());

		foreach_const(pl, frame->playerList) {
			const Player& p = *pl;
			const ActorMaster* main = frame->FindMaster(p.masters[0]);
			const ActorMaster* sub = frame->FindMaster(p.masters[1]);
			if(!main || !sub) continue;

			packet.Write<UserID>(p.userID);
			// mainPC
			packet.Write<LocalActorID>(GetLocalActorID(*clientHd, main->actorUID));
			packet.Write<CreatureIndex>(CreatureIndex(100000000 + (i32)main->classType));
			packet.Write<ClassType>(main->classType);
			packet.Write<i32>(2400); // hp  (TODO: use actual)
			packet.Write<i32>(2400); // maxHp
			// subPC
			packet.Write<LocalActorID>(GetLocalActorID(*clientHd, sub->actorUID));
			packet.Write<CreatureIndex>(CreatureIndex(100000000 + (i32)sub->classType));
			packet.Write<ClassType>(sub->classType);
			packet.Write<i32>(2400); // hp
			packet.Write<i32>(2400); // maxHp

			packet.Write<i32>(0); // remainTagCooltimeMS
			packet.Write<u8>(0); // canCastSkillSlotUG
		}

		SendPacket(*clientHd, packet);
	}
}

void Replication::SendNotifyPcDetailInfosEnemyOnly()
{
	// Send SN_NotifyPcDetailInfos with ONLY enemy team players
	// This provides enemy portraits for death info without overwriting squad GFx
	ClientList list;
	GetPlayersInGame(&list);

	Frame* frame = framePrev->playerList.empty() ? frameCur : framePrev;

	foreach_const(clientHd, list) {
		// Find this client's team
		const i32 clientID = playerMap.at(*clientHd);
		i32 clientTeam = -1;
		foreach_const(pl, frame->playerList) {
			if(pl->clientHd == *clientHd) {
				clientTeam = pl->team;
				break;
			}
		}
		if(clientTeam == -1) continue;

		PacketWriter<Sv::SN_NotifyPcDetailInfos, 2048> packet;

		// Count ENEMY players only
		u16 enemyCount = 0;
		foreach_const(pl, frame->playerList) {
			if(pl->team != clientTeam) enemyCount++;
		}
		packet.Write<u16>(enemyCount);

		foreach_const(pl, frame->playerList) {
			const Player& p = *pl;
			if(p.team == clientTeam) continue; // skip same team

			const ActorMaster* main = frame->FindMaster(p.masters[0]);
			const ActorMaster* sub = frame->FindMaster(p.masters[1]);
			if(!main || !sub) continue;

			packet.Write<UserID>(p.userID);
			packet.Write<LocalActorID>(GetLocalActorID(*clientHd, main->actorUID));
			packet.Write<CreatureIndex>(CreatureIndex(100000000 + (i32)main->classType));
			packet.Write<ClassType>(main->classType);
			packet.Write<i32>(2400);
			packet.Write<i32>(2400);
			packet.Write<LocalActorID>(GetLocalActorID(*clientHd, sub->actorUID));
			packet.Write<CreatureIndex>(CreatureIndex(100000000 + (i32)sub->classType));
			packet.Write<ClassType>(sub->classType);
			packet.Write<i32>(2400);
			packet.Write<i32>(2400);
			packet.Write<i32>(0); // remainTagCooltimeMS
			packet.Write<u8>(0); // canCastSkillSlotUG
		}

		SendPacket(*clientHd, packet);
	}
}

void Replication::SendActionStateBroadcast(ActorUID actorUID, ActionStateID state, i32 param1, i32 param2)
{
	ClientList list;
	GetPlayersInGame(&list);
	foreach_const(clientHd, list) {
		LocalActorID localID = GetLocalActorID(*clientHd, actorUID);
		if(localID == LocalActorID::INVALID) continue;

		PacketWriter<Sv::SN_PlayerSyncActionStateOnly> packet;
		packet.Write<LocalActorID>(localID);
		packet.Write<ActionStateID>(state);
		packet.Write<i32>(param1);
		packet.Write<i32>(param2);
		packet.Write<f32>(0); // rotate
		packet.Write<f32>(0); // upperRotate

		// graphMove - no move
		packet.Write<u8>(0); // bApply
		packet.Write<float3>(float3()); // startPos
		packet.Write<float3>(float3()); // endPos
		packet.Write<f32>(0); // durationTimeS
		packet.Write<f32>(0); // originDistance

		SendPacket(*clientHd, packet);
	}
}

void Replication::SendPvpResult(ClientHandle clientHd, i32 gameEndReason, i32 playTimeMs, i32 winningTeam)
{
	// SN_PVP_RESULT (62079) - 81 bytes payload
	// Wire format from pvp_result_62079_decoded.txt (verified against official captures)
	PacketWriter<Sv::SN_PvpResult, 256> packet;

	packet.Write<i32>(gameEndReason);    // 6=time, 7=score_limit
	packet.Write<i32>(playTimeMs);       // game duration in ms

	// VEC(PST_PVP_RESULT) pvpResults - 2 entries (RED + BLUE)
	packet.Write<u16>(2);

	// Determine results: winningTeam 0=RED wins, 1=BLUE wins, -1=draw
	i32 redResult, blueResult;
	if(winningTeam == 0) {
		redResult = 1;   // WIN
		blueResult = 2;  // LOSE
	} else if(winningTeam == 1) {
		redResult = 2;   // LOSE
		blueResult = 1;  // WIN
	} else {
		redResult = 3;   // DRAW
		blueResult = 3;  // DRAW
	}

	packet.Write<i32>(3);           // teamType = RED
	packet.Write<i32>(redResult);   // pvpResultType
	packet.Write<i32>(4);           // teamType = BLUE
	packet.Write<i32>(blueResult);  // pvpResultType

	// PST_RESULT_REWARD (empty - no rewards)
	packet.Write<i32>(0);           // m_playerID
	packet.Write<i32>(0);           // m_baseRon
	packet.Write<u16>(0);           // bonusRon VEC count
	packet.Write<i32>(0);           // m_baseExp
	packet.Write<u16>(0);           // bonusExp VEC count
	packet.Write<u16>(0);           // rewardItems VEC count

	// ST_GUILD_REWARD (15 bytes, all zeros)
	u8 guildReward[15] = {0};
	packet.WriteRaw(guildReward, 15);

	// ST_REWARD_ITEM pcCafeBonusItem (12 bytes, empty sentinel)
	packet.Write<i32>(0);           // m_ItemType = 0
	packet.Write<i32>(-1);          // m_ItemIndex = -1 (0xFFFFFFFF)
	packet.Write<i32>(0);           // m_ItemAmount = 0

	// Tier/ranking fields
	packet.Write<u8>(6);            // tierGameResult (always 6 in official)
	packet.Write<u8>(0);            // tierTypeResult
	packet.Write<u16>(0);           // curTierPoint
	packet.Write<u16>(0);           // deltaTierPoint
	packet.Write<u16>(0);           // deltaGuildRp

	// VEC(i32) pcCafeUserIds (empty)
	packet.Write<u16>(0);

	SendPacket(clientHd, packet);
}

void Replication::SendPvpResultScoreDeathmatch(ClientHandle clientHd, const PlayerScoreData* scores, i32 scoreCount,
	i32 redKills, i32 blueKills)
{
	// SN_PVP_RESULT_SCORE_DEATHMATCH (62080)
	// Wire format from pvp_result_score_deathmatch_decoded.txt (verified against 9 official packets)
	// Uses framePrev since frameCur may be empty during GameOver

	Frame* frame = framePrev->playerList.empty() ? frameCur : framePrev;

	PacketWriter<Sv::SN_PvpResultScoreDeathmatch, 4096> packet;

	// --- HEADER: Team scores ---
	packet.Write<u16>(2);           // numTeams (VEC count for m_TeamScore)

	// Team RED (teamType=3)
	packet.Write<i32>(3);           // m_Team = RED
	packet.Write<i32>(redKills);    // m_TotalScore = team total kills

	// Team BLUE (teamType=4)
	packet.Write<i32>(4);           // m_Team = BLUE
	packet.Write<i32>(blueKills);   // m_TotalScore = team total kills

	// --- PLAYER LIST ---
	u16 numPlayers = (u16)frame->playerList.size();
	packet.Write<u16>(numPlayers);  // numPlayers (VEC count for m_PvPScore)

	foreach_const(pl, frame->playerList) {
		const Player& p = *pl;

		// Find matching score data
		i32 kills = 0, deaths = 0, assists = 0, score = 0;
		i32 totalDamageDealt = 0, totalDamageReceived = 0, highestKillStreak = 0;
		u8 disconnected = 0;
		for(i32 i = 0; i < scoreCount; i++) {
			if(scores[i].playerIndex == p.index) {
				kills = scores[i].kills;
				deaths = scores[i].deaths;
				assists = scores[i].assists;
				score = scores[i].score;
				disconnected = scores[i].disconnected ? 1 : 0;
				totalDamageDealt = scores[i].totalDamageDealt;
				totalDamageReceived = scores[i].totalDamageReceived;
				highestKillStreak = scores[i].highestKillStreak;
				break;
			}
		}

		// Per-player header
		packet.Write<i32>((i32)p.userID);    // m_playerID
		packet.WriteStringObj(p.name.data()); // m_nickName (u16 len + UTF-16)
		packet.Write<i32>(3 + p.team);        // m_teamType (3=RED, 4=BLUE)
		packet.Write<u8>(disconnected);        // m_Disconnected: MUST be 0 for K/D/A to show
		packet.Write<i32>(kills);              // m_TotalScore = total kills across both masters

		// Per-master entries (always 2: main + sub)
		packet.Write<u16>(2);                  // numMasters (VEC count for m_charScore)

		// Master 1 (main) - all K/D/A attributed here for now
		packet.Write<i32>(100000000 + (i32)p.mainClass);  // m_CreatureIndex
		packet.Write<i32>(150000);             // m_PlayTimeMS (placeholder, ~2.5 min)
		packet.Write<u8>((u8)kills);           // m_KillCount
		packet.Write<u8>((u8)highestKillStreak); // m_MaxContinuousKillCount
		packet.Write<u8>(0);                   // m_MaxMultiKillCount
		packet.Write<u8>((u8)deaths);          // m_DeadCount
		packet.Write<u8>((u8)assists);         // m_AssistCount
		// 8 x i32 damage stats
		packet.Write<i32>(totalDamageDealt);   // m_TotalDamageDealt
		packet.Write<i32>(totalDamageDealt);   // m_MaxDamageDealt (same as total for single-hero tracking)
		packet.Write<i32>(totalDamageDealt);   // m_TotalNormalDamageDealt (all damage counted as normal for now)
		packet.Write<i32>(0);                  // m_TotalSkillDamageDealt (TODO: separate skill vs auto-attack)
		packet.Write<i32>(totalDamageReceived); // m_TotalDamageTaken
		packet.Write<i32>(totalDamageReceived); // m_MaxDamageTaken (same as total for single-hero tracking)
		packet.Write<i32>(totalDamageReceived); // m_TotalNormalDamageTaken (all damage counted as normal for now)
		packet.Write<i32>(0);                  // m_TotalSkillDamageTaken (TODO: separate skill vs auto-attack)

		// Master 2 (sub) - zeros (no tag tracking yet)
		packet.Write<i32>(100000000 + (i32)p.subClass);   // m_CreatureIndex
		packet.Write<i32>(100000);             // m_PlayTimeMS (placeholder, ~1.7 min)
		packet.Write<u8>(0);                   // m_KillCount
		packet.Write<u8>(0);                   // m_MaxContinuousKillCount
		packet.Write<u8>(0);                   // m_MaxMultiKillCount
		packet.Write<u8>(0);                   // m_DeadCount
		packet.Write<u8>(0);                   // m_AssistCount
		// 8 x i32 damage stats (zeros)
		packet.Write<i32>(0);                  // m_TotalDamageDealt
		packet.Write<i32>(0);                  // m_MaxDamageDealt
		packet.Write<i32>(0);                  // m_TotalNormalDamageDealt
		packet.Write<i32>(0);                  // m_TotalSkillDamageDealt
		packet.Write<i32>(0);                  // m_TotalDamageTaken
		packet.Write<i32>(0);                  // m_MaxDamageTaken
		packet.Write<i32>(0);                  // m_TotalNormalDamageTaken
		packet.Write<i32>(0);                  // m_TotalSkillDamageTaken
	}

	SendPacket(clientHd, packet);
}

void Replication::SendAuthResultForRTB(ClientHandle clientHd)
{
	Sv::SA_AuthResult auth;
	auth.result = 91;
	SendPacket(clientHd, auth);
}

void Replication::SendReturnToCity(ClientHandle clientHd)
{
	Sv::SA_ReturnToCity packet;
	packet.errCode = 0;
	SendPacket(clientHd, packet);
}

void Replication::SendDoConnectChannelServer(ClientHandle clientHd)
{
	// Send SN_DoConnectChannelServer to redirect to hub
	PacketWriter<Sv::SN_DoConnectChannelServer, 256> packet;
	packet.Write<u16>(1); // count
	u8 ip[] = {127, 0, 0, 1};
	packet.WriteRaw(ip, 4);
	packet.Write<u16>(11900); // hub port
	packet.WriteStringObj(L"Lobby");
	packet.WriteStringObj(L"Player");
	packet.Write<i32>(0);
	packet.Write<i32>(0);
	SendPacket(clientHd, packet);
}

void Replication::SendTeamScoreToAll(i32 redKills, i32 blueKills, i32 redScore, i32 blueScore)
{
	// Uses SN_ScoreUpdate (62504) with usn=0 for team updates
	// Format: i32(0) + i32(teamType) + u16(count) + count*(i32 statID + i32 value)
	// RED=3, BLUE=4. statID: 0=Score, 1=Kills
	ClientList list;
	GetPlayersInGame(&list);

	foreach_const(clientHd, list) {
		// RED team (teamType=3)
		{
			PacketWriter<Sv::SN_ScoreUpdate, 64> packet;
			packet.Write<i32>(0);         // usn = 0 (team update)
			packet.Write<i32>(3);         // teamType RED
			packet.Write<u16>(2);         // 2 records
			packet.Write<i32>(1);         // statID = Kills
			packet.Write<i32>(redKills);
			packet.Write<i32>(0);         // statID = Score
			packet.Write<i32>(redScore);
			SendPacket(*clientHd, packet);
		}
		// BLUE team (teamType=4)
		{
			PacketWriter<Sv::SN_ScoreUpdate, 64> packet;
			packet.Write<i32>(0);         // usn = 0 (team update)
			packet.Write<i32>(4);         // teamType BLUE
			packet.Write<u16>(2);         // 2 records
			packet.Write<i32>(1);         // statID = Kills
			packet.Write<i32>(blueKills);
			packet.Write<i32>(0);         // statID = Score
			packet.Write<i32>(blueScore);
			SendPacket(*clientHd, packet);
		}
	}
}

void Replication::SendScoreUpdatePlayer(UserID usn, i32 statID, i32 value)
{
	// SN_SCORE_UPDATE (62504) — individual player stat update
	// Format: i32 usn + i32 teamType(-1) + u16 count + count*(i32 statID + i32 value)
	ClientList list;
	GetPlayersInGame(&list);
	foreach_const(clientHd, list) {
		PacketWriter<Sv::SN_ScoreUpdate, 64> packet;
		packet.Write<i32>((i32)usn);    // player USN
		packet.Write<i32>(-1);          // teamType = -1 (individual)
		packet.Write<u16>(1);           // 1 record
		packet.Write<i32>(statID);
		packet.Write<i32>(value);
		SendPacket(*clientHd, packet);
	}
}

void Replication::SendScoreUpdateTeam(i32 teamType, i32 statID, i32 value)
{
	// SN_SCORE_UPDATE (62504) — team stat update
	// Format: i32 usn(0) + i32 teamType + u16 count + count*(i32 statID + i32 value)
	ClientList list;
	GetPlayersInGame(&list);
	foreach_const(clientHd, list) {
		PacketWriter<Sv::SN_ScoreUpdate, 64> packet;
		packet.Write<i32>(0);           // usn = 0 (team update)
		packet.Write<i32>(teamType);    // 3=RED, 4=BLUE
		packet.Write<u16>(1);           // 1 record
		packet.Write<i32>(statID);
		packet.Write<i32>(value);
		SendPacket(*clientHd, packet);
	}
}

void Replication::SendCreateGroundItem(i32 groundItemID, i32 itemDocIndex, const vec3& pos)
{
	ClientList list;
	GetPlayersInGame(&list);
	foreach_const(clientHd, list) {
		PacketWriter<Sv::SN_CreateGroundItem, 128> packet;

		// Official capture format (55 bytes):
		packet.Write<u8>(0x00); // excludedFieldBits
		// startPos (vec3 float)
		packet.Write<f32>(pos.x);
		packet.Write<f32>(pos.y);
		packet.Write<f32>(pos.z);
		// destPos (vec3 float, same as startPos)
		packet.Write<f32>(pos.x);
		packet.Write<f32>(pos.y);
		packet.Write<f32>(pos.z);
		packet.Write<i32>(groundItemID); // itemID (0xC0000000 | sequential)
		packet.Write<i32>(0); // dropType
		packet.Write<i32>(itemDocIndex); // nIndex (130101000=UG, 130100060=HP, 130100150=Recharge)
		packet.Write<i32>(1); // count
		packet.Write<i32>(0); // ownerID
		packet.Write<i32>(0); // gettableType
		packet.Write<i32>(-1); // factionType
		packet.Write<u8>(0); // isPrivate
		packet.Write<u8>(1); // positionType

		SendPacket(*clientHd, packet);
	}
}

void Replication::SendDestroyGroundItem(i32 groundItemID)
{
	ClientList list;
	GetPlayersInGame(&list);
	foreach_const(clientHd, list) {
		Sv::SN_DestroyGroundItem packet;
		packet.groundItemID = groundItemID;
		SendPacket(*clientHd, packet);
	}
}
