#pragma once
#include <common/base.h>

namespace In {

constexpr u32 MagicHandshake = 0xaedf45;

struct Q_Handshake
{
	enum { NET_ID = 1001 };

	u32 magic;
};

struct R_Handshake
{
	enum { NET_ID = 2001 };

	u8 result;
};

}
