#include "protocol_version.h"
#include <common/logger.h>
#include <EAStdC/EAString.h>

const char* ClientVersionName(ClientVersion v)
{
	switch(v) {
		case ClientVersion::RETAIL: return "retail";
		case ClientVersion::ALPHA:  return "alpha";
		default: return "?";
	}
}

ClientVersion ClientVersionFromString(const char* str)
{
	if(EA::StdC::Stricmp(str, "alpha") == 0)  return ClientVersion::ALPHA;
	if(EA::StdC::Stricmp(str, "retail") == 0) return ClientVersion::RETAIL;
	WARN("unknown ClientVersion '%s', defaulting to retail", str);
	return ClientVersion::RETAIL;
}

ClientVersion DetectClientVersion(u32 protocolCRC, u32 errorCRC, u32 version, i32 packetSize)
{
	// Two independent signals, either is sufficient. The CRC triple is exact;
	// the 12-vs-13 byte size catches a build with different CRCs but the alpha's
	// (shorter) CQ_FirstHello.
	const bool crcMatch = (protocolCRC == ALPHA_PROTOCOL_CRC
	                    && errorCRC == ALPHA_ERROR_CRC
	                    && version == ALPHA_VERSION);
	const bool sizeMatch = (packetSize == ALPHA_FIRST_HELLO_SIZE);

	if(crcMatch || sizeMatch) {
		if(crcMatch != sizeMatch) {
			// Worth knowing about: an unrecorded build, or a changed CRC.
			LOG("[version] alpha detected on only one signal (crc=%d size=%d, "
			    "protocolCRC=%x errorCRC=%x version=%x size=%d)",
			    (i32)crcMatch, (i32)sizeMatch, protocolCRC, errorCRC, version, packetSize);
		}
		return ClientVersion::ALPHA;
	}
	return ClientVersion::RETAIL;
}
