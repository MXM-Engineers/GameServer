#include <EASTL/sort.h>
#include "config.h"
#include <mxm/game_content.h>
#include <common/packet_serialize.h>
#include <common/packet_validator.h>
#include "instance.h"
#include "account.h"
#include "matchmaker_connector.h"
#include "guild_store.h"
#include <mxm/account_progress.h>
#include "arsenal.h"
#include "achieve_send.h"

static const u32 GEAR_SELECT_OK = 0;

template<typename Writer>
static void WriteMasterGearPages(Writer& packet, const wchar* nickname)
{
	const AccountGears gears = AccountGearsLoad(nickname);
	const AccountInventory inventory = AccountInventoryLoad(nickname);
	packet.template Write<u16>((u16)gears.pages.size());
	for(i32 i = 0; i < (i32)gears.pages.size(); i++) {
		const AccountGearPage& page = gears.pages[i];
		packet.template Write<u8>((u8)(i + 1));
		packet.WriteStringObj(page.name.data());
		packet.template Write<u16>((u16)page.items.size());
		for(u32 uid : page.items) {
			i32 type = -1;
			foreach_const(it, inventory.items) {
				if(uid == 0 || it->uid != uid) continue;
				if(const GameXmlContent::GearItem* gear = GetGameXmlContent().FindGearItem(it->docIndex)) type = gear->slotType;
			}
			packet.template Write<i32>(type);
			packet.template Write<u32>(type >= 0 ? uid : 0);
		}
	}
}
#include "bot_loadouts.h"

bool HubInstance::Init(Server* server_)
{
	game.Init(server_, &plidMap);
	packetHandler.Init(&game);
	return true;
}

void HubInstance::Update(Time localTime_)
{
	game.Update(localTime_);
}

void HubInstance::OnClientsConnected(const NewUser* clientList, const i32 count)
{
	for(int i = 0; i < count; i++) {
		plidMap.Push(clientList[i].clientHd);
	}

	// FIXME: we should pull account data at the last moment and just pass AccountUIDs
	const AccountManager& am = GetAccountManager();
	eastl::fixed_vector<eastl::pair<ClientHandle, const Account*>,MAX_CLIENTS> list;
	for(int i = 0; i < count; i++) {
		list.push_back({ clientList[i].clientHd, &*am.accountMap.at(clientList[i].accountUID) });
	}

	packetHandler.OnClientsConnected(list.data(), count);
}

void HubInstance::OnClientsDisconnected(const ClientHandle* clientList, const i32 count)
{
	packetHandler.OnClientsDisconnected(clientList, count);

	for(int i = 0; i < count; i++) {
		plidMap.Pop(clientList[i]);
	}
}

void HubInstance::OnClientsTransferOut(const ClientHandle* clientList, const i32 count)
{
	// TODO: push to a 'transfer list' that will call replicate.SendClearAllEntities() before detaching clients
	packetHandler.OnClientsDisconnected(clientList, count);

	for(int i = 0; i < count; i++) {
		plidMap.Pop(clientList[i]);
	}
}

void HubInstance::OnClientPacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData)
{
	packetHandler.OnNewPacket(clientHd, header, packetData);
}

void HubInstance::OnMatchmakerPacket(const NetHeader& header, const u8* packetData)
{
	packetHandler.OnMatchmakerPacket(header, packetData);
}

const i32 PICKING_TIME = 60;
static const u32 MEASURED_BAN_TIME_SEC = 60;
static const u32 MEASURED_BANS_PER_ROUND = 1;
static const u8 MEASURED_BAN_ROUNDS = 2;
static const u32 MEASURED_BAN_SLOT_STATE = 2;
static const f64 HYP_BOT_BAN_DELAY_SEC = 5.0;
static const u32 HYP_BAN_REFUSED = 1;
const i32 MATCH_WAIT_TIME = 10;

const f64 SENT_TO_GAME_EXPIRE_SEC = 600.0;

struct SentToGameEntry
{
	WideString nickname;
	SortieUID sortieUID;
	AccountUID gameAccountUID;
	Time endTime;
};

typedef eastl::fixed_vector<SentToGameEntry,64,true> SentToGameList;

static SentToGameList& GetSentToGameList()
{
	static SentToGameList list;

	const Time now = TimeRelNow();
	for(auto it = list.begin(); it != list.end();) {
		if(it->endTime != Time::ZERO && TimeDiffSec(TimeDiff(it->endTime, now)) > SENT_TO_GAME_EXPIRE_SEC) it = list.erase(it);
		else ++it;
	}
	return list;
}

void MarkAccountSentToGame(const WideString& nickname, SortieUID sortieUID, AccountUID gameAccountUID)
{
	SentToGameList& list = GetSentToGameList();
	bool found = false;
	foreach(it, list) {
		if(it->nickname == nickname) { it->sortieUID = sortieUID; it->gameAccountUID = gameAccountUID; it->endTime = Time::ZERO; found = true; }
	}
	if(!found) list.push_back({ nickname, sortieUID, gameAccountUID, Time::ZERO });
	LOG("[Room] %ls sent to a game server", nickname.data());
}

void MarkSortieEnded(SortieUID sortieUID)
{
	SentToGameList& list = GetSentToGameList();
	const Time now = TimeRelNow();
	foreach(it, list) {
		if(it->sortieUID == sortieUID) it->endTime = now;
	}
}

struct SortieGameServer
{
	SortieUID sortieUID;
	eastl::array<u8,4> ip;
	u16 port;
};
static eastl::fixed_vector<SortieGameServer,64,true> g_sortieGameServers;

void MarkSortieGameServer(SortieUID sortieUID, const eastl::array<u8,4>& ip, u16 port)
{
	foreach(it, g_sortieGameServers) {
		if(it->sortieUID != sortieUID) continue;
		it->ip = ip;
		it->port = port;
		return;
	}
	if(g_sortieGameServers.size() >= 64) g_sortieGameServers.erase(g_sortieGameServers.begin());
	g_sortieGameServers.push_back({ sortieUID, ip, port });
}

bool FindSortieGameServer(SortieUID sortieUID, eastl::array<u8,4>* ip, u16* port)
{
	foreach_const(it, g_sortieGameServers) {
		if(it->sortieUID != sortieUID) continue;
		*ip = it->ip;
		*port = it->port;
		return true;
	}
	return false;
}

bool TakeAccountReturningFromGame(const WideString& nickname, SortieUID* sortieUID, AccountUID* gameAccountUID)
{
	SentToGameList& list = GetSentToGameList();
	foreach(it, list) {
		if(it->nickname != nickname) continue;
		*sortieUID = it->sortieUID;
		*gameAccountUID = it->gameAccountUID;
		list.erase(it);
		return true;
	}
	return false;
}

