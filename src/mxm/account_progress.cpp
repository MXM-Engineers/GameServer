#include "account_progress.h"
#include "achievements.h"
#include "summary.h"
#include <stdio.h>
#include <string.h>
#include <direct.h>
#include <mutex>
#include "safe_file.h"

static const char* ACCOUNT_DIR = "accounts";
static std::mutex g_accountFileMutex;

static void AccountPath(const wchar* nickname, const char* ext, char* out, i32 outSize)
{
	i32 o = snprintf(out, outSize, "%s/", ACCOUNT_DIR);
	for(const wchar* c = nickname; *c && o < outSize - 8; c++) o += snprintf(out + o, outSize - o, "%04x", (u32)(u16)*c);
	snprintf(out + o, outSize - o, ".%s", ext);
}

AccountSettingsBlob AccountSettingsLoad(const wchar* nickname, u8 settingType)
{
	std::lock_guard<std::mutex> lock(g_accountFileMutex);
	AccountSettingsBlob blob;
	char ext[16];
	snprintf(ext, sizeof(ext), "settings%u", (u32)settingType);
	char path[512];
	AccountPath(nickname, ext, path, sizeof(path));
	FILE* f = fopen(path, "rb");
	if(!f) return blob;
	u8 buff[4096];
	const size_t n = fread(buff, 1, sizeof(buff), f);
	fclose(f);
	blob.data.assign(buff, buff + n);
	return blob;
}

void AccountSettingsSave(const wchar* nickname, u8 settingType, const u8* data, i32 size)
{
	std::lock_guard<std::mutex> lock(g_accountFileMutex);
	_mkdir(ACCOUNT_DIR);
	char ext[16];
	snprintf(ext, sizeof(ext), "settings%u", (u32)settingType);
	char path[512];
	char tmp[520];
	AccountPath(nickname, ext, path, sizeof(path));
	snprintf(tmp, sizeof(tmp), "%s.tmp", path);
	FILE* f = SafeFileOpen(path, tmp, sizeof(tmp));
	ASSERT(f);
	fwrite(data, 1, size, f);
	SafeFileCommit(f, tmp, path);
}

static const i32 NEW_ACCOUNT_CLEARED_STAGES[] = { 200003200, 200007101, 200008000 };

AccountStages AccountStagesLoad(const wchar* nickname)
{
	std::lock_guard<std::mutex> lock(g_accountFileMutex);
	AccountStages stages;
	char path[512];
	AccountPath(nickname, "stages", path, sizeof(path));
	FILE* f = fopen(path, "rb");
	if(!f) {
		for(const i32 s : NEW_ACCOUNT_CLEARED_STAGES) stages.cleared.push_back(s);
		return stages;
	}
	i32 id = 0;
	while(fscanf(f, "%d", &id) == 1) {
		if(!stages.cleared.full()) stages.cleared.push_back(id);
	}
	fclose(f);
	return stages;
}

void AccountStagesSave(const wchar* nickname, const AccountStages& stages)
{
	std::lock_guard<std::mutex> lock(g_accountFileMutex);
	_mkdir(ACCOUNT_DIR);
	char path[512];
	char tmp[520];
	AccountPath(nickname, "stages", path, sizeof(path));
	snprintf(tmp, sizeof(tmp), "%s.tmp", path);
	FILE* f = SafeFileOpen(path, tmp, sizeof(tmp));
	ASSERT(f);
	foreach_const(s, stages.cleared) fprintf(f, "%d\n", *s);
	SafeFileCommit(f, tmp, path);
}

AccountProgress AccountProgressLoad(const wchar* nickname)
{
	std::lock_guard<std::mutex> lock(g_accountFileMutex);
	AccountProgress progress;
	char path[512];
	AccountPath(nickname, "txt", path, sizeof(path));
	FILE* f = fopen(path, "rb");
	if(!f) return progress;
	char line[256];
	while(fgets(line, sizeof(line), f)) {
		u32 v32 = 0;
		long long v64 = 0;
		if(sscanf(line, "Level=%u", &v32) == 1) progress.level = (u16)v32;
		else if(sscanf(line, "Exp=%u", &v32) == 1) progress.levelExp = v32;
		else if(sscanf(line, "Gold=%lld", &v64) == 1) progress.gold = (i64)v64;
		else if(sscanf(line, "Wins=%u", &v32) == 1) progress.wins = v32;
		else if(sscanf(line, "Draws=%u", &v32) == 1) progress.draws = v32;
		else if(sscanf(line, "Losses=%u", &v32) == 1) progress.losses = v32;
		else if(sscanf(line, "Honors=%u", &v32) == 1) progress.honors = v32;
		else if(sscanf(line, "CCoin=%lld", &v64) == 1) progress.ccoin = (i64)v64;
		else if(sscanf(line, "InventoryLines=%d %d %d", &progress.inventoryLines[0], &progress.inventoryLines[1], &progress.inventoryLines[2]) == 3) {}
		else if(sscanf(line, "Activity=%d %u %lld", &progress.activityPoint, &v32, &v64) == 3) { progress.activityRewarded = (u8)v32; progress.activityPeriod = (i64)v64; }
	}
	fclose(f);
	return progress;
}

