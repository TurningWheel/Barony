/*-------------------------------------------------------------------------------

	BARONY
	File: monster_shopkeeper.cpp
	Desc: implements all of the shopkeeper's code

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

void initShopkeeper(Entity* my, Stat* myStats)
{
	int c;
	node_t* node;

	my->initMonster(217);

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = -1;
		MONSTER_SPOTVAR = 1;
		MONSTER_IDLESND = -1;
		MONSTER_IDLEVAR = 1;
	}
	if ( multiplayer != CLIENT && !MONSTER_INIT )
	{
		my->createPathBoundariesNPC();

		for ( int x = my->monsterPathBoundaryXStart - 16; x <= my->monsterPathBoundaryXEnd + 16; x += 16 )
		{
			for ( int y = my->monsterPathBoundaryYStart - 16; y <= my->monsterPathBoundaryYEnd + 16; y += 16 )
			{
				if ( x / 16 >= 0 && x / 16 < map.width && y / 16 >= 0 && y / 16 < map.height )
				{
					shoparea[y / 16 + (x / 16)*map.height] = true;
				}
			}
		}

		if ( myStats != NULL )
		{
			if ( !myStats->leader_uid )
			{
				myStats->leader_uid = 0;
			}

			if ( !strcmp(myStats->name, "") )
			{
				strcpy(myStats->name, language[158 + rand() % 26]);
			}

			// apply random stat increases if set in stat_shared.cpp or editor
			setRandomMonsterStats(myStats);

			if ( currentlevel >= 25 && myStats->HP == 300 && myStats->MAXHP == 300 )
			{
				myStats->HP *= 2;
				myStats->MAXHP *= 2;
				myStats->OLDHP = myStats->HP;
			}

			// generate 6 items max, less if there are any forced items from boss variants
			int customItemsToGenerate = ITEM_CUSTOM_SLOT_LIMIT;

			// boss variants

			// random effects
			if ( rand() % 20 == 0 )
			{
				myStats->EFFECTS[EFF_ASLEEP] = true;
				myStats->EFFECTS_TIMERS[EFF_ASLEEP] = 1800 + rand() % 3600;
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
				if ( currentlevel < 25 )
				{
					myStats->weapon = newItem(SPELLBOOK_MAGICMISSILE, EXCELLENT, 0, 1, 0, false, NULL);
				}
				else
				{
					if ( rand() % 2 == 0 )
					{
						myStats->weapon = newItem(SPELLBOOK_DRAIN_SOUL, EXCELLENT, 0, 1, 0, false, NULL);
					}
					else
					{
						myStats->weapon = newItem(SPELLBOOK_BLEED, EXCELLENT, 0, 1, 0, false, NULL);
					}
				}
			}

			// give shopkeeper items

			if ( myStats->MISC_FLAGS[STAT_FLAG_NPC] > 0 )
			{
				my->monsterStoreType = myStats->MISC_FLAGS[STAT_FLAG_NPC] - 1;
				if ( my->monsterStoreType > 9 )
				{
					my->monsterStoreType = rand() % 9;
					if ( my->monsterStoreType == 8 )
					{
						my->monsterStoreType++;
					}
				}
			}
			else
			{
				my->monsterStoreType = rand() % 9;
				if ( my->monsterStoreType == 8 )
				{
					my->monsterStoreType++;
				}
			}
			int numitems = 10 + rand() % 5;

			int blessedShopkeeper = 1; // bless important pieces of gear like armor, jewelry, weapons..
			if ( currentlevel >= 30 )
			{
				if ( rand() % 3 == 0 )
				{
					blessedShopkeeper = 3;
				}
				else
				{
					blessedShopkeeper = 2;
				}
			}
			else if ( currentlevel >= 25 )
			{
				if ( rand() % 4 == 0 )
				{
					blessedShopkeeper = 3;
				}
				else
				{
					blessedShopkeeper = 2;
				}
			}
			else if ( currentlevel >= 18 )
			{
				if ( rand() % 3 == 0 )
				{
					blessedShopkeeper = 2;
				}
			}

			Item* tmpItem = nullptr;

			switch ( my->monsterStoreType )
			{
				case 0:
					// arms & armor store
					if ( blessedShopkeeper > 0 )
					{
						numitems += rand() % 5; // offset some of the quantity reduction.
					}
					for ( c = 0; c < numitems; c++ )
					{
						if ( currentlevel >= 18 )
						{
							if ( rand() % 2 )
							{
								if ( rand() % 10 == 0 )
								{
									tmpItem = newItem(itemLevelCurve(THROWN, 8, currentlevel), static_cast<Status>(SERVICABLE + rand() % 2), 0, 3 + rand() % 3, rand(), false, &myStats->inventory);
								}
								else
								{
									tmpItem = newItem(itemLevelCurve(ARMOR, 5, currentlevel), static_cast<Status>(WORN + rand() % 3), rand() % blessedShopkeeper, 1 + rand() % 4, rand(), false, &myStats->inventory);
								}
							}
							else
							{
								tmpItem = newItem(itemLevelCurve(WEAPON, 10, currentlevel), static_cast<Status>(WORN + rand() % 3), rand() % blessedShopkeeper, 1 + rand() % 4, rand(), false, &myStats->inventory);
							}
						}
						else
						{
							if ( rand() % 2 )
							{
								if ( rand() % 8 == 0 )
								{
									tmpItem = newItem(itemLevelCurve(THROWN, 0, currentlevel + 20), static_cast<Status>(WORN + rand() % 3), 0, 3 + rand() % 3, rand(), false, &myStats->inventory);
								}
								else
								{
									tmpItem = newItem(static_cast<ItemType>(rand() % 20), static_cast<Status>(WORN + rand() % 3), 0, 1 + rand() % 4, rand(), false, &myStats->inventory);
								}
							}
							else
							{
								int i = rand() % 23;
								if ( i < 18 )
								{
									tmpItem = newItem(static_cast<ItemType>(GLOVES + i), static_cast<Status>(WORN + rand() % 3), 0, 1 + rand() % 4, rand(), false, &myStats->inventory);
								}
								else if ( i < 21 )
								{
									tmpItem = newItem(static_cast<ItemType>(GLOVES + i + 4), static_cast<Status>(WORN + rand() % 3), 0, 1 + rand() % 6, rand(), false, &myStats->inventory);
								}
								else
								{
									// punching armaments
									tmpItem = newItem(static_cast<ItemType>(BRASS_KNUCKLES + rand() % 3), static_cast<Status>(WORN + rand() % 3), 0, 1 + rand() % 2, rand(), false, &myStats->inventory);
								}
							}
						}
						// post-processing
						if ( tmpItem && tmpItem->beatitude > 0 )
						{
							tmpItem->count = 1;
							tmpItem->status = static_cast<Status>(SERVICABLE + rand() % 2);
						}
					}
					break;
				case 1:
					// hat store
					for ( c = 0; c < numitems; c++ )
					{
						tmpItem = newItem(static_cast<ItemType>(HAT_PHRYGIAN + rand() % 7), static_cast<Status>(WORN + rand() % 3), rand() % blessedShopkeeper, 1 + rand() % 6, rand(), false, &myStats->inventory);
						// post-processing
						if ( tmpItem && tmpItem->beatitude > 0 )
						{
							tmpItem->count = 1;
							tmpItem->status = static_cast<Status>(SERVICABLE + rand() % 2);
						}
					}
					break;
				case 2:
					// jewelry store
					for ( c = 0; c < numitems; c++ )
					{
						switch ( rand() % 3 )
						{
							case 0:
								tmpItem = newItem(itemLevelCurve(AMULET, 0, currentlevel + 5), static_cast<Status>(WORN + rand() % 3), rand() % blessedShopkeeper, 1 + rand() % 2, rand(), false, &myStats->inventory);
								break;
							case 1:
								tmpItem = newItem(itemLevelCurve(RING, 0, currentlevel + 5), static_cast<Status>(WORN + rand() % 3), rand() % blessedShopkeeper, 1 + rand() % 2, rand(), false, &myStats->inventory);
								break;
							case 2:
								tmpItem = newItem(static_cast<ItemType>(GEM_GARNET + rand() % 16), static_cast<Status>(WORN + rand() % 3), 0, 1 + rand() % 2, rand(), false, &myStats->inventory);
								break;
						}
						// post-processing
						if ( tmpItem && tmpItem->beatitude > 0 )
						{
							tmpItem->count = 1;
							tmpItem->status = static_cast<Status>(SERVICABLE + rand() % 2);
						}
					}
					break;
				case 3:
					// bookstore
					for ( c = 0; c < numitems; c++ )
					{
						switch ( rand() % 3 )
						{
							case 0:
								if ( currentlevel >= 18 )
								{
									tmpItem = newItem(itemLevelCurve(SPELLBOOK, 0, currentlevel), static_cast<Status>(WORN + rand() % 3), 0, 1 + rand() % 2, rand(), true, &myStats->inventory);
								}
								else
								{
									tmpItem = newItem(static_cast<ItemType>(SPELLBOOK_FORCEBOLT + rand() % 22), static_cast<Status>(WORN + rand() % 3), 0, 1 + rand() % 2, rand(), true, &myStats->inventory);
								}
								break;
							case 1:
								tmpItem = newItem(static_cast<ItemType>(SCROLL_MAIL + rand() % 14), static_cast<Status>(WORN + rand() % 3), 0, 1 + rand() % 2, rand(), true, &myStats->inventory);
								break;
							case 2:
								tmpItem = newItem(READABLE_BOOK, static_cast<Status>(WORN + rand() % 3), 0, 1 + rand() % 3, rand(), false, &myStats->inventory);
								break;
						}
						// post-processing
						if ( rand() % blessedShopkeeper > 0 )
						{
							tmpItem->status = static_cast<Status>(SERVICABLE + rand() % 2);
						}
					}
					break;
				case 4:
					// apothecary
					for ( c = 0; c < numitems; c++ )
					{
						tmpItem = newItem(static_cast<ItemType>(POTION_WATER + rand() % 15), static_cast<Status>(WORN + rand() % 3), 0, 1 + rand() % 5, rand(), true, &myStats->inventory);
						// post-processing
						if ( rand() % blessedShopkeeper > 0 )
						{
							tmpItem->status = static_cast<Status>(SERVICABLE + rand() % 2);
						}
					}
					break;
				case 5:
					// staff shop
					for ( c = 0; c < numitems; c++ )
					{
						if ( currentlevel >= 18 )
						{
							tmpItem = newItem(itemLevelCurve(MAGICSTAFF, 0, currentlevel), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), true, &myStats->inventory);
						}
						else
						{
							tmpItem = newItem(static_cast<ItemType>(MAGICSTAFF_LIGHT + rand() % 10), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), true, &myStats->inventory);
						}
						// post-processing
						if ( rand() % blessedShopkeeper > 0 )
						{
							tmpItem->status = static_cast<Status>(SERVICABLE + rand() % 2);
						}
					}
					break;
				case 6:
					// food store
					for ( c = 0; c < numitems; c++ )
					{
						tmpItem = newItem(static_cast<ItemType>(FOOD_BREAD + rand() % 7), static_cast<Status>(SERVICABLE + rand() % 2), 0, 1 + rand() % 3, rand(), false, &myStats->inventory);
						// post-processing
						if ( rand() % blessedShopkeeper > 0 )
						{
							tmpItem->status = static_cast<Status>(SERVICABLE + rand() % 2);
						}
					}
					break;
				case 7:
					// hardware store
					for ( c = 0; c < numitems; c++ )
					{
						if ( rand() % 6 == 0 )
						{
							tmpItem = newItem(itemLevelCurve(THROWN, 0, currentlevel + 20), static_cast<Status>(WORN + rand() % 3), 0, 3 + rand() % 3, rand(), false, &myStats->inventory);
						}
						else
						{
							tmpItem = newItem(static_cast<ItemType>(TOOL_PICKAXE + rand() % 11), static_cast<Status>(WORN + rand() % 3), 0, 1 + rand() % 3, rand(), false, &myStats->inventory);
						}
						// post-processing
						if ( rand() % blessedShopkeeper > 0 )
						{
							tmpItem->status = static_cast<Status>(SERVICABLE + rand() % 2);
						}
					}
					break;
				case 8:
					// lighting store
					for ( c = 0; c < numitems; c++ )
					{
						tmpItem = newItem(static_cast<ItemType>(TOOL_TORCH + rand() % 2), EXCELLENT, 0, 1, rand(), false, &myStats->inventory);
						// post-processing
						if ( rand() % blessedShopkeeper > 0 )
						{
							tmpItem->status = static_cast<Status>(SERVICABLE + rand() % 2);
						}
					}
					break;
				case 9:
					// general store
					for ( c = 0; c < numitems; c++ )
					{
						Category cat = static_cast<Category>(rand() % (NUMCATEGORIES - 1));
						tmpItem = newItem(itemLevelCurve(cat, 0, currentlevel + 5), static_cast<Status>(WORN + rand() % 3), 0, 1 + rand() % 3, rand(), false, &myStats->inventory);
						if ( tmpItem && (itemCategory(tmpItem) == WEAPON || itemCategory(tmpItem) == ARMOR || itemCategory(tmpItem) == RING || itemCategory(tmpItem) == AMULET) )
						{
							tmpItem->beatitude += rand() % blessedShopkeeper;
							// post-processing
							if ( tmpItem->beatitude > 0 )
							{
								tmpItem->count = 1;
							}
						}
					}
					break;
			}
		}
	}

	// torso
	Entity* entity = newEntity(218, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SHOPKEEPER][1][0]; // 0
	entity->focaly = limbs[SHOPKEEPER][1][1]; // 0
	entity->focalz = limbs[SHOPKEEPER][1][2]; // 0
	entity->behavior = &actShopkeeperLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right leg
	entity = newEntity(222, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SHOPKEEPER][2][0]; // 0
	entity->focaly = limbs[SHOPKEEPER][2][1]; // 0
	entity->focalz = limbs[SHOPKEEPER][2][2]; // 2
	entity->behavior = &actShopkeeperLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left leg
	entity = newEntity(221, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SHOPKEEPER][3][0]; // 0
	entity->focaly = limbs[SHOPKEEPER][3][1]; // 0
	entity->focalz = limbs[SHOPKEEPER][3][2]; // 2
	entity->behavior = &actShopkeeperLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right arm
	entity = newEntity(220, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SHOPKEEPER][4][0]; // 0
	entity->focaly = limbs[SHOPKEEPER][4][1]; // 0
	entity->focalz = limbs[SHOPKEEPER][4][2]; // 1.5
	entity->behavior = &actShopkeeperLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left arm
	entity = newEntity(219, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SHOPKEEPER][5][0]; // 0
	entity->focaly = limbs[SHOPKEEPER][5][1]; // 0
	entity->focalz = limbs[SHOPKEEPER][5][2]; // 1.5
	entity->behavior = &actShopkeeperLimb;
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
	entity->focalx = limbs[SHOPKEEPER][6][0]; // 1.5
	entity->focaly = limbs[SHOPKEEPER][6][1]; // 0
	entity->focalz = limbs[SHOPKEEPER][6][2]; // -.5
	entity->behavior = &actShopkeeperLimb;
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
	entity->focalx = limbs[SHOPKEEPER][7][0]; // 2
	entity->focaly = limbs[SHOPKEEPER][7][1]; // 0
	entity->focalz = limbs[SHOPKEEPER][7][2]; // 0
	entity->behavior = &actShopkeeperLimb;
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
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SHOPKEEPER][8][0]; // 0
	entity->focaly = limbs[SHOPKEEPER][8][1]; // 0
	entity->focalz = limbs[SHOPKEEPER][8][2]; // 4
	entity->behavior = &actShopkeeperLimb;
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
	entity->focalx = limbs[SHOPKEEPER][9][0]; // 0
	entity->focaly = limbs[SHOPKEEPER][9][1]; // 0
	entity->focalz = limbs[SHOPKEEPER][9][2]; // -1.75
	entity->behavior = &actShopkeeperLimb;
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
	entity->scalex = .99;
	entity->scaley = .99;
	entity->scalez = .99;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SHOPKEEPER][10][0]; // 0
	entity->focaly = limbs[SHOPKEEPER][10][1]; // 0
	entity->focalz = limbs[SHOPKEEPER][10][2]; // .5
	entity->behavior = &actShopkeeperLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);
}

void actShopkeeperLimb(Entity* my)
{
	my->actMonsterLimb();
}

void shopkeeperDie(Entity* my)
{
	int c;
	for ( c = 0; c < 5; c++ )
	{
		Entity* gib = spawnGib(my);
		serverSpawnGibForClient(gib);
	}

	my->spawnBlood();

	my->removeMonsterDeathNodes();

	list_RemoveNode(my->mynode);
	return;
}

#define SHOPKEEPERWALKSPEED .15

void shopkeeperMoveBodyparts(Entity* my, Stat* myStats, double dist)
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
			my->pitch = 0;
		}
	}

	//Move bodyparts
	for (bodypart = 0, node = my->children.first; node != NULL; node = node->next, bodypart++)
	{
		if ( bodypart < LIMB_HUMANOID_TORSO )
		{
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
			my->humanoidAnimateWalk(entity, node, bodypart, SHOPKEEPERWALKSPEED, dist, 0.4);
		}
		else if ( bodypart == LIMB_HUMANOID_LEFTLEG || bodypart == LIMB_HUMANOID_RIGHTARM || bodypart == LIMB_HUMANOID_CLOAK )
		{
			// left leg, right arm, cloak.
			if ( bodypart == LIMB_HUMANOID_RIGHTARM )
			{
				weaponarm = entity;
				if ( my->monsterAttack > 0 )
				{
					my->handleWeaponArmAttack(weaponarm);
				}
			}
			my->humanoidAnimateWalk(entity, node, bodypart, SHOPKEEPERWALKSPEED, dist, 0.4);

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
			case 2:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->breastplate == nullptr )
					{
						entity->sprite = 218;
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
						entity->sprite = 222;
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
						entity->sprite = 221;
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
				node_t* weaponNode = list_Node(&my->children, LIMB_HUMANOID_WEAPON);
				if ( weaponNode )
				{
					Entity* weapon = (Entity*)weaponNode->element;
					if ( MONSTER_ARMBENDED || (weapon->flags[INVISIBLE] && my->monsterState != MONSTER_STATE_ATTACK) )
					{
						// if weapon invisible and I'm not attacking, relax arm.
						entity->focalx = limbs[SHOPKEEPER][4][0]; // 0
						entity->focaly = limbs[SHOPKEEPER][4][1]; // 0
						entity->focalz = limbs[SHOPKEEPER][4][2]; // 1.5
						entity->sprite = 220;
					}
					else
					{
						// else flex arm.
						entity->focalx = limbs[SHOPKEEPER][4][0] + 0.75;
						entity->focaly = limbs[SHOPKEEPER][4][1];
						entity->focalz = limbs[SHOPKEEPER][4][2] - 0.75;
						entity->sprite = 111;
					}
				}
				entity->x += 2.25 * cos(my->yaw + PI / 2) - .20 * cos(my->yaw);
				entity->y += 2.25 * sin(my->yaw + PI / 2) - .20 * sin(my->yaw);
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
				node_t* shieldNode = list_Node(&my->children, 8);
				if ( shieldNode )
				{
					Entity* shield = (Entity*)shieldNode->element;
					if ( shield->flags[INVISIBLE] && (my->monsterState != MONSTER_STATE_ATTACK) )
					{
						// if shield invisible and I'm not attacking, relax arm.
						entity->focalx = limbs[SHOPKEEPER][5][0]; // 0
						entity->focaly = limbs[SHOPKEEPER][5][1]; // 0
						entity->focalz = limbs[SHOPKEEPER][5][2]; // 1.5
						entity->sprite = 219;
					}
					else
					{
						// else flex arm.
						entity->focalx = limbs[SHOPKEEPER][5][0] + 0.75;
						entity->focaly = limbs[SHOPKEEPER][5][1];
						entity->focalz = limbs[SHOPKEEPER][5][2] - 0.75;
						entity->sprite = 112;
					}
				}
				entity->x -= 2.25 * cos(my->yaw + PI / 2) + .20 * cos(my->yaw);
				entity->y -= 2.25 * sin(my->yaw + PI / 2) + .20 * sin(my->yaw);
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
				entity->focalx = limbs[SHOPKEEPER][9][0]; // 0
				entity->focaly = limbs[SHOPKEEPER][9][1]; // 0
				entity->focalz = limbs[SHOPKEEPER][9][2]; // -1.75
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
				entity->focalx = limbs[SHOPKEEPER][10][0]; // 0
				entity->focaly = limbs[SHOPKEEPER][10][1]; // 0
				entity->focalz = limbs[SHOPKEEPER][10][2]; // .5
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
					entity->focalx = limbs[SHOPKEEPER][10][0] + .35; // .35
					entity->focaly = limbs[SHOPKEEPER][10][1] - 2; // -2
					entity->focalz = limbs[SHOPKEEPER][10][2]; // .5
				}
				else
				{
					entity->focalx = limbs[SHOPKEEPER][10][0] + .25; // .25
					entity->focaly = limbs[SHOPKEEPER][10][1] - 2.25; // -2.25
					entity->focalz = limbs[SHOPKEEPER][10][2]; // .5
				}
				break;
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
