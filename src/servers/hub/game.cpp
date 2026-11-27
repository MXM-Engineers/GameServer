#include "game.h"
#include "coordinator.h" // account data
#include "instance.h"
#include <mxm/game_content.h>
#include "config.h"
#include <EAStdC/EAString.h>
#include <common/utils.h>
#include <time.h>
#include <EASTL/sort.h>

static const ClassType DEFAULT_LEADER_CLASS = ClassType::LAUNCHER;


void HubGame::Init(Server* server_, const ClientLocalMapping* plidMap_)
{
	plidMap = plidMap_;
	replication.Init(server_);
	replication.plidMap = plidMap_;
	world.Init(&replication);

	matchmaker = &Matchmaker();

	playerActorUID.fill(ActorUID::INVALID);

	playerMap.fill(playerList.end());

	LoadMap();
}

void HubGame::Update(Time localTime_)
{
	ProfileFunction();

	localTime = localTime_;
	world.Update(localTime);
	replication.FrameEnd();
}

ErrorType HubGame::JukeboxQueueSong(i32 userID, SongID songID)
{
	ASSERT(playerAccountData[userID]);
	const ClientHandle clientHd = playerMap[userID]->clientHd;

	WorldHub::ActorJukebox& jukebox = world.jukebox;
	ASSERT(jukebox.UID != ActorUID::INVALID);

	const GameXmlContent::Song* xmlSong = GetGameXmlContent().FindJukeboxSongByID(songID);
	if(!xmlSong) {
		WARN("[client%x] HubGame :: jukebox song %d is unknown", (u32)clientHd, (i32)songID);
		return ErrorType::JUKEBOX_NOT_IN_TOWN;
	}

	if(jukebox.queue.full()) return ErrorType::JUKEBOX_QUEUE_FULL;

	const WideString& nick = playerAccountData[userID]->nickname;
	bool found = jukebox.currentSong.requesterNick.compare(nick) == 0;
	if(!found) {
		foreach(it, jukebox.queue) {
			if(it->requesterNick.compare(nick) == 0) {
				found = true;
				break;
			}
		}
	}
	if(found) return ErrorType::JUKEBOX_ALREADY_ENQUEUED;

	WorldHub::ActorJukebox::Song song;
	song.requesterNick = nick;
	song.songID = songID;
	song.lengthInSec = xmlSong->length;
	jukebox.queue.push_back(song);
	return ErrorType::SUCCESS;
}

void HubGame::DevJukeboxQueueSong(i32 userID, SongID songID)
{
	const ClientHandle clientHd = playerMap[userID]->clientHd;
	const ErrorType result = JukeboxQueueSong(userID, songID);
	switch(result) {
		case ErrorType::SUCCESS: break;
		case ErrorType::JUKEBOX_QUEUE_FULL: SendDbgMsg(clientHd, L"Jukebox queue is full"); break;
		case ErrorType::JUKEBOX_ALREADY_ENQUEUED: SendDbgMsg(clientHd, L"Song reservation is limited to 1 per player"); break;
		default: SendDbgMsg(clientHd, LFMT(L"ERROR: Jukebox song not found (%d)", songID)); break;
	}
}

bool HubGame::LoadMap()
{
	const GameXmlContent& content = GetGameXmlContent();

	foreach(it, content.mapLobby.creatures) {
		// don't spawn "spawn points"
		if(it->IsSpawnPoint()) {
			mapSpawnPoints.push_back(SpawnPoint{ it->pos, it->rot });
			continue;
		}

		if(it->docID == CreatureIndex::Jukebox || it->docID == CreatureIndex::HalloweenJukebox) {
			WorldHub::ActorJukebox& actor = world.SpawnJukeboxActor(it->docID, it->localID, it->pos, it->rot);
			actor.spawnAnim = it->spawnAnim;
			actor.wanderDist = it->wanderDist;
			actor.tagID = it->tagID;
			actor.ownerID = it->ownerID;
			actor.dirToNearPC = it->dirToNearPC;
			actor.actionState = it->actionState;
			actor.faction = (i32)it->faction;
			actor.type = it->entityType;
			actor.seed = (i32)RandUint();
		}
		else {
			WorldHub::ActorNpc& actor = world.SpawnNpcActor(it->docID, it->localID);
			actor.pos = it->pos;
			actor.dir = it->rot;
			actor.spawnAnim = it->spawnAnim;
			actor.wanderDist = it->wanderDist;
			actor.tagID = it->tagID;
			actor.ownerID = it->ownerID;
			actor.dirToNearPC = it->dirToNearPC;
			actor.actionState = it->actionState;
			actor.faction = (i32)it->faction;
			actor.type = it->entityType;
			actor.seed = (i32)RandUint();
		}

	}

	return true;
}

void HubGame::OnPlayerConnect(ClientHandle clientHd, Account* accountData)
{
	const i32 userID = plidMap->Get(clientHd);
	playerAccountData[userID] = accountData;
	accountClientHandleMap.emplace(accountData->UID, clientHd);

	playerList.push_back(Player(clientHd));
	playerMap[userID] = --playerList.end();

	replication.SendAccountDataLobby(clientHd, *accountData);

	replication.OnPlayerConnect(clientHd);

	LOG("[client%x] HubGame :: account %llu connected (%ls)", clientHd, (u64)accountData->UID, accountData->nickname.data());
	if(TakeAccountReturningFromGame(accountData->nickname)) {
		LOG("[client%x] HubGame :: back from a game, loading the lobby", clientHd);
		replication.SendLoadLobby(clientHd, (MapIndex)Config().LobbyMap);
	}
}

