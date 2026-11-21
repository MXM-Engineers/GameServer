#include "achievements.h"
#include <EAStdC/EAString.h>
#include <time.h>
#include "account_progress.h"

using namespace tinyxml2;
typedef GameXmlContent::AchieveSub AchieveSub;

static const char* ACHIEVE_SUB_NAMES[] = {
	"", "PVE_KILL_MONSTER_TOTAL", "PVE_SUFFER_FROM_TARGET", "PVE_STAGE_CLEAR", "PVE_STAGE_CLEAR_WITH_FRIENDS", "PVE_DEATH_TOTAL", "PVE_KILL_IN_CERTAIN_TIME",
	"PVE_SURVIVAL", "PVE_INTERACTION", "PVE_BRING_COMRADE_COUNT", "PVP_PLAY", "PVP_WIN_TOTAL", "PVP_LOSE_TOTAL", "PVP_DRAW_TOTAL", "PVP_DEATH_TOTAL",
	"PVP_KILL_STREAK", "PVP_KILL_STREAK_TOTAL", "PVP_KDA", "PVP_OCCUPY_ALL_CORES", "PVP_SPORTS_AWARDED", "COMMON_USE_SKILL", "COMMON_SKILL_UPGRADE_COUNT",
	"COMMON_MASTER_SORTIE", "COMMON_MASTER_UNLOCK", "COMMON_MASTER_TOTAL", "COMMON_WEAPON_UPGRADE_COUNT", "COMMON_ITEM", "COMMON_MULTI", "COMMON_LEVEL",
	"COMMON_FRIEND", "COMMON_ACHIEVEMENT_POINT", "COMMON_ATTEND_ROLL_CALL", "COMMON_FREE_CCOIN_ASSETS", "COMMON_RON_ASSETS", "COMMON_SALES",
	"COMMON_TRADE_COUNT", "COMMON_TITLE_COUNT", "COMMON_MISSION_ACHIEVE_COUNT", "COMMON_JOIN_GUILD", "COMMON_CREATE_GUILD",
};
static_assert(ARRAY_COUNT(ACHIEVE_SUB_NAMES) == (size_t)AchieveSub::COUNT, "achievement subtype names");

static const char* ACHIEVE_GROUP_NAMES[] = { "", "ACHIEVEMENT_TYPE_COMMON", "ACHIEVEMENT_TYPE_MASTER", "ACHIEVEMENT_TYPE_PVE", "ACHIEVEMENT_TYPE_PVP" };
static const char* ACHIEVE_GAME_TYPE_NAMES[] = {
	"GAME_TYPE_INVALID", "GAME_TYPE_PVE_NORMAL", "GAME_TYPE_PVE_RANDOM", "GAME_TYPE_PVE_TICKET", "GAME_TYPE_PVP_NORMAL",
	"GAME_TYPE_PVP_RANK", "GAME_TYPE_PVP_AI", "GAME_TYPE_PVP_NORMAL_FFA", "GAME_TYPE_PVP_GUILD", "GAME_TYPE_PVP_FAKE_AI",
};

GameXmlContent::AchieveSub AchieveSubFromName(const char* name)
{
	static const char PREFIX[] = "ACHIEVEMENT_SUBTYPE_";
	if(!name || EA::StdC::Strncmp(name, PREFIX, sizeof(PREFIX) - 1) != 0) return AchieveSub::NONE;
	name += sizeof(PREFIX) - 1;
	for(u32 i = 1; i < (u32)AchieveSub::COUNT; i++) if(EA::StdC::Strcmp(name, ACHIEVE_SUB_NAMES[i]) == 0) return (AchieveSub)i;
	return AchieveSub::NONE;
}

bool GameXmlContent::SectionContains(i32 section, i32 stage) const
{
	if(section < 0 || section >= (i32)sections.size()) return true;
	foreach_const(s, sections[section]) if(*s == stage) return true;
	return false;
}

