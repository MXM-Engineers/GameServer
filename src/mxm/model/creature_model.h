#pragma once
#include "../core.h"
#include "entity_model.h"

class CreatureModel :
	public EntityModel {
public:
	CreatureModel();
	CreatureModel(i32 ID, EntityType entityType, float NormalMoveSpeed, float MoveSpeed, float RotateSpeed,
		float Scale, CreatureType creatureType, i32 colliderRadius, i32 colliderHeight);
	~CreatureModel();

	CreatureType getCreatureType();
	void setCreatureType(CreatureType creatureType);
	i32 getColliderRadius();
	void setColliderRadius(i32 colliderRadius);
	i32 getColliderHeight();
	void setColliderHeight(i32 colliderHeight);

	void Print();

private:
	CreatureType _CreatureType = CreatureType::CREATURE_TYPE_INVALID;
	//these are under the PhysXMeshComData but they are only used for creatures to my knowledge
	i32 _ColliderRadius = 0;
	i32 _ColliderHeight = 0;
};
