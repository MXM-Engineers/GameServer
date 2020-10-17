#include "creatureModel.h"

CreatureModel::CreatureModel(i32 ID, EntityType entityType, float NormalMoveSpeed, float MoveSpeed, float RotateSpeed, float Scale, CreatureType creatureType)
	: EntityModel(ID, entityType)
{
	this->_NormalMoveSpeed = NormalMoveSpeed;
	this->_MoveSpeed = MoveSpeed;
	this->_CreatureType = creatureType;
	this->_RotateSpeed = RotateSpeed;
	this->_Scale = Scale;
}

CreatureModel::~CreatureModel()
{
}

float CreatureModel::getNormalMoveSpeed()
{
	return this->_NormalMoveSpeed;
}

void CreatureModel::setNormalMoveSpeed(float normalMoveSpeed)
{
	this->_NormalMoveSpeed = normalMoveSpeed;
}

float CreatureModel::getMoveSpeed()
{
	return this->_MoveSpeed;
}

void CreatureModel::setMoveSpeed(float moveSpeed)
{
	this->_MoveSpeed = moveSpeed;
}

float CreatureModel::getRotateSpeed(void)
{
	return this->_RotateSpeed;
}

void CreatureModel::setRotateSpeed(float rotateSpeed)
{
	this->_RotateSpeed = rotateSpeed;
}

float CreatureModel::getScale()
{
	return this->_Scale;
}

void CreatureModel::setScale(float scale)
{
	this->_Scale = scale;
}

CreatureType CreatureModel::getCreatureType()
{
	return this->_CreatureType;
}

void CreatureModel::setCreatureType(CreatureType creatureType)
{
	this->_CreatureType = creatureType;
}
