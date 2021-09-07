#include "game.h"

#include <EAStdC/EAString.h>
#include <EAStdC/EAScanf.h>
#include <common/utils.h>

#include "coordinator.h" // account data
#include <mxm/game_content.h>
#include "config.h"

void Game::Init(Server* server_, const ClientLocalMapping* plidMap_)
{
	plidMap = plidMap_;
	replication.Init(server_);
	replication.plidMap = plidMap_;
	world.Init(&replication);

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
	World::Player& player = world.CreatePlayer(ClientHandle(2), L"LordSk", L"Alpha", ClassType::LUA, SkinIndex::DEFAULT, ClassType::SIZUKA, SkinIndex::DEFAULT, pos);
	clone = &world.CreatePlayer(ClientHandle::INVALID, L"Clone", L"BeepBoop", ClassType::LUA, SkinIndex::DEFAULT, ClassType::SIZUKA, SkinIndex::DEFAULT, pos);

	//lego = &world.CreatePlayer(-1, L"legomage15", L"MEME", (ClassType)18, SkinIndex::DEFAULT, ClassType::LUA, SkinIndex::DEFAULT, vec3(2800, 3532, 1000)));

	dbgGameUID = Dbg::PushNewGame("PVP_DeathMatch");
}

void Game::Update(Time localTime_)
{
	ProfileFunction();
	localTime = localTime_;
	Dbg::PushNewFrame(dbgGameUID);

	// update clone
	clone->input = world.FindPlayer((UserID)0)->input; // TODO: hardcoded hack
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
		e.UID = (u32)player->userID;
		e.name = player->name;
		e.pos = player->body->pos;
		e.rot = player->input.rot;
		e.moveDir = NormalizeSafe(player->input.moveTo - player->body->pos);
		e.moveDest = player->input.moveTo;
		e.color = vec3(1, 0, 1);
		Dbg::PushEntity(dbgGameUID, e);
	}

	Dbg::PushPhysics(dbgGameUID, world.physics);

	replication.FrameEnd();
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

void Game::OnPlayerConnect(ClientHandle clientHd, const AccountData* accountData)
{
	const i32 clientID = plidMap->Get(clientHd);
	playerAccountData[clientID] = accountData;

	playerList.push_back(Player(clientHd));
	playerMap[clientID] = --playerList.end();

	replication.OnPlayerConnect(clientHd, accountData);
}

void Game::OnPlayerDisconnect(ClientHandle clientHd)
{
	const i32 clientID = plidMap->Get(clientHd);

	if(playerMap[clientID] != playerList.end()) {
		const Player& player = *playerMap[clientID];

		LOG("[client%03d] Game :: OnClientDisconnect :: actorUID=%u", clientID, (u32)player.actorUID);

		// we can disconnect before spawning, so test if we have an actor associated
		// TODO: KILL MASTERS

		playerList.erase(playerMap[clientID]);
	}


	playerMap[clientID] = playerList.end();
	playerAccountData[clientID] = nullptr;

	replication.OnPlayerDisconnect(clientHd);
}

void Game::OnPlayerReadyToLoad(ClientHandle clientHd)
{
	replication.SendLoadPvpMap(clientHd, StageIndex::PVP_DEATHMATCH);
}

void Game::OnPlayerGetCharacterInfo(ClientHandle clientHd, ActorUID actorUID)
{
	const i32 clientID = plidMap->Get(clientHd);
	ASSERT(playerMap[clientID] != playerList.end());

	// TODO: associate clients to world players in some way
	// FIXME: hack

	const World::Player* player = world.FindPlayer((UserID)0);
	ASSERT(player);
	ASSERT(player->clientHd == clientHd);

	foreach_const(chit, player->characters) {
		const World::ActorMaster& chara = **chit;

		if(chara.UID == actorUID) {
			// TODO: health
			replication.SendCharacterInfo(clientHd, chara.UID, (CreatureIndex)(100000000 + (i32)chara.classType), chara.classType, 2400, 2400);

			return;
		}
	}

	WARN("Client sent an invalid actorUID (clientID=%d actorUID=%u)", clientID, (u32)actorUID);
}

