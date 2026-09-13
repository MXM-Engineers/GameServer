#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool BridgeOsSafetyInitialize();
bool BridgeInitialize();
void BridgeBeforeFrame();
void BridgeAfterFrame();
bool BridgeSetCursor(int32_t x, int32_t y);
bool BridgePlay();
const char* BridgeLastError();
bool BridgeReady();
void BridgeNotifyKey(uint32_t vk, int down);
void BridgeNoteSetCursorPos(int32_t x, int32_t y);
void BridgeSetPendingSeq(uint32_t seq);
void BridgeOnStop();
bool BridgeUnfocused();
const char* BridgePhaseName();

#ifdef __cplusplus
}
#endif
