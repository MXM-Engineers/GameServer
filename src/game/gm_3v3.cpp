#include "gm_3v3.h"
#include "coordinator.h" // account data
#include "game_content.h"
#include "config.h"
#include <EAStdC/EAString.h>


void Game3v3::Init(Replication* replication_)
{
	replication = replication_;
	replication->stageType = StageType::GAME_INSTANCE;
	memset(&playerActorUID, 0, sizeof(playerActorUID));

	world.Init(replication);

	foreach(it, playerClientIDMap) {
		*it = playerList.end();
	}

	LoadMap();
}

void Game3v3::Update(f64 delta, Time localTime_)
{
	ProfileFunction();

	localTime = localTime_;
	world.Update(delta, localTime);
}

bool Game3v3::LoadMap()
{
	const GameXmlContent& content = GetGameXmlContent();

	foreach(it, content.mapPvpDeathMatch.spawns) {
		// don't spawn "spawn points"
		if(it->IsSpawnPoint()) {
			if(it->team != TeamID::INVALID) {
				mapSpawnPoints[(i32)it->team].push_back(SpawnPoint{ it->pos, it->rot });
			}
			continue;
		}

		// spawn npc
		SpawnNPC(it->docID, it->localID, it->pos, it->rot);
	}

	return true;
}

void Game3v3::OnPlayerConnect(i32 clientID, const AccountData* accountData)
{
	playerAccountData[clientID] = accountData;

	playerList.push_back(Player(clientID));
	playerClientIDMap[clientID] = --playerList.end();

	replication->SendAccountDataPvp(clientID, *accountData);
}

void Game3v3::OnPlayerDisconnect(i32 clientID)
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

void Game3v3::OnPlayerReadyToLoad(i32 clientID)
{
	replication->SendLoadPvpMap(clientID, StageIndex::PVP_DEATHMATCH);
}

void Game3v3::OnPlayerGetCharacterInfo(i32 clientID, LocalActorID characterID)
{
	// TODO: health
	const World::ActorPlayer* actor = world.FindPlayerActor(playerActorUID[clientID]);
	ASSERT(actor->clientID == clientID);
	replication->EventPlayerRequestCharacterInfo(clientID, actor->UID, actor->docID, actor->classType, 100, 100);
}

void Game3v3::OnPlayerUpdatePosition(i32 clientID, LocalActorID characterID, const Vec3& pos, const Vec3& dir, const Vec3& eye, f32 rotate, f32 speed, ActionStateID state, i32 actionID)
{
	// NOTE: the client is not aware that we spawned a new actor for them yet, we ignore this packet
	// LordSk (30/08/2020)
	if(playerActorUID[clientID] != replication->GetActorUID(clientID, characterID)) {
		WARN("Client sent an invalid characterID (clientID=%d characterID=%d)", clientID, (u32)characterID);
		return;
	}

	World::ActorPlayer* actor = world.FindPlayerActor(playerActorUID[clientID]);
	ASSERT(actor);

	// TODO: check for movement hacking
	actor->pos = pos;
	actor->dir = dir;
	actor->eye = eye;
	actor->rotate = rotate;
	actor->speed = speed;
}

void Game3v3::OnPlayerChatMessage(i32 clientID, i32 chatType, const wchar* msg, i32 msgLen)
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

void Game3v3::OnPlayerChatWhisper(i32 clientID, const wchar* destNick, const wchar* msg)
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

void Game3v3::OnPlayerSetLeaderCharacter(i32 clientID, LocalActorID characterID, SkinIndex skinIndex)
{
	const i32 leaderMasterID = (u32)characterID - (u32)LocalActorID::FIRST_SELF_MASTER;

	const auto& redSpawnPoints = mapSpawnPoints[(i32)TeamID::RED];

	// select a spawn point at random
	const SpawnPoint& spawnPoint = redSpawnPoints[RandUint() % redSpawnPoints.size()];
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

void Game3v3::OnPlayerSyncActionState(i32 clientID, LocalActorID characterID, ActionStateID state, i32 param1, i32 param2, f32 rotate, f32 upperRotate)
{
	// NOTE: the client is not aware that we spawned a new actor for them yet, we ignore this packet
	// LordSk (10/09/2020)
	if(replication->GetLocalActorID(clientID, playerActorUID[clientID]) != characterID) {
		WARN("Client sent an invalid characterID (clientID=%d characterID=%d)", clientID, (u32)characterID);
		return;
	}

	World::ActorPlayer* actor = world.FindPlayerActor(playerActorUID[clientID]);
	ASSERT(actor);

	// TODO: check hacking
	actor->rotate = rotate;
	actor->upperRotate = upperRotate;
	actor->actionState = state;
	actor->actionParam1 = param1;
	actor->actionParam2 = param2;
}

void Game3v3::OnPlayerJukeboxQueueSong(i32 clientID, SongID songID)
{

}

bool Game3v3::ParseChatCommand(i32 clientID, const wchar* msg, const i32 len)
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
	}

	return false;
}

void Game3v3::SendDbgMsg(i32 clientID, const wchar* msg)
{
	replication->SendChatMessageToClient(clientID, L"System", 1, msg);
}

void Game3v3::SpawnNPC(CreatureIndex docID, i32 localID, const Vec3& pos, const Vec3& dir)
{
	World::ActorCore& actor = world.SpawnNpcActor(docID, localID);
	actor.pos = pos;
	actor.dir = dir;
}
