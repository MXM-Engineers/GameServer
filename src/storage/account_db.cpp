#include "account_db.h"
#include <common/platform.h>
#include <argon2.h>
#include <time.h>

static const char* const g_AccountMigrations[] = {
	"CREATE TABLE accounts("
	"	account_uid INTEGER PRIMARY KEY AUTOINCREMENT,"
	"	login_name TEXT NOT NULL UNIQUE COLLATE NOCASE,"
	"	password_hash TEXT,"
	"	banned INTEGER NOT NULL DEFAULT 0,"
	"	created_at INTEGER NOT NULL,"
	"	last_login_at INTEGER"
	");"
	"CREATE TABLE login_sessions("
	"	account_uid INTEGER PRIMARY KEY REFERENCES accounts(account_uid),"
	"	instant_key INTEGER NOT NULL,"
	"	client_ip TEXT NOT NULL,"
	"	created_at INTEGER NOT NULL"
	");",

	"ALTER TABLE accounts ADD COLUMN nickname TEXT COLLATE NOCASE;"
	"CREATE UNIQUE INDEX accounts_nickname ON accounts(nickname);"
};

enum {
	ARGON2_TIME_COST = 2,
	ARGON2_MEMORY_KIB = 19456,
	ARGON2_PARALLELISM = 1,
	ARGON2_SALT_LEN = 16,
	ARGON2_HASH_LEN = 32,
	ARGON2_ENCODED_LEN = 128
};

typedef eastl::fixed_string<char,ARGON2_ENCODED_LEN,false> PasswordHash;

static bool IsLoginNameValid(const wchar* loginName, i32 loginNameLen)
{
	if(loginNameLen < 1 || loginNameLen > AccountDB::MAX_LOGIN_NAME_LEN) return false;
	for(i32 i = 0; i < loginNameLen; i++) {
		if(loginName[i] <= 0x20 || loginName[i] == 0x7F) return false;
	}
	return true;
}

static bool IsNicknameValid(const wchar* nickname, i32 nicknameLen)
{
	if(nicknameLen < AccountDB::MIN_NICKNAME_LEN || nicknameLen > AccountDB::MAX_NICKNAME_LEN) return false;
	for(i32 i = 0; i < nicknameLen; i++) {
		if(nickname[i] <= 0x20 || nickname[i] == 0x7F || nickname[i] == L'@') return false;
	}
	return true;
}

static void ReadNickname(Statement& st, i32 column, WideString* out)
{
	out->clear();
	if(st.ColumnIsNull(column)) return;
	i32 len = 0;
	const wchar* text = st.ColumnWide(column, &len);
	out->assign(text, len);
}

static bool HashPassword(const wchar* password, i32 passwordLen, PasswordHash* out)
{
	u8 salt[ARGON2_SALT_LEN];
	if(!PlatformRandomBytes(salt, sizeof(salt))) return false;

	char encoded[ARGON2_ENCODED_LEN];
	const i32 r = argon2id_hash_encoded(ARGON2_TIME_COST, ARGON2_MEMORY_KIB, ARGON2_PARALLELISM, password, passwordLen * sizeof(wchar), salt, sizeof(salt), ARGON2_HASH_LEN, encoded, sizeof(encoded));
	if(r != ARGON2_OK) {
		LOG("ERROR(HashPassword): %s", argon2_error_message(r));
		return false;
	}

	out->assign(encoded);
	return true;
}

static bool VerifyPassword(const char* encoded, const wchar* password, i32 passwordLen)
{
	return argon2id_verify(encoded, password, passwordLen * sizeof(wchar)) == ARGON2_OK;
}

static u32 NewInstantKey()
{
	u32 key = 0;
	while(key == 0) {
		const bool r = PlatformRandomBytes(&key, sizeof(key));
		ASSERT(r);
		key &= 0x7FFFFFFF;
	}
	return key;
}

bool AccountDB::Open(const char* path)
{
	if(!db.Open(path)) return false;
	return db.Migrate(g_AccountMigrations, ARRAY_COUNT(g_AccountMigrations));
}

