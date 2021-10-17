#include "game.h"

#include <EAStdC/EAString.h>
#include <EAStdC/EAScanf.h>
#include <common/utils.h>

#include <mxm/game_content.h>
#include "config.h"

void Game::Init(Server* server_, const In::MQ_CreateGame& gameInfo, const eastl::array<ClientHandle, MAX_PLAYERS>& playerClientHdList)
{
	replication.Init(server_);
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

	LoadMap();

	// create players
	i32 spawnPointIndex[2] = { 0 };

	for(int pi = 0; pi < gameInfo.playerCount; pi++) {
		const In::MQ_CreateGame::Player& p = gameInfo.players[pi];
		if(p.team != 0 && p.team != 1) continue; // skip spectators

		World::PlayerDescription desc;
		desc.userID = UserID(pi+1);
		desc.clientHd = playerClientHdList[pi];
		desc.name.assign(p.name.data, p.name.len);
		desc.guildTag = L"Alpha";
		desc.team = p.team;
		desc.masters = p.masters;
		desc.skins = p.skins;
		desc.skills = p.skills;

		const auto& spawnPoints = mapSpawnPoints[p.team];
		const SpawnPoint& spawnPoint = spawnPoints[spawnPointIndex[p.team]++ % spawnPoints.size()];
		vec3 pos = spawnPoint.pos;
		vec3 dir = spawnPoint.dir;

		World::Player& worldPlayer = world.CreatePlayer(desc, pos);

		playerList.emplace_back(desc.clientHd, p.accountUID, desc.name, worldPlayer.index);
		if(desc.clientHd != ClientHandle::INVALID) {
			playerMap.emplace(desc.clientHd, --playerList.end());

			replication.OnPlayerConnect(desc.clientHd, worldPlayer.index);
			replication.PlayerRegisterMasterActor(desc.clientHd, worldPlayer.Main().UID, worldPlayer.mainClass);
			replication.PlayerRegisterMasterActor(desc.clientHd, worldPlayer.Sub().UID, worldPlayer.subClass);
		}
	}

	//clone = &world.CreatePlayer(ClientHandle::INVALID, L"Clone", L"BeepBoop", ClassType::Lua, SkinIndex::DEFAULT, ClassType::Sizuka, SkinIndex::DEFAULT, pos);
	//lego = &world.CreatePlayer(-1, L"legomage15", L"MEME", (ClassType)18, SkinIndex::DEFAULT, ClassType::Lua, SkinIndex::DEFAULT, vec3(2800, 3532, 1000)));

	dbgGameUID = Dbg::PushNewGame("PVP_DeathMatch");
}

