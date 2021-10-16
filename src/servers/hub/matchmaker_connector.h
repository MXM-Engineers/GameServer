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
	struct RoomPlayer
	{
		AccountUID accountUID;
		u8 team;
		u8 isBot;
		eastl::array<ClassType,2> masters;
		eastl::array<SkinIndex,2> skins;
		eastl::array<SkillID,4> skills;
		// TODO weapon
	};

	struct Query
	{
		enum class Type: u8 {
			Invalid = 0,
			PartyCreate,
			PartyEnqueue,
			PlayerNotifyRoomFound,
			PlayerRoomConfirm,
			RoomCreateGame
		};

		const MMQueryUID UID;
		const Type type;

		union {
			struct {
				In::StrName name;
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

			// NOTE: this one is big and is impacting the other queries in terms of size @Speed
			struct {
				SortieUID sortieUID;
				u8 playerCount;
				eastl::array<RoomPlayer,16> players;
			} RoomCreateGame;
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

	void QueryPartyCreate(const WideString& name, AccountUID leader);
	void QueryPartyEnqueue(PartyUID partyUID);
	void QueryPlayerNotifyRoomFound(AccountUID playerAccountUID, SortieUID sortieUID);
	void QueryPlayerRoomConfirm(AccountUID playerAccountUID, SortieUID sortieUID, u8 confirm);
	void QueryRoomCreateGame(SortieUID sortieUID, const RoomPlayer* playerList, u32 playerCount);
};

MatchmakerConnector& Matchmaker();
