#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "detours.h"
#include "velqor.h"


BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
	if (reason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(instance);
		VelqorDllAttach();
	} else if (reason == DLL_PROCESS_DETACH) {
		VelqorDllDetach((reserved != 0) ? 1 : 0);
	}
	return TRUE;
}
