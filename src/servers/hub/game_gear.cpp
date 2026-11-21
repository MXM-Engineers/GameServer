#include "game.h"
#include "coordinator.h"
#include <mxm/game_content.h>
#include <mxm/account_progress.h>

static const u32 MEASURED_ITEM_UPDATE_REASON_GEAR = 208;
static const u32 GEAR_RESULT_OK = 0;
static const u32 HYP_GEAR_RESULT_FAILED = 1;

Account* HubGame::GearAccount(ClientHandle clientHd)
{
	const i32 userID = plidMap->Get(clientHd);
	if(userID < 0 || !playerAccountData[userID]) return nullptr;
	auto found = GetAccountManager().accountMap.find(playerAccountData[userID]->UID);
	return found == GetAccountManager().accountMap.end() ? nullptr : &*found->second;
}

const wchar* HubGame::AccountNickname(ClientHandle clientHd)
{
	Account* account = GearAccount(clientHd);
	return account ? account->nickname.data() : L"";
}

static AccountItem* FindItem(AccountInventory& inventory, u32 uid)
{
	foreach(it, inventory.items) if(it->uid == uid) return &*it;
	return nullptr;
}

void HubGame::OnGearEquip(ClientHandle clientHd, u8 pageNo, u32 itemUID, u32 slot)
{
	Account* account = GearAccount(clientHd);
	if(!account) return;
	AccountGears gears = AccountGearsLoad(account->nickname.data());
	AccountInventory inventory = AccountInventoryLoad(account->nickname.data());
	AccountItem* item = FindItem(inventory, itemUID);
	const GameXmlContent::GearItem* gear = item ? GetGameXmlContent().FindGearItem(item->docIndex) : nullptr;
	if(!gear || pageNo < 1 || pageNo > gears.pages.size() || slot >= (u32)MEASURED_GEAR_SLOTS) {
		WARN("[client%x] Gear :: invalid equip page %d item %u slot %u", clientHd, (i32)pageNo, itemUID, slot);
		replication.SendGearEquip(clientHd, HYP_GEAR_RESULT_FAILED, pageNo, itemUID, slot);
		return;
	}
	foreach(p, gears.pages) for(u32& uid : p->items) if(uid == itemUID) uid = 0;
	AccountGearPage& page = gears.pages[pageNo - 1];
	eastl::fixed_vector<AccountItem,2,false> touched;
	if(AccountItem* previous = page.items[slot] != 0 ? FindItem(inventory, page.items[slot]) : nullptr) {
		if(previous != item) {
			previous->invenType = item->invenType;
			previous->slot = item->slot;
			touched.push_back(*previous);
		}
	}
	page.items[slot] = itemUID;
	item->invenType = MEASURED_GEAR_INVEN_TYPE;
	item->slot = (i32)slot;
	touched.push_back(*item);
	AccountInventorySave(account->nickname.data(), inventory);
	AccountGearsSave(account->nickname.data(), gears);
	replication.SendItemsUpdate(clientHd, touched.data(), (i32)touched.size(), MEASURED_ITEM_UPDATE_REASON_GEAR);
	replication.SendGearEquip(clientHd, GEAR_RESULT_OK, pageNo, itemUID, slot);
	LOG("[client%x] Gear :: %ls equips item %u (doc %d) on page %d slot %u", clientHd, account->nickname.data(), itemUID, item->docIndex, (i32)pageNo, slot);
}

void HubGame::OnGearUnequip(ClientHandle clientHd, u8 pageNo, u32 slot)
{
	Account* account = GearAccount(clientHd);
	if(!account) return;
	AccountGears gears = AccountGearsLoad(account->nickname.data());
	if(pageNo < 1 || pageNo > gears.pages.size() || slot >= (u32)MEASURED_GEAR_SLOTS || gears.pages[pageNo - 1].items[slot] == 0) {
		WARN("[client%x] Gear :: invalid unequip page %d slot %u", clientHd, (i32)pageNo, slot);
		replication.SendGearUnequip(clientHd, HYP_GEAR_RESULT_FAILED, pageNo, slot);
		return;
	}
	AccountInventory inventory = AccountInventoryLoad(account->nickname.data());
	const u32 uid = gears.pages[pageNo - 1].items[slot];
	gears.pages[pageNo - 1].items[slot] = 0;
	if(AccountItem* item = FindItem(inventory, uid)) {
		i32 free = MEASURED_INVENTORY_FIRST_SLOT;
		for(bool used = true; used; ) {
			used = false;
			foreach_const(it, inventory.items) if(it->invenType == 0 && it->slot == free) { used = true; free++; break; }
		}
		item->invenType = 0;
		item->slot = free;
		AccountInventorySave(account->nickname.data(), inventory);
		replication.SendItemsUpdate(clientHd, item, 1, MEASURED_ITEM_UPDATE_REASON_GEAR);
	}
	AccountGearsSave(account->nickname.data(), gears);
	replication.SendGearUnequip(clientHd, GEAR_RESULT_OK, pageNo, slot);
	LOG("[client%x] Gear :: %ls unequips page %d slot %u", clientHd, account->nickname.data(), (i32)pageNo, slot);
}

