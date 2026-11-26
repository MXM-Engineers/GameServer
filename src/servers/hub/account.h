#pragma once
#include <common/inner_protocol.h>
#include <common/utils.h>
#include <common/network.h> // MAX_CLIENTS
#include <storage/game_store.h>

struct Account
{
	const AccountUID UID;
	WideString nickname;
	WideString guildTag;
	i32 leaderMasterID;
	PlayerProfile profile;
	bool profileStored = false;

	// TODO: add to this

	Account(AccountUID accountUID_): UID(accountUID_) {}
};


struct AccountManager
{
	eastl::fixed_list<Account,MAX_CLIENTS> accountList;
	hash_map<AccountUID, decltype(accountList)::iterator,MAX_CLIENTS> accountMap;
};

AccountManager& GetAccountManager();
