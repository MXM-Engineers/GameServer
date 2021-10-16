#include <common/platform.h>
#include <common/base.h>
#include <common/network.h>
#include <common/inner_protocol.h>
#include <common/protocol.h>
#include <common/packet_serialize.h>
#include <EAStdC/EAScanf.h>
#include <EASTL/hash_map.h>
#include "config.h"

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
			PlayServer = 2
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

		eastl::fixed_vector<Member,5,false> memberList;

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
			};

			const WideString name;
			const AccountUID accountUID;
			const ClientHandle instanceChd;
			PlayerStatus status = PlayerStatus::Unknown;
			Team team;
			u8 isBot = false;

			eastl::array<Master,2> masters;

			Player(const WideString& name_, AccountUID accountUID_, ClientHandle instanceChd_):
				name(name_),
				accountUID(accountUID_),
				instanceChd(instanceChd_)
			{}
		};

		const SortieUID UID;
		eastl::fixed_list<Player,16,false> playerList;
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

			connList.erase(connMap.at(clientHd));
			connMap.erase(clientHd);
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

		Connection& conn = *connMap.at(clientHd);

		switch(conn.type) {
			case Connection::Type::Undecided: OnPacketUndecided(conn, header, packetData, packetSize); break;
			case Connection::Type::HubServer: OnPacketHub(conn, header, packetData, packetSize); break;
			case Connection::Type::PlayServer: OnPacketPlay(conn, header, packetData, packetSize); break;

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

			case In::PQ_Handshake::NET_ID: {
				NT_LOG("[client%x] PQ_Handshake", conn.clientHd);

				// TODO: check white list
				// TODO: validate args
				const In::PQ_Handshake& packet = SafeCast<In::PQ_Handshake>(packetData, packetSize);
				conn.type = Connection::Type::PlayServer;
				conn.listenPort = packet.listenPort;

				In::MR_Handshake resp;
				resp.result = 1;
				SendPacket(conn.clientHd, resp);

				LOG("[client%x] New Play connection", conn.clientHd);
			} break;

			default: {
				ASSERT_MSG(0, "case not handled");
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

			case In::HQ_PartyEnqueue::NET_ID: {
				NT_LOG("[hub%x] %s", conn.clientHd, PacketSerialize<In::HQ_PartyEnqueue>(packetData, packetSize));
				const In::HQ_PartyEnqueue& packet = SafeCast<In::HQ_PartyEnqueue>(packetData, packetSize);

				// TODO: validate args?
				matchingPartyList.push_back(packet.partyUID);

				const Party& party = *partyMap.at(packet.partyUID);
				eastl::fixed_set<ClientHandle,5,false> setInstance;
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
				Room& room = *roomMap.at(packet.sortieUID);
				foreach(pl, room.playerList) {
					if(pl->accountUID == packet.accountUID) {
						pl->status = Room::PlayerStatus::RoomFoundAck;
						found = true;
					}
				}

				if(!found) {
					WARN("Player not found in room (accountUID=%u sortieUID=%llu)", packet.accountUID, packet.sortieUID);
				}
			} break;

			case In::HN_PlayerRoomConfirm::NET_ID: {
				NT_LOG("[hub%x] %s", conn.clientHd, PacketSerialize<In::HN_PlayerRoomConfirm>(packetData, packetSize));
				const In::HN_PlayerRoomConfirm& packet = SafeCast<In::HN_PlayerRoomConfirm>(packetData, packetSize);

				// TODO: validate args?

				bool found = false;
				Room& room = *roomMap.at(packet.sortieUID);
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
					WARN("Player not found in room (accountUID=%u sortieUID=%llu)", packet.accountUID, packet.sortieUID);
				}
			} break;

			case In::HQ_RoomCreateGame::NET_ID: {
				NT_LOG("[hub%x] %s", conn.clientHd, PacketSerialize<In::HQ_RoomCreateGame>(packetData, packetSize));
				const In::HQ_RoomCreateGame& packet = SafeCast<In::HQ_RoomCreateGame>(packetData, packetSize);

				// TODO: validate args?

				Room& room = *roomMap.at(packet.sortieUID);
				for(int i = 0; i < packet.playerCount; i++) {
					const In::HQ_RoomCreateGame::Player pp = packet.players[i];

					bool found = false;
					foreach(p, room.playerList) {
						if(p->accountUID == pp.accountUID) {
							found = true;
							p->masters[0].classType = pp.masters[0];
							p->masters[1].classType = pp.masters[1];
							p->masters[0].skin = pp.skins[0];
							p->masters[1].skin = pp.skins[1];
							p->masters[0].skills[0] = pp.skills[0];
							p->masters[0].skills[1] = pp.skills[1];
							p->masters[1].skills[0] = pp.skills[2];
							p->masters[1].skills[1] = pp.skills[3];
						}
					}

					ASSERT_MSG(found, "player not found in room");
				}

				RoomCreateGame(room);
			} break;

			default: {
				ASSERT_MSG(0, "case not handled");
			}
		}
	}

	void OnPacketPlay(Connection& conn, const NetHeader& header, const u8* packetData, const i32 packetSize)
	{
		switch(header.netID) {
			case In::PR_GameCreated::NET_ID: {
				NT_LOG("[play%x] %s", conn.clientHd, PacketSerialize<In::PR_GameCreated>(packetData, packetSize));
				const In::PR_GameCreated& packet = SafeCast<In::PR_GameCreated>(packetData, packetSize);

				// TODO: validate args?

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
		foreach_const(puid, matchingPartyList) {
			const Party& party = *partyMap.at(*puid);

			// create room
			roomList.emplace_back(nextSortieUID);
			Room& room = *(--roomList.end());
			nextSortieUID = SortieUID((u64)nextSortieUID + 1);
			roomMap.emplace(room.UID, --roomList.end());

			foreach_const(pl, party.memberList) {
				Room::Player player(pl->name, pl->accountUID, pl->instanceChd);
				player.team = Team::RED;
				room.playerList.push_back(player);
				room.teamRed.push_back(--room.playerList.end());
			}

			// fill empty slots with bots
			i32 botID = 1;
			while(room.teamRed.size() < 3) {
				Room::Player player(LFMT(L"Bot%d", botID++), AccountUID::INVALID, ClientHandle::INVALID);
				player.team = Team::RED;
				player.isBot = true;

				room.playerList.push_back(player);
				room.teamRed.push_back(--room.playerList.end());
			}
			while(room.teamBlue.size() < 3) {
				Room::Player player(LFMT(L"Bot%d", botID++), AccountUID::INVALID, ClientHandle::INVALID);
				player.team = Team::BLUE;
				player.isBot = true;

				room.playerList.push_back(player);
				room.teamBlue.push_back(--room.playerList.end());
			}

			// send 'match found' packet to all instances
			eastl::fixed_set<ClientHandle,16,false> setInstance;
			foreach_const(pl, room.playerList) {
				if(pl->instanceChd != ClientHandle::INVALID) {
					setInstance.insert(pl->instanceChd);
				}
			}

			foreach_const(chd, setInstance) {
				In::MN_MatchingPartyFound resp;
				resp.partyUID = *puid;
				resp.sortieUID = room.UID;

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
		matchingPartyList.clear();
	}

	void UpdateRooms()
	{
		eastl::fixed_vector<SortieUID,128> removeList;

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
				packet.playerCount = 0;


				foreach_const(pl, r->playerList) {
					In::RoomUser player;
					player.name.Copy(pl->name);
					player.accountUID = pl->accountUID;
					player.team = (u8)pl->team;
					player.isBot = pl->isBot;
					packet.playerList[packet.playerCount++] = player;
				}

				foreach(pl, r->playerList) {
					if(!pl->isBot) {
						SendPacket(pl->instanceChd, packet);
					}
				}
			}
		}
	}

	void RoomCreateGame(Room& room)
	{
		In::MQ_CreateGame packet;
		packet.sortieUID = room.UID;
		packet.playerCount = 0;
		packet.spectatorCount = 0;

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
				packet.players[packet.playerCount++] = player;
			}
		}

		// TODO: choose game server based on load
		Connection* conn = GetAvailablePlayServer();
		ASSERT(conn);
		SendPacket(conn->clientHd, packet);
	}

	Connection* GetAvailablePlayServer()
	{
		foreach(c, connList) {
			if(c->type == Connection::Type::PlayServer) {
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
	EA::Thread::SetThreadAffinityMask(1 << (i32)CoreAffinity::MATCHMAKER);

	Matchmaker& mm = *(Matchmaker*)pData;
	bool r = mm.Init();
	if(!r) {
		LOG("ERROR: Failed to init matchmaker");
		ShutdownServer();
		return 1;
	}

	const f64 UPDATE_RATE_MS = (1.0/120.0) * 1000.0;
	const Time startTime = TimeNow();
	Time t0 = startTime;

	while(mm.server.running)
	{
		Time t1 = TimeNow();
		mm.localTime = TimeDiff(startTime, t1);
		f64 delta = TimeDiffMs(TimeDiff(t0, t1));

		if(delta > UPDATE_RATE_MS) {
			ProfileNewFrame("Matchmaker");
			mm.Update();
			t0 = t1;
		}
		else {
			EA::Thread::ThreadSleep((EA::Thread::ThreadTime)(UPDATE_RATE_MS - delta)); // yield
			// EA::Thread::ThreadSleep(EA::Thread::kTimeoutYield);
			// Sleep on windows is notoriously innacurate, we'll probably need to "just yield"
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
