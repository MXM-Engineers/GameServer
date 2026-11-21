#include <common/packet_serialize.h>
#include "arsenal.h"
#include "account.h"
#include "game.h"
#include "achieve_send.h"

static const i32 MEASURED_MONEY_REASON_WEAPON_UNLOCK = 44;
static const i32 MEASURED_MONEY_REASON_SKILL_UNLOCK = 4;
static const u32 MEASURED_ITEM_UPDATE_REASON_WEAPON_UPGRADE = 204;
static const i32 HYP_MONEY_REASON_SKILL_UPGRADE = 4;
static const u32 ARSENAL_RESULT_OK = 0;
static const u32 HYP_ARSENAL_RESULT_FAILED = 1;
static const u32 HYP_ARSENAL_RESULT_NO_MONEY = 2;

static const GameXmlContent::Master* MasterOfCharacter(u32 characterID)
{
	const ClassType cls = (ClassType)((i32)characterID - (i32)LocalActorID::FIRST_SELF_MASTER);
	auto f = GetGameXmlContent().masterClassTypeMap.find(cls);
	return f == GetGameXmlContent().masterClassTypeMap.end() ? nullptr : f->second;
}

bool IsArsenalPacket(u16 netID)
{
	return netID == MEASURED_CQ_WEAPON_UNLOCK || netID == MEASURED_CQ_WEAPON_EQUIP || netID == MEASURED_CQ_SKILL_UNLOCK || netID == MEASURED_CQ_SKILL_EQUIP || netID == MEASURED_CQ_SKILL_UPGRADE || netID == MEASURED_CQ_WEAPON_UPGRADE;
}

i32 ArsenalWeaponIndex(const GameXmlContent::Master& master, u32 type, u32 grade)
{
	return GetGameXmlContent().WeaponIndexFor(master, type, grade);
}

i32 ArsenalEquippedWeapon(const AccountArsenal& arsenal, const GameXmlContent::Master& master, i32* grade)
{
	const u8 type = arsenal.EquippedType((i32)master.classType);
	const u8 g = arsenal.Grade((i32)master.classType, type);
	const i32 index = ArsenalWeaponIndex(master, type, g);
	if(grade) *grade = index >= 0 ? g : 0;
	return index;
}

static bool SkillAvailable(const AccountArsenal& arsenal, const GameXmlContent::Master& master, i32 skill, i32* order)
{
	for(i32 i = 0; i < (i32)master.skillIDs.size() && i < 4; i++) {
		if((i32)master.skillIDs[i] != skill) continue;
		if(order) *order = i < (i32)master.skillUnlockOrder.size() ? master.skillUnlockOrder[i] : 0;
		return i >= (i32)master.skillUnlocked.size() || master.skillUnlocked[i] || arsenal.SkillUnlocked((i32)master.classType, skill);
	}
	if(order) *order = -1;
	return false;
}

void ArsenalSkills(const AccountArsenal& arsenal, const GameXmlContent::Master& master, SkillID* out)
{
	out[0] = master.skillIDs[0];
	out[1] = master.skillIDs[1];
	const AccountArsenal::SkillLoadout* l = arsenal.FindLoadout((i32)master.classType);
	if(!l) return;
	for(int s = 0; s < 2; s++) if(l->skills[s] && SkillAvailable(arsenal, master, l->skills[s], nullptr)) out[s] = (SkillID)l->skills[s];
	if(out[0] == out[1]) { out[0] = master.skillIDs[0]; out[1] = master.skillIDs[1]; }
}

static bool IsLockedSlot(const GameXmlContent::Master& master, i32 skill)
{
	for(i32 i = 0; i < (i32)master.skillIDs.size() && i < (i32)master.skillUnlocked.size(); i++) if((i32)master.skillIDs[i] == skill) return !master.skillUnlocked[i];
	return false;
}

static bool ArsenalSpend(Account& account, u8 buyType, i32 amount, i32 reason, ArsenalResult* out)
{
	if(amount <= 0) return true;
	AccountProgress progress = AccountProgressLoad(account.nickname.data());
	i64& wallet = buyType == BUY_TYPE_GOLD ? progress.gold : progress.ccoin;
	if(wallet < amount) return false;
	wallet -= amount;
	AccountProgressSave(account.nickname.data(), progress);
	account.gold = progress.gold;
	account.ccoin = progress.ccoin;
	out->spentType = buyType == BUY_TYPE_GOLD ? BUY_TYPE_GOLD : BUY_TYPE_CCOIN;
	out->wallet = wallet;
	out->moneyReason = reason;
	return true;
}