u8 GameXmlContent::ClearRankFromName(const char* name)
{
	static const char* CLEAR_RANK_NAMES[] = { "", "E_CLEAR_RANK_TYPE_SSS", "E_CLEAR_RANK_TYPE_SS", "E_CLEAR_RANK_TYPE_S", "E_CLEAR_RANK_TYPE_A", "E_CLEAR_RANK_TYPE_B", "E_CLEAR_RANK_TYPE_C" };
	if(!name) return 0;
	for(u8 i = 1; i < ARRAY_COUNT(CLEAR_RANK_NAMES); i++) if(EA::StdC::Strcmp(name, CLEAR_RANK_NAMES[i]) == 0) return i;
	return 0;
}

bool GameXmlContent::LoadSections()
{
	constexpr eastl::hash<const char*> hashName;
	if(!sections.empty()) return true;
	XMLDocument docSection;
	if(LoadXMLFile(L"/Design/GAMEINFO/SECTION.xml", docSection, true)) {
		XMLElement* root = docSection.FirstChildElement() ? docSection.FirstChildElement()->FirstChildElement("ST_SECTION_INFO") : nullptr;
		for(XMLElement* s = root ? root->FirstChildElement("SECTION_INFO") : nullptr; s; s = s->NextSiblingElement("SECTION_INFO")) {
			const char* key = s->Attribute("DATAKEY");
			if(!key) continue;
			eastl::vector<i32> areas;
			for(XMLElement* a = s->FirstChildElement("_AREA"); a; a = a->NextSiblingElement("_AREA")) areas.push_back(a->IntAttribute("DATA"));
			sectionByName[hashName(key)] = (i32)sections.size();
			sections.push_back(areas);
		}
	}
	return true;
}

