#include "base.h"
#include <time.h>
#include <eathread/eathread.h>
#include <EASTL/array.h>
#include <EAStdC/EASprintf.h>
#include <EAStdC/EAString.h>

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
	thread_local char buff[8192];

	va_list list;
	va_start(list, fmt);
	EA::StdC::Vsnprintf(buff, sizeof(buff), fmt, list);
	va_end(list);

	return buff;
}

const wchar* _TempWideStrFormat(const wchar* fmt, ...)
{
	thread_local wchar buff[8192];

	va_list list;
	va_start(list, fmt);
	EA::StdC::Vsnprintf(buff, ARRAY_COUNT(buff), fmt, list);
	va_end(list);

	return buff;
}

i32 PacketWriter::WriteStringObj(const wchar* str, i32 len)
{
	if(len == -1) len = EA::StdC::Strlen(str);
	Write<u16>(len);
	WriteRaw(str, len * sizeof(wchar));
	return size;
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

f64 TimeDurationSec(Time t0, Time t1)
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

Time TimeMsToTime(f64 ms)
{
	return Time(u64(ms * 1000000.0));
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
