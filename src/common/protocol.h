#pragma once
#include <common/base.h>
#include <EASTL/array.h>

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
	Invalid = -1,
	MasterStriker = 100000001, // taejin
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
	STRIKER = 1,
	ARTILLERY,
	ASSASSIN,
	ELECTRO,
	DEFENDER,
	SNIPER,
	DEATHKNIGHT,
	DESTROYER,
	MECHANIC,
	SOULMASTER,
	DOUBLEGUN,
	JINSEOYEON,
	KROMEDE,
	RODMASTER,
	PHOTOG,
	ICEQUEEN,
	MAGICGIRL,
	POHWARAN,
	BOOMERANG,
	SLIME,
	MONDOZAX,
	LILU,
	EFREET,
	SHADOWHUNTER,
	RYTLOCK,
	CATTHECAT,
	RNB,
	BATTER,
	ANDROA,
	ANDROB,
	ATTACKER,
	BOXER,
	SHUGOTRADER,
	NAGA,
	LAUNCHER,
	STATESMAN,
	YURI,
	ESPER,
	PRIEST,
	GHOSTWIDOW,
	SHUTTLE,
	JAVELIN,
	GATLING,
	GUARDIAN,
	RESERVED_START,
	ALEX,
	TOPAZ,
	TITAN,
	TITAN_SIEGE,
	MAX,
};

ClassType ClassTypeFromString(const char* str);

// values are from decompile and work ingame
enum class EChatType : i32
{
	INVALID = 0,
	NOTICE,
	NOTICE_CHAT,
	NOTICE_BANNER,
	SYSTEM_GM,
	SYSTEM1,
	SYSTEM2,
	SYSTEM3,
	MOTD,
	NPC,
	NORMAL,
	RECV_WHISPER,
	SEND_WHISPER,
	ALL_PLAYER,
	TEAM,
	GUILD,
	PARTY,
	CHANNEL = 20
};

// values from decompile
enum class GamePingType : i32
{
	GAME_PING_TYPE_INVALID = -1,
	GAME_PING_TYPE_NORMAL = 0,
	GAME_PING_TYPE_WARNING,
	GAME_PING_TYPE_TOGO,
	GAME_PING_TYPE_DEFENCE,
	GAME_PING_TYPE_SUPPORT,
	GAME_PING_TYPE_ATTACK,
	GAME_PING_TYPE_DESTROY,
	GAME_PING_TYPE_OCCUPY,
	GAME_PING_TYPE_BACK,
	GAME_PING_TYPE_MOBGEN
};

enum class SkillID: i32
{
	INVALID = -1,
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

const char* ActionStateToString(ActionStateID state);
ActionStateID ActionStateFromString(const char* str);

enum class MapIndex: i32
{
	LOBBY_NORMAL = 160000042,
	PVP_DEATHMATCH = 160000094,
};

enum class StageIndex: i32
{
	CombatArena = 200020102,
};

enum class StageType: i32
{
	INVALID = 0,
	CITY = 1, // LOBBY
	PVP_GAME = 2
};

enum class StageRule: i32
{
	INVALID = -1,
	Unfair = 0,
	Fair = 1,
	Max = 2
};

enum class GameType: i32
{
	INVALID = 0,
	PvP_Normal = 1, // not PvP?
	PvP_NormalFFA = 2,
	PVP_AI = 3,
	PVP_Rank = 4,
	PVP_Custom = 5,
	PVP_Tutorial = 6,
	PVP_GuildPvp = 7,
	PVP_FakeAi = 8,
};

enum class GameDefinition: i32
{
	INVALID = 0,
	System = 1,
	User = 2,
};

enum class EntrySystemID: i32
{
	ARENA_3v3 = 210036812
};

// _Type in EntityComData
// got values from decompile
enum class EntityType : i32
{
	INVALID = -1, // UNKNOWN_ENTITY_TYPE default return
	TERRAIN = 0, // haven't found this one in EntityComData was in an attribute CaseValue1 in tag <StatePlay>
	CREATURE,
	ITEM,
	DYNAMIC,
	REMOTE,
	MARKER,
	EFFECT,
	SOUND, //not found in chinese data files check if this is changed to SFX in chinese release
	DEFAULT,
	MAX,
	SFX //found in chinese data files check if exist in West files
};

enum class Faction: i32
{
	INVALID = -1,
	RED = 0,
	BLUE = 1,
	DYNAMIC = 2,
	_COUNT = 2
};

enum class RemoteIdx: i32
{
	INVALID = -1
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
	u32 instantKey;
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
		float3 rot;
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

struct CQ_RoomEquipWeapon
{
	enum { NET_ID = 60059 };

	// TODO: fill this
};

struct CQ_RoomEquipSkill
{
	enum { NET_ID = 60071 };

	// TODO: fill this
};

struct CQ_RoomSwapSkill
{
	enum { NET_ID = 60072 };

	// TODO: fill this
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

struct CQ_MasterPick
{
	enum { NET_ID = 60100 };

	LocalActorID localMasterID;
};
ASSERT_SIZE(CQ_MasterPick, 4);

struct CQ_MasterUnpick
{
	enum { NET_ID = 60101 };

	LocalActorID localMasterID;
};
ASSERT_SIZE(CQ_MasterUnpick, 4);

struct CQ_MasterReset
{
	enum { NET_ID = 60102 };
};

struct CQ_ReadySortieRoom
{
	enum { NET_ID = 60103 };

	u8 ready;
};
ASSERT_SIZE(CQ_ReadySortieRoom, 1);

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
	eastl::array<u8,4> ip;

	i32 gameID;
	u32 idcHash; // not sure what this is used for

	u16 nickLen;
	wchar nick[1]; // length is nickLen

	u32 instantKey;
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
	// payloadless: client validator consumes without field checks; captures show size=4 (NetHeader only).
};

struct SN_CastSkill
{
	enum { NET_ID = 62035 };

	LocalActorID entityID;
	i32 ret;
	SkillID skillID;
	u8 costLevel;
	ActionStateID actionState;
	float3 targetPos;

	u16 targetList_count;
	LocalActorID targetList[1];

	u8 bSyncMyPosition;

	struct PosStruct {
		float3 pos;
		float3 destPos;
		float2 moveDir;
		float3 rotateStruct;
		f32 speed;
		i32 clientTime;
	} posStruct;
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
	// payloadless: client validator consumes without field checks; captures show size=4 (NetHeader only).
};

struct SN_ScanEnd
{
	enum { NET_ID = 62051 };
	// payloadless: client validator consumes without field checks; captures show size=4 (NetHeader only).
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
	GameType gameType;
	i32 areaIndex;
	StageIndex stageIndex;
	GameDefinition gameDefinitionType;
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
	// payloadless: client validator consumes without field checks; captures show size=4 (NetHeader only).
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
	// payloadless: client validator consumes without field checks; captures show size=4 (NetHeader only).
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
		SkillID skillIndex;
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
		u32 gearItemID;
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

	SortieUID sortieUID;
	StageIndex stageIndex;
	GameType gameType;
	GameDefinition gameDefinitionType;
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

struct SA_MasterPick
{
	enum { NET_ID = 62208 };

	i32 retval;
	LocalActorID localMasterID;
};
ASSERT_SIZE(SA_MasterPick, 8);

PUSH_PACKED
struct SN_MasterPick
{
	enum { NET_ID = 62209 };

	// client logger: PST_MASTER_SELECT_INFO[(characterID:)(creatureIndex:)(skillSlot1:)(skillSlot2:)
	struct CharacterSelectInfo
	{
		LocalActorID characterID;
		CreatureIndex creatureIndex; // can be different when bots are picking (creature index specifically for bot variants)
		SkillID skillSlot1;
		SkillID skillSlot2;
	};

	UserID userID;
	u16 characterSelectInfos_count; // can be larger than 2
	CharacterSelectInfo characterSelectInfos[2];
};
POP_PACKED
ASSERT_SIZE(SN_MasterPick, 38);

PUSH_PACKED
struct SN_ReadySortieRoom
{
	enum { NET_ID = 62214 };

	UserID userID;
	u8 ready;
};
POP_PACKED
ASSERT_SIZE(SN_ReadySortieRoom, 5);

struct SN_StartCountdownSortieRoom
{
	enum { NET_ID = 62217 };

	StageType stageType;
	i32 timeToWaitSec;
};
ASSERT_SIZE(SN_StartCountdownSortieRoom, 8);

struct SN_SortiePrepare
{
	enum { NET_ID = 62218 };

	PUSH_PACKED
	struct Skin
	{
		ClassType classType;
		SkinIndex skinIndex;
		i32 bufCount;
		i64 expireTime;
	};
	POP_PACKED

	u16 skinList_count;
	Skin skinList[1];
	u16 skillList_count;
	SkillID skillList[1];
};

struct SN_SortiePrepareBotInfo
{
	enum { NET_ID = 62219 };

	u16 botIndexes_count;
	i32 botindexes[1];
};

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
	// payloadless: client validator consumes without field checks; captures show size=4 (NetHeader only).
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

struct SN_LoadingProgressData
{
	enum { NET_ID = 62450 };

	UserID usn;
	u16 nick_len;
	wchar nick[1];
	u8 progressData;
	CreatureIndex activeCreatureIndex;
	CreatureIndex inactiveCreatureIndex;
	u8 isSpectator;
};

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

struct SN_SortieCharacterSlotInfo
{
	enum { NET_ID = 62462 };

	struct SlotInfo {
		CreatureIndex creatureIndex;
		u16 slotStates_count;
		i32 slotSlates[1];
	};

	u16 slotInfos_count;
	SlotInfo slotInfos[1];
};

struct SN_SortieMasterPickPhaseStart
{
	enum { NET_ID = 62465 };

	u8 isRandomPick;
	u16 alliesSlotInfos_count;
};

struct SN_SortieMasterPickPhaseEnd
{
	enum { NET_ID = 62466 };
	// payloadless: client validator consumes without field checks; captures show size=4 (NetHeader only).
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

struct SN_SortieMasterPickPhaseStep
{
	enum { NET_ID = 62468 };

	u8 isRandomPick;
};
ASSERT_SIZE(SN_SortieMasterPickPhaseStep, 1);

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

struct SN_ActionChangeLevelEvent
{
	enum { NET_ID = 62577 };

