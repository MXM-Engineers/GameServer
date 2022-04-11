#pragma once
#include "base.h"
#include <EASTL/fixed_string.h>
#include <EASTL/fixed_list.h>
#include <EASTL/fixed_map.h>
#include <EASTL/fixed_hash_map.h>
#include <EASTL/array.h>

typedef eastl::fixed_string<wchar,64,true> WideString;
typedef eastl::fixed_string<char,32,false> FixedStr32;
typedef eastl::fixed_string<char,64,false> FixedStr64;

typedef eastl::fixed_string<wchar,512,true> Path;

void PathSet(Path& path, const wchar* str);
void PathAppend(Path& path, const wchar* app);

u8* FileOpenAndReadAll(const wchar* filename, i32* pOutSize);

u32 RandUint();
f64 Randf01();
i32 RandInt(i32 vmin, i32 vmax);

template<int DEST_CAP, int SRC_CAP>
inline void StrConv(eastl::fixed_string<char,DEST_CAP,false>* dest, const eastl::fixed_string<wchar, SRC_CAP>& src)
{
	char buff[DEST_CAP];
	char* destStart = buff;
	char* destEnd = buff + DEST_CAP;
	const wchar* srcStart = src.cbegin();
	const wchar* srcEnd = src.end();
	eastl::DecodePart(srcStart, srcEnd, destStart, destEnd);
	buff[src.length()] = 0;
	*dest = buff;
}

template<typename T>
using ListItT = eastl::ListIterator<T, T*, T&>;

template<typename T>
using ListConstItT = eastl::ListIterator<T, const T*, const T&>;

template<class OutputIterator>
void StringSplit(const char* str, const int len, const char separator, OutputIterator out, const int maxCount)
{
	int start = 0;
	int outCount = 0;
	for(int i = 0; i < len && outCount < maxCount; i++) {
		if(str[i] == separator) {
			*out = eastl::string_view(str + start, i - start);
			++out;
			outCount++;
			start = i+1;
		}
	}

	if(outCount < maxCount && len - start > 0) {
		*out = eastl::string_view(str + start, len - start);
	}
}

inline bool StringViewEquals(const eastl::string_view& sv, const char* str)
{
	const int len = strlen(str);
	if(len != sv.length()) return false;
	return sv.compare(str) == 0;
}

// TODO: benchmark to find optimal bucket count
template<typename T1, typename T2, int EXPECTED_CAPACITY, bool GrowOnOverflow = false>
using hash_map = eastl::fixed_hash_map<T1 ,T2, EXPECTED_CAPACITY, 2, GrowOnOverflow>;

// LOCAL_MIN: included
// LOCAL_MAX: excluded
template<typename LocalID, typename GlobalID, LocalID LOCAL_MIN, LocalID LOCAL_MAX, LocalID LOCAL_INVALID = LocalID::INVALID>
struct LocalMapping
{
	enum {
		LOCAL_CAPACITY = (i64)LOCAL_MAX - (i64)LOCAL_MIN
	};

	hash_map<GlobalID,i32,LOCAL_CAPACITY> map;
	eastl::array<u8,LOCAL_CAPACITY> occupied = {0};

	LocalID Push(GlobalID globalID)
	{
		ASSERT_MSG(map.find(globalID) == map.end(), "GlobalID already in map");

		for(i32 i = 0; i < LOCAL_CAPACITY; i++) {
			if(!occupied[i]) {
				occupied[i] = 1;
				map.emplace(globalID, i);
				return LocalID((i64)LOCAL_MIN + i);
			}
		}

		ASSERT(0);
		return LOCAL_INVALID;
	}

	LocalID GetOrPush(GlobalID globalID)
	{
		auto found = map.find(globalID);
		if(found == map.end()) return Push(globalID);
		return LocalID((i64)LOCAL_MIN + found->second);
	}

	LocalID Get(GlobalID globalID) const
	{
		auto found = map.find(globalID);
		ASSERT_MSG(found != map.end(), "LocalID not found in map");
		return LocalID((i64)LOCAL_MIN + found->second);
	}

	LocalID TryGet(GlobalID globalID) const
	{
		auto found = map.find(globalID);
		if(found == map.end()) return LOCAL_INVALID;
		return LocalID((i64)LOCAL_MIN + found->second);
	}

	void Pop(GlobalID globalID)
	{
		auto found = map.find(globalID);
		ASSERT_MSG(found != map.end(), "GlobalID not found in map");
		occupied[found->second] = 0;
		map.erase(found);
	}

	void TryPop(GlobalID globalID)
	{
		auto found = map.find(globalID);
		if(found == map.end()) return;
		occupied[found->second] = 0;
		map.erase(found);
	}
};

void ThreadSetCoreAffinity(i32 coreID);
