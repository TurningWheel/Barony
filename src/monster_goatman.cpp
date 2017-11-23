/*-------------------------------------------------------------------------------

	BARONY
	File: monster_goatman.cpp
	Desc: implements all of the goatman monster's code

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

const int NUM_GOATMAN_POTIONS = 4;
const int NUM_GOATMAN_THROWN_WEAPONS = 2;

const int NUM_GOATMAN_BOSS_GHARBAD_POTIONS = 3;
const int NUM_GOATMAN_BOSS_GHARBAD_THROWN_WEAPONS = 3;

const int BOSS_GHARBAD = 1;

void initGoatman(Entity* my, Stat* myStats)
{
	int c;
	node_t* node;
	int boss = 0;

	//Sprite 463 = Goatman head model
	my->initMonster(463);

	if ( multiplayer != CLIENT )
	{
		//TODO: Update with new goatman sound effects.
		MONSTER_SPOTSND = 265;
		MONSTER_SPOTVAR = 4;
		MONSTER_IDLESND = 260;
		MONSTER_IDLEVAR = 5;
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
				strcpy(myStats->name, "Gharbad");
				boss = BOSS_GHARBAD;
				//TODO: Boss stats

				//Spawn in potions.
				for ( int i = 0; i < rand()%NUM_GOATMAN_BOSS_GHARBAD_POTIONS + 5; ++i )
				{
					switch ( rand()%10 )
					{
						case 0:
						case 1:
						case 2:
						case 3:
						case 4:
						case 5:
						case 6:
						case 7:
						case 8:
							newItem(POTION_BOOZE, EXCELLENT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, &myStats->inventory);
							break;
						case 9:
							newItem(POTION_HEALING, EXCELLENT, 0, 1, rand(), false, &myStats->inventory);
							break;
						default:
							printlog("Tried to spawn goatman boss \"Gharbad\" invalid potion.");
							break;
					}
				}

				newItem(CRYSTAL_SHURIKEN, EXCELLENT, 1 + rand()%1, rand()%NUM_GOATMAN_BOSS_GHARBAD_THROWN_WEAPONS + 2, rand(), true, &myStats->inventory);
			}

			// random effects
			if ( rand() % 8 == 0 )
			{
				my->setEffect(EFF_ASLEEP, true, 1800 + rand() % 1800, false);
			}

			// generates equipment and weapons if available from editor
			createMonsterEquipment(myStats);

			// create any custom inventory items from editor if available
			createCustomInventory(myStats, customItemsToGenerate);

			// count if any custom inventory items from editor
			int customItems = countCustomItems(myStats); //max limit of 6 custom items per entity.

			// count any inventory items set to default in edtior
			int defaultItems = countDefaultItems(myStats);

			bool isShaman = false;
			if ( rand() % 2 )
			{
				isShaman = true;
			}
			else
			{
				myStats->DEX += 3; // more speed for brawlers.
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
					if ( isShaman && rand() % 20 == 0 )
					{
						switch ( rand() % 4 )
						{
							case 0:
								newItem(SPELLBOOK_SLOW, static_cast<Status>(rand() % 3 + DECREPIT), -1 + rand() % 3, 1, rand(), false, &myStats->inventory);
								break;
							case 1:
								newItem(SPELLBOOK_FIREBALL, static_cast<Status>(rand() % 3 + DECREPIT), -1 + rand() % 3, 1, rand(), false, &myStats->inventory);
								break;
							case 2:
								newItem(SPELLBOOK_COLD, static_cast<Status>(rand() % 3 + DECREPIT), -1 + rand() % 3, 1, rand(), false, &myStats->inventory);
								break;
							case 3:
								newItem(SPELLBOOK_FORCEBOLT, static_cast<Status>(rand() % 3 + DECREPIT), -1 + rand() % 3, 1, rand(), false, &myStats->inventory);
								break;
						}
					}
					break;
				default:
					break;
			}


			//Give weapons.
			if ( !boss )
			{
				if ( !isShaman && rand() % 2 == 0 )
				{
					newItem(STEEL_CHAKRAM, static_cast<Status>(rand() % 3 + DECREPIT), 0, rand()%NUM_GOATMAN_THROWN_WEAPONS + 1, rand(), false, &myStats->inventory);
				}
				int numpotions = rand() % NUM_GOATMAN_POTIONS + 2;
				if ( rand() % 5 == 0 )
				{
					int numhealpotion = rand() % 2 + 1;
					newItem(POTION_HEALING, static_cast<Status>(rand() % 3 + DECREPIT), 0, numhealpotion, rand(), false, &myStats->inventory);
					numpotions -= numhealpotion;
				}
				if ( rand() % 4 > 0 )
				{
					// undroppable
					newItem(POTION_BOOZE, static_cast<Status>(rand() % 3 + DECREPIT), 0, numpotions, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, &myStats->inventory);
				}
			}

			if ( isShaman )
			{
				//give shield
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
							myStats->shield = newItem(TOOL_CRYSTALSHARD, static_cast<Status>(rand() % 3 + WORN), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
						case 8:
							myStats->shield = newItem(MIRROR_SHIELD, static_cast<Status>(rand() % 4 + DECREPIT), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
						default:
							myStats->shield = newItem(TOOL_LANTERN, static_cast<Status>(rand() % 3 + WORN), -1 + rand() % 3, 1, rand(), false, nullptr);
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
							break;
						default:
							myStats->cloak = newItem(CLOAK, WORN, -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
					}
				}
				// give helmet
				if ( myStats->helmet == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_HELM] == 1 )
				{
					switch ( rand() % 10 )
					{
						case 0:
						case 1:
							myStats->helmet = newItem(HAT_HOOD, WORN, -1 + rand() % 3, 1, 0, false, nullptr);
							break;
						default:
							myStats->helmet = newItem(HAT_WIZARD, static_cast<Status>(rand() % 3 + WORN), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
					}
				}
				// give armor
				if ( myStats->breastplate == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_ARMOR] == 1 )
				{
					switch ( rand() % 10 )
					{
						case 0:
						case 1:
							myStats->breastplate = newItem(WIZARD_DOUBLET, static_cast<Status>(rand() % 3 + WORN), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
						case 2:
							myStats->breastplate = newItem(LEATHER_BREASTPIECE, static_cast<Status>(rand() % 3 + DECREPIT), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
						default:
							break;
					}
				}
				// give booties
				if ( myStats->shoes == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_BOOTS] == 1 )
				{
					switch ( rand() % 20 )
					{
						case 0:
						case 1:
						case 2:
						case 3:
							myStats->shoes = newItem(IRON_BOOTS, static_cast<Status>(rand() % 3 + DECREPIT), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
						case 19:
							myStats->shoes = newItem(CRYSTAL_BOOTS, static_cast<Status>(rand() % 4 + DECREPIT), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
						default:
							myStats->shoes = newItem(STEEL_BOOTS, static_cast<Status>(rand() % 3 + WORN), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
					}
				}
				// give weapon
				if ( myStats->weapon == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1 )
				{
					switch ( rand() % 20 )
					{
						case 0:
						case 1:
						case 2:
						case 3:
						case 4:
							myStats->weapon = newItem(MAGICSTAFF_COLD, static_cast<Status>(rand() % 3 + DECREPIT), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
						case 5:
						case 6:
						case 7:
						case 8:
							myStats->weapon = newItem(MAGICSTAFF_FIRE, static_cast<Status>(rand() % 3 + DECREPIT), -1 + rand() % 3, 1, rand(), false, nullptr);
							break;
						case 9:
							switch ( rand() % 4 )
							{
								case 0:
									myStats->weapon = newItem(SPELLBOOK_SLOW, static_cast<Status>(rand() % 3 + DECREPIT), -1 + rand() % 3, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
									break;
								case 1:
									myStats->weapon = newItem(SPELLBOOK_FIREBALL, static_cast<Status>(rand() % 3 + DECREPIT), -1 + rand() % 3, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
									break;
								case 2:
									myStats->weapon = newItem(SPELLBOOK_COLD, static_cast<Status>(rand() % 3 + DECREPIT), -1 + rand() % 3, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
									break;
								case 3:
									myStats->weapon = newItem(SPELLBOOK_FORCEBOLT, static_cast<Status>(rand() % 3 + DECREPIT), -1 + rand() % 3, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
									break;
							}
							break;
						default:
							myStats->weapon = newItem(MAGICSTAFF_SLOW, static_cast<Status>(rand() % 3 + WORN), -1 + rand() % 3, 1, 0, false, nullptr);
							break;
					}
				}
			}
			else
			{
				////give shield
				//if ( myStats->shield == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_SHIELD] == 1 )
				//{
				//	switch ( rand() % 20 )
				//	{
				//	case 0:
				//	case 1:
				//	case 2:
				//	case 3:
				//	case 4:
				//	case 5:
				//	case 6:
				//	case 7:
				//		myStats->shield = newItem(TOOL_CRYSTALSHARD, static_cast<Status>(rand() % 3 + WORN), -1 + rand() % 3, 1, rand(), false, nullptr);
				//		break;
				//	case 8:
				//		myStats->shield = newItem(MIRROR_SHIELD, static_cast<Status>(rand() % 4 + DECREPIT), -1 + rand() % 3, 1, rand(), false, nullptr);
				//		break;
				//	default:
				//		myStats->shield = newItem(TOOL_LANTERN, static_cast<Status>(rand() % 3 + WORN), -1 + rand() % 3, 1, rand(), false, nullptr);
				//		break;
				//	}
				//}
				// give cloak
				/*if ( myStats->cloak == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_CLOAK] == 1 )
				{
					switch ( rand() % 10 )
					{
					case 0:
					case 1:
						break;
					default:
						myStats->cloak = newItem(CLOAK, WORN, -1 + rand() % 3, 1, rand(), false, nullptr);
						break;
					}
				}*/
				//// give helmet
				//if ( myStats->helmet == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_HELM] == 1 )
				//{
				//	switch ( rand() % 10 )
				//	{
				//	case 0:
				//	case 1:
				//		myStats->helmet = newItem(HAT_HOOD, WORN, -1 + rand() % 3, 1, 0, false, nullptr);
				//		break;
				//	default:
				//		myStats->helmet = newItem(HAT_WIZARD, static_cast<Status>(rand() % 3 + WORN), -1 + rand() % 3, 1, 0, false, nullptr);
				//		break;
				//	}
				//}
				// give armor
				if ( myStats->breastplate == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_ARMOR] == 1 )
				{
					switch ( rand() % 20 )
					{
					case 0:
					case 1:
					case 2:
						myStats->breastplate = newItem(STEEL_BREASTPIECE, static_cast<Status>(rand() % 4 + DECREPIT), -1 + rand() % 3, 1, rand(), false, nullptr);
						break;
					case 3:
					case 4:
						myStats->breastplate = newItem(LEATHER_BREASTPIECE, static_cast<Status>(rand() % 3 + WORN), -1 + rand() % 3, 1, rand(), false, nullptr);
						break;
					case 5:
					case 6:
						myStats->breastplate = newItem(IRON_BREASTPIECE, static_cast<Status>(rand() % 3 + WORN), -1 + rand() % 3, 1, rand(), false, nullptr);
						break;
					case 19:
						myStats->breastplate = newItem(CRYSTAL_BREASTPIECE, static_cast<Status>(rand() % 3 + WORN), -1 + rand() % 3, 1, rand(), false, nullptr);
						break;
					default:
						break;
					}
				}
				// give booties
				if ( myStats->shoes == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_BOOTS] == 1 )
				{
					switch ( rand() % 20 )
					{
					case 0:
					case 1:
					case 2:
					case 3:
						myStats->shoes = newItem(IRON_BOOTS, static_cast<Status>(rand() % 3 + DECREPIT), -1 + rand() % 3, 1, 0, false, nullptr);
						break;
					case 19:
						myStats->shoes = newItem(CRYSTAL_BOOTS, static_cast<Status>(rand() % 4 + DECREPIT), -1 + rand() % 3, 1, 0, false, nullptr);
						break;
					default:
						myStats->shoes = newItem(STEEL_BOOTS, static_cast<Status>(rand() % 3 + WORN), -1 + rand() % 3, 1, 0, false, nullptr);
						break;
					}
				}
				// give weapon
				if ( myStats->weapon == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1 )
				{
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
						myStats->weapon = newItem(STEEL_AXE, static_cast<Status>(rand() % 3 + WORN), -1 + rand() % 3, 1, 0, false, nullptr);
						break;
					case 18:
						myStats->weapon = newItem(CRYSTAL_BATTLEAXE, static_cast<Status>(rand() % 3 + WORN), -1 + rand() % 3, 1, 0, false, nullptr);
						break;
					case 19:
						myStats->weapon = newItem(CRYSTAL_MACE, static_cast<Status>(rand() % 3 + WORN), -1 + rand() % 3, 1, 0, false, nullptr);
						break;
					default:
						myStats->weapon = newItem(STEEL_MACE, static_cast<Status>(rand() % 3 + WORN), -1 + rand() % 3, 1, 0, false, nullptr);
						break;
					}
				}
			}
		}
	}

	// torso
	Entity* entity = newEntity(466, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->scalex = 1.01;
	entity->scaley = 1.01;
	entity->scalez = 1.01;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[GOATMAN][1][0]; // 0
	entity->focaly = limbs[GOATMAN][1][1]; // 0
	entity->focalz = limbs[GOATMAN][1][2]; // 0
	entity->behavior = &actGoatmanLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// right leg
	entity = newEntity(465, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[GOATMAN][2][0]; // 0
	entity->focaly = limbs[GOATMAN][2][1]; // 0
	entity->focalz = limbs[GOATMAN][2][2]; // 2
	entity->behavior = &actGoatmanLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// left leg
	entity = newEntity(464, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[GOATMAN][3][0]; // 0
	entity->focaly = limbs[GOATMAN][3][1]; // 0
	entity->focalz = limbs[GOATMAN][3][2]; // 2
	entity->behavior = &actGoatmanLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// right arm
	entity = newEntity(461, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[GOATMAN][4][0]; // 0
	entity->focaly = limbs[GOATMAN][4][1]; // 0
	entity->focalz = limbs[GOATMAN][4][2]; // 1.5
	entity->behavior = &actGoatmanLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// left arm
	entity = newEntity(459, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[GOATMAN][5][0]; // 0
	entity->focaly = limbs[GOATMAN][5][1]; // 0
	entity->focalz = limbs[GOATMAN][5][2]; // 1.5
	entity->behavior = &actGoatmanLimb;
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
	entity->focalx = limbs[GOATMAN][6][0]; // 1.5
	entity->focaly = limbs[GOATMAN][6][1]; // 0
	entity->focalz = limbs[GOATMAN][6][2]; // -.5
	entity->behavior = &actGoatmanLimb;
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
	entity->focalx = limbs[GOATMAN][7][0]; // 2
	entity->focaly = limbs[GOATMAN][7][1]; // 0
	entity->focalz = limbs[GOATMAN][7][2]; // 0
	entity->behavior = &actGoatmanLimb;
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
	entity->focalx = limbs[GOATMAN][8][0]; // 0
	entity->focaly = limbs[GOATMAN][8][1]; // 0
	entity->focalz = limbs[GOATMAN][8][2]; // 4
	entity->behavior = &actGoatmanLimb;
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
	entity->focalx = limbs[GOATMAN][9][0]; // 0
	entity->focaly = limbs[GOATMAN][9][1]; // 0
	entity->focalz = limbs[GOATMAN][9][2]; // -2
	entity->behavior = &actGoatmanLimb;
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
	entity->focalx = limbs[GOATMAN][10][0]; // 0
	entity->focaly = limbs[GOATMAN][10][1]; // 0
	entity->focalz = limbs[GOATMAN][10][2]; // .25
	entity->behavior = &actGoatmanLimb;
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

void actGoatmanLimb(Entity* my)
{
	my->actMonsterLimb(true);
}

void goatmanDie(Entity* my)
{
	int c;
	for ( c = 0; c < 5; c++ )
	{
		Entity* gib = spawnGib(my);
		serverSpawnGibForClient(gib);
	}

	my->spawnBlood();

	playSoundEntity(my, 257 + rand() % 3, 128);

	my->removeMonsterDeathNodes();

	list_RemoveNode(my->mynode);
	return;
}

#define GOATMANWALKSPEED .13

void goatmanMoveBodyparts(Entity* my, Stat* myStats, double dist)
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
				if ( bodypart < LIMB_HUMANOID_TORSO )
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
				if ( bodypart < LIMB_HUMANOID_TORSO )
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
			if ( my->monsterAttack != MONSTER_POSE_RANGED_WINDUP3 && bodypart == 1 && multiplayer != CLIENT )
			{
				limbAnimateToLimit(my, ANIMATE_PITCH, 0.1, 0, false, 0.0);
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
			my->humanoidAnimateWalk(entity, node, bodypart, GOATMANWALKSPEED, dist, 0.4);
		}
		else if ( bodypart == LIMB_HUMANOID_LEFTLEG || bodypart == LIMB_HUMANOID_RIGHTARM || bodypart == LIMB_HUMANOID_CLOAK )
		{
			// left leg, right arm, cloak.
			if ( bodypart == LIMB_HUMANOID_RIGHTARM )
			{
				weaponarm = entity;
				if ( MONSTER_ATTACK > 0 )
				{
					if ( my->monsterAttack == MONSTER_POSE_RANGED_WINDUP3 )
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

						if ( my->monsterAttackTime == 0 )
						{
							// init rotations
							weaponarm->pitch = 0;
							my->monsterArmbended = 0;
							my->monsterWeaponYaw = 0;
							weaponarm->roll = 0;
							weaponarm->skill[1] = 0;
							if ( multiplayer != CLIENT )
							{
								myStats->EFFECTS[EFF_PARALYZED] = true;
								myStats->EFFECTS_TIMERS[EFF_PARALYZED] = 40;
							}
						}
						if ( multiplayer != CLIENT )
						{
							// move the head and weapon yaw
							limbAnimateToLimit(my, ANIMATE_PITCH, -0.1, 11 * PI / 6, true, 0.05);
							limbAnimateToLimit(my, ANIMATE_WEAPON_YAW, -0.25, 14 * PI / 8, false, 0.0);
						}
						limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.25, 7 * PI / 4, true, 0.0);
						//limbAnimateToLimit(weaponarm, ANIMATE_ROLL, -0.25, 7 * PI / 4, false, 0.0);

						if ( my->monsterAttackTime >= 4 * ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
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

			my->humanoidAnimateWalk(entity, node, bodypart, GOATMANWALKSPEED, dist, 0.4);

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
						entity->sprite = 466;
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
						entity->sprite = 465;
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
						entity->sprite = 464;
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
				node_t* weaponNode = list_Node(&my->children, 7);
				if ( weaponNode )
				{
					Entity* weapon = (Entity*)weaponNode->element;
					if ( MONSTER_ARMBENDED || (weapon->flags[INVISIBLE] && my->monsterState == MONSTER_STATE_WAIT) )
					{
						// if weapon invisible and I'm not attacking, relax arm.
						entity->focalx = limbs[GOATMAN][4][0]; // 0
						entity->focaly = limbs[GOATMAN][4][1]; // 0
						entity->focalz = limbs[GOATMAN][4][2]; // 2
						entity->sprite = 461;
					}
					else
					{
						// else flex arm.
						entity->focalx = limbs[GOATMAN][4][0] + 0.75;
						entity->focaly = limbs[GOATMAN][4][1];
						entity->focalz = limbs[GOATMAN][4][2] - 0.75;
						entity->sprite = 462;
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
				// left arm
			}
			case LIMB_HUMANOID_LEFTARM:
			{
				node_t* shieldNode = list_Node(&my->children, 8);
				if ( shieldNode )
				{
					Entity* shield = (Entity*)shieldNode->element;
					if ( shield->flags[INVISIBLE] && my->monsterState == MONSTER_STATE_WAIT )
					{
						entity->focalx = limbs[GOATMAN][5][0]; // 0
						entity->focaly = limbs[GOATMAN][5][1]; // 0
						entity->focalz = limbs[GOATMAN][5][2]; // 2
						entity->sprite = 459;
					}
					else
					{
						entity->focalx = limbs[GOATMAN][5][0] + 0.75;
						entity->focaly = limbs[GOATMAN][5][1];
						entity->focalz = limbs[GOATMAN][5][2] - 0.75;
						entity->sprite = 460;
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
				entity->focalx = limbs[GOATMAN][9][0]; // 0
				entity->focaly = limbs[GOATMAN][9][1]; // 0
				entity->focalz = limbs[GOATMAN][9][2]; // -2
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
				if ( entity->sprite != items[STEEL_HELM].index )
				{
					if ( entity->sprite == items[HAT_PHRYGIAN].index )
					{
						entity->focalx = limbs[GOATMAN][9][0] - .5;
						entity->focaly = limbs[GOATMAN][9][1] - 3.55;
						entity->focalz = limbs[GOATMAN][9][2] + 2.5;
						entity->roll = PI / 2;
					}
					else if ( entity->sprite >= items[HAT_HOOD].index && entity->sprite < items[HAT_HOOD].index + items[HAT_HOOD].variations )
					{
						entity->focalx = limbs[GOATMAN][9][0] - .5;
						entity->focaly = limbs[GOATMAN][9][1] - 2.75;
						entity->focalz = limbs[GOATMAN][9][2] + 2.5;
						entity->roll = PI / 2;
					}
					else if ( entity->sprite == items[HAT_WIZARD].index )
					{
						entity->focalx = limbs[GOATMAN][9][0];
						entity->focaly = limbs[GOATMAN][9][1] - 5;
						entity->focalz = limbs[GOATMAN][9][2] + 2.5;
						entity->roll = PI / 2;
					}
					else if ( entity->sprite == items[HAT_JESTER].index )
					{
						entity->focalx = limbs[GOATMAN][9][0];
						entity->focaly = limbs[GOATMAN][9][1] - 5;
						entity->focalz = limbs[GOATMAN][9][2] + 2.5;
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
				entity->focalx = limbs[GOATMAN][10][0]; // 0
				entity->focaly = limbs[GOATMAN][10][1]; // 0
				entity->focalz = limbs[GOATMAN][10][2]; // .25
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
					entity->focalx = limbs[GOATMAN][10][0] + .35; // .35
					entity->focaly = limbs[GOATMAN][10][1] - 2; // -2
					entity->focalz = limbs[GOATMAN][10][2]; // .25
				}
				else
				{
					entity->focalx = limbs[GOATMAN][10][0] + .25; // .25
					entity->focaly = limbs[GOATMAN][10][1] - 2.25; // -2.25
					entity->focalz = limbs[GOATMAN][10][2]; // .25
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

void Entity::goatmanChooseWeapon(const Entity* target, double dist)
{
	if ( monsterSpecialState != 0 )
	{
		//Holding a weapon assigned from the special attack. Don't switch weapons.
		//messagePlayer()
		return;
	}

	//TODO: I don't like this function getting called every frame. Find a better place to put it.
	//Although if I do that, can't do this dirty little hack for the goatman's special...

	//TODO: If applying attack animations that will involve holding a potion for several frames while this code has a chance to run, do a check here to cancel the function if holding a potion.

	Stat *myStats = getStats();
	if ( !myStats )
	{
		return;
	}

	if ( myStats->weapon && (itemCategory(myStats->weapon) == SPELLBOOK) )
	{
		return;
	}

	int specialRoll = -1;
	bool usePotionSpecial = false;

	/*
	 * For the goatman's special:
	 * * If specialRoll == 0, want to use a booze or healing potion (prioritize healing potion if damaged enough).
	 * * If no have potion, try to use THROWN in melee.
	 * * If in melee, if potion is not a healing potion, check if have any THROWN and then 50% chance to use those instead.
	 */

	node_t* hasPotion = nullptr;
	bool isHealingPotion = false;

	if ( monsterSpecialTimer == 0 && (ticks % 10 == 0) && monsterAttack == 0 )
	{
		//messagePlayer(clientnum, "Cooldown done!");
		specialRoll = rand()%10;

		if ( specialRoll == 0 )
		{
			if ( myStats->HP <= myStats->MAXHP / 3 * 2 )
			{
				//Try to get a health potion.
				hasPotion = itemNodeInInventory(myStats, POTION_EXTRAHEALING, static_cast<Category>(-1));
				if ( !hasPotion )
				{
					hasPotion = itemNodeInInventory(myStats, POTION_HEALING, static_cast<Category>(-1));
					if ( hasPotion )
					{
						//Equip and chuck it now.
						bool swapped = swapMonsterWeaponWithInventoryItem(this, myStats, hasPotion, false, false);
						if ( !swapped )
						{
							printlog("Error in Entity::goatmanChooseWeapon(): failed to swap healing potion into hand!");
							//Don't return, want to try equipping either a potion of booze, or one of the other weapon routes (e.h. a THROWN special if in melee or just an axe if worst comes to worst).
						}
						else
						{
							monsterSpecialState = GOATMAN_POTION;
							//monsterHitTime = 2 * HITRATE;
							return;
						}
					}
				}
				else
				{
					//Equip and chuck it now.
					bool swapped = swapMonsterWeaponWithInventoryItem(this, myStats, hasPotion, false, false);
					if ( !swapped )
					{
						printlog("Error in Entity::goatmanChooseWeapon(): failed to swap healing potion into hand!");
						//Don't return, want to try equipping either a potion of booze, or one of the other weapon routes (e.h. a THROWN special if in melee or just an axe if worst comes to worst).
					}
					else
					{
						monsterSpecialState = GOATMAN_POTION;
						//monsterHitTime = 2 * HITRATE;
						return;
					}
				}
			}

			if ( !hasPotion )
			{
				//Couldn't find a healing potion? Try for a potion of booze.
				hasPotion = itemNodeInInventory(myStats, POTION_BOOZE, static_cast<Category>(-1));
				if ( hasPotion )
				{
					//Equip and chuck it now.
					bool swapped = swapMonsterWeaponWithInventoryItem(this, myStats, hasPotion, false, false);
					if ( !swapped )
					{
						printlog("Error in Entity::goatmanChooseWeapon(): failed to swap healing potion into hand!");
						//Don't return, want to try equipping either a potion of booze, or one of the other weapon routes (e.h. a THROWN special if in melee or just an axe if worst comes to worst).
					}
					else
					{
						monsterSpecialState = GOATMAN_POTION;
						//monsterHitTime = 2 * HITRATE;
						return;
					}
				}
			}
		}
	}

	bool inMeleeRange = monsterInMeleeRange(target, dist);

	if ( inMeleeRange )
	{
		if ( monsterSpecialTimer == 0 && (ticks % 10 == 0) && monsterAttack == 0 && specialRoll == 0 )
		{
			bool tryChakram = true;
			if ( hasPotion && rand()%10 )
			{
				tryChakram = false;
			}

			if ( tryChakram )
			{
				//Grab a chakram instead.
				node_t* thrownNode = itemNodeInInventory(myStats, static_cast<ItemType>(-1), THROWN);
				if ( thrownNode )
				{
					bool swapped = swapMonsterWeaponWithInventoryItem(this, myStats, thrownNode, false, false);
					if ( !swapped )
					{
						printlog("Error in Entity::goatmanChooseWeapon(): failed to swap THROWN into hand! Cursed? (%d)", myStats->weapon->beatitude);
						//Don't return, make sure holding a melee weapon at least.
					}
					else
					{
						monsterSpecialState = GOATMAN_THROW;
						return;
					}
				}
			}
		}

		//Switch to a melee weapon if not already wielding one. Unless monster special state is overriding the AI.
		if ( !myStats->weapon || !isMeleeWeapon(*myStats->weapon) )
		{
			node_t* weaponNode = getMeleeWeaponItemNodeInInventory(myStats);
			if ( !weaponNode )
			{
				if ( myStats->weapon && myStats->weapon->type == MAGICSTAFF_SLOW )
				{
					monsterUnequipSlotFromCategory(myStats, &myStats->weapon, MAGICSTAFF);
				}
				return; //Resort to fists.
			}

			bool swapped = swapMonsterWeaponWithInventoryItem(this, myStats, weaponNode, false, false);
			if ( !swapped )
			{
				printlog("Error in Entity::goatmanChooseWeapon(): failed to swap melee weapon into hand! Cursed? (%d)", myStats->weapon->beatitude);
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

	//if ( hasPotion )
	//{
	//	//Try to equip the potion first. If fails, then equip normal ranged.
	//	bool swapped = swapMonsterWeaponWithInventoryItem(this, myStats, hasPotion, false, false);
	//	if ( !swapped )
	//	{
	//		printlog("Error in Entity::goatmanChooseWeapon(): failed to swap non-healing potion into hand! (non-melee block) Cursed? (%d)", myStats->weapon->beatitude);
	//	}
	//	else
	//	{
	//		monsterSpecialState = GOATMAN_POTION;
	//		return;
	//	}
	//}

	//Switch to a thrown weapon or a ranged weapon. Potions are reserved as a special attack.
	if ( !myStats->weapon || isMeleeWeapon(*myStats->weapon) )
	{
		//First search the inventory for a THROWN weapon.
		node_t *weaponNode = nullptr;
		if ( monsterSpecialTimer == 0 && (ticks % 10 == 0) && monsterAttack == 0 && rand() % 10 == 0 )
		{
			weaponNode = itemNodeInInventory(myStats, static_cast<ItemType>(-1), THROWN);
			if ( weaponNode )
			{
				if ( swapMonsterWeaponWithInventoryItem(this, myStats, weaponNode, false, false) )
				{
					monsterSpecialState = GOATMAN_THROW;
					return;
				}
			}
		}
		if ( !weaponNode )
		{
			//If couldn't find any, search the inventory for a ranged weapon.
			weaponNode = getRangedWeaponItemNodeInInventory(myStats, true);
		}

		bool swapped = swapMonsterWeaponWithInventoryItem(this, myStats, weaponNode, false, false);
		return;
	}

	return;
}

bool Entity::goatmanCanWieldItem(const Item& item) const
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
			switch ( item.type )
			{
				case POTION_BOOZE:
					return true;
				case POTION_HEALING:
					return true;
				default:
					return false;
			}
		case THROWN:
			return true;
		case ARMOR:
			{ //Little baby compiler stop whining, wah wah.
				int equipType = checkEquipType(&item);
				if ( equipType == TYPE_HAT || equipType == TYPE_HELM )
				{
					return false; //No can wear hats, because horns.
				}
				return true; //Can wear all other armor.
			}
		default:
			return false;
	}

	return false;
}





