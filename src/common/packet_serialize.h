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
#define SER(fmt, ...) str.append_sprintf(fmt "\n", ##__VA_ARGS__)

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
inline const char* PacketSerialize<Cl::CQ_UserLogin>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("CQ_UserLogin(%d, %d) :: {", Cl::CQ_UserLogin::NET_ID, packetSize);
	SER("	nick='%ls'", buff.ReadWideStringObj().data());
	SER("	password='%ls'", buff.ReadWideStringObj().data());
	SER("	type='%ls'", buff.ReadWideStringObj().data());
	SER("	extra='%ls'", buff.ReadWideStringObj().data());
	SER("	unknown=%u", buff.Read<u32>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SA_UserloginResult>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SA_UserloginResult(%d, %d) :: {", Sv::SA_UserloginResult::NET_ID, packetSize);
	SER("	result=%%d", buff.Read<i32>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SA_EnterWaitingQueue>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SA_EnterWaitingQueue(%d, %d) :: {", Sv::SA_EnterWaitingQueue::NET_ID, packetSize);
	SER("	var1=%%u", buff.Read<u8>());
	SER("	var2=%%u", buff.Read<u32>());
	SER("	var3=%%u", buff.Read<u32>());
	SER("	var4=%%u", buff.Read<u32>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SA_ServerVersionInfo>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SA_ServerVersionInfo(%d, %d) :: {", Sv::SA_ServerVersionInfo::NET_ID, packetSize);
	SER("	versionName='%%ls'", buff.ReadWideStringObj().data());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_TgchatServerInfo>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_TgchatServerInfo(%d, %d) :: {", Sv::SN_TgchatServerInfo::NET_ID, packetSize);
	const u16 hostLen = buff.Read<u16>();
	SER("	host='%%.*s'", hostLen, buff.ReadRaw(hostLen));
	SER("	port=%%u", buff.Read<u16>());
	SER("	gameID=%%d", buff.Read<i32>());
	SER("	serverID=%%d", buff.Read<i32>());
	SER("	userID=%%u", buff.Read<u32>());
	SER("	gamename='%%ls'", buff.ReadWideStringObj().data());
	SER("	chatname='%%ls'", buff.ReadWideStringObj().data());
	SER("	playncname='%%ls'", buff.ReadWideStringObj().data());
	const u16 sigLen = buff.Read<u16>();
	SER("	signature_len=%%u", sigLen);
	buff.ReadRaw(sigLen);
	SER("	serverType=%%u", buff.Read<u8>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_StationList>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_StationList(%d, %d) :: {", Sv::SN_StationList::NET_ID, packetSize);
	const u16 count = buff.Read<u16>();
	SER("	stationList(%%u)=[", count);
	for(int i = 0; i < count; i++) {
		const u32 idc = buff.Read<u32>();
		const u16 sc = buff.Read<u16>();
		SER("		{ idc=%%u stations(%%u)=[", idc, sc);
		for(int j = 0; j < sc; j++) {
			SER("			{ gameServerIp=(%%u.%%u.%%u.%%u) pingServerIp=(%%u.%%u.%%u.%%u) port=%%u }",
				buff.Read<u8>(), buff.Read<u8>(), buff.Read<u8>(), buff.Read<u8>(),
				buff.Read<u8>(), buff.Read<u8>(), buff.Read<u8>(), buff.Read<u8>(),
				buff.Read<u16>());
		}
		SER("		] }");
	}
	SER("	]");
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_DoConnectChannelServer>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_DoConnectChannelServer(%d, %d) :: {", Sv::SN_DoConnectChannelServer::NET_ID, packetSize);
	const u16 count = buff.Read<u16>();
	SER("	addresses(%%u)=[", count);
	for(int i = 0; i < count; i++) {
		const u32 ip = buff.Read<u32>();
		const u16 port = buff.Read<u16>();
		const u16 ispLen = buff.Read<u16>();
		SER("		{ ip=%%u port=%%u isp='%%.*ls' }", ip, port, ispLen, (wchar*)buff.ReadRaw(ispLen * sizeof(wchar)));
	}
	SER("	]");
	SER("	nick='%%ls'", buff.ReadWideStringObj().data());
	SER("	instantKey=%%u", buff.Read<u32>());
	SER("	reasonCode=%%u", buff.Read<u32>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_ServerVersionInfo>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	SER("CQ_ServerVersionInfo(%d, %d) :: {}", Cl::CQ_ServerVersionInfo::NET_ID, packetSize);
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_SetIspName>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("CQ_SetIspName(%d, %d) :: {", Cl::CQ_SetIspName::NET_ID, packetSize);
	SER("	w1='%ls'", buff.ReadWideStringObj().data());
	SER("	w2='%ls'", buff.ReadWideStringObj().data());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_EnterWaitingQueue>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("CQ_EnterWaitingQueue(%d, %d) :: {", Cl::CQ_EnterWaitingQueue::NET_ID, packetSize);
	SER("	var1=%d", buff.Read<i32>());
	SER("	var2=%u", buff.Read<u32>());
	const u16 c1 = buff.Read<u16>();
	SER("	latencies(%u)=[", c1);
	for(int i = 0; i < c1; i++) SER("		{ ip=%u rtt=%u }", buff.Read<u32>(), buff.Read<u16>());
	SER("	]");
	const u16 c2 = buff.Read<u16>();
	SER("	extras(%u)=[", c2);
	for(int i = 0; i < c2; i++) SER("		{ a=%u b=%u c=%u d=%u }", buff.Read<u32>(), buff.Read<u16>(), buff.Read<u16>(), buff.Read<u16>());
	SER("	]");
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_AuthenticateGameServer>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("CQ_AuthenticateGameServer(%d, %d) :: {", Cl::CQ_AuthenticateGameServer::NET_ID, packetSize);
	SER("	nick='%ls'", buff.ReadWideStringObj().data());
	SER("	instantKey=%u", buff.Read<u32>());
	SER("	var1=%u", buff.Read<u32>());
	SER("	b1=%d", buff.Read<u8>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_PlayerCastSkill>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("CQ_PlayerCastSkill(%d, %d) :: {", Cl::CQ_PlayerCastSkill::NET_ID, packetSize);
	SER("	playerID=0x%08x", buff.Read<LocalActorID>());
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
	SER("		clientTime=%g",buff.Read<f32>());
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
	SER("	port=%u", buff.Read<u16>());
	SER("	ip=(%u.%u.%u.%u)", buff.Read<u8>(), buff.Read<u8>(), buff.Read<u8>(), buff.Read<u8>());
	SER("	gameID=%d", buff.Read<i32>());
	SER("	idcHash=%u", buff.Read<u32>());
	SER("	nickname='%ls'", buff.ReadWideStringObj().data());
	SER("	instantKey=%u", buff.Read<u32>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_GameCreateActor>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_GameCreateActor(%d, %d) :: {", Sv::SN_GameCreateActor::NET_ID, packetSize);
	SER("	objectID=0x%08x", buff.Read<LocalActorID>());
	SER("	nType=%d", buff.Read<i32>());
	SER("	nIDX=%d", buff.Read<CreatureIndex>());
	SER("	dwLocalID=%d", buff.Read<i32>());
	SER("	p3nPos=%s", PS::ToStr(buff.Read<float3>()));
	SER("	p3nDir=%s", PS::ToStr(buff.Read<float3>()));
	SER("	spawnType=%d", buff.Read<i32>());
	SER("	actionState=%d", buff.Read<i32>());
	SER("	ownerID=0x%08x", buff.Read<i32>());
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
	SER("	objectID=0x%08x", buff.Read<LocalActorID>());
	SER("	mainEntityID=0x%08x", buff.Read<LocalActorID>());
	SER("	nType=%d", buff.Read<i32>());
	SER("	nIDX=%d", buff.Read<CreatureIndex>());
	SER("	dwLocalID=%d", buff.Read<i32>());
	SER("	p3nPos=%s", PS::ToStr(buff.Read<float3>()));
	SER("	p3nDir=%s", PS::ToStr(buff.Read<float3>()));
	SER("	spawnType=%d", buff.Read<i32>());
	SER("	actionState=%d", buff.Read<i32>());
	SER("	ownerID=0x%08x", buff.Read<i32>());
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

	SER("	objectID=0x%08x",  buff.Read<i32>());
	SER("	p3nPos=%s",  PS::ToStr(buff.Read<float3>()));
	SER("	p3nDir=%s",  PS::ToStr(buff.Read<float3>()));
	SER("	p2nMoveDir=%s",  PS::ToStr(buff.Read<float2>()));
	SER("	p2nUpperDir=%s",  PS::ToStr(buff.Read<float2>()));
	SER("	p3nMoveTargetPos=%s",  PS::ToStr(buff.Read<float3>()));
	SER("	isBattleState=%d",  buff.Read<u8>());
	SER("	baseMoveSpeed=%f",  buff.Read<f32>());
	SER("	actionState=%d",  buff.Read<i32>());
	SER("	aiTargetID=%d",  buff.Read<i32>());

	SER("	statSnapshot={");

		u16 count = buff.Read<u16>();
		SER("		curStats(%d)=[",  count);
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
	SER("	objectID=0x%08x", buff.Read<LocalActorID>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_StatusSnapshot>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_StatusSnapshot(%d, %d) :: {", Sv::SN_StatusSnapshot::NET_ID, packetSize);
	SER("	objectID=0x%08x", buff.Read<LocalActorID>());

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
	SER("	entity=0x%08x", buff.Read<LocalActorID>());
	SER("	ret=%u", buff.Read<u32>());
	SER("	skillIndex=%d", buff.Read<SkillID>());
	SER("	costLevel=%d", buff.Read<i8>());
	SER("	actionState=%d", buff.Read<ActionStateID>());
	SER("	targetPos=%s", PS::ToStr(buff.Read<float3>()));

	const u16 count = buff.Read<u16>();
	SER("	targetIds(%u)=[", count);
	for(int i = 0; i < count; i++) {
		SER("		0x%08x,", buff.Read<LocalActorID>());
	}
	SER("	]");

	SER("	bSyncMyPosition=%u", buff.Read<u8>());
	SER("	posStruct={");
	SER("		pos=%s", PS::ToStr(buff.Read<float3>()));
	SER("		destPos=%s", PS::ToStr(buff.Read<float3>()));
	SER("		moveDir=%s", PS::ToStr(buff.Read<float2>()));
	SER("		rotateStruct=%s", PS::ToStr(buff.Read<float3>()));
	SER("		speed=%g",buff.Read<f32>());
	SER("		clientTime=%g",buff.Read<f32>());
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
	SER("	entity=0x%08x", buff.Read<LocalActorID>());
	SER("	ret=%u", buff.Read<u32>());
	SER("	skillIndex=%d", buff.Read<SkillID>());
	SER("	costLevel=%d", buff.Read<i8>());
	SER("	actionState=%d", buff.Read<ActionStateID>());
	SER("	targetPos=%s", PS::ToStr(buff.Read<float3>()));

	const u16 count = buff.Read<u16>();
	SER("	targetIds(%u)=[", count);
	for(int i = 0; i < count; i++) {
		SER("		0x%08x,", buff.Read<LocalActorID>());
	}
	SER("	]");

	SER("	bSyncMyPosition=%u", buff.Read<u8>());
	SER("	posStruct={");
	SER("		pos=%s", PS::ToStr(buff.Read<float3>()));
	SER("		destPos=%s", PS::ToStr(buff.Read<float3>()));
	SER("		moveDir=%s", PS::ToStr(buff.Read<float2>()));
	SER("		rotateStruct=%s", PS::ToStr(buff.Read<float3>()));
	SER("		speed=%g",buff.Read<f32>());
	SER("		clientTime=%g",buff.Read<f32>());
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
	SER("	entity=0x%08x", (u32)cast.entity);
	SER("	ret=%u", cast.ret);
	SER("	skillIndex=%d", cast.skillIndex);
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_PlayerSkillSlot>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_PlayerSkillSlot(%d, %d) :: {", Sv::SN_PlayerSkillSlot::NET_ID, packetSize);
	SER("	characterID=0x%08x", buff.Read<LocalActorID>());

	const u16 slotList_count = buff.Read<u16>();
	SER("	slotList(%d)=[", slotList_count);

	for(int i = 0; i < slotList_count; i++) {
		SER("	{");
		SER("		nSkillIndex=%d", buff.Read<SkillID>());
		SER("		nCoolTime=%u", buff.Read<u32>());
		SER("		bUnlocked=%u", buff.Read<u8>());
		const u16 propListCount = buff.Read<u16>();
		SER("		properties(%u)=[", propListCount);
		for(u16 j = 0; j < propListCount; j++) {
			SER("		{");
			SER("			m_skillPropertyIndex=%u", buff.Read<u32>());
			SER("			m_level=%u", buff.Read<u32>());
			SER("		},");
		}
		SER("		]");
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
	SER("	InGameID=%d", buff.Read<i32>());
	SER("	GameType=%d", buff.Read<i32>());
	SER("	AreaIndex=%d", buff.Read<i32>());
	SER("	StageIndex=%d", buff.Read<i32>());
	SER("	GameDefinitionType=%d", buff.Read<i32>());
	SER("	initPlayerCount=%u", buff.Read<u8>());
	SER("	CanEscape=%u", buff.Read<u8>());
	SER("	IsTrespass=%u", buff.Read<u8>());
	SER("	IsSpectator=%u", buff.Read<u8>());

	const u16 ingameUsers_len = buff.Read<u16>();
	SER("	IngameUsers(%d)=[", ingameUsers_len);
	for(int i = 0; i < ingameUsers_len; i++) {
		SER("	{");
		SER("		userId=0x%08x", buff.Read<i32>());
		SER("		nickname='%S'", buff.ReadWideStringObj().data());
		SER("		team=%u", buff.Read<u8>());
		SER("		isBot=%u", buff.Read<u8>());
		SER("	},");
	}
	SER("	]");

	const u16 ingamePlayers_len = buff.Read<u16>();
	SER("	IngamePlayers(%d)=[", ingamePlayers_len);
	for(int i = 0; i < ingamePlayers_len; i++) {
		SER("	{");
		SER("		userId=0x%08x", buff.Read<i32>());
		SER("		mainCreatureIndex=%d", buff.Read<CreatureIndex>());
		SER("		mainSkinIndex=%d", buff.Read<SkinIndex>());
		SER("		mainSkillIndex1=%d", buff.Read<SkillID>());
		SER("		mainSkillIndex2=%d", buff.Read<SkillID>());
		SER("		subCreatureIndex=%d", buff.Read<CreatureIndex>());
		SER("		subSkinIndex=%d", buff.Read<SkinIndex>());
		SER("		subSkillIndex1=%d", buff.Read<SkillID>());
		SER("		subSkillIndex2=%d", buff.Read<SkillID>());
		SER("		stageSkillIndex1=%d", buff.Read<SkillID>());
		SER("		stageSkillIndex2=%d", buff.Read<SkillID>());
		SER("		supportKitIndex=%d", buff.Read<i32>());
		SER("		isBot=%u", buff.Read<u8>());
		SER("	},");
	}
	SER("	]");

	const u16 ingameGuilds_len = buff.Read<u16>();
	SER("	IngameGuilds(%d)=[", ingameGuilds_len);
	for(int i = 0; i < ingameGuilds_len; i++) {
		SER("	{");
		SER("		teamType=%u", buff.Read<u8>());
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
	SER("	characterID=0x%08x", buff.Read<LocalActorID>());
	SER("	weaponDocIndex=%d", buff.Read<WeaponIndex>());
	SER("	additionalOverheatGauge=%g", buff.Read<f32>());
	SER("	additionalOverheatGaugeRatio=%g", buff.Read<f32>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_GamePlayerStock>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_GamePlayerStock(%d, %d) :: {", Sv::SN_GamePlayerStock::NET_ID, packetSize);
	SER("	playerID=0x%08x", buff.Read<LocalActorID>());
	SER("	wStrPlayerName='%ls'", buff.ReadWideStringObj().data());
	SER("	m_Class=%u", buff.Read<u32>());
	SER("	m_DisplayTitleIDX=%u", buff.Read<u32>());
	SER("	m_StatTitleIDX=%u", buff.Read<u32>());
	SER("	m_badgeType=%u", buff.Read<u8>());
	SER("	m_badgeTierLevel=%u", buff.Read<u8>());
	SER("	m_guildTag='%ls'", buff.ReadWideStringObj().data());
	SER("	m_vipLevel=%d", buff.Read<i8>());
	SER("	m_staffType=%d", buff.Read<i8>());
	SER("	m_isSubstituted=%u", buff.Read<u8>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_SelectSkin>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CQ_SelectSkin(%d, %d) :: {", Cl::CQ_SelectSkin::NET_ID, packetSize);
	SER("	classType=%u", buff.Read<u32>());
	SER("	skinIndex=%u", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SA_CharacterSkinSelect>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SA_CharacterSkinSelect(%d, %d) :: {", Sv::SA_CharacterSkinSelect::NET_ID, packetSize);
	SER("	result=0x%x", buff.Read<u32>());
	SER("	classType=%u", buff.Read<u32>());
	SER("	skinIndex=%u", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_MailShowList>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CQ_MailShowList(%d, %d) :: {", Cl::CQ_MailShowList::NET_ID, packetSize);
	SER("	mailboxType=%u", buff.Read<u8>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_RequestMissionReward>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CQ_RequestMissionReward(%d, %d) :: {", Cl::CQ_RequestMissionReward::NET_ID, packetSize);
	SER("	missionIndex=%u", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_DailyMissionChange>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CQ_DailyMissionChange(%d, %d) :: {", Cl::CQ_DailyMissionChange::NET_ID, packetSize);
	SER("	step=%u", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_MailRead>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CQ_MailRead(%d, %d) :: {", Cl::CQ_MailRead::NET_ID, packetSize);
	SER("	mailId=%llu", buff.Read<u64>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_MailWrite>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CQ_MailWrite(%d, %d) :: {", Cl::CQ_MailWrite::NET_ID, packetSize);
	SER("	to='%ls'", buff.ReadWideStringObj().data());
	SER("	subject='%ls'", buff.ReadWideStringObj().data());
	SER("	content='%ls'", buff.ReadWideStringObj().data());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_MailMove>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CQ_MailMove(%d, %d) :: {", Cl::CQ_MailMove::NET_ID, packetSize);
	SER("	moveType=%u", buff.Read<u8>());
	const u16 count = buff.Read<u16>();
	SER("	mailUIDs_count=%u", count);
	for(u16 i = 0; i < count; i++) SER("	mailUIDs[%u]=%llu", i, buff.Read<u64>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_MailUnreadNotice>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_MailUnreadNotice(%d, %d) :: {", Sv::SN_MailUnreadNotice::NET_ID, packetSize);
	SER("	unreadInboxMailCount=%u", buff.Read<u16>());
	SER("	unreadArchivedMailCount=%u", buff.Read<u16>());
	SER("	unreadShopMailCount=%u", buff.Read<u16>());
	SER("	inboxMailCount=%u", buff.Read<u16>());
	SER("	archivedMailCount=%u", buff.Read<u16>());
	SER("	shopMailCount=%u", buff.Read<u16>());
	SER("	newAttachmentsPending_count=%u", buff.Read<u16>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_MailList>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_MailList(%d, %d) :: {", Sv::SN_MailList::NET_ID, packetSize);
	SER("	packetNum=%u", buff.Read<u8>());
	SER("	mailboxType=%u", buff.Read<u8>());
	const u16 count = buff.Read<u16>();
	SER("	mailList_count=%u", count);
	for(u16 i = 0; i < count; i++) {
		const i64 id = buff.Read<i64>();
		const i64 sent = buff.Read<i64>();
		const i64 expire = buff.Read<i64>();
		const u8 isRead = buff.Read<u8>();
		const u8 isGM = buff.Read<u8>();
		const u8 hasAttachment = buff.Read<u8>();
		SER("	mailList[%u]=(mailId=%lld send=%lld expire=%lld isRead=%u isGMMail=%u hasAttachment=%u", i, id, sent, expire, isRead, isGM, hasAttachment);
		SER("		from='%ls'", buff.ReadWideStringObj().data());
		SER("		subject='%ls'", buff.ReadWideStringObj().data());
		const u16 items = buff.Read<u16>();
		for(u16 k = 0; k < items; k++) SER("		items[%u]=%u", k, buff.Read<u32>());
	}
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_MailRead>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_MailRead(%d, %d) :: {", Sv::SN_MailRead::NET_ID, packetSize);
	SER("	mailId=%lld", buff.Read<i64>());
	SER("	isRead=%u", buff.Read<u8>());
	SER("	isGMMail=%u", buff.Read<u8>());
	SER("	fromNickname='%ls'", buff.ReadWideStringObj().data());
	SER("	subject='%ls'", buff.ReadWideStringObj().data());
	SER("	content='%ls'", buff.ReadWideStringObj().data());
	SER("	sendUTCDate=%lld", buff.Read<i64>());
	SER("	expireUTCDate=%lld", buff.Read<i64>());
	SER("	money=%lld", buff.Read<i64>());
	SER("	exp=%lld", buff.Read<i64>());
	SER("	ccoin=%lld", buff.Read<i64>());
	SER("	guildFund=%u", buff.Read<u32>());
	SER("	itemList_count=%u", buff.Read<u16>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_MailListNewInbox>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_MailListNewInbox(%d, %d) :: {", Sv::SN_MailListNewInbox::NET_ID, packetSize);
	SER("	mailboxType=%u", buff.Read<u8>());
	const u16 count = buff.Read<u16>();
	SER("	mailList_count=%u", count);
	for(u16 i = 0; i < count; i++) {
		SER("	mailList[%u].mailId=%lld", i, buff.Read<i64>());
		buff.Read<i64>();
		buff.Read<i64>();
		buff.Read<u8>();
		buff.Read<u8>();
		buff.Read<u8>();
		SER("		from='%ls'", buff.ReadWideStringObj().data());
		SER("		subject='%ls'", buff.ReadWideStringObj().data());
		const u16 items = buff.Read<u16>();
		for(u16 k = 0; k < items; k++) buff.Read<u32>();
	}
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_MailWriteResult>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_MailWriteResult(%d, %d) :: {", Sv::SN_MailWriteResult::NET_ID, packetSize);
	SER("	nErrorType=0x%x", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_MailMoveResult>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_MailMoveResult(%d, %d) :: {", Sv::SN_MailMoveResult::NET_ID, packetSize);
	SER("	nErrorType=0x%x", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SA_RequestMissionReward>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SA_RequestMissionReward(%d, %d) :: {", Sv::SA_RequestMissionReward::NET_ID, packetSize);
	SER("	result=%u", buff.Read<u32>());
	const u32 index = buff.Read<u32>();
	const i64 progress = buff.Read<i64>();
	const u16 changed = buff.Read<u16>();
	const u8 cleared = buff.Read<u8>();
	const u8 rewarded = buff.Read<u8>();
	SER("	mission=(index=%u progressCount=%lld changedCount=%u isCleared=%u isRewarded=%u)", index, progress, changed, cleared, rewarded);
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SA_DailyMissionChange>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SA_DailyMissionChange(%d, %d) :: {", Sv::SA_DailyMissionChange::NET_ID, packetSize);
	SER("	result=%u", buff.Read<u32>());
	SER("	step=%u", buff.Read<u32>());
	const u32 index = buff.Read<u32>();
	const i64 progress = buff.Read<i64>();
	const u16 changed = buff.Read<u16>();
	const u8 cleared = buff.Read<u8>();
	const u8 rewarded = buff.Read<u8>();
	SER("	mission=(index=%u progressCount=%lld changedCount=%u isCleared=%u isRewarded=%u)", index, progress, changed, cleared, rewarded);
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_ExpGain>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_ExpGain(%d, %d) :: {", Sv::SN_ExpGain::NET_ID, packetSize);
	SER("	amount=%u", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_MissionList>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_MissionList(%d, %d) :: {", Sv::SN_MissionList::NET_ID, packetSize);
	const u16 count = buff.Read<u16>();
	SER("	missions_count=%u", count);
	for(u16 i = 0; i < count; i++) {
		const u32 index = buff.Read<u32>();
		const i64 progress = buff.Read<i64>();
		const u16 changed = buff.Read<u16>();
		const u8 cleared = buff.Read<u8>();
		const u8 rewarded = buff.Read<u8>();
		SER("	missions[%u]=(index=%u progressCount=%lld changedCount=%u isCleared=%u isRewarded=%u)", i, index, progress, changed, cleared, rewarded);
	}
	SER("	dailyMissionRemainTimeMS=%u", buff.Read<u32>());
	SER("	weeklyMissionRemainTimeMS=%u", buff.Read<u32>());
	SER("	monthlyMissionRemainTimeMS=%u", buff.Read<u32>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_Exp>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_Exp(%d, %d) :: {", Sv::SN_Exp::NET_ID, packetSize);
	SER("	leaderCSN=0x%08x", buff.Read<u32>());
	SER("	nickname='%ls'", buff.ReadWideStringObj().data());
	SER("	isLevelUp=%u", buff.Read<u8>());
	SER("	level=%u", buff.Read<u16>());
	SER("	currentLevelExp=%u", buff.Read<u32>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_GamePlayerTag>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	const Sv::SN_GamePlayerTag& packet = *(Sv::SN_GamePlayerTag*)packetData;

	SER("SN_GamePlayerTag(%d, %d) :: {", Sv::SN_GamePlayerTag::NET_ID, packetSize);
	SER("	result=%u", packet.result);
	SER("	mainID=0x%08x", packet.mainID);
	SER("	subID=0x%08x", packet.subID);
	SER("	attackerID=0x%08x", packet.attackerID);
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
		SER("		characterID=0x%08x", chara.characterID);
		SER("		creatureIndex=%d", chara.creatureIndex);
		SER("		skillSlot1=%d", chara.skillSlot1);
		SER("		skillSlot2=%d", chara.skillSlot2);
		SER("		class=%d", chara.classType);
		SER("		x=%f", chara.x);
		SER("		y=%f", chara.y);
		SER("		z=%f", chara.z);
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
inline const char* PacketSerialize<Sv::SN_ProfileItems>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_ProfileItems(%d, %d) :: {", Sv::SN_ProfileItems::NET_ID, packetSize);
	SER("	packetNum=%d", buff.Read<u8>());

	const u16 count = buff.Read<u16>();
	SER("	items(%d)=[", count);

	for(int i = 0; i < count; i++) {
		SER("	{");
		SER("		itemID=0x%08x", buff.Read<i32>());
		SER("		invenType=%u", buff.Read<u8>());
		SER("		slot=%d", buff.Read<i32>());
		SER("		itemIndex=0x%08x", buff.Read<i32>());
		SER("		count=%d", buff.Read<i32>());
		SER("		propertyGroupIndex=%d", buff.Read<i32>());
		SER("		isLifeTimeAbsolute=%u", buff.Read<u8>());
		SER("		lifeEndTimeUTC=%lld", buff.Read<i64>());
		const u16 propCount = buff.Read<u16>();
		SER("		properties(%d)=[", propCount);
		for(int p = 0; p < propCount; p++) {
			SER("		{");
			SER("			type=%u", buff.Read<u8>());
			SER("			typeDetail=%d", buff.Read<i32>());
			SER("			valueType=%u", buff.Read<u8>());
			SER("			value=%f", buff.Read<f32>());
			SER("			fixed=%u", buff.Read<u8>());
			SER("		},");
		}
		SER("		]");
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
		SER("		characterID=0x%08x", weap.characterID);
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
		SER("		characterID=0x%08x", buff.Read<LocalActorID>());
		SER("		skillIndex=%d", buff.Read<SkillID>());
		SER("		isUnlocked=%u", buff.Read<u8>());
		SER("		isActivated=%u", buff.Read<u8>());
		const u16 propCount = buff.Read<u16>();
		SER("		properties(%d)=[", propCount);
		for(int p = 0; p < propCount; p++) {
			SER("		{");
			SER("			skillPropertyIndex=%d", buff.Read<i32>());
			SER("			level=%d", buff.Read<i32>());
			SER("		},");
		}
		SER("		]");
		SER("	},");
	}

	SER("	]");
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_ProfileMasterGears>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_ProfileMasterGears(%d, %d) :: {", Sv::SN_ProfileMasterGears::NET_ID, packetSize);

	const u16 count = buff.Read<u16>();
	SER("	masterGears(%d)=[", count);

	for(int i = 0; i < count; i++) {
		SER("	{");
		SER("		masterGearNo=%u", buff.Read<u8>());
		SER("		name='%S'", buff.ReadWideStringObj().data());
		const u16 slotCount = buff.Read<u16>();
		SER("		slots(%d)=[", slotCount);
		for(int s = 0; s < slotCount; s++) {
			SER("		{");
			SER("			gearType=%d", buff.Read<i32>());
			SER("			gearItemID=0x%08x", buff.Read<ItemUID>());
			SER("		},");
		}
		SER("		]");
		SER("	},");
	}

	SER("	]");
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_AccountEquipmentList>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_AccountEquipmentList(%d, %d) :: {", Sv::SN_AccountEquipmentList::NET_ID, packetSize);
	SER("	supportKitDocIndex=%d", buff.Read<i32>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_Money>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_Money(%d, %d) :: {", Sv::SN_Money::NET_ID, packetSize);
	SER("	nMoney=%lld", buff.Read<i64>());
	SER("	nReason=%d", buff.Read<i32>());
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
		SER("		userId=0x%08x", buff.Read<UserID>());
		Sv::SN_NotifyPcDetailInfos::ST_PcInfo mainPC = buff.Read<Sv::SN_NotifyPcDetailInfos::ST_PcInfo>();
		Sv::SN_NotifyPcDetailInfos::ST_PcInfo subPC = buff.Read<Sv::SN_NotifyPcDetailInfos::ST_PcInfo>();
		SER("		mainPC=(characterID=0x%08x docID=%d classType=%d hp=%d maxHp=%d)", mainPC.characterID, mainPC.docID, mainPC.classType, mainPC.hp, mainPC.maxHp);
		SER("		subPC=(characterID=0x%08x docID=%d classType=%d hp=%d maxHp=%d)", subPC.characterID, subPC.docID, subPC.classType, subPC.hp, subPC.maxHp);
		SER("		remainTagCooltimeMS=%d", buff.Read<i32>());
		SER("		canCastSkillSlotUG=%u", buff.Read<u8>());
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
	SER("	ownerID=0x%08x", buff.Read<LocalActorID>());
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
	SER("	ScoreBoardUserInfos(%d)=[", count);

	for(int i = 0; i < count; i++) {
		SER("	{");
		SER("		usn=0x%08x", buff.Read<UserID>());
		SER("		nickname='%S'", buff.ReadWideStringObj().data());
		SER("		teamType=%d", buff.Read<TeamType>());
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
	SER("	entityID=0x%08x", buff.Read<LocalActorID>());
	SER("	DestPos=%s", PS::ToStr(buff.Read<float3>()));
	SER("	MoveDir=%s", PS::ToStr(buff.Read<float2>()));
	SER("	UpperDir=%s", PS::ToStr(buff.Read<float2>()));
	SER("	nRotate=%f", buff.Read<f32>());
	SER("	nSpeed=%f", buff.Read<f32>());
	SER("	flags=%u", buff.Read<u8>());
	ActionStateID action = buff.Read<ActionStateID>();
	SER("	actionStateID=%s (%d)", ActionStateToString(action), action);
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

inline void SerializeSkillProperties(eastl::fixed_string<char,8192,true>& str, ConstBuffer& buff)
{
	const u16 count = buff.Read<u16>();
	SER("	properties_count=%u", count);
	for(u16 i = 0; i < count; i++) {
		const u32 prop = buff.Read<u32>();
		SER("	property[%u]={ index=%u level=%u }", i, prop, buff.Read<u32>());
	}
}

template<>
inline const char* PacketSerialize<Sv::SA_SkillUpgradeIngame>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SA_SkillUpgradeIngame(%d, %d) :: {", Sv::SA_SkillUpgradeIngame::NET_ID, packetSize);
	SER("	result=%d", buff.Read<i32>());
	for(i32 m = 0; m < 2; m++) {
		SER("	masterID=0x%08x", buff.Read<u32>());
		SER("	characterID=0x%08x", buff.Read<u32>());
		SER("	skillIndex=%u", buff.Read<u32>());
		SER("	isUnlocked=%u", buff.Read<u8>());
		SER("	isActivated=%u", buff.Read<u8>());
		SerializeSkillProperties(str, buff);
	}
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_PlayerSkillUpdate>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_PlayerSkillUpdate(%d, %d) :: {", Sv::SN_PlayerSkillUpdate::NET_ID, packetSize);
	SER("	characterID=0x%08x", buff.Read<u32>());
	SER("	nSkillIndex=%u", buff.Read<u32>());
	SER("	nCoolTime=%u", buff.Read<u32>());
	SER("	bUnlocked=%u", buff.Read<u8>());
	SerializeSkillProperties(str, buff);
	SER("	isUnlocked=%u", buff.Read<u8>());
	SER("	isActivated=%u", buff.Read<u8>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_NotifyIngameSkillPoint>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();

	const Sv::SN_NotifyIngameSkillPoint& p = *(Sv::SN_NotifyIngameSkillPoint*)packetData;

	SER("SN_NotifyIngameSkillPoint(%d, %d) :: {", Sv::SN_NotifyIngameSkillPoint::NET_ID, packetSize);
	SER("	userId=0x%08x", p.userId);
	SER("	skillPoint=%d", p.skillPoint);
	SER("}");

	return str.data();
}


template<>
inline const char* PacketSerialize<Sv::SA_GameReady>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();

	const Sv::SA_GameReady& p = *(Sv::SA_GameReady*)packetData;

	SER("SA_GameReady(%d, %d) :: {", Sv::SA_GameReady::NET_ID, packetSize);
	SER("	waitingTimeMS=%d", p.waitingTimeMS);
	SER("	serverTimestamp=%lld", p.serverTimestamp);
	SER("	readyElapsedMS=%d", p.readyElapsedMS);
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_NotifyTimestamp>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();

	const Sv::SN_NotifyTimestamp& p = *(Sv::SN_NotifyTimestamp*)packetData;

	SER("SN_NotifyTimestamp(%d, %d) :: {", Sv::SN_NotifyTimestamp::NET_ID, packetSize);
	SER("	serverTimestamp=%lld", p.serverTimestamp);
	SER("	curCount=%d", p.curCount);
	SER("	maxCount=%d", p.maxCount);
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_NotifyAasRestricted>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();

	const Sv::SN_NotifyAasRestricted& p = *(Sv::SN_NotifyAasRestricted*)packetData;

	SER("SN_NotifyAasRestricted(%d, %d) :: {", Sv::SN_NotifyAasRestricted::NET_ID, packetSize);
	SER("	isRestrictedByAAS=%u", p.isRestrictedByAAS);
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SA_GetCharacterInfo>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();

	const Sv::SA_GetCharacterInfo& p = *(Sv::SA_GetCharacterInfo*)packetData;

	SER("SA_GetCharacterInfo(%d, %d) :: {", Sv::SA_GetCharacterInfo::NET_ID, packetSize);
	SER("	characterID=0x%08x", p.characterID);
	SER("	docIndex=%d", p.docIndex);
	SER("	classType=%d", p.classType);
	SER("	hp=%d", p.hp);
	SER("	maxHp=%d", p.maxHp);
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_LeaderCharacter>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();

	const Sv::SN_LeaderCharacter& p = *(Sv::SN_LeaderCharacter*)packetData;

	SER("SN_LeaderCharacter(%d, %d) :: {", Sv::SN_LeaderCharacter::NET_ID, packetSize);
	SER("	leaderID=0x%08x", p.leaderID);
	SER("	skinIndex=%d", p.skinIndex);
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SA_SetLeader>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();

	const Sv::SA_SetLeader& p = *(Sv::SA_SetLeader*)packetData;

	SER("SA_SetLeader(%d, %d) :: {", Sv::SA_SetLeader::NET_ID, packetSize);
	SER("	result=%d", p.result);
	SER("	leaderID=0x%08x", p.leaderID);
	SER("	skinIndex=%d", p.skinIndex);
	SER("}");

	return str.data();
}





template<>
inline const char* PacketSerialize<Sv::SN_SpawnPosForMinimap>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();

	const Sv::SN_SpawnPosForMinimap& p = *(Sv::SN_SpawnPosForMinimap*)packetData;

	SER("SN_SpawnPosForMinimap(%d, %d) :: {", Sv::SN_SpawnPosForMinimap::NET_ID, packetSize);
	SER("	objectID=0x%08x", (u32)p.objectID);
	SER("	p3nPos=%s", PS::ToStr(p.p3nPos));
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_InitIngameModeInfo>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_InitIngameModeInfo(%d, %d) :: {", Sv::SN_InitIngameModeInfo::NET_ID, packetSize);
	SER("	transformationVotingPlayerCoolTimeByVotingFail=%d", buff.Read<i32>());
	SER("	transformationVotingTeamCoolTimeByTransformationEnd=%d", buff.Read<i32>());
	SER("	playerCoolTimeByTransformationEnd=%d", buff.Read<i32>());
	SER("	currentTransformationVotingPlayerCoolTimeByVotingFail=%d", buff.Read<i32>());
	SER("	currentTransformationVotingTeamCoolTimeByTransformationEnd=%d", buff.Read<i32>());
	SER("	currentPlayerCoolTimeByTransformationEnd=%d", buff.Read<i32>());
	SER("	chPropertyResetCoolTime=%d", buff.Read<i32>());
	SER("	transformationPieceCount=%u", buff.Read<u8>());

	const u16 titanDocIndexsCount = buff.Read<u16>();
	SER("	titanDocIndexs(%u)=[", titanDocIndexsCount);
	for(u16 i = 0; i < titanDocIndexsCount; i++) {
		SER("		%d,", buff.Read<i32>());
	}
	SER("	]");
	SER("	nextTitanIndex=%d", buff.Read<i8>());

	const u16 limitExceptionStatCount = buff.Read<u16>();
	SER("	limitExceptionStat(%u)=[", limitExceptionStatCount);
	for(u16 i = 0; i < limitExceptionStatCount; i++) {
		SER("	{");
		SER("		statType=%d", buff.Read<i32>());
		SER("		min=%g", buff.Read<f32>());
		SER("		max=%g", buff.Read<f32>());
		SER("		incRatioMin=%g", buff.Read<f32>());
		SER("		incRatioMax=%g", buff.Read<f32>());
		SER("	},");
	}
	SER("	]");
	SER("}");

	return str.data();
}




template<>
inline const char* PacketSerialize<Sv::SA_ResultSpAction>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SA_ResultSpAction(%d, %d) :: {", Sv::SA_ResultSpAction::NET_ID, packetSize);
	const u8 excludedFieldBits = buff.Read<u8>();
	SER("	excludedFieldBits=%u", excludedFieldBits);
	SER("	actionID=%d", buff.Read<i32>());
	SER("	objectID=0x%08x", buff.Read<LocalActorID>());
	SER("	rotate=%f", buff.Read<f32>());
	SER("	moveDir=%s", PS::ToStr(buff.Read<float2>()));
	SER("	errorType=%d", buff.Read<i32>());
	if(!(excludedFieldBits & 0x20)) {
		SER("	startPos=%s", PS::ToStr(buff.Read<float3>()));
	}
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
	SER("	ownerUserID=0x%08x", buff.Read<i32>());
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

	SER("SA_FirstHello(%d, %d) :: {", Sv::SA_FirstHello::NET_ID, packetSize);
	SER("	dwProtocolCRC=%u", packet.dwProtocolCRC);
	SER("	dwErrorCRC=%u", packet.dwErrorCRC);
	SER("	serverType=%u", (u32)packet.serverType);
	SER("	clientIp=(%u.%u.%u.%u)", packet.clientIp[0], packet.clientIp[1], packet.clientIp[2], packet.clientIp[3]);
	SER("	clientPort=%u", packet.clientPort);
	SER("	tqosWorldId=%u", packet.tqosWorldId);
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SA_AuthResult>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SA_AuthResult(%d, %d) :: {", Sv::SA_AuthResult::NET_ID, packetSize);
	SER("	nResult=%d", buff.Read<i32>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_AccountInfo>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_AccountInfo(%d, %d) :: {", Sv::SN_AccountInfo::NET_ID, packetSize);
	SER("	Nickname='%S'", buff.ReadWideStringObj().data());
	SER("	inventoryLineCountTab0=%d", buff.Read<i32>());
	SER("	inventoryLineCountTab1=%d", buff.Read<i32>());
	SER("	inventoryLineCountTab2=%d", buff.Read<i32>());
	SER("	displayTitleIndex=%d", buff.Read<i32>());
	SER("	statTitleIndex=%d", buff.Read<i32>());
	SER("	warehouseLineCount=%d", buff.Read<i32>());
	SER("	tutorialState=%d", buff.Read<i32>());
	SER("	masterGearDurability=%d", buff.Read<i32>());
	SER("	badgeType=%u", buff.Read<u8>());
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
inline const char* PacketSerialize<Cl::CQ_WhisperSend>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("CQ_WhisperSend(%d, %d) :: {", Cl::CQ_WhisperSend::NET_ID, packetSize);
	SER("	destNick='%ls'", buff.ReadWideStringObj().data());
	SER("	msg='%ls'", buff.ReadWideStringObj().data());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CN_ChannelChatMessage>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("CN_ChannelChatMessage(%d, %d) :: {", Cl::CN_ChannelChatMessage::NET_ID, packetSize);
	SER("	chatType=%d", buff.Read<i32>());
	SER("	msg='%ls'", buff.ReadWideStringObj().data());
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
	const u16 count = buff.Read<u16>();
	SER("	alliesSlotInfos(%d)=[", count);
	for(int i = 0; i < count; i++) {
		SER("		{");
		SER("			creatureIndex=%d", buff.Read<CreatureIndex>());
		const u16 slotStates_count = buff.Read<u16>();
		SER("			slotStates(%d)=[", slotStates_count);
		for(int j = 0; j < slotStates_count; j++) {
			SER("				%d,", buff.Read<i32>());
		}
		SER("			]");
		SER("		},");
	}
	SER("	]");
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
inline const char* PacketSerialize<Sv::SA_MasterPick>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SA_MasterPick(%d, %d) :: {", Sv::SA_MasterPick::NET_ID, packetSize);
	SER("	retval=%d", buff.Read<i32>());
	SER("	localMasterID=0x%08x", buff.Read<i32>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_MasterPick>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_MasterPick(%d, %d) :: {", Sv::SN_MasterPick::NET_ID, packetSize);
	SER("	userID=0x%08x", buff.Read<UserID>());
	const u16 count = buff.Read<u16>();
	SER("	characterSelectInfos(%d)=[", count);
	for(int i = 0; i < count; i++) {
		SER("		{");
		SER("			localMasterID=0x%08x", buff.Read<LocalActorID>());
		SER("			creatureIndex=%d", buff.Read<CreatureIndex>());
		SER("			skillSlot1=%d", buff.Read<SkillID>());
		SER("			skillSlot2=%d", buff.Read<SkillID>());
		SER("		},");
	}
	SER("	]");
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_SortieCharacterSlotInfo>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_SortieCharacterSlotInfo(%d, %d) :: {", Sv::SN_SortieCharacterSlotInfo::NET_ID, packetSize);
	const u16 count = buff.Read<u16>();
	SER("	slotInfos(%d)=[", count);
	for(int i = 0; i < count; i++) {
		SER("		{");
		SER("			creatureIndex=%d", buff.Read<CreatureIndex>());
		const u16 slotStates_count = buff.Read<u16>();
		SER("			slotStates(%d)=[", slotStates_count);
		for(int j = 0; j < slotStates_count; j++) {
			SER("				%d,", buff.Read<i32>());
		}
		SER("			]");
		SER("		},");
	}
	SER("	]");
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_ReadySortieRoom>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_ReadySortieRoom(%d, %d) :: {", Sv::SN_ReadySortieRoom::NET_ID, packetSize);
	SER("	userID=0x%08x", buff.Read<UserID>());
	SER("	ready=%u", buff.Read<u8>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_LoadingProgressData>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_LoadingProgressData(%d, %d) :: {", Sv::SN_LoadingProgressData::NET_ID, packetSize);
	SER("	usn=0x%08x", buff.Read<UserID>());
	SER("	nickname='%ls'", buff.ReadWideStringObj().data());
	SER("	progressData=%u", buff.Read<u8>());
	SER("	activeCreatureIndex=%d", buff.Read<CreatureIndex>());
	SER("	inactiveCreatureIndex=%d", buff.Read<CreatureIndex>());
	SER("	isSpectator=%u", buff.Read<u8>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_StartCountdownSortieRoom>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_StartCountdownSortieRoom(%d, %d) :: {", Sv::SN_StartCountdownSortieRoom::NET_ID, packetSize);
	SER("	stageType=%d", buff.Read<StageType>());
	SER("	timeToWaitSec=%d", buff.Read<i32>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_SortiePrepare>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_SortiePrepare(%d, %d) :: {", Sv::SN_SortiePrepare::NET_ID, packetSize);
	const u16 count = buff.Read<u16>();
	SER("	skinList(%d)=[", count);
	for(int i = 0; i < count; i++) {
		const auto& s = buff.Read<Sv::SN_SortiePrepare::Skin>();
		SER("		{");
		SER("			classType=%d", s.classType);
		SER("			skinIndex=%d", s.skinIndex);
		SER("			bufCount=%d", s.bufCount);
		SER("			expireTime=%lld", s.expireTime);
		SER("		},");
	}
	SER("	]");

	const u16 count1 = buff.Read<u16>();
	SER("	skillList(%d)=[", count1);
	for(int i = 0; i < count1; i++) {
		SER("		%d,", buff.Read<SkillID>());
	}
	SER("	]");
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_SortiePrepareBotInfo>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_SortiePrepareBotInfo(%d, %d) :: {", Sv::SN_SortiePrepareBotInfo::NET_ID, packetSize);
	const u16 count1 = buff.Read<u16>();
	SER("	botIndexes(%d)=[", count1);
	for(int i = 0; i < count1; i++) {
		SER("		%d,", buff.Read<i32>());
	}
	SER("	]");
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_SortieMasterPickPhaseStep>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_SortieMasterPickPhaseStep(%d, %d) :: {", Sv::SN_SortieMasterPickPhaseStep::NET_ID, packetSize);
	SER("	isRandomPick=%u", buff.Read<u8>());
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_SortieMasterPickPhaseEnd>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);

	SER("SN_SortieMasterPickPhaseEnd(%d, %d) :: {}", Sv::SN_SortieMasterPickPhaseEnd::NET_ID, packetSize);

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_RunClientLevelEvent>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();

	const Sv::SN_RunClientLevelEvent& packet = *(Sv::SN_RunClientLevelEvent*)packetData;

	SER("SN_RunClientLevelEvent(%d, %d) :: {", Sv::SN_RunClientLevelEvent::NET_ID, packetSize);
	SER("	eventID=%d", packet.eventID);
	SER("	caller=%u", packet.caller);
	SER("	serverTime=%lld", packet.serverTime);
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_UpdateGameOwner>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();

	const Sv::SN_UpdateGameOwner& packet = *(Sv::SN_UpdateGameOwner*)packetData;

	SER("SN_UpdateGameOwner(%d, %d) :: {", Sv::SN_UpdateGameOwner::NET_ID, packetSize);
	SER("	userId=0x%08x", packet.userId);
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_LobbyStartGame>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();

	const Sv::SN_LobbyStartGame& packet = *(Sv::SN_LobbyStartGame*)packetData;

	SER("SN_LobbyStartGame(%d, %d) :: {", Sv::SN_LobbyStartGame::NET_ID, packetSize);
	SER("	stageType=%d", packet.stageType);
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_CityMapInfo>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();

	const Sv::SN_CityMapInfo& packet = *(Sv::SN_CityMapInfo*)packetData;

	SER("SN_CityMapInfo(%d, %d) :: {", Sv::SN_CityMapInfo::NET_ID, packetSize);
	SER("	CityMapID=%d", packet.CityMapID);
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_RunClientLevelEventSeq>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();

	const Sv::SN_RunClientLevelEventSeq& packet = *(Sv::SN_RunClientLevelEventSeq*)packetData;

	SER("SN_RunClientLevelEventSeq(%d, %d) :: {", Sv::SN_RunClientLevelEventSeq::NET_ID, packetSize);
	SER("	needCompleteTriggerAckID=%d", packet.needCompleteTriggerAckID);
	SER("	rootEventID=%d", packet.rootEventID);
	SER("	caller=%u", packet.caller);
	SER("	serverTime=%lld", packet.serverTime);
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_ActionChangeLevelEvent>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();

	ConstBuffer buff(packetData, packetSize);

	SER("SN_ActionChangeLevelEvent(%d, %d) :: {", Sv::SN_ActionChangeLevelEvent::NET_ID, packetSize);
	const u16 count = buff.Read<u16>();
	SER("	targetIDs(%u)=[", count);
	for(u16 i = 0; i < count; i++) {
		SER("		%d,", buff.Read<LocalActorID>());
	}
	SER("	]");
	SER("	actionID=%d", buff.Read<ActionStateID>());
	SER("	serverTime=%lld", buff.Read<i64>());
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
	SER("	partyUID=0x%08x", packet.partyUID);
	SER("	leader=0x%08x", packet.leader);
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
	SER("	partyUID=0x%08x", packet.partyUID);
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<In::HQ_PartyCreate>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	In::HQ_PartyCreate packet = SafeCast<In::HQ_PartyCreate>(packetData, packetSize);

	SER("HQ_PartyCreate(%d, %d) :: {", In::HQ_PartyCreate::NET_ID, packetSize);
	SER("	leader=0x%08x", packet.leader);
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<In::HQ_PartyEnqueue>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	In::HQ_PartyEnqueue packet = SafeCast<In::HQ_PartyEnqueue>(packetData, packetSize);

	SER("HQ_PartyEnqueue(%d, %d) :: {", In::HQ_PartyEnqueue::NET_ID, packetSize);
	SER("	partyUID=0x%08x", packet.partyUID);
	SER("	areaIndex=%d", (i32)packet.areaIndex);
	SER("	stageIndex=%d", (i32)packet.stageIndex);
	SER("	mapIndex=%d", (i32)packet.mapIndex);
	SER("	teamSize=%d", (i32)packet.teamSize);
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<In::MN_MatchingPartyFound>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	const In::MN_MatchingPartyFound& packet = SafeCast<In::MN_MatchingPartyFound>(packetData, packetSize);

	SER("MN_MatchingPartyFound(%d, %d) :: {", In::MN_MatchingPartyFound::NET_ID, packetSize);
	SER("	partyUID=0x%08x", packet.partyUID);
	SER("	sortieUID=%llu", packet.sortieUID);
	SER("	playerList(%d)=[", packet.playerCount);
	for(auto* p = packet.playerList.begin(); p != packet.playerList.begin()+packet.playerCount; ++p) {
		SER("	{");
		SER("		name='%.*ls'", p->name.len, p->name.data);
		SER("		accountUID=0x%08x", p->accountUID);
		SER("		team=%u", p->team);
		SER("		isBot=%u", p->isBot);
		SER("	},");
	}
	SER("	]");
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<In::HN_PlayerRoomFound>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	const In::HN_PlayerRoomFound& packet = SafeCast<In::HN_PlayerRoomFound>(packetData, packetSize);

	SER("HN_PlayerNotifyRoomFound(%d, %d) :: {", In::HN_PlayerRoomFound::NET_ID, packetSize);
	SER("	accountUID=0x%08x", packet.accountUID);
	SER("	sortieUID=%llu", packet.sortieUID);
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<In::HN_PlayerRoomConfirm>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	const In::HN_PlayerRoomConfirm& packet = SafeCast<In::HN_PlayerRoomConfirm>(packetData, packetSize);

	SER("HN_PlayerRoomConfirm(%d, %d) :: {", In::HN_PlayerRoomConfirm::NET_ID, packetSize);
	SER("	accountUID=0x%08x", packet.accountUID);
	SER("	confirm=%u", packet.confirm);
	SER("	sortieUID=%llu", packet.sortieUID);
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<In::MN_RoomCreated>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	const In::MN_RoomCreated& packet = SafeCast<In::MN_RoomCreated>(packetData, packetSize);

	SER("MN_RoomCreated(%d, %d) :: {", In::MN_RoomCreated::NET_ID, packetSize);
	SER("	sortieUID=%llu", packet.sortieUID);
	SER("	playerList(%d)=[", packet.playerCount);
	for(auto* p = packet.playerList.begin(); p != packet.playerList.begin()+packet.playerCount; ++p) {
		SER("	{");
		SER("		name='%.*ls'", p->name.len, p->name.data);
		SER("		accountUID=0x%08x", p->accountUID);
		SER("		team=%u", p->team);
		SER("		isBot=%u", p->isBot);
		SER("	},");
	}
	SER("	]");
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<In::HQ_RoomCreateGame>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	const In::HQ_RoomCreateGame& packet = SafeCast<In::HQ_RoomCreateGame>(packetData, packetSize);

	SER("HQ_RoomCreateGame(%d, %d) :: {", In::HQ_RoomCreateGame::NET_ID, packetSize);
	SER("	sortieUID=%llu", packet.sortieUID);
	SER("	players(%d)=[", packet.playerCount);
	for(auto* p = packet.players.begin(); p != packet.players.begin()+packet.playerCount; ++p) {
		SER("	{");
		SER("		accountUID=0x%08x", p->accountUID);
		SER("		team=%u", p->team);
		SER("		isBot=%u", p->isBot);
		SER("		masters[0]=%d", p->masters[0]);
		SER("		masters[1]=%d", p->masters[1]);
		SER("		skins[0]=%d", p->skins[0]);
		SER("		skins[1]=%d", p->skins[1]);
		SER("		skills[0]=%d", p->skills[0]);
		SER("		skills[1]=%d", p->skills[1]);
		SER("		skills[2]=%d", p->skills[2]);
		SER("		skills[3]=%d", p->skills[3]);
		SER("		weapons[0]=%d", p->weapons[0]);
		SER("		weapons[1]=%d", p->weapons[1]);
		SER("		weaponGrades[0]=%d", p->weaponGrades[0]);
		SER("		weaponGrades[1]=%d", p->weaponGrades[1]);
		SER("		masterGearNo[0]=%u", p->masterGearNo[0]);
		SER("		masterGearNo[1]=%u", p->masterGearNo[1]);
		SER("		characterType[0]=%d", p->characterType[0]);
		SER("		characterType[1]=%d", p->characterType[1]);
		SER("	},");
	}

	SER("	spectators(%d)=[", packet.playerCount);
	for(auto* p = packet.spectators.begin(); p != packet.spectators.begin()+packet.spectatorCount; ++p) {
		SER("	accountUID=0x%08x,", *p);
	}
	SER("	]");
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<In::MQ_CreateGame>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	const In::MQ_CreateGame& packet = SafeCast<In::MQ_CreateGame>(packetData, packetSize);

	SER("MQ_CreateGame(%d, %d) :: {", In::MQ_CreateGame::NET_ID, packetSize);
	SER("	sortieUID=%llu", packet.sortieUID);
	SER("	players(%d)=[", packet.playerCount);
	for(auto* p = packet.players.begin(); p != packet.players.begin()+packet.playerCount; ++p) {
		SER("	{");
		SER("		name='%.*ls'", p->name.len, p->name.data);
		SER("		accountUID=0x%08x", p->accountUID);
		SER("		team=%u", p->team);
		SER("		isBot=%u", p->isBot);
		SER("		masters[0]=%d", p->masters[0]);
		SER("		masters[1]=%d", p->masters[1]);
		SER("		skins[0]=%d", p->skins[0]);
		SER("		skins[1]=%d", p->skins[1]);
		SER("		skills[0]=%d", p->skills[0]);
		SER("		skills[1]=%d", p->skills[1]);
		SER("		skills[2]=%d", p->skills[2]);
		SER("		skills[3]=%d", p->skills[3]);
		SER("		weapons[0]=%d", p->weapons[0]);
		SER("		weapons[1]=%d", p->weapons[1]);
		SER("		weaponGrades[0]=%d", p->weaponGrades[0]);
		SER("		weaponGrades[1]=%d", p->weaponGrades[1]);
		SER("		masterGearNo[0]=%u", p->masterGearNo[0]);
		SER("		masterGearNo[1]=%u", p->masterGearNo[1]);
		SER("		characterType[0]=%d", p->characterType[0]);
		SER("		characterType[1]=%d", p->characterType[1]);
		SER("	},");
	}
	SER("	]");

	SER("	spectators(%d)=[", packet.playerCount);
	for(auto* p = packet.spectators.begin(); p != packet.spectators.begin()+packet.spectatorCount; ++p) {
		SER("	accountUID=0x%08x,", *p);
	}
	SER("	]");
	SER("	gameType=%d", (i32)packet.gameType);
	SER("	areaIndex=%d", (i32)packet.areaIndex);
	SER("	stageIndex=%d", (i32)packet.stageIndex);
	SER("	mapIndex=%d", (i32)packet.mapIndex);
	SER("	canEscape=%u", packet.canEscape);
	SER("	surrenderAbleTime=%d", packet.surrenderAbleTime);
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<In::GR_GameCreated>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	const In::GR_GameCreated& packet = SafeCast<In::GR_GameCreated>(packetData, packetSize);

	SER("GR_GameCreated(%d, %d) :: {", In::GR_GameCreated::NET_ID, packetSize);
	SER("	sortieUID=%llu", packet.sortieUID);
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<In::GN_GameEnded>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	const In::GN_GameEnded& packet = SafeCast<In::GN_GameEnded>(packetData, packetSize);

	SER("GN_GameEnded(%d, %d) :: {", In::GN_GameEnded::NET_ID, packetSize);
	SER("	sortieUID=%llu", packet.sortieUID);
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<In::MN_GameEnded>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	const In::MN_GameEnded& packet = SafeCast<In::MN_GameEnded>(packetData, packetSize);

	SER("MN_GameEnded(%d, %d) :: {", In::MN_GameEnded::NET_ID, packetSize);
	SER("	sortieUID=%llu", packet.sortieUID);
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<In::MN_RoomCancelled>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	const In::MN_RoomCancelled& packet = SafeCast<In::MN_RoomCancelled>(packetData, packetSize);

	SER("MN_RoomCancelled(%d, %d) :: {", In::MN_RoomCancelled::NET_ID, packetSize);
	SER("	sortieUID=%llu", packet.sortieUID);
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<In::MN_MatchCreated>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	const In::MN_MatchCreated& packet = SafeCast<In::MN_MatchCreated>(packetData, packetSize);

	SER("MN_MatchCreated(%d, %d) :: {", In::MN_MatchCreated::NET_ID, packetSize);
	SER("	sortieUID=%llu", packet.sortieUID);
	SER("	serverIp=(%u.%u.%u.%u)", packet.serverIp[0], packet.serverIp[1], packet.serverIp[2], packet.serverIp[3]);
	SER("	serverPort=%u", packet.serverPort);
	SER("}");

	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CA_SetGameGvt>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CA_SetGameGvt(%d, %d) :: {", Cl::CA_SetGameGvt::NET_ID, packetSize);
	SER("	sendTime=%d", buff.Read<i32>());
	SER("	virtualTime=%d", buff.Read<i32>());
	SER("	unk=%d", buff.Read<i32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_GetCharacterInfo>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CQ_GetCharacterInfo(%d, %d) :: {", Cl::CQ_GetCharacterInfo::NET_ID, packetSize);
	SER("	characterID=0x%08x", buff.Read<LocalActorID>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CN_GameUpdatePosition>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CN_GameUpdatePosition(%d, %d) :: {", Cl::CN_GameUpdatePosition::NET_ID, packetSize);
	SER("	characterID=0x%08x", buff.Read<LocalActorID>());
	SER("	p3nPos=%s", PS::ToStr(buff.Read<float3>()));
	SER("	p3nDir=%s", PS::ToStr(buff.Read<float2>()));
	SER("	upperYaw=%g", buff.Read<f32>());
	SER("	upperPitch=%g", buff.Read<f32>());
	SER("	bodyYaw=%g", buff.Read<f32>());
	SER("	nSpeed=%g", buff.Read<f32>());
	SER("	unk1=%u", buff.Read<u8>());
	SER("	actionState=%d", buff.Read<ActionStateID>());
	SER("	localTimeS=%g", buff.Read<f32>());
	SER("	unk2=%u", buff.Read<u8>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CN_GameUpdateRotation>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CN_GameUpdateRotation(%d, %d) :: {", Cl::CN_GameUpdateRotation::NET_ID, packetSize);
	SER("	characterID=0x%08x", buff.Read<LocalActorID>());
	SER("	upperYaw=%g", buff.Read<f32>());
	SER("	upperPitch=%g", buff.Read<f32>());
	SER("	bodyYaw=%g", buff.Read<f32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_SetLeaderCharacter>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CQ_SetLeaderCharacter(%d, %d) :: {", Cl::CQ_SetLeaderCharacter::NET_ID, packetSize);
	SER("	characterID=0x%08x", buff.Read<LocalActorID>());
	SER("	skinIndex=%d", buff.Read<SkinIndex>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_RoomEquipSkill>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CQ_RoomEquipSkill(%d, %d) :: {", Cl::CQ_RoomEquipSkill::NET_ID, packetSize);
	SER("	characterID=0x%08x", buff.Read<LocalActorID>());
	SER("	skillID=%d", buff.Read<SkillID>());
	SER("	skillSlot=%u", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_RoomSwapSkill>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CQ_RoomSwapSkill(%d, %d) :: {", Cl::CQ_RoomSwapSkill::NET_ID, packetSize);
	SER("	characterID=0x%08x", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CN_GamePlayerSyncActionStateOnly>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CN_GamePlayerSyncActionStateOnly(%d, %d) :: {", Cl::CN_GamePlayerSyncActionStateOnly::NET_ID, packetSize);
	SER("	characterID=0x%08x", buff.Read<LocalActorID>());
	SER("	state=%d", buff.Read<ActionStateID>());
	SER("	bApply=%u", buff.Read<u8>());
	SER("	param1=%d", buff.Read<i32>());
	SER("	param2=%d", buff.Read<i32>());
	SER("	i4=%d", buff.Read<i32>());
	SER("	rotate=%g", buff.Read<f32>());
	SER("	upperRotate=%g", buff.Read<f32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_RTT_Time>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CQ_RTT_Time(%d, %d) :: {", Cl::CQ_RTT_Time::NET_ID, packetSize);
	SER("	time=%u", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_LoadingProgressData>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CQ_LoadingProgressData(%d, %d) :: {", Cl::CQ_LoadingProgressData::NET_ID, packetSize);
	SER("	progress=%u", buff.Read<u8>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_GetGroundItem>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CQ_GetGroundItem(%d, %d) :: {", Cl::CQ_GetGroundItem::NET_ID, packetSize);
	SER("	srcEntityId=%u", buff.Read<u32>());
	SER("	targetEntityId=0x%08x", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_CreateGroundItem>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	SER("SN_CreateGroundItem(%d, %d) :: {}", Sv::SN_CreateGroundItem::NET_ID, packetSize);
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SA_GetGroundItem>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	SER("SA_GetGroundItem(%d, %d) :: {}", Sv::SA_GetGroundItem::NET_ID, packetSize);
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_GetPublicGroundItem>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	SER("SN_GetPublicGroundItem(%d, %d) :: {}", Sv::SN_GetPublicGroundItem::NET_ID, packetSize);
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_PvpEventAnnouncement>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	SER("SN_PvpEventAnnouncement(%d, %d) :: {}", Sv::SN_PvpEventAnnouncement::NET_ID, packetSize);
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_DestroyGroundItem>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	SER("SN_DestroyGroundItem(%d, %d) :: {}", Sv::SN_DestroyGroundItem::NET_ID, packetSize);
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_SkillUpgradeIngame>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CQ_SkillUpgradeIngame(%d, %d) :: {", Cl::CQ_SkillUpgradeIngame::NET_ID, packetSize);
	SER("	userID=%u", buff.Read<u32>());
	SER("	slot=%u", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_GamePlayerTag>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CQ_GamePlayerTag(%d, %d) :: {", Cl::CQ_GamePlayerTag::NET_ID, packetSize);
	SER("	characterID=0x%08x", buff.Read<LocalActorID>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_PlayerJump>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CQ_PlayerJump(%d, %d) :: {", Cl::CQ_PlayerJump::NET_ID, packetSize);
	const u8 excludedFieldBits = buff.Read<u8>();
	SER("	excludedFieldBits=%u", excludedFieldBits);
	SER("	actionID=%d", buff.Read<i32>());
	SER("	actorID=0x%08x", buff.Read<LocalActorID>());
	SER("	rotate=%g", buff.Read<f32>());
	SER("	moveDirX=%g", buff.Read<f32>());
	SER("	moveDirY=%g", buff.Read<f32>());
	if(!(excludedFieldBits & 0x10)) {
		SER("	startPos=%s", PS::ToStr(buff.Read<float3>()));
	}
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
template<>
inline const char* PacketSerialize<Sv::SN_ChatChannelMessage>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_ChatChannelMessage(%d, %d) :: {", Sv::SN_ChatChannelMessage::NET_ID, packetSize);
	SER("	chatType=%u", buff.Read<u32>());
	SER("	senderNickname='%ls'", buff.ReadWideStringObj().data());
	SER("	senderStaffType=%d", buff.Read<i8>());
	SER("	chatMsg='%ls'", buff.ReadWideStringObj().data());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SA_WhisperSend>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SA_WhisperSend(%d, %d) :: {", Sv::SA_WhisperSend::NET_ID, packetSize);
	SER("	retval=%u", buff.Read<u32>());
	SER("	nickname='%ls'", buff.ReadWideStringObj().data());
	SER("	message='%ls'", buff.ReadWideStringObj().data());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_WhisperReceived>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_WhisperReceived(%d, %d) :: {", Sv::SN_WhisperReceived::NET_ID, packetSize);
	SER("	nickname='%ls'", buff.ReadWideStringObj().data());
	SER("	staffType=%d", buff.Read<i8>());
	SER("	message='%ls'", buff.ReadWideStringObj().data());
	SER("}");
	return str.data();
}
DEFAULT_SERIALIZE(Sv::SA_LoadingComplete);
DEFAULT_SERIALIZE(Sv::SN_GameStart);
DEFAULT_SERIALIZE(Sv::SN_ScanEnd);
DEFAULT_SERIALIZE(Sv::SN_SetGameGvt);



template<>
inline const char* PacketSerialize<Sv::SN_PlayerSyncTurn>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	const Sv::SN_PlayerSyncTurn& p = *(Sv::SN_PlayerSyncTurn*)packetData;
	SER("SN_PlayerSyncTurn(%d, %d) :: {", Sv::SN_PlayerSyncTurn::NET_ID, packetSize);
	SER("	entityID=0x%08x", (u32)p.entityID);
	SER("	UpperDir=%s", PS::ToStr(p.UpperDir));
	SER("	nRotate=%g", p.nRotate);
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_DestroyEntity>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	const Sv::SN_DestroyEntity& p = *(Sv::SN_DestroyEntity*)packetData;
	SER("SN_DestroyEntity(%d, %d) :: {", Sv::SN_DestroyEntity::NET_ID, packetSize);
	SER("	objectID=0x%08x", (u32)p.objectID);
	SER("}");
	return str.data();
}
template<>
inline const char* PacketSerialize<Sv::SN_RemoteActivated>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	const Sv::SN_RemoteActivated& p = *(Sv::SN_RemoteActivated*)packetData;
	SER("SN_RemoteActivated(%d, %d) :: {", Sv::SN_RemoteActivated::NET_ID, packetSize);
	SER("	remoteID=0x%08x", p.remoteID);
	SER("	defID=%u", p.defID);
	SER("	penetrationCount=%u", p.penetrationCount);
	SER("	remotePos=(%f, %f, %f)", p.remotePos.x, p.remotePos.y, p.remotePos.z);
	SER("}");
	return str.data();
}
template<>
inline const char* PacketSerialize<Sv::SN_ChangeBattleState>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	const Sv::SN_ChangeBattleState& p = *(Sv::SN_ChangeBattleState*)packetData;
	SER("SN_ChangeBattleState(%d, %d) :: {", Sv::SN_ChangeBattleState::NET_ID, packetSize);
	SER("	objectID=%u", p.objectID);
	SER("	isInBattle=%u", p.isInBattle);
	SER("	baseMoveSpeed=%f", p.baseMoveSpeed);
	SER("}");
	return str.data();
}
template<>
inline const char* PacketSerialize<Sv::SN_UpdateTargetGraphMove>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	const Sv::SN_UpdateTargetGraphMove& p = *(Sv::SN_UpdateTargetGraphMove*)packetData;
	SER("SN_UpdateTargetGraphMove(%d, %d) :: {", Sv::SN_UpdateTargetGraphMove::NET_ID, packetSize);
	SER("	characterID=%u", p.characterID);
	SER("	targetID=%u", p.targetID);
	SER("	targetPos=(%f, %f, %f)", p.targetPos.x, p.targetPos.y, p.targetPos.z);
	SER("	curActionID=%u", p.curActionID);
	SER("}");
	return str.data();
}
template<>
inline const char* PacketSerialize<Sv::SN_BroadcastEvade>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	const Sv::SN_BroadcastEvade& p = *(Sv::SN_BroadcastEvade*)packetData;
	SER("SN_BroadcastEvade(%d, %d) :: {", Sv::SN_BroadcastEvade::NET_ID, packetSize);
	SER("	attID=%u", p.attID);
	SER("	defID=%u", p.defID);
	SER("	remoteID=0x%08x", p.remoteID);
	SER("	remoteDocIndex=%u", p.remoteDocIndex);
	SER("}");
	return str.data();
}
template<>
inline const char* PacketSerialize<Sv::SN_ModifyStatus>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	const Sv::SN_ModifyStatus& p = *(Sv::SN_ModifyStatus*)packetData;
	SER("SN_ModifyStatus(%d, %d) :: {", Sv::SN_ModifyStatus::NET_ID, packetSize);
	SER("	statusID=%u", p.statusID);
	SER("	targetID=%u", p.targetID);
	SER("	overlapCount=%u", p.overlapCount);
	SER("	customValue=%u", p.customValue);
	SER("}");
	return str.data();
}
template<>
inline const char* PacketSerialize<Sv::SN_ResistStatus>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	const Sv::SN_ResistStatus& p = *(Sv::SN_ResistStatus*)packetData;
	SER("SN_ResistStatus(%d, %d) :: {", Sv::SN_ResistStatus::NET_ID, packetSize);
	SER("	target=%u", p.target);
	SER("	caster=%u", p.caster);
	SER("	event=%u", p.event);
	SER("}");
	return str.data();
}
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
DEFAULT_SERIALIZE(Sv::SN_WarehouseItems);
DEFAULT_SERIALIZE(Sv::SN_MutualFriendList);
DEFAULT_SERIALIZE(Sv::SN_GuildMemberStatus);
DEFAULT_SERIALIZE(Sv::SN_GamePlayerSyncByInt);
DEFAULT_SERIALIZE(Sv::SN_PlayerSyncActionStateOnly);
DEFAULT_SERIALIZE(Sv::SN_JukeboxPlay);
DEFAULT_SERIALIZE(Sv::SN_JukeboxEnqueuedList);
DEFAULT_SERIALIZE(Sv::SN_TownHudStatistics);
DEFAULT_SERIALIZE(Sv::SA_GetGuildProfile);
DEFAULT_SERIALIZE(Sv::SA_GetGuildMemberList);
DEFAULT_SERIALIZE(Sv::SA_GetGuildHistoryList);
DEFAULT_SERIALIZE(Sv::SA_GetGuildRankingSeasonList);
DEFAULT_SERIALIZE(Sv::SA_TierRecord);
DEFAULT_SERIALIZE(Sv::SN_ClientSettings);
DEFAULT_SERIALIZE(Cl::CQ_GetGuildProfile);
DEFAULT_SERIALIZE(Cl::CQ_GetGuildMemberList);
DEFAULT_SERIALIZE(Cl::CQ_GetGuildHistoryList);
DEFAULT_SERIALIZE(Cl::CQ_TierRecord);
DEFAULT_SERIALIZE(Cl::CN_ReadyToLoadGameMap);
DEFAULT_SERIALIZE(Cl::CN_GameMapLoaded);
DEFAULT_SERIALIZE(Cl::CQ_LoadingComplete);
DEFAULT_SERIALIZE(Cl::CQ_GameIsReady);
DEFAULT_SERIALIZE(Sv::SN_Unknown_62472);
DEFAULT_SERIALIZE(Sv::SN_UpdateEntrySystem);
DEFAULT_SERIALIZE(Sv::SN_PveTodayStageInfo);
DEFAULT_SERIALIZE(Sv::SN_FatiguePointInfo);
DEFAULT_SERIALIZE(Sv::SN_GameEventActivated);
DEFAULT_SERIALIZE(Sv::SN_GameEventPending);
DEFAULT_SERIALIZE(Sv::SN_AddictionWarning);
DEFAULT_SERIALIZE(Sv::SN_ItemLifetimeExpired);
DEFAULT_SERIALIZE(Sv::SN_EffectLifetimeExpired);
DEFAULT_SERIALIZE(Sv::SN_EffectLifetimeList);
DEFAULT_SERIALIZE(Sv::SN_CurrentGuideMission);
DEFAULT_SERIALIZE(Sv::SN_ServerUtcTime);
DEFAULT_SERIALIZE(Sv::SN_GmDisabledMasters);
DEFAULT_SERIALIZE(Sv::SN_GmDisabledSkins);
DEFAULT_SERIALIZE(Sv::SN_GmDisabledStages);
DEFAULT_SERIALIZE(Sv::SN_HudEventList);
DEFAULT_SERIALIZE(Sv::SN_CshopEventList);
DEFAULT_SERIALIZE(Sv::SN_PlayerStateInTown);

DEFAULT_SERIALIZE(In::HQ_PartyAddMember);
DEFAULT_SERIALIZE(In::HQ_PartyRemoveMember);
DEFAULT_SERIALIZE(In::HQ_PartyCancelMatching);
DEFAULT_SERIALIZE(Cl::CQ_BreakUpPartyRoom);
DEFAULT_SERIALIZE(Cl::CQ_CancelMatching);
DEFAULT_SERIALIZE(Sv::SN_PartyInvite);
DEFAULT_SERIALIZE(Sv::SA_PartyInvite);
DEFAULT_SERIALIZE(Sv::SA_PartyJoin);
DEFAULT_SERIALIZE(Sv::SN_PartyJoin);
DEFAULT_SERIALIZE(Sv::SN_PartyLeave);
DEFAULT_SERIALIZE(Sv::SA_PartyBreakup);
DEFAULT_SERIALIZE(Sv::SN_PartyBreakup);
DEFAULT_SERIALIZE(Sv::SN_PartyCancelInvite);
DEFAULT_SERIALIZE(Sv::SN_CancelMatchingQueue);
template<>
inline const char* PacketSerialize<Cl::CQ_PartyInvite>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CQ_PartyInvite(%d, %d) :: {", Cl::CQ_PartyInvite::NET_ID, packetSize);
	SER("	nickname='%ls'", buff.ReadWideStringObj().data());
	SER("	teamType=%u", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_PartyInviteAccept>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CQ_PartyInviteAccept(%d, %d) :: {", Cl::CQ_PartyInviteAccept::NET_ID, packetSize);
	SER("	partyID=%d", buff.Read<i32>());
	SER("	zero=%u", buff.Read<u32>());
	SER("	inviterNick='%ls'", buff.ReadWideStringObj().data());
	SER("}");
	return str.data();
}
DEFAULT_SERIALIZE(Sv::SN_FriendStateChanged);
DEFAULT_SERIALIZE(Sv::SA_FriendRequest);
DEFAULT_SERIALIZE(Sv::SN_FriendRequested);
DEFAULT_SERIALIZE(Sv::SA_FriendRespond);
DEFAULT_SERIALIZE(Sv::SN_FriendAdded);
DEFAULT_SERIALIZE(Sv::SA_FriendRemove);
DEFAULT_SERIALIZE(Sv::SN_FriendRemoved);
template<>
inline const char* PacketSerialize<Cl::CQ_FriendRequest>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CQ_FriendRequest(%d, %d) :: {", Cl::CQ_FriendRequest::NET_ID, packetSize);
	SER("	nickname='%ls'", buff.ReadWideStringObj().data());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_FriendRequestRespond>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CQ_FriendRequestRespond(%d, %d) :: {", Cl::CQ_FriendRequestRespond::NET_ID, packetSize);
	SER("	nickname='%ls'", buff.ReadWideStringObj().data());
	SER("	accepted=%u", buff.Read<u8>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_FriendRemove>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CQ_FriendRemove(%d, %d) :: {", Cl::CQ_FriendRemove::NET_ID, packetSize);
	SER("	nickname='%ls'", buff.ReadWideStringObj().data());
	SER("}");
	return str.data();
}
DEFAULT_SERIALIZE(Sv::SN_CommunityStatusMessage);
DEFAULT_SERIALIZE(Sv::SA_CommunityStatusMessage);
DEFAULT_SERIALIZE(Sv::SN_FriendPartycreation);
DEFAULT_SERIALIZE(Sv::SA_PartyLeave);
DEFAULT_SERIALIZE(Sv::SN_RestartPvpEntryStart);
DEFAULT_SERIALIZE(Sv::SN_RestartPvpEntryUpdate);
DEFAULT_SERIALIZE(Sv::SA_Honor);
DEFAULT_SERIALIZE(Sv::SN_ItemUpdate);
DEFAULT_SERIALIZE(Sv::SN_ItemAcquisition);
DEFAULT_SERIALIZE(Sv::SN_CCoin);
DEFAULT_SERIALIZE(Sv::SA_NpBuyCshopItem);
DEFAULT_SERIALIZE(Sv::SA_BuyCshopItem);
DEFAULT_SERIALIZE(Sv::SA_BuyShopItem);
DEFAULT_SERIALIZE(Sv::SA_ItemCraft);
DEFAULT_SERIALIZE(Sv::SA_InventoryExpand);
DEFAULT_SERIALIZE(Sv::SA_CreateGuild);
DEFAULT_SERIALIZE(Sv::SA_DissolveGuild);
DEFAULT_SERIALIZE(Sv::SA_DonateToGuild);
DEFAULT_SERIALIZE(Sv::SA_EditGuildnotice);
DEFAULT_SERIALIZE(Sv::SA_GetGuildinvitationlist);
DEFAULT_SERIALIZE(Sv::SA_GetGuildjoinrequestlist);
DEFAULT_SERIALIZE(Sv::SA_GetGuildpublicprofile);
DEFAULT_SERIALIZE(Sv::SA_GetRecommendedguildlist);
DEFAULT_SERIALIZE(Sv::SA_JoinGuild);
DEFAULT_SERIALIZE(Sv::SA_QuitGuild);
DEFAULT_SERIALIZE(Sv::SA_RespondGuildinvitation);
DEFAULT_SERIALIZE(Sv::SA_RespondGuildjoinrequest);
DEFAULT_SERIALIZE(Sv::SA_SetGuildjointype);
DEFAULT_SERIALIZE(Sv::SN_GuildInvitation);
DEFAULT_SERIALIZE(Sv::SN_GuildJoinapproval);
DEFAULT_SERIALIZE(Sv::SN_GuildLevelup);
DEFAULT_SERIALIZE(Sv::SN_GuildNotice);
DEFAULT_SERIALIZE(Sv::SN_GuildchannelLeave);
DEFAULT_SERIALIZE(Sv::SN_GuildfundAdded);
DEFAULT_SERIALIZE(Sv::SN_GuildmemberJoin);
DEFAULT_SERIALIZE(Sv::SN_GuildmemberQuit);
DEFAULT_SERIALIZE(Sv::SN_GuildpointandfundChange);
DEFAULT_SERIALIZE(Sv::SN_LevelIngame);
DEFAULT_SERIALIZE(Sv::SN_ExpIngame);
DEFAULT_SERIALIZE(Sv::SN_CharacterPropertyPoint);
DEFAULT_SERIALIZE(Sv::SQ_TeleportObject);
DEFAULT_SERIALIZE(Sv::SN_PlayerSyncTeleport);
DEFAULT_SERIALIZE(Cl::CQ_PlayerSyncTeleport);
DEFAULT_SERIALIZE(Sv::SN_CharacterPropertyInfo);
DEFAULT_SERIALIZE(Sv::SN_CharacterPropertyReset);
DEFAULT_SERIALIZE(Sv::SA_CharacterPropertyUpgrade);
DEFAULT_SERIALIZE(Sv::SA_CharacterPropertyReset);
DEFAULT_SERIALIZE(Sv::SN_NotifyIsInSafeZone);
DEFAULT_SERIALIZE(Sv::SN_RequestVoting);
DEFAULT_SERIALIZE(Sv::SN_Vote);
DEFAULT_SERIALIZE(Sv::SN_VotingResult);
DEFAULT_SERIALIZE(Sv::SA_TransformCharacter);
DEFAULT_SERIALIZE(Sv::SN_TransformCharacter);
DEFAULT_SERIALIZE(Sv::SN_ChangeCharacterMode);
DEFAULT_SERIALIZE(Sv::SN_PvpResultScoreGot);
DEFAULT_SERIALIZE(Sv::SN_PraiseInfo);
DEFAULT_SERIALIZE(Sv::SN_MonsterRespawnTime);
DEFAULT_SERIALIZE(Sv::SN_InteractionCancel);
DEFAULT_SERIALIZE(Sv::SN_StageSkillAvailable);
DEFAULT_SERIALIZE(Sv::SA_StageSkillSelect);
DEFAULT_SERIALIZE(Sv::SN_StageSkillSelect);
DEFAULT_SERIALIZE(Sv::SN_BushObjectState);
DEFAULT_SERIALIZE(Sv::SN_LightOnArea);
DEFAULT_SERIALIZE(Sv::SN_LightOffArea);
DEFAULT_SERIALIZE(Sv::SN_UpdateGamePlayerTagCooltime);
DEFAULT_SERIALIZE(Sv::SN_UpdateCanCastSkillSlotUg);
DEFAULT_SERIALIZE(Sv::SN_JukeboxHotTrackList);
DEFAULT_SERIALIZE(Sv::SN_Calendar);
DEFAULT_SERIALIZE(Sv::SN_MatchingPartyConfirm);
DEFAULT_SERIALIZE(Cl::CQ_GetUserInfo);
DEFAULT_SERIALIZE(Sv::SA_MasterUnpick);
DEFAULT_SERIALIZE(Sv::SN_MasterUnpick);
DEFAULT_SERIALIZE(Sv::SA_ItemTrade);
DEFAULT_SERIALIZE(Sv::SA_ItemUse);
DEFAULT_SERIALIZE(Sv::SA_ActivityReward);
DEFAULT_SERIALIZE(Sv::SN_MailGetAttachmentResult);
DEFAULT_SERIALIZE(Sv::SA_PartySwapTeam);
DEFAULT_SERIALIZE(Sv::SN_PartySwapTeam);
DEFAULT_SERIALIZE(Sv::SA_PartyAddBot);
DEFAULT_SERIALIZE(Sv::SA_PartyRemoveBot);
DEFAULT_SERIALIZE(Sv::SN_PvpResultScoreOccupy);
DEFAULT_SERIALIZE(Sv::SA_GetGuildranklist);
DEFAULT_SERIALIZE(Sv::SA_GetGuildrank);
DEFAULT_SERIALIZE(Sv::SA_GetGuildrankrewardinfo);
DEFAULT_SERIALIZE(Sv::SA_GetPlayingGuildpvplist);
DEFAULT_SERIALIZE(Sv::SN_SortieMasterBanPhaseEnd);
DEFAULT_SERIALIZE(Sv::SN_SortieMasterBan);
DEFAULT_SERIALIZE(Sv::SA_SortieMasterBan);
DEFAULT_SERIALIZE(Sv::SN_SortieMasterBanPhaseStart);
DEFAULT_SERIALIZE(Sv::SA_GetGuildskilllist);
DEFAULT_SERIALIZE(Sv::SA_BuyGuildskill);
DEFAULT_SERIALIZE(Sv::SA_ExtendGuildlimitedskill);
DEFAULT_SERIALIZE(Sv::SN_GuildskillAcquired);
DEFAULT_SERIALIZE(Sv::SN_GuildskillUpgraded);
DEFAULT_SERIALIZE(Sv::SN_GuildskillExtended);
DEFAULT_SERIALIZE(Sv::SN_GuildconstraintChange);
DEFAULT_SERIALIZE(Sv::SA_EditGuildtag);
DEFAULT_SERIALIZE(Sv::SN_GuildtagChanged);
DEFAULT_SERIALIZE(Sv::SA_SetGuildemblem);
DEFAULT_SERIALIZE(Sv::SN_GuildemblemChange);
DEFAULT_SERIALIZE(Sv::SA_SetGuildmemberclass);
DEFAULT_SERIALIZE(Sv::SA_SetGuildmaster);
DEFAULT_SERIALIZE(Sv::SA_CreateGuildmembership);
DEFAULT_SERIALIZE(Sv::SA_DeleteGuildmembership);
DEFAULT_SERIALIZE(Sv::SA_EditGuildmembership);
DEFAULT_SERIALIZE(Sv::SN_GuildmembershipAdded);
DEFAULT_SERIALIZE(Sv::SN_GuildmembershipRemoved);
DEFAULT_SERIALIZE(Sv::SN_GuildmembershipModified);
DEFAULT_SERIALIZE(Sv::SN_GuildmemberChgclass);
DEFAULT_SERIALIZE(Sv::SN_GuildmasterDelegation);
DEFAULT_SERIALIZE(Sv::SA_MyPveRanking);
DEFAULT_SERIALIZE(Sv::SA_RequestSummaryInfoEach);
DEFAULT_SERIALIZE(Sv::SA_StartGame);
DEFAULT_SERIALIZE(Sv::SA_MasterTrainingEvent);
DEFAULT_SERIALIZE(Sv::SN_GameAreaMove);
DEFAULT_SERIALIZE(Sv::SN_StageClearResult);
DEFAULT_SERIALIZE(Sv::SN_RewardItemList);
DEFAULT_SERIALIZE(Sv::SN_RewardStageStartNp);
DEFAULT_SERIALIZE(Sv::SA_RewardSelectSlot);
DEFAULT_SERIALIZE(Sv::SA_RewardRequestFinish);
DEFAULT_SERIALIZE(Sv::SN_WarnPveGameend);
DEFAULT_SERIALIZE(Sv::SN_NotifyUserLifeInfo);
DEFAULT_SERIALIZE(Sv::SN_StageRank);
DEFAULT_SERIALIZE(Sv::SN_DefenceClearResult);
DEFAULT_SERIALIZE(Sv::SN_PhaseStart);
DEFAULT_SERIALIZE(Sv::SN_PhaseEnd);
DEFAULT_SERIALIZE(Sv::SA_DefenceBestRecord);
DEFAULT_SERIALIZE(Sv::SA_GearEquip);
DEFAULT_SERIALIZE(Sv::SA_PvpRecord);
DEFAULT_SERIALIZE(Sv::SA_FriendStateChange);
DEFAULT_SERIALIZE(Sv::SA_QuickRunArena);
DEFAULT_SERIALIZE(Sv::SA_WeaponUnlock);
DEFAULT_SERIALIZE(Sv::SA_WeaponUpgrade);
DEFAULT_SERIALIZE(Sv::SA_WeaponEquip);
DEFAULT_SERIALIZE(Sv::SA_SkillUpgrade);
DEFAULT_SERIALIZE(Sv::SN_TitleAdd);
DEFAULT_SERIALIZE(Sv::SN_SummaryRewardResult);
DEFAULT_SERIALIZE(Sv::SN_SummaryUpdate);
DEFAULT_SERIALIZE(Sv::SN_PlayerTitleSelect);
DEFAULT_SERIALIZE(Sv::SA_MastergearAdd);
DEFAULT_SERIALIZE(Sv::SN_MissionUpdate);
DEFAULT_SERIALIZE(Sv::SN_RecommendedfriendList);
DEFAULT_SERIALIZE(Sv::SA_SearchGuild);
DEFAULT_SERIALIZE(Sv::SN_ActiveIngameEventList);
DEFAULT_SERIALIZE(Sv::SN_PveLastgameRanking);
DEFAULT_SERIALIZE(Sv::SN_RegameData);
DEFAULT_SERIALIZE(Sv::SN_RegameAvailable);
DEFAULT_SERIALIZE(Sv::SA_RestartGame);
DEFAULT_SERIALIZE(Sv::SA_GiftFriendsAvailable);
DEFAULT_SERIALIZE(Sv::SA_RestartPvpEntryJoin);
DEFAULT_SERIALIZE(Sv::SN_RestartPvpGameInfo);
DEFAULT_SERIALIZE(Sv::SN_RestartPvpEntryFinish);
DEFAULT_SERIALIZE(Sv::SA_SelectBadge);
DEFAULT_SERIALIZE(Sv::SA_RewardBuyChance);
DEFAULT_SERIALIZE(Sv::SA_TierStageRecord);
DEFAULT_SERIALIZE(Sv::SN_PartyInviteRecommend);
DEFAULT_SERIALIZE(Sv::SA_PartyInviteRecommendResponse);
DEFAULT_SERIALIZE(Sv::SN_PartyInviteRecommendResponse);
DEFAULT_SERIALIZE(Sv::SN_AvailableAccountEquipmentList);
DEFAULT_SERIALIZE(Sv::SN_BroadcastGamePingData);
DEFAULT_SERIALIZE(Sv::SN_DefenceLastgameRanking);
DEFAULT_SERIALIZE(Sv::SN_ActivatedSupportkit);
DEFAULT_SERIALIZE(Sv::SA_SelectAccountEquipment);
DEFAULT_SERIALIZE(Sv::SA_TitleSelect);
DEFAULT_SERIALIZE(Sv::SA_SkillUnlock);
DEFAULT_SERIALIZE(Sv::SA_PvpDetailRecord);
DEFAULT_SERIALIZE(Sv::SA_GearUnequip);
DEFAULT_SERIALIZE(Sv::SA_MastergearSelect);
DEFAULT_SERIALIZE(Sv::SA_MastergearRename);
DEFAULT_SERIALIZE(Sv::SA_ItemMerge);
DEFAULT_SERIALIZE(Sv::SQ_PrepareHandoverToChannel);
#undef DEFAULT_SERIALIZE

template<>
inline const char* PacketSerialize<Sv::SN_AddStatus>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_AddStatus(%d, %d) :: {", Sv::SN_AddStatus::NET_ID, packetSize);
	const u8 bits = buff.Read<u8>();
	SER("	excludedFieldBits=%u", bits);
	if(!(bits & 1)) buff.ReadRaw(40);
	SER("	statusID=%u", buff.Read<u32>());
	SER("	bEnabled=%u", buff.Read<u8>());
	SER("	targetID=0x%08x", buff.Read<u32>());
	SER("	casterID=0x%08x", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SA_SkillSelect>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SA_SkillSelect(%d, %d) :: {", Sv::SA_SkillSelect::NET_ID, packetSize);
	SER("	characterID=%u", buff.Read<u32>());
	SER("	result=%u", buff.Read<u32>());
	SER("	skill=%u", buff.Read<u32>());
	SER("	slot=%u", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_SkillSelect>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_SkillSelect(%d, %d) :: {", Sv::SN_SkillSelect::NET_ID, packetSize);
	SER("	userId=%u", buff.Read<u32>());
	SER("	characterID=%u", buff.Read<u32>());
	SER("	skill=%u", buff.Read<u32>());
	SER("	slot=%u", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_Praise>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CQ_Praise(%d, %d) :: {", Cl::CQ_Praise::NET_ID, packetSize);
	SER("	nickname='%ls'", buff.ReadWideStringObj().data());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_EnterChannel>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_EnterChannel(%d, %d) :: {", Sv::SN_EnterChannel::NET_ID, packetSize);
	SER("	chID=%u", buff.Read<u32>());
	SER("	channelType=%u", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_PvpAvailableReportCount>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_PvpAvailableReportCount(%d, %d) :: {", Sv::SN_PvpAvailableReportCount::NET_ID, packetSize);
	SER("	isTroll=%u", buff.Read<u8>());
	SER("	remainingCount=%u", buff.Read<u8>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_PraiseStart>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_PraiseStart(%d, %d) :: {", Sv::SN_PraiseStart::NET_ID, packetSize);
	SER("	remainDurationMs=%u", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SA_Praise>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SA_Praise(%d, %d) :: {", Sv::SA_Praise::NET_ID, packetSize);
	SER("	errorType=%u", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_Praise>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_Praise(%d, %d) :: {", Sv::SN_Praise::NET_ID, packetSize);
	SER("	nickName='%ls'", buff.ReadWideStringObj().data());
	SER("	praiseCount=%u", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_UserReturnToCity>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_UserReturnToCity(%d, %d) :: {", Sv::SN_UserReturnToCity::NET_ID, packetSize);
	SER("	nickname='%ls'", buff.ReadWideStringObj().data());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SA_ReturnToCity>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SA_ReturnToCity(%d, %d) :: {", Sv::SA_ReturnToCity::NET_ID, packetSize);
	SER("	errcode=%u", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_AIMonsterSyncMove>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	SER("SN_AIMonsterSyncMove(%d, %d) :: {}", Sv::SN_AIMonsterSyncMove::NET_ID, packetSize);
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_AIMonsterSyncTurn>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	SER("SN_AIMonsterSyncTurn(%d, %d) :: {}", Sv::SN_AIMonsterSyncTurn::NET_ID, packetSize);
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SA_RestartPvpEntryLeave>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	SER("SA_RestartPvpEntryLeave(%d, %d) :: {}", Sv::SA_RestartPvpEntryLeave::NET_ID, packetSize);
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_RewardStageStart>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	SER("SN_RewardStageStart(%d, %d) :: {}", Sv::SN_RewardStageStart::NET_ID, packetSize);
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_RewardStageEnd>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	SER("SN_RewardStageEnd(%d, %d) :: {}", Sv::SN_RewardStageEnd::NET_ID, packetSize);
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_PvpResult>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	SER("SN_PvpResult(%d, %d) :: {}", Sv::SN_PvpResult::NET_ID, packetSize);
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_PvpResultScoreDeathmatch>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	SER("SN_PvpResultScoreDeathmatch(%d, %d) :: {}", Sv::SN_PvpResultScoreDeathmatch::NET_ID, packetSize);
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_Gauge>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_Gauge(%d, %d) :: {", Sv::SN_Gauge::NET_ID, packetSize);
	SER("	userId=%u", buff.Read<u32>());
	SER("	updateType=%u", buff.Read<u8>());
	SER("	flag=%u", buff.Read<u8>());
	SER("	overheatTime=%g", buff.Read<f32>());
	SER("	gauge=%u", buff.Read<u16>());
	SER("	maxGauge=%u", buff.Read<u16>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_RemoveStatus>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_RemoveStatus(%d, %d) :: {", Sv::SN_RemoveStatus::NET_ID, packetSize);
	SER("	statusID=%u", buff.Read<u32>());
	SER("	targetID=0x%08x", buff.Read<u32>());
	SER("	casterID=0x%08x", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_RemoteSyncTarget>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_RemoteSyncTarget(%d, %d) :: {", Sv::SN_RemoteSyncTarget::NET_ID, packetSize);
	SER("	excludedFieldBits=%u", buff.Read<u8>());
	SER("	remoteID=0x%08x", buff.Read<u32>());
	SER("	targetEntity=0x%08x", buff.Read<u32>());
	SER("	DestPos=%s", PS::ToStr(buff.Read<float3>()));
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_AiSetTarget>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_AiSetTarget(%d, %d) :: {", Sv::SN_AiSetTarget::NET_ID, packetSize);
	SER("	entityID=0x%08x", buff.Read<u32>());
	SER("	targetEntityID=0x%08x", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_AiSyncMonsterSkillTarget>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_AiSyncMonsterSkillTarget(%d, %d) :: {", Sv::SN_AiSyncMonsterSkillTarget::NET_ID, packetSize);
	SER("	entityID=0x%08x", buff.Read<u32>());
	SER("	targetID=0x%08x", buff.Read<u32>());
	SER("	targetPos=%s", PS::ToStr(buff.Read<float3>()));
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_AiSetMovetargetpos>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_AiSetMovetargetpos(%d, %d) :: {", Sv::SN_AiSetMovetargetpos::NET_ID, packetSize);
	SER("	entityID=0x%08x", buff.Read<u32>());
	SER("	movePresetType=%u", buff.Read<u32>());
	SER("	moveTargetPos=%s", PS::ToStr(buff.Read<float3>()));
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_AiSetActionState>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_AiSetActionState(%d, %d) :: {", Sv::SN_AiSetActionState::NET_ID, packetSize);
	const u8 excluded = buff.Read<u8>();
	SER("	excludedFieldBits=%u", excluded);
	SER("	entityID=0x%08x", buff.Read<u32>());
	if(!(excluded & 0x02)) SER("	motionType=%d", buff.Read<i16>());
	if(!(excluded & 0x04)) SER("	lowerBodyYaw=%d", buff.Read<i16>());
	if(!(excluded & 0x08)) SER("	moveSpeed=%u", buff.Read<u16>());
	SER("	pos=%s", PS::ToStr(buff.Read<float3>()));
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_BroadcastDamage>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_BroadcastDamage(%d, %d) :: {", Sv::SN_BroadcastDamage::NET_ID, packetSize);
	SER("	damageSeqNum=%u", buff.Read<u32>());
	SER("	remoteID=0x%08x", buff.Read<u32>());
	SER("	defID=0x%08x", buff.Read<u32>());
	SER("	attID=0x%08x", buff.Read<u32>());
	SER("	remoteDocIndex=%u", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_RemoteSyncCreateFromRemoteDoc>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_RemoteSyncCreateFromRemoteDoc(%d, %d) :: {", Sv::SN_RemoteSyncCreateFromRemoteDoc::NET_ID, packetSize);
	SER("	ParentEntity=0x%08x", buff.Read<u32>());
	SER("	OwnerEntity=0x%08x", buff.Read<u32>());
	SER("	RemoteSeedID=0x%08x", buff.Read<u32>());
	SER("	FirePosition=%s", PS::ToStr(buff.Read<float3>()));
	const u16 rotX = buff.Read<u16>();
	const u16 rotY = buff.Read<u16>();
	const u16 rotZ = buff.Read<u16>();
	SER("	FireRotation=(%u, %u, %u)", rotX, rotY, rotZ);
	SER("	TargetPosition=%s", PS::ToStr(buff.Read<float3>()));
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_RemoteSyncCreateFromCreatorId>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_RemoteSyncCreateFromCreatorId(%d, %d) :: {", Sv::SN_RemoteSyncCreateFromCreatorId::NET_ID, packetSize);
	SER("	ParentEntity=0x%08x", buff.Read<u32>());
	SER("	OwnerEntity=0x%08x", buff.Read<u32>());
	SER("	RemoteSeedID=0x%08x", buff.Read<u32>());
	SER("	FirePosition=%s", PS::ToStr(buff.Read<float3>()));
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_GamePlayerSyncPhy>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CQ_GamePlayerSyncPhy(%d, %d) :: {", Cl::CQ_GamePlayerSyncPhy::NET_ID, packetSize);
	SER("	ParentEntity=0x%08x", buff.Read<u32>());
	SER("	OwnerEntity=0x%08x", buff.Read<u32>());
	SER("	RemoteSeedID=0x%08x", buff.Read<u32>());
	SER("	FirePosition=%s", PS::ToStr(buff.Read<float3>()));
	for(int i = 0; i < 6; i++) buff.Read<u16>();
	const i16 aimX = buff.Read<i16>();
	const i16 aimY = buff.Read<i16>();
	const i16 aimZ = buff.Read<i16>();
	SER("	aimOffset=(%d, %d, %d)", aimX, aimY, aimZ);
	const u16 nameLen = buff.Read<u16>();
	SER("	charName='%.*ls'", nameLen, (const wchar*)buff.ReadRaw(nameLen * sizeof(wchar)));
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_RemoteSyncCreateFromRemoteDoc>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CQ_RemoteSyncCreateFromRemoteDoc(%d, %d) :: {", Cl::CQ_RemoteSyncCreateFromRemoteDoc::NET_ID, packetSize);
	SER("	ParentEntity=0x%08x", buff.Read<u32>());
	SER("	OwnerEntity=0x%08x", buff.Read<u32>());
	SER("	RemoteSeedID=0x%08x", buff.Read<u32>());
	SER("	FirePosition=%s", PS::ToStr(buff.Read<float3>()));
	const u16 rotX = buff.Read<u16>();
	const u16 rotY = buff.Read<u16>();
	const u16 rotZ = buff.Read<u16>();
	SER("	FireRotation=(%u, %u, %u)", rotX, rotY, rotZ);
	SER("	TargetPosition=%s", PS::ToStr(buff.Read<float3>()));
	const u16 nameLen = buff.Read<u16>();
	SER("	FireObject='%.*ls'", nameLen, (const wchar*)buff.ReadRaw(nameLen * sizeof(wchar)));
	SER("	Scale=%u", buff.Read<u16>());
	SER("	LifeTime=%g", buff.Read<f32>());
	SER("	HitInvalidTarget=%u", buff.Read<u32>());
	SER("	RemoteDocIndex=%u", buff.Read<u32>());
	SER("	TargetEntity=0x%08x", buff.Read<u32>());
	SER("	RemoteCreateFlags=%u", buff.Read<u8>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_RemoteSyncTarget>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CQ_RemoteSyncTarget(%d, %d) :: {", Cl::CQ_RemoteSyncTarget::NET_ID, packetSize);
	const u8 bits = buff.Read<u8>();
	SER("	excludedFieldBits=%u", bits);
	SER("	remoteID=0x%08x", buff.Read<u32>());
	SER("	targetEntity=0x%08x", buff.Read<u32>());
	SER("	DestPos=%s", PS::ToStr(buff.Read<float3>()));
	if(!(bits & 8)) SER("	MoveDir=%s", PS::ToStr(buff.Read<float3>()));
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_PreCastSkill>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	const Cl::CQ_PreCastSkill& pre = SafeCast<Cl::CQ_PreCastSkill>(packetData, packetSize);
	SER("CQ_PreCastSkill(%d, %d) :: {", Cl::CQ_PreCastSkill::NET_ID, packetSize);
	SER("	skillId=%u", pre.skillId);
	SER("	castId=%u", pre.castId);
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_ExecuteSkill>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CQ_ExecuteSkill(%d, %d) :: {", Cl::CQ_ExecuteSkill::NET_ID, packetSize);
	SER("	charID=0x%08x", buff.Read<u32>());
	SER("	skillID=%u", buff.Read<u32>());
	SER("	targetPos=%s", PS::ToStr(buff.Read<float3>()));
	const u16 count = buff.Read<u16>();
	SER("	targetIDs(%u)=[", count);
	for(u16 i = 0; i < count; i++) SER("		0x%08x", buff.Read<u32>());
	SER("	]");
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_SwitchOnToggleSkill>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	const Cl::CQ_SwitchOnToggleSkill& toggle = SafeCast<Cl::CQ_SwitchOnToggleSkill>(packetData, packetSize);
	SER("CQ_SwitchOnToggleSkill(%d, %d) :: {", Cl::CQ_SwitchOnToggleSkill::NET_ID, packetSize);
	SER("	charID=0x%08x", toggle.charID);
	SER("	skillID=%u", toggle.skillID);
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_SwitchOffToggleSkill>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	const Cl::CQ_SwitchOffToggleSkill& toggle = SafeCast<Cl::CQ_SwitchOffToggleSkill>(packetData, packetSize);
	SER("CQ_SwitchOffToggleSkill(%d, %d) :: {", Cl::CQ_SwitchOffToggleSkill::NET_ID, packetSize);
	SER("	charID=0x%08x", toggle.charID);
	SER("	skillID=%u", toggle.skillID);
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_InteractionStatus>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_InteractionStatus(%d, %d) :: {", Sv::SN_InteractionStatus::NET_ID, packetSize);
	SER("	allowedTeamMask=0x%x", buff.Read<u32>());
	SER("	objectID=0x%08x", buff.Read<u32>());
	SER("	interactionID=%u", buff.Read<u32>());
	SER("	occupiedTeam=%d", buff.Read<i32>());
	SER("	coolTime=%g", buff.Read<f32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_InteractionCasting>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_InteractionCasting(%d, %d) :: {", Sv::SN_InteractionCasting::NET_ID, packetSize);
	SER("	objectID=0x%08x", buff.Read<u32>());
	SER("	targetID=0x%08x", buff.Read<u32>());
	SER("	actionState=%u", buff.Read<u32>());
	SER("	castingTimeMS=%u", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_InteractionExecute>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_InteractionExecute(%d, %d) :: {", Sv::SN_InteractionExecute::NET_ID, packetSize);
	SER("	objectID=0x%08x", buff.Read<u32>());
	SER("	targetID=0x%08x", buff.Read<u32>());
	SER("	actionState=%u", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_InteractionCasting>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CQ_InteractionCasting(%d, %d) :: {", Cl::CQ_InteractionCasting::NET_ID, packetSize);
	SER("	self=0x%08x", buff.Read<u32>());
	SER("	target=0x%08x", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_InteractionCancel>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CQ_InteractionCancel(%d, %d) :: {", Cl::CQ_InteractionCancel::NET_ID, packetSize);
	SER("	param=0x%08x", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Cl::CQ_WeaponState>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("CQ_WeaponState(%d, %d) :: {", Cl::CQ_WeaponState::NET_ID, packetSize);
	SER("	ownerID=0x%08x", buff.Read<u32>());
	SER("	state=%u", buff.Read<u32>());
	SER("	chargeLevel=%u", buff.Read<u8>());
	SER("	firingCombo=%u", buff.Read<u8>());
	SER("	unk4=%u", buff.Read<u32>());
	SER("	aimYaw=%g", buff.Read<f32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_BushState>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_BushState(%d, %d) :: {", Sv::SN_BushState::NET_ID, packetSize);
	SER("	bushID=%u", buff.Read<u32>());
	SER("	curState=%u", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_UpdateMasterGroupingEffect>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_UpdateMasterGroupingEffect(%d, %d) :: {", Sv::SN_UpdateMasterGroupingEffect::NET_ID, packetSize);
	SER("	instanceID=%u", buff.Read<u32>());
	SER("	index=%d", buff.Read<i32>());
	SER("	classType=%d", buff.Read<i32>());
	SER("	activate=%d", buff.Read<u8>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_DespawnMonster>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_DespawnMonster(%d, %d) :: {", Sv::SN_DespawnMonster::NET_ID, packetSize);
	SER("	objectID=0x%08x", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_UpdateStat>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_UpdateStat(%d, %d) :: {", Sv::SN_UpdateStat::NET_ID, packetSize);
	SER("	characterID=0x%08x", buff.Read<u32>());
	SER("	statType=%u", buff.Read<u32>());
	SER("	cur=%g", buff.Read<f32>());
	SER("	max=%g", buff.Read<f32>());
	SER("	reasonCode=%u", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_NotifyCooltime>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_NotifyCooltime(%d, %d) :: {", Sv::SN_NotifyCooltime::NET_ID, packetSize);
	SER("	characterID=0x%08x", buff.Read<u32>());
	const u16 count = buff.Read<u16>();
	for(int i = 0; i < count; i++) {
		const u32 type = buff.Read<u32>();
		const u32 id = buff.Read<u32>();
		const u32 left = buff.Read<u32>();
		const u32 total = buff.Read<u32>();
		SER("	type=%u id=%u left=%u total=%u", type, id, left, total);
	}
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SA_SwitchOnToggleSkill>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SA_SwitchOnToggleSkill(%d, %d) :: {", Sv::SA_SwitchOnToggleSkill::NET_ID, packetSize);
	SER("	entity=0x%08x", buff.Read<u32>());
	SER("	ret=%u", buff.Read<u32>());
	SER("	skillIndex=%u", buff.Read<u32>());
	SER("	actionState=%u", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SA_SwitchOffToggleSkill>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SA_SwitchOffToggleSkill(%d, %d) :: {", Sv::SA_SwitchOffToggleSkill::NET_ID, packetSize);
	SER("	entity=0x%08x", buff.Read<u32>());
	SER("	ret=%u", buff.Read<u32>());
	SER("	skillIndex=%u", buff.Read<u32>());
	SER("	actionState=%u", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_SwitchOnToggleSkill>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_SwitchOnToggleSkill(%d, %d) :: {", Sv::SN_SwitchOnToggleSkill::NET_ID, packetSize);
	SER("	entity=0x%08x", buff.Read<u32>());
	SER("	ret=%u", buff.Read<u32>());
	SER("	skillIndex=%u", buff.Read<u32>());
	const u16 count = buff.Read<u16>();
	SER("	remoteList(%u)", count);
	for(u16 i = 0; i < count; i++) SER("		%u", buff.Read<u32>());
	SER("	param1=%u", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_SwitchOffToggleSkill>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_SwitchOffToggleSkill(%d, %d) :: {", Sv::SN_SwitchOffToggleSkill::NET_ID, packetSize);
	SER("	entity=0x%08x", buff.Read<u32>());
	SER("	ret=%u", buff.Read<u32>());
	SER("	skillIndex=%u", buff.Read<u32>());
	SER("	param1=%u", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_CancelSkill>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_CancelSkill(%d, %d) :: {", Sv::SN_CancelSkill::NET_ID, packetSize);
	SER("	entity=0x%08x", buff.Read<u32>());
	SER("	ret=%u", buff.Read<u32>());
	SER("	skillIndex=%u", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SA_ExecuteSkill>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SA_ExecuteSkill(%d, %d) :: {", Sv::SA_ExecuteSkill::NET_ID, packetSize);
	SER("	entity=0x%08x", buff.Read<u32>());
	SER("	ret=%u", buff.Read<u32>());
	SER("	skillIndex=%u", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_HideDetect>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_HideDetect(%d, %d) :: {", Sv::SN_HideDetect::NET_ID, packetSize);
	SER("	hideDetectorOwner=0x%08x", buff.Read<u32>());
	SER("	targetID=0x%08x", buff.Read<u32>());
	SER("	flag=%u", buff.Read<u8>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_ChangeSkillSlot>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_ChangeSkillSlot(%d, %d) :: {", Sv::SN_ChangeSkillSlot::NET_ID, packetSize);
	SER("	entity=0x%08x", buff.Read<u32>());
	SER("	slotIndex=%u", buff.Read<u32>());
	SER("	skillIndex=%u", buff.Read<u32>());
	SER("	resetBaseComboCooltimeID=%u", buff.Read<u32>());
	SER("	param=%u", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_AiSyncBehaviorMotion>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_AiSyncBehaviorMotion(%d, %d) :: {", Sv::SN_AiSyncBehaviorMotion::NET_ID, packetSize);
	SER("	entityID=0x%08x", buff.Read<u32>());
	SER("	motionType=%u", buff.Read<u32>());
	SER("	stance=%u", buff.Read<u8>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_SyncStanceType>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_SyncStanceType(%d, %d) :: {", Sv::SN_SyncStanceType::NET_ID, packetSize);
	SER("	entityID=0x%08x", buff.Read<u32>());
	SER("	stanceType=%u", buff.Read<u32>());
	SER("	durationtimeSec=%g", buff.Read<f32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_ScoreUpdate>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_ScoreUpdate(%d, %d) :: {", Sv::SN_ScoreUpdate::NET_ID, packetSize);
	SER("	usn=%u", buff.Read<u32>());
	SER("	teamType=%d", buff.Read<i32>());
	const u16 count = buff.Read<u16>();
	for(int i = 0; i < count; i++) {
		const u32 type = buff.Read<u32>();
		SER("	record type=%u value=%u", type, buff.Read<u32>());
	}
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_DeadAck>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_DeadAck(%d, %d) :: {", Sv::SN_DeadAck::NET_ID, packetSize);
	SER("	sourceID=0x%08x", buff.Read<u32>());
	SER("	targetID=0x%08x", buff.Read<u32>());
	SER("	targetDocIndex=%u", buff.Read<u32>());
	SER("	dwRemoteID=%u", buff.Read<u32>());
	SER("	dwRemoteDocIndex=%u", buff.Read<u32>());
	SER("	isPast=%u", buff.Read<u8>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_DeadDamageInfo>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_DeadDamageInfo(%d, %d) :: {", Sv::SN_DeadDamageInfo::NET_ID, packetSize);
	auto readList = [&](const char* label) {
		const u16 count = buff.Read<u16>();
		SER("	%s count=%u", label, count);
		for(int i = 0; i < count; i++) {
			const u32 objectID = buff.Read<u32>();
			const u32 skill = buff.Read<u32>();
			const u32 status = buff.Read<u32>();
			const u32 damage = buff.Read<u32>();
			const u8 monster = buff.Read<u8>();
			const u8 type = buff.Read<u8>();
			SER("		object=0x%08x skill=%u status=%u damage=%u monster=%u type=%u", objectID, skill, status, damage, monster, type);
		}
		SER("	%s attackerKey=%u", label, buff.Read<u32>());
	};
	readList("killer");
	const u16 others = buff.Read<u16>();
	for(int i = 0; i < others; i++) readList("other");
	SER("	durationTime=%u", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_RespawnDelaytime>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_RespawnDelaytime(%d, %d) :: {", Sv::SN_RespawnDelaytime::NET_ID, packetSize);
	SER("	usn=%u", buff.Read<u32>());
	SER("	respawnDelayTimeMS=%u", buff.Read<u32>());
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_RevivePlayerAtStartingPoint>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_RevivePlayerAtStartingPoint(%d, %d) :: {", Sv::SN_RevivePlayerAtStartingPoint::NET_ID, packetSize);
	SER("	usn=%u", buff.Read<u32>());
	SER("	activeID=0x%08x", buff.Read<u32>());
	SER("	inactiveID=0x%08x", buff.Read<u32>());
	SER("	position=%s", PS::ToStr(buff.Read<float3>()));
	SER("}");
	return str.data();
}

template<>
inline const char* PacketSerialize<Sv::SN_BroadcastNonRemoteDamage>(const void* packetData, const i32 packetSize)
{
	SER_BEGIN();
	ConstBuffer buff(packetData, packetSize);
	SER("SN_BroadcastNonRemoteDamage(%d, %d) :: {", Sv::SN_BroadcastNonRemoteDamage::NET_ID, packetSize);
	SER("	defID=0x%08x", buff.Read<u32>());
	SER("	attID=0x%08x", buff.Read<u32>());
	SER("	statusDocID=%u", buff.Read<u32>());
	SER("	skillDocID=%u", buff.Read<u32>());
	SER("	damageType=%u", buff.Read<u32>());
	SER("	damage=%u", buff.Read<u32>());
	SER("	masterGroupingDamage=%u", buff.Read<u32>());
	SER("}");
	return str.data();
}


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
