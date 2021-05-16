#pragma once
#include "../core.h"

class EntityModel {
public:
	EntityModel();
	EntityModel(i32 ID, EntityType entityType, float NormalMoveSpeed, float MoveSpeed, float RotateSpeed, float Scale);
	~EntityModel();

	i32 getID();
	void setID(i32 ID);
	float getNormalMoveSpeed();
	void setNormalMoveSpeed(float normalMoveSpeed);
	float getMoveSpeed();
	void setMoveSpeed(float moveSpeed);
	float getRotateSpeed(void);
	void setRotateSpeed(float rotateSpeed);
	float getScale();
	void setScale(float scale);
	EntityType getEntityType();
	void setEntityType(EntityType entityType);
	void Print();

private:
	i32 _ID = 0;
	float _NormalMoveSpeed = 0.0f;		// Exclusive for Creature Monster
	float _MoveSpeed = 0.0f;
	float _RotateSpeed = 0.0f;
	float _Scale = 0.0f;
	EntityType _entityType = EntityType::ENTITY_INVALID;
};
