#include "summary.h"
#include <EAStdC/EAString.h>

using namespace tinyxml2;

static const char* SUMMARY_REWARD_TYPES[] = { "", "GOLD", "EXP", "CCOIN", "ITEM" };
static const char* SUMMARY_CHALLENGE_TYPES[] = {
	"", "SUMMARY_CHALLENGE_TYPE_NODIE", "SUMMARY_CHALLENGE_TYPE_STATUSTYPE", "SUMMARY_CHALLENGE_TYPE_NOHITBYBOSS", "SUMMARY_CHALLENGE_TYPE_KILL_IN_TIME",
	"SUMMARY_CHALLENGE_TYPE_RANK_OVER", "SUMMARY_CHALLENGE_TYPE_NOKILL", "SUMMARY_CHALLENGE_TYPE_KILL_GREATER_EQUAL", "SUMMARY_CHALLENGE_TYPE_STATUSINDEX",
	"SUMMARY_CHALLENGE_TYPE_KILL_LESS",
};
static const char* SUMMARY_DIFFICULTIES[] = {
	"WORLD_STAGE_DIFFICULT_EASY", "WORLD_STAGE_DIFFICULT_NORMAL", "WORLD_STAGE_DIFFICULT_HARD", "WORLD_STAGE_DIFFICULT_EXTREME", "WORLD_STAGE_DIFFICULT_HELL",
};
static const char* SUMMARY_STAGE_ATTRIBUTES[] = { "_EasyStageIndex", "_NormalStageIndex", "_HardStageIndex", "_ExtremeStageIndex", "_HellStageIndex" };
static const char* SUMMARY_RANKS[] = { "", "E_CLEAR_RANK_TYPE_SSS", "E_CLEAR_RANK_TYPE_SS", "E_CLEAR_RANK_TYPE_S", "E_CLEAR_RANK_TYPE_A", "E_CLEAR_RANK_TYPE_B", "E_CLEAR_RANK_TYPE_C" };

static i32 IndexOf(const char* value, const char* const* names, i32 count)
{
	if(!value) return -1;
	for(i32 i = 0; i < count; i++) if(EA::StdC::Strcmp(value, names[i]) == 0) return i;
	return -1;
}

static GameXmlContent::SummaryReward ReadReward(XMLElement* parent)
{
	GameXmlContent::SummaryReward r;
	XMLElement* e = parent ? parent->FirstChildElement("_Reward") : nullptr;
	if(!e) return r;
	const i32 t = IndexOf(e->Attribute("_Type"), SUMMARY_REWARD_TYPES, ARRAY_COUNT(SUMMARY_REWARD_TYPES));
	r.type = t > 0 ? (u8)t : 0;
	r.amount = e->IntAttribute("_Amount");
	r.item = e->IntAttribute("_ItemIndex");
	return r;
}

bool GameXmlContent::LoadSummaries()
{
	XMLDocument docArea;
	if(LoadXMLFile(L"/Design/DOCUMENT/AREALIST.xml", docArea, true)) {
		for(XMLElement* info = docArea.FirstChildElement()->FirstChildElement(); info; info = info->NextSiblingElement()) {
			XMLElement* body = info->FirstChildElement();
			for(XMLElement* st = body ? body->FirstChildElement("_StageList") : nullptr; st; st = st->NextSiblingElement("_StageList")) {
				const i32 summary = st->IntAttribute("_SummaryIndex");
				if(summary == 0) continue;
				for(i32 d = 0; d < (i32)ARRAY_COUNT(SUMMARY_STAGE_ATTRIBUTES); d++) {
					const i32 stage = st->IntAttribute(SUMMARY_STAGE_ATTRIBUTES[d]);
					if(stage != 0) stageSummary[stage] = eastl::make_pair(summary, d);
				}
			}
		}
	}

	XMLDocument doc;
	if(!LoadXMLFile(L"/Design/DOCUMENT/SUMMARY.xml", doc, true)) return true;
	for(XMLElement* info = doc.FirstChildElement()->FirstChildElement("INFO"); info; info = info->NextSiblingElement("INFO")) {
		XMLElement* st = info->FirstChildElement("ST_SUMMARY");
		if(!st) continue;
		Summary s;
		s.index = st->IntAttribute("_Index");
		s.stage = st->IntAttribute("_StageIndex");
		for(XMLElement* e = st->FirstChildElement("_RARE_MONSTER_LIST"); e && !s.rares.full(); e = e->NextSiblingElement("_RARE_MONSTER_LIST")) {
			s.rares.push_back(eastl::make_pair(e->IntAttribute("_CreatureIndex"), ReadReward(e)));
		}
		for(XMLElement* e = st->FirstChildElement("_CHALLENGE_LIST"); e && !s.challenges.full(); e = e->NextSiblingElement("_CHALLENGE_LIST")) {
			SummaryChallengeInfo c;
			c.index = e->IntAttribute("_index");
			const i32 type = IndexOf(e->Attribute("_eType"), SUMMARY_CHALLENGE_TYPES, ARRAY_COUNT(SUMMARY_CHALLENGE_TYPES));
			c.type = type > 0 ? (SummaryChallenge)type : SummaryChallenge::NONE;
			c.difficulty = IndexOf(e->Attribute("_Difficulty"), SUMMARY_DIFFICULTIES, ARRAY_COUNT(SUMMARY_DIFFICULTIES));
			const i32 rank = IndexOf(e->Attribute("_rank"), SUMMARY_RANKS, ARRAY_COUNT(SUMMARY_RANKS));
			c.rank = rank > 0 ? (u8)rank : 0;
			c.timeSec = e->FloatAttribute("_fTime");
			c.totalCount = e->IntAttribute("_TotalCount");
			c.status = e->IntAttribute("_StatusIndex");
			if(const char* statusType = e->Attribute("_StatusType")) c.status = (i32)StatusTypeFromString(statusType);
			for(XMLElement* t = e->FirstChildElement("_TargetInfo"); t && !c.targets.full(); t = t->NextSiblingElement("_TargetInfo")) {
				c.targets.push_back(eastl::make_pair(t->IntAttribute("_Index"), t->IntAttribute("_Count")));
			}
			c.reward = ReadReward(e);
			s.challenges.push_back(c);
		}
		for(XMLElement* e = st->FirstChildElement("_PROGRESS_REWARD"); e && !s.progress.full(); e = e->NextSiblingElement("_PROGRESS_REWARD")) {
			s.progress.push_back(ReadReward(e));
		}
		summaries.push_back(s);
	}
	LOG("Loaded %d stage summaries (%d stages)", (i32)summaries.size(), (i32)stageSummary.size());
	return true;
}

const GameXmlContent::Summary* GameXmlContent::FindStageSummary(i32 stage, i32* difficulty) const
{
	auto f = stageSummary.find(stage);
	if(f == stageSummary.end()) return nullptr;
	if(difficulty) *difficulty = f->second.second;
	return FindSummary(f->second.first);
}

AccountSummaryEntry* AccountSummaries::Find(i32 summary, u8 type, i32 data)
{
	foreach(e, entries) if(e->summary == summary && e->type == type && e->data == data) return &*e;
	return nullptr;
}

bool AccountSummaries::Add(i32 summary, i32 stage, u8 type, i32 data, i64 date)
{
	if(Find(summary, type, data)) return false;
	AccountSummaryEntry e;
	e.summary = summary;
	e.stage = stage;
	e.type = type;
	e.data = data;
	e.date = date;
	entries.push_back(e);
	return true;
}
