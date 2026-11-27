#include "game_store.h"
#include <time.h>

ProfileMaster* PlayerProfile::FindMaster(ClassType classType)
{
	foreach(it, masters) {
		if(it->classType == classType) return &*it;
	}
	return nullptr;
}

const ProfileMaster* PlayerProfile::FindMaster(ClassType classType) const
{
	foreach_const(it, masters) {
		if(it->classType == classType) return &*it;
	}
	return nullptr;
}

bool PlayerProfile::OwnsSkin(ClassType classType, SkinIndex skinIndex) const
{
	foreach_const(it, skins) {
		if(it->classType == classType && it->skinIndex == skinIndex) return true;
	}
	return false;
}

SettingsSaveCheck PlayerProfile::CheckSettingsSave(i32 settingType, i32 size)
{
	if(settingType < 0 || settingType >= PROFILE_SETTINGS_SLOTS) return SettingsSaveCheck::BAD_SLOT;
	if(size <= 0) return SettingsSaveCheck::EMPTY;
	if(size > PROFILE_SETTINGS_MAX_SIZE) return SettingsSaveCheck::TOO_LARGE;
	return SettingsSaveCheck::OK;
}

void PlayerProfile::SetSettings(i32 settingType, const u8* data, i32 size)
{
	ASSERT(CheckSettingsSave(settingType, size) == SettingsSaveCheck::OK);
	ProfileSettings& slot = settings[settingType];
	slot.present = true;
	slot.data.assign(data, data + size);
}

ErrorType PlayerProfile::CheckStatusMessage(i32 len, i64 now) const
{
	if(len > STATUS_MESSAGE_MAX_LEN) return ErrorType::STATUS_MESSAGE_TOO_LONG;
	if(statusMessageAt != 0 && now - statusMessageAt < STATUS_MESSAGE_INTERVAL_SEC) return ErrorType::STATUS_MESSAGE_TOO_OFTEN;
	return ErrorType::SUCCESS;
}

void PlayerProfile::SetStatusMessage(const wchar* msg, i32 len, i64 now)
{
	ASSERT(len >= 0 && len <= STATUS_MESSAGE_MAX_LEN);
	statusMessage.assign(msg, len);
	statusMessageAt = now;
}

void ItemUIDAllocator::Reset(u32 lastSeq)
{
	ASSERT(lastSeq <= ITEM_UID_MAX_SEQ);
	seq.SetValue(lastSeq);
}

ItemUID ItemUIDAllocator::Next()
{
	const u32 next = seq.Increment();
	ASSERT_MSG(next <= ITEM_UID_MAX_SEQ, "item uid space exhausted");
	return (ItemUID)(ITEM_UID_TAG | next);
}

u32 ItemUIDAllocator::LastSeq() const
{
	return seq.GetValue();
}

bool ItemUIDAllocator::IsItemUID(ItemUID uid)
{
	return ((u32)uid & 0xC0000000) == ITEM_UID_TAG && ((u32)uid & ITEM_UID_MAX_SEQ) != 0;
}

u32 ItemUIDAllocator::Seq(ItemUID uid)
{
	ASSERT(IsItemUID(uid));
	return (u32)uid & ITEM_UID_MAX_SEQ;
}

i32 InventorySlot(i32 tab, i32 index)
{
	ASSERT(tab >= 0 && tab < INVENTORY_TABS);
	ASSERT(index >= 0 && index < INVENTORY_MAX_LINES * INVENTORY_SLOTS_PER_LINE);
	return (tab + 1) * INVENTORY_TAB_SLOT_BASE + index;
}

i32 ItemContainer(InvenType invenType, i32 slot)
{
	if(slot < 0) return -1;
	if(invenType == InvenType::WAREHOUSE) {
		return slot < WAREHOUSE_MAX_LINES * WAREHOUSE_SLOTS_PER_LINE ? CONTAINER_WAREHOUSE : -1;
	}
	if(invenType != InvenType::INVENTORY) return -1;
	const i32 tab = slot / INVENTORY_TAB_SLOT_BASE - 1;
	const i32 index = slot % INVENTORY_TAB_SLOT_BASE;
	if(tab < 0 || tab >= INVENTORY_TABS) return -1;
	if(index >= INVENTORY_MAX_LINES * INVENTORY_SLOTS_PER_LINE) return -1;
	return tab;
}

