#include <common/platform.h>
#include <common/base.h>
#include <common/utils.h>
#include <common/network.h>
#include <common/inner_protocol.h>
#include <common/protocol.h>
#include <common/packet_serialize.h>
#include <EAStdC/EAScanf.h>
#include <EASTL/hash_map.h>
#include <EASTL/algorithm.h>
#include "config.h"

static const i32 MAX_TEAM_SIZE = 5;
static const f64 HYP_ROOM_CONFIRM_TIMEOUT_SEC = 20.0;

Server* g_Server = nullptr;
Listener* g_Listener = nullptr;

inline void ShutdownServer()
{
	g_Server->running = false;
	g_Listener->Stop();
}

intptr_t ThreadMatchmaker(void* pData);

enum class Team: u8
{
	RED = 0,
	BLUE,
	SPECTATOR
};

struct Matchmaker
{
	Server& server;
	Time localTime;
	EA::Thread::Thread thread;

	struct Connection
	{
		enum class Type: u8
		{
			Undecided = 0,
			HubServer = 1,
			GameServer = 2
		};

		Type type; // TODO: timeout when undecided for a while
		ClientHandle clientHd;
		eastl::array<u8,4> ip;
		u16 listenPort;
	};

	struct Party
	{
		const PartyUID UID;

		struct Member
		{
			WideString name;
			AccountUID accountUID;
			ClientHandle instanceChd;
		};

		eastl::fixed_vector<Member,10,false> memberList;
		In::CustomTeams custom = {};
		u8 draft = 0;
		AreaIndex areaIndex = AreaIndex(0);
		StageIndex stageIndex = StageIndex(0);
		MapIndex mapIndex = MapIndex(0);
		GameType gameType = GameType::PVP_NORMAL;
		i32 teamSize = 0;

		Party(PartyUID UID_): UID(UID_) {}
	};

	struct Room
	{
		enum class PlayerStatus: u8 {
			Unknown = 0,
			RoomFoundAck,
			Accepted,
			Refused,
			InLobby
		};

		struct Player
		{
			struct Master
			{
				ClassType classType = ClassType::NONE;
				SkinIndex skin = SkinIndex::DEFAULT;
				eastl::array<SkillID,2> skills = {
					SkillID::INVALID,
					SkillID::INVALID
				};
				WeaponIndex weapon = WeaponIndex::INVALID;
				i32 weaponGrade = 0;
				u8 masterGearNo = 1;
				i32 characterType = 1;
			};

			const WideString name;
			const AccountUID accountUID;
			const ClientHandle instanceChd;
			PlayerStatus status = PlayerStatus::Unknown;
			Team team;
			u8 isBot = false;
			In::StrGuildTag guildTag = {};
			SkillID stageSkill = SkillID::INVALID;
			u16 guildBoostGold = 0;
			u16 guildBoostExp = 0;

			eastl::array<Master,2> masters;

			Player(const WideString& name_, AccountUID accountUID_, ClientHandle instanceChd_):
				name(name_),
				accountUID(accountUID_),
				instanceChd(instanceChd_)
			{}
		};

		const SortieUID UID;
		AreaIndex areaIndex = AreaIndex(0);
		StageIndex stageIndex = StageIndex(0);
		MapIndex mapIndex = MapIndex(0);
		GameType gameType = GameType::PVP_NORMAL;
		u8 draft = 0;
		Time tCreated = Time::ZERO;
		eastl::fixed_vector<Player,16,false> playerList;
		eastl::fixed_vector<decltype(playerList)::iterator,5> teamRed;
		eastl::fixed_vector<decltype(playerList)::iterator,5> teamBlue;
		eastl::fixed_vector<decltype(playerList)::iterator,6> teamSpectators;

		Room(SortieUID UID_): UID(UID_) {}
	};

	eastl::fixed_list<Connection, 32> connList;
	hash_map<ClientHandle, decltype(connList)::iterator, 32> connMap;

	eastl::fixed_list<Party,2048> partyList;
	hash_map<PartyUID, decltype(partyList)::iterator, 2048> partyMap;
	eastl::fixed_vector<PartyUID,2048> matchingPartyList;
	Time matchingStartTime = Time::ZERO;

	eastl::fixed_list<Room,2048> roomList;
	hash_map<SortieUID, decltype(roomList)::iterator, 2048> roomMap;

	GrowableBuffer recvDataBuff;

	PartyUID nextPartyUID = PartyUID(1);
	SortieUID nextSortieUID = SortieUID(1); // TODO: load from database

	Matchmaker(Server& server_):
		server(server_)
	{
		thread.Begin(ThreadMatchmaker, this);
	}