void AccountProgressSave(const wchar* nickname, const AccountProgress& progress)
{
	std::lock_guard<std::mutex> lock(g_accountFileMutex);
	_mkdir(ACCOUNT_DIR);
	char path[512];
	char tmp[520];
	AccountPath(nickname, "txt", path, sizeof(path));
	snprintf(tmp, sizeof(tmp), "%s.tmp", path);
	FILE* f = SafeFileOpen(path, tmp, sizeof(tmp));
	ASSERT(f);
	fprintf(f, "Level=%u\nExp=%u\nGold=%lld\nWins=%u\nDraws=%u\nLosses=%u\nHonors=%u\nCCoin=%lld\nInventoryLines=%d %d %d\nActivity=%d %u %lld\n", (u32)progress.level, progress.levelExp, (long long)progress.gold, progress.wins, progress.draws, progress.losses, progress.honors, (long long)progress.ccoin,
		progress.inventoryLines[0], progress.inventoryLines[1], progress.inventoryLines[2], progress.activityPoint, (u32)progress.activityRewarded, (long long)progress.activityPeriod);
	SafeFileCommit(f, tmp, path);
}

void AccountGameRecordAppend(const wchar* nickname, const AccountGameRecord& record)
{
	std::lock_guard<std::mutex> lock(g_accountFileMutex);
	_mkdir(ACCOUNT_DIR);
	char path[512];
	AccountPath(nickname, "reward", path, sizeof(path));
	FILE* f = fopen(path, "ab");
	ASSERT(f);
	fprintf(f, "%lld %u %u %d %d %d %u %lld %d %d %d %d %d %u %u %d %d %d %d %d %u %u %u %u %lld\n", (long long)record.gold, record.exp, (u32)record.won, record.kills, record.assists, record.bestKillStreak,
		(u32)record.firstBlood, (long long)record.damage, record.titanScore, record.stageIndex, record.gameType, record.classTypes[0], record.classTypes[1], (u32)record.draw, record.playSeconds, record.guildPoints,
		record.deaths, record.score, record.myTeamScore, record.enemyTeamScore, (u32)record.myTeamCount, (u32)record.enemyTeamCount, (u32)record.occupyCount, (u32)record.pvpSaveType, (long long)record.beginTime);
	fclose(f);
}

void AccountRewardItemAppend(const wchar* nickname, i32 docIndex, i32 count)
{
	std::lock_guard<std::mutex> lock(g_accountFileMutex);
	_mkdir(ACCOUNT_DIR);
	char path[512];
	AccountPath(nickname, "rewarditems", path, sizeof(path));
	FILE* f = fopen(path, "ab");
	ASSERT(f);
	fprintf(f, "%d %d\n", docIndex, count);
	fclose(f);
}

eastl::fixed_vector<eastl::pair<i32,i32>,64,true> AccountRewardItemsTake(const wchar* nickname)
{
	std::lock_guard<std::mutex> lock(g_accountFileMutex);
	eastl::fixed_vector<eastl::pair<i32,i32>,64,true> items;
	char path[512];
	AccountPath(nickname, "rewarditems", path, sizeof(path));
	FILE* f = fopen(path, "rb");
	if(!f) return items;
	i32 doc = 0, count = 0;
	while(fscanf(f, "%d %d", &doc, &count) == 2) items.push_back(eastl::make_pair(doc, count));
	fclose(f);
	remove(path);
	return items;
}

void AccountStageCleared(const wchar* nickname, i32 stageIndex)
{
	AccountStages stages = AccountStagesLoad(nickname);
	foreach_const(s, stages.cleared) if(*s == stageIndex) return;
	if(stages.cleared.full()) return;
	stages.cleared.push_back(stageIndex);
	AccountStagesSave(nickname, stages);
}

static const char* DEFENCE_WORLD_RECORD_PATH = "accounts/defence_world.txt";
static const i32 MEASURED_PVP_RECENT_GAMES = 10;
static const u8 MEASURED_PVP_RESULT_WIN = 1;
static const u8 MEASURED_PVP_RESULT_LOSE = 2;
static const u8 MEASURED_PVP_RESULT_DRAW = 3;

