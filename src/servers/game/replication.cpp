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
	owner.userId = 1;
	SendPacket(clientHd, owner);

	Sv::SN_LobbyStartGame lobby;
	lobby.stageType = StageType::PVP_GAME;
	SendPacket(clientHd, lobby);

	// SN_CityMapInfo
	Sv::SN_CityMapInfo cityMapInfo;
	cityMapInfo.CityMapID = stageIndex;
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

void Replication::SendChatWhisperConfirmToClient(ClientHandle senderClientHd, const wchar* destNick, const wchar* msg, ErrorType retval)
{
	PacketWriter<Sv::SA_WhisperSend> packet;

	packet.Write<ErrorType>(retval);
	packet.WriteStringObj(destNick);
	packet.WriteStringObj(msg);

	SendPacket(senderClientHd, packet);
}

void Replication::SendChatWhisperToClient(ClientHandle destClientHd, const wchar* senderName, const wchar* msg)
{
	PacketWriter<Sv::SN_WhisperReceived> packet;

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

		packet.Write<u16>(2);

		{
			Sv::SN_ProfileCharacters::Character chara;
			chara.characterID = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)player->mainClass);
			chara.creatureIndex = CreatureIndex(100000000 + (i32)player->mainClass);
			chara.skillSlot1 = player->skills[0];
			chara.skillSlot2 = player->skills[1];
			chara.classType = player->mainClass;
			chara.x = actorMain->pos.x;
			chara.y = actorMain->pos.y;
			chara.z = actorMain->pos.z;
			chara.characterType = player->mainCharacterType;
			chara.skinIndex = player->mainSkin;
			chara.weaponIndex = player->mainWeapon;
			chara.masterGearNo = player->mainMasterGearNo;
			packet.Write(chara);
		}

		{
			Sv::SN_ProfileCharacters::Character chara;
			chara.characterID = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)player->subClass);
			chara.creatureIndex = CreatureIndex(100000000 + (i32)player->subClass);
			chara.skillSlot1 = player->skills[2];
			chara.skillSlot2 = player->skills[3];
			chara.classType = player->subClass;
			chara.x = actorSub->pos.x;
			chara.y = actorSub->pos.y;
			chara.z = actorSub->pos.z;
			chara.characterType = player->subCharacterType;
			chara.skinIndex = player->subSkin;
			chara.weaponIndex = player->subWeapon;
			chara.masterGearNo = player->subMasterGearNo;
			packet.Write(chara);
		}

		SendPacket(clientHd, packet);
	}

	// SN_ProfileWeapons
	{
		PacketWriter<Sv::SN_ProfileWeapons,4096> packet;

		packet.Write<u16>(2);

		Sv::SN_ProfileWeapons::Weapon weap;
		weap.characterID = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)player->mainClass);
		weap.weaponType = content.WeaponTypeOf(player->mainClass, player->mainWeapon);
		weap.weaponIndex = player->mainWeapon;
		weap.grade = player->mainWeaponGrade;
		weap.isUnlocked = 1;
		weap.isActivated = 1;
		packet.Write(weap);

		weap.characterID = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)player->subClass);
		weap.weaponType = content.WeaponTypeOf(player->subClass, player->subWeapon);
		weap.weaponIndex = player->subWeapon;
		weap.grade = player->subWeaponGrade;
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
		packet.Write<u16>((u16)skillCount);

		auto writeSkills = [&](const GameXmlContent::Master& master) {
			const LocalActorID characterID = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)master.classType);
			for(int si = 0; si < (int)master.skillIDs.size(); si++) {
				packet.Write(characterID);
				packet.Write(master.skillIDs[si]);
				const u8 unlocked = (si < (int)master.skillUnlocked.size()) ? master.skillUnlocked[si] : (u8)1;
				packet.Write<u8>(unlocked);
				packet.Write<u8>(1);
				packet.Write<u16>(0);
			}
		};
		writeSkills(masterMain);
		writeSkills(masterSub);

		SendPacket(clientHd, packet);
	}

	// SN_ProfileMasterGears
	{
		PacketWriter<Sv::SN_ProfileMasterGears,4096> packet;

		packet.Write<u16>(0);

		SendPacket(clientHd, packet);
	}

	// SN_AccountEquipmentList
	{
		PacketWriter<Sv::SN_AccountEquipmentList,4096> packet;
		packet.Write<i32>(supportKitIndex);
		SendPacket(clientHd, packet);
	}

	// SN_Money
	{
		Sv::SN_Money money;
		money.nMoney = 0;
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

	{
		PacketWriter<Sv::SN_GameFieldReady,4096> packet;

		packet.Write<i32>(inGameID);
		packet.Write(gameType);
		packet.Write(areaIndex);
		packet.Write(stageIndex);
		packet.Write(GameDefinition::System);
		packet.Write<u8>((u8)frameCur->playerList.size());
		packet.Write<u8>(canEscape);
		packet.Write<u8>(isTrespass);
		packet.Write<u8>(player->team == 2);

		packet.Write<u16>(frameCur->playerList.size());

		foreach_const(pit, frameCur->playerList) {
			packet.Write(pit->userID);
			packet.WriteStringObj(pit->name.data());
			packet.Write<u8>(3 + pit->team);
			packet.Write<u8>(pit->clientHd == ClientHandle::INVALID);
		}

		packet.Write<u16>(frameCur->playerList.size());

		foreach_const(pit, frameCur->playerList) {
			const Player& p = *pit;

			packet.Write(p.userID);
			packet.Write<CreatureIndex>((CreatureIndex)(100000000 + (i32)p.mainClass));
			packet.Write<SkinIndex>(p.mainSkin);
			packet.Write<SkillID>(p.skills[0]);
			packet.Write<SkillID>(p.skills[1]);
			packet.Write<CreatureIndex>((CreatureIndex)(100000000 + (i32)p.subClass));
			packet.Write<SkinIndex>(p.subSkin);
			packet.Write<SkillID>(p.skills[2]);
			packet.Write<SkillID>(p.skills[3]);
			packet.Write<SkillID>(stageSkills[0]);
			packet.Write<SkillID>(stageSkills[1]);
			packet.Write<i32>(supportKitIndex);
			packet.Write<u8>(p.clientHd == ClientHandle::INVALID);
		}

		packet.Write<u16>(0);
		packet.Write<i32>(surrenderAbleTime);

		SendPacket(clientHd, packet);
	}
}

