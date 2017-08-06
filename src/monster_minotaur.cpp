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

void initMinotaur(Entity* my, Stat* myStats)
{
	int c;
	node_t* node;

	my->sprite = 239;

	//my->flags[GENIUS]=true;
	my->flags[UPDATENEEDED] = true;
	my->flags[BLOCKSIGHT] = true;
	my->flags[INVISIBLE] = false;

	if ( localPlayerNetworkType != NetworkType::CLIENT )
	{
		MONSTER_SPOTSND = 107;
		MONSTER_SPOTVAR = 3;
		MONSTER_IDLESND = 110;
		MONSTER_IDLEVAR = 3;
	}
	if ( localPlayerNetworkType != NetworkType::CLIENT && !MONSTER_INIT )
	{
		myStats->sex = MALE;
		myStats->appearance = rand();
		strcpy(myStats->name, "");
		myStats->inventory.first = NULL;
		myStats->inventory.last = NULL;
		myStats->HP = 400;
		myStats->MAXHP = 400;
		myStats->MP = 100;
		myStats->MAXMP = 100;
		myStats->OLDHP = myStats->HP;
		if ( strcmp(map.name, "Hell Boss") )
		{
			myStats->STR = 35;
			myStats->DEX = 15;
			myStats->CON = 15;
		}
		else
		{
			myStats->STR = 50;
			myStats->DEX = 20;
			myStats->CON = 20;
		}
		myStats->INT = 5;
		myStats->PER = 5;
		myStats->CHR = -5;
		myStats->EXP = 0;
		myStats->LVL = 20;
		myStats->GOLD = 0;
		myStats->HUNGER = 900;
		if ( !myStats->leader_uid )
		{
			myStats->leader_uid = 0;
		}
		myStats->FOLLOWERS.first = NULL;
		myStats->FOLLOWERS.last = NULL;
		for ( c = 0; c < std::max(NUMPROFICIENCIES, NUMEFFECTS); c++ )
		{
			if ( c < NUMPROFICIENCIES )
			{
				myStats->PROFICIENCIES[c] = 0;
			}
			if ( c < NUMEFFECTS )
			{
				myStats->EFFECTS[c] = false;
			}
			if ( c < NUMEFFECTS )
			{
				myStats->EFFECTS_TIMERS[c] = 0;
			}
		}
		myStats->helmet = NULL;
		myStats->breastplate = NULL;
		myStats->gloves = NULL;
		myStats->shoes = NULL;
		myStats->shield = NULL;
		myStats->weapon = NULL;
		myStats->cloak = NULL;
		myStats->amulet = NULL;
		myStats->ring = NULL;
		myStats->mask = NULL;

		// minotaurs can traverse waters and pits (pits with magic :))
		myStats->EFFECTS[EFF_LEVITATING] = true;
		myStats->EFFECTS_TIMERS[EFF_LEVITATING] = 0;

		ItemType gemtype = GEM_RUBY;
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
		newItem( gemtype, EXCELLENT, 0, 1, rand(), true, &myStats->inventory );
	}

	// head
	Entity* entity = newEntity(237, 0, map.entities);
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

	// chest
	entity = newEntity(238, 0, map.entities);
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

	// right leg
	entity = newEntity(243, 0, map.entities);
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

	// left leg
	entity = newEntity(242, 0, map.entities);
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

	// right arm
	entity = newEntity(241, 0, map.entities);
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

	// left arm
	entity = newEntity(240, 0, map.entities);
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
}

void actMinotaurLimb(Entity* my)
{
	int i;

	Entity* parent = NULL;
	if ( (parent = uidToEntity(my->skill[2])) == NULL )
	{
		list_RemoveNode(my->mynode);
		return;
	}

	if ( localPlayerNetworkType != NetworkType::CLIENT )
	{
		for ( i = 0; i < MAXPLAYERS; i++ )
		{
			if ( inrange[i] )
			{
				if ( i == 0 && selectedEntity == my )
				{
					parent->skill[13] = i + 1;
				}
				else if ( client_selected[i] == my )
				{
					parent->skill[13] = i + 1;
				}
			}
		}
	}
	return;
}