static bool WeaponUnlocked(const AccountArsenal& arsenal, i32 classType, u32 type)
{
	const AccountArsenal::Weapon* state = arsenal.FindWeapon(classType, (u8)type);
	return type == 1 || (state && state->unlocked);
}

static AccountArsenal::Weapon* WeaponState(AccountArsenal& arsenal, i32 classType, u32 type)
{
	AccountArsenal::Weapon* state = arsenal.FindWeapon(classType, (u8)type);
	if(state || arsenal.weapons.full()) return state;
	AccountArsenal::Weapon n;
	n.classType = classType;
	n.type = (u8)type;
	arsenal.weapons.push_back(n);
	return &arsenal.weapons.back();
}

static void WeaponUnlock(Account& account, const GameXmlContent::Master& master, AccountArsenal& arsenal, ArsenalResult* r)
{
	const i32 cls = (i32)master.classType;
	if(r->type >= GetGameXmlContent().weaponUnlockCost.size() || ArsenalWeaponIndex(master, r->type, 0) < 0) return;
	if(!WeaponUnlocked(arsenal, cls, r->type)) {
		const GameXmlContent::PriceCost& cost = GetGameXmlContent().weaponUnlockCost[r->type];
		if(!ArsenalSpend(account, cost.buyType, cost.amount, MEASURED_MONEY_REASON_WEAPON_UNLOCK, r)) {
			r->result = HYP_ARSENAL_RESULT_NO_MONEY;
			return;
		}
		if(AccountArsenal::Weapon* state = WeaponState(arsenal, cls, r->type)) state->unlocked = 1;
		AccountArsenalSave(account.nickname.data(), arsenal);
		LOG("Arsenal :: %ls unlocks weapon type %u of class %d (%d)", account.nickname.data(), r->type, cls, cost.amount);
	}
	r->grade = arsenal.Grade(cls, (u8)r->type);
	r->weaponIndex = ArsenalWeaponIndex(master, r->type, (u32)r->grade);
	r->unlocked = 1;
	r->result = ARSENAL_RESULT_OK;
}

static void WeaponEquip(Account& account, const GameXmlContent::Master& master, AccountArsenal& arsenal, ArsenalResult* r)
{
	const i32 cls = (i32)master.classType;
	const i32 index = ArsenalWeaponIndex(master, r->type, arsenal.Grade(cls, (u8)r->type));
	if(index < 0) return;
	if(!WeaponUnlocked(arsenal, cls, r->type)) {
		WeaponUnlock(account, master, arsenal, r);
		if(r->result != ARSENAL_RESULT_OK) return;
		r->unlockedOnEquip = true;
		r->result = HYP_ARSENAL_RESULT_FAILED;
	}
	arsenal.SetEquipped(cls, (u8)r->type);
	AccountArsenalSave(account.nickname.data(), arsenal);
	r->weaponIndex = index;
	r->result = ARSENAL_RESULT_OK;
	LOG("Arsenal :: %ls equips weapon %d on class %d", account.nickname.data(), index, cls);
}

static void WeaponUpgrade(Account& account, const GameXmlContent::Master& master, AccountArsenal& arsenal, ArsenalResult* r)
{
	const i32 cls = (i32)master.classType;
	const u32 grade = arsenal.Grade(cls, (u8)r->type);
	const i32 next = ArsenalWeaponIndex(master, r->type, grade + 1);
	r->weaponIndex = ArsenalWeaponIndex(master, r->type, grade);
	r->grade = (i32)grade;
	r->unlocked = WeaponUnlocked(arsenal, cls, r->type) ? 1 : 0;
	if(!r->unlocked || next < 0) return;
	AccountInventory inventory = AccountInventoryLoad(account.nickname.data());
	auto need = GetGameXmlContent().weaponIngredients.find(next);
	if(need != GetGameXmlContent().weaponIngredients.end()) {
		foreach_const(n, need->second) {
			i32 owned = 0;
			foreach_const(it, inventory.items) if(it->docIndex == n->first) owned += it->count;
			if(owned < n->second) return;
		}
		foreach_const(n, need->second) {
			i32 left = n->second;
			for(auto it = inventory.items.begin(); it != inventory.items.end() && left > 0; ) {
				if(it->docIndex != n->first) { ++it; continue; }
				const i32 take = MIN(left, it->count);
				it->count -= take;
				left -= take;
				if(!r->items.full()) {
					AccountItem sent = *it;
					if(sent.count <= 0) { sent = AccountItem(); sent.uid = it->uid; }
					r->items.push_back(sent);
				}
				if(it->count <= 0) it = inventory.items.erase(it);
				else ++it;
			}
		}
		AccountInventorySave(account.nickname.data(), inventory);
		r->itemReason = MEASURED_ITEM_UPDATE_REASON_WEAPON_UPGRADE;
	}
	if(AccountArsenal::Weapon* state = WeaponState(arsenal, cls, r->type)) {
		state->grade = (u8)(grade + 1);
		if(r->type == 1) state->unlocked = 1;
	}
	AccountArsenalSave(account.nickname.data(), arsenal);
	r->weaponIndex = next;
	r->grade = (i32)grade + 1;
	r->result = ARSENAL_RESULT_OK;
	r->achieve.sub = GameXmlContent::AchieveSub::COMMON_WEAPON_UPGRADE_COUNT;
	r->achieve.object = next;
	r->achieve.classType = cls;
	LOG("Arsenal :: %ls upgrades weapon type %u of class %d to grade %u (%d)", account.nickname.data(), r->type, cls, grade + 1, next);
}

