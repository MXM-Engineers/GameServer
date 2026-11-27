#include <common/base.h>
#include <storage/game_store.h>
#include <storage/game_db.h>
#include <EASTL/vector.h>
#include <stdio.h>
#include <string.h>

#define CHECK(cond) do { if(!(cond)) { LOG("FAILED: %s (line %d)", #cond, __LINE__); failures++; } } while(0)

static i32 failures = 0;

static GameStoreOptions TestOptions()
{
	GameStoreOptions options;
	options.assertOnFailure = false;
	return options;
}

static bool WaitLoaded(GameStore& store, ProfileLoaded* out)
{
	GameStore::LoadedList list;
	for(i32 i = 0; i < 5000; i++) {
		store.TransferLoaded(&list);
		if(!list.empty()) {
			ASSERT(list.size() == 1);
			*out = list[0];
			return true;
		}
		EA::Thread::ThreadSleep(1);
	}
	return false;
}

static bool CreatePlayer(GameStoreBackend& backend, AccountUID accountUID)
{
	PlayerProfile ignored;
	return backend.LoadProfile(accountUID, 1000, ProfileSeed(), &ignored);
}

static void RemoveDatabase(const char* path)
{
	remove(path);
	remove(FMT("%s-wal", path));
	remove(FMT("%s-shm", path));
}

static void TestJobsRunInOrder()
{
	SqliteGameStore backend;
	GameStore store;
	CHECK(store.Start(&backend, ":memory:", TestOptions()));

	eastl::vector<i32> order;
	for(i32 i = 0; i < 200; i++) {
		store.QueueWrite("Order", [&order, i](GameStoreBackend&) {
			order.push_back(i);
			return true;
		});
	}
	store.Shutdown();

	CHECK(order.size() == 200);
	for(i32 i = 0; i < (i32)order.size(); i++) {
		CHECK(order[i] == i);
	}
	CHECK(store.FailedJobCount() == 0);
}

static void TestLoadSeesEarlierWrites()
{
	SqliteGameStore backend;
	GameStore store;
	CHECK(store.Start(&backend, ":memory:", TestOptions()));

	store.QueueWrite("Slow", [](GameStoreBackend& b) {
		EA::Thread::ThreadSleep(50);
		return CreatePlayer(b, AccountUID(5));
	});
	store.QueueLoadProfile(42, AccountUID(5));

	ProfileLoaded loaded;
	CHECK(WaitLoaded(store, &loaded));
	CHECK(loaded.tag == 42);
	CHECK(loaded.profile.accountUID == AccountUID(5));
	CHECK(!loaded.profile.firstEntry);
	CHECK(loaded.profile.createdAt == 1000);
	CHECK(loaded.profile.lastLoginAt > 1000);

	store.QueueLoadProfile(43, AccountUID(6));
	CHECK(WaitLoaded(store, &loaded));
	CHECK(loaded.tag == 43);
	CHECK(loaded.profile.firstEntry);

	store.Shutdown();
}

static void TestFailedJobRollsBack()
{
	SqliteGameStore backend;
	GameStore store;
	CHECK(store.Start(&backend, ":memory:", TestOptions()));

	store.QueueWrite("Fails", [](GameStoreBackend& b) {
		const bool created = CreatePlayer(b, AccountUID(9));
		ASSERT(created);
		return false;
	});
	store.QueueLoadProfile(1, AccountUID(9));

	ProfileLoaded loaded;
	CHECK(WaitLoaded(store, &loaded));
	CHECK(loaded.profile.firstEntry);
	CHECK(loaded.profile.createdAt != 1000);
	CHECK(store.FailedJobCount() == 1);

	store.QueueWrite("AfterFailure", [](GameStoreBackend& b) {
		return CreatePlayer(b, AccountUID(10));
	});
	store.Shutdown();
	CHECK(store.FailedJobCount() == 1);
}

static void TestShutdownDrains()
{
	const char* path = "test_game_store.db";
	RemoveDatabase(path);

	{
		SqliteGameStore backend;
		GameStore store;
		CHECK(store.Start(&backend, path, TestOptions()));

		i32 ran = 0;
		store.QueueWrite("Slow", [&ran](GameStoreBackend&) {
			EA::Thread::ThreadSleep(50);
			ran++;
			return true;
		});
		for(i32 i = 1; i <= 50; i++) {
			store.QueueWrite("Create", [&ran, i](GameStoreBackend& b) {
				ran++;
				return CreatePlayer(b, AccountUID(i));
			});
		}
		store.QueueLoadProfile(7, AccountUID(50));
		store.Shutdown();

		CHECK(ran == 51);

		GameStore::LoadedList list;
		store.TransferLoaded(&list);
		CHECK(list.size() == 1);
		CHECK(list.size() == 1 && list[0].tag == 7 && !list[0].profile.firstEntry);
	}

	{
		SqliteGameStore reopened;
		CHECK(reopened.Open(path));
		for(i32 i = 1; i <= 50; i++) {
			PlayerProfile profile;
			CHECK(reopened.Begin());
			CHECK(reopened.LoadProfile(AccountUID(i), 3000, ProfileSeed(), &profile));
			CHECK(reopened.Commit());
			CHECK(!profile.firstEntry);
			CHECK(profile.createdAt == 1000);
		}
		reopened.Close();
	}

	RemoveDatabase(path);
}

static void TestRestartAfterShutdown()
{
	SqliteGameStore backend;
	GameStore store;
	CHECK(store.Start(&backend, ":memory:", TestOptions()));
	store.Shutdown();
	CHECK(store.Start(&backend, ":memory:", TestOptions()));
	store.QueueLoadProfile(3, AccountUID(3));
	store.Shutdown();

	GameStore::LoadedList list;
	store.TransferLoaded(&list);
	CHECK(list.size() == 1);
}

static ProfileSeed RetailSeed()
{
	ProfileSeed seed;
	seed.leaderClass = ClassType::LAUNCHER;
	seed.masters.push_back(ClassType::LAUNCHER);
	seed.masters.push_back(ClassType::ASSASSIN);
	seed.skins.push_back(ProfileSeed::Skin{ ClassType::LAUNCHER, SkinIndex::DEFAULT });
	seed.skins.push_back(ProfileSeed::Skin{ ClassType::ASSASSIN, SkinIndex::DEFAULT });
	seed.gold = 1000;
	seed.ccoin = 30;
	seed.items.push_back(ProfileSeed::Item{ (ItemDocIndex)132199998, 1, 723002202, 0 });
	seed.items.push_back(ProfileSeed::Item{ (ItemDocIndex)132199998, 1, 723002202, 0 });
	return seed;
}

