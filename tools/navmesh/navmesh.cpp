#include <common/base.h>
#include <EAStdC/EAString.h>

#ifdef CONF_DEBUG
	#define DBG LOG
#else
	#define DBG
#endif

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

struct NavMeshReader
{
	struct Tag
	{
		u16 type;
		u16 size;
		union {
			char name[4];
			u32 ID;
		};

		inline bool CompareName(const char* cmp) const {
			return EA::StdC::Strncmp(name, cmp, 4) == 0;
		}
	};

	const u8* fileData;
	const i32 fileSize;

	bool ProcessTag_TAG0(const Tag& tag, ConstBuffer& buff)
	{
		auto rt = ReadTag(buff);
		if(!rt.first.CompareName("SDKV") || !rt.second) {
			LOG("ERROR: expected 'SDKV' tag");
			return false;
		}

		rt = ReadTag(buff);
		if(!rt.first.CompareName("DATA") || !rt.second) {
			LOG("ERROR: expected 'DATA' tag");
			return false;
		}
		return true;
	}

	bool ProcessTag_SDKV(const Tag& tag, ConstBuffer& buff)
	{
		const char* versionStr = (char*)buff.ReadRaw(tag.size);
		LOG("version = '%.*s'", tag.size, versionStr);
		return true;
	}

	bool ProcessTag(const Tag& tag, ConstBuffer& buff)
	{
		LOG("[%.4s : %u] {", tag.name, tag.size);

		if(!buff.CanRead(tag.size - 8)) {
			LOG("ERROR: tag size is too large (%u > %d)", tag.size, buff.RemainingDataSize());
			return false;
		}

		ConstBuffer tagBuff(buff.ReadRaw(tag.size), tag.size);

		bool r = false;
		switch(tag.ID) {
			case 0x30474154: r = ProcessTag_TAG0(tag, tagBuff); break;
			case 0x564b4453: r = ProcessTag_SDKV(tag, tagBuff); break;
			default: return false;
		}

		LOG("}");
		return true;
	}

	eastl::pair<Tag,bool> ReadTag(ConstBuffer& buff)
	{
		Tag tag;
		u32 typeSize = buff.Read<u32>();
		tag.type = typeSize & 0xFF;
		typeSize &= 0xFFFFFF00; // zero out type
		tag.size = littleToBigEndian(typeSize) - 8;

		tag.ID = buff.Read<u32>();
		return {tag, ProcessTag(tag, buff)};
	}

	bool ReadFile()
	{
		ConstBuffer buff(fileData, fileSize);
		auto rt = ReadTag(buff);
		const Tag& tag = rt.first;
		bool r = rt.second;

		if(!r || !tag.CompareName("TAG0")) {
			return false;
		}
		return true;
	}
};

int main(int argc, char** argv)
{
	ASSERT(argc == 2);

	LogInit("navmesh.log");

	const char* inputFilename = argv[1];

	i32 fileSize;
	const u8* fileData = fileOpenAndReadAll(inputFilename, &fileSize);
	if(!fileData) {
		LOG("ERROR: failed to open '%s'", inputFilename);
		return 1;
	}

	LOG("'%s'", inputFilename);
	LOG("fileSize = %d", fileSize);

	NavMeshReader reader = { fileData, fileSize };
	if(!reader.ReadFile()) {
		LOG("ERROR: failed to process '%s'", inputFilename);
		return 1;
	}

	return 0;
}
