#include "game.h"
#include "coordinator.h" // account data


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
	// TODO: pull this info from the game's xml files
	SpawnNPC(100036952, Vec3(12437, 4859.2, 2701.5), Vec3(0, 0, 1.02137));
	SpawnNPC(100036896, Vec3(11556.2, 13308.7, 3328.29), Vec3(-1.61652, -1.14546, -0.893085));
	SpawnNPC(100036891, Vec3(14819.3, 9705.18, 2604.1), Vec3(0, 0, 0.783478));
	SpawnNPC(100036895, Vec3(13522, 12980, 3313.52), Vec3(0, 0, 0.703193));
	SpawnNPC(100036897, Vec3(12263.3, 13262.3, 3328.29), Vec3(0, 0, 0.426558));
	SpawnNPC(100036894, Vec3(12005.8, 13952.3, 3529.39), Vec3(0, 0, 0));
	SpawnNPC(100036909, Vec3(11551.5, 5382.32, 2701.5), Vec3(-3.08504, -0.897274, 0.665145));
	SpawnNPC(100036842, Vec3(8511.02, 8348.46, 2604.1), Vec3(0, 0, -1.63747));
	SpawnNPC(100036902, Vec3(9042.14, 9732.58, 2604.1), Vec3(3.06654, 1.39138, -0.873886));
	SpawnNPC(100036843, Vec3(14809.8, 7021.74, 2604.1), Vec3(0, 0, 2.46842));
	SpawnNPC(100036899, Vec3(10309, 13149, 3313.52), Vec3(0.914029, 0.112225, -0.642456));
	SpawnNPC(100036904, Vec3(7922.89, 6310.55, 3016.64), Vec3(0, 0, -1.33937));
	SpawnNPC(100036905, Vec3(8617, 5617, 3016.64), Vec3(0, 0, 3.08347));
	SpawnNPC(100036903, Vec3(12949.5, 8886.19, 2604.1), Vec3(0.0986111, 0.642107, -1.29835));
	SpawnNPC(100036954, Vec3(9094, 7048, 2604.1), Vec3(0, 0, -2.31972));
	SpawnNPC(100036951, Vec3(11301, 12115, 3313.52), Vec3(0, 0, -1.01316));
	SpawnNPC(100036906, Vec3(10931, 7739, 2605.23), Vec3(0, 0, 1.83539));
	SpawnNPC(100036833, Vec3(15335.5, 8370.4, 2604.1), Vec3(0, 0, 1.53903));
	SpawnNPC(100036777, Vec3(11925, 6784, 3013), Vec3(0, 0, 0));
	SpawnNPC(110041382, Vec3(3667.41, 2759.76, 2601), Vec3(0, 0, -0.598997));
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
	replication->EventPlayerRequestCharacterInfo(clientID, (u32)actor->UID, (i32)actor->modelID, actor->classType, 100, 100);
}

void Game::OnPlayerUpdatePosition(i32 clientID, LocalActorID characterID, const Vec3& pos, const Vec3& dir, const Vec3& eye, f32 rotate, f32 speed, i32 state, i32 actionID)
{
	ASSERT(playerActorUID[clientID] == replication->GetActorUID(clientID, characterID)); // TODO: soft cancel, kick client
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

void Game::OnPlayerSetLeaderCharacter(i32 clientID, LocalActorID characterID, i32 skinIndex)
{
	const i32 leaderMasterID = (u32)characterID - (u32)LocalActorID::FIRST_SELF_MASTER;

	Vec3 pos(11959.4f, 6451.76f, 3012);
	Vec3 dir(0, 0, 2.68874f);
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
	const eastl::array<i32,40> MasterClass = {
		35, // Lua
		3,	// Sizuka
		18, // Poharan
	};

	World::ActorPlayer& actor = world.SpawnPlayerActor(clientID, MasterClass[leaderMasterID], account->nickname.data(), account->guildTag.data());
	actor.pos = pos;
	actor.dir = dir;
	actor.eye = eye;
	actor.clientID = clientID; // TODO: this is not useful right now
	playerActorUID[clientID] = actor.UID;

	replication->EventPlayerSetLeaderMaster(clientID, playerActorUID[clientID], leaderMasterID);
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

			World::ActorCore& actor = world.SpawnPlayerActor(-1, 18, L"legomage15", L"MEME");
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

void Game::SpawnNPC(i32 modelID, const Vec3& pos, const Vec3& dir)
{
	World::ActorCore& actor = world.SpawnNpcActor((ActorModelID)modelID);
	actor.pos = pos;
	actor.dir = dir;
}