static ProfileSeed DevSeed()
{
	ProfileSeed seed;
	seed.dev = true;
	seed.leaderClass = ClassType::LAUNCHER;
	for(i32 c = 1; c <= 40; c++) {
		seed.masters.push_back((ClassType)c);
		for(i32 s = 0; s < 3; s++) {
			seed.skins.push_back(ProfileSeed::Skin{ (ClassType)c, (SkinIndex)s });
		}
	}
	return seed;
}

static bool LoadIn(SqliteGameStore& backend, AccountUID accountUID, const ProfileSeed& seed, PlayerProfile* out)
{
	if(!backend.Begin()) return false;
	if(!backend.LoadProfile(accountUID, 2000, seed, out)) {
		backend.Rollback();
		return false;
	}
	return backend.Commit();
}

static bool SetLeaderIn(SqliteGameStore& backend, AccountUID accountUID, ClassType classType, SkinIndex skinIndex)
{
	if(!backend.Begin()) return false;
	if(!backend.SetLeader(accountUID, classType, skinIndex)) {
		backend.Rollback();
		return false;
	}
	return backend.Commit();
}

static void TestRetailSeed()
{
	SqliteGameStore backend;
	CHECK(backend.Open(":memory:"));

	PlayerProfile profile;
	CHECK(LoadIn(backend, AccountUID(1), RetailSeed(), &profile));
	CHECK(profile.firstEntry);
	CHECK(profile.leaderClass == ClassType::LAUNCHER);
	CHECK(profile.leaderSkin == SkinIndex::DEFAULT);
	CHECK(profile.masters.size() == 2);
	CHECK(profile.skins.size() == 2);
	CHECK(profile.FindMaster(ClassType::LAUNCHER) && profile.FindMaster(ClassType::ASSASSIN));
	CHECK(!profile.FindMaster(ClassType::STRIKER));
	CHECK(profile.OwnsSkin(ClassType::ASSASSIN, SkinIndex::DEFAULT));
	CHECK(!profile.OwnsSkin(ClassType::ASSASSIN, (SkinIndex)1));

	PlayerProfile again;
	CHECK(LoadIn(backend, AccountUID(1), DevSeed(), &again));
	CHECK(!again.firstEntry);
	CHECK(again.masters.size() == 2);
	CHECK(again.skins.size() == 2);

	backend.Close();
}

static void TestDevSeed()
{
	SqliteGameStore backend;
	CHECK(backend.Open(":memory:"));

	PlayerProfile profile;
	CHECK(LoadIn(backend, AccountUID(2), DevSeed(), &profile));
	CHECK(profile.masters.size() == 40);
	CHECK(profile.skins.size() == 120);
	CHECK(profile.leaderClass == ClassType::LAUNCHER);
	CHECK(profile.OwnsSkin((ClassType)17, (SkinIndex)2));

	backend.Close();
}

static void TestExistingPlayerWithoutMastersIsSeeded()
{
	SqliteGameStore backend;
	CHECK(backend.Open(":memory:"));

	PlayerProfile profile;
	CHECK(LoadIn(backend, AccountUID(3), ProfileSeed(), &profile));
	CHECK(profile.firstEntry);
	CHECK(profile.masters.empty());
	CHECK(profile.leaderClass == ClassType::NONE);

	CHECK(LoadIn(backend, AccountUID(3), RetailSeed(), &profile));
	CHECK(!profile.firstEntry);
	CHECK(profile.masters.size() == 2);
	CHECK(profile.leaderClass == ClassType::LAUNCHER);

	backend.Close();
}

static void TestSetLeaderAndRefuseUnowned()
{
	SqliteGameStore backend;
	CHECK(backend.Open(":memory:"));

	ProfileSeed seed = RetailSeed();
	seed.skins.push_back(ProfileSeed::Skin{ ClassType::ASSASSIN, (SkinIndex)4 });

	PlayerProfile profile;
	CHECK(LoadIn(backend, AccountUID(4), seed, &profile));

	CHECK(SetLeaderIn(backend, AccountUID(4), ClassType::ASSASSIN, (SkinIndex)4));
	CHECK(!SetLeaderIn(backend, AccountUID(4), ClassType::STRIKER, SkinIndex::DEFAULT));
	CHECK(!SetLeaderIn(backend, AccountUID(4), ClassType::LAUNCHER, (SkinIndex)4));
	CHECK(!SetLeaderIn(backend, AccountUID(5), ClassType::ASSASSIN, SkinIndex::DEFAULT));

	CHECK(LoadIn(backend, AccountUID(4), seed, &profile));
	CHECK(profile.leaderClass == ClassType::ASSASSIN);
	CHECK(profile.leaderSkin == (SkinIndex)4);
	const ProfileMaster* assassin = profile.FindMaster(ClassType::ASSASSIN);
	CHECK(assassin && assassin->skinIndex == (SkinIndex)4);
	const ProfileMaster* launcher = profile.FindMaster(ClassType::LAUNCHER);
	CHECK(launcher && launcher->skinIndex == SkinIndex::DEFAULT);

	backend.Close();
}

static void TestLeaderSurvivesRestart()
{
	const char* path = "test_game_store_leader.db";
	RemoveDatabase(path);

	GameStoreOptions options = TestOptions();
	options.seed = RetailSeed();

	{
		SqliteGameStore backend;
		GameStore store;
		CHECK(store.Start(&backend, path, options));
		store.QueueLoadProfile(1, AccountUID(8));
		ProfileLoaded loaded;
		CHECK(WaitLoaded(store, &loaded));
		CHECK(loaded.profile.leaderClass == ClassType::LAUNCHER);
		CHECK(loaded.profile.masters.size() == 2);

		store.QueueWrite("SetLeader", [](GameStoreBackend& b) {
			return b.SetLeader(AccountUID(8), ClassType::ASSASSIN, SkinIndex::DEFAULT);
		});
		store.QueueWrite("SetLeaderUnowned", [](GameStoreBackend& b) {
			return b.SetLeader(AccountUID(8), ClassType::STRIKER, SkinIndex::DEFAULT);
		});
		store.Shutdown();
		CHECK(store.FailedJobCount() == 1);
	}

	{
		SqliteGameStore backend;
		GameStore store;
		CHECK(store.Start(&backend, path, options));
		store.QueueLoadProfile(2, AccountUID(8));
		ProfileLoaded loaded;
		CHECK(WaitLoaded(store, &loaded));
		CHECK(!loaded.profile.firstEntry);
		CHECK(loaded.profile.leaderClass == ClassType::ASSASSIN);
		CHECK(loaded.profile.leaderSkin == SkinIndex::DEFAULT);
		CHECK(loaded.profile.masters.size() == 2);
		store.Shutdown();
	}

	RemoveDatabase(path);
}