void RoomInstance::Init(Server* server_, const NewUser* userlist, const i32 userCount)
{
	server = server_;

	teamMasterPickCount[Team::RED].fill(0);
	teamMasterPickCount[Team::BLUE].fill(0);

	foreach_const(m, GetGameXmlContent().masters) {
		if(m->skillIDs.empty() || m->skinIDs.empty() || m->defaultWeaponIDs.empty()) continue;
		ASSERT(allowedMastersSet.size() < MAX_ALLOWED_MASTERS);
		allowedMastersSet.insert(m->classType);
	}
	LOG("[Room] %d pickable characters", (i32)allowedMastersSet.size());

	startTime = TimeNow();

	LOG("[Room(%llx)] room created (userCount=%d)", sortieUID, userCount);

	for(const NewUser* nu = userlist; nu != userlist+userCount; ++nu) {
		userList.emplace_back(nu->clientHd, nu->accountUID, nu->name, UserID(nu->userID+1), nu->isBot, Team(nu->team));
		User* user = &userList.back();

		// has account when client is connected
		ASSERT(nu->clientHd == ClientHandle::INVALID || nu->accountUID != AccountUID::INVALID);

		switch(nu->team) {
			case Team::RED: { teamRed.push_back(user); } break;
			case Team::BLUE: { teamBlue.push_back(user); } break;
			case Team::SPECTATORS: { teamSpectator.push_back(user); } break;
			default: { ASSERT_MSG(0, "case not handled"); }
		}

		if(nu->clientHd != ClientHandle::INVALID) {
			connectedUsers.push_back(user);
		}
	}

	const GameXmlContent::StageMaps* stageInfo = GetGameXmlContent().FindStageMaps(stageIndex);
	if(stageInfo && !stageInfo->stageSkillCandidates.empty()) {
		foreach(u, userList) u->stageSkill = stageInfo->stageSkillCandidates[0];
	}

	foreach_const(u, connectedUsers) {
		const User& user = **u;

		// is this for when all players accepted or just our own party?
		Sv::SN_MatchingPartyGathered gathered;
		gathered.allConfirmed = 1;
		SendPacket(user.clientHd, gathered);

		const GameXmlContent& content = GetGameXmlContent();
		const AccountArsenal arsenal = AccountArsenalLoad(user.name.data());
		{
			PacketWriter<Sv::SN_ActiveIngameEventList,128> packet;
			eastl::fixed_vector<i32,16,false> active;
			foreach_const(ev, content.activeGameEvents) {
				const bool stageMatch = eastl::find(ev->stages.begin(), ev->stages.end(), (i32)stageIndex) != ev->stages.end();
				if(stageMatch || (ev->stages.empty() && ev->gameTypes.empty())) active.push_back(ev->index);
			}
			packet.Write<u16>((u16)active.size());
			foreach_const(i, active) packet.Write<i32>(*i);
			SendPacket(user.clientHd, packet);
		}

		// SN_ProfileCharacters
		{
			PacketWriter<Sv::SN_ProfileCharacters,2048> packet;

			packet.Write<u16>(allowedMastersSet.size()); // charaList_count

			const AccountGears gears = AccountGearsLoad(user.name.data());
			foreach_const(it, allowedMastersSet) {
				const GameXmlContent::Master& master = *content.masterClassTypeMap.at(*it);

				Sv::SN_ProfileCharacters::Character chara;
				chara.characterID = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)master.classType);
				chara.creatureIndex = master.ID;
				SkillID loadout[2];
				ArsenalSkills(arsenal, master, loadout);
				chara.skillSlot1 = loadout[0];
				chara.skillSlot2 = loadout[1];
				chara.classType = master.classType;
				chara.x = 0;
				chara.y = 0;
				chara.z = 0;
				chara.characterType = 1;
				chara.skinIndex = SkinIndex::DEFAULT;
				const u8 equippedType = arsenal.EquippedType((i32)master.classType);
				const i32 equipped = ArsenalWeaponIndex(master, equippedType, arsenal.Grade((i32)master.classType, equippedType));
				chara.weaponIndex = equipped >= 0 ? (WeaponIndex)equipped : master.weaponIDs[1];
				chara.masterGearNo = gears.SelectedPage((i32)master.classType);
				packet.Write(chara);
			}

			SendPacket(user.clientHd, packet);
		}

		// SN_ProfileWeapons
		{
			PacketWriter<Sv::SN_ProfileWeapons,4096> packet;

			u16 weaponCount = 0;
			foreach_const(it, allowedMastersSet) {
				weaponCount += 3;
			}

			packet.Write<u16>(weaponCount); // weaponList_count

			foreach_const(it, allowedMastersSet) {
				const GameXmlContent::Master& master = *content.masterClassTypeMap.at(*it);

				Sv::SN_ProfileWeapons::Weapon weapon;
				weapon.characterID = (LocalActorID)((u32)LocalActorID::FIRST_SELF_MASTER + (i32)master.classType);

				for(int wi = 0; wi < 3; wi++) {
					weapon.weaponType = wi+1;
					const AccountArsenal::Weapon* state = arsenal.FindWeapon((i32)master.classType, (u8)(wi + 1));
					const i32 grade = state ? state->grade : 0;
					const i32 index = ArsenalWeaponIndex(master, (u32)(wi + 1), (u32)grade);
					weapon.weaponIndex = (WeaponIndex)(index >= 0 ? index : (i32)master.weaponIDs[1 + wi*4]);
					weapon.grade = index >= 0 ? grade : 0;
					weapon.isUnlocked = wi == 0 || (state && state->unlocked);
					weapon.isActivated = 1;
					packet.Write(weapon);
				}
			}

			SendPacket(user.clientHd, packet);
		}

		// SN_ProfileMasterGears
		{
			PacketWriter<Sv::SN_ProfileMasterGears,2048> packet;

			WriteMasterGearPages(packet, user.name.data());

			SendPacket(user.clientHd, packet);
		}

		// SN_ProfileSkills
		{
			PacketWriter<Sv::SN_ProfileSkills,24576> packet;
			// TODO: split in several packets?
			packet.Write<u8>(1); // packetNum

			u16 skillCount = 0;
			foreach_const(it, allowedMastersSet) {
				const GameXmlContent::Master& master = *content.masterClassTypeMap.at(*it);
				skillCount += master.skillIDs.size();
			}

			packet.Write<u16>(skillCount); // skills_count

			foreach_const(it, allowedMastersSet) {
				const GameXmlContent::Master& master = *content.masterClassTypeMap.at(*it);
				const LocalActorID characterID = LocalActorID((u32)LocalActorID::FIRST_SELF_MASTER + (i32)master.classType);
				for(int si = 0; si < (int)master.skillIDs.size(); si++) {
					packet.Write(characterID);
					packet.Write(master.skillIDs[si]);
					const u8 unlocked = (si < (int)master.skillUnlocked.size() && !master.skillUnlocked[si]) ? (u8)arsenal.SkillUnlocked((i32)master.classType, (i32)master.skillIDs[si]) : (u8)1;
					packet.Write<u8>(unlocked);
					packet.Write<u8>(1);
					WriteSkillProperties(packet, (i32)master.skillIDs[si], arsenal.SkillLevel((i32)master.skillIDs[si]));
				}
			}

			SendPacket(user.clientHd, packet);
		}

		// SN_MasterRotationInfo
		{
			PacketWriter<Sv::SN_MasterRotationInfo> packet;
			packet.Write<i32>(0); // refreshCount

			eastl::fixed_vector<CreatureIndex,100,false> freeList;
			foreach_const(m, allowedMastersSet) {
				freeList.push_back(CreatureIndex(100000000 + (i32)*m));
			}

			packet.WriteVec(freeList.data(), freeList.size()); // freeRotation
			packet.WriteVec((CreatureIndex*)nullptr, 0); // pccafeRotation
			packet.WriteVec((CreatureIndex*)nullptr, 0); // vipRotation

			SendPacket(user.clientHd, packet);
		}

		if(stageInfo && !stageInfo->stageSkillCandidates.empty()) {
			PacketWriter<Sv::SN_StageSkillAvailable,256> packet;
			packet.WriteVec(stageInfo->stageSkillCandidates.data(), stageInfo->stageSkillCandidates.size());
			packet.Write<SkillID>(user.stageSkill);
			SendPacket(user.clientHd, packet);
			foreach_const(m, userList) {
				if(m->team != user.team) continue;
				Sv::SN_StageSkillSelect select;
				select.userId = (u32)m->userID;
				select.skillIndex = (u32)m->stageSkill;
				SendPacket(user.clientHd, select);
			}
		}

		if(!draft) SendPickPhaseStart(user);
	}

	if(Config().DevShowcase) {
		static i32 showcaseRooms = 0;
		static eastl::fixed_vector<i32,8> showcaseNextBySlice;
		const i32 slices = eastl::max(1, Config().DevShowcaseSlices);
		const i32 slice = showcaseRooms++ % slices;
		if((i32)showcaseNextBySlice.size() < slices) showcaseNextBySlice.resize(slices, Config().DevShowcaseStart);
		i32& showcaseNext = showcaseNextBySlice[slice];
		eastl::fixed_vector<ClassType,100,false> all;
		if(!Config().devShowcaseList.empty()) {
			for(i32 c : Config().devShowcaseList) all.push_back((ClassType)c);
		}
		else {
			foreach_const(it, allowedMastersSet) all.push_back(*it);
			eastl::sort(all.begin(), all.end());
		}
		if(slices > 1) {
			const i32 count = (i32)all.size();
			all.erase(all.begin() + (slice + 1) * count / slices, all.end());
			all.erase(all.begin(), all.begin() + slice * count / slices);
			LOG("[Room] showcase slice %d/%d first %d count %d", slice + 1, slices, all.empty() ? 0 : (i32)all[0], (i32)all.size());
		}
		i32 humanTeam = -1;
		foreach_const(u, userList) if(!u->isBot) humanTeam = (i32)u->team;
		bool mirrorBotDone = false;
		bool mirrorEnemyDone = false;
		foreach(u, userList) {
			const bool mirrorBot = u->isBot && (i32)u->team == humanTeam && !mirrorBotDone;
			const bool mirrorEnemy = u->isBot && (i32)u->team != humanTeam && !mirrorEnemyDone;
			const bool mirror = Config().DevMirror && all.size() >= 2 && (!u->isBot || mirrorBot || mirrorEnemy);
			if(mirror && mirrorBot) mirrorBotDone = true;
			if(mirror && mirrorEnemy) mirrorEnemyDone = true;
			if(mirror && u->Main().classType == ClassType::NONE) {
				TryPickMaster(u, all[0]);
				TryPickMaster(u, all[1]);
				const GameXmlContent& content = GetGameXmlContent();
				for(int mi = 0; mi < 2; mi++) {
					if(u->masters[mi].classType == ClassType::NONE) continue;
					eastl::fixed_vector<SkillID,8,true> ids;
					foreach_const(sid, content.masterClassTypeMap.at(u->masters[mi].classType)->skillIDs) {
						const i32 slot = (i32)*sid % 100;
						if(slot == 10 || slot == 20 || slot == 30 || slot == 40) ids.push_back(*sid);
					}
					eastl::sort(ids.begin(), ids.end());
					const i32 first = Config().DevMirrorPass ? 2 : 0;
					if((i32)ids.size() > first + 1) {
						u->masters[mi].skills[0] = ids[first];
						u->masters[mi].skills[1] = ids[first + 1];
					}
				}
				u->replicate.masterPick = true;
				LOG("[Room] mirror %s masters %d %d skills %d %d", u->isBot ? "bot" : "human", (i32)u->Main().classType, (i32)u->Sub().classType, (i32)u->Main().skills[0], (i32)u->Main().skills[1]);
				continue;
			}
			if(!u->isBot && u->Main().classType == ClassType::NONE) {
				if(!Config().devShowcaseList.empty()) {
					TryPickMaster(u, all[0]);
					if(all.size() > 1) TryPickMaster(u, all[1]);
				}
				if(u->Main().classType == ClassType::NONE) TryPickMaster(u, ClassType::STRIKER);
				if(u->Sub().classType == ClassType::NONE) TryPickMaster(u, ClassType::LAUNCHER);
				if(u->Sub().classType == ClassType::NONE) TryPickMaster(u, ClassType::STRIKER);
				u->replicate.masterPick = true;
			}
			if(!u->isBot || all.empty()) continue;
			for(int k = 0; k < 2; k++) {
				const ClassType c = all[showcaseNext % all.size()];
				showcaseNext++;
				TryPickMaster(u, c);
			}
			if(u->Sub().classType == ClassType::NONE) TryPickMaster(u, all[showcaseNext++ % all.size()]);
			foreach_const(fallback, allowedMastersSet) {
				if(u->Main().classType != ClassType::NONE && u->Sub().classType != ClassType::NONE) break;
				TryPickMaster(u, *fallback);
			}
			u->replicate.masterPick = true;
			LOG("[Room] showcase bot masters %d %d", (i32)u->Main().classType, (i32)u->Sub().classType);
		}
	}

	if(!Config().DevShowcase && !Config().devHumanPick.empty()) {
		foreach(u, userList) {
			if(u->isBot || u->Main().classType != ClassType::NONE) continue;
			foreach_const(c, Config().devHumanPick) TryPickMaster(u, (ClassType)*c);
			u->replicate.masterPick = true;
			u->isReady = true;
			LOG("[Room] dev human masters %d %d", (i32)u->Main().classType, (i32)u->Sub().classType);
		}
	}
	if(!Config().DevShowcase && !Config().devBotPool.empty()) {
		static u32 poolNext = 0;
		foreach(u, userList) {
			if(!u->isBot) continue;
			bool humanTeam = false;
			foreach_const(h, userList) if(!h->isBot && h->team == u->team) humanTeam = true;
			for(u32 tries = 0; tries < Config().devBotPool.size() * 2 && u->Sub().classType == ClassType::NONE; tries++) {
				const i32 cls = Config().devBotPool[poolNext++ % Config().devBotPool.size()];
				if(humanTeam && (cls == Config().DevPickMasters[0] || cls == Config().DevPickMasters[1])) continue;
				TryPickMaster(u, (ClassType)cls);
			}
			u->replicate.masterPick = true;
			LOG("[Room] dev bot masters %d %d", (i32)u->Main().classType, (i32)u->Sub().classType);
		}
	}

	if(draft) {
		phase = Phase::Banning;
		StartBanRound();
	}
	else PickBotMasters();
}

