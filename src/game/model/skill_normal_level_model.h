#pragma once
#include "../core.h"

class SkillNormalLevelModel {
public:
	SkillNormalLevelModel();
	~SkillNormalLevelModel();

	//ToDo: check if for some of these float's we can use int
private:
	i32 _SkillIndex;
	i8 _Level;
	float _BaseDamage; //int?
	float _AttackMultiplier;
	float _CoolTime; //int?
	float _TargetRangeLengthX; //int?
	float _TargetRangeLengthY; //int?
	float _ConsumeMP; //int?
	float _ConsumeEP; //int?
	float _ConsumeUG; //int?
	float _TargetMaxDistance; //int?
	float _AddGroggy; //int?
};
