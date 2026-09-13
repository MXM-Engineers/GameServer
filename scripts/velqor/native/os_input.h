#pragma once

#include <stdint.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace velqor {
namespace os {

struct Viewport {
	int32_t width;
	int32_t height;
	int32_t origin_x;
	int32_t origin_y;
};

struct GuardStats {
	uint64_t attempted;
	uint64_t blocked;
	uint64_t failed_open;
};

bool Initialize();
void Shutdown();
bool Ready();

void SetVirtualCursor(int32_t x, int32_t y);
void GetVirtualCursor(int32_t* x, int32_t* y);
void SetVirtualKey(uint32_t vk, int down);
SHORT GetVirtualAsyncKey(uint32_t vk);
void ReleaseAllVirtualKeys();

bool ClientUnfocused();
void NoteClientHwnd(void* hwnd);
void* ClientHwnd();
Viewport GetViewport();
GuardStats GetGuardStats();

bool ApplyStartupWindowPolicy(void* hwnd);
bool AbortIfUnguarded(const char* reason);

}
}
