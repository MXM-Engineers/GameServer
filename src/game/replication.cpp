#include "replication.h"
#include <common/protocol.h>
#include <EASTL/algorithm.h>
#include <EASTL/fixed_hash_map.h>
#include <EAStdC/EAString.h>

template<typename T>
inline bool IsListIteratorValid(const T& it)
{
	return it.mpNode != nullptr;
}


void Replication::Frame::Clear()
{
	actorList.clear();
	actorNameplateList.clear();
	actorStatsList.clear();
	actorPlayerInfoList.clear();
	actorUIDMap.clear();
	actorUIDSet.clear();
	transformMap.clear();
	actionStateMap.clear();
}

void Replication::PlayerLocalInfo::Reset()
{
	localActorIDMap.clear();
	actorUIDSet.clear();
	nextPlayerLocalActorID = LocalActorID::FIRST_OTHER_PLAYER;
	nextNpcLocalActorID = LocalActorID::FIRST_NPC;
	nextMonsterLocalActorID = LocalActorID::INVALID;
	isFirstLoad = true;
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

	DoFrameDifference();

	// send SN_ScanEnd if requested
	for(int clientID = 0; clientID < Server::MAX_CLIENTS; clientID++) {
		if(playerState[clientID] != PlayerState::IN_GAME) continue;

		if(playerLocalInfo[clientID].isFirstLoad) {
			playerLocalInfo[clientID].isFirstLoad = false;

			// SN_ScanEnd
			LOG("[client%03d] Server :: SN_ScanEnd ::", clientID);
			SendPacketData(clientID, Sv::SN_ScanEnd::NET_ID, 0, nullptr);

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

				LOG("[client%03d] Server :: SN_TownHudStatistics :: ", clientID);
				SendPacketData(clientID, Sv::SN_TownHudStatistics::NET_ID, packet.size, packet.data);
			}
		}
	}

	eastl::swap(frameCur, framePrev);
	frameCur->Clear(); // clear frame
}

void Replication::FramePushActor(const Actor& actor, const ActorNameplate* nameplate, const ActorStats* stats, const ActorPlayerInfo* playerInfo)
{
	ASSERT(frameCur->actorUIDMap.find(actor.UID) == frameCur->actorUIDMap.end());
	ASSERT(frameCur->actorUIDSet.find(actor.UID) == frameCur->actorUIDSet.end());

	Actor& a = frameCur->actorList.push_back();
	a = actor;
	frameCur->actorUIDMap.emplace(actor.UID, --frameCur->actorList.end());
	DBG_ASSERT((*frameCur->actorUIDMap.find(actor.UID)->second).UID == actor.UID); // TODO: remove
	frameCur->actorUIDSet.insert(actor.UID);

	if(nameplate) {
		ActorNameplate& np = frameCur->actorNameplateList.push_back();
		np = *nameplate;
		a.nameplate = --frameCur->actorNameplateList.end();
	}
	if(stats) {
		ActorStats& s = frameCur->actorStatsList.push_back();
		s = *stats;
		a.stats = --frameCur->actorStatsList.end();
	}
	if(playerInfo) {
		ActorPlayerInfo& p = frameCur->actorPlayerInfoList.push_back();
		p = *playerInfo;
		a.playerInfo = --frameCur->actorPlayerInfoList.end();
	}

	Frame::Transform tf;
	tf.pos = actor.pos;
	tf.dir = actor.dir;
	tf.eye = actor.eye;
	tf.rotate = actor.rotate;
	tf.speed = actor.speed;
	frameCur->transformMap.emplace(actor.UID, tf);

	Frame::ActionState at;
	at.actionState = actor.actionState;
	at.actionParam1 = actor.actionParam1;
	at.actionParam2 = actor.actionParam2;
	at.rotate = actor.rotate;
	at.upperRotate = actor.upperRotate;
	frameCur->actionStateMap.emplace(actor.UID, at);
}

void Replication::EventPlayerConnect(i32 clientID)
{
	playerState[clientID] = PlayerState::CONNECTED;
	playerLocalInfo[clientID].Reset();
}

