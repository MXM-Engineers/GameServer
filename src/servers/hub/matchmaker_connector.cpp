#include "matchmaker_connector.h"
#include <common/inner_protocol.h>

MatchmakerConnector* g_connector = nullptr;

MatchmakerConnector::MatchmakerConnector()
{
	ASSERT(g_connector == nullptr);
	g_connector = this;
}

bool MatchmakerConnector::Init()
{
	packetQueue.Init(10 * (1024*1024)); // 10 MB

	// TODO: load this from somewhere
	const u8 ip[4] = { 127, 0, 0, 1 };
	const u16 port = 13900;
	bool r = conn.async.ConnectTo(ip, port);
	if(!r) {
		LOG("ERROR: Failed to connect to matchmaker server");
		return false;
	}

	conn.async.StartReceiving(); // TODO: move this to ConnectTo()?

	In::HQ_Handshake handshake;
	handshake.magic = In::MagicHandshake;
	conn.SendPacket(handshake);
	return true;
}

// Thread: Coordinator
void MatchmakerConnector::Update()
{
	// handle inner communication
	conn.SendPendingData();

	u8 recvBuff[8192];
	i32 recvLen = 0;
	conn.RecvPendingData(recvBuff, sizeof(recvBuff), &recvLen);

	if(recvLen > 0) {
		packetQueue.Append(recvBuff, recvLen);
	}

	// process queries
	{
		LOCK_MUTEX(mutexQueries);

		foreach_const(q, queries) {
			switch(q->type) {
				case Query::Type::PartyCreate: {
					In::HQ_PartyCreate packet;
					packet.leader = q->PartyCreate.leader;
					conn.SendPacket(packet);
				} break;

				case Query::Type::PartyEnqueue: {
					In::HQ_PartyEnqueue packet;
					packet.partyUID = q->PartyEnqueue.partyUID;
					conn.SendPacket(packet);
				} break;

				case Query::Type::PlayerNotifyRoomFound: {
					In::HN_PlayerRoomFound packet;
					packet.accountUID = q->PlayerNotifyRoomFound.playerAccountUID;
					packet.sortieUID = q->PlayerNotifyRoomFound.sortieUID;
					conn.SendPacket(packet);
				} break;

				case Query::Type::PlayerRoomConfirm: {
					In::HN_PlayerRoomConfirm packet;
					packet.accountUID = q->PlayerRoomConfirm.playerAccountUID;
					packet.confirm = q->PlayerRoomConfirm.confirm;
					packet.sortieUID = q->PlayerRoomConfirm.sortieUID;
					conn.SendPacket(packet);
				} break;

				default: {
					ASSERT_MSG(0, "case not handled");
				}
			}
		}

		queries.clear();
	}
}

// Thread: Any Lane
void MatchmakerConnector::QueryPartyCreate(AccountUID leader)
{
	MMQueryUID queryUID = nextQueryUID;
	nextQueryUID = MMQueryUID((u32)nextQueryUID + 1);

	Query query(queryUID, Query::Type::PartyCreate);
	query.PartyCreate.leader = leader;

	LOCK_MUTEX(mutexQueries);
	queries.push_back(query);
}

// Thread: Any Lane
void MatchmakerConnector::QueryPartyEnqueue(PartyUID partyUID)
{
	DBG_ASSERT(partyUID != PartyUID::INVALID);

	MMQueryUID queryUID = nextQueryUID;
	nextQueryUID = MMQueryUID((u32)nextQueryUID + 1);

	Query query(queryUID, Query::Type::PartyEnqueue);
	query.PartyEnqueue.partyUID = partyUID;

	LOCK_MUTEX(mutexQueries);
	queries.push_back(query);
}

// Thread: Any Lane
void MatchmakerConnector::QueryPlayerNotifyRoomFound(AccountUID playerAccountUID, SortieUID sortieUID)
{
	DBG_ASSERT(sortieUID != SortieUID::INVALID);

	MMQueryUID queryUID = nextQueryUID;
	nextQueryUID = MMQueryUID((u32)nextQueryUID + 1);

	Query query(queryUID, Query::Type::PlayerNotifyRoomFound);
	query.PlayerNotifyRoomFound.playerAccountUID = playerAccountUID;
	query.PlayerNotifyRoomFound.sortieUID = sortieUID;

	LOCK_MUTEX(mutexQueries);
	queries.push_back(query);
}

// Thread: Any Lane
void MatchmakerConnector::QueryPlayerRoomConfirm(AccountUID playerAccountUID, SortieUID sortieUID, u8 confirm)
{
	DBG_ASSERT(sortieUID != SortieUID::INVALID);

	MMQueryUID queryUID = nextQueryUID;
	nextQueryUID = MMQueryUID((u32)nextQueryUID + 1);

	Query query(queryUID, Query::Type::PlayerRoomConfirm);
	query.PlayerRoomConfirm.playerAccountUID = playerAccountUID;
	query.PlayerRoomConfirm.confirm = confirm;
	query.PlayerRoomConfirm.sortieUID = sortieUID;

	LOCK_MUTEX(mutexQueries);
	queries.push_back(query);
}

MatchmakerConnector& Matchmaker()
{
	return *g_connector;
}
