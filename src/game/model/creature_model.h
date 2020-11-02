#pragma once
#include "../core.h"
#include "entity_model.h"

class CreatureModel :
	public EntityModel {
public:
	CreatureModel(i32 ID, EntityType entityType, float NormalMoveSpeed, float MoveSpeed, float RotateSpeed,
		float Scale, CreatureType creatureType);
	~CreatureModel();

	float getNormalMoveSpeed();
	void setNormalMoveSpeed(float normalMoveSpeed);
	float getMoveSpeed();
	void setMoveSpeed(float moveSpeed);
	float getRotateSpeed(void);
	void setRotateSpeed(float rotateSpeed);
	float getScale();
	void setScale(float scale);
	CreatureType getCreatureType();
	void setCreatureType(CreatureType creatureType);

private:
	float _NormalMoveSpeed;
	float _MoveSpeed;
	float _RotateSpeed;
	float _Scale;
	CreatureType _CreatureType;
};
