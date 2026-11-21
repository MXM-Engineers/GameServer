#include "mailbox.h"
#include <mxm/safe_file.h>
#include <mutex>
#include <time.h>
#include <stdio.h>
#include <direct.h>
#include <EASTL/algorithm.h>
#include <EASTL/sort.h>

static std::mutex g_mailMutex;
static eastl::vector<MailMessage> g_mails;
static eastl::vector<WideString> g_mailAccounts;
static u64 g_nextMailID = 1;
static bool g_mailLoaded = false;

static const char* MAIL_DIR = "accounts";
static const char* MAIL_PATH = "accounts/mail.txt";
static const char* MAIL_TMP_PATH = "accounts/mail.txt.tmp";

static const i64 FILETIME_UNIX_EPOCH = 116444736000000000LL;
static const i64 FILETIME_PER_SEC = 10000000LL;
static const i64 MAIL_NO_EXPIRY = 0;

static i64 NowFiletime()
{
	return FILETIME_UNIX_EPOCH + (i64)time(nullptr) * FILETIME_PER_SEC;
}

static void WriteHex(FILE* f, const wchar* str, i32 len)
{
	if(len <= 0) {
		fputc('-', f);
		return;
	}
	for(i32 i = 0; i < len; i++) fprintf(f, "%04x", (u32)(u16)str[i]);
}

template<typename String>
static bool ReadHex(const char* hex, String* out)
{
	out->clear();
	if(hex[0] == '-' && hex[1] == 0) return true;
	for(const char* c = hex; *c; c += 4) {
		u32 v = 0;
		if(sscanf(c, "%4x", &v) != 1) return false;
		out->push_back((wchar)v);
		if(!c[1] || !c[2] || !c[3]) return false;
	}
	return true;
}

static void SaveMails()
{
	_mkdir(MAIL_DIR);
	FILE* f = fopen(MAIL_TMP_PATH, "wb");
	if(!f) return;
	fprintf(f, "N %llu\n", (unsigned long long)g_nextMailID);
	foreach_const(a, g_mailAccounts) {
		fputs("A ", f);
		WriteHex(f, a->data(), (i32)a->size());
		fputc('\n', f);
	}
	foreach_const(m, g_mails) {
		fprintf(f, "M %llu ", (unsigned long long)m->ID);
		WriteHex(f, m->owner.data(), (i32)m->owner.size());
		fputc(' ', f);
		WriteHex(f, m->from.data(), (i32)m->from.size());
		fputc(' ', f);
		WriteHex(f, m->subject.data(), (i32)m->subject.size());
		fputc(' ', f);
		WriteHex(f, m->content.data(), (i32)m->content.size());
		fprintf(f, " %lld %lld %u %u %u %u",(long long)m->sendFiletime, (long long)m->expireFiletime, (u32)m->mailbox, (u32)m->read, (u32)m->system, (u32)m->items.size());
		foreach_const(it, m->items) fprintf(f, " %d %d", it->index, it->count);
		fputc('\n', f);
	}
	SafeFileCommit(f, MAIL_TMP_PATH, MAIL_PATH);
}

