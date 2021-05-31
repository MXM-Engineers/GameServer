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
#include <game/core.h>

struct AccountData;

struct Replication
{
	enum class ActorType: i32
	{
		INVALID = 0,
		Player,
		PlayerCharacter,
		Npc,
	};

	template<ActorType TYPE_>
	struct Actor
	{
		ActorUID actorUID;

		inline ActorType Type() const { return TYPE_; }
	};

	struct ActorPlayer: Actor<ActorType::Player>
	{
		i32 clientID;
		WideString name;
		WideString guildTag;

		eastl::array<ActorUID, PLAYER_CHARACTER_COUNT> characters;
		u8 mainCharaID;
	};

	struct ActorPlayerCharacter: Actor<ActorType::PlayerCharacter>
	{
		i32 clientID; // useful to copy it here
		ActorUID parentActorUID;
		ClassType classType;
		SkinIndex skinIndex;

		vec3 pos;
		vec3 dir;
		f32 speed;
		RotationHumanoid rotation;

		i32 weaponID;
		i32 additionnalOverHeatGauge;
		i32 additionnalOverHeatGaugeRatio;

		ActionStateID actionState;
		i32 actionParam1;
		i32 actionParam2;
	};

	struct ActorNpc: Actor<ActorType::Npc>
	{
		CreatureIndex docID;
		i32 type;
		i32 localID;
		i32 faction;

		vec3 pos;
		vec3 dir;
	};

	struct Frame
	{
		// TODO: replace fixed_map with fixed_hash_map
		eastl::fixed_list<ActorPlayer,2048,true> playerList;
		eastl::fixed_list<ActorPlayerCharacter,2048,true> playerCharaList;
		eastl::fixed_list<ActorNpc,2048,true> npcList;
		eastl::fixed_map<ActorUID,decltype(playerList)::iterator,2048,true> playerMap;
		eastl::fixed_map<ActorUID,decltype(playerCharaList)::iterator,2048,true> playerCharaMap;
		eastl::fixed_map<ActorUID,decltype(npcList)::iterator,2048,true> npcMap;

		eastl::fixed_set<ActorUID,2048> actorUIDSet;
		eastl::fixed_map<ActorUID,ActorType,2048,true> actorType;

		void Clear();

		inline ActorPlayer* FindPlayer(ActorUID actorUID)
		{
			auto found = playerMap.find(actorUID);
			if(found == playerMap.end()) return nullptr;
			return &(*found->second);
		}

		inline ActorPlayerCharacter* FindPlayerChara(ActorUID actorUID)
		{
			auto found = playerCharaMap.find(actorUID);
			if(found == playerCharaMap.end()) return nullptr;
			return &(*found->second);
		}
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

	void Init(Server* server_);

	void FrameEnd();
	void FramePushPlayerActor(const ActorPlayer& actor);
	void FramePushPlayerCharacterActor(const ActorPlayerCharacter& actor);
	void FramePushNpcActor(const ActorNpc& actor);

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

	void SendAccountDataLobby(i32 clientID, const AccountData& account);
	void SendAccountDataPvp(i32 clientID, const AccountData& account);

	void SendConnectToServer(i32 clientID, const AccountData& account, const u8 ip[4], u16 port);
	void SendPvpLoadingComplete(i32 clientID);
	void SendGameReady(i32 clientID);
	void SendGameStart(i32 clientID);
	void SendPlayerTag(i32 clientID, ActorUID mainActorUID, ActorUID subActorUID);
	void SendPlayerJump(i32 clientID, ActorUID mainActorUID, f32 rotate, f32 moveDirX, f32 moveDirY);
	void SendPlayerAcceptCast(i32 clientID, const PlayerCastSkill& cast);

	void EventClientDisconnect(i32 clientID);

	void PlayerRegisterMasterActor(i32 clientiD, ActorUID masterActorUID, ClassType classType); // TODO: temp, find a better solution

	LocalActorID GetLocalActorID(i32 clientID, ActorUID actorUID) const; // Can return INVALID
	ActorUID GetWorldActorUID(i32 clientID, LocalActorID localActorID) const; // Can return INVALID

private:
	void PlayerForceLocalActorID(i32 clientID, ActorUID actorUID, LocalActorID localActorID);

	void UpdatePlayersLocalState();
	void FrameDifference();

	void SendActorPlayerCharacterSpawn(i32 clientID, const ActorPlayerCharacter& actor, const ActorPlayer& parent);
	void SendActorNpcSpawn(i32 clientID, const ActorNpc& actor);
	void SendActorDestroy(i32 clientID, ActorUID actorUID);
	void SendJukeboxPlay(i32 clientID, SongID songID, const wchar* requesterNick, i32 playPosInSec);

	void SendMasterSkillSlots(i32 clientID, const ActorPlayerCharacter& actor);

	void SendInitialFrame(i32 clientID);

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
