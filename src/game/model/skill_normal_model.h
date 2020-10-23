#pragma once
#include "../core.h"
#include "skill_normal_level_model.h"

class SkillNormalModel {
public:
	SkillNormalModel();
	~SkillNormalModel();

private:
	i32 _ID;
	SkillNormalLevelModel _skillNormalLevelModel[6];
};
