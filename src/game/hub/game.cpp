#include "game.h"
#include <game/coordinator.h> // account data
#include <game/game_content.h>
#include <game/config.h>
#include <EAStdC/EAString.h>


void GameHub::Init(ReplicationHub* replication_)
{
	replication = replication_;
	replication->stageType = StageType::CITY;

	memset(&playerActorUID, 0, sizeof(playerActorUID));

	world.Init(replication);

	foreach(it, playerClientIDMap) {
		*it = playerList.end();
	}

	LoadMap();
}

void GameHub::Update(Time localTime_)
{
	ProfileFunction();

	localTime = localTime_;
	world.Update(localTime);
}

bool GameHub::JukeboxQueueSong(i32 clientID, SongID songID)
{
	ASSERT(playerAccountData[clientID]);

	WorldHub::ActorJukebox& jukebox = world.jukebox;
	ASSERT(jukebox.UID != ActorUID::INVALID);

	if(jukebox.queue.full()) {
		// TODO: send *actual* packet answer
		SendDbgMsg(clientID, L"Jukebox queue is full");
		return false;
	}

	// limit reservation to 1 per player
	// @Speed
	// TODO: store the accountID (when we have it)
	const WideString& nick = playerAccountData[clientID]->nickname;
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
		SendDbgMsg(clientID, L"Song reservation is limited to 1 per player");
		return false;
	}

	const GameXmlContent::Song* xmlSong = GetGameXmlContent().FindJukeboxSongByID(songID);
	if(!xmlSong) {
		SendDbgMsg(clientID, LFMT(L"ERROR: Jukebox song not found (%d)", songID));
		return false;
	}

	WorldHub::ActorJukebox::Song song;
	song.requesterNick = playerAccountData[clientID]->nickname;
	song.songID = songID;
	song.lengthInSec = xmlSong->length;
	jukebox.queue.push_back(song);
	return true;
}

bool GameHub::LoadMap()
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

void GameHub::OnPlayerConnect(i32 clientID, const AccountData* accountData)
{
	playerAccountData[clientID] = accountData;

	playerList.push_back(Player(clientID));
	playerClientIDMap[clientID] = --playerList.end();

	replication->SendAccountDataLobby(clientID, *accountData);
}

void GameHub::OnPlayerDisconnect(i32 clientID)
{
	LOG("[client%03d] GameHub :: OnClientDisconnect :: actorUID=%u", clientID, (u32)playerActorUID[clientID]);

	// we can disconnect before spawning, so test if we have an actor associated
	if(playerActorUID[clientID] != ActorUID::INVALID) {
		world.DestroyPlayerActor(playerActorUID[clientID]);
	}
	playerActorUID[clientID] = ActorUID::INVALID;

	if(playerClientIDMap[clientID] != playerList.end()) {
		playerList.erase(playerClientIDMap[clientID]);
	}
	playerClientIDMap[clientID] = playerList.end();

	playerAccountData[clientID] = nullptr;
}

void GameHub::OnPlayerGetCharacterInfo(i32 clientID, ActorUID actorUID)
{
	// TODO: health
	const WorldHub::ActorPlayer* actor = world.FindPlayerActor(actorUID);
	ASSERT(actor->clientID == clientID);
	replication->SendCharacterInfo(clientID, actor->UID, actor->docID, actor->classType, 100, 100);
}

void GameHub::OnPlayerUpdatePosition(i32 clientID, ActorUID characterActorUID, const vec3& pos, const vec3& dir, const vec3& eye, f32 rotate, f32 speed, ActionStateID state, i32 actionID)
{
	// NOTE: the client is not aware that we spawned a new actor for them yet, we ignore this packet
	// LordSk (30/08/2020)
	if(playerActorUID[clientID] != characterActorUID) {
		WARN("Client sent an invalid characterID (clientID=%d characterID=%d)", clientID, (u32)characterActorUID);
		return;
	}

	WorldHub::ActorPlayer* actor = world.FindPlayerActor(playerActorUID[clientID]);
	ASSERT(actor);

	// TODO: check for movement hacking
	actor->pos = pos;
	actor->dir = dir;
	actor->eye = eye;
	actor->rotate = rotate;
	actor->speed = speed;
}

void GameHub::OnPlayerChatMessage(i32 clientID, i32 chatType, const wchar* msg, i32 msgLen)
{
	// command
	if(ParseChatCommand(clientID, msg, msgLen)) {
		return; // we're done here
	}

	// TODO: chat types
	// TODO: senderStaffType
	// TODO: Actual chat system

	ASSERT(playerAccountData[clientID]);
	replication->SendChatMessageToAll(playerAccountData[clientID]->nickname.data(), chatType, msg, msgLen);
}

