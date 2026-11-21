#pragma once
#include <common/network.h>
#include <common/vector_math.h>
#include <common/utils.h>
#include <common/protocol.h>
#include <common/inner_protocol.h>
#include <common/packet_serialize.h>
#include <EASTL/array.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/fixed_set.h>
#include <EASTL/fixed_map.h>
#include <EASTL/fixed_list.h>
#include <mxm/core.h>

struct Account;

struct MatchingParty
{
	struct Player
	{
		WideString& name;
		u8 userID;
		u8 team;
		u8 isBot;
		// TODO: more useful stuff
	};

	SortieUID sortieUID;
	eastl::fixed_vector<Player,16,false> players;
};

struct HubReplication
{
	enum class ActorType: i32
	{
		INVALID = 0,
		PLAYER = 1,
		NPC = 2,
		JUKEBOX = 3,
	};

	template<ActorType TYPE_>
	struct Actor
	{
		ActorUID actorUID;
		CreatureIndex docID;
		vec3 pos;
		vec3 dir;

		inline ActorType Type() const { return TYPE_; }
	};

	struct ActorPlayer: Actor<ActorType::PLAYER>
	{
		ActorUID parentActorUID;
		vec3 eye;
		f32 rotate;
		f32 upperRotate;
		f32 speed;
		ClassType classType;
		SkinIndex skinIndex;

		WideString name;
		WideString guildTag;
		i32 weaponID;
		i32 additionnalOverHeatGauge;
		i32 additionnalOverHeatGaugeRatio;
		u8 playerStateInTown;
		u16 level = 1;
		i32 displayTitle = -1;
		u8 badgeType = 0;
		u8 badgeTier = 0;
		i32 statTitle = -1;
		u32 levelExp = 0;
		eastl::fixed_vector<u8,16> matchingGameModes;

		ActionStateID actionState;
		i32 spawnAnim = 0;
		i32 ownerID = 0;
		u8 dirToNearPC = 0;
		i32 wanderDist = -1;
		i32 tagID = -1;
		i32 faction = 0;
		i32 seed = 0;

		i32 actionParam1;
		i32 actionParam2;
	};

	struct ActorNpc: Actor<ActorType::NPC>
	{
		i32 type;
		i32 localID;
		i32 faction;
		i32 spawnAnim = 0;
		i32 ownerID = 0;
		u8 dirToNearPC = 0;
		i32 wanderDist = -1;
		i32 tagID = -1;
		ActionStateID actionState = ActionStateID::INVALID;
		i32 seed = 0;

	};

	struct ActorJukebox: Actor<ActorType::JUKEBOX>
	{
		struct Track
		{
			SongID songID;
			WideString requesterNick;
		};

		i32 localID;
		i32 spawnAnim = 0;
		i32 ownerID = 0;
		u8 dirToNearPC = 0;
		i32 wanderDist = -1;
		i32 tagID = -1;
		i32 faction = -1;
		ActionStateID actionState = ActionStateID::INVALID;
		i32 seed = 0;
		i32 type = 1;



		i32 playPosition;
		Time playStartTime; // identifier
		Track currentSong;
		eastl::fixed_vector<Track,JUKEBOX_MAX_TRACKS,false> tracks;

		ActorJukebox() {
			actorUID = ActorUID::INVALID;
		}
	};

	struct Party
	{
		u32 UID;
		ClientHandle leader;
		EntrySystemID entry;
		StageType stageType;
		// TODO: do fancy party stuff later on
	};

	struct Frame
	{
		struct Transform
		{
			vec3 pos;
			vec3 dir;
			vec3 eye;
			f32 rotate;
			f32 speed;

			bool HasNotChanged(const Transform& other) const;
		};

		struct ActionState
		{
			ActionStateID actionState;
			i32 actionParam1;
			i32 actionParam2;
			f32 rotate;
			f32 upperRotate;

			bool HasNotChanged(const ActionState& other) const;
		};

		// TODO: replace fixed_map with fixed_hash_map
		eastl::fixed_list<ActorPlayer,2048,true> playerList;
		eastl::fixed_list<ActorNpc,2048,true> npcList;
		eastl::fixed_map<ActorUID,decltype(playerList)::iterator,2048,true> playerMap;
		eastl::fixed_map<ActorUID,decltype(npcList)::iterator,2048,true> npcMap;

		eastl::fixed_set<ActorUID,2048> actorUIDSet;
		eastl::fixed_map<ActorUID,ActorType,2048,true> actorType;

		eastl::fixed_map<ActorUID,Transform,2048> transformMap;
		eastl::fixed_map<ActorUID,ActionState,2048> actionStateMap;

		ActorJukebox jukebox;

		void Clear();
	};

	enum class PlayerState: u8 {
		DISCONNECTED=0,
		CONNECTED=1,
		IN_GAME=2,
	};

