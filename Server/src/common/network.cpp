#include "network.h"

const char* IpToString(const u8* ip)
{
	return FMT("%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}

const void SetIp(u8* ip, u8 i0, u8 i1, u8 i2, u8 i3)
{
	ip[0] = i0;
	ip[1] = i1;
	ip[2] = i2;
	ip[3] = i3;
}

const char* GetIpString(const sockaddr& addr)
{
	const sockaddr_in& in = *(sockaddr_in*)&addr;
	return FMT("%s:%d", inet_ntoa(in.sin_addr), in.sin_port);
}
