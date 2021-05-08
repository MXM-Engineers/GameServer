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
	LOG("[SkillsModel]:");
	LOG("skill 1:\n");
	_skills[0].Print();
	LOG("\nskill 2:\n");
	_skills[1].Print();
	LOG("\nskill 3:\n");
	_skills[2].Print();
	LOG("\nskill 4:\n");
	_skills[3].Print();
	LOG("\nUltimate:\n");
	_ultimate.Print();
}
