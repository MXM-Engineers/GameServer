#pragma once
#include "../core.h"
#include "creature_model.h"
#include "skills_model.h"

class CharacterModel :
	public CreatureModel {
public:
	CharacterModel();
	~CharacterModel();

private:
	SkillsModel _skills;
};
