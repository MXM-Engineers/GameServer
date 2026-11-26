#include "game_db.h"

static const char* const g_GameMigrations[] = {
	"CREATE TABLE players("
	"	account_uid INTEGER PRIMARY KEY,"
	"	created_at INTEGER NOT NULL,"
	"	last_login_at INTEGER"
	");"
};

bool SqliteGameStore::Open(const char* path)
{
	if(!db.Open(path)) return false;
	return db.Migrate(g_GameMigrations, ARRAY_COUNT(g_GameMigrations));
}

void SqliteGameStore::Close()
{
	db.Close();
}

bool SqliteGameStore::Begin()
{
	ASSERT(!db.InTransaction());
	return db.Exec("BEGIN IMMEDIATE");
}

bool SqliteGameStore::Commit()
{
	return db.Exec("COMMIT");
}

void SqliteGameStore::Rollback()
{
	if(db.InTransaction()) db.Exec("ROLLBACK");
	ASSERT(!db.InTransaction());
}

bool SqliteGameStore::LoadProfile(AccountUID accountUID, i64 now, const ProfileSeed& seed, PlayerProfile* out)
{
	ASSERT(accountUID != AccountUID::INVALID);
	(void)seed;

	*out = PlayerProfile();
	out->accountUID = accountUID;

	{
		Statement insert(db, "INSERT INTO players(account_uid, created_at) VALUES(?1, ?2) ON CONFLICT(account_uid) DO NOTHING");
		insert.BindInt(1, (u32)accountUID);
		insert.BindInt(2, now);
		if(insert.Step() != StepResult::DONE) return false;
		out->firstEntry = db.Changes() == 1;
	}

	{
		Statement touch(db, "UPDATE players SET last_login_at=?2 WHERE account_uid=?1");
		touch.BindInt(1, (u32)accountUID);
		touch.BindInt(2, now);
		if(touch.Step() != StepResult::DONE) return false;
	}

	Statement select(db, "SELECT created_at, last_login_at FROM players WHERE account_uid=?1");
	select.BindInt(1, (u32)accountUID);
	if(select.Step() != StepResult::ROW) return false;
	out->createdAt = select.ColumnInt(0);
	out->lastLoginAt = select.ColumnInt(1);
	return true;
}
