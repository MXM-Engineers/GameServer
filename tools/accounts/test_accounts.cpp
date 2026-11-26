#include <common/base.h>
#include <storage/account_db.h>
#include <storage/game_db.h>

#define CHECK(cond) do { if(!(cond)) { LOG("FAILED: %s (line %d)", #cond, __LINE__); failures++; } } while(0)

static i32 failures = 0;

static LoginOutcome Login(AccountDB& db, const wchar* name, const wchar* password, const LoginRules& rules, LoginSession* out)
{
	return db.Login(name, (i32)EA::StdC::Strlen(name), password, (i32)EA::StdC::Strlen(password), "127.0.0.1", rules, out);
}

static void TestPasswordRules()
{
	AccountDB db;
	CHECK(db.Open(":memory:"));

	LoginRules strict;
	LoginSession created;
	CHECK(Login(db, L"delta", L"hunter2", strict, &created) == LoginOutcome::SUCCESS);
	CHECK(created.created);
	CHECK(created.accountUID != AccountUID::INVALID);
	CHECK(created.instantKey != 0);

	LoginSession again;
	CHECK(Login(db, L"delta", L"hunter2", strict, &again) == LoginOutcome::SUCCESS);
	CHECK(!again.created);
	CHECK(again.accountUID == created.accountUID);
	CHECK(again.instantKey != created.instantKey);

	LoginSession caseInsensitive;
	CHECK(Login(db, L"DELTA", L"hunter2", strict, &caseInsensitive) == LoginOutcome::SUCCESS);
	CHECK(caseInsensitive.accountUID == created.accountUID);

	LoginSession refused;
	CHECK(Login(db, L"delta", L"hunter3", strict, &refused) == LoginOutcome::INCORRECT_PASSWORD);
	CHECK(refused.accountUID == AccountUID::INVALID);
	CHECK(Login(db, L"delta", L"", strict, &refused) == LoginOutcome::INCORRECT_PASSWORD);
	CHECK(Login(db, L"", L"hunter2", strict, &refused) == LoginOutcome::NOT_REGISTERED);
	CHECK(Login(db, L"two words", L"hunter2", strict, &refused) == LoginOutcome::NOT_REGISTERED);
	CHECK(Login(db, L"0123456789012345678901234567890123", L"hunter2", strict, &refused) == LoginOutcome::NOT_REGISTERED);

	LoginSession found;
	CHECK(db.FindSession(L"delta", 5, &found));
	CHECK(found.accountUID == created.accountUID);
	CHECK(found.instantKey == caseInsensitive.instantKey);
	CHECK(!db.FindSession(L"nobody", 6, &found));

	LoginSession other;
	CHECK(Login(db, L"other", L"pw", strict, &other) == LoginOutcome::SUCCESS);
	CHECK(other.accountUID != created.accountUID);

	LoginRules closed;
	closed.autoCreateAccounts = false;
	CHECK(Login(db, L"stranger", L"pw", closed, &refused) == LoginOutcome::NOT_REGISTERED);
	CHECK(Login(db, L"delta", L"hunter2", closed, &again) == LoginOutcome::SUCCESS);

	CHECK(db.SetBanned(L"delta", 5, true));
	CHECK(Login(db, L"delta", L"hunter2", strict, &refused) == LoginOutcome::BANNED);
	CHECK(db.SetBanned(L"delta", 5, false));
	CHECK(Login(db, L"delta", L"hunter2", strict, &again) == LoginOutcome::SUCCESS);

	db.Close();
}

static void TestDevRules()
{
	AccountDB db;
	CHECK(db.Open(":memory:"));

	LoginRules dev;
	dev.requirePassword = false;
	LoginRules strict;

	LoginSession first;
	CHECK(Login(db, L"test", L"", dev, &first) == LoginOutcome::SUCCESS);
	CHECK(first.created);

	LoginSession second;
	CHECK(Login(db, L"test", L"anything", dev, &second) == LoginOutcome::SUCCESS);
	CHECK(second.accountUID == first.accountUID);

	LoginSession refused;
	CHECK(Login(db, L"test", L"anything", strict, &refused) == LoginOutcome::INCORRECT_PASSWORD);

	db.Close();
}

static void TestPersistence()
{
	const char* path = "test_accounts.db";
	remove(path);
	remove("test_accounts.db-wal");
	remove("test_accounts.db-shm");

	LoginRules strict;
	LoginSession before;
	{
		AccountDB db;
		CHECK(db.Open(path));
		CHECK(db.db.SchemaVersion() == 2);
		CHECK(Login(db, L"keeper", L"pw", strict, &before) == LoginOutcome::SUCCESS);
		db.Close();
	}

	{
		AccountDB db;
		CHECK(db.Open(path));
		CHECK(db.db.SchemaVersion() == 2);
		LoginSession after;
		CHECK(Login(db, L"keeper", L"pw", strict, &after) == LoginOutcome::SUCCESS);
		CHECK(!after.created);
		CHECK(after.accountUID == before.accountUID);
		LoginSession refused;
		CHECK(Login(db, L"keeper", L"wrong", strict, &refused) == LoginOutcome::INCORRECT_PASSWORD);
		db.Close();
	}

	remove(path);
	remove("test_accounts.db-wal");
	remove("test_accounts.db-shm");
}

