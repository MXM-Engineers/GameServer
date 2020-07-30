#pragma once
#include <common/base.h>

#define ASSERT_SIZE(T, SIZE) STATIC_ASSERT(sizeof(T) == SIZE)

struct NetHeader
{
	u16 size;
	u16 netID;
};

ASSERT_SIZE(NetHeader, 4);

// Client packets
namespace Cl {

struct Hello
{
	enum { NET_ID = 60002 };

	char key[13];
};

ASSERT_SIZE(Hello, 13);

struct UserLogin
{
	enum { NET_ID = 60003 };


	u8 data[1]; // variable size
};

struct ConfirmLogin
{
	enum { NET_ID = 60035 };
};

ASSERT_SIZE(ConfirmLogin, 1);

// ?
struct ConfirmGatewayInfo
{
	enum { NET_ID = 60005 };
	i32 var;
};

ASSERT_SIZE(ConfirmGatewayInfo, 4);

PUSH_PACKED
struct EnterQueue
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
ASSERT_SIZE(EnterQueue, 30);

struct RequestConnectGame
{
	enum { NET_ID = 60008 };

	u16 nickLen;
	wchar nick[1]; // length is nickLen
	i32 var;
};

} // Cl


// Server packets
namespace Sv {

PUSH_PACKED
struct SA_FirstHello
{
	enum { NET_ID = 62002 };

	u32 dwProtocolCRC;
	u32 dwErrorCRC;
	u8 serverType;
	u8 clientIp[4];
	u16 clientPort;
	u8 tqosWorldId;
};
POP_PACKED
ASSERT_SIZE(SA_FirstHello, 16);

// type 1 is 62004 - related to packet encryption perhaps?
struct SA_UserloginResult
{
	enum { NET_ID = 62003 };

	i32 result;
};

ASSERT_SIZE(SA_UserloginResult, 4);

struct SA_AuthResult
{
	enum { NET_ID = 62005 };

	i32 result;
};

ASSERT_SIZE(SA_UserloginResult, 4);

struct SN_TgchatServerInfo
{
	enum { NET_ID = 62009 };

	u8 data[1]; // variable size
};

struct SA_VersionInfo
{
	enum { NET_ID = 62047 };

	u16 strLen;
	wchar str[1]; // variable size
};

struct SN_RegionServicePolicy
{
	enum { NET_ID = 62006 };

	// TODO: fill in
};

struct SendStationList
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
struct QueueStatus
{
	enum { NET_ID = 62501 };

	i32 var1;
	u8 unk[5];
	i32 var2;
};
POP_PACKED
ASSERT_SIZE(QueueStatus, 13);

struct Finish
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

} // Sv
