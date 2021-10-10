#include "instance.h"
#include "account.h"
#include <mxm/game_content.h>

bool HubInstance::Init(Server* server_)
{
	game.Init(server_, &plidMap);
	packetHandler.Init(&game);
	return true;
}

void HubInstance::Update(Time localTime_)
{
	game.Update(localTime_);
}

void HubInstance::OnClientsConnected(const NewUser* clientList, const i32 count)
{
	for(int i = 0; i < count; i++) {
		plidMap.Push(clientList[i].clientHd);
	}

	// FIXME: we should pull account data at the last moment and just pass AccountUIDs
	const AccountManager& am = GetAccountManager();
	eastl::fixed_vector<eastl::pair<ClientHandle, const Account*>,MAX_CLIENTS> list;
	for(int i = 0; i < count; i++) {
		list.push_back({ clientList[i].clientHd, &*am.accountMap.at(clientList[i].accountUID) });
	}

	packetHandler.OnClientsConnected(list.data(), count);
}

void HubInstance::OnClientsDisconnected(const ClientHandle* clientList, const i32 count)
{
	packetHandler.OnClientsDisconnected(clientList, count);

	for(int i = 0; i < count; i++) {
		plidMap.Pop(clientList[i]);
	}
}

void HubInstance::OnClientsTransferOut(const ClientHandle* clientList, const i32 count)
{
	// TODO: push to a 'transfer list' that will call replicate.SendClearAllEntities() before detaching clients
	packetHandler.OnClientsDisconnected(clientList, count);

	for(int i = 0; i < count; i++) {
		plidMap.Pop(clientList[i]);
	}
}

void HubInstance::OnClientPacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData)
{
	packetHandler.OnNewPacket(clientHd, header, packetData);
}

void HubInstance::OnMatchmakerPacket(const NetHeader& header, const u8* packetData)
{
	packetHandler.OnMatchmakerPacket(header, packetData);
}

