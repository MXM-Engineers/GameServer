#include "game_db.h"

static const char* const g_GameMigrations[] = {
	"CREATE TABLE players("
	"	account_uid INTEGER PRIMARY KEY,"
	"	created_at INTEGER NOT NULL,"
	"	last_login_at INTEGER"
	");",

	"ALTER TABLE players ADD COLUMN leader_class INTEGER;"
	"ALTER TABLE players ADD COLUMN leader_skin INTEGER NOT NULL DEFAULT 0;"
	"CREATE TABLE player_masters("
	"	account_uid INTEGER NOT NULL,"
	"	class_type INTEGER NOT NULL,"
	"	skin_index INTEGER NOT NULL DEFAULT 0,"
	"	unlocked_at INTEGER NOT NULL,"
	"	PRIMARY KEY(account_uid, class_type)"
	");"
	"CREATE TABLE player_skins("
	"	account_uid INTEGER NOT NULL,"
	"	class_type INTEGER NOT NULL,"
	"	skin_index INTEGER NOT NULL,"
	"	expire_at INTEGER NOT NULL DEFAULT 0,"
	"	unlocked_at INTEGER NOT NULL,"
	"	PRIMARY KEY(account_uid, class_type, skin_index)"
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

	if(!SeedMasters(accountUID, now, seed)) return false;

	{
		Statement select(db, "SELECT created_at, last_login_at, leader_class, leader_skin FROM players WHERE account_uid=?1");
		select.BindInt(1, (u32)accountUID);
		if(select.Step() != StepResult::ROW) return false;
		out->createdAt = select.ColumnInt(0);
		out->lastLoginAt = select.ColumnInt(1);
		out->leaderClass = select.ColumnIsNull(2) ? ClassType::NONE : (ClassType)select.ColumnInt(2);
		out->leaderSkin = (SkinIndex)select.ColumnInt(3);
	}

	return LoadMasters(accountUID, out);
}

bool SqliteGameStore::SeedMasters(AccountUID accountUID, i64 now, const ProfileSeed& seed)
{
	{
		Statement count(db, "SELECT COUNT(*) FROM player_masters WHERE account_uid=?1");
		count.BindInt(1, (u32)accountUID);
		if(count.Step() != StepResult::ROW) return false;
		if(count.ColumnInt(0) > 0) return true;
	}

	if(seed.masters.empty()) return true;

	foreach_const(it, seed.masters) {
		Statement insert(db, "INSERT INTO player_masters(account_uid, class_type, unlocked_at) VALUES(?1, ?2, ?3) ON CONFLICT(account_uid, class_type) DO NOTHING");
		insert.BindInt(1, (u32)accountUID);
		insert.BindInt(2, (i32)*it);
		insert.BindInt(3, now);
		if(insert.Step() != StepResult::DONE) return false;
	}

	foreach_const(it, seed.skins) {
		Statement insert(db, "INSERT INTO player_skins(account_uid, class_type, skin_index, unlocked_at) VALUES(?1, ?2, ?3, ?4) ON CONFLICT(account_uid, class_type, skin_index) DO NOTHING");
		insert.BindInt(1, (u32)accountUID);
		insert.BindInt(2, (i32)it->classType);
		insert.BindInt(3, (i32)it->skinIndex);
		insert.BindInt(4, now);
		if(insert.Step() != StepResult::DONE) return false;
	}

	Statement leader(db, "UPDATE players SET leader_class=?2, leader_skin=0 WHERE account_uid=?1 AND leader_class IS NULL");
	leader.BindInt(1, (u32)accountUID);
	leader.BindInt(2, (i32)seed.leaderClass);
	return leader.Step() == StepResult::DONE;
}

bool SqliteGameStore::LoadMasters(AccountUID accountUID, PlayerProfile* out)
{
	out->masters.clear();
	out->skins.clear();

	{
		Statement select(db, "SELECT class_type, skin_index FROM player_masters WHERE account_uid=?1 ORDER BY class_type");
		select.BindInt(1, (u32)accountUID);
		StepResult r;
		while((r = select.Step()) == StepResult::ROW) {
			ProfileMaster master;
			master.classType = (ClassType)select.ColumnInt(0);
			master.skinIndex = (SkinIndex)select.ColumnInt(1);
			out->masters.push_back(master);
		}
		if(r != StepResult::DONE) return false;
	}

	Statement select(db, "SELECT class_type, skin_index, expire_at FROM player_skins WHERE account_uid=?1 ORDER BY class_type, skin_index");
	select.BindInt(1, (u32)accountUID);
	StepResult r;
	while((r = select.Step()) == StepResult::ROW) {
		ProfileSkin skin;
		skin.classType = (ClassType)select.ColumnInt(0);
		skin.skinIndex = (SkinIndex)select.ColumnInt(1);
		skin.expireAt = select.ColumnInt(2);
		out->skins.push_back(skin);
	}
	return r == StepResult::DONE;
}

bool SqliteGameStore::SetLeader(AccountUID accountUID, ClassType classType, SkinIndex skinIndex)
{
	ASSERT(accountUID != AccountUID::INVALID);

	{
		Statement owned(db, "SELECT 1 FROM player_skins WHERE account_uid=?1 AND class_type=?2 AND skin_index=?3");
		owned.BindInt(1, (u32)accountUID);
		owned.BindInt(2, (i32)classType);
		owned.BindInt(3, (i32)skinIndex);
		if(owned.Step() != StepResult::ROW) return false;
	}

	{
		Statement master(db, "UPDATE player_masters SET skin_index=?3 WHERE account_uid=?1 AND class_type=?2");
		master.BindInt(1, (u32)accountUID);
		master.BindInt(2, (i32)classType);
		master.BindInt(3, (i32)skinIndex);
		if(master.Step() != StepResult::DONE || db.Changes() != 1) return false;
	}

	Statement leader(db, "UPDATE players SET leader_class=?2, leader_skin=?3 WHERE account_uid=?1");
	leader.BindInt(1, (u32)accountUID);
	leader.BindInt(2, (i32)classType);
	leader.BindInt(3, (i32)skinIndex);
	return leader.Step() == StepResult::DONE && db.Changes() == 1;
}
