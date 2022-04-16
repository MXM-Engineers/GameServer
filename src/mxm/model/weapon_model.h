#pragma once
#include "../core.h"
#include "item_model.h"
#include "weapon_spec.h"

class WeaponModel :
	public ItemModel {
public:
	WeaponModel();
	~WeaponModel();

	float getAttack();
	void setAttack(float var);
	float getPVPAttack();
	void setPVPAttack(float var);
	ClassTypeCharacter getRequireClass();
	void setRequireClass(ClassTypeCharacter var);
	float getWeaponChangeDelayTime();
	void setWeaponChangeDelayTime(float var);
	i32 getBaseWeaponIndex();
	void setBaseWeaponIndex(i32 var);
	i32 getFirstGradeIndex();
	void setFirstGradeIndex(i32 var);
	i32 getNextGradeIndex();
	void setNextGradeIndex(i32 var);
	WeaponSpec getWeaponSpec();
	void setWeaponSpec(WeaponSpec var);

private:
	float _Attack = 0.0f;
	float _PVPAttack = 0.0f;
	//Require class needed to link to character class
	ClassTypeCharacter _RequireClass = ClassTypeCharacter::CLASS_TYPE_UNKNOWN;
	//WeaponCategory
	//EquipSlot
	float _WeaponChangeDelayTime = 0.5f;
	//WeaponType
	i32 _BaseWeaponIndex = -1;
	i32 _FirstGradeIndex = -1;
	i32 _NextGradeIndex = -1;
	//Specifications
	WeaponSpec _WeaponSpec;
};
