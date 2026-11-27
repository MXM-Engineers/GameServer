#pragma once
#include <common/base.h>

struct sqlite3;
struct sqlite3_stmt;

struct Database
{
	sqlite3* handle = nullptr;

	bool Open(const char* path);
	bool OpenReadOnly(const char* path);
	void Close();
	bool Exec(const char* sql);
	bool Migrate(const char* const* steps, i32 stepCount);
	i32 SchemaVersion();
	i64 LastInsertRowID() const;
	i32 Changes() const;
	bool InTransaction() const;
	const char* LastError() const;
};

enum class StepResult: i32
{
	ROW = 0,
	DONE,
	FAILED
};

struct Statement
{
	Database& db;
	sqlite3_stmt* handle = nullptr;

	Statement(Database& db_, const char* sql);
	~Statement();

	void BindInt(i32 index, i64 value);
	void BindText(i32 index, const char* text);
	void BindWide(i32 index, const wchar* text, i32 len);
	void BindNull(i32 index);
	void BindBlob(i32 index, const void* data, i32 size);
	void BindDouble(i32 index, f64 value);

	StepResult Step();

	bool ColumnIsNull(i32 column);
	i64 ColumnInt(i32 column);
	const char* ColumnText(i32 column);
	const wchar* ColumnWide(i32 column, i32* outLen);
	const u8* ColumnBlob(i32 column, i32* outSize);
	f64 ColumnDouble(i32 column);

private:
	Statement(const Statement&);
	const Statement& operator=(const Statement&);
};