static void TestNicknames()
{
	AccountDB db;
	CHECK(db.Open(":memory:"));

	LoginRules strict;
	LoginSession first;
	LoginSession second;
	CHECK(Login(db, L"one", L"pw", strict, &first) == LoginOutcome::SUCCESS);
	CHECK(Login(db, L"two", L"pw", strict, &second) == LoginOutcome::SUCCESS);
	CHECK(first.nickname.empty());

	CHECK(db.CheckNickname(L"Delta", 5) == NicknameResult::OK);
	CHECK(db.CheckNickname(L"D", 1) == NicknameResult::INVALID_FORMAT);
	CHECK(db.CheckNickname(L"two words", 9) == NicknameResult::INVALID_FORMAT);
	CHECK(db.CheckNickname(L"name@host", 9) == NicknameResult::INVALID_FORMAT);
	CHECK(db.CheckNickname(L"01234567890123456", 17) == NicknameResult::INVALID_FORMAT);

	CHECK(db.SetNickname(first.accountUID, L"Delta", 5) == NicknameResult::OK);
	CHECK(db.CheckNickname(L"delta", 5) == NicknameResult::TAKEN);
	CHECK(db.SetNickname(first.accountUID, L"Delta", 5) == NicknameResult::OK);
	CHECK(db.SetNickname(second.accountUID, L"DELTA", 5) == NicknameResult::TAKEN);
	CHECK(db.SetNickname(second.accountUID, L"Ark", 3) == NicknameResult::OK);

	LoginSession again;
	CHECK(Login(db, L"one", L"pw", strict, &again) == LoginOutcome::SUCCESS);
	CHECK(again.nickname == L"Delta");

	LoginSession found;
	CHECK(db.FindSession(L"two", 3, &found));
	CHECK(found.nickname == L"Ark");
	CHECK(db.FindSessionByNickname(L"ark", 3, &found));
	CHECK(found.accountUID == second.accountUID);
	CHECK(!db.FindSessionByNickname(L"nobody", 6, &found));

	LoginRules dev;
	dev.requirePassword = false;
	dev.loginNameAsNickname = true;
	LoginSession named;
	CHECK(Login(db, L"devuser", L"", dev, &named) == LoginOutcome::SUCCESS);
	CHECK(named.nickname == L"devuser");
	LoginSession clash;
	CHECK(Login(db, L"ark", L"", dev, &clash) == LoginOutcome::SUCCESS);
	CHECK(clash.nickname.empty());

	db.Close();
}

static void TestGamePlayers()
{
	SqliteGameStore db;
	CHECK(db.Open(":memory:"));

	const ProfileSeed seed;
	PlayerProfile profile;
	CHECK(db.Begin());
	CHECK(db.LoadProfile(AccountUID(7), 1000, seed, &profile));
	CHECK(db.Commit());
	CHECK(profile.firstEntry);
	CHECK(profile.createdAt == 1000);

	CHECK(db.Begin());
	CHECK(db.LoadProfile(AccountUID(7), 2000, seed, &profile));
	CHECK(db.Commit());
	CHECK(!profile.firstEntry);
	CHECK(profile.createdAt == 1000);
	CHECK(profile.lastLoginAt == 2000);

	db.Close();
}

static void TestSessionReadOnly()
{
	const char* path = "test_sessions.db";
	remove(path);
	remove("test_sessions.db-wal");
	remove("test_sessions.db-shm");

	{
		AccountDB writer;
		CHECK(writer.Open(path));

		AccountDB reader;
		CHECK(reader.OpenReadOnly(path));

		LoginRules strict;
		LoginSession session;
		CHECK(Login(writer, L"delta", L"pw", strict, &session) == LoginOutcome::SUCCESS);

		LoginSession seen;
		CHECK(reader.FindSession(L"delta", 5, &seen));
		CHECK(seen.accountUID == session.accountUID);
		CHECK(seen.instantKey == session.instantKey);

		LoginSession next;
		CHECK(Login(writer, L"delta", L"pw", strict, &next) == LoginOutcome::SUCCESS);
		CHECK(reader.FindSession(L"delta", 5, &seen));
		CHECK(seen.instantKey == next.instantKey);

		reader.Close();
		writer.Close();
	}

	remove(path);
	remove("test_sessions.db-wal");
	remove("test_sessions.db-shm");
}

int main(int argc, char** argv)
{
	PlatformInit();
	LogInit("test_accounts.log");

	TestPasswordRules();
	TestDevRules();
	TestPersistence();
	TestNicknames();
	TestGamePlayers();
	TestSessionReadOnly();

	if(failures > 0) {
		LOG("test_accounts: %d check(s) failed", failures);
		return 1;
	}
	LOG("test_accounts: all checks passed");
	return 0;
}