void RoomInstance::PickBotMasters()
{
	foreach(u, userList) {
		if(u->isBot && !Config().DevShowcase && (Config().devBotPool.empty() || u->Sub().classType == ClassType::NONE)) {
			static const ClassType botMasters[] = {
				ClassType::STRIKER, ClassType::ARTILLERY, ClassType::ASSASSIN, ClassType::ELECTRO,
				ClassType::DEFENDER, ClassType::SNIPER, ClassType::LAUNCHER,
			};
			for(int attempts = 0; attempts < 1000; attempts++) {
				TryPickMaster(u, botMasters[RandUint() % ARRAY_COUNT(botMasters)]);

				if(u->Sub().classType != ClassType::NONE) {
					break;
				}
			}
			u->replicate.masterPick = true;
		}
	}
}

void RoomInstance::SendPickPhaseStart(const User& user)
{
	{
		PacketWriter<Sv::SN_SortieMasterPickPhaseStart,1024> packet;
		packet.Write<u8>(0);
		packet.Write<u16>((u16)allowedMastersSet.size());
		foreach_const(m, allowedMastersSet) {
			packet.Write(CreatureIndex(100000000 + (i32)*m));
			packet.Write<u16>(1);
			packet.Write<u32>(ProhibitedFor(user.team, *m) ? MEASURED_BAN_SLOT_STATE : 0);
		}
		SendPacket(user.clientHd, packet);
	}

	{
		PacketWriter<Sv::SN_SortieMasterPickPhaseStepStart> packet;
		packet.Write<i32>(PICKING_TIME);

		eastl::fixed_vector<UserID,5> allyTeamUserIds;
		eastl::fixed_vector<UserID,5> enemyTeamUserIds;

		foreach_const(u, teamRed) {
			if(user.team == Team::BLUE) enemyTeamUserIds.push_back((*u)->userID);
			else allyTeamUserIds.push_back((*u)->userID);
		}
		foreach_const(u, teamBlue) {
			if(user.team == Team::BLUE) allyTeamUserIds.push_back((*u)->userID);
			else enemyTeamUserIds.push_back((*u)->userID);
		}

		packet.WriteVec(allyTeamUserIds.data(), allyTeamUserIds.size());
		packet.WriteVec(enemyTeamUserIds.data(), enemyTeamUserIds.size());

		SendPacket(user.clientHd, packet);
	}
}

const RoomInstance::User* RoomInstance::TeamLeader(i32 team) const
{
	const User* leader = nullptr;
	foreach_const(u, userList) {
		if(u->team != team) continue;
		if(!leader || (leader->isBot && !u->isBot)) leader = &*u;
	}
	return leader;
}

