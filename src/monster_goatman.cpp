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

const int NUM_GOATMAN_POTIONS = 4;
const int NUM_GOATMAN_THROWN_WEAPONS = 2;

const int NUM_GOATMAN_BOSS_GHARBAD_POTIONS = 3;
const int NUM_GOATMAN_BOSS_GHARBAD_THROWN_WEAPONS = 3;

void initGoatman(Entity* my, Stat* myStats)
{
	node_t* node;
	bool spawnedBoss = false;

	my->flags[BURNABLE] = true;
	my->initMonster(463); //Sprite 463 = Goatman head model
	my->z = 0;

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 335;
		MONSTER_SPOTVAR = 3;
		MONSTER_IDLESND = 332;
		MONSTER_IDLEVAR = 2;
	}

	if ( multiplayer != CLIENT && !MONSTER_INIT )
	{
		auto& rng = my->entity_rng ? *my->entity_rng : local_rng;

		if ( myStats != nullptr )
		{
		    if (myStats->sex == FEMALE)
		    {
		        my->sprite = 1029;
		    }
			if ( strstr(map.name, "Hell") )
			{
				strcpy(myStats->name, "lesser goatman");
			}
			bool minion = false;
			if ( !myStats->leader_uid )
			{
				myStats->leader_uid = 0;
			}

			if ( my->parent != 0 )
			{
				minion = true;
			}

			if ( !strncmp(myStats->name, "lesser goatman", strlen("lesser goatman")) )
			{
				myStats->MAXHP = 150;
				myStats->HP = myStats->MAXHP;
				myStats->OLDHP = myStats->HP;
				myStats->RANDOM_MAXHP = 20;
				myStats->RANDOM_HP = myStats->RANDOM_MAXHP;
				//stats->RANDOM_MAXMP = 20;
				//stats->RANDOM_MP = stats->RANDOM_MAXMP;
				myStats->STR = 15;
				myStats->DEX = 5;
				myStats->CON = 5;
				myStats->INT = -1;
				myStats->PER = 0;
				myStats->RANDOM_PER = 5;
				myStats->CHR = -1;
				myStats->LVL = 20;
			}

			// apply random stat increases if set in stat_shared.cpp or editor
			setRandomMonsterStats(myStats, rng);

			// generate 6 items max, less if there are any forced items from boss variants
			int customItemsToGenerate = ITEM_CUSTOM_SLOT_LIMIT;


			// boss variants
			const bool boss =
			    rng.rand() % 50 == 0 &&
			    !my->flags[USERFLAG2] &&
			    !myStats->MISC_FLAGS[STAT_FLAG_DISABLE_MINIBOSS];
			if ( (boss || (*cvar_summonBosses && conductGameChallenges[CONDUCT_CHEATS_ENABLED])) && myStats->leader_uid == 0 )
			{
				myStats->setAttribute("special_npc", "gharbad");
				strcpy(myStats->name, MonsterData_t::getSpecialNPCName(*myStats).c_str());
				my->sprite = MonsterData_t::getSpecialNPCBaseModel(*myStats);
				myStats->sex = MALE;
				myStats->STR += 10;
				myStats->DEX += 2;
				myStats->MAXHP += 75;
				myStats->HP = myStats->MAXHP;
				myStats->OLDHP = myStats->MAXHP;
				myStats->CHR = -1;
				spawnedBoss = true;
				//TODO: Boss stats

				//Spawn in potions.
				int end = rng.rand()%NUM_GOATMAN_BOSS_GHARBAD_POTIONS + 5;
				for ( int i = 0; i < end; ++i )
				{
					switch ( rng.rand()%10 )
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
							newItem(POTION_HEALING, EXCELLENT, 0, 1, rng.rand(), false, &myStats->inventory);
							break;
						default:
							printlog("Tried to spawn goatman boss \"Gharbad\" invalid potion.");
							break;
					}
				}

				newItem(CRYSTAL_SHURIKEN, EXCELLENT, 1 + rng.rand()%1, rng.rand()%NUM_GOATMAN_BOSS_GHARBAD_THROWN_WEAPONS + 2, rng.rand(), true, &myStats->inventory);
			}

			// random effects
			if ( rng.rand() % 8 == 0 )
			{
				my->setEffect(EFF_ASLEEP, true, 1800 + rng.rand() % 1800, false);
			}

			// generates equipment and weapons if available from editor
			createMonsterEquipment(myStats, rng);

			// create any custom inventory items from editor if available
			createCustomInventory(myStats, customItemsToGenerate, rng);

			// count if any custom inventory items from editor
			int customItems = countCustomItems(myStats); //max limit of 6 custom items per entity.

			// count any inventory items set to default in edtior
			int defaultItems = countDefaultItems(myStats);

			my->setHardcoreStats(*myStats);

			bool isShaman = false;
			if ( rng.rand() % 2 && !spawnedBoss && !minion )
			{
				isShaman = true;
				if ( myStats->leader_uid == 0 && !my->flags[USERFLAG2] && rng.rand() % 2 == 0 )
				{
					Entity* entity = summonMonster(GOATMAN, my->x, my->y);
					if ( entity )
					{
						entity->parent = my->getUID();
						if ( Stat* followerStats = entity->getStats() )
						{
							followerStats->leader_uid = entity->parent;
						}
						entity->seedEntityRNG(rng.getU32());
					}
					if ( rng.rand() % 5 == 0 )
					{
						// summon second ally randomly.
						entity = summonMonster(GOATMAN, my->x, my->y);
						if ( entity )
						{
							entity->parent = my->getUID();
							if ( Stat* followerStats = entity->getStats() )
							{
								followerStats->leader_uid = entity->parent;
							}
							entity->seedEntityRNG(rng.getU32());
						}
					}
				}
			}
			else
			{
				myStats->DEX += 1; // more speed for brawlers.
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
					if ( isShaman && rng.rand() % 10 == 0 )
					{
						switch ( rng.rand() % 4 )
						{
							case 0:
								newItem(SPELLBOOK_SLOW, static_cast<Status>(rng.rand() % 3 + DECREPIT), -1 + rng.rand() % 3, 1, rng.rand(), false, &myStats->inventory);
								break;
							case 1:
								newItem(SPELLBOOK_FIREBALL, static_cast<Status>(rng.rand() % 3 + DECREPIT), -1 + rng.rand() % 3, 1, rng.rand(), false, &myStats->inventory);
								break;
							case 2:
								newItem(SPELLBOOK_COLD, static_cast<Status>(rng.rand() % 3 + DECREPIT), -1 + rng.rand() % 3, 1, rng.rand(), false, &myStats->inventory);
								break;
							case 3:
								newItem(SPELLBOOK_FORCEBOLT, static_cast<Status>(rng.rand() % 3 + DECREPIT), -1 + rng.rand() % 3, 1, rng.rand(), false, &myStats->inventory);
								break;
						}
					}
					break;
				default:
					break;
			}


			//Give weapons.
			if ( !spawnedBoss )
			{
				if ( !isShaman && rng.rand() % 3 > 0 )
				{
					newItem(STEEL_CHAKRAM, SERVICABLE, 0, rng.rand()%NUM_GOATMAN_THROWN_WEAPONS + 1, rng.rand(), false, &myStats->inventory);
				}
				int numpotions = rng.rand() % NUM_GOATMAN_POTIONS + 2;
				if ( rng.rand() % 3 == 0 )
				{
					int numhealpotion = rng.rand() % 2 + 1;
					newItem(POTION_HEALING, static_cast<Status>(rng.rand() % 3 + DECREPIT), 0, numhealpotion, rng.rand(), false, &myStats->inventory);
					numpotions -= numhealpotion;
				}
				if ( rng.rand() % 4 > 0 )
				{
					// undroppable
					newItem(POTION_BOOZE, static_cast<Status>(rng.rand() % 3 + DECREPIT), 0, numpotions, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, &myStats->inventory);
				}
			}

			if ( isShaman )
			{
				//give shield
				if ( myStats->shield == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_SHIELD] == 1 )
				{
					// give shield
					switch ( rng.rand() % 20 )
					{
						case 0:
						case 1:
						case 2:
						case 3:
						case 4:
						case 5:
						case 6:
						case 7:
							myStats->shield = newItem(TOOL_CRYSTALSHARD, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
							break;
						case 8:
							myStats->shield = newItem(MIRROR_SHIELD, static_cast<Status>(rng.rand() % 4 + DECREPIT), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
							break;
						default:
							myStats->shield = newItem(TOOL_LANTERN, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
							break;
					}
				}
				// give cloak
				if ( myStats->cloak == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_CLOAK] == 1 )
				{
					switch ( rng.rand() % 10 )
					{
						case 0:
						case 1:
							break;
						default:
							myStats->cloak = newItem(CLOAK, WORN, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
							break;
					}
				}
				// give helmet
				if ( myStats->helmet == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_HELM] == 1 )
				{
					switch ( rng.rand() % 10 )
					{
						case 0:
						case 1:
							myStats->helmet = newItem(HAT_HOOD, WORN, -1 + rng.rand() % 3, 1, 0, false, nullptr);
							break;
						default:
							myStats->helmet = newItem(HAT_WIZARD, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
							break;
					}
				}
				// give armor
				if ( myStats->breastplate == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_ARMOR] == 1 )
				{
					switch ( rng.rand() % 10 )
					{
						case 0:
						case 1:
							myStats->breastplate = newItem(WIZARD_DOUBLET, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
							break;
						case 2:
							myStats->breastplate = newItem(LEATHER_BREASTPIECE, static_cast<Status>(rng.rand() % 3 + DECREPIT), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
							break;
						default:
							break;
					}
				}
				// give booties
				if ( myStats->shoes == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_BOOTS] == 1 )
				{
					switch ( rng.rand() % 20 )
					{
						case 0:
						case 1:
						case 2:
						case 3:
							myStats->shoes = newItem(IRON_BOOTS, static_cast<Status>(rng.rand() % 3 + DECREPIT), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
							break;
						case 19:
							myStats->shoes = newItem(CRYSTAL_BOOTS, static_cast<Status>(rng.rand() % 4 + DECREPIT), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
							break;
						default:
							myStats->shoes = newItem(STEEL_BOOTS, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
							break;
					}
				}
				// give weapon
				if ( myStats->weapon == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1 )
				{
					switch ( rng.rand() % 12 )
					{
						case 0:
						case 1:
						case 2:
						case 3:
						case 4:
							myStats->weapon = newItem(MAGICSTAFF_COLD, static_cast<Status>(rng.rand() % 2 + SERVICABLE), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
							break;
						case 5:
						case 6:
						case 7:
						case 8:
							myStats->weapon = newItem(MAGICSTAFF_FIRE, static_cast<Status>(rng.rand() % 2 + SERVICABLE), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
							break;
						case 9:
							switch ( rng.rand() % 4 )
							{
								case 0:
									myStats->weapon = newItem(SPELLBOOK_SLOW, static_cast<Status>(rng.rand() % 2 + DECREPIT), -1 + rng.rand() % 3, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
									break;
								case 1:
									myStats->weapon = newItem(SPELLBOOK_FIREBALL, static_cast<Status>(rng.rand() % 3 + DECREPIT), -1 + rng.rand() % 3, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
									break;
								case 2:
									myStats->weapon = newItem(SPELLBOOK_COLD, static_cast<Status>(rng.rand() % 3 + DECREPIT), -1 + rng.rand() % 3, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
									break;
								case 3:
									myStats->weapon = newItem(SPELLBOOK_FORCEBOLT, static_cast<Status>(rng.rand() % 3 + DECREPIT), -1 + rng.rand() % 3, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
									break;
							}
							break;
						default:
							myStats->weapon = newItem(MAGICSTAFF_SLOW, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
							break;
					}
				}
			}
			else
			{
				////give shield
				//if ( myStats->shield == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_SHIELD] == 1 )
				//{
				//	switch ( rng.rand() % 20 )
				//	{
				//	case 0:
				//	case 1:
				//	case 2:
				//	case 3:
				//	case 4:
				//	case 5:
				//	case 6:
				//	case 7:
				//		myStats->shield = newItem(TOOL_CRYSTALSHARD, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
				//		break;
				//	case 8:
				//		myStats->shield = newItem(MIRROR_SHIELD, static_cast<Status>(rng.rand() % 4 + DECREPIT), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
				//		break;
				//	default:
				//		myStats->shield = newItem(TOOL_LANTERN, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
				//		break;
				//	}
				//}
				// give cloak
				/*if ( myStats->cloak == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_CLOAK] == 1 )
				{
					switch ( rng.rand() % 10 )
					{
					case 0:
					case 1:
						break;
					default:
						myStats->cloak = newItem(CLOAK, WORN, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						break;
					}
				}*/
				//// give helmet
				//if ( myStats->helmet == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_HELM] == 1 )
				//{
				//	switch ( rng.rand() % 10 )
				//	{
				//	case 0:
				//	case 1:
				//		myStats->helmet = newItem(HAT_HOOD, WORN, -1 + rng.rand() % 3, 1, 0, false, nullptr);
				//		break;
				//	default:
				//		myStats->helmet = newItem(HAT_WIZARD, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, 0, false, nullptr);
				//		break;
				//	}
				//}
				// give armor
				if ( myStats->breastplate == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_ARMOR] == 1 )
				{
					switch ( rng.rand() % 20 )
					{
					case 0:
					case 1:
					case 2:
						myStats->breastplate = newItem(STEEL_BREASTPIECE, static_cast<Status>(rng.rand() % 4 + DECREPIT), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						break;
					case 3:
					case 4:
						myStats->breastplate = newItem(LEATHER_BREASTPIECE, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						break;
					case 5:
					case 6:
						myStats->breastplate = newItem(IRON_BREASTPIECE, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						break;
					case 19:
						myStats->breastplate = newItem(CRYSTAL_BREASTPIECE, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						break;
					default:
						break;
					}
				}
				// give booties
				if ( myStats->shoes == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_BOOTS] == 1 )
				{
					switch ( rng.rand() % 20 )
					{
					case 0:
					case 1:
					case 2:
					case 3:
						myStats->shoes = newItem(IRON_BOOTS, static_cast<Status>(rng.rand() % 3 + DECREPIT), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						break;
					case 19:
						myStats->shoes = newItem(CRYSTAL_BOOTS, static_cast<Status>(rng.rand() % 4 + DECREPIT), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						break;
					default:
						myStats->shoes = newItem(STEEL_BOOTS, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						break;
					}
				}
				// give weapon
				if ( myStats->weapon == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1 )
				{
					switch ( rng.rand() % 20 )
					{
					case 0:
					case 1:
					case 2:
					case 3:
					case 4:
					case 5:
					case 6:
					case 7:
						myStats->weapon = newItem(STEEL_AXE, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						break;
					case 18:
						myStats->weapon = newItem(CRYSTAL_BATTLEAXE, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						break;
					case 19:
						myStats->weapon = newItem(CRYSTAL_MACE, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						break;
					default:
						myStats->weapon = newItem(STEEL_MACE, static_cast<Status>(rng.rand() % 3 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						break;
					}
				}
			}
		}
	}

	// torso
	const int torso_sprite = my->sprite == 1025 ? 1028 :
	    (my->sprite == 1029 ? 1030 : 466);
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
	entity->focalx = limbs[GOATMAN][1][0]; // 0
	entity->focaly = limbs[GOATMAN][1][1]; // 0
	entity->focalz = limbs[GOATMAN][1][2]; // 0
	entity->behavior = &actGoatmanLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right leg
	entity = newEntity(my->sprite == 1025 ? 1027 : 465, 1, map.entities, nullptr); //Limb entity.
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
	my->bodyparts.push_back(entity);

	// left leg
	entity = newEntity(my->sprite == 1025 ? 1026 : 464, 1, map.entities, nullptr); //Limb entity.
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
	my->bodyparts.push_back(entity);

	// right arm
	entity = newEntity(my->sprite == 1025 ? 1023 : 461, 1, map.entities, nullptr); //Limb entity.
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
	my->bodyparts.push_back(entity);

	// left arm
	entity = newEntity(my->sprite == 1025 ? 1021 : 459, 1, map.entities, nullptr); //Limb entity.
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
	entity->focalx = limbs[GOATMAN][7][0]; // 2
	entity->focaly = limbs[GOATMAN][7][1]; // 0
	entity->focalz = limbs[GOATMAN][7][2]; // 0
	entity->behavior = &actGoatmanLimb;
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
	entity->focalx = limbs[GOATMAN][8][0]; // 0
	entity->focaly = limbs[GOATMAN][8][1]; // 0
	entity->focalz = limbs[GOATMAN][8][2]; // 4
	entity->behavior = &actGoatmanLimb;
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
	entity->focalx = limbs[GOATMAN][9][0]; // 0
	entity->focaly = limbs[GOATMAN][9][1]; // 0
	entity->focalz = limbs[GOATMAN][9][2]; // -2
	entity->behavior = &actGoatmanLimb;
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
	entity->focalx = limbs[GOATMAN][10][0]; // 0
	entity->focaly = limbs[GOATMAN][10][1]; // 0
	entity->focalz = limbs[GOATMAN][10][2]; // .25
	entity->behavior = &actGoatmanLimb;
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

void actGoatmanLimb(Entity* my)
{
	my->actMonsterLimb(true);
}

void goatmanDie(Entity* my)
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

	playSoundEntity(my, 338 + local_rng.rand() % 2, 128);

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

	Entity* shieldarm = nullptr;
	Entity* helmet = nullptr;

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
						entity->sprite =
						    my->sprite == 1025 ? 1028 :
						    (my->sprite == 1029 ? 1030 : 466);
					}
					else
					{
						entity->sprite = itemModel(myStats->breastplate);
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
				my->setHumanoidLimbOffset(entity, GOATMAN, LIMB_HUMANOID_TORSO);
				break;
			// right leg
			case LIMB_HUMANOID_RIGHTLEG:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->shoes == nullptr )
					{
						entity->sprite = my->sprite == 1025 ? 1027 : 465;
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
				my->setHumanoidLimbOffset(entity, GOATMAN, LIMB_HUMANOID_RIGHTLEG);
				break;
			// left leg
			case LIMB_HUMANOID_LEFTLEG:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->shoes == nullptr )
					{
						entity->sprite = my->sprite == 1025 ? 1026 : 464;
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
				my->setHumanoidLimbOffset(entity, GOATMAN, LIMB_HUMANOID_LEFTLEG);
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
						entity->sprite = my->sprite == 1025 ? 1023 : 461;
					}
					else
					{
						// else flex arm.
						entity->focalx = limbs[GOATMAN][4][0] + 0.75;
						entity->focaly = limbs[GOATMAN][4][1];
						entity->focalz = limbs[GOATMAN][4][2] - 0.75;
						entity->sprite = my->sprite == 1025 ? 1024 : 462;
					}
				}
				my->setHumanoidLimbOffset(entity, GOATMAN, LIMB_HUMANOID_RIGHTARM);
				entity->yaw += MONSTER_WEAPONYAW;
				break;
				// left arm
			}
			case LIMB_HUMANOID_LEFTARM:
			{
				shieldarm = entity;
				node_t* shieldNode = list_Node(&my->children, 8);
				if ( shieldNode )
				{
					Entity* shield = (Entity*)shieldNode->element;
					if ( shield->flags[INVISIBLE] && my->monsterState == MONSTER_STATE_WAIT )
					{
						entity->focalx = limbs[GOATMAN][5][0]; // 0
						entity->focaly = limbs[GOATMAN][5][1]; // 0
						entity->focalz = limbs[GOATMAN][5][2]; // 2
						entity->sprite = my->sprite == 1025 ? 1021 : 459;
					}
					else
					{
						entity->focalx = limbs[GOATMAN][5][0] + 0.75;
						entity->focaly = limbs[GOATMAN][5][1];
						entity->focalz = limbs[GOATMAN][5][2] - 0.75;
						entity->sprite = my->sprite == 1025 ? 1022 : 460;
					}
				}
				my->setHumanoidLimbOffset(entity, GOATMAN, LIMB_HUMANOID_LEFTARM);
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
				entity->x -= cos(my->yaw);
				entity->y -= sin(my->yaw);
				entity->yaw += PI / 2;
				break;
			// helm
			case LIMB_HUMANOID_HELMET:
				helmet = entity;
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
				entity->focalx = limbs[GOATMAN][10][0]; // 0
				entity->focaly = limbs[GOATMAN][10][1]; // 0
				entity->focalz = limbs[GOATMAN][10][2]; // .25
				entity->pitch = my->pitch;
				entity->roll = PI / 2;
				if ( multiplayer != CLIENT )
				{
					bool hasSteelHelm = false;
					/*if ( myStats->helmet )
					{
						if ( myStats->helmet->type == STEEL_HELM
							|| myStats->helmet->type == CRYSTAL_HELM
							|| myStats->helmet->type == ARTIFACT_HELM )
						{
							hasSteelHelm = true;
						}
					}*/
					if ( myStats->mask == nullptr || myStats->EFFECTS[EFF_INVISIBLE] || wearingring || hasSteelHelm ) //TODO: isInvisible()?
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
				if ( entity->sprite != 165 && entity->sprite != 1196 )
				{
					if ( entity->sprite == items[MASK_SHAMAN].index )
					{
						entity->roll = 0;
						my->setHelmetLimbOffset(entity);
						my->setHelmetLimbOffsetWithMask(helmet, entity);
					}
					else if ( EquipmentModelOffsets.modelOffsetExists(GOATMAN, entity->sprite) )
					{
						my->setHelmetLimbOffset(entity);
						my->setHelmetLimbOffsetWithMask(helmet, entity);
					}
					else
					{
						entity->focalx = limbs[GOATMAN][10][0] + .35; // .35
						entity->focaly = limbs[GOATMAN][10][1] - 2; // -2
						entity->focalz = limbs[GOATMAN][10][2]; // .25
					}
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
		specialRoll = local_rng.rand()%10;

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
							//printlog("Error in Entity::goatmanChooseWeapon(): failed to swap healing potion into hand!");
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
						//printlog("Error in Entity::goatmanChooseWeapon(): failed to swap healing potion into hand!");
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
						//printlog("Error in Entity::goatmanChooseWeapon(): failed to swap healing potion into hand!");
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
			if ( hasPotion && local_rng.rand()%10 )
			{
				tryChakram = false;
			}

			if ( tryChakram )
			{
				//Grab a chakram instead.
				node_t* thrownNode = itemNodeInInventory(myStats, -1, THROWN);
				if ( thrownNode )
				{
					bool swapped = swapMonsterWeaponWithInventoryItem(this, myStats, thrownNode, false, false);
					if ( !swapped )
					{
						//printlog("Error in Entity::goatmanChooseWeapon(): failed to swap THROWN into hand! Cursed? (%d)", myStats->weapon->beatitude);
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
				//printlog("Error in Entity::goatmanChooseWeapon(): failed to swap melee weapon into hand! Cursed? (%d)", myStats->weapon->beatitude);
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
		if ( monsterSpecialTimer == 0 && (ticks % 10 == 0) && monsterAttack == 0 && local_rng.rand() % 10 == 0 )
		{
			weaponNode = itemNodeInInventory(myStats, -1, THROWN);
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

	if ( monsterAllyIndex >= 0 && (monsterAllyClass != ALLY_CLASS_MIXED || item.interactNPCUid == getUID()) )
	{
		return monsterAllyEquipmentInClass(item);
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
			break;
		case TOOL:
			if ( itemTypeIsQuiver(item.type) )
			{
				return true;
			}
			break;
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