void Replication::EventPlayerLoad(i32 clientID)
{
	// SN_LoadCharacterStart
	LOG("[client%03d] Server :: SN_LoadCharacterStart :: ", clientID);
	SendPacketData(clientID, Sv::SN_LoadCharacterStart::NET_ID, 0, nullptr);

	// SN_PlayerSkillSlot
	{
		u8 sendData[2048];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<i32>(11111); // characterID

		packet.Write<u16>(7); // slotList_count

		// slotList[0]
		packet.Write<i32>(180350010); // skillIndex
		packet.Write<i32>(0); // coolTime
		packet.Write<u8>(1);  // unlocked
		packet.Write<u16>(1); // propList_count
		packet.Write<i32>(280351001); // skillPropertyIndex
		packet.Write<i32>(1); // level
		packet.Write<u8>(1); // isUnlocked
		packet.Write<u8>(1); // isActivated

		// slotList[1]
		packet.Write<i32>(180350030); // skillIndex
		packet.Write<i32>(0); // coolTime
		packet.Write<u8>(1);  // unlocked
		packet.Write<u16>(1); // propList_count
		packet.Write<i32>(280353001); // skillPropertyIndex
		packet.Write<i32>(1); // level
		packet.Write<u8>(1); // isUnlocked
		packet.Write<u8>(1); // isActivated

		// slotList[2]
		packet.Write<i32>(180350040); // skillIndex
		packet.Write<i32>(0); // coolTime
		packet.Write<u8>(1);  // unlocked
		packet.Write<u16>(0); // propList_count

		packet.Write<u8>(1); // isUnlocked
		packet.Write<u8>(1); // isActivated

		// slotList[3]
		packet.Write<i32>(180350020); // skillIndex
		packet.Write<i32>(0); // coolTime
		packet.Write<u8>(1);  // unlocked
		packet.Write<u16>(0); // propList_count
		packet.Write<u8>(0); // isUnlocked
		packet.Write<u8>(0); // isActivated

		// slotList[4]
		packet.Write<i32>(180350050); // skillIndex
		packet.Write<i32>(0); // coolTime
		packet.Write<u8>(1);  // unlocked
		packet.Write<u16>(0); // propList_count
		packet.Write<u8>(1); // isUnlocked
		packet.Write<u8>(1); // isActivated

		// slotList[5]
		packet.Write<i32>(180350000); // skillIndex
		packet.Write<i32>(0); // coolTime
		packet.Write<u8>(1);  // unlocked
		packet.Write<u16>(0); // propList_count
		packet.Write<u8>(1); // isUnlocked
		packet.Write<u8>(1); // isActivated

		// slotList[6]
		packet.Write<i32>(180350002); // skillIndex
		packet.Write<i32>(0); // coolTime
		packet.Write<u8>(1);  // unlocked
		packet.Write<u16>(0); // propList_count
		packet.Write<u8>(1); // isUnlocked
		packet.Write<u8>(1); // isActivated

		// slotList[7]
		packet.Write<i32>(-1);
		packet.Write<i32>(-1);
		packet.Write<i32>(-1);
		packet.Write<i32>(-1);
		packet.Write<i32>(-1);

		ASSERT(packet.size == 133); // TODO: remove

		LOG("[client%03d] Server :: SN_PlayerSkillSlot :: ", clientID);
		SendPacketData(clientID, Sv::SN_PlayerSkillSlot::NET_ID, packet.size, packet.data);
	}

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
	cityMapInfo.cityMapID = 160000042;
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

void Replication::SetPlayerAsInGame(i32 clientID)
{
	playerState[clientID] = PlayerState::IN_GAME;
}

void Replication::EventPlayerRequestCharacterInfo(i32 clientID, ActorUID actorUID, CreatureIndex docID, ClassType classType, i32 health, i32 healthMax)
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
	LOG("[client%03d] Server :: SA_GetCharacterInfo :: ", clientID);
	SendPacket(clientID, info);
}

void Replication::SendPlayerSetLeaderMaster(i32 clientID, ActorUID masterActorUID, i32 leaderMasterID, SkinIndex skinIndex)
{
	LocalActorID laiLeader = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + leaderMasterID);
	ASSERT(laiLeader >= LocalActorID::FIRST_SELF_MASTER && laiLeader < LocalActorID::LAST_SELF_MASTER);

	PlayerForceLocalActorID(clientID, masterActorUID, laiLeader);

	if(playerState[clientID] < PlayerState::IN_GAME) {
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
		if(playerState[clientID] != PlayerState::IN_GAME) continue;

		LOG("[client%03d] Server :: SN_ChatChannelMessage :: sender='%ls' msg='%.*S'", clientID, senderName, msgLen, msg);
		SendPacketData(clientID, Sv::SN_ChatChannelMessage::NET_ID, packet.size, packet.data);
	}
}