void minotaurDie(Entity* my)
{
	node_t* node, *nextnode;

	int c;
	for ( c = 0; c < 5; c++ )
	{
		Entity* gib = spawnGib(my);
		serverSpawnGibForClient(gib);
	}
	if (spawn_blood)
	{
		int x, y;
		x = std::min<unsigned int>(std::max<int>(0, my->x / 16), map.width - 1);
		y = std::min<unsigned int>(std::max<int>(0, my->y / 16), map.height - 1);
		if ( map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height] )
		{
			if ( !checkObstacle(my->x, my->y, my, NULL) )
			{
				Entity* entity = newEntity(160, 1, map.entities);
				entity->x = my->x;
				entity->y = my->y;
				entity->z = 7.4 + (rand() % 20) / 100.f;
				entity->parent = my->getUID();
				entity->sizex = 2;
				entity->sizey = 2;
				entity->yaw = (rand() % 360) * PI / 180.0;
				entity->flags[UPDATENEEDED] = true;
				entity->flags[PASSABLE] = true;
			}
		}
	}
	for ( c = 0; c < MAXPLAYERS; c++ )
	{
		playSoundPlayer(c, 114, 128);
	}
	int i = 0;
	for (node = my->children.first; node != NULL; node = nextnode)
	{
		nextnode = node->next;
		if (node->element != NULL && i >= 2)
		{
			Entity* entity = (Entity*)node->element;
			entity->flags[UPDATENEEDED] = false;
			list_RemoveNode(entity->mynode);
		}
		list_RemoveNode(node);
		++i;
	}
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

	// set invisibility
	if ( localPlayerNetworkType != NetworkType::CLIENT )
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
				if ( MONSTER_ATTACK == 1 )
				{
					// vertical chop
					if ( MONSTER_ATTACKTIME == 0 )
					{
						MONSTER_ARMBENDED = 0;
						MONSTER_WEAPONYAW = 0;
						entity->pitch = -3 * PI / 4;
						entity->roll = 0;
					}
					else
					{
						if ( entity->pitch >= -PI / 2 )
						{
							MONSTER_ARMBENDED = 1;
						}
						if ( entity->pitch >= PI / 4 )
						{
							entity->skill[0] = rightbody->skill[0];
							MONSTER_WEAPONYAW = 0;
							entity->pitch = rightbody->pitch;
							entity->roll = 0;
							MONSTER_ARMBENDED = 0;
							MONSTER_ATTACK = 0;
						}
						else
						{
							entity->pitch += .25;
						}
					}
				}
				else if ( MONSTER_ATTACK == 2 )
				{
					// horizontal chop
					if ( MONSTER_ATTACKTIME == 0 )
					{
						MONSTER_ARMBENDED = 1;
						MONSTER_WEAPONYAW = -3 * PI / 4;
						entity->pitch = 0;
						entity->roll = -PI / 2;
					}
					else
					{
						if ( MONSTER_WEAPONYAW >= PI / 8 )
						{
							entity->skill[0] = rightbody->skill[0];
							MONSTER_WEAPONYAW = 0;
							entity->pitch = rightbody->pitch;
							entity->roll = 0;
							MONSTER_ARMBENDED = 0;
							MONSTER_ATTACK = 0;
						}
						else
						{
							MONSTER_WEAPONYAW += .25;
						}
					}
				}
				else if ( MONSTER_ATTACK == 3 )
				{
					// stab
					if ( MONSTER_ATTACKTIME == 0 )
					{
						MONSTER_ARMBENDED = 0;
						MONSTER_WEAPONYAW = 0;
						entity->pitch = 2 * PI / 3;
						entity->roll = 0;
					}
					else
					{
						if ( MONSTER_ATTACKTIME >= 5 )
						{
							MONSTER_ARMBENDED = 1;
							entity->pitch = -PI / 6;
						}
						if ( MONSTER_ATTACKTIME >= 10 )
						{
							entity->skill[0] = rightbody->skill[0];
							MONSTER_WEAPONYAW = 0;
							entity->pitch = rightbody->pitch;
							entity->roll = 0;
							MONSTER_ARMBENDED = 0;
							MONSTER_ATTACK = 0;
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

void actMinotaurTrap(Entity* my)
{
	if ( !my->skill[28] )
	{
		return;
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
}

#define MINOTAURTIMER_LIFE my->skill[0]
#define MINOTAURTIMER_ACTIVE my->skill[1]

void actMinotaurTimer(Entity* my)
{
	node_t* node;

	MINOTAURTIMER_LIFE++;
	if ( MINOTAURTIMER_LIFE == TICKS_PER_SECOND * 120 && rand() % 5 == 0 )   // two minutes
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
	else if ( MINOTAURTIMER_LIFE >= TICKS_PER_SECOND * 150 && !MINOTAURTIMER_ACTIVE )     // two and a half minutes
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
			playSoundPlayer(c, 120 + rand() % 3, 128);
			Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 255);
			messagePlayerColor(c, color, language[1116]);
			messagePlayerColor(c, color, language[73]);
		}
		list_RemoveNode(my->mynode);
		return;
	}
}

void actMinotaurCeilingBuster(Entity* my)
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
			Entity* entity = newEntity(171, 1, map.entities);
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
			if ( localPlayerNetworkType != NetworkType::CLIENT )
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
					map.tiles[index] = 0;
					if ( localPlayerNetworkType != NetworkType::CLIENT )
					{
						playSoundEntity(my, 67, 128);
						MONSTER_ATTACK = 1;
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
						Entity* entity = spawnGib(my);
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
				node_t* node, *nextnode;
				for ( node = map.entities->first; node != NULL; node = nextnode )
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
								Entity* childEntity = spawnGib(my);
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
							list_RemoveNode(entity->mynode);
						}
						else if ( entity->behavior == &actDoor )
						{
							if ( localPlayerNetworkType != NetworkType::CLIENT )
							{
								entity->skill[4] = 0; // destroy the door
							}
						}
						else if ( entity->behavior == &actGate )
						{
							if ( localPlayerNetworkType != NetworkType::CLIENT )
							{
								playSoundEntity(entity, 76, 64);
								list_RemoveNode(entity->mynode);
							}
						}
					}
				}
			}
		}
	}
}

void createMinotaurTimer(Entity* entity, map_t* map)
{
	Entity* childEntity = newEntity(37, 0, map->entities);
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
