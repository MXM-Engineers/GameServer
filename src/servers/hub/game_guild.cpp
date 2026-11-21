#include "game.h"
#include "coordinator.h"
#include "guild_store.h"
#include "mailbox.h"
#include "town_rules.h"
#include <mxm/mission_rules.h>
#include <mxm/game_content.h>
#include <mxm/account_progress.h>
#include <EASTL/algorithm.h>
#include <EASTL/sort.h>
#include <time.h>

static i64 NowFiletime()
{
	return FILETIME_UNIX_EPOCH_100NS + (i64)time(nullptr) * 10000000LL;
}

template<typename String>
static bool ReadWideString(ConstBuffer& buff, String* out, i32 maxLen)
{
	if(!buff.CanRead(2)) return false;
	const u16 len = buff.Read<u16>();
	if(len > maxLen || !buff.CanRead(len * 2)) return false;
	out->assign((const wchar*)buff.ReadRaw(len * 2), len);
	return true;
}

static bool ValidGuildText(const WideString& text, i32 minLen, i32 maxLen)
{
	if((i32)text.size() < minLen || (i32)text.size() > maxLen) return false;
	foreach_const(c, text) {
		const wchar w = *c;
		const bool letter = (w >= L'a' && w <= L'z') || (w >= L'A' && w <= L'Z') || (w >= L'0' && w <= L'9');
		if(!letter) return false;
	}
	return true;
}

static i32 GuildLevel(const GuildRecord& guild)
{
	return MAX(1, GetGameXmlContent().GuildLevelForPoints(guild.point));
}

static const i64 FILETIME_PER_HOUR = 36000000000LL;

static i32 GuildSkillLevel(const GuildRecord& guild, u8 type)
{
	foreach_const(s, guild.skills) {
		if(s->type != type) continue;
		if(type > MEASURED_GUILD_SKILL_LIMITED_START && s->expiry <= NowFiletime()) return 0;
		return s->level;
	}
	return 0;
}

static i32 GuildSkillBonus(const GuildRecord& guild, u8 type)
{
	return GetGameXmlContent().GuildSkillValueByType(type, GuildSkillLevel(guild, type));
}

static i32 GuildMemberMax(const GuildRecord& guild)
{
	return MEASURED_GUILD_BASE_MEMBER_MAX + GuildSkillBonus(guild, MEASURED_GUILD_SKILL_MEMBER_MAX);
}

static bool ServerWeekend()
{
	const time_t now = time(nullptr);
	tm utc;
	gmtime_s(&utc, &now);
	return utc.tm_wday == 0 || utc.tm_wday == 6;
}

static i32 GuildDailyCap(const GuildRecord& guild, u8 skillType)
{
	const GameXmlContent& content = GetGameXmlContent();
	return (ServerWeekend() ? content.guildActivityCapWeekend : content.guildActivityCapWeekday) + GuildSkillBonus(guild, skillType);
}

static const GameXmlContent::GuildSkillLevel* FindGuildSkillLevel(const GameXmlContent::GuildSkillInfo& info, i32 level)
{
	foreach_const(l, info.levels) if(l->level == level) return &*l;
	return nullptr;
}

static GuildSkillRecord* FindGuildSkillRecord(GuildRecord& guild, u8 type)
{
	foreach(s, guild.skills) if(s->type == type) return &*s;
	return nullptr;
}

template<typename Writer>
static void WriteGuildRights(Writer& packet, const eastl::array<u8,10>& rights)
{
	for(u8 v : rights) packet.template Write<u8>(v);
}

static bool ReadGuildRights(ConstBuffer& buff, eastl::array<u8,10>* rights)
{
	if(!buff.CanRead(10)) return false;
	for(u8& v : *rights) v = buff.Read<u8>() ? 1 : 0;
	return true;
}

static const GuildClass* FindGuildClass(const GuildRecord& guild, i32 membershipID)
{
	foreach_const(c, guild.classes) if(c->id == membershipID) return &*c;
	return nullptr;
}

static bool HasGuildRight(const GuildRecord& guild, const GuildMemberRecord& member, i32 right)
{
	if(member.membershipID == MEASURED_GUILD_MASTER_MEMBERSHIP) return true;
	const GuildClass* c = FindGuildClass(guild, member.membershipID);
	return c && c->rights[right] != 0;
}

template<typename Writer>
static void WriteGuildMemberProfile(Writer& packet, const GuildMemberRecord& member, u16 leaderClass)
{
	const AccountProgress progress = AccountProgressLoad(member.nickname.data());
	packet.WriteStringObj(member.nickname.data());
	packet.template Write<i32>(member.membershipID);
	packet.template Write<u16>(progress.level);
	packet.template Write<u16>(leaderClass);
	packet.template Write<u16>((u16)GetGameXmlContent().masters.size());
	packet.template Write<i32>(0);
	packet.template Write<u8>(0);
	packet.template Write<u16>(0);
	packet.template Write<i32>(member.contributedPoint);
	packet.template Write<i32>(member.contributedFund);
	packet.template Write<u16>(0);
	packet.template Write<u16>(0);
	packet.template Write<i64>(member.lastLogout);
}

u16 HubGame::GuildLeaderClass(const WideString& nickname)
{
	const i32 memberID = FindUserByNickname(nickname);
	if(memberID < 0) return 0;
	WorldHub::ActorPlayer* actor = world.FindPlayerActor(playerActorUID[memberID]);
	return actor ? (u16)actor->classType : 0;
}

void HubGame::SendGuildResult(ClientHandle clientHd, u16 netID, u32 result)
{
	PacketWriter<Sv::SA_QuitGuild> packet;
	packet.Write<u32>(result);
	replication.server->SendPacketData(clientHd, netID, (u16)packet.size, packet.data);
}

void HubGame::SendGuildNickResult(ClientHandle clientHd, u16 netID, u32 result, const WideString& text)
{
	replication.SendNicknameResult(clientHd, netID, result, text.data());
}

void HubGame::SendMyGuild(ClientHandle clientHd, const GuildRecord* guild, u8 rollcall)
{
	PacketWriter<Sv::SN_MyGuild> packet;
	packet.WriteStringObj(guild ? guild->tag.data() : L"");
	packet.Write<i64>(guild ? guild->dissolution : 0);
	packet.Write<u8>(rollcall);
	replication.SendWriter(clientHd, packet);
}

void HubGame::SendGuildChannelEnter(ClientHandle clientHd, const GuildRecord& guild, const WideString& nickname, u8 status)
{
	PacketWriter<Sv::SN_GuildChannelEnter> packet;
	packet.WriteStringObj(guild.name.data());
	packet.WriteStringObj(nickname.data());
	packet.Write<u8>(status);
	replication.SendWriter(clientHd, packet);
}

void HubGame::SendGuildStatus(ClientHandle clientHd, const WideString* nicks, const u8* states, i32 count)
{
	PacketWriter<Sv::SN_GuildMemberStatus,2048> packet;
	packet.Write<u16>((u16)count);
	for(i32 i = 0; i < count; i++) {
		packet.WriteStringObj(nicks[i].data());
		packet.Write<u8>(states[i]);
	}
	replication.SendWriter(clientHd, packet);
}

void HubGame::SendGuildPointAndFund(const GuildRecord& guild)
{
	PacketWriter<Sv::SN_GuildpointandfundChange> packet;
	packet.Write<i64>(NowFiletime());
	packet.Write<u32>((u32)guild.point);
	packet.Write<u32>(0);
	packet.Write<u32>((u32)guild.fund);
	ForEachOnlineGuildMember(guild, [&](i32, ClientHandle hd) { replication.SendWriter(hd, packet); });
}

void HubGame::GuildOnConnect(i32 userID)
{
	GuildStore& store = GetGuildStore();
	std::lock_guard<std::recursive_mutex> lock(store.mutex);
	const WideString& nick = playerAccountData[userID]->nickname;
	GuildRecord* guild = store.FindByMember(nick);
	if(!guild) return;
	const ClientHandle clientHd = playerMap[userID]->clientHd;
	GuildMemberRecord& me = *store.FindMember(*guild, nick);
	const i64 day = ComputeMissionCalendar((i64)time(nullptr)).periodDay;
	if(guild->rollcallDay != day) {
		guild->rollcallDay = day;
		guild->todayRollCall = 0;
	}
	if(me.lastRollcallDay != day) {
		me.lastRollcallDay = day;
		guild->todayRollCall++;
		const i32 points = GetGameXmlContent().guildRollcallPoint;
		const i32 before = GuildLevel(*guild);
		guild->point += points;
		guild->fund += points;
		me.contributedPoint += points;
		GuildLevelCheck(*guild, before);
		SendGuildPointAndFund(*guild);
		if(me.joinedDay != day) SendGuildRewardMail(nick, MEASURED_GUILD_ROLLCALL_MAIL_SUBJECT, MEASURED_GUILD_ROLLCALL_MAIL_CONTENT, GetGameXmlContent().guildMedalItem, MEASURED_GUILD_ROLLCALL_MEDALS);
		LOG("[client%x] Guild :: %ls roll call in %ls (+%d)", clientHd, nick.data(), guild->name.data(), points);
	}
	auto account = GetAccountManager().accountMap.find(playerAccountData[userID]->UID);
	if(account != GetAccountManager().accountMap.end() && account->second->pendingGuildPoints > 0) {
		const i32 points = account->second->pendingGuildPoints;
		account->second->pendingGuildPoints = 0;
		const i32 before = GuildLevel(*guild);
		guild->point += points;
		guild->fund += points;
		me.contributedPoint += points;
		GuildLevelCheck(*guild, before);
		SendGuildPointAndFund(*guild);
		LOG("[client%x] Guild :: %ls brings %d point(s) from games to %ls", clientHd, nick.data(), points, guild->name.data());
	}
	store.Save();
	eastl::fixed_vector<WideString,64,false> nicks;
	eastl::fixed_vector<u8,64,false> states;
	ForEachOnlineGuildMember(*guild, [&](i32 memberID, ClientHandle hd) {
		if(hd != clientHd) {
			SendGuildChannelEnter(hd, *guild, nick, 0);
			const u8 online = MEASURED_FRIEND_STATE_ONLINE;
			SendGuildStatus(hd, &nick, &online, 1);
		}
		if(!nicks.full()) {
			nicks.push_back(playerAccountData[memberID]->nickname);
			states.push_back(MEASURED_FRIEND_STATE_ONLINE);
		}
	});
	SendGuildStatus(clientHd, nicks.data(), states.data(), (i32)nicks.size());
}

