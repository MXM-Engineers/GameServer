#include "world.h"
#include "world_common.h"
#include <mxm/game_content.h>

static const u32 MEASURED_PHASE_END_KILLS = 2;
static const u32 HYP_PHASE_END_TIMEOUT = 1;
static const i32 MEASURED_DEFENCE_START_MINERALS = 1;
static const u32 MEASURED_ANNOUNCE_MINERAL_GAIN = 16;

void World::StartDefence(i32 coreDoc)
{
	defence = DefenceState();
	defence.active = !GetGameXmlContent().defencePhases.empty();
	defence.coreDoc = coreDoc;
	minerals = defence.active ? MEASURED_DEFENCE_START_MINERALS : 0;
	mineralCarrierKills = 0;
	LOG("Defence :: %d phases, core %d", (i32)GetGameXmlContent().defencePhases.size(), coreDoc);
}

Faction World::PlayerSideFaction() const
{
	foreach_const(p, players) {
		if(p->team == 0 || p->team == 1) return (Faction)p->team;
	}
	return Faction::RED;
}

void World::FirePhaseTriggers(GameXmlContent::LevelEventNode::Kind kind, i32 phase)
{
	foreach_const(n, map->levelEvents) {
		if(n->kind != kind || n->phase != phase || !NodeInDifficulty(*n)) continue;
		foreach_const(o, n->outputs) FireLevelEvent(*o, -1, n->id);
	}
}

bool World::DefenceMonstersAlive() const
{
	if(!monsterCalls.empty()) return true;
	foreach_const(n, actorNpcList) {
		if(n->monsterAi && n->faction == Faction::MONSTER && n->hp > 0 && n->removeAt == Time::ZERO) return true;
	}
	return false;
}

void World::EndDefencePhase(u32 reason)
{
	const auto& phases = GetGameXmlContent().defencePhases;
	const GameXmlContent::DefencePhase& phase = phases[defence.step - 1];
	defence.inWave = false;
	defence.cleared = defence.step;
	defence.clearMs = (u32)(GameTimeSec() * 1000.0);
	replication->SendPhaseEndToAll(defence.step, reason, defence.clearMs);
	FirePhaseTriggers(GameXmlContent::LevelEventNode::END_PHASE, defence.step);
	LOG("Defence :: phase %d ends (reason %u, %d kills) t=%.1f", defence.step, reason, defence.kills, GameTimeSec());
	if(defence.step >= (i32)phases.size()) defence.won = true;
	else defence.nextStartAt = TimeAdd(localTime, TimeMsToTime(phase.endDelay * 1000.0));
}

void World::UpdateDefence()
{
	if(!defence.active || frozen || gameStartedAt == Time::ZERO || defence.won || defence.failed) return;
	const auto& phases = GetGameXmlContent().defencePhases;
	if(!defence.inWave) {
		if(defence.step >= (i32)phases.size() || localTime < defence.nextStartAt) return;
		defence.step++;
		defence.inWave = true;
		defence.kills = 0;
		defence.waveStartAt = localTime;
		replication->SendPhaseStartToAll(defence.step);
		if(defence.step == 1) SendMinerals();
		FirePhaseTriggers(GameXmlContent::LevelEventNode::START_PHASE, defence.step);
		LOG("Defence :: phase %d starts t=%.1f", defence.step, GameTimeSec());
		return;
	}
	const GameXmlContent::DefencePhase& phase = phases[defence.step - 1];
	if(defence.kills >= phase.killCount) EndDefencePhase(MEASURED_PHASE_END_KILLS);
	else if(phase.timeout > 0 && TimeDiffSec(TimeDiff(defence.waveStartAt, localTime)) >= phase.timeout && !DefenceMonstersAlive()) EndDefencePhase(HYP_PHASE_END_TIMEOUT);
}

void World::SendMinerals()
{
	replication->SendTeamScoreToAll(Replication::SCORE_TYPE_MINERAL, minerals);
}

void World::DefenceMonsterDied(const ActorNpc& npc)
{
	if(!defence.active) return;
	if(npc.monsterAi && npc.monsterAi->mineral && npc.killerUID != ActorUID::INVALID && FindMasterActor(npc.killerUID)) {
		minerals++;
		mineralCarrierKills++;
		replication->SendPvpAnnouncementToAll(MEASURED_ANNOUNCE_MINERAL_GAIN, 0, npc.killerUID, npc.UID, 1);
		SendMinerals();
		replication->SendTeamScoreToAll(Replication::SCORE_TYPE_MINERAL_CARRIER_KILLS, mineralCarrierKills);
		LOG("Defence :: mineral carrier killed, %d mineral(s)", minerals);
	}
	if(defence.coreDoc != 0 && (i32)npc.docID == defence.coreDoc) {
		defence.failed = true;
		LOG("Defence :: core destroyed at phase %d t=%.1f", defence.step, GameTimeSec());
		return;
	}
	if(!defence.inWave || npc.faction != Faction::MONSTER) return;
	if(GetGameXmlContent().defencePhases[defence.step - 1].Excludes((i32)npc.docID)) return;
	defence.kills++;
}