bool AccountDB::OpenReadOnly(const char* path)
{
	return db.OpenReadOnly(path);
}

void AccountDB::Close()
{
	db.Close();
}

LoginOutcome AccountDB::Login(const wchar* loginName, i32 loginNameLen, const wchar* password, i32 passwordLen, const char* clientIp, const LoginRules& rules, LoginSession* out)
{
	*out = LoginSession();

	if(!IsLoginNameValid(loginName, loginNameLen)) return LoginOutcome::NOT_REGISTERED;
	if(passwordLen < 0 || passwordLen > MAX_PASSWORD_LEN) return LoginOutcome::INCORRECT_PASSWORD;
	if(rules.requirePassword && passwordLen == 0) return LoginOutcome::INCORRECT_PASSWORD;

	LOCK_MUTEX(mutex);

	const i64 now = (i64)time(nullptr);
	i64 accountUID = 0;
	bool created = false;

	WideString nickname;

	{
		Statement find(db, "SELECT account_uid, password_hash, banned, nickname FROM accounts WHERE login_name=?1");
		find.BindWide(1, loginName, loginNameLen);
		const StepResult r = find.Step();
		if(r == StepResult::FAILED) return LoginOutcome::DATABASE_FAILURE;

		if(r == StepResult::ROW) {
			accountUID = find.ColumnInt(0);
			if(find.ColumnInt(2) != 0) return LoginOutcome::BANNED;

			if(rules.requirePassword) {
				if(find.ColumnIsNull(1)) return LoginOutcome::INCORRECT_PASSWORD;
				if(!VerifyPassword(find.ColumnText(1), password, passwordLen)) return LoginOutcome::INCORRECT_PASSWORD;
			}

			ReadNickname(find, 3, &nickname);
		}
	}

	if(accountUID == 0) {
		if(!rules.autoCreateAccounts) return LoginOutcome::NOT_REGISTERED;

		PasswordHash hash;
		if(passwordLen > 0 && !HashPassword(password, passwordLen, &hash)) return LoginOutcome::DATABASE_FAILURE;

		Statement insert(db, "INSERT INTO accounts(login_name, password_hash, created_at) VALUES(?1, ?2, ?3)");
		insert.BindWide(1, loginName, loginNameLen);
		if(passwordLen > 0) insert.BindText(2, hash.c_str());
		else insert.BindNull(2);
		insert.BindInt(3, now);
		if(insert.Step() != StepResult::DONE) return LoginOutcome::DATABASE_FAILURE;

		accountUID = db.LastInsertRowID();
		created = true;
	}

	ASSERT(accountUID > 0 && accountUID <= 0xFFFFFFFFLL);
	const u32 instantKey = NewInstantKey();

	{
		Statement session(db, "INSERT OR REPLACE INTO login_sessions(account_uid, instant_key, client_ip, created_at) VALUES(?1, ?2, ?3, ?4)");
		session.BindInt(1, accountUID);
		session.BindInt(2, instantKey);
		session.BindText(3, clientIp);
		session.BindInt(4, now);
		if(session.Step() != StepResult::DONE) return LoginOutcome::DATABASE_FAILURE;
	}

	{
		Statement touch(db, "UPDATE accounts SET last_login_at=?2 WHERE account_uid=?1");
		touch.BindInt(1, accountUID);
		touch.BindInt(2, now);
		if(touch.Step() != StepResult::DONE) return LoginOutcome::DATABASE_FAILURE;
	}

	if(nickname.empty() && rules.loginNameAsNickname) {
		if(SetNicknameLocked((AccountUID)(u32)accountUID, loginName, loginNameLen) == NicknameResult::OK) {
			nickname.assign(loginName, loginNameLen);
		}
	}

	out->accountUID = (AccountUID)(u32)accountUID;
	out->instantKey = instantKey;
	out->created = created;
	out->nickname = nickname;
	return LoginOutcome::SUCCESS;
}

