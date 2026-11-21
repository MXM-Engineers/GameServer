#include "game.h"
#include "coordinator.h"
#include <mxm/summary.h>
#include <mxm/achievements.h>
#include <mxm/account_progress.h>
#include <EASTL/sort.h>
#include <mxm/mission_progress.h>

static const i32 MEASURED_SUMMARY_REWARD_MONEY_REASON = 15;
static const u32 HYP_SUMMARY_ITEM_UPDATE_REASON = 3;
static const u32 HYP_SUMMARY_ITEM_ACQUISITION_REASON = 3;
static const u32 HYP_SUMMARY_REWARD_REFUSED = 1;
static const u8 SUMMARY_REWARD_GOLD = 1;
static const u8 SUMMARY_REWARD_EXP = 2;
static const u8 SUMMARY_REWARD_CCOIN = 3;
static const u8 SUMMARY_REWARD_ITEM = 4;

template<typename Writer>
static void WriteSummaryEntry(Writer& packet, const AccountSummaryEntry& e)
{
	packet.template Write<i32>(e.summary);
	packet.template Write<i32>(e.stage);
	packet.template Write<i32>((i32)e.type);
	packet.template Write<i32>(e.data);
	packet.template Write<u8>(e.received);
	packet.template Write<i64>(e.date);
}

static const GameXmlContent::SummaryReward* RewardOf(const GameXmlContent::Summary& summary, u8 type, i32 data)
{
	if(type == (u8)SummaryEntryType::RARE_MONSTER) foreach_const(r, summary.rares) if(r->first == data) return &r->second;
	if(type == (u8)SummaryEntryType::CHALLENGE) foreach_const(c, summary.challenges) if(c->index == data) return &c->reward;
	if(type == (u8)SummaryEntryType::PROGRESS && data >= 1 && data <= (i32)summary.progress.size()) return &summary.progress[data - 1];
	return nullptr;
}

void HubGame::OnSelectBadge(ClientHandle clientHd, u8 badgeType)
{
	const i32 userID = plidMap->Get(clientHd);
	const wchar* nick = AccountNickname(clientHd);
	if(!nick) return;
	AccountAchievements acc = AccountAchievementsLoad(nick);
	acc.badge = badgeType;
	AccountAchievementsSave(nick, acc);
	if(WorldHub::ActorPlayer* actor = world.FindPlayerActor(playerActorUID[userID])) {
		actor->badgeType = badgeType;
		actor->badgeTier = badgeType == MEASURED_BADGE_ACHIEVEMENT ? GetGameXmlContent().AchievementTier(acc.Score()) : 0;
	}
	PacketWriter<Sv::SA_SelectBadge,8> answer;
	answer.Write<u8>(badgeType);
	replication.SendWriter(clientHd, answer);
	LOG("[client%x] Badge :: %ls selects badge %u", clientHd, nick, (u32)badgeType);
}

void HubGame::OnTitleSelect(ClientHandle clientHd, i32 displayTitle, i32 statTitle)
{
	static const u32 MEASURED_ERROR_TITLE_SELECT_FAIL = 103;
	const i32 userID = plidMap->Get(clientHd);
	const wchar* nick = AccountNickname(clientHd);
	if(!nick) return;
	AccountAchievements acc = AccountAchievementsLoad(nick);
	auto valid = [&](i32 t) { return t <= 0 || acc.HasTitle(t); };
	const ActorUID actorUID = playerActorUID[userID];
	const bool statTitleHasStats = statTitle <= 0 || GetGameXmlContent().FindTitleStats(statTitle) != nullptr;
	if(!valid(displayTitle) || !valid(statTitle) || !statTitleHasStats) {
		replication.SendTitleSelect(clientHd, MEASURED_ERROR_TITLE_SELECT_FAIL, actorUID, acc.displayTitle, acc.statTitle);
		return;
	}
	acc.displayTitle = displayTitle;
	acc.statTitle = statTitle;
	AccountAchievementsSave(nick, acc);
	if(WorldHub::ActorPlayer* actor = world.FindPlayerActor(actorUID)) {
		actor->displayTitle = displayTitle;
		actor->statTitle = statTitle;
	}
	replication.SendTitleSelect(clientHd, 0, actorUID, displayTitle, statTitle);
	LOG("[client%x] Title :: %ls display %d stat %d", clientHd, nick, displayTitle, statTitle);
}

