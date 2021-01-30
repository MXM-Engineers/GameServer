#include "gm_3v3.h"
#include "coordinator.h" // account data
#include "game_content.h"
#include "config.h"
#include <EAStdC/EAString.h>


void Game3v3::Init(Replication* replication_)
{
	replication = replication_;
	replication->stageType = StageType::GAME_INSTANCE;

	world.Init(replication);

	playerMap.fill(playerList.end());

	LoadMap();

	dbgGameUID = Dbg::PushNewGame("PVP_DeathMatch");
}

void Game3v3::Update(f64 delta, Time localTime_)
{
	ProfileFunction();

	Dbg::PushNewFrame(dbgGameUID);

	localTime = localTime_;
	world.Update(delta, localTime);

	foreach_const(ent, world.actorPlayerList) {
		Dbg::PushEntity(dbgGameUID, ent->pos, vec3(1, 0, 1));
	}
}

bool Game3v3::LoadMap()
{
	const GameXmlContent& content = GetGameXmlContent();

	foreach(it, content.mapPvpDeathMatch.creatures) {
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

	// TODO: restore this (spawn safe area walls)
	/*
	foreach(it, content.mapPvpDeathMatch.dynamic) {
		// spawn npc
		World::ActorNpc& npc = SpawnNPC(it->docID, it->localID, it->pos, it->rot);
		npc.type = 3;
		npc.faction = 2;
	}
	*/

	return true;
}

void Game3v3::OnPlayerConnect(i32 clientID, const AccountData* accountData)
{
	playerAccountData[clientID] = accountData;

	playerList.push_back(Player(clientID));
	playerMap[clientID] = --playerList.end();

	replication->SendAccountDataPvp(clientID, *accountData);
}

void Game3v3::OnPlayerDisconnect(i32 clientID)
{
	if(playerMap[clientID] != playerList.end()) {
		const Player& player = *playerMap[clientID];

		LOG("[client%03d] Game :: OnClientDisconnect :: mainActorUID=%u subActorUID=%u", clientID, (u32)player.mainActorUID, (u32)player.subActorUID);

		// we can disconnect before spawning, so test if we have an actor associated
		if(player.mainActorUID != ActorUID::INVALID) world.DestroyPlayerActor(player.mainActorUID);
		if(player.subActorUID != ActorUID::INVALID) world.DestroyPlayerActor(player.subActorUID);

		playerList.erase(playerMap[clientID]);
	}

	playerMap[clientID] = playerList.end();
	playerAccountData[clientID] = nullptr;
}

void Game3v3::OnPlayerReadyToLoad(i32 clientID)
{
	replication->SendLoadPvpMap(clientID, StageIndex::PVP_DEATHMATCH);
}

void Game3v3::OnPlayerGetCharacterInfo(i32 clientID, ActorUID actorUID)
{
	ASSERT(playerMap[clientID] != playerList.end());
	const Player& player = *playerMap[clientID];

	if(player.mainActorUID != actorUID && player.subActorUID != actorUID) {
		WARN("Client sent an invalid actorUID (clientID=%d actorUID=%u)", clientID, (u32)actorUID);
		return;
	}

	// TODO: health
	const World::ActorPlayer* actor = world.FindPlayerActor(actorUID);
	ASSERT(actor->clientID == clientID);
	replication->SendCharacterInfo(clientID, actor->UID, actor->docID, actor->classType, 2400, 2400);
}

void Game3v3::OnPlayerUpdatePosition(i32 clientID, ActorUID actorUID, const vec3& pos, const vec3& dir, const vec3& eye, f32 rotate, f32 speed, ActionStateID state, i32 actionID)
{
	ASSERT(playerMap[clientID] != playerList.end());
	const Player& player = *playerMap[clientID];

	// NOTE: the client is not aware that we spawned a new actor for them yet, we ignore this packet
	// LordSk (30/08/2020)
	if(player.mainActorUID != actorUID && player.subActorUID != actorUID) {
		WARN("Client sent an invalid actorUID (clientID=%d actorUID=%u)", clientID, (u32)actorUID);
		return;
	}

	World::ActorPlayer* actor = world.FindPlayerActor(actorUID);
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

}

void Game3v3::OnPlayerSyncActionState(i32 clientID, ActorUID actorUID, ActionStateID state, i32 param1, i32 param2, f32 rotate, f32 upperRotate)
{
	ASSERT(playerMap[clientID] != playerList.end());
	const Player& player = *playerMap[clientID];

	// NOTE: the client is not aware that we spawned a new actor for them yet, we ignore this packet
	// LordSk (10/09/2020)
	if(player.mainActorUID != actorUID && player.subActorUID != actorUID) {
		WARN("Client sent an invalid actorUID (clientID=%d actorUID=%u)", clientID, (u32)actorUID);
		return;
	}

	World::ActorPlayer* actor = world.FindPlayerActor(actorUID);
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

void Game3v3::OnPlayerLoadingComplete(i32 clientID)
{
	replication->SendPvpLoadingComplete(clientID);
}

void Game3v3::OnPlayerGameMapLoaded(i32 clientID)
{
	// map is loaded, spawn
	// TODO: team, masters

	ASSERT(playerMap[clientID] != playerList.end());
	Player& player = *playerMap[clientID];

	const auto& redSpawnPoints = mapSpawnPoints[(i32)TeamID::RED];

	// select a spawn point at random
	const SpawnPoint& spawnPoint = redSpawnPoints[RandUint() % redSpawnPoints.size()];
	vec3 pos = spawnPoint.pos;
	vec3 dir = spawnPoint.dir;
	vec3 eye(0, 0, 0);

	// TODO: check if already leader character
	if((player.mainActorUID != ActorUID::INVALID)) {
		World::ActorCore* actor = world.FindPlayerActor(player.mainActorUID);
		ASSERT(actor);

		pos = actor->pos;
		dir = actor->dir;
		eye = actor->eye;

		world.DestroyPlayerActor(player.mainActorUID);
		world.DestroyPlayerActor(player.subActorUID);
	}

	ASSERT(playerAccountData[clientID]); // account data is not assigned
	const AccountData* account = playerAccountData[clientID];

	// TODO: tie in account->leaderMasterID,skinIndex with class and model
	const ClassType classType = ClassType::LUA;
	World::ActorPlayer& main = world.SpawnPlayerActor(clientID, classType, SkinIndex::DEFAULT, account->nickname.data(), account->guildTag.data());
	main.pos = pos;
	main.dir = dir;
	main.eye = eye;
	main.clientID = clientID; // TODO: this is not useful right now
	player.mainActorUID = main.UID;

	// spawn sub actor as well
	ClassType subClassType = ClassType::SIZUKA;
	World::ActorPlayer& sub = world.SpawnPlayerSubActor(clientID, main.UID, subClassType, SkinIndex::DEFAULT);
	sub.pos = pos;
	sub.dir = dir;
	sub.eye = eye;
	sub.clientID = clientID; // TODO: this is not useful right now
	player.subActorUID = sub.UID;

	// force LocalActorID for both actors
	// TODO: eventually get rid of this system
	replication->PlayerRegisterMasterActor(clientID, player.mainActorUID, classType);
	replication->PlayerRegisterMasterActor(clientID, player.subActorUID, subClassType);
}

void Game3v3::OnPlayerTag(i32 clientID, LocalActorID toLocalActorID)
{
	ASSERT(playerMap[clientID] != playerList.end());
	Player& player = *playerMap[clientID];

	eastl::swap(player.mainActorUID, player.subActorUID);
	replication->SendPlayerTag(clientID, player.mainActorUID, player.subActorUID);
}

void Game3v3::OnPlayerJump(i32 clientID, LocalActorID toLocalActorID, f32 rotate, f32 moveDirX, f32 moveDirY)
{
	ASSERT(playerMap[clientID] != playerList.end());
	Player& player = *playerMap[clientID];

	replication->SendPlayerJump(clientID, player.mainActorUID, rotate, moveDirX, moveDirY);
}

void Game3v3::OnPlayerCastSkill(i32 clientID, const PlayerCastSkill& cast)
{
	ASSERT(playerMap[clientID] != playerList.end());
	Player& player = *playerMap[clientID];

	replication->SendPlayerAcceptCast(clientID, cast);
}

void Game3v3::OnPlayerGameIsReady(i32 clientID)
{
	replication->SendGameReady(clientID);

	// TODO: after 5s
	replication->SendGameStart(clientID);
}

bool Game3v3::ParseChatCommand(i32 clientID, const wchar* msg, const i32 len)
{
	if(!Config().DevMode) return false; // don't allow command when dev mode is not enabled

	ASSERT(playerMap[clientID] != playerList.end());
	const Player& player = *playerMap[clientID];

	static ActorUID lastLegoActorUID = ActorUID::INVALID;

	if(msg[0] == L'!') {
		msg++;

		if(EA::StdC::Strncmp(msg, L"lego", 4) == 0) {
			World::ActorCore* playerActor = world.FindPlayerActor(player.mainActorUID); // TODO: find currently active actor
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

World::ActorNpc& Game3v3::SpawnNPC(CreatureIndex docID, i32 localID, const vec3& pos, const vec3& dir)
{
	World::ActorNpc& actor = world.SpawnNpcActor(docID, localID);
	actor.pos = pos;
	actor.dir = dir;
	return actor;
}
