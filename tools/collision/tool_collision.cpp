#include <common/base.h>
#include <EAStdC/EAString.h>

#ifdef CONF_DEBUG
	#define DBG LOG
#else
	#define DBG
#endif

template<int CAPACITY>
using FixedStr = eastl::fixed_string<char,CAPACITY,false>;

template<typename T>
inline T littleToBigEndian(T in)
{
	T out = 0x0;
	for(size_t i = 0; i < sizeof(T); i++) {
		out |= ((in >> (i * 8)) & 0xFF) << ((sizeof(T) - i - 1) * 8);
	}
	return out;
}

inline u32 littleToBigEndian(u32 in)
{
	u32 b0 = (in & 0x000000ff) << 24u;
	u32 b1 = (in & 0x0000ff00) << 8u;
	u32 b2 = (in & 0x00ff0000) >> 8u;
	u32 b3 = (in & 0xff000000) >> 24u;;
	return b0 | b1 | b2 | b3;
}

struct CollisionNifReader
{
	const u8* fileData;
	const i32 fileSize;

	bool ReadFile()
	{
		ConstBuffer buff(fileData, fileSize);

		// header string
		FixedStr<80> headerStr(buff.Read<char[38]>(), 38);
		LOG("header = '%s'", headerStr.data());

		char headerEnd = buff.Read<char>();
		if(headerEnd != '\n') {
			return false;
		}

		const u32 fileVersion = buff.Read<u32>();
		LOG("fileVersion = %#x", fileVersion);

		const u8 littleEndian = buff.Read<u8>();
		if(littleEndian != 1) {
			LOG("ERROR: file is big endian");
			return false;
		}

		const u32 userVersion = buff.Read<u32>();
		const u32 numBlocks = buff.Read<u32>();
		LOG("numBlocks = %u", numBlocks);

		const u16 typeStringCount = buff.Read<u16>();
		LOG("Block Type String(%d): {", typeStringCount);
		for(int i = 0; i < typeStringCount; i++) {
			const u32 len = buff.Read<u32>();
			const char* str = (char*)buff.ReadRaw(len);
			LOG("'%.*s'", len, str);
		}
		LOG("}");

		LOG("Block Type Index: {");
		for(int i = 0; i < numBlocks; i++) {
			const u16 blockTypeIndex = buff.Read<u16>();
			LOG("%d", blockTypeIndex);
		}
		LOG("}");

		LOG("Block Size: {");
		for(int i = 0; i < numBlocks; i++) {
			const u32 blockSize = buff.Read<u32>();
			LOG("%u", blockSize);
		}
		LOG("}");

		return true;
	}
};

int main(int argc, char** argv)
{
	ASSERT(argc == 2);

	LogInit("col.log");

	const char* inputFilename = argv[1];

	i32 fileSize;
	const u8* fileData = fileOpenAndReadAll(inputFilename, &fileSize);
	if(!fileData) {
		LOG("ERROR: failed to open '%s'", inputFilename);
		return 1;
	}

	LOG("'%s'", inputFilename);
	LOG("fileSize = %d", fileSize);

	CollisionNifReader reader = { fileData, fileSize };
	if(!reader.ReadFile()) {
		LOG("ERROR: failed to process '%s'", inputFilename);
		return 1;
	}

	return 0;
}
