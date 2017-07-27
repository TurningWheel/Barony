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
#include "magic.hpp"

void setupSpells()   ///TODO: Verify this function.
{
	node_t* node = NULL;
	spellElement_t* element = NULL;

	spellElementConstructor(&spellElement_unintelligible);
	spellElement_unintelligible.mana = 0;
	spellElement_unintelligible.base_mana = 0;
	spellElement_unintelligible.overload_multiplier = 0; //NOTE: Might crash due to divide by zero?
	spellElement_unintelligible.damage = 0;
	spellElement_unintelligible.duration = 0;
	spellElement_unintelligible.can_be_learned = false;
	strcpy(spellElement_unintelligible.name, language[413]);

	spellElementConstructor(&spellElement_missile);
	spellElement_missile.mana = 1;
	spellElement_missile.base_mana = 1;
	spellElement_missile.overload_multiplier = 1;
	spellElement_missile.damage = 0;
	spellElement_missile.duration = 75; //1.25 seconds.
	//spellElement_missile.name = "Missile";
	strcpy(spellElement_missile.name, language[414]);

	spellElementConstructor(&spellElement_force);
	spellElement_force.mana = 4;
	spellElement_force.base_mana = 4;
	spellElement_force.overload_multiplier = 1;
	spellElement_force.damage = 15;
	spellElement_force.duration = 0;
	strcpy(spellElement_force.name, language[415]);

	spellElementConstructor(&spellElement_fire);
	spellElement_fire.mana = 6;
	spellElement_fire.base_mana = 6;
	spellElement_fire.overload_multiplier = 1;
	spellElement_fire.damage = 25;
	spellElement_fire.duration = 0;
	strcpy(spellElement_fire.name, language[416]);

	spellElementConstructor(&spellElement_lightning);
	spellElement_lightning.mana = 5;
	spellElement_lightning.base_mana = 5;
	spellElement_lightning.overload_multiplier = 1;
	spellElement_lightning.damage = 25;
	spellElement_lightning.duration = 75;
	strcpy(spellElement_lightning.name, language[417]);

	spellElementConstructor(&spellElement_light);
	spellElement_light.mana = 1;
	spellElement_light.base_mana = 1;
	spellElement_light.overload_multiplier = 1;
	spellElement_light.damage = 0;
	spellElement_light.duration = 750; //500 a better value? //NOTE: 750 is original value.
	strcpy(spellElement_light.name, language[418]);

	spellElementConstructor(&spellElement_dig);
	spellElement_dig.mana = 20;
	spellElement_dig.base_mana = 20;
	spellElement_dig.overload_multiplier = 1;
	spellElement_dig.damage = 0;
	spellElement_dig.duration = 0;
	strcpy(spellElement_dig.name, language[419]);

	spellElementConstructor(&spellElement_invisible);
	spellElement_invisible.mana = 2;
	spellElement_invisible.base_mana = 2;
	spellElement_invisible.overload_multiplier = 1;
	spellElement_invisible.damage = 0;
	spellElement_invisible.duration = 100;
	strcpy(spellElement_invisible.name, language[420]);

	spellElementConstructor(&spellElement_identify);
	spellElement_identify.mana = 10;
	spellElement_identify.base_mana = 10;
	spellElement_identify.overload_multiplier = 0; //NOTE: Might segfault due to divide by zero?
	spellElement_identify.damage = 0;
	spellElement_identify.duration = 0;
	strcpy(spellElement_identify.name, language[421]);

	spellElementConstructor(&spellElement_magicmapping);
	spellElement_magicmapping.mana = 40;
	spellElement_magicmapping.base_mana = 40;
	spellElement_magicmapping.overload_multiplier = 0; //NOTE: Might segfault due to divide by zero?
	spellElement_magicmapping.damage = 0;
	spellElement_magicmapping.duration = 0;
	strcpy(spellElement_magicmapping.name, language[422]);

	spellElementConstructor(&spellElement_heal);
	spellElement_heal.mana = 1;
	spellElement_heal.base_mana = 1;
	spellElement_heal.overload_multiplier = 1;
	spellElement_heal.damage = 1;
	spellElement_heal.duration = 0;
	strcpy(spellElement_heal.name, language[423]);

	spellElementConstructor(&spellElement_confuse);
	spellElement_confuse.mana = 4;
	spellElement_confuse.base_mana = 4;
	spellElement_confuse.overload_multiplier = 1;
	spellElement_confuse.damage = 0;
	spellElement_confuse.duration = TICKS_PER_SECOND * SPELLELEMENT_CONFUSE_BASE_DURATION; //TODO: Decide on something.
	strcpy(spellElement_confuse.name, language[424]);

	spellElementConstructor(&spellElement_cure_ailment);
	spellElement_cure_ailment.mana = 10;
	spellElement_cure_ailment.base_mana = 10;
	spellElement_cure_ailment.overload_multiplier = 0;
	spellElement_cure_ailment.damage = 0;
	spellElement_cure_ailment.duration = 0;
	strcpy(spellElement_cure_ailment.name, language[425]);

	spellElementConstructor(&spellElement_locking);
	spellElement_locking.mana = 10;
	spellElement_locking.base_mana = 10;
	spellElement_locking.overload_multiplier = 0;
	spellElement_locking.damage = 0;
	spellElement_locking.duration = 0;
	strcpy(spellElement_locking.name, language[426]);

	spellElementConstructor(&spellElement_opening);
	spellElement_opening.mana = 5;
	spellElement_opening.base_mana = 5;
	spellElement_opening.overload_multiplier = 0;
	spellElement_opening.damage = 0;
	spellElement_opening.duration = 0;
	strcpy(spellElement_opening.name, language[427]);

	spellElementConstructor(&spellElement_sleep);
	spellElement_sleep.mana = 3;
	spellElement_sleep.base_mana = 3;
	spellElement_sleep.overload_multiplier = 0;
	spellElement_sleep.damage = 0;
	spellElement_sleep.duration = 0;
	strcpy(spellElement_opening.name, language[428]);

	spellElementConstructor(&spellElement_cold);
	spellElement_cold.mana = 5;
	spellElement_cold.base_mana = 5;
	spellElement_cold.overload_multiplier = 1;
	spellElement_cold.damage = 20;
	spellElement_cold.duration = 180;
	strcpy(spellElement_cold.name, language[429]);

	spellElementConstructor(&spellElement_slow);
	spellElement_slow.mana = 3;
	spellElement_slow.base_mana = 3;
	spellElement_slow.overload_multiplier = 1;
	spellElement_slow.damage = 0;
	spellElement_slow.duration = 180;
	strcpy(spellElement_slow.name, language[430]);

	spellElementConstructor(&spellElement_levitation);
	spellElement_levitation.mana = 1;
	spellElement_levitation.base_mana = 1;
	spellElement_levitation.overload_multiplier = 1;
	spellElement_levitation.damage = 0;
	spellElement_levitation.duration = 30;
	strcpy(spellElement_slow.name, language[431]);

	spellElementConstructor(&spellElement_teleportation);
	spellElement_teleportation.mana = 20;
	spellElement_teleportation.base_mana = 20;
	spellElement_teleportation.overload_multiplier = 0;
	spellElement_teleportation.damage = 0;
	spellElement_teleportation.duration = 0;
	strcpy(spellElement_teleportation.name, language[432]);

	spellElementConstructor(&spellElement_magicmissile);
	spellElement_magicmissile.mana = 6;
	spellElement_magicmissile.base_mana = 6;
	spellElement_magicmissile.overload_multiplier = 1;
	spellElement_magicmissile.damage = 30;
	spellElement_magicmissile.duration = 0;
	strcpy(spellElement_magicmissile.name, language[433]);

	spellElementConstructor(&spellElement_removecurse);
	spellElement_removecurse.mana = 20;
	spellElement_removecurse.base_mana = 20;
	spellElement_removecurse.overload_multiplier = 0;
	spellElement_removecurse.damage = 0;
	spellElement_removecurse.duration = 0;
	strcpy(spellElement_removecurse.name, language[434]);

	spellConstructor(&spell_forcebolt);
	strcpy(spell_forcebolt.name, language[415]);
	spell_forcebolt.ID = SPELL_FORCEBOLT;
	spell_forcebolt.difficulty = 0;
	node = list_AddNodeLast(&spell_forcebolt.elements);
	node->element = copySpellElement(&spellElement_missile);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node; //Tell the element what list it resides in.
	//Now for the second element.
	element->elements.first = NULL;
	element->elements.last = NULL;
	node = list_AddNodeLast(&element->elements);
	node->element = copySpellElement(&spellElement_force);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node;

	spellConstructor(&spell_magicmissile);
	strcpy(spell_magicmissile.name, language[433]);
	spell_magicmissile.ID = SPELL_MAGICMISSILE;
	spell_magicmissile.difficulty = 50;
	node = list_AddNodeLast(&spell_magicmissile.elements);
	node->element = copySpellElement(&spellElement_missile);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node; //Tell the element what list it resides in.
	//Now for the second element.
	element->elements.first = NULL;
	element->elements.last = NULL;
	node = list_AddNodeLast(&element->elements);
	node->element = copySpellElement(&spellElement_magicmissile);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node;

	spellConstructor(&spell_cold);
	strcpy(spell_cold.name, language[429]);
	spell_cold.ID = SPELL_COLD;
	spell_cold.difficulty = 50;
	node = list_AddNodeLast(&spell_cold.elements);
	node->element = copySpellElement(&spellElement_missile);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node; //Tell the element what list it resides in.
	//Now for the second element.
	element->elements.first = NULL;
	element->elements.last = NULL;
	node = list_AddNodeLast(&element->elements);
	node->element = copySpellElement(&spellElement_cold);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node;

	spellConstructor(&spell_fireball);
	strcpy(spell_fireball.name, language[416]);
	spell_fireball.ID = SPELL_FIREBALL;
	spell_fireball.difficulty = 25;
	spell_fireball.elements.first = NULL;
	spell_fireball.elements.last = NULL;
	node = list_AddNodeLast(&spell_fireball.elements);
	node->element = copySpellElement(&spellElement_missile);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node; //Tell the element what list it resides in.
	//Now for the second element.
	element->elements.first = NULL;
	element->elements.last = NULL;
	node = list_AddNodeLast(&element->elements);
	node->element = copySpellElement(&spellElement_fire);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node;

	spellConstructor(&spell_lightning);
	strcpy(spell_lightning.name, language[417]);
	spell_lightning.ID = SPELL_LIGHTNING;
	spell_lightning.difficulty = 50;
	spell_lightning.elements.first = NULL;
	spell_lightning.elements.last = NULL;
	node = list_AddNodeLast(&spell_lightning.elements);
	node->element = copySpellElement(&spellElement_missile);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node; //Tell the element what list it resides in.
	//Now for the second element.
	element->elements.first = NULL;
	element->elements.last = NULL;
	node = list_AddNodeLast(&element->elements);
	node->element = copySpellElement(&spellElement_lightning);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node;

	spellConstructor(&spell_removecurse);
	strcpy(spell_removecurse.name, language[434]);
	spell_removecurse.ID = SPELL_REMOVECURSE;
	spell_removecurse.difficulty = 50;
	spell_removecurse.elements.first = NULL;
	spell_removecurse.elements.last = NULL;
	node = list_AddNodeLast(&spell_removecurse.elements);
	node->element = copySpellElement(&spellElement_removecurse);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node; //Tell the element what list it resides in.

	spellConstructor(&spell_light);
	strcpy(spell_light.name, language[418]);
	spell_light.ID = SPELL_LIGHT;
	spell_light.difficulty = 0;
	spell_light.elements.first = NULL;
	spell_light.elements.last = NULL;
	node = list_AddNodeLast(&spell_light.elements);
	node->element = copySpellElement(&spellElement_light);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node; //Tell the element what list it resides in.
	element->channeled = true;

	spellConstructor(&spell_identify);
	strcpy(spell_identify.name, language[421]);
	spell_identify.ID = SPELL_IDENTIFY;
	spell_identify.difficulty = 50;
	spell_identify.elements.first = NULL;
	spell_identify.elements.last = NULL;
	node = list_AddNodeLast(&spell_identify.elements);
	node->element = copySpellElement(&spellElement_identify);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node; //Tell the element what list it resides in.

	spellConstructor(&spell_magicmapping);
	strcpy(spell_magicmapping.name, language[435]);
	spell_magicmapping.ID = SPELL_MAGICMAPPING;
	spell_magicmapping.difficulty = 50;
	spell_magicmapping.elements.first = NULL;
	spell_magicmapping.elements.last = NULL;
	node = list_AddNodeLast(&spell_magicmapping.elements);
	node->element = copySpellElement(&spellElement_magicmapping);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node;

	spellConstructor(&spell_sleep);
	strcpy(spell_sleep.name, language[428]);
	spell_sleep.ID = SPELL_SLEEP;
	spell_sleep.difficulty = 25;
	node = list_AddNodeLast(&spell_sleep.elements);
	node->element = copySpellElement(&spellElement_missile);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node; //Tell the element what list it resides in.
	//Now for the second element.
	element->elements.first = NULL;
	element->elements.last = NULL;
	node = list_AddNodeLast(&element->elements);
	node->element = copySpellElement(&spellElement_sleep);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node;

	spellConstructor(&spell_confuse);
	strcpy(spell_confuse.name, language[424]);
	spell_confuse.ID = SPELL_CONFUSE;
	spell_confuse.difficulty = 25;
	node = list_AddNodeLast(&spell_confuse.elements);
	node->element = copySpellElement(&spellElement_missile);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node; //Tell the element what list it resides in.
	//Now for the second element.
	element->elements.first = NULL;
	element->elements.last = NULL;
	node = list_AddNodeLast(&element->elements);
	node->element = copySpellElement(&spellElement_confuse);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node;
	element->mana = 15; //Set the spell's mana to 15 so that it lasts ~30 seconds.

	spellConstructor(&spell_slow);
	strcpy(spell_slow.name, language[430]);
	spell_slow.ID = SPELL_SLOW;
	spell_slow.difficulty = 25;
	node = list_AddNodeLast(&spell_slow.elements);
	node->element = copySpellElement(&spellElement_missile);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node; //Tell the element what list it resides in.
	//Now for the second element.
	element->elements.first = NULL;
	element->elements.last = NULL;
	node = list_AddNodeLast(&element->elements);
	node->element = copySpellElement(&spellElement_slow);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node;

	spellConstructor(&spell_opening);
	strcpy(spell_opening.name, language[427]);
	spell_opening.ID = SPELL_OPENING;
	spell_opening.difficulty = 25;
	node = list_AddNodeLast(&spell_opening.elements);
	node->element = copySpellElement(&spellElement_missile);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node; //Tell the element what list it resides in.
	//Now for the second element.
	element->elements.first = NULL;
	element->elements.last = NULL;
	node = list_AddNodeLast(&element->elements);
	node->element = copySpellElement(&spellElement_opening);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node;

	spellConstructor(&spell_locking);
	strcpy(spell_locking.name, language[426]);
	spell_locking.ID = SPELL_LOCKING;
	spell_locking.difficulty = 25;
	node = list_AddNodeLast(&spell_locking.elements);
	node->element = copySpellElement(&spellElement_missile);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node; //Tell the element what list it resides in.
	//Now for the second element.
	element->elements.first = NULL;
	element->elements.last = NULL;
	node = list_AddNodeLast(&element->elements);
	node->element = copySpellElement(&spellElement_locking);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node;

	spellConstructor(&spell_levitation);
	strcpy(spell_levitation.name, language[431]);
	spell_levitation.ID = SPELL_LEVITATION;
	spell_levitation.difficulty = 75;
	spell_levitation.elements.first = NULL;
	spell_levitation.elements.last = NULL;
	node = list_AddNodeLast(&spell_levitation.elements);
	node->element = copySpellElement(&spellElement_levitation);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node; //Tell the element what list it resides in.
	element->channeled = true;

	spellConstructor(&spell_invisibility);
	strcpy(spell_invisibility.name, language[420]);
	spell_invisibility.ID = SPELL_INVISIBILITY;
	spell_invisibility.difficulty = 75;
	spell_invisibility.elements.first = NULL;
	spell_invisibility.elements.last = NULL;
	node = list_AddNodeLast(&spell_invisibility.elements);
	node->element = copySpellElement(&spellElement_invisible);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node; //Tell the element what list it resides in.
	element->channeled = true;

	spellConstructor(&spell_teleportation);
	strcpy(spell_teleportation.name, language[432]);
	spell_teleportation.ID = SPELL_TELEPORTATION;
	spell_teleportation.difficulty = 75;
	spell_teleportation.elements.first = NULL;
	spell_teleportation.elements.last = NULL;
	node = list_AddNodeLast(&spell_teleportation.elements);
	node->element = copySpellElement(&spellElement_teleportation);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node; //Tell the element what list it resides in.

	spellConstructor(&spell_healing);
	strcpy(spell_healing.name, language[423]);
	spell_healing.ID = SPELL_HEALING;
	spell_healing.difficulty = 25;
	spell_healing.elements.first = NULL;
	spell_healing.elements.last = NULL;
	node = list_AddNodeLast(&spell_healing.elements);
	node->element = copySpellElement(&spellElement_heal);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node;
	element->mana = 10;

	spellConstructor(&spell_extrahealing);
	strcpy(spell_extrahealing.name, language[438]);
	spell_extrahealing.ID = SPELL_EXTRAHEALING;
	spell_extrahealing.difficulty = 50;
	spell_extrahealing.elements.first = NULL;
	spell_extrahealing.elements.last = NULL;
	node = list_AddNodeLast(&spell_extrahealing.elements);
	node->element = copySpellElement(&spellElement_heal);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node;
	element->mana = 40;

	/*spellConstructor(&spell_restoreability);
	//spell_restoreability.mana = 1;
	strcpy(spell_restoreability.name, "Restore Ability");
	spell_restoreability.ID = SPELL_RESTOREABILITY;
	spell_restoreability.difficulty = 100; //Basically unlearnable (since unimplemented).*/

	spellConstructor(&spell_cureailment);
	strcpy(spell_cureailment.name, language[425]);
	spell_cureailment.ID = SPELL_CUREAILMENT;
	spell_cureailment.difficulty = 25;
	spell_cureailment.elements.first = NULL;
	spell_cureailment.elements.last = NULL;
	node = list_AddNodeLast(&spell_cureailment.elements);
	node->element = copySpellElement(&spellElement_cure_ailment);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node;

	spellConstructor(&spell_dig);
	strcpy(spell_dig.name, "Dig");
	spell_dig.ID = SPELL_DIG;
	spell_dig.difficulty = 50;
	spell_dig.elements.first = NULL;
	spell_dig.elements.last = NULL;
	node = list_AddNodeLast(&spell_dig.elements);
	node->element = copySpellElement(&spellElement_missile);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node; //Tell the element what list it resides in.
	//Now for the second element.
	element->elements.first = NULL;
	element->elements.last = NULL;
	node = list_AddNodeLast(&element->elements);
	node->element = copySpellElement(&spellElement_dig);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node;
}