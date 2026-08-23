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

// Inventory-stack instance uid assigned by the server; bit30 set (0x40000000 tag).
// Same id space as SN_ItemUpdate.m_itemID; echoed verbatim by client packets (e.g. CQ_ItemUse).
enum class ItemUID: u32
{
	INVALID = 0
};

// Static item content id from game data (m_itemIndex family, e.g. 136020015).
enum class ItemDocIndex: u32
{
	INVALID = 0
};

// Dropped-ground-item entity uid; distinct tag space (bit31+30 set, 0xC0000000 | seq).
enum class GroundItemUID: u32
{
	INVALID = 0
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

struct CN_GamePlayerSyncByInt
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
ASSERT_SIZE(CN_GamePlayerSyncByInt, 56);

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

struct CA_CityLobbyJoinCity
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
	u16 nicknameLen;
	wchar nickname[1]; // length is nicknameLen


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

PUSH_PACKED
struct CQ_UserLoginByNp
{
	enum { NET_ID = 60004 };
	u16 field1_len; // 2 bytes
	char field1[1]; // 1 byte each (ansi string: u16 len + len)
	u16 field2_len; // 2 bytes
	char field2[1]; // 1 byte each (ansi string: u16 len + len)
	u32 field3; // 4 bytes
};
POP_PACKED

PUSH_PACKED
struct CN_StationLatency
{
	enum { NET_ID = 60006 };
	u32 elapsedMs; // 4 bytes
	// vec_u16<{u32,u16}>: variable-size (u16 count + count x u32)
	u16 stationPingList_ip_rtt_count; // 2 bytes
	u32 stationPingList_ip_rtt[1]; // 4 bytes each
	// vec_u16<{u32,u16,u16,u32}>: variable-size (u16 count + count x u32)
	u16 stationList_ip_port_extra_count; // 2 bytes
	u32 stationList_ip_port_extra[1]; // 4 bytes each
};
POP_PACKED

PUSH_PACKED
struct CQ_HandoverAuthRequestToSpectator
{
	enum { NET_ID = 60010 };
	u16 nick_len; // 2 bytes
	wchar_t nick[1]; // 2 bytes each (wide string: u16 len + len*2)
	u32 instantKey; // 4 bytes
};
POP_PACKED

PUSH_PACKED
struct CQ_UNKNOWN_60011
{
	enum { NET_ID = 60011 };
};
POP_PACKED

PUSH_PACKED
struct CQ_ItemMerge
{
	enum { NET_ID = 60012 };
	u32 field1_id_id; // 4 bytes
	u32 field2_id_id; // 4 bytes
	u32 computedCount; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_ItemMerge, 12);

PUSH_PACKED
struct CN_EnterCityChannelForce
{
	enum { NET_ID = 60013 };
};
POP_PACKED

PUSH_PACKED
struct CQ_ItemUse
{
	enum { NET_ID = 60015 };
	ItemUID itemInstanceUID; // 4 bytes: 0x40000000-tagged instance uid, same id space as SN_ItemUpdate.m_itemID; server zeroes/decrements it on use
};
POP_PACKED
ASSERT_SIZE(CQ_ItemUse, 4);

PUSH_PACKED
struct CQ_SellShopItem
{
	enum { NET_ID = 60017 };
	u32 shopRef;
	// u16 count + count x {u32,u32}
	VEC(u32, sellEntries); // 8 bytes per element


};
POP_PACKED

PUSH_PACKED
struct CA_PrepareHandoverToChannel
{
	enum { NET_ID = 60019 };
};
POP_PACKED

PUSH_PACKED
struct CQ_GetGroundItem
{
	enum { NET_ID = 60020 };
	u32 srcEntityId; // 4 bytes
	u32 targetEntityId; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_GetGroundItem, 8);

PUSH_PACKED
struct CQ_ReturnToGame
{
	enum { NET_ID = 60025 };
};
POP_PACKED

PUSH_PACKED
struct CQ_ReturnToCity
{
	enum { NET_ID = 60026 };
};
POP_PACKED

PUSH_PACKED
struct CQ_HandoverToChannel
{
	enum { NET_ID = 60027 };
};
POP_PACKED

PUSH_PACKED
struct CN_CommunityChat
{
	enum { NET_ID = 60028 };
	u32 cmdType_const4; // 4 bytes
	u16 commandText_len; // 2 bytes
	wchar_t commandText[1]; // 2 bytes each (wide string: u16 len + len*2)
	u16 accountName_len; // 2 bytes
	wchar_t accountName[1]; // 2 bytes each (wide string: u16 len + len*2)
};
POP_PACKED

PUSH_PACKED
struct CQ_BuyShopItem
{
	enum { NET_ID = 60029 };
	u32 playerId; // 4 bytes
	u32 itemId; // 4 bytes
	u32 opFlag; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_BuyShopItem, 12);

PUSH_PACKED
struct CQ_BuyCShopItem
{
	enum { NET_ID = 60030 };
	ItemUID itemInstanceUID; // 4 bytes: 0x40000000-tagged instance uid, same id space as SN_ItemUpdate.m_itemID; server zeroes/decrements it on use
	u32 actionParam; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_BuyCShopItem, 8);

PUSH_PACKED
struct CQ_ItemDelete
{
	enum { NET_ID = 60031 };
	u32 itemId; // 4 bytes
	u32 count; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_ItemDelete, 8);

PUSH_PACKED
struct CN_RequestShopItemList
{
	enum { NET_ID = 60036 };
	u32 npcId; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CN_RequestShopItemList, 4);

PUSH_PACKED
struct CQ_ItemChange
{
	enum { NET_ID = 60037 };
	u32 itemIdA; // 4 bytes
	u32 itemIdB; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_ItemChange, 8);

PUSH_PACKED
struct CQ_PreCastSkill
{
	enum { NET_ID = 60039 };
	u32 skillId; // 4 bytes
	u32 castId; // 4 bytes
	u32 skillDataRecord; // 4 bytes (unclassified writer 13xdword)
};
POP_PACKED

PUSH_PACKED
struct CQ_ExecuteSkill
{
	enum { NET_ID = 60041 };
	u32 charID;
	u32 skillID;
	float3 targetPos;
	VEC(u32, targetIDs);
	u32 unkParams[13];


};
POP_PACKED

PUSH_PACKED
struct CQ_SwitchOnToggleSkill
{
	enum { NET_ID = 60042 };
	u16 nick_len; // 2 bytes
	wchar_t nick[1]; // 2 bytes each (wide string: u16 len + len*2)
	u32 instantKey; // 4 bytes
};
POP_PACKED

PUSH_PACKED
struct CQ_SwitchOffToggleSkill
{
	enum { NET_ID = 60043 };
};
POP_PACKED

PUSH_PACKED
struct CQ_PlayerCancelStance
{
	enum { NET_ID = 60044 };
	u32 field1_id_id; // 4 bytes
	u32 field2_id_id; // 4 bytes
	u32 computedCount; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_PlayerCancelStance, 12);

PUSH_PACKED
struct CQ_RequestSummaryInfoEach
{
	enum { NET_ID = 60045 };
};
POP_PACKED

PUSH_PACKED
struct CQ_RequestSummaryReward
{
	enum { NET_ID = 60046 };
};
POP_PACKED

PUSH_PACKED
struct CQ_AntiHackAuth
{
	enum { NET_ID = 60047 };
	VEC(u8, authData); // blob


};
POP_PACKED

PUSH_PACKED
struct CQ_UNKNOWN_60048
{
	VEC(u8, unkBlob); // blob


};
POP_PACKED

PUSH_PACKED
struct CQ_UNKNOWN_60054
{
	enum { NET_ID = 60054 };
	u32 entityId; // 4 bytes
	u32 position; // 4 bytes (unclassified writer u32[3])
	u32 velocity; // 4 bytes (unclassified writer u32[3])
	u32 auxVec3; // 4 bytes (unclassified writer u32[3])
	u32 param8; // 4 bytes
	u32 param9; // 4 bytes
	u32 sentinel_minus1; // 4 bytes
	u32 sentinel_minus1_3; // 4 bytes
};
POP_PACKED

PUSH_PACKED
struct CQ_SkillUpgradeIngame
{
	enum { NET_ID = 60055 };
	u32 unkIngameContext; // 4 bytes: session-constant from game context (FUN_01a37e2b); observed 1 (EU/NA) and 8 (Titan Ruins)
	u32 skillUpgradeId; // 4 bytes: upgraded skill/option id; observed 0, 1 and 12
};
POP_PACKED

ASSERT_SIZE(CQ_SkillUpgradeIngame, 8);

PUSH_PACKED
struct CQ_UNKNOWN_60056
{
	enum { NET_ID = 60056 };
	u32 sendTime; // 4 bytes
	u32 virtualTime; // 4 bytes
	u32 currentGvt; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_UNKNOWN_60056, 12);

PUSH_PACKED
struct CQ_UNKNOWN_60057
{
	enum { NET_ID = 60057 };
};
POP_PACKED

PUSH_PACKED
struct CQ_UNKNOWN_60058
{
	enum { NET_ID = 60058 };
};
POP_PACKED

PUSH_PACKED
struct CQ_GearEquip
{
	enum { NET_ID = 60060 };
	u32 cmdType_const4; // 4 bytes
	u16 commandText_len; // 2 bytes
	wchar_t commandText[1]; // 2 bytes each (wide string: u16 len + len*2)
	u16 accountName_len; // 2 bytes
	wchar_t accountName[1]; // 2 bytes each (wide string: u16 len + len*2)
};
POP_PACKED

PUSH_PACKED
struct CQ_GearUnequip
{
	enum { NET_ID = 60061 };
	u32 playerId; // 4 bytes
	u32 itemId; // 4 bytes
	u32 opFlag; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_GearUnequip, 12);

PUSH_PACKED
struct CQ_GearSwap
{
	enum { NET_ID = 60062 };
	ItemUID itemInstanceUID; // 4 bytes: 0x40000000-tagged instance uid, same id space as SN_ItemUpdate.m_itemID; server zeroes/decrements it on use
	u32 actionParam; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_GearSwap, 8);

PUSH_PACKED
struct CQ_MasterGearRepair
{
	enum { NET_ID = 60065 };
};
POP_PACKED

PUSH_PACKED
struct CQ_MasterGearRename
{
	u8 gearSlot;
	u16 newNameLen;
	wchar newName[1]; // length is newNameLen


};
POP_PACKED

PUSH_PACKED
struct CQ_MasterGearAdd
{
	enum { NET_ID = 60067 };
};
POP_PACKED

PUSH_PACKED
struct CQ_CharacterUnlock
{
	enum { NET_ID = 60068 };
	u32 npcId; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_CharacterUnlock, 4);

PUSH_PACKED
struct CQ_SkillUnlock
{
	enum { NET_ID = 60069 };
	u32 itemIdA; // 4 bytes
	u32 itemIdB; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_SkillUnlock, 8);

PUSH_PACKED
struct CQ_SkillSelect
{
	enum { NET_ID = 60070 };
	u32 skillId; // 4 bytes
	u32 castId; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_SkillSelect, 8);

PUSH_PACKED
struct CQ_PartySwapTeam
{
	enum { NET_ID = 60077 };
	u32 field1_id_id; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_PartySwapTeam, 4);

PUSH_PACKED
struct CQ_PartyInvite
{
	enum { NET_ID = 60078 };
	u32 field1; // 4 bytes
	u32 field2; // 4 bytes
	u32 field3; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_PartyInvite, 12);

PUSH_PACKED
struct CQ_PartyInviteResponse
{
	enum { NET_ID = 60079 };
	u32 byteBlob; // 4 bytes (unclassified writer u16 count + N x u8)
};
POP_PACKED

PUSH_PACKED
struct CQ_PartyInviteRecommendResponse
{
	enum { NET_ID = 60080 };
	u16 targetNameLen;
	wchar targetName[1]; // length is targetNameLen
	u8 bAccept;


};
POP_PACKED

PUSH_PACKED
struct CQ_UNKNOWN_60081
{
	enum { NET_ID = 60081 };
	u32 newNickname; // 4 bytes (unclassified writer wstring)
};
POP_PACKED

PUSH_PACKED
struct CQ_UNKNOWN_60082
{
	enum { NET_ID = 60082 };
	u32 wideStringField; // 4 bytes (unclassified writer wstring)
};
POP_PACKED

PUSH_PACKED
struct CQ_RejectPartyRoom
{
	enum { NET_ID = 60083 };
	u32 characterId; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_RejectPartyRoom, 4);

PUSH_PACKED
struct CQ_JoinPartyAsSpectator
{
	enum { NET_ID = 60084 };
	u32 newLeaderProfileId; // 4 bytes
	u32 secondId; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_JoinPartyAsSpectator, 8);

PUSH_PACKED
struct CQ_LeavePartyRoom
{
	enum { NET_ID = 60085 };
	u32 field1_id_id; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_LeavePartyRoom, 4);

PUSH_PACKED
struct CQ_BreakUpPartyRoom
{
	enum { NET_ID = 60086 };
	ItemUID itemInstanceUID; // 4 bytes: 0x40000000-tagged instance uid, same id space as SN_ItemUpdate.m_itemID; server zeroes/decrements it on use
	u32 slotOrParam; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_BreakUpPartyRoom, 8);

PUSH_PACKED
struct CQ_PartyKickAll
{
	enum { NET_ID = 60087 };
	u32 field1_id_id; // 4 bytes
	u32 field2_state_state; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_PartyKickAll, 8);

PUSH_PACKED
struct CQ_QuickRunArena
{
	enum { NET_ID = 60088 };
	u32 posX; // 4 bytes
	u32 posY; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_QuickRunArena, 8);

PUSH_PACKED
struct CQ_RestartGame
{
	enum { NET_ID = 60091 };
	u32 memberProfileId; // 4 bytes
	u32 secondId; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_RestartGame, 8);

PUSH_PACKED
struct CQ_RestartVote
{
	enum { NET_ID = 60092 };
	u8 flagByte; // 1 bytes (bool)
	u32 itemId; // 4 bytes
	u32 countOrParam; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_RestartVote, 9);

PUSH_PACKED
struct CQ_UNKNOWN_60093
{
	enum { NET_ID = 60093 };
	u8 flagByte; // 1 bytes (bool)
	u32 itemId; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_UNKNOWN_60093, 5);

PUSH_PACKED
struct CQ_RestartVote2
{
	enum { NET_ID = 60094 };
	u8 field1; // 1 bytes (bool)
	u8 field2; // 1 bytes (bool)
	u8 field3; // 1 bytes (bool)
};
POP_PACKED
ASSERT_SIZE(CQ_RestartVote2, 3);

PUSH_PACKED
struct CQ_UNKNOWN_60095
{
	enum { NET_ID = 60095 };
	u32 field1_u32_u32; // 4 bytes
	u32 byteBlob; // 4 bytes (unclassified writer blob(u16 len + bytes))
	u32 u32List; // 4 bytes (unclassified writer array<u32>(u16 count + N x u32))
};
POP_PACKED

PUSH_PACKED
struct CQ_UNKNOWN_60096
{
	enum { NET_ID = 60096 };
	u32 field1_u32_u32; // 4 bytes
	u32 byteBlob; // 4 bytes (unclassified writer blob(u16 len + bytes))
};
POP_PACKED

PUSH_PACKED
struct CQ_CancelMatching
{
	enum { NET_ID = 60098 };
	u8 flagByte; // 1 bytes (bool)
	u32 wideStringField; // 4 bytes (unclassified writer wstring)
};
POP_PACKED

PUSH_PACKED
struct CQ_CancelMatchingTrollPenalty
{
	enum { NET_ID = 60099 };
};
POP_PACKED

PUSH_PACKED
struct CQ_WeaponUpgrade
{
	enum { NET_ID = 60104 };
	u32 memberId; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_WeaponUpgrade, 4);

PUSH_PACKED
struct CQ_InventoryExpand
{
	enum { NET_ID = 60105 };
	u32 field1_id_id; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_InventoryExpand, 4);

PUSH_PACKED
struct CQ_InventoryRearrange
{
	enum { NET_ID = 60106 };
	u32 roomTypeOrStageId; // 4 bytes
	u32 option; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_InventoryRearrange, 8);

PUSH_PACKED
struct CN_UNKNOWN_60107
{
	enum { NET_ID = 60107 };
	u32 partyModifyPayload; // 4 bytes (unclassified writer array<u32>(u16 count + N x u32) + u32 + u32 + u8)
};
POP_PACKED

PUSH_PACKED
struct CN_RemoteActivated
{
	enum { NET_ID = 60108 };
	u32 unkU32_1;
	u32 unkU32_2;
	u32 unkU32_3;
	u32 unkU32_4;
	u32 unkU32_5;
	u32 unkU32_6;
	u32 unkU32_7;
	u32 unkU32_8;
	u32 unkU32_9;
	u32 unkU32_10;
	u16 unkWstrLen;
	wchar unkWstr[1]; // length is unkWstrLen
	u16 unkStrLen;
	char unkStr[1]; // length is unkStrLen
	u32 unkU32_13;
	u32 unkU32_14;
	u32 unkU32_15;
	u32 unkU32_16;
	u32 unkU32_17;
	u32 unkU32_18;
	u32 unkU32_19;
	u32 unkU32_20;
	u32 unkU32_21;
	u32 unkU32_22;
	u32 unkU32_23;
	u32 unkU32_24;
	u32 unkU32_25;
	u32 unkU32_26;
	u32 unkU32_27;
	u32 unkU32_28;
	u32 unkU32_29;
	u32 unkU32_30;
	u32 unkU32_31;


};
POP_PACKED

PUSH_PACKED
struct CN_RemoteHitRemote
{
	enum { NET_ID = 60109 };
	u32 field1; // 4 bytes
	u8 flag1; // 1 bytes (bool)
	u32 field3; // 4 bytes
	u8 flag2; // 1 bytes (bool)
};
POP_PACKED
ASSERT_SIZE(CN_RemoteHitRemote, 10);

PUSH_PACKED
struct CQ_DestroyRemote
{
	enum { NET_ID = 60110 };
	u32 targetName; // 4 bytes (unclassified writer wstring)
	u32 inviteRole; // 4 bytes
};
POP_PACKED

PUSH_PACKED
struct CQ_RequestSpAction
{
	enum { NET_ID = 60111 };
	u32 targetName; // 4 bytes (unclassified writer wstring)
};
POP_PACKED

PUSH_PACKED
struct CN_UNKNOWN_60112
{
	enum { NET_ID = 60112 };
	u32 targetName; // 4 bytes (unclassified writer wstring)
	u8 flag; // 1 bytes (bool)
};
POP_PACKED

PUSH_PACKED
struct CQ_SelectSkin
{
	enum { NET_ID = 60115 };
	u32 roomIdOrInviterId; // 4 bytes
	u8 constZero; // 1 bytes (bool)
	u32 rejectMessage; // 4 bytes (unclassified writer wstring)
};
POP_PACKED

PUSH_PACKED
struct CQ_UNKNOWN_60116
{
	enum { NET_ID = 60116 };
	u32 roomOrHostId; // 4 bytes
	u32 joinRole; // 4 bytes
	u32 password; // 4 bytes (unclassified writer wstring)
};
POP_PACKED

PUSH_PACKED
struct CQ_FriendStateChange
{
	enum { NET_ID = 60117 };
};
POP_PACKED

PUSH_PACKED
struct CQ_UNKNOWN_60118
{
	enum { NET_ID = 60118 };
	u32 percentValue; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_UNKNOWN_60118, 4);

PUSH_PACKED
struct CQ_UNKNOWN_60119
{
	enum { NET_ID = 60119 };
	u16 textField_len; // 2 bytes
	wchar_t textField[1]; // 2 bytes each (wide string: u16 len + len*2)
};
POP_PACKED

PUSH_PACKED
struct CQ_FriendSetComrade
{
	enum { NET_ID = 60120 };
	u16 textField_len; // 2 bytes
	wchar_t textField[1]; // 2 bytes each (wide string: u16 len + len*2)
};
POP_PACKED

PUSH_PACKED
struct CQ_FriendUnsetComrade
{
	enum { NET_ID = 60121 };
	u16 textField_len; // 2 bytes
	wchar_t textField[1]; // 2 bytes each (wide string: u16 len + len*2)
};
POP_PACKED

PUSH_PACKED
struct CQ_FriendSetFavorite
{
	enum { NET_ID = 60122 };
	u16 textField_len; // 2 bytes
	wchar_t textField[1]; // 2 bytes each (wide string: u16 len + len*2)
};
POP_PACKED

PUSH_PACKED
struct CQ_FriendUnsetFavorite
{
	enum { NET_ID = 60123 };
	u16 textField_len; // 2 bytes
	wchar_t textField[1]; // 2 bytes each (wide string: u16 len + len*2)
};
POP_PACKED

PUSH_PACKED
struct CQ_FriendRemove
{
	enum { NET_ID = 60124 };
	u16 textField_len; // 2 bytes
	wchar_t textField[1]; // 2 bytes each (wide string: u16 len + len*2)
};
POP_PACKED

PUSH_PACKED
struct CQ_CommunityStatusMessage
{
	enum { NET_ID = 60125 };
	u16 textField_len; // 2 bytes
	wchar_t textField[1]; // 2 bytes each (wide string: u16 len + len*2)
};
POP_PACKED

PUSH_PACKED
struct CQ_CommunityChannelMove
{
	enum { NET_ID = 60126 };
	u16 targetNick_len; // 2 bytes
	wchar_t targetNick[1]; // 2 bytes each (wide string: u16 len + len*2)
};
POP_PACKED

PUSH_PACKED
struct CQ_CommunityBlockAdd
{
	enum { NET_ID = 60127 };
	u16 textField_len; // 2 bytes
	wchar_t textField[1]; // 2 bytes each (wide string: u16 len + len*2)
};
POP_PACKED

PUSH_PACKED
struct CQ_GetUserInfo
{
	enum { NET_ID = 60128 };
	u16 textField_len; // 2 bytes
	wchar_t textField[1]; // 2 bytes each (wide string: u16 len + len*2)
};
POP_PACKED

PUSH_PACKED
struct CQ_JukeboxEnqueue
{
	enum { NET_ID = 60129 };
	u16 textField_len; // 2 bytes
	wchar_t textField[1]; // 2 bytes each (wide string: u16 len + len*2)
};
POP_PACKED

PUSH_PACKED
struct CQ_UNKNOWN_60131
{
	enum { NET_ID = 60131 };
};
POP_PACKED

PUSH_PACKED
struct CQ_UNKNOWN_60132
{
	enum { NET_ID = 60132 };
	u32 field1_id_id; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_UNKNOWN_60132, 4);

PUSH_PACKED
struct CQ_CreateGuild
{
	enum { NET_ID = 60133 };
	u16 textField1_len; // 2 bytes
	wchar_t textField1[1]; // 2 bytes each (wide string: u16 len + len*2)
	u16 textField2_len; // 2 bytes
	wchar_t textField2[1]; // 2 bytes each (wide string: u16 len + len*2)
};
POP_PACKED

PUSH_PACKED
struct CQ_DissolveGuild
{
	enum { NET_ID = 60134 };
};
POP_PACKED

PUSH_PACKED
struct CQ_UNKNOWN_60135
{
	enum { NET_ID = 60135 };
};
POP_PACKED

PUSH_PACKED
struct CQ_KickGuildMember
{
	enum { NET_ID = 60137 };
	u16 textField_len; // 2 bytes
	wchar_t textField[1]; // 2 bytes each (wide string: u16 len + len*2)
};
POP_PACKED

PUSH_PACKED
struct CQ_QuitGuild
{
	enum { NET_ID = 60138 };
};
POP_PACKED

PUSH_PACKED
struct CQ_UNKNOWN_60139
{
	enum { NET_ID = 60139 };
	u16 textField_len; // 2 bytes
	wchar_t textField[1]; // 2 bytes each (wide string: u16 len + len*2)
};
POP_PACKED

PUSH_PACKED
struct CQ_GetGuildJoinRequestList
{
	enum { NET_ID = 60140 };
};
POP_PACKED

PUSH_PACKED
struct CQ_RespondGuildJoinRequest
{
	enum { NET_ID = 60141 };
	u16 valueAsString_len; // 2 bytes
	wchar_t valueAsString[1]; // 2 bytes each (wide string: u16 len + len*2)
	u8 flag; // 1 bytes (bool)
};
POP_PACKED

PUSH_PACKED
struct CQ_InviteGuildMember
{
	enum { NET_ID = 60142 };
	u16 textField_len; // 2 bytes
	wchar_t textField[1]; // 2 bytes each (wide string: u16 len + len*2)
};
POP_PACKED

PUSH_PACKED
struct CQ_GetGuildInvitationList
{
	enum { NET_ID = 60143 };
};
POP_PACKED

PUSH_PACKED
struct CQ_RespondGuildInvitation
{
	enum { NET_ID = 60144 };
	u16 valueAsString_len; // 2 bytes
	wchar_t valueAsString[1]; // 2 bytes each (wide string: u16 len + len*2)
	u8 flag; // 1 bytes (bool)
};
POP_PACKED

PUSH_PACKED
struct CQ_GetGuildMemberStatusList
{
	enum { NET_ID = 60147 };
};
POP_PACKED

PUSH_PACKED
struct CQ_GetGuildPublicProfile
{
	enum { NET_ID = 60149 };
	u16 textField_len; // 2 bytes
	wchar_t textField[1]; // 2 bytes each (wide string: u16 len + len*2)
};
POP_PACKED

PUSH_PACKED
struct CQ_UNKNOWN_60150
{
	enum { NET_ID = 60150 };
	u16 textField_len; // 2 bytes
	wchar_t textField[1]; // 2 bytes each (wide string: u16 len + len*2)
};
POP_PACKED

PUSH_PACKED
struct CQ_EditGuildNotice
{
	enum { NET_ID = 60152 };
	u16 textField_len; // 2 bytes
	wchar_t textField[1]; // 2 bytes each (wide string: u16 len + len*2)
};
POP_PACKED

PUSH_PACKED
struct CQ_EditGuildIntro
{
	enum { NET_ID = 60153 };
	u16 textField_len; // 2 bytes
	wchar_t textField[1]; // 2 bytes each (wide string: u16 len + len*2)
};
POP_PACKED

PUSH_PACKED
struct CQ_EditGuildTag
{
	enum { NET_ID = 60154 };
	u16 textField_len; // 2 bytes
	wchar_t textField[1]; // 2 bytes each (wide string: u16 len + len*2)
};
POP_PACKED

PUSH_PACKED
struct CQ_SetGuildInterest
{
	enum { NET_ID = 60155 };
	u32 entity7ByteRecord; // 4 bytes (unclassified writer bytes[7])
};
POP_PACKED

PUSH_PACKED
struct CQ_SetGuildEmblem
{
	enum { NET_ID = 60156 };
	u32 field1_id_id; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_SetGuildEmblem, 4);

PUSH_PACKED
struct CQ_SetGuildJoinType
{
	enum { NET_ID = 60157 };
	u8 stateByte; // 1 bytes (bool)
};
POP_PACKED
ASSERT_SIZE(CQ_SetGuildJoinType, 1);

PUSH_PACKED
struct CQ_CreateGuildMembership
{
	enum { NET_ID = 60158 };
	u32 targetName; // 4 bytes (unclassified writer wstring)
	u32 targetEntryId; // 4 bytes
	u32 actionIndex; // 4 bytes
};
POP_PACKED

PUSH_PACKED
struct CQ_DeleteGuildMembership
{
	enum { NET_ID = 60159 };
	u32 name; // 4 bytes (unclassified writer wstring)
};
POP_PACKED

PUSH_PACKED
struct CQ_EditGuildMembership
{
	enum { NET_ID = 60160 };
	u8 gradeIndex; // 1 bytes (bool)
	u32 gradeName; // 4 bytes (unclassified writer wstring)
	u32 unknownBytes10; // 4 bytes (unclassified writer 10 x u8 struct)
};
POP_PACKED

PUSH_PACKED
struct CQ_DonateToGuild
{
	enum { NET_ID = 60161 };
	u32 gradeId; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_DonateToGuild, 4);

PUSH_PACKED
struct CQ_ExtendGuildLimitedSkill
{
	enum { NET_ID = 60162 };
	u32 gradeId; // 4 bytes
	u8 gradeIndex; // 1 bytes (bool)
	u32 gradeName; // 4 bytes (unclassified writer wstring)
	u32 unknownBytes10; // 4 bytes (unclassified writer 10 x u8 struct)
};
POP_PACKED

PUSH_PACKED
struct CQ_BuyGuildSkill
{
	enum { NET_ID = 60163 };
	u32 targetId; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_BuyGuildSkill, 4);

PUSH_PACKED
struct CQ_GetGuildRankList
{
	enum { NET_ID = 60164 };
};
POP_PACKED

PUSH_PACKED
struct CQ_GetGuildRank
{
	enum { NET_ID = 60165 };
	u8 settingA; // 1 bytes (bool)
	u8 settingB; // 1 bytes (bool)
	u8 settingC; // 1 bytes (bool)
};
POP_PACKED
ASSERT_SIZE(CQ_GetGuildRank, 3);

PUSH_PACKED
struct CQ_GetRecommendedGuildList
{
	enum { NET_ID = 60166 };
	u8 settingA; // 1 bytes (bool)
	u16 value16; // 2 bytes
	u8 settingB; // 1 bytes (bool)
};
POP_PACKED
ASSERT_SIZE(CQ_GetRecommendedGuildList, 4);

PUSH_PACKED
struct CQ_GetGuildRankingList
{
	enum { NET_ID = 60168 };
	u8 category; // 1 bytes (bool)
	u16 value16; // 2 bytes
	u32 mode; // 4 bytes
	u8 value8; // 1 bytes (bool)
};
POP_PACKED
ASSERT_SIZE(CQ_GetGuildRankingList, 8);

PUSH_PACKED
struct CQ_GetGuildRanking
{
	enum { NET_ID = 60169 };
	u8 rankingType;
	u16 seasonNo;
	u16 unkQueryOrGuildNameLen;
	wchar unkQueryOrGuildName[1]; // length is unkQueryOrGuildNameLen


};
POP_PACKED

PUSH_PACKED
struct CQ_ChatChannelInviteFeedback
{
	enum { NET_ID = 60170 };
	u8 category; // 1 bytes (bool)
};
POP_PACKED
ASSERT_SIZE(CQ_ChatChannelInviteFeedback, 1);

PUSH_PACKED
struct CQ_UNKNOWN_60171
{
	enum { NET_ID = 60171 };
	u8 actionType; // 1 bytes (bool)
};
POP_PACKED
ASSERT_SIZE(CQ_UNKNOWN_60171, 1);

PUSH_PACKED
struct CQ_UNKNOWN_60172
{
	enum { NET_ID = 60172 };
	u32 queryName; // 4 bytes (unclassified writer wstring)
	u8 flagLevelRange; // 1 bytes (bool)
	u8 flagActive; // 1 bytes (bool)
	u8 flagHasOptions; // 1 bytes (bool)
	u8 valueA; // 1 bytes (bool)
	u8 valueB; // 1 bytes (bool)
	u8 unknownBytes7; // 1 bytes (bool)
	u32 requestParam; // 4 bytes (unclassified writer 7 x u8 struct)
	u32 constOpt8; // 4 bytes
	u8 constOpt8_3; // 1 bytes (bool)
};
POP_PACKED

PUSH_PACKED
struct CQ_UNKNOWN_60173
{
	enum { NET_ID = 60173 };
	u16 field0; // 2 bytes
	u8 field0_3; // 1 bytes (bool)
};
POP_PACKED
ASSERT_SIZE(CQ_UNKNOWN_60173, 3);

PUSH_PACKED
struct CQ_UNKNOWN_60174
{
	enum { NET_ID = 60174 };
	u32 uiValue; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_UNKNOWN_60174, 4);

PUSH_PACKED
struct CQ_UNKNOWN_60175
{
	enum { NET_ID = 60175 };
	u32 field0; // 4 bytes
	u32 field0_3; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_UNKNOWN_60175, 8);

PUSH_PACKED
struct CQ_UNKNOWN_60176
{
	enum { NET_ID = 60176 };
	u32 channelId; // 4 bytes (unclassified writer wstring)
	u32 targetName; // 4 bytes (unclassified writer wstring)
	u32 channelInfoU32; // 4 bytes
	u32 padZero; // 4 bytes
	u32 channelTextA; // 4 bytes (unclassified writer wstring)
	u32 channelTextB; // 4 bytes (unclassified writer wstring)
	u8 flagSet1; // 1 bytes (bool)
};
POP_PACKED

PUSH_PACKED
struct CA_UNKNOWN_60177
{
	enum { NET_ID = 60177 };
	u32 channelId; // 4 bytes
	u32 channelName; // 4 bytes (unclassified writer wstring)
	u32 secretFlag; // 4 bytes (unclassified writer wstring)
};
POP_PACKED

PUSH_PACKED
struct CQ_WeaponState
{
	enum { NET_ID = 60180 };
	u32 field0; // 4 bytes
	u32 field0_3; // 4 bytes
	u8 field1; // 1 bytes (bool)
	u8 field2; // 1 bytes (bool)
	u32 field3; // 4 bytes
	u32 field4; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_WeaponState, 18);

PUSH_PACKED
struct CQ_SyncActionMove
{
	enum { NET_ID = 60181 };
	u32 actorID;
	u32 actionID;
	float3 destPos;


};
ASSERT_SIZE(CQ_SyncActionMove, 20);

POP_PACKED

PUSH_PACKED
struct CQ_AISyncMoveMotion
{
	enum { NET_ID = 60182 };
	u32 actorID;
	u32 unkU32b;
	u32 unkU32c;
	u32 unkU32d;
	u32 unkU32e;
	float3 motionVec;
	u32 unkTail1;
	u32 unkTail2;


};
ASSERT_SIZE(CQ_AISyncMoveMotion, 40);

POP_PACKED

PUSH_PACKED
struct CQ_AISyncBehaviorMotion
{
	enum { NET_ID = 60183 };
	u32 actorID;
	u32 behaviorID;
	u8 unkU8;
	float3 pos1;
	float3 pos2;
	float3 pos3;
	float3 pos4;
	u32 unkTail1;
	u32 unkTail2;


};
ASSERT_SIZE(CQ_AISyncBehaviorMotion, 65);

POP_PACKED

PUSH_PACKED
struct CQ_AISyncSpeedRatio
{
	enum { NET_ID = 60184 };
	u32 field0; // 4 bytes
	u32 field0_3; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_AISyncSpeedRatio, 8);

PUSH_PACKED
struct CQ_AISyncRotateRatio
{
	enum { NET_ID = 60185 };
	u32 field0; // 4 bytes
	u32 field0_3; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_AISyncRotateRatio, 8);

PUSH_PACKED
struct CQ_AISyncSceneRoot
{
	enum { NET_ID = 60186 };
	u32 field0; // 4 bytes
	u32 field0_3; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_AISyncSceneRoot, 8);

PUSH_PACKED
struct CQ_AISyncTarPos
{
	enum { NET_ID = 60187 };
	u32 actorID;
	u32 unkU32;
	float3 tarPos1;
	float3 tarPos2;
	float3 tarPos3;
	float3 tarPos4;
	u32 unkTail1;
	u32 unkTail2;
	u32 unkTail3;


};
ASSERT_SIZE(CQ_AISyncTarPos, 68);

POP_PACKED

PUSH_PACKED
struct CQ_AISyncTarPosToServer
{
	enum { NET_ID = 60188 };
	u32 actorID;
	u32 unkU32;
	float3 tarPos;
	float2 unkVec2;
	u32 unkTail1;
	u32 unkTail2;
	u32 unkTail3;
	u32 unkTail4;


};
ASSERT_SIZE(CQ_AISyncTarPosToServer, 44);

POP_PACKED

PUSH_PACKED
struct CQ_RemoteSyncCreateFromCreatorId
{
	enum { NET_ID = 60189 };
	u32 entityValue; // 4 bytes
	u32 ownerId; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_RemoteSyncCreateFromCreatorId, 8);

PUSH_PACKED
struct CQ_RemoteSyncTarget
{
	enum { NET_ID = 60191 };
	u32 field0; // 4 bytes
	u32 field0_3; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_RemoteSyncTarget, 8);

PUSH_PACKED
struct CN_UNKNOWN_60193
{
	enum { NET_ID = 60193 };
	u32 unkA;
	u32 unkB;
	float3 unkPos;


};
ASSERT_SIZE(CN_UNKNOWN_60193, 20);

POP_PACKED

PUSH_PACKED
struct CQ_GamePlayerSyncPhy
{
	enum { NET_ID = 60195 };
	u32 unkId1;
	u32 unkId2;
	u32 unkId3;
	float3 currPos;
	u16 unkShort1;
	u16 unkShort2;
	u16 unkShort3;
	u16 unkShort4;
	u16 unkShort5;
	u16 unkShort6;
	u16 unkShort7;
	u16 unkShort8;
	u16 unkShort9;
	u16 charNameLen;
	wchar charName[1]; // length is charNameLen
	u16 unkU16;
	u32 unkA;
	u32 unkB;
	u16 unkU16b;
	u16 unkU16c;
	u16 unkU16d;
	u8 unkU8a;
	u8 unkU8b;
	u16 unkU16e;
	u8 unkU8c;
	u32 unkC;
	u32 unkD0;
	u32 unkD1;
	u32 unkD2;
	u32 unkD3;
	u8 unkU8d;
	u8 unkU8e;


};
POP_PACKED

PUSH_PACKED
struct CQ_GamePlayerSyncStatus
{
	enum { NET_ID = 60196 };
	u32 unkId1;
	u32 unkId2;
	u32 unkId3;
	float3 currPos;
	u16 unkShort1;
	u16 unkShort2;
	u16 unkShort3;
	u16 unkShort4;
	u16 unkShort5;
	u16 unkShort6;
	u16 unkShort7;
	u16 unkShort8;
	u16 unkShort9;
	u16 charNameLen;
	wchar charName[1]; // length is charNameLen
	u16 unkU16;
	u32 unkA;
	u32 unkB;
	u32 unkC;
	u32 unkD;
	u32 unkE;
	u32 unkF;
	u32 unkG;
	u8 unkU8a;
	u8 unkU8b;


};
POP_PACKED

PUSH_PACKED
struct CQ_RevivePartyMemberUseMyLifeCount
{
	enum { NET_ID = 60197 };
	u8 bPresenceFlags;
	u32 unkID1;
	u32 targetMemberID;
	float3 revivePos;
	float3 optionalPos;
	u32 lifeCount;


};

POP_PACKED

PUSH_PACKED
struct CN_UNKNOWN_60198
{
	enum { NET_ID = 60198 };
	u32 elements; // 4 bytes (unclassified writer array: u16 count + N x {u32,u32,u32,u32,u32,u32,u8} (28-byte element))
};
POP_PACKED

PUSH_PACKED
struct CN_PlayerTagMaster
{
	enum { NET_ID = 60199 };
	LocalActorID masterActorID; // 4 bytes: master switched to via tag (21000-range); 0 sent at match start before CQ_GameIsReady
};
POP_PACKED

ASSERT_SIZE(CN_PlayerTagMaster, 4);

PUSH_PACKED
struct CQ_SinglemodeSelectedCharacter
{
	enum { NET_ID = 60200 };
	u32 field0; // 4 bytes
	u32 field0_3; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_SinglemodeSelectedCharacter, 8);

PUSH_PACKED
struct CN_UNKNOWN_60201
{
	enum { NET_ID = 60201 };
};
POP_PACKED

PUSH_PACKED
struct CN_UNKNOWN_60202
{
	enum { NET_ID = 60202 };
	u32 field0; // 4 bytes
	u32 field0_3; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CN_UNKNOWN_60202, 8);

PUSH_PACKED
struct CN_UNKNOWN_60203
{
	enum { NET_ID = 60203 };
	u32 field0; // 4 bytes
	u32 field0_3; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CN_UNKNOWN_60203, 8);

PUSH_PACKED
struct CN_UNKNOWN_60204
{
	enum { NET_ID = 60204 };
};
POP_PACKED

PUSH_PACKED
struct CN_UNKNOWN_60205
{
	enum { NET_ID = 60205 };
	u32 field0; // 4 bytes (unclassified writer wstring)
	u32 field0_3; // 4 bytes
	u32 field1; // 4 bytes (unclassified writer wstring)
	u32 field2; // 4 bytes
	float3 field3; // 12 bytes
};
POP_PACKED

PUSH_PACKED
struct CN_UNKNOWN_60206
{
	enum { NET_ID = 60206 };
	u32 field0; // 4 bytes
	u32 field0_3; // 4 bytes
	u32 field1; // 4 bytes
	u32 field2; // 4 bytes
	u32 field3; // 4 bytes
	u32 field4; // 4 bytes
	u32 field5; // 4 bytes
	u32 field6; // 4 bytes
	u32 field7; // 4 bytes
	u32 field8; // 4 bytes
	u32 field9; // 4 bytes
	u32 field10; // 4 bytes
	u32 field11; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CN_UNKNOWN_60206, 52);

PUSH_PACKED
struct CQ_UNKNOWN_60207
{
	enum { NET_ID = 60207 };
	u32 itemId; // 4 bytes
	u32 destSlot; // 4 bytes
	u32 srcSlot; // 4 bytes
	u32 slotExtra; // 4 bytes
	u32 itemCount; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_UNKNOWN_60207, 20);

PUSH_PACKED
// NOTE: old-version ID mapping called this CQ_RewardSelectSlot, but that packet is
// 60214 in this binary (+6 drift; strict SA_RewardSelectSlot(62413) ack pairing).
// Never observed on the wire. Alpha 60202 = CN_INVENTORY_REARRANGE.
struct CN_UNKNOWN_60208
{
	enum { NET_ID = 60208 };
	u32 unkValue; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CN_UNKNOWN_60208, 4);

PUSH_PACKED
struct CQ_RewardSelectSlotAllRandom
{
	enum { NET_ID = 60209 };
	u32 unkBoxID; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_RewardSelectSlotAllRandom, 4);

PUSH_PACKED
struct CQ_UNKNOWN_60211
{
	enum { NET_ID = 60211 };
	u32 unk1; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_UNKNOWN_60211, 4);

PUSH_PACKED
struct CA_UNKNOWN_60212
{
	enum { NET_ID = 60212 };
};
POP_PACKED

PUSH_PACKED
struct CQ_UNKNOWN_60213
{
	enum { NET_ID = 60213 };
};
POP_PACKED

PUSH_PACKED
struct CQ_RewardSelectSlot
{
	enum { NET_ID = 60214 };
	u32 slotIndex; // 4 bytes: selected reward slot (observed 6-13); always acked by SA_RewardSelectSlot(62413)
};
POP_PACKED
ASSERT_SIZE(CQ_RewardSelectSlot, 4);

PUSH_PACKED
struct CN_UNKNOWN_60215
{
	enum { NET_ID = 60215 };
};
POP_PACKED

PUSH_PACKED
struct CN_UNKNOWN_60216
{
	enum { NET_ID = 60216 };
};
POP_PACKED

PUSH_PACKED
struct CN_UNKNOWN_60217
{
	enum { NET_ID = 60217 };
};
POP_PACKED

PUSH_PACKED
struct CQ_UNKNOWN_60218
{
	enum { NET_ID = 60218 };
	u8 option_index_u8; // 1 bytes (bool)
};
POP_PACKED
ASSERT_SIZE(CQ_UNKNOWN_60218, 1);

PUSH_PACKED
struct CQ_MailWrite
{
	enum { NET_ID = 60219 };
	u32 wstring_1; // 4 bytes (unclassified writer wstring)
	u32 wstring_2; // 4 bytes (unclassified writer wstring)
	u32 wstring_3; // 4 bytes (unclassified writer wstring)
};
POP_PACKED

PUSH_PACKED
struct CQ_MailRead
{
	enum { NET_ID = 60220 };
	u64 target_user_id_u64; // 8 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_MailRead, 8);

PUSH_PACKED
struct CQ_MailMove
{
	enum { NET_ID = 60221 };
	u64 queued_id_u64; // 8 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_MailMove, 8);

PUSH_PACKED
struct CQ_MailGetAttachmentMulti
{
	enum { NET_ID = 60222 };
	VEC(u64, attachmentUIDs);


};
POP_PACKED

PUSH_PACKED
struct CQ_MailGetNewMail
{
	enum { NET_ID = 60223 };
	u8 unkFlag;
	VEC(u64, mailUIDs);


};
POP_PACKED

PUSH_PACKED
struct CQ_UNKNOWN_60224
{
	enum { NET_ID = 60224 };
};
POP_PACKED

PUSH_PACKED
struct CQ_UNKNOWN_60225
{
	enum { NET_ID = 60225 };
};
POP_PACKED

PUSH_PACKED
struct CQ_WarehouseItemDelete
{
	enum { NET_ID = 60226 };
	u32 amount_u32; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_WarehouseItemDelete, 4);

PUSH_PACKED
struct CQ_UNKNOWN_60227
{
	enum { NET_ID = 60227 };
	u32 value_u32; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_UNKNOWN_60227, 4);

PUSH_PACKED
struct CQ_UNKNOWN_60228
{
	enum { NET_ID = 60228 };
};
POP_PACKED

PUSH_PACKED
struct CQ_UNKNOWN_60229
{
	enum { NET_ID = 60229 };
	u32 action_code_u32; // 4 bytes
	u8 slot_value_u8; // 1 bytes (bool)
};
POP_PACKED
ASSERT_SIZE(CQ_UNKNOWN_60229, 5);

PUSH_PACKED
struct CQ_UNKNOWN_60230
{
	enum { NET_ID = 60230 };
	u32 slot_id_u32; // 4 bytes
	u32 flag_u32; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_UNKNOWN_60230, 8);

PUSH_PACKED
struct CN_UNKNOWN_60231
{
	enum { NET_ID = 60231 };
};
POP_PACKED

PUSH_PACKED
struct CN_UNKNOWN_60232
{
	enum { NET_ID = 60232 };
	u32 owner_container_u32; // 4 bytes
	u32 id_a_u32; // 4 bytes
	u32 entity_field_a8c_u32; // 4 bytes
	u32 id_b_u32; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CN_UNKNOWN_60232, 16);

PUSH_PACKED
struct CN_UNKNOWN_60233
{
	enum { NET_ID = 60233 };
	u32 context_id_u32; // 4 bytes
	float3 target_world_pos_vec3; // 12 bytes
	u32 picked_entity_id_u32; // 4 bytes
	u32 interaction_type_u32; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CN_UNKNOWN_60233, 24);

PUSH_PACKED
struct CA_Heartbeat
{
	enum { NET_ID = 60234 };
};
POP_PACKED

PUSH_PACKED
struct CQ_SortieMasterBan
{
	enum { NET_ID = 60236 };
	u32 input_value_u32; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_SortieMasterBan, 4);

PUSH_PACKED
struct CQ_SortieMasterAssign
{
	enum { NET_ID = 60237 };
	u32 input_value_u32; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_SortieMasterAssign, 4);

PUSH_PACKED
struct CQ_UNKNOWN_60238
{
	enum { NET_ID = 60238 };
};
POP_PACKED

PUSH_PACKED
// Fired from City_Scene_OnEnter when the client enters the city scene (constant 0).
struct CN_EnterCityScene
{
	enum { NET_ID = 60239 };
	u8 unkZero; // 1 byte: always 0
};
POP_PACKED
ASSERT_SIZE(CN_EnterCityScene, 1);

PUSH_PACKED
// Sibling scene-entered notification for scene types 2/3 (dispatcher FUN_0080a20a:
// type 1 -> CN_EnterCityScene(0), type 2 -> (0,1), type 3 -> (0,2)).
struct CN_SceneEnter
{
	enum { NET_ID = 60240 };
	u8 unkZero; // 1 byte: always 0
	u8 sceneId; // 1 byte: 0-based scene id (observed 1 and 2)
};
POP_PACKED
ASSERT_SIZE(CN_SceneEnter, 2);

PUSH_PACKED
struct CQ_UNKNOWN_60241
{
	enum { NET_ID = 60241 };
	u8 entry_id_u8; // 1 bytes (bool)
	u8 subtype_u8; // 1 bytes (bool)
};
POP_PACKED
ASSERT_SIZE(CQ_UNKNOWN_60241, 2);

PUSH_PACKED
struct CQ_UNKNOWN_60242
{
	enum { NET_ID = 60242 };
	u32 computed_id_u32; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_UNKNOWN_60242, 4);

PUSH_PACKED
struct CQ_UNKNOWN_60243
{
	enum { NET_ID = 60243 };
	u32 entity_id_u32; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_UNKNOWN_60243, 4);

PUSH_PACKED
struct CN_Ping
{
	enum { NET_ID = 60244 };
	u32 rttMs; // 4 bytes: median of >=10 accumulated latency samples (FUN_01a9804a); HUD logs 'Update HUD RTT : %d'; >249ms triggers bad-network warning
};
POP_PACKED
ASSERT_SIZE(CN_Ping, 4);

PUSH_PACKED
struct CQ_CompleteNonSyncEvents
{
	enum { NET_ID = 60246 };
	u32 param_u32; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_CompleteNonSyncEvents, 4);

PUSH_PACKED
struct CQ_InteractionCasting
{
	enum { NET_ID = 60247 };
	u32 self_entity_id_u32; // 4 bytes
	u32 target_entity_id_u32; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_InteractionCasting, 8);

PUSH_PACKED
struct CQ_InteractionCancel
{
	enum { NET_ID = 60248 };
	u32 param_u32; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_InteractionCancel, 4);

PUSH_PACKED
struct CQ_PlayerSyncTeleport
{
	enum { NET_ID = 60249 };
	u32 objectID;
	float3 pos;


};
POP_PACKED
ASSERT_SIZE(CQ_PlayerSyncTeleport, 16);

PUSH_PACKED
struct CQ_SaveSettings
{
	enum { NET_ID = 60250 };
	u8 unkFlag;
	VEC(u8, settingsData); // blob


};
POP_PACKED

PUSH_PACKED
struct CQ_RefreshWaitingQueue
{
	enum { NET_ID = 60251 };
};
POP_PACKED

PUSH_PACKED
struct CQ_ItemTrade
{
	enum { NET_ID = 60252 };
	u32 unkContextId; // 4 bytes: game-state singleton +0xA88, gated on TIREDNESS_NOT_AVAILABLE; observed 5007
	u32 itemDocId; // 4 bytes: item doc index (e.g. 241020015), family of server-echoed itemIndex 136020015
	u8 count; // 1 byte: quantity or flag, observed 1
};
POP_PACKED
ASSERT_SIZE(CQ_ItemTrade, 9);

PUSH_PACKED
struct CQ_SkinChipDissolve
{
	enum { NET_ID = 60253 };
	u8 unkZero; // 1 byte: UI caller hardcodes 0
	u32 chipItemDocId; // 4 bytes: skin-chip item doc index (e.g. 240000002)
	u8 craftCount; // 1 byte: dissolve count, clamped by 'craft max count over' check in UI
};
POP_PACKED
ASSERT_SIZE(CQ_SkinChipDissolve, 6);

PUSH_PACKED
struct CQ_OlympicBestRecord
{
	enum { NET_ID = 60254 };
	u8 unkFlag;
	// u16 count + count x (3 x u32)
	VEC(u32, records); // 12 bytes per element


};
POP_PACKED

PUSH_PACKED
struct CQ_PveRefillBattery
{
	enum { NET_ID = 60255 };
};
POP_PACKED

PUSH_PACKED
struct CN_UNKNOWN_60256
{
	enum { NET_ID = 60256 };
};
POP_PACKED

PUSH_PACKED
struct CQ_UNKNOWN_60257
{
	enum { NET_ID = 60257 };
};
POP_PACKED

PUSH_PACKED
struct CQ_UNKNOWN_60259
{
	enum { NET_ID = 60259 };
	u8 value_u8; // 1 bytes (bool)
};
POP_PACKED
ASSERT_SIZE(CQ_UNKNOWN_60259, 1);

PUSH_PACKED
struct CN_UNKNOWN_60260
{
	enum { NET_ID = 60260 };
	u32 context_id_u32; // 4 bytes
	u32 resolved_entity_id_u32; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CN_UNKNOWN_60260, 8);

PUSH_PACKED
struct CQ_UNKNOWN_60261
{
	enum { NET_ID = 60261 };
	u16 textLen;
	wchar text[1]; // length is textLen


};
POP_PACKED

PUSH_PACKED
struct CQ_UNKNOWN_60262
{
	enum { NET_ID = 60262 };
	u16 textLen;
	wchar text[1]; // length is textLen
	u8 category;


};
POP_PACKED

PUSH_PACKED
struct CQ_UNKNOWN_60263
{
	enum { NET_ID = 60263 };
	u16 textLen;
	wchar text[1]; // length is textLen


};
POP_PACKED

PUSH_PACKED
struct CN_UNKNOWN_60264
{
	enum { NET_ID = 60264 };
};
POP_PACKED

PUSH_PACKED
struct CQ_NcguardData
{
	enum { NET_ID = 60265 };
	VEC(u8, guardPayload); // blob


};
POP_PACKED

PUSH_PACKED
struct CA_UNKNOWN_60266
{
	enum { NET_ID = 60266 };
	u16 total_size_u16; // 2 bytes
	u16 netid_u16; // 2 bytes
	u16 data_len_u16; // 2 bytes
	u32 payload_blob; // 4 bytes (unclassified writer blob[u8])
};
POP_PACKED

PUSH_PACKED
struct CA_UNKNOWN_60267
{
	enum { NET_ID = 60267 };
};
POP_PACKED

PUSH_PACKED
struct CQ_RequestVoting
{
	enum { NET_ID = 60268 };
	u32 param_u32; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_RequestVoting, 4);

PUSH_PACKED
struct CN_UNKNOWN_60269
{
	enum { NET_ID = 60269 };
	u32 param_u32; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CN_UNKNOWN_60269, 4);

PUSH_PACKED
struct CQ_Vote
{
	enum { NET_ID = 60270 };
	u32 zero_u32; // 4 bytes
	u32 boolean_state_u32; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_Vote, 8);

PUSH_PACKED
struct CQ_UNKNOWN_60271
{
	enum { NET_ID = 60271 };
};
POP_PACKED

PUSH_PACKED
struct CQ_CalendarDetail
{
	enum { NET_ID = 60273 };
	u32 value_u32; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_CalendarDetail, 4);

PUSH_PACKED
struct CQ_StageSkillSelect
{
	enum { NET_ID = 60274 };
	u32 object_id_u32; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_StageSkillSelect, 4);

PUSH_PACKED
struct CQ_NP_CharacterSkinUnlock
{
	enum { NET_ID = 60276 };
	u32 item_id_u32; // 4 bytes
	u32 char_id_u32; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_NP_CharacterSkinUnlock, 8);

PUSH_PACKED
struct CQ_NP_WarehouseExpand
{
	enum { NET_ID = 60277 };
	u32 target_handle_u32; // 4 bytes
	u32 flag_u32; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_NP_WarehouseExpand, 8);

PUSH_PACKED
struct CQ_NP_InventoryExpand
{
	enum { NET_ID = 60278 };
	u32 player_id_u32; // 4 bytes
	u16 mode_u16; // 2 bytes
	u16 unknown_u16; // 2 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_NP_InventoryExpand, 8);

PUSH_PACKED
struct CQ_NP_ReviveByCoin
{
	enum { NET_ID = 60279 };
	u8 guide_mission_flag_u8; // 1 bytes (bool)
};
POP_PACKED
ASSERT_SIZE(CQ_NP_ReviveByCoin, 1);

PUSH_PACKED
struct CQ_NP_RewardBuyChance
{
	enum { NET_ID = 60280 };
	u8 tiredness_flag_u8; // 1 bytes (bool)
};
POP_PACKED
ASSERT_SIZE(CQ_NP_RewardBuyChance, 1);

PUSH_PACKED
struct CQ_NP_RewardOpenAllSlots
{
	enum { NET_ID = 60281 };
	u8 state_u8; // 1 bytes (bool)
};
POP_PACKED
ASSERT_SIZE(CQ_NP_RewardOpenAllSlots, 1);

PUSH_PACKED
struct CQ_BuyGift
{
	enum { NET_ID = 60282 };
	u8 amount_u8; // 1 bytes (bool)
};
POP_PACKED
ASSERT_SIZE(CQ_BuyGift, 1);

PUSH_PACKED
struct CQ_MasterTrainingEvent
{
	enum { NET_ID = 60283 };
	u32 unkDword1;
	u32 unkDword2;
	u64 unkQword;
	u16 unkWord1;
	u32 unkDword3;
	u32 unkDword4;
	u32 unkDword5;
	u32 unkDword6;
	u32 unkDword7;
	u32 unkDword8;
	u16 unkWstr1Len;
	wchar unkWstr1[1]; // length is unkWstr1Len
	u16 unkWstr2Len;
	wchar unkWstr2[1]; // length is unkWstr2Len
	// u16 count + count x (3 x u32)
	VEC(u32, unkTripleVec); // 12 bytes per element


};
POP_PACKED

PUSH_PACKED
struct CQ_PromotionAttend
{
	enum { NET_ID = 60284 };
	u32 unkDword1;
	u32 unkDword2;
	u64 unkQword;
	u16 unkWord1;
	u32 unkDword3;
	u32 unkDword4;
	u32 unkDword5;
	u32 unkDword6;
	u32 unkDword7;
	u32 unkDword8;
	u16 unkWstr1Len;
	wchar unkWstr1[1]; // length is unkWstr1Len
	u16 unkWstr2Len;
	wchar unkWstr2[1]; // length is unkWstr2Len
	// u16 count + count x (3 x u32)
	VEC(u32, unkTripleVec); // 12 bytes per element


};
POP_PACKED

PUSH_PACKED
struct CQ_CharacterPropertyUpgrade
{
	enum { NET_ID = 60285 };
	u32 unkDword1;
	u32 unkDword2;
	u64 unkQword;
	u16 unkWord1;
	u32 unkDword3;
	u32 unkDword4;
	u32 unkDword5;
	u32 unkDword6;
	u32 unkDword7;
	u32 unkDword8;
	u16 unkWstr1Len;
	wchar unkWstr1[1]; // length is unkWstr1Len
	u16 unkWstr2Len;
	wchar unkWstr2[1]; // length is unkWstr2Len
	// u16 count + count x (3 x u32)
	VEC(u32, unkTripleVec); // 12 bytes per element


};
POP_PACKED

PUSH_PACKED
struct CQ_CharacterPropertyReset
{
	enum { NET_ID = 60286 };
	u32 unkDword1;
	u32 unkDword2;
	u64 unkQword;
	u16 unkWord1;
	u32 unkDword3;
	u32 unkDword4;
	u32 unkDword5;
	u32 unkDword6;
	u32 unkDword7;
	u32 unkDword8;
	u16 unkWstr1Len;
	wchar unkWstr1[1]; // length is unkWstr1Len
	u16 unkWstr2Len;
	wchar unkWstr2[1]; // length is unkWstr2Len
	// u16 count + count x (3 x u32)
	VEC(u32, unkTripleVec); // 12 bytes per element


};
POP_PACKED

PUSH_PACKED
struct CQ_ChatGetInviteeGameAccountID
{
	enum { NET_ID = 60287 };
	u32 unkDword1;
	u32 unkDword2;
	u64 unkQword;
	u16 unkWord1;
	u32 unkDword3;
	u32 unkDword4;
	u32 unkDword5;
	u32 unkDword6;
	u32 unkDword7;
	u32 unkDword8;
	u16 unkWstr1Len;
	wchar unkWstr1[1]; // length is unkWstr1Len
	u16 unkWstr2Len;
	wchar unkWstr2[1]; // length is unkWstr2Len
	// u16 count + count x (3 x u32)
	VEC(u32, unkTripleVec); // 12 bytes per element


};
POP_PACKED

PUSH_PACKED
struct CQ_UseCoupon
{
	enum { NET_ID = 60288 };
	u32 unkDword1;
	u32 unkDword2;
	u64 unkQword;
	u16 unkWord1;
	u32 unkDword3;
	u32 unkDword4;
	u32 unkDword5;
	u32 unkDword6;
	u32 unkDword7;
	u32 unkDword8;
	u16 unkWstr1Len;
	wchar unkWstr1[1]; // length is unkWstr1Len
	u16 unkWstr2Len;
	wchar unkWstr2[1]; // length is unkWstr2Len
	// u16 count + count x (3 x u32)
	VEC(u32, unkTripleVec); // 12 bytes per element


};
POP_PACKED

PUSH_PACKED
struct CQ_NotifyChattingChannelStatus
{
	enum { NET_ID = 60289 };
	u32 unkDword1;
	u32 unkDword2;
	u64 unkQword;
	u16 unkWord1;
	u32 unkDword3;
	u32 unkDword4;
	u32 unkDword5;
	u32 unkDword6;
	u32 unkDword7;
	u32 unkDword8;
	u16 unkWstr1Len;
	wchar unkWstr1[1]; // length is unkWstr1Len
	u16 unkWstr2Len;
	wchar unkWstr2[1]; // length is unkWstr2Len
	// u16 count + count x (3 x u32)
	VEC(u32, unkTripleVec); // 12 bytes per element


};
POP_PACKED

PUSH_PACKED
struct CQ_RequestToken
{
	enum { NET_ID = 60290 };
	u32 unkDword1;
	u32 unkDword2;
	u64 unkQword;
	u16 unkWord1;
	u32 unkDword3;
	u32 unkDword4;
	u32 unkDword5;
	u32 unkDword6;
	u32 unkDword7;
	u32 unkDword8;
	u16 unkWstr1Len;
	wchar unkWstr1[1]; // length is unkWstr1Len
	u16 unkWstr2Len;
	wchar unkWstr2[1]; // length is unkWstr2Len
	// u16 count + count x (3 x u32)
	VEC(u32, unkTripleVec); // 12 bytes per element


};
POP_PACKED

PUSH_PACKED
struct CN_UNKNOWN_60291
{
	enum { NET_ID = 60291 };
	u32 unkDword1;
	u32 unkDword2;
	u64 unkQword;
	u16 unkWord1;
	u32 unkDword3;
	u32 unkDword4;
	u32 unkDword5;
	u32 unkDword6;
	u32 unkDword7;
	u32 unkDword8;
	u16 unkWstr1Len;
	wchar unkWstr1[1]; // length is unkWstr1Len
	u16 unkWstr2Len;
	wchar unkWstr2[1]; // length is unkWstr2Len
	// u16 count + count x (3 x u32)
	VEC(u32, unkTripleVec); // 12 bytes per element


};
POP_PACKED

PUSH_PACKED
struct CN_UNKNOWN_60292
{
	enum { NET_ID = 60292 };
	u32 unkDword1;
	u32 unkDword2;
	u64 unkQword;
	u16 unkWord1;
	u32 unkDword3;
	u32 unkDword4;
	u32 unkDword5;
	u32 unkDword6;
	u32 unkDword7;
	u32 unkDword8;
	u16 unkWstr1Len;
	wchar unkWstr1[1]; // length is unkWstr1Len
	u16 unkWstr2Len;
	wchar unkWstr2[1]; // length is unkWstr2Len
	// u16 count + count x (3 x u32)
	VEC(u32, unkTripleVec); // 12 bytes per element


};
POP_PACKED

PUSH_PACKED
struct CQ_UNKNOWN_60293
{
	enum { NET_ID = 60293 };
};
POP_PACKED

PUSH_PACKED
struct CQ_UNKNOWN_60294
{
	enum { NET_ID = 60294 };
	u32 text_wstring; // 4 bytes (unclassified writer wstring)
};
POP_PACKED

PUSH_PACKED
struct CQ_TirednessRequest
{
	enum { NET_ID = 60295 };
	u32 unkDword1;
	u32 unkDword2;
	u64 unkQword;
	u16 unkWord1;
	u32 unkDword3;
	u32 unkDword4;
	u32 unkDword5;
	u32 unkDword6;
	u32 unkDword7;
	u32 unkDword8;
	u16 unkWstr1Len;
	wchar unkWstr1[1]; // length is unkWstr1Len
	u16 unkWstr2Len;
	wchar unkWstr2[1]; // length is unkWstr2Len
	// u16 count + count x (3 x u32)
	VEC(u32, unkTripleVec); // 12 bytes per element


};
POP_PACKED

PUSH_PACKED
struct CN_UNKNOWN_60296
{
	enum { NET_ID = 60296 };
	u8 mode_u8; // 1 bytes (bool)
	u32 arg0_u32; // 4 bytes
	u32 arg1_u32; // 4 bytes
	u32 arg2_u32; // 4 bytes
	u32 slot_pair_data; // 4 bytes (unclassified writer struct)
};
POP_PACKED

PUSH_PACKED
struct CQ_UNKNOWN_60297
{
	enum { NET_ID = 60297 };
	u16 param0_u16; // 2 bytes
	u16 param1_u16; // 2 bytes
};
POP_PACKED
ASSERT_SIZE(CQ_UNKNOWN_60297, 4);

PUSH_PACKED
struct CQ_MasterSupplyRequest
{
	enum { NET_ID = 60298 };
	LocalActorID masterActorID; // 4 bytes: own profile master (FIRST_SELF_MASTER range); observed 21015/21016
	// supplies: list of supply slot ids to grant; typically N copies of one id (UI builds deficit runs)
	// blob: variable-size (u16 count + count x u8)
	u16 byte_list_count; // 2 bytes
	u8 byte_list[1]; // 1 bytes each
};
POP_PACKED

PUSH_PACKED
struct CN_UNKNOWN_60299
{
	enum { NET_ID = 60299 };
	u32 value_u32; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(CN_UNKNOWN_60299, 4);

PUSH_PACKED
struct CQ_UNKNOWN_60301
{
	enum { NET_ID = 60301 };
	u16 unkStrLen;
	char unkStr[1]; // length is unkStrLen
	// u16 count + count x 13-byte struct elements
	VEC(u8, unkStructVec); // stride 13B


};
POP_PACKED

PUSH_PACKED
struct CN_NotifyMyVoiceStatus
{
	enum { NET_ID = 60302 };
	// voice-chat state notification: five context strings (channel/session/token - empty when offline)
	// + own character name; u32 = status/kind (0..3, see Voice_Notify* wrappers). Client debug:
	// '[VOICE] reqNotifyMyVoiceStatus'
	u16 voiceStr1Len;
	wchar voiceStr1[1]; // length is voiceStr1Len
	u16 voiceStr2Len;
	wchar voiceStr2[1]; // length is voiceStr2Len
	u16 voiceStr3Len;
	wchar voiceStr3[1]; // length is voiceStr3Len
	u16 voiceStr4Len;
	wchar voiceStr4[1]; // length is voiceStr4Len
	u16 voiceStr5Len;
	wchar voiceStr5[1]; // length is voiceStr5Len
	u16 voiceStr6Len;
	wchar voiceStr6[1]; // length is voiceStr6Len
	u32 voiceKind; // status/kind tag: 0|1 = my mic status, 2 = channel info, 3 = invitees


};
POP_PACKED

PUSH_PACKED
struct CA_UNKNOWN_60303
{
	enum { NET_ID = 60303 };
};
POP_PACKED

PUSH_PACKED
struct CQ_UNKNOWN_60304
{
	enum { NET_ID = 60304 };
	u32 text0_wstring; // 4 bytes (unclassified writer wstring)
	u8 flag_u8; // 1 bytes (bool)
	u32 text1_wstring; // 4 bytes (unclassified writer wstring)
};
POP_PACKED

PUSH_PACKED
struct CQ_UNKNOWN_60305
{
	enum { NET_ID = 60305 };
};
POP_PACKED

PUSH_PACKED
struct CN_UNKNOWN_60306
{
	enum { NET_ID = 60306 };
	u32 unkValue;


};
POP_PACKED
ASSERT_SIZE(CN_UNKNOWN_60306, 4);

PUSH_PACKED
struct CQ_UNKNOWN_60307
{
	enum { NET_ID = 60307 };
};
POP_PACKED

PUSH_PACKED
struct CQ_UNKNOWN_60308
{
	enum { NET_ID = 60308 };
	u32 text_wstring; // 4 bytes (unclassified writer wstring)
};
POP_PACKED

PUSH_PACKED
struct CN_UNKNOWN_60309
{
	enum { NET_ID = 60309 };
};
POP_PACKED

PUSH_PACKED
struct CN_UNKNOWN_60310
{
	enum { NET_ID = 60310 };
};
POP_PACKED

PUSH_PACKED
struct CN_UNKNOWN_60311
{
	enum { NET_ID = 60311 };
};
POP_PACKED

PUSH_PACKED
struct CA_UNKNOWN_60312
{
	enum { NET_ID = 60312 };
	u16 unkWstrLen;
	wchar unkWstr[1]; // length is unkWstrLen
	// u16 count + count x 4-byte struct elements
	VEC(u8, unkEntryVec); // stride 4B


};
POP_PACKED

PUSH_PACKED
struct CQ_UNKNOWN_60313
{
	enum { NET_ID = 60313 };
	u32 text0_wstring; // 4 bytes (unclassified writer wstring)
	u64 value_u64; // 8 bytes
	u32 text1_wstring; // 4 bytes (unclassified writer wstring)
	u32 text2_wstring; // 4 bytes (unclassified writer wstring)
	u32 ansi_string; // 4 bytes (unclassified writer string)
};
POP_PACKED

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

PUSH_PACKED
struct SN_TgchatServerInfo
{
	enum { NET_ID = 62009 };
	u16 host_len; // 2 bytes
	char host[1]; // variable (ANSI string: 2 + len bytes)
	u16 port; // 2 bytes
	u32 gameID; // 4 bytes
	u32 serverID; // 4 bytes
	u32 userID; // 4 bytes
	u16 gamename_len; // 2 bytes
	wchar_t gamename[1]; // variable (wide string: 2 + len*2 bytes)
	u16 chatname_len; // 2 bytes
	wchar_t chatname[1]; // variable (wide string: 2 + len*2 bytes)
	u16 playncname_len; // 2 bytes
	wchar_t playncname[1]; // variable (wide string: 2 + len*2 bytes)
	u16 signature_count; // 2 bytes
	u8 signature[1]; // variable (byte vector: 2 + count bytes)
	u8 serverType; // 1 bytes (i8)
	// logger 0x9a7060
};
POP_PACKED
ASSERT_SIZE(SN_TgchatServerInfo, 33);
;

PUSH_PACKED
struct SN_DoConnectGameServer
{
	enum { NET_ID = 62010 };
	u16 dwPort; // 2 bytes
	u32 dwIP; // 4 bytes
	u32 dwGameID; // 4 bytes
	u32 idcHash; // 4 bytes
	u16 nickname_len; // 2 bytes
	wchar_t nickname[1]; // 2 bytes
	u32 instantKey; // 4 bytes
	// logger 0x9914f7
};
POP_PACKED
ASSERT_SIZE(SN_DoConnectGameServer, 22);
;

PUSH_PACKED
struct SN_DoConnectChannelServer
{
	enum { NET_ID = 62011 };
	// variable part (ST_HOST_ADDRESS):
	PUSH_PACKED
	struct HostAddress
	{
		u32 ip; // 4 bytes
		u16 port; // 2 bytes
		u16 isp_len; // 2 bytes
		wchar_t isp[1]; // 2*isp_len bytes (wide string)
	};
	POP_PACKED
	u16 addresses_count; // 2 bytes
	HostAddress addresses[1]; // 8 + 2*isp_len bytes each
	u16 nickname_len; // 2 bytes
	wchar_t nickname[1]; // 2*nickname_len bytes (wide string)
	u32 instantKey; // 4 bytes
	u32 reasonCode; // 4 bytes
	// logger 0x991344
};
POP_PACKED
// variable size (VEC of variable-size HostAddress + wide strings) - no ASSERT_SIZE possible
// variable size (VEC of variable-size HostAddress + wide strings) - no ASSERT_SIZE possible
;

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

PUSH_PACKED
struct SA_VersionInfo
{
	enum { NET_ID = 62047 };
	u16 m_VersionName_len; // 2 bytes (wchar count)
	wchar_t m_VersionName[1]; // variable: 2 * m_VersionName_len bytes
	// logger 0x9884cf
};
POP_PACKED
;

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

PUSH_PACKED
struct SN_GamePlayerStock
{
	enum { NET_ID = 62089 };
	u32 playerID; // 4 bytes
	u16 wStrPlayerName_len; // 2 bytes
	wchar_t wStrPlayerName[1]; // 2 bytes (wide)
	u32 m_Class; // 4 bytes
	u32 m_DisplayTitleIDX; // 4 bytes
	u32 m_StatTitleIDX; // 4 bytes
	u8 m_badgeType; // 1 bytes
	u8 m_badgeTierLevel; // 1 bytes
	u16 m_guildTag_len; // 2 bytes
	wchar_t m_guildTag[1]; // 2 bytes (wide)
	u8 m_vipLevel; // 1 bytes
	u8 m_staffType; // 1 bytes
	u8 m_isSubstituted; // 1 bytes (bool)
	// logger 0x995269
};
POP_PACKED
ASSERT_SIZE(SN_GamePlayerStock, 29);
;

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

PUSH_PACKED
struct SN_AccountInfo
{
	enum { NET_ID = 62106 };
	u16 Nickname_len; // 2 bytes
	wchar_t Nickname[1]; // 2 bytes (wide string)
	u32 inventoryLineCountTab0; // 4 bytes
	u32 inventoryLineCountTab1; // 4 bytes
	u32 inventoryLineCountTab2; // 4 bytes
	u32 displayTitleIndex; // 4 bytes
	u32 statTitleIndex; // 4 bytes
	u32 warehouseLineCount; // 4 bytes
	u32 tutorialState; // 4 bytes
	u32 masterGearDurability; // 4 bytes
	u8 badgeType; // 1 bytes
	// logger 0x98ade0
};
POP_PACKED
ASSERT_SIZE(SN_AccountInfo, 37);
;

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

PUSH_PACKED
struct SA_CheckDupNickname
{
	enum { NET_ID = 62121 };
	u32 result; // 4 bytes
	u16 reqNickname_len; // 2 bytes
	wchar_t reqNickname[1]; // 2 bytes (wide string)
	// logger 0x984cb6
};
POP_PACKED
ASSERT_SIZE(SA_CheckDupNickname, 8);
;

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
		ItemUID gearItemID;
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

PUSH_PACKED
struct SN_ChatChannelMessage
{
	enum { NET_ID = 62242 };
	u32 chatType; // 4 bytes
	// variable part (wide string):
	u16 senderNickname_len; // 2 bytes
	wchar_t senderNickname[1]; // 2 bytes
	u8 senderStaffType; // 1 bytes
	// variable part (wide string):
	u16 chatMsg_len; // 2 bytes
	wchar_t chatMsg[1]; // 2 bytes
	// logger 0x98fa3c
};
POP_PACKED
ASSERT_SIZE(SN_ChatChannelMessage, 13);
;

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

PUSH_PACKED
struct SN_Exp
{
	enum { NET_ID = 62278 };
	u32 leaderCSN; // 4 bytes
	u16 nickname_len; // 2 bytes
	wchar_t nickname[1]; // 2 bytes (wide string)
	u8 isLevelUp; // 1 bytes (bool)
	u16 level; // 2 bytes
	u32 currentLevelExp; // 4 bytes
	// logger 0x9924fd
};
POP_PACKED
ASSERT_SIZE(SN_Exp, 15);
;

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
}
;

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

PUSH_PACKED
struct SN_MyGuild
{
	enum { NET_ID = 62330 };
	u16 guildTag_len; // 2 bytes
	wchar_t guildTag[1]; // 2 bytes (wide string, reader 0xa19080)
	i64 dissolutionDate; // 8 bytes
	u8 isFirstTodayRollCall; // 1 bytes (bool)
	// logger 0x99be52
};
POP_PACKED
ASSERT_SIZE(SN_MyGuild, 13);
;

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

PUSH_PACKED
struct SN_GuildChannelEnter
{
	enum { NET_ID = 62358 };
	u16 guildName_len; // 2 bytes
	wchar_t guildName[1]; // 2*N bytes (variable, wide string)
	u16 nickname_len; // 2 bytes
	wchar_t nickname[1]; // 2*N bytes (variable, wide string)
	u8 onlineStatus; // 1 bytes
	// logger 0x9963d6
};
POP_PACKED
;

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

PUSH_PACKED
struct SA_WhisperSend
{
	enum { NET_ID = 62406 };
	u32 retval; // 4 bytes
	// variable part (wide string reader 0xa19080):
	u16 nickname_len; // 2 bytes
	wchar_t nickname[1]; // 2 + count*2
	u16 message_len; // 2 bytes
	wchar_t message[1]; // 2 + count*2
	// logger 0x98a94f
};
POP_PACKED
// variable-size packet: wire = 4 + (2 + 2*nickname_len) + (2 + 2*message_len); no fixed ASSERT_SIZE
// variable-size packet: wire = 4 + (2 + 2*nickname_len) + (2 + 2*message_len); no fixed ASSERT_SIZE
;

PUSH_PACKED
struct SN_WhisperReceive
{
	enum { NET_ID = 62407 };
	u16 nickname_len; // 2 bytes
	wchar_t nickname[1]; // 2 bytes
	u8 staffType; // 1 bytes
	u16 message_len; // 2 bytes
	wchar_t message[1]; // 2 bytes
	// logger 0x009a8914
};
POP_PACKED
ASSERT_SIZE(SN_WhisperReceive, 9);
;

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

PUSH_PACKED
struct SN_LoadingProgressData
{
	enum { NET_ID = 62450 };
	u32 usn; // 4 bytes
	u16 nickname_len; // 2 bytes
	wchar_t nickname[1]; // 2 bytes each (variable: wide string)
	u8 progressData; // 1 bytes
	u32 activeCreatureIndex; // 4 bytes
	u32 inactiveCreatureIndex; // 4 bytes
	u8 isSpectator; // 1 bytes
	// logger 0x99996e
};
POP_PACKED
// ASSERT_SIZE n/a: wire = 16 + 2*nickname_len (variable string)
// ASSERT_SIZE n/a: wire = 16 + 2*nickname_len (variable string)
;

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
	u32 dwIP; // 4 bytes
	u32 idcHash; // 4 bytes
	u16 nickname_len; // 2 bytes
	wchar_t nickname[1]; // 2 bytes (wide string)
	u32 instantKey; // 4 bytes
	// logger 0x991667
};
POP_PACKED
ASSERT_SIZE(SN_DoConnectSpectateServer, 18);
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
	// variable part (ST_SHOP_ITEM_INFO):
	PUSH_PACKED
	struct Item
	{
		u32 m_ItemKey; // 4 bytes
		u32 m_IDX; // 4 bytes
		u32 m_Count; // 4 bytes
		u32 m_Price; // 4 bytes
		u32 UpgradeIndex; // 4 bytes
	};
	POP_PACKED
	u16 m_ShopItem_count; // 2 bytes
	Item m_ShopItem[1]; // 20 bytes
	// logger 0x9a403f
};
POP_PACKED
ASSERT_SIZE(SN_ShopItemList, 22);
PUSH_PACKED
struct SA_SellShopItem
{
	enum { NET_ID = 62023 };
	u32 Result; // 4 bytes
	u16 sellItemDocIndexes_count; // 2 bytes
	u32 sellItemDocIndexes[1]; // 4 bytes
	i64 sellAmount; // 8 bytes
	// logger 0x98836f
};
POP_PACKED
ASSERT_SIZE(SA_SellShopItem, 18);
PUSH_PACKED
struct SN_CreateGroundItem
{
	enum { NET_ID = 62024 };
	u8 excludedFieldBits; // 1 bytes (bit0=!startPos, bit1=!destPos)
	float3 startPos; // 12 bytes (ST_POS3, present if !(bits & 1))
	float3 destPos; // 12 bytes (ST_POS3, present if !(bits & 2))
	GroundItemUID itemID; // 4 bytes
	u32 dropType; // 4 bytes
	u32 nIndex; // 4 bytes
	u32 count; // 4 bytes
	u32 ownerID; // 4 bytes
	i32 gettableType; // 4 bytes
	i32 factionType; // 4 bytes
	u8 isPrivate; // 1 bytes (bool)
	u8 positionType; // 1 bytes
	// logger 0x9900ee
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
	float3 p3nPos; // 12 bytes (ST_POS3: fX, fY, fZ)
	float3 p3nDir; // 12 bytes (ST_POS3)
	u32 SpawnType; // 4 bytes
	u32 actionState; // 4 bytes
	u32 ownerID; // 4 bytes
	u8 bDirectionToNearPC; // 1 bytes (bool)
	u32 nAIWanderDistOverride; // 4 bytes
	u32 tagID; // 4 bytes
	u32 faction; // 4 bytes
	u32 classType; // 4 bytes
	u32 skinIndex; // 4 bytes
	u32 seed; // 4 bytes