void HubGame::OnPlayerDisconnect(ClientHandle clientHd)
{
	const i32 userID = plidMap->Get(clientHd);
	LOG("[client%x] GameHub :: OnClientDisconnect :: actorUID=%u", clientHd, (u32)playerActorUID[userID]);

	// we can disconnect before spawning, so test if we have an actor associated
	if(playerActorUID[userID] != ActorUID::INVALID) {
		world.DestroyPlayerActor(playerActorUID[userID]);
	}
	playerActorUID[userID] = ActorUID::INVALID;

	if(playerMap[userID] != playerList.end()) {
		playerList.erase(playerMap[userID]);
	}
	playerMap[userID] = playerList.end();

	accountClientHandleMap.erase(playerAccountData[userID]->UID);
	playerAccountData[userID] = nullptr;

	replication.OnClientDisconnect(clientHd);
}

void HubGame::OnPlayerGetCharacterInfo(ClientHandle clientHd, ActorUID actorUID)
{
	const i32 userID = plidMap->Get(clientHd);

	const WorldHub::ActorPlayer* actor = world.FindPlayerActor(actorUID);
	ASSERT(actor->clientHd == clientHd);
	replication.SendCharacterInfo(clientHd, actor->UID, actor->docID, actor->classType, 100, 100);

}

void HubGame::OnPlayerUpdatePosition(ClientHandle clientHd, ActorUID characterActorUID, const vec3& pos, const vec3& dir, const vec3& eye, f32 rotate, f32 speed, ActionStateID state, i32 actionID)
{
	const i32 userID = plidMap->Get(clientHd);

	// NOTE: the client is not aware that we spawned a new actor for them yet, we ignore this packet
	// LordSk (30/08/2020)
	if(playerActorUID[userID] != characterActorUID) {
		WARN("Client sent an invalid characterID (userID=0x%08x characterID=0x%08x)", userID, (u32)characterActorUID);
		return;
	}

	WorldHub::ActorPlayer* actor = world.FindPlayerActor(playerActorUID[userID]);
	ASSERT(actor);

	// TODO: check for movement hacking
	actor->pos = pos;
	actor->dir = dir;
	actor->eye = eye;
	actor->rotate = rotate;
	actor->speed = speed;
}

void HubGame::OnPlayerChatMessage(ClientHandle clientHd, i32 chatType, const wchar* msg, i32 msgLen)
{
	// command
	if(ParseChatCommand(clientHd, msg, msgLen)) {
		return; // we're done here
	}

	// TODO: chat types
	// TODO: senderStaffType
	// TODO: Actual chat system

	const i32 userID = plidMap->Get(clientHd);
	ASSERT(playerAccountData[userID]);
	replication.SendChatMessageToAll(playerAccountData[userID]->nickname.data(), chatType, msg, msgLen);
}

void HubGame::OnPlayerChatWhisper(ClientHandle clientHd, const wchar* destNick, const wchar* msg)
{
	const i32 userID = plidMap->Get(clientHd);

	ASSERT(playerAccountData[userID]);

	i32 destClientID = -1;
	for(int i = 0; i < playerAccountData.size(); i++) {
		if(playerAccountData[i]) {
			if(playerAccountData[i]->nickname.compare(destNick) == 0) {
				destClientID = i;
				break;
			}
		}
	}

	if(destClientID == -1) {
		replication.SendChatWhisperConfirmToClient(clientHd, destNick, msg, ErrorType::WHISPER_SEND_NOT_FOUND);
		return;
	}

	replication.SendChatWhisperConfirmToClient(clientHd, destNick, msg, ErrorType::SUCCESS);
	replication.SendChatWhisperToClient(playerMap[destClientID]->clientHd, playerAccountData[userID]->nickname.data(), msg);
}

void HubGame::OnPlayerEnterCity(ClientHandle clientHd)
{
	const i32 userID = plidMap->Get(clientHd);
	if(playerActorUID[userID] != ActorUID::INVALID || !playerAccountData[userID]) return;

	const GameXmlContent& content = GetGameXmlContent();
	const PlayerProfile& profile = playerAccountData[userID]->profile;
	ClassType classType = profile.leaderClass;
	SkinIndex skin = profile.leaderSkin;
	if(content.masterClassTypeMap.find(classType) == content.masterClassTypeMap.end() || !profile.FindMaster(classType) || !profile.OwnsSkin(classType, skin)) {
		WARN("[client%x] HubGame :: stored leader (class=%d skin=%d) is not a known owned master, spawning class %d", (u32)clientHd, (i32)classType, (i32)skin, (i32)DEFAULT_LEADER_CLASS);
		classType = DEFAULT_LEADER_CLASS;
		skin = SkinIndex::DEFAULT;
	}

	LOG("[client%x] HubGame :: entered the city without a leader character, spawning the stored one (class=%d skin=%d)", (u32)clientHd, (i32)classType, (i32)skin);
	SpawnLeader(clientHd, classType, skin);

	const WideString& statusMessage = profile.statusMessage;
	if(!statusMessage.empty()) {
		replication.SendCommunityStatusMessage(clientHd, playerAccountData[userID]->nickname.data(), statusMessage.data(), (i32)statusMessage.size());
	}
}

void HubGame::OnPlayerSetLeaderCharacter(ClientHandle clientHd, LocalActorID characterID, SkinIndex skinIndex)
{
	const i32 userID = plidMap->Get(clientHd);
	Account* account = playerAccountData[userID];
	ASSERT(account);
	PlayerProfile& profile = account->profile;

	if(playerActorUID[userID] == ActorUID::INVALID) {
		OnPlayerEnterCity(clientHd);
		return;
	}

	const ClassType classType = (ClassType)((i64)(u32)characterID - (i64)LocalActorID::FIRST_SELF_MASTER);
	const GameXmlContent& content = GetGameXmlContent();
	const bool known = content.masterClassTypeMap.find(classType) != content.masterClassTypeMap.end();
	ProfileMaster* master = profile.FindMaster(classType);

	if(!known || !master || !profile.OwnsSkin(classType, skinIndex)) {
		WARN("[client%x] HubGame :: set leader refused (account=%u characterID=%u skinIndex=%d known=%d ownedMaster=%d)", (u32)clientHd, (u32)account->UID, (u32)characterID, (i32)skinIndex, known, master != nullptr);
		replication.SendSetLeaderRefused(clientHd, ErrorType::SET_LEADER_INVALID, profile.leaderClass, profile.leaderSkin);
		return;
	}

	profile.leaderClass = classType;
	profile.leaderSkin = skinIndex;
	master->skinIndex = skinIndex;

	if(account->profileStored) {
		const AccountUID accountUID = account->UID;
		GetGameStore().QueueWrite("SetLeader", [accountUID, classType, skinIndex](GameStoreBackend& b) {
			return b.SetLeader(accountUID, classType, skinIndex);
		});
	}

	SpawnLeader(clientHd, classType, skinIndex);
}

