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

		// Combat stats for tag enter packet
		f32 hp;
		f32 maxHp;
		f32 mana;
		f32 maxMana;
		f32 stamina;
		f32 maxStamina;
		f32 atk;
		f32 defense;
		f32 moveSpeed;
		i32 resourceStatType;
		f32 ultimateGauge;
		f32 maxUltimateGauge;
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

		u8 taggedOut = false;
	};

	struct ActorNpc: Actor<ActorType::Npc>
	{
		CreatureIndex docID;
		i32 localID;
		Faction faction;

		vec3 pos;
		vec3 dir;

		f32 hp = 0;
		f32 maxHp = 0;
		bool hasHP = false;
	};

	struct ActorDynamic: Actor<ActorType::Dynamic>
	{
		CreatureIndex docID;
		i32 localID;
		Faction faction;
		ActionStateID action;
		vec3 pos;
		vec3 rot;

		f32 hp = 0;
		f32 maxHp = 0;
		bool hasHP = false;
	};

	struct SkillCast
	{
		ClientHandle clientHd;
		ActorUID casterUID;
		SkillID skillID;
		ActionStateID actionID;
		vec3 castPos;
		vec3 casterPos;
		vec2 casterMoveDir;
		RotationHumanoid casterRot;
		f32 casterSpeed;
		eastl::fixed_vector<ActorUID,10,false> targetList;
	};

	struct SkillExec
	{
		ActorUID casterUID;
		SkillID skillID;
		ActionStateID actionID;
		vec3 castPos;
		eastl::fixed_vector<ActorUID,10,false> targetList;

		// movement
		vec3 startPos;
		vec3 endPos;
		vec2 moveDir;
		RotationHumanoid rot;
		f32 speed;
		f32 moveDuration;
	};

	struct Frame
	{
		eastl::fixed_list<Player,10,false> playerList;
		eastl::fixed_list<ActorMaster,64,true> masterList;
		eastl::fixed_list<ActorNpc,256,true> npcList;
		eastl::fixed_list<ActorDynamic,128,true> dynamicList;

		eastl::array<decltype(playerList)::iterator,10> playerMap;
		hash_map<ActorUID,decltype(masterList)::iterator,128,true> masterMap;
		hash_map<ActorUID,decltype(npcList)::iterator,128,true> npcMap;
		hash_map<ActorUID,decltype(dynamicList)::iterator,128,true> dynamicMap;

		eastl::fixed_set<ActorUID,2048> actorUIDSet;
		hash_map<ActorUID,ActorType,2048,true> actorType;

		eastl::fixed_vector<SkillCast,40,true> skillCastList;
		eastl::fixed_vector<SkillExec,40,true> skillExecList;

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
	MapIndex mapIndex = MapIndex::PVP_DEATHMATCH;

	void Init(Server* server_);

	void FrameEnd();
	void FramePushPlayer(const Player& player);
	void FramePushMasterActors(const ActorMaster* actorList, const i32 count);
	void FramePushNpcActor(const ActorNpc& actor);
	void FramePushDynamicActor(const ActorDynamic& actor);
	void FramePushSkillCast(const SkillCast& skillCast);
	void FramePushSkillExec(const SkillExec& skillExec);

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
	void SendCountdown(ClientHandle clientHd, i32 curCount, i32 maxCount);
	void SendGameStart(ClientHandle clientHd);
	void SendPlayerTag(ClientHandle clientHd, ActorUID mainActorUID, ActorUID subActorUID);
	void SendPlayerJump(ClientHandle clientHd, ActorUID mainActorUID, f32 rotate, f32 moveDirX, f32 moveDirY);

	// Combat system
	void SendUpdateStatToAll(ActorUID actorUID, u8 statType, f32 maxValue, f32 curValue);
	void SendUpdateStatToClient(ClientHandle clientHd, ActorUID actorUID, u8 statType, f32 maxValue, f32 curValue);
	void SendBroadcastDamage(ActorUID attackerUID, ActorUID targetUID, i32 damage,
		const vec3& attackerPos, const vec3& attackerDir,
		const vec3& hitPos, const vec3& hitDir,
		i32 damageType, i32 skillDocID);
	void SendDeadAck(ActorUID victimUID, ActorUID killerUID, i32 victimDocIndex);

	// Death damage info screen - shows killer portrait, skills used, damage breakdown, assisters
	struct DeadDamageEntry {
		i32 objectID;        // LocalActorID of damage source (per-client, filled during send)
		ActorUID actorUID;   // World ActorUID (resolved to LocalActorID per client)
		i32 skillDocIndex;   // Skill document index, -1 for auto-attack
		i32 statusDocIndex;  // Status document index for icon, -1 for none
		i32 damage;          // Damage amount
		u8  attackerIsMonster; // 0=player, 1=monster
		u8  damageType;      // 0=auto-attack, 1=skill
	};
	struct DeadDamageAttacker {
		i32 attackerKey;     // 1-based player index in match
		ActorUID actorUID;   // Primary ActorUID (for LocalActorID resolution)
		eastl::fixed_vector<DeadDamageEntry, 8, false> entries;
	};
	void SendDeadDamageInfo(ClientHandle victimClientHd, ActorUID victimUID,
		const DeadDamageAttacker& killer,
		const eastl::fixed_vector<DeadDamageAttacker, 8, false>& others,
		i32 durationTimeMs);
	void SendRespawnDelaytime(UserID usn, i32 delayMs);
	void SendRevivePlayerAtStartingPoint(UserID userID, ActorUID activeUID, ActorUID inactiveUID, const vec3& pos);
	void SendPlayerSyncTeleport(ActorUID actorUID, const vec3& pos, const vec3& rot);
	void SendKillNotify(ActorUID killerUID, ActorUID victimUID, i32 skillIndex);
	void SendPvpEventAnnouncement(i32 type, i32 param1, i32 param2, ActorUID killerUID, ActorUID victimUID);
	void SendCancelSkill(ActorUID actorUID, i32 skillIndex);
	void SendTagCooltime(ActorUID actorUID, i32 cooltimeMs);
	void SendChangeBattleState(ActorUID actorUID, bool inBattle, f32 baseMoveSpeed);
	void SendNotifyPcDetailInfos();
	void SendNotifyPcDetailInfosEnemyOnly();
	void SendNotifyPcDetailInfosForClient(ClientHandle clientHd, i32 excludeTeam);
	void SendNotifyPcDetailInfosAllForClient(ClientHandle clientHd); // ALL players for death screen

	// Match results
	struct PlayerScoreData {
		u32 playerIndex;
		i32 kills;
		i32 deaths;
		i32 assists;
		i32 score;
		bool disconnected;
		i32 totalDamageDealt;
		i32 totalDamageReceived;
		i32 highestKillStreak;
	};
	void SendPvpResult(ClientHandle clientHd, i32 gameEndReason, i32 playTimeMs, i32 winningTeam);
	void SendPvpResultScoreDeathmatch(ClientHandle clientHd, const PlayerScoreData* scores, i32 scoreCount,
		i32 redKills, i32 blueKills);
	void SendAuthResultForRTB(ClientHandle clientHd);
	void SendReturnToCity(ClientHandle clientHd);
	void SendDoConnectChannelServer(ClientHandle clientHd);

	// Buff/Debuff system
	void SendAddStatusToAll(i32 statusID, ActorUID targetUID, ActorUID casterUID,
		f32 durationTime, u8 overlapCount = 1, u8 customValue = 0);
	void SendRemoveStatusToAll(i32 statusID, ActorUID targetUID, ActorUID casterUID);

	// Scoreboard
	void SendTeamScoreToAll(i32 redKills, i32 blueKills, i32 redScore, i32 blueScore);
	void SendScoreUpdatePlayer(UserID usn, i32 statID, i32 value);
	void SendScoreUpdateTeam(i32 teamType, i32 statID, i32 value);

	// Pickups
	void SendCreateGroundItem(i32 groundItemID, i32 itemDocIndex, const vec3& pos);
	void SendDestroyGroundItem(i32 groundItemID);

	// VFX system
	void SendRemoteSyncCreateToAll(ActorUID casterUID, i32 remoteDocIndex, i32 remoteSeedID,
		const vec3& firePos, f32 fireYaw, const vec3& targetPos, i32 scale, u8 fireObjectType = 0,
		i32 lifeTimeMs = 0, ActorUID targetUID = ActorUID::INVALID);
	void SendRemoteActivatedToAll(ActorUID casterUID, i32 remoteSeedID,
		ActorUID targetUID, i32 penetrationCount, const vec3& hitPos);

	// Action state broadcast
	void SendActionStateBroadcast(ActorUID actorUID, ActionStateID state, i32 param1, i32 param2);

	void OnPlayerDisconnect(ClientHandle clientHd);

	void PlayerRegisterMasterActor(ClientHandle clientHd, ActorUID masterActorUID, ClassType classType); // TODO: temp, find a better solution

	LocalActorID GetLocalActorID(ClientHandle clientHd, ActorUID actorUID) const; // Can return INVALID
	ActorUID GetWorldActorUID(ClientHandle clientHd, LocalActorID localActorID) const; // Can return INVALID

	// Debug: send raw action change or destroy to a specific client
	void DbgSendActionChange(ClientHandle clientHd, ActorUID uid, i32 actionID);
	void DbgSendDestroy(ClientHandle clientHd, ActorUID uid);

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
