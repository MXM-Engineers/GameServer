#include "entityModel.h"

EntityModel::EntityModel(i32 ID, EntityType entityType)
{
	this->_ID = ID;
	this->_entityType = entityType;
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
	this->_ID = ID;
}

EntityType EntityModel::getEntityType()
{
	return _entityType;
}

void EntityModel::setEntityType(EntityType entityType)
{
	this->_entityType = entityType;
}
