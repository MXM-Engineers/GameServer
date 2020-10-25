#pragma once
#include "../core.h"
#include "skill_normal_model.h"

class SkillsModel {
public:
	SkillsModel();
	~SkillsModel();

private:
	//passive skill
	//support skill
	SkillNormalModel _skills[4];
};