void Replication::SendChatMessageToClient(i32 toClientID, const wchar* senderName, i32 chatType, const wchar* msg, i32 msgLen)
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

void Replication::EventClientDisconnect(i32 clientID)
{
	playerState[clientID] = PlayerState::DISCONNECTED;
}


void Replication::JukeboxPlaySong(i32 result, i32 trackID, wchar* nickname, u16 playPositionSec)
{
	u8 sendData[256];
	PacketWriter packet(sendData, sizeof(sendData));

	packet.Write<i32>(result); // result
	packet.Write<i32>(trackID); // trackID
	packet.WriteStringObj(nickname); // nickname
	packet.Write<u16>(playPositionSec); // playPositionSec

	for (int clientID = 0; clientID < Server::MAX_CLIENTS; clientID++) {
		if (playerState[clientID] != PlayerState::IN_GAME) continue;

		LOG("[client%03d] Server :: SN_JukeboxPlay ::", clientID);
		SendPacketData(clientID, Sv::SN_JukeboxPlay::NET_ID, packet.size, packet.data);
	}
}

void Replication::SendJukeboxPlay(i32 clientID, SongID songID, const wchar* requesterNick, i32 playPosInSec)
{
	/*
	// SN_JukeboxHotTrackList
	{
		u8 sendData[256];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(0); // trackList_count

		LOG("[client%03d] Server :: SN_JukeboxHotTrackList ::", clientID);
		SendPacketData(clientID, Sv::SN_JukeboxHotTrackList::NET_ID, packet.size, packet.data);
	}
	*/

	// SN_JukeboxPlay
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
}

void Replication::SendJukeboxQueue(i32 clientID, const Replication::JukeboxTrack* tracks, const i32 trackCount)
{
	u8 sendData[2048];
	PacketWriter packet(sendData, sizeof(sendData));

	packet.Write<u16>(trackCount); // trackList_count
	for(int i = 0; i < trackCount; i++) {
		packet.Write<SongID>(tracks[i].songID);
		packet.WriteStringObj(tracks[i].requesterNick.data(), tracks[i].requesterNick.size());
	}

	LOG("[client%03d] Server :: SN_JukeboxEnqueuedList ::", clientID);
	SendPacketData(clientID, Sv::SN_JukeboxEnqueuedList::NET_ID, packet.size, packet.data);
}

bool Replication::IsActorReplicatedForClient(i32 clientID, ActorUID actorUID) const
{
	const auto& set = playerLocalInfo[clientID].actorUIDSet;
	return set.find(actorUID) != set.end();
}

void Replication::PlayerForceLocalActorID(i32 clientID, ActorUID actorUID, LocalActorID localActorID)
{
	auto& map = playerLocalInfo[clientID].localActorIDMap;
	ASSERT(map.find(actorUID) == map.end());
	map.emplace(actorUID, localActorID);
}

LocalActorID Replication::GetLocalActorID(i32 clientID, ActorUID actorUID)
{
	const auto& map = playerLocalInfo[clientID].localActorIDMap;
	auto found = map.find(actorUID);
	if(found != map.end()) {
		return found->second;
	}
	return LocalActorID::INVALID;
}

