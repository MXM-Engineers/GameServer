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
	buff.replace(buff.begin(), buff.end(), '/', '\\');
	return buff.data();
}
#endif

