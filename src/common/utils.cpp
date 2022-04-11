#include "utils.h"
#include <time.h>
#include <EAStdC/EAString.h>

void PathSet(Path& path, const wchar* str)
{
	path = str;

#ifdef CONF_DEBUG
	foreach(it, path) {
		ASSERT(*it != L'\\'); // path must be in linux form (path/to/thing)
	}
#endif

	// convert to windows path
#ifdef _WIN32
	foreach(it, path) {
		if(*it == L'/') {
			*it = L'\\';
		}
	}
#endif
}

void PathAppend(Path& path, const wchar* app)
{
#ifdef CONF_DEBUG
	const i32 len = (i32)EA::StdC::Strlen(app);
	for(i32 i = 0; i < len; i++) {
		ASSERT(app[i] != L'\\'); // path must be in linux form (path/to/thing)
	}
#endif

	path.append(app);

	// remove double '/'
	foreach(it, path) {
		if(*it == L'/' && (it+1) != path.end() && *(it+1) == L'/') {
			it = path.erase(it);
		}
	}

	// convert to windows path
#ifdef _WIN32
	foreach(it, path) {
		if(*it == L'/') {
			*it = L'\\';
		}
	}
#endif
}

u8* FileOpenAndReadAll(const wchar* filename, i32* pOutSize)
{
	char utf8Path[512] = {0};
	char* dest = utf8Path;
	eastl::DecodePart(filename, filename + EA::StdC::Strlen(filename), dest, utf8Path + sizeof(utf8Path));

	FILE* f = fopen(utf8Path, "rb");
	if(f) {
		fseek(f, 0, SEEK_END);
		int size = ftell(f);
		fseek(f, 0, SEEK_SET);

		u8* buff = (u8*)memAlloc(size + 1);
		fread(buff, size, 1, f);
		buff[size] = 0;
		*pOutSize = size;

		fclose(f);
		return buff;
	}

	return nullptr;
}

static u32 g_XorShiftState = (u32)time(0);

u32 RandUint()
{
	/* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
	uint32_t x = g_XorShiftState;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	g_XorShiftState = x;
	return x;
}

f64 Randf01()
{
	return (f64)RandUint() / 0xFFFFFFFF;
}

i32 RandInt(i32 vmin, i32 vmax)
{
	return vmin + (RandUint() % (vmax + 1 - vmin));
}

void ThreadSetCoreAffinity(i32 coreID)
{
    // loop around by default
    const i32 count = EA::Thread::GetProcessorCount();
    EA::Thread::SetThreadAffinityMask(1 << (coreID % count));
}
