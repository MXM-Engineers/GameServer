#pragma once
#include <eathread/eathread_thread.h>
#include <common/network.h>
#include <common/utils.h>
#include <common/protocol.h>
#include <EASTL/fixed_set.h>

#include "matchmaker_connector.h"
#include "instance.h"
#include "account.h"

template<typename PacketHandler>
void NetworkParseReceiveBuffer(PacketHandler* ph, Server* server, const u8* data, const u32 size, const char* handlerName)
{
	ConstBuffer buff(data, size);
	while(buff.CanRead(sizeof(Server::RecvChunkHeader))) {
		const Server::RecvChunkHeader& chunkInfo = buff.Read<Server::RecvChunkHeader>();
		const u8* data = buff.ReadRaw(chunkInfo.len);

		// handle each packet in chunk
		ConstBuffer reader(data, chunkInfo.len);
		if(!reader.CanRead(sizeof(NetHeader))) {
			WARN("Packet too small (clientHd=%u size=%d)", chunkInfo.clientHd, chunkInfo.len);
			server->DisconnectClient(chunkInfo.clientHd);
			continue;
		}

		while(reader.CanRead(sizeof(NetHeader))) {
			const NetHeader& header = reader.Read<NetHeader>();
			const i32 packetDataSize = header.size - sizeof(NetHeader);

			if(!reader.CanRead(packetDataSize)) {
				WARN("Packet header size differs from actual data size (clientHd=%u size=%d)", chunkInfo.clientHd, header.size);
				server->DisconnectClient(chunkInfo.clientHd);
				break;
			}

			if(Config().TraceNetwork) {
				fileSaveBuff(FormatPath(FMT("trace/%s_%d_cl_%d.raw", handlerName, server->packetCounter, header.netID)), &header, header.size);
				server->packetCounter++;
			}

			const u8* packetData = reader.ReadRaw(packetDataSize);
			ph->ClientHandlePacket(chunkInfo.clientHd, header, packetData);
		}
	}
}

enum class InstanceType: u8
{
	NONE = 0,
	HUB,
	ROOM
};

const int CPU_COUNT = 1;

struct InstancePool
{
	struct RoomUser
	{
		ClientHandle clientHd;
		AccountUID accountUID;
		u8 team;
		u8 userID;
	};

	struct Lane
	{
		struct Client
		{
			ClientHandle clientHd;
			AccountUID accountUID;
			InstanceType instanceType;
			InstanceUID instanceUID;
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

		ProfileMutex(Mutex, mutexClientTransferOutQueue);
		eastl::fixed_vector<ClientHandle,128> clientTransferOutQueue;

		ProfileMutex(Mutex, mutexCreateRoomQueue);
		struct CreateRoomEntry {
			SortieUID sortieUID;
			eastl::fixed_vector<RoomUser,16> users;
		};
		eastl::fixed_vector<CreateRoomEntry,128> createRoomQueue;

		ProfileMutex(Mutex, mutexHubPushPlayerQueue);
		struct HubPlayerEntry {
			ClientHandle clientHd;
			AccountUID accountUID;
		};
		eastl::fixed_vector<HubPlayerEntry,128> hubPushPlayerQueue;

		eastl::list<HubInstance> instanceHubList;
		eastl::list<RoomInstance> instanceRoomList;

		hash_map<InstanceUID,decltype(instanceHubList)::iterator,128> instanceHubMap;
		hash_map<InstanceUID,decltype(instanceRoomList)::iterator,128> instanceRoomMap;

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

	ClientLocalMapping plidMap;
	eastl::array<ClientHandle, MAX_CLIENTS> clientHandle;
	eastl::array<ClientLocation, MAX_CLIENTS> clientLocation;

	// Thread: Coordinator
	bool Init(Server* server_);
	void Cleanup();

	// Thread: Coordinator
	void QueuePushPlayerToHub(ClientHandle clientHd, AccountUID accountUID);
	void QueueCreateRoom(SortieUID sortieUID, const RoomUser* userList, const i32 userCount);

	void QueuePopPlayers(const ClientHandle* clientList, const i32 count); // on disconnect

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
	InstancePool instancePool;

	ClientLocalMapping plidMap;
	eastl::array<ClientHandle, MAX_CLIENTS> clientHandle;
	eastl::array<AccountUID, MAX_CLIENTS> clientAccountUID;

	hash_map<AccountUID,ClientHandle,MAX_CLIENTS> accChdMap;

	GrowableBuffer recvDataBuff;

	EA::Thread::Thread thread;
	Time localTime;

	MatchmakerConnector matchmaker;

	bool Init(Server* server_);
	void Cleanup();

	void Update(f64 delta);

	void ClientHandlePacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData);
private:

	void PushClientToHubInstance(ClientHandle clientHd);

	void HandlePacket_CQ_FirstHello(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);
	void HandlePacket_CQ_Authenticate(ClientHandle clientHd, const NetHeader& header, const u8* packetData, const i32 packetSize);

	void ClientSendAccountData(ClientHandle clientHd);

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
		NT_LOG("[client%x] Coordinator :: %s", clientHd, PacketSerialize<Packet>(packetData, packetSize));
		server->SendPacketData(clientHd, Packet::NET_ID, packetSize, packetData);
	}
};