void Game::OnPlayerUpdatePosition(ClientHandle clientHd, ActorUID actorUID, const vec3& pos, const vec2& dir, const RotationHumanoid& rot, f32 speed, ActionStateID state, i32 actionID, f32 clientTime)
{
	ProfileFunction();

	const i32 clientID = plidMap->Get(clientHd);
	ASSERT(playerMap[clientID] != playerList.end());

	// TODO: associate clients to world players in some way
	// FIXME: hack

	World::Player* player = world.FindPlayer((UserID)0);
	ASSERT(player);
	ASSERT(player->clientHd == clientHd);

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

	const f64 serverTime = TimeDiffSec(TimeRelNow());

	static f64 prevClientTime = 0;
	static f64 prevServerTime = 0;
	static vec3 prevPos = vec3(0);
	static vec3 prevBodyPos = vec3(0);
	static f64 accumulatedSpeedDiff = 0.0;

	if(dir == vec2(0)) {
		accumulatedSpeedDiff = 0.0;
	}
	else {
		f64 clientDelta = clientTime - prevClientTime;
		f64 serverDelta = serverTime - prevServerTime;
		vec3 posDelta = prevPos - pos;
		vec3 posBodyDelta = prevBodyPos - player->body->pos;

		vec2 delta = vec2(pos - player->body->pos);
		f64 speedClient = glm::length(posDelta) / clientDelta;
		f64 speedServer = glm::length(posBodyDelta) / serverDelta;
		if(speedClient - speedServer > 0) {
			accumulatedSpeedDiff += speedClient - speedServer;
		}

		LOG("Position diff = %f", glm::length(delta));
		LOG("Speed client = %g", speedClient);
		LOG("Speed server = %g", speedServer);
		LOG("Speed diff accumulated = %g", accumulatedSpeedDiff);
	}

	prevClientTime = clientTime;
	prevServerTime = serverTime;
	prevPos = pos;
	prevBodyPos = player->body->pos;


	// TODO: check for movement hacking
	if(dir.x == 0 && dir.y == 0) {
		player->input.moveTo = pos;
	}
	else {
		player->input.moveTo = pos + vec3(glm::normalize(vec2(dir)) * speed, 0);
	}
	player->input.rot = rot;
	player->input.speed = speed;
}

void Game::OnPlayerUpdateRotation(ClientHandle clientHd, ActorUID actorUID, const RotationHumanoid& rot)
{
	const i32 clientID = plidMap->Get(clientHd);
	ASSERT(playerMap[clientID] != playerList.end());

	// TODO: associate clients to world players in some way
	// FIXME: hack

	World::Player* player = world.FindPlayer((UserID)0);
	ASSERT(player);
	ASSERT(player->clientHd == clientHd);

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

void Game::OnPlayerChatMessage(ClientHandle clientHd, i32 chatType, const wchar* msg, i32 msgLen)
{
	// command
	if(ParseChatCommand(clientHd, msg, msgLen)) {
		return; // we're done here
	}

	// TODO: chat types
	// TODO: senderStaffType
	// TODO: Actual chat system

	const i32 clientID = plidMap->Get(clientHd);
	ASSERT(playerAccountData[clientID]);
	replication.SendChatMessageToAll(playerAccountData[clientID]->nickname.data(), chatType, msg, msgLen);
}

void Game::OnPlayerChatWhisper(ClientHandle clientHd, const wchar* destNick, const wchar* msg)
{
	const i32 clientID = plidMap->Get(clientHd);
	ASSERT(playerAccountData[clientID]);
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

	replication.SendChatWhisperToClient(playerMap[destClientID]->clientHd, playerAccountData[clientID]->nickname.data(), msg);
}

void Game::OnPlayerSetLeaderCharacter(ClientHandle clientHd, LocalActorID characterID, SkinIndex skinIndex)
{

}

void Game::OnPlayerSyncActionState(ClientHandle clientHd, ActorUID actorUID, ActionStateID state, i32 param1, i32 param2, f32 rotate, f32 upperRotate)
{
	const i32 clientID = plidMap->Get(clientHd);
	ASSERT(playerMap[clientID] != playerList.end());

	// TODO: associate clients to world players in some way
	// FIXME: hack

	World::Player* player = world.FindPlayer((UserID)0);
	ASSERT(player);
	ASSERT(player->clientHd == clientHd);

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
		WARN("Client sent an invalid actorUID (clientID=%d actorUID=%u)", clientHd, (u32)actorUID);
		return;
	}

	// TODO: check hacking
	player->input.rot.bodyYaw = rotate;
	player->input.rot.upperYaw = upperRotate;
	player->input.actionState = state;
	player->input.actionParam1 = param1;
	player->input.actionParam2 = param2;
}

