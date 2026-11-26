#include "account.h"

static AccountManager g_AccountManager;

AccountManager& GetAccountManager()
{
	return g_AccountManager;
}

void Account::ApplyLogin(const AccountLogin& login)
{
	ASSERT(login.profile.accountUID == UID);
	nickname = login.nickname;
	guildTag = login.guildTag;
	profile = login.profile;
	profileStored = login.profileStored;
}
