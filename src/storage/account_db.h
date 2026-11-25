#pragma once
#include "database.h"
#include <common/inner_protocol.h>
#include <common/utils.h>

enum class LoginOutcome: i32
{
	SUCCESS = 0,
	NOT_REGISTERED,
	INCORRECT_PASSWORD,
	BANNED,
	DATABASE_FAILURE
};

enum class NicknameResult: i32
{
	OK = 0,
	INVALID_FORMAT,
	TAKEN,
	DATABASE_FAILURE
};

struct LoginRules
{
	bool autoCreateAccounts = true;
	bool requirePassword = true;
	bool loginNameAsNickname = false;
};

struct LoginSession
{
	AccountUID accountUID = AccountUID::INVALID;
	u32 instantKey = 0;
	bool created = false;
	WideString nickname;
};

struct AccountDB
{
	enum {
		MAX_LOGIN_NAME_LEN = 32,
		MAX_PASSWORD_LEN = 128,
		MIN_NICKNAME_LEN = 2,
		MAX_NICKNAME_LEN = 16
	};

	Database db;
	Mutex mutex;

	bool Open(const char* path);
	bool OpenReadOnly(const char* path);
	void Close();

	LoginOutcome Login(const wchar* loginName, i32 loginNameLen, const wchar* password, i32 passwordLen, const char* clientIp, const LoginRules& rules, LoginSession* out);
	bool FindSession(const wchar* loginName, i32 loginNameLen, LoginSession* out);
	bool FindSessionByNickname(const wchar* nickname, i32 nicknameLen, LoginSession* out);
	bool SetBanned(const wchar* loginName, i32 loginNameLen, bool banned);

	NicknameResult CheckNickname(const wchar* nickname, i32 nicknameLen);
	NicknameResult SetNickname(AccountUID accountUID, const wchar* nickname, i32 nicknameLen);

private:
	NicknameResult SetNicknameLocked(AccountUID accountUID, const wchar* nickname, i32 nicknameLen);
};
