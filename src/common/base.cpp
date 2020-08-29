#include "base.h"
#include <time.h>
#include <EAThread/eathread.h>
#include <EASTL/array.h>

#ifdef _WIN32
	#include <windows.h> // OutputDebugStringA
#endif

FILE* g_LogFile;
const char* g_LogFileName;

struct Logger
{
	Mutex mutex;

	~Logger() {
		fclose(g_LogFile);
	}
};

static Logger g_Logger;

void LogInit(const char* name)
{
	g_LogFileName = name;
	g_LogFile = fopen(g_LogFileName, "wb");
	ASSERT(g_LogFile);
}

void __Logf(const char* fmt, ...)
{
	char buff[4096];
	char final[4096];

	va_list list;
	va_start(list, fmt);
	vsnprintf(buff, sizeof(buff), fmt, list);
	va_end(list);

	// this one is faster but gives a big number
	/*
	EA::Thread::ThreadUniqueId threadID;
	EAThreadGetUniqueId(threadID);
	*/
	EA::Thread::ThreadId threadID = EA::Thread::GetThreadId();

	snprintf(final, sizeof(final), "[%x] %s", (i32)(intptr_t)threadID, buff);

	const LockGuard lock(g_Logger.mutex);
	printf(final);
	fprintf(g_LogFile, final);

#ifdef _WIN32
#ifdef CONF_DEBUG
	if(IsDebuggerPresent()) {
		OutputDebugStringA(final);
	}
#endif
#endif
}

// EASTL new operators
void* operator new[](size_t size, const char* name, int flags, unsigned debugFlags, const char* file, int line)
{
	return memAlloc(size);
}

void* operator new[](size_t size, size_t alignment, size_t offset, const char* name, int flags, unsigned debugFlags, const char* file, int line)
{
	// TODO: align
	return memAlloc(size);
}
const char* _TempStrFormat(const char* fmt, ...)
{
	thread_local char buff[4096];

	va_list list;
	va_start(list, fmt);
	vsnprintf(buff, sizeof(buff), fmt, list);
	va_end(list);

	return buff;
}

const wchar* _TempWideStrFormat(const wchar* fmt, ...)
{
	thread_local wchar buff[4096];

	va_list list;
	va_start(list, fmt);
	vswprintf(buff, ARRAY_COUNT(buff), fmt, list);
	va_end(list);

	return buff;
}


#define SOKOL_IMPL
#include "sokol_time.h"

static timept g_StartTime;

void TimeInit()
{
	stm_setup();
	g_StartTime = TimeNow();
}

timept TimeNow()
{
	return stm_now();
}

timept TimeRelNow()
{
	return stm_since(g_StartTime);
}

f64 TimeDiffSec(timept t0, timept t1)
{
	return stm_sec(stm_diff(t1, t0));
}

f64 TimeDurationMs(timept t0, timept t1)
{
	return stm_ms(stm_diff(t1, t0));
}

f64 TimeDiffSec(timept diff)
{
	return stm_sec(diff);
}

f64 TimeDiffMs(timept diff)
{
	return stm_ms(diff);
}

f64 TimeDurationSinceSec(timept t0)
{
	return stm_sec(stm_since(t0));
}

f64 TimeDurationSinceMs(timept t0)
{
	return stm_ms(stm_since(t0));
}

static void EASTL_AssertionFailed(const char* expression, void* pContext)
{
	__assertion_failed(expression, "eastl", 0);
}

struct EASTLInit
{
	EASTLInit() {
		eastl::SetAssertionFailureFunction(EASTL_AssertionFailed, nullptr);
		EA::Thread::SetAssertionFailureFunction(EASTL_AssertionFailed, nullptr);
	}
};

static EASTLInit;
