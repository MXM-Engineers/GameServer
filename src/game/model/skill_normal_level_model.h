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
