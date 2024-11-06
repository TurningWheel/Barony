/*-------------------------------------------------------------------------------

	BARONY
	File: spell.cpp
	Desc: contains spell definitions

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../main.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "../entity.hpp"
#include "../interface/interface.hpp"
#include "../engine/audio/sound.hpp"
#include "../items.hpp"
#include "../net.hpp"
#include "../player.hpp"
#include "magic.hpp"
#include "../mod_tools.hpp"
#include "../ui/GameUI.hpp"

list_t channeledSpells[MAXPLAYERS];

spellElement_t spellElement_unintelligible;
spellElement_t spellElement_missile;
spellElement_t spellElement_missile_trio;
spellElement_t spellElement_force;
spellElement_t spellElement_fire;
spellElement_t spellElement_lightning;
spellElement_t spellElement_light;
spellElement_t spellElement_dig;
spellElement_t spellElement_invisible;
spellElement_t spellElement_identify;
spellElement_t spellElement_magicmapping;
spellElement_t spellElement_heal;
spellElement_t spellElement_confuse;
spellElement_t spellElement_cure_ailment;
spellElement_t spellElement_locking;
spellElement_t spellElement_opening;
spellElement_t spellElement_sleep;
spellElement_t spellElement_cold;
spellElement_t spellElement_slow;
spellElement_t spellElement_levitation;
spellElement_t spellElement_teleportation;
spellElement_t spellElement_magicmissile;
spellElement_t spellElement_removecurse;
spellElement_t spellElement_summon;
spellElement_t spellElement_stoneblood;
spellElement_t spellElement_bleed;
spellElement_t spellElement_dominate;
spellElement_t spellElement_reflectMagic;
spellElement_t spellElement_acidSpray;
spellElement_t spellElement_stealWeapon;
spellElement_t spellElement_drainSoul;
spellElement_t spellElement_vampiricAura;
spellElement_t spellElement_charmMonster;
spellElement_t spellElement_shapeshift;
spellElement_t spellElement_sprayWeb;
spellElement_t spellElement_poison;
spellElement_t spellElement_speed;
spellElement_t spellElement_fear;
spellElement_t spellElement_strike;
spellElement_t spellElement_detectFood;
spellElement_t spellElement_weakness;
spellElement_t spellElement_amplifyMagic;
spellElement_t spellElement_shadowTag;
spellElement_t spellElement_telePull;
spellElement_t spellElement_demonIllusion;
spellElement_t spellElement_trollsBlood;
spellElement_t spellElement_salvageItem;
spellElement_t spellElement_flutter;
spellElement_t spellElement_dash;
spellElement_t spellElement_selfPolymorph;
spellElement_t spellElement_ghostBolt;
spellElement_t spellElement_slimeAcid;
spellElement_t spellElement_slimeWater;
spellElement_t spellElement_slimeFire;
spellElement_t spellElement_slimeTar;
spellElement_t spellElement_slimeMetal;
spellElement_t spellElement_slime_spray;

spell_t spell_forcebolt;
spell_t spell_magicmissile;
spell_t spell_cold;
spell_t spell_fireball;
spell_t spell_lightning;
spell_t spell_removecurse;
spell_t spell_light;
spell_t spell_identify;
spell_t spell_magicmapping;
spell_t spell_sleep;
spell_t spell_confuse;
spell_t spell_slow;
spell_t spell_opening;
spell_t spell_locking;
spell_t spell_levitation;
spell_t spell_invisibility;
spell_t spell_teleportation;
spell_t spell_healing;
spell_t spell_extrahealing;
//spell_t spell_restoreability;
spell_t spell_cureailment;
spell_t spell_dig;
spell_t spell_summon;
spell_t spell_stoneblood;
spell_t spell_bleed;
spell_t spell_dominate;
spell_t spell_reflectMagic;
spell_t spell_acidSpray;
spell_t spell_stealWeapon;
spell_t spell_drainSoul;
spell_t spell_vampiricAura;
spell_t spell_charmMonster;
spell_t spell_revertForm;
spell_t spell_ratForm;
spell_t spell_spiderForm;
spell_t spell_trollForm;
spell_t spell_impForm;
spell_t spell_sprayWeb;
spell_t spell_poison;
spell_t spell_speed;
spell_t spell_fear;
spell_t spell_strike;
spell_t spell_detectFood;
spell_t spell_weakness;
spell_t spell_amplifyMagic;
spell_t spell_shadowTag;
spell_t spell_telePull;
spell_t spell_demonIllusion;
spell_t spell_trollsBlood;
spell_t spell_salvageItem;
spell_t spell_flutter;
spell_t spell_dash;
spell_t spell_polymorph;
spell_t spell_ghost_bolt;
spell_t spell_slime_acid;
spell_t spell_slime_water;
spell_t spell_slime_fire;
spell_t spell_slime_tar;
spell_t spell_slime_metal;

bool addSpell(int spell, int player, bool ignoreSkill)
{
	node_t* node = nullptr;

	// this is a local function
	if ( !players[player]->isLocalPlayer() )
	{
		return false;
	}

	spell_t* new_spell = nullptr;

	switch ( spell )
	{
		case SPELL_FORCEBOLT:
			new_spell = copySpell(&spell_forcebolt);
			break;
		case SPELL_MAGICMISSILE:
			new_spell = copySpell(&spell_magicmissile);
			break;
		case SPELL_COLD:
			new_spell = copySpell(&spell_cold);
			break;
		case SPELL_FIREBALL:
			new_spell = copySpell(&spell_fireball);
			break;
		case SPELL_LIGHTNING:
			new_spell = copySpell(&spell_lightning);
			break;
		case SPELL_REMOVECURSE:
			new_spell = copySpell(&spell_removecurse);
			break;
		case SPELL_LIGHT:
			new_spell = copySpell(&spell_light);
			break;
		case SPELL_IDENTIFY:
			new_spell = copySpell(&spell_identify);
			break;
		case SPELL_MAGICMAPPING:
			new_spell = copySpell(&spell_magicmapping);
			break;
		case SPELL_SLEEP:
			new_spell = copySpell(&spell_sleep);
			break;
		case SPELL_CONFUSE:
			new_spell = copySpell(&spell_confuse);
			break;
		case SPELL_SLOW:
			new_spell = copySpell(&spell_slow);
			break;
		case SPELL_OPENING:
			new_spell = copySpell(&spell_opening);
			break;
		case SPELL_LOCKING:
			new_spell = copySpell(&spell_locking);
			break;
		case SPELL_LEVITATION:
			new_spell = copySpell(&spell_levitation);
			break;
		case SPELL_INVISIBILITY:
			new_spell = copySpell(&spell_invisibility);
			break;
		case SPELL_TELEPORTATION:
			new_spell = copySpell(&spell_teleportation);
			break;
		case SPELL_HEALING:
			new_spell = copySpell(&spell_healing);
			break;
		case SPELL_EXTRAHEALING:
			new_spell = copySpell(&spell_extrahealing);
			break;
		case SPELL_CUREAILMENT:
			new_spell = copySpell(&spell_cureailment);
			break;
		case SPELL_DIG:
			new_spell = copySpell(&spell_dig);
			break;
		case SPELL_STONEBLOOD:
			new_spell = copySpell(&spell_stoneblood);
			break;
		case SPELL_BLEED:
			new_spell = copySpell(&spell_bleed);
			break;
		case SPELL_SUMMON:
			new_spell = copySpell(&spell_summon);
			break;
		case SPELL_DOMINATE:
			new_spell = copySpell(&spell_dominate);
			break;
		case SPELL_REFLECT_MAGIC:
			new_spell = copySpell(&spell_reflectMagic);
			break;
		case SPELL_ACID_SPRAY:
			new_spell = copySpell(&spell_acidSpray);
			break;
		case SPELL_STEAL_WEAPON:
			new_spell = copySpell(&spell_stealWeapon);
			break;
		case SPELL_DRAIN_SOUL:
			new_spell = copySpell(&spell_drainSoul);
			break;
		case SPELL_VAMPIRIC_AURA:
			new_spell = copySpell(&spell_vampiricAura);
			break;
		case SPELL_CHARM_MONSTER:
			new_spell = copySpell(&spell_charmMonster);
			break;
		case SPELL_REVERT_FORM:
			new_spell = copySpell(&spell_revertForm);
			break;
		case SPELL_RAT_FORM:
			new_spell = copySpell(&spell_ratForm);
			break;
		case SPELL_SPIDER_FORM:
			new_spell = copySpell(&spell_spiderForm);
			break;
		case SPELL_TROLL_FORM:
			new_spell = copySpell(&spell_trollForm);
			break;
		case SPELL_IMP_FORM:
			new_spell = copySpell(&spell_impForm);
			break;
		case SPELL_SPRAY_WEB:
			new_spell = copySpell(&spell_sprayWeb);
			break;
		case SPELL_POISON:
			new_spell = copySpell(&spell_poison);
			break;
		case SPELL_SPEED:
			new_spell = copySpell(&spell_speed);
			break;
		case SPELL_FEAR:
			new_spell = copySpell(&spell_fear);
			break;
		case SPELL_STRIKE:
			new_spell = copySpell(&spell_strike);
			break;
		case SPELL_DETECT_FOOD:
			new_spell = copySpell(&spell_detectFood);
			break;
		case SPELL_WEAKNESS:
			new_spell = copySpell(&spell_weakness);
			break;
		case SPELL_AMPLIFY_MAGIC:
			new_spell = copySpell(&spell_amplifyMagic);
			break;
		case SPELL_SHADOW_TAG:
			new_spell = copySpell(&spell_shadowTag);
			break;
		case SPELL_TELEPULL:
			new_spell = copySpell(&spell_telePull);
			break;
		case SPELL_DEMON_ILLUSION:
			new_spell = copySpell(&spell_demonIllusion);
			break;
		case SPELL_TROLLS_BLOOD:
			new_spell = copySpell(&spell_trollsBlood);
			break;
		case SPELL_SALVAGE:
			new_spell = copySpell(&spell_salvageItem);
			break;
		case SPELL_FLUTTER:
			new_spell = copySpell(&spell_flutter);
			break;
		case SPELL_DASH:
			new_spell = copySpell(&spell_dash);
			break;
		case SPELL_SELF_POLYMORPH:
			new_spell = copySpell(&spell_polymorph);
			break;
		case SPELL_CRAB_FORM:
		    new_spell = copySpell(&spell_spiderForm);
		    break;
		case SPELL_CRAB_WEB:
			new_spell = copySpell(&spell_sprayWeb);
			break;
		case SPELL_GHOST_BOLT:
			new_spell = copySpell(&spell_ghost_bolt);
			break;
		case SPELL_SLIME_ACID:
			new_spell = copySpell(&spell_slime_acid);
			break;
		case SPELL_SLIME_WATER:
			new_spell = copySpell(&spell_slime_water);
			break;
		case SPELL_SLIME_FIRE:
			new_spell = copySpell(&spell_slime_fire);
			break;
		case SPELL_SLIME_TAR:
			new_spell = copySpell(&spell_slime_tar);
			break;
		case SPELL_SLIME_METAL:
			new_spell = copySpell(&spell_slime_metal);
			break;
		default:
			return false;
	}
	if ( spellInList(&players[player]->magic.spellList, new_spell) )
	{
		// check inventory for shapeshift spells
		bool foundShapeshiftSpell = false;
		bool foundNormalSpell = false;
		if ( !ignoreSkill ) // learning the proper way, not being forced at character creation or debug.
		{
			for ( node = stats[player]->inventory.first; node != NULL; node = node->next )
			{
				Item* item = (Item*)node->element;
				if ( item->type == SPELL_ITEM )
				{
					if ( item->appearance >= 1000 )
					{
						if ( item->appearance - 1000 == spell )
						{
							foundShapeshiftSpell = true;
						}
					}
					else
					{
						if ( item->appearance == spell )
						{
							foundNormalSpell = true;
						}
					}
				}
				if ( foundNormalSpell && foundShapeshiftSpell )
				{
					// found both varieties, no point searching rest of inventory.
					break;
				}
			}
			if ( !foundNormalSpell )
			{
				// can learn. proceed.
			}
			else if ( foundNormalSpell )
			{
				// can't learn, already have it.
				messagePlayer(player, MESSAGE_STATUS, Language::get(439), new_spell->getSpellName());
				spellDeconstructor((void*)new_spell);
				playSoundPlayer(player, 90, 64);
				return false;
			}
		}
		else
		{
			// can't learn, already have it.
			if ( !(spell == SPELL_FORCEBOLT && skillCapstoneUnlocked(player, PRO_SPELLCASTING)) )
			{
				messagePlayer(player, MESSAGE_STATUS, Language::get(439), new_spell->getSpellName());
			}
			spellDeconstructor((void*)new_spell);
			return false;
		}
	}
	int skillLVL = stats[player]->getModifiedProficiency(PRO_MAGIC) + statGetINT(stats[player], players[player]->entity);
	if ( stats[player]->getModifiedProficiency(PRO_MAGIC) >= 100 )
	{
		skillLVL = 100;
	}
	if ( !ignoreSkill && skillLVL < new_spell->difficulty )
	{
		messagePlayer(player, MESSAGE_PROGRESSION, Language::get(440));
		spellDeconstructor((void*)new_spell);
		playSoundPlayer(player, 90, 64);
		return false;
	}
	if ( !intro )
	{
		messagePlayer(player, MESSAGE_PROGRESSION, Language::get(441), new_spell->getSpellName());
		skillUpAnimation[player].addSpellLearned(new_spell->ID);
	}
	node = list_AddNodeLast(&players[player]->magic.spellList);
	node->element = new_spell;
	node->size = sizeof(spell_t);
	node->deconstructor = &spellDeconstructor;

	if ( !ignoreSkill )
	{
		if ( players[player]->entity )
		{
			players[player]->entity->increaseSkill(PRO_MAGIC);
		}
	}

	Item* item = newItem(SPELL_ITEM, SERVICABLE, 0, 1, spell, true, nullptr);
	if ( Item* pickedUp = itemPickup(player, item) )
	{
		if ( !intro )
		{
			pickedUp->notifyIcon = true;
			if ( players[player]->magic.spellbookUidFromHotbarSlot != 0 )
			{
				if ( autoAddHotbarFilter(*pickedUp) )
				{
					for ( auto& slot : players[player]->hotbar.slots() )
					{
						if ( slot.item == players[player]->magic.spellbookUidFromHotbarSlot )
						{
							slot.item = pickedUp->uid;
							break;
						}
					}
				}
				players[player]->magic.spellbookUidFromHotbarSlot = 0;
			}
		}
	}
	free(item);

	if ( !ignoreSkill || (ignoreSkill && !intro) )
	{
		Compendium_t::Events_t::eventUpdateWorld(player, Compendium_t::CPDM_SPELLS_LEARNED, "magicians guild", 1);
	}
	return true;
}

void spellConstructor(spell_t* spell)
{
	spell->ID = -1;
	strcpy(spell->spell_internal_name, "spell_default");
	spell->elements.first = NULL;
	spell->elements.last = NULL;
	spell->sustain = true;
	spell->magicstaff = false;
	spell->sustain_node = NULL;
	spell->magic_effects_node = NULL;
	spell->caster = -1;
	spell->channel_duration = 0;
	//spell->timer = 0;
	allGameSpells.push_back(spell);
}

void spellDeconstructor(void* data)
{
	spell_t* spell;

	if ( data != NULL )
	{
		spell = (spell_t*) data;

		if ( spell_isChanneled(spell) )
		{
			if ( spell->sustain_node )
			{
				list_RemoveNode(spell->sustain_node);
			}
		}

		list_FreeAll(&spell->elements);
		free(spell);
	}
}

void spellElementConstructor(spellElement_t* element)
{
	element->mana = 0;
	element->base_mana = 0;
	element->overload_multiplier = 1;
	element->damage = 0;
	element->duration = 0;
	element->can_be_learned = true;
	strcpy(element->element_internal_name, "element_default");
	element->elements.first = NULL;
	element->elements.last = NULL;
	element->node = NULL;
	element->channeled = false;
}

void spellElementDeconstructor(void* data)
{
	spellElement_t* spellElement;
	if (data != NULL)
	{
		spellElement = (spellElement_t*) data;

		list_FreeAll(&spellElement->elements);
		free(spellElement);
	}
}

spellElement_t* newSpellelement()
{
	spellElement_t* element = (spellElement_t*) malloc(sizeof(spellElement_t));
	spellElementConstructor(element);
	return element;
}

spell_t* copySpell(spell_t* spell)
{
	node_t* node;

	spell_t* result = (spell_t*) malloc(sizeof(spell_t));
	*result = *spell; // copy over all the static data members.

	result->sustain_node = NULL;
	result->magic_effects_node = NULL;
	result->elements.first = NULL;
	result->elements.last = NULL;

	for ( node = spell->elements.first; node != NULL; node = node->next )
	{
		spellElement_t* tempElement = (spellElement_t*) node->element;

		node_t* tempNode = list_AddNodeLast(&result->elements);
		tempNode->deconstructor = &spellElementDeconstructor;
		tempNode->size = sizeof(spellElement_t);
		tempNode->element = copySpellElement(tempElement);

		spellElement_t* newElement = (spellElement_t*)tempNode->element;
		newElement->node = tempNode;
	}

	return result;
}

spellElement_t* copySpellElement(spellElement_t* spellElement)
{
	node_t* node;

	spellElement_t* result = (spellElement_t*) malloc(sizeof(spellElement_t));
	*result = *spellElement; // copy over all the static data members.

	result->node = NULL;
	result->elements.first = NULL;
	result->elements.last = NULL;

	for ( node = spellElement->elements.first; node != NULL; node = node->next )
	{
		spellElement_t* tempElement = (spellElement_t*) node->element;

		node_t* tempNode = list_AddNodeLast(&result->elements);
		tempNode->deconstructor = &spellElementDeconstructor;
		tempNode->size = sizeof(spellElement_t);
		tempNode->element = copySpellElement(tempElement);

		spellElement_t* newElement = (spellElement_t*)tempNode->element;
		newElement->node = tempNode;
	}

	return result;
}

int getCostOfSpell(spell_t* spell, Entity* caster)
{
	int cost = 0;
	if ( !spell )
	{
		return 0;
	}

	node_t* node;
	for ( node = spell->elements.first; node != NULL; node = node->next )
	{
		spellElement_t* spellElement = (spellElement_t*)node->element;
		cost += getCostOfSpellElement(spellElement);
	}

	if ( spell->ID == SPELL_FORCEBOLT && caster && caster->skillCapstoneUnlockedEntity(PRO_SPELLCASTING) )
	{
		cost = 0;
	}
	else if ( spell->ID == SPELL_SUMMON && caster )
	{
		Stat* casterStats = caster->getStats();
		if ( casterStats )
		{
			int summonLevel = casterStats->playerSummonLVLHP >> 16;
			cost += 5 * (summonLevel / 5);
		}
	}
	else if ( caster && (spell->ID == SPELL_TELEPORTATION || spell->ID == SPELL_TELEPULL) )
	{
		if ( caster->creatureShadowTaggedThisUid != 0 && uidToEntity(caster->creatureShadowTaggedThisUid) )
		{
			cost /= 2;
		}
	}

	return cost;
}

int getCostOfSpellElement(spellElement_t* spellElement)
{
	int cost = spellElement->mana;

	node_t* node;
	for ( node = spellElement->elements.first; node != NULL; node = node->next )
	{
		spellElement_t* spellElement2 = (spellElement_t*)node->element;
		cost += getCostOfSpellElement(spellElement2);
	}

	return cost;
}

bool spell_isChanneled(spell_t* spell)
{
	node_t* node = NULL;

	for ( node = spell->elements.first; node != NULL; node = node->next )
	{
		spellElement_t* spellElement = (spellElement_t*)node->element;
		if ( spellElement_isChanneled(spellElement) )
		{
			return true;
		}
	}

	return false;
}

real_t getSpellBonusFromCasterINT(Entity* caster, Stat* casterStats)
{
	if ( caster && caster->behavior != &actPlayer )
	{
		return 0.0;
	}

	real_t bonus = 0.0;
	if ( !casterStats && caster )
	{
		casterStats = caster->getStats();
	}
	if ( casterStats )
	{
		int INT = statGetINT(casterStats, caster);
		if ( INT > 0 )
		{
			bonus += INT / 100.0;
		}
	}
	return bonus;
}

real_t getBonusFromCasterOfSpellElement(Entity* caster, Stat* casterStats, spellElement_t* spellElement, int spellID)
{
	if ( caster && caster->behavior != &actPlayer )
	{
		return 0.0;
	}

	if ( !casterStats && caster )
	{
		casterStats = caster->getStats();
	}

	real_t bonus = 0.0;
	bonus += getSpellBonusFromCasterINT(caster, casterStats);

	if ( casterStats )
	{
		if ( casterStats->EFFECTS[EFF_PWR] )
		{
			bonus += 0.25;
			int percentMP = static_cast<int>(100.0 * (real_t)casterStats->MP / std::max(1, casterStats->MAXMP));
			percentMP = std::min(100, std::max(0, percentMP));
			percentMP = (100 - percentMP) / 10;
			bonus += 0.5 * percentMP / 10.0;
		}
		if ( casterStats->helmet )
		{
			if ( casterStats->helmet->type == HAT_MITER )
			{
				real_t hatBonus = 0.0;
				if ( casterStats->helmet->beatitude >= 0 || shouldInvertEquipmentBeatitude(casterStats) )
				{
					hatBonus += (0.10 + (0.05 * abs(casterStats->helmet->beatitude)));
				}
				else
				{
					hatBonus -= ((0.10 * abs(casterStats->helmet->beatitude)));
				}
				if ( spellID == SPELL_HEALING || spellID == SPELL_EXTRAHEALING )
				{
					hatBonus *= 2;
				}
				bonus += hatBonus;
			}
			else if ( casterStats->helmet->type == HAT_HEADDRESS )
			{
				real_t hatBonus = 0.0;
				if ( casterStats->helmet->beatitude >= 0 || shouldInvertEquipmentBeatitude(casterStats) )
				{
					hatBonus += (0.10 + (0.05 * abs(casterStats->helmet->beatitude)));
				}
				else
				{
					hatBonus -= ((0.10 * abs(casterStats->helmet->beatitude)));
				}
				if ( ItemTooltips.spellItems.find(spellID) != ItemTooltips.spellItems.end() )
				{
					auto& entry = ItemTooltips.spellItems[spellID];
					if ( entry.spellTags.find(ItemTooltips_t::SpellTagTypes::SPELL_TAG_DAMAGE) != entry.spellTags.end() )
					{
						hatBonus *= 2;
					}
				}
				bonus += hatBonus;
			}
			else if ( casterStats->helmet->type == HAT_CIRCLET
				|| casterStats->helmet->type == HAT_CIRCLET_WISDOM )
			{
				if ( casterStats->helmet->beatitude >= 0 || shouldInvertEquipmentBeatitude(casterStats) )
				{
					bonus += (0.05 + (0.05 * abs(casterStats->helmet->beatitude)));
				}
				else
				{
					bonus -= ((0.05 * abs(casterStats->helmet->beatitude)));
				}
			}
		}
	}
	
	return bonus;
}

bool spellElement_isChanneled(spellElement_t* spellElement)
{
	node_t* node = NULL;

	if ( spellElement->channeled )
	{
		return true;
	}
	for ( node = spellElement->elements.first; node != NULL; node = node->next )
	{
		spellElement_t* tempElement = (spellElement_t*)node->element;
		if ( spellElement_isChanneled(tempElement) )
		{
			return true;
		}
	}

	return false;
}

void equipSpell(spell_t* spell, int playernum, Item* spellItem)
{
	if ( players[playernum]->isLocalPlayer() )
	{
		players[playernum]->magic.equipSpell(spell);
		messagePlayer(playernum, MESSAGE_MISC, Language::get(442), spell->getSpellName());
		players[playernum]->magic.selected_spell_last_appearance = spellItem->appearance; // to keep track of shapeshift/normal spells.
	}
}

const char* spell_t::getSpellName()
{
	if ( ItemTooltips.spellItems.find(ID) != ItemTooltips.spellItems.end() )
	{
		return ItemTooltips.spellItems[ID].name.c_str();
	}
	return "";
}

spell_t* getSpellFromID(int ID)
{
	spell_t* spell = nullptr;

	switch (ID)
	{
		case SPELL_FORCEBOLT:
			spell = &spell_forcebolt;
			break;
		case SPELL_MAGICMISSILE:
			spell = &spell_magicmissile;
			break;
		case SPELL_COLD:
			spell = &spell_cold;
			break;
		case SPELL_FIREBALL:
			spell = &spell_fireball;
			break;
		case SPELL_LIGHTNING:
			spell = &spell_lightning;
			break;
		case SPELL_REMOVECURSE:
			spell = &spell_removecurse;
			break;
		case SPELL_LIGHT:
			spell = &spell_light;
			break;
		case SPELL_IDENTIFY:
			spell = &spell_identify;
			break;
		case SPELL_MAGICMAPPING:
			spell = &spell_magicmapping;
			break;
		case SPELL_SLEEP:
			spell = &spell_sleep;
			break;
		case SPELL_CONFUSE:
			spell = &spell_confuse;
			break;
		case SPELL_SLOW:
			spell = &spell_slow;
			break;
		case SPELL_OPENING:
			spell = &spell_opening;
			break;
		case SPELL_LOCKING:
			spell = &spell_locking;
			break;
		case SPELL_LEVITATION:
			spell = &spell_levitation;
			break;
		case SPELL_INVISIBILITY:
			spell = &spell_invisibility;
			break;
		case SPELL_TELEPORTATION:
			spell = &spell_teleportation;
			break;
		case SPELL_HEALING:
			spell = &spell_healing;
			break;
		case SPELL_EXTRAHEALING:
			spell = &spell_extrahealing;
			break;
		/*case SPELL_RESTOREABILITY:
			spell = &spell_restoreability;
			break;*/
		case SPELL_CUREAILMENT:
			spell = &spell_cureailment;
			break;
		case SPELL_DIG:
			spell = &spell_dig;
			break;
		case SPELL_SUMMON:
			spell = &spell_summon;
			break;
		case SPELL_STONEBLOOD:
			spell = &spell_stoneblood;
			break;
		case SPELL_BLEED:
			spell = &spell_bleed;
			break;
		case SPELL_DOMINATE:
			spell = &spell_dominate;
			break;
		case SPELL_REFLECT_MAGIC:
			spell = &spell_reflectMagic;
			break;
		case SPELL_ACID_SPRAY:
			spell = &spell_acidSpray;
			break;
		case SPELL_STEAL_WEAPON:
			spell = &spell_stealWeapon;
			break;
		case SPELL_DRAIN_SOUL:
			spell = &spell_drainSoul;
			break;
		case SPELL_VAMPIRIC_AURA:
			spell = &spell_vampiricAura;
			break;
		case SPELL_CHARM_MONSTER:
			spell = &spell_charmMonster;
			break;
		case SPELL_REVERT_FORM:
			spell = &spell_revertForm;
			break;
		case SPELL_RAT_FORM:
			spell = &spell_ratForm;
			break;
		case SPELL_SPIDER_FORM:
			spell = &spell_spiderForm;
			break;
		case SPELL_TROLL_FORM:
			spell = &spell_trollForm;
			break;
		case SPELL_IMP_FORM:
			spell = &spell_impForm;
			break;
		case SPELL_SPRAY_WEB:
			spell = &spell_sprayWeb;
			break;
		case SPELL_POISON:
			spell = &spell_poison;
			break;
		case SPELL_SPEED:
			spell = &spell_speed;
			break;
		case SPELL_FEAR:
			spell = &spell_fear;
			break;
		case SPELL_STRIKE:
			spell = &spell_strike;
			break;
		case SPELL_DETECT_FOOD:
			spell = &spell_detectFood;
			break;
		case SPELL_WEAKNESS:
			spell = &spell_weakness;
			break;
		case SPELL_AMPLIFY_MAGIC:
			spell = &spell_amplifyMagic;
			break;
		case SPELL_SHADOW_TAG:
			spell = &spell_shadowTag;
			break;
		case SPELL_TELEPULL:
			spell = &spell_telePull;
			break;
		case SPELL_DEMON_ILLUSION:
			spell = &spell_demonIllusion;
			break;
		case SPELL_TROLLS_BLOOD:
			spell = &spell_trollsBlood;
			break;
		case SPELL_SALVAGE:
			spell = &spell_salvageItem;
			break;
		case SPELL_FLUTTER:
			spell = &spell_flutter;
			break;
		case SPELL_DASH:
			spell = &spell_dash;
			break;
		case SPELL_SELF_POLYMORPH:
			spell = &spell_polymorph;
			break;
		case SPELL_CRAB_FORM:
		    spell = &spell_spiderForm;
		    break;
		case SPELL_CRAB_WEB:
			spell = &spell_sprayWeb;
			break;
		case SPELL_GHOST_BOLT:
			spell = &spell_ghost_bolt;
			break;
		case SPELL_SLIME_ACID:
			spell = &spell_slime_acid;
			break;
		case SPELL_SLIME_WATER:
			spell = &spell_slime_water;
			break;
		case SPELL_SLIME_FIRE:
			spell = &spell_slime_fire;
			break;
		case SPELL_SLIME_TAR:
			spell = &spell_slime_tar;
			break;
		case SPELL_SLIME_METAL:
			spell = &spell_slime_metal;
			break;
		default:
			break;
	}

	return spell;
}