AccountPvpHistory AccountPvpHistoryLoad(const wchar* nickname)
{
	std::lock_guard<std::mutex> lock(g_accountFileMutex);
	AccountPvpHistory history;
	char path[512];
	AccountPath(nickname, "pvp", path, sizeof(path));
	FILE* f = fopen(path, "rb");
	if(!f) return history;
	char line[512];
	while(fgets(line, sizeof(line), f)) {
		u32 a[11] = {};
		long long t = 0;
		u32 b[14] = {};
		if(sscanf(line, "S %u %u %u %u %u %u %u %u %u %u %u", &a[0], &a[1], &a[2], &a[3], &a[4], &a[5], &a[6], &a[7], &a[8], &a[9], &a[10]) >= 10 && !history.stats.full()) {
			AccountPvpStat s;
			s.gameType = (u8)a[0]; s.saveType = (u8)a[1]; s.win = a[2]; s.lose = a[3]; s.draw = a[4]; s.leave = a[5]; s.kill = a[6]; s.death = a[7]; s.assist = a[8]; s.score = a[9]; s.captures = a[10];
			history.stats.push_back(s);
		}
		else if(sscanf(line, "R %u %u %u %u %u %u %u %u %u %u %u %u %u %u %lld", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5], &b[6], &b[7], &b[8], &b[9], &b[10], &b[11], &b[12], &b[13], &t) == 15 && !history.recent.full()) {
			AccountPvpRecent r;
			r.gameType = (u8)b[0]; r.saveType = (u8)b[1]; r.result = (u8)b[2]; r.classes[0] = (i32)b[3]; r.classes[1] = (i32)b[4]; r.myScore = (i32)b[5]; r.myTeamScore = (i32)b[6]; r.enemyTeamScore = (i32)b[7];
			r.myCount = (u8)b[8]; r.enemyCount = (u8)b[9]; r.kill = (u8)b[10]; r.death = (u8)b[11]; r.assist = (u8)b[12]; r.occupy = (u8)b[13]; r.beginTime = (i64)t;
			history.recent.push_back(r);
		}
	}
	fclose(f);
	return history;
}

void AccountPvpHistorySave(const wchar* nickname, const AccountPvpHistory& history)
{
	std::lock_guard<std::mutex> lock(g_accountFileMutex);
	_mkdir(ACCOUNT_DIR);
	char path[512];
	AccountPath(nickname, "pvp", path, sizeof(path));
	char tmpPath[520];
	FILE* f = SafeFileOpen(path, tmpPath, sizeof(tmpPath));
	if(!f) return;
	foreach_const(s, history.stats) fprintf(f, "S %u %u %u %u %u %u %u %u %u %u %u\n", (u32)s->gameType, (u32)s->saveType, s->win, s->lose, s->draw, s->leave, s->kill, s->death, s->assist, s->score, s->captures);
	foreach_const(r, history.recent) fprintf(f, "R %u %u %u %d %d %d %d %d %u %u %u %u %u %u %lld\n", (u32)r->gameType, (u32)r->saveType, (u32)r->result, r->classes[0], r->classes[1], r->myScore, r->myTeamScore, r->enemyTeamScore,
		(u32)r->myCount, (u32)r->enemyCount, (u32)r->kill, (u32)r->death, (u32)r->assist, (u32)r->occupy, (long long)r->beginTime);
	SafeFileCommit(f, tmpPath, path);
}

void AccountPvpHistoryAdd(AccountPvpHistory* history, const AccountGameRecord& record)
{
	AccountPvpStat* stat = nullptr;
	foreach(s, history->stats) if(s->gameType == (u8)record.gameType && s->saveType == record.pvpSaveType) stat = &*s;
	if(!stat && !history->stats.full()) {
		AccountPvpStat s;
		s.gameType = (u8)record.gameType;
		s.saveType = record.pvpSaveType;
		history->stats.push_back(s);
		stat = &history->stats.back();
	}
	if(stat) {
		if(record.won) stat->win++;
		else if(record.draw) stat->draw++;
		else stat->lose++;
		stat->kill += (u32)MAX(record.kills, 0);
		stat->death += (u32)MAX(record.deaths, 0);
		stat->assist += (u32)MAX(record.assists, 0);
		stat->score += (u32)MAX(record.score, 0);
		stat->captures += record.occupyCount;
	}
	AccountPvpRecent r;
	r.gameType = (u8)record.gameType;
	r.saveType = record.pvpSaveType;
	r.result = record.won ? MEASURED_PVP_RESULT_WIN : record.draw ? MEASURED_PVP_RESULT_DRAW : MEASURED_PVP_RESULT_LOSE;
	r.classes[0] = record.classTypes[0];
	r.classes[1] = record.classTypes[1];
	r.myScore = record.score;
	r.myTeamScore = record.myTeamScore;
	r.enemyTeamScore = record.enemyTeamScore;
	r.myCount = record.myTeamCount;
	r.enemyCount = record.enemyTeamCount;
	r.kill = (u8)MIN(record.kills, 255);
	r.death = (u8)MIN(record.deaths, 255);
	r.assist = (u8)MIN(record.assists, 255);
	r.occupy = record.occupyCount;
	r.beginTime = record.beginTime;
	i32 same = 0;
	foreach_const(e, history->recent) if(e->gameType == r.gameType && e->saveType == r.saveType) same++;
	if(same >= MEASURED_PVP_RECENT_GAMES || history->recent.full()) {
		for(auto it = history->recent.begin(); it != history->recent.end(); ++it) {
			if(it->gameType == r.gameType && it->saveType == r.saveType) { history->recent.erase(it); break; }
		}
	}
	if(!history->recent.full()) history->recent.push_back(r);
}

