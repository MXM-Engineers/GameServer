#include "os_input.h"
#include "velqor.h"

#include "third_party/Detours-4.0.1/src/detours.h"

namespace velqor {
namespace os {
namespace {

typedef BOOL (WINAPI *FnGetCursorPos)(LPPOINT);
typedef BOOL (WINAPI *FnSetCursorPos)(int, int);
typedef BOOL (WINAPI *FnClipCursor)(const RECT*);
typedef SHORT (WINAPI *FnGetAsyncKeyState)(int);
typedef SHORT (WINAPI *FnGetKeyState)(int);
typedef BOOL (WINAPI *FnShowWindow)(HWND, int);
typedef BOOL (WINAPI *FnSetWindowPos)(HWND, HWND, int, int, int, int, UINT);
typedef BOOL (WINAPI *FnSetForegroundWindow)(HWND);
typedef HWND (WINAPI *FnSetActiveWindow)(HWND);
typedef BOOL (WINAPI *FnBringWindowToTop)(HWND);
typedef BOOL (WINAPI *FnAllowSetForegroundWindow)(DWORD);
typedef HWND (WINAPI *FnSetCapture)(HWND);
typedef BOOL (WINAPI *FnReleaseCapture)(void);
typedef LONG (WINAPI *FnChangeDisplaySettingsA)(DEVMODEA*, DWORD);
typedef LONG (WINAPI *FnChangeDisplaySettingsW)(DEVMODEW*, DWORD);
typedef LONG (WINAPI *FnChangeDisplaySettingsExA)(LPCSTR, DEVMODEA*, HWND, DWORD, LPVOID);
typedef LONG (WINAPI *FnChangeDisplaySettingsExW)(LPCWSTR, DEVMODEW*, HWND, DWORD, LPVOID);
typedef LONG (WINAPI *FnSetWindowLongA)(HWND, int, LONG);
typedef LONG (WINAPI *FnSetWindowLongW)(HWND, int, LONG);
typedef LONG_PTR (WINAPI *FnSetWindowLongPtrA)(HWND, int, LONG_PTR);
typedef LONG_PTR (WINAPI *FnSetWindowLongPtrW)(HWND, int, LONG_PTR);
typedef HWND (WINAPI *FnCreateWindowExA)(DWORD, LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
typedef HWND (WINAPI *FnCreateWindowExW)(DWORD, LPCWSTR, LPCWSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
typedef HWND (WINAPI *FnCreateWindowA)(LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
typedef HWND (WINAPI *FnCreateWindowW)(LPCWSTR, LPCWSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);

FnGetCursorPos g_fn_get_cursor_pos = NULL;
FnSetCursorPos g_fn_set_cursor_pos = NULL;
FnClipCursor g_fn_clip_cursor = NULL;
FnGetAsyncKeyState g_fn_get_async_key_state = NULL;
FnGetKeyState g_fn_get_key_state = NULL;
FnShowWindow g_fn_show_window = NULL;
FnSetWindowPos g_fn_set_window_pos = NULL;
FnSetForegroundWindow g_fn_set_foreground_window = NULL;
FnSetActiveWindow g_fn_set_active_window = NULL;
FnBringWindowToTop g_fn_bring_window_to_top = NULL;
FnAllowSetForegroundWindow g_fn_allow_set_foreground_window = NULL;
FnSetCapture g_fn_set_capture = NULL;
FnReleaseCapture g_fn_release_capture = NULL;
FnChangeDisplaySettingsA g_fn_change_display_settings_a = NULL;
FnChangeDisplaySettingsW g_fn_change_display_settings_w = NULL;
FnChangeDisplaySettingsExA g_fn_change_display_settings_ex_a = NULL;
FnChangeDisplaySettingsExW g_fn_change_display_settings_ex_w = NULL;
FnSetWindowLongA g_fn_set_window_long_a = NULL;
FnSetWindowLongW g_fn_set_window_long_w = NULL;
FnSetWindowLongPtrA g_fn_set_window_long_ptr_a = NULL;
FnSetWindowLongPtrW g_fn_set_window_long_ptr_w = NULL;
FnCreateWindowExA g_fn_create_window_ex_a = NULL;
FnCreateWindowExW g_fn_create_window_ex_w = NULL;
FnCreateWindowA g_fn_create_window_a = NULL;
FnCreateWindowW g_fn_create_window_w = NULL;

const char kGuardSetCursorPos[] = "\"api\":\"SetCursorPos\",\"blocked\":true,\"mutates\":true";
const char kGuardClipCursor[] = "\"api\":\"ClipCursor\",\"blocked\":true,\"mutates\":true";
const char kGuardShowWindow[] = "\"api\":\"ShowWindow\",\"blocked\":true,\"mutates\":true";
const char kGuardSetWindowPos[] = "\"api\":\"SetWindowPos\",\"blocked\":true,\"mutates\":true";
const char kGuardSetForegroundWindow[] = "\"api\":\"SetForegroundWindow\",\"blocked\":true,\"mutates\":true";
const char kGuardSetActiveWindow[] = "\"api\":\"SetActiveWindow\",\"blocked\":true,\"mutates\":true";
const char kGuardBringWindowToTop[] = "\"api\":\"BringWindowToTop\",\"blocked\":true,\"mutates\":true";
const char kGuardAllowSetForegroundWindow[] = "\"api\":\"AllowSetForegroundWindow\",\"blocked\":true,\"mutates\":true";
const char kGuardSetCapture[] = "\"api\":\"SetCapture\",\"blocked\":true,\"mutates\":true";
const char kGuardReleaseCapture[] = "\"api\":\"ReleaseCapture\",\"blocked\":true,\"mutates\":false";
const char kGuardChangeDisplaySettingsA[] = "\"api\":\"ChangeDisplaySettingsA\",\"blocked\":true,\"mutates\":true";
const char kGuardChangeDisplaySettingsW[] = "\"api\":\"ChangeDisplaySettingsW\",\"blocked\":true,\"mutates\":true";
const char kGuardChangeDisplaySettingsExA[] = "\"api\":\"ChangeDisplaySettingsExA\",\"blocked\":true,\"mutates\":true";
const char kGuardChangeDisplaySettingsExW[] = "\"api\":\"ChangeDisplaySettingsExW\",\"blocked\":true,\"mutates\":true";
const char kGuardSetWindowLongA[] = "\"api\":\"SetWindowLongA\",\"blocked\":true,\"mutates\":true";
const char kGuardSetWindowLongW[] = "\"api\":\"SetWindowLongW\",\"blocked\":true,\"mutates\":true";
const char kGuardSetWindowLongPtrA[] = "\"api\":\"SetWindowLongPtrA\",\"blocked\":true,\"mutates\":true";
const char kGuardSetWindowLongPtrW[] = "\"api\":\"SetWindowLongPtrW\",\"blocked\":true,\"mutates\":true";
const char kGuardCreateWindowExA[] = "\"api\":\"CreateWindowExA\",\"blocked\":true,\"mutates\":true";
const char kGuardCreateWindowExW[] = "\"api\":\"CreateWindowExW\",\"blocked\":true,\"mutates\":true";
const char kGuardCreateWindowA[] = "\"api\":\"CreateWindowA\",\"blocked\":true,\"mutates\":true";
const char kGuardCreateWindowW[] = "\"api\":\"CreateWindowW\",\"blocked\":true,\"mutates\":true";

volatile LONG g_lock = 0;
volatile HWND g_client_hwnd = NULL;
volatile LONG g_ready = 0;
uint8_t g_key_down[256];
uint8_t g_key_isolated[256];
uint8_t g_key_pressed[256];
int32_t g_cursor_x = 0;
int32_t g_cursor_y = 0;
RECT g_clip = { 0, 0, 0, 0 };
uint8_t g_clip_active = 0;
GuardStats g_stats = { 0, 0, 0 };
uintptr_t g_module_base = 0;
uintptr_t g_image_end = 0;

void LockState() {
	DWORD spins = 0;
	while (InterlockedCompareExchange((LONG volatile*)&g_lock, 1, 0) != 0) {
		if (++spins >= 1000) {
			Sleep(0);
			spins = 0;
		}
	}
}

void UnlockState() {
	InterlockedExchange((LONG volatile*)&g_lock, 0);
}

void Blocked(const char* fragment) {
	LockState();
	++g_stats.attempted;
	++g_stats.blocked;
	UnlockState();
	velqor::Emit("os_guard", fragment);
}

void ClampScreenPointLocked(int32_t* sx, int32_t* sy) {
	if (g_clip_active == 0) {
		return;
	}
	if (g_clip.right <= g_clip.left || g_clip.bottom <= g_clip.top) {
		return;
	}
	if (*sx < g_clip.left) {
		*sx = g_clip.left;
	}
	if (*sx > g_clip.right - 1) {
		*sx = g_clip.right - 1;
	}
	if (*sy < g_clip.top) {
		*sy = g_clip.top;
	}
	if (*sy > g_clip.bottom - 1) {
		*sy = g_clip.bottom - 1;
	}
}

volatile LONG g_hwnd_scan_tick = 0;

int SameStringA(const char* a, const char* b) {
	if (a == NULL || b == NULL) {
		return 0;
	}
	while (*a != 0 && *a == *b) {
		++a;
		++b;
	}
	return *a == *b;
}

const char kClientClassName[] = "Gamebryo Application";

struct ClientWindowSearch {
	HWND by_class;
	HWND largest;
	int64_t largest_area;
};

BOOL CALLBACK EnumClientWindowProc(HWND hwnd, LPARAM param) {
	ClientWindowSearch* search = reinterpret_cast<ClientWindowSearch*>(param);
	DWORD pid = 0;
	GetWindowThreadProcessId(hwnd, &pid);
	if (pid != GetCurrentProcessId()) {
		return TRUE;
	}
	if (GetParent(hwnd) != NULL) {
		return TRUE;
	}
	char cls[64];
	cls[0] = 0;
	if (GetClassNameA(hwnd, cls, sizeof(cls)) > 0 && SameStringA(cls, kClientClassName)) {
		search->by_class = hwnd;
		return FALSE;
	}
	if (IsWindowVisible(hwnd) == FALSE) {
		return TRUE;
	}
	RECT rc;
	if (GetClientRect(hwnd, &rc) == FALSE) {
		return TRUE;
	}
	int64_t width = rc.right - rc.left;
	int64_t height = rc.bottom - rc.top;
	if (width < 640 || height < 480) {
		return TRUE;
	}
	if (width * height > search->largest_area) {
		search->largest_area = width * height;
		search->largest = hwnd;
	}
	return TRUE;
}

void EnsureClientHwnd() {
	if (g_client_hwnd != NULL) {
		if (IsWindow(g_client_hwnd) != FALSE) {
			return;
		}
		g_client_hwnd = NULL;
	}
	DWORD now = GetTickCount();
	LONG last = g_hwnd_scan_tick;
	if (last != 0 && (DWORD)(now - (DWORD)last) < 500) {
		return;
	}
	InterlockedExchange((LONG volatile*)&g_hwnd_scan_tick, (LONG)now);
	ClientWindowSearch search;
	search.by_class = NULL;
	search.largest = NULL;
	search.largest_area = 0;
	EnumWindows(EnumClientWindowProc, reinterpret_cast<LPARAM>(&search));
	HWND found = (search.by_class != NULL) ? search.by_class : search.largest;
	if (found != NULL) {
		NoteClientHwnd(reinterpret_cast<void*>(found));
	}
}

void ClientOrigin(int32_t* ox, int32_t* oy) {
	EnsureClientHwnd();
	*ox = 0;
	*oy = 0;
	HWND h = g_client_hwnd;
	if (h == NULL) {
		return;
	}
	POINT p;
	p.x = 0;
	p.y = 0;
	if (ClientToScreen(h, &p)) {
		*ox = p.x;
		*oy = p.y;
	}
}

int CommittedPointer(const void* p) {
	MEMORY_BASIC_INFORMATION mbi;
	if (p == NULL) {
		return 0;
	}
	if (VirtualQuery(p, &mbi, sizeof(mbi)) == 0) {
		return 0;
	}
	if (mbi.State != MEM_COMMIT || mbi.Protect == PAGE_NOACCESS) {
		return 0;
	}
	return 1;
}

int ValidHookSlot(void** slot) {
	MEMORY_BASIC_INFORMATION mbi;
	if (VirtualQuery(slot, &mbi, sizeof(mbi)) == 0) {
		return 0;
	}
	if (mbi.State != MEM_COMMIT || mbi.Protect == PAGE_NOACCESS) {
		return 0;
	}
	return CommittedPointer(*slot);
}

void AppendText(char* out, size_t cap, const char* text) {
	size_t n = 0;
	while (out[n] != 0 && n + 1 < cap) {
		++n;
	}
	while (text != NULL && *text != 0 && n + 1 < cap) {
		out[n++] = *text++;
	}
	out[n] = 0;
}

void AppendEscaped(char* out, size_t cap, const char* text) {
	size_t n = 0;
	while (out[n] != 0 && n + 1 < cap) {
		++n;
	}
	while (text != NULL && *text != 0 && n + 1 < cap) {
		char c = *text++;
		if (c == '"' || c == '\\') {
			if (n + 2 >= cap) {
				break;
			}
			out[n++] = '\\';
		}
		out[n++] = c;
	}
	out[n] = 0;
}

uintptr_t ModuleImageSize(HMODULE module) {
	const uint8_t* base = reinterpret_cast<const uint8_t*>(module);
	if (base == NULL) {
		return 0;
	}
	if (*reinterpret_cast<const uint16_t*>(base) != 0x5A4Du) {
		return 0;
	}
	int32_t lfanew = *reinterpret_cast<const int32_t*>(base + 0x3Cu);
	if (lfanew <= 0) {
		return 0;
	}
	const uint8_t* nt = base + lfanew;
	if (*reinterpret_cast<const uint32_t*>(nt) != 0x00004550u) {
		return 0;
	}
	if (*reinterpret_cast<const uint16_t*>(nt + 24) != 0x010Bu) {
		return 0;
	}
	return *reinterpret_cast<const uint32_t*>(nt + 24 + 56);
}

void MarkBuiltinKeys() {
	static const uint8_t keys[] = {
		0x01, 0x02, 0x04, 0x05, 0x06,
		0x10, 0x11, 0x12,
		0x25, 0x26, 0x27, 0x28,
		0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5,
		0xD8, 0xD9
	};
	LockState();
	for (size_t i = 0; i < sizeof(keys); ++i) {
		g_key_isolated[keys[i]] = 1;
	}
	UnlockState();
}

BOOL WINAPI Detour_GetCursorPos(LPPOINT out) {
	if (out == NULL) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	int32_t ox = 0;
	int32_t oy = 0;
	ClientOrigin(&ox, &oy);
	LockState();
	int32_t sx = g_cursor_x + ox;
	int32_t sy = g_cursor_y + oy;
	ClampScreenPointLocked(&sx, &sy);
	g_cursor_x = sx - ox;
	g_cursor_y = sy - oy;
	UnlockState();
	out->x = sx;
	out->y = sy;
	return TRUE;
}

BOOL WINAPI Detour_SetCursorPos(int x, int y) {
	Blocked(kGuardSetCursorPos);
	int32_t ox = 0;
	int32_t oy = 0;
	ClientOrigin(&ox, &oy);
	int32_t sx = x;
	int32_t sy = y;
	LockState();
	ClampScreenPointLocked(&sx, &sy);
	g_cursor_x = sx - ox;
	g_cursor_y = sy - oy;
	UnlockState();
	return TRUE;
}

BOOL WINAPI Detour_ClipCursor(const RECT* r) {
	Blocked(kGuardClipCursor);
	int32_t ox = 0;
	int32_t oy = 0;
	ClientOrigin(&ox, &oy);
	LockState();
	if (r == NULL) {
		g_clip_active = 0;
	} else if (r->right > r->left && r->bottom > r->top) {
		g_clip = *r;
		g_clip_active = 1;
	} else {
		UnlockState();
		return FALSE;
	}
	int32_t sx = g_cursor_x + ox;
	int32_t sy = g_cursor_y + oy;
	ClampScreenPointLocked(&sx, &sy);
	g_cursor_x = sx - ox;
	g_cursor_y = sy - oy;
	UnlockState();
	return TRUE;
}

SHORT WINAPI Detour_GetAsyncKeyState(int vk) {
	if (vk < 0 || vk > 0xFF) {
		if (g_fn_get_async_key_state != NULL) {
			return g_fn_get_async_key_state(vk);
		}
		return 0;
	}
	LockState();
	if (g_key_isolated[vk] == 0) {
		UnlockState();
		if (g_fn_get_async_key_state != NULL) {
			return g_fn_get_async_key_state(vk);
		}
		return 0;
	}
	int value = 0;
	if (g_key_down[vk] != 0) {
		value |= 0x8000;
	}
	if (g_key_pressed[vk] != 0) {
		value |= 1;
		g_key_pressed[vk] = 0;
	}
	UnlockState();
	return (SHORT)value;
}

SHORT WINAPI Detour_GetKeyState(int vk) {
	if (vk < 0 || vk > 0xFF) {
		if (g_fn_get_key_state != NULL) {
			return g_fn_get_key_state(vk);
		}
		return 0;
	}
	LockState();
	int isolated = g_key_isolated[vk];
	int down = g_key_down[vk];
	UnlockState();
	if (isolated == 0) {
		if (g_fn_get_key_state != NULL) {
			return g_fn_get_key_state(vk);
		}
		return 0;
	}
	return (SHORT)(down != 0 ? 0x8000 : 0);
}

BOOL WINAPI Detour_ShowWindow(HWND hwnd, int cmd) {
	if (cmd == SW_HIDE || cmd == SW_SHOWNOACTIVATE || cmd == SW_SHOWNA) {
		if (g_fn_show_window != NULL) {
			return g_fn_show_window(hwnd, cmd);
		}
		return FALSE;
	}
	Blocked(kGuardShowWindow);
	if (cmd == SW_SHOWNORMAL || cmd == SW_SHOW || cmd == SW_SHOWDEFAULT || cmd == SW_RESTORE || cmd == SW_SHOWMAXIMIZED) {
		if (g_fn_show_window != NULL) {
			return g_fn_show_window(hwnd, SW_SHOWNOACTIVATE);
		}
	}
	return TRUE;
}

BOOL WINAPI Detour_SetWindowPos(HWND hwnd, HWND after, int x, int y, int cx, int cy, UINT flags) {
	HWND safe_after = (after == HWND_TOPMOST) ? HWND_NOTOPMOST : after;
	UINT safe_flags = ((flags & SWP_NOACTIVATE) != 0) ? flags : (flags | SWP_NOACTIVATE);
	if (safe_after == after && safe_flags == flags) {
		if (g_fn_set_window_pos != NULL) {
			return g_fn_set_window_pos(hwnd, after, x, y, cx, cy, flags);
		}
		return FALSE;
	}
	Blocked(kGuardSetWindowPos);
	if (g_fn_set_window_pos != NULL) {
		return g_fn_set_window_pos(hwnd, safe_after, x, y, cx, cy, safe_flags);
	}
	return TRUE;
}

BOOL WINAPI Detour_SetForegroundWindow(HWND) {
	Blocked(kGuardSetForegroundWindow);
	return FALSE;
}

HWND WINAPI Detour_SetActiveWindow(HWND) {
	Blocked(kGuardSetActiveWindow);
	return NULL;
}

BOOL WINAPI Detour_BringWindowToTop(HWND) {
	Blocked(kGuardBringWindowToTop);
	return FALSE;
}

BOOL WINAPI Detour_AllowSetForegroundWindow(DWORD) {
	Blocked(kGuardAllowSetForegroundWindow);
	return TRUE;
}

HWND WINAPI Detour_SetCapture(HWND) {
	Blocked(kGuardSetCapture);
	return NULL;
}

BOOL WINAPI Detour_ReleaseCapture(void) {
	Blocked(kGuardReleaseCapture);
	return TRUE;
}

LONG WINAPI Detour_ChangeDisplaySettingsA(DEVMODEA*, DWORD) {
	Blocked(kGuardChangeDisplaySettingsA);
	return DISP_CHANGE_FAILED;
}

LONG WINAPI Detour_ChangeDisplaySettingsW(DEVMODEW*, DWORD) {
	Blocked(kGuardChangeDisplaySettingsW);
	return DISP_CHANGE_FAILED;
}

LONG WINAPI Detour_ChangeDisplaySettingsExA(LPCSTR, DEVMODEA*, HWND, DWORD, LPVOID) {
	Blocked(kGuardChangeDisplaySettingsExA);
	return DISP_CHANGE_FAILED;
}

LONG WINAPI Detour_ChangeDisplaySettingsExW(LPCWSTR, DEVMODEW*, HWND, DWORD, LPVOID) {
	Blocked(kGuardChangeDisplaySettingsExW);
	return DISP_CHANGE_FAILED;
}

LONG WINAPI Detour_SetWindowLongA(HWND hwnd, int index, LONG value) {
	if (index == GWL_EXSTYLE && ((DWORD)value & WS_EX_TOPMOST) != 0) {
		Blocked(kGuardSetWindowLongA);
		return GetWindowLongA(hwnd, index);
	}
	if (index == GWL_STYLE && ((DWORD)value & WS_POPUP) != 0) {
		Blocked(kGuardSetWindowLongA);
		return GetWindowLongA(hwnd, index);
	}
	if (g_fn_set_window_long_a != NULL) {
		return g_fn_set_window_long_a(hwnd, index, value);
	}
	return 0;
}

LONG WINAPI Detour_SetWindowLongW(HWND hwnd, int index, LONG value) {
	if (index == GWL_EXSTYLE && ((DWORD)value & WS_EX_TOPMOST) != 0) {
		Blocked(kGuardSetWindowLongW);
		return GetWindowLongW(hwnd, index);
	}
	if (index == GWL_STYLE && ((DWORD)value & WS_POPUP) != 0) {
		Blocked(kGuardSetWindowLongW);
		return GetWindowLongW(hwnd, index);
	}
	if (g_fn_set_window_long_w != NULL) {
		return g_fn_set_window_long_w(hwnd, index, value);
	}
	return 0;
}

LONG_PTR WINAPI Detour_SetWindowLongPtrA(HWND hwnd, int index, LONG_PTR value) {
	if (index == GWL_EXSTYLE && ((DWORD_PTR)value & WS_EX_TOPMOST) != 0) {
		Blocked(kGuardSetWindowLongPtrA);
		return GetWindowLongPtrA(hwnd, index);
	}
	if (index == GWL_STYLE && ((DWORD_PTR)value & WS_POPUP) != 0) {
		Blocked(kGuardSetWindowLongPtrA);
		return GetWindowLongPtrA(hwnd, index);
	}
	if (g_fn_set_window_long_ptr_a != NULL) {
		return g_fn_set_window_long_ptr_a(hwnd, index, value);
	}
	return 0;
}

LONG_PTR WINAPI Detour_SetWindowLongPtrW(HWND hwnd, int index, LONG_PTR value) {
	if (index == GWL_EXSTYLE && ((DWORD_PTR)value & WS_EX_TOPMOST) != 0) {
		Blocked(kGuardSetWindowLongPtrW);
		return GetWindowLongPtrW(hwnd, index);
	}
	if (index == GWL_STYLE && ((DWORD_PTR)value & WS_POPUP) != 0) {
		Blocked(kGuardSetWindowLongPtrW);
		return GetWindowLongPtrW(hwnd, index);
	}
	if (g_fn_set_window_long_ptr_w != NULL) {
		return g_fn_set_window_long_ptr_w(hwnd, index, value);
	}
	return 0;
}

DWORD SafeCreateExStyle(DWORD ex_style) {
	return (ex_style | WS_EX_NOACTIVATE) & ~((DWORD)WS_EX_TOPMOST);
}

void AfterClientWindowCreate(HWND hwnd, DWORD style) {
	if (hwnd == NULL || IsWindow(hwnd) == FALSE) {
		return;
	}
	char cls[64];
	cls[0] = 0;
	GetClassNameA(hwnd, cls, sizeof(cls));
	if (SameStringA(cls, kClientClassName) == 0) {
		return;
	}
	NoteClientHwnd(reinterpret_cast<void*>(hwnd));
	if ((style & WS_VISIBLE) != 0 && g_fn_show_window != NULL) {
		g_fn_show_window(hwnd, SW_SHOWNOACTIVATE);
	}
}

HWND WINAPI Detour_CreateWindowExA(DWORD ex_style, LPCSTR cls, LPCSTR text, DWORD style, int x, int y, int w, int h, HWND parent, HMENU menu, HINSTANCE inst, LPVOID param) {
	DWORD safe_ex = SafeCreateExStyle(ex_style);
	if (safe_ex != ex_style) {
		Blocked(kGuardCreateWindowExA);
	}
	if (g_fn_create_window_ex_a == NULL) {
		SetLastError(ERROR_PROC_NOT_FOUND);
		return NULL;
	}
	HWND hwnd = g_fn_create_window_ex_a(safe_ex, cls, text, style, x, y, w, h, parent, menu, inst, param);
	AfterClientWindowCreate(hwnd, style);
	return hwnd;
}

HWND WINAPI Detour_CreateWindowExW(DWORD ex_style, LPCWSTR cls, LPCWSTR text, DWORD style, int x, int y, int w, int h, HWND parent, HMENU menu, HINSTANCE inst, LPVOID param) {
	DWORD safe_ex = SafeCreateExStyle(ex_style);
	if (safe_ex != ex_style) {
		Blocked(kGuardCreateWindowExW);
	}
	if (g_fn_create_window_ex_w == NULL) {
		SetLastError(ERROR_PROC_NOT_FOUND);
		return NULL;
	}
	HWND hwnd = g_fn_create_window_ex_w(safe_ex, cls, text, style, x, y, w, h, parent, menu, inst, param);
	AfterClientWindowCreate(hwnd, style);
	return hwnd;
}

HWND WINAPI Detour_CreateWindowA(LPCSTR cls, LPCSTR text, DWORD style, int x, int y, int w, int h, HWND parent, HMENU menu, HINSTANCE inst, LPVOID param) {
	Blocked(kGuardCreateWindowA);
	if (g_fn_create_window_a == NULL) {
		SetLastError(ERROR_PROC_NOT_FOUND);
		return NULL;
	}
	HWND hwnd = g_fn_create_window_a(cls, text, style, x, y, w, h, parent, menu, inst, param);
	AfterClientWindowCreate(hwnd, style);
	return hwnd;
}

HWND WINAPI Detour_CreateWindowW(LPCWSTR cls, LPCWSTR text, DWORD style, int x, int y, int w, int h, HWND parent, HMENU menu, HINSTANCE inst, LPVOID param) {
	Blocked(kGuardCreateWindowW);
	if (g_fn_create_window_w == NULL) {
		SetLastError(ERROR_PROC_NOT_FOUND);
		return NULL;
	}
	HWND hwnd = g_fn_create_window_w(cls, text, style, x, y, w, h, parent, menu, inst, param);
	AfterClientWindowCreate(hwnd, style);
	return hwnd;
}

struct HookEntry {
	const char* api_name;
	void** real;
	void* detour;
	int required;
	int resolved;
	int attached;
};

enum { kMaxHooks = 32 };

HookEntry g_hooks[kMaxHooks];
int g_hook_count = 0;

struct PinnedSlot {
	uint32_t static_va;
	const char* api_name;
	void* detour;
	void** slot;
};

PinnedSlot g_pinned[3] = {
	{ 0x0277AF24u, "GetCursorPos", NULL, NULL },
	{ 0x0277AFA4u, "SetCursorPos", NULL, NULL },
	{ 0x0277B00Cu, "ClipCursor", NULL, NULL }
};

void AddHook(const char* api_name, void** real, void* detour, int required) {
	if (g_hook_count >= kMaxHooks) {
		return;
	}
	HookEntry& e = g_hooks[g_hook_count];
	++g_hook_count;
	e.api_name = api_name;
	e.real = real;
	e.detour = detour;
	e.required = required;
	e.resolved = 0;
	e.attached = 0;
}

void BuildHookTable() {
	g_hook_count = 0;
	AddHook("GetCursorPos", reinterpret_cast<void**>(&g_fn_get_cursor_pos), reinterpret_cast<void*>(&Detour_GetCursorPos), 1);
	AddHook("SetCursorPos", reinterpret_cast<void**>(&g_fn_set_cursor_pos), reinterpret_cast<void*>(&Detour_SetCursorPos), 1);
	AddHook("ClipCursor", reinterpret_cast<void**>(&g_fn_clip_cursor), reinterpret_cast<void*>(&Detour_ClipCursor), 1);
	AddHook("GetAsyncKeyState", reinterpret_cast<void**>(&g_fn_get_async_key_state), reinterpret_cast<void*>(&Detour_GetAsyncKeyState), 1);
	AddHook("GetKeyState", reinterpret_cast<void**>(&g_fn_get_key_state), reinterpret_cast<void*>(&Detour_GetKeyState), 1);
	AddHook("ShowWindow", reinterpret_cast<void**>(&g_fn_show_window), reinterpret_cast<void*>(&Detour_ShowWindow), 1);
	AddHook("SetWindowPos", reinterpret_cast<void**>(&g_fn_set_window_pos), reinterpret_cast<void*>(&Detour_SetWindowPos), 1);
	AddHook("SetForegroundWindow", reinterpret_cast<void**>(&g_fn_set_foreground_window), reinterpret_cast<void*>(&Detour_SetForegroundWindow), 1);
	AddHook("SetActiveWindow", reinterpret_cast<void**>(&g_fn_set_active_window), reinterpret_cast<void*>(&Detour_SetActiveWindow), 1);
	AddHook("BringWindowToTop", reinterpret_cast<void**>(&g_fn_bring_window_to_top), reinterpret_cast<void*>(&Detour_BringWindowToTop), 1);
	AddHook("AllowSetForegroundWindow", reinterpret_cast<void**>(&g_fn_allow_set_foreground_window), reinterpret_cast<void*>(&Detour_AllowSetForegroundWindow), 1);
	AddHook("SetCapture", reinterpret_cast<void**>(&g_fn_set_capture), reinterpret_cast<void*>(&Detour_SetCapture), 1);
	AddHook("ReleaseCapture", reinterpret_cast<void**>(&g_fn_release_capture), reinterpret_cast<void*>(&Detour_ReleaseCapture), 1);
	AddHook("ChangeDisplaySettingsA", reinterpret_cast<void**>(&g_fn_change_display_settings_a), reinterpret_cast<void*>(&Detour_ChangeDisplaySettingsA), 1);
	AddHook("ChangeDisplaySettingsW", reinterpret_cast<void**>(&g_fn_change_display_settings_w), reinterpret_cast<void*>(&Detour_ChangeDisplaySettingsW), 0);
	AddHook("ChangeDisplaySettingsExA", reinterpret_cast<void**>(&g_fn_change_display_settings_ex_a), reinterpret_cast<void*>(&Detour_ChangeDisplaySettingsExA), 1);
	AddHook("ChangeDisplaySettingsExW", reinterpret_cast<void**>(&g_fn_change_display_settings_ex_w), reinterpret_cast<void*>(&Detour_ChangeDisplaySettingsExW), 0);
	AddHook("SetWindowLongA", reinterpret_cast<void**>(&g_fn_set_window_long_a), reinterpret_cast<void*>(&Detour_SetWindowLongA), 1);
	AddHook("SetWindowLongW", reinterpret_cast<void**>(&g_fn_set_window_long_w), reinterpret_cast<void*>(&Detour_SetWindowLongW), 0);
	AddHook("SetWindowLongPtrA", reinterpret_cast<void**>(&g_fn_set_window_long_ptr_a), reinterpret_cast<void*>(&Detour_SetWindowLongPtrA), 0);
	AddHook("SetWindowLongPtrW", reinterpret_cast<void**>(&g_fn_set_window_long_ptr_w), reinterpret_cast<void*>(&Detour_SetWindowLongPtrW), 0);
	AddHook("CreateWindowExA", reinterpret_cast<void**>(&g_fn_create_window_ex_a), reinterpret_cast<void*>(&Detour_CreateWindowExA), 1);
	AddHook("CreateWindowExW", reinterpret_cast<void**>(&g_fn_create_window_ex_w), reinterpret_cast<void*>(&Detour_CreateWindowExW), 1);
	AddHook("CreateWindowA", reinterpret_cast<void**>(&g_fn_create_window_a), reinterpret_cast<void*>(&Detour_CreateWindowA), 0);
	AddHook("CreateWindowW", reinterpret_cast<void**>(&g_fn_create_window_w), reinterpret_cast<void*>(&Detour_CreateWindowW), 0);
	g_pinned[0].detour = reinterpret_cast<void*>(&Detour_GetCursorPos);
	g_pinned[1].detour = reinterpret_cast<void*>(&Detour_SetCursorPos);
	g_pinned[2].detour = reinterpret_cast<void*>(&Detour_ClipCursor);
}

}

bool Initialize() {
	if (g_ready != 0) {
		return true;
	}
	BuildHookTable();
	HMODULE module = GetModuleHandleW(NULL);
	if (module == NULL) {
		return AbortIfUnguarded("module_base");
	}
	g_module_base = reinterpret_cast<uintptr_t>(module);
	g_image_end = g_module_base + ModuleImageSize(module);
	MarkBuiltinKeys();
	for (int i = 0; i < g_hook_count; ++i) {
		HookEntry& e = g_hooks[i];
		void* resolved = velqor::HookGetProc("user32.dll", e.api_name);
		*e.real = resolved;
		e.resolved = (resolved != NULL) ? 1 : 0;
		if (resolved == NULL && e.required != 0) {
			return AbortIfUnguarded(e.api_name);
		}
	}
	if (!velqor::HookTransactionBegin()) {
		return AbortIfUnguarded("transaction_begin");
	}
	for (int i = 0; i < g_hook_count; ++i) {
		HookEntry& e = g_hooks[i];
		if (e.resolved == 0) {
			continue;
		}
		if (DetourAttach(reinterpret_cast<PVOID*>(e.real), e.detour) != NO_ERROR) {
			DetourTransactionAbort();
			return AbortIfUnguarded(e.api_name);
		}
		e.attached = 1;
	}
	if (!velqor::HookTransactionCommit()) {
		return AbortIfUnguarded("transaction_commit");
	}
	for (int i = 0; i < 3; ++i) {
		uintptr_t slot_va = g_module_base + (static_cast<uintptr_t>(g_pinned[i].static_va) - 0x00400000u);
		if (slot_va < g_module_base || slot_va + sizeof(void*) > g_image_end) {
			return AbortIfUnguarded(g_pinned[i].api_name);
		}
		void** slot = reinterpret_cast<void**>(slot_va);
		if (!ValidHookSlot(slot)) {
			return AbortIfUnguarded(g_pinned[i].api_name);
		}
		if (!velqor::HookAttach(slot, g_pinned[i].detour)) {
			return AbortIfUnguarded(g_pinned[i].api_name);
		}
		g_pinned[i].slot = slot;
	}
	InterlockedExchange((LONG volatile*)&g_ready, 1);
	return true;
}

void Shutdown() {
	if (g_ready == 0) {
		return;
	}
	InterlockedExchange((LONG volatile*)&g_ready, 0);
	for (int i = 0; i < 3; ++i) {
		if (g_pinned[i].slot != NULL) {
			velqor::HookDetach(g_pinned[i].slot, g_pinned[i].detour);
			g_pinned[i].slot = NULL;
		}
	}
	if (velqor::HookTransactionBegin()) {
		for (int i = 0; i < g_hook_count; ++i) {
			HookEntry& e = g_hooks[i];
			if (e.attached == 0) {
				continue;
			}
			DetourDetach(reinterpret_cast<PVOID*>(e.real), e.detour);
			e.attached = 0;
		}
		velqor::HookTransactionCommit();
	}
}

bool Ready() {
	return g_ready != 0;
}

void SetVirtualCursor(int32_t x, int32_t y) {
	int32_t ox = 0;
	int32_t oy = 0;
	ClientOrigin(&ox, &oy);
	int32_t sx = x + ox;
	int32_t sy = y + oy;
	LockState();
	ClampScreenPointLocked(&sx, &sy);
	g_cursor_x = sx - ox;
	g_cursor_y = sy - oy;
	UnlockState();
}

void GetVirtualCursor(int32_t* x, int32_t* y) {
	LockState();
	int32_t cx = g_cursor_x;
	int32_t cy = g_cursor_y;
	UnlockState();
	if (x != NULL) {
		*x = cx;
	}
	if (y != NULL) {
		*y = cy;
	}
}

void SetVirtualKey(uint32_t vk, int down) {
	if (vk > 0xFF) {
		return;
	}
	LockState();
	g_key_isolated[vk] = 1;
	if (down != 0) {
		if (g_key_down[vk] == 0) {
			g_key_pressed[vk] = 1;
		}
		g_key_down[vk] = 1;
	} else {
		g_key_down[vk] = 0;
	}
	UnlockState();
}

SHORT GetVirtualAsyncKey(uint32_t vk) {
	if (vk > 0xFF) {
		return 0;
	}
	LockState();
	int value = 0;
	if (g_key_isolated[vk] != 0) {
		if (g_key_down[vk] != 0) {
			value |= 0x8000;
		}
		if (g_key_pressed[vk] != 0) {
			value |= 1;
		}
	}
	UnlockState();
	return (SHORT)value;
}

void ReleaseAllVirtualKeys() {
	LockState();
	for (int i = 0; i < 256; ++i) {
		g_key_down[i] = 0;
		g_key_pressed[i] = 0;
	}
	UnlockState();
}

bool ClientUnfocused() {
	HWND fg = GetForegroundWindow();
	if (fg == NULL) {
		return true;
	}
	HWND client = g_client_hwnd;
	if (client != NULL && fg == client) {
		return false;
	}
	DWORD pid = 0;
	GetWindowThreadProcessId(fg, &pid);
	return pid != GetCurrentProcessId();
}

volatile HWND g_policy_hwnd = NULL;

void NoteClientHwnd(void* hwnd) {
	HWND h = reinterpret_cast<HWND>(hwnd);
	if (h == NULL) {
		g_client_hwnd = NULL;
		return;
	}
	g_client_hwnd = h;
	if (g_policy_hwnd == h) {
		return;
	}
	g_policy_hwnd = h;
	ApplyStartupWindowPolicy(hwnd);
}

void* ClientHwnd() {
	HWND h = g_client_hwnd;
	return reinterpret_cast<void*>(h);
}

Viewport GetViewport() {
	Viewport v;
	v.width = 0;
	v.height = 0;
	v.origin_x = 0;
	v.origin_y = 0;
	EnsureClientHwnd();
	HWND h = g_client_hwnd;
	if (h == NULL || IsWindow(h) == FALSE) {
		return v;
	}
	RECT rc;
	if (GetClientRect(h, &rc)) {
		v.width = rc.right - rc.left;
		v.height = rc.bottom - rc.top;
	}
	POINT p;
	p.x = 0;
	p.y = 0;
	if (ClientToScreen(h, &p)) {
		v.origin_x = p.x;
		v.origin_y = p.y;
	}
	return v;
}

GuardStats GetGuardStats() {
	GuardStats out;
	LockState();
	out = g_stats;
	UnlockState();
	return out;
}

bool ApplyStartupWindowPolicy(void* hwnd) {
	HWND h = reinterpret_cast<HWND>(hwnd);
	if (h == NULL || IsWindow(h) == FALSE) {
		return false;
	}
	if (g_fn_show_window == NULL || g_fn_set_window_pos == NULL) {
		return false;
	}
	NoteClientHwnd(hwnd);
	g_fn_show_window(h, SW_SHOWNOACTIVATE);
	if (g_fn_set_window_long_a != NULL) {
		LONG ex = GetWindowLongA(h, GWL_EXSTYLE);
		if ((ex & WS_EX_TOPMOST) != 0) {
			g_fn_set_window_long_a(h, GWL_EXSTYLE, ex & ~WS_EX_TOPMOST);
		}
	}
	if (g_fn_set_window_pos(h, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER) == FALSE) {
		return false;
	}
	if (IsWindowVisible(h) == FALSE) {
		return false;
	}
	return IsIconic(h) == FALSE;
}

bool AbortIfUnguarded(const char* reason) {
	LockState();
	++g_stats.failed_open;
	UnlockState();
	char field[256];
	field[0] = 0;
	AppendText(field, sizeof(field), "\"message\":\"os_input:");
	AppendEscaped(field, sizeof(field), (reason != NULL) ? reason : "unguarded");
	AppendText(field, sizeof(field), "\"");
	velqor::Emit("error", field);
	TerminateProcess(GetCurrentProcess(), 2);
	return false;
}

}
}
