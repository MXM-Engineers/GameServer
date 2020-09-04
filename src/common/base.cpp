#include "base.h"
#include <time.h>
#include <EAThread/eathread.h>
#include <EASTL/array.h>

#ifdef CONF_WINDOWS
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
	char final[4096+100];

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
	const i32 len = strlen(final);

	const LockGuard lock(g_Logger.mutex);
	fwrite(final, 1, len, stdout);
	fwrite(final, 1, len, g_LogFile);

#ifdef CONF_WINDOWS
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

static Time g_StartTime;

void TimeInit()
{
	stm_setup();
	g_StartTime = TimeNow();
}

Time TimeNow()
{
	return (Time)stm_now();
}

Time TimeRelNow()
{
	return (Time)stm_since((u64)g_StartTime);
}

Time TimeDiff(Time start, Time end)
{
	return (Time)(stm_diff((u64)end, (u64)start));
}

f64 TimeDiffSec(Time t0, Time t1)
{
	return stm_sec(stm_diff((u64)t1, (u64)t0));
}

f64 TimeDurationMs(Time t0, Time t1)
{
	return stm_ms(stm_diff((u64)t1, (u64)t0));
}

f64 TimeDiffSec(Time diff)
{
	return stm_sec((u64)diff);
}

f64 TimeDiffMs(Time diff)
{
	return stm_ms((u64)diff);
}

f64 TimeDurationSinceSec(Time t0)
{
	return stm_sec(stm_since((u64)t0));
}

f64 TimeDurationSinceMs(Time t0)
{
	return stm_ms(stm_since((u64)t0));
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

static EASTLInit eastlInit;
