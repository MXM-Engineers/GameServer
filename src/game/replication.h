#pragma once
#include <common/network.h>
#include <common/vector_math.h>
#include <EASTL/array.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/fixed_set.h>

struct Replication
{
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
	};

	struct Frame
	{
		eastl::array<bool,Server::MAX_CLIENTS> playerDoScanEnd;
		eastl::fixed_vector<Actor,2048> actorList;

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
	eastl::array<PlayerState,Server::MAX_CLIENTS> playerState;
	eastl::array<eastl::fixed_set<u32,2048>,Server::MAX_CLIENTS> playerLocalActorUidSet;

	void Init(Server* server_);

	void FrameEnd();
	void FramePushActor(const Actor& actor);

	void EventPlayerConnect(i32 clientID, u32 playerAssignedActorUID);
	void EventPlayerGameEnter(i32 clientID);
	void EventPlayerRequestCharacterInfo(i32 clientID, u32 actorUID, i32 modelID, i32 classType, i32 health, i32 healthMax);

private:
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
