#pragma once
#include <common/network.h>
#include <common/vector_math.h>
#include <common/utils.h>
#include <EASTL/array.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/fixed_set.h>
#include <EASTL/fixed_map.h>
#include <EASTL/fixed_list.h>

struct Replication
{
	template<class T>
	using List = eastl::fixed_list<T,2048>;

	struct ActorNameplate
	{
		WideString name;
		WideString guildTag;
	};

	struct ActorStats
	{

	};

	struct ActorPlayerInfo
	{
		i32 weaponID;
		i32 additionnalOverHeatGauge;
		i32 additionnalOverHeatGaugeRatio;
		u8 playerStateInTown;
		eastl::fixed_vector<u8,16> matchingGameModes;
	};

	struct Actor
	{
		u32 UID;
		i32 type;
		i32 modelID;
		Vec3 pos;
		Vec3 dir;
		i32 spawnType;
		i32 actionState;
		i32 ownerID;
		i32 faction;
		i32 classType;
		i32 skinIndex;

	private:

		List<ActorNameplate>::iterator nameplate = nullptr;
		List<ActorStats>::iterator stats = nullptr;
		List<ActorPlayerInfo>::iterator playerInfo = nullptr;

		friend struct Replication;
	};

	struct Frame
	{
		eastl::array<bool,Server::MAX_CLIENTS> playerDoScanEnd;
		List<Actor> actorList;
		List<ActorNameplate> actorNameplateList;
		List<ActorStats> actorStatsList;
		List<ActorPlayerInfo> actorPlayerInfoList;
		eastl::fixed_map<u32,List<Actor>::iterator,2048> actorUidMap;
		eastl::fixed_set<u32,2048> actorUidSet;

		void Clear();
	};

	enum class PlayerState: u8 {
		DISCONNECTED=0,
		CONNECTED=1,
		IN_GAME=2,
	};

	Server* server;
	Frame frameCur;
	Frame framePrev;

	// TODO: we propably do not need to store every possible client data here
	// Use a fixed_vector?

	eastl::array<PlayerState,Server::MAX_CLIENTS> playerState;
	eastl::array<eastl::fixed_set<u32,2048>,Server::MAX_CLIENTS> playerLocalActorUidSet;

	void Init(Server* server_);

	void FrameEnd();
	void FramePushActor(const Actor& actor, const ActorNameplate* nameplate, const ActorStats* stats, const ActorPlayerInfo* playerInfo);

	void EventPlayerConnect(i32 clientID, u32 playerAssignedActorUID);
	void EventPlayerGameEnter(i32 clientID);
	void EventPlayerRequestCharacterInfo(i32 clientID, u32 actorUID, i32 modelID, i32 classType, i32 health, i32 healthMax);

private:
	void SendActorSpawn(i32 clientID, const Actor& actor);

	template<typename Packet>
	inline void SendPacket(i32 clientID, const Packet& packet)
	{
		SendPacketData(clientID, Packet::NET_ID, sizeof(packet), &packet);
	}
	inline void SendPacketData(i32 clientID, u16 netID, u16 packetSize, const void* packetData)
	{
		server->SendPacketData(clientID, netID, packetSize, packetData);
	}
};
