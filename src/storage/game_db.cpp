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
	"ALTER TABLE players ADD COLUMN last_logout_at INTEGER;",

	"CREATE TABLE wallets("
	"	account_uid INTEGER PRIMARY KEY,"
	"	gold BIGINT NOT NULL CHECK(gold >= 0),"
	"	ccoin BIGINT NOT NULL CHECK(ccoin >= 0)"
	");"
	"CREATE TABLE asset_ledger("
	"	account_uid INTEGER NOT NULL,"
	"	at BIGINT NOT NULL,"
	"	asset INTEGER NOT NULL,"
	"	delta BIGINT NOT NULL,"
	"	balance BIGINT NOT NULL,"
	"	reason INTEGER NOT NULL,"
	"	doc_index BIGINT NOT NULL,"
	"	quantity INTEGER NOT NULL,"
	"	item_uid BIGINT NOT NULL"
	");"
	"CREATE INDEX asset_ledger_account ON asset_ledger(account_uid, at);"
	"CREATE TABLE items("
	"	item_uid BIGINT PRIMARY KEY,"
	"	account_uid INTEGER NOT NULL,"
	"	inven_type INTEGER NOT NULL,"
	"	slot INTEGER NOT NULL,"
	"	doc_index BIGINT NOT NULL,"
	"	count INTEGER NOT NULL CHECK(count > 0),"
	"	property_group INTEGER NOT NULL,"
	"	life_absolute INTEGER NOT NULL,"
	"	life_end_utc BIGINT NOT NULL"
	");"
	"CREATE INDEX items_account ON items(account_uid);"
	"CREATE TABLE item_properties("
	"	item_uid BIGINT NOT NULL,"
	"	ord INTEGER NOT NULL,"
	"	type INTEGER NOT NULL,"
	"	type_detail INTEGER NOT NULL,"
	"	value_type INTEGER NOT NULL,"
	"	value REAL NOT NULL,"
	"	fixed INTEGER NOT NULL,"
	"	PRIMARY KEY(item_uid, ord)"
	");"
	"CREATE TABLE inventory_lines("
	"	account_uid INTEGER NOT NULL,"
	"	container INTEGER NOT NULL,"
	"	lines INTEGER NOT NULL,"
	"	PRIMARY KEY(account_uid, container)"
	");"
	"CREATE TABLE counters("
	"	name VARCHAR(32) PRIMARY KEY,"
	"	value BIGINT NOT NULL"
	");"
};

bool SqliteGameStore::Open(const char* path)
{
	if(!db.Open(path)) return false;
	if(!db.Migrate(g_GameMigrations, ARRAY_COUNT(g_GameMigrations))) return false;

	u32 lastSeq = 0;
	if(!ReadLastItemSeq(&lastSeq)) return false;
	itemUIDs.Reset(lastSeq);
	LOG("GameStore :: item uids continue after sequence %u", lastSeq);
	return true;
}

