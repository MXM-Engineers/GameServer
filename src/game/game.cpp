#include "game.h"
#include "coordinator.h" // account data
#include "game_content.h"


void Game::Init(Replication* replication_)
{
	replication = replication_;
	memset(&playerActorUID, 0, sizeof(playerActorUID));

	world.Init(replication);

	LoadMap();
}

void Game::Update(f64 delta)
{
	world.Update(delta); // TODO: update delta
}

bool Game::LoadMap()
{
	const GameXmlContent& content = GetGameXmlContent();

	foreach(it, content.mapLobbyNormal.spawns) {
		// don't spawn "spawn points"
		if(it->docID == CreatureIndex::SpawnPoint) {
			mapSpawnPoints.push_back(SpawnPoint{ it->pos, it->rot });
			continue;
		}

		// spawn npc
		SpawnNPC(it->docID, it->localID, it->pos, it->rot);
	}

	return true;
}

void Game::OnPlayerConnect(i32 clientID, const AccountData* accountData)
{
	playerAccountData[clientID] = accountData;
}

void Game::OnPlayerDisconnect(i32 clientID)
{
	LOG("[client%03d] Game :: OnClientDisconnect :: actorUID=%u", clientID, playerActorUID[clientID]);

	// we can disconnect before spawning, so test if we have an actor associated
	if(playerActorUID[clientID] != ActorUID::INVALID) {
		world.DestroyPlayerActor(playerActorUID[clientID]);
	}
	playerActorUID[clientID] = ActorUID::INVALID;
}

void Game::OnPlayerGetCharacterInfo(i32 clientID, LocalActorID characterID)
{
	// TODO: health
	const World::ActorPlayer* actor = world.FindPlayerActor(playerActorUID[clientID]);
	ASSERT(actor->clientID == clientID);
	replication->EventPlayerRequestCharacterInfo(clientID, actor->UID, (i32)actor->docID, actor->classType, 100, 100);
}

void Game::OnPlayerUpdatePosition(i32 clientID, LocalActorID characterID, const Vec3& pos, const Vec3& dir, const Vec3& eye, f32 rotate, f32 speed, i32 state, i32 actionID)
{
	// NOTE: the client is not aware that we spawned a new actor for them yet, we ignore this packet
	// LordSk (30/08/2020)
	if(playerActorUID[clientID] != replication->GetActorUID(clientID, characterID)) {
		WARN("Client sent an invalid characterID (clientID=%d characterID=%d)", clientID, characterID);
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

	replication->EventChatMessage(playerAccountData[clientID]->nickname.data(), chatType, msg, msgLen);
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

	replication->EventPlayerSetLeaderMaster(clientID, playerActorUID[clientID], leaderMasterID, skinIndex);
}

void Game::OnPlayerSyncActionState(i32 clientID, const Cl::CN_GamePlayerSyncActionStateOnly& sync)
{
	DBG_ASSERT(replication->GetLocalActorID(clientID, playerActorUID[clientID]) == sync.characterID);

	// TODO: this should probably pass by the world in a form or another?
	// So we have actors that change action state
	replication->EventPlayerActionState(playerActorUID[clientID], sync); // TODO: temporarily directly pass the packet
}

bool Game::ParseChatCommand(i32 clientID, const wchar* msg, const i32 len)
{
	static ActorUID lastLegoActorUID = ActorUID::INVALID;

	if(msg[0] == L'!') {
		msg++;

		if(wcsncmp(msg, L"lego", 4) == 0) {
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

		if(wcsncmp(msg, L"delete", 6) == 0) {
			world.DestroyPlayerActor(lastLegoActorUID);

			SendDbgMsg(clientID, LFMT(L"Actor destroyed (%u)", lastLegoActorUID));
			return true;
		}
	}

	return false;
}

void Game::SendDbgMsg(i32 clientID, const wchar* msg)
{
	replication->EventChatMessageToClient(clientID, L"System", 1, msg);
}

void Game::SpawnNPC(CreatureIndex docID, i32 localID, const Vec3& pos, const Vec3& dir)
{
	World::ActorCore& actor = world.SpawnNpcActor(docID, localID);
	actor.pos = pos;
	actor.dir = dir;
}
