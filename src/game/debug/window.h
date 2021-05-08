#pragma once

#include <common/base.h>
#include <common/utils.h>
#include <glm/vec3.hpp>
#include <game/core.h>

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
	RotationHumanoid rot;
	vec2 moveDir;

	vec3 color;
};

GameUID PushNewGame(const FixedStr32& mapName);
void PushNewFrame(GameUID gameUID);
void PushEntity(GameUID gameUID, const Entity& entity);
void PopGame(GameUID gameUID);

}
