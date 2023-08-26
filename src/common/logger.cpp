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

static intptr_t ThreadLogger(void* pData)
{
	Logger& logger = *(Logger*)pData;

	while(logger.running) {
		logger.WriteOut();
		EA::Thread::ThreadSleep((EA::Thread::ThreadTime)(200));
	}

	return 0;
}

void Logger::Init(const char* filepath_, int flags_)
{
	flags = flags_;
	filepath = filepath_;
	file = fopen(filepath, "wb");
	if(!file) DebugBreak();

	running = true;
	thread.Begin(ThreadLogger, this);
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

	const EA::Thread::AutoFutex lock(mutexBuffer);
	buffer.append(fmtBuff.data(), fmtBuff.length());
}

void Logger::FlushAndClose()
{
	running = false;
	thread.WaitForEnd();

	WriteOut();

	fflush(stdout);
	fflush(file);
	fclose(file);
}

void Logger::WriteOut()
{
	const EA::Thread::AutoFutex lock(mutexBuffer);

	const size_t len = buffer.length();
	if(flags & LoggerFlags::PrintToStdout) fwrite(buffer.data(), 1, len, stdout);
	fwrite(buffer.data(), 1, len, file);

#ifdef CONF_WINDOWS
#ifdef CONF_DEBUG
	if(IsDebuggerPresent()) {
		OutputDebugStringA(buffer.data());
	}
#endif
#endif

	buffer.clear();
}

Logger::~Logger() {

	if(file) {
		FlushAndClose();
	}
}
