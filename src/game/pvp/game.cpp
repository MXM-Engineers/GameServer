#include "game.h"

#include <EAStdC/EAString.h>
#include <common/utils.h>

#include <game/coordinator.h> // account data
#include <game/game_content.h>
#include <game/config.h>

void Game::Init(Replication* replication_)
{
	replication = replication_;
	world.Init(replication);

	// TODO: move OUT
	MeshFile mfCollision;
	MeshFile mfEnv;
	ShapeMesh shape[2];

	bool r = OpenMeshFile("gamedata/PVP_DeathMatch01_Collision.msh", &mfCollision);
	ASSERT(r);
	r = OpenMeshFile("gamedata/PVP_DeathMatch01_Env.msh", &mfEnv);
	ASSERT(r);
	const MeshFile::Mesh& mshCol = mfCollision.meshList.front();
	r = MakeMapCollisionMesh(mshCol, &shape[0]);
	ASSERT(r);
	r = MakeMapCollisionMesh(mfEnv.meshList.front(), &shape[1]);
	ASSERT(r);
	world.physics.PushStaticMeshes(shape, 2);
	// --------------------------

	playerMap.fill(playerList.end());

	LoadMap();

	const auto& redSpawnPoints = mapSpawnPoints[(i32)TeamID::RED];
	const SpawnPoint& spawnPoint = redSpawnPoints[RandUint() % redSpawnPoints.size()];
	vec3 pos = spawnPoint.pos;
	vec3 dir = spawnPoint.dir;

	// create players
	World::Player& player = world.CreatePlayer(0, L"LordSk", L"Alpha", ClassType::LUA, SkinIndex::DEFAULT, ClassType::SIZUKA, SkinIndex::DEFAULT, pos);
	clone = &world.CreatePlayer(-1, L"Clone", L"BeepBoop", ClassType::LUA, SkinIndex::DEFAULT, ClassType::SIZUKA, SkinIndex::DEFAULT, pos);

	//lego = &world.CreatePlayer(-1, L"legomage15", L"MEME", (ClassType)18, SkinIndex::DEFAULT, ClassType::LUA, SkinIndex::DEFAULT, vec3(2800, 3532, 1000)));

	dbgGameUID = Dbg::PushNewGame("PVP_DeathMatch");
}

void Game::Update(Time localTime_)
{
	ProfileFunction();
	localTime = localTime_;
	Dbg::PushNewFrame(dbgGameUID);

	// update clone
	clone->input = world.FindPlayer((PlayerID)0)->input; // TODO: hardcoded hack
	// TODO: maybe don't replicate cloned players, just their master?

	if(lego) {
		enum Step {
			Move = 0,
			Stop = 1,
		};

		f64 localTimeSec = TimeDiffSec(localTime);
		u32 step = ((u64)localTimeSec % 5) == 0;

		if(legoLastStep == Step::Stop && step == Step::Move) {
			f32 a = randf01() * 2 * PI;
			legoDir = vec2(cosf(a), sinf(a));
			legoAngle = (legoAngle+1) % 4;
		}

		if(step == Step::Move) { // move
			f32 a = legoAngle * PI/2;
			lego->input.rot.upperYaw = a;
			lego->input.rot.bodyYaw = a;
			lego->input.moveTo = lego->body->pos + vec3(legoDir * 1000.f, 0);
			lego->input.speed = 626;
		}
		else { // stop
			lego->input.moveTo = vec3(0);
			lego->input.speed = 0;
		}

		legoLastStep = step;
	}


	world.Update(localTime);

	foreach_const(player, world.players) {
		Dbg::Entity e;
		e.UID = (u32)player->playerID;
		e.name = player->name;
		e.pos = player->body->pos;
		e.rot = player->input.rot;
		e.moveDir = NormalizeSafe(player->input.moveTo - player->body->pos);
		e.color = vec3(1, 0, 1);
		Dbg::PushEntity(dbgGameUID, e);
	}

	Dbg::PushPhysics(dbgGameUID, world.physics);
}

bool Game::LoadMap()
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

void Game::OnPlayerConnect(i32 clientID, const AccountData* accountData)
{
	playerAccountData[clientID] = accountData;

	playerList.push_back(Player(clientID));
	playerMap[clientID] = --playerList.end();
}

void Game::OnPlayerDisconnect(i32 clientID)
{
	if(playerMap[clientID] != playerList.end()) {
		const Player& player = *playerMap[clientID];

		LOG("[client%03d] Game :: OnClientDisconnect :: actorUID=%u", clientID, (u32)player.actorUID);

		// we can disconnect before spawning, so test if we have an actor associated
		// TODO: KILL MASTERS

		playerList.erase(playerMap[clientID]);
	}

	playerMap[clientID] = playerList.end();
	playerAccountData[clientID] = nullptr;
}

