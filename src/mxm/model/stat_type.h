#pragma once

class StatType
{
public:
	StatType();
	~StatType();

	float getHealth();
	void setHealth(float var);
	float getSkill();
	void setSkill(float var);
	float getStamina();
	void setStamina(float var);
	float getHealthRegen();
	void setHealthRegen(float var);
	float getSkillRegen();
	void setSkillRegen(float var);
	float getAttack();
	void setAttack(float var);
	float getDefence();
	void setDefence(float var);
	float getSkillPower();
	void setSkillPower(float var);
	float getCriticalRate();
	void setCriticalRate(float var);
	float getCriticalDamage();
	void setCriticalDamage(float var);
	float getWeakCriticalRate();
	void setWeakCriticalRate(float var);
	float getLifeSteal();
	void setLifeSteal(float var);
	float getBaseSpeed();
	void setBaseSpeed(float var);
	float getMoveSpeed();
	void setMoveSpeed(float var);
	float getAttackSpeed();
	void setAttackSpeed(float var);
	float getAbilityPoint();
	void setAbilityPoint(float var);
	float getEnergy();
	void setEnergy(float var);
	float getRotateSpeed();
	void setRotateSpeed(float var);
	float getShieldDefence();
	void setShieldDefence(float var);
	float getDefencePenetration();
	void setDefencePenetration(float var);
	float getSkillDefencePenetration();
	void setSkillDefencePenetration(float var);
	float getPickupRadius();
	void setPickupRadius(float var);
	float getTagHealthRegenValue();
	void setTagHealthRegenValue(float var);

private:
	float _Health = 0.0f;
	float _Skill = 0.0f;
	float _Stamina = 0.0f;
	float _HealthRegen = 0.0f;
	float _SkillRegen = 0.0f;
	float _Attack = 0.0f;
	float _Defence = 0.0f;
	float _SkillPower = 0.0f;
	float _CiritcalRate = 0.0f;
	float _CiritcalDamage = 0.0f;
	float _WeakCiritcalRate = 0.0f;
	float _LifeSteal = 0.0f;
	float _BaseSpeed = 0.0f;
	float _MoveSpeed = 0.0f;
	float _AttackSpeed = 0.0f;
	float _AbilityPoint = 0.0f;
	float _Energy = 0.0f;
	float _RotateSpeed = 0.0f;
	float _ShieldDefence = 0.0f;
	float _DefencePenetration = 0.0f;
	float _SkillDefencePenetration = 0.0f;
	float _PickupRadius = 0.0f;
	float _TagHealthRegenValue = 0.0f;

};