void HubGame::GuildOnDisconnect(i32 userID)
{
	GuildStore& store = GetGuildStore();
	std::lock_guard<std::recursive_mutex> lock(store.mutex);
	const WideString& nick = playerAccountData[userID]->nickname;
	GuildRecord* guild = store.FindByMember(nick);
	if(!guild) return;
	store.FindMember(*guild, nick)->lastLogout = NowFiletime();
	store.Save();
	PacketWriter<Sv::SN_GuildchannelLeave> packet;
	packet.WriteStringObj(guild->name.data());
	packet.WriteStringObj(nick.data());
	const ClientHandle self = playerMap[userID]->clientHd;
	ForEachOnlineGuildMember(*guild, [&](i32, ClientHandle hd) { if(hd != self) replication.SendWriter(hd, packet); });
}

void HubGame::GuildBroadcastState(i32 userID, u8 state)
{
	GuildStore& store = GetGuildStore();
	std::lock_guard<std::recursive_mutex> lock(store.mutex);
	const WideString& nick = playerAccountData[userID]->nickname;
	GuildRecord* guild = store.FindByMember(nick);
	if(!guild || state == MEASURED_FRIEND_STATE_OFFLINE) return;
	ForEachOnlineGuildMember(*guild, [&](i32, ClientHandle hd) { SendGuildStatus(hd, &nick, &state, 1); });
}

bool HubGame::GuildChat(i32 userID, const wchar* msg, i32 msgLen)
{
	GuildStore& store = GetGuildStore();
	std::lock_guard<std::recursive_mutex> lock(store.mutex);
	const WideString& nick = playerAccountData[userID]->nickname;
	GuildRecord* guild = store.FindByMember(nick);
	if(!guild) return true;
	ForEachOnlineGuildMember(*guild, [&](i32, ClientHandle hd) { replication.SendChatMessageToClient(hd, nick.data(), MEASURED_CHAT_TYPE_GUILD, msg, msgLen); });
	return true;
}

void HubGame::SendGuildConstraint(const GuildRecord& guild)
{
	PacketWriter<Sv::SN_GuildconstraintChange> packet;
	packet.Write<i64>(NowFiletime());
	packet.Write<u8>((u8)GuildMemberMax(guild));
	packet.Write<u8>((u8)GuildSkillLevel(guild, MEASURED_GUILD_SKILL_EMBLEM));
	packet.Write<u8>((u8)GuildSkillLevel(guild, MEASURED_GUILD_SKILL_MISSION));
	packet.Write<u16>((u16)GuildDailyCap(guild, MEASURED_GUILD_SKILL_STAGE_POINT));
	packet.Write<u16>((u16)GuildDailyCap(guild, MEASURED_GUILD_SKILL_ARENA_POINT));
	ForEachOnlineGuildMember(guild, [&](i32, ClientHandle hd) { replication.SendWriter(hd, packet); });
}

void HubGame::SendGuildRewardMail(const WideString& nick, const wchar* subject, const wchar* content, i32 item, i32 count)
{
	if(item <= 0 || count <= 0) return;
	const MailItem attachment = { item, count };
	MailMessage mail;
	if(!MailSendSystem(nick.data(), subject, content, &attachment, 1, &mail)) return;
	const i32 memberID = FindUserByNickname(nick);
	if(memberID >= 0 && playerMap[memberID] != playerList.end()) {
		replication.SendMailNewInbox(playerMap[memberID]->clientHd, mail);
		replication.SendMailUnreadNotice(playerMap[memberID]->clientHd, nick.data());
	}
}

void HubGame::GuildLevelCheck(GuildRecord& guild, i32 levelBefore)
{
	const i32 levelAfter = GuildLevel(guild);
	if(levelAfter <= levelBefore) return;
	const GameXmlContent& content = GetGameXmlContent();
	i32 medals = 0;
	foreach_const(l, content.guildLevels) if(l->level > levelBefore && l->level <= levelAfter) medals += l->medalGiftCount;
	foreach_const(m, guild.members) SendGuildRewardMail(m->nickname, MEASURED_GUILD_LEVELUP_MAIL_SUBJECT, MEASURED_GUILD_LEVELUP_MAIL_CONTENT, content.guildMedalItem, medals);
	SendGuildConstraint(guild);
	GetGuildStore().AddHistory(guild, MEASURED_GUILD_HISTORY_LEVEL, MEASURED_GUILD_EVENT_LEVELUP, NowFiletime(), WideString(), WideString(), levelBefore, levelAfter);
	PacketWriter<Sv::SN_GuildLevelup> packet;
	packet.Write<u8>((u8)levelBefore);
	packet.Write<u8>((u8)levelAfter);
	ForEachOnlineGuildMember(guild, [&](i32, ClientHandle hd) { replication.SendWriter(hd, packet); });
}

void HubGame::SendGuildProfile(ClientHandle clientHd, const GuildRecord* guild)
{
	PacketWriter<Sv::SA_GetGuildProfile,2048> packet;
	if(!guild) {
		packet.Write<i32>(MEASURED_GUILD_NOT_JOINED);
		replication.server->SendPacketData(clientHd, Sv::SA_GetGuildProfile::NET_ID, (u16)packet.size, packet.data);
		return;
	}
	const GameXmlContent& content = GetGameXmlContent();
	packet.Write<i32>(0);
	packet.WriteStringObj(guild->name.data());
	packet.WriteStringObj(guild->tag.data());
	packet.Write<i32>(guild->emblemIndex);
	packet.Write<u8>((u8)GuildLevel(*guild));
	packet.Write<u8>((u8)GuildMemberMax(*guild));
	const GuildMemberRecord* master = nullptr;
	foreach_const(m, guild->members) if(m->membershipID == MEASURED_GUILD_MASTER_MEMBERSHIP) master = &*m;
	packet.WriteStringObj(master ? master->nickname.data() : L"");
	packet.Write<i64>(guild->created);
	packet.Write<i64>(guild->dissolution);
	packet.Write<u8>(guild->joinType);
	for(u8 v : guild->interest) packet.Write<u8>(v);
	packet.WriteStringObj(guild->intro.data(), (i32)guild->intro.size());
	packet.WriteStringObj(guild->notice.data(), (i32)guild->notice.size());
	packet.Write<i32>(guild->point);
	packet.Write<i32>(guild->fund);
	packet.Write<i32>(0);
	packet.Write<u16>(0);
	packet.Write<u16>(0);
	packet.Write<u16>(0);
	packet.Write<i32>(-1);
	packet.Write<u16>((u16)guild->classes.size());
	foreach_const(c, guild->classes) {
		packet.Write<i32>(c->id);
		packet.Write<u8>(c->type);
		packet.Write<u8>(c->iconIndex);
		packet.WriteStringObj(c->name.data());
		for(u8 v : c->rights) packet.Write<u8>(v);
	}
	packet.Write<u16>((u16)guild->skills.size());
	foreach_const(s, guild->skills) {
		packet.Write<u8>(s->type);
		packet.Write<u8>(s->level);
		packet.Write<i64>(s->expiry);
		packet.Write<u16>(s->extensionCount);
	}
	packet.Write<i32>(0);
	packet.Write<i32>(GuildDailyCap(*guild, MEASURED_GUILD_SKILL_STAGE_POINT));
	packet.Write<i32>(0);
	packet.Write<i32>(GuildDailyCap(*guild, MEASURED_GUILD_SKILL_ARENA_POINT));
	packet.Write<u8>((u8)guild->todayRollCall);
	replication.SendWriter(clientHd, packet);
}

void HubGame::SendGuildMemberList(ClientHandle clientHd, const GuildRecord* guild)
{
	PacketWriter<Sv::SA_GetGuildMemberList,8192> packet;
	packet.Write<i32>(guild ? 0 : MEASURED_GUILD_NOT_JOINED);
	packet.Write<u16>(guild ? (u16)guild->members.size() : 0);
	if(guild) foreach_const(m, guild->members) WriteGuildMemberProfile(packet, *m, GuildLeaderClass(m->nickname));
	replication.SendWriter(clientHd, packet);
	if(!guild) return;
	eastl::fixed_vector<WideString,64,false> nicks;
	eastl::fixed_vector<u8,64,false> states;
	ForEachOnlineGuildMember(*guild, [&](i32 memberID, ClientHandle) {
		if(nicks.full()) return;
		nicks.push_back(playerAccountData[memberID]->nickname);
		states.push_back(MEASURED_FRIEND_STATE_ONLINE);
	});
	SendGuildStatus(clientHd, nicks.data(), states.data(), (i32)nicks.size());
}

void HubGame::SendGuildHistory(ClientHandle clientHd, const GuildRecord* guild)
{
	PacketWriter<Sv::SA_GetGuildHistoryList,8192> packet;
	packet.Write<i32>(guild ? 0 : MEASURED_GUILD_NOT_JOINED);
	packet.Write<u16>(guild ? (u16)guild->history.size() : 0);
	if(guild) foreach_const(h, guild->history) {
		packet.Write<u8>(h->historyType);
		packet.Write<u8>(h->eventType);
		packet.Write<i64>(h->date);
		packet.WriteStringObj(h->s1.data());
		packet.WriteStringObj(h->s2.data());
		packet.Write<i32>(h->l1);
		packet.Write<i32>(h->l2);
	}
	replication.SendWriter(clientHd, packet);
}