static void TestSeedInMemory()
{
	PlayerProfile profile;
	ItemUIDAllocator uids;
	uids.Reset(0);
	SeedProfileInMemory(RetailSeed(), uids, &profile);
	CHECK(profile.gold == 1000);
	CHECK(profile.ccoin == 30);
	CHECK(profile.items.size() == 2);
	CHECK(profile.items.size() == 2 && profile.items[0].uid == (ItemUID)0x40000001 && profile.items[1].slot == 101);
	CHECK(profile.leaderClass == ClassType::LAUNCHER);
	CHECK(profile.masters.size() == 2);
	CHECK(profile.OwnsSkin(ClassType::LAUNCHER, SkinIndex::DEFAULT));
}

static eastl::vector<u8> MakeBlob(u8 seed, i32 size)
{
	eastl::vector<u8> blob;
	for(i32 i = 0; i < size; i++) blob.push_back((u8)(seed + i * 7));
	return blob;
}

static bool SameBlob(const ProfileSettings& slot, const eastl::vector<u8>& blob)
{
	return slot.present && slot.data.size() == blob.size() && memcmp(slot.data.data(), blob.data(), blob.size()) == 0;
}

static void TestSettingsDefaultWhenAbsent()
{
	SqliteGameStore backend;
	CHECK(backend.Open(":memory:"));

	PlayerProfile profile;
	CHECK(LoadIn(backend, AccountUID(20), RetailSeed(), &profile));
	for(i32 i = 0; i < PROFILE_SETTINGS_SLOTS; i++) {
		CHECK(!profile.settings[i].present);
		CHECK(profile.settings[i].data.empty());
	}
	CHECK(profile.tutorialState == TUTORIAL_STATE_TOWN);
	CHECK(profile.statusMessage.empty());
	CHECK(profile.statusMessageAt == 0);
	CHECK(profile.lastLogoutAt == 0);

	PlayerProfile inMemory;
	CHECK(inMemory.tutorialState == -1);
	CHECK(!inMemory.settings[0].present);

	backend.Close();
}

static void TestSettingsSurviveRestart()
{
	const char* path = "test_game_store_settings.db";
	RemoveDatabase(path);

	GameStoreOptions options = TestOptions();
	options.seed = RetailSeed();

	eastl::vector<u8> blobs[PROFILE_SETTINGS_SLOTS] = {
		MakeBlob(0x78, 300),
		MakeBlob(0x10, 1),
		MakeBlob(0x55, 2048),
		MakeBlob(0x99, PROFILE_SETTINGS_MAX_SIZE),
	};
	const eastl::vector<u8> replaced = MakeBlob(0x20, 900);

	{
		SqliteGameStore backend;
		GameStore store;
		CHECK(store.Start(&backend, path, options));
		store.QueueLoadProfile(1, AccountUID(21));
		ProfileLoaded loaded;
		CHECK(WaitLoaded(store, &loaded));
		store.QueueLoadProfile(2, AccountUID(22));
		CHECK(WaitLoaded(store, &loaded));

		for(i32 i = 0; i < PROFILE_SETTINGS_SLOTS; i++) {
			const eastl::vector<u8> blob = blobs[i];
			store.QueueWrite("SaveSettings", [i, blob](GameStoreBackend& b) {
				return b.SaveSettings(AccountUID(21), i, blob.data(), (i32)blob.size(), 3000 + i);
			});
		}
		store.QueueWrite("SaveSettings", [replaced](GameStoreBackend& b) {
			return b.SaveSettings(AccountUID(21), 0, replaced.data(), (i32)replaced.size(), 4000);
		});
		store.QueueWrite("SaveSettings", [](GameStoreBackend& b) {
			const eastl::vector<u8> blob = MakeBlob(1, 10);
			return b.SaveSettings(AccountUID(22), 2, blob.data(), (i32)blob.size(), 4000);
		});
		store.Shutdown();
		CHECK(store.FailedJobCount() == 0);
	}

	{
		SqliteGameStore backend;
		GameStore store;
		CHECK(store.Start(&backend, path, options));
		store.QueueLoadProfile(3, AccountUID(21));
		ProfileLoaded loaded;
		CHECK(WaitLoaded(store, &loaded));
		CHECK(SameBlob(loaded.profile.settings[0], replaced));
		CHECK(SameBlob(loaded.profile.settings[1], blobs[1]));
		CHECK(SameBlob(loaded.profile.settings[2], blobs[2]));
		CHECK(SameBlob(loaded.profile.settings[3], blobs[3]));

		store.QueueLoadProfile(4, AccountUID(22));
		CHECK(WaitLoaded(store, &loaded));
		CHECK(!loaded.profile.settings[0].present);
		CHECK(!loaded.profile.settings[1].present);
		CHECK(SameBlob(loaded.profile.settings[2], MakeBlob(1, 10)));
		CHECK(!loaded.profile.settings[3].present);
		store.Shutdown();
	}

	RemoveDatabase(path);
}

static void TestSettingsRefused()
{
	CHECK(PlayerProfile::CheckSettingsSave(0, 1) == SettingsSaveCheck::OK);
	CHECK(PlayerProfile::CheckSettingsSave(3, PROFILE_SETTINGS_MAX_SIZE) == SettingsSaveCheck::OK);
	CHECK(PlayerProfile::CheckSettingsSave(0, PROFILE_SETTINGS_MAX_SIZE + 1) == SettingsSaveCheck::TOO_LARGE);
	CHECK(PlayerProfile::CheckSettingsSave(2, 0xFFFF) == SettingsSaveCheck::TOO_LARGE);
	CHECK(PlayerProfile::CheckSettingsSave(1, 0) == SettingsSaveCheck::EMPTY);
	CHECK(PlayerProfile::CheckSettingsSave(4, 10) == SettingsSaveCheck::BAD_SLOT);
	CHECK(PlayerProfile::CheckSettingsSave(255, 10) == SettingsSaveCheck::BAD_SLOT);
	CHECK(PlayerProfile::CheckSettingsSave(-1, 10) == SettingsSaveCheck::BAD_SLOT);

	SqliteGameStore backend;
	GameStore store;
	GameStoreOptions options = TestOptions();
	options.seed = RetailSeed();
	CHECK(store.Start(&backend, ":memory:", options));
	store.QueueLoadProfile(1, AccountUID(23));
	ProfileLoaded loaded;
	CHECK(WaitLoaded(store, &loaded));

	store.QueueWrite("SaveSettingsOversize", [](GameStoreBackend& b) {
		const eastl::vector<u8> blob = MakeBlob(0, PROFILE_SETTINGS_MAX_SIZE + 1);
		return b.SaveSettings(AccountUID(23), 0, blob.data(), (i32)blob.size(), 5000);
	});
	store.QueueWrite("SaveSettingsBadSlot", [](GameStoreBackend& b) {
		const eastl::vector<u8> blob = MakeBlob(0, 10);
		return b.SaveSettings(AccountUID(23), 4, blob.data(), (i32)blob.size(), 5000);
	});
	store.QueueLoadProfile(2, AccountUID(23));
	CHECK(WaitLoaded(store, &loaded));
	for(i32 i = 0; i < PROFILE_SETTINGS_SLOTS; i++) {
		CHECK(!loaded.profile.settings[i].present);
	}
	store.Shutdown();
	CHECK(store.FailedJobCount() == 2);
}

