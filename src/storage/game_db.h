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
	bool SetLeader(AccountUID accountUID, ClassType classType, SkinIndex skinIndex) override;

private:
	bool SeedMasters(AccountUID accountUID, i64 now, const ProfileSeed& seed);
	bool LoadMasters(AccountUID accountUID, PlayerProfile* out);
};
