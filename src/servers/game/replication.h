#pragma once
#include <common/network.h>
#include <common/vector_math.h>
#include <common/utils.h>
#include <common/protocol.h>
#include <common/packet_serialize.h>
#include <common/stat.h>
#include <mxm/account_progress.h>
#include <EASTL/array.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/fixed_set.h>
#include <EASTL/fixed_map.h>
#include <EASTL/fixed_list.h>
#include <mxm/core.h>

struct AccountData;

static const wchar* const FIRE_OBJECT_SCENE_ROOT = L"Scene Root";
static const u8 MEASURED_REMOTE_CREATE_FLAG_PARENT_REMOTE = 8;
static const wchar* const FIRE_OBJECT_ATTACK_PREFIX = L"Attack";

inline bool FireObjectStartsWith(const wchar* name, u16 len, const wchar* prefix)
{
	const u16 n = (u16)EA::StdC::Strlen(prefix);
	return len >= n && EA::StdC::Strncmp(name, prefix, n) == 0;
}

inline bool FireObjectIs(const wchar* name, u16 len, const wchar* expected)
{
	return len == (u16)EA::StdC::Strlen(expected) && FireObjectStartsWith(name, len, expected);
}

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
		eastl::array<SkillID,4> skills;
		eastl::array<SkillID,2> stageSkills;
		eastl::fixed_vector<eastl::pair<i32,u8>,16,false> skillLevels;
		i32 supportKit = -1;
		u8 badgeType = 0;
		u8 badgeTier = 0;
		i32 displayTitle = -1;
		i32 statTitle = -1;
		WeaponIndex mainWeapon;
		WeaponIndex subWeapon;
		i32 mainWeaponGrade;
		i32 subWeaponGrade;
		u8 mainMasterGearNo;
		u8 subMasterGearNo;
		i32 mainCharacterType;
		i32 subCharacterType;

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
		i32 hp;
		i32 hpMax;
		i32 seed;
		const StatValue* initStats = nullptr;
		u16 initStatCount = 0;
		i32 spawnAnim = -1;
		i32 ownerID = 0;
		u8 dirToNearPC = 0;
		i32 wanderDist = -1;
		i32 tagID = -1;




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
		bool skillMoving = false;
		bool graphMoving = false;
		bool statusGraph = false;
		bool graphHold = false;
	};

	struct ActorNpc: Actor<ActorType::Npc>
	{
		CreatureIndex docID;
		i32 localID;
		Faction faction;
		vec3 createRot = vec3(0);
		bool hasCreateRot = false;
		i32 spawnAnim = 0;
		i32 ownerID = 0;
		u8 dirToNearPC = 0;
		i32 wanderDist = -1;
		i32 tagID = -1;
		ActionStateID actionState = ActionStateID::INVALID;
		i32 seed = 0;
		i32 entityType = 1;
		ActorUID ownerUID = ActorUID::INVALID;
		f32 ownerSkillPower = 0;
		i32 hp = 0;
		i32 growthLevel = 0;
		f32 statScale = 1.0f;
		f32 stateSpeed = 0;
		vec2 dashGoal = vec2(0);
		f32 dashSpeed = 0;


		vec3 pos;
		vec3 dir;
	};

	struct ActorDynamic: Actor<ActorType::Dynamic>
	{
		CreatureIndex docID;
		i32 localID;
		Faction faction;
		ActionStateID action;
		i32 spawnAnim = 0;
		i32 ownerID = 0;
		u8 dirToNearPC = 0;
		i32 wanderDist = -1;
		i32 tagID = -1;
		i32 seed = 0;
		i32 entityType = 3;

		vec3 pos;
		vec3 rot;
	};

	struct SkillCast
	{
		ClientHandle clientHd;
		ActorUID casterUID;
		SkillID skillID;
		ActionStateID actionID;
		i8 costLevel = 0;
		bool sendAction = false;
		vec3 castPos;
		vec3 casterPos;
		vec2 casterMoveDir;
		RotationHumanoid casterRot;
		f32 casterSpeed;
		f32 clientTime;
		eastl::fixed_vector<ActorUID,10,false> targetList;
	};

	struct SkillExec
	{
		ActorUID casterUID;
		SkillID skillID;
		ActionStateID actionID;
		i8 costLevel = 0;
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
		eastl::fixed_list<ActorMaster,32,true> masterList;
		eastl::fixed_list<ActorNpc,32,true> npcList;
		eastl::fixed_list<ActorDynamic,32,true> dynamicList;

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
		eastl::fixed_map<ActorUID,LocalActorID,256> departedActorIDMap;
		eastl::fixed_set<ActorUID,2048> actorUIDSet;
		LocalActorID nextPlayerLocalActorID;
		LocalActorID nextNpcLocalActorID;
		LocalActorID nextMonsterLocalActorID;
		eastl::fixed_set<ActorUID,256> hiddenActors;
		eastl::fixed_map<ActorUID,ActorUID,32> shownMain;
		eastl::fixed_map<ActorUID,u16,512> monsterSpeedSent;

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

	struct SentMove
	{
		vec2 moveDir = vec2(0);
		f32 speed = 0;
		i32 stillFrames = 0;
		Time at = Time::ZERO;
		vec3 pos = vec3(0);
		f32 upperYaw = 0;
		f32 upperPitch = 0;
		f32 bodyYaw = 0;
		vec3 probePos = vec3(0);
		Time probeAt = Time::ZERO;
		f32 measuredSpeed = -1.0f;
		bool blocked = false;
	};
	eastl::fixed_map<ActorUID,SentMove,64> sentMoves;
	eastl::fixed_map<ActorUID,i16,64> npcActionStopYaw;
	void NoteNpcMoveFlag(ActorUID actorUID, bool actionStop, i16 yaw);

	hash_map<ClientHandle, i32, MAX_PLAYERS> playerMap;

	u32 damageSeq = 1;
	i32 inGameID = 0;
	GameType gameType = GameType::INVALID;
	AreaIndex areaIndex = AreaIndex(0);
	StageIndex stageIndex = StageIndex(0);
	MapIndex mapIndex = MapIndex(0);
	u8 canEscape = 0;
	u8 isTrespass = 0;
	i32 surrenderAbleTime = 0;
	bool titanMode = false;
	bool occupyMode = false;
	bool pveTeams = false;
	i32 PlayerTeamType(i32 team) const { return pveTeams ? (i32)TeamType::PLAYER : (i32)TeamType::RED + team; }
	TeamType ActorTeamType(Faction faction) const { return pveTeams && (faction == Faction::RED || faction == Faction::BLUE) ? TeamType::PLAYER : FactionTeamType(faction); }
	i8 titanNextIndex = 0;
	i32 supportKitIndex = -1;

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
	void SendGameAreaMove(ClientHandle clientHd, MapIndex map);
	struct StageMember { ClassType mainClass; ClassType subClass; const wchar* name; };
	struct RewardBonus { u32 type; u32 amount; };
	typedef eastl::fixed_vector<RewardBonus,4,false> RewardBonusList;
	struct StageClear { LocalActorID player; i32 stage; i32 kills; i32 deaths; u32 playTimeMs; u8 rank; u32 baseRon; u32 baseExp; u32 attackScore; u32 defenseScore; u32 timeScore; u32 totalScore; u8 technicGrade; RewardBonusList bonusRon; RewardBonusList bonusExp; };
	struct RewardCard { u32 type; i32 index; u32 amount; };
	struct DefenceClear { LocalActorID player; i32 stage; i32 clearStep; u32 clearMs; u32 baseRon; u32 baseExp; RewardBonusList bonusRon; RewardBonusList bonusExp; eastl::fixed_vector<RewardCard,4,false> items; u32 guildBase; u32 guildBonus; u8 guildMembers; u16 boostGold; u16 boostExp; };
	void SendDefenceClearResult(ClientHandle clientHd, const DefenceClear& clear, const StageMember* members, i32 memberCount);
	void SendPhaseStartToAll(i32 step);
	enum : u32 { SCORE_TYPE_MINERAL_CARRIER_KILLS = 0, SCORE_TYPE_MINERAL = 40 };
	void SendTeamScoreToAll(u32 type, i32 value);
	void SendDefenceBestRecord(ClientHandle clientHd, i32 personalStep, u32 personalMs, i32 worldStep, u32 worldMs);
	void SendPhaseEndToAll(i32 step, u32 reason, u32 clearMs);
	void SendStageClearResult(ClientHandle clientHd, const StageClear& clear, const StageMember* members, i32 memberCount);
	void SendExpGain(ClientHandle clientHd, u32 exp);
	void SendClearedStage(ClientHandle clientHd, i32 stage);
	void SendRewardItemList(ClientHandle clientHd, const RewardCard* cards, i32 count);
	void SendRewardStageStart(ClientHandle clientHd, u32 waitMs, u8 slotCount);
	void SendRewardSelectSlot(ClientHandle clientHd, u32 result, u32 slot, const RewardCard& card);
	void SendRewardRequestFinish(ClientHandle clientHd);
	void SendRewardStageEnd(ClientHandle clientHd, const RewardCard* cards, i32 count);
	void SendWarnPveGameEnd(ClientHandle clientHd, u32 remainMs);
	void SendStageRank(ClientHandle clientHd, u32 rank, u32 timeScoreRank);
	void SendUserLifeInfo(ClientHandle clientHd, i32 players);
	void SendPrepareHandover(ClientHandle clientHd);
	void ResetClientForAreaMove(ClientHandle clientHd);
	void SendMasterTrainingEvent(ClientHandle clientHd, u8 eventType, u32 ret, u32 entityID, u32 param1, u32 param2);
	void SetPlayerAsInGame(ClientHandle clientHd);
	void SetPlayerLoaded(ClientHandle clientHd);
	void SendCharacterInfo(ClientHandle clientHd, ActorUID actorUID, CreatureIndex docID, ClassType classType, i32 health, i32 healthMax);
	void SendPlayerSetLeaderMaster(ClientHandle clientHd, ActorUID masterActorUID, ClassType classType, SkinIndex skinIndex);

	void SendChatMessageToAll(const wchar* senderName, i32 chatType, const wchar* msg, i32 msgLen);
	void SendChatMessageToClient(ClientHandle toClientHd, const wchar* senderName, EChatType chatType, const wchar* msg, i32 msgLen = -1);

	void SendClientLevelEvent(ClientHandle clientHd, i32 eventID, ActorUID caller = ActorUID::INVALID);
	void SendClientLevelEventToAll(i32 eventID, ActorUID caller = ActorUID::INVALID);
	void SendClientLevelEventSeqToAll(i32 eventID, ActorUID caller = ActorUID::INVALID);
	void SendCreateGroundItemToAll(u32 itemUID, i32 itemIndex, const vec3& pos, i32 gettableType = 0, i32 factionType = -1);
	void SendCreatePrivateGroundItem(ClientHandle clientHd, ActorUID ownerUID, u32 itemUID, bool ron, i32 itemIndex, i32 count, const vec3& pos);
	void SendGroundItemTaken(ClientHandle clientHd, ActorUID takerUID, u32 itemUID, u32 itemISN);
	void SendMoney(ClientHandle clientHd, i64 gold, u32 reason);
	void SendItemMerge(ClientHandle clientHd, u32 itemUID, i32 count);
	void SendItemUpdate(ClientHandle clientHd, const AccountItem& item, u32 reason);
	void SendGroundItemTaken(ClientHandle clientHd, ActorUID takerUID, u32 itemUID);
	void SendPublicGroundItemTaken(ClientHandle takerHd, ActorUID takerUID, u32 itemUID);
	void SendGroundItemAnswer(ClientHandle clientHd, ActorUID takerUID, u32 itemUID, u32 itemISN);
	void SendDestroyGroundItemToAll(u32 itemUID);
	void SendClientLevelEventSeq(ClientHandle clientHd, i32 eventID);

	void SendChatWhisperConfirmToClient(ClientHandle senderClientHd, const wchar* destNick, const wchar* msg, ErrorType retval);
	void SendChatWhisperToClient(ClientHandle destClientHd, const wchar* destNick, const wchar* msg);

	void SendAccountDataPvp(ClientHandle clientHd);

	void SendPvpLoadingComplete(ClientHandle clientHd);
	void SendGameReady(ClientHandle clientHd, i32 waitTime, i32 elapsed, UserID userId);
	void SendActivatedSupportKit(ClientHandle clientHd, LocalActorID chara, i32 kit, i32 coolTimeID, f32 coolTime);
	void SendKitConsumed(ClientHandle clientHd, u32 uid, u8 invenType, i32 slot, i32 doc, i32 count);
	void SendSkillPoints(ClientHandle clientHd, UserID userId, i32 points);
	void SendSkillUpgradeResult(ClientHandle clientHd, i32 result, ActorUID mainUID, SkillID mainSkill, Slice<const i32> mainProps, ActorUID subUID, SkillID subSkill, Slice<const i32> subProps);
	void SendPlayerSkillUpdateToAll(ActorUID charaUID, SkillID skill, bool active, Slice<const i32> props);

	void SendPreGameLevelEvents(ClientHandle clientHd);
	void SendGameStart(ClientHandle clientHd);
	void SendPlayerTag(ClientHandle clientHd, ActorUID mainActorUID, ActorUID subActorUID);
	void SendPlayerJump(ClientHandle clientHd, ActorUID mainActorUID, f32 rotate, f32 moveDirX, f32 moveDirY);
	void SendDamageToAll(ActorUID attackerUID, ActorUID targetUID, SkillID skillID, i32 damage, i32 hp, i32 hpMax, u32 damageType = 0, i32 statusID = 0);
	void SendHpToAll(ActorUID actorUID, i32 hp, i32 hpMax);
	void SendHp(ClientHandle clientHd, ActorUID actorUID, i32 hp, i32 hpMax);
	void SendPlayerSyncTeleport(ClientHandle clientHd, ActorUID actorUID, const vec3& pos);
	void SendBushStateToAll(ActorUID actorUID, ActorUID bushUID);
	void SendLightArea(ClientHandle clientHd, bool on, u32 cell, u8 team, u8 radius);
	void SendTagCooltime(ClientHandle clientHd, ActorUID actorUID, u32 ms);
	void SendUltReady(ClientHandle clientHd, ActorUID actorUID, bool ready);
	void SendTeamPointsToAll(u8 team, i32 teamPoints);
	struct EnterGraph { vec3 start = vec3(0); vec3 end = vec3(0); f32 dist = 0; f32 originDist = 0; f32 totalSec = 0; f32 curSec = 0; };
	void SendEnterNpc(ClientHandle clientHd, ActorUID actorUID, CreatureIndex docID, const vec3& pos, f32 yaw, f32 speed, ActionStateID action, i32 hp, i32 hpMax, f32 growthLevel, const EnterGraph* graph = nullptr);
	void SendEnterDynamic(ClientHandle clientHd, ActorUID actorUID, const vec3& pos, f32 yaw, ActionStateID action, bool jumpWall, f32 curHp, f32 maxHp);
	void SendDoConnectChannel(ClientHandle clientHd, const wchar* nickname, const i32 hubIP[4], u16 hubPort);
	struct EndPlayer
	{
		u32 playerIndex = 0;
		UserID userID;
		ClientHandle clientHd;
		const wchar* name;
		u8 team;
		i32 kills;
		i32 deaths;
		CreatureIndex masters[2];
		u32 gold = 0;
		u32 exp = 0;
		RewardBonusList bonusRon;
		RewardBonusList bonusExp;
		u32 medals = 0;
		u8 tierType = 0;
		u16 tierPoints = 0;
		i32 assists = 0;
		i32 bestKillStreak = 0;
		i32 points = 0;
		u8 ingameLevel = 1;
		i64 damageDealt = 0;
		u8 occupyCaptures = 0;
		i32 occupyScore = 0;
		i32 savedPieces = 0;
		eastl::array<u8,4> monsterKills = {};
		eastl::array<u8,4> traits[2] = {};
		eastl::array<u32,9> damageStats = {};
		struct MasterScore { i32 kills = 0; i32 deaths = 0; i32 assists = 0; i64 damageDealt = 0; eastl::array<u32,9> damageStats = {}; u32 playMs = 0; };
		u32 guildBase = 0;
		u32 guildBonus = 0;
		u8 guildMembers = 0;
		eastl::array<u16,2> guildBoost = {};
		eastl::array<MasterScore,2> masterScores;
		u8 statueCaptures = 0;
		u8 visionCaptures = 0;
	};
	struct EndPraise
	{
		UserID userID;
		const wchar* name;
		u8 team;
		CreatureIndex activeMaster;
		CreatureIndex inactiveMaster;
		u8 type;
		u32 value;
		u8 relation;
		u32 relationValue;
	};
	struct EndGot
	{
		eastl::fixed_vector<i32,32,false> summons[2];
		eastl::fixed_vector<EndPraise,10,false> praises;
	};
	void SendGameEnd(const EndPlayer* players, i32 count, i32 redScore, i32 blueScore, u32 playTimeMs, i32 winnerTeam = -1, const EndGot* got = nullptr);
	void SendGauge(ClientHandle clientHd, UserID userID, u8 updateType, bool overheated, f32 overheatTime, u16 gauge, u16 maxGauge);
	void SendStatToAll(ActorUID actorUID, i32 statType, f32 cur, f32 max, u32 reason = 0);
	void SendDespawnMonsterToAll(ActorUID actorUID);
	void SendBushCurStateToAll(ActorUID bushUID, u32 state);
	void SendStanceToAll(ActorUID actorUID, i32 stance, f32 durationSec);
	void SendNpcBehaviorToAll(ActorUID actorUID, ActionStateID action, const vec3& pos, const vec3& targetPos, ActorUID targetUID, f32 moveSpeed);
	void SendAiActionStateToAll(ActorUID actorUID, ActionStateID action, f32 worldYaw, const vec3& pos, bool keepSpeed, f32 speed = 0.0f);
	void SendAiSetTargetToAll(ActorUID actorUID, ActorUID targetUID);
	void SendExecuteSkillAck(ClientHandle clientHd, ActorUID casterUID, SkillID skillID);
	void SendCancelSkillToAll(ActorUID casterUID, SkillID skillID, ClientHandle skipHd = ClientHandle::INVALID);
	void SendInteractionStatusToAll(ActorUID objectUID, u32 allowedTeamMask, i32 interactionID, i32 occupiedTeam, ClientHandle exclude = ClientHandle::INVALID, f32 coolTime = 0);
	void SendCastSkillRefused(ClientHandle clientHd, ActorUID casterUID, SkillID skillID, u32 ret, const vec3& casterPos, f32 angle, f32 speed, f32 clientTime);
	void SendFrameSkillPackets();
	void SendInteractionCastingToAll(ActorUID casterUID, ActorUID objectUID, u32 actionState, u32 castingTimeMS);
	void SendInteractionExecuteOnlyToAll(ActorUID casterUID, ActorUID objectUID, u32 actionState);
	void SendInteractionCancelToAll(ActorUID casterUID, ActorUID objectUID);
	void SendInteractionExecuteToAll(ActorUID objectUID, ActorUID targetUID, u32 actionState, u32 castingTimeMS);
	void SendToggleSkill(ActorUID casterUID, ClientHandle casterHd, SkillID skillID, bool on, ActionStateID action);
	void SendHideDetect(ClientHandle clientHd, ActorUID detectorUID, ActorUID targetUID, u8 flag);
	void SendLeaveActor(ClientHandle clientHd, ActorUID actorUID);
	void MarkHidden(ClientHandle clientHd, ActorUID actorUID);
	void SendEnterActor(ClientHandle clientHd, ActorUID actorUID, ClassType classType, const vec3& pos, f32 bodyYaw, f32 upperYaw, const vec3& moveTo, bool battle, f32 speed, ActionStateID action, i32 hp, i32 hpMax);
	void SendChangeSkillSlot(ClientHandle clientHd, ActorUID actorUID, i32 slot, SkillID skill, i32 resetCooltimeID);
	void SendTeleportRequest(ClientHandle clientHd, ActorUID actorUID, const vec3& pos);
	void SendPlayerSyncTeleportToAll(ActorUID actorUID, const vec3& pos);
	void SendSelfTeleport(ClientHandle clientHd, ActorUID actorUID, const vec3& pos, f32 worldYaw, const vec3* clientPos = nullptr);
	void SendSelfMove(ClientHandle clientHd, ActorUID actorUID, const vec3& pos, const vec2& moveDir, f32 speed, f32 worldYaw);
	void RelayPlayerMove(ClientHandle fromHd, ActorUID actorUID, const Cl::CN_GameUpdatePosition& update);
	void RelayPlayerTurn(ClientHandle fromHd, ActorUID actorUID, const Cl::CN_GameUpdateRotation& update);
	void SendCooltimeReset(ClientHandle clientHd, ActorUID characterUID, Slice<const i32> coolTimeIDs, u32 leftMs = 1, u32 totalMs = 1);
	void SendAddStatusToAll(i32 statusID, ActorUID targetUID, ActorUID casterUID, f32 durationSec, const vec3& from, const vec3& to, f32 graphDuration, i32 overlap = 1, f32 scale = 1.0f, f32 nominalDist = -1.0f, bool withGraph = true, bool refresh = false, u8 customValue = 0);
	void SendRemoveStatusToAll(i32 statusID, ActorUID targetUID, ActorUID casterUID);
	bool linkMirror = true;
	LocalActorID GetCasterLocalID(ClientHandle clientHd, ActorUID actorUID) const;
	struct RemoteDamage
	{
		u32 remoteID = 0;
		RemoteIdx remoteDoc = RemoteIdx::INVALID;
		u32 pierceHits = 0;
		ActorUID attackerUID = ActorUID::INVALID;
		ActorUID targetUID = ActorUID::INVALID;
		vec3 remotePos = vec3(0);
		vec3 targetPos = vec3(0);
		vec2 dir = vec2(1, 0);
		u32 damageType = 0;
		u32 elementRelation = 0;
		i32 damage = 0;
		i32 hp = 0;
		i32 hpMax = 0;
		ClientHandle seedOwnerHd = ClientHandle::INVALID;
	};
	void SendRemoteDamageToAll(const RemoteDamage& hit);
	void SendRemoteActivatedToAll(u32 remoteID, ClientHandle seedOwnerHd, ClientHandle skipHd, ActorUID targetUID, u32 penetration, const vec3& pos);
	void SendResistStatusToAll(ActorUID targetUID, ActorUID casterUID, u32 event);
	void SendBattleStateToAll(ActorUID actorUID, bool inBattle, f32 baseMoveSpeed);
	void SendMasterGroupingEffect(ClientHandle clientHd, ActorUID masterUID, i32 index, ClassType classType, bool activate);
	void SendTargetGraphMoveToAll(ActorUID casterUID, const vec3& casterPos, ActorUID targetUID, const vec3& targetPos, ActionStateID actionID);
	void SendEvadeToAll(ActorUID attackerUID, ActorUID defenderUID, u32 remoteID, RemoteIdx remoteDoc, ClientHandle seedOwnerHd);
	void SendModifyStatusToAll(i32 statusID, ActorUID targetUID, ActorUID casterUID, u8 overlap, u8 customValue, ActorUID replaceCasterUID = ActorUID::INVALID);
	void SendAttackActionToOthers(ClientHandle senderHd, ActorUID actorUID, f32 mxmYaw, ActionStateID state);
	void SendActionStateOnlyToAll(ActorUID actorUID, ActionStateID state, f32 mxmYaw, bool move, const vec3& startPos, const vec3& endPos, f32 durationSec);
	void SendDestroyRemoteToAll(u32 remoteID);
	void SendNpcMoveToAll(ActorUID actorUID, const vec3& pos, const vec3& goal, f32 worldYaw, f32 speed);
	void SendNpcTurnToAll(ActorUID actorUID, f32 worldYaw);
	void SendMonsterStateToAll(ActorUID actorUID, ActionStateID action, const vec3& pos, f32 speed);
	void SendLevelIngameToAll(UserID userID, i32 expToLevelUp, i32 level);
	void SendExpIngame(ClientHandle clientHd, UserID userID, i32 gain, i32 current, ActorUID target);
	void SendCharacterPropertyPoint(ClientHandle clientHd, ActorUID character, i32 points);
	void SendCharacterPropertyInfoToAll(ActorUID character, const eastl::array<u8,4>& levels);
	void SendCharacterPropertyResetToAll(ActorUID character);
	void SendTraitAnswer(ClientHandle clientHd, bool reset, u32 error);
	void SendSafeZone(ClientHandle clientHd, UserID userID, bool inside);
	void SendPvpAnnouncementRaw(ClientHandle clientHd, u32 type, u32 data, u32 score, u32 attacker);
	void SendTransformAnswer(ClientHandle clientHd, ActorUID charUID, i32 doc);
	void SendMonsterRespawnTimeToAll(u32 stringKey, u32 localID, u32 remainMs);
	void SendTitanFormToAll(bool modeChange, ActorUID charUID, i32 doc, u32 durationMs, i32 weapon, i32 skill1, i32 skill2, i32 ug, const StatValue* stats, u16 count, f32 curHp, f32 curStamina, f32 curUlt, f32 moveSpeed);
	void SendRequestVoting(ClientHandle clientHd, UserID requestor, u32 type, u32 periodMs, i32 voters, bool modify);
	void SendVote(ClientHandle clientHd, u32 type, u32 choice);
	void SendVotingResult(ClientHandle clientHd, UserID candidate, u32 type, u32 state, u32 reason, u32 playerCool, u32 teamCool);
	void SendMonsterStopWithSpeedToAll(ActorUID actorUID, const vec3& pos, f32 speed);
	void SendMonsterStopTurnToAll(ActorUID actorUID, const vec3& pos, f32 fromYaw, f32 toYaw, f32 speed);
	void SendMonsterMoveToAll(ActorUID actorUID, const vec3& pos, const vec3& goal, f32 worldYaw, f32 speed, bool beforeAction = false);
	void SendMonsterLiftToAll(ActorUID actorUID, const vec3& pos);
	void SendNpcLegStartToAll(ActorUID actorUID, const vec3& pos, const vec3& goal, f32 speed);
	void SendNpcLegStopToAll(ActorUID actorUID, const vec3& pos, f32 worldYaw, bool beforeAction = false);
	void SendAiMoveTargetToAll(ActorUID actorUID, const vec3& goal, u32 movePreset);
	void SendRemoteCreateToAll(ActorUID ownerUID, u32 seed, RemoteIdx doc, const vec3& firePos, f32 worldYaw, const vec3& targetPos, ActorUID targetUID, const wchar* fireObject = L"Scene Root", ClientHandle exclude = ClientHandle::INVALID);
	struct RemoteCreate
	{
		ActorUID parentUID = ActorUID::INVALID;
		ActorUID ownerUID = ActorUID::INVALID;
		u32 seed = 0;
		vec3 firePos = vec3(0);
		eastl::array<u16,3> fireRotation = { 0, 0, 0 };
		vec3 targetPos = vec3(0);
		const wchar* fireObject = nullptr;
		u16 fireObjectLen = 0;
		u16 scale = 100;
		f32 lifeTime = 0;
		u32 hitInvalidTarget = 0;
		RemoteIdx doc = RemoteIdx::INVALID;
		ActorUID targetUID = ActorUID::INVALID;
		u8 flags = 0;
		u16 creator = 0;
		eastl::array<u16,2> errorAngle = { 0, 0 };
		u8 chargingLevel = 0;
		u8 gaugeLevel = 0;
		u16 firingCount = 0;
		u8 comboCount = 0;
		u32 parentRemoteID = 0;
		ClientHandle parentSeedOwnerHd = ClientHandle::INVALID;
		ActorUID hitInvalidUID = ActorUID::INVALID;
	};
	void SendRemoteCreate(const RemoteCreate& remote, ClientHandle exclude);
	bool SendRemoteCreateTo(ClientHandle clientHd, const RemoteCreate& remote);
	void SendWeaponCreatorShotToAll(ActorUID ownerUID, u32 seed, RemoteIdx doc, const vec3& firePos, f32 worldYaw, const vec3& targetPos, const wchar* fireObject, u16 creator, u8 chargingLevel, u8 comboCount, u16 firingCount);
	void SendChildRemoteCreateToAll(ActorUID ownerUID, ActorUID parentUID, u32 parentRemoteID, ClientHandle parentSeedOwnerHd, u32 seed, RemoteIdx doc, u16 creator, const vec3& firePos, f32 worldYaw, const vec3& targetPos, ActorUID targetUID, ActorUID hitInvalidUID, const wchar* fireObject, u8 flags, ClientHandle exclude);
	void SendReflectedShotToAll(ActorUID shooterUID, ActorUID reflectorUID, u32 seed, const vec3& firePos, f32 worldYaw, const wchar* fireObject, u32 groupSeed, RemoteIdx doc);
	void RelayRemoteSyncCreateFromRemoteDoc(ClientHandle senderHd, const u8* packetData, i32 packetSize);
	void RelayRemoteSyncTarget(ClientHandle senderHd, const u8* packetData, i32 packetSize);
	void SendRemoteSyncTargetToAll(u32 remoteID, ActorUID targetUID, const vec3& pos, const vec2* moveDir, f32 moveDirZ = 0);
	u16 RelayWeaponShot(ClientHandle senderHd, const u8* packetData, i32 packetSize, const vec3& target, RemoteIdx remoteDoc, bool groundTarget = false);
	void RelayWeaponState(ClientHandle senderHd, ActorUID ownerUID, i32 weaponID, i32 state, u8 chargeLevel, u8 firingCombo);
	void SendWeaponStateTo(ClientHandle clientHd, ActorUID ownerUID, i32 weaponID, i32 state, i32 result);
	void SendScoreToAll(UserID killerID, u8 team, i32 kills, i32 score, i32 teamKills, i32 teamScore, UserID victimID, u8 victimTeam, i32 victimDeaths, i32 victimTeamKills);
	void SendOccupyScoreToAll(UserID userID, u8 team, i32 captures, i32 occupyScore, i32 score, i32 teamHeld, i32 teamScore, i32 otherHeld, bool capture);
	void SendPlayerScoreToAll(UserID userID, u32 type, i32 value);
	void SendAssistScoreToAll(UserID userID, u8 team, i32 assists, i32 score, i32 teamScore);
	struct DamageTaken
	{
		u32 attackerIndex;
		UserID attackerUserID;
		ActorUID objectUID;
		SkillID skill;
		i32 status;
		i32 damage;
		u8 damageType;
		Time at;
	};
	void SendDeadDamageInfo(ClientHandle clientHd, u32 killerIndex, const DamageTaken* records, i32 count, i32 durationMs);
	void SendDeathToAll(ActorUID killerUID, ActorUID targetUID, CreatureIndex killerDocID, u32 remoteID, RemoteIdx remoteDoc, UserID targetUserID, i32 respawnDelayMs);
	void SendPvpAnnouncementToAll(u32 type, u32 data, ActorUID attackerUID, ActorUID victimUID, u32 score = 1);
	void SendPointsToAll(UserID userID, u8 team, i32 points, i32 teamPoints);
	void SendMonsterKillCountsToAll(UserID userID, i32 minionKills, i32 neutralKills);
	void SendNpcDeadAckToAll(ActorUID npcUID, ActorUID killerUID, u32 killerDoc);
	void SendReviveToAll(UserID userID, ActorUID activeUID, ActorUID inactiveUID, const vec3& pos);
	void SendRevive(ClientHandle clientHd, UserID userID, ActorUID activeUID, ActorUID inactiveUID, const vec3& pos);

	void OnPlayerDisconnect(ClientHandle clientHd);

	void PlayerRegisterMasterActor(ClientHandle clientHd, ActorUID masterActorUID, ClassType classType); // TODO: temp, find a better solution

	LocalActorID GetLocalActorID(ClientHandle clientHd, ActorUID actorUID) const; // Can return INVALID
	bool HiddenFor(ClientHandle clientHd, ActorUID actorUID) const;
	ActorUID GetWorldActorUID(ClientHandle clientHd, LocalActorID localActorID) const; // Can return INVALID

	template<typename Packet, u32 CAPACITY>
	inline void SendWriter(ClientHandle clientHd, const PacketWriter<Packet,CAPACITY>& writer) { SendPacket(clientHd, writer); }

private:
	void UpdatePlayersLocalState();
	void FrameDifference();

	void SendActorMasterSpawn(ClientHandle clientHd, const ActorMaster& actor, const Player& parent);
	void SendActorNpcSpawn(ClientHandle clientHd, const ActorNpc& actor);
	void SendActorDynamicSpawn(ClientHandle clientHd, const ActorDynamic& actor);
	void SendActorDestroy(ClientHandle clientHd, ActorUID actorUID, bool despawned = false);

	void SendMasterSkillSlots(ClientHandle clientHd, const ActorMaster& actor, const Player& player);

	void SendInitialFrame(ClientHandle clientHd);
	bool SendRemoteCreateToClient(i32 clientID, const RemoteCreate& remote);

	void PlayerForceLocalActorID(i32 clientID, ActorUID actorUID, LocalActorID localActorID);
	void CreateLocalActorID(i32 clientID, ActorUID actorUID);
	void DeleteLocalActorID(i32 clientID, ActorUID actorUID, bool keepForCaster);

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
