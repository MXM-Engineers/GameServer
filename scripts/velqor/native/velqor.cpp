#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <intrin.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "detours.h"
#include "velqor.h"
#include "sha256.h"
#include "os_input.h"
#include "client_bridge.h"

namespace {

const uintptr_t kRvaVtableUpdateFrameSlot = 0x0238B8A8u;
const uintptr_t kRvaUpdateFrame = 0x001EF7F7u;
const uintptr_t kRvaInputManagerGlobal = 0x02F08F18u;
const uintptr_t kRvaGameSessionGlobal = 0x02F08B00u;
const uintptr_t kRvaOnKeyDown = 0x016E4AEBu;
const uintptr_t kRvaOnKeyUp = 0x016E4DB2u;
const uintptr_t kRvaEmitRawVkEvent = 0x016E037Du;
const uintptr_t kRvaIsInputTypeLocked = 0x016DF2ABu;
const uintptr_t kRvaDiHwndGlobal = 0x0332FDD4u;
const char* const kClientWindowClass = "Gamebryo Application";
const char* const kExeSha256Hex = "df32850dd8e7a2329ddca2dc57a3eb310a620f4a6477bbb135049a938f477583";

const int kQueueCapacity = 256;
const int kHeldCapacity = 32;
const int kHookCapacity = 64;
const int kLineCapacity = 512;
const int kUsedVkCapacity = 64;

const LONG kAbortExitCode = 2;

enum CommandOp {
	OpKey = 1,
	OpCursor = 2,
	OpMark = 3
};

struct Command {
	uint32_t seq;
	uint32_t op;
	uint32_t vk;
	uint32_t down;
	int32_t x;
	int32_t y;
	uint64_t due_ns;
	uint64_t at_ms;
	char id[40];
};

struct HookEntry {
	void** target;
	void* original;
};

typedef void(__thiscall* OnKeyDownFn)(void*, uint32_t, int32_t);
typedef void(__thiscall* OnKeyUpFn)(void*, uint32_t);
typedef void(__thiscall* EmitRawVkEventFn)(void*, uint32_t, uint32_t);
typedef uint8_t(__thiscall* IsInputTypeLockedFn)(void*, uint32_t);
typedef void(__thiscall* UpdateFrameFn)(void*);

uintptr_t g_base = 0;
volatile LONG g_attached = 0;
volatile LONG g_bridge_inited = 0;
unsigned char g_exe_digest[32];

CRITICAL_SECTION g_trace_lock;
volatile LONG g_trace_enabled = 0;
volatile LONG g_trace_error = 0;
volatile LONG g_trace_open_failed = 0;
volatile LONG g_hello_written = 0;
HANDLE g_trace_file = INVALID_HANDLE_VALUE;
char g_run_id[160];
char g_trace_path_a[520];
WCHAR g_trace_path_w[520];
uint32_t g_pid = 0;

__declspec(align(8)) volatile LONGLONG g_qpc_freq = 0;
__declspec(align(8)) volatile LONGLONG g_frame = 0;
__declspec(align(8)) volatile LONGLONG g_epoch_ns = 0;
volatile LONG g_auto_active = 0;

HookEntry g_hooks[kHookCapacity];
volatile LONG g_hook_lock = 0;

CRITICAL_SECTION g_queue_lock;
Command g_queue[kQueueCapacity];
uint32_t g_queue_head = 0;
uint32_t g_queue_count = 0;
volatile LONG g_cmd_seq = 0;

uint32_t g_held[kHeldCapacity];
int g_held_count = 0;

volatile LONG g_play_pending = 0;
volatile LONG g_play_result = 0;
HANDLE g_play_done = 0;

volatile LONG g_stop_pending = 0;
volatile LONG g_stop_result = 0;
volatile LONG g_stop_seq = 0;
uint32_t g_stop_released[kHeldCapacity];
volatile LONG g_stop_released_count = 0;
HANDLE g_stop_done = 0;

volatile LONG g_pipe_stop = 0;
HANDLE g_pipe_thread = 0;
WCHAR g_pipe_name[128];
int g_pipe_error_logged = 0;
unsigned char g_pipe_dacl[256];
unsigned char g_pipe_sacl[256];
SECURITY_DESCRIPTOR g_pipe_sd;
SECURITY_ATTRIBUTES g_pipe_sa;
int g_pipe_label_set = 0;
volatile LONG g_hwnd_state = 0;

UpdateFrameFn g_orig_update_frame = 0;

inline void* Va(uintptr_t rva) {
	return (void*)(g_base + rva);
}

template <typename T>
inline T VaFn(uintptr_t rva) {
	return reinterpret_cast<T>(g_base + rva);
}

void JsonEscape(char* out, int cap, const char* in) {
	int n = 0;
	if (cap <= 0) {
		return;
	}
	if (in == 0) {
		out[0] = 0;
		return;
	}
	for (const unsigned char* p = (const unsigned char*)in; *p != 0; ++p) {
		unsigned char c = *p;
		if (c == '"' || c == '\\') {
			if (n + 2 >= cap) {
				break;
			}
			out[n++] = '\\';
			out[n++] = (char)c;
		} else if (c == '\n') {
			if (n + 2 >= cap) {
				break;
			}
			out[n++] = '\\';
			out[n++] = 'n';
		} else if (c == '\r') {
			if (n + 2 >= cap) {
				break;
			}
			out[n++] = '\\';
			out[n++] = 'r';
		} else if (c == '\t') {
			if (n + 2 >= cap) {
				break;
			}
			out[n++] = '\\';
			out[n++] = 't';
		} else if (c < 0x20) {
			if (n + 6 >= cap) {
				break;
			}
			out[n++] = '\\';
			out[n++] = 'u';
			out[n++] = '0';
			out[n++] = '0';
			out[n++] = "0123456789abcdef"[c >> 4];
			out[n++] = "0123456789abcdef"[c & 15];
		} else {
			if (n + 1 >= cap) {
				break;
			}
			out[n++] = (char)c;
		}
	}
	out[n] = 0;
}

struct AckBuf {
	char b[2048];
	int n;
	int truncated;

	AckBuf() : n(0), truncated(0) {
		b[0] = 0;
	}

	void raw(const char* s) {
		if (s == 0) {
			return;
		}
		int len = (int)strlen(s);
		if (len <= 0) {
			return;
		}
		if (n + len >= (int)sizeof(b)) {
			truncated = 1;
			return;
		}
		memcpy(b + n, s, (size_t)len);
		n += len;
		b[n] = 0;
	}

	void str(const char* s) {
		char esc[1100];
		JsonEscape(esc, (int)sizeof(esc), s);
		raw(esc);
	}

	void unum(unsigned long long v) {
		char t[32];
		int len = _snprintf_s(t, sizeof(t), _TRUNCATE, "%llu", v);
		if (len > 0) {
			raw(t);
		} else {
			truncated = 1;
		}
	}

