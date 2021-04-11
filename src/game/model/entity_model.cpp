#include "entity_model.h"

EntityModel::EntityModel()
{
}

EntityModel::EntityModel(i32 ID, EntityType entityType, float NormalMoveSpeed, float MoveSpeed, float RotateSpeed, float Scale)
{
	_ID = ID;
	_entityType = entityType;
	_NormalMoveSpeed = NormalMoveSpeed;
	_MoveSpeed = MoveSpeed;
	_RotateSpeed = RotateSpeed;
	_Scale = Scale;
}

EntityModel::~EntityModel()
{
}

i32 EntityModel::getID()
{
	return _ID;
}

void EntityModel::setID(i32 ID)
{
	_ID = ID;
}

float EntityModel::getNormalMoveSpeed()
{
	return _NormalMoveSpeed;
}

void EntityModel::setNormalMoveSpeed(float normalMoveSpeed)
{
	_NormalMoveSpeed = normalMoveSpeed;
}

float EntityModel::getMoveSpeed()
{
	return _MoveSpeed;
}

void EntityModel::setMoveSpeed(float moveSpeed)
{
	_MoveSpeed = moveSpeed;
}

float EntityModel::getRotateSpeed(void)
{
	return _RotateSpeed;
}

void EntityModel::setRotateSpeed(float rotateSpeed)
{
	_RotateSpeed = rotateSpeed;
}

float EntityModel::getScale()
{
	return _Scale;
}

void EntityModel::setScale(float scale)
{
	_Scale = scale;
}

EntityType EntityModel::getEntityType()
{
	return _entityType;
}

void EntityModel::setEntityType(EntityType entityType)
{
	_entityType = entityType;
}

void EntityModel::Print()
{
	LOG("[Entity]: ID: %d, Normal Move Speed: %f, Move Speed: %f, RotateSpeed: %f, Scale: %f, Entity Type: %d", _ID, _NormalMoveSpeed, _MoveSpeed, _RotateSpeed, _Scale, _entityType);
}