void RoomInstance::StartBanRound()
{
	banRoundStart = localTime == Time::ZERO ? TimeNow() : localTime;
	bannedThisRound = {{ false, false }};
	foreach_const(cu, connectedUsers) {
		const User& user = **cu;
		const i32 enemy = user.team == Team::BLUE ? Team::RED : Team::BLUE;
		const User* allies = TeamLeader(user.team == Team::BLUE ? Team::BLUE : Team::RED);
		const User* enemies = TeamLeader(enemy);
		PacketWriter<Sv::SN_SortieMasterBanPhaseStart,2048> packet;
		packet.Write<u32>(allies ? (u32)allies->userID : 0);
		packet.Write<u32>(enemies ? (u32)enemies->userID : 0);
		packet.Write<u32>(MEASURED_BAN_TIME_SEC);
		packet.Write<u32>(MEASURED_BANS_PER_ROUND);
		packet.Write<u16>((u16)allowedMastersSet.size());
		foreach_const(m, allowedMastersSet) {
			packet.Write(CreatureIndex(100000000 + (i32)*m));
			packet.Write<u16>(1);
			const bool own = eastl::find(teamBans[user.team].begin(), teamBans[user.team].end(), *m) != teamBans[user.team].end();
			packet.Write<u32>(own ? MEASURED_BAN_SLOT_STATE : 0);
		}
		SendPacket(user.clientHd, packet);
	}
	LOG("[Room] ban round %d starts", (i32)banRound + 1);
}

bool RoomInstance::BanMaster(i32 team, ClassType classType)
{
	if(team != Team::RED && team != Team::BLUE) return false;
	if(bannedThisRound[team] || !allowedMastersSet.count(classType) || eastl::find(teamBans[team].begin(), teamBans[team].end(), classType) != teamBans[team].end()) return false;
	bannedThisRound[team] = true;
	teamBans[team].push_back(classType);
	const CreatureIndex creature = CreatureIndex(100000000 + (i32)classType);
	foreach_const(cu, connectedUsers) {
		if((*cu)->team != team) continue;
		Sv::SN_SortieMasterBan ban;
		ban.teamType = (u32)(team == Team::RED ? TeamType::RED : TeamType::BLUE);
		ban.creatureIndex = (u32)creature;
		SendPacket((*cu)->clientHd, ban);
		PacketWriter<Sv::SN_SortieCharacterSlotInfo,64> slot;
		slot.Write<u16>(1);
		slot.Write(creature);
		slot.Write<u16>(1);
		slot.Write<u32>(MEASURED_BAN_SLOT_STATE);
		SendPacket((*cu)->clientHd, slot);
	}
	LOG("[Room] team %d bans master %d", team, (i32)classType);
	return true;
}

void RoomInstance::UpdateBans()
{
	const f64 elapsed = TimeDurationSec(banRoundStart, localTime);
	for(i32 team = Team::RED; team <= Team::BLUE; team++) {
		if(bannedThisRound[team]) continue;
		const User* leader = TeamLeader(team);
		const bool botTurn = !leader || (leader->isBot && elapsed >= HYP_BOT_BAN_DELAY_SEC);
		if(!botTurn && elapsed < MEASURED_BAN_TIME_SEC) continue;
		eastl::fixed_vector<ClassType,100,false> pool;
		foreach_const(m, allowedMastersSet) if(eastl::find(teamBans[team].begin(), teamBans[team].end(), *m) == teamBans[team].end()) pool.push_back(*m);
		if(pool.empty()) { bannedThisRound[team] = true; continue; }
		BanMaster(team, pool[RandUint() % pool.size()]);
	}
	if(!bannedThisRound[Team::RED] || !bannedThisRound[Team::BLUE]) return;
	foreach_const(cu, connectedUsers) {
		const User& user = **cu;
		const i32 ally = user.team == Team::BLUE ? Team::BLUE : Team::RED;
		const i32 enemy = ally == Team::BLUE ? Team::RED : Team::BLUE;
		PacketWriter<Sv::SN_SortieMasterBanPhaseEnd,128> end;
		end.Write<u16>((u16)teamBans[ally].size());
		foreach_const(b, teamBans[ally]) end.Write(CreatureIndex(100000000 + (i32)*b));
		end.Write<u16>((u16)teamBans[enemy].size());
		foreach_const(b, teamBans[enemy]) end.Write(CreatureIndex(100000000 + (i32)*b));
		SendPacket(user.clientHd, end);
	}
	banRound++;
	if(banRound < MEASURED_BAN_ROUNDS) {
		StartBanRound();
		return;
	}
	phase = Phase::Picking;
	startTime = localTime;
	PickBotMasters();
	foreach_const(cu, connectedUsers) SendPickPhaseStart(**cu);
	LOG("[Room] bans done (%d / %d), picking starts", (i32)teamBans[Team::RED].size(), (i32)teamBans[Team::BLUE].size());
}

