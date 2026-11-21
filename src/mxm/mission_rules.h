#pragma once
#include <common/base.h>

static const i32 MEASURED_WEEKLY_MISSIONS_COMMON[] = { 302000000, 302000100, 302000120 };
static const i32 MEASURED_WEEKLY_MISSIONS[2][8] = {
	{ 302000051, 302000102, 302000104, 302000118, 302000203, 302000204, 302000205, 302000219 },
	{ 302000050, 302000101, 302000105, 302000119, 302000200, 302000208, 302000209, 302000218 },
};
static const i32 MEASURED_GUILD_MISSIONS[2][5] = {
	{ 304000102, 304000202, 304000302, 304000402, 304000502 },
	{ 304000101, 304000201, 304000303, 304000401, 304000503 },
};
static const i32 MEASURED_MONTHLY_MISSIONS[] = {
	303000000, 303000001, 303000002, 303000050, 303000100, 303000101, 303000105, 303000118,
	303000119, 303000120, 303000121, 303000122, 303000123, 303000124, 303000125, 303000200,
	303000201, 303000203, 303000204, 303000205, 303000208, 303000218, 303000219,
};
static const i32 MISSION_DAILY_STEPS = 4;
static const i32 MEASURED_DAILY_GRADED_STEP_MAX = 2;
static const i32 MEASURED_MISSION_REWARD_MONEY_REASON = 16;

struct DailyMissionGradeRule
{
	u16 maxLevel;
	i32 grade;
};

static const DailyMissionGradeRule MEASURED_DAILY_MISSION_GRADES[] = {
	{ 28, 1 },
	{ 98, 2 },
	{ 0xFFFF, 3 },
};

static inline i32 DailyMissionGrade(u16 level)
{
	for(const DailyMissionGradeRule& rule : MEASURED_DAILY_MISSION_GRADES) {
		if(level <= rule.maxLevel) return rule.grade;
	}
	return MEASURED_DAILY_MISSION_GRADES[ARRAY_COUNT(MEASURED_DAILY_MISSION_GRADES) - 1].grade;
}
static const i64 MISSION_RESET_HOUR_PACIFIC = 3;
static const i32 MISSION_WEEKLY_RESET_WEEKDAY = 3;

static inline i64 CivilToDays(i64 y, i64 m, i64 d)
{
	y -= m <= 2;
	const i64 era = (y >= 0 ? y : y - 399) / 400;
	const i64 yoe = y - era * 400;
	const i64 doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
	const i64 doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
	return era * 146097 + doe - 719468;
}

static inline void DaysToCivil(i64 z, i64* y, i64* m, i64* d)
{
	z += 719468;
	const i64 era = (z >= 0 ? z : z - 146096) / 146097;
	const i64 doe = z - era * 146097;
	const i64 yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
	const i64 doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
	const i64 mp = (5 * doy + 2) / 153;
	*d = doy - (153 * mp + 2) / 5 + 1;
	*m = mp < 10 ? mp + 3 : mp - 9;
	*y = yoe + era * 400 + (*m <= 2);
}

static inline i64 FloorDiv(i64 a, i64 b) { return a >= 0 ? a / b : -((-a + b - 1) / b); }
static inline i64 WeekdayOfDays(i64 days) { return ((days + 4) % 7 + 7) % 7; }

static inline i64 NthWeekdayOfMonth(i64 y, i64 m, i64 weekday, i64 n)
{
	const i64 first = CivilToDays(y, m, 1);
	return first + (weekday - WeekdayOfDays(first) + 7) % 7 + 7 * (n - 1);
}

static inline i64 PacificOffsetSec(i64 utc)
{
	i64 y, m, d;
	DaysToCivil(FloorDiv(utc - 8 * 3600, 86400), &y, &m, &d);
	const i64 dstStart = NthWeekdayOfMonth(y, 3, 0, 2) * 86400 + 10 * 3600;
	const i64 dstEnd = NthWeekdayOfMonth(y, 11, 0, 1) * 86400 + 9 * 3600;
	return utc >= dstStart && utc < dstEnd ? -7 * 3600 : -8 * 3600;
}

static inline i64 PacificLocalToUtc(i64 local)
{
	const i64 guess = local + 8 * 3600;
	return local - PacificOffsetSec(guess - 3600);
}

struct MissionCalendar
{
	i64 dailyResetUtc;
	i64 weeklyResetUtc;
	i64 monthlyResetUtc;
	i64 periodDay;
	i32 weeklySet;
};

static inline MissionCalendar ComputeMissionCalendar(i64 nowUtc)
{
	MissionCalendar cal;
	const i64 local = nowUtc + PacificOffsetSec(nowUtc);
	const i64 day = FloorDiv(local, 86400);
	const i64 resetSec = MISSION_RESET_HOUR_PACIFIC * 3600;
	const bool beforeReset = local - day * 86400 < resetSec;

	const i64 dailyDay = beforeReset ? day : day + 1;
	cal.dailyResetUtc = PacificLocalToUtc(dailyDay * 86400 + resetSec);
	cal.periodDay = dailyDay - 1;

	i64 weeklyDay = day + (MISSION_WEEKLY_RESET_WEEKDAY - WeekdayOfDays(day) + 7) % 7;
	if(weeklyDay == day && !beforeReset) weeklyDay += 7;
	cal.weeklyResetUtc = PacificLocalToUtc(weeklyDay * 86400 + resetSec);

	i64 y, m, d;
	DaysToCivil(day, &y, &m, &d);
	i64 monthlyDay = NthWeekdayOfMonth(y, m, MISSION_WEEKLY_RESET_WEEKDAY, 1);
	if(monthlyDay < day || (monthlyDay == day && !beforeReset)) {
		if(++m > 12) { m = 1; y++; }
		monthlyDay = NthWeekdayOfMonth(y, m, MISSION_WEEKLY_RESET_WEEKDAY, 1);
	}
	cal.monthlyResetUtc = PacificLocalToUtc(monthlyDay * 86400 + resetSec);

	const i64 isoWeekday = WeekdayOfDays(weeklyDay) == 0 ? 7 : WeekdayOfDays(weeklyDay);
	const i64 thursday = weeklyDay - (isoWeekday - 4);
	i64 ty, tm, td;
	DaysToCivil(thursday, &ty, &tm, &td);
	const i64 isoWeek = (thursday - CivilToDays(ty, 1, 1)) / 7 + 1;
	cal.weeklySet = (i32)(isoWeek & 1);
	return cal;
}
