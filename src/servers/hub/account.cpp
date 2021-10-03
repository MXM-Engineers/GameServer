#include "account.h"

static AccountManager g_AccountManager;

AccountManager& GetAccountManager()
{
	return g_AccountManager;
}
