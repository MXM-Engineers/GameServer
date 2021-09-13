#include "game.h"
#include "coordinator.h" // account data
#include <mxm/game_content.h>
#include "config.h"
#include <EAStdC/EAString.h>

void HubGame::Init(Server* server_, const ClientLocalMapping* plidMap_)
{
	plidMap = plidMap_;
	replication.Init(server_);
	replication.plidMap = plidMap_;
	world.Init(&replication);

	matchmaker = &Matchmaker();

	playerActorUID.fill(ActorUID::INVALID);

	playerMap.fill(playerList.end());

	LoadMap();
}

void HubGame::Update(Time localTime_)
{
	ProfileFunction();

	localTime = localTime_;
	world.Update(localTime);
	replication.FrameEnd();

	ProcessMatchmakerUpdates();
}

void HubGame::ProcessMatchmakerUpdates()
{
	// FIXME: very inefficient locking
	LOCK_MUTEX(matchmaker->mutexUpdates);

	foreach_const(p, matchmaker->updatePartiesCreated) {
		// TODO: find and error out if not found
		const ClientHandle clientHd = accountClientHandleMap.at(p->leader);
		const i32 userID = plidMap->Get(clientHd);
		playerMap[userID]->partyUID = p->UID;

		// create party
		ASSERT(partyMap.find(p->UID) == partyMap.end());
		partyList.emplace_back(p->UID);
		Party& party = *(--partyList.end());
		party.memberList.push_back(p->leader);

		partyMap.emplace(p->UID, --partyList.end());

		replication.SendPartyCreateSucess(clientHd, UserID(userID + 1), StageType::PLAY_INSTANCE);
	}

	foreach_const(p, matchmaker->updatePartiesEnqueued) {
		// TODO: find and error out if not found
		Party& party = *partyMap.at(p->UID);
		foreach_const(m, party.memberList) {
			// TODO: check if on this hub
			const ClientHandle clientHd = accountClientHandleMap.at(*m);
			replication.SendPartyEnqueue(clientHd);
		}
	}

	matchmaker->updatePartiesCreated.clear();
	matchmaker->updatePartiesEnqueued.clear();
}

bool HubGame::JukeboxQueueSong(i32 userID, SongID songID)
{
	ASSERT(playerAccountData[userID]);
	const ClientHandle clientHd = playerMap[userID]->clientHd;

	WorldHub::ActorJukebox& jukebox = world.jukebox;
	ASSERT(jukebox.UID != ActorUID::INVALID);

	if(jukebox.queue.full()) {
		// TODO: send *actual* packet answer
		SendDbgMsg(clientHd, L"Jukebox queue is full");
		return false;
	}

	// limit reservation to 1 per player
	// @Speed
	// TODO: store the accountID (when we have it)
	const WideString& nick = playerAccountData[userID]->nickname;
	bool found = false;
	if(jukebox.currentSong.requesterNick.compare(nick) == 0) {
		found = true;
	}

	if(!found) {
		foreach(it, jukebox.queue) {
			if(it->requesterNick.compare(nick) == 0) {
				found = true;
				break;
			}
		}
	}

	if(found) {
		SendDbgMsg(clientHd, L"Song reservation is limited to 1 per player");
		return false;
	}

	const GameXmlContent::Song* xmlSong = GetGameXmlContent().FindJukeboxSongByID(songID);
	if(!xmlSong) {
		SendDbgMsg(clientHd, LFMT(L"ERROR: Jukebox song not found (%d)", songID));
		return false;
	}

	WorldHub::ActorJukebox::Song song;
	song.requesterNick = playerAccountData[userID]->nickname;
	song.songID = songID;
	song.lengthInSec = xmlSong->length;
	jukebox.queue.push_back(song);
	return true;
}

bool HubGame::LoadMap()
{
	const GameXmlContent& content = GetGameXmlContent();

	foreach(it, content.mapLobby.creatures) {
		// don't spawn "spawn points"
		if(it->IsSpawnPoint()) {
			mapSpawnPoints.push_back(SpawnPoint{ it->pos, it->rot });
			continue;
		}

		if(it->docID == CreatureIndex::Jukebox) {
			world.SpawnJukeboxActor(CreatureIndex::Jukebox, it->localID, it->pos, it->rot);
		}
		if (it->docID == CreatureIndex::HalloweenJukebox) {
			world.SpawnJukeboxActor(CreatureIndex::HalloweenJukebox, it->localID, it->pos, it->rot);
		}
		else {
			// spawn npc
			SpawnNPC(it->docID, it->localID, it->pos, it->rot);
		}
	}

	return true;
}

