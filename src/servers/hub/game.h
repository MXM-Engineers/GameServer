#pragma once
#include <common/base.h>
#include <common/protocol.h>
#include <common/inner_protocol.h>
#include <common/network.h>
#include <common/utils.h>
#include <EASTL/fixed_list.h>
#include <EASTL/fixed_hash_map.h>

#include "world.h"
#include "matchmaker_connector.h"
#include "guild_store.h"
#include <mxm/account_progress.h>

struct HubGame
{
	enum {
		MAX_PLAYERS = MAX_CLIENTS
	};

	struct SpawnPoint
	{
		vec3 pos, dir;
	};

	struct Player
	{
		const ClientHandle clientHd;
		PartyUID partyUID = PartyUID::INVALID;
		SortieUID sortieUID = SortieUID::INVALID;
		SortieUID resultSortieUID = SortieUID::INVALID;
		SortieUID suspendedSortieUID = SortieUID::INVALID;
		AccountUID suspendedAccountUID = AccountUID::INVALID;

		Player(): clientHd(ClientHandle::INVALID) {}

		Player(ClientHandle clientHd_):
			clientHd(clientHd_) {}
	};

	struct Party
	{
		struct Member {
			AccountUID accountUID;
			u32 userId = 1;
			u32 team = 0;
			u8 slot = 0;
			bool isBot = false;
		};

		const PartyUID UID;
		EntrySystemID entry;
		StageType stageType;
		AreaIndex areaIndex = AreaIndex(0);
		StageIndex stageIndex = StageIndex(0);
		MapIndex mapIndex = MapIndex(0);
		GameType gameType = GameType::PVP_NORMAL;
		GameDefinition gameDefinition = GameDefinition::System;
		u8 selectMasterType = 0;
		bool isQueued = false;

		eastl::fixed_vector<Member,12> memberList;
		eastl::fixed_vector<AccountUID,8> invites;
		struct Recommend { WideString invitee; AccountUID by; };
		eastl::fixed_vector<Recommend,8,false> recommends;

		Party(PartyUID UID_): UID(UID_) {}
	};

	const ClientLocalMapping* plidMap;
	struct LastLeader { WideString nickname; LocalActorID characterID; SkinIndex skin; };
	eastl::fixed_vector<LastLeader,64,true> lastLeaders;
	eastl::array<const Account*,MAX_PLAYERS> playerAccountData;

	WorldHub world;
	HubReplication replication;

	eastl::array<ActorUID,MAX_PLAYERS> playerActorUID;
	eastl::fixed_list<Player,MAX_PLAYERS> playerList;
	eastl::array<decltype(playerList)::iterator,MAX_PLAYERS> playerMap;

	hash_map<AccountUID, ClientHandle, MAX_PLAYERS> accountClientHandleMap;

	// NOTE: we use MAX_PLAYERS as base capacity because each player can create their own party
	eastl::fixed_list<Party,MAX_PLAYERS> partyList;
	hash_map<PartyUID, decltype(partyList)::iterator, MAX_PLAYERS> partyMap;
	eastl::array<EntrySystemID,MAX_PLAYERS> pendingPartyEntry;
	eastl::array<StageType,MAX_PLAYERS> pendingPartyStage;
	struct PraiseCount { SortieUID sortieUID; WideString nickname; u32 count; };
	eastl::fixed_vector<PraiseCount,64,true> praiseCounts;
	eastl::array<GameType,MAX_PLAYERS> pendingPartyGameType;
	eastl::array<GameDefinition,MAX_PLAYERS> pendingPartyDefinition;
	eastl::array<u8,MAX_PLAYERS> pendingPartySelectMaster;
	eastl::array<StageIndex,MAX_PLAYERS> pendingPartyStageIndex;
	eastl::array<bool,MAX_PLAYERS> pendingRegame = {};
	eastl::fixed_vector<SortieUID,64,true> endedSorties;
	struct SortieMember { AccountUID accountUID; u8 row; u32 teamType; bool bot; WideString nick; u8 team = 0; bool stayJoined = false; };
	struct SortieRoster { SortieUID sortieUID; eastl::fixed_vector<SortieMember,16,false> members; EntrySystemID entry; StageType stageType; StageIndex stage; GameType gameType; GameDefinition gameDefinition; Time stayEndAt = Time::ZERO; bool stayDone = false; };
	struct StayInvite { WideString leader; WideString invitee; };
	eastl::fixed_vector<StayInvite,16,false> stayInvites;
	eastl::fixed_vector<SortieRoster,MAX_PLAYERS,false> sortieRosters;
	void BroadcastSortieConfirm(const SortieRoster& roster, u8 row, u32 teamType, bool confirm);

	eastl::fixed_vector<SpawnPoint,128> mapSpawnPoints;

	Time localTime;
	MatchmakerConnector* matchmaker;

	void Init(Server* server_, const ClientLocalMapping* plidMap_);
	void Update(Time localTime_);

	bool JukeboxQueueSong(i32 userID, SongID songID);

	bool LoadMap();

