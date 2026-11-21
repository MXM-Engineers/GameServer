#pragma once
#include <common/base.h>
#include <common/utils.h>
#include <EASTL/vector.h>
#include <EASTL/fixed_string.h>
#include <EASTL/fixed_vector.h>

struct MailItem
{
	i32 index;
	i32 count;
};

struct MailMessage
{
	u64 ID;
	WideString owner;
	WideString from;
	eastl::fixed_string<wchar,64,true> subject;
	eastl::fixed_string<wchar,512,true> content;
	i64 sendFiletime;
	i64 expireFiletime;
	u8 mailbox;
	bool read;
	bool system = false;
	eastl::fixed_vector<MailItem,8,false> items;
};

struct MailCounts
{
	u16 unread[3];
	u16 total[3];
};

enum class MailWriteResult: u8
{
	SUCCESS,
	EMPTY_NICKNAME,
	EMPTY_SUBJECT,
	EMPTY_CONTENTS,
	EXCESS_SUBJECT,
	EXCESS_CONTENTS,
	UNKNOWN_RECIPIENT,
	RECIPIENT_FULL,
};

static const i32 MAILBOX_INBOX = 0;
static const i32 MAILBOX_ARCHIVE = 1;
static const u8 MAIL_MOVE_ARCHIVE = 1;
static const u8 MAIL_MOVE_DELETE = 2;
static const i32 MAILBOX_CAPACITY = 50;
static const i64 MAIL_LIFETIME_DAYS = 30;

void MailRegisterAccount(const wchar* nickname);
MailWriteResult MailSend(const wchar* from, const wchar* to, const wchar* subject, i32 subjectLen, const wchar* content, i32 contentLen, MailMessage* sent);
MailCounts MailGetCounts(const wchar* owner);
void MailList(const wchar* owner, u8 mailbox, eastl::vector<MailMessage>* out);
bool MailRead(const wchar* owner, u64 mailID, MailMessage* out);
i32 MailMove(const wchar* owner, u8 moveType, const u64* ids, i32 count);
bool MailSendSystem(const wchar* to, const wchar* subjectKey, const wchar* contentKey, const MailItem* items, i32 itemCount, MailMessage* sent);
bool MailTakeAttachments(const wchar* owner, u64 mailID, MailMessage* taken);
static const wchar* MEASURED_SYSTEM_MAIL_SENDER = L"$$SYS_NAME";

