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
	");",

	"CREATE TABLE player_settings("
	"	account_uid INTEGER NOT NULL,"
	"	setting_type INTEGER NOT NULL,"
	"	data BLOB NOT NULL,"
	"	updated_at INTEGER NOT NULL,"
	"	PRIMARY KEY(account_uid, setting_type)"
	");"
	"ALTER TABLE players ADD COLUMN tutorial_state INTEGER NOT NULL DEFAULT -1;"
	"ALTER TABLE players ADD COLUMN status_message TEXT NOT NULL DEFAULT '';"
	"ALTER TABLE players ADD COLUMN status_message_at INTEGER NOT NULL DEFAULT 0;"
	"ALTER TABLE players ADD COLUMN last_logout_at INTEGER;"
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
		Statement select(db, "SELECT created_at, last_login_at, leader_class, leader_skin, tutorial_state, status_message, status_message_at, last_logout_at FROM players WHERE account_uid=?1");
		select.BindInt(1, (u32)accountUID);
		if(select.Step() != StepResult::ROW) return false;
		out->createdAt = select.ColumnInt(0);
		out->lastLoginAt = select.ColumnInt(1);
		out->leaderClass = select.ColumnIsNull(2) ? ClassType::NONE : (ClassType)select.ColumnInt(2);
		out->leaderSkin = (SkinIndex)select.ColumnInt(3);
		out->tutorialState = (i32)select.ColumnInt(4);
		i32 msgLen = 0;
		const wchar* msg = select.ColumnWide(5, &msgLen);
		if(msg && msgLen > 0) out->statusMessage.assign(msg, msgLen);
		out->statusMessageAt = select.ColumnInt(6);
		out->lastLogoutAt = select.ColumnIsNull(7) ? 0 : select.ColumnInt(7);
	}

	if(!LoadMasters(accountUID, out)) return false;
	return LoadSettings(accountUID, out);
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

bool SqliteGameStore::LoadSettings(AccountUID accountUID, PlayerProfile* out)
{
	Statement select(db, "SELECT setting_type, data FROM player_settings WHERE account_uid=?1 ORDER BY setting_type");
	select.BindInt(1, (u32)accountUID);
	StepResult r;
	while((r = select.Step()) == StepResult::ROW) {
		const i64 settingType = select.ColumnInt(0);
		i32 size = 0;
		const u8* data = select.ColumnBlob(1, &size);
		if(PlayerProfile::CheckSettingsSave((i32)settingType, size) != SettingsSaveCheck::OK || settingType != (i32)settingType) {
			LOG("ERROR(GameStore): account %u has an invalid settings row (type=%lld size=%d)", (u32)accountUID, (long long)settingType, size);
			return false;
		}
		out->SetSettings((i32)settingType, data, size);
	}
	return r == StepResult::DONE;
}

bool SqliteGameStore::SaveSettings(AccountUID accountUID, i32 settingType, const u8* data, i32 size, i64 now)
{
	ASSERT(accountUID != AccountUID::INVALID);
	if(PlayerProfile::CheckSettingsSave(settingType, size) != SettingsSaveCheck::OK) return false;

	Statement upsert(db, "INSERT INTO player_settings(account_uid, setting_type, data, updated_at) VALUES(?1, ?2, ?3, ?4) "
		"ON CONFLICT(account_uid, setting_type) DO UPDATE SET data=excluded.data, updated_at=excluded.updated_at");
	upsert.BindInt(1, (u32)accountUID);
	upsert.BindInt(2, settingType);
	upsert.BindBlob(3, data, size);
	upsert.BindInt(4, now);
	return upsert.Step() == StepResult::DONE && db.Changes() == 1;
}

bool SqliteGameStore::SetStatusMessage(AccountUID accountUID, const wchar* msg, i32 len, i64 at)
{
	ASSERT(accountUID != AccountUID::INVALID);
	if(len < 0 || len > STATUS_MESSAGE_MAX_LEN) return false;

	Statement update(db, "UPDATE players SET status_message=?2, status_message_at=?3 WHERE account_uid=?1");
	update.BindInt(1, (u32)accountUID);
	if(len > 0) update.BindWide(2, msg, len);
	else update.BindText(2, "");
	update.BindInt(3, at);
	return update.Step() == StepResult::DONE && db.Changes() == 1;
}

bool SqliteGameStore::SetLastLogout(AccountUID accountUID, i64 at)
{
	ASSERT(accountUID != AccountUID::INVALID);

	Statement update(db, "UPDATE players SET last_logout_at=?2 WHERE account_uid=?1");
	update.BindInt(1, (u32)accountUID);
	update.BindInt(2, at);
	return update.Step() == StepResult::DONE && db.Changes() == 1;
}
