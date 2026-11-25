#pragma once
#include "database.h"
#include <common/inner_protocol.h>

struct GameDB
{
	Database db;
	Mutex mutex;

	bool Open(const char* path);
	void Close();

	bool TouchPlayer(AccountUID accountUID, bool* outCreated);
};
