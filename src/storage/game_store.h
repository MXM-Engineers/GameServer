#pragma once
#include <common/base.h>
#include <common/inner_protocol.h>
#include <common/protocol.h>
#include <common/utils.h>
#include <eathread/eathread_thread.h>
#include <eathread/eathread_semaphore.h>
#include <eathread/eathread_atomic.h>
#include <EASTL/functional.h>
#include <EASTL/deque.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/vector.h>
#include <EASTL/array.h>

struct ProfileMaster
{
	ClassType classType = ClassType::NONE;
	SkinIndex skinIndex = SkinIndex::DEFAULT;
};

struct ProfileSkin
{
	ClassType classType = ClassType::NONE;
	SkinIndex skinIndex = SkinIndex::DEFAULT;
	i64 expireAt = 0;
};

enum {
	PROFILE_SETTINGS_SLOTS = 4,
	PROFILE_SETTINGS_MAX_SIZE = 6144,
	STATUS_MESSAGE_MAX_LEN = 64,
	TUTORIAL_STATE_TOWN = -1
};

const i64 STATUS_MESSAGE_INTERVAL_SEC = 60;

enum class SettingsSaveCheck: u8
{
	OK = 0,
	BAD_SLOT,
	EMPTY,
	TOO_LARGE
};

struct ProfileSettings
{
	bool present = false;
	eastl::vector<u8> data;
};

enum class Currency: u8
{
	GOLD = 0,
	CCOIN = 1
};

enum class LedgerAsset: u8
{
	GOLD = 0,
	CCOIN = 1,
	ITEM = 2
};

enum {
	INVENTORY_TABS = 3,
	INVENTORY_CONTAINERS = 4,
	CONTAINER_WAREHOUSE = 3,
	INVENTORY_TAB_SLOT_BASE = 100,
	INVENTORY_SLOTS_PER_LINE = 6,
	INVENTORY_MAX_LINES = 16,
	WAREHOUSE_SLOTS_PER_LINE = 6,
	WAREHOUSE_MAX_LINES = 9,
	ITEM_UID_TAG = 0x40000000,
	ITEM_UID_MAX_SEQ = 0x3FFFFFFF
};

const i64 CURRENCY_MAX = 999999999999ll;

struct ItemProperty
{
	u8 type = 0;
	i32 typeDetail = 0;
	u8 valueType = 0;
	f32 value = 0;
	u8 fixed = 0;
};

struct ProfileItem
{
	ItemUID uid = ItemUID::INVALID;
	InvenType invenType = InvenType::INVENTORY;
	i32 slot = 0;
	ItemDocIndex docIndex = ItemDocIndex::INVALID;
	i32 count = 0;
	i32 propertyGroup = -1;
	u8 lifeAbsolute = 0;
	i64 lifeEndUTC = 0;
	eastl::vector<ItemProperty> properties;
};

enum class ItemCheck: u8
{
	OK = 0,
	NOT_FOUND,
	BAD_UID,
	UID_TAKEN,
	BAD_DOC,
	BAD_COUNT,
	BAD_SLOT,
	SLOT_TAKEN,
	NO_FREE_SLOT
};

struct LedgerNote
{
	AssetChangeReason reason = AssetChangeReason::UNKNOWN;
	ItemDocIndex docIndex = ItemDocIndex::INVALID;
	i32 quantity = 0;
	i64 at = 0;
};

struct ItemUIDAllocator
{
	void Reset(u32 lastSeq);
	ItemUID Next();
	u32 LastSeq() const;

	static bool IsItemUID(ItemUID uid);
	static u32 Seq(ItemUID uid);

private:
	EA::Thread::AtomicUint32 seq{0};
};

i32 InventorySlot(i32 tab, i32 index);
i32 ItemContainer(InvenType invenType, i32 slot);
i32 ContainerSlotIndex(InvenType invenType, i32 slot);
i32 ContainerSlotsPerLine(i32 container);
i32 ContainerMaxLines(i32 container);
i32 ContainerSlot(i32 container, i32 index, InvenType* outInvenType);

struct PlayerProfile
{
	AccountUID accountUID = AccountUID::INVALID;
	i64 createdAt = 0;
	i64 lastLoginAt = 0;
	bool firstEntry = false;
	ClassType leaderClass = ClassType::NONE;
	SkinIndex leaderSkin = SkinIndex::DEFAULT;
	eastl::vector<ProfileMaster> masters;
	eastl::vector<ProfileSkin> skins;
	eastl::array<ProfileSettings,PROFILE_SETTINGS_SLOTS> settings;
	i32 tutorialState = TUTORIAL_STATE_TOWN;
	WideString statusMessage;
	i64 statusMessageAt = 0;
	i64 lastLogoutAt = 0;
	i64 gold = 0;
	i64 ccoin = 0;
	eastl::array<i32,INVENTORY_CONTAINERS> lines = {{ 4, 4, 4, 1 }};
	eastl::vector<ProfileItem> items;

	ProfileMaster* FindMaster(ClassType classType);
	const ProfileMaster* FindMaster(ClassType classType) const;
	bool OwnsSkin(ClassType classType, SkinIndex skinIndex) const;

	static SettingsSaveCheck CheckSettingsSave(i32 settingType, i32 size);
	void SetSettings(i32 settingType, const u8* data, i32 size);
	ErrorType CheckStatusMessage(i32 len, i64 now) const;
	void SetStatusMessage(const wchar* msg, i32 len, i64 now);

