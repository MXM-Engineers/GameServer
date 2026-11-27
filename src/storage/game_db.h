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

	bool GrantCurrency(AccountUID accountUID, Currency currency, i64 amount, const LedgerNote& note) override;
	bool SpendCurrency(AccountUID accountUID, Currency currency, i64 amount, const LedgerNote& note) override;
	bool AddItem(AccountUID accountUID, const ProfileItem& item, const LedgerNote& note) override;
	bool RemoveItem(AccountUID accountUID, ItemUID uid, i32 count, const LedgerNote& note) override;
	bool MoveItem(AccountUID accountUID, ItemUID uid, InvenType invenType, i32 slot, const LedgerNote& note) override;
	bool SetInventoryLines(AccountUID accountUID, i32 container, i32 lines) override;

private:
	bool SeedMasters(AccountUID accountUID, i64 now, const ProfileSeed& seed);
	bool LoadMasters(AccountUID accountUID, PlayerProfile* out);
	bool LoadSettings(AccountUID accountUID, PlayerProfile* out);
	bool SeedWallet(AccountUID accountUID, i64 now, const ProfileSeed& seed, PlayerProfile* out);
	bool LoadWallet(AccountUID accountUID, PlayerProfile* out);
	bool LoadInventory(AccountUID accountUID, PlayerProfile* out);
	bool ChangeCurrency(AccountUID accountUID, Currency currency, i64 delta, const LedgerNote& note);
	bool WriteLedger(AccountUID accountUID, LedgerAsset asset, i64 delta, i64 balance, ItemDocIndex docIndex, ItemUID uid, const LedgerNote& note);
	bool StoredSlotFree(AccountUID accountUID, InvenType invenType, i32 slot, ItemUID except);
	bool StoredSlotUsable(AccountUID accountUID, InvenType invenType, i32 slot);
	bool ReadLastItemSeq(u32* out);
};
