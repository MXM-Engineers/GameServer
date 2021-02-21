#pragma once
#include "base.h"
#include <EASTL/fixed_string.h>

typedef eastl::fixed_string<wchar,64,true> WideString;
typedef eastl::fixed_string<char,32,false> FixedStr32;


typedef eastl::fixed_string<wchar,512,true> Path;

void PathSet(Path& path, const wchar* str);
void PathAppend(Path& path, const wchar* app);

u8* FileOpenAndReadAll(const wchar* filename, i32* pOutSize);

u32 RandUint();

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
