#include "game.h"

#include <EAStdC/EAString.h>
#include <EAStdC/EAScanf.h>
#include <common/utils.h>

#include <mxm/game_content.h>
#include <mxm/hero_stats.h>
#include "config.h"

const CreatureIndex CI_DOOR = CreatureIndex(110040546);
const CreatureIndex CI_WALL = CreatureIndex(110042602);

// Titan Ruins gate docIDs (from Spawn.xml)
const CreatureIndex CI_GATE_BLUE_SIDE  = CreatureIndex(110043018); // Gate_Blue_Left, Gate_Blue_Right
const CreatureIndex CI_GATE_RED_SIDE   = CreatureIndex(110043019); // Gate_Red_Left, Gate_Red_Right
const CreatureIndex CI_GATE_BLUE_CTR   = CreatureIndex(110043020); // Gate_Blue_Center
const CreatureIndex CI_GATE_RED_CTR    = CreatureIndex(110043021); // Gate_Red_Center

// All gate/barrier docIDs from DYNAMIC.xml and Spawn.xml
const CreatureIndex CI_SPAWN_BARRIER  = CreatureIndex(110040547);  // spawn platform barriers
const CreatureIndex CI_GATE_GOT_CTR_B = CreatureIndex(110043010);  // Got_Gt_CenterGate_Blue
const CreatureIndex CI_GATE_GOT_CTR_R = CreatureIndex(110043011);  // Got_Gt_CenterGate_Red
const CreatureIndex CI_GATE_NORTH_B   = CreatureIndex(110043012);  // Got_Gt_NorthGate_Blue
const CreatureIndex CI_GATE_NORTH_R   = CreatureIndex(110043013);  // Got_Gt_NorthGate_Red
const CreatureIndex CI_GATE_SOUTH_B   = CreatureIndex(110043014);  // Got_Gt_SouthGate_Blue
const CreatureIndex CI_GATE_SOUTH_R   = CreatureIndex(110043015);  // Got_Gt_SouthGate_Red
const CreatureIndex CI_GATE_SMALL_B   = CreatureIndex(110043016);  // Got_Gt_SmallGate_Blue
const CreatureIndex CI_GATE_SMALL_R   = CreatureIndex(110043017);  // Got_Gt_SmallGate_Red

static bool IsGateCreature(CreatureIndex docID) {
	i32 id = (i32)docID;
	// Gate types only: 110043010-110043021 (NOT spawn barriers 110040547)
	return (id >= 110043010 && id <= 110043021);
}

// Speed buff applied at game start (STATUS_TYPE_FASTRUN, 30% speed, 10s)
const i32 STATUS_GAME_START_SPEED = 1230001010;

// Gate open action — use our enum value, replication subtracts 1 for client
const ActionStateID GATE_ACTION_OPEN = ActionStateID::DYNAMIC_OPEN;

// Forward declare lane waypoint helper (defined before UpdateBotAI)
static const vec2* GetLaneWaypoints(Game::Lane lane, i32* outCount);

// Per-hero auto-attack REMOTE IDs from REMOTE_PC.xml (ALL heroes)
static i32 GetAttackRemoteID(ClassType classType)
{
	switch(classType) {
		case ClassType::STRIKER:      return 1430010010; // REMOTE_STRIKER_RIFLE_DEFAULT_REMOTE
		case ClassType::ARTILLERY:    return 1430020010; // REMOTE_ARTILLERY_CANNON_DEFAULT_REMOTE
		case ClassType::ASSASSIN:     return 1430030010; // REMOTE_ASSASSIN_BLADE_DEFAULT_COMBO1
		case ClassType::ELECTRO:      return 1430040010; // REMOTE_ELECTRO_ESSENCE_DEFAULT_CHAIN1
		case ClassType::DEFENDER:     return 1430050010; // REMOTE_DEFENDER_SOULSTONE_DEFAULT_REMOTE
		case ClassType::SNIPER:       return 1430060010; // REMOTE_SNIPER_BARRETT_DEFAULT_CHARGE1
		case ClassType::DEATHKNIGHT:  return 1430070010; // REMOTE_DEATHKNIGHT_SWORD_DEFAULT_RIGHT
		case ClassType::DESTROYER:    return 1430080010; // REMOTE_DESTROYER_WARHAMMER_DEFAULT
		case ClassType::MECHANIC:     return 1430090010; // REMOTE_MECHANIC_PLAZMAGUN_DEFAULT_CHARGE1_HOST
		case ClassType::SOULMASTER:   return 1430100000; // REMOTE_SOULMASTER_SPIRIT_DEFAULT_REMOTE
		case ClassType::DOUBLEGUN:    return 1430110010; // REMOTE_DOUBLEGUN_DOUBLESHOT_DEFAULT
		case ClassType::JINSEOYEON:   return 1430120010; // REMOTE_JINSEOYEON_SWORD_DEFAULT_COMBO1
		case ClassType::KROMEDE:      return 1430130010; // REMOTE_KROMEDE_HUNTERBOW_DEFAULT_CHARGE1
		case ClassType::RODMASTER:    return 1430140010; // REMOTE_RODMASTER_STICK_DEFAULT_REMOTE1
		case ClassType::PHOTOG:       return 1430150010; // REMOTE_PHOTOG_CAMERA_DEFAULT_REMOTE
		case ClassType::ICEQUEEN:     return 1430160010; // REMOTE_ICEQUEEN_ICE_DEFAULT_REMOTE
		case ClassType::MAGICGIRL:    return 1430170010; // REMOTE_MAGICGIRL_ATTACK_DEFAULT
		case ClassType::POHWARAN:     return 1430180010; // REMOTE_POHWARAN_MINIGUN_DAMAGE_DEFAULT
		case ClassType::BOOMERANG:    return 1430190010; // REMOTE_BOOMERANG_BOLEADORAS_DEFAULT_REMOTE1
		case ClassType::SLIME:        return 1430200010; // REMOTE_SLIME_ATTACK_DEFAULT_REMOTE
		case ClassType::MONDOZAX:     return 1430210000; // REMOTE_MONDOZAX_ATTACK_DEFAULT
		case ClassType::LILU:         return 1430220010; // REMOTE_LILU_ATTACK_DEFAULT
		case ClassType::EFREET:       return 1430230010; // REMOTE_EFREET_ATTACK_DEFAULT_CHAIN1
		case ClassType::SHADOWHUNTER: return 1430240010; // REMOTE_SHADOWHUNTER_ATTACK_DEFAULT_COMBO1
		case ClassType::RYTLOCK:      return 1430250010; // REMOTE_RYTLOCK_BLADE_DEFAULT_COMBO1
		case ClassType::CATTHECAT:    return 1430260010; // REMOTE_CATTHECAT_ATTACK_CAT
		case ClassType::RNB:          return 1430270010; // REMOTE_RNB_ATTACK_DEFAULT_RANGE
		case ClassType::BATTER:       return 1430280000; // REMOTE_BATTER_ATTACK_DEFAULT_CHARGE0
		case ClassType::ANDROA:       return 1430290010; // REMOTE_ANDROIDA_ATTACK_DEFAULT_CENTER
		case ClassType::ANDROB:       return 1430300000; // REMOTE_ANDROIDB_ATTACK_DEFAULT1
		case ClassType::BOXER:        return 1430320001; // REMOTE_BOXER_ATTACK_DEFAULT_REMOTE_LEFT
		case ClassType::SHUGOTRADER:  return 1430330000; // REMOTE_SHUGOTRADER_ATTACK_DEFAULT_REMOTE
		case ClassType::NAGA:         return 1430340000; // REMOTE_NAGA_ATTACK_DEFAULT_COMBO1
		case ClassType::LAUNCHER:     return 1430350010; // REMOTE_LAUNCHER_ATTACK_DEFAULT
		case ClassType::ATTACKER:     return 1430310000; // REMOTE_ATTACKKOOM_SOULSTONE_DEFAULT_REMOTE (Taejin/Jingtai)
		case ClassType::STATESMAN:    return 1430360010; // REMOTE_STATESMAN_PUNCH_DEFAULT_COMBO1
		case ClassType::YURI:         return 1430370001; // REMOTE_YURI_ATTACK_DEFAULT_REMOTE_LEFT
		case ClassType::ESPER:        return 1430380010; // REMOTE_ESPER_ATTACK_DEFAULT
		case ClassType::PRIEST:       return 1430390010; // REMOTE_PRIEST_ATTACK_DEFAULT_RANGE
		case ClassType::GHOSTWIDOW:   return 1430400010; // REMOTE_GHOSTWIDOW_ATTACK_DEFAULT_REMOTE
		default: return 0;
	}
}

i32 Game::npcLocalID = 200;

void Game::LoadHeightmap()
{
	FILE* f = fopen("titan_ruins_heightmap.bin", "rb");
	if(!f) { LOG("[Heightmap] No cached file found"); return; }
	fread(heightmap, sizeof(heightmap), 1, f);
	fread(heightmapSet, sizeof(heightmapSet), 1, f);
	fclose(f);
	i32 count = 0;
	for(i32 x = 0; x < HMAP_W; x++)
		for(i32 y = 0; y < HMAP_H; y++)
			if(heightmapSet[x][y]) count++;
	LOG("[Heightmap] Loaded: %d/%d cells (%.0f%%)", count, HMAP_W*HMAP_H, 100.f*count/(HMAP_W*HMAP_H));
}

void Game::SaveHeightmap()
{
	FILE* f = fopen("titan_ruins_heightmap.bin", "wb");
	if(!f) { LOG("[Heightmap] Failed to save"); return; }
	fwrite(heightmap, sizeof(heightmap), 1, f);
	fwrite(heightmapSet, sizeof(heightmapSet), 1, f);
	fclose(f);
	i32 count = 0;
	for(i32 x = 0; x < HMAP_W; x++)
		for(i32 y = 0; y < HMAP_H; y++)
			if(heightmapSet[x][y]) count++;
	LOG("[Heightmap] Saved: %d/%d cells", count, HMAP_W*HMAP_H);
	heightmapDirty = false;
}

void Game::Init(Server* server_, const In::MQ_CreateGame& gameInfo, const eastl::array<ClientHandle, MAX_PLAYERS>& playerClientHdList)
{
	replication.Init(server_);
	world.Init(&replication);

	mapIndex = gameInfo.mapIndex;
	replication.mapIndex = mapIndex;
	LOG("[Game::Init] mapIndex=%d (%s)", (i32)mapIndex,
		mapIndex == MapIndex::PVP_TITAN_RUINS ? "TitanRuins" : "DeathMatch");

	LoadMap();
	if(IsTitanRuins()) LoadHeightmap();

	LOG("[Game::Init] Spawn points: RED=%d, BLUE=%d", (i32)mapSpawnPoints[0].size(), (i32)mapSpawnPoints[1].size());

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
		if(spawnPoints.empty()) {
			LOG("WARNING: No spawn points for team %d on map %d, using default pos", p.team, (i32)mapIndex);
			continue;
		}
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
			// Assign lane for Titan Ruins (distribute: top/mid/bot)
			if(IsTitanRuins()) {
				Bot& newBot = botList.back();
				i32 botIdx = (i32)botList.size() - 1;
				static const Lane lanes[] = { Lane::TOP, Lane::MID, Lane::BOT, Lane::TOP, Lane::MID };
				newBot.lane = lanes[botIdx % 5];
				if(worldPlayer.team == 1) {
					i32 wpCount = 0;
					GetLaneWaypoints(newBot.lane, &wpCount);
					newBot.waypointIdx = wpCount - 1;
				}
				LOG("[BOT] %S assigned to %s lane", worldPlayer.name.data(),
					newBot.lane == Lane::TOP ? "TOP" : newBot.lane == Lane::MID ? "MID" : "BOT");
			}
		}
	}

	dbgGameUID = Dbg::PushNewGame("PVP_DeathMatch");
}

