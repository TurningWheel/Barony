/*-------------------------------------------------------------------------------

	BARONY
	File: updatecharactersheet.cpp
	Desc: contains updateCharacterSheet()

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../main.hpp"
#include "../draw.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "../items.hpp"
#include "../player.hpp"
#include "../colors.hpp"
#include "interface.hpp"
#include "../engine/audio/sound.hpp"
#include "../magic/magic.hpp"
#include "../menu.hpp"
#include "../net.hpp"
#include "../scores.hpp"
#include "../ui/GameUI.hpp"
#include "../prng.hpp"

Sint32 displayAttackPower(const int player, AttackHoverText_t& output)
{
	Sint32 attack = 0;
	Entity* entity = nullptr;
	if ( players[player] )
	{
		entity = players[player]->entity;
		if ( stats[player] )
		{
			bool shapeshiftUseMeleeAttack = false;
			if ( entity && entity->effectShapeshift != NOTHING )
			{
				shapeshiftUseMeleeAttack = true;
				if ( entity && entity->effectShapeshift == CREATURE_IMP
					&& stats[player]->weapon && itemCategory(stats[player]->weapon) == MAGICSTAFF )
				{
					shapeshiftUseMeleeAttack = false;
				}
			}

			if ( !stats[player]->weapon || shapeshiftUseMeleeAttack )
			{
				// fists
				attack += Entity::getAttack(players[player]->entity, stats[player], true);
				output.hoverType = AttackHoverText_t::ATK_HOVER_TYPE_UNARMED; // melee
				output.totalAttack = attack;
				output.proficiencyBonus = (stats[player]->getModifiedProficiency(PRO_UNARMED) / 20); // bonus from proficiency
				output.mainAttributeBonus = statGetSTR(stats[player], entity); // bonus from main attribute
				output.equipmentAndEffectBonus = attack - statGetSTR(stats[player], entity) - BASE_PLAYER_UNARMED_DAMAGE - output.proficiencyBonus; // bonus from equipment

				{
					real_t variance = 20;
					real_t baseSkillModifier = 50.0; // 40-60 base
					Entity::setMeleeDamageSkillModifiers(players[player]->entity, stats[player], PRO_UNARMED, baseSkillModifier, variance, nullptr);
					real_t skillModifierMin = baseSkillModifier - (variance / 2) + (stats[player]->getModifiedProficiency(PRO_UNARMED) / 2.0);
					real_t skillModifierMax = skillModifierMin + variance;
					skillModifierMin /= 100.0;
					skillModifierMin = std::min(skillModifierMin, 1.0);
					skillModifierMax /= 100.0;
					skillModifierMax = std::min(skillModifierMax, 1.0);
					output.proficiencyVariance = variance;
					output.attackMaxRange = attack - static_cast<int>((1.0 - skillModifierMax) * attack);
					output.attackMinRange = attack - static_cast<int>((1.0 - skillModifierMin) * attack);
					output.proficiency = PRO_UNARMED;
					output.totalAttack = output.attackMaxRange - ((output.attackMaxRange - output.attackMinRange) / 2.0);
				}
			}
			else
			{
				int weaponskill = getWeaponSkill(stats[player]->weapon);
				if ( weaponskill == PRO_RANGED )
				{
					if ( isRangedWeapon(*stats[player]->weapon) )
					{
						if ( entity )
						{
							attack += entity->getRangedAttack();
						}
						output.hoverType = AttackHoverText_t::ATK_HOVER_TYPE_RANGED; // ranged
						output.totalAttack = attack;
						output.weaponBonus = stats[player]->weapon->weaponGetAttack(stats[player]); // bonus from weapon
						output.equipmentAndEffectBonus = 0;
						Sint32 quiverATK = 0;
						if ( stats[player]->shield && rangedWeaponUseQuiverOnAttack(stats[player]) )
						{
							quiverATK = stats[player]->shield->weaponGetAttack(stats[player]);
							attack += quiverATK;
						}
						output.mainAttributeBonus = statGetDEX(stats[player], entity); // bonus from main attribute
						output.equipmentAndEffectBonus += attack - output.mainAttributeBonus
							- BASE_RANGED_DAMAGE - output.weaponBonus; // bonus from equipment

						{
							real_t variance = 20;
							real_t baseSkillModifier = 50.0; // 40-60 base
							Entity::setMeleeDamageSkillModifiers(players[player]->entity, stats[player], weaponskill, baseSkillModifier, variance, nullptr);
							real_t skillModifierMin = baseSkillModifier - (variance / 2) + (stats[player]->getModifiedProficiency(weaponskill) / 2.0);
							real_t skillModifierMax = skillModifierMin + variance;
							skillModifierMin /= 100.0;
							skillModifierMin = std::min(skillModifierMin, 1.0);
							skillModifierMax /= 100.0;
							skillModifierMax = std::min(skillModifierMax, 1.0);
							output.proficiencyVariance = variance;
							output.attackMaxRange = attack - static_cast<int>((1.0 - skillModifierMax) * attack);
							output.attackMinRange = attack - static_cast<int>((1.0 - skillModifierMin) * attack);
							output.proficiency = weaponskill;
							output.totalAttack = output.attackMaxRange - ((output.attackMaxRange - output.attackMinRange) / 2.0);
						}
					}
					else if ( stats[player]->weapon && stats[player]->weapon->type == TOOL_WHIP )
					{
						attack += Entity::getAttack(players[player]->entity, stats[player], true);
						output.hoverType = AttackHoverText_t::ATK_HOVER_TYPE_WHIP; // ranged
						output.totalAttack = attack;
						output.weaponBonus = stats[player]->weapon->weaponGetAttack(stats[player]); // bonus from weapon
						Sint32 STR = statGetSTR(stats[player], entity);
						Sint32 DEX = statGetDEX(stats[player], entity);
						Sint32 totalAttributeBonus = (STR + DEX);
						totalAttributeBonus = std::min(totalAttributeBonus / 2, totalAttributeBonus);
						//output.mainAttributeBonus = totalAttributeBonus - STRComponent; // bonus from main attribute (DEX)
						//output.secondaryAttributeBonus = totalAttributeBonus - DEXComponent; // secondary (STR)
						output.mainAttributeBonus = totalAttributeBonus;
						output.equipmentAndEffectBonus += attack - totalAttributeBonus
							- BASE_MELEE_DAMAGE - output.weaponBonus; // bonus from equipment

						{
							real_t variance = 20;
							real_t baseSkillModifier = 50.0; // 40-60 base
							Entity::setMeleeDamageSkillModifiers(players[player]->entity, stats[player], weaponskill, baseSkillModifier, variance, nullptr);
							real_t skillModifierMin = baseSkillModifier - (variance / 2) + (stats[player]->getModifiedProficiency(weaponskill) / 2.0);
							real_t skillModifierMax = skillModifierMin + variance;
							skillModifierMin /= 100.0;
							skillModifierMin = std::min(skillModifierMin, 1.0);
							skillModifierMax /= 100.0;
							skillModifierMax = std::min(skillModifierMax, 1.0);
							output.proficiencyVariance = variance;
							output.attackMaxRange = attack - static_cast<int>((1.0 - skillModifierMax) * attack);
							output.attackMinRange = attack - static_cast<int>((1.0 - skillModifierMin) * attack);
							output.proficiency = weaponskill;
							output.totalAttack = output.attackMaxRange - ((output.attackMaxRange - output.attackMinRange) / 2.0);
						}
					}
					else
					{
						Sint32 skillLVL = stats[player]->getModifiedProficiency(PRO_RANGED);
						if ( entity )
						{
							attack += entity->getThrownAttack();
						}
						output.hoverType = AttackHoverText_t::ATK_HOVER_TYPE_THROWN; // thrown
						output.totalAttack = attack;
						// bonus from weapon
						output.weaponBonus = stats[player]->weapon->weaponGetAttack(stats[player]);
						// bonus from dex
						if ( itemCategory(stats[player]->weapon) != POTION )
						{
							Sint32 oldDEX = statGetDEX(stats[player], entity);
							Sint32 DEXComponent = oldDEX / 4;
							if ( itemCategory(stats[player]->weapon) == THROWN )
							{
								output.mainAttributeBonus = DEXComponent; //(output.totalAttack - entity->getThrownAttack());

								Sint32 oldSkill = stats[player]->getProficiency(PRO_RANGED);
								stats[player]->setProficiencyUnsafe(PRO_RANGED, -999);
								output.proficiencyBonus = output.totalAttack - (entity ? entity->getThrownAttack() : 0);
								stats[player]->setProficiency(PRO_RANGED, oldSkill);
							}
							else
							{
								if ( itemCategory(stats[player]->weapon) == GEM )
								{
									output.hoverType = AttackHoverText_t::ATK_HOVER_TYPE_THROWN_GEM; // thrown
								}
								// gems etc.
								Sint32 tmpDEX = stats[player]->DEX;
								stats[player]->DEX -= oldDEX;
								if ( entity )
								{
									output.mainAttributeBonus = (output.totalAttack - (entity ? entity->getThrownAttack() : 0));
								}
								output.proficiencyBonus = skillLVL / 10;
								stats[player]->DEX = tmpDEX;
							}
							output.proficiency = weaponskill;
						}
						else
						{
							output.hoverType = AttackHoverText_t::ATK_HOVER_TYPE_THROWN_POTION;
							output.mainAttributeBonus = 0;
							// bonus from proficiency
							Sint32 oldAlchemy = stats[player]->getProficiency(PRO_ALCHEMY);
							stats[player]->setProficiencyUnsafe(PRO_ALCHEMY, -999);
							output.proficiencyBonus = output.totalAttack - (entity ? entity->getThrownAttack() : 0);
							stats[player]->setProficiency(PRO_ALCHEMY, oldAlchemy);
							output.proficiency = PRO_ALCHEMY;
						}
						output.attackMaxRange = output.totalAttack;
						if ( itemCategory(stats[player]->weapon) == THROWN || itemCategory(stats[player]->weapon) == GEM )
						{
							output.attackMaxRange += 3; // maximum charge damage
						}
						output.attackMinRange = output.totalAttack;
						if ( itemCategory(stats[player]->weapon) == POTION )
						{
							output.attackMinRange -= (output.attackMinRange / 4);
							output.totalAttack -= (output.attackMaxRange - output.attackMinRange) / 2;
						}
						else
						{
							output.totalAttack = output.attackMinRange;
						}
					}
				}
				else if ( (weaponskill >= PRO_SWORD && weaponskill <= PRO_POLEARM) )
				{
					// melee weapon
					attack += Entity::getAttack(players[player]->entity, stats[player], true);
					output.hoverType = AttackHoverText_t::ATK_HOVER_TYPE_MELEE_WEAPON; // melee
					output.totalAttack = attack;
					output.weaponBonus = stats[player]->weapon->weaponGetAttack(stats[player]); // bonus from weapon
					output.mainAttributeBonus = statGetSTR(stats[player], entity); // bonus from main attribute
					if ( weaponskill == PRO_AXE )
					{
						output.weaponBonus += 1; // bonus from equipment
						output.totalAttack += 1;
						attack += 1;
					}

					{
						real_t variance = 20;
						real_t baseSkillModifier = 50.0; // 40-60 base
						Entity::setMeleeDamageSkillModifiers(players[player]->entity, stats[player], weaponskill, baseSkillModifier, variance, nullptr);
						real_t skillModifierMin = baseSkillModifier - (variance / 2) + (stats[player]->getModifiedProficiency(weaponskill) / 2.0);
						real_t skillModifierMax = skillModifierMin + variance;
						skillModifierMin /= 100.0;
						skillModifierMin = std::min(skillModifierMin, 1.0);
						skillModifierMax /= 100.0;
						skillModifierMax = std::min(skillModifierMax, 1.0);
						output.proficiencyVariance = variance;
						output.attackMaxRange = attack - static_cast<int>((1.0 - skillModifierMax) * attack);
						output.attackMinRange = attack - static_cast<int>((1.0 - skillModifierMin) * attack);
						output.proficiency = weaponskill;
						output.totalAttack = output.attackMaxRange - ((output.attackMaxRange - output.attackMinRange) / 2.0);
					}
				}
				else if ( itemCategory(stats[player]->weapon) == MAGICSTAFF ) // staffs.
				{
					attack = 0;
					output.hoverType = AttackHoverText_t::ATK_HOVER_TYPE_MAGICSTAFF; // staffs
					output.totalAttack = attack;
					output.attackMaxRange = output.totalAttack;
					output.attackMinRange = output.totalAttack;
					output.proficiency = PRO_SPELLCASTING;
				}
				else // tools etc.
				{
					attack += Entity::getAttack(players[player]->entity, stats[player], true);
					output.hoverType = AttackHoverText_t::ATK_HOVER_TYPE_TOOL; // tools
					if ( stats[player]->weapon->type == TOOL_PICKAXE )
					{
						output.hoverType = AttackHoverText_t::ATK_HOVER_TYPE_PICKAXE;
					}
					else if ( itemIsThrowableTinkerTool(stats[player]->weapon) || stats[player]->weapon->type == TOOL_BEARTRAP )
					{
						output.hoverType = AttackHoverText_t::ATK_HOVER_TYPE_TOOL_TRAP;
					}
					output.totalAttack = attack;
					output.weaponBonus = 0; // bonus from weapon
					output.mainAttributeBonus = statGetSTR(stats[player], entity); // bonus from main attribute
					output.proficiencyBonus = 0; // bonus from proficiency
					output.equipmentAndEffectBonus = attack - output.mainAttributeBonus - BASE_MELEE_DAMAGE; // bonus from equipment
					output.attackMaxRange = output.totalAttack;
					output.attackMinRange = output.totalAttack;
					output.proficiency = PRO_LOCKPICKING;
					output.totalAttack = output.attackMaxRange - ((output.attackMaxRange - output.attackMinRange) / 2.0);
				}
			}
		}
	}
	else
	{
		attack = 0;
	}
	//return attack;
	return output.totalAttack;
}