void HubGame::OnPlayerSaveSettings(ClientHandle clientHd, u8 settingType, const u8* data, i32 size)
{
	const i32 userID = plidMap->Get(clientHd);
	Account* account = playerAccountData[userID];
	ASSERT(account);

	const SettingsSaveCheck check = PlayerProfile::CheckSettingsSave(settingType, size);
	if(check != SettingsSaveCheck::OK) {
		WARN("[client%x] HubGame :: settings save refused (account=%u settingType=%u size=%d check=%d)", (u32)clientHd, (u32)account->UID, settingType, size, (i32)check);
		return;
	}

	account->profile.SetSettings(settingType, data, size);
	LOG("[client%x] HubGame :: settings saved (account=%u settingType=%u size=%d stored=%d)", (u32)clientHd, (u32)account->UID, settingType, size, account->profileStored);

	if(account->profileStored) {
		const AccountUID accountUID = account->UID;
		eastl::vector<u8> blob(data, data + size);
		const i64 now = (i64)time(nullptr);
		GetGameStore().QueueWrite("SaveSettings", [accountUID, settingType, blob, now](GameStoreBackend& b) {
			return b.SaveSettings(accountUID, settingType, blob.data(), (i32)blob.size(), now);
		});
	}
}

void HubGame::OnPlayerCommunityStatusMessage(ClientHandle clientHd, const wchar* msg, i32 len)
{
	const i32 userID = plidMap->Get(clientHd);
	Account* account = playerAccountData[userID];
	ASSERT(account);

	const i64 now = (i64)time(nullptr);
	const ErrorType result = account->profile.CheckStatusMessage(len, now);
	if(result != ErrorType::SUCCESS) {
		WARN("[client%x] HubGame :: status message refused (account=%u len=%d result=%u)", (u32)clientHd, (u32)account->UID, len, (u32)result);
		replication.SendCommunityStatusMessageResult(clientHd, result);
		return;
	}

	account->profile.SetStatusMessage(msg, len, now);

	if(account->profileStored) {
		const AccountUID accountUID = account->UID;
		const WideString text = account->profile.statusMessage;
		GetGameStore().QueueWrite("SetStatusMessage", [accountUID, text, now](GameStoreBackend& b) {
			return b.SetStatusMessage(accountUID, text.data(), (i32)text.size(), now);
		});
	}

	replication.SendCommunityStatusMessageResult(clientHd, ErrorType::SUCCESS);
	replication.SendCommunityStatusMessage(clientHd, account->nickname.data(), account->profile.statusMessage.data(), (i32)account->profile.statusMessage.size());
}

static ProfileItem RemovedNotice(const ProfileItem& item)
{
	ProfileItem notice = item;
	notice.count = 0;
	return notice;
}

void HubGame::OnPlayerItemDelete(ClientHandle clientHd, InvenType invenType, ItemUID uid, u32 count)
{
	const i32 userID = plidMap->Get(clientHd);
	Account* account = playerAccountData[userID];
	ASSERT(account);
	PlayerProfile& profile = account->profile;

	const ProfileItem* item = profile.FindItem(uid);
	ItemCheck check = ItemCheck::OK;
	if(!item || item->invenType != invenType) check = ItemCheck::NOT_FOUND;
	else if(count > (u32)INT32_MAX) check = ItemCheck::BAD_COUNT;
	else check = profile.CheckRemoveItem(uid, (i32)count);
	if(check != ItemCheck::OK) {
		WARN("[client%x] HubGame :: item delete refused (account=%u invenType=%d uid=0x%08x count=%u check=%d)", (u32)clientHd, (u32)account->UID, (i32)invenType, (u32)uid, count, (i32)check);
		replication.SendItemDeleteResult(clientHd, invenType, ErrorType::INVENTORY_NOT_FOUND);
		return;
	}

	const ProfileItem before = *profile.FindItem(uid);
	profile.RemoveItem(uid, (i32)count);
	const ProfileItem* left = profile.FindItem(uid);
	const ProfileItem notice = left ? *left : RemovedNotice(before);
	LOG("[client%x] HubGame :: item deleted (account=%u uid=0x%08x doc=%u count=%u left=%d)", (u32)clientHd, (u32)account->UID, (u32)uid, (u32)before.docIndex, count, notice.count);

	if(account->profileStored) {
		const AccountUID accountUID = account->UID;
		LedgerNote note;
		note.reason = AssetChangeReason::UNKNOWN;
		note.docIndex = before.docIndex;
		note.quantity = (i32)count;
		note.at = (i64)time(nullptr);
		GetGameStore().QueueWrite("RemoveItem", [accountUID, uid, count, note](GameStoreBackend& b) {
			return b.RemoveItem(accountUID, uid, (i32)count, note);
		});
	}

	replication.SendItemDeleteResult(clientHd, invenType, ErrorType::SUCCESS);
	replication.SendItemNotices(clientHd, &notice, 1);
}

