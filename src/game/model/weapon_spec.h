#pragma once

class WeaponSpec
{
public:
	WeaponSpec();
	~WeaponSpec();
private:
	float _FiringMethodChargingTimeLevel1 = 0.0f;
	float _FiringMethodChargingTimeLevel2 = 0.0f;
	float _FiringMethodChargingTimeLevel3 = 0.0f;
	float _FiringMethodComboFireDelay = 0.0f;
	float _FiringMethodComboValidTime = 0.0f;
	float _FiringMethodConsumption = 0.0f;
	float _FiringMethodConsumptionByCharging = 0.0f;
	float _FiringMethodErrorAngle = 0.0f;
	float _FiringMethodFireDelay = 0.0f;
	float _FiringMethodMaxDistance = 0.0f;
	float _FiringMethodMinDistance = 0.0f;
	float _FiringMethodChargingLevel1CreateRadiateMaxNum = 0.0f;
	float _FiringMethodChargingLevel2CreateRadiateMaxNum = 0.0f;
	float _FiringMethodCreateRadiateMaxAngBle = 0.0f;
	float _FiringMethodCreateRadiateMaxNum = 0.0f;
	float _FiringMethodFiringStatMovementSpeed = 0.0f;
	float _FiringMethodFiringGaugeLevel = 0.0f;
	float _FiringMethodChargingLevel1CreateParallelMaxNum = 0.0f;
	float _FiringMethodChargingLevel2CreateParallelMaxNum = 0.0f;
	float _FiringMethodChargingLevel1AnimationMoveHorizon = 0.0f;
	float _FiringMethodChargingLevel2AnimationMoveHorizon = 0.0f;
	float _FiringMethodGaugeAutoRegen = 0.0f;
	float _RemoteAngle = 0.0f;
	float _RemoteAttackMultiplier = 0.0f;
	float _RemoteDamageOnBound = 0.0f;
	float _RemoteFOV = 0.0f;
	float _RemoteLengthX = 0.0f;
	float _RemoteLengthY = 0.0f;
	float _RemoteMaxDistance = 0.0f;
	float _RemoteMaxScale = 0.0f;
	float _RemoteMinScale = 0.0f;
	float _RemoteMaxSpeed = 0.0f;
	float _RemotePenetrationCount = 0.0f;
	float _RemoteSight = 0.0f;
	float _RemoteStatusRate = 0.0f;
	float _RemoteCriticalRate = 0.0f;
	float _RemoteSubBoundLenght = 0.0f;
	float _ActionBaseParam1 = 0.0f;
	float _StatusDistance = 0.0f;
	float _StatusRegenAndHealth = 0.0f;
	float _StatusStatMovementSpeed = 0.0f;
	float _StatusStatDefence = 0.0f;
	float _StatusDurationTime = 0.0f;
	float _StatusMaxOverlapCount = 0.0f;
	float _StatusDotDamageMultiplier = 0.0f;
	float _Attack = 0.0f;
	float _StatAndCriticalDamage = 0.0f;
	float _StatAndCriticalRate = 0.0f;
};