i32 ContainerSlotIndex(InvenType invenType, i32 slot)
{
	ASSERT(ItemContainer(invenType, slot) >= 0);
	if(invenType == InvenType::WAREHOUSE) return slot;
	return slot % INVENTORY_TAB_SLOT_BASE;
}

i32 ContainerSlotsPerLine(i32 container)
{
	ASSERT(container >= 0 && container < INVENTORY_CONTAINERS);
	return container == CONTAINER_WAREHOUSE ? WAREHOUSE_SLOTS_PER_LINE : INVENTORY_SLOTS_PER_LINE;
}

i32 ContainerMaxLines(i32 container)
{
	ASSERT(container >= 0 && container < INVENTORY_CONTAINERS);
	return container == CONTAINER_WAREHOUSE ? WAREHOUSE_MAX_LINES : INVENTORY_MAX_LINES;
}

i32 ContainerSlot(i32 container, i32 index, InvenType* outInvenType)
{
	ASSERT(container >= 0 && container < INVENTORY_CONTAINERS);
	if(container == CONTAINER_WAREHOUSE) {
		ASSERT(index >= 0 && index < WAREHOUSE_MAX_LINES * WAREHOUSE_SLOTS_PER_LINE);
		*outInvenType = InvenType::WAREHOUSE;
		return index;
	}
	*outInvenType = InvenType::INVENTORY;
	return InventorySlot(container, index);
}

i64 PlayerProfile::Balance(Currency currency) const
{
	switch(currency) {
		case Currency::GOLD: return gold;
		case Currency::CCOIN: return ccoin;
	}
	ASSERT(0);
	return 0;
}

bool PlayerProfile::CanGrant(Currency currency, i64 amount) const
{
	if(amount < 0 || amount > CURRENCY_MAX) return false;
	return Balance(currency) <= CURRENCY_MAX - amount;
}

bool PlayerProfile::CanSpend(Currency currency, i64 amount) const
{
	if(amount < 0) return false;
	return Balance(currency) >= amount;
}

void PlayerProfile::Grant(Currency currency, i64 amount)
{
	ASSERT(CanGrant(currency, amount));
	if(currency == Currency::GOLD) gold += amount;
	else ccoin += amount;
}

void PlayerProfile::Spend(Currency currency, i64 amount)
{
	ASSERT(CanSpend(currency, amount));
	if(currency == Currency::GOLD) gold -= amount;
	else ccoin -= amount;
}

ProfileItem* PlayerProfile::FindItem(ItemUID uid)
{
	foreach(it, items) {
		if(it->uid == uid) return &*it;
	}
	return nullptr;
}

const ProfileItem* PlayerProfile::FindItem(ItemUID uid) const
{
	foreach_const(it, items) {
		if(it->uid == uid) return &*it;
	}
	return nullptr;
}

const ProfileItem* PlayerProfile::ItemAt(InvenType invenType, i32 slot) const
{
	foreach_const(it, items) {
		if(it->invenType == invenType && it->slot == slot) return &*it;
	}
	return nullptr;
}

bool PlayerProfile::SlotUsable(InvenType invenType, i32 slot) const
{
	const i32 container = ItemContainer(invenType, slot);
	if(container < 0) return false;
	return ContainerSlotIndex(invenType, slot) < lines[container] * ContainerSlotsPerLine(container);
}

i32 PlayerProfile::FreeSlotCount(i32 container) const
{
	ASSERT(container >= 0 && container < INVENTORY_CONTAINERS);
	i32 used = 0;
	foreach_const(it, items) {
		if(ItemContainer(it->invenType, it->slot) == container) used++;
	}
	const i32 total = lines[container] * ContainerSlotsPerLine(container);
	return used >= total ? 0 : total - used;
}

