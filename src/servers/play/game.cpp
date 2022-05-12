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

	LoadMap();

	const GameXmlContent& xml = GetGameXmlContent();

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

		const auto& master0 = xml.GetMaster(p.masters[0]);
		const auto& master1 = xml.GetMaster(p.masters[1]);

		desc.colliderSize[0] = {
			(u16)master0.character.getColliderRadius(),
			(u16)master0.character.getColliderHeight(),
		};
		desc.colliderSize[1] = {
			(u16)master1.character.getColliderRadius(),
			(u16)master1.character.getColliderHeight(),
		};

		const auto& spawnPoints = mapSpawnPoints[p.team];
		const SpawnPoint& spawnPoint = spawnPoints[spawnPointIndex[p.team]++ % spawnPoints.size()];

		World::Player& worldPlayer = world.CreatePlayer(desc, spawnPoint.pos, RotationHumanoid{0.f, 0.f, MxmYawToWorldYaw(spawnPoint.rot.z)});

		playerList.emplace_back(desc.clientHd, p.accountUID, desc.name, worldPlayer.index);
		if(desc.clientHd != ClientHandle::INVALID) {
			playerMap.emplace(desc.clientHd, --playerList.end());

			replication.OnPlayerConnect(desc.clientHd, worldPlayer.index);
			replication.PlayerRegisterMasterActor(desc.clientHd, worldPlayer.Main().UID, worldPlayer.mainClass);
			replication.PlayerRegisterMasterActor(desc.clientHd, worldPlayer.Sub().UID, worldPlayer.subClass);
		}
		else {
			botList.emplace_back(worldPlayer.index);
		}
	}

	dbgGameUID = Dbg::PushNewGame("PVP_DeathMatch");
}

void Game::Cleanup()
{
	world.Cleanup();
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
			f32 a = Randf01() * 2 * PI;
			legoDir = vec2(cosf(a), sinf(a));
			legoAngle = (legoAngle+1) % 4;
		}

		if(step == Step::Move) { // move
			f32 a = legoAngle * PI/2;
			lego->input.rot.upperYaw = a;
			lego->input.rot.bodyYaw = a;
			lego->input.moveTo = lego->body->GetWorldPos() + vec3(legoDir * 1000.f, 0);
			lego->input.speed = 626;
		}
		else { // stop
			lego->input.moveTo = vec3(0);
			lego->input.speed = 0;
		}

		legoLastStep = step;
	}

	switch(phase) {
		case Phase::WaitingForReady: {
			if(phaseTime < localTime) {
				phase = Phase::PreGame;
				phaseTime = TimeAdd(localTime, TimeMsToTime(10000));

				foreach_const(p, playerList) {
					replication.SendPreGameLevelEvents(p->clientHd);
				}
			}
		} break;

		case Phase::PreGame: {
			if(phaseTime < localTime) {
				phase = Phase::Game;

				// FIXME: hack to delete the spawn doors
				// FIXME: door death effect
				for(auto it = world.actorDynamicList.begin(); it != world.actorDynamicList.end();) {
					if(it->docID == CreatureIndex(110040546)) { // door
						world.actorDynamicMap.erase(it->UID);
						it = world.actorDynamicList.erase(it);
					}
					else {
						++it;
					}
				}

				foreach_const(p, playerList) {
					replication.SendGameStart(p->clientHd);
				}
			}
		} break;

		case Phase::Game: {
#if 0
			// bot random actions
			foreach(bot, botList) {
				if(localTime > bot->tNextAction) {
					bot->tNextAction = TimeAdd(localTime, TimeMsToTime(Randf01() * 5 * 1000));
					World::Player& wpl = world.GetPlayer(bot->playerIndex);

					struct Action {
						enum Enum: i32 {
							Move = 0,
							Tag,
							Jump,
							_Count
						};
					};

					eastl::array<f32,Action::_Count> actionWeight = {0.f};
					actionWeight[Action::Move] = 8;
					actionWeight[Action::Tag] = 2;
					actionWeight[Action::Jump] = 2;

					f32 actionSpace = 0;
					foreach_const(w, actionWeight) actionSpace += *w;

					Action::Enum actionID = Action::Move;

					f32 rs = Randf01() * actionSpace;
					actionSpace = 0;
					for(int wi = 0; wi < Action::_Count; wi++) {
						if(rs >= actionSpace && rs < actionSpace + actionWeight[wi]) {
							actionID = (Action::Enum)wi;
							break;
						}
						actionSpace += actionWeight[wi];
					}

					switch(actionID) {
						case Action::Move: {
							f32 angle = Randf01() * 2*PI;
							f32 dist = (f32)RandInt(250, 1000);
							vec2 off = vec2(cosf(angle) * dist, sinf(angle) * dist);
							wpl.input.moveTo = wpl.body->GetWorldPos() + vec3(off, 0);
							wpl.input.rot.upperYaw = angle;
							wpl.input.rot.upperPitch = 0;
							wpl.input.rot.bodyYaw = angle;
							wpl.input.speed = 600;
						} break;

						case Action::Tag: {
							wpl.input.tag = true;
						} break;

						case Action::Jump: {
							wpl.input.jump = true;
						} break;
					}
				}
			}
#endif
		} break;
	}

	world.Update(localTime);

	foreach_const(player, world.players) {
		Dbg::PlayerMaster e;
		e.UID = (u32)player->userID;
		e.name = player->name;
		e.pos = player->body->GetWorldPos();
		e.rot = player->input.rot;
		e.moveDir = NormalizeSafe(player->input.moveTo - player->body->GetWorldPos());
		e.moveDest = player->input.moveTo;
		e.color = vec3(1, 0, 1);
		Dbg::Push(dbgGameUID, e);
	}

	foreach_const(npc, world.actorNpcList) {
		Dbg::Npc n;
		n.UID = (u32)npc->UID;
		n.pos = npc->pos;
		n.rot = {}; // TODO: fill
		Dbg::Push(dbgGameUID, n);
	}

	foreach_const(dyn, world.actorDynamicList) {
		Dbg::Npc n;
		n.UID = (u32)dyn->UID;
		n.pos = dyn->pos;
		n.rot = {}; // TODO: fill
		Dbg::Push(dbgGameUID, n);
	}

	Dbg::PushPhysics(dbgGameUID, world.physics);

	replication.FrameEnd();
}

