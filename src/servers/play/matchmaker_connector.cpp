#include <common/inner_protocol.h>
#include "matchmaker_connector.h"
#include "config.h"

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

	In::PQ_Handshake handshake;
	handshake.magic = In::MagicHandshake;
	handshake.listenPort = Config().ListenPort;
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
				case Query::Type::GameCreated: {
					In::PR_GameCreated packet;
					packet.sortieUID = q->GameCreated.sortieUID;
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

void MatchmakerConnector::QueryGameCreated(SortieUID sortieUID)
{
	Query query(Query::Type::GameCreated);
	query.GameCreated.sortieUID = sortieUID;

	LOCK_MUTEX(mutexQueries);
	queries.push_back(query);
}

MatchmakerConnector& Matchmaker()
{
	return *g_connector;
}
