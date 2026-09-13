#pragma once

#include <common/platform.h>

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace VelqorTrace
{
	inline bool& OverflowFlag()
	{
		static thread_local bool overflow = false;
		return overflow;
	}

	inline void Catf(char* buffer, size_t capacity, size_t& length, const char* format, ...)
	{
		if(capacity == 0 || length >= capacity - 1) {
			OverflowFlag() = true;
			return;
		}

		va_list args;
		va_start(args, format);
		const int written = vsnprintf(buffer + length, capacity - length, format, args);
		va_end(args);

		if(written < 0) {
			OverflowFlag() = true;
			return;
		}

		const size_t remaining = capacity - length;
		if((size_t)written >= remaining) {
			length = capacity - 1;
			OverflowFlag() = true;
		}
		else {
			length += (size_t)written;
		}
	}

	inline void CatF32(char* buffer, size_t capacity, size_t& length, float value)
	{
		if(value == value && value >= -3.402823466e38f && value <= 3.402823466e38f) Catf(buffer, capacity, length, "%.6f", value);
		else Catf(buffer, capacity, length, "null");
	}

	inline void CatVec(char* buffer, size_t capacity, size_t& length, const float* values, size_t count)
	{
		Catf(buffer, capacity, length, "[");
		for(size_t i = 0; i < count; ++i) {
			if(i) Catf(buffer, capacity, length, ",");
			CatF32(buffer, capacity, length, values[i]);
		}
		Catf(buffer, capacity, length, "]");
	}

	inline void CatUids(char* buffer, size_t capacity, size_t& length, const uint32_t* uids, size_t count)
	{
		Catf(buffer, capacity, length, "[");
		for(size_t i = 0; i < count; ++i) Catf(buffer, capacity, length, "%s%u", i ? "," : "", uids[i]);
		Catf(buffer, capacity, length, "]");
	}
}

#ifdef CONF_WINDOWS

namespace VelqorTrace
{
	struct TraceState
	{
		bool initialized = false;
		bool enabled = false;
		FILE* file = nullptr;
		DWORD pid = 0;
		uint64_t freq = 0;
		uint64_t seq = 0;
		char runDir[1024];
		char runId[256];
	};

	inline TraceState& State()
	{
		static TraceState state;
		return state;
	}

	inline SRWLOCK& Lock()
	{
		static SRWLOCK lock = SRWLOCK_INIT;
		return lock;
	}

	struct ScopedLock
	{
		ScopedLock() { AcquireSRWLockExclusive(&Lock()); }
		~ScopedLock() { ReleaseSRWLockExclusive(&Lock()); }
		ScopedLock(const ScopedLock&) = delete;
		ScopedLock& operator=(const ScopedLock&) = delete;
	};

	[[noreturn]] inline void ReportFailure(const char* what)
	{
		fprintf(stderr, "velqor_trace: %s (pid %lu)\n", what, (unsigned long)GetCurrentProcessId());
		fflush(stderr);
		abort();
	}


	inline uint64_t QpcNsFromCount(uint64_t count, uint64_t freq)
	{
		if(freq == 0) ReportFailure("invalid QPC frequency");
		return (count / freq) * 1000000000ull + ((count % freq) * 1000000000ull) / freq;
	}

	inline void WriteBytes(TraceState& state, const char* data, size_t length)
	{
		if(fwrite(data, 1, length, state.file) != length) {
			ReportFailure("write failed");
		}
	}

	inline void WriteOverflowRecord(TraceState& state, uint64_t qpc, uint64_t seq, const char* droppedEvent)
	{
		char line[1024];
		const int written = snprintf(line, sizeof(line),
			"{\"schema\":1,\"source\":\"server\",\"run_id\":\"%s\",\"pid\":%lu,\"qpc_ns\":%llu,\"seq\":%llu,\"event\":\"trace_overflow\",\"dropped_event\":\"%s\"}\n",
			state.runId,
			(unsigned long)state.pid,
			(unsigned long long)qpc,
			(unsigned long long)seq,
			droppedEvent ? droppedEvent : "");

		if(written <= 0 || (size_t)written >= sizeof(line)) {
			ReportFailure("trace_overflow record too large");
		}

		WriteBytes(state, line, (size_t)written);
	}