void RoomInstance::Init(Server* server_, const NewUser* userlist, const i32 userCount)
{
	server = server_;

	teamMasterPickCount[Team::RED].fill(0);
	teamMasterPickCount[Team::BLUE].fill(0);

	allowedMastersSet = {
		ClassType::Taejin,
		ClassType::MBA_07,
		ClassType::Sizuka,
		ClassType::Demenos,
		ClassType::Koom,
		ClassType::Innowin,
		ClassType::Lua,
	};

	LOG("[Room(%llx)] room created (userCount=%d)", sortieUID, userCount);

	for(const NewUser* nu = userlist; nu != userlist+userCount; ++nu) {
		userList.emplace_back(nu->clientHd, nu->accountUID, nu->userID, nu->isBot, Team(nu->team));
		User* user = &userList.back();

		// has account when client is connected
		ASSERT(nu->clientHd == ClientHandle::INVALID || nu->accountUID != AccountUID::INVALID);

		switch(nu->team) {
			case Team::RED: { teamRed.push_back(user); } break;
			case Team::BLUE: { teamBlue.push_back(user); } break;
			case Team::SPECTATORS: { teamSpectator.push_back(user); } break;
			default: { ASSERT_MSG(0, "case not handled"); }
		}

		if(nu->clientHd != ClientHandle::INVALID) {
			connectedUsers.push_back(user);
		}
	}

	foreach_const(u, connectedUsers) {
		const User& user = **u;

		// is this for when all players accepted or just our own party?
		Sv::SN_MatchingPartyGathered gathered;
		gathered.allConfirmed = 1;
		SendPacket(user.clientHd, gathered);

		const GameXmlContent& content = GetGameXmlContent();

		// SN_ProfileCharacters
		{
			PacketWriter<Sv::SN_ProfileCharacters,2048> packet;

			packet.Write<u16>(allowedMastersSet.size()); // charaList_count

			foreach_const(it, allowedMastersSet) {
				const GameXmlContent::Master& master = *content.masterClassTypeMap.at(*it);

				Sv::SN_ProfileCharacters::Character chara;
				chara.characterID = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)master.classType);
				chara.creatureIndex = master.ID;
				chara.skillShot1 = master.skillIDs[0];
				chara.skillShot2 = master.skillIDs[1];
				chara.classType = master.classType;
				chara.x = 0;
				chara.y = 0;
				chara.z = 0;
				chara.characterType = 1;
				chara.skinIndex = SkinIndex::DEFAULT;
				chara.weaponIndex = master.weaponIDs[0];
				chara.masterGearNo = 1;
				packet.Write(chara);
			}

			SendPacket(user.clientHd, packet);
		}

		// SN_ProfileWeapons
		{
			PacketWriter<Sv::SN_ProfileWeapons,4096> packet;

			u16 weaponCount = 0;
			foreach_const(it, allowedMastersSet) {
				weaponCount += 1;
			}

			packet.Write<u16>(weaponCount); // weaponList_count

			foreach_const(it, allowedMastersSet) {
				const GameXmlContent::Master& master = *content.masterClassTypeMap.at(*it);

				Sv::SN_ProfileWeapons::Weapon weapon;
				weapon.characterID = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)master.classType);
				weapon.weaponType = 1;
				weapon.weaponIndex = master.weaponIDs[0];
				weapon.grade = 0;
				weapon.isUnlocked = 1;
				weapon.isActivated = 1;
				packet.Write(weapon);
			}

			SendPacket(user.clientHd, packet);
		}

		// SN_ProfileMasterGears
		{
			PacketWriter<Sv::SN_ProfileMasterGears,2048> packet;

			// NOTE: we send one empty gear page for now

			packet.Write<u16>(1); // masterGears_count

			// 0
			packet.Write<u8>(1); // masterGearNo
			packet.WriteStringObj(L"Default");
			const Sv::SN_ProfileMasterGears::Slot slots[] = {
				{ -1, 0 },
				{ -1, 0 },
				{ -1, 0 },
				{ -1, 0 },
				{ -1, 0 },
				{ -1, 0 },
			};
			packet.WriteVec(slots, ARRAY_COUNT(slots));

			SendPacket(user.clientHd, packet);
		}

		// SN_ProfileSkills
		{
			PacketWriter<Sv::SN_ProfileSkills,8192> packet;
			// TODO: split in several packets?
			packet.Write<u8>(1); // packetNum

			u16 skillCount = 0;
			foreach_const(it, allowedMastersSet) {
				const GameXmlContent::Master& master = *content.masterClassTypeMap.at(*it);
				skillCount += master.skillIDs.size();
			}

			packet.Write<u16>(skillCount); // skills_count

			foreach_const(it, allowedMastersSet) {
				const GameXmlContent::Master& master = *content.masterClassTypeMap.at(*it);

				LocalActorID characterID = LocalActorID((u32)LocalActorID::FIRST_SELF_MASTER + (i32)master.classType);

				foreach_const(s, master.skillIDs) {
					packet.Write(characterID); // characterID
					packet.Write(*s); // skillIndex
					packet.Write<u8>(1); // isUnlocked
					packet.Write<u8>(1); // isActivated
					packet.Write<u16>(0); // properties_count
				}
			}

			SendPacket(user.clientHd, packet);
		}

		// SN_MasterRotationInfo
		{
			PacketWriter<Sv::SN_MasterRotationInfo> packet;
			packet.Write<i32>(0); // refreshCount

			eastl::fixed_vector<CreatureIndex,100,false> freeList;
			foreach_const(m, allowedMastersSet) {
				freeList.push_back(CreatureIndex(100000000 + (i32)*m));
			}

			packet.WriteVec(freeList.data(), freeList.size()); // freeRotation
			packet.WriteVec((CreatureIndex*)nullptr, 0); // pccafeRotation
			packet.WriteVec((CreatureIndex*)nullptr, 0); // vipRotation

			SendPacket(user.clientHd, packet);
		}

		{
			PacketWriter<Sv::SN_SortieMasterPickPhaseStart> packet;
			packet.Write<u8>(0); // isRandomPick
			packet.Write<u16>(0); // alliesSlot_count

			SendPacket(user.clientHd, packet);
		}

		// allow everyone to pick at once
		{
			PacketWriter<Sv::SN_SortieMasterPickPhaseStepStart> packet;
			packet.Write<i32>(60); // timeSec

			eastl::fixed_vector<UserID,5> allyTeamUserIds;
			eastl::fixed_vector<UserID,5> enemyTeamUserIds;

			foreach_const(u, teamRed) {
				allyTeamUserIds.push_back(UserID((*u)->userID + 1));
			}
			foreach_const(u, teamBlue) {
				enemyTeamUserIds.push_back(UserID((*u)->userID + 1));
			}

			packet.WriteVec(allyTeamUserIds.data(), allyTeamUserIds.size()); // alliesTeamUserIds
			packet.WriteVec(enemyTeamUserIds.data(), enemyTeamUserIds.size()); // enemiesTeamUserIds

			SendPacket(user.clientHd, packet);
		}
	}

	// make bots pick random masters
	foreach(u, userList) {
		if(u->isBot) {
			for(int attempts = 0; attempts < 1000; attempts++) {
				u32 r0 = RandUint() % allowedMastersSet.size();

				u32 i = 0;
				foreach_const(m, allowedMastersSet) {
					if(i == r0) {
						TryPickMaster(u, *m);
						break;
					}
					i++;
				}

				if(u->masterSub != ClassType::INVALID) {
					break;
				}
			}
			u->replicate.masterPick = true;
		}
	}
}

