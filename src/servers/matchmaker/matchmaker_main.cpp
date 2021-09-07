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

// TODO: move this and merge with the other AccountID
enum class AccountID: u32 {
	INVALID = 0
};

// fixed hash map
template<typename T1, typename T2, int CAPACITY, bool Growing = false>
using hash_map = eastl::fixed_hash_map<T1 ,T2, CAPACITY, CAPACITY+1, Growing>;

struct Matchmaker
{
	Server& server;
	Time localTime;
	EA::Thread::Thread thread;

	struct Match3v3
	{
		eastl::fixed_vector<AccountID,6,false> players;
	};

	struct Connection
	{
		In::ConnType type; // TODO: timeout when undecided for a while
		ClientHandle clientHd;
	};

	eastl::fixed_list<Connection, 32> connList;
	hash_map<ClientHandle, decltype(connList)::iterator, 32> connMap;
	eastl::fixed_vector<AccountID, 2048> waitingQueue;
	eastl::fixed_vector<Match3v3, 2048> pendingMatch3v3;

	GrowableBuffer recvDataBuff;

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
		eastl::fixed_vector<ClientHandle,128> clientConnectedList;
		server.TransferConnectedClientList(&clientConnectedList);

		foreach_const(cl, clientConnectedList) {
			Connection conn;
			conn.type = In::ConnType::Undecided;
			conn.clientHd = *cl;
			connList.push_back(conn);
			connMap.emplace(*cl, --connList.end());
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
	}

	void ClientHandlePacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData)
	{
		const i32 packetSize = header.size - sizeof(NetHeader);

		Connection& conn = *connMap.at(clientHd);

		switch(conn.type) {
			case In::ConnType::Undecided: {
				switch(header.netID) {
					case In::Q_Handshake::NET_ID: {
						// TODO: check white list
						// TODO: validate args
						const In::Q_Handshake& packet = SafeCast<In::Q_Handshake>(packetData, packetSize);
						conn.type = packet.type;

						In::R_Handshake resp;
						resp.result = 1;
						SendPacket(clientHd, resp);

						const eastl::array<const char*, 3> ConnTypeStr = {
							"Undecided",
							"HubServer",
							"PlayServer"
						};

						LOG("[client%x] New connection (type='%s')", clientHd, ConnTypeStr[(i32)conn.type]);
						return; // accept packet
					} break;
				}
			} break;
		}


		WARN("[client%x] Unhandled packet (netID=%u size=%u)", clientHd, header.netID, header.size);
		server.DisconnectClient(clientHd);
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