	i64 Balance(Currency currency) const;
	bool CanGrant(Currency currency, i64 amount) const;
	bool CanSpend(Currency currency, i64 amount) const;
	void Grant(Currency currency, i64 amount);
	void Spend(Currency currency, i64 amount);

	ProfileItem* FindItem(ItemUID uid);
	const ProfileItem* FindItem(ItemUID uid) const;
	const ProfileItem* ItemAt(InvenType invenType, i32 slot) const;
	bool SlotUsable(InvenType invenType, i32 slot) const;
	i32 FreeSlotCount(i32 container) const;
	bool FindFreeSlot(i32 container, InvenType* outInvenType, i32* outSlot) const;

	ItemCheck CheckAddItem(const ProfileItem& item) const;
	ItemCheck CheckRemoveItem(ItemUID uid, i32 count) const;
	ItemCheck CheckMoveItem(ItemUID uid, InvenType invenType, i32 slot) const;
	void AddItem(const ProfileItem& item);
	void RemoveItem(ItemUID uid, i32 count);
	void MoveItem(ItemUID uid, InvenType invenType, i32 slot);
};

struct ProfileSeed
{
	struct Skin
	{
		ClassType classType;
		SkinIndex skinIndex;
	};

	bool dev = false;
	ClassType leaderClass = ClassType::NONE;
	eastl::vector<ClassType> masters;
	eastl::vector<Skin> skins;

	struct Item
	{
		ItemDocIndex docIndex;
		i32 count;
		i32 propertyGroup;
		i32 container;
	};

	i64 gold = 0;
	i64 ccoin = 0;
	eastl::array<i32,INVENTORY_CONTAINERS> lines = {{ 4, 4, 4, 1 }};
	eastl::vector<Item> items;
};

bool PlaceSeedItems(const ProfileSeed& seed, ItemUIDAllocator& uids, PlayerProfile* profile, eastl::vector<ProfileItem>* placed);
void SeedProfileInMemory(const ProfileSeed& seed, ItemUIDAllocator& uids, PlayerProfile* profile);

struct GameStoreBackend
{
	virtual ~GameStoreBackend() {}

	virtual bool Open(const char* path) = 0;
	virtual void Close() = 0;

	virtual bool Begin() = 0;
	virtual bool Commit() = 0;
	virtual void Rollback() = 0;

	virtual bool LoadProfile(AccountUID accountUID, i64 now, const ProfileSeed& seed, PlayerProfile* out) = 0;
	virtual bool SetLeader(AccountUID accountUID, ClassType classType, SkinIndex skinIndex) = 0;
	virtual bool SaveSettings(AccountUID accountUID, i32 settingType, const u8* data, i32 size, i64 now) = 0;
	virtual bool SetStatusMessage(AccountUID accountUID, const wchar* msg, i32 len, i64 at) = 0;
	virtual bool SetLastLogout(AccountUID accountUID, i64 at) = 0;

	virtual bool GrantCurrency(AccountUID accountUID, Currency currency, i64 amount, const LedgerNote& note) = 0;
	virtual bool SpendCurrency(AccountUID accountUID, Currency currency, i64 amount, const LedgerNote& note) = 0;
	virtual bool AddItem(AccountUID accountUID, const ProfileItem& item, const LedgerNote& note) = 0;
	virtual bool RemoveItem(AccountUID accountUID, ItemUID uid, i32 count, const LedgerNote& note) = 0;
	virtual bool MoveItem(AccountUID accountUID, ItemUID uid, InvenType invenType, i32 slot, const LedgerNote& note) = 0;
	virtual bool SetInventoryLines(AccountUID accountUID, i32 container, i32 lines) = 0;

	ItemUIDAllocator itemUIDs;
};

struct GameStoreOptions
{
	ProfileSeed seed;
	bool assertOnFailure = true;
};

struct ProfileLoaded
{
	u32 tag;
	PlayerProfile profile;
};

typedef eastl::function<bool(GameStoreBackend&)> GameStoreWrite;

struct GameStore
{
	enum {
		MAX_LOADED_PER_TRANSFER = 128
	};

	typedef eastl::fixed_vector<ProfileLoaded,MAX_LOADED_PER_TRANSFER,false> LoadedList;

	bool Start(GameStoreBackend* backend_, const char* path, const GameStoreOptions& options_);
	void Shutdown();

	void QueueWrite(const char* name, GameStoreWrite write);
	void QueueLoadProfile(u32 tag, AccountUID accountUID);
	void TransferLoaded(LoadedList* out);

	i32 FailedJobCount();
	ItemUID AllocateItemUID();
	ItemUIDAllocator& ItemUIDs();

private:
	enum class JobKind: u8
	{
		WRITE = 0,
		LOAD_PROFILE
	};

	struct Job
	{
		JobKind kind = JobKind::WRITE;
		const char* name = "";
		u32 tag = 0;
		AccountUID accountUID = AccountUID::INVALID;
		GameStoreWrite write;
	};

	GameStoreBackend* backend = nullptr;
	GameStoreOptions options;
	EA::Thread::Thread thread;
	EA::Thread::Semaphore wakeUp{0};
	bool running = false;

	Mutex mutexQueue;
	eastl::deque<Job> queue;
	bool stopping = false;

	Mutex mutexLoaded;
	eastl::deque<ProfileLoaded> loaded;
	i32 failedJobs = 0;

	static intptr_t ThreadStorage(void* pData);
	void Run();
	void RunJob(const Job& job);
	void Push(Job&& job);
};
