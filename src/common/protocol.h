#pragma once
#include <common/base.h>

enum class LocalActorID: u32
{
	INVALID = 0,

	FIRST_NPC = 5000,

	FIRST_SELF_MASTER = 21000, // First master
	// Every master, in order
	LAST_SELF_MASTER = 21500, // Last master possible. 500 should be enough :)
	FIRST_OTHER_PLAYER = 21501
};

/*
 Leader/Profile Masters have a hardcoded LocalActorID starting at 21000
 We do this because weed to know preemptively which LocalActorID *each* Master owned will get when setting as leader.

 We then link the LocalActorID on connect or when switching leader so it matches up.

 It goes like this:
	- Send profile master list (with hardcoded LocalActorID)
	- Spawn leader actor
	- Force link LocalActorID so it matches the profile one
*/

enum class UserID: u32
{
	INVALID = 0
};

// TODO: move those?
// TODO: populate those everywhere
enum class CreatureIndex: i32
{
	Invalid = 0,
	MasterStriker = 100000001, // taejin?
	Jukebox = 100036777,
	HalloweenJukebox = 100036778
};

enum class SkinIndex: i32
{
	DEFAULT = 0,
};

enum class WeaponIndex: i32
{
	INVALID = 0,
};

enum class ClassType: i32
{
	NONE = -1,
	SIZUKA = 3,
	LUA = 35,
};

enum class SkillID: i32
{
	INVALID = 0,
};

enum class SongID: i32
{
	Default = -1,
	INVALID = 0,
	UpsideDown = 7770015,
	Scml = 7770002,
	Poharan = 7770010,
	Triangle = 7770030,
	Arami = 7770012,
};

enum class SortieUID: u64
{
	INVALID = 0
};

enum class ActionStateID: i32
{
	INVALID = -1,
	ACTION_STATE_TYPE_IDLE = 0,
	NORMAL_STAND_MOVESTATE,
	BATTLE_STAND_MOVESTATE,
	NORMAL_RUN_MOVESTATE,
	BATTLE_RUN_MOVESTATE,
	NORMAL_RUN_FRONT_MOVESTATE,
	BATTLE_RUN_FRONT_MOVESTATE,
	NORMAL_RUN_LEFT_MOVESTATE,
	BATTLE_RUN_LEFT_MOVESTATE,
	NORMAL_RUN_RIGHT_MOVESTATE,
	BATTLE_RUN_RIGHT_MOVESTATE,
	NORMAL_RUN_BACK_MOVESTATE,
	BATTLE_RUN_BACK_MOVESTATE,
	NORMAL_ROTATE_LEFT,
	BATTLE_ROTATE_LEFT,
	NORMAL_ROTATE_RIGHT,
	BATTLE_ROTATE_RIGHT,
	MAXTYPE_MOVESTATE,
	NONE_BEHAVIORSTATE,
	ATTACK_BEHAVIORSTATE,
	ATTACK1_BEHAVIORSTATE,
	ATTACK2_BEHAVIORSTATE,
	ATTACK3_BEHAVIORSTATE,
	ATTACK4_BEHAVIORSTATE,
	ATTACK5_BEHAVIORSTATE,
	ATTACK6_BEHAVIORSTATE,
	ATTACK7_BEHAVIORSTATE,
	ATTACK8_BEHAVIORSTATE,
	ATTACK9_BEHAVIORSTATE,
	ATTACK10_BEHAVIORSTATE,
	HIT_BEHAVIORSTATE,
	SKILL_1_BEHAVIORSTATE,
	SKILL_2_BEHAVIORSTATE,
	SKILL_3_BEHAVIORSTATE,
	SKILL_4_BEHAVIORSTATE,
	SKILL_5_BEHAVIORSTATE,
	SKILL_6_BEHAVIORSTATE,
	SKILL_7_BEHAVIORSTATE,
	SKILL_8_BEHAVIORSTATE,
	SKILL_9_BEHAVIORSTATE,
	SKILL_10_BEHAVIORSTATE,
	SKILL_11_BEHAVIORSTATE,
	SKILL_12_BEHAVIORSTATE,
	SKILL_13_BEHAVIORSTATE,
	SKILL_14_BEHAVIORSTATE,
	SKILL_15_BEHAVIORSTATE,
	SKILL_16_BEHAVIORSTATE,
	SKILL_17_BEHAVIORSTATE,
	SKILL_18_BEHAVIORSTATE,
	SKILL_19_BEHAVIORSTATE,
	SKILL_20_BEHAVIORSTATE,
	SKILL_21_BEHAVIORSTATE,
	SKILL_22_BEHAVIORSTATE,
	SKILL_23_BEHAVIORSTATE,
	SKILL_24_BEHAVIORSTATE,
	SKILL_25_BEHAVIORSTATE,
	SKILL_26_BEHAVIORSTATE,
	SKILL_27_BEHAVIORSTATE,
	SKILL_28_BEHAVIORSTATE,
	SKILL_29_BEHAVIORSTATE,
	SKILL_30_BEHAVIORSTATE,
	SKILL_MAX_BEHAVIORSTATE,
	SKILL_STAGE1_BEHAVIORSTATE,
	SKILL_STAGE2_BEHAVIORSTATE,
	SKILL_STAGE3_BEHAVIORSTATE,
	SKILL_STAGE4_BEHAVIORSTATE,
	SKILL_STAGE5_BEHAVIORSTATE,
	SKILL_STAGE6_BEHAVIORSTATE,
	SKILL_STAGE7_BEHAVIORSTATE,
	SKILL_STAGE8_BEHAVIORSTATE,
	SKILL_STAGE9_BEHAVIORSTATE,
	SKILL_STAGE10_BEHAVIORSTATE,
	SKILL_STAGE11_BEHAVIORSTATE,
	SKILL_STAGE12_BEHAVIORSTATE,
	SKILL_STAGE13_BEHAVIORSTATE,
	SKILL_STAGE14_BEHAVIORSTATE,
	SKILL_STAGE15_BEHAVIORSTATE,
	SKILL_STAGE16_BEHAVIORSTATE,
	SKILL_STAGE17_BEHAVIORSTATE,
	SKILL_STAGE18_BEHAVIORSTATE,
	SKILL_STAGE19_BEHAVIORSTATE,
	SKILL_STAGE20_BEHAVIORSTATE,
	SKILL_STAGE_MAX_BEHAVIORSTATE,
	ATTACKCHARGE_BEHAVIORSTATE,
	DEFEAT_FLY_BEHAVIORSTATE,
	DEFEAT_AIRBORNE_BEHAVIORSTATE,
	DEFEAT_PUSH_BEHAVIORSTATE,
	DEFEAT_KNOCKDOWN_BEHAVIORSTATE,
	DEFEAT_SYNC_BEHAVIORSTATE,
	DEFEAT_GROGGY_BEHAVIORSTATE,
	LIE_BEHAVIORSTATE,
	INTERACTION_CAST_BEHAVIORSTATE,
	INTERACTION_EXECUTE_BEHAVIORSTAT,
	TAG_IN_EXECUTE_BEHAVIORSTATE,
	TAG_OUT_EXECUTE_BEHAVIORSTATE,
	STANCE_IN_BEHAVIORSTATE,
	STANCE_OUT_BEHAVIORSTATE,
	RELOAD_BEHAVIORSTATE,
	WEAPONCHANGE_BEHAVIORSTATE,
	SPAWN_BEHAVIORSTATE,
	RESPAWN_BEHAVIORSTATE,
	DIE_BEHAVIORSTATE,
	UNDEAD_BEHAVIORSTATE,
	SHIRK_BEHAVIORSTATE,
	SHIRK_COMBO_1_BEHAVIORSTATE,
	SHIRK_MOVE_BEHAVIORSTATE,
	TALK_BEHAVIORSTATE,
	BATTLE_STAND_EMOTION_BEHAVIORSTA,
	CRAWLUP_START_BEHAVIORSTATE,
	CRAWLUP_MOVE_BEHAVIORSTATE,
	CRAWLUP_END_BEHAVIORSTATE,
	CRAWLDOWN_START_BEHAVIORSTATE,
	CRAWLDOWN_MOVE_BEHAVIORSTATE,
	CRAWLDOWN_END_BEHAVIORSTATE,
	JUMP_UP_BEHAVIORSTATE,
	JUMP_DOWN_BEHAVIORSTATE,
	EXTRAMOTION_1_BEHAVIORSTATE,
	EXTRAMOTION_2_BEHAVIORSTATE,
	EXTRAMOTION_3_BEHAVIORSTATE,
	EXTRAMOTION_4_BEHAVIORSTATE,
	EXTRAMOTION_5_BEHAVIORSTATE,
	EXTRAMOTION_6_BEHAVIORSTATE,
	EXTRAMOTION_7_BEHAVIORSTATE,
	EXTRAMOTION_8_BEHAVIORSTATE,
	EXTRAMOTION_9_BEHAVIORSTATE,
	EXTRAMOTION_10_BEHAVIORSTATE,
	SEQUENCE_1_BEHAVIORSTATE,
	SEQUENCE_2_BEHAVIORSTATE,
	SEQUENCE_3_BEHAVIORSTATE,
	SEQUENCE_4_BEHAVIORSTATE,
	SEQUENCE_5_BEHAVIORSTATE,
	COMBO_1_BEHAVIORSTATE,
	COMBO_2_BEHAVIORSTATE,
	COMBO_3_BEHAVIORSTATE,
	COMBO_4_BEHAVIORSTATE,
	COMBO_5_BEHAVIORSTATE,
	COMBO_6_BEHAVIORSTATE,
	COMBO_7_BEHAVIORSTATE,
	COMBO_8_BEHAVIORSTATE,
	COMBO_9_BEHAVIORSTATE,
	COMBO_10_BEHAVIORSTATE,
	MELEE_1_BEHAVIORSTATE,
	STYLISH_1_BEHAVIORSTATE,
	CAUGHT_BEHAVIORSTATE,
	DISABLED_BEHAVIORSTATE,
	LIFTED_BEHAVIORSTATE,
	PULLED_BEHAVIORSTATE,
	DRAGGED_BEHAVIORSTATE,
	EVENT_BEHAVIORSTATE,
	EMOTION_BEHAVIORSTATE,
	JUMP_LOOP_MOVESTATE,
	BATTLE_JUMP_LOOP_MOVESTATE,
	JUMP_START_MOVESTATE,
	BATTLE_JUMP_START_MOVESTATE,
	JUMP_END_MOVESTATE,
	BATTLE_JUMP_END_MOVESTATE,
	BREAKFALL_BEHAVIORSTATE,
	MAXTYPE_BEHAVIORSTATE,
	DYNAMIC_OPEN,
	DYNAMIC_CLOSE,
	DYNAMIC_SPAWN,
	DYNAMIC_NORMAL_STAND,
	DYNAMIC_BATTLE_STAND,
	DYNAMIC_RUN_FRONT,
	DYNAMIC_ROTATE_LEFT,
	DYNAMIC_ROTATE_RIGHT,
	DYNAMIC_ATTACK,
	DYNAMIC_HIT,
	DYNAMIC_DIE,
	DYNAMIC_ACTIVE,
	DYNAMIC_EXTRAMOTION_1,
	DYNAMIC_EXTRAMOTION_2,
	DYNAMIC_EXTRAMOTION_3,
	ACTION_STATE_TYPE_MAX,
	UNKNOWN_ACTION_STATE_TYPE,
};

