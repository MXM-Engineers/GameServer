#ifdef CONF_WINDOWS
#include "platform.h"
#include "base.h"
#include <direct.h>
#include <EASTL/fixed_string.h>

void DbgBreak()
{
	__debugbreak();
}

static Func_CloseSignalHandler g_CloseSignalHandler = nullptr;

BOOL WINAPI ConsoleHandler(DWORD signal)
{
	if(signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT) {
		LOG(">> Exit signal");
		g_CloseSignalHandler();
	}

	return TRUE;
}

bool SetCloseSignalHandler(Func_CloseSignalHandler func)
{
	g_CloseSignalHandler = func;
	if(!SetConsoleCtrlHandler(ConsoleHandler, TRUE)) {
		LOG("ERROR: Could not set control handler");
		return false;
	}
	return true;
}

void MakeDirectory(const char* path)
{
	_mkdir(path);
}

const char* FormatPath(const char* path)
{
	thread_local eastl::fixed_string<char,512,false> buff;
	buff = path;
	foreach(it, buff) {
		if(*it == '/') {
			*it = '\\';
		}
	}
	return buff.data();
}

void PlatformInit()
{
#ifdef TRACY_ENABLE
	MakeDirectory("./captures");
	time_t t = time(0);
	struct tm* tt = localtime(&t);
	char cmdLine[512];
	sprintf(cmdLine, "@start cmd /c \"..\\profiler\\capture.exe -a localhost -o captures\\capture_%d-%d-%d_%d-%d-%d.tracy\"", tt->tm_mday, tt->tm_mon+1, tt->tm_year+1900, tt->tm_hour, tt->tm_min, tt->tm_sec);

	system(cmdLine);

	ProfileSetThreadName("Main");
#endif

	AllocConsole();
	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stderr);
	freopen("CONOUT$", "w", stdout);
}
#endif