AccountTier AccountTierLoad(const wchar* nickname)
{
	std::lock_guard<std::mutex> lock(g_accountFileMutex);
	AccountTier tier;
	char path[512];
	AccountPath(nickname, "tier", path, sizeof(path));
	FILE* f = fopen(path, "rb");
	if(!f) return tier;
	if(fscanf(f, "%d %d %d %d %d %d", &tier.tierType, &tier.points, &tier.win, &tier.draw, &tier.lose, &tier.leave) != 6) tier = AccountTier();
	fclose(f);
	return tier;
}

void AccountTierSave(const wchar* nickname, const AccountTier& tier)
{
	std::lock_guard<std::mutex> lock(g_accountFileMutex);
	_mkdir(ACCOUNT_DIR);
	char path[512];
	AccountPath(nickname, "tier", path, sizeof(path));
	char tmpPath[520];
	FILE* f = SafeFileOpen(path, tmpPath, sizeof(tmpPath));
	if(!f) return;
	fprintf(f, "%d %d %d %d %d %d\n", tier.tierType, tier.points, tier.win, tier.draw, tier.lose, tier.leave);
	SafeFileCommit(f, tmpPath, path);
}

static bool DefenceRecordBetter(const DefenceRecord& a, const DefenceRecord& b)
{
	return a.step > b.step || (a.step == b.step && a.step > 0 && a.timeMs < b.timeMs);
}

static DefenceRecord DefenceRecordRead(const char* path)
{
	DefenceRecord record;
	FILE* f = fopen(path, "rb");
	if(!f) return record;
	if(fscanf(f, "%d %u", &record.step, &record.timeMs) != 2) record = DefenceRecord();
	fclose(f);
	return record;
}

static void DefenceRecordWrite(const char* path, const DefenceRecord& record)
{
	_mkdir(ACCOUNT_DIR);
	char tmpPath[520];
	FILE* f = SafeFileOpen(path, tmpPath, sizeof(tmpPath));
	if(!f) return;
	fprintf(f, "%d %u\n", record.step, record.timeMs);
	SafeFileCommit(f, tmpPath, path);
}

DefenceRecord AccountDefenceRecordLoad(const wchar* nickname)
{
	std::lock_guard<std::mutex> lock(g_accountFileMutex);
	char path[512];
	AccountPath(nickname, "defence", path, sizeof(path));
	return DefenceRecordRead(path);
}

DefenceRecord DefenceWorldRecordLoad()
{
	std::lock_guard<std::mutex> lock(g_accountFileMutex);
	return DefenceRecordRead(DEFENCE_WORLD_RECORD_PATH);
}

void AccountDefenceRecordSubmit(const wchar* nickname, const DefenceRecord& record)
{
	std::lock_guard<std::mutex> lock(g_accountFileMutex);
	char path[512];
	AccountPath(nickname, "defence", path, sizeof(path));
	if(DefenceRecordBetter(record, DefenceRecordRead(path))) DefenceRecordWrite(path, record);
	if(DefenceRecordBetter(record, DefenceRecordRead(DEFENCE_WORLD_RECORD_PATH))) DefenceRecordWrite(DEFENCE_WORLD_RECORD_PATH, record);
}

static const u32 HYP_ACTIVITY_MAX_SECONDS_PER_GAME = 1800;