	// ST_BASE_STAT: maxStats/curStats (count + count x {u8 type, f32 value})
	PUSH_PACKED
	struct Stat
	{
		u8 type; // 1 bytes
		f32 value; // 4 bytes
	};
	POP_PACKED
	u16 maxStats_count; // 2 bytes
	Stat maxStats[1]; // 5 bytes
	u16 curStats_count; // 2 bytes
	Stat curStats[1]; // 5 bytes
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
	u8 bSyncMyPosition; // 1 bytes (bool)
	// posStruct (PST_SKILL_POS_SYNC):
	PUSH_PACKED
	struct SkillPosSync
	{
		float3 pos; // 12 bytes (ST_POS3: fX, fY, fZ)
		float3 destPos; // 12 bytes (ST_POS3: fX, fY, fZ)
		f32 moveDirX; // 4 bytes (ST_POS2: fX)
		f32 moveDirY; // 4 bytes (ST_POS2: fY)
		float3 rotateStruct; // 12 bytes (ST_POS3: fX, fY, fZ)
		f32 speed; // 4 bytes
		f32 clientTime; // 4 bytes
	};
	POP_PACKED
	SkillPosSync posStruct; // 52 bytes
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
	// variable part (VEC of u32, remoteList):
	u16 remoteList_count; // 2 bytes
	u32 remoteList[1]; // 4 bytes
	u32 param1; // 4 bytes
	// logger 0x9a6b8a
};
POP_PACKED
ASSERT_SIZE(SN_SwitchOnToggleSkill, 22);
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
	float3 curPos; // 12 bytes (ST_POS3: fX, fY, fZ)
	u32 targetID; // 4 bytes
	float3 targetPos; // 12 bytes (ST_POS3: fX, fY, fZ)
	u32 curActionID; // 4 bytes
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
	// killer (PST_DEAD_DAMAGE_INFO_LIST):
	PUSH_PACKED
	struct DeadDamageInfo
	{
		u32 objectID; // 4 bytes
		u32 skillDocIndex; // 4 bytes
		u32 statusDocIndex; // 4 bytes
		u32 damage; // 4 bytes
		u8 attackerIsMonster; // 1 bytes (bool)
		u8 damageType; // 1 bytes
	};
	POP_PACKED
	PUSH_PACKED
	struct DamageInfoList
	{
		u16 deadDamageInfo_count; // 2 bytes
		DeadDamageInfo deadDamageInfo[1]; // 18 bytes
		u32 attackerKey; // 4 bytes
	};
	POP_PACKED
	DamageInfoList killer; // 24 bytes (PST_DEAD_DAMAGE_INFO_LIST)
	// others (VEC of PST_DEAD_DAMAGE_INFO_LIST):
	u16 others_count; // 2 bytes
	DamageInfoList others[1]; // 24 bytes
	u32 durationTime; // 4 bytes
	// logger 0x99088a
};
POP_PACKED
ASSERT_SIZE(SN_DeadDamageInfo, 54);
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
	// variable part (friendsList:VEC of wide strings, wire elem = u16 len + wchar[len]):
	u16 friendsList_count; // 2 bytes
	struct
	{
		u16 len; // 2 bytes
		wchar_t str[1]; // 2 bytes
	} friendsList[1]; // 4 bytes
	// logger 0x982dd1
};
POP_PACKED
ASSERT_SIZE(SA_GiftFriendsAvailable, 10);
PUSH_PACKED
struct SA_GiftFriendCharsAndSkins
{
	enum { NET_ID = 62068 };
	u32 result; // 4 bytes
	u16 nickname_len; // 2 bytes
	wchar_t nickname[1]; // 2 bytes (wide string, reader 0xa19080)
	u16 characterList_count; // 2 bytes
	u32 characterList[1]; // 4 bytes (VEC of u32)
	u16 skinList_count; // 2 bytes
	// variable part (PST_CHARACTER_SKIN):
	PUSH_PACKED
	struct CharacterSkin
	{
		u32 classType; // 4 bytes
		u32 skinIndex; // 4 bytes
	};
	POP_PACKED
	CharacterSkin skinList[1]; // 8 bytes
	// logger 0x982f18
};
POP_PACKED
ASSERT_SIZE(SA_GiftFriendCharsAndSkins, 24);
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
	// stSkillSlot (ST_SKILL_SLOT):
	u32 nSkillIndex; // 4 bytes
	u32 nCoolTime; // 4 bytes
	u8 bUnlocked; // 1 bytes (bool)
	// variable part (ST_SKILL_SLOT properties):
	PUSH_PACKED
	struct SkillProperty
	{
		u32 m_skillPropertyIndex; // 4 bytes
		u32 m_level; // 4 bytes
	};
	POP_PACKED
	u16 properties_count; // 2 bytes
	SkillProperty properties[1]; // 8 bytes
	u8 isUnlocked; // 1 bytes (bool)
	u8 isActivated; // 1 bytes (bool)
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
	// variable part (ST/PST_ITEM_SIMPLE_INFO):
	PUSH_PACKED
	struct ItemSimpleInfo
	{
		ItemUID itemId; // 4 bytes
		ItemDocIndex itemDocIndex; // 4 bytes
		u32 itemCount; // 4 bytes
	};
	POP_PACKED
	u16 addedItemList_count; // 2 bytes
	ItemSimpleInfo addedItemList[1]; // 12 bytes
	// logger 0x983f8d
};
POP_PACKED
ASSERT_SIZE(SA_ItemUse, 30);
PUSH_PACKED
struct SN_PvpResult
{
	enum { NET_ID = 62079 };
	u32 gameEndReason; // 4 bytes
	u32 m_playTime; // 4 bytes
	// pvpResults (ST_PVP_RESULT):
	PUSH_PACKED
	struct PvpResult
	{
		u32 teamType; // 4 bytes
		u32 pvpResultType; // 4 bytes
	};
	POP_PACKED
	u16 pvpResults_count; // 2 bytes
	PvpResult pvpResults[1]; // 8 bytes
	// m_resultReward (PST_RESULT_REWARD, logger LogPST_ResultReward 0x97bb16):
	PUSH_PACKED
	struct ResultReward
	{
		u32 m_playerID; // 4 bytes
		u32 m_baseRon; // 4 bytes
		// m_bonusRon (PST_REWARD_BONUS):
		PUSH_PACKED
		struct RewardBonus
		{
			u32 bonusType; // 4 bytes
			u32 amount; // 4 bytes
		};
		POP_PACKED
		u16 m_bonusRon_count; // 2 bytes
		RewardBonus m_bonusRon[1]; // 8 bytes
		u32 m_baseExp; // 4 bytes
		u16 m_bonusExp_count; // 2 bytes
		RewardBonus m_bonusExp[1]; // 8 bytes
		// rewardItems (ST_REWARD_ITEM):
		PUSH_PACKED
		struct RewardItem
		{
			u32 m_ItemType; // 4 bytes
			u32 m_ItemIndex; // 4 bytes
			u32 m_ItemAmount; // 4 bytes
		};
		POP_PACKED
		u16 rewardItems_count; // 2 bytes
		RewardItem rewardItems[1]; // 12 bytes
		// m_guildPointReward (ST_GUILD_REWARD):
		PUSH_PACKED
		struct GuildReward
		{
			u32 m_baseGuildPoint; // 4 bytes
			u32 m_bonusGuildPoint; // 4 bytes
			u8 m_guildMemberNum; // 1 bytes
			u16 m_boostGoldRate; // 2 bytes
			u16 m_boostExpRate; // 2 bytes
			u16 m_bonusSlotOpen; // 2 bytes
		};
		POP_PACKED
		GuildReward m_guildPointReward; // 15 bytes
		RewardItem m_pcCafeBonusItem; // 12 bytes
	};
	POP_PACKED
	ResultReward m_resultReward; // 73 bytes
	u8 tierGameResult; // 1 bytes
	u8 tierTypeResult; // 1 bytes
	u16 curTierPoint; // 2 bytes
	u16 deltaTierPoint; // 2 bytes
	u16 deltaGuildRp; // 2 bytes
	// pcCafeUserIds (VEC of u32):
	u16 pcCafeUserIds_count; // 2 bytes
	u32 pcCafeUserIds[1]; // 4 bytes
	// logger LogPacket_SN_PvpResult 0x99fd9b
};
POP_PACKED
ASSERT_SIZE(SN_PvpResult, 105);
PUSH_PACKED
struct SN_PvpResultScoreDeathmatch
{
	enum { NET_ID = 62080 };
	// m_TeamScore (ST_PVP_TEAM_SCORE, LogPST_PvpTeamScore 0x9aeec3):
	PUSH_PACKED
	struct TeamScore
	{
		u32 m_Team; // 4 bytes
		u32 m_TotalScore; // 4 bytes
	};
	POP_PACKED
	u16 m_TeamScore_count; // 2 bytes
	TeamScore m_TeamScore[1]; // 8 bytes
	// m_PvPScore (ST_PVP_PLAYER_SCORE_DEATHMATCH, LogPST_PvpPlayerScoreDeathmatch 0x9aded4):
	PUSH_PACKED
	struct PvPScore
	{
		u32 m_playerID; // 4 bytes
		u16 m_nickName_len; // 2 bytes
		wchar_t m_nickName[1]; // 2 bytes (wide string)
		u32 m_teamType; // 4 bytes
		u8 m_Disconnected; // 1 bytes (bool)
		u32 m_TotalScore; // 4 bytes
		// m_charScore (ST_PVP_CHAR_SCORE_DEATHMATCH, LogPST_PvpCharScoreDeathmatch 0x9ad22c):
		PUSH_PACKED
		struct CharScore
		{
			u32 m_CreatureIndex; // 4 bytes
			u32 m_PlayTimeMS; // 4 bytes
			u8 m_KillCount; // 1 bytes
			u8 m_MaxContinuousKillCount; // 1 bytes
			u8 m_MaxMultiKillCount; // 1 bytes
			u8 m_DeadCount; // 1 bytes
			u8 m_AssistCount; // 1 bytes
			u32 m_TotalDamageDealt; // 4 bytes
			u32 m_MaxDamageDealt; // 4 bytes
			u32 m_TotalNormalDamageDealt; // 4 bytes
			u32 m_TotalSkillDamageDealt; // 4 bytes
			u32 m_TotalDamageTaken; // 4 bytes
			u32 m_MaxDamageTaken; // 4 bytes
			u32 m_TotalNormalDamageTaken; // 4 bytes
			u32 m_TotalSkillDamageTaken; // 4 bytes
		};
		POP_PACKED
		u16 m_charScore_count; // 2 bytes
		CharScore m_charScore[1]; // 45 bytes
	};
	POP_PACKED
	u16 m_PvPScore_count; // 2 bytes
	PvPScore m_PvPScore[1]; // 64 bytes
	// logger 0x9a00c6
};
POP_PACKED
ASSERT_SIZE(SN_PvpResultScoreDeathmatch, 76);
PUSH_PACKED
struct SN_PvpResultScoreOccupy
{
	enum { NET_ID = 62081 };
	// variable part (ST_PVP_TEAM_SCORE):
	PUSH_PACKED
	struct TeamScore
	{
		u32 m_Team; // 4 bytes
		u32 m_TotalScore; // 4 bytes
	};
	POP_PACKED
	u16 m_TeamScore_count; // 2 bytes
	TeamScore m_TeamScore[1]; // 8 bytes
	// variable part (ST_PVP_PLAYER_SCORE_OCCUPY):
	PUSH_PACKED
	struct PvPScore
	{
		u32 m_playerID; // 4 bytes
		u16 m_nickName_len; // 2 bytes
		wchar_t m_nickName[1]; // 2 bytes (wide string)
		u32 m_teamType; // 4 bytes
		u8 m_Disconnected; // 1 bytes (bool)
		// variable part (ST_PVP_CHAR_SCORE_OCCUPY):
		PUSH_PACKED
		struct CharScore
		{
			u32 m_CreatureIndex; // 4 bytes
			u32 m_PlayTimeMS; // 4 bytes
			u8 m_KillCount; // 1 bytes
			u8 m_MaxContinuousKillCount; // 1 bytes
			u8 m_MaxMultiKillCount; // 1 bytes
			u8 m_DeadCount; // 1 bytes
			u8 m_AssistCount; // 1 bytes
			u32 m_TotalDamageDealt; // 4 bytes
			u32 m_MaxDamageDealt; // 4 bytes
			u32 m_TotalNormalDamageDealt; // 4 bytes
			u32 m_TotalSkillDamageDealt; // 4 bytes
			u32 m_TotalDamageTaken; // 4 bytes
			u32 m_MaxDamageTaken; // 4 bytes
			u32 m_TotalNormalDamageTaken; // 4 bytes
			u32 m_TotalSkillDamageTaken; // 4 bytes
			u8 m_OccupyCount; // 1 bytes
			u16 m_OccupyScore; // 2 bytes
		};
		POP_PACKED
		u16 m_charScore_count; // 2 bytes
		CharScore m_charScore[1]; // 48 bytes
	};
	POP_PACKED
	u16 m_PvPScore_count; // 2 bytes
	PvPScore m_PvPScore[1]; // 63 bytes
	// logger 0x9a04bf
};
POP_PACKED
ASSERT_SIZE(SN_PvpResultScoreOccupy, 75);
PUSH_PACKED
struct SN_PvpResultScoreGot
{
	enum { NET_ID = 62082 };
	// variable part (ST_PVP_TEAM_SCORE_GOT):
	PUSH_PACKED
	struct TeamScore
	{
		u32 m_Team; // 4 bytes
		u32 m_TotalScore; // 4 bytes
		// variable part (m_TitanSummonList):
		u16 m_TitanSummonList_count; // 2 bytes
		u32 m_TitanSummonList[1]; // 4 bytes
	};
	POP_PACKED
	u16 m_TeamScore_count; // 2 bytes
	TeamScore m_TeamScore[1]; // 14 bytes
	// variable part (ST_PVP_PLAYER_SCORE_GOT):
	PUSH_PACKED
	struct PvPScore
	{
		u32 m_playerID; // 4 bytes
		u16 m_nickName_len; // 2 bytes
		wchar_t m_nickName[1]; // 2 bytes
		u32 m_teamType; // 4 bytes
		u8 m_Disconnected; // 1 bytes (bool)
		u8 m_IngameLevel; // 1 bytes
		u32 m_TotalScore; // 4 bytes
		// variable part (ST_CHARACTER_PROPERTY_INFO):
		PUSH_PACKED
		struct CharPropertyInfo
		{
			u8 propertyIndex; // 1 bytes
			u8 propertyLevel; // 1 bytes
		};
		POP_PACKED
		u16 m_charPropertyInfo1_count; // 2 bytes
		CharPropertyInfo m_charPropertyInfo1[1]; // 2 bytes
		u16 m_charPropertyInfo2_count; // 2 bytes
		CharPropertyInfo m_charPropertyInfo2[1]; // 2 bytes
		// variable part (ST_PVP_CHAR_SCORE_GOT):
		PUSH_PACKED
		struct CharScore
		{
			u32 m_CreatureIndex; // 4 bytes
			u32 m_PlayTimeMS; // 4 bytes
			u8 m_KillCount; // 1 bytes
			u8 m_MaxContinuousKillCount; // 1 bytes
			u8 m_MaxMultiKillCount; // 1 bytes
			u8 m_DeadCount; // 1 bytes
			u8 m_AssistCount; // 1 bytes
			u32 m_TotalDamageDealt; // 4 bytes
			u32 m_MaxDamageDealt; // 4 bytes
			u32 m_TotalNormalDamageDealt; // 4 bytes
			u32 m_TotalSkillDamageDealt; // 4 bytes
			u32 m_TotalBuildingDamageDealt; // 4 bytes
			u32 m_TotalDamageTaken; // 4 bytes
			u32 m_MaxDamageTaken; // 4 bytes
			u32 m_TotalNormalDamageTaken; // 4 bytes
			u32 m_TotalSkillDamageTaken; // 4 bytes
			u32 m_TotalHeal; // 4 bytes
			u8 m_DestroyTowerCount; // 1 bytes
			u8 m_KillBossCount; // 1 bytes
			u8 m_OccupyStatueCount; // 1 bytes
			u8 m_SavePieceCount; // 1 bytes
			u8 m_OccupySightCount; // 1 bytes
			u8 m_KillMinionCount; // 1 bytes
			u8 m_KillNeutralMobCount; // 1 bytes
		};
		POP_PACKED
		u16 m_charScore_count; // 2 bytes
		CharScore m_charScore[1]; // 60 bytes
		u16 m_activeTransformCharScore_count; // 2 bytes
		CharScore m_activeTransformCharScore[1]; // 60 bytes
		u16 m_inActiveTransformCharScore_count; // 2 bytes
		CharScore m_inActiveTransformCharScore[1]; // 60 bytes
	};
	POP_PACKED
	u16 m_PvPScore_count; // 2 bytes
	PvPScore m_PvPScore[1]; // 212 bytes
	// logger 0x9a02ba
};
POP_PACKED
ASSERT_SIZE(SN_PvpResultScoreGot, 230);
PUSH_PACKED
struct SN_PvpResultScoreSport
{
	enum { NET_ID = 62083 };
	// variable part (ST_PVP_TEAM_SCORE):
	PUSH_PACKED
	struct TeamScore
	{
		u32 m_Team; // 4 bytes
		u32 m_TotalScore; // 4 bytes
	};
	POP_PACKED
	u16 m_TeamScore_count; // 2 bytes
	TeamScore m_TeamScore[1]; // 8 bytes
	// variable part (ST_PVP_PLAYER_SCORE_SPORT):
	PUSH_PACKED
	struct PlayerScore
	{
		u32 m_playerID; // 4 bytes
		u16 m_nickName_len; // 2 bytes
		wchar_t m_nickName[1]; // 2 bytes
		u32 m_teamType; // 4 bytes
		u32 m_activeCreatureIndex; // 4 bytes
		u32 m_inactiveCreatureIndex; // 4 bytes
		u8 m_Disconnected; // 1 bytes (bool)
		u32 m_OlympicRecord; // 4 bytes
		char m_Ranking; // 1 bytes
		u8 m_IsFinished; // 1 bytes (bool)
		u32 m_TotalScore; // 4 bytes
	};
	POP_PACKED
	u16 m_PvPScore_count; // 2 bytes
	PlayerScore m_PvPScore[1]; // 31 bytes
	// logger 0x9a06b3
};
POP_PACKED
ASSERT_SIZE(SN_PvpResultScoreSport, 43);
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
	// variable part (ST_MEMBER):
	PUSH_PACKED
	struct Member
	{
		u32 mainClassType; // 4 bytes
		u32 subClassType; // 4 bytes
		u16 nickname_len; // 2 bytes
		wchar_t nickname[1]; // 2 bytes
	};
	POP_PACKED
	u16 member_count; // 2 bytes
	Member member[1]; // 12 bytes
	u8 isRandomRestart; // 1 bytes (bool)
	u32 m_StageIndex; // 4 bytes
	u32 m_KillCount; // 4 bytes
	u32 m_DeathCount; // 4 bytes
	u32 m_PlayTime; // 4 bytes
	u8 m_ClearRank; // 1 bytes
	u32 m_CombatScore; // 4 bytes
	u32 m_baseRon; // 4 bytes
	// variable part (ST_REWARD_BONUS):
	PUSH_PACKED
	struct RewardBonus
	{
		u32 bonusType; // 4 bytes
		u32 amount; // 4 bytes
	};
	POP_PACKED
	u16 m_bonusRon_count; // 2 bytes
	RewardBonus m_bonusRon[1]; // 8 bytes
	u32 m_baseExp; // 4 bytes
	u16 m_bonusExp_count; // 2 bytes
	RewardBonus m_bonusExp[1]; // 8 bytes
	u32 m_attackScore; // 4 bytes
	u32 m_defenseScore; // 4 bytes
	u32 m_timeScore; // 4 bytes
	u32 m_deathTimeScore; // 4 bytes
	u32 m_totalScore; // 4 bytes
	u32 m_ranking; // 4 bytes
	u8 m_technicGrade; // 1 bytes
	// ST_GUILD_REWARD (m_guildPointReward):
	PUSH_PACKED
	struct GuildReward
	{
		u32 m_baseGuildPoint; // 4 bytes
		u32 m_bonusGuildPoint; // 4 bytes
		u8 m_guildMemberNum; // 1 bytes
		u16 m_boostGoldRate; // 2 bytes
		u16 m_boostExpRate; // 2 bytes
		u16 m_bonusSlotOpen; // 2 bytes
	};
	POP_PACKED
	GuildReward m_guildPointReward; // 15 bytes
	// logger 0x9a585a
};
POP_PACKED
ASSERT_SIZE(SN_StageClearResult, 108);
PUSH_PACKED
struct SN_DefenceClearResult
{
	enum { NET_ID = 62087 };
	u32 m_playerID; // 4 bytes
	// variable part (ST_MEMBER):
	PUSH_PACKED
	struct Member
	{
		u32 mainClassType; // 4 bytes
		u32 subClassType; // 4 bytes
		u16 nickname_len; // 2 bytes
		wchar_t nickname[1]; // 2 bytes
	};
	POP_PACKED
	u16 member_count; // 2 bytes
	Member member[1]; // 12 bytes
	u32 m_StageIndex; // 4 bytes
	u32 m_ClearStepIndex; // 4 bytes
	u32 m_ClearTime; // 4 bytes
	// m_pstResultReward (PST_RESULT_REWARD):
	PUSH_PACKED
	struct ResultReward
	{
		u32 m_playerID; // 4 bytes
		u32 m_baseRon; // 4 bytes
		// variable part (PST_REWARD_BONUS):
		PUSH_PACKED
		struct RewardBonus
		{
			u32 bonusType; // 4 bytes
			u32 amount; // 4 bytes
		};
		POP_PACKED
		u16 m_bonusRon_count; // 2 bytes
		RewardBonus m_bonusRon[1]; // 8 bytes
		u32 m_baseExp; // 4 bytes
		u16 m_bonusExp_count; // 2 bytes
		RewardBonus m_bonusExp[1]; // 8 bytes
		// variable part (ST_REWARD_ITEM):
		PUSH_PACKED
		struct RewardItem
		{
			u32 m_ItemType; // 4 bytes
			u32 m_ItemIndex; // 4 bytes
			u32 m_ItemAmount; // 4 bytes
		};
		POP_PACKED
		u16 rewardItems_count; // 2 bytes
		RewardItem rewardItems[1]; // 12 bytes
		// ST_GUILD_REWARD:
		PUSH_PACKED
		struct GuildReward
		{
			u32 m_baseGuildPoint; // 4 bytes
			u32 m_bonusGuildPoint; // 4 bytes
			u8 m_guildMemberNum; // 1 bytes
			u16 m_boostGoldRate; // 2 bytes
			u16 m_boostExpRate; // 2 bytes
			u16 m_bonusSlotOpen; // 2 bytes
		};
		POP_PACKED
		GuildReward m_guildPointReward; // 15 bytes
		RewardItem m_pcCafeBonusItem; // 12 bytes
	};
	POP_PACKED
	ResultReward m_pstResultReward; // 73 bytes
	// variable part (VEC of u32):
	u16 pcCafeUserIds_count; // 2 bytes
	u32 pcCafeUserIds[1]; // 4 bytes
	// logger 0x990a40
};
POP_PACKED
ASSERT_SIZE(SN_DefenceClearResult, 109);
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
	u16 m_SendName_len; // 2 bytes
	wchar_t m_SendName[1]; // 2 bytes
	u8 m_SendStaffType; // 1 bytes
	u16 m_Msg_len; // 2 bytes
	wchar_t m_Msg[1]; // 2 bytes
	u16 m_DurationMS; // 2 bytes
	// logger 0x98feaa
};
POP_PACKED
ASSERT_SIZE(SN_CommunityChat, 19);

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
	GroundItemUID itemID; // 4 bytes
	// logger 0x9911f2
};
POP_PACKED
ASSERT_SIZE(SN_DestroyGroundItem, 4);

