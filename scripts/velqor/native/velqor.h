#pragma once

#include <stdint.h>

namespace velqor {

void Emit(const char* event, const char* json_fields);
uint64_t QpcNs();
bool AutomationActive();
uint64_t CurrentFrame();

bool HookTransactionBegin();
bool HookUpdateThread(void* thread);
bool HookAttach(void** target, void* detour);
bool HookDetach(void** target, void* detour);
bool HookTransactionCommit();
void* HookGetProc(const char* module, const char* name);

}

#ifdef __cplusplus
extern "C" {
#endif
int VelqorDllAttach();
void VelqorDllDetach(int process_exit);
#ifdef __cplusplus
}
#endif
