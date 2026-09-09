#pragma once
#include <common/base.h>
#include <mxm/core.h>

struct StatValue
{
	u8 type;
	f32 value;
};

template<typename W>
inline void WriteInitStat(W& packet, const StatValue* stats, u16 count)
{
	ASSERT(stats);
	ASSERT(count > 0);

	packet.Write<u16>(count);
	for(u16 i = 0; i < count; i++) {
		packet.Write<u8>(stats[i].type);
		packet.Write<f32>(stats[i].value);
	}

	StatValue cur[8];
	u16 n = 0;
	f32 health = 0.f;
	f32 stamina = 0.f;
	f32 resourceVal = 0.f;
	u8 resourceType = 0;
	bool hasUlt = false;
	bool hasRes = false;
	for(u16 i = 0; i < count; i++) {
		const StatValue& s = stats[i];
		switch((StatType)s.type) {
		case StatType::STAT_TYPE_HEALTH: health = s.value; break;
		case StatType::STAT_TYPE_STAMINA: stamina = s.value; break;
		case StatType::STAT_TYPE_ULTIMATE: hasUlt = true; break;
		case StatType::STAT_TYPE_MANA:
		case StatType::STAT_TYPE_RAGE:
		case StatType::STAT_TYPE_ENERGY:
			hasRes = true;
			resourceType = s.type;
			resourceVal = s.value;
			break;
		default: break;
		}
	}
	cur[n++] = { (u8)StatType::STAT_TYPE_HEALTH, health };
	if(hasUlt) cur[n++] = { (u8)StatType::STAT_TYPE_ULTIMATE, 0.f };
	if(hasRes) cur[n++] = { resourceType, resourceVal };
	if(stamina != 0.f) cur[n++] = { (u8)StatType::STAT_TYPE_STAMINA, stamina };

	packet.Write<u16>(n);
	for(u16 i = 0; i < n; i++) {
		packet.Write<u8>(cur[i].type);
		packet.Write<f32>(cur[i].value);
	}
}

template<typename W, typename Stats>
inline void WriteInitStat(W& packet, const Stats& stats)
{
	WriteInitStat(packet, stats.data(), (u16)stats.size());
}