PUSH_PACKED
struct SN_GroundItemsSnapshot
{
	enum { NET_ID = 62095 };
	// variable part (ST_GROUND_ITEM):
	PUSH_PACKED
	struct GroundItem
	{
		u32 docIndex; // 4 bytes
		GroundItemUID itemID; // 4 bytes
		u32 dropType; // 4 bytes
		u32 count; // 4 bytes
		u32 ownerID; // 4 bytes
		u32 gettableType; // 4 bytes
		u32 factionType; // 4 bytes
		u8 isPrivate; // 1 bytes (bool)
		char positionType; // 1 bytes
		u8 bFiltered; // 1 bytes (bool)
		float3 pos; // 12 bytes (ST_POS3: fX, fY, fZ)
	};
	POP_PACKED
	u16 items_count; // 2 bytes
	GroundItem items[1]; // 43 bytes
	// logger 0x996290
};
POP_PACKED
ASSERT_SIZE(SN_GroundItemsSnapshot, 45);
PUSH_PACKED
struct SA_WorldSetInfoList
{
	enum { NET_ID = 62096 };
	// variable part (ST_SERVER_SET_INFO):
	PUSH_PACKED
	struct ServerInfo
	{
		u32 m_nWorldID; // 4 bytes
		u32 m_nChannelID; // 4 bytes
		u32 m_nServerID; // 4 bytes
		u32 m_nServerRealID; // 4 bytes
		u32 m_nServerType; // 4 bytes
		u32 m_nIP; // 4 bytes
		u32 m_nPort; // 4 bytes
		u32 m_nState; // 4 bytes
		u32 m_nMaxSessionCount; // 4 bytes
		u32 m_nCurrentSessionCount; // 4 bytes
	};
	POP_PACKED
	u16 m_ServerInfo_count; // 2 bytes
	ServerInfo m_ServerInfo[1]; // 40 bytes
	// logger 0x98aa49
};
POP_PACKED
ASSERT_SIZE(SA_WorldSetInfoList, 42);
PUSH_PACKED
struct SN_AntihackAuth
{
	enum { NET_ID = 62099 };
	// variable part (VEC of PST_CH):
	u16 authKey_count; // 2 bytes
	u8 authKey[1]; // 1 bytes
	// logger 0x98d43f
};
POP_PACKED
ASSERT_SIZE(SN_AntihackAuth, 3);
PUSH_PACKED
struct SN_MissionList
{
	enum { NET_ID = 62103 };
	// variable part (PST_MISSION_INFO):
	PUSH_PACKED
	struct Mission
	{
		u32 index; // 4 bytes
		i64 progressCount; // 8 bytes
		u16 changedCount; // 2 bytes
		u8 isCleared; // 1 bytes (bool)
		u8 isRewarded; // 1 bytes (bool)
	};
	POP_PACKED
	u16 missions_count; // 2 bytes
	Mission missions[1]; // 16 bytes
	u32 dailyMissionRemainTimeMS; // 4 bytes
	u32 weeklyMissionRemainTimeMS; // 4 bytes
	u32 monthlyMissionRemainTimeMS; // 4 bytes
	// logger 0x99b378
};
POP_PACKED
ASSERT_SIZE(SN_MissionList, 30);
PUSH_PACKED
struct SN_MissionUpdate
{
	enum { NET_ID = 62104 };
	// PST_MISSION_INFO (logger 0x97a8c0):
	PUSH_PACKED
	struct MissionInfo
	{
		u32 index; // 4 bytes
		i64 progressCount; // 8 bytes
		u16 changedCount; // 2 bytes
		u8 isCleared; // 1 bytes (bool)
		u8 isRewarded; // 1 bytes (bool)
	};
	POP_PACKED
	MissionInfo mission; // 16 bytes
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
	// mission (PST_MISSION_INFO):
	PUSH_PACKED
	struct MissionInfo
	{
		u32 index; // 4 bytes
		i64 progressCount; // 8 bytes
		u16 changedCount; // 2 bytes
		u8 isCleared; // 1 bytes (bool)
		u8 isRewarded; // 1 bytes (bool)
	};
	POP_PACKED
	MissionInfo mission; // 16 bytes
	// logger 0x97ed26
};
POP_PACKED
ASSERT_SIZE(SA_DailyMissionChange, 24);
PUSH_PACKED
struct SN_ChangedUserGradeInfo
{
	enum { NET_ID = 62108 };
	// userGradeInfo (PST_USER_GRADE_INFO, LogPST_UserGradeInfo 0x97d251):
	PUSH_PACKED
	struct UserGradeInfo
	{
		u8 userGrade; // 1 bytes
		u8 activated; // 1 bytes
		i64 expireDateTime64; // 8 bytes
		u8 level; // 1 bytes
		u16 point; // 2 bytes
		u16 nextPoint; // 2 bytes
	};
	POP_PACKED
	UserGradeInfo userGradeInfo; // 15 bytes
	u32 activityPoint; // 4 bytes
	u8 activityRewardedState; // 1 bytes
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
	GroundItemUID itemID; // 4 bytes
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
	// itemInfo (PST_ITEM_SIMPLE_INFO):
	PUSH_PACKED
	struct ItemInfo
	{
		ItemUID itemId; // 4 bytes
		ItemDocIndex itemDocIndex; // 4 bytes
		u32 itemCount; // 4 bytes
	};
	POP_PACKED
	ItemInfo itemInfo; // 12 bytes
	// logger 0x998c03
};
POP_PACKED
ASSERT_SIZE(SN_ItemAcquisition, 16);
PUSH_PACKED
struct SN_ItemUpdate
{
	enum { NET_ID = 62117 };
	// variable part (m_updatedItems, PST_PROFILE_ITEM):
	PUSH_PACKED
	struct ProfileItem
	{
		ItemUID m_itemID; // 4 bytes
		u8 m_invenType; // 1 bytes
		u32 m_slot; // 4 bytes
		ItemDocIndex m_itemIndex; // 4 bytes
		u32 m_count; // 4 bytes
		u32 m_propertyGroupIndex; // 4 bytes
		u8 m_isLifeTimeAbsolute; // 1 bytes (bool)
		i64 m_lifeEndTimeUTC; // 8 bytes
		// variable part (m_properties, PST_ITEM_PROPERTY):
		PUSH_PACKED
		struct ItemProperty
		{
			u8 Type; // 1 bytes
			u32 TypeDetail; // 4 bytes
			u8 ValueType; // 1 bytes
			f32 Value; // 4 bytes
			u8 Fixed; // 1 bytes (bool)
		};
		POP_PACKED
		u16 m_properties_count; // 2 bytes
		ItemProperty m_properties[1]; // 11 bytes
	};
	POP_PACKED
	u16 m_updatedItems_count; // 2 bytes
	ProfileItem m_updatedItems[1]; // 43 bytes
	u32 context; // 4 bytes
	// logger 0x998f40
};
POP_PACKED
ASSERT_SIZE(SN_ItemUpdate, 49);
PUSH_PACKED
struct SN_UNKNOWN_62118
{
	enum { NET_ID = 62118 };
	u16 str_len; // 2 bytes
	wchar_t str[1]; // 2 bytes
};
POP_PACKED
ASSERT_SIZE(SN_UNKNOWN_62118, 4);

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
	u16 reqNickname_len; // 2 bytes
	wchar_t reqNickname[1]; // 2 bytes (wide string, char count in reqNickname_len)
	// logger 0x984d88
};
POP_PACKED
ASSERT_SIZE(SA_NicknameSet, 8);
PUSH_PACKED
struct SN_ExtraCharacters
{
	enum { NET_ID = 62130 };
	u8 isSelected; // 1 bytes (bool)
	// variable part (PST_COMRADE_PROFILE):
	PUSH_PACKED
	struct ComradeProfile
	{
		u32 leaderID; // 4 bytes
		u16 nickname_len; // 2 bytes
		wchar_t nickname[1]; // 2 bytes
		u32 displayTitleIndex; // 4 bytes
		u32 tier; // 4 bytes
		u8 available; // 1 bytes (bool)
	};
	POP_PACKED
	u16 profiles_count; // 2 bytes
	ComradeProfile profiles[1]; // 17 bytes
	// variable part (PST_PROFILE_CHARACTER):
	PUSH_PACKED
	struct Character
	{
		u32 m_characterID; // 4 bytes
		u32 m_creatureIndex; // 4 bytes
		u32 m_skillSlot1; // 4 bytes
		u32 m_skillSlot2; // 4 bytes
		u32 m_class; // 4 bytes
		f32 m_x; // 4 bytes
		f32 m_y; // 4 bytes
		f32 m_z; // 4 bytes
		u32 m_characterType; // 4 bytes
		u32 m_skinIndex; // 4 bytes
		u32 m_weaponIndex; // 4 bytes
		u8 m_masterGearNo; // 1 bytes
	};
	POP_PACKED
	u16 characters_count; // 2 bytes
	Character characters[1]; // 45 bytes
	// variable part (PST_PROFILE_ITEM):
	PUSH_PACKED
	struct ItemProperty
	{
		u8 Type; // 1 bytes
		u32 TypeDetail; // 4 bytes
		u8 ValueType; // 1 bytes
		f32 Value; // 4 bytes
		u8 Fixed; // 1 bytes
	};
	POP_PACKED
	PUSH_PACKED
	struct ProfileItem
	{
		ItemUID m_itemID; // 4 bytes
		u8 m_invenType; // 1 bytes
		u32 m_slot; // 4 bytes
		ItemDocIndex m_itemIndex; // 4 bytes
		u32 m_count; // 4 bytes
		u32 m_propertyGroupIndex; // 4 bytes
		u8 m_isLifeTimeAbsolute; // 1 bytes (bool)
		i64 m_lifeEndTimeUTC; // 8 bytes
		u16 m_properties_count; // 2 bytes
		ItemProperty m_properties[1]; // 11 bytes
	};
	POP_PACKED
	u16 items_count; // 2 bytes
	ProfileItem items[1]; // 43 bytes
	// variable part (PST_PROFILE_WEAPON):
	PUSH_PACKED
	struct ProfileWeapon
	{
		u32 characterID; // 4 bytes
		u32 weaponType; // 4 bytes
		u32 weaponIndex; // 4 bytes
		u32 grade; // 4 bytes
		u8 isUnlocked; // 1 bytes (bool)
		u8 isActivated; // 1 bytes (bool)
	};
	POP_PACKED
	u16 weapons_count; // 2 bytes
	ProfileWeapon weapons[1]; // 18 bytes
	// variable part (PST_PROFILE_SKILL):
	PUSH_PACKED
	struct SkillProperty
	{
		u32 m_skillPropertyIndex; // 4 bytes
		u32 m_level; // 4 bytes
	};
	POP_PACKED
	PUSH_PACKED
	struct ProfileSkill
	{
		u32 m_characterID; // 4 bytes
		u32 m_skillIndex; // 4 bytes
		u8 isUnlocked; // 1 bytes (bool)
		u8 isActivated; // 1 bytes (bool)
		u16 m_properties_count; // 2 bytes
		SkillProperty m_properties[1]; // 8 bytes
	};
	POP_PACKED
	u16 skills_count; // 2 bytes
	ProfileSkill skills[1]; // 20 bytes
	// variable part (PST_PROFILE_MASTERGEAR):
	PUSH_PACKED
	struct MasterGearSlot
	{
		u32 gearType; // 4 bytes
		ItemUID gearItemID; // 4 bytes
	};
	POP_PACKED
	PUSH_PACKED
	struct ProfileMasterGear
	{
		u8 masterGearNo; // 1 bytes
		u16 name_len; // 2 bytes
		wchar_t name[1]; // 2 bytes
		u16 slots_count; // 2 bytes
		MasterGearSlot slots[1]; // 8 bytes
	};
	POP_PACKED
	u16 masterGears_count; // 2 bytes
	ProfileMasterGear masterGears[1]; // 15 bytes
	// logger 0x992814
};
POP_PACKED
ASSERT_SIZE(SN_ExtraCharacters, 171);
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
	// skill (PST_PROFILE_SKILL):
	PUSH_PACKED
	struct ProfileSkill
	{
		u32 m_characterID; // 4 bytes
		u32 m_skillIndex; // 4 bytes
		u8 isUnlocked; // 1 bytes (bool)
		u8 isActivated; // 1 bytes (bool)
		// variable part (PST_PROFILE_SKILL_PROPERTY):
		PUSH_PACKED
		struct ProfileSkillProperty
		{
			u32 m_skillPropertyIndex; // 4 bytes
			u32 m_level; // 4 bytes
		};
		POP_PACKED
		u16 m_properties_count; // 2 bytes
		ProfileSkillProperty m_properties[1]; // 8 bytes
	};
	POP_PACKED
	ProfileSkill skill; // 20 bytes
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
	// mainSkill (PST_PROFILE_SKILL):
	PUSH_PACKED
	struct ProfileSkill
	{
		u32 m_characterID; // 4 bytes
		u32 m_skillIndex; // 4 bytes
		u8 isUnlocked; // 1 bytes (bool)
		u8 isActivated; // 1 bytes (bool)
		// variable part:
		PUSH_PACKED
		struct ProfileSkillProperty
		{
			u32 m_skillPropertyIndex; // 4 bytes
			u32 m_level; // 4 bytes
		};
		POP_PACKED
		u16 m_properties_count; // 2 bytes
		ProfileSkillProperty m_properties[1]; // 8 bytes
	};
	POP_PACKED
	ProfileSkill mainSkill; // 20 bytes (PST_PROFILE_SKILL, incl. m_properties[1])
	u32 subID; // 4 bytes
	ProfileSkill subSkill; // 20 bytes (PST_PROFILE_SKILL, incl. m_properties[1])
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
	// newWeapon (PST_PROFILE_WEAPON):
	PUSH_PACKED
	struct ProfileWeapon
	{
		u32 characterID; // 4 bytes
		u32 weaponType; // 4 bytes
		u32 weaponIndex; // 4 bytes
		u32 grade; // 4 bytes
		u8 isUnlocked; // 1 bytes (bool)
		u8 isActivated; // 1 bytes (bool)
	};
	POP_PACKED
	ProfileWeapon newWeapon; // 18 bytes
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
	ItemUID gearItemID; // 4 bytes
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
	u16 newName_len; // 2 bytes (wide string count)
	wchar_t newName[1]; // 2 bytes
	// logger 0x98453e
};
POP_PACKED
ASSERT_SIZE(SA_MastergearRename, 9);
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
	// newMasterGear (PST_PROFILE_MASTERGEAR):
	PUSH_PACKED
	struct MasterGear
	{
		u8 masterGearNo; // 1 bytes
		u16 name_len; // 2 bytes
		wchar_t name[1]; // 2 bytes (wide string)
		u16 slots_count; // 2 bytes
		// variable part (vector of PST_PROFILE_MASTERGEAR_SLOT):
		PUSH_PACKED
		struct Slot
		{
			u32 gearType; // 4 bytes
			ItemUID gearItemID; // 4 bytes
		};
		POP_PACKED
		Slot slots[1]; // 8 bytes
	};
	POP_PACKED
	MasterGear newMasterGear; // 15 bytes
	// logger 0x98444a
};
POP_PACKED
ASSERT_SIZE(SA_MastergearAdd, 19);
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
	// variable part (PST_PROFILE_SKILL):
	PUSH_PACKED
	struct ProfileSkill
	{
		u32 m_characterID; // 4 bytes
		u32 m_skillIndex; // 4 bytes
		u8 isUnlocked; // 1 bytes (bool)
		u8 isActivated; // 1 bytes (bool)
		// variable part (PST_PROFILE_SKILL_PROPERTY):
		PUSH_PACKED
		struct Property
		{
			u32 m_skillPropertyIndex; // 4 bytes
			u32 m_level; // 4 bytes
		};
		POP_PACKED
		u16 m_properties_count; // 2 bytes
		Property m_properties[1]; // 8 bytes
	};
	POP_PACKED
	u16 newSkills_count; // 2 bytes
	ProfileSkill newSkills[1]; // 20 bytes
	// logger 0x988d86
};
POP_PACKED
ASSERT_SIZE(SA_SkillUnlock, 26);
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
	// variable part (ST_SORTIE_USERINFO):
	PUSH_PACKED
	struct SortieUserInfo
	{
		u32 userId; // 4 bytes
		u16 nickname_len; // 2 bytes
		wchar_t nickname[1]; // 2 bytes
		u8 isBot; // 1 bytes (bool)
		u32 tier; // 4 bytes
		u32 tierGroupRanking; // 4 bytes
		u32 tierSeriesFlag; // 4 bytes
		f32 pvpRate; // 4 bytes
	};
	POP_PACKED
	u16 m_allies_count; // 2 bytes
	SortieUserInfo m_allies[1]; // 25 bytes
	u16 voteTimeMS; // 2 bytes
	// logger 0x9a2be4
};
POP_PACKED
ASSERT_SIZE(SN_RestartVoteStart, 45);
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
	// variable part (voteList): vector of u8 votes
	u16 voteList_count; // 2 bytes
	u8 voteList[1]; // 1 bytes
	// logger 0x9a2df4
};
POP_PACKED
ASSERT_SIZE(SN_RestartVoteUpdate, 3);
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
	u16 nickname_len; // 2 bytes
	wchar_t nickname[1]; // 2 bytes
	u8 isJoined; // 1 bytes (bool)
	// logger 0x9a28c4
};
POP_PACKED
ASSERT_SIZE(SN_RestartPvpEntryUpdate, 5);
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
	u16 nickname_len; // 2 bytes
	wchar_t nickname[1]; // 2 bytes (wide string, reader 0xa19080: u16 count + count*2 wchar payload)
	// logger 0x98540f
};
POP_PACKED
ASSERT_SIZE(SA_PartyInvite, 8);
PUSH_PACKED
struct SA_PartyInviteRecommend
{
	enum { NET_ID = 62181 };
	u32 retval; // 4 bytes
	u16 inviteeNickname_len; // 2 bytes
	wchar_t inviteeNickname[1]; // 2 bytes (wide string, reader 0xa19080: u16 count + count*2 wchar payload)
	// logger 0x9854e1
};
POP_PACKED
ASSERT_SIZE(SA_PartyInviteRecommend, 8);
PUSH_PACKED
struct SN_PartyInviteResponse
{
	enum { NET_ID = 62182 };
	// account: wide string (validator helper 0xa19080: u16 count, then count*2 wchar bytes)
	u16 account_len; // 2 bytes
	wchar_t account[1]; // 2 bytes each
	u8 accept; // 1 bytes (bool)
	// logger 0x99d10b
};
POP_PACKED
ASSERT_SIZE(SN_PartyInviteResponse, 5);
PUSH_PACKED
struct SN_PartyInvite
{
	enum { NET_ID = 62183 };
	u32 channelID; // 4 bytes
	// inviterNick (wide string):
	u16 inviterNick_len; // 2 bytes
	wchar_t inviterNick[1]; // 2 bytes
	// stageIndexes (VEC of u32):
	u16 stageIndexes_count; // 2 bytes
	u32 stageIndexes[1]; // 4 bytes
	u32 gameType; // 4 bytes
	u32 gameDefinitionType; // 4 bytes
	u32 stageRule; // 4 bytes
	u32 partyID; // 4 bytes
	u32 teamType; // 4 bytes
	// logger 0x99cd47
};
POP_PACKED
ASSERT_SIZE(SN_PartyInvite, 34);
PUSH_PACKED
struct SN_PartyInviteRecommend
{
	enum { NET_ID = 62184 };
	u16 inviterNickname_len; // 2 bytes
	wchar_t inviterNickname[1]; // 2 bytes (wide string, reader 0xa19080: u16 count + count*2 wchar payload)
	u16 inviteeNickname_len; // 2 bytes
	wchar_t inviteeNickname[1]; // 2 bytes (wide string, reader 0xa19080: u16 count + count*2 wchar payload)
	// logger 0x99cf68
};
POP_PACKED
ASSERT_SIZE(SN_PartyInviteRecommend, 8);
PUSH_PACKED
struct SA_PartyInviteRecommendResponse
{
	enum { NET_ID = 62185 };
	u32 retval; // 4 bytes
	u16 inviteeNickname_len; // 2 bytes
	wchar_t inviteeNickname[1]; // 2 bytes (wide string, reader 0xa19080: u16 count + count*2 wchar payload)
	// logger 0x9855b3
};
POP_PACKED
ASSERT_SIZE(SA_PartyInviteRecommendResponse, 8);
PUSH_PACKED
struct SN_PartyInviteRecommendResponse
{
	enum { NET_ID = 62186 };
	// inviteeNickname (wide string):
	u16 inviteeNickname_len; // 2 bytes
	wchar_t inviteeNickname[1]; // 2 bytes
	u8 accept; // 1 bytes (bool)
	// logger 0x99d039
};
POP_PACKED
ASSERT_SIZE(SN_PartyInviteRecommendResponse, 5);
PUSH_PACKED
struct SA_PartyJoin
{
	enum { NET_ID = 62187 };
	u32 retval; // 4 bytes
	u32 partyID; // 4 bytes
	u32 stageType; // 4 bytes
	u32 gameType; // 4 bytes
	u32 gameDefinitionType; // 4 bytes
	// variable part (ST_SORTIE_USERINFO):
	PUSH_PACKED
	struct SortieUserInfo
	{
		u32 userId; // 4 bytes
		u16 nickname_len; // 2 bytes
		wchar_t nickname[1]; // 2 bytes
		u8 isBot; // 1 bytes (bool)
		u32 tier; // 4 bytes
		u32 tierGroupRanking; // 4 bytes
		u32 tierSeriesFlag; // 4 bytes
		u32 pvpRate; // 4 bytes
	};
	POP_PACKED
	u16 m_allies_count; // 2 bytes
	SortieUserInfo m_allies[1]; // 23 + 2*nickname_len bytes
	u32 alliesTeamType; // 4 bytes
	u16 m_enemies_count; // 2 bytes
	SortieUserInfo m_enemies[1]; // 23 + 2*nickname_len bytes
	u32 enemiesTeamType; // 4 bytes
	u16 m_spectators_count; // 2 bytes
	SortieUserInfo m_spectators[1]; // 23 + 2*nickname_len bytes
	u8 isSpectator; // 1 bytes (bool)
	// logger 0x985685 (LogPacket_SN_PartyJoin)
};
POP_PACKED
// variable size: 29 fixed + 3 * (2 + n * (23 + 2*nickname_len)) — no static assert
// variable size: 29 fixed + 3 * (2 + n * (23 + 2*nickname_len)) — no static assert
PUSH_PACKED
struct SN_PartyJoin
{
	enum { NET_ID = 62188 };
	u32 userId; // 4 bytes
	u16 username_len; // 2 bytes
	wchar_t username[1]; // 2 bytes
	u8 isBot; // 1 bytes (bool)
	u32 creatureIndex; // 4 bytes
	u8 isOwner; // 1 bytes (bool)
	u32 gameType; // 4 bytes
	u32 gameDefinitionType; // 4 bytes
	u32 teamType; // 4 bytes
	u8 teamSlotIndex; // 1 bytes
	// logger 0x99d1dd
};
POP_PACKED
ASSERT_SIZE(SN_PartyJoin, 27);
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
	u16 username_len; // 2 bytes
	wchar_t username[1]; // 2 bytes (wide string)
	u8 isOwner; // 1 bytes (bool)
	// logger 0x99d497
};
POP_PACKED
ASSERT_SIZE(SN_PartyLeave, 9);
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
	// partyInfo (ST_MODIFY_PARTYINFO):
	PUSH_PACKED
	struct ModifyPartyInfo
	{
		u16 stageIndexes_count; // 2 bytes
		u32 stageIndexes[1]; // 4 bytes (VEC)
		u32 gameType; // 4 bytes
		u32 gameDefinitionMode; // 4 bytes
		u8 selectSortieMasterType; // 1 bytes
	};
	POP_PACKED
	ModifyPartyInfo partyInfo; // 15 bytes (with 1 stageIndex)
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
	u16 channelName_len; // 2 bytes
	wchar_t channelName[1]; // 2 bytes
	u16 password_len; // 2 bytes
	wchar_t password[1]; // 2 bytes
	// logger 0x98f0c1
};
POP_PACKED
ASSERT_SIZE(SN_ChannelChattingChannel, 12);
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
	// variable part (ST_MASTER_SELECT_INFO):
	PUSH_PACKED
	struct MasterSelectInfo
	{
		u32 characterID; // 4 bytes
		u32 creatureIndex; // 4 bytes
		u32 skillSlot1; // 4 bytes
		u32 skillSlot2; // 4 bytes
	};
	POP_PACKED
	u16 m_characterSelectInfos_count; // 2 bytes
	MasterSelectInfo m_characterSelectInfos[1]; // 16 bytes
	// logger 0x99ae22
};
POP_PACKED
ASSERT_SIZE(SN_MasterUnpick, 22);
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
	// variable part (PST_MASTER_SELECT_INFO):
	PUSH_PACKED
	struct MasterSelectInfo
	{
		u32 characterID; // 4 bytes
		u32 creatureIndex; // 4 bytes
		u32 skillSlot1; // 4 bytes
		u32 skillSlot2; // 4 bytes
	};
	POP_PACKED
	u16 masters_count; // 2 bytes
	MasterSelectInfo masters[1]; // 16 bytes
	// logger 0x99af91
};
POP_PACKED
ASSERT_SIZE(SN_MasterUnpickAll, 22);
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
	// weaponInfo (PST_PROFILE_WEAPON):
	PUSH_PACKED
	struct PST_ProfileWeapon
	{
		u32 characterID; // 4 bytes
		u32 weaponType; // 4 bytes
		u32 weaponIndex; // 4 bytes
		u32 grade; // 4 bytes
		u8 isUnlocked; // 1 bytes (bool)
		u8 isActivated; // 1 bytes (bool)
	};
	POP_PACKED
	PST_ProfileWeapon weaponInfo; // 18 bytes
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
	// variable part (u32 vector, count=u16):
	u16 clearIDs_count; // 2 bytes
	u32 clearIDs[1]; // 4 bytes
	// logger 0x99103e
};
POP_PACKED
ASSERT_SIZE(SN_DespawnMonsters, 6);
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
	// variable part (LogPST_SummaryInfo):
	PUSH_PACKED
	struct SummaryInfo
	{
		u32 summaryIndex; // 4 bytes
		u32 stageIndex; // 4 bytes
		u32 summaryType; // 4 bytes
		u32 summaryData; // 4 bytes
		u8 rewardReceived; // 1 bytes (bool)
		i64 completedDate; // 8 bytes
	};
	POP_PACKED
	u16 summaryInfoList_count; // 2 bytes
	SummaryInfo summaryInfoList[1]; // 25 bytes
	// logger 0x986e4a
};
POP_PACKED
ASSERT_SIZE(SA_RequestSummaryInfoEach, 31);
PUSH_PACKED
struct SN_SummaryUpdate
{
	enum { NET_ID = 62228 };
	// summaryInfo (PST_SUMMARY_INFO, logger 0x9a68a6 -> LogPST_SummaryInfo):
	PUSH_PACKED
	struct SummaryInfo
	{
		u32 summaryIndex; // 4 bytes
		u32 stageIndex; // 4 bytes
		u32 summaryType; // 4 bytes
		u32 summaryData; // 4 bytes
		u8 rewardReceived; // 1 bytes (bool)
		i64 completedDate; // 8 bytes
	};
	POP_PACKED
	SummaryInfo summaryInfo; // 25 bytes
	// logger 0x9a68a6
};
POP_PACKED
ASSERT_SIZE(SN_SummaryUpdate, 25);
PUSH_PACKED
struct SN_AddStatus
{
	enum { NET_ID = 62230 };
	u8 excludedFieldBits; // 1 bytes (bit0=!extraGraphInfo)
	// extraGraphInfo (PST_STATUS_GRAPH_DETAIL, present if !(bits & 1)):
	PUSH_PACKED
	struct StatusGraphDetail
	{
		f32 totalRatio; // 4 bytes
		float3 defeatFrom; // 12 bytes
		float3 defeatTo; // 12 bytes
		f32 defeatDist; // 4 bytes
		f32 defeatOriginDist; // 4 bytes
		f32 defeatDurationTime; // 4 bytes
	};
	POP_PACKED
	StatusGraphDetail extraGraphInfo; // 40 bytes
	u32 statusID; // 4 bytes
	u8 bEnabled; // 1 bytes (bool)
	u32 targetID; // 4 bytes
	u32 casterID; // 4 bytes
	u8 isOverlap; // 1 bytes (bool)
	u8 overlapCount; // 1 bytes
	u8 customValue; // 1 bytes
	f32 durationTime; // 4 bytes
	f32 elapsedTime; // 4 bytes
	// logger 0x98b802
};
POP_PACKED
ASSERT_SIZE(SN_AddStatus, 65);

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
	float3 remotePos; // 12 bytes (ST_POS3: fX, fY, fZ)
	float3 remoteDir; // 12 bytes (ST_POS3: fX, fY, fZ)
	float3 remoteForceDir; // 12 bytes (ST_POS3: fX, fY, fZ)
	float3 hitPos; // 12 bytes (ST_POS3: fX, fY, fZ)
	float3 hitDir; // 12 bytes (ST_POS3: fX, fY, fZ)
	u32 damageType; // 4 bytes
	u32 skillDocID; // 4 bytes
	u32 nRagePoint; // 4 bytes
	u32 nRelativeElement; // 4 bytes
	u16 hitNodeName_len; // 2 bytes
	char hitNodeName[1]; // 1 bytes (ANSI string)
	u32 damage; // 4 bytes
	u16 partName_len; // 2 bytes
	wchar_t partName[1]; // 2 bytes (wide string)
	u32 partDamage; // 4 bytes
	u32 masterGroupingDamage; // 4 bytes
	u32 optionalResultOfHit; // 4 bytes
	// logger 0x98da4c
};
POP_PACKED
ASSERT_SIZE(SN_BroadcastDamage, 119);
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
	float3 remotePos; // 12 bytes (ST_POS3: fX, fY, fZ)
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
	u16 chatChannelName_len; // 2 bytes
	wchar_t chatChannelName[1]; // 2 bytes
	u16 password_len; // 2 bytes
	wchar_t password[1]; // 2 bytes
	// logger 0x98f841
};
POP_PACKED
ASSERT_SIZE(SN_ChatChannelJoin, 12);
PUSH_PACKED
struct SN_ChatChannelLeave
{
	enum { NET_ID = 62241 };
	u32 channelType; // 4 bytes
	u16 chatChannelName_len; // 2 bytes
	wchar_t chatChannelName[1]; // 2 bytes (wide string, reader 0xa19080: u16 count + count*2 wchar payload)
	// logger 0x98f93b
};
POP_PACKED
ASSERT_SIZE(SN_ChatChannelLeave, 8);
PUSH_PACKED
struct SN_ChatChannelUserJoin
{
	enum { NET_ID = 62243 };
	u16 channelName_len; // 2 bytes
	wchar_t channelName[1]; // 2 bytes (wide string)
	u16 nickname_len; // 2 bytes
	wchar_t nickname[1]; // 2 bytes (wide string)
	// logger 0x98fb30
};
POP_PACKED



