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
		ClientHandle clientHd;
		UserID userID = UserID::INVALID;
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
		ClientHandle clientHd; // useful to copy it here
		UserID playerID;
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

		eastl::fixed_map<UserID,decltype(playerList)::iterator,2048,true> playerMap;
		eastl::fixed_map<ActorUID,decltype(masterList)::iterator,2048,true> masterMap;
		eastl::fixed_map<ActorUID,decltype(npcList)::iterator,2048,true> npcMap;

		eastl::fixed_set<ActorUID,2048> actorUIDSet;
		eastl::fixed_map<ActorUID,ActorType,2048,true> actorType;

		eastl::fixed_map<i32,UserID,10,false> clientPlayerMap;

		void Clear();

		inline Player* FindPlayer(UserID playerID)
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

	const ClientLocalMapping* plidMap;

	eastl::array<ClientHandle,MAX_CLIENTS> clientHandle;
	eastl::array<PlayerStatePair,MAX_CLIENTS> playerState;
	eastl::array<PlayerLocalInfo,MAX_CLIENTS> playerLocalInfo;

	eastl::array<const AccountData*,MAX_CLIENTS> playerAccountData;

	void Init(Server* server_);

	void FrameEnd();
	void FramePushPlayer(const Player& player);
	void FramePushMasterActor(const ActorMaster& actor);
	void FramePushNpcActor(const ActorNpc& actor);

	void OnPlayerConnect(ClientHandle clientHd, const AccountData* account);
	void SendLoadPvpMap(ClientHandle clientHd, StageIndex stageIndex);
	void SetPlayerAsInGame(ClientHandle clientHd);
	void SetPlayerLoaded(ClientHandle clientHd);
	void SendCharacterInfo(ClientHandle clientHd, ActorUID actorUID, CreatureIndex docID, ClassType classType, i32 health, i32 healthMax);
	void SendPlayerSetLeaderMaster(ClientHandle clientHd, ActorUID masterActorUID, ClassType classType, SkinIndex skinIndex);

	void SendChatMessageToAll(const wchar* senderName, i32 chatType, const wchar* msg, i32 msgLen);
	void SendChatMessageToClient(ClientHandle toClientHd, const wchar* senderName, i32 chatType, const wchar* msg, i32 msgLen = -1);
	void SendChatWhisperConfirmToClient(ClientHandle senderClientHd, const wchar* destNick, const wchar* msg);
	void SendChatWhisperToClient(ClientHandle destClientHd, const wchar* destNick, const wchar* msg);

	void SendAccountDataPvp(ClientHandle clientHd);

	void SendConnectToServer(ClientHandle clientHd, const AccountData& account, const u8 ip[4], u16 port);
	void SendPvpLoadingComplete(ClientHandle clientHd);
	void SendGameReady(ClientHandle clientHd);
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
};
