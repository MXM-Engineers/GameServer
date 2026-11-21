#pragma once
#include <mxm/account_progress.h>

template<typename Writer>
inline void WriteInventoryItem(Writer& packet, const AccountItem& item)
{
	packet.template Write<u32>(item.uid);
	packet.template Write<u8>(item.invenType);
	packet.template Write<i32>(item.slot);
	packet.template Write<i32>(item.docIndex);
	packet.template Write<i32>(item.count);
	packet.template Write<i32>(item.docIndex == 0 ? 0 : -1);
	packet.template Write<u8>(0);
	packet.template Write<i64>(0);
	packet.template Write<u16>(0);
}
