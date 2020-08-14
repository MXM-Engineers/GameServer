#include "base.h"
#include <mutex>
#include <windows.h>

FILE* g_LogFile;
const char* g_LogFileName;

struct Logger
{
	std::mutex mutex;

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

	const std::lock_guard<std::mutex> lock(g_Logger.mutex);
	printf(final);
	fprintf(g_LogFile, final);

#ifdef CONF_DEBUG
	if(IsDebuggerPresent()) {
		OutputDebugStringA(final);
	}
#endif
}
