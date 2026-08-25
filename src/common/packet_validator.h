#pragma once
#include <common/base.h>
#include <common/protocol.h>

// ValidatePacket: checks an incoming client packet before it is parsed.
// Each packet type decides what makes it valid (exact wire size, member
// bounds, ...). The default rule is an exact size match; specialize for
// packets whose members need per-field checks.
template<typename T>
inline bool ValidatePacket(const void* packetData, i32 packetSize)
{
	(void)packetData;
	return packetSize == sizeof(T);
}

// CQ_FirstHello: every member is a plain integer (u32 dwProtocolCRC,
// u32 dwErrorCRC, u32 version, u8 unknown) with no specific bound, so a
// valid packet is simply one whose size matches the struct exactly.
template<>
inline bool ValidatePacket<Cl::CQ_FirstHello>(const void* packetData, i32 packetSize)
{
	(void)packetData;
	return packetSize == sizeof(Cl::CQ_FirstHello);
}

// CQ_UserLogin: four wide strings (nick, password, type, extra) followed by
// a u32. Each string is a u16 char count + count*2 bytes, so validation must
// walk each string and verify its length against the remaining packet size.
template<>
inline bool ValidatePacket<Cl::CQ_UserLogin>(const void* packetData, i32 packetSize)
{
	ConstBuffer buff(packetData, packetSize);

	for(int i = 0; i < 4; i++) {
		if(!buff.CanRead(sizeof(u16))) return false;
		const u16 len = buff.Read<u16>();
		if(!buff.CanRead(len * sizeof(wchar))) return false;
		buff.ReadRaw(len * sizeof(wchar));
	}

	// trailing u32 (client-generated token / reserved)
	return buff.CanRead(sizeof(u32));
}

// ConfirmGatewayInfo: two wide strings (builder Send_ConfirmGatewayInfo).
template<>
inline bool ValidatePacket<Cl::CQ_SetIspName>(const void* packetData, i32 packetSize)
{
	ConstBuffer buff(packetData, packetSize);
	for(int i = 0; i < 2; i++) {
		if(!buff.CanRead(sizeof(u16))) return false;
		const u16 len = buff.Read<u16>();
		if(!buff.CanRead(len * sizeof(wchar))) return false;
		buff.ReadRaw(len * sizeof(wchar));
	}
	return true;
}

// EnterQueue: u32 + u32 + VEC<6B wire> + VEC<10B wire> (ping results).
template<>
inline bool ValidatePacket<Cl::CQ_EnterWaitingQueue>(const void* packetData, i32 packetSize)
{
	ConstBuffer buff(packetData, packetSize);
	if(!buff.CanRead(8)) return false;
	buff.ReadRaw(8);
	if(!buff.CanRead(2)) return false;
	const u16 c1 = buff.Read<u16>();
	if(!buff.CanRead(c1 * 6)) return false;
	buff.ReadRaw(c1 * 6);
	if(!buff.CanRead(2)) return false;
	const u16 c2 = buff.Read<u16>();
	if(!buff.CanRead(c2 * 10)) return false;
	buff.ReadRaw(c2 * 10);
	return true;
}
