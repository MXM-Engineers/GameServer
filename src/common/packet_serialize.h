#pragma once
#include "protocol.h"
#include <EASTL/fixed_string.h>

namespace PS
{
	inline eastl::fixed_string<char,8192,false>& NewString()
	{
		static eastl::fixed_string<char,8192,false> buff;
		buff.clear();
		return buff;
	}
}

#define SER_BEGIN() auto& str = PS::NewString()
#define SER(fmt, ...) str.append_sprintf(fmt "\n", __VA_ARGS__);

template<typename Packet>
const char* PacketSerialize(const void* packetData, const i32 packetSize)
{
	ASSERT(0);
	return nullptr;
}

template<>
const char* PacketSerialize<Sv::SN_PlayerSkillSlot>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_PlayerSkillSlot(%d, %d) :: {", Sv::SN_PlayerSkillSlot::NET_ID, packetSize);
	SER("	characterID=%d", buff.Read<LocalActorID>());

	const u16 slotList_count = buff.Read<u16>();
	SER("	slotList(%d)=[", slotList_count);

	for(int i = 0; i < slotList_count; i++) {
		SER("	{");
		SER("		skillIndex=%d", buff.Read<SkillID>());
		SER("		coolTime=%d", buff.Read<i32>());
		SER("		unlocked=%d", buff.Read<u8>());
		SER("		propList_count=%d", buff.Read<u16>());
		SER("		isUnlocked=%d", buff.Read<u8>());
		SER("		isActivated=%d", buff.Read<u8>());
		SER("	},");
	}

	SER("	]");
	SER("	stageSkillIndex1=%d", buff.Read<i32>());
	SER("	stageSkillIndex2=%d", buff.Read<i32>());
	SER("	currentSkillSlot1=%d", buff.Read<i32>());
	SER("	currentSkillSlot2=%d", buff.Read<i32>());
	SER("	shirkSkillSlot=%d", buff.Read<i32>());
	SER("}");

	return str.data();
}

template<>
const char* PacketSerialize<Sv::SN_ProfileSkills>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_ProfileSkills(%d, %d) :: {", Sv::SN_ProfileSkills::NET_ID, packetSize);
	SER("	packetNum=%d", buff.Read<u8>());

	const u16 skills_count = buff.Read<u16>();
	SER("	skills(%d)=[", skills_count);

	for(int i = 0; i < skills_count; i++) {
		SER("	{");
		SER("		characterID=%d", buff.Read<LocalActorID>());
		SER("		skillIndex=%d", buff.Read<SkillID>());
		SER("		isUnlocked=%d", buff.Read<u8>());
		SER("		isActivated=%d", buff.Read<u8>());
		SER("		propList_count=%d", buff.Read<u16>());
		SER("	},");
	}

	SER("	]");
	SER("}");

	return str.data();
}


#undef SER_BEGIN
#undef SER