void HubGame::OnPlayerWarehouseItemChange(ClientHandle clientHd, ItemUID uid, i32 srcInvenType, i32 dstInvenType, u32 dstSlot, i32 count)
{
	const i32 userID = plidMap->Get(clientHd);
	Account* account = playerAccountData[userID];
	ASSERT(account);
	PlayerProfile& profile = account->profile;

	const ProfileItem* item = profile.FindItem(uid);
	const bool knownTypes = (srcInvenType == (i32)InvenType::INVENTORY || srcInvenType == (i32)InvenType::WAREHOUSE) &&
		(dstInvenType == (i32)InvenType::INVENTORY || dstInvenType == (i32)InvenType::WAREHOUSE);

	if(knownTypes && dstInvenType == (i32)InvenType::INVENTORY && dstSlot == 0) {
		WARN("[client%x] HubGame :: item move refused (account=%u uid=0x%08x): the TO_INVENTORY button needs the item's tab, later work", (u32)clientHd, (u32)account->UID, (u32)uid);
		replication.SendWarehouseItemChangeResult(clientHd, uid, srcInvenType, dstInvenType, dstSlot, ErrorType::INVENFULL);
		return;
	}

	InvenType invenType = (InvenType)dstInvenType;
	i32 slot = (i32)dstSlot;
	ItemCheck check = ItemCheck::OK;
	if(!item || !knownTypes || (i32)item->invenType != srcInvenType) check = ItemCheck::NOT_FOUND;
	else if(count <= 0 || count > item->count) check = ItemCheck::BAD_COUNT;
	else if(invenType == InvenType::WAREHOUSE && dstSlot == 0xFFFFFFFF) {
		if(!profile.FindFreeSlot(CONTAINER_WAREHOUSE, &invenType, &slot)) check = ItemCheck::NO_FREE_SLOT;
	}
	else if(dstSlot > (u32)INT32_MAX) check = ItemCheck::BAD_SLOT;
	if(check == ItemCheck::OK) check = profile.CheckMoveItem(uid, invenType, slot);

	const bool split = check == ItemCheck::OK && count < item->count;
	ProfileItem splitItem;
	if(split) {
		splitItem = *item;
		splitItem.uid = GetGameStore().AllocateItemUID();
		splitItem.invenType = invenType;
		splitItem.slot = slot;
		splitItem.count = count;
		check = profile.CheckAddItem(splitItem);
	}

	if(check != ItemCheck::OK) {
		WARN("[client%x] HubGame :: item move refused (account=%u uid=0x%08x src=%d dst=%d dstSlot=%u count=%d check=%d)", (u32)clientHd, (u32)account->UID, (u32)uid, srcInvenType, dstInvenType, dstSlot, count, (i32)check);
		const ErrorType error = check == ItemCheck::NOT_FOUND ? ErrorType::INVENTORY_NOT_FOUND : ErrorType::INVENFULL;
		replication.SendWarehouseItemChangeResult(clientHd, uid, srcInvenType, dstInvenType, dstSlot, error);
		return;
	}

	const ProfileItem before = *item;
	LedgerNote note;
	note.reason = AssetChangeReason::UNKNOWN;
	note.docIndex = before.docIndex;
	note.quantity = count;
	note.at = (i64)time(nullptr);
	const AccountUID accountUID = account->UID;

	eastl::fixed_vector<ProfileItem,2,false> notices;
	if(split) {
		profile.RemoveItem(uid, count);
		profile.AddItem(splitItem);
		notices.push_back(*profile.FindItem(uid));
		notices.push_back(splitItem);
		LOG("[client%x] HubGame :: item split (account=%u uid=0x%08x doc=%u %d slot %d -> %d slot %d, new uid=0x%08x count=%d left=%d)", (u32)clientHd, (u32)account->UID, (u32)uid, (u32)before.docIndex, srcInvenType, before.slot, (i32)invenType, slot, (u32)splitItem.uid, count, notices[0].count);

		if(account->profileStored) {
			GetGameStore().QueueWrite("SplitItem", [accountUID, uid, count, splitItem, note](GameStoreBackend& b) {
				return b.RemoveItem(accountUID, uid, count, note) && b.AddItem(accountUID, splitItem, note);
			});
		}
	}
	else {
		profile.MoveItem(uid, invenType, slot);
		if(before.invenType != invenType) notices.push_back(RemovedNotice(before));
		notices.push_back(*profile.FindItem(uid));
		LOG("[client%x] HubGame :: item moved (account=%u uid=0x%08x doc=%u %d slot %d -> %d slot %d)", (u32)clientHd, (u32)account->UID, (u32)uid, (u32)before.docIndex, srcInvenType, before.slot, (i32)invenType, slot);

		if(account->profileStored) {
			GetGameStore().QueueWrite("MoveItem", [accountUID, uid, invenType, slot, note](GameStoreBackend& b) {
				return b.MoveItem(accountUID, uid, invenType, slot, note);
			});
		}
	}

	replication.SendWarehouseItemChangeResult(clientHd, uid, srcInvenType, (i32)invenType, (u32)slot, ErrorType::SUCCESS);
	replication.SendItemNotices(clientHd, notices.data(), (i32)notices.size());
}