ActorUID Replication::GetActorUID(i32 clientID, LocalActorID localActorID)
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
		if(playerState[clientID] != PlayerState::IN_GAME) continue;

		PlayerLocalInfo& localInfo = playerLocalInfo[clientID];
		auto& playerActorUIDSet = localInfo.actorUIDSet; // replicated actors UID set

		eastl::fixed_set<ActorUID,2048> playerMinusCur;
		eastl::fixed_set<ActorUID,2048> curMinusPlayer;
		eastl::set_difference(playerActorUIDSet.begin(), playerActorUIDSet.end(), frameCur->actorUIDSet.begin(), frameCur->actorUIDSet.end(), eastl::inserter(playerMinusCur, playerMinusCur.begin()));
		eastl::set_difference(frameCur->actorUIDSet.begin(), frameCur->actorUIDSet.end(), playerActorUIDSet.begin(), playerActorUIDSet.end(), eastl::inserter(curMinusPlayer, curMinusPlayer.begin()));

		// send destroy entity for deleted actors
		foreach(setIt, playerMinusCur) {
			const ActorUID actorUID = *setIt;
#ifdef CONF_DEBUG // we don't actually need to verify the actor was in the previous frame, but do it in debug mode anyway
			const auto actorIt = framePrev->actorUIDMap.find(actorUID);
			ASSERT(actorIt != framePrev->actorUIDMap.end());
			const Actor& actor = *actorIt->second;
			ASSERT(actor.UID == actorUID);
#endif

			SendActorDestroy(clientID, actorUID);

			// Remove LocalActorID link
			DeleteLocalActorID(clientID, actorUID);
		}

		// send new spawns
		foreach(setIt, curMinusPlayer) {
			const ActorUID actorUID = *setIt;
			const auto actorIt = frameCur->actorUIDMap.find(actorUID);
			ASSERT(actorIt != frameCur->actorUIDMap.end());
			const Actor& actor = *actorIt->second;

			// Create a LocalActorID link if none exists already
			// If one exists already, we have pre-allocated it (like with leader master)
			if(GetLocalActorID(clientID, actorUID) == LocalActorID::INVALID) {
				CreateLocalActorID(clientID, actorUID);
			}

			SendActorSpawn(clientID, actor);
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

void Replication::DoFrameDifference()
{
	// send position update

	eastl::fixed_vector<eastl::pair<ActorUID,Frame::Transform>, 2048> tfToSendList;
	eastl::fixed_vector<eastl::pair<ActorUID,Frame::ActionState>, 2048> atToSendList;

	// find if the position has changed since last frame
	foreach(it, frameCur->actorUIDSet) {
		const ActorUID actorUID = *it;

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

			// action state
			{
				auto pf = framePrev->actionStateMap.find(actorUID);
				ASSERT(pf != framePrev->actionStateMap.end());
				const Frame::ActionState& prev = pf->second;

				auto cf = frameCur->actionStateMap.find(actorUID);
				ASSERT(cf != frameCur->actionStateMap.end());
				const Frame::ActionState& cur = cf->second;

				if(!prev.HasNotChanged(cur)) {
					atToSendList.emplace_back(actorUID, cur);
				}
			}
		}
		// was not present in last frame
		else {
			// action state
			{
				auto cf = frameCur->actionStateMap.find(actorUID);
				ASSERT(cf != frameCur->actionStateMap.end());
				const Frame::ActionState& cur = cf->second;

				if(cur.actionState != ActionStateID::INVALID) {
					atToSendList.emplace_back(actorUID, cur);
				}
			}
		}
	}

	// send updates
	foreach(it, tfToSendList) {
		const auto& e = *it;
		const Frame::Transform& tf = e.second;

		Sv::SN_GamePlayerSyncByInt sync;
		sync.p3nPos = tf.pos;
		sync.p3nDir = tf.dir;
		sync.p3nEye = tf.eye;
		sync.nRotate = tf.rotate;
		sync.nSpeed = tf.speed;
		sync.nState = -1;
		sync.nActionIDX = -1;

		for(int clientID = 0; clientID < Server::MAX_CLIENTS; clientID++) {
			if(playerState[clientID] != PlayerState::IN_GAME) continue;

			sync.characterID = GetLocalActorID(clientID, e.first);
			LOG("[client%03d] Server :: SN_GamePlayerSyncByInt :: actorUID=%u", clientID, (u32)e.first);
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
			LOG("[client%03d] Server :: SN_PlayerSyncActionStateOnly :: state=%d param1=%d", clientID, packet.state, packet.param1);
			SendPacket(clientID, packet);
		}
	}
}

