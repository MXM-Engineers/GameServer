#pragma once
#include "../core.h"
#include "entity_model.h"

class ItemModel :
	public EntityModel {
public:
	ItemModel();
	~ItemModel();

private:
	ItemType _ItemType = ItemType::ITEM_TYPE_UNKNOWN;
	ItemRank _ItemRank = ItemRank::ITEM_RANK_INVALID;
};