void HubGame::OnPlayerConnect(ClientHandle clientHd, const AccountData* accountData)
{
	const i32 userID = plidMap->Get(clientHd);
	playerAccountData[userID] = accountData;
	accountClientHandleMap.emplace(accountData->accountUID, clientHd);

	playerList.push_back(Player(clientHd));
	playerMap[userID] = --playerList.end();

	replication.SendAccountDataLobby(clientHd, *accountData);

	replication.OnPlayerConnect(clientHd);
}

void HubGame::OnPlayerDisconnect(ClientHandle clientHd)
{
	const i32 userID = plidMap->Get(clientHd);
	LOG("[client%x] GameHub :: OnClientDisconnect :: actorUID=%u", clientHd, (u32)playerActorUID[userID]);

	// we can disconnect before spawning, so test if we have an actor associated
	if(playerActorUID[userID] != ActorUID::INVALID) {
		world.DestroyPlayerActor(playerActorUID[userID]);
	}
	playerActorUID[userID] = ActorUID::INVALID;

	if(playerMap[userID] != playerList.end()) {
		playerList.erase(playerMap[userID]);
	}
	playerMap[userID] = playerList.end();

	accountClientHandleMap.erase(playerAccountData[userID]->accountUID);
	playerAccountData[userID] = nullptr;

	replication.OnClientDisconnect(clientHd);
}

void HubGame::OnPlayerGetCharacterInfo(ClientHandle clientHd, ActorUID actorUID)
{
	const i32 userID = plidMap->Get(clientHd);

	// TODO: health
	const WorldHub::ActorPlayer* actor = world.FindPlayerActor(actorUID);
	ASSERT(actor->clientHd == clientHd);
	replication.SendCharacterInfo(clientHd, actor->UID, actor->docID, actor->classType, 100, 100);
}

void HubGame::OnPlayerUpdatePosition(ClientHandle clientHd, ActorUID characterActorUID, const vec3& pos, const vec3& dir, const vec3& eye, f32 rotate, f32 speed, ActionStateID state, i32 actionID)
{
	const i32 userID = plidMap->Get(clientHd);

	// NOTE: the client is not aware that we spawned a new actor for them yet, we ignore this packet
	// LordSk (30/08/2020)
	if(playerActorUID[userID] != characterActorUID) {
		WARN("Client sent an invalid characterID (userID=%d characterID=%d)", userID, (u32)characterActorUID);
		return;
	}

	WorldHub::ActorPlayer* actor = world.FindPlayerActor(playerActorUID[userID]);
	ASSERT(actor);

	// TODO: check for movement hacking
	actor->pos = pos;
	actor->dir = dir;
	actor->eye = eye;
	actor->rotate = rotate;
	actor->speed = speed;
}

void HubGame::OnPlayerChatMessage(ClientHandle clientHd, i32 chatType, const wchar* msg, i32 msgLen)
{
	// command
	if(ParseChatCommand(clientHd, msg, msgLen)) {
		return; // we're done here
	}

	// TODO: chat types
	// TODO: senderStaffType
	// TODO: Actual chat system

	const i32 userID = plidMap->Get(clientHd);
	ASSERT(playerAccountData[userID]);
	replication.SendChatMessageToAll(playerAccountData[userID]->nickname.data(), chatType, msg, msgLen);
}

void HubGame::OnPlayerChatWhisper(ClientHandle clientHd, const wchar* destNick, const wchar* msg)
{
	const i32 userID = plidMap->Get(clientHd);

	ASSERT(playerAccountData[userID]);
	replication.SendChatWhisperConfirmToClient(clientHd, destNick, msg); // TODO: send a fail when the client is not found

	i32 destClientID = -1;
	for(int i = 0; i < playerAccountData.size(); i++) {
		if(playerAccountData[i]) {
			if(playerAccountData[i]->nickname.compare(destNick) == 0) {
				destClientID = i;
				break;
			}
		}
	}

	if(destClientID == -1) {
		SendDbgMsg(clientHd, LFMT(L"Player '%s' not found", destNick));
		return;
	}

	replication.SendChatWhisperToClient(playerMap[destClientID]->clientHd, playerAccountData[userID]->nickname.data(), msg);
}