static void LoadMails()
{
	if(g_mailLoaded) return;
	g_mailLoaded = true;
	FILE* f = fopen(MAIL_PATH, "rb");
	if(!f) return;
	static char line[8192];
	static char owner[512], from[512], subject[1024], content[4096];
	while(fgets(line, sizeof(line), f)) {
		unsigned long long id = 0;
		if(sscanf(line, "N %llu", &id) == 1) {
			g_nextMailID = (u64)id;
			continue;
		}
		if(sscanf(line, "A %511s", owner) == 1) {
			WideString name;
			if(ReadHex(owner, &name) && !name.empty()) g_mailAccounts.push_back(name);
			continue;
		}
		long long sendTime = 0, expireTime = 0;
		u32 mailbox = 0, read = 0, system = 0, itemCount = 0;
		i32 used = 0;
		if(sscanf(line, "M %llu %511s %511s %1023s %4095s %lld %lld %u %u%n", &id, owner, from, subject, content, &sendTime, &expireTime, &mailbox, &read, &used) != 9) continue;
		const char* tail = line + used;
		i32 tailUsed = 0;
		if(sscanf(tail, " %u %u%n", &system, &itemCount, &tailUsed) == 2) tail += tailUsed;
		else itemCount = 0;
		MailMessage mail;
		mail.ID = (u64)id;
		if(!ReadHex(owner, &mail.owner) || !ReadHex(from, &mail.from) || !ReadHex(subject, &mail.subject) || !ReadHex(content, &mail.content)) continue;
		mail.sendFiletime = (i64)sendTime;
		mail.expireFiletime = (i64)expireTime;
		mail.mailbox = (u8)mailbox;
		mail.read = read != 0;
		mail.system = system != 0;
		for(u32 i = 0; i < itemCount && !mail.items.full(); i++) {
			MailItem item;
			if(sscanf(tail, " %d %d%n", &item.index, &item.count, &tailUsed) != 2) break;
			tail += tailUsed;
			mail.items.push_back(item);
		}
		g_mails.push_back(mail);
		if(mail.ID >= g_nextMailID) g_nextMailID = mail.ID + 1;
	}
	fclose(f);
}

static bool KnownAccount(const wchar* nickname)
{
	foreach_const(a, g_mailAccounts) if(a->compare(nickname) == 0) return true;
	return false;
}

static void PurgeExpired(i64 now)
{
	LoadMails();
	const size_t before = g_mails.size();
	g_mails.erase(eastl::remove_if(g_mails.begin(), g_mails.end(), [now](const MailMessage& m) { return m.expireFiletime != MAIL_NO_EXPIRY && m.expireFiletime <= now; }), g_mails.end());
	if(g_mails.size() != before) SaveMails();
}

void MailRegisterAccount(const wchar* nickname)
{
	std::lock_guard<std::mutex> lock(g_mailMutex);
	LoadMails();
	if(KnownAccount(nickname)) return;
	g_mailAccounts.push_back(WideString(nickname));
	SaveMails();
}

MailWriteResult MailSend(const wchar* from, const wchar* to, const wchar* subject, i32 subjectLen, const wchar* content, i32 contentLen, MailMessage* sent)
{
	if(!to || !to[0]) return MailWriteResult::EMPTY_NICKNAME;
	if(subjectLen <= 0) return MailWriteResult::EMPTY_SUBJECT;
	if(contentLen <= 0) return MailWriteResult::EMPTY_CONTENTS;
	if(subjectLen >= 64) return MailWriteResult::EXCESS_SUBJECT;
	if(contentLen >= 512) return MailWriteResult::EXCESS_CONTENTS;

	std::lock_guard<std::mutex> lock(g_mailMutex);
	LoadMails();
	if(!KnownAccount(to)) return MailWriteResult::UNKNOWN_RECIPIENT;
	const i64 now = NowFiletime();
	PurgeExpired(now);
	i32 owned = 0;
	foreach_const(m, g_mails) if(m->owner.compare(to) == 0 && m->mailbox == MAILBOX_INBOX) owned++;
	if(owned >= MAILBOX_CAPACITY) return MailWriteResult::RECIPIENT_FULL;

	MailMessage mail;
	mail.ID = g_nextMailID++;
	mail.owner = to;
	mail.from = from;
	mail.subject.assign(subject, subjectLen);
	mail.content.assign(content, contentLen);
	mail.sendFiletime = now;
	mail.expireFiletime = now + MAIL_LIFETIME_DAYS * 86400 * FILETIME_PER_SEC;
	mail.mailbox = MAILBOX_INBOX;
	mail.read = false;
	g_mails.push_back(mail);
	SaveMails();
	*sent = mail;
	return MailWriteResult::SUCCESS;
}

