#pragma once
#include "../core.h"
#include "entity_model.h"

class ItemModel :
	public EntityModel {
public:
	ItemModel();
	~ItemModel();

private:
	ItemType _ItemType;
	ItemRank _ItemRank;
};
