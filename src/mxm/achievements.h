#pragma once
#include "game_content.h"
#include <common/base.h>
#include <common/protocol.h>
#include <EASTL/vector.h>

struct AccountAchieveState
{
	i32 index = 0;
	i64 progress = 0;
	u8 cleared = 0;
	i64 date = 0;
	eastl::fixed_vector<i32,16,false> achieved;
};

static const u8 MEASURED_BADGE_ACHIEVEMENT = 1;
static const i32 MEASURED_WAREHOUSE_LINES = 9;

struct AccountAchievements
{
	eastl::vector<AccountAchieveState> states;
	eastl::vector<i32> titles;
	i32 displayTitle = -1;
	i32 statTitle = -1;
	u8 badge = MEASURED_BADGE_ACHIEVEMENT;

	AccountAchieveState* Find(i32 index);
	const AccountAchieveState* Find(i32 index) const;
	AccountAchieveState& Get(i32 index);
	i32 Score() const;
	bool HasTitle(i32 title) const;
};

AccountAchievements AccountAchievementsLoad(const wchar* nickname);
void AccountAchievementsSave(const wchar* nickname, const AccountAchievements& achievements);

struct AchieveEvent
{
	GameXmlContent::AchieveSub sub = GameXmlContent::AchieveSub::NONE;
	i32 stage = 0;
	i32 object = 0;
	i32 classType = -1;
	i32 gameType = -1;
	i64 amount = 1;
	i64 value = 0;
	f32 fvalue = 0;
	u8 rank = 0;
	bool absolute = false;
};

struct AchieveChanges
{
	eastl::fixed_vector<i32,64,true> indices;
	eastl::fixed_vector<i32,16,true> titles;
	void Touch(i32 index) { foreach_const(i, indices) if(*i == index) return; indices.push_back(index); }
};

i64 AchieveNow();
void AchieveApply(AccountAchievements& acc, const AchieveEvent& ev, AchieveChanges* changes);
GameXmlContent::AchieveSub AchieveSubFromName(const char* name);
void AchieveApplyRecord(AccountAchievements& acc, const struct AccountGameRecord& record, AchieveChanges* changes);

template<typename Writer>
void WriteAchieveState(Writer& packet, const AccountAchieveState& st)
{
	const GameXmlContent::Achievement* a = GetGameXmlContent().FindAchievement(st.index);
	packet.template Write<i32>(st.index);
	packet.template Write<i32>(a ? (i32)a->group : 0);
	packet.template Write<u8>(st.cleared);
	packet.template Write<u16>((u16)st.achieved.size());
	foreach_const(x, st.achieved) packet.template Write<i32>(*x);
	packet.template Write<i64>(st.cleared ? 0 : st.progress);
	packet.template Write<i64>(st.cleared ? st.date : 0);
}

template<typename Send>
void AchieveSendChanges(const AccountAchievements& acc, const AchieveChanges& changes, Send send)
{
	const i32 score = acc.Score();
	foreach_const(i, changes.indices) {
		const AccountAchieveState* st = acc.Find(*i);
		if(!st) continue;
		PacketWriter<Sv::SN_AchieveUpdate,256> packet;
		packet.Write<i32>(score);
		WriteAchieveState(packet, *st);
		send(packet);
	}
	foreach_const(t, changes.titles) {
		PacketWriter<Sv::SN_TitleAdd,16> packet;
		packet.Write<i32>(*t);
		send(packet);
	}
}

