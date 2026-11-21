#include "guild_store.h"
#include <mxm/safe_file.h>
#include <stdio.h>
#include <direct.h>

static const char* GUILD_DIR = "accounts";
static const char* GUILD_PATH = "accounts/guilds.txt";
static const char* GUILD_TMP_PATH = "accounts/guilds.txt.tmp";
static const i32 GUILD_HISTORY_MAX = 100;

GuildStore& GetGuildStore()
{
	static GuildStore store;
	return store;
}

template<typename String>
static void WriteHex(FILE* f, const String& str)
{
	fputc(' ', f);
	if(str.empty()) {
		fputc('-', f);
		return;
	}
	foreach_const(c, str) fprintf(f, "%04x", (u32)(u16)*c);
}

template<typename String>
static bool ReadHex(const char* hex, String* out)
{
	out->clear();
	if(hex[0] == '-' && hex[1] == 0) return true;
	for(const char* c = hex; *c; c += 4) {
		u32 v = 0;
		if(!c[1] || !c[2] || !c[3] || sscanf(c, "%4x", &v) != 1) return false;
		out->push_back((wchar)v);
	}
	return true;
}

static char* NextToken(char** cursor)
{
	char* c = *cursor;
	while(*c == ' ') c++;
	if(!*c || *c == '\n' || *c == '\r') return nullptr;
	char* start = c;
	while(*c && *c != ' ' && *c != '\n' && *c != '\r') c++;
	if(*c) *c++ = 0;
	*cursor = c;
	return start;
}

void GuildStore::Load()
{
	if(loaded) return;
	loaded = true;
	FILE* f = fopen(GUILD_PATH, "rb");
	if(!f) return;
	static char line[4096];
	GuildRecord* guild = nullptr;
	while(fgets(line, sizeof(line), f)) {
		char* cursor = line;
		char* kind = NextToken(&cursor);
		if(!kind) continue;
		char* tok[16] = {};
		i32 n = 0;
		while(n < 16 && (tok[n] = NextToken(&cursor)) != nullptr) n++;
		if(kind[0] == 'N' && n >= 1) nextID = (u32)strtoul(tok[0], nullptr, 10);
		else if(kind[0] == 'G' && n >= 14) {
			guilds.push_back(GuildRecord());
			guild = &guilds.back();
			guild->id = (u32)strtoul(tok[0], nullptr, 10);
			ReadHex(tok[1], &guild->name);
			ReadHex(tok[2], &guild->tag);
			guild->emblemIndex = atoi(tok[3]);
			guild->point = atoi(tok[4]);
			guild->fund = atoi(tok[5]);
			guild->created = _atoi64(tok[6]);
			guild->dissolution = _atoi64(tok[7]);
			guild->joinType = (u8)atoi(tok[8]);
			for(i32 i = 0; i < 7 && tok[9][i]; i++) guild->interest[i] = (u8)(tok[9][i] == '1');
			ReadHex(tok[10], &guild->intro);
			ReadHex(tok[11], &guild->notice);
			guild->rollcallDay = _atoi64(tok[12]);
			guild->todayRollCall = atoi(tok[13]);
			if(guild->id >= nextID) nextID = guild->id + 1;
		}
		else if(!guild) continue;
		else if(kind[0] == 'C' && n >= 5) {
			GuildClass c;
			c.id = atoi(tok[0]);
			c.type = (u8)atoi(tok[1]);
			c.iconIndex = (u8)atoi(tok[2]);
			ReadHex(tok[3], &c.name);
			for(i32 i = 0; i < 10 && tok[4][i]; i++) c.rights[i] = (u8)(tok[4][i] == '1');
			guild->classes.push_back(c);
		}
		else if(kind[0] == 'M' && n >= 6) {
			GuildMemberRecord m;
			ReadHex(tok[0], &m.nickname);
			m.membershipID = atoi(tok[1]);
			m.contributedPoint = atoi(tok[2]);
			m.contributedFund = atoi(tok[3]);
			m.lastLogout = _atoi64(tok[4]);
			m.lastRollcallDay = _atoi64(tok[5]);
			if(n >= 7) m.joinedDay = _atoi64(tok[6]);
			guild->members.push_back(m);
		}
		else if(kind[0] == 'H' && n >= 7) {
			GuildHistoryRecord h;
			h.historyType = (u8)atoi(tok[0]);
			h.eventType = (u8)atoi(tok[1]);
			h.date = _atoi64(tok[2]);
			ReadHex(tok[3], &h.s1);
			ReadHex(tok[4], &h.s2);
			h.l1 = atoi(tok[5]);
			h.l2 = atoi(tok[6]);
			guild->history.push_back(h);
		}
		else if(kind[0] == 'S' && n >= 4) {
			GuildSkillRecord s;
			s.type = (u8)atoi(tok[0]);
			s.level = (u8)atoi(tok[1]);
			s.expiry = _atoi64(tok[2]);
			s.extensionCount = (u16)atoi(tok[3]);
			guild->skills.push_back(s);
		}
		else if(kind[0] == 'R' && n >= 1) {
			WideString nick;
			ReadHex(tok[0], &nick);
			guild->joinRequests.push_back(nick);
		}
		else if(kind[0] == 'I' && n >= 2) {
			GuildInvitation inv;
			ReadHex(tok[0], &inv.invitee);
			ReadHex(tok[1], &inv.inviter);
			guild->invitations.push_back(inv);
		}
	}
	fclose(f);
}