	void num(long long v) {
		char t[32];
		int len = _snprintf_s(t, sizeof(t), _TRUNCATE, "%lld", v);
		if (len > 0) {
			raw(t);
		} else {
			truncated = 1;
		}
	}
};

uint64_t QpcFreqValue() {
	LONGLONG v = InterlockedCompareExchange64(&g_qpc_freq, 0, 0);
	if (v != 0) {
		return (uint64_t)v;
	}
	LARGE_INTEGER f;
	if (QueryPerformanceFrequency(&f) && f.QuadPart > 0) {
		InterlockedCompareExchange64(&g_qpc_freq, f.QuadPart, 0);
		v = InterlockedCompareExchange64(&g_qpc_freq, 0, 0);
	}
	return (uint64_t)v;
}

void TraceWriteLocked(const char* buf, int len) {
	if (len <= 0) {
		return;
	}
	if (g_trace_file == INVALID_HANDLE_VALUE) {
		InterlockedExchange(&g_trace_error, 1);
		return;
	}
	DWORD wrote = 0;
	if (!WriteFile(g_trace_file, buf, (DWORD)len, &wrote, 0) || wrote != (DWORD)len) {
		InterlockedExchange(&g_trace_error, 1);
	}
}

void EmitInternal(const char* event, const char* fields) {
	uint64_t qpc = velqor::QpcNs();
	const char* f = (fields != 0) ? fields : "";
	char line[2048];
	int n = _snprintf_s(line, (int)(sizeof(line) / sizeof((line)[0])), _TRUNCATE,
		"{\"schema\":1,\"source\":\"client\",\"run_id\":\"%s\",\"pid\":%lu,\"qpc_ns\":%llu,\"event\":\"%s\"%s%s}\n",
		g_run_id, (unsigned long)g_pid, (unsigned long long)qpc, event,
		(f[0] != 0) ? "," : "", f);
	EnterCriticalSection(&g_trace_lock);
	if (n < 0) {
		InterlockedExchange(&g_trace_error, 1);
		char ov[512];
		int m = _snprintf_s(ov, (int)(sizeof(ov) / sizeof((ov)[0])), _TRUNCATE,
			"{\"schema\":1,\"source\":\"client\",\"run_id\":\"%s\",\"pid\":%lu,\"qpc_ns\":%llu,\"event\":\"trace_overflow\"}\n",
			g_run_id, (unsigned long)g_pid, (unsigned long long)qpc);
		if (m > 0) {
			TraceWriteLocked(ov, m);
		}
	} else {
		TraceWriteLocked(line, n);
	}
	LeaveCriticalSection(&g_trace_lock);
}

bool IsTrimWide(WCHAR c) {
	return c == L' ' || c == L'\t' || c == L'\r' || c == L'\n';
}

void TrimWide(WCHAR* s) {
	int len = 0;
	while (s[len] != 0) {
		++len;
	}
	int start = 0;
	while (start < len && IsTrimWide(s[start])) {
		++start;
	}
	while (len > start && IsTrimWide(s[len - 1])) {
		--len;
	}
	for (int i = start; i < len; ++i) {
		s[i - start] = s[i];
	}
	s[len - start] = 0;
}

void NormalizeRunDir(WCHAR* dir) {
	TrimWide(dir);
	int len = 0;
	while (dir[len] != 0) {
		++len;
	}
	int start = 0;
	if (len >= 2 && dir[0] == L'"' && dir[len - 1] == L'"') {
		start = 1;
		--len;
	}
	for (int i = start; i < len; ++i) {
		dir[i - start] = dir[i];
	}
	len -= start;
	dir[len] = 0;
	for (int i = 0; i < len; ++i) {
		if (dir[i] == L'/') {
			dir[i] = L'\\';
		}
	}
	while (len > 3 && dir[len - 1] == L'\\') {
		dir[--len] = 0;
	}
}

bool TraceFileOpen() {
	EnterCriticalSection(&g_trace_lock);
	bool open = g_trace_file != INVALID_HANDLE_VALUE;
	LeaveCriticalSection(&g_trace_lock);
	return open;
}

bool TraceHelloGate() {
	if (InterlockedCompareExchange(&g_trace_enabled, 0, 0) == 0) {
		return true;
	}
	return InterlockedCompareExchange(&g_hello_written, 0, 0) != 0;
}

void TraceFailClosed() {
	InterlockedExchange(&g_trace_enabled, 1);
	InterlockedExchange(&g_trace_open_failed, 1);
	InterlockedExchange(&g_trace_error, 1);
}

void TraceInit() {
	InitializeCriticalSection(&g_trace_lock);
	g_pid = (uint32_t)GetCurrentProcessId();
	WCHAR dir[512];
	WCHAR run[160];
	DWORD dir_len = GetEnvironmentVariableW(L"VELQOR_RUN_DIR", dir, 512);
	DWORD run_len = GetEnvironmentVariableW(L"VELQOR_RUN_ID", run, 160);
	if (dir_len == 0 || run_len == 0) {
		return;
	}
	if (dir_len >= 512 || run_len >= 160) {
		TraceFailClosed();
		return;
	}
	TrimWide(run);
	char raw_id[160];
	int conv = WideCharToMultiByte(CP_UTF8, 0, run, -1, raw_id, (int)sizeof(raw_id) - 1, 0, 0);
	if (conv <= 0) {
		TraceFailClosed();
		return;
	}
	raw_id[sizeof(raw_id) - 1] = 0;
	JsonEscape(g_run_id, (int)sizeof(g_run_id), raw_id);
	NormalizeRunDir(dir);
	if (dir[0] == 0) {
		TraceFailClosed();
		return;
	}
	if (_snwprintf_s(g_trace_path_w, (int)(sizeof(g_trace_path_w) / sizeof((g_trace_path_w)[0])), _TRUNCATE, L"%s\\client_%lu.ndjson",
			dir, (unsigned long)g_pid) <= 0) {
		TraceFailClosed();
		return;
	}
	if (WideCharToMultiByte(CP_UTF8, 0, g_trace_path_w, -1, g_trace_path_a, (int)sizeof(g_trace_path_a) - 1, 0, 0) <= 0) {
		TraceFailClosed();
		return;
	}
	g_trace_path_a[sizeof(g_trace_path_a) - 1] = 0;
	g_trace_file = CreateFileW(g_trace_path_w, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, 0,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	InterlockedExchange(&g_trace_enabled, 1);
	if (g_trace_file == INVALID_HANDLE_VALUE) {
		InterlockedExchange(&g_trace_error, 1);
		InterlockedExchange(&g_trace_open_failed, 1);
	}
}

void EmitErrorMessage(const char* message) {
	char f[192];
	int n = _snprintf_s(f, (int)(sizeof(f) / sizeof((f)[0])), _TRUNCATE, "\"message\":\"%s\"", message);
	if (n > 0) {
		velqor::Emit("error", f);
	}
}

void TraceClose() {
	EnterCriticalSection(&g_trace_lock);
	InterlockedExchange(&g_trace_enabled, 0);
	HANDLE file = g_trace_file;
	g_trace_file = INVALID_HANDLE_VALUE;
	LeaveCriticalSection(&g_trace_lock);
	if (file != INVALID_HANDLE_VALUE) {
		CloseHandle(file);
	}
}

uint32_t QueueNextSeq() {
	return (uint32_t)InterlockedIncrement(&g_cmd_seq);
}

uint32_t QueuePush(Command* cmd) {
	EnterCriticalSection(&g_queue_lock);
	if (g_queue_count >= (uint32_t)kQueueCapacity) {
		LeaveCriticalSection(&g_queue_lock);
		return 0;
	}
	cmd->seq = (uint32_t)InterlockedIncrement(&g_cmd_seq);
	g_queue[(g_queue_head + g_queue_count) & (uint32_t)(kQueueCapacity - 1)] = *cmd;
	g_queue_count++;
	LeaveCriticalSection(&g_queue_lock);
	return cmd->seq;
}

void QueueClear() {
	EnterCriticalSection(&g_queue_lock);
	g_queue_head = 0;
	g_queue_count = 0;
	LeaveCriticalSection(&g_queue_lock);
}

bool HeldContains(uint32_t vk) {
	for (int i = 0; i < g_held_count; i++) {
		if (g_held[i] == vk) {
			return true;
		}
	}
	return false;
}

void HeldPush(uint32_t vk) {
	if (HeldContains(vk)) {
		return;
	}
	if (g_held_count < kHeldCapacity) {
		g_held[g_held_count++] = vk;
	}
}

void HeldRemove(uint32_t vk) {
	for (int i = 0; i < g_held_count; i++) {
		if (g_held[i] == vk) {
			for (int j = i; j + 1 < g_held_count; j++) {
				g_held[j] = g_held[j + 1];
			}
			g_held_count--;
			return;
		}
	}
}

void EmitKeyExec(const Command& c) {
	char f[224];
	int n = _snprintf_s(f, (int)(sizeof(f) / sizeof((f)[0])), _TRUNCATE,
		"\"cmd_seq\":%lu,\"op\":\"key\",\"vk\":%lu,\"down\":%lu,\"frame\":%llu,\"at_ms\":%llu",
		(unsigned long)c.seq, (unsigned long)c.vk, (unsigned long)c.down,
		(unsigned long long)velqor::CurrentFrame(), (unsigned long long)c.at_ms);
	if (n > 0) {
		velqor::Emit("cmd_exec", f);
	}
}

void EmitInputState(uint32_t seq, uint32_t vk, void* manager, const uint32_t* before = 0) {
	AckBuf fields;
	fields.raw("\"cmd_seq\":");
	fields.unum(seq);
	fields.raw(",\"vk\":");
	fields.unum(vk);
	fields.raw(",\"frame\":");
	fields.unum(velqor::CurrentFrame());
	fields.raw(",\"modifier_vk\":");
	fields.num(*(int32_t*)((uint8_t*)manager + 0x880u));
	fields.raw(",\"quickslot_locked\":");
	fields.raw(*((uint8_t*)manager + 0x8C8u) != 0 &&
		VaFn<IsInputTypeLockedFn>(kRvaIsInputTypeLocked)(manager, 14) != 0 ? "true" : "false");
	fields.raw(",\"held_types\":[");
	bool first = true;
	for (uint32_t type = 0; type < 0x9Cu; ++type) {
		if (*((uint8_t*)manager + 0x1A4u + type) == 0)
			continue;
		if (!first)
			fields.raw(",");
		fields.unum(type);
		first = false;
	}
	fields.raw("]");
	fields.raw(",\"activated_types\":[");
	first = true;
	if (before != 0) {
		const uint32_t* counters = (const uint32_t*)((uint8_t*)manager + 0x8CCu);
		for (uint32_t type = 0; type < 0x9Cu; ++type) {
			if (before[type] == counters[type])
				continue;
			if (!first)
				fields.raw(",");
			fields.unum(type);
			first = false;
		}
	}
	fields.raw("]");
	velqor::Emit("input_state", fields.b);
}

void EmitCursorExec(const Command& c) {
	char f[224];
	int n = _snprintf_s(f, (int)(sizeof(f) / sizeof((f)[0])), _TRUNCATE,
		"\"cmd_seq\":%lu,\"op\":\"cursor\",\"x\":%ld,\"y\":%ld,\"frame\":%llu,\"at_ms\":%llu",
		(unsigned long)c.seq, (long)c.x, (long)c.y,
		(unsigned long long)velqor::CurrentFrame(), (unsigned long long)c.at_ms);
	if (n > 0) {
		velqor::Emit("cmd_exec", f);
	}
}

void EmitMarkExec(const Command& c) {
	char id[160];
	JsonEscape(id, (int)sizeof(id), c.id);
	char f[288];
	int n = _snprintf_s(f, (int)(sizeof(f) / sizeof((f)[0])), _TRUNCATE,
		"\"cmd_seq\":%lu,\"op\":\"mark\",\"id\":\"%s\",\"frame\":%llu,\"at_ms\":%llu",
		(unsigned long)c.seq, id, (unsigned long long)velqor::CurrentFrame(),
		(unsigned long long)c.at_ms);
	if (n > 0) {
		velqor::Emit("cmd_exec", f);
	}
}

uint64_t ElapsedMs() {
	uint64_t epoch = (uint64_t)InterlockedCompareExchange64(&g_epoch_ns, 0, 0);
	uint64_t now = velqor::QpcNs();
	if (epoch == 0 || now <= epoch) {
		return 0;
	}
	return (now - epoch) / 1000000ull;
}

void EmitStopReleaseExec(uint32_t seq, uint32_t vk) {
	char f[224];
	int n = _snprintf_s(f, (int)(sizeof(f) / sizeof((f)[0])), _TRUNCATE,
		"\"cmd_seq\":%lu,\"op\":\"stop_release\",\"vk\":%lu,\"down\":0,\"frame\":%llu,\"at_ms\":%llu",
		(unsigned long)seq, (unsigned long)vk,
		(unsigned long long)velqor::CurrentFrame(), (unsigned long long)ElapsedMs());
	if (n > 0) {
		velqor::Emit("cmd_exec", f);
	}
}

void EmitStopReleaseEvent(const uint32_t* released, int count) {
	char f[8 + kHeldCapacity * 12];
	int n = _snprintf_s(f, (int)(sizeof(f) / sizeof((f)[0])), _TRUNCATE, "\"released\":[");
	if (n < 0) {
		velqor::Emit("stop_release", "\"released\":[]");
		return;
	}
	for (int i = 0; i < count; i++) {
		int r = _snprintf_s(f + n, (sizeof(f) - (size_t)n), _TRUNCATE, "%s%lu", (i != 0) ? "," : "",
			(unsigned long)released[i]);
		if (r < 0) {
			break;
		}
		n += r;
	}
	_snprintf_s(f + n, (sizeof(f) - (size_t)n), _TRUNCATE, "]");
	velqor::Emit("stop_release", f);
}

void DispatchNativeKey(void* manager, uint32_t vk, uint32_t down) {
	if (down != 0)
		VaFn<OnKeyDownFn>(kRvaOnKeyDown)(manager, vk, 0);
	else
		VaFn<OnKeyUpFn>(kRvaOnKeyUp)(manager, vk);
	if (vk != 1 && vk != 2 && vk != 4 && vk != 5 && vk != 6 && vk != 216 && vk != 217)
		VaFn<EmitRawVkEventFn>(kRvaEmitRawVkEvent)(manager, vk, down);
}

void ExecKey(const Command& c) {
	void* mgr = *(void**)Va(kRvaInputManagerGlobal);
	if (mgr == 0) {
		EmitErrorMessage("input_manager_unavailable");
		return;
	}
	if (c.down != 0 && (*((uint8_t*)mgr + 0x890u) != 0 || *((uint8_t*)mgr + 0x891u) != 0)) {
		EmitErrorMessage("input_manager_captured");
		return;
	}
	uint32_t before[0x9C];
	memcpy(before, (uint8_t*)mgr + 0x8CCu, sizeof(before));
	if (c.down != 0) {
		velqor::os::SetVirtualKey(c.vk, 1);
		DispatchNativeKey(mgr, c.vk, 1);
		HeldPush(c.vk);
	} else {
		velqor::os::SetVirtualKey(c.vk, 0);
		DispatchNativeKey(mgr, c.vk, 0);
		HeldRemove(c.vk);
	}
	BridgeNotifyKey(c.vk, (c.down != 0) ? 1 : 0);
	EmitKeyExec(c);
	EmitInputState(c.seq, c.vk, mgr, before);
}

void ExecCursor(const Command& c) {
	if (!BridgeSetCursor(c.x, c.y)) {
		EmitErrorMessage("cursor_manager_unavailable");
	}
	EmitCursorExec(c);
}

void ExecMark(const Command& c) {
	EmitMarkExec(c);
}

void ExecStopRelease(uint32_t seq) {
	uint32_t released[kHeldCapacity];
	int count = 0;
	void* mgr = *(void**)Va(kRvaInputManagerGlobal);
	while (g_held_count > 0) {
		uint32_t vk = g_held[--g_held_count];
		velqor::os::SetVirtualKey(vk, 0);
		if (mgr != 0) {
			DispatchNativeKey(mgr, vk, 0);
		}
		BridgeNotifyKey(vk, 0);
		EmitStopReleaseExec(seq, vk);
		if (mgr != 0)
			EmitInputState(seq, vk, mgr);
		released[count++] = vk;
	}
	BridgeOnStop();
	for (int i = 0; i < count; i++) {
		g_stop_released[i] = released[i];
	}
	InterlockedExchange(&g_stop_released_count, (LONG)count);
	EmitStopReleaseEvent(released, count);
	InterlockedExchange(&g_stop_result, 1);
	if (g_stop_done != 0) {
		SetEvent(g_stop_done);
	}
}

void RequestStop(uint32_t seq) {
	InterlockedExchange(&g_auto_active, 0);
	InterlockedExchange(&g_stop_seq, (LONG)seq);
	InterlockedExchange(&g_stop_pending, 1);
}

void SafetyRelease() {
	RequestStop(QueueNextSeq());
}

void DrainCommands() {
	uint32_t used_vks[kUsedVkCapacity];
	int used = 0;
	for (;;) {
		Command c;
		EnterCriticalSection(&g_queue_lock);
		if (g_queue_count == 0) {
			LeaveCriticalSection(&g_queue_lock);
			break;
		}
		c = g_queue[g_queue_head];
		LeaveCriticalSection(&g_queue_lock);
		if (c.due_ns != 0 && c.due_ns > velqor::QpcNs()) {
			break;
		}
		if (c.op == OpKey) {
			bool conflict = false;
			for (int i = 0; i < used; i++) {
				if (used_vks[i] == c.vk) {
					conflict = true;
				}
			}
			if (conflict) {
				break;
			}
			if (used < kUsedVkCapacity) {
				used_vks[used++] = c.vk;
			}
		}
		EnterCriticalSection(&g_queue_lock);
		g_queue_head = (g_queue_head + 1) & (uint32_t)(kQueueCapacity - 1);
		g_queue_count--;
		LeaveCriticalSection(&g_queue_lock);
		BridgeSetPendingSeq(c.seq);
		if (c.op == OpKey) {
			ExecKey(c);
		} else if (c.op == OpCursor) {
			ExecCursor(c);
		} else if (c.op == OpMark) {
			ExecMark(c);
		}
	}
}

void NoteClientWindow() {
	if (InterlockedCompareExchange(&g_hwnd_state, 0, 0) != 0) {
		return;
	}
	HWND hwnd = *(HWND*)Va(kRvaDiHwndGlobal);
	if (hwnd == 0) {
		return;
	}
	if (!IsWindow(hwnd)) {
		InterlockedExchange(&g_hwnd_state, 2);
		return;
	}
	DWORD owner = 0;
	GetWindowThreadProcessId(hwnd, &owner);
	if (owner != GetCurrentProcessId()) {
		InterlockedExchange(&g_hwnd_state, 2);
		return;
	}
	char cls[40];
	if (GetClassNameA(hwnd, cls, (int)sizeof(cls)) == 0 ||
		lstrcmpA(cls, kClientWindowClass) != 0) {
		InterlockedExchange(&g_hwnd_state, 2);
		return;
	}
	velqor::os::NoteClientHwnd(hwnd);
	InterlockedExchange(&g_hwnd_state, 1);
}

void __fastcall HookedUpdateFrame(void* self) {
	InterlockedIncrement64(&g_frame);
	NoteClientWindow();
	if (InterlockedCompareExchange(&g_bridge_inited, 0, 0) == 0) {
		void* mgr = *(void**)Va(kRvaInputManagerGlobal);
		void* session = *(void**)Va(kRvaGameSessionGlobal);
		if (mgr != 0 && session != 0 && BridgeInitialize()) {
			InterlockedExchange(&g_bridge_inited, 1);
		}
	}
	if (InterlockedExchange(&g_play_pending, 0) != 0) {
		InterlockedExchange(&g_play_result, BridgePlay() ? 1 : 0);
		if (g_play_done != 0) {
			SetEvent(g_play_done);
		}
	}
	if (InterlockedExchange(&g_stop_pending, 0) != 0) {
		QueueClear();
		ExecStopRelease((uint32_t)InterlockedCompareExchange(&g_stop_seq, 0, 0));
	}
	else if (InterlockedCompareExchange(&g_auto_active, 0, 0) != 0) {
		DrainCommands();
	}
	LONG inited = InterlockedCompareExchange(&g_bridge_inited, 0, 0);
	if (inited != 0) {
		BridgeBeforeFrame();
	}
	if (g_orig_update_frame != 0) {
		g_orig_update_frame(self);
	}
	if (inited != 0) {
		BridgeAfterFrame();
	}
}

void AbortAttach(const char* token) {
	EmitErrorMessage(token);
	TerminateProcess(GetCurrentProcess(), (UINT)kAbortExitCode);
	for (;;) {
		Sleep(1000);
	}
}

bool InstallUpdateFrameHook() {
	void** slot = (void**)Va(kRvaVtableUpdateFrameSlot);
	void* expected = (void*)(g_base + kRvaUpdateFrame);
	if (*slot != expected) {
		return false;
	}
	g_orig_update_frame = VaFn<UpdateFrameFn>(kRvaUpdateFrame);
	if (!velqor::HookAttach(slot, (void*)&HookedUpdateFrame)) {
		g_orig_update_frame = 0;
		return false;
	}
	return *slot == (void*)&HookedUpdateFrame;
}

void RemoveUpdateFrameHook() {
	if (g_orig_update_frame == 0) {
		return;
	}
	velqor::HookDetach((void**)Va(kRvaVtableUpdateFrameSlot), (void*)&HookedUpdateFrame);
	g_orig_update_frame = 0;
}

void EmitHello() {
	if (!TraceFileOpen()) {
		return;
	}
	char hex[65];
	for (int i = 0; i < 32; i++) {
		unsigned char v = g_exe_digest[i];
		hex[i * 2] = "0123456789abcdef"[v >> 4];
		hex[i * 2 + 1] = "0123456789abcdef"[v & 15];
	}
	hex[64] = 0;
	char f[160];
	int n = _snprintf_s(f, (int)(sizeof(f) / sizeof((f)[0])), _TRUNCATE, "\"module_base\":%llu,\"exe_sha256\":\"%s\"",
		(unsigned long long)g_base, hex);
	if (n > 0) {
		velqor::Emit("hello", f);
		if (InterlockedCompareExchange(&g_trace_error, 0, 0) == 0) {
			InterlockedExchange(&g_hello_written, 1);
		}
	}
}

bool VerifyModule() {
	WCHAR path[1024];
	DWORD len = GetModuleFileNameW(0, path, (DWORD)(sizeof(path) / sizeof(path[0])));
	if (len == 0 || len >= (DWORD)(sizeof(path) / sizeof(path[0]))) {
		return false;
	}
	int ok = VelqorFileSha256(path, g_exe_digest);
	if (!ok) {
		return false;
	}
	return VelqorSha256EqHex(g_exe_digest, kExeSha256Hex) != 0;
}

bool ParseU32(const char* s, uint32_t* out) {
	uint64_t v = 0;
	if (s == 0 || *s == 0) {
		return false;
	}
	for (const char* p = s; *p != 0; ++p) {
		if (*p < '0' || *p > '9') {
			return false;
		}
		if (v > 429496729ull) {
			return false;
		}
		v = v * 10ull + (uint64_t)(*p - '0');
		if (v > 0xFFFFFFFFull) {
			return false;
		}
	}
	*out = (uint32_t)v;
	return true;
}

bool ParseU64(const char* s, uint64_t* out) {
	uint64_t v = 0;
	if (s == 0 || *s == 0) {
		return false;
	}
	for (const char* p = s; *p != 0; ++p) {
		if (*p < '0' || *p > '9') {
			return false;
		}
		uint64_t digit = (uint64_t)(*p - '0');
		if (v > (0xFFFFFFFFFFFFFFFFull - digit) / 10ull) {
			return false;
		}
		v = v * 10ull + digit;
	}
	*out = v;
	return true;
}

bool ParseI32(const char* s, int32_t* out) {
	int64_t v = 0;
	int sign = 0;
	if (s == 0 || *s == 0) {
		return false;
	}
	const char* p = s;
	if (*p == '-') {
		sign = 1;
		p++;
	}
	if (*p == 0) {
		return false;
	}
	for (; *p != 0; ++p) {
		if (*p < '0' || *p > '9') {
			return false;
		}
		if (v > 214748364ll) {
			return false;
		}
		v = v * 10 + (*p - '0');
		if (v > 2147483648ll) {
			return false;
		}
	}
	if (sign != 0) {
		v = -v;
	}
	if (v < -2147483648ll || v > 2147483647ll) {
		return false;
	}
	*out = (int32_t)v;
	return true;
}

uint64_t CommandDueNs(uint64_t at_ms) {
	uint64_t epoch = (uint64_t)InterlockedCompareExchange64(&g_epoch_ns, 0, 0);
	if (epoch == 0) {
		return 0;
	}
	return epoch + at_ms * 1000000ull;
}

bool SendAck(HANDLE pipe, const char* json) {
	char out[2200];
	int len = (int)strlen(json);
	if (len <= 0 || len + 1 > (int)sizeof(out)) {
		return false;
	}
	memcpy(out, json, (size_t)len);
	out[len++] = '\n';
	DWORD wrote = 0;
	return WriteFile(pipe, out, (DWORD)len, &wrote, 0) && wrote == (DWORD)len;
}

void AckError(HANDLE pipe, uint32_t seq, const char* reason) {
	AckBuf out;
	out.raw("{\"status\":\"error\",\"seq\":");
	out.unum(seq);
	out.raw(",\"error\":\"");
	out.str(reason);
	out.raw("\"}");
	if (out.truncated) {
		SendAck(pipe, "{\"status\":\"error\",\"seq\":0,\"error\":\"ack_overflow\"}");
		return;
	}
	SendAck(pipe, out.b);
}

void AckStatus(HANDLE pipe, uint32_t seq) {
	velqor::os::Viewport vp = velqor::os::GetViewport();
	int32_t cx = 0;
	int32_t cy = 0;
	velqor::os::GetVirtualCursor(&cx, &cy);
	AckBuf out;
	out.raw("{\"status\":\"ok\",\"seq\":");
	out.unum(seq);
	out.raw(",\"ready\":");
	out.raw((BridgeReady() && TraceHelloGate()) ? "true" : "false");
	out.raw(",\"phase\":\"");
	out.str(BridgePhaseName());
	out.raw("\",\"error\":\"");
	if (InterlockedCompareExchange(&g_trace_open_failed, 0, 0) != 0) {
		out.raw("trace_open_failed");
	} else {
		out.str(BridgeLastError());
	}
	out.raw("\",\"unfocused\":");
	out.raw(BridgeUnfocused() ? "true" : "false");
	out.raw(",\"tracepath\":\"");
	out.str(g_trace_path_a);
	out.raw("\",\"viewport_w\":");
	out.num(vp.width);
	out.raw(",\"viewport_h\":");
	out.num(vp.height);
	out.raw(",\"cursor_x\":");
	out.num(cx);
	out.raw(",\"cursor_y\":");
	out.num(cy);
	out.raw(",\"frame\":");
	out.unum(velqor::CurrentFrame());
	out.raw(",\"qpc_ns\":");
	out.unum(velqor::QpcNs());
	out.raw("}");
	if (out.truncated) {
		AckError(pipe, seq, "ack_overflow");
		return;
	}
	SendAck(pipe, out.b);
}

void AckQueued(HANDLE pipe, uint32_t seq, uint32_t cmd_seq) {
	AckBuf out;
	out.raw("{\"status\":\"queued\",\"seq\":");
	out.unum(seq);
	out.raw(",\"cmd_seq\":");
	out.unum(cmd_seq);
	out.raw("}");
	if (out.truncated) {
		AckError(pipe, seq, "ack_overflow");
		return;
	}
	SendAck(pipe, out.b);
}

void AckBegin(HANDLE pipe, uint32_t seq) {
	uint64_t epoch = velqor::QpcNs();
	InterlockedExchange64(&g_epoch_ns, (LONGLONG)epoch);
	InterlockedExchange(&g_auto_active, 1);
	uint32_t next_seq = (uint32_t)InterlockedCompareExchange(&g_cmd_seq, 0, 0) + 1;
	AckBuf out;
	out.raw("{\"status\":\"ok\",\"seq\":");
	out.unum(seq);
	out.raw(",\"epoch_ns\":");
	out.unum(epoch);
	out.raw(",\"cmd_seq\":");
	out.unum(next_seq);
	out.raw("}");
	if (out.truncated) {
		AckError(pipe, seq, "ack_overflow");
		return;
	}
	SendAck(pipe, out.b);
}

void AckPlay(HANDLE pipe, uint32_t seq) {
	if (g_play_done == 0) {
		AckError(pipe, seq, "play_unavailable");
		return;
	}
	ResetEvent(g_play_done);
	InterlockedExchange(&g_play_result, 0);
	InterlockedExchange(&g_play_pending, 1);
	DWORD waited = WaitForSingleObject(g_play_done, 3000);
	bool ok = (waited == WAIT_OBJECT_0) && (InterlockedCompareExchange(&g_play_result, 0, 0) != 0);
	AckBuf out;
	out.raw("{\"status\":\"");
	out.raw(ok ? "ok" : "error");
	out.raw("\",\"seq\":");
	out.unum(seq);
	out.raw(",\"phase\":\"");
	out.str(BridgePhaseName());
	out.raw("\"}");
	if (out.truncated) {
		AckError(pipe, seq, "ack_overflow");
		return;
	}
	SendAck(pipe, out.b);
}

void AckStop(HANDLE pipe, uint32_t seq) {
	uint32_t stop_seq = QueueNextSeq();
	uint32_t released[kHeldCapacity];
	uint32_t count = 0;
	if (g_stop_done != 0) {
		ResetEvent(g_stop_done);
	}
	InterlockedExchange(&g_stop_released_count, 0);
	InterlockedExchange(&g_stop_result, 0);
	RequestStop(stop_seq);
	if (g_stop_done != 0) {
		WaitForSingleObject(g_stop_done, 3000);
	}
	LONG recorded = InterlockedCompareExchange(&g_stop_released_count, 0, 0);
	if (recorded < 0) {
		recorded = 0;
	}
	if (recorded > kHeldCapacity) {
		recorded = kHeldCapacity;
	}
	for (LONG i = 0; i < recorded; i++) {
		released[count++] = g_stop_released[i];
	}
	AckBuf out;
	out.raw("{\"status\":\"ok\",\"seq\":");
	out.unum(seq);
	out.raw(",\"released\":[");
	for (uint32_t i = 0; i < count; i++) {
		if (i != 0) {
			out.raw(",");
		}
		out.unum(released[i]);
	}
	out.raw("]}");
	if (out.truncated) {
		AckError(pipe, seq, "ack_overflow");
		return;
	}
	SendAck(pipe, out.b);
}

void HandleRequest(HANDLE pipe, char* line) {
	char* tok[8];
	int ntok = 0;
	char* p = line;
	while (ntok < 8 && *p != 0) {
		while (*p == ' ' || *p == '\t') {
			p++;
		}
		if (*p == 0) {
			break;
		}
		tok[ntok++] = p;
		while (*p != 0 && *p != ' ' && *p != '\t') {
			p++;
		}
		if (*p != 0) {
			*p = 0;
			p++;
		}
	}
	if (ntok < 2) {
		AckError(pipe, 0, "bad_request");
		return;
	}
	uint32_t seq = 0;
	if (!ParseU32(tok[1], &seq)) {
		AckError(pipe, 0, "bad_seq");
		return;
	}
	const char* verb = tok[0];
	if (strcmp(verb, "STATUS") == 0) {
		if (ntok != 2) {
			AckError(pipe, seq, "bad_request");
			return;
		}
		AckStatus(pipe, seq);
		return;
	}
	if (strcmp(verb, "BEGIN") == 0) {
		if (ntok != 2) {
			AckError(pipe, seq, "bad_request");
			return;
		}
		AckBegin(pipe, seq);
		return;
	}
	if (strcmp(verb, "KEY") == 0) {
		uint64_t at_ms = 0;
		uint32_t vk = 0;
		uint32_t down = 0;
		if (ntok != 5 || !ParseU64(tok[2], &at_ms) || !ParseU32(tok[3], &vk) || !ParseU32(tok[4], &down) ||
			down > 1) {
			AckError(pipe, seq, "bad_request");
			return;
		}
		Command c;
		memset(&c, 0, sizeof(c));
		c.op = OpKey;
		c.vk = vk;
		c.down = down;
		c.at_ms = at_ms;
		c.due_ns = CommandDueNs(at_ms);
		uint32_t cmd_seq = QueuePush(&c);
		if (cmd_seq == 0) {
			AckError(pipe, seq, "queue_full");
			SafetyRelease();
			return;
		}
		AckQueued(pipe, seq, cmd_seq);
		return;
	}
	if (strcmp(verb, "CURSOR") == 0) {
		uint64_t at_ms = 0;
		int32_t x = 0;
		int32_t y = 0;
		if (ntok != 5 || !ParseU64(tok[2], &at_ms) || !ParseI32(tok[3], &x) || !ParseI32(tok[4], &y)) {
			AckError(pipe, seq, "bad_request");
			return;
		}
		Command c;
		memset(&c, 0, sizeof(c));
		c.op = OpCursor;
		c.x = x;
		c.y = y;
		c.at_ms = at_ms;
		c.due_ns = CommandDueNs(at_ms);
		uint32_t cmd_seq = QueuePush(&c);
		if (cmd_seq == 0) {
			AckError(pipe, seq, "queue_full");
			SafetyRelease();
			return;
		}
		AckQueued(pipe, seq, cmd_seq);
		return;
	}
	if (strcmp(verb, "MARK") == 0) {
		uint64_t at_ms = 0;
		if (ntok != 4 || !ParseU64(tok[2], &at_ms) || tok[3][0] == 0) {
			AckError(pipe, seq, "bad_request");
			return;
		}
		Command c;
		memset(&c, 0, sizeof(c));
		c.op = OpMark;
		c.at_ms = at_ms;
		c.due_ns = CommandDueNs(at_ms);
		strncpy_s(c.id, sizeof(c.id), tok[3], _TRUNCATE);
		uint32_t cmd_seq = QueuePush(&c);
		if (cmd_seq == 0) {
			AckError(pipe, seq, "queue_full");
			SafetyRelease();
			return;
		}
		AckQueued(pipe, seq, cmd_seq);
		return;
	}
	if (strcmp(verb, "PLAY") == 0) {
		if (ntok != 2) {
			AckError(pipe, seq, "bad_request");
			return;
		}
		AckPlay(pipe, seq);
		return;
	}
	if (strcmp(verb, "STOP") == 0) {
		if (ntok != 2) {
			AckError(pipe, seq, "bad_request");
			return;
		}
		AckStop(pipe, seq);
		return;
	}
	AckError(pipe, seq, "unknown_request");
}

void ServeConnection(HANDLE pipe) {
	char line[kLineCapacity];
	int len = 0;
	int dropping = 0;
	char buf[512];
	for (;;) {
		DWORD got = 0;
		if (!ReadFile(pipe, buf, (DWORD)sizeof(buf), &got, 0) || got == 0) {
			break;
		}
		for (DWORD i = 0; i < got; i++) {
			char ch = buf[i];
			if (ch == '\n' || ch == '\r') {
				if (dropping != 0) {
					dropping = 0;
					len = 0;
					continue;
				}
				if (len == 0) {
					continue;
				}
				line[len] = 0;
				HandleRequest(pipe, line);
				len = 0;
				continue;
			}
			if (dropping != 0) {
				continue;
			}
			if (len + 1 >= kLineCapacity) {
				dropping = 1;
				len = 0;
				AckError(pipe, 0, "request_overflow");
				SafetyRelease();
				continue;
			}
			line[len++] = ch;
		}
	}
}

int PipePrivilegeSet(int enable) {
	HANDLE token = 0;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
		return 0;
	}
	LUID luid;
	int assigned = 0;
	if (LookupPrivilegeValueW(0, SE_SECURITY_NAME, &luid)) {
		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = luid;
		tp.Privileges[0].Attributes = (enable != 0) ? SE_PRIVILEGE_ENABLED : 0;
		SetLastError(ERROR_SUCCESS);
		if (AdjustTokenPrivileges(token, FALSE, &tp, (DWORD)sizeof(tp), 0, 0) && GetLastError() == ERROR_SUCCESS) {
			assigned = 1;
		}
	}
	CloseHandle(token);
	return assigned;
}

int PipeSecurityDacl() {
	HANDLE token = 0;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
		return 0;
	}
	unsigned char info[sizeof(TOKEN_USER) + SECURITY_MAX_SID_SIZE];
	DWORD len = 0;
	int ok = 0;
	if (GetTokenInformation(token, TokenUser, info, (DWORD)sizeof(info), &len)) {
		PSID user = ((TOKEN_USER*)info)->User.Sid;
		PACL dacl = (PACL)g_pipe_dacl;
		if (user != 0 && IsValidSid(user) != FALSE &&
			InitializeAcl(dacl, (DWORD)sizeof(g_pipe_dacl), ACL_REVISION) != FALSE &&
			AddAccessAllowedAce(dacl, ACL_REVISION, FILE_GENERIC_READ | FILE_GENERIC_WRITE, user) != FALSE &&
			InitializeSecurityDescriptor(&g_pipe_sd, SECURITY_DESCRIPTOR_REVISION) != FALSE &&
			SetSecurityDescriptorDacl(&g_pipe_sd, TRUE, dacl, FALSE) != FALSE) {
			g_pipe_sa.nLength = (DWORD)sizeof(g_pipe_sa);
			g_pipe_sa.lpSecurityDescriptor = &g_pipe_sd;
			g_pipe_sa.bInheritHandle = FALSE;
			ok = 1;
		}
	}
	CloseHandle(token);
	return ok;
}

