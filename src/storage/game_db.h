#pragma once
#include "database.h"
#include "game_store.h"

struct SqliteGameStore: GameStoreBackend
{
	Database db;

	bool Open(const char* path) override;
	void Close() override;

	bool Begin() override;
	bool Commit() override;
	void Rollback() override;

	bool LoadProfile(AccountUID accountUID, i64 now, const ProfileSeed& seed, PlayerProfile* out) override;
};
