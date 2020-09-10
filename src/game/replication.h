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
		CreatureIndex docID;
		Vec3 pos;
		Vec3 dir;
		Vec3 eye;
		f32 rotate;
		f32 upperRotate;
		f32 speed;
		i32 spawnType;
		i32 ownerID;
		i32 faction;
		ClassType classType;
		SkinIndex skinIndex;
		i32 localID;

		ActionStateID actionState;
		i32 actionParam1;
		i32 actionParam2;

	private:

		List<ActorNameplate>::iterator nameplate = nullptr;
		List<ActorStats>::iterator stats = nullptr;
		List<ActorPlayerInfo>::iterator playerInfo = nullptr;

		friend struct Replication;
	};

	struct Frame
	{
		struct Transform
		{
			Vec3 pos;
			Vec3 dir;
			Vec3 eye;
			f32 rotate;
			f32 speed;

			bool HasNotChanged(const Transform& other) const;
		};

		struct ActionState
		{
			ActionStateID actionState;
			i32 actionParam1;
			i32 actionParam2;
			f32 rotate;
			f32 upperRotate;

			bool HasNotChanged(const ActionState& other) const;
		};

		List<Actor> actorList;
		List<ActorNameplate> actorNameplateList;
		List<ActorStats> actorStatsList;
		List<ActorPlayerInfo> actorPlayerInfoList;
		eastl::fixed_map<ActorUID,List<Actor>::iterator,2048> actorUIDMap;
		eastl::fixed_set<ActorUID,2048> actorUIDSet;

		eastl::fixed_map<ActorUID,Transform,2048> transformMap;
		eastl::fixed_map<ActorUID,ActionState,2048> actionStateMap;

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
		bool isFirstLoad;

		void Reset();
	};

	struct JukeboxTrack
	{
		SongID songID;
		WideString requesterNick;
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
	void FramePushActor(const Actor& actor, const ActorNameplate* nameplate, const ActorStats* stats, const ActorPlayerInfo* playerInfo);

	void EventPlayerConnect(i32 clientID);
	void EventPlayerLoad(i32 clientID);
	void SetPlayerAsInGame(i32 clientID);
	void EventPlayerRequestCharacterInfo(i32 clientID, ActorUID actorUID, CreatureIndex docID, ClassType classType, i32 health, i32 healthMax);
	void SendPlayerSetLeaderMaster(i32 clientID, ActorUID masterActorUID, i32 leaderMasterID, SkinIndex skinIndex);

	void SendChatMessageToAll(const wchar* senderName, i32 chatType, const wchar* msg, i32 msgLen);
	void SendChatMessageToClient(i32 toClientID, const wchar* senderName, i32 chatType, const wchar* msg, i32 msgLen = -1);
	void SendChatWhisperConfirmToClient(i32 senderClientID, const wchar* destNick, const wchar* msg);
	void SendChatWhisperToClient(i32 destClientID, const wchar* destNick, const wchar* msg);

	void EventClientDisconnect(i32 clientID);

	LocalActorID GetLocalActorID(i32 clientID, ActorUID actorUID); // Can return INVALID
	ActorUID GetActorUID(i32 clientID, LocalActorID localActorID); // Can return INVALID

	void JukeboxPlaySong(i32 result, i32 trackID, wchar* nickname, u16 playPositionSec);
	void SendJukeboxPlay(i32 clientID, SongID songID, const wchar* requesterNick, i32 playPosInSec);
	void SendJukeboxQueue(i32 clientID, const JukeboxTrack* tracks, const i32 trackCount);

	bool IsActorReplicatedForClient(i32 clientID, ActorUID actorUID) const;

private:
	void PlayerForceLocalActorID(i32 clientID, ActorUID actorUID, LocalActorID localActorID);

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
