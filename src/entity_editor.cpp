/*-------------------------------------------------------------------------------

	BARONY
	File: entity_editor.cpp
	Desc: implements a dummy entity class so the editor can compile
	Yes, this file is an utter bodge. If a better solution can be found, great!

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "entity.hpp"



Entity::Entity(Sint32 sprite, Uint32 pos, list_t *entlist) :
	char_gonnavomit(skill[26]),
	char_heal(skill[22]),
	char_energize(skill[23]),
	char_torchtime(skill[25]),
	char_poison(skill[21]),
	monster_attack(skill[8]),
	monster_attacktime(skill[9]),
	monster_state(skill[0]),
	monster_target(skill[1]),
	circuit_status(skill[28]),
	switch_power(skill[0]),
	chest_status(skill[1]),
	chest_opener(skill[5])
{ }

Entity::~Entity() { }