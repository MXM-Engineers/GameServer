#include "character_model.h"

CharacterModel::CharacterModel()
	: CreatureModel(0, EntityType::ENTITY_INVALID, 0, 0, 0, 0, CreatureType::CREATURE_INVALID)
{
}

CharacterModel::~CharacterModel()
{
}

SkillsModel CharacterModel::getSkills()
{
	return _skills;
}

void CharacterModel::setSkills(SkillsModel skills)
{
	_skills = skills;
}
