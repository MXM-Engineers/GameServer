#ifdef CONF_LINUX
#include "platform.h"
#include <signal.h>
#include <sys/stat.h>

void DbgBreak()
{
	raise(SIGTRAP);
}

bool SetCloseSignalHandler(Func_CloseSignalHandler func)
{
	// TODO: clean exit on ctrl + c
	return true;
}

void MakeDirectory(const char* path)
{
	mkdir(path, 0755);
}

#endif
