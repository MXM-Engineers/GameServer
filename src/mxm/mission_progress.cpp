#include "mission_progress.h"
#include "mission_rules.h"
#include <EASTL/sort.h>
#include <EASTL/algorithm.h>

typedef GameXmlContent::Mission Mission;

static i64 MissionPeriod(const MissionCalendar& cal, Mission::Category category)
{
	switch(category) {
		case Mission::Category::DAILY: return cal.dailyResetUtc;
		case Mission::Category::WEEKLY: return cal.weeklyResetUtc;
		case Mission::Category::GUILD: return cal.weeklyResetUtc;
		case Mission::Category::MONTHLY: return cal.monthlyResetUtc;
		default: return 0;
	}
}

static u32 DailySeed(const wchar* nickname, const MissionCalendar& cal)
{
	u32 seed = 2166136261u;
	for(const wchar* c = nickname; *c; c++) seed = (seed ^ (u32)*c) * 16777619u;
	return (seed ^ (u32)cal.periodDay) * 16777619u;
}

static void DailyPool(i32 step, u16 level, eastl::fixed_vector<const Mission*,64,false>* pool)
{
	const i32 grade = DailyMissionGrade(level);
	foreach_const(m, GetGameXmlContent().missions) {
		if(m->category != Mission::Category::DAILY || m->step != step || m->dayRestricted) continue;
		if(step > 0 && step <= MEASURED_DAILY_GRADED_STEP_MAX && m->grade != grade) continue;
		if(!pool->full()) pool->push_back(&*m);
	}
}

static const AccountDailyChange* FindDailyChange(const AccountMissions& saved, i32 step, i64 period)
{
	foreach_const(c, saved.dailyChanges) if(c->step == step && c->period == period) return &*c;
	return nullptr;
}

static void PushActive(const Mission* mission, i64 period, u16 changedCount, const AccountMissions& saved, ActiveMissionList* out)
{
	if(out->full()) return;
	ActiveMission active;
	active.mission = mission;
	active.period = period;
	active.changedCount = changedCount;
	foreach_const(s, saved.states) {
		if(s->ID != mission->ID || s->period != period) continue;
		active.progress = s->progress;
		active.rewarded = s->rewarded != 0;
	}
	out->push_back(active);
}

static void RefreshCleared(ActiveMissionList* listPtr)
{
	ActiveMissionList& list = *listPtr;
	foreach(a, list) {
		if(a->mission->subType != Mission::SubType::ACHIEVE_MISSION) continue;
		i64 cleared = 0;
		foreach_const(b, list) {
			if(b->mission->subType == Mission::SubType::ACHIEVE_MISSION || b->mission->category != a->mission->requiredCategory) continue;
			if(b->mission->count > 0 && b->progress >= b->mission->count) cleared++;
		}
		a->progress = cleared;
	}
	foreach(a, list) a->cleared = a->mission->count > 0 && a->progress >= a->mission->count;
}

MissionTimers MissionBuildActive(const wchar* nickname, u16 level, i64 nowUtc, const AccountMissions& saved, ActiveMissionList* out)
{
	ActiveMissionList& list = *out;
	const MissionCalendar cal = ComputeMissionCalendar(nowUtc);
	const GameXmlContent& content = GetGameXmlContent();
	const u32 seed = DailySeed(nickname, cal);
	for(i32 step = 0; step <= MISSION_DAILY_STEPS; step++) {
		const AccountDailyChange* change = FindDailyChange(saved, step, cal.dailyResetUtc);
		const Mission* changed = change ? content.FindMission(change->missionID) : nullptr;
		if(changed) {
			PushActive(changed, cal.dailyResetUtc, change->changedCount, saved, out);
			continue;
		}
		eastl::fixed_vector<const Mission*,64,false> pool;
		DailyPool(step, level, &pool);
		if(pool.empty()) continue;
		const u32 pick = (seed ^ (u32)step * 2654435761u) % (u32)pool.size();
		PushActive(pool[pick], cal.dailyResetUtc, 0, saved, out);
	}
	auto pushList = [&](const i32* ids, i32 count) {
		for(i32 i = 0; i < count; i++) {
			const Mission* mission = content.FindMission(ids[i]);
			if(mission) PushActive(mission, MissionPeriod(cal, mission->category), 0, saved, out);
		}
	};
	pushList(MEASURED_WEEKLY_MISSIONS_COMMON, (i32)ARRAY_COUNT(MEASURED_WEEKLY_MISSIONS_COMMON));
	pushList(MEASURED_WEEKLY_MISSIONS[cal.weeklySet], (i32)ARRAY_COUNT(MEASURED_WEEKLY_MISSIONS[cal.weeklySet]));
	pushList(MEASURED_MONTHLY_MISSIONS, (i32)ARRAY_COUNT(MEASURED_MONTHLY_MISSIONS));
	pushList(MEASURED_GUILD_MISSIONS[cal.weeklySet], (i32)ARRAY_COUNT(MEASURED_GUILD_MISSIONS[cal.weeklySet]));
	eastl::sort(list.begin(), list.end(), [](const ActiveMission& a, const ActiveMission& b) { return a.mission->ID < b.mission->ID; });
	RefreshCleared(out);

	MissionTimers timers;
	timers.dailyRemainSec = cal.dailyResetUtc - nowUtc;
	timers.weeklyRemainSec = cal.weeklyResetUtc - nowUtc;
	timers.monthlyRemainSec = cal.monthlyResetUtc - nowUtc;
	return timers;
}

