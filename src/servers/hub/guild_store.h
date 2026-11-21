#pragma once
#include <common/base.h>
#include <common/utils.h>
#include <EASTL/vector.h>
#include <EASTL/array.h>
#include <EASTL/fixed_string.h>
#include <mutex>

typedef eastl::fixed_string<wchar,256,true> GuildText;

struct GuildClass
{
	i32 id = 0;
	u8 type = 0;
	u8 iconIndex = 0;
	WideString name;
	eastl::array<u8,10> rights = {};
};

struct GuildMemberRecord
{
	WideString nickname;
	i32 membershipID = 0;
	i32 contributedPoint = 0;
	i32 contributedFund = 0;
	i64 lastLogout = 0;
	i64 lastRollcallDay = -1;
	i64 joinedDay = -1;
};

struct GuildSkillRecord
{
	u8 type = 0;
	u8 level = 0;
	i64 expiry = 0;
	u16 extensionCount = 0;
};

struct GuildHistoryRecord
{
	u8 historyType = 0;
	u8 eventType = 0;
	i64 date = 0;
	WideString s1;
	WideString s2;
	i32 l1 = 0;
	i32 l2 = 0;
};

struct GuildInvitation
{
	WideString invitee;
	WideString inviter;
};

struct GuildRecord
{
	u32 id = 0;
	WideString name;
	WideString tag;
	i32 emblemIndex = 0;
	i32 point = 0;
	i32 fund = 0;
	i64 created = 0;
	i64 dissolution = 0;
	u8 joinType = 1;
	eastl::array<u8,7> interest = {};
	GuildText intro;
	GuildText notice;
	i64 rollcallDay = -1;
	i32 todayRollCall = 0;
	eastl::vector<GuildClass> classes;
	eastl::vector<GuildMemberRecord> members;
	eastl::vector<GuildHistoryRecord> history;
	eastl::vector<WideString> joinRequests;
	eastl::vector<GuildInvitation> invitations;
	eastl::vector<GuildSkillRecord> skills;
};

struct GuildStore
{
	std::recursive_mutex mutex;
	eastl::vector<GuildRecord> guilds;
	u32 nextID = 1;
	bool loaded = false;

	void Load();
	void Save();
	GuildRecord* FindByMember(const WideString& nickname);
	GuildRecord* FindByName(const WideString& name);
	GuildRecord* FindByID(u32 id);
	GuildMemberRecord* FindMember(GuildRecord& guild, const WideString& nickname);
	void AddHistory(GuildRecord& guild, u8 historyType, u8 eventType, i64 date, const WideString& s1, const WideString& s2, i32 l1, i32 l2);
};

GuildStore& GetGuildStore();
void GuildRewardBoost(const WideString& nickname, u16* goldRate, u16* expRate);
