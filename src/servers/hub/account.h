#pragma once
#include <common/inner_protocol.h>
#include <common/utils.h>
#include <common/network.h> // MAX_CLIENTS

struct Account
{
	const AccountUID UID;
	WideString nickname;
	WideString guildTag;
	i32 leaderMasterID;
	bool pveFailedReturn = false;
	i32 pendingGuildPoints = 0;
	u16 level = 1;
	u32 levelExp = 0;
	i64 gold = 0;
	u32 wins = 0;
	u32 draws = 0;
	u32 losses = 0;
	i64 ccoin = 0;

	// TODO: add to this

	Account(AccountUID accountUID_): UID(accountUID_) {}
};


struct AccountManager
{
	eastl::fixed_list<Account,MAX_CLIENTS> accountList;
	hash_map<AccountUID, decltype(accountList)::iterator,MAX_CLIENTS> accountMap;
};

AccountManager& GetAccountManager();
