#pragma once
#include <common/base.h>
#include <EASTL/array.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/fixed_string.h>
#include <common/utils.h>

struct AccountProgress
{
	u16 level = 1;
	u32 levelExp = 0;
	i64 gold = 116472;
	u32 wins = 0;
	u32 draws = 0;
	u32 losses = 0;
	u32 honors = 0;
	i64 ccoin = 0;
	eastl::array<i32,3> inventoryLines = { 4, 4, 4 };
	i32 activityPoint = 0;
	u8 activityRewarded = 0;
	i64 activityPeriod = 0;
};

struct AccountItem
{
	u32 uid = 0;
	u8 invenType = 0;
	i32 slot = 0;
	i32 docIndex = 0;
	i32 count = 0;
};

struct AccountInventory
{
	u32 nextUid = 0x40000001;
	eastl::fixed_vector<AccountItem,256,true> items;
};

AccountInventory AccountInventoryLoad(const wchar* nickname);
static const i32 MEASURED_GEAR_SLOTS = 6;
static const i32 MEASURED_GEAR_DEFAULT_PAGES = 2;
static const i32 GEAR_MAX_PAGES = 32;
static const u8 MEASURED_GEAR_INVEN_TYPE = 4;
struct AccountGearPage
{
	eastl::fixed_string<wchar,32,false> name;
	eastl::array<u32,MEASURED_GEAR_SLOTS> items = {};
};
struct AccountGears
{
	eastl::fixed_vector<AccountGearPage,GEAR_MAX_PAGES,false> pages;
	eastl::fixed_vector<eastl::pair<i32,u8>,64,false> selected;
	u8 SelectedPage(i32 classType) const { foreach_const(s, selected) if(s->first == classType) return s->second; return 1; }
};
AccountGears AccountGearsLoad(const wchar* nickname);
struct AccountArsenal
{
	struct Weapon { i32 classType = 0; u8 type = 0; u8 grade = 0; u8 unlocked = 0; };
	eastl::fixed_vector<Weapon,192,false> weapons;
	eastl::fixed_vector<eastl::pair<i32,i32>,192,false> skills;
	Weapon* FindWeapon(i32 classType, u8 type) { foreach(w, weapons) if(w->classType == classType && w->type == type) return &*w; return nullptr; }
	const Weapon* FindWeapon(i32 classType, u8 type) const { foreach_const(w, weapons) if(w->classType == classType && w->type == type) return &*w; return nullptr; }
	eastl::fixed_vector<eastl::pair<i32,i32>,64,false> equipped;
	i32 supportKit = -1;
	struct SkillLoadout { i32 classType = 0; eastl::array<i32,2> skills = {}; };
	eastl::fixed_vector<eastl::pair<i32,i32>,192,false> skillLevels;
	i32 SkillLevel(i32 skill) const { foreach_const(s, skillLevels) if(s->first == skill) return s->second; return 0; }
	void SetSkillLevel(i32 skill, i32 level) { foreach(s, skillLevels) if(s->first == skill) { s->second = level; return; } if(!skillLevels.full()) skillLevels.push_back(eastl::make_pair(skill, level)); }
	eastl::fixed_vector<SkillLoadout,64,false> loadouts;
	const SkillLoadout* FindLoadout(i32 classType) const { foreach_const(l, loadouts) if(l->classType == classType) return &*l; return nullptr; }
	SkillLoadout* Loadout(i32 classType) { foreach(l, loadouts) if(l->classType == classType) return &*l; if(loadouts.full()) return nullptr; SkillLoadout n; n.classType = classType; loadouts.push_back(n); return &loadouts.back(); }
	u8 EquippedType(i32 classType) const { foreach_const(e, equipped) if(e->first == classType) return (u8)e->second; return 1; }
	void SetEquipped(i32 classType, u8 type) { foreach(e, equipped) if(e->first == classType) { e->second = type; return; } if(!equipped.full()) equipped.push_back(eastl::make_pair(classType, (i32)type)); }
	u8 Grade(i32 classType, u8 type) const { const Weapon* w = FindWeapon(classType, type); return w ? w->grade : 0; }
	bool SkillUnlocked(i32 classType, i32 skill) const { foreach_const(s, skills) if(s->first == classType && s->second == skill) return true; return false; }
};
AccountArsenal AccountArsenalLoad(const wchar* nickname);
void AccountArsenalSave(const wchar* nickname, const AccountArsenal& arsenal);
void AccountGearsSave(const wchar* nickname, const AccountGears& gears);
void AccountInventorySave(const wchar* nickname, const AccountInventory& inventory);
static const i32 MEASURED_INVENTORY_FIRST_SLOT = 200;
typedef eastl::fixed_vector<AccountItem,8,false> AccountItemList;
void AccountInventoryAdd(AccountInventory* inventory, i32 docIndex, i32 count, i32 maxStack, AccountItemList* touched, i32 firstSlot = MEASURED_INVENTORY_FIRST_SLOT);
i32 AccountInventoryFreeSlot(const AccountInventory& inventory, i32 firstSlot);