void RoomInstance::Update(Time localTime_)
{
	localTime = localTime_;
	if(startTime == Time::ZERO) startTime = localTime_;

	Replicate();

	if(phase == Phase::Banning) UpdateBans();

	if(phase == Phase::Picking && !devPicked && Config().DevPickMasters[0] > 0) {
		devPicked = true;
		foreach(u, userList) {
			if(u->isBot) continue;
			TryPickMaster(u, (ClassType)Config().DevPickMasters[0]);
			if(Config().DevPickMasters[1] > 0) TryPickMaster(u, (ClassType)Config().DevPickMasters[1]);
			foreach_const(m, allowedMastersSet) {
				if(u->Main().classType != ClassType::NONE && u->Sub().classType != ClassType::NONE) break;
				TryPickMaster(u, *m);
			}
			SetReady(u);
			LOG("[Room] %ls dev picks masters %d and %d", u->name.data(), Config().DevPickMasters[0], Config().DevPickMasters[1]);
		}
		Replicate();
	}

	if(phase == Phase::Picking) {
		bool allReady = true;
		foreach_const(u, userList) {
			if(!u->isReady) {
				allReady = false;
				break;
			}
		}

		bool randomPick = false;
		if(TimeDurationSec(startTime, localTime) > PICKING_TIME) {
			allReady = true;
			eastl::fixed_vector<ClassType,100,false> pool;
			foreach_const(m, allowedMastersSet) pool.push_back(*m);
			u32 pickSeed = (u32)(u64)sortieUID * 2654435761u;
			foreach(u, userList) {
				for(u32 tries = 0; tries < pool.size() * 2 && (u->Main().classType == ClassType::NONE || u->Sub().classType == ClassType::NONE) && !pool.empty(); tries++) {
					pickSeed = pickSeed * 1664525u + 1013904223u;
					if(TryPickMaster(u, pool[(pickSeed >> 8) % pool.size()])) {
						randomPick = true;
						LOG("[Room] %ls ran out of time, random master %d", u->name.data(), (i32)(u->Sub().classType != ClassType::NONE ? u->Sub().classType : u->Main().classType));
					}
				}
			}
			if(randomPick) Replicate();
		}

		// when every player is ready, create a game
		if(allReady) {
			phase = Phase::StartingMatch;
			matchWaitTime = localTime;

			MatchmakerConnector& mm = Matchmaker();

			eastl::fixed_vector<MatchmakerConnector::RoomPlayer,16,false> rpList;
			foreach_const(u, userList) {
				MatchmakerConnector::RoomPlayer rp;
				rp.accountUID = u->accountUID;
				rp.team = u->team;
				rp.isBot = u->isBot;
				rp.masters[0] = u->masters[0].classType;
				rp.masters[1] = u->masters[1].classType;
				rp.skins[0] = u->masters[0].skin;
				rp.skins[1] = u->masters[1].skin;
				rp.skills[0] = u->masters[0].skills[0];
				rp.skills[1] = u->masters[0].skills[1];
				rp.skills[2] = u->masters[1].skills[0];
				rp.skills[3] = u->masters[1].skills[1];
				rp.weapons[0] = u->masters[0].weapon;
				rp.weapons[1] = u->masters[1].weapon;
				rp.weaponGrades[0] = u->masters[0].weaponGrade;
				rp.weaponGrades[1] = u->masters[1].weaponGrade;
				rp.masterGearNo[0] = u->masters[0].masterGearNo;
				rp.masterGearNo[1] = u->masters[1].masterGearNo;
				rp.characterType[0] = u->masters[0].characterType;
				rp.characterType[1] = u->masters[1].characterType;
				rp.guildTag.len = 0;
				rp.stageSkill = u->stageSkill;
				auto account = GetAccountManager().accountMap.find(u->accountUID);
				if(!u->isBot && account != GetAccountManager().accountMap.end() && account->second->guildTag.size() <= ARRAY_COUNT(rp.guildTag.data)) rp.guildTag.Copy(account->second->guildTag);
				if(!u->isBot && account != GetAccountManager().accountMap.end()) GuildRewardBoost(account->second->nickname, &rp.guildBoostGold, &rp.guildBoostExp);
				rpList.push_back(rp);
			}

			mm.QueryRoomCreateGame(sortieUID, rpList.data(), rpList.size());

			// update clients
			Sv::SN_SortieMasterPickPhaseStep packetStep;
			packetStep.isRandomPick = randomPick ? 1 : 0;

			Sv::SN_StartCountdownSortieRoom packetCountdown;
			packetCountdown.stageType = StageType::PVP_GAME;
			packetCountdown.timeToWaitSec = MATCH_WAIT_TIME;

			foreach_const(cu, connectedUsers) {
				SendPacket((*cu)->clientHd, packetStep);
				SendPacketData<Sv::SN_SortieMasterPickPhaseEnd>((*cu)->clientHd, 0, nullptr);
				SendPacket((*cu)->clientHd, packetCountdown);
			}
		}
	}

	if(phase == Phase::MatchReady && TimeDurationSec(matchWaitTime, localTime) > MATCH_WAIT_TIME) {
		foreach_const(u, connectedUsers) {
			const User& user = **u;

			{
				PacketWriter<Sv::SN_SortiePrepareBotInfo> packet;
				const GameXmlContent& content = GetGameXmlContent();
				eastl::fixed_vector<CreatureIndex,16,false> botIndexes;
				foreach_const(bu, userList) {
					if(!bu->isBot) continue;
					const CreatureIndex botIndex = content.FindDeathMatchBotIndex(bu->masters[0].classType);
					if(botIndex == CreatureIndex::Invalid) continue;
					botIndexes.push_back(botIndex);
				}
				packet.Write<u16>((u16)botIndexes.size());
				foreach_const(id, botIndexes) {
					packet.Write<CreatureIndex>(*id);
				}
				SendPacket(user.clientHd, packet);
			}

			{
				PacketWriter<Sv::SN_SortiePrepare> packet;

				packet.Write<u16>(2); // skinList

				Sv::SN_SortiePrepare::Skin skin;
				skin.classType = user.masters[0].classType;
				skin.skinIndex = user.masters[0].skin;
				skin.bufCount = 0;
				skin.expireTime = 0;
				packet.Write(skin);

				skin.classType = user.masters[1].classType;
				skin.skinIndex = user.masters[1].skin;
				skin.bufCount = 0;
				skin.expireTime = 0;
				packet.Write(skin);

				packet.Write<u16>(4); // skillList
				packet.Write(user.masters[0].skills[0]);
				packet.Write(user.masters[0].skills[1]);
				packet.Write(user.masters[1].skills[0]);
				packet.Write(user.masters[1].skills[1]);
				SendPacket(user.clientHd, packet);
			}

			{
				PacketWriter<Sv::SN_DoConnectGameServer> packet;
				packet.Write(matchServerPort); // port
				packet.Write(matchServerIp); // ip
				packet.Write<i32>((u64)sortieUID & 0xFFFFFFFF); // gameID
				packet.Write<u32>(0x0); // idcHash
				packet.WriteStringObj(user.name.data(), user.name.size());
				packet.Write<u32>(In::ProduceInstantKey(user.accountUID, sortieUID));
				SendPacket(user.clientHd, packet);
				MarkAccountSentToGame(user.name, sortieUID, user.accountUID);
				MarkSortieGameServer(sortieUID, matchServerIp, matchServerPort);
			}
		}

		// delete room
		markedAsRemove = true;
	}
}

void RoomInstance::Replicate()
{
	bool doReplicateMasterCount = false;

	foreach(u, userList) {
		User& user = *u;
		if(user.replicate.masterPick) {
			user.replicate.masterPick = false;
			doReplicateMasterCount = true;

			Sv::SN_MasterPick pickTeam;
			pickTeam.userID = user.userID;
			pickTeam.characterSelectInfos_count = 2;

			for(int mi = 0; mi < _MASTER_COUNT; mi++) {
				if(user.masters[mi].classType != ClassType::NONE) {
					pickTeam.characterSelectInfos[mi].characterID = LocalActorID((u32)LocalActorID::FIRST_SELF_MASTER + (i32)user.masters[mi].classType);
					pickTeam.characterSelectInfos[mi].creatureIndex = CreatureIndex(100000000 + (i32)user.masters[mi].classType);
					pickTeam.characterSelectInfos[mi].skillSlot1 = user.masters[mi].skills[0];
					pickTeam.characterSelectInfos[mi].skillSlot2 = user.masters[mi].skills[1];
				}
				else {
					pickTeam.characterSelectInfos[mi].characterID = LocalActorID::INVALID;
					pickTeam.characterSelectInfos[mi].creatureIndex = CreatureIndex::Invalid;
					pickTeam.characterSelectInfos[mi].skillSlot1 = SkillID::INVALID;
					pickTeam.characterSelectInfos[mi].skillSlot2 = SkillID::INVALID;
				}
			}

			// hide info for enemies
			Sv::SN_MasterPick pickEnemy = pickTeam;
			pickEnemy.characterSelectInfos[0].skillSlot1 = SkillID::INVALID;
			pickEnemy.characterSelectInfos[0].skillSlot2 = SkillID::INVALID;
			pickEnemy.characterSelectInfos[1].characterID = LocalActorID::INVALID;
			pickEnemy.characterSelectInfos[1].creatureIndex = CreatureIndex::Invalid;
			pickEnemy.characterSelectInfos[1].skillSlot1 = SkillID::INVALID;
			pickEnemy.characterSelectInfos[1].skillSlot2 = SkillID::INVALID;

			const bool unpick = user.replicate.masterUnpick;
			user.replicate.masterUnpick = false;
			foreach_const(cu, connectedUsers) {
				const Sv::SN_MasterPick& view = (*cu)->team == u->team ? pickTeam : pickEnemy;
				if(!unpick) {
					SendPacket((*cu)->clientHd, view);
					continue;
				}
				PacketWriter<Sv::SN_MasterUnpick,64> packet;
				packet.Write<u32>((u32)view.userID);
				packet.Write<u16>(view.characterSelectInfos_count);
				for(i32 mi = 0; mi < view.characterSelectInfos_count; mi++) {
					packet.Write<u32>((u32)view.characterSelectInfos[mi].characterID);
					packet.Write<u32>((u32)view.characterSelectInfos[mi].creatureIndex);
					packet.Write<u32>((u32)view.characterSelectInfos[mi].skillSlot1);
					packet.Write<u32>((u32)view.characterSelectInfos[mi].skillSlot2);
				}
				SendPacket((*cu)->clientHd, packet);
			}
		}

		if(user.replicate.ready) {
			user.replicate.ready = false;

			Sv::SN_ReadySortieRoom packet;
			packet.userID = user.userID;
			packet.ready = 1;

			foreach_const(cu, connectedUsers) {
				SendPacket((*cu)->clientHd, packet);
			}
		}
	}

	if(doReplicateMasterCount) {
		PacketWriter<Sv::SN_SortieCharacterSlotInfo,2 + MAX_ALLOWED_MASTERS * 14> packet[2];
		u16 slotInfos_count[2] = {0};

		for(int team = 0; team < 2; team++) {
			// TODO: we *only* need to send which one have been updated, might need to do frame differential after all...
			// right now we send everything, which is not a LOT but still
			slotInfos_count[team] = allowedMastersSet.size();
			packet[team].Write<u16>(slotInfos_count[team]);

			foreach_const(m, allowedMastersSet) {
				u8 c = teamMasterPickCount[team][(i32)*m];
				DBG_ASSERT(c <= 2);

				packet[team].Write(CreatureIndex(100000000 + (i32)*m));
				packet[team].Write<u16>(2);
				for(int s = 0; s < c; s++) packet[team].Write<i32>(1);
				if(c < 2) {
					for(int s = 0; s < (2-c); s++) packet[team].Write<i32>(0);
				}
			}
		}

		foreach_const(u, connectedUsers) {
			if((*u)->team != Team::RED && (*u)->team != Team::BLUE) continue;
			SendPacket((*u)->clientHd, packet[(*u)->team]);
		}
	}
}