	struct PlayerLocalInfo
	{
		eastl::fixed_map<ActorUID,LocalActorID,2048> localActorIDMap;
		eastl::fixed_set<ActorUID,2048> actorUIDSet;
		LocalActorID nextPlayerLocalActorID;
		LocalActorID nextNpcLocalActorID;
		LocalActorID nextMonsterLocalActorID;
		bool isFirstLoad;
		bool selfExpPending;

		void Reset();
	};

	Server* server;
	Frame frames[2];
	Frame* frameCur;
	Frame* framePrev;

	// TODO: we propably do not need to store every possible client data here
	// Use a fixed_vector?

	const ClientLocalMapping* plidMap;

	eastl::array<ClientHandle,MAX_CLIENTS> playerClientHd;
	eastl::array<const Account*,MAX_CLIENTS> playerAccount;
	eastl::array<PlayerState,MAX_CLIENTS> playerState;
	eastl::array<PlayerLocalInfo,MAX_CLIENTS> playerLocalInfo;

	void Init(Server* server_);

	void FrameEnd();
	void FramePushPlayerActor(const ActorPlayer& actor);
	void FramePushNpcActor(const ActorNpc& actor);
	void FramePushJukebox(const ActorJukebox& actor);

	void OnPlayerConnect(ClientHandle clientHd);
	void SetPlayerAccount(ClientHandle clientHd, const Account* account);
	void SendUpdateEntrySystem(ClientHandle clientHd);
	void SendTownBurst(ClientHandle clientHd);
	void SendMasterRotation(ClientHandle clientHd);
	void SendPlayerStateInTown(ClientHandle clientHd, ActorUID actorUID, i8 state, const u8* modes, i32 modeCount);
	void SendSelfExp(ClientHandle clientHd);
	void SendLoadLobby(ClientHandle clientHd, MapIndex stageIndex, bool joinCity);
	void SendJoinCity(ClientHandle clientHd);
	void SendResultScreenStart(ClientHandle clientHd, u32 stayVoteMs);
	void SendUserReturnToCity(ClientHandle clientHd, const wchar* nickname);
	void SendReturnToCity(ClientHandle clientHd);
	void SendPraiseResult(ClientHandle clientHd, u32 errorType);
	void SendHonorResult(ClientHandle clientHd, u32 result);
	void SendCCoin(ClientHandle clientHd, i64 amount, u32 reason);
	void SendInventoryExpand(ClientHandle clientHd, u32 result, i32 tab, i32 lineCount);
	void SendShopBuyResult(ClientHandle clientHd, bool npShop, u32 result, u32 goodsID, u32 quantity, i32 itemIndex);
	void SendItemUpdate(ClientHandle clientHd, const struct AccountItem& item, u32 reason);
	void SendItemsUpdate(ClientHandle clientHd, const struct AccountItem* items, i32 count, u32 reason);
	void SendFriendStateChangeResult(ClientHandle clientHd, u32 result);
	void SendAchievements(ClientHandle clientHd, const struct AccountAchievements& acc);
	void SendPveLastGameRanking(ClientHandle clientHd, const wchar* nickname);
	void SendTitleSelect(ClientHandle clientHd, u32 result, ActorUID actorUID, i32 displayTitle, i32 statTitle);
	void SendQuickRunArena(ClientHandle clientHd, const StageIndex* stages, i32 count, GameType gameType, GameDefinition gameDefinition);
	void SendProfileMasterGears(ClientHandle clientHd, const wchar* nickname);
	void SendGearEquip(ClientHandle clientHd, u32 result, u8 pageNo, u32 itemUID, u32 slot);
	void SendGearUnequip(ClientHandle clientHd, u32 result, u8 pageNo, u32 slot);
	void SendMasterGearSelect(ClientHandle clientHd, u32 result, u32 characterID, u8 pageNo);
	void SendMasterGearRename(ClientHandle clientHd, u32 result, u8 pageNo, const wchar* name);
	void SendPveRanking(ClientHandle clientHd, u32 stage);
	void SendSummaryInfoEach(ClientHandle clientHd, u32 summary);
	void SendStartGame(ClientHandle clientHd, u32 result);
	void SendItemTradeResult(ClientHandle clientHd, u32 result, i64 gold, const struct AccountItem* items, i32 count);
	void SendItemCraftResult(ClientHandle clientHd, u8 craftType, u32 result, const struct AccountItem* items, i32 count);
	void SendGoldShopBuyResult(ClientHandle clientHd, u32 result, i32 itemIndex);
	void SendItemUseResult(ClientHandle clientHd, u32 result, i32 usedDoc, i64 gold, const struct AccountItem* items, i32 count);
	void SendItemAcquisition(ClientHandle clientHd, u32 reason, const struct AccountItem& item, i32 amount);
	void SendActivityReward(ClientHandle clientHd, u32 result, u8 rewardedState);
	void SendPraise(ClientHandle clientHd, const wchar* nickname, u32 count);
	void SetPlayerAsInGame(ClientHandle clientHd);
	void SendCharacterInfo(ClientHandle clientHd, ActorUID actorUID, CreatureIndex docID, ClassType classType, i32 health, i32 healthMax);
	void SendPlayerSetLeaderMaster(ClientHandle clientHd, ActorUID masterActorUID, ClassType classType, SkinIndex skinIndex);

