/*-------------------------------------------------------------------------------

	BARONY
	File: mechanism.cpp
	Desc: implements all mechanism related code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "sound.hpp"
#include "net.hpp"
#include "player.hpp"

//Circuits do not overlap. They connect to all their neighbors, allowing for circuits to interfere with eachother.

void actCircuit(Entity *my) {
	my->flags[PASSABLE] = TRUE; // these should ALWAYS be passable. No exceptions
}

void Entity::circuitPowerOn() {
	if (behavior == actCircuit && circuit_status && circuit_status != CIRCUIT_ON) {
		circuit_status = CIRCUIT_ON; //On.
		//TODO: Play a sound effect?

		updateCircuitNeighbors(); //This'll power on all neighbors that are unpowered.
	}
}

void Entity::circuitPowerOff() {
	if (behavior == actCircuit && circuit_status != CIRCUIT_OFF) {
		circuit_status = CIRCUIT_OFF; //Off.
		//TODO: Play a sound effect?

		updateCircuitNeighbors(); //Send the poweroff signal to all neighbors.
	}
}

void Entity::updateCircuitNeighbors() {
	//Send the power on or off signal to all neighboring circuits & mechanisms.
	list_t *neighbors = getPowerableNeighbors(); //Grab a list of all neighboring circuits and mechanisms.

	if (neighbors) {
		node_t *node = NULL;
		for (node = neighbors->first; node != NULL; node = node->next) {
			if (node->element) {
				Entity *powerable = (Entity *)(node->element);

				if (powerable) {
					if (powerable->behavior == actCircuit)
						(circuit_status > 1) ? powerable->circuitPowerOn() : powerable->circuitPowerOff();
					else
						(circuit_status > 1) ? powerable->mechanismPowerOn() : powerable->mechanismPowerOff();
				}
			}
		}

		//Free the list.
		list_FreeAll(neighbors);
		free(neighbors);
	}
}









//I don't want an explicit mechanism entity.
//Rather, I want normal object entities, such as traps and whatever else will interact with circuits to have the variables necessary to function as a mechanism.
//Hence, no actMechanism(). Each individual entity will handle that in its own act() function.

void Entity::mechanismPowerOn() {
	if (skill)
		circuit_status = CIRCUIT_ON; //Power on.
}

void Entity::mechanismPowerOff() {
	if (skill)
		circuit_status = CIRCUIT_OFF; //Power off.
}










/*
 * skill[0] = power status.
 * * 0 = off
 * * 1 = on
 */


void actSwitch(Entity *my) {
	//TODO: If powered on, and it detects a depowered neighbor, it should pulse that neighbor to turn on.
	//Thus, this function needs to be called periodically.
	//This is so that if a switch goes off and there's another switch on the network, the entire network shuts off regardless of the other switch's status.
	//So then when that second switch's actSwitch() comes up, and if it's on, it'll repower the entire network -- which will stay powered until ALL connected switches go off.
	my->flags[PASSABLE] = TRUE; // these should ALWAYS be passable. No exceptions

	if( multiplayer!=CLIENT ) {
		int i = 0;
		for (i = 0; i < MAXPLAYERS; ++i) {
			if ( (i==0 && selectedEntity==my) || (client_selected[i]==my) ) {
				if (inrange[i]) { //Act on it only if the player (or monster, if/when this is changed to support monster interaction?) is in range.
					messagePlayer(i, language[1110]);
					playSoundEntity(my, 56, 64);
					my->toggleSwitch();
				}
			}
		}

		if (my->skill[0]) {
			//Power on any neighbors that don't have power.
			my->switchUpdateNeighbors();
			//TODO: Alternatively, instead of using CPU cycles on this, have the recursive network shutdown alert any switches connected to it that are powered on that it's shutting down, so that they can repower the network come next frame.
		}
	} else {
		my->flags[NOUPDATE] = TRUE;
	}
	
	// Rotate the switch when it is on/off.
	if( my->skill[0] ) {
		if( my->roll > -PI/4 ) {
			my->roll -= std::max((my->roll+PI/4)/2,.05);
		} else {
			my->roll = -PI/4;
		}
	} else {
		if( my->roll < PI/4 ) {
			my->roll += std::max(-(my->roll-PI/4)/2,.05);
		} else {
			my->roll = PI/4;
		}
	}

	/*
	 * TODO:
	if (SWITCH_TYPE == pressure_plate) {
		if (playerOrMonsterOnTile()) {
			toggleSwitch(my);
		} else {
			if (on) {
				toggleSwitch(my);
			}
		}
	}
	 */

}

#define TRAP_ON my->skill[0]
void actTrap(Entity *my) {
	// activates circuit when certain entities are occupying its tile
	node_t *node;
	Entity *entity;
	bool somebodyonme = FALSE;
	my->flags[PASSABLE] = TRUE; // these should ALWAYS be passable. No exceptions
	
	if( TRAP_ON ) {
		my->switchUpdateNeighbors();
	}
	
	for( node=map.entities->first; node!=NULL; node=node->next ) {
		entity = (Entity *)node->element;
		if( entity->behavior == &actPlayer || entity->behavior == &actItem || entity->behavior == &actMonster || entity->behavior == &actBoulder ) {
			if( floor(entity->x/16) == floor(my->x/16) && floor(entity->y/16) == floor(my->y/16) ) {
				somebodyonme = TRUE;
				if( !TRAP_ON ) {
					my->toggleSwitch();
					TRAP_ON = 1;
				}
			}
		}
	}
	if( !somebodyonme ) {
		if( TRAP_ON ) {
			my->toggleSwitch();
			TRAP_ON = 0;
		}
	}
}

