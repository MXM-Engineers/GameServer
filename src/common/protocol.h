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

	struct CharacterSelectInfo
	{
		LocalActorID characterID;
		CreatureIndex creatureIndex; // can be different when bots are picking (creature index specifically for bot variants)
		SkillID skillSlot1;
		SkillID skillSlot2;
	};

	UserID userID;
	u16 characterSelectInfos_count;
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

// ==== Auto-extracted from MXMClient_DP_p3.exe (client packet validators + loggers) ====
// Field names/types from the client's packet logger (wide-char format strings); sizes from the validators.
namespace Sv {

PUSH_PACKED
struct SN_UNKNOWN_62001
{
	enum { NET_ID = 62001 };
	i64 field_0; // 8 bytes
	u32 field_1; // 4 bytes
	// logger none
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62001, 12);

PUSH_PACKED
struct SN_PlayerTitleSelect
{
	enum { NET_ID = 62008 };
	u32 playerID; // 4 bytes
	u32 nDisplayTitleIndex; // 4 bytes
	u32 nStatTitleIndex; // 4 bytes
	// logger 0x99e794
};
POP_PACKED
ASSERT_SIZE(SN_PlayerTitleSelect, 12);

PUSH_PACKED
struct SN_DoConnectSpectateServer
{
	enum { NET_ID = 62012 };
	u16 dwPort; // 2 bytes
	u8 dwIP[4]; // 4 bytes
	u32 idcHash; // 4 bytes
	// nickname: variable-size (string/vector)
	u16 instantKey; // 2 bytes
	u32 unk_0; // 4 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x991667
};
POP_PACKED

PUSH_PACKED
struct SN_UNKNOWN_62013
{
	enum { NET_ID = 62013 };
	u16 field_0; // 2 bytes
	u32 field_1; // 4 bytes
	// logger none
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62013, 6);

PUSH_PACKED
struct SN_UNKNOWN_62014
{
	enum { NET_ID = 62014 };
	// no fixed fields (payloadless or unresolved)
	// logger none
};
POP_PACKED

PUSH_PACKED
struct SQ_PrepareHandoverToChannel
{
	enum { NET_ID = 62015 };
	// no fixed fields (payloadless or unresolved)
	// logger 0x9a8fb1
};
POP_PACKED

PUSH_PACKED
struct SN_UNKNOWN_62016
{
	enum { NET_ID = 62016 };
	// no fixed fields (payloadless or unresolved)
	// logger none
};
POP_PACKED

PUSH_PACKED
struct SN_SuspendedToCity
{
	enum { NET_ID = 62017 };
	u32 stageIdx; // 4 bytes
	u8 gameType; // 1 bytes
	u8 gameDefType; // 1 bytes
	// logger 0x9a696f
};
POP_PACKED
ASSERT_SIZE(SN_SuspendedToCity, 6);

PUSH_PACKED
struct SA_ReturnToCity
{
	enum { NET_ID = 62018 };
	u32 errcode; // 4 bytes
	// logger 0x987925
};
POP_PACKED
ASSERT_SIZE(SA_ReturnToCity, 4);

PUSH_PACKED
struct SN_EnterChannel
{
	enum { NET_ID = 62019 };
	u32 chID; // 4 bytes
	u32 channelType; // 4 bytes
	// logger 0x991f2f
};
POP_PACKED
ASSERT_SIZE(SN_EnterChannel, 8);

PUSH_PACKED
struct SN_LeaveChannel
{
	enum { NET_ID = 62020 };
	u32 chID; // 4 bytes
	// logger 0x9994fe
};
POP_PACKED
ASSERT_SIZE(SN_LeaveChannel, 4);

PUSH_PACKED
struct SN_GameRestriction
{
	enum { NET_ID = 62021 };
	u32 timeToReleaseSec; // 4 bytes
	// logger 0x99592e
};
POP_PACKED
ASSERT_SIZE(SN_GameRestriction, 4);

PUSH_PACKED
struct SN_ShopItemList
{
	enum { NET_ID = 62022 };
	u32 field_0; // 4 bytes
	u32 field_1; // 4 bytes
	u32 field_2; // 4 bytes
	u32 field_3; // 4 bytes
	u32 field_4; // 4 bytes
	// logger 0x9a403f
};
POP_PACKED
ASSERT_SIZE(SN_ShopItemList, 20);

PUSH_PACKED
struct SA_SellShopItem
{
	enum { NET_ID = 62023 };
	u32 Result; // 4 bytes
	u32 sellAmount; // 4 bytes
	i64 unk_0; // 8 bytes
	// logger 0x98836f
};
POP_PACKED
ASSERT_SIZE(SA_SellShopItem, 16);

PUSH_PACKED
struct SN_CreateGroundItem
{
	enum { NET_ID = 62024 };
	u8 field_0; // 1 bytes
	u32 field_1; // 4 bytes
	u32 field_2; // 4 bytes
	u32 field_3; // 4 bytes
	u32 field_4; // 4 bytes
	u32 field_5; // 4 bytes
	u32 field_6; // 4 bytes
	u32 field_7; // 4 bytes
	u32 field_8; // 4 bytes
	u32 field_9; // 4 bytes
	u32 field_10; // 4 bytes
	u32 field_11; // 4 bytes
	u32 field_12; // 4 bytes
	u32 field_13; // 4 bytes
	u8 field_14; // 1 bytes
	u8 field_15; // 1 bytes
	// logger none
};
POP_PACKED
ASSERT_SIZE(SN_CreateGroundItem, 55);

PUSH_PACKED
struct SN_GameModifyActor
{
	enum { NET_ID = 62027 };
	u32 oldID; // 4 bytes
	u32 newID; // 4 bytes
	u32 nType; // 4 bytes
	u32 nIDX; // 4 bytes
	u32 dwLocalID; // 4 bytes
	// p3nPos: variable-size (string/vector)
	// p3nDir: variable-size (string/vector)
	u32 SpawnType; // 4 bytes
	u32 actionState; // 4 bytes
	u32 ownerID; // 4 bytes
	u32 bDirectionToNearPC; // 4 bytes
	u32 nAIWanderDistOverride; // 4 bytes
	u32 tagID; // 4 bytes
	u32 faction; // 4 bytes
	u32 classType; // 4 bytes
	u32 skinIndex; // 4 bytes
	u8 seed; // 1 bytes
	// initStat: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	u32 unk_4; // 4 bytes
	u32 unk_5; // 4 bytes
	u16 unk_6; // 2 bytes
	u8 unk_7; // 1 bytes
	u32 unk_8; // 4 bytes
	u16 unk_9; // 2 bytes
	u8 unk_10; // 1 bytes
	u32 unk_11; // 4 bytes
	// logger 0x994d1b
};
POP_PACKED
ASSERT_SIZE(SN_GameModifyActor, 95);

PUSH_PACKED
struct SA_ItemMerge
{
	enum { NET_ID = 62032 };
	u32 nEquipKey; // 4 bytes
	u32 nCount; // 4 bytes
	u32 nEquipKey2; // 4 bytes
	u32 nCount2; // 4 bytes
	u32 nErrorType; // 4 bytes
	// logger 0x983bd3
};
POP_PACKED
ASSERT_SIZE(SA_ItemMerge, 20);

PUSH_PACKED
struct SN_PrecastSkill
{
	enum { NET_ID = 62034 };
	u32 entity; // 4 bytes
	u32 ret; // 4 bytes
	u32 skillIndex; // 4 bytes
	u8 bSyncMyPosition; // 1 bytes
	// posStruct: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	u32 unk_4; // 4 bytes
	u32 unk_5; // 4 bytes
	u32 unk_6; // 4 bytes
	u32 unk_7; // 4 bytes
	u32 unk_8; // 4 bytes
	u32 unk_9; // 4 bytes
	u32 unk_10; // 4 bytes
	u32 unk_11; // 4 bytes
	u32 unk_12; // 4 bytes
	// logger 0x99ec9f
};
POP_PACKED
ASSERT_SIZE(SN_PrecastSkill, 65);

PUSH_PACKED
struct SN_SwitchOnToggleSkill
{
	enum { NET_ID = 62037 };
	u32 entity; // 4 bytes
	u32 ret; // 4 bytes
	u32 skillIndex; // 4 bytes
	u32 param1; // 4 bytes
	u32 unk_0; // 4 bytes
	// logger 0x9a6b8a
};
POP_PACKED
ASSERT_SIZE(SN_SwitchOnToggleSkill, 20);

PUSH_PACKED
struct SN_SwitchOffToggleSkill
{
	enum { NET_ID = 62038 };
	u32 entity; // 4 bytes
	u32 ret; // 4 bytes
	u32 skillIndex; // 4 bytes
	u32 param1; // 4 bytes
	// logger 0x9a6a6b
};
POP_PACKED
ASSERT_SIZE(SN_SwitchOffToggleSkill, 16);

PUSH_PACKED
struct SN_CancelSkill
{
	enum { NET_ID = 62039 };
	u32 entity; // 4 bytes
	u32 ret; // 4 bytes
	u32 skillIndex; // 4 bytes
	// logger 0x98e7bb
};
POP_PACKED
ASSERT_SIZE(SN_CancelSkill, 12);

PUSH_PACKED
struct SA_PrecastSkill
{
	enum { NET_ID = 62040 };
	u32 entity; // 4 bytes
	u32 ret; // 4 bytes
	u32 skillIndex; // 4 bytes
	// logger 0x98601b
};
POP_PACKED
ASSERT_SIZE(SA_PrecastSkill, 12);

PUSH_PACKED
struct SA_ExecuteSkill
{
	enum { NET_ID = 62042 };
	u32 entity; // 4 bytes
	u32 ret; // 4 bytes
	u32 skillIndex; // 4 bytes
	// logger 0x97f9b4
};
POP_PACKED
ASSERT_SIZE(SA_ExecuteSkill, 12);

PUSH_PACKED
struct SA_SwitchOnToggleSkill
{
	enum { NET_ID = 62043 };
	u32 entity; // 4 bytes
	u32 ret; // 4 bytes
	u32 skillIndex; // 4 bytes
	u32 actionState; // 4 bytes
	// logger 0x989630
};
POP_PACKED
ASSERT_SIZE(SA_SwitchOnToggleSkill, 16);

PUSH_PACKED
struct SA_SwitchOffToggleSkill
{
	enum { NET_ID = 62044 };
	u32 entity; // 4 bytes
	u32 ret; // 4 bytes
	u32 skillIndex; // 4 bytes
	u32 actionState; // 4 bytes
	// logger 0x989511
};
POP_PACKED
ASSERT_SIZE(SA_SwitchOffToggleSkill, 16);

PUSH_PACKED
struct SA_CancelSkill
{
	enum { NET_ID = 62045 };
	u32 entity; // 4 bytes
	u32 ret; // 4 bytes
	u32 skillIndex; // 4 bytes
	// logger 0x97de79
};
POP_PACKED
ASSERT_SIZE(SA_CancelSkill, 12);

PUSH_PACKED
struct SN_ChangeSkillSlot
{
	enum { NET_ID = 62046 };
	u32 entity; // 4 bytes
	u32 slotIndex; // 4 bytes
	u32 skillIndex; // 4 bytes
	u32 resetBaseComboCooltimeID; // 4 bytes
	u32 param; // 4 bytes
	// logger 0x98ef7b
};
POP_PACKED
ASSERT_SIZE(SN_ChangeSkillSlot, 20);

PUSH_PACKED
struct SN_UpdateTargetGraphMove
{
	enum { NET_ID = 62049 };
	u32 characterID; // 4 bytes
	// curPos: variable-size (string/vector)
	u32 targetID; // 4 bytes
	// targetPos: variable-size (string/vector)
	u32 curActionID; // 4 bytes
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	u32 unk_4; // 4 bytes
	u32 unk_5; // 4 bytes
	// logger 0x9a7fff
};
POP_PACKED
ASSERT_SIZE(SN_UpdateTargetGraphMove, 36);

PUSH_PACKED
struct SA_ItemChange
{
	enum { NET_ID = 62053 };
	u32 nEquipKey; // 4 bytes
	u32 nESlot; // 4 bytes
	u32 nErrorType; // 4 bytes
	// logger 0x98361d
};
POP_PACKED
ASSERT_SIZE(SA_ItemChange, 12);

PUSH_PACKED
struct SA_ItemDelete
{
	enum { NET_ID = 62054 };
	u32 result; // 4 bytes
	// logger 0x9838b7
};
POP_PACKED
ASSERT_SIZE(SA_ItemDelete, 4);

PUSH_PACKED
struct SA_WarehouseItemDelete
{
	enum { NET_ID = 62055 };
	u32 result; // 4 bytes
	// logger 0x98a425
};
POP_PACKED
ASSERT_SIZE(SA_WarehouseItemDelete, 4);

PUSH_PACKED
struct SN_UpdateStat
{
	enum { NET_ID = 62056 };
	u32 characterID; // 4 bytes
	u32 _STAT_TYPE; // 4 bytes
	f32 _CUR; // 4 bytes
	f32 _MAX; // 4 bytes
	u32 _ReasonCode; // 4 bytes
	// logger 0x9a7ea9
};
POP_PACKED
ASSERT_SIZE(SN_UpdateStat, 20);

PUSH_PACKED
struct SN_CCoin
{
	enum { NET_ID = 62058 };
	i64 amount; // 8 bytes
	u32 nReason; // 4 bytes
	// logger 0x9905ca
};
POP_PACKED
ASSERT_SIZE(SN_CCoin, 12);

PUSH_PACKED
struct SN_DeadAck
{
	enum { NET_ID = 62062 };
	u32 sourceID; // 4 bytes
	u32 targetID; // 4 bytes
	u32 targetDocIndex; // 4 bytes
	u32 dwRemoteID; // 4 bytes
	u32 dwRemoteDocIndex; // 4 bytes
	u8 isPast; // 1 bytes
	// logger 0x99071b
};
POP_PACKED
ASSERT_SIZE(SN_DeadAck, 21);

PUSH_PACKED
struct SN_DeadDamageInfo
{
	enum { NET_ID = 62063 };
	// killer: variable-size (string/vector)
	u16 durationTime; // 2 bytes
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	u8 unk_4; // 1 bytes
	u8 unk_5; // 1 bytes
	u32 unk_6; // 4 bytes
	u16 unk_7; // 2 bytes
	u32 unk_8; // 4 bytes
	u32 unk_9; // 4 bytes
	u32 unk_10; // 4 bytes
	u32 unk_11; // 4 bytes
	u8 unk_12; // 1 bytes
	u8 unk_13; // 1 bytes
	u32 unk_14; // 4 bytes
	u32 unk_15; // 4 bytes
	// logger 0x99088a
};
POP_PACKED
ASSERT_SIZE(SN_DeadDamageInfo, 52);

PUSH_PACKED
struct SA_BuyShopItem
{
	enum { NET_ID = 62065 };
	u32 Result; // 4 bytes
	u32 m_IDX; // 4 bytes
	// logger 0x97dadf
};
POP_PACKED
ASSERT_SIZE(SA_BuyShopItem, 8);

PUSH_PACKED
struct SA_BuyCshopItem
{
	enum { NET_ID = 62066 };
	u32 result; // 4 bytes
	u32 itemIndex; // 4 bytes
	// logger 0x97d7c8
};
POP_PACKED
ASSERT_SIZE(SA_BuyCshopItem, 8);

PUSH_PACKED
struct SA_GiftFriendsAvailable
{
	enum { NET_ID = 62067 };
	u32 result; // 4 bytes
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x982dd1
};
POP_PACKED

PUSH_PACKED
struct SA_GiftFriendCharsAndSkins
{
	enum { NET_ID = 62068 };
	u32 result; // 4 bytes
	// nickname: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x982f18
};
POP_PACKED

PUSH_PACKED
struct SA_BuyGift
{
	enum { NET_ID = 62069 };
	u32 result; // 4 bytes
	// logger 0x97d899
};
POP_PACKED
ASSERT_SIZE(SA_BuyGift, 4);

PUSH_PACKED
struct SN_PlayerSkillUpdate
{
	enum { NET_ID = 62070 };
	u32 characterID; // 4 bytes
	// stSkillSlot: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u8 unk_2; // 1 bytes
	u16 unk_3; // 2 bytes
	u32 unk_4; // 4 bytes
	u32 unk_5; // 4 bytes
	u8 unk_6; // 1 bytes
	u8 unk_7; // 1 bytes
	// logger 0x99df3c
};
POP_PACKED
ASSERT_SIZE(SN_PlayerSkillUpdate, 25);

PUSH_PACKED
struct SN_GameAreaMove
{
	enum { NET_ID = 62071 };
	u32 dwAreaIDX; // 4 bytes
	// logger 0x99376b
};
POP_PACKED
ASSERT_SIZE(SN_GameAreaMove, 4);

PUSH_PACKED
struct SN_TrespassReady
{
	enum { NET_ID = 62074 };
	u8 isGameStarted; // 1 bytes
	u8 isPaused; // 1 bytes
	u32 readyElapsedMS; // 4 bytes
	// logger 0x9a77ff
};
POP_PACKED
ASSERT_SIZE(SN_TrespassReady, 6);

PUSH_PACKED
struct SN_TrespassGameStart
{
	enum { NET_ID = 62077 };
	u32 m_PlayTimeMS; // 4 bytes
	// logger 0x9a7756
};
POP_PACKED
ASSERT_SIZE(SN_TrespassGameStart, 4);

PUSH_PACKED
struct SA_ItemUse
{
	enum { NET_ID = 62078 };
	u32 nErrorType; // 4 bytes
	u32 usedItemDocIndex; // 4 bytes
	i64 goldAmount; // 8 bytes
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	// logger 0x983f8d
};
POP_PACKED
ASSERT_SIZE(SA_ItemUse, 28);

PUSH_PACKED
struct SN_PvpResult
{
	enum { NET_ID = 62079 };
	u32 gameEndReason; // 4 bytes
	u32 m_playTime; // 4 bytes
	// m_resultReward: variable-size (string/vector)
	u32 tierGameResult; // 4 bytes
	u32 tierTypeResult; // 4 bytes
	u32 curTierPoint; // 4 bytes
	u32 deltaTierPoint; // 4 bytes
	u16 deltaGuildRp; // 2 bytes
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	u16 unk_3; // 2 bytes
	u32 unk_4; // 4 bytes
	u32 unk_5; // 4 bytes
	u16 unk_6; // 2 bytes
	u32 unk_7; // 4 bytes
	u32 unk_8; // 4 bytes
	u32 unk_9; // 4 bytes
	u32 unk_10; // 4 bytes
	u32 unk_11; // 4 bytes
	u8 unk_12; // 1 bytes
	u16 unk_13; // 2 bytes
	u16 unk_14; // 2 bytes
	u16 unk_15; // 2 bytes
	u32 unk_16; // 4 bytes
	u32 unk_17; // 4 bytes
	u32 unk_18; // 4 bytes
	u8 unk_19; // 1 bytes
	u8 unk_20; // 1 bytes
	u16 unk_21; // 2 bytes
	u16 unk_22; // 2 bytes
	u16 unk_23; // 2 bytes
	u32 unk_24; // 4 bytes
	// logger 0x99fd9b
};
POP_PACKED
ASSERT_SIZE(SN_PvpResult, 101);

PUSH_PACKED
struct SN_PvpResultScoreDeathmatch
{
	enum { NET_ID = 62080 };
	u32 field_0; // 4 bytes
	u32 field_1; // 4 bytes
	u32 field_2; // 4 bytes
	u16 field_3; // 2 bytes
	u32 field_4; // 4 bytes
	u8 field_5; // 1 bytes
	u32 field_6; // 4 bytes
	u16 field_7; // 2 bytes
	u32 field_8; // 4 bytes
	u32 field_9; // 4 bytes
	u8 field_10; // 1 bytes
	u8 field_11; // 1 bytes
	u8 field_12; // 1 bytes
	u8 field_13; // 1 bytes
	u8 field_14; // 1 bytes
	u32 field_15; // 4 bytes
	u32 field_16; // 4 bytes
	u32 field_17; // 4 bytes
	u32 field_18; // 4 bytes
	u32 field_19; // 4 bytes
	u32 field_20; // 4 bytes
	u32 field_21; // 4 bytes
	u32 field_22; // 4 bytes
	// logger 0x9a00c6
};
POP_PACKED
ASSERT_SIZE(SN_PvpResultScoreDeathmatch, 70);

PUSH_PACKED
struct SN_PvpResultScoreOccupy
{
	enum { NET_ID = 62081 };
	u32 field_0; // 4 bytes
	u32 field_1; // 4 bytes
	u32 field_2; // 4 bytes
	u16 field_3; // 2 bytes
	u32 field_4; // 4 bytes
	u8 field_5; // 1 bytes
	u16 field_6; // 2 bytes
	u32 field_7; // 4 bytes
	u32 field_8; // 4 bytes
	u8 field_9; // 1 bytes
	u8 field_10; // 1 bytes
	u8 field_11; // 1 bytes
	u8 field_12; // 1 bytes
	u8 field_13; // 1 bytes
	u32 field_14; // 4 bytes
	u32 field_15; // 4 bytes
	u32 field_16; // 4 bytes
	u32 field_17; // 4 bytes
	u32 field_18; // 4 bytes
	u32 field_19; // 4 bytes
	u32 field_20; // 4 bytes
	u32 field_21; // 4 bytes
	u8 field_22; // 1 bytes
	u16 field_23; // 2 bytes
	// logger 0x9a04bf
};
POP_PACKED
ASSERT_SIZE(SN_PvpResultScoreOccupy, 69);

PUSH_PACKED
struct SN_PvpResultScoreGot
{
	enum { NET_ID = 62082 };
	u32 field_0; // 4 bytes
	u32 field_1; // 4 bytes
	u16 field_2; // 2 bytes
	u32 field_3; // 4 bytes
	u32 field_4; // 4 bytes
	u16 field_5; // 2 bytes
	u32 field_6; // 4 bytes
	u8 field_7; // 1 bytes
	u8 field_8; // 1 bytes
	u32 field_9; // 4 bytes
	u16 field_10; // 2 bytes
	u8 field_11; // 1 bytes
	u8 field_12; // 1 bytes
	u16 field_13; // 2 bytes
	u8 field_14; // 1 bytes
	u8 field_15; // 1 bytes
	u16 field_16; // 2 bytes
	u32 field_17; // 4 bytes
	u32 field_18; // 4 bytes
	u8 field_19; // 1 bytes
	u8 field_20; // 1 bytes
	u8 field_21; // 1 bytes
	u8 field_22; // 1 bytes
	u8 field_23; // 1 bytes
	u32 field_24; // 4 bytes
	u32 field_25; // 4 bytes
	u32 field_26; // 4 bytes
	u32 field_27; // 4 bytes
	u32 field_28; // 4 bytes
	u32 field_29; // 4 bytes
	u32 field_30; // 4 bytes
	u32 field_31; // 4 bytes
	u32 field_32; // 4 bytes
	u32 field_33; // 4 bytes
	u8 field_34; // 1 bytes
	u8 field_35; // 1 bytes
	u8 field_36; // 1 bytes
	u8 field_37; // 1 bytes
	u8 field_38; // 1 bytes
	u8 field_39; // 1 bytes
	u8 field_40; // 1 bytes
	u16 field_41; // 2 bytes
	u32 field_42; // 4 bytes
	u32 field_43; // 4 bytes
	u8 field_44; // 1 bytes
	u8 field_45; // 1 bytes
	u8 field_46; // 1 bytes
	u8 field_47; // 1 bytes
	u8 field_48; // 1 bytes
	u32 field_49; // 4 bytes
	u32 field_50; // 4 bytes
	u32 field_51; // 4 bytes
	u32 field_52; // 4 bytes
	u32 field_53; // 4 bytes
	u32 field_54; // 4 bytes
	u32 field_55; // 4 bytes
	u32 field_56; // 4 bytes
	u32 field_57; // 4 bytes
	u32 field_58; // 4 bytes
	u8 field_59; // 1 bytes
	// logger 0x9a02ba
};
POP_PACKED
ASSERT_SIZE(SN_PvpResultScoreGot, 156);

PUSH_PACKED
struct SN_PvpResultScoreSport
{
	enum { NET_ID = 62083 };
	u32 field_0; // 4 bytes
	u32 field_1; // 4 bytes
	u32 field_2; // 4 bytes
	u16 field_3; // 2 bytes
	u32 field_4; // 4 bytes
	u32 field_5; // 4 bytes
	u32 field_6; // 4 bytes
	u8 field_7; // 1 bytes
	u32 field_8; // 4 bytes
	u8 field_9; // 1 bytes
	u8 field_10; // 1 bytes
	u32 field_11; // 4 bytes
	// logger 0x9a06b3
};
POP_PACKED
ASSERT_SIZE(SN_PvpResultScoreSport, 37);

PUSH_PACKED
struct SA_GetGroundItem
{
	enum { NET_ID = 62085 };
	u32 playerID; // 4 bytes
	u32 itemID; // 4 bytes
	u32 itemISN; // 4 bytes
	u32 nErrorType; // 4 bytes
	// logger 0x980a92
};
POP_PACKED
ASSERT_SIZE(SA_GetGroundItem, 16);

PUSH_PACKED
struct SN_StageClearResult
{
	enum { NET_ID = 62086 };
	u32 m_playerID; // 4 bytes
	u32 isRandomRestart; // 4 bytes
	u32 m_StageIndex; // 4 bytes
	u16 m_KillCount; // 2 bytes
	u8 m_DeathCount; // 1 bytes
	u32 m_PlayTime; // 4 bytes
	u32 m_ClearRank; // 4 bytes
	u32 m_CombatScore; // 4 bytes
	u32 m_baseRon; // 4 bytes
	u8 m_baseExp; // 1 bytes
	u32 m_attackScore; // 4 bytes
	u32 m_defenseScore; // 4 bytes
	u32 m_timeScore; // 4 bytes
	u32 m_deathTimeScore; // 4 bytes
	u32 m_totalScore; // 4 bytes
	u32 m_ranking; // 4 bytes
	u32 m_technicGrade; // 4 bytes
	// m_guildPointReward: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	u32 unk_4; // 4 bytes
	u32 unk_5; // 4 bytes
	u8 unk_6; // 1 bytes
	u32 unk_7; // 4 bytes
	u32 unk_8; // 4 bytes
	u8 unk_9; // 1 bytes
	u16 unk_10; // 2 bytes
	u16 unk_11; // 2 bytes
	u16 unk_12; // 2 bytes
	// logger 0x9a585a
};
POP_PACKED
ASSERT_SIZE(SN_StageClearResult, 100);

PUSH_PACKED
struct SN_DefenceClearResult
{
	enum { NET_ID = 62087 };
	u32 m_playerID; // 4 bytes
	u32 m_StageIndex; // 4 bytes
	u32 m_ClearStepIndex; // 4 bytes
	u16 m_ClearTime; // 2 bytes
	// m_pstResultReward: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	u32 unk_4; // 4 bytes
	u16 unk_5; // 2 bytes
	u32 unk_6; // 4 bytes
	u32 unk_7; // 4 bytes
	u32 unk_8; // 4 bytes
	u16 unk_9; // 2 bytes
	u32 unk_10; // 4 bytes
	u32 unk_11; // 4 bytes
	u16 unk_12; // 2 bytes
	u32 unk_13; // 4 bytes
	u32 unk_14; // 4 bytes
	u32 unk_15; // 4 bytes
	u32 unk_16; // 4 bytes
	u32 unk_17; // 4 bytes
	u8 unk_18; // 1 bytes
	u16 unk_19; // 2 bytes
	u16 unk_20; // 2 bytes
	u16 unk_21; // 2 bytes
	u32 unk_22; // 4 bytes
	u32 unk_23; // 4 bytes
	u32 unk_24; // 4 bytes
	u32 unk_25; // 4 bytes
	// logger 0x990a40
};
POP_PACKED
ASSERT_SIZE(SN_DefenceClearResult, 103);

PUSH_PACKED
struct SN_UNKNOWN_62088
{
	enum { NET_ID = 62088 };
	u32 field_0; // 4 bytes
	u32 field_1; // 4 bytes
	// logger none
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62088, 8);

PUSH_PACKED
struct SN_CommunityChat
{
	enum { NET_ID = 62092 };
	u32 m_ChatType; // 4 bytes
	u32 m_SendPlayerID; // 4 bytes
	// m_SendName: variable-size (string/vector)
	u16 m_SendStaffType; // 2 bytes
	// m_Msg: variable-size (string/vector)
	u8 m_DurationMS; // 1 bytes
	u16 unk_0; // 2 bytes
	u16 unk_1; // 2 bytes
	// NOTE: 2 variable-size field(s) after the fixed prefix
	// logger 0x98feaa
};
POP_PACKED

PUSH_PACKED
struct SN_GetGroundItem
{
	enum { NET_ID = 62093 };
	u32 dwPlayerID; // 4 bytes
	u32 dwItemIndex; // 4 bytes
	u32 oid; // 4 bytes
	// logger 0x995bd1
};
POP_PACKED
ASSERT_SIZE(SN_GetGroundItem, 12);

PUSH_PACKED
struct SN_DestroyGroundItem
{
	enum { NET_ID = 62094 };
	u32 itemID; // 4 bytes
	// logger 0x9911f2
};
POP_PACKED
ASSERT_SIZE(SN_DestroyGroundItem, 4);

PUSH_PACKED
struct SN_GroundItemsSnapshot
{
	enum { NET_ID = 62095 };
	u32 field_0; // 4 bytes
	u32 field_1; // 4 bytes
	u32 field_2; // 4 bytes
	u32 field_3; // 4 bytes
	u32 field_4; // 4 bytes
	u32 field_5; // 4 bytes
	u32 field_6; // 4 bytes
	u8 field_7; // 1 bytes
	u8 field_8; // 1 bytes
	u8 field_9; // 1 bytes
	u32 field_10; // 4 bytes
	u32 field_11; // 4 bytes
	u32 field_12; // 4 bytes
	// logger 0x996290
};
POP_PACKED
ASSERT_SIZE(SN_GroundItemsSnapshot, 43);

PUSH_PACKED
struct SA_WorldSetInfoList
{
	enum { NET_ID = 62096 };
	u32 field_0; // 4 bytes
	u32 field_1; // 4 bytes
	u32 field_2; // 4 bytes
	u32 field_3; // 4 bytes
	u32 field_4; // 4 bytes
	u32 field_5; // 4 bytes
	u32 field_6; // 4 bytes
	u32 field_7; // 4 bytes
	u32 field_8; // 4 bytes
	u32 field_9; // 4 bytes
	// logger 0x98aa49
};
POP_PACKED
ASSERT_SIZE(SA_WorldSetInfoList, 40);

PUSH_PACKED
struct SN_AntihackAuth
{
	enum { NET_ID = 62099 };
	u8 field_0; // 1 bytes
	// logger 0x98d43f
};
POP_PACKED
ASSERT_SIZE(SN_AntihackAuth, 1);

PUSH_PACKED
struct SN_MissionList
{
	enum { NET_ID = 62103 };
	u32 dailyMissionRemainTimeMS; // 4 bytes
	i64 weeklyMissionRemainTimeMS; // 8 bytes
	u16 monthlyMissionRemainTimeMS; // 2 bytes
	u8 unk_0; // 1 bytes
	u8 unk_1; // 1 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	u32 unk_4; // 4 bytes
	// logger 0x99b378
};
POP_PACKED
ASSERT_SIZE(SN_MissionList, 28);

PUSH_PACKED
struct SN_MissionUpdate
{
	enum { NET_ID = 62104 };
	// mission: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	i64 unk_1; // 8 bytes
	u16 unk_2; // 2 bytes
	u8 unk_3; // 1 bytes
	u8 unk_4; // 1 bytes
	// logger 0x99b545
};
POP_PACKED
ASSERT_SIZE(SN_MissionUpdate, 16);

PUSH_PACKED
struct SA_DailyMissionChange
{
	enum { NET_ID = 62105 };
	u32 result; // 4 bytes
	u32 step; // 4 bytes
	// mission: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	i64 unk_1; // 8 bytes
	u16 unk_2; // 2 bytes
	u8 unk_3; // 1 bytes
	u8 unk_4; // 1 bytes
	// logger 0x97ed26
};
POP_PACKED
ASSERT_SIZE(SA_DailyMissionChange, 24);

PUSH_PACKED
struct SN_ChangedUserGradeInfo
{
	enum { NET_ID = 62108 };
	// userGradeInfo: variable-size (string/vector)
	u8 activityPoint; // 1 bytes
	u8 activityRewardedState; // 1 bytes
	i64 unk_0; // 8 bytes
	u8 unk_1; // 1 bytes
	u16 unk_2; // 2 bytes
	u16 unk_3; // 2 bytes
	u32 unk_4; // 4 bytes
	u8 unk_5; // 1 bytes
	// logger 0x98eb49
};
POP_PACKED
ASSERT_SIZE(SN_ChangedUserGradeInfo, 20);

PUSH_PACKED
struct SN_StageRank
{
	enum { NET_ID = 62110 };
	u32 rank; // 4 bytes
	u32 timeScoreRank; // 4 bytes
	// logger 0x9a5dc8
};
POP_PACKED
ASSERT_SIZE(SN_StageRank, 8);

PUSH_PACKED
struct SN_GetPublicGroundItem
{
	enum { NET_ID = 62111 };
	u32 playerID; // 4 bytes
	u32 itemID; // 4 bytes
	// logger 0x995cc9
};
POP_PACKED
ASSERT_SIZE(SN_GetPublicGroundItem, 8);

PUSH_PACKED
struct SN_UpdateGamePlayerTagCooltime
{
	enum { NET_ID = 62114 };
	u32 playerID; // 4 bytes
	u32 tagCooltimeMS; // 4 bytes
	// logger 0x9a7cb7
};
POP_PACKED
ASSERT_SIZE(SN_UpdateGamePlayerTagCooltime, 8);

PUSH_PACKED
struct SN_UpdateCanCastSkillSlotUg
{
	enum { NET_ID = 62115 };
	u32 playerID; // 4 bytes
	u8 canCastSkillSlotUG; // 1 bytes
	// logger 0x9a79f5
};
POP_PACKED
ASSERT_SIZE(SN_UpdateCanCastSkillSlotUg, 5);

PUSH_PACKED
struct SN_ItemAcquisition
{
	enum { NET_ID = 62116 };
	u32 nReason; // 4 bytes
	// itemInfo: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	// logger 0x998c03
};
POP_PACKED
ASSERT_SIZE(SN_ItemAcquisition, 16);

PUSH_PACKED
struct SN_ItemUpdate
{
	enum { NET_ID = 62117 };
	u32 context; // 4 bytes
	u8 unk_0; // 1 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	u32 unk_4; // 4 bytes
	u8 unk_5; // 1 bytes
	i64 unk_6; // 8 bytes
	u16 unk_7; // 2 bytes
	u8 unk_8; // 1 bytes
	u32 unk_9; // 4 bytes
	u8 unk_10; // 1 bytes
	u32 unk_11; // 4 bytes
	u8 unk_12; // 1 bytes
	u32 unk_13; // 4 bytes
	// logger 0x998f40
};
POP_PACKED
ASSERT_SIZE(SN_ItemUpdate, 47);

PUSH_PACKED
struct SN_UNKNOWN_62118
{
	enum { NET_ID = 62118 };
	u16 field_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger none
};
POP_PACKED

PUSH_PACKED
struct SN_NicknameNeeded
{
	enum { NET_ID = 62119 };
	// no fixed fields (payloadless or unresolved)
	// logger 0x99c050
};
POP_PACKED

PUSH_PACKED
struct SA_NicknameSet
{
	enum { NET_ID = 62120 };
	u32 result; // 4 bytes
	// reqNickname: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x984d88
};
POP_PACKED

PUSH_PACKED
struct SN_ExtraCharacters
{
	enum { NET_ID = 62130 };
	u8 isSelected; // 1 bytes
	u32 unk_0; // 4 bytes
	u16 unk_1; // 2 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	u8 unk_4; // 1 bytes
	u32 unk_5; // 4 bytes
	u32 unk_6; // 4 bytes
	u32 unk_7; // 4 bytes
	u32 unk_8; // 4 bytes
	u32 unk_9; // 4 bytes
	u32 unk_10; // 4 bytes
	u32 unk_11; // 4 bytes
	u32 unk_12; // 4 bytes
	u32 unk_13; // 4 bytes
	u32 unk_14; // 4 bytes
	u32 unk_15; // 4 bytes
	u8 unk_16; // 1 bytes
	u32 unk_17; // 4 bytes
	u8 unk_18; // 1 bytes
	u32 unk_19; // 4 bytes
	u32 unk_20; // 4 bytes
	u32 unk_21; // 4 bytes
	u32 unk_22; // 4 bytes
	u8 unk_23; // 1 bytes
	i64 unk_24; // 8 bytes
	u16 unk_25; // 2 bytes
	u8 unk_26; // 1 bytes
	u32 unk_27; // 4 bytes
	u8 unk_28; // 1 bytes
	u32 unk_29; // 4 bytes
	u8 unk_30; // 1 bytes
	u32 unk_31; // 4 bytes
	u32 unk_32; // 4 bytes
	u32 unk_33; // 4 bytes
	u32 unk_34; // 4 bytes
	u8 unk_35; // 1 bytes
	u8 unk_36; // 1 bytes
	u32 unk_37; // 4 bytes
	u32 unk_38; // 4 bytes
	u8 unk_39; // 1 bytes
	u8 unk_40; // 1 bytes
	u16 unk_41; // 2 bytes
	u32 unk_42; // 4 bytes
	u32 unk_43; // 4 bytes
	u8 unk_44; // 1 bytes
	u16 unk_45; // 2 bytes
	u16 unk_46; // 2 bytes
	u32 unk_47; // 4 bytes
	u32 unk_48; // 4 bytes
	// logger 0x992814
};
POP_PACKED
ASSERT_SIZE(SN_ExtraCharacters, 155);

PUSH_PACKED
struct SN_UNKNOWN_62131
{
	enum { NET_ID = 62131 };
	u32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	u32 field_2; // 4 bytes
	u32 field_3; // 4 bytes
	u32 field_4; // 4 bytes
	u32 field_5; // 4 bytes
	u8 field_6; // 1 bytes
	i64 field_7; // 8 bytes
	u16 field_8; // 2 bytes
	u8 field_9; // 1 bytes
	u32 field_10; // 4 bytes
	u8 field_11; // 1 bytes
	u32 field_12; // 4 bytes
	u8 field_13; // 1 bytes
	u32 field_14; // 4 bytes
	u32 field_15; // 4 bytes
	u32 field_16; // 4 bytes
	u32 field_17; // 4 bytes
	u8 field_18; // 1 bytes
	u8 field_19; // 1 bytes
	u32 field_20; // 4 bytes
	u32 field_21; // 4 bytes
	u8 field_22; // 1 bytes
	u8 field_23; // 1 bytes
	u16 field_24; // 2 bytes
	u32 field_25; // 4 bytes
	u32 field_26; // 4 bytes
	u8 field_27; // 1 bytes
	u16 field_28; // 2 bytes
	u16 field_29; // 2 bytes
	u32 field_30; // 4 bytes
	u32 field_31; // 4 bytes
	// logger none
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62131, 94);

PUSH_PACKED
struct SA_SkillUpgrade
{
	enum { NET_ID = 62132 };
	u32 result; // 4 bytes
	u32 characterID; // 4 bytes
	// skill: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u8 unk_2; // 1 bytes
	u8 unk_3; // 1 bytes
	u16 unk_4; // 2 bytes
	u32 unk_5; // 4 bytes
	u32 unk_6; // 4 bytes
	// logger 0x988efa
};
POP_PACKED
ASSERT_SIZE(SA_SkillUpgrade, 28);

PUSH_PACKED
struct SA_SkillUpgradeIngame
{
	enum { NET_ID = 62133 };
	u32 result; // 4 bytes
	u32 mainID; // 4 bytes
	// mainSkill: variable-size (string/vector)
	u32 subID; // 4 bytes
	// subSkill: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	u8 unk_1; // 1 bytes
	u8 unk_2; // 1 bytes
	u16 unk_3; // 2 bytes
	u32 unk_4; // 4 bytes
	u32 unk_5; // 4 bytes
	u32 unk_6; // 4 bytes
	u32 unk_7; // 4 bytes
	u32 unk_8; // 4 bytes
	u8 unk_9; // 1 bytes
	u8 unk_10; // 1 bytes
	u16 unk_11; // 2 bytes
	u32 unk_12; // 4 bytes
	u32 unk_13; // 4 bytes
	// logger 0x989015
};
POP_PACKED
ASSERT_SIZE(SA_SkillUpgradeIngame, 52);

PUSH_PACKED
struct SA_TitleSelect
{
	enum { NET_ID = 62134 };
	u32 Result; // 4 bytes
	u32 DisplayTitleIndex; // 4 bytes
	u32 StatTitleIndex; // 4 bytes
	// logger 0x989b4d
};
POP_PACKED
ASSERT_SIZE(SA_TitleSelect, 12);

PUSH_PACKED
struct SN_TitleAdd
{
	enum { NET_ID = 62135 };
	u32 NewTitleIndex; // 4 bytes
	// logger 0x9a7314
};
POP_PACKED
ASSERT_SIZE(SN_TitleAdd, 4);

PUSH_PACKED
struct SA_ItemEquip
{
	enum { NET_ID = 62136 };
	u32 result; // 4 bytes
	u32 itemID; // 4 bytes
	// logger 0x983b02
};
POP_PACKED
ASSERT_SIZE(SA_ItemEquip, 8);

PUSH_PACKED
struct SA_ItemUnequip
{
	enum { NET_ID = 62137 };
	u32 result; // 4 bytes
	u32 itemID; // 4 bytes
	// logger 0x983ebc
};
POP_PACKED
ASSERT_SIZE(SA_ItemUnequip, 8);

PUSH_PACKED
struct SA_SelectAccountEquipment
{
	enum { NET_ID = 62138 };
	u32 result; // 4 bytes
	u8 etcType; // 1 bytes
	u32 itemDocIndex; // 4 bytes
	// logger 0x9881ca
};
POP_PACKED
ASSERT_SIZE(SA_SelectAccountEquipment, 9);

PUSH_PACKED
struct SA_WeaponUnlock
{
	enum { NET_ID = 62139 };
	u32 result; // 4 bytes
	// newWeapon: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	u8 unk_4; // 1 bytes
	u8 unk_5; // 1 bytes
	// logger 0x98a767
};
POP_PACKED
ASSERT_SIZE(SA_WeaponUnlock, 22);

PUSH_PACKED
struct SA_WeaponEquip
{
	enum { NET_ID = 62140 };
	u32 result; // 4 bytes
	u32 characterID; // 4 bytes
	u32 weaponType; // 4 bytes
	u32 weaponIndex; // 4 bytes
	// logger 0x98a648
};
POP_PACKED
ASSERT_SIZE(SA_WeaponEquip, 16);

PUSH_PACKED
struct SA_GearEquip
{
	enum { NET_ID = 62141 };
	u32 result; // 4 bytes
	u8 masterGearNo; // 1 bytes
	u32 gearItemID; // 4 bytes
	u32 slot; // 4 bytes
	// logger 0x98063b
};
POP_PACKED
ASSERT_SIZE(SA_GearEquip, 13);

PUSH_PACKED
struct SA_GearUnequip
{
	enum { NET_ID = 62142 };
	u32 result; // 4 bytes
	u8 masterGearNo; // 1 bytes
	u32 slot; // 4 bytes
	// logger 0x9808cf
};
POP_PACKED
ASSERT_SIZE(SA_GearUnequip, 9);

PUSH_PACKED
struct SA_GearSwap
{
	enum { NET_ID = 62143 };
	u32 result; // 4 bytes
	u8 masterGearNo; // 1 bytes
	u8 fromSlot; // 1 bytes
	u32 fromGearItemID; // 4 bytes
	u8 toSlot; // 1 bytes
	u32 toGearItemID; // 4 bytes
	// logger 0x98075c
};
POP_PACKED
ASSERT_SIZE(SA_GearSwap, 15);

PUSH_PACKED
struct SN_UNKNOWN_62144
{
	enum { NET_ID = 62144 };
	u32 field_0; // 4 bytes
	u32 field_1; // 4 bytes
	u8 field_2; // 1 bytes
	u32 field_3; // 4 bytes
	// logger none
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62144, 13);

PUSH_PACKED
struct SN_UNKNOWN_62145
{
	enum { NET_ID = 62145 };
	u32 field_0; // 4 bytes
	u32 field_1; // 4 bytes
	u8 field_2; // 1 bytes
	// logger none
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62145, 9);

PUSH_PACKED
struct SA_MastergearSelect
{
	enum { NET_ID = 62146 };
	u32 result; // 4 bytes
	u32 characterID; // 4 bytes
	u8 masterGearNo; // 1 bytes
	// logger 0x98470a
};
POP_PACKED
ASSERT_SIZE(SA_MastergearSelect, 9);

PUSH_PACKED
struct SA_MastergearRename
{
	enum { NET_ID = 62147 };
	u32 result; // 4 bytes
	u8 masterGearNo; // 1 bytes
	// newName: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x98453e
};
POP_PACKED

PUSH_PACKED
struct SA_MastergearRepair
{
	enum { NET_ID = 62148 };
	u32 result; // 4 bytes
	u32 durabillity; // 4 bytes
	// logger 0x984639
};
POP_PACKED
ASSERT_SIZE(SA_MastergearRepair, 8);

PUSH_PACKED
struct SA_MastergearAdd
{
	enum { NET_ID = 62149 };
	u32 result; // 4 bytes
	// newMasterGear: variable-size (string/vector)
	u8 unk_0; // 1 bytes
	u16 unk_1; // 2 bytes
	u16 unk_2; // 2 bytes
	u32 unk_3; // 4 bytes
	u32 unk_4; // 4 bytes
	// logger 0x98444a
};
POP_PACKED
ASSERT_SIZE(SA_MastergearAdd, 17);

PUSH_PACKED
struct SA_CharacterUnlock
{
	enum { NET_ID = 62150 };
	u32 result; // 4 bytes
	u32 characterDocIndex; // 4 bytes
	// logger 0x97e35c
};
POP_PACKED
ASSERT_SIZE(SA_CharacterUnlock, 8);

PUSH_PACKED
struct SA_SkillUnlock
{
	enum { NET_ID = 62151 };
	u32 result; // 4 bytes
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u8 unk_2; // 1 bytes
	u8 unk_3; // 1 bytes
	u16 unk_4; // 2 bytes
	u32 unk_5; // 4 bytes
	u32 unk_6; // 4 bytes
	// logger 0x988d86
};
POP_PACKED
ASSERT_SIZE(SA_SkillUnlock, 24);

PUSH_PACKED
struct SA_SkillSelect
{
	enum { NET_ID = 62152 };
	u32 characterID; // 4 bytes
	u32 result; // 4 bytes
	u32 dwSkillIndex; // 4 bytes
	u32 dwSkillSlot; // 4 bytes
	// logger 0x988b48
};
POP_PACKED
ASSERT_SIZE(SA_SkillSelect, 16);

PUSH_PACKED
struct SN_SkillSelect
{
	enum { NET_ID = 62153 };
	u32 userId; // 4 bytes
	u32 characterID; // 4 bytes
	u32 dwSkillIndex; // 4 bytes
	u32 dwSkillSlot; // 4 bytes
	// logger 0x9a4202
};
POP_PACKED
ASSERT_SIZE(SN_SkillSelect, 16);

PUSH_PACKED
struct SA_SkillSwap
{
	enum { NET_ID = 62154 };
	u32 characterID; // 4 bytes
	u32 result; // 4 bytes
	u32 dwSlot1SkillIndex; // 4 bytes
	u32 dwSlot2SkillIndex; // 4 bytes
	// logger 0x988c67
};
POP_PACKED
ASSERT_SIZE(SA_SkillSwap, 16);

PUSH_PACKED
struct SN_SkillSwap
{
	enum { NET_ID = 62155 };
	u32 userId; // 4 bytes
	u32 characterID; // 4 bytes
	u32 dwSlot1SkillIndex; // 4 bytes
	u32 dwSlot2SkillIndex; // 4 bytes
	// logger 0x9a4321
};
POP_PACKED
ASSERT_SIZE(SN_SkillSwap, 16);

PUSH_PACKED
struct SN_UNKNOWN_62156
{
	enum { NET_ID = 62156 };
	u32 field_0; // 4 bytes
	// logger none
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62156, 4);

PUSH_PACKED
struct SA_StartGame
{
	enum { NET_ID = 62157 };
	u32 retval; // 4 bytes
	// logger 0x989468
};
POP_PACKED
ASSERT_SIZE(SA_StartGame, 4);

PUSH_PACKED
struct SN_RestartVoteStart
{
	enum { NET_ID = 62159 };
	u32 stageIndex; // 4 bytes
	u32 gameType; // 4 bytes
	u32 gameDefinitionType; // 4 bytes
	u32 stageRule; // 4 bytes
	u32 voteTimeMS; // 4 bytes
	u16 unk_0; // 2 bytes
	u8 unk_1; // 1 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	u32 unk_4; // 4 bytes
	u32 unk_5; // 4 bytes
	u16 unk_6; // 2 bytes
	// logger 0x9a2be4
};
POP_PACKED
ASSERT_SIZE(SN_RestartVoteStart, 41);

PUSH_PACKED
struct SN_RestartVoteFinish
{
	enum { NET_ID = 62160 };
	u8 voteResult; // 1 bytes
	u8 voteFailReason; // 1 bytes
	// logger 0x9a2b0f
};
POP_PACKED
ASSERT_SIZE(SN_RestartVoteFinish, 2);

PUSH_PACKED
struct SN_RestartVoteUpdate
{
	enum { NET_ID = 62161 };
	u8 field_0; // 1 bytes
	// logger 0x9a2df4
};
POP_PACKED
ASSERT_SIZE(SN_RestartVoteUpdate, 1);

PUSH_PACKED
struct SA_RestartGame
{
	enum { NET_ID = 62162 };
	u32 retval; // 4 bytes
	// logger 0x9873f9
};
POP_PACKED
ASSERT_SIZE(SA_RestartGame, 4);

PUSH_PACKED
struct SA_RestartVote
{
	enum { NET_ID = 62163 };
	u32 retval; // 4 bytes
	// logger 0x9875f4
};
POP_PACKED
ASSERT_SIZE(SA_RestartVote, 4);

PUSH_PACKED
struct SN_RestartPvpEntryStart
{
	enum { NET_ID = 62164 };
	u32 voteTimeMS; // 4 bytes
	// logger 0x9a281b
};
POP_PACKED
ASSERT_SIZE(SN_RestartPvpEntryStart, 4);

PUSH_PACKED
struct SN_RestartPvpEntryFinish
{
	enum { NET_ID = 62165 };
	// no fixed fields (payloadless or unresolved)
	// logger 0x9a279e
};
POP_PACKED

PUSH_PACKED
struct SA_RestartPvpEntryJoin
{
	enum { NET_ID = 62166 };
	u32 errcode; // 4 bytes
	// logger 0x9874a2
};
POP_PACKED
ASSERT_SIZE(SA_RestartPvpEntryJoin, 4);

PUSH_PACKED
struct SA_RestartPvpEntryLeave
{
	enum { NET_ID = 62167 };
	u32 errcode; // 4 bytes
	// logger 0x98754b
};
POP_PACKED
ASSERT_SIZE(SA_RestartPvpEntryLeave, 4);

PUSH_PACKED
struct SN_RestartPvpEntryUpdate
{
	enum { NET_ID = 62168 };
	// nickname: variable-size (string/vector)
	u16 isJoined; // 2 bytes
	u8 unk_0; // 1 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x9a28c4
};
POP_PACKED

PUSH_PACKED
struct SN_RestartPvpGameInfo
{
	enum { NET_ID = 62169 };
	u32 stageIndex; // 4 bytes
	u8 gameType; // 1 bytes
	u8 gameDefType; // 1 bytes
	// logger 0x9a2996
};
POP_PACKED
ASSERT_SIZE(SN_RestartPvpGameInfo, 6);

PUSH_PACKED
struct SN_RestartPvpWaitingOthers
{
	enum { NET_ID = 62170 };
	// no fixed fields (payloadless or unresolved)
	// logger 0x9a2a92
};
POP_PACKED

PUSH_PACKED
struct SN_RestartPvpCanceled
{
	enum { NET_ID = 62171 };
	u32 reasonCode; // 4 bytes
	// logger 0x9a26f5
};
POP_PACKED
ASSERT_SIZE(SN_RestartPvpCanceled, 4);

PUSH_PACKED
struct SN_MatchingPenalty
{
	enum { NET_ID = 62172 };
	u8 isMyPenalty; // 1 bytes
	u32 remainPenaltyTimeMS; // 4 bytes
	// logger 0x99b2a5
};
POP_PACKED
ASSERT_SIZE(SN_MatchingPenalty, 5);

PUSH_PACKED
struct SN_PartySwapTeam
{
	enum { NET_ID = 62178 };
	u32 originTeam; // 4 bytes
	u8 originSlotIndex; // 1 bytes
	u32 targetTeam; // 4 bytes
	u8 targetSlotIndex; // 1 bytes
	// logger 0x99d592
};
POP_PACKED
ASSERT_SIZE(SN_PartySwapTeam, 10);

PUSH_PACKED
struct SA_PartySwapTeam
{
	enum { NET_ID = 62179 };
	u32 retval; // 4 bytes
	// logger 0x985e20
};
POP_PACKED
ASSERT_SIZE(SA_PartySwapTeam, 4);

PUSH_PACKED
struct SA_PartyInvite
{
	enum { NET_ID = 62180 };
	u32 retval; // 4 bytes
	// nickname: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x98540f
};
POP_PACKED

PUSH_PACKED
struct SA_PartyInviteRecommend
{
	enum { NET_ID = 62181 };
	u32 retval; // 4 bytes
	// inviteeNickname: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x9854e1
};
POP_PACKED

PUSH_PACKED
struct SN_PartyInviteResponse
{
	enum { NET_ID = 62182 };
	// account: variable-size (string/vector)
	u16 accept; // 2 bytes
	u8 unk_0; // 1 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x99d10b
};
POP_PACKED

PUSH_PACKED
struct SN_PartyInvite
{
	enum { NET_ID = 62183 };
	u32 channelID; // 4 bytes
	// inviterNick: variable-size (string/vector)
	u16 gameType; // 2 bytes
	u32 gameDefinitionType; // 4 bytes
	u32 stageRule; // 4 bytes
	u32 partyID; // 4 bytes
	u32 teamType; // 4 bytes
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x99cd47
};
POP_PACKED

PUSH_PACKED
struct SN_PartyInviteRecommend
{
	enum { NET_ID = 62184 };
	// inviterNickname: variable-size (string/vector)
	// inviteeNickname: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	u16 unk_1; // 2 bytes
	// NOTE: 2 variable-size field(s) after the fixed prefix
	// logger 0x99cf68
};
POP_PACKED

PUSH_PACKED
struct SA_PartyInviteRecommendResponse
{
	enum { NET_ID = 62185 };
	u32 retval; // 4 bytes
	// inviteeNickname: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x9855b3
};
POP_PACKED

PUSH_PACKED
struct SN_PartyInviteRecommendResponse
{
	enum { NET_ID = 62186 };
	// inviteeNickname: variable-size (string/vector)
	u16 accept; // 2 bytes
	u8 unk_0; // 1 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x99d039
};
POP_PACKED

PUSH_PACKED
struct SA_PartyJoin
{
	enum { NET_ID = 62187 };
	u32 retval; // 4 bytes
	u32 partyID; // 4 bytes
	u32 stageType; // 4 bytes
	u32 gameType; // 4 bytes
	u32 gameDefinitionType; // 4 bytes
	u32 alliesTeamType; // 4 bytes
	u16 enemiesTeamType; // 2 bytes
	u8 isSpectator; // 1 bytes
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	u32 unk_4; // 4 bytes
	u32 unk_5; // 4 bytes
	u16 unk_6; // 2 bytes
	u8 unk_7; // 1 bytes
	u32 unk_8; // 4 bytes
	u32 unk_9; // 4 bytes
	u32 unk_10; // 4 bytes
	u32 unk_11; // 4 bytes
	u32 unk_12; // 4 bytes
	u32 unk_13; // 4 bytes
	u16 unk_14; // 2 bytes
	u8 unk_15; // 1 bytes
	u32 unk_16; // 4 bytes
	u32 unk_17; // 4 bytes
	u32 unk_18; // 4 bytes
	u32 unk_19; // 4 bytes
	u8 unk_20; // 1 bytes
	// logger 0x985685
};
POP_PACKED
ASSERT_SIZE(SA_PartyJoin, 98);

PUSH_PACKED
struct SN_PartyJoin
{
	enum { NET_ID = 62188 };
	u32 userId; // 4 bytes
	// username: variable-size (string/vector)
	u16 isBot; // 2 bytes
	u8 creatureIndex; // 1 bytes
	u32 isOwner; // 4 bytes
	u8 gameType; // 1 bytes
	u32 gameDefinitionType; // 4 bytes
	u32 teamType; // 4 bytes
	u32 teamSlotIndex; // 4 bytes
	u8 unk_0; // 1 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x99d1dd
};
POP_PACKED

PUSH_PACKED
struct SA_PartyLeave
{
	enum { NET_ID = 62189 };
	u32 retval; // 4 bytes
	// logger 0x985b29
};
POP_PACKED
ASSERT_SIZE(SA_PartyLeave, 4);

PUSH_PACKED
struct SN_PartyLeave
{
	enum { NET_ID = 62190 };
	u32 userId; // 4 bytes
	// username: variable-size (string/vector)
	u16 isOwner; // 2 bytes
	u8 unk_0; // 1 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x99d497
};
POP_PACKED

PUSH_PACKED
struct SA_PartyAddBot
{
	enum { NET_ID = 62191 };
	u32 retval; // 4 bytes
	// logger 0x98519d
};
POP_PACKED
ASSERT_SIZE(SA_PartyAddBot, 4);

PUSH_PACKED
struct SA_PartyRemoveBot
{
	enum { NET_ID = 62192 };
	u32 retval; // 4 bytes
	// logger 0x985d77
};
POP_PACKED
ASSERT_SIZE(SA_PartyRemoveBot, 4);

PUSH_PACKED
struct SA_PartyBreakup
{
	enum { NET_ID = 62193 };
	u32 retval; // 4 bytes
	u32 remainMemberCount; // 4 bytes
	// logger 0x985246
};
POP_PACKED
ASSERT_SIZE(SA_PartyBreakup, 8);

PUSH_PACKED
struct SN_PartyBreakup
{
	enum { NET_ID = 62194 };
	// no fixed fields (payloadless or unresolved)
	// logger 0x99cc21
};
POP_PACKED

PUSH_PACKED
struct SN_PartyKicked
{
	enum { NET_ID = 62195 };
	u32 partyID; // 4 bytes
	u32 reason; // 4 bytes
	// logger 0x99d3c6
};
POP_PACKED
ASSERT_SIZE(SN_PartyKicked, 8);

PUSH_PACKED
struct SA_PartyKickAll
{
	enum { NET_ID = 62196 };
	u32 retval; // 4 bytes
	// logger 0x985a80
};
POP_PACKED
ASSERT_SIZE(SA_PartyKickAll, 4);

PUSH_PACKED
struct SA_QuickRunArena
{
	enum { NET_ID = 62197 };
	u32 retval; // 4 bytes
	// partyInfo: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	u8 unk_4; // 1 bytes
	// logger 0x9869cb
};
POP_PACKED
ASSERT_SIZE(SA_QuickRunArena, 19);

PUSH_PACKED
struct SN_EnqueueTrollPenalty
{
	enum { NET_ID = 62198 };
	u32 retval; // 4 bytes
	u16 trollReducingGameCount; // 2 bytes
	f32 maxPenaltyMinutes; // 4 bytes
	// logger 0x991db0
};
POP_PACKED
ASSERT_SIZE(SN_EnqueueTrollPenalty, 10);

PUSH_PACKED
struct SA_EnqueueTrollPenaltyCancel
{
	enum { NET_ID = 62199 };
	u32 retval; // 4 bytes
	// logger 0x97f7ea
};
POP_PACKED
ASSERT_SIZE(SA_EnqueueTrollPenaltyCancel, 4);

PUSH_PACKED
struct SN_EnqueueTrollPenaltyCancel
{
	enum { NET_ID = 62200 };
	// no fixed fields (payloadless or unresolved)
	// logger 0x991eb2
};
POP_PACKED

PUSH_PACKED
struct SA_CancelMatchingQueue
{
	enum { NET_ID = 62202 };
	u32 retval; // 4 bytes
	// logger 0x97ddd0
};
POP_PACKED
ASSERT_SIZE(SA_CancelMatchingQueue, 4);

PUSH_PACKED
struct SN_CancelMatchingQueue
{
	enum { NET_ID = 62203 };
	u8 byRemoveMember; // 1 bytes
	// logger 0x98e710
};
POP_PACKED
ASSERT_SIZE(SN_CancelMatchingQueue, 1);

PUSH_PACKED
struct SN_MatchingPartyConfirm
{
	enum { NET_ID = 62206 };
	u32 userId; // 4 bytes
	u32 teamType; // 4 bytes
	u8 comfirmed; // 1 bytes
	// logger 0x99b100
};
POP_PACKED
ASSERT_SIZE(SN_MatchingPartyConfirm, 9);

PUSH_PACKED
struct SN_ChannelChattingChannel
{
	enum { NET_ID = 62207 };
	u32 channelType; // 4 bytes
	// channelName: variable-size (string/vector)
	// password: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	u16 unk_1; // 2 bytes
	// NOTE: 2 variable-size field(s) after the fixed prefix
	// logger 0x98f0c1
};
POP_PACKED

PUSH_PACKED
struct SA_MasterUnpick
{
	enum { NET_ID = 62210 };
	u32 retval; // 4 bytes
	u32 characterID; // 4 bytes
	// logger 0x984a1d
};
POP_PACKED
ASSERT_SIZE(SA_MasterUnpick, 8);

PUSH_PACKED
struct SN_MasterUnpick
{
	enum { NET_ID = 62211 };
	u32 userId; // 4 bytes
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	// logger 0x99ae22
};
POP_PACKED
ASSERT_SIZE(SN_MasterUnpick, 20);

PUSH_PACKED
struct SA_MasterUnpickAll
{
	enum { NET_ID = 62212 };
	u32 retval; // 4 bytes
	// logger 0x984aee
};
POP_PACKED
ASSERT_SIZE(SA_MasterUnpickAll, 4);

PUSH_PACKED
struct SN_MasterUnpickAll
{
	enum { NET_ID = 62213 };
	u32 userId; // 4 bytes
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	// logger 0x99af91
};
POP_PACKED
ASSERT_SIZE(SN_MasterUnpickAll, 20);

PUSH_PACKED
struct SA_SortieRoomBreakup
{
	enum { NET_ID = 62215 };
	u32 retval; // 4 bytes
	// logger 0x9892ee
};
POP_PACKED
ASSERT_SIZE(SA_SortieRoomBreakup, 4);

PUSH_PACKED
struct SN_SortieRoomBreakup
{
	enum { NET_ID = 62216 };
	u32 stageType; // 4 bytes
	u32 nReason; // 4 bytes
	// logger 0x9a5448
};
POP_PACKED
ASSERT_SIZE(SN_SortieRoomBreakup, 8);

PUSH_PACKED
struct SA_WeaponUpgrade
{
	enum { NET_ID = 62220 };
	u32 result; // 4 bytes
	// weaponInfo: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	u8 unk_4; // 1 bytes
	u8 unk_5; // 1 bytes
	// logger 0x98a85b
};
POP_PACKED
ASSERT_SIZE(SA_WeaponUpgrade, 22);

PUSH_PACKED
struct SA_InventoryExpand
{
	enum { NET_ID = 62221 };
	u32 result; // 4 bytes
	u32 invenTab; // 4 bytes
	u32 lineCount; // 4 bytes
	// logger 0x983453
};
POP_PACKED
ASSERT_SIZE(SA_InventoryExpand, 12);

PUSH_PACKED
struct SN_DespawnMonsters
{
	enum { NET_ID = 62222 };
	u32 field_0; // 4 bytes
	// logger 0x99103e
};
POP_PACKED
ASSERT_SIZE(SN_DespawnMonsters, 4);

PUSH_PACKED
struct SN_DespawnMonster
{
	enum { NET_ID = 62223 };
	u32 objectID; // 4 bytes
	// logger 0x990f95
};
POP_PACKED
ASSERT_SIZE(SN_DespawnMonster, 4);

PUSH_PACKED
struct SN_UpdateAihost
{
	enum { NET_ID = 62225 };
	u8 serverBot; // 1 bytes
	u32 userId; // 4 bytes
	u32 reason; // 4 bytes
	// logger 0x9a78fb
};
POP_PACKED
ASSERT_SIZE(SN_UpdateAihost, 9);

PUSH_PACKED
struct SA_RequestSummaryInfoEach
{
	enum { NET_ID = 62226 };
	u32 summaryIndex; // 4 bytes
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	u8 unk_4; // 1 bytes
	i64 unk_5; // 8 bytes
	// logger 0x986e4a
};
POP_PACKED
ASSERT_SIZE(SA_RequestSummaryInfoEach, 29);

PUSH_PACKED
struct SN_SummaryUpdate
{
	enum { NET_ID = 62228 };
	// summaryInfo: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	u8 unk_4; // 1 bytes
	i64 unk_5; // 8 bytes
	// logger 0x9a68a6
};
POP_PACKED
ASSERT_SIZE(SN_SummaryUpdate, 25);

PUSH_PACKED
struct SN_AddStatus
{
	enum { NET_ID = 62230 };
	u32 field_0; // 4 bytes
	u32 field_1; // 4 bytes
	u32 field_2; // 4 bytes
	u32 field_3; // 4 bytes
	u32 field_4; // 4 bytes
	u32 field_5; // 4 bytes
	u32 field_6; // 4 bytes
	u32 field_7; // 4 bytes
	u32 field_8; // 4 bytes
	u32 field_9; // 4 bytes
	u32 field_10; // 4 bytes
	u32 field_11; // 4 bytes
	u32 field_12; // 4 bytes
	u32 field_13; // 4 bytes
	u32 field_14; // 4 bytes
	// logger none
};
POP_PACKED
ASSERT_SIZE(SN_AddStatus, 60);

PUSH_PACKED
struct SN_ModifyStatus
{
	enum { NET_ID = 62231 };
	u32 statusID; // 4 bytes
	u8 bEnabled; // 1 bytes
	u32 targetID; // 4 bytes
	u32 casterID; // 4 bytes
	u32 replaceCasterID; // 4 bytes
	u8 overlapCount; // 1 bytes
	u8 customValue; // 1 bytes
	u8 extraStatusProcEvent; // 1 bytes
	// logger 0x99b60e
};
POP_PACKED
ASSERT_SIZE(SN_ModifyStatus, 20);

PUSH_PACKED
struct SN_RemoveStatus
{
	enum { NET_ID = 62232 };
	u32 statusID; // 4 bytes
	u32 targetID; // 4 bytes
	u32 casterID; // 4 bytes
	// logger 0x9a22ec
};
POP_PACKED
ASSERT_SIZE(SN_RemoveStatus, 12);

PUSH_PACKED
struct SN_EnterUserByTrespass
{
	enum { NET_ID = 62233 };
	u32 usn; // 4 bytes
	// logger 0x992000
};
POP_PACKED
ASSERT_SIZE(SN_EnterUserByTrespass, 4);

PUSH_PACKED
struct SN_LeaveUser
{
	enum { NET_ID = 62234 };
	u32 usn; // 4 bytes
	u8 canReturn; // 1 bytes
	// logger 0x9995a7
};
POP_PACKED
ASSERT_SIZE(SN_LeaveUser, 5);

PUSH_PACKED
struct SN_BroadcastDamage
{
	enum { NET_ID = 62235 };
	u32 damageSeqNum; // 4 bytes
	u32 remoteID; // 4 bytes
	u32 defID; // 4 bytes
	u32 attID; // 4 bytes
	u32 remoteDocIndex; // 4 bytes
	// remotePos: variable-size (string/vector)
	// remoteDir: variable-size (string/vector)
	// remoteForceDir: variable-size (string/vector)
	// hitPos: variable-size (string/vector)
	// hitDir: variable-size (string/vector)
	u32 damageType; // 4 bytes
	u32 skillDocID; // 4 bytes
	u32 nRagePoint; // 4 bytes
	u32 nRelativeElement; // 4 bytes
	// hitNodeName: variable-size (string/vector)
	u32 damage; // 4 bytes
	// partName: variable-size (string/vector)
	u32 partDamage; // 4 bytes
	u32 masterGroupingDamage; // 4 bytes
	u32 optionalResultOfHit; // 4 bytes
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	u32 unk_4; // 4 bytes
	u32 unk_5; // 4 bytes
	u32 unk_6; // 4 bytes
	u32 unk_7; // 4 bytes
	u32 unk_8; // 4 bytes
	u32 unk_9; // 4 bytes
	u32 unk_10; // 4 bytes
	u16 unk_11; // 2 bytes
	u32 unk_12; // 4 bytes
	u16 unk_13; // 2 bytes
	u32 unk_14; // 4 bytes
	u32 unk_15; // 4 bytes
	u32 unk_16; // 4 bytes
	// NOTE: 2 variable-size field(s) after the fixed prefix
	// logger 0x98da4c
};
POP_PACKED

PUSH_PACKED
struct SN_BroadcastNonRemoteDamage
{
	enum { NET_ID = 62236 };
	u32 defID; // 4 bytes
	u32 attID; // 4 bytes
	u32 statusDocID; // 4 bytes
	u32 skillDocID; // 4 bytes
	u32 damageType; // 4 bytes
	u32 damage; // 4 bytes
	u32 masterGroupingDamage; // 4 bytes
	// logger 0x98e11d
};
POP_PACKED
ASSERT_SIZE(SN_BroadcastNonRemoteDamage, 28);

PUSH_PACKED
struct SN_RemoteActivated
{
	enum { NET_ID = 62237 };
	u32 remoteID; // 4 bytes
	u32 defID; // 4 bytes
	u32 penetrationCount; // 4 bytes
	// remotePos: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	// logger 0x9a1227
};
POP_PACKED
ASSERT_SIZE(SN_RemoteActivated, 24);

PUSH_PACKED
struct SA_ChannelChat
{
	enum { NET_ID = 62239 };
	u32 retval; // 4 bytes
	// logger 0x97e069
};
POP_PACKED
ASSERT_SIZE(SA_ChannelChat, 4);

PUSH_PACKED
struct SN_ChatChannelJoin
{
	enum { NET_ID = 62240 };
	u32 channelType; // 4 bytes
	// chatChannelName: variable-size (string/vector)
	// password: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	u16 unk_1; // 2 bytes
	// NOTE: 2 variable-size field(s) after the fixed prefix
	// logger 0x98f841
};
POP_PACKED

PUSH_PACKED
struct SN_ChatChannelLeave
{
	enum { NET_ID = 62241 };
	u32 channelType; // 4 bytes
	// chatChannelName: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x98f93b
};
POP_PACKED

PUSH_PACKED
struct SN_ChatChannelUserJoin
{
	enum { NET_ID = 62243 };
	// channelName: variable-size (string/vector)
	// nickname: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	u16 unk_1; // 2 bytes
	// NOTE: 2 variable-size field(s) after the fixed prefix
	// logger 0x98fb30
};
POP_PACKED

PUSH_PACKED
struct SN_ChatChannelUserLeave
{
	enum { NET_ID = 62244 };
	// channelName: variable-size (string/vector)
	// nickname: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	u16 unk_1; // 2 bytes
	// NOTE: 2 variable-size field(s) after the fixed prefix
	// logger 0x98fc01
};
POP_PACKED

PUSH_PACKED
struct SN_FriendAdded
{
	enum { NET_ID = 62245 };
	// newFriendInfo: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	u32 unk_1; // 4 bytes
	u16 unk_2; // 2 bytes
	i64 unk_3; // 8 bytes
	// logger 0x992deb
};
POP_PACKED
ASSERT_SIZE(SN_FriendAdded, 16);

PUSH_PACKED
struct SA_FriendRemove
{
	enum { NET_ID = 62246 };
	u32 retval; // 4 bytes
	// targetNickname: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x97fdbc
};
POP_PACKED

PUSH_PACKED
struct SA_CommunityBlockAdd
{
	enum { NET_ID = 62247 };
	u32 retval; // 4 bytes
	// userinfo: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	// logger 0x97e70d
};
POP_PACKED
ASSERT_SIZE(SA_CommunityBlockAdd, 6);

PUSH_PACKED
struct SA_CommunityBlockRemove
{
	enum { NET_ID = 62248 };
	u32 retval; // 4 bytes
	// userinfo: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	// logger 0x97e801
};
POP_PACKED
ASSERT_SIZE(SA_CommunityBlockRemove, 6);

PUSH_PACKED
struct SN_FriendRemoved
{
	enum { NET_ID = 62249 };
	// nickname: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x9932bd
};
POP_PACKED

PUSH_PACKED
struct SA_FriendStateChange
{
	enum { NET_ID = 62250 };
	u32 retval; // 4 bytes
	// logger 0x9802f3
};
POP_PACKED
ASSERT_SIZE(SA_FriendStateChange, 4);

PUSH_PACKED
struct SN_FriendStateChanged
{
	enum { NET_ID = 62251 };
	// nickname: variable-size (string/vector)
	u16 newState; // 2 bytes
	u8 isOnLeavePenalty; // 1 bytes
	u8 stateUpdatedTime; // 1 bytes
	i64 unk_0; // 8 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x993646
};
POP_PACKED

PUSH_PACKED
struct SN_FriendNicknameChanged
{
	enum { NET_ID = 62252 };
	// oldNickname: variable-size (string/vector)
	// newNickname: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	u16 unk_1; // 2 bytes
	// NOTE: 2 variable-size field(s) after the fixed prefix
	// logger 0x9930ca
};
POP_PACKED

PUSH_PACKED
struct SA_CommunityStatusMessage
{
	enum { NET_ID = 62253 };
	u32 retval; // 4 bytes
	// logger 0x97e99e
};
POP_PACKED
ASSERT_SIZE(SA_CommunityStatusMessage, 4);

PUSH_PACKED
struct SA_CommunityChannelMove
{
	enum { NET_ID = 62254 };
	u32 retval; // 4 bytes
	// logger 0x97e8f5
};
POP_PACKED
ASSERT_SIZE(SA_CommunityChannelMove, 4);

PUSH_PACKED
struct SN_CommunityStatusMessage
{
	enum { NET_ID = 62255 };
	// nickname: variable-size (string/vector)
	// msg: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	u16 unk_1; // 2 bytes
	// NOTE: 2 variable-size field(s) after the fixed prefix
	// logger 0x99001d
};
POP_PACKED

PUSH_PACKED
struct SN_FriendLeaderChanged
{
	enum { NET_ID = 62256 };
	// nickname: variable-size (string/vector)
	u16 creatureIndex; // 2 bytes
	u32 unk_0; // 4 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x992eb4
};
POP_PACKED

PUSH_PACKED
struct SN_RecommendedfriendList
{
	enum { NET_ID = 62260 };
	u16 field_0; // 2 bytes
	u32 field_1; // 4 bytes
	// logger 0x9a097a
};
POP_PACKED
ASSERT_SIZE(SN_RecommendedfriendList, 6);

PUSH_PACKED
struct SA_FriendRequest
{
	enum { NET_ID = 62262 };
	u32 retval; // 4 bytes
	// nickname: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x97fe8e
};
POP_PACKED

PUSH_PACKED
struct SN_FriendRequested
{
	enum { NET_ID = 62263 };
	// userInfo: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	u32 unk_1; // 4 bytes
	// logger 0x993365
};
POP_PACKED
ASSERT_SIZE(SN_FriendRequested, 6);

PUSH_PACKED
struct SA_FriendRespond
{
	enum { NET_ID = 62264 };
	u32 retval; // 4 bytes
	// nickname: variable-size (string/vector)
	u16 bAccept; // 2 bytes
	u8 unk_0; // 1 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x980054
};
POP_PACKED

PUSH_PACKED
struct SN_FriendResponded
{
	enum { NET_ID = 62265 };
	// nickname: variable-size (string/vector)
	u16 accept; // 2 bytes
	u8 unk_0; // 1 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x993574
};
POP_PACKED

PUSH_PACKED
struct SA_FriendRequestLeaderInfo
{
	enum { NET_ID = 62266 };
	u32 retval; // 4 bytes
	// charinfo: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	u16 unk_1; // 2 bytes
	u16 unk_2; // 2 bytes
	u32 unk_3; // 4 bytes
	u32 unk_4; // 4 bytes
	u32 unk_5; // 4 bytes
	u32 unk_6; // 4 bytes
	u16 unk_7; // 2 bytes
	u32 unk_8; // 4 bytes
	u32 unk_9; // 4 bytes
	u8 unk_10; // 1 bytes
	u8 unk_11; // 1 bytes
	u16 unk_12; // 2 bytes
	u32 unk_13; // 4 bytes
	u32 unk_14; // 4 bytes
	// logger 0x97ff60
};
POP_PACKED
ASSERT_SIZE(SA_FriendRequestLeaderInfo, 48);

PUSH_PACKED
struct SA_RequestLeaderInfo
{
	enum { NET_ID = 62267 };
	u32 retval; // 4 bytes
	// charinfo: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	u16 unk_1; // 2 bytes
	u16 unk_2; // 2 bytes
	u32 unk_3; // 4 bytes
	u32 unk_4; // 4 bytes
	u32 unk_5; // 4 bytes
	u32 unk_6; // 4 bytes
	u16 unk_7; // 2 bytes
	u32 unk_8; // 4 bytes
	u32 unk_9; // 4 bytes
	u8 unk_10; // 1 bytes
	u8 unk_11; // 1 bytes
	u16 unk_12; // 2 bytes
	u32 unk_13; // 4 bytes
	u32 unk_14; // 4 bytes
	// logger 0x986c62
};
POP_PACKED
ASSERT_SIZE(SA_RequestLeaderInfo, 48);

PUSH_PACKED
struct SA_FriendSetComrade
{
	enum { NET_ID = 62268 };
	u32 retval; // 4 bytes
	// friendNickname: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x98014f
};
POP_PACKED

PUSH_PACKED
struct SA_FriendUnsetComrade
{
	enum { NET_ID = 62269 };
	u32 retval; // 4 bytes
	// friendNickname: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x98039c
};
POP_PACKED

PUSH_PACKED
struct SA_FriendSetFavorite
{
	enum { NET_ID = 62270 };
	u32 result; // 4 bytes
	// friendNickname: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x980221
};
POP_PACKED

PUSH_PACKED
struct SA_FriendUnsetFavorite
{
	enum { NET_ID = 62271 };
	u32 result; // 4 bytes
	// friendNickname: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x98046e
};
POP_PACKED

PUSH_PACKED
struct SN_FriendPartycreation
{
	enum { NET_ID = 62272 };
	// friendNickname: variable-size (string/vector)
	u16 stageIndex; // 2 bytes
	u32 gameType; // 4 bytes
	u8 gameDefType; // 1 bytes
	u8 unk_0; // 1 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x99319b
};
POP_PACKED

PUSH_PACKED
struct SA_RequestMissionReward
{
	enum { NET_ID = 62273 };
	u32 result; // 4 bytes
	// mission: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	i64 unk_1; // 8 bytes
	u16 unk_2; // 2 bytes
	u8 unk_3; // 1 bytes
	u8 unk_4; // 1 bytes
	// logger 0x986d56
};
POP_PACKED
ASSERT_SIZE(SA_RequestMissionReward, 20);

PUSH_PACKED
struct SN_PvpAvailableReportCount
{
	enum { NET_ID = 62274 };
	u8 isTroll; // 1 bytes
	u8 remainingCount; // 1 bytes
	// logger 0x99fbfd
};
POP_PACKED
ASSERT_SIZE(SN_PvpAvailableReportCount, 2);

PUSH_PACKED
struct SN_ReturnRoomExpirationCountdown
{
	enum { NET_ID = 62275 };
	// no fixed fields (payloadless or unresolved)
	// logger 0x9a301c
};
POP_PACKED

PUSH_PACKED
struct SN_GlobalNotice
{
	enum { NET_ID = 62277 };
	u32 chatType; // 4 bytes
	u16 durationMS; // 2 bytes
	u8 unk_0; // 1 bytes
	u16 unk_1; // 2 bytes
	// logger 0x995d9a
};
POP_PACKED
ASSERT_SIZE(SN_GlobalNotice, 9);

PUSH_PACKED
struct SN_ExpGain
{
	enum { NET_ID = 62279 };
	u32 amount; // 4 bytes
	// logger 0x992648
};
POP_PACKED
ASSERT_SIZE(SN_ExpGain, 4);

PUSH_PACKED
struct SA_GetUserinfo
{
	enum { NET_ID = 62280 };
	u32 result; // 4 bytes
	// nickname: variable-size (string/vector)
	u16 level; // 2 bytes
	u16 achievementScore; // 2 bytes
	u32 tierGrade; // 4 bytes
	// statusMessage: variable-size (string/vector)
	u8 creatureIndex; // 1 bytes
	u16 power; // 2 bytes
	u32 viability; // 4 bytes
	u32 skinId; // 4 bytes
	// weapon: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	u8 unk_1; // 1 bytes
	u32 unk_2; // 4 bytes
	u8 unk_3; // 1 bytes
	u8 unk_4; // 1 bytes
	u8 unk_5; // 1 bytes
	u32 unk_6; // 4 bytes
	u8 unk_7; // 1 bytes
	u8 unk_8; // 1 bytes
	u32 unk_9; // 4 bytes
	u8 unk_10; // 1 bytes
	u32 unk_11; // 4 bytes
	u32 unk_12; // 4 bytes
	u32 unk_13; // 4 bytes
	u32 unk_14; // 4 bytes
	u8 unk_15; // 1 bytes
	i64 unk_16; // 8 bytes
	u16 unk_17; // 2 bytes
	u8 unk_18; // 1 bytes
	u32 unk_19; // 4 bytes
	u8 unk_20; // 1 bytes
	u32 unk_21; // 4 bytes
	u8 unk_22; // 1 bytes
	// NOTE: 2 variable-size field(s) after the fixed prefix
	// logger 0x982a08
};
POP_PACKED

PUSH_PACKED
struct SA_JukeboxEnqueue
{
	enum { NET_ID = 62281 };
	u32 result; // 4 bytes
	// logger 0x984252
};
POP_PACKED
ASSERT_SIZE(SA_JukeboxEnqueue, 4);

PUSH_PACKED
struct SN_OpenEventMasters
{
	enum { NET_ID = 62286 };
	u32 field_0; // 4 bytes
	// logger 0x99cb16
};
POP_PACKED
ASSERT_SIZE(SN_OpenEventMasters, 4);

PUSH_PACKED
struct SA_CreateGuild
{
	enum { NET_ID = 62287 };
	u32 result; // 4 bytes
	// guildName: variable-size (string/vector)
	// guildTag: variable-size (string/vector)
	u16 masterTopPvpTierGrade; // 2 bytes
	u16 masterTopPvpTierPoint; // 2 bytes
	u16 masterAchievementScore; // 2 bytes
	u32 unk_0; // 4 bytes
	// NOTE: 2 variable-size field(s) after the fixed prefix
	// logger 0x97ea47
};
POP_PACKED

PUSH_PACKED
struct SA_DissolveGuild
{
	enum { NET_ID = 62288 };
	u32 result; // 4 bytes
	i64 dissolutionDate; // 8 bytes
	// logger 0x97f11e
};
POP_PACKED
ASSERT_SIZE(SA_DissolveGuild, 12);

PUSH_PACKED
struct SA_CancelGuilddissolution
{
	enum { NET_ID = 62289 };
	u32 result; // 4 bytes
	// logger 0x97dd27
};
POP_PACKED
ASSERT_SIZE(SA_CancelGuilddissolution, 4);

PUSH_PACKED
struct SN_UNKNOWN_62290
{
	enum { NET_ID = 62290 };
	u32 field_0; // 4 bytes
	// logger none
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62290, 4);

PUSH_PACKED
struct SA_QuitGuild
{
	enum { NET_ID = 62291 };
	u32 result; // 4 bytes
	// logger 0x986abf
};
POP_PACKED
ASSERT_SIZE(SA_QuitGuild, 4);

PUSH_PACKED
struct SA_KickGuildmember
{
	enum { NET_ID = 62292 };
	u32 result; // 4 bytes
	// nicknameKickedOut: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x9842fb
};
POP_PACKED

PUSH_PACKED
struct SA_JoinGuild
{
	enum { NET_ID = 62293 };
	u32 result; // 4 bytes
	// guildName: variable-size (string/vector)
	u16 guildJoinType; // 2 bytes
	u8 unk_0; // 1 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x984157
};
POP_PACKED

PUSH_PACKED
struct SA_GetGuildjoinrequestlist
{
	enum { NET_ID = 62294 };
	u32 result; // 4 bytes
	u16 unk_0; // 2 bytes
	u16 unk_1; // 2 bytes
	u16 unk_2; // 2 bytes
	u16 unk_3; // 2 bytes
	u32 unk_4; // 4 bytes
	u8 unk_5; // 1 bytes
	u16 unk_6; // 2 bytes
	// logger 0x980fcf
};
POP_PACKED
ASSERT_SIZE(SA_GetGuildjoinrequestlist, 19);

PUSH_PACKED
struct SA_RespondGuildjoinrequest
{
	enum { NET_ID = 62295 };
	u32 result; // 4 bytes
	u8 isApproval; // 1 bytes
	// candidate: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	u32 unk_1; // 4 bytes
	u16 unk_2; // 2 bytes
	u16 unk_3; // 2 bytes
	u16 unk_4; // 2 bytes
	u32 unk_5; // 4 bytes
	u8 unk_6; // 1 bytes
	u16 unk_7; // 2 bytes
	u32 unk_8; // 4 bytes
	u32 unk_9; // 4 bytes
	u16 unk_10; // 2 bytes
	u16 unk_11; // 2 bytes
	i64 unk_12; // 8 bytes
	// logger 0x9872dc
};
POP_PACKED
ASSERT_SIZE(SA_RespondGuildjoinrequest, 44);

PUSH_PACKED
struct SA_InviteGuildmember
{
	enum { NET_ID = 62296 };
	u32 result; // 4 bytes
	// inviteeNickname: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x98354b
};
POP_PACKED

PUSH_PACKED
struct SA_GetGuildinvitationlist
{
	enum { NET_ID = 62297 };
	u32 result; // 4 bytes
	u16 unk_0; // 2 bytes
	u16 unk_1; // 2 bytes
	// logger 0x980e5b
};
POP_PACKED
ASSERT_SIZE(SA_GetGuildinvitationlist, 8);

PUSH_PACKED
struct SA_RespondGuildinvitation
{
	enum { NET_ID = 62298 };
	u32 result; // 4 bytes
	u8 accept; // 1 bytes
	// guildName: variable-size (string/vector)
	// guildTag: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	u16 unk_1; // 2 bytes
	// NOTE: 2 variable-size field(s) after the fixed prefix
	// logger 0x9871b9
};
POP_PACKED

PUSH_PACKED
struct SA_GetGuildemblemlist
{
	enum { NET_ID = 62301 };
	u32 result; // 4 bytes
	u32 unk_0; // 4 bytes
	// logger 0x980bb1
};
POP_PACKED
ASSERT_SIZE(SA_GetGuildemblemlist, 8);

PUSH_PACKED
struct SA_GetGuildpublicprofile
{
	enum { NET_ID = 62303 };
	u32 result; // 4 bytes
	// guildName: variable-size (string/vector)
	// guildTag: variable-size (string/vector)
	u16 emblemIndex; // 2 bytes
	u16 guildLvl; // 2 bytes
	u32 memberMax; // 4 bytes
	// ownerNickname: variable-size (string/vector)
	u8 createdDate; // 1 bytes
	u8 dissolutionDate; // 1 bytes
	u16 joinType; // 2 bytes
	// guildInterest: variable-size (string/vector)
	// guildIntro: variable-size (string/vector)
	i64 memberNum; // 8 bytes
	i64 unk_0; // 8 bytes
	u16 unk_1; // 2 bytes
	// NOTE: 4 variable-size field(s) after the fixed prefix
	// logger 0x9818a8
};
POP_PACKED

PUSH_PACKED
struct SA_GetGuildpublicprofileFromUser
{
	enum { NET_ID = 62304 };
	u32 result; // 4 bytes
	// guildName: variable-size (string/vector)
	// guildTag: variable-size (string/vector)
	u16 emblemIndex; // 2 bytes
	u16 guildLvl; // 2 bytes
	u32 memberMax; // 4 bytes
	// ownerNickname: variable-size (string/vector)
	u8 createdDate; // 1 bytes
	u8 dissolutionDate; // 1 bytes
	u16 joinType; // 2 bytes
	// guildInterest: variable-size (string/vector)
	// guildIntro: variable-size (string/vector)
	i64 memberNum; // 8 bytes
	// memberNickname: variable-size (string/vector)
	i64 unk_0; // 8 bytes
	u8 unk_1; // 1 bytes
	u16 unk_2; // 2 bytes
	u8 unk_3; // 1 bytes
	u16 unk_4; // 2 bytes
	// NOTE: 5 variable-size field(s) after the fixed prefix
	// logger 0x981b60
};
POP_PACKED

PUSH_PACKED
struct SA_EditGuildnotice
{
	enum { NET_ID = 62305 };
	u32 result; // 4 bytes
	// guildNotice: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x97f54f
};
POP_PACKED

PUSH_PACKED
struct SA_EditGuildintro
{
	enum { NET_ID = 62306 };
	u32 result; // 4 bytes
	// guildIntro: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x97f311
};
POP_PACKED

PUSH_PACKED
struct SA_EditGuildtag
{
	enum { NET_ID = 62307 };
	u32 result; // 4 bytes
	// guildTag: variable-size (string/vector)
	u16 guildFundCost; // 2 bytes
	u32 totalGuildFund; // 4 bytes
	u32 unk_0; // 4 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x97f621
};
POP_PACKED

PUSH_PACKED
struct SA_SetGuildinterest
{
	enum { NET_ID = 62308 };
	u32 result; // 4 bytes
	// guildInterest: variable-size (string/vector)
	// logger 0x988648
};
POP_PACKED
ASSERT_SIZE(SA_SetGuildinterest, 4);

PUSH_PACKED
struct SA_SetGuildemblem
{
	enum { NET_ID = 62309 };
	u32 result; // 4 bytes
	u32 emblemIndex; // 4 bytes
	// logger 0x988577
};
POP_PACKED
ASSERT_SIZE(SA_SetGuildemblem, 8);

PUSH_PACKED
struct SA_SetGuildjointype
{
	enum { NET_ID = 62310 };
	u32 result; // 4 bytes
	u8 joinType; // 1 bytes
	// logger 0x98873c
};
POP_PACKED
ASSERT_SIZE(SA_SetGuildjointype, 5);

PUSH_PACKED
struct SA_SetGuildmemberclass
{
	enum { NET_ID = 62311 };
	u32 result; // 4 bytes
	// memberNickname: variable-size (string/vector)
	u16 oldMembershipId; // 2 bytes
	u32 newMembershipId; // 4 bytes
	// className: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	u16 unk_1; // 2 bytes
	// NOTE: 2 variable-size field(s) after the fixed prefix
	// logger 0x988908
};
POP_PACKED

PUSH_PACKED
struct SA_SetGuildmaster
{
	enum { NET_ID = 62312 };
	u32 result; // 4 bytes
	u32 myMembershipId; // 4 bytes
	// newMasterNickname: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x98880f
};
POP_PACKED

PUSH_PACKED
struct SA_CreateGuildmembership
{
	enum { NET_ID = 62313 };
	u32 result; // 4 bytes
	u32 membershipId; // 4 bytes
	u8 iconIndex; // 1 bytes
	// className: variable-size (string/vector)
	// rights: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x97ebba
};
POP_PACKED

PUSH_PACKED
struct SA_DeleteGuildmembership
{
	enum { NET_ID = 62314 };
	u32 result; // 4 bytes
	u32 membershipId; // 4 bytes
	u32 regularMembershipId; // 4 bytes
	// regularClassName: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	u16 unk_1; // 2 bytes
	// NOTE: 2 variable-size field(s) after the fixed prefix
	// logger 0x97ef60
};
POP_PACKED

PUSH_PACKED
struct SA_EditGuildmembership
{
	enum { NET_ID = 62315 };
	u32 result; // 4 bytes
	u32 membershipId; // 4 bytes
	u8 iconIndex; // 1 bytes
	// className: variable-size (string/vector)
	// rights: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x97f3e3
};
POP_PACKED

PUSH_PACKED
struct SA_DonateToGuild
{
	enum { NET_ID = 62316 };
	u32 result; // 4 bytes
	u32 donatedGold; // 4 bytes
	u32 guildFundGain; // 4 bytes
	u32 totalGuildFund; // 4 bytes
	// logger 0x97f1f2
};
POP_PACKED
ASSERT_SIZE(SA_DonateToGuild, 16);

PUSH_PACKED
struct SN_UNKNOWN_62317
{
	enum { NET_ID = 62317 };
	u32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	u32 field_2; // 4 bytes
	// logger none
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62317, 9);

PUSH_PACKED
struct SN_UNKNOWN_62318
{
	enum { NET_ID = 62318 };
	u32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	u32 field_2; // 4 bytes
	// logger none
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62318, 9);

PUSH_PACKED
struct SA_GetGuildskilllist
{
	enum { NET_ID = 62319 };
	u32 result; // 4 bytes
	u8 unk_0; // 1 bytes
	u8 unk_1; // 1 bytes
	i64 unk_2; // 8 bytes
	u16 unk_3; // 2 bytes
	// logger 0x982531
};
POP_PACKED
ASSERT_SIZE(SA_GetGuildskilllist, 16);

PUSH_PACKED
struct SA_BuyGuildskill
{
	enum { NET_ID = 62320 };
	u32 result; // 4 bytes
	u8 guildSkillType; // 1 bytes
	u8 guildSkillLvl; // 1 bytes
	i64 expiryDate; // 8 bytes
	u16 timeLimitHour; // 2 bytes
	u32 guildFundCost; // 4 bytes
	u32 totalGuildFund; // 4 bytes
	// logger 0x97d942
};
POP_PACKED
ASSERT_SIZE(SA_BuyGuildskill, 24);

PUSH_PACKED
struct SA_ExtendGuildlimitedskill
{
	enum { NET_ID = 62321 };
	u32 result; // 4 bytes
	u8 guildSkillType; // 1 bytes
	u8 guildSkillLvl; // 1 bytes
	u16 extensionCount; // 2 bytes
	i64 expiryDate; // 8 bytes
	u32 guildFundCost; // 4 bytes
	u32 totalGuildFund; // 4 bytes
	// logger 0x97faac
};
POP_PACKED
ASSERT_SIZE(SA_ExtendGuildlimitedskill, 24);

PUSH_PACKED
struct SA_GetGuildrank
{
	enum { NET_ID = 62323 };
	u32 result; // 4 bytes
	u8 rankingType; // 1 bytes
	u16 seasonNo; // 2 bytes
	// rankInfo: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u16 unk_2; // 2 bytes
	u16 unk_3; // 2 bytes
	u16 unk_4; // 2 bytes
	u32 unk_5; // 4 bytes
	// logger 0x981e43
};
POP_PACKED
ASSERT_SIZE(SA_GetGuildrank, 25);

PUSH_PACKED
struct SA_GetGuildranklist
{
	enum { NET_ID = 62324 };
	u32 result; // 4 bytes
	u8 rankingType; // 1 bytes
	u16 seasonNo; // 2 bytes
	u32 pageNo; // 4 bytes
	u8 rowsPerPage; // 1 bytes
	u32 maxPage; // 4 bytes
	i64 latestUpdateTime; // 8 bytes
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u16 unk_2; // 2 bytes
	u16 unk_3; // 2 bytes
	u16 unk_4; // 2 bytes
	u32 unk_5; // 4 bytes
	// logger 0x98212b
};
POP_PACKED
ASSERT_SIZE(SA_GetGuildranklist, 42);

PUSH_PACKED
struct SA_GetGuildrankrewardinfo
{
	enum { NET_ID = 62325 };
	u32 result; // 4 bytes
	u8 rankingType; // 1 bytes
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u16 unk_2; // 2 bytes
	u32 unk_3; // 4 bytes
	u32 unk_4; // 4 bytes
	u32 unk_5; // 4 bytes
	// logger 0x98238f
};
POP_PACKED
ASSERT_SIZE(SA_GetGuildrankrewardinfo, 27);

PUSH_PACKED
struct SA_GetRecommendedguildlist
{
	enum { NET_ID = 62326 };
	u32 result; // 4 bytes
	u16 unk_0; // 2 bytes
	u16 unk_1; // 2 bytes
	u32 unk_2; // 4 bytes
	// logger 0x982899
};
POP_PACKED
ASSERT_SIZE(SA_GetRecommendedguildlist, 12);

PUSH_PACKED
struct SA_SearchGuild
{
	enum { NET_ID = 62327 };
	u32 result; // 4 bytes
	u32 pageNo; // 4 bytes
	u8 rowsPerPage; // 1 bytes
	u32 maxPage; // 4 bytes
	u16 unk_0; // 2 bytes
	u16 unk_1; // 2 bytes
	u32 unk_2; // 4 bytes
	// logger 0x987fe2
};
POP_PACKED
ASSERT_SIZE(SA_SearchGuild, 21);

PUSH_PACKED
struct SA_GetPlayingGuildpvplist
{
	enum { NET_ID = 62328 };
	u32 result; // 4 bytes
	u16 pageNo; // 2 bytes
	u8 rowsPerPage; // 1 bytes
	u16 maxPageNo; // 2 bytes
	u32 unk_0; // 4 bytes
	i64 unk_1; // 8 bytes
	u32 unk_2; // 4 bytes
	u16 unk_3; // 2 bytes
	u32 unk_4; // 4 bytes
	u16 unk_5; // 2 bytes
	u16 unk_6; // 2 bytes
	u32 unk_7; // 4 bytes
	u16 unk_8; // 2 bytes
	u32 unk_9; // 4 bytes
	u16 unk_10; // 2 bytes
	u16 unk_11; // 2 bytes
	// logger 0x9826a5
};
POP_PACKED
ASSERT_SIZE(SA_GetPlayingGuildpvplist, 49);

PUSH_PACKED
struct SA_WatchGuildpvp
{
	enum { NET_ID = 62329 };
	u32 result; // 4 bytes
	u32 guildPvpInstanceId; // 4 bytes
	// logger 0x98a577
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
	// logger none
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62331, 12);

PUSH_PACKED
struct SN_GuildInvitation
{
	enum { NET_ID = 62332 };
	// guildName: variable-size (string/vector)
	// inviterName: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	u16 unk_1; // 2 bytes
	// NOTE: 2 variable-size field(s) after the fixed prefix
	// logger 0x997d4e
};
POP_PACKED

PUSH_PACKED
struct SN_GuildJoinapproval
{
	enum { NET_ID = 62333 };
	// guildName: variable-size (string/vector)
	// guildTag: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	u16 unk_1; // 2 bytes
	// NOTE: 2 variable-size field(s) after the fixed prefix
	// logger 0x997e1f
};
POP_PACKED

PUSH_PACKED
struct SN_GuildtagChanged
{
	enum { NET_ID = 62334 };
	// tag: variable-size (string/vector)
	u16 guildFundCost; // 2 bytes
	u32 totalGuildFund; // 4 bytes
	u32 unk_0; // 4 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x997c57
};
POP_PACKED

PUSH_PACKED
struct SN_GuildNotice
{
	enum { NET_ID = 62335 };
	// notice: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x997fc5
};
POP_PACKED

PUSH_PACKED
struct SN_GuildmemberJoin
{
	enum { NET_ID = 62336 };
	// newMember: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	u32 unk_1; // 4 bytes
	u16 unk_2; // 2 bytes
	u16 unk_3; // 2 bytes
	u16 unk_4; // 2 bytes
	u32 unk_5; // 4 bytes
	u8 unk_6; // 1 bytes
	u16 unk_7; // 2 bytes
	u32 unk_8; // 4 bytes
	u32 unk_9; // 4 bytes
	u16 unk_10; // 2 bytes
	u16 unk_11; // 2 bytes
	i64 unk_12; // 8 bytes
	// logger 0x996ffa
};
POP_PACKED
ASSERT_SIZE(SN_GuildmemberJoin, 39);

PUSH_PACKED
struct SN_GuildmemberQuit
{
	enum { NET_ID = 62337 };
	// nickname: variable-size (string/vector)
	u16 isKickedOut; // 2 bytes
	u8 unk_0; // 1 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x9970c3
};
POP_PACKED

PUSH_PACKED
struct SN_GuildmemberChgnickname
{
	enum { NET_ID = 62339 };
	// oldNickname: variable-size (string/vector)
	// newNickname: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	u16 unk_1; // 2 bytes
	// NOTE: 2 variable-size field(s) after the fixed prefix
	// logger 0x996f29
};
POP_PACKED

PUSH_PACKED
struct SN_GuildmemberChgleaderclass
{
	enum { NET_ID = 62340 };
	// nickname: variable-size (string/vector)
	u16 newLeaderClass; // 2 bytes
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x996e57
};
POP_PACKED

PUSH_PACKED
struct SN_GuildmembershipAdded
{
	enum { NET_ID = 62341 };
	u32 membershipId; // 4 bytes
	u8 iconIndex; // 1 bytes
	// className: variable-size (string/vector)
	// rights: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x996a03
};
POP_PACKED

PUSH_PACKED
struct SN_GuildmembershipRemoved
{
	enum { NET_ID = 62342 };
	u32 membershipId; // 4 bytes
	// logger 0x996c8d
};
POP_PACKED
ASSERT_SIZE(SN_GuildmembershipRemoved, 4);

PUSH_PACKED
struct SN_GuildmembershipModified
{
	enum { NET_ID = 62343 };
	u32 membershipId; // 4 bytes
	u8 iconIndex; // 1 bytes
	// className: variable-size (string/vector)
	// rights: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x996b48
};
POP_PACKED

PUSH_PACKED
struct SN_GuildmemberChgclass
{
	enum { NET_ID = 62344 };
	u32 oldMembershipId; // 4 bytes
	u32 newMembershipId; // 4 bytes
	// className: variable-size (string/vector)
	// memberNickname: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	u16 unk_1; // 2 bytes
	// NOTE: 2 variable-size field(s) after the fixed prefix
	// logger 0x996d36
};
POP_PACKED

PUSH_PACKED
struct SN_GuildmasterDelegation
{
	enum { NET_ID = 62345 };
	// formerMasterNickname: variable-size (string/vector)
	u16 formerMasterMembershipId; // 2 bytes
	// newMasterNickname: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	u16 unk_1; // 2 bytes
	// NOTE: 2 variable-size field(s) after the fixed prefix
	// logger 0x99690b
};
POP_PACKED

PUSH_PACKED
struct SN_GuildfundAdded
{
	enum { NET_ID = 62346 };
	// memberNickname: variable-size (string/vector)
	u16 guildFundGain; // 2 bytes
	u32 totalGuildFund; // 4 bytes
	u32 unk_0; // 4 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x996814
};
POP_PACKED

PUSH_PACKED
struct SN_GuildemblemChange
{
	enum { NET_ID = 62347 };
	u32 emblemIndex; // 4 bytes
	// logger 0x99676b
};
POP_PACKED
ASSERT_SIZE(SN_GuildemblemChange, 4);

PUSH_PACKED
struct SN_GuildemblemAdded
{
	enum { NET_ID = 62348 };
	u32 emblemIndex; // 4 bytes
	u8 assetChangeReason; // 1 bytes
	// logger 0x996698
};
POP_PACKED
ASSERT_SIZE(SN_GuildemblemAdded, 5);

PUSH_PACKED
struct SN_UNKNOWN_62349
{
	enum { NET_ID = 62349 };
	u8 field_0; // 1 bytes
	// logger none
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62349, 1);

PUSH_PACKED
struct SN_GuildskillAcquired
{
	enum { NET_ID = 62350 };
	// memberNickname: variable-size (string/vector)
	u16 skillType; // 2 bytes
	u8 skillLvl; // 1 bytes
	u8 timeLimitHour; // 1 bytes
	u16 expiryDate; // 2 bytes
	i64 guildFundCost; // 8 bytes
	u32 totalGuildFund; // 4 bytes
	u32 unk_0; // 4 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x99775a
};
POP_PACKED

PUSH_PACKED
struct SN_GuildskillUpgraded
{
	enum { NET_ID = 62351 };
	// memberNickname: variable-size (string/vector)
	u16 skillType; // 2 bytes
	u8 skillLvl; // 1 bytes
	u8 timeLimitHour; // 1 bytes
	u16 expiryDate; // 2 bytes
	i64 guildFundCost; // 8 bytes
	u32 totalGuildFund; // 4 bytes
	u32 unk_0; // 4 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x997abb
};
POP_PACKED

PUSH_PACKED
struct SN_GuildskillExtended
{
	enum { NET_ID = 62352 };
	// memberNickname: variable-size (string/vector)
	u16 skillType; // 2 bytes
	u8 skillLvl; // 1 bytes
	u8 skillExtensionCount; // 1 bytes
	u16 extensionTimeHour; // 2 bytes
	u16 skillExpiryDate; // 2 bytes
	i64 guildFundCost; // 8 bytes
	u32 totalGuildFund; // 4 bytes
	u32 unk_0; // 4 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x9978f6
};
POP_PACKED

PUSH_PACKED
struct SN_GuildconstraintChange
{
	enum { NET_ID = 62353 };
	i64 changeDate; // 8 bytes
	// constraint: variable-size (string/vector)
	u8 unk_0; // 1 bytes
	u8 unk_1; // 1 bytes
	u8 unk_2; // 1 bytes
	u16 unk_3; // 2 bytes
	u16 unk_4; // 2 bytes
	// logger 0x9965a1
};
POP_PACKED
ASSERT_SIZE(SN_GuildconstraintChange, 15);

PUSH_PACKED
struct SN_GuildmissionCompleted
{
	enum { NET_ID = 62354 };
	// memberNickname: variable-size (string/vector)
	u8 missionIndex; // 1 bytes
	u32 rewardedGuildFund; // 4 bytes
	u16 totalGuildFund; // 2 bytes
	u16 contributedGuildFund; // 2 bytes
	u16 unk_0; // 2 bytes
	u16 unk_1; // 2 bytes
	u16 unk_2; // 2 bytes
	// NOTE: 2 variable-size field(s) after the fixed prefix
	// logger 0x9972db
};
POP_PACKED

PUSH_PACKED
struct SN_GuildpvpResult
{
	enum { NET_ID = 62355 };
	u8 guildPvpResultType; // 1 bytes
	u32 rp; // 4 bytes
	u16 win; // 2 bytes
	u16 draw; // 2 bytes
	u16 lose; // 2 bytes
	// oppGuildName: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	u16 unk_1; // 2 bytes
	// NOTE: 2 variable-size field(s) after the fixed prefix
	// logger 0x997542
};
POP_PACKED

PUSH_PACKED
struct SN_GuildpointandfundChange
{
	enum { NET_ID = 62356 };
	i64 changeDate; // 8 bytes
	u32 totalGuildPoint; // 4 bytes
	u32 seasonalGuildPoint; // 4 bytes
	u32 totalGuildFund; // 4 bytes
	// logger 0x997420
};
POP_PACKED
ASSERT_SIZE(SN_GuildpointandfundChange, 20);

PUSH_PACKED
struct SN_GuildLevelup
{
	enum { NET_ID = 62357 };
	u8 prevGuildLvl; // 1 bytes
	u8 currGuildLvl; // 1 bytes
	// logger 0x997ef0
};
POP_PACKED
ASSERT_SIZE(SN_GuildLevelup, 2);

PUSH_PACKED
struct SN_GuildchannelLeave
{
	enum { NET_ID = 62359 };
	// guildName: variable-size (string/vector)
	// nickname: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	u16 unk_1; // 2 bytes
	// NOTE: 2 variable-size field(s) after the fixed prefix
	// logger 0x9964d0
};
POP_PACKED

PUSH_PACKED
struct SN_PlayerBlink
{
	enum { NET_ID = 62362 };
	u32 entityID; // 4 bytes
	u32 targetID; // 4 bytes
	// CurPos: variable-size (string/vector)
	// NewPos: variable-size (string/vector)
	// NewDir: variable-size (string/vector)
	f32 nSpeed; // 4 bytes
	u32 nState; // 4 bytes
	u32 bGhostBlink; // 4 bytes
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	u32 unk_4; // 4 bytes
	u32 unk_5; // 4 bytes
	u32 unk_6; // 4 bytes
	u8 unk_7; // 1 bytes
	// logger 0x99d9d0
};
POP_PACKED
ASSERT_SIZE(SN_PlayerBlink, 49);

PUSH_PACKED
struct SN_InvalidRemoteLog
{
	enum { NET_ID = 62363 };
	// log: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x998b5b
};
POP_PACKED

PUSH_PACKED
struct SN_SyncActionMove
{
	enum { NET_ID = 62366 };
	u32 entityID; // 4 bytes
	u32 targetID; // 4 bytes
	// TargetPos: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	// logger 0x9a6d35
};
POP_PACKED
ASSERT_SIZE(SN_SyncActionMove, 20);

PUSH_PACKED
struct SN_SyncStanceType
{
	enum { NET_ID = 62367 };
	u32 entityID; // 4 bytes
	u32 StanceType; // 4 bytes
	f32 durationtimeSec; // 4 bytes
	// logger 0x9a6e50
};
POP_PACKED
ASSERT_SIZE(SN_SyncStanceType, 12);

PUSH_PACKED
struct SN_AiSyncMoveMotion
{
	enum { NET_ID = 62368 };
	u32 entityID; // 4 bytes
	u32 dwMotionType; // 4 bytes
	u32 dwMotionTypeKey; // 4 bytes
	u32 dwAIMoveType; // 4 bytes
	u32 dwAIMoveVariationType; // 4 bytes
	// vPos: variable-size (string/vector)
	u32 isAttack; // 4 bytes
	u32 targetId; // 4 bytes
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	// logger 0x98c9c1
};
POP_PACKED
ASSERT_SIZE(SN_AiSyncMoveMotion, 40);

PUSH_PACKED
struct SN_AiSyncBehaviorMotion
{
	enum { NET_ID = 62369 };
	u32 entityID; // 4 bytes
	u32 dwMotionType; // 4 bytes
	u8 stance; // 1 bytes
	// vPos: variable-size (string/vector)
	// vDir: variable-size (string/vector)
	// vUpperDir: variable-size (string/vector)
	// p3nPos: variable-size (string/vector)
	u32 isAttack; // 4 bytes
	u32 targetId; // 4 bytes
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	u32 unk_4; // 4 bytes
	u32 unk_5; // 4 bytes
	u32 unk_6; // 4 bytes
	u32 unk_7; // 4 bytes
	u32 unk_8; // 4 bytes
	u32 unk_9; // 4 bytes
	u32 unk_10; // 4 bytes
	u32 unk_11; // 4 bytes
	// logger 0x98c369
};
POP_PACKED
ASSERT_SIZE(SN_AiSyncBehaviorMotion, 65);

PUSH_PACKED
struct SN_AiSyncCooltime
{
	enum { NET_ID = 62370 };
	u32 entityID; // 4 bytes
	// cooltimeID: variable-size (string/vector)
	u16 fCooltime; // 2 bytes
	u32 nodeID; // 4 bytes
	u32 unk_0; // 4 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x98c77e
};
POP_PACKED

PUSH_PACKED
struct SN_AiSyncSpeedRatio
{
	enum { NET_ID = 62371 };
	u32 entityID; // 4 bytes
	f32 fRatio; // 4 bytes
	// logger 0x98ce76
};
POP_PACKED
ASSERT_SIZE(SN_AiSyncSpeedRatio, 8);

PUSH_PACKED
struct SN_AiSyncRotateRatio
{
	enum { NET_ID = 62372 };
	u32 entityID; // 4 bytes
	f32 fRatio; // 4 bytes
	// logger 0x98ccc4
};
POP_PACKED
ASSERT_SIZE(SN_AiSyncRotateRatio, 8);

PUSH_PACKED
struct SN_AiSyncSceneroot
{
	enum { NET_ID = 62373 };
	u32 entityID; // 4 bytes
	f32 wantedYaw; // 4 bytes
	// logger 0x98cd9d
};
POP_PACKED
ASSERT_SIZE(SN_AiSyncSceneroot, 8);

PUSH_PACKED
struct SN_AiSyncTarpos
{
	enum { NET_ID = 62374 };
	u32 entityID; // 4 bytes
	u32 dwSentTime; // 4 bytes
	// vSPos: variable-size (string/vector)
	// vPos: variable-size (string/vector)
	// vDir: variable-size (string/vector)
	// vUpperDir: variable-size (string/vector)
	f32 fSpeed; // 4 bytes
	f32 fFrameRate; // 4 bytes
	f32 fUpForce; // 4 bytes
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	u32 unk_4; // 4 bytes
	u32 unk_5; // 4 bytes
	u32 unk_6; // 4 bytes
	u32 unk_7; // 4 bytes
	u32 unk_8; // 4 bytes
	u32 unk_9; // 4 bytes
	u32 unk_10; // 4 bytes
	u32 unk_11; // 4 bytes
	// logger 0x98d020
};
POP_PACKED
ASSERT_SIZE(SN_AiSyncTarpos, 68);

PUSH_PACKED
struct SN_AiSyncTarget
{
	enum { NET_ID = 62375 };
	u32 entityID; // 4 bytes
	u32 targetID; // 4 bytes
	// logger 0x98cf4f
};
POP_PACKED
ASSERT_SIZE(SN_AiSyncTarget, 8);

PUSH_PACKED
struct SN_AiAddCom
{
	enum { NET_ID = 62376 };
	u32 entityID; // 4 bytes
	// strComName: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x98be42
};
POP_PACKED

PUSH_PACKED
struct SN_AiSyncBodyYaw
{
	enum { NET_ID = 62377 };
	u32 entityID; // 4 bytes
	f32 fradian; // 4 bytes
	// logger 0x98c6a5
};
POP_PACKED
ASSERT_SIZE(SN_AiSyncBodyYaw, 8);

PUSH_PACKED
struct SN_AiSyncBodyPitch
{
	enum { NET_ID = 62378 };
	u32 entityID; // 4 bytes
	f32 fradian; // 4 bytes
	// logger 0x98c5cc
};
POP_PACKED
ASSERT_SIZE(SN_AiSyncBodyPitch, 8);

PUSH_PACKED
struct SN_AiSyncPhy
{
	enum { NET_ID = 62379 };
	u32 entityID; // 4 bytes
	f32 fradian; // 4 bytes
	// vPos: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	// logger 0x98cba1
};
POP_PACKED
ASSERT_SIZE(SN_AiSyncPhy, 20);

PUSH_PACKED
struct SN_AiSetTarget
{
	enum { NET_ID = 62380 };
	u32 entityID; // 4 bytes
	u32 targetEntityID; // 4 bytes
	// logger 0x98c298
};
POP_PACKED
ASSERT_SIZE(SN_AiSetTarget, 8);

PUSH_PACKED
struct SN_AiSyncMonsterSkillTarget
{
	enum { NET_ID = 62381 };
	u32 entityID; // 4 bytes
	u32 targetID; // 4 bytes
	// targetPos: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	// logger 0x98c8a6
};
POP_PACKED
ASSERT_SIZE(SN_AiSyncMonsterSkillTarget, 20);

PUSH_PACKED
struct SN_AiSetActionState
{
	enum { NET_ID = 62382 };
	u8 field_0; // 1 bytes
	u32 field_1; // 4 bytes
	u16 field_2; // 2 bytes
	u16 field_3; // 2 bytes
	u16 field_4; // 2 bytes
	u32 field_5; // 4 bytes
	u32 field_6; // 4 bytes
	u32 field_7; // 4 bytes
	// logger none
};
POP_PACKED
ASSERT_SIZE(SN_AiSetActionState, 23);

PUSH_PACKED
struct SN_AiSetMovetargetpos
{
	enum { NET_ID = 62383 };
	u32 entityID; // 4 bytes
	u32 movePresetType; // 4 bytes
	// moveTargetPos: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	// logger 0x98c17d
};
POP_PACKED
ASSERT_SIZE(SN_AiSetMovetargetpos, 20);

PUSH_PACKED
struct SN_AiChangeMoveType
{
	enum { NET_ID = 62384 };
	u32 entityID; // 4 bytes
	u32 moveType; // 4 bytes
	// logger 0x98bf14
};
POP_PACKED
ASSERT_SIZE(SN_AiChangeMoveType, 8);

PUSH_PACKED
struct SN_RemoteSyncCreateFromCreatorId
{
	enum { NET_ID = 62385 };
	u32 ParentEntity; // 4 bytes
	u32 OwnerEntity; // 4 bytes
	u32 RemoteSeedID; // 4 bytes
	// FirePosition: variable-size (string/vector)
	u32 FireRotationX; // 4 bytes
	u32 FireRotationY; // 4 bytes
	u32 FireRotationZ; // 4 bytes
	// TargetPosition: variable-size (string/vector)
	// FireObject: variable-size (string/vector)
	u16 Scale; // 2 bytes
	u16 LifeTime; // 2 bytes
	u16 HitInvalidTarget; // 2 bytes
	u32 RemoteCreatorID; // 4 bytes
	u32 ErrorAngleX; // 4 bytes
	u32 ErrorAngleY; // 4 bytes
	u16 ChargingLevel; // 2 bytes
	u16 GaugeLevel; // 2 bytes
	u32 FiringCount; // 4 bytes
	u32 ComboCount; // 4 bytes
	u16 TargetEntity; // 2 bytes
	u16 RemoteCreateFlags; // 2 bytes
	u16 unk_0; // 2 bytes
	u8 unk_1; // 1 bytes
	u8 unk_2; // 1 bytes
	u16 unk_3; // 2 bytes
	u32 unk_4; // 4 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x9a18e1
};
POP_PACKED

PUSH_PACKED
struct SN_RemoteSyncCreateFromRemoteDoc
{
	enum { NET_ID = 62386 };
	u32 ParentEntity; // 4 bytes
	u32 OwnerEntity; // 4 bytes
	u32 RemoteSeedID; // 4 bytes
	// FirePosition: variable-size (string/vector)
	u32 FireRotationX; // 4 bytes
	u32 FireRotationY; // 4 bytes
	u32 FireRotationZ; // 4 bytes
	// TargetPosition: variable-size (string/vector)
	// FireObject: variable-size (string/vector)
	u16 Scale; // 2 bytes
	u16 LifeTime; // 2 bytes
	u16 HitInvalidTarget; // 2 bytes
	u32 RemoteDocIndex; // 4 bytes
	u32 TargetEntity; // 4 bytes
	u32 RemoteCreateFlags; // 4 bytes
	u16 unk_0; // 2 bytes
	u16 unk_1; // 2 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	u32 unk_4; // 4 bytes
	u32 unk_5; // 4 bytes
	u8 unk_6; // 1 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x9a1cfb
};
POP_PACKED

PUSH_PACKED
struct SN_RemoteSnapshotFromRemoteDoc
{
	enum { NET_ID = 62387 };
	u32 ParentEntity; // 4 bytes
	u32 OwnerEntity; // 4 bytes
	u32 RemoteSeedID; // 4 bytes
	// FirePosition: variable-size (string/vector)
	u32 FireRotationX; // 4 bytes
	u32 FireRotationY; // 4 bytes
	u32 FireRotationZ; // 4 bytes
	// TargetPosition: variable-size (string/vector)
	// FireObject: variable-size (string/vector)
	u16 Scale; // 2 bytes
	u16 LifeTime; // 2 bytes
	u16 DurationTime; // 2 bytes
	u32 HitInvalidTarget; // 4 bytes
	u32 RemoteDocIndex; // 4 bytes
	u32 TargetEntity; // 4 bytes
	u16 RemoteCreateFlags; // 2 bytes
	u16 unk_0; // 2 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	u32 unk_4; // 4 bytes
	u32 unk_5; // 4 bytes
	u8 unk_6; // 1 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x9a1590
};
POP_PACKED

PUSH_PACKED
struct SN_UNKNOWN_62388
{
	enum { NET_ID = 62388 };
	u8 field_0; // 1 bytes
	u32 field_1; // 4 bytes
	u32 field_2; // 4 bytes
	u32 field_3; // 4 bytes
	u32 field_4; // 4 bytes
	u32 field_5; // 4 bytes
	u32 field_6; // 4 bytes
	u32 field_7; // 4 bytes
	u32 field_8; // 4 bytes
	// logger none
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62388, 33);

PUSH_PACKED
struct SN_RemoteSyncForecastCollision
{
	enum { NET_ID = 62389 };
	u32 field_0; // 4 bytes
	u32 field_1; // 4 bytes
	u32 field_2; // 4 bytes
	u32 field_3; // 4 bytes
	u32 field_4; // 4 bytes
	u32 field_5; // 4 bytes
	u8 field_6; // 1 bytes
	// logger 0x9a201d
};
POP_PACKED
ASSERT_SIZE(SN_RemoteSyncForecastCollision, 25);

PUSH_PACKED
struct SA_CharacterSkinSelect
{
	enum { NET_ID = 62391 };
	u32 result; // 4 bytes
	u32 classType; // 4 bytes
	u32 skinIndex; // 4 bytes
	// logger 0x97e264
};
POP_PACKED
ASSERT_SIZE(SA_CharacterSkinSelect, 12);

PUSH_PACKED
struct SN_CharacterSkinUnlock
{
	enum { NET_ID = 62392 };
	u32 result; // 4 bytes
	u32 classType; // 4 bytes
	u32 skinIndex; // 4 bytes
	u32 bufCount; // 4 bytes
	// logger 0x98f490
};
POP_PACKED
ASSERT_SIZE(SN_CharacterSkinUnlock, 16);

PUSH_PACKED
struct SN_CallMonsterByRemoteControl
{
	enum { NET_ID = 62393 };
	u32 objectID; // 4 bytes
	u32 docIndex; // 4 bytes
	u8 bAIUse; // 1 bytes
	// logger 0x98e616
};
POP_PACKED
ASSERT_SIZE(SN_CallMonsterByRemoteControl, 9);

PUSH_PACKED
struct SQ_UseLiferecover
{
	enum { NET_ID = 62394 };
	u32 usn; // 4 bytes
	u32 liferecoverCount; // 4 bytes
	// logger 0x9a9274
};
POP_PACKED
ASSERT_SIZE(SQ_UseLiferecover, 8);

PUSH_PACKED
struct SQ_ReviveByLifeCount
{
	enum { NET_ID = 62395 };
	u32 countdownPeriod; // 4 bytes
	// logger 0x9a90d7
};
POP_PACKED
ASSERT_SIZE(SQ_ReviveByLifeCount, 4);

PUSH_PACKED
struct SQ_ReviveByCoin
{
	enum { NET_ID = 62396 };
	u32 usn; // 4 bytes
	// logger 0x9a902e
};
POP_PACKED
ASSERT_SIZE(SQ_ReviveByCoin, 4);

PUSH_PACKED
struct SN_RevivePlayer
{
	enum { NET_ID = 62397 };
	u32 usn; // 4 bytes
	u32 activeID; // 4 bytes
	u32 inactiveID; // 4 bytes
	u32 followID; // 4 bytes
	u8 reviveBySelf; // 1 bytes
	// position: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	// logger 0x9a3099
};
POP_PACKED
ASSERT_SIZE(SN_RevivePlayer, 29);

PUSH_PACKED
struct SN_RevivePlayerAtStartingPoint
{
	enum { NET_ID = 62398 };
	u32 usn; // 4 bytes
	u32 activeID; // 4 bytes
	u32 inactiveID; // 4 bytes
	// position: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	// logger 0x9a322b
};
POP_PACKED
ASSERT_SIZE(SN_RevivePlayerAtStartingPoint, 24);

PUSH_PACKED
struct SN_RespawnDelaytime
{
	enum { NET_ID = 62399 };
	u32 usn; // 4 bytes
	u32 respawnDelayTimeMS; // 4 bytes
	// logger 0x9a2624
};
POP_PACKED
ASSERT_SIZE(SN_RespawnDelaytime, 8);

PUSH_PACKED
struct SN_WarnPveGameend
{
	enum { NET_ID = 62401 };
	u32 remainTimeMS; // 4 bytes
	// logger 0x9a86fa
};
POP_PACKED
ASSERT_SIZE(SN_WarnPveGameend, 4);

PUSH_PACKED
struct SN_StartGameNotQualified
{
	enum { NET_ID = 62402 };
	u32 reason; // 4 bytes
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x9a6170
};
POP_PACKED

PUSH_PACKED
struct SQ_KickFromGameserver
{
	enum { NET_ID = 62403 };
	u32 reason; // 4 bytes
	// logger 0x9a8b08
};
POP_PACKED
ASSERT_SIZE(SQ_KickFromGameserver, 4);

PUSH_PACKED
struct SN_WarehouseItemUpdate
{
	enum { NET_ID = 62405 };
	u32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	u32 field_2; // 4 bytes
	u32 field_3; // 4 bytes
	u32 field_4; // 4 bytes
	u32 field_5; // 4 bytes
	u8 field_6; // 1 bytes
	i64 field_7; // 8 bytes
	u16 field_8; // 2 bytes
	u8 field_9; // 1 bytes
	u32 field_10; // 4 bytes
	u8 field_11; // 1 bytes
	u32 field_12; // 4 bytes
	u8 field_13; // 1 bytes
	// logger 0x9a85b4
};
POP_PACKED
ASSERT_SIZE(SN_WarehouseItemUpdate, 43);

PUSH_PACKED
struct SA_WarehouseExpand
{
	enum { NET_ID = 62408 };
	u32 result; // 4 bytes
	u32 lineCount; // 4 bytes
	// logger 0x98a20e
};
POP_PACKED
ASSERT_SIZE(SA_WarehouseExpand, 8);

PUSH_PACKED
struct SA_UserAfk
{
	enum { NET_ID = 62409 };
	u32 result; // 4 bytes
	// logger 0x989eb6
};
POP_PACKED
ASSERT_SIZE(SA_UserAfk, 4);

PUSH_PACKED
struct SN_RewardItemList
{
	enum { NET_ID = 62410 };
	u32 spSlotIndex; // 4 bytes
	u32 vipSlotCount; // 4 bytes
	u32 guildSkillSlotCount; // 4 bytes
	u32 pcCafeSlotCount; // 4 bytes
	u8 unk_0; // 1 bytes
	u8 unk_1; // 1 bytes
	u8 unk_2; // 1 bytes
	u32 unk_3; // 4 bytes
	// logger 0x9a336d
};
POP_PACKED
ASSERT_SIZE(SN_RewardItemList, 23);

PUSH_PACKED
struct SN_RewardStageStart
{
	enum { NET_ID = 62411 };
	u32 timeToWaitSec; // 4 bytes
	u8 nextAction; // 1 bytes
	u32 nextActionCost; // 4 bytes
	u8 slotCount; // 1 bytes
	u32 slotRewardPenaltyReason; // 4 bytes
	// logger 0x9a3833
};
POP_PACKED
ASSERT_SIZE(SN_RewardStageStart, 14);

PUSH_PACKED
struct SN_RewardStageStartNp
{
	enum { NET_ID = 62412 };
	u32 timeToWaitSec; // 4 bytes
	u8 nextAction; // 1 bytes
	u8 slotCount; // 1 bytes
	u32 slotRewardPenaltyReason; // 4 bytes
	// logger 0x9a397d
};
POP_PACKED
ASSERT_SIZE(SN_RewardStageStartNp, 10);

PUSH_PACKED
struct SA_RewardSelectSlot
{
	enum { NET_ID = 62413 };
	u32 retval; // 4 bytes
	u32 slotIndex; // 4 bytes
	// rewardItem: variable-size (string/vector)
	u32 isSpecialSlot; // 4 bytes
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u8 unk_2; // 1 bytes
	// logger 0x987b98
};
POP_PACKED
ASSERT_SIZE(SA_RewardSelectSlot, 21);

PUSH_PACKED
struct SA_RewardSelectSlotAllRandom
{
	enum { NET_ID = 62414 };
	u32 retval; // 4 bytes
	u32 spSlotIndex; // 4 bytes
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	// logger 0x987cde
};
POP_PACKED
ASSERT_SIZE(SA_RewardSelectSlotAllRandom, 24);

PUSH_PACKED
struct SN_RewardStageEnd
{
	enum { NET_ID = 62415 };
	u32 spSlotIndex; // 4 bytes
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	u16 unk_3; // 2 bytes
	u16 unk_4; // 2 bytes
	u32 unk_5; // 4 bytes
	u32 unk_6; // 4 bytes
	// logger 0x9a35f7
};
POP_PACKED
ASSERT_SIZE(SN_RewardStageEnd, 28);

PUSH_PACKED
struct SA_RewardBuyChance
{
	enum { NET_ID = 62416 };
	u32 retval; // 4 bytes
	u32 newSlotCount; // 4 bytes
	u8 nextAction; // 1 bytes
	u32 nextActionCost; // 4 bytes
	// logger 0x9879ce
};
POP_PACKED
ASSERT_SIZE(SA_RewardBuyChance, 13);

PUSH_PACKED
struct SA_RewardRequestFinish
{
	enum { NET_ID = 62417 };
	u32 retval; // 4 bytes
	// logger 0x987aef
};
POP_PACKED
ASSERT_SIZE(SA_RewardRequestFinish, 4);

PUSH_PACKED
struct SA_WarehouseItemChange
{
	enum { NET_ID = 62418 };
	u32 orgItemID; // 4 bytes
	u32 orgInvenType; // 4 bytes
	u32 targetInvenType; // 4 bytes
	u32 targetSlot; // 4 bytes
	u32 nErrorType; // 4 bytes
	// logger 0x98a2df
};
POP_PACKED
ASSERT_SIZE(SA_WarehouseItemChange, 20);

PUSH_PACKED
struct SN_MailList
{
	enum { NET_ID = 62419 };
	u8 packetNum; // 1 bytes
	u8 mailboxType; // 1 bytes
	i64 unk_0; // 8 bytes
	i64 unk_1; // 8 bytes
	i64 unk_2; // 8 bytes
	u8 unk_3; // 1 bytes
	u8 unk_4; // 1 bytes
	u8 unk_5; // 1 bytes
	u16 unk_6; // 2 bytes
	u16 unk_7; // 2 bytes
	u16 unk_8; // 2 bytes
	u32 unk_9; // 4 bytes
	// logger 0x99a0bd
};
POP_PACKED
ASSERT_SIZE(SN_MailList, 39);

PUSH_PACKED
struct SN_MailBlocklist
{
	enum { NET_ID = 62420 };
	u8 packetNum; // 1 bytes
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x999d11
};
POP_PACKED

PUSH_PACKED
struct SN_MailWriteResult
{
	enum { NET_ID = 62421 };
	u32 nErrorType; // 4 bytes
	// logger 0x99a9c4
};
POP_PACKED
ASSERT_SIZE(SN_MailWriteResult, 4);

PUSH_PACKED
struct SN_MailRead
{
	enum { NET_ID = 62422 };
	i64 mailId; // 8 bytes
	u8 isRead; // 1 bytes
	u8 isGMMail; // 1 bytes
	// fromNickname: variable-size (string/vector)
	// subject: variable-size (string/vector)
	// content: variable-size (string/vector)
	u16 sendUTCDate; // 2 bytes
	u16 expireUTCDate; // 2 bytes
	u16 money; // 2 bytes
	i64 exp; // 8 bytes
	i64 ccoin; // 8 bytes
	i64 guildFund; // 8 bytes
	i64 unk_0; // 8 bytes
	i64 unk_1; // 8 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	u32 unk_4; // 4 bytes
	// NOTE: 3 variable-size field(s) after the fixed prefix
	// logger 0x99a480
};
POP_PACKED

PUSH_PACKED
struct SN_MailGetAttachmentResult
{
	enum { NET_ID = 62423 };
	u32 nErrorType; // 4 bytes
	i64 mailId; // 8 bytes
	i64 expireUTCDate; // 8 bytes
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	// logger 0x999e5a
};
POP_PACKED
ASSERT_SIZE(SN_MailGetAttachmentResult, 36);

PUSH_PACKED
struct SN_MailMoveResult
{
	enum { NET_ID = 62424 };
	u32 nErrorType; // 4 bytes
	// logger 0x99a3d7
};
POP_PACKED
ASSERT_SIZE(SN_MailMoveResult, 4);

PUSH_PACKED
struct SN_GameEventActivated
{
	enum { NET_ID = 62427 };
	u32 field_0; // 4 bytes
	// logger 0x9944c3
};
POP_PACKED
ASSERT_SIZE(SN_GameEventActivated, 4);

PUSH_PACKED
struct SN_GameEventPending
{
	enum { NET_ID = 62428 };
	u32 field_0; // 4 bytes
	// logger 0x994677
};
POP_PACKED
ASSERT_SIZE(SN_GameEventPending, 4);

PUSH_PACKED
struct SN_GameEventStart
{
	enum { NET_ID = 62429 };
	u32 eventIndex; // 4 bytes
	// logger 0x994782
};
POP_PACKED
ASSERT_SIZE(SN_GameEventStart, 4);

PUSH_PACKED
struct SN_GameEventEnd
{
	enum { NET_ID = 62430 };
	u32 eventIndex; // 4 bytes
	// logger 0x9945ce
};
POP_PACKED
ASSERT_SIZE(SN_GameEventEnd, 4);

PUSH_PACKED
struct SN_PartyCancelInvite
{
	enum { NET_ID = 62431 };
	u32 partyId; // 4 bytes
	// logger 0x99cc9e
};
POP_PACKED
ASSERT_SIZE(SN_PartyCancelInvite, 4);

PUSH_PACKED
struct SN_ActivatedSupportkit
{
	enum { NET_ID = 62432 };
	u32 characterID; // 4 bytes
	u32 supportKitItemIndex; // 4 bytes
	u32 cooltimeId; // 4 bytes
	f32 cooltimeSec; // 4 bytes
	// logger 0x98b525
};
POP_PACKED
ASSERT_SIZE(SN_ActivatedSupportkit, 16);

PUSH_PACKED
struct SN_NotifyCooltime
{
	enum { NET_ID = 62433 };
	u32 characterID; // 4 bytes
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	// logger 0x99c2e8
};
POP_PACKED
ASSERT_SIZE(SN_NotifyCooltime, 20);

PUSH_PACKED
struct SN_MailListNewInbox
{
	enum { NET_ID = 62434 };
	u8 mailboxType; // 1 bytes
	i64 unk_0; // 8 bytes
	i64 unk_1; // 8 bytes
	i64 unk_2; // 8 bytes
	u8 unk_3; // 1 bytes
	u8 unk_4; // 1 bytes
	u8 unk_5; // 1 bytes
	u16 unk_6; // 2 bytes
	u16 unk_7; // 2 bytes
	u16 unk_8; // 2 bytes
	u32 unk_9; // 4 bytes
	// logger 0x99a261
};
POP_PACKED
ASSERT_SIZE(SN_MailListNewInbox, 38);

PUSH_PACKED
struct SN_PlayerServerPosition
{
	enum { NET_ID = 62435 };
	// Pos: variable-size (string/vector)
	// MoveDir: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	u32 unk_4; // 4 bytes
	// logger 0x99dbf6
};
POP_PACKED
ASSERT_SIZE(SN_PlayerServerPosition, 20);

PUSH_PACKED
struct SN_RemoteServerPosition
{
	enum { NET_ID = 62436 };
	i64 remoteServerID; // 8 bytes
	// pos: variable-size (string/vector)
	// dir: variable-size (string/vector)
	u32 boundType; // 4 bytes
	u32 durationType; // 4 bytes
	u32 colorType; // 4 bytes
	// dim: variable-size (string/vector)
	u32 flag; // 4 bytes
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u8 unk_2; // 1 bytes
	u8 unk_3; // 1 bytes
	u8 unk_4; // 1 bytes
	u32 unk_5; // 4 bytes
	u32 unk_6; // 4 bytes
	u32 unk_7; // 4 bytes
	u8 unk_8; // 1 bytes
	// logger 0x9a1369
};
POP_PACKED
ASSERT_SIZE(SN_RemoteServerPosition, 48);

PUSH_PACKED
struct SN_MonsterServerPosition
{
	enum { NET_ID = 62437 };
	u32 monsterID; // 4 bytes
	// Pos: variable-size (string/vector)
	// MoveDir: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	u32 unk_4; // 4 bytes
	// logger 0x99bbdb
};
POP_PACKED
ASSERT_SIZE(SN_MonsterServerPosition, 24);

PUSH_PACKED
struct SN_MonsterServerChase
{
	enum { NET_ID = 62438 };
	u32 monsterID; // 4 bytes
	f32 chaseNearRadius; // 4 bytes
	f32 chaseFarRadius; // 4 bytes
	// logger 0x99bad3
};
POP_PACKED
ASSERT_SIZE(SN_MonsterServerChase, 12);

PUSH_PACKED
struct SN_RegameData
{
	enum { NET_ID = 62439 };
	u32 ownerUserId; // 4 bytes
	u32 areaIndex; // 4 bytes
	// logger 0x9a0d0a
};
POP_PACKED
ASSERT_SIZE(SN_RegameData, 8);

PUSH_PACKED
struct SN_RegameAvailable
{
	enum { NET_ID = 62440 };
	u32 gameType; // 4 bytes
	u32 stageIndex; // 4 bytes
	u8 bRetry; // 1 bytes
	u8 reasonCode; // 1 bytes
	// logger 0x9a0be7
};
POP_PACKED
ASSERT_SIZE(SN_RegameAvailable, 10);

PUSH_PACKED
struct SA_PvpRecord
{
	enum { NET_ID = 62441 };
	u8 field_0; // 1 bytes
	u8 field_1; // 1 bytes
	u32 field_2; // 4 bytes
	u32 field_3; // 4 bytes
	u32 field_4; // 4 bytes
	u32 field_5; // 4 bytes
	u32 field_6; // 4 bytes
	u32 field_7; // 4 bytes
	u32 field_8; // 4 bytes
	u32 field_9; // 4 bytes
	u32 field_10; // 4 bytes
	u8 field_11; // 1 bytes
	u32 field_12; // 4 bytes
	u32 field_13; // 4 bytes
	u32 field_14; // 4 bytes
	u32 field_15; // 4 bytes
	// logger 0x9867c6
};
POP_PACKED
ASSERT_SIZE(SA_PvpRecord, 55);

PUSH_PACKED
struct SA_PvpDetailRecord
{
	enum { NET_ID = 62442 };
	u32 gameType; // 4 bytes
	u8 pvpSaveType; // 1 bytes
	// stPvpMasterRecord: variable-size (string/vector)
	u32 olympicTotalPlayCount; // 4 bytes
	u32 olympicWorldRecord; // 4 bytes
	// olympicWorldRecordNickname: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u8 unk_2; // 1 bytes
	u32 unk_3; // 4 bytes
	u32 unk_4; // 4 bytes
	u16 unk_5; // 2 bytes
	u16 unk_6; // 2 bytes
	u16 unk_7; // 2 bytes
	u32 unk_8; // 4 bytes
	i64 unk_9; // 8 bytes
	u32 unk_10; // 4 bytes
	u32 unk_11; // 4 bytes
	u16 unk_12; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x9863cc
};
POP_PACKED

PUSH_PACKED
struct SN_SummaryRewardResult
{
	enum { NET_ID = 62443 };
	u32 result; // 4 bytes
	// summaryInfo: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	u8 unk_4; // 1 bytes
	i64 unk_5; // 8 bytes
	// logger 0x9a67b2
};
POP_PACKED
ASSERT_SIZE(SN_SummaryRewardResult, 29);

PUSH_PACKED
struct SN_ReduceCooltime
{
	enum { NET_ID = 62444 };
	u32 characterID; // 4 bytes
	u32 cooltimeID; // 4 bytes
	u32 valueType; // 4 bytes
	f32 value; // 4 bytes
	// logger 0x9a0ac0
};
POP_PACKED
ASSERT_SIZE(SN_ReduceCooltime, 16);

PUSH_PACKED
struct SN_BroadcastGamePingData
{
	enum { NET_ID = 62445 };
	u32 characterID; // 4 bytes
	// pingPos: variable-size (string/vector)
	u32 targetID; // 4 bytes
	u32 pingData; // 4 bytes
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	// logger 0x98dfd9
};
POP_PACKED
ASSERT_SIZE(SN_BroadcastGamePingData, 24);

PUSH_PACKED
struct SN_BroadcastEvade
{
	enum { NET_ID = 62447 };
	u32 attID; // 4 bytes
	u32 defID; // 4 bytes
	u32 remoteID; // 4 bytes
	u32 remoteDocIndex; // 4 bytes
	// logger 0x98deba
};
POP_PACKED
ASSERT_SIZE(SN_BroadcastEvade, 16);

PUSH_PACKED
struct SN_ErrorMessage
{
	enum { NET_ID = 62451 };
	u8 type; // 1 bytes
	u32 duration; // 4 bytes
	u32 errCode; // 4 bytes
	// param1: variable-size (string/vector)
	// param2: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	u16 unk_1; // 2 bytes
	// NOTE: 2 variable-size field(s) after the fixed prefix
	// logger 0x9920a9
};
POP_PACKED

PUSH_PACKED
struct SN_BushObjectState
{
	enum { NET_ID = 62452 };
	u32 objectID; // 4 bytes
	u32 bushID; // 4 bytes
	// logger 0x98e2b1
};
POP_PACKED
ASSERT_SIZE(SN_BushObjectState, 8);

PUSH_PACKED
struct SN_BushState
{
	enum { NET_ID = 62453 };
	u32 bushID; // 4 bytes
	u32 curState; // 4 bytes
	// logger 0x98e382
};
POP_PACKED
ASSERT_SIZE(SN_BushState, 8);

PUSH_PACKED
struct SN_HideDetect
{
	enum { NET_ID = 62454 };
	u32 hideDetectorOwner; // 4 bytes
	u32 targetID; // 4 bytes
	u8 flag; // 1 bytes
	// logger 0x99806d
};
POP_PACKED
ASSERT_SIZE(SN_HideDetect, 9);

PUSH_PACKED
struct SN_SortieMasterBanPhaseStart
{
	enum { NET_ID = 62456 };
	u32 alliesLeaderUserId; // 4 bytes
	u32 enemiesLeaderUserId; // 4 bytes
	u32 timeSec; // 4 bytes
	u32 banCount; // 4 bytes
	u32 unk_0; // 4 bytes
	u16 unk_1; // 2 bytes
	u32 unk_2; // 4 bytes
	// logger 0x9a4b2a
};
POP_PACKED
ASSERT_SIZE(SN_SortieMasterBanPhaseStart, 26);

PUSH_PACKED
struct SN_SortieMasterAssignPhaseStart
{
	enum { NET_ID = 62457 };
	u32 alliesLeaderUserId; // 4 bytes
	u32 enemiesLeaderUserId; // 4 bytes
	u32 timeSec; // 4 bytes
	u32 assignCount; // 4 bytes
	u32 unk_0; // 4 bytes
	u16 unk_1; // 2 bytes
	u32 unk_2; // 4 bytes
	// logger 0x9a4761
};
POP_PACKED
ASSERT_SIZE(SN_SortieMasterAssignPhaseStart, 26);

PUSH_PACKED
struct SA_SortieMasterBan
{
	enum { NET_ID = 62458 };
	u32 retval; // 4 bytes
	// logger 0x989245
};
POP_PACKED
ASSERT_SIZE(SA_SortieMasterBan, 4);

PUSH_PACKED
struct SN_SortieMasterBan
{
	enum { NET_ID = 62459 };
	u32 teamType; // 4 bytes
	u32 creatureIndex; // 4 bytes
	// logger 0x9a4947
};
POP_PACKED
ASSERT_SIZE(SN_SortieMasterBan, 8);

PUSH_PACKED
struct SA_SortieMasterAssign
{
	enum { NET_ID = 62460 };
	u32 retval; // 4 bytes
	// logger 0x98919c
};
POP_PACKED
ASSERT_SIZE(SA_SortieMasterAssign, 4);

PUSH_PACKED
struct SN_SortieMasterAssign
{
	enum { NET_ID = 62461 };
	u32 teamType; // 4 bytes
	u32 creatureIndex; // 4 bytes
	// logger 0x9a457e
};
POP_PACKED
ASSERT_SIZE(SN_SortieMasterAssign, 8);

PUSH_PACKED
struct SN_SortieMasterBanPhaseEnd
{
	enum { NET_ID = 62463 };
	// alliesBanInfo: variable-size (string/vector)
	// enemiesBanInfo: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	u32 unk_1; // 4 bytes
	u16 unk_2; // 2 bytes
	u32 unk_3; // 4 bytes
	// logger 0x9a4a18
};
POP_PACKED
ASSERT_SIZE(SN_SortieMasterBanPhaseEnd, 12);

PUSH_PACKED
struct SN_SortieMasterAssignPhaseEnd
{
	enum { NET_ID = 62464 };
	// alliesAssignInfo: variable-size (string/vector)
	// enemiesAssignInfo: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	u16 unk_1; // 2 bytes
	u32 unk_2; // 4 bytes
	u16 unk_3; // 2 bytes
	u32 unk_4; // 4 bytes
	u32 unk_5; // 4 bytes
	u16 unk_6; // 2 bytes
	u32 unk_7; // 4 bytes
	u16 unk_8; // 2 bytes
	u32 unk_9; // 4 bytes
	// logger 0x9a464f
};
POP_PACKED
ASSERT_SIZE(SN_SortieMasterAssignPhaseEnd, 32);

PUSH_PACKED
struct SA_TierStageRecord
{
	enum { NET_ID = 62470 };
	u8 seasonId; // 1 bytes
	// stageRecord: variable-size (string/vector)
	u8 sectorId; // 1 bytes
	u8 unk_0; // 1 bytes
	u16 unk_1; // 2 bytes
	u16 unk_2; // 2 bytes
	u8 unk_3; // 1 bytes
	u32 unk_4; // 4 bytes
	u32 unk_5; // 4 bytes
	u32 unk_6; // 4 bytes
	u32 unk_7; // 4 bytes
	u32 unk_8; // 4 bytes
	u8 unk_9; // 1 bytes
	u16 unk_10; // 2 bytes
	u16 unk_11; // 2 bytes
	u32 unk_12; // 4 bytes
	u32 unk_13; // 4 bytes
	u32 unk_14; // 4 bytes
	u32 unk_15; // 4 bytes
	u32 unk_16; // 4 bytes
	// logger 0x989966
};
POP_PACKED
ASSERT_SIZE(SA_TierStageRecord, 53);

PUSH_PACKED
struct SA_PvpRanking
{
	enum { NET_ID = 62471 };
	u8 seasonId; // 1 bytes
	u8 pvpSaveType; // 1 bytes
	u8 unk_0; // 1 bytes
	u16 unk_1; // 2 bytes
	u8 unk_2; // 1 bytes
	u16 unk_3; // 2 bytes
	u32 unk_4; // 4 bytes
	u32 unk_5; // 4 bytes
	u32 unk_6; // 4 bytes
	u32 unk_7; // 4 bytes
	u32 unk_8; // 4 bytes
	// logger 0x986622
};
POP_PACKED
ASSERT_SIZE(SA_PvpRanking, 28);

PUSH_PACKED
struct SA_PveRanking
{
	enum { NET_ID = 62475 };
	u32 stageId; // 4 bytes
	u32 packetNum; // 4 bytes
	// highScore: variable-size (string/vector)
	u32 highScoreClearRankType; // 4 bytes
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u16 unk_2; // 2 bytes
	u32 unk_3; // 4 bytes
	u32 unk_4; // 4 bytes
	u16 unk_5; // 2 bytes
	u32 unk_6; // 4 bytes
	u32 unk_7; // 4 bytes
	u32 unk_8; // 4 bytes
	u32 unk_9; // 4 bytes
	u16 unk_10; // 2 bytes
	u32 unk_11; // 4 bytes
	u32 unk_12; // 4 bytes
	u16 unk_13; // 2 bytes
	// logger 0x986113
};
POP_PACKED
ASSERT_SIZE(SA_PveRanking, 60);

PUSH_PACKED
struct SA_MyPveRanking
{
	enum { NET_ID = 62476 };
	u32 stageIndex; // 4 bytes
	// highScore: variable-size (string/vector)
	u32 highScoreClearRankType; // 4 bytes
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	u16 unk_3; // 2 bytes
	u32 unk_4; // 4 bytes
	u32 unk_5; // 4 bytes
	u16 unk_6; // 2 bytes
	u8 unk_7; // 1 bytes
	// logger 0x984b97
};
POP_PACKED
ASSERT_SIZE(SA_MyPveRanking, 33);

PUSH_PACKED
struct SN_PveLastgameRanking
{
	enum { NET_ID = 62477 };
	u32 stageId; // 4 bytes
	u8 packetNum; // 1 bytes
	u32 ranking; // 4 bytes
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	u16 unk_3; // 2 bytes
	u32 unk_4; // 4 bytes
	u32 unk_5; // 4 bytes
	u16 unk_6; // 2 bytes
	u32 unk_7; // 4 bytes
	// logger 0x99f7ea
};
POP_PACKED
ASSERT_SIZE(SN_PveLastgameRanking, 37);

PUSH_PACKED
struct SN_DefenceLastgameRanking
{
	enum { NET_ID = 62478 };
	u32 stageId; // 4 bytes
	u8 packetNum; // 1 bytes
	u32 ranking; // 4 bytes
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u16 unk_2; // 2 bytes
	u32 unk_3; // 4 bytes
	u32 unk_4; // 4 bytes
	u16 unk_5; // 2 bytes
	u32 unk_6; // 4 bytes
	// logger 0x990cfb
};
POP_PACKED
ASSERT_SIZE(SN_DefenceLastgameRanking, 33);

PUSH_PACKED
struct SN_UNKNOWN_62479
{
	enum { NET_ID = 62479 };
	u32 field_0; // 4 bytes
	u32 field_1; // 4 bytes
	u8 field_2; // 1 bytes
	u32 field_3; // 4 bytes
	u32 field_4; // 4 bytes
	u32 field_5; // 4 bytes
	u16 field_6; // 2 bytes
	u32 field_7; // 4 bytes
	u32 field_8; // 4 bytes
	u16 field_9; // 2 bytes
	u32 field_10; // 4 bytes
	u32 field_11; // 4 bytes
	u32 field_12; // 4 bytes
	u16 field_13; // 2 bytes
	u32 field_14; // 4 bytes
	u32 field_15; // 4 bytes
	u16 field_16; // 2 bytes
	// logger none
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62479, 57);

PUSH_PACKED
struct SN_ActiveIngameEventList
{
	enum { NET_ID = 62480 };
	u32 field_0; // 4 bytes
	// logger 0x98b64c
};
POP_PACKED
ASSERT_SIZE(SN_ActiveIngameEventList, 4);

PUSH_PACKED
struct SN_PveTodayStageInfo
{
	enum { NET_ID = 62483 };
	u16 availablePlayCount; // 2 bytes
	u16 maxPlayCount; // 2 bytes
	// logger 0x99fa5f
};
POP_PACKED
ASSERT_SIZE(SN_PveTodayStageInfo, 4);

PUSH_PACKED
struct SN_FatiguePointInfo
{
	enum { NET_ID = 62484 };
	u16 todayFatiguePoint; // 2 bytes
	u16 maxFatiguePoint; // 2 bytes
	// logger 0x992d16
};
POP_PACKED
ASSERT_SIZE(SN_FatiguePointInfo, 4);

PUSH_PACKED
struct SN_PlaytimePause
{
	enum { NET_ID = 62486 };
	u32 playTimeMS; // 4 bytes
	// logger 0x99e88c
};
POP_PACKED
ASSERT_SIZE(SN_PlaytimePause, 4);

PUSH_PACKED
struct SN_PlaytimeResume
{
	enum { NET_ID = 62487 };
	// no fixed fields (payloadless or unresolved)
	// logger 0x99e935
};
POP_PACKED

PUSH_PACKED
struct SN_PvpEventAnnouncement
{
	enum { NET_ID = 62488 };
	// stAnnounceMent: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	u32 unk_4; // 4 bytes
	u8 unk_5; // 1 bytes
	// logger 0x99fcd2
};
POP_PACKED
ASSERT_SIZE(SN_PvpEventAnnouncement, 21);

PUSH_PACKED
struct SA_ChatChannelInvite
{
	enum { NET_ID = 62489 };
	u32 retval; // 4 bytes
	// inviteeNickname: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x97e42d
};
POP_PACKED

PUSH_PACKED
struct SN_ChatChannelInvite
{
	enum { NET_ID = 62490 };
	// inviterName: variable-size (string/vector)
	// channelID: variable-size (string/vector)
	u16 channelCenter; // 2 bytes
	u16 channelType; // 2 bytes
	// channelName: variable-size (string/vector)
	// channelPassword: variable-size (string/vector)
	u32 bInvite; // 4 bytes
	u32 unk_0; // 4 bytes
	u16 unk_1; // 2 bytes
	u16 unk_2; // 2 bytes
	u8 unk_3; // 1 bytes
	// NOTE: 4 variable-size field(s) after the fixed prefix
	// logger 0x98f5af
};
POP_PACKED

PUSH_PACKED
struct SA_ChatChannelInviteFeedback
{
	enum { NET_ID = 62491 };
	u32 retval; // 4 bytes
	// logger 0x97e4ff
};
POP_PACKED
ASSERT_SIZE(SA_ChatChannelInviteFeedback, 4);

PUSH_PACKED
struct SN_ChatChannelInviteFeedback
{
	enum { NET_ID = 62492 };
	u32 errorCode; // 4 bytes
	// inviteeNickname: variable-size (string/vector)
	// channelName: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	u16 unk_1; // 2 bytes
	// NOTE: 2 variable-size field(s) after the fixed prefix
	// logger 0x98f747
};
POP_PACKED

PUSH_PACKED
struct SN_InteractionStatus
{
	enum { NET_ID = 62493 };
	u32 state; // 4 bytes
	u32 objectID; // 4 bytes
	u32 interactionID; // 4 bytes
	u32 occupiedTeam; // 4 bytes
	f32 coolTime; // 4 bytes
	// logger 0x998a0d
};
POP_PACKED
ASSERT_SIZE(SN_InteractionStatus, 20);

PUSH_PACKED
struct SN_InteractionCancel
{
	enum { NET_ID = 62494 };
	u32 objectID; // 4 bytes
	u32 targetID; // 4 bytes
	// logger 0x998725
};
POP_PACKED
ASSERT_SIZE(SN_InteractionCancel, 8);

PUSH_PACKED
struct SN_InteractionCasting
{
	enum { NET_ID = 62495 };
	u32 objectID; // 4 bytes
	u32 targetID; // 4 bytes
	u32 actionState; // 4 bytes
	u32 castingTimeMS; // 4 bytes
	// logger 0x9987f6
};
POP_PACKED
ASSERT_SIZE(SN_InteractionCasting, 16);

PUSH_PACKED
struct SN_InteractionExecute
{
	enum { NET_ID = 62496 };
	u32 objectID; // 4 bytes
	u32 targetID; // 4 bytes
	u32 actionState; // 4 bytes
	// logger 0x998915
};
POP_PACKED
ASSERT_SIZE(SN_InteractionExecute, 12);

PUSH_PACKED
struct SQ_TeleportObject
{
	enum { NET_ID = 62497 };
	u32 objectID; // 4 bytes
	// pos: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	// logger 0x9a9180
};
POP_PACKED
ASSERT_SIZE(SQ_TeleportObject, 16);

PUSH_PACKED
struct SN_PlayerSyncTeleport
{
	enum { NET_ID = 62498 };
	u32 objectID; // 4 bytes
	// pos: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	// logger 0x99e57b
};
POP_PACKED
ASSERT_SIZE(SN_PlayerSyncTeleport, 16);

PUSH_PACKED
struct SN_ChangeBattleState
{
	enum { NET_ID = 62499 };
	u32 objectID; // 4 bytes
	u8 isInBattle; // 1 bytes
	f32 baseMoveSpeed; // 4 bytes
	// logger 0x98ec68
};
POP_PACKED
ASSERT_SIZE(SN_ChangeBattleState, 9);

PUSH_PACKED
struct SA_RefreshWaitingQueue
{
	enum { NET_ID = 62502 };
	u32 waitingNumber; // 4 bytes
	u32 estimatedSec; // 4 bytes
	u16 channelCount; // 2 bytes
	// logger 0x986b68
};
POP_PACKED
ASSERT_SIZE(SA_RefreshWaitingQueue, 10);

PUSH_PACKED
struct SN_GamePartyBroken
{
	enum { NET_ID = 62503 };
	// no fixed fields (payloadless or unresolved)
	// logger 0x9950bd
};
POP_PACKED

PUSH_PACKED
struct SN_ScoreUpdate
{
	enum { NET_ID = 62504 };
	u32 usn; // 4 bytes
	u32 teamType; // 4 bytes
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	// logger 0x9a3d3a
};
POP_PACKED
ASSERT_SIZE(SN_ScoreUpdate, 16);

PUSH_PACKED
struct SN_AIMonsterSyncMove
{
	enum { NET_ID = 62505 };
	u8 field_0; // 1 bytes
	u32 field_1; // 4 bytes
	u16 field_2; // 2 bytes
	u16 field_3; // 2 bytes
	u16 field_4; // 2 bytes
	u16 field_5; // 2 bytes
	u16 field_6; // 2 bytes
	u32 field_7; // 4 bytes
	u32 field_8; // 4 bytes
	u32 field_9; // 4 bytes
	u32 field_10; // 4 bytes
	u32 field_11; // 4 bytes
	u32 field_12; // 4 bytes
	u8 field_13; // 1 bytes
	// logger none
};
POP_PACKED
ASSERT_SIZE(SN_AIMonsterSyncMove, 40);

PUSH_PACKED
struct SN_AIMonsterSyncTurn
{
	enum { NET_ID = 62506 };
	u8 field_0; // 1 bytes
	u32 field_1; // 4 bytes
	u16 field_2; // 2 bytes
	u16 field_3; // 2 bytes
	u16 field_4; // 2 bytes
	u16 field_5; // 2 bytes
	// logger none
};
POP_PACKED
ASSERT_SIZE(SN_AIMonsterSyncTurn, 13);

PUSH_PACKED
struct SA_ItemTrade
{
	enum { NET_ID = 62507 };
	u32 result; // 4 bytes
	i64 goldAmount; // 8 bytes
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	// logger 0x983d19
};
POP_PACKED
ASSERT_SIZE(SA_ItemTrade, 24);

PUSH_PACKED
struct SA_ItemCraft
{
	enum { NET_ID = 62508 };
	u8 craftingItemType; // 1 bytes
	u32 result; // 4 bytes
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	// logger 0x983715
};
POP_PACKED
ASSERT_SIZE(SA_ItemCraft, 17);

PUSH_PACKED
struct SA_ItemDisassemble
{
	enum { NET_ID = 62509 };
	u8 craftingItemType; // 1 bytes
	u32 result; // 4 bytes
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	// logger 0x983960
};
POP_PACKED
ASSERT_SIZE(SA_ItemDisassemble, 17);

PUSH_PACKED
struct SA_OlympicBestRecord
{
	enum { NET_ID = 62510 };
	u32 personalRecord; // 4 bytes
	u32 worldRecord; // 4 bytes
	// worldRecordNickname: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x9850a4
};
POP_PACKED

PUSH_PACKED
struct SA_DefenceBestRecord
{
	enum { NET_ID = 62511 };
	u32 personalClearStepIndex; // 4 bytes
	u32 personalClearTime; // 4 bytes
	u32 worldClearStepIndex; // 4 bytes
	u32 worldClearTime; // 4 bytes
	// logger 0x97ee41
};
POP_PACKED
ASSERT_SIZE(SA_DefenceBestRecord, 16);

PUSH_PACKED
struct SN_OlympicIngameRecords
{
	enum { NET_ID = 62512 };
	u32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	u32 field_2; // 4 bytes
	u8 field_3; // 1 bytes
	u8 field_4; // 1 bytes
	// logger 0x99c9d8
};
POP_PACKED
ASSERT_SIZE(SN_OlympicIngameRecords, 11);

PUSH_PACKED
struct SA_PveRefillBattery
{
	enum { NET_ID = 62513 };
	u32 retval; // 4 bytes
	// logger 0x986323
};
POP_PACKED
ASSERT_SIZE(SA_PveRefillBattery, 4);

PUSH_PACKED
struct SN_UNKNOWN_62514
{
	enum { NET_ID = 62514 };
	u32 field_0; // 4 bytes
	// logger none
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62514, 4);

PUSH_PACKED
struct SA_SelectBadge
{
	enum { NET_ID = 62515 };
	u8 badgeType; // 1 bytes
	// logger 0x9882c4
};
POP_PACKED
ASSERT_SIZE(SA_SelectBadge, 1);

PUSH_PACKED
struct SN_ResistStatus
{
	enum { NET_ID = 62516 };
	u32 target; // 4 bytes
	u32 caster; // 4 bytes
	u32 event; // 4 bytes
	// logger 0x9a252c
};
POP_PACKED
ASSERT_SIZE(SN_ResistStatus, 12);

PUSH_PACKED
struct SA_TransformCharacter
{
	enum { NET_ID = 62517 };
	u32 result; // 4 bytes
	u32 characterID; // 4 bytes
	u32 docIndex; // 4 bytes
	u32 coolTime; // 4 bytes
	// logger 0x989c45
};
POP_PACKED
ASSERT_SIZE(SA_TransformCharacter, 16);

PUSH_PACKED
struct SN_TransformCharacter
{
	enum { NET_ID = 62518 };
	u32 characterID; // 4 bytes
	u32 docIndex; // 4 bytes
	u32 durationTime; // 4 bytes
	u32 weaponIndex; // 4 bytes
	u32 SkillSlot1; // 4 bytes
	u32 SkillSlot2; // 4 bytes
	u32 ugSkill; // 4 bytes
	// initStat: variable-size (string/vector)
	u16 baseMoveSpeed; // 2 bytes
	u8 action; // 1 bytes
	u32 unk_0; // 4 bytes
	u16 unk_1; // 2 bytes
	u8 unk_2; // 1 bytes
	u32 unk_3; // 4 bytes
	u32 unk_4; // 4 bytes
	u8 unk_5; // 1 bytes
	// logger 0x9a751e
};
POP_PACKED
ASSERT_SIZE(SN_TransformCharacter, 47);

PUSH_PACKED
struct SN_ChangeCharacterMode
{
	enum { NET_ID = 62519 };
	u32 characterID; // 4 bytes
	u32 docIndex; // 4 bytes
	u32 weaponIndex; // 4 bytes
	u32 skillSlot1; // 4 bytes
	u32 skillSlot2; // 4 bytes
	u32 ugSkill; // 4 bytes
	// initStat: variable-size (string/vector)
	u16 baseMoveSpeed; // 2 bytes
	u8 action; // 1 bytes
	u32 unk_0; // 4 bytes
	u16 unk_1; // 2 bytes
	u8 unk_2; // 1 bytes
	u32 unk_3; // 4 bytes
	u32 unk_4; // 4 bytes
	u8 unk_5; // 1 bytes
	// logger 0x98ed6a
};
POP_PACKED
ASSERT_SIZE(SN_ChangeCharacterMode, 43);

PUSH_PACKED
struct SN_ItemOptiongroupList
{
	enum { NET_ID = 62520 };
	u32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	u8 field_2; // 1 bytes
	u32 field_3; // 4 bytes
	u8 field_4; // 1 bytes
	u32 field_5; // 4 bytes
	u32 field_6; // 4 bytes
	// logger 0x998e02
};
POP_PACKED
ASSERT_SIZE(SN_ItemOptiongroupList, 20);

PUSH_PACKED
struct SA_Honor
{
	enum { NET_ID = 62521 };
	u32 result; // 4 bytes
	// logger 0x9833aa
};
POP_PACKED
ASSERT_SIZE(SA_Honor, 4);

PUSH_PACKED
struct SA_Blame
{
	enum { NET_ID = 62522 };
	u32 result; // 4 bytes
	// logger 0x97d71f
};
POP_PACKED
ASSERT_SIZE(SA_Blame, 4);

PUSH_PACKED
struct SN_DynamicSyncAniTrack
{
	enum { NET_ID = 62523 };
	u32 oid; // 4 bytes
	u32 aniTrackType; // 4 bytes
	u32 pathIndex; // 4 bytes
	u8 isPlaying; // 1 bytes
	f32 aniMoveSpeedTimeSec; // 4 bytes
	f32 progress; // 4 bytes
	// pos: variable-size (string/vector)
	u32 curLoopCount; // 4 bytes
	u32 maxLoopCount; // 4 bytes
	u32 serverTime; // 4 bytes
	u32 startTime; // 4 bytes
	u32 unk_0; // 4 bytes
	i64 unk_1; // 8 bytes
	i64 unk_2; // 8 bytes
	// logger 0x9917b0
};
POP_PACKED
ASSERT_SIZE(SN_DynamicSyncAniTrack, 57);

PUSH_PACKED
struct SN_DisconnectReason
{
	enum { NET_ID = 62524 };
	u32 reason; // 4 bytes
	// logger 0x99129b
};
POP_PACKED
ASSERT_SIZE(SN_DisconnectReason, 4);

PUSH_PACKED
struct SN_AddictionWarning
{
	enum { NET_ID = 62526 };
	u8 addictionLevel; // 1 bytes
	// logger 0x98b757
};
POP_PACKED
ASSERT_SIZE(SN_AddictionWarning, 1);

PUSH_PACKED
struct SA_WatchGame
{
	enum { NET_ID = 62527 };
	u32 errcode; // 4 bytes
	// logger 0x98a4ce
};
POP_PACKED
ASSERT_SIZE(SA_WatchGame, 4);

PUSH_PACKED
struct SN_UserReturnToCity
{
	enum { NET_ID = 62528 };
	// nickname: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x9a8188
};
POP_PACKED

PUSH_PACKED
struct SN_NcguardMsg
{
	enum { NET_ID = 62529 };
	u8 field_0; // 1 bytes
	// logger 0x99bf4e
};
POP_PACKED
ASSERT_SIZE(SN_NcguardMsg, 1);

PUSH_PACKED
struct SN_TenprotectPunishment
{
	enum { NET_ID = 62530 };
	u8 punishMode; // 1 bytes
	// punishReason: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x9a6f50
};
POP_PACKED

PUSH_PACKED
struct SN_UNKNOWN_62531
{
	enum { NET_ID = 62531 };
	u16 field_0; // 2 bytes
	u16 field_1; // 2 bytes
	u16 field_2; // 2 bytes
	// logger none
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62531, 6);

PUSH_PACKED
struct SN_LightOnArea
{
	enum { NET_ID = 62532 };
	u32 cellArrayIndex; // 4 bytes
	u8 team; // 1 bytes
	u8 radius; // 1 bytes
	// logger 0x999872
};
POP_PACKED
ASSERT_SIZE(SN_LightOnArea, 6);

PUSH_PACKED
struct SN_LightOffArea
{
	enum { NET_ID = 62533 };
	u32 cellArrayIndex; // 4 bytes
	u8 team; // 1 bytes
	u8 radius; // 1 bytes
	// logger 0x999776
};
POP_PACKED
ASSERT_SIZE(SN_LightOffArea, 6);

PUSH_PACKED
struct SA_RequestVoting
{
	enum { NET_ID = 62534 };
	u32 usn; // 4 bytes
	u32 votingType; // 4 bytes
	u32 errorType; // 4 bytes
	u32 coolTime; // 4 bytes
	// logger 0x98709a
};
POP_PACKED
ASSERT_SIZE(SA_RequestVoting, 16);

PUSH_PACKED
struct SN_RequestVoting
{
	enum { NET_ID = 62535 };
	u32 requestorUsn; // 4 bytes
	u32 votingType; // 4 bytes
	u32 period; // 4 bytes
	u32 voterCount; // 4 bytes
	u8 isModify; // 1 bytes
	// logger 0x9a23e4
};
POP_PACKED
ASSERT_SIZE(SN_RequestVoting, 17);

PUSH_PACKED
struct SN_UNKNOWN_62536
{
	enum { NET_ID = 62536 };
	u32 field_0; // 4 bytes
	// logger none
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62536, 4);

PUSH_PACKED
struct SA_Vote
{
	enum { NET_ID = 62537 };
	u32 usn; // 4 bytes
	u32 votingType; // 4 bytes
	u32 errorType; // 4 bytes
	// logger 0x98a116
};
POP_PACKED
ASSERT_SIZE(SA_Vote, 12);

PUSH_PACKED
struct SN_Vote
{
	enum { NET_ID = 62538 };
	u32 votingType; // 4 bytes
	u32 voteType; // 4 bytes
	// logger 0x9a8230
};
POP_PACKED
ASSERT_SIZE(SN_Vote, 8);

PUSH_PACKED
struct SN_VotingResult
{
	enum { NET_ID = 62539 };
	u32 candidateUsn; // 4 bytes
	u32 votingType; // 4 bytes
	u32 votingStateType; // 4 bytes
	u32 votingResultReasonType; // 4 bytes
	u32 playerCoolTime; // 4 bytes
	u32 teamCoolTime; // 4 bytes
	// logger 0x9a8301
};
POP_PACKED
ASSERT_SIZE(SN_VotingResult, 24);

PUSH_PACKED
struct SN_PenaltyBadMouther
{
	enum { NET_ID = 62540 };
	u32 remainPenaltyTimeMS; // 4 bytes
	// logger 0x99d786
};
POP_PACKED
ASSERT_SIZE(SN_PenaltyBadMouther, 4);

PUSH_PACKED
struct SN_PvpAiModeEventAnnouncement
{
	enum { NET_ID = 62541 };
	// stAnnounceMent: variable-size (string/vector)
	u32 unk_0; // 4 bytes
	u32 unk_1; // 4 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	u32 unk_4; // 4 bytes
	u8 unk_5; // 1 bytes
	// logger 0x99fb34
};
POP_PACKED
ASSERT_SIZE(SN_PvpAiModeEventAnnouncement, 21);

PUSH_PACKED
struct SN_DateChanged
{
	enum { NET_ID = 62542 };
	// no fixed fields (payloadless or unresolved)
	// logger 0x99069e
};
POP_PACKED

PUSH_PACKED
struct SN_CalendarUpdated
{
	enum { NET_ID = 62543 };
	// no fixed fields (payloadless or unresolved)
	// logger 0x98e599
};
POP_PACKED

PUSH_PACKED
struct SN_Calendar
{
	enum { NET_ID = 62544 };
	i64 field_0; // 8 bytes
	u16 field_1; // 2 bytes
	u8 field_2; // 1 bytes
	u32 field_3; // 4 bytes
	i64 field_4; // 8 bytes
	i64 field_5; // 8 bytes
	// logger 0x98e453
};
POP_PACKED
ASSERT_SIZE(SN_Calendar, 31);

PUSH_PACKED
struct SN_StageSkillAvailable
{
	enum { NET_ID = 62546 };
	u32 currentSkillIndex; // 4 bytes
	u32 unk_0; // 4 bytes
	// logger 0x9a5e99
};
POP_PACKED
ASSERT_SIZE(SN_StageSkillAvailable, 8);

PUSH_PACKED
struct SA_StageSkillSelect
{
	enum { NET_ID = 62547 };
	u32 errcode; // 4 bytes
	u32 currentSkillIndex; // 4 bytes
	// logger 0x989397
};
POP_PACKED
ASSERT_SIZE(SA_StageSkillSelect, 8);

PUSH_PACKED
struct SN_StageSkillSelect
{
	enum { NET_ID = 62548 };
	u32 userId; // 4 bytes
	u32 skillIndex; // 4 bytes
	// logger 0x9a5fce
};
POP_PACKED
ASSERT_SIZE(SN_StageSkillSelect, 8);

PUSH_PACKED
struct SN_SinglemodeSelectCharacter
{
	enum { NET_ID = 62549 };
	// no fixed fields (payloadless or unresolved)
	// logger 0x9a4185
};
POP_PACKED

PUSH_PACKED
struct SN_AvailableAccountEquipmentList
{
	enum { NET_ID = 62550 };
	u8 field_0; // 1 bytes
	u32 field_1; // 4 bytes
	u16 field_2; // 2 bytes
	// logger 0x98d6b5
};
POP_PACKED
ASSERT_SIZE(SN_AvailableAccountEquipmentList, 7);

PUSH_PACKED
struct SN_ItemLifetimeExpired
{
	enum { NET_ID = 62551 };
	u32 field_0; // 4 bytes
	// logger 0x998cf7
};
POP_PACKED
ASSERT_SIZE(SN_ItemLifetimeExpired, 4);

PUSH_PACKED
struct SN_EffectLifetimeExpired
{
	enum { NET_ID = 62552 };
	u32 field_0; // 4 bytes
	// logger 0x991a1d
};
POP_PACKED
ASSERT_SIZE(SN_EffectLifetimeExpired, 4);

PUSH_PACKED
struct SN_Gauge
{
	enum { NET_ID = 62553 };
	u32 userId; // 4 bytes
	u8 updateType; // 1 bytes
	u8 flag; // 1 bytes
	f32 overheatTime; // 4 bytes
	u16 gauge; // 2 bytes
	u16 maxGauge; // 2 bytes
	// logger 0x995a54
};
POP_PACKED
ASSERT_SIZE(SN_Gauge, 14);

PUSH_PACKED
struct SN_EffectLifetimeList
{
	enum { NET_ID = 62554 };
	u32 field_0; // 4 bytes
	i64 field_1; // 8 bytes
	// logger 0x991b28
};
POP_PACKED
ASSERT_SIZE(SN_EffectLifetimeList, 12);

PUSH_PACKED
struct SN_MonsterRespawnTime
{
	enum { NET_ID = 62555 };
	u32 stringKey; // 4 bytes
	u32 localId; // 4 bytes
	u32 remainTime; // 4 bytes
	// logger 0x99b9db
};
POP_PACKED
ASSERT_SIZE(SN_MonsterRespawnTime, 12);

PUSH_PACKED
struct SA_GuideMissionAccept
{
	enum { NET_ID = 62556 };
	u32 result; // 4 bytes
	u8 step; // 1 bytes
	// logger 0x983131
};
POP_PACKED
ASSERT_SIZE(SA_GuideMissionAccept, 5);

PUSH_PACKED
struct SA_GuideMissionReward
{
	enum { NET_ID = 62557 };
	u32 result; // 4 bytes
	u8 step; // 1 bytes
	// logger 0x9832d7
};
POP_PACKED
ASSERT_SIZE(SA_GuideMissionReward, 5);

PUSH_PACKED
struct SA_GuideMissionQuit
{
	enum { NET_ID = 62558 };
	u32 result; // 4 bytes
	u8 step; // 1 bytes
	// logger 0x983204
};
POP_PACKED
ASSERT_SIZE(SA_GuideMissionQuit, 5);

PUSH_PACKED
struct SN_CurrentGuideMission
{
	enum { NET_ID = 62559 };
	u8 step; // 1 bytes
	u8 state; // 1 bytes
	u8 isRewarded; // 1 bytes
	// logger 0x9904cc
};
POP_PACKED
ASSERT_SIZE(SN_CurrentGuideMission, 3);

PUSH_PACKED
struct SN_MonsterMineralLossTiming
{
	enum { NET_ID = 62560 };
	u32 tagID; // 4 bytes
	u32 unk_0; // 4 bytes
	// logger 0x99b8a5
};
POP_PACKED
ASSERT_SIZE(SN_MonsterMineralLossTiming, 8);

PUSH_PACKED
struct SN_PveTimeOverWarning
{
	enum { NET_ID = 62561 };
	i64 remainTime; // 8 bytes
	// logger 0x99f9b3
};
POP_PACKED
ASSERT_SIZE(SN_PveTimeOverWarning, 8);

PUSH_PACKED
struct SA_ActivityReward
{
	enum { NET_ID = 62562 };
	u32 result; // 4 bytes
	u8 activityRewardedState; // 1 bytes
	// logger 0x97d4fa
};
POP_PACKED
ASSERT_SIZE(SA_ActivityReward, 5);

PUSH_PACKED
struct SA_NpBuyCshopItem
{
	enum { NET_ID = 62563 };
	u32 result; // 4 bytes
	u32 goodsId; // 4 bytes
	u32 quantity; // 4 bytes
	// logger 0x984e5a
};
POP_PACKED
ASSERT_SIZE(SA_NpBuyCshopItem, 12);

PUSH_PACKED
struct SN_ExpIngame
{
	enum { NET_ID = 62564 };
	u32 usn; // 4 bytes
	u16 acquisitionExp; // 2 bytes
	u16 currentExp; // 2 bytes
	u32 target; // 4 bytes
	// logger 0x9926f1
};
POP_PACKED
ASSERT_SIZE(SN_ExpIngame, 12);

PUSH_PACKED
struct SN_LevelIngame
{
	enum { NET_ID = 62565 };
	u32 usn; // 4 bytes
	u16 expForLevelUp; // 2 bytes
	u8 currentLevel; // 1 bytes
	// logger 0x99967a
};
POP_PACKED
ASSERT_SIZE(SN_LevelIngame, 7);

PUSH_PACKED
struct SN_SpectatorDelay
{
	enum { NET_ID = 62566 };
	u32 delayMs; // 4 bytes
	// logger 0x9a560d
};
POP_PACKED
ASSERT_SIZE(SN_SpectatorDelay, 4);

PUSH_PACKED
struct SN_SpectatorStart
{
	enum { NET_ID = 62567 };
	u32 readyElapsedMS; // 4 bytes
	u32 playtimeMs; // 4 bytes
	// logger 0x9a56b6
};
POP_PACKED
ASSERT_SIZE(SN_SpectatorStart, 8);

PUSH_PACKED
struct SN_SpectatorTrespassReady
{
	enum { NET_ID = 62568 };
	u8 isPaused; // 1 bytes
	u32 readyElapsedMS; // 4 bytes
	// logger 0x9a5787
};
POP_PACKED
ASSERT_SIZE(SN_SpectatorTrespassReady, 5);

PUSH_PACKED
struct SN_PhaseStart
{
	enum { NET_ID = 62569 };
	u32 step; // 4 bytes
	// logger 0x99d927
};
POP_PACKED
ASSERT_SIZE(SN_PhaseStart, 4);

PUSH_PACKED
struct SN_PhaseEnd
{
	enum { NET_ID = 62570 };
	u32 step; // 4 bytes
	u32 endReason; // 4 bytes
	u32 clearTime; // 4 bytes
	// logger 0x99d82f
};
POP_PACKED
ASSERT_SIZE(SN_PhaseEnd, 12);

PUSH_PACKED
struct SA_MasterTrainingEvent
{
	enum { NET_ID = 62571 };
	u8 eventType; // 1 bytes
	u32 ret; // 4 bytes
	u32 entityID; // 4 bytes
	u32 param1; // 4 bytes
	u32 param2; // 4 bytes
	// logger 0x9848d5
};
POP_PACKED
ASSERT_SIZE(SA_MasterTrainingEvent, 17);

PUSH_PACKED
struct SN_GmDisabledMasters
{
	enum { NET_ID = 62572 };
	u32 field_0; // 4 bytes
	// logger 0x995f3c
};
POP_PACKED
ASSERT_SIZE(SN_GmDisabledMasters, 4);

PUSH_PACKED
struct SN_GmDisabledSkins
{
	enum { NET_ID = 62573 };
	u16 field_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x996047
};
POP_PACKED

PUSH_PACKED
struct SN_GmDisabledStages
{
	enum { NET_ID = 62574 };
	u32 field_0; // 4 bytes
	u8 field_1; // 1 bytes
	// logger 0x996160
};
POP_PACKED
ASSERT_SIZE(SN_GmDisabledStages, 5);

PUSH_PACKED
struct SN_CharacterPropertyPoint
{
	enum { NET_ID = 62578 };
	u32 characterId; // 4 bytes
	u8 propertyPoint; // 1 bytes
	// logger 0x98f314
};
POP_PACKED
ASSERT_SIZE(SN_CharacterPropertyPoint, 5);

PUSH_PACKED
struct SN_UNKNOWN_62579
{
	enum { NET_ID = 62579 };
	u32 field_0; // 4 bytes
	// logger none
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62579, 4);

PUSH_PACKED
struct SN_UNKNOWN_62580
{
	enum { NET_ID = 62580 };
	u32 field_0; // 4 bytes
	// logger none
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62580, 4);

PUSH_PACKED
struct SA_CharacterPropertyUpgrade
{
	enum { NET_ID = 62581 };
	u32 errorType; // 4 bytes
	// logger 0x97e1bb
};
POP_PACKED
ASSERT_SIZE(SA_CharacterPropertyUpgrade, 4);

PUSH_PACKED
struct SA_CharacterPropertyReset
{
	enum { NET_ID = 62582 };
	u32 errorType; // 4 bytes
	// logger 0x97e112
};
POP_PACKED
ASSERT_SIZE(SA_CharacterPropertyReset, 4);

PUSH_PACKED
struct SN_CharacterPropertyReset
{
	enum { NET_ID = 62583 };
	u32 characterId; // 4 bytes
	// logger 0x98f3e7
};
POP_PACKED
ASSERT_SIZE(SN_CharacterPropertyReset, 4);

PUSH_PACKED
struct SN_CharacterPropertyInfo
{
	enum { NET_ID = 62584 };
	u32 characterId; // 4 bytes
	u8 unk_0; // 1 bytes
	u8 unk_1; // 1 bytes
	// logger 0x98f1bb
};
POP_PACKED
ASSERT_SIZE(SN_CharacterPropertyInfo, 6);

PUSH_PACKED
struct SN_GameRecordInfoList
{
	enum { NET_ID = 62585 };
	u32 field_0; // 4 bytes
	u16 field_1; // 2 bytes
	u8 field_2; // 1 bytes
	// logger 0x9957f0
};
POP_PACKED
ASSERT_SIZE(SN_GameRecordInfoList, 7);

PUSH_PACKED
struct SA_ChatGetInviteeGameaccountid
{
	enum { NET_ID = 62586 };
	u32 result; // 4 bytes
	u32 inviteeCenterId; // 4 bytes
	// inviteeNickname: variable-size (string/vector)
	// inviteeGameAccountId: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	u16 unk_1; // 2 bytes
	// NOTE: 2 variable-size field(s) after the fixed prefix
	// logger 0x97e5a8
};
POP_PACKED

PUSH_PACKED
struct SA_UseCoupon
{
	enum { NET_ID = 62587 };
	u32 result; // 4 bytes
	// couponKey: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x98a008
};
POP_PACKED

PUSH_PACKED
struct SN_NotifyChattingChannelStatus
{
	enum { NET_ID = 62588 };
	// channelId: variable-size (string/vector)
	// channelCenter: variable-size (string/vector)
	// password: variable-size (string/vector)
	// token: variable-size (string/vector)
	// argument: variable-size (string/vector)
	u16 reason; // 2 bytes
	u16 unk_0; // 2 bytes
	u16 unk_1; // 2 bytes
	u16 unk_2; // 2 bytes
	u16 unk_3; // 2 bytes
	u32 unk_4; // 4 bytes
	// NOTE: 5 variable-size field(s) after the fixed prefix
	// logger 0x99c178
};
POP_PACKED

PUSH_PACKED
struct SN_DefenceModeIngameInfo
{
	enum { NET_ID = 62589 };
	u32 currentStep; // 4 bytes
	u32 totalClearTime; // 4 bytes
	// logger 0x990ec4
};
POP_PACKED
ASSERT_SIZE(SN_DefenceModeIngameInfo, 8);

PUSH_PACKED
struct SA_RequestToken
{
	enum { NET_ID = 62590 };
	// authnToken: variable-size (string/vector)
	u16 unk_0; // 2 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x986fb9
};
POP_PACKED

PUSH_PACKED
struct SN_HudEventList
{
	enum { NET_ID = 62591 };
	u8 field_0; // 1 bytes
	u16 field_1; // 2 bytes
	u16 field_2; // 2 bytes
	// logger 0x998167
};
POP_PACKED
ASSERT_SIZE(SN_HudEventList, 5);

PUSH_PACKED
struct SN_CshopEventList
{
	enum { NET_ID = 62592 };
	u8 field_0; // 1 bytes
	u8 field_1; // 1 bytes
	u32 field_2; // 4 bytes
	u16 field_3; // 2 bytes
	// logger 0x99038e
};
POP_PACKED
ASSERT_SIZE(SN_CshopEventList, 8);

PUSH_PACKED
struct SA_UserReport
{
	enum { NET_ID = 62593 };
	u32 result; // 4 bytes
	// logger 0x989f5f
};
POP_PACKED
ASSERT_SIZE(SA_UserReport, 4);

PUSH_PACKED
struct SN_UNKNOWN_62594
{
	enum { NET_ID = 62594 };
	u8 field_0; // 1 bytes
	u8 field_1; // 1 bytes
	// logger none
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62594, 2);

PUSH_PACKED
struct SN_ServerUtcTime
{
	enum { NET_ID = 62595 };
	i64 serverUTCTime; // 8 bytes
	// logger 0x9a3ec2
};
POP_PACKED
ASSERT_SIZE(SN_ServerUtcTime, 8);

PUSH_PACKED
struct SN_NotifyPccafeTagbuff
{
	enum { NET_ID = 62596 };
	// no fixed fields (payloadless or unresolved)
	// logger 0x99c5fb
};
POP_PACKED

PUSH_PACKED
struct SN_PraiseInfo
{
	enum { NET_ID = 62597 };
	u32 durationMs; // 4 bytes
	u32 unk_0; // 4 bytes
	u16 unk_1; // 2 bytes
	u32 unk_2; // 4 bytes
	u32 unk_3; // 4 bytes
	u32 unk_4; // 4 bytes
	u32 unk_5; // 4 bytes
	u32 unk_6; // 4 bytes
	u32 unk_7; // 4 bytes
	u32 unk_8; // 4 bytes
	// logger 0x99ea82
};
POP_PACKED
ASSERT_SIZE(SN_PraiseInfo, 38);

PUSH_PACKED
struct SN_PraiseStart
{
	enum { NET_ID = 62598 };
	u32 remainDurationMs; // 4 bytes
	// logger 0x99ebf6
};
POP_PACKED
ASSERT_SIZE(SN_PraiseStart, 4);

PUSH_PACKED
struct SA_Praise
{
	enum { NET_ID = 62599 };
	u32 errorType; // 4 bytes
	// logger 0x985f72
};
POP_PACKED
ASSERT_SIZE(SA_Praise, 4);

PUSH_PACKED
struct SN_Praise
{
	enum { NET_ID = 62600 };
	// nickName: variable-size (string/vector)
	u16 praiseCount; // 2 bytes
	u32 unk_0; // 4 bytes
	// NOTE: 1 variable-size field(s) after the fixed prefix
	// logger 0x99e9b2
};
POP_PACKED

PUSH_PACKED
struct SA_PauseGame
{
	enum { NET_ID = 62602 };
	u32 errorType; // 4 bytes
	// logger 0x985ec9
};
POP_PACKED
ASSERT_SIZE(SA_PauseGame, 4);

PUSH_PACKED
struct SN_PausedGame
{
	enum { NET_ID = 62603 };
	u32 remainDurationMs; // 4 bytes
	u32 playTimeMS; // 4 bytes
	// logger 0x99d6b5
};
POP_PACKED
ASSERT_SIZE(SN_PausedGame, 8);

PUSH_PACKED
struct SA_ResumeGame
{
	enum { NET_ID = 62604 };
	u32 errorType; // 4 bytes
	// logger 0x98787c
};
POP_PACKED
ASSERT_SIZE(SA_ResumeGame, 4);

PUSH_PACKED
struct SN_ResumeGameDelay
{
	enum { NET_ID = 62605 };
	u32 resumeDelayMs; // 4 bytes
	// logger 0x9a2f73
};
POP_PACKED
ASSERT_SIZE(SN_ResumeGameDelay, 4);

PUSH_PACKED
struct SN_ResumedGame
{
	enum { NET_ID = 62606 };
	// no fixed fields (payloadless or unresolved)
	// logger 0x9a2ef6
};
POP_PACKED

PUSH_PACKED
struct SA_NpReportChat
{
	enum { NET_ID = 62607 };
	u32 result; // 4 bytes
	// logger 0x984f52
};
POP_PACKED
ASSERT_SIZE(SA_NpReportChat, 4);

PUSH_PACKED
struct SA_NpReportMail
{
	enum { NET_ID = 62608 };
	u32 result; // 4 bytes
	// logger 0x984ffb
};
POP_PACKED
ASSERT_SIZE(SA_NpReportMail, 4);

} // Sv

#undef VEC