AccountReward AccountRewardTake(const wchar* nickname)
{
	std::lock_guard<std::mutex> lock(g_accountFileMutex);
	AccountReward reward;
	char path[512];
	AccountPath(nickname, "reward", path, sizeof(path));
	FILE* f = fopen(path, "rb");
	if(!f) return reward;
	char line[512];
	while(fgets(line, sizeof(line), f)) {
		AccountGameRecord record;
		long long gold = 0, damage = 0;
		u32 won = 0, firstBlood = 0, draw = 0;
		u32 myCount = 0, enemyCount = 0, occupy = 0, saveType = 0;
		long long beginTime = 0;
		const i32 n = sscanf(line, "%lld %u %u %d %d %d %u %lld %d %d %d %d %d %u %u %d %d %d %d %d %u %u %u %u %lld", &gold, &record.exp, &won, &record.kills, &record.assists, &record.bestKillStreak,
			&firstBlood, &damage, &record.titanScore, &record.stageIndex, &record.gameType, &record.classTypes[0], &record.classTypes[1], &draw, &record.playSeconds, &record.guildPoints,
			&record.deaths, &record.score, &record.myTeamScore, &record.enemyTeamScore, &myCount, &enemyCount, &occupy, &saveType, &beginTime);
		record.myTeamCount = (u8)myCount;
		record.enemyTeamCount = (u8)enemyCount;
		record.occupyCount = (u8)occupy;
		record.pvpSaveType = (u8)saveType;
		record.beginTime = (i64)beginTime;
		if(n < 2) continue;
		record.gold = (i64)gold;
		record.won = (u8)won;
		record.draw = (u8)draw;
		record.firstBlood = (u8)firstBlood;
		record.damage = (i64)damage;
		reward.gold += record.gold;
		reward.exp += record.exp;
		reward.games++;
		reward.playSeconds += MIN(record.playSeconds, HYP_ACTIVITY_MAX_SECONDS_PER_GAME);
		if(n >= 13 && !reward.records.full()) reward.records.push_back(record);
	}
	fclose(f);
	remove(path);
	return reward;
}

AccountMissions AccountMissionsLoad(const wchar* nickname)
{
	std::lock_guard<std::mutex> lock(g_accountFileMutex);
	AccountMissions missions;
	char path[512];
	AccountPath(nickname, "missions", path, sizeof(path));
	FILE* f = fopen(path, "rb");
	if(!f) return missions;
	char line[256];
	while(fgets(line, sizeof(line), f)) {
		long long period = 0, progress = 0;
		i32 id = 0, step = 0;
		u32 rewarded = 0, changed = 0;
		if(sscanf(line, "M %d %lld %lld %u", &id, &period, &progress, &rewarded) == 4) {
			if(missions.states.full()) continue;
			AccountMissionState state;
			state.ID = id;
			state.period = (i64)period;
			state.progress = (i64)progress;
			state.rewarded = (u8)rewarded;
			missions.states.push_back(state);
		}
		else if(sscanf(line, "C %d %lld %d %u", &step, &period, &id, &changed) == 4) {
			if(missions.dailyChanges.full()) continue;
			AccountDailyChange change;
			change.step = step;
			change.period = (i64)period;
			change.missionID = id;
			change.changedCount = (u16)changed;
			missions.dailyChanges.push_back(change);
		}
	}
	fclose(f);
	return missions;
}

void AccountMissionsSave(const wchar* nickname, const AccountMissions& missions)
{
	std::lock_guard<std::mutex> lock(g_accountFileMutex);
	_mkdir(ACCOUNT_DIR);
	char path[512];
	char tmp[520];
	AccountPath(nickname, "missions", path, sizeof(path));
	snprintf(tmp, sizeof(tmp), "%s.tmp", path);
	FILE* f = SafeFileOpen(path, tmp, sizeof(tmp));
	ASSERT(f);
	foreach_const(s, missions.states) fprintf(f, "M %d %lld %lld %u\n", s->ID, (long long)s->period, (long long)s->progress, (u32)s->rewarded);
	foreach_const(c, missions.dailyChanges) fprintf(f, "C %d %lld %d %u\n", c->step, (long long)c->period, c->missionID, (u32)c->changedCount);
	SafeFileCommit(f, tmp, path);
}

bool AccountExists(const wchar* nickname)
{
	std::lock_guard<std::mutex> lock(g_accountFileMutex);
	char path[512];
	AccountPath(nickname, "txt", path, sizeof(path));
	FILE* f = fopen(path, "rb");
	if(!f) return false;
	fclose(f);
	return true;
}

static bool ParseHexName(const char* hex, WideString* out)
{
	out->clear();
	for(const char* c = hex; *c && *c != '\n' && *c != '\r'; c += 4) {
		u32 v = 0;
		if(!c[1] || !c[2] || !c[3] || sscanf(c, "%4x", &v) != 1) return false;
		out->push_back((wchar)v);
	}
	return !out->empty();
}

static void WriteHexName(FILE* f, const WideString& name)
{
	foreach_const(c, name) fprintf(f, "%04x", (u32)(u16)*c);
}

AccountFriends AccountFriendsLoad(const wchar* nickname)
{
	std::lock_guard<std::mutex> lock(g_accountFileMutex);
	AccountFriends friends;
	char path[512];
	AccountPath(nickname, "friends", path, sizeof(path));
	FILE* f = fopen(path, "rb");
	if(!f) return friends;
	char line[512];
	while(fgets(line, sizeof(line), f)) {
		WideString name;
		if(line[0] == 0 || line[1] != ' ' || !ParseHexName(line + 2, &name)) continue;
		if(line[0] == 'F' && !friends.friends.full()) friends.friends.push_back(name);
		else if(line[0] == 'R' && !friends.requests.full()) friends.requests.push_back(name);
		else if(line[0] == 'V' && !friends.favorites.full()) friends.favorites.push_back(name);
		else if(line[0] == 'B' && !friends.blocked.full()) friends.blocked.push_back(name);
		else if(line[0] == 'S') friends.statusMessage = name;
	}
	fclose(f);
	return friends;
}

