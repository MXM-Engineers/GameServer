#pragma once
#include <eathread/eathread_thread.h>
#include <common/network.h>
#include <common/utils.h>
#include <common/protocol.h>
#include <EASTL/queue.h>
#include <EASTL/fixed_set.h>
#include <EASTL/fixed_map.h>

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

struct InnerConnection
{
	AsyncConnection async;

	struct SendQueue
	{
		enum {
			QUEUE_CAPACITY = 1024 * 1024 // 1MB
		};

		u8 data[QUEUE_CAPACITY];
		i32 size = 0;
	};

	i32 ID;
	SendQueue sendQ;

	template<typename Packet>
	inline void SendPacket(const Packet& packet)
	{
		SendPacketData(Packet::NET_ID, sizeof(packet), &packet);
	}

	void SendPacketData(u16 netID, u16 packetSize, const void* packetData);

	void SendPendingData();
	void RecvPendingData(u8* buff, const i32 buffCapacity, i32* size);
};

struct MatchFindFilter
{
	// pvp: 3v3, 5v5
	// pve: map, level?
	// mini game
	// ?

	u8 region;
};

struct Matchmaker
{
	struct Match3v3
	{
		eastl::fixed_vector<AccountID,6,false> players;
	};

	eastl::fixed_vector<InnerConnection, 16, false> connGameSrv;
	eastl::fixed_vector<AccountID, 2048> waitingQueue;
	eastl::fixed_vector<Match3v3, 2048> pendingMatch3v3;

	bool Init();
	bool Update();

	void PushPlayerToQueue(AccountID accountID, const MatchFindFilter& filter);

private:
	void HandlePacket(InnerConnection& conn, u16 netID, const u8* packetData, const i32 packetSize);
};

struct AccountData;

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

	Matchmaker matchmaker;

	bool Init(Server* server_);
	void Cleanup();

	void Update(f64 delta);

private:
	void ClientHandlePacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData);
	void ClientHandleReceivedChunk(ClientHandle clientHd, const u8* data, const i32 dataSize);

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
