#pragma once
#include <stdio.h>
#include <eathread/eathread_futex.h> // mutex
#include <EASTL/fixed_string.h>

struct LoggerFlags
{
	enum
	{
		PrintToStdout = 1
	};
};

struct Logger
{
	int flags = 0x0;
	const char* filepath;
	FILE* file;
	EA::Thread::Futex mutex;

	void Init(const char* filepath, int flags_);
	void Vlogf(const char* fmt, va_list list);
	void FlushAndClose();

	inline void __Logf(const char* fmt, ...)
	{
		va_list list;
		va_start(list, fmt);
		Vlogf(fmt, list);
		va_end(list);
	}

	inline void __LogfLine(const char* fmt, ...)
	{
		eastl::fixed_string<char,8192,false> preBuff;
		preBuff = fmt;
		preBuff.append("\n");

		va_list list;
		va_start(list, fmt);
		Vlogf(preBuff.data(), list);
		va_end(list);
	}

	inline void __Warnf(const char* functionName, const char* fmt, ...)
	{
		eastl::fixed_string<char,8192,false> preBuff;
		preBuff.sprintf("WARNING(%s): %s\n", functionName, fmt);

		va_list list;
		va_start(list, fmt);
		Vlogf(preBuff.data(), list);
		va_end(list);
	}

	~Logger() {
		if(file) {
			fclose(file);
		}
	}
};

extern Logger g_LogBase;
extern Logger g_LogNetTraffic;

#define MSVC_VERIFY_FORMATTING(...) (0 && snprintf(0, 0, ##__VA_ARGS__))

#define LOG(...) do { g_LogBase.__LogfLine(__VA_ARGS__); MSVC_VERIFY_FORMATTING(__VA_ARGS__); } while(0)
#define LOGN(...) do { g_LogBase.__Logf(__VA_ARGS__); MSVC_VERIFY_FORMATTING(__VA_ARGS__); } while(0)
#define WARN(...) do { g_LogBase.__Warnf(FUNCTION_STR, ##__VA_ARGS__); MSVC_VERIFY_FORMATTING(__VA_ARGS__); } while(0)

#define NT_LOG(...) do { g_LogNetTraffic.__LogfLine(__VA_ARGS__); MSVC_VERIFY_FORMATTING(__VA_ARGS__); } while(0)
#define NT_LOGN(...) do { g_LogNetTraffic.__Logf(__VA_ARGS__); MSVC_VERIFY_FORMATTING(__VA_ARGS__); } while(0)
#define NT_WARN(...) do { g_LogNetTraffic.__Warnf(FUNCTION_STR, ##__VA_ARGS__); MSVC_VERIFY_FORMATTING(__VA_ARGS__); } while(0)

inline void LogInit(const char* filepath, int flags = LoggerFlags::PrintToStdout)
{
	g_LogBase.Init(filepath, flags);
}

inline void LogNetTrafficInit(const char* filepath, int flags = LoggerFlags::PrintToStdout)
{
	g_LogNetTraffic.Init(filepath, flags);
}

inline void LogsFlushAndClose()
{
	g_LogBase.FlushAndClose();
	g_LogNetTraffic.FlushAndClose();
}
