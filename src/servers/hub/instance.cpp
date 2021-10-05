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

	LOG("[Room(%llx)] room created (userCount=%d)", sortieUID, userCount);

	for(const NewUser* nu = userlist; nu != userlist+userCount; ++nu) {
		userList.emplace_back(nu->clientHd, nu->accountUID, nu->userID, nu->isBot);
		auto user = --userList.end();

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

			SendPacket(user.clientHd, packet);
		}

		{
			PacketWriter<Sv::SN_MasterRotationInfo> packet;
			packet.Write<i32>(0); // refreshCount

			eastl::fixed_vector<CreatureIndex,200,false> freeList;
			foreach_const(m, content.masters) {
				freeList.push_back(m->ID);
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
}

void RoomInstance::Update(Time localTime_)
{

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

			if(user->masterMain == ClassType::NONE) {
				user->masterMain = selected;
			}
			else {
				user->masterSub = selected;
			}
		} break;

		case Cl::CQ_MasterUnpick::NET_ID: {
			const Cl::CQ_MasterUnpick& packet = SafeCast<Cl::CQ_MasterUnpick>(packetData, packetSize);
			User* user = FindUser(clientHd);
			ASSERT(user);

			ClassType selected = ClassType((u32)packet.localMasterID - (u32)LocalActorID::FIRST_SELF_MASTER);

			if(user->masterMain == selected) {
				user->masterMain = ClassType::NONE;
			}
			else if(user->masterSub == selected) {
				user->masterSub = ClassType::NONE;
			}
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