bool PlayerProfile::FindFreeSlot(i32 container, InvenType* outInvenType, i32* outSlot) const
{
	ASSERT(container >= 0 && container < INVENTORY_CONTAINERS);
	const i32 total = lines[container] * ContainerSlotsPerLine(container);
	for(i32 i = 0; i < total; i++) {
		InvenType invenType;
		const i32 slot = ContainerSlot(container, i, &invenType);
		if(!ItemAt(invenType, slot)) {
			*outInvenType = invenType;
			*outSlot = slot;
			return true;
		}
	}
	return false;
}

ItemCheck PlayerProfile::CheckAddItem(const ProfileItem& item) const
{
	if(!ItemUIDAllocator::IsItemUID(item.uid)) return ItemCheck::BAD_UID;
	if(FindItem(item.uid)) return ItemCheck::UID_TAKEN;
	if(item.docIndex == ItemDocIndex::INVALID) return ItemCheck::BAD_DOC;
	if(item.count <= 0) return ItemCheck::BAD_COUNT;
	if(!SlotUsable(item.invenType, item.slot)) return ItemCheck::BAD_SLOT;
	if(ItemAt(item.invenType, item.slot)) return ItemCheck::SLOT_TAKEN;
	return ItemCheck::OK;
}

ItemCheck PlayerProfile::CheckRemoveItem(ItemUID uid, i32 count) const
{
	const ProfileItem* item = FindItem(uid);
	if(!item) return ItemCheck::NOT_FOUND;
	if(count <= 0 || count > item->count) return ItemCheck::BAD_COUNT;
	return ItemCheck::OK;
}

ItemCheck PlayerProfile::CheckMoveItem(ItemUID uid, InvenType invenType, i32 slot) const
{
	const ProfileItem* item = FindItem(uid);
	if(!item) return ItemCheck::NOT_FOUND;
	if(!SlotUsable(invenType, slot)) return ItemCheck::BAD_SLOT;
	const ProfileItem* occupant = ItemAt(invenType, slot);
	if(occupant && occupant != item) return ItemCheck::SLOT_TAKEN;
	return ItemCheck::OK;
}

void PlayerProfile::AddItem(const ProfileItem& item)
{
	ASSERT(CheckAddItem(item) == ItemCheck::OK);
	items.push_back(item);
}

void PlayerProfile::RemoveItem(ItemUID uid, i32 count)
{
	ASSERT(CheckRemoveItem(uid, count) == ItemCheck::OK);
	foreach(it, items) {
		if(it->uid != uid) continue;
		if(it->count == count) items.erase(it);
		else it->count -= count;
		return;
	}
}

void PlayerProfile::MoveItem(ItemUID uid, InvenType invenType, i32 slot)
{
	ASSERT(CheckMoveItem(uid, invenType, slot) == ItemCheck::OK);
	ProfileItem* item = FindItem(uid);
	item->invenType = invenType;
	item->slot = slot;
}

bool PlaceSeedItems(const ProfileSeed& seed, ItemUIDAllocator& uids, PlayerProfile* profile, eastl::vector<ProfileItem>* placed)
{
	placed->clear();
	foreach_const(it, seed.items) {
		ProfileItem item;
		if(!profile->FindFreeSlot(it->container, &item.invenType, &item.slot)) return false;
		item.uid = uids.Next();
		item.docIndex = it->docIndex;
		item.count = it->count;
		item.propertyGroup = it->propertyGroup;
		if(profile->CheckAddItem(item) != ItemCheck::OK) return false;
		profile->AddItem(item);
		placed->push_back(item);
	}
	return true;
}

void SeedProfileInMemory(const ProfileSeed& seed, ItemUIDAllocator& uids, PlayerProfile* profile)
{
	ASSERT(!seed.masters.empty());
	profile->masters.clear();
	profile->skins.clear();
	foreach_const(it, seed.masters) {
		ProfileMaster master;
		master.classType = *it;
		profile->masters.push_back(master);
	}
	foreach_const(it, seed.skins) {
		ProfileSkin skin;
		skin.classType = it->classType;
		skin.skinIndex = it->skinIndex;
		profile->skins.push_back(skin);
	}
	profile->leaderClass = seed.leaderClass;
	profile->leaderSkin = SkinIndex::DEFAULT;

	profile->gold = seed.gold;
	profile->ccoin = seed.ccoin;
	profile->lines = seed.lines;
	profile->items.clear();
	eastl::vector<ProfileItem> placed;
	const bool fits = PlaceSeedItems(seed, uids, profile, &placed);
	ASSERT_MSG(fits, "seed items do not fit the seed inventory");
}