static void SkillUnlock(Account& account, const GameXmlContent::Master& master, AccountArsenal& arsenal, u8 order, ArsenalResult* r)
{
	for(i32 i = 0; i < (i32)master.skillIDs.size() && i < (i32)master.skillUnlockOrder.size(); i++) {
		if(master.skillUnlockOrder[i] == order && !master.skillUnlocked[i]) r->skill = (i32)master.skillIDs[i];
	}
	if(r->skill < 0 || order >= GetGameXmlContent().skillUnlockCost.size()) return;
	if(!arsenal.SkillUnlocked((i32)master.classType, r->skill)) {
		const GameXmlContent::PriceCost& cost = GetGameXmlContent().skillUnlockCost[order];
		if(!ArsenalSpend(account, cost.buyType, cost.amount, MEASURED_MONEY_REASON_SKILL_UNLOCK, r)) {
			r->result = HYP_ARSENAL_RESULT_NO_MONEY;
			return;
		}
		if(!arsenal.skills.full()) arsenal.skills.push_back(eastl::make_pair((i32)master.classType, r->skill));
		AccountArsenalSave(account.nickname.data(), arsenal);
		LOG("Arsenal :: %ls unlocks skill %d (order %u)", account.nickname.data(), r->skill, (u32)order);
	}
	r->result = ARSENAL_RESULT_OK;
}

static void SkillEquip(Account& account, const GameXmlContent::Master& master, AccountArsenal& arsenal, ArsenalResult* r)
{
	i32 order = -1;
	const bool available = SkillAvailable(arsenal, master, r->skill, &order);
	if(r->skillSlot > 1 || order < 0) return;
	if(!available) {
		const i32 skill = r->skill;
		SkillUnlock(account, master, arsenal, (u8)order, r);
		const bool unlocked = r->result == ARSENAL_RESULT_OK && r->skill == skill;
		r->skill = skill;
		if(!unlocked) { r->result = HYP_ARSENAL_RESULT_NO_MONEY; return; }
		r->unlockedOnEquip = true;
	}
	SkillID current[2];
	ArsenalSkills(arsenal, master, current);
	if((i32)current[r->skillSlot ^ 1] == r->skill) current[r->skillSlot ^ 1] = current[r->skillSlot];
	current[r->skillSlot] = (SkillID)r->skill;
	if(AccountArsenal::SkillLoadout* l = arsenal.Loadout((i32)master.classType)) {
		l->skills[0] = (i32)current[0];
		l->skills[1] = (i32)current[1];
	}
	AccountArsenalSave(account.nickname.data(), arsenal);
	r->result = ARSENAL_RESULT_OK;
	LOG("Arsenal :: %ls equips skill %d in slot %u of class %d", account.nickname.data(), r->skill, r->skillSlot, (i32)master.classType);
}

static void SkillUpgradeStage(Account& account, const GameXmlContent::Master& master, AccountArsenal& arsenal, ArsenalResult* r)
{
	bool owned = false;
	foreach_const(s, master.skillIDs) if((i32)*s == r->skill) owned = true;
	const GameXmlContent::SkillUpgradeList* ups = GetGameXmlContent().FindSkillUpgrades(r->skill);
	const i32 level = arsenal.SkillLevel(r->skill);
	r->grade = level;
	const auto& costs = GetGameXmlContent().pveSkillUpgradeCost;
	if(!owned || !ups || level >= (i32)ups->size() || level >= (i32)costs.size() || (IsLockedSlot(master, r->skill) && !arsenal.SkillUnlocked((i32)master.classType, r->skill))) return;
	if(!ArsenalSpend(account, costs[level].buyType, costs[level].amount, HYP_MONEY_REASON_SKILL_UPGRADE, r)) {
		r->result = HYP_ARSENAL_RESULT_NO_MONEY;
		return;
	}
	arsenal.SetSkillLevel(r->skill, level + 1);
	AccountArsenalSave(account.nickname.data(), arsenal);
	r->grade = level + 1;
	r->result = ARSENAL_RESULT_OK;
	r->achieve.sub = GameXmlContent::AchieveSub::COMMON_SKILL_UPGRADE_COUNT;
	r->achieve.object = r->skill;
	r->achieve.value = level + 1;
	r->achieve.absolute = true;
	LOG("Arsenal :: %ls upgrades stage skill %d to level %d (%d)", account.nickname.data(), r->skill, level + 1, costs[level].amount);
}