PUSH_PACKED
struct SN_ChatChannelUserLeave
{
	enum { NET_ID = 62244 };
	// channelName: wide string
	u16 channelName_len; // 2 bytes
	wchar_t channelName[1]; // 2 bytes
	// nickname: wide string
	u16 nickname_len; // 2 bytes
	wchar_t nickname[1]; // 2 bytes
	// logger 0x98fc01
};
POP_PACKED
ASSERT_SIZE(SN_ChatChannelUserLeave, 8);
PUSH_PACKED
struct SN_FriendAdded
{
	enum { NET_ID = 62245 };
	// newFriendInfo (ST_FRIEND_USERINFO):
	PUSH_PACKED
	struct FriendUserinfo
	{
		u16 nickname_len; // 2 bytes
		wchar_t nickname[1]; // 2 bytes
		u32 leaderCreatureIndex; // 4 bytes
		u8 state; // 1 bytes
		u16 level; // 2 bytes
		u8 isFavorite; // 1 bytes (bool)
		u8 isComrade; // 1 bytes (bool)
		u8 isOnLeavePenalty; // 1 bytes (bool)
		i64 lastLogoutDate; // 8 bytes
	};
	POP_PACKED
	FriendUserinfo newFriendInfo; // 22 bytes
	// logger 0x992deb
};
POP_PACKED
ASSERT_SIZE(SN_FriendAdded, 22);
PUSH_PACKED
struct SA_FriendRemove
{
	enum { NET_ID = 62246 };
	u32 retval; // 4 bytes
	// targetNickname: variable-size (wide string)
	u16 targetNickname_len; // 2 bytes
	wchar_t targetNickname[1]; // 2 bytes
	// logger 0x97fdbc
};
POP_PACKED
ASSERT_SIZE(SA_FriendRemove, 8);
PUSH_PACKED
struct SA_CommunityBlockAdd
{
	enum { NET_ID = 62247 };
	u32 retval; // 4 bytes
	// userinfo (ST_BLOCK_USERINFO: nickname wide string, wire = 2 + 2*len):
	PUSH_PACKED
	struct UserInfo
	{
		u16 nickname_len; // 2 bytes
		wchar_t nickname[1]; // 2 bytes
	};
	POP_PACKED
	UserInfo userinfo; // variable tail: 2 + 2*len bytes
	// logger 0x97e70d
};
POP_PACKED
// variable-length (wide string) — fixed prefix 6 bytes; ASSERT_SIZE not applicable
// variable-length (wide string) — fixed prefix 6 bytes; ASSERT_SIZE not applicable
PUSH_PACKED
struct SA_CommunityBlockRemove
{
	enum { NET_ID = 62248 };
	u32 retval; // 4 bytes
	u16 userinfo_len; // 2 bytes
	wchar_t userinfo[1]; // 2 bytes (wide string, wire = 2 + count*2)
	// logger 0x97e801
};
POP_PACKED
ASSERT_SIZE(SA_CommunityBlockRemove, 8);
PUSH_PACKED
struct SN_FriendRemoved
{
	enum { NET_ID = 62249 };
	// variable part (wide string):
	u16 nickname_len; // 2 bytes
	wchar_t nickname[1]; // 2 bytes each
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
	// nickname: variable-size (wide string)
	u16 nickname_len; // 2 bytes
	wchar_t nickname[1]; // 2 bytes
	u8 newState; // 1 bytes
	u8 isOnLeavePenalty; // 1 bytes (bool)
	i64 stateUpdatedTime; // 8 bytes
	// logger 0x993646
};
POP_PACKED
ASSERT_SIZE(SN_FriendStateChanged, 14);
PUSH_PACKED
struct SN_FriendNicknameChanged
{
	enum { NET_ID = 62252 };
	// variable part (wide string):
	u16 oldNickname_len; // 2 bytes
	wchar_t oldNickname[1]; // 2 bytes
	u16 newNickname_len; // 2 bytes
	wchar_t newNickname[1]; // 2 bytes
	// logger 0x9930ca
};
POP_PACKED
ASSERT_SIZE(SN_FriendNicknameChanged, 8);
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
	// nickname (wide string):
	u16 nickname_len; // 2 bytes
	wchar_t nickname[1]; // 2 bytes
	// msg (wide string):
	u16 msg_len; // 2 bytes
	wchar_t msg[1]; // 2 bytes
	// logger 0x99001d
};
POP_PACKED
ASSERT_SIZE(SN_CommunityStatusMessage, 8);
PUSH_PACKED
struct SN_FriendLeaderChanged
{
	enum { NET_ID = 62256 };
	u16 nickname_len; // 2 bytes
	wchar_t nickname[1]; // 2 bytes (wide string, reader 0xa19080: u16 count + count*2 wchar payload)
	u32 creatureIndex; // 4 bytes
	// logger 0x992eb4
};
POP_PACKED
ASSERT_SIZE(SN_FriendLeaderChanged, 8);
PUSH_PACKED
struct SN_RecommendedfriendList
{
	enum { NET_ID = 62260 };
	// variable part (ST_FRIENDCANDIDATE_INFO):
	PUSH_PACKED
	struct Candidate
	{
		u16 nickname_len; // 2 bytes
		wchar_t nickname[1]; // 2 bytes (wide string, wire = 2 + len*2)
		u32 leaderCreatureIndex; // 4 bytes
	};
	POP_PACKED
	u16 candidates_count; // 2 bytes
	Candidate candidates[1]; // 8 bytes (min element)
	// logger 0x9a097a
};
POP_PACKED
ASSERT_SIZE(SN_RecommendedfriendList, 10);
PUSH_PACKED
struct SA_FriendRequest
{
	enum { NET_ID = 62262 };
	u32 retval; // 4 bytes
	u16 nickname_len; // 2 bytes
	wchar_t nickname[1]; // 2 bytes
	// logger 0x97fe8e
};
POP_PACKED
ASSERT_SIZE(SA_FriendRequest, 8);
PUSH_PACKED
struct SN_FriendRequested
{
	enum { NET_ID = 62263 };
	// userInfo (ST_FRIEND_REQUEST_INFO, LogPST_FriendRequestInfo 0x9a9bdd):
	PUSH_PACKED
	struct FriendRequestInfo
	{
		u16 nickname_len; // 2 bytes (wchar count)
		wchar_t nickname[1]; // 2 bytes (wide string, 2*nickname_len bytes on wire)
		u32 leaderCreatureIndex; // 4 bytes
	};
	POP_PACKED
	FriendRequestInfo userInfo; // 8 bytes
	// logger 0x993365
};
POP_PACKED
ASSERT_SIZE(SN_FriendRequested, 8);
PUSH_PACKED
struct SA_FriendRespond
{
	enum { NET_ID = 62264 };
	u32 retval; // 4 bytes
	// nickname: wide string (u16 char count + wchar payload, 2 + count*2)
	u16 nickname_len; // 2 bytes
	wchar_t nickname[1]; // 2 bytes
	u8 bAccept; // 1 bytes (bool)
	// logger 0x980054
};
POP_PACKED
ASSERT_SIZE(SA_FriendRespond, 9);
PUSH_PACKED
struct SN_FriendResponded
{
	enum { NET_ID = 62265 };
	u16 nickname_len; // 2 bytes
	wchar_t nickname[1]; // 2 bytes
	u8 accept; // 1 bytes
	// logger 0x993574
};
POP_PACKED
ASSERT_SIZE(SN_FriendResponded, 5);
PUSH_PACKED
struct SA_FriendRequestLeaderInfo
{
	enum { NET_ID = 62266 };
	u32 retval; // 4 bytes
	// charinfo (PST_LEADER_INFO):
	PUSH_PACKED
	struct LeaderInfo
	{
		u16 nickname_len; // 2 bytes
		wchar_t nickname[1]; // 2*len bytes (wide string)
		u16 statusMessage_len; // 2 bytes
		wchar_t statusMessage[1]; // 2*len bytes (wide string)
		u16 level; // 2 bytes
		u32 creatureIndex; // 4 bytes
		u32 power; // 4 bytes
		u32 viability; // 4 bytes
		u32 tierType; // 4 bytes
		// variable part (PST_PROFILE_SKILL):
		PUSH_PACKED
		struct Skill
		{
			u32 m_characterID; // 4 bytes
			u32 m_skillIndex; // 4 bytes
			u8 isUnlocked; // 1 bytes (bool)
			u8 isActivated; // 1 bytes (bool)
			// variable part (PST_PROFILE_SKILL_PROPERTY):
			PUSH_PACKED
			struct Property
			{
				u32 m_skillPropertyIndex; // 4 bytes
				u32 m_level; // 4 bytes
			};
			POP_PACKED
			u16 m_properties_count; // 2 bytes
			Property m_properties[1]; // 8 bytes
		};
		POP_PACKED
		u16 skills_count; // 2 bytes
		Skill skills[1]; // 14 bytes
	};
	POP_PACKED
	LeaderInfo charinfo; // variable-size (strings + nested VECs)
	// logger 0x97ff60
};
POP_PACKED
PUSH_PACKED
struct SA_RequestLeaderInfo
{
	enum { NET_ID = 62267 };
	u32 retval; // 4 bytes
	// charinfo (PST_LEADER_INFO):
	PUSH_PACKED
	struct LeaderInfo
	{
		u16 nickname_len; // 2 bytes
		wchar_t nickname[1]; // 2 bytes (wide string)
		u16 statusMessage_len; // 2 bytes
		wchar_t statusMessage[1]; // 2 bytes (wide string)
		u16 level; // 2 bytes
		u32 creatureIndex; // 4 bytes
		u32 power; // 4 bytes
		u32 viability; // 4 bytes
		u32 tierType; // 4 bytes
		u16 skills_count; // 2 bytes
		// variable part (PST_PROFILE_SKILL):
		PUSH_PACKED
		struct Skill
		{
			u32 m_characterID; // 4 bytes
			u32 m_skillIndex; // 4 bytes
			u8 isUnlocked; // 1 bytes (bool)
			u8 isActivated; // 1 bytes (bool)
			u16 m_properties_count; // 2 bytes
			// variable part (PST_PROFILE_SKILL_PROPERTY):
			PUSH_PACKED
			struct Property
			{
				u32 m_skillPropertyIndex; // 4 bytes
				u32 m_level; // 4 bytes
			};
			POP_PACKED
			Property m_properties[1]; // 8 bytes
		};
		POP_PACKED
		Skill skills[1]; // 20 bytes
	};
	POP_PACKED
	LeaderInfo charinfo; // variable
	// logger 0x986c62
};
POP_PACKED
ASSERT_SIZE(SA_RequestLeaderInfo, 52);
PUSH_PACKED
struct SA_FriendSetComrade
{
	enum { NET_ID = 62268 };
	u32 retval; // 4 bytes
	u16 friendNickname_len; // 2 bytes
	wchar_t friendNickname[1]; // 2 bytes (wide string, VEC convention)
	// logger 0x98014f
};
POP_PACKED
ASSERT_SIZE(SA_FriendSetComrade, 8);
PUSH_PACKED
struct SA_FriendUnsetComrade
{
	enum { NET_ID = 62269 };
	u32 retval; // 4 bytes
	// variable part (wide string):
	u16 friendNickname_len; // 2 bytes
	wchar_t friendNickname[1]; // 2 bytes
	// logger 0x98039c
};
POP_PACKED
ASSERT_SIZE(SA_FriendUnsetComrade, 8);
PUSH_PACKED
struct SA_FriendSetFavorite
{
	enum { NET_ID = 62270 };
	u32 result; // 4 bytes
	// variable part (wide string):
	u16 friendNickname_len; // 2 bytes
	wchar_t friendNickname[1]; // 2 bytes
	// logger 0x980221
};
POP_PACKED
ASSERT_SIZE(SA_FriendSetFavorite, 8);
PUSH_PACKED
struct SA_FriendUnsetFavorite
{
	enum { NET_ID = 62271 };
	u32 result; // 4 bytes
	// variable part (wide string):
	u16 friendNickname_len; // 2 bytes
	wchar_t friendNickname[1]; // 2 bytes each
	// logger 0x98046e
};
POP_PACKED
ASSERT_SIZE(SA_FriendUnsetFavorite, 8);
PUSH_PACKED
struct SN_FriendPartycreation
{
	enum { NET_ID = 62272 };
	u16 friendNickname_len; // 2 bytes
	wchar_t friendNickname[1]; // 2 bytes (wide string, u16 count + count*2 wchar)
	u32 stageIndex; // 4 bytes
	u8 gameType; // 1 bytes
	u8 gameDefType; // 1 bytes
	// logger 0x99319b
};
POP_PACKED
ASSERT_SIZE(SN_FriendPartycreation, 10);

