#pragma once
#include <mxm/achievements.h>

struct Account;
struct AccountProgress;

void AchieveOnLogin(Account& account, const AccountProgress& progress);