void Replication::SendPvpLoadingComplete(ClientHandle clientHd)
{
	const u32 playerIndex = playerMap.at(clientHd);
	const Player* player = frameCur->FindPlayer(playerIndex);
	ASSERT(player);

	{
		PacketWriter<Sv::SN_NotifyPcDetailInfos,1024> packet;

		packet.Write<u16>((u16)frameCur->playerList.size());

		foreach_const(pl, frameCur->playerList) {
			const Player& p = *pl;
			const ActorMaster* main = frameCur->FindMaster(p.masters[0]);
			const ActorMaster* sub = frameCur->FindMaster(p.masters[1]);
			ASSERT(main);
			ASSERT(sub);

			packet.Write<UserID>(p.userID);
			packet.Write<LocalActorID>(GetLocalActorID(clientHd, main->actorUID));
			packet.Write<CreatureIndex>(CreatureIndex(100000000 + (i32)main->classType));
			packet.Write<ClassType>(main->classType);
			packet.Write<i32>(main->hp);
			packet.Write<i32>(main->hpMax);
			packet.Write<LocalActorID>(GetLocalActorID(clientHd, sub->actorUID));
			packet.Write<CreatureIndex>(CreatureIndex(100000000 + (i32)sub->classType));

			packet.Write<ClassType>(sub->classType);
			packet.Write<i32>(sub->hp);
			packet.Write<i32>(sub->hpMax);

			packet.Write<i32>(0);
			packet.Write<u8>(0);
		}


		SendPacket(clientHd, packet);
	}

	{
		PacketWriter<Sv::SN_InitScoreBoard,4096> packet;

		packet.Write<u16>(frameCur->playerList.size());

		foreach_const(pl, frameCur->playerList) {
			const Player& p = *pl;
			const ActorMaster* main = frameCur->FindMaster(p.masters[0]);
			const ActorMaster* sub = frameCur->FindMaster(p.masters[1]);
			ASSERT(main);
			ASSERT(sub);
			ASSERT(p.team == 0 || p.team == 1);

			packet.Write<UserID>(p.userID);
			packet.WriteStringObj(p.name.data());
			packet.Write<TeamType>(p.team == 0 ? TeamType::RED : TeamType::BLUE);
			packet.Write<CreatureIndex>(CreatureIndex(100000000 + (i32)main->classType));
			packet.Write<CreatureIndex>(CreatureIndex(100000000 + (i32)sub->classType));
		}


		SendPacket(clientHd, packet);
	}


	SendPacketData<Sv::SA_LoadingComplete>(clientHd,  0, nullptr);
}


