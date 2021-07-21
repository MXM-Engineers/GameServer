#include "logger.h"
#include <EASTL/array.h>
#include <EAStdC/EASprintf.h>
#include <EAStdC/EAString.h>

#ifdef CONF_WINDOWS
	#include <windows.h> // OutputDebugStringA
#endif

Logger g_LogBase;
Logger g_LogNetTraffic;

void Logger::Init(const char* filepath_, int flags_)
{
	flags = flags_;
	filepath = filepath_;
	file = fopen(filepath, "wb");
	if(!file) DebugBreak();
}

void Logger::Vlogf(const char* fmt, va_list list)
{
	char fmtBuff[8192];

	// this one is faster but gives a big number
	/*
	EA::Thread::ThreadUniqueId threadID;
	EAThreadGetUniqueId(threadID);
	*/
	EA::Thread::ThreadId threadID = EA::Thread::GetThreadId();

	const int headerLen = EA::StdC::Snprintf(fmtBuff, sizeof(fmtBuff), "[%x] ", (int)(intptr_t)threadID);
	EA::StdC::Vsnprintf(fmtBuff + headerLen, sizeof(fmtBuff) - headerLen, fmt, list);

	const int len = EA::StdC::Strlen(fmtBuff);

	mutex.Lock();
	{
		if(flags & LoggerFlags::PrintToStdout) fwrite(fmtBuff, 1, len, stdout);
		fwrite(fmtBuff, 1, len, file);

	#ifdef CONF_WINDOWS
	#ifdef CONF_DEBUG
		if(IsDebuggerPresent()) {
			OutputDebugStringA(fmtBuff);
		}
	#endif
	#endif
	}
	mutex.Unlock();
}

void Logger::FlushAndClose()
{
	fflush(stdout);
	fflush(file);
	fclose(file);
}