static void TestStatusMessageRule()
{
	PlayerProfile profile;
	CHECK(profile.CheckStatusMessage(10, 1000) == ErrorType::SUCCESS);
	CHECK(profile.CheckStatusMessage(STATUS_MESSAGE_MAX_LEN, 1000) == ErrorType::SUCCESS);
	CHECK(profile.CheckStatusMessage(STATUS_MESSAGE_MAX_LEN + 1, 1000) == ErrorType::STATUS_MESSAGE_TOO_LONG);
	CHECK((u32)ErrorType::STATUS_MESSAGE_TOO_LONG == 378);
	CHECK((u32)ErrorType::STATUS_MESSAGE_TOO_OFTEN == 379);

	profile.SetStatusMessage(L"hello", 5, 1000);
	CHECK(profile.statusMessage == L"hello");
	CHECK(profile.statusMessageAt == 1000);
	CHECK(profile.CheckStatusMessage(3, 1000) == ErrorType::STATUS_MESSAGE_TOO_OFTEN);
	CHECK(profile.CheckStatusMessage(3, 1059) == ErrorType::STATUS_MESSAGE_TOO_OFTEN);
	CHECK(profile.CheckStatusMessage(0, 1059) == ErrorType::STATUS_MESSAGE_TOO_OFTEN);
	CHECK(profile.CheckStatusMessage(3, 1060) == ErrorType::SUCCESS);
	CHECK(profile.CheckStatusMessage(STATUS_MESSAGE_MAX_LEN + 1, 1060) == ErrorType::STATUS_MESSAGE_TOO_LONG);

	SqliteGameStore backend;
	CHECK(backend.Open(":memory:"));
	PlayerProfile stored;
	CHECK(LoadIn(backend, AccountUID(24), RetailSeed(), &stored));

	const wchar text[] = L"été 你好";
	const i32 textLen = (i32)(sizeof(text) / sizeof(wchar)) - 1;
	CHECK(backend.Begin());
	CHECK(backend.SetStatusMessage(AccountUID(24), text, textLen, 7000));
	CHECK(!backend.SetStatusMessage(AccountUID(25), text, textLen, 7000));
	CHECK(!backend.SetStatusMessage(AccountUID(24), text, STATUS_MESSAGE_MAX_LEN + 1, 7000));
	CHECK(backend.Commit());

	CHECK(LoadIn(backend, AccountUID(24), RetailSeed(), &stored));
	CHECK((i32)stored.statusMessage.size() == textLen);
	CHECK(memcmp(stored.statusMessage.data(), text, textLen * sizeof(wchar)) == 0);
	CHECK(stored.statusMessageAt == 7000);
	CHECK(stored.CheckStatusMessage(1, 7030) == ErrorType::STATUS_MESSAGE_TOO_OFTEN);
	CHECK(stored.CheckStatusMessage(1, 7060) == ErrorType::SUCCESS);

	CHECK(backend.Begin());
	CHECK(backend.SetStatusMessage(AccountUID(24), L"", 0, 7100));
	CHECK(backend.Commit());
	CHECK(LoadIn(backend, AccountUID(24), RetailSeed(), &stored));
	CHECK(stored.statusMessage.empty());
	CHECK(stored.statusMessageAt == 7100);

	backend.Close();
}

static void TestLastLogout()
{
	SqliteGameStore backend;
	CHECK(backend.Open(":memory:"));
	PlayerProfile profile;
	CHECK(LoadIn(backend, AccountUID(26), RetailSeed(), &profile));
	CHECK(profile.lastLogoutAt == 0);

	CHECK(backend.Begin());
	CHECK(backend.SetLastLogout(AccountUID(26), 8000));
	CHECK(!backend.SetLastLogout(AccountUID(27), 8000));
	CHECK(backend.Commit());

	CHECK(LoadIn(backend, AccountUID(26), RetailSeed(), &profile));
	CHECK(profile.lastLogoutAt == 8000);
	CHECK(profile.lastLoginAt == 2000);

	backend.Close();
}

static i64 CountLedger(SqliteGameStore& backend, AccountUID accountUID)
{
	Statement select(backend.db, "SELECT COUNT(*) FROM asset_ledger WHERE account_uid=?1");
	select.BindInt(1, (u32)accountUID);
	if(select.Step() != StepResult::ROW) return -1;
	return select.ColumnInt(0);
}

struct LedgerRow
{
	i64 asset, delta, balance, reason, docIndex, quantity, itemUID;
};

static bool LastLedger(SqliteGameStore& backend, AccountUID accountUID, LedgerRow* out)
{
	Statement select(backend.db, "SELECT asset, delta, balance, reason, doc_index, quantity, item_uid FROM asset_ledger WHERE account_uid=?1 ORDER BY rowid DESC LIMIT 1");
	select.BindInt(1, (u32)accountUID);
	if(select.Step() != StepResult::ROW) return false;
	out->asset = select.ColumnInt(0);
	out->delta = select.ColumnInt(1);
	out->balance = select.ColumnInt(2);
	out->reason = select.ColumnInt(3);
	out->docIndex = select.ColumnInt(4);
	out->quantity = select.ColumnInt(5);
	out->itemUID = select.ColumnInt(6);
	return true;
}

template<typename F>
static bool InJob(SqliteGameStore& backend, F f)
{
	if(!backend.Begin()) return false;
	if(!f()) {
		backend.Rollback();
		return false;
	}
	return backend.Commit();
}

static LedgerNote Note(AssetChangeReason reason, u32 docIndex = 0, i32 quantity = 0)
{
	LedgerNote note;
	note.reason = reason;
	note.docIndex = (ItemDocIndex)docIndex;
	note.quantity = quantity;
	note.at = 3000;
	return note;
}

