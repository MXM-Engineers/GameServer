#pragma once
#include "base.h"
#include <EASTL/fixed_string.h>

typedef eastl::fixed_string<wchar,64,true> WideString;


typedef eastl::fixed_string<wchar,512,true> Path;

void PathSet(Path& path, const wchar* str);
void PathAppend(Path& path, const wchar* app);

u8* FileOpenAndReadAll(const wchar* filename, i32* pOutSize);