	inline void WriteLocked(const char* event, const char* fields, bool fieldsOverflowed)
	{
		TraceState& state = State();
		if(!state.file || state.freq == 0) return;

		LARGE_INTEGER counter;
		QueryPerformanceCounter(&counter);
		const uint64_t qpc = QpcNsFromCount((uint64_t)counter.QuadPart, state.freq);
		const uint64_t seq = ++state.seq;

		if(fieldsOverflowed) {
			WriteOverflowRecord(state, qpc, seq, event);
			return;
		}

		char line[4096];
		const int written = snprintf(line, sizeof(line),
			"{\"schema\":1,\"source\":\"server\",\"run_id\":\"%s\",\"pid\":%lu,\"qpc_ns\":%llu,\"seq\":%llu,\"event\":\"%s\"%s%s}\n",
			state.runId,
			(unsigned long)state.pid,
			(unsigned long long)qpc,
			(unsigned long long)seq,
			event,
			(fields && fields[0]) ? "," : "",
			fields ? fields : "");

		if(written <= 0) {
			ReportFailure("record formatting failed");
			return;
		}

		if((size_t)written >= sizeof(line)) {
			WriteOverflowRecord(state, qpc, seq, event);
			return;
		}

		WriteBytes(state, line, (size_t)written);
	}


	inline bool Enabled()
	{
		TraceState& state = State();
		ScopedLock lock;
		if(state.initialized) return state.enabled;
		state.initialized = true;

		const DWORD dirLen = GetEnvironmentVariableA("VELQOR_RUN_DIR", state.runDir, (DWORD)sizeof(state.runDir));
		const DWORD idLen = GetEnvironmentVariableA("VELQOR_RUN_ID", state.runId, (DWORD)sizeof(state.runId));
		if(dirLen == 0 && idLen == 0) {
			state.enabled = false;
			return false;
		}

		if(dirLen == 0 || dirLen >= sizeof(state.runDir) || idLen == 0 || idLen >= sizeof(state.runId)) {
			ReportFailure("both VELQOR_RUN_DIR and VELQOR_RUN_ID must be present and fit");
		}

		state.enabled = true;
		state.pid = GetCurrentProcessId();

		LARGE_INTEGER freq;
		if(!QueryPerformanceFrequency(&freq) || freq.QuadPart <= 0) {
			ReportFailure("QueryPerformanceFrequency unavailable, no records can be written");
			return true;
		}
		state.freq = (uint64_t)freq.QuadPart;

		char path[1400];
		const int pathLen = snprintf(path, sizeof(path), "%s/server_%lu.ndjson", state.runDir, (unsigned long)state.pid);
		if(pathLen <= 0 || (size_t)pathLen >= sizeof(path)) {
			ReportFailure("trace path too long");
			return true;
		}

		state.file = fopen(path, "wb");
		if(!state.file) {
			const int crtErrno = errno;
			const unsigned long dosErrno = _doserrno;
			fprintf(stderr, "velqor_trace: fopen path=%s errno=%d doserrno=%lu\n", path, crtErrno, dosErrno);
			ReportFailure("trace open failed");
		}

		if(setvbuf(state.file, nullptr, _IONBF, 0) != 0) {
			ReportFailure("unbuffered trace setup failed");
		}
		state.seq = 0;
		WriteLocked("server_start", nullptr, false);
		return true;
	}

	inline void Write(const char* event, const char* fields)
	{
		const bool fieldsOverflowed = OverflowFlag();
		OverflowFlag() = false;
		if(!Enabled()) return;
		ScopedLock lock;
		WriteLocked(event, fields, fieldsOverflowed);
	}

	inline void Emit(const char* event, const char* fields)
	{
		Write(event, fields);
	}

	inline void Flush()
	{
		if(!Enabled()) return;
		TraceState& state = State();
		ScopedLock lock;
		if(fflush(state.file) != 0) {
			ReportFailure("flush failed");
		}
	}

	inline uint64_t QpcNs()
	{
		if(!Enabled()) return 0;
		TraceState& state = State();
		ScopedLock lock;
		if(state.freq == 0) return 0;

		LARGE_INTEGER counter;
		QueryPerformanceCounter(&counter);
		return QpcNsFromCount((uint64_t)counter.QuadPart, state.freq);
	}
}

#else

namespace VelqorTrace
{
	inline bool Enabled() { return false; }
	inline void Emit(const char*, const char*) {}
	inline void Write(const char*, const char*) {}
	inline void Flush() {}
	inline uint64_t QpcNs() { return 0; }
}

#endif
