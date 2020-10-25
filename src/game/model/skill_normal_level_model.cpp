#include "skill_normal_level_model.h"

SkillNormalLevelModel::SkillNormalLevelModel()
{
}

SkillNormalLevelModel::~SkillNormalLevelModel()
{
}

i32 SkillNormalLevelModel::getSkillIndex()
{
	return this->_SkillIndex;
}

void SkillNormalLevelModel::setSkillIndex(i32 skillIndex)
{
	this->_SkillIndex = skillIndex;
}

i8 SkillNormalLevelModel::getLevel()
{
	return this->_Level;
}

void SkillNormalLevelModel::setLevel(i8 level)
{
	this->_Level = level;
}

float SkillNormalLevelModel::getBaseDamage()
{
	return this->_BaseDamage;
}

void SkillNormalLevelModel::setBaseDamage(float baseDamage)
{
	this->_BaseDamage = baseDamage;
}

float SkillNormalLevelModel::getAttackMultiplier()
{
	return this->getAttackMultiplier;
}

void SkillNormalLevelModel::setAttackMultiplier(float attackMultiplier)
{
	this->_AttackMultiplier = attackMultiplier;
}

float SkillNormalLevelModel::getCoolTime()
{
	return this->_CoolTime;
}

void SkillNormalLevelModel::setCoolTime(float coolTime)
{
	this->_CoolTime = coolTime;
}

float SkillNormalLevelModel::getTargetRangeLengthX()
{
	return _TargetRangeLengthX;
}

void SkillNormalLevelModel::setTargetRangeLengthX(float lenght)
{
	this->getTargetRangeLengthX = lenght;
}

float SkillNormalLevelModel::getTargetRangeLengthY()
{
	return _TargetRangeLengthY;
}

void SkillNormalLevelModel::setTargetRangeLengthY(float lenght)
{
	this->_TargetRangeLengthY = lenght;
}

float SkillNormalLevelModel::getConsumeMP()
{
	return _ConsumeMP;
}

void SkillNormalLevelModel::setConsumeMP(float mp)
{
	this->_ConsumeMP = mp;
}

float SkillNormalLevelModel::getConsumeEP()
{
	return this->_ConsumeEP;
}

void SkillNormalLevelModel::setConsumeEP(float ep)
{
	this->_ConsumeEP = ep;
}

float SkillNormalLevelModel::getConsumeUG()
{
	return this->_ConsumeUG;
}

void SkillNormalLevelModel::setConsumeUG(float ug)
{
	this->_ConsumeUG = ug;
}

float SkillNormalLevelModel::getTargetMaxDistance()
{
	return _TargetMaxDistance;
}

void SkillNormalLevelModel::setTargetMaxDistance(float distance)
{
	this->_TargetMaxDistance = distance;
}

float SkillNormalLevelModel::getAddGroggy()
{
	return this->_AddGroggy;
}

void SkillNormalLevelModel::setAddGroggy(float groggy)
{
	this->_AddGroggy = groggy;
}
