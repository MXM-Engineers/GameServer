#pragma once
#include <common/base.h>

#define ASSERT_SIZE(T, SIZE) STATIC_ASSERT(sizeof(T) == SIZE)

struct NetHeader
{
	u16 size;
	u16 netID;
};

ASSERT_SIZE(NetHeader, 4);

struct Cl_Hello
{
	enum { NET_ID = 60002 };

	char key[13];
};

ASSERT_SIZE(Cl_Hello, 13);

struct Cl_Login
{
	enum { NET_ID = 60003 };


	u8 data[1]; // variable size
};

struct Cl_ConfirmLogin
{
	enum { NET_ID = 60035 };
};

ASSERT_SIZE(Cl_ConfirmLogin, 1);

struct Sv_Hello
{
	enum { NET_ID = 62002 };

	char key[16];
};

ASSERT_SIZE(Sv_Hello, 16);

// type 1 is 62004 - related to packet encryption perhaps?
struct Sv_AcceptLoginType0
{
	enum { NET_ID = 62003 };

	i32 ping;
};

ASSERT_SIZE(Sv_AcceptLoginType0, 4);

struct Sv_AccountData
{
	enum { NET_ID = 62009 };

	u8 unk[16];
	u8 data[1]; // variable size
};
