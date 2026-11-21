#include "achieve_rules.h"
#include "account.h"
#include <mxm/account_progress.h>

typedef GameXmlContent::AchieveSub AchieveSub;

static AchieveEvent Event(AchieveSub sub, i64 amount, bool absolute = false)
{
	AchieveEvent ev;
	ev.sub = sub;
	ev.amount = amount;
	ev.absolute = absolute;
	return ev;
}

void AchieveOnLogin(Account& account, const AccountProgress& progress)
{
	const wchar* nick = account.nickname.data();
	AccountAchievements acc = AccountAchievementsLoad(nick);
	AchieveChanges state;
	AchieveApply(acc, Event(AchieveSub::COMMON_LEVEL, progress.level, true), &state);
	AchieveApply(acc, Event(AchieveSub::COMMON_RON_ASSETS, progress.gold, true), &state);
	AchieveApply(acc, Event(AchieveSub::COMMON_FREE_CCOIN_ASSETS, progress.ccoin, true), &state);
	AchieveApply(acc, Event(AchieveSub::COMMON_FRIEND, (i64)AccountFriendsLoad(nick).friends.size(), true), &state);
	const GameXmlContent& content = GetGameXmlContent();
	foreach_const(m, content.masters) {
		AchieveEvent unlock = Event(AchieveSub::COMMON_MASTER_UNLOCK, 1, true);
		unlock.classType = (i32)m->classType;
		AchieveApply(acc, unlock, &state);
	}
	AchieveApply(acc, Event(AchieveSub::COMMON_MASTER_TOTAL, (i64)content.masters.size(), true), &state);
	const AccountInventory inventory = AccountInventoryLoad(nick);
	foreach_const(it, inventory.items) {
		AchieveEvent item = Event(AchieveSub::COMMON_ITEM, 1, true);
		item.object = it->docIndex;
		AchieveApply(acc, item, &state);
	}

	if(state.indices.empty() && state.titles.empty()) return;
	AccountAchievementsSave(nick, acc);
	LOG("Achievements :: %ls %d state change(s), score %d, %d title(s)", nick, (i32)state.indices.size(), acc.Score(), (i32)acc.titles.size());
}
