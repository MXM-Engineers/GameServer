#pragma once
#include "../core.h"
#include "item_model.h"

class WeaponModel :
	public ItemModel {
public:
	WeaponModel();
	~WeaponModel();

private:
	float _Attack = 0;
	float _PVPAttack = 0;
	//WeaponCategory
	//EquipSlot
	float _WeaponChangeDelayTime = 0.5;
	//WeaponType
	i32 _BaseWeaponIndex = -1;
	i32 _FirstGradeIndex = -1;
	i32 _NextGradeIndex = -1;
	//Specifications
	// 24 spec entries max how to solve this
};
