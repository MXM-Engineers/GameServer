#pragma once
#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <eathread/eathread_futex.h> // mutex
#include <EASTL/fixed_vector.h>
#include <EASTL/fixed_string.h>
#include <EAStdC/EAString.h>
#include "logger.h"

#define STATIC_ASSERT(cond) static_assert(cond, #cond)
#define ASSERT_SIZE(T, SIZE) STATIC_ASSERT(sizeof(T) == SIZE)

inline void __assertion_failed(const char* cond, const char* file, int line)
{
	LOG("Assertion failed (%s : %d): %s", file, line, cond);
	LogsFlushAndClose();
	DbgBreak();
}

#define ASSERT(cond) do { if(!(cond)) { __assertion_failed(#cond, __FILE__, __LINE__); } } while(0)
#define ASSERT_MSG(cond, msg) do { if(!(cond)) { __assertion_failed(#cond " (" #msg ")", __FILE__, __LINE__); } } while(0)
#ifdef CONF_DEBUG
	#define DBG_ASSERT(cond) ASSERT(cond)
#else
	#define DBG_ASSERT(cond)
#endif

#define ARRAY_COUNT(A) (sizeof(A)/sizeof(A[0]))

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

STATIC_ASSERT(sizeof(wchar) == 2);

inline void* memAlloc(size_t size)
{
	//ProfileBlock("memAlloc");
	void* ptr = malloc(size);
	//ProfileMemAlloc(ptr, size);
	return ptr;
}

inline void* memRealloc(void* inPtr, size_t size)
{
	//ProfileBlock("memRealloc");
	//ProfileMemFree(inPtr);
	void* ptr = realloc(inPtr, size);
	//ProfileMemAlloc(ptr, size);
	return ptr;
}

inline void memFree(void* ptr)
{
	//ProfileBlock("memFree");
	//ProfileMemFree(ptr)
	return free(ptr);
}

struct Buffer
{
	u8* data;
	i32 size;
	i32 capacity;

	Buffer(i32 capacity_)
	{
		data = (u8*)memAlloc(capacity_);
		size = 0;
		capacity = capacity_;
	}

	~Buffer()
	{
		free(data);
	}

	void Clear()
	{
		size = 0;
	}

	void Append(const void* ap, i32 apSize)
	{
		ASSERT(size + apSize <= capacity);
		memmove(data + size, ap, apSize);
		size += apSize;
	}

	void AppendString(const char* str)
	{
		const i32 apSize = (i32)strlen(str);
		ASSERT(size + apSize <= capacity);
		memmove(data + size, str, apSize);
		size += apSize;
	}
};

