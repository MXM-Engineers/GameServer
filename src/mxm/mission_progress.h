#pragma once
#include <common/base.h>
#include <mxm/game_content.h>
#include <mxm/account_progress.h>
#include <EASTL/fixed_vector.h>

struct ActiveMission
{
	const GameXmlContent::Mission* mission = nullptr;
	i64 period = 0;
	i64 progress = 0;
	u16 changedCount = 0;
	bool rewarded = false;
	bool cleared = false;
};

typedef eastl::fixed_vector<ActiveMission,64,false> ActiveMissionList;

struct MissionTimers
{
	i64 dailyRemainSec;
	i64 weeklyRemainSec;
	i64 monthlyRemainSec;
};

MissionTimers MissionBuildActive(const wchar* nickname, u16 level, i64 nowUtc, const AccountMissions& saved, ActiveMissionList* out);
typedef eastl::fixed_vector<i32,64,true> MissionChanges;
void MissionApplyGame(const AccountGameRecord& record, ActiveMissionList* list, MissionChanges* changed = nullptr);
void MissionApplyCombat(i32 kills, i32 assists, const AccountGameRecord& section, ActiveMissionList* list, MissionChanges* changed);
void MissionStore(const ActiveMissionList& list, AccountMissions* saved);
ActiveMission* MissionFind(ActiveMissionList* list, i32 missionID);
bool MissionChangeDaily(const wchar* nickname, u16 level, i64 nowUtc, i32 step, AccountMissions* saved, ActiveMission* changed);
void ActivityRefresh(AccountProgress* progress, i64 nowUtc);
bool ProgressGainExp(AccountProgress* progress, u32 exp);
