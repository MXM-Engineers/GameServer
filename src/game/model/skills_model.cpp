#include "skills_model.h"

SkillsModel::SkillsModel()
{
}

SkillsModel::~SkillsModel()
{
}

SkillNormalModel SkillsModel::getSkillByIndex(i32 index)
{
	return _skills[index];
}

void SkillsModel::setSkillByIndex(SkillNormalModel skill, i32 index)
{
	_skills[index] = skill;
}

SkillNormalModel SkillsModel::getUltimate()
{
	return _ultimate;
}

void SkillsModel::setUltimate(SkillNormalModel skill)
{
	_ultimate = skill;
}