bool GameXmlContent::LoadAchievements()
{
	constexpr eastl::hash<const char*> hashName;
	LoadSections();
	XMLDocument doc;
	if(!LoadXMLFile(L"/Design/DOCUMENT/ACHIEVEMENT.xml", doc, true)) return true;
	for(XMLElement* info = doc.FirstChildElement()->FirstChildElement("INFO"); info; info = info->NextSiblingElement("INFO")) {
		XMLElement* st = info->FirstChildElement("ST_ACHIEVEMENT");
		if(!st) continue;
		Achievement a;
		a.index = st->IntAttribute("_Index");
		a.sub = AchieveSubFromName(st->Attribute("_SubType"));
		if(a.index == 0 || a.sub == AchieveSub::NONE) continue;
		const char* group = st->Attribute("_Type");
		for(u8 g = 1; g < ARRAY_COUNT(ACHIEVE_GROUP_NAMES); g++) if(group && EA::StdC::Strcmp(group, ACHIEVE_GROUP_NAMES[g]) == 0) a.group = g;
		a.point = st->IntAttribute("_Point");
		a.gainTitle = st->IntAttribute("_GainTitle");
		if(XMLElement* d = st->FirstChildElement("_ST_ACHIEVEMENT_DATA")) {
			a.count = d->Int64Attribute("_count", 1);
			a.value = d->Int64Attribute("_value", 0);
			a.fvalue = d->FloatAttribute("_fvalue", 0);
			a.rank = ClearRankFromName(d->Attribute("_Rank"));
			if(const char* key = d->Attribute("_SectionKey")) {
				auto f = sectionByName.find(hashName(key));
				if(f != sectionByName.end()) a.section = f->second;
			}
			for(XMLElement* o = d->FirstChildElement(); o; o = o->NextSiblingElement()) {
				const char* name = o->Name();
				const char* data = o->Attribute("DATA");
				if(!data) continue;
				if(EA::StdC::Strcmp(name, "_Master") == 0) {
					auto f = masterClassStringMap.find(hashName(data));
					if(f != masterClassStringMap.end()) a.classType = (i32)f->second->classType;
				}
				else if(EA::StdC::Strcmp(name, "_GameType") == 0) {
					for(i32 t = 0; t < (i32)ARRAY_COUNT(ACHIEVE_GAME_TYPE_NAMES); t++) if(EA::StdC::Strcmp(data, ACHIEVE_GAME_TYPE_NAMES[t]) == 0) a.gameType = t;
				}
				else if(EA::StdC::Strcmp(name, "_CompleteObjectIndex") == 0 || EA::StdC::Strcmp(name, "_Ach") == 0 || EA::StdC::Strcmp(name, "_Area") == 0) {
					if(!a.objects.full()) a.objects.push_back(o->IntAttribute("DATA"));
				}
			}
		}
		achievementPos[a.index] = (i32)achievements.size();
		achievementsBySub[(i32)a.sub].push_back(a.index);
		if(a.sub == AchieveSub::COMMON_MULTI) foreach_const(o, a.objects) achievementParents[*o].push_back(a.index);
		achievements.push_back(a);
	}
	XMLDocument docTitle;
	if(LoadXMLFile(L"/Design/DOCUMENT/TITLE.xml", docTitle, true)) {
		static const struct { const char* name; StatType type; } TITLE_STATS[] = {
			{ "STAT_TYPE_HEALTH", StatType::STAT_TYPE_HEALTH }, { "STAT_TYPE_STAMINA", StatType::STAT_TYPE_STAMINA },
			{ "STAT_TYPE_HEALTHREGEN", StatType::STAT_TYPE_HEALTHREGEN }, { "STAT_TYPE_STAMINAREGEN", StatType::STAT_TYPE_STAMINAREGEN },
			{ "STAT_TYPE_ATTACK", StatType::STAT_TYPE_ATTACK }, { "STAT_TYPE_DEFENCE", StatType::STAT_TYPE_DEFENCE },
			{ "STAT_TYPE_SKILLPOWER", StatType::STAT_TYPE_SKILLPOWER }, { "STAT_TYPE_CRITICALRATE", StatType::STAT_TYPE_CRITICALRATE },
			{ "STAT_TYPE_CRITICALDAMAGE", StatType::STAT_TYPE_CRITICALDAMAGE }, { "STAT_TYPE_LIFESTEAL", StatType::STAT_TYPE_LIFESTEAL },
			{ "STAT_TYPE_MOVESPEED", StatType::STAT_TYPE_MOVESPEED }, { "STAT_TYPE_DEFENCE_PENETRATION", StatType::STAT_TYPE_DEFENCE_PENETRATION },
			{ "STAT_TYPE_TAG_HEALTHREGEN_VALUE", StatType::STAT_TYPE_TAG_HEALTHREGEN_VALUE }, { "STAT_TYPE_TAG_MANAREGEN_VALUE", StatType::STAT_TYPE_TAG_MANAREGEN_VALUE },
			{ "STAT_TYPE_MANAREGEN", StatType::STAT_TYPE_MANAREGEN }, { "STAT_TYPE_ENERGYREGEN", StatType::STAT_TYPE_ENERGYREGEN },
			{ "STAT_TYPE_ULTIMATEREGEN", StatType::STAT_TYPE_ULTIMATEREGEN },
		};
		for(XMLElement* info = docTitle.FirstChildElement()->FirstChildElement("INFO"); info; info = info->NextSiblingElement("INFO")) {
			XMLElement* st = info->FirstChildElement("ST_TITLEINFO");
			if(!st) continue;
			const i32 title = st->IntAttribute("_Index");
			for(XMLElement* e = st->FirstChildElement("_STATINFO"); e; e = e->NextSiblingElement("_STATINFO")) {
				const char* type = e->Attribute("_Type");
				for(const auto& t : TITLE_STATS) {
					if(!type || EA::StdC::Strcmp(type, t.name) != 0) continue;
					TitleStat ts;
					ts.type = (u8)t.type;
					ts.value = e->FloatAttribute("_Value");
					const char* vt = e->Attribute("_ValueType");
					ts.mul = vt && EA::StdC::Strcmp(vt, "MUL") == 0;
					auto& list = titleStats[title];
					if(!list.full()) list.push_back(ts);
				}
			}
		}
	}
	LOG("Loaded %d achievements, %d sections, %d titles with stats", (i32)achievements.size(), (i32)sections.size(), (i32)titleStats.size());
	return true;
}

AccountAchieveState* AccountAchievements::Find(i32 index)
{
	foreach(s, states) if(s->index == index) return &*s;
	return nullptr;
}

const AccountAchieveState* AccountAchievements::Find(i32 index) const
{
	foreach_const(s, states) if(s->index == index) return &*s;
	return nullptr;
}