int getSpellbookFromSpellID(int spellID)
{
	ItemType itemType = WOODEN_SHIELD;
	switch (spellID)
	{
		case SPELL_FORCEBOLT:
			itemType = SPELLBOOK_FORCEBOLT;
			break;
		case SPELL_MAGICMISSILE:
			itemType = SPELLBOOK_MAGICMISSILE;
			break;
		case SPELL_COLD:
			itemType = SPELLBOOK_COLD;
			break;
		case SPELL_FIREBALL:
			itemType = SPELLBOOK_FIREBALL;
			break;
		case SPELL_LIGHTNING:
			itemType = SPELLBOOK_LIGHTNING;
			break;
		case SPELL_REMOVECURSE:
			itemType = SPELLBOOK_REMOVECURSE;
			break;
		case SPELL_LIGHT:
			itemType = SPELLBOOK_LIGHT;
			break;
		case SPELL_IDENTIFY:
			itemType = SPELLBOOK_IDENTIFY;
			break;
		case SPELL_MAGICMAPPING:
			itemType = SPELLBOOK_MAGICMAPPING;
			break;
		case SPELL_SLEEP:
			itemType = SPELLBOOK_SLEEP;
			break;
		case SPELL_CONFUSE:
			itemType = SPELLBOOK_CONFUSE;
			break;
		case SPELL_SLOW:
			itemType = SPELLBOOK_SLOW;
			break;
		case SPELL_OPENING:
			itemType = SPELLBOOK_OPENING;
			break;
		case SPELL_LOCKING:
			itemType = SPELLBOOK_LOCKING;
			break;
		case SPELL_LEVITATION:
			itemType = SPELLBOOK_LEVITATION;
			break;
		case SPELL_INVISIBILITY:
			itemType = SPELLBOOK_INVISIBILITY;
			break;
		case SPELL_TELEPORTATION:
			itemType = SPELLBOOK_TELEPORTATION;
			break;
		case SPELL_HEALING:
			itemType = SPELLBOOK_HEALING;
			break;
		case SPELL_EXTRAHEALING:
			itemType = SPELLBOOK_EXTRAHEALING;
			break;
		case SPELL_CUREAILMENT:
			itemType = SPELLBOOK_CUREAILMENT;
			break;
		case SPELL_DIG:
			itemType = SPELLBOOK_DIG;
			break;
		case SPELL_SUMMON:
			itemType = SPELLBOOK_SUMMON;
			break;
		case SPELL_STONEBLOOD:
			itemType = SPELLBOOK_STONEBLOOD;
			break;
		case SPELL_BLEED:
			itemType = SPELLBOOK_BLEED;
			break;
		/*case SPELL_DOMINATE:
			itemType = SPELLBOOK_DOMINATE;
			break;*/
		case SPELL_REFLECT_MAGIC:
			itemType = SPELLBOOK_REFLECT_MAGIC;
			break;
		case SPELL_ACID_SPRAY:
			itemType = SPELLBOOK_ACID_SPRAY;
			break;
		case SPELL_STEAL_WEAPON:
			itemType = SPELLBOOK_STEAL_WEAPON;
			break;
		case SPELL_DRAIN_SOUL:
			itemType = SPELLBOOK_DRAIN_SOUL;
			break;
		case SPELL_VAMPIRIC_AURA:
			itemType = SPELLBOOK_VAMPIRIC_AURA;
			break;
		case SPELL_CHARM_MONSTER:
			itemType = SPELLBOOK_CHARM_MONSTER;
			break;
		case SPELL_REVERT_FORM:
			itemType = SPELLBOOK_REVERT_FORM;
			break;
		case SPELL_RAT_FORM:
			itemType = SPELLBOOK_RAT_FORM;
			break;
		case SPELL_SPIDER_FORM:
			itemType = SPELLBOOK_SPIDER_FORM;
			break;
		case SPELL_TROLL_FORM:
			itemType = SPELLBOOK_TROLL_FORM;
			break;
		case SPELL_IMP_FORM:
			itemType = SPELLBOOK_IMP_FORM;
			break;
		case SPELL_SPRAY_WEB:
			itemType = SPELLBOOK_SPRAY_WEB;
			break;
		case SPELL_POISON:
			itemType = SPELLBOOK_POISON;
			break;
		case SPELL_SPEED:
			itemType = SPELLBOOK_SPEED;
			break;
		case SPELL_FEAR:
			itemType = SPELLBOOK_FEAR;
			break;
		case SPELL_STRIKE:
			itemType = SPELLBOOK_STRIKE;
			break;
		case SPELL_DETECT_FOOD:
			itemType = SPELLBOOK_DETECT_FOOD;
			break;
		case SPELL_WEAKNESS:
			itemType = SPELLBOOK_WEAKNESS;
			break;
		case SPELL_AMPLIFY_MAGIC:
			itemType = SPELLBOOK_AMPLIFY_MAGIC;
			break;
		case SPELL_SHADOW_TAG:
			itemType = SPELLBOOK_SHADOW_TAG;
			break;
		case SPELL_TELEPULL:
			itemType = SPELLBOOK_TELEPULL;
			break;
		case SPELL_DEMON_ILLUSION:
			itemType = SPELLBOOK_DEMON_ILLU;
			break;
		case SPELL_TROLLS_BLOOD:
			itemType = SPELLBOOK_TROLLS_BLOOD;
			break;
		case SPELL_SALVAGE:
			itemType = SPELLBOOK_SALVAGE;
			break;
		case SPELL_FLUTTER:
			itemType = SPELLBOOK_FLUTTER;
			break;
		case SPELL_DASH:
			itemType = SPELLBOOK_DASH;
			break;
		case SPELL_SELF_POLYMORPH:
			itemType = SPELLBOOK_SELF_POLYMORPH;
			break;
		case SPELL_CRAB_FORM:
			itemType = SPELLBOOK_9;
			break;
		case SPELL_CRAB_WEB:
			itemType = SPELLBOOK_10;
			break;
		case SPELL_GHOST_BOLT:
			itemType = SPELLBOOK_9;
			break;
		default:
			break;
	}

	return itemType;
}

