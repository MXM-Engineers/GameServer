#pragma once
#include <common/base.h>

#define ASSERT_SIZE(T, SIZE) STATIC_ASSERT(sizeof(T) == SIZE)

struct NetHeader
{
	u16 size;
	u16 netID;
};

ASSERT_SIZE(NetHeader, 4);


// CLIENT -----------------------------------------------------
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

// ?
struct Cl_ConfirmGatewayInfo
{
	enum { NET_ID = 60005 };
	i32 var;
};

ASSERT_SIZE(Cl_ConfirmGatewayInfo, 4);

PUSH_PACKED
struct Cl_EnterQueue
{
	enum { NET_ID = 60007 };

	i32 var1;
	u8 gameIp[4];
	u16 unk;
	u8 pingIp[4];
	u16 port; // ?
	u16 unk2;
	i32 stationID;
	u8 unk3[8];
};
POP_PACKED
ASSERT_SIZE(Cl_EnterQueue, 30);


// SERVER -----------------------------------------------------
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

struct Sv_GatewayServerInfo
{
	enum { NET_ID = 62047 };

	u16 strLen;
	wchar str[1]; // variable size
};

struct Sv_SendStationList
{
	enum { NET_ID = 62007 };

	u16 count;

	struct Station
	{
		u32 ID;
		u16 count;

		struct ServerIP
		{
			u8 game[4];
			u8 ping[4];
			u16 port;
		};

		ServerIP serverIPs[1]; // variable size
	};

	Station stations[1]; // variable size
};

// maybe?
PUSH_PACKED
struct Sv_QueueStatus
{
	enum { NET_ID = 62501 };

	i32 var1;
	u8 unk[5];
	i32 var2;
};
POP_PACKED
ASSERT_SIZE(Sv_QueueStatus, 13);

struct Sv_Finish
{
	enum { NET_ID = 62011 };

	u16 count;
	u8 ip[4];
	u16 port;

	u16 serverNamelen;
	wchar serverName[1]; // length is serverNamelen

	u16 nickLen;
	wchar nick[1]; // length is nickLen

	i32 var1;
	i32 var2;
};
