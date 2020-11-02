#include "skill_normal_level_model.h"

SkillNormalLevelModel::SkillNormalLevelModel()
{
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
	_TargetRangeLengthY;
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