bool ArsenalHandle(Account& account, u16 netID, const u8* packetData, i32 packetSize, ArsenalResult* out)
{
	if(!IsArsenalPacket(netID)) return false;
	ArsenalResult& r = *out;
	r = ArsenalResult();
	r.answer = netID;
	r.result = HYP_ARSENAL_RESULT_FAILED;
	ConstBuffer buff(packetData, packetSize);
	if(packetSize < 5) return true;
	r.characterID = buff.Read<u32>();
	const GameXmlContent::Master* master = MasterOfCharacter(r.characterID);
	if(!master) return true;
	AccountArsenal arsenal = AccountArsenalLoad(account.nickname.data());
	if(netID == MEASURED_CQ_SKILL_UNLOCK) {
		SkillUnlock(account, *master, arsenal, buff.Read<u8>(), &r);
		return true;
	}
	if(netID == MEASURED_CQ_SKILL_UPGRADE) {
		if(packetSize < 8) return true;
		r.skill = buff.Read<i32>();
		SkillUpgradeStage(account, *master, arsenal, &r);
		return true;
	}
	if(netID == MEASURED_CQ_SKILL_EQUIP) {
		if(packetSize < 12) return true;
		r.skill = buff.Read<i32>();
		r.skillSlot = buff.Read<u32>();
		SkillEquip(account, *master, arsenal, &r);
		return true;
	}
	if(packetSize < 8) return true;
	r.type = buff.Read<u32>();
	if(r.type < 1) return true;
	if(netID == MEASURED_CQ_WEAPON_UNLOCK) WeaponUnlock(account, *master, arsenal, &r);
	else if(netID == MEASURED_CQ_WEAPON_EQUIP) WeaponEquip(account, *master, arsenal, &r);
	else WeaponUpgrade(account, *master, arsenal, &r);
	return true;
}

void HubGame::OnSelectSupportKit(ClientHandle clientHd, u8 etcType, i32 doc)
{
	static const u32 HYP_SELECT_KIT_FAILED = 1;
	Account* account = GearAccount(clientHd);
	if(!account) return;
	bool owned = doc <= 0;
	if(!owned && GetGameXmlContent().FindSupportKit(doc)) {
		const AccountInventory inventory = AccountInventoryLoad(account->nickname.data());
		foreach_const(it, inventory.items) if(it->docIndex == doc && it->count > 0) owned = true;
	}
	PacketWriter<Sv::SA_SelectAccountEquipment,16> answer;
	answer.Write<u32>(owned ? 0 : HYP_SELECT_KIT_FAILED);
	answer.Write<u8>(etcType);
	answer.Write<i32>(doc);
	replication.SendWriter(clientHd, answer);
	if(!owned) return;
	AccountArsenal arsenal = AccountArsenalLoad(account->nickname.data());
	arsenal.supportKit = doc > 0 ? doc : -1;
	AccountArsenalSave(account->nickname.data(), arsenal);
	PacketWriter<Sv::SN_AccountEquipmentList,16> list;
	list.Write<i32>(arsenal.supportKit);
	replication.SendWriter(clientHd, list);
	LOG("[client%x] Arsenal :: %ls support kit %d", clientHd, account->nickname.data(), arsenal.supportKit);
}

void HubGame::OnArsenal(ClientHandle clientHd, u16 netID, const u8* packetData, i32 packetSize)
{
	Account* account = GearAccount(clientHd);
	if(!account) return;
	ArsenalResult r;
	if(!ArsenalHandle(*account, netID, packetData, packetSize, &r)) return;
	auto send = [&](const auto& writer) { replication.SendWriter(clientHd, writer); };
	ArsenalSend(r, send);
	if(r.achieve.sub != GameXmlContent::AchieveSub::NONE) AchieveRaise(account->nickname.data(), r.achieve, send);
}
