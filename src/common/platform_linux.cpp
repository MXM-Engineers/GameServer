#ifdef CONF_LINUX
#include "platform.h"
#include "base.h"
#include <signal.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <sys/random.h>
#include <sys/time.h>
#include <errno.h>

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

const char* FormatPath(const char* path)
{
	return path;
}

void PlatformInit()
{
	prctl(PR_SET_DUMPABLE, 1); // enable generating dump when program crashes.
}

uint64_t CurrentFiletimeTimestampUTC()
{
	//WARNING: Implementation CurrentFiletimeTimestampUTC not tested"
	struct timeval tv;
	gettimeofday(&tv, NULL);
	uint64_t result = tv.tv_sec * (uint64_t)1000000 + tv.tv_usec;
	return result * 10 + 116444736000000000ULL;
}

bool PlatformRandomBytes(void* out, int32_t size)
{
	uint8_t* cursor = (uint8_t*)out;
	int32_t remaining = size;
	while(remaining > 0) {
		const ssize_t r = getrandom(cursor, remaining, 0);
		if(r < 0) {
			if(errno == EINTR) continue;
			return false;
		}
		cursor += r;
		remaining -= (int32_t)r;
	}
	return true;
}
#endif