void HubGame::SendSummaries(ClientHandle clientHd, const wchar* nickname)
{
	const AccountSummaries s = AccountSummariesLoad(nickname);
	{
		PacketWriter<Sv::SN_SummaryInfoAll,16> packet;
		packet.Write<u16>(0);
		replication.SendWriter(clientHd, packet);
	}
	{
		eastl::fixed_map<i32,i32,64,true> pending;
		foreach_const(e, s.entries) if(!e->received && (e->type == (u8)SummaryEntryType::RARE_MONSTER || e->type == (u8)SummaryEntryType::CHALLENGE)) pending[e->summary]++;
		PacketWriter<Sv::SN_AvailableSummaryRewardCountList,1024> packet;
		packet.Write<u16>((u16)(pending.size() * 2));
		foreach_const(p, pending) {
			packet.Write<i32>(p->first);
			packet.Write<i32>(p->second);
		}
		replication.SendWriter(clientHd, packet);
	}
	{
		static const i32 MEASURED_SUMMARY_LATEST = 10;
		eastl::vector<const AccountSummaryEntry*> sorted;
		foreach_const(e, s.entries) sorted.push_back(&*e);
		eastl::sort(sorted.begin(), sorted.end(), [](const AccountSummaryEntry* a, const AccountSummaryEntry* b) { return a->date > b->date; });
		const i32 n = MIN((i32)sorted.size(), MEASURED_SUMMARY_LATEST);
		PacketWriter<Sv::SN_SummaryInfoLatest,512> packet;
		packet.Write<u16>((u16)n);
		for(i32 i = 0; i < n; i++) WriteSummaryEntry(packet, *sorted[i]);
		replication.SendWriter(clientHd, packet);
	}
}

void HubGame::OnSummaryInfoEach(ClientHandle clientHd, i32 summary)
{
	const wchar* nick = AccountNickname(clientHd);
	const AccountSummaries s = nick ? AccountSummariesLoad(nick) : AccountSummaries();
	PacketWriter<Sv::SA_RequestSummaryInfoEach,2048> packet;
	packet.Write<i32>(summary);
	i32 n = 0;
	foreach_const(e, s.entries) if(e->summary == summary) n++;
	packet.Write<u16>((u16)n);
	foreach_const(e, s.entries) if(e->summary == summary) WriteSummaryEntry(packet, *e);
	replication.SendWriter(clientHd, packet);
}

void HubGame::OnSummaryReward(ClientHandle clientHd, i32 summaryIndex, u8 type, i32 data)
{
	Account* account = GearAccount(clientHd);
	const GameXmlContent::Summary* summary = GetGameXmlContent().FindSummary(summaryIndex);
	AccountSummaryEntry result;
	result.summary = summaryIndex;
	result.stage = summary ? summary->stage : 0;
	result.type = type;
	result.data = data;
	auto refuse = [&]() {
		PacketWriter<Sv::SN_SummaryRewardResult,64> packet;
		packet.Write<u32>(HYP_SUMMARY_REWARD_REFUSED);
		WriteSummaryEntry(packet, result);
		replication.SendWriter(clientHd, packet);
	};
	if(!account || !summary) { refuse(); return; }
	AccountSummaries s = AccountSummariesLoad(account->nickname.data());
	const GameXmlContent::SummaryReward* reward = RewardOf(*summary, type, data);
	AccountSummaryEntry* entry = s.Find(summaryIndex, type, data);
	if(type == (u8)SummaryEntryType::PROGRESS && !entry) {
		i32 completed = 0;
		foreach_const(e, s.entries) if(e->summary == summaryIndex && (e->type == (u8)SummaryEntryType::RARE_MONSTER || e->type == (u8)SummaryEntryType::CHALLENGE)) completed++;
		if(completed > 0) {
			s.Add(summaryIndex, summary->stage, type, data, AchieveNow());
			entry = s.Find(summaryIndex, type, data);
		}
	}
	if(!reward || !entry || entry->received) { refuse(); return; }
	entry->received = 1;
	AccountSummariesSave(account->nickname.data(), s);

	AccountProgress progress = AccountProgressLoad(account->nickname.data());
	if(reward->type == SUMMARY_REWARD_GOLD) progress.gold += reward->amount;
	else if(reward->type == SUMMARY_REWARD_EXP) ProgressGainExp(&progress, (u32)reward->amount);
	else if(reward->type == SUMMARY_REWARD_CCOIN) progress.ccoin += reward->amount;
	AccountProgressSave(account->nickname.data(), progress);
	account->level = progress.level;
	account->levelExp = progress.levelExp;
	account->gold = progress.gold;
	account->ccoin = progress.ccoin;
	if(reward->type == SUMMARY_REWARD_GOLD) replication.SendMoney(clientHd, account->gold, MEASURED_SUMMARY_REWARD_MONEY_REASON);
	else if(reward->type == SUMMARY_REWARD_CCOIN) replication.SendCCoin(clientHd, account->ccoin, MEASURED_SUMMARY_REWARD_MONEY_REASON);
	else if(reward->type == SUMMARY_REWARD_ITEM) GrantItem(plidMap->Get(clientHd), reward->item, reward->amount, HYP_SUMMARY_ITEM_UPDATE_REASON, HYP_SUMMARY_ITEM_ACQUISITION_REASON);
	PacketWriter<Sv::SN_SummaryRewardResult,64> packet;
	packet.Write<u32>(0);
	WriteSummaryEntry(packet, *entry);
	replication.SendWriter(clientHd, packet);
	LOG("[client%x] Summary :: %ls reward %d type %u data %d (%u x %d)", clientHd, account->nickname.data(), summaryIndex, (u32)type, data, (u32)reward->type, reward->amount);
}
