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

std::map<int, spell_t*> allGameSpells;

spell_t* createSimpleSpell(int spellID, int difficulty, int mana, int base_mana, int overload_mult, int damage, int duration, const char* internal_name, bool sustained = false);
void setupSpells()   ///TODO: Verify this function.
{
	for ( auto it : allGameSpells )
	{
		spell_t* spell = it.second;
		if ( spell_isChanneled(spell) )
		{
			if ( spell->sustain_node )
			{
				list_RemoveNode(spell->sustain_node);
			}
		}
		list_FreeAll(&spell->elements);
	}
	allGameSpells.clear();
	spellElementMap.clear();

	node_t* node = NULL;
	spellElement_t* element = NULL;

	spellElementConstructor(&spellElement_unintelligible);
	spellElement_unintelligible.mana = 0;
	spellElement_unintelligible.base_mana = 0;
	spellElement_unintelligible.overload_multiplier = 0; //NOTE: Might crash due to divide by zero?
	spellElement_unintelligible.damage = 0;
	spellElement_unintelligible.duration = 0;
	spellElement_unintelligible.can_be_learned = false;
	strcpy(spellElement_unintelligible.element_internal_name, "spell_element_unintelligible");

	spellElementConstructor(&spellElement_missile);
	spellElement_missile.mana = 1;
	spellElement_missile.base_mana = 1;
	spellElement_missile.overload_multiplier = 1;
	spellElement_missile.damage = 0;
	spellElement_missile.duration = 75; //1.25 seconds.
	//spellElement_missile.name = "Missile";
	strcpy(spellElement_missile.element_internal_name, "spell_element_missile");
	spellElementMap[SPELL_ELEMENT_PROPULSION_MISSILE] = *copySpellElement(&spellElement_missile);
	spellElementMap[SPELL_ELEMENT_PROPULSION_MISSILE_NOCOST] = *copySpellElement(&spellElement_missile);
	spellElementMap[SPELL_ELEMENT_PROPULSION_MISSILE_NOCOST].mana = 0;
	spellElementMap[SPELL_ELEMENT_PROPULSION_MISSILE_NOCOST].base_mana = 0;

	spellElementConstructor(&spellElement_force);
	spellElement_force.mana = 4;
	spellElement_force.base_mana = 4;
	spellElement_force.overload_multiplier = 1;
	spellElement_force.damage = 15;
	spellElement_force.duration = 0;
	strcpy(spellElement_force.element_internal_name, "spell_element_forcebolt");

	spellElementConstructor(&spellElement_fire);
	spellElement_fire.mana = 6;
	spellElement_fire.base_mana = 6;
	spellElement_fire.overload_multiplier = 1;
	spellElement_fire.damage = 25;
	spellElement_fire.duration = 0;
	strcpy(spellElement_fire.element_internal_name, "spell_element_fireball");

	spellElementConstructor(&spellElement_lightning);
	spellElement_lightning.mana = 5;
	spellElement_lightning.base_mana = 5;
	spellElement_lightning.overload_multiplier = 1;
	spellElement_lightning.damage = 25;
	spellElement_lightning.duration = 75;
	strcpy(spellElement_lightning.element_internal_name, "spell_element_lightning");

	spellElementConstructor(&spellElement_light);
	spellElement_light.mana = 1;
	spellElement_light.base_mana = 1;
	spellElement_light.overload_multiplier = 1;
	spellElement_light.damage = 0;
	spellElement_light.duration = 750; //500 a better value? //NOTE: 750 is original value.
	strcpy(spellElement_light.element_internal_name, "spell_element_light");

	spellElementConstructor(&spellElement_dig);
	spellElement_dig.mana = 20;
	spellElement_dig.base_mana = 20;
	spellElement_dig.overload_multiplier = 1;
	spellElement_dig.damage = 0;
	spellElement_dig.duration = 0;
	strcpy(spellElement_dig.element_internal_name, "spell_element_dig");

	spellElementConstructor(&spellElement_invisible);
	spellElement_invisible.mana = 2;
	spellElement_invisible.base_mana = 2;
	spellElement_invisible.overload_multiplier = 1;
	spellElement_invisible.damage = 0;
	spellElement_invisible.duration = 100;
	strcpy(spellElement_invisible.element_internal_name, "spell_element_invisibility");

	spellElementConstructor(&spellElement_identify);
	spellElement_identify.mana = 10;
	spellElement_identify.base_mana = 10;
	spellElement_identify.overload_multiplier = 0; //NOTE: Might segfault due to divide by zero?
	spellElement_identify.damage = 0;
	spellElement_identify.duration = 0;
	strcpy(spellElement_identify.element_internal_name, "spell_element_identify");

	spellElementConstructor(&spellElement_magicmapping);
	spellElement_magicmapping.mana = 40;
	spellElement_magicmapping.base_mana = 40;
	spellElement_magicmapping.overload_multiplier = 0; //NOTE: Might segfault due to divide by zero?
	spellElement_magicmapping.damage = 0;
	spellElement_magicmapping.duration = 0;
	strcpy(spellElement_magicmapping.element_internal_name, "spell_element_magicmapping");

	spellElementConstructor(&spellElement_heal);
	spellElement_heal.mana = 1;
	spellElement_heal.base_mana = 1;
	spellElement_heal.overload_multiplier = 1;
	spellElement_heal.damage = 1;
	spellElement_heal.duration = 0;
	strcpy(spellElement_heal.element_internal_name, "spell_element_healing");

	spellElementConstructor(&spellElement_confuse);
	spellElement_confuse.mana = 4;
	spellElement_confuse.base_mana = 4;
	spellElement_confuse.overload_multiplier = 1;
	spellElement_confuse.damage = 0;
	spellElement_confuse.duration = TICKS_PER_SECOND * SPELLELEMENT_CONFUSE_BASE_DURATION; //TODO: Decide on something.
	strcpy(spellElement_confuse.element_internal_name, "spell_element_confuse");

	spellElementConstructor(&spellElement_cure_ailment);
	spellElement_cure_ailment.mana = 10;
	spellElement_cure_ailment.base_mana = 10;
	spellElement_cure_ailment.overload_multiplier = 0;
	spellElement_cure_ailment.damage = 0;
	spellElement_cure_ailment.duration = 0;
	strcpy(spellElement_cure_ailment.element_internal_name, "spell_element_cureailment");

	spellElementConstructor(&spellElement_locking);
	spellElement_locking.mana = 10;
	spellElement_locking.base_mana = 10;
	spellElement_locking.overload_multiplier = 0;
	spellElement_locking.damage = 0;
	spellElement_locking.duration = 0;
	strcpy(spellElement_locking.element_internal_name, "spell_element_locking");

	spellElementConstructor(&spellElement_opening);
	spellElement_opening.mana = 5;
	spellElement_opening.base_mana = 5;
	spellElement_opening.overload_multiplier = 0;
	spellElement_opening.damage = 0;
	spellElement_opening.duration = 0;
	strcpy(spellElement_opening.element_internal_name, "spell_element_opening");

	spellElementConstructor(&spellElement_sleep);
	spellElement_sleep.mana = 3;
	spellElement_sleep.base_mana = 3;
	spellElement_sleep.overload_multiplier = 0;
	spellElement_sleep.damage = 0;
	spellElement_sleep.duration = 0;
	strcpy(spellElement_sleep.element_internal_name, "spell_element_sleep");

	spellElementConstructor(&spellElement_cold);
	spellElement_cold.mana = 5;
	spellElement_cold.base_mana = 5;
	spellElement_cold.overload_multiplier = 1;
	spellElement_cold.damage = 20;
	spellElement_cold.duration = 180;
	strcpy(spellElement_cold.element_internal_name, "spell_element_cold");

	spellElementConstructor(&spellElement_slow);
	spellElement_slow.mana = 3;
	spellElement_slow.base_mana = 3;
	spellElement_slow.overload_multiplier = 1;
	spellElement_slow.damage = 0;
	spellElement_slow.duration = 180;
	strcpy(spellElement_slow.element_internal_name, "spell_element_slow");

	spellElementConstructor(&spellElement_levitation);
	spellElement_levitation.mana = 1;
	spellElement_levitation.base_mana = 1;
	spellElement_levitation.overload_multiplier = 1;
	spellElement_levitation.damage = 0;
	spellElement_levitation.duration = 30;
	strcpy(spellElement_levitation.element_internal_name, "spell_element_levitation");

	spellElementConstructor(&spellElement_teleportation);
	spellElement_teleportation.mana = 20;
	spellElement_teleportation.base_mana = 20;
	spellElement_teleportation.overload_multiplier = 0;
	spellElement_teleportation.damage = 0;
	spellElement_teleportation.duration = 0;
	strcpy(spellElement_teleportation.element_internal_name, "spell_element_teleportation");

	spellElementConstructor(&spellElement_selfPolymorph);
	spellElement_selfPolymorph.mana = 40;
	spellElement_selfPolymorph.base_mana = 40;
	spellElement_selfPolymorph.overload_multiplier = 0;
	spellElement_selfPolymorph.damage = 0;
	spellElement_selfPolymorph.duration = 0;
	strcpy(spellElement_selfPolymorph.element_internal_name, "spell_element_self_polymorph");

	spellElementConstructor(&spellElement_magicmissile);
	spellElement_magicmissile.mana = 6;
	spellElement_magicmissile.base_mana = 6;
	spellElement_magicmissile.overload_multiplier = 1;
	spellElement_magicmissile.damage = 30;
	spellElement_magicmissile.duration = 0;
	strcpy(spellElement_magicmissile.element_internal_name, "spell_element_magicmissile");

	spellElementConstructor(&spellElement_removecurse);
	spellElement_removecurse.mana = 20;
	spellElement_removecurse.base_mana = 20;
	spellElement_removecurse.overload_multiplier = 0;
	spellElement_removecurse.damage = 0;
	spellElement_removecurse.duration = 0;
	strcpy(spellElement_removecurse.element_internal_name, "spell_element_removecurse");

	spellElementConstructor(&spellElement_summon);
	spellElement_summon.mana = 17;
	spellElement_summon.base_mana = 17;
	spellElement_summon.overload_multiplier = 1;
	spellElement_summon.damage = 0;
	spellElement_summon.duration = 0;
	strcpy(spellElement_summon.element_internal_name, "spell_element_summon");

	spellElementConstructor(&spellElement_stoneblood);
	spellElement_stoneblood.mana = 20;
	spellElement_stoneblood.base_mana = 20;
	spellElement_stoneblood.overload_multiplier = 1;
	spellElement_stoneblood.damage = 0;
	spellElement_stoneblood.duration = TICKS_PER_SECOND * SPELLELEMENT_STONEBLOOD_BASE_DURATION;
	strcpy(spellElement_stoneblood.element_internal_name, "spell_element_stoneblood");

	spellElementConstructor(&spellElement_bleed);
	spellElement_bleed.mana = 10;
	spellElement_bleed.base_mana = 10;
	spellElement_bleed.overload_multiplier = 1;
	spellElement_bleed.damage = 30;
	spellElement_bleed.duration = TICKS_PER_SECOND * SPELLELEMENT_BLEED_BASE_DURATION; //TODO: Decide on something.;
	strcpy(spellElement_bleed.element_internal_name, "spell_element_bleed");

	spellElementConstructor(&spellElement_missile_trio);
	spellElement_missile_trio.mana = 1;
	spellElement_missile_trio.base_mana = 1;
	spellElement_missile_trio.overload_multiplier = 1;
	spellElement_missile_trio.damage = 0;
	spellElement_missile_trio.duration = 25; //1 second.
	strcpy(spellElement_missile_trio.element_internal_name, "spell_element_trio");

	spellElementConstructor(&spellElement_slime_spray);
	spellElement_slime_spray.mana = 1;
	spellElement_slime_spray.base_mana = 1;
	spellElement_slime_spray.overload_multiplier = 1;
	spellElement_slime_spray.damage = 0;
	spellElement_slime_spray.duration = 100;
	strcpy(spellElement_slime_spray.element_internal_name, "spell_element_slime_spray");

	spellElementConstructor(&spellElement_dominate);
	spellElement_dominate.mana = 20;
	spellElement_dominate.base_mana = 20;
	spellElement_dominate.overload_multiplier = 1;
	spellElement_dominate.damage = 0;
	spellElement_dominate.duration = 0;
	strcpy(spellElement_dominate.element_internal_name, "spell_element_dominate");

	spellElementConstructor(&spellElement_reflectMagic);
	spellElement_reflectMagic.mana = 10;
	spellElement_reflectMagic.base_mana = 10;
	spellElement_reflectMagic.overload_multiplier = 1;
	spellElement_reflectMagic.damage = 0;
	spellElement_reflectMagic.duration = 3000;
	strcpy(spellElement_reflectMagic.element_internal_name, "spell_element_reflect_magic");

	spellElementConstructor(&spellElement_acidSpray);
	spellElement_acidSpray.mana = 10;
	spellElement_acidSpray.base_mana = 10;
	spellElement_acidSpray.overload_multiplier = 1;
	spellElement_acidSpray.damage = 10;
	spellElement_acidSpray.duration = TICKS_PER_SECOND * SPELLELEMENT_ACIDSPRAY_BASE_DURATION; //TODO: Decide on something.;
	strcpy(spellElement_acidSpray.element_internal_name, "spell_element_acid_spray");

	spellElementConstructor(&spellElement_stealWeapon);
	spellElement_stealWeapon.mana = 50;
	spellElement_stealWeapon.base_mana = 50;
	spellElement_stealWeapon.overload_multiplier = 1;
	spellElement_stealWeapon.damage = 0;
	spellElement_stealWeapon.duration = 0;
	strcpy(spellElement_stealWeapon.element_internal_name, "spell_element_steal_weapon");

	spellElementConstructor(&spellElement_drainSoul);
	spellElement_drainSoul.mana = 17;
	spellElement_drainSoul.base_mana = 17;
	spellElement_drainSoul.overload_multiplier = 1;
	spellElement_drainSoul.damage = 18;
	spellElement_drainSoul.duration = 0;
	strcpy(spellElement_drainSoul.element_internal_name, "spell_element_drain_soul");

	spellElementConstructor(&spellElement_vampiricAura);
	spellElement_vampiricAura.mana = 5;
	spellElement_vampiricAura.base_mana = 5;
	spellElement_vampiricAura.overload_multiplier = 1;
	spellElement_vampiricAura.damage = 0;
	spellElement_vampiricAura.duration = 85; //TODO: Decide on something.
	strcpy(spellElement_vampiricAura.element_internal_name, "spell_element_vampiric_aura");

	spellElementConstructor(&spellElement_amplifyMagic);
	spellElement_amplifyMagic.mana = 7;
	spellElement_amplifyMagic.base_mana = 7;
	spellElement_amplifyMagic.overload_multiplier = 1;
	spellElement_amplifyMagic.damage = 0;
	spellElement_amplifyMagic.duration = 85; //TODO: Decide on something.
	strcpy(spellElement_amplifyMagic.element_internal_name, "spell_element_amplify_magic");

	spellElementConstructor(&spellElement_charmMonster);
	spellElement_charmMonster.mana = 49;
	spellElement_charmMonster.base_mana = 49;
	spellElement_charmMonster.overload_multiplier = 1;
	spellElement_charmMonster.damage = 0;
	spellElement_charmMonster.duration = 300;
	strcpy(spellElement_charmMonster.element_internal_name, "spell_element_charm");

	spellElementConstructor(&spellElement_shapeshift);
	spellElement_shapeshift.mana = 1;
	spellElement_shapeshift.base_mana = 1;
	spellElement_shapeshift.overload_multiplier = 1;
	spellElement_shapeshift.damage = 0;
	spellElement_shapeshift.duration = 0;
	strcpy(spellElement_shapeshift.element_internal_name, "spell_element_shapeshift");

	spellElementConstructor(&spellElement_sprayWeb);
	spellElement_sprayWeb.mana = 7;
	spellElement_sprayWeb.base_mana = 7;
	spellElement_sprayWeb.overload_multiplier = 1;
	spellElement_sprayWeb.damage = 0;
	spellElement_sprayWeb.duration = 0;
	strcpy(spellElement_sprayWeb.element_internal_name, "spell_element_spray_web");

	spellElementConstructor(&spellElement_poison);
	spellElement_poison.mana = 4;
	spellElement_poison.base_mana = 4;
	spellElement_poison.overload_multiplier = 1;
	spellElement_poison.damage = 10;
	spellElement_poison.duration = 0;
	strcpy(spellElement_poison.element_internal_name, "spell_element_poison");

	spellElementConstructor(&spellElement_speed);
	spellElement_speed.mana = 11;
	spellElement_speed.base_mana = 11;
	spellElement_speed.overload_multiplier = 1;
	spellElement_speed.damage = 0;
	spellElement_speed.duration = 30 * TICKS_PER_SECOND;
	strcpy(spellElement_speed.element_internal_name, "spell_element_speed");

	spellElementConstructor(&spellElement_fear);
	spellElement_fear.mana = 28;
	spellElement_fear.base_mana = 28;
	spellElement_fear.overload_multiplier = 1;
	spellElement_fear.damage = 0;
	spellElement_fear.duration = 0;
	strcpy(spellElement_fear.element_internal_name, "spell_element_fear");

	spellElementConstructor(&spellElement_strike);
	spellElement_strike.mana = 23;
	spellElement_strike.base_mana = 23;
	spellElement_strike.overload_multiplier = 1;
	spellElement_strike.damage = 1;
	spellElement_strike.duration = 0;
	strcpy(spellElement_strike.element_internal_name, "spell_element_strike");

	spellElementConstructor(&spellElement_weakness);
	spellElement_weakness.mana = 1;
	spellElement_weakness.base_mana = 1;
	spellElement_weakness.overload_multiplier = 1;
	spellElement_weakness.damage = 0;
	spellElement_weakness.duration = 50;
	strcpy(spellElement_weakness.element_internal_name, "spell_element_weakness");

	spellElementConstructor(&spellElement_detectFood);
	spellElement_detectFood.mana = 14;
	spellElement_detectFood.base_mana = 14;
	spellElement_detectFood.overload_multiplier = 0; //NOTE: Might segfault due to divide by zero?
	spellElement_detectFood.damage = 0;
	spellElement_detectFood.duration = 0;
	strcpy(spellElement_detectFood.element_internal_name, "spell_element_detect_food");

	spellElementConstructor(&spellElement_trollsBlood);
	spellElement_trollsBlood.mana = 25;
	spellElement_trollsBlood.base_mana = 25;
	spellElement_trollsBlood.overload_multiplier = 0; //NOTE: Might segfault due to divide by zero?
	spellElement_trollsBlood.damage = 0;
	spellElement_trollsBlood.duration = 80 * TICKS_PER_SECOND;
	strcpy(spellElement_trollsBlood.element_internal_name, "spell_element_trolls_blood");

	spellElementConstructor(&spellElement_flutter);
	spellElement_flutter.mana = 10;
	spellElement_flutter.base_mana = 10;
	spellElement_flutter.overload_multiplier = 1;
	spellElement_flutter.damage = 0;
	spellElement_flutter.duration = 6 * TICKS_PER_SECOND;
	strcpy(spellElement_flutter.element_internal_name, "spell_element_flutter");

	spellElementConstructor(&spellElement_dash);
	spellElement_dash.mana = 5;
	spellElement_dash.base_mana = 5;
	spellElement_dash.overload_multiplier = 1;
	spellElement_dash.damage = 0;
	spellElement_dash.duration = 1 * TICKS_PER_SECOND;
	strcpy(spellElement_dash.element_internal_name, "spell_element_dash");

	spellElementConstructor(&spellElement_salvageItem);
	spellElement_salvageItem.mana = 6;
	spellElement_salvageItem.base_mana = 6;
	spellElement_salvageItem.overload_multiplier = 0; //NOTE: Might segfault due to divide by zero?
	spellElement_salvageItem.damage = 0;
	spellElement_salvageItem.duration = 0;
	strcpy(spellElement_salvageItem.element_internal_name, "spell_element_salvage");

	spellElementConstructor(&spellElement_shadowTag);
	spellElement_shadowTag.mana = 4;
	spellElement_shadowTag.base_mana = 4;
	spellElement_shadowTag.overload_multiplier = 1;
	spellElement_shadowTag.damage = 0;
	spellElement_shadowTag.duration = 0;
	strcpy(spellElement_shadowTag.element_internal_name, "spell_element_shadow_tag");

	spellElementConstructor(&spellElement_telePull);
	spellElement_telePull.mana = 19;
	spellElement_telePull.base_mana = 19;
	spellElement_telePull.overload_multiplier = 1;
	spellElement_telePull.damage = 0;
	spellElement_telePull.duration = 0;
	strcpy(spellElement_telePull.element_internal_name, "spell_element_telepull");

	spellElementConstructor(&spellElement_demonIllusion);
	spellElement_demonIllusion.mana = 24;
	spellElement_demonIllusion.base_mana = 24;
	spellElement_demonIllusion.overload_multiplier = 1;
	spellElement_demonIllusion.damage = 0;
	spellElement_demonIllusion.duration = 0;
	strcpy(spellElement_demonIllusion.element_internal_name, "spell_element_demon_illu");

	spellElementConstructor(&spellElement_ghostBolt);
	spellElement_ghostBolt.mana = 5;
	spellElement_ghostBolt.base_mana = 5;
	spellElement_ghostBolt.overload_multiplier = 1;
	spellElement_ghostBolt.damage = 0;
	spellElement_ghostBolt.duration = 75;
	strcpy(spellElement_ghostBolt.element_internal_name, "spell_element_ghost_bolt");

	spellElementConstructor(&spellElement_slimeAcid);
	spellElement_slimeAcid.mana = 5;
	spellElement_slimeAcid.base_mana = 5;
	spellElement_slimeAcid.overload_multiplier = 1;
	spellElement_slimeAcid.damage = 5;
	spellElement_slimeAcid.duration = 100;
	strcpy(spellElement_slimeAcid.element_internal_name, "spell_element_slime_acid");

	spellElementConstructor(&spellElement_slimeWater);
	spellElement_slimeWater.mana = 5;
	spellElement_slimeWater.base_mana = 5;
	spellElement_slimeWater.overload_multiplier = 1;
	spellElement_slimeWater.damage = 5;
	spellElement_slimeWater.duration = 100;
	strcpy(spellElement_slimeWater.element_internal_name, "spell_element_slime_water");

	spellElementConstructor(&spellElement_slimeFire);
	spellElement_slimeFire.mana = 5;
	spellElement_slimeFire.base_mana = 5;
	spellElement_slimeFire.overload_multiplier = 1;
	spellElement_slimeFire.damage = 5;
	spellElement_slimeFire.duration = 100;
	strcpy(spellElement_slimeFire.element_internal_name, "spell_element_slime_fire");

	spellElementConstructor(&spellElement_slimeTar);
	spellElement_slimeTar.mana = 5;
	spellElement_slimeTar.base_mana = 5;
	spellElement_slimeTar.overload_multiplier = 1;
	spellElement_slimeTar.damage = 5;
	spellElement_slimeTar.duration = 100;
	strcpy(spellElement_slimeTar.element_internal_name, "spell_element_slime_tar");

	spellElementConstructor(&spellElement_slimeMetal);
	spellElement_slimeMetal.mana = 5;
	spellElement_slimeMetal.base_mana = 5;
	spellElement_slimeMetal.overload_multiplier = 1;
	spellElement_slimeMetal.damage = 5;
	spellElement_slimeMetal.duration = 100;
	strcpy(spellElement_slimeMetal.element_internal_name, "spell_element_slime_metal");

	spellConstructor(&spell_forcebolt, SPELL_FORCEBOLT);
	strcpy(spell_forcebolt.spell_internal_name, "spell_forcebolt");
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

	spellConstructor(&spell_magicmissile, SPELL_MAGICMISSILE);
	strcpy(spell_magicmissile.spell_internal_name, "spell_magicmissile");
	spell_magicmissile.difficulty = 60;
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

	spellConstructor(&spell_cold, SPELL_COLD);
	strcpy(spell_cold.spell_internal_name, "spell_cold");
	spell_cold.difficulty = 40;
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

	spellConstructor(&spell_fireball, SPELL_FIREBALL);
	strcpy(spell_fireball.spell_internal_name, "spell_fireball");
	spell_fireball.difficulty = 20;
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

	spellConstructor(&spell_lightning, SPELL_LIGHTNING);
	strcpy(spell_lightning.spell_internal_name, "spell_lightning");
	spell_lightning.difficulty = 60;
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

	spellConstructor(&spell_removecurse, SPELL_REMOVECURSE);
	strcpy(spell_removecurse.spell_internal_name, "spell_removecurse");
	spell_removecurse.difficulty = 60;
	spell_removecurse.elements.first = NULL;
	spell_removecurse.elements.last = NULL;
	node = list_AddNodeLast(&spell_removecurse.elements);
	node->element = copySpellElement(&spellElement_removecurse);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node; //Tell the element what list it resides in.

	spellConstructor(&spell_light, SPELL_LIGHT);
	strcpy(spell_light.spell_internal_name, "spell_light");
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

	spellConstructor(&spell_identify, SPELL_IDENTIFY);
	strcpy(spell_identify.spell_internal_name, "spell_identify");
	spell_identify.difficulty = 60;
	spell_identify.elements.first = NULL;
	spell_identify.elements.last = NULL;
	node = list_AddNodeLast(&spell_identify.elements);
	node->element = copySpellElement(&spellElement_identify);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node; //Tell the element what list it resides in.

	spellConstructor(&spell_magicmapping, SPELL_MAGICMAPPING);
	strcpy(spell_magicmapping.spell_internal_name, "spell_magicmapping");
	spell_magicmapping.difficulty = 60;
	spell_magicmapping.elements.first = NULL;
	spell_magicmapping.elements.last = NULL;
	node = list_AddNodeLast(&spell_magicmapping.elements);
	node->element = copySpellElement(&spellElement_magicmapping);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node;

	spellConstructor(&spell_sleep, SPELL_SLEEP);
	strcpy(spell_sleep.spell_internal_name, "spell_sleep");
	spell_sleep.difficulty = 20;
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

	spellConstructor(&spell_confuse, SPELL_CONFUSE);
	strcpy(spell_confuse.spell_internal_name, "spell_confuse");
	spell_confuse.difficulty = 20;
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

	spellConstructor(&spell_slow, SPELL_SLOW);
	strcpy(spell_slow.spell_internal_name, "spell_slow");
	spell_slow.difficulty = 20;
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

	spellConstructor(&spell_opening, SPELL_OPENING);
	strcpy(spell_opening.spell_internal_name, "spell_opening");
	spell_opening.difficulty = 20;
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

	spellConstructor(&spell_locking, SPELL_LOCKING);
	strcpy(spell_locking.spell_internal_name, "spell_locking");
	spell_locking.difficulty = 20;
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

	spellConstructor(&spell_levitation, SPELL_LEVITATION);
	strcpy(spell_levitation.spell_internal_name, "spell_levitation");
	spell_levitation.difficulty = 80;
	spell_levitation.elements.first = NULL;
	spell_levitation.elements.last = NULL;
	node = list_AddNodeLast(&spell_levitation.elements);
	node->element = copySpellElement(&spellElement_levitation);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node; //Tell the element what list it resides in.
	element->channeled = true;

	spellConstructor(&spell_invisibility, SPELL_INVISIBILITY);
	strcpy(spell_invisibility.spell_internal_name, "spell_invisibility");
	spell_invisibility.difficulty = 80;
	spell_invisibility.elements.first = NULL;
	spell_invisibility.elements.last = NULL;
	node = list_AddNodeLast(&spell_invisibility.elements);
	node->element = copySpellElement(&spellElement_invisible);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node; //Tell the element what list it resides in.
	element->channeled = true;

	spellConstructor(&spell_teleportation, SPELL_TELEPORTATION);
	strcpy(spell_teleportation.spell_internal_name, "spell_teleportation");
	spell_teleportation.difficulty = 80;
	spell_teleportation.elements.first = NULL;
	spell_teleportation.elements.last = NULL;
	node = list_AddNodeLast(&spell_teleportation.elements);
	node->element = copySpellElement(&spellElement_teleportation);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node; //Tell the element what list it resides in.

	spellConstructor(&spell_polymorph, SPELL_SELF_POLYMORPH);
	strcpy(spell_polymorph.spell_internal_name, "spell_self_polymorph");
	spell_polymorph.difficulty = 60;
	spell_polymorph.elements.first = NULL;
	spell_polymorph.elements.last = NULL;
	node = list_AddNodeLast(&spell_polymorph.elements);
	node->element = copySpellElement(&spellElement_selfPolymorph);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node; //Tell the element what list it resides in.

	spellConstructor(&spell_healing, SPELL_HEALING);
	strcpy(spell_healing.spell_internal_name, "spell_healing");
	spell_healing.difficulty = 20;
	spell_healing.elements.first = NULL;
	spell_healing.elements.last = NULL;
	node = list_AddNodeLast(&spell_healing.elements);
	node->element = copySpellElement(&spellElement_heal);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node;
	element->mana = 10;

	spellConstructor(&spell_extrahealing, SPELL_EXTRAHEALING);
	strcpy(spell_extrahealing.spell_internal_name, "spell_extrahealing");
	spell_extrahealing.difficulty = 60;
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

	spellConstructor(&spell_cureailment, SPELL_CUREAILMENT);
	strcpy(spell_cureailment.spell_internal_name, "spell_cureailment");
	spell_cureailment.difficulty = 20;
	spell_cureailment.elements.first = NULL;
	spell_cureailment.elements.last = NULL;
	node = list_AddNodeLast(&spell_cureailment.elements);
	node->element = copySpellElement(&spellElement_cure_ailment);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node;

	spellConstructor(&spell_dig, SPELL_DIG);
	strcpy(spell_dig.spell_internal_name, "spell_dig");
	spell_dig.difficulty = 40;
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

	spellConstructor(&spell_stoneblood, SPELL_STONEBLOOD);
	strcpy(spell_stoneblood.spell_internal_name, "spell_stoneblood");
	spell_stoneblood.difficulty = 80;
	node = list_AddNodeLast(&spell_stoneblood.elements);
	node->element = copySpellElement(&spellElement_missile_trio);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node; //Tell the element what list it resides in.
	//Now for the second element.
	element->elements.first = NULL;
	element->elements.last = NULL;
	node = list_AddNodeLast(&element->elements);
	node->element = copySpellElement(&spellElement_stoneblood);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node;

	spellConstructor(&spell_bleed, SPELL_BLEED);
	strcpy(spell_bleed.spell_internal_name, "spell_bleed");
	spell_bleed.difficulty = 80;
	node = list_AddNodeLast(&spell_bleed.elements);
	node->element = copySpellElement(&spellElement_missile);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node; //Tell the element what list it resides in.
	//Now for the second element.
	element->elements.first = NULL;
	element->elements.last = NULL;
	node = list_AddNodeLast(&element->elements);
	node->element = copySpellElement(&spellElement_bleed);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node;

	spellConstructor(&spell_summon, SPELL_SUMMON);
	strcpy(spell_summon.spell_internal_name, "spell_summon");
	spell_summon.difficulty = 40;
	spell_summon.elements.first = NULL;
	spell_summon.elements.last = NULL;
	node = list_AddNodeLast(&spell_summon.elements);
	node->element = copySpellElement(&spellElement_summon);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node;

	spellConstructor(&spell_dominate, SPELL_DOMINATE);
	strcpy(spell_dominate.spell_internal_name, "spell_dominate");
	spell_dominate.difficulty = 100;
	node = list_AddNodeLast(&spell_dominate.elements);
	node->element = copySpellElement(&spellElement_missile);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node; //Tell the element what list it resides in.
	//Now for the second element.
	element->elements.first = NULL;
	element->elements.last = NULL;
	node = list_AddNodeLast(&element->elements);
	node->element = copySpellElement(&spellElement_dominate);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node;

	spellConstructor(&spell_reflectMagic, SPELL_REFLECT_MAGIC);
	strcpy(spell_reflectMagic.spell_internal_name, "spell_reflect_magic");
	spell_reflectMagic.difficulty = 80;
	spell_reflectMagic.elements.first = nullptr;
	spell_reflectMagic.elements.last = nullptr;
	node = list_AddNodeLast(&spell_reflectMagic.elements);
	node->element = copySpellElement(&spellElement_reflectMagic);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*) node->element;
	element->node = node; //Tell the element what list it resides in.
	element->channeled = true;

	spellConstructor(&spell_acidSpray, SPELL_ACID_SPRAY);
	strcpy(spell_acidSpray.spell_internal_name, "spell_acid_spray");
	spell_acidSpray.difficulty = 80;
	node = list_AddNodeLast(&spell_acidSpray.elements);
	node->element = copySpellElement(&spellElement_missile_trio);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node; //Tell the element what list it resides in.
						  //Now for the second element.
	element->elements.first = NULL;
	element->elements.last = NULL;
	node = list_AddNodeLast(&element->elements);
	node->element = copySpellElement(&spellElement_acidSpray);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node;

	spellConstructor(&spell_stealWeapon, SPELL_STEAL_WEAPON);
	strcpy(spell_stealWeapon.spell_internal_name, "spell_steal_weapon");
	spell_stealWeapon.difficulty = 100;
	node = list_AddNodeLast(&spell_stealWeapon.elements);
	node->element = copySpellElement(&spellElement_missile);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node; //Tell the element what list it resides in.
						  //Now for the second element.
	element->elements.first = NULL;
	element->elements.last = NULL;
	node = list_AddNodeLast(&element->elements);
	node->element = copySpellElement(&spellElement_stealWeapon);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node;

	spellConstructor(&spell_drainSoul, SPELL_DRAIN_SOUL);
	strcpy(spell_drainSoul.spell_internal_name, "spell_drain_soul");
	spell_drainSoul.difficulty = 80;
	node = list_AddNodeLast(&spell_drainSoul.elements);
	node->element = copySpellElement(&spellElement_missile);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node; //Tell the element what list it resides in.
	//Now for the second element.
	element->elements.first = NULL;
	element->elements.last = NULL;
	node = list_AddNodeLast(&element->elements);
	node->element = copySpellElement(&spellElement_drainSoul);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node;

	spellConstructor(&spell_vampiricAura, SPELL_VAMPIRIC_AURA);
	strcpy(spell_vampiricAura.spell_internal_name, "spell_vampiric_aura");
	spell_vampiricAura.difficulty = 80;
	spell_vampiricAura.elements.first = nullptr;
	spell_vampiricAura.elements.last = nullptr;
	node = list_AddNodeLast(&spell_vampiricAura.elements);
	node->element = copySpellElement(&spellElement_vampiricAura);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node; //Tell the element what list it resides in.
	element->channeled = true;

	spellConstructor(&spell_amplifyMagic, SPELL_AMPLIFY_MAGIC);
	strcpy(spell_amplifyMagic.spell_internal_name, "spell_amplify_magic");
	spell_amplifyMagic.difficulty = 80;
	spell_amplifyMagic.elements.first = nullptr;
	spell_amplifyMagic.elements.last = nullptr;
	node = list_AddNodeLast(&spell_amplifyMagic.elements);
	node->element = copySpellElement(&spellElement_amplifyMagic);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node; //Tell the element what list it resides in.
	element->channeled = true;

	spellConstructor(&spell_charmMonster, SPELL_CHARM_MONSTER);
	strcpy(spell_charmMonster.spell_internal_name, "spell_charm");
	spell_charmMonster.difficulty = 80;
	node = list_AddNodeLast(&spell_charmMonster.elements);
	node->element = copySpellElement(&spellElement_missile);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node; //Tell the element what list it resides in.
						  //Now for the second element.
	element->elements.first = NULL;
	element->elements.last = NULL;
	node = list_AddNodeLast(&element->elements);
	node->element = copySpellElement(&spellElement_charmMonster);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node;

	spellConstructor(&spell_revertForm, SPELL_REVERT_FORM);
	strcpy(spell_revertForm.spell_internal_name, "spell_revert_form");
	spell_revertForm.difficulty = 0;
	spell_revertForm.elements.first = NULL;
	spell_revertForm.elements.last = NULL;
	node = list_AddNodeLast(&spell_revertForm.elements);
	node->element = copySpellElement(&spellElement_shapeshift);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node;
	element->mana = 5;

	spellConstructor(&spell_ratForm, SPELL_RAT_FORM);
	strcpy(spell_ratForm.spell_internal_name, "spell_rat_form");
	spell_ratForm.difficulty = 0;
	spell_ratForm.elements.first = NULL;
	spell_ratForm.elements.last = NULL;
	node = list_AddNodeLast(&spell_ratForm.elements);
	node->element = copySpellElement(&spellElement_shapeshift);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node;
	element->mana = 8;

	spellConstructor(&spell_spiderForm, SPELL_SPIDER_FORM);
	strcpy(spell_spiderForm.spell_internal_name, "spell_spider_form");
	spell_spiderForm.difficulty = 40;
	spell_spiderForm.elements.first = NULL;
	spell_spiderForm.elements.last = NULL;
	node = list_AddNodeLast(&spell_spiderForm.elements);
	node->element = copySpellElement(&spellElement_shapeshift);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node;
	element->mana = 16;

	spellConstructor(&spell_trollForm, SPELL_TROLL_FORM);
	strcpy(spell_trollForm.spell_internal_name, "spell_troll_form");
	spell_trollForm.difficulty = 60;
	spell_trollForm.elements.first = NULL;
	spell_trollForm.elements.last = NULL;
	node = list_AddNodeLast(&spell_trollForm.elements);
	node->element = copySpellElement(&spellElement_shapeshift);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node;
	element->mana = 24;

	spellConstructor(&spell_impForm, SPELL_IMP_FORM);
	strcpy(spell_impForm.spell_internal_name, "spell_imp_form");
	spell_impForm.difficulty = 80;
	spell_impForm.elements.first = NULL;
	spell_impForm.elements.last = NULL;
	node = list_AddNodeLast(&spell_impForm.elements);
	node->element = copySpellElement(&spellElement_shapeshift);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node;
	element->mana = 32;

	spellConstructor(&spell_sprayWeb, SPELL_SPRAY_WEB);
	strcpy(spell_sprayWeb.spell_internal_name, "spell_spray_web");
	spell_sprayWeb.difficulty = 20;
	node = list_AddNodeLast(&spell_sprayWeb.elements);
	node->element = copySpellElement(&spellElement_missile_trio);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node; //Tell the element what list it resides in.
						  //Now for the second element.
	element->elements.first = NULL;
	element->elements.last = NULL;
	node = list_AddNodeLast(&element->elements);
	node->element = copySpellElement(&spellElement_sprayWeb);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node;

	spellConstructor(&spell_poison, SPELL_POISON);
	strcpy(spell_poison.spell_internal_name, "spell_poison");
	spell_poison.difficulty = 40;
	node = list_AddNodeLast(&spell_poison.elements);
	node->element = copySpellElement(&spellElement_missile);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node; //Tell the element what list it resides in.
						  //Now for the second element.
	element->elements.first = NULL;
	element->elements.last = NULL;
	node = list_AddNodeLast(&element->elements);
	node->element = copySpellElement(&spellElement_poison);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node;

	spellConstructor(&spell_speed, SPELL_SPEED);
	strcpy(spell_speed.spell_internal_name, "spell_speed");
	spell_speed.difficulty = 40;
	spell_speed.elements.first = NULL;
	spell_speed.elements.last = NULL;
	node = list_AddNodeLast(&spell_speed.elements);
	node->element = copySpellElement(&spellElement_speed);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node;

	spellConstructor(&spell_fear, SPELL_FEAR);
	strcpy(spell_fear.spell_internal_name, "spell_fear");
	spell_fear.difficulty = 80;
	spell_fear.elements.first = NULL;
	spell_fear.elements.last = NULL;
	node = list_AddNodeLast(&spell_fear.elements);
	node->element = copySpellElement(&spellElement_fear);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node;

	spellConstructor(&spell_weakness, SPELL_WEAKNESS);
	strcpy(spell_weakness.spell_internal_name, "spell_weakness");
	spell_weakness.difficulty = 100;
	node = list_AddNodeLast(&spell_weakness.elements);
	node->element = copySpellElement(&spellElement_missile);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node; //Tell the element what list it resides in.
						  //Now for the second element.
	element->elements.first = NULL;
	element->elements.last = NULL;
	node = list_AddNodeLast(&element->elements);
	node->element = copySpellElement(&spellElement_weakness);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node;

	spellConstructor(&spell_strike, SPELL_STRIKE);
	strcpy(spell_strike.spell_internal_name, "spell_strike");
	spell_strike.difficulty = 80;
	spell_strike.elements.first = NULL;
	spell_strike.elements.last = NULL;
	node = list_AddNodeLast(&spell_strike.elements);
	node->element = copySpellElement(&spellElement_strike);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node;

	spellConstructor(&spell_detectFood, SPELL_DETECT_FOOD);
	strcpy(spell_detectFood.spell_internal_name, "spell_detect_food");
	spell_detectFood.difficulty = 40;
	spell_detectFood.elements.first = NULL;
	spell_detectFood.elements.last = NULL;
	node = list_AddNodeLast(&spell_detectFood.elements);
	node->element = copySpellElement(&spellElement_detectFood);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node;

	spellConstructor(&spell_trollsBlood, SPELL_TROLLS_BLOOD);
	strcpy(spell_trollsBlood.spell_internal_name, "spell_trolls_blood");
	spell_trollsBlood.difficulty = 40;
	spell_trollsBlood.elements.first = NULL;
	spell_trollsBlood.elements.last = NULL;
	node = list_AddNodeLast(&spell_trollsBlood.elements);
	node->element = copySpellElement(&spellElement_trollsBlood);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node;

	spellConstructor(&spell_flutter, SPELL_FLUTTER);
	strcpy(spell_flutter.spell_internal_name, "spell_flutter");
	spell_flutter.difficulty = 60;
	spell_flutter.elements.first = NULL;
	spell_flutter.elements.last = NULL;
	node = list_AddNodeLast(&spell_flutter.elements);
	node->element = copySpellElement(&spellElement_flutter);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node;

	spellConstructor(&spell_dash, SPELL_DASH);
	strcpy(spell_dash.spell_internal_name, "spell_dash");
	spell_dash.difficulty = 40;
	spell_dash.elements.first = NULL;
	spell_dash.elements.last = NULL;
	node = list_AddNodeLast(&spell_dash.elements);
	node->element = copySpellElement(&spellElement_dash);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node;

	spellConstructor(&spell_shadowTag, SPELL_SHADOW_TAG);
	strcpy(spell_shadowTag.spell_internal_name, "spell_shadow_tag");
	spell_shadowTag.difficulty = 20;
	node = list_AddNodeLast(&spell_shadowTag.elements);
	node->element = copySpellElement(&spellElement_missile);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node; //Tell the element what list it resides in.
						  //Now for the second element.
	element->elements.first = NULL;
	element->elements.last = NULL;
	node = list_AddNodeLast(&element->elements);
	node->element = copySpellElement(&spellElement_shadowTag);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node;

	spellConstructor(&spell_telePull, SPELL_TELEPULL);
	strcpy(spell_telePull.spell_internal_name, "spell_telepull");
	spell_telePull.difficulty = 60;
	node = list_AddNodeLast(&spell_telePull.elements);
	node->element = copySpellElement(&spellElement_missile);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node; //Tell the element what list it resides in.
						  //Now for the second element.
	element->elements.first = NULL;
	element->elements.last = NULL;
	node = list_AddNodeLast(&element->elements);
	node->element = copySpellElement(&spellElement_telePull);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node;

	spellConstructor(&spell_demonIllusion, SPELL_DEMON_ILLUSION);
	strcpy(spell_demonIllusion.spell_internal_name, "spell_demon_illu");
	spell_demonIllusion.difficulty = 80;
	node = list_AddNodeLast(&spell_demonIllusion.elements);
	node->element = copySpellElement(&spellElement_missile);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node; //Tell the element what list it resides in.
						  //Now for the second element.
	element->elements.first = NULL;
	element->elements.last = NULL;
	node = list_AddNodeLast(&element->elements);
	node->element = copySpellElement(&spellElement_demonIllusion);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node;

	spellConstructor(&spell_salvageItem, SPELL_SALVAGE);
	strcpy(spell_salvageItem.spell_internal_name, "spell_salvage");
	spell_salvageItem.difficulty = 20;
	spell_salvageItem.elements.first = NULL;
	spell_salvageItem.elements.last = NULL;
	node = list_AddNodeLast(&spell_salvageItem.elements);
	node->element = copySpellElement(&spellElement_salvageItem);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node;

	spellConstructor(&spell_ghost_bolt, SPELL_GHOST_BOLT);
	strcpy(spell_ghost_bolt.spell_internal_name, "spell_ghost_bolt");
	spell_ghost_bolt.difficulty = 100;
	spell_ghost_bolt.elements.first = NULL;
	spell_ghost_bolt.elements.last = NULL;
	node = list_AddNodeLast(&spell_ghost_bolt.elements);
	node->element = copySpellElement(&spellElement_missile);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node; //Tell the element what list it resides in.
	//Now for the second element.
	element->elements.first = NULL;
	element->elements.last = NULL;
	node = list_AddNodeLast(&element->elements);
	node->element = copySpellElement(&spellElement_ghostBolt);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node;

	spellConstructor(&spell_slime_acid, SPELL_SLIME_ACID);
	strcpy(spell_slime_acid.spell_internal_name, "spell_slime_acid");
	spell_slime_acid.difficulty = 100;
	spell_slime_acid.elements.first = NULL;
	spell_slime_acid.elements.last = NULL;
	node = list_AddNodeLast(&spell_slime_acid.elements);
	node->element = copySpellElement(&spellElement_slime_spray);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node; //Tell the element what list it resides in.
	//Now for the second element.
	element->elements.first = NULL;
	element->elements.last = NULL;
	node = list_AddNodeLast(&element->elements);
	node->element = copySpellElement(&spellElement_slimeAcid);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node;

	spellConstructor(&spell_slime_water, SPELL_SLIME_WATER);
	strcpy(spell_slime_water.spell_internal_name, "spell_slime_water");
	spell_slime_water.difficulty = 100;
	spell_slime_water.elements.first = NULL;
	spell_slime_water.elements.last = NULL;
	node = list_AddNodeLast(&spell_slime_water.elements);
	node->element = copySpellElement(&spellElement_slime_spray);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node; //Tell the element what list it resides in.
	//Now for the second element.
	element->elements.first = NULL;
	element->elements.last = NULL;
	node = list_AddNodeLast(&element->elements);
	node->element = copySpellElement(&spellElement_slimeWater);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node;

	spellConstructor(&spell_slime_fire, SPELL_SLIME_FIRE);
	strcpy(spell_slime_fire.spell_internal_name, "spell_slime_fire");
	spell_slime_fire.difficulty = 100;
	spell_slime_fire.elements.first = NULL;
	spell_slime_fire.elements.last = NULL;
	node = list_AddNodeLast(&spell_slime_fire.elements);
	node->element = copySpellElement(&spellElement_slime_spray);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node; //Tell the element what list it resides in.
	//Now for the second element.
	element->elements.first = NULL;
	element->elements.last = NULL;
	node = list_AddNodeLast(&element->elements);
	node->element = copySpellElement(&spellElement_slimeFire);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node;

	spellConstructor(&spell_slime_tar, SPELL_SLIME_TAR);
	strcpy(spell_slime_tar.spell_internal_name, "spell_slime_tar");
	spell_slime_tar.difficulty = 100;
	spell_slime_tar.elements.first = NULL;
	spell_slime_tar.elements.last = NULL;
	node = list_AddNodeLast(&spell_slime_tar.elements);
	node->element = copySpellElement(&spellElement_slime_spray);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node; //Tell the element what list it resides in.
	//Now for the second element.
	element->elements.first = NULL;
	element->elements.last = NULL;
	node = list_AddNodeLast(&element->elements);
	node->element = copySpellElement(&spellElement_slimeTar);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node;

	spellConstructor(&spell_slime_metal, SPELL_SLIME_METAL);
	strcpy(spell_slime_metal.spell_internal_name, "spell_slime_metal");
	spell_slime_metal.difficulty = 100;
	spell_slime_metal.elements.first = NULL;
	spell_slime_metal.elements.last = NULL;
	node = list_AddNodeLast(&spell_slime_metal.elements);
	node->element = copySpellElement(&spellElement_slime_spray);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node; //Tell the element what list it resides in.
	//Now for the second element.
	element->elements.first = NULL;
	element->elements.last = NULL;
	node = list_AddNodeLast(&element->elements);
	node->element = copySpellElement(&spellElement_slimeMetal);
	node->size = sizeof(spellElement_t);
	node->deconstructor = &spellElementDeconstructor;
	element = (spellElement_t*)node->element;
	element->node = node;

	spell_t* spell = nullptr;

	spellElementConstructor(SPELL_ELEMENT_PROPULSION_FOCI_SPRAY,
		1,		// mana
		1,		// base mana
		1,		// overload
		0,		// damage
		100,	// duration
		"spell_element_foci_spray");

	spellElementConstructor(SPELL_FOCI_FIRE,
		1,		// mana
		1,		// base mana
		1,		// overload
		0,		// damage
		100,	// duration
		"spell_element_foci_fire");
	spell = spellConstructor(
		SPELL_FOCI_FIRE,											// ID
		100,														// difficulty
		"spell_foci_fire",											// internal name
		// elements
		{ SPELL_ELEMENT_PROPULSION_FOCI_SPRAY, SPELL_FOCI_FIRE }	
	);
	spell->hide_from_ui = true;

	spellElementConstructor(SPELL_FOCI_SNOW,
		1,		// mana
		1,		// base mana
		1,		// overload
		0,		// damage
		100,	// duration
		"spell_element_foci_snow");
	spell = spellConstructor(
		SPELL_FOCI_SNOW,											// ID
		100,														// difficulty
		"spell_foci_snow",											// internal name
		// elements
		{ SPELL_ELEMENT_PROPULSION_FOCI_SPRAY, SPELL_FOCI_SNOW }	
	);
	spell->hide_from_ui = true;

	spellElementConstructor(SPELL_FOCI_NEEDLES,
		1,		// mana
		1,		// base mana
		1,		// overload
		0,		// damage
		100,	// duration
		"spell_element_foci_needles");
	spell = spellConstructor(
		SPELL_FOCI_NEEDLES,											// ID
		100,														// difficulty
		"spell_foci_needles",										// internal name
		// elements
		{ SPELL_ELEMENT_PROPULSION_FOCI_SPRAY, SPELL_FOCI_NEEDLES } 
	);
	spell->hide_from_ui = true;

	spellElementConstructor(SPELL_FOCI_ARCS,
		1,		// mana
		1,		// base mana
		1,		// overload
		0,		// damage
		100,	// duration
		"spell_element_foci_arcs");
	spell = spellConstructor(
		SPELL_FOCI_ARCS,											// ID
		100,														// difficulty
		"spell_foci_arcs",											// internal name
		// elements
		{ SPELL_ELEMENT_PROPULSION_FOCI_SPRAY, SPELL_FOCI_ARCS }	
	);
	spell->hide_from_ui = true;

	spellElementConstructor(SPELL_FOCI_SANDBLAST,
		1,		// mana
		1,		// base mana
		1,		// overload
		0,		// damage
		100,	// duration
		"spell_element_foci_sandblast");
	spell = spellConstructor(
		SPELL_FOCI_SANDBLAST,										// ID
		100,														// difficulty
		"spell_foci_sandblast",										// internal name
		// elements
		{ SPELL_ELEMENT_PROPULSION_FOCI_SPRAY, SPELL_FOCI_SANDBLAST } 
	);
	spell->hide_from_ui = true;

	spellElementConstructor(SPELL_ELEMENT_METEOR_FLAMES,
		2,		// mana
		2,		// base mana
		1,		// overload
		2,		// damage
		0,		// duration
		"spell_element_flames");

	spellElementConstructor(SPELL_METEOR,
		1,		// mana
		1,		// base mana
		1,		// overload
		5,		// damage
		0,		// duration
		"spell_element_fireball");
	spell = spellConstructor(
		SPELL_METEOR,										// ID
		100,												// difficulty
		"spell_meteor",										// internal name
		// elements
		{ SPELL_ELEMENT_PROPULSION_MISSILE, SPELL_METEOR, SPELL_ELEMENT_METEOR_FLAMES, SPELL_ELEMENT_METEOR_FLAMES, SPELL_ELEMENT_METEOR_FLAMES }
	);
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH_FLOOR_TILE;
	spell->distance = 64.0;

	spellElementConstructor(SPELL_METEOR_SHOWER,
		1,		// mana
		1,		// base mana
		1,		// overload
		10,		// damage
		0,		// duration
		"spell_element_fireball");
	spell = spellConstructor(
		SPELL_METEOR_SHOWER,										// ID
		100,												// difficulty
		"spell_meteor_shower",										// internal name
		// elements
		{ SPELL_ELEMENT_PROPULSION_MISSILE, SPELL_ELEMENT_METEOR_FLAMES, SPELL_METEOR, SPELL_ELEMENT_METEOR_FLAMES, SPELL_METEOR, SPELL_ELEMENT_METEOR_FLAMES, SPELL_METEOR_SHOWER }
	);
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH_FLOOR_TILE;
	spell->distance = 64.0;

	spellElementConstructor(SPELL_ELEMENT_SPRITE_FLAMES,
		2,		// mana
		2,		// base mana
		1,		// overload
		2,		// damage
		0,		// duration
		"spell_element_flames");
	spell = spellConstructor(
		SPELL_FLAMES,										// ID
		100,												// difficulty
		"spell_flames",										// internal name
		// elements
		{ SPELL_ELEMENT_PROPULSION_MISSILE, SPELL_ELEMENT_SPRITE_FLAMES }
	);

	spellElementConstructor(SPELL_ELEMENT_PROPULSION_FLOOR_TILE,
		1,		// mana
		1,		// base mana
		1,		// overload
		0,		// damage
		50,	// duration
		"spell_element_propulsion_floor_tile");
	//spellElementConstructor(SPELL_ICE_WAVE,
	//	5,		// mana
	//	5,		// base mana
	//	1,		// overload
	//	0,		// damage
	//	50,	// duration
	//	"spell_element_ice_wave");

	//spell = spellConstructor(
	//	SPELL_ICE_WAVE,										// ID
	//	100,												// difficulty
	//	"spell_ice_wave",										// internal name
	//	// elements
	//	{ SPELL_ELEMENT_PROPULSION_FLOOR_TILE, SPELL_ICE_WAVE, SPELL_ICE_WAVE, SPELL_ICE_WAVE, SPELL_ICE_WAVE, SPELL_ICE_WAVE, SPELL_ICE_WAVE, SPELL_ICE_WAVE, SPELL_ICE_WAVE, SPELL_ICE_WAVE }
	//);
	//spell->hide_from_ui = true;
	//spell->rangefinder = SpellRangefinderType::RANGEFINDER_TARGET;
	//spell->distance = 16.0;

	spell = createSimpleSpell(
		SPELL_CONJURE_FOOD,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_conjure_food");

	spell = createSimpleSpell(
		SPELL_NULL_MELEE,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_null_melee");

	spell = createSimpleSpell(
		SPELL_NULL_MAGIC,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_null_magic");

	spell = createSimpleSpell(
		SPELL_NULL_RANGED,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_null_ranged");

	spell = createSimpleSpell(
		SPELL_PROF_NIMBLENESS,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_prof_nimbleness");

	spell = createSimpleSpell(
		SPELL_PROF_GREATER_MIGHT,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_prof_greater_might");

	spell = createSimpleSpell(
		SPELL_PROF_COUNSEL,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_prof_counsel");

	spell = createSimpleSpell(
		SPELL_PROF_STURDINESS,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_prof_sturdiness");

	spell = createSimpleSpell(
		SPELL_BLESS_FOOD,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_bless_food");

	spell = createSimpleSpell(
		SPELL_PINPOINT,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_pinpoint");

	spell = createSimpleSpell(
		SPELL_DONATION,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_donation");

	spell = createSimpleSpell(
		SPELL_SCRY_ALLIES,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_scry_allies");

	spell = createSimpleSpell(
		SPELL_SCRY_SHRINES,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_scry_shrines");

	spell = createSimpleSpell(
		SPELL_SCRY_TRAPS,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_scry_traps");

	spell = createSimpleSpell(
		SPELL_SCRY_TREASURES,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_scry_treasures");

	spell = createSimpleSpell(
		SPELL_PENANCE,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_penance");

	spell = createSimpleSpell(
		SPELL_CALL_ALLIES,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_call_allies");

	spell = createSimpleSpell(
		SPELL_SACRED_PATH,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_sacred_path");

	spell = createSimpleSpell(
		SPELL_MANIFEST_DESTINY,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_manifest_destiny");

	spell = createSimpleSpell(
		SPELL_DETECT_ENEMY,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_detect_enemy");

	spell = createSimpleSpell(
		SPELL_TURN_UNDEAD,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_turn_undead");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_HEAL_OTHER,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		5, // damage
		1, // duration
		"spell_heal_other");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_BLOOD_WARD,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		50, // duration
		"spell_blood_ward",
		true);

	spell = createSimpleSpell(
		SPELL_TRUE_BLOOD,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		50, // duration
		"spell_true_blood",
		true);

	spell = createSimpleSpell(
		SPELL_DIVINE_ZEAL,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		50, // duration
		"spell_divine_zeal",
		true);

	spell = createSimpleSpell(
		SPELL_ALTER_INSTRUMENT,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_alter_instrument",
		true);

	spell = createSimpleSpell(
		SPELL_MAXIMISE,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_maximise");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_MINIMISE,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		50, // duration
		"spell_minimise");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_JUMP,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_jump");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH_FLOOR_TILE;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_INCOHERENCE,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		50, // duration
		"spell_incoherence");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_OVERCHARGE,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		50, // duration
		"spell_overcharge",
		true);

	spell = createSimpleSpell(
		SPELL_ENVENOM_WEAPON,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		50, // duration
		"spell_envenom_weapon",
		true);

	spellElementConstructor(SPELL_HUMILIATE,
		1,		// mana
		1,		// base mana
		1,		// overload
		50,		// damage
		0,		// duration
		"spell_element_humiliate");
	spell = spellConstructor(
		SPELL_HUMILIATE,										// ID
		100,												// difficulty
		"spell_humiliate",										// internal name
		// elements
		{ SPELL_ELEMENT_PROPULSION_MISSILE, SPELL_HUMILIATE }
	);

	spellElementConstructor(SPELL_LVL_DEATH,
		1,		// mana
		1,		// base mana
		1,		// overload
		5,		// damage
		0,		// duration
		"spell_element_lvl_death");
	spell = spellConstructor(
		SPELL_LVL_DEATH,										// ID
		100,												// difficulty
		"spell_lvl_death",										// internal name
		// elements
		{ SPELL_ELEMENT_PROPULSION_MISSILE, SPELL_LVL_DEATH }
	);

	spellElementConstructor(SPELL_ELEMENT_PROPULSION_MAGIC_SPRAY,
		1,		// mana
		1,		// base mana
		1,		// overload
		0,		// damage
		100,	// duration
		"spell_element_propulsion_magic_spray");
	spellElementConstructor(SPELL_GREASE_SPRAY,
		1,		// mana
		1,		// base mana
		1,		// overload
		0,		// damage
		50,	// duration
		"spell_element_grease_spray");
	spell = spellConstructor(
		SPELL_GREASE_SPRAY,										// ID
		100,												// difficulty
		"spell_grease_spray",										// internal name
		// elements
		{ SPELL_ELEMENT_PROPULSION_MAGIC_SPRAY, SPELL_GREASE_SPRAY }
	);

	spellElementConstructor(SPELL_MANA_BURST,
		1,		// mana
		1,		// base mana
		1,		// overload
		0,		// damage
		0,		// duration
		"spell_element_mana_burst");
	spell = spellConstructor(
		SPELL_MANA_BURST,										// ID
		100,												// difficulty
		"spell_mana_burst",										// internal name
		// elements
		{ SPELL_ELEMENT_PROPULSION_MISSILE, SPELL_MANA_BURST }
	);

	spell = createSimpleSpell(
		SPELL_BOOBY_TRAP,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		25, // damage
		50, // duration
		"spell_booby_trap");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_COMMAND,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		25, // damage
		50, // duration
		"spell_command");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_METALLURGY,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_metallurgy");

	spell = createSimpleSpell(
		SPELL_GEOMANCY,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_geomancy");

	spell = createSimpleSpell(
		SPELL_FORGE_KEY,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_forge_key");

	spell = createSimpleSpell(
		SPELL_FORGE_JEWEL,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_forge_jewel");

	spell = createSimpleSpell(
		SPELL_ENHANCE_WEAPON,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_enhance_weapon");

	spell = createSimpleSpell(
		SPELL_RESHAPE_WEAPON,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_reshape_weapon");

	spell = createSimpleSpell(
		SPELL_ALTER_ARROW,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_alter_arrow");

	spell = createSimpleSpell(
		SPELL_VOID_CHEST,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		25, // damage
		50, // duration
		"spell_void_chest");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_PUNCTURE_VOID,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_puncture_void");

	spell = createSimpleSpell(
		SPELL_CURSE_FLESH,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_curse_flesh");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_REVENANT_CURSE,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_revenant_curse");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_SPIRIT_WEAPON,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		2, // damage
		1, // duration
		"spell_spirit_weapon");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH_FLOOR_TILE;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_ADORCISM,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_adorcism");

	spellElementConstructor(SPELL_LEAD_BOLT,
		0,		// mana
		0,		// base mana
		1,		// overload
		5,		// damage
		0,		// duration
		"spell_element_lead_bolt");
	spell = spellConstructor(
		SPELL_LEAD_BOLT,										// ID
		100,												// difficulty
		"spell_lead_bolt",										// internal name
		// elements
		{ SPELL_ELEMENT_PROPULSION_MISSILE_NOCOST, SPELL_LEAD_BOLT }
	);

	spellElementConstructor(SPELL_MERCURY_BOLT,
		10,		// mana
		10,		// base mana
		1,		// overload
		10,		// damage
		0,		// duration
		"spell_element_mercury_bolt");
	spell = spellConstructor(
		SPELL_MERCURY_BOLT,										// ID
		100,												// difficulty
		"spell_mercury_bolt",										// internal name
		// elements
		{ SPELL_ELEMENT_PROPULSION_MISSILE_NOCOST, SPELL_MERCURY_BOLT }
	);

	spellElementConstructor(SPELL_NUMBING_BOLT,
		1,		// mana
		1,		// base mana
		1,		// overload
		0,		// damage
		0,		// duration
		"spell_element_numbing_bolt");
	spell = spellConstructor(
		SPELL_NUMBING_BOLT,										// ID
		100,												// difficulty
		"spell_numbing_bolt",										// internal name
		// elements
		{ SPELL_ELEMENT_PROPULSION_MISSILE, SPELL_NUMBING_BOLT }
	);

	spell = createSimpleSpell(
		SPELL_DELAY_PAIN,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_delay_pain");

	spell = createSimpleSpell(
		SPELL_SEEK_ALLY,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_seek_ally");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_SEEK_FOE,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_seek_foe");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_TABOO,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_taboo");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_COURAGE,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_courage");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_COWARDICE,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_cowardice");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_DEEP_SHADE,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		750, // duration
		"spell_deep_shade",
		true);

	spellElementConstructor(SPELL_SHADE_BOLT,
		1,		// mana
		1,		// base mana
		1,		// overload
		0,		// damage
		0,		// duration
		"spell_element_shade_bolt");
	spell = spellConstructor(
		SPELL_SHADE_BOLT,										// ID
		100,												// difficulty
		"spell_shade_bolt",										// internal name
		// elements
		{ SPELL_ELEMENT_PROPULSION_MISSILE, SPELL_SHADE_BOLT }
	);

	spellElementConstructor(SPELL_WONDERLIGHT,
		1,		// mana
		1,		// base mana
		1,		// overload
		0,		// damage
		0,		// duration
		"spell_element_wonderlight");
	spell = spellConstructor(
		SPELL_WONDERLIGHT,										// ID
		100,												// difficulty
		"spell_wonderlight",										// internal name
		// elements
		{ SPELL_ELEMENT_PROPULSION_MISSILE, SPELL_WONDERLIGHT }
	);

	spell = createSimpleSpell(
		SPELL_SPORES,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		750, // duration
		"spell_spores",
		true);

	spellElementConstructor(SPELL_SPORE_BOMB,
		1,		// mana
		1,		// base mana
		1,		// overload
		5,		// damage
		0,		// duration
		"spell_element_spore_bomb");
	spell = spellConstructor(
		SPELL_SPORE_BOMB,										// ID
		100,												// difficulty
		"spell_spore_bomb",										// internal name
		// elements
		{ SPELL_ELEMENT_PROPULSION_MISSILE, SPELL_SPORE_BOMB }
	);

	spell = createSimpleSpell(
		SPELL_WINDGATE,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_windgate");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH_WALL_TILE;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_TELEKINESIS,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_telekinesis");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH_INTERACT;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_KINETIC_PUSH,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_kinetic_push");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_DISARM,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_disarm");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_STRIP,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_strip");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_ABUNDANCE,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		750, // duration
		"spell_abundance",
		true);

	spell = createSimpleSpell(
		SPELL_GREATER_ABUNDANCE,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		750, // duration
		"spell_greater_abundance",
		true);

	spell = createSimpleSpell(
		SPELL_PRESERVE,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		750, // duration
		"spell_preserve",
		true);
	spell->sustainEffectDissipate = EFF_PRESERVE;

	spell = createSimpleSpell(
		SPELL_RESTORE,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		750, // duration
		"spell_restore");

	spell = createSimpleSpell(
		SPELL_SABOTAGE,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		50, // damage
		1, // duration
		"spell_sabotage");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_HARVEST_TRAP,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_harvest_trap");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_MIST_FORM,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		750, // duration
		"spell_mist_form",
		true);
	spell->sustainEffectDissipate = EFF_MIST_FORM;

	spell = createSimpleSpell(
		SPELL_HOLOGRAM,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_hologram");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH_FLOOR_TILE;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_FORCE_SHIELD,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		750, // duration
		"spell_force_shield");

	spell = createSimpleSpell(
		SPELL_REFLECTOR,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		750, // duration
		"spell_reflector");

	spell = createSimpleSpell(
		SPELL_SPLINTER_GEAR,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		25, // damage
		1, // duration
		"spell_splinter_gear");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_LIGHTEN_LOAD,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		750, // duration
		"spell_lighten_load",
		true);
	spell->sustainEffectDissipate = EFF_LIGHTEN_LOAD;

	spell = createSimpleSpell(
		SPELL_ATTRACT_ITEMS,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		750, // duration
		"spell_attract_items",
		true);
	spell->sustainEffectDissipate = EFF_ATTRACT_ITEMS;

	spell = createSimpleSpell(
		SPELL_RETURN_ITEMS,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		750, // duration
		"spell_return_items",
		true);
	spell->sustainEffectDissipate = EFF_RETURN_ITEM;

	spell = createSimpleSpell(
		SPELL_DEFACE,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_deface");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_DEMESNE_DOOR,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		25, // damage
		1, // duration
		"spell_demesne_door");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_TUNNEL,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_tunnel");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH_WALL_TILE;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_NULL_AREA,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_null_area");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH_FLOOR_TILE;
	spell->distance = 64.0;

	spellElementConstructor(SPELL_SPHERE_SILENCE,
		1,		// mana
		1,		// base mana
		1,		// overload
		0,		// damage
		0,		// duration
		"spell_element_sphere_silence");
	spell = spellConstructor(
		SPELL_SPHERE_SILENCE,										// ID
		100,												// difficulty
		"spell_sphere_silence",										// internal name
		// elements
		{ SPELL_ELEMENT_PROPULSION_MISSILE, SPELL_SPHERE_SILENCE }
	);

	spell = createSimpleSpell(
		SPELL_FORGE_METAL_SCRAP,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_forge_metal_scrap");

	spell = createSimpleSpell(
		SPELL_FORGE_MAGIC_SCRAP,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_forge_magic_scrap");

	spell = createSimpleSpell(
		SPELL_FIRE_SPRITE,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_fire_sprite");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH_FLOOR_TILE;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_FLAME_ELEMENTAL,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_flame_elemental");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH_FLOOR_TILE;
	spell->distance = 64.0;

	spellElementConstructor(SPELL_SPIN,
		1,		// mana
		1,		// base mana
		1,		// overload
		0,		// damage
		0,		// duration
		"spell_element_spin");
	spell = spellConstructor(
		SPELL_SPIN,										// ID
		100,												// difficulty
		"spell_spin",										// internal name
		// elements
		{ SPELL_ELEMENT_PROPULSION_MISSILE, SPELL_SPIN }
	);

	spellElementConstructor(SPELL_DIZZY,
		1,		// mana
		1,		// base mana
		1,		// overload
		0,		// damage
		0,		// duration
		"spell_element_dizzy");
	spell = spellConstructor(
		SPELL_DIZZY,										// ID
		100,												// difficulty
		"spell_dizzy",										// internal name
		// elements
		{ SPELL_ELEMENT_PROPULSION_MISSILE, SPELL_DIZZY }
	);

	spell = createSimpleSpell(
		SPELL_VANDALISE,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_vandalise");

	spell = createSimpleSpell(
		SPELL_DESECRATE,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_desecrate");

	spell = createSimpleSpell(
		SPELL_SANCTIFY,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_sanctify");

	spell = createSimpleSpell(
		SPELL_SANCTIFY_WATER,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_sanctify_water");

	spell = createSimpleSpell(
		SPELL_CLEANSE_FOOD,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_cleanse_food");

	spell = createSimpleSpell(
		SPELL_ADORCISE_INSTRUMENT,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_adorcise_instrument");

	spell = createSimpleSpell(
		SPELL_CRITICAL_SPELL,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_critical_spell");

	spell = createSimpleSpell(
		SPELL_MAGIC_WELL,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_magic_well");

	spell = createSimpleSpell(
		SPELL_FLAME_CLOAK,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_flame_cloak");

	spell = createSimpleSpell(
		SPELL_FLAME_SHIELD,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_flame_shield");

	spell = createSimpleSpell(
		SPELL_ABSORB_MAGIC,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_absorb_magic");

	spell = createSimpleSpell(
		SPELL_LIGHTNING_BOLT,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		20, // damage
		1, // duration
		"spell_lightning_bolt");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH_FLOOR_TILE;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_DISRUPT_EARTH,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		5, // damage
		1, // duration
		"spell_disrupt_earth");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH_FLOOR_TILE;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_EARTH_SPINES,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_earth_spines");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH_FLOOR_TILE;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_FIRE_WALL,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		5, // damage
		1, // duration
		"spell_fire_wall");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH_FLOOR_TILE;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_ICE_WAVE,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		5, // damage
		1, // duration
		"spell_ice_wave");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH_FLOOR_TILE;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_SLAM,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		5, // damage
		1, // duration
		"spell_slam");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH_FLOOR_TILE;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_KINETIC_FIELD,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		5, // damage
		1, // duration
		"spell_kinetic_field");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH_FLOOR_TILE;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_CHRONOMIC_FIELD,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		5, // damage
		1, // duration
		"spell_chronomic_field");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH_FLOOR_TILE;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_ETERNALS_GAZE,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		12, // damage
		1, // duration
		"spell_eternals_gaze");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH_FLOOR_TILE;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_SHATTER_EARTH,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		20, // damage
		1, // duration
		"spell_shatter_earth");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH_FLOOR_TILE;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_EARTH_ELEMENTAL,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		20, // damage
		1, // duration
		"spell_earth_elemental");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH_FLOOR_TILE;
	spell->distance = 64.0;

	spell = createSimpleSpell(
		SPELL_ROOTS,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		5, // damage
		1, // duration
		"spell_roots");

	spell = createSimpleSpell(
		SPELL_MUSHROOM,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		1, // duration
		"spell_mushroom");
	spell->rangefinder = SpellRangefinderType::RANGEFINDER_TOUCH_FLOOR_TILE;
	spell->distance = 64.0;

	spellElementConstructor(SPELL_MYCELIUM_BOMB,
		1,		// mana
		1,		// base mana
		1,		// overload
		5,		// damage
		0,		// duration
		"spell_element_mycelium_bomb");
	spell = spellConstructor(
		SPELL_MYCELIUM_BOMB,										// ID
		100,												// difficulty
		"spell_mycelium_bomb",										// internal name
		// elements
		{ SPELL_ELEMENT_PROPULSION_MISSILE, SPELL_MYCELIUM_BOMB }
	);

	spell = createSimpleSpell(
		SPELL_MYCELIUM_SPORES,
		100, // difficulty
		1, // mana
		1, // base mana
		1, // overload
		0, // damage
		750, // duration
		"spell_mycelium_spores",
		true);

	//static const int SPELL_LIGHTNING_NEXUS = 182;
	//static const int SPELL_LIFT = 184;
	//static const int SPELL_SLAM = 185;
	//static const int SPELL_IGNITE = 186;
	//static const int SPELL_SHATTER_OBJECTS = 187;
	//static const int SPELL_KINETIC_FIELD = 188;
	//static const int SPELL_ICE_BLOCK = 189;
}

spell_t* createSimpleSpell(int spellID, int difficulty, int mana, int base_mana, int overload_mult, int damage, 
	int duration, const char* internal_name, bool sustained)
{
	std::string elementName = internal_name;
	if ( elementName.find("element_") == std::string::npos )
	{
		auto find = elementName.find("spell_");
		if ( find != std::string::npos )
		{
			elementName.insert(find + strlen("spell_"), "element_");
		}
	}
	spellElementConstructor(spellID,
		mana,		// mana
		base_mana,		// base mana
		overload_mult,		// overload
		damage,		// damage
		duration,		// duration
		elementName.c_str());
	spellElementMap[spellID].channeled = sustained;
	spell_t* spell = spellConstructor(
		// ID
		spellID,
		// difficulty
		difficulty,
		// internal name
		internal_name,
		// elements
		{ spellID }
	);
	return spell;
}