void HubGame::OnPlayerInventoryExpand(ClientHandle clientHd, u32 invenTab)
{
	const i32 userID = plidMap->Get(clientHd);
	Account* account = playerAccountData[userID];
	ASSERT(account);
	PlayerProfile& profile = account->profile;

	if(invenTab < 1 || invenTab > INVENTORY_TABS) {
		WARN("[client%x] HubGame :: inventory expand refused (account=%u invenTab=%u is not a tab)", (u32)clientHd, (u32)account->UID, invenTab);
		replication.SendInventoryExpandResult(clientHd, ErrorType::INVENTORY_EXPAND_UNKNOWN, invenTab, 0);
		return;
	}

	const i32 container = (i32)invenTab - 1;
	const i32 lines = profile.lines[container];
	if(lines >= ContainerMaxLines(container)) {
		WARN("[client%x] HubGame :: inventory expand refused (account=%u invenTab=%u lines=%d is full)", (u32)clientHd, (u32)account->UID, invenTab, lines);
		replication.SendInventoryExpandResult(clientHd, ErrorType::INVENTORY_EXPAND_ALREADY_FULL, invenTab, lines);
		return;
	}

	const eastl::fixed_vector<i32,16>& prices = Config().servicePriceInvenUnlockGold;
	const i32 step = eastl::max(lines - Config().serviceRuleDefaultLineCount[container] + 1, 1);
	if(step > (i32)prices.size()) {
		WARN("[client%x] HubGame :: inventory expand refused (account=%u invenTab=%u lines=%d step=%d is a CCoin step, not built)", (u32)clientHd, (u32)account->UID, invenTab, lines, step);
		replication.SendInventoryExpandResult(clientHd, ErrorType::INVENTORY_EXPAND_UNKNOWN, invenTab, lines);
		return;
	}

	const i64 price = prices[step - 1];
	if(!profile.CanSpend(Currency::GOLD, price)) {
		WARN("[client%x] HubGame :: inventory expand refused (account=%u invenTab=%u step=%d price=%lld gold=%lld)", (u32)clientHd, (u32)account->UID, invenTab, step, (long long)price, (long long)profile.gold);
		replication.SendInventoryExpandResult(clientHd, ErrorType::INVENTORY_EXPAND_NOT_ENOUGH_RON, invenTab, lines);
		return;
	}

	profile.Spend(Currency::GOLD, price);
	const i32 newLines = lines + 1;
	profile.lines[container] = newLines;
	LOG("[client%x] HubGame :: inventory expanded (account=%u invenTab=%u lines=%d step=%d price=%lld gold=%lld)", (u32)clientHd, (u32)account->UID, invenTab, newLines, step, (long long)price, (long long)profile.gold);

	if(account->profileStored) {
		const AccountUID accountUID = account->UID;
		LedgerNote note;
		note.reason = AssetChangeReason::INVENTORY_EXPAND;
		note.docIndex = (ItemDocIndex)0;
		note.quantity = 1;
		note.at = (i64)time(nullptr);
		GetGameStore().QueueWrite("InventoryExpand", [accountUID, price, note, container, newLines](GameStoreBackend& b) {
			return b.SpendCurrency(accountUID, Currency::GOLD, price, note) && b.SetInventoryLines(accountUID, container, newLines);
		});
	}

	replication.SendInventoryExpandResult(clientHd, ErrorType::SUCCESS, invenTab, newLines);
	replication.SendMoney(clientHd, profile.gold, AssetChangeReason::INVENTORY_EXPAND);
}

void HubGame::SpawnLeader(ClientHandle clientHd, ClassType classType, SkinIndex skinIndex)
{
	const i32 userID = plidMap->Get(clientHd);
	const auto masterIt = GetGameXmlContent().masterClassTypeMap.find(classType);
	ASSERT(masterIt != GetGameXmlContent().masterClassTypeMap.end());

	// select a spawn point at random
	const SpawnPoint& spawnPoint = mapSpawnPoints[RandUint() % mapSpawnPoints.size()];
	vec3 pos = spawnPoint.pos;
	vec3 dir = spawnPoint.dir;
	vec3 eye(0, 0, 0);

	// TODO: check if already leader character
	if((playerActorUID[userID] != ActorUID::INVALID)) {
		WorldHub::ActorCore* actor = world.FindPlayerActor(playerActorUID[userID]);
		ASSERT(actor);

		pos = actor->pos;
		dir = actor->dir;
		eye = actor->eye;

		world.DestroyPlayerActor(playerActorUID[userID]);
	}

	ASSERT(playerAccountData[userID]);
	const Account* account = playerAccountData[userID];
	ASSERT(masterIt->second->classType == classType);

	WorldHub::ActorPlayer& actor = world.SpawnPlayerActor(userID, classType, skinIndex, account->nickname.data(), account->guildTag.data());
	actor.pos = pos;
	actor.dir = dir;
	actor.eye = eye;
	actor.clientHd = clientHd; // TODO: this is not useful right now
	playerActorUID[userID] = actor.UID;

	replication.SendPlayerSetLeaderMaster(clientHd, playerActorUID[userID], classType, skinIndex);
}

void HubGame::OnPlayerSyncActionState(ClientHandle clientHd, ActorUID actorUID, ActionStateID state, i32 param1, i32 param2, f32 rotate, f32 upperRotate)
{
	const i32 userID = plidMap->Get(clientHd);

	WorldHub::ActorPlayer* actor = world.FindPlayerActor(playerActorUID[userID]);
	ASSERT(actor);

	// TODO: check hacking
	actor->rotate = rotate;
	actor->upperRotate = upperRotate;
	actor->actionState = state;
	actor->actionParam1 = param1;
	actor->actionParam2 = param2;
}

