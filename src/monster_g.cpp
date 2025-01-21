/*-------------------------------------------------------------------------------

	BARONY
	File: monster_gnome.cpp
	Desc: implements all of the gnome monster's code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "items.hpp"
#include "monster.hpp"
#include "engine/audio/sound.hpp"
#include "book.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "prng.hpp"
#include "mod_tools.hpp"

real_t getNormalHeightMonsterG(Entity& my)
{
	return 1.5;
}

void initMonsterG(Entity* my, Stat* myStats)
{
	int c;
	node_t* node;

	my->flags[BURNABLE] = true;
	my->initMonster(1569);
	my->z = getNormalHeightMonsterG(*my);

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 220;
		MONSTER_SPOTVAR = 5;
		MONSTER_IDLESND = 217;
		MONSTER_IDLEVAR = 3;
	}
	if ( multiplayer != CLIENT && !MONSTER_INIT )
	{
		auto& rng = my->entity_rng ? *my->entity_rng : local_rng;

		if ( myStats != nullptr )
		{
			if ( myStats->sex == FEMALE )
			{
				my->sprite = 1570;
			}
			if ( !myStats->leader_uid )
			{
				myStats->leader_uid = 0;
			}

			// apply random stat increases if set in stat_shared.cpp or editor
			setRandomMonsterStats(myStats, rng);

			// generate 6 items max, less if there are any forced items from boss variants
			int customItemsToGenerate = ITEM_CUSTOM_SLOT_LIMIT;

			// boss variants

			// random effects
			/*if ( rng.rand() % 8 == 0 )
			{
				myStats->EFFECTS[EFF_ASLEEP] = true;
				myStats->EFFECTS_TIMERS[EFF_ASLEEP] = 1800 + rng.rand() % 1800;
			}*/

			// generates equipment and weapons if available from editor
			createMonsterEquipment(myStats, rng);

			// create any custom inventory items from editor if available
			createCustomInventory(myStats, customItemsToGenerate, rng);

			// count if any custom inventory items from editor
			int customItems = countCustomItems(myStats); //max limit of 6 custom items per entity.

			// count any inventory items set to default in edtior
			int defaultItems = countDefaultItems(myStats);

			my->setHardcoreStats(*myStats);

			// generate the default inventory items for the monster, provided the editor sprite allowed enough default slots
			//if ( gnomeVariant == GNOME_DEFAULT )
			//{
			//	switch ( defaultItems )
			//	{
			//		case 6:
			//		case 5:
			//		case 4:
			//		case 3:
			//			if ( rng.rand() % 50 == 0 )
			//			{
			//				if ( rng.rand() % 2 == 0 )
			//				{
			//					newItem(ENCHANTED_FEATHER, WORN, 0, 1, (2 * (ENCHANTED_FEATHER_MAX_DURABILITY - 1)) / 4, false, &myStats->inventory);
			//				}
			//				else
			//				{
			//					newItem(READABLE_BOOK, EXCELLENT, 0, 1, getBook("Winny's Report"), false, &myStats->inventory);
			//				}
			//			}
			//		case 2:
			//			if ( rng.rand() % 10 == 0 )
			//			{
			//				if ( rng.rand() % 2 == 0 )
			//				{
			//					newItem(MASK_PIPE, SERVICABLE, -1 + rng.rand() % 3, 1, rng.rand(), false, &myStats->inventory);
			//				}
			//				else
			//				{
			//					int i = 1 + rng.rand() % 4;
			//					for ( c = 0; c < i; c++ )
			//					{
			//						newItem(static_cast<ItemType>(GEM_GARNET + rng.rand() % 15), static_cast<Status>(1 + rng.rand() % 4), 0, 1, rng.rand(), false, &myStats->inventory);
			//					}
			//				}
			//			}
			//		case 1:
			//			if ( rng.rand() % 3 == 0 )
			//			{
			//				newItem(FOOD_FISH, EXCELLENT, 0, 1, rng.rand(), false, &myStats->inventory);
			//			}
			//			break;
			//		default:
			//			break;
			//	}
			//}
			//else
			//{
			//	switch ( defaultItems )
			//	{
			//	case 6:
			//	case 5:
			//	case 4:
			//	case 3:
			//		if ( rng.rand() % 100 == 0 )
			//		{
			//			newItem(ENCHANTED_FEATHER, WORN, 0, 1, (2 * (ENCHANTED_FEATHER_MAX_DURABILITY - 1)) / 4, false, &myStats->inventory);
			//		}
			//		else
			//		{
			//			if ( (rng.rand() % 4 > 0 && gnomeVariant == GNOME_THIEF_RANGED)
			//				|| (rng.rand() % 2 == 0 && gnomeVariant == GNOME_THIEF_MELEE) )
			//			{
			//				if ( rng.rand() % 2 == 0 )
			//				{
			//					auto item = newItem(TOOL_BEARTRAP, DECREPIT, -1, 1, rng.rand(), false, &myStats->inventory);
			//					if ( item )
			//					{
			//						item->isDroppable = false;
			//					}
			//				}
			//				else
			//				{
			//					auto item = newItem(static_cast<ItemType>(TOOL_BOMB + rng.rand() % 3), EXCELLENT, -1, 1, rng.rand(), false, &myStats->inventory);
			//					if ( item )
			//					{
			//						item->isDroppable = false;
			//					}
			//				}
			//			}
			//		}
			//	case 2:
			//		if ( rng.rand() % 20 == 0 )
			//		{
			//			if ( rng.rand() % 2 == 0 )
			//			{
			//				newItem(static_cast<ItemType>(GEM_GLASS), static_cast<Status>(1 + rng.rand() % 4), 0, 1, rng.rand(), false, &myStats->inventory);
			//			}
			//			else
			//			{
			//				newItem(static_cast<ItemType>(GEM_GARNET + rng.rand() % 15), static_cast<Status>(1 + rng.rand() % 4), 0, 1, rng.rand(), false, &myStats->inventory);
			//			}
			//		}
			//	case 1:
			//		if ( rng.rand() % 10 == 0 )
			//		{
			//			if ( rng.rand() % 4 == 0 )
			//			{
			//				newItem(FOOD_CREAMPIE, static_cast<Status>(3 + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, &myStats->inventory);
			//			}
			//			else
			//			{
			//				newItem(TOOL_LOCKPICK, EXCELLENT, 0, 1, rng.rand(), false, &myStats->inventory);
			//			}
			//		}
			//		break;
			//	default:
			//		break;
			//	}
			//}

			////give weapon
			//if ( myStats->weapon == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1 )
			//{
			//	if ( gnomeVariant == GNOME_DEFAULT )
			//	{
			//		switch ( rng.rand() % 10 )
			//		{
			//			case 0:
			//			case 1:
			//			case 2:
			//			case 3:
			//			case 4:
			//				myStats->weapon = newItem(TOOL_PICKAXE, EXCELLENT, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//				break;
			//			case 5:
			//			case 6:
			//			case 7:
			//			case 8:
			//			case 9:
			//				myStats->weapon = newItem(MAGICSTAFF_LIGHTNING, EXCELLENT, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//				break;
			//		}
			//	}
			//	else if ( gnomeVariant == GNOME_THIEF_RANGED )
			//	{
			//		if ( rng.rand() % 2 == 0 )
			//		{
			//			myStats->weapon = newItem(SHORTBOW, SERVICABLE, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//		}
			//		else
			//		{
			//			myStats->weapon = newItem(CROSSBOW, SERVICABLE, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//		}
			//	}
			//	else if ( gnomeVariant == GNOME_THIEF_MELEE )
			//	{
			//		if ( rng.rand() % 2 == 0 )
			//		{
			//			myStats->weapon = newItem(STEEL_SWORD, SERVICABLE, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//		}
			//		else
			//		{
			//			myStats->weapon = newItem(STEEL_MACE, SERVICABLE, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//		}
			//	}
			//}

			////give shield
			//if ( myStats->shield == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_SHIELD] == 1 )
			//{
			//	if ( gnomeVariant == GNOME_DEFAULT )
			//	{
			//		switch ( rng.rand() % 10 )
			//		{
			//		case 0:
			//		case 1:
			//			myStats->shield = newItem(TOOL_LANTERN, EXCELLENT, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//			break;
			//		case 2:
			//		case 3:
			//		case 4:
			//		case 5:
			//		case 6:
			//			break;
			//		case 7:
			//		case 8:
			//		case 9:
			//			myStats->shield = newItem(WOODEN_SHIELD, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//			break;
			//		}
			//	}
			//	else if ( gnomeVariant == GNOME_THIEF_RANGED )
			//	{
			//		if ( myStats->weapon && isRangedWeapon(*myStats->weapon) )
			//		{
			//			my->monsterGenerateQuiverItem(myStats);
			//			if ( myStats->shield )
			//			{
			//				myStats->shield->isDroppable = (rng.rand() % 2 == 0) ? true : false;
			//			}
			//		}
			//		else
			//		{
			//			/*if ( rng.rand() % 10 <= 4 )
			//			{
			//				myStats->shield = newItem(WOODEN_SHIELD, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//			}*/
			//		}
			//	}
			//	/*else if ( gnomeVariant == GNOME_THIEF_MELEE )
			//	{
			//		if ( rng.rand() % 10 <= 3 )
			//		{
			//			if ( rng.rand() % 2 == 0 )
			//			{
			//				myStats->shield = newItem(IRON_SHIELD, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//			}
			//			else
			//			{
			//				myStats->shield = newItem(WOODEN_SHIELD, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//			}
			//		}
			//	}*/
			//}

			//// give cloak
			//if ( myStats->cloak == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_CLOAK] == 1 )
			//{
			//	switch ( rng.rand() % 10 )
			//	{
			//		case 0:
			//		case 1:
			//		case 2:
			//		case 3:
			//		case 4:
			//		case 5:
			//			break;
			//		case 6:
			//		case 7:
			//		case 8:
			//		case 9:
			//			myStats->cloak = newItem(CLOAK, SERVICABLE, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//			if ( gnomeVariant == GNOME_THIEF_MELEE || gnomeVariant == GNOME_THIEF_RANGED )
			//			{
			//				myStats->cloak->isDroppable = (rng.rand() % 4 == 0) ? true : false;
			//			}
			//			break;
			//	}
			//}

			//if ( myStats->shoes == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_BOOTS] == 1 )
			//{
			//	if ( gnomeVariant == GNOME_THIEF_MELEE )
			//	{
			//		myStats->shoes = newItem(SUEDE_BOOTS, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//		myStats->shoes->isDroppable = (rng.rand() % 8 == 0) ? true : false;
			//	}
			//}

			//if ( myStats->gloves == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_GLOVES] == 1 )
			//{
			//	if ( gnomeVariant == GNOME_THIEF_RANGED )
			//	{
			//		myStats->gloves = newItem(SUEDE_GLOVES, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//		myStats->gloves->isDroppable = (rng.rand() % 8 == 0) ? true : false;
			//	}
			//}

			//if ( myStats->helmet == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_HELM] == 1 )
			//{
			//	if ( gnomeVariant == GNOME_THIEF_RANGED )
			//	{
			//		if ( myStats->leader_uid != 0 )
			//		{
			//			myStats->helmet = newItem(HAT_HOOD_WHISPERS, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, 0, false, nullptr);
			//			myStats->helmet->isDroppable = (rng.rand() % 2 == 0) ? true : false;
			//		}
			//		else
			//		{
			//			// leader has the bycocket
			//			myStats->helmet = newItem(HAT_BYCOCKET, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, 0, false, nullptr);
			//			myStats->helmet->isDroppable = (rng.rand() % 2 == 0) ? true : false;
			//		}
			//	}
			//	else if ( gnomeVariant == GNOME_THIEF_MELEE )
			//	{
			//		if ( rng.rand() % 4 == 0 )
			//		{
			//			myStats->helmet = newItem(HAT_BANDANA, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//			myStats->helmet->isDroppable = (rng.rand() % 2 == 0) ? true : false;
			//		}
			//		else
			//		{
			//			if ( rng.rand() % 2 == 0 )
			//			{
			//				myStats->helmet = newItem(HAT_HOOD, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, 2, false, nullptr);
			//				myStats->helmet->isDroppable = (rng.rand() % 8 == 0) ? true : false;
			//			}
			//			else
			//			{
			//				myStats->helmet = newItem(HAT_HOOD_ASSASSIN, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//				myStats->helmet->isDroppable = (rng.rand() % 2 == 0) ? true : false;
			//			}
			//		}
			//	}
			//}

			//if ( myStats->mask == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_MASK] == 1 )
			//{
			//	if ( gnomeVariant == GNOME_THIEF_RANGED )
			//	{
			//		switch ( rng.rand() % 10 )
			//		{
			//			case 0:
			//				myStats->mask = newItem(MASK_PIPE, SERVICABLE, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//				break;
			//			case 1:
			//			case 2:
			//				myStats->mask = newItem(TOOL_GLASSES, SERVICABLE, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//				break;
			//			case 3:
			//			case 4:
			//			case 5:
			//				myStats->mask = newItem(MASK_EYEPATCH, SERVICABLE, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//				break;
			//			case 6:
			//			case 7:
			//			case 8:
			//			case 9:
			//				myStats->mask = newItem(MASK_BANDIT, SERVICABLE, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//				myStats->mask->isDroppable = (rng.rand() % 8 == 0) ? true : false;
			//				break;
			//			default:
			//				break;
			//		}
			//	}
			//	else if ( gnomeVariant == GNOME_THIEF_MELEE )
			//	{
			//		switch ( rng.rand() % 10 )
			//		{
			//		case 0:
			//			myStats->mask = newItem(MASK_PIPE, SERVICABLE, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//			break;
			//		case 1:
			//		case 2:
			//		case 3:
			//			myStats->mask = newItem(MASK_MOUTHKNIFE, SERVICABLE, 0, 1, rng.rand(), false, nullptr);
			//			break;
			//		case 4:
			//		case 5:
			//		case 6:
			//		case 7:
			//		case 8:
			//		case 9:
			//			myStats->mask = newItem(MASK_BANDIT, SERVICABLE, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//			myStats->mask->isDroppable = (rng.rand() % 8 == 0) ? true : false;
			//			break;
			//		default:
			//			break;
			//		}
			//	}
			//}
		}
	}

	// torso
	Entity* entity = newEntity(my->sprite == 1569 ? 1583 : 1584, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[MONSTER_G][1][0]; // 0
	entity->focaly = limbs[MONSTER_G][1][1]; // 0
	entity->focalz = limbs[MONSTER_G][1][2]; // 0
	entity->behavior = &actGnomeLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right leg
	entity = newEntity(my->sprite == 1569 ? 1580 : 1582, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[MONSTER_G][2][0]; // .25
	entity->focaly = limbs[MONSTER_G][2][1]; // 0
	entity->focalz = limbs[MONSTER_G][2][2]; // 1.5
	entity->behavior = &actGnomeLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left leg
	entity = newEntity(my->sprite == 1569 ? 1579 : 1581, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[MONSTER_G][3][0]; // .25
	entity->focaly = limbs[MONSTER_G][3][1]; // 0
	entity->focalz = limbs[MONSTER_G][3][2]; // 1.5
	entity->behavior = &actGnomeLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right arm
	entity = newEntity(my->sprite == 1569 ? 1573 : 1577, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[MONSTER_G][4][0]; // 0
	entity->focaly = limbs[MONSTER_G][4][1]; // 0
	entity->focalz = limbs[MONSTER_G][4][2]; // 2
	entity->behavior = &actGnomeLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left arm
	entity = newEntity(my->sprite == 1569 ? 1571 : 1575, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[MONSTER_G][5][0]; // 0
	entity->focaly = limbs[MONSTER_G][5][1]; // 0
	entity->focalz = limbs[MONSTER_G][5][2]; // 2
	entity->behavior = &actGnomeLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// world weapon
	entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[INVISIBLE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->noColorChangeAllyLimb = 1.0;
	entity->focalx = limbs[MONSTER_G][6][0]; // 2
	entity->focaly = limbs[MONSTER_G][6][1]; // 0
	entity->focalz = limbs[MONSTER_G][6][2]; // -.5
	entity->behavior = &actGnomeLimb;
	entity->parent = my->getUID();
	entity->pitch = .25;
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// shield
	entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[INVISIBLE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->noColorChangeAllyLimb = 1.0;
	entity->focalx = limbs[MONSTER_G][7][0]; // 0
	entity->focaly = limbs[MONSTER_G][7][1]; // 0
	entity->focalz = limbs[MONSTER_G][7][2]; // 1.5
	entity->behavior = &actGnomeLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// cloak
	entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
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
	entity->noColorChangeAllyLimb = 1.0;
	entity->focalx = limbs[MONSTER_G][8][0]; // 0
	entity->focaly = limbs[MONSTER_G][8][1]; // 0
	entity->focalz = limbs[MONSTER_G][8][2]; // 4
	entity->behavior = &actGnomeLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// helmet
	entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->scalex = 1.01;
	entity->scaley = 1.01;
	entity->scalez = 1.01;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->noColorChangeAllyLimb = 1.0;
	entity->focalx = limbs[MONSTER_G][9][0]; // 0
	entity->focaly = limbs[MONSTER_G][9][1]; // 0
	entity->focalz = limbs[MONSTER_G][9][2]; // -2
	entity->behavior = &actGnomeLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// mask
	entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->noColorChangeAllyLimb = 1.0;
	entity->focalx = limbs[MONSTER_G][10][0]; // 0
	entity->focaly = limbs[MONSTER_G][10][1]; // 0
	entity->focalz = limbs[MONSTER_G][10][2]; // .25
	entity->behavior = &actGnomeLimb;
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

void actMonsterGLimb(Entity* my)
{
	my->actMonsterLimb(true);
}

void monsterGDie(Entity* my)
{
	if ( !my ) { return; }
	for ( int c = 0; c < 10; c++ )
	{
		Entity* entity = spawnGib(my);
		if ( entity )
		{
		    if (c < 6) {
		        entity->sprite = 295 + c;
		        entity->skill[5] = 1; // poof
		    }
			serverSpawnGibForClient(entity);
		}
	}

	playSoundEntity(my, 225 + local_rng.rand() % 4, 128);

	my->spawnBlood();

	my->removeMonsterDeathNodes();

	list_RemoveNode(my->mynode);
	return;
}

#define MONSTER_GWALKSPEED .13

void monsterGMoveBodyparts(Entity* my, Stat* myStats, double dist)
{
	node_t* node;
	Entity* entity = nullptr;
	Entity* entity2 = nullptr;
	Entity* rightbody = nullptr;
	Entity* weaponarm = nullptr;
	int bodypart;
	bool wearingring = false;

	my->focalx = limbs[MONSTER_G][0][0];
	my->focaly = limbs[MONSTER_G][0][1];
	my->focalz = limbs[MONSTER_G][0][2];
	/*if ( my->sprite == 1430 )
	{
		my->focalx -= 0.26;
	}
	else if ( my->sprite == 295 )
	{
		my->focalx -= 0.25;
		my->focalz -= 0.25;
	}*/

	bool debugModel = monsterDebugModels(my, &dist);

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
					++bodypart;
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
			my->z = 4;
			my->pitch = PI / 4;
		}
		else
		{
			my->z = getNormalHeightMonsterG(*my);
			if ( my->monsterAttack == 0 )
			{
				if ( debugModel )
				{
					my->pitch = my->fskill[0];
					if ( my->fskill[1] > 0.0 )
					{
						my->fskill[1] = std::max(0.0, my->fskill[1] - 0.05);
						my->z += -3.0 * sqrt(sin(PI * my->fskill[1]));
					}
				}
				else
				{
					my->pitch = 0;
				}
			}
		}
	}

	Entity* shieldarm = nullptr;
	Entity* helmet = nullptr;

	//Move bodyparts
	for (bodypart = 0, node = my->children.first; node != nullptr; node = node->next, bodypart++)
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
			my->humanoidAnimateWalk(entity, node, bodypart, MONSTER_GWALKSPEED, dist, 0.4);
		}
		else if ( bodypart == LIMB_HUMANOID_LEFTLEG || bodypart == LIMB_HUMANOID_RIGHTARM || bodypart == LIMB_HUMANOID_CLOAK )
		{
			// left leg, right arm, cloak.
			if ( bodypart == LIMB_HUMANOID_RIGHTARM )
			{
				weaponarm = entity;
				if ( my->monsterAttack > 0 )
				{
					my->handleWeaponArmAttack(entity);
				}
			}
			else if ( bodypart == LIMB_HUMANOID_CLOAK )
			{
				entity->pitch = entity->fskill[0];
			}

			my->humanoidAnimateWalk(entity, node, bodypart, MONSTER_GWALKSPEED, dist, 0.4);

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
				entity->focalx = limbs[MONSTER_G][1][0];
				entity->focaly = limbs[MONSTER_G][1][1];
				entity->focalz = limbs[MONSTER_G][1][2];
				if ( multiplayer != CLIENT )
				{
					if ( myStats->breastplate == nullptr )
					{
						entity->sprite = my->sprite == 1569 ? 1583 : 1584;
					}
					else
					{
						entity->sprite = itemModel(myStats->breastplate, true);
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->ticks >= *cvar_entity_bodypart_sync_tick )
						{
							bool updateBodypart = false;
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								updateBodypart = true;
							}
							if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
							{
								updateBodypart = true;
							}
							if ( updateBodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
						}
					}
				}
				my->setHumanoidLimbOffset(entity, MONSTER_G, LIMB_HUMANOID_TORSO);
				break;
			// right leg
			case LIMB_HUMANOID_RIGHTLEG:
				entity->focalx = limbs[MONSTER_G][2][0];
				entity->focaly = limbs[MONSTER_G][2][1];
				entity->focalz = limbs[MONSTER_G][2][2];
				if ( multiplayer != CLIENT )
				{
					if ( myStats->shoes == nullptr )
					{
						entity->sprite = my->sprite == 1569 ? 1580 : 1582;
					}
					else
					{
						my->setBootSprite(entity, SPRITE_BOOT_RIGHT_OFFSET);
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->ticks >= *cvar_entity_bodypart_sync_tick )
						{
							bool updateBodypart = false;
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								updateBodypart = true;
							}
							if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
							{
								updateBodypart = true;
							}
							if ( updateBodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
						}
					}
				}
				my->setHumanoidLimbOffset(entity, MONSTER_G, LIMB_HUMANOID_RIGHTLEG);
				break;
			// left leg
			case LIMB_HUMANOID_LEFTLEG:
				entity->focalx = limbs[MONSTER_G][3][0];
				entity->focaly = limbs[MONSTER_G][3][1];
				entity->focalz = limbs[MONSTER_G][3][2];
				if ( multiplayer != CLIENT )
				{
					if ( myStats->shoes == nullptr )
					{
						entity->sprite = my->sprite == 1569 ? 1579 : 1581;
					}
					else
					{
						my->setBootSprite(entity, SPRITE_BOOT_LEFT_OFFSET);
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->ticks >= *cvar_entity_bodypart_sync_tick )
						{
							bool updateBodypart = false;
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								updateBodypart = true;
							}
							if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
							{
								updateBodypart = true;
							}
							if ( updateBodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
						}
					}
				}
				my->setHumanoidLimbOffset(entity, MONSTER_G, LIMB_HUMANOID_LEFTLEG);
				break;
			// right arm
			case LIMB_HUMANOID_RIGHTARM:
			{
				if ( multiplayer != CLIENT )
				{
					if ( myStats->gloves == nullptr )
					{
						entity->sprite = my->sprite == 1569 ? 1573 : 1577;
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
						if ( entity->ticks >= *cvar_entity_bodypart_sync_tick )
						{
							bool updateBodypart = false;
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								updateBodypart = true;
							}
							if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
							{
								updateBodypart = true;
							}
							if ( updateBodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
						}
					}
				}

				if ( multiplayer == CLIENT )
				{
					if ( entity->skill[7] == 0 )
					{
						if ( entity->sprite == 1573 || entity->sprite == 1577 )
						{
							// these are the default arms.
							// chances are they may be wrong if sent by the server, 
						}
						else
						{
							// otherwise we're being sent gloves armor etc so it's probably right.
							entity->skill[7] = entity->sprite;
						}
					}
					if ( entity->skill[7] == 0 )
					{
						// we set this ourselves until proper initialisation.
						entity->sprite = my->sprite == 1569 ? 1573 : 1577;
					}
					else
					{
						entity->sprite = entity->skill[7];
					}
				}

				node_t* weaponNode = list_Node(&my->children, 7);
				bool bentArm = false;
				if ( weaponNode )
				{
					Entity* weapon = (Entity*)weaponNode->element;
					if ( my->monsterArmbended || (weapon->flags[INVISIBLE] && my->monsterState == MONSTER_STATE_WAIT) )
					{
						entity->focalx = limbs[MONSTER_G][4][0]; // 0
						entity->focaly = limbs[MONSTER_G][4][1]; // 0
						entity->focalz = limbs[MONSTER_G][4][2]; // 2
					}
					else
					{
						entity->focalx = limbs[MONSTER_G][4][0] + 1; // 1
						entity->focaly = limbs[MONSTER_G][4][1] + 0.25; // 0
						entity->focalz = limbs[MONSTER_G][4][2] - 0.75; // 1
						if ( entity->sprite == 1573 || entity->sprite == 1577 )
						{
							entity->sprite += 1;
						}
						else
						{
							entity->sprite += 2;
						}
					}
				}
				my->setHumanoidLimbOffset(entity, MONSTER_G, LIMB_HUMANOID_RIGHTARM);
				entity->yaw += MONSTER_WEAPONYAW;
				break;
			// left arm
			}
			case LIMB_HUMANOID_LEFTARM:
			{
				if ( multiplayer != CLIENT )
				{
					if ( myStats->gloves == nullptr )
					{
						entity->sprite = my->sprite == 1569 ? 1571 : 1575;
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
						if ( entity->ticks >= *cvar_entity_bodypart_sync_tick )
						{
							bool updateBodypart = false;
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								updateBodypart = true;
							}
							if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
							{
								updateBodypart = true;
							}
							if ( updateBodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
						}
					}
				}

				if ( multiplayer == CLIENT )
				{
					if ( entity->skill[7] == 0 )
					{
						if ( entity->sprite == 1571 || entity->sprite == 1575 )
						{
							// these are the default arms.
							// chances are they may be wrong if sent by the server, 
						}
						else
						{
							// otherwise we're being sent gloves armor etc so it's probably right.
							entity->skill[7] = entity->sprite;
						}
					}
					if ( entity->skill[7] == 0 )
					{
						// we set this ourselves until proper initialisation.
						entity->sprite = my->sprite == 1569 ? 1571 : 1575;
					}
					else
					{
						entity->sprite = entity->skill[7];
					}
				}

				shieldarm = entity;
				node_t* shieldNode = list_Node(&my->children, 8);
				if ( shieldNode )
				{
					Entity* shield = (Entity*)shieldNode->element;
					if ( shield->flags[INVISIBLE] && my->monsterState == MONSTER_STATE_WAIT )
					{
						entity->focalx = limbs[MONSTER_G][5][0]; // 0
						entity->focaly = limbs[MONSTER_G][5][1]; // 0
						entity->focalz = limbs[MONSTER_G][5][2]; // 2
					}
					else
					{
						entity->focalx = limbs[MONSTER_G][5][0] + 1; // 1
						entity->focaly = limbs[MONSTER_G][5][1] - 0.25; // 0
						entity->focalz = limbs[MONSTER_G][5][2] - 0.75; // 1
						if ( entity->sprite == 1571 || entity->sprite == 1575 )
						{
							entity->sprite += 1;
						}
						else
						{
							entity->sprite += 2;
						}
					}
				}
				my->setHumanoidLimbOffset(entity, MONSTER_G, LIMB_HUMANOID_LEFTARM);
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
						if ( entity->ticks >= *cvar_entity_bodypart_sync_tick )
						{
							bool updateBodypart = false;
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								updateBodypart = true;
							}
							if ( entity->skill[11] != entity->flags[INVISIBLE] )
							{
								entity->skill[11] = entity->flags[INVISIBLE];
								updateBodypart = true;
							}
							if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
							{
								updateBodypart = true;
							}
							if ( updateBodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
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
						if ( itemTypeIsQuiver(myStats->shield->type) )
						{
							entity->handleQuiverThirdPersonModel(*myStats);
						}
					}
					if ( myStats->EFFECTS[EFF_INVISIBLE] || wearingring ) //TODO: isInvisible()?
					{
						entity->flags[INVISIBLE] = true;
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->ticks >= *cvar_entity_bodypart_sync_tick )
						{
							bool updateBodypart = false;
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								updateBodypart = true;
							}
							if ( entity->skill[11] != entity->flags[INVISIBLE] )
							{
								entity->skill[11] = entity->flags[INVISIBLE];
								updateBodypart = true;
							}
							if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
							{
								updateBodypart = true;
							}
							if ( updateBodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
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
				my->handleHumanoidShieldLimb(entity, shieldarm);
				break;
			// cloak
			case LIMB_HUMANOID_CLOAK:
				entity->focalx = limbs[MONSTER_G][8][0];
				entity->focaly = limbs[MONSTER_G][8][1];
				entity->focalz = limbs[MONSTER_G][8][2];
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
						if ( entity->ticks >= *cvar_entity_bodypart_sync_tick )
						{
							bool updateBodypart = false;
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								updateBodypart = true;
							}
							if ( entity->skill[11] != entity->flags[INVISIBLE] )
							{
								entity->skill[11] = entity->flags[INVISIBLE];
								updateBodypart = true;
							}
							if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
							{
								updateBodypart = true;
							}
							if ( updateBodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
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
				if ( entity->sprite != 1583 && entity->sprite != 1584 )
				{
					// push back for larger armors
					entity->x -= cos(my->yaw) * 1.0;
					entity->y -= sin(my->yaw) * 1.0;
				}
				my->setHumanoidLimbOffset(entity, MONSTER_G, LIMB_HUMANOID_TORSO);
				break;
				// helm
			case LIMB_HUMANOID_HELMET:
				helmet = entity;
				entity->focalx = limbs[MONSTER_G][9][0]; // 0
				entity->focaly = limbs[MONSTER_G][9][1]; // 0
				entity->focalz = limbs[MONSTER_G][9][2]; // -2
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
						if ( entity->ticks >= *cvar_entity_bodypart_sync_tick )
						{
							bool updateBodypart = false;
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								updateBodypart = true;
							}
							if ( entity->skill[11] != entity->flags[INVISIBLE] )
							{
								entity->skill[11] = entity->flags[INVISIBLE];
								updateBodypart = true;
							}
							if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
							{
								updateBodypart = true;
							}
							if ( updateBodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
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
				entity->focalx = limbs[MONSTER_G][10][0]; // 0
				entity->focaly = limbs[MONSTER_G][10][1]; // 0
				entity->focalz = limbs[MONSTER_G][10][2]; // .25
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
						else if ( myStats->mask->type == MONOCLE )
						{
							entity->sprite = 1196; // monocleWorn.vox
						}
						else
						{
							entity->sprite = itemModel(myStats->mask);
						}
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->ticks >= *cvar_entity_bodypart_sync_tick )
						{
							bool updateBodypart = false;
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								updateBodypart = true;
							}
							if ( entity->skill[11] != entity->flags[INVISIBLE] )
							{
								entity->skill[11] = entity->flags[INVISIBLE];
								updateBodypart = true;
							}
							if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
							{
								updateBodypart = true;
							}
							if ( updateBodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
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

				if ( entity->sprite == items[MASK_SHAMAN].index )
				{
					entity->roll = 0;
					my->setHelmetLimbOffset(entity);
					my->setHelmetLimbOffsetWithMask(helmet, entity);
				}
				else if ( EquipmentModelOffsets.modelOffsetExists(MONSTER_G, entity->sprite) )
				{
					my->setHelmetLimbOffset(entity);
					my->setHelmetLimbOffsetWithMask(helmet, entity);
				}
				else
				{
					entity->focalx = limbs[MONSTER_G][10][0] + .35; // .35
					entity->focaly = limbs[MONSTER_G][10][1] - 2; // -2
					entity->focalz = limbs[MONSTER_G][10][2]; // .25
				}
				break;
			default:
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