bool SqliteGameStore::ReadLastItemSeq(u32* out)
{
	i64 last = 0;
	{
		Statement select(db, "SELECT MAX(item_uid) FROM items");
		if(select.Step() != StepResult::ROW) return false;
		if(!select.ColumnIsNull(0)) {
			const i64 uid = select.ColumnInt(0);
			if(!ItemUIDAllocator::IsItemUID((ItemUID)uid) || uid != (i64)(u32)uid) {
				LOG("ERROR(GameStore): items holds an invalid item uid %lld", (long long)uid);
				return false;
			}
			last = ItemUIDAllocator::Seq((ItemUID)uid);
		}
	}

	Statement counter(db, "SELECT value FROM counters WHERE name='item_seq'");
	const StepResult r = counter.Step();
	if(r == StepResult::FAILED) return false;
	if(r == StepResult::ROW) {
		const i64 value = counter.ColumnInt(0);
		if(value < 0 || value > ITEM_UID_MAX_SEQ) {
			LOG("ERROR(GameStore): counters.item_seq is out of range (%lld)", (long long)value);
			return false;
		}
		if(value > last) last = value;
	}

	*out = (u32)last;
	return true;
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
	if(!LoadSettings(accountUID, out)) return false;
	if(!SeedWallet(accountUID, now, seed, out)) return false;
	return LoadWallet(accountUID, out);
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

bool SqliteGameStore::SeedWallet(AccountUID accountUID, i64 now, const ProfileSeed& seed, PlayerProfile* out)
{
	bool exists = false;
	{
		Statement select(db, "SELECT 1 FROM wallets WHERE account_uid=?1");
		select.BindInt(1, (u32)accountUID);
		const StepResult r = select.Step();
		if(r == StepResult::FAILED) return false;
		exists = r == StepResult::ROW;
	}

	if(exists) return LoadInventory(accountUID, out);

	if(seed.gold < 0 || seed.gold > CURRENCY_MAX || seed.ccoin < 0 || seed.ccoin > CURRENCY_MAX) return false;

	{
		Statement insert(db, "INSERT INTO wallets(account_uid, gold, ccoin) VALUES(?1, ?2, ?3)");
		insert.BindInt(1, (u32)accountUID);
		insert.BindInt(2, seed.gold);
		insert.BindInt(3, seed.ccoin);
		if(insert.Step() != StepResult::DONE) return false;
	}

	LedgerNote note;
	note.reason = AssetChangeReason::INITIALIZE;
	note.at = now;
	if(!WriteLedger(accountUID, LedgerAsset::GOLD, seed.gold, seed.gold, ItemDocIndex::INVALID, ItemUID::INVALID, note)) return false;
	if(!WriteLedger(accountUID, LedgerAsset::CCOIN, seed.ccoin, seed.ccoin, ItemDocIndex::INVALID, ItemUID::INVALID, note)) return false;

	for(i32 c = 0; c < INVENTORY_CONTAINERS; c++) {
		if(seed.lines[c] < 1 || seed.lines[c] > ContainerMaxLines(c)) return false;
		Statement insert(db, "INSERT INTO inventory_lines(account_uid, container, lines) VALUES(?1, ?2, ?3) "
			"ON CONFLICT(account_uid, container) DO UPDATE SET lines=excluded.lines");
		insert.BindInt(1, (u32)accountUID);
		insert.BindInt(2, c);
		insert.BindInt(3, seed.lines[c]);
		if(insert.Step() != StepResult::DONE) return false;
	}

	if(!LoadInventory(accountUID, out)) return false;

	eastl::vector<ProfileItem> placed;
	if(!PlaceSeedItems(seed, itemUIDs, out, &placed)) return false;
	foreach_const(it, placed) {
		note.quantity = it->count;
		note.docIndex = it->docIndex;
		if(!AddItem(accountUID, *it, note)) return false;
	}
	return true;
}

bool SqliteGameStore::LoadWallet(AccountUID accountUID, PlayerProfile* out)
{
	Statement select(db, "SELECT gold, ccoin FROM wallets WHERE account_uid=?1");
	select.BindInt(1, (u32)accountUID);
	if(select.Step() != StepResult::ROW) return false;
	out->gold = select.ColumnInt(0);
	out->ccoin = select.ColumnInt(1);
	return out->gold >= 0 && out->gold <= CURRENCY_MAX && out->ccoin >= 0 && out->ccoin <= CURRENCY_MAX;
}

bool SqliteGameStore::LoadInventory(AccountUID accountUID, PlayerProfile* out)
{
	out->lines = PlayerProfile().lines;
	out->items.clear();

	{
		Statement select(db, "SELECT container, lines FROM inventory_lines WHERE account_uid=?1");
		select.BindInt(1, (u32)accountUID);
		StepResult r;
		while((r = select.Step()) == StepResult::ROW) {
			const i64 container = select.ColumnInt(0);
			const i64 lines = select.ColumnInt(1);
			if(container < 0 || container >= INVENTORY_CONTAINERS || lines < 1 || lines > ContainerMaxLines((i32)container)) {
				LOG("ERROR(GameStore): account %u has an invalid inventory_lines row (container=%lld lines=%lld)", (u32)accountUID, (long long)container, (long long)lines);
				return false;
			}
			out->lines[(i32)container] = (i32)lines;
		}
		if(r != StepResult::DONE) return false;
	}

	{
		Statement select(db, "SELECT item_uid, inven_type, slot, doc_index, count, property_group, life_absolute, life_end_utc FROM items WHERE account_uid=?1 ORDER BY item_uid");
		select.BindInt(1, (u32)accountUID);
		StepResult r;
		while((r = select.Step()) == StepResult::ROW) {
			ProfileItem item;
			const i64 uid = select.ColumnInt(0);
			item.uid = (ItemUID)(u32)uid;
			item.invenType = (InvenType)select.ColumnInt(1);
			item.slot = (i32)select.ColumnInt(2);
			item.docIndex = (ItemDocIndex)(u32)select.ColumnInt(3);
			item.count = (i32)select.ColumnInt(4);
			item.propertyGroup = (i32)select.ColumnInt(5);
			item.lifeAbsolute = (u8)select.ColumnInt(6);
			item.lifeEndUTC = select.ColumnInt(7);
			if(uid != (i64)(u32)uid || out->CheckAddItem(item) != ItemCheck::OK) {
				LOG("ERROR(GameStore): account %u has an invalid items row (uid=%lld invenType=%d slot=%d check=%d)", (u32)accountUID, (long long)uid, (i32)item.invenType, item.slot, (i32)out->CheckAddItem(item));
				return false;
			}
			out->items.push_back(item);
		}
		if(r != StepResult::DONE) return false;
	}

	Statement select(db, "SELECT p.item_uid, p.type, p.type_detail, p.value_type, p.value, p.fixed FROM item_properties p "
		"JOIN items i ON i.item_uid=p.item_uid WHERE i.account_uid=?1 ORDER BY p.item_uid, p.ord");
	select.BindInt(1, (u32)accountUID);
	StepResult r;
	while((r = select.Step()) == StepResult::ROW) {
		ProfileItem* item = out->FindItem((ItemUID)(u32)select.ColumnInt(0));
		if(!item) return false;
		ItemProperty prop;
		prop.type = (u8)select.ColumnInt(1);
		prop.typeDetail = (i32)select.ColumnInt(2);
		prop.valueType = (u8)select.ColumnInt(3);
		prop.value = (f32)select.ColumnDouble(4);
		prop.fixed = (u8)select.ColumnInt(5);
		item->properties.push_back(prop);
	}
	return r == StepResult::DONE;
}

bool SqliteGameStore::WriteLedger(AccountUID accountUID, LedgerAsset asset, i64 delta, i64 balance, ItemDocIndex docIndex, ItemUID uid, const LedgerNote& note)
{
	Statement insert(db, "INSERT INTO asset_ledger(account_uid, at, asset, delta, balance, reason, doc_index, quantity, item_uid) "
		"VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9)");
	insert.BindInt(1, (u32)accountUID);
	insert.BindInt(2, note.at);
	insert.BindInt(3, (i32)asset);
	insert.BindInt(4, delta);
	insert.BindInt(5, balance);
	insert.BindInt(6, (i32)note.reason);
	insert.BindInt(7, (u32)docIndex);
	insert.BindInt(8, note.quantity);
	insert.BindInt(9, (u32)uid);
	return insert.Step() == StepResult::DONE;
}

bool SqliteGameStore::ChangeCurrency(AccountUID accountUID, Currency currency, i64 delta, const LedgerNote& note)
{
	ASSERT(accountUID != AccountUID::INVALID);
	const char* column = currency == Currency::GOLD ? "gold" : "ccoin";

	i64 balance = 0;
	{
		Statement select(db, currency == Currency::GOLD ? "SELECT gold FROM wallets WHERE account_uid=?1" : "SELECT ccoin FROM wallets WHERE account_uid=?1");
		select.BindInt(1, (u32)accountUID);
		if(select.Step() != StepResult::ROW) return false;
		balance = select.ColumnInt(0);
	}

	const i64 after = balance + delta;
	if(after < 0 || after > CURRENCY_MAX) {
		LOG("ERROR(GameStore): account %u %s change refused (balance=%lld delta=%lld)", (u32)accountUID, column, (long long)balance, (long long)delta);
		return false;
	}

	{
		Statement update(db, currency == Currency::GOLD ? "UPDATE wallets SET gold=?2 WHERE account_uid=?1" : "UPDATE wallets SET ccoin=?2 WHERE account_uid=?1");
		update.BindInt(1, (u32)accountUID);
		update.BindInt(2, after);
		if(update.Step() != StepResult::DONE || db.Changes() != 1) return false;
	}

	const LedgerAsset asset = currency == Currency::GOLD ? LedgerAsset::GOLD : LedgerAsset::CCOIN;
	return WriteLedger(accountUID, asset, delta, after, note.docIndex, ItemUID::INVALID, note);
}

bool SqliteGameStore::GrantCurrency(AccountUID accountUID, Currency currency, i64 amount, const LedgerNote& note)
{
	if(amount < 0 || amount > CURRENCY_MAX) return false;
	return ChangeCurrency(accountUID, currency, amount, note);
}

bool SqliteGameStore::SpendCurrency(AccountUID accountUID, Currency currency, i64 amount, const LedgerNote& note)
{
	if(amount < 0 || amount > CURRENCY_MAX) return false;
	return ChangeCurrency(accountUID, currency, -amount, note);
}

bool SqliteGameStore::StoredSlotUsable(AccountUID accountUID, InvenType invenType, i32 slot)
{
	const i32 container = ItemContainer(invenType, slot);
	if(container < 0) return false;

	Statement select(db, "SELECT lines FROM inventory_lines WHERE account_uid=?1 AND container=?2");
	select.BindInt(1, (u32)accountUID);
	select.BindInt(2, container);
	if(select.Step() != StepResult::ROW) return false;
	return ContainerSlotIndex(invenType, slot) < select.ColumnInt(0) * ContainerSlotsPerLine(container);
}

bool SqliteGameStore::StoredSlotFree(AccountUID accountUID, InvenType invenType, i32 slot, ItemUID except)
{
	Statement select(db, "SELECT 1 FROM items WHERE account_uid=?1 AND inven_type=?2 AND slot=?3 AND item_uid<>?4");
	select.BindInt(1, (u32)accountUID);
	select.BindInt(2, (i32)invenType);
	select.BindInt(3, slot);
	select.BindInt(4, (u32)except);
	return select.Step() == StepResult::DONE;
}

bool SqliteGameStore::AddItem(AccountUID accountUID, const ProfileItem& item, const LedgerNote& note)
{
	ASSERT(accountUID != AccountUID::INVALID);
	if(!ItemUIDAllocator::IsItemUID(item.uid) || item.docIndex == ItemDocIndex::INVALID || item.count <= 0) return false;
	if(!StoredSlotUsable(accountUID, item.invenType, item.slot)) return false;
	if(!StoredSlotFree(accountUID, item.invenType, item.slot, ItemUID::INVALID)) return false;

	{
		Statement insert(db, "INSERT INTO items(item_uid, account_uid, inven_type, slot, doc_index, count, property_group, life_absolute, life_end_utc) "
			"VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9)");
		insert.BindInt(1, (u32)item.uid);
		insert.BindInt(2, (u32)accountUID);
		insert.BindInt(3, (i32)item.invenType);
		insert.BindInt(4, item.slot);
		insert.BindInt(5, (u32)item.docIndex);
		insert.BindInt(6, item.count);
		insert.BindInt(7, item.propertyGroup);
		insert.BindInt(8, item.lifeAbsolute);
		insert.BindInt(9, item.lifeEndUTC);
		if(insert.Step() != StepResult::DONE) return false;
	}

	for(i32 i = 0; i < (i32)item.properties.size(); i++) {
		const ItemProperty& prop = item.properties[i];
		Statement insert(db, "INSERT INTO item_properties(item_uid, ord, type, type_detail, value_type, value, fixed) VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7)");
		insert.BindInt(1, (u32)item.uid);
		insert.BindInt(2, i);
		insert.BindInt(3, prop.type);
		insert.BindInt(4, prop.typeDetail);
		insert.BindInt(5, prop.valueType);
		insert.BindDouble(6, prop.value);
		insert.BindInt(7, prop.fixed);
		if(insert.Step() != StepResult::DONE) return false;
	}

	{
		Statement counter(db, "INSERT INTO counters(name, value) VALUES('item_seq', ?1) "
			"ON CONFLICT(name) DO UPDATE SET value=excluded.value WHERE excluded.value > counters.value");
		counter.BindInt(1, ItemUIDAllocator::Seq(item.uid));
		if(counter.Step() != StepResult::DONE) return false;
	}

	return WriteLedger(accountUID, LedgerAsset::ITEM, item.count, item.count, item.docIndex, item.uid, note);
}

bool SqliteGameStore::RemoveItem(AccountUID accountUID, ItemUID uid, i32 count, const LedgerNote& note)
{
	ASSERT(accountUID != AccountUID::INVALID);

	i64 stack = 0;
	ItemDocIndex docIndex;
	{
		Statement select(db, "SELECT count, doc_index FROM items WHERE item_uid=?1 AND account_uid=?2");
		select.BindInt(1, (u32)uid);
		select.BindInt(2, (u32)accountUID);
		if(select.Step() != StepResult::ROW) return false;
		stack = select.ColumnInt(0);
		docIndex = (ItemDocIndex)(u32)select.ColumnInt(1);
	}
	if(count <= 0 || count > stack) return false;

	const i64 remaining = stack - count;
	if(remaining == 0) {
		{
			Statement del(db, "DELETE FROM item_properties WHERE item_uid=?1");
			del.BindInt(1, (u32)uid);
			if(del.Step() != StepResult::DONE) return false;
		}
		Statement del(db, "DELETE FROM items WHERE item_uid=?1 AND account_uid=?2");
		del.BindInt(1, (u32)uid);
		del.BindInt(2, (u32)accountUID);
		if(del.Step() != StepResult::DONE || db.Changes() != 1) return false;
	}
	else {
		Statement update(db, "UPDATE items SET count=?3 WHERE item_uid=?1 AND account_uid=?2");
		update.BindInt(1, (u32)uid);
		update.BindInt(2, (u32)accountUID);
		update.BindInt(3, remaining);
		if(update.Step() != StepResult::DONE || db.Changes() != 1) return false;
	}

	return WriteLedger(accountUID, LedgerAsset::ITEM, -count, remaining, docIndex, uid, note);
}

bool SqliteGameStore::MoveItem(AccountUID accountUID, ItemUID uid, InvenType invenType, i32 slot, const LedgerNote& note)
{
	ASSERT(accountUID != AccountUID::INVALID);

	i64 stack = 0;
	ItemDocIndex docIndex;
	{
		Statement select(db, "SELECT count, doc_index FROM items WHERE item_uid=?1 AND account_uid=?2");
		select.BindInt(1, (u32)uid);
		select.BindInt(2, (u32)accountUID);
		if(select.Step() != StepResult::ROW) return false;
		stack = select.ColumnInt(0);
		docIndex = (ItemDocIndex)(u32)select.ColumnInt(1);
	}

	if(!StoredSlotUsable(accountUID, invenType, slot)) return false;
	if(!StoredSlotFree(accountUID, invenType, slot, uid)) return false;

	{
		Statement update(db, "UPDATE items SET inven_type=?3, slot=?4 WHERE item_uid=?1 AND account_uid=?2");
		update.BindInt(1, (u32)uid);
		update.BindInt(2, (u32)accountUID);
		update.BindInt(3, (i32)invenType);
		update.BindInt(4, slot);
		if(update.Step() != StepResult::DONE || db.Changes() != 1) return false;
	}

	return WriteLedger(accountUID, LedgerAsset::ITEM, 0, stack, docIndex, uid, note);
}

bool SqliteGameStore::SetInventoryLines(AccountUID accountUID, i32 container, i32 lines)
{
	ASSERT(accountUID != AccountUID::INVALID);
	if(container < 0 || container >= INVENTORY_CONTAINERS) return false;
	if(lines < 1 || lines > ContainerMaxLines(container)) return false;

	Statement update(db, "UPDATE inventory_lines SET lines=?3 WHERE account_uid=?1 AND container=?2 AND lines<=?3");
	update.BindInt(1, (u32)accountUID);
	update.BindInt(2, container);
	update.BindInt(3, lines);
	return update.Step() == StepResult::DONE && db.Changes() == 1;
}
