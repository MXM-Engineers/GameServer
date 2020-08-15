#pragma once
#include <common/base.h>

#define ASSERT_SIZE(T, SIZE) STATIC_ASSERT(sizeof(T) == SIZE)

struct NetHeader
{
	u16 size;
	u16 netID;
};

ASSERT_SIZE(NetHeader, 4);

// Client packets
namespace Cl {

struct Hello
{
	enum { NET_ID = 60002 };

	char key[13];
};

ASSERT_SIZE(Hello, 13);

struct UserLogin
{
	enum { NET_ID = 60003 };


	u8 data[1]; // variable size
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

struct RequestConnectGame
{
	enum { NET_ID = 60008 };

	u16 nickLen;
	wchar nick[1]; // length is nickLen
	i32 var;
};

struct ReadyToLoadCharacter
{
	enum { NET_ID = 60014 };
};

struct MapIsLoaded
{
	enum { NET_ID = 60034 };
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

	i32 characterID;
};

struct Unknown_60148
{
	enum { NET_ID = 60148 };
};

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

	i32 objectID;
	i32 nType;
	i32 nIDX;
	i32 dwLocalID;
	Vec3 p3nPos;
	Vec3 p3nDir;
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
	Vec3 p3nPos;
};

struct SQ_CityLobbyJoinCity
{
	enum { NET_ID = 62033 };
};

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
		i32 skillIndex;
		i32 coolTime;
		u8 unlocked;

		u16 propList_count;
		Property propList[1];

		u8 isUnlocked;
		u8 isActivated;
	};

	i32 characterID;

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

PUSH_PACKED
struct SN_Money
{
	enum { NET_ID = 62057 };

	i64 nMoney;
	i32 nReason;
};
POP_PACKED

ASSERT_SIZE(SN_Money, 12);

struct SN_SetGameGvt
{
	enum { NET_ID = 62060 };

	i32 sendTime;
	i32 virtualTime;
};

ASSERT_SIZE(SN_SetGameGvt, 8);

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

	i32 playerID;
	u16 name_len;
	wchar name[1];
	i32 class_;
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

	i32 cityMapID;
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
		i64 expireDateTime64;
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

struct SA_GetCharacterInfo
{
	enum { NET_ID = 62113 };

	i32 characterID;
	i32 docIndex;
	i32 class_;
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

struct SN_LeaderCharacter
{
	enum { NET_ID = 62123 };

	i32 leaderID; // characterIndex
	i32 skinIndex;
};
ASSERT_SIZE(SN_LeaderCharacter, 8);


struct SN_ProfileCharacters
{
	enum { NET_ID = 62124 };

	PUSH_PACKED
	struct Character
	{
		i32 characterID;
		i32 creatureIndex;
		i32 skillShot1;
		i32 skillShot2;
		i32 class_;
		f32 x;
		f32 y;
		f32 z;
		i32 characterType;
		i32 skinIndex;
		i32 weaponIndex;
		u8 masterGearNo;
	};
	POP_PACKED

	u16 charaList_count;
	Character chara[1];
};

struct SN_ProfileItems
{
	enum { NET_ID = 62125 };

	PUSH_PACKED
	struct Property
	{
		u8 type;
		i32 typeDetail;
		u8 valueType;
		f32 value;
		u8 fixed;
	};
	POP_PACKED

	struct Item
	{
		i32 itemID;
		u8 invenType;
		i32 slot;
		i32 itemIndex;
		i32 count;
		i32 propertyGroupIndex;
		u8 isLifeTimeAbsolute;
		i64 lifeEndTimeUTC;
	};

	u8 packetNum;
	u16 item_count;
	Item items[1];
};

struct SN_ProfileWeapons
{
	enum { NET_ID = 62126 };

	PUSH_PACKED
	struct Weapon
	{
		i32 characterID;
		i32 weaponType;
		i32 weaponIndex;
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
		i32 characterID;
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

	// TODO: reverse and send this packet
};

struct SN_BlockList
{
	enum { NET_ID = 62261 };

	// TODO: reverse and send this packet
};

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

	// TODO: reverse and send this packet
};

struct SA_GetGuildMemberList
{
	enum { NET_ID = 62300 };

	// TODO: reverse and send this packet
};

struct SA_GetGuildHistoryList
{
	enum { NET_ID = 62302 };

	// TODO: reverse and send this packet
};

struct SA_GetGuildRankingSeasonList
{
	enum { NET_ID = 62322 };

	// TODO: reverse and send this packet
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

	// TODO: reverse and send this packet
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

struct SN_ProfileCharacterSkinList
{
	enum { NET_ID = 62390 };

	// TODO: reverse and send this packet
};

struct SN_WarehouseItems
{
	enum { NET_ID = 62404 };

	// TODO: reverse and send this packet
	u16 itemList_count;
};

struct SN_MailUnreadNotice
{
	enum { NET_ID = 62425 };

	// TODO: reverse and send this packet
};

struct SA_TierRecord
{
	enum { NET_ID = 62469 };

	// TODO: reverse and send this packet
};

struct SN_PveComradeInfo
{
	enum { NET_ID = 62485 };

	// TODO: reverse and send this packet
};

struct SN_ClientSettings
{
	enum { NET_ID = 62500 };

	u8 settingType;
	u16 data_len;
	u8 data[1];
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

struct Finish
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
};

} // Sv