int getSpellIDFromSpellbook(int spellbookType)
{
	switch (spellbookType )
	{
		case SPELLBOOK_FORCEBOLT:
			return spell_forcebolt.ID;
		case SPELLBOOK_MAGICMISSILE:
			return spell_magicmissile.ID;
		case SPELLBOOK_COLD:
			return spell_cold.ID;
		case SPELLBOOK_FIREBALL:
			return spell_fireball.ID;
		case SPELLBOOK_LIGHTNING:
			return spell_lightning.ID;
		case SPELLBOOK_REMOVECURSE:
			return spell_removecurse.ID;
		case SPELLBOOK_LIGHT:
			return spell_light.ID;
		case SPELLBOOK_IDENTIFY:
			return spell_identify.ID;
		case SPELLBOOK_MAGICMAPPING:
			return spell_magicmapping.ID;
		case SPELLBOOK_SLEEP:
			return spell_sleep.ID;
		case SPELLBOOK_CONFUSE:
			return spell_confuse.ID;
		case SPELLBOOK_SLOW:
			return spell_slow.ID;
		case SPELLBOOK_OPENING:
			return spell_opening.ID;
		case SPELLBOOK_LOCKING:
			return spell_locking.ID;
		case SPELLBOOK_LEVITATION:
			return spell_levitation.ID;
		case SPELLBOOK_INVISIBILITY:
			return spell_invisibility.ID;
		case SPELLBOOK_TELEPORTATION:
			return spell_teleportation.ID;
		case SPELLBOOK_HEALING:
			return spell_healing.ID;
		case SPELLBOOK_EXTRAHEALING:
			return spell_extrahealing.ID;
		case SPELLBOOK_CUREAILMENT:
			return spell_cureailment.ID;
		case SPELLBOOK_DIG:
			return spell_dig.ID;
		case SPELLBOOK_SUMMON:
			return spell_summon.ID;
		case SPELLBOOK_STONEBLOOD:
			return spell_stoneblood.ID;
		case SPELLBOOK_BLEED:
			return spell_bleed.ID;
		case SPELLBOOK_REFLECT_MAGIC:
			return spell_reflectMagic.ID;
		case SPELLBOOK_ACID_SPRAY:
			return spell_acidSpray.ID;
		case SPELLBOOK_STEAL_WEAPON:
			return spell_stealWeapon.ID;
		case SPELLBOOK_DRAIN_SOUL:
			return spell_drainSoul.ID;
		case SPELLBOOK_VAMPIRIC_AURA:
			return spell_vampiricAura.ID;
		case SPELLBOOK_CHARM_MONSTER:
			return spell_charmMonster.ID;
		case SPELLBOOK_REVERT_FORM:
			return spell_revertForm.ID;
		case SPELLBOOK_RAT_FORM:
			return spell_ratForm.ID;
		case SPELLBOOK_SPIDER_FORM:
			return spell_spiderForm.ID;
		case SPELLBOOK_TROLL_FORM:
			return spell_trollForm.ID;
		case SPELLBOOK_IMP_FORM:
			return spell_impForm.ID;
		case SPELLBOOK_SPRAY_WEB:
			return spell_sprayWeb.ID;
		case SPELLBOOK_POISON:
			return spell_poison.ID;
		case SPELLBOOK_SPEED:
			return spell_speed.ID;
		case SPELLBOOK_FEAR:
			return spell_fear.ID;
		case SPELLBOOK_STRIKE:
			return spell_strike.ID;
		case SPELLBOOK_DETECT_FOOD:
			return spell_detectFood.ID;
		case SPELLBOOK_WEAKNESS:
			return spell_weakness.ID;
		case SPELLBOOK_AMPLIFY_MAGIC:
			return spell_amplifyMagic.ID;
		case SPELLBOOK_SHADOW_TAG:
			return spell_shadowTag.ID;
		case SPELLBOOK_TELEPULL:
			return spell_telePull.ID;
		case SPELLBOOK_DEMON_ILLU:
			return spell_demonIllusion.ID;
		case SPELLBOOK_TROLLS_BLOOD:
			return spell_trollsBlood.ID;
		case SPELLBOOK_SALVAGE:
			return spell_salvageItem.ID;
		case SPELLBOOK_FLUTTER:
			return spell_flutter.ID;
		case SPELLBOOK_DASH:
			return spell_dash.ID;
		case SPELLBOOK_SELF_POLYMORPH:
			return spell_polymorph.ID;
		case SPELLBOOK_9:
			return spell_spiderForm.ID;
		case SPELLBOOK_10:
			return spell_sprayWeb.ID;
		default:
			return SPELL_NONE;
	}
}

