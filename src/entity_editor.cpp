/*-------------------------------------------------------------------------------

	BARONY
	File: entity_editor.cpp
	Desc: implements a dummy entity class so the editor can compile
	Yes, this file is an utter bodge. If a better solution can be found, great!

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/
#include "entity.hpp"



Entity::Entity(Sint32 in_sprite, Uint32 pos, list_t* entlist) :
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
	CHEST_INIT(skill[0]),
	CHEST_STATUS(skill[1]),
	CHEST_HEALTH(skill[3]),
	CHEST_LOCKED(skill[4]),
	CHEST_OPENER(skill[5]),
	CHEST_LIDCLICKED(skill[6]),
	CHEST_AMBIENCE(skill[7]),
	CHEST_MAXHEALTH(skill[8]),
	CHEST_TYPE(skill[9]),
	CHEST_PREVENT_LOCKPICK_CAPSTONE_EXPLOIT(skill[10])
{
	int c;
	// add the entity to the entity list
	if (!pos)
	{
		mynode = list_AddNodeFirst(entlist);
	}
	else
	{
		mynode = list_AddNodeLast(entlist);
	}
	mynode->element = this;
	mynode->deconstructor = &entityDeconstructor;
	mynode->size = sizeof(Entity);

	// now reset all of my data elements
	lastupdate = 0;
	lastupdateserver = 0;
	ticks = 0;
	x = 0;
	y = 0;
	z = 0;
	new_x = 0;
	new_y = 0;
	new_z = 0;
	focalx = 0;
	focaly = 0;
	focalz = 0;
	scalex = 1;
	scaley = 1;
	scalez = 1;
	vel_x = 0;
	vel_y = 0;
	vel_z = 0;
	sizex = 0;
	sizey = 0;
	yaw = 0;
	pitch = 0;
	roll = 0;
	new_yaw = 0;
	new_pitch = 0;
	new_roll = 0;
	sprite = in_sprite;
	light = nullptr;
	string = nullptr;
	children.first = nullptr;
	children.last = nullptr;
	//this->magic_effects = (list_t *) malloc(sizeof(list_t));
	//this->magic_effects->first = NULL; this->magic_effects->last = NULL;
	for (c = 0; c < 30; ++c)
	{
		skill[c] = 0;
		fskill[c] = 0;
	}
	skill[2] = -1;
	for (c = 0; c < 16; ++c)
	{
		flags[c] = false;
	}
	if (entlist == map.entities)
	{
		if (multiplayer != CLIENT || loading)
		{
			uid = entity_uids;
			entity_uids++;
		}
		else
		{
			uid = -2;
		}
	}
	else
	{
		uid = -2;
	}
	behavior = nullptr;
	ranbehavior = false;
	parent = 0;
	path = nullptr;
}

Entity::~Entity() { }

Stat* Entity::getStats()
{
		if ( this->children.first != nullptr )
		{
			if ( this->children.first->next != nullptr )
			{
				return (Stat*)this->children.first->next->element;
			}
		}
	

	return nullptr;
}