static ProfileItem MakeItem(ItemUID uid, InvenType invenType, i32 slot, u32 docIndex, i32 count)
{
	ProfileItem item;
	item.uid = uid;
	item.invenType = invenType;
	item.slot = slot;
	item.docIndex = (ItemDocIndex)docIndex;
	item.count = count;
	return item;
}

static void TestWalletSeed()
{
	SqliteGameStore backend;
	CHECK(backend.Open(":memory:"));

	PlayerProfile profile;
	CHECK(LoadIn(backend, AccountUID(40), RetailSeed(), &profile));
	CHECK(profile.gold == 1000);
	CHECK(profile.ccoin == 30);
	CHECK(profile.lines[0] == 4 && profile.lines[1] == 4 && profile.lines[2] == 4 && profile.lines[CONTAINER_WAREHOUSE] == 1);
	CHECK(profile.items.size() == 2);
	foreach_const(it, profile.items) {
		CHECK(ItemUIDAllocator::IsItemUID(it->uid));
		CHECK(it->invenType == InvenType::INVENTORY);
		CHECK(it->docIndex == (ItemDocIndex)132199998);
		CHECK(it->count == 1);
		CHECK(it->propertyGroup == 723002202);
		CHECK(ItemContainer(it->invenType, it->slot) == 0);
	}
	CHECK(profile.items.size() == 2 && profile.items[0].slot == 100 && profile.items[1].slot == 101);
	CHECK(CountLedger(backend, AccountUID(40)) == 4);
	LedgerRow row;
	CHECK(LastLedger(backend, AccountUID(40), &row));
	CHECK(row.asset == (i64)LedgerAsset::ITEM && row.reason == (i64)AssetChangeReason::INITIALIZE && row.delta == 1);

	PlayerProfile again;
	CHECK(LoadIn(backend, AccountUID(40), RetailSeed(), &again));
	CHECK(again.gold == 1000 && again.items.size() == 2);
	CHECK(again.items.size() == 2 && again.items[0].uid == profile.items[0].uid && again.items[1].uid == profile.items[1].uid);
	CHECK(CountLedger(backend, AccountUID(40)) == 4);

	PlayerProfile empty;
	CHECK(LoadIn(backend, AccountUID(41), ProfileSeed(), &empty));
	CHECK(empty.gold == 0 && empty.ccoin == 0 && empty.items.empty());

	backend.Close();
}

static void TestGrantAndSpend()
{
	SqliteGameStore backend;
	CHECK(backend.Open(":memory:"));
	const AccountUID uid = AccountUID(42);

	PlayerProfile profile;
	CHECK(LoadIn(backend, uid, RetailSeed(), &profile));
	const i64 ledgerStart = CountLedger(backend, uid);

	CHECK(InJob(backend, [&]() { return backend.GrantCurrency(uid, Currency::GOLD, 500, Note(AssetChangeReason::SHOP_SELL_ITEM, 132199191, 1)); }));
	LedgerRow row;
	CHECK(LastLedger(backend, uid, &row));
	CHECK(row.asset == (i64)LedgerAsset::GOLD && row.delta == 500 && row.balance == 1500 && row.reason == (i64)AssetChangeReason::SHOP_SELL_ITEM && row.docIndex == 132199191 && row.quantity == 1);

	CHECK(InJob(backend, [&]() { return backend.GrantCurrency(uid, Currency::CCOIN, 70, Note(AssetChangeReason::BUILDER_COMMAND)); }));
	CHECK(!InJob(backend, [&]() { return backend.SpendCurrency(uid, Currency::GOLD, 1501, Note(AssetChangeReason::SHOP_BUY_ITEM)); }));
	CHECK(!InJob(backend, [&]() { return backend.SpendCurrency(uid, Currency::CCOIN, 101, Note(AssetChangeReason::SHOP_BUY_ITEM)); }));
	CHECK(!InJob(backend, [&]() { return backend.SpendCurrency(uid, Currency::GOLD, -5, Note(AssetChangeReason::SHOP_BUY_ITEM)); }));
	CHECK(!InJob(backend, [&]() { return backend.GrantCurrency(uid, Currency::GOLD, -5, Note(AssetChangeReason::SHOP_BUY_ITEM)); }));
	CHECK(!InJob(backend, [&]() { return backend.GrantCurrency(uid, Currency::GOLD, CURRENCY_MAX, Note(AssetChangeReason::BUILDER_COMMAND)); }));
	CHECK(!InJob(backend, [&]() { return backend.GrantCurrency(AccountUID(999), Currency::GOLD, 1, Note(AssetChangeReason::BUILDER_COMMAND)); }));
	CHECK(CountLedger(backend, uid) == ledgerStart + 2);

	CHECK(InJob(backend, [&]() { return backend.SpendCurrency(uid, Currency::GOLD, 1500, Note(AssetChangeReason::SHOP_BUY_ITEM, 132199391, 1)); }));
	CHECK(LastLedger(backend, uid, &row));
	CHECK(row.delta == -1500 && row.balance == 0 && row.reason == (i64)AssetChangeReason::SHOP_BUY_ITEM);

	PlayerProfile reloaded;
	CHECK(LoadIn(backend, uid, RetailSeed(), &reloaded));
	CHECK(reloaded.gold == 0);
	CHECK(reloaded.ccoin == 100);

	CHECK(reloaded.CanSpend(Currency::CCOIN, 100));
	CHECK(!reloaded.CanSpend(Currency::CCOIN, 101));
	CHECK(!reloaded.CanSpend(Currency::GOLD, 1));
	CHECK(!reloaded.CanSpend(Currency::GOLD, -1));
	CHECK(reloaded.CanGrant(Currency::GOLD, 10));
	CHECK(!reloaded.CanGrant(Currency::GOLD, -1));
	reloaded.Grant(Currency::GOLD, 10);
	reloaded.Spend(Currency::GOLD, 4);
	CHECK(reloaded.gold == 6);

	backend.Close();
}