void Game::Update(Time localTime_)
{
	ProfileFunction();
	localTime = localTime_;
	Dbg::PushNewFrame(dbgGameUID);

	// update clone
	if(clone) {
		clone->input = world.GetPlayer(0).input; // TODO: hardcoded hack
		// TODO: maybe don't replicate cloned players, just their master?
	}

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

void Game::OnPlayerDisconnect(ClientHandle clientHd)
{
	Player& player = *playerMap.at(clientHd);

	// TODO: KILL MASTERS
	replication.OnPlayerDisconnect(clientHd);
}

void Game::OnPlayerReadyToLoad(ClientHandle clientHd)
{
	replication.SendLoadPvpMap(clientHd, MapIndex::PVP_DEATHMATCH);
}

void Game::OnPlayerGetCharacterInfo(ClientHandle clientHd, ActorUID actorUID)
{
	Player& p = *playerMap.at(clientHd);
	const World::Player& player = world.GetPlayer(p.playerIndex);
	ASSERT(player.clientHd == clientHd);

	foreach_const(chit, player.characters) {
		const World::ActorMaster& chara = **chit;

		if(chara.UID == actorUID) {
			// TODO: health
			replication.SendCharacterInfo(clientHd, chara.UID, (CreatureIndex)(100000000 + (i32)chara.classType), chara.classType, 2400, 2400);

			return;
		}
	}

	WARN("Client sent an invalid actorUID (clientHd=%x actorUID=%u)", clientHd, (u32)actorUID);
}

void Game::OnPlayerUpdatePosition(ClientHandle clientHd, ActorUID actorUID, const vec3& pos, const vec2& dir, const RotationHumanoid& rot, f32 speed, ActionStateID state, i32 actionID, f32 clientTime)
{
	ProfileFunction();

	Player& p = *playerMap.at(clientHd);
	World::Player& player = world.GetPlayer(p.playerIndex);
	ASSERT(player.clientHd == clientHd);

	bool found = false;
	foreach_const(chit, player.characters) {
		const World::ActorMaster& chara = **chit;
		if(chara.UID == actorUID) {
			found = true;
			break;
		}
	}

	// NOTE: the client is not aware that we spawned a new actor for them yet, we ignore this packet
	// LordSk (30/08/2020)
	if(!found) {
		WARN("Client sent an invalid actorUID (clientID=%x actorUID=%u)", clientHd, (u32)actorUID);
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
		vec3 posBodyDelta = prevBodyPos - player.body->pos;

		vec2 delta = vec2(pos - player.body->pos);
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
	prevBodyPos = player.body->pos;


	// TODO: check for movement hacking
	if(dir.x == 0 && dir.y == 0) {
		player.input.moveTo = pos;
	}
	else {
		player.input.moveTo = pos + vec3(glm::normalize(vec2(dir)) * speed, 0);
	}
	player.input.rot = rot;
	player.input.speed = speed;
}

void Game::OnPlayerUpdateRotation(ClientHandle clientHd, ActorUID actorUID, const RotationHumanoid& rot)
{
	Player& p = *playerMap.at(clientHd);
	World::Player& player = world.GetPlayer(p.playerIndex);
	ASSERT(player.clientHd == clientHd);

	bool found = false;
	foreach_const(chit, player.characters) {
		const World::ActorMaster& chara = **chit;
		if(chara.UID == actorUID) {
			found = true;
			break;
		}
	}

	// NOTE: the client is not aware that we spawned a new actor for them yet, we ignore this packet
	// LordSk (30/08/2020)
	if(!found) {
		WARN("Client sent an invalid actorUID (clientHd=%x actorUID=%u)", clientHd, (u32)actorUID);
		return;
	}

	player.input.rot = rot;
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

	Player& p = *playerMap.at(clientHd);
	replication.SendChatMessageToAll(p.name.data(), chatType, msg, msgLen);
}

void Game::OnPlayerChatWhisper(ClientHandle clientHd, const wchar* destNick, const wchar* msg)
{
	replication.SendChatWhisperConfirmToClient(clientHd, destNick, msg); // TODO: send a fail when the client is not found

	const Player* destPlayer = nullptr;
	foreach_const(pl, playerList) {
		if(pl->name.compare(destNick) == 0) {
			destPlayer = &*pl;
			break;
		}
	}

	if(!destPlayer) {
		SendDbgMsg(clientHd, LFMT(L"Player '%s' not found", destNick));
		return;
	}

	Player& p = *playerMap.at(clientHd);
	replication.SendChatWhisperToClient(destPlayer->clientHd, p.name.data(), msg);
}

void Game::OnPlayerSetLeaderCharacter(ClientHandle clientHd, LocalActorID characterID, SkinIndex skinIndex)
{

}

void Game::OnPlayerSyncActionState(ClientHandle clientHd, ActorUID actorUID, ActionStateID state, i32 param1, i32 param2, f32 rotate, f32 upperRotate)
{
	Player& p = *playerMap.at(clientHd);
	World::Player& player = world.GetPlayer(p.playerIndex);
	ASSERT(player.clientHd == clientHd);

	bool found = false;
	foreach_const(chit, player.characters) {
		const World::ActorMaster& chara = **chit;
		if(chara.UID == actorUID) {
			found = true;
			break;
		}
	}

	// NOTE: the client is not aware that we spawned a new actor for them yet, we ignore this packet
	// LordSk (30/08/2020)
	if(!found) {
		WARN("Client sent an invalid actorUID (clientHd=%x actorUID=%u)", clientHd, (u32)actorUID);
		return;
	}

	// TODO: check hacking
	player.input.rot.bodyYaw = rotate;
	player.input.rot.upperYaw = upperRotate;
	player.input.actionState = state;
	player.input.actionParam1 = param1;
	player.input.actionParam2 = param2;
}

void Game::OnPlayerLoadingComplete(ClientHandle clientHd)
{
	replication.SetPlayerLoaded(clientHd);
}

void Game::OnPlayerGameMapLoaded(ClientHandle clientHd)
{
	replication.SetPlayerAsInGame(clientHd);
}

void Game::OnPlayerTag(ClientHandle clientHd, LocalActorID toLocalActorID)
{
	Player& p = *playerMap.at(clientHd);
	World::Player& player = world.GetPlayer(p.playerIndex);
	ASSERT(player.clientHd == clientHd);

	// TODO: cooldown
	player.input.tag = 1;
}

void Game::OnPlayerJump(ClientHandle clientHd, LocalActorID toLocalActorID, f32 rotate, f32 moveDirX, f32 moveDirY)
{
	Player& p = *playerMap.at(clientHd);
	World::Player& player = world.GetPlayer(p.playerIndex);
	ASSERT(player.clientHd == clientHd);

	player.input.jump = 1;
}

void Game::OnPlayerCastSkill(ClientHandle clientHd, const PlayerCastSkill& cast)
{
	Player& p = *playerMap.at(clientHd);
	World::Player& player = world.GetPlayer(p.playerIndex);
	ASSERT(player.clientHd == clientHd);

	player.input.castSkill = cast.skillID;
	player.input.castPos = cast.p3nPos;
	float3 r = cast.posStruct.rotateStruct;
	player.input.rot = RotConvertToWorld({ r.x, r.y, r.z });

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

	Player& p = *playerMap.at(clientHd);

	static ActorUID lastLegoActorUID = ActorUID::INVALID;

	if(msg[0] == L'!') {
		msg++;

		if(EA::StdC::Strcmp(msg, L"lego") == 0) {
			// TODO: restore?
			/*
			World::Player* playerActor = world.FindPlayer(player.actorUID); // TODO: find currently active actor
			ASSERT(playerActor);
			const vec3 pos = playerActor->Main().body->pos;

			World::Player& actor = world.SpawnPlayerMasters(-1, L"legomage15", L"MEME", (ClassType)18, SkinIndex::DEFAULT, ClassType::Lua, SkinIndex::DEFAULT, pos);
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
	actor.faction = 2;
	return actor;
}
