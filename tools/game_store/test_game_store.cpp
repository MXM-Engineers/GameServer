#include <common/base.h>
#include <storage/game_store.h>
#include <storage/game_db.h>
#include <EASTL/vector.h>
#include <stdio.h>

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

int main(int argc, char** argv)
{
	PlatformInit();
	LogInit("test_game_store.log");

	TestJobsRunInOrder();
	TestLoadSeesEarlierWrites();
	TestFailedJobRollsBack();
	TestShutdownDrains();
	TestRestartAfterShutdown();

	if(failures > 0) {
		LOG("test_game_store: %d check(s) failed", failures);
		return 1;
	}
	LOG("test_game_store: all checks passed");
	return 0;
}