void HubGame::SendGuildPublicProfile(ClientHandle clientHd, const GuildRecord* guild, u16 netID, const WideString* memberNick)
{
	PacketWriter<Sv::SA_GetGuildpublicprofile,2048> packet;
	packet.Write<u32>(guild ? 0 : MEASURED_GUILD_NOT_FOUND);
	if(guild) {
		packet.WriteStringObj(guild->name.data());
		packet.WriteStringObj(guild->tag.data());
		packet.Write<u32>((u32)guild->emblemIndex);
		packet.Write<u8>((u8)GuildLevel(*guild));
		packet.Write<u8>((u8)GuildMemberMax(*guild));
		const GuildMemberRecord* master = nullptr;
		foreach_const(m, guild->members) if(m->membershipID == MEASURED_GUILD_MASTER_MEMBERSHIP) master = &*m;
		packet.WriteStringObj(master ? master->nickname.data() : L"");
		packet.Write<i64>(guild->created);
		packet.Write<i64>(guild->dissolution);
		packet.Write<u8>(guild->joinType);
		for(u8 v : guild->interest) packet.Write<u8>(v);
		packet.WriteStringObj(guild->intro.data(), (i32)guild->intro.size());
		packet.Write<u8>((u8)guild->members.size());
		if(memberNick) packet.WriteStringObj(memberNick->data());
	}
	replication.server->SendPacketData(clientHd, netID, (u16)packet.size, packet.data);
}

void HubGame::GuildAddMember(GuildRecord& guild, const WideString& nick)
{
	GuildStore& store = GetGuildStore();
	GuildMemberRecord member;
	member.nickname = nick;
	member.membershipID = MEASURED_GUILD_REGULAR_MEMBERSHIP;
	member.lastLogout = NowFiletime();
	member.joinedDay = ComputeMissionCalendar((i64)time(nullptr)).periodDay;
	guild.members.push_back(member);
	guild.joinRequests.erase(eastl::remove(guild.joinRequests.begin(), guild.joinRequests.end(), nick), guild.joinRequests.end());
	guild.invitations.erase(eastl::remove_if(guild.invitations.begin(), guild.invitations.end(), [&](const GuildInvitation& i) { return i.invitee == nick; }), guild.invitations.end());
	store.AddHistory(guild, MEASURED_GUILD_HISTORY_MANAGEMENT, MEASURED_GUILD_EVENT_JOIN, NowFiletime(), nick, WideString(), 0, 0);
	store.Save();

	PacketWriter<Sv::SN_GuildmemberJoin,512> joined;
	WriteGuildMemberProfile(joined, guild.members.back(), GuildLeaderClass(nick));
	const i32 newID = FindUserByNickname(nick);
	ForEachOnlineGuildMember(guild, [&](i32 memberID, ClientHandle hd) {
		if(memberID == newID) return;
		replication.SendWriter(hd, joined);
		SendGuildChannelEnter(hd, guild, nick, MEASURED_FRIEND_STATE_ONLINE);
	});
	if(newID >= 0 && playerMap[newID] != playerList.end()) {
		const ClientHandle hd = playerMap[newID]->clientHd;
		PacketWriter<Sv::SN_GuildJoinapproval> approval;
		approval.WriteStringObj(guild.name.data());
		approval.WriteStringObj(guild.tag.data());
		replication.SendWriter(hd, approval);
		SendMyGuild(hd, &guild, 0);
		SendGuildChannelEnter(hd, guild, nick, 0);
		auto found = GetAccountManager().accountMap.find(playerAccountData[newID]->UID);
		if(found != GetAccountManager().accountMap.end()) found->second->guildTag = guild.tag;
	}
	LOG("Guild :: %ls joins %ls", nick.data(), guild.name.data());
}

void HubGame::GuildRemoveMember(GuildRecord& guild, const WideString& nick, bool kicked)
{
	GuildStore& store = GetGuildStore();
	PacketWriter<Sv::SN_GuildmemberQuit> quit;
	quit.WriteStringObj(nick.data());
	quit.Write<u8>(kicked ? 1 : 0);
	PacketWriter<Sv::SN_GuildchannelLeave> leave;
	leave.WriteStringObj(guild.name.data());
	leave.WriteStringObj(nick.data());
	ForEachOnlineGuildMember(guild, [&](i32, ClientHandle hd) {
		replication.SendWriter(hd, quit);
		replication.SendWriter(hd, leave);
	});
	guild.members.erase(eastl::remove_if(guild.members.begin(), guild.members.end(), [&](const GuildMemberRecord& m) { return m.nickname == nick; }), guild.members.end());
	store.AddHistory(guild, MEASURED_GUILD_HISTORY_MANAGEMENT, kicked ? MEASURED_GUILD_EVENT_KICK : MEASURED_GUILD_EVENT_QUIT, NowFiletime(), nick, WideString(), 0, 0);
	store.Save();
	const i32 leftID = FindUserByNickname(nick);
	if(leftID >= 0 && playerMap[leftID] != playerList.end()) {
		SendMyGuild(playerMap[leftID]->clientHd, nullptr, 0);
		auto found = GetAccountManager().accountMap.find(playerAccountData[leftID]->UID);
		if(found != GetAccountManager().accountMap.end()) found->second->guildTag.clear();
	}
	LOG("Guild :: %ls %s %ls", nick.data(), kicked ? "kicked from" : "leaves", guild.name.data());
}