	bool Init()
	{
		recvDataBuff.Init(10 * (1024*1024)); // 10MB
		return true;
	}

	void Update()
	{
		// handle client connections
		eastl::fixed_vector<eastl::pair<ClientHandle,Server::ClientInfo>,128> clientConnectedList;
		server.TransferConnectedClientListEx(&clientConnectedList);

		foreach_const(cl, clientConnectedList) {
			Connection conn;
			conn.type = Connection::Type::Undecided;
			conn.clientHd = cl->first;
			conn.ip = cl->second.ip;
			conn.listenPort = 0;

			connList.push_back(conn);
			connMap.emplace(conn.clientHd, --connList.end());
		}

		// handle client disconnections
		eastl::fixed_vector<ClientHandle,128> clientDisconnectedList;
		server.TransferDisconnectedClientList(&clientDisconnectedList);

		foreach_const(cl, clientDisconnectedList) {
			const ClientHandle clientHd = *cl;
			const auto found = connMap.find(clientHd);
			if(found == connMap.end()) continue;

			connList.erase(found->second);
			connMap.erase(found);
		}

		server.TransferAllReceivedData(&recvDataBuff);

		ConstBuffer buff(recvDataBuff.data, recvDataBuff.size);
		while(buff.CanRead(sizeof(Server::RecvChunkHeader))) {
			const Server::RecvChunkHeader& chunkInfo = buff.Read<Server::RecvChunkHeader>();
			const u8* data = buff.ReadRaw(chunkInfo.len);

			// handle each packet in chunk
			ConstBuffer reader(data, chunkInfo.len);
			if(!reader.CanRead(sizeof(NetHeader))) {
				WARN("Packet too small (clientHd=%u size=%d)", chunkInfo.clientHd, chunkInfo.len);
				server.DisconnectClient(chunkInfo.clientHd);
				continue;
			}

			while(reader.CanRead(sizeof(NetHeader))) {
				const NetHeader& header = reader.Read<NetHeader>();
				const i32 packetDataSize = header.size - sizeof(NetHeader);

				if(!reader.CanRead(packetDataSize)) {
					WARN("Packet header size differs from actual data size (clientHd=%u size=%d)", chunkInfo.clientHd, header.size);
					server.DisconnectClient(chunkInfo.clientHd);
					break;
				}

				if(Config().TraceNetwork) {
					fileSaveBuff(FormatPath(FMT("trace/mm_%d_cl_%d.raw", server.packetCounter, header.netID)), &header, header.size);
					server.packetCounter++;
				}

				const u8* packetData = reader.ReadRaw(packetDataSize);
				ClientHandlePacket(chunkInfo.clientHd, header, packetData);
			}
		}

		recvDataBuff.Clear();

		MatchParties();
		UpdateRooms();
	}