static void TestLedgerInSameTransaction()
{
	SqliteGameStore backend;
	GameStore store;
	GameStoreOptions options = TestOptions();
	options.seed = RetailSeed();
	CHECK(store.Start(&backend, ":memory:", options));
	const AccountUID uid = AccountUID(43);

	store.QueueLoadProfile(1, uid);
	ProfileLoaded loaded;
	CHECK(WaitLoaded(store, &loaded));
	CHECK(loaded.profile.gold == 1000);
	const i32 takenSlot = loaded.profile.items.empty() ? 100 : loaded.profile.items[0].slot;

	i64 ledgerBefore = -1;
	store.QueueWrite("Count", [&](GameStoreBackend&) { ledgerBefore = CountLedger(backend, uid); return true; });

	const ProfileItem clash = MakeItem(store.AllocateItemUID(), InvenType::INVENTORY, takenSlot, 132199391, 1);
	store.QueueWrite("BuyClash", [uid, clash](GameStoreBackend& b) {
		const LedgerNote note = Note(AssetChangeReason::SHOP_BUY_ITEM, 132199391, 1);
		return b.SpendCurrency(uid, Currency::GOLD, 250, note) && b.AddItem(uid, clash, note);
	});

	i64 ledgerAfterFail = -1;
	store.QueueWrite("Count", [&](GameStoreBackend&) { ledgerAfterFail = CountLedger(backend, uid); return true; });

	const ProfileItem bought = MakeItem(store.AllocateItemUID(), InvenType::INVENTORY, 102, 132199391, 1);
	store.QueueWrite("Buy", [uid, bought](GameStoreBackend& b) {
		const LedgerNote note = Note(AssetChangeReason::SHOP_BUY_ITEM, 132199391, 1);
		return b.SpendCurrency(uid, Currency::GOLD, 250, note) && b.AddItem(uid, bought, note);
	});

	i64 ledgerAfterBuy = -1;
	store.QueueWrite("Count", [&](GameStoreBackend&) { ledgerAfterBuy = CountLedger(backend, uid); return true; });

	store.QueueLoadProfile(2, uid);
	CHECK(WaitLoaded(store, &loaded));
	CHECK(store.FailedJobCount() == 1);
	CHECK(ledgerBefore >= 0 && ledgerAfterFail == ledgerBefore);
	CHECK(ledgerAfterBuy == ledgerBefore + 2);
	CHECK(loaded.profile.gold == 750);
	CHECK(loaded.profile.items.size() == 3);
	CHECK(loaded.profile.FindItem(bought.uid) && loaded.profile.FindItem(bought.uid)->slot == 102);
	CHECK(!loaded.profile.FindItem(clash.uid));

	store.Shutdown();
}

static void TestItemAddRemoveMove()
{
	SqliteGameStore backend;
	CHECK(backend.Open(":memory:"));
	const AccountUID uid = AccountUID(44);

	PlayerProfile profile;
	CHECK(LoadIn(backend, uid, RetailSeed(), &profile));

	ProfileItem stack = MakeItem(backend.itemUIDs.Next(), InvenType::INVENTORY, 200, 137120001, 5);
	stack.lifeAbsolute = 1;
	stack.lifeEndUTC = 123456789;
	ItemProperty prop;
	prop.type = 1;
	prop.typeDetail = 7;
	prop.valueType = 2;
	prop.value = 2.4f;
	prop.fixed = 1;
	stack.properties.push_back(prop);

	CHECK(profile.CheckAddItem(stack) == ItemCheck::OK);
	profile.AddItem(stack);
	CHECK(profile.CheckAddItem(stack) == ItemCheck::UID_TAKEN);
	CHECK(InJob(backend, [&]() { return backend.AddItem(uid, stack, Note(AssetChangeReason::MAIL_ATTACHMENT, 137120001, 5)); }));
	CHECK(!InJob(backend, [&]() { return backend.AddItem(uid, stack, Note(AssetChangeReason::MAIL_ATTACHMENT)); }));

	const ProfileItem sameSlot = MakeItem(backend.itemUIDs.Next(), InvenType::INVENTORY, 200, 137120001, 1);
	CHECK(profile.CheckAddItem(sameSlot) == ItemCheck::SLOT_TAKEN);
	CHECK(!InJob(backend, [&]() { return backend.AddItem(uid, sameSlot, Note(AssetChangeReason::MAIL_ATTACHMENT)); }));

	const ProfileItem lockedSlot = MakeItem(backend.itemUIDs.Next(), InvenType::INVENTORY, 124, 137120001, 1);
	CHECK(profile.CheckAddItem(lockedSlot) == ItemCheck::BAD_SLOT);
	CHECK(!InJob(backend, [&]() { return backend.AddItem(uid, lockedSlot, Note(AssetChangeReason::MAIL_ATTACHMENT)); }));

	const ProfileItem badSlot = MakeItem(backend.itemUIDs.Next(), InvenType::INVENTORY, 50, 137120001, 1);
	CHECK(profile.CheckAddItem(badSlot) == ItemCheck::BAD_SLOT);
	const ProfileItem zero = MakeItem(backend.itemUIDs.Next(), InvenType::INVENTORY, 201, 137120001, 0);
	CHECK(profile.CheckAddItem(zero) == ItemCheck::BAD_COUNT);
	CHECK(!InJob(backend, [&]() { return backend.AddItem(uid, zero, Note(AssetChangeReason::MAIL_ATTACHMENT)); }));
	const ProfileItem untagged = MakeItem((ItemUID)77, InvenType::INVENTORY, 201, 137120001, 1);
	CHECK(profile.CheckAddItem(untagged) == ItemCheck::BAD_UID);
	CHECK(!InJob(backend, [&]() { return backend.AddItem(uid, untagged, Note(AssetChangeReason::MAIL_ATTACHMENT)); }));

	CHECK(profile.CheckRemoveItem(stack.uid, 6) == ItemCheck::BAD_COUNT);
	CHECK(profile.CheckRemoveItem(stack.uid, 0) == ItemCheck::BAD_COUNT);
	CHECK(profile.CheckRemoveItem((ItemUID)0x40001234, 1) == ItemCheck::NOT_FOUND);
	CHECK(!InJob(backend, [&]() { return backend.RemoveItem(uid, stack.uid, 6, Note(AssetChangeReason::UNKNOWN)); }));
	CHECK(!InJob(backend, [&]() { return backend.RemoveItem(AccountUID(45), stack.uid, 1, Note(AssetChangeReason::UNKNOWN)); }));

	CHECK(profile.CheckRemoveItem(stack.uid, 2) == ItemCheck::OK);
	profile.RemoveItem(stack.uid, 2);
	CHECK(InJob(backend, [&]() { return backend.RemoveItem(uid, stack.uid, 2, Note(AssetChangeReason::UNKNOWN, 137120001, 2)); }));
	LedgerRow row;
	CHECK(LastLedger(backend, uid, &row));
	CHECK(row.asset == (i64)LedgerAsset::ITEM && row.delta == -2 && row.balance == 3 && row.itemUID == (u32)stack.uid && row.docIndex == 137120001);

	const ItemUID gear = profile.items[0].uid;
	CHECK(profile.CheckMoveItem(stack.uid, InvenType::INVENTORY, 100) == ItemCheck::SLOT_TAKEN);
	CHECK(!InJob(backend, [&]() { return backend.MoveItem(uid, stack.uid, InvenType::INVENTORY, 100, Note(AssetChangeReason::UNKNOWN)); }));
	CHECK(profile.CheckMoveItem(stack.uid, InvenType::WAREHOUSE, 6) == ItemCheck::BAD_SLOT);
	CHECK(!InJob(backend, [&]() { return backend.MoveItem(uid, stack.uid, InvenType::WAREHOUSE, 6, Note(AssetChangeReason::UNKNOWN)); }));
	CHECK(profile.CheckMoveItem(stack.uid, InvenType::WAREHOUSE, 5) == ItemCheck::OK);
	profile.MoveItem(stack.uid, InvenType::WAREHOUSE, 5);
	CHECK(InJob(backend, [&]() { return backend.MoveItem(uid, stack.uid, InvenType::WAREHOUSE, 5, Note(AssetChangeReason::UNKNOWN)); }));
	CHECK(profile.CheckMoveItem(gear, InvenType::INVENTORY, 200) == ItemCheck::OK);
	profile.MoveItem(gear, InvenType::INVENTORY, 200);
	CHECK(InJob(backend, [&]() { return backend.MoveItem(uid, gear, InvenType::INVENTORY, 200, Note(AssetChangeReason::UNKNOWN)); }));

	PlayerProfile reloaded;
	CHECK(LoadIn(backend, uid, RetailSeed(), &reloaded));
	CHECK(reloaded.items.size() == profile.items.size());
	const ProfileItem* stored = reloaded.FindItem(stack.uid);
	CHECK(stored && stored->invenType == InvenType::WAREHOUSE && stored->slot == 5 && stored->count == 3);
	CHECK(stored && stored->lifeAbsolute == 1 && stored->lifeEndUTC == 123456789);
	CHECK(stored && stored->properties.size() == 1 && stored->properties[0].typeDetail == 7 && stored->properties[0].value == 2.4f && stored->properties[0].fixed == 1);
	CHECK(reloaded.FindItem(gear) && reloaded.FindItem(gear)->slot == 200);

	profile.RemoveItem(stack.uid, 3);
	CHECK(!profile.FindItem(stack.uid));
	CHECK(InJob(backend, [&]() { return backend.RemoveItem(uid, stack.uid, 3, Note(AssetChangeReason::SHOP_SELL_ITEM, 137120001, 3)); }));
	{
		Statement select(backend.db, "SELECT COUNT(*) FROM item_properties WHERE item_uid=?1");
		select.BindInt(1, (u32)stack.uid);
		CHECK(select.Step() == StepResult::ROW && select.ColumnInt(0) == 0);
	}
	CHECK(LoadIn(backend, uid, RetailSeed(), &reloaded));
	CHECK(!reloaded.FindItem(stack.uid));
	CHECK(reloaded.items.size() == 2);

	CHECK(!InJob(backend, [&]() { return backend.SetInventoryLines(uid, 0, 3); }));
	CHECK(!InJob(backend, [&]() { return backend.SetInventoryLines(uid, 0, 17); }));
	CHECK(!InJob(backend, [&]() { return backend.SetInventoryLines(uid, CONTAINER_WAREHOUSE, 10); }));
	CHECK(!InJob(backend, [&]() { return backend.SetInventoryLines(uid, 4, 5); }));
	CHECK(InJob(backend, [&]() { return backend.SetInventoryLines(uid, 0, 5); }));
	CHECK(LoadIn(backend, uid, RetailSeed(), &reloaded));
	CHECK(reloaded.lines[0] == 5);
	CHECK(reloaded.SlotUsable(InvenType::INVENTORY, 129));
	CHECK(!reloaded.SlotUsable(InvenType::INVENTORY, 130));
	CHECK(reloaded.FreeSlotCount(0) == 29);

	backend.Close();
}

