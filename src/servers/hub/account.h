#pragma once
#include <common/inner_protocol.h>
#include <common/utils.h>
#include <common/network.h> // MAX_CLIENTS
#include <storage/game_store.h>

struct AccountLogin
{
	WideString nickname;
	WideString guildTag;
	PlayerProfile profile;
	bool profileStored = false;
};

struct Account
{
	const AccountUID UID;
	WideString nickname;
	WideString guildTag;
	PlayerProfile profile;
	bool profileStored = false;

	Account(AccountUID accountUID_): UID(accountUID_) {}

	void ApplyLogin(const AccountLogin& login);
};


struct AccountManager
{
	eastl::fixed_list<Account,MAX_CLIENTS> accountList;
	hash_map<AccountUID, decltype(accountList)::iterator,MAX_CLIENTS> accountMap;
};

AccountManager& GetAccountManager();