void AccountFriendsSave(const wchar* nickname, const AccountFriends& friends)
{
	std::lock_guard<std::mutex> lock(g_accountFileMutex);
	_mkdir(ACCOUNT_DIR);
	char path[512];
	char tmp[520];
	AccountPath(nickname, "friends", path, sizeof(path));
	snprintf(tmp, sizeof(tmp), "%s.tmp", path);
	FILE* f = SafeFileOpen(path, tmp, sizeof(tmp));
	ASSERT(f);
	foreach_const(n, friends.friends) {
		fputs("F ", f);
		WriteHexName(f, *n);
		fputc('\n', f);
	}
	foreach_const(n, friends.requests) {
		fputs("R ", f);
		WriteHexName(f, *n);
		fputc('\n', f);
	}
	foreach_const(n, friends.favorites) {
		fputs("V ", f);
		WriteHexName(f, *n);
		fputc('\n', f);
	}
	foreach_const(n, friends.blocked) {
		fputs("B ", f);
		WriteHexName(f, *n);
		fputc('\n', f);
	}
	if(!friends.statusMessage.empty()) {
		fputs("S ", f);
		WriteHexName(f, friends.statusMessage);
		fputc('\n', f);
	}
	SafeFileCommit(f, tmp, path);
}

AccountArsenal AccountArsenalLoad(const wchar* nickname)
{
	std::lock_guard<std::mutex> lock(g_accountFileMutex);
	AccountArsenal arsenal;
	char path[512];
	AccountPath(nickname, "arsenal", path, sizeof(path));
	FILE* f = fopen(path, "rb");
	if(!f) return arsenal;
	char line[128];
	while(fgets(line, sizeof(line), f)) {
		i32 cls = 0, a = 0, b = 0, c = 0;
		if(sscanf(line, "W %d %d %d %d", &cls, &a, &b, &c) == 4 && !arsenal.weapons.full()) {
			AccountArsenal::Weapon w;
			w.classType = cls; w.type = (u8)a; w.grade = (u8)b; w.unlocked = (u8)c;
			arsenal.weapons.push_back(w);
		}
		else if(sscanf(line, "S %d %d", &cls, &a) == 2 && !arsenal.skills.full()) arsenal.skills.push_back(eastl::make_pair(cls, a));
		else if(sscanf(line, "E %d %d", &cls, &a) == 2) arsenal.SetEquipped(cls, (u8)a);
		else if(sscanf(line, "P %d", &a) == 1) arsenal.supportKit = a;
		else if(sscanf(line, "U %d %d", &cls, &a) == 2) arsenal.SetSkillLevel(cls, a);
		else if(sscanf(line, "K %d %d %d", &cls, &a, &b) == 3) { if(AccountArsenal::SkillLoadout* l = arsenal.Loadout(cls)) { l->skills[0] = a; l->skills[1] = b; } }
	}
	fclose(f);
	return arsenal;
}

void AccountArsenalSave(const wchar* nickname, const AccountArsenal& arsenal)
{
	std::lock_guard<std::mutex> lock(g_accountFileMutex);
	_mkdir(ACCOUNT_DIR);
	char path[512];
	AccountPath(nickname, "arsenal", path, sizeof(path));
	char tmpPath[520];
	FILE* f = SafeFileOpen(path, tmpPath, sizeof(tmpPath));
	if(!f) return;
	foreach_const(w, arsenal.weapons) fprintf(f, "W %d %u %u %u\n", w->classType, (u32)w->type, (u32)w->grade, (u32)w->unlocked);
	foreach_const(s, arsenal.skills) fprintf(f, "S %d %d\n", s->first, s->second);
	foreach_const(e, arsenal.equipped) fprintf(f, "E %d %d\n", e->first, e->second);
	fprintf(f, "P %d\n", arsenal.supportKit);
	foreach_const(u, arsenal.skillLevels) fprintf(f, "U %d %d\n", u->first, u->second);
	foreach_const(l, arsenal.loadouts) fprintf(f, "K %d %d %d\n", l->classType, l->skills[0], l->skills[1]);
	SafeFileCommit(f, tmpPath, path);
}

