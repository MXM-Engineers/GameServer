#pragma once
#include "../core.h"

class EntityModel {
public:
	EntityModel(i32 ID, EntityType entityType);
	~EntityModel();

	i32 getID();
	void setID(i32 ID);
	EntityType getEntityType();
	void setEntityType(EntityType entityType);
private:
	i32 _ID;
	EntityType _entityType = ENTITY_INVALID;
};
