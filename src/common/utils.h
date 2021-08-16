#pragma once
#include "base.h"
#include <EASTL/fixed_string.h>
#include <EASTL/fixed_list.h>

typedef eastl::fixed_string<wchar,64,true> WideString;
typedef eastl::fixed_string<char,32,false> FixedStr32;
typedef eastl::fixed_string<char,64,false> FixedStr64;


typedef eastl::fixed_string<wchar,512,true> Path;

void PathSet(Path& path, const wchar* str);
void PathAppend(Path& path, const wchar* app);

u8* FileOpenAndReadAll(const wchar* filename, i32* pOutSize);

u32 RandUint();
f64 randf01();

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
