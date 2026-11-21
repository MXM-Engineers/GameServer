#pragma once
#include <common/base.h>
#include <common/utils.h>
#include <EASTL/vector.h>
#include <EASTL/fixed_vector.h>

struct PveRankingMember
{
	i32 mainClass = 0;
	i32 subClass = 0;
	WideString nick;
};

struct PveRankingEntry
{
	i32 stage = 0;
	u32 totalScore = 0;
	u32 playMs = 0;
	u32 deaths = 0;
	u8 clearRank = 0;
	eastl::fixed_vector<PveRankingMember,5,false> members;
	bool HasMember(const wchar* nick) const;
};

void PveRankingSubmit(const PveRankingEntry& entry);
eastl::vector<PveRankingEntry> PveRankingLoad(i32 stage);
i32 PveRankingFindBest(const eastl::vector<PveRankingEntry>& ranking, const wchar* nick);
void PveRankingSetLastGame(const wchar* nickname, i32 stage);
i32 PveRankingTakeLastGame(const wchar* nickname);

template<typename Writer>
void WritePveRankingEntry(Writer& packet, const PveRankingEntry& e, u32 position)
{
	packet.template Write<u32>(position);
	packet.template Write<u32>(e.totalScore);
	packet.template Write<u32>(e.playMs);
	packet.template Write<u32>(e.deaths);
	packet.template Write<u16>((u16)e.members.size());
	foreach_const(m, e.members) {
		packet.template Write<i32>(m->mainClass);
		packet.template Write<i32>(m->subClass);
		packet.WriteStringObj(m->nick.data());
	}
}

template<typename Writer>
void WriteDefenceRankingEntry(Writer& packet, const PveRankingEntry& e, u32 position)
{
	packet.template Write<u32>(position);
	packet.template Write<u32>(e.totalScore);
	packet.template Write<u32>(e.playMs);
	packet.template Write<u16>((u16)e.members.size());
	foreach_const(m, e.members) {
		packet.template Write<i32>(m->mainClass);
		packet.template Write<i32>(m->subClass);
		packet.WriteStringObj(m->nick.data());
	}
}