	void SendChatMessageToAll(const wchar* senderName, i32 chatType, const wchar* msg, i32 msgLen);
	void SendChatMessageToClient(ClientHandle toClientID, const wchar* senderName, i32 chatType, const wchar* msg, i32 msgLen = -1);
	void SendChatWhisperConfirmToClient(ClientHandle senderClientHd, const wchar* destNick, const wchar* msg, ErrorType retval);
	void SendChatWhisperToClient(ClientHandle destClientHd, const wchar* destNick, const wchar* msg);

	void SendAccountDataLobby(ClientHandle clientHd, const Account& account);
	void SendMissionList(ClientHandle clientHd, const Account& account);
	void SendMissionReward(ClientHandle clientHd, u32 result, const struct ActiveMission& mission);
	void SendDailyMissionChange(ClientHandle clientHd, u32 result, i32 step, const struct ActiveMission& mission);
	void SendMoney(ClientHandle clientHd, i64 gold, i32 reason);
	void SendExpGain(ClientHandle clientHd, ActorUID actorUID, const wchar* name, bool levelUp, u16 level, u32 levelExp, u32 gained);
	void SendMailUnreadNotice(ClientHandle clientHd, const wchar* owner);
	void SendMailList(ClientHandle clientHd, const wchar* owner, u8 mailbox);
	void SendMailRead(ClientHandle clientHd, const struct MailMessage& mail);
	void SendMailAttachmentResult(ClientHandle clientHd, u32 result, const struct MailMessage& mail);
	void SendMailNewInbox(ClientHandle clientHd, const struct MailMessage& mail);
	void SendMailWriteResult(ClientHandle clientHd, ErrorType result);
	void SendMailMoveResult(ClientHandle clientHd, ErrorType result);

	void SendGameReady(ClientHandle clientHd);

	void SendCalendar(ClientHandle clientHd);
	void SendCalendarSchedule(ClientHandle clientHd);
	void SendProfileCharactersWeapons(ClientHandle clientHd, const Account* account = nullptr);
	void SendProfileSkillsSkins(ClientHandle clientHd, const Account* account = nullptr);
	void SendAreaPopularity(ClientHandle clientHd, AreaIndex areaID);
	void SendPartyCreateSucess(ClientHandle clientHd, UserID ownerUserID, StageType stageType);