void GameHub::OnPlayerChatWhisper(i32 clientID, const wchar* destNick, const wchar* msg)
{
	ASSERT(playerAccountData[clientID]);
	replication->SendChatWhisperConfirmToClient(clientID, destNick, msg); // TODO: send a fail when the client is not found

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
		SendDbgMsg(clientID, LFMT(L"Player '%s' not found", destNick));
		return;
	}

	replication->SendChatWhisperToClient(destClientID, playerAccountData[clientID]->nickname.data(), msg);
}

void GameHub::OnPlayerSetLeaderCharacter(i32 clientID, LocalActorID characterID, SkinIndex skinIndex)
{
	const i32 leaderMasterContentID = (u32)characterID - (u32)LocalActorID::FIRST_SELF_MASTER - 1;

	// select a spawn point at random
	const SpawnPoint& spawnPoint = mapSpawnPoints[RandUint() % mapSpawnPoints.size()];
	vec3 pos = spawnPoint.pos;
	vec3 dir = spawnPoint.dir;
	vec3 eye(0, 0, 0);

	// TODO: check if already leader character
	if((playerActorUID[clientID] != ActorUID::INVALID)) {
		WorldHub::ActorCore* actor = world.FindPlayerActor(playerActorUID[clientID]);
		ASSERT(actor);

		pos = actor->pos;
		dir = actor->dir;
		eye = actor->eye;

		world.DestroyPlayerActor(playerActorUID[clientID]);
	}

	ASSERT(playerAccountData[clientID]); // account data is not assigned
	const AccountData* account = playerAccountData[clientID];

	// TODO: tie in account->leaderMasterID,skinIndex with class and model
	const ClassType classType = GetGameXmlContent().masters[leaderMasterContentID].classType;
	ASSERT((i32)classType == leaderMasterContentID+1);

	WorldHub::ActorPlayer& actor = world.SpawnPlayerActor(clientID, classType, skinIndex, account->nickname.data(), account->guildTag.data());
	actor.pos = pos;
	actor.dir = dir;
	actor.eye = eye;
	actor.clientID = clientID; // TODO: this is not useful right now
	playerActorUID[clientID] = actor.UID;

	replication->SendPlayerSetLeaderMaster(clientID, playerActorUID[clientID], classType, skinIndex);
}

void GameHub::OnPlayerSyncActionState(i32 clientID, ActorUID actorUID, ActionStateID state, i32 param1, i32 param2, f32 rotate, f32 upperRotate)
{
	// FIXME: remove
	// connect to gameserver when jumping
	if(state == ActionStateID::JUMP_START_MOVESTATE) {
		const u8 ip[4] = { 127, 0, 0, 1 };
		replication->SendConnectToServer(clientID, *playerAccountData[clientID], ip, 12900);
	}

	WorldHub::ActorPlayer* actor = world.FindPlayerActor(playerActorUID[clientID]);
	ASSERT(actor);

	// TODO: check hacking
	actor->rotate = rotate;
	actor->upperRotate = upperRotate;
	actor->actionState = state;
	actor->actionParam1 = param1;
	actor->actionParam2 = param2;
}

void GameHub::OnPlayerJukeboxQueueSong(i32 clientID, SongID songID)
{
	JukeboxQueueSong(clientID, songID);
}

void GameHub::OnPlayerReadyToLoad(i32 clientID)
{
	replication->SendLoadLobby(clientID, StageIndex::LOBBY_NORMAL);
}

