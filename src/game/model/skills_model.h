#pragma once
#include "../core.h"
#include "skill_normal_model.h"

class SkillsModel {
public:
	SkillsModel();
	~SkillsModel();

	SkillNormalModel* getSkillByIndex(i32 index);
	void setSkillByIndex(SkillNormalModel skill, i32 index);
	SkillNormalModel* getUltimate();
	void setUltimate(SkillNormalModel skill);

	void Print();

private:
	//passive skill
	//support skill
	SkillNormalModel _skills[4];
	SkillNormalModel _ultimate;
};