	void OnPlayerConnect(ClientHandle clientHd, const Account* accountData);
	void OnPlayerDisconnect(ClientHandle clientHd);
	void DisconnectUser(i32 userID, ClientHandle clientHd);
	void OnPlayerGetCharacterInfo(ClientHandle clientHd, ActorUID actorUID);
	void OnPlayerUpdatePosition(ClientHandle clientHd, ActorUID characterActorUID, const vec3& pos, const vec3& dir, const vec3& eye, f32 rotate, f32 speed, ActionStateID state, i32 actionID);
	void OnPlayerChatMessage(ClientHandle clientHd, i32 chatType, const wchar* msg, i32 msglen);
	void OnPlayerChatWhisper(ClientHandle clientHd, const wchar* destNick, const wchar* msg);
	void OnPlayerEnterCity(ClientHandle clientHd);
	void OnReturnToCity(ClientHandle clientHd);
	void OnReturnToGame(ClientHandle clientHd);
	void BroadcastPlayerStateInTown(i32 userID, i8 state, const u8* modes, i32 modeCount);
	void OnPraise(ClientHandle clientHd, const wchar* nickname, i32 nicknameLen);
	void OnHonor(ClientHandle clientHd, const WideString& nickname);
	bool GrantItem(i32 userID, i32 docIndex, i32 amount, u32 updateReason, u32 acquisitionReason, eastl::fixed_vector<AccountItem,8,false>* granted = nullptr);
	void OnItemTrade(ClientHandle clientHd, i32 tradeID, i32 count, bool craft = false, u8 craftType = 0);
	void OnItemUse(ClientHandle clientHd, u32 itemUID);
	void OnGoldShopBuyItem(ClientHandle clientHd, i32 itemIndex);
	struct Account* GearAccount(ClientHandle clientHd);
	const wchar* AccountNickname(ClientHandle clientHd);
	eastl::array<u32,MAX_PLAYERS> chosenFriendState = {};
	void OnFriendStateChange(ClientHandle clientHd, u32 state);
	void OnQuickRunArena(ClientHandle clientHd);
	void OnArsenal(ClientHandle clientHd, u16 netID, const u8* packetData, i32 packetSize);
	void OnSelectSupportKit(ClientHandle clientHd, u8 etcType, i32 doc);
	void SendSummaries(ClientHandle clientHd, const wchar* nickname);
	void OnTitleSelect(ClientHandle clientHd, i32 displayTitle, i32 statTitle);
	void OnSelectBadge(ClientHandle clientHd, u8 badgeType);
	void OnSummaryInfoEach(ClientHandle clientHd, i32 summary);
	void OnSummaryReward(ClientHandle clientHd, i32 summary, u8 type, i32 data);
	void OnGearEquip(ClientHandle clientHd, u8 pageNo, u32 itemUID, u32 slot);
	void OnGearUnequip(ClientHandle clientHd, u8 pageNo, u32 slot);
	void OnMasterGearSelect(ClientHandle clientHd, u32 characterID, u8 pageNo);
	void OnMasterGearAdd(ClientHandle clientHd);
	void OnMasterGearRename(ClientHandle clientHd, u8 pageNo, const wchar* name, i32 len);
	void OnActivityReward(ClientHandle clientHd, i32 tier);
	void OnSaveSettings(ClientHandle clientHd, u8 settingType, const u8* data, i32 size);
	void OnInventoryExpand(ClientHandle clientHd, i32 tab);
	void OnShopBuyItem(ClientHandle clientHd, u32 goodsID, i32 quantity, i32 itemIndex, i32 itemCount, bool npShop);
	void OnPlayerSetLeaderCharacter(ClientHandle clientHd, LocalActorID characterID, SkinIndex skinIndex);
	void OnPlayerSyncActionState(ClientHandle clientHd, ActorUID actorUID, ActionStateID state, i32 param1, i32 param2, f32 rotate, f32 upperRotate);
	void OnPlayerJukeboxQueueSong(ClientHandle clientHd, SongID songID);
	void OnPlayerReadyToLoad(ClientHandle clientHd);
	void OnCreateParty(ClientHandle clientHd, EntrySystemID entry, StageType stageType);
	void OnPartyModify(ClientHandle clientHd, StageIndex stage, GameType gameType, GameDefinition gameDefinition, u8 selectMasterType);
	void OnPartySwapTeam(ClientHandle clientHd, u32 originTeam, u8 originSlot, u32 targetTeam, u8 targetSlot);
	void OnPartyAddBot(ClientHandle clientHd);
	void OnPartyRemoveBot(ClientHandle clientHd, u32 userId);
	void OnStartGame(ClientHandle clientHd);
	void OnStayAsTeam(ClientHandle clientHd, bool join);
	void OnGiftFriends(ClientHandle clientHd);
	SortieRoster* FindResultRoster(i32 userID, SortieMember** member);
	void UpdateStayAsTeam();
	void OnRegame(ClientHandle clientHd);
	i32 PartyTeamCapacity(const Party& party, u32 team) const;
	bool PartyPlaceMember(Party& party, Party::Member* member) const;
	const wchar* PartyMemberName(const Party::Member& member) const;
	void SendCustomPartyJoinAnswer(ClientHandle clientHd, const Party& party, u32 myTeam);

