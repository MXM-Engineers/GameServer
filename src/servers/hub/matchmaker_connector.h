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

	struct UpdateEvent
	{
		enum class Type: u8 {
			Invalid = 0,
			PartyCreated,
			PartyEnqueued,
			MatchFound,
			MatchCreated
		};

		Type type;
		InstanceUID instanceUID;

		union {
			struct {
				PartyUID UID;
				AccountUID leader;
			} PartyCreated;

			struct {
				PartyUID UID;
			} PartyEnqueued;

			struct {
				PartyUID UID;
				SortieUID sortieUID;
			} MatchFound;

			struct {
				SortieUID sortieUID;
				u16 playerCount;
				eastl::array<AccountUID,16> playerList;
			} MatchCreated;
		};

		UpdateEvent(Type type_, InstanceUID instanceUID_):
			type(type_),
			instanceUID(instanceUID_)
		{

		}
	};

	InnerConnection conn;

	ProfileMutex(Mutex, mutexQueries);
	eastl::fixed_vector<Query,2048> queries;

	MMQueryUID nextQueryUID = MMQueryUID(1);

	ProfileMutex(Mutex, mutexUpdates); // TODO: this is lazy, but could be enough
	eastl::fixed_vector<UpdateEvent, 1024> updateQueue;

	MatchmakerConnector();

	bool Init();
	void Update();

	void QueryPartyCreate(AccountUID leader);
	void QueryPartyEnqueue(PartyUID partyUID);
	void QueryPlayerNotifyRoomFound(AccountUID playerAccountUID, SortieUID sortieUID);
	void QueryPlayerRoomConfirm(AccountUID playerAccountUID, SortieUID sortieUID, u8 confirm);

	template<class OutputIterator>
	void ExtractInstanceUpdates(InstanceUID instUID, OutputIterator out)
	{
		LOCK_MUTEX(mutexUpdates);

		for(auto u = updateQueue.cbegin(); u != updateQueue.cend();) {
			if(u->instanceUID == instUID) {
				*out++ = *u;
				u = updateQueue.erase(u);
			}
			else {
				++u;
			}
		}
	}

private:
	void HandlePacket(const NetHeader& header, const u8* packetData);
};

MatchmakerConnector& Matchmaker();
