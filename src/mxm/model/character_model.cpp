#include "character_model.h"

CharacterModel::CharacterModel()
	: CreatureModel(0, EntityType::INVALID, 0, 0, 0, 0, CreatureType::CREATURE_TYPE_INVALID, 0, 0, 0, 0)
{
}

CharacterModel::~CharacterModel()
{
}

SkillsModel* CharacterModel::getSkills()
{
	return &_skills;
}

//Fixme: this needs testing if it actually works correctly
void CharacterModel::setSkills(SkillsModel skills)
{
	_skills = skills;
}

void CharacterModel::Print()
{
	VERBOSE("[CharacterModel]");
	CreatureModel::Print();
	_skills.Print();
}