static void TestItemSplitInOneJob()
{
	SqliteGameStore backend;
	GameStore store;
	GameStoreOptions options = TestOptions();
	options.seed = RetailSeed();
	CHECK(store.Start(&backend, ":memory:", options));
	const AccountUID uid = AccountUID(46);

	store.QueueLoadProfile(1, uid);
	ProfileLoaded loaded;
	CHECK(WaitLoaded(store, &loaded));
	PlayerProfile& profile = loaded.profile;

	ProfileItem stack = MakeItem(store.AllocateItemUID(), InvenType::INVENTORY, 200, 137120001, 5);
	stack.lifeAbsolute = 1;
	stack.lifeEndUTC = 987654321;
	ItemProperty prop;
	prop.type = 3;
	prop.typeDetail = 11;
	prop.value = 1.5f;
	stack.properties.push_back(prop);
	profile.AddItem(stack);
	store.QueueWrite("Add", [uid, stack](GameStoreBackend& b) {
		return b.AddItem(uid, stack, Note(AssetChangeReason::MAIL_ATTACHMENT, 137120001, 5));
	});

	ProfileItem split = stack;
	split.uid = store.AllocateItemUID();
	split.invenType = InvenType::WAREHOUSE;
	split.slot = 2;
	split.count = 2;

	ProfileItem clash = split;
	clash.slot = 200;
	clash.invenType = InvenType::INVENTORY;
	CHECK(profile.CheckAddItem(clash) == ItemCheck::SLOT_TAKEN);
	store.QueueWrite("SplitClash", [uid, stack, clash](GameStoreBackend& b) {
		const LedgerNote note = Note(AssetChangeReason::UNKNOWN, 137120001, 2);
		return b.RemoveItem(uid, stack.uid, 2, note) && b.AddItem(uid, clash, note);
	});

	CHECK(profile.CheckAddItem(split) == ItemCheck::OK);
	profile.RemoveItem(stack.uid, 2);
	profile.AddItem(split);
	store.QueueWrite("Split", [uid, stack, split](GameStoreBackend& b) {
		const LedgerNote note = Note(AssetChangeReason::UNKNOWN, 137120001, 2);
		return b.RemoveItem(uid, stack.uid, 2, note) && b.AddItem(uid, split, note);
	});

	store.QueueLoadProfile(2, uid);
	PlayerProfile reloaded;
	CHECK(WaitLoaded(store, &loaded));
	reloaded = loaded.profile;
	CHECK(store.FailedJobCount() == 1);
	const ProfileItem* source = reloaded.FindItem(stack.uid);
	const ProfileItem* moved = reloaded.FindItem(split.uid);
	CHECK(source && source->invenType == InvenType::INVENTORY && source->slot == 200 && source->count == 3);
	CHECK(moved && moved->invenType == InvenType::WAREHOUSE && moved->slot == 2 && moved->count == 2);
	CHECK(moved && moved->docIndex == (ItemDocIndex)137120001 && moved->lifeAbsolute == 1 && moved->lifeEndUTC == 987654321);
	CHECK(moved && moved->properties.size() == 1 && moved->properties[0].typeDetail == 11 && moved->properties[0].value == 1.5f);
	CHECK(reloaded.items.size() == 4);

	store.Shutdown();
}