AccountGears AccountGearsLoad(const wchar* nickname)
{
	std::lock_guard<std::mutex> lock(g_accountFileMutex);
	AccountGears gears;
	char path[512];
	AccountPath(nickname, "gears", path, sizeof(path));
	FILE* f = fopen(path, "rb");
	if(f) {
		char line[512];
		while(fgets(line, sizeof(line), f)) {
			AccountGearPage page;
			char name[160] = {};
			i32 classType = 0, selected = 0;
			if(sscanf(line, "P %159s %u %u %u %u %u %u", name, &page.items[0], &page.items[1], &page.items[2], &page.items[3], &page.items[4], &page.items[5]) == 7 && !gears.pages.full()) {
				for(const char* c = name; c[0] && c[1] && c[2] && c[3] && !page.name.full(); c += 4) {
					u32 v = 0;
					if(sscanf(c, "%4x", &v) != 1) break;
					page.name.push_back((wchar)v);
				}
				gears.pages.push_back(page);
			}
			else if(sscanf(line, "S %d %d", &classType, &selected) == 2 && !gears.selected.full()) gears.selected.push_back(eastl::make_pair(classType, (u8)selected));
		}
		fclose(f);
	}
	while((i32)gears.pages.size() < MEASURED_GEAR_DEFAULT_PAGES) gears.pages.push_back(AccountGearPage());
	return gears;
}

void AccountGearsSave(const wchar* nickname, const AccountGears& gears)
{
	std::lock_guard<std::mutex> lock(g_accountFileMutex);
	_mkdir(ACCOUNT_DIR);
	char path[512];
	AccountPath(nickname, "gears", path, sizeof(path));
	char tmpPath[520];
	FILE* f = SafeFileOpen(path, tmpPath, sizeof(tmpPath));
	ASSERT(f);
	foreach_const(p, gears.pages) {
		fprintf(f, "P ");
		if(p->name.empty()) fprintf(f, "-");
		foreach_const(c, p->name) fprintf(f, "%04x", (u32)*c);
		for(u32 uid : p->items) fprintf(f, " %u", uid);
		fprintf(f, "\n");
	}
	foreach_const(s, gears.selected) fprintf(f, "S %d %d\n", s->first, (i32)s->second);
	SafeFileCommit(f, tmpPath, path);
}

AccountInventory AccountInventoryLoad(const wchar* nickname)
{
	std::lock_guard<std::mutex> lock(g_accountFileMutex);
	AccountInventory inventory;
	char path[512];
	AccountPath(nickname, "items", path, sizeof(path));
	FILE* f = fopen(path, "rb");
	if(!f) {
		AccountItem jukeboxCoins;
		jukeboxCoins.uid = inventory.nextUid++;
		jukeboxCoins.slot = 200;
		jukeboxCoins.docIndex = 137120001;
		jukeboxCoins.count = 99;
		inventory.items.push_back(jukeboxCoins);
		return inventory;
	}
	char line[256];
	while(fgets(line, sizeof(line), f)) {
		u32 uid = 0, inven = 0;
		i32 slot = 0, doc = 0, count = 0;
		if(sscanf(line, "N %u", &uid) == 1) inventory.nextUid = uid;
		else if(sscanf(line, "I %u %u %d %d %d", &uid, &inven, &slot, &doc, &count) == 5 && !inventory.items.full()) {
			AccountItem item;
			item.uid = uid;
			item.invenType = (u8)inven;
			item.slot = slot;
			item.docIndex = doc;
			item.count = count;
			inventory.items.push_back(item);
		}
	}
	fclose(f);
	return inventory;
}

i32 AccountInventoryFreeSlot(const AccountInventory& inventory, i32 firstSlot)
{
	i32 slot = firstSlot;
	for(bool taken = true; taken; ) {
		taken = false;
		foreach_const(it, inventory.items) if(it->invenType == 0 && it->slot == slot) { taken = true; slot++; break; }
	}
	return slot;
}

void AccountInventoryAdd(AccountInventory* inventory, i32 docIndex, i32 count, i32 maxStack, AccountItemList* touched, i32 firstSlot)
{
	i32 left = count;
	foreach(it, inventory->items) {
		if(left <= 0) break;
		if(it->docIndex != docIndex || it->count >= maxStack) continue;
		const i32 add = MIN(left, maxStack - it->count);
		it->count += add;
		left -= add;
		if(touched && !touched->full()) touched->push_back(*it);
	}
	while(left > 0 && !inventory->items.full()) {
		AccountItem item;
		item.uid = inventory->nextUid++;
		item.slot = AccountInventoryFreeSlot(*inventory, firstSlot);
		item.docIndex = docIndex;
		item.count = MIN(left, maxStack);
		left -= item.count;
		inventory->items.push_back(item);
		if(touched && !touched->full()) touched->push_back(item);
	}
}