bool Game::LoadMap()
{
	// TODO: Should probably part of world?
	// Also at some point we need to better organise map data

	auto& phys = PhysContext();
	PxTriangleMesh* pvpCollision1;
	PxTriangleMesh* pvpCollision2;

	const GameXmlContent& gc = GetGameXmlContent();
	bool r = phys.LoadCollisionMesh(&pvpCollision1, gc.filePvpDeathmatch01Collision);
	if(!r) {
		LOG("ERROR: LoadCollisionMesh failed (pvpCollision1)");
		return false;
	}
	r = phys.LoadCollisionMesh(&pvpCollision2, gc.filePvpDeathmatch01CollisionWalls);
	if(!r) {
		LOG("ERROR: LoadCollisionMesh failed (pvpCollision2)");
		return false;
	}
	world.physics.CreateStaticCollider(pvpCollision1);
	world.physics.CreateStaticCollider(pvpCollision2);
	// --------------------------------

	const GameXmlContent& content = GetGameXmlContent();

	foreach(it, content.mapPvpDeathMatch.creatures) {
		// don't spawn "spawn points"
		if(it->IsSpawnPoint()) {
			if(it->faction != Faction::INVALID) {
				mapSpawnPoints[(i32)it->faction].push_back(SpawnPoint{ it->pos, it->rot });
			}
			continue;
		}

		// spawn npc
		World::ActorNpc& actor = world.SpawnNpcActor(it->docID, it->localID);
		actor.pos = it->pos;
		actor.rot = it->rot;
		actor.faction = it->faction;
	}

	foreach(it, content.mapPvpDeathMatch.dynamic) {
		// spawn dynamic
		auto& actor = world.SpawnDynamic(it->docID, it->localID);
		actor.pos = it->pos;
		actor.rot = it->rot;
		actor.faction = it->faction;
	}

	// TODO: spawn walls dynamically
	foreach(it, content.mapPvpDeathMatch.areas) {
		if(it->layer == 49) {
			// spawn wall
			auto& actor = world.SpawnDynamic(CreatureIndex(110042602), it->ID);
			actor.pos = it->pos;
			actor.rot = it->rot;
		}
	}
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
		vec3 posBodyDelta = prevBodyPos - player.body->GetWorldPos();

		vec2 delta = vec2(pos - player.body->GetWorldPos());
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
	prevBodyPos = player.body->GetWorldPos();


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

}

void Game::OnPlayerGameMapLoaded(ClientHandle clientHd)
{

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
	const i32 READY_WAIT = 3000;

	if(phase == Phase::WaitingForFirstPlayer) {
		phase = Phase::WaitingForReady;
		phaseTime = TimeAdd(localTime, TimeMsToTime(READY_WAIT));
		replication.SendGameReady(clientHd, READY_WAIT, 0);
	}
	else {
		replication.SendGameReady(clientHd, READY_WAIT, MAX(0, READY_WAIT - (i32)TimeDurationMs(localTime, phaseTime)));
	}
}

bool Game::ParseChatCommand(ClientHandle clientHd, const wchar* msg, const i32 len)
{
	if(!Config().DevMode) return false; // don't allow command when dev mode is not enabled

	// null terminate
	eastl::fixed_string<wchar,1024,true> msgBuff;
	msgBuff.assign(msg, len);
	msg = msgBuff.data();

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

		i32 event = 0;
		if(EA::StdC::Sscanf(msg, L"e %d", &event) == 1) {
			replication.SendChatMessageToClient(clientHd, L"Dbg", EChatType::NOTICE_CHAT, LFMT(L"Event (%d)", event));
			replication.SendClientLevelEvent(clientHd, event);
			return true;
		}

		i32 eventSeq = 0;
		if(EA::StdC::Sscanf(msg, L"es %d", &eventSeq) == 1) {
			replication.SendChatMessageToClient(clientHd, L"Dbg", EChatType::NOTICE_CHAT, LFMT(L"Event Sequence (%d)", eventSeq));
			replication.SendClientLevelEventSeq(clientHd, eventSeq);
			return true;
		}

		// Spawn a wall
		// NOTE: Walls probably spawn on an "area point" (Area.xml)
		if(EA::StdC::Strcmp(msg, L"wall") == 0) {
			World::Player& playerActor = world.GetPlayer(p.playerIndex); // TODO: find currently active actor
			const vec3 pos = playerActor.body->GetWorldPos();

			// is lovalID used for anything?
			static i32 localID = 37;
			World::ActorDynamic& dyn = world.SpawnDynamic(CreatureIndex(110042602), localID++);
			dyn.pos = pos;

			SendDbgMsg(clientHd, LFMT(L"Actor spawned at (%g, %g, %g)", pos.x, pos.y, pos.z));
			return true;
		}

		// TODO:
		// - Spawn all walls
		// - Make them have a physical body (box)
	}

	return false;
}

void Game::SendDbgMsg(ClientHandle clientHd, const wchar* msg)
{
	replication.SendChatMessageToClient(clientHd, L"System", EChatType::NOTICE, msg);
}
