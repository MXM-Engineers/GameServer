#pragma once
#include <common/base.h>

namespace In {

constexpr u32 MagicHandshake = 0xaedf45;

enum class ConnType: u8
{
	Undecided = 0,
	HubServer = 1,
	PlayServer = 2
};

PUSH_PACKED
struct Q_Handshake
{
	enum { NET_ID = 1001 };

	ConnType type;
	u32 magic;
};
POP_PACKED

struct R_Handshake
{
	enum { NET_ID = 2001 };

	u8 result;
};

}