void RoomInstance::OnClientPacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData)
{
	const i32 packetSize = header.size - sizeof(header);

	switch(header.netID) {
		case Cl::CQ_RTT_Time::NET_ID: {
			if(!ValidatePacket<Cl::CQ_RTT_Time>(packetData, packetSize)) {
				WARN("[client%x] WARNING: invalid CQ_RTT_Time (size=%d)", clientHd, packetSize);
				break;
			}
			NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_RTT_Time>(packetData, packetSize));
			const Cl::CQ_RTT_Time& rtt = SafeCast<Cl::CQ_RTT_Time>(packetData, packetSize);
			Sv::SA_RTT_Time answer;
			answer.clientTimestamp = rtt.time;
			answer.serverTimestamp = (i64)TimeDiffMs(TimeRelNow());
			SendPacket(clientHd, answer);
		} break;

		case Cl::CQ_MasterPick::NET_ID: {
			if(!ValidatePacket<Cl::CQ_MasterPick>(packetData, packetSize)) {
				WARN("[client%x] WARNING: invalid CQ_MasterPick (size=%d)", clientHd, packetSize);
				break;
			}
			const Cl::CQ_MasterPick& packet = SafeCast<Cl::CQ_MasterPick>(packetData, packetSize);
			NT_LOG("[client%x] Client :: CQ_MasterPick :: localMasterID=0x%08x", clientHd, (u32)packet.localMasterID);
			User* user = FindUser(clientHd);
			ASSERT(user);

			ClassType selected = ClassType((u32)packet.localMasterID - (u32)LocalActorID::FIRST_SELF_MASTER);
			bool r = TryPickMaster(user, selected);

			Sv::SA_MasterPick answer;
			answer.retval = r ? 0:1; // TODO: actual error message
			answer.localMasterID = packet.localMasterID;
			SendPacket(clientHd, answer);
		} break;

		case Cl::CQ_SortieMasterBan::NET_ID: {
			if(!ValidatePacket<Cl::CQ_SortieMasterBan>(packetData, packetSize)) {
				WARN("[client%x] WARNING: invalid CQ_SortieMasterBan (size=%d)", clientHd, packetSize);
				break;
			}
			const Cl::CQ_SortieMasterBan& packet = SafeCast<Cl::CQ_SortieMasterBan>(packetData, packetSize);
			NT_LOG("[client%x] Client :: CQ_SortieMasterBan :: creature=%u", clientHd, packet.input_value_u32);
			User* user = FindUser(clientHd);
			ASSERT(user);
			const bool leader = TeamLeader(user->team) == user;
			const bool ok = phase == Phase::Banning && leader && BanMaster(user->team, ClassType((i32)packet.input_value_u32 - 100000000));
			Sv::SA_SortieMasterBan answer;
			answer.retval = ok ? 0 : HYP_BAN_REFUSED;
			SendPacket(clientHd, answer);
		} break;

		case Cl::CQ_MasterUnpick::NET_ID: {
			if(!ValidatePacket<Cl::CQ_MasterUnpick>(packetData, packetSize)) {
				WARN("[client%x] WARNING: invalid CQ_MasterUnpick (size=%d)", clientHd, packetSize);
				break;
			}
			const Cl::CQ_MasterUnpick& packet = SafeCast<Cl::CQ_MasterUnpick>(packetData, packetSize);
			NT_LOG("[client%x] Client :: CQ_MasterUnpick :: localMasterID=0x%08x", clientHd, (u32)packet.localMasterID);
			User* user = FindUser(clientHd);
			ASSERT(user);

			ClassType selected = ClassType((u32)packet.localMasterID - (u32)LocalActorID::FIRST_SELF_MASTER);
			UnpickMaster(user, selected);
			user->replicate.masterUnpick = true;
			Sv::SA_MasterUnpick answer;
			answer.retval = 0;
			answer.characterID = (u32)packet.localMasterID;
			SendPacket(clientHd, answer);
		} break;

		case Cl::CQ_MasterReset::NET_ID: {
			if(!ValidatePacket<Cl::CQ_MasterReset>(packetData, packetSize)) {
				WARN("[client%x] WARNING: invalid CQ_MasterReset (size=%d)", clientHd, packetSize);
				break;
			}
			NT_LOG("[client%x] Client :: CQ_MasterReset ::", clientHd);
			User* user = FindUser(clientHd);
			ASSERT(user);
			ResetMasters(user);
		} break;

		case Cl::CQ_ReadySortieRoom::NET_ID: {
			if(!ValidatePacket<Cl::CQ_ReadySortieRoom>(packetData, packetSize)) {
				WARN("[client%x] WARNING: invalid CQ_ReadySortieRoom (size=%d)", clientHd, packetSize);
				break;
			}
			NT_LOG("[client%x] Client :: CQ_ReadySortieRoom ::", clientHd);
			User* user = FindUser(clientHd);
			ASSERT(user);
			SetReady(user);
		} break;

		case Cl::CQ_RoomEquipSkill::NET_ID: {
			if(!ValidatePacket<Cl::CQ_RoomEquipSkill>(packetData, packetSize)) {
				WARN("[client%x] WARNING: invalid CQ_RoomEquipSkill (size=%d)", clientHd, packetSize);
				break;
			}
			NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_RoomEquipSkill>(packetData, packetSize));
			const Cl::CQ_RoomEquipSkill& packet = SafeCast<Cl::CQ_RoomEquipSkill>(packetData, packetSize);
			User* user = FindUser(clientHd);
			if(!user || user->isBot) break;
			auto found = GetAccountManager().accountMap.find(user->accountUID);
			if(found == GetAccountManager().accountMap.end()) break;
			ArsenalResult r;
			ArsenalHandle(*found->second, header.netID, packetData, packetSize, &r);
			ArsenalSend(r, [&](const auto& writer) { SendPacket(clientHd, writer); });
			if(r.result != 0) break;
			const u32 characterID = (u32)packet.characterID;
			const SkillID skill = SkillID(r.skill);
			const i32 slot = (i32)r.skillSlot;
			const ClassType cls = ClassType((i32)characterID - (i32)LocalActorID::FIRST_SELF_MASTER);
			const AccountArsenal arsenal = AccountArsenalLoad(user->name.data());
			auto master = GetGameXmlContent().masterClassTypeMap.find(cls);
			for(int mi = 0; mi < 2; mi++) {
				if(user->masters[mi].classType != cls || master == GetGameXmlContent().masterClassTypeMap.end()) continue;
				ArsenalSkills(arsenal, *master->second, user->masters[mi].skills.data());
				user->replicate.masterPick = true;
			}
			LOG("[Room] %ls equips skill %d in slot %d of class %d", user->name.data(), (i32)skill, slot, (i32)cls);
			Sv::SN_SkillSelect notify;
			notify.userId = (u32)user->userID;
			notify.characterID = characterID;
			notify.dwSkillIndex = (u32)skill;
			notify.dwSkillSlot = (u32)slot;
			SendPacket(clientHd, notify);
		} break;

		case Cl::CQ_StageSkillSelect::NET_ID: {
			if(!ValidatePacket<Cl::CQ_StageSkillSelect>(packetData, packetSize)) {
				WARN("[client%x] WARNING: invalid CQ_StageSkillSelect (size=%d)", clientHd, packetSize);
				break;
			}
			const Cl::CQ_StageSkillSelect& packet = SafeCast<Cl::CQ_StageSkillSelect>(packetData, packetSize);
			User* user = FindUser(clientHd);
			const GameXmlContent::StageMaps* stageInfo = GetGameXmlContent().FindStageMaps(stageIndex);
			if(!user || !stageInfo) break;
			bool allowed = false;
			foreach_const(c, stageInfo->stageSkillCandidates) if(*c == packet.skillIndex) allowed = true;
			if(allowed) user->stageSkill = packet.skillIndex;
			LOG("[Room] %ls selects stage skill %d (%s)", user->name.data(), (i32)packet.skillIndex, allowed ? "ok" : "refused");
			Sv::SA_StageSkillSelect ack;
			ack.errcode = allowed ? 0 : 1;
			ack.currentSkillIndex = (u32)user->stageSkill;
			SendPacket(clientHd, ack);
			if(!allowed) break;
			foreach_const(m, connectedUsers) {
				if((*m)->team != user->team) continue;
				Sv::SN_StageSkillSelect select;
				select.userId = (u32)user->userID;
				select.skillIndex = (u32)user->stageSkill;
				SendPacket((*m)->clientHd, select);
			}
		} break;

		case Cl::CQ_MasterGearRepair::NET_ID: {
			User* user = FindUser(clientHd);
			if(!user || packetSize != 5) break;
			ConstBuffer buff(packetData, packetSize);
			const u32 characterID = buff.Read<u32>();
			const u8 page = buff.Read<u8>();
			const ClassType cls = (ClassType)((i32)characterID - (i32)LocalActorID::FIRST_SELF_MASTER);
			AccountGears gears = AccountGearsLoad(user->name.data());
			if(page < 1 || page > gears.pages.size()) break;
			bool found = false;
			foreach(s, gears.selected) if(s->first == (i32)cls) { s->second = page; found = true; }
			if(!found && !gears.selected.full()) gears.selected.push_back(eastl::make_pair((i32)cls, page));
			AccountGearsSave(user->name.data(), gears);
			for(int mi = 0; mi < 2; mi++) if(user->masters[mi].classType == cls) user->masters[mi].masterGearNo = page;
			Sv::SA_MastergearSelect ack;
			ack.result = GEAR_SELECT_OK;
			ack.characterID = characterID;
			ack.masterGearNo = page;
			SendPacket(clientHd, ack);
			LOG("[Room] %ls selects gear page %d for class %d", user->name.data(), (i32)page, (i32)cls);
		} break;

		case Cl::CQ_SelectSkin::NET_ID: {
			if(!ValidatePacket<Cl::CQ_SelectSkin>(packetData, packetSize)) {
				WARN("[client%x] WARNING: invalid CQ_SelectSkin (size=%d)", clientHd, packetSize);
				break;
			}
			const Cl::CQ_SelectSkin& packet = SafeCast<Cl::CQ_SelectSkin>(packetData, packetSize);
			NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_SelectSkin>(packetData, packetSize));
			User* user = FindUser(clientHd);
			if(!user) break;
			const ClassType cls = (ClassType)packet.classType;
			const SkinIndex skin = (SkinIndex)packet.skinIndex;
			bool known = false;
			foreach_const(m, GetGameXmlContent().masters) {
				if(m->classType != cls) continue;
				foreach_const(s, m->skinIDs) if(*s == skin) known = true;
			}
			bool applied = false;
			for(int mi = 0; mi < 2 && known; mi++) {
				if(user->masters[mi].classType != cls) continue;
				user->masters[mi].skin = skin;
				user->replicate.masterPick = true;
				applied = true;
			}
			LOG("[Room] %ls skin %d for class %d: %s", user->name.data(), (i32)skin, (i32)cls, applied ? "applied" : (known ? "master not picked" : "unknown skin"));
			Sv::SA_CharacterSkinSelect ack;
			ack.result = (u32)(known ? ErrorType::SUCCESS : ErrorType::CHARACTER_SKIN_SELECT_INVALID_DATA);
			ack.classType = packet.classType;
			ack.skinIndex = packet.skinIndex;
			SendPacket(clientHd, ack);
		} break;

		case Cl::CQ_RoomSwapSkill::NET_ID: {
			if(!ValidatePacket<Cl::CQ_RoomSwapSkill>(packetData, packetSize)) {
				WARN("[client%x] WARNING: invalid CQ_RoomSwapSkill (size=%d)", clientHd, packetSize);
				break;
			}
			NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CQ_RoomSwapSkill>(packetData, packetSize));
			const Cl::CQ_RoomSwapSkill& packet = SafeCast<Cl::CQ_RoomSwapSkill>(packetData, packetSize);
			User* user = FindUser(clientHd);
			if(!user) break;
			const u32 v = packet.characterID;
			for(int mi = 0; mi < 2; mi++) {
				if(user->masters[mi].classType == ClassType::NONE) continue;
				if(v != (u32)LocalActorID::FIRST_SELF_MASTER + (u32)user->masters[mi].classType && v != (u32)user->masters[mi].classType) continue;
				eastl::swap(user->masters[mi].skills[0], user->masters[mi].skills[1]);
				user->replicate.masterPick = true;
			}
		} break;

		case MEASURED_CQ_WEAPON_UNLOCK:
		case MEASURED_CQ_WEAPON_EQUIP:
		case MEASURED_CQ_SKILL_UNLOCK:
		case MEASURED_CQ_WEAPON_UPGRADE: {
			User* user = FindUser(clientHd);
			if(!user || user->isBot) break;
			auto found = GetAccountManager().accountMap.find(user->accountUID);
			if(found == GetAccountManager().accountMap.end()) break;
			ArsenalResult r;
			if(!ArsenalHandle(*found->second, header.netID, packetData, packetSize, &r)) break;
			auto send = [&](const auto& writer) { SendPacket(clientHd, writer); };
			ArsenalSend(r, send);
			if(r.achieve.sub != GameXmlContent::AchieveSub::NONE) AchieveRaise(user->name.data(), r.achieve, send);
			if(r.result != 0 || r.answer == MEASURED_CQ_SKILL_UNLOCK) break;
			const AccountArsenal arsenal = AccountArsenalLoad(user->name.data());
			for(int mi = 0; mi < 2; mi++) {
				User::Master& m = user->masters[mi];
				if(m.classType == ClassType::NONE) continue;
				auto master = GetGameXmlContent().masterClassTypeMap.find(m.classType);
				if(master == GetGameXmlContent().masterClassTypeMap.end()) continue;
				i32 grade = 0;
				const i32 weapon = ArsenalEquippedWeapon(arsenal, *master->second, &grade);
				if(weapon < 0) continue;
				m.weapon = (WeaponIndex)weapon;
				m.weaponGrade = grade;
			}
		} break;

		case Cl::CN_ChannelChatMessage::NET_ID: {
			if(!ValidatePacket<Cl::CN_ChannelChatMessage>(packetData, packetSize)) {
				WARN("[client%x] WARNING: invalid CN_ChannelChatMessage (size=%d)", clientHd, packetSize);
				break;
			}
			NT_LOG("[client%x] Client :: %s", clientHd, PacketSerialize<Cl::CN_ChannelChatMessage>(packetData, packetSize));
		} break;

		default: {
			WARN("Unknown packet (netID=%d size=%d)", header.netID, header.size);
		}
	}
}

