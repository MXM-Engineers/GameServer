#include "skill_normal_model.h"

SkillNormalModel::SkillNormalModel()
{
}

SkillNormalModel::~SkillNormalModel()
{
}

i32 SkillNormalModel::getID()
{
	return _ID;
}

void SkillNormalModel::setID(i32 id)
{
	_ID = id;
}

SkillNormalLevelModel* SkillNormalModel::getSkillNormalLevelByIndex(i32 index)
{
	return &_skillNormalLevelModel[index];
}

void SkillNormalModel::setSkillNormalLevelByIndex(SkillNormalLevelModel skill, i32 index)
{
	_skillNormalLevelModel[index] = skill;
}

void SkillNormalModel::Print()
{
	VERBOSE("[SkillNormalModel]: %d", _ID);
	_skillNormalLevelModel[0].Print();
	_skillNormalLevelModel[1].Print();
	_skillNormalLevelModel[2].Print();
	_skillNormalLevelModel[3].Print();
	_skillNormalLevelModel[4].Print();
	_skillNormalLevelModel[5].Print();
}
