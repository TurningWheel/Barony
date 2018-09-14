/*-------------------------------------------------------------------------------

	BARONY
	File: monster_incubus.cpp
	Desc: implements all of the incubus monster's code

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

void initIncubus(Entity* my, Stat* myStats)
{
	int c;
	node_t* node;

	//Sprite 445 = incubus head sprite
	my->initMonster(445);

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 282;
		MONSTER_SPOTVAR = 3;
		MONSTER_IDLESND = 276;
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

			bool lesserMonster = false;
			if ( !strncmp(myStats->name, "lesser incubus", strlen("lesser incubus")) )
			{
				lesserMonster = true;
				myStats->HP = 80;
				myStats->MAXHP = myStats->HP;
				myStats->OLDHP = myStats->HP;
				myStats->STR = 12;
				myStats->DEX = 6;
				myStats->CON = 3;
				myStats->INT = -2;
				myStats->PER = 5;
				myStats->CHR = 5;
				myStats->EXP = 0;
				myStats->LVL = 15;
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
				/*myStats->DEX = 10;
				strcpy(myStats->name, "Lilith");
				for ( c = 0; c < 2; c++ )
				{
					Entity* entity = summonMonster(SUCCUBUS, my->x, my->y);
					if ( entity )
					{
						entity->parent = my->getUID();
					}
				}*/
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

			my->setHardcoreStats(*myStats);

			// always give special spell to incubus, undroppable.
			newItem(SPELLBOOK_STEAL_WEAPON, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, &myStats->inventory);
			newItem(SPELLBOOK_CHARM_MONSTER, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, &myStats->inventory);

			if ( rand() % 4 == 0 ) // 1 in 4
			{
				newItem(POTION_CONFUSION, SERVICABLE, 0, 0 + rand() % 3, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, &myStats->inventory);
			}
			else // 3 in 4
			{
				newItem(POTION_BOOZE, SERVICABLE, 0, 1 + rand() % 3, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, &myStats->inventory);
			}


			// generate the default inventory items for the monster, provided the editor sprite allowed enough default slots
			switch ( defaultItems )
			{
				case 6:
				case 5:
				case 4:
				case 3:
					if ( rand() % 2 == 0 && !lesserMonster ) // 1 in 2
					{
						newItem(MAGICSTAFF_COLD, SERVICABLE, 0, 1, rand(), false, &myStats->inventory);
					}
				case 2:
					if ( rand() % 5 == 0 ) // 1 in 5
					{
						newItem(POTION_CONFUSION, SERVICABLE, 0, 1 + rand() % 2, rand(), false, &myStats->inventory);
					}
				case 1:
					if ( rand() % 3 == 0 ) // 1 in 3
					{
						newItem(POTION_BOOZE, SERVICABLE, 0, 1 + rand() % 3, rand(), false, &myStats->inventory);
					}
					break;
				default:
					break;
			}

			//give weapon
			if ( lesserMonster )
			{
				if ( myStats->weapon == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1 )
				{
					switch ( rand() % 10 )
					{
						case 0:
						case 1:
							myStats->weapon = newItem(CROSSBOW, static_cast<Status>(WORN + rand() % 2), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
						case 2:
						case 3:
						case 4:
						case 5:
						case 6:
						case 7:
						case 8:
							myStats->weapon = newItem(STEEL_HALBERD, static_cast<Status>(WORN + rand() % 2), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
						case 9:
							myStats->weapon = newItem(MAGICSTAFF_COLD, static_cast<Status>(WORN + rand() % 2), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
					}
				}
			}
			else if ( myStats->weapon == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1 )
			{
				switch ( rand() % 10 )
				{
					case 0:
					case 1:
					case 2:
					case 3:
						myStats->weapon = newItem(CRYSTAL_SPEAR, static_cast<Status>(WORN + rand() % 2), -1 + rand() % 3, 1, rand(), false, nullptr);
						break;
					case 4:
					case 5:
					case 6:
						myStats->weapon = newItem(CROSSBOW, static_cast<Status>(WORN + rand() % 2), -1 + rand() % 3, 1, rand(), false, nullptr);
						break;
					case 7:
					case 8:
						myStats->weapon = newItem(STEEL_HALBERD, static_cast<Status>(WORN + rand() % 2), -1 + rand() % 3, 1, rand(), false, nullptr);
						break;
					case 9:
						myStats->weapon = newItem(MAGICSTAFF_COLD, static_cast<Status>(WORN + rand() % 2), -1 + rand() % 3, 1, rand(), false, nullptr);
						break;
				}
			}

			// give shield
			if ( myStats->shield == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_SHIELD] == 1 )
			{
				switch ( rand() % 10 )
				{
					case 0:
					case 1:
					case 2:
					case 3:
					case 4:
					case 5:
					case 6:
					case 7:
						break;
					case 8:
					case 9:
						myStats->shield = newItem(MIRROR_SHIELD, static_cast<Status>(WORN + rand() % 2), -1 + rand() % 3, 1, rand(), false, nullptr);
						break;
				}
			}
		}
	}

	// torso
	Entity* entity = newEntity(446, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[INCUBUS][1][0]; // 0
	entity->focaly = limbs[INCUBUS][1][1]; // 0
	entity->focalz = limbs[INCUBUS][1][2]; // 0
	entity->behavior = &actIncubusLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right leg
	entity = newEntity(450, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[INCUBUS][2][0]; // 1
	entity->focaly = limbs[INCUBUS][2][1]; // 0
	entity->focalz = limbs[INCUBUS][2][2]; // 2
	entity->behavior = &actIncubusLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left leg
	entity = newEntity(449, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[INCUBUS][3][0]; // 1
	entity->focaly = limbs[INCUBUS][3][1]; // 0
	entity->focalz = limbs[INCUBUS][3][2]; // 2
	entity->behavior = &actIncubusLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right arm
	entity = newEntity(448, 0, map.entities, nullptr); //595 //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[INCUBUS][4][0]; // -.25
	entity->focaly = limbs[INCUBUS][4][1]; // 0
	entity->focalz = limbs[INCUBUS][4][2]; // 1.5
	entity->behavior = &actIncubusLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left arm
	entity = newEntity(447, 0, map.entities, nullptr); //447 //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[INCUBUS][5][0]; // -.25
	entity->focaly = limbs[INCUBUS][5][1]; // 0
	entity->focalz = limbs[INCUBUS][5][2]; // 1.5
	entity->behavior = &actIncubusLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// world weapon
	entity = newEntity(-1, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[INCUBUS][6][0]; // 
	entity->focaly = limbs[INCUBUS][6][1]; // 
	entity->focalz = limbs[INCUBUS][6][2]; // 
	entity->behavior = &actIncubusLimb;
	entity->parent = my->getUID();
	entity->pitch = .25;
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// shield
	entity = newEntity(-1, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[INCUBUS][7][0]; // 
	entity->focaly = limbs[INCUBUS][7][1]; // 
	entity->focalz = limbs[INCUBUS][7][2]; // 
	entity->behavior = &actIncubusLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);
}

bool actIncubusLimb(Entity* my)
{
	return my->actMonsterLimb();
}

void incubusDie(Entity* my)
{
	int c;
	for ( c = 0; c < 5; c++ )
	{
		Entity* gib = spawnGib(my);
		serverSpawnGibForClient(gib);
	}

	my->spawnBlood();

	playSoundEntity(my, 279 + rand() % 3, 128);

	my->removeMonsterDeathNodes();

	list_RemoveNode(my->mynode);
	return;
}

#define INCUBUSWALKSPEED .07

void incubusMoveBodyparts(Entity* my, Stat* myStats, double dist)
{
	node_t* node;
	Entity* entity = nullptr, *entity2 = nullptr;
	Entity* rightbody = nullptr;
	Entity* weaponarm = nullptr;
	int bodypart;
	bool wearingring = false;

	// set invisibility //TODO: isInvisible()?
	if ( multiplayer != CLIENT )
	{
		if ( myStats->ring != nullptr )
		{
			if ( myStats->ring->type == RING_INVISIBILITY )
			{
				wearingring = true;
			}
		}
		if ( myStats->cloak != nullptr )
		{
			if ( myStats->cloak->type == CLOAK_INVISIBILITY )
			{
				wearingring = true;
			}
		}
		if ( myStats->EFFECTS[EFF_INVISIBLE] == true )
		{
			my->flags[INVISIBLE] = true;
			my->flags[BLOCKSIGHT] = false;
			bodypart = 0;
			for (node = my->children.first; node != nullptr; node = node->next)
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
			for (node = my->children.first; node != nullptr; node = node->next)
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
			my->z = 1.5;
		}
		else
		{
			my->z = -1;
		}
	}

	Entity* shieldarm = nullptr;

	//Move bodyparts
	for (bodypart = 0, node = my->children.first; node != nullptr; node = node->next, bodypart++)
	{
		if ( bodypart < LIMB_HUMANOID_TORSO )
		{
			// post-swing head animation. client doesn't need to adjust the entity pitch, server will handle.
			if ( multiplayer != CLIENT && bodypart == 1 )
			{
				if ( my->monsterAttack != MONSTER_POSE_MAGIC_WINDUP3 && my->monsterAttack != MONSTER_POSE_INCUBUS_TELEPORT )
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
		if ( MONSTER_ATTACK == MONSTER_POSE_MAGIC_WINDUP1 && bodypart == LIMB_HUMANOID_RIGHTARM )
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
				((my->monsterSpecialState == INCUBUS_STEAL && my->monsterAttack != 0 ) ||
				my->monsterAttack == MONSTER_POSE_INCUBUS_TELEPORT) )
			{
				// leftarm follows the right arm during special steal state/teleport attack
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
				my->humanoidAnimateWalk(entity, node, bodypart, INCUBUSWALKSPEED, dist, 0.4);
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

					// potion special throw
					if ( my->monsterAttack == MONSTER_POSE_SPECIAL_WINDUP1 )
					{
						if ( my->monsterAttackTime == 0 )
						{
							// init rotations
							weaponarm->pitch = 0;
							my->monsterArmbended = 0;
							my->monsterWeaponYaw = 0;
							weaponarm->roll = 0;
							createParticleDot(my);
						}

						limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.25, 5 * PI / 4, false, 0.0);

						if ( my->monsterAttackTime >= ANIMATE_DURATION_WINDUP * 4 / (monsterGlobalAnimationMultiplier / 10.0) )
						{
							if ( multiplayer != CLIENT )
							{
								my->attack(1, 0, nullptr);
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
							// play casting sound
							playSoundEntityLocal(my, 170, 64);
							// monster scream
							playSoundEntityLocal(my, MONSTER_SPOTSND + 1, 128);
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
								// throw the spell
								my->attack(MONSTER_POSE_MELEE_WINDUP1, 0, nullptr);
							}
						}
					}
					// teleport animation
					else if( my->monsterAttack == MONSTER_POSE_INCUBUS_TELEPORT )
					{
						if ( my->monsterAttackTime == 0 )
						{
							// init rotations
							weaponarm->pitch = 0;
							my->monsterArmbended = 0;
							my->monsterWeaponYaw = 0;
							weaponarm->roll = 0;
							weaponarm->skill[1] = 0; // use this for direction of animation
							// monster scream
							playSoundEntityLocal(my, MONSTER_SPOTSND + 2, 128);
							if ( multiplayer != CLIENT )
							{
								// set overshoot for head, freeze incubus in place
								my->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_TO_SETPOINT;
								myStats->EFFECTS[EFF_PARALYZED] = true;
								myStats->EFFECTS_TIMERS[EFF_PARALYZED] = 100;
							}
						}

						if ( weaponarm->skill[1] == 0 )
						{
							// wind up arm, change direction when setpoint reached.
							if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.2, 13 * PI / 8, false, 0.0) )
							{
								weaponarm->skill[1] = 1;
							}
						}
						else
						{
							// swing and flare out arm.
							limbAnimateToLimit(weaponarm, ANIMATE_PITCH, 0.1, 2 * PI / 16, false, 0.0);
							limbAnimateToLimit(weaponarm, ANIMATE_ROLL, -0.1, 31 * PI / 16, false, 0.0);
						}

						if ( multiplayer != CLIENT )
						{
							// move the head back and forth.
							// keeps between PI and 0 (2PI) so we can lower the head at completion to 0 (2PI).
							if ( my->monsterAnimationLimbOvershoot >= ANIMATE_OVERSHOOT_TO_SETPOINT )
							{
								limbAnimateWithOvershoot(my, ANIMATE_PITCH, -0.1, 7 * PI / 4, -0.1, 15 * PI / 8, ANIMATE_DIR_POSITIVE);
							}
							else
							{
								// after 1 cycle is complete, reset the overshoot flag and repeat the animation.
								my->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_TO_SETPOINT;
							}
						}

						// animation takes roughly 2 seconds.
						if ( my->monsterAttackTime >= ANIMATE_DURATION_WINDUP * 10 / (monsterGlobalAnimationMultiplier / 10.0) )
						{
							weaponarm->skill[0] = rightbody->skill[0];
							weaponarm->pitch = rightbody->pitch;
							weaponarm->roll = -PI / 32;
							my->monsterArmbended = 0;
							my->monsterAttack = 0;
							Entity* leftarm = nullptr;
							node_t* leftarmNode = list_Node(&my->children, LIMB_HUMANOID_LEFTARM);
							if ( leftarmNode )
							{
								leftarm = (Entity*)leftarmNode->element;
								leftarm->roll = PI / 32;
							}
							else
							{
								return;
							}
							if ( multiplayer != CLIENT )
							{
								my->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_NONE;
							}
						}
						++my->monsterAttackTime; // manually increment timer
					}
					else
					{
						my->handleWeaponArmAttack(weaponarm);
						if ( my->monsterAttack != MONSTER_POSE_MELEE_WINDUP2 && my->monsterAttack != 2 )
						{
							// flare out the weapon arm to match neutral arm position. 
							// breaks the horizontal chop attack animation so we skip it.
							weaponarm->roll = -PI / 32;
						}
					}
				}
			}
			else if ( bodypart == LIMB_HUMANOID_CLOAK )
			{
				entity->pitch = entity->fskill[0];
			}

			my->humanoidAnimateWalk(entity, node, bodypart, INCUBUSWALKSPEED, dist, 0.4);

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
				entity->x -= .5 * cos(my->yaw);
				entity->y -= .5 * sin(my->yaw);
				entity->z += 2.5;
				break;
			// right leg
			case LIMB_HUMANOID_RIGHTLEG:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->shoes == nullptr )
					{
						entity->sprite = 450;
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
				entity->x += 1 * cos(my->yaw + PI / 2) - .75 * cos(my->yaw);
				entity->y += 1 * sin(my->yaw + PI / 2) - .75 * sin(my->yaw);
				entity->z += 5;
				if ( my->z >= 1.4 && my->z <= 1.6 )
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
						entity->sprite = 449;
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
				entity->x -= 1 * cos(my->yaw + PI / 2) + .75 * cos(my->yaw);
				entity->y -= 1 * sin(my->yaw + PI / 2) + .75 * sin(my->yaw);
				entity->z += 5;
				if ( my->z >= 1.4 && my->z <= 1.6 )
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
					if ( MONSTER_ARMBENDED || (weapon->flags[INVISIBLE] && my->monsterState != MONSTER_STATE_ATTACK) )
					{
						// if weapon invisible and I'm not attacking, relax arm.
						entity->focalx = limbs[INCUBUS][4][0] - 0.25; // 0
						entity->focaly = limbs[INCUBUS][4][1] - 0.25; // 0
						entity->focalz = limbs[INCUBUS][4][2]; // 2
						entity->sprite = 448;
						if ( my->monsterAttack == 0 )
						{
							entity->roll = -PI / 32;
						}
					}
					else
					{
						// else flex arm.
						entity->focalx = limbs[INCUBUS][4][0];
						entity->focaly = limbs[INCUBUS][4][1];
						entity->focalz = limbs[INCUBUS][4][2];
						entity->sprite = 595;
					}
				}
				entity->x += 2.5 * cos(my->yaw + PI / 2) - .20 * cos(my->yaw);
				entity->y += 2.5 * sin(my->yaw + PI / 2) - .20 * sin(my->yaw);
				entity->z += 0.5;
				entity->yaw += MONSTER_WEAPONYAW;
				if ( my->z >= 1.4 && my->z <= 1.6 )
				{
					entity->pitch = 0;
				}
				break;
			}
			// left arm
			case LIMB_HUMANOID_LEFTARM:
			{
				shieldarm = entity;
				node_t* shieldNode = list_Node(&my->children, 8);
				if ( shieldNode )
				{
					Entity* shield = (Entity*)shieldNode->element;
					if ( shield->flags[INVISIBLE] && (my->monsterState != MONSTER_STATE_ATTACK) )
					{
						// if weapon invisible and I'm not attacking, relax arm.
						entity->focalx = limbs[INCUBUS][5][0] - 0.25; // 0
						entity->focaly = limbs[INCUBUS][5][1] + 0.25; // 0
						entity->focalz = limbs[INCUBUS][5][2]; // 2
						entity->sprite = 447;
						if ( my->monsterAttack == 0 )
						{
							entity->roll = PI / 32;
						}
					}
					else
					{
						// else flex arm.
						entity->focalx = limbs[INCUBUS][5][0];
						entity->focaly = limbs[INCUBUS][5][1];
						entity->focalz = limbs[INCUBUS][5][2];
						entity->sprite = 594;
						if ( my->monsterSpecialState == INCUBUS_STEAL )
						{
							entity->yaw -= MONSTER_WEAPONYAW;
						}
					}
				}
				entity->x -= 2.5 * cos(my->yaw + PI / 2) + .20 * cos(my->yaw);
				entity->y -= 2.5 * sin(my->yaw + PI / 2) + .20 * sin(my->yaw);
				entity->z += 0.5;
				if ( my->z >= 1.4 && my->z <= 1.6 )
				{
					entity->pitch = 0;
				}
				if ( my->monsterDefend && my->monsterAttack == 0 )
				{
					MONSTER_SHIELDYAW = PI / 5;
				}
				else
				{
					MONSTER_SHIELDYAW = 0;
				}
				entity->yaw += MONSTER_SHIELDYAW;
				break;
			}
			// weapon
			case LIMB_HUMANOID_WEAPON:
			{
				if ( multiplayer != CLIENT )
				{
					if ( myStats->weapon == nullptr || myStats->EFFECTS[EFF_INVISIBLE] || wearingring ) //TODO: isInvisible()?
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
			}
			// shield
			case LIMB_HUMANOID_SHIELD:
			{
				if ( multiplayer != CLIENT )
				{
					if ( myStats->shield == nullptr )
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
				entity->yaw = shieldarm->yaw;
				entity->roll = 0;
				entity->pitch = 0;
				if ( entity->sprite == items[TOOL_TORCH].index )
				{
					entity2 = spawnFlame(entity, SPRITE_FLAME);
					entity2->x += 2 * cos(entity->yaw);
					entity2->y += 2 * sin(entity->yaw);
					entity2->z -= 2;
				}
				else if ( entity->sprite == items[TOOL_CRYSTALSHARD].index )
				{
					entity2 = spawnFlame(entity, SPRITE_CRYSTALFLAME);
					entity2->x += 2 * cos(entity->yaw);
					entity2->y += 2 * sin(entity->yaw);
					entity2->z -= 2;
				}
				else if ( entity->sprite == items[TOOL_LANTERN].index )
				{
					entity->z += 2;
					entity2 = spawnFlame(entity, SPRITE_FLAME);
					entity2->x += 2 * cos(entity->yaw);
					entity2->y += 2 * sin(entity->yaw);
					entity2->z += 1;
				}
				if ( MONSTER_SHIELDYAW > PI / 32 )
				{
					if ( entity->sprite != items[TOOL_TORCH].index && entity->sprite != items[TOOL_LANTERN].index && entity->sprite != items[TOOL_CRYSTALSHARD].index )
					{
						// shield, so rotate a little.
						entity->roll += PI / 64;
					}
					else
					{
						entity->x += 0.25 * cos(my->yaw);
						entity->y += 0.25 * sin(my->yaw);
						entity->pitch += PI / 16;
						if ( entity2 )
						{
							entity2->x += 0.75 * cos(shieldarm->yaw);
							entity2->y += 0.75 * sin(shieldarm->yaw);
						}
					}
				}
				break;
			}
		}
	}
	// rotate shield a bit
	node_t* shieldNode = list_Node(&my->children, LIMB_HUMANOID_SHIELD);
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

