#pragma once
#include "game_content.h"
#include <EASTL/vector.h>

enum class SummaryEntryType: u8 { RARE_MONSTER = 2, CHALLENGE = 3, PROGRESS = 4 };

struct AccountSummaryEntry
{
	i32 summary = 0;
	i32 stage = 0;
	u8 type = 0;
	i32 data = 0;
	u8 received = 0;
	i64 date = 0;
};

struct AccountSummaries
{
	eastl::vector<AccountSummaryEntry> entries;
	AccountSummaryEntry* Find(i32 summary, u8 type, i32 data);
	bool Add(i32 summary, i32 stage, u8 type, i32 data, i64 date);
};

AccountSummaries AccountSummariesLoad(const wchar* nickname);
void AccountSummariesSave(const wchar* nickname, const AccountSummaries& summaries);
