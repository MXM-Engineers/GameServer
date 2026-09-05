#pragma once
#include <common/base.h>
#include <common/protocol.h>
#include <mxm/game_content.h>

struct Guild
{
	typedef Sv::SA_GetGuildProfile::ST_GuildMemberRights Rights;

	const wchar_t* name = L"Alpha testers";
	const wchar_t* tag = L"Alpha";
	i32 emblemIndex = 0;
	u8 lvl = 1;
	u8 memberMax = 10;
	const wchar_t* ownerNickname = L"Malachi";
	i64 createdDate = 131474874000000000;
	i64 dissolutionDate = 0;
	u8 joinType = 1;
	const wchar_t* intro = L"Welcome to Alpha";
	const wchar_t* notice = L"";
	i32 point = 0;
	i32 fund = 0;
	i32 rankNo = -1;
	i32 maxDailyStage = 100;
	i32 maxDailyArena = 100;

	struct Rank
	{
		i32 id;
		GuildRankType type;
		u8 iconIndex;
		const wchar_t* name;
		Rights rights;
	};

	struct Member
	{
		const wchar_t* nick;
		i32 membershipID;
		u16 lvl;
		u16 leaderClassType;
		u16 masterCount;
		i32 achievementScore;
	};

	struct OwnedSkill
	{
		i32 type;
		u8 level;
	};

	Member owner = { L"Malachi", 0, 99, 10, 27, 12455 };
	i32 newMemberRankID = 3;
	eastl::fixed_vector<OwnedSkill,10,false> skills;
};

inline Guild::Rights GuildOfficerRights()
{
	Guild::Rights r;
	r.hasInviteRight = 1;
	r.hasExpelRight = 1;
	r.hasMembershipChgRight = 1;
	r.hasClassAssignRight = 1;
	r.hasNoticeChgRight = 0;
	r.hasIntroChgRight = 0;
	r.hasInterestChgRight = 0;
	r.hasFundManageRight = 1;
	r.hasJoinTypeRight = 0;
	r.hasEmblemRight = 0;
	return r;
}

inline Guild::Rights GuildInviteOnlyRights()
{
	Guild::Rights r;
	r.hasInviteRight = 1;
	r.hasExpelRight = 0;
	r.hasMembershipChgRight = 0;
	r.hasClassAssignRight = 0;
	r.hasNoticeChgRight = 0;
	r.hasIntroChgRight = 0;
	r.hasInterestChgRight = 0;
	r.hasFundManageRight = 0;
	r.hasJoinTypeRight = 0;
	r.hasEmblemRight = 0;
	return r;
}

inline Guild::Rights GuildFullRights()
{
	Guild::Rights r;
	r.hasInviteRight = 1;
	r.hasExpelRight = 1;
	r.hasMembershipChgRight = 1;
	r.hasClassAssignRight = 1;
	r.hasNoticeChgRight = 1;
	r.hasIntroChgRight = 1;
	r.hasInterestChgRight = 1;
	r.hasFundManageRight = 1;
	r.hasJoinTypeRight = 1;
	r.hasEmblemRight = 1;
	return r;
}

inline eastl::fixed_vector<Guild::Rank,8,false>& GetAlphaGuildRanks()
{
	static eastl::fixed_vector<Guild::Rank,8,false> ranks;
	if(ranks.empty()) {
		Guild::Rank r;
		r.id = 1; r.type = GuildRankType::Custom; r.iconIndex = 2; r.name = L"Veteran"; r.rights = GuildOfficerRights();
		ranks.push_back(r);
		r.id = 2; r.type = GuildRankType::Custom; r.iconIndex = 6; r.name = L"Member"; r.rights = GuildInviteOnlyRights();
		ranks.push_back(r);
		r.id = 3; r.type = GuildRankType::Default; r.iconIndex = 4; r.name = L"New Member"; r.rights = GuildInviteOnlyRights();
		ranks.push_back(r);
		r.id = 4; r.type = GuildRankType::Custom; r.iconIndex = 2; r.name = L"Officer"; r.rights = GuildFullRights();
		ranks.push_back(r);
	}
	return ranks;
}

inline const Guild& GetAlphaGuild()
{
	static Guild guild;
	static bool resolved = false;
	if(!resolved) {
		const GameXmlContent& content = GetGameXmlContent();
		guild.lvl = (u8)content.GuildLevelForPoints(guild.point);
		guild.memberMax = (u8)(10 + content.GuildSkillValue("GUILD_MEMBER_MAX_UP", 0));
		guild.maxDailyStage = content.guildActivityCapWeekday;
		guild.maxDailyArena = content.guildActivityCapWeekday;
		if(!content.masters.empty()) {
			guild.owner.leaderClassType = (u16)content.masters[0].classType;
			guild.owner.masterCount = (u16)content.masters.size();
		}
		guild.owner.achievementScore = 0;
		guild.createdDate = (i64)CurrentFiletimeTimestampUTC();
		ASSERT(!content.validGuildEmblems.empty());
		guild.emblemIndex = content.validGuildEmblems[0];
		resolved = true;
	}
	return guild;
}
