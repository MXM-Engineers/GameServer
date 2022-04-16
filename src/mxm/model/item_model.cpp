#include "item_model.h"

ItemModel::ItemModel()
	: EntityModel()
{
}

ItemModel::ItemModel(i32 ID, EntityType entityType, float NormalMoveSpeed, float MoveSpeed, float RotateSpeed, float Scale)
	: EntityModel(ID, entityType, NormalMoveSpeed, MoveSpeed, RotateSpeed, Scale)

{
}


ItemModel::~ItemModel()
{
}
