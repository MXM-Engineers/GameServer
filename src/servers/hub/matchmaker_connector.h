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
		eastl::array<WeaponIndex,2> weapons;
		eastl::array<i32,2> weaponGrades;
		eastl::array<u8,2> masterGearNo;
		eastl::array<i32,2> characterType;
		In::StrGuildTag guildTag;
		SkillID stageSkill;
		u16 guildBoostGold = 0;
		u16 guildBoostExp = 0;
	};

	struct Query
	{
		enum class Type: u8 {
			Invalid = 0,
			PartyCreate,
			PartyAddMember,
			PartyRemoveMember,
			PartyCancelMatching,
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
				In::StrName name;
				AccountUID accountUID;
			} PartyAddMember;

			struct {
				PartyUID partyUID;
				AccountUID accountUID;
			} PartyRemoveMember;

			struct {
				PartyUID partyUID;
			} PartyCancelMatching;

			struct {
				PartyUID partyUID;
				AreaIndex areaIndex;
				StageIndex stageIndex;
				MapIndex mapIndex;
				GameType gameType;
				In::CustomTeams custom;
				u8 draft;
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
	void QueryPartyAddMember(PartyUID partyUID, const WideString& name, AccountUID accountUID);
	void QueryPartyRemoveMember(PartyUID partyUID, AccountUID accountUID);
	void QueryPartyCancelMatching(PartyUID partyUID);
	void QueryPartyEnqueue(PartyUID partyUID, AreaIndex areaIndex, StageIndex stageIndex, MapIndex mapIndex, GameType gameType, const In::CustomTeams* custom, bool draft);
	void QueryPlayerNotifyRoomFound(AccountUID playerAccountUID, SortieUID sortieUID);
	void QueryPlayerRoomConfirm(AccountUID playerAccountUID, SortieUID sortieUID, u8 confirm);
	void QueryRoomCreateGame(SortieUID sortieUID, const RoomPlayer* playerList, u32 playerCount);
};

MatchmakerConnector& Matchmaker();
