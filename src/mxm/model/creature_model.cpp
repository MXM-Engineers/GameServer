#include "creature_model.h"

CreatureModel::CreatureModel()
{
}

CreatureModel::CreatureModel(i32 ID, EntityType entityType, float NormalMoveSpeed, float MoveSpeed, float RotateSpeed, float Scale, CreatureType creatureType, i32 colliderRadius, i32 colliderHeight, i32 actorRadius, i32 actorHeight)
	: EntityModel(ID, entityType, NormalMoveSpeed, MoveSpeed, RotateSpeed, Scale)
{
	_CreatureType = creatureType;
	_ColliderRadius = colliderRadius;
	_ColliderHeight = colliderHeight;
	_ActorRadius = actorRadius;
	_ActorHeight = actorHeight;
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

i32 CreatureModel::getColliderRadius()
{
	return _ColliderRadius;
}

void CreatureModel::setColliderRadius(i32 colliderRadius)
{
	_ColliderRadius = colliderRadius;
}

i32 CreatureModel::getColliderHeight()
{
	return _ColliderHeight;
}

void CreatureModel::setColliderHeight(i32 colliderHeight)
{
	_ColliderHeight = colliderHeight;
}

i32 CreatureModel::getActorRadius()
{
	return _ActorRadius;
}

void CreatureModel::setActorRadius(i32 actorRadius)
{
	_ActorRadius = actorRadius;
}

i32 CreatureModel::getActorHeight()
{
	return _ActorHeight;
}

void CreatureModel::setActorHeight(i32 actorHeight)
{
	_ActorHeight = actorHeight;
}

void CreatureModel::Print()
{
	LOG("[CreatureModel]: Creature Type %d, Collider Radius: %d, Collider Height: %d", _CreatureType, _ColliderRadius, _ColliderHeight);
	EntityModel::Print();
}