void Game::Cleanup()
{
	if(IsTitanRuins() && heightmapDirty) SaveHeightmap();
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
				// 15s pregame, then 5-4-3-2-1 countdown in the last 2.5s, then barriers open
				i32 preGameMs = IsTitanRuins() ? 15000 : 10000;
				phaseTime = TimeAdd(localTime, TimeMsToTime(preGameMs));

				foreach_const(p, playerList) {
					replication.SendPreGameLevelEvents(p->clientHd);
				}

				// Start countdown 2.5s before game starts (5 packets at 500ms)
				countdownSent = 0;
				tNextCountdown = TimeAdd(localTime, TimeMsToTime(preGameMs - 2500));
			}
		} break;

		case Phase::PreGame: {
			// Send countdown packets (official sends curCount 0-4 at ~500ms intervals)
			if(countdownSent < 5 && localTime > tNextCountdown) {
				foreach_const(p, playerList) {
					replication.SendCountdown(p->clientHd, countdownSent, 5);
				}
				countdownSent++;
				tNextCountdown = TimeAdd(tNextCountdown, TimeMsToTime(500));
			}

			if(phaseTime < localTime) {
				phase = Phase::Game;
				matchStartTime = localTime;

				// Open DM doors
				foreach_mut(it, world.actorDynamicList) {
					if(it->docID == CI_DOOR) {
						it->action = ActionStateID::DYNAMIC_OPEN;
						LOG("[Game] Door opened: docID=%d UID=%u", (i32)it->docID, (u32)it->UID);
					}
				}

				// Titan Ruins: open gates at game start
				// Gates (110043010-110043021) get DYNAMIC_OPEN then destroyed after 1s
				// Spawn barriers (110040547) are kept alive — used as speed buff triggers
				if(IsTitanRuins()) {
					// Set action state — frame diff in replication sends SN_ActionChangeLevelEvent
					foreach_mut(it, world.actorDynamicList) {
						if(IsGateCreature(it->docID) || it->docID == CI_SPAWN_BARRIER) {
							it->action = ActionStateID::DYNAMIC_OPEN;
							LOG("[Game] Gate/barrier opening: docID=%d UID=%u", (i32)it->docID, (u32)it->UID);
						}
					}
					gateDestroyTime = TimeAdd(localTime, TimeMsToTime(1000));
				}

				// SendGameStart sends event 838 + SN_GAME_START + level events
				// (gate removal is handled client-side via events, not SN_ActionChangeLevelEvent)
				foreach_const(p, playerList) {
					if(p->clientHd == ClientHandle::INVALID) continue; // skip bots
					replication.SendGameStart(p->clientHd);
				}

				// Send initial speed
				foreach(player, world.players) {
					replication.SendChangeBattleState(player->Main().UID, false, player->moveSpeed);
				}

				// Titan Ruins: reset speed pad state
				if(IsTitanRuins()) {
					foreach(player, world.players) {
						player->inSpeedPad = 0;
					}
				}

				// Enemy info for death screen — DISABLED (breaks tag swap)
				// TODO: find alternative way to send enemy data without breaking swap
				// replication.SendNotifyPcDetailInfosEnemyOnly();

				// Resource corrections — only add correct type, don't clear defaults
				foreach(player, world.players) {
					i32 mainResType = player->resourceStatType;
					if(mainResType != 35 && mainResType != 0) {
						replication.SendUpdateStatToAll(player->Main().UID, (u8)mainResType, player->maxMana, player->mana);
					}
					i32 subResType = player->heroState[player->mainCharaID ^ 1].resourceStatType;
					f32 subMaxMana = player->heroState[player->mainCharaID ^ 1].maxMana;
					f32 subMana = player->heroState[player->mainCharaID ^ 1].mana;
					if(subResType != 17 && subResType != 0) {
						replication.SendUpdateStatToAll(player->Sub().UID, (u8)subResType, subMaxMana, subMana);
					}
				}

				// Initialize pickup spots
				InitPickupSpots();
			}
		} break;

		case Phase::Game: {
			// Bot AI
			foreach(bot, botList) {
				UpdateBotAI(*bot);
			}

			// Tower auto-attack AI
			if(IsTitanRuins()) {
				UpdateTowerAI();
			}

			// Player regen and UG (once per second)
			foreach(player, world.players) {
				if(!player->isDead) {
					UpdateRegen(*player);
					UpdateUltimateGauge(*player);
				}
			}

			// Titan Ruins: speed buff walls — one-time trigger per player per pad (human only)
			if(IsTitanRuins()) {
				foreach(player, world.players) {
					if(player->isDead || player->IsBot()) continue;
					vec3 pp = player->body->GetWorldPos();
					for(i32 si = 0; si < (i32)speedPads.size(); si++) {
						SpeedPad& pad = speedPads[si];
						if((player->inSpeedPad >> si) & 1) continue; // already triggered
						f32 dx = pp.x - pad.pos.x;
						f32 dy = pp.y - pad.pos.y;
						f32 distSq = dx*dx + dy*dy;
						if(distSq < pad.radius * pad.radius) {
							// Trigger speed buff (one-time per player)
							player->inSpeedPad |= (1 << si);
							world.AddStatus(*player, STATUS_GAME_START_SPEED, ActorUID::INVALID, 10.0f);
							LOG("[Game] Speed pad %d: player=%d pos=(%.0f,%.0f)", si, player->index, pp.x, pp.y);
						}
					}
				}
			}

			// (barrier green effect cycling removed — was re-closing barriers after game start)

			// Tag cooldown expiry (server-side only — client handles countdown UI internally)
			foreach(player, world.players) {
				if(player->tagCooldownActive && localTime >= player->tagCooldownUntil) {
					player->tagCooldownActive = false;
					// Do NOT send 62114 reset — client timer handles it internally
					// Sending 62114 broadcast can interfere with other players' countdowns
				}
			}

			// Titan's Sight capture (requires F key press each tick)
			if(IsTitanRuins()) {
				UpdateSights();
				// Reset interacting flag — must be refreshed by CQ_PlayerCastSkill every ~300ms
				foreach_mut(player, world.players) {
					// Only reset if the interaction is older than 400ms (allow some tolerance for packet timing)
					if(player->interacting && TimeDurationMs(player->tInteractStart, localTime) > 400) {
						player->interacting = false;
					}
				}
			}

			// Respawn check
			foreach(player, world.players) {
				if(player->isDead && player->tRespawnTime < localTime && player->tRespawnTime != Time::ZERO) {
					RespawnPlayer(*player);
				}
			}

			// Delayed gate + barrier destroy (after DYNAMIC_OPEN animation plays)
			if(gateDestroyTime != Time::ZERO && localTime > gateDestroyTime) {
				gateDestroyTime = Time::ZERO;
				eastl::fixed_vector<ActorUID, 32, false> toDestroy;
				foreach(it, world.actorDynamicList) {
					if(IsGateCreature(it->docID) || it->docID == CI_SPAWN_BARRIER) {
						toDestroy.push_back(it->UID);
					}
				}
				foreach(uid, toDestroy) {
					world.DestroyDynamic(*uid);
				}
				LOG("[Game] Gates destroyed: %d (after animation)", (i32)toDestroy.size());
			}

			// Update pickups
			UpdatePickups();

			// Match timer check
			if(matchStartTime != Time::ZERO) {
				f64 elapsed = TimeDurationSec(matchStartTime, localTime);
				f64 timeLimit = IsTitanRuins() ? (f64)TITAN_RUINS_MATCH_TIME_SEC : (f64)MATCH_TIME_SEC;
				if(elapsed >= timeLimit && !matchEnded) {
					if(IsTitanRuins()) {
						// Titan Ruins timeout: team with more towers alive wins;
						// if tied, team with more kills wins; if still tied, red wins
						i32 winTeam;
						if(towersAlive[0] != towersAlive[1]) {
							winTeam = (towersAlive[0] > towersAlive[1]) ? 0 : 1;
						} else {
							winTeam = (teamKills[0] >= teamKills[1]) ? 0 : 1;
						}
						TriggerGameOver(winTeam);
					} else {
						// DeathMatch: team with more kills wins
						i32 winTeam = (teamKills[0] >= teamKills[1]) ? 0 : 1;
						TriggerGameOver(winTeam);
					}
				}
			}
		} break;

		case Phase::GameOver: {
			// Auto-disconnect after 20s
			if(matchEndTime != Time::ZERO && TimeDurationSec(matchEndTime, localTime) > 20.0) {
				// Players should have returned to bridge by now
				matchEndTime = Time::ZERO; // prevent re-triggering
			}
		} break;
	}

	world.Update(localTime);

	// Tick status effects (buff/debuff durations, DoT, HoT)
	if(phase == Phase::Game) {
		world.TickStatuses((f32)UPDATE_RATE);
	}

	// Process pending status applications from ExecuteSkillProgram
	foreach(ps, world.pendingStatuses) {
		World::Player& target = world.GetPlayer(ps->targetIndex);
		if(!target.isDead) {
			if(ps->isRemove) {
				world.RemoveStatus(target, ps->statusID, ps->casterUID);
			} else {
				world.AddStatus(target, ps->statusID, ps->casterUID);
			}
		}
	}
	world.pendingStatuses.clear();

	// Process pending skill damage from ExecuteSkillProgram
	foreach(pd, world.pendingSkillDamage) {
		World::Player& attacker = world.GetPlayer(pd->attackerIndex);
		World::Player& target = world.GetPlayer(pd->targetIndex);
		if(!target.isDead && !attacker.isDead) {
			ApplyDamage(attacker, target, pd->damage, pd->skillID, pd->statusID);
		}
	}
	world.pendingSkillDamage.clear();

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
		Dbg::Dynamic n;
		n.UID = (u32)dyn->UID;
		n.docID = dyn->docID;
		n.pos = dyn->pos;
		n.rot = dyn->rot;
		Dbg::Push(dbgGameUID, n);
	}

	Dbg::PushPhysics(dbgGameUID, world.physics);

	replication.FrameEnd();
}

bool Game::LoadMap()
{
	const GameXmlContent& content = GetGameXmlContent();
	const GameXmlContent::Map* mapData = nullptr;

	if(mapIndex == MapIndex::PVP_TITAN_RUINS) {
		LOG("[LoadMap] Loading Titan Ruins (GoT)");
		world.physics.CreateStaticCollider("PvP_Titan_Ruins_Collision", vec3(0, 0, -100));
		// Disable gravity for Titan Ruins (flat collision plane, no real terrain mesh)
		world.physics.DisableGravity();
		mapData = &content.mapPvpTitanRuins;
	}
	else {
		LOG("[LoadMap] Loading DeathMatch");
		world.physics.CreateStaticCollider("PVP_DeathMatch01_Collision", vec3(0));
		world.physics.CreateStaticCollider("PVP_Deathmatch01_GuardrailMob", vec3(0));
		mapData = &content.mapPvpDeathMatch;
	}

	foreach(it, mapData->creatures) {
		// Player spawn points: dwDoc=100000001 with RED/BLUE team
		if((i32)it->docID == 100000001 && it->faction != Faction::INVALID && it->faction != Faction::DYNAMIC) {
			mapSpawnPoints[(i32)it->faction].push_back(SpawnPoint{ it->pos, it->rot });
			continue;
		}

		// spawn npc
		World::ActorNpc& actor = world.SpawnNpcActor(it->docID, it->localID);
		actor.pos = it->pos;
		actor.rot = it->rot;
		actor.faction = it->faction;

		// Titan Ruins: configure towers and cores
		if(IsTitanRuins()) {
			u8 team = (it->faction == Faction::RED) ? 0 : 1;

			if(IsTowerCreature(it->docID)) {
				actor.hasHP = true;
				actor.hp = 15000;
				actor.maxHp = 15000;

				TowerInfo ti;
				ti.uid = actor.UID;
				ti.team = team;
				ti.destroyed = false;
				towers.push_back(ti);
				towersAlive[team]++;

				LOG("[TitanRuins] Tower spawned: UID=%u docID=%d team=%d pos=(%.0f,%.0f,%.0f)",
					(u32)actor.UID, (i32)it->docID, team, it->pos.x, it->pos.y, it->pos.z);
			}
			else if(IsCoreCreature(it->docID)) {
				actor.hasHP = true;
				actor.hp = 50000;
				actor.maxHp = 50000;

				cores[team].uid = actor.UID;
				cores[team].team = team;
				cores[team].invulnerable = true;

				LOG("[TitanRuins] Core spawned: UID=%u docID=%d team=%d pos=(%.0f,%.0f,%.0f)",
					(u32)actor.UID, (i32)it->docID, team, it->pos.x, it->pos.y, it->pos.z);
			}
		}
	}

	foreach(it, mapData->dynamic) {
		auto& actor = world.SpawnDynamic(it->docID, it->localID);
		actor.pos = it->pos;
		actor.rot = it->rot;
		actor.faction = it->faction;

		// Track Titan's Sight buildings
		if(IsTitanRuins() && it->docID == CI_SIGHT_TOTEM) {
			SightInfo si;
			si.uid = actor.UID;
			si.pos = it->pos;
			sights.push_back(si);
			LOG("[TitanRuins] Sight totem spawned: UID=%u pos=(%.0f,%.0f,%.0f)",
				(u32)actor.UID, it->pos.x, it->pos.y, it->pos.z);
		}
	}

	// Titan Ruins: spawn extra towers to reach 8 per team
	// Spawn.xml provides 2 outer towers per team (top/bot lane at ~7335/24166)
	// We add 6 more per team: 2 core, 3 lane exits, 1 mid
	// Positions from in-game observation (!stats). Blue = mirrored (center X ≈ 15752)
	if(IsTitanRuins()) {
		struct ExtraTower { CreatureIndex docID; vec3 pos; vec3 rot; u8 team; };
		const f32 Z = 70.85f;    // lane tower height from Spawn.xml
		const f32 ZC = 341.59f;  // core area tower height (same as cores in Spawn.xml)
		const ExtraTower extraTowers[] = {
			// === RED TEAM (team 0) ===
			// Core towers (flanking Red core at X~3850, higher terrain)
			{ CreatureIndex(105000308), vec3(4851.0f, 14206.0f, ZC), vec3(0,0,-1.570796f), 0 },
			{ CreatureIndex(105000308), vec3(4833.0f, 15358.0f, ZC), vec3(0,0,-1.570796f), 0 },
			// Lane exit towers (past outer towers, at lane entrances)
			{ CreatureIndex(105000308), vec3(9274.0f, 11015.0f, Z), vec3(0,0,-1.570796f), 0 },  // bot
			{ CreatureIndex(105000308), vec3(9597.0f, 14797.0f, Z), vec3(0,0,-1.570796f), 0 },  // mid
			{ CreatureIndex(105000308), vec3(9298.0f, 18530.0f, Z), vec3(0,0,-1.570796f), 0 },  // top
			// Mid tower (forward)
			{ CreatureIndex(105000308), vec3(12752.0f, 15736.0f, Z), vec3(0,0,-1.570796f), 0 },

			// === BLUE TEAM (team 1) — mirrored ===
			// Core towers (flanking Blue core at X~27654, higher terrain)
			{ CreatureIndex(105000308), vec3(26653.0f, 14206.0f, ZC), vec3(0,0,1.570796f), 1 },
			{ CreatureIndex(105000308), vec3(26671.0f, 15358.0f, ZC), vec3(0,0,1.570796f), 1 },
			// Lane exit towers
			{ CreatureIndex(105000308), vec3(22230.0f, 11015.0f, Z), vec3(0,0,1.570796f), 1 },  // bot
			{ CreatureIndex(105000308), vec3(21907.0f, 14797.0f, Z), vec3(0,0,1.570796f), 1 },  // mid
			{ CreatureIndex(105000308), vec3(22206.0f, 18530.0f, Z), vec3(0,0,1.570796f), 1 },  // top
			// Mid tower (forward)
			{ CreatureIndex(105000308), vec3(18741.0f, 13816.0f, Z), vec3(0,0,1.570796f), 1 },
		};

		for(const auto& it : extraTowers) {
			i32 localID = npcLocalID++;
			World::ActorNpc& actor = world.SpawnNpcActor(it.docID, localID);
			actor.pos = it.pos;
			actor.rot = it.rot;
			actor.faction = (it.team == 0) ? Faction::RED : Faction::BLUE;
			actor.hasHP = true;
			actor.hp = 15000;
			actor.maxHp = 15000;

			TowerInfo ti;
			ti.uid = actor.UID;
			ti.team = it.team;
			ti.destroyed = false;
			towers.push_back(ti);
			towersAlive[it.team]++;

			LOG("[TitanRuins] Extra tower spawned: UID=%u team=%d faction=%d pos=(%.0f,%.0f,%.0f)",
				(u32)actor.UID, it.team, (i32)actor.faction, it.pos.x, it.pos.y, it.pos.z);
		}
	}

	// Titan Ruins: log tower/core summary
	if(IsTitanRuins()) {
		LOG("[TitanRuins] Towers: RED=%d, BLUE=%d | Cores: RED=%s, BLUE=%s",
			towersAlive[0], towersAlive[1],
			(cores[0].uid != ActorUID::INVALID) ? "OK" : "MISSING",
			(cores[1].uid != ActorUID::INVALID) ? "OK" : "MISSING");
	}

	// Titan Ruins: speed buff walls next to cores (one-time trigger when passing through)
	// Link each speed pad to the nearest spawn barrier (110040547) for animation
	if(IsTitanRuins()) {
		const f32 R = 600.0f;
		// Red base exits (next to Red core at ~3850,14776)
		speedPads.push_back(SpeedPad{ vec3(4066.0f, 15872.0f, 613.0f), R });
		speedPads.push_back(SpeedPad{ vec3(4073.0f, 13675.0f, 613.0f), R });
		// Blue base exits (mirrored, next to Blue core at ~27654,14782)
		speedPads.push_back(SpeedPad{ vec3(27438.0f, 15872.0f, 613.0f), R });
		speedPads.push_back(SpeedPad{ vec3(27431.0f, 13675.0f, 613.0f), R });

		LOG("[TitanRuins] Speed pads initialized: %d zones", (i32)speedPads.size());
	}

	// DeathMatch-specific: spawn walls dynamically
	if(mapIndex == MapIndex::PVP_DEATHMATCH) {
		// NOTE: Found in "Data/Design/Level/PVP/PVP_DeathMatch/EVENTNODES/LEVELEVENT_SERVER.XML"
		const eastl::fixed_set<i32,12,false> configurations[] = {
			{ 179, 178, 177, 176, 175, 174, 173, 172, 171, 170 }, // 10 A type
			{ 40, 41, 45, 46, 39, 48, 37, 49, 36, 38, 51, 50 }, // 12 A type
			{ 39, 48, 45, 40, 37, 49, 38, 50 }, // 8 A type
			{ 39, 45, 46, 49, 36, 51 }, // 6 B type
			{ 40, 45, 48, 37, 38, 50 }, // 6 A type
			{ 48, 37, 41, 51 }, // 4 C type
			{ 48, 37, 45, 38 }, // 4 B type
			{ 51, 41, 48, 36 }, // 4 A type
		};

		const eastl::array<const char*, ARRAY_COUNT(configurations)> configurationName = {
			"10 A type",
			"12 A type",
			"8 A type",
			"6 B type",
			"6 A type",
			"4 C type",
			"4 B type",
			"4 A type",
		};

		const i32 chosenConf = RandInt(0, ARRAY_COUNT(configurations)-1);
		const auto& wallConf = configurations[chosenConf];
		const char* wallConfName = configurationName[chosenConf];

		LOG("LoadMap> chosen wall configuration = '%s'", wallConfName);

		foreach(it, mapData->areas) {
			if(wallConf.find(it->ID) != wallConf.end()) {
				auto& actor = world.SpawnDynamic(CI_WALL, it->ID);
				actor.pos = it->pos;
				actor.rot = it->rot;
				world.physics.CreateStaticCollider("PvP_Death_NM_Wall04_GuardrailMob", it->pos, it->rot);
			}
		}
	}

	LOG("[LoadMap] Spawn points: RED=%d, BLUE=%d",
		(i32)mapSpawnPoints[(i32)Faction::RED].size(),
		(i32)mapSpawnPoints[(i32)Faction::BLUE].size());

	return true;
}