static bool ReadSession(Statement& find, LoginSession* out)
{
	if(find.Step() != StepResult::ROW) return false;
	out->accountUID = (AccountUID)(u32)find.ColumnInt(0);
	out->instantKey = (u32)find.ColumnInt(1);
	ReadNickname(find, 2, &out->nickname);
	return true;
}

bool AccountDB::FindSession(const wchar* loginName, i32 loginNameLen, LoginSession* out)
{
	*out = LoginSession();
	if(!IsLoginNameValid(loginName, loginNameLen)) return false;

	LOCK_MUTEX(mutex);

	Statement find(db, "SELECT a.account_uid, s.instant_key, a.nickname FROM accounts a JOIN login_sessions s ON s.account_uid=a.account_uid WHERE a.login_name=?1");
	find.BindWide(1, loginName, loginNameLen);
	return ReadSession(find, out);
}

bool AccountDB::FindSessionByNickname(const wchar* nickname, i32 nicknameLen, LoginSession* out)
{
	*out = LoginSession();
	if(!IsNicknameValid(nickname, nicknameLen)) return false;

	LOCK_MUTEX(mutex);

	Statement find(db, "SELECT a.account_uid, s.instant_key, a.nickname FROM accounts a JOIN login_sessions s ON s.account_uid=a.account_uid WHERE a.nickname=?1");
	find.BindWide(1, nickname, nicknameLen);
	return ReadSession(find, out);
}

bool AccountDB::SetBanned(const wchar* loginName, i32 loginNameLen, bool banned)
{
	if(!IsLoginNameValid(loginName, loginNameLen)) return false;

	LOCK_MUTEX(mutex);

	Statement update(db, "UPDATE accounts SET banned=?2 WHERE login_name=?1");
	update.BindWide(1, loginName, loginNameLen);
	update.BindInt(2, banned ? 1 : 0);
	return update.Step() == StepResult::DONE;
}

NicknameResult AccountDB::CheckNickname(const wchar* nickname, i32 nicknameLen)
{
	if(!IsNicknameValid(nickname, nicknameLen)) return NicknameResult::INVALID_FORMAT;

	LOCK_MUTEX(mutex);

	Statement find(db, "SELECT 1 FROM accounts WHERE nickname=?1");
	find.BindWide(1, nickname, nicknameLen);
	const StepResult r = find.Step();
	if(r == StepResult::FAILED) return NicknameResult::DATABASE_FAILURE;
	return r == StepResult::ROW ? NicknameResult::TAKEN : NicknameResult::OK;
}

NicknameResult AccountDB::SetNickname(AccountUID accountUID, const wchar* nickname, i32 nicknameLen)
{
	LOCK_MUTEX(mutex);
	return SetNicknameLocked(accountUID, nickname, nicknameLen);
}

NicknameResult AccountDB::SetNicknameLocked(AccountUID accountUID, const wchar* nickname, i32 nicknameLen)
{
	ASSERT(accountUID != AccountUID::INVALID);
	if(!IsNicknameValid(nickname, nicknameLen)) return NicknameResult::INVALID_FORMAT;

	{
		Statement find(db, "SELECT 1 FROM accounts WHERE nickname=?1 AND account_uid!=?2");
		find.BindWide(1, nickname, nicknameLen);
		find.BindInt(2, (u32)accountUID);
		const StepResult r = find.Step();
		if(r == StepResult::FAILED) return NicknameResult::DATABASE_FAILURE;
		if(r == StepResult::ROW) return NicknameResult::TAKEN;
	}

	Statement update(db, "UPDATE accounts SET nickname=?2 WHERE account_uid=?1");
	update.BindInt(1, (u32)accountUID);
	update.BindWide(2, nickname, nicknameLen);
	if(update.Step() != StepResult::DONE) return NicknameResult::DATABASE_FAILURE;
	if(db.Changes() != 1) return NicknameResult::DATABASE_FAILURE;
	return NicknameResult::OK;
}
