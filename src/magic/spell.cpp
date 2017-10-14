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
#include "../sound.hpp"
#include "../items.hpp"
#include "../net.hpp"
#include "../player.hpp"
#include "magic.hpp"

list_t spellList;
list_t channeledSpells[4];
spell_t* selected_spell = NULL;

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

void addSpell(int spell, int player, bool ignoreSkill)
{
	node_t* node = NULL;

	// this is a local function
	if ( player != clientnum )
	{
		return;
	}

	spell_t* new_spell = NULL;

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
		default:
			return;
	}
	if ( spellInList(&spellList, new_spell) )
	{
		messagePlayer(player, language[439], new_spell->name);
		spellDeconstructor((void*)new_spell);
		return;
	}
	if ( !ignoreSkill && stats[player]->PROFICIENCIES[PRO_MAGIC] + statGetINT(stats[player]) < new_spell->difficulty )
	{
		messagePlayer(player, language[440]);
		spellDeconstructor((void*)new_spell);
		return;
	}
	messagePlayer(player, language[441], new_spell->name);
	node = list_AddNodeLast(&spellList);
	node->element = new_spell;
	node->size = sizeof(spell_t);
	node->deconstructor = &spellDeconstructor;

	players[player]->entity->increaseSkill(PRO_MAGIC);

	Item* item = newItem(SPELL_ITEM, SERVICABLE, 0, 1, spell, true, NULL);
	itemPickup(player, item);
	free(item);
}

spell_t* newSpell()
{
	spell_t* spell = (spell_t*) malloc(sizeof(spell_t));
	spellConstructor(spell);
	return spell;
}

void spellConstructor(spell_t* spell)
{
	spell->ID = -1;
	strcpy(spell->name, "Spell");
	spell->elements.first = NULL;
	spell->elements.last = NULL;
	spell->sustain = true;
	spell->magicstaff = false;
	spell->sustain_node = NULL;
	spell->magic_effects_node = NULL;
	spell->caster = -1;
	spell->channel_duration = 0;
	//spell->timer = 0;
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
	strcpy(element->name, "New Element");
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

int getCostOfSpell(spell_t* spell)
{
	int cost = 0;

	node_t* node;
	for ( node = spell->elements.first; node != NULL; node = node->next )
	{
		spellElement_t* spellElement = (spellElement_t*)node->element;
		cost += getCostOfSpellElement(spellElement);
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

void equipSpell(spell_t* spell, int playernum)
{
	if ( playernum == clientnum )
	{
		selected_spell = spell;
		messagePlayer(playernum, language[442], spell->name);
	}
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
		default:
			break;
	}

	return spell;
}

Item* getSpellbookFromSpellID(int spellID)
{
	Item *spellbook = nullptr;

	int itemType = -1;
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
		default:
			break;
	}

	if ( itemType > 0 )
	{
		spellbook = newItem(static_cast<ItemType>(itemType), static_cast<Status>(0), 0, 1, rand(), true, nullptr);
	}

	return spellbook;
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
		default:
			return SPELL_NONE;
	}
}

bool spellInList(list_t* list, spell_t* spell)
{
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

void spell_changeHealth(Entity* entity, int amount)
{
	if (!entity)
	{
		return;
	}

	entity->modHP(amount);

	int player = -1;
	int i = 0;
	for (i = 0; i < 4; ++i)
	{
		if (entity == players[i]->entity)
		{
			player = i;
		}
	}

	if (player > -1 && player <= 4)
	{
		if (amount > 0)
		{
			Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
			messagePlayerColor(player, color, language[443]);
		}
		else
		{
			Uint32 color = SDL_MapRGB(mainsurface->format, 255, 255, 0);
			if (amount == 0)
			{
				messagePlayerColor(player, color, language[444]);
			}
			else
			{
				messagePlayerColor(player, color, language[445]);
			}
		}

		if (multiplayer == SERVER)
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

spell_t* getSpellFromItem(Item* item)
{
	spell_t* spell = NULL;
	node_t* node = NULL;
	for (node = spellList.first; node; node = node->next)
	{
		if (node->element)
		{
			spell = (spell_t*) node->element;
			if (spell->ID == item->appearance)
			{
				return spell;    //Found the spell.
			}
		}
	}

	return NULL;
}