const char* ActionStateString(ActionStateID state);

enum class MapIndex: i32
{
	LOBBY_NORMAL = 160000042,
	PVP_DEATHMATCH = 160000094,
};

enum class StageIndex: i32
{
	CombatArena = 200020102,
};

enum StageType: i32
{
	INVALID = 0,
	CITY = 1, // LOBBY
	PLAY_INSTANCE = 2
};

enum class EntrySystemID: i32
{
	ARENA_3v3 = 210036812
};

struct NetHeader
{
	u16 size;
	u16 netID;
};

ASSERT_SIZE(NetHeader, 4);

#define VEC(TYPE, NAME)\
	u16 NAME##_count;\
	TYPE NAME[1]

// Client packets
namespace Cl {

PUSH_PACKED
struct CQ_FirstHello
{
	enum { NET_ID = 60002 };

	u32 dwProtocolCRC;
	u32 dwErrorCRC;
	u32 version;
	u8 unknown;
};
POP_PACKED

ASSERT_SIZE(CQ_FirstHello, 13);

struct CQ_UserLogin
{
	enum { NET_ID = 60003 };

	u16 nick_len;
	wchar nick[1];
	u16 password_len;
	wchar password[1];
	u16 tpye_len;
	wchar type[1];

	u8 unk[5];
};

struct ConfirmLogin
{
	enum { NET_ID = 60035 };
};

ASSERT_SIZE(ConfirmLogin, 1);

// ?
struct ConfirmGatewayInfo
{
	enum { NET_ID = 60005 };
	i32 var;
};

ASSERT_SIZE(ConfirmGatewayInfo, 4);

PUSH_PACKED
struct EnterQueue
{
	enum { NET_ID = 60007 };

	i32 var1;
	u8 gameIp[4];
	u16 unk;
	u8 pingIp[4];
	u16 port; // ?
	u16 unk2;
	i32 stationID;
	u8 unk3[8];
};
POP_PACKED
ASSERT_SIZE(EnterQueue, 30);

struct CQ_Authenticate
{
	enum { NET_ID = 60008 };

	u16 nickLen;
	wchar nick[1]; // length is nickLen
	i32 var;
};

struct CQ_AuthenticateGameServer
{
	enum { NET_ID = 60009 };

	u16 nickLen;
	wchar nick[1]; // length is nickLen
	i32 var;
	i32 var2;
	u8 b1;
};

struct CN_ReadyToLoadCharacter
{
	enum { NET_ID = 60014 };
};

struct CN_GameMapLoaded
{
	enum { NET_ID = 60016 };
};

struct CN_ReadyToLoadGameMap
{
	enum { NET_ID = 60021 };
};

struct CN_UpdatePosition
{
	enum { NET_ID = 60022 };

	LocalActorID characterID;
	float3 p3nPos;
	float3 p3nDir;
	float3 p3nEye;
	f32 nRotate;
	f32 nSpeed;
	ActionStateID nState;
	i32 nActionIDX;
};
ASSERT_SIZE(CN_UpdatePosition, 56);

PUSH_PACKED
struct CN_GamePlayerSyncActionStateOnly
{
	enum { NET_ID = 60023 };

	LocalActorID characterID;
	ActionStateID state;
	u8 bApply;
	i32 param1;
	i32 param2;
	i32 i4;
	f32 rotate;
	f32 upperRotate;
};
POP_PACKED
ASSERT_SIZE(CN_GamePlayerSyncActionStateOnly, 29);

struct CA_SetGameGvt
{
	enum { NET_ID = 60024 };

	i32 sendTime;
	i32 virtualTime;
	i32 unk;
};
ASSERT_SIZE(CA_SetGameGvt, 12);

struct CQ_GameIsReady
{
	enum { NET_ID = 60032 };
};

struct CQ_LoadingComplete
{
	enum { NET_ID = 60033 };
};

struct CN_MapIsLoaded
{
	enum { NET_ID = 60034 };
};

struct CN_PlayerTagCompleted
{
	enum { NET_ID = 60038 };

	LocalActorID playerID;
	i32 unk;
};
ASSERT_SIZE(CN_PlayerTagCompleted, 8);

struct CQ_PlayerCastSkill
{
	enum { NET_ID = 60040 };

	struct PosStruct
	{
		float3 pos;
		float3 destPos;
		float2 moveDir;
		float3 rotateStruct;
		f32 speed;
		i32 clientTime;
	};

	LocalActorID playerID;
	SkillID skillID;
	float3 p3npos;

	u16 targetList_count;
	LocalActorID targetList[1];

	PosStruct posStruct;
};

struct SetNickname
{
	enum { NET_ID = 60049 };

	u16 nick_len;
	wchar nick[1];
};

struct CheckDupNickname
{
	enum { NET_ID = 60050 };

	u16 nick_len;
	wchar nick[1];
};

struct CQ_GetCharacterInfo
{
	enum { NET_ID = 60051 };

