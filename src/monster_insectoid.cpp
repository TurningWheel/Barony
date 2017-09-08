/*-------------------------------------------------------------------------------

	BARONY
	File: monster_insectoid.cpp
	Desc: implements all of the insectoid monster's code

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
#include "magic/magic.hpp"

void initInsectoid(Entity* my, Stat* myStats)
{
	int c;
	node_t* node;

 	//Sprite 455 = Insectoid head model
	my->initMonster(455);

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 60;
		MONSTER_SPOTVAR = 3;
		MONSTER_IDLESND = 98;
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
			if ( rand() % 50 || my->flags[USERFLAG2] )
			{
			}
			else
			{
				myStats->HP = 120;
				myStats->MAXHP = 120;
				myStats->OLDHP = myStats->HP;
				strcpy(myStats->name, "The Potato King");
				myStats->weapon = newItem(ARTIFACT_MACE, EXCELLENT, 1, 1, rand(), true, NULL);
				myStats->helmet = newItem(HAT_JESTER, SERVICABLE, 3 + rand() % 3, 1, 0, false, NULL);

				int c;
				for ( c = 0; c < 3; c++ )
				{
					Entity* entity = summonMonster(GOBLIN, my->x, my->y);
					if ( entity )
					{
						entity->parent = my->getUID();
					}
				}
			}

			// random effects
			if ( rand() % 8 == 0 )
			{
				myStats->EFFECTS[EFF_ASLEEP] = true;
				myStats->EFFECTS_TIMERS[EFF_ASLEEP] = 1800 + rand() % 1800;
			}

			// generates equipment and weapons if available from editor
			createMonsterEquipment(myStats);

			// create any custom inventory items from editor if available
			createCustomInventory(myStats, customItemsToGenerate);

			// count if any custom inventory items from editor
			int customItems = countCustomItems(myStats); //max limit of 6 custom items per entity.

														 // count any inventory items set to default in edtior
			int defaultItems = countDefaultItems(myStats);

			// always give special spell to insectoid, undroppable.
			newItem(SPELLBOOK_ACID_SPRAY, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, &myStats->inventory);

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

			//give shield
			//if ( myStats->shield == NULL && myStats->EDITOR_ITEMS[ITEM_SLOT_SHIELD] == 1 )
			//{
			//	// give shield
			//	switch ( rand() % 10 )
			//	{
			//		case 0:
			//		case 1:
			//			myStats->shield = newItem(TOOL_TORCH, SERVICABLE, -1 + rand() % 3, 1, rand(), false, NULL);
			//			break;
			//		case 2:
			//		case 3:
			//		case 4:
			//			break;
			//		case 5:
			//		case 6:
			//			myStats->shield = newItem(WOODEN_SHIELD, DECREPIT, -1 + rand() % 3, 1, rand(), false, NULL);
			//			break;
			//		case 7:
			//		case 8:
			//			myStats->shield = newItem(BRONZE_SHIELD, DECREPIT, -1 + rand() % 3, 1, rand(), false, NULL);
			//			break;
			//		case 9:
			//			myStats->shield = newItem(IRON_SHIELD, DECREPIT, -1 + rand() % 3, 1, rand(), false, NULL);
			//			break;
			//	}
			//}

			//give weapon
			/*if ( myStats->weapon == NULL && myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1 )
			{
				switch ( rand() % 10 )
				{
					case 0:
					case 1:
					case 2:
						myStats->weapon = newItem(SHORTBOW, WORN, -1 + rand() % 3, 1, rand(), false, NULL);
						break;
					case 3:
					case 4:
					case 5:
						myStats->weapon = newItem(BRONZE_AXE, WORN, -1 + rand() % 3, 1, rand(), false, NULL);
						break;
					case 6:
					case 7:
						myStats->weapon = newItem(IRON_MACE, WORN, -1 + rand() % 3, 1, rand(), false, NULL);
						break;
					case 8:
						myStats->weapon = newItem(IRON_AXE, WORN, -1 + rand() % 3, 1, rand(), false, NULL);
						break;
					case 9:
						myStats->weapon = newItem(MAGICSTAFF_FIRE, EXCELLENT, -1 + rand() % 3, 1, rand(), false, NULL);
						break;
				}
			}*/

			// give cloak
			/*if ( myStats->cloak == NULL && myStats->EDITOR_ITEMS[ITEM_SLOT_CLOAK] == 1 )
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
					case 8:
						myStats->cloak = newItem(CLOAK, WORN, -1 + rand() % 3, 1, rand(), false, NULL);
						break;
					case 9:
						myStats->cloak = newItem(CLOAK_MAGICREFLECTION, WORN, 0, 1, rand(), false, NULL);
						break;
				}
			}*/

			// give helmet
			/*if ( myStats->helmet == NULL && myStats->EDITOR_ITEMS[ITEM_SLOT_HELM] == 1 )
			{
				switch ( rand() % 10 )
				{
					case 0:
					case 1:
					case 2:
						break;
					case 3:
					case 4:
						myStats->helmet = newItem(HAT_PHRYGIAN, WORN, -1 + rand() % 3, 1, 0, false, NULL);
						break;
					case 5:
						myStats->helmet = newItem(HAT_WIZARD, WORN, -1 + rand() % 3, 1, 0, false, NULL);
						break;
					case 6:
					case 7:
						myStats->helmet = newItem(LEATHER_HELM, WORN, -1 + rand() % 3, 1, 0, false, NULL);
						break;
					case 8:
					case 9:
						myStats->helmet = newItem(IRON_HELM, WORN, -1 + rand() % 3, 1, 0, false, NULL);
						break;
				}
			}*/

			// give armor
			/*if ( myStats->breastplate == NULL && myStats->EDITOR_ITEMS[ITEM_SLOT_ARMOR] == 1 )
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
					case 6:
					case 7:
						myStats->breastplate = newItem(LEATHER_BREASTPIECE, DECREPIT, -1 + rand() % 3, 1, rand(), false, NULL);
						break;
					case 8:
					case 9:
						myStats->breastplate = newItem(IRON_BREASTPIECE, DECREPIT, -1 + rand() % 3, 1, rand(), false, NULL);
						break;
				}
			}*/
		}
	}

	// torso
	Entity* entity = newEntity(458, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->scalex = 1.01;
	entity->scaley = 1.01;
	entity->scalez = 1.01;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[INSECTOID][1][0]; // 0
	entity->focaly = limbs[INSECTOID][1][1]; // 0
	entity->focalz = limbs[INSECTOID][1][2]; // 0
	entity->behavior = &actInsectoidLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// right leg
	entity = newEntity(457, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[INSECTOID][2][0]; // 0
	entity->focaly = limbs[INSECTOID][2][1]; // 0
	entity->focalz = limbs[INSECTOID][2][2]; // 2
	entity->behavior = &actInsectoidLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// left leg
	entity = newEntity(456, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[INSECTOID][3][0]; // 0
	entity->focaly = limbs[INSECTOID][3][1]; // 0
	entity->focalz = limbs[INSECTOID][3][2]; // 2
	entity->behavior = &actInsectoidLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// right arm
	entity = newEntity(453, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[INSECTOID][4][0]; // 0
	entity->focaly = limbs[INSECTOID][4][1]; // 0
	entity->focalz = limbs[INSECTOID][4][2]; // 1.5
	entity->behavior = &actInsectoidLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// left arm
	entity = newEntity(451, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[INSECTOID][5][0]; // 0
	entity->focaly = limbs[INSECTOID][5][1]; // 0
	entity->focalz = limbs[INSECTOID][5][2]; // 1.5
	entity->behavior = &actInsectoidLimb;
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
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[INSECTOID][6][0]; // 1.5
	entity->focaly = limbs[INSECTOID][6][1]; // 0
	entity->focalz = limbs[INSECTOID][6][2]; // -.5
	entity->behavior = &actInsectoidLimb;
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
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[INSECTOID][7][0]; // 2
	entity->focaly = limbs[INSECTOID][7][1]; // 0
	entity->focalz = limbs[INSECTOID][7][2]; // 0
	entity->behavior = &actInsectoidLimb;
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
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[INSECTOID][8][0]; // 0
	entity->focaly = limbs[INSECTOID][8][1]; // 0
	entity->focalz = limbs[INSECTOID][8][2]; // 4
	entity->behavior = &actInsectoidLimb;
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
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[INSECTOID][9][0]; // 0
	entity->focaly = limbs[INSECTOID][9][1]; // 0
	entity->focalz = limbs[INSECTOID][9][2]; // -2
	entity->behavior = &actInsectoidLimb;
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
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[INSECTOID][10][0]; // 0
	entity->focaly = limbs[INSECTOID][10][1]; // 0
	entity->focalz = limbs[INSECTOID][10][2]; // .25
	entity->behavior = &actInsectoidLimb;
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

void actInsectoidLimb(Entity* my)
{
	my->actMonsterLimb(true);
}

void insectoidDie(Entity* my)
{
	int c;
	for ( c = 0; c < 5; c++ )
	{
		Entity* gib = spawnGib(my);
		serverSpawnGibForClient(gib);
	}

	my->spawnBlood(212);

	playSoundEntity(my, 63 + rand() % 3, 128);

	my->removeMonsterDeathNodes();

	list_RemoveNode(my->mynode);
	return;
}

#define INSECTOIDWALKSPEED .13

void insectoidMoveBodyparts(Entity* my, Stat* myStats, double dist)
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
			my->z = 2.5;
			my->pitch = PI / 4;
		}
		else
		{
			my->z = 0;
			if ( my->monsterAttack == 0 )
			{
				my->pitch = 0;
			}
		}
	}

	//Move bodyparts
	for (bodypart = 0, node = my->children.first; node != NULL; node = node->next, bodypart++)
	{
		if ( bodypart < LIMB_HUMANOID_TORSO )
		{
			// post-swing head animation. client doesn't need to adjust the entity pitch, server will handle.
			if ( my->monsterAttack != MONSTER_POSE_MAGIC_WINDUP3 && bodypart == 1 && multiplayer != CLIENT )
			{
				limbAnimateToLimit(my, ANIMATE_PITCH, 0.1, 0, false, 0.0);
			}
			continue;
		}
		entity = (Entity*)node->element;
		entity->x = my->x;
		entity->y = my->y;
		entity->z = my->z;
		if ( (my->monsterAttack == MONSTER_POSE_MAGIC_WINDUP1 ) && bodypart == LIMB_HUMANOID_RIGHTARM )
		{
			// don't let the creatures's yaw move the casting arm
		}
		else
		{
			entity->yaw = my->yaw;
		}
		if ( bodypart == LIMB_HUMANOID_RIGHTLEG || bodypart == LIMB_HUMANOID_LEFTARM )
		{
			if ( bodypart == LIMB_HUMANOID_LEFTARM && 
				(my->monsterSpecialState == INSECTOID_ACID) )
			{
				Entity* weaponarm = nullptr;
				// leftarm follows the right arm during special acid attack
				node_t* weaponarmNode = list_Node(&my->children, LIMB_HUMANOID_RIGHTARM);
				if ( weaponarmNode )
				{
					weaponarm = (Entity*)weaponarmNode->element;
				}
				else
				{
					return;
				}
				entity->pitch = weaponarm->pitch;
				entity->roll = -weaponarm->roll;
			}
			else
			{
				my->humanoidAnimateWalk(entity, node, bodypart, INSECTOIDWALKSPEED, dist, 0.4);
			}
		}
		else if ( bodypart == LIMB_HUMANOID_LEFTLEG || bodypart == LIMB_HUMANOID_RIGHTARM || bodypart == LIMB_HUMANOID_CLOAK )
		{
			// left leg, right arm, cloak.
			if ( bodypart == LIMB_HUMANOID_RIGHTARM )
			{
				weaponarm = entity;
				if ( my->monsterAttack > 0 )
				{
					Entity* rightbody = nullptr;
					// set rightbody to left leg.
					node_t* rightbodyNode = list_Node(&my->children, LIMB_HUMANOID_LEFTLEG);
					if ( rightbodyNode )
					{
						rightbody = (Entity*)rightbodyNode->element;
					}
					else
					{
						return;
					}

					if ( my->monsterAttack == MONSTER_POSE_RANGED_WINDUP3 )
					{
						if ( my->monsterAttackTime == 0 )
						{
							// init rotations
							weaponarm->pitch = 0;
							my->monsterArmbended = 0;
							my->monsterWeaponYaw = 0;
							weaponarm->roll = 0;
							weaponarm->skill[1] = 0;
							createParticleDot(my);
						}

						limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.25, 5 * PI / 4, false, 0.0);

						if ( my->monsterAttackTime >= ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
						{
							if ( multiplayer != CLIENT )
							{
								my->attack(MONSTER_POSE_INSECTOID_DOUBLETHROW, 0, nullptr);
							}
						}
					}
					// vertical throw
					else if ( my->monsterAttack == MONSTER_POSE_INSECTOID_DOUBLETHROW )
					{
						if ( weaponarm->pitch >= 3 * PI / 2 )
						{
							my->monsterArmbended = 1;
						}

						if ( weaponarm->skill[1] == 0 )
						{
							// chop forwards
							if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, 0.4, PI / 3, false, 0.0) )
							{
								weaponarm->skill[1] = 1;
							}
						}
						else if ( weaponarm->skill[1] == 1 )
						{
							// return to neutral
							if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.25, 7 * PI / 4, false, 0.0) )
							{
								weaponarm->skill[0] = rightbody->skill[0];
								my->monsterWeaponYaw = 0;
								weaponarm->pitch = rightbody->pitch;
								weaponarm->roll = 0;
								my->monsterArmbended = 0;

								if ( multiplayer != CLIENT && my->monsterSpecialState == INSECTOID_DOUBLETHROW_FIRST )
								{
									my->monsterSpecialState = INSECTOID_DOUBLETHROW_SECOND;
									my->attack(MONSTER_POSE_RANGED_WINDUP3, 0, nullptr);
								}
								else
								{
									my->monsterAttack = 0;
								}
							}
						}
						++my->monsterAttackTime;
					}
					else if ( my->monsterAttack == MONSTER_POSE_MAGIC_WINDUP3 )
					{
						if ( my->monsterAttackTime == 0 )
						{
							// init rotations
							weaponarm->pitch = 0;
							my->monsterArmbended = 0;
							my->monsterWeaponYaw = 0;
							weaponarm->roll = 0;
							weaponarm->skill[1] = 0;
							createParticleDot(my);
						}

						limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.25, 5 * PI / 4, false, 0.0);
						if ( multiplayer != CLIENT )
						{
							// move the head and weapon yaw
							limbAnimateToLimit(my, ANIMATE_PITCH, -0.1, 15 * PI / 8, true, 0.1);
							limbAnimateToLimit(my, ANIMATE_WEAPON_YAW, 0.25, 4 * PI / 8, false, 0.0);
						}

						if ( my->monsterAttackTime >= 3 * ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
						{
							if ( multiplayer != CLIENT )
							{
								my->attack(MONSTER_POSE_MELEE_WINDUP1, 0, nullptr);
							}
						}
					}
					else
					{
						my->handleWeaponArmAttack(entity);
					}
				}
			}
			else if ( bodypart == LIMB_HUMANOID_CLOAK )
			{
				entity->pitch = entity->fskill[0];
			}

			my->humanoidAnimateWalk(entity, node, bodypart, INSECTOIDWALKSPEED, dist, 0.4);

			if ( bodypart == LIMB_HUMANOID_CLOAK )
			{
				entity->fskill[0] = entity->pitch;
				entity->roll = my->roll - fabs(entity->pitch) / 2;
				entity->pitch = 0;
			}
		}
		switch ( bodypart )
		{
			// torso
			case LIMB_HUMANOID_TORSO:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->breastplate == NULL )
					{
						entity->sprite = 458;
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
			case LIMB_HUMANOID_RIGHTLEG:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->shoes == nullptr )
					{
						entity->sprite = 457;
					}
					else
					{
						my->setBootSprite(entity, SPRITE_BOOT_RIGHT_OFFSET);
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
				entity->x += 1 * cos(my->yaw + PI / 2) + .25 * cos(my->yaw);
				entity->y += 1 * sin(my->yaw + PI / 2) + .25 * sin(my->yaw);
				entity->z += 4;
				if ( my->z >= 2.4 && my->z <= 2.6 )
				{
					entity->yaw += PI / 8;
					entity->pitch = -PI / 2;
				}
				break;
			// left leg
			case LIMB_HUMANOID_LEFTLEG:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->shoes == nullptr )
					{
						entity->sprite = 456;
					}
					else
					{
						my->setBootSprite(entity, SPRITE_BOOT_LEFT_OFFSET);
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
				entity->x -= 1 * cos(my->yaw + PI / 2) - .25 * cos(my->yaw);
				entity->y -= 1 * sin(my->yaw + PI / 2) - .25 * sin(my->yaw);
				entity->z += 4;
				if ( my->z >= 2.4 && my->z <= 2.6 )
				{
					entity->yaw -= PI / 8;
					entity->pitch = -PI / 2;
				}
				break;
			// right arm
			case LIMB_HUMANOID_RIGHTARM:
			{
				node_t* weaponNode = list_Node(&my->children, LIMB_HUMANOID_WEAPON);
				if ( weaponNode )
				{
					Entity* weapon = (Entity*)weaponNode->element;
					if ( MONSTER_ARMBENDED || (weapon->flags[INVISIBLE] && my->monsterState == MONSTER_STATE_WAIT) )
					{
						// if weapon invisible and I'm not attacking, relax arm.
						entity->focalx = limbs[INSECTOID][4][0]; // 0
						entity->focaly = limbs[INSECTOID][4][1]; // 0
						entity->focalz = limbs[INSECTOID][4][2]; // 2
						entity->sprite = 453;
					}
					else
					{
						// else flex arm.
						entity->focalx = limbs[INSECTOID][4][0] + 0.75;
						entity->focaly = limbs[INSECTOID][4][1];
						entity->focalz = limbs[INSECTOID][4][2] - 0.75;
						entity->sprite = 454;
					}
				}
				entity->x += 2.5 * cos(my->yaw + PI / 2) - .20 * cos(my->yaw);
				entity->y += 2.5 * sin(my->yaw + PI / 2) - .20 * sin(my->yaw);
				entity->z += .5;
				entity->yaw += MONSTER_WEAPONYAW;
				if ( my->z >= 2.4 && my->z <= 2.6 )
				{
					entity->pitch = 0;
				}
				break;
			}
			// left arm
			case LIMB_HUMANOID_LEFTARM:
			{
				node_t* shieldNode = list_Node(&my->children, LIMB_HUMANOID_SHIELD);
				if ( shieldNode )
				{
					Entity* shield = (Entity*)shieldNode->element;
					if ( shield->flags[INVISIBLE] && my->monsterState == MONSTER_STATE_WAIT )
					{
						entity->focalx = limbs[INSECTOID][5][0]; // 0
						entity->focaly = limbs[INSECTOID][5][1]; // 0
						entity->focalz = limbs[INSECTOID][5][2]; // 2
						entity->sprite = 451;
					}
					else
					{
						entity->focalx = limbs[INSECTOID][5][0] + 0.75;
						entity->focaly = limbs[INSECTOID][5][1];
						entity->focalz = limbs[INSECTOID][5][2] - 0.75;
						entity->sprite = 452;
					}
				}
				entity->x -= 2.5 * cos(my->yaw + PI / 2) + .20 * cos(my->yaw);
				entity->y -= 2.5 * sin(my->yaw + PI / 2) + .20 * sin(my->yaw);
				entity->z += .5;
				if ( my->z >= 2.4 && my->z <= 2.6 )
				{
					entity->pitch = 0;
				}
				break;
			}
			// weapon
			case LIMB_HUMANOID_WEAPON:
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
				if ( weaponarm != nullptr )
				{
					my->handleHumanoidWeaponLimb(entity, weaponarm);
				}
				break;
			// shield
			case LIMB_HUMANOID_SHIELD:
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
			case LIMB_HUMANOID_CLOAK:
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
			case LIMB_HUMANOID_HELMET:
				entity->focalx = limbs[INSECTOID][9][0]; // 0
				entity->focaly = limbs[INSECTOID][9][1]; // 0
				entity->focalz = limbs[INSECTOID][9][2]; // -2
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
						entity->focalx = limbs[INSECTOID][9][0] - .5;
						entity->focaly = limbs[INSECTOID][9][1] - 3.55;
						entity->focalz = limbs[INSECTOID][9][2] + 2.5;
						entity->roll = PI / 2;
					}
					else if ( entity->sprite >= items[HAT_HOOD].index && entity->sprite < items[HAT_HOOD].index + items[HAT_HOOD].variations )
					{
						entity->focalx = limbs[INSECTOID][9][0] - .5;
						entity->focaly = limbs[INSECTOID][9][1] - 2.75;
						entity->focalz = limbs[INSECTOID][9][2] + 2.5;
						entity->roll = PI / 2;
					}
					else if ( entity->sprite == items[HAT_WIZARD].index )
					{
						entity->focalx = limbs[INSECTOID][9][0];
						entity->focaly = limbs[INSECTOID][9][1] - 5;
						entity->focalz = limbs[INSECTOID][9][2] + 2.5;
						entity->roll = PI / 2;
					}
					else if ( entity->sprite == items[HAT_JESTER].index )
					{
						entity->focalx = limbs[INSECTOID][9][0];
						entity->focaly = limbs[INSECTOID][9][1] - 5;
						entity->focalz = limbs[INSECTOID][9][2] + 2.5;
						entity->roll = PI / 2;
					}
				}
				else
				{
					my->flags[INVISIBLE] = true;
				}
				break;
			// mask
			case LIMB_HUMANOID_MASK:
				entity->focalx = limbs[INSECTOID][10][0]; // 0
				entity->focaly = limbs[INSECTOID][10][1]; // 0
				entity->focalz = limbs[INSECTOID][10][2]; // .25
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
					entity->focalx = limbs[INSECTOID][10][0] + .35; // .35
					entity->focaly = limbs[INSECTOID][10][1] - 2; // -2
					entity->focalz = limbs[INSECTOID][10][2]; // .25
				}
				else
				{
					entity->focalx = limbs[INSECTOID][10][0] + .25; // .25
					entity->focaly = limbs[INSECTOID][10][1] - 2.25; // -2.25
					entity->focalz = limbs[INSECTOID][10][2]; // .25
				}
				break;
		}
	}
	// rotate shield a bit
	node_t* shieldNode = list_Node(&my->children, 8);
	if ( shieldNode )
	{
		Entity* shieldEntity = (Entity*)shieldNode->element;
		if ( shieldEntity->sprite != items[TOOL_TORCH].index && shieldEntity->sprite != items[TOOL_LANTERN].index && shieldEntity->sprite != items[TOOL_CRYSTALSHARD].index )
		{
			shieldEntity->yaw -= PI / 6;
		}
	}
	if ( MONSTER_ATTACK > 0 && MONSTER_ATTACK <= MONSTER_POSE_MAGIC_CAST3 )
	{
		MONSTER_ATTACKTIME++;
	}
	else if ( MONSTER_ATTACK == 0 )
	{
		MONSTER_ATTACKTIME = 0;
	}
	else
	{
		// do nothing, don't reset attacktime or increment it.
	}
}

