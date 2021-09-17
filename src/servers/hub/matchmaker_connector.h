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
				InstanceUID instanceUID;
				AccountUID leader;
			} PartyCreate;

			struct {
				InstanceUID instanceUID;
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

	InnerConnection conn;

	ProfileMutex(Mutex, mutexQueries);
	eastl::fixed_vector<Query,2048> queries;

	MMQueryUID nextQueryUID = MMQueryUID(1);

	GrowableBuffer packetQueue;

	MatchmakerConnector();

	bool Init();
	void Update();

	void QueryPartyCreate(InstanceUID instanceUID, AccountUID leader);
	void QueryPartyEnqueue(PartyUID partyUID);
	void QueryPlayerNotifyRoomFound(AccountUID playerAccountUID, SortieUID sortieUID);
	void QueryPlayerRoomConfirm(AccountUID playerAccountUID, SortieUID sortieUID, u8 confirm);
};

MatchmakerConnector& Matchmaker();