inline u8* fileOpenAndReadAll(const char* filename, i32* pOutSize)
{
	FILE* f = fopen(filename, "rb");
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

inline bool fileSaveBuff(const char* filename, const void* buff, i32 size)
{
	FILE* f = fopen(filename, "wb");
	if(f) {
		fwrite(buff, 1, size, f);
		fclose(f);
		return true;
	}

	WARN("Failed to save '%s'", filename);
	return false;
}

const char* _TempStrFormat(const char* fmt, ...);
#define FMT(...) _TempStrFormat(__VA_ARGS__)

const wchar* _TempWideStrFormat(const wchar* fmt, ...);
#define LFMT(...) _TempWideStrFormat(__VA_ARGS__)

struct ConstBuffer
{
	u8* data;
	u8* cursor;
	i32 size;

	ConstBuffer(const void* ptr, i32 size_)
	{
		data = (u8*)ptr;
		cursor = (u8*)ptr;
		size = size_;
	}

	template<typename T>
	inline T& Read()
	{
		ASSERT((cursor - data) + sizeof(T) <= size);
		T& t = *(T*)cursor;
		cursor += sizeof(T);
		return t;
	}

	inline u8* ReadRaw(i32 len)
	{
		ASSERT((cursor - data) + len <= size);
		u8* r = cursor;
		cursor += len;
		return r;
	}

	inline bool CanRead(i32 len)
	{
		return ((cursor - data) + len <= size);
	}

	inline eastl::fixed_string<wchar,256> ReadWideStringObj(u16* outLen = nullptr)
	{
		u16 len = Read<u16>();
		if(outLen) *outLen = len;
		return eastl::fixed_string<wchar,256>((wchar*)ReadRaw(len * sizeof(wchar)), len);
	}

	inline i32 RemainingDataSize() const
	{
		return size - (cursor - data);
	}

	inline i32 GetCursorPos() const
	{
		return (cursor - data);
	}
};

inline bool StringEquals(const char* str1, const char* str2)
{
	const i32 len = (i32)strlen(str1);
	if(len != strlen(str2)) {
		return false;
	}
	return strncmp(str1, str2, len) == 0;
}

inline void logAsHex(const void* data, int size)
{
	LOGN("[ ");
	const u8* b = (u8*)data;
	for(int i = 0; i < size; i++) {
		LOGN("%x, ", b[i]);
	}
	LOGN("]");
}

// https://www.gingerbill.org/article/2015/08/19/defer-in-cpp/
template <typename F>
struct privDefer {
	F f;
	privDefer(F f) : f(f) {}
	~privDefer() { f(); }
};

template <typename F>
privDefer<F> defer_func(F f) {
	return privDefer<F>(f);
}

#define DEFER_1(x, y) x##y
#define DEFER_2(x, y) DEFER_1(x, y)
#define DEFER_3(x)    DEFER_2(x, __COUNTER__)
#define defer(code)   auto DEFER_3(_defer_) = defer_func([&](){code;})

template<typename Packet, u32 CAPACITY=256>
struct PacketWriter
{
	enum {
		NET_ID = Packet::NET_ID,
		capacity = CAPACITY
	};

	u8 data[capacity];
	i32 size = 0;

	template<typename T>
	inline i32 Write(const T& val)
	{
		ASSERT(size + sizeof(T) <= capacity);
		memmove(data + size, &val, sizeof(T));
		size += sizeof(T);
		return size;
	}

	template<typename T>
	inline i32 WriteVec(const T* list, const u16 count)
	{
		Write<u16>(count);
		WriteRaw(list, count * sizeof(T));
		return size;
	}

	inline i32 WriteRaw(const void* buff, i32 buffSize)
	{
		ASSERT(size + buffSize <= capacity);
		memmove(data + size, buff, buffSize);
		size += buffSize;
		return size;
	}

	i32 WriteStringObj(const wchar* str, i32 len = -1)
	{
		if(len == -1) len = EA::StdC::Strlen(str);
		Write<u16>(len);
		WriteRaw(str, len * sizeof(wchar));
		return size;
	}
};

template<typename T>
inline T SafeCast(const void* data, i32 size)
{
	DBG_ASSERT(sizeof(T) == size);
	if(size < sizeof(T)) {
		T out;
		memset(&out, 0, sizeof(T));
		memmove(&out, data, size);
		return out;
	}
	return *(T*)data;
}

struct GrowableBuffer
{
	u8* data = nullptr;
	i32 size = 0;
	i32 capacity = 0;

	GrowableBuffer() = default;
	explicit GrowableBuffer(i32 capacity_)
	{
		Init(capacity_);
	}

	void Init(i32 capacity_)
	{
		ASSERT(data == nullptr);
		data = (u8*)memAlloc(capacity_);
		size = 0;
		capacity = capacity_;
	}

	~GrowableBuffer()
	{
		Release();
	}

	void Release()
	{
		free(data);
		data = nullptr;
		size = 0;
		capacity = 0;
	}

	void Reserve(i32 newCapacity)
	{
		if(newCapacity <= capacity) return;
		data = (u8*)memRealloc(data, newCapacity);
		capacity = newCapacity;
	}

	u8* Append(const void* buff, i32 buffSize)
	{
		if(size + buffSize > capacity) {
			Reserve(MIN(size+buffSize, capacity*2));
		}
		memmove(data+size, buff, buffSize);
		u8* r = data+size;
		size += buffSize;
		return r;
	}

	void Clear()
	{
		size = 0;
	}
};



#ifdef TRACY_ENABLE
#include <Tracy.hpp>
struct Mutex: EA::Thread::Futex
{
	tracy_force_inline void lock() { Lock(); }
	tracy_force_inline void unlock() { Unlock(); }
};

template<class Mutex>
class LockGuardT
{
public:
	LockGuardT(Mutex& futex):
		mFutex(futex)
	{
		mFutex.lock();
	}
	~LockGuardT()
	{
		mFutex.unlock();
	}

protected:
	Mutex& mFutex;

	// Prevent copying by default, as copying is dangerous.
	LockGuardT(const LockGuardT&);
	const LockGuardT& operator=(const LockGuardT&);
};

typedef LockGuardT<tracy::Lockable<Mutex>> LockGuard;

#define LOCK_MUTEX(MUTEX) const LockGuardT<decltype(MUTEX)> lock(MUTEX)
#define LOCK_MUTEXN(MUTEX, NAME) const LockGuardT<decltype(MUTEX)> NAME(MUTEX)

#define ProfileFunction() ZoneScopedN(FUNCTION_STR)
#define ProfileBlock(name) ZoneScopedN(FUNCTION_STR ">>" name)
#define ProfileMutex(TYPE, NAME) TracyLockable(TYPE, NAME)
#define ProfileNewFrame(NAME) FrameMarkNamed(NAME)
#define ProfilePlotVar(V) TracyPlot(#V, V)
#define ProfilePlotVarN(N, V) TracyPlot(N, V)
#define ProfileMemAlloc(PTR, SIZE) TracyAlloc(PTR, SIZE)
#define ProfileMemFree(PTR) TracyFree(PTR)
#define ProfileAttachStringf(STRF, ...) char __buff##__LINE__[64];\
	snprintf(__buff##__LINE__, sizeof(__buff##__LINE__), STRF, __VA_ARGS__);\
	ZoneName(__buff##__LINE__, sizeof(__buff##__LINE__))
#define ProfileSetThreadName(NAME) tracy::SetThreadName(NAME)

#else
#define ProfileFunction()
#define ProfileBlock(name)
#define ProfileMutex(TYPE, NAME) TYPE NAME;
#define ProfileNewFrame(NAME)
#define ProfilePlotVar(V)
#define ProfilePlotVarN(N,V)
#define ProfileMemAlloc(PTR, SIZE)
#define ProfileMemFree(PTR)
#define ProfileAttachStringf(STRF, ...)
#define ProfileSetThreadName(NAME)

typedef EA::Thread::Futex Mutex;
typedef EA::Thread::AutoFutex LockGuard;

#define LOCK_MUTEX(MUTEX) const LockGuard __lock##__LINE__(MUTEX)
#endif

// NOTE: this is kinda dirty but funny at the same time? And useful?
#define foreach(IT,CONTAINER) for(auto IT = CONTAINER.begin(), IT##End = CONTAINER.end(); IT != IT##End; ++IT)
#define foreach_const(IT,CONTAINER) for(auto IT = CONTAINER.cbegin(), IT##End = CONTAINER.cend(); IT != IT##End; ++IT)
#define foreach_mut(IT,CONTAINER) for(auto IT = CONTAINER.begin(); IT != CONTAINER.end(); ++IT)

#define forarr(IT,ARRAY,COUNT) for(auto IT = ARRAY; IT != ARRAY+COUNT; ++IT)

// time API
enum class Time: u64
{
	ZERO = 0
};

void TimeInit();
Time TimeNow();
Time TimeRelNow(); // time ticks since start
f64 TimeDurationSec(Time t0, Time t1);
f64 TimeDurationMs(Time t0, Time t1);
Time TimeDiff(Time start, Time end);
f64 TimeDiffSec(Time diff);
f64 TimeDiffMs(Time diff);
f64 TimeDurationSinceSec(Time t0);
f64 TimeDurationSinceMs(Time t0);
Time TimeMsToTime(f64 ms);

inline Time TimeAdd(Time a, Time b)
{
	return Time((u64)a + (u64)b);
}


template<typename T>
T clamp(T val, T min, T max)
{
	if(val < min) return min;
	if(val > max) return max;
	return val;
}

template<typename T>
T sign(T val)
{
	return val < (T)0 ? (T)-1 : (T)1;
}

template<size_t CAPACITY>
using FixedStr = eastl::fixed_string<char,CAPACITY,false>;

constexpr i32 UPDATE_TICK_RATE = 60;
constexpr f64 UPDATE_RATE = 1.0/UPDATE_TICK_RATE;

struct float2
{
	f32 x = 0, y = 0;
};

struct float3
{
	f32 x = 0, y = 0, z = 0;
};

enum class CoreAffinity: i32
{
	MAIN = 0,
	NETWORK = 1,
	COORDINATOR = 2,
	LANES = 3,

	MATCHMAKER = 2,
};

inline u32 hash_fnv1a(const void* data, const u32 dataSize)
{
	u32 h = 0x811c9dc5;

	const u8* d = (u8*)data;
	for(u32 i = 0; i < dataSize; i++) {
		h ^= d[i];
		h *= 0x01000193;
	}

	return h;
}

struct FileBuffer
{
	u8* data;
	i32 size;
};
