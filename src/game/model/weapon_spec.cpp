#include "weapon_spec.h"

WeaponSpec::WeaponSpec()
{
};

WeaponSpec::~WeaponSpec()
{
}
float WeaponSpec::getFiringMethodChargingTimeLevel1()
{
	return _FiringMethodChargingTimeLevel1;
}
void WeaponSpec::setFiringMethodChargingTimeLevel1(float var)
{
	_FiringMethodChargingTimeLevel1 = var;
}
float WeaponSpec::getFiringMethodChargingTimeLevel2()
{
	return _FiringMethodChargingTimeLevel2;
};
void WeaponSpec::setFiringMethodChargingTimeLevel2(float var)
{
	_FiringMethodChargingTimeLevel2 = var;
};
float WeaponSpec::getFiringMethodChargingTimeLevel3()
{
	return _FiringMethodChargingTimeLevel3;
}
void WeaponSpec::setFiringMethodChargingTimeLevel3(float var)
{
	_FiringMethodChargingTimeLevel3 = var;
};