#define TRAPPERMANENT_ON my->skill[0]
void actTrapPermanent(Entity *my) {
	// activates circuit when certain entities are occupying its tile
	// unlike actTrap, never deactivates
	node_t *node;
	Entity *entity;
	my->flags[PASSABLE] = TRUE; // these should ALWAYS be passable. No exceptions

	if( !strcmp(map.name,"Boss") ) {
		for( node=map.entities->first; node!=NULL; node=node->next ) {
			entity = (Entity *)node->element;
			if( entity->behavior==&actPlayer ) {
				if( entity->x < 26*16 || entity->y < 6*16 || entity->y >= 26*16 ) { // hardcoded, I know...
					return;
				}
			}
		}
	}
	
	if( TRAPPERMANENT_ON ) {
		my->switchUpdateNeighbors();
	} else {
		for( node=map.entities->first; node!=NULL; node=node->next ) {
			entity = (Entity *)node->element;
			if( entity->behavior == &actPlayer || entity->behavior == &actItem || entity->behavior == &actMonster || entity->behavior == &actBoulder ) {
				if( floor(entity->x/16) == floor(my->x/16) && floor(entity->y/16) == floor(my->y/16) ) {
					my->toggleSwitch();
					TRAPPERMANENT_ON = 1;
				}
			}
		}
	}
}

//This is called when the switch is toggled by the player.
void Entity::toggleSwitch() {
	//If off, power on and send poweron signal. If on, power off and send poweroff signal.

	switch_power = (switch_power == SWITCH_UNPOWERED);
	serverUpdateEntitySkill(this, 0);

	//(my->skill[0]) ? my->sprite = 171 : my->sprite = 168;

	list_t *neighbors = getPowerableNeighbors(); //Grab a list of all neighboring circuits and mechanisms.

	if (neighbors) {
		node_t *node = NULL;
		for (node = neighbors->first; node != NULL; node = node->next) {
			if (node->element) {
				Entity *powerable = (Entity *)(node->element);

				if (powerable) {
					if (powerable->behavior == actCircuit)
						(switch_power) ? powerable->circuitPowerOn() : powerable->circuitPowerOff();
					else
						(switch_power) ? powerable->mechanismPowerOn() : powerable->mechanismPowerOff();
				}
			}
		}

		list_FreeAll(neighbors); //Free the list.
		free(neighbors);
	}
}

void Entity::switchUpdateNeighbors() {
	list_t *neighbors = getPowerableNeighbors(); //Grab a list of all neighboring circuits and mechanisms.

	if (neighbors) {
		node_t *node = NULL;
		for (node = neighbors->first; node != NULL; node = node->next) {
			if (node->element) {
				Entity *powerable = (Entity *)(node->element);

				if (powerable) {
					if (powerable->circuit_status != CIRCUIT_ON) {
						if (powerable->behavior == actCircuit)
							powerable->circuitPowerOn();
						else
							powerable->mechanismPowerOn();
					}
				}
			}
		}

		list_FreeAll(neighbors); //Free the list.
		free(neighbors);
	}
}

void getPowerablesOnTile(int x, int y, list_t **list) {

	//Take the return value of checkTileForEntity() and sort that list for powerables.
	//if (entity->powerable == true)
	//And then free the list returned by checkTileForEntity.

	//Right. First, grab all the entities on the tile.
	list_t *entities = NULL; entities = checkTileForEntity(x, y);

	if (!entities) return; //No use continuing, got no entities.

	node_t *node = NULL;
	node_t *node2 = NULL;
	//Loop through the list of entities.
	for (node = entities->first; node != NULL; node = node->next) {
		if (node->element) {
			Entity *entity = (Entity *) node->element;
			//Check if the entity is powerable.
			if (entity && entity->skill[28]) { //If skill 28 = 0, the entity is not a powerable.
				//If this is the first powerable found, the list needs to be created.
				if (!(*list)) {
					*list = (list_t *) malloc(sizeof(list_t));
					(*list)->first = NULL;
					(*list)->last = NULL;
				}

				//Add the current entity to it.
				node2 = list_AddNodeLast(*list);
				node2->element = entity;
				node2->deconstructor = &emptyDeconstructor;
			}
		}
	}

	if (entities) {
		list_FreeAll(entities);
		free(entities);
	}

	//return return_val;
}

list_t* Entity::getPowerableNeighbors() {
	list_t *return_val = NULL;


	int tx = x / 16;
	int ty = y / 16;

	getPowerablesOnTile(tx, ty, &return_val); //Check current tile
	getPowerablesOnTile(tx - 1, ty, &return_val); //Check tile to the left.
	getPowerablesOnTile(tx + 1, ty, &return_val); //Check tile to the right.
	getPowerablesOnTile(tx, ty - 1, &return_val); //Check tile up.
	getPowerablesOnTile(tx, ty + 1, &return_val); //Check tile down.
	//getPowerablesOnTile(tx - 1, ty - 1, &return_val); //Check tile diagonal up left.
	//getPowerablesOnTile(tx + 1, ty - 1, &return_val); //Check tile diagonal up right.
	//getPowerablesOnTile(tx - 1, ty + 1, &return_val); //Check tile diagonal down left.
	//getPowerablesOnTile(tx + 1, ty + 1, &return_val); //Check tile diagonal down right.

	return return_val;
}
