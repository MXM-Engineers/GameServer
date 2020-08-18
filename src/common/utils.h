#pragma once
#include "base.h"

template<u32 CAPACITY_>
struct WStringT
{
	enum
	{
		CAPACITY = CAPACITY_
	};

	wchar data[CAPACITY] = {0};
	i32 length = 0;

	void Set(const wchar* str, i32 len = -1)
	{
		if(len == -1) wcslen(str);
		memmove(data, str, sizeof(wchar) * len);
	}

	const wchar* Data()
	{
		return data;
	}
};

typedef WStringT<256> WString;