void Game::OnPlayerReadyToLoad(i32 clientID)
{
	replication->SendLoadPvpMap(clientID, StageIndex::PVP_DEATHMATCH);
}

void Game::OnPlayerGetCharacterInfo(i32 clientID, ActorUID actorUID)
{
	ASSERT(playerMap[clientID] != playerList.end());

	// TODO: associate clients to world players in some way
	// FIXME: hack

	const World::Player* player = world.FindPlayer((PlayerID)0);
	ASSERT(player);
	ASSERT(player->clientID == clientID);

	foreach_const(chit, player->characters) {
		const World::ActorMaster& chara = **chit;

		if(chara.UID == actorUID) {
			// TODO: health
			replication->SendCharacterInfo(clientID, chara.UID, (CreatureIndex)(100000000 + (i32)chara.classType), chara.classType, 2400, 2400);

			return;
		}
	}

	WARN("Client sent an invalid actorUID (clientID=%d actorUID=%u)", clientID, (u32)actorUID);
}

void Game::OnPlayerUpdatePosition(i32 clientID, ActorUID actorUID, const vec3& pos, const vec2& dir, const RotationHumanoid& rot, f32 speed, ActionStateID state, i32 actionID)
{
	ProfileFunction();

	ASSERT(playerMap[clientID] != playerList.end());

	// TODO: associate clients to world players in some way
	// FIXME: hack

	World::Player* player = world.FindPlayer((PlayerID)0);
	ASSERT(player);
	ASSERT(player->clientID == clientID);

	bool found = false;
	foreach_const(chit, player->characters) {
		const World::ActorMaster& chara = **chit;
		if(chara.UID == actorUID) {
			found = true;
			break;
		}
	}

	// NOTE: the client is not aware that we spawned a new actor for them yet, we ignore this packet
	// LordSk (30/08/2020)
	if(!found) {
		WARN("Client sent an invalid actorUID (clientID=%d actorUID=%u)", clientID, (u32)actorUID);
		return;
	}

	// TODO: check for movement hacking
	if(dir.x == 0 && dir.y == 0) {
		player->input.moveTo = pos;
	}
	else {
		player->input.moveTo = pos + vec3(glm::normalize(vec2(dir)) * speed, 0);
	}
	player->input.rot = rot;
	player->input.speed = speed;

	vec2 delta = vec2(pos - player->body->pos);
	LOG("Position diff = %f", glm::length(delta));
}

void Game::OnPlayerUpdateRotation(i32 clientID, ActorUID actorUID, const RotationHumanoid& rot)
{
	ASSERT(playerMap[clientID] != playerList.end());

	// TODO: associate clients to world players in some way
	// FIXME: hack

	World::Player* player = world.FindPlayer((PlayerID)0);
	ASSERT(player);
	ASSERT(player->clientID == clientID);

	bool found = false;
	foreach_const(chit, player->characters) {
		const World::ActorMaster& chara = **chit;
		if(chara.UID == actorUID) {
			found = true;
			break;
		}
	}

	// NOTE: the client is not aware that we spawned a new actor for them yet, we ignore this packet
	// LordSk (30/08/2020)
	if(!found) {
		WARN("Client sent an invalid actorUID (clientID=%d actorUID=%u)", clientID, (u32)actorUID);
		return;
	}

	player->input.rot = rot;
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

}

void Game::OnPlayerSyncActionState(i32 clientID, ActorUID actorUID, ActionStateID state, i32 param1, i32 param2, f32 rotate, f32 upperRotate)
{
	ASSERT(playerMap[clientID] != playerList.end());

	// TODO: associate clients to world players in some way
	// FIXME: hack

	World::Player* player = world.FindPlayer((PlayerID)0);
	ASSERT(player);
	ASSERT(player->clientID == clientID);

	bool found = false;
	foreach_const(chit, player->characters) {
		const World::ActorMaster& chara = **chit;
		if(chara.UID == actorUID) {
			found = true;
			break;
		}
	}

	// NOTE: the client is not aware that we spawned a new actor for them yet, we ignore this packet
	// LordSk (30/08/2020)
	if(!found) {
		WARN("Client sent an invalid actorUID (clientID=%d actorUID=%u)", clientID, (u32)actorUID);
		return;
	}

	// TODO: check hacking
	player->input.rot.bodyYaw = rotate;
	player->input.rot.upperYaw = upperRotate;
	player->input.actionState = state;
	player->input.actionParam1 = param1;
	player->input.actionParam2 = param2;
}

void Game::OnPlayerLoadingComplete(i32 clientID)
{

}