void AccountInventorySave(const wchar* nickname, const AccountInventory& inventory)
{
	std::lock_guard<std::mutex> lock(g_accountFileMutex);
	_mkdir(ACCOUNT_DIR);
	char path[512];
	char tmp[520];
	AccountPath(nickname, "items", path, sizeof(path));
	snprintf(tmp, sizeof(tmp), "%s.tmp", path);
	FILE* f = SafeFileOpen(path, tmp, sizeof(tmp));
	ASSERT(f);
	fprintf(f, "N %u\n", inventory.nextUid);
	foreach_const(it, inventory.items) fprintf(f, "I %u %u %d %d %d\n", it->uid, (u32)it->invenType, it->slot, it->docIndex, it->count);
	SafeFileCommit(f, tmp, path);
}

AccountAchievements AccountAchievementsLoad(const wchar* nickname)
{
	std::lock_guard<std::mutex> lock(g_accountFileMutex);
	AccountAchievements acc;
	char path[512];
	AccountPath(nickname, "achieve", path, sizeof(path));
	FILE* f = fopen(path, "rb");
	if(!f) return acc;
	char line[512];
	while(fgets(line, sizeof(line), f)) {
		if(line[0] == 'A') {
			AccountAchieveState st;
			long long progress = 0, date = 0;
			u32 cleared = 0;
			i32 n = 0, used = 0;
			if(sscanf(line, "A %d %lld %u %lld %d%n", &st.index, &progress, &cleared, &date, &n, &used) < 5) continue;
			st.progress = progress;
			st.cleared = (u8)cleared;
			st.date = date;
			const char* c = line + used;
			for(i32 i = 0; i < n && !st.achieved.full(); i++) {
				i32 v = 0, u = 0;
				if(sscanf(c, " %d%n", &v, &u) != 1) break;
				st.achieved.push_back(v);
				c += u;
			}
			acc.states.push_back(st);
		}
		else if(line[0] == 'T') {
			i32 t = 0;
			if(sscanf(line, "T %d", &t) == 1) acc.titles.push_back(t);
		}
		else if(line[0] == 'D') sscanf(line, "D %d %d", &acc.displayTitle, &acc.statTitle);
		else if(line[0] == 'B') {
			u32 b = 1;
			if(sscanf(line, "B %u", &b) == 1) acc.badge = (u8)b;
		}
	}
	fclose(f);
	return acc;
}

void AccountAchievementsSave(const wchar* nickname, const AccountAchievements& acc)
{
	std::lock_guard<std::mutex> lock(g_accountFileMutex);
	_mkdir(ACCOUNT_DIR);
	char path[512];
	AccountPath(nickname, "achieve", path, sizeof(path));
	char tmpPath[520];
	FILE* f = SafeFileOpen(path, tmpPath, sizeof(tmpPath));
	if(!f) return;
	fprintf(f, "D %d %d\n", acc.displayTitle, acc.statTitle);
	fprintf(f, "B %u\n", (u32)acc.badge);
	foreach_const(t, acc.titles) fprintf(f, "T %d\n", *t);
	foreach_const(s, acc.states) {
		fprintf(f, "A %d %lld %u %lld %d", s->index, (long long)s->progress, (u32)s->cleared, (long long)s->date, (i32)s->achieved.size());
		foreach_const(a, s->achieved) fprintf(f, " %d", *a);
		fprintf(f, "\n");
	}
	SafeFileCommit(f, tmpPath, path);
}

AccountSummaries AccountSummariesLoad(const wchar* nickname)
{
	std::lock_guard<std::mutex> lock(g_accountFileMutex);
	AccountSummaries s;
	char path[512];
	AccountPath(nickname, "summary", path, sizeof(path));
	FILE* f = fopen(path, "rb");
	if(!f) return s;
	char line[128];
	while(fgets(line, sizeof(line), f)) {
		AccountSummaryEntry e;
		u32 type = 0, received = 0;
		long long date = 0;
		if(sscanf(line, "%d %d %u %d %u %lld", &e.summary, &e.stage, &type, &e.data, &received, &date) != 6) continue;
		e.type = (u8)type;
		e.received = (u8)received;
		e.date = date;
		s.entries.push_back(e);
	}
	fclose(f);
	return s;
}

void AccountSummariesSave(const wchar* nickname, const AccountSummaries& s)
{
	std::lock_guard<std::mutex> lock(g_accountFileMutex);
	_mkdir(ACCOUNT_DIR);
	char path[512];
	AccountPath(nickname, "summary", path, sizeof(path));
	char tmpPath[520];
	FILE* f = SafeFileOpen(path, tmpPath, sizeof(tmpPath));
	if(!f) return;
	foreach_const(e, s.entries) fprintf(f, "%d %d %u %d %u %lld\n", e->summary, e->stage, (u32)e->type, e->data, (u32)e->received, (long long)e->date);
	SafeFileCommit(f, tmpPath, path);
}

