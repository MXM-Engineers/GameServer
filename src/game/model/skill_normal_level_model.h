#pragma once
#include "../core.h"

class SkillNormalLevelModel {
public:
	SkillNormalLevelModel();
	~SkillNormalLevelModel();

	i32 getSkillIndex();
	void setSkillIndex(i32 skillIndex);
	i8 getLevel();
	void setLevel(i8 level);
	float getBaseDamage();
	void setBaseDamage(float baseDamage);
	float getAttackMultiplier();
	void setAttackMultiplier(float attackMultiplier);
	float getCoolTime();
	void setCoolTime(float coolTime);
	void setSkillRangeLengthX(float lenght);
	float getSkillRangeLengthX();
	void setSkillRangeLengthY(float lenght);
	float getSkillRangeLengthY();
	float getTargetRangeLengthX();
	void setTargetRangeLengthX(float lenght);
	float getTargetRangeLengthY();
	void setTargetRangeLengthY(float lenght);
	float getConsumeMP();
	void setConsumeMP(float mp);
	float getConsumeEP();
	void setConsumeEP(float ep);
	float getConsumeUG();
	void setConsumeUG(float ug);
	float getTargetMaxDistance();
	void setTargetMaxDistance(float distance);
	float getAddGroggy();
	void setAddGroggy(float groggy);
	//ToDo: check if for some of these float's we can use int

	void Print();

private:
	i32 _SkillIndex = 0;
	i8 _Level = 0;
	float _BaseDamage = 0.0f; //int?
	float _AttackMultiplier = 0.0f;
	float _CoolTime = 0.0f; //int?
	float _SkillRangeLengthX = 0.0f;
	float _SkillRangeLengthY = 0.0f;
	float _TargetRangeLengthX = 0.0f; //int?
	float _TargetRangeLengthY = 0.0f; //int?
	float _ConsumeMP = 0.0f; //int?
	float _ConsumeEP = 0.0f; //int?
	float _ConsumeUG = 0.0f; //int?
	float _TargetMaxDistance = 0.0f; //int?
	float _AddGroggy = 0.0f; //int?
};
