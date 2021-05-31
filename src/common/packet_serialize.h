#pragma once
#include "protocol.h"
#include <EASTL/fixed_string.h>

namespace PS
{
	inline eastl::fixed_string<char,8192,false>& NewString()
	{
		thread_local eastl::fixed_string<char,8192,false> buff;
		buff.clear();
		return buff;
	}

	inline const char* ToStr(const float2& v)
	{
		return FMT("(%g, %g)", v.x, v.y);
	}

	inline const char* ToStr(const float3& v)
	{
		return FMT("(%g, %g, %g)", v.x, v.y, v.z);
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
inline const char* PacketSerialize<Cl::CQ_PlayerCastSkill>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("CQ_PlayerCastSkill(%d, %d) :: {", Cl::CQ_PlayerCastSkill::NET_ID, packetSize);
	SER("	playerID=%d", buff.Read<LocalActorID>());
	SER("	skillID=%d", buff.Read<SkillID>());
	SER("	p3nPos=%s", PS::ToStr(buff.Read<float3>()));

	const u16 count = buff.Read<u16>();
	SER("	targetList(%u)=[", count);
	for(int i = 0; i < count; i++) {
		SER("		%d,", buff.Read<LocalActorID>());
	}
	SER("	]");

	SER("	posStruct={");
	SER("		pos=%s", PS::ToStr(buff.Read<float3>()));
	SER("		destPos=%s", PS::ToStr(buff.Read<float3>()));
	SER("		moveDir=%s", PS::ToStr(buff.Read<float2>()));
	SER("		rotateStruct=%s", PS::ToStr(buff.Read<float3>()));
	SER("		speed=%g",buff.Read<f32>());
	SER("		clientTime=%d",buff.Read<i32>());
	SER("	}");
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
inline const char* PacketSerialize<Sv::SN_GameCreateActor>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_GameCreateActor(%d, %d) :: {", Sv::SN_GameCreateActor::NET_ID, packetSize);
	SER("	objectID=%d", buff.Read<LocalActorID>());
	SER("	nType=%d", buff.Read<i32>());
	SER("	nIDX=%d", buff.Read<CreatureIndex>());
	SER("	dwLocalID=%d", buff.Read<i32>());
	SER("	p3nPos=%s", PS::ToStr(buff.Read<float3>()));
	SER("	p3nDir=%s", PS::ToStr(buff.Read<float3>()));
	SER("	spawnType=%d", buff.Read<i32>());
	SER("	actionState=%d", buff.Read<i32>());
	SER("	ownerID=%d", buff.Read<i32>());
	SER("	bDirectionToNearPC=%d", buff.Read<u8>());
	SER("	AiWanderDistOverride=%d", buff.Read<i32>());
	SER("	tagID=%d", buff.Read<i32>());
	SER("	faction=%d", buff.Read<i32>());
	SER("	classType=%d", buff.Read<i32>());
	SER("	skinIndex=%d", buff.Read<i32>());
	SER("	seed=%d", buff.Read<i32>());
	SER("	initStat={");

	const u16 maxStats_count = buff.Read<u16>();
	SER("		maxStats(%d)=[", maxStats_count);
	for(int i = 0; i < maxStats_count; i++) {
		u8 type = buff.Read<u8>();
		f32 value = buff.Read<f32>();
		SER("			(type=%d value=%g),", type, value);
	}
	SER("		]");

	const u16 curStats_count = buff.Read<u16>();
	SER("		curStats(%d)=[", curStats_count);
	for(int i = 0; i < curStats_count; i++) {
		u8 type = buff.Read<u8>();
		f32 value = buff.Read<f32>();
		SER("			(type=%d value=%g),", type, value);
	}
	SER("		]");
	SER("	}");

	SER("	isInSight=%d", buff.Read<u8>());
	SER("	isDead=%d", buff.Read<u8>());
	SER("	serverTime=%lld", buff.Read<i64>());

	SER("	meshChangeActionHistory=[");
	const u16 meshChangeActionHistory_count = buff.Read<u16>();
	for(int i = 0; i < meshChangeActionHistory_count; i++) {
		SER("		(actionState=%d serverTime=%lld),", buff.Read<i32>(), buff.Read<i64>());
	}
	SER("	]");

	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_GameCreateSubActor>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_GameCreateSubActor(%d, %d) :: {", Sv::SN_GameCreateSubActor::NET_ID, packetSize);
	SER("	objectID=%d", buff.Read<LocalActorID>());
	SER("	mainEntityID=%d", buff.Read<LocalActorID>());
	SER("	nType=%d", buff.Read<i32>());
	SER("	nIDX=%d", buff.Read<CreatureIndex>());
	SER("	dwLocalID=%d", buff.Read<i32>());
	SER("	p3nPos=%s", PS::ToStr(buff.Read<float3>()));
	SER("	p3nDir=%s", PS::ToStr(buff.Read<float3>()));
	SER("	spawnType=%d", buff.Read<i32>());
	SER("	actionState=%d", buff.Read<i32>());
	SER("	ownerID=%d", buff.Read<i32>());
	SER("	tagID=%d", buff.Read<i32>());
	SER("	faction=%d", buff.Read<i32>());
	SER("	classType=%d", buff.Read<i32>());
	SER("	skinIndex=%d", buff.Read<i32>());
	SER("	seed=%d", buff.Read<i32>());
	SER("	initStat={");

	const u16 maxStats_count = buff.Read<u16>();
	SER("		maxStats(%d)=[", maxStats_count);
	for(int i = 0; i < maxStats_count; i++) {
		u8 type = buff.Read<u8>();
		f32 value = buff.Read<f32>();
		SER("			(type=%d value=%g),", type, value);
	}
	SER("		]");

	const u16 curStats_count = buff.Read<u16>();
	SER("		curStats(%d)=[", curStats_count);
	for(int i = 0; i < curStats_count; i++) {
		u8 type = buff.Read<u8>();
		f32 value = buff.Read<f32>();
		SER("			(type=%d value=%g),", type, value);
	}
	SER("		]");
	SER("	}");

	SER("	meshChangeActionHistory=[");
	const u16 meshChangeActionHistory_count = buff.Read<u16>();
	for(int i = 0; i < meshChangeActionHistory_count; i++) {
		SER("		(actionState=%d serverTime=%lld),", buff.Read<i32>(), buff.Read<i64>());
	}
	SER("	]");

	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_GameLeaveActor>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_GameLeaveActor(%d, %d) :: {", Sv::SN_GameLeaveActor::NET_ID, packetSize);
	SER("	objectID=%d", buff.Read<LocalActorID>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_StatusSnapshot>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_StatusSnapshot(%d, %d) :: {", Sv::SN_PlayerSkillSlot::NET_ID, packetSize);
	SER("	objectID=%d", buff.Read<LocalActorID>());

	const u16 count = buff.Read<u16>();
	SER("	statusArray(%d)=[", count);
	for(int i = 0; i < count; i++) {
		SER("	{");
		SER("		statusIndex=%d", buff.Read<SkillID>());
		SER("		bEnabled=%d", buff.Read<u8>());
		SER("		caster=%d", buff.Read<i32>());
		SER("		overlapCount=%d", buff.Read<u8>());
		SER("		customValue=%d", buff.Read<u8>());
		SER("		durationTimeMs=%d", buff.Read<i32>());
		SER("		remainTimeMs=%d", buff.Read<i32>());
		SER("	},");
	}

	SER("	]");
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_CastSkill>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_CastSkill(%d, %d) :: {", Sv::SN_CastSkill::NET_ID, packetSize);
	SER("	entityID=%d", buff.Read<LocalActorID>());
	SER("	ret=%d", buff.Read<i32>());
	SER("	skillID=%d", buff.Read<SkillID>());
	SER("	costLevel=%u", buff.Read<u8>());
	SER("	actionState=%d", buff.Read<ActionStateID>());
	SER("	tartgetPos=%s", PS::ToStr(buff.Read<float3>()));

	const u16 count = buff.Read<u16>();
	SER("	targetList(%u)=[", count);
	for(int i = 0; i < count; i++) {
		SER("		%d,", buff.Read<LocalActorID>());
	}
	SER("	]");

	SER("	bSyncMyPosition=%u", buff.Read<u8>());
	SER("	posStruct={");
	SER("		pos=%s", PS::ToStr(buff.Read<float3>()));
	SER("		destPos=%s", PS::ToStr(buff.Read<float3>()));
	SER("		moveDir=%s", PS::ToStr(buff.Read<float2>()));
	SER("		rotateStruct=%s", PS::ToStr(buff.Read<float3>()));
	SER("		speed=%g",buff.Read<f32>());
	SER("		clientTime=%d",buff.Read<i32>());
	SER("	}");
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_ExecuteSkill>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_ExecuteSkill(%d, %d) :: {", Sv::SN_ExecuteSkill::NET_ID, packetSize);
	SER("	entityID=%d", buff.Read<LocalActorID>());
	SER("	ret=%d", buff.Read<i32>());
	SER("	skillID=%d", buff.Read<SkillID>());
	SER("	costLevel=%u", buff.Read<u8>());
	SER("	actionState=%d", buff.Read<ActionStateID>());
	SER("	tartgetPos=%s", PS::ToStr(buff.Read<float3>()));

	const u16 count = buff.Read<u16>();
	SER("	targetList(%u)=[", count);
	for(int i = 0; i < count; i++) {
		SER("		%d,", buff.Read<LocalActorID>());
	}
	SER("	]");

	SER("	bSyncMyPosition=%u", buff.Read<u8>());
	SER("	posStruct={");
	SER("		pos=%s", PS::ToStr(buff.Read<float3>()));
	SER("		destPos=%s", PS::ToStr(buff.Read<float3>()));
	SER("		moveDir=%s", PS::ToStr(buff.Read<float2>()));
	SER("		rotateStruct=%s", PS::ToStr(buff.Read<float3>()));
	SER("		speed=%g",buff.Read<f32>());
	SER("		clientTime=%d",buff.Read<i32>());
	SER("	}");
	SER("	fSkillChargeDamageMultiplier=%g", buff.Read<f32>());
	SER("	graphMove={");
	SER("		bApply=%d", buff.Read<u8>());
	SER("		startPos=%s", PS::ToStr(buff.Read<float3>()));
	SER("		endPos=%s", PS::ToStr(buff.Read<float3>()));
	SER("		durationTimeS=%g", buff.Read<f32>());
	SER("		originDistance=%g", buff.Read<f32>());
	SER("	}");
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SA_CastSkill>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	Sv::SA_CastSkill cast = SafeCast<Sv::SA_CastSkill>(packetData, packetSize);

	SER("SA_CastSkill(%d, %d) :: {", Sv::SA_CastSkill::NET_ID, packetSize);
	SER("	characterID=%d", cast.characterID);
	SER("	ret=%d", cast.ret);
	SER("	skillID=%d", cast.skillIndex);
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
inline const char* PacketSerialize<Sv::SN_LoadClearedStages>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_LoadClearedStages(%d, %d) :: {", Sv::SN_LoadClearedStages::NET_ID, packetSize);
	const u16 count = buff.Read<u16>();
	SER("	clearedStageList(%d)=[", count);
	for(int i = 0; i < count; i++) {
		SER("	%d,", buff.Read<i32>());
	}
	SER("	]");
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
inline const char* PacketSerialize<Sv::SN_GamePlayerEquipWeapon>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_GamePlayerEquipWeapon(%d, %d) :: {", Sv::SN_GamePlayerEquipWeapon::NET_ID, packetSize);
	SER("	characterID=%d", buff.Read<LocalActorID>());
	SER("	weaponIndex=%d", buff.Read<WeaponIndex>());
	SER("	additionnalOverHeatGauge=%d", buff.Read<i32>());
	SER("	additionnalOverHeatGaugeRatio=%d", buff.Read<i32>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_GamePlayerStock>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_GamePlayerStock(%d, %d) :: {", Sv::SN_GamePlayerStock::NET_ID, packetSize);
	SER("	characterID=%d", buff.Read<LocalActorID>());
	SER("	name='%S'", buff.ReadWideStringObj().data());
	SER("	classType=%d", buff.Read<ClassType>());
	SER("	displayTitleIDX=%d", buff.Read<i32>());
	SER("	statTitleIDX=%d", buff.Read<i32>());
	SER("	badgeType=%d", buff.Read<u8>());
	SER("	badgeTierLevel=%d", buff.Read<u8>());
	SER("	guildTag='%S'", buff.ReadWideStringObj().data());
	SER("	vipLevel=%d", buff.Read<u8>());
	SER("	staffType=%d", buff.Read<u8>());
	SER("	isSubstituted=%d", buff.Read<u8>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_GamePlayerTag>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	const Sv::SN_GamePlayerTag& packet = *(Sv::SN_GamePlayerTag*)packetData;

	SER("SN_GamePlayerTag(%d, %d) :: {", Sv::SN_GamePlayerTag::NET_ID, packetSize);
	SER("	result=%d", packet.result);
	SER("	mainID=%u", packet.mainID);
	SER("	subID=%u", packet.subID);
	SER("	attackerID=%u", packet.attackerID);
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

template<>
inline const char* PacketSerialize<Sv::SN_NotifyPcDetailInfos>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_NotifyPcDetailInfos(%d, %d) :: {", Sv::SN_NotifyPcDetailInfos::NET_ID, packetSize);

	const u16 count = buff.Read<u16>();
	SER("	pcList(%d)=[", count);

	for(int i = 0; i < count; i++) {
		SER("	{");
		SER("		userID=%d", buff.Read<i32>());
		Sv::SN_NotifyPcDetailInfos::ST_PcInfo mainPc = buff.Read<Sv::SN_NotifyPcDetailInfos::ST_PcInfo>();
		Sv::SN_NotifyPcDetailInfos::ST_PcInfo subPc = buff.Read<Sv::SN_NotifyPcDetailInfos::ST_PcInfo>();
		SER("		mainPc=(characterID=%d docID=%d classType=%d hp=%d maxHp=%d)", mainPc.characterID, mainPc.docID, mainPc.classType, mainPc.hp, mainPc.maxHp);
		SER("		subPc=(characterID=%d docID=%d classType=%d hp=%d maxHp=%d)", subPc.characterID, subPc.docID, subPc.classType, subPc.hp, subPc.maxHp);
		SER("		remainTagCooltimeMS=%d", buff.Read<i32>());
		SER("		canCastSkillSlotUG=%d", buff.Read<u8>());
		SER("	},");
	}

	SER("	]");
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_WeaponState>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_WeaponState(%d, %d) :: {", Sv::SN_WeaponState::NET_ID, packetSize);
	SER("	ownerID=%d", buff.Read<LocalActorID>());
	SER("	weaponID=%d", buff.Read<i32>());
	SER("	state=%d", buff.Read<i32>());
	SER("	chargeLevel=%d", buff.Read<u8>());
	SER("	firingCombo=%d", buff.Read<u8>());
	SER("	result=%d", buff.Read<i32>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_InitScoreBoard>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_InitScoreBoard(%d, %d) :: {", Sv::SN_InitScoreBoard::NET_ID, packetSize);

	const u16 count = buff.Read<u16>();
	SER("	userInfos(%d)=[", count);

	for(int i = 0; i < count; i++) {
		SER("	{");
		SER("		usn=%d", buff.Read<i32>());
		SER("		name='%S'", buff.ReadWideStringObj().data());
		SER("		teamType=%d", buff.Read<i32>());
		SER("		mainCreatureIndex=%d", buff.Read<CreatureIndex>());
		SER("		subCreatureIndex=%d", buff.Read<CreatureIndex>());
		SER("	},");
	}

	SER("	]");
	SER("}");

	return str.data();
}


#undef SER_BEGIN
#undef SER
