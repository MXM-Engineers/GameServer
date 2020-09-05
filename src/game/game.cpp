#include "game.h"
#include "coordinator.h" // account data
#include "game_content.h"
#include "config.h"
#include <EAStdC/EAString.h>


void Game::Init(Replication* replication_)
{
	replication = replication_;
	memset(&playerActorUID, 0, sizeof(playerActorUID));

	world.Init(replication);

	foreach(it, playerClientIDMap) {
		*it = playerList.end();
	}

	LoadMap();
}

void Game::Update(f64 delta, Time localTime_)
{
	localTime = localTime_;
	world.Update(delta);

	UpdateJukebox();
}

void Game::UpdateJukebox()
{
	// if the song playing ended, make the current song null
	bool doSendNewSong = false;
	if(jukebox.currentSong.songID != SongID::INVALID) {
		if(TimeDiffSec(TimeDiff(jukebox.playStartTime, localTime)) >= jukebox.currentSong.lengthInSec) {
			jukebox.currentSong = {};
		}
	}

	if(jukebox.currentSong.songID == SongID::INVALID) {
		if(!jukebox.queue.empty()) {
			jukebox.currentSong = jukebox.queue.front();
			jukebox.queue.pop_front();
			jukebox.playStartTime = localTime;
			doSendNewSong = true;
		}
	}

	// build new replication song queue
	eastl::fixed_vector<Replication::JukeboxTrack,Jukebox::MAX_TRACKS,false> tracks;
	foreach(it, jukebox.queue) {
		Replication::JukeboxTrack track;
		track.songID = it->songID;
		track.requesterNick = it->requesterNick;
		tracks.push_back(track);
	}

	// when the jukebox is replicated, we send its status
	// TODO: there is probably a better way to do this, we *know* when it is replicated since we send SN_ScanEnd
	// TODO: find a better way to represent this dependency
	const wchar* requesterNick = L"";
	i32 playPos;
	if(jukebox.currentSong.songID != SongID::INVALID) {
		requesterNick = jukebox.currentSong.requesterNick.data();
		playPos = round(TimeDiffSec(TimeDiff(jukebox.playStartTime, localTime)));
	}

	foreach(it, playerList) {
		if(!it->isJukeboxActorReplicated) {
			if(replication->IsActorReplicatedForClient(it->clientID, jukebox.npcActorUID)) {
				it->isJukeboxActorReplicated = true;

				if(jukebox.currentSong.songID != SongID::INVALID) {
					replication->SendJukeboxPlay(it->clientID, jukebox.currentSong.songID, requesterNick, playPos);
				}
				else { // send the Lobby music if no song is playing
					replication->SendJukeboxPlay(it->clientID, SongID::Default, L"", 0);
				}

				replication->SendJukeboxQueue(it->clientID, tracks.data(), tracks.size());
			}
		}
	}

	// replicate new sound playing
	if(doSendNewSong) {
		const wchar* requesterNick = jukebox.currentSong.requesterNick.data();
		const i32 playPos = round(TimeDiffSec(TimeDiff(jukebox.playStartTime, localTime)));

		foreach(it, playerList) {
			if(it->isJukeboxActorReplicated) {
				replication->SendJukeboxPlay(it->clientID, jukebox.currentSong.songID, requesterNick, playPos);
				replication->SendJukeboxQueue(it->clientID, tracks.data(), tracks.size());
			}
		}
	}
}

bool Game::JukeboxQueueSong(i32 clientID, SongID songID)
{
	ASSERT(playerAccountData[clientID]);

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

	Jukebox::Song song;
	song.requesterNick = playerAccountData[clientID]->nickname;
	song.songID = songID;
	song.lengthInSec = xmlSong->length;
	jukebox.queue.push_back(song);

	eastl::fixed_vector<Replication::JukeboxTrack,Jukebox::MAX_TRACKS,false> tracks;
	foreach(it, jukebox.queue) {
		Replication::JukeboxTrack track;
		track.songID = it->songID;
		track.requesterNick = it->requesterNick;
		tracks.push_back(track);
	}

	// TODO: find a better way to represent this dependency
	// send jukebox queue to players
	foreach(it, playerList) {
		if(it->isJukeboxActorReplicated) {
			replication->SendJukeboxQueue(it->clientID, tracks.data(), tracks.size());
		}
	}

	return true;
}