void RoomInstance::OnMatchmakerPacket(const NetHeader& header, const u8* packetData)
{
	const i32 packetSize = header.size - sizeof(header);

	switch(header.netID) {
		case In::MN_MatchCreated::NET_ID: {
			const In::MN_MatchCreated& created = SafeCast<In::MN_MatchCreated>(packetData, packetSize);
			if(created.sortieUID == sortieUID) {
				NT_LOG("[MM] %s", PacketSerialize<In::MN_MatchCreated>(packetData, packetSize));

				phase = Phase::MatchReady;
				matchServerIp = created.serverIp;
				matchServerPort = created.serverPort;
			}
		} break;
	}
}

RoomInstance::User* RoomInstance::FindUser(ClientHandle clientHd)
{
	// slow but good enough
	foreach_const(u, connectedUsers) {
		if((*u)->clientHd == clientHd) {
			return &**u;
		}
	}

	return nullptr;
}

static void PickLabSkillPair(const GameXmlContent::Master& master, eastl::array<SkillID,2>& skills)
{
	static const i32 pairs[6][2] = { { 10, 20 }, { 30, 40 }, { 10, 30 }, { 20, 40 }, { 10, 40 }, { 20, 30 } };
	static u32 next = RandUint() % 6;
	const i32 forced[2] = { Config().DevLabSkillA, Config().DevLabSkillB };
	const i32* pair = forced[0] > 0 ? forced : pairs[next++ % 6];
	SkillID first = SkillID::INVALID;
	SkillID second = SkillID::INVALID;
	foreach_const(sid, master.skillIDs) {
		if((i32)*sid % 100 == pair[0]) first = *sid;
		if((i32)*sid % 100 == pair[1]) second = *sid;
	}
	if(first == SkillID::INVALID || second == SkillID::INVALID) return;
	skills[0] = first;
	skills[1] = second;
}

