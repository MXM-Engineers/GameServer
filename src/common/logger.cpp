#include "logger.h"
#include <EASTL/array.h>
#include <EAStdC/EASprintf.h>
#include <EAStdC/EAString.h>

#ifdef CONF_WINDOWS
	#include <windows.h> // OutputDebugStringA
#endif

Logger g_LogBase;
Logger g_LogNetTraffic;
bool g_LogVerbose = false; // TODO: load from config

void Logger::Init(const char* filepath_, int flags_)
{
	flags = flags_;
	filepath = filepath_;
	file = fopen(filepath, "wb");
	if(!file) DebugBreak();
}

void Logger::Vlogf(const char* fmt, va_list list)
{
	eastl::fixed_string<char,8192> fmtBuff;

	// this one is faster but gives a big number
	/*
	EA::Thread::ThreadUniqueId threadID;
	EAThreadGetUniqueId(threadID);
	*/
	EA::Thread::ThreadId threadID = EA::Thread::GetThreadId();

	fmtBuff.sprintf("[%x] ", (int)(intptr_t)threadID);
	fmtBuff.append_sprintf_va_list(fmt, list);
	const int len = fmtBuff.length();

	mutex.Lock();
	{
		if(flags & LoggerFlags::PrintToStdout) fwrite(fmtBuff.data(), 1, len, stdout);
		fwrite(fmtBuff.data(), 1, len, file);

	#ifdef CONF_WINDOWS
	#ifdef CONF_DEBUG
		if(IsDebuggerPresent()) {
			OutputDebugStringA(fmtBuff.data());
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
