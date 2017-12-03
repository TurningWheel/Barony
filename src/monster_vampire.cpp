/*-------------------------------------------------------------------------------

BARONY
File: monster_vampire.cpp
Desc: implements all of the vampire monster's code

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
#include "classdescriptions.hpp"
#include "player.hpp"
#include "magic/magic.hpp"

void initVampire(Entity* my, Stat* myStats)
{
	int c;
	node_t* node;

	//Sprite 437 = Vampire head model
	my->initMonster(437);

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 256;
		MONSTER_SPOTVAR = 1;
		MONSTER_IDLESND = 254;
		MONSTER_IDLEVAR = 2;
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
			if ( !strncmp(myStats->name, "young vampire", strlen("young vampire")) )
			{
				lesserMonster = true;
				myStats->HP = 150;
				myStats->MAXHP = myStats->HP;
				myStats->RANDOM_MAXHP = 0;
				myStats->RANDOM_HP = myStats->RANDOM_MAXHP;
				myStats->OLDHP = myStats->HP;
				myStats->STR = 15;
				myStats->RANDOM_STR = 0;
				myStats->DEX = 8;
				myStats->RANDOM_DEX = 0;
				myStats->CON = -10;
				myStats->RANDOM_CON = 0;
				myStats->INT = 15;
				myStats->RANDOM_INT = 0;
				myStats->PER = 5;
				myStats->RANDOM_PER = 0;
				myStats->CHR = -3;
				myStats->RANDOM_CHR = 0;
				myStats->EXP = 0;
				myStats->LVL = 18;
				myStats->GOLD = 50 + rand() % 50;
				myStats->RANDOM_GOLD = 0;
				for ( c = 0; c < 4; ++c )
				{
					if ( rand() % 2 == 0 )
					{
						Entity* entity = summonMonster(GHOUL, my->x, my->y);
						if ( entity )
						{
							entity->parent = my->getUID();
							Stat* followerStats = entity->getStats();
							if ( followerStats )
							{
								strcpy(followerStats->name, "enslaved ghoul");
							}
						}
					}
				}
			}

			// apply random stat increases if set in stat_shared.cpp or editor
			setRandomMonsterStats(myStats);

			// generate 6 items max, less if there are any forced items from boss variants
			int customItemsToGenerate = ITEM_CUSTOM_SLOT_LIMIT;

			// boss variants
			//if ( rand() % 50 || my->flags[USERFLAG2] )
			//{
			//	if ( strncmp(map.name, "Underworld", 10) )
			//	{
			//		switch ( rand() % 10 )
			//		{
			//			case 0:
			//			case 1:
			//				//myStats->weapon = newItem(BRONZE_AXE, WORN, -1 + rand() % 2, 1, rand(), false, nullptr);
			//				break;
			//			case 2:
			//			case 3:
			//				//myStats->weapon = newItem(BRONZE_SWORD, WORN, -1 + rand() % 2, 1, rand(), false, nullptr);
			//				break;
			//			case 4:
			//			case 5:
			//				//myStats->weapon = newItem(IRON_SPEAR, WORN, -1 + rand() % 2, 1, rand(), false, nullptr);
			//				break;
			//			case 6:
			//			case 7:
			//				//myStats->weapon = newItem(IRON_AXE, WORN, -1 + rand() % 2, 1, rand(), false, nullptr);
			//				break;
			//			case 8:
			//			case 9:
			//				//myStats->weapon = newItem(IRON_SWORD, WORN, -1 + rand() % 2, 1, rand(), false, nullptr);
			//				break;
			//		}
			//	}
			//}
			//else
			//{
			//	/*myStats->HP = 100;
			//	myStats->MAXHP = 100;
			//	strcpy(myStats->name, "Funny Bones");
			//	myStats->weapon = newItem(ARTIFACT_AXE, EXCELLENT, 1, 1, rand(), true, nullptr);
			//	myStats->cloak = newItem(CLOAK_PROTECTION, WORN, 0, 1, 2, true, nullptr);*/
			//}
			
			// random effects
			//my->setEffect(EFF_MAGICRESIST, true, -1, true); //-1 duration, never expires.

			// generates equipment and weapons if available from editor
			createMonsterEquipment(myStats);

			// create any custom inventory items from editor if available
			createCustomInventory(myStats, customItemsToGenerate);

			// count if any custom inventory items from editor
			int customItems = countCustomItems(myStats); //max limit of 6 custom items per entity.

			// count any inventory items set to default in edtior
			int defaultItems = countDefaultItems(myStats);

			newItem(SPELLBOOK_VAMPIRIC_AURA, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, &myStats->inventory);
			newItem(SPELLBOOK_DRAIN_SOUL, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, &myStats->inventory);

			// generate the default inventory items for the monster, provided the editor sprite allowed enough default slots
			switch ( defaultItems )
			{
				case 6:
				case 5:
				case 4:
				case 3:
				case 2:
					if ( rand() % 4 == 0 ) // 1 in 4
					{
						newItem(MAGICSTAFF_BLEED, static_cast<Status>(DECREPIT + rand() % 2), -1 + rand() % 3, 1, rand(), false, &myStats->inventory);
					}
				case 1:
					if ( rand() % 10 == 0 ) // 1 in 10
					{
						newItem(VAMPIRE_DOUBLET, static_cast<Status>(WORN + rand() % 2), -1 + rand() % 3, 1, rand(), false, &myStats->inventory);
					}
					break;
				default:
					break;
			}

			//give weapon
			if ( myStats->weapon == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1 )
			{
				switch ( rand() % 10 )
				{
					case 0:
					case 1:
					case 2:
					case 3:
						//myStats->weapon = newItem(SHORTBOW, WORN, -1 + rand() % 2, 1, rand(), false, nullptr);
						break;
					case 4:
					case 5:
					case 6:
					case 7:
						//myStats->weapon = newItem(CROSSBOW, WORN, -1 + rand() % 2, 1, rand(), false, nullptr);
						break;
					case 8:
					case 9:
						//myStats->weapon = newItem(MAGICSTAFF_COLD, EXCELLENT, -1 + rand() % 2, 1, rand(), false, nullptr);
						break;
				}
			}

			//give helmet
			if ( myStats->helmet == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_HELM] == 1 )
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
						//myStats->helmet = newItem(LEATHER_HELM, DECREPIT, -1 + rand() % 2, 1, 0, false, nullptr);
						break;
					case 6:
					case 7:
					case 8:
					case 9:
						//myStats->helmet = newItem(IRON_HELM, DECREPIT, -1 + rand() % 2, 1, 0, false, nullptr);
						break;
				}
			}

			//give shield
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
						break;
					case 6:
					case 7:
						//myStats->shield = newItem(WOODEN_SHIELD, DECREPIT, -1 + rand() % 2, 1, rand(), false, nullptr);
						break;
					case 8:
						//myStats->shield = newItem(BRONZE_SHIELD, DECREPIT, -1 + rand() % 2, 1, rand(), false, nullptr);
						break;
					case 9:
						//myStats->shield = newItem(IRON_SHIELD, DECREPIT, -1 + rand() % 2, 1, rand(), false, nullptr);
						break;
				}
			}
		}
	}

	// torso
	Entity* entity = newEntity(438, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[VAMPIRE][1][0]; // 0
	entity->focaly = limbs[VAMPIRE][1][1]; // 0
	entity->focalz = limbs[VAMPIRE][1][2]; // 0
	entity->behavior = &actVampireLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// right leg
	entity = newEntity(444, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[VAMPIRE][2][0]; // 0
	entity->focaly = limbs[VAMPIRE][2][1]; // 0
	entity->focalz = limbs[VAMPIRE][2][2]; // 2
	entity->behavior = &actVampireLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// left leg
	entity = newEntity(443, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[VAMPIRE][3][0]; // 0
	entity->focaly = limbs[VAMPIRE][3][1]; // 0
	entity->focalz = limbs[VAMPIRE][3][2]; // 2
	entity->behavior = &actVampireLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// right arm
	entity = newEntity(440, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[VAMPIRE][4][0]; // 0
	entity->focaly = limbs[VAMPIRE][4][1]; // 0
	entity->focalz = limbs[VAMPIRE][4][2]; // 1.5
	entity->behavior = &actVampireLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// left arm
	entity = newEntity(439, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[VAMPIRE][5][0]; // 0
	entity->focaly = limbs[VAMPIRE][5][1]; // 0
	entity->focalz = limbs[VAMPIRE][5][2]; // 1.5
	entity->behavior = &actVampireLimb;
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
	entity->focalx = limbs[VAMPIRE][6][0]; // 1.5
	entity->focaly = limbs[VAMPIRE][6][1]; // 0
	entity->focalz = limbs[VAMPIRE][6][2]; // -.5
	entity->behavior = &actVampireLimb;
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
	entity->focalx = limbs[VAMPIRE][7][0]; // 2
	entity->focaly = limbs[VAMPIRE][7][1]; // 0
	entity->focalz = limbs[VAMPIRE][7][2]; // 0
	entity->behavior = &actVampireLimb;
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
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[VAMPIRE][8][0]; // 0
	entity->focaly = limbs[VAMPIRE][8][1]; // 0
	entity->focalz = limbs[VAMPIRE][8][2]; // 4
	entity->behavior = &actVampireLimb;
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
	entity->focalx = limbs[VAMPIRE][9][0]; // 0
	entity->focaly = limbs[VAMPIRE][9][1]; // 0
	entity->focalz = limbs[VAMPIRE][9][2]; // -1.75
	entity->behavior = &actVampireLimb;
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
	entity->scalex = .99;
	entity->scaley = .99;
	entity->scalez = .99;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[VAMPIRE][10][0]; // 0
	entity->focaly = limbs[VAMPIRE][10][1]; // 0
	entity->focalz = limbs[VAMPIRE][10][2]; // .5
	entity->behavior = &actVampireLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	if ( multiplayer == CLIENT )
	{
		my->sprite = 437; // vampire head model
		return;
	}
}

void actVampireLimb(Entity* my)
{
	my->actMonsterLimb(true);
}

void vampireDie(Entity* my)
{
	int c;
	for ( c = 0; c < 5; c++ )
	{
		Entity* gib = spawnGib(my);
		serverSpawnGibForClient(gib);
	}

	my->spawnBlood();

	playSoundEntity(my, 253, 128);

	my->removeMonsterDeathNodes();

	list_RemoveNode(my->mynode);
	return;
}

#define VAMPIREWALKSPEED .12

void vampireMoveBodyparts(Entity* my, Stat* myStats, double dist)
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
		if ( myStats->EFFECTS[EFF_INVISIBLE] == true || wearingring == true )
		{
			my->flags[INVISIBLE] = true;
			my->flags[BLOCKSIGHT] = false;
			bodypart = 0;
			for ( node = my->children.first; node != nullptr; node = node->next )
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
			for ( node = my->children.first; node != nullptr; node = node->next )
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
			my->pitch = PI / 4;
		}
		else
		{
			my->z = -1;
			if ( my->monsterAttack == 0 )
			{
				my->pitch = 0;
			}
		}

		// levitation
		bool levitating = isLevitating(myStats);
		if ( levitating )
		{
			my->z -= 1; // floating
		}
	}

	// move bodyparts
	for ( bodypart = 0, node = my->children.first; node != nullptr; node = node->next, bodypart++ )
	{
		if ( bodypart < LIMB_HUMANOID_TORSO )
		{
			// post-swing head animation. client doesn't need to adjust the entity pitch, server will handle.
			if ( multiplayer != CLIENT && bodypart == 1 )
			{
				if ( my->monsterAttack != MONSTER_POSE_VAMPIRE_DRAIN 
					&& my->monsterAttack != MONSTER_POSE_VAMPIRE_AURA_CHARGE )
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
			if ( bodypart == LIMB_HUMANOID_LEFTARM 
				&& ((my->monsterSpecialState == VAMPIRE_CAST_DRAIN || my->monsterSpecialState == VAMPIRE_CAST_AURA )
				&& my->monsterAttack != 0) )
			{
				// leftarm follows the right arm during special attack
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
				if ( bodypart == LIMB_HUMANOID_LEFTARM )
				{
					// for clients to adjust roll in case the above roll/pitch code was not cleared.
					entity->roll = 0;
				}
				my->humanoidAnimateWalk(entity, node, bodypart, VAMPIREWALKSPEED, dist, 0.4);
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
					Entity* leftarm = nullptr;
					// set leftarm
					node_t* leftarmNode = list_Node(&my->children, LIMB_HUMANOID_LEFTARM);
					if ( leftarmNode )
					{
						leftarm = (Entity*)leftarmNode->element;
					}
					else
					{
						return;
					}

					if ( my->monsterAttack == MONSTER_POSE_VAMPIRE_DRAIN )
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
						}

						limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.25, 5 * PI / 4, false, 0.0);
						if ( multiplayer != CLIENT )
						{
							// move the head and weapon yaw
							limbAnimateToLimit(my, ANIMATE_PITCH, -0.1, 14 * PI / 8, true, 0.1);
							limbAnimateToLimit(my, ANIMATE_WEAPON_YAW, 0.25, 2 * PI / 8, false, 0.0);
						}

						if ( my->monsterAttackTime >= 2 * ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
						{
							if ( multiplayer != CLIENT )
							{
								// throw the spell
								my->attack(MONSTER_POSE_MELEE_WINDUP1, 0, nullptr);
							}
						}
						++my->monsterAttackTime;
					}
					else if ( my->monsterAttack == MONSTER_POSE_VAMPIRE_AURA_CHARGE )
					{

						if ( my->monsterAttackTime == 0 )
						{
							// init rotations
							weaponarm->pitch = 6 * PI / 4;
							leftarm->pitch = 6 * PI / 4;
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
								myStats->EFFECTS[EFF_PARALYZED] = true;
								myStats->EFFECTS_TIMERS[EFF_PARALYZED] = 50;
							}
						}

						limbAnimateToLimit(weaponarm, ANIMATE_PITCH, 0.05, 2 * PI / 8, true, 0.05);
						limbAnimateToLimit(weaponarm, ANIMATE_ROLL, -0.1, 15 * PI / 8, false, 0.0);
						if ( multiplayer != CLIENT )
						{
							// move the head and weapon yaw
							limbAnimateToLimit(my, ANIMATE_PITCH, -0.1, 14 * PI / 8, true, 0.1);
						}

						if ( my->monsterAttackTime >= 6 * ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
						{
							// reset the limbs
							weaponarm->skill[0] = rightbody->skill[0];
							my->monsterWeaponYaw = 0;
							weaponarm->pitch = rightbody->pitch;
							weaponarm->roll = 0;
							my->monsterArmbended = 0;
							leftarm->roll = 0;
							leftarm->pitch = 0;
							if ( multiplayer != CLIENT )
							{
								my->attack(MONSTER_POSE_VAMPIRE_AURA_CAST, 0, nullptr);
							}
						}
						++my->monsterAttackTime;
					}
					else if ( my->monsterAttack == MONSTER_POSE_VAMPIRE_AURA_CAST )
					{
						weaponarm->roll = 0;
						leftarm->roll = 0;
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

			my->humanoidAnimateWalk(entity, node, bodypart, VAMPIREWALKSPEED, dist, 0.4);

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
					if ( myStats->breastplate == nullptr )
					{
						entity->sprite = 438;
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
				entity->z += 2.5;
				break;
				// right leg
			case LIMB_HUMANOID_RIGHTLEG:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->shoes == nullptr )
					{
						entity->sprite = 444;
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
						entity->sprite = 443;
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
				if ( multiplayer != CLIENT )
				{
					if ( myStats->gloves == nullptr )
					{
						entity->sprite = 440;
					}
					else
					{
						if ( setGloveSprite(myStats, entity, SPRITE_GLOVE_RIGHT_OFFSET) != 0 )
						{
							// successfully set sprite for the human model
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
						if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
						{
							serverUpdateEntityBodypart(my, bodypart);
						}
					}
				}

				if ( multiplayer == CLIENT )
				{
					if ( entity->skill[7] == 0 )
					{
						entity->skill[7] = entity->sprite;
					}
					entity->sprite = entity->skill[7];
				}

				node_t* tempNode = list_Node(&my->children, LIMB_HUMANOID_WEAPON);
				if ( tempNode )
				{
					Entity* weapon = (Entity*)tempNode->element;
					if ( MONSTER_ARMBENDED || (weapon->flags[INVISIBLE] && my->monsterState != MONSTER_STATE_ATTACK) )
					{
						// if weapon invisible and I'm not attacking, relax arm.
						entity->focalx = limbs[VAMPIRE][4][0]; // 0
						entity->focaly = limbs[VAMPIRE][4][1]; // 0
						entity->focalz = limbs[VAMPIRE][4][2]; // 1.5
					}
					else
					{
						// else flex arm.
						entity->focalx = limbs[VAMPIRE][4][0] + 0.75;
						entity->focaly = limbs[VAMPIRE][4][1];
						entity->focalz = limbs[VAMPIRE][4][2] - 0.75;
						entity->sprite += 2;
					}
				}
				entity->x += 2.5 * cos(my->yaw + PI / 2) - .20 * cos(my->yaw);
				entity->y += 2.5 * sin(my->yaw + PI / 2) - .20 * sin(my->yaw);
				entity->z += 1.5;
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
				if ( multiplayer != CLIENT )
				{
					if ( myStats->gloves == nullptr )
					{
						entity->sprite = 439;
					}
					else
					{
						if ( setGloveSprite(myStats, entity, SPRITE_GLOVE_LEFT_OFFSET) != 0 )
						{
							// successfully set sprite for the human model
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
						if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
						{
							serverUpdateEntityBodypart(my, bodypart);
						}
					}
				}

				if ( multiplayer == CLIENT )
				{
					if ( entity->skill[7] == 0 )
					{
						entity->skill[7] = entity->sprite;
					}
					entity->sprite = entity->skill[7];
				}

				node_t* tempNode = list_Node(&my->children, LIMB_HUMANOID_SHIELD);
				if ( tempNode )
				{
					Entity* shield = (Entity*)tempNode->element;
					if ( shield->flags[INVISIBLE] && (my->monsterState != MONSTER_STATE_ATTACK) )
					{
						// if shield invisible and I'm not attacking, relax arm.
						entity->focalx = limbs[VAMPIRE][5][0]; // 0
						entity->focaly = limbs[VAMPIRE][5][1]; // 0
						entity->focalz = limbs[VAMPIRE][5][2]; // 1.5
					}
					else
					{
						// else flex arm.
						entity->focalx = limbs[VAMPIRE][5][0] + 0.75;
						entity->focaly = limbs[VAMPIRE][5][1];
						entity->focalz = limbs[VAMPIRE][5][2] - 0.75;
						entity->sprite += 2;
						if ( my->monsterSpecialState == VAMPIRE_CAST_DRAIN || my->monsterSpecialState == VAMPIRE_CAST_AURA )
						{
							entity->yaw -= MONSTER_WEAPONYAW;
						}
					}
				}
				entity->x -= 2.5 * cos(my->yaw + PI / 2) + .20 * cos(my->yaw);
				entity->y -= 2.5 * sin(my->yaw + PI / 2) + .20 * sin(my->yaw);
				entity->z += 1.5;
				if ( my->z >= 1.4 && my->z <= 1.6 )
				{
					entity->pitch = 0;
				}
				break;
			}
			// weapon
			case LIMB_HUMANOID_WEAPON:
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
				// shield
			case LIMB_HUMANOID_SHIELD:
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
					if ( myStats->cloak == nullptr || myStats->EFFECTS[EFF_INVISIBLE] || wearingring ) //TODO: isInvisible()?
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
				entity->focalx = limbs[VAMPIRE][9][0]; // 0
				entity->focaly = limbs[VAMPIRE][9][1]; // 0
				entity->focalz = limbs[VAMPIRE][9][2]; // -1.75
				entity->pitch = my->pitch;
				entity->roll = 0;
				if ( multiplayer != CLIENT )
				{
					entity->sprite = itemModel(myStats->helmet);
					if ( myStats->helmet == nullptr || myStats->EFFECTS[EFF_INVISIBLE] || wearingring ) //TODO: isInvisible()?
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
				entity->focalx = limbs[VAMPIRE][10][0]; // 0
				entity->focaly = limbs[VAMPIRE][10][1]; // 0
				entity->focalz = limbs[VAMPIRE][10][2]; // .5
				entity->pitch = my->pitch;
				entity->roll = PI / 2;
				if ( multiplayer != CLIENT )
				{
					if ( myStats->mask == nullptr || myStats->EFFECTS[EFF_INVISIBLE] || wearingring ) //TODO: isInvisible()?
					{
						entity->flags[INVISIBLE] = true;
					}
					else
					{
						entity->flags[INVISIBLE] = false;
					}
					if ( myStats->mask != nullptr )
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
					entity->focalx = limbs[VAMPIRE][10][0] + .35; // .35
					entity->focaly = limbs[VAMPIRE][10][1] - 2; // -2
					entity->focalz = limbs[VAMPIRE][10][2]; // .5
				}
				else
				{
					entity->focalx = limbs[VAMPIRE][10][0] + .25; // .25
					entity->focaly = limbs[VAMPIRE][10][1] - 2.25; // -2.25
					entity->focalz = limbs[VAMPIRE][10][2]; // .5
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

void Entity::vampireChooseWeapon(const Entity* target, double dist)
{
	if ( monsterSpecialState != 0 )
	{
		return;
	}

	Stat *myStats = getStats();
	if ( !myStats )
	{
		return;
	}

	int specialRoll = -1;
	int bonusFromHP = 0;

	if ( monsterSpecialTimer == 0 && (ticks % 10 == 0) && monsterAttack == 0 )
	{
		Stat* targetStats = target->getStats();
		if ( !targetStats )
		{
			return;
		}

		// occurs less often against fellow monsters.
		specialRoll = rand() % (20 + 50 * (target->behavior == &actMonster));
		if ( myStats->HP <= myStats->MAXHP * 0.8 )
		{
			bonusFromHP += 1; // +5% chance if on low health
		}
		if ( myStats->HP <= myStats->MAXHP * 0.4 )
		{
			bonusFromHP += 3; // +extra 15% chance if on lower health
		}

		int requiredRoll = (1 + bonusFromHP);
		if ( targetStats->type == HUMAN )
		{
			requiredRoll++; // tasty human
		}
		if ( targetStats->HUNGER < 500 )
		{
			requiredRoll++; // tasty weak human
		}

		// check the roll
		if ( specialRoll < requiredRoll )
		{
			node_t* node = nullptr;
			bool chooseAura = false;
			if ( !myStats->EFFECTS[EFF_VAMPIRICAURA] )
			{
				if ( myStats->HP <= myStats->MAXHP * 0.4)
				{
					if ( rand() % 4 > 0 )
					{
						chooseAura = true; // 75% chance if low HP
					}
				}
				if ( !chooseAura )
				{
					if ( rand() % 5 == 0 )
					{
						chooseAura = true; // 20% chance base
					}
				}
			}
			
			if ( chooseAura )
			{
				node = itemNodeInInventory(myStats, SPELLBOOK_VAMPIRIC_AURA, static_cast<Category>(-1));
				if ( node != nullptr )
				{
					swapMonsterWeaponWithInventoryItem(this, myStats, node, true, true);
					monsterSpecialState = VAMPIRE_CAST_AURA;
					serverUpdateEntitySkill(this, 33); // for clients to keep track of animation
					monsterHitTime = HITRATE * 2; // force immediate attack
					return;
				}
			}
			else
			{
				node = itemNodeInInventory(myStats, SPELLBOOK_DRAIN_SOUL, static_cast<Category>(-1));
				if ( node != nullptr )
				{
					swapMonsterWeaponWithInventoryItem(this, myStats, node, true, true);
					monsterSpecialState = VAMPIRE_CAST_DRAIN;
					serverUpdateEntitySkill(this, 33); // for clients to keep track of animation
					monsterHitTime = HITRATE * 2; // force immediate attack
					return;
				}
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