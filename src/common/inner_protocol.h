#pragma once
#include <common/base.h>
#include <common/protocol.h>
#include <EASTL/array.h>

// MQ, MR: Matchmaker Query, Response
// HQ, HR: Hub server Query, Response
// PQ, PR: Play server Query, Response

enum class AccountUID: u32 {
	INVALID = 0
};

enum class PartyUID: u32 {
	INVALID = 0
};

enum class InstanceUID: u32 {
	INVALID = 0
};

namespace In {

constexpr u32 MagicHandshake = 0xaedf45;

struct HQ_Handshake
{
	enum { NET_ID = 1001 };

	u32 magic;
};

struct HQ_PartyCreate
{
	enum { NET_ID = 1002 };

	// TODO: some useful stuff here
	AccountUID leader;
};

struct HQ_PartyEnqueue
{
	enum { NET_ID = 1003 };

	// TODO: some useful stuff here
	PartyUID partyUID;
};

PUSH_PACKED
struct HN_PlayerRoomFound
{
	enum { NET_ID = 1004 };

	AccountUID accountUID;
	SortieUID sortieUID;
};
POP_PACKED

PUSH_PACKED
struct HN_PlayerRoomConfirm
{
	enum { NET_ID = 1005 };

	AccountUID accountUID;
	u8 confirm;
	SortieUID sortieUID;
};
POP_PACKED

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
	AccountUID leader;
};

struct MR_PartyEnqueued
{
	enum { NET_ID = 3004 };

	// TODO: some useful stuff here
	u8 result;
	PartyUID partyUID;
};

struct MN_MatchFound
{
	enum { NET_ID = 3005 };

	// TODO: some useful stuff here
	PartyUID partyUID;
	SortieUID sortieUID;
};

struct MN_RoomCreated
{
	enum { NET_ID = 3006 };

	struct Player
	{
		AccountUID accountUID;
		u8 team; // 0: red, 1: blue, 2: spectators
	};

	SortieUID sortieUID;
	u8 playerCount;
	eastl::array<Player,16> playerList;
};

}
