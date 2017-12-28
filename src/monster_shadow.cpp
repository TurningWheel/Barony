/*-------------------------------------------------------------------------------

BARONY
File: monster_shadow.cpp
Desc: implements all of the shadow monster's code

Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/

#include <string>
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

void initShadow(Entity* my, Stat* myStats)
{
	int c;
	node_t* node;
	my->monsterShadowDontChangeName = 0; //By default, it does.
	if ( myStats && strcmp(myStats->name, "") != 0 )
	{
		my->monsterShadowDontChangeName = 1; //User set a name.
	}

	my->initMonster(481);

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 318;
		MONSTER_SPOTVAR = 3;
		MONSTER_IDLESND = 313;
		MONSTER_IDLEVAR = 3;
	}
	if ( multiplayer != CLIENT && !MONSTER_INIT )
	{
		if ( myStats != nullptr )
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
			if ( rand() % 50 == 0 && !my->flags[USERFLAG2] )
			{
				strcpy(myStats->name, "Baratheon"); //Long live the king, who commands his grue army.
				my->monsterShadowDontChangeName = 1; //Special monsters don't change their name either.
				myStats->GOLD = 1000;
				myStats->RANDOM_GOLD = 500;
				myStats->LVL = 50; // >:U
			}

			// random effects
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
		}
	}

	// torso
	Entity* entity = newEntity(482, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->scalex = 1.01;
	entity->scaley = 1.01;
	entity->scalez = 1.01;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SHADOW][1][0]; // 0
	entity->focaly = limbs[SHADOW][1][1]; // 0
	entity->focalz = limbs[SHADOW][1][2]; // 0
	entity->behavior = &actShadowLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// right leg
	entity = newEntity(436, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SHADOW][2][0]; // 0
	entity->focaly = limbs[SHADOW][2][1]; // 0
	entity->focalz = limbs[SHADOW][2][2]; // 2
	entity->behavior = &actShadowLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// left leg
	entity = newEntity(435, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SHADOW][3][0]; // 0
	entity->focaly = limbs[SHADOW][3][1]; // 0
	entity->focalz = limbs[SHADOW][3][2]; // 2
	entity->behavior = &actShadowLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// right arm
	entity = newEntity(433, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SHADOW][4][0]; // 0
	entity->focaly = limbs[SHADOW][4][1]; // 0
	entity->focalz = limbs[SHADOW][4][2]; // 1.5
	entity->behavior = &actShadowLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// left arm
	entity = newEntity(431, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SHADOW][5][0]; // 0
	entity->focaly = limbs[SHADOW][5][1]; // 0
	entity->focalz = limbs[SHADOW][5][2]; // 1.5
	entity->behavior = &actShadowLimb;
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
	entity->focalx = limbs[SHADOW][6][0]; // 1.5
	entity->focaly = limbs[SHADOW][6][1]; // 0
	entity->focalz = limbs[SHADOW][6][2]; // -.5
	entity->behavior = &actShadowLimb;
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
	entity->focalx = limbs[SHADOW][7][0]; // 2
	entity->focaly = limbs[SHADOW][7][1]; // 0
	entity->focalz = limbs[SHADOW][7][2]; // 0
	entity->behavior = &actShadowLimb;
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
	entity->focalx = limbs[SHADOW][8][0]; // 0
	entity->focaly = limbs[SHADOW][8][1]; // 0
	entity->focalz = limbs[SHADOW][8][2]; // 4
	entity->behavior = &actShadowLimb;
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
	entity->focalx = limbs[SHADOW][9][0]; // 0
	entity->focaly = limbs[SHADOW][9][1]; // 0
	entity->focalz = limbs[SHADOW][9][2]; // -2
	entity->behavior = &actShadowLimb;
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
	entity->focalx = limbs[SHADOW][10][0]; // 0
	entity->focaly = limbs[SHADOW][10][1]; // 0
	entity->focalz = limbs[SHADOW][10][2]; // .25
	entity->behavior = &actShadowLimb;
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

void actShadowLimb(Entity* my)
{
	my->actMonsterLimb(true);
}

void shadowDie(Entity* my)
{
	int c;
	for ( c = 0; c < 5; c++ )
	{
		Entity* gib = spawnGib(my);
		serverSpawnGibForClient(gib);
	}

	playSoundEntity(my, 316 + rand() % 2, 128);

	my->removeMonsterDeathNodes();

	list_RemoveNode(my->mynode);
	return;
}

#define SHADOWWALKSPEED .05

void shadowMoveBodyparts(Entity* my, Stat* myStats, double dist)
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
			for ( node = my->children.first; node != NULL; node = node->next )
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
			for ( node = my->children.first; node != NULL; node = node->next )
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

		// sleeping
		if ( myStats->EFFECTS[EFF_ASLEEP] )
		{
			my->z = 2.5;
			my->pitch = PI / 4;
		}
		else
		{
			if ( multiplayer != CLIENT )
			{
				if ( my->monsterAnimationLimbOvershoot == ANIMATE_OVERSHOOT_NONE )
				{
					my->z = -1.2;
					my->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_TO_SETPOINT;
				}
				if ( dist < 0.1 )
				{
					// not moving, float.
					limbAnimateWithOvershoot(my, ANIMATE_Z, 0.005, -2, 0.005, -1.2, ANIMATE_DIR_NEGATIVE);
				}
			}
			//my->z = -2;
		}
	}

	//Shadow stares you down while he does his special ability windup, and any of his spellcasting animations.
	if ( my->monsterAttack == MONSTER_POSE_MAGIC_WINDUP3 )
	{
		//Always turn to face the target.
		Entity* target = uidToEntity(my->monsterTarget);
		if ( target )
		{
			my->lookAtEntity(*target);
			my->monsterRotate();
		}
	}

	//Move bodyparts
	for ( bodypart = 0, node = my->children.first; node != nullptr; node = node->next, bodypart++ )
	{
		if ( bodypart < 2 )
		{
			// post-swing head animation. client doesn't need to adjust the entity pitch, server will handle.
			if ( multiplayer != CLIENT && bodypart == 1 )
			{
				if ( my->monsterAttack != MONSTER_POSE_MAGIC_WINDUP3 )
				{
					limbAnimateToLimit(my, ANIMATE_PITCH, 0.1, 0, false, 0.0);
				}
			}
			continue;
		}
		entity = (Entity*)node->element;
		entity->x = my->x;
		entity->y = my->y;
		entity->z = my->z;

		if ( (MONSTER_ATTACK == MONSTER_POSE_MAGIC_WINDUP1 ) && bodypart == LIMB_HUMANOID_RIGHTARM )
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
				(my->monsterAttack == MONSTER_POSE_MAGIC_WINDUP3
					|| my->monsterAttack == MONSTER_POSE_SPECIAL_WINDUP1 
					|| my->monsterAttack == MONSTER_POSE_MAGIC_WINDUP1
					|| my->monsterAttack == MONSTER_POSE_MAGIC_WINDUP2
					|| (my->monsterAttack == MONSTER_POSE_MAGIC_CAST1)) )
			{
				// leftarm follows the right arm during special mimic attack
				// will not work when shield is visible
				// else animate normally.
				node_t* shieldNode = list_Node(&my->children, 8);
				if ( shieldNode )
				{
					Entity* shield = (Entity*)shieldNode->element;
					if ( shield->flags[INVISIBLE] )
					{
						Entity* weaponarm = nullptr;
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
				}
			}
			else
			{
				if ( bodypart == LIMB_HUMANOID_RIGHTLEG )
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

					node_t* shieldNode = list_Node(&my->children, 8);
					if ( shieldNode )
					{
						Entity* shield = (Entity*)shieldNode->element;
						if ( dist > 0.1 && (bodypart != LIMB_HUMANOID_LEFTARM || shield->sprite == 0) )
						{
							// walking to destination
							if ( !rightbody->skill[0] )
							{
								entity->pitch -= dist * SHADOWWALKSPEED / 2.0;
								if ( entity->pitch < 0 )
								{
									entity->pitch = 0;
									if ( bodypart == LIMB_HUMANOID_RIGHTLEG )
									{
										entity->skill[0] = 1;
									}
								}
							}
							else
							{
								entity->pitch += dist * SHADOWWALKSPEED / 2.0;
								if ( entity->pitch > 3 * PI / 8.0 )
								{
									entity->pitch = 3 * PI / 8.0;
									if ( bodypart == LIMB_HUMANOID_RIGHTLEG )
									{
										entity->skill[0] = 0;
									}
								}
							}
						}
						else
						{
							// coming to a stop
							if ( entity->pitch < PI / 4 )
							{
								entity->pitch += 1 / fmax(dist * .1, 10.0);
								if ( entity->pitch > PI / 4 )
								{
									entity->pitch = PI / 4;
								}
							}
							else if ( entity->pitch > PI / 4 )
							{
								entity->pitch -= 1 / fmax(dist * .1, 10.0);
								if ( entity->pitch < PI / 4 )
								{
									entity->pitch = PI / 4;
								}
							}
						}
					}
				}
				else
				{
					my->humanoidAnimateWalk(entity, node, bodypart, SHADOWWALKSPEED, dist, 0.4);
				}
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
				
					if ( my->monsterAttack == MONSTER_POSE_MAGIC_WINDUP3 )
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
							// play casting sound
							playSoundEntityLocal(my, 170, 64);
							// monster scream
							playSoundEntityLocal(my, MONSTER_SPOTSND, 128);
							if ( multiplayer != CLIENT )
							{
								// freeze in place.
								myStats->EFFECTS[EFF_PARALYZED] = true;
								myStats->EFFECTS_TIMERS[EFF_PARALYZED] = 100;
							}
						}

						limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.25, 5 * PI / 4, false, 0.0);
						if ( multiplayer != CLIENT )
						{
							// move the head and weapon yaw
							limbAnimateToLimit(my, ANIMATE_PITCH, -0.1, 14 * PI / 8, true, 0.1);
							limbAnimateToLimit(my, ANIMATE_WEAPON_YAW, 0.25, 1 * PI / 8, false, 0.0);
						}

						if ( my->monsterAttackTime >= 3 * ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
						{
							if ( multiplayer != CLIENT )
							{
								// cast spell on target.
								Entity* target = uidToEntity(my->monsterTarget);
								if ( target )
								{
									Entity* spellEntity = createParticleSapCenter(my, target, SHADOW_SPELLCAST, 624, 624);
									if ( spellEntity )
									{
										playSoundEntity(target, 251, 128); // play sound on hit target.
									}
									my->attack(MONSTER_POSE_SPECIAL_WINDUP1, 0, nullptr);
									my->shadowTeleportToTarget(target, 7);
									my->setEffect(EFF_INVISIBLE, true, TICKS_PER_SECOND * 10, true);
								}
							}
						}
					}
					else if ( my->monsterAttack == MONSTER_POSE_SPECIAL_WINDUP1 )
					{
						if ( my->monsterAttackTime == 0 )
						{
							// init rotations
							weaponarm->skill[1] = 0;
						}

						if ( weaponarm->skill[1] == 0 && my->monsterAttackTime > 2 * ANIMATE_DURATION_WINDUP )
						{
							// swing and flare out arm.
							if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, 0.25, 1 * PI / 4, false, 0.0) && limbAnimateToLimit(weaponarm, ANIMATE_ROLL, -0.1, 30 * PI / 16, false, 0.0) )
							{
								weaponarm->skill[1] = 1;
							}
						}
						else if ( weaponarm->skill[1] == 1 )
						{
							// return to neutral pitch.
							limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.25, 0, false, 0.0);
						}
						if ( my->monsterAttackTime >= 4 * ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
						{
							weaponarm->skill[0] = rightbody->skill[0];
							weaponarm->pitch = rightbody->pitch;
							weaponarm->roll = 0;
							my->monsterWeaponYaw = 0;
							my->monsterArmbended = 0;
							my->monsterAttack = 0;
							Entity* leftarm = nullptr;
							node_t* leftarmNode = list_Node(&my->children, LIMB_HUMANOID_LEFTARM);
							if ( leftarmNode )
							{
								leftarm = (Entity*)leftarmNode->element;
								leftarm->roll = 0;
							}
						}
					}
					// vertical chop attack
					else if ( my->monsterAttack == MONSTER_POSE_MAGIC_CAST1 )
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
							if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.25, 7 * PI / 4, false, 0.0) )
							{
								weaponarm->skill[0] = rightbody->skill[0];
								my->monsterWeaponYaw = 0;
								weaponarm->pitch = rightbody->pitch;
								weaponarm->roll = 0;
								my->monsterArmbended = 0;
								my->monsterAttack = 0;
								Entity* leftarm = nullptr;
								// set leftbody to right leg.
								node_t* leftarmNode = list_Node(&my->children, LIMB_HUMANOID_RIGHTLEG);
								if ( leftarmNode )
								{
									leftarm = (Entity*)leftarmNode->element;
									leftarm->pitch = PI / 16;
									leftarm->roll = 0;
								}
								else
								{
									return;
								}
							}
						}
					}
					else
					{
						my->handleWeaponArmAttack(weaponarm);
					}
				}
			}
			else if ( bodypart == LIMB_HUMANOID_CLOAK )
			{
				entity->pitch = entity->fskill[0];
			}

			if ( bodypart == LIMB_HUMANOID_LEFTLEG )
			{
				if ( bodypart != LIMB_HUMANOID_RIGHTARM || (my->monsterAttack == 0 && my->monsterAttackTime == 0) )
				{
					if ( dist > 0.1 )
					{
						if ( entity->skill[0] )
						{
							entity->pitch -= dist * SHADOWWALKSPEED / 2.0;
							if ( entity->pitch < 0 )
							{
								entity->skill[0] = 0;
								entity->pitch = 0;
							}
						}
						else
						{
							entity->pitch += dist * SHADOWWALKSPEED / 2.0;
							if ( entity->pitch > 3 * PI / 8.0 )
							{
								entity->skill[0] = 1;
								entity->pitch = 3 * PI / 8.0;
							}
						}
					}
					else
					{
						if ( entity->pitch < PI / 4 )
						{
							entity->pitch += 1 / fmax(dist * .1, 10.0);
							if ( entity->pitch > PI / 4 )
							{
								entity->pitch = PI / 4;
							}
						}
						else if ( entity->pitch > PI / 4 )
						{
							entity->pitch -= 1 / fmax(dist * .1, 10.0);
							if ( entity->pitch < PI / 4 )
							{
								entity->pitch = PI / 4;
							}
						}
					}
				}
			}
			else
			{
				my->humanoidAnimateWalk(entity, node, bodypart, SHADOWWALKSPEED, dist, 0.4);
			}

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
						entity->sprite = 482;
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
				entity->sprite = 436;
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
				entity->sprite = 435;
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
						entity->focalx = limbs[SHADOW][4][0] - 0.25; // 0
						entity->focaly = limbs[SHADOW][4][1] - 0.25; // 0
						entity->focalz = limbs[SHADOW][4][2]; // 2
						entity->sprite = 433;
					}
					else
					{
						// else flex arm.
						entity->focalx = limbs[SHADOW][4][0];
						entity->focaly = limbs[SHADOW][4][1];
						entity->focalz = limbs[SHADOW][4][2];
						entity->sprite = 434;
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
				node_t* shieldNode = list_Node(&my->children, 8);
				if ( shieldNode )
				{
					Entity* shield = (Entity*)shieldNode->element;
					if ( shield->flags[INVISIBLE] && my->monsterState == MONSTER_STATE_WAIT )
					{
						// if weapon invisible and I'm not attacking, relax arm.
						entity->focalx = limbs[SHADOW][5][0] - 0.25; // 0
						entity->focaly = limbs[SHADOW][5][1] + 0.25; // 0
						entity->focalz = limbs[SHADOW][5][2]; // 2
						entity->sprite = 431;
					}
					else
					{
						// else flex arm.
						entity->focalx = limbs[SHADOW][5][0];
						entity->focaly = limbs[SHADOW][5][1];
						entity->focalz = limbs[SHADOW][5][2];
						entity->sprite = 432;
						if ( my->monsterAttack == MONSTER_POSE_MAGIC_WINDUP3 || my->monsterAttack == MONSTER_POSE_SPECIAL_WINDUP1 )
						{
							entity->yaw -= MONSTER_WEAPONYAW;
						}
						else if ( my->monsterAttack == MONSTER_POSE_MAGIC_WINDUP1 )
						{
							entity->yaw += (my->yaw - weaponarm->yaw);
						}
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
				else
				{
					if ( entity->sprite <= 0 )
					{
						entity->flags[INVISIBLE] = true;
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
				else
				{
					if ( entity->sprite <= 0 )
					{
						entity->flags[INVISIBLE] = true;
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
				else
				{
					if ( entity->sprite <= 0 )
					{
						entity->flags[INVISIBLE] = true;
					}
				}
				entity->x -= cos(my->yaw);
				entity->y -= sin(my->yaw);
				entity->yaw += PI / 2;
				break;
				// helm
			case LIMB_HUMANOID_HELMET:
				entity->focalx = limbs[SHADOW][9][0]; // 0
				entity->focaly = limbs[SHADOW][9][1]; // 0
				entity->focalz = limbs[SHADOW][9][2]; // -2
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
				else
				{
					if ( entity->sprite <= 0 )
					{
						entity->flags[INVISIBLE] = true;
					}
				}
				my->setHelmetLimbOffset(entity);
				break;
				// mask
			case LIMB_HUMANOID_MASK:
				entity->focalx = limbs[SHADOW][10][0]; // 0
				entity->focaly = limbs[SHADOW][10][1]; // 0
				entity->focalz = limbs[SHADOW][10][2]; // .25
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
				else
				{
					if ( entity->sprite <= 0 )
					{
						entity->flags[INVISIBLE] = true;
					}
				}
				if ( entity->sprite != 165 )
				{
					entity->focalx = limbs[SHADOW][10][0] + .35; // .35
					entity->focaly = limbs[SHADOW][10][1] - 2; // -2
					entity->focalz = limbs[SHADOW][10][2]; // .25
				}
				else
				{
					entity->focalx = limbs[SHADOW][10][0] + .25; // .25
					entity->focaly = limbs[SHADOW][10][1] - 2.25; // -2.25
					entity->focalz = limbs[SHADOW][10][2]; // .25
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

bool Entity::shadowCanWieldItem(const Item& item) const
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
		case THROWN:
			return true;
		case ARMOR:
			if ( checkEquipType(&item) == TYPE_SHIELD )
			{
				return true; //Can only wear shields, out of armor.
			}
			return false;
		default:
			return false;
	}
}

void Entity::shadowSpecialAbility(bool initialMimic)
{
	//1. Turn invisible.
	//2. Mimic target's weapon & shield (only on initial cast).
	//3. Random chance to mimic other things.
	//4. Teleport to target.

	Stat *myStats = getStats();
	if ( !myStats )
	{
		return;
	}

	Entity *target = uidToEntity(monsterTarget);
	if ( !target )
	{
		//messagePlayer(clientnum, "Shadow's target deaded!");
		monsterReleaseAttackTarget();
		return;
	}

	Stat* targetStats = target->getStats();
	if ( !targetStats )
	{
		monsterReleaseAttackTarget(true); //Force get rid of the target since it has no stats -- it's useless to us!
		return;
	}

	//1. Turn invisible.
	//myStats->EFFECTS[EFF_INVISIBLE] = true;
	//myStats->EFFECTS_TIMERS[EFF_INVISIBLE] = 0; //Does not deactivate until it attacks.
	//messagePlayer(clientnum, "Turned invisible!");

	int numSpellsToMimic = 2;
	int numSkillsToMimic = 3;

	//2. Copy target's weapon & shield on initial activation of this ability only.
	if ( initialMimic )
	{
		if ( !monsterShadowDontChangeName )
		{
			std::string newName = "Shadow of ";
			if ( strcmp(targetStats->name, "") != 0 )
			{
				newName += targetStats->name;
			}
			else
			{
				newName += monstertypename[targetStats->type];
			}
			strcpy(myStats->name, newName.c_str());
		}

		monsterShadowInitialMimic = 0;
		//messagePlayer(clientnum, "[DEBUG: Entity::shadowSpecialAbility() ] Initial mimic.");
		//TODO: On initial mimic, need to reset some the tracking info on what's already been mimic'ed.
		//Such as dropping already equipped items.
		if ( itemCategory(myStats->weapon) == SPELLBOOK )
		{
			//Don't want to drop spellbooks, though. Then the shadow would lose the spell.
			addItemToMonsterInventory(myStats->weapon);
			myStats->weapon = nullptr;
		}
		else
		{
			dropItemMonster(myStats->weapon, this, myStats);
		}
		dropItemMonster(myStats->shield, this, myStats);

		//Skills do not get reset.
		//Attributes do not get reset.
		//Spells do not get reset.

		//On initial mimic, copy best melee weapon and shield from target's hands or inventory.
		Item *bestMeleeWeapon = target->getBestMeleeWeaponIHave();
		Item *bestShield = target->getBestShieldIHave();

		if ( bestMeleeWeapon )
		{
			Item* wieldedCopy = new Item();
			copyItem(wieldedCopy, bestMeleeWeapon);
			wieldedCopy->appearance = MONSTER_ITEM_UNDROPPABLE_APPEARANCE;
			monsterEquipItem(*wieldedCopy, &myStats->weapon);
		}

		if ( bestShield )
		{
			Item* wieldedCopy = new Item();
			copyItem(wieldedCopy, bestShield);
			wieldedCopy->appearance = MONSTER_ITEM_UNDROPPABLE_APPEARANCE;
			monsterEquipItem(*wieldedCopy, &myStats->shield);
		}

		//On initial mimic, copy more spells & skills.
		numSkillsToMimic += rand()%3 + 1;
		numSpellsToMimic += rand()%3 + 1;

		if ( target->behavior == actPlayer )
		{
			messagePlayer(target->skill[2], language[2516]);
		}
	}
	else
	{
		if ( target->behavior == actPlayer )
		{
			messagePlayer(target->skill[2], language[2517]);
		}
	}

	//3. Random chance to mimic other things.
	//Mimic target's skills (proficiencies).
	//First, get proficiencies to mimic:
	std::vector<int> skillsCanMimic;
	for ( int i = 0; i < NUMPROFICIENCIES; ++i )
	{
		if ( targetStats->PROFICIENCIES[i] > myStats->PROFICIENCIES[i] )
		{
			//Target is better, can mimic this proficiency.
			skillsCanMimic.push_back(i);
		}
	}
	//Now choose a random skill and copy it over.
	for ( int skillsMimicked = 0; skillsCanMimic.size() && skillsMimicked < numSkillsToMimic; ++skillsMimicked )
	{
		int choosen = rand()%skillsCanMimic.size();
		myStats->PROFICIENCIES[skillsCanMimic[choosen]] = targetStats->PROFICIENCIES[skillsCanMimic[choosen]];

		//messagePlayer(clientnum, "DEBUG: Shadow mimicked skill %d.", skillsCanMimic[choosen]);
		skillsCanMimic.erase(skillsCanMimic.begin() + choosen); //No longer an eligible skill.
	}

	//Mimick spells.
	//First, search the target's inventory for spells the shadow likes.
	//For a player, it searches for the spell item.
	//For a monster, it searches for spellbooks (since monsters don't learn spells the normal way.
	std::vector<int> spellsCanMimic; //Array of spell IDs.
	if ( target->behavior == actMonster && itemCategory(targetStats->weapon) == SPELLBOOK )
	{
		int spellID = getSpellIDFromSpellbook(targetStats->weapon->type);
		if ( spellID != SPELL_NONE && shadowCanMimickSpell(spellID) && !monsterHasSpellbook(getSpellIDFromSpellbook(targetStats->weapon->type)) )
		{
			spellsCanMimic.push_back(spellID);
		}
	}
	for ( node_t* node = targetStats->inventory.first; node; node = node->next)
	{
		Item* item = static_cast<Item*>(node->element);
		if ( !item )
		{
			continue;
		}

		if ( target->behavior == actPlayer )
		{
			//Search player's inventory for the special spell item.
			if ( itemCategory(item) != SPELL_CAT )
			{
				continue;
			}

			spell_t *spell = getSpellFromItem(item); //Do not free or delete this.
			if ( !spell )
			{
				continue;
			}

			Item* spellbook = getSpellbookFromSpellID(spell->ID);
			if ( !spellbook )
			{
				continue;
			}

			if ( shadowCanMimickSpell(spell->ID) && !monsterHasSpellbook(getSpellIDFromSpellbook(spellbook->type)) )
			{
				spellsCanMimic.push_back(spell->ID);
			}

			free(spellbook);
		}
		else
		{
			//Search monster's inventory for spellbooks.
			if ( itemCategory(item) != SPELLBOOK )
			{
				continue;
			}

			spell_t *spell = getSpellFromID(getSpellIDFromSpellbook(item->type));

			if ( shadowCanMimickSpell(spell->ID) && !monsterHasSpellbook(getSpellIDFromSpellbook(item->type)) )
			{
				spellsCanMimic.push_back(spell->ID);
			}
		}
	}
	//Now randomly choose & copy over a spell.
	for ( int spellsMimicked = 0; spellsCanMimic.size() && spellsMimicked < numSkillsToMimic; ++spellsMimicked )
	{
		int choosen = rand()%spellsCanMimic.size();

		Item* spellbook = getSpellbookFromSpellID(spellsCanMimic[choosen]);

		if ( spellbook )
		{
			spellbook->appearance = MONSTER_ITEM_UNDROPPABLE_APPEARANCE;
			addItemToMonsterInventory(spellbook);

			//TODO: Delete debug.
			spell_t* spell = getSpellFromID(getSpellIDFromSpellbook(spellbook->type));
			//messagePlayer(clientnum, "DEBUG: Shadow mimicked spell %s.", spell->name);
		}

		spellsCanMimic.erase(spellsCanMimic.begin() + choosen); //No longer an eligible spell.
	}

	//shadowTeleportToTarget(target);
}

bool Entity::shadowCanMimickSpell(int spellID)
{
	switch ( spellID )
	{
		case SPELL_FORCEBOLT:
		case SPELL_MAGICMISSILE:
		case SPELL_COLD:
		case SPELL_FIREBALL:
		case SPELL_LIGHTNING:
		case SPELL_SLEEP:
		case SPELL_CONFUSE:
		case SPELL_SLOW:
		case SPELL_STONEBLOOD:
		case SPELL_BLEED:
		case SPELL_ACID_SPRAY:
			return true;
		default:
			return false;
	}
}

void Entity::shadowTeleportToTarget(const Entity* target, int range)
{
	Entity* spellTimer = createParticleTimer(this, 60, 625);
	spellTimer->particleTimerPreDelay = 20; // wait 20 ticks before animation.
	spellTimer->particleTimerEndAction = PARTICLE_EFFECT_SHADOW_TELEPORT; // teleport behavior of timer.
	spellTimer->particleTimerEndSprite = 625; // sprite to use for end of timer function.
	spellTimer->particleTimerCountdownAction = 1;
	spellTimer->particleTimerCountdownSprite = 625;
	if ( target != nullptr )
	{
		spellTimer->particleTimerTarget = static_cast<Sint32>(target->getUID()); // get the target to teleport around.
	}
	spellTimer->particleTimerVariable1 = range; // distance of teleport in tiles
	if ( multiplayer == SERVER )
	{
		serverSpawnMiscParticles(this, PARTICLE_EFFECT_SHADOW_TELEPORT, 625);
	}
}

void Entity::shadowChooseWeapon(const Entity* target, double dist)
{
	if ( monsterSpecialState != 0 )
	{
		//Holding a weapon assigned from the special attack. Don't switch weapons.
		//messagePlayer(clientnum, "Shadow not choosing.");
		// handle idle teleporting to target
		if ( monsterSpecialState == SHADOW_TELEPORT_ONLY && monsterSpecialTimer == 0 )
		{
			monsterSpecialState = 0;
			serverUpdateEntitySkill(this, 33); // for clients to keep track of animation
		}
		return;
	}
	//messagePlayer(clientnum, "Shadow choosing.");

	Stat *myStats = getStats();
	if ( !myStats )
	{
		return;
	}

	int specialRoll = -1;

	bool inMeleeRange = monsterInMeleeRange(target, dist);

	if ( monsterSpecialTimer == 0 && (ticks % 10 == 0) && monsterAttack == 0 )
	{
		//messagePlayer(clientnum, "Preliminary special check.");
		Stat* targetStats = target->getStats();
		if ( !targetStats )
		{
			return;
		}

		/* THIS NEEDS TO BE ELSEWHERE, TO BE CALLED CONSTANTLY TO ALLOW SHADOW TO TELEPORT IF NO PATH/ DISTANCE IS TOO GREAT */

		// occurs less often against fellow monsters.
		specialRoll = rand() % (20 + 50 * (target->behavior == &actMonster));

		int requiredRoll = 10;

		// check the roll
		if ( specialRoll < requiredRoll )
		//if ( rand() % 150 )
		{
			//messagePlayer(clientnum, "Rolled the special!");
			node_t* node = nullptr;
			bool telemimic  = (rand() % 4 == 0); //By default, 25% chance it'll telepotty instead of casting a spell.
			if ( monsterState != MONSTER_STATE_ATTACK )
			{
				//If it's hunting down the player, always want it to teleport and find them.
				telemimic = true;
				//messagePlayer(clientnum, "Forcing tele-mimic!");
			}

			if ( telemimic )
			{
				//Do the tele-mimic-invisibility special ability.
				//messagePlayer(clientnum, "Executing telemimic.");
				//monsterShadowInitialMimic = 0; //False!
				monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_SHADOW_TELEMIMICINVISI_ATTACK;
				attack(MONSTER_POSE_MAGIC_WINDUP3, 0, nullptr);
				return;
			}

			//messagePlayer(clientnum, "Defaulting to spell.");
			node = chooseAttackSpellbookFromInventory();
			if ( node != nullptr )
			{
				//messagePlayer(clientnum, "Shadow equipped a spell!");
				swapMonsterWeaponWithInventoryItem(this, myStats, node, true, true);
				monsterSpecialState = SHADOW_SPELLCAST;
				serverUpdateEntitySkill(this, 33); // for clients to keep track of animation
				monsterHitTime = HITRATE * 2; // force immediate attack
				return;
			}
			else
			{
				//Always set the cooldown, even if didn't cast anything.
				monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_SHADOW_SPELLCAST;
			}
		}
	}

	if ( inMeleeRange ) //Shadows are paunchy people, they don't like to shoost much.
	{
		//Switch to a melee weapon if not already wielding one. Unless monster special state is overriding the AI.
		if ( !myStats->weapon || !isMeleeWeapon(*myStats->weapon) )
		{
			node_t* weaponNode = getMeleeWeaponItemNodeInInventory(myStats);
			if ( !weaponNode )
			{
				return; //Resort to fists.
			}

			bool swapped = swapMonsterWeaponWithInventoryItem(this, myStats, weaponNode, false, false);
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
	return;
}




