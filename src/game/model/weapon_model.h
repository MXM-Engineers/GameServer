#pragma once
#include "../core.h"
#include "item_model.h"
#include "weapon_spec.h"

class WeaponModel :
	public ItemModel {
public:
	WeaponModel();
	~WeaponModel();

private:
	float _Attack = 0.0f;
	float _PVPAttack = 0.0f;
	//WeaponCategory
	//EquipSlot
	float _WeaponChangeDelayTime = 0.5f;
	//WeaponType
	i32 _BaseWeaponIndex = -1;
	i32 _FirstGradeIndex = -1;
	i32 _NextGradeIndex = -1;
	//Specifications
	WeaponSpec WeaponSpec;
};
