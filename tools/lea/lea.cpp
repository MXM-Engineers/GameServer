#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#define LOG(fmt, ...) printf(fmt "\n", __VA_ARGS__);
#define ASSERT(cond) assert(cond)

#ifdef CONF_DEBUG
	#define DBG LOG
#else
	#define DBG
#endif

typedef uint8_t u8;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;

static u8* fileOpenAndReadAll(const char* filename, i32* pOutSize)
{
	FILE* f = fopen(filename, "rb");
	if(f) {
		fseek(f, 0, SEEK_END);
		int size = ftell(f);
		fseek(f, 0, SEEK_SET);

		u8* buff = (u8*)malloc(size + 1);
		fread(buff, size, 1, f);
		buff[size] = 0;
		*pOutSize = size;

		fclose(f);
		return buff;
	}

	return nullptr;
}

struct NetHeader
{
	u16 size;
	u16 netID;
};

const u32 PROTOCOL_CRC = 0x28845199;
const u32 ERROR_CRC = 0x93899e2c;
const u32 VERSION = 0xb4381e;

struct Key16
{
	u32 protocol;
	u32 error;
	u32 version;
	u32 portPlusIp;

	void Create(i32 ip[4], i32 port)
	{
		protocol = PROTOCOL_CRC;
		error = ERROR_CRC;
		version = VERSION;
		protocol = PROTOCOL_CRC;

		union FlatIP {
			u8 b[4];
			u32 whole;
		};

		FlatIP flat = { (u8)ip[3], (u8)ip[2], (u8)ip[1], (u8)ip[0] };
		portPlusIp = (u16)port + flat.whole;
	}

	void Print()
	{
		const u8* cur = (u8*)this;
		for(int i = 0; i < sizeof(*this); i++) {
			printf("%02X ", cur[i]);
		}
		printf("\n");
	}
};

int main(int argc, char** argv)
{
	ASSERT(argc == 3);

	const char* filePath = argv[1];
	const char* ipPortString = argv[2];
	DBG("file = '%s'", filePath);
	DBG("ip:port = '%s'", ipPortString);

	i32 ip[4];
	i32 port;

	bool r = sscanf(ipPortString, "%d.%d.%d.%d:%d", &ip[0], &ip[1], &ip[2], &ip[3], &port) == 5;
	ASSERT(r);

	DBG("ip:port = %d.%d.%d.%d:%d", ip[0], ip[1], ip[2], ip[3], port);

	// create 16B key
	Key16 key16;
	key16.Create(ip, port);
	key16.Print();

	i32 fileSize;
	u8* fileData = fileOpenAndReadAll(filePath, &fileSize);
	ASSERT(fileData);

	const NetHeader& header = *(NetHeader*)fileData;
	ASSERT(fileSize == header.size);

	return 0;
}
