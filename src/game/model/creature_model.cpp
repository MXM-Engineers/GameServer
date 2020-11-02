#include "creature_model.h"

CreatureModel::CreatureModel(i32 ID, EntityType entityType, float NormalMoveSpeed, float MoveSpeed, float RotateSpeed, float Scale, CreatureType creatureType)
	: EntityModel(ID, entityType)
{
	_NormalMoveSpeed = NormalMoveSpeed;
	_MoveSpeed = MoveSpeed;
	_CreatureType = creatureType;
	_RotateSpeed = RotateSpeed;
	_Scale = Scale;
}

CreatureModel::~CreatureModel()
{
}

float CreatureModel::getNormalMoveSpeed()
{
	return _NormalMoveSpeed;
}

void CreatureModel::setNormalMoveSpeed(float normalMoveSpeed)
{
	_NormalMoveSpeed = normalMoveSpeed;
}

float CreatureModel::getMoveSpeed()
{
	return _MoveSpeed;
}

void CreatureModel::setMoveSpeed(float moveSpeed)
{
	_MoveSpeed = moveSpeed;
}

float CreatureModel::getRotateSpeed(void)
{
	return _RotateSpeed;
}

void CreatureModel::setRotateSpeed(float rotateSpeed)
{
	_RotateSpeed = rotateSpeed;
}

float CreatureModel::getScale()
{
	return _Scale;
}

void CreatureModel::setScale(float scale)
{
	_Scale = scale;
}

CreatureType CreatureModel::getCreatureType()
{
	return _CreatureType;
}

void CreatureModel::setCreatureType(CreatureType creatureType)
{
	_CreatureType = creatureType;
}
