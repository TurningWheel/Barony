/*-------------------------------------------------------------------------------

	BARONY
	File: monster_skeleton.cpp
	Desc: implements all of the skeleton monster's code

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
#include "player.hpp"

void initSkeleton(Entity* my, Stat* myStats)
{
	int c;
	node_t* node;

	//Sprite 229 = Skeleton head model
	my->initMonster(229);

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = -1;
		MONSTER_SPOTVAR = 1;
		MONSTER_IDLESND = -1;
		MONSTER_IDLEVAR = 1;
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
			if ( rand() % 50 || my->flags[USERFLAG2] )
			{
				if ( strncmp(map.name, "Underworld", 10) )
				{
					switch ( rand() % 10 )
					{
						case 0:
						case 1:
							myStats->weapon = newItem(BRONZE_AXE, WORN, -1 + rand() % 2, 1, rand(), false, NULL);
							break;
						case 2:
						case 3:
							myStats->weapon = newItem(BRONZE_SWORD, WORN, -1 + rand() % 2, 1, rand(), false, NULL);
							break;
						case 4:
						case 5:
							myStats->weapon = newItem(IRON_SPEAR, WORN, -1 + rand() % 2, 1, rand(), false, NULL);
							break;
						case 6:
						case 7:
							myStats->weapon = newItem(IRON_AXE, WORN, -1 + rand() % 2, 1, rand(), false, NULL);
							break;
						case 8:
						case 9:
							myStats->weapon = newItem(IRON_SWORD, WORN, -1 + rand() % 2, 1, rand(), false, NULL);
							break;
					}
				}
			}
			else
			{
				myStats->HP = 100;
				myStats->MAXHP = 100;
				strcpy(myStats->name, "Funny Bones");
				myStats->weapon = newItem(ARTIFACT_AXE, EXCELLENT, 1, 1, rand(), true, NULL);
				myStats->cloak = newItem(CLOAK_PROTECTION, WORN, 0, 1, 2, true, NULL);
			}

			// random effects

			// generates equipment and weapons if available from editor
			createMonsterEquipment(myStats);

			// create any custom inventory items from editor if available
			createCustomInventory(myStats, customItemsToGenerate);

			// count if any custom inventory items from editor
			int customItems = countCustomItems(myStats); //max limit of 6 custom items per entity.

														 // count any inventory items set to default in edtior
			int defaultItems = countDefaultItems(myStats);

			// generate the default inventory items for the monster, provided the editor sprite allowed enough default slots
			switch ( defaultItems )
			{
				case 6:
				case 5:
				case 4:
				case 3:
				case 2:
				case 1:
					break;
				default:
					break;
			}

			//give weapon
			if ( myStats->weapon == NULL && myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1 )
			{
				switch ( rand() % 10 )
				{
					case 0:
					case 1:
					case 2:
					case 3:
						myStats->weapon = newItem(SHORTBOW, WORN, -1 + rand() % 2, 1, rand(), false, NULL);
						break;
					case 4:
					case 5:
					case 6:
					case 7:
						myStats->weapon = newItem(CROSSBOW, WORN, -1 + rand() % 2, 1, rand(), false, NULL);
						break;
					case 8:
					case 9:
						myStats->weapon = newItem(MAGICSTAFF_COLD, EXCELLENT, -1 + rand() % 2, 1, rand(), false, NULL);
						break;
				}
			}

			//give helmet
			if ( myStats->helmet == NULL && myStats->EDITOR_ITEMS[ITEM_SLOT_HELM] == 1 )
			{
				switch ( rand() % 10 )
				{
					case 0:
					case 1:
					case 2:
					case 3:
					case 4:
						break;
					case 5:
						myStats->helmet = newItem(LEATHER_HELM, DECREPIT, -1 + rand() % 2, 1, 0, false, NULL);
						break;
					case 6:
					case 7:
					case 8:
					case 9:
						myStats->helmet = newItem(IRON_HELM, DECREPIT, -1 + rand() % 2, 1, 0, false, NULL);
						break;
				}
			}

			//give shield
			if ( myStats->shield == NULL && myStats->EDITOR_ITEMS[ITEM_SLOT_SHIELD] == 1 )
			{
				switch ( rand() % 10 )
				{
					case 0:
					case 1:
					case 2:
					case 3:
					case 4:
					case 5:
						break;
					case 6:
					case 7:
						myStats->shield = newItem(WOODEN_SHIELD, DECREPIT, -1 + rand() % 2, 1, rand(), false, NULL);
						break;
					case 8:
						myStats->shield = newItem(BRONZE_SHIELD, DECREPIT, -1 + rand() % 2, 1, rand(), false, NULL);
						break;
					case 9:
						myStats->shield = newItem(IRON_SHIELD, DECREPIT, -1 + rand() % 2, 1, rand(), false, NULL);
						break;
				}
			}
		}
	}

	// torso
	Entity* entity = newEntity(230, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SKELETON][1][0]; // 0
	entity->focaly = limbs[SKELETON][1][1]; // 0
	entity->focalz = limbs[SKELETON][1][2]; // 0
	entity->behavior = &actSkeletonLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// right leg
	entity = newEntity(236, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SKELETON][2][0]; // 0
	entity->focaly = limbs[SKELETON][2][1]; // 0
	entity->focalz = limbs[SKELETON][2][2]; // 2
	entity->behavior = &actSkeletonLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// left leg
	entity = newEntity(235, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SKELETON][3][0]; // 0
	entity->focaly = limbs[SKELETON][3][1]; // 0
	entity->focalz = limbs[SKELETON][3][2]; // 2
	entity->behavior = &actSkeletonLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// right arm
	entity = newEntity(233, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SKELETON][4][0]; // 0
	entity->focaly = limbs[SKELETON][4][1]; // 0
	entity->focalz = limbs[SKELETON][4][2]; // 2
	entity->behavior = &actSkeletonLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// left arm
	entity = newEntity(231, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SKELETON][5][0]; // 0
	entity->focaly = limbs[SKELETON][5][1]; // 0
	entity->focalz = limbs[SKELETON][5][2]; // 2
	entity->behavior = &actSkeletonLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// world weapon
	entity = newEntity(-1, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[INVISIBLE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SKELETON][6][0]; // 2.5
	entity->focaly = limbs[SKELETON][6][1]; // 0
	entity->focalz = limbs[SKELETON][6][2]; // 0
	entity->behavior = &actSkeletonLimb;
	entity->parent = my->getUID();
	entity->pitch = .25;
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// shield
	entity = newEntity(-1, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[INVISIBLE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SKELETON][7][0]; // 2
	entity->focaly = limbs[SKELETON][7][1]; // 0
	entity->focalz = limbs[SKELETON][7][2]; // 0
	entity->behavior = &actSkeletonLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// cloak
	entity = newEntity(-1, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->scalex = 1.01;
	entity->scaley = 1.01;
	entity->scalez = 1.01;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[INVISIBLE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SKELETON][8][0]; // 0
	entity->focaly = limbs[SKELETON][8][1]; // 0
	entity->focalz = limbs[SKELETON][8][2]; // 4
	entity->behavior = &actSkeletonLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// helmet
	entity = newEntity(-1, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->scalex = 1.01;
	entity->scaley = 1.01;
	entity->scalez = 1.01;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[INVISIBLE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SKELETON][9][0]; // 0
	entity->focaly = limbs[SKELETON][9][1]; // 0
	entity->focalz = limbs[SKELETON][9][2]; // -2
	entity->behavior = &actSkeletonLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// mask
	entity = newEntity(-1, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[INVISIBLE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SKELETON][10][0]; // 0
	entity->focaly = limbs[SKELETON][10][1]; // 0
	entity->focalz = limbs[SKELETON][10][2]; // .5
	entity->behavior = &actSkeletonLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	if ( multiplayer == CLIENT || MONSTER_INIT )
	{
		return;
	}
}

void actSkeletonLimb(Entity* my)
{
	my->actMonsterLimb(true);
}

void skeletonDie(Entity* my)
{
	my->removeMonsterDeathNodes();

	int c;
	for ( c = 0; c < 6; c++ )
	{
		Entity* entity = spawnGib(my);
		if ( entity )
		{
			switch ( c )
			{
				case 0:
					entity->sprite = 229;
					break;
				case 1:
					entity->sprite = 230;
					break;
				case 2:
					entity->sprite = 231;
					break;
				case 3:
					entity->sprite = 233;
					break;
				case 4:
					entity->sprite = 235;
					break;
				case 5:
					entity->sprite = 236;
					break;
			}
			serverSpawnGibForClient(entity);
		}
	}
	playSoundEntity(my, 94, 128);
	list_RemoveNode(my->mynode);
	return;
}

#define SKELETONWALKSPEED .13

void skeletonMoveBodyparts(Entity* my, Stat* myStats, double dist)
{
	node_t* node;
	Entity* entity = NULL, *entity2 = NULL;
	Entity* rightbody = NULL;
	Entity* weaponarm = NULL;
	int bodypart;
	bool wearingring = false;

	// set invisibility //TODO: isInvisible()?
	if ( multiplayer != CLIENT )
	{
		if ( myStats->ring != NULL )
			if ( myStats->ring->type == RING_INVISIBILITY )
			{
				wearingring = true;
			}
		if ( myStats->cloak != NULL )
			if ( myStats->cloak->type == CLOAK_INVISIBILITY )
			{
				wearingring = true;
			}
		if ( myStats->EFFECTS[EFF_INVISIBLE] == true || wearingring == true )
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

		// sleeping
		if ( myStats->EFFECTS[EFF_ASLEEP] )
		{
			my->z = 2;
			my->pitch = PI / 4;
		}
		else
		{
			my->z = -.5;
			my->pitch = 0;
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
		if ( bodypart == 3 || bodypart == 6 )
		{
			if ( bodypart == 3 )
			{
				rightbody = (Entity*)node->next->element;
			}
			node_t* shieldNode = list_Node(&my->children, 7);
			if ( shieldNode )
			{
				Entity* shield = (Entity*)shieldNode->element;
				if ( dist > 0.1 && (bodypart != 6 || shield->flags[INVISIBLE]) )
				{
					if ( !rightbody->skill[0] )
					{
						entity->pitch -= dist * SKELETONWALKSPEED;
						if ( entity->pitch < -PI / 4.0 )
						{
							entity->pitch = -PI / 4.0;
							if (bodypart == 3)
							{
								entity->skill[0] = 1;
								if ( dist > .1 )
								{
									playSoundEntityLocal(my, 95, 32);
								}
							}
						}
					}
					else
					{
						entity->pitch += dist * SKELETONWALKSPEED;
						if ( entity->pitch > PI / 4.0 )
						{
							entity->pitch = PI / 4.0;
							if (bodypart == 3)
							{
								entity->skill[0] = 0;
								if ( dist > .1 )
								{
									playSoundEntityLocal(my, 95, 32);
								}
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
		}
		else if ( bodypart == 4 || bodypart == 5 || bodypart == 9 )
		{
			if ( bodypart == 5 )
			{
				weaponarm = entity;
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
			else if ( bodypart == 9 )
			{
				entity->pitch = entity->fskill[0];
			}

			if ( bodypart != 5 || (MONSTER_ATTACK == 0 && MONSTER_ATTACKTIME == 0) )
			{
				if ( dist > 0.1 )
				{
					if ( entity->skill[0] )
					{
						entity->pitch -= dist * SKELETONWALKSPEED;
						if ( entity->pitch < -PI / 4.0 )
						{
							entity->skill[0] = 0;
							entity->pitch = -PI / 4.0;
						}
					}
					else
					{
						entity->pitch += dist * SKELETONWALKSPEED;
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
			if ( bodypart == 9 )
			{
				entity->fskill[0] = entity->pitch;
				entity->roll = my->roll - fabs(entity->pitch) / 2;
				entity->pitch = 0;
			}
		}
		switch ( bodypart )
		{
			// torso
			case 2:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->breastplate == NULL )
					{
						entity->sprite = 230;
					}
					else
					{
						entity->sprite = itemModel(myStats->breastplate);
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->skill[10] != entity->sprite )
						{
							entity->skill[10] = entity->sprite;
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
						{
							serverUpdateEntityBodypart(my, bodypart);
						}
					}
				}
				entity->x -= .25 * cos(my->yaw);
				entity->y -= .25 * sin(my->yaw);
				entity->z += 2;
				break;
			// right leg
			case 3:
				entity->sprite = 236;
				entity->x += 1 * cos(my->yaw + PI / 2) + .25 * cos(my->yaw);
				entity->y += 1 * sin(my->yaw + PI / 2) + .25 * sin(my->yaw);
				entity->z += 4;
				if ( my->z >= 1.9 && my->z <= 2.1 )
				{
					entity->yaw += PI / 8;
					entity->pitch = -PI / 2;
				}
				break;
			// left leg
			case 4:
				entity->sprite = 235;
				entity->x -= 1 * cos(my->yaw + PI / 2) - .25 * cos(my->yaw);
				entity->y -= 1 * sin(my->yaw + PI / 2) - .25 * sin(my->yaw);
				entity->z += 4;
				if ( my->z >= 1.9 && my->z <= 2.1 )
				{
					entity->yaw -= PI / 8;
					entity->pitch = -PI / 2;
				}
				break;
			// right arm
			case 5:
			{
				entity->sprite = 233;
				node_t* weaponNode = list_Node(&my->children, 7);
				if ( weaponNode )
				{
					Entity* weapon = (Entity*)weaponNode->element;
					if ( !MONSTER_ARMBENDED )
					{
						entity->sprite += (weapon->flags[INVISIBLE] != true);
					}
					if ( weapon->flags[INVISIBLE] || MONSTER_ARMBENDED )
					{
						entity->focalx = limbs[SKELETON][4][0]; // 0
						entity->focaly = limbs[SKELETON][4][1]; // 0
						entity->focalz = limbs[SKELETON][4][2]; // 2
					}
					else
					{
						entity->focalx = limbs[SKELETON][4][0] + 1; // 1
						entity->focaly = limbs[SKELETON][4][1]; // 0
						entity->focalz = limbs[SKELETON][4][2] - 1; // 1
					}
				}
				entity->x += 1.75 * cos(my->yaw + PI / 2) - .20 * cos(my->yaw);
				entity->y += 1.75 * sin(my->yaw + PI / 2) - .20 * sin(my->yaw);
				entity->z += .5;
				entity->yaw += MONSTER_WEAPONYAW;
				if ( my->z >= 1.9 && my->z <= 2.1 )
				{
					entity->pitch = 0;
				}
				break;
				// left arm
			}
			case 6:
			{
				entity->sprite = 231;
				node_t* shieldNode = list_Node(&my->children, 8);
				if ( shieldNode )
				{
					Entity* shield = (Entity*)shieldNode->element;
					entity->sprite += (shield->flags[INVISIBLE] != true);
					if ( shield->flags[INVISIBLE] )
					{
						entity->focalx = limbs[SKELETON][5][0]; // 0
						entity->focaly = limbs[SKELETON][5][1]; // 0
						entity->focalz = limbs[SKELETON][5][2]; // 2
					}
					else
					{
						entity->focalx = limbs[SKELETON][5][0] + 1; // 1
						entity->focaly = limbs[SKELETON][5][1]; // 0
						entity->focalz = limbs[SKELETON][5][2] - 1; // 1
					}
				}
				entity->x -= 1.75 * cos(my->yaw + PI / 2) + .20 * cos(my->yaw);
				entity->y -= 1.75 * sin(my->yaw + PI / 2) + .20 * sin(my->yaw);
				entity->z += .5;
				if ( my->z >= 1.9 && my->z <= 2.1 )
				{
					entity->pitch = 0;
				}
				break;
			}
			// weapon
			case 7:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->weapon == NULL || myStats->EFFECTS[EFF_INVISIBLE] || wearingring ) //TODO: isInvisible()?
					{
						entity->flags[INVISIBLE] = true;
					}
					else
					{
						entity->sprite = itemModel(myStats->weapon);
						if ( itemCategory(myStats->weapon) == SPELLBOOK )
						{
							entity->flags[INVISIBLE] = true;
						}
						else
						{
							entity->flags[INVISIBLE] = false;
						}
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->skill[10] != entity->sprite )
						{
							entity->skill[10] = entity->sprite;
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( entity->skill[11] != entity->flags[INVISIBLE] )
						{
							entity->skill[11] = entity->flags[INVISIBLE];
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
						{
							serverUpdateEntityBodypart(my, bodypart);
						}
					}
				}
				if ( weaponarm != NULL )
				{
					if ( entity->flags[INVISIBLE] != true )
					{
						if ( entity->sprite == items[SHORTBOW].index )
						{
							entity->x = weaponarm->x - .5 * cos(weaponarm->yaw);
							entity->y = weaponarm->y - .5 * sin(weaponarm->yaw);
							entity->z = weaponarm->z + 1;
							entity->pitch = weaponarm->pitch + .25;
						}
						else if ( entity->sprite == items[ARTIFACT_BOW].index )
						{
							entity->x = weaponarm->x - 1.5 * cos(weaponarm->yaw);
							entity->y = weaponarm->y - 1.5 * sin(weaponarm->yaw);
							entity->z = weaponarm->z + 2;
							entity->pitch = weaponarm->pitch + .25;
						}
						else if ( entity->sprite == items[CROSSBOW].index )
						{
							entity->x = weaponarm->x;
							entity->y = weaponarm->y;
							entity->z = weaponarm->z + 1;
							entity->pitch = weaponarm->pitch;
						}
						else
						{
							entity->x = weaponarm->x + .5 * cos(weaponarm->yaw) * (MONSTER_ATTACK == 0);
							entity->y = weaponarm->y + .5 * sin(weaponarm->yaw) * (MONSTER_ATTACK == 0);
							entity->z = weaponarm->z - .5 * (MONSTER_ATTACK == 0);
							entity->pitch = weaponarm->pitch + .25 * (MONSTER_ATTACK == 0);
						}
					}
					entity->yaw = weaponarm->yaw;
					entity->roll = weaponarm->roll;
					if ( !MONSTER_ARMBENDED )
					{
						entity->focalx = limbs[SKELETON][6][0]; // 2.5
						if ( entity->sprite == items[CROSSBOW].index )
						{
							entity->focalx += 2;
						}
						entity->focaly = limbs[SKELETON][6][1]; // 0
						entity->focalz = limbs[SKELETON][6][2]; // -.5
					}
					else
					{
						entity->focalx = limbs[SKELETON][6][0] + 1; // 3.5
						entity->focaly = limbs[SKELETON][6][1]; // 0
						entity->focalz = limbs[SKELETON][6][2] - 2; // -2.5
						entity->yaw -= sin(weaponarm->roll) * PI / 2;
						entity->pitch += cos(weaponarm->roll) * PI / 2;
					}
				}
				break;
			// shield
			case 8:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->shield == NULL )
					{
						entity->flags[INVISIBLE] = true;
						entity->sprite = 0;
					}
					else
					{
						entity->flags[INVISIBLE] = false;
						entity->sprite = itemModel(myStats->shield);
					}
					if ( myStats->EFFECTS[EFF_INVISIBLE] || wearingring ) //TODO: isInvisible()?
					{
						entity->flags[INVISIBLE] = true;
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->skill[10] != entity->sprite )
						{
							entity->skill[10] = entity->sprite;
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( entity->skill[11] != entity->flags[INVISIBLE] )
						{
							entity->skill[11] = entity->flags[INVISIBLE];
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
						{
							serverUpdateEntityBodypart(my, bodypart);
						}
					}
				}
				entity->x -= 2.5 * cos(my->yaw + PI / 2) + .20 * cos(my->yaw);
				entity->y -= 2.5 * sin(my->yaw + PI / 2) + .20 * sin(my->yaw);
				entity->z += 2.5;
				if ( entity->sprite == items[TOOL_TORCH].index )
				{
					entity2 = spawnFlame(entity, SPRITE_FLAME);
					entity2->x += 2 * cos(my->yaw);
					entity2->y += 2 * sin(my->yaw);
					entity2->z -= 2;
				}
				else if ( entity->sprite == items[TOOL_CRYSTALSHARD].index )
				{
					entity2 = spawnFlame(entity, SPRITE_CRYSTALFLAME);
					entity2->x += 2 * cos(my->yaw);
					entity2->y += 2 * sin(my->yaw);
					entity2->z -= 2;
				}
				else if ( entity->sprite == items[TOOL_LANTERN].index )
				{
					entity->z += 2;
					entity2 = spawnFlame(entity, SPRITE_FLAME);
					entity2->x += 2 * cos(my->yaw);
					entity2->y += 2 * sin(my->yaw);
					entity2->z += 1;
				}
				break;
			// cloak
			case 9:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->cloak == NULL || myStats->EFFECTS[EFF_INVISIBLE] || wearingring ) //TODO: isInvisible()?
					{
						entity->flags[INVISIBLE] = true;
					}
					else
					{
						entity->flags[INVISIBLE] = false;
						entity->sprite = itemModel(myStats->cloak);
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->skill[10] != entity->sprite )
						{
							entity->skill[10] = entity->sprite;
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( entity->skill[11] != entity->flags[INVISIBLE] )
						{
							entity->skill[11] = entity->flags[INVISIBLE];
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
						{
							serverUpdateEntityBodypart(my, bodypart);
						}
					}
				}
				entity->x -= cos(my->yaw);
				entity->y -= sin(my->yaw);
				entity->yaw += PI / 2;
				break;
			// helm
			case 10:
				entity->focalx = limbs[SKELETON][9][0]; // 0
				entity->focaly = limbs[SKELETON][9][1]; // 0
				entity->focalz = limbs[SKELETON][9][2]; // -2
				entity->pitch = my->pitch;
				entity->roll = 0;
				if ( multiplayer != CLIENT )
				{
					entity->sprite = itemModel(myStats->helmet);
					if ( myStats->helmet == NULL || myStats->EFFECTS[EFF_INVISIBLE] || wearingring ) //TODO: isInvisible()?
					{
						entity->flags[INVISIBLE] = true;
					}
					else
					{
						entity->flags[INVISIBLE] = false;
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->skill[10] != entity->sprite )
						{
							entity->skill[10] = entity->sprite;
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( entity->skill[11] != entity->flags[INVISIBLE] )
						{
							entity->skill[11] = entity->flags[INVISIBLE];
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
						{
							serverUpdateEntityBodypart(my, bodypart);
						}
					}
				}
				if ( entity->sprite != items[STEEL_HELM].index )
				{
					if ( entity->sprite == items[HAT_PHRYGIAN].index )
					{
						entity->focalx = limbs[SKELETON][9][0] - .5; // -.5
						entity->focaly = limbs[SKELETON][9][1] - 3.25; // -3.25
						entity->focalz = limbs[SKELETON][9][2] + 2.5; // .5
						entity->roll = PI / 2;
					}
					else if ( entity->sprite >= items[HAT_HOOD].index && entity->sprite < items[HAT_HOOD].index + items[HAT_HOOD].variations )
					{
						entity->focalx = limbs[SKELETON][9][0] - .5; // -.5
						entity->focaly = limbs[SKELETON][9][1] - 2.5; // -2.5
						entity->focalz = limbs[SKELETON][9][2] + 2.5; // 2.5
						entity->roll = PI / 2;
					}
					else if ( entity->sprite == items[HAT_WIZARD].index )
					{
						entity->focalx = limbs[SKELETON][9][0]; // 0
						entity->focaly = limbs[SKELETON][9][1] - 4.75; // -4.75
						entity->focalz = limbs[SKELETON][9][2] + .5; // .5
						entity->roll = PI / 2;
					}
					else if ( entity->sprite == items[HAT_JESTER].index )
					{
						entity->focalx = limbs[SKELETON][9][0]; // 0
						entity->focaly = limbs[SKELETON][9][1] - 4.75; // -4.75
						entity->focalz = limbs[SKELETON][9][2] + .5; // .5
						entity->roll = PI / 2;
					}
				}
				else
				{
					my->flags[INVISIBLE] = true;
				}
				break;
			// mask
			case 11:
				entity->focalx = limbs[SKELETON][10][0]; // 0
				entity->focaly = limbs[SKELETON][10][1]; // 0
				entity->focalz = limbs[SKELETON][10][2]; // .5
				entity->pitch = my->pitch;
				entity->roll = PI / 2;
				if ( multiplayer != CLIENT )
				{
					if ( myStats->mask == NULL || myStats->EFFECTS[EFF_INVISIBLE] || wearingring ) //TODO: isInvisible()?
					{
						entity->flags[INVISIBLE] = true;
					}
					else
					{
						entity->flags[INVISIBLE] = false;
					}
					if ( myStats->mask != NULL )
					{
						if ( myStats->mask->type == TOOL_GLASSES )
						{
							entity->sprite = 165; // GlassesWorn.vox
						}
						else
						{
							entity->sprite = itemModel(myStats->mask);
						}
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->skill[10] != entity->sprite )
						{
							entity->skill[10] = entity->sprite;
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( entity->skill[11] != entity->flags[INVISIBLE] )
						{
							entity->skill[11] = entity->flags[INVISIBLE];
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
						{
							serverUpdateEntityBodypart(my, bodypart);
						}
					}
				}
				if ( entity->sprite != 165 )
				{
					entity->focalx = limbs[SKELETON][10][0] + .35; // .35
					entity->focaly = limbs[SKELETON][10][1] - 2; // -2
					entity->focalz = limbs[SKELETON][10][2]; // .5
				}
				else
				{
					entity->focalx = limbs[SKELETON][10][0] + .25; // .25
					entity->focaly = limbs[SKELETON][10][1] - 2.25; // -2.25
					entity->focalz = limbs[SKELETON][10][2]; // .5
				}
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