void HubGame::OnPlayerJukeboxQueueSong(ClientHandle clientHd, SongID songID)
{
	const i32 userID = plidMap->Get(clientHd);
	Account* account = playerAccountData[userID];
	ASSERT(account);
	PlayerProfile& profile = account->profile;

	const GameXmlContent::Song* xmlSong = GetGameXmlContent().FindJukeboxSongByID(songID);
	if(!xmlSong) {
		WARN("[client%x] HubGame :: jukebox song refused (account=%u song=%d is unknown)", (u32)clientHd, (u32)account->UID, (i32)songID);
		replication.SendJukeboxEnqueueResult(clientHd, ErrorType::JUKEBOX_NOT_IN_TOWN);
		return;
	}

	const ItemDocIndex coinIndex = (ItemDocIndex)Config().FixedDataJukeboxCoinIndex;
	eastl::fixed_vector<const ProfileItem*,32,true> stacks;
	i32 owned = 0;
	foreach_const(it, profile.items) {
		if(it->invenType != InvenType::INVENTORY || it->docIndex != coinIndex) continue;
		stacks.push_back(&*it);
		owned += it->count;
	}
	eastl::sort(stacks.begin(), stacks.end(), [](const ProfileItem* a, const ProfileItem* b) { return a->slot < b->slot; });

	if(owned < xmlSong->coinCost) {
		WARN("[client%x] HubGame :: jukebox song refused (account=%u song=%d cost=%d coins=%d)", (u32)clientHd, (u32)account->UID, (i32)songID, xmlSong->coinCost, owned);
		replication.SendJukeboxEnqueueResult(clientHd, ErrorType::JUKEBOX_NOT_ENOUGH_COIN);
		return;
	}

	const ErrorType result = JukeboxQueueSong(userID, songID);
	if(result != ErrorType::SUCCESS) {
		WARN("[client%x] HubGame :: jukebox song refused (account=%u song=%d result=%u)", (u32)clientHd, (u32)account->UID, (i32)songID, (u32)result);
		replication.SendJukeboxEnqueueResult(clientHd, result);
		return;
	}

	eastl::fixed_vector<ProfileItem,8,true> notices;
	eastl::fixed_vector<i32,8,true> taken;
	i32 remaining = xmlSong->coinCost;
	foreach_const(it, stacks) {
		if(remaining == 0) break;
		const i32 take = eastl::min((*it)->count, remaining);
		notices.push_back(**it);
		notices.back().count -= take;
		taken.push_back(take);
		remaining -= take;
	}
	ASSERT(remaining == 0);

	for(i32 i = 0; i < (i32)notices.size(); i++) {
		profile.RemoveItem(notices[i].uid, taken[i]);
	}
	LOG("[client%x] HubGame :: jukebox song queued (account=%u song=%d coins=%d left=%d)", (u32)clientHd, (u32)account->UID, (i32)songID, xmlSong->coinCost, owned - xmlSong->coinCost);

	if(account->profileStored) {
		const AccountUID accountUID = account->UID;
		const i64 now = (i64)time(nullptr);
		eastl::vector<eastl::pair<ItemUID,i32>> removals;
		for(i32 i = 0; i < (i32)notices.size(); i++) {
			removals.push_back(eastl::make_pair(notices[i].uid, taken[i]));
		}
		GetGameStore().QueueWrite("PlayJukebox", [accountUID, coinIndex, removals, now](GameStoreBackend& b) {
			foreach_const(it, removals) {
				LedgerNote note;
				note.reason = AssetChangeReason::PLAY_JUKEBOX;
				note.docIndex = coinIndex;
				note.quantity = it->second;
				note.at = now;
				if(!b.RemoveItem(accountUID, it->first, it->second, note)) return false;
			}
			return true;
		});
	}

	replication.SendJukeboxEnqueueResult(clientHd, ErrorType::SUCCESS);
	replication.SendItemNotices(clientHd, notices.data(), (i32)notices.size());
}

void HubGame::OnPlayerReadyToLoad(ClientHandle clientHd)
{
	replication.SendLoadLobby(clientHd, (MapIndex)Config().LobbyMap);
}

void HubGame::OnCreateParty(ClientHandle clientHd, EntrySystemID entry, StageType stageType)
{
	const i32 userID = plidMap->Get(clientHd);
	if(!GetGameXmlContent().HasEntrySystem((i32)entry)) {
		WARN("[client%x] WARNING: unknown entrySysID (%d)", clientHd, (i32)entry);
		return;
	}

	// TODO: validate args
	const Account& acc = *playerAccountData[userID];

	if(Config().DevMode && Config().DevQuickConnect) {
		PacketWriter<Sv::SN_DoConnectGameServer> packet;
		packet.Write<u16>(12900); // port
		eastl::array<u8,4> ip = { 127, 0, 0, 1 };
		packet.Write(ip); // ip
		packet.Write<i32>(1); // gameID
		packet.Write<u32>(0x0); // idcHash
		packet.WriteStringObj(acc.nickname.data(), acc.nickname.size());
		packet.Write<u32>(In::ProduceInstantKey(acc.UID, SortieUID(1)));
		NT_LOG("[client%x] HubGame :: %s", clientHd, PacketSerialize<Sv::SN_DoConnectGameServer>(packet.data, packet.size));
		replication.server->SendPacket(clientHd, packet);
	}
	else {
		AreaIndex areaID = AreaIndex(0);
		StageIndex stageID = StageIndex(0);
		MapIndex mapID = MapIndex(0);
		if(!GetGameXmlContent().FindQueueAreaStage((i32)entry, &areaID, &stageID) || !GetGameXmlContent().FindStageMap(stageID, &mapID)) {
			WARN("[client%x] WARNING: no playable map for entrySysID (%d)", clientHd, (i32)entry);
			return;
		}
		pendingPartyEntry[userID] = entry;
		pendingPartyStage[userID] = stageType;
		matchmaker->QueryPartyCreate(acc.nickname, acc.UID);
	}
}

void HubGame::OnEnqueueGame(ClientHandle clientHd)
{
	const i32 userID = plidMap->Get(clientHd);

	const PartyUID partyUID = playerMap[userID]->partyUID;
	if(partyUID == PartyUID::INVALID) return;
	auto f = partyMap.find(partyUID);
	if(f == partyMap.end()) return;
	Party& party = *f->second;
	if(party.isQueued) {
		WARN("[client%x] WARNING: party is already queued (partyUID=%u)", clientHd, (u32)partyUID);
		return;
	}
	ASSERT(party.areaIndex != AreaIndex(0));
	ASSERT(party.stageIndex != StageIndex(0));
	ASSERT(party.mapIndex != MapIndex(0));
	party.isQueued = true;
	matchmaker->QueryPartyEnqueue(partyUID, party.areaIndex, party.stageIndex, party.mapIndex);
}

void HubGame::OnSortieRoomFound(ClientHandle clientHd, SortieUID sortieID)
{
	// This indicates the client has received the 'match found' packet
	// When waiting time is 0 this is like accepting the match

	const i32 userID = plidMap->Get(clientHd);
	matchmaker->QueryPlayerNotifyRoomFound(playerAccountData[userID]->UID, sortieID);
}