bool spellInList(list_t* list, spell_t* spell)
{
	if ( !spell )
	{
		return false;
	}
	node_t* node;
	for ( node = list->first; node != NULL; node = node->next )
	{
		spell_t* current = (spell_t*)node->element;
		if (current)
		{
			if (current->ID == spell->ID)
			{
				return true;
			}
		}
	}

	return false;
}

void spell_changeHealth(Entity* entity, int amount, bool overdrewFromHP)
{
	if (!entity)
	{
		return;
	}

	entity->modHP(amount);

	int player = -1;
	int i = 0;
	for (i = 0; i < MAXPLAYERS; ++i)
	{
		if (entity == players[i]->entity)
		{
			player = i;
		}
	}

	if (player > -1 && player < MAXPLAYERS)
	{
		if (amount > 0)
		{
			if ( overdrewFromHP )
			{
				Uint32 color = makeColorRGB(255, 255, 255);
				messagePlayerColor(player, MESSAGE_STATUS, color, Language::get(3400));
			}
			else
			{
				Uint32 color = makeColorRGB(0, 255, 0);
				messagePlayerColor(player, MESSAGE_STATUS, color, Language::get(443));
			}
		}
		else
		{
			Uint32 color = makeColorRGB(255, 255, 0);
			if (amount == 0)
			{
				messagePlayerColor(player, MESSAGE_COMBAT, color, Language::get(444));
			}
			else
			{
				messagePlayerColor(player, MESSAGE_COMBAT, color, Language::get(445));
			}
		}

		if ( multiplayer == SERVER && player > 0 && !players[player]->isLocalPlayer() )
		{
			strcpy((char*)net_packet->data, "UPHP");
			SDLNet_Write32((Uint32)stats[player]->HP, &net_packet->data[4]);
			SDLNet_Write32(0, &net_packet->data[8]);
			net_packet->address.host = net_clients[player - 1].host;
			net_packet->address.port = net_clients[player - 1].port;
			net_packet->len = 12;
			sendPacketSafe(net_sock, net_packet->channel, net_packet, player - 1);
		}
	}
}

