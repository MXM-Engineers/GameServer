#include "creature_model.h"

CreatureModel::CreatureModel()
{
}

CreatureModel::CreatureModel(i32 ID, EntityType entityType, float NormalMoveSpeed, float MoveSpeed, float RotateSpeed, float Scale, CreatureType creatureType)
	: EntityModel(ID, entityType, NormalMoveSpeed, MoveSpeed, RotateSpeed, Scale)
{
	_CreatureType = creatureType;
}

CreatureModel::~CreatureModel()
{
}

CreatureType CreatureModel::getCreatureType()
{
	return _CreatureType;
}

void CreatureModel::setCreatureType(CreatureType creatureType)
{
	_CreatureType = creatureType;
}