void Replication::SendGameReady(ClientHandle clientHd, i32 waitTime, i32 elapsed, UserID userId)
{
	Sv::SA_GameReady ready;
	ready.waitingTimeMS = waitTime;
	ready.serverTimestamp = (i64)TimeDiffMs(TimeRelNow());
	ready.readyElapsedMS = elapsed;
	SendPacket(clientHd, ready);

	Sv::SN_NotifyIngameSkillPoint notify;
	notify.userId = userId;

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


	// EVENT: Starts music?
	// Sv::SN_RunClientLevelEventSeq
	{
		Sv::SN_RunClientLevelEventSeq seq;
		seq.needCompleteTriggerAckID = -1;
		seq.rootEventID = 218;
		seq.caller = 0;
		seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
		SendPacket(clientHd, seq);
	}

	// EVENT: You have entered the Combat Arena, the match begins soon
	// Sv::SN_RunClientLevelEventSeq
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
	SendPacketData<Sv::SN_GameStart>(clientHd, 0, nullptr);

	Sv::SN_NotifyAasRestricted notify;
	notify.isRestrictedByAAS = 0;
	SendPacket(clientHd, notify);

	// EVENT: Release input lock (Data\Design\Level\PVP\PVP_DeathMatch\EVENTNODES\LEVELEVENT_CLIENT.XML)
	// Sv::SN_RunClientLevelEventSeq
	{
		Sv::SN_RunClientLevelEventSeq seq;
		seq.needCompleteTriggerAckID = -1;
		seq.rootEventID = 163;
		seq.caller = 0;
		seq.serverTime = (i64)TimeDiffMs(TimeRelNow());
		SendPacket(clientHd, seq);
	}


	// EVENT: Start the internal battle timer (240s)
	// -> will then call following sequence events (271, 272, 273, 274). "battle will end in 1min, 30s".
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
			const ActorUID upMainUID = cur.masters[cur.mainCharaID];
			const ActorUID upSubUID = cur.masters[cur.mainCharaID ^ 1];
			const ActorMaster* chara = frameCur->FindMaster(cur.masters[cur.mainCharaID]); // @Speed
			ASSERT(chara);

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

				// Retail behavior (Titan Ruins capture): remote viewers receive ONLY the
				// SN_GamePlayerTag swap announcement - both masters already exist client-side.
				// Sending a destructive GameLeaveActor here removes the player's visible actor
				// without a valid re-bind, making tagged-out bots disappear.
			}
		}

		if(cur.hasJumped && !prev.hasJumped) {
			const ActorUID actorUID = cur.masters[cur.mainCharaID];
			const ActorMaster* chara = frameCur->FindMaster(cur.masters[cur.mainCharaID]);  // @Speed
			ASSERT(chara);
			const f32 rotate = WorldYawToMxmYaw(chara->rotation.bodyYaw);
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
			sync.DestPos = v2f(cur.pos);
			sync.MoveDir = v2f(cur.moveDir);
			sync.UpperDir = { WorldYawToMxmYaw(cur.rotation.upperYaw), WorldPitchToMxmPitch(cur.rotation.upperPitch) };
			sync.nRotate = WorldYawToMxmYaw(cur.rotation.bodyYaw);
			sync.nSpeed = cur.speed;
			sync.flags = 0;
			sync.actionStateID = action;

			for(int pi = 0; pi < MAX_PLAYERS; pi++) {
				if(playerState[pi].cur < PlayerState::IN_GAME) continue;
				const ClientHandle clientHd = clientHandle[pi];
				if(clientHd == cur.clientHd) continue; // ignore self
				sync.entityID = GetLocalActorID(clientHd, cur.actorUID);
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
			sync.UpperDir = { WorldYawToMxmYaw(cur.rotation.upperYaw), WorldPitchToMxmPitch(cur.rotation.upperPitch) };
			sync.nRotate = WorldYawToMxmYaw(cur.rotation.bodyYaw);

			for(int pi = 0; pi < MAX_PLAYERS; pi++) {
				if(playerState[pi].cur < PlayerState::IN_GAME) continue;
				const ClientHandle clientHd = clientHandle[pi];
				if(clientHd == cur.clientHd) continue; // ignore self

				sync.entityID = GetLocalActorID(clientHd, cur.actorUID);
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
			ClientList list;
			GetPlayersInGame(&list);
			foreach_const(clientHd, list) {
				LocalActorID localActorID = GetLocalActorID(*clientHd, cur.actorUID);

				PacketWriter<Sv::SN_ActionChangeLevelEvent,64> packet;
				packet.WriteVec<LocalActorID>(&localActorID, 1);
				packet.Write(cur.action); // action
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
				accept.entity = localActorID;
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
				packet.Write<f32>(cast.clientTime);

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
				packet.Write<f32>(0.f);

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

	LOG("[client%03d] Replication :: SendActorSpawn :: actorUID=%u localActorID=%u", clientHd, (u32)actor.actorUID, (u32)localActorID);

	// this is the main actor
	if(actor.actorUID == parent.masters[0]) {
		// SN_GameCreateActor
		{
			PacketWriter<Sv::SN_GameCreateActor,512> packet;

			packet.Write<LocalActorID>(localActorID); // objectID
			packet.Write<EntityType>(EntityType::CREATURE); // nType
			packet.Write<CreatureIndex>((CreatureIndex)(100000000 + (i32)actor.classType)); // nIDX
			packet.Write<i32>(-1);
			packet.Write(actor.pos);
			packet.Write(WorldYawToMxmYaw(actor.rotation.upperYaw));
			packet.Write(WorldPitchToMxmPitch(actor.rotation.upperPitch));
			packet.Write(WorldYawToMxmYaw(actor.rotation.bodyYaw));
			packet.Write<i32>(actor.spawnAnim);
			packet.Write<ActionStateID>(actor.actionState);
			packet.Write<i32>(actor.ownerID);
			packet.Write<u8>(actor.dirToNearPC);
			packet.Write<i32>(actor.wanderDist);
			packet.Write<i32>(actor.tagID);

			packet.Write<i32>(3 + parent.team); // faction
			packet.Write<ClassType>(actor.classType); // classType
			packet.Write<SkinIndex>(actor.skinIndex); // skinIndex
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
		const LocalActorID parentLocalActorID = GetLocalActorID(clientHd, parent.masters[0]);
		ASSERT(parentLocalActorID != LocalActorID::INVALID);

		// SN_GameCreateSubActor
		{
			PacketWriter<Sv::SN_GameCreateSubActor,512> packet;

			packet.Write<LocalActorID>(localActorID); // objectID
			packet.Write<LocalActorID>(parentLocalActorID); // mainEntityID
			packet.Write<EntityType>(EntityType::CREATURE); // nType
			packet.Write<CreatureIndex>((CreatureIndex)(100000000 + (i32)actor.classType)); // nIDX
			packet.Write<i32>(-1);
			packet.Write(actor.pos);
			packet.Write(WorldYawToMxmYaw(actor.rotation.upperYaw));
			packet.Write(WorldPitchToMxmPitch(actor.rotation.upperPitch));
			packet.Write(WorldYawToMxmYaw(actor.rotation.bodyYaw));
			packet.Write<i32>(actor.spawnAnim);
			packet.Write<ActionStateID>(actor.actionState);
			packet.Write<i32>(actor.ownerID);
			packet.Write<i32>(actor.tagID);

			packet.Write<i32>(3 + parent.team);
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
		packet.Write<WeaponIndex>(actor.actorUID == parent.masters[0] ? parent.mainWeapon : parent.subWeapon);
		packet.Write<f32>(0.f);
		packet.Write<f32>(0.f);

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

	SendMasterSkillSlots(clientHd, actor, parent);
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
		packet.Write<i32>(actor.entityType);
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
		packet.Write<Faction>(actor.faction);
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
		packet.Write<i32>(actor.entityType);
		packet.Write<CreatureIndex>(actor.docID);
		packet.Write<i32>(actor.localID);
		packet.Write(actor.pos);
		packet.Write(actor.rot);
		packet.Write<i32>(actor.spawnAnim);
		packet.Write<ActionStateID>(actor.action);
		packet.Write<i32>(actor.ownerID);
		packet.Write<u8>(actor.dirToNearPC);
		packet.Write<i32>(actor.wanderDist);
		packet.Write<i32>(actor.tagID);
		packet.Write<Faction>(actor.faction);
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

void Replication::SendActorDestroy(ClientHandle clientHd, ActorUID actorUID)
{
	const i32 clientID = playerMap.at(clientHd);
	auto found = playerLocalInfo[clientID].localActorIDMap.find(actorUID);
	ASSERT(found != playerLocalInfo[clientID].localActorIDMap.end());
	const LocalActorID localActorID = found->second;

	Sv::SN_DestroyEntity packet;
	packet.objectID = localActorID;
	LOG("[client%03d] Server :: SN_DestroyEntity :: actorUID=%u", clientID, (u32)actorUID);
	SendPacket(clientHd, packet);
}

void Replication::SendMasterSkillSlots(ClientHandle clientHd, const Replication::ActorMaster& actor, const Player& player)
{
	DBG_ASSERT(actor.actorUID != ActorUID::INVALID);
	const LocalActorID localActorID = GetLocalActorID(clientHd, actor.actorUID);
	ASSERT(localActorID != LocalActorID::INVALID);

	const GameXmlContent& content = GetGameXmlContent();

	// SN_PlayerSkillSlot
	{
		PacketWriter<Sv::SN_PlayerSkillSlot,4096> packet;

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

		const size_t skillOffset = actor.actorUID == player.masters[0] ? 0 : 2;
		packet.Write<SkillID>(stageSkills[0]);
		packet.Write<SkillID>(stageSkills[1]);
		packet.Write<SkillID>(player.skills[skillOffset]);
		packet.Write<SkillID>(player.skills[skillOffset + 1]);
		packet.Write<SkillID>(master.skillIDs.back());

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
		packet.Write<u16>(0); // titanDocIndexs_count
		packet.Write<i8>(0); // nextTitanIndex
		packet.Write<u16>(0); // limitExceptionStat_count


		SendPacket(clientHd, packet);
	}

	// SN_ScanEnd
	SendPacketData<Sv::SN_ScanEnd>(clientHd, 0, nullptr);

	// SN_LoadClearedStages
	{
		PacketWriter<Sv::SN_LoadClearedStages> packet;

		packet.Write<u16>(0); // clearedStageList_count

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
