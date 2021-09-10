#pragma once
#include <common/base.h>
#include <common/protocol.h>
#include <EASTL/array.h>

// MQ, MR: Matchmaker Query, Response
// HQ, HR: Hub server Query, Response
// PQ, PR: Play server Query, Response

namespace In {

constexpr u32 MagicHandshake = 0xaedf45;

enum class ConnType: u8
{
	Undecided = 0,
	HubServer = 1,
	PlayServer = 2
};

enum class PartyUID: u32 {
	INVALID = 0
};

struct HQ_Handshake
{
	enum { NET_ID = 1001 };

	u32 magic;
};

struct HQ_PartyCreate
{
	enum { NET_ID = 1002 };

	// TODO: some useful stuff here
	u32 dummy;
};

struct HQ_PartyEnqueue
{
	enum { NET_ID = 1003 };

	// TODO: some useful stuff here
	u32 dummy;
};

struct MQ_CreatePlaySession
{
	enum { NET_ID = 3001 };

	struct Player
	{
		u32 accountID; // TODO: AccountID type
		ClassType mainMaster;
		ClassType subMaster;
	};

	eastl::array<Player, 6> players;
};

struct MR_Handshake
{
	enum { NET_ID = 3002 };

	u8 result;
};

struct MR_PartyCreated
{
	enum { NET_ID = 3003 };

	// TODO: some useful stuff here
	u8 result;
	PartyUID partyUID;
};

struct MR_PartyEnqueued
{
	enum { NET_ID = 3004 };

	// TODO: some useful stuff here
	u8 result;
	PartyUID partyUID;
};

struct PQ_Handshake
{
	enum { NET_ID = 2001 };

	u32 magic;
};

struct PR_CreatePlaySession
{
	enum { NET_ID = 2002 };

	u8 result;
};

}
