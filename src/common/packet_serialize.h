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
	// implement
	STATIC_ASSERT(0);
	ASSERT(0);
	return nullptr;
}

template<>
inline const char* PacketSerialize<Cl::CQ_FirstHello>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("CQ_FirstHello(%d, %d) :: {", Cl::CQ_FirstHello::NET_ID, packetSize);
	SER("	dwProtocolCRC=%x", buff.Read<u32>());
	SER("	dwErrorCRC=%x", buff.Read<u32>());
	SER("	version=%x", buff.Read<u32>());
	SER("	unknown=%d", buff.Read<u8>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_DoConnectGameServer>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_DoConnectGameServer(%d, %d) :: {", Sv::SN_DoConnectGameServer::NET_ID, packetSize);
	SER("	port=%x", buff.Read<u16>());
	SER("	ip=(%d.%d.%d.%d)", buff.Read<u8>(), buff.Read<u8>(), buff.Read<u8>(), buff.Read<u8>());
	SER("	gameID=%d", buff.Read<i32>());
	SER("	idcHash=%u", buff.Read<u32>());
	SER("	nickname='%s'", buff.ReadWideStringObj().data());
	SER("	instantKey=%d", buff.Read<i32>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_PlayerSkillSlot>(const void* packetData, const i32 packetSize)
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
inline const char* PacketSerialize<Sv::SN_GameFieldReady>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_GameFieldReady(%d, %d) :: {", Sv::SN_GameFieldReady::NET_ID, packetSize);
	SER("	gameID=%d", buff.Read<i32>());
	SER("	gameType=%d", buff.Read<i32>());
	SER("	areaIndex=%d", buff.Read<i32>());
	SER("	stageIndex=%d", buff.Read<i32>());
	SER("	gameDefinitionType=%d", buff.Read<i32>());
	SER("	initPlayerCount=%d", buff.Read<u8>());
	SER("	canEscape=%d", buff.Read<u8>());
	SER("	isTrespass=%d", buff.Read<u8>());
	SER("	isSpectator=%d", buff.Read<u8>());

	const u16 ingameUsers_len = buff.Read<u16>();
	SER("	IngameUsers(%d)=[", ingameUsers_len);

	for(int i = 0; i < ingameUsers_len; i++) {
		SER("	{");
		SER("		userID=%d", buff.Read<i32>());
		SER("		nick='%S'", buff.ReadWideStringObj().data());
		SER("		team=%d", buff.Read<u8>());
		SER("		isBot=%d", buff.Read<u8>());
		SER("	},");
	}

	const u16 ingamePlayers_len = buff.Read<u16>();
	SER("	IngamePlayers(%d)=[", ingamePlayers_len);

	for(int i = 0; i < ingamePlayers_len; i++) {
		SER("	{");
		SER("		userID=%d", buff.Read<i32>());
		SER("		mainCreatureIndex=%d", buff.Read<CreatureIndex>());
		SER("		mainSkinIndex=%d", buff.Read<SkinIndex>());
		SER("		mainSkillIndex1=%d", buff.Read<SkillID>());
		SER("		mainSkillIndex2=%d", buff.Read<SkillID>());
		SER("		subCreatureIndex=%d", buff.Read<CreatureIndex>());
		SER("		subSkinIndex1=%d", buff.Read<SkinIndex>());
		SER("		subSkillIndex1=%d", buff.Read<SkillID>());
		SER("		subSkillIndex2=%d", buff.Read<SkillID>());
		SER("		stageSkillIndex1=%d", buff.Read<SkillID>());
		SER("		stageSkillIndex2=%d", buff.Read<SkillID>());
		SER("		supportKitIndex=%d", buff.Read<i32>());
		SER("		isBot=%d", buff.Read<u8>());
		SER("	},");
	}

	const u16 ingameGuilds_len = buff.Read<u16>();
	SER("	IngameGuilds(%d)=[", ingameGuilds_len);

	for(int i = 0; i < ingameGuilds_len; i++) {
		SER("	{");
		SER("		teamType=%d", buff.Read<u8>());
		SER("		guildName='%S'", buff.ReadWideStringObj().data());
		SER("		guildTag='%S'", buff.ReadWideStringObj().data());
		SER("		guildEmblemIndex=%d", buff.Read<i32>());
		SER("		guildPvpRankNo=%d", buff.Read<i32>());
		SER("	},");
	}

	SER("	]");
	SER("	surrenderAbleTime=%d", buff.Read<i32>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_ProfileCharacters>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_ProfileCharacters(%d, %d) :: {", Sv::SN_ProfileCharacters::NET_ID, packetSize);

	const u16 count = buff.Read<u16>();
	SER("	characters(%d)=[", count);

	for(int i = 0; i < count; i++) {
		const Sv::SN_ProfileCharacters::Character chara = buff.Read<Sv::SN_ProfileCharacters::Character>();
		SER("	{");
		SER("		characterID=%d", chara.characterID);
		SER("		creatureIndex=%d", chara.creatureIndex);
		SER("		skillShot1=%d", chara.skillShot1);
		SER("		skillShot2=%d", chara.skillShot2);
		SER("		classType=%d", chara.classType);
		SER("		x=%d", chara.x);
		SER("		y=%d", chara.y);
		SER("		z=%d", chara.z);
		SER("		characterType=%d", chara.characterType);
		SER("		skinIndex=%d", chara.skinIndex);
		SER("		weaponIndex=%d", chara.weaponIndex);
		SER("		masterGearNo=%d", chara.masterGearNo);
		SER("	},");
	}

	SER("	]");
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_ProfileWeapons>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_ProfileWeapons(%d, %d) :: {", Sv::SN_ProfileWeapons::NET_ID, packetSize);

	const u16 count = buff.Read<u16>();
	SER("	weapons(%d)=[", count);

	for(int i = 0; i < count; i++) {
		const Sv::SN_ProfileWeapons::Weapon weap = buff.Read<Sv::SN_ProfileWeapons::Weapon>();
		SER("	{");
		SER("		characterID=%d", weap.characterID);
		SER("		weaponType=%d", weap.weaponType);
		SER("		weaponIndex=%d", weap.weaponIndex);
		SER("		grade=%d", weap.grade);
		SER("		isUnlocked=%d", weap.isUnlocked);
		SER("		isActivated=%d", weap.isActivated);
		SER("	},");
	}

	SER("	]");
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_ProfileSkills>(const void* packetData, const i32 packetSize)
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