void GuildStore::Save()
{
	_mkdir(GUILD_DIR);
	FILE* f = fopen(GUILD_TMP_PATH, "wb");
	ASSERT(f);
	fprintf(f, "N %u\n", nextID);
	foreach_const(g, guilds) {
		fprintf(f, "G %u", g->id);
		WriteHex(f, g->name);
		WriteHex(f, g->tag);
		fprintf(f, " %d %d %d %lld %lld %u ", g->emblemIndex, g->point, g->fund, (long long)g->created, (long long)g->dissolution, (u32)g->joinType);
		for(u8 v : g->interest) fputc(v ? '1' : '0', f);
		WriteHex(f, g->intro);
		WriteHex(f, g->notice);
		fprintf(f, " %lld %d\n", (long long)g->rollcallDay, g->todayRollCall);
		foreach_const(c, g->classes) {
			fprintf(f, "C %d %u %u", c->id, (u32)c->type, (u32)c->iconIndex);
			WriteHex(f, c->name);
			fputc(' ', f);
			for(u8 v : c->rights) fputc(v ? '1' : '0', f);
			fputc('\n', f);
		}
		foreach_const(m, g->members) {
			fputs("M", f);
			WriteHex(f, m->nickname);
			fprintf(f, " %d %d %d %lld %lld %lld\n", m->membershipID, m->contributedPoint, m->contributedFund, (long long)m->lastLogout, (long long)m->lastRollcallDay, (long long)m->joinedDay);
		}
		foreach_const(h, g->history) {
			fprintf(f, "H %u %u %lld", (u32)h->historyType, (u32)h->eventType, (long long)h->date);
			WriteHex(f, h->s1);
			WriteHex(f, h->s2);
			fprintf(f, " %d %d\n", h->l1, h->l2);
		}
		foreach_const(s, g->skills) fprintf(f, "S %u %u %lld %u\n", (u32)s->type, (u32)s->level, (long long)s->expiry, (u32)s->extensionCount);
		foreach_const(r, g->joinRequests) {
			fputs("R", f);
			WriteHex(f, *r);
			fputc('\n', f);
		}
		foreach_const(i, g->invitations) {
			fputs("I", f);
			WriteHex(f, i->invitee);
			WriteHex(f, i->inviter);
			fputc('\n', f);
		}
	}
	SafeFileCommit(f, GUILD_TMP_PATH, GUILD_PATH);
}

GuildRecord* GuildStore::FindByMember(const WideString& nickname)
{
	Load();
	foreach(g, guilds) {
		foreach_const(m, g->members) if(m->nickname == nickname) return &*g;
	}
	return nullptr;
}

GuildRecord* GuildStore::FindByName(const WideString& name)
{
	Load();
	foreach(g, guilds) if(g->name == name) return &*g;
	return nullptr;
}

GuildRecord* GuildStore::FindByID(u32 id)
{
	Load();
	foreach(g, guilds) if(g->id == id) return &*g;
	return nullptr;
}

GuildMemberRecord* GuildStore::FindMember(GuildRecord& guild, const WideString& nickname)
{
	foreach(m, guild.members) if(m->nickname == nickname) return &*m;
	return nullptr;
}

void GuildStore::AddHistory(GuildRecord& guild, u8 historyType, u8 eventType, i64 date, const WideString& s1, const WideString& s2, i32 l1, i32 l2)
{
	GuildHistoryRecord h;
	h.historyType = historyType;
	h.eventType = eventType;
	h.date = date;
	h.s1 = s1;
	h.s2 = s2;
	h.l1 = l1;
	h.l2 = l2;
	guild.history.insert(guild.history.begin(), h);
	if((i32)guild.history.size() > GUILD_HISTORY_MAX) guild.history.resize(GUILD_HISTORY_MAX);
}
