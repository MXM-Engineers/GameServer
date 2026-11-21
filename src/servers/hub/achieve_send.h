#pragma once
#include <common/base.h>
#include <common/protocol.h>
#include <mxm/achievements.h>

template<typename Send>
void AchieveRaise(const wchar* nickname, const AchieveEvent& ev, Send send)
{
	AccountAchievements acc = AccountAchievementsLoad(nickname);
	AchieveChanges changes;
	AchieveApply(acc, ev, &changes);
	if(changes.indices.empty() && changes.titles.empty()) return;
	AccountAchievementsSave(nickname, acc);
	AchieveSendChanges(acc, changes, send);
}