void Game::OnPlayerGameMapLoaded(i32 clientID)
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
	RotationHumanoid rot;

	// TODO: check if already leader character
	DBG_ASSERT(player.actorUID == ActorUID::INVALID);
	if((player.actorUID != ActorUID::INVALID)) {

		//world.DestroyPlayerActor(player.actorUID);
		//world.DestroyPlayerActor(player.cloneActorUID);
	}

	ASSERT(playerAccountData[clientID]); // account data is not assigned
	const AccountData* account = playerAccountData[clientID];

	World::Player& worldPlayer = world.GetPlayer((PlayerID)0);

	//player.actorUID = actor.UID;

	// force LocalActorID for both actors
	// TODO: eventually get rid of this system
	replication->PlayerRegisterMasterActor(clientID, worldPlayer.Main().UID, worldPlayer.mainClass);
	replication->PlayerRegisterMasterActor(clientID, worldPlayer.Sub().UID, worldPlayer.subClass);

	//player.cloneActorUID = clone.UID;
}

void Game::OnPlayerTag(i32 clientID, LocalActorID toLocalActorID)
{
	ASSERT(playerMap[clientID] != playerList.end());
	// TODO: associate clients to world players in some way
	// FIXME: hack

	World::Player* player = world.FindPlayer((PlayerID)0);
	ASSERT(player);
	ASSERT(player->clientID == clientID);

	// TODO: cooldown
	player->input.tag = 1;
}

void Game::OnPlayerJump(i32 clientID, LocalActorID toLocalActorID, f32 rotate, f32 moveDirX, f32 moveDirY)
{
	ASSERT(playerMap[clientID] != playerList.end());
	// TODO: associate clients to world players in some way
	// FIXME: hack

	World::Player* player = world.FindPlayer((PlayerID)0);
	ASSERT(player);
	ASSERT(player->clientID == clientID);

	player->input.jump = 1;

	// TODO: do this at the end of a frame
	replication->SendPlayerJump(clientID, player->Main().UID, rotate, moveDirX, moveDirY);
}

void Game::OnPlayerCastSkill(i32 clientID, const PlayerCastSkill& cast)
{
	ASSERT(playerMap[clientID] != playerList.end());
	Player& player = *playerMap[clientID];

	replication->SendPlayerAcceptCast(clientID, cast);
}

void Game::OnPlayerGameIsReady(i32 clientID)
{
	replication->SendGameReady(clientID);

	// TODO: after 5s
	replication->SendGameStart(clientID);
}

bool Game::ParseChatCommand(i32 clientID, const wchar* msg, const i32 len)
{
	if(!Config().DevMode) return false; // don't allow command when dev mode is not enabled

	ASSERT(playerMap[clientID] != playerList.end());
	const Player& player = *playerMap[clientID];

	static ActorUID lastLegoActorUID = ActorUID::INVALID;

	if(msg[0] == L'!') {
		msg++;

		if(EA::StdC::Strncmp(msg, L"lego", 4) == 0) {
			// TODO: restore?
			/*
			World::Player* playerActor = world.FindPlayer(player.actorUID); // TODO: find currently active actor
			ASSERT(playerActor);
			const vec3 pos = playerActor->Main().body->pos;

			World::Player& actor = world.SpawnPlayerMasters(-1, L"legomage15", L"MEME", (ClassType)18, SkinIndex::DEFAULT, ClassType::LUA, SkinIndex::DEFAULT, pos);
			actor.input.rot = playerActor->input.rot;

			// trigger second emote
			actor.input.actionState = ActionStateID::EMOTION_BEHAVIORSTATE;
			actor.input.actionParam1 = 2;

			lastLegoActorUID = actor.UID;

			SendDbgMsg(clientID, LFMT(L"Actor spawned at (%g, %g, %g)", pos.x, pos.y, pos.z));
			return true;
			*/
		}

		if(EA::StdC::Strncmp(msg, L"delete", 6) == 0) {
			// TODO: restore?
			/*
			world.DestroyPlayerActor(lastLegoActorUID);

			SendDbgMsg(clientID, LFMT(L"Actor destroyed (%u)", lastLegoActorUID));
			return true;
			*/
		}
	}

	return false;
}

void Game::SendDbgMsg(i32 clientID, const wchar* msg)
{
	replication->SendChatMessageToClient(clientID, L"System", 1, msg);
}

World::ActorNpc& Game::SpawnNPC(CreatureIndex docID, i32 localID, const vec3& pos, const vec3& dir)
{
	World::ActorNpc& actor = world.SpawnNpcActor(docID, localID);
	actor.pos = pos;
	actor.dir = dir;
	return actor;
}