PUSH_PACKED
struct SA_RequestMissionReward
{
	enum { NET_ID = 62273 };
	u32 result; // 4 bytes
	// mission (PST_MISSION_INFO):
	PUSH_PACKED
	struct MissionInfo
	{
		u32 index; // 4 bytes
		i64 progressCount; // 8 bytes
		u16 changedCount; // 2 bytes
		u8 isCleared; // 1 bytes
		u8 isRewarded; // 1 bytes
	};
	POP_PACKED
	MissionInfo mission; // 16 bytes
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
	// variable part (PST_GLOBAL_NOTICE):
	PUSH_PACKED
	struct Notice
	{
		u8 locale; // 1 bytes
		u16 message_len; // 2 bytes
		wchar_t message[1]; // 2 bytes
	};
	POP_PACKED
	u16 noticeList_count; // 2 bytes
	Notice noticeList[1]; // 5 bytes
	// logger 0x995d9a
};
POP_PACKED
ASSERT_SIZE(SN_GlobalNotice, 13);
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
	u16 nickname_len; // 2 bytes
	wchar_t nickname[1]; // 2 bytes (wide string)
	u16 level; // 2 bytes
	u32 achievementScore; // 4 bytes
	u8 tierGrade; // 1 bytes
	u16 statusMessage_len; // 2 bytes
	wchar_t statusMessage[1]; // 2 bytes (wide string)
	u32 creatureIndex; // 4 bytes
	u32 power; // 4 bytes
	u32 viability; // 4 bytes
	u8 skinId; // 1 bytes
	// variable part (ST_SKILLINFO):
	PUSH_PACKED
	struct SkillInfo
	{
		u32 index; // 4 bytes
		u8 level; // 1 bytes
		u8 isUnlocked; // 1 bytes (bool)
	};
	POP_PACKED
	u16 skills_count; // 2 bytes
	SkillInfo skills[1]; // 6 bytes
	// variable part (ST_WEAPONINFO):
	PUSH_PACKED
	struct WeaponInfo
	{
		u8 type; // 1 bytes
		u32 index; // 4 bytes
		u8 grade; // 1 bytes
		u8 isUnlocked; // 1 bytes (bool)
	};
	POP_PACKED
	WeaponInfo weapon; // 7 bytes
	// variable part (PST_PROFILE_ITEM):
	PUSH_PACKED
	struct ProfileItem
	{
		ItemUID m_itemID; // 4 bytes
		u8 m_invenType; // 1 bytes
		u32 m_slot; // 4 bytes
		ItemDocIndex m_itemIndex; // 4 bytes
		u32 m_count; // 4 bytes
		u32 m_propertyGroupIndex; // 4 bytes
		u8 m_isLifeTimeAbsolute; // 1 bytes (bool)
		i64 m_lifeEndTimeUTC; // 8 bytes
		// variable part (PST_ITEM_PROPERTY):
		PUSH_PACKED
		struct ItemProperty
		{
			u8 Type; // 1 bytes
			u32 TypeDetail; // 4 bytes
			u8 ValueType; // 1 bytes
			f32 Value; // 4 bytes
			u8 Fixed; // 1 bytes (bool)
		};
		POP_PACKED
		u16 m_properties_count; // 2 bytes
		ItemProperty m_properties[1]; // 11 bytes
	};
	POP_PACKED
	u16 masterGears_count; // 2 bytes
	ProfileItem masterGears[1]; // 43 bytes
	// logger 0x982a08
};
POP_PACKED
ASSERT_SIZE(SA_GetUserinfo, 92);
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
	// variable part (VEC of u32 event master IDs):
	u16 openedEventMasters_count; // 2 bytes
	u32 openedEventMasters[1]; // 4 bytes
	// logger 0x99cb16
};
POP_PACKED
ASSERT_SIZE(SN_OpenEventMasters, 6);
PUSH_PACKED
struct SA_CreateGuild
{
	enum { NET_ID = 62287 };
	u32 result; // 4 bytes
	u16 guildName_len; // 2 bytes
	wchar_t guildName[1]; // 2 bytes (wide string)
	u16 guildTag_len; // 2 bytes
	wchar_t guildTag[1]; // 2 bytes
	u8 masterTopPvpTierGrade; // 1 bytes
	u16 masterTopPvpTierPoint; // 2 bytes
	u32 masterAchievementScore; // 4 bytes
	// logger 0x97ea47
};
POP_PACKED
ASSERT_SIZE(SA_CreateGuild, 19);

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
	u16 nicknameKickedOut_len; // 2 bytes
	wchar_t nicknameKickedOut[1]; // 2 bytes (wide string, 0xa19080 reader)
	// logger 0x9842fb
};
POP_PACKED
ASSERT_SIZE(SA_KickGuildmember, 8);
PUSH_PACKED
struct SA_JoinGuild
{
	enum { NET_ID = 62293 };
	u32 result; // 4 bytes
	u16 guildName_len; // 2 bytes (wide string char count)
	wchar_t guildName[1]; // 2 bytes (wide string, reader 0xa19080: u16 count + count*2 wchar payload)
	u8 guildJoinType; // 1 bytes
	// logger 0x984157
};
POP_PACKED
ASSERT_SIZE(SA_JoinGuild, 9);
PUSH_PACKED
struct SA_GetGuildjoinrequestlist
{
	enum { NET_ID = 62294 };
	u32 result; // 4 bytes
	// variable part (ST_GUILDJOINREQUEST):
	PUSH_PACKED
	struct GuildJoinRequest
	{
		u16 candidateNickname_len; // 2 bytes
		wchar_t candidateNickname[1]; // 2 bytes
		u16 lvl; // 2 bytes
		u16 leaderClassType; // 2 bytes
		u16 masterCount; // 2 bytes
		u32 achievementScore; // 4 bytes
		u8 topPvpTierGrade; // 1 bytes
		u16 topPvpTierPoint; // 2 bytes
	};
	POP_PACKED
	u16 guildJoinRequestList_count; // 2 bytes
	GuildJoinRequest guildJoinRequestList[1]; // 17 bytes
	// logger 0x980fcf
};
POP_PACKED
ASSERT_SIZE(SA_GetGuildjoinrequestlist, 23);
PUSH_PACKED
struct SA_RespondGuildjoinrequest
{
	enum { NET_ID = 62295 };
	u32 result; // 4 bytes
	u8 isApproval; // 1 bytes (bool)
	// candidate (ST_GUILDMEMBER_PROFILE, logger 0x9aa587):
	PUSH_PACKED
	struct GuildmemberProfile
	{
		// variable part (wide string):
		u16 nickname_len; // 2 bytes
		wchar_t nickname[1]; // 2 bytes
		u32 membershipId; // 4 bytes
		u16 lvl; // 2 bytes
		u16 leaderClassType; // 2 bytes
		u16 masterCount; // 2 bytes
		u32 achievementScore; // 4 bytes
		u8 topPvpTierGrade; // 1 bytes
		u16 topPvpTierPoint; // 2 bytes
		u32 contributedGuildPoint; // 4 bytes
		u32 contributedGuildFund; // 4 bytes
		u16 guildPvpWin; // 2 bytes
		u16 guildPvpPlay; // 2 bytes
		i64 lastLogoutDate; // 8 bytes
	};
	POP_PACKED
	GuildmemberProfile candidate; // 41 bytes (count + arr[1] + 37 fixed)
	// logger 0x9872dc
};
POP_PACKED
ASSERT_SIZE(SA_RespondGuildjoinrequest, 46);
PUSH_PACKED
struct SA_InviteGuildmember
{
	enum { NET_ID = 62296 };
	u32 result; // 4 bytes
	// variable part (wide string):
	u16 inviteeNickname_len; // 2 bytes
	wchar_t inviteeNickname[1]; // 2 bytes
	// logger 0x98354b
};
POP_PACKED
ASSERT_SIZE(SA_InviteGuildmember, 8);
PUSH_PACKED
struct SA_GetGuildinvitationlist
{
	enum { NET_ID = 62297 };
	u32 result; // 4 bytes
	// variable part (ST_GUILDINVITATION):
	PUSH_PACKED
	struct GuildInvitation
	{
		u16 guildName_len; // 2 bytes
		wchar_t guildName[1]; // 2 bytes
		u16 inviterName_len; // 2 bytes
		wchar_t inviterName[1]; // 2 bytes
	};
	POP_PACKED
	u16 guildInvitationList_count; // 2 bytes
	GuildInvitation guildInvitationList[1]; // 8 bytes
	// logger 0x980e5b
};
POP_PACKED
ASSERT_SIZE(SA_GetGuildinvitationlist, 14);
PUSH_PACKED
struct SA_RespondGuildinvitation
{
	enum { NET_ID = 62298 };
	u32 result; // 4 bytes
	u8 accept; // 1 bytes (bool)
	u16 guildName_len; // 2 bytes
	wchar_t guildName[1]; // 2 bytes
	u16 guildTag_len; // 2 bytes
	wchar_t guildTag[1]; // 2 bytes
	// logger 0x9871b9
};
POP_PACKED
ASSERT_SIZE(SA_RespondGuildinvitation, 13);
PUSH_PACKED
struct SA_GetGuildemblemlist
{
	enum { NET_ID = 62301 };
	u32 result; // 4 bytes
	u16 guildEmblemList_count; // 2 bytes
	u32 guildEmblemList[1]; // 4 bytes
	// logger 0x980bb1
};
POP_PACKED
ASSERT_SIZE(SA_GetGuildemblemlist, 10);
PUSH_PACKED
struct SA_GetGuildpublicprofile
{
	enum { NET_ID = 62303 };
	u32 result; // 4 bytes
	u16 guildName_len; // 2 bytes
	wchar_t guildName[1]; // 2 bytes
	u16 guildTag_len; // 2 bytes
	wchar_t guildTag[1]; // 2 bytes
	u32 emblemIndex; // 4 bytes
	u8 guildLvl; // 1 bytes
	u8 memberMax; // 1 bytes
	u16 ownerNickname_len; // 2 bytes
	wchar_t ownerNickname[1]; // 2 bytes
	i64 createdDate; // 8 bytes
	i64 dissolutionDate; // 8 bytes
	u8 joinType; // 1 bytes
	// ST_GUILD_INTEREST (fixed 7 bools):
	u8 likePveStage; // 1 bytes (bool)
	u8 likeDefence; // 1 bytes (bool)
	u8 likePvpNormal; // 1 bytes (bool)
	u8 likePvpOccupy; // 1 bytes (bool)
	u8 likePvpGot; // 1 bytes (bool)
	u8 likePvpRank; // 1 bytes (bool)
	u8 likeOlympic; // 1 bytes (bool)
	u16 guildIntro_len; // 2 bytes
	wchar_t guildIntro[1]; // 2 bytes
	u8 memberNum; // 1 bytes
	// logger 0x9818a8
};
POP_PACKED
ASSERT_SIZE(SA_GetGuildpublicprofile, 51);
PUSH_PACKED
struct SA_GetGuildpublicprofileFromUser
{
	enum { NET_ID = 62304 };
	u32 result; // 4 bytes
	u16 guildName_len; wchar_t guildName[1]; // variable (wide string)
	u16 guildTag_len; wchar_t guildTag[1]; // variable (wide string)
	u32 emblemIndex; // 4 bytes
	u8 guildLvl; // 1 bytes
	u8 memberMax; // 1 bytes
	u16 ownerNickname_len; wchar_t ownerNickname[1]; // variable (wide string)
	i64 createdDate; // 8 bytes
	i64 dissolutionDate; // 8 bytes
	u8 joinType; // 1 bytes
	// ST_GUILD_INTEREST (7 bytes, LogST_GuildInterest 0x9ab21f):
	PUSH_PACKED
	struct GuildInterest
	{
		u8 likePveStage; // 1 bytes (bool)
		u8 likeDefence; // 1 bytes (bool)
		u8 likePvpNormal; // 1 bytes (bool)
		u8 likePvpOccupy; // 1 bytes (bool)
		u8 likePvpGot; // 1 bytes (bool)
		u8 likePvpRank; // 1 bytes (bool)
		u8 likeOlympic; // 1 bytes (bool)
	};
	POP_PACKED
	GuildInterest guildInterest; // 7 bytes
	u16 guildIntro_len; wchar_t guildIntro[1]; // variable (wide string)
	u8 memberNum; // 1 bytes
	u16 memberNickname_len; wchar_t memberNickname[1]; // variable (wide string)
	// logger 0x981b60
};
POP_PACKED
ASSERT_SIZE(SA_GetGuildpublicprofileFromUser, 55);
PUSH_PACKED
struct SA_EditGuildnotice
{
	enum { NET_ID = 62305 };
	u32 result; // 4 bytes
	u16 guildNotice_len; // 2 bytes
	wchar_t guildNotice[1]; // 2 bytes (wide string, wire = 2 + len*2)
};
POP_PACKED
ASSERT_SIZE(SA_EditGuildnotice, 8);
PUSH_PACKED
struct SA_EditGuildintro
{
	enum { NET_ID = 62306 };
	u32 result; // 4 bytes
	// wide string guildIntro:
	u16 guildIntro_len; // 2 bytes
	wchar_t guildIntro[1]; // 2 bytes
	// logger 0x97f311
};
POP_PACKED
ASSERT_SIZE(SA_EditGuildintro, 8);
PUSH_PACKED
struct SA_EditGuildtag
{
	enum { NET_ID = 62307 };
	u32 result; // 4 bytes
	// variable part (wide string):
	u16 guildTag_len; // 2 bytes
	wchar_t guildTag[1]; // 2 bytes (string)
	u32 guildFundCost; // 4 bytes
	u32 totalGuildFund; // 4 bytes
	// logger 0x97f621
};
POP_PACKED
ASSERT_SIZE(SA_EditGuildtag, 16);
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
	u16 memberNickname_len; // 2 bytes
	wchar_t memberNickname[1]; // 2 bytes
	u32 oldMembershipId; // 4 bytes
	u32 newMembershipId; // 4 bytes
	u16 className_len; // 2 bytes
	wchar_t className[1]; // 2 bytes
	// logger 0x988908
};
POP_PACKED
ASSERT_SIZE(SA_SetGuildmemberclass, 20);
PUSH_PACKED
struct SA_SetGuildmaster
{
	enum { NET_ID = 62312 };
	u32 result; // 4 bytes
	u32 myMembershipId; // 4 bytes
	u16 newMasterNickname_len; // 2 bytes
	wchar_t newMasterNickname[1]; // 2 bytes (wide string)
	// logger 0x98880f
};
POP_PACKED
ASSERT_SIZE(SA_SetGuildmaster, 12);
PUSH_PACKED
struct SA_CreateGuildmembership
{
	enum { NET_ID = 62313 };
	u32 result; // 4 bytes
	u32 membershipId; // 4 bytes
	u8 iconIndex; // 1 bytes
	u16 className_len; // 2 bytes
	wchar_t className[1]; // 2 bytes (wide string)
	// rights (ST_GuildmemberRights):
	PUSH_PACKED
	struct Rights
	{
		u8 hasInviteRight; // 1 bytes (bool)
		u8 hasExpelRight; // 1 bytes (bool)
		u8 hasMembershipChgRight; // 1 bytes (bool)
		u8 hasClassAssignRight; // 1 bytes (bool)
		u8 hasNoticeChgRight; // 1 bytes (bool)
		u8 hasIntroChgRight; // 1 bytes (bool)
		u8 hasInterestChgRight; // 1 bytes (bool)
		u8 hasFundManageRight; // 1 bytes (bool)
		u8 hasJoinTypeChgRight; // 1 bytes (bool)
		u8 hasEmblemChgRight; // 1 bytes (bool)
	};
	POP_PACKED
	Rights rights; // 10 bytes
	// logger 0x97ebba
};
POP_PACKED
ASSERT_SIZE(SA_CreateGuildmembership, 23);
PUSH_PACKED
struct SA_DeleteGuildmembership
{
	enum { NET_ID = 62314 };
	u32 result; // 4 bytes
	u32 membershipId; // 4 bytes
	u32 regularMembershipId; // 4 bytes
	u16 regularClassName_len; // 2 bytes (wchar count)
	wchar_t regularClassName[1]; // 2 bytes (wide string, reader 0xa19080: 2 + count*2)
	// variable part (VEC of wide strings):
	PUSH_PACKED
	struct ReassignedMemberNickname
	{
		u16 len; // 2 bytes
		wchar_t data[1]; // 2 bytes
	};
	POP_PACKED
	u16 reassignedMemberNicknames_count; // 2 bytes
	ReassignedMemberNickname reassignedMemberNicknames[1]; // 4 bytes (per element: u16 len + wchar data)
	// logger 0x97ef60
};
POP_PACKED
ASSERT_SIZE(SA_DeleteGuildmembership, 22);
PUSH_PACKED
struct SA_EditGuildmembership
{
	enum { NET_ID = 62315 };
	u32 result; // 4 bytes
	u32 membershipId; // 4 bytes
	u8 iconIndex; // 1 bytes
	u16 className_len; // 2 bytes
	wchar_t className[1]; // 2 bytes (wide string)
	// rights (ST_GUILDMEMBER_RIGHTS): 10 x u8 bool flags
	PUSH_PACKED
	struct GuildmemberRights
	{
		u8 hasInviteRight; // 1 bytes (bool)
		u8 hasExpelRight; // 1 bytes (bool)
		u8 hasMembershipChgRight; // 1 bytes (bool)
		u8 hasClassAssignRight; // 1 bytes (bool)
		u8 hasNoticeChgRight; // 1 bytes (bool)
		u8 hasIntroChgRight; // 1 bytes (bool)
		u8 hasInterestChgRight; // 1 bytes (bool)
		u8 hasFundManageRight; // 1 bytes (bool)
		u8 hasJoinTypeChgRight; // 1 bytes (bool)
		u8 hasEmblemChgRight; // 1 bytes (bool)
	};
	POP_PACKED
	GuildmemberRights rights; // 10 bytes
	// logger 0x97f3e3
};
POP_PACKED
ASSERT_SIZE(SA_EditGuildmembership, 23);
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
	// variable part (ST_GUILD_SKILL):
	PUSH_PACKED
	struct GuildSkill
	{
		u8 type; // 1 bytes
		u8 lvl; // 1 bytes
		i64 expiryDate; // 8 bytes
		u16 extensionCount; // 2 bytes
	};
	POP_PACKED
	u16 guildSkills_count; // 2 bytes
	GuildSkill guildSkills[1]; // 12 bytes
	// logger 0x982531
};
POP_PACKED
ASSERT_SIZE(SA_GetGuildskilllist, 18);
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
	u8 rankingType; // 1 bytes (bool)
	u16 seasonNo; // 2 bytes
	// variable part (ST_GUILD_RANKINFO):
	PUSH_PACKED
	struct GuildRankInfo
	{
		u32 rankNo; // 4 bytes
		u32 prevRankNo; // 4 bytes
		u16 guildTag_len; // 2 bytes
		wchar_t guildTag[1]; // 2 bytes
		u16 guildName_len; // 2 bytes
		wchar_t guildName[1]; // 2 bytes
		u16 guildOwnerNickname_len; // 2 bytes
		wchar_t guildOwnerNickname[1]; // 2 bytes
		u32 rankingValue; // 4 bytes
	};
	POP_PACKED
	GuildRankInfo rankInfo;
	// logger 0x981e43
};
POP_PACKED
ASSERT_SIZE(SA_GetGuildrank, 31);
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
	// variable part (ST_GUILD_RANKINFO):
	PUSH_PACKED
	struct GuildRankInfo
	{
		u32 rankNo; // 4 bytes
		u32 prevRankNo; // 4 bytes
		u16 guildTag_len; // 2 bytes
		wchar_t guildTag[1]; // 2 bytes
		u16 guildName_len; // 2 bytes
		wchar_t guildName[1]; // 2 bytes
		u16 guildOwnerNickname_len; // 2 bytes
		wchar_t guildOwnerNickname[1]; // 2 bytes
		u32 rankingValue; // 4 bytes
	};
	POP_PACKED
	u16 guildRankList_count; // 2 bytes
	GuildRankInfo guildRankList[1]; // 24 bytes
	// logger 0x98212b
};
POP_PACKED
ASSERT_SIZE(SA_GetGuildranklist, 50);
PUSH_PACKED
struct SA_GetGuildrankrewardinfo
{
	enum { NET_ID = 62325 };
	u32 result; // 4 bytes
	u8 rankingType; // 1 bytes
	// variable part (ST_RANKING_REWARD):
	PUSH_PACKED
	struct RankingReward
	{
		u32 firstRankNo; // 4 bytes
		u32 lastRankNo; // 4 bytes
		// variable part (ST_REWARD_ITEM):
		PUSH_PACKED
		struct RewardItem
		{
			u32 m_ItemType; // 4 bytes
			u32 m_ItemIndex; // 4 bytes
			u32 m_ItemAmount; // 4 bytes
		};
		POP_PACKED
		u16 items_count; // 2 bytes
		RewardItem items[1]; // 12 bytes
	};
	POP_PACKED
	u16 rewards_count; // 2 bytes
	RankingReward rewards[1]; // 22 bytes
	// logger 0x98238f
};
POP_PACKED
ASSERT_SIZE(SA_GetGuildrankrewardinfo, 29);
PUSH_PACKED
struct SA_GetRecommendedguildlist
{
	enum { NET_ID = 62326 };
	u32 result; // 4 bytes
	// variable part (ST_GUILD_SEARCH_INFO):
	PUSH_PACKED
	struct GuildSearchInfo
	{
		u16 guildName_len; // 2 bytes
		wchar_t guildName[1]; // 2 bytes (wide string)
		u16 guildTag_len; // 2 bytes
		wchar_t guildTag[1]; // 2 bytes (wide string)
		u32 emblemIndex; // 4 bytes
		u8 guildLvl; // 1 bytes
		u8 joinType; // 1 bytes
		u8 memberNum; // 1 bytes
		u8 memberMax; // 1 bytes
		// nested (ST_GUILD_INTEREST):
		PUSH_PACKED
		struct Interest
		{
			u8 likePveStage; // 1 bytes (bool)
			u8 likeDefence; // 1 bytes (bool)
			u8 likePvpNormal; // 1 bytes (bool)
			u8 likePvpOccupy; // 1 bytes (bool)
			u8 likePvpGot; // 1 bytes (bool)
			u8 likePvpRank; // 1 bytes (bool)
			u8 likeOlympic; // 1 bytes (bool)
		};
		POP_PACKED
		Interest interest; // 7 bytes
	};
	POP_PACKED
	u16 guilds_count; // 2 bytes
	GuildSearchInfo guilds[1]; // variable (wide strings)
	// logger 0x982899
};
POP_PACKED
// ASSERT_SIZE omitted: guilds[] elements contain variable-length wide strings
PUSH_PACKED
struct SA_SearchGuild
{
	enum { NET_ID = 62327 };
	u32 result; // 4 bytes
	u32 pageNo; // 4 bytes
	u8 rowsPerPage; // 1 bytes
	u32 maxPage; // 4 bytes
	// variable part (ST_GUILD_SEARCHINFO):
	PUSH_PACKED
	struct GuildSearchInfo
	{
		u16 guildName_len; // 2 bytes
		wchar_t guildName[1]; // 2 bytes (wide)
		u16 guildTag_len; // 2 bytes
		wchar_t guildTag[1]; // 2 bytes (wide)
		u32 emblemIndex; // 4 bytes
		u8 guildLvl; // 1 bytes
		u8 joinType; // 1 bytes
		u8 memberNum; // 1 bytes
		u8 memberMax; // 1 bytes
		// interest (ST_GUILD_INTEREST; wire = 4 bytes):
		PUSH_PACKED
		struct Interest
		{
			u8 likePveStage; // 1 bytes (bool)
			u8 likeDefence; // 1 bytes (bool)
			u8 likePvpNormal; // 1 bytes (bool)
			u8 likePvpOccupy; // 1 bytes (bool)
		};
		POP_PACKED
		Interest interest; // 4 bytes
	};
	POP_PACKED
	u16 guilds_count; // 2 bytes
	GuildSearchInfo guilds[1]; // variable (16 + 2*len_name + 2*len_tag per element)
	// logger 0x987fe2
};
POP_PACKED
ASSERT_SIZE(SA_SearchGuild, 35);
PUSH_PACKED
struct SA_GetPlayingGuildpvplist
{
	enum { NET_ID = 62328 };
	u32 result; // 4 bytes
	u16 pageNo; // 2 bytes
	u8 rowsPerPage; // 1 bytes
	u16 maxPageNo; // 2 bytes
	// variable part (ST_GUILDPVP_SESSION): guildPvpList
	PUSH_PACKED
	struct Team
	{
		u32 guildId; // 4 bytes
		u16 guildName_len; // 2 bytes
		wchar_t guildName[1]; // 2 bytes
		u32 guildEmblemIndex; // 4 bytes
		// variable part: nicknameList (vector of wide strings)
		u16 nicknameList_count; // 2 bytes
		u16 nicknameList_len[1]; // 2 bytes
		wchar_t nicknameList[1][1]; // 2 bytes
	};
	POP_PACKED
	PUSH_PACKED
	struct Session
	{
		u32 instanceId; // 4 bytes
		i64 createdDate; // 8 bytes
		Team redTeam; // 18 bytes + var
		Team blueTeam; // 18 bytes + var
	};
	POP_PACKED
	u16 guildPvpList_count; // 2 bytes
	Session guildPvpList[1]; // 48 bytes
	// logger 0x9826a5
};
POP_PACKED
ASSERT_SIZE(SA_GetPlayingGuildpvplist, 59);
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
	u16 guildName_len; // 2 bytes
	wchar_t guildName[1]; // 2 bytes (wide string)
	u16 inviterName_len; // 2 bytes
	wchar_t inviterName[1]; // 2 bytes (wide string)
	// logger 0x997d4e
};
POP_PACKED
ASSERT_SIZE(SN_GuildInvitation, 8);
PUSH_PACKED
struct SN_GuildJoinapproval
{
	enum { NET_ID = 62333 };
	u16 guildName_len; // 2 bytes
	wchar_t guildName[1]; // 2 bytes (wide string)
	u16 guildTag_len; // 2 bytes
	wchar_t guildTag[1]; // 2 bytes (wide string)
	// logger 0x997e1f
};
POP_PACKED
ASSERT_SIZE(SN_GuildJoinapproval, 8);
PUSH_PACKED
struct SN_GuildtagChanged
{
	enum { NET_ID = 62334 };
	u16 tag_len; // 2 bytes
	wchar_t tag[1]; // 2 bytes (wide string)
	u32 guildFundCost; // 4 bytes
	u32 totalGuildFund; // 4 bytes
	// logger 0x997c57
};
POP_PACKED
ASSERT_SIZE(SN_GuildtagChanged, 12);
PUSH_PACKED
struct SN_GuildNotice
{
	enum { NET_ID = 62335 };
	// notice: wide string (u16 char count + wchar payload)
	u16 notice_len; // 2 bytes
	wchar_t notice[1]; // 2 bytes per char
};
POP_PACKED
ASSERT_SIZE(SN_GuildNotice, 4);
PUSH_PACKED
struct SN_GuildmemberJoin
{
	enum { NET_ID = 62336 };
	// newMember (ST_GUILDMEMBER_PROFILE):
	u16 nickname_len; // 2 bytes
	wchar_t nickname[1]; // 2 bytes (wide string)
	u32 membershipId; // 4 bytes
	u16 lvl; // 2 bytes
	u16 leaderClassType; // 2 bytes
	u16 masterCount; // 2 bytes
	u32 achievementScore; // 4 bytes
	u8 topPvpTierGrade; // 1 bytes
	u16 topPvpTierPoint; // 2 bytes
	u32 contributedGuildPoint; // 4 bytes
	u32 contributedGuildFund; // 4 bytes
	u16 guildPvpWin; // 2 bytes
	u16 guildPvpPlay; // 2 bytes
	i64 lastLogoutDate; // 8 bytes
	// logger 0x996ffa
};
POP_PACKED
ASSERT_SIZE(SN_GuildmemberJoin, 41);
PUSH_PACKED
struct SN_GuildmemberQuit
{
	enum { NET_ID = 62337 };
	u16 nickname_len; // 2 bytes
	wchar_t nickname[1]; // 2 bytes
	u8 isKickedOut; // 1 bytes (bool)
	// logger 0x9970c3
};
POP_PACKED
ASSERT_SIZE(SN_GuildmemberQuit, 5);
PUSH_PACKED
struct SN_GuildmemberChgnickname
{
	enum { NET_ID = 62339 };
	u16 oldNickname_len; // 2 bytes
	wchar_t oldNickname[1]; // 2 bytes (variable)
	u16 newNickname_len; // 2 bytes
	wchar_t newNickname[1]; // 2 bytes (variable)
	// logger 0x996f29
};
POP_PACKED
ASSERT_SIZE(SN_GuildmemberChgnickname, 8);
PUSH_PACKED
struct SN_GuildmemberChgleaderclass
{
	enum { NET_ID = 62340 };
	u16 nickname_len; // 2 bytes
	wchar_t nickname[1]; // 2 bytes (variable)
	u16 newLeaderClass; // 2 bytes
	// logger 0x996e57
};
POP_PACKED
ASSERT_SIZE(SN_GuildmemberChgleaderclass, 6);
PUSH_PACKED
struct SN_GuildmembershipAdded
{
	enum { NET_ID = 62341 };
	u32 membershipId; // 4 bytes
	u8 iconIndex; // 1 bytes
	// className: wide string
	u16 className_len; // 2 bytes
	wchar_t className[1]; // 2 bytes
	// rights (ST_GUILDMEMBER_RIGHTS):
	PUSH_PACKED
	struct GuildmemberRights
	{
		u8 hasInviteRight; // 1 bytes (bool)
		u8 hasExpelRight; // 1 bytes (bool)
		u8 hasMembershipChgRight; // 1 bytes (bool)
		u8 hasClassAssignRight; // 1 bytes (bool)
		u8 hasNoticeChgRight; // 1 bytes (bool)
		u8 hasIntroChgRight; // 1 bytes (bool)
		u8 hasInterestChgRight; // 1 bytes (bool)
		u8 hasFundManageRight; // 1 bytes (bool)
		u8 hasJoinTypeChgRight; // 1 bytes (bool)
		u8 hasEmblemChgRight; // 1 bytes (bool)
	};
	POP_PACKED
	GuildmemberRights rights; // 10 bytes
	// logger 0x996a03
};
POP_PACKED
ASSERT_SIZE(SN_GuildmembershipAdded, 19);
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
	// variable part (wide string):
	u16 className_len; // 2 bytes
	wchar_t className[1]; // 2*len bytes
	// rights (ST_GUILDMEMBER_RIGHTS):
	PUSH_PACKED
	struct Rights
	{
		u8 hasInviteRight; // 1 bytes (bool)
		u8 hasExpelRight; // 1 bytes (bool)
		u8 hasMembershipChgRight; // 1 bytes (bool)
		u8 hasClassAssignRight; // 1 bytes (bool)
		u8 hasNoticeChgRight; // 1 bytes (bool)
		u8 hasIntroChgRight; // 1 bytes (bool)
		u8 hasInterestChgRight; // 1 bytes (bool)
		u8 hasFundManageRight; // 1 bytes (bool)
		u8 hasJoinTypeChgRight; // 1 bytes (bool)
		u8 hasEmblemChgRight; // 1 bytes (bool)
	};
	POP_PACKED
	Rights rights; // 10 bytes
	// logger 0x996b48
};
POP_PACKED
ASSERT_SIZE(SN_GuildmembershipModified, 19);
PUSH_PACKED
struct SN_GuildmemberChgclass
{
	enum { NET_ID = 62344 };
	u32 oldMembershipId; // 4 bytes
	u32 newMembershipId; // 4 bytes
	// variable part:
	u16 className_len; // 2 bytes
	wchar_t className[1]; // 2 bytes (wide string)
	u16 memberNickname_len; // 2 bytes
	wchar_t memberNickname[1]; // 2 bytes (wide string)
	// logger 0x996d36
};
POP_PACKED
ASSERT_SIZE(SN_GuildmemberChgclass, 16);
PUSH_PACKED
struct SN_GuildmasterDelegation
{
	enum { NET_ID = 62345 };
	// formerMasterNickname:
	u16 formerMasterNickname_len; // 2 bytes
	wchar_t formerMasterNickname[1]; // 2 bytes (1 wchar, wide string)
	u32 formerMasterMembershipId; // 4 bytes
	// newMasterNickname:
	u16 newMasterNickname_len; // 2 bytes
	wchar_t newMasterNickname[1]; // 2 bytes (1 wchar, wide string)
	// logger 0x99690b
};
POP_PACKED
ASSERT_SIZE(SN_GuildmasterDelegation, 12);
PUSH_PACKED
struct SN_GuildfundAdded
{
	enum { NET_ID = 62346 };
	// variable part (wide string):
	u16 memberNickname_len; // 2 bytes
	wchar_t memberNickname[1]; // 2 bytes (wide string, 2 + count*2 wire)
	u32 guildFundGain; // 4 bytes
	u32 totalGuildFund; // 4 bytes
	// logger 0x996814
};
POP_PACKED
ASSERT_SIZE(SN_GuildfundAdded, 12);
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
	u16 memberNickname_len; // 2 bytes
	wchar_t memberNickname[1]; // 2 bytes each (wide string)
	u8 skillType; // 1 bytes
	u8 skillLvl; // 1 bytes
	u16 timeLimitHour; // 2 bytes
	i64 expiryDate; // 8 bytes
	u32 guildFundCost; // 4 bytes
	u32 totalGuildFund; // 4 bytes
	// logger 0x99775a
};
POP_PACKED
ASSERT_SIZE(SN_GuildskillAcquired, 24);
PUSH_PACKED
struct SN_GuildskillUpgraded
{
	enum { NET_ID = 62351 };
	// variable part (wide string):
	u16 memberNickname_len; // 2 bytes
	wchar_t memberNickname[1]; // 2 bytes
	u8 skillType; // 1 bytes
	u8 skillLvl; // 1 bytes
	u16 timeLimitHour; // 2 bytes
	i64 expiryDate; // 8 bytes
	u32 guildFundCost; // 4 bytes
	u32 totalGuildFund; // 4 bytes
	// logger 0x997abb
};
POP_PACKED
ASSERT_SIZE(SN_GuildskillUpgraded, 24);