static bool SectionMatches(const Mission& mission, const AccountGameRecord& record)
{
	const GameType gameType = (GameType)record.gameType;
	if(mission.gameType != GameType::INVALID && mission.gameType != gameType) return false;
	const bool pvp = gameType >= GameType::PVP_NORMAL && gameType < GameType::MAX;
	switch(mission.section) {
		case Mission::Section::ALL: return true;
		case Mission::Section::PVP_ALL: return pvp;
		case Mission::Section::PVP_TITAN: {
			const GameXmlContent::StageMaps* stage = GetGameXmlContent().FindStageMaps(StageIndex(record.stageIndex));
			return pvp && stage && stage->titan;
		}
		case Mission::Section::STAGES: {
			if(!GetGameXmlContent().SectionContains(mission.sectionIndex, record.stageIndex)) return false;
			if(mission.minDifficultyBit == 0) return true;
			const GameXmlContent::StageMaps* stage = GetGameXmlContent().FindStageMaps(StageIndex(record.stageIndex));
			return stage && stage->difficultyBit != GameXmlContent::ALL_DIFFICULTIES && stage->difficultyBit >= mission.minDifficultyBit;
		}
		default: return false;
	}
}

typedef eastl::fixed_vector<eastl::pair<i64,bool>,64,false> MissionSnapshot;

static void Snapshot(const ActiveMissionList& list, MissionSnapshot* out)
{
	foreach_const(a, list) out->push_back(eastl::make_pair(a->progress, a->cleared));
}

static void Diff(const ActiveMissionList& list, const MissionSnapshot& before, MissionChanges* changed)
{
	if(!changed) return;
	for(i32 i = 0; i < (i32)list.size() && i < (i32)before.size(); i++) {
		if(list[i].progress != before[i].first || list[i].cleared != before[i].second) changed->push_back(i);
	}
}

void MissionApplyCombat(i32 kills, i32 assists, const AccountGameRecord& section, ActiveMissionList* listPtr, MissionChanges* changed)
{
	ActiveMissionList& list = *listPtr;
	MissionSnapshot before;
	Snapshot(list, &before);
	foreach(a, list) {
		const Mission& m = *a->mission;
		if(!SectionMatches(m, section)) continue;
		if(m.subType == Mission::SubType::PVP_KILL) a->progress += kills;
		else if(m.subType == Mission::SubType::PVP_ASSIST) a->progress += assists;
	}
	RefreshCleared(listPtr);
	Diff(list, before, changed);
}

