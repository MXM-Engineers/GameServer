#pragma once
#include <common/network.h>
#include <common/vector_math.h>
#include <common/utils.h>
#include <common/protocol.h>
#include <common/inner_protocol.h>
#include <EASTL/array.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/fixed_set.h>
#include <EASTL/fixed_map.h>
#include <EASTL/fixed_list.h>
#include <mxm/core.h>

struct Account;

struct MatchingParty
{
	struct Player
	{
		WideString& name;
		u8 userID;
		u8 team;
		u8 isBot;
		// TODO: more useful stuff
	};

	SortieUID sortieUID;
	eastl::fixed_vector<Player,16,false> players;
};

struct HubReplication
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
		vec3 pos;
		vec3 dir;

		inline ActorType Type() const { return TYPE_; }
	};

	struct ActorPlayer: Actor<ActorType::PLAYER>
	{
		ActorUID parentActorUID;
		vec3 eye;
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

	struct Party
	{
		u32 UID;
		ClientHandle leader;
		EntrySystemID entry;
		StageType stageType;
		// TODO: do fancy party stuff later on
	};

	struct Frame
	{
		struct Transform
		{
			vec3 pos;
			vec3 dir;
			vec3 eye;
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

	const ClientLocalMapping* plidMap;

	eastl::array<ClientHandle,MAX_CLIENTS> playerClientHd;
	eastl::array<PlayerState,MAX_CLIENTS> playerState;
	eastl::array<PlayerLocalInfo,MAX_CLIENTS> playerLocalInfo;

	void Init(Server* server_);

	void FrameEnd();
	void FramePushPlayerActor(const ActorPlayer& actor);
	void FramePushNpcActor(const ActorNpc& actor);
	void FramePushJukebox(const ActorJukebox& actor);

	void OnPlayerConnect(ClientHandle clientHd);
	void SendLoadLobby(ClientHandle clientHd, MapIndex stageIndex);
	void SetPlayerAsInGame(ClientHandle clientHd);
	void SendCharacterInfo(ClientHandle clientHd, ActorUID actorUID, CreatureIndex docID, ClassType classType, i32 health, i32 healthMax);
	void SendPlayerSetLeaderMaster(ClientHandle clientHd, ActorUID masterActorUID, ClassType classType, SkinIndex skinIndex);

	void SendChatMessageToAll(const wchar* senderName, i32 chatType, const wchar* msg, i32 msgLen);
	void SendChatMessageToClient(ClientHandle toClientID, const wchar* senderName, i32 chatType, const wchar* msg, i32 msgLen = -1);
	void SendChatWhisperConfirmToClient(ClientHandle senderClientHd, const wchar* destNick, const wchar* msg);
	void SendChatWhisperToClient(ClientHandle destClientHd, const wchar* destNick, const wchar* msg);

	void SendAccountDataLobby(ClientHandle clientHd, const Account& account);

	void SendConnectToServer(ClientHandle clientHd, const Account& account, const u8 ip[4], u16 port);
	void SendGameReady(ClientHandle clientHd);

	void SendCalendar(ClientHandle clientHd);
	void SendAreaPopularity(ClientHandle clientHd, u32 areaID);
	void SendPartyCreateSucess(ClientHandle clientHd, UserID ownerUserID, StageType stageType);

	void SendPartyEnqueue(ClientHandle clientHd);
	void SendMatchingPartyFound(ClientHandle clientHd, const In::MN_MatchingPartyFound& matchingParty);

	void OnClientDisconnect(ClientHandle clientHd);

	void PlayerRegisterMasterActor(ClientHandle clientHd, ActorUID masterActorUID, ClassType classType); // TODO: temp, find a better solution

	LocalActorID GetLocalActorID(ClientHandle clientHd, ActorUID actorUID); // Can return INVALID
	ActorUID GetWorldActorUID(ClientHandle clientHd, LocalActorID localActorID); // Can return INVALID

private:
	void PlayerForceLocalActorID(ClientHandle clientHd, ActorUID actorUID, LocalActorID localActorID);

	void UpdatePlayersLocalState();
	void FrameDifference();

	void SendActorPlayerSpawn(ClientHandle clientHd, const ActorPlayer& actor);
	void SendActorNpcSpawn(ClientHandle clientHd, const ActorNpc& actor);
	void SendJukeboxSpawn(ClientHandle clientHd, const ActorJukebox& actor);
	void SendActorDestroy(ClientHandle clientHd, ActorUID actorUID);
	void SendJukeboxPlay(ClientHandle clientHd, SongID songID, const wchar* requesterNick, i32 playPosInSec);
	void SendJukeboxQueue(ClientHandle clientHd, const ActorJukebox::Track* tracks, const i32 trackCount);

	void SendMasterSkillSlots(ClientHandle clientHd, const ActorPlayer& actor);

	void SendInitialFrame(ClientHandle clientHd);

	void CreateLocalActorID(ClientHandle clientHd, ActorUID actorUID);
	void DeleteLocalActorID(ClientHandle clientHd, ActorUID actorUID);

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