int PipeSecurityLabel() {
	unsigned char label[SECURITY_MAX_SID_SIZE];
	DWORD len = (DWORD)sizeof(label);
	PSID sid = (PSID)label;
	if (CreateWellKnownSid(WinMediumLabelSid, 0, sid, &len) == FALSE) {
		return 0;
	}
	PACL sacl = (PACL)g_pipe_sacl;
	if (InitializeAcl(sacl, (DWORD)sizeof(g_pipe_sacl), ACL_REVISION) == FALSE) {
		return 0;
	}
	if (AddMandatoryAce(sacl, ACL_REVISION, 0, SYSTEM_MANDATORY_LABEL_NO_WRITE_UP, sid) == FALSE) {
		return 0;
	}
	if (SetSecurityDescriptorSacl(&g_pipe_sd, TRUE, sacl, FALSE) == FALSE) {
		return 0;
	}
	return 1;
}

DWORD WINAPI PipeThreadMain(void*) {
	_snwprintf_s(g_pipe_name, (int)(sizeof(g_pipe_name) / sizeof((g_pipe_name)[0])), _TRUNCATE, L"\\\\.\\pipe\\velqor-%lu", (unsigned long)g_pid);
	if (PipeSecurityDacl() == 0) {
		EmitErrorMessage("pipe_security_dacl_failed");
		return 0;
	}
	if (PipeSecurityLabel() != 0) {
		g_pipe_label_set = 1;
	} else {
		EmitErrorMessage("pipe_security_label_failed");
	}
	for (;;) {
		if (InterlockedCompareExchange(&g_pipe_stop, 0, 0) != 0) {
			return 0;
		}
		int privilege = 0;
		if (g_pipe_label_set != 0) {
			privilege = PipePrivilegeSet(1);
		}
		HANDLE pipe = CreateNamedPipeW(g_pipe_name, PIPE_ACCESS_DUPLEX,
			PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, 8192, 8192, 0, &g_pipe_sa);
		if (privilege != 0) {
			PipePrivilegeSet(0);
		}
		if (pipe == INVALID_HANDLE_VALUE && g_pipe_label_set != 0 && GetLastError() == ERROR_PRIVILEGE_NOT_HELD) {
			g_pipe_label_set = 0;
			SetSecurityDescriptorSacl(&g_pipe_sd, FALSE, 0, FALSE);
			EmitErrorMessage("pipe_security_label_denied");
			pipe = CreateNamedPipeW(g_pipe_name, PIPE_ACCESS_DUPLEX,
				PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, 8192, 8192, 0, &g_pipe_sa);
		}
		if (pipe == INVALID_HANDLE_VALUE) {
			if (g_pipe_error_logged == 0) {
				g_pipe_error_logged = 1;
				EmitErrorMessage("pipe_create_failed");
			}
			Sleep(1000);
			continue;
		}
		BOOL connected = ConnectNamedPipe(pipe, 0);
		if (!connected && GetLastError() != ERROR_PIPE_CONNECTED) {
			CloseHandle(pipe);
			Sleep(100);
			continue;
		}
		ServeConnection(pipe);
		FlushFileBuffers(pipe);
		DisconnectNamedPipe(pipe);
		CloseHandle(pipe);
		if (InterlockedCompareExchange(&g_pipe_stop, 0, 0) == 0) {
			SafetyRelease();
		}
	}
}

