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
	enum
	{
		MAX_PLAYERS = 16
	};

	enum class ActorType: i32
	{
		INVALID = 0,
		Master,
		Npc,
		Dynamic,
	};

	template<ActorType TYPE_>
	struct Actor
	{
		ActorUID actorUID;

		inline ActorType Type() const { return TYPE_; }
	};

	struct Player
	{
		u32 index;
		ClientHandle clientHd;
		UserID userID;
		WideString name;
		WideString guildTag;
		u8 team;

		ClassType mainClass;
		SkinIndex mainSkin;
		ClassType subClass;
		SkinIndex subSkin;

		eastl::array<ActorUID, PLAYER_CHARACTER_COUNT> masters;
		u8 mainCharaID;
		bool hasJumped;
	};

	struct ActorMaster: Actor<ActorType::Master>
	{
		ClientHandle clientHd; // useful to copy it here
		u32 playerIndex;
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

		u8 taggedOut = false;
	};

	struct ActorNpc: Actor<ActorType::Npc>
	{
		CreatureIndex docID;
		i32 localID;
		Faction faction;

		vec3 pos;
		vec3 dir;
	};

	struct ActorDynamic: Actor<ActorType::Dynamic>
	{
		CreatureIndex docID;
		i32 localID;
		Faction faction;
		ActionStateID action;
		vec3 pos;
		vec3 rot;
	};

	struct Frame
	{
		eastl::fixed_list<Player,10,false> playerList;
		eastl::fixed_list<ActorMaster,32,true> masterList;
		eastl::fixed_list<ActorNpc,32,true> npcList;
		eastl::fixed_list<ActorDynamic,32,true> dynamicList;

		eastl::array<decltype(playerList)::iterator,10> playerMap;
		hash_map<ActorUID,decltype(masterList)::iterator,128,true> masterMap;
		hash_map<ActorUID,decltype(npcList)::iterator,128,true> npcMap;
		hash_map<ActorUID,decltype(dynamicList)::iterator,128,true> dynamicMap;

		eastl::fixed_set<ActorUID,2048> actorUIDSet;
		hash_map<ActorUID,ActorType,2048,true> actorType;

		void Clear();

		inline Player* FindPlayer(u32 playerIndex)
		{
			auto found = playerMap[playerIndex];
			if(found == playerList.end()) return nullptr;
			return &(*found);
		}

		inline ActorMaster* FindMaster(ActorUID actorUID)
		{
			auto found = masterMap.find(actorUID);
			if(found == masterMap.end()) return nullptr;
			return &(*found->second);
		}

		inline ActorDynamic* FindDynamic(ActorUID actorUID)
		{
			auto found = dynamicMap.find(actorUID);
			if(found == dynamicMap.end()) return nullptr;
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

	eastl::array<ClientHandle,MAX_PLAYERS> clientHandle;
	eastl::array<PlayerStatePair,MAX_PLAYERS> playerState;
	eastl::array<PlayerLocalInfo,MAX_PLAYERS> playerLocalInfo;

	hash_map<ClientHandle, i32, MAX_PLAYERS> playerMap;

	void Init(Server* server_);

	void FrameEnd();
	void FramePushPlayer(const Player& player);
	void FramePushMasterActors(const ActorMaster* actorList, const i32 count);
	void FramePushNpcActor(const ActorNpc& actor);
	void FramePushDynamicActor(const ActorDynamic& actor);

	void OnPlayerConnect(ClientHandle clientHd, u32 playerIndex);
	void SendLoadPvpMap(ClientHandle clientHd, MapIndex stageIndex);
	void SetPlayerAsInGame(ClientHandle clientHd);
	void SetPlayerLoaded(ClientHandle clientHd);
	void SendCharacterInfo(ClientHandle clientHd, ActorUID actorUID, CreatureIndex docID, ClassType classType, i32 health, i32 healthMax);
	void SendPlayerSetLeaderMaster(ClientHandle clientHd, ActorUID masterActorUID, ClassType classType, SkinIndex skinIndex);

	void SendChatMessageToAll(const wchar* senderName, i32 chatType, const wchar* msg, i32 msgLen);
	void SendChatMessageToClient(ClientHandle toClientHd, const wchar* senderName, EChatType chatType, const wchar* msg, i32 msgLen = -1);

	void SendClientLevelEvent(ClientHandle clientHd, i32 eventID);
	void SendClientLevelEventSeq(ClientHandle clientHd, i32 eventID);

	void SendChatWhisperConfirmToClient(ClientHandle senderClientHd, const wchar* destNick, const wchar* msg);
	void SendChatWhisperToClient(ClientHandle destClientHd, const wchar* destNick, const wchar* msg);

	void SendAccountDataPvp(ClientHandle clientHd);

	void SendPvpLoadingComplete(ClientHandle clientHd);
	void SendGameReady(ClientHandle clientHd, i32 waitTime, i32 elapsed);
	void SendPreGameLevelEvents(ClientHandle clientHd);
	void SendGameStart(ClientHandle clientHd);
	void SendPlayerTag(ClientHandle clientHd, ActorUID mainActorUID, ActorUID subActorUID);
	void SendPlayerJump(ClientHandle clientHd, ActorUID mainActorUID, f32 rotate, f32 moveDirX, f32 moveDirY);
	void SendPlayerAcceptCast(ClientHandle clientHd, const PlayerCastSkill& cast);

	void OnPlayerDisconnect(ClientHandle clientHd);

	void PlayerRegisterMasterActor(ClientHandle clientHd, ActorUID masterActorUID, ClassType classType); // TODO: temp, find a better solution

	LocalActorID GetLocalActorID(ClientHandle clientHd, ActorUID actorUID) const; // Can return INVALID
	ActorUID GetWorldActorUID(ClientHandle clientHd, LocalActorID localActorID) const; // Can return INVALID

private:
	void UpdatePlayersLocalState();
	void FrameDifference();

	void SendActorMasterSpawn(ClientHandle clientHd, const ActorMaster& actor, const Player& parent);
	void SendActorNpcSpawn(ClientHandle clientHd, const ActorNpc& actor);
	void SendActorDynamicSpawn(ClientHandle clientHd, const ActorDynamic& actor);
	void SendActorDestroy(ClientHandle clientHd, ActorUID actorUID);

	void SendMasterSkillSlots(ClientHandle clientHd, const ActorMaster& actor);

	void SendInitialFrame(ClientHandle clientHd);

	void PlayerForceLocalActorID(i32 clientID, ActorUID actorUID, LocalActorID localActorID);
	void CreateLocalActorID(i32 clientID, ActorUID actorUID);
	void DeleteLocalActorID(i32 clientID, ActorUID actorUID);

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
		NT_LOG("[client%x] Replication :: %s", clientHd, PacketSerialize<Packet>(packetData, packetSize));
		server->SendPacketData(clientHd, Packet::NET_ID, packetSize, packetData);
	}

	typedef eastl::fixed_vector<ClientHandle,MAX_PLAYERS,false> ClientList;
	void GetPlayersInGame(ClientList* list);
};