	void SendPartyEnqueue(ClientHandle clientHd, StageIndex stageIndex, bool leader);
	void SendPartyInvite(ClientHandle clientHd, const wchar* inviterNick, StageIndex stageIndex, GameType gameType, GameDefinition gameDefinition, i32 partyID);
	void SendPartyInviteResult(ClientHandle clientHd, u32 result, const wchar* nickname);
	struct PartyUser { u32 userId; const wchar* nickname; };
	void SendPartyJoinAnswer(ClientHandle clientHd, i32 partyID, StageType stageType, GameType gameType, const PartyUser* members, i32 count);
	void SendPartyJoin(ClientHandle clientHd, u32 userId, const wchar* nickname, CreatureIndex leaderCreature, bool receiverIsOwner, GameType gameType, GameDefinition gameDefinition, u32 team, u8 slot, bool isBot);
	struct PartySlot { u32 userId; const wchar* nickname; u8 isBot; };
	void SendCustomPartyJoinAnswer(ClientHandle clientHd, i32 partyID, StageType stageType, GameType gameType, GameDefinition gameDefinition, const PartySlot* allies, i32 alliesCount, u32 alliesTeam, const PartySlot* enemies, i32 enemiesCount, u32 enemiesTeam, const PartySlot* spectators, i32 spectatorCount, bool isSpectator);
	void SendPartySwapTeamAnswer(ClientHandle clientHd, u32 result);
	void SendPartySwapTeam(ClientHandle clientHd, u32 originTeam, u8 originSlot, u32 targetTeam, u8 targetSlot);
	void SendPartyAddBotAnswer(ClientHandle clientHd, u32 result);
	void SendPartyRemoveBotAnswer(ClientHandle clientHd, u32 result);
	void SendPartyLeave(ClientHandle clientHd, u32 userId, const wchar* nickname, bool receiverIsOwner);
	void SendPartyBreakupAnswer(ClientHandle clientHd, i32 remainMemberCount);
	void SendPartyBreakup(ClientHandle clientHd);
	void SendPartyLeaveAnswer(ClientHandle clientHd);
	void SendPartyCancelInvite(ClientHandle clientHd, i32 partyID);
	void SendCancelMatchingQueue(ClientHandle clientHd);
	struct FriendEntry { const wchar* nickname; i32 leaderCreature; u8 state; u16 level; u8 favorite; };
	void SendNicknameResult(ClientHandle clientHd, u16 netID, u32 result, const wchar* nickname);
	void SendBlockList(ClientHandle clientHd, const WideString* names, i32 count);
	void SendCommunityStatusMessage(ClientHandle clientHd, const wchar* nickname, const wchar* message);
	void SendCommunityStatusMessageResult(ClientHandle clientHd, u32 result);
	void SendFriendPartyCreation(ClientHandle clientHd, const wchar* nickname, StageIndex stageIndex, GameType gameType);
	void SendFriendList(ClientHandle clientHd, const FriendEntry* friends, i32 count);
	void SendFriendRequestList(ClientHandle clientHd, const FriendEntry* requests, i32 count);
	void SendFriendStateChanged(ClientHandle clientHd, const wchar* nickname, u8 state);
	void SendFriendRequestResult(ClientHandle clientHd, u32 result, const wchar* nickname);
	void SendFriendRequested(ClientHandle clientHd, const wchar* nickname, i32 leaderCreature);
	void SendFriendRespondResult(ClientHandle clientHd, u32 result, const wchar* nickname, u8 accept);
	void SendFriendAdded(ClientHandle clientHd, const FriendEntry& entry);
	void SendFriendRemoveResult(ClientHandle clientHd, u32 result, const wchar* nickname);
	void SendFriendRemoved(ClientHandle clientHd, const wchar* nickname);
	void SendMatchingPartyConfirm(ClientHandle clientHd, u8 row, u32 teamType, bool confirm);
	void SendMatchingPartyFound(ClientHandle clientHd, const In::MN_MatchingPartyFound& matchingParty, StageIndex stageIndex, GameDefinition gameDefinition, const eastl::fixed_vector<UserID,16>& rowIDs, u8 myTeam);

	void OnClientDisconnect(ClientHandle clientHd);

	void PlayerRegisterMasterActor(ClientHandle clientHd, ActorUID masterActorUID, ClassType classType); // TODO: temp, find a better solution

	LocalActorID GetLocalActorID(ClientHandle clientHd, ActorUID actorUID); // Can return INVALID
	ActorUID GetWorldActorUID(ClientHandle clientHd, LocalActorID localActorID); // Can return INVALID

	template<typename Packet, u32 CAPACITY>
	inline void SendWriter(ClientHandle clientHd, const PacketWriter<Packet,CAPACITY>& writer) { SendPacket(clientHd, writer); }

private:
	void PlayerForceLocalActorID(ClientHandle clientHd, ActorUID actorUID, LocalActorID localActorID);

	void UpdatePlayersLocalState();
	void FrameDifference();

	void SendActorPlayerSpawn(ClientHandle clientHd, const ActorPlayer& actor);
	void SendActorNpcSpawn(ClientHandle clientHd, const ActorNpc& actor);
	void SendJukeboxSpawn(ClientHandle clientHd, const ActorJukebox& actor);
	void SendActorDestroy(ClientHandle clientHd, ActorUID actorUID);
	void SendJukeboxPlay(ClientHandle clientHd, SongID songID, const wchar* requesterNick, i32 playPosInSec);
	void SendJukeboxQueue(ClientHandle clientHd, const ActorJukebox::Track* tracks, const i32 trackCount);

	void SendMasterSkillSlots(ClientHandle clientHd, const ActorPlayer& actor);

	void SendInitialFrame(ClientHandle clientHd);

	void CreateLocalActorID(ClientHandle clientHd, ActorUID actorUID);
	void DeleteLocalActorID(ClientHandle clientHd, ActorUID actorUID);

	template<typename Packet>
	inline void SendPacket(ClientHandle clientHd, const Packet& packet)
	{
		SendPacketData<Packet>(clientHd, sizeof(packet), &packet);
	}

	template<typename Packet, u32 CAPACITY>
	inline void SendPacket(ClientHandle clientHd, const PacketWriter<Packet,CAPACITY>& writer)
	{
		SendPacketData<Packet>(clientHd, writer.size, writer.data);
	}

	template<typename Packet>
	inline void SendPacketData(ClientHandle clientHd, u16 packetSize, const void* packetData)
	{
		NT_LOG("[client%x] Replication :: %s", clientHd, PacketSerialize<Packet>(packetData, packetSize));
		server->SendPacketData(clientHd, Packet::NET_ID, packetSize, packetData);
	}
};