void HubGame::OnMasterGearAdd(ClientHandle clientHd)
{
	static const u32 HYP_GEAR_ADD_FAILED = 1;
	static const i32 HYP_MONEY_REASON_GEAR_ADD = 0;
	static const u8 BUY_TYPE_GOLD_PRICE = 1;
	Account* account = GearAccount(clientHd);
	if(!account) return;
	AccountGears gears = AccountGearsLoad(account->nickname.data());
	const auto& costs = GetGameXmlContent().masterGearAddCost;
	const i32 n = (i32)gears.pages.size();
	PacketWriter<Sv::SA_MastergearAdd,128> packet;
	if(gears.pages.full() || n >= (i32)costs.size()) {
		packet.Write<u32>(HYP_GEAR_ADD_FAILED);
		packet.Write<u8>(0);
		packet.WriteStringObj(L"");
		packet.Write<u16>(0);
		replication.SendWriter(clientHd, packet);
		return;
	}
	const GameXmlContent::PriceCost& cost = costs[n];
	AccountProgress progress = AccountProgressLoad(account->nickname.data());
	i64& wallet = cost.buyType == BUY_TYPE_GOLD_PRICE ? progress.gold : progress.ccoin;
	if(wallet < cost.amount) {
		packet.Write<u32>(HYP_GEAR_ADD_FAILED);
		packet.Write<u8>(0);
		packet.WriteStringObj(L"");
		packet.Write<u16>(0);
		replication.SendWriter(clientHd, packet);
		return;
	}
	wallet -= cost.amount;
	AccountProgressSave(account->nickname.data(), progress);
	account->gold = progress.gold;
	account->ccoin = progress.ccoin;
	if(cost.amount > 0) {
		if(cost.buyType == BUY_TYPE_GOLD_PRICE) replication.SendMoney(clientHd, progress.gold, HYP_MONEY_REASON_GEAR_ADD);
		else replication.SendCCoin(clientHd, progress.ccoin, (u32)HYP_MONEY_REASON_GEAR_ADD);
	}
	gears.pages.push_back(AccountGearPage());
	AccountGearsSave(account->nickname.data(), gears);
	packet.Write<u32>(0);
	packet.Write<u8>((u8)(n + 1));
	packet.WriteStringObj(L"");
	packet.Write<u16>((u16)MEASURED_GEAR_SLOTS);
	for(i32 s = 0; s < MEASURED_GEAR_SLOTS; s++) {
		packet.Write<i32>(-1);
		packet.Write<u32>(0);
	}
	replication.SendWriter(clientHd, packet);
	LOG("[client%x] Gear :: %ls buys page %d (%d)", clientHd, account->nickname.data(), n + 1, cost.amount);
}

void HubGame::OnMasterGearSelect(ClientHandle clientHd, u32 characterID, u8 pageNo)
{
	Account* account = GearAccount(clientHd);
	if(!account) return;
	AccountGears gears = AccountGearsLoad(account->nickname.data());
	const i32 classType = (i32)characterID - (i32)LocalActorID::FIRST_SELF_MASTER;
	if(pageNo < 1 || pageNo > gears.pages.size()) {
		replication.SendMasterGearSelect(clientHd, HYP_GEAR_RESULT_FAILED, characterID, pageNo);
		return;
	}
	bool found = false;
	foreach(s, gears.selected) if(s->first == classType) { s->second = pageNo; found = true; }
	if(!found && !gears.selected.full()) gears.selected.push_back(eastl::make_pair(classType, pageNo));
	AccountGearsSave(account->nickname.data(), gears);
	replication.SendMasterGearSelect(clientHd, GEAR_RESULT_OK, characterID, pageNo);
	LOG("[client%x] Gear :: %ls selects page %d for class %d", clientHd, account->nickname.data(), (i32)pageNo, classType);
}

void HubGame::OnMasterGearRename(ClientHandle clientHd, u8 pageNo, const wchar* name, i32 len)
{
	Account* account = GearAccount(clientHd);
	if(!account) return;
	AccountGears gears = AccountGearsLoad(account->nickname.data());
	if(pageNo < 1 || pageNo > gears.pages.size()) return;
	AccountGearPage& page = gears.pages[pageNo - 1];
	page.name.clear();
	for(i32 i = 0; i < len && !page.name.full(); i++) page.name.push_back(name[i]);
	AccountGearsSave(account->nickname.data(), gears);
	replication.SendMasterGearRename(clientHd, GEAR_RESULT_OK, pageNo, page.name.data());
	LOG("[client%x] Gear :: %ls renames page %d", clientHd, account->nickname.data(), (i32)pageNo);
}
