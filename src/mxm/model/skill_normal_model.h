#pragma once
#include "../core.h"
#include "skill_normal_level_model.h"

class SkillNormalModel {
public:
	SkillNormalModel();
	~SkillNormalModel();

	i32 getID();
	void setID(i32 id);
	SkillNormalLevelModel *getSkillNormalLevelByIndex(i32 index);
	void setSkillNormalLevelByIndex(SkillNormalLevelModel skill, i32 index);

	void Print();

	SkillType type = SkillType::INVALID;
	ActionStateID action = ActionStateID::INVALID;

private:
	i32 _ID = 0;
	SkillNormalLevelModel _skillNormalLevelModel[6];
};
