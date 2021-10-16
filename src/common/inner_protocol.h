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

namespace In {

constexpr u32 MagicHandshake = 0xaedf45;

template<u16 CAPACITY>
struct FixedWideString
{
	u16 len;
	wchar data[CAPACITY];

	FixedWideString() = default;

	// convenient copy function
	template<i32 CAP, bool Overflow>
	inline void Copy(const eastl::fixed_string<wchar,CAP,Overflow>& str) {
		ASSERT(str.size() <= CAPACITY);
		len = str.size();
		memmove(data, str.data(), sizeof(wchar) * len);
	}
};

typedef FixedWideString<32> StrName;

inline u32 ProduceInstantKey(AccountUID accountUID, SortieUID sortieUID)
{
	PUSH_PACKED
	struct {
		AccountUID accountUID;
		SortieUID sortieUID;
	} data;
	POP_PACKED

	data.accountUID = accountUID;
	data.sortieUID = sortieUID;

	// TODO: XOR encryption
	return hash_fnv1a(&data, sizeof(data));
}

struct RoomUser
{
	StrName name;
	AccountUID accountUID;
	u8 team; // 0: red, 1: blue, 2: spectators
	u8 isBot;
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
	StrName name;
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

// TODO: don't send that much information, in theory the Matchmaker already knows all of it
// but for now we assume we only have one hub so we don't relay the info as we should.
// Fix it. - LordSk (11/10/2021)
PUSH_PACKED
struct HQ_RoomCreateGame
{
	enum { NET_ID = 1006 };

	struct Player
	{
		AccountUID accountUID;
		u8 team: 3;
		u8 isBot: 1;
		eastl::array<ClassType,2> masters;
		eastl::array<SkinIndex,2> skins;
		eastl::array<SkillID,4> skills;
		// TODO weapon
	};

	SortieUID sortieUID;
	u8 playerCount;
	u8 spectatorCount;
	eastl::array<Player,10> players;
	eastl::array<AccountUID,6> spectators;
};
POP_PACKED

struct PQ_Handshake
{
	enum { NET_ID = 2001 };

	u32 magic;
	u16 listenPort;
};

struct PR_GameCreated
{
	enum { NET_ID = 2002 };

	SortieUID sortieUID;
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

struct MN_MatchingPartyFound
{
	enum { NET_ID = 3005 };

	// TODO: some useful stuff here
	PartyUID partyUID;
	SortieUID sortieUID;
	u8 playerCount;
	eastl::array<RoomUser,16> playerList;
};

struct MN_RoomCreated
{
	enum { NET_ID = 3006 };

	SortieUID sortieUID;
	u8 playerCount;
	eastl::array<RoomUser,16> playerList;
};

struct MQ_CreateGame
{
	enum { NET_ID = 3007 };

	struct Player
	{
		StrName name;
		AccountUID accountUID;
		u8 team: 3;
		u8 isBot: 1;
		eastl::array<ClassType,2> masters;
		eastl::array<SkinIndex,2> skins;
		eastl::array<SkillID,4> skills;
		// TODO weapon
	};

	SortieUID sortieUID;
	u8 playerCount;
	u8 spectatorCount;
	eastl::array<Player,10> players;
	eastl::array<AccountUID,6> spectators;
};

PUSH_PACKED
struct MN_MatchCreated
{
	enum { NET_ID = 3008 };

	SortieUID sortieUID;
	eastl::array<u8,4> serverIp;
	u16 serverPort;
};
POP_PACKED

}
