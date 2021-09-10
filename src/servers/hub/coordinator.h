#pragma once
#include <eathread/eathread_thread.h>
#include <common/network.h>
#include <common/utils.h>
#include <common/protocol.h>
#include <EASTL/queue.h>
#include <EASTL/fixed_set.h>
#include <EASTL/fixed_map.h>

#include "matchmaker_connector.h"

// TODO: move this
enum class AccountID: u32 {
	INVALID = 0
};

struct AccountData
{
	WideString nickname;
	WideString guildTag;
	i32 leaderMasterID;

	// TODO: add to this
};

struct AccountData;

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

struct IInstance
{
	virtual bool Init(Server* server_) = 0;
	virtual void Update(Time localTime_) = 0;
	virtual void OnNewClientsConnected(const eastl::pair<ClientHandle, const AccountData*>* clientList, const i32 count) = 0;
	virtual void OnNewClientsDisconnected(const ClientHandle* clientList, const i32 count) = 0;
	virtual void OnNewPacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData) = 0;
};

typedef IInstance* (*fn_InstanceAllocate)();

enum class LaneID: i32
{
	NONE = -1,
	FIRST = 0,
	_COUNT // TODO: fetch core count instead and fill them that way
};

// Each lane is a separate thread hosting instances
struct Lane
{
	struct EventOnClientConnect
	{
		ClientHandle clientHd;
		const AccountData* accountData;
	};

	u32 laneIndex;
	Time localTime;

	EA::Thread::Thread thread;
	Server* server;

	GrowableBuffer recvDataBuff;

	ProfileMutex(Mutex, mutexNewPlayerQueue);
	GrowableBuffer packetDataQueue;
	GrowableBuffer processPacketQueue;

	ProfileMutex(Mutex, mutexClientDisconnectedList);
	eastl::fixed_vector<ClientHandle,128> clientDisconnectedList;

	ProfileMutex(Mutex, mutexPacketDataQueue);
	eastl::fixed_vector<EventOnClientConnect,128> newPlayerQueue;

	eastl::fixed_set<ClientHandle,MAX_CLIENTS,false> clientSet;

	fn_InstanceAllocate allocInstance;
	IInstance* instance;

	void Init(Server* server_);
	void Update();
	void Cleanup();

	void CoordinatorRegisterNewPlayer(ClientHandle clientHd, const AccountData* accountData);
	void CoordinatorClientHandlePacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData);
	void CoordinatorHandleDisconnectedClients(ClientHandle* clientIDList, const i32 count);

	void ClientHandlePacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData);
};

// Responsible for managing Account data and dispatching client to game channels/instances
struct Coordinator
{
	Server* server;
	eastl::array<Lane, (i32)LaneID::_COUNT> lanes;
	eastl::array<LaneID, MAX_CLIENTS> associatedLane;

	ClientLocalMapping plidMap;
	eastl::array<ClientHandle, MAX_CLIENTS> clientHandle;
	eastl::array<AccountData, MAX_CLIENTS> accountData;

	GrowableBuffer recvDataBuff;

	EA::Thread::Thread thread;
	Time localTime;

	MatchmakerConnector matchmaker;

	bool Init(Server* server_);
	void Cleanup();

	void Update(f64 delta);

	void ClientHandlePacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData);
private:

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
