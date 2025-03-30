#include "skill_normal_level_model.h"

SkillNormalLevelModel::SkillNormalLevelModel()
{
	_AddGroggy = 0.0f;
	_AttackMultiplier = 0.0f;
	_BaseDamage = 0.0f;
	_ConsumeEP = 0.0f;
	_ConsumeMP = 0.0f;
	_ConsumeUG = 0.0f;
	_CoolTime = 0.0f;
	_Level = 0;
	_SkillIndex = 0;
	_TargetMaxDistance = 0.0f;
	_TargetRangeLengthX = 0.0f;
	_TargetRangeLengthY = 0.0f;
}

SkillNormalLevelModel::~SkillNormalLevelModel()
{
}

i32 SkillNormalLevelModel::getSkillIndex()
{
	return _SkillIndex;
}

void SkillNormalLevelModel::setSkillIndex(i32 skillIndex)
{
	_SkillIndex = skillIndex;
}

i8 SkillNormalLevelModel::getLevel()
{
	return _Level;
}

void SkillNormalLevelModel::setLevel(i8 level)
{
	_Level = level;
}

float SkillNormalLevelModel::getBaseDamage()
{
	return _BaseDamage;
}

void SkillNormalLevelModel::setBaseDamage(float baseDamage)
{
	_BaseDamage = baseDamage;
}

float SkillNormalLevelModel::getAttackMultiplier()
{
	return _AttackMultiplier;
}

void SkillNormalLevelModel::setAttackMultiplier(float attackMultiplier)
{
	_AttackMultiplier = attackMultiplier;
}

float SkillNormalLevelModel::getCoolTime()
{
	return _CoolTime;
}

void SkillNormalLevelModel::setCoolTime(float coolTime)
{
	_CoolTime = coolTime;
}

void SkillNormalLevelModel::setSkillRangeLengthX(float lenght)
{
	_SkillRangeLengthX = lenght;
}

float SkillNormalLevelModel::getSkillRangeLengthX()
{
	return _SkillRangeLengthX;
}

void SkillNormalLevelModel::setSkillRangeLengthY(float lenght)
{
	_SkillRangeLengthY = lenght;
}

float SkillNormalLevelModel::getSkillRangeLengthY()
{
	return _SkillRangeLengthY;
}

float SkillNormalLevelModel::getTargetRangeLengthX()
{
	return _TargetRangeLengthX;
}

void SkillNormalLevelModel::setTargetRangeLengthX(float lenght)
{
	_TargetRangeLengthX = lenght;
}

float SkillNormalLevelModel::getTargetRangeLengthY()
{
	return _TargetRangeLengthY;
}

void SkillNormalLevelModel::setTargetRangeLengthY(float lenght)
{
	_TargetRangeLengthY = lenght;
}

float SkillNormalLevelModel::getConsumeMP()
{
	return _ConsumeMP;
}

void SkillNormalLevelModel::setConsumeMP(float mp)
{
	_ConsumeMP = mp;
}

float SkillNormalLevelModel::getConsumeEP()
{
	return _ConsumeEP;
}

void SkillNormalLevelModel::setConsumeEP(float ep)
{
	_ConsumeEP = ep;
}

float SkillNormalLevelModel::getConsumeUG()
{
	return _ConsumeUG;
}

void SkillNormalLevelModel::setConsumeUG(float ug)
{
	_ConsumeUG = ug;
}

float SkillNormalLevelModel::getTargetMaxDistance()
{
	return _TargetMaxDistance;
}

void SkillNormalLevelModel::setTargetMaxDistance(float distance)
{
	_TargetMaxDistance = distance;
}

float SkillNormalLevelModel::getAddGroggy()
{
	return _AddGroggy;
}

void SkillNormalLevelModel::setAddGroggy(float groggy)
{
	_AddGroggy = groggy;
}

void SkillNormalLevelModel::Print()
{
	VERBOSE("[SkillNormalLevelModel]: skillIndex: %d, level: %d, Base Damage: %f, Attack Multiplier: %f, Cool Time: %f, Skill Range Length X: %f, Skill Range Length Y: %f, Target Range Length X: %f, Target Range Lenght Y: %f, Consume MP: %f, Consume EP: %f, Consume UG: %f, Target Max Distance: %f, Add Groggy: %f",
		_SkillIndex, _Level, _BaseDamage, _AttackMultiplier, _CoolTime, _SkillRangeLengthX, _SkillRangeLengthY, _TargetRangeLengthX, _TargetRangeLengthY, _ConsumeMP, _ConsumeEP, _ConsumeUG, _TargetMaxDistance, _AddGroggy);
}
