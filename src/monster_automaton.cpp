/*-------------------------------------------------------------------------------

	BARONY
	File: monster_automaton.cpp
	Desc: implements all of the automaton monster's code

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

void initAutomaton(Entity* my, Stat* myStats)
{
	node_t* node;

	 //Sprite 467 = Automaton head model
	my->initMonster(467);

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 263;
		MONSTER_SPOTVAR = 3;
		MONSTER_IDLESND = 257;
		MONSTER_IDLEVAR = 2;
	}
	if ( multiplayer != CLIENT && !MONSTER_INIT )
	{
		if ( myStats != NULL )
		{
			if ( !myStats->leader_uid )
			{
				myStats->leader_uid = 0;
			}
			bool lesserMonster = false;
			bool greaterMonster = false;
			if ( !strncmp(myStats->name, "damaged automaton", strlen("damaged automaton")) )
			{
				lesserMonster = true;
				myStats->HP = 60;
				myStats->MAXHP = 115;
				myStats->RANDOM_MAXHP = 0;
				myStats->RANDOM_HP = 30;
				myStats->OLDHP = myStats->HP;
				myStats->STR = 13;
				myStats->DEX = 4;
				myStats->CON = 8;
				myStats->INT = -1;
				myStats->PER = 10;
				myStats->CHR = -3;
				myStats->EXP = 0;
				myStats->LVL = 16;
			}
			else if ( !strncmp(myStats->name, "corrupted automaton", strlen("corrupted automaton")) )
			{
				greaterMonster = true;
				myStats->HP = 150;
				myStats->MAXHP = 150;
				myStats->RANDOM_MAXHP = 0;
				myStats->RANDOM_HP = 0;
				myStats->OLDHP = myStats->HP;
				myStats->STR = 35;
				myStats->DEX = 13;
				myStats->CON = 8;
				myStats->INT = 10;
				myStats->PER = 25;
				myStats->CHR = -3;
				myStats->LVL = 30;
				myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] = 1;
				myStats->EDITOR_ITEMS[ITEM_SLOT_SHIELD] = 1;
				myStats->EDITOR_ITEMS[ITEM_SLOT_ARMOR] = 1;
				myStats->EDITOR_ITEMS[ITEM_SLOT_BOOTS] = 1;
				myStats->EDITOR_ITEMS[ITEM_SLOT_CLOAK] = 1;
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
			//				//myStats->weapon = newItem(BRONZE_AXE, WORN, -1 + rand() % 2, 1, rand(), false, NULL);
			//				break;
			//			case 2:
			//			case 3:
			//				//myStats->weapon = newItem(BRONZE_SWORD, WORN, -1 + rand() % 2, 1, rand(), false, NULL);
			//				break;
			//			case 4:
			//			case 5:
			//				//myStats->weapon = newItem(IRON_SPEAR, WORN, -1 + rand() % 2, 1, rand(), false, NULL);
			//				break;
			//			case 6:
			//			case 7:
			//				//myStats->weapon = newItem(IRON_AXE, WORN, -1 + rand() % 2, 1, rand(), false, NULL);
			//				break;
			//			case 8:
			//			case 9:
			//				//myStats->weapon = newItem(IRON_SWORD, WORN, -1 + rand() % 2, 1, rand(), false, NULL);
			//				break;
			//		}
			//	}
			//}
			//else
			//{
			//	myStats->HP = 100;
			//	myStats->MAXHP = 100;
			//	strcpy(myStats->name, "Funny Bones");
			//	myStats->weapon = newItem(ARTIFACT_AXE, EXCELLENT, 1, 1, rand(), true, NULL);
			//	myStats->cloak = newItem(CLOAK_PROTECTION, WORN, 0, 1, 2, true, NULL);
			//}

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
				if ( greaterMonster )
				{
					switch ( rand() % 4 )
					{
						case 0:
							myStats->weapon = newItem(MAGICSTAFF_LIGHTNING, EXCELLENT, -1 + rand() % 2, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, NULL);
							break;
						case 1:
							myStats->weapon = newItem(CRYSTAL_SPEAR, EXCELLENT, -1 + rand() % 2, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, NULL);
							break;
						case 2:
							myStats->weapon = newItem(SHORTBOW, EXCELLENT, -1 + rand() % 2, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, NULL);
							break;
						case 3:
							myStats->weapon = newItem(CROSSBOW, EXCELLENT, -1 + rand() % 2, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, NULL);
							break;
						default:
							break;
					}
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
						//myStats->helmet = newItem(LEATHER_HELM, DECREPIT, -1 + rand() % 2, 1, 0, false, NULL);
						break;
					case 6:
					case 7:
					case 8:
					case 9:
						//myStats->helmet = newItem(IRON_HELM, DECREPIT, -1 + rand() % 2, 1, 0, false, NULL);
						break;
				}
			}

			//give shield
			if ( myStats->shield == NULL && myStats->EDITOR_ITEMS[ITEM_SLOT_SHIELD] == 1 )
			{
				if ( greaterMonster )
				{
					switch ( rand() % 4 )
					{
						case 0:
							myStats->shield = newItem(CRYSTAL_SHIELD, EXCELLENT, -1 + rand() % 2, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, NULL);
							break;
						case 1:
							myStats->shield = newItem(STEEL_SHIELD_RESISTANCE, EXCELLENT, -1 + rand() % 2, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, NULL);
							break;
						case 2:
							myStats->shield = newItem(STEEL_SHIELD, EXCELLENT, -1 + rand() % 2, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, NULL);
							break;
						case 3:
							myStats->shield = newItem(MIRROR_SHIELD, EXCELLENT, -1 + rand() % 2, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, NULL);
							break;
						default:
							break;
					}
				}
			}

			//give boots
			if ( myStats->shoes == NULL && myStats->EDITOR_ITEMS[ITEM_SLOT_BOOTS] == 1 )
			{
				if ( greaterMonster )
				{
					switch ( rand() % 4 )
					{
						case 0:
						case 1:
						case 2:
						case 3:
							myStats->shoes = newItem(STEEL_BOOTS_LEVITATION, EXCELLENT, -1 + rand() % 2, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, NULL);
							break;
						default:
							break;
					}
				}
			}

			//give cloak
			if ( myStats->cloak == NULL && myStats->EDITOR_ITEMS[ITEM_SLOT_CLOAK] == 1 )
			{
				if ( greaterMonster )
				{
					switch ( rand() % 4 )
					{
						case 0:
							myStats->cloak = newItem(CLOAK_MAGICREFLECTION, EXCELLENT, -1 + rand() % 2, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, NULL);
							break;
						case 1:
							myStats->cloak = newItem(CLOAK_PROTECTION, EXCELLENT, -1 + rand() % 2, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, NULL);
							break;
						case 2:
							myStats->cloak = newItem(CLOAK, EXCELLENT, -1 + rand() % 2, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, NULL);
							break;
						case 3:
							break;
						default:
							break;
					}
				}
			}
		}
	}

	// torso
	Entity* entity = newEntity(468, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[AUTOMATON][1][0]; // 0
	entity->focaly = limbs[AUTOMATON][1][1]; // 0
	entity->focalz = limbs[AUTOMATON][1][2]; // 0
	entity->behavior = &actAutomatonLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right leg
	entity = newEntity(474, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[AUTOMATON][2][0]; // 0
	entity->focaly = limbs[AUTOMATON][2][1]; // 0
	entity->focalz = limbs[AUTOMATON][2][2]; // 2
	entity->behavior = &actAutomatonLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left leg
	entity = newEntity(473, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[AUTOMATON][3][0]; // 0
	entity->focaly = limbs[AUTOMATON][3][1]; // 0
	entity->focalz = limbs[AUTOMATON][3][2]; // 2
	entity->behavior = &actAutomatonLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right arm
	entity = newEntity(471, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[AUTOMATON][4][0]; // 0
	entity->focaly = limbs[AUTOMATON][4][1]; // 0
	entity->focalz = limbs[AUTOMATON][4][2]; // 2
	entity->behavior = &actAutomatonLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left arm
	entity = newEntity(469, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[AUTOMATON][5][0]; // 0
	entity->focaly = limbs[AUTOMATON][5][1]; // 0
	entity->focalz = limbs[AUTOMATON][5][2]; // 2
	entity->behavior = &actAutomatonLimb;
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
	entity->flags[INVISIBLE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[AUTOMATON][6][0]; // 2.5
	entity->focaly = limbs[AUTOMATON][6][1]; // 0
	entity->focalz = limbs[AUTOMATON][6][2]; // 0
	entity->behavior = &actAutomatonLimb;
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
	entity->flags[INVISIBLE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[AUTOMATON][7][0]; // 2
	entity->focaly = limbs[AUTOMATON][7][1]; // 0
	entity->focalz = limbs[AUTOMATON][7][2]; // 0
	entity->behavior = &actAutomatonLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// cloak
	entity = newEntity(-1, 0, map.entities, nullptr); //Limb entity.
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
	entity->focalx = limbs[AUTOMATON][8][0]; // 0
	entity->focaly = limbs[AUTOMATON][8][1]; // 0
	entity->focalz = limbs[AUTOMATON][8][2]; // 4
	entity->behavior = &actAutomatonLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// helmet
	entity = newEntity(-1, 0, map.entities, nullptr); //Limb entity.
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
	entity->focalx = limbs[AUTOMATON][9][0]; // 0
	entity->focaly = limbs[AUTOMATON][9][1]; // 0
	entity->focalz = limbs[AUTOMATON][9][2]; // -2
	entity->behavior = &actAutomatonLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// mask
	entity = newEntity(-1, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[INVISIBLE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[AUTOMATON][10][0]; // 0
	entity->focaly = limbs[AUTOMATON][10][1]; // 0
	entity->focalz = limbs[AUTOMATON][10][2]; // .5
	entity->behavior = &actAutomatonLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	if ( multiplayer == CLIENT || MONSTER_INIT )
	{
		return;
	}
}

void actAutomatonLimb(Entity* my)
{
	my->actMonsterLimb(true);
}

void automatonDie(Entity* my)
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
					entity->sprite = 467;
					break;
				case 1:
					entity->sprite = 468;
					break;
				case 2:
					entity->sprite = 469;
					break;
				case 3:
					entity->sprite = 470;
					break;
				case 4:
					entity->sprite = 471;
					break;
				case 5:
					entity->sprite = 472;
					break;
			}
			serverSpawnGibForClient(entity);
		}
	}
	playSoundEntity(my, 260 + rand() % 3, 128);
	list_RemoveNode(my->mynode);
	return;
}

#define AUTOMATONWALKSPEED .13

void automatonMoveBodyparts(Entity* my, Stat* myStats, double dist)
{
	node_t* node;
	Entity* entity = NULL, *entity2 = NULL;
	Entity* rightbody = NULL;
	Entity* weaponarm = NULL;
	int bodypart;
	bool wearingring = false;

	// set invisibility //TODO: use isInvisible()?
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
				if ( bodypart < LIMB_HUMANOID_TORSO )
				{
					bodypart++;
					continue;
				}
				if ( bodypart >= LIMB_HUMANOID_WEAPON )
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
				if ( bodypart < LIMB_HUMANOID_TORSO )
				{
					bodypart++;
					continue;
				}
				if ( bodypart >= LIMB_HUMANOID_WEAPON )
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
		if ( myStats->EFFECTS[EFF_ASLEEP] 
			&& (my->monsterSpecialState != AUTOMATON_MALFUNCTION_START && my->monsterSpecialState != AUTOMATON_MALFUNCTION_RUN) )
		{
			my->z = 2;
			my->pitch = PI / 4;
		}
		else
		{
			if ( my->monsterSpecialState != AUTOMATON_MALFUNCTION_START && my->monsterSpecialState != AUTOMATON_MALFUNCTION_RUN )
			{
				my->z = -.5;
				my->pitch = 0;
				if ( (myStats->HP < 25 && !myStats->EFFECTS[EFF_CONFUSED])
					|| (myStats->HP < 50 && !strncmp(myStats->name, "corrupted automaton", strlen("corrupted automaton")))
					)
				{
					// threshold for boom boom
					if ( rand() % 4 > 0 ) // 3/4
					{
						my->monsterSpecialState = AUTOMATON_MALFUNCTION_START;
						my->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_AUTOMATON_MALFUNCTION;
						serverUpdateEntitySkill(my, 33);

						myStats->EFFECTS[EFF_PARALYZED] = true;
						myStats->EFFECTS_TIMERS[EFF_PARALYZED] = -1;
					}
					else
					{
						myStats->EFFECTS[EFF_CONFUSED] = true;
						myStats->EFFECTS_TIMERS[EFF_CONFUSED] = -1;
						myStats->EFFECTS[EFF_PARALYZED] = true;
						myStats->EFFECTS_TIMERS[EFF_PARALYZED] = 25;
						playSoundEntity(my, 263, 128);
						spawnMagicEffectParticles(my->x, my->y, my->z, 170);
					}
				}
			}
		}
	}

	if ( my->monsterSpecialState == AUTOMATON_MALFUNCTION_START || my->monsterSpecialState == AUTOMATON_MALFUNCTION_RUN )
	{
		if ( my->monsterSpecialState == AUTOMATON_MALFUNCTION_START )
		{
			my->monsterSpecialState = AUTOMATON_MALFUNCTION_RUN;
			createParticleExplosionCharge(my, 174, 100, 0.1);
		}
		if ( multiplayer != CLIENT )
		{
			if ( my->monsterSpecialTimer <= 0 )
			{
				my->attack(MONSTER_POSE_AUTOMATON_MALFUNCTION, 0, my);
				spawnExplosion(my->x, my->y, my->z);
				my->modHP(-1000);
			}
		}
	}

	if ( ticks % (2 * TICKS_PER_SECOND) == 0 && rand() % 5 > 0 )
	{
		playSoundEntityLocal(my, 259, 16);
	}

	//Move bodyparts
	for (bodypart = 0, node = my->children.first; node != NULL; node = node->next, bodypart++)
	{
		if ( bodypart < LIMB_HUMANOID_TORSO )
		{
			if ( multiplayer != CLIENT )
			{
				if ( bodypart == 0 && my->monsterSpecialState == AUTOMATON_MALFUNCTION_RUN  )
				{
					--my->monsterSpecialTimer;
					if ( my->monsterSpecialTimer == 100 )
					{
						playSoundEntity(my, 321, 128);
					}
					if ( my->monsterSpecialTimer < 80 )
					{
						my->z += 0.02;
						limbAnimateToLimit(my, ANIMATE_PITCH, 0.1, 0.7, true, 0.1);
					}
					else
					{
						limbAnimateToLimit(my, ANIMATE_PITCH, 0.01, PI / 5, false, 0.0);
					}
				}
			}
			continue;
		}
		entity = (Entity*)node->element;
		entity->x = my->x;
		entity->y = my->y;
		if ( my->monsterSpecialState != AUTOMATON_MALFUNCTION_START && my->monsterSpecialState != AUTOMATON_MALFUNCTION_RUN )
		{
			entity->z = my->z;
		}
		else
		{
			if ( bodypart == LIMB_HUMANOID_RIGHTLEG || bodypart == LIMB_HUMANOID_LEFTLEG )
			{
				entity->z = -.5;
			}
			else
			{
				entity->z = my->z;
			}
		}

		if ( (MONSTER_ATTACK == MONSTER_POSE_MAGIC_WINDUP1 || MONSTER_ATTACK == MONSTER_POSE_SPECIAL_WINDUP1) && bodypart == LIMB_HUMANOID_RIGHTARM )
		{
			// don't let the creatures's yaw move the casting arm
		}
		else
		{
			entity->yaw = my->yaw;
		}

		if ( bodypart == LIMB_HUMANOID_RIGHTLEG || bodypart == LIMB_HUMANOID_LEFTARM )
		{
			if ( bodypart == LIMB_HUMANOID_LEFTARM && my->monsterSpecialState == AUTOMATON_MALFUNCTION_RUN )
			{
				limbAnimateToLimit(entity, ANIMATE_PITCH, -0.1, 13 * PI / 8, true, 0.1);
			}
			else
			{
				my->humanoidAnimateWalk(entity, node, bodypart, AUTOMATONWALKSPEED, dist, 0.1);
			}
		}
		else if ( bodypart == LIMB_HUMANOID_LEFTLEG || bodypart == LIMB_HUMANOID_RIGHTARM || bodypart == LIMB_HUMANOID_CLOAK )
		{
			// left leg, right arm, cloak.
			if ( bodypart == LIMB_HUMANOID_RIGHTARM )
			{
				weaponarm = entity;
				if ( MONSTER_ATTACK > 0 )
				{
					if ( my->monsterAttack == MONSTER_POSE_SPECIAL_WINDUP1 )
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
							if ( multiplayer != CLIENT )
							{
								myStats->EFFECTS[EFF_PARALYZED] = true;
								myStats->EFFECTS_TIMERS[EFF_PARALYZED] = 30;
							}
						}

						double animationYawSetpoint = 0.f;
						double animationYawEndpoint = 0.f;
						double armSwingRate = 0.f;
						double animationPitchSetpoint = 0.f;
						double animationPitchEndpoint = 0.f;

						switch ( my->monsterSpellAnimation )
						{
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

						if ( my->monsterAttackTime >= 3 * ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
						{
							weaponarm->skill[0] = rightbody->skill[0];
							weaponarm->pitch = rightbody->pitch;
							my->monsterArmbended = 0;
							my->monsterAttack = 0;
							if ( multiplayer != CLIENT )
							{
								spawnMagicEffectParticles(my->x, my->y, my->z / 2, 174);
								my->monsterSpecialState = AUTOMATON_RECYCLE_ANIMATION_COMPLETE;
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

			if ( bodypart == LIMB_HUMANOID_RIGHTARM && my->monsterSpecialState == AUTOMATON_MALFUNCTION_RUN )
			{
				limbAnimateToLimit(entity, ANIMATE_PITCH, -0.1, 13 * PI / 8, true, 0.1);
			}
			else
			{
				my->humanoidAnimateWalk(entity, node, bodypart, AUTOMATONWALKSPEED, dist, 0.1);
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
					if ( myStats->breastplate == nullptr )
					{
						entity->sprite = 468;
					}
					else
					{
						entity->sprite = itemModel(myStats->breastplate);
						entity->scalex = 1;
						// shrink the width of the breastplate
						entity->scaley = 0.8;
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
				else if ( multiplayer == CLIENT )
				{
					if ( entity->sprite != 468 )
					{
						entity->scalex = 1;
						// shrink the width of the breastplate
						entity->scaley = 0.8;
					}
					else
					{
						entity->scalex = 1;
						entity->scaley = 1;
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
						entity->sprite = 474;
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
				if ( my->z >= 1.9 && my->z <= 2.1 )
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
						entity->sprite = 473;
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
				if ( my->z >= 1.9 && my->z <= 2.1 )
				{
					entity->yaw -= PI / 8;
					entity->pitch = -PI / 2;
				}
				break;
			// right arm
			case LIMB_HUMANOID_RIGHTARM:
			{
				node_t* weaponNode = list_Node(&my->children, 7);
				if ( weaponNode )
				{
					Entity* weapon = (Entity*)weaponNode->element;
					if ( MONSTER_ARMBENDED || (weapon->flags[INVISIBLE] && my->monsterAttack == 0) )
					{
						// if weapon invisible and I'm not attacking, relax arm.
						entity->focalx = limbs[AUTOMATON][4][0]; // 0
						entity->focaly = limbs[AUTOMATON][4][1]; // 0
						entity->focalz = limbs[AUTOMATON][4][2]; // 2
						entity->sprite = 471;
					}
					else
					{
						// else flex arm.
						entity->focalx = limbs[AUTOMATON][4][0] + 1.5; // 1
						entity->focaly = limbs[AUTOMATON][4][1] + 0.25; // 0
						entity->focalz = limbs[AUTOMATON][4][2] - 1; // 1
						entity->sprite = 472;
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
			case LIMB_HUMANOID_LEFTARM:
			{
				node_t* shieldNode = list_Node(&my->children, 8);
				if ( shieldNode )
				{
					Entity* shield = (Entity*)shieldNode->element;
					if ( shield->flags[INVISIBLE] )
					{
						// if shield invisible, relax arm.
						entity->sprite = 469;
						entity->focalx = limbs[AUTOMATON][5][0]; // 0
						entity->focaly = limbs[AUTOMATON][5][1]; // 0
						entity->focalz = limbs[AUTOMATON][5][2]; // 2
					}
					else
					{
						// else flex arm.
						entity->sprite = 470;
						entity->focalx = limbs[AUTOMATON][5][0] + 1.5; // 1
						entity->focaly = limbs[AUTOMATON][5][1] - 0.25; // 0
						entity->focalz = limbs[AUTOMATON][5][2] - 1; // 1
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
				entity->focalx = limbs[AUTOMATON][9][0]; // 0
				entity->focaly = limbs[AUTOMATON][9][1]; // 0
				entity->focalz = limbs[AUTOMATON][9][2]; // -2
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
				entity->focalx = limbs[AUTOMATON][10][0]; // 0
				entity->focaly = limbs[AUTOMATON][10][1]; // 0
				entity->focalz = limbs[AUTOMATON][10][2]; // .5
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
					entity->focalx = limbs[AUTOMATON][10][0] + .35; // .35
					entity->focaly = limbs[AUTOMATON][10][1] - 2; // -2
					entity->focalz = limbs[AUTOMATON][10][2]; // .5
				}
				else
				{
					entity->focalx = limbs[AUTOMATON][10][0] + .25; // .25
					entity->focaly = limbs[AUTOMATON][10][1] - 2.25; // -2.25
					entity->focalz = limbs[AUTOMATON][10][2]; // .5
				}
				break;
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

bool Entity::automatonCanWieldItem(const Item& item) const
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
		case ARMOR:
			{
				int equipType = checkEquipType(&item);
				if ( equipType == TYPE_HAT )
				{
					return false; //No can wear hats, beep boop
				}
				return true;
			}
		case THROWN:
			return true;
		case MAGICSTAFF:
			return true;
		default:
			return false;
	}

	return false;
}


void Entity::automatonRecycleItem()
{
	Stat* myStats = getStats();
	if ( !myStats )
	{
		return;
	}

	if ( this->monsterSpecialTimer > 0 && !(this->monsterSpecialState == AUTOMATON_RECYCLE_ANIMATION_COMPLETE) )
	{
		// if we're on cooldown, skip checking
		// also we need the callback from my->attack() to set monsterSpecialState = AUTOMATON_RECYCLE_ANIMATION_COMPLETE once the animation completes.
		return;
	}

	node_t* node = nullptr;
	node_t* nextnode = nullptr;
	int numItemsHeld = list_Size(&myStats->inventory);
	//messagePlayer(0, "Numitems: %d", numItemsHeld);
	if ( numItemsHeld < 2 )
	{
		return;
	}

	if ( this->monsterSpecialTimer == 0 && this->monsterSpecialState == AUTOMATON_RECYCLE_ANIMATION_WAITING )
	{
		// put the skill on cooldown.
		this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_AUTOMATON_RECYCLE;
	}

	int i = 0;
	int itemIndex = 0;
	int chances[10] = { -1 }; // max 10 items
	int matches = 0;

	// search for valid recyclable items, set chance for valid index.
	for ( node = myStats->inventory.first; node != nullptr; node = nextnode )
	{
		if ( matches >= 10 )
		{
			break;
		}
		nextnode = node->next;
		Item* item = (Item*)node->element;
		if ( item != nullptr )
		{
			if ( (itemCategory(item) == WEAPON || itemCategory(item) == THROWN || itemCategory(item) == ARMOR)
				&& item->status > BROKEN )
			{
				chances[matches] = itemIndex;
				++matches;
			}
		}
		++itemIndex;
	}

	//messagePlayer(0, "Found %d items", matches);

	if ( matches < 2 ) // not enough valid items found.
	{
		this->monsterSpecialTimer = 250; // reset cooldown to 5 seconds to check again quicker.
		return;
	}
	
	if ( this->monsterSpecialState == AUTOMATON_RECYCLE_ANIMATION_WAITING )
	{
		// this is the first run of the check, we'll execute the casting animation and wait for this to be set to 1 when it ends.
		this->attack(MONSTER_POSE_SPECIAL_WINDUP1, 0, nullptr);
		return;
	}

	this->monsterSpecialState = AUTOMATON_RECYCLE_ANIMATION_WAITING; // reset my special state after the previous lines.
	int pickItem1 = rand() % matches; // pick random valid item index in inventory
	int pickItem2 = rand() % matches;
	while ( pickItem2 == pickItem1 )
	{
		pickItem2 = rand() % matches; // make sure index 2 is unique
	}

	itemIndex = 0;
	Item* item1 = nullptr;
	Item* item2 = nullptr;

	// search again for the 2 indexes, store the items and nodes.
	for ( node = myStats->inventory.first; node != nullptr; node = nextnode )
	{
		nextnode = node->next;
		if ( chances[pickItem1] == itemIndex )
		{
			item1 = (Item*)node->element;
		}
		else if ( chances[pickItem2] == itemIndex )
		{
			item2 = (Item*)node->element;
		}
		++itemIndex;
	}

	if ( item1 == nullptr || item2 == nullptr )
	{
		return;
	}

	//messagePlayer(0, "made it past");

	int maxGoldValue = ((items[item1->type].value + items[item2->type].value) * 2) / 3;
	if ( rand() % 2 == 0 )
	{
		maxGoldValue = ((items[item1->type].value + items[item2->type].value) * 1) / 2;
	}
	int minGoldValue = ((items[item1->type].value + items[item2->type].value) * 1) / 3;
	ItemType type;
	// generate a weapon/armor piece and add it into the inventory.
	switch ( rand() % 10 )
	{
		case 0:
		case 1:
		case 2:
			type = itemTypeWithinGoldValue(WEAPON, minGoldValue, maxGoldValue);
			break;
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
			type = itemTypeWithinGoldValue(ARMOR, minGoldValue, maxGoldValue);
			break;
		case 9:
			type = itemTypeWithinGoldValue(THROWN, minGoldValue, maxGoldValue);
			break;
		default:
			break;
	}

	if ( type != GEM_ROCK ) // found an item in category
	{
		Item* item = nullptr;
		// recycle item1 or item2, reduce durability.
		if ( rand() % 2 == 0 )
		{
			item = newItem(type, item1->status, item1->beatitude, 1, rand(), item1->identified, &myStats->inventory);
			item1->status = static_cast<Status>(std::max(0, item1->status - 2));
		}
		else
		{
			item = newItem(type, item2->status, item2->beatitude, 1, rand(), item2->identified, &myStats->inventory);
			item2->status = static_cast<Status>(std::max(0, item2->status - 2));
		}
		// drop newly created item. To pickup if possible or leave behind if overburdened.
		dropItemMonster(item, this, myStats);
		//messagePlayer(0, "Generated %d!", type);
	}

	return;
}