void Replication::SendActorSpawn(i32 clientID, const Actor& actor)
{
	DBG_ASSERT(actor.UID != ActorUID::INVALID);
	auto found = playerLocalInfo[clientID].localActorIDMap.find(actor.UID);
	ASSERT(found != playerLocalInfo[clientID].localActorIDMap.end());

	const LocalActorID localActorID = found->second;

	LOG("[client%03d] Replication :: SendActorSpawn :: actorUID=%u localActorID=%u", clientID, (u32)actor.UID, (u32)localActorID);

	// SN_GameCreateActor
	{
		i32 localID = -1;
		if(!IsListIteratorValid(actor.playerInfo)) {
			static i32 nextLocalID = 1;
			localID = nextLocalID++;
		}

		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<LocalActorID>(localActorID); // objectID
		packet.Write<i32>(actor.type); // nType
		packet.Write<CreatureIndex>(actor.docID); // nIDX
		packet.Write<i32>(localID); // dwLocalID
		// TODO: localID?

		packet.Write(actor.pos); // p3nPos
		packet.Write(actor.dir); // p3nDir
		packet.Write<i32>(actor.spawnType); // spawnType
		packet.Write<ActionStateID>(actor.actionState); // actionState
		packet.Write<i32>(actor.ownerID); // ownerID
		packet.Write<u8>(0); // bDirectionToNearPC
		packet.Write<i32>(-1); // AiWanderDistOverride
		packet.Write<i32>(-1); // tagID
		packet.Write<i32>(actor.faction); // faction
		packet.Write<ClassType>(actor.classType); // classType
		packet.Write<SkinIndex>(actor.skinIndex); // skinIndex
		packet.Write<i32>(0); // seed

		if(!IsListIteratorValid(actor.stats)) {
			packet.Write<u16>(0); // maxStats_count
			packet.Write<u16>(0); // curStats_count
		}
		else {
			typedef Sv::SN_GameCreateActor::BaseStat::Stat Stat;

			// initStat ------------------------
			packet.Write<u16>(53); // maxStats_count
			packet.Write(Stat{ 0, 2400 });
			packet.Write(Stat{ 35, 1000 });
			packet.Write(Stat{ 17, 0 });
			packet.Write(Stat{ 36, 0 });
			packet.Write(Stat{ 56, 0 });
			packet.Write(Stat{ 2, 200 });
			packet.Write(Stat{ 37, 120 });
			packet.Write(Stat{ 3, 0 });
			packet.Write(Stat{ 39, 5 });
			packet.Write(Stat{ 41, 0 });
			packet.Write(Stat{ 40, 0 });
			packet.Write(Stat{ 57, 0 });
			packet.Write(Stat{ 50, 0 });
			packet.Write(Stat{ 51, 0 });
			packet.Write(Stat{ 5, 5 });
			packet.Write(Stat{ 42, 0.6f });
			packet.Write(Stat{ 6, 0 });
			packet.Write(Stat{ 7, 93.75f });
			packet.Write(Stat{ 8, 0 });
			packet.Write(Stat{ 9, 3 });
			packet.Write(Stat{ 10, 150 });
			packet.Write(Stat{ 12, 0 });
			packet.Write(Stat{ 20, 0 });
			packet.Write(Stat{ 21, 0 });
			packet.Write(Stat{ 18, 100 });
			packet.Write(Stat{ 13, 100 });
			packet.Write(Stat{ 14, 98 });
			packet.Write(Stat{ 15, 100 });
			packet.Write(Stat{ 52, 100 });
			packet.Write(Stat{ 16, 1 });
			packet.Write(Stat{ 27, 0 });
			packet.Write(Stat{ 47, 0 });
			packet.Write(Stat{ 49, 0 });
			packet.Write(Stat{ 48, 0 });
			packet.Write(Stat{ 29, 20 });
			packet.Write(Stat{ 23, 9 });
			packet.Write(Stat{ 44, 15 });
			packet.Write(Stat{ 46, 0 });
			packet.Write(Stat{ 45, 0 });
			packet.Write(Stat{ 26, 0 });
			packet.Write(Stat{ 25, 0 });
			packet.Write(Stat{ 31, 14 });
			packet.Write(Stat{ 22, 2 });
			packet.Write(Stat{ 54, 15 });
			packet.Write(Stat{ 60, 0 });
			packet.Write(Stat{ 61, 0 });
			packet.Write(Stat{ 62, 0 });
			packet.Write(Stat{ 63, 3 });
			packet.Write(Stat{ 64, 150 });
			packet.Write(Stat{ 53, 0 });
			packet.Write(Stat{ 58, 0 });
			packet.Write(Stat{ 65, 0 });
			packet.Write(Stat{ 55, 15 });

			packet.Write<u16>(2); // curStats_count
			packet.Write(Stat{ 0, 2400 });
			packet.Write(Stat{ 2, 200 });
			// ------------------------------------
		}

		packet.Write<u8>(1); // isInSight
		packet.Write<u8>(0); // isDead
		packet.Write<i64>((i64)TimeDiffMs(TimeRelNow())); // serverTime

		packet.Write<u16>(0); // meshChangeActionHistory_count

		LOG("[client%03d] Server :: SN_GameCreateActor :: actorUID=%d", clientID, (u32)actor.UID);
		SendPacketData(clientID, Sv::SN_GameCreateActor::NET_ID, packet.size, packet.data);
	}

	// SN_SpawnPosForMinimap
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<LocalActorID>(localActorID); // objectID
		packet.Write(actor.pos); // p3nPos

		LOG("[client%03d] Server :: SN_SpawnPosForMinimap :: actorUID=%d", clientID, (u32)actor.UID);
		SendPacketData(clientID, Sv::SN_SpawnPosForMinimap::NET_ID, packet.size, packet.data);
	}

	if(IsListIteratorValid(actor.nameplate)) {
		const ActorNameplate& plate = *actor.nameplate;

		// SN_GamePlayerStock
		{
			u8 sendData[2048];
			PacketWriter packet(sendData, sizeof(sendData));

			packet.Write<LocalActorID>(localActorID); // playerID
			packet.WriteStringObj(plate.name.data()); // name
			packet.Write<ClassType>(actor.classType); // class_
#if 0
			packet.Write<i32>(320080005); // displayTitleIDX
			packet.Write<i32>(320080005); // statTitleIDX
#else // disable titles
			packet.Write<i32>(0); // displayTitleIDX
			packet.Write<i32>(0); // statTitleIDX
#endif
			packet.Write<u8>(0); // badgeType
			packet.Write<u8>(0); // badgeTierLevel
			packet.WriteStringObj(plate.guildTag.data()); // guildTag
			packet.Write<u8>(0); // vipLevel
			packet.Write<u8>(0); // staffType
			packet.Write<u8>(0); // isSubstituted

			LOG("[client%03d] Server :: SN_GamePlayerStock :: ", clientID);
			SendPacketData(clientID, Sv::SN_GamePlayerStock::NET_ID, packet.size, packet.data);
		}
	}

	if(IsListIteratorValid(actor.playerInfo)) {

		// SN_GamePlayerEquipWeapon
		{
			u8 sendData[1024];
			PacketWriter packet(sendData, sizeof(sendData));

			packet.Write<LocalActorID>(localActorID); // characterID
			packet.Write<i32>(131135012); // weaponDocIndex
			packet.Write<i32>(0); // additionnalOverHeatGauge
			packet.Write<i32>(0); // additionnalOverHeatGaugeRatio

			LOG("[client%03d] Server :: SN_GamePlayerEquipWeapon :: ", clientID);
			SendPacketData(clientID, Sv::SN_GamePlayerEquipWeapon::NET_ID, packet.size, packet.data);
		}

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

bool Replication::Frame::Transform::HasNotChanged(const Frame::Transform& other) const
{
	const f32 posEpsilon = 0.05f;
	if(fabs(pos.x - other.pos.x) > posEpsilon) return false;
	if(fabs(pos.y - other.pos.y) > posEpsilon) return false;
	if(fabs(pos.z - other.pos.z) > posEpsilon) return false;
	if(fabs(dir.x - other.dir.x) > posEpsilon) return false;
	if(fabs(dir.y - other.dir.y) > posEpsilon) return false;
	if(fabs(dir.z - other.dir.z) > posEpsilon) return false;
	if(fabs(eye.x - other.eye.x) > posEpsilon) return false;
	if(fabs(eye.y - other.eye.y) > posEpsilon) return false;
	if(fabs(eye.z - other.eye.z) > posEpsilon) return false;
	const f32 rotEpsilon = 0.01f;
	if(fabs(rotate - other.rotate) > rotEpsilon) return false;
	const f32 rotSpeed = 0.1f;
	if(fabs(speed - other.speed) > rotSpeed) return false;
	return true;
}

bool Replication::Frame::ActionState::HasNotChanged(const Replication::Frame::ActionState& other) const
{
	if(other.actionState == ActionStateID::INVALID) return true;
	if(actionState != other.actionState) return false;
	if(actionParam1 != other.actionParam1) return false;
	if(actionParam2 != other.actionParam2) return false;
	return true;
}
