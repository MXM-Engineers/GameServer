#include "config.h"
#include "world.h"
#include "world_common.h"
#include <mxm/game_content.h>

static const MasterGrouping::Unit* GroupingUnit(const MasterGrouping& g, ClassType classType)
{
	foreach_const(u, g.units) if(u->classType == classType) return &*u;
	return nullptr;
}

static bool GroupingHasPartner(const MasterGrouping& g, const MasterGrouping::Unit& self, ClassType partner)
{
	foreach_const(u, g.units) if(&*u != &self && u->classType == partner) return true;
	return false;
}

bool World::GroupingPartnerPresent(const Player& p, const MasterGrouping& g, ClassType partner) const
{
	if(g.kind == MasterGrouping::PAIR) return p.Sub().classType == partner;
	foreach_const(q, players) {
		if(&*q == &p || !q->body) continue;
		if(g.kind == MasterGrouping::FRIENDLY && q->team != p.team) continue;
		if(g.kind == MasterGrouping::ENEMY && q->team == p.team) continue;
		if(q->Main().classType == partner || q->Sub().classType == partner) return true;
	}
	return false;
}

f32 World::GroupingStatMod(const Player& p, i32 stat, bool mul) const
{
	f32 sum = 0;
	foreach_const(id, p.groupings) {
		const MasterGrouping* g = GetGameXmlContent().FindMasterGrouping(*id);
		const MasterGrouping::Unit* u = g ? GroupingUnit(*g, p.Main().classType) : nullptr;
		if(!u || u->effect != MasterGrouping::STAT) continue;
		foreach_const(m, u->mods) if((i32)m->stat == stat && m->mul == mul && m->stat != StatusMod::OTHER) sum += m->value;
	}
	return sum;
}

f32 World::GroupingDamageMul(const Player& attacker, const Player& target) const
{
	f32 mul = 1.0f;
	foreach_const(id, attacker.groupings) {
		const MasterGrouping* g = GetGameXmlContent().FindMasterGrouping(*id);
		const MasterGrouping::Unit* u = g && g->apply == MasterGrouping::HIT ? GroupingUnit(*g, attacker.Main().classType) : nullptr;
		if(u && u->effect == MasterGrouping::DAMAGE_BONUS && GroupingHasPartner(*g, *u, target.Main().classType)) mul *= 1.0f + u->value / 100.0f;
	}
	foreach_const(id, target.groupings) {
		const MasterGrouping* g = GetGameXmlContent().FindMasterGrouping(*id);
		const MasterGrouping::Unit* u = g && g->apply == MasterGrouping::HIT ? GroupingUnit(*g, target.Main().classType) : nullptr;
		if(u && u->effect == MasterGrouping::DAMAGE_REDUCE && GroupingHasPartner(*g, *u, attacker.Main().classType)) mul *= MAX(1.0f - u->value / 100.0f, 0.0f);
	}
	return mul;
}

void World::GroupingSummonKill(Player& killer, const ActorNpc& summon)
{
	if(summon.ownerUID == ActorUID::INVALID || summon.ownerClass < 0 || players[summon.ownerIndex].team == killer.team) return;
	foreach_const(id, killer.groupings) {
		const MasterGrouping* g = GetGameXmlContent().FindMasterGrouping(*id);
		const MasterGrouping::Unit* u = g && g->apply == MasterGrouping::KILL_SUMMONEE ? GroupingUnit(*g, killer.Main().classType) : nullptr;
		if(!u || u->effect != MasterGrouping::CONSUMABLE_STAT || !GroupingHasPartner(*g, *u, (ClassType)summon.ownerClass)) continue;
		foreach_const(m, u->mods) {
			if(m->statType != (i32)StatType::STAT_TYPE_MANA || killer.mana < 0) continue;
			killer.mana = MIN(killer.mana + m->value, MasterStat(killer.Main().classType, STAT_MANA));
			killer.manaDirty = true;
			LOG("Grouping :: %ls gains %.0f mana from grouping %d (summon %d)", killer.name.data(), m->value, g->id, (i32)summon.docID);
		}
	}
}

void World::UpdateMasterGroupings()
{
	const GameXmlContent& content = GetGameXmlContent();
	if(content.masterGroupings.empty()) return;
	foreach(p, players) {
		if(!p->body) continue;
		const ActorMaster& main = p->Main();
		eastl::fixed_vector<i32,8,false> want;
		if(!p->dead && main.classType != ClassType::NONE) {
			foreach_const(g, content.masterGroupings) {
				const MasterGrouping::Unit* u = GroupingUnit(*g, main.classType);
				if(!u || want.full()) continue;
				bool partner = false;
				foreach_const(o, g->units) if(&*o != u && GroupingPartnerPresent(*p, *g, o->classType)) partner = true;
				if(partner) want.push_back(g->id);
			}
		}
		const bool masterChanged = p->groupingMaster != main.UID;
		if(!masterChanged && want.size() == p->groupings.size() && eastl::equal(want.begin(), want.end(), p->groupings.begin())) continue;
		eastl::fixed_vector<i32,16,false> touched;
		const auto touch = [&](i32 id) {
			const MasterGrouping* g = content.FindMasterGrouping(id);
			if(!g) return;
			foreach_const(u, g->units) foreach_const(m, u->mods) if(!touched.full() && eastl::find(touched.begin(), touched.end(), m->statType) == touched.end()) touched.push_back(m->statType);
		};
		const ActorMaster* previous = p->groupingMaster != ActorUID::INVALID ? FindMasterActor(p->groupingMaster) : nullptr;
		foreach_const(id, p->groupings) {
			if(!masterChanged && eastl::find(want.begin(), want.end(), *id) != want.end()) continue;
			if(previous && p->clientHd != ClientHandle::INVALID) replication->SendMasterGroupingEffect(p->clientHd, previous->UID, *id, previous->classType, false);
			LOG("Grouping :: %ls grouping %d off", p->name.data(), *id);
			touch(*id);
		}
		foreach_const(id, want) {
			if(!masterChanged && eastl::find(p->groupings.begin(), p->groupings.end(), *id) != p->groupings.end()) continue;
			if(p->clientHd != ClientHandle::INVALID) replication->SendMasterGroupingEffect(p->clientHd, main.UID, *id, main.classType, true);
			LOG("Grouping :: %ls grouping %d on (class %d)", p->name.data(), *id, (i32)main.classType);
			touch(*id);
		}
		p->groupings = want;
		p->groupingMaster = main.UID;
		foreach_const(st, touched) {
			if(*st == (i32)StatType::STAT_TYPE_HEALTH) { RefreshMasterHealth(*p, p->Main()); continue; }
			if(*st == (i32)StatType::STAT_TYPE_MANA) continue;
			StatusMod::Stat mod = StatusMod::OTHER;
			foreach_const(g, content.masterGroupings) foreach_const(u, g->units) foreach_const(m, u->mods) if(m->statType == *st) mod = m->stat;
			const f32 add = GroupingStatMod(*p, mod, false);
			const f32 mul = GroupingStatMod(*p, mod, true);
			const WeaponIndex weapon = (main.UID == p->Main().UID) == ((p->mainCharaID & 1) == 0) ? p->mainWeapon : p->subWeapon;
			const f32 base = *st == (i32)StatType::STAT_TYPE_ATTACK ? GetGameXmlContent().GetWeaponAttack((i32)weapon) + LevelBonus(*p, main.classType, (u8)*st) : MasterStat(main.classType, *st) + StatBonus(*p, (u8)*st);
			replication->SendStatToAll(main.UID, *st, 0, (base + add) * (1.0f + mul / 100.0f));
		}
	}
}