void HubGame::OnSortieRoomConfirm(ClientHandle clientHd, bool confirm)
{
	const i32 userID = plidMap->Get(clientHd);
	matchmaker->QueryPlayerRoomConfirm(playerAccountData[userID]->UID, playerMap[userID]->sortieUID, confirm);
}

void HubGame::MmOnPartyCreated(PartyUID partyUID, AccountUID leader)
{
	// TODO: find and skip if not found
	const ClientHandle clientHd = accountClientHandleMap.at(leader);
	const i32 userID = plidMap->Get(clientHd);
	playerMap[userID]->partyUID = partyUID;

	// create party
	ASSERT(partyMap.find(partyUID) == partyMap.end());
	partyList.emplace_back(partyUID);
	Party& party = *(--partyList.end());
	Party::Member member;
	member.accountUID = leader;
	party.memberList.push_back(member);

	partyMap.emplace(partyUID, --partyList.end());
	party.entry = pendingPartyEntry[userID];
	party.stageType = pendingPartyStage[userID];
	AreaIndex areaID = AreaIndex(0);
	StageIndex stageID = StageIndex(0);
	MapIndex mapID = MapIndex(0);
	ASSERT(GetGameXmlContent().FindQueueAreaStage((i32)party.entry, &areaID, &stageID));
	ASSERT(GetGameXmlContent().FindStageMap(stageID, &mapID));
	party.areaIndex = areaID;
	party.stageIndex = stageID;
	party.mapIndex = mapID;

	replication.SendPartyCreateSucess(clientHd, UserID(userID + 1), StageType::PVP_GAME);
}

void HubGame::MmOnPartyEnqueued(PartyUID partyUID)
{
	auto f = partyMap.find(partyUID);
	if(f == partyMap.end()) return;
	Party& party = *f->second;
	foreach_const(m, party.memberList) {
		// TODO: check if on this hub
		const ClientHandle clientHd = accountClientHandleMap.at(m->accountUID);
		replication.SendPartyEnqueue(clientHd, party.stageIndex);
	}
}

void HubGame::MmOnMatchFound(const In::MN_MatchingPartyFound& matchingParty)
{
	auto f = partyMap.find(matchingParty.partyUID);
	if(f == partyMap.end()) return;
	Party& party = *f->second;
	party.isQueued = false;
	eastl::fixed_vector<UserID,16> rowIDs;
	for(i32 i = 0; i < matchingParty.playerCount; i++) {
		rowIDs.push_back(UserID(i + 1));
	}
	foreach_const(m, party.memberList) {
		auto h = accountClientHandleMap.find(m->accountUID);
		if(h == accountClientHandleMap.end()) continue;
		const ClientHandle clientHd = h->second;
		const i32 userID = plidMap->Get(clientHd);
		if(!playerAccountData[userID]) continue;
		playerMap[userID]->sortieUID = matchingParty.sortieUID;

		u8 myTeam = 0;
		for(i32 i = 0; i < matchingParty.playerCount; i++) {
			if(matchingParty.playerList[i].accountUID == m->accountUID) myTeam = matchingParty.playerList[i].team;
		}
		replication.SendMatchingPartyFound(clientHd, matchingParty, party.stageIndex, rowIDs, myTeam);
	}
}

