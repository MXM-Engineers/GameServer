#ifdef CONF_LINUX
#include "window.h"

void WindowCreate() {}
intptr_t ThreadWindow(void*) { return 0; }
void WindowRequestClose() {}
void WindowWaitForCleanup() {}

namespace Dbg {

GameUID PushNewGame(const FixedStr32&) { return GameUID::INVALID; }
void PushNewFrame(GameUID) {}
void Push(GameUID, const PlayerMaster&) {}
void Push(GameUID, const Npc&) {}
void Push(GameUID, const Dynamic&) {}
void PushPhysics(GameUID, const PhysicsScene&) {}
void PopGame(GameUID) {}

}
#endif
