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
	SeedProfileInMemory(RetailSeed(), &profile);
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

	if(failures > 0) {
		LOG("test_game_store: %d check(s) failed", failures);
		return 1;
	}
	LOG("test_game_store: all checks passed");
	return 0;
}
