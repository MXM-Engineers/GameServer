#pragma once
#include <common/network.h>
#include <common/inner_protocol.h>

enum class MMQueryUID: u32 {
	INVALID = 0
};

struct MatchFindFilter
{
	// pvp: 3v3, 5v5
	// pve: map, level?
	// mini game
	// ?

	u8 region;
};

struct MatchmakerConnector
{
	struct Query
	{
		enum class Type: u8 {
			Invalid = 0,
			PartyCreate,
			PartyEnqueue,
			PlayerNotifyRoomFound,
			PlayerRoomConfirm
		};

		const MMQueryUID UID;
		const Type type;

		union {
			struct {
				AccountUID leader;
			} PartyCreate;

			struct {
				PartyUID partyUID;
			} PartyEnqueue;

			struct {
				AccountUID playerAccountUID;
				SortieUID sortieUID;
			} PlayerNotifyRoomFound;

			struct {
				AccountUID playerAccountUID;
				u8 confirm;
				SortieUID sortieUID;
			} PlayerRoomConfirm;
		};

		explicit Query(MMQueryUID UID_, Type type_): UID(UID_), type(type_) {}
	};

	struct UpdatePartyCreated
	{
		PartyUID UID;
		AccountUID leader;
	};

	struct UpdatePartyEnqueued
	{
		PartyUID UID;
	};

	struct UpdateMatchFound
	{
		PartyUID UID;
		SortieUID sortieUID;
	};

	struct UpdateSortieBegin
	{
		SortieUID sortieUID;
		eastl::fixed_vector<AccountUID,16,false> playerList;
	};

	InnerConnection conn;

	ProfileMutex(Mutex, mutexQueries);
	eastl::fixed_vector<Query,2048> queries;

	MMQueryUID nextQueryUID = MMQueryUID(1);

	ProfileMutex(Mutex, mutexUpdates); // TODO: this is lazy, but could be enough
	// TODO: union?
	eastl::fixed_vector<UpdatePartyCreated, 256> updatePartiesCreated;
	eastl::fixed_vector<UpdatePartyEnqueued, 256> updatePartiesEnqueued;
	eastl::fixed_vector<UpdateMatchFound, 256> updateMatchFound;
	eastl::fixed_vector<UpdateSortieBegin, 256> updateSortieBegin;

	MatchmakerConnector();

	bool Init();
	void Update();

	void QueryPartyCreate(AccountUID leader);
	void QueryPartyEnqueue(PartyUID partyUID);
	void QueryPlayerNotifyRoomFound(AccountUID playerAccountUID, SortieUID sortieUID);
	void QueryPlayerRoomConfirm(AccountUID playerAccountUID, SortieUID sortieUID, u8 confirm);

private:
	void HandlePacket(const NetHeader& header, const u8* packetData);
};

MatchmakerConnector& Matchmaker();
