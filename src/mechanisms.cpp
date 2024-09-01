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
#include "engine/audio/sound.hpp"
#include "net.hpp"
#include "player.hpp"
#include "scores.hpp"
#include "mod_tools.hpp"

//Circuits do not overlap. They connect to all their neighbors, allowing for circuits to interfere with eachother.
static ConsoleVariable<bool> cvar_wire_debug("/wire_debug", false);
void signalGateANDOnReceive(Entity& gate, const bool powered, const int receivex, const int receivey);

void actCircuit(Entity* my)
{
	my->flags[PASSABLE] = true; // these should ALWAYS be passable. No exceptions
	if ( (svFlags & SV_FLAG_CHEATS) && *cvar_wire_debug )
	{
		my->flags[INVISIBLE] = false;
		my->sprite = 170;
	}
	else
	{
		my->sprite = 18;
		my->flags[INVISIBLE] = true;
	}
}

void Entity::circuitPowerOn()
{
	if (behavior == actCircuit && circuit_status && circuit_status != CIRCUIT_ON)
	{
		circuit_status = CIRCUIT_ON; //On.
		//TODO: Play a sound effect?

		updateCircuitNeighbors(); //This'll power on all neighbors that are unpowered.
	}
}

void Entity::circuitPowerOff()
{
	if (behavior == actCircuit && circuit_status != CIRCUIT_OFF)
	{
		circuit_status = CIRCUIT_OFF; //Off.
		//TODO: Play a sound effect?

		updateCircuitNeighbors(); //Send the poweroff signal to all neighbors.
	}
}

