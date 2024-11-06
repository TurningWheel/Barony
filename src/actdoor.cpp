/*-------------------------------------------------------------------------------

	BARONY
	File: actdoor.cpp
	Desc: behavior function for doors

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "engine/audio/sound.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "interface/interface.hpp"
#include "items.hpp"
#include "prng.hpp"
#include "mod_tools.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

void actDoor(Entity* my)
{
	if (!my)
	{
		return;
	}

	Entity* entity;
	int i, c;

	auto& rng = my->entity_rng ? *my->entity_rng : local_rng;

	if ( !my->doorInit )
	{
		my->createWorldUITooltip();

		my->doorInit = 1;
		my->doorStartAng = my->yaw;
		my->doorHealth = 15 + rng.rand() % 5;
		my->doorMaxHealth = my->doorHealth;
		my->doorOldHealth = my->doorHealth;
		my->doorPreventLockpickExploit = 1;
		my->doorLockpickHealth = 20;
		if ( my->doorForceLockedUnlocked == 2 )
		{
			my->doorLocked = 0; // force unlocked.
		}
		else if ( rng.rand() % 20 == 0 || (!strncmp(map.name, "The Great Castle", 16) && rng.rand() % 2 == 0) || my->doorForceLockedUnlocked == 1 )   // 5% chance
		{
			my->doorLocked = 1;
			my->doorPreventLockpickExploit = 0;
		}
		my->doorOldStatus = my->doorStatus;
		my->scalex = 1.01;
		my->scaley = 1.01;
		my->scalez = 1.01;
		my->flags[BURNABLE] = true;
	}
	else
	{
		if ( multiplayer != CLIENT )
		{
			// burning
			if ( my->flags[BURNING] )
			{
				if ( ticks % 30 == 0 )
				{
					my->doorHealth--;
				}
			}

			my->doorOldHealth = my->doorHealth;

			// door mortality :p
			if ( my->doorHealth <= 0 )
			{
				for ( c = 0; c < 5; c++ )
				{
					entity = spawnGib(my);
					entity->flags[INVISIBLE] = false;
					entity->sprite = 187; // Splinter.vox
					entity->x = floor(my->x / 16) * 16 + 8;
					entity->y = floor(my->y / 16) * 16 + 8;
					entity->z = 0;
					entity->z += -7 + local_rng.rand() % 14;
					if ( !my->doorDir )
					{
						// horizontal door
						entity->y += -4 + local_rng.rand() % 8;
						if ( my->doorSmacked )
						{
							entity->yaw = PI;
						}
						else
						{
							entity->yaw = 0;
						}
					}
					else
					{
						// vertical door
						entity->x += -4 + local_rng.rand() % 8;
						if ( my->doorSmacked )
						{
							entity->yaw = PI / 2;
						}
						else
						{
							entity->yaw = 3 * PI / 2;
						}
					}
					entity->pitch = (local_rng.rand() % 360) * PI / 180.0;
					entity->roll = (local_rng.rand() % 360) * PI / 180.0;
					entity->vel_x = cos(entity->yaw) * (1.2 + (local_rng.rand() % 10) / 50.0);
					entity->vel_y = sin(entity->yaw) * (1.2 + (local_rng.rand() % 10) / 50.0);
					entity->vel_z = -.25;
					entity->fskill[3] = 0.04;
					serverSpawnGibForClient(entity);
				}
				playSoundEntity(my, 177, 64);
				list_RemoveNode(my->mynode);
				return;
			}

			// using door
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if ( selectedEntity[i] == my || client_selected[i] == my )
				{
					if ( Player::getPlayerInteractEntity(i) && inrange[i])
					{
						Entity* playerEntity = Player::getPlayerInteractEntity(i);
						if ( !my->doorLocked )   // door unlocked
						{
							if ( !my->doorDir && !my->doorStatus )
							{
								// open door
								my->doorStatus = 1 + (playerEntity->x > my->x);
								playSoundEntity(my, 21, 96);
								messagePlayer(i, MESSAGE_INTERACTION, Language::get(464));
								Compendium_t::Events_t::eventUpdateWorld(i, Compendium_t::CPDM_DOOR_OPENED, "door", 1);
							}
							else if ( my->doorDir && !my->doorStatus )
							{
								// open door
								my->doorStatus = 1 + (playerEntity->y < my->y);
								playSoundEntity(my, 21, 96);
								messagePlayer(i, MESSAGE_INTERACTION, Language::get(464));
								Compendium_t::Events_t::eventUpdateWorld(i, Compendium_t::CPDM_DOOR_OPENED, "door", 1);
							}
							else
							{
								// close door
								my->doorStatus = 0;
								playSoundEntity(my, 22, 96);
								messagePlayer(i, MESSAGE_INTERACTION, Language::get(465));
								Compendium_t::Events_t::eventUpdateWorld(i, Compendium_t::CPDM_DOOR_CLOSED, "door", 1);
							}
						}
						else
						{
							// door locked
							messagePlayer(i, MESSAGE_INTERACTION, Language::get(466));
							playSoundEntity(my, 152, 64);
						}
					}
				}
			}
		}

		// door swinging
		if ( !my->doorStatus )
		{
			// closing door
			if ( my->yaw > my->doorStartAng )
			{
				my->yaw = std::max(my->doorStartAng, my->yaw - 0.15);
			}
			else if ( my->yaw < my->doorStartAng )
			{
				my->yaw = std::min(my->doorStartAng, my->yaw + 0.15);
			}
		}
		else
		{
			// opening door
			if ( my->doorStatus == 1 )
			{
				if ( my->yaw > my->doorStartAng + PI / 2 )
				{
					my->yaw = std::max(my->doorStartAng + PI / 2, my->yaw - 0.15);
				}
				else if ( my->yaw < my->doorStartAng + PI / 2 )
				{
					my->yaw = std::min(my->doorStartAng + PI / 2, my->yaw + 0.15);
				}
			}
			else if ( my->doorStatus == 2 )
			{
				if ( my->yaw > my->doorStartAng - PI / 2 )
				{
					my->yaw = std::max(my->doorStartAng - PI / 2, my->yaw - 0.15);
				}
				else if ( my->yaw < my->doorStartAng - PI / 2 )
				{
					my->yaw = std::min(my->doorStartAng - PI / 2, my->yaw + 0.15);
				}
			}
		}

		// setting collision
		if ( my->yaw == my->doorStartAng && my->flags[PASSABLE] )
		{
			// don't set impassable if someone's inside, otherwise do
			node_t* node;
			bool somebodyinside = false;
			std::vector<list_t*> entLists;
			if ( multiplayer == CLIENT )
			{
				entLists.push_back(map.entities); // clients use old map.entities method
			}
			else
			{
				entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 2);
			}
			real_t oldmyx = my->x;
			real_t oldmyy = my->y;
			my->x = (static_cast<int>(my->x) >> 4) * 16.0 + 8.0; // door positioning isn't centred on tile so adjust
			my->y = (static_cast<int>(my->y) >> 4) * 16.0 + 8.0;
			for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end() && !somebodyinside; ++it )
			{
				list_t* currentList = *it;
				for ( node = currentList->first; node != nullptr; node = node->next )
				{
					Entity* entity = (Entity*)node->element;
					if ( entity == my || (entity->flags[PASSABLE] && entity->behavior != &actDeathGhost) 
						|| entity->behavior == &actDoorFrame )
					{
						continue;
					}

					bool insideEntity = false;
					if ( entity->behavior == &actDoor )
					{
						real_t oldx = entity->x;
						real_t oldy = entity->y;
						entity->x = (static_cast<int>(entity->x) >> 4) * 16.0 + 8.0; // door positioning isn't centred on tile so adjust
						entity->y = (static_cast<int>(entity->y) >> 4) * 16.0 + 8.0;
						insideEntity = entityInsideEntity(my, entity);
						entity->x = oldx;
						entity->y = oldy;
					}
					else
					{
						insideEntity = entityInsideEntity(my, entity);
					}

					if ( insideEntity )
					{
						somebodyinside = true;
						break;
					}
				}
			}
			my->x = oldmyx;
			my->y = oldmyy;
			if ( !somebodyinside )
			{
				my->focaly = 0;
				if ( my->doorStartAng == 0 )
				{
					my->y -= 5;
				}
				else
				{
					my->x -= 5;
				}
				my->flags[PASSABLE] = false;
			}
		}
		else if ( my->yaw != my->doorStartAng && !my->flags[PASSABLE] )
		{
			my->focaly = -5;
			if ( my->doorStartAng == 0 )
			{
				my->y += 5;
			}
			else
			{
				my->x += 5;
			}
			my->flags[PASSABLE] = true;
		}

		// update for clients
		if ( multiplayer == SERVER )
		{
			if ( my->doorOldStatus != my->doorStatus )
			{
				my->doorOldStatus = my->doorStatus;
				serverUpdateEntitySkill(my, 3);
			}
		}
	}
}

void actDoorFrame(Entity* my)
{
	// dummy function
	// intended to make it easier
	// to determine whether an entity
	// is part of a door frame
	if ( my->flags[INVISIBLE] == false )
	{
		my->flags[PASSABLE] = true; // the actual frame should ALWAYS be passable
	}
}

void Entity::doorHandleDamageMagic(int damage, Entity &magicProjectile, Entity *caster)
{
	doorHealth -= damage; //Decrease door health.
	if ( caster )
	{
		if ( caster->behavior == &actPlayer )
		{
			if ( doorHealth <= 0 )
			{
				if ( magicProjectile.behavior == &actBomb )
				{
					messagePlayer(caster->skill[2], MESSAGE_COMBAT, Language::get(3617), items[magicProjectile.skill[21]].getIdentifiedName(), Language::get(674));
				}
				else
				{
					messagePlayer(caster->skill[2], MESSAGE_COMBAT, Language::get(387));
				}
				Compendium_t::Events_t::eventUpdateWorld(caster->skill[2], Compendium_t::CPDM_DOOR_BROKEN, "door", 1);
			}
			else
			{
				if ( magicProjectile.behavior == &actBomb )
				{
					messagePlayer(caster->skill[2], MESSAGE_COMBAT_BASIC, Language::get(3618), items[magicProjectile.skill[21]].getIdentifiedName(), Language::get(674));
				}
				else
				{
					messagePlayer(caster->skill[2], MESSAGE_COMBAT_BASIC, Language::get(378), Language::get(674));
				}
			}
			updateEnemyBar(caster, this, Language::get(674), doorHealth, doorMaxHealth,
				false, DamageGib::DMG_DEFAULT);
		}
	}
	if ( !doorDir )
	{
		doorSmacked = (magicProjectile.x > this->x);
	}
	else
	{
		doorSmacked = (magicProjectile.y < this->y);
	}

	playSoundEntity(this, 28, 128);
}