void MissionApplyGame(const AccountGameRecord& record, ActiveMissionList* listPtr, MissionChanges* changed)
{
	ActiveMissionList& list = *listPtr;
	MissionSnapshot before;
	Snapshot(list, &before);
	foreach(a, list) {
		const Mission& m = *a->mission;
		if(!SectionMatches(m, record)) continue;
		switch(m.subType) {
			case Mission::SubType::GET_GOLD: a->progress += record.gold; break;
			case Mission::SubType::GET_EXP: a->progress += record.exp; break;
			case Mission::SubType::MASTER_SORTIE: {
				bool played = false;
				foreach_const(c, m.requiredClasses) played = played || (i32)*c == record.classTypes[0] || (i32)*c == record.classTypes[1];
				if(played) a->progress++;
			} break;
			case Mission::SubType::PVP_PLAY: a->progress++; break;
			case Mission::SubType::PVP_WIN: a->progress += record.won; break;
			case Mission::SubType::PVP_KILL: a->progress += record.kills; break;
			case Mission::SubType::PVP_ASSIST: a->progress += record.assists; break;
			case Mission::SubType::PVP_KILL_STREAK: a->progress = MAX(a->progress, (i64)record.bestKillStreak); break;
			case Mission::SubType::PVP_FIRST_BLOOD: a->progress += record.firstBlood; break;
			case Mission::SubType::PVP_DAMAGE: {
				if(m.category == Mission::Category::MONTHLY) a->progress += record.damage;
				else a->progress = MAX(a->progress, record.damage);
			} break;
			case Mission::SubType::PVP_TITAN_SCORE: if(record.titanScore >= m.value) a->progress++; break;
			case Mission::SubType::PVP_TITAN_SCORE_TOTAL: a->progress = MAX(a->progress, (i64)record.titanScore); break;
			case Mission::SubType::PVE_MISSION_RANK: if(record.won && record.clearRank != 0 && (m.rank == 0 || record.clearRank <= m.rank)) a->progress++; break;
			case Mission::SubType::PVE_STAGE_CLEAR_NO_DIE: if(record.won && record.deaths == 0) a->progress++; break;
			case Mission::SubType::PVE_KILL_TOTAL: a->progress += record.stageKills; break;
			case Mission::SubType::PVE_RANDOM_STAGE_CLEAR: if(record.won && record.gameType == (i32)GameType::PVE_RANDOM) a->progress++; break;
			case Mission::SubType::PVE_STAGE_CLEAR_WITH_COMRADE: if(record.won && record.partySize >= 2) a->progress++; break;
			default: break;
		}
	}
	RefreshCleared(listPtr);
	Diff(list, before, changed);
}

void MissionStore(const ActiveMissionList& list, AccountMissions* saved)
{
	saved->states.clear();
	foreach_const(a, list) {
		if(a->progress == 0 && !a->rewarded) continue;
		if(saved->states.full()) break;
		AccountMissionState state;
		state.ID = a->mission->ID;
		state.period = a->period;
		state.progress = a->progress;
		state.rewarded = a->rewarded ? 1 : 0;
		saved->states.push_back(state);
	}
}

ActiveMission* MissionFind(ActiveMissionList* listPtr, i32 missionID)
{
	ActiveMissionList& list = *listPtr;
	foreach(a, list) if(a->mission->ID == missionID) return &*a;
	return nullptr;
}

bool MissionChangeDaily(const wchar* nickname, u16 level, i64 nowUtc, i32 step, AccountMissions* saved, ActiveMission* changed)
{
	if(step <= 0 || step > MISSION_DAILY_STEPS) return false;
	ActiveMissionList list;
	MissionBuildActive(nickname, level, nowUtc, *saved, &list);
	const ActiveMission* current = nullptr;
	foreach_const(a, list) if(a->mission->category == Mission::Category::DAILY && a->mission->step == step) current = &*a;
	if(!current || current->rewarded) return false;

	eastl::fixed_vector<const Mission*,64,false> pool;
	DailyPool(step, level, &pool);
	pool.erase(eastl::remove(pool.begin(), pool.end(), current->mission), pool.end());
	if(pool.empty()) return false;

	const MissionCalendar cal = ComputeMissionCalendar(nowUtc);
	const u16 changedCount = current->changedCount + 1;
	const u32 pick = (DailySeed(nickname, cal) ^ (u32)step * 2654435761u ^ (u32)changedCount * 40503u) % (u32)pool.size();
	saved->dailyChanges.erase(eastl::remove_if(saved->dailyChanges.begin(), saved->dailyChanges.end(), [&](const AccountDailyChange& c) { return c.step == step || c.period != cal.dailyResetUtc; }), saved->dailyChanges.end());
	AccountDailyChange change;
	change.step = step;
	change.period = cal.dailyResetUtc;
	change.missionID = pool[pick]->ID;
	change.changedCount = changedCount;
	saved->dailyChanges.push_back(change);

	changed->mission = pool[pick];
	changed->period = cal.dailyResetUtc;
	changed->progress = 0;
	changed->changedCount = changedCount;
	changed->rewarded = false;
	changed->cleared = false;
	return true;
}

bool ProgressGainExp(AccountProgress* progress, u32 exp)
{
	const auto& levels = GetGameXmlContent().expLevels;
	bool levelUp = false;
	progress->levelExp += exp;
	while(progress->level + 1 < (i32)levels.size() && levels[progress->level].required > 0 && progress->levelExp >= levels[progress->level].required) {
		progress->levelExp -= levels[progress->level].required;
		progress->level++;
		progress->gold += levels[progress->level].gold;
		levelUp = true;
	}
	return levelUp;
}

void ActivityRefresh(AccountProgress* progress, i64 nowUtc)
{
	const i64 period = ComputeMissionCalendar(nowUtc).dailyResetUtc;
	if(progress->activityPeriod == period) return;
	progress->activityPeriod = period;
	progress->activityPoint = 0;
	progress->activityRewarded = 0;
}