void Entity::updateCircuitNeighbors()
{
	//Send the power on or off signal to all neighboring circuits & mechanisms.
	list_t* neighbors = getPowerableNeighbors(); //Grab a list of all neighboring circuits and mechanisms.

	if (neighbors)
	{
		node_t* node = NULL;
		for (node = neighbors->first; node != NULL; node = node->next)
		{
			if (node->element)
			{
				Entity* powerable = (Entity*)(node->element);

				if (powerable)
				{
					if (powerable->behavior == actCircuit)
					{
						(circuit_status > 1) ? powerable->circuitPowerOn() : powerable->circuitPowerOff();
					}
					else if ( powerable->behavior == &::actSignalTimer )
					{
						int x1 = static_cast<int>(this->x / 16);
						int x2 = static_cast<int>(powerable->x / 16);
						int y1 = static_cast<int>(this->y / 16);
						int y2 = static_cast<int>(powerable->y / 16);
						//messagePlayer(0, "%d, %d, %d, %d", x1, x2, y1, y2);
						switch ( powerable->signalInputDirection )
						{
							case 0: // west
								if ( (x1 + 1) == x2 )
								{
									(circuit_status > 1) ? powerable->mechanismPowerOn() : powerable->mechanismPowerOff();
								}
								break;
							case 1: // south
								if ( (y1 - 1) == y2 )
								{
									(circuit_status > 1) ? powerable->mechanismPowerOn() : powerable->mechanismPowerOff();
								}
								break;
							case 2: // east
								if ( (x1 - 1) == x2 )
								{
									(circuit_status > 1) ? powerable->mechanismPowerOn() : powerable->mechanismPowerOff();
								}
								break;
							case 3: // north
								if ( (y1 + 1) == y2 )
								{
									(circuit_status > 1) ? powerable->mechanismPowerOn() : powerable->mechanismPowerOff();
								}
								break;
							default:
								break;
						}
					}
					else if ( powerable->behavior == &::actSignalGateAND )
					{
						int x1 = static_cast<int>(this->x / 16);
						int y1 = static_cast<int>(this->y / 16);
						//messagePlayer(0, "%d, %d, %d, %d", x1, x2, y1, y2);
						signalGateANDOnReceive(*powerable, circuit_status > 1, x1, y1);
					}
					else
					{
						(circuit_status > 1) ? powerable->mechanismPowerOn() : powerable->mechanismPowerOff();
					}
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

void Entity::mechanismPowerOn()
{
	//if (skill)
	{
		circuit_status = CIRCUIT_ON;    //Power on.
	}
}

void Entity::mechanismPowerOff()
{
	//if (skill)
	{
		circuit_status = CIRCUIT_OFF;    //Power off.
	}
}


/*
 * skill[0] = power status.
 * * 0 = off
 * * 1 = on
 */


void actSwitch(Entity* my)
{
	//TODO: If powered on, and it detects a depowered neighbor, it should pulse that neighbor to turn on.
	//Thus, this function needs to be called periodically.
	//This is so that if a switch goes off and there's another switch on the network, the entire network shuts off regardless of the other switch's status.
	//So then when that second switch's actSwitch() comes up, and if it's on, it'll repower the entire network -- which will stay powered until ALL connected switches go off.
	my->flags[PASSABLE] = true; // these should ALWAYS be passable. No exceptions

	if ( my->ticks == 1 )
	{
		my->createWorldUITooltip();
	}

	if ( multiplayer != CLIENT )
	{
		for (int i = 0; i < MAXPLAYERS; ++i)
		{
			if ( selectedEntity[i] == my || client_selected[i] == my )
			{
				if (inrange[i])   //Act on it only if the player (or monster, if/when this is changed to support monster interaction?) is in range.
				{
					messagePlayer(i, MESSAGE_INTERACTION, Language::get(1110));
					playSoundEntity(my, 56, 64);
					my->toggleSwitch();
					Compendium_t::Events_t::eventUpdateWorld(i, Compendium_t::CPDM_LEVER_PULLED, "lever", 1);
				}
			}
		}
		if ( my->isInteractWithMonster() )
		{
			Entity* monsterInteracting = uidToEntity(my->interactedByMonster);
			if ( monsterInteracting && monsterInteracting->getMonsterTypeFromSprite() == GYROBOT )
			{
				Entity* leader = monsterInteracting->monsterAllyGetPlayerLeader();
				if ( leader )
				{
					achievementObserver.playerAchievements[monsterInteracting->monsterAllyIndex].checkPathBetweenObjects(leader, my, AchievementObserver::BARONY_ACH_LEVITANT_LACKEY);
					Compendium_t::Events_t::eventUpdateWorld(monsterInteracting->monsterAllyIndex, Compendium_t::CPDM_LEVER_FOLLOWER_PULLED, "lever", 1);
				}
			}
			my->toggleSwitch();
			my->clearMonsterInteract();
		}

		if (my->skill[0])
		{
			//Power on any neighbors that don't have power.
			my->switchUpdateNeighbors();
			//TODO: Alternatively, instead of using CPU cycles on this, have the recursive network shutdown alert any switches connected to it that are powered on that it's shutting down, so that they can repower the network come next frame.
		}
	}
	else
	{
		my->flags[NOUPDATE] = true;
	}

	// Rotate the switch when it is on/off.
	if ( my->skill[0] )
	{
		if ( my->roll > -PI / 4 )
		{
			my->roll -= std::max<real_t>((my->roll + PI / 4) / 2, .05);
		}
		else
		{
			my->roll = -PI / 4;
		}
	}
	else
	{
		if ( my->roll < PI / 4 )
		{
			my->roll += std::max<real_t>(-(my->roll - PI / 4) / 2, .05);
		}
		else
		{
			my->roll = PI / 4;
		}
	}
}

void actSwitchWithTimer(Entity* my)
{
	my->flags[PASSABLE] = true; // these should ALWAYS be passable. No exceptions

	if ( my->ticks == 1 )
	{
		my->createWorldUITooltip();
	}

	if ( multiplayer != CLIENT )
	{
		int i = 0;
		for ( i = 0; i < MAXPLAYERS; ++i )
		{
			if ( selectedEntity[i] == my || client_selected[i] == my )
			{
				// server/client has clicked on the entity.
				if ( inrange[i] )   //Act on it only if the player (or monster, if/when this is changed to support monster interaction?) is in range.
				{
					switch ( my->leverStatus )
					{
						case 0:
							messagePlayer(i, MESSAGE_INTERACTION, Language::get(2360));
							break;
						case 1:
							messagePlayer(i, MESSAGE_INTERACTION, Language::get(2361));
							break;
						case 2:
							messagePlayer(i, MESSAGE_INTERACTION, Language::get(2362));
							break;
						default:
							messagePlayer(i, MESSAGE_INTERACTION, Language::get(2363));
							break;
					}

					if ( my->leverStatus < 3 )
					{
						++my->leverStatus;
						playSoundEntity(my, 248, 64);
						serverUpdateEntitySkill(my, 1);
						if ( my->leverStatus == 3 )
						{
							playSoundEntity(my, 56, 64);
							my->toggleSwitch();
						}
					}
				}
			}
		}

		if ( my->leverStatus == 4 )
		{
			//Power on any neighbors that don't have power.
			my->switchUpdateNeighbors();
			//TODO: Alternatively, instead of using CPU cycles on this, have the recursive network shutdown alert any switches connected to it that are powered on that it's shutting down, so that they can repower the network come next frame.
		}
	}
	else
	{
		my->flags[NOUPDATE] = true;
	}

	// Rotate the switch when it is on/off.
	if ( my->leverStatus == 0 )
	{
		if ( my->roll > -PI / 4 )
		{
			my->roll -= std::max<real_t>((my->roll + PI / 4) / 2, .05);
		}
		else
		{
			my->roll = -PI / 4;
		}
	}
	else if (my->leverStatus == 1 ) // 1/3 of the way up
	{
		if ( my->roll < -PI / 12 )
		{
			my->roll += std::max<real_t>(-(my->roll + PI / 12) / 8, .02);
		}
		else
		{
			my->roll = -PI / 12;
		}
	}
	else if ( my->leverStatus == 2 ) // 2/3 of the way up
	{
		if ( my->roll < PI / 12 )
		{
			my->roll += std::max<real_t>(-(my->roll - PI / 12) / 8, .02);
		}
		else
		{
			my->roll = PI / 12;
		}
	}
	else if ( my->leverStatus == 3 ) // all the way up
	{
		if ( my->roll < PI / 4 )
		{
			my->roll += std::max<real_t>(-(my->roll - PI / 4) / 4, .02);
		}
		else
		{
			my->roll = PI / 4;
			if ( multiplayer != CLIENT )
			{
				my->leverStatus = 4;
				serverUpdateEntitySkill(my, 1);
			}
		}
	}
	else if ( my->leverStatus == 4 ) // ticking down
	{
		if ( my->roll > -PI / 12 )
		{
			my->roll -= (PI / 3) / static_cast<real_t>(my->leverTimerTicks); // move slowly towards 2/3rds of the resting point
			if ( my->ticks % 10 == 0 )
			{
				playSoundEntityLocal(my, 247, 32);
			}
		}
		else
		{
			my->roll = -PI / 12;
			if ( multiplayer != CLIENT )
			{
				playSoundEntity(my, 56, 64);
				my->leverStatus = 0;
				serverUpdateEntitySkill(my, 1);
				my->toggleSwitch();
			}
		}
	}
}

#define TRAP_ON my->skill[0]
void actTrap(Entity* my)
{
	if ( (svFlags & SV_FLAG_CHEATS) && *cvar_wire_debug )
	{
		my->flags[INVISIBLE] = false;
	}
	else
	{
		my->flags[INVISIBLE] = true;
	}

	// activates circuit when certain entities are occupying its tile
	node_t* node;
	Entity* entity;
	bool somebodyonme = false;
	my->flags[PASSABLE] = true; // these should ALWAYS be passable. No exceptions

	if ( TRAP_ON )
	{
		my->switchUpdateNeighbors();
	}

	std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 2);
	for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end() && !somebodyonme; ++it )
	{
		list_t* currentList = *it;
		for ( node = currentList->first; node != nullptr; node = node->next )
		{
			entity = (Entity*)node->element;
			if ( entity->behavior == &actItem && entity->flags[INVISIBLE] )
			{
				continue;
			}
			if ( entity->behavior == &actPlayer || entity->behavior == &actItem 
				|| (entity->behavior == &actMonster && !entity->isInertMimic()) || entity->behavior == &actBoulder
				|| entity->behavior == &actBomb || entity->behavior == &actDecoyBox )
			{
				if ( floor(entity->x / 16) == floor(my->x / 16) && floor(entity->y / 16) == floor(my->y / 16) )
				{
					switch ( my->pressurePlateTriggerType )
					{
					default:
					case Entity::PRESSURE_PLATE_DEFAULT_ALL:
						break;
					case Entity::PRESSURE_PLATE_PLAYERS:
						if ( entity->behavior != &actPlayer )
						{
							continue;
						}
						break;
					case Entity::PRESSURE_PLATE_MONSTERS:
						if ( entity->behavior != &actMonster )
						{
							continue;
						}
						break;
					case Entity::PRESSURE_PLATE_ITEMS:
						if ( entity->behavior != &actItem )
						{
							continue;
						}
						break;
					case Entity::PRESSURE_PLATE_BOULDERS:
						if ( entity->behavior != &actBoulder )
						{
							continue;
						}
						break;
					case Entity::PRESSURE_PLATE_PLAYERS_OR_MONSTERS:
						if ( !(entity->behavior == &actPlayer || entity->behavior == &actMonster) )
						{
							continue;
						}
						break;
					case Entity::PRESSURE_PLATE_PLAYERS_OR_ALLIES:
						if ( !(entity->behavior == &actPlayer || (entity->behavior == &actMonster && entity->monsterAllyGetPlayerLeader())) )
						{
							continue;
						}
						break;
					case Entity::PRESSURE_PLATE_MONSTERS_NON_ALLY:
						if ( entity->behavior != &actMonster )
						{
							continue;
						}
						if ( entity->monsterAllyGetPlayerLeader() )
						{
							continue; // non allies only 
						}
						break;
					}
					somebodyonme = true;
					if ( !TRAP_ON )
					{
						my->toggleSwitch();
						TRAP_ON = 1;
					}
					break;
				}
			}
		}
	}
	if ( !somebodyonme )
	{
		if ( TRAP_ON )
		{
			my->toggleSwitch();
			TRAP_ON = 0;
		}
	}
}

#define TRAPPERMANENT_ON my->skill[0]
void actTrapPermanent(Entity* my)
{
	if ( (svFlags & SV_FLAG_CHEATS) && *cvar_wire_debug )
	{
		my->flags[INVISIBLE] = false;
	}
	else
	{
		my->flags[INVISIBLE] = true;
	}

	// activates circuit when certain entities are occupying its tile
	// unlike actTrap, never deactivates
	node_t* node;
	Entity* entity;
	my->flags[PASSABLE] = true; // these should ALWAYS be passable. No exceptions

	if ( !strcmp(map.name, "Boss") )
	{
		for ( node = map.creatures->first; node != nullptr; node = node->next ) //Only looking at players? Don't search full map.entities.
		{
			entity = (Entity*)node->element;
			if ( entity->behavior == &actPlayer )
			{
				if ( entity->x < 26 * 16 || entity->y < 6 * 16 || entity->y >= 26 * 16 )   // hardcoded, I know...
				{
					return;
				}
			}
		}
	}
	else if ( !strcmp(map.name, "Sanctum") )
	{
		if ( my->x > 50 * 16 )
		{
			// exit gate, act abnormal!
			bool monsterAlive = false;
			for ( node = map.creatures->first; node != nullptr; node = node->next )
			{
				entity = (Entity*)node->element;
				if ( entity->behavior == &actMonster && (entity->getRace() == LICH_FIRE || entity->getRace() == LICH_ICE) )
				{
					monsterAlive = true;
				}
			}
			if ( !monsterAlive )
			{
				// turn on when safe.
				TRAPPERMANENT_ON = 1;
			}
		}
		else if ( my->x < 27 * 16 )
		{
			// entry gates, act normal!
		}
		else
		{
			// fight trigger plates, wait for players to assemble.
			for ( node = map.creatures->first; node != nullptr; node = node->next ) //Only looking at players? Don't search full map.entities.
			{
				entity = (Entity*)node->element;
				if ( entity->behavior == &actPlayer )
				{
					if ( entity->x < 29 * 16 )   // hardcoded, I know...
					{
						return;
					}
				}
			}
		}
	}

	if ( TRAPPERMANENT_ON )
	{
		my->switchUpdateNeighbors();
	}
	else
	{
		if ( my->skill[1] == 1 )
		{
			// skip checking for entities.
			return;
		}
		std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 2);
		for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end(); ++it )
		{
			list_t* currentList = *it;
			for ( node = currentList->first; node != nullptr; node = node->next )
			{
				entity = (Entity*)node->element;
				if ( entity->behavior == &actItem && entity->flags[INVISIBLE] )
				{
					continue;
				}
				if ( entity->behavior == &actPlayer || entity->behavior == &actItem 
					|| (entity->behavior == &actMonster && !entity->isInertMimic()) || entity->behavior == &actBoulder
					|| entity->behavior == &actBomb || entity->behavior == &actDecoyBox )
				{
					if ( floor(entity->x / 16) == floor(my->x / 16) && floor(entity->y / 16) == floor(my->y / 16) )
					{
						switch ( my->pressurePlateTriggerType )
						{
						default:
						case Entity::PRESSURE_PLATE_DEFAULT_ALL:
							break;
						case Entity::PRESSURE_PLATE_PLAYERS:
							if ( entity->behavior != &actPlayer )
							{
								continue;
							}
							break;
						case Entity::PRESSURE_PLATE_MONSTERS:
							if ( entity->behavior != &actMonster )
							{
								continue;
							}
							break;
						case Entity::PRESSURE_PLATE_ITEMS:
							if ( entity->behavior != &actItem )
							{
								continue;
							}
							break;
						case Entity::PRESSURE_PLATE_BOULDERS:
							if ( entity->behavior != &actBoulder )
							{
								continue;
							}
							break;
						case Entity::PRESSURE_PLATE_PLAYERS_OR_MONSTERS:
							if ( !(entity->behavior == &actPlayer || entity->behavior == &actMonster) )
							{
								continue;
							}
							break;
						case Entity::PRESSURE_PLATE_PLAYERS_OR_ALLIES:
							if ( !(entity->behavior == &actPlayer || (entity->behavior == &actMonster && entity->monsterAllyGetPlayerLeader())) )
							{
								continue;
							}
							break;
						case Entity::PRESSURE_PLATE_MONSTERS_NON_ALLY:
							if ( entity->behavior != &actMonster )
							{
								continue;
							}
							if ( entity->monsterAllyGetPlayerLeader() )
							{
								continue; // non allies only 
							}
							break;
						}

						my->toggleSwitch();
						TRAPPERMANENT_ON = 1;
					}
				}
			}
		}
	}
}

//This is called when the switch is toggled by the player.
void Entity::toggleSwitch(int skillIndexForPower)
{
	//If off, power on and send poweron signal. If on, power off and send poweroff signal.
	if ( skillIndexForPower < 0 )
	{
		skillIndexForPower = 0;
	}
	Sint32& switchPower = skill[skillIndexForPower];
	switchPower = (switchPower == SWITCH_UNPOWERED);
	serverUpdateEntitySkill(this, skillIndexForPower);

	//(my->skill[0]) ? my->sprite = 171 : my->sprite = 168;

	list_t* neighbors = getPowerableNeighbors(); //Grab a list of all neighboring circuits and mechanisms.

	if (neighbors)
	{
		node_t* node = NULL;
		for (node = neighbors->first; node != NULL; node = node->next)
		{
			if (node->element)
			{
				Entity* powerable = (Entity*)(node->element);

				if (powerable)
				{
					if (powerable->behavior == actCircuit)
					{
						(switchPower) ? powerable->circuitPowerOn() : powerable->circuitPowerOff();
					}
					else if ( powerable->behavior == &::actSignalTimer )
					{
						int x1 = static_cast<int>(this->x / 16);
						int x2 = static_cast<int>(powerable->x / 16);
						int y1 = static_cast<int>(this->y / 16);
						int y2 = static_cast<int>(powerable->y / 16);
						//messagePlayer(0, "%d, %d, %d, %d", x1, x2, y1, y2);
						switch ( powerable->signalInputDirection )
						{
							case 0: // west
								if ( (x1 + 1) == x2 )
								{
									(switchPower) ? powerable->mechanismPowerOn() : powerable->mechanismPowerOff();
								}
								break;
							case 1: // south
								if ( (y1 - 1) == y2 )
								{
									(switchPower) ? powerable->mechanismPowerOn() : powerable->mechanismPowerOff();
								}
								break;
							case 2: // east
								if ( (x1 - 1) == x2 )
								{
									(switchPower) ? powerable->mechanismPowerOn() : powerable->mechanismPowerOff();
								}
								break;
							case 3: // north
								if ( (y1 + 1) == y2 )
								{
									(switchPower) ? powerable->mechanismPowerOn() : powerable->mechanismPowerOff();
								}
								break;
							default:
								break;
						}
					}
					else if ( powerable->behavior == &::actSignalGateAND )
					{
						int x1 = static_cast<int>(this->x / 16);
						int y1 = static_cast<int>(this->y / 16);
						//messagePlayer(0, "%d, %d, %d, %d", x1, x2, y1, y2);
						signalGateANDOnReceive(*powerable, switchPower, x1, y1);
					}
					else
					{
						(switchPower) ? powerable->mechanismPowerOn() : powerable->mechanismPowerOff();
					}
				}
			}
		}

		list_FreeAll(neighbors); //Free the list.
		free(neighbors);
	}
}

void Entity::switchUpdateNeighbors()
{
	list_t* neighbors = getPowerableNeighbors(); //Grab a list of all neighboring circuits and mechanisms.

	if (neighbors)
	{
		node_t* node = NULL;
		for (node = neighbors->first; node != NULL; node = node->next)
		{
			if (node->element)
			{
				Entity* powerable = (Entity*)(node->element);

				if (powerable)
				{
					if (powerable->circuit_status != CIRCUIT_ON)
					{
						if (powerable->behavior == actCircuit)
						{
							powerable->circuitPowerOn();
						}
						else if ( powerable->behavior == &::actSignalTimer )
						{
							int x1 = static_cast<int>(this->x / 16);
							int x2 = static_cast<int>(powerable->x / 16);
							int y1 = static_cast<int>(this->y / 16);
							int y2 = static_cast<int>(powerable->y / 16);
							//messagePlayer(0, "%d, %d, %d, %d", x1, x2, y1, y2);
							switch ( powerable->signalInputDirection )
							{
								case 0: // west
									if ( (x1 + 1) == x2 )
									{
										powerable->mechanismPowerOn();
									}
									break;
								case 1: // south
									if ( (y1 - 1) == y2 )
									{
										powerable->mechanismPowerOn();
									}
									break;
								case 2: // east
									if ( (x1 - 1) == x2 )
									{
										powerable->mechanismPowerOn();
									}
									break;
								case 3: // north
									if ( (y1 + 1) == y2 )
									{
										powerable->mechanismPowerOn();
									}
									break;
								default:
									break;
							}
						}
						else if ( powerable->behavior == &::actSignalGateAND )
						{
							int x1 = static_cast<int>(this->x / 16);
							int y1 = static_cast<int>(this->y / 16);
							//messagePlayer(0, "%d, %d, %d, %d", x1, x2, y1, y2);
							signalGateANDOnReceive(*powerable, true, x1, y1);
						}
						else
						{
							powerable->mechanismPowerOn();
						}
					}
				}
			}
		}

		list_FreeAll(neighbors); //Free the list.
		free(neighbors);
	}
}

void getPowerablesOnTile(int x, int y, list_t** list)
{

	//Take the return value of checkTileForEntity() and sort that list for powerables.
	//if (entity->powerable == true)
	//And then free the list returned by checkTileForEntity.

	//Right. First, grab all the entities on the tile.
	list_t* entities = NULL;
	entities = checkTileForEntity(x, y);

	if (!entities)
	{
		return;    //No use continuing, got no entities.
	}

	node_t* node = NULL;
	node_t* node2 = NULL;
	//Loop through the list of entities.
	for (node = entities->first; node != NULL; node = node->next)
	{
		if (node->element)
		{
			Entity* entity = (Entity*) node->element;
			//Check if the entity is powerable.
			if (entity && entity->skill[28])   //If skill 28 = 0, the entity is not a powerable.
			{
				//If this is the first powerable found, the list needs to be created.
				if (!(*list))
				{
					*list = (list_t*) malloc(sizeof(list_t));
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

	/*if (entities)
	{
		list_FreeAll(entities);
		free(entities);
	}*/

	//return return_val;
}

list_t* Entity::getPowerableNeighbors()
{
	list_t* return_val = NULL;


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

void actSoundSource(Entity* my)
{
	if ( !my )
	{
		return;
	}

	my->actSoundSource();
}

void Entity::actSoundSource()
{
#ifdef SOUND
	if ( multiplayer == CLIENT )
	{
		return;
	}

	if ( soundSourceDelay > 0 && soundSourceDelayCounter == 0 )
	{
		soundSourceDelayCounter = soundSourceDelay;
	}

	if ( circuit_status == CIRCUIT_ON )
	{
		// received power
		if ( soundSourceDelayCounter > 0 )
		{
			--soundSourceDelayCounter;
			if ( soundSourceDelayCounter != 0 )
			{
				return;
			}
		}
		if ( !soundSourceFired )
		{
			soundSourceFired = 1;
			if ( soundSourceToPlay >= 0 && soundSourceToPlay < numsounds )
			{
				if ( soundSourceOrigin == 1 )
				{
					for ( int c = 0; c < MAXPLAYERS; ++c )
					{
						playSoundPlayer(c, soundSourceToPlay, soundSourceVolume);
					}
				}
				else
				{
					playSoundEntity(this, soundSourceToPlay, soundSourceVolume);
				}
			}
		}
	}
	else if ( circuit_status == CIRCUIT_OFF )
	{
		if ( soundSourceDelay > 0 )
		{
			soundSourceDelayCounter = soundSourceDelay;
		}
		if ( soundSourceFired && !soundSourceLatchOn )
		{
			soundSourceFired = 0;
		}
	}
#endif // SOUND
}

#define SIGNALTIMER_DELAYCOUNT skill[6]
#define SIGNALTIMER_TIMERCOUNT skill[7]
#define SIGNALTIMER_REPEATCOUNT skill[8]
#define SIGNAL_INIT skill[11]

void actSignalTimer(Entity* my)
{
	if ( !my )
	{
		return;
	}

	my->actSignalTimer();
}

void Entity::actSignalTimer()
{
	if ( multiplayer == CLIENT )
	{
		return;
	}

	int tx = x / 16;
	int ty = y / 16;
	list_t *neighbors = nullptr;
	bool updateNeighbors = false;
	if ( !SIGNAL_INIT )
	{
		SIGNAL_INIT = 1;
		if ( signalInvertOutput != 0 )
		{
			updateNeighbors = true; // once off power the neighbours since needs an external kick
		}
	}

	if ( circuit_status == CIRCUIT_ON || signalTimerLatchInput == 2 )
	{
		if ( signalTimerLatchInput == 1 )
		{
			signalTimerLatchInput = 2;
		}
		if ( signalActivateDelay > 0 && SIGNALTIMER_DELAYCOUNT == 0 && switch_power == SWITCH_UNPOWERED )
		{
			SIGNALTIMER_DELAYCOUNT = signalActivateDelay;
		}
		if ( SIGNALTIMER_DELAYCOUNT > 0 )
		{
			--SIGNALTIMER_DELAYCOUNT;
			if ( SIGNALTIMER_DELAYCOUNT != 0 )
			{
				return;
			}
		}
		if ( switch_power == SWITCH_UNPOWERED )
		{
			switch_power = SWITCH_POWERED;
			updateNeighbors = true;
			if ( signalTimerRepeatCount > 0 && SIGNALTIMER_REPEATCOUNT <= 0 )
			{
				SIGNALTIMER_REPEATCOUNT = signalTimerRepeatCount;
			}
		}
		else if ( signalTimerInterval > 0 )
		{
			if ( SIGNALTIMER_TIMERCOUNT == 0 )
			{
				SIGNALTIMER_TIMERCOUNT = signalTimerInterval;
			}
			if ( SIGNALTIMER_TIMERCOUNT > 0 )
			{
				--SIGNALTIMER_TIMERCOUNT;
				if ( SIGNALTIMER_TIMERCOUNT != 0 )
				{
					return;
				}
			}
			if ( switch_power == SWITCH_POWERED )
			{
				switch_power = 2;
				updateNeighbors = true;
			}
			else
			{
				if ( signalTimerRepeatCount > 0 )
				{
					if ( SIGNALTIMER_REPEATCOUNT > 1 )
					{
						switch_power = SWITCH_POWERED;
						updateNeighbors = true;
						--SIGNALTIMER_REPEATCOUNT;
					}
				}
				else
				{
					switch_power = SWITCH_POWERED;
					updateNeighbors = true;
				}
			}
		}
	}
	else if ( circuit_status == CIRCUIT_OFF && signalTimerLatchInput == 0 )
	{
		if ( switch_power != SWITCH_UNPOWERED )
		{
			switch_power = SWITCH_UNPOWERED;
			updateNeighbors = true;
		}
		if ( signalTimerInterval > 0 )
		{
			SIGNALTIMER_TIMERCOUNT = signalTimerInterval;
		}
		if ( signalTimerRepeatCount > 0 )
		{
			SIGNALTIMER_REPEATCOUNT = signalTimerRepeatCount;
		}
	}

	if ( updateNeighbors )
	{
		switch ( signalInputDirection )
		{
			case 0: // west
				getPowerablesOnTile(tx + 1, ty, &neighbors); //Check tile to the left.
				break;
			case 1: // south
				getPowerablesOnTile(tx, ty - 1, &neighbors); //Check tile to the north.
				break;
			case 2: // east
				getPowerablesOnTile(tx - 1, ty, &neighbors); //Check tile to the right.
				break;
			case 3: // north
				getPowerablesOnTile(tx, ty + 1, &neighbors); //Check tile to the south
				break;
		}
		if ( neighbors != nullptr )
		{
			bool power_to_neighbors = ((signalInvertOutput == 0) ? (switch_power == SWITCH_POWERED) : (!(switch_power == SWITCH_POWERED)));
			node_t* node = nullptr;
			for ( node = neighbors->first; node != nullptr; node = node->next )
			{
				if ( node->element )
				{
					Entity* powerable = (Entity*)(node->element);

					if ( powerable )
					{
						if ( powerable->behavior == actCircuit )
						{
							(power_to_neighbors) ? powerable->circuitPowerOn() : powerable->circuitPowerOff();
						}
						else
						{
							if ( powerable->behavior == &::actSignalTimer )
							{
								switch ( powerable->signalInputDirection )
								{
									case 0: // west
										if ( static_cast<int>(this->x / 16) == static_cast<int>((powerable->x / 16) - 1) )
										{
											(power_to_neighbors) ? powerable->mechanismPowerOn() : powerable->mechanismPowerOff();
										}
										break;
									case 1: // south
										if ( static_cast<int>(this->y / 16) == static_cast<int>((powerable->y / 16) - 1) )
										{
											(power_to_neighbors) ? powerable->mechanismPowerOn() : powerable->mechanismPowerOff();
										}
										break;
									case 2: // east
										if ( static_cast<int>(this->x / 16) == static_cast<int>((powerable->x / 16) + 1) )
										{
											(power_to_neighbors) ? powerable->mechanismPowerOn() : powerable->mechanismPowerOff();
										}
										break;
									case 3: // north
										if ( static_cast<int>(this->y / 16) == static_cast<int>((powerable->y / 16) + 1) )
										{
											(power_to_neighbors) ? powerable->mechanismPowerOn() : powerable->mechanismPowerOff();
										}
										break;
									default:
										break;
								}
							}
							else if ( powerable->behavior == &::actSignalGateAND )
							{
								int x1 = static_cast<int>(this->x / 16);
								int y1 = static_cast<int>(this->y / 16);
								//messagePlayer(0, "%d, %d, %d, %d", x1, x2, y1, y2);
								signalGateANDOnReceive(*powerable, power_to_neighbors, x1, y1);
							}
							else
							{
								(power_to_neighbors) ? powerable->mechanismPowerOn() : powerable->mechanismPowerOff();
							}
						}
					}
				}
			}
			list_FreeAll(neighbors); //Free the list.
			free(neighbors);
		}
	}
}

void actSignalGateAND(Entity* my)
{
	if ( !my )
	{
		return;
	}

	my->actSignalGateAND();
}

struct SignalGate_t
{
	enum SignalGateDir
	{
		DIR_EAST,
		DIR_SOUTH,
		DIR_WEST,
		DIR_NORTH
	};
	std::map<int, std::vector<SignalGateDir>> recvDirs =
	{
		{ DIR_EAST + 0, {DIR_SOUTH, DIR_NORTH}},
		{ DIR_EAST + 4, {DIR_SOUTH, DIR_WEST}},
		{ DIR_EAST + 8, {DIR_WEST, DIR_NORTH}},
		{ DIR_SOUTH + 0, {DIR_EAST, DIR_WEST}},
		{ DIR_SOUTH + 4, {DIR_NORTH, DIR_WEST}},
		{ DIR_SOUTH + 8, {DIR_EAST, DIR_NORTH}},
		{ DIR_WEST + 0, {DIR_SOUTH, DIR_NORTH}},
		{ DIR_WEST + 4, {DIR_EAST, DIR_NORTH}},
		{ DIR_WEST + 8, {DIR_SOUTH, DIR_EAST}},
		{ DIR_NORTH + 0, {DIR_EAST, DIR_WEST}},
		{ DIR_NORTH + 4, {DIR_EAST, DIR_SOUTH}},
		{ DIR_NORTH + 8, {DIR_SOUTH, DIR_WEST}}
	};
};

SignalGate_t SignalGateProps;

void signalGateANDOnReceive(Entity& gate, const bool powered, const int receivex, const int receivey)
{
	int x = static_cast<int>(gate.x / 16);
	int y = static_cast<int>(gate.y / 16);

	auto& dirsAllowed = SignalGateProps.recvDirs[gate.signalInputDirection];
	Uint32 bits = 0;
	bool foundResult = false;
	for ( auto dir : dirsAllowed )
	{
		bits |= (1 << dir);
		bool res = false;
		if ( dir == SignalGate_t::DIR_EAST )
		{
			if ( receivey == y && ((receivex - 1) == x) )
			{
				res = true;
			}
		}
		else if ( dir == SignalGate_t::DIR_SOUTH )
		{
			if ( receivex == x && ((receivey - 1) == y) )
			{
				res = true;
			}
		}
		else if ( dir == SignalGate_t::DIR_WEST )
		{
			if ( receivey == y && ((receivex + 1) == x) )
			{
				res = true;
			}
		}
		else if ( dir == SignalGate_t::DIR_NORTH )
		{
			if ( receivex == x && ((receivey + 1) == y) )
			{
				res = true;
			}
		}

		if ( res )
		{
			if ( powered )
			{
				gate.signalGateANDPowerCount |= (1 << dir);
			}
			else
			{
				gate.signalGateANDPowerCount &= ~(1 << dir);
			}
			foundResult = true;
		}
	}

	if ( foundResult )
	{
		if ( gate.signalGateANDPowerCount == bits )
		{
			gate.skill[28] = 2;
		}
		else
		{
			gate.skill[28] = 1;
		}
	}
}

void Entity::actSignalGateAND()
{
	if ( multiplayer == CLIENT )
	{
		return;
	}

	int tx = x / 16;
	int ty = y / 16;
	list_t* neighbors = nullptr;
	bool updateNeighbors = false;
	if ( !SIGNAL_INIT )
	{
		SIGNAL_INIT = 1;
		if ( signalInvertOutput != 0 )
		{
			updateNeighbors = true; // once off power the neighbours since needs an external kick
		}
	}

	if ( circuit_status == CIRCUIT_ON || signalTimerLatchInput == 2 )
	{
		if ( signalTimerLatchInput == 1 )
		{
			signalTimerLatchInput = 2;
		}
		if ( signalActivateDelay > 0 && SIGNALTIMER_DELAYCOUNT == 0 && switch_power == SWITCH_UNPOWERED )
		{
			SIGNALTIMER_DELAYCOUNT = signalActivateDelay;
		}
		if ( SIGNALTIMER_DELAYCOUNT > 0 )
		{
			--SIGNALTIMER_DELAYCOUNT;
			if ( SIGNALTIMER_DELAYCOUNT != 0 )
			{
				return;
			}
		}
		if ( switch_power == SWITCH_UNPOWERED )
		{
			switch_power = SWITCH_POWERED;
			updateNeighbors = true;
			if ( signalTimerRepeatCount > 0 && SIGNALTIMER_REPEATCOUNT <= 0 )
			{
				SIGNALTIMER_REPEATCOUNT = signalTimerRepeatCount;
			}
		}
		else if ( signalTimerInterval > 0 )
		{
			if ( SIGNALTIMER_TIMERCOUNT == 0 )
			{
				SIGNALTIMER_TIMERCOUNT = signalTimerInterval;
			}
			if ( SIGNALTIMER_TIMERCOUNT > 0 )
			{
				--SIGNALTIMER_TIMERCOUNT;
				if ( SIGNALTIMER_TIMERCOUNT != 0 )
				{
					return;
				}
			}
			if ( switch_power == SWITCH_POWERED )
			{
				switch_power = 2;
				updateNeighbors = true;
			}
			else
			{
				if ( signalTimerRepeatCount > 0 )
				{
					if ( SIGNALTIMER_REPEATCOUNT > 1 )
					{
						switch_power = SWITCH_POWERED;
						updateNeighbors = true;
						--SIGNALTIMER_REPEATCOUNT;
					}
				}
				else
				{
					switch_power = SWITCH_POWERED;
					updateNeighbors = true;
				}
			}
		}
	}
	else if ( circuit_status == CIRCUIT_OFF && signalTimerLatchInput == 0 )
	{
		if ( switch_power != SWITCH_UNPOWERED )
		{
			switch_power = SWITCH_UNPOWERED;
			updateNeighbors = true;
		}
		if ( signalTimerInterval > 0 )
		{
			SIGNALTIMER_TIMERCOUNT = signalTimerInterval;
		}
		if ( signalTimerRepeatCount > 0 )
		{
			SIGNALTIMER_REPEATCOUNT = signalTimerRepeatCount;
		}
	}

	if ( updateNeighbors )
	{
		switch ( signalInputDirection % 4 )
		{
		case 0: // east
			getPowerablesOnTile(tx + 1, ty, &neighbors);
			break;
		case 1: // south
			getPowerablesOnTile(tx, ty + 1, &neighbors);
			break;
		case 2: // east
			getPowerablesOnTile(tx - 1, ty, &neighbors);
			break;
		case 3: // north
			getPowerablesOnTile(tx, ty - 1, &neighbors);
			break;
		}
		if ( neighbors != nullptr )
		{
			bool power_to_neighbors = ((signalInvertOutput == 0) ? (switch_power == SWITCH_POWERED) : (!(switch_power == SWITCH_POWERED)));

			node_t* node = nullptr;
			for ( node = neighbors->first; node != nullptr; node = node->next )
			{
				if ( node->element )
				{
					Entity* powerable = (Entity*)(node->element);

					if ( powerable )
					{
						if ( powerable->behavior == actCircuit )
						{
							(power_to_neighbors) ? powerable->circuitPowerOn() : powerable->circuitPowerOff();
						}
						else
						{
							if ( powerable->behavior == &::actSignalTimer )
							{
								switch ( powerable->signalInputDirection )
								{
								case 0: // west
									if ( static_cast<int>(this->x / 16) == static_cast<int>((powerable->x / 16) - 1) )
									{
										(power_to_neighbors) ? powerable->mechanismPowerOn() : powerable->mechanismPowerOff();
									}
									break;
								case 1: // south
									if ( static_cast<int>(this->y / 16) == static_cast<int>((powerable->y / 16) - 1) )
									{
										(power_to_neighbors) ? powerable->mechanismPowerOn() : powerable->mechanismPowerOff();
									}
									break;
								case 2: // east
									if ( static_cast<int>(this->x / 16) == static_cast<int>((powerable->x / 16) + 1) )
									{
										(power_to_neighbors) ? powerable->mechanismPowerOn() : powerable->mechanismPowerOff();
									}
									break;
								case 3: // north
									if ( static_cast<int>(this->y / 16) == static_cast<int>((powerable->y / 16) + 1) )
									{
										(power_to_neighbors) ? powerable->mechanismPowerOn() : powerable->mechanismPowerOff();
									}
									break;
								default:
									break;
								}
							}
							else if ( powerable->behavior == &::actSignalGateAND )
							{
								int x1 = static_cast<int>(this->x / 16);
								int y1 = static_cast<int>(this->y / 16);
								//messagePlayer(0, "%d, %d, %d, %d", x1, x2, y1, y2);
								signalGateANDOnReceive(*powerable, power_to_neighbors, x1, y1);
							}
							else
							{
								(power_to_neighbors) ? powerable->mechanismPowerOn() : powerable->mechanismPowerOff();
							}
						}
					}
				}
			}
			list_FreeAll(neighbors); //Free the list.
			free(neighbors);
		}
	}
}