spell_t* getSpellFromItem(const int player, Item* item, bool usePlayerInventory)
{
	spell_t* spell = nullptr;
	node_t* node = nullptr;
	if ( !item )
	{
		return nullptr;
	}
	if ( item->type != SPELL_ITEM )
	{
		return nullptr;
	}

	Uint32 appearance = item->appearance;
	if ( item->type == SPELL_ITEM && item->appearance >= 1000 )
	{
		appearance -= 1000; // hack for normally uncontrollable spells.
	}
	if ( usePlayerInventory )
	{
		if ( player < 0 || player >= MAXPLAYERS )
		{
			return nullptr;
		}
		for ( node = players[player]->magic.spellList.first; node; node = node->next )
		{
			if ( node->element )
			{
				spell = (spell_t*) node->element;
				if ( spell->ID == appearance )
				{
					return spell;    //Found the spell.
				}
			}
		}
	}
	else
	{
		for ( auto spell : allGameSpells )
		{
			if ( spell->ID == appearance )
			{
				return spell;    //Found the spell.
			}
		}
	}
	return nullptr;
}

int canUseShapeshiftSpellInCurrentForm(const int player, Item& item)
{
	if ( itemCategory(&item) != SPELL_CAT )
	{
		return -1;
	}
	if ( item.appearance < 1000 )
	{
		return -1;
	}
	if ( !stats[player] )
	{
		return -1;
	}
	spell_t* spell = getSpellFromItem(player, &item, false);
	if ( !spell )
	{
		return -1;
	}
	if ( !stats[player]->EFFECTS[EFF_SHAPESHIFT] )
	{
		return 0;
	}
	switch ( spell->ID )
	{
		case SPELL_SPEED:
		case SPELL_DETECT_FOOD:
			if ( stats[player]->type == RAT )
			{
				return 1;
			}
			break;
		case SPELL_POISON:
		case SPELL_SPRAY_WEB:
			if ( stats[player]->type == SPIDER )
			{
				return 1;
			}
			break;
		case SPELL_STRIKE:
		case SPELL_FEAR:
		case SPELL_TROLLS_BLOOD:
			if ( stats[player]->type == TROLL )
			{
				return 1;
			}
			break;
		case SPELL_LIGHTNING:
		case SPELL_CONFUSE:
		case SPELL_AMPLIFY_MAGIC:
			if ( stats[player]->type == CREATURE_IMP )
			{
				return 1;
			}
			break;
		default:
			break;
	}
	return 0;
}
