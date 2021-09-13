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
		ConstBuffer reader(recvBuff, recvLen);

		while(reader.CanRead(sizeof(NetHeader))) {
			const NetHeader& header = reader.Read<NetHeader>();
			const i32 packetDataSize = header.size - sizeof(NetHeader);
			ASSERT(reader.CanRead(packetDataSize));
			const u8* packetData = reader.ReadRaw(packetDataSize);

			HandlePacket(header, packetData);
		}
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

void MatchmakerConnector::HandlePacket(const NetHeader& header, const u8* packetData)
{
	const i32 packetSize = header.size - sizeof(NetHeader);

	switch(header.netID) {
		case In::MR_Handshake::NET_ID: {
			const In::MR_Handshake resp = SafeCast<In::MR_Handshake>(packetData, packetSize);
			if(resp.result == 1) {
				LOG("[MM] Connected to Matchmaker server");
			}
			else {
				WARN("[MM] handshake failed (%d)", resp.result);
				ASSERT_MSG(0, "mm handshake failed"); // should not happen
			}
		} break;

		case In::MR_PartyCreated::NET_ID: {
			const In::MR_PartyCreated resp = SafeCast<In::MR_PartyCreated>(packetData, packetSize);

			UpdatePartyCreated created;
			created.UID = resp.partyUID;
			created.leader = resp.leader;
			LOCK_MUTEX(mutexUpdates);

			updatePartiesCreated.push_back(created);
		} break;

		case In::MR_PartyEnqueued::NET_ID: {
			const In::MR_PartyEnqueued resp = SafeCast<In::MR_PartyEnqueued>(packetData, packetSize);
			LOCK_MUTEX(mutexUpdates);
			updatePartiesEnqueued.push_back(UpdatePartyEnqueued{ resp.partyUID });
		} break;

		case In::MR_MatchFound::NET_ID: {
			const In::MR_MatchFound resp = SafeCast<In::MR_MatchFound>(packetData, packetSize);
			LOCK_MUTEX(mutexUpdates);
			updateMatchFound.push_back(UpdateMatchFound{ resp.partyUID });
		} break;

		default: {
			ASSERT_MSG(0, "case not handled");
		}
	}
}

MatchmakerConnector& Matchmaker()
{
	return *g_connector;
}
