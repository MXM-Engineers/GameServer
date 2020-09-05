#pragma once

#ifdef CONF_LINUX
	#define PUSH_PACKED _Pragma("pack(push, 1)")
	#define POP_PACKED _Pragma("pack(pop)")
	#define STRINGIFY(a) #a
	#define FUNCTION_STR STRINGIFY(__FUNCTION__)

	typedef wchar_t wchar;
#endif

#ifdef CONF_WINDOWS
	#include <windows.h>

	#define PUSH_PACKED __pragma(pack(push, 1))
	#define POP_PACKED __pragma(pack(pop))
	#define FUNCTION_STR __FUNCTION__

	typedef wchar_t wchar;
#endif

void DbgBreak();

typedef void(*Func_CloseSignalHandler)();
bool SetCloseSignalHandler(Func_CloseSignalHandler func);

void MakeDirectory(const char* path);
