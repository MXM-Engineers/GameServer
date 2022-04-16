#pragma once
#include "../core.h"
#include "entity_model.h"

class CreatureModel :
	public EntityModel {
public:
	CreatureModel();
	CreatureModel(i32 ID, EntityType entityType, float NormalMoveSpeed, float MoveSpeed, float RotateSpeed,
		float Scale, CreatureType creatureType, i32 colliderRadius, i32 colliderHeight, i32 actorRadius, i32 actorHeight);
	~CreatureModel();

	CreatureType getCreatureType();
	void setCreatureType(CreatureType creatureType);
	i32 getColliderRadius();
	void setColliderRadius(i32 colliderRadius);
	i32 getColliderHeight();
	void setColliderHeight(i32 colliderHeight);
	i32 getActorRadius();
	void setActorRadius(i32 actorRadius);
	i32 getActorHeight();
	void setActorHeight(i32 actorHeight);

	void Print();

private:
	CreatureType _CreatureType = CreatureType::CREATURE_TYPE_INVALID;
	//these are under the PhysXMeshComData but they are only used for creatures to my knowledge (what is this used for?)
	i32 _ColliderRadius = 0;
	i32 _ColliderHeight = 0;
	//these are under MoveController_ComData they are used for creatures (monsters, pc's) but not npc's needed for collison and bullet physics.
	i32 _ActorRadius = 0;
	i32 _ActorHeight = 0;
};