AccountAchieveState& AccountAchievements::Get(i32 index)
{
	if(AccountAchieveState* s = Find(index)) return *s;
	AccountAchieveState n;
	n.index = index;
	states.push_back(n);
	return states.back();
}

i32 AccountAchievements::Score() const
{
	i32 score = 0;
	foreach_const(s, states) {
		if(!s->cleared) continue;
		if(const GameXmlContent::Achievement* a = GetGameXmlContent().FindAchievement(s->index)) score += a->point;
	}
	return score;
}

bool AccountAchievements::HasTitle(i32 title) const
{
	foreach_const(t, titles) if(*t == title) return true;
	return false;
}

i64 AchieveNow()
{
	static const i64 FILETIME_UNIX_OFFSET_SEC = 11644473600LL;
	return ((i64)time(nullptr) + FILETIME_UNIX_OFFSET_SEC) * 10000000LL;
}

static void Clear(AccountAchievements& acc, const GameXmlContent::Achievement& a, AccountAchieveState& st, AchieveChanges* changes, eastl::fixed_vector<i32,32,true>* cleared)
{
	st.cleared = 1;
	st.date = AchieveNow();
	if(a.gainTitle != 0 && !acc.HasTitle(a.gainTitle)) {
		acc.titles.push_back(a.gainTitle);
		changes->titles.push_back(a.gainTitle);
	}
	cleared->push_back(a.index);
}

static bool Matches(const GameXmlContent::Achievement& a, const AchieveEvent& ev)
{
	const GameXmlContent& content = GetGameXmlContent();
	if(a.classType >= 0 && ev.classType != a.classType) return false;
	if(a.gameType >= 0 && ev.gameType != a.gameType) return false;
	if(a.section >= 0 && ev.stage != 0 && !content.SectionContains(a.section, ev.stage)) return false;
	if(a.section >= 0 && ev.stage == 0 && a.sub != AchieveSub::COMMON_USE_SKILL) return false;
	if(a.value > 0 && ev.value < a.value) return false;
	if(a.fvalue > 0 && ev.fvalue < a.fvalue) return false;
	if(a.rank != 0 && (ev.rank == 0 || ev.rank > a.rank)) return false;
	if(a.objects.size() == 1 && a.sub != AchieveSub::COMMON_MULTI && ev.object != a.objects[0]) return false;
	if(a.objects.size() > 1) {
		bool found = false;
		foreach_const(o, a.objects) if(*o == ev.object) found = true;
		if(!found) return false;
	}
	return true;
}

static void ApplyOne(AccountAchievements& acc, const AchieveEvent& ev, AchieveChanges* changes, eastl::fixed_vector<i32,32,true>* cleared)
{
	const GameXmlContent& content = GetGameXmlContent();
	if(ev.sub == AchieveSub::NONE || ev.sub >= AchieveSub::COUNT) return;
	foreach_const(idx, content.achievementsBySub[(i32)ev.sub]) {
		const GameXmlContent::Achievement* a = content.FindAchievement(*idx);
		if(!a || !Matches(*a, ev)) continue;
		AccountAchieveState* existing = acc.Find(a->index);
		if(existing && existing->cleared) continue;
		AccountAchieveState& st = acc.Get(a->index);
		const i64 before = st.progress;
		const bool distinct = a->objects.size() > 1 || (a->sub == AchieveSub::PVE_STAGE_CLEAR && a->count > 1);
		if(distinct) {
			bool known = false;
			foreach_const(o, st.achieved) if(*o == ev.object) known = true;
			if(!known && !st.achieved.full()) st.achieved.push_back(ev.object);
			st.progress = (i64)st.achieved.size();
		}
		else if(ev.absolute) st.progress = MAX(st.progress, ev.amount);
		else st.progress += ev.amount;
		if(st.progress != before) changes->Touch(a->index);
		if(st.progress >= a->count) Clear(acc, *a, st, changes, cleared);
	}
}

