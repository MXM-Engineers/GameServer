#include <mxm/achievements.h>
#include <mxm/summary.h>
#include <mxm/pve_ranking.h>
#include "game.h"

#include <EAStdC/EAString.h>
#include <EAStdC/EAScanf.h>
#include <common/utils.h>

#include <mxm/game_content.h>
#include "config.h"
#include "world_master_rules.h"
#include <mxm/account_progress.h>

static const f32 STAGE_TIME_LIMIT_FALLBACK_SEC = 300.0f;
static const f64 RETURN_TO_LOBBY_DELAY_SEC = 20.0;
static const u32 MEASURED_PVE_FAIL_RETURN_MS = 15000;
static const f32 HYP_HUMAN_POSITION_SNAP = 100.0f;
static const f32 HYP_CLIENT_BLOCKED_STEP = 5.0f;
static const f32 HYP_CLIENT_SLIDE_RATIO = 0.5f;
static const f32 HYP_CLIENT_SLIDE_MIN_SEC = 0.1f;
static const f32 HYP_CLIENT_SLIDE_MAX_SEC = 2.0f;
static const f32 HYP_CLIENT_SAME_DIR_DOT = 0.95f;
static const f32 HYP_CLIENT_SLIDE_DIR_DOT = 0.985f;
static const f64 HYP_CLIENT_PUSH_INFLIGHT_SEC = 0.3;
static const f32 HYP_HUMAN_DASH_SNAP = 400.0f;
static const f32 MEASURED_BOT_TAG_SHARE = 2.5f / 49.9f;
static const u32 MEASURED_DEFENCE_RETURN_MS = 280;
static const i32 MEASURED_DEFENCE_RESPAWN_MS = 12000;
struct PvpGuildPointRule { i32 teamSize; u32 win; u32 lose; };
static const PvpGuildPointRule MEASURED_PVP_GUILD_POINTS[] = { { 3, 10, 5 }, { 4, 10, 4 }, { 5, 50, 20 } };

static u8 GuildMembersInTeam(World& world, const World::Player& player)
{
	if(player.guildTag.empty()) return 0;
	u8 members = 0;
	for(u32 i = 0; i < world.PlayerCount(); i++) {
		const World::Player& other = world.GetPlayer(i);
		if(other.team == player.team && other.clientHd != ClientHandle::INVALID && other.guildTag == player.guildTag) members++;
	}
	return members;
}
static bool BotWeaponUsable(const GameXmlContent& xml, WeaponIndex current, i32 doc)
{
	const WeaponFiring* base = xml.FindWeaponFiring((i32)current);
	const WeaponFiring* variant = xml.FindWeaponFiring(doc);
	if(!base || !variant || xml.GetWeaponAttack(doc) <= 0) return false;
	for(i8 stance = 1; stance < 16; stance++) {
		if(xml.FindWeaponFiring((i32)current, stance) != base && xml.FindWeaponFiring(doc, stance) == variant) return false;
	}
	return true;
}

static void RollBotGear(World::PlayerDescription& desc)
{
	if(!desc.gearDocs[0].empty() || !desc.gearDocs[1].empty()) return;
	const GameXmlContent& xml = GetGameXmlContent();
	for(i32 i = 0; i < 2; i++) {
		const GameXmlContent::MeasuredBotGear* gear = xml.PickMeasuredBotGear(desc.masters[i], RandUint());
		desc.botGear[i] = gear;
		if(gear && gear->classType == (i32)desc.masters[i] && gear->weapon != 0 && BotWeaponUsable(xml, desc.weapons[i], gear->weapon)) desc.weapons[i] = (WeaponIndex)gear->weapon;
	}
}

static const f64 PVE_REWARD_TIMEOUT_SEC = 60.0;
static const u32 MEASURED_REWARD_WAIT_MS = 30000;
static const u8 MEASURED_REWARD_VIP_SLOTS = 1;
static const i32 MEASURED_REWARD_CARD_COUNT = 25;
static const u32 MEASURED_REWARD_TYPE_ITEM = 2;
static const u32 MEASURED_REWARD_TYPE_EXP = 4;
static const u8 HYP_TECHNIC_NORMAL = 0;
static const u8 HYP_TECHNIC_GOOD = 1;
static const u8 HYP_TECHNIC_GREAT = 2;
static const i32 HYP_REWARD_CARDS_PER_VIEW_ITEM = 3;
static const i32 HYP_REWARD_EXP_MIN_PERCENT = 25;
static const i32 HYP_REWARD_EXP_STEP_PERCENT = 7;
static const i32 HYP_REWARD_EXP_MAX_PERCENT = 60;
static const u8 MEASURED_STAGE_GRADE_SSS = 1;
static const u8 MEASURED_STAGE_GRADE_SS = 2;
static const u8 MEASURED_STAGE_GRADE_S = 3;
static const u32 MEASURED_MONEY_REASON_PICKUP = 23;
static const u32 MEASURED_ITEM_UPDATE_REASON_PICKUP = 101;
static const u8 MEASURED_STAGE_GRADE_A = 4;
static const u8 MEASURED_STAGE_GRADE_B = 5;
static const u8 MEASURED_STAGE_GRADE_C = 6;
static const u32 MEASURED_STAGE_GRADE_SCALE = 100;
static const u32 MEASURED_TIME_SCORE_RANK = 1;
static const f64 MEASURED_STAGE_RANK_PERIOD_MS = 1000.0;
static const u32 MEASURED_PVE_BASE_EXP = 750;
static const u32 MEASURED_REWARD_BONUS_EVENT = 1;
static const u32 MEASURED_REWARD_BONUS_VIP = 6;
static const u32 MEASURED_REWARD_BONUS_GUILDSKILL = 7;
static const i32 MEASURED_MEMBERSHIP_REWARD_PERCENT = 25;

static void AddRewardBonus(Replication::RewardBonusList* list, u32 type, u32 base, i32 percent)
{
	const u32 amount = (u32)((u64)base * (u64)MAX(percent, 0) / 100);
	if(amount > 0 && !list->full()) list->push_back({ type, amount });
}

static u32 RewardBonusTotal(const Replication::RewardBonusList& list)
{
	u32 total = 0;
	foreach_const(b, list) total += b->amount;
	return total;
}

static i32 KillsOf(const eastl::vector<eastl::pair<i32,f32>>& kills, i32 doc)
{
	i32 n = 0;
	foreach_const(k, kills) if(k->first == doc) n++;
	return n;
}

static i32 BestKillsInWindow(const eastl::vector<eastl::pair<i32,f32>>& kills, const GameXmlContent::SummaryChallengeInfo& c, i32 doc)
{
	eastl::fixed_vector<f32,256,true> times;
	foreach_const(k, kills) {
		bool target = false;
		foreach_const(t, c.targets) if(t->first == k->first && (doc == 0 || t->first == doc)) target = true;
		if(target) times.push_back(k->second);
	}
	const f32 window = c.timeSec > 0 ? c.timeSec : 1e9f;
	i32 best = 0;
	i32 start = 0;
	for(i32 i = 0; i < (i32)times.size(); i++) {
		while(times[i] - times[start] > window) start++;
		best = MAX(best, i - start + 1);
	}
	return best;
}

static bool KillWindowReached(const eastl::vector<eastl::pair<i32,f32>>& kills, const GameXmlContent::SummaryChallengeInfo& c)
{
	if(c.totalCount > 0) return BestKillsInWindow(kills, c, 0) >= c.totalCount;
	foreach_const(t, c.targets) if(BestKillsInWindow(kills, c, t->first) < MAX(t->second, 1)) return false;
	return !c.targets.empty();
}

static void AppendSummaryEvents(World& world, const World::Player& p, i32 stage, bool noDeath, u8 rank)
{
	const eastl::vector<eastl::pair<i32,f32>>& kills = world.stageKills;
	const wchar* name = p.name.data();
	typedef GameXmlContent::SummaryChallenge Challenge;
	const GameXmlContent& content = GetGameXmlContent();
	i32 difficulty = -1;
	const GameXmlContent::Summary* summary = content.FindStageSummary(stage, &difficulty);
	if(!summary) return;
	static const u8 MEASURED_SUMMARY_TYPE_CLEAR_RANK = 1;
	world.SummaryLive(p, summary->index, stage, MEASURED_SUMMARY_TYPE_CLEAR_RANK, rank);
	foreach_const(c, summary->challenges) {
		if(c->difficulty >= 0 && c->difficulty != difficulty) continue;
		bool done = false;
		switch(c->type) {
			case Challenge::NODIE: done = noDeath; break;
			case Challenge::RANK_OVER: done = c->rank != 0 && rank <= c->rank; break;
			case Challenge::NOHITBYBOSS: {
				done = true;
				foreach_const(t, c->targets) foreach_const(d, p.hitByDocs) if(*d == t->first) done = false;
			} break;
			case Challenge::KILL_IN_TIME: done = KillWindowReached(kills, *c); break;
			case Challenge::KILL_GREATER_EQUAL: done = KillWindowReached(kills, *c); break;
			case Challenge::NOKILL: {
				done = true;
				foreach_const(t, c->targets) if(KillsOf(kills, t->first) > 0) done = false;
			} break;
			case Challenge::KILL_LESS: {
				done = true;
				foreach_const(t, c->targets) if(KillsOf(kills, t->first) >= MAX(t->second, c->totalCount)) done = false;
			} break;
			case Challenge::STATUSINDEX: {
				done = true;
				foreach_const(s, p.npcStatuses) if(*s == c->status) done = false;
			} break;
			case Challenge::STATUSTYPE: {
				done = true;
				foreach_const(s, p.npcStatuses) {
					const StatusInfo* info = content.FindStatus(*s);
					if(info && (i32)info->type == c->status) done = false;
				}
			} break;
			default: break;
		}
		if(done) world.SummaryLive(p, summary->index, stage, (u8)SummaryEntryType::CHALLENGE, c->index);
		LOG("Summary :: %ls stage %d challenge %d (%d) %s", name, stage, c->index, (i32)c->type, done ? "done" : "missed");
	}
}

static void ComputeRewardBonuses(GameType type, StageIndex stage, u32 baseRon, u32 baseExp, const eastl::array<u16,2>& guildBoost, Replication::RewardBonusList* bonusRon, Replication::RewardBonusList* bonusExp)
{
	i32 eventExp = 0, eventRon = 0;
	GetGameXmlContent().ActiveEventBonus(type, stage, &eventExp, &eventRon);
	AddRewardBonus(bonusRon, MEASURED_REWARD_BONUS_EVENT, baseRon, eventRon);
	AddRewardBonus(bonusExp, MEASURED_REWARD_BONUS_EVENT, baseExp, eventExp);
	AddRewardBonus(bonusRon, MEASURED_REWARD_BONUS_VIP, baseRon, MEASURED_MEMBERSHIP_REWARD_PERCENT);
	AddRewardBonus(bonusExp, MEASURED_REWARD_BONUS_VIP, baseExp, MEASURED_MEMBERSHIP_REWARD_PERCENT);
	AddRewardBonus(bonusRon, MEASURED_REWARD_BONUS_GUILDSKILL, baseRon, guildBoost[0]);
	AddRewardBonus(bonusExp, MEASURED_REWARD_BONUS_GUILDSKILL, baseExp, guildBoost[1]);
}
static const i32 HYP_ATTACK_SCORE_PER_KILL = 3;
static const i32 HYP_DEFENSE_SCORE_PER_DEATH = 33;

const CreatureIndex CI_DOOR = CreatureIndex(110040546);
const CreatureIndex CI_WALL = CreatureIndex(110042602);
static bool IsClientOnLoopback(const Server* server, ClientHandle clientHd)
{
	auto found = server->clientHandle2IDMap.find(clientHd);
	if(found == server->clientHandle2IDMap.end()) return false;
	return server->clientInfo[found->second].ip[0] == 127;
}

static const f64 LEVEL_START_BEFORE_GAME_MS = 5000.0;
static const f64 PRE_GAME_MIN_MS = 10000.0;

static const eastl::pair<i32,i32> MEASURED_STAGE_SKILL_PICKS[] = { { 180000100, 14 }, { 180000110, 100 }, { 180000120, 14 }, { 180000130, 12 }, { 180000140, 14 } };

static SkillID MeasuredBotStageSkill(const GameXmlContent::StageMaps& stage, SkillID fallback)
{
	i32 total = 0;
	foreach_const(c, stage.stageSkillCandidates) for(const auto& m : MEASURED_STAGE_SKILL_PICKS) if(m.first == (i32)*c) total += m.second;
	if(total <= 0) return fallback;
	i32 roll = (i32)(RandUint() % (u32)total);
	foreach_const(c, stage.stageSkillCandidates) for(const auto& m : MEASURED_STAGE_SKILL_PICKS) {
		if(m.first != (i32)*c) continue;
		if(roll < m.second) return *c;
		roll -= m.second;
	}
	return fallback;
}