bool HubGame::ParseChatCommand(ClientHandle clientHd, const wchar* msg, const i32 len)
{
	if(!Config().DevMode) return false; // don't allow command when dev mode is not enabled

	const i32 userID = plidMap->Get(clientHd);

	static ActorUID lastLegoActorUID = ActorUID::INVALID;

	if(msg[0] == L'!') {
		msg++;

		if(EA::StdC::Strncmp(msg, L"lego", 4) == 0) {
			WorldHub::ActorCore* playerActor = world.FindPlayerActor(playerActorUID[userID]);
			ASSERT(playerActor);

			WorldHub::ActorCore& actor = world.SpawnPlayerActor(-1, (ClassType)18, SkinIndex::DEFAULT, L"legomage15", L"MEME");
			actor.pos = playerActor->pos;
			actor.dir = playerActor->dir;
			actor.eye = playerActor->eye;

			// trigger second emote
			actor.actionState = ActionStateID::EMOTION_BEHAVIORSTATE;
			actor.actionParam1 = 2;

			lastLegoActorUID = actor.UID;

			SendDbgMsg(clientHd, LFMT(L"Actor spawned at (%g, %g, %g)", actor.pos.x, actor.pos.y, actor.pos.z));
			return true;
		}

		if (EA::StdC::Strncmp(msg, L"spawn ", 6) == 0) {
			msg += 6; // skip command
			int classTypeVal = EA::StdC::AtoI32(msg); // convert wide string to integer

			WorldHub::ActorCore* playerActor = world.FindPlayerActor(playerActorUID[userID]);
			ASSERT(playerActor);

			WorldHub::ActorCore& actor = world.SpawnPlayerActor(-1, (ClassType)classTypeVal, SkinIndex::DEFAULT, L"spawned", L"TEST");
			actor.pos = playerActor->pos;
			actor.dir = playerActor->dir;
			actor.eye = playerActor->eye;

			lastLegoActorUID = actor.UID;

			SendDbgMsg(clientHd, LFMT(L"Actor spawned with class %d at (%g, %g, %g)",
				classTypeVal, actor.pos.x, actor.pos.y, actor.pos.z));
			return true;
		}

		if(EA::StdC::Strncmp(msg, L"delete", 6) == 0) {
			world.DestroyPlayerActor(lastLegoActorUID);

			SendDbgMsg(clientHd, LFMT(L"Actor destroyed (%u)", lastLegoActorUID));
			return true;
		}

		if (EA::StdC::Strncmp(msg, L"change ", 7) == 0) {
			msg += 7; // skip command

			// Parse class type and optional skin
			int classTypeVal;
			int skinIndex = (int)SkinIndex::DEFAULT;

			// Check if we have two arguments (class and skin)
			const wchar* space = wcschr(msg, L' ');
			if (space) {
				// Temporarily null terminate first number
				wchar firstNum[32];
				size_t len = space - msg;
				if (len < sizeof(firstNum)) {
					EA::StdC::Strncpy(firstNum, msg, len);
					firstNum[len] = L'\0';
					classTypeVal = EA::StdC::AtoI32(firstNum);
					skinIndex = EA::StdC::AtoI32(space + 1); // Convert second number after space
				}
			}
			else {
				classTypeVal = EA::StdC::AtoI32(msg); // Just class type provided
			}

			WorldHub::ActorPlayer* playerActor = world.FindPlayerActor(playerActorUID[userID]);
			ASSERT(playerActor);

			vec3 oldPos = playerActor->pos;
			vec3 oldDir = playerActor->dir;
			vec3 oldEye = playerActor->eye;

			// Destroy the current player actor
			world.DestroyPlayerActor(playerActorUID[userID]);

			// Create new player actor properly
			ASSERT(playerAccountData[userID]); // account data should exist
			WorldHub::ActorPlayer& actor = world.SpawnPlayerActor(
				userID,
				(ClassType)classTypeVal,
				(SkinIndex)skinIndex,
				playerAccountData[userID]->nickname.data(),
				playerAccountData[userID]->guildTag.data()
			);

			// Set position and orientation
			actor.pos = oldPos;
			actor.dir = oldDir;
			actor.eye = oldEye;

			// Store the new actor UID
			playerActorUID[userID] = actor.UID;

			// Notify clients about the new character
			replication.SendPlayerSetLeaderMaster(clientHd, actor.UID, (ClassType)classTypeVal, (SkinIndex)skinIndex);

			SendDbgMsg(clientHd, LFMT(L"Character changed to class %d with skin %d", classTypeVal, skinIndex));
			return true;
		}

		if(EA::StdC::Strncmp(msg, L"rozark", 6) == 0) {
			WorldHub::ActorCore* playerActor = world.FindPlayerActor(playerActorUID[userID]);
			ASSERT(playerActor);

			WorldHub::ActorCore& actor = world.SpawnPlayerActor(-1, (ClassType)5001200, SkinIndex::DEFAULT, L"rozark", L"MEME");
			actor.pos = playerActor->pos;
			actor.dir = playerActor->dir;
			actor.eye = playerActor->eye;
			lastLegoActorUID = actor.UID;

			SendDbgMsg(clientHd, LFMT(L"Actor spawned at (%g, %g, %g)", actor.pos.x, actor.pos.y, actor.pos.z));
			return true;
		}

		if(EA::StdC::Strncmp(msg, L"tanian", 6) == 0) {
			WorldHub::ActorCore* playerActor = world.FindPlayerActor(playerActorUID[userID]);
			ASSERT(playerActor);

			WorldHub::ActorCore& actor = world.SpawnPlayerActor(-1, (ClassType)5001040, SkinIndex::DEFAULT, L"rozark", L"MEME");
			actor.pos = playerActor->pos;
			actor.dir = playerActor->dir;
			actor.eye = playerActor->eye;
			lastLegoActorUID = actor.UID;

			SendDbgMsg(clientHd, LFMT(L"Actor spawned at (%g, %g, %g)", actor.pos.x, actor.pos.y, actor.pos.z));
			return true;
		}

		if(EA::StdC::Strncmp(msg, L"fish", 4) == 0) {
			WorldHub::ActorCore* playerActor = world.FindPlayerActor(playerActorUID[userID]);
			ASSERT(playerActor);

			WorldHub::ActorCore& actor = world.SpawnPlayerActor(-1, (ClassType)5000800, SkinIndex::DEFAULT, L"rozark", L"MEME");
			actor.pos = playerActor->pos;
			actor.dir = playerActor->dir;
			actor.eye = playerActor->eye;
			lastLegoActorUID = actor.UID;

			SendDbgMsg(clientHd, LFMT(L"Actor spawned at (%g, %g, %g)", actor.pos.x, actor.pos.y, actor.pos.z));
			return true;
		}

		if(EA::StdC::Strncmp(msg, L"smoke", 5) == 0) {
			WorldHub::ActorPlayer* playerActor = world.FindPlayerActor(playerActorUID[userID]);
			ASSERT(playerActor);

			SendDbgMsg(clientHd, LFMT(L"All you have to do was follow the damn train %s :(", playerActor->name.data()));
			return true;
		}

		if(EA::StdC::Strncmp(msg, L"upsidedown", 10) == 0) {
			DevJukeboxQueueSong(userID, SongID::UpsideDown);
			return true;
		}
		
		if(EA::StdC::Strncmp(msg, L"scml", 4) == 0) {
			DevJukeboxQueueSong(userID, SongID::Scml);
			return true;
		}

		if(EA::StdC::Strncmp(msg, L"poharan", 7) == 0) {
			DevJukeboxQueueSong(userID, SongID::Poharan);
			return true;
		}

		if(EA::StdC::Strncmp(msg, L"triangle", 8) == 0) {
			DevJukeboxQueueSong(userID, SongID::Triangle);
			return true;
		}
		
		if(EA::StdC::Strncmp(msg, L"arami", 5) == 0) {
			DevJukeboxQueueSong(userID, SongID::Arami);
			return true;
		}

	}

	return false;
}

void HubGame::SendDbgMsg(ClientHandle clientHd, const wchar* msg)
{
	replication.SendChatMessageToClient(clientHd, L"System", 1, msg);
}

void HubGame::SpawnNPC(CreatureIndex docID, i32 localID, const vec3& pos, const vec3& dir)
{
	WorldHub::ActorCore& actor = world.SpawnNpcActor(docID, localID);
	actor.pos = pos;
	actor.dir = dir;
}
