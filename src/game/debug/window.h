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

struct Entity
{
	u32 UID;
	WideString name;
	vec3 pos;
	vec3 eye;
	vec3 dir; // body dir

	f32 upperRotate = 0;
	f32 bodyRotate = 0;

	vec3 color;
};

GameUID PushNewGame(const FixedStr32& mapName);
void PushNewFrame(GameUID gameUID);
void PushEntity(GameUID gameUID, const Entity& entity);
void PopGame(GameUID gameUID);

}

#endif
