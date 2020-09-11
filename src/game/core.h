#pragma once
#include <common/base.h>

enum class ActorUID: u32
{
	INVALID = 0,
};

enum class CoreAffinity: i32
{
	MAIN = 0,
	NETWORK = 1,
	COORDINATOR = 2,
	CHANNELS = 3,
};

constexpr i32 JUKEBOX_MAX_TRACKS = 8;
