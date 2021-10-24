#pragma once
#include <eathread/eathread_thread.h>
#include <common/network.h>
#include <common/utils.h>
#include <common/protocol.h>
#include <common/inner_protocol.h>

#include "instance.h"
#include "matchmaker_connector.h"

enum class InstanceType: u8
{
	NONE = 0,
	PVP_3V3,
};

const int CPU_COUNT = 1;

struct InstancePool
{
	struct Lane
	{
		struct Client
		{
			ClientHandle clientHd;
			InstanceType instanceType;
			SortieUID sortieUID;
		};

		Server* server;
		EA::Thread::Thread thread;
		i32 laneIndex;
		Time localTime = Time::ZERO;

		GrowableBuffer recvDataBuff;

		eastl::fixed_list<Client, MAX_CLIENTS, false> clientList;
		hash_map<ClientHandle, decltype(clientList)::iterator, MAX_CLIENTS> clientMap;
		eastl::fixed_set<ClientHandle,MAX_CLIENTS> clientHandleSet;

		ProfileMutex(Mutex, mutexMatchmakerPacketsQueue);
		GrowableBuffer mmPacketQueues[2];
		u8 mmPacketQueueFront = 0; // being pushed to by coordinator

		ProfileMutex(Mutex, mutexRoguePacketsQueue);
		GrowableBuffer roguePacketsQueue;

		ProfileMutex(Mutex, mutexClientDisconnectQueue);
		eastl::fixed_vector<ClientHandle,128> clientDisconnectQueue;

		ProfileMutex(Mutex, mutexClientConnectQueue);
		struct ClientConnectEntry {
			ClientHandle clientHd;
			AccountUID accountUID;
			SortieUID sortieUID;
		};
		eastl::fixed_vector<ClientConnectEntry,128> clientConnectQueue;

		ProfileMutex(Mutex, mutexCreateGameQueue);
		eastl::fixed_vector<In::MQ_CreateGame,128> createGameQueue;

		eastl::list<PvpInstance> instancePvpList;
		hash_map<SortieUID,decltype(instancePvpList)::iterator,128> instancePvpMap;

		// Thread: Lane
		void Update();
		void Cleanup();

		void ClientHandlePacket();
	};

	union ClientLocation
	{
		u8 whole;

		struct {
			u8 lane;
		};

		static ClientLocation Null() {
			return {0x0};
		}

		inline bool IsNull() const { return whole == 0x0; }
	};

	Server* server;
	eastl::array<Lane,CPU_COUNT> lanes;

	ClientLocalMapping plidMap; // only modified on Coordinator Thread
	eastl::array<ClientHandle, MAX_CLIENTS> clientHandle;
	eastl::array<ClientLocation, MAX_CLIENTS> clientLocation;

	// FIXME: pop when game is destroyed
	hash_map<SortieUID,u8,4096> sortieLocation;

	// Thread: Coordinator
	bool Init(Server* server_);
	void Cleanup();

	// Thread: Coordinator
	void QueuePushPlayerToGame(ClientHandle clientHd, AccountUID accountUID, SortieUID sortieUID);
	void QueuePopPlayers(const ClientHandle* clientList, const i32 count); // on disconnect
	void QueueCreateGame(const In::MQ_CreateGame& gameInfo);

	// packets piped to coordinator before player is assigned to an instance
	void QueueRogueCoordinatorPacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData);
	void QueueMatchmakerPackets(const u8* buffer, u32 bufferSize);

	inline bool IsClientInsideAnInstance(ClientHandle clientHd) const {
		return plidMap.TryGet(clientHd) != -1;
	}
};

// Responsible for managing Account data and dispatching client to game channels/instances
struct Coordinator
{
	Server* server;
	MatchmakerConnector matchmaker;
	InstancePool instancePool;

	ClientLocalMapping plidMap;
	eastl::array<ClientHandle, MAX_CLIENTS> clientHandle;

	GrowableBuffer recvDataBuff;

	EA::Thread::Thread thread;
	Time localTime;

	bool Init(Server* server_);
	void Cleanup();

	void Update();

	void ClientHandlePacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData);
private:

	struct PendingClientEntry
	{
		AccountUID accountUID;
		u32 instantKey;
		SortieUID sortieUID;
		Time time;
	};

	eastl::fixed_vector<PendingClientEntry,2048> pendingClientQueue; // TODO: timeout entries eventually if client does not connect

	void ProcessMatchmakerPackets();
	void HandleMatchmakerPacket(const NetHeader& header, const u8* packetData, const i32 packetSize);

	void HandlePacket_CQ_FirstHello(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_AuthenticateGameServer(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);

	void CreateDevGame();

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
		NT_LOG("[client%x] Play :: %s", clientHd, PacketSerialize<Packet>(packetData, packetSize));
		server->SendPacketData(clientHd, Packet::NET_ID, packetSize, packetData);
	}
};