void Entity::incubusChooseWeapon(const Entity* target, double dist)
{
	if ( monsterSpecialState != 0 )
	{
		//Holding a weapon assigned from the special attack. Don't switch weapons.
		if ( monsterSpecialState == INCUBUS_TELEPORT_STEAL && monsterSpecialTimer == 0 )
		{
			// handle steal weapon random teleportation
			incubusTeleportRandom();
			monsterSpecialState = 0;
			serverUpdateEntitySkill(this, 33); // for clients to keep track of animation
			attack(MONSTER_POSE_INCUBUS_TELEPORT, 0, nullptr);
		}
		else if ( monsterSpecialState == INCUBUS_TELEPORT && monsterSpecialTimer == 0 )
		{
			// handle idle teleporting to target
			monsterSpecialState = 0;
			serverUpdateEntitySkill(this, 33); // for clients to keep track of animation
		}
		return;
	}

	Stat *myStats = getStats();
	if ( !myStats )
	{
		return;
	}

	int specialRoll = -1;
	int bonusFromHP = 0;

	if ( ticks % 10 == 0 && monsterSpecialState != INCUBUS_TELEPORT_STEAL )
	{
		// teleport to target, higher chance at greater distance or lower HP
		specialRoll = rand() % 50;
		if ( specialRoll < (1 + (dist > 80 ? 4 : 0) + (myStats->HP <= myStats->MAXHP * 0.8 ? 4 : 0)) )
		{
			monsterSpecialState = INCUBUS_TELEPORT;
			incubusTeleportToTarget(target);
			return;
		}
	}

	if ( monsterSpecialTimer == 0 && (ticks % 10 == 0) && monsterAttack == 0 )
	{
		Stat* targetStats = target->getStats();
		if ( !targetStats )
		{
			return;
		}

		bool tryCharm = true;
		bool trySteal = true;
		if ( targetStats->EFFECTS[EFF_PACIFY] )
		{
			tryCharm = false;
		}
		if ( !targetStats->weapon )
		{
			trySteal = false;
		}

		if ( trySteal || tryCharm )
		{
			if ( myStats->HP <= myStats->MAXHP * 0.8 )
			{
				bonusFromHP += 1; // +2.5% chance if on low health
			}
			if ( myStats->HP <= myStats->MAXHP * 0.4 )
			{
				bonusFromHP += 1; // +extra 2.5% chance if on lower health
			}

			int requiredRoll = (1 + bonusFromHP + (targetStats->EFFECTS[EFF_CONFUSED] ? 4 : 0)
				+ (targetStats->EFFECTS[EFF_DRUNK] ? 2 : 0)
				+ (targetStats->EFFECTS[EFF_PACIFY] ? 2 : 0)); // +2.5% base, + extra if target is inebriated

			if ( trySteal && tryCharm )
			{
				if ( rand() % 8 == 0 )
				{
					trySteal = false; // try charm 12.5% of the time.
				}
				else
				{
					tryCharm = false;
				}
			}

			if ( trySteal )
			{
				// try to steal weapon if target is holding.
				// occurs less often against fellow monsters.
				specialRoll = rand() % (40 + 40 * (target->behavior == &actMonster));
			}
			else if ( tryCharm )
			{
				specialRoll = rand() % 40;
			}

			if ( trySteal )
			{
				requiredRoll += (myStats->weapon == nullptr ? 3 : 0); // bonus if no weapon held
			}

			if ( specialRoll < requiredRoll ) 
			{
				node_t* node = nullptr;
				if ( trySteal )
				{
					node = itemNodeInInventory(myStats, SPELLBOOK_STEAL_WEAPON, static_cast<Category>(-1));
					if ( node != nullptr )
					{
						swapMonsterWeaponWithInventoryItem(this, myStats, node, true, true);
						monsterSpecialState = INCUBUS_STEAL;
						serverUpdateEntitySkill(this, 33); // for clients to keep track of animation
						return;
					}
				}
				else if ( tryCharm )
				{
					node = itemNodeInInventory(myStats, SPELLBOOK_CHARM_MONSTER, static_cast<Category>(-1));
					if ( node != nullptr )
					{
						swapMonsterWeaponWithInventoryItem(this, myStats, node, true, true);
						monsterSpecialState = INCUBUS_CHARM;
						serverUpdateEntitySkill(this, 33); // for clients to keep track of animation
						return;
					}
				}
			}
		}
		
		// try new roll for alternate potion throw special.
		// occurs less often against fellow monsters.
		specialRoll = rand() % (30 + 30 * (target->behavior == &actMonster));
		if ( specialRoll < (2 + bonusFromHP) ) // +5% base
		{
			node_t* node = nullptr;
			node = itemNodeInInventory(myStats, static_cast<ItemType>(-1), POTION);
			if ( node != nullptr )
			{
				swapMonsterWeaponWithInventoryItem(this, myStats, node, true, true);
				monsterSpecialState = INCUBUS_CONFUSION;
				return;
			}
		}
	}

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

	//Switch to a thrown weapon or a ranged weapon.
	if ( !myStats->weapon || isMeleeWeapon(*myStats->weapon) )
	{
		node_t *weaponNode = getRangedWeaponItemNodeInInventory(myStats, true);
		if ( !weaponNode )
		{
			return; //Nothing available
		}
		bool swapped = swapMonsterWeaponWithInventoryItem(this, myStats, weaponNode, false, false);
		return;
	}
	return;
}

void Entity::incubusTeleportToTarget(const Entity* target)
{
	Entity* spellTimer = createParticleTimer(this, 40, 593);
	spellTimer->particleTimerEndAction = PARTICLE_EFFECT_INCUBUS_TELEPORT_TARGET; // teleport behavior of timer.
	spellTimer->particleTimerEndSprite = 593; // sprite to use for end of timer function.
	spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_SHOOT_PARTICLES;
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

void Entity::incubusTeleportRandom()
{
	Entity* spellTimer = createParticleTimer(this, 80, 593);
	spellTimer->particleTimerEndAction = PARTICLE_EFFECT_INCUBUS_TELEPORT_STEAL; // teleport behavior of timer.
	spellTimer->particleTimerEndSprite = 593; // sprite to use for end of timer function.
	spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_SHOOT_PARTICLES;
	spellTimer->particleTimerCountdownSprite = 593;
	spellTimer->particleTimerPreDelay = 40;
	if ( multiplayer == SERVER )
	{
		serverSpawnMiscParticles(this, PARTICLE_EFFECT_INCUBUS_TELEPORT_STEAL, 593);
	}
}
