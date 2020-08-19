#include "base.h"
#include <windows.h>

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

	snprintf(final, sizeof(final), "[%x] %s", GetCurrentThreadId(), buff);

	const std::lock_guard<Mutex> lock(g_Logger.mutex);
	printf(final);
	fprintf(g_LogFile, final);

#ifdef CONF_DEBUG
	if(IsDebuggerPresent()) {
		OutputDebugStringA(final);
	}
#endif
}

i64 GetGlobalTime()
{
#ifdef _WIN32
	return (i64)_time64(NULL);
#endif
}

i32 GetTime()
{
#ifdef _WIN32
	return (i32)clock();
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
