#include "pve_ranking.h"
#include <EASTL/sort.h>
#include <stdio.h>
#include <direct.h>
#include <mutex>
#include "safe_file.h"

static const char* PVE_RANKING_DIR = "accounts";
static const i32 PVE_RANKING_KEEP = 200;
static std::mutex g_pveRankingMutex;

static void HexNick(const wchar* nick, char* out, i32 outSize)
{
	i32 o = 0;
	for(const wchar* c = nick; *c && o < outSize - 5; c++) o += snprintf(out + o, outSize - o, "%04x", (u32)(u16)*c);
	out[o] = 0;
}

static WideString UnhexNick(const char* hex)
{
	WideString nick;
	for(const char* c = hex; c[0] && c[1] && c[2] && c[3]; c += 4) {
		u32 v = 0;
		if(sscanf(c, "%4x", &v) != 1) break;
		nick.push_back((wchar)v);
	}
	return nick;
}

static void RankingPath(i32 stage, char* out, i32 outSize)
{
	snprintf(out, outSize, "%s/pve_ranking_%d.txt", PVE_RANKING_DIR, stage);
}

bool PveRankingEntry::HasMember(const wchar* nick) const
{
	foreach_const(m, members) if(m->nick.compare(nick) == 0) return true;
	return false;
}

static bool Better(const PveRankingEntry& a, const PveRankingEntry& b)
{
	if(a.totalScore != b.totalScore) return a.totalScore > b.totalScore;
	if(a.playMs != b.playMs) return a.playMs < b.playMs;
	return a.deaths < b.deaths;
}

static bool SameTeam(const PveRankingEntry& a, const PveRankingEntry& b)
{
	if(a.members.size() != b.members.size()) return false;
	foreach_const(m, a.members) if(!b.HasMember(m->nick.data())) return false;
	return true;
}

static eastl::vector<PveRankingEntry> LoadUnlocked(i32 stage)
{
	eastl::vector<PveRankingEntry> out;
	char path[256];
	RankingPath(stage, path, sizeof(path));
	FILE* f = fopen(path, "rb");
	if(!f) return out;
	char line[1024];
	while(fgets(line, sizeof(line), f)) {
		PveRankingEntry e;
		u32 rank = 0;
		i32 count = 0, used = 0;
		if(sscanf(line, "%u %u %u %u %d%n", &e.totalScore, &e.playMs, &e.deaths, &rank, &count, &used) != 5) continue;
		e.stage = stage;
		e.clearRank = (u8)rank;
		const char* c = line + used;
		for(i32 i = 0; i < count && !e.members.full(); i++) {
			PveRankingMember m;
			char hex[256] = {};
			i32 u = 0;
			if(sscanf(c, " %d %d %255s%n", &m.mainClass, &m.subClass, hex, &u) != 3) break;
			m.nick = UnhexNick(hex);
			e.members.push_back(m);
			c += u;
		}
		out.push_back(e);
	}
	fclose(f);
	return out;
}

void PveRankingSubmit(const PveRankingEntry& entry)
{
	std::lock_guard<std::mutex> lock(g_pveRankingMutex);
	eastl::vector<PveRankingEntry> ranking = LoadUnlocked(entry.stage);
	bool merged = false;
	foreach(e, ranking) {
		if(!SameTeam(*e, entry)) continue;
		if(Better(entry, *e)) *e = entry;
		merged = true;
	}
	if(!merged) ranking.push_back(entry);
	eastl::sort(ranking.begin(), ranking.end(), Better);
	if((i32)ranking.size() > PVE_RANKING_KEEP) ranking.resize(PVE_RANKING_KEEP);
	_mkdir(PVE_RANKING_DIR);
	char path[256];
	RankingPath(entry.stage, path, sizeof(path));
	char tmpPath[520];
	FILE* f = SafeFileOpen(path, tmpPath, sizeof(tmpPath));
	if(!f) return;
	foreach_const(e, ranking) {
		fprintf(f, "%u %u %u %u %d", e->totalScore, e->playMs, e->deaths, (u32)e->clearRank, (i32)e->members.size());
		foreach_const(m, e->members) {
			char hex[256];
			HexNick(m->nick.data(), hex, sizeof(hex));
			fprintf(f, " %d %d %s", m->mainClass, m->subClass, hex);
		}
		fprintf(f, "\n");
	}
	SafeFileCommit(f, tmpPath, path);
}

eastl::vector<PveRankingEntry> PveRankingLoad(i32 stage)
{
	std::lock_guard<std::mutex> lock(g_pveRankingMutex);
	return LoadUnlocked(stage);
}

i32 PveRankingFindBest(const eastl::vector<PveRankingEntry>& ranking, const wchar* nick)
{
	for(i32 i = 0; i < (i32)ranking.size(); i++) if(ranking[i].HasMember(nick)) return i;
	return -1;
}

void PveRankingSetLastGame(const wchar* nickname, i32 stage)
{
	std::lock_guard<std::mutex> lock(g_pveRankingMutex);
	_mkdir(PVE_RANKING_DIR);
	char hex[256];
	HexNick(nickname, hex, sizeof(hex));
	char path[512];
	snprintf(path, sizeof(path), "%s/%s.lastpve", PVE_RANKING_DIR, hex);
	char tmpPath[520];
	FILE* f = SafeFileOpen(path, tmpPath, sizeof(tmpPath));
	if(!f) return;
	fprintf(f, "%d\n", stage);
	SafeFileCommit(f, tmpPath, path);
}

i32 PveRankingTakeLastGame(const wchar* nickname)
{
	std::lock_guard<std::mutex> lock(g_pveRankingMutex);
	char hex[256];
	HexNick(nickname, hex, sizeof(hex));
	char path[512];
	snprintf(path, sizeof(path), "%s/%s.lastpve", PVE_RANKING_DIR, hex);
	FILE* f = fopen(path, "rb");
	if(!f) return 0;
	i32 stage = 0;
	if(fscanf(f, "%d", &stage) != 1) stage = 0;
	fclose(f);
	remove(path);
	return stage;
}
