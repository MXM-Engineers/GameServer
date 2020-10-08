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

	Key16(i32 ip[4], i32 port)
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
		LOG("Key:");
		const u8* cur = (u8*)this;
		for(int i = 0; i < sizeof(*this); i++) {
			printf("%02X ", cur[i]);
		}
		printf("\n");
	}
};

union Filter
{
	struct
	{
		Key16 key1;
		Key16 key2;
		u32 block1[193];
		u32 block2[193];
		u8 isInitialized;
		u8 _pad[7];
	} s;

	u8 data[0x630];

	static_assert(sizeof(s) == sizeof(data), "");

	Filter(Key16 key)
	{
		s.key1 = key;
		s.key2 = key;

		GenerateBlock(s.block1, &s.key1, sizeof(s.key1));
		GenerateBlock(s.block2, &s.key2, sizeof(s.key2));

		s.isInitialized = 1;
	}

	void Print()
	{
		LOG("Filter:");
		const u8* cur = (u8*)this;
		for(int i = 0; i < sizeof(*this); i++) {
			printf("%02X ", cur[i]);
			if(i % 16 == 15) printf("\n");
		}
		printf("\n");
	}

private:
	void GenerateBlock(u32 *out, const Key16 *key, u32 keySize)
	{
		i32 iVar1;
		u32 uVar2;

		iVar1 = key->protocol + 0xc3efe9db;
		uVar2 = iVar1 * 2 | (u32)(iVar1 < 0);
		*out = uVar2;
		iVar1 = uVar2 + 0x88c4d604;
		uVar2 = iVar1 * 2 | (u32)(iVar1 < 0);
		out[6] = uVar2;
		iVar1 = uVar2 + 0xe789f229;
		uVar2 = iVar1 * 2 | (u32)(iVar1 < 0);
		out[0xc] = uVar2;
		iVar1 = uVar2 + 0xc6f98763;
		uVar2 = iVar1 * 2 | (u32)(iVar1 < 0);
		out[0x12] = uVar2;
		iVar1 = uVar2 + 0x3efe9dbc;
		uVar2 = iVar1 * 2 | (u32)(iVar1 < 0);
		out[0x18] = uVar2;
		iVar1 = uVar2 + 0x8c4d6048;
		uVar2 = iVar1 * 2 | (u32)(iVar1 < 0);
		out[0x1e] = uVar2;
		iVar1 = uVar2 + 0x789f229e;
		uVar2 = iVar1 * 2 | (u32)(iVar1 < 0);
		out[0x24] = uVar2;
		iVar1 = uVar2 + 0x6f98763c;
		uVar2 = iVar1 * 2 | (u32)(iVar1 < 0);
		out[0x2a] = uVar2;
		iVar1 = uVar2 + 0xefe9dbc3;
		uVar2 = iVar1 * 2 | (u32)(iVar1 < 0);
		out[0x30] = uVar2;
		iVar1 = uVar2 + 0xc4d60488;
		uVar2 = iVar1 * 2 | (u32)(iVar1 < 0);
		out[0x36] = uVar2;
		iVar1 = uVar2 + 0x89f229e7;
		uVar2 = iVar1 * 2 | (u32)(iVar1 < 0);
		out[0x3c] = uVar2;
		iVar1 = uVar2 + 0xf98763c6;
		uVar2 = iVar1 * 2 | (u32)(iVar1 < 0);
		out[0x42] = uVar2;
		iVar1 = uVar2 + 0xfe9dbc3e;
		uVar2 = iVar1 * 2 | (u32)(iVar1 < 0);
		out[0x48] = uVar2;
		iVar1 = uVar2 + 0x4d60488c;
		uVar2 = iVar1 * 2 | (u32)(iVar1 < 0);
		out[0x4e] = uVar2;
		iVar1 = uVar2 + 0x9f229e78;
		uVar2 = iVar1 * 2 | (u32)(iVar1 < 0);
		out[0x54] = uVar2;
		iVar1 = uVar2 + 0x98763c6f;
		uVar2 = iVar1 * 2 | (u32)(iVar1 < 0);
		out[0x5a] = uVar2;
		iVar1 = uVar2 + 0xe9dbc3ef;
		uVar2 = iVar1 * 2 | (u32)(iVar1 < 0);
		out[0x60] = uVar2;
		iVar1 = uVar2 + 0xd60488c4;
		uVar2 = iVar1 * 2 | (u32)(iVar1 < 0);
		out[0x66] = uVar2;
		iVar1 = uVar2 + 0xf229e789;
		uVar2 = iVar1 * 2 | (u32)(iVar1 < 0);
		out[0x6c] = uVar2;
		iVar1 = uVar2 + 0x8763c6f9;
		uVar2 = iVar1 * 2 | (u32)(iVar1 < 0);
		out[0x72] = uVar2;
		iVar1 = uVar2 + 0x9dbc3efe;
		uVar2 = iVar1 * 2 | (u32)(iVar1 < 0);
		out[0x78] = uVar2;
		iVar1 = uVar2 + 0x60488c4d;
		uVar2 = iVar1 * 2 | (u32)(iVar1 < 0);
		out[0x7e] = uVar2;
		iVar1 = uVar2 + 0x229e789f;
		uVar2 = iVar1 * 2 | (u32)(iVar1 < 0);
		out[0x84] = uVar2;
		iVar1 = uVar2 + 0x763c6f98;
		out[0x8a] = iVar1 * 2 | (u32)(iVar1 < 0);
		uVar2 = key->error + 0x87dfd3b7;
		uVar2 = uVar2 * 8 | uVar2 >> 0x1d;
		out[5] = uVar2;
		out[3] = uVar2;
		out[1] = uVar2;
		uVar2 = (uVar2 + 0x1189ac09) * 8 | uVar2 + 0x1189ac09 >> 0x1d;
		out[0xb] = uVar2;
		out[9] = uVar2;
		out[7] = uVar2;
		uVar2 = (uVar2 + 0xcf13e453) * 8 | uVar2 + 0xcf13e453 >> 0x1d;
		out[0x11] = uVar2;
		out[0xf] = uVar2;
		out[0xd] = uVar2;
		uVar2 = (uVar2 + 0x8df30ec7) * 8 | uVar2 + 0x8df30ec7 >> 0x1d;
		out[0x17] = uVar2;
		out[0x15] = uVar2;
		out[0x13] = uVar2;
		uVar2 = (uVar2 + 0x7dfd3b78) * 8 | uVar2 + 0x7dfd3b78 >> 0x1d;
		out[0x1d] = uVar2;
		out[0x1b] = uVar2;
		out[0x19] = uVar2;
		uVar2 = (uVar2 + 0x189ac091) * 8 | uVar2 + 0x189ac091 >> 0x1d;
		out[0x23] = uVar2;
		out[0x21] = uVar2;
		out[0x1f] = uVar2;
		uVar2 = (uVar2 + 0xf13e453c) * 8 | uVar2 + 0xf13e453c >> 0x1d;
		out[0x29] = uVar2;
		out[0x27] = uVar2;
		out[0x25] = uVar2;
		uVar2 = (uVar2 + 0xdf30ec78) * 8 | uVar2 + 0xdf30ec78 >> 0x1d;
		out[0x2f] = uVar2;
		out[0x2d] = uVar2;
		out[0x2b] = uVar2;
		uVar2 = (uVar2 + 0xdfd3b787) * 8 | uVar2 + 0xdfd3b787 >> 0x1d;
		out[0x35] = uVar2;
		out[0x33] = uVar2;
		out[0x31] = uVar2;
		uVar2 = (uVar2 + 0x89ac0911) * 8 | uVar2 + 0x89ac0911 >> 0x1d;
		out[0x3b] = uVar2;
		out[0x39] = uVar2;
		out[0x37] = uVar2;
		uVar2 = (uVar2 + 0x13e453cf) * 8 | uVar2 + 0x13e453cf >> 0x1d;
		out[0x41] = uVar2;
		out[0x3f] = uVar2;
		out[0x3d] = uVar2;
		uVar2 = (uVar2 + 0xf30ec78d) * 8 | uVar2 + 0xf30ec78d >> 0x1d;
		out[0x47] = uVar2;
		out[0x45] = uVar2;
		out[0x43] = uVar2;
		uVar2 = (uVar2 + 0xfd3b787d) * 8 | uVar2 + 0xfd3b787d >> 0x1d;
		out[0x4d] = uVar2;
		out[0x4b] = uVar2;
		out[0x49] = uVar2;
		uVar2 = (uVar2 + 0x9ac09118) * 8 | uVar2 + 0x9ac09118 >> 0x1d;
		out[0x53] = uVar2;
		out[0x51] = uVar2;
		out[0x4f] = uVar2;
		uVar2 = (uVar2 + 0x3e453cf1) * 8 | uVar2 + 0x3e453cf1 >> 0x1d;
		out[0x59] = uVar2;
		out[0x57] = uVar2;
		out[0x55] = uVar2;
		uVar2 = (uVar2 + 0x30ec78df) * 8 | uVar2 + 0x30ec78df >> 0x1d;
		out[0x5f] = uVar2;
		out[0x5d] = uVar2;
		out[0x5b] = uVar2;
		uVar2 = (uVar2 + 0xd3b787df) * 8 | uVar2 + 0xd3b787df >> 0x1d;
		out[0x65] = uVar2;
		out[99] = uVar2;
		out[0x61] = uVar2;
		uVar2 = (uVar2 + 0xac091189) * 8 | uVar2 + 0xac091189 >> 0x1d;
		out[0x6b] = uVar2;
		out[0x69] = uVar2;
		out[0x67] = uVar2;
		uVar2 = (uVar2 + 0xe453cf13) * 8 | uVar2 + 0xe453cf13 >> 0x1d;
		out[0x71] = uVar2;
		out[0x6f] = uVar2;
		out[0x6d] = uVar2;
		uVar2 = (uVar2 + 0xec78df3) * 8 | uVar2 + 0xec78df3 >> 0x1d;
		out[0x77] = uVar2;
		out[0x75] = uVar2;
		out[0x73] = uVar2;
		uVar2 = (uVar2 + 0x3b787dfd) * 8 | uVar2 + 0x3b787dfd >> 0x1d;
		out[0x7d] = uVar2;
		out[0x7b] = uVar2;
		out[0x79] = uVar2;
		uVar2 = (uVar2 + 0xc091189a) * 8 | uVar2 + 0xc091189a >> 0x1d;
		out[0x83] = uVar2;
		out[0x81] = uVar2;
		out[0x7f] = uVar2;
		uVar2 = (uVar2 + 0x453cf13e) * 8 | uVar2 + 0x453cf13e >> 0x1d;
		out[0x89] = uVar2;
		out[0x87] = uVar2;
		out[0x85] = uVar2;
		uVar2 = (uVar2 + 0xec78df30) * 8 | uVar2 + 0xec78df30 >> 0x1d;
		out[0x8f] = uVar2;
		out[0x8d] = uVar2;
		out[0x8b] = uVar2;
		uVar2 = key->version + 0xfbfa76f;
		uVar2 = uVar2 * 0x40 | uVar2 >> 0x1a;
		out[2] = uVar2;
		uVar2 = uVar2 + 0x23135812;
		uVar2 = uVar2 * 0x40 | uVar2 >> 0x1a;
		out[8] = uVar2;
		uVar2 = uVar2 + 0x9e27c8a7;
		uVar2 = uVar2 * 0x40 | uVar2 >> 0x1a;
		out[0xe] = uVar2;
		uVar2 = uVar2 + 0x1be61d8f;
		uVar2 = uVar2 * 0x40 | uVar2 >> 0x1a;
		out[0x14] = uVar2;
		uVar2 = uVar2 + 0xfbfa76f0;
		uVar2 = uVar2 * 0x40 | uVar2 >> 0x1a;
		out[0x1a] = uVar2;
		uVar2 = uVar2 + 0x31358122;
		uVar2 = uVar2 * 0x40 | uVar2 >> 0x1a;
		out[0x20] = uVar2;
		uVar2 = uVar2 + 0xe27c8a79;
		uVar2 = uVar2 * 0x40 | uVar2 >> 0x1a;
		out[0x26] = uVar2;
		uVar2 = uVar2 + 0xbe61d8f1;
		uVar2 = uVar2 * 0x40 | uVar2 >> 0x1a;
		out[0x2c] = uVar2;
		uVar2 = uVar2 + 0xbfa76f0f;
		uVar2 = uVar2 * 0x40 | uVar2 >> 0x1a;
		out[0x32] = uVar2;
		uVar2 = uVar2 + 0x13581223;
		uVar2 = uVar2 * 0x40 | uVar2 >> 0x1a;
		out[0x38] = uVar2;
		uVar2 = uVar2 + 0x27c8a79e;
		uVar2 = uVar2 * 0x40 | uVar2 >> 0x1a;
		out[0x3e] = uVar2;
		uVar2 = uVar2 + 0xe61d8f1b;
		uVar2 = uVar2 * 0x40 | uVar2 >> 0x1a;
		out[0x44] = uVar2;
		uVar2 = uVar2 + 0xfa76f0fb;
		uVar2 = uVar2 * 0x40 | uVar2 >> 0x1a;
		out[0x4a] = uVar2;
		uVar2 = uVar2 + 0x35812231;
		uVar2 = uVar2 * 0x40 | uVar2 >> 0x1a;
		out[0x50] = uVar2;
		uVar2 = uVar2 + 0x7c8a79e2;
		uVar2 = uVar2 * 0x40 | uVar2 >> 0x1a;
		out[0x56] = uVar2;
		uVar2 = uVar2 + 0x61d8f1be;
		uVar2 = uVar2 * 0x40 | uVar2 >> 0x1a;
		out[0x5c] = uVar2;
		uVar2 = uVar2 + 0xa76f0fbf;
		uVar2 = uVar2 * 0x40 | uVar2 >> 0x1a;
		out[0x62] = uVar2;
		uVar2 = uVar2 + 0x58122313;
		uVar2 = uVar2 * 0x40 | uVar2 >> 0x1a;
		out[0x68] = uVar2;
		uVar2 = uVar2 + 0xc8a79e27;
		uVar2 = uVar2 * 0x40 | uVar2 >> 0x1a;
		out[0x6e] = uVar2;
		uVar2 = uVar2 + 0x1d8f1be6;
		uVar2 = uVar2 * 0x40 | uVar2 >> 0x1a;
		out[0x74] = uVar2;
		uVar2 = uVar2 + 0x76f0fbfa;
		uVar2 = uVar2 * 0x40 | uVar2 >> 0x1a;
		out[0x7a] = uVar2;
		uVar2 = uVar2 + 0x81223135;
		uVar2 = uVar2 * 0x40 | uVar2 >> 0x1a;
		out[0x80] = uVar2;
		uVar2 = uVar2 + 0x8a79e27c;
		uVar2 = uVar2 * 0x40 | uVar2 >> 0x1a;
		out[0x86] = uVar2;
		uVar2 = uVar2 + 0xd8f1be61;
		out[0x8c] = uVar2 * 0x40 | uVar2 >> 0x1a;
		uVar2 = key->portPlusIp + 0x1f7f4ede;
		uVar2 = uVar2 * 0x800 | uVar2 >> 0x15;
		out[4] = uVar2;
		uVar2 = uVar2 + 0x4626b024;
		uVar2 = uVar2 * 0x800 | uVar2 >> 0x15;
		out[10] = uVar2;
		uVar2 = uVar2 + 0x3c4f914f;
		uVar2 = uVar2 * 0x800 | uVar2 >> 0x15;
		out[0x10] = uVar2;
		uVar2 = uVar2 + 0x37cc3b1e;
		uVar2 = uVar2 * 0x800 | uVar2 >> 0x15;
		out[0x16] = uVar2;
		uVar2 = uVar2 + 0xf7f4ede1;
		uVar2 = uVar2 * 0x800 | uVar2 >> 0x15;
		out[0x1c] = uVar2;
		uVar2 = uVar2 + 0x626b0244;
		uVar2 = uVar2 * 0x800 | uVar2 >> 0x15;
		out[0x22] = uVar2;
		uVar2 = uVar2 + 0xc4f914f3;
		uVar2 = uVar2 * 0x800 | uVar2 >> 0x15;
		out[0x28] = uVar2;
		uVar2 = uVar2 + 0x7cc3b1e3;
		uVar2 = uVar2 * 0x800 | uVar2 >> 0x15;
		out[0x2e] = uVar2;
		uVar2 = uVar2 + 0x7f4ede1f;
		uVar2 = uVar2 * 0x800 | uVar2 >> 0x15;
		out[0x34] = uVar2;
		uVar2 = uVar2 + 0x26b02446;
		uVar2 = uVar2 * 0x800 | uVar2 >> 0x15;
		out[0x3a] = uVar2;
		uVar2 = uVar2 + 0x4f914f3c;
		uVar2 = uVar2 * 0x800 | uVar2 >> 0x15;
		out[0x40] = uVar2;
		uVar2 = uVar2 + 0xcc3b1e37;
		uVar2 = uVar2 * 0x800 | uVar2 >> 0x15;
		out[0x46] = uVar2;
		uVar2 = uVar2 + 0xf4ede1f7;
		uVar2 = uVar2 * 0x800 | uVar2 >> 0x15;
		out[0x4c] = uVar2;
		uVar2 = uVar2 + 0x6b024462;
		uVar2 = uVar2 * 0x800 | uVar2 >> 0x15;
		out[0x52] = uVar2;
		uVar2 = uVar2 + 0xf914f3c4;
		uVar2 = uVar2 * 0x800 | uVar2 >> 0x15;
		out[0x58] = uVar2;
		uVar2 = uVar2 + 0xc3b1e37c;
		uVar2 = uVar2 * 0x800 | uVar2 >> 0x15;
		out[0x5e] = uVar2;
		uVar2 = uVar2 + 0x4ede1f7f;
		uVar2 = uVar2 * 0x800 | uVar2 >> 0x15;
		out[100] = uVar2;
		uVar2 = uVar2 + 0xb0244626;
		uVar2 = uVar2 * 0x800 | uVar2 >> 0x15;
		out[0x6a] = uVar2;
		uVar2 = uVar2 + 0x914f3c4f;
		uVar2 = uVar2 * 0x800 | uVar2 >> 0x15;
		out[0x70] = uVar2;
		uVar2 = uVar2 + 0x3b1e37cc;
		uVar2 = uVar2 * 0x800 | uVar2 >> 0x15;
		out[0x76] = uVar2;
		uVar2 = uVar2 + 0xede1f7f4;
		uVar2 = uVar2 * 0x800 | uVar2 >> 0x15;
		out[0x7c] = uVar2;
		uVar2 = uVar2 + 0x244626b;
		uVar2 = uVar2 * 0x800 | uVar2 >> 0x15;
		out[0x82] = uVar2;
		uVar2 = uVar2 + 0x14f3c4f9;
		uVar2 = uVar2 * 0x800 | uVar2 >> 0x15;
		out[0x88] = uVar2;
		uVar2 = uVar2 + 0xb1e37cc3;
		out[0x8e] = uVar2 * 0x800 | uVar2 >> 0x15;
		out[0xc0] = 0x18;
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
	Key16 key16(ip, port);
	key16.Print();

	Filter filter(key16);
	filter.Print();

	i32 fileSize;
	u8* fileData = fileOpenAndReadAll(filePath, &fileSize);
	ASSERT(fileData);

	const NetHeader& header = *(NetHeader*)fileData;
	ASSERT(fileSize == header.size);

	return 0;
}