	u16 targetIDs_count;
	LocalActorID targetIDs[1];
	ActionStateID actionID;
	i64 serverTime;
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

// ==== Auto-extracted from MXMClient_DP_p3.exe (client packet validators) ====
// Field layout = ordered sizes read by the client's per-packet validator (CanRead sequence).
// 4-byte fields are i32/u32/f32 (floats for Vec3/rot fields; IDs/counts are ints); names are placeholders.
namespace Sv {

PUSH_PACKED
struct SN_UNKNOWN_62001
{
	enum { NET_ID = 62001 };
	// Reversed from MXMClient_DP_p3.exe: validator ValidatePacket_62001 (0xa1a46b) only
	// bounds-checks 8+4 bytes (CanRead(8), CanRead(4)) and never reads the values, so the
	// exact types are unconfirmed (i64/i32 or u64/u32 or f64/f32). The client has NO name
	// for this packet (no wide name string, no logger function, absent from sv_name.py)
	// and it is never sent in captured traffic — likely a legacy/unused packet.
	i64 field_0; // 8 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a1a46b
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62001, 12);

PUSH_PACKED
struct SN_PlayerTitleSelect
{
	enum { NET_ID = 62008 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	// handler FUN_00a263d9
};
POP_PACKED
ASSERT_SIZE(SN_PlayerTitleSelect, 12);

PUSH_PACKED
struct SN_DoConnectSpectateServer
{
	enum { NET_ID = 62012 };
	u16 field_0; // 2 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	u16 field_3; // 2 bytes
	// <variable-size field: string/vector>
	i32 field_4; // 4 bytes
	// handler &LAB_00a219f9
};
POP_PACKED

PUSH_PACKED
struct SN_UNKNOWN_62013
{
	enum { NET_ID = 62013 };
	u16 field_0; // 2 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a295bc
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62013, 6);

PUSH_PACKED
struct SN_UNKNOWN_62014
{
	enum { NET_ID = 62014 };
	// payloadless: client validator consumes without field checks, logger dumps no fields,
	// and real captures show size=4 (NetHeader only). Send with no payload.
	// handler FUN_00a2975f
};
POP_PACKED

PUSH_PACKED
struct SQ_PrepareHandoverToChannel
{
	enum { NET_ID = 62015 };
	// payloadless: client validator consumes without field checks, logger dumps no fields,
	// and real captures show size=4 (NetHeader only). Send with no payload.
	// handler FUN_00a29746
};
POP_PACKED

PUSH_PACKED
struct SN_UNKNOWN_62016
{
	enum { NET_ID = 62016 };
	// payloadless: client validator consumes without field checks, logger dumps no fields,
	// and real captures show size=4 (NetHeader only). Send with no payload.
	// handler FUN_00a2072f
};
POP_PACKED

PUSH_PACKED
struct SN_SuspendedToCity
{
	enum { NET_ID = 62017 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	u8 field_2; // 1 bytes
	// handler FUN_00a28a5c
};
POP_PACKED
ASSERT_SIZE(SN_SuspendedToCity, 6);

PUSH_PACKED
struct SA_ReturnToCity
{
	enum { NET_ID = 62018 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1e2fe
};
POP_PACKED
ASSERT_SIZE(SA_ReturnToCity, 4);

PUSH_PACKED
struct SN_EnterChannel
{
	enum { NET_ID = 62019 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a21cc3
};
POP_PACKED
ASSERT_SIZE(SN_EnterChannel, 8);

PUSH_PACKED
struct SN_LeaveChannel
{
	enum { NET_ID = 62020 };
	i32 field_0; // 4 bytes
	// handler FUN_00a248c3
};
POP_PACKED
ASSERT_SIZE(SN_LeaveChannel, 4);

PUSH_PACKED
struct SN_GameRestriction
{
	enum { NET_ID = 62021 };
	i32 field_0; // 4 bytes
	// handler FUN_00a2317e
};
POP_PACKED
ASSERT_SIZE(SN_GameRestriction, 4);

PUSH_PACKED
struct SN_ShopItemList
{
	enum { NET_ID = 62022 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	// handler &LAB_00a27de7
};
POP_PACKED
ASSERT_SIZE(SN_ShopItemList, 20);

PUSH_PACKED
struct SA_SellShopItem
{
	enum { NET_ID = 62023 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i64 field_2; // 8 bytes
	// handler &LAB_00a1e621
};
POP_PACKED
ASSERT_SIZE(SA_SellShopItem, 16);

PUSH_PACKED
struct SN_UNKNOWN_62024
{
	enum { NET_ID = 62024 };
	u8 field_0; // 1 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	i32 field_8; // 4 bytes
	i32 field_9; // 4 bytes
	i32 field_10; // 4 bytes
	i32 field_11; // 4 bytes
	i32 field_12; // 4 bytes
	i32 field_13; // 4 bytes
	u8 field_14; // 1 bytes
	u8 field_15; // 1 bytes
	// handler FUN_00a212b6
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62024, 55);

PUSH_PACKED
struct SN_GameModifyActor
{
	enum { NET_ID = 62027 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	i32 field_8; // 4 bytes
	i32 field_9; // 4 bytes
	i32 field_10; // 4 bytes
	i32 field_11; // 4 bytes
	i32 field_12; // 4 bytes
	i32 field_13; // 4 bytes
	u8 field_14; // 1 bytes
	i32 field_15; // 4 bytes
	i32 field_16; // 4 bytes
	i32 field_17; // 4 bytes
	i32 field_18; // 4 bytes
	i32 field_19; // 4 bytes
	i32 field_20; // 4 bytes
	u16 field_21; // 2 bytes
	u8 field_22; // 1 bytes
	i32 field_23; // 4 bytes
	u16 field_24; // 2 bytes
	u8 field_25; // 1 bytes
	i32 field_26; // 4 bytes
	// handler &LAB_00a22d47
};
POP_PACKED
ASSERT_SIZE(SN_GameModifyActor, 95);

PUSH_PACKED
struct SA_ItemMerge
{
	enum { NET_ID = 62032 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	// handler FUN_00a1ccb3
};
POP_PACKED
ASSERT_SIZE(SA_ItemMerge, 20);

PUSH_PACKED
struct SN_PrecastSkill
{
	enum { NET_ID = 62034 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	u8 field_3; // 1 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	i32 field_8; // 4 bytes
	i32 field_9; // 4 bytes
	i32 field_10; // 4 bytes
	i32 field_11; // 4 bytes
	i32 field_12; // 4 bytes
	i32 field_13; // 4 bytes
	i32 field_14; // 4 bytes
	i32 field_15; // 4 bytes
	i32 field_16; // 4 bytes
	// handler FUN_00a2656d
};
POP_PACKED
ASSERT_SIZE(SN_PrecastSkill, 65);

PUSH_PACKED
struct SN_SwitchOnToggleSkill
{
	enum { NET_ID = 62037 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	// handler &LAB_00a28afc
};
POP_PACKED
ASSERT_SIZE(SN_SwitchOnToggleSkill, 20);

PUSH_PACKED
struct SN_SwitchOffToggleSkill
{
	enum { NET_ID = 62038 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	// handler FUN_00a28aa4
};
POP_PACKED
ASSERT_SIZE(SN_SwitchOffToggleSkill, 16);

PUSH_PACKED
struct SN_CancelSkill
{
	enum { NET_ID = 62039 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	// handler FUN_00a20748
};
POP_PACKED
ASSERT_SIZE(SN_CancelSkill, 12);

PUSH_PACKED
struct SA_PrecastSkill
{
	enum { NET_ID = 62040 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	// handler FUN_00a1da68
};
POP_PACKED
ASSERT_SIZE(SA_PrecastSkill, 12);

PUSH_PACKED
struct SA_ExecuteSkill
{
	enum { NET_ID = 62042 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	// handler FUN_00a1b400
};
POP_PACKED
ASSERT_SIZE(SA_ExecuteSkill, 12);

PUSH_PACKED
struct SA_SwitchOnToggleSkill
{
	enum { NET_ID = 62043 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	// handler FUN_00a1ecf9
};
POP_PACKED
ASSERT_SIZE(SA_SwitchOnToggleSkill, 16);

PUSH_PACKED
struct SA_SwitchOffToggleSkill
{
	enum { NET_ID = 62044 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	// handler FUN_00a1eca1
};
POP_PACKED
ASSERT_SIZE(SA_SwitchOffToggleSkill, 16);

PUSH_PACKED
struct SA_CancelSkill
{
	enum { NET_ID = 62045 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	// handler FUN_00a1a7ea
};
POP_PACKED
ASSERT_SIZE(SA_CancelSkill, 12);

PUSH_PACKED
struct SN_ChangeSkillSlot
{
	enum { NET_ID = 62046 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	// handler FUN_00a209d1
};
POP_PACKED
ASSERT_SIZE(SN_ChangeSkillSlot, 20);

PUSH_PACKED
struct SN_UpdateTargetGraphMove
{
	enum { NET_ID = 62049 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	i32 field_8; // 4 bytes
	// handler FUN_00a29226
};
POP_PACKED
ASSERT_SIZE(SN_UpdateTargetGraphMove, 36);

PUSH_PACKED
struct SA_ItemChange
{
	enum { NET_ID = 62053 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	// handler FUN_00a1cb23
};
POP_PACKED
ASSERT_SIZE(SA_ItemChange, 12);

PUSH_PACKED
struct SA_ItemDelete
{
	enum { NET_ID = 62054 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1cbdb
};
POP_PACKED
ASSERT_SIZE(SA_ItemDelete, 4);

PUSH_PACKED
struct SA_WarehouseItemDelete
{
	enum { NET_ID = 62055 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1f135
};
POP_PACKED
ASSERT_SIZE(SA_WarehouseItemDelete, 4);

PUSH_PACKED
struct SN_UpdateStat
{
	enum { NET_ID = 62056 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	// handler FUN_00a291c0
};
POP_PACKED
ASSERT_SIZE(SN_UpdateStat, 20);

PUSH_PACKED
struct SN_CCoin
{
	enum { NET_ID = 62058 };
	i64 field_0; // 8 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a2149a
};
POP_PACKED
ASSERT_SIZE(SN_CCoin, 12);

PUSH_PACKED
struct SN_DeadAck
{
	enum { NET_ID = 62062 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	u8 field_5; // 1 bytes
	// handler FUN_00a214ed
};
POP_PACKED
ASSERT_SIZE(SN_DeadAck, 21);

PUSH_PACKED
struct SN_DeadDamageInfo
{
	enum { NET_ID = 62063 };
	u16 field_0; // 2 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	u8 field_5; // 1 bytes
	u8 field_6; // 1 bytes
	i32 field_7; // 4 bytes
	u16 field_8; // 2 bytes
	i32 field_9; // 4 bytes
	i32 field_10; // 4 bytes
	i32 field_11; // 4 bytes
	i32 field_12; // 4 bytes
	u8 field_13; // 1 bytes
	u8 field_14; // 1 bytes
	i32 field_15; // 4 bytes
	i32 field_16; // 4 bytes
	// handler &LAB_00a21562
};
POP_PACKED
ASSERT_SIZE(SN_DeadDamageInfo, 52);

PUSH_PACKED
struct SA_BuyShopItem
{
	enum { NET_ID = 62065 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a1a6ef
};
POP_PACKED
ASSERT_SIZE(SA_BuyShopItem, 8);

PUSH_PACKED
struct SA_BuyCshopItem
{
	enum { NET_ID = 62066 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a1a5ff
};
POP_PACKED
ASSERT_SIZE(SA_BuyCshopItem, 8);

PUSH_PACKED
struct SA_GiftFriendsAvailable
{
	enum { NET_ID = 62067 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a1c868
};
POP_PACKED

PUSH_PACKED
struct SA_GiftFriendCharsAndSkins
{
	enum { NET_ID = 62068 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	// handler &LAB_00a1c8cb
};
POP_PACKED

PUSH_PACKED
struct SA_BuyGift
{
	enum { NET_ID = 62069 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1a639
};
POP_PACKED
ASSERT_SIZE(SA_BuyGift, 4);

PUSH_PACKED
struct SN_PlayerSkillUpdate
{
	enum { NET_ID = 62070 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	u8 field_3; // 1 bytes
	u16 field_4; // 2 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	u8 field_7; // 1 bytes
	u8 field_8; // 1 bytes
	// handler &LAB_00a2613a
};
POP_PACKED
ASSERT_SIZE(SN_PlayerSkillUpdate, 25);

PUSH_PACKED
struct SN_GameAreaMove
{
	enum { NET_ID = 62071 };
	i32 field_0; // 4 bytes
	// handler FUN_00a2268f
};
POP_PACKED
ASSERT_SIZE(SN_GameAreaMove, 4);

PUSH_PACKED
struct SN_TrespassReady
{
	enum { NET_ID = 62074 };
	u8 field_0; // 1 bytes
	u8 field_1; // 1 bytes
	i32 field_2; // 4 bytes
	// handler FUN_00a28fe2
};
POP_PACKED
ASSERT_SIZE(SN_TrespassReady, 6);

PUSH_PACKED
struct SN_TrespassGameStart
{
	enum { NET_ID = 62077 };
	i32 field_0; // 4 bytes
	// handler FUN_00a28fb2
};
POP_PACKED
ASSERT_SIZE(SN_TrespassGameStart, 4);

PUSH_PACKED
struct SA_ItemUse
{
	enum { NET_ID = 62078 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i64 field_2; // 8 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	// handler &LAB_00a1cdc2
};
POP_PACKED
ASSERT_SIZE(SA_ItemUse, 28);

PUSH_PACKED
struct SN_PvpResult
{
	enum { NET_ID = 62079 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	u16 field_6; // 2 bytes
	i32 field_7; // 4 bytes
	i32 field_8; // 4 bytes
	i32 field_9; // 4 bytes
	u16 field_10; // 2 bytes
	i32 field_11; // 4 bytes
	i32 field_12; // 4 bytes
	u16 field_13; // 2 bytes
	i32 field_14; // 4 bytes
	i32 field_15; // 4 bytes
	i32 field_16; // 4 bytes
	i32 field_17; // 4 bytes
	i32 field_18; // 4 bytes
	u8 field_19; // 1 bytes
	u16 field_20; // 2 bytes
	u16 field_21; // 2 bytes
	u16 field_22; // 2 bytes
	i32 field_23; // 4 bytes
	i32 field_24; // 4 bytes
	i32 field_25; // 4 bytes
	u8 field_26; // 1 bytes
	u8 field_27; // 1 bytes
	u16 field_28; // 2 bytes
	u16 field_29; // 2 bytes
	u16 field_30; // 2 bytes
	i32 field_31; // 4 bytes
	// handler &LAB_00a26a16
};
POP_PACKED
ASSERT_SIZE(SN_PvpResult, 101);

PUSH_PACKED
struct SN_PvpResultScoreDeathmatch
{
	enum { NET_ID = 62080 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	u16 field_3; // 2 bytes
	i32 field_4; // 4 bytes
	u8 field_5; // 1 bytes
	i32 field_6; // 4 bytes
	u16 field_7; // 2 bytes
	i32 field_8; // 4 bytes
	i32 field_9; // 4 bytes
	u8 field_10; // 1 bytes
	u8 field_11; // 1 bytes
	u8 field_12; // 1 bytes
	u8 field_13; // 1 bytes
	u8 field_14; // 1 bytes
	i32 field_15; // 4 bytes
	i32 field_16; // 4 bytes
	i32 field_17; // 4 bytes
	i32 field_18; // 4 bytes
	i32 field_19; // 4 bytes
	i32 field_20; // 4 bytes
	i32 field_21; // 4 bytes
	i32 field_22; // 4 bytes
	// handler &LAB_00a26b16
};
POP_PACKED
ASSERT_SIZE(SN_PvpResultScoreDeathmatch, 70);

PUSH_PACKED
struct SN_PvpResultScoreOccupy
{
	enum { NET_ID = 62081 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	u16 field_3; // 2 bytes
	i32 field_4; // 4 bytes
	u8 field_5; // 1 bytes
	u16 field_6; // 2 bytes
	i32 field_7; // 4 bytes
	i32 field_8; // 4 bytes
	u8 field_9; // 1 bytes
	u8 field_10; // 1 bytes
	u8 field_11; // 1 bytes
	u8 field_12; // 1 bytes
	u8 field_13; // 1 bytes
	i32 field_14; // 4 bytes
	i32 field_15; // 4 bytes
	i32 field_16; // 4 bytes
	i32 field_17; // 4 bytes
	i32 field_18; // 4 bytes
	i32 field_19; // 4 bytes
	i32 field_20; // 4 bytes
	i32 field_21; // 4 bytes
	u8 field_22; // 1 bytes
	u16 field_23; // 2 bytes
	// handler &LAB_00a26c08
};
POP_PACKED
ASSERT_SIZE(SN_PvpResultScoreOccupy, 69);

PUSH_PACKED
struct SN_PvpResultScoreGot
{
	enum { NET_ID = 62082 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	u16 field_2; // 2 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	u16 field_5; // 2 bytes
	i32 field_6; // 4 bytes
	u8 field_7; // 1 bytes
	u8 field_8; // 1 bytes
	i32 field_9; // 4 bytes
	u16 field_10; // 2 bytes
	u8 field_11; // 1 bytes
	u8 field_12; // 1 bytes
	u16 field_13; // 2 bytes
	u8 field_14; // 1 bytes
	u8 field_15; // 1 bytes
	u16 field_16; // 2 bytes
	i32 field_17; // 4 bytes
	i32 field_18; // 4 bytes
	u8 field_19; // 1 bytes
	u8 field_20; // 1 bytes
	u8 field_21; // 1 bytes
	u8 field_22; // 1 bytes
	u8 field_23; // 1 bytes
	i32 field_24; // 4 bytes
	i32 field_25; // 4 bytes
	i32 field_26; // 4 bytes
	i32 field_27; // 4 bytes
	i32 field_28; // 4 bytes
	i32 field_29; // 4 bytes
	i32 field_30; // 4 bytes
	i32 field_31; // 4 bytes
	i32 field_32; // 4 bytes
	i32 field_33; // 4 bytes
	u8 field_34; // 1 bytes
	u8 field_35; // 1 bytes
	u8 field_36; // 1 bytes
	u8 field_37; // 1 bytes
	u8 field_38; // 1 bytes
	u8 field_39; // 1 bytes
	u8 field_40; // 1 bytes
	u16 field_41; // 2 bytes
	i32 field_42; // 4 bytes
	i32 field_43; // 4 bytes
	u8 field_44; // 1 bytes
	u8 field_45; // 1 bytes
	u8 field_46; // 1 bytes
	u8 field_47; // 1 bytes
	u8 field_48; // 1 bytes
	i32 field_49; // 4 bytes
	i32 field_50; // 4 bytes
	i32 field_51; // 4 bytes
	i32 field_52; // 4 bytes
	i32 field_53; // 4 bytes
	i32 field_54; // 4 bytes
	i32 field_55; // 4 bytes
	i32 field_56; // 4 bytes
	i32 field_57; // 4 bytes
	i32 field_58; // 4 bytes
	u8 field_59; // 1 bytes
	// handler &LAB_00a26b8f
};
POP_PACKED
ASSERT_SIZE(SN_PvpResultScoreGot, 156);

PUSH_PACKED
struct SN_PvpResultScoreSport
{
	enum { NET_ID = 62083 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	u16 field_3; // 2 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	u8 field_7; // 1 bytes
	i32 field_8; // 4 bytes
	u8 field_9; // 1 bytes
	u8 field_10; // 1 bytes
	i32 field_11; // 4 bytes
	// handler &LAB_00a26c81
};
POP_PACKED
ASSERT_SIZE(SN_PvpResultScoreSport, 37);

PUSH_PACKED
struct SA_GetGroundItem
{
	enum { NET_ID = 62085 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	// handler FUN_00a1baa3
};
POP_PACKED
ASSERT_SIZE(SA_GetGroundItem, 16);

PUSH_PACKED
struct SN_StageClearResult
{
	enum { NET_ID = 62086 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	u16 field_3; // 2 bytes
	u8 field_4; // 1 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	i32 field_8; // 4 bytes
	u8 field_9; // 1 bytes
	i32 field_10; // 4 bytes
	i32 field_11; // 4 bytes
	i32 field_12; // 4 bytes
	i32 field_13; // 4 bytes
	i32 field_14; // 4 bytes
	i32 field_15; // 4 bytes
	i32 field_16; // 4 bytes
	i32 field_17; // 4 bytes
	i32 field_18; // 4 bytes
	i32 field_19; // 4 bytes
	i32 field_20; // 4 bytes
	i32 field_21; // 4 bytes
	i32 field_22; // 4 bytes
	u8 field_23; // 1 bytes
	i32 field_24; // 4 bytes
	i32 field_25; // 4 bytes
	u8 field_26; // 1 bytes
	u16 field_27; // 2 bytes
	u16 field_28; // 2 bytes
	u16 field_29; // 2 bytes
	// handler &LAB_00a28560
};
POP_PACKED
ASSERT_SIZE(SN_StageClearResult, 100);

PUSH_PACKED
struct SN_DefenceClearResult
{
	enum { NET_ID = 62087 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	u16 field_3; // 2 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	i32 field_8; // 4 bytes
	u16 field_9; // 2 bytes
	i32 field_10; // 4 bytes
	i32 field_11; // 4 bytes
	i32 field_12; // 4 bytes
	u16 field_13; // 2 bytes
	i32 field_14; // 4 bytes
	i32 field_15; // 4 bytes
	u16 field_16; // 2 bytes
	i32 field_17; // 4 bytes
	i32 field_18; // 4 bytes
	i32 field_19; // 4 bytes
	i32 field_20; // 4 bytes
	i32 field_21; // 4 bytes
	u8 field_22; // 1 bytes
	u16 field_23; // 2 bytes
	u16 field_24; // 2 bytes
	u16 field_25; // 2 bytes
	i32 field_26; // 4 bytes
	i32 field_27; // 4 bytes
	i32 field_28; // 4 bytes
	i32 field_29; // 4 bytes
	// handler &LAB_00a21609
};
POP_PACKED
ASSERT_SIZE(SN_DefenceClearResult, 103);

PUSH_PACKED
struct SN_UNKNOWN_62088
{
	enum { NET_ID = 62088 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a28526
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62088, 8);

PUSH_PACKED
struct SN_CommunityChat
{
	enum { NET_ID = 62092 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	u16 field_2; // 2 bytes
	// <variable-size field: string/vector>
	u8 field_3; // 1 bytes
	u16 field_4; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_5; // 2 bytes
	// handler &LAB_00a21153
};
POP_PACKED

PUSH_PACKED
struct SN_GetGroundItem
{
	enum { NET_ID = 62093 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	// handler FUN_00a2323d
};
POP_PACKED
ASSERT_SIZE(SN_GetGroundItem, 12);

PUSH_PACKED
struct SN_DestroyGroundItem
{
	enum { NET_ID = 62094 };
	i32 field_0; // 4 bytes
	// handler FUN_00a21847
};
POP_PACKED
ASSERT_SIZE(SN_DestroyGroundItem, 4);

PUSH_PACKED
struct SN_GroundItemsSnapshot
{
	enum { NET_ID = 62095 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	u8 field_7; // 1 bytes
	u8 field_8; // 1 bytes
	u8 field_9; // 1 bytes
	i32 field_10; // 4 bytes
	i32 field_11; // 4 bytes
	i32 field_12; // 4 bytes
	// handler &LAB_00a23429
};
POP_PACKED
ASSERT_SIZE(SN_GroundItemsSnapshot, 43);

PUSH_PACKED
struct SA_WorldSetInfoList
{
	enum { NET_ID = 62096 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	i32 field_8; // 4 bytes
	i32 field_9; // 4 bytes
	// handler &LAB_00a1f367
};
POP_PACKED
ASSERT_SIZE(SA_WorldSetInfoList, 40);

PUSH_PACKED
struct SN_AntihackAuth
{
	enum { NET_ID = 62099 };
	u8 field_0; // 1 bytes
	// handler &LAB_00a200e1
};
POP_PACKED
ASSERT_SIZE(SN_AntihackAuth, 1);

PUSH_PACKED
struct SN_MissionList
{
	enum { NET_ID = 62103 };
	i32 field_0; // 4 bytes
	i64 field_1; // 8 bytes
	u16 field_2; // 2 bytes
	u8 field_3; // 1 bytes
	u8 field_4; // 1 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	// handler &LAB_00a2520f
};
POP_PACKED
ASSERT_SIZE(SN_MissionList, 28);

PUSH_PACKED
struct SN_MissionUpdate
{
	enum { NET_ID = 62104 };
	i32 field_0; // 4 bytes
	i64 field_1; // 8 bytes
	u16 field_2; // 2 bytes
	u8 field_3; // 1 bytes
	u8 field_4; // 1 bytes
	// handler FUN_00a2528b
};
POP_PACKED
ASSERT_SIZE(SN_MissionUpdate, 16);

PUSH_PACKED
struct SA_DailyMissionChange
{
	enum { NET_ID = 62105 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i64 field_3; // 8 bytes
	u16 field_4; // 2 bytes
	u8 field_5; // 1 bytes
	u8 field_6; // 1 bytes
	// handler FUN_00a1ae72
};
POP_PACKED
ASSERT_SIZE(SA_DailyMissionChange, 24);

PUSH_PACKED
struct SN_ChangedUserGradeInfo
{
	enum { NET_ID = 62108 };
	u8 field_0; // 1 bytes
	u8 field_1; // 1 bytes
	i64 field_2; // 8 bytes
	u8 field_3; // 1 bytes
	u16 field_4; // 2 bytes
	u16 field_5; // 2 bytes
	i32 field_6; // 4 bytes
	u8 field_7; // 1 bytes
	// handler FUN_00a20863
};
POP_PACKED
ASSERT_SIZE(SN_ChangedUserGradeInfo, 20);

PUSH_PACKED
struct SN_StageRank
{
	enum { NET_ID = 62110 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a286f5
};
POP_PACKED
ASSERT_SIZE(SN_StageRank, 8);

PUSH_PACKED
struct SN_GetPublicGroundItem
{
	enum { NET_ID = 62111 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a23287
};
POP_PACKED
ASSERT_SIZE(SN_GetPublicGroundItem, 8);

PUSH_PACKED
struct SN_UpdateGamePlayerTagCooltime
{
	enum { NET_ID = 62114 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a2912d
};
POP_PACKED
ASSERT_SIZE(SN_UpdateGamePlayerTagCooltime, 8);

PUSH_PACKED
struct SN_UpdateCanCastSkillSlotUg
{
	enum { NET_ID = 62115 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	// handler FUN_00a29070
};
POP_PACKED
ASSERT_SIZE(SN_UpdateCanCastSkillSlotUg, 5);

PUSH_PACKED
struct SN_ItemAcquisition
{
	enum { NET_ID = 62116 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	// handler FUN_00a24606
};
POP_PACKED
ASSERT_SIZE(SN_ItemAcquisition, 16);

PUSH_PACKED
struct SN_ItemUpdate
{
	enum { NET_ID = 62117 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	u8 field_6; // 1 bytes
	i64 field_7; // 8 bytes
	u16 field_8; // 2 bytes
	u8 field_9; // 1 bytes
	i32 field_10; // 4 bytes
	u8 field_11; // 1 bytes
	i32 field_12; // 4 bytes
	u8 field_13; // 1 bytes
	i32 field_14; // 4 bytes
	// handler &LAB_00a246f3
};
POP_PACKED
ASSERT_SIZE(SN_ItemUpdate, 47);

PUSH_PACKED
struct SN_UNKNOWN_62118
{
	enum { NET_ID = 62118 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a21437
};
POP_PACKED

PUSH_PACKED
struct SN_NicknameNeeded
{
	enum { NET_ID = 62119 };
	// payloadless: client validator consumes without field checks, logger dumps no fields,
	// and real captures show size=4 (NetHeader only). Send with no payload.
	// handler FUN_00a2560f
};
POP_PACKED

PUSH_PACKED
struct SA_NicknameSet
{
	enum { NET_ID = 62120 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a1d3ec
};
POP_PACKED

PUSH_PACKED
struct SN_ExtraCharacters
{
	enum { NET_ID = 62130 };
	u8 field_0; // 1 bytes
	i32 field_1; // 4 bytes
	u16 field_2; // 2 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	u8 field_5; // 1 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	i32 field_8; // 4 bytes
	i32 field_9; // 4 bytes
	i32 field_10; // 4 bytes
	i32 field_11; // 4 bytes
	i32 field_12; // 4 bytes
	i32 field_13; // 4 bytes
	i32 field_14; // 4 bytes
	i32 field_15; // 4 bytes
	i32 field_16; // 4 bytes
	u8 field_17; // 1 bytes
	i32 field_18; // 4 bytes
	u8 field_19; // 1 bytes
	i32 field_20; // 4 bytes
	i32 field_21; // 4 bytes
	i32 field_22; // 4 bytes
	i32 field_23; // 4 bytes
	u8 field_24; // 1 bytes
	i64 field_25; // 8 bytes
	u16 field_26; // 2 bytes
	u8 field_27; // 1 bytes
	i32 field_28; // 4 bytes
	u8 field_29; // 1 bytes
	i32 field_30; // 4 bytes
	u8 field_31; // 1 bytes
	i32 field_32; // 4 bytes
	i32 field_33; // 4 bytes
	i32 field_34; // 4 bytes
	i32 field_35; // 4 bytes
	u8 field_36; // 1 bytes
	u8 field_37; // 1 bytes
	i32 field_38; // 4 bytes
	i32 field_39; // 4 bytes
	u8 field_40; // 1 bytes
	u8 field_41; // 1 bytes
	u16 field_42; // 2 bytes
	i32 field_43; // 4 bytes
	i32 field_44; // 4 bytes
	u8 field_45; // 1 bytes
	u16 field_46; // 2 bytes
	u16 field_47; // 2 bytes
	i32 field_48; // 4 bytes
	i32 field_49; // 4 bytes
	// handler &LAB_00a22011
};
POP_PACKED
ASSERT_SIZE(SN_ExtraCharacters, 155);

PUSH_PACKED
struct SN_UNKNOWN_62131
{
	enum { NET_ID = 62131 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	u8 field_6; // 1 bytes
	i64 field_7; // 8 bytes
	u16 field_8; // 2 bytes
	u8 field_9; // 1 bytes
	i32 field_10; // 4 bytes
	u8 field_11; // 1 bytes
	i32 field_12; // 4 bytes
	u8 field_13; // 1 bytes
	i32 field_14; // 4 bytes
	i32 field_15; // 4 bytes
	i32 field_16; // 4 bytes
	i32 field_17; // 4 bytes
	u8 field_18; // 1 bytes
	u8 field_19; // 1 bytes
	i32 field_20; // 4 bytes
	i32 field_21; // 4 bytes
	u8 field_22; // 1 bytes
	u8 field_23; // 1 bytes
	u16 field_24; // 2 bytes
	i32 field_25; // 4 bytes
	i32 field_26; // 4 bytes
	u8 field_27; // 1 bytes
	u16 field_28; // 2 bytes
	u16 field_29; // 2 bytes
	i32 field_30; // 4 bytes
	i32 field_31; // 4 bytes
	// handler &LAB_00a2212f
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62131, 94);

PUSH_PACKED
struct SA_SkillUpgrade
{
	enum { NET_ID = 62132 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	u8 field_4; // 1 bytes
	u8 field_5; // 1 bytes
	u16 field_6; // 2 bytes
	i32 field_7; // 4 bytes
	i32 field_8; // 4 bytes
	// handler &LAB_00a1ea56
};
POP_PACKED
ASSERT_SIZE(SA_SkillUpgrade, 28);

PUSH_PACKED
struct SA_SkillUpgradeIngame
{
	enum { NET_ID = 62133 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	u8 field_4; // 1 bytes
	u8 field_5; // 1 bytes
	u16 field_6; // 2 bytes
	i32 field_7; // 4 bytes
	i32 field_8; // 4 bytes
	i32 field_9; // 4 bytes
	i32 field_10; // 4 bytes
	i32 field_11; // 4 bytes
	u8 field_12; // 1 bytes
	u8 field_13; // 1 bytes
	u16 field_14; // 2 bytes
	i32 field_15; // 4 bytes
	i32 field_16; // 4 bytes
	// handler &LAB_00a1eada
};
POP_PACKED
ASSERT_SIZE(SA_SkillUpgradeIngame, 52);

PUSH_PACKED
struct SA_TitleSelect
{
	enum { NET_ID = 62134 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	// handler FUN_00a1ee79
};
POP_PACKED
ASSERT_SIZE(SA_TitleSelect, 12);

PUSH_PACKED
struct SN_TitleAdd
{
	enum { NET_ID = 62135 };
	i32 field_0; // 4 bytes
	// handler FUN_00a28e3f
};
POP_PACKED
ASSERT_SIZE(SN_TitleAdd, 4);

PUSH_PACKED
struct SA_ItemEquip
{
	enum { NET_ID = 62136 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a1cc79
};
POP_PACKED
ASSERT_SIZE(SA_ItemEquip, 8);

PUSH_PACKED
struct SA_ItemUnequip
{
	enum { NET_ID = 62137 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a1cd88
};
POP_PACKED
ASSERT_SIZE(SA_ItemUnequip, 8);

PUSH_PACKED
struct SA_SelectAccountEquipment
{
	enum { NET_ID = 62138 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	i32 field_2; // 4 bytes
	// handler FUN_00a1e5a9
};
POP_PACKED
ASSERT_SIZE(SA_SelectAccountEquipment, 9);

PUSH_PACKED
struct SA_WeaponUnlock
{
	enum { NET_ID = 62139 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	u8 field_5; // 1 bytes
	u8 field_6; // 1 bytes
	// handler FUN_00a1f227
};
POP_PACKED
ASSERT_SIZE(SA_WeaponUnlock, 22);

PUSH_PACKED
struct SA_WeaponEquip
{
	enum { NET_ID = 62140 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	// handler FUN_00a1f1cf
};
POP_PACKED
ASSERT_SIZE(SA_WeaponEquip, 16);

PUSH_PACKED
struct SA_GearEquip
{
	enum { NET_ID = 62141 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	// handler FUN_00a1b950
};
POP_PACKED
ASSERT_SIZE(SA_GearEquip, 13);

PUSH_PACKED
struct SA_GearUnequip
{
	enum { NET_ID = 62142 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	i32 field_2; // 4 bytes
	// handler FUN_00a1ba1c
};
POP_PACKED
ASSERT_SIZE(SA_GearUnequip, 9);

PUSH_PACKED
struct SA_GearSwap
{
	enum { NET_ID = 62143 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	u8 field_2; // 1 bytes
	i32 field_3; // 4 bytes
	u8 field_4; // 1 bytes
	i32 field_5; // 4 bytes
	// handler FUN_00a1b9a8
};
POP_PACKED
ASSERT_SIZE(SA_GearSwap, 15);

PUSH_PACKED
struct SN_UNKNOWN_62144
{
	enum { NET_ID = 62144 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	u8 field_2; // 1 bytes
	i32 field_3; // 4 bytes
	// handler &LAB_00a1d1e7
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62144, 13);

PUSH_PACKED
struct SN_UNKNOWN_62145
{
	enum { NET_ID = 62145 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	u8 field_2; // 1 bytes
	// handler &LAB_00a1d280
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62145, 9);

PUSH_PACKED
struct SA_MastergearSelect
{
	enum { NET_ID = 62146 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	u8 field_2; // 1 bytes
	// handler FUN_00a1d093
};
POP_PACKED
ASSERT_SIZE(SA_MastergearSelect, 9);

PUSH_PACKED
struct SA_MastergearRename
{
	enum { NET_ID = 62147 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	u16 field_2; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a1cfd8
};
POP_PACKED

PUSH_PACKED
struct SA_MastergearRepair
{
	enum { NET_ID = 62148 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a1d059
};
POP_PACKED
ASSERT_SIZE(SA_MastergearRepair, 8);

PUSH_PACKED
struct SA_MastergearAdd
{
	enum { NET_ID = 62149 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	u16 field_2; // 2 bytes
	u16 field_3; // 2 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	// handler &LAB_00a1cf79
};
POP_PACKED
ASSERT_SIZE(SA_MastergearAdd, 17);

PUSH_PACKED
struct SA_CharacterUnlock
{
	enum { NET_ID = 62150 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a1a988
};
POP_PACKED
ASSERT_SIZE(SA_CharacterUnlock, 8);

PUSH_PACKED
struct SA_SkillUnlock
{
	enum { NET_ID = 62151 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	u8 field_3; // 1 bytes
	u8 field_4; // 1 bytes
	u16 field_5; // 2 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	// handler &LAB_00a1e9f5
};
POP_PACKED
ASSERT_SIZE(SA_SkillUnlock, 24);

PUSH_PACKED
struct SA_SkillSelect
{
	enum { NET_ID = 62152 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	// handler FUN_00a1e945
};
POP_PACKED
ASSERT_SIZE(SA_SkillSelect, 16);

PUSH_PACKED
struct SN_SkillSelect
{
	enum { NET_ID = 62153 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	// handler FUN_00a27e53
};
POP_PACKED
ASSERT_SIZE(SN_SkillSelect, 16);

PUSH_PACKED
struct SA_SkillSwap
{
	enum { NET_ID = 62154 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	// handler FUN_00a1e99d
};
POP_PACKED
ASSERT_SIZE(SA_SkillSwap, 16);

PUSH_PACKED
struct SN_SkillSwap
{
	enum { NET_ID = 62155 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	// handler FUN_00a27eab
};
POP_PACKED
ASSERT_SIZE(SN_SkillSwap, 16);

PUSH_PACKED
struct SN_UNKNOWN_62156
{
	enum { NET_ID = 62156 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1dab2
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62156, 4);

PUSH_PACKED
struct SA_StartGame
{
	enum { NET_ID = 62157 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1ec71
};
POP_PACKED
ASSERT_SIZE(SA_StartGame, 4);

PUSH_PACKED
struct SN_RestartVoteStart
{
	enum { NET_ID = 62159 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	u16 field_5; // 2 bytes
	u8 field_6; // 1 bytes
	i32 field_7; // 4 bytes
	i32 field_8; // 4 bytes
	i32 field_9; // 4 bytes
	i32 field_10; // 4 bytes
	u16 field_11; // 2 bytes
	// handler &LAB_00a27834
};
POP_PACKED
ASSERT_SIZE(SN_RestartVoteStart, 41);

PUSH_PACKED
struct SN_RestartVoteFinish
{
	enum { NET_ID = 62160 };
	u8 field_0; // 1 bytes
	u8 field_1; // 1 bytes
	// handler FUN_00a277fc
};
POP_PACKED
ASSERT_SIZE(SN_RestartVoteFinish, 2);

PUSH_PACKED
struct SN_RestartVoteUpdate
{
	enum { NET_ID = 62161 };
	u8 field_0; // 1 bytes
	// handler &LAB_00a278ca
};
POP_PACKED
ASSERT_SIZE(SN_RestartVoteUpdate, 1);

PUSH_PACKED
struct SA_RestartGame
{
	enum { NET_ID = 62162 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1e16e
};
POP_PACKED
ASSERT_SIZE(SA_RestartGame, 4);

PUSH_PACKED
struct SA_RestartVote
{
	enum { NET_ID = 62163 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1e1fe
};
POP_PACKED
ASSERT_SIZE(SA_RestartVote, 4);

PUSH_PACKED
struct SN_RestartPvpEntryStart
{
	enum { NET_ID = 62164 };
	i32 field_0; // 4 bytes
	// handler FUN_00a276fd
};
POP_PACKED
ASSERT_SIZE(SN_RestartPvpEntryStart, 4);

PUSH_PACKED
struct SN_RestartPvpEntryFinish
{
	enum { NET_ID = 62165 };
	// payloadless: client validator consumes without field checks, logger dumps no fields,
	// and real captures show size=4 (NetHeader only). Send with no payload.
	// handler FUN_00a276e4
};
POP_PACKED

PUSH_PACKED
struct SA_RestartPvpEntryJoin
{
	enum { NET_ID = 62166 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1e19e
};
POP_PACKED
ASSERT_SIZE(SA_RestartPvpEntryJoin, 4);

PUSH_PACKED
struct SA_RestartPvpEntryLeave
{
	enum { NET_ID = 62167 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1e1ce
};
POP_PACKED
ASSERT_SIZE(SA_RestartPvpEntryLeave, 4);

PUSH_PACKED
struct SN_RestartPvpEntryUpdate
{
	enum { NET_ID = 62168 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	u8 field_1; // 1 bytes
	// handler &LAB_00a2772d
};
POP_PACKED

PUSH_PACKED
struct SN_RestartPvpGameInfo
{
	enum { NET_ID = 62169 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	u8 field_2; // 1 bytes
	// handler FUN_00a2779b
};
POP_PACKED
ASSERT_SIZE(SN_RestartPvpGameInfo, 6);

PUSH_PACKED
struct SN_RestartPvpWaitingOthers
{
	enum { NET_ID = 62170 };
	// payloadless: client validator consumes without field checks, logger dumps no fields,
	// and real captures show size=4 (NetHeader only). Send with no payload.
	// handler FUN_00a277e3
};
POP_PACKED

PUSH_PACKED
struct SN_RestartPvpCanceled
{
	enum { NET_ID = 62171 };
	i32 field_0; // 4 bytes
	// handler FUN_00a276b4
};
POP_PACKED
ASSERT_SIZE(SN_RestartPvpCanceled, 4);

PUSH_PACKED
struct SN_MatchingPenalty
{
	enum { NET_ID = 62172 };
	u8 field_0; // 1 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a251d7
};
POP_PACKED
ASSERT_SIZE(SN_MatchingPenalty, 5);

PUSH_PACKED
struct SN_PartySwapTeam
{
	enum { NET_ID = 62178 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	i32 field_2; // 4 bytes
	u8 field_3; // 1 bytes
	// handler FUN_00a25e62
};
POP_PACKED
ASSERT_SIZE(SN_PartySwapTeam, 10);

PUSH_PACKED
struct SA_PartySwapTeam
{
	enum { NET_ID = 62179 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1d9d8
};
POP_PACKED
ASSERT_SIZE(SA_PartySwapTeam, 4);

PUSH_PACKED
struct SA_PartyInvite
{
	enum { NET_ID = 62180 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a1d670
};
POP_PACKED

PUSH_PACKED
struct SA_PartyInviteRecommend
{
	enum { NET_ID = 62181 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a1d6e3
};
POP_PACKED

PUSH_PACKED
struct SN_PartyInviteResponse
{
	enum { NET_ID = 62182 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	u8 field_1; // 1 bytes
	// handler &LAB_00a25c68
};
POP_PACKED

PUSH_PACKED
struct SN_PartyInvite
{
	enum { NET_ID = 62183 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	// handler &LAB_00a25a88
};
POP_PACKED

PUSH_PACKED
struct SN_PartyInviteRecommend
{
	enum { NET_ID = 62184 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a25b66
};
POP_PACKED

PUSH_PACKED
struct SA_PartyInviteRecommendResponse
{
	enum { NET_ID = 62185 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a1d756
};
POP_PACKED

PUSH_PACKED
struct SN_PartyInviteRecommendResponse
{
	enum { NET_ID = 62186 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	u8 field_1; // 1 bytes
	// handler &LAB_00a25bfa
};
POP_PACKED

PUSH_PACKED
struct SA_PartyJoin
{
	enum { NET_ID = 62187 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	u16 field_6; // 2 bytes
	u8 field_7; // 1 bytes
	i32 field_8; // 4 bytes
	i32 field_9; // 4 bytes
	i32 field_10; // 4 bytes
	i32 field_11; // 4 bytes
	i32 field_12; // 4 bytes
	i32 field_13; // 4 bytes
	u16 field_14; // 2 bytes
	u8 field_15; // 1 bytes
	i32 field_16; // 4 bytes
	i32 field_17; // 4 bytes
	i32 field_18; // 4 bytes
	i32 field_19; // 4 bytes
	i32 field_20; // 4 bytes
	i32 field_21; // 4 bytes
	u16 field_22; // 2 bytes
	u8 field_23; // 1 bytes
	i32 field_24; // 4 bytes
	i32 field_25; // 4 bytes
	i32 field_26; // 4 bytes
	i32 field_27; // 4 bytes
	u8 field_28; // 1 bytes
	// handler &LAB_00a1d7c9
};
POP_PACKED
ASSERT_SIZE(SA_PartyJoin, 98);

PUSH_PACKED
struct SN_PartyJoin
{
	enum { NET_ID = 62188 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	u8 field_2; // 1 bytes
	i32 field_3; // 4 bytes
	u8 field_4; // 1 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	u8 field_8; // 1 bytes
	// handler &LAB_00a25cd6
};
POP_PACKED

PUSH_PACKED
struct SA_PartyLeave
{
	enum { NET_ID = 62189 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1d900
};
POP_PACKED
ASSERT_SIZE(SA_PartyLeave, 4);

PUSH_PACKED
struct SN_PartyLeave
{
	enum { NET_ID = 62190 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	u8 field_2; // 1 bytes
	// handler &LAB_00a25de4
};
POP_PACKED

PUSH_PACKED
struct SA_PartyAddBot
{
	enum { NET_ID = 62191 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1d5bc
};
POP_PACKED
ASSERT_SIZE(SA_PartyAddBot, 4);

PUSH_PACKED
struct SA_PartyRemoveBot
{
	enum { NET_ID = 62192 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1d9a8
};
POP_PACKED
ASSERT_SIZE(SA_PartyRemoveBot, 4);

PUSH_PACKED
struct SA_PartyBreakup
{
	enum { NET_ID = 62193 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a1d5ec
};
POP_PACKED
ASSERT_SIZE(SA_PartyBreakup, 8);

PUSH_PACKED
struct SN_PartyBreakup
{
	enum { NET_ID = 62194 };
	// payloadless: client validator consumes without field checks, logger dumps no fields,
	// and real captures show size=4 (NetHeader only). Send with no payload.
	// handler FUN_00a25a3f
};
POP_PACKED

PUSH_PACKED
struct SN_PartyKicked
{
	enum { NET_ID = 62195 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a25daa
};
POP_PACKED
ASSERT_SIZE(SN_PartyKicked, 8);

PUSH_PACKED
struct SA_PartyKickAll
{
	enum { NET_ID = 62196 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1d8d0
};
POP_PACKED
ASSERT_SIZE(SA_PartyKickAll, 4);

PUSH_PACKED
struct SA_QuickRunArena
{
	enum { NET_ID = 62197 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	u8 field_5; // 1 bytes
	// handler &LAB_00a1dd8d
};
POP_PACKED
ASSERT_SIZE(SA_QuickRunArena, 19);

PUSH_PACKED
struct SN_EnqueueTrollPenalty
{
	enum { NET_ID = 62198 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	i32 field_2; // 4 bytes
	// handler FUN_00a21c60
};
POP_PACKED
ASSERT_SIZE(SN_EnqueueTrollPenalty, 10);

PUSH_PACKED
struct SA_EnqueueTrollPenaltyCancel
{
	enum { NET_ID = 62199 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1b378
};
POP_PACKED
ASSERT_SIZE(SA_EnqueueTrollPenaltyCancel, 4);

PUSH_PACKED
struct SN_EnqueueTrollPenaltyCancel
{
	enum { NET_ID = 62200 };
	// payloadless: client validator consumes without field checks, logger dumps no fields,
	// and real captures show size=4 (NetHeader only). Send with no payload.
	// handler FUN_00a21caa
};
POP_PACKED

PUSH_PACKED
struct SA_CancelMatchingQueue
{
	enum { NET_ID = 62202 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1a7ba
};
POP_PACKED
ASSERT_SIZE(SA_CancelMatchingQueue, 4);

PUSH_PACKED
struct SN_CancelMatchingQueue
{
	enum { NET_ID = 62203 };
	u8 field_0; // 1 bytes
	// handler FUN_00a206ff
};
POP_PACKED
ASSERT_SIZE(SN_CancelMatchingQueue, 1);

PUSH_PACKED
struct SN_MatchingPartyConfirm
{
	enum { NET_ID = 62206 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	u8 field_2; // 1 bytes
	// handler FUN_00a2515d
};
POP_PACKED
ASSERT_SIZE(SN_MatchingPartyConfirm, 9);

PUSH_PACKED
struct SN_ChannelChattingChannel
{
	enum { NET_ID = 62207 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_2; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a20a37
};
POP_PACKED

PUSH_PACKED
struct SA_MasterUnpick
{
	enum { NET_ID = 62210 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a1d17d
};
POP_PACKED
ASSERT_SIZE(SA_MasterUnpick, 8);

PUSH_PACKED
struct SN_MasterUnpick
{
	enum { NET_ID = 62211 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	// handler &LAB_00a2509b
};
POP_PACKED
ASSERT_SIZE(SN_MasterUnpick, 20);

PUSH_PACKED
struct SA_MasterUnpickAll
{
	enum { NET_ID = 62212 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1d1b7
};
POP_PACKED
ASSERT_SIZE(SA_MasterUnpickAll, 4);

PUSH_PACKED
struct SN_MasterUnpickAll
{
	enum { NET_ID = 62213 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	// handler &LAB_00a250fc
};
POP_PACKED
ASSERT_SIZE(SN_MasterUnpickAll, 20);

PUSH_PACKED
struct SA_SortieRoomBreakup
{
	enum { NET_ID = 62215 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1ec07
};
POP_PACKED
ASSERT_SIZE(SA_SortieRoomBreakup, 4);

PUSH_PACKED
struct SN_SortieRoomBreakup
{
	enum { NET_ID = 62216 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a28402
};
POP_PACKED
ASSERT_SIZE(SN_SortieRoomBreakup, 8);

PUSH_PACKED
struct SA_WeaponUpgrade
{
	enum { NET_ID = 62220 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	u8 field_5; // 1 bytes
	u8 field_6; // 1 bytes
	// handler FUN_00a1f275
};
POP_PACKED
ASSERT_SIZE(SA_WeaponUpgrade, 22);

PUSH_PACKED
struct SA_InventoryExpand
{
	enum { NET_ID = 62221 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	// handler FUN_00a1ca66
};
POP_PACKED
ASSERT_SIZE(SA_InventoryExpand, 12);

PUSH_PACKED
struct SN_DespawnMonsters
{
	enum { NET_ID = 62222 };
	i32 field_0; // 4 bytes
	// handler &LAB_00a217c4
};
POP_PACKED
ASSERT_SIZE(SN_DespawnMonsters, 4);

PUSH_PACKED
struct SN_DespawnMonster
{
	enum { NET_ID = 62223 };
	i32 field_0; // 4 bytes
	// handler FUN_00a21794
};
POP_PACKED
ASSERT_SIZE(SN_DespawnMonster, 4);

PUSH_PACKED
struct SN_UpdateAihost
{
	enum { NET_ID = 62225 };
	u8 field_0; // 1 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	// handler FUN_00a29028
};
POP_PACKED
ASSERT_SIZE(SN_UpdateAihost, 9);

PUSH_PACKED
struct SA_RequestSummaryInfoEach
{
	enum { NET_ID = 62226 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	u8 field_5; // 1 bytes
	i64 field_6; // 8 bytes
	// handler &LAB_00a1df2c
};
POP_PACKED
ASSERT_SIZE(SA_RequestSummaryInfoEach, 29);

PUSH_PACKED
struct SN_SummaryUpdate
{
	enum { NET_ID = 62228 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	u8 field_4; // 1 bytes
	i64 field_5; // 8 bytes
	// handler FUN_00a28a15
};
POP_PACKED
ASSERT_SIZE(SN_SummaryUpdate, 25);

PUSH_PACKED
struct SN_UNKNOWN_62230
{
	enum { NET_ID = 62230 };
	// (0-byte check)
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	i32 field_8; // 4 bytes
	i32 field_9; // 4 bytes
	i32 field_10; // 4 bytes
	// (0-byte check)
	i32 field_11; // 4 bytes
	i32 field_12; // 4 bytes
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	i32 field_13; // 4 bytes
	i32 field_14; // 4 bytes
	// handler FUN_00a1f7ac
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62230, 60);

PUSH_PACKED
struct SN_ModifyStatus
{
	enum { NET_ID = 62231 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	u8 field_5; // 1 bytes
	u8 field_6; // 1 bytes
	u8 field_7; // 1 bytes
	// handler FUN_00a252cb
};
POP_PACKED
ASSERT_SIZE(SN_ModifyStatus, 20);

PUSH_PACKED
struct SN_RemoveStatus
{
	enum { NET_ID = 62232 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	// handler FUN_00a2757f
};
POP_PACKED
ASSERT_SIZE(SN_RemoveStatus, 12);

PUSH_PACKED
struct SN_EnterUserByTrespass
{
	enum { NET_ID = 62233 };
	i32 field_0; // 4 bytes
	// handler FUN_00a21cfd
};
POP_PACKED
ASSERT_SIZE(SN_EnterUserByTrespass, 4);

PUSH_PACKED
struct SN_LeaveUser
{
	enum { NET_ID = 62234 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	// handler FUN_00a248f3
};
POP_PACKED
ASSERT_SIZE(SN_LeaveUser, 5);

PUSH_PACKED
struct SN_BroadcastDamage
{
	enum { NET_ID = 62235 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	i32 field_8; // 4 bytes
	i32 field_9; // 4 bytes
	i32 field_10; // 4 bytes
	i32 field_11; // 4 bytes
	i32 field_12; // 4 bytes
	i32 field_13; // 4 bytes
	i32 field_14; // 4 bytes
	i32 field_15; // 4 bytes
	i32 field_16; // 4 bytes
	i32 field_17; // 4 bytes
	i32 field_18; // 4 bytes
	i32 field_19; // 4 bytes
	i32 field_20; // 4 bytes
	i32 field_21; // 4 bytes
	i32 field_22; // 4 bytes
	i32 field_23; // 4 bytes
	u16 field_24; // 2 bytes
	// <variable-size field: string/vector>
	i32 field_25; // 4 bytes
	u16 field_26; // 2 bytes
	// <variable-size field: string/vector>
	i32 field_27; // 4 bytes
	i32 field_28; // 4 bytes
	i32 field_29; // 4 bytes
	// handler &LAB_00a202d8
};
POP_PACKED

PUSH_PACKED
struct SN_BroadcastNonRemoteDamage
{
	enum { NET_ID = 62236 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	// handler FUN_00a20553
};
POP_PACKED
ASSERT_SIZE(SN_BroadcastNonRemoteDamage, 28);

PUSH_PACKED
struct SN_RemoteActivated
{
	enum { NET_ID = 62237 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	// handler FUN_00a26fa5
};
POP_PACKED
ASSERT_SIZE(SN_RemoteActivated, 24);

PUSH_PACKED
struct SA_ChannelChat
{
	enum { NET_ID = 62239 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1a8ae
};
POP_PACKED
ASSERT_SIZE(SA_ChannelChat, 4);

PUSH_PACKED
struct SN_ChatChannelJoin
{
	enum { NET_ID = 62240 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_2; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a20dcf
};
POP_PACKED

PUSH_PACKED
struct SN_ChatChannelLeave
{
	enum { NET_ID = 62241 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a20e73
};
POP_PACKED

PUSH_PACKED
struct SN_ChatChannelUserJoin
{
	enum { NET_ID = 62243 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a20f98
};
POP_PACKED

PUSH_PACKED
struct SN_ChatChannelUserLeave
{
	enum { NET_ID = 62244 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a2102c
};
POP_PACKED

PUSH_PACKED
struct SN_FriendAdded
{
	enum { NET_ID = 62245 };
	u16 field_0; // 2 bytes
	i32 field_1; // 4 bytes
	// (0-byte check)
	u16 field_2; // 2 bytes
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	i64 field_3; // 8 bytes
	// handler &LAB_00a2222e
};
POP_PACKED
ASSERT_SIZE(SN_FriendAdded, 16);

PUSH_PACKED
struct SA_FriendRemove
{
	enum { NET_ID = 62246 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a1b547
};
POP_PACKED

PUSH_PACKED
struct SA_CommunityBlockAdd
{
	enum { NET_ID = 62247 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// handler &LAB_00a1ab7b
};
POP_PACKED
ASSERT_SIZE(SA_CommunityBlockAdd, 6);

PUSH_PACKED
struct SA_CommunityBlockRemove
{
	enum { NET_ID = 62248 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// handler &LAB_00a1abf5
};
POP_PACKED
ASSERT_SIZE(SA_CommunityBlockRemove, 6);

PUSH_PACKED
struct SN_FriendRemoved
{
	enum { NET_ID = 62249 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a2246d
};
POP_PACKED

PUSH_PACKED
struct SA_FriendStateChange
{
	enum { NET_ID = 62250 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1b7f0
};
POP_PACKED
ASSERT_SIZE(SA_FriendStateChange, 4);

PUSH_PACKED
struct SN_FriendStateChanged
{
	enum { NET_ID = 62251 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	u8 field_1; // 1 bytes
	u8 field_2; // 1 bytes
	i64 field_3; // 8 bytes
	// handler &LAB_00a22603
};
POP_PACKED

PUSH_PACKED
struct SN_FriendNicknameChanged
{
	enum { NET_ID = 62252 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a2234d
};
POP_PACKED

PUSH_PACKED
struct SA_CommunityStatusMessage
{
	enum { NET_ID = 62253 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1ac9f
};
POP_PACKED
ASSERT_SIZE(SA_CommunityStatusMessage, 4);

PUSH_PACKED
struct SA_CommunityChannelMove
{
	enum { NET_ID = 62254 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1ac6f
};
POP_PACKED
ASSERT_SIZE(SA_CommunityChannelMove, 4);

PUSH_PACKED
struct SN_CommunityStatusMessage
{
	enum { NET_ID = 62255 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a21222
};
POP_PACKED

PUSH_PACKED
struct SN_FriendLeaderChanged
{
	enum { NET_ID = 62256 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	i32 field_1; // 4 bytes
	// handler &LAB_00a2228a
};
POP_PACKED

PUSH_PACKED
struct SN_RecommendedfriendList
{
	enum { NET_ID = 62260 };
	u16 field_0; // 2 bytes
	i32 field_1; // 4 bytes
	// handler &LAB_00a26d34
};
POP_PACKED
ASSERT_SIZE(SN_RecommendedfriendList, 6);

PUSH_PACKED
struct SA_FriendRequest
{
	enum { NET_ID = 62262 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a1b5ba
};
POP_PACKED

PUSH_PACKED
struct SN_FriendRequested
{
	enum { NET_ID = 62263 };
	u16 field_0; // 2 bytes
	i32 field_1; // 4 bytes
	// handler &LAB_00a224d0
};
POP_PACKED
ASSERT_SIZE(SN_FriendRequested, 6);

PUSH_PACKED
struct SA_FriendRespond
{
	enum { NET_ID = 62264 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	u8 field_2; // 1 bytes
	// handler &LAB_00a1b68c
};
POP_PACKED

PUSH_PACKED
struct SN_FriendResponded
{
	enum { NET_ID = 62265 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	u8 field_1; // 1 bytes
	// handler &LAB_00a22595
};
POP_PACKED

PUSH_PACKED
struct SA_FriendRequestLeaderInfo
{
	enum { NET_ID = 62266 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	u16 field_2; // 2 bytes
	u16 field_3; // 2 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	u16 field_8; // 2 bytes
	i32 field_9; // 4 bytes
	i32 field_10; // 4 bytes
	u8 field_11; // 1 bytes
	u8 field_12; // 1 bytes
	u16 field_13; // 2 bytes
	i32 field_14; // 4 bytes
	i32 field_15; // 4 bytes
	// handler &LAB_00a1b62d
};
POP_PACKED
ASSERT_SIZE(SA_FriendRequestLeaderInfo, 48);

PUSH_PACKED
struct SA_RequestLeaderInfo
{
	enum { NET_ID = 62267 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	u16 field_2; // 2 bytes
	u16 field_3; // 2 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	u16 field_8; // 2 bytes
	i32 field_9; // 4 bytes
	i32 field_10; // 4 bytes
	u8 field_11; // 1 bytes
	u8 field_12; // 1 bytes
	u16 field_13; // 2 bytes
	i32 field_14; // 4 bytes
	i32 field_15; // 4 bytes
	// handler &LAB_00a1de7f
};
POP_PACKED
ASSERT_SIZE(SA_RequestLeaderInfo, 48);

PUSH_PACKED
struct SA_FriendSetComrade
{
	enum { NET_ID = 62268 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a1b70a
};
POP_PACKED

PUSH_PACKED
struct SA_FriendUnsetComrade
{
	enum { NET_ID = 62269 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a1b820
};
POP_PACKED

PUSH_PACKED
struct SA_FriendSetFavorite
{
	enum { NET_ID = 62270 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a1b77d
};
POP_PACKED

PUSH_PACKED
struct SA_FriendUnsetFavorite
{
	enum { NET_ID = 62271 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a1b893
};
POP_PACKED

PUSH_PACKED
struct SN_FriendPartycreation
{
	enum { NET_ID = 62272 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	i32 field_1; // 4 bytes
	u8 field_2; // 1 bytes
	u8 field_3; // 1 bytes
	// handler &LAB_00a223e1
};
POP_PACKED

PUSH_PACKED
struct SA_RequestMissionReward
{
	enum { NET_ID = 62273 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i64 field_2; // 8 bytes
	u16 field_3; // 2 bytes
	u8 field_4; // 1 bytes
	u8 field_5; // 1 bytes
	// handler FUN_00a1dede
};
POP_PACKED
ASSERT_SIZE(SA_RequestMissionReward, 20);

PUSH_PACKED
struct SN_PvpAvailableReportCount
{
	enum { NET_ID = 62274 };
	u8 field_0; // 1 bytes
	u8 field_1; // 1 bytes
	// handler FUN_00a2699c
};
POP_PACKED
ASSERT_SIZE(SN_PvpAvailableReportCount, 2);

PUSH_PACKED
struct SN_ReturnRoomExpirationCountdown
{
	enum { NET_ID = 62275 };
	// payloadless: client validator consumes without field checks, logger dumps no fields,
	// and real captures show size=4 (NetHeader only). Send with no payload.
	// handler FUN_00a27966
};
POP_PACKED

PUSH_PACKED
struct SN_GlobalNotice
{
	enum { NET_ID = 62277 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	u8 field_2; // 1 bytes
	u16 field_3; // 2 bytes
	// handler &LAB_00a232c1
};
POP_PACKED
ASSERT_SIZE(SN_GlobalNotice, 9);

PUSH_PACKED
struct SN_ExpGain
{
	enum { NET_ID = 62279 };
	i32 field_0; // 4 bytes
	// handler FUN_00a21f87
};
POP_PACKED
ASSERT_SIZE(SN_ExpGain, 4);

PUSH_PACKED
struct SA_GetUserinfo
{
	enum { NET_ID = 62280 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_2; // 2 bytes
	i32 field_3; // 4 bytes
	u8 field_4; // 1 bytes
	u16 field_5; // 2 bytes
	// <variable-size field: string/vector>
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	i32 field_8; // 4 bytes
	u8 field_9; // 1 bytes
	i32 field_10; // 4 bytes
	u8 field_11; // 1 bytes
	u8 field_12; // 1 bytes
	u8 field_13; // 1 bytes
	i32 field_14; // 4 bytes
	u8 field_15; // 1 bytes
	u8 field_16; // 1 bytes
	i32 field_17; // 4 bytes
	u8 field_18; // 1 bytes
	i32 field_19; // 4 bytes
	i32 field_20; // 4 bytes
	i32 field_21; // 4 bytes
	i32 field_22; // 4 bytes
	u8 field_23; // 1 bytes
	i64 field_24; // 8 bytes
	u16 field_25; // 2 bytes
	u8 field_26; // 1 bytes
	i32 field_27; // 4 bytes
	u8 field_28; // 1 bytes
	i32 field_29; // 4 bytes
	u8 field_30; // 1 bytes
	// handler &LAB_00a1c706
};
POP_PACKED

PUSH_PACKED
struct SA_JukeboxEnqueue
{
	enum { NET_ID = 62281 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1cebd
};
POP_PACKED
ASSERT_SIZE(SA_JukeboxEnqueue, 4);

PUSH_PACKED
struct SN_OpenEventMasters
{
	enum { NET_ID = 62286 };
	i32 field_0; // 4 bytes
	// handler &LAB_00a259ec
};
POP_PACKED
ASSERT_SIZE(SN_OpenEventMasters, 4);

PUSH_PACKED
struct SA_CreateGuild
{
	enum { NET_ID = 62287 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_2; // 2 bytes
	// <variable-size field: string/vector>
	// (0-byte check)
	u16 field_3; // 2 bytes
	i32 field_4; // 4 bytes
	// handler &LAB_00a1acff
};
POP_PACKED

PUSH_PACKED
struct SA_DissolveGuild
{
	enum { NET_ID = 62288 };
	i32 field_0; // 4 bytes
	i64 field_1; // 8 bytes
	// handler FUN_00a1b09e
};
POP_PACKED
ASSERT_SIZE(SA_DissolveGuild, 12);

PUSH_PACKED
struct SA_CancelGuilddissolution
{
	enum { NET_ID = 62289 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1a78a
};
POP_PACKED
ASSERT_SIZE(SA_CancelGuilddissolution, 4);

PUSH_PACKED
struct SN_UNKNOWN_62290
{
	enum { NET_ID = 62290 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1accf
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62290, 4);

PUSH_PACKED
struct SA_QuitGuild
{
	enum { NET_ID = 62291 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1de05
};
POP_PACKED
ASSERT_SIZE(SA_QuitGuild, 4);

PUSH_PACKED
struct SA_KickGuildmember
{
	enum { NET_ID = 62292 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a1ceed
};
POP_PACKED

PUSH_PACKED
struct SA_JoinGuild
{
	enum { NET_ID = 62293 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	u8 field_2; // 1 bytes
	// handler &LAB_00a1ce3f
};
POP_PACKED

PUSH_PACKED
struct SA_GetGuildjoinrequestlist
{
	enum { NET_ID = 62294 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	u16 field_2; // 2 bytes
	u16 field_3; // 2 bytes
	u16 field_4; // 2 bytes
	i32 field_5; // 4 bytes
	u8 field_6; // 1 bytes
	u16 field_7; // 2 bytes
	// handler &LAB_00a1bc20
};
POP_PACKED
ASSERT_SIZE(SA_GetGuildjoinrequestlist, 19);

PUSH_PACKED
struct SA_RespondGuildjoinrequest
{
	enum { NET_ID = 62295 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	u16 field_2; // 2 bytes
	i32 field_3; // 4 bytes
	u16 field_4; // 2 bytes
	u16 field_5; // 2 bytes
	u16 field_6; // 2 bytes
	i32 field_7; // 4 bytes
	u8 field_8; // 1 bytes
	u16 field_9; // 2 bytes
	i32 field_10; // 4 bytes
	i32 field_11; // 4 bytes
	u16 field_12; // 2 bytes
	u16 field_13; // 2 bytes
	i64 field_14; // 8 bytes
	// handler &LAB_00a1e0f7
};
POP_PACKED
ASSERT_SIZE(SA_RespondGuildjoinrequest, 44);

PUSH_PACKED
struct SA_InviteGuildmember
{
	enum { NET_ID = 62296 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a1cab0
};
POP_PACKED

PUSH_PACKED
struct SA_GetGuildinvitationlist
{
	enum { NET_ID = 62297 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	u16 field_2; // 2 bytes
	// handler &LAB_00a1bbbf
};
POP_PACKED
ASSERT_SIZE(SA_GetGuildinvitationlist, 8);

PUSH_PACKED
struct SA_RespondGuildinvitation
{
	enum { NET_ID = 62298 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	u16 field_2; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_3; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a1e045
};
POP_PACKED

PUSH_PACKED
struct SA_GetGuildemblemlist
{
	enum { NET_ID = 62301 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler &LAB_00a1bafb
};
POP_PACKED
ASSERT_SIZE(SA_GetGuildemblemlist, 8);

PUSH_PACKED
struct SA_GetGuildpublicprofile
{
	enum { NET_ID = 62303 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_2; // 2 bytes
	// <variable-size field: string/vector>
	i32 field_3; // 4 bytes
	u8 field_4; // 1 bytes
	u8 field_5; // 1 bytes
	u16 field_6; // 2 bytes
	// <variable-size field: string/vector>
	i64 field_7; // 8 bytes
	i64 field_8; // 8 bytes
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	u16 field_9; // 2 bytes
	// <variable-size field: string/vector>
	// (0-byte check)
	// handler &LAB_00a1c01c
};
POP_PACKED

PUSH_PACKED
struct SA_GetGuildpublicprofileFromUser
{
	enum { NET_ID = 62304 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_2; // 2 bytes
	// <variable-size field: string/vector>
	i32 field_3; // 4 bytes
	u8 field_4; // 1 bytes
	u8 field_5; // 1 bytes
	u16 field_6; // 2 bytes
	// <variable-size field: string/vector>
	i64 field_7; // 8 bytes
	i64 field_8; // 8 bytes
	u8 field_9; // 1 bytes
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	u16 field_10; // 2 bytes
	// <variable-size field: string/vector>
	u8 field_11; // 1 bytes
	u16 field_12; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a1c198
};
POP_PACKED

PUSH_PACKED
struct SA_EditGuildnotice
{
	enum { NET_ID = 62305 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a1b247
};
POP_PACKED

PUSH_PACKED
struct SA_EditGuildintro
{
	enum { NET_ID = 62306 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a1b130
};
POP_PACKED

PUSH_PACKED
struct SA_EditGuildtag
{
	enum { NET_ID = 62307 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	// handler &LAB_00a1b2ba
};
POP_PACKED

PUSH_PACKED
struct SA_SetGuildinterest
{
	enum { NET_ID = 62308 };
	i32 field_0; // 4 bytes
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// handler FUN_00a1e72e
};
POP_PACKED
ASSERT_SIZE(SA_SetGuildinterest, 4);

PUSH_PACKED
struct SA_SetGuildemblem
{
	enum { NET_ID = 62309 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a1e6f4
};
POP_PACKED
ASSERT_SIZE(SA_SetGuildemblem, 8);

PUSH_PACKED
struct SA_SetGuildjointype
{
	enum { NET_ID = 62310 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	// handler FUN_00a1e776
};
POP_PACKED
ASSERT_SIZE(SA_SetGuildjointype, 5);

PUSH_PACKED
struct SA_SetGuildmemberclass
{
	enum { NET_ID = 62311 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	u16 field_4; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a1e833
};
POP_PACKED

PUSH_PACKED
struct SA_SetGuildmaster
{
	enum { NET_ID = 62312 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	u16 field_2; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a1e7b0
};
POP_PACKED

PUSH_PACKED
struct SA_CreateGuildmembership
{
	enum { NET_ID = 62313 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	u8 field_2; // 1 bytes
	u16 field_3; // 2 bytes
	// <variable-size field: string/vector>
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// handler &LAB_00a1adce
};
POP_PACKED

PUSH_PACKED
struct SA_DeleteGuildmembership
{
	enum { NET_ID = 62314 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	u16 field_3; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_4; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a1afe8
};
POP_PACKED

PUSH_PACKED
struct SA_EditGuildmembership
{
	enum { NET_ID = 62315 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	u8 field_2; // 1 bytes
	u16 field_3; // 2 bytes
	// <variable-size field: string/vector>
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// handler &LAB_00a1b1a3
};
POP_PACKED

PUSH_PACKED
struct SA_DonateToGuild
{
	enum { NET_ID = 62316 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	// handler FUN_00a1b0d8
};
POP_PACKED
ASSERT_SIZE(SA_DonateToGuild, 16);

PUSH_PACKED
struct SN_UNKNOWN_62317
{
	enum { NET_ID = 62317 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	i32 field_2; // 4 bytes
	// handler FUN_00a1a4ed
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62317, 9);

PUSH_PACKED
struct SN_UNKNOWN_62318
{
	enum { NET_ID = 62318 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	i32 field_2; // 4 bytes
	// handler FUN_00a1a4a5
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62318, 9);

PUSH_PACKED
struct SA_GetGuildskilllist
{
	enum { NET_ID = 62319 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	u8 field_2; // 1 bytes
	i64 field_3; // 8 bytes
	u16 field_4; // 2 bytes
	// handler &LAB_00a1c5ba
};
POP_PACKED
ASSERT_SIZE(SA_GetGuildskilllist, 16);

PUSH_PACKED
struct SA_BuyGuildskill
{
	enum { NET_ID = 62320 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	u8 field_2; // 1 bytes
	i64 field_3; // 8 bytes
	u16 field_4; // 2 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	// handler FUN_00a1a669
};
POP_PACKED
ASSERT_SIZE(SA_BuyGuildskill, 24);

PUSH_PACKED
struct SA_ExtendGuildlimitedskill
{
	enum { NET_ID = 62321 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	u8 field_2; // 1 bytes
	u16 field_3; // 2 bytes
	i64 field_4; // 8 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	// handler FUN_00a1b44a
};
POP_PACKED
ASSERT_SIZE(SA_ExtendGuildlimitedskill, 24);

PUSH_PACKED
struct SA_GetGuildrank
{
	enum { NET_ID = 62323 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	u16 field_2; // 2 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	u16 field_5; // 2 bytes
	u16 field_6; // 2 bytes
	u16 field_7; // 2 bytes
	i32 field_8; // 4 bytes
	// handler &LAB_00a1c3b4
};
POP_PACKED
ASSERT_SIZE(SA_GetGuildrank, 25);

PUSH_PACKED
struct SA_GetGuildranklist
{
	enum { NET_ID = 62324 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	u16 field_2; // 2 bytes
	i32 field_3; // 4 bytes
	u8 field_4; // 1 bytes
	i32 field_5; // 4 bytes
	i64 field_6; // 8 bytes
	i32 field_7; // 4 bytes
	i32 field_8; // 4 bytes
	u16 field_9; // 2 bytes
	u16 field_10; // 2 bytes
	u16 field_11; // 2 bytes
	i32 field_12; // 4 bytes
	// handler &LAB_00a1c49c
};
POP_PACKED
ASSERT_SIZE(SA_GetGuildranklist, 42);

PUSH_PACKED
struct SA_GetGuildrankrewardinfo
{
	enum { NET_ID = 62325 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	u16 field_4; // 2 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	// handler &LAB_00a1c54c
};
POP_PACKED
ASSERT_SIZE(SA_GetGuildrankrewardinfo, 27);

PUSH_PACKED
struct SA_GetRecommendedguildlist
{
	enum { NET_ID = 62326 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	u16 field_2; // 2 bytes
	i32 field_3; // 4 bytes
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// handler &LAB_00a1c6a5
};
POP_PACKED
ASSERT_SIZE(SA_GetRecommendedguildlist, 12);

PUSH_PACKED
struct SA_SearchGuild
{
	enum { NET_ID = 62327 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	u8 field_2; // 1 bytes
	i32 field_3; // 4 bytes
	u16 field_4; // 2 bytes
	u16 field_5; // 2 bytes
	i32 field_6; // 4 bytes
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// handler &LAB_00a1e4ea
};
POP_PACKED
ASSERT_SIZE(SA_SearchGuild, 21);

PUSH_PACKED
struct SA_GetPlayingGuildpvplist
{
	enum { NET_ID = 62328 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	u8 field_2; // 1 bytes
	u16 field_3; // 2 bytes
	i32 field_4; // 4 bytes
	i64 field_5; // 8 bytes
	i32 field_6; // 4 bytes
	u16 field_7; // 2 bytes
	i32 field_8; // 4 bytes
	u16 field_9; // 2 bytes
	u16 field_10; // 2 bytes
	i32 field_11; // 4 bytes
	u16 field_12; // 2 bytes
	i32 field_13; // 4 bytes
	u16 field_14; // 2 bytes
	u16 field_15; // 2 bytes
	// handler &LAB_00a1c61b
};
POP_PACKED
ASSERT_SIZE(SA_GetPlayingGuildpvplist, 49);

PUSH_PACKED
struct SA_WatchGuildpvp
{
	enum { NET_ID = 62329 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a1f195
};
POP_PACKED
ASSERT_SIZE(SA_WatchGuildpvp, 8);

PUSH_PACKED
struct SN_UNKNOWN_62331
{
	enum { NET_ID = 62331 };
	u8 field_0; // 1 bytes
	u8 field_1; // 1 bytes
	i64 field_2; // 8 bytes
	u16 field_3; // 2 bytes
	// handler &LAB_00a24233
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62331, 12);

PUSH_PACKED
struct SN_GuildInvitation
{
	enum { NET_ID = 62332 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a24070
};
POP_PACKED

PUSH_PACKED
struct SN_GuildJoinapproval
{
	enum { NET_ID = 62333 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a24104
};
POP_PACKED

PUSH_PACKED
struct SN_GuildtagChanged
{
	enum { NET_ID = 62334 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	// handler &LAB_00a23ff0
};
POP_PACKED

PUSH_PACKED
struct SN_GuildNotice
{
	enum { NET_ID = 62335 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a241d0
};
POP_PACKED

PUSH_PACKED
struct SN_GuildmemberJoin
{
	enum { NET_ID = 62336 };
	u16 field_0; // 2 bytes
	i32 field_1; // 4 bytes
	u16 field_2; // 2 bytes
	u16 field_3; // 2 bytes
	u16 field_4; // 2 bytes
	i32 field_5; // 4 bytes
	u8 field_6; // 1 bytes
	u16 field_7; // 2 bytes
	i32 field_8; // 4 bytes
	i32 field_9; // 4 bytes
	u16 field_10; // 2 bytes
	u16 field_11; // 2 bytes
	i64 field_12; // 8 bytes
	// handler &LAB_00a23ac8
};
POP_PACKED
ASSERT_SIZE(SN_GuildmemberJoin, 39);

PUSH_PACKED
struct SN_GuildmemberQuit
{
	enum { NET_ID = 62337 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	u8 field_1; // 1 bytes
	// handler &LAB_00a23b24
};
POP_PACKED

PUSH_PACKED
struct SN_GuildmemberChgnickname
{
	enum { NET_ID = 62339 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a23a34
};
POP_PACKED

PUSH_PACKED
struct SN_GuildmemberChgleaderclass
{
	enum { NET_ID = 62340 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_1; // 2 bytes
	// handler &LAB_00a239c4
};
POP_PACKED

PUSH_PACKED
struct SN_GuildmembershipAdded
{
	enum { NET_ID = 62341 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	u16 field_2; // 2 bytes
	// <variable-size field: string/vector>
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// handler &LAB_00a237b4
};
POP_PACKED

PUSH_PACKED
struct SN_GuildmembershipRemoved
{
	enum { NET_ID = 62342 };
	i32 field_0; // 4 bytes
	// handler FUN_00a238e0
};
POP_PACKED
ASSERT_SIZE(SN_GuildmembershipRemoved, 4);

PUSH_PACKED
struct SN_GuildmembershipModified
{
	enum { NET_ID = 62343 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	u16 field_2; // 2 bytes
	// <variable-size field: string/vector>
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// handler &LAB_00a2384a
};
POP_PACKED

PUSH_PACKED
struct SN_GuildmemberChgclass
{
	enum { NET_ID = 62344 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	u16 field_2; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_3; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a23910
};
POP_PACKED

PUSH_PACKED
struct SN_GuildmasterDelegation
{
	enum { NET_ID = 62345 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	i32 field_1; // 4 bytes
	u16 field_2; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a23711
};
POP_PACKED

PUSH_PACKED
struct SN_GuildfundAdded
{
	enum { NET_ID = 62346 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	// handler &LAB_00a23661
};
POP_PACKED

PUSH_PACKED
struct SN_GuildemblemChange
{
	enum { NET_ID = 62347 };
	i32 field_0; // 4 bytes
	// handler FUN_00a23631
};
POP_PACKED
ASSERT_SIZE(SN_GuildemblemChange, 4);

PUSH_PACKED
struct SN_GuildemblemAdded
{
	enum { NET_ID = 62348 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	// handler FUN_00a235f7
};
POP_PACKED
ASSERT_SIZE(SN_GuildemblemAdded, 5);

PUSH_PACKED
struct SN_UNKNOWN_62349
{
	enum { NET_ID = 62349 };
	u8 field_0; // 1 bytes
	// handler FUN_00a236e1
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62349, 1);

PUSH_PACKED
struct SN_GuildskillAcquired
{
	enum { NET_ID = 62350 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	u8 field_1; // 1 bytes
	u8 field_2; // 1 bytes
	u16 field_3; // 2 bytes
	i64 field_4; // 8 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	// handler &LAB_00a23dae
};
POP_PACKED

PUSH_PACKED
struct SN_GuildskillUpgraded
{
	enum { NET_ID = 62351 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	u8 field_1; // 1 bytes
	u8 field_2; // 1 bytes
	u16 field_3; // 2 bytes
	i64 field_4; // 8 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	// handler &LAB_00a23f34
};
POP_PACKED

PUSH_PACKED
struct SN_GuildskillExtended
{
	enum { NET_ID = 62352 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	u8 field_1; // 1 bytes
	u8 field_2; // 1 bytes
	u16 field_3; // 2 bytes
	u16 field_4; // 2 bytes
	i64 field_5; // 8 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	// handler &LAB_00a23e6a
};
POP_PACKED

PUSH_PACKED
struct SN_GuildconstraintChange
{
	enum { NET_ID = 62353 };
	i64 field_0; // 8 bytes
	u8 field_1; // 1 bytes
	u8 field_2; // 1 bytes
	u8 field_3; // 1 bytes
	u16 field_4; // 2 bytes
	u16 field_5; // 2 bytes
	// handler FUN_00a235af
};
POP_PACKED
ASSERT_SIZE(SN_GuildconstraintChange, 15);

PUSH_PACKED
struct SN_GuildmissionCompleted
{
	enum { NET_ID = 62354 };
	u8 field_0; // 1 bytes
	i32 field_1; // 4 bytes
	u16 field_2; // 2 bytes
	u16 field_3; // 2 bytes
	u16 field_4; // 2 bytes
	u16 field_5; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_6; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a23cda
};
POP_PACKED

PUSH_PACKED
struct SN_GuildpvpResult
{
	enum { NET_ID = 62355 };
	u8 field_0; // 1 bytes
	i32 field_1; // 4 bytes
	u16 field_2; // 2 bytes
	u16 field_3; // 2 bytes
	u16 field_4; // 2 bytes
	u16 field_5; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_6; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a23cda
};
POP_PACKED

PUSH_PACKED
struct SN_GuildpointandfundChange
{
	enum { NET_ID = 62356 };
	i64 field_0; // 8 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	// handler FUN_00a23c80
};
POP_PACKED
ASSERT_SIZE(SN_GuildpointandfundChange, 20);

PUSH_PACKED
struct SN_GuildLevelup
{
	enum { NET_ID = 62357 };
	u8 field_0; // 1 bytes
	u8 field_1; // 1 bytes
	// handler FUN_00a24198
};
POP_PACKED
ASSERT_SIZE(SN_GuildLevelup, 2);

PUSH_PACKED
struct SN_GuildchannelLeave
{
	enum { NET_ID = 62359 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a2351b
};
POP_PACKED

PUSH_PACKED
struct SN_PlayerBlink
{
	enum { NET_ID = 62362 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	i32 field_8; // 4 bytes
	i32 field_9; // 4 bytes
	i32 field_10; // 4 bytes
	i32 field_11; // 4 bytes
	u8 field_12; // 1 bytes
	// handler FUN_00a25f9e
};
POP_PACKED
ASSERT_SIZE(SN_PlayerBlink, 49);

PUSH_PACKED
struct SN_InvalidRemoteLog
{
	enum { NET_ID = 62363 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a245a3
};
POP_PACKED

PUSH_PACKED
struct SN_SyncActionMove
{
	enum { NET_ID = 62366 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	// handler FUN_00a28b87
};
POP_PACKED
ASSERT_SIZE(SN_SyncActionMove, 20);

PUSH_PACKED
struct SN_SyncStanceType
{
	enum { NET_ID = 62367 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	// handler FUN_00a28bdd
};
POP_PACKED
ASSERT_SIZE(SN_SyncStanceType, 12);

PUSH_PACKED
struct SN_AiSyncMoveMotion
{
	enum { NET_ID = 62368 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	i32 field_8; // 4 bytes
	i32 field_9; // 4 bytes
	// handler FUN_00a1fdd4
};
POP_PACKED
ASSERT_SIZE(SN_AiSyncMoveMotion, 40);

PUSH_PACKED
struct SN_AiSyncBehaviorMotion
{
	enum { NET_ID = 62369 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	u8 field_2; // 1 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	i32 field_8; // 4 bytes
	i32 field_9; // 4 bytes
	i32 field_10; // 4 bytes
	i32 field_11; // 4 bytes
	i32 field_12; // 4 bytes
	i32 field_13; // 4 bytes
	i32 field_14; // 4 bytes
	i32 field_15; // 4 bytes
	i32 field_16; // 4 bytes
	// handler FUN_00a1fbb2
};
POP_PACKED
ASSERT_SIZE(SN_AiSyncBehaviorMotion, 65);

PUSH_PACKED
struct SN_AiSyncCooltime
{
	enum { NET_ID = 62370 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	// handler &LAB_00a1fcf0
};
POP_PACKED

PUSH_PACKED
struct SN_AiSyncSpeedRatio
{
	enum { NET_ID = 62371 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a1ff33
};
POP_PACKED
ASSERT_SIZE(SN_AiSyncSpeedRatio, 8);

PUSH_PACKED
struct SN_AiSyncRotateRatio
{
	enum { NET_ID = 62372 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a1febf
};
POP_PACKED
ASSERT_SIZE(SN_AiSyncRotateRatio, 8);

PUSH_PACKED
struct SN_AiSyncSceneroot
{
	enum { NET_ID = 62373 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a1fef9
};
POP_PACKED
ASSERT_SIZE(SN_AiSyncSceneroot, 8);

PUSH_PACKED
struct SN_AiSyncTarpos
{
	enum { NET_ID = 62374 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	i32 field_8; // 4 bytes
	i32 field_9; // 4 bytes
	i32 field_10; // 4 bytes
	i32 field_11; // 4 bytes
	i32 field_12; // 4 bytes
	i32 field_13; // 4 bytes
	i32 field_14; // 4 bytes
	i32 field_15; // 4 bytes
	i32 field_16; // 4 bytes
	// handler FUN_00a1ffa7
};
POP_PACKED
ASSERT_SIZE(SN_AiSyncTarpos, 68);

PUSH_PACKED
struct SN_AiSyncTarget
{
	enum { NET_ID = 62375 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a1ff6d
};
POP_PACKED
ASSERT_SIZE(SN_AiSyncTarget, 8);

PUSH_PACKED
struct SN_AiAddCom
{
	enum { NET_ID = 62376 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a1f9e0
};
POP_PACKED

PUSH_PACKED
struct SN_AiSyncBodyYaw
{
	enum { NET_ID = 62377 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a1fcb6
};
POP_PACKED
ASSERT_SIZE(SN_AiSyncBodyYaw, 8);

PUSH_PACKED
struct SN_AiSyncBodyPitch
{
	enum { NET_ID = 62378 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a1fc7c
};
POP_PACKED
ASSERT_SIZE(SN_AiSyncBodyPitch, 8);

PUSH_PACKED
struct SN_AiSyncPhy
{
	enum { NET_ID = 62379 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	// handler FUN_00a1fe69
};
POP_PACKED
ASSERT_SIZE(SN_AiSyncPhy, 20);

PUSH_PACKED
struct SN_AiSetTarget
{
	enum { NET_ID = 62380 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a1fb78
};
POP_PACKED
ASSERT_SIZE(SN_AiSetTarget, 8);

PUSH_PACKED
struct SN_AiSyncMonsterSkillTarget
{
	enum { NET_ID = 62381 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	// handler FUN_00a1fd7e
};
POP_PACKED
ASSERT_SIZE(SN_AiSyncMonsterSkillTarget, 20);

PUSH_PACKED
struct SN_UNKNOWN_62382
{
	enum { NET_ID = 62382 };
	u8 field_0; // 1 bytes
	i32 field_1; // 4 bytes
	u16 field_2; // 2 bytes
	u16 field_3; // 2 bytes
	u16 field_4; // 2 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	// handler FUN_00a1fa8d
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62382, 23);

PUSH_PACKED
struct SN_AiSetMovetargetpos
{
	enum { NET_ID = 62383 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	// handler FUN_00a1fb22
};
POP_PACKED
ASSERT_SIZE(SN_AiSetMovetargetpos, 20);

PUSH_PACKED
struct SN_AiChangeMoveType
{
	enum { NET_ID = 62384 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a1fa53
};
POP_PACKED
ASSERT_SIZE(SN_AiChangeMoveType, 8);

PUSH_PACKED
struct SN_RemoteSyncCreateFromCreatorId
{
	enum { NET_ID = 62385 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	u16 field_6; // 2 bytes
	u16 field_7; // 2 bytes
	u16 field_8; // 2 bytes
	i32 field_9; // 4 bytes
	i32 field_10; // 4 bytes
	i32 field_11; // 4 bytes
	u16 field_12; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_13; // 2 bytes
	i32 field_14; // 4 bytes
	i32 field_15; // 4 bytes
	u16 field_16; // 2 bytes
	u16 field_17; // 2 bytes
	u16 field_18; // 2 bytes
	u8 field_19; // 1 bytes
	u8 field_20; // 1 bytes
	u16 field_21; // 2 bytes
	// (0-byte check)
	i32 field_22; // 4 bytes
	// (0-byte check)
	// handler &LAB_00a271f6
};
POP_PACKED

PUSH_PACKED
struct SN_RemoteSyncCreateFromRemoteDoc
{
	enum { NET_ID = 62386 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	u16 field_6; // 2 bytes
	u16 field_7; // 2 bytes
	u16 field_8; // 2 bytes
	i32 field_9; // 4 bytes
	i32 field_10; // 4 bytes
	i32 field_11; // 4 bytes
	u16 field_12; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_13; // 2 bytes
	i32 field_14; // 4 bytes
	i32 field_15; // 4 bytes
	i32 field_16; // 4 bytes
	i32 field_17; // 4 bytes
	u8 field_18; // 1 bytes
	// handler &LAB_00a27371
};
POP_PACKED

PUSH_PACKED
struct SN_RemoteSnapshotFromRemoteDoc
{
	enum { NET_ID = 62387 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	u16 field_6; // 2 bytes
	u16 field_7; // 2 bytes
	u16 field_8; // 2 bytes
	i32 field_9; // 4 bytes
	i32 field_10; // 4 bytes
	i32 field_11; // 4 bytes
	u16 field_12; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_13; // 2 bytes
	i32 field_14; // 4 bytes
	i32 field_15; // 4 bytes
	i32 field_16; // 4 bytes
	i32 field_17; // 4 bytes
	i32 field_18; // 4 bytes
	u8 field_19; // 1 bytes
	// handler &LAB_00a270ba
};
POP_PACKED

PUSH_PACKED
struct SN_UNKNOWN_62388
{
	enum { NET_ID = 62388 };
	u8 field_0; // 1 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	i32 field_8; // 4 bytes
	// handler FUN_00a274f4
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62388, 33);

PUSH_PACKED
struct SN_RemoteSyncForecastCollision
{
	enum { NET_ID = 62389 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	u8 field_6; // 1 bytes
	// handler &LAB_00a274a1
};
POP_PACKED
ASSERT_SIZE(SN_RemoteSyncForecastCollision, 25);

PUSH_PACKED
struct SA_CharacterSkinSelect
{
	enum { NET_ID = 62391 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	// handler FUN_00a1a93e
};
POP_PACKED
ASSERT_SIZE(SA_CharacterSkinSelect, 12);

PUSH_PACKED
struct SN_CharacterSkinUnlock
{
	enum { NET_ID = 62392 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	// handler FUN_00a20ba6
};
POP_PACKED
ASSERT_SIZE(SN_CharacterSkinUnlock, 16);

PUSH_PACKED
struct SN_CallMonsterByRemoteControl
{
	enum { NET_ID = 62393 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	u8 field_2; // 1 bytes
	// handler FUN_00a206b5
};
POP_PACKED
ASSERT_SIZE(SN_CallMonsterByRemoteControl, 9);

PUSH_PACKED
struct SQ_UseLiferecover
{
	enum { NET_ID = 62394 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a29820
};
POP_PACKED
ASSERT_SIZE(SQ_UseLiferecover, 8);

PUSH_PACKED
struct SQ_ReviveByLifeCount
{
	enum { NET_ID = 62395 };
	i32 field_0; // 4 bytes
	// handler FUN_00a297a8
};
POP_PACKED
ASSERT_SIZE(SQ_ReviveByLifeCount, 4);

PUSH_PACKED
struct SQ_ReviveByCoin
{
	enum { NET_ID = 62396 };
	i32 field_0; // 4 bytes
	// handler FUN_00a29778
};
POP_PACKED
ASSERT_SIZE(SQ_ReviveByCoin, 4);

PUSH_PACKED
struct SN_RevivePlayer
{
	enum { NET_ID = 62397 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	u8 field_4; // 1 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	// handler FUN_00a2797f
};
POP_PACKED
ASSERT_SIZE(SN_RevivePlayer, 29);

PUSH_PACKED
struct SN_RevivePlayerAtStartingPoint
{
	enum { NET_ID = 62398 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	// handler FUN_00a279fb
};
POP_PACKED
ASSERT_SIZE(SN_RevivePlayerAtStartingPoint, 24);

PUSH_PACKED
struct SN_RespawnDelaytime
{
	enum { NET_ID = 62399 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a2767a
};
POP_PACKED
ASSERT_SIZE(SN_RespawnDelaytime, 8);

PUSH_PACKED
struct SN_WarnPveGameend
{
	enum { NET_ID = 62401 };
	i32 field_0; // 4 bytes
	// handler FUN_00a2945d
};
POP_PACKED
ASSERT_SIZE(SN_WarnPveGameend, 4);

PUSH_PACKED
struct SN_StartGameNotQualified
{
	enum { NET_ID = 62402 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a28803
};
POP_PACKED

PUSH_PACKED
struct SQ_KickFromGameserver
{
	enum { NET_ID = 62403 };
	i32 field_0; // 4 bytes
	// handler FUN_00a2960f
};
POP_PACKED
ASSERT_SIZE(SQ_KickFromGameserver, 4);

PUSH_PACKED
struct SN_WarehouseItemUpdate
{
	enum { NET_ID = 62405 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	u8 field_6; // 1 bytes
	i64 field_7; // 8 bytes
	u16 field_8; // 2 bytes
	u8 field_9; // 1 bytes
	i32 field_10; // 4 bytes
	u8 field_11; // 1 bytes
	i32 field_12; // 4 bytes
	u8 field_13; // 1 bytes
	// handler &LAB_00a2940a
};
POP_PACKED
ASSERT_SIZE(SN_WarehouseItemUpdate, 43);

PUSH_PACKED
struct SA_WarehouseExpand
{
	enum { NET_ID = 62408 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a1f095
};
POP_PACKED
ASSERT_SIZE(SA_WarehouseExpand, 8);

PUSH_PACKED
struct SA_UserAfk
{
	enum { NET_ID = 62409 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1ef7b
};
POP_PACKED
ASSERT_SIZE(SA_UserAfk, 4);

PUSH_PACKED
struct SN_RewardItemList
{
	enum { NET_ID = 62410 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	u8 field_4; // 1 bytes
	u8 field_5; // 1 bytes
	u8 field_6; // 1 bytes
	i32 field_7; // 4 bytes
	// handler &LAB_00a27a5e
};
POP_PACKED
ASSERT_SIZE(SN_RewardItemList, 23);

PUSH_PACKED
struct SN_RewardStageStart
{
	enum { NET_ID = 62411 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	i32 field_2; // 4 bytes
	u8 field_3; // 1 bytes
	i32 field_4; // 4 bytes
	// handler FUN_00a27b96
};
POP_PACKED
ASSERT_SIZE(SN_RewardStageStart, 14);

PUSH_PACKED
struct SN_RewardStageStartNp
{
	enum { NET_ID = 62412 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	u8 field_2; // 1 bytes
	i32 field_3; // 4 bytes
	// handler FUN_00a27bfc
};
POP_PACKED
ASSERT_SIZE(SN_RewardStageStartNp, 10);

PUSH_PACKED
struct SA_RewardSelectSlot
{
	enum { NET_ID = 62413 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	u8 field_5; // 1 bytes
	// handler FUN_00a1e3b6
};
POP_PACKED
ASSERT_SIZE(SA_RewardSelectSlot, 21);

PUSH_PACKED
struct SA_RewardSelectSlotAllRandom
{
	enum { NET_ID = 62414 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	// handler &LAB_00a1e419
};
POP_PACKED
ASSERT_SIZE(SA_RewardSelectSlotAllRandom, 24);

PUSH_PACKED
struct SN_RewardStageEnd
{
	enum { NET_ID = 62415 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	u16 field_4; // 2 bytes
	u16 field_5; // 2 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	// handler &LAB_00a27b0d
};
POP_PACKED
ASSERT_SIZE(SN_RewardStageEnd, 28);

PUSH_PACKED
struct SA_RewardBuyChance
{
	enum { NET_ID = 62416 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	u8 field_2; // 1 bytes
	i32 field_3; // 4 bytes
	// handler FUN_00a1e32e
};
POP_PACKED
ASSERT_SIZE(SA_RewardBuyChance, 13);

PUSH_PACKED
struct SA_RewardRequestFinish
{
	enum { NET_ID = 62417 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1e386
};
POP_PACKED
ASSERT_SIZE(SA_RewardRequestFinish, 4);

PUSH_PACKED
struct SA_WarehouseItemChange
{
	enum { NET_ID = 62418 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	// handler FUN_00a1f0cf
};
POP_PACKED
ASSERT_SIZE(SA_WarehouseItemChange, 20);

PUSH_PACKED
struct SN_MailList
{
	enum { NET_ID = 62419 };
	u8 field_0; // 1 bytes
	u8 field_1; // 1 bytes
	i64 field_2; // 8 bytes
	i64 field_3; // 8 bytes
	i64 field_4; // 8 bytes
	u8 field_5; // 1 bytes
	u8 field_6; // 1 bytes
	u8 field_7; // 1 bytes
	u16 field_8; // 2 bytes
	u16 field_9; // 2 bytes
	u16 field_10; // 2 bytes
	i32 field_11; // 4 bytes
	// handler &LAB_00a24c53
};
POP_PACKED
ASSERT_SIZE(SN_MailList, 39);

PUSH_PACKED
struct SN_MailBlocklist
{
	enum { NET_ID = 62420 };
	u8 field_0; // 1 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a24b4d
};
POP_PACKED

PUSH_PACKED
struct SN_MailWriteResult
{
	enum { NET_ID = 62421 };
	i32 field_0; // 4 bytes
	// handler FUN_00a24f5b
};
POP_PACKED
ASSERT_SIZE(SN_MailWriteResult, 4);

PUSH_PACKED
struct SN_MailRead
{
	enum { NET_ID = 62422 };
	i64 field_0; // 8 bytes
	u8 field_1; // 1 bytes
	u8 field_2; // 1 bytes
	u16 field_3; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_4; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_5; // 2 bytes
	// <variable-size field: string/vector>
	i64 field_6; // 8 bytes
	i64 field_7; // 8 bytes
	i64 field_8; // 8 bytes
	i64 field_9; // 8 bytes
	i64 field_10; // 8 bytes
	i32 field_11; // 4 bytes
	i32 field_12; // 4 bytes
	i32 field_13; // 4 bytes
	// handler &LAB_00a24d50
};
POP_PACKED

PUSH_PACKED
struct SN_MailGetAttachmentResult
{
	enum { NET_ID = 62423 };
	i32 field_0; // 4 bytes
	i64 field_1; // 8 bytes
	i64 field_2; // 8 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	// handler &LAB_00a24bb0
};
POP_PACKED
ASSERT_SIZE(SN_MailGetAttachmentResult, 36);

PUSH_PACKED
struct SN_MailMoveResult
{
	enum { NET_ID = 62424 };
	i32 field_0; // 4 bytes
	// handler FUN_00a24d20
};
POP_PACKED
ASSERT_SIZE(SN_MailMoveResult, 4);

PUSH_PACKED
struct SN_GameEventActivated
{
	enum { NET_ID = 62427 };
	i32 field_0; // 4 bytes
	// handler &LAB_00a22af2
};
POP_PACKED
ASSERT_SIZE(SN_GameEventActivated, 4);

PUSH_PACKED
struct SN_GameEventPending
{
	enum { NET_ID = 62428 };
	i32 field_0; // 4 bytes
	// handler &LAB_00a22b75
};
POP_PACKED
ASSERT_SIZE(SN_GameEventPending, 4);

PUSH_PACKED
struct SN_GameEventStart
{
	enum { NET_ID = 62429 };
	i32 field_0; // 4 bytes
	// handler FUN_00a22bc8
};
POP_PACKED
ASSERT_SIZE(SN_GameEventStart, 4);

PUSH_PACKED
struct SN_GameEventEnd
{
	enum { NET_ID = 62430 };
	i32 field_0; // 4 bytes
	// handler FUN_00a22b45
};
POP_PACKED
ASSERT_SIZE(SN_GameEventEnd, 4);

PUSH_PACKED
struct SN_PartyCancelInvite
{
	enum { NET_ID = 62431 };
	i32 field_0; // 4 bytes
	// handler FUN_00a25a58
};
POP_PACKED
ASSERT_SIZE(SN_PartyCancelInvite, 4);

PUSH_PACKED
struct SN_ActivatedSupportkit
{
	enum { NET_ID = 62432 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	// handler FUN_00a1f6d1
};
POP_PACKED
ASSERT_SIZE(SN_ActivatedSupportkit, 16);

PUSH_PACKED
struct SN_NotifyCooltime
{
	enum { NET_ID = 62433 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	// handler &LAB_00a257b6
};
POP_PACKED
ASSERT_SIZE(SN_NotifyCooltime, 20);

PUSH_PACKED
struct SN_MailListNewInbox
{
	enum { NET_ID = 62434 };
	u8 field_0; // 1 bytes
	i64 field_1; // 8 bytes
	i64 field_2; // 8 bytes
	i64 field_3; // 8 bytes
	u8 field_4; // 1 bytes
	u8 field_5; // 1 bytes
	u8 field_6; // 1 bytes
	u16 field_7; // 2 bytes
	u16 field_8; // 2 bytes
	u16 field_9; // 2 bytes
	i32 field_10; // 4 bytes
	// handler &LAB_00a24cc0
};
POP_PACKED
ASSERT_SIZE(SN_MailListNewInbox, 38);

PUSH_PACKED
struct SN_PlayerServerPosition
{
	enum { NET_ID = 62435 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	// handler FUN_00a2604a
};
POP_PACKED
ASSERT_SIZE(SN_PlayerServerPosition, 20);

PUSH_PACKED
struct SN_RemoteServerPosition
{
	enum { NET_ID = 62436 };
	i64 field_0; // 8 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	u8 field_7; // 1 bytes
	u8 field_8; // 1 bytes
	u8 field_9; // 1 bytes
	i32 field_10; // 4 bytes
	i32 field_11; // 4 bytes
	i32 field_12; // 4 bytes
	u8 field_13; // 1 bytes
	// handler FUN_00a27008
};
POP_PACKED
ASSERT_SIZE(SN_RemoteServerPosition, 48);

PUSH_PACKED
struct SN_MonsterServerPosition
{
	enum { NET_ID = 62437 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	// handler FUN_00a2548d
};
POP_PACKED
ASSERT_SIZE(SN_MonsterServerPosition, 24);

PUSH_PACKED
struct SN_MonsterServerChase
{
	enum { NET_ID = 62438 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	// handler FUN_00a25443
};
POP_PACKED
ASSERT_SIZE(SN_MonsterServerChase, 12);

PUSH_PACKED
struct SN_RegameData
{
	enum { NET_ID = 62439 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a26e37
};
POP_PACKED
ASSERT_SIZE(SN_RegameData, 8);

PUSH_PACKED
struct SN_RegameAvailable
{
	enum { NET_ID = 62440 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	u8 field_2; // 1 bytes
	u8 field_3; // 1 bytes
	// handler FUN_00a26ddf
};
POP_PACKED
ASSERT_SIZE(SN_RegameAvailable, 10);

PUSH_PACKED
struct SA_PvpRecord
{
	enum { NET_ID = 62441 };
	u8 field_0; // 1 bytes
	u8 field_1; // 1 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	i32 field_8; // 4 bytes
	i32 field_9; // 4 bytes
	i32 field_10; // 4 bytes
	u8 field_11; // 1 bytes
	i32 field_12; // 4 bytes
	i32 field_13; // 4 bytes
	i32 field_14; // 4 bytes
	i32 field_15; // 4 bytes
	// handler &LAB_00a1dd14
};
POP_PACKED
ASSERT_SIZE(SA_PvpRecord, 55);

PUSH_PACKED
struct SA_PvpDetailRecord
{
	enum { NET_ID = 62442 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	u8 field_6; // 1 bytes
	i32 field_7; // 4 bytes
	i32 field_8; // 4 bytes
	u16 field_9; // 2 bytes
	u16 field_10; // 2 bytes
	u16 field_11; // 2 bytes
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	// (0-byte check)
	i32 field_12; // 4 bytes
	i64 field_13; // 8 bytes
	i32 field_14; // 4 bytes
	i32 field_15; // 4 bytes
	u16 field_16; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a1dbd3
};
POP_PACKED

PUSH_PACKED
struct SN_SummaryRewardResult
{
	enum { NET_ID = 62443 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	u8 field_5; // 1 bytes
	i64 field_6; // 8 bytes
	// handler FUN_00a289c0
};
POP_PACKED
ASSERT_SIZE(SN_SummaryRewardResult, 29);

PUSH_PACKED
struct SN_ReduceCooltime
{
	enum { NET_ID = 62444 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	// handler FUN_00a26d87
};
POP_PACKED
ASSERT_SIZE(SN_ReduceCooltime, 16);

PUSH_PACKED
struct SN_BroadcastGamePingData
{
	enum { NET_ID = 62445 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	// handler FUN_00a204ee
};
POP_PACKED
ASSERT_SIZE(SN_BroadcastGamePingData, 24);

PUSH_PACKED
struct SN_BroadcastEvade
{
	enum { NET_ID = 62447 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	// handler FUN_00a20496
};
POP_PACKED
ASSERT_SIZE(SN_BroadcastEvade, 16);

PUSH_PACKED
struct SN_ErrorMessage
{
	enum { NET_ID = 62451 };
	u8 field_0; // 1 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	u16 field_3; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_4; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a21d2d
};
POP_PACKED

PUSH_PACKED
struct SN_BushObjectState
{
	enum { NET_ID = 62452 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a205d5
};
POP_PACKED
ASSERT_SIZE(SN_BushObjectState, 8);

PUSH_PACKED
struct SN_BushState
{
	enum { NET_ID = 62453 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a2060f
};
POP_PACKED
ASSERT_SIZE(SN_BushState, 8);

PUSH_PACKED
struct SN_HideDetect
{
	enum { NET_ID = 62454 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	u8 field_2; // 1 bytes
	// handler FUN_00a24286
};
POP_PACKED
ASSERT_SIZE(SN_HideDetect, 9);

PUSH_PACKED
struct SN_SortieMasterBanPhaseStart
{
	enum { NET_ID = 62456 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	u16 field_5; // 2 bytes
	i32 field_6; // 4 bytes
	// handler &LAB_00a2817e
};
POP_PACKED
ASSERT_SIZE(SN_SortieMasterBanPhaseStart, 26);

PUSH_PACKED
struct SN_SortieMasterAssignPhaseStart
{
	enum { NET_ID = 62457 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	u16 field_5; // 2 bytes
	i32 field_6; // 4 bytes
	// handler &LAB_00a2802d
};
POP_PACKED
ASSERT_SIZE(SN_SortieMasterAssignPhaseStart, 26);

PUSH_PACKED
struct SA_SortieMasterBan
{
	enum { NET_ID = 62458 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1ebd7
};
POP_PACKED
ASSERT_SIZE(SA_SortieMasterBan, 4);

PUSH_PACKED
struct SN_SortieMasterBan
{
	enum { NET_ID = 62459 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a280b3
};
POP_PACKED
ASSERT_SIZE(SN_SortieMasterBan, 8);

PUSH_PACKED
struct SA_SortieMasterAssign
{
	enum { NET_ID = 62460 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1eba7
};
POP_PACKED
ASSERT_SIZE(SA_SortieMasterAssign, 4);

PUSH_PACKED
struct SN_SortieMasterAssign
{
	enum { NET_ID = 62461 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a27f56
};
POP_PACKED
ASSERT_SIZE(SN_SortieMasterAssign, 8);

PUSH_PACKED
struct SN_SortieMasterBanPhaseEnd
{
	enum { NET_ID = 62463 };
	u16 field_0; // 2 bytes
	i32 field_1; // 4 bytes
	u16 field_2; // 2 bytes
	i32 field_3; // 4 bytes
	// handler &LAB_00a280ed
};
POP_PACKED
ASSERT_SIZE(SN_SortieMasterBanPhaseEnd, 12);

PUSH_PACKED
struct SN_SortieMasterAssignPhaseEnd
{
	enum { NET_ID = 62464 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	i32 field_2; // 4 bytes
	u16 field_3; // 2 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	u16 field_6; // 2 bytes
	i32 field_7; // 4 bytes
	u16 field_8; // 2 bytes
	i32 field_9; // 4 bytes
	// handler &LAB_00a27f90
};
POP_PACKED
ASSERT_SIZE(SN_SortieMasterAssignPhaseEnd, 32);

PUSH_PACKED
struct SA_TierStageRecord
{
	enum { NET_ID = 62470 };
	u8 field_0; // 1 bytes
	u8 field_1; // 1 bytes
	u8 field_2; // 1 bytes
	u16 field_3; // 2 bytes
	u16 field_4; // 2 bytes
	u8 field_5; // 1 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	i32 field_8; // 4 bytes
	i32 field_9; // 4 bytes
	i32 field_10; // 4 bytes
	u8 field_11; // 1 bytes
	u16 field_12; // 2 bytes
	u16 field_13; // 2 bytes
	i32 field_14; // 4 bytes
	i32 field_15; // 4 bytes
	i32 field_16; // 4 bytes
	i32 field_17; // 4 bytes
	i32 field_18; // 4 bytes
	// handler &LAB_00a1ede4
};
POP_PACKED
ASSERT_SIZE(SA_TierStageRecord, 53);

PUSH_PACKED
struct SA_PvpRanking
{
	enum { NET_ID = 62471 };
	u8 field_0; // 1 bytes
	u8 field_1; // 1 bytes
	u8 field_2; // 1 bytes
	u16 field_3; // 2 bytes
	u8 field_4; // 1 bytes
	u16 field_5; // 2 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	i32 field_8; // 4 bytes
	i32 field_9; // 4 bytes
	i32 field_10; // 4 bytes
	// handler &LAB_00a1dca7
};
POP_PACKED
ASSERT_SIZE(SA_PvpRanking, 28);

PUSH_PACKED
struct SA_PveRanking
{
	enum { NET_ID = 62475 };
	i32 field_0; // 4 bytes
	// (0-byte check)
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	u16 field_5; // 2 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	u16 field_8; // 2 bytes
	// (0-byte check)
	i32 field_9; // 4 bytes
	i32 field_10; // 4 bytes
	i32 field_11; // 4 bytes
	i32 field_12; // 4 bytes
	u16 field_13; // 2 bytes
	i32 field_14; // 4 bytes
	i32 field_15; // 4 bytes
	u16 field_16; // 2 bytes
	// handler &LAB_00a1dae2
};
POP_PACKED
ASSERT_SIZE(SA_PveRanking, 60);

PUSH_PACKED
struct SA_MyPveRanking
{
	enum { NET_ID = 62476 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	u16 field_5; // 2 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	u16 field_8; // 2 bytes
	u8 field_9; // 1 bytes
	// handler &LAB_00a1d2f3
};
POP_PACKED
ASSERT_SIZE(SA_MyPveRanking, 33);

PUSH_PACKED
struct SN_PveLastgameRanking
{
	enum { NET_ID = 62477 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	u16 field_6; // 2 bytes
	i32 field_7; // 4 bytes
	i32 field_8; // 4 bytes
	u16 field_9; // 2 bytes
	i32 field_10; // 4 bytes
	// handler &LAB_00a26872
};
POP_PACKED
ASSERT_SIZE(SN_PveLastgameRanking, 37);

PUSH_PACKED
struct SN_DefenceLastgameRanking
{
	enum { NET_ID = 62478 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	u16 field_5; // 2 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	u16 field_8; // 2 bytes
	i32 field_9; // 4 bytes
	// handler &LAB_00a216dc
};
POP_PACKED
ASSERT_SIZE(SN_DefenceLastgameRanking, 33);

PUSH_PACKED
struct SN_UNKNOWN_62479
{
	enum { NET_ID = 62479 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	u8 field_2; // 1 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	u16 field_6; // 2 bytes
	i32 field_7; // 4 bytes
	i32 field_8; // 4 bytes
	u16 field_9; // 2 bytes
	i32 field_10; // 4 bytes
	i32 field_11; // 4 bytes
	i32 field_12; // 4 bytes
	u16 field_13; // 2 bytes
	i32 field_14; // 4 bytes
	i32 field_15; // 4 bytes
	u16 field_16; // 2 bytes
	// handler &LAB_00a1af26
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62479, 57);

PUSH_PACKED
struct SN_ActiveIngameEventList
{
	enum { NET_ID = 62480 };
	i32 field_0; // 4 bytes
	// handler &LAB_00a1f729
};
POP_PACKED
ASSERT_SIZE(SN_ActiveIngameEventList, 4);

PUSH_PACKED
struct SN_PveTodayStageInfo
{
	enum { NET_ID = 62483 };
	u16 field_0; // 2 bytes
	u16 field_1; // 2 bytes
	// handler FUN_00a26920
};
POP_PACKED
ASSERT_SIZE(SN_PveTodayStageInfo, 4);

PUSH_PACKED
struct SN_FatiguePointInfo
{
	enum { NET_ID = 62484 };
	u16 field_0; // 2 bytes
	u16 field_1; // 2 bytes
	// handler FUN_00a221f4
};
POP_PACKED
ASSERT_SIZE(SN_FatiguePointInfo, 4);

PUSH_PACKED
struct SN_PlaytimePause
{
	enum { NET_ID = 62486 };
	i32 field_0; // 4 bytes
	// handler FUN_00a26423
};
POP_PACKED
ASSERT_SIZE(SN_PlaytimePause, 4);

PUSH_PACKED
struct SN_PlaytimeResume
{
	enum { NET_ID = 62487 };
	// payloadless: client validator consumes without field checks, logger dumps no fields,
	// and real captures show size=4 (NetHeader only). Send with no payload.
	// handler FUN_00a26453
};
POP_PACKED

PUSH_PACKED
struct SN_PvpEventAnnouncement
{
	enum { NET_ID = 62488 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	u8 field_5; // 1 bytes
	// handler FUN_00a269d4
};
POP_PACKED
ASSERT_SIZE(SN_PvpEventAnnouncement, 21);

PUSH_PACKED
struct SA_ChatChannelInvite
{
	enum { NET_ID = 62489 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a1a9c2
};
POP_PACKED

PUSH_PACKED
struct SN_ChatChannelInvite
{
	enum { NET_ID = 62490 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	u16 field_4; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_5; // 2 bytes
	// <variable-size field: string/vector>
	u8 field_6; // 1 bytes
	// handler &LAB_00a20bfe
};
POP_PACKED

PUSH_PACKED
struct SA_ChatChannelInviteFeedback
{
	enum { NET_ID = 62491 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1aa35
};
POP_PACKED
ASSERT_SIZE(SA_ChatChannelInviteFeedback, 4);

PUSH_PACKED
struct SN_ChatChannelInviteFeedback
{
	enum { NET_ID = 62492 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_2; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a20d2b
};
POP_PACKED

PUSH_PACKED
struct SN_InteractionStatus
{
	enum { NET_ID = 62493 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	// handler FUN_00a2453d
};
POP_PACKED
ASSERT_SIZE(SN_InteractionStatus, 20);

PUSH_PACKED
struct SN_InteractionCancel
{
	enum { NET_ID = 62494 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a24461
};
POP_PACKED
ASSERT_SIZE(SN_InteractionCancel, 8);

PUSH_PACKED
struct SN_InteractionCasting
{
	enum { NET_ID = 62495 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	// handler FUN_00a2449b
};
POP_PACKED
ASSERT_SIZE(SN_InteractionCasting, 16);

PUSH_PACKED
struct SN_InteractionExecute
{
	enum { NET_ID = 62496 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	// handler FUN_00a244f3
};
POP_PACKED
ASSERT_SIZE(SN_InteractionExecute, 12);

PUSH_PACKED
struct SQ_TeleportObject
{
	enum { NET_ID = 62497 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	// handler FUN_00a297d8
};
POP_PACKED
ASSERT_SIZE(SQ_TeleportObject, 16);

PUSH_PACKED
struct SN_PlayerSyncTeleport
{
	enum { NET_ID = 62498 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	// handler FUN_00a2633d
};
POP_PACKED
ASSERT_SIZE(SN_PlayerSyncTeleport, 16);

PUSH_PACKED
struct SN_ChangeBattleState
{
	enum { NET_ID = 62499 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	i32 field_2; // 4 bytes
	// handler FUN_00a208c1
};
POP_PACKED
ASSERT_SIZE(SN_ChangeBattleState, 9);

PUSH_PACKED
struct SA_RefreshWaitingQueue
{
	enum { NET_ID = 62502 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	u16 field_2; // 2 bytes
	// handler FUN_00a1de35
};
POP_PACKED
ASSERT_SIZE(SA_RefreshWaitingQueue, 10);

PUSH_PACKED
struct SN_GamePartyBroken
{
	enum { NET_ID = 62503 };
	// payloadless: client validator consumes without field checks, logger dumps no fields,
	// and real captures show size=4 (NetHeader only). Send with no payload.
	// handler FUN_00a22e97
};
POP_PACKED

PUSH_PACKED
struct SN_ScoreUpdate
{
	enum { NET_ID = 62504 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	// handler &LAB_00a27d0e
};
POP_PACKED
ASSERT_SIZE(SN_ScoreUpdate, 16);

PUSH_PACKED
struct SN_UNKNOWN_62505
{
	enum { NET_ID = 62505 };
	u8 field_0; // 1 bytes
	i32 field_1; // 4 bytes
	u16 field_2; // 2 bytes
	u16 field_3; // 2 bytes
	u16 field_4; // 2 bytes
	u16 field_5; // 2 bytes
	u16 field_6; // 2 bytes
	i32 field_7; // 4 bytes
	i32 field_8; // 4 bytes
	i32 field_9; // 4 bytes
	i32 field_10; // 4 bytes
	i32 field_11; // 4 bytes
	i32 field_12; // 4 bytes
	u8 field_13; // 1 bytes
	// handler FUN_00a1f873
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62505, 40);

PUSH_PACKED
struct SN_UNKNOWN_62506
{
	enum { NET_ID = 62506 };
	u8 field_0; // 1 bytes
	i32 field_1; // 4 bytes
	u16 field_2; // 2 bytes
	u16 field_3; // 2 bytes
	u16 field_4; // 2 bytes
	u16 field_5; // 2 bytes
	// handler FUN_00a1f952
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62506, 13);

PUSH_PACKED
struct SA_ItemTrade
{
	enum { NET_ID = 62507 };
	i32 field_0; // 4 bytes
	i64 field_1; // 8 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	// handler &LAB_00a1cd19
};
POP_PACKED
ASSERT_SIZE(SA_ItemTrade, 24);

PUSH_PACKED
struct SA_ItemCraft
{
	enum { NET_ID = 62508 };
	u8 field_0; // 1 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	// handler &LAB_00a1cb6d
};
POP_PACKED
ASSERT_SIZE(SA_ItemCraft, 17);

PUSH_PACKED
struct SA_ItemDisassemble
{
	enum { NET_ID = 62509 };
	u8 field_0; // 1 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	// handler &LAB_00a1cc0b
};
POP_PACKED
ASSERT_SIZE(SA_ItemDisassemble, 17);

PUSH_PACKED
struct SA_OlympicBestRecord
{
	enum { NET_ID = 62510 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	u16 field_2; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a1d509
};
POP_PACKED

PUSH_PACKED
struct SA_DefenceBestRecord
{
	enum { NET_ID = 62511 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	// handler FUN_00a1aece
};
POP_PACKED
ASSERT_SIZE(SA_DefenceBestRecord, 16);

PUSH_PACKED
struct SN_OlympicIngameRecords
{
	enum { NET_ID = 62512 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	i32 field_2; // 4 bytes
	u8 field_3; // 1 bytes
	u8 field_4; // 1 bytes
	// handler &LAB_00a25999
};
POP_PACKED
ASSERT_SIZE(SN_OlympicIngameRecords, 11);

PUSH_PACKED
struct SA_PveRefillBattery
{
	enum { NET_ID = 62513 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1dba3
};
POP_PACKED
ASSERT_SIZE(SA_PveRefillBattery, 4);

PUSH_PACKED
struct SN_UNKNOWN_62514
{
	enum { NET_ID = 62514 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1ab1b
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62514, 4);

PUSH_PACKED
struct SA_SelectBadge
{
	enum { NET_ID = 62515 };
	u8 field_0; // 1 bytes
	// handler FUN_00a1e5f1
};
POP_PACKED
ASSERT_SIZE(SA_SelectBadge, 1);

PUSH_PACKED
struct SN_ResistStatus
{
	enum { NET_ID = 62516 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	// handler FUN_00a27630
};
POP_PACKED
ASSERT_SIZE(SN_ResistStatus, 12);

PUSH_PACKED
struct SA_TransformCharacter
{
	enum { NET_ID = 62517 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	// handler FUN_00a1eec3
};
POP_PACKED
ASSERT_SIZE(SA_TransformCharacter, 16);

PUSH_PACKED
struct SN_TransformCharacter
{
	enum { NET_ID = 62518 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	u16 field_7; // 2 bytes
	u8 field_8; // 1 bytes
	i32 field_9; // 4 bytes
	u16 field_10; // 2 bytes
	u8 field_11; // 1 bytes
	i32 field_12; // 4 bytes
	i32 field_13; // 4 bytes
	u8 field_14; // 1 bytes
	// handler &LAB_00a28ede
};
POP_PACKED
ASSERT_SIZE(SN_TransformCharacter, 47);

PUSH_PACKED
struct SN_ChangeCharacterMode
{
	enum { NET_ID = 62519 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	u16 field_6; // 2 bytes
	u8 field_7; // 1 bytes
	i32 field_8; // 4 bytes
	u16 field_9; // 2 bytes
	u8 field_10; // 1 bytes
	i32 field_11; // 4 bytes
	i32 field_12; // 4 bytes
	u8 field_13; // 1 bytes
	// handler &LAB_00a20909
};
POP_PACKED
ASSERT_SIZE(SN_ChangeCharacterMode, 43);

PUSH_PACKED
struct SN_ItemOptiongroupList
{
	enum { NET_ID = 62520 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	u8 field_2; // 1 bytes
	i32 field_3; // 4 bytes
	u8 field_4; // 1 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	// handler &LAB_00a246a0
};
POP_PACKED
ASSERT_SIZE(SN_ItemOptiongroupList, 20);

PUSH_PACKED
struct SA_Honor
{
	enum { NET_ID = 62521 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1ca36
};
POP_PACKED
ASSERT_SIZE(SA_Honor, 4);

PUSH_PACKED
struct SA_Blame
{
	enum { NET_ID = 62522 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1a5cf
};
POP_PACKED
ASSERT_SIZE(SA_Blame, 4);

PUSH_PACKED
struct SN_DynamicSyncAniTrack
{
	enum { NET_ID = 62523 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	u8 field_3; // 1 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	i32 field_8; // 4 bytes
	i32 field_9; // 4 bytes
	i32 field_10; // 4 bytes
	i64 field_11; // 8 bytes
	i64 field_12; // 8 bytes
	// handler FUN_00a21a96
};
POP_PACKED
ASSERT_SIZE(SN_DynamicSyncAniTrack, 57);

PUSH_PACKED
struct SN_DisconnectReason
{
	enum { NET_ID = 62524 };
	i32 field_0; // 4 bytes
	// handler FUN_00a21877
};
POP_PACKED
ASSERT_SIZE(SN_DisconnectReason, 4);

PUSH_PACKED
struct SN_AddictionWarning
{
	enum { NET_ID = 62526 };
	u8 field_0; // 1 bytes
	// handler FUN_00a1f77c
};
POP_PACKED
ASSERT_SIZE(SN_AddictionWarning, 1);

PUSH_PACKED
struct SA_WatchGame
{
	enum { NET_ID = 62527 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1f165
};
POP_PACKED
ASSERT_SIZE(SA_WatchGame, 4);

PUSH_PACKED
struct SN_UserReturnToCity
{
	enum { NET_ID = 62528 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a292a6
};
POP_PACKED

PUSH_PACKED
struct SN_NcguardMsg
{
	enum { NET_ID = 62529 };
	u8 field_0; // 1 bytes
	// handler &LAB_00a255bc
};
POP_PACKED
ASSERT_SIZE(SN_NcguardMsg, 1);

PUSH_PACKED
struct SN_TenprotectPunishment
{
	enum { NET_ID = 62530 };
	u8 field_0; // 1 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a28c27
};
POP_PACKED

PUSH_PACKED
struct SN_UNKNOWN_62531
{
	enum { NET_ID = 62531 };
	u16 field_0; // 2 bytes
	u16 field_1; // 2 bytes
	u16 field_2; // 2 bytes
	// handler FUN_00a20134
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62531, 6);

PUSH_PACKED
struct SN_LightOnArea
{
	enum { NET_ID = 62532 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	u8 field_2; // 1 bytes
	// handler FUN_00a249bf
};
POP_PACKED
ASSERT_SIZE(SN_LightOnArea, 6);

PUSH_PACKED
struct SN_LightOffArea
{
	enum { NET_ID = 62533 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	u8 field_2; // 1 bytes
	// handler FUN_00a24977
};
POP_PACKED
ASSERT_SIZE(SN_LightOffArea, 6);

PUSH_PACKED
struct SA_RequestVoting
{
	enum { NET_ID = 62534 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	// handler FUN_00a1dfed
};
POP_PACKED
ASSERT_SIZE(SA_RequestVoting, 16);

PUSH_PACKED
struct SN_RequestVoting
{
	enum { NET_ID = 62535 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	u8 field_4; // 1 bytes
	// handler FUN_00a275c9
};
POP_PACKED
ASSERT_SIZE(SN_RequestVoting, 17);

PUSH_PACKED
struct SN_UNKNOWN_62536
{
	enum { NET_ID = 62536 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1a834
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62536, 4);

PUSH_PACKED
struct SA_Vote
{
	enum { NET_ID = 62537 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	// handler FUN_00a1f04b
};
POP_PACKED
ASSERT_SIZE(SA_Vote, 12);

PUSH_PACKED
struct SN_Vote
{
	enum { NET_ID = 62538 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a29309
};
POP_PACKED
ASSERT_SIZE(SN_Vote, 8);

PUSH_PACKED
struct SN_VotingResult
{
	enum { NET_ID = 62539 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	// handler FUN_00a29343
};
POP_PACKED
ASSERT_SIZE(SN_VotingResult, 24);

PUSH_PACKED
struct SN_PenaltyBadMouther
{
	enum { NET_ID = 62540 };
	i32 field_0; // 4 bytes
	// handler FUN_00a25ef4
};
POP_PACKED
ASSERT_SIZE(SN_PenaltyBadMouther, 4);

PUSH_PACKED
struct SN_PvpAiModeEventAnnouncement
{
	enum { NET_ID = 62541 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	u8 field_5; // 1 bytes
	// handler FUN_00a2695a
};
POP_PACKED
ASSERT_SIZE(SN_PvpAiModeEventAnnouncement, 21);

PUSH_PACKED
struct SN_DateChanged
{
	enum { NET_ID = 62542 };
	// payloadless: client validator consumes without field checks, logger dumps no fields,
	// and real captures show size=4 (NetHeader only). Send with no payload.
	// handler FUN_00a214d4
};
POP_PACKED

PUSH_PACKED
struct SN_CalendarUpdated
{
	enum { NET_ID = 62543 };
	// payloadless: client validator consumes without field checks, logger dumps no fields,
	// and real captures show size=4 (NetHeader only). Send with no payload.
	// handler FUN_00a2069c
};
POP_PACKED

PUSH_PACKED
struct SN_Calendar
{
	enum { NET_ID = 62544 };
	i64 field_0; // 8 bytes
	u16 field_1; // 2 bytes
	u8 field_2; // 1 bytes
	i32 field_3; // 4 bytes
	i64 field_4; // 8 bytes
	i64 field_5; // 8 bytes
	// handler &LAB_00a20649
};
POP_PACKED
ASSERT_SIZE(SN_Calendar, 31);

PUSH_PACKED
struct SN_StageSkillAvailable
{
	enum { NET_ID = 62546 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler &LAB_00a2872f
};
POP_PACKED
ASSERT_SIZE(SN_StageSkillAvailable, 8);

PUSH_PACKED
struct SA_StageSkillSelect
{
	enum { NET_ID = 62547 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a1ec37
};
POP_PACKED
ASSERT_SIZE(SA_StageSkillSelect, 8);

PUSH_PACKED
struct SN_StageSkillSelect
{
	enum { NET_ID = 62548 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a2878f
};
POP_PACKED
ASSERT_SIZE(SN_StageSkillSelect, 8);

PUSH_PACKED
struct SN_SinglemodeSelectCharacter
{
	enum { NET_ID = 62549 };
	// payloadless: client validator consumes without field checks, logger dumps no fields,
	// and real captures show size=4 (NetHeader only). Send with no payload.
	// handler FUN_00a27e3a
};
POP_PACKED

PUSH_PACKED
struct SN_AvailableAccountEquipmentList
{
	enum { NET_ID = 62550 };
	u8 field_0; // 1 bytes
	i32 field_1; // 4 bytes
	u16 field_2; // 2 bytes
	// handler &LAB_00a201df
};
POP_PACKED
ASSERT_SIZE(SN_AvailableAccountEquipmentList, 7);

PUSH_PACKED
struct SN_ItemLifetimeExpired
{
	enum { NET_ID = 62551 };
	i32 field_0; // 4 bytes
	// handler &LAB_00a2464d
};
POP_PACKED
ASSERT_SIZE(SN_ItemLifetimeExpired, 4);

PUSH_PACKED
struct SN_EffectLifetimeExpired
{
	enum { NET_ID = 62552 };
	i32 field_0; // 4 bytes
	// handler &LAB_00a21b53
};
POP_PACKED
ASSERT_SIZE(SN_EffectLifetimeExpired, 4);

PUSH_PACKED
struct SN_Gauge
{
	enum { NET_ID = 62553 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	u8 field_2; // 1 bytes
	i32 field_3; // 4 bytes
	u16 field_4; // 2 bytes
	u16 field_5; // 2 bytes
	// handler FUN_00a231c7
};
POP_PACKED
ASSERT_SIZE(SN_Gauge, 14);

PUSH_PACKED
struct SN_EffectLifetimeList
{
	enum { NET_ID = 62554 };
	i32 field_0; // 4 bytes
	i64 field_1; // 8 bytes
	// handler &LAB_00a21ba6
};
POP_PACKED
ASSERT_SIZE(SN_EffectLifetimeList, 12);

PUSH_PACKED
struct SN_MonsterRespawnTime
{
	enum { NET_ID = 62555 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	// handler FUN_00a253f9
};
POP_PACKED
ASSERT_SIZE(SN_MonsterRespawnTime, 12);

PUSH_PACKED
struct SA_GuideMissionAccept
{
	enum { NET_ID = 62556 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	// handler FUN_00a1c988
};
POP_PACKED
ASSERT_SIZE(SA_GuideMissionAccept, 5);

PUSH_PACKED
struct SA_GuideMissionReward
{
	enum { NET_ID = 62557 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	// handler FUN_00a1c9fc
};
POP_PACKED
ASSERT_SIZE(SA_GuideMissionReward, 5);

PUSH_PACKED
struct SA_GuideMissionQuit
{
	enum { NET_ID = 62558 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	// handler FUN_00a1c9c2
};
POP_PACKED
ASSERT_SIZE(SA_GuideMissionQuit, 5);

PUSH_PACKED
struct SN_CurrentGuideMission
{
	enum { NET_ID = 62559 };
	u8 field_0; // 1 bytes
	u8 field_1; // 1 bytes
	u8 field_2; // 1 bytes
	// handler FUN_00a213f1
};
POP_PACKED
ASSERT_SIZE(SN_CurrentGuideMission, 3);

PUSH_PACKED
struct SN_MonsterMineralLossTiming
{
	enum { NET_ID = 62560 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler &LAB_00a25396
};
POP_PACKED
ASSERT_SIZE(SN_MonsterMineralLossTiming, 8);

PUSH_PACKED
struct SN_PveTimeOverWarning
{
	enum { NET_ID = 62561 };
	i64 field_0; // 8 bytes
	// handler FUN_00a268f0
};
POP_PACKED
ASSERT_SIZE(SN_PveTimeOverWarning, 8);

PUSH_PACKED
struct SA_ActivityReward
{
	enum { NET_ID = 62562 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	// handler FUN_00a1a535
};
POP_PACKED
ASSERT_SIZE(SA_ActivityReward, 5);

PUSH_PACKED
struct SA_NpBuyCshopItem
{
	enum { NET_ID = 62563 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	// handler FUN_00a1d45f
};
POP_PACKED
ASSERT_SIZE(SA_NpBuyCshopItem, 12);

PUSH_PACKED
struct SN_ExpIngame
{
	enum { NET_ID = 62564 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	u16 field_2; // 2 bytes
	i32 field_3; // 4 bytes
	// handler FUN_00a21fb7
};
POP_PACKED
ASSERT_SIZE(SN_ExpIngame, 12);

PUSH_PACKED
struct SN_LevelIngame
{
	enum { NET_ID = 62565 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	u8 field_2; // 1 bytes
	// handler FUN_00a2492d
};
POP_PACKED
ASSERT_SIZE(SN_LevelIngame, 7);

PUSH_PACKED
struct SN_SpectatorDelay
{
	enum { NET_ID = 62566 };
	i32 field_0; // 4 bytes
	// handler FUN_00a28484
};
POP_PACKED
ASSERT_SIZE(SN_SpectatorDelay, 4);

PUSH_PACKED
struct SN_SpectatorStart
{
	enum { NET_ID = 62567 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a284b4
};
POP_PACKED
ASSERT_SIZE(SN_SpectatorStart, 8);

PUSH_PACKED
struct SN_SpectatorTrespassReady
{
	enum { NET_ID = 62568 };
	u8 field_0; // 1 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a284ee
};
POP_PACKED
ASSERT_SIZE(SN_SpectatorTrespassReady, 5);

PUSH_PACKED
struct SN_PhaseStart
{
	enum { NET_ID = 62569 };
	i32 field_0; // 4 bytes
	// handler FUN_00a25f6e
};
POP_PACKED
ASSERT_SIZE(SN_PhaseStart, 4);

PUSH_PACKED
struct SN_PhaseEnd
{
	enum { NET_ID = 62570 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	// handler FUN_00a25f24
};
POP_PACKED
ASSERT_SIZE(SN_PhaseEnd, 12);

PUSH_PACKED
struct SA_MasterTrainingEvent
{
	enum { NET_ID = 62571 };
	u8 field_0; // 1 bytes
	i32 field_1; // 4 bytes
	i32 field_2; // 4 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	// handler FUN_00a1d117
};
POP_PACKED
ASSERT_SIZE(SA_MasterTrainingEvent, 17);

PUSH_PACKED
struct SN_GmDisabledMasters
{
	enum { NET_ID = 62572 };
	i32 field_0; // 4 bytes
	// handler &LAB_00a23330
};
POP_PACKED
ASSERT_SIZE(SN_GmDisabledMasters, 4);

PUSH_PACKED
struct SN_GmDisabledSkins
{
	enum { NET_ID = 62573 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a23383
};
POP_PACKED

PUSH_PACKED
struct SN_GmDisabledStages
{
	enum { NET_ID = 62574 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	// handler &LAB_00a233d6
};
POP_PACKED
ASSERT_SIZE(SN_GmDisabledStages, 5);

PUSH_PACKED
struct SN_CharacterPropertyPoint
{
	enum { NET_ID = 62578 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	// handler FUN_00a20b3c
};
POP_PACKED
ASSERT_SIZE(SN_CharacterPropertyPoint, 5);

PUSH_PACKED
struct SN_UNKNOWN_62579
{
	enum { NET_ID = 62579 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1d58c
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62579, 4);

PUSH_PACKED
struct SN_UNKNOWN_62580
{
	enum { NET_ID = 62580 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1ab4b
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62580, 4);

PUSH_PACKED
struct SA_CharacterPropertyUpgrade
{
	enum { NET_ID = 62581 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1a90e
};
POP_PACKED
ASSERT_SIZE(SA_CharacterPropertyUpgrade, 4);

PUSH_PACKED
struct SA_CharacterPropertyReset
{
	enum { NET_ID = 62582 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1a8de
};
POP_PACKED
ASSERT_SIZE(SA_CharacterPropertyReset, 4);

PUSH_PACKED
struct SN_CharacterPropertyReset
{
	enum { NET_ID = 62583 };
	i32 field_0; // 4 bytes
	// handler FUN_00a20b76
};
POP_PACKED
ASSERT_SIZE(SN_CharacterPropertyReset, 4);

PUSH_PACKED
struct SN_CharacterPropertyInfo
{
	enum { NET_ID = 62584 };
	i32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	u8 field_2; // 1 bytes
	// handler &LAB_00a20adb
};
POP_PACKED
ASSERT_SIZE(SN_CharacterPropertyInfo, 6);

PUSH_PACKED
struct SN_GameRecordInfoList
{
	enum { NET_ID = 62585 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	u8 field_2; // 1 bytes
	// handler &LAB_00a2312b
};
POP_PACKED
ASSERT_SIZE(SN_GameRecordInfoList, 7);

PUSH_PACKED
struct SA_ChatGetInviteeGameaccountid
{
	enum { NET_ID = 62586 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	u16 field_2; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_3; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a1aa65
};
POP_PACKED

PUSH_PACKED
struct SA_UseCoupon
{
	enum { NET_ID = 62587 };
	i32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a1efdb
};
POP_PACKED

PUSH_PACKED
struct SN_NotifyChattingChannelStatus
{
	enum { NET_ID = 62588 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_1; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_2; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_3; // 2 bytes
	// <variable-size field: string/vector>
	u16 field_4; // 2 bytes
	// <variable-size field: string/vector>
	i32 field_5; // 4 bytes
	// handler &LAB_00a25658
};
POP_PACKED

PUSH_PACKED
struct SN_DefenceModeIngameInfo
{
	enum { NET_ID = 62589 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a2175a
};
POP_PACKED
ASSERT_SIZE(SN_DefenceModeIngameInfo, 8);

PUSH_PACKED
struct SA_RequestToken
{
	enum { NET_ID = 62590 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	// handler &LAB_00a1df8d
};
POP_PACKED

PUSH_PACKED
struct SN_HudEventList
{
	enum { NET_ID = 62591 };
	u8 field_0; // 1 bytes
	u16 field_1; // 2 bytes
	u16 field_2; // 2 bytes
	// handler &LAB_00a242d0
};
POP_PACKED
ASSERT_SIZE(SN_HudEventList, 5);

PUSH_PACKED
struct SN_CshopEventList
{
	enum { NET_ID = 62592 };
	u8 field_0; // 1 bytes
	u8 field_1; // 1 bytes
	i32 field_2; // 4 bytes
	u16 field_3; // 2 bytes
	// handler &LAB_00a2139e
};
POP_PACKED
ASSERT_SIZE(SN_CshopEventList, 8);

PUSH_PACKED
struct SA_UserReport
{
	enum { NET_ID = 62593 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1efab
};
POP_PACKED
ASSERT_SIZE(SA_UserReport, 4);

PUSH_PACKED
struct SN_UNKNOWN_62594
{
	enum { NET_ID = 62594 };
	u8 field_0; // 1 bytes
	u8 field_1; // 1 bytes
	// handler FUN_00a1e571
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62594, 2);

PUSH_PACKED
struct SN_ServerUtcTime
{
	enum { NET_ID = 62595 };
	i64 field_0; // 8 bytes
	// handler FUN_00a27d7d
};
POP_PACKED
ASSERT_SIZE(SN_ServerUtcTime, 8);

PUSH_PACKED
struct SN_NotifyPccafeTagbuff
{
	enum { NET_ID = 62596 };
	// payloadless: client validator consumes without field checks, logger dumps no fields,
	// and real captures show size=4 (NetHeader only). Send with no payload.
	// handler FUN_00a2588b
};
POP_PACKED

PUSH_PACKED
struct SN_PraiseInfo
{
	enum { NET_ID = 62597 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	u16 field_2; // 2 bytes
	i32 field_3; // 4 bytes
	i32 field_4; // 4 bytes
	i32 field_5; // 4 bytes
	i32 field_6; // 4 bytes
	i32 field_7; // 4 bytes
	i32 field_8; // 4 bytes
	i32 field_9; // 4 bytes
	// handler &LAB_00a264dc
};
POP_PACKED
ASSERT_SIZE(SN_PraiseInfo, 38);

PUSH_PACKED
struct SN_PraiseStart
{
	enum { NET_ID = 62598 };
	i32 field_0; // 4 bytes
	// handler FUN_00a2653d
};
POP_PACKED
ASSERT_SIZE(SN_PraiseStart, 4);

PUSH_PACKED
struct SA_Praise
{
	enum { NET_ID = 62599 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1da38
};
POP_PACKED
ASSERT_SIZE(SA_Praise, 4);

PUSH_PACKED
struct SN_Praise
{
	enum { NET_ID = 62600 };
	u16 field_0; // 2 bytes
	// <variable-size field: string/vector>
	i32 field_1; // 4 bytes
	// handler &LAB_00a2646c
};
POP_PACKED

PUSH_PACKED
struct SA_PauseGame
{
	enum { NET_ID = 62602 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1da08
};
POP_PACKED
ASSERT_SIZE(SA_PauseGame, 4);

PUSH_PACKED
struct SN_PausedGame
{
	enum { NET_ID = 62603 };
	i32 field_0; // 4 bytes
	i32 field_1; // 4 bytes
	// handler FUN_00a25eba
};
POP_PACKED
ASSERT_SIZE(SN_PausedGame, 8);

PUSH_PACKED
struct SA_ResumeGame
{
	enum { NET_ID = 62604 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1e2ce
};
POP_PACKED
ASSERT_SIZE(SA_ResumeGame, 4);

PUSH_PACKED
struct SN_ResumeGameDelay
{
	enum { NET_ID = 62605 };
	i32 field_0; // 4 bytes
	// handler FUN_00a27936
};
POP_PACKED
ASSERT_SIZE(SN_ResumeGameDelay, 4);

PUSH_PACKED
struct SN_ResumedGame
{
	enum { NET_ID = 62606 };
	// payloadless: client validator consumes without field checks, logger dumps no fields,
	// and real captures show size=4 (NetHeader only). Send with no payload.
	// handler FUN_00a2791d
};
POP_PACKED

PUSH_PACKED
struct SA_NpReportChat
{
	enum { NET_ID = 62607 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1d4a9
};
POP_PACKED
ASSERT_SIZE(SA_NpReportChat, 4);

PUSH_PACKED
struct SA_NpReportMail
{
	enum { NET_ID = 62608 };
	i32 field_0; // 4 bytes
	// handler FUN_00a1d4d9
};
POP_PACKED
ASSERT_SIZE(SA_NpReportMail, 4);

} // Sv

// ==== Client->server (Cl) packets registered in the client's send table, not yet reversed ====
// netid : category (0=normal, 1=?, 2=?)
// 60004 (0xea64): cat=0  <TODO: reverse layout>
// 60006 (0xea66): cat=0  <TODO: reverse layout>
// 60010 (0xea6a): cat=1  <TODO: reverse layout>
// 60011 (0xea6b): cat=0  <TODO: reverse layout>
// 60012 (0xea6c): cat=0  <TODO: reverse layout>
// 60013 (0xea6d): cat=0  <TODO: reverse layout>
// 60015 (0xea6f): cat=0  <TODO: reverse layout>
// 60017 (0xea71): cat=0  <TODO: reverse layout>
// 60019 (0xea73): cat=1  <TODO: reverse layout>
// 60020 (0xea74): cat=1  <TODO: reverse layout>
// 60025 (0xea79): cat=0  <TODO: reverse layout>
// 60026 (0xea7a): cat=0  <TODO: reverse layout>
// 60027 (0xea7b): cat=1  <TODO: reverse layout>
// 60028 (0xea7c): cat=2  <TODO: reverse layout>
// 60029 (0xea7d): cat=0  <TODO: reverse layout>
// 60030 (0xea7e): cat=0  <TODO: reverse layout>
// 60031 (0xea7f): cat=0  <TODO: reverse layout>
// 60036 (0xea84): cat=0  <TODO: reverse layout>
// 60037 (0xea85): cat=0  <TODO: reverse layout>
// 60039 (0xea87): cat=1  <TODO: reverse layout>
// 60041 (0xea89): cat=1  <TODO: reverse layout>
// 60042 (0xea8a): cat=1  <TODO: reverse layout>
// 60043 (0xea8b): cat=1  <TODO: reverse layout>
// 60044 (0xea8c): cat=1  <TODO: reverse layout>
// 60045 (0xea8d): cat=0  <TODO: reverse layout>
// 60046 (0xea8e): cat=0  <TODO: reverse layout>
// 60047 (0xea8f): cat=0  <TODO: reverse layout>
// 60048 (0xea90): cat=0  <TODO: reverse layout>
// 60054 (0xea96): cat=0  <TODO: reverse layout>
// 60055 (0xea97): cat=1  <TODO: reverse layout>
// 60056 (0xea98): cat=0  <TODO: reverse layout>
// 60057 (0xea99): cat=0  <TODO: reverse layout>
// 60058 (0xea9a): cat=0  <TODO: reverse layout>
// 60060 (0xea9c): cat=0  <TODO: reverse layout>
// 60061 (0xea9d): cat=0  <TODO: reverse layout>
// 60062 (0xea9e): cat=0  <TODO: reverse layout>
// 60065 (0xeaa1): cat=0  <TODO: reverse layout>
// 60066 (0xeaa2): cat=0  <TODO: reverse layout>
// 60067 (0xeaa3): cat=0  <TODO: reverse layout>
// 60068 (0xeaa4): cat=0  <TODO: reverse layout>
// 60069 (0xeaa5): cat=0  <TODO: reverse layout>
// 60070 (0xeaa6): cat=0  <TODO: reverse layout>
// 60077 (0xeaad): cat=0  <TODO: reverse layout>
// 60078 (0xeaae): cat=0  <TODO: reverse layout>
// 60079 (0xeaaf): cat=0  <TODO: reverse layout>
// 60080 (0xeab0): cat=0  <TODO: reverse layout>
// 60081 (0xeab1): cat=0  <TODO: reverse layout>
// 60082 (0xeab2): cat=0  <TODO: reverse layout>
// 60083 (0xeab3): cat=0  <TODO: reverse layout>
// 60084 (0xeab4): cat=0  <TODO: reverse layout>
// 60085 (0xeab5): cat=0  <TODO: reverse layout>
// 60086 (0xeab6): cat=0  <TODO: reverse layout>
// 60087 (0xeab7): cat=0  <TODO: reverse layout>
// 60088 (0xeab8): cat=0  <TODO: reverse layout>
// 60091 (0xeabb): cat=0  <TODO: reverse layout>
// 60092 (0xeabc): cat=0  <TODO: reverse layout>
// 60093 (0xeabd): cat=0  <TODO: reverse layout>
// 60094 (0xeabe): cat=0  <TODO: reverse layout>
// 60095 (0xeabf): cat=0  <TODO: reverse layout>
// 60096 (0xeac0): cat=0  <TODO: reverse layout>
// 60098 (0xeac2): cat=0  <TODO: reverse layout>
// 60099 (0xeac3): cat=0  <TODO: reverse layout>
// 60104 (0xeac8): cat=0  <TODO: reverse layout>
// 60105 (0xeac9): cat=0  <TODO: reverse layout>
// 60106 (0xeaca): cat=0  <TODO: reverse layout>
// 60107 (0xeacb): cat=1  <TODO: reverse layout>
// 60108 (0xeacc): cat=1  <TODO: reverse layout>
// 60109 (0xeacd): cat=1  <TODO: reverse layout>
// 60110 (0xeace): cat=1  <TODO: reverse layout>
// 60111 (0xeacf): cat=1  <TODO: reverse layout>
// 60112 (0xead0): cat=1  <TODO: reverse layout>
// 60115 (0xead3): cat=0  <TODO: reverse layout>
// 60116 (0xead4): cat=0  <TODO: reverse layout>
// 60117 (0xead5): cat=0  <TODO: reverse layout>
// 60118 (0xead6): cat=0  <TODO: reverse layout>
// 60119 (0xead7): cat=0  <TODO: reverse layout>
// 60120 (0xead8): cat=0  <TODO: reverse layout>
// 60121 (0xead9): cat=0  <TODO: reverse layout>
// 60122 (0xeada): cat=0  <TODO: reverse layout>
// 60123 (0xeadb): cat=0  <TODO: reverse layout>
// 60124 (0xeadc): cat=0  <TODO: reverse layout>
// 60125 (0xeadd): cat=0  <TODO: reverse layout>
// 60126 (0xeade): cat=0  <TODO: reverse layout>
// 60127 (0xeadf): cat=2  <TODO: reverse layout>
// 60128 (0xeae0): cat=2  <TODO: reverse layout>
// 60129 (0xeae1): cat=0  <TODO: reverse layout>
// 60131 (0xeae3): cat=0  <TODO: reverse layout>
// 60132 (0xeae4): cat=0  <TODO: reverse layout>
// 60133 (0xeae5): cat=0  <TODO: reverse layout>
// 60134 (0xeae6): cat=0  <TODO: reverse layout>
// 60135 (0xeae7): cat=0  <TODO: reverse layout>
// 60137 (0xeae9): cat=0  <TODO: reverse layout>
// 60138 (0xeaea): cat=0  <TODO: reverse layout>
// 60139 (0xeaeb): cat=0  <TODO: reverse layout>
// 60140 (0xeaec): cat=0  <TODO: reverse layout>
// 60141 (0xeaed): cat=0  <TODO: reverse layout>
// 60142 (0xeaee): cat=0  <TODO: reverse layout>
// 60143 (0xeaef): cat=0  <TODO: reverse layout>
// 60144 (0xeaf0): cat=0  <TODO: reverse layout>
// 60147 (0xeaf3): cat=0  <TODO: reverse layout>
// 60149 (0xeaf5): cat=0  <TODO: reverse layout>
// 60150 (0xeaf6): cat=0  <TODO: reverse layout>
// 60152 (0xeaf8): cat=0  <TODO: reverse layout>
// 60153 (0xeaf9): cat=0  <TODO: reverse layout>
// 60154 (0xeafa): cat=0  <TODO: reverse layout>
// 60155 (0xeafb): cat=0  <TODO: reverse layout>
// 60156 (0xeafc): cat=0  <TODO: reverse layout>
// 60157 (0xeafd): cat=0  <TODO: reverse layout>
// 60158 (0xeafe): cat=0  <TODO: reverse layout>
// 60159 (0xeaff): cat=0  <TODO: reverse layout>
// 60160 (0xeb00): cat=0  <TODO: reverse layout>
// 60161 (0xeb01): cat=0  <TODO: reverse layout>
// 60162 (0xeb02): cat=0  <TODO: reverse layout>
// 60163 (0xeb03): cat=0  <TODO: reverse layout>
// 60164 (0xeb04): cat=0  <TODO: reverse layout>
// 60165 (0xeb05): cat=0  <TODO: reverse layout>
// 60166 (0xeb06): cat=0  <TODO: reverse layout>
// 60168 (0xeb08): cat=0  <TODO: reverse layout>
// 60169 (0xeb09): cat=0  <TODO: reverse layout>
// 60170 (0xeb0a): cat=0  <TODO: reverse layout>
// 60171 (0xeb0b): cat=0  <TODO: reverse layout>
// 60172 (0xeb0c): cat=0  <TODO: reverse layout>
// 60173 (0xeb0d): cat=0  <TODO: reverse layout>
// 60174 (0xeb0e): cat=0  <TODO: reverse layout>
// 60175 (0xeb0f): cat=0  <TODO: reverse layout>
// 60176 (0xeb10): cat=0  <TODO: reverse layout>
// 60177 (0xeb11): cat=2  <TODO: reverse layout>
// 60180 (0xeb14): cat=1  <TODO: reverse layout>
// 60181 (0xeb15): cat=1  <TODO: reverse layout>
// 60182 (0xeb16): cat=1  <TODO: reverse layout>
// 60183 (0xeb17): cat=1  <TODO: reverse layout>
// 60184 (0xeb18): cat=1  <TODO: reverse layout>
// 60185 (0xeb19): cat=1  <TODO: reverse layout>
// 60186 (0xeb1a): cat=1  <TODO: reverse layout>
// 60187 (0xeb1b): cat=1  <TODO: reverse layout>
// 60188 (0xeb1c): cat=1  <TODO: reverse layout>
// 60189 (0xeb1d): cat=1  <TODO: reverse layout>
// 60191 (0xeb1f): cat=1  <TODO: reverse layout>
// 60193 (0xeb21): cat=1  <TODO: reverse layout>
// 60195 (0xeb23): cat=1  <TODO: reverse layout>
// 60196 (0xeb24): cat=1  <TODO: reverse layout>
// 60197 (0xeb25): cat=1  <TODO: reverse layout>
// 60198 (0xeb26): cat=1  <TODO: reverse layout>
// 60199 (0xeb27): cat=1  <TODO: reverse layout>
// 60200 (0xeb28): cat=1  <TODO: reverse layout>
// 60201 (0xeb29): cat=1  <TODO: reverse layout>
// 60202 (0xeb2a): cat=1  <TODO: reverse layout>
// 60203 (0xeb2b): cat=1  <TODO: reverse layout>
// 60204 (0xeb2c): cat=1  <TODO: reverse layout>
// 60205 (0xeb2d): cat=1  <TODO: reverse layout>
// 60206 (0xeb2e): cat=1  <TODO: reverse layout>
// 60207 (0xeb2f): cat=0  <TODO: reverse layout>
// 60208 (0xeb30): cat=0  <TODO: reverse layout>
// 60209 (0xeb31): cat=0  <TODO: reverse layout>
// 60211 (0xeb33): cat=0  <TODO: reverse layout>
// 60212 (0xeb34): cat=2  <TODO: reverse layout>
// 60213 (0xeb35): cat=0  <TODO: reverse layout>
// 60214 (0xeb36): cat=1  <TODO: reverse layout>
// 60215 (0xeb37): cat=1  <TODO: reverse layout>
// 60216 (0xeb38): cat=1  <TODO: reverse layout>
// 60217 (0xeb39): cat=1  <TODO: reverse layout>
// 60218 (0xeb3a): cat=0  <TODO: reverse layout>
// 60219 (0xeb3b): cat=0  <TODO: reverse layout>
// 60220 (0xeb3c): cat=0  <TODO: reverse layout>
// 60221 (0xeb3d): cat=0  <TODO: reverse layout>
// 60222 (0xeb3e): cat=0  <TODO: reverse layout>
// 60223 (0xeb3f): cat=0  <TODO: reverse layout>
// 60224 (0xeb40): cat=0  <TODO: reverse layout>
// 60225 (0xeb41): cat=0  <TODO: reverse layout>
// 60226 (0xeb42): cat=0  <TODO: reverse layout>
// 60227 (0xeb43): cat=0  <TODO: reverse layout>
// 60228 (0xeb44): cat=0  <TODO: reverse layout>
// 60229 (0xeb45): cat=0  <TODO: reverse layout>
// 60230 (0xeb46): cat=0  <TODO: reverse layout>
// 60231 (0xeb47): cat=1  <TODO: reverse layout>
// 60232 (0xeb48): cat=1  <TODO: reverse layout>
// 60233 (0xeb49): cat=1  <TODO: reverse layout>
// 60234 (0xeb4a): cat=2  <TODO: reverse layout>
// 60236 (0xeb4c): cat=0  <TODO: reverse layout>
// 60237 (0xeb4d): cat=0  <TODO: reverse layout>
// 60238 (0xeb4e): cat=0  <TODO: reverse layout>
// 60239 (0xeb4f): cat=2  <TODO: reverse layout>
// 60240 (0xeb50): cat=2  <TODO: reverse layout>
// 60241 (0xeb51): cat=0  <TODO: reverse layout>
// 60242 (0xeb52): cat=0  <TODO: reverse layout>
// 60243 (0xeb53): cat=0  <TODO: reverse layout>
// 60244 (0xeb54): cat=1  <TODO: reverse layout>
// 60246 (0xeb56): cat=1  <TODO: reverse layout>
// 60247 (0xeb57): cat=1  <TODO: reverse layout>
// 60248 (0xeb58): cat=1  <TODO: reverse layout>
// 60249 (0xeb59): cat=1  <TODO: reverse layout>
// 60250 (0xeb5a): cat=2  <TODO: reverse layout>
// 60251 (0xeb5b): cat=0  <TODO: reverse layout>
// 60252 (0xeb5c): cat=0  <TODO: reverse layout>
// 60253 (0xeb5d): cat=0  <TODO: reverse layout>
// 60254 (0xeb5e): cat=0  <TODO: reverse layout>
// 60255 (0xeb5f): cat=1  <TODO: reverse layout>
// 60256 (0xeb60): cat=1  <TODO: reverse layout>
// 60257 (0xeb61): cat=0  <TODO: reverse layout>
// 60259 (0xeb63): cat=0  <TODO: reverse layout>
// 60260 (0xeb64): cat=1  <TODO: reverse layout>
// 60261 (0xeb65): cat=0  <TODO: reverse layout>
// 60262 (0xeb66): cat=0  <TODO: reverse layout>
// 60263 (0xeb67): cat=0  <TODO: reverse layout>
// 60264 (0xeb68): cat=1  <TODO: reverse layout>
// 60265 (0xeb69): cat=2  <TODO: reverse layout>
// 60266 (0xeb6a): cat=2  <TODO: reverse layout>
// 60267 (0xeb6b): cat=2  <TODO: reverse layout>
// 60268 (0xeb6c): cat=1  <TODO: reverse layout>
// 60269 (0xeb6d): cat=1  <TODO: reverse layout>
// 60270 (0xeb6e): cat=1  <TODO: reverse layout>
// 60271 (0xeb6f): cat=0  <TODO: reverse layout>
// 60273 (0xeb71): cat=0  <TODO: reverse layout>
// 60274 (0xeb72): cat=1  <TODO: reverse layout>
// 60276 (0xeb74): cat=1  <TODO: reverse layout>
// 60277 (0xeb75): cat=1  <TODO: reverse layout>
// 60278 (0xeb76): cat=1  <TODO: reverse layout>
// 60279 (0xeb77): cat=0  <TODO: reverse layout>
// 60280 (0xeb78): cat=0  <TODO: reverse layout>
// 60281 (0xeb79): cat=0  <TODO: reverse layout>
// 60282 (0xeb7a): cat=0  <TODO: reverse layout>
// 60283 (0xeb7b): cat=0  <TODO: reverse layout>
// 60284 (0xeb7c): cat=0  <TODO: reverse layout>
// 60285 (0xeb7d): cat=0  <TODO: reverse layout>
// 60286 (0xeb7e): cat=0  <TODO: reverse layout>
// 60287 (0xeb7f): cat=0  <TODO: reverse layout>
// 60288 (0xeb80): cat=0  <TODO: reverse layout>
// 60289 (0xeb81): cat=0  <TODO: reverse layout>
// 60290 (0xeb82): cat=1  <TODO: reverse layout>
// 60291 (0xeb83): cat=1  <TODO: reverse layout>
// 60292 (0xeb84): cat=1  <TODO: reverse layout>
// 60293 (0xeb85): cat=0  <TODO: reverse layout>
// 60294 (0xeb86): cat=0  <TODO: reverse layout>
// 60295 (0xeb87): cat=0  <TODO: reverse layout>
// 60296 (0xeb88): cat=1  <TODO: reverse layout>
// 60297 (0xeb89): cat=0  <TODO: reverse layout>
// 60298 (0xeb8a): cat=1  <TODO: reverse layout>
// 60299 (0xeb8b): cat=1  <TODO: reverse layout>
// 60301 (0xeb8d): cat=0  <TODO: reverse layout>
// 60302 (0xeb8e): cat=2  <TODO: reverse layout>
// 60303 (0xeb8f): cat=2  <TODO: reverse layout>
// 60304 (0xeb90): cat=0  <TODO: reverse layout>
// 60305 (0xeb91): cat=0  <TODO: reverse layout>
// 60306 (0xeb92): cat=1  <TODO: reverse layout>
// 60307 (0xeb93): cat=0  <TODO: reverse layout>
// 60308 (0xeb94): cat=0  <TODO: reverse layout>
// 60309 (0xeb95): cat=1  <TODO: reverse layout>
// 60310 (0xeb96): cat=1  <TODO: reverse layout>
// 60311 (0xeb97): cat=1  <TODO: reverse layout>
// 60312 (0xeb98): cat=2  <TODO: reverse layout>
// 60313 (0xeb99): cat=0  <TODO: reverse layout>
