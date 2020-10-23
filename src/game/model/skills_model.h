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
	SkillNormalModel _skill1;
	SkillNormalModel _skill2;
	SkillNormalModel _skill3;
	SkillNormalModel _skill4;
};
