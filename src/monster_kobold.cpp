/*-------------------------------------------------------------------------------

	BARONY
	File: monster_kobold.cpp
	Desc: implements all of the kobold monster's code

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
#include "book.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "magic/magic.hpp"

void initKobold(Entity* my, Stat* myStats)
{
	int c;
	node_t* node;

	//Sprite 421 = Kobold head model
	my->initMonster(421);

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 220;
		MONSTER_SPOTVAR = 5;
		MONSTER_IDLESND = 217;
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

			//give weapon
			if ( myStats->weapon == NULL && myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1 )
			{
				switch ( rand() % 10 )
				{
					case 0:
					case 1:
					case 2:
						myStats->weapon = newItem(STEEL_SWORD, static_cast<Status>(WORN + rand() % 3), -1 + rand() % 3, 1, rand(), false, nullptr);
						break;
					case 3:
					case 4:
						myStats->weapon = newItem(STEEL_HALBERD, static_cast<Status>(WORN + rand() % 3), -1 + rand() % 3, 1, rand(), false, nullptr);
						break;
					case 5:
					case 6:
					case 7:
					case 8:
						myStats->weapon = newItem(CROSSBOW, static_cast<Status>(WORN + rand() % 3), -1 + rand() % 3, 1, rand(), false, nullptr);
						break;
					case 9:
						myStats->weapon = newItem(IRON_AXE, static_cast<Status>(DECREPIT + rand() % 4), -2 + rand() % 5, 1, rand(), false, nullptr);
						break;
				}
			}

			// generate the default inventory items for the monster, provided the editor sprite allowed enough default slots
			switch ( defaultItems )
			{
				case 6:
				case 5:
				case 4:
				case 3:
				case 2:
				case 1:
					if ( myStats->weapon && myStats->weapon->type == CROSSBOW )
					{
						if ( rand() % 2 == 0 ) // 50% chance
						{
							newItem(SPELLBOOK_SLOW, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, &myStats->inventory);
						}
					}
					break;
				default:
					break;
			}

			//give shield
			if ( myStats->shield == NULL && myStats->EDITOR_ITEMS[ITEM_SLOT_SHIELD] == 1 )
			{
				switch ( rand() % 10 )
				{
					case 0:
					case 1:
						myStats->shield = newItem(IRON_SHIELD, static_cast<Status>(WORN + rand() % 2), -2 + rand() % 5, 1, rand(), false, nullptr);
						break;
					case 2:
					case 3:
					case 4:
					case 5:
						myStats->shield = newItem(STEEL_SHIELD, static_cast<Status>(DECREPIT + rand() % 4), -1 + rand() % 3, 1, rand(), false, nullptr);
						break;
					case 6:
					case 7:
						myStats->shield = newItem(TOOL_LANTERN, EXCELLENT, -1 + rand() % 3, 1, rand(), false, nullptr);
						break;
					case 8:
						myStats->shield = newItem(TOOL_CRYSTALSHARD, SERVICABLE, -1 + rand() % 3, 1, rand(), false, nullptr);
						break;
					case 9:
						// nothing
						break;
				}
			}

			// give cloak
			if ( myStats->cloak == NULL && myStats->EDITOR_ITEMS[ITEM_SLOT_CLOAK] == 1 )
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
					case 8:
					case 9:
						myStats->cloak = newItem(CLOAK, static_cast<Status>(WORN + rand() % 3), -1 + rand() % 3, 1, rand(), false, nullptr);
						break;
				}
			}

			// give helm
			/*if ( myStats->helmet == NULL && myStats->EDITOR_ITEMS[ITEM_SLOT_HELM] == 1 )
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
						myStats->helmet = newItem(HAT_HOOD, static_cast<Status>(WORN + rand() % 3), -1 + rand() % 3, 1, rand(), false, nullptr);
						break;
					case 7:
					case 8:
					case 9:
						myStats->helmet = newItem(STEEL_HELM, static_cast<Status>(WORN + rand() % 2), -1 + rand() % 3, 1, rand(), false, nullptr);
						break;
				}
			}*/
		}
	}

	// torso
	Entity* entity = newEntity(422, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[KOBOLD][1][0]; // 0
	entity->focaly = limbs[KOBOLD][1][1]; // 0
	entity->focalz = limbs[KOBOLD][1][2]; // 0
	entity->behavior = &actKoboldLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// right leg
	entity = newEntity(423, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[KOBOLD][2][0]; // .25
	entity->focaly = limbs[KOBOLD][2][1]; // 0
	entity->focalz = limbs[KOBOLD][2][2]; // 1.5
	entity->behavior = &actKoboldLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// left leg
	entity = newEntity(424, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[KOBOLD][3][0]; // .25
	entity->focaly = limbs[KOBOLD][3][1]; // 0
	entity->focalz = limbs[KOBOLD][3][2]; // 1.5
	entity->behavior = &actKoboldLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// right arm
	entity = newEntity(425, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[KOBOLD][4][0]; // 0
	entity->focaly = limbs[KOBOLD][4][1]; // 0
	entity->focalz = limbs[KOBOLD][4][2]; // 2
	entity->behavior = &actKoboldLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// left arm
	entity = newEntity(427, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[KOBOLD][5][0]; // 0
	entity->focaly = limbs[KOBOLD][5][1]; // 0
	entity->focalz = limbs[KOBOLD][5][2]; // 2
	entity->behavior = &actKoboldLimb;
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
	entity->focalx = limbs[KOBOLD][6][0]; // 2
	entity->focaly = limbs[KOBOLD][6][1]; // 0
	entity->focalz = limbs[KOBOLD][6][2]; // -.5
	entity->behavior = &actKoboldLimb;
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
	entity->focalx = limbs[KOBOLD][7][0]; // 0
	entity->focaly = limbs[KOBOLD][7][1]; // 0
	entity->focalz = limbs[KOBOLD][7][2]; // 1.5
	entity->behavior = &actKoboldLimb;
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
	entity->focalx = limbs[KOBOLD][8][0]; // 0
	entity->focaly = limbs[KOBOLD][8][1]; // 0
	entity->focalz = limbs[KOBOLD][8][2]; // 4
	entity->behavior = &actKoboldLimb;
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

void actKoboldLimb(Entity* my)
{
	my->actMonsterLimb(true);
}

void koboldDie(Entity* my)
{
	int c;
	for ( c = 0; c < 6; ++c )
	{
		Entity* entity = spawnGib(my);
		if ( entity )
		{
			serverSpawnGibForClient(entity);
		}
	}

	my->spawnBlood();

	my->removeMonsterDeathNodes();

	playSoundEntity(my, 225 + rand() % 4, 128);
	list_RemoveNode(my->mynode);
	return;
}

#define KOBOLDWALKSPEED .13

void koboldMoveBodyparts(Entity* my, Stat* myStats, double dist)
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
					++bodypart;
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
				++bodypart;
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
					++bodypart;
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
				++bodypart;
			}
		}

		// sleeping
		if ( myStats->EFFECTS[EFF_ASLEEP] )
		{
			my->z = 4;
			my->pitch = PI / 4;
		}
		else
		{
			my->z = 2.25;
			my->pitch = 0;
		}
	}

	//Move bodyparts
	for (bodypart = 0, node = my->children.first; node != NULL; node = node->next, ++bodypart)
	{
		if ( bodypart < 2 )
		{
			continue;
		}
		entity = (Entity*)node->element;
		entity->x = my->x;
		entity->y = my->y;
		entity->z = my->z;
		if ( MONSTER_ATTACK == MONSTER_POSE_MAGIC_WINDUP1 && bodypart == 5 )
		{
			//entity->yaw = my->yaw;
		}
		else
		{
			entity->yaw = my->yaw;
		}
		if ( bodypart == 3 || bodypart == 6 )
		{
			// right leg, left arm.
			if ( bodypart == 3 )
			{
				// set rightbody to left leg.
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
						entity->pitch -= dist * KOBOLDWALKSPEED;
						if ( entity->pitch < -PI / 4.0 )
						{
							entity->pitch = -PI / 4.0;
							if (bodypart == 3)
							{
								entity->skill[0] = 1;
								if ( dist > .4 )
								{
									playSoundEntityLocal(my, rand() % 7, 32);
								}
							}
						}
					}
					else
					{
						entity->pitch += dist * KOBOLDWALKSPEED;
						if ( entity->pitch > PI / 4.0 )
						{
							entity->pitch = PI / 4.0;
							if (bodypart == 3)
							{
								entity->skill[0] = 0;
								if ( dist > .4 )
								{
									playSoundEntityLocal(my, rand() % 7, 32);
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
			// left leg, right arm, cloak.
			if ( bodypart == 5 )
			{
				weaponarm = entity;

				// vertical chop windup
				if ( MONSTER_ATTACK == MONSTER_POSE_MELEE_WINDUP1 )
				{
					if ( MONSTER_ATTACKTIME == 0 )
					{
						// init rotations
						entity->pitch = 0;
						MONSTER_ARMBENDED = 0;
						MONSTER_WEAPONYAW = 0;
						entity->roll = 0;
					}
					if ( limbAnimateToLimit(entity, ANIMATE_PITCH, -0.25, 5 * PI / 4, false, 0.0) )
					{
						if ( multiplayer != CLIENT )
						{
							my->attack(1, 0, nullptr);
						}
					}
				}
				// vertical chop attack
				else if ( MONSTER_ATTACK == 1 )
				{
					if ( entity->pitch >= 3 * PI / 2 )
					{
						MONSTER_ARMBENDED = 1;
					}
					if ( limbAnimateToLimit(entity, ANIMATE_PITCH, 0.5, PI / 4, false, 0.0) )
					{
						entity->skill[0] = rightbody->skill[0];
						MONSTER_WEAPONYAW = 0;
						entity->pitch = rightbody->pitch;
						entity->roll = 0;
						MONSTER_ARMBENDED = 0;
						MONSTER_ATTACK = 0;
					}
				}

				// horizontal chop windup
				else if ( MONSTER_ATTACK == MONSTER_POSE_MELEE_WINDUP2 )
				{
					if ( MONSTER_ATTACKTIME == 0 )
					{
						// init rotations
						entity->pitch = PI / 4;
						entity->roll = 0;
					}

					MONSTER_ARMBENDED = 1;
					limbAnimateToLimit(entity, ANIMATE_ROLL, -0.2, 3 * PI / 2, false, 0.0);
					limbAnimateToLimit(entity, ANIMATE_PITCH, -0.2, 0, false, 0.0);
					
					MONSTER_WEAPONYAW = 5 * PI / 4;
					//entity->roll = PI / 2;

					if ( MONSTER_ATTACKTIME >= ANIMATE_DURATION_WINDUP )
					{
						if ( multiplayer != CLIENT )
						{
							my->attack(2, 0, nullptr);
						}
					}
				}
				// horizontal chop attack
				else if ( MONSTER_ATTACK == 2 )
				{
					if ( limbAnimateToLimit(my, ANIMATE_WEAPON_YAW, 0.25, PI / 8, false, 0.0) )
					{
						entity->skill[0] = rightbody->skill[0];
						MONSTER_WEAPONYAW = 0;
						entity->pitch = rightbody->pitch;
						entity->roll = 0;
						MONSTER_ARMBENDED = 0;
						MONSTER_ATTACK = 0;
					}
				}
				// stab windup
				else if ( MONSTER_ATTACK == MONSTER_POSE_MELEE_WINDUP3 )
				{
					if ( MONSTER_ATTACKTIME == 0 )
					{
						// init rotations
						MONSTER_ARMBENDED = 0;
						MONSTER_WEAPONYAW = 0;
						entity->roll = 0;
						entity->pitch = 0;
					}

					limbAnimateToLimit(entity, ANIMATE_PITCH, 0.5, 2 * PI / 3, true, 0.05);

					if ( MONSTER_ATTACKTIME >= ANIMATE_DURATION_WINDUP )
					{
						if ( multiplayer != CLIENT )
						{
							my->attack(3, 0, nullptr);
						}
					}

				}
				// stab attack - refer to weapon limb code for additional animation
				else if ( MONSTER_ATTACK == 3 )
				{
					if ( limbAnimateToLimit(entity, ANIMATE_PITCH, -0.3, 11 * PI / 6, false, 0.0) )
					{
						entity->skill[0] = rightbody->skill[0];
						MONSTER_WEAPONYAW = 0;
						entity->pitch = rightbody->pitch;
						entity->roll = 0;
						MONSTER_ARMBENDED = 0;
						MONSTER_ATTACK = 0;
					}
				}

				else if ( MONSTER_ATTACK == MONSTER_POSE_RANGED_WINDUP1 )
				{
					// crossbow
					if ( MONSTER_ATTACKTIME == 0 )
					{
						// init rotations
						MONSTER_ARMBENDED = 0;
						MONSTER_WEAPONYAW = 0;
						entity->roll = 0;
						entity->pitch = 9 * PI / 5;
					}

					limbAnimateToLimit(entity, ANIMATE_PITCH, 0.15, 0, false, 0.0);

					if ( MONSTER_ATTACKTIME >= ANIMATE_DURATION_WINDUP )
					{
						if ( multiplayer != CLIENT )
						{
							entity->skill[0] = rightbody->skill[0];
							MONSTER_WEAPONYAW = 0;
							entity->pitch = rightbody->pitch;
							entity->roll = 0;
							MONSTER_ARMBENDED = 0;
							MONSTER_ATTACK = 0;
							my->attack(0, 0, nullptr);
						}
					}

					MONSTER_ATTACKTIME++; // manually increment counter
				}
				else if ( MONSTER_ATTACK == MONSTER_POSE_MAGIC_WINDUP1 )
				{
					// magic
					if ( MONSTER_ATTACKTIME == 0 )
					{
						// init rotations
						MONSTER_ARMBENDED = 0;
						MONSTER_WEAPONYAW = 0;
						entity->roll = 0;
						entity->pitch = PI / 8;
						entity->yaw = my->yaw - PI / 8;
						entity->skill[0] = 0;
						createParticleDot(my);
					}

					double animationSetpoint = normaliseAngle2PI(my->yaw + PI / 8);
					double animationEndpoint = normaliseAngle2PI(my->yaw - PI / 8);

					if ( entity->skill[0] == 0 )
					{
						if ( limbAnimateToLimit(entity, ANIMATE_PITCH, 0.1, PI / 8, false, 0.0) && limbAnimateToLimit(entity, ANIMATE_YAW, 0.1, animationSetpoint, false, 0.0) )
						{
							entity->skill[0] = 1;
						}
					}
					else
					{
						if ( limbAnimateToLimit(entity, ANIMATE_PITCH, -0.1, 15 * PI / 8, false, 0.0) && limbAnimateToLimit(entity, ANIMATE_YAW, -0.1, animationEndpoint, false, 0.0) )
						{
							entity->skill[0] = 0;
						}
					}

					//limbAnimateWithOvershoot(entity, ANIMATE_YAW, 0.2, animationSetpoint, 0.2, animationEndpoint, ANIMATE_DIR_POSITIVE);
					//limbAnimateToLimit(entity, ANIMATE_PITCH, 0.2, 0, true, 0.001);

					if ( MONSTER_ATTACKTIME >= 30 )
					{
						if ( multiplayer != CLIENT )
						{
							entity->skill[0] = rightbody->skill[0];
							MONSTER_WEAPONYAW = 0;
							entity->pitch = rightbody->pitch;
							entity->roll = 0;
							MONSTER_ARMBENDED = 0;
							MONSTER_ATTACK = 0;
							my->attack(0, 0, nullptr);
						}
					}

					MONSTER_ATTACKTIME++; // manually increment counter
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
						entity->pitch -= dist * KOBOLDWALKSPEED;
						if ( entity->pitch < -PI / 4.0 )
						{
							entity->skill[0] = 0;
							entity->pitch = -PI / 4.0;
						}
					}
					else
					{
						entity->pitch += dist * KOBOLDWALKSPEED;
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
				entity->x -= .25 * cos(my->yaw);
				entity->y -= .25 * sin(my->yaw);
				entity->z += 1.25;
				break;
			// right leg
			case 3:
				entity->x += 1.25 * cos(my->yaw + PI / 2);
				entity->y += 1.25 * sin(my->yaw + PI / 2);
				entity->z += 2.75;
				if ( my->z >= 3.9 && my->z <= 4.1 )
				{
					entity->yaw += PI / 8;
					entity->pitch = -PI / 2;
				}
				break;
			// left leg
			case 4:
				entity->x -= 1.25 * cos(my->yaw + PI / 2);
				entity->y -= 1.25 * sin(my->yaw + PI / 2);
				entity->z += 2.75;
				if ( my->z >= 3.9 && my->z <= 4.1 )
				{
					entity->yaw -= PI / 8;
					entity->pitch = -PI / 2;
				}
				break;
			// right arm
			case 5:
			{
				node_t* weaponNode = list_Node(&my->children, 7);
				if ( weaponNode )
				{
					Entity* weapon = (Entity*)weaponNode->element;
					if ( weapon->flags[INVISIBLE] || MONSTER_ARMBENDED )
					{
						entity->focalx = limbs[KOBOLD][4][0]; // 0
						entity->focaly = limbs[KOBOLD][4][1]; // 0
						entity->focalz = limbs[KOBOLD][4][2]; // 2
						entity->sprite = 425;
					}
					else
					{
						entity->focalx = limbs[KOBOLD][4][0] + 1; // 1
						entity->focaly = limbs[KOBOLD][4][1]; // 0
						entity->focalz = limbs[KOBOLD][4][2] - 1; // 1
						entity->sprite = 426;
					}
				}
				entity->x += 2.5 * cos(my->yaw + PI / 2) - .75 * cos(my->yaw);
				entity->y += 2.5 * sin(my->yaw + PI / 2) - .75 * sin(my->yaw);
				entity->z -= .25;
				entity->yaw += MONSTER_WEAPONYAW;
				if ( my->z >= 3.9 && my->z <= 4.1 )
				{
					entity->pitch = 0;
				}
				break;
			}
			// left arm
			case 6:
			{
				node_t* shieldNode = list_Node(&my->children, 8);
				if ( shieldNode )
				{
					Entity* shield = (Entity*)shieldNode->element;
					if ( shield->flags[INVISIBLE] )
					{
						entity->focalx = limbs[KOBOLD][5][0]; // 0
						entity->focaly = limbs[KOBOLD][5][1]; // 0
						entity->focalz = limbs[KOBOLD][5][2]; // 2
						entity->sprite = 427;
					}
					else
					{
						entity->focalx = limbs[KOBOLD][5][0] + 1; // 1
						entity->focaly = limbs[KOBOLD][5][1]; // 0
						entity->focalz = limbs[KOBOLD][5][2] - 1; // 1
						entity->sprite = 428;
					}
				}
				entity->x -= 2.5 * cos(my->yaw + PI / 2) + .75 * cos(my->yaw);
				entity->y -= 2.5 * sin(my->yaw + PI / 2) + .75 * sin(my->yaw);
				entity->z -= .25;
				if ( my->z >= 3.9 && my->z <= 4.1 )
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
							if ( MONSTER_ATTACK == 3 )
							{
								if ( weaponarm->pitch < PI / 4 || weaponarm->pitch > 11 * PI / 6 )
								{
									entity->focalx = limbs[KOBOLD][6][0];
									entity->focalz = limbs[KOBOLD][6][2] - 4.5;
									entity->x = weaponarm->x + 0 * cos(weaponarm->yaw + PI / 2) - ((0 - 1.5 * cos(weaponarm->pitch)) * cos(weaponarm->yaw));
									entity->y = weaponarm->y + 0 * sin(weaponarm->yaw + PI / 2) - ((0 - 1.5 * cos(weaponarm->pitch)) * sin(weaponarm->yaw));
									entity->z = weaponarm->z + 1 + 2 * sin(weaponarm->pitch);
									limbAnimateToLimit(entity, ANIMATE_PITCH, 0.2, PI / 2, false, 0);
								}
								else
								{
									entity->focalx = limbs[KOBOLD][6][0];
									entity->focalz = limbs[KOBOLD][6][2];
									entity->x = weaponarm->x + .5 * cos(weaponarm->yaw) * (MONSTER_ATTACK == 0);
									entity->y = weaponarm->y + .5 * sin(weaponarm->yaw) * (MONSTER_ATTACK == 0);
									entity->z = weaponarm->z - .5 * (MONSTER_ATTACK == 0);
									entity->pitch = weaponarm->pitch + .25 * (MONSTER_ATTACK == 0);
								}
							}
							else
							{
								entity->focalx = limbs[KOBOLD][6][0];
								entity->focalz = limbs[KOBOLD][6][2];
								entity->x = weaponarm->x + .5 * cos(weaponarm->yaw) * (MONSTER_ATTACK == 0);
								entity->y = weaponarm->y + .5 * sin(weaponarm->yaw) * (MONSTER_ATTACK == 0);
								entity->z = weaponarm->z - .5 * (MONSTER_ATTACK == 0);
								entity->pitch = weaponarm->pitch + .25 * (MONSTER_ATTACK == 0);
							}
							//entity->pitch += 0.1;
						}
					}

					entity->yaw = weaponarm->yaw;
					entity->roll = weaponarm->roll;
					if ( !MONSTER_ARMBENDED )
					{
						if ( MONSTER_ATTACK != 3 )
						{
							entity->focalx = limbs[KOBOLD][6][0];
							if ( entity->sprite == items[CROSSBOW].index )
							{
								entity->focalx += 2;
							}
							entity->focaly = limbs[KOBOLD][6][1];
							entity->focalz = limbs[KOBOLD][6][2];
						}
					}
					else
					{
						entity->focalx = limbs[KOBOLD][6][0] + 1;
						entity->focaly = limbs[KOBOLD][6][1];
						entity->focalz = limbs[KOBOLD][6][2] - 2;
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
				entity->z += 1;
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
				entity->x -= cos(my->yaw) * 1.5;
				entity->y -= sin(my->yaw) * 1.5;
				entity->yaw += PI / 2;
				break;
		}
	}
	if ( MONSTER_ATTACK > 0 && MONSTER_ATTACK <= MONSTER_POSE_MELEE_WINDUP3 )
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