static void PickMeasuredLoadout(const GameXmlContent::Master& master, ClassType classType, eastl::array<SkillID,2>& skills)
{
	u32 total = 0;
	for(const MeasuredSkillLoadout& l : MEASURED_SKILL_LOADOUTS) if(l.master == classType) total += l.count;
	if(total == 0) return;
	u32 roll = RandUint() % total;
	for(const MeasuredSkillLoadout& l : MEASURED_SKILL_LOADOUTS) {
		if(l.master != classType) continue;
		if(roll >= l.count) { roll -= l.count; continue; }
		SkillID first = SkillID::INVALID;
		SkillID second = SkillID::INVALID;
		foreach_const(sid, master.skillIDs) {
			if((i32)*sid % 100 == l.first) first = *sid;
			if((i32)*sid % 100 == l.second) second = *sid;
		}
		if(first == SkillID::INVALID || second == SkillID::INVALID) return;
		skills[0] = first;
		skills[1] = second;
		return;
	}
}

bool RoomInstance::ProhibitedFor(i32 team, ClassType classType) const
{
	if(team != Team::RED && team != Team::BLUE) return false;
	const auto& bans = teamBans[team == Team::RED ? Team::BLUE : Team::RED];
	return eastl::find(bans.begin(), bans.end(), classType) != bans.end();
}

bool RoomInstance::TryPickMaster(User* user, ClassType classType)
{
	if(ProhibitedFor(user->team, classType)) return false;
	if(teamMasterPickCount[user->team][(i32)classType] < 2) {
		const GameXmlContent& content = GetGameXmlContent();
		const GameXmlContent::Master& master = *content.masterClassTypeMap.at(classType);

		if(user->Main().classType == ClassType::NONE) {
			user->Main().classType = classType;
			user->Main().skills[0] = master.skillIDs[0];
			user->Main().skills[1] = master.skillIDs[1];
			if(user->isBot && !Config().DevShowcase) PickMeasuredLoadout(master, classType, user->Main().skills);
			if(!master.fairPvpWeaponIDs.empty()) user->Main().weapon = master.fairPvpWeaponIDs[0];
			else if(!master.defaultWeaponIDs.empty()) user->Main().weapon = master.defaultWeaponIDs[0];
			else {
				ASSERT(!master.weaponIDs.empty());
				user->Main().weapon = master.weaponIDs.size() > 1 ? master.weaponIDs[1] : master.weaponIDs[0];
			}
			user->Main().weaponGrade = 0;
			if(!user->isBot) {
				const AccountArsenal arsenal = AccountArsenalLoad(user->name.data());
				i32 grade = 0;
				const i32 owned = ArsenalEquippedWeapon(arsenal, master, &grade);
				if(owned >= 0) {
					user->Main().weapon = (WeaponIndex)owned;
					user->Main().weaponGrade = grade;
				}
				ArsenalSkills(arsenal, master, user->Main().skills.data());
			}
			if(Config().DevLabSkills) PickLabSkillPair(master, user->Main().skills);
			user->Main().masterGearNo = user->isBot ? 1 : AccountGearsLoad(user->name.data()).SelectedPage((i32)classType);
			user->Main().characterType = 1;
		}
		else if(user->Main().classType != classType) {
			user->Sub().classType = classType;
			user->Sub().skills[0] = master.skillIDs[0];
			user->Sub().skills[1] = master.skillIDs[1];
			if(user->isBot && !Config().DevShowcase) PickMeasuredLoadout(master, classType, user->Sub().skills);
			if(!master.fairPvpWeaponIDs.empty()) user->Sub().weapon = master.fairPvpWeaponIDs[0];
			else if(!master.defaultWeaponIDs.empty()) user->Sub().weapon = master.defaultWeaponIDs[0];
			else {
				ASSERT(!master.weaponIDs.empty());
				user->Sub().weapon = master.weaponIDs.size() > 1 ? master.weaponIDs[1] : master.weaponIDs[0];
			}
			user->Sub().weaponGrade = 0;
			if(!user->isBot) {
				const AccountArsenal arsenal = AccountArsenalLoad(user->name.data());
				i32 grade = 0;
				const i32 owned = ArsenalEquippedWeapon(arsenal, master, &grade);
				if(owned >= 0) {
					user->Sub().weapon = (WeaponIndex)owned;
					user->Sub().weaponGrade = grade;
				}
				ArsenalSkills(arsenal, master, user->Sub().skills.data());
			}
			if(Config().DevLabSkills) PickLabSkillPair(master, user->Sub().skills);
			user->Sub().masterGearNo = user->isBot ? 1 : AccountGearsLoad(user->name.data()).SelectedPage((i32)classType);
			user->Sub().characterType = 1;
		}
		else {
			return false;
		}

		teamMasterPickCount[user->team][(i32)classType]++;
		user->replicate.masterPick = true;
		return true;
	}

	return false;
}

void RoomInstance::UnpickMaster(User* user, ClassType classType)
{
	u8& masterPickCount = teamMasterPickCount[user->team][(i32)classType];

	foreach(m, user->masters) {
		if(m->classType == classType) {
			m->classType = ClassType::NONE;

			DBG_ASSERT(masterPickCount > 0);
			if(masterPickCount > 0) {
				masterPickCount--;
			}

			user->replicate.masterPick = true;
			break;
		}
	}
}

void RoomInstance::ResetMasters(User* user)
{
	bool changed = false;
	foreach(m, user->masters) {
		if(m->classType != ClassType::NONE) {
			u8& masterPickCount = teamMasterPickCount[user->team][(i32)m->classType];
			DBG_ASSERT(masterPickCount > 0);
			if(masterPickCount > 0) {
				masterPickCount--;
			}

			m->classType = ClassType::NONE;
			changed = true;
		}
	}

	if(changed) {
		user->replicate.masterPick = true;
	}
}

void RoomInstance::SetReady(User* user)
{
	user->isReady = true;
	user->replicate.ready = true;
}

void RoomInstance::SendDbgMsg(ClientHandle clientHd, const wchar* msg)
{
	PacketWriter<Sv::SN_ChatChannelMessage> packet;

	packet.Write<i32>(1); // chatType
	packet.WriteStringObj(L"System");
	packet.Write<u8>(0); // senderStaffType
	packet.WriteStringObj(msg);

	SendPacket(clientHd, packet);
}