	LocalActorID characterID;
};

struct CQ_SetLeaderCharacter
{
	enum { NET_ID = 60052 };

	LocalActorID characterID;
	SkinIndex skinIndex;
};

struct CQ_GamePlayerTag
{
	enum { NET_ID = 60053 };

	LocalActorID characterID;
};

struct CQ_RequestAreaPopularity
{
	enum { NET_ID = 60073 };

	u32 areaID; // TODO: type ID
};
ASSERT_SIZE(CQ_RequestAreaPopularity, 4);

struct CQ_PartyCreate
{
	enum { NET_ID = 60074 };

	EntrySystemID entrySysID;
	StageType stageType;
};
ASSERT_SIZE(CQ_PartyCreate, 8);

struct CQ_PartyModify
{
	enum { NET_ID = 60075 };

	u16 stageCount;
	i32 stages[1];
	i32 gametype;
	i32 unk1;
	u8 unk2;
};

struct CQ_PartyOptionModify
{
	enum { NET_ID = 60076 };

	u8 option;
	u8 enable;
};
ASSERT_SIZE(CQ_PartyOptionModify, 2);

struct CA_SortieRoomFound
{
	enum { NET_ID = 60089 };

	SortieUID sortieID;
};
ASSERT_SIZE(CA_SortieRoomFound, 8);

struct CN_SortieRoomConfirm
{
	enum { NET_ID = 60090 };

	u8 confirm;
};
ASSERT_SIZE(CN_SortieRoomConfirm, 1);

struct CQ_EnqueueGame
{
	enum { NET_ID = 60097 };
};

struct CQ_PlayerJump
{
	enum { NET_ID = 60113 };

	u8 excludedFieldBits;
	i32 actionID;
	LocalActorID actorID;
	f32 rotate;
	f32 moveDirX;
	f32 moveDirY;
	float3 startPos;
};

struct CN_ChannelChatMessage
{
	enum { NET_ID = 60114 };

	i32 chatType;
	u16 msg_len;
	wchar msg[1];
};

struct CQ_JukeboxQueueSong
{
	enum { NET_ID = 60130 };

	SongID songID;
};
ASSERT_SIZE(CQ_JukeboxQueueSong, 4);

struct CQ_GetGuildProfile
{
	enum { NET_ID = 60145 };
};

struct CQ_GetGuildMemberList
{
	enum { NET_ID = 60146 };
};

struct CQ_GetGuildHistoryList
{
	enum { NET_ID = 60148 };
};

struct CQ_TierRecord
{
	enum { NET_ID = 60151 };
};

struct CQ_GetGuildRankingSeasonList
{
	enum { NET_ID = 60167 };

	u8 rankingType;
};
ASSERT_SIZE(CQ_GetGuildRankingSeasonList, 1);

PUSH_PACKED
struct CN_GameUpdatePosition
{
	enum { NET_ID = 60178 };

	LocalActorID characterID;
	float3 p3nPos;
	float2 p3nDir;
	f32 upperYaw;
	f32 upperPitch;
	f32 bodyYaw;
	f32 nSpeed;
	u8 unk1; // flags maybe?
	ActionStateID actionState;
	f32 localTimeS;
	u8 unk2;
};
POP_PACKED
ASSERT_SIZE(CN_GameUpdatePosition, 50);

PUSH_PACKED
struct CN_GameUpdateRotation
{
	enum { NET_ID = 60179 };

	LocalActorID characterID;
	f32 upperYaw;
	f32 upperPitch;
	f32 bodyYaw;
};
POP_PACKED
ASSERT_SIZE(CN_GameUpdateRotation, 16);

struct CQ_WhisperSend
{
	enum { NET_ID = 60210 };

	u16 destNick_len;
	wchar destNick[1];
	u16 msg_len;
	wchar msg[1];
};

struct CQ_LoadingProgressData
{
	enum { NET_ID = 60235 };
	u8 progress;
};
ASSERT_SIZE(CQ_LoadingProgressData, 1);

struct CQ_RTT_Time
{
	enum { NET_ID = 60245 };

	u32 time;
};

struct CQ_RequestCalendar
{
	enum { NET_ID = 60272 };

	u64 filetimeUTC;
};
ASSERT_SIZE(CQ_RequestCalendar, 8);

} // Cl


// Server packets
namespace Sv {

PUSH_PACKED
struct SA_FirstHello
{
	enum { NET_ID = 62002 };

	u32 dwProtocolCRC;
	u32 dwErrorCRC;
	u8 serverType;
	u8 clientIp[4];
	u16 clientPort;
	u8 tqosWorldId;
};
POP_PACKED
ASSERT_SIZE(SA_FirstHello, 16);

// type 1 is 62004 - related to packet encryption perhaps?
struct SA_UserloginResult
{
	enum { NET_ID = 62003 };

	i32 result;
};

struct SA_UserloginResult2
{
	enum { NET_ID = 62004 };

	i32 result;
};

ASSERT_SIZE(SA_UserloginResult, 4);

struct SA_AuthResult
{
	enum { NET_ID = 62005 };

	i32 result;
};

ASSERT_SIZE(SA_UserloginResult, 4);

struct SN_RegionServicePolicy
{
	enum { NET_ID = 62006 };

	/*
	SN_REGION_SERVICE_POLICY[(bmPolicy:PST_BM_POLICY[(newMasterRestrict:VEC(1)[(1))(userGradePolicy:VEC(1)[(PST_USER_GRADE_POLICY[(userGrade:5)(benefits:VEC(1)[(9))]))(bmServiceMethod:PST_BM_SERVICE_METHOD[(purchaseCCoinMethod:2)(exchangeCCoinForGoldMethod:1)(rewardCCoinMethod:0)(pveRewardSlotOpenBuyChanceMethod:1)])])(regionBanMaster:VEC(3)[(100000041)(100000042)(100000043))(regionNewMaster:VEC(1)[(100000038))(eventBanMaster:VEC(0)[)(floodingRule:PST_USER_CHAT_FLOODING_RULE[(checkPeriodSec:5)(maxTalkCount:10)(blockPeriodSec:120)])(regionBanSkinList:VEC(0)[)(pcCafeSkinList:VEC(0)[)(useFatigueSystem:1)]
	*/

	u16 newMasterRestrict_count;
	u8 newMasterRestrict[1];

	struct UserGradePolicy {
		u8 userGrade;
		u16 benefits_count;
		u8 benefits[1];
	};

	u16 userGradePolicy_count;
	UserGradePolicy userGradePolicy[1];

	u8 purchaseCCoinMethod;
	u8 exchangeCCoinForGoldMethod;
	u8 rewardCCoinMethod;
	u8 pveRewardSlotOpenBuyChanceMethod;

	u16 regionBanMaster_count;
	i32 regionBanMaster[1];

	u16 regionNewMaster_count;
	i32 regionNewMaster[1];

	u16 eventBanMaster_count;
	i32 eventBanMaster[1];

	// flooding rule
	i32 checkPeriodSec;
	i32 maxTalkCount;
	i32 blockPeriodSec;

	struct PST_CharacterSkin {
		i32 classType;
		i32 skinIndex;
	};

	u16 regionBanSkinList_count;
	PST_CharacterSkin regionBanSkinList[1];

	u16 pcCafeSkinList_count;
	PST_CharacterSkin pcCafeSkinList[1];

	u8 useFatigueSystem;
};

struct SN_StationList
{
	enum { NET_ID = 62007 };

	struct PST_Station
	{
		u32 idc;
		u16 stations_count;

		struct PST_Address
		{
			u8 gameServerIp[4];
			u8 pingServerIp[4];
			u16 port;
		};

		PST_Address stations[1]; // variable size
	};

	u16 stationList_count;
	PST_Station stationList[1]; // variable size
};

struct SN_TgchatServerInfo
{
	enum { NET_ID = 62009 };

	u8 data[1]; // variable size

	// TODO: proper ghidra reverse
};

struct SN_DoConnectGameServer
{
	enum { NET_ID = 62010 };

	u16 port;
	u8 ip[4];

	i32 gameID;
	u32 idcHash;

	u16 nickLen;
	wchar nick[1]; // length is nickLen

