#pragma once

#include <common/base.h>
#include <common/utils.h>
#include <mxm/core.h>
#include "physics.h"

void WindowCreate();
intptr_t ThreadWindow(void* pData);
void WindowRequestClose();
void WindowWaitForCleanup();

namespace Dbg {

enum class GameUID: u64 {
	INVALID = 0
};

struct EntityCore
{
	u32 UID;
	vec3 pos;
	RotationHumanoid rot;
};

struct PlayerMaster: EntityCore
{
	WideString name;
	vec2 moveDir;
	vec3 moveDest;
	vec3 color;
};

struct Npc: EntityCore
{
	CreatureIndex docID;
};

struct Dynamic
{
	u32 UID;
	CreatureIndex docID;
	vec3 pos;
	vec3 rot;
};

GameUID PushNewGame(const FixedStr32& mapName);
void PushNewFrame(GameUID gameUID);
void Push(GameUID gameUID, const PlayerMaster& entity);
void Push(GameUID gameUID, const Npc& entity);
void Push(GameUID gameUID, const Dynamic& entity);
void PushPhysics(GameUID gameUID, const PhysicsScene& scene);
void PopGame(GameUID gameUID);

}