void AchieveApply(AccountAchievements& acc, const AchieveEvent& ev, AchieveChanges* changes)
{
	const GameXmlContent& content = GetGameXmlContent();
	eastl::fixed_vector<i32,32,true> cleared;
	ApplyOne(acc, ev, changes, &cleared);
	for(i32 i = 0; i < (i32)cleared.size(); i++) {
		const i32 child = cleared[i];
		auto parents = content.achievementParents.find(child);
		if(parents != content.achievementParents.end()) {
			foreach_const(p, parents->second) {
				const GameXmlContent::Achievement* a = content.FindAchievement(*p);
				if(!a) continue;
				AccountAchieveState& st = acc.Get(*p);
				if(st.cleared) continue;
				bool known = false;
				foreach_const(o, st.achieved) if(*o == child) known = true;
				if(!known && !st.achieved.full()) st.achieved.push_back(child);
				st.progress = (i64)st.achieved.size();
				changes->Touch(*p);
				if(st.progress >= a->count) Clear(acc, *a, st, changes, &cleared);
			}
		}
		if(i == (i32)cleared.size() - 1) {
			AchieveEvent score;
			score.sub = AchieveSub::COMMON_ACHIEVEMENT_POINT;
			score.amount = acc.Score();
			score.absolute = true;
			ApplyOne(acc, score, changes, &cleared);
			AchieveEvent titles;
			titles.sub = AchieveSub::COMMON_TITLE_COUNT;
			titles.amount = (i64)acc.titles.size();
			titles.absolute = true;
			ApplyOne(acc, titles, changes, &cleared);
		}
	}
	foreach_const(i, cleared) changes->Touch(*i);
}

static AchieveEvent Event(AchieveSub sub, i64 amount, bool absolute = false)
{
	AchieveEvent ev;
	ev.sub = sub;
	ev.amount = amount;
	ev.absolute = absolute;
	return ev;
}

void AchieveApplyRecord(AccountAchievements& acc, const AccountGameRecord& r, AchieveChanges* changes)
{
	const bool pvp = r.gameType >= (i32)GameType::PVP_NORMAL && r.gameType <= (i32)GameType::PVP_FAKE_AI;
	const bool pve = r.gameType >= (i32)GameType::PVE_NORMAL && r.gameType <= (i32)GameType::PVE_TICKET;
	auto apply = [&](AchieveEvent ev) {
		ev.stage = r.stageIndex;
		ev.gameType = r.gameType;
		if(ev.object == 0) ev.object = r.stageIndex;
		AchieveApply(acc, ev, changes);
	};
	for(int mi = 0; mi < 2; mi++) {
		if(r.classTypes[mi] <= 0) continue;
		AchieveEvent ev = Event(AchieveSub::COMMON_MASTER_SORTIE, 1);
		ev.classType = r.classTypes[mi];
		apply(ev);
	}
	if(pvp) {
		apply(Event(AchieveSub::PVP_PLAY, 1));
		if(r.won) apply(Event(AchieveSub::PVP_WIN_TOTAL, 1));
		else if(r.draw) apply(Event(AchieveSub::PVP_DRAW_TOTAL, 1));
		else apply(Event(AchieveSub::PVP_LOSE_TOTAL, 1));
		if(r.deaths > 0) apply(Event(AchieveSub::PVP_DEATH_TOTAL, r.deaths));
		if(r.bestKillStreak > 0) apply(Event(AchieveSub::PVP_KILL_STREAK, r.bestKillStreak, true));
		static const i32 HYP_KILL_STREAK_MIN = 2;
		if(r.bestKillStreak >= HYP_KILL_STREAK_MIN) apply(Event(AchieveSub::PVP_KILL_STREAK_TOTAL, 1));
		AchieveEvent kda = Event(AchieveSub::PVP_KDA, 1);
		kda.fvalue = (f32)(r.kills + r.assists) / (f32)MAX(r.deaths, 1);
		apply(kda);
	}
	if(pve) {
		if(r.won) {
			AchieveEvent clear = Event(AchieveSub::PVE_STAGE_CLEAR, 1);
			clear.rank = r.clearRank;
			apply(clear);
		}
		if(r.deaths > 0) apply(Event(AchieveSub::PVE_DEATH_TOTAL, r.deaths));
	}
}

