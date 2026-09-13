#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "detours.h"
#include "sha256.h"

namespace {

const char kPinnedExeSha256[] = "df32850dd8e7a2329ddca2dc57a3eb310a620f4a6477bbb135049a938f477583";

const unsigned long long kFiletimeUnixEpoch = 116444736000000000ULL;

const int kBufferChars = 32768;
const int kMaxArgs = 128;

const DWORD kOffscreenWindowX = 10000;
const DWORD kOffscreenWindowY = 10000;

wchar_t g_source[kBufferChars];
wchar_t g_exe[kBufferChars];
wchar_t g_dll[kBufferChars];
char g_dll_ansi[kBufferChars];
wchar_t g_cwd[kBufferChars];
wchar_t g_child[kBufferChars];
wchar_t g_json_path[kBufferChars];
wchar_t g_json_final[kBufferChars];
wchar_t g_env_block[kBufferChars];

void Fail(const char* token) {
	printf("VELQOR_ERROR %s\n", token);
	fflush(stdout);
	ExitProcess(2);
}

void FailCode(const char* token, DWORD code) {
	fprintf(stderr, "VELQOR_DETAIL code=%lu\n", (unsigned long)code);
	fflush(stderr);
	Fail(token);
}

void NormalizeSlashes(wchar_t* path) {
	for (wchar_t* p = path; *p != 0; ++p) {
		if (*p == L'/') {
			*p = L'\\';
		}
	}
}

void ResolvePath(const wchar_t* value, wchar_t* out, const char* fail_token) {
	DWORD n = GetFullPathNameW(value, (DWORD)kBufferChars, out, NULL);
	if (n == 0 || n >= (DWORD)kBufferChars) {
		Fail(fail_token);
	}
	NormalizeSlashes(out);
}

bool HasEnvironmentVariable(const wchar_t* name) {
	wchar_t value[1024];
	return GetEnvironmentVariableW(name, value, (DWORD)(sizeof(value) / sizeof(value[0]))) > 0;
}

void BuildChildEnvironment() {
	if (!HasEnvironmentVariable(L"VELQOR_RUN_DIR") || !HasEnvironmentVariable(L"VELQOR_RUN_ID")) {
		Fail("env");
	}
	wchar_t* src = GetEnvironmentStringsW();
	if (src == NULL) {
		Fail("env");
	}
	int n = 0;
	for (const wchar_t* p = src;; ++p) {
		if (*p == 0 && p[1] == 0) {
			if (n + 2 > kBufferChars) {
				FreeEnvironmentStringsW(src);
				Fail("env");
			}
			g_env_block[n++] = 0;
			g_env_block[n++] = 0;
			break;
		}
		if (n + 2 > kBufferChars) {
			FreeEnvironmentStringsW(src);
			Fail("env");
		}
		g_env_block[n++] = *p;
	}
	FreeEnvironmentStringsW(src);
}

bool Elevated() {
	HANDLE token = NULL;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
		return false;
	}
	TOKEN_ELEVATION elevation;
	ZeroMemory(&elevation, sizeof(elevation));
	DWORD cb = 0;
	bool elevated = false;
	if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &cb)) {
		elevated = elevation.TokenIsElevated != 0;
	}
	CloseHandle(token);
	return elevated;
}

bool IsSpace(wchar_t c) {
	return c == L' ' || c == L'\t';
}

int SplitCommandLine(wchar_t* line, wchar_t** argv, int cap, wchar_t** tail) {
	int argc = 0;
	wchar_t* p = line;
	*tail = NULL;
	bool after_marker = false;
	while (*p != 0) {
		while (*p != 0 && IsSpace(*p)) {
			++p;
		}
		if (*p == 0) {
			break;
		}
		if (after_marker) {
			*tail = p;
			break;
		}
		if (argc >= cap) {
			return -1;
		}
		wchar_t* dst = p;
		wchar_t* start = p;
		bool quoted = false;
		while (*p != 0 && (quoted || !IsSpace(*p))) {
			if (*p == L'\\') {
				int slashes = 0;
				while (*p == L'\\') {
					++p;
					++slashes;
				}
				if (*p == L'"') {
					for (int i = 0; i < slashes / 2; ++i) {
						*dst++ = L'\\';
					}
					if ((slashes & 1) == 0) {
						quoted = !quoted;
					} else {
						*dst++ = L'"';
					}
					++p;
				} else {
					for (int i = 0; i < slashes; ++i) {
						*dst++ = L'\\';
					}
				}
			} else if (*p == L'"') {
				if (quoted && p[1] == L'"') {
					*dst++ = L'"';
					p += 2;
				} else {
					quoted = !quoted;
					++p;
				}
			} else {
				*dst++ = *p++;
			}
		}
		wchar_t* end = p;
		bool at_end = *end == 0;
		*dst = 0;
		argv[argc++] = start;
		if (dst == end) {
			p = at_end ? end : end + 1;
		} else {
			p = end;
		}
		if (wcscmp(argv[argc - 1], L"--") == 0) {
			after_marker = true;
		}
	}
	return argc;
}