void RoomInstance::Update(Time localTime_)
{
	bool doReplicateMasterCount = false;

	foreach(u, userList) {
		User& user = *u;
		if(user.replicate.masterPick) {
			user.replicate.masterPick = false;
			doReplicateMasterCount = true;

			Sv::SN_MasterPick pick;
			pick.userID = UserID(user.userID + 1);
			pick.characterSelectInfos_count = 2;

			if(user.masterMain != ClassType::INVALID) {
				pick.characterSelectInfos[0].localMasterID = LocalActorID((u32)LocalActorID::FIRST_SELF_MASTER + (i32)user.masterMain);
				pick.characterSelectInfos[0].creatureIndex = CreatureIndex(100000000 + (i32)user.masterMain);
				pick.characterSelectInfos[0].skillSlot1 = SkillID::INVALID; // TODO: skill
				pick.characterSelectInfos[0].skillSlot2 = SkillID::INVALID;
			}
			else {
				pick.characterSelectInfos[0].localMasterID = LocalActorID::INVALID;
				pick.characterSelectInfos[0].creatureIndex = CreatureIndex::Invalid;
				pick.characterSelectInfos[0].skillSlot1 = SkillID::INVALID;
				pick.characterSelectInfos[0].skillSlot2 = SkillID::INVALID;
			}

			if(user.masterSub != ClassType::INVALID) {
				pick.characterSelectInfos[1].localMasterID = LocalActorID((u32)LocalActorID::FIRST_SELF_MASTER + (i32)user.masterSub);
				pick.characterSelectInfos[1].creatureIndex = CreatureIndex(100000000 + (i32)user.masterSub);
				pick.characterSelectInfos[1].skillSlot1 = SkillID::INVALID; // TODO: skill
				pick.characterSelectInfos[1].skillSlot2 = SkillID::INVALID;
			}
			else {
				pick.characterSelectInfos[1].localMasterID = LocalActorID::INVALID;
				pick.characterSelectInfos[1].creatureIndex = CreatureIndex::Invalid;
				pick.characterSelectInfos[1].skillSlot1 = SkillID::INVALID; // TODO: skill
				pick.characterSelectInfos[1].skillSlot2 = SkillID::INVALID;
			}

			foreach_const(u, connectedUsers) {
				SendPacket((*u)->clientHd, pick);
			}
		}
	}

	if(doReplicateMasterCount) {
		PacketWriter<Sv::SN_SortieCharacterSlotInfo> packet[2];
		u16 slotInfos_count[2] = {0};

		for(int team = 0; team < 2; team++) {
			// TODO: we *only* need to send which one have been updated, might need to do frame differential after all...
			// right now we send everything, which is not a LOT but still
			slotInfos_count[team] = allowedMastersSet.size();
			packet[team].Write<u16>(slotInfos_count[team]);

			foreach_const(m, allowedMastersSet) {
				u8 c = teamMasterPickCount[team][(i32)*m];
				DBG_ASSERT(c <= 2);

				packet[team].Write(CreatureIndex(100000000 + (i32)*m));
				packet[team].Write<u16>(2);
				for(int s = 0; s < c; s++) packet[team].Write<i32>(1);
				if(c < 2) {
					for(int s = 0; s < (2-c); s++) packet[team].Write<i32>(0);
				}
			}
		}

		foreach_const(u, connectedUsers) {
			if((*u)->team != Team::RED && (*u)->team != Team::BLUE) continue;
			SendPacket((*u)->clientHd, packet[(*u)->team]);
		}
	}
}

