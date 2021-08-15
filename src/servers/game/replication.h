#pragma once
#include <common/network.h>
#include <common/vector_math.h>
#include <common/utils.h>
#include <common/protocol.h>
#include <common/packet_serialize.h>
#include <EASTL/array.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/fixed_set.h>
#include <EASTL/fixed_map.h>
#include <EASTL/fixed_list.h>
#include <mxm/core.h>

struct AccountData;

struct Replication
{
	enum class ActorType: i32
	{
		INVALID = 0,
		Master,
		Npc,
	};

	template<ActorType TYPE_>
	struct Actor
	{
		ActorUID actorUID;

		inline ActorType Type() const { return TYPE_; }
	};

	struct Player
	{
		i32 clientID;
		PlayerID playerID = PlayerID::INVALID;
		WideString name;
		WideString guildTag;

		ClassType mainClass;
		SkinIndex mainSkin;
		ClassType subClass;
		SkinIndex subSkin;

		eastl::array<ActorUID, PLAYER_CHARACTER_COUNT> characters;
		u8 mainCharaID;
		bool hasJumped;
	};

	struct ActorMaster: Actor<ActorType::Master>
	{
		i32 clientID; // useful to copy it here
		PlayerID playerID;
		ClassType classType;
		SkinIndex skinIndex;

		vec3 pos = vec3(0);
		vec2 moveDir = vec2(0);
		f32 speed = 0;
		RotationHumanoid rotation = {0};

		i32 weaponID;
		i32 additionnalOverHeatGauge;
		i32 additionnalOverHeatGaugeRatio;

		ActionStateID actionState;
		i32 actionParam1;
		i32 actionParam2;

		SkillID castSkill = SkillID::INVALID;
		vec3 skillStartPos;
		vec3 skillEndPos;
		f32 skillMoveDurationS;
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
		eastl::fixed_list<Player,10,false> playerList;
		eastl::fixed_list<ActorMaster,2048,true> masterList;
		eastl::fixed_list<ActorNpc,2048,true> npcList;

		eastl::fixed_map<PlayerID,decltype(playerList)::iterator,2048,true> playerMap;
		eastl::fixed_map<ActorUID,decltype(masterList)::iterator,2048,true> masterMap;
		eastl::fixed_map<ActorUID,decltype(npcList)::iterator,2048,true> npcMap;

		eastl::fixed_set<ActorUID,2048> actorUIDSet;
		eastl::fixed_map<ActorUID,ActorType,2048,true> actorType;

		eastl::fixed_map<i32,PlayerID,10,false> clientPlayerMap;

		void Clear();

		inline Player* FindPlayer(PlayerID playerID)
		{
			auto found = playerMap.find(playerID);
			if(found == playerMap.end()) return nullptr;
			return &(*found->second);
		}

		inline Player* FindClientPlayer(i32 clientID)
		{
			auto found = clientPlayerMap.find(clientID);
			if(found == clientPlayerMap.end()) return nullptr;
			return FindPlayer(found->second);
		}

		inline ActorMaster* FindMaster(ActorUID actorUID)
		{
			auto found = masterMap.find(actorUID);
			if(found == masterMap.end()) return nullptr;
			return &(*found->second);
		}
	};

	enum class PlayerState: u8 {
		DISCONNECTED=0,
		CONNECTED,
		IN_GAME,
		LOADED
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

	struct PlayerStatePair
	{
		PlayerState prev;
		PlayerState cur;
	};

	eastl::array<PlayerStatePair,Server::MAX_CLIENTS> playerState;
	eastl::array<PlayerLocalInfo,Server::MAX_CLIENTS> playerLocalInfo;

	eastl::array<const AccountData*,Server::MAX_CLIENTS> playerAccountData;

	void Init(Server* server_);

	void FrameEnd();
	void FramePushPlayer(const Player& player);
	void FramePushMasterActor(const ActorMaster& actor);
	void FramePushNpcActor(const ActorNpc& actor);

	void OnPlayerConnect(i32 clientID, const AccountData* account);
	void SendLoadPvpMap(i32 clientID, StageIndex stageIndex);
	void SetPlayerAsInGame(i32 clientID);
	void SetPlayerLoaded(i32 clientID);
	void SendCharacterInfo(i32 clientID, ActorUID actorUID, CreatureIndex docID, ClassType classType, i32 health, i32 healthMax);
	void SendPlayerSetLeaderMaster(i32 clientID, ActorUID masterActorUID, ClassType classType, SkinIndex skinIndex);

	void SendChatMessageToAll(const wchar* senderName, i32 chatType, const wchar* msg, i32 msgLen);
	void SendChatMessageToClient(i32 toClientID, const wchar* senderName, i32 chatType, const wchar* msg, i32 msgLen = -1);
	void SendChatWhisperConfirmToClient(i32 senderClientID, const wchar* destNick, const wchar* msg);
	void SendChatWhisperToClient(i32 destClientID, const wchar* destNick, const wchar* msg);

	void SendAccountDataPvp(i32 clientID);

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

	void SendActorMasterSpawn(i32 clientID, const ActorMaster& actor, const Player& parent);
	void SendActorNpcSpawn(i32 clientID, const ActorNpc& actor);
	void SendActorDestroy(i32 clientID, ActorUID actorUID);

	void SendMasterSkillSlots(i32 clientID, const ActorMaster& actor);

	void SendInitialFrame(i32 clientID);

	void CreateLocalActorID(i32 clientID, ActorUID actorUID);
	void DeleteLocalActorID(i32 clientID, ActorUID actorUID);

	template<typename Packet>
	inline void SendPacket(i32 clientID, const Packet& packet)
	{
		SendPacketData<Packet>(clientID, sizeof(packet), &packet);
	}

	template<typename Packet>
	inline void SendPacket(i32 clientID, const PacketWriter& writer)
	{
		SendPacketData<Packet>(clientID, writer.size, writer.data);
	}

	template<typename Packet>
	inline void SendPacketData(i32 clientID, u16 packetSize, const void* packetData)
	{
		NT_LOG("[client%03d] Replication :: %s", clientID, PacketSerialize<Packet>(packetData, packetSize));
		server->SendPacketData(clientID, Packet::NET_ID, packetSize, packetData);
	}
};