struct AccountGameRecord
{
	i64 gold = 0;
	u32 exp = 0;
	u8 won = 0;
	u8 draw = 0;
	i32 kills = 0;
	i32 assists = 0;
	i32 bestKillStreak = 0;
	u8 firstBlood = 0;
	i64 damage = 0;
	i32 titanScore = 0;
	i32 stageIndex = 0;
	i32 gameType = 0;
	eastl::array<i32,2> classTypes = { 0, 0 };
	u32 playSeconds = 0;
	i32 guildPoints = 0;
	i32 deaths = 0;
	u8 clearRank = 0;
	i32 stageKills = 0;
	i32 partySize = 0;
	i32 score = 0;
	i32 myTeamScore = 0;
	i32 enemyTeamScore = 0;
	u8 myTeamCount = 0;
	u8 enemyTeamCount = 0;
	u8 occupyCount = 0;
	u8 pvpSaveType = 0;
	i64 beginTime = 0;
};

struct AccountPvpStat { u8 gameType = 0; u8 saveType = 0; u32 win = 0; u32 lose = 0; u32 draw = 0; u32 leave = 0; u32 kill = 0; u32 death = 0; u32 assist = 0; u32 score = 0; u32 captures = 0; };
struct AccountPvpRecent { u8 gameType = 0; u8 saveType = 0; u8 result = 0; i32 classes[2] = { 0, 0 }; i32 myScore = 0; i32 myTeamScore = 0; i32 enemyTeamScore = 0; u8 myCount = 0; u8 enemyCount = 0; u8 kill = 0; u8 death = 0; u8 assist = 0; u8 occupy = 0; i64 beginTime = 0; };
struct AccountPvpHistory
{
	eastl::fixed_vector<AccountPvpStat,16,false> stats;
	eastl::fixed_vector<AccountPvpRecent,40,false> recent;
};
AccountPvpHistory AccountPvpHistoryLoad(const wchar* nickname);
void AccountPvpHistorySave(const wchar* nickname, const AccountPvpHistory& history);
void AccountPvpHistoryAdd(AccountPvpHistory* history, const struct AccountGameRecord& record);

struct AccountReward
{
	i64 gold = 0;
	u32 exp = 0;
	i32 games = 0;
	u32 playSeconds = 0;
	eastl::fixed_vector<AccountGameRecord,16,true> records;
};

struct AccountMissionState
{
	i32 ID = 0;
	i64 period = 0;
	i64 progress = 0;
	u8 rewarded = 0;
};

struct AccountDailyChange
{
	i32 step = 0;
	i64 period = 0;
	i32 missionID = 0;
	u16 changedCount = 0;
};

struct AccountMissions
{
	eastl::fixed_vector<AccountMissionState,64,true> states;
	eastl::fixed_vector<AccountDailyChange,8,true> dailyChanges;
};

struct AccountFriends
{
	eastl::fixed_vector<WideString,64,true> friends;
	eastl::fixed_vector<WideString,32,true> requests;
	eastl::fixed_vector<WideString,64,true> favorites;
	eastl::fixed_vector<WideString,64,true> blocked;
	WideString statusMessage;
};

bool AccountExists(const wchar* nickname);
AccountFriends AccountFriendsLoad(const wchar* nickname);
void AccountFriendsSave(const wchar* nickname, const AccountFriends& friends);
struct AccountStages
{
	eastl::fixed_vector<i32,512,true> cleared;
};

struct AccountSettingsBlob
{
	eastl::fixed_vector<u8,4096,false> data;
};

AccountSettingsBlob AccountSettingsLoad(const wchar* nickname, u8 settingType);
void AccountSettingsSave(const wchar* nickname, u8 settingType, const u8* data, i32 size);
AccountStages AccountStagesLoad(const wchar* nickname);
void AccountStagesSave(const wchar* nickname, const AccountStages& stages);
AccountProgress AccountProgressLoad(const wchar* nickname);
void AccountProgressSave(const wchar* nickname, const AccountProgress& progress);
void AccountGameRecordAppend(const wchar* nickname, const AccountGameRecord& record);
AccountReward AccountRewardTake(const wchar* nickname);
void AccountRewardItemAppend(const wchar* nickname, i32 docIndex, i32 count);
eastl::fixed_vector<eastl::pair<i32,i32>,64,true> AccountRewardItemsTake(const wchar* nickname);
void AccountStageCleared(const wchar* nickname, i32 stageIndex);
struct DefenceRecord { i32 step = 0; u32 timeMs = 0; };
static const i32 MEASURED_TIER_START_TYPE = 2;
static const i32 HYP_TIER_START_POINTS = 100;
struct AccountTier { i32 tierType = MEASURED_TIER_START_TYPE; i32 points = HYP_TIER_START_POINTS; i32 win = 0; i32 draw = 0; i32 lose = 0; i32 leave = 0; };
AccountTier AccountTierLoad(const wchar* nickname);
void AccountTierSave(const wchar* nickname, const AccountTier& tier);
DefenceRecord AccountDefenceRecordLoad(const wchar* nickname);
DefenceRecord DefenceWorldRecordLoad();
void AccountDefenceRecordSubmit(const wchar* nickname, const DefenceRecord& record);
AccountMissions AccountMissionsLoad(const wchar* nickname);
void AccountMissionsSave(const wchar* nickname, const AccountMissions& missions);
