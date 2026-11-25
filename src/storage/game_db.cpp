#include "game_db.h"
#include <time.h>

static const char* const g_GameMigrations[] = {
	"CREATE TABLE players("
	"	account_uid INTEGER PRIMARY KEY,"
	"	created_at INTEGER NOT NULL,"
	"	last_login_at INTEGER"
	");"
};

bool GameDB::Open(const char* path)
{
	if(!db.Open(path)) return false;
	return db.Migrate(g_GameMigrations, ARRAY_COUNT(g_GameMigrations));
}

void GameDB::Close()
{
	db.Close();
}

bool GameDB::TouchPlayer(AccountUID accountUID, bool* outCreated)
{
	ASSERT(accountUID != AccountUID::INVALID);

	LOCK_MUTEX(mutex);

	const i64 now = (i64)time(nullptr);

	{
		Statement insert(db, "INSERT OR IGNORE INTO players(account_uid, created_at) VALUES(?1, ?2)");
		insert.BindInt(1, (u32)accountUID);
		insert.BindInt(2, now);
		if(insert.Step() != StepResult::DONE) return false;
		*outCreated = db.Changes() == 1;
	}

	Statement touch(db, "UPDATE players SET last_login_at=?2 WHERE account_uid=?1");
	touch.BindInt(1, (u32)accountUID);
	touch.BindInt(2, now);
	return touch.Step() == StepResult::DONE;
}