	void ClientHandlePacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData)
	{
		const i32 packetSize = header.size - sizeof(NetHeader);

		const auto found = connMap.find(clientHd);
		if(found == connMap.end()) return;
		Connection& conn = *found->second;

		switch(conn.type) {
			case Connection::Type::Undecided: OnPacketUndecided(conn, header, packetData, packetSize); break;
			case Connection::Type::HubServer: OnPacketHub(conn, header, packetData, packetSize); break;
			case Connection::Type::GameServer: OnPacketGame(conn, header, packetData, packetSize); break;

			default: {
				ASSERT_MSG(0, "case not handled");
			}
		}
	}

	void OnPacketUndecided(Connection& conn, const NetHeader& header, const u8* packetData, const i32 packetSize)
	{
		switch(header.netID) {
			case In::HQ_Handshake::NET_ID: {
				NT_LOG("[client%x] HQ_Handshake", conn.clientHd);

				// TODO: check white list
				// TODO: validate args
				const In::HQ_Handshake& packet = SafeCast<In::HQ_Handshake>(packetData, packetSize);
				conn.type = Connection::Type::HubServer;

				In::MR_Handshake resp;
				resp.result = 1;
				SendPacket(conn.clientHd, resp);

				LOG("[client%x] New Hub connection", conn.clientHd);
			} break;

			case In::GQ_Handshake::NET_ID: {
				NT_LOG("[client%x] GQ_Handshake", conn.clientHd);

				// TODO: check white list
				// TODO: validate args
				const In::GQ_Handshake& packet = SafeCast<In::GQ_Handshake>(packetData, packetSize);
				conn.type = Connection::Type::GameServer;
				conn.listenPort = packet.listenPort;

				In::MR_Handshake resp;
				resp.result = 1;
				SendPacket(conn.clientHd, resp);

				LOG("[client%x] New Game connection", conn.clientHd);
			} break;

			default: {
				WARN("[client%x] Unhandled packet (netID=%u size=%u)", conn.clientHd, header.netID, header.size);
				server.DisconnectClient(conn.clientHd);
			}
		}
	}

	void OnPacketHub(Connection& conn, const NetHeader& header, const u8* packetData, const i32 packetSize)
	{
		switch(header.netID) {
			case In::HQ_PartyCreate::NET_ID: {
				NT_LOG("[hub%x] %s", conn.clientHd, PacketSerialize<In::HQ_PartyCreate>(packetData, packetSize));
				const In::HQ_PartyCreate& packet = SafeCast<In::HQ_PartyCreate>(packetData, packetSize);

				// TODO: validate args?

				// create party
				Party party(nextPartyUID);
				nextPartyUID = PartyUID((u32)nextPartyUID + 1);

				Party::Member member;
				member.name.assign(packet.name.data, packet.name.len);
				member.accountUID = packet.leader;
				member.instanceChd = conn.clientHd;
				party.memberList.push_back(member);

				partyList.push_back(party);
				partyMap.emplace(party.UID, --partyList.end());

				// send confirmation
				In::MR_PartyCreated resp;
				resp.result = 1;
				resp.partyUID = party.UID;
				resp.leader = packet.leader;
				SendPacket(conn.clientHd, resp);
			} break;

			case In::HQ_PartyAddMember::NET_ID: {
				NT_LOG("[hub%x] %s", conn.clientHd, PacketSerialize<In::HQ_PartyAddMember>(packetData, packetSize));
				const In::HQ_PartyAddMember& packet = SafeCast<In::HQ_PartyAddMember>(packetData, packetSize);
				auto f = partyMap.find(packet.partyUID);
				if(f == partyMap.end()) break;
				Party& party = *f->second;
				bool known = false;
				foreach_const(m, party.memberList) known = known || m->accountUID == packet.accountUID;
				if(known || party.memberList.full()) break;
				Party::Member member;
				member.name.assign(packet.name.data, packet.name.len);
				member.accountUID = packet.accountUID;
				member.instanceChd = conn.clientHd;
				party.memberList.push_back(member);
				LOG("Party %u :: %ls joined (%d member(s))", (u32)party.UID, member.name.data(), (i32)party.memberList.size());
			} break;

			case In::HQ_PartyRemoveMember::NET_ID: {
				NT_LOG("[hub%x] %s", conn.clientHd, PacketSerialize<In::HQ_PartyRemoveMember>(packetData, packetSize));
				const In::HQ_PartyRemoveMember& packet = SafeCast<In::HQ_PartyRemoveMember>(packetData, packetSize);
				auto f = partyMap.find(packet.partyUID);
				if(f == partyMap.end()) break;
				Party& party = *f->second;
				party.memberList.erase(eastl::remove_if(party.memberList.begin(), party.memberList.end(), [&](const Party::Member& m) { return m.accountUID == packet.accountUID; }), party.memberList.end());
				if(party.memberList.empty()) matchingPartyList.erase(eastl::remove(matchingPartyList.begin(), matchingPartyList.end(), packet.partyUID), matchingPartyList.end());
			} break;

			case In::HQ_PartyCancelMatching::NET_ID: {
				NT_LOG("[hub%x] %s", conn.clientHd, PacketSerialize<In::HQ_PartyCancelMatching>(packetData, packetSize));
				const In::HQ_PartyCancelMatching& packet = SafeCast<In::HQ_PartyCancelMatching>(packetData, packetSize);
				matchingPartyList.erase(eastl::remove(matchingPartyList.begin(), matchingPartyList.end(), packet.partyUID), matchingPartyList.end());
			} break;

			case In::HQ_PartyEnqueue::NET_ID: {
				NT_LOG("[hub%x] %s", conn.clientHd, PacketSerialize<In::HQ_PartyEnqueue>(packetData, packetSize));
				const In::HQ_PartyEnqueue& packet = SafeCast<In::HQ_PartyEnqueue>(packetData, packetSize);

				if(eastl::find(matchingPartyList.begin(), matchingPartyList.end(), packet.partyUID) != matchingPartyList.end()) {
					WARN("Party is already in the matching queue (partyUID=%u)", (u32)packet.partyUID);
					break;
				}

				Party& party = *partyMap.at(packet.partyUID);
				const i32 teamSize = packet.teamSize > 0 ? MIN((i32)packet.teamSize, MAX_TEAM_SIZE) : Config().TeamSize;
				const bool pve = packet.gameType == GameType::PVE_NORMAL || packet.gameType == GameType::PVE_RANDOM || packet.gameType == GameType::PVE_TICKET;
				if(!pve && packet.custom.count == 0 && (i32)party.memberList.size() > teamSize * 2) {
					WARN("Party is too large for a room (partyUID=%u, members=%d)", (u32)packet.partyUID, (i32)party.memberList.size());
					break;
				}
				ASSERT(packet.areaIndex != AreaIndex(0));
				ASSERT(packet.stageIndex != StageIndex(0));
				ASSERT(packet.mapIndex != MapIndex(0));
				party.areaIndex = packet.areaIndex;
				party.stageIndex = packet.stageIndex;
				party.mapIndex = packet.mapIndex;
				party.gameType = packet.gameType;
				party.teamSize = packet.teamSize;
				party.custom = packet.custom;
				party.draft = packet.draft;
				matchingPartyList.push_back(packet.partyUID);

				eastl::fixed_set<ClientHandle,10,false> setInstance;
				foreach_const(mem, party.memberList) {
					setInstance.insert(mem->instanceChd);
				}

				foreach_const(it, setInstance) {
					In::MR_PartyEnqueued resp;
					resp.result = 1;
					resp.partyUID = packet.partyUID;
					SendPacket(*it, resp);
				}
			} break;

			case In::HN_PlayerRoomFound::NET_ID: {
				NT_LOG("[hub%x] %s", conn.clientHd, PacketSerialize<In::HN_PlayerRoomFound>(packetData, packetSize));
				const In::HN_PlayerRoomFound& packet = SafeCast<In::HN_PlayerRoomFound>(packetData, packetSize);

				// TODO: validate args?

				bool found = false;
				auto roomIt = roomMap.find(packet.sortieUID);
				if(roomIt == roomMap.end()) break;
				Room& room = *roomIt->second;
				foreach(pl, room.playerList) {
					if(pl->accountUID == packet.accountUID) {
						pl->status = Room::PlayerStatus::RoomFoundAck;
						found = true;
					}
				}

				if(!found) {
					WARN("Player not found in room (accountUID=0x%08x sortieUID=%llu)", packet.accountUID, packet.sortieUID);
				}
			} break;

			case In::HN_PlayerRoomConfirm::NET_ID: {
				NT_LOG("[hub%x] %s", conn.clientHd, PacketSerialize<In::HN_PlayerRoomConfirm>(packetData, packetSize));
				const In::HN_PlayerRoomConfirm& packet = SafeCast<In::HN_PlayerRoomConfirm>(packetData, packetSize);

				// TODO: validate args?

				bool found = false;
				auto roomIt = roomMap.find(packet.sortieUID);
				if(roomIt == roomMap.end()) break;
				Room& room = *roomIt->second;
				foreach(pl, room.playerList) {
					if(pl->accountUID == packet.accountUID) {
						if(packet.confirm) {
							pl->status = Room::PlayerStatus::Accepted;
						}
						else {
							pl->status = Room::PlayerStatus::Refused;
						}

						found = true;
					}
				}

				if(!found) {
					WARN("Player not found in room (accountUID=0x%08x sortieUID=%llu)", packet.accountUID, packet.sortieUID);
				}
			} break;

			case In::HQ_RoomCreateGame::NET_ID: {
				NT_LOG("[hub%x] %s", conn.clientHd, PacketSerialize<In::HQ_RoomCreateGame>(packetData, packetSize));
				const In::HQ_RoomCreateGame& packet = SafeCast<In::HQ_RoomCreateGame>(packetData, packetSize);

				// TODO: validate args?
				// FIXME: hacky? can we guarantee that the player arrays match?

				Room& room = *roomMap.at(packet.sortieUID);
				for(int i = 0; i < packet.playerCount; i++) {
					const In::HQ_RoomCreateGame::Player pp = packet.players[i];
					Room::Player& p = room.playerList[i];
					ASSERT(pp.accountUID == p.accountUID);

					p.masters[0].classType = pp.masters[0];
					p.masters[1].classType = pp.masters[1];
					p.masters[0].skin = pp.skins[0];
					p.masters[1].skin = pp.skins[1];
					p.masters[0].skills[0] = pp.skills[0];
					p.masters[0].skills[1] = pp.skills[1];
					p.masters[1].skills[0] = pp.skills[2];
					p.masters[1].skills[1] = pp.skills[3];
					p.masters[0].weapon = pp.weapons[0];
					p.masters[1].weapon = pp.weapons[1];
					p.masters[0].weaponGrade = pp.weaponGrades[0];
					p.masters[1].weaponGrade = pp.weaponGrades[1];
					p.masters[0].masterGearNo = pp.masterGearNo[0];
					p.masters[1].masterGearNo = pp.masterGearNo[1];
					p.masters[0].characterType = pp.characterType[0];
					p.masters[1].characterType = pp.characterType[1];
					p.guildTag = pp.guildTag;
					p.stageSkill = pp.stageSkill;
					p.guildBoostGold = pp.guildBoostGold;
					p.guildBoostExp = pp.guildBoostExp;
				}

				RoomCreateGame(room);
			} break;

			default: {
				ASSERT_MSG(0, "case not handled");
			}
		}
	}

	void OnPacketGame(Connection& conn, const NetHeader& header, const u8* packetData, const i32 packetSize)
	{
		switch(header.netID) {
			case In::GN_GameEnded::NET_ID: {
				const In::GN_GameEnded& packet = SafeCast<In::GN_GameEnded>(packetData, packetSize);
				LOG("[game%x] Game ended (sortieUID=%llu)", conn.clientHd, (u64)packet.sortieUID);
				In::MN_GameEnded notify;
				notify.sortieUID = packet.sortieUID;
				foreach_const(c, connList) {
					if(c->type == Connection::Type::HubServer) SendPacket(c->clientHd, notify);
				}
			} break;

			case In::GR_GameCreated::NET_ID: {
				NT_LOG("[game%x] %s", conn.clientHd, PacketSerialize<In::GR_GameCreated>(packetData, packetSize));
				const In::GR_GameCreated& packet = SafeCast<In::GR_GameCreated>(packetData, packetSize);

				// TODO: validate args?

				if(roomMap.find(packet.sortieUID) == roomMap.end()) {
					WARN("Room not found (sortieUID=%llu) // if you run dev mode this is normal", packet.sortieUID);
					return;
				}

				Room& room = *roomMap.at(packet.sortieUID);

				// send 'match created' packet to all instances
				eastl::fixed_set<ClientHandle,16,false> setInstance;
				foreach_const(pl, room.playerList) {
					if(pl->instanceChd != ClientHandle::INVALID) {
						setInstance.insert(pl->instanceChd);
					}
				}

				foreach_const(chd, setInstance) {
					In::MN_MatchCreated resp;
					resp.sortieUID = room.UID;
					resp.serverIp = conn.ip;
					if(conn.ip[0] == 127 && Config().GameServerPublicIP[0] != 0) {
						for(int i = 0; i < 4; i++) resp.serverIp[i] = (u8)Config().GameServerPublicIP[i];
					}
					resp.serverPort = conn.listenPort;
					SendPacket(*chd, resp);
				}

				// done with the room, remove it
				auto r = roomMap.find(room.UID);
				roomList.erase(r->second);
				roomMap.erase(r);

				// TODO: retry if this packet is never received
			} break;

			default: {
				ASSERT_MSG(0, "case not handled");
			}
		}
	}

	void MatchParties()
	{
		if(matchingPartyList.empty()) {
			matchingStartTime = Time::ZERO;
			return;
		}

		const Time now = TimeRelNow();
		if(matchingStartTime == Time::ZERO) matchingStartTime = now;

		const Party& firstParty = *partyMap.at(matchingPartyList.front());
		const i32 teamSize = firstParty.teamSize > 0 ? MIN(firstParty.teamSize, MAX_TEAM_SIZE) : Config().TeamSize;
		const bool pve = firstParty.gameType == GameType::PVE_NORMAL || firstParty.gameType == GameType::PVE_RANDOM || firstParty.gameType == GameType::PVE_TICKET;
		const In::CustomTeams frontCustom = firstParty.custom;
		const bool custom = frontCustom.count > 0;
		const bool soloRoom = Config().SoloRooms || pve || custom;

		i32 humanCount = 0;
		foreach_const(puid, matchingPartyList) {
			const Party& party = *partyMap.at(*puid);
			if(party.stageIndex != firstParty.stageIndex) continue;
			humanCount += party.memberList.size();
		}
		if(!soloRoom && humanCount < teamSize * 2 && TimeDiffSec(TimeDiff(matchingStartTime, now)) < (f64)Config().MatchingWaitSec) return;
		matchingStartTime = Time::ZERO;
		decltype(matchingPartyList) laterParties;
		decltype(matchingPartyList) placedParties;

		roomList.emplace_back(nextSortieUID);
		Room& room = *(--roomList.end());
		nextSortieUID = SortieUID((u64)nextSortieUID + 1);
		roomMap.emplace(room.UID, --roomList.end());
		ASSERT(firstParty.areaIndex != AreaIndex(0));
		ASSERT(firstParty.stageIndex != StageIndex(0));
		ASSERT(firstParty.mapIndex != MapIndex(0));
		room.areaIndex = firstParty.areaIndex;
		room.stageIndex = firstParty.stageIndex;
		room.mapIndex = firstParty.mapIndex;
		room.gameType = firstParty.gameType;
		room.draft = firstParty.draft;
		room.tCreated = now;

		foreach_const(puid, matchingPartyList) {
			const Party& party = *partyMap.at(*puid);
			const bool first = puid == matchingPartyList.begin();
			const i32 memberCount = party.memberList.size();
			const i32 redCount = room.teamRed.size();
			const i32 blueCount = room.teamBlue.size();
			const bool compatible = first || (!soloRoom && party.stageIndex == firstParty.stageIndex);
			const bool fitsRed = redCount + memberCount <= teamSize;
			const bool fitsBlue = blueCount + memberCount <= teamSize;
			if(!compatible || (!first && !fitsRed && !fitsBlue)) {
				laterParties.push_back(*puid);
				continue;
			}

			if(custom) {
				for(i32 i = 0; i < frontCustom.count; i++) {
					const Party::Member* found = nullptr;
					foreach_const(pl, party.memberList) if(pl->accountUID == frontCustom.accounts[i]) found = &*pl;
					if(!found) continue;
					const bool toRed = frontCustom.teams[i] == (u8)Team::RED;
					Room::Player player(found->name, found->accountUID, found->instanceChd);
					player.team = toRed ? Team::RED : Team::BLUE;
					room.playerList.push_back(player);
					if(toRed) room.teamRed.push_back(&room.playerList.back());
					else room.teamBlue.push_back(&room.playerList.back());
				}
				placedParties.push_back(*puid);
				continue;
			}

			const bool red = pve || (fitsRed && (!fitsBlue || redCount <= blueCount)) || (!fitsRed && !fitsBlue && redCount <= blueCount);
			foreach_const(pl, party.memberList) {
				if(room.teamRed.size() >= teamSize && room.teamBlue.size() >= teamSize) break;
				const bool toRed = pve || (red ? room.teamRed.size() < teamSize : room.teamBlue.size() >= teamSize);
				Room::Player player(pl->name, pl->accountUID, pl->instanceChd);
				player.team = toRed ? Team::RED : Team::BLUE;
				room.playerList.push_back(player);
				if(toRed) room.teamRed.push_back(&room.playerList.back());
				else room.teamBlue.push_back(&room.playerList.back());
			}
			placedParties.push_back(*puid);
		}
		ASSERT(!placedParties.empty());

		LOG("MatchParties :: creating room %llu with %d player(s) from %d party(ies)", (u64)room.UID, (i32)room.playerList.size(), (i32)placedParties.size());

		i32 botID = 1;
		for(i32 t = 0; custom && t < 2; t++) {
			for(u8 b = 0; b < frontCustom.bots[t]; b++) {
				Room::Player player(LFMT(L"Bot%d", botID++), AccountUID::INVALID, ClientHandle::INVALID);
				player.team = t == 0 ? Team::RED : Team::BLUE;
				player.isBot = true;
				room.playerList.push_back(player);
				if(t == 0) room.teamRed.push_back(&room.playerList.back());
				else room.teamBlue.push_back(&room.playerList.back());
			}
		}
		while(!pve && !custom && room.teamRed.size() < teamSize) {
			Room::Player player(LFMT(L"Bot%d", botID++), AccountUID::INVALID, ClientHandle::INVALID);
			player.team = Team::RED;
			player.isBot = true;

			room.playerList.push_back(player);
			room.teamRed.push_back(&room.playerList.back());
		}
		while(!pve && !custom && room.teamBlue.size() < teamSize) {
			Room::Player player(LFMT(L"Bot%d", botID++), AccountUID::INVALID, ClientHandle::INVALID);
			player.team = Team::BLUE;
			player.isBot = true;

			room.playerList.push_back(player);
			room.teamBlue.push_back(&room.playerList.back());
		}

		foreach_const(puid, placedParties) {
			const Party& party = *partyMap.at(*puid);

			eastl::fixed_set<ClientHandle,16,false> setInstance;
			foreach_const(mem, party.memberList) {
				setInstance.insert(mem->instanceChd);
			}

			foreach_const(chd, setInstance) {
				In::MN_MatchingPartyFound resp;
				resp.partyUID = *puid;
				resp.sortieUID = room.UID;
				resp.gameType = room.gameType;

				resp.playerCount = 0;
				foreach(pl, room.playerList) {
					In::RoomUser player;
					player.name.Copy(pl->name);
					player.accountUID = pl->accountUID;
					player.team = (u8)pl->team;
					player.isBot = pl->isBot;
					resp.playerList[resp.playerCount++] = player;
				}

				SendPacket(*chd, resp);
			}
		}
		matchingPartyList = laterParties;
	}

	void UpdateRooms()
	{
		eastl::fixed_vector<SortieUID,128> removeList;
		const Time now = TimeRelNow();

		foreach(r, roomList) {
			// check if all players have accepted the match
			bool allAccepted = true;

			foreach_const(pl, r->playerList) {
				if(!pl->isBot && pl->status != Room::PlayerStatus::Accepted) {
					allAccepted = false;
					break;
				}
			}

			if(allAccepted) {
				foreach(pl, r->playerList) {
					pl->status = Room::PlayerStatus::InLobby;
				}

				In::MN_RoomCreated packet;
				packet.sortieUID = r->UID;
				packet.stageIndex = r->stageIndex;
				packet.draft = r->draft;
				packet.playerCount = 0;

				foreach_const(pl, r->playerList) {
					In::RoomUser player;
					player.name.Copy(pl->name);
					player.accountUID = pl->accountUID;
					player.team = (u8)pl->team;
					player.isBot = pl->isBot;
					packet.playerList[packet.playerCount++] = player;
				}

				eastl::fixed_set<ClientHandle,16,false> setInstance;
				foreach(pl, r->playerList) {
					if(!pl->isBot) {
						setInstance.insert(pl->instanceChd);
					}
				}
				foreach_const(chd, setInstance) {
					SendPacket(*chd, packet);
				}
			}

			bool expired = false;
			bool inLobby = false;
			foreach_const(pl, r->playerList) {
				if(pl->isBot) continue;
				inLobby = inLobby || pl->status == Room::PlayerStatus::InLobby;
				if(pl->status == Room::PlayerStatus::Accepted || pl->status == Room::PlayerStatus::InLobby) continue;
				expired = expired || pl->status == Room::PlayerStatus::Refused || TimeDiffSec(TimeDiff(r->tCreated, now)) >= HYP_ROOM_CONFIRM_TIMEOUT_SEC;
			}
			if(expired && !inLobby && !removeList.full()) removeList.push_back(r->UID);
		}

		foreach_const(uid, removeList) {
			auto r = roomMap.find(*uid);
			In::MN_RoomCancelled cancel;
			cancel.sortieUID = *uid;
			eastl::fixed_set<ClientHandle,16,false> setInstance;
			foreach_const(pl, r->second->playerList) {
				if(!pl->isBot) setInstance.insert(pl->instanceChd);
			}
			foreach_const(chd, setInstance) {
				SendPacket(*chd, cancel);
			}
			LOG("Room %llu cancelled, match not confirmed", (u64)*uid);
			roomList.erase(r->second);
			roomMap.erase(r);
		}
	}

	void RoomCreateGame(Room& room)
	{
		In::MQ_CreateGame packet;
		packet.sortieUID = room.UID;
		packet.playerCount = 0;
		packet.spectatorCount = 0;
		packet.gameType = room.gameType;
		packet.areaIndex = room.areaIndex;
		packet.stageIndex = room.stageIndex;
		packet.mapIndex = room.mapIndex;
		packet.canEscape = GameTypeCanEscape(packet.gameType);
		packet.surrenderAbleTime = 180000;

		foreach_const(p, room.playerList) {
			if(p->team == Team::SPECTATOR) {
				packet.spectators[packet.spectatorCount++] = p->accountUID;
			}
			else {
				In::MQ_CreateGame::Player player;
				player.name.Copy(p->name);
				player.accountUID = p->accountUID;
				player.team = (u8)p->team;
				player.isBot = p->isBot;
				player.masters[0] = p->masters[0].classType;
				player.masters[1] = p->masters[1].classType;
				player.skins[0] = p->masters[0].skin;
				player.skins[1] = p->masters[1].skin;
				player.skills[0] = p->masters[0].skills[0];
				player.skills[1] = p->masters[0].skills[1];
				player.skills[2] = p->masters[1].skills[0];
				player.skills[3] = p->masters[1].skills[1];
				player.weapons[0] = p->masters[0].weapon;
				player.weapons[1] = p->masters[1].weapon;
				player.weaponGrades[0] = p->masters[0].weaponGrade;
				player.weaponGrades[1] = p->masters[1].weaponGrade;
				player.masterGearNo[0] = p->masters[0].masterGearNo;
				player.masterGearNo[1] = p->masters[1].masterGearNo;
				player.characterType[0] = p->masters[0].characterType;
				player.characterType[1] = p->masters[1].characterType;
				player.guildTag = p->guildTag;
				player.stageSkill = p->stageSkill;
				player.guildBoostGold = p->guildBoostGold;
				player.guildBoostExp = p->guildBoostExp;
				packet.players[packet.playerCount++] = player;
			}
		}

		// TODO: choose game server based on load
		Connection* conn = GetAvailableGameServer();
		ASSERT(conn);
		SendPacket(conn->clientHd, packet);
	}

	Connection* GetAvailableGameServer()
	{
		foreach(c, connList) {
			if(c->type == Connection::Type::GameServer) {
				return &*c;
			}
		}
		return nullptr; // TODO: return nothing when all servers are under heavy load and wait?
	}

	void Cleanup()
	{

	}

	template<typename Packet>
	inline void SendPacket(ClientHandle clientHd, const Packet& packet)
	{
		SendPacketData<Packet>(clientHd, sizeof(packet), &packet);
	}

	template<typename Packet, u32 CAPACITY>
	inline void SendPacket(ClientHandle clientHd, const PacketWriter<Packet,CAPACITY>& writer)
	{
		SendPacketData<Packet>(clientHd, writer.size, writer.data);
	}

	template<typename Packet>
	inline void SendPacketData(ClientHandle clientHd, u16 packetSize, const void* packetData)
	{
		NT_LOG("[client%x] Matchmaker :: %s", clientHd, PacketSerialize<Packet>(packetData, packetSize));
		server.SendPacketData(clientHd, Packet::NET_ID, packetSize, packetData);
	}
};