	i32 instantKey;
};

struct SN_DoConnectChannelServer
{
	enum { NET_ID = 62011 };

	u16 count;
	u8 ip[4];
	u16 port;

	u16 serverNamelen;
	wchar serverName[1]; // length is serverNamelen

	u16 nickLen;
	wchar nick[1]; // length is nickLen

	i32 var1;
	i32 var2;

	// TODO: proper ghidra reverse
};

struct SN_GameCreateActor
{
	enum { NET_ID = 62025 };

	struct BaseStat
	{
		PUSH_PACKED
		struct Stat
		{
			u8 type;
			f32 value;
		};
		POP_PACKED

		u16 maxStats_count;
		Stat maxStats[1];
		u16 curStats_count;
		Stat curStats[1];
	};

	struct ST_ActionChangeTime
	{
		i32 actionState;
		i64 serverTime;
	};

	LocalActorID objectID;
	i32 nType;
	CreatureIndex nIDX;
	i32 dwLocalID;
	float3 p3nPos;
	float3 p3nDir; // rotation
	i32 spawnType;
	i32 actionState;
	i32 ownerID;
	u8 bDirectionToNearPC;
	i32 AiWanderDistOverride;
	i32 tagID;
	i32 faction;
	i32 classType;
	i32 skinIndex;
	i32 seed;
	BaseStat initStat;
	u8 isInSight;
	u8 isDead;
	i64 serverTime;

	u16 meshChangeActionHistory_count;
	ST_ActionChangeTime meshChangeActionHistory[1];
};

struct SN_SpawnPosForMinimap
{
	enum { NET_ID = 62026 };

	i32 objectID;
	float3 p3nPos;
};

struct SN_GameCreateSubActor
{
	enum { NET_ID = 62028 };

	struct BaseStat
	{
		PUSH_PACKED
		struct Stat
		{
			u8 type;
			f32 value;
		};
		POP_PACKED

		VEC(Stat, maxStats);
		VEC(Stat, curStats);
	};

	struct ST_ActionChangeTime
	{
		i32 actionState;
		i64 serverTime;
	};

	LocalActorID objectID;
	LocalActorID mainEntityID;
	i32 nType;
	i32 nIDX;
	i32 dwLocalID;
	float3 p3nPos;
	float3 p3nDir;
	i32 spawnType;
	i32 actionState;
	i32 ownerID;
	i32 tagID;
	i32 faction;
	i32 classType;
	i32 skinIndex;
	i32 seed;
	BaseStat initStat;
	VEC(ST_ActionChangeTime, meshChangeActionHistory);
};

struct SN_GameEnterActor
{
	enum { NET_ID = 62029 };

	struct ST_GraphMoveSnapshot
	{
		u8 hasGraphMove;
		f32 distance;
		f32 totalTimeS;
		f32 curTimeS;
		float3 startPos;
		float3 endPos;
		f32 originDistance;
		u8 hasExtraMove;
		float2 vExtraPointMoveTarget;
		f32 vExtraPointMoveRemainTime;
		float2 vExtraDirMove;
		f32 vExtraDirMoveRemainTime;
	};

	PUSH_PACKED
	struct ST_StatData
	{
		u8 type;
		f32 value;
	};
	POP_PACKED

	struct ST_StatSnapshot
	{
		VEC(ST_StatData, maxStats);
		VEC(ST_StatData, curStats);
		VEC(ST_StatData, addPrivate);
		VEC(ST_StatData, mulPrivate);
	};

	u8 excludedBits;
	ST_GraphMoveSnapshot extraGraphMove; // if not exluded (0x1)
	LocalActorID objectID;
	float3 p3nPos;
	float3 p3nDir;
	float2 p2nMoveDir;
	float2 p2nMoveUpperDir;
	float3 p3nMoveTargetPos;
	u8 isBattleState;
	f32 baseMoveSpeed;
	ActionStateID actionState;
	i32 aiTargetID;
	ST_StatSnapshot statSnapshot;
};

struct SN_GameLeaveActor
{
	enum { NET_ID = 62030 };

	LocalActorID objectID;
};

struct SN_StatusSnapshot
{
	enum { NET_ID = 62031 };

	struct Status
	{
		i32 statusIndex;
		bool bEnabled;
		i32 caster;
		u8 overlapCount;
		u8 customValue;
		i32 durationTimeMs;
		i32 remainTimeMs;
	};

	LocalActorID objectID;
	u16 statusArray_count;
	Status statusArray[1];
};

struct SQ_CityLobbyJoinCity
{
	enum { NET_ID = 62033 };
};

struct SN_CastSkill
{
	enum { NET_ID = 62035 };

	// TODO: fill
};

struct SN_ExecuteSkill
{
	enum { NET_ID = 62036 };

	// TODO: fill
};

struct SA_CastSkill
{
	enum { NET_ID = 62041 };

	LocalActorID characterID;
	i32 ret;
	SkillID skillIndex;
};
ASSERT_SIZE(SA_CastSkill, 12);

struct SA_VersionInfo
{
	enum { NET_ID = 62047 };

	u16 strLen;
	wchar str[1]; // variable size
};

struct SN_PlayerSkillSlot
{
	enum { NET_ID = 62048 };

	struct Property
	{
		i32 skillPropertyIndex;
		i32 level;
	};

	struct Slot
	{
		SkillID skillIndex;
		i32 coolTime;
		u8 unlocked;

		u16 propList_count;
		Property propList[1];

		u8 isUnlocked;
		u8 isActivated;
	};

	LocalActorID characterID;

	u16 slotList_count;
	Slot slotList[1];

	i32 stageSkillIndex1;
	i32 stageSkillIndex2;
	i32 currentSkillSlot1;
	i32 currentSkillSlot2;
	i32 shirkSkillSlot;
};

struct SN_LoadCharacterStart
{
	enum { NET_ID = 62050 };
};

struct SN_ScanEnd
{
	enum { NET_ID = 62051 };
};

struct SN_GamePlayerSyncByInt
{
	enum { NET_ID = 62052 };

	LocalActorID characterID;
	float3 p3nPos;
	float3 p3nDir;
	float3 p3nEye;
	f32 nRotate;
	f32 nSpeed;
	i32 nState;
	i32 nActionIDX;
};
ASSERT_SIZE(SN_GamePlayerSyncByInt, 56);

PUSH_PACKED
struct SN_Money
{
	enum { NET_ID = 62057 };

	i64 nMoney;
	i32 nReason;
};
POP_PACKED
ASSERT_SIZE(SN_Money, 12);

struct SN_DestroyEntity
{
	enum { NET_ID = 62059 };

	LocalActorID characterID;
};

struct SN_SetGameGvt
{
	enum { NET_ID = 62060 };

	u32 sendTime;
	u32 virtualTime;
};

ASSERT_SIZE(SN_SetGameGvt, 8);

struct SN_LobbyStartGame
{
	enum { NET_ID = 62061 };

	StageType stageType;
};
ASSERT_SIZE(SN_LobbyStartGame, 4);

struct SN_LoadClearedStages
{
	enum { NET_ID = 62064 };

	u16 count;
	i32 clearedStageList[1];
};

struct SN_GameFieldReady
{
	enum { NET_ID = 62072 };

	struct User
	{
		i32 userID;
		u16 nick_len;
		wchar nick[1];
		u8 team;
		u8 isBot;
	};

	PUSH_PACKED
	struct Player
	{
		i32 userID;
		CreatureIndex mainCreatureIndex;
		SkinIndex mainSkinIndex;
		SkillID mainSkillIndex1;
		SkillID mainSkillIndex2;
		CreatureIndex subCreatureIndex;
		SkinIndex subSkinIndex1;
		SkillID subSkillIndex1;
		SkillID subSkillIndex2;
		SkillID stageSkillIndex1;
		SkillID stageSkillIndex2;
		i32 supportKitIndex;
		u8 isBot;
	};
	POP_PACKED

	struct Guild
	{
		u8 teamType;
		u16 guildName_len;
		wchar guildName[1];
		u16 guildTag_len;
		wchar guildTag[1];
		i32 guildEmblemIndex;
		i32 guildPvpRankNo;
	};

