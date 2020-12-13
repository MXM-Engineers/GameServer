#pragma once
#include "../core.h"
#include "creature_model.h"
#include "skills_model.h"

class CharacterModel :
	public CreatureModel {
public:
	CharacterModel();
	~CharacterModel();

	SkillsModel getSkills();
	void setSkills(SkillsModel skills);
	SkillNormalModel getSkillByID(u8 id);

private:
	SkillsModel _skills;
};
