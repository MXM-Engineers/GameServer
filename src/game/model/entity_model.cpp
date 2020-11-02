#include "entity_model.h"

EntityModel::EntityModel(i32 ID, EntityType entityType)
{
	_ID = ID;
	_entityType = entityType;
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

EntityType EntityModel::getEntityType()
{
	return _entityType;
}

void EntityModel::setEntityType(EntityType entityType)
{
	_entityType = entityType;
}
