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
#include "engine/audio/sound.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "prng.hpp"
#include "scores.hpp"
#include "mod_tools.hpp"

real_t getNormalHeightMonsterM(Entity& my)
{
	if ( my.sprite == 1520 )
	{
		return 1.5;
	}
	return -0.5;
}

void initMonsterM(Entity* my, Stat* myStats)
{
	if ( !my )
	{
		return;
	}
	node_t* node;
	bool spawnedBoss = false;

	my->flags[BURNABLE] = true;
	my->initMonster(1519);
	my->z = getNormalHeightMonsterM(*my);

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 747;
		MONSTER_SPOTVAR = 2;
		MONSTER_IDLESND = 742;
		MONSTER_IDLEVAR = 3;
	}

	if ( multiplayer != CLIENT && !MONSTER_INIT )
	{
		auto& rng = my->entity_rng ? *my->entity_rng : local_rng;

		if ( myStats != nullptr )
		{
			if ( myStats->sex == FEMALE )
			{
				my->sprite = 1520;
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
			//const bool boss =
			//    rng.rand() % 50 == 0 &&
			//    !my->flags[USERFLAG2] &&
			//    !myStats->MISC_FLAGS[STAT_FLAG_DISABLE_MINIBOSS];
			//if ( (boss || (*cvar_summonBosses && conductGameChallenges[CONDUCT_CHEATS_ENABLED])) && myStats->leader_uid == 0 )
			//{
			//	myStats->setAttribute("special_npc", "gharbad");
			//	strcpy(myStats->name, MonsterMata_t::getSpecialNPCName(*myStats).c_str());
			//	my->sprite = MonsterMata_t::getSpecialNPCBaseModel(*myStats);
			//	myStats->sex = MALE;
			//	myStats->STR += 10;
			//	myStats->DEX += 2;
			//	myStats->MAXHP += 75;
			//	myStats->HP = myStats->MAXHP;
			//	myStats->OLDHP = myStats->MAXHP;
			//	myStats->CHR = -1;
			//	spawnedBoss = true;
			//	//TODO: Boss stats

			//	//Spawn in potions.
			//	int end = rng.rand()%NUM_GOATMAN_BOSS_GHARBAD_POTIONS + 5;
			//	for ( int i = 0; i < end; ++i )
			//	{
			//		switch ( rng.rand()%10 )
			//		{
			//			case 0:
			//			case 1:
			//			case 2:
			//			case 3:
			//			case 4:
			//			case 5:
			//			case 6:
			//			case 7:
			//			case 8:
			//				newItem(POTION_BOOZE, EXCELLENT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, &myStats->inventory);
			//				break;
			//			case 9:
			//				newItem(POTION_HEALING, EXCELLENT, 0, 1, rng.rand(), false, &myStats->inventory);
			//				break;
			//			default:
			//				printlog("Tried to spawn goatman boss \"Gharbad\" invalid potion.");
			//				break;
			//		}
			//	}

			//	newItem(CRYSTAL_SHURIKEN, EXCELLENT, 1 + rng.rand()%1, rng.rand()%NUM_GOATMAN_BOSS_GHARBAD_THROWN_WEAPONS + 2, rng.rand(), true, &myStats->inventory);
			//}

			// random effects
			/*if ( rng.rand() % 8 == 0 )
			{
				my->setEffect(EFF_ASLEEP, true, 1800 + rng.rand() % 1800, false);
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

			if ( myStats->getAttribute("monster_m_type") == "" )
			{
				if ( rng.rand() % 2 == 0 )
				{
					myStats->setAttribute("monster_m_type", "duster");
				}
				else
				{
					myStats->setAttribute("monster_m_type", "discanter");
				}
			}

			if ( myStats->getAttribute("monster_m_type") == "duster" )
			{
				newItem(DUST_BALL, SERVICABLE, 0, rng.rand() % 2 + 2, rng.rand(), false, &myStats->inventory);
			}

			// give weapon
			if ( myStats->getAttribute("monster_m_type") == "duster" )
			{
				if ( myStats->weapon == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1 )
				{
					switch ( rng.rand() % 10 )
					{
					case 0:
					case 1:
					case 2:
					case 3:
					case 4:
						myStats->weapon = newItem(IRON_AXE, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						break;
					case 5:
					case 6:
					case 7:
					case 8:
					case 9:
						myStats->weapon = newItem(IRON_MACE, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						break;
					default:
						break;
					}
				}
			}
			else if ( myStats->getAttribute("monster_m_type") == "discanter" )
			{
				if ( myStats->weapon == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1 )
				{
					switch ( rng.rand() % 10 )
					{
					case 0:
					case 1:
					case 2:
					case 3:
					case 4:
						myStats->weapon = newItem(QUARTERSTAFF, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						break;
					case 5:
					case 6:
					case 7:
					case 8:
					case 9:
						myStats->weapon = newItem(QUARTERSTAFF, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						break;
					default:
						break;
					}
				}
			}

			//bool isShaman = false;
			//if ( rng.rand() % 2 && !spawnedBoss && !minion )
			//{
			//	isShaman = true;
			//	if ( myStats->leader_uid == 0 && !my->flags[USERFLAG2] && rng.rand() % 2 == 0 )
			//	{
			//		Entity* entity = summonMonster(GOATMAN, my->x, my->y);
			//		if ( entity )
			//		{
			//			entity->parent = my->getUID();
			//			if ( Stat* followerStats = entity->getStats() )
			//			{
			//				followerStats->leader_uid = entity->parent;
			//			}
			//			entity->seedEntityRNG(rng.getU32());
			//		}
			//		if ( rng.rand() % 5 == 0 )
			//		{
			//			// summon second ally randomly.
			//			entity = summonMonster(GOATMAN, my->x, my->y);
			//			if ( entity )
			//			{
			//				entity->parent = my->getUID();
			//				if ( Stat* followerStats = entity->getStats() )
			//				{
			//					followerStats->leader_uid = entity->parent;
			//				}
			//				entity->seedEntityRNG(rng.getU32());
			//			}
			//		}
			//	}
			//}
			//else
			//{
			//	myStats->DEX += 1; // more speed for brawlers.
			//}

			// generate the default inventory items for the monster, provided the editor sprite allowed enough default slots
			//switch ( defaultItems )
			//{
			//	case 6:
			//	case 5:
			//	case 4:
			//	case 3:
			//	case 2:
			//	case 1:
			//		if ( isShaman && rng.rand() % 10 == 0 )
			//		{
			//			switch ( rng.rand() % 4 )
			//			{
			//				case 0:
			//					newItem(SPELLBOOK_SLOW, static_cast<Status>(rng.rand() % 3 + DECREPIT), -1 + rng.rand() % 3, 1, rng.rand(), false, &myStats->inventory);
			//					break;
			//				case 1:
			//					newItem(SPELLBOOK_FIREBALL, static_cast<Status>(rng.rand() % 3 + DECREPIT), -1 + rng.rand() % 3, 1, rng.rand(), false, &myStats->inventory);
			//					break;
			//				case 2:
			//					newItem(SPELLBOOK_COLD, static_cast<Status>(rng.rand() % 3 + DECREPIT), -1 + rng.rand() % 3, 1, rng.rand(), false, &myStats->inventory);
			//					break;
			//				case 3:
			//					newItem(SPELLBOOK_FORCEBOLT, static_cast<Status>(rng.rand() % 3 + DECREPIT), -1 + rng.rand() % 3, 1, rng.rand(), false, &myStats->inventory);
			//					break;
			//			}
			//		}
			//		break;
			//	default:
			//		break;
			//}


			////Give weapons.
			//if ( !spawnedBoss )
			//{
			//	if ( !isShaman && rng.rand() % 3 > 0 )
			//	{
			//		newItem(STEEL_CHAKRAM, SERVICABLE, 0, rng.rand()%NUM_GOATMAN_THROWN_WEAPONS + 1, rng.rand(), false, &myStats->inventory);
			//	}
			//	int numpotions = rng.rand() % NUM_GOATMAN_POTIONS + 2;
			//	if ( rng.rand() % 3 == 0 )
			//	{
			//		int numhealpotion = rng.rand() % 2 + 1;
			//		newItem(POTION_HEALING, static_cast<Status>(rng.rand() % 3 + DECREPIT), 0, numhealpotion, rng.rand(), false, &myStats->inventory);
			//		numpotions -= numhealpotion;
			//	}
			//	if ( rng.rand() % 4 > 0 )
			//	{
			//		// undroppable
			//		newItem(POTION_BOOZE, static_cast<Status>(rng.rand() % 3 + DECREPIT), 0, numpotions, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, &myStats->inventory);
			//	}
			//}

			//if ( isShaman )
			//{
			//	//give shield
			//	if ( myStats->shield == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_SHIELD] == 1 )
			//	{
			//		// give shield
			//		switch ( rng.rand() % 20 )
			//		{
			//			case 0:
			//			case 1:
			//			case 2:
			//			case 3:
			//			case 4:
			//			case 5:
			//			case 6:
			//			case 7:
			//				myStats->shield = newItem(TOOL_CRYSTALSHARD, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//				break;
			//			case 8:
			//				myStats->shield = newItem(MIRROR_SHIELD, static_cast<Status>(rng.rand() % 4 + DECREPIT), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//				break;
			//			default:
			//				myStats->shield = newItem(TOOL_LANTERN, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//				break;
			//		}
			//	}
			//	// give cloak
			//	if ( myStats->cloak == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_CLOAK] == 1 )
			//	{
			//		switch ( rng.rand() % 10 )
			//		{
			//			case 0:
			//			case 1:
			//				break;
			//			default:
			//				myStats->cloak = newItem(CLOAK, WORN, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//				break;
			//		}
			//	}
			//	// give helmet
			//	if ( myStats->helmet == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_HELM] == 1 )
			//	{
			//		switch ( rng.rand() % 10 )
			//		{
			//			case 0:
			//			case 1:
			//				myStats->helmet = newItem(HAT_HOOD, WORN, -1 + rng.rand() % 3, 1, 0, false, nullptr);
			//				break;
			//			default:
			//				myStats->helmet = newItem(HAT_WIZARD, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//				break;
			//		}
			//	}
			//	// give armor
			//	if ( myStats->breastplate == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_ARMOR] == 1 )
			//	{
			//		switch ( rng.rand() % 10 )
			//		{
			//			case 0:
			//			case 1:
			//				myStats->breastplate = newItem(WIZARD_DOUBLET, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//				break;
			//			case 2:
			//				myStats->breastplate = newItem(LEATHER_BREASTPIECE, static_cast<Status>(rng.rand() % 3 + DECREPIT), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//				break;
			//			default:
			//				break;
			//		}
			//	}
			//	// give booties
			//	if ( myStats->shoes == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_BOOTS] == 1 )
			//	{
			//		switch ( rng.rand() % 20 )
			//		{
			//			case 0:
			//			case 1:
			//			case 2:
			//			case 3:
			//				myStats->shoes = newItem(IRON_BOOTS, static_cast<Status>(rng.rand() % 3 + DECREPIT), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//				break;
			//			case 19:
			//				myStats->shoes = newItem(CRYSTAL_BOOTS, static_cast<Status>(rng.rand() % 4 + DECREPIT), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//				break;
			//			default:
			//				myStats->shoes = newItem(STEEL_BOOTS, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//				break;
			//		}
			//	}
			//	// give weapon
			//	if ( myStats->weapon == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1 )
			//	{
			//		switch ( rng.rand() % 12 )
			//		{
			//			case 0:
			//			case 1:
			//			case 2:
			//			case 3:
			//			case 4:
			//				myStats->weapon = newItem(MAGICSTAFF_COLD, static_cast<Status>(rng.rand() % 2 + SERVICABLE), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//				break;
			//			case 5:
			//			case 6:
			//			case 7:
			//			case 8:
			//				myStats->weapon = newItem(MAGICSTAFF_FIRE, static_cast<Status>(rng.rand() % 2 + SERVICABLE), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//				break;
			//			case 9:
			//				switch ( rng.rand() % 4 )
			//				{
			//					case 0:
			//						myStats->weapon = newItem(SPELLBOOK_SLOW, static_cast<Status>(rng.rand() % 2 + DECREPIT), -1 + rng.rand() % 3, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
			//						break;
			//					case 1:
			//						myStats->weapon = newItem(SPELLBOOK_FIREBALL, static_cast<Status>(rng.rand() % 3 + DECREPIT), -1 + rng.rand() % 3, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
			//						break;
			//					case 2:
			//						myStats->weapon = newItem(SPELLBOOK_COLD, static_cast<Status>(rng.rand() % 3 + DECREPIT), -1 + rng.rand() % 3, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
			//						break;
			//					case 3:
			//						myStats->weapon = newItem(SPELLBOOK_FORCEBOLT, static_cast<Status>(rng.rand() % 3 + DECREPIT), -1 + rng.rand() % 3, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
			//						break;
			//				}
			//				break;
			//			default:
			//				myStats->weapon = newItem(MAGICSTAFF_SLOW, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//				break;
			//		}
			//	}
			//}
			//else
			//{
			//	////give shield
			//	//if ( myStats->shield == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_SHIELD] == 1 )
			//	//{
			//	//	switch ( rng.rand() % 20 )
			//	//	{
			//	//	case 0:
			//	//	case 1:
			//	//	case 2:
			//	//	case 3:
			//	//	case 4:
			//	//	case 5:
			//	//	case 6:
			//	//	case 7:
			//	//		myStats->shield = newItem(TOOL_CRYSTALSHARD, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//	//		break;
			//	//	case 8:
			//	//		myStats->shield = newItem(MIRROR_SHIELD, static_cast<Status>(rng.rand() % 4 + DECREPIT), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//	//		break;
			//	//	default:
			//	//		myStats->shield = newItem(TOOL_LANTERN, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//	//		break;
			//	//	}
			//	//}
			//	// give cloak
			//	/*if ( myStats->cloak == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_CLOAK] == 1 )
			//	{
			//		switch ( rng.rand() % 10 )
			//		{
			//		case 0:
			//		case 1:
			//			break;
			//		default:
			//			myStats->cloak = newItem(CLOAK, WORN, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//			break;
			//		}
			//	}*/
			//	//// give helmet
			//	//if ( myStats->helmet == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_HELM] == 1 )
			//	//{
			//	//	switch ( rng.rand() % 10 )
			//	//	{
			//	//	case 0:
			//	//	case 1:
			//	//		myStats->helmet = newItem(HAT_HOOD, WORN, -1 + rng.rand() % 3, 1, 0, false, nullptr);
			//	//		break;
			//	//	default:
			//	//		myStats->helmet = newItem(HAT_WIZARD, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, 0, false, nullptr);
			//	//		break;
			//	//	}
			//	//}
			//	// give armor
			//	if ( myStats->breastplate == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_ARMOR] == 1 )
			//	{
			//		switch ( rng.rand() % 20 )
			//		{
			//		case 0:
			//		case 1:
			//		case 2:
			//			myStats->breastplate = newItem(STEEL_BREASTPIECE, static_cast<Status>(rng.rand() % 4 + DECREPIT), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//			break;
			//		case 3:
			//		case 4:
			//			myStats->breastplate = newItem(LEATHER_BREASTPIECE, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//			break;
			//		case 5:
			//		case 6:
			//			myStats->breastplate = newItem(IRON_BREASTPIECE, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//			break;
			//		case 19:
			//			myStats->breastplate = newItem(CRYSTAL_BREASTPIECE, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//			break;
			//		default:
			//			break;
			//		}
			//	}
			//	// give booties
			//	if ( myStats->shoes == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_BOOTS] == 1 )
			//	{
			//		switch ( rng.rand() % 20 )
			//		{
			//		case 0:
			//		case 1:
			//		case 2:
			//		case 3:
			//			myStats->shoes = newItem(IRON_BOOTS, static_cast<Status>(rng.rand() % 3 + DECREPIT), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//			break;
			//		case 19:
			//			myStats->shoes = newItem(CRYSTAL_BOOTS, static_cast<Status>(rng.rand() % 4 + DECREPIT), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//			break;
			//		default:
			//			myStats->shoes = newItem(STEEL_BOOTS, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//			break;
			//		}
			//	}
			//	// give weapon
			//	if ( myStats->weapon == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1 )
			//	{
			//		switch ( rng.rand() % 20 )
			//		{
			//		case 0:
			//		case 1:
			//		case 2:
			//		case 3:
			//		case 4:
			//		case 5:
			//		case 6:
			//		case 7:
			//			myStats->weapon = newItem(STEEL_AXE, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//			break;
			//		case 18:
			//			myStats->weapon = newItem(CRYSTAL_BATTLEAXE, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//			break;
			//		case 19:
			//			myStats->weapon = newItem(CRYSTAL_MACE, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//			break;
			//		default:
			//			myStats->weapon = newItem(STEEL_MACE, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
			//			break;
			//		}
			//	}
			//}
		}
	}

	// torso
	const int torso_sprite = my->sprite == 1519 ? 1534 : 1535;
	Entity* entity = newEntity(torso_sprite, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->scalex = 1.01;
	entity->scaley = 1.01;
	entity->scalez = 1.01;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[MONSTER_M][1][0]; // 0
	entity->focaly = limbs[MONSTER_M][1][1]; // 0
	entity->focalz = limbs[MONSTER_M][1][2]; // 0
	entity->behavior = &actMonsterMLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right leg
	entity = newEntity(my->sprite == 1519 ? 1532 : 1533, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[MONSTER_M][2][0]; // 0
	entity->focaly = limbs[MONSTER_M][2][1]; // 0
	entity->focalz = limbs[MONSTER_M][2][2]; // 2
	entity->behavior = &actMonsterMLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left leg
	entity = newEntity(my->sprite == 1519 ? 1530 : 1531, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[MONSTER_M][3][0]; // 0
	entity->focaly = limbs[MONSTER_M][3][1]; // 0
	entity->focalz = limbs[MONSTER_M][3][2]; // 2
	entity->behavior = &actMonsterMLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right arm
	entity = newEntity(1523, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[MONSTER_M][4][0]; // 0
	entity->focaly = limbs[MONSTER_M][4][1]; // 0
	entity->focalz = limbs[MONSTER_M][4][2]; // 1.5
	entity->behavior = &actMonsterMLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left arm
	entity = newEntity(1521, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[MONSTER_M][5][0]; // 0
	entity->focaly = limbs[MONSTER_M][5][1]; // 0
	entity->focalz = limbs[MONSTER_M][5][2]; // 1.5
	entity->behavior = &actMonsterMLimb;
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
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->noColorChangeAllyLimb = 1.0;
	entity->focalx = limbs[MONSTER_M][6][0]; // 1.5
	entity->focaly = limbs[MONSTER_M][6][1]; // 0
	entity->focalz = limbs[MONSTER_M][6][2]; // -.5
	entity->behavior = &actMonsterMLimb;
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
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->noColorChangeAllyLimb = 1.0;
	entity->focalx = limbs[MONSTER_M][7][0]; // 2
	entity->focaly = limbs[MONSTER_M][7][1]; // 0
	entity->focalz = limbs[MONSTER_M][7][2]; // 0
	entity->behavior = &actMonsterMLimb;
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
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->noColorChangeAllyLimb = 1.0;
	entity->focalx = limbs[MONSTER_M][8][0]; // 0
	entity->focaly = limbs[MONSTER_M][8][1]; // 0
	entity->focalz = limbs[MONSTER_M][8][2]; // 4
	entity->behavior = &actMonsterMLimb;
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
	entity->focalx = limbs[MONSTER_M][9][0]; // 0
	entity->focaly = limbs[MONSTER_M][9][1]; // 0
	entity->focalz = limbs[MONSTER_M][9][2]; // -2
	entity->behavior = &actMonsterMLimb;
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
	entity->focalx = limbs[MONSTER_M][10][0]; // 0
	entity->focaly = limbs[MONSTER_M][10][1]; // 0
	entity->focalz = limbs[MONSTER_M][10][2]; // .25
	entity->behavior = &actMonsterMLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// head left
	entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[MONSTER_M][11][0];
	entity->focaly = limbs[MONSTER_M][11][1];
	entity->focalz = limbs[MONSTER_M][11][2];
	entity->behavior = &actMonsterMLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// head right
	entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[MONSTER_M][13][0];
	entity->focaly = limbs[MONSTER_M][13][1];
	entity->focalz = limbs[MONSTER_M][13][2];
	entity->behavior = &actMonsterMLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// head center
	entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[MONSTER_M][15][0];
	entity->focaly = limbs[MONSTER_M][15][1];
	entity->focalz = limbs[MONSTER_M][15][2];
	entity->scalex = 1.008;
	entity->scaley = 1.008;
	entity->scalez = 1.008;
	entity->behavior = &actMonsterMLimb;
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

void actMonsterMLimb(Entity* my)
{
	my->actMonsterLimb(true);
}

void monsterMDie(Entity* my)
{
	Entity* gib = spawnGib(my);
	gib->skill[5] = 1; // poof
	gib->sprite = my->sprite;
	serverSpawnGibForClient(gib);
	for ( int c = 0; c < 8; c++ )
	{
		Entity* gib = spawnGib(my);
		serverSpawnGibForClient(gib);
	}

	my->spawnBlood();

	playSoundEntity(my, 745 + local_rng.rand() % 2, 128);

	my->removeMonsterDeathNodes();

	list_RemoveNode(my->mynode);
	return;
}

#define MONSTER_MWALKSPEED .13

void monsterMMoveBodyparts(Entity* my, Stat* myStats, double dist)
{
	node_t* node;
	Entity* entity = nullptr, * entity2 = nullptr;
	Entity* additionalLimb = nullptr;
	Entity* rightbody = nullptr;
	Entity* weaponarm = nullptr;
	int bodypart;
	bool wearingring = false;

	my->focalx = limbs[MONSTER_M][0][0];
	my->focaly = limbs[MONSTER_M][0][1];
	my->focalz = limbs[MONSTER_M][0][2];

	/*if ( keystatus[SDLK_j] )
	{
		keystatus[SDLK_j] = 0;
		my->fskill[1] = 1.0;

		my->monsterAttack = MONSTER_POSE_MAGIC_WINDUP3;
	}*/

	if ( my->sprite == 1520 )
	{
		my->focalz += 0.5;
		my->monsterSpellAnimation = MONSTER_SPELLCAST_SMALL_HUMANOID;
	}

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
		if ( myStats->getEffectActive(EFF_INVISIBLE) || wearingring == true )
		{
			my->flags[INVISIBLE] = true;
			my->flags[BLOCKSIGHT] = false;
			bodypart = 0;
			for ( node = my->children.first; node != nullptr; node = node->next )
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
			for ( node = my->children.first; node != nullptr; node = node->next )
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
		if ( myStats->getEffectActive(EFF_ASLEEP) )
		{
			if ( my->sprite == 1520 )
			{
				my->z = 3.75;
			}
			else
			{
				my->z = 3.0;
			}
			my->pitch = PI / 4;
		}
		else
		{
			my->z = getNormalHeightMonsterM(*my);

			if ( my->monsterAttack == 0 )
			{
				if ( debugModel )
				{
					my->pitch = my->fskill[0];
					if ( my->fskill[1] > 0.0 ) // jumpy
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
		my->creatureHandleLiftZ();
	}

	Entity* shieldarm = nullptr;
	Entity* helmet = nullptr;

	//Move bodyparts
	for ( bodypart = 0, node = my->children.first; node != nullptr; node = node->next, bodypart++ )
	{
		if ( bodypart < LIMB_HUMANOID_TORSO )
		{
			// post-swing head animation. client doesn't need to adjust the entity pitch, server will handle.
			if ( my->monsterAttack != MONSTER_POSE_RANGED_WINDUP3 && my->monsterAttack != MONSTER_POSE_SPECIAL_WINDUP1
				&& bodypart == 1 && multiplayer != CLIENT )
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
			my->humanoidAnimateWalk(entity, node, bodypart, MONSTER_MWALKSPEED, dist, 0.4);
		}
		else if ( bodypart == LIMB_HUMANOID_LEFTLEG || bodypart == LIMB_HUMANOID_RIGHTARM || bodypart == LIMB_HUMANOID_CLOAK )
		{
			// left leg, right arm, cloak.
			if ( bodypart == LIMB_HUMANOID_RIGHTARM )
			{
				weaponarm = entity;
				if ( MONSTER_ATTACK > 0 )
				{
					if ( my->monsterAttack == MONSTER_POSE_RANGED_WINDUP3 
						|| my->monsterAttack == MONSTER_POSE_SPECIAL_WINDUP1 )
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
							playSoundEntityLocal(my, 747, 128);
							createParticleDot(my);
							if ( multiplayer != CLIENT )
							{
								myStats->setEffectActive(EFF_PARALYZED, 1);
								myStats->EFFECTS_TIMERS[EFF_PARALYZED] = 40;
							}
						}
						if ( multiplayer != CLIENT )
						{
							// move the head and weapon yaw
							limbAnimateToLimit(my, ANIMATE_PITCH, -0.1, 11 * PI / 6, false, 0.0);
							limbAnimateToLimit(my, ANIMATE_WEAPON_YAW, 0.05, 2 * PI / 8, false, 0.0);
						}
						limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.25, 7 * PI / 4, true, 0.0);
						//limbAnimateToLimit(weaponarm, ANIMATE_ROLL, -0.25, 7 * PI / 4, false, 0.0);

						if ( my->monsterAttackTime >= 4 * ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
						{
							if ( multiplayer != CLIENT )
							{
								if ( my->monsterAttack == MONSTER_POSE_SPECIAL_WINDUP1 )
								{
									my->attack(MONSTER_POSE_MAGIC_WINDUP3, 0, nullptr);
								}
								else
								{
									my->attack(MONSTER_POSE_MELEE_WINDUP1, 0, nullptr);
								}
							}
						}
					}
					else if ( my->monsterAttack == MONSTER_POSE_MAGIC_WINDUP3 )
					{
						if ( multiplayer != CLIENT )
						{
							if ( my->monsterAttackTime == 1 )
							{
								int spellID = SPELL_SPORE_BOMB;
								if ( my->monsterSpecialState == MONSTER_M_SPECIAL_CAST3 )
								{
									real_t oldYaw = my->yaw;
									real_t dist = 100.0;
									if ( Entity* target = uidToEntity(my->monsterTarget) )
									{
										real_t tangent = atan2(target->y - my->y,
											target->x - my->x);
										my->yaw = tangent;
										dist = entityDist(my, target);
									}

									castSpell(my->getUID(), getSpellFromID(spellID), true, false);
									
									playSoundEntity(my, 171, 128);
									my->yaw = oldYaw;
								}
								else
								{
									real_t dist = 100.0;
									real_t yawDiff = 0.0;
									if ( Entity* target = uidToEntity(my->monsterTarget) )
									{
										yawDiff = my->yawDifferenceFromEntity(target);
										dist = entityDist(my, target);
									}
									bool setProps = false;
									CastSpellProps_t props;
									if ( my->monsterSpecialState == MONSTER_M_SPECIAL_CAST1
										&& (dist > TOUCHRANGE || (yawDiff >= 0 && yawDiff < PI)) )
									{
										spellID = SPELL_TELEKINESIS;
										setProps = props.setToMonsterCast(my, spellID);
									}
									if ( !setProps )
									{
										spellID = SPELL_SPORES;
									}

									if ( setProps )
									{
										castSpell(my->getUID(), getSpellFromID(spellID), true, false, false, &props);
									}
									else
									{
										castSpell(my->getUID(), getSpellFromID(spellID), true, false);

										if ( spellID == SPELL_SPORES )
										{
											for ( int x = -1; x <= 1; ++x )
											{
												for ( int y = -1; y <= 1; ++y )
												{
													floorMagicCreateSpores(my, my->x + x * 16.0, my->y + y * 16.0, my, 0, SPELL_SPORES);
												}
											}
										}
									}
								}
							}
						}
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
						else if ( weaponarm->skill[1] >= 1 )
						{
							if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.25, 7 * PI / 4, false, 0.0) )
							{
								Entity* rightbody = nullptr;
								// set rightbody to left leg.
								node_t* rightbodyNode = list_Node(&my->children, LIMB_HUMANOID_LEFTLEG);
								if ( rightbodyNode )
								{
									rightbody = (Entity*)rightbodyNode->element;
								}
								if ( rightbody )
								{
									weaponarm->skill[0] = rightbody->skill[0];
									weaponarm->pitch = rightbody->pitch;
								}
								my->monsterWeaponYaw = 0;
								weaponarm->roll = 0;
								my->monsterArmbended = 0;
								my->monsterAttack = 0;
							}
						}
					}
					else
					{
						if ( multiplayer != CLIENT )
						{
							if ( my->monsterAttack == MONSTER_POSE_MAGIC_WINDUP1 &&
								my->monsterSpecialState == MONSTER_M_SPECIAL_CAST1
								&& my->monsterAttackTime == 0 )
							{
								my->setEffect(EFF_ROOTED, true, 50, false, true, false, false);
							}
						}
						my->handleWeaponArmAttack(entity);
					}
				}
			}
			else if ( bodypart == LIMB_HUMANOID_CLOAK )
			{
				entity->pitch = entity->fskill[0];
			}

			my->humanoidAnimateWalk(entity, node, bodypart, MONSTER_MWALKSPEED, dist, 0.4);

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
			entity->scalex = 1.0;
			entity->scaley = 1.0;
			entity->scalez = 1.0;
			entity->focalx = limbs[MONSTER_M][1][0];
			entity->focaly = limbs[MONSTER_M][1][1];
			entity->focalz = limbs[MONSTER_M][1][2];
			if ( multiplayer != CLIENT )
			{
				if ( myStats->breastplate == nullptr )
				{
					entity->sprite =
						my->sprite == 1519 ? 1534 : 1535;
				}
				else
				{
					entity->sprite = itemModel(myStats->breastplate, my->sprite == 1520);
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
			my->setHumanoidLimbOffset(entity, MONSTER_M, LIMB_HUMANOID_TORSO);
			break;
			// right leg
		case LIMB_HUMANOID_RIGHTLEG:
			entity->focalx = limbs[MONSTER_M][2][0];
			entity->focaly = limbs[MONSTER_M][2][1];
			entity->focalz = limbs[MONSTER_M][2][2];
			if ( multiplayer != CLIENT )
			{
				if ( myStats->shoes == nullptr )
				{
					entity->sprite = 
						my->sprite == 1519 ? 1532 : 1533;
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
			my->setHumanoidLimbOffset(entity, MONSTER_M, LIMB_HUMANOID_RIGHTLEG);
			break;
			// left leg
		case LIMB_HUMANOID_LEFTLEG:
			entity->focalx = limbs[MONSTER_M][3][0];
			entity->focaly = limbs[MONSTER_M][3][1];
			entity->focalz = limbs[MONSTER_M][3][2];
			if ( multiplayer != CLIENT )
			{
				if ( myStats->shoes == nullptr )
				{
					entity->sprite =
						my->sprite == 1519 ? 1530 : 1531;
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
			my->setHumanoidLimbOffset(entity, MONSTER_M, LIMB_HUMANOID_LEFTLEG);
			break;
			// right arm
		case LIMB_HUMANOID_RIGHTARM:
		{
			if ( multiplayer != CLIENT )
			{
				if ( myStats->gloves == nullptr )
				{
					entity->sprite = 1523;
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
					if ( entity->sprite == 1523 )
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
					entity->sprite = 1523;
				}
				else
				{
					entity->sprite = entity->skill[7];
				}
			}

			node_t* weaponNode = list_Node(&my->children, 7);
			if ( weaponNode )
			{
				Entity* weapon = (Entity*)weaponNode->element;
				if ( MONSTER_ARMBENDED || (weapon->flags[INVISIBLE] && my->monsterState == MONSTER_STATE_WAIT) )
				{
					// if weapon invisible and I'm not attacking, relax arm.
					entity->focalx = limbs[MONSTER_M][4][0]; // 0
					entity->focaly = limbs[MONSTER_M][4][1];
					entity->focalz = limbs[MONSTER_M][4][2]; // 2

					if ( entity->sprite == 1523 )
					{
						entity->focaly += 0.25;
					}
				}
				else
				{
					// else flex arm.
					entity->focalx = limbs[MONSTER_M][4][0] + 0.75;
					entity->focaly = limbs[MONSTER_M][4][1] + 0.25;
					entity->focalz = limbs[MONSTER_M][4][2] - 0.75;
					if ( entity->sprite == 1523 )
					{
						entity->sprite += 1;
					}
					else
					{
						entity->sprite += 2;
					}
				}
			}
			my->setHumanoidLimbOffset(entity, MONSTER_M, LIMB_HUMANOID_RIGHTARM);
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
					entity->sprite = 1521;
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
					if ( entity->sprite == 1521 )
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
					entity->sprite = 1521;
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
					entity->focalx = limbs[MONSTER_M][5][0]; // 0
					entity->focaly = limbs[MONSTER_M][5][1];
					entity->focalz = limbs[MONSTER_M][5][2]; // 2

					if ( entity->sprite == 1521 )
					{
						entity->focaly -= 0.25;
					}
				}
				else
				{
					entity->focalx = limbs[MONSTER_M][5][0] + 0.75;
					entity->focaly = limbs[MONSTER_M][5][1] - 0.25;
					entity->focalz = limbs[MONSTER_M][5][2] - 0.75;
					if ( entity->sprite == 1521 )
					{
						entity->sprite += 1;
					}
					else
					{
						entity->sprite += 2;
					}
				}
			}
			my->setHumanoidLimbOffset(entity, MONSTER_M, LIMB_HUMANOID_LEFTARM);
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
				if ( myStats->weapon == nullptr || myStats->getEffectActive(EFF_INVISIBLE) || wearingring ) //TODO: isInvisible()?
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
				if ( myStats->getEffectActive(EFF_INVISIBLE) || wearingring ) //TODO: isInvisible()?
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
			entity->focalx = limbs[MONSTER_M][8][0];
			entity->focaly = limbs[MONSTER_M][8][1];
			entity->focalz = limbs[MONSTER_M][8][2];
			if ( multiplayer != CLIENT )
			{
				if ( myStats->cloak == nullptr || myStats->getEffectActive(EFF_INVISIBLE) || wearingring ) //TODO: isInvisible()?
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
			entity->x -= cos(my->yaw);
			entity->y -= sin(my->yaw);
			entity->yaw += PI / 2;
			break;
			// helm
		case LIMB_HUMANOID_HELMET:
			helmet = entity;
			entity->focalx = limbs[MONSTER_M][9][0]; // 0
			entity->focaly = limbs[MONSTER_M][9][1]; // 0
			entity->focalz = limbs[MONSTER_M][9][2]; // -2
			entity->pitch = my->pitch;
			entity->roll = 0;
			if ( multiplayer != CLIENT )
			{
				entity->sprite = itemModel(myStats->helmet);
				if ( myStats->helmet == nullptr || myStats->getEffectActive(EFF_INVISIBLE) || wearingring ) //TODO: isInvisible()?
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
			entity->focalx = limbs[MONSTER_M][10][0]; // 0
			entity->focaly = limbs[MONSTER_M][10][1]; // 0
			entity->focalz = limbs[MONSTER_M][10][2]; // .25
			entity->pitch = my->pitch;
			entity->roll = PI / 2;
			if ( multiplayer != CLIENT )
			{
				if ( myStats->mask == nullptr || myStats->getEffectActive(EFF_INVISIBLE) || wearingring ) //TODO: isInvisible()?
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
			if ( EquipmentModelOffsets.modelOffsetExists(MONSTER_M, entity->sprite, my->sprite) )
			{
				my->setHelmetLimbOffset(entity);
				my->setHelmetLimbOffsetWithMask(helmet, entity);
			}
			else
			{
				entity->focalx = limbs[MONSTER_M][10][0] + .35; // .35
				entity->focaly = limbs[MONSTER_M][10][1] - 2; // -2
				entity->focalz = limbs[MONSTER_M][10][2]; // .25
			}
			break;
			// head left
		case 12:
		{
			entity->focalx = limbs[MONSTER_M][11][0];
			entity->focaly = limbs[MONSTER_M][11][1];
			entity->focalz = limbs[MONSTER_M][11][2];
			entity->x += limbs[MONSTER_M][12][0] * cos(my->yaw + PI / 2) + limbs[MONSTER_M][12][1] * cos(my->yaw);
			entity->y += limbs[MONSTER_M][12][0] * sin(my->yaw + PI / 2) + limbs[MONSTER_M][12][1] * sin(my->yaw);
			entity->z += limbs[MONSTER_M][12][2];
			entity->pitch = my->pitch;
			if ( multiplayer != CLIENT )
			{
				entity->sprite = 1528;
				if ( myStats->getEffectActive(EFF_INVISIBLE) || wearingring ) //TODO: isInvisible()?
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

			additionalLimb = entity;
			bool moving = false;
			if ( dist > 0.1 )
			{
				moving = true;
			}
			if ( entity->skill[0] == 0 )
			{
				if ( moving )
				{
					entity->fskill[0] += std::min(dist * MONSTER_MWALKSPEED, 2.f * MONSTER_MWALKSPEED); // move proportional to move speed
				}
				else if ( my->monsterAttack != 0 )
				{
					entity->fskill[0] += MONSTER_MWALKSPEED; // move fixed speed when attacking if stationary
				}
				else
				{
					entity->fskill[0] += 0.01; // otherwise move slow idle
				}

				if ( entity->fskill[0] > PI / 3 || ((!moving || my->monsterAttack != 0) && entity->fskill[0] > PI / 5) )
				{
					// switch direction if angle too great, angle is shorter if attacking or stationary
					entity->skill[0] = 1;
				}
			}
			else // reverse of the above
			{
				if ( moving )
				{
					entity->fskill[0] -= std::min(dist * MONSTER_MWALKSPEED, 2.f * MONSTER_MWALKSPEED);
				}
				else if ( my->monsterAttack != 0 )
				{
					entity->fskill[0] -= MONSTER_MWALKSPEED;
				}
				else
				{
					entity->fskill[0] -= 0.007;
				}

				if ( entity->fskill[0] < -PI / 32 )
				{
					entity->skill[0] = 0;
				}
			}
			entity->yaw += -entity->fskill[0] / 4;
			//entity->pitch += entity->fskill[0] / 4;
			entity->roll = -entity->fskill[0];
		}
		break;
		// head right
		case 13:
		{
			entity->focalx = limbs[MONSTER_M][13][0];
			entity->focaly = limbs[MONSTER_M][13][1];
			entity->focalz = limbs[MONSTER_M][13][2];
			entity->x += limbs[MONSTER_M][14][0] * cos(my->yaw + PI / 2) + limbs[MONSTER_M][14][1] * cos(my->yaw);
			entity->y += limbs[MONSTER_M][14][0] * sin(my->yaw + PI / 2) + limbs[MONSTER_M][14][1] * sin(my->yaw);
			entity->z += limbs[MONSTER_M][14][2];
			entity->pitch = my->pitch;
			if ( multiplayer != CLIENT )
			{
				entity->sprite = 1529;
				if ( myStats->getEffectActive(EFF_INVISIBLE) || wearingring ) //TODO: isInvisible()?
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

			if ( additionalLimb ) // follow the yaw of the previous limb.
			{
				entity->yaw -= -additionalLimb->fskill[0] / 4;
				//entity->pitch += additionalLimb->fskill[0] / 16;
				entity->roll = additionalLimb->fskill[0];
			}
		}
		break;
		// head center
		case 14:
			entity->focalx = limbs[MONSTER_M][15][0];
			entity->focaly = limbs[MONSTER_M][15][1];
			entity->focalz = limbs[MONSTER_M][15][2];
			entity->x += limbs[MONSTER_M][16][0] * cos(my->yaw + PI / 2) + limbs[MONSTER_M][16][1] * cos(my->yaw);
			entity->y += limbs[MONSTER_M][16][0] * sin(my->yaw + PI / 2) + limbs[MONSTER_M][16][1] * sin(my->yaw);
			entity->z += limbs[MONSTER_M][16][2];
			entity->pitch = my->pitch;
			if ( keystatus[SDLK_h] )
			{
				entity->fskill[0] += 0.05;
			}
			if ( entity->sprite == 1526 )
			{
				entity->focalz -= .25;
			}

			entity->pitch += (entity->fskill[2]) * (PI / 64) * sin(entity->fskill[1]);
			entity->roll = (entity->fskill[2]) * (PI / 64) * sin(entity->fskill[1] + PI / 2);

			real_t bobScale = 0.08 * entity->skill[0];
			entity->scalex = 1.0 + bobScale * sin(entity->fskill[3]);
			entity->scaley = 1.0 + bobScale * sin(entity->fskill[3]);
			entity->scalez = 1.0 - bobScale * sin(entity->fskill[3]);

			if ( my->monsterAttack == MONSTER_POSE_SPECIAL_WINDUP1
				&& my->monsterAttackTime == 1 )
			{
				entity->skill[0] = 4;

				entity->fskill[3] = 0.0;
				entity->fskill[1] = 2 * PI * 0.0;
				entity->fskill[2] = 0.0;
			}

			if ( entity->skill[0] > 0 )
			{
				entity->fskill[2] = std::min(1.0, std::max(0.05, entity->fskill[2] * 1.15));
				if ( entity->skill[0] == 4 ) // circle first
				{
					if ( entity->fskill[1] >= 2 * PI )
					{
						entity->skill[0]--;
					}
				}
				else
				{
					entity->fskill[3] += 0.25;
					if ( entity->fskill[3] >= 2 * PI )
					{
						entity->fskill[3] -= 2 * PI;
						entity->skill[0]--;
					}
				}
			}
			else
			{
				entity->fskill[2] *= 0.95;
			}
			entity->fskill[1] += 0.125;

			//entity->scalex = 1.008 + std::max(0.0, 0.05 * sin(entity->fskill[0]));
			//entity->scaley = 1.008 + std::max(0.0, 0.05 * sin(entity->fskill[0]));
			//entity->scalez = 1.008 + 0.05 * sin(entity->fskill[0]);
			if ( multiplayer != CLIENT )
			{
				if ( debugModel )
				{
					static int stage = 0;
					if ( keystatus[SDLK_g] )
					{
						keystatus[SDLK_g] = 0;
						stage += 1;
						if ( stage > 2 )
						{
							stage = 0;
						}
					}
					entity->sprite = 1525;
					if ( stage == 0 )
					{
						entity->sprite = 1525;
					}
					else if ( stage == 1 )
					{
						entity->sprite = 1526;
					}
					else if ( stage == 2 )
					{
						entity->sprite = 1527;
					}
				}
				else
				{
					entity->sprite = 1525;
					if ( myStats->getAttribute("monster_m_type") == "duster" )
					{
						entity->sprite = 1526;
					}
					else if ( myStats->getAttribute("monster_m_type") == "discanter" )
					{
						entity->sprite = 1527;
					}
				}

				if ( myStats->getEffectActive(EFF_INVISIBLE) || wearingring
					|| (helmet && helmet->sprite > 0 && !helmet->flags[INVISIBLE]) ) //TODO: isInvisible()?
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

void Entity::monsterMChooseWeapon(const Entity* target, double dist)
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

	if ( myStats->weapon && (itemCategory(myStats->weapon) == SPELLBOOK) )
	{
		return;
	}

	bool inMeleeRange = monsterInMeleeRange(target, dist);

	//if ( inMeleeRange )
	//{
	//	//if ( monsterSpecialTimer == 0 && (ticks % 10 == 0) && monsterAttack == 0 && local_rng.rand() % 10 == 0 )
	//	//{
	//	//	bool tryThrow = true;
	//	//	if ( tryThrow )
	//	//	{
	//	//		node_t* thrownNode = itemNodeInInventory(myStats, -1, THROWN);
	//	//		if ( thrownNode )
	//	//		{
	//	//			bool swapped = swapMonsterWeaponWithInventoryItem(this, myStats, thrownNode, false, true);
	//	//			if ( !swapped )
	//	//			{
	//	//				//Don't return, make sure holding a melee weapon at least.
	//	//			}
	//	//			else
	//	//			{
	//	//				monsterSpecialState = MONSTER_M_SPECIAL_THROW;
	//	//				return;
	//	//			}
	//	//		}
	//	//	}
	//	//}

	//	//Switch to a melee weapon if not already wielding one. Unless monster special state is overriding the AI.
	//	if ( !myStats->weapon || !isMeleeWeapon(*myStats->weapon) )
	//	{
	//		node_t* weaponNode = getMeleeWeaponItemNodeInInventory(myStats);
	//		if ( !weaponNode )
	//		{
	//			/*if ( myStats->weapon && myStats->weapon->type == MAGICSTAFF_SLOW )
	//			{
	//				monsterUnequipSlotFromCategory(myStats, &myStats->weapon, MAGICSTAFF);
	//			}*/
	//			return; //Resort to fists.
	//		}

	//		bool swapped = swapMonsterWeaponWithInventoryItem(this, myStats, weaponNode, false, true);
	//		if ( !swapped )
	//		{
	//			//Don't return so that monsters will at least equip ranged weapons in melee range if they don't have anything else.
	//		}
	//		else
	//		{
	//			return;
	//		}
	//	}
	//	else
	//	{
	//		return;
	//	}
	//}

	if ( myStats->getAttribute("monster_m_type") == "discanter" )
	{
		if ( monsterSpecialTimer == 0 && (ticks % 10 == 0) && monsterAttack == 0
			&& local_rng.rand() % 3 == 0 )
		{
			if ( (dist >= TOUCHRANGE && local_rng.rand() % 3 == 0) || dist > 80.0 )
			{
				monsterSpecialState = MONSTER_M_SPECIAL_CAST3;
			}
			else
			{
				if ( dist <= TOUCHRANGE && myStats->getEffectActive(EFF_SPORES) )
				{
					// inside melee and currently sporing
				}
				else
				{
					monsterSpecialState = MONSTER_M_SPECIAL_CAST1;
				}
			}
		}
		return;
	}

	//Switch to a thrown weapon or a ranged weapon
	/*if ( !myStats->weapon || isMeleeWeapon(*myStats->weapon) )*/
	{
		//First search the inventory for a THROWN weapon.
		node_t *weaponNode = nullptr;
		if ( monsterSpecialTimer == 0 && (ticks % 10 == 0) && monsterAttack == 0
			&& (dist > STRIKERANGE)
			&& local_rng.rand() % 3 == 0 )
		{
			weaponNode = itemNodeInInventory(myStats, -1, THROWN);
			if ( weaponNode )
			{
				if ( swapMonsterWeaponWithInventoryItem(this, myStats, weaponNode, false, true) )
				{
					monsterSpecialState = MONSTER_M_SPECIAL_THROW;
					return;
				}
			}
		}
		//if ( !weaponNode )
		//{
		//	//If couldn't find any, search the inventory for a ranged weapon.
		//	weaponNode = getRangedWeaponItemNodeInInventory(myStats, true);
		//}

		bool swapped = swapMonsterWeaponWithInventoryItem(this, myStats, weaponNode, false, true);
		return;
	}

	return;
}



