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

	playerMap.fill(playerList.end());

	LoadMap();

	// spawn test
	World::ActorPlayer& lego = world.SpawnPlayer(-1, L"legomage15", L"MEME", (ClassType)18, SkinIndex::DEFAULT, ClassType::LUA, SkinIndex::DEFAULT, vec3(2800, 3532, 1000));

	legoUID = lego.UID;

	dbgGameUID = Dbg::PushNewGame("PVP_DeathMatch");
}

void Game::Update(Time localTime_)
{
	ProfileFunction();
	localTime = localTime_;
	Dbg::PushNewFrame(dbgGameUID);

	// update clone
	foreach_const(p, playerList) {
		if(p->actorUID != ActorUID::INVALID) {
			World::ActorPlayer* main = world.FindPlayerActor(p->actorUID);
			World::ActorPlayer* clone = world.FindPlayerActor(p->cloneActorUID);
			ASSERT(main);
			ASSERT(clone);

			clone->input = main->input;
		}
	}

	enum Step {
		Move = 0,
		Stop = 1,
	};

	World::ActorPlayer* lego = world.FindPlayerActor(legoUID);
	ASSERT(lego);
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
		lego->input.moveTo = lego->Current().body->pos + vec3(legoDir * 1000.f, 0);
		lego->input.speed = 626;
	}
	else { // stop
		lego->input.moveTo = vec3(0);
		lego->input.speed = 0;
	}

	legoLastStep = step;


	world.Update(localTime);

	foreach_const(actor, world.actorPlayerList) {
		Dbg::Entity e;
		e.UID = (u32)actor->UID;
		e.name = actor->name;
		e.pos = actor->Current().body->pos;
		e.rot = actor->input.rot;
		e.moveDir = NormalizeSafe(actor->input.moveTo - actor->Current().body->pos);
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

	replication->SendAccountDataPvp(clientID, *accountData);
}