void RoomInstance::OnClientPacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData)
{
	const i32 packetSize = header.size - sizeof(header);

	switch(header.netID) {
		case Cl::CQ_MasterPick::NET_ID: {
			const Cl::CQ_MasterPick& packet = SafeCast<Cl::CQ_MasterPick>(packetData, packetSize);
			User* user = FindUser(clientHd);
			ASSERT(user);

			ClassType selected = ClassType((u32)packet.localMasterID - (u32)LocalActorID::FIRST_SELF_MASTER);
			bool r = TryPickMaster(user, selected);

			Sv::SA_MasterPick answer;
			answer.retval = r ? 0:1; // TODO: actual error message
			answer.localMasterID = packet.localMasterID;
			SendPacket(clientHd, answer);
		} break;

		case Cl::CQ_MasterUnpick::NET_ID: {
			const Cl::CQ_MasterUnpick& packet = SafeCast<Cl::CQ_MasterUnpick>(packetData, packetSize);
			User* user = FindUser(clientHd);
			ASSERT(user);

			ClassType selected = ClassType((u32)packet.localMasterID - (u32)LocalActorID::FIRST_SELF_MASTER);
			UnpickMaster(user, selected);
		} break;

		case Cl::CQ_MasterReset::NET_ID: {
			User* user = FindUser(clientHd);
			ASSERT(user);
			ResetMasters(user);
		} break;

		default: {
			WARN("Unknown packet (netID=%d size=%d)", header.netID, header.size);
		}
	}
}

void RoomInstance::OnMatchmakerPacket(const NetHeader& header, const u8* packetData)
{

}

RoomInstance::User* RoomInstance::FindUser(ClientHandle clientHd)
{
	foreach_const(u, connectedUsers) {
		if((*u)->clientHd == clientHd) {
			return &**u;
		}
	}

	return nullptr;
}

bool RoomInstance::TryPickMaster(User* user, ClassType master)
{
	if(teamMasterPickCount[user->team][(i32)master] < 2) {
		if(user->masterMain == ClassType::INVALID) {
			user->masterMain = master;
		}
		else if(user->masterMain != master) { // no duplicate masters
			user->masterSub = master;
		}
		else {
			return false;
		}

		teamMasterPickCount[user->team][(i32)master]++;
		user->replicate.masterPick = true;

		return true;
	}

	return false;
}

void RoomInstance::UnpickMaster(User* user, ClassType master)
{
	u8& masterPickCount = teamMasterPickCount[user->team][(i32)master];

	if(user->masterMain == master) {
		user->masterMain = ClassType::INVALID;

		DBG_ASSERT(masterPickCount > 0);
		if(masterPickCount > 0) {
			masterPickCount--;
		}

		user->replicate.masterPick = true;
	}
	else if(user->masterSub == master) {
		user->masterSub = ClassType::INVALID;

		DBG_ASSERT(masterPickCount > 0);
		if(masterPickCount > 0) {
			masterPickCount--;
		}

		user->replicate.masterPick = true;
	}
}

void RoomInstance::ResetMasters(User* user)
{
	bool changed = false;
	if(user->masterMain != ClassType::INVALID) {
		u8& masterPickCount = teamMasterPickCount[user->team][(i32)user->masterMain];
		DBG_ASSERT(masterPickCount > 0);
		if(masterPickCount > 0) {
			masterPickCount--;
		}

		user->masterMain = ClassType::INVALID;
		changed = true;
	}
	if(user->masterSub != ClassType::INVALID) {
		u8& masterPickCount = teamMasterPickCount[user->team][(i32)user->masterSub];
		DBG_ASSERT(masterPickCount > 0);
		if(masterPickCount > 0) {
			masterPickCount--;
		}

		user->masterSub = ClassType::INVALID;
		changed = true;
	}

	if(changed) {
		user->replicate.masterPick = true;
	}
}