	i32 gameID;
	i32 gameType;
	i32 areaIndex;
	i32 stageIndex;
	i32 gameDefinitionType;
	u8 initPlayerCount;
	u8 canEscape;
	u8 isTrespass;
	u8 isSpectator;

	u16 ingameUsers_len;
	User IngameUsers[1];
	u16 ingamePlayers_len;
	User ingamePlayers[1];
	u16 ingameGuilds_len;
	User IngameGuilds[1];

	i32 surrenderAbleTime;
};

struct SA_LoadingComplete
{
	enum { NET_ID = 62073 };
};

PUSH_PACKED
struct SA_GameReady
{
	enum { NET_ID = 62075 };

	i32 waitingTimeMs;
	i64 serverTimestamp; // TODO: find out what this represents and how
	i32 readyElapsedMs;
};
POP_PACKED
ASSERT_SIZE(SA_GameReady, 16);

struct SN_GameStart
{
	enum { NET_ID = 62076 };
};

struct SN_GamePlayerEquipWeapon
{
	enum { NET_ID = 62084 };

	i32 characterID;
	i32 weaponDocIndex;
	i32 additionnalOverHeatGauge;
	i32 additionnalOverHeatGaugeRatio;
};

ASSERT_SIZE(SN_GamePlayerEquipWeapon, 16);

struct SN_GamePlayerStock
{
	enum { NET_ID = 62089 };

	LocalActorID playerID;
	u16 name_len;
	wchar name[1];
	ClassType classType;
	i32 displayTitleIDX;
	i32 statTitleIDX;
	u8 badgeType;
	u8 badgeTierLevel;
	u16 guildTag_len;
	wchar guildTag[1];
	u8 vipLevel;
	u8 staffType;
	u8 isSubstituted;
};

struct SN_PlayerStateInTown
{
	enum { NET_ID = 62090 };

	i32 playerID;
	i8 playerStateInTown;

	// TODO: reverse this
	u16 matchingGameModes_count;
	void* matchingGameModes[1];
};

struct SN_CityMapInfo
{
	enum { NET_ID = 62091 };

	MapIndex cityMapID;
};
ASSERT_SIZE(SN_CityMapInfo, 4);

struct SN_SummaryInfoAll
{
	enum { NET_ID = 62097 };

	// TODO: reverse
	u16 finishedSummaryList_count;
};

struct SN_AvailableSummaryRewardCountList
{
	enum { NET_ID = 62098 };

	// TODO: reverse
	u16 rewardCountList_count;
	i32 rewardCountList[1];
};

struct SN_AchieveInfo
{
	enum { NET_ID = 62100 };

	u8 packetNum;
	i32 achievementScore;

	u16 achList_count;
	void* achList[1];

	// TODO: finish reversing
};

struct SN_AchieveLatest
{
	enum { NET_ID = 62101 };

	u16 achList_count;
	void* achList[1];

	// TODO: finish reversing
};

struct SN_AchieveUpdate
{
	enum { NET_ID = 62102 };

	struct ST_AchieveInfo
	{
		i32 index;
		i32 type;
		u8 isCleared;

		u16 achievedList_count;
		i32 achievedList[1];

		i64 progressInt64;
		i64 date;
	};

	i32 achievementScore;
	ST_AchieveInfo achieve;
};

struct SN_AccountInfo
{
	enum { NET_ID = 62106 };

	u16 nick_len;
	wchar_t nick[1];

	i32 inventoryLineCountTab0;
	i32 inventoryLineCountTab1;
	i32 inventoryLineCountTab2;
	i32 displayTitlteIndex;
	i32 statTitleIndex;
	i32 warehouseLineCount;
	i32 tutorialState;
	i32 masterGearDurability;
	u8 badgeType;
};

struct SN_AccountExtraInfo
{
	enum { NET_ID = 62107 };

	PUSH_PACKED
	struct UserGrade
	{
		u8 userGrade;
		u8 activated;
		u64 expireDateTime64;
		u8 level;
		u16 point;
		u16 nextPoint;
	};
	POP_PACKED

	u16 userGradeList_count;
	UserGrade userGradeList[1];
	i32 activityPoint;
	u8 activityRewaredState;
};

struct SN_AllCharacterBaseData
{
	enum { NET_ID = 62109 };

	/*
	SN_ALL_CHARACTER_BASEDATA[(charBaseData:VEC(1)[(PST_CHARACTER_BASE_DATA[(docIndex:100000001)(baseStats:VEC(22)[(ST_STAT_DATA[(type:0)(value:2400)])(ST_STAT_DATA[(type:2)(value:200)])(ST_STAT_DATA[(type:37)(value:120)])(ST_STAT_DATA[(type:5)(value:5)])(ST_STAT_DATA[(type:42)(value:0.6)])(ST_STAT_DATA[(type:7)(value:92.3077)])(ST_STAT_DATA[(type:9)(value:3)])(ST_STAT_DATA[(type:10)(value:150)])(ST_STAT_DATA[(type:18)(value:100)])(ST_STAT_DATA[(type:13)(value:100)])(ST_STAT_DATA[(type:14)(value:100)])(ST_STAT_DATA[(type:15)(value:100)])(ST_STAT_DATA[(type:52)(value:100)])(ST_STAT_DATA[(type:16)(value:1)])(ST_STAT_DATA[(type:29)(value:20)])(ST_STAT_DATA[(type:23)(value:9)])(ST_STAT_DATA[(type:31)(value:14)])(ST_STAT_DATA[(type:22)(value:2)])(ST_STAT_DATA[(type:54)(value:15)])(ST_STAT_DATA[(type:63)(value:3)])(ST_STAT_DATA[(type:64)(value:150)])(ST_STAT_DATA[(type:55)(value:15)]))(skillData:VEC(7)[(ST_SKILL_RATIO[(skillIndex:180010020)(baseDamage:355)(attMultiplier:0.42)(healMultiplier:0)(shieldMultiplier:0)(dotMultiplier:0)])(ST_SKILL_RATIO[(skillIndex:180010040)(baseDamage:995)(attMultiplier:0.81)(healMultiplier:0)(shieldMultiplier:0)(dotMultiplier:0.1)])(ST_SKILL_RATIO[(skillIndex:180010010)(baseDamage:550)(attMultiplier:0.56)(healMultiplier:0)(shieldMultiplier:0)(dotMultiplier:0)])(ST_SKILL_RATIO[(skillIndex:180010030)(baseDamage:0)(attMultiplier:0)(healMultiplier:0)(shieldMultiplier:0)(dotMultiplier:0)])(ST_SKILL_RATIO[(skillIndex:180010050)(baseDamage:680)(attMultiplier:0.37)(healMultiplier:0)(shieldMultiplier:0)(dotMultiplier:0)])(ST_SKILL_RATIO[(skillIndex:180010000)(baseDamage:0)(attMultiplier:1)(healMultiplier:0)(shieldMultiplier:0)(dotMultiplier:0)])(ST_SKILL_RATIO[(skillIndex:180010002)(baseDamage:0)(attMultiplier:1)(healMultiplier:0)(shieldMultiplier:0)(dotMultiplier:0)]))]))(cur:1)(max:4)]
	*/

	struct Character
	{
		PUSH_PACKED
		struct Stat {
			u8 type;
			f32 value;
		};
		POP_PACKED

		struct SkillRatio {
			i32 skillIndex;
			f32 baseDamage;
			f32 attMultiplier;
			f32 healMultiplier;
			f32 shieldMultiplier;
			f32 dotMultiplier;
		};

		i32 docIndex;
		u16 baseStats_count;
		Stat baseStats[1];
		u16 skillData_count;
		SkillRatio skillData[1];
	};

	u16 charaList_count;
	Character charaList[1];

	i32 cur; // current packet ID
	i32 max; // packet count total

	// the first packet is for example: cur:1 max:4
	// and the last is cur:4 max:4
};

struct SN_GamePlayerTag
{
	enum { NET_ID = 62112 };

	i32 result;
	LocalActorID mainID;
	LocalActorID subID;
	LocalActorID attackerID;
};
ASSERT_SIZE(SN_GamePlayerTag, 16);

struct SA_GetCharacterInfo
{
	enum { NET_ID = 62113 };