bool Entity::insectoidCanWieldItem(const Item& item) const
{
	Stat* myStats = getStats();
	if ( !myStats )
	{
		return false;
	}

	switch ( itemCategory(&item) )
	{
		case WEAPON:
			return true;
		case POTION:
			return false;
		case THROWN:
			return true;
		case ARMOR:
		{ //Little baby compiler stop whining, wah wah.
			int equipType = checkEquipType(&item);
			if ( equipType == TYPE_HAT || equipType == TYPE_HELM )
			{
				return false; //No can wear hats, because antennae.
			}
			return true; //Can wear all other armor.
		}
		default:
			return false;
	}

	return false;
}

void Entity::insectoidChooseWeapon(const Entity* target, double dist)
{
	if ( monsterSpecialState == 1 )
	{
		//Holding a weapon assigned from the special attack. Don't switch weapons.
		//messagePlayer()
		return;
	}

	Stat *myStats = getStats();
	if ( !myStats )
	{
		return;
	}

	/*if ( myStats->weapon && (itemCategory(myStats->weapon) == MAGICSTAFF || itemCategory(myStats->weapon) == SPELLBOOK) )
	{
		return;
	}*/

	int specialRoll = -1;
	bool usePotionSpecial = false;

	node_t* hasPotion = nullptr;
	bool isHealingPotion = false;

	bool inMeleeRange = monsterInMeleeRange(target, dist);

	if ( inMeleeRange )
	{
		//Switch to a melee weapon if not already wielding one. Unless monster special state is overriding the AI.
		if ( !myStats->weapon || !isMeleeWeapon(*myStats->weapon) )
		{
			node_t* weaponNode = getMeleeWeaponItemNodeInInventory(myStats);
			if ( !weaponNode )
			{
				return; //Resort to fists.
			}

			bool swapped = swapMonsterWeaponWithInventoryItem(this, myStats, weaponNode, false);
			if ( !swapped )
			{
				//Don't return so that monsters will at least equip ranged weapons in melee range if they don't have anything else.
			}
			else
			{
				return;
			}
		}
		else
		{
			return;
		}
	}

	//Switch to a thrown weapon or a ranged weapon.
	if ( !myStats->weapon || isMeleeWeapon(*myStats->weapon) )
	{
		//First search the inventory for a THROWN weapon.
		node_t *weaponNode = getRangedWeaponItemNodeInInventory(myStats);
		if ( !weaponNode )
		{
			return; //Nothing available
		}
		bool swapped = swapMonsterWeaponWithInventoryItem(this, myStats, weaponNode, false);
		return;
	}
	return;
}