int VelqorDllAttachImpl() {
	if (InterlockedCompareExchange(&g_attached, 1, 0) != 0) {
		return 0;
	}
	TraceInit();
	g_base = (uintptr_t)GetModuleHandleW(0);
	if (g_base == 0) {
		AbortAttach("module_base_unavailable");
	}
	if (!VerifyModule()) {
		AbortAttach("exe_sha256_mismatch");
	}
	if (!velqor::os::Initialize()) {
		AbortAttach("os_isolation_init_failed");
	}
	InitializeCriticalSection(&g_queue_lock);
	g_play_done = CreateEventW(0, TRUE, FALSE, 0);
	g_stop_done = CreateEventW(0, TRUE, FALSE, 0);
	if (!InstallUpdateFrameHook()) {
		AbortAttach("update_frame_hook_failed");
	}
	EmitHello();
	g_pipe_thread = CreateThread(0, 0, PipeThreadMain, 0, 0, 0);
	return 0;
}

void VelqorDllDetachImpl(int process_exit) {
	if (InterlockedCompareExchange(&g_attached, 0, 1) != 1) {
		return;
	}
	InterlockedExchange(&g_pipe_stop, 1);
	if (g_pipe_thread != 0) {
		if (process_exit == 0) {
			CancelSynchronousIo(g_pipe_thread);
			WaitForSingleObject(g_pipe_thread, 2000);
		}
		CloseHandle(g_pipe_thread);
		g_pipe_thread = 0;
	}
	RemoveUpdateFrameHook();
	TraceClose();
	if (g_play_done != 0) {
		CloseHandle(g_play_done);
		g_play_done = 0;
	}
	if (g_stop_done != 0) {
		CloseHandle(g_stop_done);
		g_stop_done = 0;
	}
}

}