bool GameStore::Start(GameStoreBackend* backend_, const char* path, const GameStoreOptions& options_)
{
	ASSERT(!running);
	ASSERT(backend_);

	if(!backend_->Open(path)) return false;

	backend = backend_;
	options = options_;
	stopping = false;
	failedJobs = 0;
	running = true;
	thread.Begin(ThreadStorage, this);
	return true;
}

void GameStore::Shutdown()
{
	if(!running) return;

	{
		LOCK_MUTEX(mutexQueue);
		ASSERT(!stopping);
		stopping = true;
	}
	wakeUp.Post();
	thread.WaitForEnd();

	ASSERT(queue.empty());
	backend->Close();
	backend = nullptr;
	running = false;
}

void GameStore::QueueWrite(const char* name, GameStoreWrite write)
{
	ASSERT(write);
	Job job;
	job.kind = JobKind::WRITE;
	job.name = name;
	job.write = eastl::move(write);
	Push(eastl::move(job));
}

void GameStore::QueueLoadProfile(u32 tag, AccountUID accountUID)
{
	ASSERT(accountUID != AccountUID::INVALID);
	Job job;
	job.kind = JobKind::LOAD_PROFILE;
	job.name = "LoadProfile";
	job.tag = tag;
	job.accountUID = accountUID;
	Push(eastl::move(job));
}

void GameStore::TransferLoaded(LoadedList* out)
{
	out->clear();
	LOCK_MUTEX(mutexLoaded);
	while(!loaded.empty() && out->size() < out->capacity()) {
		out->push_back(loaded.front());
		loaded.pop_front();
	}
}

ItemUID GameStore::AllocateItemUID()
{
	ASSERT(running);
	return backend->itemUIDs.Next();
}

ItemUIDAllocator& GameStore::ItemUIDs()
{
	ASSERT(running);
	return backend->itemUIDs;
}

i32 GameStore::FailedJobCount()
{
	LOCK_MUTEX(mutexLoaded);
	return failedJobs;
}

void GameStore::Push(Job&& job)
{
	ASSERT(running);
	{
		LOCK_MUTEX(mutexQueue);
		ASSERT(!stopping);
		queue.push_back(eastl::move(job));
	}
	wakeUp.Post();
}

intptr_t GameStore::ThreadStorage(void* pData)
{
	ProfileSetThreadName("Storage");
	((GameStore*)pData)->Run();
	return 0;
}

void GameStore::Run()
{
	for(;;) {
		const i32 woken = wakeUp.Wait();
		ASSERT(woken >= 0);

		Job job;
		{
			LOCK_MUTEX(mutexQueue);
			if(queue.empty()) {
				if(stopping) break;
				continue;
			}
			job = eastl::move(queue.front());
			queue.pop_front();
		}

		RunJob(job);
	}

	LOG("GameStore :: storage thread drained its queue and stops");
}

void GameStore::RunJob(const Job& job)
{
	ProfileFunction();

	const i64 now = (i64)time(nullptr);

	bool ok = backend->Begin();
	PlayerProfile profile;
	if(ok) {
		switch(job.kind) {
			case JobKind::WRITE: ok = job.write(*backend); break;
			case JobKind::LOAD_PROFILE: ok = backend->LoadProfile(job.accountUID, now, options.seed, &profile); break;
		}

		if(ok) ok = backend->Commit();
		if(!ok) backend->Rollback();
	}

	if(!ok) {
		LOG("ERROR(GameStore): job '%s' failed and was rolled back (account=%u)", job.name, (u32)job.accountUID);
		{
			LOCK_MUTEX(mutexLoaded);
			failedJobs++;
		}
		if(options.assertOnFailure) ASSERT_MSG(0, "game store job failed");
		return;
	}

	if(job.kind == JobKind::LOAD_PROFILE) {
		LOCK_MUTEX(mutexLoaded);
		loaded.push_back(ProfileLoaded{ job.tag, profile });
	}
}