void Game::Init(Server* server_, const In::MQ_CreateGame& gameInfo, const eastl::array<ClientHandle, MAX_PLAYERS>& playerClientHdList)
{
	replication.Init(server_);
	ASSERT((i32)gameInfo.playerCount <= (i32)world.players.max_size());
	ASSERT(gameInfo.gameType != GameType::INVALID);
	ASSERT(gameInfo.areaIndex != AreaIndex(0));
	ASSERT(gameInfo.stageIndex != StageIndex(0));
	ASSERT(gameInfo.mapIndex != MapIndex(0));
	replication.inGameID = (i32)(u64)gameInfo.sortieUID;
	replication.gameType = gameInfo.gameType;
	replication.areaIndex = gameInfo.areaIndex;
	replication.stageIndex = gameInfo.stageIndex;
	replication.mapIndex = gameInfo.mapIndex;
	if(Config().DevForcedMap != 0) {
		replication.mapIndex = (MapIndex)Config().DevForcedMap;
	}
	replication.canEscape = gameInfo.canEscape;
	replication.isTrespass = 0;
	replication.surrenderAbleTime = gameInfo.surrenderAbleTime;
	replication.supportKitIndex = -1;
	world.Init(&replication);
	if(const GameXmlContent::StageMaps* stage = GetGameXmlContent().FindStageMaps(gameInfo.stageIndex)) {
		const bool pve = stage->gameMode == GameModeType::GAME_MODE_STAGE || stage->gameMode == GameModeType::GAME_MODE_DEFENCE;
		world.hostileMonsters = pve;
		world.ingameSkillPoints = !pve;
		replication.pveTeams = pve;
		world.occupyMode = stage->gameMode == GameModeType::GAME_MODE_OCCUPY;
		replication.occupyMode = world.occupyMode;
		if(pve) world.difficultyBit = stage->difficultyBit;
		if(pve) world.monsterStatScale = GetGameXmlContent().DifficultyMonsterStat(stage->difficultyBit);
		if(stage->gameMode == GameModeType::GAME_MODE_STAGE) {
			pveStage = true;
			world.noRespawn = true;
			world.stageClearMonster = stage->clearMonster;
			world.stageClearCount = stage->clearCount;
			world.stageCommonDrop = stage->commonDrop;
			foreach_const(m, stage->maps) if(!zoneMaps.full()) zoneMaps.push_back(*m);
			if(stage->bossMap != MapIndex(0) && !zoneMaps.full()) zoneMaps.push_back(stage->bossMap);
			LOG("Stage :: %d zone(s), difficulty bit %d", (i32)zoneMaps.size(), (i32)world.difficultyBit);
		}
		if(stage->gameMode == GameModeType::GAME_MODE_DEFENCE) {
			defenceStage = true;
			world.fixedRespawnMs = MEASURED_DEFENCE_RESPAWN_MS;
			world.StartDefence(stage->defendTarget);
		}
	}

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
		desc.guildTag.assign(p.guildTag.data, p.guildTag.len);
		desc.team = p.team;
		desc.masters = p.masters;
		desc.skins = p.skins;
		desc.skills = p.skills;
		desc.stageSkills = { SkillID::INVALID, SkillID::INVALID };
		if(const GameXmlContent::StageMaps* stage = xml.FindStageMaps(gameInfo.stageIndex)) {
			desc.stageSkills[0] = stage->stageSkill;
			if(!stage->stageSkillCandidates.empty()) desc.stageSkills[1] = stage->stageSkillCandidates[0];
			foreach_const(c, stage->stageSkillCandidates) if(*c == p.stageSkill) desc.stageSkills[1] = p.stageSkill;
			if(p.isBot) desc.stageSkills[1] = Config().DevBotStageSkill != 0 ? SkillID(Config().DevBotStageSkill) : MeasuredBotStageSkill(*stage, desc.stageSkills[1]);
		}
		desc.weapons = p.weapons;
		desc.weaponGrades = p.weaponGrades;
		desc.masterGearNo = p.masterGearNo;
		{
			const AccountGears gears = AccountGearsLoad(desc.name.data());
			const AccountInventory inventory = AccountInventoryLoad(desc.name.data());
			for(int mi = 0; mi < 2; mi++) {
				const i32 page = (i32)desc.masterGearNo[mi] - 1;
				if(page < 0 || page >= (i32)gears.pages.size()) continue;
				for(u32 uid : gears.pages[page].items) {
					if(uid == 0) continue;
					foreach_const(it, inventory.items) if(it->uid == uid && !desc.gearDocs[mi].full()) desc.gearDocs[mi].push_back(it->docIndex);
				}
			}
			if(!desc.gearDocs[0].empty() || !desc.gearDocs[1].empty()) LOG("Gear :: %ls pages (%d, %d) items (%d, %d)", desc.name.data(), (i32)desc.masterGearNo[0], (i32)desc.masterGearNo[1], (i32)desc.gearDocs[0].size(), (i32)desc.gearDocs[1].size());
		}
		desc.characterType = p.characterType;
		if(gameInfo.gameType == GameType::PVE_NORMAL || gameInfo.gameType == GameType::PVE_RANDOM || gameInfo.gameType == GameType::PVE_TICKET) {
			const AccountArsenal arsenal = AccountArsenalLoad(desc.name.data());
			for(int mi = 0; mi < 2; mi++) {
				if(p.masters[mi] == ClassType::NONE) continue;
				foreach_const(sk, xml.GetMaster(p.masters[mi]).skillIDs) {
					const i32 level = arsenal.SkillLevel((i32)*sk);
					if(level > 0 && !desc.stageSkillLevels.full()) desc.stageSkillLevels.push_back(eastl::make_pair((i32)*sk, (u8)level));
				}
			}
			if(!desc.stageSkillLevels.empty()) LOG("Arsenal :: %ls stage skill upgrades %d", desc.name.data(), (i32)desc.stageSkillLevels.size());
			const AccountAchievements achievements = AccountAchievementsLoad(desc.name.data());
			desc.statTitle = achievements.statTitle;
			desc.displayTitle = achievements.displayTitle;
			desc.badgeType = achievements.badge;
			desc.badgeTier = achievements.badge == MEASURED_BADGE_ACHIEVEMENT ? xml.AchievementTier(achievements.Score()) : 0;
			const i32 kit = AccountArsenalLoad(desc.name.data()).supportKit;
			if(kit > 0 && xml.FindSupportKit(kit) && xml.FindSupportKit(kit)->hpPercent > 0) {
				const AccountInventory inventory = AccountInventoryLoad(desc.name.data());
				foreach_const(it, inventory.items) if(it->docIndex == kit) desc.supportKitCount += it->count;
				if(desc.supportKitCount > 0) desc.supportKit = kit;
			}
		}

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

		const auto& spawnPoints = mapSpawnPoints[p.team].empty() ? mapSpawnPoints[p.team ^ 1] : mapSpawnPoints[p.team];
		const SpawnPoint fallback = { vec3(0), vec3(0) };
		const SpawnPoint& spawnPoint = spawnPoints.empty() ? fallback : spawnPoints[spawnPointIndex[p.team]++ % spawnPoints.size()];
		if(spawnPoints.empty()) WARN("Spawn :: map %d has no spawn point", (i32)replication.mapIndex);

		if(p.isBot) RollBotGear(desc);
		LOG("Spawn :: %ls team=%d masters=(%d, %d) skins=(%d, %d) weapons=(%d, %d)", desc.name.data(), (i32)desc.team, (i32)desc.masters[0], (i32)desc.masters[1], (i32)desc.skins[0], (i32)desc.skins[1], (i32)desc.weapons[0], (i32)desc.weapons[1]);
		World::Player& worldPlayer = world.CreatePlayer(desc, spawnPoint.pos, RotationHumanoid{0.f, 0.f, MxmYawToWorldYaw(spawnPoint.rot.z)});

		playerList.emplace_back(desc.clientHd, p.accountUID, desc.name, worldPlayer.index);
		if(worldPlayer.index < guildBoost.size()) guildBoost[worldPlayer.index] = { p.guildBoostGold, p.guildBoostExp };
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
				phaseTime = TimeAdd(localTime, TimeMsToTime(pveStage ? 0.0 : MAX(PRE_GAME_MIN_MS, LevelStartLeadMs())));

				foreach_const(p, playerList) {
					replication.SendPreGameLevelEvents(p->clientHd);
				}
				if(defenceStage) {
					const DefenceRecord best = DefenceWorldRecordLoad();
					foreach_const(p, world.players) {
						if(p->clientHd == ClientHandle::INVALID || p->removed) continue;
						const DefenceRecord mine = AccountDefenceRecordLoad(p->name.data());
						replication.SendDefenceBestRecord(p->clientHd, mine.step, mine.timeMs, best.step, best.timeMs);
					}
				}
			}
		} break;

		case Phase::PreGame: {
			if(TimeAdd(localTime, TimeMsToTime(LevelStartLeadMs())) >= phaseTime) world.StartLevelEvents();
			if(phaseTime < localTime) {
				phase = Phase::Game;

				// TODO: delete when inactive
				for(auto it = world.actorDynamicList.begin(); it != world.actorDynamicList.end();) {
					if(it->docID == CI_DOOR) { // door
						it->action = ActionStateID::DYNAMIC_OPEN;

						//world.actorDynamicMap.erase(it->UID);
						//it = world.actorDynamicList.erase(it);
						++it;
					}
					else {
						++it;
					}
				}

				foreach(p, playerList) {
					replication.SendGameStart(p->clientHd);
					p->gameStartSent = p->ready;
				}
				tGameStarted = localTime;
				gameStartUnix = (i64)time(nullptr);
				world.combatLocked = false;
				world.gameStartedAt = localTime;
				world.RollBotSiegeFrom();
				world.ApplyAllPassives();
				world.StartSkillPoints();
				foreach_const(p, world.players) world.SendTagCooltimeToTeam(*p, 0);
				world.StartTimeRecord();
				world.StartIngameLevels();
			}
		} break;

		case Phase::Game: {
			if(Config().DevAutoStage && pveStage && !gameEnded) {
				foreach(p, world.players) if(p->clientHd != ClientHandle::INVALID && !p->removed) p->devGod = true;
				if(tGameStarted != Time::ZERO) UpdateAutoStage();
			}
			if(world.portalRequested && !gameEnded) MoveToNextZone();
			if(pveStage && world.stageCleared && !gameEnded) EndStage(true);
			if(defenceStage && !gameEnded && (world.defence.won || world.defence.failed)) EndDefence();
			foreach(r, rewardStates) {
				if(r->finished || r->startedAt == Time::ZERO || r->picks >= r->slotCount) continue;
				if(TimeDurationMs(r->startedAt, localTime) >= (f64)MEASURED_REWARD_WAIT_MS) AutoPickRewards(r->clientHd);
			}
			if(pveStage && !gameEnded && tGameStarted != Time::ZERO && AllHumansDead()) EndStage(false);
			if(pveStage && !gameEnded && tGameStarted != Time::ZERO && localTime >= tNextStageRank) {
				tNextStageRank = TimeAdd(localTime, TimeMsToTime(MEASURED_STAGE_RANK_PERIOD_MS));
				foreach_const(p, world.players) {
					if(p->clientHd == ClientHandle::INVALID || p->removed) continue;
					replication.SendStageRank(p->clientHd, ComputeStageScore(*p, TimeDiffSec(TimeDiff(tGameStarted, localTime))).grade * MEASURED_STAGE_GRADE_SCALE, MEASURED_TIME_SCORE_RANK);
				}
			}
			if(!gameEnded && tGameStarted != Time::ZERO && world.winnerTeam < 0 && (replication.titanMode || world.occupyMode)) {
				const i32 scoreLimit = GetGameXmlContent().GetStageScoreLimit((i32)replication.stageIndex);
				for(i32 team = 0; team < 2 && scoreLimit > 0; team++) {
					if(world.TeamPoints(team) >= scoreLimit && world.TeamPoints(team) > world.TeamPoints(1 - team)) {
						world.winnerTeam = team;
						LOG("Game :: team %d reaches %d points, wins", team, scoreLimit);
					}
				}
			}
			if(!gameEnded && tGameStarted != Time::ZERO && world.winnerTeam >= 0) EndGame();
			if(!gameEnded && tGameStarted != Time::ZERO) {
				f32 limit = GetGameXmlContent().GetStageTimeLimit((i32)replication.stageIndex);
				if(limit <= 0) limit = STAGE_TIME_LIMIT_FALLBACK_SEC;
				if(TimeDiffSec(TimeDiff(tGameStarted, localTime)) >= limit) {
					if(pveStage) EndStage(false);
					else if(defenceStage) { world.defence.failed = true; EndDefence(); }
					else EndGame();
				}
			}
			const f64 lobbyDelay = defenceStage ? (f64)MEASURED_DEFENCE_RETURN_MS / 1000.0 : pveStage && !world.stageCleared ? (f64)MEASURED_PVE_FAIL_RETURN_MS / 1000.0 : pveStage ? PVE_REWARD_TIMEOUT_SEC : RETURN_TO_LOBBY_DELAY_SEC;
			if(gameEnded && !sentBackToLobby && TimeDiffSec(TimeDiff(tGameEnded, localTime)) >= lobbyDelay) {
				sentBackToLobby = true;
				foreach_const(p, playerList) {
					if(p->clientHd == ClientHandle::INVALID) continue;
					if(pveStage || defenceStage) replication.SendPrepareHandover(p->clientHd);
					else OnPlayerReturnToChannel(p->clientHd);
				}
				LOG("Game :: players sent back to the lobby");
			}
#if 1
			// bot random actions
			if(!gameEnded) foreach(bot, botList) {
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
					actionWeight[Action::Tag] = actionWeight[Action::Move] * MEASURED_BOT_TAG_SHARE / (1.0f - MEASURED_BOT_TAG_SHARE);
					actionWeight[Action::Jump] = 0;

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
						case Action::Move: break;

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

			// Dynamic walls going up and down randomly
			/*
			foreach(it, world.actorDynamicList) {
				if(it->docID == CI_WALL && TimeDiffSec(TimeDiff(it->tLastActionChange, localTime)) > 4.0) { // wall
					if(RandInt(0, 150) == 0) {
						if(it->action == ActionStateID::DYNAMIC_NORMAL_STAND || it->action == ActionStateID::DYNAMIC_CLOSE) {
							it->action = ActionStateID::DYNAMIC_OPEN;
						}
						else {
							it->action = ActionStateID::DYNAMIC_CLOSE;
						}

						it->tLastActionChange = localTime;
					}
				}
			}
			*/

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
	const GameXmlContent::Map* map = content.FindPvpMap(replication.mapIndex);
	if(!map) {
		WARN("LoadMap> map %d has no server data, falling back to deathmatch", (i32)replication.mapIndex);
		map = content.FindPvpMap(MapIndex::PVP_DEATHMATCH);
	}
	world.map = map;
	const GameXmlContent::TitanInfo& titan = content.titan;
	if(map->ingameLevels && !titan.forms.empty() && !titan.giants.empty()) {
		world.giantCycle = RandUint() % (u32)titan.giants.size();
		replication.titanMode = true;
		replication.titanNextIndex = (i8)world.giantCycle;
	}
	world.physics.SetGroundPlaneEnabled(map->groundPlane);
	world.killZ = map->groundPlane ? -1000000.0f : map->killZ;
	foreach_const(c, map->colliders) {
		if(!c->walk) continue;
		world.physics.CreateStaticCollider(c->mesh.data(), c->pos, c->rot, c->scale);
	}

	foreach(it, map->creatures) {
		if(!it->IsSpawnPoint() && !(it->difficultyMask & world.difficultyBit)) continue;
		// don't spawn "spawn points"
		if(it->IsSpawnPoint()) {
			if(it->faction == Faction::RED || it->faction == Faction::BLUE) {
				mapSpawnPoints[(i32)it->faction].push_back(SpawnPoint{ it->pos, it->rot });
			}
			continue;
		}

		// spawn npc
		World::ActorNpc& actor = world.SpawnNpcActor(it->docID, it->localID);
		actor.pos = it->pos;
		actor.rot = it->rot;
		actor.home = it->pos;
		actor.faction = world.hostileMonsters && it->faction == Faction::INVALID ? Faction::MONSTER : it->faction;
		actor.spawnAnim = it->spawnAnim;
		actor.wanderDist = it->wanderDist;
		actor.tagID = it->tagID;
		actor.ownerID = it->ownerID;
		actor.dirToNearPC = it->dirToNearPC;
		actor.actionState = it->actionState;
		actor.entityType = it->entityType;
		actor.seed = (i32)RandUint();
		world.InitMonsterStats(actor);
	}

	foreach(it, map->dynamic) {
		if(!(it->difficultyMask & world.difficultyBit)) continue;
		if(world.hostileMonsters && GetGameXmlContent().FindMonsterAi((i32)it->docID)) {
			World::ActorNpc& npc = world.SpawnNpcActor(it->docID, it->localID);
			npc.pos = it->pos;
			npc.rot = it->rot;
			npc.home = it->pos;
			npc.spawnAnim = it->spawnAnim;
			npc.tagID = it->tagID;
			npc.ownerID = it->ownerID;
			npc.dirToNearPC = it->dirToNearPC;
			npc.actionState = it->actionState;
			npc.entityType = it->entityType;
			npc.seed = (i32)RandUint();
			world.InitMonsterStats(npc);
			npc.faction = npc.monsterAi->playerSide ? world.PlayerSideFaction() : Faction::MONSTER;
			continue;
		}
		// spawn dynamic
		auto& actor = world.SpawnDynamic(it->docID, it->localID);
		actor.pos = it->pos;
		actor.rot = it->rot;
		actor.faction = it->faction;
		actor.spawnAnim = it->spawnAnim;
		actor.wanderDist = it->wanderDist;
		actor.tagID = it->tagID;
		actor.ownerID = it->ownerID;
		actor.dirToNearPC = it->dirToNearPC;
		actor.action = it->actionState;
		actor.entityType = it->entityType;
		actor.seed = (i32)RandUint();
		const auto meshKey = GetGameXmlContent().dynamicMeshKey.find((i32)it->docID);
		if(map->ingameLevels && meshKey != GetGameXmlContent().dynamicMeshKey.end()) {
			eastl::fixed_string<char,100,false> meshName;
			meshName.sprintf("%s/%s", meshKey->second.c_str(), meshKey->second.c_str());
			if(world.physics.HasStaticMesh(meshName.c_str())) actor.collider = world.physics.CreateStaticCollider(meshName.c_str(), actor.pos, actor.rot, it->scale, GetGameXmlContent().dynamicBlockSight.count((i32)it->docID) > 0);
		}
	}

	if(map->index != MapIndex::PVP_DEATHMATCH) return true;

	// spawn walls dynamically

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

	foreach(it, map->areas) {
		if(wallConf.find(it->ID) != wallConf.end()) {
			auto& actor = world.SpawnDynamic(CI_WALL, it->ID);
			actor.pos = it->pos;
			actor.rot = it->rot;
			actor.collider = world.physics.CreateStaticCollider("PvP_Death_NM_Wall04_GuardrailMob", it->pos, it->rot);
		}
	}
	return true;
}

void Game::UpdateAutoStage()
{
	static const i32 AUTO_STAGE_PERIOD_MS = 3000;
	static const i32 AUTO_STAGE_STUCK_MS = 20000;
	static const i32 AUTO_STAGE_TELEPORT_WAIT_MS = 8000;
	static const i32 AUTO_STAGE_RUNAWAY_KILLS = 400;
	if(zoneLoading || localTime < tAutoStageNext) return;
	tAutoStageNext = TimeAdd(localTime, TimeMsToTime(AUTO_STAGE_PERIOD_MS));
	if(autoStageZone != zoneIndex) {
		autoStageZone = zoneIndex;
		autoStageZoneKills = 0;
		autoStagePendingWaits = 0;
		autoStageVisited.clear();
		tAutoStageWait = TimeAdd(localTime, TimeMsToTime(AUTO_STAGE_STUCK_MS));
	}
	World::Player* killer = nullptr;
	foreach(p, world.players) {
		if(p->clientHd == ClientHandle::INVALID || p->removed) continue;
		p->devGod = true;
		if(!killer) killer = &*p;
	}
	if(!killer) return;
	i32 killed = 0;
	foreach(npc, world.actorNpcList) {
		if(npc->hp <= 0 || npc->removeAt != Time::ZERO || npc->ownerUID != ActorUID::INVALID || npc->faction != Faction::MONSTER) continue;
		const i32 hpBefore = npc->hp;
		npc->hp = 0;
		world.OnMonsterDamaged(*npc, hpBefore);
		world.MonsterDie(*npc, killer->Main().UID, CreatureIndex(100000000 + (i32)killer->Main().classType));
		killed++;
	}
	const auto& destroyHits = GetGameXmlContent().dynamicDestroyHits;
	foreach(d, world.actorDynamicList) {
		if(d->tagID < 0 || d->destroyed || destroyHits.find((i32)d->docID) == destroyHits.end()) continue;
		world.DestroyDynamic(*d, (i32)killer->index);
		killed++;
	}
	autoStageZoneKills += killed;
	static const i32 AUTO_STAGE_WAVE_KILLS = 3;
	if(killed > 0 && autoStageZoneKills < AUTO_STAGE_RUNAWAY_KILLS) {
		LOG("AutoStage :: zone %d killed %d monster(s)", zoneIndex, killed);
		if(killed >= AUTO_STAGE_WAVE_KILLS) {
			tAutoStageWait = TimeAdd(localTime, TimeMsToTime(AUTO_STAGE_STUCK_MS));
			return;
		}
	}
	const bool runaway = autoStageZoneKills >= AUTO_STAGE_RUNAWAY_KILLS;
	if(runaway) LOG("AutoStage :: zone %d runaway spawns (%d kills)", zoneIndex, autoStageZoneKills);
	else if(localTime < tAutoStageWait) return;
	if(world.map && killer->body) {
		const vec2 here = vec2(killer->body->GetWorldPos());
		const GameXmlContent::LevelEventNode* nearest = nullptr;
		const GameXmlContent::Map::Area* nearestArea = nullptr;
		f32 nearestDist = 0.0f;
		foreach_const(n, world.map->levelEvents) {
			if(n->kind != GameXmlContent::LevelEventNode::BOUND_TRIGGER || !n->enter || !world.LevelTriggerActive(*n)) continue;
			if(eastl::find(autoStageVisited.begin(), autoStageVisited.end(), n->id) != autoStageVisited.end()) continue;
			const GameXmlContent::Map::Area* area = world.map->FindArea(n->areaID);
			if(!area) continue;
			const f32 dist = glm::length(vec2(area->pos) - here);
			if(nearest && dist >= nearestDist) continue;
			nearest = &*n;
			nearestArea = area;
			nearestDist = dist;
		}
		if(nearest) {
			autoStageVisited.push_back(nearest->id);
			world.WarpPlayer(*killer, nearestArea->pos, killer->input.rot.bodyYaw);
			tAutoStageWait = TimeAdd(localTime, TimeMsToTime(AUTO_STAGE_TELEPORT_WAIT_MS));
			LOG("AutoStage :: zone %d teleports to trigger area %d (node %d, %.0f away)", zoneIndex, nearest->areaID, nearest->id, nearestDist);
			return;
		}
	}
	static const i32 AUTO_STAGE_MAX_PENDING_WAITS = 6;
	tAutoStageWait = TimeAdd(localTime, TimeMsToTime(AUTO_STAGE_STUCK_MS));
	if(!runaway && (!world.levelEventTimers.empty() || !world.monsterCalls.empty()) && autoStagePendingWaits < AUTO_STAGE_MAX_PENDING_WAITS) {
		autoStagePendingWaits++;
		LOG("AutoStage :: zone %d waits for %d timer(s) and %d call(s)", zoneIndex, (i32)world.levelEventTimers.size(), (i32)world.monsterCalls.size());
		return;
	}
	autoStageZoneKills = 0;
	if(zoneIndex + 1 < (i32)zoneMaps.size()) {
		LOG("AutoStage :: zone %d done, forcing the portal", zoneIndex);
		world.portalRequested = true;
	}
	else {
		LOG("AutoStage :: zone %d done, forcing the clear", zoneIndex);
		world.stageCleared = true;
	}
}

void Game::MoveToNextZone()
{
	world.portalRequested = false;
	if(zoneIndex + 1 >= (i32)zoneMaps.size()) {
		LOG("Stage :: portal without a next zone");
		return;
	}
	zoneIndex++;
	replication.mapIndex = zoneMaps[zoneIndex];
	LOG("Stage :: moving to zone %d (map %d)", zoneIndex, (i32)replication.mapIndex);
	foreach_const(p, playerList) {
		if(p->clientHd == ClientHandle::INVALID) continue;
		replication.SendGameAreaMove(p->clientHd, replication.mapIndex);
		replication.ResetClientForAreaMove(p->clientHd);
	}
	world.ClearMapContent();
	foreach(p, world.players) world.ResetMainMasterForAreaMove(*p);
	foreach(sp, mapSpawnPoints) sp->clear();
	LoadMap();
	PlacePlayersAtSpawn();
	zoneLoading = true;
}

static u8 RewardSlotsForGrade(u8 grade)
{
	static const u8 MEASURED_SLOTS_SSS = 6;
	static const u8 HYP_SLOTS_SS = 5;
	static const u8 MEASURED_SLOTS_S = 5;
	static const u8 MEASURED_SLOTS_A = 4;
	static const u8 HYP_SLOTS_B = 3;
	static const u8 MEASURED_SLOTS_C = 3;
	if(grade == MEASURED_STAGE_GRADE_SSS) return MEASURED_SLOTS_SSS;
	if(grade == MEASURED_STAGE_GRADE_SS) return HYP_SLOTS_SS;
	if(grade == MEASURED_STAGE_GRADE_S) return MEASURED_SLOTS_S;
	if(grade == MEASURED_STAGE_GRADE_A) return MEASURED_SLOTS_A;
	if(grade == MEASURED_STAGE_GRADE_B) return HYP_SLOTS_B;
	return MEASURED_SLOTS_C;
}

Game::StageScore Game::ComputeStageScore(const World::Player& player, f64 playSec) const
{
	static const f32 MEASURED_WEIGHT_ATTACK = 0.25f;
	static const f32 MEASURED_WEIGHT_DEFENSE = 0.30f;
	static const f32 MEASURED_WEIGHT_TIME = 0.45f;
	static const i32 MEASURED_ATTACK_SCORE_MAX = 99;
	static const u32 MEASURED_TOTAL_MIN_S = 90;
	static const u32 HYP_TOTAL_MIN_A = 60;
	static const u32 HYP_TOTAL_MIN_B = 50;
	static const f64 HYP_TIME_PAR_SEC_BY_DIFFICULTY[] = { 210.0, 210.0, 270.0, 600.0, 600.0 };
	static const f64 MEASURED_TIME_SCORE_PER_OVER_RATIO = 134.0;
	f64 par = HYP_TIME_PAR_SEC_BY_DIFFICULTY[1];
	for(i32 d = 0; d < (i32)ARRAY_COUNT(HYP_TIME_PAR_SEC_BY_DIFFICULTY); d++) if(world.difficultyBit == (u8)(1 << d)) par = HYP_TIME_PAR_SEC_BY_DIFFICULTY[d];
	StageScore s;
	s.attack = (u32)MIN(player.pveKills * HYP_ATTACK_SCORE_PER_KILL, MEASURED_ATTACK_SCORE_MAX);
	s.defense = (u32)MAX(100 - player.deaths * HYP_DEFENSE_SCORE_PER_DEATH, 0);
	s.time = (u32)MIN(MAX(100.0 - MEASURED_TIME_SCORE_PER_OVER_RATIO * (playSec / par - 1.0), 0.0), 100.0);
	s.total = (u32)(s.attack * MEASURED_WEIGHT_ATTACK + s.defense * MEASURED_WEIGHT_DEFENSE + s.time * MEASURED_WEIGHT_TIME + 0.5f);
	static const i32 HYP_TECHNIC_GOOD_COMBO = 30;
	static const i32 HYP_TECHNIC_GREAT_COMBO = 60;
	static const u32 MEASURED_TOTAL_MIN_SSS = 98;
	static const u32 HYP_TOTAL_MIN_SS = 95;
	s.technic = player.bestHitCombo >= HYP_TECHNIC_GREAT_COMBO ? HYP_TECHNIC_GREAT : player.bestHitCombo >= HYP_TECHNIC_GOOD_COMBO ? HYP_TECHNIC_GOOD : HYP_TECHNIC_NORMAL;
	s.grade = s.total >= MEASURED_TOTAL_MIN_S ? MEASURED_STAGE_GRADE_S : s.total >= HYP_TOTAL_MIN_A ? MEASURED_STAGE_GRADE_A : s.total >= HYP_TOTAL_MIN_B ? MEASURED_STAGE_GRADE_B : MEASURED_STAGE_GRADE_C;
	if(s.grade == MEASURED_STAGE_GRADE_S && s.technic == HYP_TECHNIC_GREAT) s.grade = s.total >= MEASURED_TOTAL_MIN_SSS ? MEASURED_STAGE_GRADE_SSS : s.total >= HYP_TOTAL_MIN_SS ? MEASURED_STAGE_GRADE_SS : MEASURED_STAGE_GRADE_S;
	return s;
}

bool Game::AllHumansDead() const
{
	i32 humans = 0;
	foreach_const(p, world.players) {
		if(p->clientHd == ClientHandle::INVALID || p->removed) continue;
		humans++;
		if(!p->dead) return false;
	}
	return humans > 0;
}

void Game::PlacePlayersAtSpawn()
{
	i32 spawnPointIndex[2] = { 0 };
	foreach(p, world.players) {
		if(p->removed || p->team > 1) continue;
		if(p->dead) world.RespawnPlayer(*p);
		const auto& spawnPoints = mapSpawnPoints[p->team];
		if(spawnPoints.empty()) continue;
		const SpawnPoint& sp = spawnPoints[spawnPointIndex[p->team]++ % spawnPoints.size()];
		p->spawnPos = sp.pos;
		world.TeleportPlayer(*p, sp.pos, MxmYawToWorldYaw(sp.rot.z));
	}
}

void Game::OnPlayerDisconnect(ClientHandle clientHd)
{
	if(playerMap.find(clientHd) == playerMap.end()) return;

	// TODO: KILL MASTERS
	replication.OnPlayerDisconnect(clientHd);
}

void Game::OnPlayerReconnect(ClientHandle clientHd, AccountUID accountUID)
{
	foreach(it, playerList) {
		if(it->accountUID != accountUID) continue;
		const ClientHandle oldHd = it->clientHd;
		World::Player& player = world.GetPlayer(it->playerIndex);
		if(oldHd != ClientHandle::INVALID) {
			replication.OnPlayerDisconnect(oldHd);
			playerMap.erase(oldHd);
		}
		for(auto b = botList.begin(); b != botList.end(); ++b) {
			if(b->playerIndex != it->playerIndex) continue;
			botList.erase(b);
			break;
		}
		it->clientHd = clientHd;
		it->left = false;
		it->returnSent = false;
		it->ready = false;
		it->gameStartSent = false;
		player.clientHd = clientHd;
		playerMap.emplace(clientHd, it);
		replication.OnPlayerConnect(clientHd, player.index);
		replication.PlayerRegisterMasterActor(clientHd, player.Main().UID, player.mainClass);
		replication.PlayerRegisterMasterActor(clientHd, player.Sub().UID, player.subClass);
		LOG("[client%x] Game :: %ls reconnected (previous client%x)", clientHd, player.name.data(), oldHd);
		return;
	}
	WARN("[client%x] Game :: reconnecting account %u is not in this game", clientHd, accountUID);
}

void Game::OnPlayerReadyToLoad(ClientHandle clientHd)
{
	replication.SendLoadPvpMap(clientHd, replication.mapIndex);
}

void Game::OnPlayerGetCharacterInfo(ClientHandle clientHd, ActorUID actorUID)
{
	Player& p = *playerMap.at(clientHd);
	const World::Player& player = world.GetPlayer(p.playerIndex);
	ASSERT(player.clientHd == clientHd);

	foreach_const(chit, player.characters) {
		const World::ActorMaster& chara = **chit;

		if(chara.UID == actorUID) {
			replication.SendCharacterInfo(clientHd, chara.UID, CreatureIndex(100000000 + (i32)chara.classType), chara.classType, chara.hp, chara.hpMax);



			return;
		}
	}

	WARN("Client sent an invalid actorUID (clientHd=%x actorUID=%u)", clientHd, (u32)actorUID);
}

void Game::OnPlayerUpdatePosition(ClientHandle clientHd, ActorUID actorUID, const vec3& pos, const vec2& dir, const RotationHumanoid& rot, f32 speed, ActionStateID action, f32 clientTime, bool force)
{
	ProfileFunction();

	Player& p = *playerMap.at(clientHd);
	World::Player& player = world.GetPlayer(p.playerIndex);
	ASSERT(player.clientHd == clientHd);

	if(!IsFinite(pos.x) || !IsFinite(pos.y) || !IsFinite(pos.z) || !IsFinite(dir.x) || !IsFinite(dir.y) || !IsFinite(speed)) {
		WARN("Client sent a non finite position (clientHd=%x actorUID=%u)", clientHd, (u32)actorUID);
		return;
	}

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


	const bool clientBlocked = dir != vec2(0) && player.lastClientDir != vec2(0) && player.clientPosCount > 0 && glm::length(vec2(pos) - vec2(player.lastClientPos)) < HYP_CLIENT_BLOCKED_STEP;
	vec2 slideVel = vec2(0);
	bool sliding = false;
	const f32 stepDt = player.lastClientPosAt != Time::ZERO ? (f32)TimeDiffSec(TimeDiff(player.lastClientPosAt, world.localTime)) : 0.0f;
	if(!clientBlocked && dir != vec2(0) && player.lastClientDir != vec2(0) && player.clientPosCount > 0 && speed > 0 && stepDt >= HYP_CLIENT_SLIDE_MIN_SEC && stepDt <= HYP_CLIENT_SLIDE_MAX_SEC && glm::dot(glm::normalize(vec2(dir)), glm::normalize(vec2(player.lastClientDir))) > HYP_CLIENT_SAME_DIR_DOT) {
		const vec2 step = vec2(pos) - vec2(player.lastClientPos);
		const f32 stepLen = glm::length(step);
		if(glm::dot(step, glm::normalize(vec2(dir))) < speed * stepDt * HYP_CLIENT_SLIDE_RATIO || (stepLen > 1.0f && glm::dot(glm::normalize(vec2(dir)), glm::normalize(vec2(player.lastClientDir))) > HYP_CLIENT_SLIDE_DIR_DOT && glm::dot(step / stepLen, glm::normalize(vec2(player.lastClientDir))) < HYP_CLIENT_SLIDE_DIR_DOT)) {
			sliding = true;
			slideVel = step / stepDt;
		}
	}
	player.lastClientPos = pos;
	player.lastClientDir = dir;
	player.lastClientPosAt = world.localTime;
	player.clientPosCount++;
	if(player.clientPushSnap && world.localTime < player.clientPushUntil && TimeDiffSec(TimeDiff(player.clientPushStart, world.localTime)) < HYP_CLIENT_PUSH_INFLIGHT_SEC) player.clientPushPosCount = player.clientPosCount;
	if(player.carriedBy >= 0) return;
	if(world.localTime < player.dashUntil) LOG("Dash :: %ls client position during dash action=%d cast=%d dir=(%.2f, %.2f) gap=%.0f", player.name.data(), (i32)action, (i32)player.lastCastAction, dir.x, dir.y, glm::length(vec2(pos) - vec2(player.body->GetWorldPos())));
	if(action != ActionStateID::INVALID && action != player.lastClientAction) {
		if(player.lastClientAction == player.lastCastAction && player.lastCastAction != ActionStateID::INVALID) world.EndActionStatuses(player);
		player.lastClientAction = action;
		player.lastClientActionSince = world.localTime;
	}
	if(world.localTime < player.dashUntil && player.dashBlockUntil != Time::ZERO && world.localTime >= player.dashBlockUntil && world.localTime >= player.ccUntil && dir != vec2(0)) {
		player.dashDone = player.dashDist;
		player.dashUntil = world.localTime;
		LOG("Dash :: %ls moves again after the state block, server dash ended", player.name.data());
	}
	const vec3 bodyPos = player.body->GetWorldPos();
	const bool drifted = glm::length(vec2(pos) - vec2(bodyPos)) > HYP_HUMAN_POSITION_SNAP || fabsf(pos.z - bodyPos.z) > HYP_HUMAN_POSITION_SNAP;
	const bool awaitStart = player.dashAwaitClientStart && !force;
	if(awaitStart) player.dashAwaitClientStart = false;
	if(awaitStart && glm::length(vec2(pos) - vec2(player.dashOrigin)) <= HYP_HUMAN_POSITION_SNAP) {
		const vec2 shift = vec2(pos) - vec2(player.dashOrigin);
		player.dashOrigin += vec3(shift, 0);
		player.body->collider->setFootPosition(PxExtendedVec3(bodyPos.x + shift.x, bodyPos.y + shift.y, bodyPos.z));
		player.body->vel = vec3(0);
	}
	else if(force || world.localTime < player.ccUntil || (world.localTime >= player.dashUntil && drifted) || glm::length(vec2(pos) - vec2(bodyPos)) > HYP_HUMAN_DASH_SNAP) {
		player.body->collider->setFootPosition(PxExtendedVec3(pos.x, pos.y, pos.z));
		player.body->vel = vec3(0);
		if(player.dashDone < player.dashDist) player.dashOrigin = pos - vec3(player.dashDir * player.dashDone, 0);
	}

	// TODO: check for movement hacking
	if((dir.x == 0 && dir.y == 0) || clientBlocked) {
		player.input.moveTo = pos;
	}
	else if(sliding) {
		player.input.moveTo = pos + vec3(slideVel, 0);
	}
	else {
		player.input.moveTo = pos + vec3(glm::normalize(vec2(dir)) * speed, 0);
	}
	player.input.rot = rot;
	player.input.speed = sliding ? MIN(glm::length(slideVel), speed) : speed;
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
	const Player* destPlayer = nullptr;
	foreach_const(pl, playerList) {
		if(pl->name.compare(destNick) == 0) {
			destPlayer = &*pl;
			break;
		}
	}

	if(!destPlayer || destPlayer->IsBot()) {
		replication.SendChatWhisperConfirmToClient(clientHd, destNick, msg, ErrorType::WHISPER_SEND_NOT_FOUND);
		return;
	}

	replication.SendChatWhisperConfirmToClient(clientHd, destNick, msg, ErrorType::SUCCESS);
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

static const u8 MEASURED_TRAINING_EVENT_LOADOUT = 4;
static const u8 MEASURED_TRAINING_EVENT_CREATE_AI = 5;
static const u8 MEASURED_TRAINING_EVENT_REMOVE_AI = 6;
static const u8 MEASURED_TRAINING_EVENT_RESET_SKILLS = 0;
static const i32 HYP_TRAINING_BOT_SUB_OFFSET = 3;
static const wchar* MEASURED_TRAINING_BOT_NAME = L"BOT_NAME_02";
static const i32 MEASURED_MASTER_CREATURE_BASE = 100000000;
static const u8 MEASURED_MASTER_GEAR_SETS = 11;

static WeaponIndex DefaultMasterWeapon(const GameXmlContent::Master& master)
{
	if(!master.fairPvpWeaponIDs.empty()) return master.fairPvpWeaponIDs[0];
	if(!master.defaultWeaponIDs.empty()) return master.defaultWeaponIDs[0];
	return master.weaponIDs.size() > 1 ? master.weaponIDs[1] : master.weaponIDs[0];
}

static bool MasterHasSkill(const GameXmlContent::Master& master, SkillID skill)
{
	foreach_const(s, master.skillIDs) if(*s == skill) return true;
	return false;
}

static bool MasterHasSkin(const GameXmlContent::Master& master, SkinIndex skin)
{
	foreach_const(s, master.skinIDs) if(*s == skin) return true;
	return skin == SkinIndex::DEFAULT;
}

void Game::OnGamePing(ClientHandle clientHd, LocalActorID chara, const float3& pos, LocalActorID target, u32 ping)
{
	auto pl = playerMap.find(clientHd);
	if(pl == playerMap.end()) return;
	const World::Player& sender = world.GetPlayer(pl->second->playerIndex);
	const ActorUID charaUID = replication.GetWorldActorUID(clientHd, chara);
	const ActorUID targetUID = target == LocalActorID::INVALID ? ActorUID::INVALID : replication.GetWorldActorUID(clientHd, target);
	foreach_const(p, world.players) {
		if(p->clientHd == ClientHandle::INVALID || p->removed || p->team != sender.team) continue;
		PacketWriter<Sv::SN_BroadcastGamePingData,32> packet;
		packet.Write<LocalActorID>(charaUID == ActorUID::INVALID ? LocalActorID::INVALID : replication.GetLocalActorID(p->clientHd, charaUID));
		packet.Write<float3>(pos);
		packet.Write<LocalActorID>(targetUID == ActorUID::INVALID ? LocalActorID::INVALID : replication.GetLocalActorID(p->clientHd, targetUID));
		packet.Write<u32>(ping);
		replication.SendWriter(p->clientHd, packet);
	}
}

void Game::OnTrainingEquipSkill(ClientHandle clientHd, LocalActorID chara, SkillID skill, u32 slot)
{
	auto pl = playerMap.find(clientHd);
	const GameXmlContent::StageMaps* stage = GetGameXmlContent().FindStageMaps(replication.stageIndex);
	if(pl == playerMap.end() || !stage || stage->gameMode != GameModeType::GAME_MODE_MASTER_TRAINING || slot > 1) return;
	World::Player& player = world.GetPlayer(pl->second->playerIndex);
	for(u32 mi = 0; mi < 2; mi++) {
		if(replication.GetLocalActorID(clientHd, player.characters[mi]->UID) != chara) continue;
		const GameXmlContent::Master& master = GetGameXmlContent().GetMaster(player.characters[mi]->classType);
		if(!MasterHasSkill(master, skill)) return;
		world.ResetSkillPoints(player);
		SkillID& target = player.skills[mi * 2 + slot];
		SkillID& other = player.skills[mi * 2 + (slot ^ 1)];
		if(other == skill) other = target;
		target = skill;
		PacketWriter<Sv::SA_SkillSelect,32> answer;
		answer.Write<LocalActorID>(chara);
		answer.Write<u32>(0);
		answer.Write<SkillID>(skill);
		answer.Write<u32>(slot);
		replication.SendWriter(clientHd, answer);
		LOG("Training :: %ls equips skill %d in slot %u of master %u", player.name.data(), (i32)skill, slot, mi);
		return;
	}
}

void Game::ChangeTrainingLoadout(ClientHandle clientHd, const Cl::CN_UNKNOWN_60296::SlotPairData& slots)
{
	auto pl = playerMap.find(clientHd);
	if(pl == playerMap.end()) return;
	World::Player& player = world.GetPlayer(pl->second->playerIndex);
	const GameXmlContent& xml = GetGameXmlContent();
	const u32 slotClass[2] = { slots.field0, slots.field6 };
	const u32 slotSkills[2][2] = { { slots.field1, slots.field2 }, { slots.field8, slots.field9 } };
	const u32 slotSkin[2] = { slots.field3, slots.field7 };
	const u8 slotGearNo[2] = { slots.field5, slots.field11 };
	World::PlayerDescription desc;
	desc.userID = player.userID;
	desc.clientHd = player.clientHd;
	desc.name = player.name;
	desc.guildTag = player.guildTag;
	desc.team = player.team;
	desc.stageSkills = player.stageSkills;
	for(int mi = 0; mi < 2; mi++) {
		const ClassType cls = (ClassType)((i32)slotClass[mi] - MEASURED_MASTER_CREATURE_BASE);
		auto found = xml.masterClassTypeMap.find(cls);
		if(found == xml.masterClassTypeMap.end() || found->second->weaponIDs.empty() || found->second->skillIDs.size() < 2) {
			WARN("Training :: loadout with unknown master %u", slotClass[mi]);
			return;
		}
		const GameXmlContent::Master& master = *found->second;
		desc.masters[mi] = cls;
		const SkinIndex skin = (SkinIndex)slotSkin[mi];
		desc.skins[mi] = MasterHasSkin(master, skin) ? skin : SkinIndex::DEFAULT;
		for(int si = 0; si < 2; si++) {
			const SkillID skill = (SkillID)slotSkills[mi][si];
			desc.skills[mi * 2 + si] = MasterHasSkill(master, skill) ? skill : master.skillIDs[si];
		}
		desc.weapons[mi] = DefaultMasterWeapon(master);
		desc.weaponGrades[mi] = 0;
		desc.masterGearNo[mi] = slotGearNo[mi] >= 1 && slotGearNo[mi] <= MEASURED_MASTER_GEAR_SETS ? slotGearNo[mi] : 1;
		desc.characterType[mi] = 1;
		desc.colliderSize[mi] = { (u16)master.character.getColliderRadius(), (u16)master.character.getColliderHeight() };
	}
	if(desc.masters[0] == desc.masters[1]) {
		WARN("Training :: loadout with the same master twice (%d)", (i32)desc.masters[0]);
		return;
	}
	world.ReplacePlayerMasters(player, desc);
	replication.PlayerRegisterMasterActor(clientHd, player.characters[0]->UID, player.mainClass);
	replication.PlayerRegisterMasterActor(clientHd, player.characters[1]->UID, player.subClass);
}

void Game::OnMasterTrainingEvent(ClientHandle clientHd, const u8* data, i32 size)
{
	const u8 eventType = data[0];
	replication.SendMasterTrainingEvent(clientHd, eventType, 0, 0, 0, 0);
	if(eventType == MEASURED_TRAINING_EVENT_LOADOUT && size >= (i32)sizeof(Cl::CN_UNKNOWN_60296)) {
		ChangeTrainingLoadout(clientHd, ((const Cl::CN_UNKNOWN_60296*)data)->slot_pair_data);
		replication.SendMasterTrainingEvent(clientHd, eventType, 0, 1, 1, 1);
	}
	if(eventType == MEASURED_TRAINING_EVENT_CREATE_AI && size >= 9) {
		const i32 choice = *(const i32*)(data + 5);
		const auto* classes = GetGameXmlContent().FindStageBots(replication.stageIndex);
		if(!classes || choice < 1 || choice > (i32)classes->size()) return;
		const i32 n = (i32)classes->size();
		const ClassType mainClass = (*classes)[choice - 1];
		const ClassType subClass = (*classes)[(choice - 1 + HYP_TRAINING_BOT_SUB_OFFSET) % n];
		auto pl = playerMap.find(clientHd);
		const u8 humanTeam = pl != playerMap.end() ? world.GetPlayer(pl->second->playerIndex).team : 0;
		SpawnTrainingBot(mainClass, subClass, humanTeam ^ 1);
	}
	if(eventType == MEASURED_TRAINING_EVENT_REMOVE_AI) {
		foreach_const(t, trainingBots) {
			world.RemovePlayer(world.GetPlayer(*t));
			for(auto b = botList.begin(); b != botList.end(); ++b) {
				if(b->playerIndex != *t) continue;
				botList.erase(b);
				break;
			}
		}
		trainingBots.clear();
	}
	if(eventType == MEASURED_TRAINING_EVENT_RESET_SKILLS) {
		auto pl = playerMap.find(clientHd);
		if(pl != playerMap.end()) world.ResetSkillPoints(world.GetPlayer(pl->second->playerIndex));
	}
}

void Game::OnTitanIncarnate(ClientHandle clientHd, i32 formDoc)
{
	auto pl = playerMap.find(clientHd);
	if(pl == playerMap.end()) return;
	const GameXmlContent::TitanInfo& titan = GetGameXmlContent().titan;
	for(i32 i = 0; i < (i32)titan.forms.size(); i++) {
		if(titan.forms[i].doc != formDoc) continue;
		world.TransformTitan(world.GetPlayer(pl->second->playerIndex), i, true);
		return;
	}
	WARN("Training :: unknown titan form %d", formDoc);
}

void Game::SpawnTrainingBot(ClassType mainClass, ClassType subClass, u8 team)
{
	if(world.players.full()) return;
	const GameXmlContent& xml = GetGameXmlContent();
	World::PlayerDescription desc;
	desc.userID = UserID(world.players.size() + 1);
	desc.clientHd = ClientHandle::INVALID;
	desc.name.assign(MEASURED_TRAINING_BOT_NAME);
	desc.team = team;
	const ClassType classes[2] = { mainClass, subClass };
	for(int mi = 0; mi < 2; mi++) {
		const GameXmlContent::Master& master = *xml.masterClassTypeMap.at(classes[mi]);
		desc.masters[mi] = classes[mi];
		desc.skins[mi] = SkinIndex::DEFAULT;
		desc.skills[mi * 2] = master.skillIDs[0];
		desc.skills[mi * 2 + 1] = master.skillIDs[1];
		desc.weapons[mi] = DefaultMasterWeapon(master);
		desc.weaponGrades[mi] = 0;
		desc.masterGearNo[mi] = 1;
		desc.characterType[mi] = 1;
		desc.colliderSize[mi] = { (u16)master.character.getColliderRadius(), (u16)master.character.getColliderHeight() };
	}
	desc.stageSkills = { SkillID::INVALID, SkillID::INVALID };
	RollBotGear(desc);
	const auto& spawnPoints = mapSpawnPoints[team];
	const vec3 pos = spawnPoints.empty() ? world.GetPlayer(0).spawnPos : spawnPoints[0].pos;
	const f32 yaw = spawnPoints.empty() ? 0.f : MxmYawToWorldYaw(spawnPoints[0].rot.z);
	World::Player& worldPlayer = world.CreatePlayer(desc, pos, RotationHumanoid{0.f, 0.f, yaw});
	playerList.emplace_back(desc.clientHd, AccountUID::INVALID, desc.name, worldPlayer.index);
	botList.emplace_back(worldPlayer.index);
	if(!trainingBots.full()) trainingBots.push_back(worldPlayer.index);
	LOG("Training :: bot masters=(%d, %d) team=%d", (i32)mainClass, (i32)subClass, (i32)team);
}

void Game::OnPlayerTag(ClientHandle clientHd, ActorUID actorUID)
{
	Player& p = *playerMap.at(clientHd);
	World::Player& player = world.GetPlayer(p.playerIndex);
	ASSERT(player.clientHd == clientHd);

	// TODO: cooldown
	player.input.tag = 1;
}

f64 Game::LevelStartLeadMs() const
{
	return world.map ? (f64)world.map->levelStartLead * 1000.0 : LEVEL_START_BEFORE_GAME_MS;
}

void Game::EndGame()
{
	if(world.winnerTeam >= 0) world.FireGameEndTriggers(world.winnerTeam);
	gameEnded = true;
	tGameEnded = localTime;
	world.frozen = true;
	eastl::fixed_vector<Replication::EndPlayer,10,false> list;
	i32 red = 0, blue = 0;
	for(u32 i = 0; i < world.PlayerCount(); i++) {
		World::Player& p = world.GetPlayer(i);
		world.AccumulateMasterScore(p);
		Replication::EndPlayer e;
		e.playerIndex = i;
		e.userID = p.userID;
		e.clientHd = p.clientHd;
		e.name = p.name.data();
		e.team = p.team;
		e.kills = p.kills;
		e.deaths = p.deaths;
		e.masters[0] = CreatureIndex(100000000 + (i32)p.mainClass);
		e.masters[1] = CreatureIndex(100000000 + (i32)p.subClass);
		e.assists = p.assists;
		e.bestKillStreak = p.bestKillStreak;
		e.points = p.points;
		e.ingameLevel = (u8)p.ingameLevel;
		e.damageDealt = p.damageDealt;
		e.occupyCaptures = p.occupyCaptures;
		e.occupyScore = p.occupyScore;
		e.savedPieces = p.savedPieces;
		e.damageStats = p.damageStats;
		e.statueCaptures = p.statueCaptures;
		e.visionCaptures = p.visionCaptures;
		e.monsterKills = p.monsterKills;
		e.traits[0] = p.characters[0]->traits;
		e.traits[1] = p.characters[1]->traits;
		for(int m = 0; m < 2; m++) {
			const World::Player::MasterScore& s = p.masterScores[m];
			e.masterScores[m].kills = s.kills;
			e.masterScores[m].deaths = s.deaths;
			e.masterScores[m].assists = s.assists;
			e.masterScores[m].damageDealt = s.damageDealt;
			e.masterScores[m].damageStats = s.damageStats;
			e.masterScores[m].playMs = (u32)(s.activeSec * 1000.0);
		}
		list.push_back(e);
		const i32 score = (world.map && world.map->ingameLevels) || world.occupyMode ? p.points : p.kills;
		if(p.team == 0) red += score; else blue += score;
	}
	if(world.map && world.map->ingameLevels) { red += world.teamBonusPoints[0]; blue += world.teamBonusPoints[1]; }
	LOG("Game :: time over, red %d - blue %d", red, blue);
	const f64 playSec = TimeDiffSec(TimeDiff(tGameStarted, localTime));
	const u32 minutes = (u32)(playSec / 60.0);
	const GameXmlContent::StageMaps* stage = GetGameXmlContent().FindStageMaps(replication.stageIndex);
	const PvpRewardRule& rule = FindPvpRewardRule(stage ? stage->joinMemberMax : 0);
	foreach(e, list) {
		if(e->clientHd == ClientHandle::INVALID) continue;
		const i32 own = e->team == 0 ? red : blue;
		const i32 other = e->team == 0 ? blue : red;
		const bool won = world.winnerTeam >= 0 ? e->team == world.winnerTeam : own > other;
		e->gold = minutes * (won ? rule.winGoldPerMin : rule.loseGoldPerMin);
		e->exp = minutes * (won ? rule.winExpPerMin : rule.loseExpPerMin);
		static const f32 HYP_MEDALS_PER_MIN_WIN = 1.0f;
		static const f32 HYP_MEDALS_PER_MIN_LOSE = 0.75f;
		e->medals = (u32)(minutes * (won ? HYP_MEDALS_PER_MIN_WIN : HYP_MEDALS_PER_MIN_LOSE));
		if(replication.titanMode) {
			const AccountTier tier = AccountTierLoad(e->name);
			e->tierType = (u8)tier.tierType;
			e->tierPoints = (u16)MAX(tier.points, 0);
		}
		ComputeRewardBonuses(replication.gameType, replication.stageIndex, e->gold, e->exp, guildBoost[e->playerIndex], &e->bonusRon, &e->bonusExp);
		const World::Player& p = world.GetPlayer(e->playerIndex);
		e->guildMembers = GuildMembersInTeam(world, p);
		if(e->guildMembers > 0 && (world.winnerTeam >= 0 || own != other)) {
			for(const PvpGuildPointRule& g : MEASURED_PVP_GUILD_POINTS) if(stage && g.teamSize == stage->joinMemberMax) e->guildBase = won ? g.win : g.lose;
			e->guildBonus = e->guildBase * (u32)GetGameXmlContent().GuildMemberBonus(GameXmlContent::GUILD_BONUS_ARENA, e->guildMembers) / 100;
			e->guildBoost = guildBoost[e->playerIndex];
		}
		AccountGameRecord record;
		record.gold = e->gold + RewardBonusTotal(e->bonusRon);
		record.exp = e->exp + RewardBonusTotal(e->bonusExp);
		record.won = won;
		record.draw = world.winnerTeam < 0 && own == other;
		record.kills = p.kills;
		record.assists = p.assists;
		record.bestKillStreak = p.bestKillStreak;
		record.firstBlood = p.firstBlood;
		record.damage = p.damageDealt;
		record.deaths = p.deaths;
		record.score = world.occupyMode ? p.occupyScore : p.points > 0 ? p.points : p.kills;
		record.myTeamScore = own;
		record.enemyTeamScore = other;
		foreach_const(q, list) { if(q->team == e->team) record.myTeamCount++; else record.enemyTeamCount++; }
		record.occupyCount = p.occupyCaptures;
		if(const GameXmlContent::MapList* mapInfo = GetGameXmlContent().FindMapListByID(replication.mapIndex)) record.pvpSaveType = mapInfo->pvpSaveType;
		record.beginTime = gameStartUnix;
		record.stageIndex = (i32)replication.stageIndex;
		record.gameType = (i32)replication.gameType;
		record.classTypes = { (i32)p.mainClass, (i32)p.subClass };
		record.playSeconds = tGameStarted != Time::ZERO ? (u32)TimeDiffSec(TimeDiff(tGameStarted, localTime)) : 0;
		record.guildPoints = (i32)(e->guildBase + e->guildBonus);
		AccountGameRecordAppend(e->name, record);
		if(e->medals > 0 && GetGameXmlContent().pvpMedalItem > 0) AccountRewardItemAppend(e->name, GetGameXmlContent().pvpMedalItem, (i32)e->medals);
		world.FinishProgress(p, record);
		LOG("Game :: %ls reward %u gold %u exp (+%u / +%u bonus, %u min, %s)", e->name, e->gold, e->exp, RewardBonusTotal(e->bonusRon), RewardBonusTotal(e->bonusExp), minutes, won ? "win" : (own == other ? "draw" : "loss"));
	}
	Replication::EndGot got;
	const bool titanEnd = replication.titanMode;
	if(titanEnd) {
		for(i32 team = 0; team < 2; team++) foreach_const(s, world.giantSummons[team]) got.summons[team].push_back(*s);
		world.BuildPraises(stage ? stage->gameMode : GameModeType::GAME_MODE_GOT, (u32)(playSec * 1000.0), &got.praises);
	}
	replication.SendGameEnd(list.data(), (i32)list.size(), red, blue, (u32)(TimeDiffSec(TimeDiff(tGameStarted, localTime)) * 1000.0), world.winnerTeam, titanEnd ? &got : nullptr);
}

void Game::BuildRewardCards(const GameXmlContent::StageMaps& stage)
{
	rewardCards.clear();
	foreach_const(v, stage.viewRewards) {
		for(i32 i = 0; i < HYP_REWARD_CARDS_PER_VIEW_ITEM && !rewardCards.full(); i++) rewardCards.push_back({ MEASURED_REWARD_TYPE_ITEM, *v, 1 });
	}
	for(i32 i = 0; (i32)rewardCards.size() < MEASURED_REWARD_CARD_COUNT; i++) {
		const i32 percent = HYP_REWARD_EXP_MIN_PERCENT + (i * HYP_REWARD_EXP_STEP_PERCENT) % (HYP_REWARD_EXP_MAX_PERCENT - HYP_REWARD_EXP_MIN_PERCENT);
		rewardCards.push_back({ MEASURED_REWARD_TYPE_EXP, -1, MAX(MEASURED_PVE_BASE_EXP * (u32)percent / 100, 1u) });
	}
	for(i32 i = (i32)rewardCards.size() - 1; i > 0; i--) eastl::swap(rewardCards[i], rewardCards[RandUint() % (u32)(i + 1)]);
}

void Game::EndStage(bool cleared)
{
	gameEnded = true;
	stageEnded = true;
	tGameEnded = localTime;
	world.frozen = true;
	const GameXmlContent::StageMaps* stage = GetGameXmlContent().FindStageMaps(replication.stageIndex);
	if(!cleared || !stage) {
		LOG("Stage :: failed");
		foreach_const(p, world.players) {
			if(p->clientHd == ClientHandle::INVALID || p->removed) continue;
			replication.SendWarnPveGameEnd(p->clientHd, MEASURED_PVE_FAIL_RETURN_MS);
			AccountGameRecord record;
			record.gold = PickedGold(*p);
			record.kills = p->pveKills;
			record.stageIndex = (i32)replication.stageIndex;
			record.gameType = (i32)replication.gameType;
			record.classTypes = { (i32)p->mainClass, (i32)p->subClass };
			record.playSeconds = tGameStarted != Time::ZERO ? (u32)TimeDiffSec(TimeDiff(tGameStarted, localTime)) : 0;
			AccountGameRecordAppend(p->name.data(), record);
			world.FinishProgress(*p, record);
		}
		return;
	}
	BuildRewardCards(*stage);
	eastl::fixed_vector<Replication::StageMember,MAX_PLAYERS,false> members;
	foreach_const(p, world.players) {
		if(p->clientHd == ClientHandle::INVALID || p->removed) continue;
		members.push_back({ p->mainClass, p->subClass, p->name.data() });
	}
	const u32 playMs = (u32)(TimeDiffSec(TimeDiff(tGameStarted, localTime)) * 1000.0);
	PveRankingEntry ranking;
	ranking.stage = (i32)replication.stageIndex;
	ranking.playMs = playMs;
	foreach_const(p, world.players) {
		if(p->clientHd == ClientHandle::INVALID || p->removed) continue;
		Replication::StageClear clear;
		clear.player = replication.GetLocalActorID(p->clientHd, p->Main().UID);
		clear.stage = (i32)replication.stageIndex;
		clear.kills = p->pveKills;
		clear.deaths = p->deaths;
		clear.playTimeMs = playMs;
		const StageScore score = ComputeStageScore(*p, (f64)playMs / 1000.0);
		clear.rank = score.grade;
		clear.baseRon = (u32)stage->moneyReward;
		clear.baseExp = MEASURED_PVE_BASE_EXP;
		clear.attackScore = score.attack;
		clear.defenseScore = score.defense;
		clear.timeScore = score.time;
		clear.totalScore = score.total;
		clear.technicGrade = score.technic;
		ComputeRewardBonuses(replication.gameType, replication.stageIndex, clear.baseRon, clear.baseExp, guildBoost[p->index], &clear.bonusRon, &clear.bonusExp);
		replication.SendExpGain(p->clientHd, clear.baseExp + RewardBonusTotal(clear.bonusExp));
		replication.SendStageClearResult(p->clientHd, clear, members.data(), (i32)members.size());
		replication.SendRewardItemList(p->clientHd, rewardCards.data(), (i32)rewardCards.size());
		replication.SendClearedStage(p->clientHd, clear.stage);
		AccountStageCleared(p->name.data(), clear.stage);
		AccountGameRecord record;
		record.gold = clear.baseRon + RewardBonusTotal(clear.bonusRon) + PickedGold(*p);
		record.exp = clear.baseExp + RewardBonusTotal(clear.bonusExp);
		record.won = 1;
		record.kills = p->pveKills;
		record.stageIndex = clear.stage;
		record.gameType = (i32)replication.gameType;
		record.classTypes = { (i32)p->mainClass, (i32)p->subClass };
		record.playSeconds = playMs / 1000;
		record.deaths = p->deaths;
		record.clearRank = clear.rank;
		record.stageKills = p->pveKills;
		record.partySize = (i32)members.size();
		AccountGameRecordAppend(p->name.data(), record);
		AppendSummaryEvents(world, *p, clear.stage, p->deaths == 0, clear.rank);
		world.FinishProgress(*p, record);
		if(!rewardStates.full()) {
			rewardStates.push_back({ p->clientHd, 0, false, Time::ZERO, {} });
			rewardStates.back().slotCount = RewardSlotsForGrade(score.grade);
		}
		LOG("Stage :: %ls cleared stage %d (%d kills, %d deaths, rank %d, scores %u/%u/%u total %u, best combo %d technic %d, %d reward slots)", p->name.data(), clear.stage, clear.kills, clear.deaths, (i32)clear.rank, score.attack, score.defense, score.time, score.total, p->bestHitCombo, (i32)score.technic, (i32)RewardSlotsForGrade(score.grade));
		ranking.totalScore = MAX(ranking.totalScore, clear.totalScore);
		ranking.deaths += (u32)p->deaths;
		ranking.clearRank = clear.rank;
		if(!ranking.members.full()) ranking.members.push_back({ (i32)p->mainClass, (i32)p->subClass, p->name });
		PveRankingSetLastGame(p->name.data(), clear.stage);
	}
	if(!ranking.members.empty()) PveRankingSubmit(ranking);
}

void Game::EndDefence()
{
	gameEnded = true;
	tGameEnded = localTime;
	world.frozen = true;
	const auto& phases = GetGameXmlContent().defencePhases;
	const i32 cleared = world.defence.cleared;
	const GameXmlContent::DefencePhase* phase = cleared > 0 && cleared <= (i32)phases.size() ? &phases[cleared - 1] : nullptr;
	eastl::fixed_vector<Replication::StageMember,MAX_PLAYERS,false> members;
	foreach_const(p, world.players) {
		if(p->clientHd == ClientHandle::INVALID || p->removed) continue;
		members.push_back({ p->mainClass, p->subClass, p->name.data() });
	}
	PveRankingEntry ranking;
	ranking.stage = (i32)replication.stageIndex;
	ranking.totalScore = (u32)MAX(cleared, 0);
	ranking.playMs = world.defence.clearMs;
	foreach_const(p, world.players) {
		if(p->clientHd == ClientHandle::INVALID || p->removed) continue;
		Replication::DefenceClear clear;
		clear.player = replication.GetLocalActorID(p->clientHd, p->Main().UID);
		clear.stage = (i32)replication.stageIndex;
		clear.clearStep = cleared;
		clear.clearMs = world.defence.clearMs;
		clear.baseRon = phase ? phase->gold : 0;
		clear.baseExp = phase ? phase->exp : 0;
		ComputeRewardBonuses(replication.gameType, replication.stageIndex, clear.baseRon, clear.baseExp, guildBoost[p->index], &clear.bonusRon, &clear.bonusExp);
		if(phase) foreach_const(it, phase->items) if(!clear.items.full()) clear.items.push_back({ MEASURED_REWARD_TYPE_ITEM, it->first, (u32)it->second });
		clear.guildMembers = GuildMembersInTeam(world, *p);
		clear.guildBase = phase && clear.guildMembers > 0 ? phase->guildPoint : 0;
		clear.guildBonus = clear.guildBase * (u32)GetGameXmlContent().GuildMemberBonus(GameXmlContent::GUILD_BONUS_DEFENCE, clear.guildMembers) / 100;
		clear.boostGold = guildBoost[p->index][0];
		clear.boostExp = guildBoost[p->index][1];
		replication.SendExpGain(p->clientHd, clear.baseExp + RewardBonusTotal(clear.bonusExp));
		replication.SendDefenceClearResult(p->clientHd, clear, members.data(), (i32)members.size());
		foreach_const(it, clear.items) AccountRewardItemAppend(p->name.data(), it->index, (i32)it->amount);
		if(world.defence.won) AccountStageCleared(p->name.data(), clear.stage);
		if(cleared > 0) AccountDefenceRecordSubmit(p->name.data(), { cleared, world.defence.clearMs });
		AccountGameRecord record;
		record.gold = clear.baseRon + RewardBonusTotal(clear.bonusRon);
		record.exp = clear.baseExp + RewardBonusTotal(clear.bonusExp);
		record.won = world.defence.won ? 1 : 0;
		record.guildPoints = (i32)(clear.guildBase + clear.guildBonus);
		record.kills = p->pveKills;
		record.stageIndex = clear.stage;
		record.gameType = (i32)replication.gameType;
		record.classTypes = { (i32)p->mainClass, (i32)p->subClass };
		record.playSeconds = tGameStarted != Time::ZERO ? (u32)TimeDiffSec(TimeDiff(tGameStarted, localTime)) : 0;
		AccountGameRecordAppend(p->name.data(), record);
		world.FinishProgress(*p, record);
		LOG("Defence :: %ls ends at phase %d (%s), %u gold %u exp", p->name.data(), cleared, world.defence.won ? "won" : "failed", (u32)record.gold, record.exp);
		if(!ranking.members.full()) ranking.members.push_back({ (i32)p->mainClass, (i32)p->subClass, p->name });
		PveRankingSetLastGame(p->name.data(), -(i32)replication.stageIndex);
	}
	if(cleared > 0 && !ranking.members.empty()) PveRankingSubmit(ranking);
}

void Game::OnRewardStageReady(ClientHandle clientHd)
{
	if(!stageEnded || rewardCards.empty()) return;
	foreach(r, rewardStates) {
		if(r->clientHd != clientHd) continue;
		replication.SendRewardStageStart(clientHd, MEASURED_REWARD_WAIT_MS, r->slotCount);
		if(r->startedAt == Time::ZERO) r->startedAt = localTime;
	}
}

void Game::AutoPickRewards(ClientHandle clientHd)
{
	foreach(r, rewardStates) {
		if(r->clientHd != clientHd || r->finished) continue;
		for(u32 tries = 0; r->picks < r->slotCount && tries < rewardCards.size() * 4; tries++) {
			const u32 slot = RandUint() % (u32)rewardCards.size();
			if(eastl::find(r->slots.begin(), r->slots.end(), slot) != r->slots.end()) continue;
			OnRewardSelectSlot(clientHd, slot);
		}
		return;
	}
}

void Game::OnRewardSelectSlot(ClientHandle clientHd, u32 slot)
{
	foreach(r, rewardStates) {
		if(r->clientHd != clientHd || r->finished) continue;
		if(slot >= rewardCards.size() || r->picks >= r->slotCount + MEASURED_REWARD_VIP_SLOTS + r->bought) return;
		if(eastl::find(r->slots.begin(), r->slots.end(), slot) != r->slots.end()) return;
		if(!r->slots.full()) r->slots.push_back(slot);
		r->picks++;
		const Replication::RewardCard& card = rewardCards[slot];
		replication.SendRewardSelectSlot(clientHd, 0, slot, card);
		auto pl = playerMap.find(clientHd);
		if(pl == playerMap.end()) return;
		const wchar* name = world.GetPlayer(pl->second->playerIndex).name.data();
		if(card.type == MEASURED_REWARD_TYPE_EXP) {
			AccountGameRecord record;
			record.exp = card.amount;
			record.gameType = (i32)replication.gameType;
			AccountGameRecordAppend(name, record);
		}
		else AccountRewardItemAppend(name, card.index, (i32)card.amount);
		LOG("Stage :: %ls picks reward slot %u (type %u, item %d x%u)", name, slot, card.type, card.index, card.amount);
		return;
	}
}

void Game::OnRewardBuyChance(ClientHandle clientHd)
{
	static const u32 MEASURED_CCOIN_REASON_PURCHASE = 46;
	static const u32 HYP_BUY_CHANCE_FAILED = 1;
	static const u8 MEASURED_NEXT_ACTION_BUY_CHANCE = 1;
	auto pl = playerMap.find(clientHd);
	if(pl == playerMap.end()) return;
	const wchar* name = world.GetPlayer(pl->second->playerIndex).name.data();
	foreach(r, rewardStates) {
		if(r->clientHd != clientHd || r->finished) continue;
		const i32 cost = GetGameXmlContent().RewardBuyChanceCost(world.difficultyBit, r->bought);
		AccountProgress progress = AccountProgressLoad(name);
		PacketWriter<Sv::SA_RewardBuyChance,16> answer;
		if(cost < 0 || progress.ccoin < cost || (i32)rewardCards.size() <= r->slotCount + MEASURED_REWARD_VIP_SLOTS + r->bought) {
			answer.Write<u32>(HYP_BUY_CHANCE_FAILED);
			answer.Write<u32>((u32)(r->slotCount + r->bought));
			answer.Write<u8>(cost < 0 ? 0 : MEASURED_NEXT_ACTION_BUY_CHANCE);
			answer.Write<u32>((u32)MAX(cost, 0));
			replication.SendWriter(clientHd, answer);
			return;
		}
		progress.ccoin -= cost;
		AccountProgressSave(name, progress);
		r->bought++;
		PacketWriter<Sv::SN_CCoin,16> coin;
		coin.Write<i64>(progress.ccoin);
		coin.Write<u32>(MEASURED_CCOIN_REASON_PURCHASE);
		replication.SendWriter(clientHd, coin);
		const i32 next = GetGameXmlContent().RewardBuyChanceCost(world.difficultyBit, r->bought);
		answer.Write<u32>(0);
		answer.Write<u32>((u32)(r->slotCount + r->bought));
		answer.Write<u8>(next < 0 ? 0 : MEASURED_NEXT_ACTION_BUY_CHANCE);
		answer.Write<u32>((u32)MAX(next, 0));
		replication.SendWriter(clientHd, answer);
		LOG("Stage :: %ls buys a reward pick for %d C-Coin (%d bought)", name, cost, r->bought);
		return;
	}
}

void Game::OnRewardRequestFinish(ClientHandle clientHd)
{
	foreach(r, rewardStates) {
		if(r->clientHd != clientHd || r->finished) continue;
		r->finished = true;
		replication.SendRewardRequestFinish(clientHd);
		replication.SendRewardStageEnd(clientHd, rewardCards.data(), (i32)rewardCards.size());
		replication.SendPrepareHandover(clientHd);
		return;
	}
}

void Game::OnPlayerReturnToChannel(ClientHandle clientHd)
{
	auto found = playerMap.find(clientHd);
	if(found == playerMap.end()) {
		WARN("Client is not a player of this game (clientHd=%x)", clientHd);
		return;
	}
	Player& p = *found->second;
	p.left = true;
	if(p.returnSent) return;
	p.returnSent = true;

	World::Player& player = world.GetPlayer(p.playerIndex);
	const i32* configIP = Config().HubServerPublicIP[0] != 0 ? Config().HubServerPublicIP : Config().HostIP;
	i32 hubIP[4] = { configIP[0], configIP[1], configIP[2], configIP[3] };
	if(hubIP[0] == 0 && IsClientOnLoopback(replication.server, clientHd)) {
		hubIP[0] = 127;
		hubIP[1] = 0;
		hubIP[2] = 0;
		hubIP[3] = 1;
	}
	replication.SendDoConnectChannel(clientHd, player.name.data(), hubIP, (u16)Config().HubServerPort);
}

void Game::OnClientRemoteCreated(ClientHandle clientHd, u32 seed, RemoteIdx doc)
{
	auto found = playerMap.find(clientHd);
	if(found == playerMap.end()) return;
	world.BindClientRemote(world.GetPlayer(found->second->playerIndex), doc, seed);
}

void Game::OnPlayerTakeGroundItem(ClientHandle clientHd, u32 itemUID)
{
	auto found = playerMap.find(clientHd);
	if(found == playerMap.end()) return;
	World::Player& player = world.GetPlayer(found->second->playerIndex);
	World::GroundItem taken;
	if(!world.TakeGroundItem(player, itemUID, &taken)) {
		LOG("GroundItem :: %ls take 0x%08x refused", player.name.data(), itemUID);
		return;
	}
	if(taken.owner < 0 || (!taken.ron && GetGameXmlContent().FindGroundItem(taken.itemIndex))) return;
	PveLoot& loot = LootFor(player);
	const ActorUID takerUID = player.Main().UID;
	if(taken.ron) {
		loot.picked += taken.count;
		replication.SendMoney(clientHd, loot.gold + loot.picked, MEASURED_MONEY_REASON_PICKUP);
		replication.SendGroundItemTaken(clientHd, takerUID, itemUID, 0);
		return;
	}
	u32 itemISN = 0;
	if(const GameXmlContent::ItemInfo* info = GetGameXmlContent().FindItemInfo(taken.itemIndex)) {
		AccountItemList touched;
		AccountInventoryAdd(&loot.inventory, taken.itemIndex, taken.count, info->maxStack, &touched, info->firstSlot);
		foreach_const(t, touched) {
			replication.SendItemMerge(clientHd, t->uid, t->count);
			replication.SendItemUpdate(clientHd, *t, MEASURED_ITEM_UPDATE_REASON_PICKUP);
			itemISN = t->uid;
		}
		AccountRewardItemAppend(player.name.data(), taken.itemIndex, taken.count);
	}
	else WARN("GroundItem :: unknown item %d picked by %ls", taken.itemIndex, player.name.data());
	replication.SendGroundItemTaken(clientHd, takerUID, itemUID, itemISN);
}

Game::PveLoot& Game::LootFor(const World::Player& player)
{
	foreach(l, pveLoot) if(l->playerIndex == (i32)player.index) return *l;
	ASSERT(!pveLoot.full());
	pveLoot.push_back();
	PveLoot& loot = pveLoot.back();
	loot.playerIndex = (i32)player.index;
	loot.gold = AccountProgressLoad(player.name.data()).gold;
	loot.inventory = AccountInventoryLoad(player.name.data());
	return loot;
}

i64 Game::PickedGold(const World::Player& player) const
{
	foreach_const(l, pveLoot) if(l->playerIndex == (i32)player.index) return l->picked;
	return 0;
}

void Game::OnPlayerSkillUpgrade(ClientHandle clientHd, i32 slot)
{
	auto found = playerMap.find(clientHd);
	if(found == playerMap.end()) return;
	World::Player& player = world.GetPlayer(found->second->playerIndex);
	if(!world.UpgradeSkillSlot(player, slot)) LOG("Upgrade :: %ls slot %d refused (points %d)", player.name.data(), slot, player.skillPoints);
}

void Game::OnTraitUpgrade(ClientHandle clientHd, ActorUID characterUID, const u8* indices, i32 count)
{
	auto found = playerMap.find(clientHd);
	if(found == playerMap.end()) return;
	World::Player& player = world.GetPlayer(found->second->playerIndex);
	const u32 error = world.UpgradeTraits(player, characterUID, indices, count);
	if(error != 0) LOG("Traits :: %ls upgrade refused (error %u)", player.name.data(), error);
	replication.SendTraitAnswer(clientHd, false, error);
}

void Game::OnRequestVote(ClientHandle clientHd, u32 type)
{
	auto found = playerMap.find(clientHd);
	if(found == playerMap.end()) return;
	world.RequestVote(world.GetPlayer(found->second->playerIndex), type);
}

void Game::OnVote(ClientHandle clientHd, u32 choice)
{
	auto found = playerMap.find(clientHd);
	if(found == playerMap.end()) return;
	world.CastVote(world.GetPlayer(found->second->playerIndex), choice);
}

void Game::OnTraitReset(ClientHandle clientHd, ActorUID characterUID)
{
	auto found = playerMap.find(clientHd);
	if(found == playerMap.end()) return;
	World::Player& player = world.GetPlayer(found->second->playerIndex);
	replication.SendTraitAnswer(clientHd, true, world.ResetTraits(player, characterUID));
}

void Game::OnPlayerSyncTeleport(ClientHandle clientHd, ActorUID actorUID, const vec3& pos)
{
	auto found = playerMap.find(clientHd);
	if(found == playerMap.end()) return;
	World::Player& player = world.GetPlayer(found->second->playerIndex);
	if(player.Main().UID != actorUID) return;
	world.TeleportPlayer(player, pos, player.input.rot.bodyYaw);
	replication.SendPlayerSyncTeleportToAll(actorUID, pos);
	LOG("Teleport :: %ls confirmed at (%.0f, %.0f, %.0f)", player.name.data(), pos.x, pos.y, pos.z);
}

void Game::OnPlayerJump(ClientHandle clientHd, ActorUID actorUID, f32 rotate, f32 moveDirX, f32 moveDirY)
{
	Player& p = *playerMap.at(clientHd);
	World::Player& player = world.GetPlayer(p.playerIndex);
	ASSERT(player.clientHd == clientHd);

	player.input.jump = 1;
}

void Game::OnPlayerWeaponFire(ClientHandle clientHd, f32 aimYaw)
{
	Player& p = *playerMap.at(clientHd);
	World::Player& player = world.GetPlayer(p.playerIndex);
	ASSERT(player.clientHd == clientHd);

	player.weaponFired = true;
	player.weaponAimYaw = aimYaw;
	world.OnWeaponAimPacket(player, aimYaw);
}

void Game::OnPlayerWeaponStateRelay(ClientHandle clientHd, ActorUID ownerUID, i32 state, u8 chargeLevel, u8 firingCombo)
{
	Player& p = *playerMap.at(clientHd);
	World::Player& player = world.GetPlayer(p.playerIndex);
	ASSERT(player.clientHd == clientHd);

	if(ownerUID == ActorUID::INVALID) ownerUID = player.Main().UID;
	world.OnWeaponState(player, state, chargeLevel, firingCombo);
	if(state != WEAPON_STATE_CHARGE) return;
	const WeaponIndex weapon = world.ActiveWeapon(player);
	replication.RelayWeaponState(clientHd, ownerUID, (i32)weapon, state, player.chargeLevel, firingCombo);
}

void Game::OnPlayerInteraction(ClientHandle clientHd, ActorUID targetUID, LocalActorID selfLocal)
{
	Player& p = *playerMap.at(clientHd);
	World::Player& player = world.GetPlayer(p.playerIndex);
	ASSERT(player.clientHd == clientHd);
	LOG("Interaction :: %ls self=%u target uid=%u", player.name.data(), (u32)selfLocal, (u32)targetUID);
	if(targetUID != ActorUID::INVALID) world.PlayerInteract(player, targetUID);
}

void Game::OnPlayerInteractionCancel(ClientHandle clientHd)
{
	auto found = playerMap.find(clientHd);
	if(found == playerMap.end()) return;
	world.CancelMapInteraction(world.GetPlayer(found->second->playerIndex));
}

void Game::OnPlayerToggleSkill(ClientHandle clientHd, SkillID skillID, bool on)
{
	Player& p = *playerMap.at(clientHd);
	World::Player& player = world.GetPlayer(p.playerIndex);
	ASSERT(player.clientHd == clientHd);
	LOG("Toggle :: %ls request skill=%d %s", player.name.data(), (i32)skillID, on ? "on" : "off");
	if(on) world.ToggleOn(player, skillID, player.body->GetWorldPos());
	else world.ToggleOff(player);
}

bool Game::IsWeaponFireObject(ClientHandle clientHd, const wchar* name, u16 len)
{
	Player& p = *playerMap.at(clientHd);
	const World::Player& player = world.GetPlayer(p.playerIndex);
	ASSERT(player.clientHd == clientHd);
	const WeaponIndex weapon = world.ActiveWeapon(player);
	const WeaponFiring* wf = GetGameXmlContent().FindWeaponFiring((i32)weapon, player.Main().stance);
	if(wf && wf->fireDummy.empty()) return FireObjectIs(name, len, FIRE_OBJECT_SCENE_ROOT);
	if(!wf) return false;
	auto matches = [name, len](const eastl::fixed_string<char,32,false>& dummy) {
		if(dummy.size() != len) return false;
		for(u16 i = 0; i < len; i++) if((wchar)dummy[i] != name[i]) return false;
		return true;
	};
	return matches(wf->fireDummy) || (!wf->chargeDummy.empty() && matches(wf->chargeDummy));
}

void Game::OnPlayerWeaponShot(ClientHandle clientHd, const vec3& origin, const vec2& dir, f32 distance, const u8* rawPacket, i32 rawSize)
{
	Player& p = *playerMap.at(clientHd);
	World::Player& player = world.GetPlayer(p.playerIndex);
	ASSERT(player.clientHd == clientHd);

	auto sid = replication.server->clientHandle2IDMap.find(clientHd);
	if(sid == replication.server->clientHandle2IDMap.end()) {
		WARN("Client is no longer connected (clientHd=%x)", clientHd);
		return;
	}
	const auto& ip = replication.server->clientInfo[sid->second].ip;
	const auto& host = Config().HostIP;
	const bool isHost = ip[0] == 127 || (ip[0] == host[0] && ip[1] == host[1] && ip[2] == host[2] && ip[3] == host[3]);
	player.shotLeadSec = isHost ? 0.0f : Config().ShotLeadMs / 1000.0f;

	world.OnWeaponShotPacket(player, origin, dir, distance, rawPacket, rawSize);
}

void Game::OnPlayerCastSkill(ClientHandle clientHd, ActorUID actorUID, const PlayerInputCastSkill& cast, const Cl::CQ_PlayerCastSkill::PosStruct& posInfo)
{
	Player& p = *playerMap.at(clientHd);
	World::Player& player = world.GetPlayer(p.playerIndex);
	ASSERT(player.clientHd == clientHd);

	player.input.cast = cast;
	player.input.cast.clientTime = posInfo.clientTime;

	RotationHumanoid rot = { posInfo.rot.x, posInfo.rot.y, posInfo.rot.z };
	// TODO: convert clientTime to localTime
	OnPlayerUpdatePosition(clientHd, actorUID, f2v(posInfo.pos), f2v(posInfo.moveDir), rot, posInfo.speed, ActionStateID::INVALID, 0, true);

	LOG("OnPlayerCastSkill :: (%f, %f, %f) targets=%d rot=(%.3f, %.3f, %.3f) moveDir=(%.2f, %.2f)", cast.pos.x, cast.pos.y, cast.pos.z, (i32)cast.targetList.size(), posInfo.rot.x, posInfo.rot.y, posInfo.rot.z, posInfo.moveDir.x, posInfo.moveDir.y);
}

void Game::OnPlayerExecuteSkill(ClientHandle clientHd, SkillID skillID, const vec3& targetPos, LocalActorID target)
{
	Player& p = *playerMap.at(clientHd);
	World::Player& player = world.GetPlayer(p.playerIndex);
	if(player.casting.skillID == skillID) {
		if(targetPos.x != 0 || targetPos.y != 0) player.casting.castPos = targetPos;
		if(target != LocalActorID::INVALID) {
			const ActorUID uid = replication.GetWorldActorUID(clientHd, target);
			if(uid != ActorUID::INVALID) player.casting.target = uid;
		}
	}
	LOG("Cast :: %ls execute request skill=%d (casting %d) pos=(%.0f, %.0f) target=%d", player.name.data(), (i32)skillID, (i32)player.casting.skillID, targetPos.x, targetPos.y, (i32)player.casting.target);
	replication.SendExecuteSkillAck(clientHd, player.Main().UID, skillID);
	if(player.casting.skillID == skillID) world.ReleaseCast(player);
}

void Game::OnRemoteSyncTarget(ClientHandle clientHd, u32 remoteID, const vec3& dest)
{
	auto found = playerMap.find(clientHd);
	if(found == playerMap.end()) return;
	world.SetCasterRemoteDest(found->second->playerIndex, remoteID, dest);
}

void Game::OnPlayerCancelCast(ClientHandle clientHd, SkillID skillID)
{
	Player& p = *playerMap.at(clientHd);
	World::Player& player = world.GetPlayer(p.playerIndex);
	if(player.casting.skillID != SkillID::INVALID && (skillID == SkillID::INVALID || skillID == player.casting.skillID)) {
		static const i32 MEASURED_CANCEL_WEAPON_RESULT = -1;
		replication.SendWeaponStateTo(clientHd, player.Main().UID, MEASURED_WEAPON_STATE_WEAPON_ID, WEAPON_STATE_CHARGE, MEASURED_CANCEL_WEAPON_RESULT);
		world.CancelCast(player, "client");
	}
}

void Game::OnPlayerCancelStance(ClientHandle clientHd)
{
	auto found = playerMap.find(clientHd);
	if(found == playerMap.end()) return;
	World::Player& player = world.GetPlayer(found->second->playerIndex);
	world.PlayerCancelStance(player);
}

void Game::OnPlayerGameIsReady(ClientHandle clientHd)
{
	const i32 READY_WAIT = 3000;

	Player& p = *playerMap.at(clientHd);
	World::Player& player = world.GetPlayer(p.playerIndex);
	ASSERT(player.clientHd == clientHd);
	p.ready = true;
	world.MarkPlayerStatsDirty();

	if(pveStage) {
		i32 humans = 0;
		foreach_const(wp, world.players) if(wp->clientHd != ClientHandle::INVALID && !wp->removed) humans++;
		replication.SendUserLifeInfo(clientHd, humans);
	}
	if(zoneLoading) {
		zoneLoading = false;
		world.StartLevelEvents();
		LOG("Stage :: zone %d ready, level events started", zoneIndex);
	}
	if(phase == Phase::WaitingForFirstPlayer) {
		phase = Phase::WaitingForReady;
		phaseTime = TimeAdd(localTime, TimeMsToTime(READY_WAIT));
		world.combatLocked = true;
		replication.SendGameReady(clientHd, READY_WAIT, 0, player.userID);
	}
	else {
		replication.SendGameReady(clientHd, READY_WAIT, MAX(0, READY_WAIT - (i32)TimeDurationMs(localTime, phaseTime)), player.userID);
		if(phase == Phase::Game && !p.gameStartSent) {
			LOG("[client%x] Game :: %ls ready after the start, sending the game start", clientHd, player.name.data());
			replication.SendPreGameLevelEvents(clientHd);
			replication.SendGameStart(clientHd);
			p.gameStartSent = true;
		}
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

		f32 tpX = 0;
		f32 tpY = 0;
		f32 tpZ = 0;
		const i32 tpArgs = EA::StdC::Sscanf(msg, L"tp %f %f %f", &tpX, &tpY, &tpZ);
		if(tpArgs >= 2) {
			World::Player& playerActor = world.GetPlayer(p.playerIndex);
			const vec3 to(tpX, tpY, tpArgs >= 3 ? tpZ : playerActor.body->GetWorldPos().z);
			world.TeleportPlayer(playerActor, to, playerActor.input.rot.bodyYaw);
			replication.SendTeleportRequest(clientHd, playerActor.Main().UID, to);
			SendDbgMsg(clientHd, LFMT(L"Teleported to (%g, %g, %g)", to.x, to.y, to.z));
			return true;
		}

		i32 tpBot = 0;
		if(EA::StdC::Sscanf(msg, L"tpbot %d %f %f %f", &tpBot, &tpX, &tpY, &tpZ) == 4) {
			foreach(b, world.players) {
				if(b->clientHd != ClientHandle::INVALID || !b->body || b->dead || EA::StdC::Strcmp(b->name.data(), LFMT(L"Bot%d", tpBot)) != 0) continue;
				world.TeleportPlayer(*b, vec3(tpX, tpY, tpZ), b->input.rot.bodyYaw);
				LOG("Scenario :: Bot%d teleported to (%.0f, %.0f, %.0f)", tpBot, tpX, tpY, tpZ);
			}
			return true;
		}

		if(EA::StdC::Strcmp(msg, L"clear") == 0) {
			world.stageCleared = true;
			SendDbgMsg(clientHd, L"Stage cleared");
			return true;
		}

		if(EA::StdC::Strcmp(msg, L"wave") == 0) {
			if(world.defence.inWave) world.EndDefencePhase(2);
			SendDbgMsg(clientHd, LFMT(L"Defence phase %d", world.defence.step));
			return true;
		}

		if(EA::StdC::Strcmp(msg, L"defeat") == 0) {
			world.defence.failed = true;
			SendDbgMsg(clientHd, L"Defence failed");
			return true;
		}

		if(EA::StdC::Strcmp(msg, L"monsters") == 0) {
			i32 alive = 0;
			foreach_const(npc, world.actorNpcList) {
				if(npc->hp <= 0 || npc->removeAt != Time::ZERO || npc->faction != Faction::MONSTER) continue;
				alive++;
				LOG("Dev :: creature=%d (local %d) at (%.0f, %.0f) target %u path %d/%d walking %d chasing %d", (i32)npc->docID, npc->localID, npc->pos.x, npc->pos.y, (u32)npc->monsterTarget, npc->pathPoint, npc->path ? (i32)npc->path->size() : -1, (i32)npc->monsterWalking, (i32)npc->monsterChasing);
			}
			SendDbgMsg(clientHd, LFMT(L"%d monster(s) alive", alive));
			return true;
		}

		if(EA::StdC::Strcmp(msg, L"portal") == 0) {
			world.portalRequested = true;
			SendDbgMsg(clientHd, L"Portal requested");
			return true;
		}

		i32 dropIndex = 0;
		if(EA::StdC::Sscanf(msg, L"drop %d", &dropIndex) == 1 && world.GetPlayer(p.playerIndex).body) {
			const World::Player& playerActor = world.GetPlayer(p.playerIndex);
			world.SpawnPrivateDropGroup(playerActor, dropIndex, playerActor.body->GetWorldPos());
			SendDbgMsg(clientHd, LFMT(L"Drop %d rolled", dropIndex));
			return true;
		}

		f32 killRadius = 0;
		if(EA::StdC::Sscanf(msg, L"killnear %f", &killRadius) == 1 && world.GetPlayer(p.playerIndex).body) {
			World::Player& playerActor = world.GetPlayer(p.playerIndex);
			const vec2 at = vec2(playerActor.body->GetWorldPos());
			i32 killed = 0;
			foreach(npc, world.actorNpcList) {
				if(npc->hp <= 0 || npc->removeAt != Time::ZERO || npc->ownerUID != ActorUID::INVALID || npc->faction != Faction::MONSTER) continue;
				if(glm::length(vec2(npc->pos) - at) > killRadius) continue;
				world.MonsterDie(*npc, playerActor.Main().UID, CreatureIndex(100000000 + (i32)playerActor.Main().classType));
				killed++;
			}
			SendDbgMsg(clientHd, LFMT(L"Killed %d monster(s) within %.0f", killed, killRadius));
			return true;
		}

		i32 monDoc = 0;
		if((EA::StdC::Sscanf(msg, L"tpmon %d", &monDoc) == 1 || EA::StdC::Sscanf(msg, L"killdoc %d", &monDoc) == 1) && world.GetPlayer(p.playerIndex).body) {
			World::Player& playerActor = world.GetPlayer(p.playerIndex);
			const bool kill = msg[0] == L'k';
			World::ActorNpc* best = nullptr;
			f32 bestDist = FLT_MAX;
			foreach(npc, world.actorNpcList) {
				if((i32)npc->docID != monDoc || npc->hp <= 0 || npc->removeAt != Time::ZERO || npc->ownerUID != ActorUID::INVALID) continue;
				const f32 d = glm::length(vec2(npc->pos) - vec2(playerActor.body->GetWorldPos()));
				if(d < bestDist) { bestDist = d; best = &*npc; }
			}
			if(!best) {
				SendDbgMsg(clientHd, LFMT(L"No alive monster %d", monDoc));
				return true;
			}
			if(kill) {
				world.MonsterDie(*best, playerActor.Main().UID, CreatureIndex(100000000 + (i32)playerActor.Main().classType));
				SendDbgMsg(clientHd, LFMT(L"Killed monster %d (local %d)", monDoc, best->localID));
			}
			else {
				static const f32 DEV_TPMON_OFFSET = 350.0f;
				const vec3 to = best->pos + vec3(DEV_TPMON_OFFSET, 0, 0);
				world.TeleportPlayer(playerActor, to, playerActor.input.rot.bodyYaw);
				replication.SendTeleportRequest(clientHd, playerActor.Main().UID, to);
				SendDbgMsg(clientHd, LFMT(L"Teleported near monster %d (local %d)", monDoc, best->localID));
			}
			return true;
		}

		i32 interactID = -1;
		if(EA::StdC::Sscanf(msg, L"interact %d", &interactID) == 1 && world.GetPlayer(p.playerIndex).body) {
			World::Player& playerActor = world.GetPlayer(p.playerIndex);
			world.BuildMapInteractions();
			const World::MapInteraction* best = nullptr;
			vec3 bestPos;
			f32 bestDist = FLT_MAX;
			foreach_const(m, world.mapInteractions) {
				vec3 at;
				if(m->object == ActorUID::INVALID || !m->active || !world.MapInteractionPos(*m, &at)) continue;
				i32 objectDoc = 0;
				if(m->creature) { if(const World::ActorNpc* npc = world.FindNpcActor(m->object)) objectDoc = (i32)npc->docID; }
				else { auto dyn = world.actorDynamicMap.find(m->object); if(dyn != world.actorDynamicMap.end()) objectDoc = (i32)dyn->second->docID; }
				if(interactID > 0 && m->interactionID != interactID && objectDoc != interactID) continue;
				const f32 d = glm::length(vec2(at) - vec2(playerActor.body->GetWorldPos()));
				if(d < bestDist) { bestDist = d; best = &*m; bestPos = at; }
			}
			if(!best) {
				SendDbgMsg(clientHd, LFMT(L"No open interaction %d", interactID));
				return true;
			}
			static const f32 DEV_INTERACT_OFFSET = 80.0f;
			const vec3 to = bestPos + vec3(DEV_INTERACT_OFFSET, 0, 0);
			world.TeleportPlayer(playerActor, to, playerActor.input.rot.bodyYaw);
			replication.SendTeleportRequest(clientHd, playerActor.Main().UID, to);
			world.StartMapInteraction(playerActor, best->object);
			SendDbgMsg(clientHd, LFMT(L"Interaction %d on object %d started", best->interactionID, best->localID));
			return true;
		}

		i32 killTag = 0;
		if(EA::StdC::Sscanf(msg, L"killtag %d", &killTag) == 1) {
			World::Player& playerActor = world.GetPlayer(p.playerIndex);
			i32 killed = 0;
			foreach(npc, world.actorNpcList) {
				if(npc->tagID != killTag || npc->hp <= 0 || npc->removeAt != Time::ZERO || npc->ownerUID != ActorUID::INVALID) continue;
				world.MonsterDie(*npc, playerActor.Main().UID, CreatureIndex(100000000 + (i32)playerActor.Main().classType));
				killed++;
			}
			SendDbgMsg(clientHd, LFMT(L"Killed %d monster(s) with tag %d", killed, killTag));
			return true;
		}

		if(EA::StdC::Strcmp(msg, L"god") == 0) {
			World::Player& playerActor = world.GetPlayer(p.playerIndex);
			playerActor.devGod = !playerActor.devGod;
			SendDbgMsg(clientHd, playerActor.devGod ? L"God mode on" : L"God mode off");
			return true;
		}

		i32 piecesTeam = 0, piecesAdd = 0;
		if(EA::StdC::Sscanf(msg, L"pieces %d %d", &piecesTeam, &piecesAdd) == 2 && (piecesTeam == 0 || piecesTeam == 1)) {
			world.AddTeamPieces(piecesTeam, piecesAdd);
			SendDbgMsg(clientHd, LFMT(L"Team %d +%d titan pieces", piecesTeam, piecesAdd));
			return true;
		}

		i32 pointsTeam = 0, pointsAdd = 0;
		if(EA::StdC::Sscanf(msg, L"points %d %d", &pointsTeam, &pointsAdd) == 2 && (pointsTeam == 0 || pointsTeam == 1)) {
			world.AddTeamPoints(pointsTeam, pointsAdd);
			SendDbgMsg(clientHd, LFMT(L"Team %d +%d points", pointsTeam, pointsAdd));
			return true;
		}

		if(EA::StdC::Strcmp(msg, L"vision") == 0) {
			World::Player& playerActor = world.GetPlayer(p.playerIndex);
			playerActor.devVision = !playerActor.devVision;
			SendDbgMsg(clientHd, playerActor.devVision ? L"Full vision on" : L"Full vision off");
			return true;
		}

		if(EA::StdC::Strcmp(msg, L"titan") == 0) {
			world.RequestVote(world.GetPlayer(p.playerIndex), 1);
			SendDbgMsg(clientHd, L"Titan vote requested");
			return true;
		}

		i32 itemIndex = 0;
		if(EA::StdC::Sscanf(msg, L"item %d", &itemIndex) == 1) {
			World::Player& playerActor = world.GetPlayer(p.playerIndex);
			world.SpawnGroundItemAt(itemIndex, playerActor.body->GetWorldPos(), -1);
			SendDbgMsg(clientHd, LFMT(L"Item %d dropped", itemIndex));
			return true;
		}

		i32 hpTag = 0, hpPercent = 0;
		if(EA::StdC::Sscanf(msg, L"monhp %d %d", &hpTag, &hpPercent) == 2) {
			i32 changed = 0;
			foreach(npc, world.actorNpcList) {
				if(npc->tagID != hpTag || npc->hp <= 0) continue;
				npc->hp = MAX(npc->hpMax * hpPercent / 100, 1);
				replication.SendHpToAll(npc->UID, npc->hp, npc->hpMax);
				changed++;
			}
			SendDbgMsg(clientHd, LFMT(L"%d monster(s) with tag %d at %d%% hp", changed, hpTag, hpPercent));
			return true;
		}

		i32 provokeTag = 0;
		if(EA::StdC::Sscanf(msg, L"provoke %d", &provokeTag) == 1) {
			World::Player& playerActor = world.GetPlayer(p.playerIndex);
			i32 provoked = 0;
			foreach(npc, world.actorNpcList) {
				if(npc->tagID != provokeTag || npc->hp <= 0 || npc->ownerUID != ActorUID::INVALID) continue;
				npc->monsterTarget = playerActor.Main().UID;
				npc->monsterReturning = false;
				provoked++;
			}
			SendDbgMsg(clientHd, LFMT(L"Provoked %d monster(s) with tag %d", provoked, provokeTag));
			return true;
		}

		i32 enemyCreature = 0;
		if(EA::StdC::Sscanf(msg, L"enemysummon %d", &enemyCreature) == 1) {
			World::Player& playerActor = world.GetPlayer(p.playerIndex);
			foreach_const(e, world.players) {
				if(e->team == playerActor.team || world.pendingSummons.full()) continue;
				World::PendingSummon ps;
				ps.time = world.localTime;
				ps.creature = enemyCreature;
				ps.ownerIndex = e->index;
				ps.pos = playerActor.body->GetWorldPos() + vec3(150, 0, 0);
				ps.rot = vec3(0);
				world.pendingSummons.push_back(ps);
				SendDbgMsg(clientHd, LFMT(L"Creature %d summoned for %ls", enemyCreature, e->name.data()));
				break;
			}
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
