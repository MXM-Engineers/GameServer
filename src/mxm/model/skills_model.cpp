#include "skills_model.h"

SkillsModel::SkillsModel()
{
}

SkillsModel::~SkillsModel()
{
}

SkillNormalModel* SkillsModel::getSkillByIndex(i32 index)
{
	return &_skills[index];
}

void SkillsModel::setSkillByIndex(SkillNormalModel skill, i32 index)
{
	_skills[index] = skill;
}

SkillNormalModel* SkillsModel::getUltimate()
{
	return &_ultimate;
}

void SkillsModel::setUltimate(SkillNormalModel skill)
{
	_ultimate = skill;
}

void SkillsModel::Print()
{
	VERBOSE("[SkillsModel]:");
	VERBOSE("skill 1:\n");
	_skills[0].Print();
	VERBOSE("\nskill 2:\n");
	_skills[1].Print();
	VERBOSE("\nskill 3:\n");
	_skills[2].Print();
	VERBOSE("\nskill 4:\n");
	_skills[3].Print();
	VERBOSE("\nUltimate:\n");
	_ultimate.Print();
}