void Game::OnPlayerLoadingComplete(ClientHandle clientHd)
{
	replication.SetPlayerLoaded(clientHd);
}

void Game::OnPlayerGameMapLoaded(ClientHandle clientHd)
{
	// map is loaded, spawn
	// TODO: team, masters

	const i32 clientID = plidMap->Get(clientHd);
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

	World::Player& worldPlayer = world.GetPlayer((UserID)0);

	//player.actorUID = actor.UID;

	// force LocalActorID for both actors
	// TODO: eventually get rid of this system
	replication.PlayerRegisterMasterActor(clientHd, worldPlayer.Main().UID, worldPlayer.mainClass);
	replication.PlayerRegisterMasterActor(clientHd, worldPlayer.Sub().UID, worldPlayer.subClass);

	//player.cloneActorUID = clone.UID;

	replication.SetPlayerAsInGame(clientHd);
}

void Game::OnPlayerTag(ClientHandle clientHd, LocalActorID toLocalActorID)
{
	const i32 clientID = plidMap->Get(clientHd);
	ASSERT(playerMap[clientID] != playerList.end());
	// TODO: associate clients to world players in some way
	// FIXME: hack

	World::Player* player = world.FindPlayer((UserID)0);
	ASSERT(player);
	ASSERT(player->clientHd == clientHd);

	// TODO: cooldown
	player->input.tag = 1;
}

void Game::OnPlayerJump(ClientHandle clientHd, LocalActorID toLocalActorID, f32 rotate, f32 moveDirX, f32 moveDirY)
{
	const i32 clientID = plidMap->Get(clientHd);
	ASSERT(playerMap[clientID] != playerList.end());
	// TODO: associate clients to world players in some way
	// FIXME: hack

	World::Player* player = world.FindPlayer((UserID)0);
	ASSERT(player);
	ASSERT(player->clientHd == clientHd);

	player->input.jump = 1;
}

void Game::OnPlayerCastSkill(ClientHandle clientHd, const PlayerCastSkill& cast)
{
	const i32 clientID = plidMap->Get(clientHd);
	ASSERT(playerMap[clientID] != playerList.end());
	// TODO: associate clients to world players in some way
	// FIXME: hack

	World::Player* player = world.FindPlayer((UserID)0);
	ASSERT(player);
	ASSERT(player->clientHd == clientHd);

	player->input.castSkill = cast.skillID;
	player->input.castPos = cast.p3nPos;
	float3 r = cast.posStruct.rotateStruct;
	player->input.rot = RotConvertToWorld({ r.x, r.y, r.z });

	LOG("OnPlayerCastSkill :: (%f, %f, %f)", cast.p3nPos.x, cast.p3nPos.y, cast.p3nPos.z);
}

void Game::OnPlayerGameIsReady(ClientHandle clientHd)
{
	replication.SendGameReady(clientHd);

	// TODO: after 5s
	replication.SendGameStart(clientHd);
}

bool Game::ParseChatCommand(ClientHandle clientHd, const wchar* msg, const i32 len)
{
	if(!Config().DevMode) return false; // don't allow command when dev mode is not enabled

	const i32 clientID = plidMap->Get(clientHd);
	ASSERT(playerMap[clientID] != playerList.end());
	const Player& player = *playerMap[clientID];

	static ActorUID lastLegoActorUID = ActorUID::INVALID;

	if(msg[0] == L'!') {
		msg++;

		if(EA::StdC::Strcmp(msg, L"lego") == 0) {
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

		if(EA::StdC::Strcmp(msg, L"delete") == 0) {
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

void Game::SendDbgMsg(ClientHandle clientHd, const wchar* msg)
{
	replication.SendChatMessageToClient(clientHd, L"System", 1, msg);
}

World::ActorNpc& Game::SpawnNPC(CreatureIndex docID, i32 localID, const vec3& pos, const vec3& dir)
{
	World::ActorNpc& actor = world.SpawnNpcActor(docID, localID);
	actor.pos = pos;
	actor.dir = dir;
	return actor;
}