static void TestInventoryExpandJob()
{
	SqliteGameStore backend;
	GameStore store;
	GameStoreOptions options = TestOptions();
	options.seed = RetailSeed();
	CHECK(store.Start(&backend, ":memory:", options));
	const AccountUID uid = AccountUID(47);

	store.QueueLoadProfile(1, uid);
	ProfileLoaded loaded;
	CHECK(WaitLoaded(store, &loaded));
	CHECK(loaded.profile.gold == 1000 && loaded.profile.lines[1] == 4);

	store.QueueWrite("Grant", [uid](GameStoreBackend& b) {
		return b.GrantCurrency(uid, Currency::GOLD, 1000, Note(AssetChangeReason::BUILDER_COMMAND));
	});
	store.QueueWrite("ExpandTooDear", [uid](GameStoreBackend& b) {
		return b.SpendCurrency(uid, Currency::GOLD, 5000, Note(AssetChangeReason::INVENTORY_EXPAND, 0, 1)) && b.SetInventoryLines(uid, 1, 5);
	});
	store.QueueWrite("Expand", [uid](GameStoreBackend& b) {
		return b.SpendCurrency(uid, Currency::GOLD, 1800, Note(AssetChangeReason::INVENTORY_EXPAND, 0, 1)) && b.SetInventoryLines(uid, 1, 5);
	});
	LedgerRow row;
	bool rowRead = false;
	store.QueueWrite("Ledger", [&](GameStoreBackend&) { rowRead = LastLedger(backend, uid, &row); return true; });

	store.QueueLoadProfile(2, uid);
	CHECK(WaitLoaded(store, &loaded));
	CHECK(store.FailedJobCount() == 1);
	CHECK(loaded.profile.gold == 200);
	CHECK(loaded.profile.lines[0] == 4 && loaded.profile.lines[1] == 5 && loaded.profile.lines[2] == 4);
	CHECK(loaded.profile.SlotUsable(InvenType::INVENTORY, 229));
	CHECK(!loaded.profile.SlotUsable(InvenType::INVENTORY, 230));
	CHECK(rowRead && row.asset == (i64)LedgerAsset::GOLD && row.delta == -1800 && row.balance == 200 && row.reason == (i64)AssetChangeReason::INVENTORY_EXPAND && row.quantity == 1);

	store.Shutdown();
}

static void TestItemUIDsUniqueAcrossRestart()
{
	const char* path = "test_game_store_uids.db";
	RemoveDatabase(path);

	eastl::vector<ItemUID> seen;
	ItemUID highest = ItemUID::INVALID;

	{
		SqliteGameStore backend;
		GameStore store;
		GameStoreOptions options = TestOptions();
		options.seed = RetailSeed();
		CHECK(store.Start(&backend, path, options));
		CHECK(backend.itemUIDs.LastSeq() == 0);

		for(i32 i = 0; i < 2; i++) {
			store.QueueLoadProfile(1 + i, AccountUID(50 + i));
			ProfileLoaded loaded;
			CHECK(WaitLoaded(store, &loaded));
			foreach_const(it, loaded.profile.items) seen.push_back(it->uid);
		}

		for(i32 i = 0; i < 3; i++) {
			const ProfileItem item = MakeItem(store.AllocateItemUID(), InvenType::INVENTORY, 200 + i, 137120001, 1);
			seen.push_back(item.uid);
			highest = item.uid;
			store.QueueWrite("Add", [item](GameStoreBackend& b) {
				return b.AddItem(AccountUID(50), item, Note(AssetChangeReason::MAIL_ATTACHMENT));
			});
		}
		store.QueueWrite("RemoveHighest", [highest](GameStoreBackend& b) {
			return b.RemoveItem(AccountUID(50), highest, 1, Note(AssetChangeReason::UNKNOWN));
		});
		store.Shutdown();
		CHECK(store.FailedJobCount() == 0);
	}

	CHECK(seen.size() == 7);
	for(i32 i = 0; i < (i32)seen.size(); i++) {
		CHECK(ItemUIDAllocator::IsItemUID(seen[i]));
		for(i32 j = i + 1; j < (i32)seen.size(); j++) CHECK(seen[i] != seen[j]);
	}

	{
		SqliteGameStore backend;
		CHECK(backend.Open(path));
		CHECK(backend.itemUIDs.LastSeq() == ItemUIDAllocator::Seq(highest));
		const ItemUID next = backend.itemUIDs.Next();
		foreach_const(it, seen) CHECK(*it != next);
		CHECK(ItemUIDAllocator::Seq(next) == ItemUIDAllocator::Seq(highest) + 1);

		PlayerProfile profile;
		CHECK(LoadIn(backend, AccountUID(52), RetailSeed(), &profile));
		foreach_const(it, profile.items) {
			foreach_const(s, seen) CHECK(*s != it->uid);
			CHECK(it->uid != next);
		}
		backend.Close();
	}

	RemoveDatabase(path);
}

int main(int argc, char** argv)
{
	PlatformInit();
	LogInit("test_game_store.log");

	TestJobsRunInOrder();
	TestLoadSeesEarlierWrites();
	TestFailedJobRollsBack();
	TestShutdownDrains();
	TestRestartAfterShutdown();
	TestRetailSeed();
	TestDevSeed();
	TestExistingPlayerWithoutMastersIsSeeded();
	TestSetLeaderAndRefuseUnowned();
	TestLeaderSurvivesRestart();
	TestSeedInMemory();
	TestSettingsDefaultWhenAbsent();
	TestSettingsSurviveRestart();
	TestSettingsRefused();
	TestStatusMessageRule();
	TestLastLogout();
	TestWalletSeed();
	TestGrantAndSpend();
	TestLedgerInSameTransaction();
	TestItemAddRemoveMove();
	TestItemSplitInOneJob();
	TestInventoryExpandJob();
	TestItemUIDsUniqueAcrossRestart();

	if(failures > 0) {
		LOG("test_game_store: %d check(s) failed", failures);
		return 1;
	}
	LOG("test_game_store: all checks passed");
	return 0;
}
