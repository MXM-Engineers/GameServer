#include "replication.h"
#include <common/protocol.h>
#include <EASTL/algorithm.h>
#include <EASTL/fixed_hash_map.h>

void Replication::Frame::Clear()
{
	memset(&playerDoScanEnd, 0, sizeof(playerDoScanEnd));
	actorList.clear();
}

void Replication::Init(Server* server_)
{
	server = server_;
	memset(&playerState, 0, sizeof(playerState));
}

void Replication::FrameEnd()
{
	// TODO: reflect all actors
	eastl::fixed_set<u32,2048> curActorUidSet;
	eastl::fixed_hash_map<u32,const Actor*,2048> actorUidMap;

	for(auto it = frameCur.actorList.begin(), itEnd = frameCur.actorList.end(); it != itEnd; ++it) {
		curActorUidSet.insert(it->UID);
		actorUidMap[it->UID] = it;
	}

	for(int clientID = 0; clientID < Server::MAX_CLIENTS; clientID++) {
		if(playerState[clientID] != PlayerState::IN_GAME) continue;

		const auto& localUidSet = playerLocalActorUidSet[clientID];

		eastl::fixed_set<u32,2048> localMinusCur;
		eastl::fixed_set<u32,2048> curMinusLocal;
		eastl::set_difference(localUidSet.begin(), localUidSet.end(), curActorUidSet.begin(), curActorUidSet.end(), eastl::inserter(localMinusCur, localMinusCur.begin()));
		eastl::set_difference(curActorUidSet.begin(), curActorUidSet.end(), localUidSet.begin(), localUidSet.end(), eastl::inserter(curMinusLocal, curMinusLocal.begin()));

		// send new spawns
		const i32 newActorCount = curMinusLocal.size();
		for(auto setIt = curMinusLocal.begin(), itEnd = curMinusLocal.end(); setIt != itEnd; ++setIt) {
			const auto actorIt = actorUidMap.find(*setIt);
			ASSERT(actorIt != actorUidMap.end());
			const Actor& actor = *actorIt->second;

			// SN_GameCreateActor
			{
				u8 sendData[1024];
				PacketWriter packet(sendData, sizeof(sendData));

				packet.Write<i32>(actor.UID); // objectID
				packet.Write<i32>(actor.type); // nType
				packet.Write<i32>(actor.modelID); // nIDX
				packet.Write<i32>(-1); // dwLocalID
				packet.Write(actor.pos); // p3nPos
				packet.Write(actor.dir); // p3nDir
				packet.Write<i32>(actor.spawnType); // spawnType
				packet.Write<i32>(actor.actionState); // actionState
				packet.Write<i32>(actor.ownerID); // ownerID
				packet.Write<u8>(0); // bDirectionToNearPC
				packet.Write<i32>(-1); // AiWanderDistOverride
				packet.Write<i32>(-1); // tagID
				packet.Write<i32>(actor.faction); // faction
				packet.Write<i32>(actor.classType); // classType
				packet.Write<i32>(actor.skinIndex); // skinIndex
				packet.Write<i32>(0); // seed

#if 0
				packet.Write<u16>(0); // maxStats_count
				packet.Write<u16>(0); // curStats_count

#else
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
#endif

				packet.Write<u8>(1); // isInSight
				packet.Write<u8>(0); // isDead
				packet.Write<i64>(GetTime()); // serverTime

				packet.Write<u16>(0); // meshChangeActionHistory_count

				LOG("[client%03d] Server :: SN_GameCreateActor :: actorUID=%d", clientID, actor.UID);
				SendPacketData(clientID, Sv::SN_GameCreateActor::NET_ID, packet.size, packet.data);
			}

			// TODO: remove these, this is just for testing

			// SN_GamePlayerStock
			{
				u8 sendData[1024];
				PacketWriter packet(sendData, sizeof(sendData));

				packet.Write<i32>(actor.UID); // playerID
				packet.WriteStringObj(L"LordSk"); // name
				packet.Write<i32>(actor.classType); // class_
				packet.Write<i32>(320080005); // displayTitleIDX
				packet.Write<i32>(320080005); // statTitleIDX
				packet.Write<u8>(0); // badgeType
				packet.Write<u8>(0); // badgeTierLevel
				packet.WriteStringObj(L"XMX"); // guildTag
				packet.Write<u8>(0); // vipLevel
				packet.Write<u8>(0); // staffType
				packet.Write<u8>(0); // isSubstituted

				LOG("[client%03d] Server :: SN_GamePlayerStock :: ", clientID);
				SendPacketData(clientID, Sv::SN_GamePlayerStock::NET_ID, packet.size, packet.data);
			}

			// SN_GamePlayerEquipWeapon
			{
				u8 sendData[1024];
				PacketWriter packet(sendData, sizeof(sendData));

				packet.Write<i32>(actor.UID); // characterID
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

				packet.Write<i32>(actor.UID); // playerID
				packet.Write<u8>(0); // playerStateInTown
				packet.Write<u16>(0); // matchingGameModes_count

				LOG("[client%03d] Server :: SN_PlayerStateInTown :: state=%d", clientID, -1);
				SendPacketData(clientID, Sv::SN_PlayerStateInTown::NET_ID, packet.size, packet.data);
			}
		}

		// update local set
		playerLocalActorUidSet[clientID] = curActorUidSet;
	}

	// send SN_ScanEnd if requested
	for(int clientID = 0; clientID < Server::MAX_CLIENTS; clientID++) {
		if(playerState[clientID] != PlayerState::IN_GAME) continue;

		if(frameCur.playerDoScanEnd[clientID]) {
			// SN_ScanEnd
			{
				LOG("[client%03d] Server :: SN_ScanEnd ::", clientID);
				SendPacketData(clientID, Sv::SN_ScanEnd::NET_ID, 0, nullptr);
			}
		}
	}

	framePrev = frameCur;
	frameCur.Clear(); // clear frame
}

void Replication::FramePushActor(const Replication::Actor& actor)
{
	frameCur.actorList.push_back(actor);
}

void Replication::EventPlayerConnect(i32 clientID, u32 playerAssignedActorUID)
{
	playerState[clientID] = PlayerState::CONNECTED;
	playerLocalActorUidSet[clientID].clear();

	// SN_LoadCharacterStart
	LOG("[client%03d] Server :: SN_LoadCharacterStart :: ", clientID);
	SendPacketData(clientID, Sv::SN_LoadCharacterStart::NET_ID, 0, nullptr);

	// SN_LeaderCharacter
	Sv::SN_LeaderCharacter leader;
	leader.leaderID = playerAssignedActorUID;
	leader.skinIndex = 0;
	LOG("[client%03d] Server :: SN_LeaderCharacter :: actorUID=%d", clientID, playerAssignedActorUID);
	SendPacket(clientID, leader);

	/*
	// SN_PlayerSkillSlot
	{
		u8 sendData[2048];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<i32>(21013);

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
*/
	// SN_SetGameGvt
	{
		Sv::SN_SetGameGvt gameGvt;
		i32 time = GetTime();
		gameGvt.sendTime = time;
		gameGvt.virtualTime = time;
		LOG("[client%03d] Server :: SN_SetGameGvt :: ", clientID);
		SendPacket(clientID, gameGvt);
	}

	/*
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
*/
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

void Replication::EventPlayerGameEnter(i32 clientID)
{
	playerState[clientID] = PlayerState::IN_GAME;
	frameCur.playerDoScanEnd[clientID] = true;
}
