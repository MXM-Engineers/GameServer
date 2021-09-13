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
			PartyEnqueue
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
	};

	InnerConnection conn;

	ProfileMutex(Mutex, mutexQueries);
	eastl::fixed_vector<Query,2048> queries;

	MMQueryUID nextQueryUID = MMQueryUID(1);

	ProfileMutex(Mutex, mutexUpdates); // TODO: this is lazy, but could be enough
	eastl::fixed_vector<UpdatePartyCreated, 256> updatePartiesCreated;
	eastl::fixed_vector<UpdatePartyEnqueued, 256> updatePartiesEnqueued;
	eastl::fixed_vector<UpdateMatchFound, 256> updateMatchFound;

	MatchmakerConnector();

	bool Init();
	void Update();

	void QueryPartyCreate(AccountUID leader);
	void QueryPartyEnqueue(PartyUID partyUID);

private:
	void HandlePacket(const NetHeader& header, const u8* packetData);
};

MatchmakerConnector& Matchmaker();
