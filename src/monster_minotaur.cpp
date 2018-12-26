/*-------------------------------------------------------------------------------

	BARONY
	File: monster_minotaur.cpp
	Desc: implements all of the minotaur monster's code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "items.hpp"
#include "monster.hpp"
#include "sound.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "magic/magic.hpp"
#include "player.hpp"
#include "colors.hpp"

void initMinotaur(Entity* my, Stat* myStats)
{
	int c;
	node_t* node;

	my->initMonster(239);

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 107;
		MONSTER_SPOTVAR = 3;
		MONSTER_IDLESND = 110;
		MONSTER_IDLEVAR = 3;
	}
	if ( multiplayer != CLIENT && !MONSTER_INIT )
	{
		if ( myStats != NULL )
		{
			if ( !myStats->leader_uid )
			{
				myStats->leader_uid = 0;
			}

			// apply random stat increases if set in stat_shared.cpp or editor
			setRandomMonsterStats(myStats);

			// generate 6 items max, less if there are any forced items from boss variants
			int customItemsToGenerate = ITEM_CUSTOM_SLOT_LIMIT;

			// boss variants
			if ( strcmp(map.name, "Hell Boss") == 0 )
			{
				myStats->STR = 50;
				myStats->DEX = 20;
				myStats->CON = 20;
			}
			else if ( currentlevel >= 25 )
			{
				myStats->HP += 400;
				myStats->MAXHP += 400;
				myStats->STR = 60;
				myStats->DEX = 20;
				myStats->CON = 20;
				myStats->EFFECTS[EFF_VAMPIRICAURA] = true;
				myStats->EFFECTS_TIMERS[EFF_VAMPIRICAURA] = -1;
			}


			// random effects
			// minotaurs can traverse waters and pits (pits with magic :))
			myStats->EFFECTS[EFF_LEVITATING] = true;
			myStats->EFFECTS_TIMERS[EFF_LEVITATING] = 0;

			// generates equipment and weapons if available from editor
			createMonsterEquipment(myStats);

			// create any custom inventory items from editor if available
			createCustomInventory(myStats, customItemsToGenerate);

			// count if any custom inventory items from editor
			int customItems = countCustomItems(myStats); //max limit of 6 custom items per entity.

														 // count any inventory items set to default in edtior
			int defaultItems = countDefaultItems(myStats);

			my->setHardcoreStats(*myStats);

			// generate the default inventory items for the monster, provided the editor sprite allowed enough default slots

			ItemType gemtype = GEM_RUBY;

			switch ( defaultItems )
			{
				case 6:
				case 5:
				case 4:
				case 3:
				case 2:
				case 1:
					switch ( rand() % 4 )
					{
						case 0:
							gemtype = GEM_RUBY;
							break;
						case 1:
							gemtype = GEM_EMERALD;
							break;
						case 2:
							gemtype = GEM_SAPPHIRE;
							break;
						case 3:
							gemtype = GEM_DIAMOND;
							break;
					}
					newItem(gemtype, EXCELLENT, 0, 1, rand(), true, &myStats->inventory);
					break;
				default:
					break;
			}
		}
	}

	// head
	Entity* entity = newEntity(237, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[MINOTAUR][1][0]; // 0
	entity->focaly = limbs[MINOTAUR][1][1]; // 0
	entity->focalz = limbs[MINOTAUR][1][2]; // 0
	entity->behavior = &actMinotaurLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// chest
	entity = newEntity(238, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[MINOTAUR][2][0]; // 0
	entity->focaly = limbs[MINOTAUR][2][1]; // 0
	entity->focalz = limbs[MINOTAUR][2][2]; // 0
	entity->behavior = &actMinotaurLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right leg
	entity = newEntity(243, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[MINOTAUR][3][0]; // 1
	entity->focaly = limbs[MINOTAUR][3][1]; // 0
	entity->focalz = limbs[MINOTAUR][3][2]; // 5
	entity->behavior = &actMinotaurLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left leg
	entity = newEntity(242, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[MINOTAUR][4][0]; // 1
	entity->focaly = limbs[MINOTAUR][4][1]; // 0
	entity->focalz = limbs[MINOTAUR][4][2]; // 5
	entity->behavior = &actMinotaurLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right arm
	entity = newEntity(241, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[MINOTAUR][5][0]; // 2.5
	entity->focaly = limbs[MINOTAUR][5][1]; // 7
	entity->focalz = limbs[MINOTAUR][5][2]; // 3.5
	entity->behavior = &actMinotaurLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left arm
	entity = newEntity(240, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[MINOTAUR][6][0]; // 2.5
	entity->focaly = limbs[MINOTAUR][6][1]; // -7
	entity->focalz = limbs[MINOTAUR][6][2]; // 3.5
	entity->behavior = &actMinotaurLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);
}

bool actMinotaurLimb(Entity* my)
{
	return my->actMonsterLimb();
}

void minotaurDie(Entity* my)
{
	int c;
	for ( c = 0; c < 5; c++ )
	{
		Entity* gib = spawnGib(my);
		serverSpawnGibForClient(gib);
	}

	my->spawnBlood();

	for ( c = 0; c < MAXPLAYERS; c++ )
	{
		playSoundPlayer(c, 114, 128);
	}

	my->removeMonsterDeathNodes();

	list_RemoveNode(my->mynode);
	return;
}

#define MINOTAURWALKSPEED .07

void minotaurMoveBodyparts(Entity* my, Stat* myStats, double dist)
{
	node_t* node;
	Entity* entity = NULL;
	Entity* rightbody = NULL;
	Entity* head = NULL;
	Entity* chest = NULL;
	int bodypart;

	// set invisibility //TODO: isInvisible()?
	if ( multiplayer != CLIENT )
	{
		if ( myStats->EFFECTS[EFF_INVISIBLE] == true )
		{
			my->flags[INVISIBLE] = true;
			my->flags[BLOCKSIGHT] = false;
			bodypart = 0;
			for (node = my->children.first; node != NULL; node = node->next)
			{
				if ( bodypart < 2 )
				{
					bodypart++;
					continue;
				}
				if ( bodypart >= 7 )
				{
					break;
				}
				entity = (Entity*)node->element;
				if ( !entity->flags[INVISIBLE] )
				{
					entity->flags[INVISIBLE] = true;
					serverUpdateEntityBodypart(my, bodypart);
				}
				bodypart++;
			}
		}
		else
		{
			my->flags[INVISIBLE] = false;
			my->flags[BLOCKSIGHT] = true;
			bodypart = 0;
			for (node = my->children.first; node != NULL; node = node->next)
			{
				if ( bodypart < 2 )
				{
					bodypart++;
					continue;
				}
				if ( bodypart >= 7 )
				{
					break;
				}
				entity = (Entity*)node->element;
				if ( entity->flags[INVISIBLE] )
				{
					entity->flags[INVISIBLE] = false;
					serverUpdateEntityBodypart(my, bodypart);
					serverUpdateEntityFlag(my, INVISIBLE);
				}
				bodypart++;
			}
		}
	}

	//Move bodyparts
	for (bodypart = 0, node = my->children.first; node != NULL; node = node->next, bodypart++)
	{
		if ( bodypart < 2 )
		{
			continue;
		}
		entity = (Entity*)node->element;
		entity->x = my->x;
		entity->y = my->y;
		entity->z = my->z;
		entity->yaw = my->yaw;
		if ( bodypart == 2 )
		{
			head = entity;
		}
		else if ( bodypart == 3 )
		{
			chest = entity;
		}
		if ( bodypart == 4 || bodypart == 7 )
		{
			if ( bodypart == 4 )
			{
				rightbody = (Entity*)node->next->element;
			}
			if ( dist > 0.1 )
			{
				if ( !rightbody->skill[0] )
				{
					entity->pitch -= dist * MINOTAURWALKSPEED;
					if ( entity->pitch < -PI / 4.0 )
					{
						entity->pitch = -PI / 4.0;
						if (bodypart == 4 && dist > .4)
						{
							playSound(115, 128);
						}
					}
				}
				else
				{
					entity->pitch += dist * MINOTAURWALKSPEED;
					if ( entity->pitch > PI / 4.0 )
					{
						entity->pitch = PI / 4.0;
						if (bodypart == 4 && dist > .4)
						{
							playSound(115, 128);
						}
					}
				}
			}
			else
			{
				if ( entity->pitch < 0 )
				{
					entity->pitch += 1 / fmax(dist * .1, 10.0);
					if ( entity->pitch > 0 )
					{
						entity->pitch = 0;
					}
				}
				else if ( entity->pitch > 0 )
				{
					entity->pitch -= 1 / fmax(dist * .1, 10.0);
					if ( entity->pitch < 0 )
					{
						entity->pitch = 0;
					}
				}
			}
		}
		else if ( bodypart == 5 || bodypart == 6 )
		{
			if ( bodypart == 6 )
			{
				if ( my->monsterAttack > 0 )
				{
					// vertical chop windup
					if ( my->monsterAttack == MONSTER_POSE_MELEE_WINDUP1 )
					{
						if ( my->monsterAttackTime == 0 )
						{
							// init rotations
							entity->pitch = 0;
							my->monsterArmbended = 0;
							my->monsterWeaponYaw = 0;
							entity->roll = 0;
							entity->skill[1] = 0;
						}

						limbAnimateToLimit(entity, ANIMATE_PITCH, -0.25, 5 * PI / 4, false, 0.0);

						if ( my->monsterAttackTime >= ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
						{
							if ( multiplayer != CLIENT )
							{
								my->attack(1, 0, nullptr);
							}
						}
					}
					// ceiling buster chop windup
					if ( my->monsterAttack == MONSTER_POSE_MELEE_WINDUP2 )
					{
						if ( my->monsterAttackTime == 0 )
						{
							// init rotations
							entity->pitch = 0;
							my->monsterArmbended = 0;
							my->monsterWeaponYaw = 0;
							entity->roll = 0;
							entity->skill[1] = 0;
						}

						limbAnimateToLimit(entity, ANIMATE_PITCH, -0.25, 5 * PI / 4, false, 0.0);

						if ( my->monsterAttackTime >= ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
						{
							my->monsterAttack = 1;
						}
					}
					// vertical chop attack
					else if ( my->monsterAttack == 1 )
					{
						if ( entity->pitch >= 3 * PI / 2 )
						{
							my->monsterArmbended = 1;
						}

						if ( entity->skill[1] == 0 )
						{
							// chop forwards
							if ( limbAnimateToLimit(entity, ANIMATE_PITCH, 0.4, PI / 3, false, 0.0) )
							{
								entity->skill[1] = 1;
							}
						}
						else if ( entity->skill[1] == 1 )
						{
							// return to neutral
							if ( limbAnimateToLimit(entity, ANIMATE_PITCH, -0.25, 7 * PI / 4, false, 0.0) )
							{
								entity->skill[0] = rightbody->skill[0];
								my->monsterWeaponYaw = 0;
								entity->pitch = rightbody->pitch;
								entity->roll = 0;
								my->monsterArmbended = 0;
								my->monsterAttack = 0;
							}
						}
					}
				}
			}

			if ( bodypart != 6 || (MONSTER_ATTACK == 0 && MONSTER_ATTACKTIME == 0) )
			{
				if ( dist > 0.1 )
				{
					if ( entity->skill[0] )
					{
						entity->pitch -= dist * MINOTAURWALKSPEED;
						if ( entity->pitch < -PI / 4.0 )
						{
							entity->skill[0] = 0;
							entity->pitch = -PI / 4.0;
						}
					}
					else
					{
						entity->pitch += dist * MINOTAURWALKSPEED;
						if ( entity->pitch > PI / 4.0 )
						{
							entity->skill[0] = 1;
							entity->pitch = PI / 4.0;
						}
					}
				}
				else
				{
					if ( entity->pitch < 0 )
					{
						entity->pitch += 1 / fmax(dist * .1, 10.0);
						if ( entity->pitch > 0 )
						{
							entity->pitch = 0;
						}
					}
					else if ( entity->pitch > 0 )
					{
						entity->pitch -= 1 / fmax(dist * .1, 10.0);
						if ( entity->pitch < 0 )
						{
							entity->pitch = 0;
						}
					}
				}
			}
		}
		if ( !MONSTER_ATTACK )
		{
			if ( bodypart == 6 )
			{
				entity->yaw += entity->pitch;
				head->yaw = entity->yaw;
				chest->yaw = entity->yaw;
			}
			else if ( bodypart == 7 )
			{
				entity->yaw -= entity->pitch;
			}
		}
		switch ( bodypart )
		{
			// head
			case 2:
				entity->z -= 11;
				break;
			// chest
			case 3:
				entity->z -= 4.5;
				break;
			// right leg
			case 4:
				entity->x += 2.5 * cos(my->yaw + PI / 2) - .5 * cos(my->yaw);
				entity->y += 2.5 * sin(my->yaw + PI / 2) - .5 * sin(my->yaw);
				entity->z += 2.5;
				break;
			// left leg
			case 5:
				entity->x -= 2.5 * cos(my->yaw + PI / 2) + .5 * cos(my->yaw);
				entity->y -= 2.5 * sin(my->yaw + PI / 2) + .5 * sin(my->yaw);
				entity->z += 2.5;
				break;
			// right arm
			case 6:
				entity->z -= 6;
				entity->yaw += MONSTER_WEAPONYAW;
				break;
			// left arm
			case 7:
				entity->z -= 6;
				break;
		}
	}
	if ( MONSTER_ATTACK != 0 )
	{
		MONSTER_ATTACKTIME++;
	}
	else
	{
		MONSTER_ATTACKTIME = 0;
	}
}

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

#define MINOTAURTRAP_FIRED my->skill[0]

bool actMinotaurTrap(Entity* my)
{
	if ( !my->skill[28] )
	{
		return false;
	}

	// received on signal
	if ( my->skill[28] == 2)
	{
		if ( !MINOTAURTRAP_FIRED )
		{
			Entity* monster = summonMonster(MINOTAUR, my->x, my->y);
			if ( monster )
			{
				MINOTAURTRAP_FIRED = 1;
				if ( strcmp(map.name, "Hell Boss") )
				{
					int c;
					for ( c = 0; c < MAXPLAYERS; c++ )
					{
						playSoundPlayer( c, 107 + rand() % 3, 128 );
						Uint32 color = SDL_MapRGB(mainsurface->format, 255, 128, 0);
						messagePlayerColor(c, color, language[1113]);
					}
				}
			}
		}
	}
	return true;
}

#define MINOTAURTIMER_LIFE my->skill[0]
#define MINOTAURTIMER_ACTIVE my->skill[1]

bool actMinotaurTimer(Entity* my)
{
	node_t* node;

	MINOTAURTIMER_LIFE++;
	if (( (currentlevel < 25 && MINOTAURTIMER_LIFE == TICKS_PER_SECOND * 120)
			|| (currentlevel >= 25 && MINOTAURTIMER_LIFE == TICKS_PER_SECOND * 180)
		)
		&& rand() % 5 == 0 )   // two minutes if currentlevel < 25, else 3 minutes.
	{
		int c;
		bool spawnedsomebody = false;
		for ( c = 0; c < 9; c++ )
		{
			Uint32 zapLeaderUid = 0;
			Entity* monster = summonMonster(HUMAN, my->x, my->y);
			if ( monster )
			{
				monster->skill[29] = 1; // so we spawn a zap brigadier
				spawnedsomebody = true;
				if ( !zapLeaderUid )
				{
					zapLeaderUid = monster->getUID();
				}
				else
				{
					Stat* monsterStats = monster->getStats();
					monsterStats->leader_uid = zapLeaderUid;
				}
			}
		}

		if ( spawnedsomebody )
		{
#ifdef MUSIC
			fadein_increment = default_fadein_increment * 20;
			fadeout_increment = default_fadeout_increment * 5;
			playmusic( sounds[175], false, true, false);
#endif
			for ( c = 0; c < MAXPLAYERS; c++ )
			{
				Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 255);
				messagePlayerColor(c, color, language[1114], stats[c]->name);
			}
		}
	}
	else if (( (currentlevel < 25 && MINOTAURTIMER_LIFE >= TICKS_PER_SECOND * 150)
					|| (currentlevel >= 25 && MINOTAURTIMER_LIFE >= TICKS_PER_SECOND * 210)
				)
		&& !MINOTAURTIMER_ACTIVE )     // two and a half minutes if currentlevel < 25, else 3.5 minutes
	{
		Entity* monster = summonMonster(MINOTAUR, my->x, my->y);
		if ( monster )
		{
			int c;
			for ( c = 0; c < MAXPLAYERS; c++ )
			{
				playSoundPlayer( c, 107 + rand() % 3, 128 );
				Uint32 color = SDL_MapRGB(mainsurface->format, 255, 128, 0);
				messagePlayerColor(c, color, language[1115]);
			}
			MINOTAURTIMER_ACTIVE = MINOTAURTIMER_LIFE;
		}
	}
	if ( MINOTAURTIMER_ACTIVE && MINOTAURTIMER_LIFE >= MINOTAURTIMER_ACTIVE + TICKS_PER_SECOND * 3 )
	{
		int c;
		for ( c = 0; c < MAXPLAYERS; c++ )
		{
			if ( currentlevel < 25 )
			{
				playSoundPlayer(c, 120 + rand() % 3, 128);
				Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 255);
				messagePlayerColor(c, color, language[1116]);
				messagePlayerColor(c, color, language[73]);
			}
			else
			{
				playSoundPlayer(c, 375, 128);
				playSoundPlayer(c, 379, 128);
				messagePlayerColor(c, uint32ColorOrange(*mainsurface), language[1116]);
				messagePlayerColor(c, uint32ColorOrange(*mainsurface), language[73]);
				messagePlayerColor(c, uint32ColorBaronyBlue(*mainsurface), language[73]);
			}
		}
		list_RemoveNode(my->mynode);
		return false;
	}
	return true;
}

bool actMinotaurCeilingBuster(Entity* my)
{
	double x, y;

	// levitate particles
	int u = std::min<unsigned int>(std::max<int>(0, my->x / 16), map.width - 1);
	int v = std::min<unsigned int>(std::max<int>(0, my->y / 16), map.height - 1);
	if ( !map.tiles[v * MAPLAYERS + u * MAPLAYERS * map.height] )
	{
		int c;
		for ( c = 0; c < 2; c++ )
		{
			Entity* entity = newEntity(171, 1, map.entities, nullptr); //Particle entity.
			entity->x = my->x - 8 + rand() % 17;
			entity->y = my->y - 8 + rand() % 17;
			entity->z = 10 + rand() % 3;
			entity->scalex = 0.7;
			entity->scaley = 0.7;
			entity->scalez = 0.7;
			entity->sizex = 1;
			entity->sizey = 1;
			entity->yaw = (rand() % 360) * PI / 180.f;
			entity->flags[PASSABLE] = true;
			entity->flags[BRIGHT] = true;
			entity->flags[NOUPDATE] = true;
			entity->flags[UNCLICKABLE] = true;
			entity->behavior = &actMagicParticle;
			if ( multiplayer != CLIENT )
			{
				entity_uids--;
			}
			entity->setUID(-3);
		}
	}

	// bust ceilings
	for ( x = my->x - my->sizex - 1; x <= my->x + my->sizex + 1; x += 1 )
	{
		for ( y = my->y - my->sizey - 1; y <= my->y + my->sizey + 1; y += 1 )
		{
			if ( x >= 0 && y >= 0 && x < map.width << 4 && y < map.height << 4 )
			{
				int index = (MAPLAYERS - 1) + ((int)floor(y / 16)) * MAPLAYERS + ((int)floor(x / 16)) * MAPLAYERS * map.height;
				if ( map.tiles[index] )
				{
					if ( my->monsterAttack == 0 )
					{
						if ( multiplayer != CLIENT )
						{
							my->attack(MONSTER_POSE_MELEE_WINDUP2, 0, nullptr);
						}
						return true;
					}
					else if ( my->monsterAttack == MONSTER_POSE_MELEE_WINDUP2 )
					{
						return true;
					}
					map.tiles[index] = 0;
					if ( multiplayer != CLIENT )
					{
						playSoundEntity(my, 67, 128);
						//MONSTER_ATTACK = 1;
						Stat* myStats = my->getStats();
						if ( myStats )
						{
							// easy hack to stop the minotaur while he breaks stuff
							myStats->EFFECTS[EFF_PARALYZED] = true;
							myStats->EFFECTS_TIMERS[EFF_PARALYZED] = 10;
						}
					}

					// spawn several rock particles (NOT items)
					int c, i = 6 + rand() % 4;
					for ( c = 0; c < i; c++ )
					{
						Entity *entity = nullptr;
						if ( multiplayer == SERVER )
						{
							entity = spawnGib(my);
						}
						else
						{
							entity = spawnGibClient(my->x, my->y, my->z, 5);
						}
						if ( entity )
						{
							entity->x = ((int)(my->x / 16)) * 16 + rand() % 16;
							entity->y = ((int)(my->y / 16)) * 16 + rand() % 16;
							entity->z = -8;
							entity->flags[PASSABLE] = true;
							entity->flags[INVISIBLE] = false;
							entity->flags[NOUPDATE] = true;
							entity->flags[UPDATENEEDED] = false;
							entity->sprite = items[GEM_ROCK].index;
							entity->yaw = rand() % 360 * PI / 180;
							entity->pitch = rand() % 360 * PI / 180;
							entity->roll = rand() % 360 * PI / 180;
							entity->vel_x = (rand() % 20 - 10) / 10.0;
							entity->vel_y = (rand() % 20 - 10) / 10.0;
							entity->vel_z = -.25;
							entity->fskill[3] = 0.03;
						}
					}
				}
				node_t* node, *nextnode;
				std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 2);
				for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end(); ++it )
				{
					list_t* currentList = *it;
					for ( node = currentList->first; node != nullptr; node = nextnode )
					{
						nextnode = node->next;
						Entity* entity = (Entity*)node->element;
						if ( (int)(x / 16) == (int)(entity->x / 16) && (int)(y / 16) == (int)(entity->y / 16) )
						{
							if ( entity->behavior == &actDoorFrame )
							{
								// spawn several rock items
								int c, i = 8 + rand() % 4;
								for ( c = 0; c < i; c++ )
								{
									Entity *entity = nullptr;
									if ( multiplayer == SERVER )
									{
										entity = spawnGib(my);
									}
									else
									{
										entity = spawnGibClient(my->x, my->y, my->z, 5);
									}
									if ( entity )
									{
										entity->x = ((int)(my->x / 16)) * 16 + rand() % 16;
										entity->y = ((int)(my->y / 16)) * 16 + rand() % 16;
										entity->z = -8;
										entity->flags[PASSABLE] = true;
										entity->flags[INVISIBLE] = false;
										entity->flags[NOUPDATE] = true;
										entity->flags[UPDATENEEDED] = false;
										entity->sprite = items[GEM_ROCK].index;
										entity->yaw = rand() % 360 * PI / 180;
										entity->pitch = rand() % 360 * PI / 180;
										entity->roll = rand() % 360 * PI / 180;
										entity->vel_x = (rand() % 20 - 10) / 10.0;
										entity->vel_y = (rand() % 20 - 10) / 10.0;
										entity->vel_z = -.25;
										entity->fskill[3] = 0.03;
									}
								}
								list_RemoveNode(entity->mynode);
							}
							else if ( entity->behavior == &actDoor )
							{
								if ( multiplayer != CLIENT )
								{
									entity->skill[4] = 0; // destroy the door
								}
							}
							else if ( entity->behavior == &actGate )
							{
								if ( multiplayer != CLIENT )
								{
									playSoundEntity(entity, 76, 64);
									list_RemoveNode(entity->mynode);
								}
							}
							else if (	entity->behavior == &actStalagCeiling	||
										entity->behavior == &actStalagFloor		||
										entity->behavior == &actStalagColumn
									)
							{
								// spawn several rock items
								int c, i = rand() % 4;
								for ( c = 0; c < i; ++c )
								{
									//Entity* childEntity = spawnGib(my);
									Entity *childEntity = nullptr;
									if ( multiplayer == SERVER )
									{
										childEntity = spawnGib(my);
									}
									else
									{
										childEntity = spawnGibClient(my->x, my->y, my->z, 5);
									}
									if ( entity )
									{
										childEntity->x = ((int)(my->x / 16)) * 16 + rand() % 16;
										childEntity->y = ((int)(my->y / 16)) * 16 + rand() % 16;
										childEntity->z = -8;
										childEntity->flags[PASSABLE] = true;
										childEntity->flags[INVISIBLE] = false;
										childEntity->flags[NOUPDATE] = true;
										childEntity->flags[UPDATENEEDED] = false;
										childEntity->sprite = items[GEM_ROCK].index;
										childEntity->yaw = rand() % 360 * PI / 180;
										childEntity->pitch = rand() % 360 * PI / 180;
										childEntity->roll = rand() % 360 * PI / 180;
										childEntity->vel_x = (rand() % 20 - 10) / 10.0;
										childEntity->vel_y = (rand() % 20 - 10) / 10.0;
										childEntity->vel_z = -.25;
										childEntity->fskill[3] = 0.03;
									}
								}
								list_RemoveNode(entity->mynode);
							}
						}
					}
				}
				return false;
			}
		}
	}
	return true;
}

void createMinotaurTimer(Entity* entity, map_t* map)
{
	Entity* childEntity = newEntity(37, 0, map->entities, nullptr); //Timer entity.
	childEntity->sizex = 2;
	childEntity->sizey = 2;
	childEntity->x = entity->x;
	childEntity->y = entity->y;
	childEntity->behavior = &actMinotaurTimer;
	childEntity->flags[SPRITE] = true;
	childEntity->flags[INVISIBLE] = true;
	childEntity->flags[PASSABLE] = true;
	childEntity->flags[NOUPDATE] = true;
	childEntity->setUID(-3);
	entity_uids--;
}
