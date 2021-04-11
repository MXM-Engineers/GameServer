#pragma once
#include "../core.h"
#include "entity_model.h"

class CreatureModel :
	public EntityModel {
public:
	CreatureModel();
	CreatureModel(i32 ID, EntityType entityType, float NormalMoveSpeed, float MoveSpeed, float RotateSpeed,
		float Scale, CreatureType creatureType);
	~CreatureModel();

	CreatureType getCreatureType();
	void setCreatureType(CreatureType creatureType);

	void Print();

private:
	CreatureType _CreatureType;
};