	LocalActorID characterID;
	CreatureIndex docIndex;
	ClassType classType;
	i32 hp;
	i32 maxHp;
};
ASSERT_SIZE(SA_GetCharacterInfo, 20);

struct SA_CheckDupNickname
{
	enum { NET_ID = 62121 };

	u16 statusCode;
	u8 unk;
	u16 nick_len;
	wchar nick[1];
};

struct SA_SetLeader
{
	enum { NET_ID = 62122 };

	i32 result;
	LocalActorID leaderID;
	SkinIndex skinIndex;
};
ASSERT_SIZE(SA_SetLeader, 12);

struct SN_LeaderCharacter
{
	enum { NET_ID = 62123 };

	LocalActorID leaderID; // characterIndex
	SkinIndex skinIndex;
};
ASSERT_SIZE(SN_LeaderCharacter, 8);


struct SN_ProfileCharacters
{
	enum { NET_ID = 62124 };

	PUSH_PACKED
	struct Character
	{
		LocalActorID characterID;
		CreatureIndex creatureIndex;
		SkillID skillShot1;
		SkillID skillShot2;
		ClassType classType;
		f32 x;
		f32 y;
		f32 z;
		i32 characterType;
		SkinIndex skinIndex;
		WeaponIndex weaponIndex;
		u8 masterGearNo;
	};
	POP_PACKED

	u16 charaList_count;
	Character chara[1];
};

PUSH_PACKED
struct PST_Property
{
	u8 type;
	i32 typeDetail;
	u8 valueType;
	f32 value;
	u8 fixed;
};
POP_PACKED

struct PST_ProfileItem
{
	i32 itemID;
	u8 invenType;
	i32 slot;
	i32 itemIndex;
	i32 count;
	i32 propertyGroupIndex;
	u8 isLifeTimeAbsolute;
	i64 lifeEndTimeUTC;

	u16 properties_count;
	PST_Property properties[1];
};

struct SN_ProfileItems
{
	enum { NET_ID = 62125 };

	u8 packetNum;
	u16 items_count;
	PST_ProfileItem items[1];
};

struct SN_ProfileWeapons
{
	enum { NET_ID = 62126 };

	PUSH_PACKED
	struct Weapon
	{
		LocalActorID characterID;
		i32 weaponType;
		WeaponIndex weaponIndex;
		i32 grade;
		u8 isUnlocked;
		u8 isActivated;
	};
	POP_PACKED

	u16 weaponList_count;
	Weapon weaponList[1];
};

struct SN_ProfileSkills
{
	enum { NET_ID = 62127 };

	struct Property
	{
		i32 skillIndex;
		i32 level;
	};

	struct Skill
	{
		LocalActorID characterID;
		i32 skillIndex;
		u8 isUnlocked;
		u8 isActivated;

		u16 properties_count;
		Property properties[1];
	};

	u8 packetNum;
	u16 skills_count;
	Skill skills[1];
};

struct SN_ProfileTitles
{
	enum { NET_ID = 62128 };

	u16 titles_count;
	i32 titles[1];
};

struct SN_ProfileMasterGears
{
	enum { NET_ID = 62129 };

	struct Slot
	{
		i32 gearType;
		i32 gearItemID;
	};

	struct Gear
	{
		u8 masterGearNo;
		u16 name_len;
		wchar name[1];

		u16 slots_count;
		Slot slots[1];
	};

	u16 masterGears_count;
	Gear masterGears;
};

struct SA_EnqueueGame
{
	enum { NET_ID = 62158 };

	i32 retval;
};
ASSERT_SIZE(SA_EnqueueGame, 4);

struct SA_AreaPopularity
{
	enum { NET_ID = 62173 };

	i32 errCode;
};
ASSERT_SIZE(SA_AreaPopularity, 4);

struct SN_AreaPopularity
{
	enum { NET_ID = 62174 };

	struct Popularity
	{
		i32 stageIndex;
		i32 gameType;
		i32 popularityLevel;
	};
	ASSERT_SIZE(Popularity, 12);

	u32 areaID;
	u16 popularityCount;
	Popularity popularities[1];
};

struct SA_PartyCreate
{
	enum { NET_ID = 62175 };

	i32 retval;
	UserID ownerUserID;
	StageType stageType;
};
ASSERT_SIZE(SA_PartyCreate, 12);

struct SA_PartyModify
{
	enum { NET_ID = 62176 };

	i32 retval;
};
ASSERT_SIZE(SA_PartyModify, 4);

PUSH_PACKED
struct SA_PartyOptionModify
{
	enum { NET_ID = 62177 };

	i32 retval;
	u8 option;
	u8 enable;
};
POP_PACKED
ASSERT_SIZE(SA_PartyOptionModify, 6);

PUSH_PACKED
struct SN_EnqueueMatchingQueue
{
	enum { NET_ID = 62201 };

	StageIndex stageIndex;
	i32 currentMatchingTimeMs;
	i32 avgMatchingTimeMs;
	u8 disableMatchExpansion;
	u8 isMatchingExpanded;
};
POP_PACKED
ASSERT_SIZE(SN_EnqueueMatchingQueue, 14);

struct SQ_MatchingPartyFound
{
	enum { NET_ID = 62204 };

	struct Player
	{
		UserID userID;
		u16 nickname_len;
		wchar nickname[1];
		u8 isBot;
		i32 tier;
		i32 tierGroupRanking;
		i32 tierSeriesFlag;
		i32 pvpRate;
	};

	i64 sortieID;
	StageIndex stageIndex;
	i32 gameType;
	i32 gameDefinitionType;
	i32 stageRule;

	u16 allies_count;
	Player allies[1];

	u16 enemies_count;
	Player enemies[1];

	u16 spectator_count;
	Player spectator[1];

	i32 timeToWaitInSec;
	u8 elementMain;
	u8 elementSub;
};

struct SN_MatchingPartyGathered
{
	enum { NET_ID = 62205 };

	u8 allConfirmed;
};
ASSERT_SIZE(SN_MatchingPartyGathered, 1);

struct SN_UpdateGameOwner
{
	enum { NET_ID = 62224 };

	i32 userID;
};
ASSERT_SIZE(SN_UpdateGameOwner, 4);

struct SN_SummaryInfoLatest
{
	enum { NET_ID = 62227 };

	PUSH_PACKED
	struct Info
	{
		i32 summaryIndex;
		i32 stageIndex;
		i32 summaryType;
		i32 summaryData;
		u8 rewardReceived;
		i64 completedDate;
	};
	POP_PACKED

	u16 infoList_count;
	Info infoList[1];
};

struct SN_NotifyPcDetailInfos
{
	enum { NET_ID = 62229 };

	struct ST_PcInfo
	{
		LocalActorID characterID;
		CreatureIndex docID;
		ClassType classType;
		i32 hp;
		i32 maxHp;
	};

	struct ST_PcDetailInfo
	{
		i32 userID;
		ST_PcInfo mainPc;
		ST_PcInfo subPc;
		i32 remainTagCooltimeMS;
		u8 canCastSkillSlotUG;
	};

	u16 pcList_count;
	ST_PcDetailInfo pcList[1];
};

struct SA_ResultSpAction
{
	enum { NET_ID = 62238 };

	u8 excludedFireldBits;
	i32 actionID;
	LocalActorID objectID;
	f32 rotate;
	f32 moveDirX;
	f32 moveDirY;
	i32 errorType;
	float3 startPos;
};

struct SN_ChatChannelMessage
{
	enum { NET_ID = 62242 };

	i32 chatType;
	u16 senderNickname_len;
	wchar senderNickname[1];
	u8 senderStaffType;
	u16 chatMsg_len;
	wchar chatMsg[1];
};

struct SN_FriendList
{
	enum { NET_ID = 62257 };

	struct Friend
	{
		u16 name_len;
		wchar name[1];
		i32 leaderCreatureIndex;
		u8 state;
		u16 level;
		u8 isFavorite;
		u8 isComrade;
		u8 isOnLeavePenalty;
		i64 lastLogoutDate;
	};

	u16 friendList_count;
	Friend friendList;
};

struct SN_FriendRequestList
{
	enum { NET_ID = 62258 };

	struct FriendRequest
	{
		u16 name_len;
		wchar name[1];
	};

