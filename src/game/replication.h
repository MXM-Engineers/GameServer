#pragma once
#include <common/network.h>
#include <common/vector_math.h>
#include <common/utils.h>
#include <common/protocol.h>
#include <EASTL/array.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/fixed_set.h>
#include <EASTL/fixed_map.h>
#include <EASTL/fixed_list.h>
#include "core.h"

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
		ActorUID UID;
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

	struct Transform
	{
		Vec3 pos;
		Vec3 dir;
		Vec3 eye;
		f32 rotate;
		f32 speed;
		i32 state;
		i32 actionID;

		bool IsEqual(const Transform& other) const;
	};

	struct Frame
	{
		eastl::array<bool,Server::MAX_CLIENTS> playerDoScanEnd;
		List<Actor> actorList;
		List<ActorNameplate> actorNameplateList;
		List<ActorStats> actorStatsList;
		List<ActorPlayerInfo> actorPlayerInfoList;
		eastl::fixed_map<ActorUID,List<Actor>::iterator,2048> actorUIDMap;
		eastl::fixed_set<ActorUID,2048> actorUIDSet;

		eastl::fixed_map<ActorUID,Transform,2048> transformMap;

		void Clear();
	};

	enum class PlayerState: u8 {
		DISCONNECTED=0,
		CONNECTED=1,
		IN_GAME=2,
	};

	struct PlayerLocalInfo
	{
		eastl::fixed_map<ActorUID,LocalActorID,2048> localActorIDMap;
		eastl::fixed_set<ActorUID,2048> actorUIDSet;
		LocalActorID nextPlayerLocalActorID;
		LocalActorID nextNpcLocalActorID;
		LocalActorID nextMonsterLocalActorID;

		void Reset();
	};

	Server* server;
	Frame frames[2];
	Frame* frameCur;
	Frame* framePrev;

	// TODO: we propably do not need to store every possible client data here
	// Use a fixed_vector?

	eastl::array<PlayerState,Server::MAX_CLIENTS> playerState;
	eastl::array<PlayerLocalInfo,Server::MAX_CLIENTS> playerLocalInfo;

	void Init(Server* server_);

	void FrameEnd();
	void FramePushActor(const Actor& actor, const Transform& tf, const ActorNameplate* nameplate, const ActorStats* stats, const ActorPlayerInfo* playerInfo);

	void EventPlayerConnect(i32 clientID);
	void EventPlayerLoad(i32 clientID);
	void EventPlayerGameEnter(i32 clientID);
	void EventPlayerRequestCharacterInfo(i32 clientID, u32 actorUID, i32 modelID, i32 classType, i32 health, i32 healthMax);
	void EventPlayerSetLeaderMaster(i32 clientID, ActorUID masterActorUID, i32 leaderMasterID);
	void EventChatMessage(const wchar* senderName, i32 chatType, const wchar* msg, i32 msgLen);
	void EventChatMessageToClient(i32 toClientID, const wchar* senderName, i32 chatType, const wchar* msg, i32 msgLen = -1);
	void EventClientDisconnect(i32 clientID);

	void PlayerForceLocalActorID(i32 clientID, ActorUID actorUID, LocalActorID localActorID);

	LocalActorID GetLocalActorID(i32 clientID, ActorUID actorUID); // Can return INVALID
	ActorUID GetActorUID(i32 clientID, LocalActorID localActorID); // Can return INVALID

private:
	void UpdatePlayersLocalState();
	void DoFrameDifference();

	void SendActorSpawn(i32 clientID, const Actor& actor);
	void SendActorDestroy(i32 clientID, ActorUID actorUID);

	void CreateLocalActorID(i32 clientID, ActorUID actorUID);
	void DeleteLocalActorID(i32 clientID, ActorUID actorUID);

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