intptr_t ThreadMatchmaker(void* pData)
{
	ProfileSetThreadName("Matchmaker");
    ThreadSetCoreAffinity((i32)CoreAffinity::MATCHMAKER);

	Matchmaker& mm = *(Matchmaker*)pData;
	bool r = mm.Init();
	if(!r) {
		LOG("ERROR: Failed to init matchmaker");
		ShutdownServer();
		return 1;
	}

	const f64 UPDATE_RATE_MS = (1.0/30.0) * 1000.0;
	const Time startTime = TimeNow();
	Time t0 = startTime;

	while(mm.server.running)
	{
		Time t1 = TimeNow();
		mm.localTime = TimeDiff(startTime, t1);
		f64 delta = TimeDiffMs(TimeDiff(t0, t1));

		if (delta >= UPDATE_RATE_MS) {
			ProfileNewFrame("Matchmaker");
			mm.Update();
			t0 = Time((u64)t0 + (u64)TimeMsToTime(UPDATE_RATE_MS));
		}
		else {
			// Calculate sleep time in milliseconds
			EA::Thread::ThreadTime sleepTime = (EA::Thread::ThreadTime)(UPDATE_RATE_MS - delta);

			if (sleepTime > 1.0) {
				// Sleep for most of the remaining time
				EA::Thread::ThreadSleep(sleepTime - 1);
			}
			// Yield for the final millisecond to improve timing accuracy
			EA::Thread::ThreadSleep(EA::Thread::kTimeoutYield);
		}
	}
	return 0;
}

