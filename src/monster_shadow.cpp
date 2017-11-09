/*-------------------------------------------------------------------------------

BARONY
File: monster_shadow.cpp
Desc: implements all of the shadow monster's code

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

void initShadow(Entity* my, Stat* myStats)
{
	int c;
	node_t* node;

	my->initMonster(481);

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 60; //TODO: Shadow SPOTSND.
		MONSTER_SPOTVAR = 3;
		MONSTER_IDLESND = 98; //TODO: Shadow IDLESND.
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

	playSoundEntity(my, 63 + rand() % 3, 128);

	my->removeMonsterDeathNodes();

	list_RemoveNode(my->mynode);
	return;
}

#define SHADOWWALKSPEED .13

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
			my->z = -1;
			my->pitch = 0;
		}
	}

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
			continue;
		}
		entity = (Entity*)node->element;
		entity->x = my->x;
		entity->y = my->y;
		entity->z = my->z;

		if ( MONSTER_ATTACK == MONSTER_POSE_MAGIC_WINDUP3 && bodypart == LIMB_HUMANOID_RIGHTARM )
		{
			// don't let the creatures's yaw move the casting arm
		}
		else
		{
			entity->yaw = my->yaw;
		}

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
						entity->pitch -= dist * SHADOWWALKSPEED;
						if ( entity->pitch < -PI / 4.0 )
						{
							entity->pitch = -PI / 4.0;
							if ( bodypart == 3 )
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
						entity->pitch += dist * SHADOWWALKSPEED;
						if ( entity->pitch > PI / 4.0 )
						{
							entity->pitch = PI / 4.0;
							if ( bodypart == 3 )
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
				else if ( MONSTER_ATTACK == MONSTER_POSE_MAGIC_WINDUP3 ) //TODO: The animation is bork.
				{
					// magic wiggle hands
					if ( my->monsterAttackTime == 0 )
					{
						// init rotations
						my->monsterArmbended = 0;
						my->monsterWeaponYaw = 0;
						weaponarm->roll = 0;
						weaponarm->pitch = 0;
						weaponarm->yaw = my->yaw;
						weaponarm->skill[1] = 0;
						// casting particles
						createParticleDot(my);
						// play casting sound
						playSoundEntityLocal(my, 170, 32);
					}

					double animationYawSetpoint = 0.f;
					double animationYawEndpoint = 0.f;
					double armSwingRate = 0.f;
					double animationPitchSetpoint = 0.f;
					double animationPitchEndpoint = 0.f;

					switch ( my->monsterSpellAnimation )
					{
						case MONSTER_SPELLCAST_NONE:
							break;
						case MONSTER_SPELLCAST_SMALL_HUMANOID:
							// smaller models so arms can wave in a larger radius and faster.
							animationYawSetpoint = normaliseAngle2PI(my->yaw + 2 * PI / 8);
							animationYawEndpoint = normaliseAngle2PI(my->yaw - 2 * PI / 8);
							animationPitchSetpoint = 2 * PI / 8;
							animationPitchEndpoint = 14 * PI / 8;
							armSwingRate = 0.3;
							if ( my->monsterAttackTime == 0 )
							{
								weaponarm->yaw = my->yaw - PI / 8;
							}
							break;
						case MONSTER_SPELLCAST_HUMANOID:
							animationYawSetpoint = normaliseAngle2PI(my->yaw + 1 * PI / 8);
							animationYawEndpoint = normaliseAngle2PI(my->yaw - 1 * PI / 8);
							animationPitchSetpoint = 1 * PI / 8;
							animationPitchEndpoint = 15 * PI / 8;
							armSwingRate = 0.15;
							break;
						default:
							break;
					}

					if ( weaponarm->skill[1] == 0 )
					{
						if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, armSwingRate, animationPitchSetpoint, false, 0.0) )
						{
							if ( limbAnimateToLimit(weaponarm, ANIMATE_YAW, armSwingRate, animationYawSetpoint, false, 0.0) )
							{
								weaponarm->skill[1] = 1;
							}
						}
					}
					else
					{
						if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -armSwingRate, animationPitchEndpoint, false, 0.0) )
						{
							if ( limbAnimateToLimit(weaponarm, ANIMATE_YAW, -armSwingRate, animationYawEndpoint, false, 0.0) )
							{
								weaponarm->skill[1] = 0;
							}
						}
					}

					if ( my->monsterAttackTime >= 2 * ANIMATE_DURATION_WINDUP_SHADOW_SPECIAL / (monsterGlobalAnimationMultiplier / 10.0) )
					{
						//TODO: my->returnWeaponarmToNeutral()?
						MONSTER_ATTACK = 0;
						if ( multiplayer != CLIENT )
						{
							// swing the arm after we prepped the spell
							//this->attack(MONSTER_POSE_MAGIC_WINDUP2, 0, nullptr);
							messagePlayer(clientnum, "TODO: Shadow invisibility mimic teleport and stuff.");
							my->shadowSpecialAbility(true);
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
						entity->pitch -= dist * SHADOWWALKSPEED;
						if ( entity->pitch < -PI / 4.0 )
						{
							entity->skill[0] = 0;
							entity->pitch = -PI / 4.0;
						}
					}
					else
					{
						entity->pitch += dist * SHADOWWALKSPEED;
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
		case 3:
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
		case 4:
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
		case 5:
		{
			entity->sprite = 433;
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
					entity->focalx = limbs[SHADOW][4][0]; // 0
					entity->focaly = limbs[SHADOW][4][1]; // 0
					entity->focalz = limbs[SHADOW][4][2]; // 1.5
				}
				else
				{
					entity->focalx = limbs[SHADOW][4][0] + 0.75;
					entity->focaly = limbs[SHADOW][4][1];
					entity->focalz = limbs[SHADOW][4][2] - 0.75;
				}
			}
			entity->x += 2.5 * cos(my->yaw + PI / 2) - .20 * cos(my->yaw);
			entity->y += 2.5 * sin(my->yaw + PI / 2) - .20 * sin(my->yaw);
			entity->z += 1.5;
			entity->yaw += MONSTER_WEAPONYAW;
			if ( my->z >= 2.4 && my->z <= 2.6 )
			{
				entity->pitch = 0;
			}
			break;
			// left arm
		}
		case 6:
		{
			entity->sprite = 431;
			node_t* shieldNode = list_Node(&my->children, 8);
			if ( shieldNode )
			{
				Entity* shield = (Entity*)shieldNode->element;
				entity->sprite += (shield->flags[INVISIBLE] != true);
				if ( shield->flags[INVISIBLE] )
				{
					entity->focalx = limbs[SHADOW][5][0]; // 0
					entity->focaly = limbs[SHADOW][5][1]; // 0
					entity->focalz = limbs[SHADOW][5][2]; // 1.5
				}
				else
				{
					entity->focalx = limbs[SHADOW][5][0] + 0.75;
					entity->focaly = limbs[SHADOW][5][1];
					entity->focalz = limbs[SHADOW][5][2] - 0.75;
				}
			}
			entity->x -= 2.5 * cos(my->yaw + PI / 2) + .20 * cos(my->yaw);
			entity->y -= 2.5 * sin(my->yaw + PI / 2) + .20 * sin(my->yaw);
			entity->z += 1.5;
			if ( my->z >= 2.4 && my->z <= 2.6 )
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
					entity->focalx = limbs[SHADOW][6][0]; // 1.5
					if ( entity->sprite == items[CROSSBOW].index )
					{
						entity->focalx += 2;
					}
					entity->focaly = limbs[SHADOW][6][1]; // 0
					entity->focalz = limbs[SHADOW][6][2]; // -.5
				}
				else
				{
					entity->focalx = limbs[SHADOW][6][0] + 1.5; // 3
					entity->focaly = limbs[SHADOW][6][1]; // 0
					entity->focalz = limbs[SHADOW][6][2] - 2; // -2.5
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
			if ( entity->sprite != items[STEEL_HELM].index )
			{
				if ( entity->sprite == items[HAT_PHRYGIAN].index )
				{
					entity->focalx = limbs[SHADOW][9][0] - .5;
					entity->focaly = limbs[SHADOW][9][1] - 3.55;
					entity->focalz = limbs[SHADOW][9][2] + 2.5;
					entity->roll = PI / 2;
				}
				else if ( entity->sprite >= items[HAT_HOOD].index && entity->sprite < items[HAT_HOOD].index + items[HAT_HOOD].variations )
				{
					entity->focalx = limbs[SHADOW][9][0] - .5;
					entity->focaly = limbs[SHADOW][9][1] - 2.75;
					entity->focalz = limbs[SHADOW][9][2] + 2.5;
					entity->roll = PI / 2;
				}
				else if ( entity->sprite == items[HAT_WIZARD].index )
				{
					entity->focalx = limbs[SHADOW][9][0];
					entity->focaly = limbs[SHADOW][9][1] - 5;
					entity->focalz = limbs[SHADOW][9][2] + 2.5;
					entity->roll = PI / 2;
				}
				else if ( entity->sprite == items[HAT_JESTER].index )
				{
					entity->focalx = limbs[SHADOW][9][0];
					entity->focaly = limbs[SHADOW][9][1] - 5;
					entity->focalz = limbs[SHADOW][9][2] + 2.5;
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
	if ( MONSTER_ATTACK != 0 )
	{
		MONSTER_ATTACKTIME++;
	}
	else
	{
		MONSTER_ATTACKTIME = 0;
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
	//TODO: Turn invisible.
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
		messagePlayer(clientnum, "Shadow's target deaded!");
		monsterReleaseAttackTarget();
		return;
	}

	Stat* targetStats = target->getStats();
	if ( !targetStats )
	{
		monsterReleaseAttackTarget(true); //Force get rid of the target since it has no stats -- it's useless to us!
		return;
	}

	//TODO: Turn invisible.
	//myStats->EFFECTS[EFF_INVISIBLE] = true;
	//myStats->EFFECTS_TIMERS[EFF_INVISIBLE] = 0; //Does not deactivate until it attacks.
	messagePlayer(clientnum, "Turned invisible!");

	int numSpellsToMimic = 2;
	int numSkillsToMimic = 3;

	//2. Copy target's weapon & shield on initial activation of this ability only.
	if ( initialMimic )
	{

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
			monsterEquipItem(*wieldedCopy, &myStats->weapon);
		}

		if ( bestShield )
		{
			Item* wieldedCopy = new Item();
			copyItem(wieldedCopy, bestShield);
			monsterEquipItem(*wieldedCopy, &myStats->shield);
		}

		//On initial mimic, copy more spells & skills.
		numSkillsToMimic += rand()%3 + 1;
		numSpellsToMimic += rand()%3 + 1;
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

		messagePlayer(clientnum, "DEBUG: Shadow mimicked skill %d.", skillsCanMimic[choosen]);
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
		if ( spellID != SPELL_NONE && shadowCanMimickSpell(spellID) && !monsterHasSpellbook(targetStats->weapon->type) )
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

			if ( shadowCanMimickSpell(spell->ID) && !monsterHasSpellbook(spellbook->type) )
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

			if ( shadowCanMimickSpell(spell->ID) && !monsterHasSpellbook(item->type) )
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
			messagePlayer(clientnum, "DEBUG: Shadow mimicked spell %s.", spell->name);
		}

		spellsCanMimic.erase(spellsCanMimic.begin() + choosen); //No longer an eligible spell.
	}

	shadowTeleportToTarget(target);
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

void Entity::shadowTeleportToTarget(const Entity* target)
{
	Entity* spellTimer = createParticleTimer(this, 40, 593);
	spellTimer->particleTimerEndAction = PARTICLE_EFFECT_INCUBUS_TELEPORT_TARGET; // teleport behavior of timer. //TODO: Use correct particles for Shadows.
	spellTimer->particleTimerEndSprite = 593; // sprite to use for end of timer function.
	spellTimer->particleTimerCountdownAction = 1;
	spellTimer->particleTimerCountdownSprite = 593;
	if ( target != nullptr )
	{
		spellTimer->particleTimerTarget = static_cast<Sint32>(target->getUID()); // get the target to teleport around.
	}
	spellTimer->particleTimerVariable1 = 3; // distance of teleport in tiles
	if ( multiplayer == SERVER )
	{
		serverSpawnMiscParticles(this, PARTICLE_EFFECT_INCUBUS_TELEPORT_TARGET, 593);
	}
}

void Entity::shadowChooseWeapon(const Entity* target, double dist)
{
	if ( monsterSpecialState != 0 )
	{
		//Holding a weapon assigned from the special attack. Don't switch weapons.
		return;
	}

	Stat *myStats = getStats();
	if ( !myStats )
	{
		return;
	}

	int specialRoll = -1;

	bool inMeleeRange = monsterInMeleeRange(target, dist);

	if ( monsterSpecialTimer == 0 && (ticks % 10 == 0) && monsterAttack == 0 )
	{
		Stat* targetStats = target->getStats();
		if ( !targetStats )
		{
			return;
		}

		// occurs less often against fellow monsters.
		specialRoll = rand() % (20 + 50 * (target->behavior == &actMonster));

		int requiredRoll = 2;

		// check the roll
		if ( specialRoll < requiredRoll )
		{
			node_t* node = nullptr;
			bool telemimic = (rand()%4 == 0); //By default, 25% chance it'll telepotty instead of casting a spell.
			if ( monsterState != MONSTER_STATE_ATTACK )
			{
				//If it's hunting down the player, always want it to teleport and find them.
				telemimic = true;
			}

			if ( telemimic )
			{
				//Do the tele-mimic-invisibility special ability.
				monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_SHADOW_TELEMIMICINVISI_ATTACK;
				attack(MONSTER_POSE_MAGIC_WINDUP3, 0, nullptr);
				return;
			}

			node = chooseAttackSpellbookFromInventory();
			if ( node != nullptr )
			{
				swapMonsterWeaponWithInventoryItem(this, myStats, node, true, true);
				monsterSpecialState = VAMPIRE_CAST_DRAIN; //TODO: MONSTER_CAST_SPELL
				serverUpdateEntitySkill(this, 33); // for clients to keep track of animation
				monsterHitTime = HITRATE * 2; // force immediate attack
				return;
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




