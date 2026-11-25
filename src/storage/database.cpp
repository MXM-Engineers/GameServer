#include "database.h"
#include <sqlite3.h>

bool Database::Open(const char* path)
{
	ASSERT(handle == nullptr);

	const i32 flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
	if(sqlite3_open_v2(path, &handle, flags, nullptr) != SQLITE_OK) {
		LOG("ERROR(Database::Open): '%s': %s", path, handle ? sqlite3_errmsg(handle) : "out of memory");
		Close();
		return false;
	}

	sqlite3_busy_timeout(handle, 5000);

	if(!Exec("PRAGMA journal_mode=WAL")) return false;
	if(!Exec("PRAGMA synchronous=NORMAL")) return false;
	if(!Exec("PRAGMA foreign_keys=ON")) return false;
	return true;
}

bool Database::OpenReadOnly(const char* path)
{
	ASSERT(handle == nullptr);

	const i32 flags = SQLITE_OPEN_READONLY | SQLITE_OPEN_FULLMUTEX;
	if(sqlite3_open_v2(path, &handle, flags, nullptr) != SQLITE_OK) {
		LOG("ERROR(Database::OpenReadOnly): '%s': %s", path, handle ? sqlite3_errmsg(handle) : "out of memory");
		Close();
		return false;
	}

	sqlite3_busy_timeout(handle, 5000);
	return true;
}

void Database::Close()
{
	if(handle) {
		sqlite3_close(handle);
		handle = nullptr;
	}
}

bool Database::Exec(const char* sql)
{
	ASSERT(handle);

	char* error = nullptr;
	if(sqlite3_exec(handle, sql, nullptr, nullptr, &error) != SQLITE_OK) {
		LOG("ERROR(Database::Exec): %s :: %s", error ? error : "unknown", sql);
		sqlite3_free(error);
		return false;
	}
	return true;
}

i32 Database::SchemaVersion()
{
	Statement st(*this, "PRAGMA user_version");
	const StepResult r = st.Step();
	ASSERT(r == StepResult::ROW);
	return (i32)st.ColumnInt(0);
}

bool Database::Migrate(const char* const* steps, i32 stepCount)
{
	const i32 version = SchemaVersion();
	if(version > stepCount) {
		LOG("ERROR(Database::Migrate): database is at version %d, this build knows %d", version, stepCount);
		return false;
	}

	for(i32 i = version; i < stepCount; i++) {
		if(!Exec("BEGIN IMMEDIATE")) return false;
		if(!Exec(steps[i]) || !Exec(FMT("PRAGMA user_version=%d", i + 1))) {
			Exec("ROLLBACK");
			return false;
		}
		if(!Exec("COMMIT")) return false;
		LOG("Database :: migrated to version %d", i + 1);
	}
	return true;
}

i64 Database::LastInsertRowID() const
{
	return sqlite3_last_insert_rowid(handle);
}

i32 Database::Changes() const
{
	return sqlite3_changes(handle);
}

const char* Database::LastError() const
{
	return sqlite3_errmsg(handle);
}

Statement::Statement(Database& db_, const char* sql):
	db(db_)
{
	ASSERT(db.handle);
	const i32 r = sqlite3_prepare_v2(db.handle, sql, -1, &handle, nullptr);
	if(r != SQLITE_OK) {
		LOG("ERROR(Statement): %s :: %s", sqlite3_errmsg(db.handle), sql);
	}
	ASSERT(r == SQLITE_OK);
}

Statement::~Statement()
{
	sqlite3_finalize(handle);
}

void Statement::BindInt(i32 index, i64 value)
{
	const i32 r = sqlite3_bind_int64(handle, index, value);
	ASSERT(r == SQLITE_OK);
}

void Statement::BindText(i32 index, const char* text)
{
	const i32 r = sqlite3_bind_text(handle, index, text, -1, SQLITE_TRANSIENT);
	ASSERT(r == SQLITE_OK);
}

void Statement::BindWide(i32 index, const wchar* text, i32 len)
{
	STATIC_ASSERT(sizeof(wchar) == 2);
	const i32 r = sqlite3_bind_text16(handle, index, text, len * (i32)sizeof(wchar), SQLITE_TRANSIENT);
	ASSERT(r == SQLITE_OK);
}

void Statement::BindNull(i32 index)
{
	const i32 r = sqlite3_bind_null(handle, index);
	ASSERT(r == SQLITE_OK);
}

StepResult Statement::Step()
{
	const i32 r = sqlite3_step(handle);
	if(r == SQLITE_ROW) return StepResult::ROW;
	if(r == SQLITE_DONE) return StepResult::DONE;
	LOG("ERROR(Statement::Step): %s", sqlite3_errmsg(db.handle));
	return StepResult::FAILED;
}

bool Statement::ColumnIsNull(i32 column)
{
	return sqlite3_column_type(handle, column) == SQLITE_NULL;
}

i64 Statement::ColumnInt(i32 column)
{
	return sqlite3_column_int64(handle, column);
}

const char* Statement::ColumnText(i32 column)
{
	return (const char*)sqlite3_column_text(handle, column);
}

const wchar* Statement::ColumnWide(i32 column, i32* outLen)
{
	const wchar* text = (const wchar*)sqlite3_column_text16(handle, column);
	*outLen = sqlite3_column_bytes16(handle, column) / (i32)sizeof(wchar);
	return text;
}