bool GameHub::ParseChatCommand(i32 clientID, const wchar* msg, const i32 len)
{
	if(!Config().DevMode) return false; // don't allow command when dev mode is not enabled

	static ActorUID lastLegoActorUID = ActorUID::INVALID;

	if(msg[0] == L'!') {
		msg++;

		if(EA::StdC::Strncmp(msg, L"lego", 4) == 0) {
			WorldHub::ActorCore* playerActor = world.FindPlayerActor(playerActorUID[clientID]);
			ASSERT(playerActor);

			WorldHub::ActorCore& actor = world.SpawnPlayerActor(-1, (ClassType)18, SkinIndex::DEFAULT, L"legomage15", L"MEME");
			actor.pos = playerActor->pos;
			actor.dir = playerActor->dir;
			actor.eye = playerActor->eye;

			// trigger second emote
			actor.actionState = ActionStateID::EMOTION_BEHAVIORSTATE;
			actor.actionParam1 = 2;

			lastLegoActorUID = actor.UID;

			SendDbgMsg(clientID, LFMT(L"Actor spawned at (%g, %g, %g)", actor.pos.x, actor.pos.y, actor.pos.z));
			return true;
		}

		if(EA::StdC::Strncmp(msg, L"delete", 6) == 0) {
			world.DestroyPlayerActor(lastLegoActorUID);

			SendDbgMsg(clientID, LFMT(L"Actor destroyed (%u)", lastLegoActorUID));
			return true;
		}

		if(EA::StdC::Strncmp(msg, L"rozark", 6) == 0) {
			WorldHub::ActorCore* playerActor = world.FindPlayerActor(playerActorUID[clientID]);
			ASSERT(playerActor);

			WorldHub::ActorCore& actor = world.SpawnPlayerActor(-1, (ClassType)5001200, SkinIndex::DEFAULT, L"rozark", L"MEME");
			actor.pos = playerActor->pos;
			actor.dir = playerActor->dir;
			actor.eye = playerActor->eye;
			lastLegoActorUID = actor.UID;

			SendDbgMsg(clientID, LFMT(L"Actor spawned at (%g, %g, %g)", actor.pos.x, actor.pos.y, actor.pos.z));
			return true;
		}

		if(EA::StdC::Strncmp(msg, L"tanian", 6) == 0) {
			WorldHub::ActorCore* playerActor = world.FindPlayerActor(playerActorUID[clientID]);
			ASSERT(playerActor);

			WorldHub::ActorCore& actor = world.SpawnPlayerActor(-1, (ClassType)5001040, SkinIndex::DEFAULT, L"rozark", L"MEME");
			actor.pos = playerActor->pos;
			actor.dir = playerActor->dir;
			actor.eye = playerActor->eye;
			lastLegoActorUID = actor.UID;

			SendDbgMsg(clientID, LFMT(L"Actor spawned at (%g, %g, %g)", actor.pos.x, actor.pos.y, actor.pos.z));
			return true;
		}

		if(EA::StdC::Strncmp(msg, L"fish", 4) == 0) {
			WorldHub::ActorCore* playerActor = world.FindPlayerActor(playerActorUID[clientID]);
			ASSERT(playerActor);

			WorldHub::ActorCore& actor = world.SpawnPlayerActor(-1, (ClassType)5000800, SkinIndex::DEFAULT, L"rozark", L"MEME");
			actor.pos = playerActor->pos;
			actor.dir = playerActor->dir;
			actor.eye = playerActor->eye;
			lastLegoActorUID = actor.UID;

			SendDbgMsg(clientID, LFMT(L"Actor spawned at (%g, %g, %g)", actor.pos.x, actor.pos.y, actor.pos.z));
			return true;
		}

		if(EA::StdC::Strncmp(msg, L"smoke", 5) == 0) {
			WorldHub::ActorPlayer* playerActor = world.FindPlayerActor(playerActorUID[clientID]);
			ASSERT(playerActor);

			SendDbgMsg(clientID, LFMT(L"All you have to do was follow the damn train %s :(", playerActor->name.data()));
			return true;
		}

		if(EA::StdC::Strncmp(msg, L"upsidedown", 10) == 0) {
			JukeboxQueueSong(clientID, SongID::UpsideDown);
			return true;
		}
		
		if(EA::StdC::Strncmp(msg, L"scml", 4) == 0) {
			JukeboxQueueSong(clientID, SongID::Scml);
			return true;
		}

		if(EA::StdC::Strncmp(msg, L"poharan", 7) == 0) {
			JukeboxQueueSong(clientID, SongID::Poharan);
			return true;
		}

		if(EA::StdC::Strncmp(msg, L"triangle", 8) == 0) {
			JukeboxQueueSong(clientID, SongID::Triangle);
			return true;
		}
		
		if(EA::StdC::Strncmp(msg, L"arami", 5) == 0) {
			JukeboxQueueSong(clientID, SongID::Arami);
			return true;
		}

	}

	return false;
}

void GameHub::SendDbgMsg(i32 clientID, const wchar* msg)
{
	replication->SendChatMessageToClient(clientID, L"System", 1, msg);
}

void GameHub::SpawnNPC(CreatureIndex docID, i32 localID, const vec3& pos, const vec3& dir)
{
	WorldHub::ActorCore& actor = world.SpawnNpcActor(docID, localID);
	actor.pos = pos;
	actor.dir = dir;
}