	u16 friendRequestList_count;
	FriendRequest friendRequestList[1];
};

struct SN_MutualFriendList
{
	enum { NET_ID = 62259 };

	struct Candidate
	{
		u16 name_len;
		wchar name[1];
		i32 leaderCreatureIndex;
		u8 mutualCount;
	};

	u16 candidates_count;
	Candidate candidates[1];
};

struct SN_BlockList
{
	enum { NET_ID = 62261 };

	struct Block
	{
		u16 name_len;
		wchar name[1];
	};

	u16 blocks_count;
	Block blocks[1];
};

struct SN_NotifyAasRestricted
{
	enum { NET_ID = 62276 };

	u8 isRestrictedByAAS;
};
ASSERT_SIZE(SN_NotifyAasRestricted, 1);

struct SN_Exp
{
	enum { NET_ID = 62278 };

	i32 leaderCsn;
	u16 nickname_len;
	wchar nickname[1];
	u8 isLevelUp;
	u16 level;
	i32 currentLevelExp;
};

struct SN_JukeboxEnqueuedList
{
	enum { NET_ID = 62282 };

	struct Track
	{
		SongID trackID;
		u16 nickname_len;
		wchar nickname[1];
	};

	u16 trackList_count;
	Track trackList[1];
};

struct SN_JukeboxPlay
{
	enum { NET_ID = 62283 };

	struct Track
	{
		SongID trackID;
		u16 nickname_len;
		wchar nickname[1];
	};

	i32 result;
	Track track;
	u16 playPositionSec;
};

struct SN_JukeboxHotTrackList
{
	enum { NET_ID = 62284 };

	u16 trackList_count;
	i32 trackList[1];
};

struct SN_TownHudStatistics
{
	enum { NET_ID = 62285 };

	u8 gameModeType;
	u8 gameType;

	u16 argList_count;
	i32 argList[1];
};

struct SA_GetGuildProfile
{
	enum { NET_ID = 62299 };

	PUSH_PACKED
	struct ST_GuildInterest
	{
		u8 likePveStage;
		u8 likeDefence;
		u8 likePvpNormal;
		u8 likePvpOccupy;
		u8 likePvpGot;
		u8 likePvpRank;
		u8 likeOlympic;
	};
	POP_PACKED

	PUSH_PACKED
	struct ST_GuildPvpRecord
	{
		i32 rp;
		u16 win;
		u16 draw;
		u16 lose;
	};
	POP_PACKED

	PUSH_PACKED
	struct ST_GuildMemberRights
	{
		u8 hasInviteRight;
		u8 hasExpelRight;
		u8 hasMembershipChgRight;
		u8 hasClassAssignRight;
		u8 hasNoticeChgRight;
		u8 hasIntroChgRight;
		u8 hasInterestChgRight;
		u8 hasFundManageRight;
		u8 hasJoinTypeRight;
		u8 hasEmblemRight;
	};
	POP_PACKED

	struct ST_GuildMembership
	{
		i32 id;
		u8 type;
		u8 iconIndex;

		u16 name_len;
		wchar name[1];

		ST_GuildMemberRights rights;
	};

	PUSH_PACKED
	struct ST_GuildSkill
	{
		u8 type;
		u8 level;
		i64 expiryDate;
		u16 extensionCount;
	};
	POP_PACKED

	i32 result;
	u16 guildName_len;
	wchar guildName[1];
	u16 guildTag_len;
	wchar guildTag[1];
	i32 emblemIndex;
	u8 guildLvl;
	u8 memberMax;
	u16 ownerNickname_len;
	wchar ownerNickname[1];
	i64 createdDate;
	i64 dissolutionDate;
	u8 joinType;
	ST_GuildInterest guildInterest;
	u16 guildIntro_len;
	wchar guildIntro[1];
	u16 guildNotice_len;
	wchar guildNotice[1];
	i32 guildPoint;
	i32 guildFund;
	ST_GuildPvpRecord guildPvpRecord;
	i32 guildRankNo;

	u16 guildMemberClassList_count;
	ST_GuildMembership guildMemberClassList[1];
	u16 guildSkills_count;
	ST_GuildSkill guildSkills[1];

	i32 curDailyStageGuildPoint;
	i32 maxDailyStageGuildPoint;
	i32 curDailyArenaGuildPoint;
	i32 maxDailyArenaGuildPoint;
	u8 todayRollCallCount;
};

struct SA_GetGuildMemberList
{
	enum { NET_ID = 62300 };

	struct ST_GuildMemberProfile
	{
		u16 nickname_len;
		wchar nickname[1];

		i32 membershipID;
		u16 lvl;
		u16 leaderClassType;
		u16 masterCount;
		i32 achievementScore;
		u8 topPvpTierGrade;
		u16 topPvpTierPoint;
		i32 contributedGuildPoint;
		i32 contributedGuildFund;
		u16 guildPvpWin;
		u16 guildPvpPlay;
		i64 lastLogoutDate;
	};

	i32 result;
	u16 guildMemberProfileList_count;
	ST_GuildMemberProfile guildMemberProfileList[1];
};

struct SA_GetGuildHistoryList
{
	enum { NET_ID = 62302 };

	struct ST_GuildHistory
	{
		u8 historyType;
		u8 eventType;
		i64 eventDate;
		u16 strParam1_len;
		wchar strParam1[1];
		u16 strParam2_len;
		wchar strParam2[1];
		i32 lParam1;
		i32 lParam2;
	};

	i32 result;
	u16 guildHistories_count;
	ST_GuildHistory guildHistories[1];
};

struct SA_GetGuildRankingSeasonList
{
	enum { NET_ID = 62322 };

	PUSH_PACKED
	struct ST_RankingSeason
	{
		u16 seasonNo;
		i64 beginDate;
		i64 endDate;
	};
	POP_PACKED

	i32 result;
	u8 rankingType;

	u16 rankingSeasonList_count;
	ST_RankingSeason rankingSeasonList[1];
};

struct SN_MyGuild
{
	enum { NET_ID = 62330 };

	u16 guildTag_len;
	wchar_t guildTag[1];
	i64 dissolutionDate;
	u8 isFirstTodayRollCall;
};

struct SN_GuildMemberStatus
{
	enum { NET_ID = 62338 };

	struct Member
	{
		u16 nickname_len;
		wchar nickname[1];
		u8 onlineStatus;
	};

	u16 guildMemberStatusList_count;
	Member guildMemberStatusList[1];
};

struct SN_GuildChannelEnter
{
	enum { NET_ID = 62358 };

	u16 guildName_len;
	wchar_t guildName[1];
	u16 nick_len;
	wchar_t nick[1];
	u8 onlineStatus;
};

PUSH_PACKED
struct SN_PlayerSyncMove
{
	enum { NET_ID = 62360 };

	LocalActorID characterID;
	float3 destPos;
	float2 moveDir;
	float2 upperDir;
	f32 nRotate;
	f32 nSpeed;
	u8 flags;
	ActionStateID state;
};
POP_PACKED
ASSERT_SIZE(SN_PlayerSyncMove, 45);

PUSH_PACKED
struct SN_PlayerSyncTurn
{
	enum { NET_ID = 62361 };

	LocalActorID characterID;
	float2 upperDir;
	f32 nRotate;
};
POP_PACKED
ASSERT_SIZE(SN_PlayerSyncTurn, 16);

PUSH_PACKED
struct SN_PlayerSyncActionStateOnly
{
	enum { NET_ID = 62364 };

	LocalActorID characterID;
	ActionStateID state;
	i32 param1;
	i32 param2;
	f32 rotate;
	f32 upperRotate;

	struct ST_GraphMoveData
	{
		u8 bApply;
		float3 startPos;
		float3 endPos;
		f32 durationTimeS;
		f32 originDistance;
	};

	ST_GraphMoveData graphMove;
};
POP_PACKED
ASSERT_SIZE(SN_PlayerSyncActionStateOnly, 57);

PUSH_PACKED
struct SN_WeaponState
{
	enum { NET_ID = 62365 };

	LocalActorID ownerID;
	i32 weaponID;
	i32 state;
	u8 chargeLevel;
	u8 firingCombo;
	i32 result;
};
POP_PACKED
ASSERT_SIZE(SN_WeaponState, 18);

struct SN_ProfileCharacterSkinList
{
	enum { NET_ID = 62390 };