void Game::OnPlayerDisconnect(i32 clientID)
{
	if(playerMap[clientID] != playerList.end()) {
		const Player& player = *playerMap[clientID];

		LOG("[client%03d] Game :: OnClientDisconnect :: actorUID=%u", clientID, (u32)player.actorUID);

		// we can disconnect before spawning, so test if we have an actor associated
		if(player.actorUID != ActorUID::INVALID) world.DestroyPlayerActor(player.actorUID);
		if(player.cloneActorUID != ActorUID::INVALID) world.DestroyPlayerActor(player.cloneActorUID);

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
	const Player& player = *playerMap[clientID];
	const World::ActorPlayer* actor = world.FindPlayerActor(player.actorUID);
	ASSERT(actor);
	ASSERT(actor->clientID == clientID);

	foreach_const(chit, actor->characters) {
		const World::ActorPlayerCharacter& chara = **chit;

		if(chara.UID == actorUID) {
			// TODO: health
			replication->SendCharacterInfo(clientID, chara.UID, (CreatureIndex)(100000000 + (i32)chara.classType), chara.classType, 2400, 2400);

			return;
		}
	}

	WARN("Client sent an invalid actorUID (clientID=%d actorUID=%u)", clientID, (u32)actorUID);
}

void Game::OnPlayerUpdatePosition(i32 clientID, ActorUID actorUID, const vec3& pos, const vec3& dir, const RotationHumanoid& rot, f32 speed, ActionStateID state, i32 actionID)
{
	ASSERT(playerMap[clientID] != playerList.end());
	const Player& player = *playerMap[clientID];
	World::ActorPlayer* actor = world.FindPlayerActor(player.actorUID);
	ASSERT(actor);
	DBG_ASSERT(actor->clientID == clientID);

	bool found = false;
	foreach_const(chit, actor->characters) {
		const World::ActorPlayerCharacter& chara = **chit;
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
	actor->input.moveTo = pos;
	actor->input.rot = rot;
	actor->input.speed = speed;
}

void Game::OnPlayerUpdateRotation(i32 clientID, ActorUID actorUID, const RotationHumanoid& rot)
{
	ASSERT(playerMap[clientID] != playerList.end());
	const Player& player = *playerMap[clientID];
	World::ActorPlayer* actor = world.FindPlayerActor(player.actorUID);
	ASSERT(actor);
	DBG_ASSERT(actor->clientID == clientID);

	bool found = false;
	foreach_const(chit, actor->characters) {
		const World::ActorPlayerCharacter& chara = **chit;
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

	actor->input.rot = rot;
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
	const Player& player = *playerMap[clientID];
	World::ActorPlayer* actor = world.FindPlayerActor(player.actorUID);
	ASSERT(actor);
	DBG_ASSERT(actor->clientID == clientID);

	bool found = false;
	foreach_const(chit, actor->characters) {
		const World::ActorPlayerCharacter& chara = **chit;
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
	actor->input.rot.bodyYaw = rotate;
	actor->input.rot.upperYaw = upperRotate;
	actor->input.actionState = state;
	actor->input.actionParam1 = param1;
	actor->input.actionParam2 = param2;
}

void Game::OnPlayerJukeboxQueueSong(i32 clientID, SongID songID)
{

}

void Game::OnPlayerLoadingComplete(i32 clientID)
{
	replication->SendPvpLoadingComplete(clientID);
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
		world.DestroyPlayerActor(player.actorUID);
		world.DestroyPlayerActor(player.cloneActorUID);
	}

	ASSERT(playerAccountData[clientID]); // account data is not assigned
	const AccountData* account = playerAccountData[clientID];

	// TODO: tie in account->leaderMasterID,skinIndex with class and model
	const ClassType classType = ClassType::LUA;
	const ClassType subClassType = ClassType::SIZUKA;

	World::ActorPlayer& actor = world.SpawnPlayer(clientID, account->nickname.data(), account->guildTag.data(), classType, SkinIndex::DEFAULT, subClassType, SkinIndex::DEFAULT, pos);

	player.actorUID = actor.UID;

	// force LocalActorID for both actors
	// TODO: eventually get rid of this system
	replication->PlayerRegisterMasterActor(clientID, actor.Main().UID, classType);
	replication->PlayerRegisterMasterActor(clientID, actor.Sub().UID, subClassType);

	World::ActorPlayer& clone = world.SpawnPlayer(-1, account->nickname.data(), account->guildTag.data(), classType, SkinIndex::DEFAULT, subClassType, SkinIndex::DEFAULT, pos);

	player.cloneActorUID = clone.UID;
}

void Game::OnPlayerTag(i32 clientID, LocalActorID toLocalActorID)
{
	ASSERT(playerMap[clientID] != playerList.end());
	Player& player = *playerMap[clientID];
	World::ActorPlayer* actor = world.FindPlayerActor(player.actorUID);
	ASSERT(actor);
	DBG_ASSERT(actor->clientID == clientID);

	actor->currentCharaID = PlayerCharaID::Enum(actor->currentCharaID ^ 1);

	// TODO: restore tag replication via frame differential
	// replication->SendPlayerTag(clientID, player.mainActorUID, player.subActorUID);
}

void Game::OnPlayerJump(i32 clientID, LocalActorID toLocalActorID, f32 rotate, f32 moveDirX, f32 moveDirY)
{
	ASSERT(playerMap[clientID] != playerList.end());
	Player& player = *playerMap[clientID];

	// TODO: restore
	//replication->SendPlayerJump(clientID, player.mainActorUID, rotate, moveDirX, moveDirY);
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
			World::ActorPlayer* playerActor = world.FindPlayerActor(player.actorUID); // TODO: find currently active actor
			ASSERT(playerActor);
			const vec3 pos = playerActor->Current().body->pos;

			World::ActorPlayer& actor = world.SpawnPlayer(-1, L"legomage15", L"MEME", (ClassType)18, SkinIndex::DEFAULT, ClassType::LUA, SkinIndex::DEFAULT, pos);
			actor.input.rot = playerActor->input.rot;

			// trigger second emote
			actor.input.actionState = ActionStateID::EMOTION_BEHAVIORSTATE;
			actor.input.actionParam1 = 2;

			lastLegoActorUID = actor.UID;

			SendDbgMsg(clientID, LFMT(L"Actor spawned at (%g, %g, %g)", pos.x, pos.y, pos.z));
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