namespace velqor {

void Emit(const char* event, const char* json_fields) {
	if (InterlockedCompareExchange(&g_trace_enabled, 0, 0) == 0 || event == 0) {
		return;
	}
	EmitInternal(event, json_fields);
}

uint64_t QpcNs() {
	LARGE_INTEGER counter;
	if (!QueryPerformanceCounter(&counter)) {
		return 0;
	}
	uint64_t freq = QpcFreqValue();
	if (freq == 0) {
		return 0;
	}
	uint64_t count = (uint64_t)counter.QuadPart;
	return (count / freq) * 1000000000ull + ((count % freq) * 1000000000ull) / freq;
}

bool AutomationActive() {
	return InterlockedCompareExchange(&g_auto_active, 0, 0) != 0;
}

uint64_t CurrentFrame() {
	return (uint64_t)InterlockedCompareExchange64(&g_frame, 0, 0);
}

bool HookTransactionBegin() {
	return DetourTransactionBegin() == NO_ERROR;
}

bool HookUpdateThread(void* thread) {
	return DetourUpdateThread((HANDLE)thread) == NO_ERROR;
}

bool HookAttach(void** target, void* detour) {
	if (target == 0 || detour == 0) {
		return false;
	}
	void* original = *target;
	if (original == detour) {
		return false;
	}
	while (InterlockedExchange(&g_hook_lock, 1) != 0) {
		Sleep(0);
	}
	int slot = -1;
	for (int i = 0; i < kHookCapacity; i++) {
		if (g_hooks[i].target == target) {
			slot = i;
			break;
		}
		if (slot < 0 && g_hooks[i].target == 0) {
			slot = i;
		}
	}
	if (slot < 0) {
		InterlockedExchange(&g_hook_lock, 0);
		return false;
	}
	DWORD old_protect = 0;
	if (!VirtualProtect(target, sizeof(void*), PAGE_EXECUTE_READWRITE, &old_protect)) {
		InterlockedExchange(&g_hook_lock, 0);
		return false;
	}
	*target = detour;
	DWORD ignored = 0;
	VirtualProtect(target, sizeof(void*), old_protect, &ignored);
	g_hooks[slot].target = target;
	g_hooks[slot].original = original;
	InterlockedExchange(&g_hook_lock, 0);
	return true;
}

bool HookDetach(void** target, void* detour) {
	if (target == 0) {
		return false;
	}
	while (InterlockedExchange(&g_hook_lock, 1) != 0) {
		Sleep(0);
	}
	int slot = -1;
	for (int i = 0; i < kHookCapacity; i++) {
		if (g_hooks[i].target == target) {
			slot = i;
			break;
		}
	}
	if (slot < 0 || *target != detour) {
		InterlockedExchange(&g_hook_lock, 0);
		return false;
	}
	void* original = g_hooks[slot].original;
	DWORD old_protect = 0;
	if (!VirtualProtect(target, sizeof(void*), PAGE_EXECUTE_READWRITE, &old_protect)) {
		InterlockedExchange(&g_hook_lock, 0);
		return false;
	}
	*target = original;
	DWORD ignored = 0;
	VirtualProtect(target, sizeof(void*), old_protect, &ignored);
	g_hooks[slot].target = 0;
	g_hooks[slot].original = 0;
	InterlockedExchange(&g_hook_lock, 0);
	return true;
}

bool HookTransactionCommit() {
	return DetourTransactionCommit() == NO_ERROR;
}

void* HookGetProc(const char* module, const char* name) {
	if (module == 0 || name == 0) {
		return 0;
	}
	PVOID found = DetourFindFunction(module, name);
	if (found != 0) {
		return found;
	}
	HMODULE handle = GetModuleHandleA(module);
	if (handle == 0) {
		return 0;
	}
	return (void*)GetProcAddress(handle, name);
}

}

extern "C" int VelqorDllAttach() {
	return VelqorDllAttachImpl();
}

extern "C" void VelqorDllDetach(int process_exit) {
	VelqorDllDetachImpl(process_exit);
}