bool Game::LoadMap()
{
	const GameXmlContent& content = GetGameXmlContent();

	foreach(it, content.mapLobbyNormal.spawns) {
		// don't spawn "spawn points"
		if(it->IsSpawnPoint()) {
			mapSpawnPoints.push_back(SpawnPoint{ it->pos, it->rot });
			continue;
		}

		// spawn npc
		SpawnNPC(it->docID, it->localID, it->pos, it->rot);
	}

	auto npcJukeBox  = world.FindNpcActorByCreatureID(CreatureIndex::Jukebox);
	ASSERT(npcJukeBox != world.InvalidNpcHandle());
	jukebox.npcActorUID = npcJukeBox->UID;

	return true;
}

void Game::OnPlayerConnect(i32 clientID, const AccountData* accountData)
{
	playerAccountData[clientID] = accountData;

	playerList.push_back(Player(clientID));
	playerClientIDMap[clientID] = --playerList.end();
}

void Game::OnPlayerDisconnect(i32 clientID)
{
	LOG("[client%03d] Game :: OnClientDisconnect :: actorUID=%u", clientID, (u32)playerActorUID[clientID]);

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

void Game::OnPlayerGetCharacterInfo(i32 clientID, LocalActorID characterID)
{
	// TODO: health
	const World::ActorPlayer* actor = world.FindPlayerActor(playerActorUID[clientID]);
	ASSERT(actor->clientID == clientID);
	replication->EventPlayerRequestCharacterInfo(clientID, actor->UID, actor->docID, actor->classType, 100, 100);
}

void Game::OnPlayerUpdatePosition(i32 clientID, LocalActorID characterID, const Vec3& pos, const Vec3& dir, const Vec3& eye, f32 rotate, f32 speed, i32 state, i32 actionID)
{
	// NOTE: the client is not aware that we spawned a new actor for them yet, we ignore this packet
	// LordSk (30/08/2020)
	if(playerActorUID[clientID] != replication->GetActorUID(clientID, characterID)) {
		WARN("Client sent an invalid characterID (clientID=%d characterID=%d)", clientID, (u32)characterID);
		return;
	}
	world.PlayerUpdatePosition(playerActorUID[clientID], pos, dir, eye, rotate, speed, state, actionID);
}

void Game::OnPlayerChatMessage(i32 clientID, i32 chatType, const wchar* msg, i32 msgLen)
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

void Game::OnPlayerChatWhisper(i32 clientID, const wchar* destNick, const wchar* msg)
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

void Game::OnPlayerSetLeaderCharacter(i32 clientID, LocalActorID characterID, SkinIndex skinIndex)
{
	const i32 leaderMasterID = (u32)characterID - (u32)LocalActorID::FIRST_SELF_MASTER;

	// select a spawn point at random
	const SpawnPoint& spawnPoint = mapSpawnPoints[RandUint() % mapSpawnPoints.size()];
	Vec3 pos = spawnPoint.pos;
	Vec3 dir = spawnPoint.dir;
	Vec3 eye(0, 0, 0);

	// TODO: check if already leader character
	if((playerActorUID[clientID] != ActorUID::INVALID)) {
		World::ActorCore* actor = world.FindPlayerActor(playerActorUID[clientID]);
		ASSERT(actor);

		pos = actor->pos;
		dir = actor->dir;
		eye = actor->eye;

		world.DestroyPlayerActor(playerActorUID[clientID]);
	}

	ASSERT(playerAccountData[clientID]); // account data is not assigned
	const AccountData* account = playerAccountData[clientID];

	// TODO: tie in account->leaderMasterID,skinIndex with class and model
	const ClassType classType = GetGameXmlContent().masters[leaderMasterID].classType;

	World::ActorPlayer& actor = world.SpawnPlayerActor(clientID, classType, skinIndex, account->nickname.data(), account->guildTag.data());
	actor.pos = pos;
	actor.dir = dir;
	actor.eye = eye;
	actor.clientID = clientID; // TODO: this is not useful right now
	playerActorUID[clientID] = actor.UID;

	replication->SendPlayerSetLeaderMaster(clientID, playerActorUID[clientID], leaderMasterID, skinIndex);
}

void Game::OnPlayerSyncActionState(i32 clientID, const Cl::CN_GamePlayerSyncActionStateOnly& sync)
{
	DBG_ASSERT(replication->GetLocalActorID(clientID, playerActorUID[clientID]) == sync.characterID);

	// TODO: this should probably pass by the world in a form or another?
	// So we have actors that change action state
	replication->EventPlayerActionState(playerActorUID[clientID], sync); // TODO: temporarily directly pass the packet
}

void Game::OnPlayerJukeboxQueueSong(i32 clientID, SongID songID)
{
	JukeboxQueueSong(clientID, songID);
}

bool Game::ParseChatCommand(i32 clientID, const wchar* msg, const i32 len)
{
	if(!Config().devMode) return false; // don't allow command when dev mode is not enabled

	static ActorUID lastLegoActorUID = ActorUID::INVALID;

	if(msg[0] == L'!') {
		msg++;

		if(EA::StdC::Strncmp(msg, L"lego", 4) == 0) {
			World::ActorCore* playerActor = world.FindPlayerActor(playerActorUID[clientID]);
			ASSERT(playerActor);

			World::ActorCore& actor = world.SpawnPlayerActor(-1, (ClassType)18, SkinIndex::DEFAULT, L"legomage15", L"MEME");
			actor.pos = playerActor->pos;
			actor.dir = playerActor->dir;
			actor.eye = playerActor->eye;
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
			World::ActorCore* playerActor = world.FindPlayerActor(playerActorUID[clientID]);
			ASSERT(playerActor);

			World::ActorCore& actor = world.SpawnPlayerActor(-1, (ClassType)5001200, SkinIndex::DEFAULT, L"rozark", L"MEME");
			actor.pos = playerActor->pos;
			actor.dir = playerActor->dir;
			actor.eye = playerActor->eye;
			lastLegoActorUID = actor.UID;

			SendDbgMsg(clientID, LFMT(L"Actor spawned at (%g, %g, %g)", actor.pos.x, actor.pos.y, actor.pos.z));
			return true;
		}

		if(EA::StdC::Strncmp(msg, L"tanian", 6) == 0) {
			World::ActorCore* playerActor = world.FindPlayerActor(playerActorUID[clientID]);
			ASSERT(playerActor);

			World::ActorCore& actor = world.SpawnPlayerActor(-1, (ClassType)5001040, SkinIndex::DEFAULT, L"rozark", L"MEME");
			actor.pos = playerActor->pos;
			actor.dir = playerActor->dir;
			actor.eye = playerActor->eye;
			lastLegoActorUID = actor.UID;

			SendDbgMsg(clientID, LFMT(L"Actor spawned at (%g, %g, %g)", actor.pos.x, actor.pos.y, actor.pos.z));
			return true;
		}

		if(EA::StdC::Strncmp(msg, L"fish", 4) == 0) {
			World::ActorCore* playerActor = world.FindPlayerActor(playerActorUID[clientID]);
			ASSERT(playerActor);

			World::ActorCore& actor = world.SpawnPlayerActor(-1, (ClassType)5000800, SkinIndex::DEFAULT, L"rozark", L"MEME");
			actor.pos = playerActor->pos;
			actor.dir = playerActor->dir;
			actor.eye = playerActor->eye;
			lastLegoActorUID = actor.UID;

			SendDbgMsg(clientID, LFMT(L"Actor spawned at (%g, %g, %g)", actor.pos.x, actor.pos.y, actor.pos.z));
			return true;
		}

		if(EA::StdC::Strncmp(msg, L"smoke", 5) == 0) {
			World::ActorPlayer* playerActor = world.FindPlayerActor(playerActorUID[clientID]);
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

void Game::SendDbgMsg(i32 clientID, const wchar* msg)
{
	replication->SendChatMessageToClient(clientID, L"System", 1, msg);
}

void Game::SpawnNPC(CreatureIndex docID, i32 localID, const Vec3& pos, const Vec3& dir)
{
	World::ActorCore& actor = world.SpawnNpcActor(docID, localID);
	actor.pos = pos;
	actor.dir = dir;
}
