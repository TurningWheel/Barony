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
		MONSTER_SPOTSND = 291;
		MONSTER_SPOTVAR = 4;
		MONSTER_IDLESND = 285;
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
			if ( !strncmp(myStats->name, "lesser insectoid", strlen("lesser insectoid")) )
			{
				lesserMonster = true;
				myStats->HP = 110;
				myStats->MAXHP = myStats->HP;
				myStats->RANDOM_MAXHP = 10;
				myStats->RANDOM_HP = myStats->RANDOM_MAXHP;
				myStats->OLDHP = myStats->HP;
				myStats->STR = 8;
				myStats->RANDOM_STR = 0;
				myStats->DEX = 6;
				myStats->CON = 7;
				myStats->INT = -2;
				myStats->PER = 5;
				myStats->CHR = 5;
				myStats->EXP = 0;
				myStats->LVL = 10;
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
				/*myStats->HP = 120;
				myStats->MAXHP = 120;
				myStats->OLDHP = myStats->HP;
				strcpy(myStats->name, "The Potato King");
				myStats->weapon = newItem(ARTIFACT_MACE, EXCELLENT, 1, 1, rand(), true, nullptr);
				myStats->helmet = newItem(HAT_JESTER, SERVICABLE, 3 + rand() % 3, 1, 0, false, nullptr);

				int c;
				for ( c = 0; c < 3; c++ )
				{
					Entity* entity = summonMonster(GOBLIN, my->x, my->y);
					if ( entity )
					{
						entity->parent = my->getUID();
					}
				}*/
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

			my->setHardcoreStats(*myStats);

			// always give special spell to insectoid, undroppable.
			newItem(SPELLBOOK_ACID_SPRAY, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, &myStats->inventory);

			// generate the default inventory items for the monster, provided the editor sprite allowed enough default slots
			switch ( defaultItems )
			{
				case 6:
				case 5:
				case 4:
				case 3:
					if ( !lesserMonster && rand() % 20 == 0 )
					{
						newItem(SPELLBOOK_ACID_SPRAY, SERVICABLE, -1 + rand() % 3, 1, rand(), false, &myStats->inventory);
					}
				case 2:
					if ( rand() % 2 == 0 )
					{
						newItem(SHORTBOW, SERVICABLE, -1 + rand() % 3, 1, rand(), false, &myStats->inventory);
					}
				case 1:
					if ( lesserMonster )
					{
						newItem(IRON_DAGGER, SERVICABLE, 0, 0 + rand() % 2, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, &myStats->inventory);
					}
					else
					{
						newItem(IRON_DAGGER, SERVICABLE, 0, 2 + rand() % 4, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, &myStats->inventory);
					}
					break;
				default:
					break;
			}

			if ( lesserMonster )
			{
				if ( myStats->shield == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_SHIELD] == 1 )
				{
					// give shield
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
							myStats->shield = newItem(STEEL_SHIELD, static_cast<Status>(DECREPIT + rand() % 3), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
						case 7:
						case 8:
						case 9:
							myStats->shield = newItem(IRON_SHIELD, static_cast<Status>(WORN + rand() % 2), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
						default:
							break;
					}
				}

				//give weapon
				if ( myStats->weapon == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1 )
				{
					switch ( rand() % 10 )
					{
						case 0:
							myStats->weapon = newItem(IRON_AXE, static_cast<Status>(DECREPIT + rand() % 2), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
						case 1:
						case 2:
						case 3:
						case 4:
							myStats->weapon = newItem(IRON_SWORD, static_cast<Status>(DECREPIT + rand() % 2), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
						case 5:
						case 6:
						case 7:
							myStats->weapon = newItem(IRON_SPEAR, static_cast<Status>(DECREPIT + rand() % 3), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
						case 8:
							myStats->weapon = newItem(STEEL_SWORD, static_cast<Status>(DECREPIT + rand() % 3), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
						case 9:
							myStats->weapon = newItem(STEEL_HALBERD, static_cast<Status>(WORN + rand() % 2), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
						default:
							break;
					}
				}

				// give cloak
				if ( myStats->cloak == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_CLOAK] == 1 )
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
							myStats->cloak = newItem(CLOAK, static_cast<Status>(WORN + rand() % 2), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
					}
				}

				// give booties
				if ( myStats->shoes == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_BOOTS] == 1 )
				{
					switch ( rand() % 10 )
					{
						case 0:
						case 1:
							myStats->shoes = newItem(STEEL_BOOTS, static_cast<Status>(DECREPIT + rand() % 3), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
						case 2:
						case 3:
						case 4:
						case 5:
						case 6:
							myStats->shoes = newItem(IRON_BOOTS, static_cast<Status>(DECREPIT + rand() % 3), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
						case 7:
						case 8:
						case 9:
							break;
						default:
							break;

					}
				}
			}
			else
			{
				if ( myStats->shield == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_SHIELD] == 1 )
				{
					// give shield
					switch ( rand() % 20 )
					{
						case 0:
						case 1:
						case 2:
						case 3:
						case 4:
						case 5:
						case 6:
						case 7:
							myStats->shield = newItem(STEEL_SHIELD, static_cast<Status>(DECREPIT + rand() % 3), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
						case 8:
						case 9:
						case 10:
						case 11:
							myStats->shield = newItem(IRON_SHIELD, static_cast<Status>(WORN + rand() % 2), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
						case 18:
							myStats->shield = newItem(CRYSTAL_SHIELD, static_cast<Status>(DECREPIT + rand() % 3), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
						case 19:
							myStats->shield = newItem(CRYSTAL_SHIELD, static_cast<Status>(WORN + rand() % 3), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
						default:
							break;
					}
				}

				//give weapon
				if ( myStats->weapon == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1 )
				{
					switch ( rand() % 20 )
					{
						case 0:
						case 1:
						case 2:
							myStats->weapon = newItem(STEEL_AXE, static_cast<Status>(WORN + rand() % 2), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
						case 3:
						case 4:
						case 5:
						case 6:
						case 7:
						case 8:
							myStats->weapon = newItem(STEEL_HALBERD, static_cast<Status>(WORN + rand() % 2), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
						case 9:
						case 10:
						case 11:
						case 12:
						case 13:
						case 14:
							myStats->weapon = newItem(STEEL_SWORD, static_cast<Status>(WORN + rand() % 2), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
						case 15:
							myStats->weapon = newItem(STEEL_SWORD, static_cast<Status>(SERVICABLE + rand() % 2), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
						case 16:
						case 17:
							myStats->weapon = newItem(STEEL_HALBERD, static_cast<Status>(SERVICABLE + rand() % 2), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
						case 18:
							myStats->weapon = newItem(CRYSTAL_SWORD, static_cast<Status>(WORN + rand() % 3), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
						case 19:
							myStats->weapon = newItem(CRYSTAL_SPEAR, static_cast<Status>(WORN + rand() % 3), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
					}
				}

				// give cloak
				if ( myStats->cloak == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_CLOAK] == 1 )
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
							myStats->cloak = newItem(CLOAK, static_cast<Status>(WORN + rand() % 2), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
					}
				}

				// give helmet
				/*if ( myStats->helmet == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_HELM] == 1 )
				{
					switch ( rand() % 10 )
					{
						case 0:
						case 1:
						case 2:
							break;
						case 3:
						case 4:
							myStats->helmet = newItem(HAT_PHRYGIAN, WORN, -1 + rand() % 3, 1, 0, false, nullptr);
							break;
						case 5:
							myStats->helmet = newItem(HAT_WIZARD, WORN, -1 + rand() % 3, 1, 0, false, nullptr);
							break;
						case 6:
						case 7:
							myStats->helmet = newItem(LEATHER_HELM, WORN, -1 + rand() % 3, 1, 0, false, nullptr);
							break;
						case 8:
						case 9:
							myStats->helmet = newItem(IRON_HELM, WORN, -1 + rand() % 3, 1, 0, false, nullptr);
							break;
					}
				}*/

				// give armor
				/*if ( myStats->breastplate == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_ARMOR] == 1 )
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
							myStats->breastplate = newItem(IRON_BREASTPIECE, static_cast<Status>(DECREPIT + rand() % 4), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
						case 9:
							myStats->breastplate = newItem(STEEL_BREASTPIECE, static_cast<Status>(DECREPIT + rand() % 4), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
					}
				}*/

				// give booties
				if ( myStats->shoes == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_BOOTS] == 1 )
				{
					switch ( rand() % 20 )
					{
						case 0:
						case 1:
						case 2:
							myStats->shoes = newItem(STEEL_BOOTS, static_cast<Status>(DECREPIT + rand() % 3), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
						case 3:
						case 4:
						case 5:
						case 6:
							myStats->shoes = newItem(IRON_BOOTS, static_cast<Status>(DECREPIT + rand() % 3), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
						case 19:
							myStats->shoes = newItem(CRYSTAL_BOOTS, static_cast<Status>(WORN + rand() % 3), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
						default:
							break;

					}
				}
			}
		}
	}

	// torso
	Entity* entity = newEntity(458, 0, map.entities, nullptr); //Limb entity.
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
	my->bodyparts.push_back(entity);

	// right leg
	entity = newEntity(457, 0, map.entities, nullptr); //Limb entity.
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
	my->bodyparts.push_back(entity);

	// left leg
	entity = newEntity(456, 0, map.entities, nullptr); //Limb entity.
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
	my->bodyparts.push_back(entity);

	// right arm
	entity = newEntity(453, 0, map.entities, nullptr); //Limb entity.
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
	my->bodyparts.push_back(entity);

	// left arm
	entity = newEntity(451, 0, map.entities, nullptr); //Limb entity.
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
	my->bodyparts.push_back(entity);

	// world weapon
	entity = newEntity(-1, 0, map.entities, nullptr); //Limb entity.
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
	my->bodyparts.push_back(entity);

	// shield
	entity = newEntity(-1, 0, map.entities, nullptr); //Limb entity.
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
	my->bodyparts.push_back(entity);

	// cloak
	entity = newEntity(-1, 0, map.entities, nullptr); //Limb entity.
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
	my->bodyparts.push_back(entity);

	// mask
	entity = newEntity(-1, 0, map.entities, nullptr); //Limb entity.
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
	my->bodyparts.push_back(entity);

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

	playSoundEntity(my, 287 + rand() % 4, 128);

	my->removeMonsterDeathNodes();

	list_RemoveNode(my->mynode);
	return;
}

#define INSECTOIDWALKSPEED .13

void insectoidMoveBodyparts(Entity* my, Stat* myStats, double dist)
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
			if ( myStats->ring->type == RING_INVISIBILITY )
			{
				wearingring = true;
			}
		if ( myStats->cloak != nullptr )
			if ( myStats->cloak->type == CLOAK_INVISIBILITY )
			{
				wearingring = true;
			}
		if ( myStats->EFFECTS[EFF_INVISIBLE] == true || wearingring == true )
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
	for (bodypart = 0, node = my->children.first; node != nullptr; node = node->next, bodypart++)
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
				(my->monsterSpecialState == INSECTOID_ACID && my->monsterAttack != 0) )
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

						if ( my->monsterAttackTime >= 3 * ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
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
							// play casting sound
							playSoundEntityLocal(my, 170, 64);
							// monster scream
							playSoundEntityLocal(my, MONSTER_SPOTSND + 1, 128);
							if ( multiplayer != CLIENT )
							{
								myStats->EFFECTS[EFF_PARALYZED] = true;
								myStats->EFFECTS_TIMERS[EFF_PARALYZED] = 60;
							}
						}

						limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.25, 5 * PI / 4, false, 0.0);
						if ( multiplayer != CLIENT )
						{
							// move the head and weapon yaw
							limbAnimateToLimit(my, ANIMATE_PITCH, -0.1, 15 * PI / 8, true, 0.1);
							limbAnimateToLimit(my, ANIMATE_WEAPON_YAW, 0.25, 4 * PI / 8, false, 0.0);
						}

						if ( my->monsterAttackTime >= 6 * ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
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
					if ( myStats->breastplate == nullptr )
					{
						entity->sprite = 458;
					}
					else
					{
						entity->sprite = itemModel(myStats->breastplate);
						entity->scalex = 0.9;
						// shrink the width of the breastplate
						entity->scaley = 0.9;
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
						entity->scalex = 0.9;
						// shrink the width of the breastplate
						entity->scaley = 0.9;
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
				entity->focalx = limbs[INSECTOID][9][0]; // 0
				entity->focaly = limbs[INSECTOID][9][1]; // 0
				entity->focalz = limbs[INSECTOID][9][2]; // -2
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
				entity->focalx = limbs[INSECTOID][10][0]; // 0
				entity->focaly = limbs[INSECTOID][10][1]; // 0
				entity->focalz = limbs[INSECTOID][10][2]; // .25
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
	if ( monsterSpecialState != 0 )
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
	int bonusFromHP = 0;

	// throwing weapons
	// occurs less often against fellow monsters.
	if ( monsterSpecialTimer == 0 && (ticks % 10 == 0) && monsterAttack == 0 )
	{
		specialRoll = rand() % (40 + 40 * (target->behavior == &actMonster));
		//messagePlayer(0, "rolled: %d", specialRoll);
		if ( myStats->HP <= myStats->MAXHP * 0.6 )
		{
			bonusFromHP += 1; // +5% chance if on low health
		}
		if ( myStats->HP <= myStats->MAXHP * 0.3 )
		{
			bonusFromHP += 1; // +extra 5% chance if on lower health
		}
		if ( specialRoll < (1 + bonusFromHP) ) // +5% base
		{
			node_t* node = itemNodeInInventory(myStats, static_cast<ItemType>(-1), THROWN);
			if ( node != nullptr )
			{
				bool swapped = swapMonsterWeaponWithInventoryItem(this, myStats, node, true, true);
				if ( swapped )
				{
					if ( myStats->weapon->count > 1 )
					{
						monsterSpecialState = INSECTOID_DOUBLETHROW_FIRST + rand() % 2; // 50% for double throw.
					}
					else
					{
						monsterSpecialState = INSECTOID_DOUBLETHROW_SECOND;
					}
				}
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