void HubGame::OnPlayerSetLeaderCharacter(ClientHandle clientHd, LocalActorID characterID, SkinIndex skinIndex)
{
	const i32 userID = plidMap->Get(clientHd);

	const i32 leaderMasterContentID = (u32)characterID - (u32)LocalActorID::FIRST_SELF_MASTER - 1;

	// select a spawn point at random
	const SpawnPoint& spawnPoint = mapSpawnPoints[RandUint() % mapSpawnPoints.size()];
	vec3 pos = spawnPoint.pos;
	vec3 dir = spawnPoint.dir;
	vec3 eye(0, 0, 0);

	// TODO: check if already leader character
	if((playerActorUID[userID] != ActorUID::INVALID)) {
		WorldHub::ActorCore* actor = world.FindPlayerActor(playerActorUID[userID]);
		ASSERT(actor);

		pos = actor->pos;
		dir = actor->dir;
		eye = actor->eye;

		world.DestroyPlayerActor(playerActorUID[userID]);
	}

	ASSERT(playerAccountData[userID]); // account data is not assigned
	const AccountData* account = playerAccountData[userID];

	// TODO: tie in account->leaderMasterID,skinIndex with class and model
	const ClassType classType = GetGameXmlContent().masters[leaderMasterContentID].classType;
	ASSERT((i32)classType == leaderMasterContentID+1);

	WorldHub::ActorPlayer& actor = world.SpawnPlayerActor(userID, classType, skinIndex, account->nickname.data(), account->guildTag.data());
	actor.pos = pos;
	actor.dir = dir;
	actor.eye = eye;
	actor.clientHd = clientHd; // TODO: this is not useful right now
	playerActorUID[userID] = actor.UID;

	replication.SendPlayerSetLeaderMaster(clientHd, playerActorUID[userID], classType, skinIndex);
}

void HubGame::OnPlayerSyncActionState(ClientHandle clientHd, ActorUID actorUID, ActionStateID state, i32 param1, i32 param2, f32 rotate, f32 upperRotate)
{
	const i32 userID = plidMap->Get(clientHd);

	// FIXME: remove
	// connect to gameserver when jumping
	if(state == ActionStateID::JUMP_START_MOVESTATE) {
		const u8 ip[4] = { 127, 0, 0, 1 };
		replication.SendConnectToServer(clientHd, *playerAccountData[userID], ip, 12900);
	}

	WorldHub::ActorPlayer* actor = world.FindPlayerActor(playerActorUID[userID]);
	ASSERT(actor);

	// TODO: check hacking
	actor->rotate = rotate;
	actor->upperRotate = upperRotate;
	actor->actionState = state;
	actor->actionParam1 = param1;
	actor->actionParam2 = param2;
}

void HubGame::OnPlayerJukeboxQueueSong(ClientHandle clientHd, SongID songID)
{
	const i32 userID = plidMap->Get(clientHd);

	JukeboxQueueSong(userID, songID);
}

void HubGame::OnPlayerReadyToLoad(ClientHandle clientHd)
{
	replication.SendLoadLobby(clientHd, StageIndex::LOBBY_NORMAL);
}

void HubGame::OnCreateParty(ClientHandle clientHd, EntrySystemID entry, StageType stageType)
{
	const i32 userID = plidMap->Get(clientHd);

	// TODO: validate args
	matchmaker->QueryPartyCreate(playerAccountData[userID]->accountUID);
}

void HubGame::OnEnqueueGame(ClientHandle clientHd)
{
	const i32 userID = plidMap->Get(clientHd);

	// TODO: validate args
	matchmaker->QueryPartyEnqueue(playerMap[userID]->partyUID);
}

