#pragma once
#include "protocol.h"
#include "inner_protocol.h"
#include <EASTL/fixed_string.h>

namespace PS
{
	inline eastl::fixed_string<char,8192,true>& NewString()
	{
		thread_local eastl::fixed_string<char,8192,true> buff;
		buff.clear();
		return buff;
	}

	inline const char* ToStr(const float2& v)
	{
		return FMT("(%f, %f)", v.x, v.y);
	}

	inline const char* ToStr(const float3& v)
	{
		return FMT("(%f, %f, %f)", v.x, v.y, v.z);
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
inline const char* PacketSerialize<Sv::SN_GameEnterActor>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_GameEnterActor(%d, %d) :: {", Sv::SN_GameEnterActor::NET_ID, packetSize);

	u8 excludedBits = buff.Read<u8>();
	SER("	excludedBits=%x",  excludedBits);

	// graph move
	if((excludedBits & 1) == 0) {
		SER("	extraGraphMove={");
		SER("		hasGraphMove=%d",  buff.Read<u8>());
		SER("		distance=%f",  buff.Read<f32>());
		SER("		totalTimeS=%f",  buff.Read<f32>());
		SER("		curTimeS=%f",  buff.Read<f32>());
		SER("		startPos=%s",  PS::ToStr(buff.Read<float3>()));
		SER("		endPos=%s",  PS::ToStr(buff.Read<float3>()));
		SER("		originDistance=%f",  buff.Read<f32>());
		SER("		hasExtraMove=%d",  buff.Read<u8>());
		SER("		vExtraPointMoveTarget=%s",  PS::ToStr(buff.Read<float2>()));
		SER("		vExtraPointMoveRemainTime=%f",  buff.Read<f32>());
		SER("		vExtraDirMove=%s",  PS::ToStr(buff.Read<float2>()));
		SER("		vExtraDirMoveRemainTime=%f",  buff.Read<f32>());
		SER("	}");
	}

	SER("	objectID=%d",  buff.Read<i32>())
	SER("	p3nPos=%s",  PS::ToStr(buff.Read<float3>()))
	SER("	p3nDir=%s",  PS::ToStr(buff.Read<float3>()))
	SER("	p2nMoveDir=%s",  PS::ToStr(buff.Read<float2>()))
	SER("	p2nUpperDir=%s",  PS::ToStr(buff.Read<float2>()))
	SER("	p3nMoveTargetPos=%s",  PS::ToStr(buff.Read<float3>()))
	SER("	isBattleState=%d",  buff.Read<u8>())
	SER("	baseMoveSpeed=%f",  buff.Read<f32>())
	SER("	actionState=%d",  buff.Read<i32>())
	SER("	aiTargetID=%d",  buff.Read<i32>())

	SER("	statSnapshot={")

		u16 count = buff.Read<u16>();
		SER("		curStats(%d)=[",  count)
		while(count > 0) {
			u8 type = buff.Read<u8>();
			f32 value = buff.Read<f32>();
			SER("			(type=%d value=%f),",  type, value);
			count--;
		}
		SER("		]");

		count = buff.Read<u16>();
		SER("		maxStats(%d)=[",  count);
		while(count > 0) {
			u8 type = buff.Read<u8>();
			f32 value = buff.Read<f32>();
			SER("			(type=%d value=%f),",  type, value);
			count--;
		}
		SER("		]");

		count = buff.Read<u16>();
		SER("		addPrivate(%d)=[",  count);
		while(count > 0) {
			u8 type = buff.Read<u8>();
			f32 value = buff.Read<f32>();
			SER("			(type=%d value=%f),",  type, value);
			count--;
		}
		SER("		]");

		count = buff.Read<u16>();
		SER("		mulPrivate(%d)=[",  count);
		while(count > 0) {
			u8 type = buff.Read<u8>();
			f32 value = buff.Read<f32>();
			SER("			(type=%d value=%f),",  type, value);
			count--;
		}
		SER("		]");

	SER("	}");
	SER("}");

	ASSERT(!buff.CanRead(1));
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

template<>
inline const char* PacketSerialize<Sv::SN_PlayerSyncMove>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_PlayerSyncMove(%d, %d) :: {", Sv::SN_PlayerSyncMove::NET_ID, packetSize);
	SER("	characterID=%d", buff.Read<LocalActorID>());
	SER("	destPos=%s", PS::ToStr(buff.Read<float3>()));
	SER("	moveDir=%s", PS::ToStr(buff.Read<float2>()));
	SER("	upperDir=%s", PS::ToStr(buff.Read<float2>()));
	SER("	nRotate=%f", buff.Read<f32>());
	SER("	nSpeed=%f", buff.Read<f32>());
	SER("	flags=%u", buff.Read<u8>());
	ActionStateID action = buff.Read<ActionStateID>();
	SER("	state=%s (%d)", ActionStateString(action), action);
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SA_RTT_Time>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();

	const Sv::SA_RTT_Time& p = *(Sv::SA_RTT_Time*)packetData;

	SER("SA_RTT_Time(%d, %d) :: {", Sv::SA_RTT_Time::NET_ID, packetSize);
	SER("	clientTimestamp=%u", p.clientTimestamp);
	SER("	serverTimestamp=%lld", p.serverTimestamp);
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SA_ResultSpAction>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SA_ResultSpAction(%d, %d) :: {", Sv::SA_ResultSpAction::NET_ID, packetSize);
	SER("	excludedFieldBits=%d", buff.Read<u8>());
	SER("	actionID=%d", buff.Read<i32>());
	SER("	localActorID=%d", buff.Read<LocalActorID>());
	SER("	rotate=%f", buff.Read<f32>());
	SER("	moveDir=%s", PS::ToStr(buff.Read<float2>()));
	SER("	errorType=%d", buff.Read<i32>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SA_PartyCreate>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SA_PartyCreate(%d, %d) :: {", Sv::SA_PartyCreate::NET_ID, packetSize);
	SER("	retval=%d", buff.Read<i32>());
	SER("	ownerUserID=%d", buff.Read<i32>());
	SER("	stageType=%d", buff.Read<i32>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SA_AreaPopularity>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SA_AreaPopularity(%d, %d) :: {", Sv::SA_AreaPopularity::NET_ID, packetSize);
	SER("	errCode=%d", buff.Read<i32>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_AreaPopularity>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_AreaPopularity(%d, %d) :: {", Sv::SN_AreaPopularity::NET_ID, packetSize);
	SER("	areaCode=%u", buff.Read<u32>());

	const u16 count = buff.Read<u16>();
	SER("	popularities(%d)=[", count);
	for(int i = 0; i < count; i++) {
		SER("	{");
		SER("		stageIndex=%d", buff.Read<i32>());
		SER("		gameType=%d", buff.Read<i32>());
		SER("		popularityLevel=%d", buff.Read<i32>());
		SER("	},");
	}
	SER("	]");
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SA_FirstHello>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();

	const Sv::SA_FirstHello& packet = *(Sv::SA_FirstHello*)packetData;

	SER("SA_FirstHello(%d, %d) :: {", Sv::SA_AreaPopularity::NET_ID, packetSize);
	SER("	dwProtocolCRC=%u", packet.dwProtocolCRC);
	SER("	dwErrorCRC=%u", packet.dwErrorCRC);
	SER("	serverType=%u", packet.serverType);
	SER("	clientIp=(%u.%u.%u.%u)", packet.clientIp[0], packet.clientIp[1], packet.clientIp[2], packet.clientIp[3]);
	SER("	clientPort=%u", packet.clientPort);
	SER("	tqosWorldId=%u", packet.tqosWorldId);
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SA_CalendarDetail>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SA_CalendarDetail(%d, %d) :: {", Sv::SA_CalendarDetail::NET_ID, packetSize);
	SER("	todayUTCDateTime=%llu", buff.Read<u64>());

	const u16 count = buff.Read<u16>();
	SER("	events(%d)=[", count);
	for(int i = 0; i < count; i++) {
		SER("	{");
		SER("		dataType=%u", buff.Read<u8>());
		SER("		index=%d", buff.Read<i32>());
		SER("		startDateTime=%llu", buff.Read<u64>());
		SER("		endDateTime=%llu", buff.Read<u64>());
		SER("	},");
	}
	SER("	]");
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_Authenticate>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("CQ_Authenticate(%d, %d) :: {", Cl::CQ_Authenticate::NET_ID, packetSize);
	auto nick = buff.ReadWideStringObj();
	SER("	nick='%ls'", nick.data());
	SER("	var=%d", buff.Read<i32>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_GetGuildRankingSeasonList>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("CQ_GetGuildRankingSeasonList(%d, %d) :: {", Cl::CQ_GetGuildRankingSeasonList::NET_ID, packetSize);
	SER("	rankingType=%d", buff.Read<u8>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<In::MR_Handshake>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("MR_Handshake(%d, %d) :: {", In::MR_Handshake::NET_ID, packetSize);
	SER("	result=%d", buff.Read<u8>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SA_PartyModify>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SA_PartyModify(%d, %d) :: {", Sv::SA_PartyModify::NET_ID, packetSize);
	SER("	retval=%d", buff.Read<i32>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SA_PartyOptionModify>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SA_PartyOptionModify(%d, %d) :: {", Sv::SA_PartyOptionModify::NET_ID, packetSize);
	SER("	retval=%d", buff.Read<i32>());
	SER("	option=%u", buff.Read<u8>());
	SER("	enable=%u", buff.Read<u8>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SA_EnqueueGame>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SA_EnqueueGame(%d, %d) :: {", Sv::SA_EnqueueGame::NET_ID, packetSize);
	SER("	retval=%d", buff.Read<i32>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_PartyModify>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("CQ_PartyModify(%d, %d) :: {", Cl::CQ_PartyModify::NET_ID, packetSize);
	const u16 count = buff.Read<u16>();
	SER("	stageList(%d)=[", count);
	for(int i = 0; i < count; i++) {
		SER("		%d,", buff.Read<i32>());
	}
	SER("	]");
	SER("	gametype=%d", buff.Read<i32>());
	SER("	unk1=%d", buff.Read<i32>());
	SER("	un2=%u", buff.Read<u8>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_PartyOptionModify>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("CQ_PartyOptionModify(%d, %d) :: {", Cl::CQ_PartyOptionModify::NET_ID, packetSize);
	SER("	option=%u", buff.Read<u8>());
	SER("	enable=%u", buff.Read<u8>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CA_SortieRoomFound>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("CA_SortieRoomFound(%d, %d) :: {", Cl::CA_SortieRoomFound::NET_ID, packetSize);
	SER("	sortieID=%lld", buff.Read<i64>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CN_SortieRoomConfirm>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("CN_SortieRoomConfirm(%d, %d) :: {", Cl::CN_SortieRoomConfirm::NET_ID, packetSize);
	SER("	confirm=%u", buff.Read<u8>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_EnqueueMatchingQueue>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_EnqueueMatchingQueue(%d, %d) :: {", Sv::SN_EnqueueMatchingQueue::NET_ID, packetSize);
	SER("	stageIndex=%d", buff.Read<i32>());
	SER("	currentMatchingTimeMs=%d", buff.Read<i32>());
	SER("	avgMatchingTimeMs=%d", buff.Read<i32>());
	SER("	disableMatchExpansion=%u", buff.Read<u8>());
	SER("	isMatchingExpanded=%u", buff.Read<u8>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SQ_MatchingPartyFound>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SQ_MatchingPartyFound(%d, %d) :: {", Sv::SQ_MatchingPartyFound::NET_ID, packetSize);
	SER("	sortieID=%lld", buff.Read<i64>());
	SER("	...");
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_MatchingPartyGathered>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_MatchingPartyGathered(%d, %d) :: {", Sv::SN_MatchingPartyGathered::NET_ID, packetSize);
	SER("	allConfirmed=%u", buff.Read<u8>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_SortieMasterPickPhaseStart>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_SortieMasterPickPhaseStart(%d, %d) :: {", Sv::SN_SortieMasterPickPhaseStart::NET_ID, packetSize);
	SER("	isRandomPick=%u", buff.Read<u8>());
	SER("	...");
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_MasterRotationInfo>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_MasterRotationInfo(%d, %d) :: {", Sv::SN_MasterRotationInfo::NET_ID, packetSize);
	SER("	refreshCount=%d", buff.Read<i32>());
	const u16 count = buff.Read<u16>();
	SER("	freeRotation(%d)=[", count);
	for(int i = 0; i < count; i++) {
		SER("		%d,", buff.Read<i32>());
	}
	SER("	]");

	const u16 count2 = buff.Read<u16>();
	SER("	pccafeRotation(%d)=[", count2);
	for(int i = 0; i < count2; i++) {
		SER("		%d,", buff.Read<i32>());
	}
	SER("	]");

	const u16 count3 = buff.Read<u16>();
	SER("	pccafeRotation(%d)=[", count3);
	for(int i = 0; i < count3; i++) {
		SER("		%d,", buff.Read<i32>());
	}
	SER("	]");

	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_SortieMasterPickPhaseStepStart>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_SortieMasterPickPhaseStepStart(%d, %d) :: {", Sv::SN_SortieMasterPickPhaseStepStart::NET_ID, packetSize);
	SER("	timeSec=%d", buff.Read<i32>());

	const u16 count = buff.Read<u16>();
	SER("	alliesTeamUserIds(%d)=[", count);
	for(int i = 0; i < count; i++) {
		SER("		%d,", buff.Read<i32>());
	}
	SER("	]");

	const u16 count2 = buff.Read<u16>();
	SER("	enemiesTeamUserIds(%d)=[", count2);
	for(int i = 0; i < count2; i++) {
		SER("		%d,", buff.Read<i32>());
	}
	SER("	]");

	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<In::MR_PartyCreated>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	In::MR_PartyCreated packet = SafeCast<In::MR_PartyCreated>(packetData, packetSize);

	SER("MR_PartyCreated(%d, %d) :: {", In::MR_PartyCreated::NET_ID, packetSize);
	SER("	result=%u", packet.result);
	SER("	partyUID=%u", packet.partyUID);
	SER("	leader=%u", packet.leader);
	SER("	instanceUID=%u", packet.instanceUID);
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<In::MR_PartyEnqueued>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	In::MR_PartyEnqueued packet = SafeCast<In::MR_PartyEnqueued>(packetData, packetSize);

	SER("MR_PartyEnqueued(%d, %d) :: {", In::MR_PartyEnqueued::NET_ID, packetSize);
	SER("	result=%u", packet.result);
	SER("	partyUID=%u", packet.partyUID);
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<In::HQ_PartyCreate>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	In::HQ_PartyCreate packet = SafeCast<In::HQ_PartyCreate>(packetData, packetSize);

	SER("HQ_PartyCreate(%d, %d) :: {", In::HQ_PartyCreate::NET_ID, packetSize);
	SER("	leader=%u", packet.leader);
	SER("	instanceUID=%u", packet.instanceUID);
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<In::HQ_PartyEnqueue>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	In::HQ_PartyEnqueue packet = SafeCast<In::HQ_PartyEnqueue>(packetData, packetSize);

	SER("HQ_PartyEnqueue(%d, %d) :: {", In::HQ_PartyEnqueue::NET_ID, packetSize);
	SER("	partyUID=%u", packet.partyUID);
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<In::MN_MatchFound>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	In::MN_MatchFound packet = SafeCast<In::MN_MatchFound>(packetData, packetSize);

	SER("MN_MatchFound(%d, %d) :: {", In::MN_MatchFound::NET_ID, packetSize);
	SER("	partyUID=%u", packet.partyUID);
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<In::HN_PlayerRoomFound>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	In::HN_PlayerRoomFound packet = SafeCast<In::HN_PlayerRoomFound>(packetData, packetSize);

	SER("HN_PlayerNotifyRoomFound(%d, %d) :: {", In::HN_PlayerRoomFound::NET_ID, packetSize);
	SER("	accountUID=%u", packet.accountUID);
	SER("	sortieUID=%llu", packet.sortieUID);
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<In::HN_PlayerRoomConfirm>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	In::HN_PlayerRoomConfirm packet = SafeCast<In::HN_PlayerRoomConfirm>(packetData, packetSize);

	SER("HN_PlayerRoomConfirm(%d, %d) :: {", In::HN_PlayerRoomConfirm::NET_ID, packetSize);
	SER("	accountUID=%u", packet.accountUID);
	SER("	confirm=%u", packet.confirm);
	SER("	sortieUID=%llu", packet.sortieUID);
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<In::MN_RoomCreated>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	In::MN_RoomCreated packet = SafeCast<In::MN_RoomCreated>(packetData, packetSize);

	SER("MN_SortieBegin(%d, %d) :: {", In::MN_RoomCreated::NET_ID, packetSize);
	SER("	sortieUID=%llu", packet.sortieUID);
	SER("	...");
	SER("}");

	return str.data();
}

#define DEFAULT_SERIALIZE(PACKET)\
	template<>\
	inline const char* PacketSerialize<PACKET>(const void* packetData, const i32 packetSize)\
	{\
		SER_BEGIN();\
		ConstBuffer buff(packetData, packetSize);\
	\
		SER(#PACKET "(%d, %d) :: {}", PACKET::NET_ID, packetSize);\
	\
		return str.data();\
	}

DEFAULT_SERIALIZE(Sv::SN_LoadCharacterStart);
DEFAULT_SERIALIZE(Sv::SN_SummaryInfoAll);
DEFAULT_SERIALIZE(Sv::SN_AvailableSummaryRewardCountList);
DEFAULT_SERIALIZE(Sv::SN_SummaryInfoLatest);
DEFAULT_SERIALIZE(Sv::SN_AchieveInfo);
DEFAULT_SERIALIZE(Sv::SN_AchieveLatest);
DEFAULT_SERIALIZE(Sv::SQ_CityLobbyJoinCity);
DEFAULT_SERIALIZE(Sv::SN_ChatChannelMessage);
DEFAULT_SERIALIZE(Sv::SA_WhisperSend);
DEFAULT_SERIALIZE(Sv::SN_WhisperReceive);
DEFAULT_SERIALIZE(Sv::SN_ProfileItems);
DEFAULT_SERIALIZE(Sv::SN_ProfileMasterGears);
DEFAULT_SERIALIZE(Sv::SN_AccountInfo);
DEFAULT_SERIALIZE(Sv::SN_AccountEquipmentList);
DEFAULT_SERIALIZE(Sv::SA_LoadingComplete);
DEFAULT_SERIALIZE(Sv::SN_GameStart);
DEFAULT_SERIALIZE(Sv::SN_SpawnPosForMinimap);
DEFAULT_SERIALIZE(Sv::SN_InitIngameModeInfo);
DEFAULT_SERIALIZE(Sv::SN_ScanEnd);
DEFAULT_SERIALIZE(Sv::SN_SetGameGvt);
DEFAULT_SERIALIZE(Sv::SN_CityMapInfo);
DEFAULT_SERIALIZE(Sv::SN_UpdateGameOwner);
DEFAULT_SERIALIZE(Sv::SN_LobbyStartGame);
DEFAULT_SERIALIZE(Sv::SA_GetCharacterInfo);
DEFAULT_SERIALIZE(Sv::SN_LeaderCharacter);
DEFAULT_SERIALIZE(Sv::SA_SetLeader);
DEFAULT_SERIALIZE(Sv::SN_Money);
DEFAULT_SERIALIZE(Sv::SA_GameReady);
DEFAULT_SERIALIZE(Sv::SN_NotifyIngameSkillPoint);
DEFAULT_SERIALIZE(Sv::SN_NotifyTimestamp);
DEFAULT_SERIALIZE(Sv::SN_NotifyAasRestricted);
DEFAULT_SERIALIZE(Sv::SN_RunClientLevelEventSeq);
DEFAULT_SERIALIZE(Sv::SN_PlayerSyncTurn);
DEFAULT_SERIALIZE(Sv::SN_DestroyEntity);
DEFAULT_SERIALIZE(Sv::SN_RegionServicePolicy);
DEFAULT_SERIALIZE(Sv::SN_AllCharacterBaseData);
DEFAULT_SERIALIZE(Sv::SN_MyGuild);
DEFAULT_SERIALIZE(Sv::SN_ProfileTitles);
DEFAULT_SERIALIZE(Sv::SN_ProfileCharacterSkinList);
DEFAULT_SERIALIZE(Sv::SN_AccountExtraInfo);
DEFAULT_SERIALIZE(Sv::SN_GuildChannelEnter);
DEFAULT_SERIALIZE(Sv::SN_FriendList);
DEFAULT_SERIALIZE(Sv::SN_PveComradeInfo);
DEFAULT_SERIALIZE(Sv::SN_AchieveUpdate);
DEFAULT_SERIALIZE(Sv::SN_FriendRequestList);
DEFAULT_SERIALIZE(Sv::SN_BlockList);
DEFAULT_SERIALIZE(Sv::SN_MailUnreadNotice);
DEFAULT_SERIALIZE(Sv::SN_WarehouseItems);
DEFAULT_SERIALIZE(Sv::SN_MutualFriendList);
DEFAULT_SERIALIZE(Sv::SN_GuildMemberStatus);
DEFAULT_SERIALIZE(Sv::SN_GamePlayerSyncByInt);
DEFAULT_SERIALIZE(Sv::SN_PlayerSyncActionStateOnly);
DEFAULT_SERIALIZE(Sv::SN_JukeboxPlay);
DEFAULT_SERIALIZE(Sv::SN_JukeboxEnqueuedList);
DEFAULT_SERIALIZE(Sv::SN_TownHudStatistics);
DEFAULT_SERIALIZE(Sv::SA_AuthResult);
DEFAULT_SERIALIZE(Sv::SA_GetGuildProfile);
DEFAULT_SERIALIZE(Sv::SA_GetGuildMemberList);
DEFAULT_SERIALIZE(Sv::SA_GetGuildHistoryList);
DEFAULT_SERIALIZE(Sv::SA_GetGuildRankingSeasonList);
DEFAULT_SERIALIZE(Sv::SA_TierRecord);
DEFAULT_SERIALIZE(Sv::SN_ClientSettings);
DEFAULT_SERIALIZE(Cl::CQ_AuthenticateGameServer);
DEFAULT_SERIALIZE(Cl::CQ_GetGuildProfile);
DEFAULT_SERIALIZE(Cl::CQ_GetGuildMemberList);
DEFAULT_SERIALIZE(Cl::CQ_GetGuildHistoryList);
DEFAULT_SERIALIZE(Cl::CQ_TierRecord);
DEFAULT_SERIALIZE(Sv::SN_Unknown_62472);
DEFAULT_SERIALIZE(Sv::SN_UpdateEntrySystem);

#undef DEFAULT_SERIALIZE


/*
template<>
inline const char* PacketSerialize<Sv::SN_LoadCharacterStart>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("Sv::SN_LoadCharacterStart(%d, %d) :: {", Sv::SN_LoadCharacterStart::NET_ID, packetSize);
	SER("}");

	return str.data();
}
*/

#undef SER_BEGIN
#undef SER