	bool SetPartyStage(Party& party, StageIndex stage);
	void OnPartyInvite(ClientHandle clientHd, const wchar* nickname, i32 nicknameLen);
	void OnPartyInviteRecommendResponse(ClientHandle clientHd, const WideString& invitee, bool accept);
	void OnPartyInviteAccept(ClientHandle clientHd, i32 partyID);
	void OnPartyBreakUp(ClientHandle clientHd);
	void OnPartyLeave(ClientHandle clientHd);
	void OnCancelMatching(ClientHandle clientHd);
	void OnGuildPacket(ClientHandle clientHd, u16 netID, const u8* data, i32 size);
	void GuildOnConnect(i32 userID);
	void GuildOnDisconnect(i32 userID);
	void GuildBroadcastState(i32 userID, u8 state);
	bool GuildChat(i32 userID, const wchar* msg, i32 msgLen);
	void GuildLevelCheck(GuildRecord& guild, i32 levelBefore);
	void SendGuildConstraint(const GuildRecord& guild);
	void SendGuildRewardMail(const WideString& nick, const wchar* subject, const wchar* content, i32 item, i32 count);
	void GuildAddMember(GuildRecord& guild, const WideString& nick);
	void GuildRemoveMember(GuildRecord& guild, const WideString& nick, bool kicked);
	u16 GuildLeaderClass(const WideString& nickname);
	void SendGuildResult(ClientHandle clientHd, u16 netID, u32 result);
	void SendGuildNickResult(ClientHandle clientHd, u16 netID, u32 result, const WideString& text);
	void SendMyGuild(ClientHandle clientHd, const GuildRecord* guild, u8 rollcall);
	void SendGuildChannelEnter(ClientHandle clientHd, const GuildRecord& guild, const WideString& nickname, u8 status);
	void SendGuildStatus(ClientHandle clientHd, const WideString* nicks, const u8* states, i32 count);
	void SendGuildPointAndFund(const GuildRecord& guild);
	void SendGuildProfile(ClientHandle clientHd, const GuildRecord* guild);
	void SendGuildMemberList(ClientHandle clientHd, const GuildRecord* guild);
	void SendGuildHistory(ClientHandle clientHd, const GuildRecord* guild);
	void SendGuildPublicProfile(ClientHandle clientHd, const GuildRecord* guild, u16 netID, const WideString* memberNick);
	template<typename Fn>
	void ForEachOnlineGuildMember(const GuildRecord& guild, Fn fn)
	{
		foreach_const(m, guild.members) {
			const i32 memberID = FindUserByNickname(m->nickname);
			if(memberID < 0 || playerMap[memberID] == playerList.end()) continue;
			fn(memberID, playerMap[memberID]->clientHd);
		}
	}
	void OnFriendRequest(ClientHandle clientHd, const WideString& nickname);
	void OnFriendRespond(ClientHandle clientHd, const WideString& nickname, bool accept);
	void OnFriendRemove(ClientHandle clientHd, const WideString& nickname);
	void OnFriendFavorite(ClientHandle clientHd, const WideString& nickname, bool favorite);
	void OnBlock(ClientHandle clientHd, const WideString& nickname, bool block);
	void OnStatusMessage(ClientHandle clientHd, const WideString& message);
	HubReplication::FriendEntry MakeFriendEntry(const WideString& nickname, u16* levelStorage);
	void SendFriendsOnConnect(i32 userID);
	void BroadcastFriendState(i32 userID, u8 state);
	Party* FindPlayerParty(i32 userID);
	i32 FindUserByNickname(const WideString& nickname);
	void LeaveParty(i32 userID, bool notify);
	void OnEnqueueGame(ClientHandle clientHd);
	void OnSortieRoomFound(ClientHandle clientHd, SortieUID sortieID);
	void OnSortieRoomConfirm(ClientHandle clientHd, bool confirm);
	void OnMailShowList(ClientHandle clientHd, u8 mailbox);
	void OnMailRead(ClientHandle clientHd, u64 mailID);
	void OnMailWrite(ClientHandle clientHd, const wchar* to, const wchar* subject, i32 subjectLen, const wchar* content, i32 contentLen);
	void OnMailMove(ClientHandle clientHd, u8 moveType, const u64* ids, i32 count);
	void OnMailGetAttachments(ClientHandle clientHd, const u64* ids, i32 count);
	void OnMissionReward(ClientHandle clientHd, i32 missionID);
	void OnDailyMissionChange(ClientHandle clientHd, i32 step);

	void MmOnPartyCreated(PartyUID partyUID, AccountUID leader);
	void MmOnPartyEnqueued(PartyUID partyUID);
	void MmOnMatchFound(const In::MN_MatchingPartyFound& matchingParty);
	void MmOnGameEnded(SortieUID sortieUID);
	void MmOnRoomCancelled(SortieUID sortieUID);

	bool ParseChatCommand(ClientHandle clientHd, const wchar* msg, const i32 len);
	void SendDbgMsg(ClientHandle clientHd, const wchar* msg);

	void SpawnNPC(CreatureIndex docID, i32 localID, const vec3& pos, const vec3& dir);
};
