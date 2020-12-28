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

	struct StringSlice
	{
		const char* at;
		u32 len;

		inline bool Equals(const char* str) const
		{
			const i32 otherLen = strlen(str);
			if(otherLen != len) return false;
			return strncmp(at, str, otherLen) == 0;
		}
	};

	struct Vector3
	{
		f32 x,y,z;
	};

	struct Matrix3x3
	{
		f32 data[9];
	};

	eastl::fixed_vector<StringSlice,1024,true> stringList;
	eastl::fixed_vector<u16,1024,true> blockSizeList;

	bool ReadBlock_NiNode(ConstBuffer& buff, i32 blockID)
	{
		const u32 stringIndex = buff.Read<u32>();
		const StringSlice& str = (stringIndex != 0xFFFFFFFF) ? stringList[stringIndex] : StringSlice{"_", 1};

		const u32 numExtraDataList = buff.Read<u32>();
		eastl::fixed_vector<i32,128,true> refExtraDataList;
		for(int i = 0; i < numExtraDataList; i++) {
			refExtraDataList.push_back(buff.Read<i32>());
		}

		const i32 refController = buff.Read<i32>();
		const u16 flags = buff.Read<u16>();
		const Vector3 translation = buff.Read<Vector3>();
		const Matrix3x3 rotation = buff.Read<Matrix3x3>();
		const f32 scale = buff.Read<f32>();

		const u32 numProperties = buff.Read<u32>();
		eastl::fixed_vector<i32,128,true> refPropertyList;
		for(int i = 0; i < numProperties; i++) {
			refPropertyList.push_back(buff.Read<i32>());
		}

		const i32 refCollisionObject = buff.Read<i32>();

		const u32 numChildren = buff.Read<u32>();
		eastl::fixed_vector<i32,128,true> refChildrenList;
		for(int i = 0; i < numChildren; i++) {
			refChildrenList.push_back(buff.Read<i32>());
		}

		const u32 numEffects = buff.Read<u32>();

		LOG("	name = '%.*s'", str.len, str.at);
		LOG("	extraDataList(%d) = [", numExtraDataList);
		foreach(it, refExtraDataList) {
			LOG("		%d,", *it);
		}
		LOG("	]");
		LOG("	refController = %d", refController);
		LOG("	flags = %#x", flags);
		LOG("	translation = (%g, %g, %g)", translation.x, translation.y, translation.z);
		LOG("	rotation = (%.2f, %.2f, %.2f", rotation.data[0], rotation.data[1], rotation.data[2]);
		LOG("	            %.2f, %.2f, %.2f", rotation.data[3], rotation.data[4], rotation.data[5]);
		LOG("	            %.2f, %.2f, %.2f)", rotation.data[6], rotation.data[7], rotation.data[8]);
		LOG("	scale = %g", scale);
		LOG("	properties(%d) = [", numProperties);
		foreach(it, refPropertyList) {
			LOG("		%d,", *it);
		}
		LOG("	]");
		LOG("	refCollisionObject = %d", refCollisionObject);
		LOG("	children(%d) = [", numChildren);
		foreach(it, refChildrenList) {
			LOG("		%d,", *it);
		}
		LOG("	]");
		LOG("	numEffects = %u", numEffects);
		return true;
	}

	bool ReadBlock(ConstBuffer& buff, i32 blockID, const u16 blockType, const StringSlice& blockTypeStr)
	{
		bool r = true;

		LOG("Block (id=%d size=%d): {", blockID, blockSizeList[blockID]);
		LOG("	type = '%.*s' (%u)", blockTypeStr.len, blockTypeStr.at, blockType);

		if(blockTypeStr.Equals("NiNode")) {
			r = ReadBlock_NiNode(buff, blockID);
		}
		else {
			buff.ReadRaw(blockSizeList[blockID]);
			LOG("	[Unknown block -- skipping]");
		}

		LOG("}");
		return r;
	}

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

		const u16 numBlockTypes = buff.Read<u16>();
		eastl::fixed_vector<StringSlice,1024,true> blockTypeStringList;

		//LOG("Block Type String(%d): {", numBlockTypes);
		for(int i = 0; i < numBlockTypes; i++) {
			const u32 len = buff.Read<u32>();
			const char* str = (char*)buff.ReadRaw(len);
			blockTypeStringList.push_back({str, len});
			//LOG("'%.*s'", len, str);
		}
		//LOG("}");

		//LOG("Block Type Index: {");
		eastl::fixed_vector<u16,1024,true> blockTypeIndexList;
		for(int i = 0; i < numBlocks; i++) {
			const u16 blockTypeIndex = buff.Read<u16>();
			blockTypeIndexList.push_back(blockTypeIndex);
			//LOG("%d", blockTypeIndex);
		}
		//LOG("}");

		//LOG("Block Size: {");
		for(int i = 0; i < numBlocks; i++) {
			const u32 blockSize = buff.Read<u32>();
			blockSizeList.push_back(blockSize);
			//LOG("%u", blockSize);
		}
		//LOG("}");

		const u32 numStrings = buff.Read<u32>();
		const u32 maxStringLength = buff.Read<u32>();
		//LOG("Strings(count=%d maxLen=%d): {", numStrings, maxStringLength);
		for(int i = 0; i < numStrings; i++) {
			const u32 len = buff.Read<u32>();
			const char* str = (char*)buff.ReadRaw(len);
			stringList.push_back({str, len});
			//LOG("'%.*s'", len, str);
		}
		//LOG("}");

		const u32 numGroups = buff.Read<u32>();
		LOG("numGroups = %u", numGroups);

		for(int i = 0; i < numBlocks; i++) {
			const u16 blockTypeIndex = blockTypeIndexList[i];
			const StringSlice& blockTypeStr = (blockTypeIndex < numBlockTypes) ? blockTypeStringList[blockTypeIndex] : StringSlice{"_", 1};

			bool r = ReadBlock(buff, i, blockTypeIndex, blockTypeStr);
			if(!r) {
				LOG("ERROR: failed to read block at <%d>", buff.GetCursorPos());
				return false;
			}
		}

		LOG("remainingSize = %d", buff.RemainingDataSize());

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
