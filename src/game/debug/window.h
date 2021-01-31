#pragma once
#ifdef CONF_WINDOWS

#include <common/base.h>
#include <glm/vec3.hpp>

typedef glm::vec3 vec3;

void WindowCreate();
intptr_t ThreadWindow(void* pData);
void WindowRequestClose();
void WindowWaitForCleanup();

namespace Dbg {

enum class GameUID: u64 {
	INVALID = 0
};

GameUID PushNewGame(const FixedStr<32>& mapName);
void PushNewFrame(GameUID gameUID);
void PushEntity(GameUID gameUID, const vec3& pos, const vec3& color);
void PopGame(GameUID gameUID);

}

#endif
