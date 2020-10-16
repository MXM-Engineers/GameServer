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

struct AccountData;

struct Replication
{
	enum class ActorType: i32
	{
		INVALID = 0,
		PLAYER = 1,
		NPC = 2,
		JUKEBOX = 3,
	};

	template<ActorType TYPE_>
	struct Actor
	{
		ActorUID actorUID;
		CreatureIndex docID;
		Vec3 pos;
		Vec3 dir;

		inline ActorType Type() const { return TYPE_; }
	};

	struct ActorPlayer: Actor<ActorType::PLAYER>
	{
		Vec3 eye;
		f32 rotate;
		f32 upperRotate;
		f32 speed;
		ClassType classType;
		SkinIndex skinIndex;

		WideString name;
		WideString guildTag;
		i32 weaponID;
		i32 additionnalOverHeatGauge;
		i32 additionnalOverHeatGaugeRatio;
		u8 playerStateInTown;
		eastl::fixed_vector<u8,16> matchingGameModes;

		ActionStateID actionState;
		i32 actionParam1;
		i32 actionParam2;
	};

	struct ActorNpc: Actor<ActorType::NPC>
	{
		i32 type;
		i32 localID;
		i32 faction;
	};

	struct ActorJukebox: Actor<ActorType::JUKEBOX>
	{
		struct Track
		{
			SongID songID;
			WideString requesterNick;
		};

		i32 localID;

		i32 playPosition;
		Time playStartTime; // identifier
		Track currentSong;
		eastl::fixed_vector<Track,JUKEBOX_MAX_TRACKS,false> tracks;

		ActorJukebox() {
			actorUID = ActorUID::INVALID;
		}
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

		// TODO: replace fixed_map with fixed_hash_map
		eastl::fixed_list<ActorPlayer,2048,true> playerList;
		eastl::fixed_list<ActorNpc,2048,true> npcList;
		eastl::fixed_map<ActorUID,decltype(playerList)::iterator,2048,true> playerMap;
		eastl::fixed_map<ActorUID,decltype(npcList)::iterator,2048,true> npcMap;

		eastl::fixed_set<ActorUID,2048> actorUIDSet;
		eastl::fixed_map<ActorUID,ActorType,2048,true> actorType;

		eastl::fixed_map<ActorUID,Transform,2048> transformMap;
		eastl::fixed_map<ActorUID,ActionState,2048> actionStateMap;

		ActorJukebox jukebox;

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

	Server* server;
	Frame frames[2];
	Frame* frameCur;
	Frame* framePrev;

	// TODO: we propably do not need to store every possible client data here
	// Use a fixed_vector?

	eastl::array<PlayerState,Server::MAX_CLIENTS> playerState;
	eastl::array<PlayerLocalInfo,Server::MAX_CLIENTS> playerLocalInfo;

	StageType stageType;

	void Init(Server* server_);

	void FrameEnd();
	void FramePushPlayerActor(const ActorPlayer& actor);
	void FramePushNpcActor(const ActorNpc& actor);
	void FramePushJukebox(const ActorJukebox& actor);

	void EventPlayerConnect(i32 clientID);
	void SendLoadLobby(i32 clientID, StageIndex stageIndex);
	void SendLoadPvpMap(i32 clientID, StageIndex stageIndex);
	void SetPlayerAsInGame(i32 clientID);
	void SendCharacterInfo(i32 clientID, ActorUID actorUID, CreatureIndex docID, ClassType classType, i32 health, i32 healthMax);
	void SendPlayerSetLeaderMaster(i32 clientID, ActorUID masterActorUID, ClassType classType, SkinIndex skinIndex);

	void SendChatMessageToAll(const wchar* senderName, i32 chatType, const wchar* msg, i32 msgLen);
	void SendChatMessageToClient(i32 toClientID, const wchar* senderName, i32 chatType, const wchar* msg, i32 msgLen = -1);
	void SendChatWhisperConfirmToClient(i32 senderClientID, const wchar* destNick, const wchar* msg);
	void SendChatWhisperToClient(i32 destClientID, const wchar* destNick, const wchar* msg);

	void SendAccountDataCity(i32 clientID, const AccountData& account);
	void SendAccountDataPvp(i32 clientID, const AccountData& account);

	void EventClientDisconnect(i32 clientID);

	void PlayerSetSubActorUID(i32 clientiD, ActorUID masterActorUID, ClassType classType); // TODO: temp, find a better solution

	LocalActorID GetLocalActorID(i32 clientID, ActorUID actorUID); // Can return INVALID
	ActorUID GetWorldActorUID(i32 clientID, LocalActorID localActorID); // Can return INVALID

private:
	void PlayerForceLocalActorID(i32 clientID, ActorUID actorUID, LocalActorID localActorID);

	void UpdatePlayersLocalState();
	void FrameDifference();

	void SendActorPlayerSpawn(i32 clientID, const ActorPlayer& actor);
	void SendActorNpcSpawn(i32 clientID, const ActorNpc& actor);
	void SendJukeboxSpawn(i32 clientID, const ActorJukebox& actor);
	void SendActorDestroy(i32 clientID, ActorUID actorUID);
	void SendJukeboxPlay(i32 clientID, SongID songID, const wchar* requesterNick, i32 playPosInSec);
	void SendJukeboxQueue(i32 clientID, const ActorJukebox::Track* tracks, const i32 trackCount);

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