void WriteInjectorJson(unsigned long pid, unsigned long long created_ns) {
	DWORD n = GetEnvironmentVariableW(L"VELQOR_RUN_DIR", g_json_path, (DWORD)kBufferChars);
	if (n == 0 || n >= (DWORD)kBufferChars) {
		return;
	}
	if (_snwprintf_s(g_json_final, kBufferChars, _TRUNCATE, L"%ls\\injector.json", g_json_path) < 0) {
		return;
	}
	if (_snwprintf_s(g_json_path, kBufferChars, _TRUNCATE, L"%ls.tmp", g_json_final) < 0) {
		return;
	}
	char json[128];
	int len = sprintf_s(json, sizeof(json), "{\"pid\":%lu,\"created\":%llu}\n", pid, created_ns);
	if (len <= 0) {
		return;
	}
	HANDLE file = CreateFileW(g_json_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
	                          FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		return;
	}
	DWORD wrote = 0;
	BOOL written = WriteFile(file, json, (DWORD)len, &wrote, NULL);
	CloseHandle(file);
	if (!written || wrote != (DWORD)len) {
		DeleteFileW(g_json_path);
		return;
	}
	if (!MoveFileExW(g_json_path, g_json_final, MOVEFILE_REPLACE_EXISTING)) {
		DeleteFileW(g_json_path);
	}
}

}

int main() {
	const wchar_t* raw = GetCommandLineW();
	if (raw == NULL || wcslen(raw) >= (size_t)kBufferChars) {
		Fail("bad_args");
	}
	wcscpy_s(g_source, kBufferChars, raw);

	wchar_t* argv[kMaxArgs];
	wchar_t* tail = NULL;
	int argc = SplitCommandLine(g_source, argv, kMaxArgs, &tail);
	if (argc < 0) {
		Fail("bad_args");
	}

	const wchar_t* exe = NULL;
	const wchar_t* dll = NULL;
	const wchar_t* cwd = NULL;
	for (int i = 1; i < argc; ++i) {
		wchar_t* arg = argv[i];
		if (wcscmp(arg, L"--") == 0) {
			break;
		}
		if (wcscmp(arg, L"--exe") == 0 && i + 1 < argc) {
			exe = argv[++i];
		} else if (wcscmp(arg, L"--dll") == 0 && i + 1 < argc) {
			dll = argv[++i];
		} else if (wcscmp(arg, L"--cwd") == 0 && i + 1 < argc) {
			cwd = argv[++i];
		} else {
			Fail("bad_args");
		}
	}
	if (exe == NULL || dll == NULL) {
		Fail("bad_args");
	}

	ResolvePath(exe, g_exe, "exe_path");

	unsigned char digest[32];
	if (!VelqorFileSha256(g_exe, digest)) {
		Fail("exe_missing");
	}
	if (!VelqorSha256EqHex(digest, kPinnedExeSha256)) {
		Fail("sha256");
	}

	if (!Elevated()) {
		Fail("not_admin");
	}

	ResolvePath(dll, g_dll, "dll_path");
	if (GetFileAttributesW(g_dll) == INVALID_FILE_ATTRIBUTES) {
		Fail("dll_missing");
	}
	if (WideCharToMultiByte(CP_ACP, 0, g_dll, -1, g_dll_ansi, kBufferChars, NULL, NULL) == 0) {
		Fail("dll_path");
	}

	if (cwd != NULL) {
		ResolvePath(cwd, g_cwd, "cwd_path");
	} else {
		wcscpy_s(g_cwd, kBufferChars, g_exe);
		wchar_t* slash = wcsrchr(g_cwd, L'\\');
		if (slash == NULL) {
			Fail("cwd_path");
		}
		*slash = 0;
	}
	DWORD attr = GetFileAttributesW(g_cwd);
	if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
		Fail("cwd_missing");
	}

	BuildChildEnvironment();

	int len = 0;
	if (tail != NULL) {
		len = _snwprintf_s(g_child, kBufferChars, _TRUNCATE, L"\"%ls\" %ls", g_exe, tail);
	} else {
		len = _snwprintf_s(g_child, kBufferChars, _TRUNCATE, L"\"%ls\"", g_exe);
	}
	if (len < 0) {
		Fail("bad_args");
	}

	STARTUPINFOW si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USEPOSITION | STARTF_FORCEOFFFEEDBACK;
	si.wShowWindow = SW_SHOWNOACTIVATE;
	si.dwX = kOffscreenWindowX;
	si.dwY = kOffscreenWindowY;

	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(pi));

	DWORD flags = CREATE_UNICODE_ENVIRONMENT | CREATE_DEFAULT_ERROR_MODE | CREATE_NO_WINDOW;

	SetLastError(0);
	if (!DetourCreateProcessWithDllExW(g_exe, g_child, NULL, NULL, FALSE, flags, g_env_block, g_cwd, &si,
	                                   &pi, g_dll_ansi, NULL)) {
		FailCode("inject", GetLastError());
	}

	unsigned long long created_ns = 0;
	FILETIME create_time;
	FILETIME exit_time;
	FILETIME kernel_time;
	FILETIME user_time;
	if (GetProcessTimes(pi.hProcess, &create_time, &exit_time, &kernel_time, &user_time)) {
		unsigned long long ticks = ((unsigned long long)create_time.dwHighDateTime << 32) |
		                           (unsigned long long)create_time.dwLowDateTime;
		if (ticks > kFiletimeUnixEpoch) {
			created_ns = (ticks - kFiletimeUnixEpoch) * 100ULL;
		}
	}

	unsigned long pid = (unsigned long)pi.dwProcessId;
	WriteInjectorJson(pid, created_ns);
	printf("VELQOR_INJECT pid=%lu created=%llu\n", pid, created_ns);
	fflush(stdout);

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	return 0;
}
