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
	bool SaveSettings(AccountUID accountUID, i32 settingType, const u8* data, i32 size, i64 now) override;
	bool SetStatusMessage(AccountUID accountUID, const wchar* msg, i32 len, i64 at) override;
	bool SetLastLogout(AccountUID accountUID, i64 at) override;

private:
	bool SeedMasters(AccountUID accountUID, i64 now, const ProfileSeed& seed);
	bool LoadMasters(AccountUID accountUID, PlayerProfile* out);
	bool LoadSettings(AccountUID accountUID, PlayerProfile* out);
};