bool HubGame::ParseChatCommand(ClientHandle clientHd, const wchar* msg, const i32 len)
{
	if(!Config().DevMode) return false; // don't allow command when dev mode is not enabled

	const i32 userID = plidMap->Get(clientHd);

	static ActorUID lastLegoActorUID = ActorUID::INVALID;

	if(msg[0] == L'!') {
		msg++;

		if(EA::StdC::Strncmp(msg, L"lego", 4) == 0) {
			WorldHub::ActorCore* playerActor = world.FindPlayerActor(playerActorUID[userID]);
			ASSERT(playerActor);

			WorldHub::ActorCore& actor = world.SpawnPlayerActor(-1, (ClassType)18, SkinIndex::DEFAULT, L"legomage15", L"MEME");
			actor.pos = playerActor->pos;
			actor.dir = playerActor->dir;
			actor.eye = playerActor->eye;

			// trigger second emote
			actor.actionState = ActionStateID::EMOTION_BEHAVIORSTATE;
			actor.actionParam1 = 2;

			lastLegoActorUID = actor.UID;

			SendDbgMsg(clientHd, LFMT(L"Actor spawned at (%g, %g, %g)", actor.pos.x, actor.pos.y, actor.pos.z));
			return true;
		}

		if(EA::StdC::Strncmp(msg, L"delete", 6) == 0) {
			world.DestroyPlayerActor(lastLegoActorUID);

			SendDbgMsg(clientHd, LFMT(L"Actor destroyed (%u)", lastLegoActorUID));
			return true;
		}

		if(EA::StdC::Strncmp(msg, L"rozark", 6) == 0) {
			WorldHub::ActorCore* playerActor = world.FindPlayerActor(playerActorUID[userID]);
			ASSERT(playerActor);

			WorldHub::ActorCore& actor = world.SpawnPlayerActor(-1, (ClassType)5001200, SkinIndex::DEFAULT, L"rozark", L"MEME");
			actor.pos = playerActor->pos;
			actor.dir = playerActor->dir;
			actor.eye = playerActor->eye;
			lastLegoActorUID = actor.UID;

			SendDbgMsg(clientHd, LFMT(L"Actor spawned at (%g, %g, %g)", actor.pos.x, actor.pos.y, actor.pos.z));
			return true;
		}

		if(EA::StdC::Strncmp(msg, L"tanian", 6) == 0) {
			WorldHub::ActorCore* playerActor = world.FindPlayerActor(playerActorUID[userID]);
			ASSERT(playerActor);

			WorldHub::ActorCore& actor = world.SpawnPlayerActor(-1, (ClassType)5001040, SkinIndex::DEFAULT, L"rozark", L"MEME");
			actor.pos = playerActor->pos;
			actor.dir = playerActor->dir;
			actor.eye = playerActor->eye;
			lastLegoActorUID = actor.UID;

			SendDbgMsg(clientHd, LFMT(L"Actor spawned at (%g, %g, %g)", actor.pos.x, actor.pos.y, actor.pos.z));
			return true;
		}

		if(EA::StdC::Strncmp(msg, L"fish", 4) == 0) {
			WorldHub::ActorCore* playerActor = world.FindPlayerActor(playerActorUID[userID]);
			ASSERT(playerActor);

			WorldHub::ActorCore& actor = world.SpawnPlayerActor(-1, (ClassType)5000800, SkinIndex::DEFAULT, L"rozark", L"MEME");
			actor.pos = playerActor->pos;
			actor.dir = playerActor->dir;
			actor.eye = playerActor->eye;
			lastLegoActorUID = actor.UID;

			SendDbgMsg(clientHd, LFMT(L"Actor spawned at (%g, %g, %g)", actor.pos.x, actor.pos.y, actor.pos.z));
			return true;
		}

		if(EA::StdC::Strncmp(msg, L"smoke", 5) == 0) {
			WorldHub::ActorPlayer* playerActor = world.FindPlayerActor(playerActorUID[userID]);
			ASSERT(playerActor);

			SendDbgMsg(clientHd, LFMT(L"All you have to do was follow the damn train %s :(", playerActor->name.data()));
			return true;
		}

		if(EA::StdC::Strncmp(msg, L"upsidedown", 10) == 0) {
			JukeboxQueueSong(userID, SongID::UpsideDown);
			return true;
		}
		
		if(EA::StdC::Strncmp(msg, L"scml", 4) == 0) {
			JukeboxQueueSong(userID, SongID::Scml);
			return true;
		}

		if(EA::StdC::Strncmp(msg, L"poharan", 7) == 0) {
			JukeboxQueueSong(userID, SongID::Poharan);
			return true;
		}

		if(EA::StdC::Strncmp(msg, L"triangle", 8) == 0) {
			JukeboxQueueSong(userID, SongID::Triangle);
			return true;
		}
		
		if(EA::StdC::Strncmp(msg, L"arami", 5) == 0) {
			JukeboxQueueSong(userID, SongID::Arami);
			return true;
		}

	}

	return false;
}

void HubGame::SendDbgMsg(ClientHandle clientHd, const wchar* msg)
{
	replication.SendChatMessageToClient(clientHd, L"System", 1, msg);
}

void HubGame::SpawnNPC(CreatureIndex docID, i32 localID, const vec3& pos, const vec3& dir)
{
	WorldHub::ActorCore& actor = world.SpawnNpcActor(docID, localID);
	actor.pos = pos;
	actor.dir = dir;
}