MailCounts MailGetCounts(const wchar* owner)
{
	std::lock_guard<std::mutex> lock(g_mailMutex);
	PurgeExpired(NowFiletime());
	MailCounts counts = {};
	foreach_const(m, g_mails) {
		if(m->owner.compare(owner) != 0 || m->mailbox > 2) continue;
		counts.total[m->mailbox]++;
		if(!m->read) counts.unread[m->mailbox]++;
	}
	return counts;
}

void MailList(const wchar* owner, u8 mailbox, eastl::vector<MailMessage>* out)
{
	std::lock_guard<std::mutex> lock(g_mailMutex);
	PurgeExpired(NowFiletime());
	foreach_const(m, g_mails) if(m->owner.compare(owner) == 0 && m->mailbox == mailbox) out->push_back(*m);
	eastl::sort(out->begin(), out->end(), [](const MailMessage& a, const MailMessage& b) { return a.ID > b.ID; });
}

bool MailRead(const wchar* owner, u64 mailID, MailMessage* out)
{
	std::lock_guard<std::mutex> lock(g_mailMutex);
	LoadMails();
	foreach(m, g_mails) {
		if(m->ID != mailID || m->owner.compare(owner) != 0) continue;
		if(!m->read) {
			m->read = true;
			SaveMails();
		}
		*out = *m;
		return true;
	}
	return false;
}

i32 MailMove(const wchar* owner, u8 moveType, const u64* ids, i32 count)
{
	std::lock_guard<std::mutex> lock(g_mailMutex);
	LoadMails();
	i32 moved = 0;
	for(i32 i = 0; i < count; i++) {
		for(auto m = g_mails.begin(); m != g_mails.end(); ++m) {
			if(m->ID != ids[i] || m->owner.compare(owner) != 0) continue;
			if(moveType == MAIL_MOVE_ARCHIVE) m->mailbox = MAILBOX_ARCHIVE;
			else if(moveType == MAIL_MOVE_DELETE && !m->items.empty()) break;
			else if(moveType == MAIL_MOVE_DELETE) g_mails.erase(m);
			else {
				if(moved > 0) SaveMails();
				return moved;
			}
			moved++;
			break;
		}
	}
	if(moved > 0) SaveMails();
	return moved;
}

bool MailSendSystem(const wchar* to, const wchar* subjectKey, const wchar* contentKey, const MailItem* items, i32 itemCount, MailMessage* sent)
{
	std::lock_guard<std::mutex> lock(g_mailMutex);
	LoadMails();
	if(!KnownAccount(to)) return false;
	const i64 now = NowFiletime();
	PurgeExpired(now);
	MailMessage mail;
	mail.ID = g_nextMailID++;
	mail.owner = to;
	mail.from = MEASURED_SYSTEM_MAIL_SENDER;
	mail.subject = subjectKey;
	mail.content = contentKey;
	mail.sendFiletime = now;
	mail.mailbox = MAILBOX_INBOX;
	mail.read = false;
	mail.system = true;
	for(i32 i = 0; i < itemCount && !mail.items.full(); i++) mail.items.push_back(items[i]);
	mail.expireFiletime = mail.items.empty() ? now + MAIL_LIFETIME_DAYS * 86400 * FILETIME_PER_SEC : MAIL_NO_EXPIRY;
	g_mails.push_back(mail);
	SaveMails();
	if(sent) *sent = mail;
	return true;
}

bool MailTakeAttachments(const wchar* owner, u64 mailID, MailMessage* taken)
{
	std::lock_guard<std::mutex> lock(g_mailMutex);
	LoadMails();
	foreach(m, g_mails) {
		if(m->ID != mailID || m->owner.compare(owner) != 0) continue;
		if(m->items.empty()) return false;
		*taken = *m;
		m->items.clear();
		m->read = true;
		m->expireFiletime = NowFiletime() + MAIL_LIFETIME_DAYS * 86400 * FILETIME_PER_SEC;
		taken->expireFiletime = m->expireFiletime;
		SaveMails();
		return true;
	}
	return false;
}
