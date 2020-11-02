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

SkillNormalLevelModel SkillNormalModel::getSkillNormalLevelByIndex(i32 index)
{
	return _skillNormalLevelModel[index];
}

void SkillNormalModel::setSkillNormalLevelByIndex(SkillNormalLevelModel skill, i32 index)
{
	_skillNormalLevelModel[index] = skill;
}