PUSH_PACKED
struct SN_GuildskillExtended
{
	enum { NET_ID = 62352 };
	// variable part (memberNickname):
	u16 memberNickname_len; // 2 bytes
	wchar_t memberNickname[1]; // 2 bytes (wide string, reader 0xa19080)
	u8 skillType; // 1 bytes
	u8 skillLvl; // 1 bytes
	u16 skillExtensionCount; // 2 bytes
	u16 extensionTimeHour; // 2 bytes
	i64 skillExpiryDate; // 8 bytes
	u32 guildFundCost; // 4 bytes
	u32 totalGuildFund; // 4 bytes
	// logger 0x9978f6
};
POP_PACKED
ASSERT_SIZE(SN_GuildskillExtended, 26);
PUSH_PACKED
struct SN_GuildconstraintChange
{
	enum { NET_ID = 62353 };
	i64 changeDate; // 8 bytes
	// constraint (ST_GUILD_CONSTRAINT):
	PUSH_PACKED
	struct Constraint
	{
		u8 maxMember; // 1 bytes
		u8 maxEmblemLevel; // 1 bytes
		u8 maxGuildMissionLevel; // 1 bytes
		u16 maxDailyStagePlayGuildPoint; // 2 bytes
		u16 maxDailyArenaPlayGuildPoint; // 2 bytes
	};
	POP_PACKED
	Constraint constraint; // 7 bytes
	// logger 0x9965a1
};
POP_PACKED
ASSERT_SIZE(SN_GuildconstraintChange, 15);
PUSH_PACKED
struct SN_GuildmissionCompleted
{
	enum { NET_ID = 62354 };
	u8 missionIndex; // 1 bytes
	u32 rewardedGuildFund; // 4 bytes
	u16 totalGuildFund; // 2 bytes
	u16 contributedGuildFund; // 2 bytes
	// variable part (vector of wide strings):
	PUSH_PACKED
	struct Nickname
	{
		u16 len; // 2 bytes
		wchar_t str[1]; // 2 bytes
	};
	POP_PACKED
	u16 memberNicknames_count; // 2 bytes
	Nickname memberNicknames[1]; // 4 bytes
	u16 memberNickname_len; // 2 bytes
	wchar_t memberNickname[1]; // 2 bytes
	// logger 0x9972db
};
POP_PACKED
ASSERT_SIZE(SN_GuildmissionCompleted, 19);