	struct Skin
	{
		ClassType classType;
		SkinIndex skinIndex;
		i32 bufCount;
		i64 expireDateTime;
	};

	u16 skins_count;
	Skin skins[1];
};

struct SN_NotifyUserLifeInfo
{
	enum { NET_ID = 62400 };

	i32 usn;
	i32 lifeCount;
	i32 maxLifeCount;
	i32 remainLifeCount;
};
ASSERT_SIZE(SN_NotifyUserLifeInfo, 16);

struct SN_WarehouseItems
{
	enum { NET_ID = 62404 };

	u16 items_count;
	PST_ProfileItem items[1];
};

struct SA_WhisperSend
{
	enum { NET_ID = 62406 };

	i32 result;
	u16 destNick_len;
	wchar destNick[1];
	u16 msg_len;
	wchar msg[1];
};

struct SN_WhisperReceive
{
	enum { NET_ID = 62407 };

	u16 senderNick_len;
	wchar senderNick[1];
	u8 staffType;
	u16 msg_len;
	wchar msg[1];
};

struct SN_MailUnreadNotice
{
	enum { NET_ID = 62425 };

	u16 unreadInboxMailCount;
	u16 unreadArchivedMailCount;
	u16 unreadShopMailCount;
	u16 inboxMailCount;
	u16 archivedMailCount;
	u16 shopMailCount;
	u16 newAttachmentsPending_count;
	i32 newAttachmentsPending[1];
};

struct SN_UpdateEntrySystem
{
	enum { NET_ID = 62426 };

	PUSH_PACKED
	struct Area
	{
		u8 areaKey;
		i32 areaIndex;
	};
	POP_PACKED

	struct StageIndex
	{
		u8 areaKey;
		i32 stageIndex;
		u16 gametypeCount;
		u8 gametypes[1];
	};

	struct Entry
	{
		u32 entrySystemIndex;
		u16 areaListCount;
		Area areaList[1];
		u16 stageListCount;
		StageIndex stageList[1];
	};

	u16 entrySystemListCount;
	Entry entrySystemList[1];
};

struct SQ_Heartbeat
{
	enum { NET_ID = 62446 };
};

struct SN_RunClientLevelEvent
{
	enum { NET_ID = 62448 };

	i32 eventID;
	i32 caller;
	i64 serverTime;
};
ASSERT_SIZE(SN_RunClientLevelEvent, 16);

PUSH_PACKED
struct SN_RunClientLevelEventSeq
{
	enum { NET_ID = 62449 };

	i32 needCompleteTriggerAckID;
	i32 rootEventID;
	i32 caller;
	i64 serverTime;
};
POP_PACKED
ASSERT_SIZE(SN_RunClientLevelEventSeq, 20);

struct SN_MasterRotationInfo
{
	enum { NET_ID = 62455 };

	i32 refreshCount;

	u16 freeRotation_count;
	CreatureIndex freeRotation[1];
	u16 pccafeRotation_count;
	CreatureIndex pccafeRotation[1];
	u16 vipRotation_count;
	CreatureIndex vipRotation[1];
};

struct SN_SortieMasterPickPhaseStart
{
	enum { NET_ID = 62465 };

	u8 isRandomPick;
	u16 alliesSlotInfos_count;
};

struct SN_SortieMasterPickPhaseStepStart
{
	enum { NET_ID = 62467 };

	i32 timeSec;
	u16 alliesTeamUserIds_count;
	UserID alliesTeamUserIds[1];
	u16 enemiesTeamUserIds_count;
	UserID enemiesTeamUserIds[1];
};

struct SA_TierRecord
{
	enum { NET_ID = 62469 };

	PUSH_PACKED
	struct PST_TierStageRecord
	{
		u8 pveStageType;
		u8 tierType;
		u16 tierPoint;
		u16 sectorId;
		u8 sectorRanking;
		i32 seasonalWin;
		i32 seasonalDraw;
		i32 seasonalLose;
		i32 seasonalLeave;
	};
	POP_PACKED

	u8 seasonId;
	i32 allTierWin;
	i32 allTierDraw;
	i32 allTierLose;
	i32 allTierLeave;
	u16 stageRecordList_count;
	PST_TierStageRecord stageRecordList[1];
};

struct SN_Unknown_62472
{
	enum { NET_ID = 62472 };

	u8 unk;
};

PUSH_PACKED
struct SN_NotifyIsInSafeZone
{
	enum { NET_ID = 62473 };

	i32 userID;
	u8 inSafeZone;
};
POP_PACKED
ASSERT_SIZE(SN_NotifyIsInSafeZone, 5);

struct SN_NotifyIngameSkillPoint
{
	enum { NET_ID = 62474 };

	i32 userID;
	i32 skillPoint;
};
ASSERT_SIZE(SN_NotifyIngameSkillPoint, 8);

PUSH_PACKED
struct SN_NotifyTimestamp
{
	enum { NET_ID = 62481 };

	i64 serverTimestamp;
	i32 curCount;
	i32 maxCount;
};
POP_PACKED
ASSERT_SIZE(SN_NotifyTimestamp, 16);

PUSH_PACKED
struct SA_RTT_Time
{
	enum { NET_ID = 62482 };

	u32 clientTimestamp;
	i64 serverTimestamp;
};
POP_PACKED
ASSERT_SIZE(SA_RTT_Time, 12);

struct SN_PveComradeInfo
{
	enum { NET_ID = 62485 };

	i32 availableComradeCount;
	i32 maxComradeCount;
};
ASSERT_SIZE(SN_PveComradeInfo, 8);

struct SN_ClientSettings
{
	enum { NET_ID = 62500 };

	u8 settingType;
	u16 data_len;
	u8 data[1]; // xml compressed with zlib
};

// maybe?
PUSH_PACKED
struct QueueStatus
{
	enum { NET_ID = 62501 };

	i32 var1;
	u8 unk[5];
	i32 var2;
};
POP_PACKED
ASSERT_SIZE(QueueStatus, 13);

struct SN_AccountEquipmentList
{
	enum { NET_ID = 62525 };

	i32 supportKitDocIndex;
};
ASSERT_SIZE(SN_AccountEquipmentList, 4);

struct SA_CalendarDetail
{
	enum { NET_ID = 62545 };

	PUSH_PACKED
	struct Event
	{
		u8 dataType;
		i32 index;
		u64 startDateTime; // filetime UTC
		u64 endDateTime;
	};
	POP_PACKED

	u64 todayUTCDateTime;
	u16 eventCount;
	Event events[1];
};

struct SN_InitScoreBoard
{
	enum { NET_ID = 62575 };

	struct PST_ScoreBoardUserInfo
	{
		i32 usn;
		u16 nickname_len;
		wchar nick[1];
		i32 teamType;
		CreatureIndex mainCreatureIndex;
		CreatureIndex subCreatureIndex;
	};

	u16 userInfos_count;
	PST_ScoreBoardUserInfo userInfos[1];
};

struct SN_InitIngameModeInfo
{
	enum { NET_ID = 62576 };

	i32 transformationVotingPlayerCoolTimeByVotingFail;
	i32 transformationVotingTeamCoolTimeByTransformationEnd;
	i32 playerCoolTimeByTransformationEnd;
	i32 currentTransformationVotingPlayerCoolTimeByVotingFail;
	i32 currentTransformationVotingTeamCoolTimeByTransformationEnd;
	i32 currentPlayerCoolTimeByTransformationEnd;
	i32 chPropertyResetCoolTime;
	u8 transformationPieceCount;
	u16 titanDocIndexes_count;
	i32 titanDocIndexes[1];
	u8 nextTitanIndex;
	u16 listExceptionStat_count;
	i32 listExceptionStat[1]; // TODO: not actually int
};

PUSH_PACKED
struct SN_UpdateMasterGroupingEffect
{
	enum { NET_ID = 62601 };

	LocalActorID instanceID;
	i32 index;
	ClassType classType;
	u8 activate;
};
POP_PACKED
ASSERT_SIZE(SN_UpdateMasterGroupingEffect, 13);

} // Sv

#undef VEC