void HubGame::OnGuildPacket(ClientHandle clientHd, u16 netID, const u8* data, i32 size)
{
	const i32 userID = plidMap->Get(clientHd);
	ASSERT(playerAccountData[userID]);
	auto foundAccount = GetAccountManager().accountMap.find(playerAccountData[userID]->UID);
	ASSERT(foundAccount != GetAccountManager().accountMap.end());
	Account& account = *foundAccount->second;
	const WideString& nick = account.nickname;
	const GameXmlContent& content = GetGameXmlContent();
	GuildStore& store = GetGuildStore();
	std::lock_guard<std::recursive_mutex> lock(store.mutex);
	GuildRecord* guild = store.FindByMember(nick);
	GuildMemberRecord* me = guild ? store.FindMember(*guild, nick) : nullptr;
	ConstBuffer buff(data, size);
	char hex[3 * 32 + 1] = {};
	for(i32 i = 0; i < size && i < 32; i++) snprintf(hex + i * 3, 4, "%02x ", data[i]);
	NT_LOG("[client%x] Client :: guild packet %u size=%d :: %s", clientHd, (u32)netID, size, hex);

	switch(netID) {
		case Cl::CQ_GetGuildProfile::NET_ID: SendGuildProfile(clientHd, guild); break;
		case Cl::CQ_GetGuildMemberList::NET_ID: SendGuildMemberList(clientHd, guild); break;
		case Cl::CQ_GetGuildHistoryList::NET_ID: SendGuildHistory(clientHd, guild); break;

		case Cl::CQ_GetGuildMemberStatusList::NET_ID: {
			if(!guild) break;
			eastl::fixed_vector<WideString,64,false> nicks;
			eastl::fixed_vector<u8,64,false> states;
			ForEachOnlineGuildMember(*guild, [&](i32 memberID, ClientHandle) {
				if(nicks.full()) return;
				nicks.push_back(playerAccountData[memberID]->nickname);
				states.push_back(MEASURED_FRIEND_STATE_ONLINE);
			});
			SendGuildStatus(clientHd, nicks.data(), states.data(), (i32)nicks.size());
		} break;

		case Cl::CQ_CreateGuild::NET_ID: {
			WideString name, tag;
			if(!ReadWideString(buff, &name, 32) || !ReadWideString(buff, &tag, 8)) break;
			u32 result = 0;
			if(guild) result = MEASURED_GUILD_ALREADY_JOINED;
			else if(account.level < content.guildFoundationLevel) result = MEASURED_GUILD_NOT_ALLOWED_LEVEL_TO_CREATE;
			else if(account.gold < content.guildFoundationGold) result = MEASURED_GUILD_NOT_ENOUGH_GOLD_TO_CREATE;
			else if(!ValidGuildText(name, 2, 15)) result = MEASURED_GUILD_ILLEGAL_NAME;
			else if(!ValidGuildText(tag, 2, 4)) result = MEASURED_GUILD_ILLEGAL_TAG;
			else if(store.FindByName(name)) result = MEASURED_GUILD_DUPLICATED_NAME;
			PacketWriter<Sv::SA_CreateGuild,512> answer;
			answer.Write<u32>(result);
			answer.WriteStringObj(name.data());
			answer.WriteStringObj(tag.data());
			answer.Write<u8>(0);
			answer.Write<u16>(0);
			answer.Write<u32>(0);
			if(result != 0) {
				replication.SendWriter(clientHd, answer);
				break;
			}
			AccountProgress progress = AccountProgressLoad(nick.data());
			progress.gold -= content.guildFoundationGold;
			AccountProgressSave(nick.data(), progress);
			account.gold = progress.gold;
			GuildRecord created;
			created.id = store.nextID++;
			created.name = name;
			created.tag = tag;
			created.emblemIndex = content.validGuildEmblems.empty() ? 0 : content.validGuildEmblems[0];
			created.created = NowFiletime();
			created.joinType = MEASURED_GUILD_JOIN_PUBLIC;
			GuildClass regular;
			regular.id = MEASURED_GUILD_REGULAR_MEMBERSHIP;
			regular.type = MEASURED_GUILD_CLASS_REGULAR;
			regular.iconIndex = MEASURED_GUILD_REGULAR_ICON;
			regular.name = L"Member";
			created.classes.push_back(regular);
			GuildMemberRecord master;
			master.nickname = nick;
			master.membershipID = MEASURED_GUILD_MASTER_MEMBERSHIP;
			master.lastLogout = created.created;
			master.lastRollcallDay = ComputeMissionCalendar((i64)time(nullptr)).periodDay;
			master.joinedDay = master.lastRollcallDay;
			created.members.push_back(master);
			created.rollcallDay = master.lastRollcallDay;
			created.todayRollCall = 1;
			store.AddHistory(created, MEASURED_GUILD_HISTORY_MANAGEMENT, MEASURED_GUILD_EVENT_JOIN, created.created, nick, WideString(), 0, 0);
			store.guilds.push_back(created);
			store.Save();
			account.guildTag = tag;
			LOG("[client%x] Guild :: %ls creates %ls [%ls]", clientHd, nick.data(), name.data(), tag.data());
			replication.SendWriter(clientHd, answer);
			replication.SendMoney(clientHd, account.gold, MEASURED_MISSION_REWARD_MONEY_REASON);
			GuildRecord& g = store.guilds.back();
			SendMyGuild(clientHd, &g, 0);
			SendGuildChannelEnter(clientHd, g, nick, 0);
			const u8 state = MEASURED_FRIEND_STATE_ONLINE;
			SendGuildStatus(clientHd, &nick, &state, 1);
		} break;

		case Cl::CQ_DissolveGuild::NET_ID: {
			u32 result = 0;
			if(!guild) result = MEASURED_GUILD_NOT_JOINED;
			else if(me->membershipID != MEASURED_GUILD_MASTER_MEMBERSHIP) result = MEASURED_GUILD_NO_PERMISSION_DISSOLVE;
			else if(guild->members.size() > 1) result = MEASURED_GUILD_KICKOUT_OTHERS_BEFORE_DISSOLVE;
			PacketWriter<Sv::SA_DissolveGuild> answer;
			answer.Write<u32>(result);
			answer.Write<i64>(0);
			replication.SendWriter(clientHd, answer);
			if(result != 0) break;
			LOG("[client%x] Guild :: %ls dissolves %ls", clientHd, nick.data(), guild->name.data());
			const u32 id = guild->id;
			store.guilds.erase(eastl::remove_if(store.guilds.begin(), store.guilds.end(), [&](const GuildRecord& g) { return g.id == id; }), store.guilds.end());
			store.Save();
			account.guildTag.clear();
			SendMyGuild(clientHd, nullptr, 0);
		} break;

		case MEASURED_CQ_CANCEL_GUILD_DISSOLUTION: SendGuildResult(clientHd, Sv::SA_CancelGuilddissolution::NET_ID, MEASURED_GUILD_NOT_DISSOLVING); break;

		case Cl::CQ_QuitGuild::NET_ID: {
			u32 result = 0;
			if(!guild) result = MEASURED_GUILD_NOT_JOINED;
			else if(me->membershipID == MEASURED_GUILD_MASTER_MEMBERSHIP) result = MEASURED_GUILD_INCORRECT_MASTER_DELEGATION;
			SendGuildResult(clientHd, Sv::SA_QuitGuild::NET_ID, result);
			if(result == 0) GuildRemoveMember(*guild, nick, false);
		} break;

		case Cl::CQ_KickGuildMember::NET_ID: {
			WideString target;
			if(!ReadWideString(buff, &target, 32)) break;
			u32 result = 0;
			GuildMemberRecord* targetMember = guild ? store.FindMember(*guild, target) : nullptr;
			if(!guild) result = MEASURED_GUILD_NOT_JOINED;
			else if(!HasGuildRight(*guild, *me, MEASURED_GUILD_RIGHT_EXPEL)) result = MEASURED_GUILD_NO_PERMISSION_KICKOUT;
			else if(!targetMember || targetMember->membershipID == MEASURED_GUILD_MASTER_MEMBERSHIP || target == nick) result = MEASURED_GUILD_NOT_FOUND_INVITEE;
			SendGuildNickResult(clientHd, Sv::SA_KickGuildmember::NET_ID, result, target);
			if(result == 0) GuildRemoveMember(*guild, target, true);
		} break;

		case MEASURED_CQ_JOIN_GUILD: {
			WideString name;
			if(!ReadWideString(buff, &name, 32)) break;
			GuildRecord* target = store.FindByName(name);
			u32 result = 0;
			if(guild) result = MEASURED_GUILD_ALREADY_JOINED;
			else if(!target) result = MEASURED_GUILD_NOT_FOUND;
			else if((i32)target->members.size() >= GuildMemberMax(*target)) result = MEASURED_GUILD_NO_ROOM_TO_JOIN;
			else if(target->joinType == MEASURED_GUILD_JOIN_PRIVATE) result = MEASURED_GUILD_INCORRECT_JOINTYPE;
			else if(target->joinType == MEASURED_GUILD_JOIN_APPROVAL && eastl::find(target->joinRequests.begin(), target->joinRequests.end(), nick) != target->joinRequests.end()) result = MEASURED_GUILD_DUPLICATED_JOINREQUEST;
			PacketWriter<Sv::SA_JoinGuild> answer;
			answer.Write<u32>(result);
			answer.WriteStringObj(name.data());
			answer.Write<u8>(target ? target->joinType : 0);
			replication.SendWriter(clientHd, answer);
			if(result != 0) break;
			if(target->joinType == MEASURED_GUILD_JOIN_APPROVAL) {
				target->joinRequests.push_back(nick);
				store.Save();
				LOG("[client%x] Guild :: %ls asks to join %ls", clientHd, nick.data(), name.data());
			}
			else GuildAddMember(*target, nick);
		} break;

		case Cl::CQ_GetGuildJoinRequestList::NET_ID: {
			PacketWriter<Sv::SA_GetGuildjoinrequestlist,4096> answer;
			answer.Write<i32>(guild ? 0 : MEASURED_GUILD_NOT_JOINED);
			answer.Write<u16>(guild ? (u16)guild->joinRequests.size() : 0);
			if(guild) foreach_const(r, guild->joinRequests) {
				answer.WriteStringObj(r->data());
				answer.Write<u16>(AccountProgressLoad(r->data()).level);
				answer.Write<u16>(GuildLeaderClass(*r));
				answer.Write<u16>((u16)content.masters.size());
				answer.Write<u32>(0);
				answer.Write<u8>(0);
				answer.Write<u16>(0);
			}
			replication.SendWriter(clientHd, answer);
		} break;

		case Cl::CQ_RespondGuildJoinRequest::NET_ID: {
			WideString target;
			if(!ReadWideString(buff, &target, 32) || !buff.CanRead(1)) break;
			const bool accept = buff.Read<u8>() != 0;
			u32 result = 0;
			if(!guild) result = MEASURED_GUILD_NOT_JOINED;
			else if(!HasGuildRight(*guild, *me, MEASURED_GUILD_RIGHT_INVITE)) result = MEASURED_GUILD_NO_PERMISSION_APPROVEJOINREQUEST;
			else if(eastl::find(guild->joinRequests.begin(), guild->joinRequests.end(), target) == guild->joinRequests.end()) result = MEASURED_GUILD_NOT_FOUND_JOINREQUEST;
			else if(accept && store.FindByMember(target)) result = MEASURED_GUILD_ALREADY_JOINED;
			else if(accept && (i32)guild->members.size() >= GuildMemberMax(*guild)) result = MEASURED_GUILD_NO_ROOM_TO_JOIN;
			PacketWriter<Sv::SA_RespondGuildjoinrequest,512> answer;
			answer.Write<u32>(result);
			answer.Write<u8>(accept ? 1 : 0);
			GuildMemberRecord candidate;
			candidate.nickname = target;
			candidate.membershipID = MEASURED_GUILD_REGULAR_MEMBERSHIP;
			WriteGuildMemberProfile(answer, candidate, GuildLeaderClass(target));
			replication.SendWriter(clientHd, answer);
			if(result != 0) break;
			guild->joinRequests.erase(eastl::remove(guild->joinRequests.begin(), guild->joinRequests.end(), target), guild->joinRequests.end());
			if(accept) GuildAddMember(*guild, target);
			else store.Save();
		} break;

		case Cl::CQ_InviteGuildMember::NET_ID: {
			WideString target;
			if(!ReadWideString(buff, &target, 32)) break;
			u32 result = 0;
			const i32 targetID = FindUserByNickname(target);
			if(!guild) result = MEASURED_GUILD_NOT_JOINED;
			else if(!HasGuildRight(*guild, *me, MEASURED_GUILD_RIGHT_INVITE)) result = MEASURED_GUILD_NO_PERMISSION_INVITE;
			else if(targetID < 0 || target == nick) result = MEASURED_GUILD_NOT_FOUND_INVITEE;
			else if(store.FindByMember(target)) result = MEASURED_GUILD_ALREADY_JOINED;
			else if(eastl::find_if(guild->invitations.begin(), guild->invitations.end(), [&](const GuildInvitation& i) { return i.invitee == target; }) != guild->invitations.end()) result = MEASURED_GUILD_DUPLICATED_INVITATION;
			else if((i32)guild->members.size() >= GuildMemberMax(*guild)) result = MEASURED_GUILD_NO_ROOM_TO_JOIN;
			SendGuildNickResult(clientHd, Sv::SA_InviteGuildmember::NET_ID, result, target);
			if(result != 0) break;
			GuildInvitation inv;
			inv.invitee = target;
			inv.inviter = nick;
			guild->invitations.push_back(inv);
			store.Save();
			PacketWriter<Sv::SN_GuildInvitation> note;
			note.WriteStringObj(guild->name.data());
			note.WriteStringObj(nick.data());
			if(playerMap[targetID] != playerList.end()) replication.SendWriter(playerMap[targetID]->clientHd, note);
			LOG("[client%x] Guild :: %ls invites %ls to %ls", clientHd, nick.data(), target.data(), guild->name.data());
		} break;

		case Cl::CQ_GetGuildInvitationList::NET_ID: {
			PacketWriter<Sv::SA_GetGuildinvitationlist,2048> answer;
			answer.Write<u32>(0);
			u16 count = 0;
			foreach_const(g, store.guilds) foreach_const(i, g->invitations) if(i->invitee == nick) count++;
			answer.Write<u16>(count);
			foreach_const(g, store.guilds) foreach_const(i, g->invitations) {
				if(i->invitee != nick) continue;
				answer.WriteStringObj(g->name.data());
				answer.WriteStringObj(i->inviter.data());
			}
			replication.SendWriter(clientHd, answer);
		} break;

		case Cl::CQ_RespondGuildInvitation::NET_ID: {
			WideString name;
			if(!ReadWideString(buff, &name, 32) || !buff.CanRead(1)) break;
			const bool accept = buff.Read<u8>() != 0;
			GuildRecord* target = store.FindByName(name);
			u32 result = 0;
			if(!target || eastl::find_if(target->invitations.begin(), target->invitations.end(), [&](const GuildInvitation& i) { return i.invitee == nick; }) == target->invitations.end()) result = MEASURED_GUILD_NOT_FOUND_INVITATION;
			else if(accept && guild) result = MEASURED_GUILD_ALREADY_JOINED;
			else if(accept && (i32)target->members.size() >= GuildMemberMax(*target)) result = MEASURED_GUILD_NO_ROOM_TO_JOIN;
			PacketWriter<Sv::SA_RespondGuildinvitation> answer;
			answer.Write<u32>(result);
			answer.Write<u8>(accept ? 1 : 0);
			answer.WriteStringObj(name.data());
			answer.WriteStringObj(target ? target->tag.data() : L"");
			replication.SendWriter(clientHd, answer);
			if(result != 0) break;
			target->invitations.erase(eastl::remove_if(target->invitations.begin(), target->invitations.end(), [&](const GuildInvitation& i) { return i.invitee == nick; }), target->invitations.end());
			if(accept) GuildAddMember(*target, nick);
			else store.Save();
		} break;

		case Cl::CQ_GetGuildPublicProfile::NET_ID: {
			WideString name;
			if(!ReadWideString(buff, &name, 32)) break;
			SendGuildPublicProfile(clientHd, store.FindByName(name), Sv::SA_GetGuildpublicprofile::NET_ID, nullptr);
		} break;

		case MEASURED_CQ_GET_GUILD_PUBLIC_PROFILE_FROM_USER: {
			WideString target;
			if(!ReadWideString(buff, &target, 32)) break;
			SendGuildPublicProfile(clientHd, store.FindByMember(target), MEASURED_SA_GET_GUILD_PUBLIC_PROFILE_FROM_USER, &target);
		} break;

		case Cl::CQ_EditGuildNotice::NET_ID:
		case Cl::CQ_EditGuildIntro::NET_ID: {
			const bool notice = netID == Cl::CQ_EditGuildNotice::NET_ID;
			GuildText text;
			if(!ReadWideString(buff, &text, 200)) break;
			u32 result = 0;
			if(!guild) result = MEASURED_GUILD_NOT_JOINED;
			else if(!HasGuildRight(*guild, *me, notice ? MEASURED_GUILD_RIGHT_NOTICE : MEASURED_GUILD_RIGHT_INTRO)) result = notice ? MEASURED_GUILD_NO_PERMISSION_EDITNOTICE : MEASURED_GUILD_NO_PERMISSION_EDITINTRO;
			PacketWriter<Sv::SA_EditGuildnotice,1024> answer;
			answer.Write<u32>(result);
			answer.WriteStringObj(text.data(), (i32)text.size());
			replication.server->SendPacketData(clientHd, notice ? Sv::SA_EditGuildnotice::NET_ID : Sv::SA_EditGuildintro::NET_ID, (u16)answer.size, answer.data);
			if(result != 0) break;
			if(notice) guild->notice = text;
			else guild->intro = text;
			store.Save();
			if(notice) {
				PacketWriter<Sv::SN_GuildNotice,1024> note;
				note.WriteStringObj(text.data(), (i32)text.size());
				ForEachOnlineGuildMember(*guild, [&](i32, ClientHandle hd) { if(hd != clientHd) replication.SendWriter(hd, note); });
			}
		} break;

		case Cl::CQ_SetGuildInterest::NET_ID: {
			if(!buff.CanRead(7)) break;
			u32 result = 0;
			if(!guild) result = MEASURED_GUILD_NOT_JOINED;
			else if(!HasGuildRight(*guild, *me, MEASURED_GUILD_RIGHT_INTEREST)) result = MEASURED_GUILD_NO_PERMISSION_SETINTEREST;
			if(result == 0) {
				for(i32 i = 0; i < 7; i++) guild->interest[i] = buff.Read<u8>() ? 1 : 0;
				store.Save();
			}
			SendGuildResult(clientHd, Sv::SA_SetGuildinterest::NET_ID, result);
		} break;

		case Cl::CQ_SetGuildJoinType::NET_ID: {
			if(!buff.CanRead(1)) break;
			const u8 joinType = buff.Read<u8>();
			u32 result = 0;
			if(!guild) result = MEASURED_GUILD_NOT_JOINED;
			else if(!HasGuildRight(*guild, *me, MEASURED_GUILD_RIGHT_JOINTYPE)) result = MEASURED_GUILD_NO_PERMISSION_CHGJOINTYPE;
			else if(joinType < MEASURED_GUILD_JOIN_PUBLIC || joinType > MEASURED_GUILD_JOIN_PRIVATE) result = MEASURED_GUILD_INCORRECT_JOINTYPE;
			if(result == 0) {
				guild->joinType = joinType;
				store.Save();
			}
			PacketWriter<Sv::SA_SetGuildjointype> answer;
			answer.Write<u32>(result);
			answer.Write<u8>(joinType);
			replication.SendWriter(clientHd, answer);
		} break;

		case MEASURED_CQ_DONATE_TO_GUILD: {
			if(!buff.CanRead(4)) break;
			const u32 gold = buff.Read<u32>();
			u32 result = 0;
			if(!guild) result = MEASURED_GUILD_NOT_JOINED;
			else if(gold == 0 || gold % (u32)content.guildDonationUnit != 0) result = MEASURED_GUILD_PARAMETER_ERROR;
			else if(account.gold < (i64)gold) result = MEASURED_TRADE_NOT_ENOUGH_GOLD;
			PacketWriter<Sv::SA_DonateToGuild> answer;
			answer.Write<u32>(result);
			const u32 gain = gold / (u32)content.guildFundExchangeRate;
			answer.Write<u32>(gold);
			answer.Write<u32>(result == 0 ? gain : 0);
			if(result != 0) {
				answer.Write<u32>(guild ? (u32)guild->fund : 0);
				replication.server->SendPacketData(clientHd, MEASURED_SA_DONATE_TO_GUILD, (u16)answer.size, answer.data);
				break;
			}
			AccountProgress progress = AccountProgressLoad(nick.data());
			progress.gold -= gold;
			AccountProgressSave(nick.data(), progress);
			account.gold = progress.gold;
			guild->fund += gain;
			me->contributedFund += gain;
			store.AddHistory(*guild, MEASURED_GUILD_HISTORY_ASSET, MEASURED_GUILD_EVENT_DONATION, NowFiletime(), nick, WideString(), (i32)gold, (i32)gain);
			store.Save();
			answer.Write<u32>((u32)guild->fund);
			replication.server->SendPacketData(clientHd, MEASURED_SA_DONATE_TO_GUILD, (u16)answer.size, answer.data);
			replication.SendMoney(clientHd, account.gold, MEASURED_MISSION_REWARD_MONEY_REASON);
			PacketWriter<Sv::SN_GuildfundAdded> note;
			note.WriteStringObj(nick.data());
			note.Write<u32>(gain);
			note.Write<u32>((u32)guild->fund);
			ForEachOnlineGuildMember(*guild, [&](i32, ClientHandle hd) { replication.SendWriter(hd, note); });
			LOG("[client%x] Guild :: %ls donates %u gold to %ls (+%u fund)", clientHd, nick.data(), gold, guild->name.data(), gain);
		} break;

		case Cl::CQ_UNKNOWN_60172::NET_ID: {
			static const i32 HYP_SEARCH_TAIL = 5;
			static const u8 HYP_SEARCH_ROWS = 8;
			const eastl::fixed_string<wchar,256> query = buff.ReadWideStringObj();
			u32 pageNo = 1;
			u8 rows = HYP_SEARCH_ROWS;
			if(size >= HYP_SEARCH_TAIL) {
				ConstBuffer tail(data + size - HYP_SEARCH_TAIL, HYP_SEARCH_TAIL);
				const u32 p = tail.Read<u32>();
				const u8 r = tail.Read<u8>();
				if(p >= 1 && p < 1000) pageNo = p;
				if(r >= 1 && r <= 50) rows = r;
			}
			auto matches = [&](const GuildRecord& g) {
				if(query.empty()) return true;
				auto contains = [&](const auto& text) {
					eastl::fixed_string<wchar,256,true> a(text.data()), b(query.data());
					a.make_lower();
					b.make_lower();
					return a.find(b) != a.npos;
				};
				return contains(g.name) || contains(g.tag);
			};
			eastl::fixed_vector<const GuildRecord*,256,true> found;
			foreach_const(g, store.guilds) if(g->joinType != MEASURED_GUILD_JOIN_PRIVATE && matches(*g)) found.push_back(&*g);
			const u32 maxPage = MAX(((u32)found.size() + rows - 1) / rows, 1u);
			const u32 first = (pageNo - 1) * rows;
			PacketWriter<Sv::SA_SearchGuild,4096> answer;
			answer.Write<u32>(0);
			answer.Write<u32>(pageNo);
			answer.Write<u8>(rows);
			answer.Write<u32>(maxPage);
			const u32 count = first < found.size() ? MIN((u32)found.size() - first, (u32)rows) : 0;
			answer.Write<u16>((u16)count);
			for(u32 i = first; i < first + count; i++) {
				const GuildRecord& g = *found[i];
				answer.WriteStringObj(g.name.data());
				answer.WriteStringObj(g.tag.data());
				answer.Write<u32>((u32)g.emblemIndex);
				answer.Write<u8>((u8)GuildLevel(g));
				answer.Write<u8>(g.joinType);
				answer.Write<u8>((u8)g.members.size());
				answer.Write<u8>((u8)GuildMemberMax(g));
				for(u8 v : g.interest) answer.Write<u8>(v);
			}
			replication.SendWriter(clientHd, answer);
			LOG("[client%x] Guild :: search '%ls' page %u -> %u result(s)", clientHd, query.data(), pageNo, count);
		} break;

		case Cl::CQ_GetRecommendedGuildList::NET_ID: {
			const u8 wanted = buff.CanRead(1) ? buff.Read<u8>() : 8;
			PacketWriter<Sv::SA_GetRecommendedguildlist,4096> answer;
			answer.Write<i32>(0);
			u16 count = 0;
			foreach_const(g, store.guilds) if(count < wanted && (!guild || g->id != guild->id) && g->joinType != MEASURED_GUILD_JOIN_PRIVATE) count++;
			answer.Write<u16>(count);
			u16 written = 0;
			foreach_const(g, store.guilds) {
				if(written >= count || (guild && g->id == guild->id) || g->joinType == MEASURED_GUILD_JOIN_PRIVATE) continue;
				answer.WriteStringObj(g->name.data());
				answer.WriteStringObj(g->tag.data());
				answer.Write<u32>((u32)g->emblemIndex);
				answer.Write<u8>((u8)GuildLevel(*g));
				answer.Write<u8>(g->joinType);
				answer.Write<u8>((u8)g->members.size());
				answer.Write<u8>((u8)GuildMemberMax(*g));
				for(u8 v : g->interest) answer.Write<u8>(v);
				written++;
			}
			replication.SendWriter(clientHd, answer);
		} break;

		case MEASURED_CQ_GET_GUILD_RANKING_LIST: {
			if(!buff.CanRead(8)) break;
			const u8 rankingType = buff.Read<u8>();
			const u16 seasonNo = buff.Read<u16>();
			const u32 pageNo = MAX(buff.Read<u32>(), 1u);
			const u8 rows = MAX(buff.Read<u8>(), (u8)1);
			eastl::fixed_vector<const GuildRecord*,256,true> ranked;
			if(rankingType == MEASURED_GUILD_RANKING_POINT) foreach_const(g, store.guilds) ranked.push_back(&*g);
			eastl::sort(ranked.begin(), ranked.end(), [](const GuildRecord* a, const GuildRecord* b) { return a->point > b->point; });
			const u32 maxPage = ((u32)ranked.size() + rows - 1) / rows;
			const u32 first = (pageNo - 1) * rows;
			PacketWriter<Sv::SA_GetGuildranklist,4096> answer;
			answer.Write<i32>(0);
			answer.Write<u8>(rankingType);
			answer.Write<u16>(seasonNo);
			answer.Write<u32>(pageNo);
			answer.Write<u8>(rows);
			answer.Write<u32>(maxPage);
			answer.Write<i64>(NowFiletime());
			const u32 last = MIN(first + rows, (u32)ranked.size());
			answer.Write<u16>((u16)(last > first ? last - first : 0));
			for(u32 r = first; r < last; r++) {
				const GuildRecord* g = ranked[r];
				const GuildMemberRecord* owner = nullptr;
				foreach_const(m, g->members) if(m->membershipID == MEASURED_GUILD_MASTER_MEMBERSHIP) owner = &*m;
				answer.Write<u32>(r + 1);
				answer.Write<u32>(r + 1);
				answer.WriteStringObj(g->tag.data());
				answer.WriteStringObj(g->name.data());
				answer.WriteStringObj(owner ? owner->nickname.data() : L"");
				answer.Write<u32>((u32)g->point);
			}
			replication.SendWriter(clientHd, answer);
		} break;

		case MEASURED_CQ_GET_GUILD_RANKING: {
			if(!buff.CanRead(3)) break;
			const u8 rankingType = buff.Read<u8>();
			const u16 seasonNo = buff.Read<u16>();
			WideString name;
			if(!ReadWideString(buff, &name, 32)) break;
			const GuildRecord* target = store.FindByName(name);
			u32 rank = MEASURED_GUILD_UNRANKED;
			if(target && rankingType == MEASURED_GUILD_RANKING_POINT) {
				rank = 1;
				foreach_const(g, store.guilds) if(g->point > target->point) rank++;
			}
			const GuildMemberRecord* owner = nullptr;
			if(target) foreach_const(m, target->members) if(m->membershipID == MEASURED_GUILD_MASTER_MEMBERSHIP) owner = &*m;
			const bool ranked = rank != MEASURED_GUILD_UNRANKED;
			PacketWriter<Sv::SA_GetGuildrank,512> answer;
			answer.Write<i32>(0);
			answer.Write<u8>(rankingType);
			answer.Write<u16>(seasonNo);
			answer.Write<u32>(rank);
			answer.Write<u32>(rank);
			answer.WriteStringObj(ranked ? target->tag.data() : L"");
			answer.WriteStringObj(ranked ? target->name.data() : L"");
			answer.WriteStringObj(ranked && owner ? owner->nickname.data() : L"");
			answer.Write<u32>(ranked ? (u32)target->point : 0);
			replication.SendWriter(clientHd, answer);
		} break;

		case MEASURED_CQ_GET_GUILD_RANK_REWARD_INFO: {
			const u8 rankingType = buff.CanRead(1) ? buff.Read<u8>() : 0;
			PacketWriter<Sv::SA_GetGuildrankrewardinfo> answer;
			answer.Write<i32>(0);
			answer.Write<u8>(rankingType);
			answer.Write<u16>(0);
			replication.SendWriter(clientHd, answer);
		} break;

		case MEASURED_CQ_GET_PLAYING_GUILD_PVP_LIST: {
			const u16 pageNo = buff.CanRead(2) ? buff.Read<u16>() : 1;
			const u8 rows = buff.CanRead(1) ? buff.Read<u8>() : 0;
			PacketWriter<Sv::SA_GetPlayingGuildpvplist> answer;
			answer.Write<i32>(0);
			answer.Write<u16>(pageNo);
			answer.Write<u8>(rows);
			answer.Write<u16>(0);
			answer.Write<u16>(0);
			replication.SendWriter(clientHd, answer);
		} break;

		case MEASURED_CQ_GET_GUILD_SKILL_LIST: {
			PacketWriter<Sv::SA_GetGuildskilllist,1024> answer;
			answer.Write<u32>(guild ? 0 : MEASURED_GUILD_NOT_JOINED);
			answer.Write<u16>(guild ? (u16)guild->skills.size() : 0);
			if(guild) foreach_const(s, guild->skills) {
				answer.Write<u8>(s->type);
				answer.Write<u8>(s->level);
				answer.Write<i64>(s->expiry);
				answer.Write<u16>(s->extensionCount);
			}
			replication.SendWriter(clientHd, answer);
		} break;

		case MEASURED_CQ_BUY_GUILD_SKILL: {
			if(!buff.CanRead(3)) break;
			const u8 type = buff.Read<u8>();
			buff.Read<u8>();
			const u8 level = buff.Read<u8>();
			const GameXmlContent::GuildSkillInfo* info = content.FindGuildSkill(type);
			const GameXmlContent::GuildSkillLevel* lvl = info ? FindGuildSkillLevel(*info, level) : nullptr;
			const i32 current = guild ? GuildSkillLevel(*guild, type) : 0;
			const bool limited = info && info->limitedHours > 0;
			u32 result = 0;
			if(!guild) result = MEASURED_GUILD_NOT_JOINED;
			else if(!HasGuildRight(*guild, *me, MEASURED_GUILD_RIGHT_FUND)) result = MEASURED_GUILD_NO_PERMISSION_BUYGUILDSKILL;
			else if(!lvl) result = MEASURED_GUILD_NOT_FOUND_SKILL;
			else if(limited ? current > 0 : level <= current) result = MEASURED_GUILD_DUPLICATED_SKILL;
			else if((!limited && level != current + 1) || GuildLevel(*guild) < lvl->unlockGuildLevel) result = MEASURED_GUILD_NOT_SATISFIED_REQUIREMENTS_FOR_SKILL;
			else if(guild->fund < lvl->cost) result = MEASURED_GUILD_NOT_ENOUGH_FUND;
			PacketWriter<Sv::SA_BuyGuildskill> answer;
			answer.Write<u32>(result);
			answer.Write<u8>(type);
			answer.Write<u8>(level);
			if(result != 0) {
				answer.Write<i64>(0);
				answer.Write<u16>(0);
				answer.Write<u32>(0);
				answer.Write<u32>(guild ? (u32)guild->fund : 0);
				replication.SendWriter(clientHd, answer);
				break;
			}
			GuildSkillRecord* record = FindGuildSkillRecord(*guild, type);
			if(!record) {
				guild->skills.push_back(GuildSkillRecord());
				record = &guild->skills.back();
				record->type = type;
			}
			record->level = level;
			record->expiry = limited ? NowFiletime() + (i64)info->limitedHours * FILETIME_PER_HOUR : 0;
			record->extensionCount = 0;
			guild->fund -= lvl->cost;
			const bool acquired = current == 0;
			store.AddHistory(*guild, MEASURED_GUILD_HISTORY_LEVEL, acquired ? MEASURED_GUILD_EVENT_SKILLACQUISITION : MEASURED_GUILD_EVENT_SKILLUPGRADE, NowFiletime(), nick, WideString(), type, level);
			store.Save();
			answer.Write<i64>(record->expiry);
			answer.Write<u16>((u16)info->limitedHours);
			answer.Write<u32>((u32)lvl->cost);
			answer.Write<u32>((u32)guild->fund);
			replication.SendWriter(clientHd, answer);
			PacketWriter<Sv::SN_GuildskillAcquired,256> note;
			note.WriteStringObj(nick.data());
			note.Write<u8>(type);
			note.Write<u8>(level);
			note.Write<u16>((u16)info->limitedHours);
			note.Write<i64>(record->expiry);
			note.Write<u32>((u32)lvl->cost);
			note.Write<u32>((u32)guild->fund);
			const u16 noteID = acquired ? (u16)Sv::SN_GuildskillAcquired::NET_ID : (u16)Sv::SN_GuildskillUpgraded::NET_ID;
			ForEachOnlineGuildMember(*guild, [&](i32, ClientHandle hd) { replication.server->SendPacketData(hd, noteID, (u16)note.size, note.data); });
			SendGuildPointAndFund(*guild);
			SendGuildConstraint(*guild);
			LOG("[client%x] Guild :: %ls buys skill %u level %u for %ls (%d fund left)", clientHd, nick.data(), (u32)type, (u32)level, guild->name.data(), guild->fund);
		} break;

		case MEASURED_CQ_EXTEND_GUILD_LIMITED_SKILL: {
			if(!buff.CanRead(4)) break;
			const u8 type = buff.Read<u8>();
			const u16 extension = buff.Read<u16>();
			buff.Read<u8>();
			const GameXmlContent::GuildSkillInfo* info = content.FindGuildSkill(type);
			const i32 current = guild ? GuildSkillLevel(*guild, type) : 0;
			const GameXmlContent::GuildSkillLevel* lvl = info ? FindGuildSkillLevel(*info, current) : nullptr;
			const i32 cost = lvl ? lvl->cost * (i32)extension : 0;
			u32 result = 0;
			if(!guild) result = MEASURED_GUILD_NOT_JOINED;
			else if(!HasGuildRight(*guild, *me, MEASURED_GUILD_RIGHT_FUND)) result = MEASURED_GUILD_NO_PERMISSION_EXTENDGUILDSKILL;
			else if(!info || info->limitedHours <= 0 || !lvl || extension == 0) result = MEASURED_GUILD_NOT_FOUND_SKILL;
			else if(guild->fund < cost) result = MEASURED_GUILD_NOT_ENOUGH_FUND;
			GuildSkillRecord* record = result == 0 ? FindGuildSkillRecord(*guild, type) : nullptr;
			if(record) {
				record->expiry += (i64)info->limitedHours * extension * FILETIME_PER_HOUR;
				record->extensionCount += extension;
				guild->fund -= cost;
				store.AddHistory(*guild, MEASURED_GUILD_HISTORY_LEVEL, MEASURED_GUILD_EVENT_SKILLEXTENSION, NowFiletime(), nick, WideString(), type, current);
				store.Save();
			}
			PacketWriter<Sv::SA_ExtendGuildlimitedskill> answer;
			answer.Write<u32>(record ? 0 : (result != 0 ? result : MEASURED_GUILD_NOT_FOUND_SKILL));
			answer.Write<u8>(type);
			answer.Write<u8>((u8)current);
			answer.Write<u16>(extension);
			answer.Write<i64>(record ? record->expiry : 0);
			answer.Write<u32>(record ? (u32)cost : 0);
			answer.Write<u32>(guild ? (u32)guild->fund : 0);
			replication.SendWriter(clientHd, answer);
			if(!record) break;
			PacketWriter<Sv::SN_GuildskillExtended,256> note;
			note.WriteStringObj(nick.data());
			note.Write<u8>(type);
			note.Write<u8>((u8)current);
			note.Write<u16>(record->extensionCount);
			note.Write<u16>((u16)(info->limitedHours * extension));
			note.Write<i64>(record->expiry);
			note.Write<u32>((u32)cost);
			note.Write<u32>((u32)guild->fund);
			ForEachOnlineGuildMember(*guild, [&](i32, ClientHandle hd) { replication.SendWriter(hd, note); });
			SendGuildPointAndFund(*guild);
		} break;

		case MEASURED_CQ_EDIT_GUILD_TAG: {
			WideString tag;
			if(!ReadWideString(buff, &tag, 8)) break;
			u32 result = 0;
			if(!guild) result = MEASURED_GUILD_NOT_JOINED;
			else if(me->membershipID != MEASURED_GUILD_MASTER_MEMBERSHIP) result = MEASURED_GUILD_NO_PERMISSION_EDITTAG;
			else if(!ValidGuildText(tag, 2, 4)) result = MEASURED_GUILD_ILLEGAL_TAG;
			else if(guild->fund < content.guildTagChangeCost) result = MEASURED_GUILD_NOT_ENOUGH_FUND;
			PacketWriter<Sv::SA_EditGuildtag,128> answer;
			answer.Write<u32>(result);
			answer.WriteStringObj(tag.data());
			answer.Write<u32>(result == 0 ? (u32)content.guildTagChangeCost : 0);
			if(result != 0) {
				answer.Write<u32>(guild ? (u32)guild->fund : 0);
				replication.SendWriter(clientHd, answer);
				break;
			}
			store.AddHistory(*guild, MEASURED_GUILD_HISTORY_MANAGEMENT, MEASURED_GUILD_EVENT_TAGCHANGE, NowFiletime(), guild->tag, tag, 0, 0);
			guild->fund -= content.guildTagChangeCost;
			guild->tag = tag;
			store.Save();
			answer.Write<u32>((u32)guild->fund);
			replication.SendWriter(clientHd, answer);
			PacketWriter<Sv::SN_GuildtagChanged,128> note;
			note.WriteStringObj(tag.data());
			note.Write<u32>((u32)content.guildTagChangeCost);
			note.Write<u32>((u32)guild->fund);
			ForEachOnlineGuildMember(*guild, [&](i32 memberID, ClientHandle hd) {
				replication.SendWriter(hd, note);
				auto found = GetAccountManager().accountMap.find(playerAccountData[memberID]->UID);
				if(found != GetAccountManager().accountMap.end()) found->second->guildTag = tag;
			});
			LOG("[client%x] Guild :: %ls renames the tag of %ls to %ls", clientHd, nick.data(), guild->name.data(), tag.data());
		} break;

		case MEASURED_CQ_SET_GUILD_EMBLEM: {
			if(!buff.CanRead(4)) break;
			const i32 emblem = buff.Read<i32>();
			i32 index = -1;
			for(i32 i = 0; i < (i32)content.validGuildEmblems.size(); i++) if(content.validGuildEmblems[i] == emblem) index = i;
			u32 result = 0;
			if(!guild) result = MEASURED_GUILD_NOT_JOINED;
			else if(!HasGuildRight(*guild, *me, MEASURED_GUILD_RIGHT_EMBLEM)) result = MEASURED_GUILD_NO_PERMISSION_CHGEMBLEM;
			else if(index < 0) result = MEASURED_GUILD_PARAMETER_ERROR;
			else if(index >= HYP_GUILD_BASE_EMBLEMS + GuildSkillBonus(*guild, MEASURED_GUILD_SKILL_EMBLEM)) result = MEASURED_GUILD_LOCKED_GUILDEMBLEM_LEVEL;
			if(result == 0) {
				guild->emblemIndex = emblem;
				store.Save();
			}
			PacketWriter<Sv::SA_SetGuildemblem> answer;
			answer.Write<u32>(result);
			answer.Write<i32>(emblem);
			replication.SendWriter(clientHd, answer);
			if(result != 0) break;
			PacketWriter<Sv::SN_GuildemblemChange> note;
			note.Write<i32>(emblem);
			ForEachOnlineGuildMember(*guild, [&](i32, ClientHandle hd) { if(hd != clientHd) replication.SendWriter(hd, note); });
		} break;

		case MEASURED_CQ_CREATE_GUILD_MEMBERSHIP:
		case MEASURED_CQ_EDIT_GUILD_MEMBERSHIP: {
			const bool create = netID == MEASURED_CQ_CREATE_GUILD_MEMBERSHIP;
			i32 id = 0;
			if(!create) {
				if(!buff.CanRead(4)) break;
				id = buff.Read<i32>();
			}
			if(!buff.CanRead(1)) break;
			const u8 icon = buff.Read<u8>();
			WideString name;
			eastl::array<u8,10> rights = {};
			if(!ReadWideString(buff, &name, 32) || !ReadGuildRights(buff, &rights)) break;
			GuildClass* target = nullptr;
			if(guild && !create) foreach(c, guild->classes) if(c->id == id) target = &*c;
			bool duplicated = false;
			if(guild) foreach_const(c, guild->classes) if(c->name == name && (create || c->id != id)) duplicated = true;
			u32 result = 0;
			if(!guild) result = MEASURED_GUILD_NOT_JOINED;
			else if(!HasGuildRight(*guild, *me, MEASURED_GUILD_RIGHT_MEMBERSHIP)) result = MEASURED_GUILD_NO_PERMISSION_EDITMEMBERSHIP;
			else if(!create && !target) result = MEASURED_GUILD_NOT_FOUND_GUILDMEMBERSHIP;
			else if(create && (i32)guild->classes.size() >= content.guildMembershipCapacity) result = MEASURED_GUILD_TOO_MANY_GUILDMEMBERSHIP;
			else if(name.empty()) result = MEASURED_GUILD_ILLEGAL_CLASSNAME;
			else if(duplicated) result = MEASURED_GUILD_DUPLICATED_GUILDMEMBERSHIPNAME;
			if(result == 0 && create) {
				GuildClass c;
				foreach_const(o, guild->classes) id = MAX(id, o->id);
				c.id = id + 1;
				c.type = MEASURED_GUILD_CLASS_CUSTOM;
				guild->classes.push_back(c);
				target = &guild->classes.back();
				id = c.id;
			}
			if(result == 0) {
				target->iconIndex = icon;
				target->name = name;
				target->rights = rights;
				store.Save();
			}
			PacketWriter<Sv::SA_CreateGuildmembership,256> answer;
			answer.Write<u32>(result);
			answer.Write<i32>(id);
			answer.Write<u8>(icon);
			answer.WriteStringObj(name.data());
			WriteGuildRights(answer, rights);
			replication.server->SendPacketData(clientHd, create ? (u16)Sv::SA_CreateGuildmembership::NET_ID : (u16)Sv::SA_EditGuildmembership::NET_ID, (u16)answer.size, answer.data);
			if(result != 0) break;
			PacketWriter<Sv::SN_GuildmembershipAdded,256> note;
			note.Write<i32>(id);
			note.Write<u8>(icon);
			note.WriteStringObj(name.data());
			WriteGuildRights(note, rights);
			const u16 noteID = create ? (u16)Sv::SN_GuildmembershipAdded::NET_ID : (u16)Sv::SN_GuildmembershipModified::NET_ID;
			ForEachOnlineGuildMember(*guild, [&](i32, ClientHandle hd) { if(hd != clientHd) replication.server->SendPacketData(hd, noteID, (u16)note.size, note.data); });
		} break;

		case MEASURED_CQ_DELETE_GUILD_MEMBERSHIP: {
			if(!buff.CanRead(4)) break;
			const i32 id = buff.Read<i32>();
			const GuildClass* target = guild ? FindGuildClass(*guild, id) : nullptr;
			const GuildClass* regular = guild ? FindGuildClass(*guild, MEASURED_GUILD_REGULAR_MEMBERSHIP) : nullptr;
			u32 result = 0;
			if(!guild) result = MEASURED_GUILD_NOT_JOINED;
			else if(!HasGuildRight(*guild, *me, MEASURED_GUILD_RIGHT_MEMBERSHIP)) result = MEASURED_GUILD_NO_PERMISSION_EDITMEMBERSHIP;
			else if(!target || !regular) result = MEASURED_GUILD_NOT_FOUND_GUILDMEMBERSHIP;
			else if(target->type != MEASURED_GUILD_CLASS_CUSTOM) result = MEASURED_GUILD_NOT_DELETABLE_GUILDMEMBERSHIP;
			eastl::fixed_vector<WideString,64,true> moved;
			if(result == 0) {
				foreach(m, guild->members) {
					if(m->membershipID != id) continue;
					m->membershipID = MEASURED_GUILD_REGULAR_MEMBERSHIP;
					moved.push_back(m->nickname);
				}
			}
			PacketWriter<Sv::SA_DeleteGuildmembership,2048> answer;
			answer.Write<u32>(result);
			answer.Write<i32>(id);
			answer.Write<i32>(MEASURED_GUILD_REGULAR_MEMBERSHIP);
			answer.WriteStringObj(regular ? regular->name.data() : L"");
			answer.Write<u16>((u16)moved.size());
			foreach_const(n, moved) answer.WriteStringObj(n->data());
			const WideString regularName = regular ? regular->name : WideString();
			if(result == 0) {
				guild->classes.erase(eastl::remove_if(guild->classes.begin(), guild->classes.end(), [&](const GuildClass& c) { return c.id == id; }), guild->classes.end());
				store.Save();
			}
			replication.SendWriter(clientHd, answer);
			if(result != 0) break;
			PacketWriter<Sv::SN_GuildmembershipRemoved> removed;
			removed.Write<i32>(id);
			ForEachOnlineGuildMember(*guild, [&](i32, ClientHandle hd) {
				if(hd != clientHd) replication.SendWriter(hd, removed);
				foreach_const(n, moved) {
					PacketWriter<Sv::SN_GuildmemberChgclass,256> chg;
					chg.Write<i32>(id);
					chg.Write<i32>(MEASURED_GUILD_REGULAR_MEMBERSHIP);
					chg.WriteStringObj(regularName.data());
					chg.WriteStringObj(n->data());
					replication.SendWriter(hd, chg);
				}
			});
		} break;

		case MEASURED_CQ_SET_GUILD_MEMBER_CLASS: {
			WideString target;
			if(!ReadWideString(buff, &target, 32) || !buff.CanRead(8)) break;
			const i32 oldID = buff.Read<i32>();
			const i32 newID = buff.Read<i32>();
			GuildMemberRecord* member = guild ? store.FindMember(*guild, target) : nullptr;
			const GuildClass* cls = guild ? FindGuildClass(*guild, newID) : nullptr;
			u32 result = 0;
			if(!guild) result = MEASURED_GUILD_NOT_JOINED;
			else if(!HasGuildRight(*guild, *me, MEASURED_GUILD_RIGHT_ASSIGN)) result = MEASURED_GUILD_NO_PERMISSION_ASSIGNCLASS;
			else if(!member || member->membershipID == MEASURED_GUILD_MASTER_MEMBERSHIP) result = MEASURED_GUILD_NOT_FOUND_INVITEE;
			else if(!cls) result = MEASURED_GUILD_NOT_FOUND_GUILDMEMBERSHIP;
			const i32 previous = member ? member->membershipID : oldID;
			if(result == 0) {
				member->membershipID = newID;
				store.AddHistory(*guild, MEASURED_GUILD_HISTORY_MANAGEMENT, MEASURED_GUILD_EVENT_ASSIGN, NowFiletime(), nick, target, previous, newID);
				store.Save();
			}
			PacketWriter<Sv::SA_SetGuildmemberclass,256> answer;
			answer.Write<u32>(result);
			answer.WriteStringObj(target.data());
			answer.Write<i32>(previous);
			answer.Write<i32>(newID);
			answer.WriteStringObj(cls ? cls->name.data() : L"");
			replication.SendWriter(clientHd, answer);
			if(result != 0) break;
			PacketWriter<Sv::SN_GuildmemberChgclass,256> note;
			note.Write<i32>(previous);
			note.Write<i32>(newID);
			note.WriteStringObj(cls->name.data());
			note.WriteStringObj(target.data());
			ForEachOnlineGuildMember(*guild, [&](i32, ClientHandle hd) { replication.SendWriter(hd, note); });
		} break;

		case MEASURED_CQ_SET_GUILD_MASTER: {
			WideString target;
			if(!ReadWideString(buff, &target, 32)) break;
			GuildMemberRecord* member = guild ? store.FindMember(*guild, target) : nullptr;
			u32 result = 0;
			if(!guild) result = MEASURED_GUILD_NOT_JOINED;
			else if(me->membershipID != MEASURED_GUILD_MASTER_MEMBERSHIP) result = MEASURED_GUILD_NO_PERMISSION_DELEGATEMASTER;
			else if(!member || target == nick) result = MEASURED_GUILD_INCORRECT_MASTER_DELEGATION;
			if(result == 0) {
				member->membershipID = MEASURED_GUILD_MASTER_MEMBERSHIP;
				me->membershipID = MEASURED_GUILD_REGULAR_MEMBERSHIP;
				store.AddHistory(*guild, MEASURED_GUILD_HISTORY_MANAGEMENT, MEASURED_GUILD_EVENT_DELEGATION, NowFiletime(), nick, target, 0, 0);
				store.Save();
			}
			PacketWriter<Sv::SA_SetGuildmaster,128> answer;
			answer.Write<u32>(result);
			answer.Write<i32>(MEASURED_GUILD_REGULAR_MEMBERSHIP);
			answer.WriteStringObj(target.data());
			replication.SendWriter(clientHd, answer);
			if(result != 0) break;
			PacketWriter<Sv::SN_GuildmasterDelegation,128> note;
			note.WriteStringObj(nick.data());
			note.Write<i32>(MEASURED_GUILD_REGULAR_MEMBERSHIP);
			note.WriteStringObj(target.data());
			ForEachOnlineGuildMember(*guild, [&](i32, ClientHandle hd) { replication.SendWriter(hd, note); });
			LOG("[client%x] Guild :: %ls hands %ls over to %ls", clientHd, nick.data(), guild->name.data(), target.data());
		} break;

		default: {
			WARN("[client%x] Guild :: packet %u not handled", clientHd, (u32)netID);
		} break;
	}
}

void GuildRewardBoost(const WideString& nickname, u16* goldRate, u16* expRate)
{
	GuildStore& store = GetGuildStore();
	std::lock_guard<std::recursive_mutex> lock(store.mutex);
	const GuildRecord* guild = store.FindByMember(nickname);
	*goldRate = guild ? (u16)(GuildSkillBonus(*guild, MEASURED_GUILD_SKILL_GOLD) + GuildSkillBonus(*guild, MEASURED_GUILD_SKILL_LIMITED_GOLD)) : 0;
	*expRate = guild ? (u16)(GuildSkillBonus(*guild, MEASURED_GUILD_SKILL_XP) + GuildSkillBonus(*guild, MEASURED_GUILD_SKILL_LIMITED_XP)) : 0;
}
