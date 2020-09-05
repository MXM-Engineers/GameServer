#ifdef CONF_LINUX
#include "platform.h"
#include "base.h"
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>

void DbgBreak()
{
	raise(SIGTRAP);
}

static Func_CloseSignalHandler g_CloseSignalHandler = nullptr;

static void SignalHandler(int signo)
{
	if(signo == SIGINT) {
		LOG(">> Exit signal");
		g_CloseSignalHandler();
	}
}

bool SetCloseSignalHandler(Func_CloseSignalHandler func)
{
	g_CloseSignalHandler = func;
	if(signal(SIGINT, SignalHandler) == SIG_ERR) {
		return false;
	}
	return true;
}

void MakeDirectory(const char* path)
{
	mkdir(path, 0755);
}
#endif