void Game::OnPlayerDisconnect(ClientHandle clientHd)
{
	auto found = playerMap.find(clientHd);
	if(found == playerMap.end()) return;
	Player& player = *found->second;

	World::Player& wpl = world.GetPlayer(player.playerIndex);
	wpl.disconnected = true;

	replication.OnPlayerDisconnect(clientHd);
}

void Game::OnPlayerReadyToLoad(ClientHandle clientHd)
{
	replication.SendLoadPvpMap(clientHd, mapIndex);
}

void Game::OnPlayerGetCharacterInfo(ClientHandle clientHd, ActorUID actorUID)
{
	auto found = playerMap.find(clientHd);
	if(found == playerMap.end()) return;
	Player& p = *found->second;
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

void Game::OnPlayerUpdatePosition(ClientHandle clientHd, ActorUID actorUID, const vec3& pos, const vec2& dir, const RotationHumanoid& rot, f32 speed, ActionStateID action, f32 clientTime)
{
	ProfileFunction();

	// Block movement during PreGame (barriers are up)
	if(phase == Phase::PreGame || phase == Phase::WaitingForReady || phase == Phase::WaitingForFirstPlayer) return;

	auto found = playerMap.find(clientHd);
	if(found == playerMap.end()) return;
	Player& p = *found->second;
	World::Player& player = world.GetPlayer(p.playerIndex);
	ASSERT(player.clientHd == clientHd);

	bool actorFound = false;
	foreach_const(chit, player.characters) {
		const World::ActorMaster& chara = **chit;
		if(chara.UID == actorUID) {
			actorFound = true;
			break;
		}
	}

	// NOTE: the client is not aware that we spawned a new actor for them yet, we ignore this packet
	// LordSk (30/08/2020)
	if(!actorFound) {
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

	// Titan Ruins (noGravity): accept client Z and record heightmap
	if(IsTitanRuins()) {
		player.body->collider->setFootPosition(PxExtendedVec3(
			player.body->GetWorldPos().x, player.body->GetWorldPos().y, pos.z));
		// Sample heightmap
		i32 gx = (i32)(pos.x / HMAP_CELL);
		i32 gy = (i32)(pos.y / HMAP_CELL);
		if(gx >= 0 && gx < HMAP_W && gy >= 0 && gy < HMAP_H) {
			if(!heightmapSet[gx][gy]) heightmapDirty = true;
			heightmap[gx][gy] = pos.z;
			heightmapSet[gx][gy] = true;
		}
	}

	player.input.rot = rot;
	player.input.speed = speed;
}

void Game::OnPlayerUpdateRotation(ClientHandle clientHd, ActorUID actorUID, const RotationHumanoid& rot)
{
	auto found = playerMap.find(clientHd);
	if(found == playerMap.end()) return;
	Player& p = *found->second;
	World::Player& player = world.GetPlayer(p.playerIndex);
	ASSERT(player.clientHd == clientHd);

	bool actorFound2 = false;
	foreach_const(chit, player.characters) {
		const World::ActorMaster& chara = **chit;
		if(chara.UID == actorUID) {
			actorFound2 = true;
			break;
		}
	}

	if(!actorFound2) {
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

	auto found = playerMap.find(clientHd);
	if(found == playerMap.end()) return;
	Player& p = *found->second;
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

	auto found = playerMap.find(clientHd);
	if(found == playerMap.end()) return;
	Player& p = *found->second;
	replication.SendChatWhisperToClient(destPlayer->clientHd, p.name.data(), msg);
}

void Game::OnPlayerSetLeaderCharacter(ClientHandle clientHd, LocalActorID characterID, SkinIndex skinIndex)
{

}

void Game::OnPlayerSyncActionState(ClientHandle clientHd, ActorUID actorUID, ActionStateID state, i32 param1, i32 param2, f32 rotate, f32 upperRotate)
{
	auto found = playerMap.find(clientHd);
	if(found == playerMap.end()) return;
	Player& p = *found->second;
	World::Player& player = world.GetPlayer(p.playerIndex);
	ASSERT(player.clientHd == clientHd);

	bool actorFound3 = false;
	foreach_const(chit, player.characters) {
		const World::ActorMaster& chara = **chit;
		if(chara.UID == actorUID) {
			actorFound3 = true;
			break;
		}
	}

	// NOTE: the client is not aware that we spawned a new actor for them yet, we ignore this packet
	// LordSk (30/08/2020)
	if(!actorFound3) {
		WARN("Client sent an invalid actorUID (clientHd=%x actorUID=%u)", clientHd, (u32)actorUID);
		return;
	}

	// TODO: check hacking
	player.input.rot.bodyYaw = rotate;
	player.input.rot.upperYaw = upperRotate;
	player.input.action = state;
	player.input.actionParam1 = param1;
	player.input.actionParam2 = param2;
}

void Game::OnPlayerLoadingComplete(ClientHandle clientHd)
{

}

void Game::OnPlayerGameMapLoaded(ClientHandle clientHd)
{

}

void Game::OnPlayerTag(ClientHandle clientHd, ActorUID actorUID)
{
	auto found = playerMap.find(clientHd);
	if(found == playerMap.end()) return;
	Player& p = *found->second;
	World::Player& player = world.GetPlayer(p.playerIndex);
	ASSERT(player.clientHd == clientHd);

	// TODO: cooldown
	player.input.tag = 1;
}

void Game::OnPlayerJump(ClientHandle clientHd, ActorUID actorUID, f32 rotate, f32 moveDirX, f32 moveDirY)
{
	auto found = playerMap.find(clientHd);
	if(found == playerMap.end()) return;
	Player& p = *found->second;
	World::Player& player = world.GetPlayer(p.playerIndex);
	ASSERT(player.clientHd == clientHd);

	player.input.jump = 1;
}

void Game::OnPlayerCastSkill(ClientHandle clientHd, ActorUID actorUID, const PlayerInputCastSkill& cast, const Cl::CQ_PlayerCastSkill::PosStruct& posInfo)
{
	auto found = playerMap.find(clientHd);
	if(found == playerMap.end()) return;
	Player& p = *found->second;
	World::Player& player = world.GetPlayer(p.playerIndex);
	ASSERT(player.clientHd == clientHd);

	// Intercept IT_ACTION_OCCUPY (900000032) — Titan's Sight F key interaction
	if((i32)cast.skillID == 900000032) {
		player.interacting = true;
		player.tInteractStart = localTime;
		LOG("[SIGHT] Player %d pressing F (IT_ACTION_OCCUPY), interacting=true", (i32)player.index);
		return;
	}

	player.input.cast = cast;

	RotationHumanoid rot = { posInfo.rot.x, posInfo.rot.y, posInfo.rot.z };
	// TODO: convert clientTime to localTime
	OnPlayerUpdatePosition(clientHd, actorUID, f2v(posInfo.pos), f2v(posInfo.moveDir), rot, posInfo.speed, ActionStateID::INVALID, 0);

	LOG("OnPlayerCastSkill :: (%f, %f, %f)", cast.pos.x, cast.pos.y, cast.pos.z);
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
	// null terminate
	eastl::fixed_string<wchar,1024,true> msgBuff;
	msgBuff.assign(msg, len);
	msg = msgBuff.data();

	if(msg[0] != L'!') return false;
	msg++;

	auto found = playerMap.find(clientHd);
	if(found == playerMap.end()) return false;
	Player& p = *found->second;
	World::Player& wpl = world.GetPlayer(p.playerIndex);

	// --- Commands available to all players ---

	if(EA::StdC::Strcmp(msg, L"stats") == 0) {
		LOG("[CMD] %S used !stats", wpl.name.data());
		vec3 pos = wpl.body->GetWorldPos();
		SendDbgMsg(clientHd, LFMT(L"[Stats] HP:%.0f/%.0f ATK:%.0f DEF:%.0f Mana:%.0f/%.0f Range:%.0f Crit:%.1f%%",
			wpl.hp, wpl.maxHp, wpl.atk, wpl.defense, wpl.mana, wpl.maxMana, wpl.attackRange, wpl.critChance));
		SendDbgMsg(clientHd, LFMT(L"[Stats] Stamina:%.0f/%.0f UG:%.0f/%.0f K:%d D:%d Pos:(%.0f,%.0f,%.0f)",
			wpl.stamina, wpl.maxStamina, wpl.ultimateGauge, wpl.maxUltimateGauge,
			wpl.kills, wpl.deaths, pos.x, pos.y, pos.z));
		return true;
	}

	if(EA::StdC::Strcmp(msg, L"log") == 0) {
		wpl.combatLogEnabled = !wpl.combatLogEnabled;
		LOG("[CMD] %S toggled combat log: %s", wpl.name.data(), wpl.combatLogEnabled ? "ON" : "OFF");
		SendDbgMsg(clientHd, LFMT(L"Combat log: %s", wpl.combatLogEnabled ? L"ON" : L"OFF"));
		return true;
	}

	i32 hpVal = 0;
	if(EA::StdC::Sscanf(msg, L"hp %d", &hpVal) == 1) {
		LOG("[CMD] %S set HP to %d", wpl.name.data(), hpVal);
		wpl.hp = (f32)hpVal;
		if(wpl.hp > wpl.maxHp) wpl.hp = wpl.maxHp;
		if(wpl.hp < 0) wpl.hp = 0;
		replication.SendUpdateStatToAll(wpl.Main().UID, 0, wpl.maxHp, wpl.hp);
		SendDbgMsg(clientHd, LFMT(L"HP set to %.0f", wpl.hp));
		return true;
	}

	if(EA::StdC::Strcmp(msg, L"kill") == 0) {
		LOG("[CMD] %S used !kill", wpl.name.data());
		if(!wpl.isDead) {
			KillPlayer(wpl, wpl); // suicide
			SendDbgMsg(clientHd, L"You killed yourself.");
		}
		return true;
	}

	if(EA::StdC::Strcmp(msg, L"info") == 0) {
		LOG("[CMD] %S used !info", wpl.name.data());
		foreach_const(other, world.players) {
			SendDbgMsg(clientHd, LFMT(L"[%d] %S - HP:%.0f/%.0f Team:%d K:%d D:%d %s",
				other->index, other->name.data(), other->hp, other->maxHp, other->team,
				other->kills, other->deaths, other->isDead ? L"DEAD" : L"ALIVE"));
		}
		return true;
	}

	if(EA::StdC::Strcmp(msg, L"rtb") == 0) {
		if(phase == Phase::GameOver) {
			OnPlayerReturnToCity(clientHd);
		} else {
			SendDbgMsg(clientHd, L"Can only return to bridge after game over.");
		}
		return true;
	}

	i32 ugVal = 0;
	if(EA::StdC::Sscanf(msg, L"ug %d", &ugVal) == 1) {
		wpl.ultimateGauge = (f32)ugVal;
		if(wpl.ultimateGauge > wpl.maxUltimateGauge) wpl.ultimateGauge = wpl.maxUltimateGauge;
		replication.SendUpdateStatToAll(wpl.Main().UID, 37, wpl.maxUltimateGauge, wpl.ultimateGauge);
		SendDbgMsg(clientHd, LFMT(L"UG set to %.0f", wpl.ultimateGauge));
		return true;
	}

	// Buff/Debuff debug commands
	if(EA::StdC::Strcmp(msg, L"buffs") == 0) {
		LOG("[CMD] %S used !buffs", wpl.name.data());
		if(wpl.activeStatuses.empty()) {
			SendDbgMsg(clientHd, L"No active statuses.");
		} else {
			foreach_const(s, wpl.activeStatuses) {
				SendDbgMsg(clientHd, LFMT(L"[%d] type=%d dur=%.1f/%.1fs stacks=%d",
					s->statusID, s->def ? (i32)s->def->type : -1, s->elapsedTime, s->durationTime, s->overlapCount));
			}
		}
		return true;
	}

	i32 debuffID = 0;
	if(EA::StdC::Sscanf(msg, L"debuff %d", &debuffID) == 1) {
		LOG("[CMD] %S applied debuff %d", wpl.name.data(), debuffID);
		world.AddStatus(wpl, debuffID, wpl.Main().UID);
		SendDbgMsg(clientHd, LFMT(L"Applied status %d", debuffID));
		return true;
	}

	if(EA::StdC::Strcmp(msg, L"debuff") == 0) {
		SendDbgMsg(clientHd, L"Usage: !debuff <name> or !debuff <id>");
		SendDbgMsg(clientHd, L"Names: stun slow burn bleed freeze root silence godmode superarmor shield fear airborne knockdown");
		return true;
	}

	// !debuff <name> — apply debuff by name
	{
		struct NamedDebuff { const wchar* name; i32 id; f32 dur; };
		static const NamedDebuff debuffs[] = {
			{ L"stun",       1230070050, 3.0f },   // DEATHKNIGHT_SHOCKSTUN (1s stun with VFX)
			{ L"slow",       1230010061, 5.0f },   // STRIKER_EP_SLOW
			{ L"burn",       1230010070, 5.0f },   // STRIKER_GRENADE_BURNING
			{ L"bleed",      1230030040, 5.0f },   // ASSASSIN_SKILL_BLEEDING
			{ L"freeze",     1230070050, 3.0f },   // Same as stun for now (freeze causes displacement)
			{ L"root",       1230080044, 3.0f },   // DESTROYER_ROOT (has VFX: STATUS_SFX_PC_Destroyer_Root)
			{ L"silence",    1230030061, 3.0f },   // ASSASSIN_CALTROP_SILENCE (has VFX)
			{ L"godmode",    1200000000, 10.0f },  // COMMAND_GODMODE
			{ L"superarmor", 1200005042, 10.0f },  // COMMANDSET_HGM_SUPERARMOR
			{ L"shield",     1230050020, 5.0f },   // DEFENDER_SHIRK_SHIELD
			{ L"fear",       1230080020, 3.0f },   // DESTROYER_FEARCRY (WARNING: displacement!)
			{ L"airborne",   1230080043, 2.0f },   // NETTHROW_AIRBORNE (WARNING: displacement!)
			{ L"knockdown",  1230010040, 2.0f },   // CANNONFIST_KNOCKDOWN (WARNING: displacement!)
		};

		wchar nameBuf[32] = {};
		if(EA::StdC::Sscanf(msg, L"debuff %31ls", nameBuf) == 1) {
			// Check if it's a number (already handled above)
			if(nameBuf[0] >= L'0' && nameBuf[0] <= L'9') { /* skip, handled by debuff %d */ }
			else {
				for(i32 i = 0; i < (i32)(sizeof(debuffs)/sizeof(debuffs[0])); i++) {
					if(EA::StdC::Strcmp(nameBuf, debuffs[i].name) == 0) {
						world.AddStatus(wpl, debuffs[i].id, wpl.Main().UID, debuffs[i].dur);
						SendDbgMsg(clientHd, LFMT(L"Applied %ls (ID=%d, %.0fs)", debuffs[i].name, debuffs[i].id, debuffs[i].dur));
						return true;
					}
				}
				SendDbgMsg(clientHd, LFMT(L"Unknown debuff: %ls", nameBuf));
				return true;
			}
		}
	}

	if(EA::StdC::Strcmp(msg, L"cleanse") == 0) {
		world.RemoveAllStatuses(wpl);
		SendDbgMsg(clientHd, L"All statuses cleared.");
		return true;
	}

	// !recall — teleport all ENEMIES to player + freeze indefinitely
	if(EA::StdC::Strcmp(msg, L"recall") == 0) {
		vec3 myPos = wpl.body->GetWorldPos();
		i32 count = 0;
		foreach(it, world.players) {
			World::Player& target = *it;
			if(target.index == wpl.index) continue;
			if(target.team == wpl.team) continue; // enemies only
			if(target.isDead) continue;

			// Teleport enemy near player (spread in a circle)
			f32 spreadAngle = (f32)count * 1.2f;
			vec3 offset = vec3(cosf(spreadAngle) * 400.0f, sinf(spreadAngle) * 400.0f, 0);
			vec3 tpPos = myPos + offset;
			target.body->collider->setFootPosition(PxExtendedVec3(tpPos.x, tpPos.y, tpPos.z));
			target.input.moveTo = tpPos;
			target.input.speed = 0;
			replication.SendPlayerSyncTeleport(target.Main().UID, tpPos, vec3(0));

			// Freeze indefinitely (999999s)
			world.AddStatus(target, 1230010012, wpl.Main().UID, 999999.0f);
			count++;
		}
		SendDbgMsg(clientHd, LFMT(L"Recalled %d enemies (frozen)", count));
		return true;
	}

	// !freeze — freeze ALL bots (enemies + allies, NOT self) indefinitely
	if(EA::StdC::Strcmp(msg, L"freeze") == 0) {
		i32 count = 0;
		foreach(it, world.players) {
			World::Player& target = *it;
			if(target.index == wpl.index) continue; // skip self
			if(target.isDead) continue;

			world.AddStatus(target, 1230010012, wpl.Main().UID, 999999.0f);
			target.input.speed = 0;
			target.input.moveTo = target.body->GetWorldPos();
			count++;
		}
		SendDbgMsg(clientHd, LFMT(L"Frozen %d bots (all teams)", count));
		return true;
	}

	// !spawn <ID> — spawn an NPC by creature ID at player position
	// !spawn <ID> <count> — spawn multiple in a circle
	{
		i32 spawnID = 0;
		i32 spawnCount = 1;
		if(EA::StdC::Sscanf(msg, L"spawn %d %d", &spawnID, &spawnCount) >= 1) {
			vec3 myPos = wpl.body->GetWorldPos();
			if(spawnCount < 1) spawnCount = 1;
			if(spawnCount > 16) spawnCount = 16;
			static i32 spawnLocalCounter = 60000;

			for(i32 i = 0; i < spawnCount; i++) {
				f32 angle = (spawnCount > 1) ? (f32)i * (6.2831853f / (f32)spawnCount) : 0;
				f32 radius = (spawnCount > 1) ? 500.0f : 300.0f;
				vec3 offset = vec3(cosf(angle) * radius, sinf(angle) * radius, 0);
				vec3 tpPos = myPos + offset;

				World::ActorNpc& npc = world.SpawnNpcActor((CreatureIndex)spawnID, spawnLocalCounter++);
				npc.pos = tpPos;
				npc.rot = vec3(0);
				npc.faction = Faction(3 + (1 - wpl.team)); // enemy faction
				npc.hasHP = true;
				npc.hp = 10000;
				npc.maxHp = 10000;

				LOG("[SPAWN] ID=%d at (%.0f,%.0f,%.0f) UID=%u localID=%d",
					spawnID, tpPos.x, tpPos.y, tpPos.z, (u32)npc.UID, npc.localID);
			}
			SendDbgMsg(clientHd, LFMT(L"Spawned %d x NPC %d", spawnCount, spawnID));
			return true;
		}
	}

	i32 removeID = 0;
	if(EA::StdC::Sscanf(msg, L"removebuff %d", &removeID) == 1) {
		world.RemoveStatus(wpl, removeID, ActorUID::INVALID);
		SendDbgMsg(clientHd, LFMT(L"Removed status %d", removeID));
		return true;
	}

	f32 speedVal = 0;
	if(EA::StdC::Sscanf(msg, L"speed %f", &speedVal) == 1) {
		replication.SendChangeBattleState(wpl.Main().UID, true, speedVal);
		SendDbgMsg(clientHd, LFMT(L"Speed set to %.0f", speedVal));
		return true;
	}

	if(EA::StdC::Strcmp(msg, L"towers") == 0 && IsTitanRuins()) {
		LOG("[CMD] %S used !towers", wpl.name.data());
		SendDbgMsg(clientHd, LFMT(L"[Towers] RED alive=%d, BLUE alive=%d", towersAlive[0], towersAlive[1]));
		SendDbgMsg(clientHd, LFMT(L"[Cores] RED %S, BLUE %S",
			cores[0].invulnerable ? L"INVULN" : L"VULNERABLE",
			cores[1].invulnerable ? L"INVULN" : L"VULNERABLE"));
		foreach(t, towers) {
			World::ActorNpc* npc = world.FindNpcActor(t->uid);
			if(npc) {
				SendDbgMsg(clientHd, LFMT(L"  Tower UID=%u team=%d HP=%.0f/%.0f %S",
					(u32)t->uid, t->team, npc->hp, npc->maxHp,
					t->destroyed ? L"DESTROYED" : L"ALIVE"));
			}
		}
		return true;
	}

	if(EA::StdC::Strcmp(msg, L"heightmap") == 0 && IsTitanRuins()) {
		i32 count = 0;
		for(i32 x = 0; x < HMAP_W; x++)
			for(i32 y = 0; y < HMAP_H; y++)
				if(heightmapSet[x][y]) count++;
		SaveHeightmap();
		SendDbgMsg(clientHd, LFMT(L"[Heightmap] %d/%d cells mapped (%.0f%%). Saved to disk.",
			count, HMAP_W*HMAP_H, 100.f*count/(HMAP_W*HMAP_H)));
		return true;
	}

	if(EA::StdC::Strcmp(msg, L"sight") == 0 && IsTitanRuins()) {
		LOG("[CMD] %S used !sight", wpl.name.data());
		SendDbgMsg(clientHd, LFMT(L"[Sight] %d totems on map", (i32)sights.size()));
		for(i32 i = 0; i < (i32)sights.size(); i++) {
			const auto& s = sights[i];
			const wchar* owner = (s.ownerTeam < 0) ? L"NEUTRAL" : (s.ownerTeam == 0) ? L"RED" : L"BLUE";
			SendDbgMsg(clientHd, LFMT(L"  Sight %d: UID=%u owner=%S capture=%.0f%% pos=(%.0f,%.0f)",
				i, (u32)s.uid, owner, s.captureProgress * 100.0f, s.pos.x, s.pos.y));
		}
		return true;
	}

	if(EA::StdC::Strcmp(msg, L"nearby") == 0) {
		vec3 myPos = wpl.body->GetWorldPos();
		LOG("[CMD] %S used !nearby at pos=(%.0f,%.0f,%.0f)", wpl.name.data(), myPos.x, myPos.y, myPos.z);
		SendDbgMsg(clientHd, LFMT(L"[Nearby] Your pos: (%.0f, %.0f, %.0f)", myPos.x, myPos.y, myPos.z));

		// Show nearby dynamic entities
		i32 count = 0;
		foreach(it, world.actorDynamicList) {
			f32 dist = glm::distance(vec2(myPos), vec2(it->pos));
			if(dist < 3000.0f) {
				SendDbgMsg(clientHd, LFMT(L"  DYN UID=%u doc=%d local=%d dist=%.0f pos=(%.0f,%.0f,%.0f) act=%d",
					(u32)it->UID, (i32)it->docID, it->localID, dist,
					it->pos.x, it->pos.y, it->pos.z, (i32)it->action));
				count++;
			}
		}

		// Show nearby NPC entities
		foreach(it, world.actorNpcList) {
			f32 dist = glm::distance(vec2(myPos), vec2(it->pos));
			if(dist < 3000.0f) {
				SendDbgMsg(clientHd, LFMT(L"  NPC UID=%u doc=%d local=%d dist=%.0f pos=(%.0f,%.0f,%.0f)",
					(u32)it->UID, (i32)it->docID, it->localID, dist,
					it->pos.x, it->pos.y, it->pos.z));
				count++;
			}
		}

		SendDbgMsg(clientHd, LFMT(L"[Nearby] Total: %d entities within 3000 units", count));
		return true;
	}

	// !destroyall <radius> — destroy all dynamic entities in radius
	{
		f32 radius = 0;
		if(EA::StdC::Sscanf(msg, L"destroyall %f", &radius) == 1 || EA::StdC::Strcmp(msg, L"destroyall") == 0) {
			if(radius <= 0) radius = 1000;
			vec3 myPos = wpl.body->GetWorldPos();
			i32 count = 0;
			foreach(it, world.actorDynamicList) {
				f32 dist = glm::distance(vec2(myPos), vec2(it->pos));
				if(dist < radius) {
					replication.DbgSendDestroy(clientHd, it->UID);
					SendDbgMsg(clientHd, LFMT(L"  DESTROYED doc=%d UID=%u dist=%.0f", (i32)it->docID, (u32)it->UID, dist));
					count++;
				}
			}
			SendDbgMsg(clientHd, LFMT(L"[DestroyAll] %d entities destroyed (radius=%.0f)", count, radius));
			return true;
		}
	}

	// !action <UID> <actionID> — send raw SN_ActionChangeLevelEvent to test gate actions
	{
		u32 uid = 0; i32 actID = 0;
		if(EA::StdC::Sscanf(msg, L"action %u %d", &uid, &actID) == 2) {
			LOG("[CMD] %S used !action UID=%u actionID=%d", wpl.name.data(), uid, actID);
			replication.DbgSendActionChange(clientHd, ActorUID(uid), actID);
			SendDbgMsg(clientHd, LFMT(L"Sent action %d to UID=%u", actID, uid));
			return true;
		}
	}

	// !destroy <UID> — send SN_DestroyEntity
	{
		u32 uid = 0;
		if(EA::StdC::Sscanf(msg, L"destroy %u", &uid) == 1) {
			LOG("[CMD] %S used !destroy UID=%u", wpl.name.data(), uid);
			replication.DbgSendDestroy(clientHd, ActorUID(uid));
			SendDbgMsg(clientHd, LFMT(L"Sent destroy for UID=%u", uid));
			return true;
		}
	}

	// --- Dev-only commands (require DevMode=1 in game.cfg) ---

	if(!Config().DevMode) return false;

	if(EA::StdC::Strcmp(msg, L"dummies") == 0) {
		dummyMode = !dummyMode;
		SendDbgMsg(clientHd, LFMT(L"Dummy mode: %s", dummyMode ? L"ON" : L"OFF"));
		return true;
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
	if(EA::StdC::Strcmp(msg, L"wall") == 0) {
		const vec3 pos = wpl.body->GetWorldPos();
		static i32 localID = 37;
		World::ActorDynamic& dyn = world.SpawnDynamic(CreatureIndex(110042602), localID++);
		dyn.pos = pos;
		SendDbgMsg(clientHd, LFMT(L"Wall spawned at (%g, %g, %g)", pos.x, pos.y, pos.z));
		return true;
	}

	return false;
}

void Game::SendDbgMsg(ClientHandle clientHd, const wchar* msg)
{
	replication.SendChatMessageToClient(clientHd, L"System", EChatType::NOTICE, msg);
}

// --- Combat system ---

void Game::ApplyDamage(World::Player& attacker, World::Player& victim, f32 damage, SkillID skillID, i32 statusID)
{
	if(victim.isDead) return;
	if(victim.team == attacker.team) return; // no friendly fire
	if(victim.tInvulnUntil > localTime) return; // invulnerable
	if(victim.isInvulnerable) return; // GODMODE status
	if(victim.isBurrowed) return; // underground, untargetable

	// Apply defense reduction with penetration (diminishing returns formula)
	bool isSkill = (skillID != SkillID::INVALID);
	f32 pen = isSkill ? attacker.skillPowerPenetration : attacker.defensePenetration;
	f32 effectiveDefense = victim.defense * (1.0f - pen / 100.0f);
	if(effectiveDefense < 0) effectiveDefense = 0;
	f32 damageReduction = effectiveDefense / (effectiveDefense + 300.0f);
	f32 finalDamage = damage * (1.0f - damageReduction);

	// Crit check — skills use skillCritChance/skillCritDamage, auto-attacks use critChance/critDamage
	bool isCrit = false;
	f32 critChanceVal = isSkill ? attacker.skillCritChance : attacker.critChance;
	f32 critDamageVal = isSkill ? attacker.skillCritDamage : attacker.critDamage;
	if(Randf01() * 100.0f < critChanceVal) {
		finalDamage *= (critDamageVal / 100.0f);
		isCrit = true;
	}

	// Shield absorption (from SHIELD status effect)
	if(victim.shieldHp > 0) {
		if(finalDamage <= victim.shieldHp) {
			victim.shieldHp -= finalDamage;
			finalDamage = 0;
		} else {
			finalDamage -= victim.shieldHp;
			victim.shieldHp = 0;
		}
	}

	i32 dmgInt = (i32)finalDamage;
	if(dmgInt < 1) dmgInt = 1;

	victim.hp -= (f32)dmgInt;
	if(victim.hp < 0) victim.hp = 0;

	// Track total damage for battle log (PVP result screen)
	attacker.totalDamageDealt += dmgInt;
	victim.totalDamageReceived += dmgInt;

	// Track damage for assists (20s window)
	{
		World::Player::DamageEntry entry;
		entry.attackerIndex = attacker.index;
		entry.damage = (f32)dmgInt;
		entry.timestamp = localTime;
		entry.skillID = skillID;
		entry.statusID = statusID;
		victim.damageLog.push_back(entry);
	}

	// UG gains from combat
	attacker.ultimateGauge += 5; // attack
	if(attacker.ultimateGauge > attacker.maxUltimateGauge) attacker.ultimateGauge = attacker.maxUltimateGauge;
	victim.ultimateGauge += 3; // being hit
	if(victim.ultimateGauge > victim.maxUltimateGauge) victim.ultimateGauge = victim.maxUltimateGauge;

	// Compute hit positions and directions for damage visual
	vec3 attackerPos = attacker.body->GetWorldPos();
	vec3 victimPos = victim.body->GetWorldPos();
	vec3 attackDir = NormalizeSafe(victimPos - attackerPos);
	// damageType: 0=normal, 2=crit (from official capture analysis)
	i32 damageType = isCrit ? 2 : 0;

	// Broadcast damage visual — SN_BroadcastDamage (62235)
	// skillDocID: -1 for auto-attack, actual skill ID for skill damage
	i32 skillDocIDParam = (skillID != SkillID::INVALID) ? (i32)skillID : -1;
	replication.SendBroadcastDamage(attacker.Main().UID, victim.Main().UID, dmgInt,
		attackerPos, attackDir, victimPos, attackDir, damageType, skillDocIDParam);

	// Update HP stat for non-lethal damage
	// For lethal damage (hp <= 0), HP=0 stat update is sent in KillPlayer after DeadAck
	// (official order: DeadAck -> UpdateStat(HP=0) -> RespawnDelaytime)
	if(victim.hp > 0) {
		replication.SendUpdateStatToAll(victim.Main().UID, 0, (f32)victim.maxHp, (f32)victim.hp);
	}

	// Update UG stat (stat 37) for attacker
	replication.SendUpdateStatToAll(attacker.Main().UID, 37, attacker.maxUltimateGauge, attacker.ultimateGauge);

	// Combat log
	if(attacker.combatLogEnabled && attacker.clientHd != ClientHandle::INVALID) {
		SendDbgMsg(attacker.clientHd, LFMT(L"[DMG] You hit %s for %d damage (HP: %.0f/%.0f)",
			victim.name.data(), dmgInt, victim.hp, victim.maxHp));
	}
	if(victim.combatLogEnabled && victim.clientHd != ClientHandle::INVALID) {
		SendDbgMsg(victim.clientHd, LFMT(L"[DMG] %s hit you for %d damage (HP: %.0f/%.0f)",
			attacker.name.data(), dmgInt, victim.hp, victim.maxHp));
	}

	// Death check
	if(victim.hp <= 0) {
		KillPlayer(victim, attacker);
	}
}

void Game::KillPlayer(World::Player& victim, World::Player& killer)
{
	if(victim.isDead) return;

	// Clear all buffs/debuffs on death (L2-style: send RemoveStatus for both heroes)
	world.RemoveAllStatuses(victim);

	victim.isDead = true;
	victim.hp = 0;
	victim.deaths++;
	killer.kills++;
	killer.score = killer.kills + killer.assists;
	victim.score = victim.kills + victim.assists;

	// Kill streak tracking
	killer.currentKillStreak++;
	if(killer.currentKillStreak > killer.highestKillStreak)
		killer.highestKillStreak = killer.currentKillStreak;
	victim.currentKillStreak = 0; // reset on death

	// UG gain for killer
	killer.ultimateGauge += 20;
	if(killer.ultimateGauge > killer.maxUltimateGauge) killer.ultimateGauge = killer.maxUltimateGauge;

	// Team kills
	teamKills[killer.team]++;

	LOG("[KILL] %S killed %S (Team kills: RED=%d BLUE=%d)", killer.name.data(), victim.name.data(), teamKills[0], teamKills[1]);

	// Process assists (20s window) - deduplicate by attacker
	eastl::fixed_vector<u32, 16, false> assisterIndices;
	{
		eastl::fixed_set<u32, 16, false> assistedPlayers;
		foreach(entry, victim.damageLog) {
			if(TimeDurationSec(entry->timestamp, localTime) > 20.0) continue;
			if(entry->attackerIndex == killer.index) continue; // killer doesn't get assist
			if(assistedPlayers.find(entry->attackerIndex) != assistedPlayers.end()) continue; // already counted

			assistedPlayers.insert(entry->attackerIndex);
			assisterIndices.push_back(entry->attackerIndex);
			World::Player& assister = world.GetPlayer(entry->attackerIndex);
			assister.assists++;
			assister.score = assister.kills + assister.assists;

			// UG gain for assists
			assister.ultimateGauge += 10;
			if(assister.ultimateGauge > assister.maxUltimateGauge) assister.ultimateGauge = assister.maxUltimateGauge;
		}
	}
	// Build death damage info BEFORE clearing damage log
	// Consolidate damage entries by attacker, then by skill within each attacker
	// Max 3 skill entries per attacker (consolidated, matching official captures)
	Replication::DeadDamageAttacker ddiKiller;
	eastl::fixed_vector<Replication::DeadDamageAttacker, 8, false> ddiOthers;

	if(Config().EnableDeathDamageInfo) {
		// Aggregate damage by attacker index
		struct AttackerAgg {
			u32 playerIndex;
			i32 totalDamage;
			// Per-skill damage: skillID -> total damage
			struct SkillDmg {
				SkillID skillID;
				i32 statusID;
				i32 damage;
			};
			eastl::fixed_vector<SkillDmg, 16, false> skillDamages;

			void AddDamage(SkillID skill, i32 dmg, i32 status) {
				foreach_mut(sd, skillDamages) {
					if(sd->skillID == skill) {
						sd->damage += dmg;
						if(sd->statusID == 0 && status != 0) sd->statusID = status;
						totalDamage += dmg;
						return;
					}
				}
				SkillDmg sd;
				sd.skillID = skill;
				sd.statusID = status;
				sd.damage = dmg;
				skillDamages.push_back(sd);
				totalDamage += dmg;
			}
		};
		eastl::fixed_vector<AttackerAgg, 16, false> attackerAggs;

		foreach(entry, victim.damageLog) {
			if(TimeDurationSec(entry->timestamp, localTime) > 20.0) continue;

			// Find or create attacker aggregation
			AttackerAgg* agg = nullptr;
			foreach_mut(a, attackerAggs) {
				if(a->playerIndex == entry->attackerIndex) {
					agg = a;
					break;
				}
			}
			if(!agg) {
				AttackerAgg newAgg;
				newAgg.playerIndex = entry->attackerIndex;
				newAgg.totalDamage = 0;
				attackerAggs.push_back(newAgg);
				agg = &attackerAggs.back();
			}
			agg->AddDamage(entry->skillID, (i32)entry->damage, entry->statusID);
		}

		// Build DDI attacker list from aggregated data
		// Killer is first, others follow
		auto buildAttackerDDI = [&](const AttackerAgg& agg) -> Replication::DeadDamageAttacker {
			Replication::DeadDamageAttacker ddiAtt;
			World::Player& pl = world.GetPlayer(agg.playerIndex);
			ddiAtt.attackerKey = (i32)agg.playerIndex + 1; // 1-based
			ddiAtt.actorUID = pl.Main().UID;

			// Log all damage entries before sorting
			foreach_const(sd, agg.skillDamages) {
				LOG("[DDI] raw entry: attacker=%d skillID=%d damage=%d",
					agg.playerIndex, (i32)sd->skillID, sd->damage);
			}
			// Take top 3 skill entries by damage (descending)
			eastl::fixed_vector<AttackerAgg::SkillDmg, 16, false> sorted = agg.skillDamages;
			for(i32 i = 0; i < (i32)sorted.size(); i++) {
				for(i32 j = i + 1; j < (i32)sorted.size(); j++) {
					if(sorted[j].damage > sorted[i].damage) {
						auto tmp = sorted[i];
						sorted[i] = sorted[j];
						sorted[j] = tmp;
					}
				}
			}

			i32 maxEntries = (i32)sorted.size();
			if(maxEntries > 3) maxEntries = 3;
			for(i32 i = 0; i < maxEntries; i++) {
				Replication::DeadDamageEntry e;
				e.objectID = 0; // resolved per-client in SendDeadDamageInfo
				e.actorUID = pl.Main().UID;
				bool isSkill = (sorted[i].skillID != SkillID::INVALID);
				e.skillDocIndex = isSkill ? (i32)sorted[i].skillID : -1;
				// statusDocIndex: use hitStatus if available, else lookup from StatusManager
				if(sorted[i].statusID != 0) {
					e.statusDocIndex = sorted[i].statusID;
				} else if(isSkill) {
					i32 mappedStatus = GetStatusManager().FindStatusForSkill((i32)sorted[i].skillID);
					e.statusDocIndex = (mappedStatus != 0) ? mappedStatus : -1;
				} else {
					e.statusDocIndex = -1;
				}
				e.damage = sorted[i].damage;
				e.attackerIsMonster = 0;
				e.damageType = isSkill ? 1 : 0;
				ddiAtt.entries.push_back(e);
			}

			LOG("[DDI] Attacker %S (key=%d): %d skills, total dmg=%d",
				pl.name.data(), ddiAtt.attackerKey, maxEntries, agg.totalDamage);
			return ddiAtt;
		};

		// Find killer in aggregated data, build their DDI
		bool killerFound = false;
		foreach(agg, attackerAggs) {
			if(agg->playerIndex == killer.index) {
				ddiKiller = buildAttackerDDI(*agg);
				killerFound = true;
				break;
			}
		}

		// If killer had no tracked damage (edge case), create a minimal entry
		if(!killerFound) {
			ddiKiller.attackerKey = (i32)killer.index + 1;
			ddiKiller.actorUID = killer.Main().UID;
			Replication::DeadDamageEntry e;
			e.objectID = 0;
			e.actorUID = killer.Main().UID;
			e.skillDocIndex = -1; // auto-attack
			e.damage = 0;
			e.attackerIsMonster = 0;
			e.damageType = 0;
			ddiKiller.entries.push_back(e);
		}

		// Build others (assisters and other damage contributors)
		foreach(agg, attackerAggs) {
			if(agg->playerIndex == killer.index) continue;
			if(ddiOthers.size() >= 8) break;
			ddiOthers.push_back(buildAttackerDDI(*agg));
		}
	}

	victim.damageLog.clear();

	LOG("[KILL] %ls killed by %ls", victim.name.data(), killer.name.data());

	// Victim doc index = 100000000 + classType (matches official capture format)
	i32 victimDocIndex = 100000000 + (i32)victim.Main().classType;

	// Official kill sequence order (from Ranked_Test.pcapng analysis):
	//   0. SN_DEAD_DAMAGE_INFO (62063) — death damage breakdown (sent BEFORE DeadAck)
	//   1. SN_PvpEventAnnouncement (62488) — kill feed + triggers client-side First Blood/Multi-Kill/Streaks
	//   2. SN_SCORE_UPDATE (62504) — killer stats, team score
	//   3. SN_DeadAck (62062) — death confirmation
	//   4. DIE_BEHAVIORSTATE — death animation
	//   5. SN_UpdateStat HP=0 (62056)
	//   6. SN_RespawnDelaytime (62399)
	//
	// NOTE: 62114 (SN_KillNotify) is actually SN_UPDATE_GAME_PLAYER_TAG_COOLTIME — do NOT send on kill.
	// The client handles First Blood, Double Kill, Triple Kill, Kill Streaks, etc. entirely client-side
	// from the 62488 type=5 packet. See operater000.xml EVENT_NOTICE strings.

	// 0. Death damage info — client already has enemy portraits from loading-time
	// SN_NotifyPcDetailInfos (sent with ALL players at load). Do NOT re-send it here
	// as it corrupts SquadMasterSet GFx and breaks tag swap after respawn.
	if(Config().EnableDeathDamageInfo && victim.clientHd != ClientHandle::INVALID) {
		replication.SendDeadDamageInfo(victim.clientHd, victim.Main().UID,
			ddiKiller, ddiOthers, 10000);
	}

	// 1. Kill feed — SN_PvpEventAnnouncement (62488, type=5 for PvP kill)
	// Client internally tracks multi-kills (2.5s window) and kill streaks (3s window) from ServiceRule.xml
	replication.SendPvpEventAnnouncement(5, 0, 1, killer.Main().UID, victim.Main().UID);

	// 2. Scoreboard updates — SN_ScoreUpdate (62504)
	// Killer: update kills + score
	SendScoreUpdateToAll(killer, 1, killer.kills);   // statID=1 = Kills
	SendScoreUpdateToAll(killer, 0, killer.score);   // statID=0 = Score

	// Victim: update deaths
	SendScoreUpdateToAll(victim, 14, victim.deaths); // statID=14 = Deaths

	// Assisters: update assists + score
	foreach(ai, assisterIndices) {
		World::Player& assister = world.GetPlayer(*ai);
		SendScoreUpdateToAll(assister, 2, assister.assists);  // statID=2 = Assists
		SendScoreUpdateToAll(assister, 0, assister.score);    // statID=0 = Score
	}

	// Team score updates
	SendTeamScoreToAll();

	// 3. Death packet — SN_DeadAck (62062, 25 bytes on wire)
	replication.SendDeadAck(victim.Main().UID, killer.Main().UID, victimDocIndex);

	// 4. DIE_BEHAVIORSTATE — triggers the death animation and black screen overlay on the client
	replication.SendActionStateBroadcast(victim.Main().UID, ActionStateID::DIE_BEHAVIORSTATE, 0, 0);

	// 5. HP=0 stat update (sent AFTER DeadAck per official capture)
	replication.SendUpdateStatToAll(victim.Main().UID, 0, (f32)victim.maxHp, 0.f);

	// 6. Respawn timer — SN_RespawnDelaytime (62399) — broadcast to all players
	replication.SendRespawnDelaytime(victim.userID, 10000);

	// Set respawn timer (10 seconds)
	victim.tRespawnTime = TimeAdd(localTime, TimeMsToTime(10000));

	// Stop victim movement
	victim.input.moveTo = victim.body->GetWorldPos();
	victim.input.speed = 0;

	// Check win condition
	CheckMatchEnd();
}

void Game::RespawnPlayer(World::Player& player)
{
	player.isDead = false;
	player.tRespawnTime = Time::ZERO;

	// Full restore for BOTH heroes
	const HeroStats& mainStats = GetHeroStats(player.mainClass);
	const HeroStats& subStats = GetHeroStats(player.subClass);

	// Helper to compute full resource for a given hero's stats
	auto getFullResource = [](const HeroStats& s, f32& outMana, f32& outMaxMana) {
		if(s.resourceStatType == 17) { // ENERGY
			f32 e = s.ENERGY > 0 ? s.ENERGY : 100;
			outMana = e; outMaxMana = e;
		} else if(s.resourceStatType == 36) { // RAGE
			outMana = 100; outMaxMana = 100;
		} else if(s.resourceStatType == 56) { // BUBBLE
			outMana = 0; outMaxMana = 100;
		} else if(s.resourceStatType == 0) { // NONE
			outMana = 0; outMaxMana = 0;
		} else { // MANA (35)
			outMana = s.MANA; outMaxMana = s.MANA;
		}
	};

	// Restore main hero (slot 0) to full
	player.heroState[0].hp = mainStats.HP;
	getFullResource(mainStats, player.heroState[0].mana, player.heroState[0].maxMana);
	player.heroState[0].stamina = mainStats.STAMINA;
	player.heroState[0].resourceStatType = mainStats.resourceStatType;

	// Restore sub hero (slot 1) to full
	player.heroState[1].hp = subStats.HP;
	getFullResource(subStats, player.heroState[1].mana, player.heroState[1].maxMana);
	player.heroState[1].stamina = subStats.STAMINA;
	player.heroState[1].resourceStatType = subStats.resourceStatType;

	// Load currently active hero's stats
	const HeroStats& activeStats = (player.mainCharaID == 0) ? mainStats : subStats;
	player.hp = player.heroState[player.mainCharaID].hp;
	player.maxHp = activeStats.HP;
	player.mana = player.heroState[player.mainCharaID].mana;
	player.maxMana = player.heroState[player.mainCharaID].maxMana;
	player.stamina = player.heroState[player.mainCharaID].stamina;
	player.maxStamina = activeStats.STAMINA;
	player.resourceStatType = player.heroState[player.mainCharaID].resourceStatType;
	player.heroStats = &activeStats;

	// UG is preserved across deaths (official behavior)
	// Do NOT reset ultimateGauge here

	// Teleport to spawn point
	const auto& spawnPoints = mapSpawnPoints[player.team];
	if(spawnPoints.empty()) return;
	const SpawnPoint& sp = spawnPoints[RandInt(0, (i32)spawnPoints.size() - 1)];
	vec3 spawnPos = sp.pos;
	vec3 spawnRot = sp.rot;

	player.body->collider->setFootPosition(PxExtendedVec3(spawnPos.x, spawnPos.y, spawnPos.z));
	player.input.moveTo = spawnPos;
	player.input.speed = 0;

	// Reset tag cooldown and dodge cooldown on respawn
	player.tagCooldownActive = false;
	player.tagCooldownUntil = Time::ZERO;
	player.tDodgeCooldownUntil = Time::ZERO;
	// Don't send 62114 broadcast — interferes with other players' countdowns

	// Invulnerability for 3 seconds after respawn (server-side timer)
	player.tInvulnUntil = TimeAdd(localTime, TimeMsToTime(3000));

	// Send respawn packets
	replication.SendRevivePlayerAtStartingPoint(player.userID, player.Main().UID, player.Sub().UID, spawnPos);
	replication.SendPlayerSyncTeleport(player.Main().UID, spawnPos, spawnRot);

	// Clear death animation state for BOTH heroes — transition out of DIE_BEHAVIORSTATE
	replication.SendActionStateBroadcast(player.Main().UID, ActionStateID::RESPAWN_BEHAVIORSTATE, 0, 0);
	replication.SendActionStateBroadcast(player.Main().UID, ActionStateID::NONE_BEHAVIORSTATE, 0, 0);
	replication.SendActionStateBroadcast(player.Sub().UID, ActionStateID::RESPAWN_BEHAVIORSTATE, 0, 0);
	replication.SendActionStateBroadcast(player.Sub().UID, ActionStateID::NONE_BEHAVIORSTATE, 0, 0);

	// Apply respawn invincibility status (STATUS_PC_RESPAWN_INVICIBLE_3S)
	// This sends SN_AddStatus with 3s duration — client shows blue cube shield VFX
	// and auto-removes via SN_RemoveStatus when TickStatuses() expires it after 3 seconds
	world.AddStatus(player, 1230000010, player.Main().UID, 3.0f);

	// Update stats for active hero (Main) — send HP=max AFTER teleport to clear damage vignette
	replication.SendUpdateStatToAll(player.Main().UID, 0, player.maxHp, player.hp);
	if(player.resourceStatType != 0) { // skip for NONE — stat 0 = HP, would overwrite
		replication.SendUpdateStatToAll(player.Main().UID, (u8)player.resourceStatType, player.maxMana, player.mana);
	}
	replication.SendUpdateStatToAll(player.Main().UID, 64, player.maxStamina, player.stamina);
	replication.SendUpdateStatToAll(player.Main().UID, 37, player.maxUltimateGauge, player.ultimateGauge);

	// Update stats for inactive hero (Sub) - restore ALL to full
	u8 subSlot = player.mainCharaID ^ 1;
	replication.SendUpdateStatToAll(player.Sub().UID, 0,
		(player.mainCharaID == 0) ? subStats.HP : mainStats.HP,
		player.heroState[subSlot].hp);
	if(player.heroState[subSlot].resourceStatType != 0) { // skip for NONE
		replication.SendUpdateStatToAll(player.Sub().UID,
			(u8)player.heroState[subSlot].resourceStatType,
			player.heroState[subSlot].maxMana,
			player.heroState[subSlot].mana);
	}
	replication.SendUpdateStatToAll(player.Sub().UID, 64,
		(player.mainCharaID == 0) ? subStats.STAMINA : mainStats.STAMINA,
		player.heroState[subSlot].stamina);

	// Send HP stat update again AFTER all respawn packets — ensures client clears damage vignette
	// The client's HP-based screen effect needs the stat update to arrive after the teleport/revive
	replication.SendUpdateStatToAll(player.Main().UID, 0, player.maxHp, player.hp);

	LOG("[RESPAWN] %S respawned at (%.0f, %.0f, %.0f) mainCharaID=%d HP=%.0f/%.0f mana=%.0f/%.0f resStat=%d UG=%.0f subHP=%.0f/%0.f subMana=%.0f/%.0f subResStat=%d",
		player.name.data(), spawnPos.x, spawnPos.y, spawnPos.z,
		player.mainCharaID, player.hp, player.maxHp, player.mana, player.maxMana, player.resourceStatType,
		player.ultimateGauge,
		player.heroState[subSlot].hp, (player.mainCharaID == 0) ? subStats.HP : mainStats.HP,
		player.heroState[subSlot].mana, player.heroState[subSlot].maxMana,
		player.heroState[subSlot].resourceStatType);
}

void Game::CheckMatchEnd()
{
	if(matchEnded) return;

	// Titan Ruins: win by core destruction only (kills don't end the game)
	// Core destruction triggers TriggerGameOver directly from OnCoreDestroyed
	if(IsTitanRuins()) return;

	for(int t = 0; t < 2; t++) {
		if(teamKills[t] >= KILLS_TO_WIN) {
			TriggerGameOver(t);
			return;
		}
	}
}

void Game::TriggerGameOver(i32 winTeam)
{
	if(matchEnded) return;
	matchEnded = true;
	matchEndTime = localTime;
	winningTeam = winTeam;
	phase = Phase::GameOver;

	LOG("[GAME] Game Over! Team %d wins (RED=%d BLUE=%d)", winTeam, teamKills[0], teamKills[1]);

	// Build score data for all players
	eastl::fixed_vector<Replication::PlayerScoreData, MAX_PLAYERS, false> scoreData;
	foreach_const(wp, world.players) {
		Replication::PlayerScoreData sd;
		sd.playerIndex = wp->index;
		sd.kills = wp->kills;
		sd.deaths = wp->deaths;
		sd.assists = wp->assists;
		sd.score = wp->score;
		sd.disconnected = wp->disconnected;
		sd.totalDamageDealt = wp->totalDamageDealt;
		sd.totalDamageReceived = wp->totalDamageReceived;
		sd.highestKillStreak = wp->highestKillStreak;
		scoreData.push_back(sd);
	}

	// Send PVP result to all players
	foreach_const(p, playerList) {
		if(p->clientHd == ClientHandle::INVALID) continue;

		i32 playTimeMs = (i32)(TimeDurationSec(matchStartTime, localTime) * 1000.0);
		i32 gameEndReason;
		if(IsTitanRuins()) {
			gameEndReason = 7; // core destroyed or time up
		} else {
			gameEndReason = (teamKills[winTeam] >= KILLS_TO_WIN) ? 7 : 6; // 7=score_limit, 6=time
		}
		replication.SendPvpResult(p->clientHd, gameEndReason, playTimeMs, winTeam);
		replication.SendPvpResultScoreDeathmatch(p->clientHd, scoreData.data(), (i32)scoreData.size(),
			teamKills[0], teamKills[1]);
	}
}

// ============================================================================
// Titan Ruins: Tower/Core damage system
// ============================================================================

void Game::ApplyNpcDamage(World::Player& attacker, World::ActorNpc& npc, f32 damage)
{
	if(npc.isDead) return;
	if(!npc.hasHP) return;

	u8 npcTeam = (npc.faction == Faction::RED) ? 0 : 1;

	// Check core invulnerability
	if(IsCoreCreature(npc.docID) && cores[npcTeam].invulnerable) {
		return;
	}

	npc.hp -= damage;
	if(npc.hp < 0) npc.hp = 0;

	// Broadcast damage
	vec3 attackerPos = attacker.body->GetWorldPos();
	f32 angle = attacker.input.rot.upperYaw;
	vec3 attackDir = vec3(cosf(angle), sinf(angle), 0);
	vec3 hitDir = NormalizeSafe(npc.pos - attackerPos);
	replication.SendBroadcastDamage(attacker.Main().UID, npc.UID, (i32)damage,
		attackerPos, attackDir, npc.pos, hitDir, 0, 0);

	// Update HP stat
	replication.SendUpdateStatToAll(npc.UID, 0, npc.maxHp, npc.hp);

	// Log for combat-log-enabled players
	foreach(pl, world.players) {
		if(pl->combatLogEnabled && pl->clientHd != ClientHandle::INVALID) {
			SendDbgMsg(pl->clientHd, LFMT(L"[NPC] %ls hit structure (docID=%d) for %.0f dmg (HP: %.0f/%.0f)",
				attacker.name.c_str(), (i32)npc.docID, damage, npc.hp, npc.maxHp));
		}
	}

	if(npc.hp <= 0) {
		npc.isDead = true;

		// Send SN_DeadAck — tells client this entity died (hides HP gauge)
		replication.SendDeadAck(npc.UID, attacker.Main().UID, (i32)npc.docID);
		// Send DIE_BEHAVIORSTATE via SN_ActionChangeLevelEvent — triggers death animation
		foreach_const(p, playerList) {
			if(p->clientHd == ClientHandle::INVALID) continue;
			replication.DbgSendActionChange(p->clientHd, npc.UID, (i32)ActionStateID::DIE_BEHAVIORSTATE);
		}
		// Send HP=0 stat update
		replication.SendUpdateStatToAll(npc.UID, 0, npc.maxHp, 0.f);
		LOG("[NPC] DeadAck + DIE + HP=0 sent for UID=%u docID=%d", (u32)npc.UID, (i32)npc.docID);

		// Check if this NPC is a tracked tower
		bool wasTower = false;
		foreach(t, towers) {
			if(t->uid == npc.UID) {
				OnTowerDestroyed(*t);
				wasTower = true;
				break;
			}
		}
		if(!wasTower && IsCoreCreature(npc.docID)) {
			OnCoreDestroyed(cores[npcTeam]);
		}
	}
}

void Game::OnTowerDestroyed(TowerInfo& tower)
{
	tower.destroyed = true;
	towersAlive[tower.team]--;

	const wchar* teamNameW = (tower.team == 0) ? L"RED" : L"BLUE";
	LOG("[TitanRuins] Tower destroyed! Team %d towers remaining: %d", tower.team, towersAlive[tower.team]);

	// Notify all players
	foreach(pl, world.players) {
		if(pl->clientHd != ClientHandle::INVALID) {
			SendDbgMsg(pl->clientHd, LFMT(L"[TOWER] %S tower destroyed! (%d remaining)",
				teamNameW, towersAlive[tower.team]));
		}
	}

	// Check if all towers are down — make core vulnerable
	CheckCoreVulnerability(tower.team);
}

void Game::CheckCoreVulnerability(u8 team)
{
	if(towersAlive[team] <= 0 && cores[team].invulnerable) {
		cores[team].invulnerable = false;

		const wchar* teamNameW = (team == 0) ? L"RED" : L"BLUE";
		LOG("[TitanRuins] All towers destroyed for team %d — Core is now VULNERABLE!", team);

		// Notify all players
		foreach(pl, world.players) {
			if(pl->clientHd != ClientHandle::INVALID) {
				SendDbgMsg(pl->clientHd, LFMT(L"[CORE] %S core is now VULNERABLE! All towers destroyed.", teamNameW));
			}
		}
	}
}

void Game::OnCoreDestroyed(CoreInfo& core)
{
	LOG("[TitanRuins] Team %d Core DESTROYED! Game Over!", core.team);

	// The team that destroyed the core wins (opposite team)
	i32 winTeam = (core.team == 0) ? 1 : 0;
	TriggerGameOver(winTeam);
}

// ============================================================================

void Game::UpdateRegen(World::Player& player)
{
	// Once per second
	if(player.tLastRegenTick == Time::ZERO) {
		player.tLastRegenTick = localTime;
		return;
	}

	f64 elapsed = TimeDurationSec(player.tLastRegenTick, localTime);
	if(elapsed < 1.0) return;
	player.tLastRegenTick = localTime;

	bool hpChanged = false;
	bool manaChanged = false;
	bool staminaChanged = false;

	// Base regen boost (Titan Ruins: near own base = 10x regen)
	f32 regenMultiplier = 1.0f;
	if(IsTitanRuins()) {
		vec3 pp = player.body->GetWorldPos();
		vec2 base = (player.team == 0) ? vec2(3850, 14776) : vec2(27654, 14782);
		if(glm::distance(vec2(pp), base) < 3000.0f) {
			regenMultiplier = 10.0f;
		}
	}

	// HP regen
	if(player.hp < player.maxHp) {
		player.hp += player.hpRegen * regenMultiplier;
		if(player.hp > player.maxHp) player.hp = player.maxHp;
		hpChanged = true;
	}

	// Mana regen
	if(player.mana < player.maxMana) {
		player.mana += player.manaRegen * regenMultiplier;
		if(player.mana > player.maxMana) player.mana = player.maxMana;
		manaChanged = true;
	}

	// Stamina regen
	if(player.stamina < player.maxStamina) {
		player.stamina += player.staminaRegen;
		if(player.stamina > player.maxStamina) player.stamina = player.maxStamina;
		staminaChanged = true;
	}

	// Send stat updates
	if(hpChanged) {
		replication.SendUpdateStatToAll(player.Main().UID, 0, player.maxHp, player.hp);
	}
	if(manaChanged && player.resourceStatType != 0) {
		u8 statType = (u8)player.resourceStatType;
		replication.SendUpdateStatToAll(player.Main().UID, statType, player.maxMana, player.mana);
	}
	if(staminaChanged) {
		replication.SendUpdateStatToAll(player.Main().UID, 64, player.maxStamina, player.stamina);
	}
}

void Game::UpdateUltimateGauge(World::Player& player)
{
	// +1 UG per 1.5 seconds (passive)
	if(player.tLastUGTick == Time::ZERO) {
		player.tLastUGTick = localTime;
		return;
	}

	f64 elapsed = TimeDurationSec(player.tLastUGTick, localTime);
	if(elapsed < 1.5) return;
	player.tLastUGTick = localTime;

	if(player.ultimateGauge < player.maxUltimateGauge) {
		player.ultimateGauge += 1.0f;
		if(player.ultimateGauge > player.maxUltimateGauge) player.ultimateGauge = player.maxUltimateGauge;

		replication.SendUpdateStatToAll(player.Main().UID, 37, player.maxUltimateGauge, player.ultimateGauge);
	}
}

void Game::SendTeamScoreToAll()
{
	replication.SendTeamScoreToAll(teamKills[0], teamKills[1], teamKills[0], teamKills[1]);
}

void Game::SendScoreUpdateToAll(World::Player& player, i32 statID, i32 value)
{
	replication.SendScoreUpdatePlayer(player.userID, statID, value);
}

// --- Bot AI ---

// Titan Ruins lane waypoints (from RED base toward BLUE base)
// Follow actual map paths to avoid walls/structures
// Bots on BLUE team walk these in reverse
static const vec2 LANE_WAYPOINTS_TOP[] = {
	{4800, 15800},  // base exit
	{5800, 17000},  // past base wall
	{7200, 17800},  // near red outer tower top
	{9300, 18500},  // red exit tower top
	{12000, 18800}, // mid-lane junction
	{15750, 18500}, // near top sight
	{19200, 18800}, // approaching blue side
	{22200, 18500}, // blue exit tower top
	{24200, 17800}, // near blue outer tower top
	{25800, 17000}, // approaching blue base
};
static const vec2 LANE_WAYPOINTS_MID[] = {
	{4800, 14800},  // base exit
	{6000, 14800},  // past base wall
	{7500, 14800},  // near red outer tower mid
	{9600, 14800},  // red exit tower mid
	{12500, 15200}, // approaching center (slight curve)
	{15750, 14800}, // map center
	{19000, 14400}, // past center (slight curve)
	{21900, 14800}, // blue exit tower mid
	{24000, 14800}, // near blue outer tower mid
	{26000, 14800}, // approaching blue base
};
static const vec2 LANE_WAYPOINTS_BOT[] = {
	{4800, 13700},  // base exit
	{5800, 12500},  // past base wall
	{7200, 11800},  // near red outer tower bot
	{9300, 11000},  // red exit tower bot
	{12000, 10700}, // mid-lane junction
	{15750, 11000}, // near bot sight
	{19200, 10700}, // approaching blue side
	{22200, 11000}, // blue exit tower bot
	{24200, 11800}, // near blue outer tower bot
	{25800, 12500}, // approaching blue base
};

static const vec2* GetLaneWaypoints(Game::Lane lane, i32* outCount)
{
	switch(lane) {
		case Game::Lane::TOP: *outCount = ARRAY_COUNT(LANE_WAYPOINTS_TOP); return LANE_WAYPOINTS_TOP;
		case Game::Lane::MID: *outCount = ARRAY_COUNT(LANE_WAYPOINTS_MID); return LANE_WAYPOINTS_MID;
		case Game::Lane::BOT: *outCount = ARRAY_COUNT(LANE_WAYPOINTS_BOT); return LANE_WAYPOINTS_BOT;
	}
	*outCount = 0;
	return nullptr;
}

// ============================================================================
// Tower Auto-Attack AI
// ============================================================================

static constexpr f32 TOWER_ATTACK_RANGE = 1500.0f;
static constexpr f32 TOWER_DAMAGE = 300.0f;
static constexpr i32 TOWER_ATTACK_INTERVAL_MS = 2000; // 2 seconds between attacks
static constexpr i32 TOWER_REMOTE_BEAM = 1480010001;   // laser guide
static constexpr i32 TOWER_REMOTE_PROJECTILE = 1480010002; // projectile

void Game::UpdateTowerAI()
{
	foreach_mut(tower, towers) {
		if(tower->destroyed) continue;

		// Find the NPC actor for this tower
		World::ActorNpc* npc = world.FindNpcActor(tower->uid);
		if(!npc || npc->isDead) continue;

		// Find nearest enemy player in range
		World::Player* bestTarget = nullptr;
		f32 bestDist = TOWER_ATTACK_RANGE;

		foreach(player, world.players) {
			if(player->isDead) continue;
			if(player->team == tower->team) continue; // skip allies

			f32 dist = glm::distance(vec2(player->body->GetWorldPos()), vec2(npc->pos));
			if(dist < bestDist) {
				bestDist = dist;
				bestTarget = &(*player);
			}
		}

		if(!bestTarget) {
			tower->targetPlayerIdx = 0xFFFFFFFF;
			tower->hitCount = 0; // reset escalation when no target
			continue;
		}

		// Reset hit count if target changed
		if(tower->targetPlayerIdx != bestTarget->index) {
			tower->hitCount = 0;
		}
		tower->targetPlayerIdx = bestTarget->index;

		// Attack on cooldown
		if(localTime < tower->tNextAttack) continue;
		tower->tNextAttack = TimeAdd(localTime, TimeMsToTime(TOWER_ATTACK_INTERVAL_MS));

		// Escalating damage: 300, 600, 900, 1200, ... (capped at 3000)
		tower->hitCount++;
		f32 damage = TOWER_DAMAGE * (f32)tower->hitCount;
		if(damage > 3000.f) damage = 3000.f;

		vec3 towerPos = npc->pos;
		vec3 targetPos = bestTarget->body->GetWorldPos();
		f32 angle = atan2f(targetPos.y - towerPos.y, targetPos.x - towerPos.x);

		// 1. Send attack animation (SKILL_1_BEHAVIORSTATE)
		foreach_const(p, playerList) {
			if(p->clientHd == ClientHandle::INVALID) continue;
			replication.DbgSendActionChange(p->clientHd, tower->uid,
				(i32)ActionStateID::SKILL_1_BEHAVIORSTATE);
		}

		// 2. Send REMOTE VFX — laser guide beam (visual indicator)
		if(Config().EnableSkillVFX) {
			i32 beamSeedID = (i32)(0xC0000000 | world.remoteSeedCounter++);
			replication.SendRemoteSyncCreateToAll(
				tower->uid,
				TOWER_REMOTE_BEAM,       // laser guide (visual only)
				beamSeedID,
				towerPos, angle, targetPos,
				100, 3, 1000,            // fireObjectType=3 (Skill_1_Fire_Dummy), 1s lifetime
				bestTarget->Main().UID   // target for beam direction
			);

			// 3. Send REMOTE VFX — homing projectile
			i32 projSeedID = (i32)(0xC0000000 | world.remoteSeedCounter++);
			replication.SendRemoteSyncCreateToAll(
				tower->uid,
				TOWER_REMOTE_PROJECTILE, // homing projectile
				projSeedID,
				towerPos, angle, targetPos,
				100, 3, 1700,            // fireObjectType=3 (Skill_1_Fire_Dummy), 1.7s lifetime
				bestTarget->Main().UID   // target entity for homing
			);

			// Hit VFX at target
			replication.SendRemoteActivatedToAll(
				tower->uid, projSeedID,
				bestTarget->Main().UID, 10, targetPos);
		}

		// 3. Apply escalating damage
		bestTarget->hp -= damage;
		if(bestTarget->hp < 0) bestTarget->hp = 0;

		// 4. Broadcast damage number
		vec3 hitDir = NormalizeSafe(targetPos - towerPos);
		replication.SendBroadcastDamage(tower->uid, bestTarget->Main().UID,
			(i32)damage, towerPos, hitDir, targetPos, hitDir, 0, 0);

		// 5. Update HP stat
		replication.SendUpdateStatToAll(bestTarget->Main().UID, 0,
			(f32)bestTarget->maxHp, (f32)bestTarget->hp);

		// 6. Kill if HP <= 0
		if(bestTarget->hp <= 0 && !bestTarget->isDead) {
			World::Player* killer = nullptr;
			foreach(other, world.players) {
				if(other->team == tower->team && !other->isDead) {
					killer = &(*other); break;
				}
			}
			if(killer) KillPlayer(*bestTarget, *killer);
		}

		LOG("[TOWER] UID=%u hit#%d attacks player %d for %.0f dmg",
			(u32)tower->uid, tower->hitCount, bestTarget->index, damage);

		LOG("[TOWER] UID=%u attacks player %d for %.0f dmg (dist=%.0f)",
			(u32)tower->uid, bestTarget->index, TOWER_DAMAGE, bestDist);
	}
}

void Game::UpdateBotAI(Bot& bot)
{
	World::Player& wpl = world.GetPlayer(bot.playerIndex);
	if(wpl.isDead) return;

	// Don't move during PreGame (barriers are up)
	if(phase != Phase::Game) {
		wpl.input.speed = 0;
		return;
	}

	// Titan Ruins: sync bot Z from heightmap (or nearest human player as fallback)
	if(IsTitanRuins()) {
		vec3 botPos = wpl.body->GetWorldPos();
		i32 gx = (i32)(botPos.x / HMAP_CELL);
		i32 gy = (i32)(botPos.y / HMAP_CELL);
		f32 z = botPos.z; // keep current Z as default

		// Try heightmap first
		if(gx >= 0 && gx < HMAP_W && gy >= 0 && gy < HMAP_H && heightmapSet[gx][gy]) {
			z = heightmap[gx][gy];
		}
		else {
			// Search nearby cells
			for(i32 r = 1; r <= 3; r++) {
				bool found = false;
				for(i32 dx = -r; dx <= r && !found; dx++) {
					for(i32 dy = -r; dy <= r && !found; dy++) {
						i32 cx = gx+dx, cy = gy+dy;
						if(cx >= 0 && cx < HMAP_W && cy >= 0 && cy < HMAP_H && heightmapSet[cx][cy]) {
							z = heightmap[cx][cy];
							found = true;
						}
					}
				}
				if(found) break;
			}
			// Last resort: nearest human player
			if(z == botPos.z) {
				f32 bestDist = 5000.f;
				foreach(other, world.players) {
					if(other->IsBot() || other->isDead) continue;
					f32 dist = glm::distance(vec2(other->body->GetWorldPos()), vec2(botPos));
					if(dist < bestDist) {
						bestDist = dist;
						z = other->body->GetWorldPos().z;
					}
				}
			}
		}
		wpl.body->collider->setFootPosition(PxExtendedVec3(botPos.x, botPos.y, z));
	}

	// Stuck detection
	vec3 curPos = wpl.body->GetWorldPos();
	if(glm::distance(vec2(curPos), vec2(bot.lastPos)) < 1.0f && wpl.input.speed > 0) {
		bot.stuckCounter++;
		if(bot.stuckCounter > 120) { // ~2 seconds stuck
			bot.aiState = Bot::AIState::Unsticking;
			bot.stuckCounter = 0;
		}
	} else {
		bot.stuckCounter = 0;
	}
	bot.lastPos = curPos;

	// Find nearest enemy (prioritize real players)
	World::Player* bestTarget = nullptr;
	f32 bestDist = 999999.f;
	bool bestIsReal = false;

	foreach(other, world.players) {
		if(other->index == wpl.index) continue;
		if(other->team == wpl.team) continue;
		if(other->isDead) continue;

		f32 dist = glm::distance(vec2(curPos), vec2(other->body->GetWorldPos()));
		bool isReal = !other->IsBot();

		// Prefer real players
		if(isReal && !bestIsReal) {
			bestTarget = &(*other);
			bestDist = dist;
			bestIsReal = true;
		} else if(isReal == bestIsReal && dist < bestDist) {
			bestTarget = &(*other);
			bestDist = dist;
		}
	}

	// Helper: move bot toward a 2D position
	auto MoveTo = [&](vec2 dest) {
		vec2 dir = NormalizeSafe(dest - vec2(curPos));
		f32 angle = atan2f(dir.y, dir.x);
		wpl.input.moveTo = curPos + vec3(dir * 500.0f, 0);
		wpl.input.rot.upperYaw = angle;
		wpl.input.rot.bodyYaw = angle;
		wpl.input.speed = wpl.moveSpeed;
	};

	// Helper: attack target (auto-attack + VFX)
	auto DoAttack = [&](World::Player& target) {
		vec3 targetPos = target.body->GetWorldPos();
		vec2 toTarget = NormalizeSafe(vec2(targetPos - curPos));
		f32 angle = atan2f(toTarget.y, toTarget.x);
		wpl.input.rot.upperYaw = angle;
		wpl.input.rot.bodyYaw = angle;
		wpl.input.moveTo = curPos;
		wpl.input.speed = 0;

		if(localTime > bot.tNextAttack) {
			bot.tNextAttack = TimeAdd(localTime, TimeMsToTime(1000));
			replication.SendActionStateBroadcast(wpl.Main().UID, ActionStateID::ATTACK_BEHAVIORSTATE, 0, 0);

			if(Config().EnableSkillVFX) {
				i32 attackRemoteID = GetAttackRemoteID(wpl.Main().classType);
				if(attackRemoteID != 0) {
					i32 seedID = (i32)(0xC0000000 | world.remoteSeedCounter++);
					const GameXmlContent& content = GetGameXmlContent();
					const Remote& remote = content.GetRemote(RemoteIdx(attackRemoteID));
					i32 lifeTimeMs = (remote.lifeTime > 0) ? (i32)(remote.lifeTime * 1000) : 1000;
					replication.SendRemoteSyncCreateToAll(wpl.Main().UID, attackRemoteID, seedID,
						curPos, angle, targetPos, 100, 1, lifeTimeMs);
					replication.SendRemoteActivatedToAll(wpl.Main().UID, seedID,
						target.Main().UID, 10, targetPos);
				}
			}
			ApplyDamage(wpl, target, wpl.atk, SkillID::INVALID);
		}

		// Use skill every 2-3 seconds
		if(localTime > bot.tNextSkill) {
			bot.tNextSkill = TimeAdd(localTime, TimeMsToTime((i32)(Randf01() * 1000 + 2000)));
			const GameXmlContent& content = GetGameXmlContent();
			auto masterIt = content.masterClassTypeMap.find(wpl.Main().classType);
			if(masterIt != content.masterClassTypeMap.end()) {
				const auto& master = *masterIt->second;
				i32 numSkills = (i32)master.skillIDs.size();
				if(numSkills > 0) {
					for(i32 attempt = 0; attempt < 3; attempt++) {
						SkillID sid = master.skillIDs[RandInt(0, numSkills - 1)];
						auto skillIt = content.skillMap.find(sid);
						if(skillIt != content.skillMap.end()) {
							auto actionListIt = content.actionListMap.find(wpl.Main().classType);
							if(actionListIt != content.actionListMap.end()) {
								bool found = false;
								foreach_const(a, actionListIt->second) {
									if(a->ID == skillIt->second.action) { found = true; break; }
								}
								if(found) {
									wpl.input.cast.skillID = sid;
									wpl.input.cast.pos = targetPos;
									wpl.input.cast.targetList.clear();
									break;
								}
							}
						}
					}
				}
			}
		}
	};

	// Get lane waypoints
	i32 wpCount = 0;
	const vec2* waypoints = IsTitanRuins() ? GetLaneWaypoints(bot.lane, &wpCount) : nullptr;
	bool isBlueTeam = (wpl.team == 1);
	i32 wpDirection = isBlueTeam ? -1 : 1; // blue walks backwards through waypoints

	// Retreat threshold
	bool lowHP = (wpl.hp < wpl.maxHp * 0.25f);

	switch(bot.aiState) {
		case Bot::AIState::Laning: {
			// Check for nearby enemy to fight (range 2500)
			if(bestTarget && bestDist < 2500.f) {
				bot.aiState = Bot::AIState::Chasing;
				bot.targetIndex = bestTarget->index;
				break;
			}

			// Check for nearby enemy tower — stop and attack it
			if(IsTitanRuins()) {
				World::ActorNpc* enemyTower = nullptr;
				f32 towerDist = 1500.f;
				foreach(npc, world.actorNpcList) {
					if(!npc->hasHP || npc->isDead) continue;
					u8 npcTeam = (npc->faction == Faction::RED) ? 0 : 1;
					if(npcTeam == wpl.team) continue;
					f32 d = glm::distance(vec2(curPos), vec2(npc->pos));
					if(d < towerDist) { towerDist = d; enemyTower = &(*npc); }
				}
				if(enemyTower) {
					vec2 toT = NormalizeSafe(vec2(enemyTower->pos - curPos));
					f32 ang = atan2f(toT.y, toT.x);
					wpl.input.rot.upperYaw = ang;
					wpl.input.rot.bodyYaw = ang;
					if(towerDist <= wpl.attackRange + 200.f) {
						wpl.input.moveTo = curPos; wpl.input.speed = 0;
						if(localTime > bot.tNextAttack) {
							bot.tNextAttack = TimeAdd(localTime, TimeMsToTime(1000));
							replication.SendActionStateBroadcast(wpl.Main().UID, ActionStateID::ATTACK_BEHAVIORSTATE, 0, 0);
							ApplyNpcDamage(wpl, *enemyTower, wpl.atk);
						}
					} else {
						MoveTo(vec2(enemyTower->pos));
					}
					break;
				}
			}

			// Walk along lane waypoints
			if(waypoints && wpCount > 0) {
				if(bot.waypointIdx < 0) bot.waypointIdx = 0;
				if(bot.waypointIdx >= wpCount) bot.waypointIdx = wpCount - 1;

				vec2 wp = waypoints[bot.waypointIdx];
				f32 distToWP = glm::distance(vec2(curPos), wp);

				if(distToWP < 300.0f) {
					i32 nextIdx = bot.waypointIdx + wpDirection;
					if(nextIdx >= 0 && nextIdx < wpCount) {
						bot.waypointIdx = nextIdx;
					}
				}
				MoveTo(waypoints[bot.waypointIdx]);
			}

			if(lowHP) {
				bot.aiState = Bot::AIState::Retreating;
				bot.tStateChange = localTime;
			}
		} break;

		case Bot::AIState::Chasing: {
			if(!bestTarget || bestTarget->isDead) {
				bot.aiState = Bot::AIState::Laning;
				break;
			}
			f32 dist = glm::distance(vec2(curPos), vec2(bestTarget->body->GetWorldPos()));

			if(dist <= wpl.attackRange) {
				bot.aiState = Bot::AIState::Attacking;
			} else if(dist > 4000) {
				bot.aiState = Bot::AIState::Laning; // lost target, go back to lane
			} else {
				MoveTo(vec2(bestTarget->body->GetWorldPos()));
			}

			if(lowHP) {
				bot.aiState = Bot::AIState::Retreating;
				bot.tStateChange = localTime;
			}
		} break;

		case Bot::AIState::Attacking: {
			if(!bestTarget || bestTarget->isDead) {
				bot.aiState = Bot::AIState::Laning;
				break;
			}
			f32 dist = glm::distance(vec2(curPos), vec2(bestTarget->body->GetWorldPos()));

			if(dist > wpl.attackRange * 1.3f) {
				bot.aiState = Bot::AIState::Chasing;
				break;
			}

			DoAttack(*bestTarget);

			// Tag swap at 30% HP
			if(wpl.hp < wpl.maxHp * 0.3f && localTime > bot.tNextTag) {
				bot.tNextTag = TimeAdd(localTime, TimeMsToTime(15000));
				wpl.input.tag = 1;
			}

			if(lowHP) {
				bot.aiState = Bot::AIState::Retreating;
				bot.tStateChange = localTime;
			}
		} break;

		case Bot::AIState::Retreating: {
			// Run back toward own base
			vec2 base = isBlueTeam ? vec2(27654, 14782) : vec2(3850, 14776);
			MoveTo(base);

			// Tag swap while retreating
			if(wpl.hp < wpl.maxHp * 0.3f && localTime > bot.tNextTag) {
				bot.tNextTag = TimeAdd(localTime, TimeMsToTime(15000));
				wpl.input.tag = 1;
			}

			// Resume laning after 4 seconds or HP recovered
			if(TimeDurationSec(bot.tStateChange, localTime) > 4.0 || wpl.hp > wpl.maxHp * 0.5f) {
				bot.aiState = Bot::AIState::Laning;
			}
		} break;

		case Bot::AIState::Dead: {
			// Handled by respawn system
			bot.aiState = Bot::AIState::Laning;
		} break;

		case Bot::AIState::Unsticking: {
			// Move perpendicular to current facing to get around walls
			f32 curAngle = wpl.input.rot.bodyYaw;
			f32 sideAngle = curAngle + (Randf01() > 0.5f ? 1.57f : -1.57f); // +/- 90 degrees
			vec2 dir = vec2(cosf(sideAngle), sinf(sideAngle));
			wpl.input.moveTo = curPos + vec3(dir * 800.0f, 0);
			wpl.input.speed = wpl.moveSpeed;
			bot.aiState = Bot::AIState::Laning;
		} break;
	}
}

// --- Auto-attack (CQ_WeaponFire) ---

void Game::OnPlayerWeaponFire(ClientHandle clientHd, ActorUID actorUID, const vec3& pos)
{
	auto found = playerMap.find(clientHd);
	if(found == playerMap.end()) return;
	Player& p = *found->second;
	World::Player& player = world.GetPlayer(p.playerIndex);

	if(player.isDead) return;
	if(phase != Phase::Game) return;
	if(player.isStunned || player.inputBlocked.attack) return; // CC blocks attack

	const f32 angle = player.input.rot.upperYaw;
	const vec2 attackDir = vec2(cosf(angle), sinf(angle));

	// Determine cone angle based on ACTIVE hero type (ranged=90, melee=120)
	f32 coneHalfAngle = (player.attackRange > 400) ? (PI / 4.0f) : (PI / 3.0f); // 90 or 120 degrees

	// Find target in range and cone
	World::Player* bestTarget = nullptr;
	f32 bestDist = 999999.f;

	foreach(other, world.players) {
		if(other->index == player.index) continue;
		if(other->team == player.team) continue;
		if(other->isDead) continue;

		vec3 otherPos = other->body->GetWorldPos();
		vec2 toOther = vec2(otherPos - player.body->GetWorldPos());
		f32 dist = glm::length(toOther);

		if(dist > player.attackRange) continue;

		// Cone check
		vec2 dirNorm = NormalizeSafe(toOther);
		f32 dot = glm::dot(attackDir, dirNorm);
		if(dot < cosf(coneHalfAngle)) continue;

		if(dist < bestDist) {
			bestDist = dist;
			bestTarget = &(*other);
		}
	}

	// Titan Ruins: also check NPC targets (towers/cores)
	World::ActorNpc* bestNpcTarget = nullptr;
	f32 bestNpcDist = 999999.f;

	if(IsTitanRuins()) {
		foreach(npc, world.actorNpcList) {
			if(!npc->hasHP || npc->isDead) continue;

			// Check faction — can only attack enemy structures
			u8 npcTeam = (npc->faction == Faction::RED) ? 0 : 1;
			if(npcTeam == player.team) continue;

			// Skip invulnerable cores
			if(IsCoreCreature(npc->docID) && cores[npcTeam].invulnerable) continue;

			vec2 toNpc = vec2(npc->pos - player.body->GetWorldPos());
			f32 dist = glm::length(toNpc);

			// Use extended range for structures (they are large)
			f32 structRange = player.attackRange + 200.0f;
			if(dist > structRange) continue;

			// Cone check
			vec2 dirNorm = NormalizeSafe(toNpc);
			f32 dot = glm::dot(attackDir, dirNorm);
			if(dot < cosf(coneHalfAngle)) continue;

			if(dist < bestNpcDist) {
				bestNpcDist = dist;
				bestNpcTarget = &(*npc);
			}
		}
	}

	// Prefer player targets over NPC targets (unless NPC is closer)
	bool hitPlayer = (bestTarget != nullptr) && (bestNpcTarget == nullptr || bestDist <= bestNpcDist);
	bool hitNpc = (bestNpcTarget != nullptr) && !hitPlayer;

	if(hitPlayer) {
		// Send auto-attack projectile + hit VFX (same as bot auto-attack)
		if(Config().EnableSkillVFX) {
			i32 attackRemoteID = GetAttackRemoteID(player.Main().classType);
			if(attackRemoteID != 0) {
				i32 seedID = (i32)(0xC0000000 | world.remoteSeedCounter);
				world.remoteSeedCounter++;

				vec3 playerPos = player.body->GetWorldPos();
				vec3 targetPos = bestTarget->body->GetWorldPos();

				// Look up lifeTime from REMOTE_PC.xml data
				const GameXmlContent& content = GetGameXmlContent();
				const Remote& remote = content.GetRemote(RemoteIdx(attackRemoteID));
				i32 lifeTimeMs = (remote.lifeTime > 0) ? (i32)(remote.lifeTime * 1000) : 1000;

				LOG("[VFX_DIAG] AUTOATTACK: player=%d angle=%.4f angleDeg=%.1f casterPos=(%.1f,%.1f,%.1f) targetPos=(%.1f,%.1f,%.1f)",
					player.index, angle, angle * 180.0f / 3.14159265f,
					playerPos.x, playerPos.y, playerPos.z,
					targetPos.x, targetPos.y, targetPos.z);

				replication.SendRemoteSyncCreateToAll(
					player.Main().UID,
					attackRemoteID,    // per-hero REMOTE doc index
					seedID,            // 0xC0000000 | counter
					playerPos,         // fire position
					angle,             // fire rotation (toward target)
					targetPos,         // target position
					100,               // scale (1.0x)
					1,                 // fireObjectType=1 for Attack_Fire_Dummy
					lifeTimeMs         // LifeTime in ms from REMOTE_PC.xml
				);

				// Send hit VFX at target position
				replication.SendRemoteActivatedToAll(
					player.Main().UID, seedID,
					bestTarget->Main().UID, 10, targetPos);
			}
		}

		ApplyDamage(player, *bestTarget, player.atk, SkillID::INVALID);
	}
	else if(hitNpc) {
		// Attack NPC structure (tower/core)
		ApplyNpcDamage(player, *bestNpcTarget, player.atk);
	}
}

// --- Return to Bridge ---

void Game::OnPlayerReturnToCity(ClientHandle clientHd)
{
	auto found = playerMap.find(clientHd);
	if(found == playerMap.end()) return;

	// Only allow during GameOver
	if(phase != Phase::GameOver) return;

	replication.SendReturnToCity(clientHd);
	replication.SendDoConnectChannelServer(clientHd);
}

// --- Pickups ---

void Game::InitPickupSpots()
{
	auto addSpot = [&](PickupType type, vec3 pos, f32 respawnSec) {
		PickupSpot spot;
		spot.type = type;
		spot.pos = pos;
		spot.respawnSec = respawnSec;
		spot.pickupRadius = 150.0f;
		spot.groundItemID = (i32)(0xC0000000 | nextGroundItemID++);
		spot.tNextSpawn = TimeAdd(localTime, TimeMsToTime((i32)(respawnSec * 1000)));
		spot.alive = false;
		pickupSpots.push_back(spot);
	};

	// UG pickups (25s respawn)
	addSpot(PickupType::UG, vec3(1537, 3314, 1115), 25.0f);
	addSpot(PickupType::UG, vec3(1499, 3769, 1115), 25.0f);

	// Recharge pickup (30s respawn)
	addSpot(PickupType::Recharge, vec3(3555, 3570, 551), 30.0f);

	// HP pickup (20s respawn)
	addSpot(PickupType::HP, vec3(6626, 3488, 743), 20.0f);
}

void Game::UpdatePickups()
{
	foreach(spot, pickupSpots) {
		if(!spot->alive && spot->tNextSpawn < localTime) {
			spot->alive = true;
			// Map pickup type to nIndex from official captures
			i32 nIndex = 0;
			switch(spot->type) {
				case PickupType::UG:       nIndex = 130101000; break;
				case PickupType::HP:       nIndex = 130100060; break;
				case PickupType::Mana:     nIndex = 130100150; break;
				case PickupType::Recharge: nIndex = 130100150; break;
			}
			replication.SendCreateGroundItem(spot->groundItemID, nIndex, spot->pos);
		}

		if(spot->alive) {
			// Check proximity to players
			foreach(player, world.players) {
				if(player->isDead) continue;
				f32 dist = glm::distance(vec2(player->body->GetWorldPos()), vec2(spot->pos));
				if(dist < spot->pickupRadius) {
					CollectPickup(*player, *spot);
					break;
				}
			}
		}
	}
}

void Game::CollectPickup(World::Player& player, PickupSpot& spot)
{
	spot.alive = false;
	spot.tNextSpawn = TimeAdd(localTime, TimeMsToTime((i32)(spot.respawnSec * 1000)));

	replication.SendDestroyGroundItem(spot.groundItemID);

	// Assign new ID for next spawn
	spot.groundItemID = (i32)(0xC0000000 | nextGroundItemID++);

	switch(spot.type) {
		case PickupType::UG: {
			player.ultimateGauge += 25;
			if(player.ultimateGauge > player.maxUltimateGauge) player.ultimateGauge = player.maxUltimateGauge;
			replication.SendUpdateStatToAll(player.Main().UID, 37, player.maxUltimateGauge, player.ultimateGauge);
			// VFX: Speed/power buff effect (STATUS_SFX_ITEM_BUFF_SpeedUp — yellow/orange glow)
			// Pickup VFX disabled — TITAN_HEAL status has DoT that causes permanent effects
		} break;

		case PickupType::HP: {
			player.hp += player.maxHp * 0.3f;
			if(player.hp > player.maxHp) player.hp = player.maxHp;
			replication.SendUpdateStatToAll(player.Main().UID, 0, player.maxHp, player.hp);
			// VFX: HP recovery effect (STATUS_STAGESKILL_TITAN_HEAL1 — STATUS_SFX_ITEM_CONTINUITY_HP)
			// Pickup VFX disabled — TITAN_HEAL status causes permanent effects
		} break;

		case PickupType::Mana: {
			if(player.resourceStatType != 0) {
				player.mana += player.maxMana * 0.5f;
				if(player.mana > player.maxMana) player.mana = player.maxMana;
				replication.SendUpdateStatToAll(player.Main().UID, (u8)player.resourceStatType, player.maxMana, player.mana);
			}
			// VFX: Mana/SP instant recovery effect (STATUS_SFX_ITEM_INSTANT_SP — blue glow)
			// Pickup VFX disabled — TITAN_HEAL status has DoT that causes permanent effects
		} break;

		case PickupType::Recharge: {
			player.hp += player.maxHp * 0.1f;
			if(player.hp > player.maxHp) player.hp = player.maxHp;
			replication.SendUpdateStatToAll(player.Main().UID, 0, player.maxHp, player.hp);
			if(player.resourceStatType != 0) {
				player.mana += player.maxMana * 0.15f;
				if(player.mana > player.maxMana) player.mana = player.maxMana;
				replication.SendUpdateStatToAll(player.Main().UID, (u8)player.resourceStatType, player.maxMana, player.mana);
			}
			player.ultimateGauge += 8;
			if(player.ultimateGauge > player.maxUltimateGauge) player.ultimateGauge = player.maxUltimateGauge;
			replication.SendUpdateStatToAll(player.Main().UID, 37, player.maxUltimateGauge, player.ultimateGauge);
			// VFX: Speed/power buff (STATUS_SFX_ITEM_BUFF_SpeedUp) — combined recovery feels like a power-up
			// Pickup VFX disabled — TITAN_HEAL status has DoT that causes permanent effects
		} break;
	}

	LOG("[PICKUP] %S collected %s at (%.0f, %.0f, %.0f)",
		player.name.data(),
		spot.type == PickupType::UG ? "UG" : spot.type == PickupType::HP ? "HP" :
		spot.type == PickupType::Mana ? "Mana" : "Recharge",
		spot.pos.x, spot.pos.y, spot.pos.z);
}

// ============================================================================
// Titan's Sight — Capturable neutral buildings
// ============================================================================

void Game::UpdateSights()
{
	const f32 dt = 1.0f / (f32)UPDATE_TICK_RATE;
	const f32 captureRate = 1.0f / ((f32)SIGHT_CAPTURE_TIME_MS / 1000.0f); // progress per second

	for(i32 si = 0; si < (i32)sights.size(); si++) {
		SightInfo& sight = sights[si];

		// Find closest alive player within capture radius
		i32 closestPlayer = -1;
		f32 closestDist = SIGHT_CAPTURE_RADIUS;

		foreach(player, world.players) {
			if(player->isDead) continue;
			if(player->isStunned) continue;
			if(!player->interacting) continue; // require F key press (IT_ACTION_OCCUPY)
			vec3 pp = player->body->GetWorldPos();
			f32 dx = pp.x - sight.pos.x;
			f32 dy = pp.y - sight.pos.y;
			f32 dist = sqrtf(dx*dx + dy*dy);
			if(dist < closestDist) {
				closestDist = dist;
				closestPlayer = (i32)player->index;
			}
		}

		if(closestPlayer >= 0) {
			World::Player& player = world.GetPlayer(closestPlayer);

			// Same team already owns — no capture needed
			if(sight.ownerTeam == (i8)player.team) {
				sight.capturingPlayer = -1;
				sight.captureProgress = 0;
				continue;
			}

			// Different player started capturing — reset progress
			if(sight.capturingPlayer != closestPlayer) {
				sight.capturingPlayer = closestPlayer;
				sight.captureProgress = 0;
			}

			// Advance capture
			sight.captureProgress += captureRate * dt;

			if(sight.captureProgress >= 1.0f) {
				// Captured!
				sight.captureProgress = 0;
				sight.capturingPlayer = -1;
				i8 oldTeam = sight.ownerTeam;
				sight.ownerTeam = (i8)player.team;

				// Award points
				teamKills[player.team] += SIGHT_CAPTURE_POINTS;
				SendTeamScoreToAll();

				// Change sight visual to team color via action state
				// DYNAMIC_BATTLE_STAND = active/captured state
				foreach_mut(dyn, world.actorDynamicList) {
					if(dyn->UID == sight.uid) {
						dyn->action = ActionStateID::DYNAMIC_BATTLE_STAND;
						dyn->faction = (player.team == 0) ? Faction::RED : Faction::BLUE;
						break;
					}
				}

				// Notify all players
				const wchar* teamName = (player.team == 0) ? L"RED" : L"BLUE";
				foreach(pl, world.players) {
					if(pl->clientHd != ClientHandle::INVALID) {
						SendDbgMsg(pl->clientHd, LFMT(L"[SIGHT] %S captured Titan's Sight! (+%d points for %S)",
							player.name.c_str(), SIGHT_CAPTURE_POINTS, teamName));
					}
				}

				LOG("[TitanRuins] Sight %d captured by player %d (team %d). Score: RED=%d BLUE=%d",
					si, closestPlayer, player.team, teamKills[0], teamKills[1]);
			}
		} else {
			// No one near — reset capture progress
			if(sight.capturingPlayer >= 0) {
				sight.capturingPlayer = -1;
				sight.captureProgress = 0;
			}
		}
	}
}