PUSH_PACKED
struct SN_GuildpvpResult
{
	enum { NET_ID = 62355 };
	u8 guildPvpResultType; // 1 bytes
	u32 rp; // 4 bytes
	u16 win; // 2 bytes
	u16 draw; // 2 bytes
	u16 lose; // 2 bytes
	// variable part (members: VEC of wide strings, each u16 len + len*2 wchars):
	u16 members_count; // 2 bytes
	wchar_t members[1][1]; // variable-size (wide string elements)
	// variable part (oppGuildName: wide string):
	u16 oppGuildName_len; // 2 bytes
	wchar_t oppGuildName[1]; // variable
	// logger 0x997542
};
POP_PACKED
// ASSERT_SIZE omitted: variable-size (string vector + wide string)
// ASSERT_SIZE omitted: variable-size (string vector + wide string)

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
	u16 guildName_len; // 2 bytes
	wchar_t guildName[1]; // 2 bytes (wide string, wire 2 + 2*len)
	u16 nickname_len; // 2 bytes
	wchar_t nickname[1]; // 2 bytes (wide string, wire 2 + 2*len)
	// logger 0x9964d0
};
POP_PACKED
ASSERT_SIZE(SN_GuildchannelLeave, 8);
PUSH_PACKED
struct SN_PlayerBlink
{
	enum { NET_ID = 62362 };
	u32 entityID; // 4 bytes
	u32 targetID; // 4 bytes
	float3 CurPos; // 12 bytes (ST_POS3: fX, fY, fZ)
	float3 NewPos; // 12 bytes (ST_POS3: fX, fY, fZ)
	// NewDir (ST_POS2: fX, fY):
	PUSH_PACKED
	struct Pos2
	{
		f32 fX; // 4 bytes
		f32 fY; // 4 bytes
	};
	POP_PACKED
	Pos2 NewDir; // 8 bytes
	f32 nSpeed; // 4 bytes
	u32 nState; // 4 bytes
	u8 bGhostBlink; // 1 bytes (bool)
	// logger 0x99d9d0
};
POP_PACKED
ASSERT_SIZE(SN_PlayerBlink, 49);
PUSH_PACKED
struct SN_InvalidRemoteLog
{
	enum { NET_ID = 62363 };
	u16 log_len; // 2 bytes (wchar count)
	wchar_t log[1]; // 2 bytes (wide string, count*2 payload)
	// logger 0x998b5b
};
POP_PACKED
ASSERT_SIZE(SN_InvalidRemoteLog, 4);
PUSH_PACKED
struct SN_SyncActionMove
{
	enum { NET_ID = 62366 };
	u32 entityID; // 4 bytes
	u32 targetID; // 4 bytes
	float3 TargetPos; // 12 bytes (ST_POS3: fX, fY, fZ)
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
	float3 vPos; // 12 bytes (ST_POS3: fX, fY, fZ)
	u32 isAttack; // 4 bytes
	u32 targetId; // 4 bytes
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
	float3 vPos; // 12 bytes (ST_POS3: fX, fY, fZ)
	float3 vDir; // 12 bytes (ST_POS3: fX, fY, fZ)
	float3 vUpperDir; // 12 bytes (ST_POS3: fX, fY, fZ)
	float3 p3nPos; // 12 bytes (ST_POS3: fX, fY, fZ)
	u32 isAttack; // 4 bytes
	u32 targetId; // 4 bytes
	// logger 0x98c369
};
POP_PACKED
ASSERT_SIZE(SN_AiSyncBehaviorMotion, 65);
PUSH_PACKED
struct SN_AiSyncCooltime
{
	enum { NET_ID = 62370 };
	u32 entityID; // 4 bytes
	u16 cooltimeID_len; // 2 bytes (wide string, variable: 2 + 2*count)
	wchar_t cooltimeID[1]; // 2 bytes
	f32 fCooltime; // 4 bytes
	u32 nodeID; // 4 bytes
};
POP_PACKED
ASSERT_SIZE(SN_AiSyncCooltime, 16);
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
	float3 vSPos; // 12 bytes (ST_POS3: fX, fY, fZ)
	float3 vPos; // 12 bytes (ST_POS3: fX, fY, fZ)
	float3 vDir; // 12 bytes (ST_POS3: fX, fY, fZ)
	float3 vUpperDir; // 12 bytes (ST_POS3: fX, fY, fZ)
	f32 fSpeed; // 4 bytes
	f32 fFrameRate; // 4 bytes
	f32 fUpForce; // 4 bytes
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
	u16 strComName_len; // 2 bytes
	wchar_t strComName[1]; // 2 bytes (wide string, reader 0xa19080)
	// logger 0x98be42
};
POP_PACKED
ASSERT_SIZE(SN_AiAddCom, 8);
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
	float3 vPos; // 12 bytes (ST_POS3: fX, fY, fZ)
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
	float3 targetPos; // 12 bytes (ST_POS3: fX, fY, fZ)
	// logger 0x98c8a6
};
POP_PACKED
ASSERT_SIZE(SN_AiSyncMonsterSkillTarget, 20);
PUSH_PACKED
struct SN_AiSetActionState
{
	enum { NET_ID = 62382 };
	u8 excludedFieldBits; // 1 bytes
	u32 entityID; // 4 bytes
	i16 motionType; // 2 bytes
	i16 lowerBodyYaw; // 2 bytes
	u16 moveSpeed; // 2 bytes
	float3 pos; // 12 bytes (ST_POS3)
	// logger 0x98bfe5
};
POP_PACKED
ASSERT_SIZE(SN_AiSetActionState, 23);

PUSH_PACKED
struct SN_AiSetMovetargetpos
{
	enum { NET_ID = 62383 };
	u32 entityID; // 4 bytes
	u32 movePresetType; // 4 bytes
	float3 moveTargetPos; // 12 bytes (ST_POS3: fX, fY, fZ)
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
	float3 FirePosition; // 12 bytes (ST_POS3: fX, fY, fZ)
	u16 FireRotationX; // 2 bytes
	u16 FireRotationY; // 2 bytes
	u16 FireRotationZ; // 2 bytes
	float3 TargetPosition; // 12 bytes (ST_POS3: fX, fY, fZ)
	u16 FireObject_len; // 2 bytes
	wchar_t FireObject[1]; // 2 bytes (wide string)
	u16 Scale; // 2 bytes
	f32 LifeTime; // 4 bytes
	u32 HitInvalidTarget; // 4 bytes
	u16 RemoteCreatorID; // 2 bytes
	u16 ErrorAngleX; // 2 bytes
	u16 ErrorAngleY; // 2 bytes
	u8 ChargingLevel; // 1 bytes
	u8 GaugeLevel; // 1 bytes
	u16 FiringCount; // 2 bytes
	u8 ComboCount; // 1 bytes
	u32 TargetEntity; // 4 bytes
	u8 RemoteCreateFlags; // 1 bytes
	// logger 0x9a18e1
};
POP_PACKED
ASSERT_SIZE(SN_RemoteSyncCreateFromCreatorId, 72);
PUSH_PACKED
struct SN_RemoteSyncCreateFromRemoteDoc
{
	enum { NET_ID = 62386 };
	u32 ParentEntity; // 4 bytes
	u32 OwnerEntity; // 4 bytes
	u32 RemoteSeedID; // 4 bytes
	float3 FirePosition; // 12 bytes (ST_POS3: fX, fY, fZ)
	u16 FireRotationX; // 2 bytes
	u16 FireRotationY; // 2 bytes
	u16 FireRotationZ; // 2 bytes
	float3 TargetPosition; // 12 bytes (ST_POS3: fX, fY, fZ)
	u16 FireObject_len; // 2 bytes
	wchar_t FireObject[1]; // 2 bytes
	u16 Scale; // 2 bytes
	f32 LifeTime; // 4 bytes
	u32 HitInvalidTarget; // 4 bytes
	u32 RemoteDocIndex; // 4 bytes
	u32 TargetEntity; // 4 bytes
	u8 RemoteCreateFlags; // 1 bytes (bool)
	// logger 0x9a1cfb
};
POP_PACKED
ASSERT_SIZE(SN_RemoteSyncCreateFromRemoteDoc, 65);

PUSH_PACKED
struct SN_RemoteSnapshotFromRemoteDoc
{
	enum { NET_ID = 62387 };
	u32 ParentEntity; // 4 bytes
	u32 OwnerEntity; // 4 bytes
	u32 RemoteSeedID; // 4 bytes
	float3 FirePosition; // 12 bytes (ST_POS3: fX, fY, fZ)
	u16 FireRotationX; // 2 bytes
	u16 FireRotationY; // 2 bytes
	u16 FireRotationZ; // 2 bytes
	float3 TargetPosition; // 12 bytes (ST_POS3: fX, fY, fZ)
	u16 FireObject_len; // 2 bytes
	wchar_t FireObject[1]; // 2 bytes (wide string, variable)
	u16 Scale; // 2 bytes
	u32 LifeTime; // 4 bytes
	u32 DurationTime; // 4 bytes
	u32 HitInvalidTarget; // 4 bytes
	u32 RemoteDocIndex; // 4 bytes
	u32 TargetEntity; // 4 bytes
	u8 RemoteCreateFlags; // 1 bytes
	// logger 0x9a1590
};
POP_PACKED
ASSERT_SIZE(SN_RemoteSnapshotFromRemoteDoc, 69);
PUSH_PACKED
struct SN_RemoteSyncTarget
{
	enum { NET_ID = 62388 };
	u8 excludedFieldBits; // 1 bytes (bit3=!MoveDir)
	u32 remoteID; // 4 bytes
	u32 targetEntity; // 4 bytes
	float3 DestPos; // 12 bytes (ST_POS3)
	float3 MoveDir; // 12 bytes (ST_POS3, present if !(bits & 8))
	// logger 0x9a2163
};
POP_PACKED
ASSERT_SIZE(SN_RemoteSyncTarget, 33);

PUSH_PACKED
struct SN_RemoteSyncForecastCollision
{
	enum { NET_ID = 62389 };
	// variable part (ST_REMOTE_COLLISION_INFO):
	PUSH_PACKED
	struct RemoteCollisionInfo
	{
		u32 remoteID; // 4 bytes
		u32 targetEntity; // 4 bytes
		float3 DestPos; // 12 bytes (ST_POS3: fX, fY, fZ)
		f32 TimeOfImpact; // 4 bytes
		u8 Destroy; // 1 bytes (bool)
	};
	POP_PACKED
	u16 m_CollisionInfo_count; // 2 bytes
	RemoteCollisionInfo m_CollisionInfo[1]; // 25 bytes
	// logger 0x9a201d
};
POP_PACKED
ASSERT_SIZE(SN_RemoteSyncForecastCollision, 27);
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
	u8 reviveBySelf; // 1 bytes (bool)
	float3 position; // 12 bytes (ST_POS3: fX, fY, fZ)
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
	float3 position; // 12 bytes (ST_POS3: fX, fY, fZ)
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
	// variable part (members:VEC of wide strings, wire elem = u16 len + wchar[len]):
	u16 members_count; // 2 bytes
	struct
	{
		u16 len; // 2 bytes
		wchar_t str[1]; // 2 bytes
	} members[1]; // 4 bytes
	// logger 0x9a6170
};
POP_PACKED
ASSERT_SIZE(SN_StartGameNotQualified, 10);
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
	// variable part (ST_PROFILE_ITEM):
	PUSH_PACKED
	struct Item
	{
		ItemUID m_itemID; // 4 bytes
		u8 m_invenType; // 1 bytes
		u32 m_slot; // 4 bytes
		ItemDocIndex m_itemIndex; // 4 bytes
		u32 m_count; // 4 bytes
		u32 m_propertyGroupIndex; // 4 bytes
		u8 m_isLifeTimeAbsolute; // 1 bytes (bool)
		i64 m_lifeEndTimeUTC; // 8 bytes
		u16 m_properties_count; // 2 bytes
		// variable part (ST_ITEM_PROPERTY):
		PUSH_PACKED
		struct Property
		{
			u8 _Type; // 1 bytes
			u32 _TypeDetail; // 4 bytes
			u8 _ValueType; // 1 bytes
			f32 _Value; // 4 bytes
			u8 _Fixed; // 1 bytes (bool)
		};
		POP_PACKED
		Property m_properties[1]; // 11 bytes
	};
	POP_PACKED
	u16 updatedItemList_count; // 2 bytes
	Item updatedItemList[1]; // 43 bytes
	// logger 0x9a85b4
};
POP_PACKED
ASSERT_SIZE(SN_WarehouseItemUpdate, 45);
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
	// variable part (ST_REWARD_ITEM):
	PUSH_PACKED
	struct RewardItem
	{
		u32 m_ItemType; // 4 bytes
		u32 m_ItemIndex; // 4 bytes
		u32 m_ItemAmount; // 4 bytes
	};
	POP_PACKED
	u16 rewardItemList_count; // 2 bytes
	RewardItem rewardItemList[1]; // 12 bytes
	u32 spSlotIndex; // 4 bytes
	u8 vipSlotCount; // 1 bytes
	u8 guildSkillSlotCount; // 1 bytes
	u8 pcCafeSlotCount; // 1 bytes
	u16 pcCafeUserIds_count; // 2 bytes
	u32 pcCafeUserIds[1]; // 4 bytes
	// logger 0x9a336d
};
POP_PACKED
ASSERT_SIZE(SN_RewardItemList, 27);
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
	// rewardItem (ST_REWARD_ITEM: m_ItemType, m_ItemIndex, m_ItemAmount):
	PUSH_PACKED
	struct RewardItem
	{
		u32 m_ItemType; // 4 bytes
		u32 m_ItemIndex; // 4 bytes
		u32 m_ItemAmount; // 4 bytes
	};
	POP_PACKED
	RewardItem rewardItem; // 12 bytes
	u8 isSpecialSlot; // 1 bytes (bool)
	// logger 0x987b98
};
POP_PACKED
ASSERT_SIZE(SA_RewardSelectSlot, 21);
PUSH_PACKED
struct SA_RewardSelectSlotAllRandom
{
	enum { NET_ID = 62414 };
	u32 retval; // 4 bytes
	// variable part (u32 VEC):
	u16 slotIndexes_count; // 2 bytes
	u32 slotIndexes[1]; // 4 bytes
	// variable part (ST_REWARD_ITEM):
	PUSH_PACKED
	struct RewardItem
	{
		u32 m_ItemType; // 4 bytes
		u32 m_ItemIndex; // 4 bytes
		u32 m_ItemAmount; // 4 bytes
	};
	POP_PACKED
	u16 rewardItems_count; // 2 bytes
	RewardItem rewardItems[1]; // 12 bytes
	u32 spSlotIndex; // 4 bytes
	// logger 0x987cde
};
POP_PACKED
ASSERT_SIZE(SA_RewardSelectSlotAllRandom, 28);
PUSH_PACKED
struct SN_RewardStageEnd
{
	enum { NET_ID = 62415 };
	// variable part (ST_REWARD_ITEM):
	PUSH_PACKED
	struct RewardItem
	{
		u32 m_ItemType; // 4 bytes
		u32 m_ItemIndex; // 4 bytes
		u32 m_ItemAmount; // 4 bytes
	};
	POP_PACKED
	u16 rewardItemList_count; // 2 bytes
	RewardItem rewardItemList[1]; // 12 bytes
	// variable part (ST_REWARD_SELECT_INFO):
	PUSH_PACKED
	struct RewardSelectInfo
	{
		u32 usn; // 4 bytes
		u16 nickname_len; // 2 bytes
		wchar_t nickname[1]; // 2 bytes
		u16 pickedItemIndexes_count; // 2 bytes
		u32 pickedItemIndexes[1]; // 4 bytes
	};
	POP_PACKED
	u16 otherSelectList_count; // 2 bytes
	RewardSelectInfo otherSelectList[1]; // 14 bytes
	u32 spSlotIndex; // 4 bytes
	// logger 0x9a35f7
};
POP_PACKED
ASSERT_SIZE(SN_RewardStageEnd, 34);
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
	ItemUID orgItemID; // 4 bytes
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
	// variable part (ST_MAIL_EX):
	PUSH_PACKED
	struct MailEx
	{
		i64 mailId; // 8 bytes
		i64 sendUTCDate; // 8 bytes
		i64 expireUTCDate; // 8 bytes
		u8 isRead; // 1 bytes (bool)
		u8 isGMMail; // 1 bytes (bool)
		u8 hasAttachment; // 1 bytes (bool)
		u16 fromNickname_len; // 2 bytes
		wchar_t fromNickname[1]; // 2 bytes
		u16 subject_len; // 2 bytes
		wchar_t subject[1]; // 2 bytes
		u16 items_count; // 2 bytes
		u32 items[1]; // 4 bytes
	};
	POP_PACKED
	u16 mailList_count; // 2 bytes
	MailEx mailList[1]; // 41 bytes
	// logger 0x99a0bd
};
POP_PACKED
ASSERT_SIZE(SN_MailList, 45);
PUSH_PACKED
struct SN_MailBlocklist
{
	enum { NET_ID = 62420 };
	u8 packetNum; // 1 bytes
	// variable part (VEC of wide strings):
	u16 blockNicknameList_count; // 2 bytes
	wchar_t blockNicknameList[1]; // 2 bytes (1 wchar)
	// logger 0x999d11
};
POP_PACKED
ASSERT_SIZE(SN_MailBlocklist, 5);

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
	u8 isRead; // 1 bytes (bool)
	u8 isGMMail; // 1 bytes (bool)
	u16 fromNickname_len; // 2 bytes
	wchar_t fromNickname[1]; // 2 bytes (wide string)
	u16 subject_len; // 2 bytes
	wchar_t subject[1]; // 2 bytes (wide string)
	u16 content_len; // 2 bytes
	wchar_t content[1]; // 2 bytes (wide string)
	i64 sendUTCDate; // 8 bytes
	i64 expireUTCDate; // 8 bytes
	i64 money; // 8 bytes
	i64 exp; // 8 bytes
	i64 ccoin; // 8 bytes
	u32 guildFund; // 4 bytes
	// variable part (ST_MAIL_ITEM):
	PUSH_PACKED
	struct MailItem
	{
		u32 itemIndex; // 4 bytes
		u32 itemCount; // 4 bytes
	};
	POP_PACKED
	u16 itemList_count; // 2 bytes
	MailItem itemList[1]; // 8 bytes
	// logger 0x99a480
};
POP_PACKED
ASSERT_SIZE(SN_MailRead, 76);
PUSH_PACKED
struct SN_MailGetAttachmentResult
{
	enum { NET_ID = 62423 };
	u32 nErrorType; // 4 bytes
	i64 mailId; // 8 bytes
	i64 expireUTCDate; // 8 bytes
	// takenItems/remainItems: VEC<ST_MailItem>
	PUSH_PACKED
	struct ST_MailItem
	{
		u32 itemIndex; // 4 bytes
		u32 itemCount; // 4 bytes
	};
	POP_PACKED
	u16 takenItems_count; // 2 bytes
	ST_MailItem takenItems[1]; // 8 bytes
	u16 remainItems_count; // 2 bytes
	ST_MailItem remainItems[1]; // 8 bytes
	// logger 0x999e5a
};
POP_PACKED
ASSERT_SIZE(SN_MailGetAttachmentResult, 40);
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
	// eventIndexList: VEC<u32>
	u16 eventIndexList_count; // 2 bytes
	u32 eventIndexList[1]; // 4 bytes
	// logger 0x9944c3
};
POP_PACKED
ASSERT_SIZE(SN_GameEventActivated, 6);
PUSH_PACKED
struct SN_GameEventPending
{
	enum { NET_ID = 62428 };
	// variable part (VEC of u32 event indices):
	u16 eventIndexList_count; // 2 bytes
	u32 eventIndexList[1]; // 4 bytes
	// logger 0x994677
};
POP_PACKED
ASSERT_SIZE(SN_GameEventPending, 6);
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
	// coolTimeList: VEC<PST_Cooltime>
	PUSH_PACKED
	struct PST_Cooltime
	{
		u32 coolTimeType; // 4 bytes
		u32 coolTimeID; // 4 bytes
		u32 leftCoolTime; // 4 bytes
		u32 totalCoolTime; // 4 bytes
	};
	POP_PACKED
	u16 coolTimeList_count; // 2 bytes
	PST_Cooltime coolTimeList[1]; // 16 bytes
	// logger 0x99c2e8
};
POP_PACKED
ASSERT_SIZE(SN_NotifyCooltime, 22);
PUSH_PACKED
struct SN_MailListNewInbox
{
	enum { NET_ID = 62434 };
	u8 mailboxType; // 1 bytes
	// variable part (ST_MAIL_EX):
	PUSH_PACKED
	struct MailEx
	{
		i64 mailId; // 8 bytes
		i64 sendUTCDate; // 8 bytes
		i64 expireUTCDate; // 8 bytes
		u8 isRead; // 1 bytes (bool)
		u8 isGMMail; // 1 bytes (bool)
		u8 hasAttachment; // 1 bytes (bool)
		u16 fromNickname_len; // 2 bytes
		wchar_t fromNickname[1]; // 2 bytes
		u16 subject_len; // 2 bytes
		wchar_t subject[1]; // 2 bytes
		u16 items_count; // 2 bytes
		u32 items[1]; // 4 bytes
	};
	POP_PACKED
	u16 mailList_count; // 2 bytes
	MailEx mailList[1]; // 41 bytes
	// logger 0x99a261
};
POP_PACKED
ASSERT_SIZE(SN_MailListNewInbox, 44);
PUSH_PACKED
struct SN_PlayerServerPosition
{
	enum { NET_ID = 62435 };
	float3 Pos; // 12 bytes (ST_POS3: fX, fY, fZ)
	// ST_POS2:
	f32 fMoveDirX; // 4 bytes (fX)
	f32 fMoveDirY; // 4 bytes (fY)
	// logger 0x99dbf6
};
POP_PACKED
ASSERT_SIZE(SN_PlayerServerPosition, 20);
PUSH_PACKED
struct SN_RemoteServerPosition
{
	enum { NET_ID = 62436 };
	i64 remoteServerID; // 8 bytes
	float3 pos; // 12 bytes (ST_POS3: fX, fY, fZ)
	float3 dir; // 12 bytes (ST_POS3: fX, fY, fZ)
	u8 boundType; // 1 bytes
	u8 durationType; // 1 bytes
	u8 colorType; // 1 bytes
	float3 dim; // 12 bytes (ST_POS3: fX, fY, fZ)
	u8 flag; // 1 bytes
	// logger 0x9a1369
};
POP_PACKED
ASSERT_SIZE(SN_RemoteServerPosition, 48);
PUSH_PACKED
struct SN_MonsterServerPosition
{
	enum { NET_ID = 62437 };
	u32 monsterID; // 4 bytes
	float3 Pos; // 12 bytes (ST_POS3: fX, fY, fZ)
	f32 MoveDir_fX; // 4 bytes (ST_POS2: fX)
	f32 MoveDir_fY; // 4 bytes (ST_POS2: fY)
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
	// variable part (ST_PVP_RECORD):
	PUSH_PACKED
	struct PvpRecord
	{
		u8 gameType; // 1 bytes
		u8 pvpSaveType; // 1 bytes
		u32 winCount; // 4 bytes
		u32 loseCount; // 4 bytes
		u32 drawCount; // 4 bytes
		u32 leaveCount; // 4 bytes
		u32 killCount; // 4 bytes
		u32 deathCount; // 4 bytes
		u32 assistCount; // 4 bytes
		u32 score; // 4 bytes
		u32 data1; // 4 bytes
	};
	POP_PACKED
	u16 stPvpRecords_count; // 2 bytes
	PvpRecord stPvpRecords[1]; // 38 bytes
	// variable part (ST_PVP_OLYMPIC_RECORD):
	PUSH_PACKED
	struct PvpOlympicRecord
	{
		u8 pvpSaveType; // 1 bytes
		u32 firstCount; // 4 bytes
		u32 secondCount; // 4 bytes
		u32 thirdCount; // 4 bytes
		u32 timeRecord; // 4 bytes
	};
	POP_PACKED
	u16 stPvpOlympicRecords_count; // 2 bytes
	PvpOlympicRecord stPvpOlympicRecords[1]; // 17 bytes
	// logger 0x9867c6
};
POP_PACKED
ASSERT_SIZE(SA_PvpRecord, 59);
PUSH_PACKED
struct SA_PvpDetailRecord
{
	enum { NET_ID = 62442 };
	u32 gameType; // 4 bytes
	u8 pvpSaveType; // 1 bytes
	// stPvpMasterRecord (ST_PVP_MASTER_RECORD):
	PUSH_PACKED
	struct PvpMasterRecord
	{
		u32 mostPlayClass; // 4 bytes
		u32 mostPlayCount; // 4 bytes
		u32 mostWinClass; // 4 bytes
		u32 mostWinCount; // 4 bytes
	};
	POP_PACKED
	PvpMasterRecord stPvpMasterRecord; // 16 bytes
	// variable part (ST_PVP_RECENT_RECORD):
	PUSH_PACKED
	struct PvpRecentRecord
	{
		u8 resultType; // 1 bytes
		u32 fstClass; // 4 bytes
		u32 scndClass; // 4 bytes
		u16 myScore; // 2 bytes
		u16 myTeamScore; // 2 bytes
		u16 enemyTeamScore; // 2 bytes
		u8 myTeamPlayerCount; // 1 bytes
		u8 enemyTeamPlayerCount; // 1 bytes
		u8 killCount; // 1 bytes
		u8 deathCount; // 1 bytes
		u8 assistCount; // 1 bytes
		u8 occupyCount; // 1 bytes
		u32 pvpSpecificRecord; // 4 bytes
		i64 beginTimeStamp; // 8 bytes
	};
	POP_PACKED
	u16 recentGames_count; // 2 bytes
	PvpRecentRecord recentGames[1]; // 33 bytes
	u32 olympicTotalPlayCount; // 4 bytes
	u32 olympicWorldRecord; // 4 bytes
	u16 olympicWorldRecordNickname_len; // 2 bytes
	wchar_t olympicWorldRecordNickname[1]; // 2 bytes
	// logger 0x9863cc
};
POP_PACKED
ASSERT_SIZE(SA_PvpDetailRecord, 68);

