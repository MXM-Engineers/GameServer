#include "utils.h"
#include <time.h>

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
	path.append(app);

#ifdef CONF_DEBUG
	foreach(it, path) {
		ASSERT(*it != L'\\'); // path must be in linux form (path/to/thing)
	}
#endif

	// remove double '/'
	foreach_mut(it, path) {
		if(*it == L'/' && (it+1) != path.end() && *(it+1) == L'/') {
			path.erase(it);
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
	eastl::DecodePart(filename, filename + wcslen(filename), dest, utf8Path + sizeof(utf8Path));

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

static u32 g_XorShiftState = time(0);

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
