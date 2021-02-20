#pragma once
#ifdef CONF_WINDOWS

#include <common/base.h>
#include <common/utils.h>
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

GameUID PushNewGame(const FixedStr32& mapName);
void PushNewFrame(GameUID gameUID);
void PushEntity(GameUID gameUID, u32 UID, const FixedStr32& name, const vec3& pos, const vec3& color);
void PushEntity(GameUID gameUID, u32 UID, const WideString& name, const vec3& pos, const vec3& color);
void PopGame(GameUID gameUID);

}

#endif