PUSH_PACKED
struct SN_SummaryRewardResult
{
	enum { NET_ID = 62443 };
	u32 result; // 4 bytes
	// summaryInfo (PST_SUMMARY_INFO):
	PUSH_PACKED
	struct SummaryInfo
	{
		u32 summaryIndex; // 4 bytes
		u32 stageIndex; // 4 bytes
		u32 summaryType; // 4 bytes
		u32 summaryData; // 4 bytes
		u8 rewardReceived; // 1 bytes (bool)
		i64 completedDate; // 8 bytes
	};
	POP_PACKED
	SummaryInfo summaryInfo; // 25 bytes
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
	float3 pingPos; // 12 bytes (ST_POS3: fX, fY, fZ)
	u32 targetID; // 4 bytes
	u32 pingData; // 4 bytes
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
	u16 param1_len; // 2 bytes
	wchar_t param1[1]; // 2 bytes
	u16 param2_len; // 2 bytes
	wchar_t param2[1]; // 2 bytes
	// logger 0x9920a9
};
POP_PACKED
ASSERT_SIZE(SN_ErrorMessage, 17);

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
	// variable part (PST_SORTIE_CHARACTER_SLOT_INFO):
	PUSH_PACKED
	struct SortieCharacterSlotInfo
	{
		u32 creatureIndex; // 4 bytes
		u16 slotStates_count; // 2 bytes
		u32 slotStates[1]; // 4 bytes
	};
	POP_PACKED
	u16 enemiesSlotInfos_count; // 2 bytes
	SortieCharacterSlotInfo enemiesSlotInfos[1]; // 10 bytes
	// logger 0x9a4b2a
};
POP_PACKED
ASSERT_SIZE(SN_SortieMasterBanPhaseStart, 28);
PUSH_PACKED
struct SN_SortieMasterAssignPhaseStart
{
	enum { NET_ID = 62457 };
	u32 alliesLeaderUserId; // 4 bytes
	u32 enemiesLeaderUserId; // 4 bytes
	u32 timeSec; // 4 bytes
	u32 assignCount; // 4 bytes
	// variable part (ST_SORTIE_CHARACTER_SLOT_INFO):
	PUSH_PACKED
	struct SlotInfo
	{
		u32 creatureIndex; // 4 bytes
		u16 slotStates_count; // 2 bytes
		u32 slotStates[1]; // 4 bytes
	};
	POP_PACKED
	u16 enemiesSlotInfos_count; // 2 bytes
	SlotInfo enemiesSlotInfos[1]; // 10 bytes
	// logger 0x9a4761
};
POP_PACKED
ASSERT_SIZE(SN_SortieMasterAssignPhaseStart, 28);
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
	// variable part (ST_SORTIE_MASTER_BAN_INFO): u16 count + u32 per element
	u16 alliesBanInfo_count; // 2 bytes
	u32 alliesBanInfo[1]; // 4 bytes
	u16 enemiesBanInfo_count; // 2 bytes
	u32 enemiesBanInfo[1]; // 4 bytes
	// logger 0x9a4a18
};
POP_PACKED
ASSERT_SIZE(SN_SortieMasterBanPhaseEnd, 12);
PUSH_PACKED
struct SN_SortieMasterAssignPhaseEnd
{
	enum { NET_ID = 62464 };
	// alliesAssignInfo / enemiesAssignInfo (PST_SORTIE_MASTER_ASSIGN_INFO):
	PUSH_PACKED
	struct AssignInfo
	{
		u32 assignedUserId; // 4 bytes
		u16 candidateUserIds_count; // 2 bytes
		u32 candidateUserIds[1]; // 4 bytes (VEC)
		u16 assignCreatureIndexes_count; // 2 bytes
		u32 assignCreatureIndexes[1]; // 4 bytes (VEC)
	};
	POP_PACKED
	AssignInfo alliesAssignInfo; // 16 bytes
	AssignInfo enemiesAssignInfo; // 16 bytes
	// logger 0x9a464f
};
POP_PACKED
ASSERT_SIZE(SN_SortieMasterAssignPhaseEnd, 32);
PUSH_PACKED
struct SA_TierStageRecord
{
	enum { NET_ID = 62470 };
	u8 seasonId; // 1 bytes
	// stageRecord (PST_TIER_STAGE_RECORD):
	PUSH_PACKED
	struct StageRecord
	{
		u8 pvpSaveType; // 1 bytes
		u8 tierType; // 1 bytes
		u16 tierPoint; // 2 bytes
		u16 sectorId; // 2 bytes
		u8 sectorRanking; // 1 bytes
		u32 seasonalWin; // 4 bytes
		u32 seasonalDraw; // 4 bytes
		u32 seasonalLose; // 4 bytes
		u32 seasonalLeave; // 4 bytes
	};
	POP_PACKED
	StageRecord stageRecord; // 23 bytes
	u32 sectorId; // 4 bytes
	// variable part (PST_TIER_SECTOR_RANKING_NODE):
	PUSH_PACKED
	struct RankingNode
	{
		u8 sectorRanking; // 1 bytes
		u16 nickname_len; // 2 bytes
		wchar_t nickname[1]; // 2 bytes
		u16 tierPoint; // 2 bytes
		u32 totalWin; // 4 bytes
		u32 recentWin; // 4 bytes
		u32 recentLose; // 4 bytes
		u32 recentDraw; // 4 bytes
		u32 recentLeave; // 4 bytes
	};
	POP_PACKED
	u16 sectorRankingList_count; // 2 bytes
	RankingNode sectorRankingList[1]; // 27 bytes
	// logger 0x989966
};
POP_PACKED
ASSERT_SIZE(SA_TierStageRecord, 57);
PUSH_PACKED
struct SA_PvpRanking
{
	enum { NET_ID = 62471 };
	u8 seasonId; // 1 bytes
	u8 pvpSaveType; // 1 bytes
	// variable part (SA_PVP_RANKING rankingList):
	PUSH_PACKED
	struct PvpRanking
	{
		u8 ranking; // 1 bytes
		u16 nickname_len; // 2 bytes
		wchar_t nickname[1]; // 2 bytes
		u8 tierType; // 1 bytes
		u16 tierPoint; // 2 bytes
		u32 totalWin; // 4 bytes
		u32 recentWin; // 4 bytes
		u32 recentLose; // 4 bytes
		u32 recentDraw; // 4 bytes
		u32 recentLeave; // 4 bytes
	};
	POP_PACKED
	u16 rankingList_count; // 2 bytes
	PvpRanking rankingList[1]; // 28 bytes
	// logger 0x986622
};
POP_PACKED
ASSERT_SIZE(SA_PvpRanking, 32);
PUSH_PACKED
struct SA_PveRanking
{
	enum { NET_ID = 62475 };
	u32 stageId; // 4 bytes
	u8 packetNum; // 1 bytes
	// highScore (ST_PVE_RANKING):
	PUSH_PACKED
	struct PveRanking
	{
		u32 ranking; // 4 bytes
		u32 totalScore; // 4 bytes
		u32 playTime; // 4 bytes
		u32 deathCount; // 4 bytes
		// variable part (ST_MEMBER):
		PUSH_PACKED
		struct Member
		{
			u32 mainClassType; // 4 bytes
			u32 subClassType; // 4 bytes
			u16 nickname_len; // 2 bytes
			wchar_t nickname[1]; // 2 bytes
		};
		POP_PACKED
		u16 member_count; // 2 bytes
		Member member[1]; // 12 bytes
	};
	POP_PACKED
	u8 highScoreClearRankType; // 1 bytes
	// variable part (ST_PVE_RANKING):
	u16 rankingList_count; // 2 bytes
	PveRanking rankingList[1]; // 30 bytes
	// logger 0x986113
};
POP_PACKED
ASSERT_SIZE(SA_PveRanking, 38);
PUSH_PACKED
struct SA_MyPveRanking
{
	enum { NET_ID = 62476 };
	u32 stageIndex; // 4 bytes
	// highScore (ST_PVE_RANKING):
	PUSH_PACKED
	struct PveRanking
	{
		u32 ranking; // 4 bytes
		u32 totalScore; // 4 bytes
		u32 playTime; // 4 bytes
		u32 deathCount; // 4 bytes
		// variable part (ST_MEMBER):
		PUSH_PACKED
		struct Member
		{
			u32 mainClassType; // 4 bytes
			u32 subClassType; // 4 bytes
			u16 nickname_len; // 2 bytes
			wchar_t nickname[1]; // 2 bytes
		};
		POP_PACKED
		u16 member_count; // 2 bytes
		Member member[1]; // 12 bytes
	};
	POP_PACKED
	PveRanking highScore; // 30 bytes
	u8 highScoreClearRankType; // 1 bytes
	// logger 0x984b97
};
POP_PACKED
ASSERT_SIZE(SA_MyPveRanking, 35);
PUSH_PACKED
struct SN_PveLastgameRanking
{
	enum { NET_ID = 62477 };
	u32 stageId; // 4 bytes
	u8 packetNum; // 1 bytes
	// variable part (ST_PVE_RANKING):
	PUSH_PACKED
	struct PveRanking
	{
		u32 ranking; // 4 bytes
		u32 totalScore; // 4 bytes
		u32 playTime; // 4 bytes
		u32 deathCount; // 4 bytes
		// variable part (ST_MEMBER):
		PUSH_PACKED
		struct Member
		{
			u32 mainClassType; // 4 bytes
			u32 subClassType; // 4 bytes
			u16 nickname_len; // 2 bytes
			wchar_t nickname[1]; // 2 bytes
		};
		POP_PACKED
		u16 member_count; // 2 bytes
		Member member[1]; // 12 bytes
	};
	POP_PACKED
	u16 rankingList_count; // 2 bytes
	PveRanking rankingList[1]; // 30 bytes
	u32 ranking; // 4 bytes
	// logger 0x99f7ea
};
POP_PACKED
ASSERT_SIZE(SN_PveLastgameRanking, 41);
PUSH_PACKED
struct SN_DefenceLastgameRanking
{
	enum { NET_ID = 62478 };
	u32 stageId; // 4 bytes
	u8 packetNum; // 1 bytes
	// variable part (ST_DEFENCE_RANKING):
	PUSH_PACKED
	struct Ranking
	{
		u32 ranking; // 4 bytes
		u32 clearStepIndex; // 4 bytes
		u32 clearTime; // 4 bytes
		// variable part (ST_MEMBER):
		PUSH_PACKED
		struct Member
		{
			u32 mainClassType; // 4 bytes
			u32 subClassType; // 4 bytes
			u16 nickname_len; // 2 bytes
			wchar_t nickname[1]; // 2 bytes
		};
		POP_PACKED
		u16 member_count; // 2 bytes
		Member member[1]; // 12 bytes
	};
	POP_PACKED
	u16 rankingList_count; // 2 bytes
	Ranking rankingList[1]; // 26 bytes
	u32 ranking; // 4 bytes
	// logger 0x990cfb
};
POP_PACKED
ASSERT_SIZE(SN_DefenceLastgameRanking, 37);
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
	// variable part (VEC of u32):
	u16 eventIndexList_count; // 2 bytes
	u32 eventIndexList[1]; // 4 bytes
	// logger 0x98b64c
};
POP_PACKED
ASSERT_SIZE(SN_ActiveIngameEventList, 6);
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
	// stAnnounceMent (ST_PVP_ANNOUNCEMENT):
	PUSH_PACKED
	struct PvpAnnouncement
	{
		u32 announcementType; // 4 bytes
		u32 announcementData; // 4 bytes
		u32 score; // 4 bytes
		u32 attackerID; // 4 bytes
		u32 attackeeID; // 4 bytes
		u8 isAllDead; // 1 bytes (bool)
	};
	POP_PACKED
	PvpAnnouncement stAnnounceMent; // 21 bytes
	// logger 0x99fcd2
};
POP_PACKED
ASSERT_SIZE(SN_PvpEventAnnouncement, 21);
PUSH_PACKED
struct SA_ChatChannelInvite
{
	enum { NET_ID = 62489 };
	u32 retval; // 4 bytes
	// inviteeNickname (wide string, reader 0xa19080):
	u16 inviteeNickname_len; // 2 bytes
	wchar_t inviteeNickname[1]; // 2 bytes (wire: 2 + count*2)
	// logger 0x97e42d
};
POP_PACKED
ASSERT_SIZE(SA_ChatChannelInvite, 8);

PUSH_PACKED
struct SN_ChatChannelInvite
{
	enum { NET_ID = 62490 };
	u16 inviterName_len; // 2 bytes
	wchar_t inviterName[1]; // 2 bytes
	u16 channelID_len; // 2 bytes
	wchar_t channelID[1]; // 2 bytes
	u32 channelCenter; // 4 bytes
	u32 channelType; // 4 bytes
	u16 channelName_len; // 2 bytes
	wchar_t channelName[1]; // 2 bytes
	u16 channelPassword_len; // 2 bytes
	wchar_t channelPassword[1]; // 2 bytes
	u8 bInvite; // 1 bytes (bool)
	// logger 0x98f5af
};
POP_PACKED
ASSERT_SIZE(SN_ChatChannelInvite, 25);

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
	u16 inviteeNickname_len; // 2 bytes
	wchar_t inviteeNickname[1]; // 2*len bytes (wide string)
	u16 channelName_len; // 2 bytes
	wchar_t channelName[1]; // 2*len bytes (wide string)
	// logger 0x98f747
};
POP_PACKED
// variable length: 4 + (2 + 2*inviteeNickname_len) + (2 + 2*channelName_len); no static ASSERT_SIZE

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
	float3 pos; // 12 bytes (ST_POS3: fX, fY, fZ)
	// logger 0x9a9180
};
POP_PACKED
ASSERT_SIZE(SQ_TeleportObject, 16);
PUSH_PACKED
struct SN_PlayerSyncTeleport
{
	enum { NET_ID = 62498 };
	u32 objectID; // 4 bytes
	float3 pos; // 12 bytes (ST_POS3: fX, fY, fZ)
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
	// variable part (PST_SCORE_PAIR):
	PUSH_PACKED
	struct ScorePair
	{
		u32 type; // 4 bytes
		u32 value; // 4 bytes
	};
	POP_PACKED
	u16 records_count; // 2 bytes
	ScorePair records[1]; // 8 bytes
	// logger 0x9a3d3a
};
POP_PACKED
ASSERT_SIZE(SN_ScoreUpdate, 18);
PUSH_PACKED
struct SN_AIMonsterSyncMove
{
	enum { NET_ID = 62505 };
	u8 excludedFieldBits; // 1 bytes
	u32 monsterID; // 4 bytes
	i16 upperBodyPitch; // 2 bytes
	i16 upperBodyYaw; // 2 bytes
	i16 lowerBodyYaw; // 2 bytes
	i16 angularSpeed; // 2 bytes
	u16 linearSpeed; // 2 bytes
	float3 currPos; // 12 bytes (ST_POS3)
	float3 goalPos; // 12 bytes (ST_POS3)
	u8 movingFlags; // 1 bytes
	// logger 0x98ba71
};
POP_PACKED
ASSERT_SIZE(SN_AIMonsterSyncMove, 40);

PUSH_PACKED
struct SN_AIMonsterSyncTurn
{
	enum { NET_ID = 62506 };
	u8 excludedFieldBits; // 1 bytes
	u32 monsterID; // 4 bytes
	i16 upperBodyPitch; // 2 bytes
	i16 upperBodyYaw; // 2 bytes
	i16 lowerBodyYaw; // 2 bytes
	i16 angularSpeed; // 2 bytes
	// logger 0x98bccb
};
POP_PACKED
ASSERT_SIZE(SN_AIMonsterSyncTurn, 13);

PUSH_PACKED
struct SA_ItemTrade
{
	enum { NET_ID = 62507 };
	u32 result; // 4 bytes
	i64 goldAmount; // 8 bytes
	// variable part (vector of LogPST_ItemSimpleInfo @0x97a37a):
	PUSH_PACKED
	struct ItemSimpleInfo
	{
		ItemUID itemId; // 4 bytes
		ItemDocIndex itemDocIndex; // 4 bytes
		u32 itemCount; // 4 bytes
	};
	POP_PACKED
	u16 tradedItemList_count; // 2 bytes
	ItemSimpleInfo tradedItemList[1]; // 12 bytes
	// logger 0x983d19
};
POP_PACKED
ASSERT_SIZE(SA_ItemTrade, 26);
PUSH_PACKED
struct SA_ItemCraft
{
	enum { NET_ID = 62508 };
	u8 craftingItemType; // 1 bytes
	u32 result; // 4 bytes
	// variable part (ItemSimpleInfo):
	PUSH_PACKED
	struct ItemSimpleInfo
	{
		ItemUID itemId; // 4 bytes
		ItemDocIndex itemDocIndex; // 4 bytes
		u32 itemCount; // 4 bytes
	};
	POP_PACKED
	u16 craftedItemList_count; // 2 bytes
	ItemSimpleInfo craftedItemList[1]; // 12 bytes
	// logger 0x983715
};
POP_PACKED
ASSERT_SIZE(SA_ItemCraft, 19);
PUSH_PACKED
struct SA_ItemDisassemble
{
	enum { NET_ID = 62509 };
	u8 craftingItemType; // 1 bytes
	u32 result; // 4 bytes
	// variable part (PST_ITEM_SIMPLE_INFO):
	PUSH_PACKED
	struct ItemSimpleInfo
	{
		ItemUID itemId; // 4 bytes
		ItemDocIndex itemDocIndex; // 4 bytes
		u32 itemCount; // 4 bytes
	};
	POP_PACKED
	u16 dissolvedItemList_count; // 2 bytes
	ItemSimpleInfo dissolvedItemList[1]; // 12 bytes
	// logger 0x983960
};
POP_PACKED
ASSERT_SIZE(SA_ItemDisassemble, 19);
PUSH_PACKED
struct SA_OlympicBestRecord
{
	enum { NET_ID = 62510 };
	u32 personalRecord; // 4 bytes
	u32 worldRecord; // 4 bytes
	u16 worldRecordNickname_len; // 2 bytes (u16 wchar count)
	wchar_t worldRecordNickname[1]; // 2 bytes (wide string, count*2 payload)
	// logger 0x9850a4
};
POP_PACKED
ASSERT_SIZE(SA_OlympicBestRecord, 12);

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
	// variable part (ST_OLYMPIC_RECORD):
	PUSH_PACKED
	struct OlympicRecord
	{
		u32 usn; // 4 bytes
		u8 ranking; // 1 bytes
		u32 record; // 4 bytes
		u8 isFinished; // 1 bytes (bool)
		u8 isDisconnected; // 1 bytes (bool)
	};
	POP_PACKED
	u16 rankingList_count; // 2 bytes
	OlympicRecord rankingList[1]; // 11 bytes
	// logger 0x99c9d8
};
POP_PACKED
ASSERT_SIZE(SN_OlympicIngameRecords, 13);
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
	// initStat (ST_BASE_STAT):
	PUSH_PACKED
	struct Stat
	{
		u8 type; // 1 bytes
		f32 value; // 4 bytes
	};
	POP_PACKED
	u16 maxStats_count; // 2 bytes
	Stat maxStats[1]; // 5 bytes
	u16 curStats_count; // 2 bytes
	Stat curStats[1]; // 5 bytes
	f32 baseMoveSpeed; // 4 bytes
	u8 action; // 1 bytes (bool)
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
	// initStat (ST_BASE_STAT):
	PUSH_PACKED
	struct Stat
	{
		u8 type; // 1 bytes
		f32 value; // 4 bytes
	};
	POP_PACKED
	u16 maxStats_count; // 2 bytes
	Stat maxStats[1]; // 5 bytes
	u16 curStats_count; // 2 bytes
	Stat curStats[1]; // 5 bytes
	f32 baseMoveSpeed; // 4 bytes
	u8 action; // 1 bytes
	// logger 0x98ed6a
};
POP_PACKED
ASSERT_SIZE(SN_ChangeCharacterMode, 43);
PUSH_PACKED
struct SN_ItemOptiongroupList
{
	enum { NET_ID = 62520 };
	// variable part (optionGroupList VEC of PST_ITEM_OPTIONGROUP):
	PUSH_PACKED
	struct OptionGroup
	{
		u32 optionGroupIndex; // 4 bytes
		// variable part (fixed VEC of PST_ITEM_OPTIONGROUP_PROPERTY):
		PUSH_PACKED
		struct Property
		{
			u8 propType; // 1 bytes
			u32 propTypeDetail; // 4 bytes
			u8 valueType; // 1 bytes
			f32 minValue; // 4 bytes
			f32 maxValue; // 4 bytes
		};
		POP_PACKED
		u16 fixed_count; // 2 bytes
		Property fixed[1]; // 14 bytes
	};
	POP_PACKED
	u16 optionGroupList_count; // 2 bytes
	OptionGroup optionGroupList[1]; // 20 bytes
	// logger 0x998e02
};
POP_PACKED
ASSERT_SIZE(SN_ItemOptiongroupList, 22);
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
	u8 isPlaying; // 1 bytes (bool)
	f32 aniMoveSpeedTimeSec; // 4 bytes
	f32 progress; // 4 bytes
	float3 pos; // 12 bytes (ST_POS3: fX, fY, fZ)
	u32 curLoopCount; // 4 bytes
	u32 maxLoopCount; // 4 bytes
	i64 serverTime; // 8 bytes
	i64 startTime; // 8 bytes
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
	// nickname: wide string (wire = u16 char count + count*2 wchar bytes)
	u16 nickname_len; // 2 bytes
	wchar_t nickname[1]; // 2 bytes
	// logger 0x9a8188
};
POP_PACKED


PUSH_PACKED
struct SN_NcguardMsg
{
	enum { NET_ID = 62529 };
	// variable part (byte string reader 0x00a13580 = u16 len + len bytes):
	u16 msg_count; // 2 bytes
	char msg[1]; // 1 bytes
	// logger 0x99bf4e
};
POP_PACKED
ASSERT_SIZE(SN_NcguardMsg, 3);
PUSH_PACKED
struct SN_TenprotectPunishment
{
	enum { NET_ID = 62530 };
	u8 punishMode; // 1 bytes
	u16 punishReason_len; // 2 bytes
	char punishReason[1]; // 1 bytes (ANSI string)
	// logger 0x9a6f50
};
POP_PACKED
ASSERT_SIZE(SN_TenprotectPunishment, 4);

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
	// stAnnounceMent (ST_PVP_ANNOUNCEMENT):
	PUSH_PACKED
	struct PvpAnnouncement
	{
		u32 announcementType; // 4 bytes
		u32 announcementData; // 4 bytes
		u32 score; // 4 bytes
		u32 attackerID; // 4 bytes
		u32 attackeeID; // 4 bytes
		u8 isAllDead; // 1 bytes (bool)
	};
	POP_PACKED
	PvpAnnouncement stAnnounceMent; // 21 bytes
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
	// variable part (calendar:VEC):
	PUSH_PACKED
	struct CalendarElement
	{
		i64 dateTime; // 8 bytes
		// variable part (calendarData:VEC):
		PUSH_PACKED
		struct CalendarSchedule
		{
			u8 dataType; // 1 bytes
			u32 index; // 4 bytes
			i64 startDateTime; // 8 bytes
			i64 endDateTime; // 8 bytes
		};
		POP_PACKED
		u16 calendarData_count; // 2 bytes
		CalendarSchedule calendarData[1]; // 21 bytes
	};
	POP_PACKED
	u16 calendar_count; // 2 bytes
	CalendarElement calendar[1]; // 31 bytes
	// logger 0x98e453
};
POP_PACKED
ASSERT_SIZE(SN_Calendar, 33);
PUSH_PACKED
struct SN_StageSkillAvailable
{
	enum { NET_ID = 62546 };
	// variable part (STAGE_SKILL_LIST: vector of u32):
	u16 stageSkillList_count; // 2 bytes
	u32 stageSkillList[1]; // 4 bytes
	u32 currentSkillIndex; // 4 bytes
	// logger 0x9a5e99
};
POP_PACKED
ASSERT_SIZE(SN_StageSkillAvailable, 10);
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
	// variable part (PST_ACCOUNT_EQUIPMENT):
	PUSH_PACKED
	struct Equipment
	{
		u8 etcType; // 1 bytes
		u32 docIndex; // 4 bytes
		u16 count; // 2 bytes
	};
	POP_PACKED
	u16 itemList_count; // 2 bytes
	Equipment itemList[1]; // 7 bytes
	// logger 0x98d6b5
};
POP_PACKED
ASSERT_SIZE(SN_AvailableAccountEquipmentList, 9);
PUSH_PACKED
struct SN_ItemLifetimeExpired
{
	enum { NET_ID = 62551 };
	// itemDocIndexList (ST_VEC of u32 doc indexes):
	u16 itemDocIndexList_count; // 2 bytes
	u32 itemDocIndexList[1]; // 4 bytes
	// logger 0x998cf7
};
POP_PACKED
ASSERT_SIZE(SN_ItemLifetimeExpired, 6);
PUSH_PACKED
struct SN_EffectLifetimeExpired
{
	enum { NET_ID = 62552 };
	// variable part (VEC of u32 effect item doc indexes):
	u16 effectItemDocIndexList_count; // 2 bytes
	u32 effectItemDocIndexList[1]; // 4 bytes
	// logger 0x991a1d
};
POP_PACKED
ASSERT_SIZE(SN_EffectLifetimeExpired, 6);
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
	// variable part (PST_EFFECT_LIFETIME):
	PUSH_PACKED
	struct EffectLifetime
	{
		u32 effectItemDocIndex; // 4 bytes
		i64 expireDateTime; // 8 bytes
	};
	POP_PACKED
	u16 effectItemDocIndexList_count; // 2 bytes
	EffectLifetime effectItemDocIndexList[1]; // 12 bytes
	// logger 0x991b28
};
POP_PACKED
ASSERT_SIZE(SN_EffectLifetimeList, 14);
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
	// variable part (vector of u32):
	u16 lossTiming_count; // 2 bytes
	u32 lossTiming[1]; // 4 bytes
	// logger 0x99b8a5
};
POP_PACKED
ASSERT_SIZE(SN_MonsterMineralLossTiming, 10);
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
	// variable part (VEC of u32):
	u16 masterList_count; // 2 bytes
	u32 masterList[1]; // 4 bytes
	// logger 0x995f3c
};
POP_PACKED
ASSERT_SIZE(SN_GmDisabledMasters, 6);
PUSH_PACKED
struct SN_GmDisabledSkins
{
	enum { NET_ID = 62573 };
	// variable part (wide-string vector, per-element label PST_CHARACTER_SKIN_INFO):
	u16 skinKeyList_count; // 2 bytes
	PUSH_PACKED
	struct PST_CHARACTER_SKIN_INFO
	{
		u16 len; // 2 bytes
		wchar_t str[1]; // 2 bytes
	};
	POP_PACKED
	PST_CHARACTER_SKIN_INFO skinKeyList[1]; // 4 bytes
	// logger 0x996047
};
POP_PACKED
ASSERT_SIZE(SN_GmDisabledSkins, 6);

PUSH_PACKED
struct SN_GmDisabledStages
{
	enum { NET_ID = 62574 };
	// variable part (ST_GM_DISABLED_STAGE):
	PUSH_PACKED
	struct GmDisabledStage
	{
		u32 stageIndex; // 4 bytes
		u8 gameType; // 1 bytes
	};
	POP_PACKED
	u16 stageList_count; // 2 bytes
	GmDisabledStage stageList[1]; // 5 bytes
	// logger 0x996160
};
POP_PACKED
ASSERT_SIZE(SN_GmDisabledStages, 7);
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
	// variable part (ST_CHARACTER_PROPERTY_INFO):
	PUSH_PACKED
	struct PropertyInfo
	{
		u8 propertyIndex; // 1 bytes
		u8 propertyLevel; // 1 bytes
	};
	POP_PACKED
	u16 propertyInfos_count; // 2 bytes
	PropertyInfo propertyInfos[1]; // 2 bytes
	// logger 0x98f1bb
};
POP_PACKED
ASSERT_SIZE(SN_CharacterPropertyInfo, 8);
PUSH_PACKED
struct SN_GameRecordInfoList
{
	enum { NET_ID = 62585 };
	// variable part (ST_PACKET_REC_INFO):
	PUSH_PACKED
	struct PacketRecInfo
	{
		u32 elapsedTime; // 4 bytes
		u16 packetData_len; // 2 bytes
		u8 packetData[1]; // 1 bytes
	};
	POP_PACKED
	u16 packetRecInfos_count; // 2 bytes
	PacketRecInfo packetRecInfos[1]; // 7 bytes
	// logger 0x9957f0
};
POP_PACKED
ASSERT_SIZE(SN_GameRecordInfoList, 9);
PUSH_PACKED
struct SA_ChatGetInviteeGameaccountid
{
	enum { NET_ID = 62586 };
	u32 result; // 4 bytes
	u32 inviteeCenterId; // 4 bytes
	u16 inviteeNickname_len; // 2 bytes
	wchar_t inviteeNickname[1]; // 2 bytes
	u16 inviteeGameAccountId_len; // 2 bytes
	char inviteeGameAccountId[1]; // 1 bytes
	// logger 0x97e5a8
};
POP_PACKED
ASSERT_SIZE(SA_ChatGetInviteeGameaccountid, 15);

PUSH_PACKED
struct SA_UseCoupon
{
	enum { NET_ID = 62587 };
	u32 result; // 4 bytes
	// variable part (ANSI string):
	u16 couponKey_len; // 2 bytes
	char couponKey[1]; // len bytes
	// logger 0x98a008
};
POP_PACKED
ASSERT_SIZE(SA_UseCoupon, 7);

PUSH_PACKED
struct SN_NotifyChattingChannelStatus
{
	enum { NET_ID = 62588 };
	u16 channelId_len; // 2 bytes
	wchar_t channelId[1]; // 2 bytes
	u16 channelCenter_len; // 2 bytes
	wchar_t channelCenter[1]; // 2 bytes
	u16 password_len; // 2 bytes
	wchar_t password[1]; // 2 bytes
	u16 token_len; // 2 bytes
	wchar_t token[1]; // 2 bytes
	u16 argument_len; // 2 bytes
	wchar_t argument[1]; // 2 bytes
	u32 reason; // 4 bytes
	// logger 0x99c178
};
POP_PACKED
ASSERT_SIZE(SN_NotifyChattingChannelStatus, 24);

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
	// authnToken (byte string, wire = u16 len + len bytes):
	u16 authnToken_len; // 2 bytes
	char authnToken[1]; // 1 bytes (variable)
	// logger 0x986fb9
};
POP_PACKED


PUSH_PACKED
struct SN_HudEventList
{
	enum { NET_ID = 62591 };
	// variable part (PST_HUD_EVENT):
	PUSH_PACKED
	struct Event
	{
		u8 eventType; // 1 bytes
		u16 bannerUrl_len; // 2 bytes
		char bannerUrl[1]; // 1 bytes
		u16 parameter_len; // 2 bytes
		char parameter[1]; // 1 bytes
	};
	POP_PACKED
	u16 eventList_count; // 2 bytes
	Event eventList[1]; // 7 bytes
	// logger 0x998167
};
POP_PACKED
ASSERT_SIZE(SN_HudEventList, 9);
PUSH_PACKED
struct SN_CshopEventList
{
	enum { NET_ID = 62592 };
	// variable part (PST_CSHOP_EVENT):
	PUSH_PACKED
	struct Event
	{
		u8 eventType; // 1 bytes
		u8 positionType; // 1 bytes
		u32 goodsId; // 4 bytes
		u16 bannerUrl_len; // 2 bytes
		char bannerUrl[1]; // 1 bytes
	};
	POP_PACKED
	u16 eventList_count; // 2 bytes
	Event eventList[1]; // 9 bytes
	// logger 0x99038e
};
POP_PACKED
ASSERT_SIZE(SN_CshopEventList, 11);
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
	// variable part (PST_PRAISE_TARGET_INFO):
	PUSH_PACKED
	struct PraiseTargetInfo
	{
		u32 usn; // 4 bytes
		u16 nickName_len; // 2 bytes
		wchar_t nickName[1]; // 2 bytes (wide string)
		u32 teamType; // 4 bytes
		u32 activeCreatureIndex; // 4 bytes
		u32 inActiveCreatureIndex; // 4 bytes
		u32 praiseType; // 4 bytes
		u32 value; // 4 bytes
		u32 praiseRelationType; // 4 bytes
		u32 relationValue; // 4 bytes
	};
	POP_PACKED
	u16 praiseTargetInfos_count; // 2 bytes
	PraiseTargetInfo praiseTargetInfos[1]; // 34 + 2*nickName_len bytes
	// logger 0x99ea82
};
POP_PACKED
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
	u16 nickName_len; // 2 bytes
	wchar_t nickName[1]; // 2 bytes
	u32 praiseCount; // 4 bytes
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