int main(int argc, char** argv)
{
	PlatformInit();
	LogInit("matchmaker.log");
	LogNetTrafficInit("matchmaker_nt.log"/*, 0x0*/);
	TimeInit();

	// TODO: linux thread affinity (sched_setaffinity)
	ProfileSetThreadName("Main");
	EA::Thread::SetThreadAffinityMask((EA::Thread::ThreadAffinityMask)1 << (i32)CoreAffinity::MAIN);

	LoadConfig();

	LOG(".: Matchmaker :.");

	Config().Print();

	bool r = SetCloseSignalHandler(ShutdownServer);

	if(!r) {
		LOG("ERROR: Could not set control handler");
		return 1;
	}

	static Server server;
	r = server.Init();
	if(!r) {
		LOG("ERROR: failed to initialize server");
		return 1;
	}
	g_Server = &server;
	server.doTraceNetwork = Config().TraceNetwork;

	Listener listen(&server);
	g_Listener = &listen;

	r = listen.Init(Config().ListenPort);
	if(!r) {
		LOG("ERROR: Could not init game listener");
		return 1;
	}

	static Matchmaker matchmaker(server);

	// listen on main thread
	listen.Listen();

	LOG("Cleaning up...");

	matchmaker.Cleanup();
	server.Cleanup();

	SaveConfig();
	LOG("Done.");
	return 0;
}
