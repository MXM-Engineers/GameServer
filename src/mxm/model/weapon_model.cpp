#include "weapon_model.h"

WeaponModel::WeaponModel()
	: ItemModel()
{
}

WeaponModel::~WeaponModel()
{
}

float WeaponModel::getAttack(void)
{
	return _Attack;
}

void WeaponModel::setAttack(float var)
{
	_Attack = var;
}

float WeaponModel::getPVPAttack()
{
	return _PVPAttack;
}

void WeaponModel::setPVPAttack(float var)
{
	_PVPAttack = var;
}

ClassTypeCharacter WeaponModel::getRequireClass()
{
	return _RequireClass;
}

void WeaponModel::setRequireClass(ClassTypeCharacter var)
{
	_RequireClass = var;
}

float WeaponModel::getWeaponChangeDelayTime()
{
	return _WeaponChangeDelayTime;
}

void WeaponModel::setWeaponChangeDelayTime(float var)
{
	_WeaponChangeDelayTime = var;
}

i32 WeaponModel::getBaseWeaponIndex()
{
	return _BaseWeaponIndex;
}

void WeaponModel::setBaseWeaponIndex(i32 var)
{
	_BaseWeaponIndex = var;
}

i32 WeaponModel::getFirstGradeIndex()
{
	return _FirstGradeIndex;
}

void WeaponModel::setFirstGradeIndex(i32 var)
{
	_FirstGradeIndex = var;
}

i32 WeaponModel::getNextGradeIndex()
{
	return _NextGradeIndex;
}

void WeaponModel::setNextGradeIndex(i32 var)
{
	_NextGradeIndex = var;
}

WeaponSpec WeaponModel::getWeaponSpec()
{
	return _WeaponSpec;
}

void WeaponModel::setWeaponSpec(WeaponSpec var)
{
	_WeaponSpec = var;
}
