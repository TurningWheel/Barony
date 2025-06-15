/*-------------------------------------------------------------------------------

	BARONY
	File: monster_skeleton.cpp
	Desc: implements all of the skeleton monster's code

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
#include "magic/magic.hpp"
#include "prng.hpp"
#include "scores.hpp"
#include "mod_tools.hpp"

void initSkeleton(Entity* my, Stat* myStats)
{
	int c;
	node_t* node;

	my->flags[BURNABLE] = false;

	//Sprite 229 = Skeleton head model
	my->initMonster(229);
	my->z = -0.5;

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = -1;
		MONSTER_SPOTVAR = 1;
		MONSTER_IDLESND = -1;
		MONSTER_IDLEVAR = 1;
	}
	if ( multiplayer != CLIENT && !MONSTER_INIT )
	{
		auto& rng = my->entity_rng ? *my->entity_rng : local_rng;

		if ( myStats != nullptr )
		{
		    if ( myStats->sex == FEMALE )
		    {
		        my->sprite = 1103;
		    }
			if ( !myStats->leader_uid )
			{
				myStats->leader_uid = 0;
			}

			// apply random stat increases if set in stat_shared.cpp or editor

			if ( my->monsterAllySummonRank != 0 )
			{
				int rank = std::min(my->monsterAllySummonRank, 7);
				bool secondarySummon = true;
				if ( MonsterData_t::nameMatchesSpecialNPCName(*myStats, "skeleton knight") )
				{
					secondarySummon = false;
				}
				my->skeletonSummonSetEquipment(myStats, rank);
				myStats->GOLD = 0;
				my->light = addLight(my->x / 16, my->y / 16, "summoned_skeleton_glow");

				Entity* leader = uidToEntity(myStats->leader_uid);
				if ( leader )
				{
					Stat* leaderStats = leader->getStats();
					if ( leaderStats )
					{
						if ( !secondarySummon )
						{
							if ( leaderStats->playerSummonLVLHP != 0 ) // first stat initialisation if equal to 0
							{
								myStats->LVL = (leaderStats->playerSummonLVLHP & 0xFFFF0000) >> 16;
								myStats->MAXHP = leaderStats->playerSummonLVLHP & 0x0000FFFF;
								myStats->HP = myStats->MAXHP;
								myStats->OLDHP = myStats->MAXHP;

								myStats->STR = (leaderStats->playerSummonSTRDEXCONINT & 0xFF000000) >> 24;
								myStats->DEX = (leaderStats->playerSummonSTRDEXCONINT & 0x00FF0000) >> 16;
								myStats->CON = (leaderStats->playerSummonSTRDEXCONINT & 0x0000FF00) >> 8;
								myStats->INT = (leaderStats->playerSummonSTRDEXCONINT & 0x000000FF) >> 0;

								myStats->PER = (leaderStats->playerSummonPERCHR & 0xFF000000) >> 24;
								myStats->CHR = (leaderStats->playerSummonPERCHR & 0x00FF0000) >> 16;
							}
							else
							{
								// set variables for first time cast
								myStats->HP = 60;
								myStats->MAXHP = myStats->HP;
								myStats->OLDHP = myStats->HP;

								leaderStats->playerSummonLVLHP = (myStats->LVL << 16);
								leaderStats->playerSummonLVLHP |= (myStats->MAXHP);

								myStats->STR = std::max(1, myStats->STR);
								myStats->DEX = std::max(2, myStats->DEX);
								myStats->CON = std::max(0, myStats->CON);
								myStats->INT = std::max(0, myStats->INT);
								myStats->PER = std::max(0, myStats->PER);
								myStats->CHR = std::max(0, myStats->CHR);
								leaderStats->playerSummonSTRDEXCONINT = (myStats->STR << 24);
								leaderStats->playerSummonSTRDEXCONINT |= (myStats->DEX << 16);
								leaderStats->playerSummonSTRDEXCONINT |= (myStats->CON << 8);
								leaderStats->playerSummonSTRDEXCONINT |= (myStats->INT);

								leaderStats->playerSummonPERCHR = (myStats->PER << 24);
								leaderStats->playerSummonPERCHR |= (myStats->CHR << 16);
								leaderStats->playerSummonPERCHR |= (my->monsterAllySummonRank << 8);
							}
						}
						else
						{
							if ( leaderStats->playerSummon2LVLHP != 0 ) // first stat initialisation if equal to 0
							{
								myStats->LVL = (leaderStats->playerSummon2LVLHP & 0xFFFF0000) >> 16;
								myStats->MAXHP = leaderStats->playerSummon2LVLHP & 0x0000FFFF;
								myStats->HP = myStats->MAXHP;
								myStats->OLDHP = myStats->MAXHP;

								myStats->STR = (leaderStats->playerSummon2STRDEXCONINT & 0xFF000000) >> 24;
								myStats->DEX = (leaderStats->playerSummon2STRDEXCONINT & 0x00FF0000) >> 16;
								myStats->CON = (leaderStats->playerSummon2STRDEXCONINT & 0x0000FF00) >> 8;
								myStats->INT = (leaderStats->playerSummon2STRDEXCONINT & 0x000000FF) >> 0;

								myStats->PER = (leaderStats->playerSummon2PERCHR & 0xFF000000) >> 24;
								myStats->CHR = (leaderStats->playerSummon2PERCHR & 0x00FF0000) >> 16;
							}
							else
							{
								// set variables for first time cast
								// make up level deficit from primary summon.
								int levelUps = 0;
								if ( leaderStats->playerSummonLVLHP > 0 )
								{
									levelUps = std::max(3, static_cast<int>((leaderStats->playerSummonLVLHP & 0xFFFF0000) >> 16) - 3);
								}
								levelUps = std::max(0, levelUps - myStats->LVL);
								int increasestat[3] = { 0, 0, 0 };
								for ( int i = 0; i < levelUps; ++i )
								{
									myStats->LVL++;
									myStats->HP += HP_MOD;
									myStats->MAXHP += HP_MOD;
									myStats->HP = std::min(myStats->HP, myStats->MAXHP);
									my->playerStatIncrease(CLASS_ROGUE, increasestat); // rogue weighting
									for ( int j = 0; j < 3; j++ )
									{
										switch ( increasestat[j] )
										{
											case STAT_STR:
												myStats->STR++;
												break;
											case STAT_DEX:
												myStats->DEX++;
												break;
											case STAT_CON:
												myStats->CON++;
												break;
											case STAT_INT:
												myStats->INT++;
												break;
											case STAT_PER:
												myStats->PER++;
												break;
											case STAT_CHR:
												myStats->CHR++;
												break;
											default:
												break;
										}
									}
								}

								my->skeletonSummonSetEquipment(myStats, std::min(7, 1 + (myStats->LVL / 5)));

								leaderStats->playerSummon2LVLHP = (myStats->LVL << 16);
								leaderStats->playerSummon2LVLHP |= (myStats->MAXHP);

								myStats->STR = std::max(0, myStats->STR);
								myStats->DEX = std::max(3, myStats->DEX);
								myStats->CON = std::max(0, myStats->CON);
								myStats->INT = std::max(0, myStats->INT);
								myStats->PER = std::max(1, myStats->PER);
								myStats->CHR = std::max(0, myStats->CHR);
								leaderStats->playerSummon2STRDEXCONINT = (myStats->STR << 24);
								leaderStats->playerSummon2STRDEXCONINT |= (myStats->DEX << 16);
								leaderStats->playerSummon2STRDEXCONINT |= (myStats->CON << 8);
								leaderStats->playerSummon2STRDEXCONINT |= (myStats->INT);

								leaderStats->playerSummon2PERCHR = (myStats->PER << 24);
								leaderStats->playerSummon2PERCHR |= (myStats->CHR << 16);
								leaderStats->playerSummon2PERCHR |= (my->monsterAllySummonRank << 8);
							}
						}

						myStats->MAXMP = getCostOfSpell(&spell_summon, leader);
						myStats->MP = 0;

						if ( multiplayer == SERVER && leader->behavior == &actPlayer )
						{
							serverUpdateAllyStat(leader->skill[2], my->getUID(), myStats->LVL, myStats->HP, myStats->MAXHP, myStats->type);
							serverUpdatePlayerSummonStrength(leader->skill[2]);
							serverUpdateEntitySkill(my, 50); // update the rank of the monster.
						}
					}
					else
					{
						myStats->HP = 0;
					}
				}
				else
				{
					myStats->HP = 0;
				}

				
			}
			else
			{
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
					myStats->setAttribute("special_npc", "funny bones");
					strcpy(myStats->name, MonsterData_t::getSpecialNPCName(*myStats).c_str());
					my->sprite = MonsterData_t::getSpecialNPCBaseModel(*myStats);
			        myStats->sex = MALE;
					myStats->HP = 100;
					myStats->MAXHP = 100;
					myStats->STR += 6;
					int status = DECREPIT + (currentlevel > 5) + (currentlevel > 15) + (currentlevel > 20);
					myStats->weapon = newItem(ARTIFACT_AXE, static_cast<Status>(status), 1, 1, rng.rand(), false, nullptr);
					myStats->cloak = newItem(CLOAK_PROTECTION, WORN, 0, 1, 2, false, nullptr);
					newItem(MASK_EYEPATCH, WORN, -1 + rng.rand() % 2, 1, rng.rand(), false, &myStats->inventory);
			    }
				else
				{
					// not boss if a follower, or name has already been set to something other than blank.
					if ( strncmp(map.name, "Underworld", 10) )
					{
						//give weapon
						if ( myStats->weapon == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1 )
						{
							switch ( rng.rand() % 10 )
							{
								case 0:
								case 1:
									myStats->weapon = newItem(BRONZE_AXE, WORN, -1 + rng.rand() % 2, 1, rng.rand(), false, nullptr);
									break;
								case 2:
								case 3:
									myStats->weapon = newItem(BRONZE_SWORD, WORN, -1 + rng.rand() % 2, 1, rng.rand(), false, nullptr);
									break;
								case 4:
								case 5:
									myStats->weapon = newItem(IRON_SPEAR, WORN, -1 + rng.rand() % 2, 1, rng.rand(), false, nullptr);
									break;
								case 6:
								case 7:
									myStats->weapon = newItem(IRON_AXE, WORN, -1 + rng.rand() % 2, 1, rng.rand(), false, nullptr);
									break;
								case 8:
								case 9:
									myStats->weapon = newItem(IRON_SWORD, WORN, -1 + rng.rand() % 2, 1, rng.rand(), false, nullptr);
									break;
							}
						}
					}
				}

				// random effects

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
				if ( myStats->weapon == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1 )
				{
					switch ( rng.rand() % 10 )
					{
						case 0:
						case 1:
						case 2:
						case 3:
							myStats->weapon = newItem(SHORTBOW, WORN, -1 + rng.rand() % 2, 1, rng.rand(), false, nullptr);
							break;
						case 4:
						case 5:
						case 6:
						case 7:
							myStats->weapon = newItem(CROSSBOW, WORN, -1 + rng.rand() % 2, 1, rng.rand(), false, nullptr);
							break;
						case 8:
						case 9:
							myStats->weapon = newItem(MAGICSTAFF_COLD, EXCELLENT, -1 + rng.rand() % 2, 1, rng.rand(), false, nullptr);
							break;
					}
				}

				//give helmet
				if ( myStats->helmet == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_HELM] == 1 )
				{
					switch ( rng.rand() % 10 )
					{
						case 0:
						case 1:
						case 2:
						case 3:
						case 4:
							break;
						case 5:
							myStats->helmet = newItem(LEATHER_HELM, DECREPIT, -1 + rng.rand() % 2, 1, rng.rand(), false, nullptr);
							break;
						case 6:
						case 7:
						case 8:
						case 9:
							myStats->helmet = newItem(IRON_HELM, DECREPIT, -1 + rng.rand() % 2, 1, rng.rand(), false, nullptr);
							break;
					}
				}

				//give shield
				if ( myStats->shield == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_SHIELD] == 1 )
				{
					if ( myStats->weapon && isRangedWeapon(*myStats->weapon) )
					{
						my->monsterGenerateQuiverItem(myStats);
					}
					else
					{
						switch ( rng.rand() % 10 )
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
								myStats->shield = newItem(WOODEN_SHIELD, DECREPIT, -1 + rng.rand() % 2, 1, rng.rand(), false, nullptr);
								break;
							case 8:
								myStats->shield = newItem(BRONZE_SHIELD, DECREPIT, -1 + rng.rand() % 2, 1, rng.rand(), false, nullptr);
								break;
							case 9:
								myStats->shield = newItem(IRON_SHIELD, DECREPIT, -1 + rng.rand() % 2, 1, rng.rand(), false, nullptr);
								break;
						}
					}
				}
			}
		}
	}

	// torso
	Entity* entity = newEntity(230, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SKELETON][1][0]; // 0
	entity->focaly = limbs[SKELETON][1][1]; // 0
	entity->focalz = limbs[SKELETON][1][2]; // 0
	entity->behavior = &actSkeletonLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right leg
	entity = newEntity(236, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SKELETON][2][0]; // 0
	entity->focaly = limbs[SKELETON][2][1]; // 0
	entity->focalz = limbs[SKELETON][2][2]; // 2
	entity->behavior = &actSkeletonLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left leg
	entity = newEntity(235, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SKELETON][3][0]; // 0
	entity->focaly = limbs[SKELETON][3][1]; // 0
	entity->focalz = limbs[SKELETON][3][2]; // 2
	entity->behavior = &actSkeletonLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right arm
	entity = newEntity(233, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SKELETON][4][0]; // 0
	entity->focaly = limbs[SKELETON][4][1]; // 0
	entity->focalz = limbs[SKELETON][4][2]; // 2
	entity->behavior = &actSkeletonLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left arm
	entity = newEntity(231, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SKELETON][5][0]; // 0
	entity->focaly = limbs[SKELETON][5][1]; // 0
	entity->focalz = limbs[SKELETON][5][2]; // 2
	entity->behavior = &actSkeletonLimb;
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
	entity->focalx = limbs[SKELETON][6][0]; // 2.5
	entity->focaly = limbs[SKELETON][6][1]; // 0
	entity->focalz = limbs[SKELETON][6][2]; // 0
	entity->behavior = &actSkeletonLimb;
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
	entity->focalx = limbs[SKELETON][7][0]; // 2
	entity->focaly = limbs[SKELETON][7][1]; // 0
	entity->focalz = limbs[SKELETON][7][2]; // 0
	entity->behavior = &actSkeletonLimb;
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
	entity->focalx = limbs[SKELETON][8][0]; // 0
	entity->focaly = limbs[SKELETON][8][1]; // 0
	entity->focalz = limbs[SKELETON][8][2]; // 4
	entity->behavior = &actSkeletonLimb;
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
	entity->flags[INVISIBLE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->noColorChangeAllyLimb = 1.0;
	entity->focalx = limbs[SKELETON][9][0]; // 0
	entity->focaly = limbs[SKELETON][9][1]; // 0
	entity->focalz = limbs[SKELETON][9][2]; // -2
	entity->behavior = &actSkeletonLimb;
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
	entity->flags[INVISIBLE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->noColorChangeAllyLimb = 1.0;
	entity->focalx = limbs[SKELETON][10][0]; // 0
	entity->focaly = limbs[SKELETON][10][1]; // 0
	entity->focalz = limbs[SKELETON][10][2]; // .5
	entity->behavior = &actSkeletonLimb;
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

void initRevenantSkull(Entity* my, Stat* myStats)
{
	node_t* node;

	my->z = 0;
	my->initMonster(1796);
	my->flags[INVISIBLE] = true; // hide the "AI" bodypart
	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 666;
		MONSTER_SPOTVAR = 1;
		MONSTER_IDLESND = 667;
		MONSTER_IDLEVAR = 3;
	}

	if ( multiplayer != CLIENT && !MONSTER_INIT )
	{
		auto& rng = my->entity_rng ? *my->entity_rng : local_rng;

		if ( myStats != nullptr )
		{
			if ( !myStats->leader_uid )
			{
				myStats->leader_uid = 0;
			}

			// apply random stat increases if set in stat_shared.cpp or editor
			setRandomMonsterStats(myStats, rng);

			// generate 6 items max, less if there are any forced items from boss variants
			int customItemsToGenerate = ITEM_CUSTOM_SLOT_LIMIT;

			// generates equipment and weapons if available from editor
			createMonsterEquipment(myStats, rng);

			// create any custom inventory items from editor if available
			createCustomInventory(myStats, customItemsToGenerate, rng);

			// count if any custom inventory items from editor
			int customItems = countCustomItems(myStats); //max limit of 6 custom items per entity.

			// count any inventory items set to default in edtior
			int defaultItems = countDefaultItems(myStats);

			my->setHardcoreStats(*myStats);
		}
	}

	// body
	Entity* entity = newEntity(1796, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 2;
	entity->sizey = 2;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	entity->z = 6;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[REVENANT_SKULL][1][0];
	entity->focaly = limbs[REVENANT_SKULL][1][1];
	entity->focalz = limbs[REVENANT_SKULL][1][2];
	entity->behavior = &actRevenantSkullLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);
}

void initAdorcisedWeapon(Entity* my, Stat* myStats)
{
	node_t* node;

	my->z = 0;
	my->initMonster(1797);
	my->flags[INVISIBLE] = true; // hide the "AI" bodypart
	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = -1;
		MONSTER_SPOTVAR = 1;
		MONSTER_IDLESND = -1;
		MONSTER_IDLEVAR = 3;
	}

	if ( multiplayer != CLIENT && !MONSTER_INIT )
	{
		auto& rng = my->entity_rng ? *my->entity_rng : local_rng;

		if ( myStats != nullptr )
		{
			if ( !myStats->leader_uid )
			{
				myStats->leader_uid = 0;
			}

			// apply random stat increases if set in stat_shared.cpp or editor
			setRandomMonsterStats(myStats, rng);

			// generate 6 items max, less if there are any forced items from boss variants
			int customItemsToGenerate = ITEM_CUSTOM_SLOT_LIMIT;

			// generates equipment and weapons if available from editor
			createMonsterEquipment(myStats, rng);

			// create any custom inventory items from editor if available
			createCustomInventory(myStats, customItemsToGenerate, rng);

			// count if any custom inventory items from editor
			int customItems = countCustomItems(myStats); //max limit of 6 custom items per entity.

			// count any inventory items set to default in edtior
			int defaultItems = countDefaultItems(myStats);

			my->setHardcoreStats(*myStats);
		}
	}

	// body
	Entity* entity = newEntity(15, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 2;
	entity->sizey = 2;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	entity->z = 6;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[MONSTER_ADORCISED_WEAPON][1][0];
	entity->focaly = limbs[MONSTER_ADORCISED_WEAPON][1][1];
	entity->focalz = limbs[MONSTER_ADORCISED_WEAPON][1][2];
	entity->behavior = &actAdorcisedWeaponLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);
}

void actSkeletonLimb(Entity* my)
{
	Entity* parent = uidToEntity(my->skill[2]);

	if ( parent && parent->behavior == &actMonster )
	{
		if ( Stat* stats = parent->getStats() )
		{
			const char* lightName = nullptr;
			if ( MonsterData_t::nameMatchesSpecialNPCName(*stats, "skeleton sentinel") )
			{
				lightName = "summoned_skeleton_glow";
			}
			else if ( MonsterData_t::nameMatchesSpecialNPCName(*stats, "skeleton knight") )
			{
				lightName = "summoned_skeleton_glow";
			}

			if ( my->light )
			{
				list_RemoveNode(my->light->node);
				my->light = nullptr;
			}

			if ( lightName )
			{
				my->light = addLight(my->x / 16, my->y / 16, lightName);
				my->actMonsterLimb(false);
				return;
			}
		}
	}
	my->actMonsterLimb(true);
}

void actRevenantSkullLimb(Entity* my)
{
	if ( my->light )
	{
		list_RemoveNode(my->light->node);
		my->light = nullptr;
	}

	my->light = addLight(my->x / 16, my->y / 16, "revenant_skull_glow");
	my->actMonsterLimb(false);
}

void actAdorcisedWeaponLimb(Entity* my)
{
	if ( my->light )
	{
		list_RemoveNode(my->light->node);
		my->light = nullptr;
	}

	my->light = addLight(my->x / 16, my->y / 16, "adorcised_weapon_glow");
	my->actMonsterLimb(false);
}

void skeletonDie(Entity* my)
{
	if ( multiplayer != CLIENT && my->monsterAllySummonRank != 0 )
	{
		Stat* myStats = my->getStats();
		Entity* leader = uidToEntity(myStats->leader_uid);
		if ( leader )
		{
			Stat* leaderStats = leader->getStats();
			if ( leaderStats )
			{
				// refund mana to caster.
				int spellCost = getCostOfSpell(&spell_summon, leader);
				if ( (leader->getINT() + leaderStats->getModifiedProficiency(PRO_MAGIC)) >= SKILL_LEVEL_EXPERT )
				{
					// we summoned 2 units, halve the return rate.
					spellCost /= 2;
				}
				//for ( node_t* node = leaderStats->FOLLOWERS.first; node != nullptr; node = node->next )
				//{
				//	Uint32* c = (Uint32*)node->element;
				//	Entity* otherSummon = uidToEntity(*c);
				//	if ( otherSummon && otherSummon->monsterAllySummonRank != 0 && otherSummon->monsterAllyGetPlayerLeader() == leader )
				//	{
				//		spellCost /= 2;
				//		Stat* otherSummonStat = otherSummon->getStats();
				//		if ( !strcmp(otherSummonStat->name, "skeleton sentinel") )
				//		{
				//			otherSummonStat->MAXMP /= 2; // halve the other summon's possible return MP.
				//			otherSummonStat->MAXMP = std::max(1, otherSummonStat->MAXMP);
				//			otherSummonStat->MP /= 2;
				//		}
				//		break;
				//	}
				//}
				int manaToRefund = std::min(spellCost, static_cast<int>(myStats->MP / static_cast<float>(myStats->MAXMP) * spellCost)); // MP to restore
				if ( manaToRefund > 0 )
				{
					manaToRefund -= local_rng.rand() % (std::max(1, manaToRefund / 3));
					if ( leaderStats->HP <= 0 )
					{
						manaToRefund = 0;
					}
					Entity* spellEntity = createParticleSapCenter(leader, my, SPELL_SUMMON, 599, 791);
					if ( spellEntity )
					{
						playSoundEntity(my, 167, 128); // succeeded spell sound
						spellEntity->skill[7] = manaToRefund;
						if ( leader->behavior == &actPlayer )
						{
							messagePlayerMonsterEvent(leader->skill[2], 0xFFFFFFFF, *myStats, Language::get(3194), Language::get(3195), MSG_COMBAT);
						}
					}
				}
			}
		}
	}

	my->removeMonsterDeathNodes();

	int c;
	for ( c = 0; c < 6; c++ )
	{
		Entity* entity = spawnGib(my);
		if ( entity )
		{
            entity->skill[5] = 1; // poof
            if (my->sprite == 1103) {
			    switch ( c )
			    {
				    case 0:
					    entity->sprite = 1099;
					    break;
				    case 1:
					    entity->sprite = 1101;
					    break;
				    case 2:
					    entity->sprite = 1103;
					    break;
				    case 3:
					    entity->sprite = 1104;
					    break;
				    case 4:
					    entity->sprite = 1105;
					    break;
				    case 5:
					    entity->sprite = 1106;
					    break;
			    }
            }
            else {
			    switch ( c )
			    {
				    case 0:
					    entity->sprite = 229;
					    break;
				    case 1:
					    entity->sprite = 230;
					    break;
				    case 2:
					    entity->sprite = 231;
					    break;
				    case 3:
					    entity->sprite = 233;
					    break;
				    case 4:
					    entity->sprite = 235;
					    break;
				    case 5:
					    entity->sprite = 236;
					    break;
			    }
			}
			serverSpawnGibForClient(entity);
		}
	}
	playSoundEntity(my, 94, 128);
	list_RemoveNode(my->mynode);
	return;
}

void revenantSkullDie(Entity* my)
{
	my->removeMonsterDeathNodes();

	int c;
	for ( c = 0; c < 6; c++ )
	{
		Entity* entity = spawnGib(my);
		if ( entity )
		{
			entity->skill[5] = 1; // poof
			switch ( c )
			{
			case 0:
				entity->sprite = 229;
				break;
			case 1:
				entity->sprite = 230;
				break;
			case 2:
				entity->sprite = 231;
				break;
			case 3:
				entity->sprite = 233;
				break;
			case 4:
				entity->sprite = 235;
				break;
			case 5:
				entity->sprite = 236;
				break;
			}
			serverSpawnGibForClient(entity);
		}
	}
	playSoundEntity(my, 94, 128);
	list_RemoveNode(my->mynode);
	return;
}

void adorcisedWeaponDie(Entity* my)
{
	my->removeMonsterDeathNodes();
	spawnPoof(my->x, my->y, my->z, 1.0, true);
	list_RemoveNode(my->mynode);
	return;
}

#define SKELETONWALKSPEED .13

void skeletonMoveBodyparts(Entity* my, Stat* myStats, double dist)
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
		if ( myStats->getEffectActive(EFF_INVISIBLE) || wearingring == true )
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
		if ( myStats->getEffectActive(EFF_ASLEEP) )
		{
			my->z = 2;
			my->pitch = PI / 4;
		}
		else
		{
			my->z = -.5;
			my->pitch = 0;
		}

		if ( myStats->getEffectActive(EFF_LIFT) )
		{
			my->creatureHoverZ += 0.25;
		}
		else
		{
			my->creatureHoverZ = 0.0;
		}
		real_t height = 2.0 * sin(std::min(my->creatureHoverZ, PI / 2));
		if ( my->creatureHoverZ >= PI / 2 )
		{
			height += 0.5 * cos(my->creatureHoverZ);
		}

		my->z -= 2 * height;
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
		if ( (MONSTER_ATTACK == MONSTER_POSE_MAGIC_WINDUP1) && bodypart == LIMB_HUMANOID_RIGHTARM )
		{
			// don't let the creatures's yaw move the casting arm
		}
		else
		{
			entity->yaw = my->yaw;
		}
		if ( bodypart == LIMB_HUMANOID_RIGHTLEG || bodypart == LIMB_HUMANOID_LEFTARM )
		{
			my->humanoidAnimateWalk(entity, node, bodypart, SKELETONWALKSPEED, dist, 0.1);
		}
		else if ( bodypart == LIMB_HUMANOID_LEFTLEG || bodypart == LIMB_HUMANOID_RIGHTARM || bodypart == LIMB_HUMANOID_CLOAK )
		{
			// left leg, right arm, cloak.
			if ( bodypart == LIMB_HUMANOID_RIGHTARM )
			{
				weaponarm = entity;
				if ( MONSTER_ATTACK > 0 )
				{
					my->handleWeaponArmAttack(entity);
				}
			}
			else if ( bodypart == LIMB_HUMANOID_CLOAK )
			{
				entity->pitch = entity->fskill[0];
			}

			my->humanoidAnimateWalk(entity, node, bodypart, SKELETONWALKSPEED, dist, 0.1);

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
						entity->sprite = my->sprite == 1103 ? 1106 : 230;
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
				my->setHumanoidLimbOffset(entity, SKELETON, LIMB_HUMANOID_TORSO);
				break;
			// right leg
			case LIMB_HUMANOID_RIGHTLEG:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->shoes == nullptr )
					{
						entity->sprite = my->sprite == 1103 ? 1105 : 236;
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
				my->setHumanoidLimbOffset(entity, SKELETON, LIMB_HUMANOID_RIGHTLEG);
				break;
			// left leg
			case LIMB_HUMANOID_LEFTLEG:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->shoes == nullptr )
					{
						entity->sprite = my->sprite == 1103 ? 1104 : 235;
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
				my->setHumanoidLimbOffset(entity, SKELETON, LIMB_HUMANOID_LEFTLEG);
				break;
			// right arm
			case LIMB_HUMANOID_RIGHTARM:
			{
				if ( multiplayer != CLIENT )
				{
					if ( myStats->gloves == nullptr )
					{
						entity->sprite = my->sprite == 1103 ? 1101 : 233;
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
						if ( entity->sprite == 233 || entity->sprite == 1101 )
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
						entity->sprite = my->sprite == 1103 ? 1101 : 233;
					}
					else
					{
						entity->sprite = entity->skill[7];
					}
				}

				node_t* weaponNode = list_Node(&my->children, LIMB_HUMANOID_WEAPON);
				if ( weaponNode )
				{
					Entity* weapon = (Entity*)weaponNode->element;
					if ( MONSTER_ARMBENDED || (weapon->flags[INVISIBLE] && my->monsterAttack == 0 ) )
					{
						// if weapon invisible and I'm not attacking, relax arm.
						entity->focalx = limbs[SKELETON][4][0]; // 0
						entity->focaly = limbs[SKELETON][4][1]; // 0
						entity->focalz = limbs[SKELETON][4][2]; // 2
						//entity->sprite = my->sprite == 1103 ? 1101 : 233;
					}
					else
					{
						// else flex arm.
						entity->focalx = limbs[SKELETON][4][0] + 1; // 1
						entity->focaly = limbs[SKELETON][4][1]; // 0
						entity->focalz = limbs[SKELETON][4][2] - 1; // 1
						if ( entity->sprite == 233 || entity->sprite == 1101 )
						{
							entity->sprite = my->sprite == 1103 ? 1102 : 234;
						}
						else
						{
							entity->sprite += 2;
						}
					}
				}
				my->setHumanoidLimbOffset(entity, SKELETON, LIMB_HUMANOID_RIGHTARM);
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
						entity->sprite = my->sprite == 1103 ? 1099 : 231;
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
						if ( entity->sprite == 231 || entity->sprite == 1099 )
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
						entity->sprite = my->sprite == 1103 ? 1099 : 231;
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
					if ( shield->flags[INVISIBLE] )
					{
						// if shield invisible, relax arm.
						//entity->sprite = my->sprite == 1103 ? 1099 : 231;
						entity->focalx = limbs[SKELETON][5][0]; // 0
						entity->focaly = limbs[SKELETON][5][1]; // 0
						entity->focalz = limbs[SKELETON][5][2]; // 2
					}
					else
					{
						// else flex arm.
						entity->focalx = limbs[SKELETON][5][0] + 1; // 1
						entity->focaly = limbs[SKELETON][5][1]; // 0
						entity->focalz = limbs[SKELETON][5][2] - 1; // 1
						if ( entity->sprite == 231 || entity->sprite == 1099 )
						{
							entity->sprite = my->sprite == 1103 ? 1100 : 232;
						}
						else
						{
							entity->sprite += 2;
						}
					}
				}
				my->setHumanoidLimbOffset(entity, SKELETON, LIMB_HUMANOID_LEFTARM);
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
				entity->focalx = limbs[SKELETON][9][0]; // 0
				entity->focaly = limbs[SKELETON][9][1]; // 0
				entity->focalz = limbs[SKELETON][9][2]; // -2
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
				entity->focalx = limbs[SKELETON][10][0]; // 0
				entity->focaly = limbs[SKELETON][10][1]; // 0
				entity->focalz = limbs[SKELETON][10][2]; // .5
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
				if ( entity->sprite != 165 && entity->sprite != 1196 )
				{
					if ( entity->sprite == items[MASK_SHAMAN].index )
					{
						entity->roll = 0;
						my->setHelmetLimbOffset(entity);
						my->setHelmetLimbOffsetWithMask(helmet, entity);
					}
					else if ( EquipmentModelOffsets.modelOffsetExists(SKELETON, entity->sprite, my->sprite) )
					{
						my->setHelmetLimbOffset(entity);
						my->setHelmetLimbOffsetWithMask(helmet, entity);
					}
					else
					{
						entity->focalx = limbs[SKELETON][10][0] + .35; // .35
						entity->focaly = limbs[SKELETON][10][1] - 2; // -2
						entity->focalz = limbs[SKELETON][10][2]; // .5
					}
				}
				else
				{
					entity->focalx = limbs[SKELETON][10][0] + .25; // .25
					entity->focaly = limbs[SKELETON][10][1] - 2.25; // -2.25
					entity->focalz = limbs[SKELETON][10][2]; // .5
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

void Entity::skeletonSummonSetEquipment(Stat* myStats, int rank)
{
	if ( MonsterData_t::nameMatchesSpecialNPCName(*myStats, "skeleton knight") )
	{
		switch ( rank )
		{
			case 1:
				if ( !myStats->weapon )
				{
					myStats->weapon = newItem(BRONZE_SWORD, EXCELLENT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->weapon->type = BRONZE_SWORD;
				}
				if ( !myStats->breastplate )
				{
					myStats->breastplate = newItem(LEATHER_BREASTPIECE, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->breastplate->type = LEATHER_BREASTPIECE;
				}
				if ( !myStats->shield )
				{
					myStats->shield = newItem(BRONZE_SHIELD, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->shield->type = BRONZE_SHIELD;
				}
				break;
			case 2:
				if ( !myStats->weapon )
				{
					myStats->weapon = newItem(IRON_SPEAR, EXCELLENT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->weapon->type = IRON_SPEAR;
				}
				if ( !myStats->breastplate )
				{
					myStats->breastplate = newItem(LEATHER_BREASTPIECE, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->breastplate->type = LEATHER_BREASTPIECE;
				}
				if ( !myStats->shield )
				{
					myStats->shield = newItem(BRONZE_SHIELD, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->shield->type = BRONZE_SHIELD;
				}
				if ( !myStats->shoes )
				{
					myStats->shoes = newItem(IRON_BOOTS, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->shoes->type = IRON_BOOTS;
				}
				break;
			case 3:
				if ( !myStats->weapon )
				{
					myStats->weapon = newItem(IRON_SPEAR, EXCELLENT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->weapon->type = IRON_SPEAR;
				}
				if ( !myStats->breastplate )
				{
					myStats->breastplate = newItem(IRON_BREASTPIECE, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->breastplate->type = IRON_BREASTPIECE;
				}
				if ( !myStats->shield )
				{
					myStats->shield = newItem(IRON_SHIELD, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->shield->type = IRON_SHIELD;
				}
				if ( !myStats->shoes )
				{
					myStats->shoes = newItem(IRON_BOOTS, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->shoes->type = IRON_BOOTS;
				}
				break;
			case 4:
				if ( !myStats->weapon )
				{
					myStats->weapon = newItem(STEEL_HALBERD, EXCELLENT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->weapon->type = STEEL_HALBERD;
				}
				if ( !myStats->breastplate )
				{
					myStats->breastplate = newItem(IRON_BREASTPIECE, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->breastplate->type = IRON_BREASTPIECE;
				}
				if ( !myStats->shield )
				{
					myStats->shield = newItem(IRON_SHIELD, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->shield->type = IRON_SHIELD;
				}
				if ( !myStats->shoes )
				{
					myStats->shoes = newItem(STEEL_BOOTS, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->shoes->type = STEEL_BOOTS;
				}
				break;
			case 5:
				if ( !myStats->weapon )
				{
					myStats->weapon = newItem(STEEL_HALBERD, EXCELLENT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->weapon->type = STEEL_HALBERD;
				}
				if ( !myStats->breastplate )
				{
					myStats->breastplate = newItem(STEEL_BREASTPIECE, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->breastplate->type = STEEL_BREASTPIECE;
				}
				if ( !myStats->shield )
				{
					myStats->shield = newItem(STEEL_SHIELD, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->shield->type = STEEL_SHIELD;
				}
				if ( !myStats->shoes )
				{
					myStats->shoes = newItem(STEEL_BOOTS, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->shoes->type = STEEL_BOOTS;
				}
				break;
			case 6:
				if ( !myStats->weapon )
				{
					myStats->weapon = newItem(CRYSTAL_SPEAR, EXCELLENT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->weapon->type = CRYSTAL_SPEAR;
				}
				if ( !myStats->breastplate )
				{
					myStats->breastplate = newItem(STEEL_BREASTPIECE, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->breastplate->type = STEEL_BREASTPIECE;
				}
				if ( !myStats->shield )
				{
					myStats->shield = newItem(STEEL_SHIELD, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->shield->type = STEEL_SHIELD;
				}
				if ( !myStats->shoes )
				{
					myStats->shoes = newItem(CRYSTAL_BOOTS, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->shoes->type = CRYSTAL_BOOTS;
				}
				break;
			case 7:
				if ( !myStats->weapon )
				{
					myStats->weapon = newItem(CRYSTAL_SPEAR, EXCELLENT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->weapon->type = CRYSTAL_SPEAR;
				}
				if ( !myStats->breastplate )
				{
					myStats->breastplate = newItem(CRYSTAL_BREASTPIECE, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->breastplate->type = CRYSTAL_BREASTPIECE;
				}
				if ( !myStats->shield )
				{
					myStats->shield = newItem(CRYSTAL_SHIELD, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->shield->type = CRYSTAL_SHIELD;
				}
				if ( !myStats->shoes )
				{
					myStats->shoes = newItem(CRYSTAL_BOOTS, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->shoes->type = CRYSTAL_BOOTS;
				}
				break;
			default:
				break;
		}
	}
	else if ( MonsterData_t::nameMatchesSpecialNPCName(*myStats, "skeleton sentinel") )
	{
		switch ( rank )
		{
			case 1:
				if ( !myStats->weapon )
				{
					myStats->weapon = newItem(SLING, EXCELLENT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->weapon->type = SLING;
				}
				if ( !myStats->helmet )
				{
					myStats->helmet = newItem(HAT_HOOD, DECREPIT, 0, 1, 1, false, nullptr);
				}
				else
				{
					myStats->helmet->type = HAT_HOOD;
				}
				if ( !myStats->cloak )
				{
					myStats->cloak = newItem(CLOAK, DECREPIT, 0, 1, 1, false, nullptr);
				}
				else
				{
					myStats->cloak->type = CLOAK;
				}
				break;
			case 2:
				if ( !myStats->weapon )
				{
					myStats->weapon = newItem(SLING, EXCELLENT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->weapon->type = SLING;
				}
				if ( !myStats->helmet )
				{
					myStats->helmet = newItem(HAT_HOOD, DECREPIT, 0, 1, 1, false, nullptr);
				}
				else
				{
					myStats->helmet->type = HAT_HOOD;
				}
				if ( !myStats->cloak )
				{
					myStats->cloak = newItem(CLOAK, DECREPIT, 0, 1, 1, false, nullptr);
				}
				else
				{
					myStats->cloak->type = CLOAK;
				}
				if ( !myStats->shoes )
				{
					myStats->shoes = newItem(LEATHER_BOOTS, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->shoes->type = LEATHER_BOOTS;
				}
				break;
			case 3:
				if ( !myStats->weapon )
				{
					myStats->weapon = newItem(SHORTBOW, EXCELLENT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->weapon->type = SHORTBOW;
				}
				if ( !myStats->helmet )
				{
					myStats->helmet = newItem(HAT_HOOD, DECREPIT, 0, 1, 1, false, nullptr);
				}
				else
				{
					myStats->helmet->type = HAT_HOOD;
				}
				if ( !myStats->cloak )
				{
					myStats->cloak = newItem(CLOAK, DECREPIT, 0, 1, 1, false, nullptr);
				}
				else
				{
					myStats->cloak->type = CLOAK;
				}
				if ( !myStats->shoes )
				{
					myStats->shoes = newItem(LEATHER_BOOTS, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->shoes->type = LEATHER_BOOTS;
				}
				break;
			case 4:
				if ( !myStats->weapon )
				{
					myStats->weapon = newItem(SHORTBOW, EXCELLENT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->weapon->type = SHORTBOW;
				}
				if ( !myStats->helmet )
				{
					myStats->helmet = newItem(HAT_HOOD, DECREPIT, 0, 1, 1, false, nullptr);
				}
				else
				{
					myStats->helmet->type = HAT_HOOD;
				}
				if ( !myStats->cloak )
				{
					myStats->cloak = newItem(CLOAK, DECREPIT, 0, 1, 1, false, nullptr);
				}
				else
				{
					myStats->cloak->type = CLOAK;
				}
				if ( !myStats->shoes )
				{
					myStats->shoes = newItem(IRON_BOOTS, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->shoes->type = IRON_BOOTS;
				}
				break;
			case 5:
				if ( !myStats->weapon )
				{
					myStats->weapon = newItem(CROSSBOW, EXCELLENT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->weapon->type = CROSSBOW;
				}
				if ( !myStats->helmet )
				{
					myStats->helmet = newItem(HAT_HOOD, DECREPIT, 0, 1, 1, false, nullptr);
				}
				else
				{
					myStats->helmet->type = HAT_HOOD;
				}
				if ( !myStats->cloak )
				{
					myStats->cloak = newItem(CLOAK, DECREPIT, 0, 1, 1, false, nullptr);
				}
				else
				{
					myStats->cloak->type = CLOAK;
				}
				if ( !myStats->gloves )
				{
					myStats->gloves = newItem(BRACERS, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->gloves->type = BRACERS;
				}
				if ( !myStats->shoes )
				{
					myStats->shoes = newItem(IRON_BOOTS, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->shoes->type = IRON_BOOTS;
				}
				break;
			case 6:
				if ( !myStats->weapon )
				{
					myStats->weapon = newItem(CROSSBOW, EXCELLENT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->weapon->type = CROSSBOW;
				}
				if ( !myStats->helmet )
				{
					myStats->helmet = newItem(HAT_HOOD, DECREPIT, 0, 1, 1, false, nullptr);
				}
				else
				{
					myStats->helmet->type = HAT_HOOD;
				}
				if ( !myStats->cloak )
				{
					myStats->cloak = newItem(CLOAK, DECREPIT, 0, 1, 1, false, nullptr);
				}
				else
				{
					myStats->cloak->type = CLOAK;
				}
				if ( !myStats->gloves )
				{
					myStats->gloves = newItem(BRACERS, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->gloves->type = BRACERS;
				}
				if ( !myStats->shoes )
				{
					myStats->shoes = newItem(STEEL_BOOTS_FEATHER, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->shoes->type = STEEL_BOOTS_FEATHER;
				}
				break;
			case 7:
				if ( !myStats->weapon )
				{
					myStats->weapon = newItem(CROSSBOW, EXCELLENT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->weapon->type = CROSSBOW;
				}
				if ( !myStats->helmet )
				{
					myStats->helmet = newItem(HAT_HOOD, DECREPIT, 0, 1, 2, false, nullptr);
				}
				else
				{
					myStats->helmet->type = HAT_HOOD;
					myStats->helmet->appearance = 2;
				}
				if ( !myStats->mask )
				{
					myStats->mask = newItem(MASK_BANDIT, DECREPIT, 0, 1, 0, false, nullptr);
				}
				else
				{
					myStats->mask->type = MASK_BANDIT;
					myStats->mask->appearance = 0;
				}
				if ( !myStats->cloak )
				{
					myStats->cloak = newItem(CLOAK_BLACK, DECREPIT, 0, 1, 1, false, nullptr);
				}
				else
				{
					myStats->cloak->type = CLOAK_BLACK;
				}
				if ( !myStats->gloves )
				{
					myStats->gloves = newItem(CRYSTAL_GLOVES, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->gloves->type = CRYSTAL_GLOVES;
				}
				if ( !myStats->shoes )
				{
					myStats->shoes = newItem(STEEL_BOOTS_FEATHER, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->shoes->type = STEEL_BOOTS_FEATHER;
				}
				break;
			default:
				break;
		}

		if ( myStats->helmet )
		{
			myStats->helmet->isDroppable = false;
		}
		if ( myStats->cloak )
		{
			myStats->cloak->isDroppable = false;
		}
		if ( myStats->mask )
		{
			myStats->mask->isDroppable = false;
		}
	}
}

#define REVENANT_SKULL_BODY 2
#define SKULL_FLOAT_X body->fskill[2]
#define SKULL_FLOAT_Y body->fskill[3]
#define SKULL_FLOAT_Z body->fskill[4]
#define SKULL_FLOAT_ATK body->fskill[5]
#define SKULL_CIRCLE_ANIM body->fskill[6]
#define SKULL_CIRCLE_SCALE body->fskill[7]
#define SKULL_BOB_ANIM body->fskill[8]
#define SKULL_FLIP body->fskill[9]
#define SKULL_BOBS body->skill[3]
#define SKULL_CIRCLES body->skill[4]
#define SKULL_CIRCLES_DECREMENT_MODE body->skill[5]
#define SKULL_NEXTBOB body->skill[6]
#define SKULL_IDLE_TIMER body->skill[7]

void revenantSkullAnimate(Entity* my, Stat* myStats, double dist)
{
	node_t* node;
	Entity* entity = nullptr;
	Entity* head = nullptr;
	int bodypart;

	my->flags[INVISIBLE] = true; // hide the "AI" bodypart

	my->sizex = 2;
	my->sizey = 2;

	Monster monsterType = my->sprite == 1797 ? MONSTER_ADORCISED_WEAPON : REVENANT_SKULL;

	my->focalx = limbs[monsterType][0][0];
	my->focaly = limbs[monsterType][0][1];
	my->focalz = limbs[monsterType][0][2];
	if ( multiplayer != CLIENT )
	{
		my->z = limbs[monsterType][5][2];
		if ( myStats && !myStats->getEffectActive(EFF_LEVITATING) )
		{
			myStats->setEffectActive(EFF_LEVITATING, 1);
			myStats->EFFECTS_TIMERS[EFF_LEVITATING] = 0;
		}

		if ( monsterType == MONSTER_ADORCISED_WEAPON )
		{
			if ( myStats->getAttribute("spirit_weapon") != "" )
			{
				my->flags[PASSABLE] = true;
			}

			if ( !myStats->weapon )
			{
				myStats->HP = 0;
				char buf[120];
				snprintf(buf, sizeof(buf), Language::get(6620), myStats->weapon->getName());
				my->setObituary(buf);
			}
			else
			{
				if ( myStats->getAttribute("spirit_weapon") != "" )
				{
					if ( my->parent != 0 )
					{
						Entity* parent = uidToEntity(my->parent);
						if ( !parent )
						{
							myStats->HP = 0;
							if ( myStats->weapon )
							{
								char buf[120];
								snprintf(buf, sizeof(buf), Language::get(6619));
								my->setObituary(buf);
							}
						}
					}
					if ( my->ticks > std::stoi(myStats->getAttribute("spirit_weapon")) )
					{
						myStats->HP = 0;
						if ( myStats->weapon )
						{
							char buf[120];
							snprintf(buf, sizeof(buf), Language::get(6619));
							my->setObituary(buf);
						}
					}
				}
				if ( myStats->getAttribute("adorcised_weapon") != "" )
				{
					if ( my->ticks > std::stoi(myStats->getAttribute("adorcised_weapon")) )
					{
						myStats->HP = 0;
						if ( myStats->weapon )
						{
							char buf[120];
							snprintf(buf, sizeof(buf), Language::get(6619));
							my->setObituary(buf);
						}
					}
				}
			}
		}
	}

	if ( enableDebugKeys && (svFlags & SV_FLAG_CHEATS) )
	{
		if ( keystatus[SDLK_g] )
		{
			keystatus[SDLK_g] = 0;
			MONSTER_ATTACK = MONSTER_POSE_MELEE_WINDUP1;
			MONSTER_ATTACKTIME = 0;
		}
		if ( keystatus[SDLK_h] )
		{
			keystatus[SDLK_h] = 0;
			MONSTER_ATTACK = MONSTER_POSE_MAGIC_WINDUP1;
			MONSTER_ATTACKTIME = 0;
		}
		if ( keystatus[SDLK_j] && myStats )
		{
			keystatus[SDLK_j] = 0;
			myStats->setEffectValueUnsafe(EFF_PARALYZED, myStats->getEffectActive(EFF_PARALYZED) ? 0 : 1);
		}
	}

	bool adorcisedWeapon = monsterType == MONSTER_ADORCISED_WEAPON;
	bool poke = false;

	Entity* body = nullptr;
	for ( bodypart = 0, node = my->children.first; node != nullptr; node = node->next, ++bodypart )
	{
		if ( bodypart < REVENANT_SKULL_BODY )
		{
			continue;
		}

		entity = (Entity*)node->element;
		entity->x = my->x;
		entity->y = my->y;
		entity->z = my->z;
		entity->yaw = my->yaw;

		if ( bodypart == REVENANT_SKULL_BODY )
		{
			if ( adorcisedWeapon )
			{
				if (   entity->sprite == items[QUARTERSTAFF].index
					|| entity->sprite == items[IRON_SPEAR].index
					|| entity->sprite == items[STEEL_HALBERD].index
					|| entity->sprite == items[ARTIFACT_SPEAR].index
					|| entity->sprite == items[CRYSTAL_SPEAR].index )
				{
					poke = true;
				}
				if ( entity->sprite ==   items[BRONZE_SWORD].index
					|| entity->sprite == items[IRON_SWORD].index
					|| entity->sprite == items[STEEL_SWORD].index
					|| entity->sprite == items[ARTIFACT_SWORD].index
					|| entity->sprite == items[CRYSTAL_SWORD].index
					|| entity->sprite == items[RAPIER].index )
				{
					poke = true;
				}
			}

			body = entity;
			entity->fskill[0] = fmod(entity->fskill[0], 2 * PI);
			while ( entity->fskill[0] >= PI )
			{
				entity->fskill[0] -= 2 * PI;
			}
			while ( entity->fskill[0] < -PI )
			{
				entity->fskill[0] += 2 * PI;
			}

			real_t basePitchSetpoint = (limbs[monsterType][14][0] * PI / 180.0);
			if ( MONSTER_ATTACK == 0 )
			{
				real_t speed = -0.05;
				if ( adorcisedWeapon )
				{
					speed = -0.2;
				}
				real_t setpoint = basePitchSetpoint;
				if ( limbAngleWithinRange(entity->fskill[0], speed, setpoint) )
				{
					entity->fskill[0] = setpoint;
				}
				else if ( entity->fskill[0] < (setpoint - 0.01) )
				{
					entity->fskill[0] += -speed;
					entity->fskill[0] = std::min(entity->fskill[0], setpoint);
				}
				else
				{
					entity->fskill[0] += speed;
					entity->fskill[0] = std::max(entity->fskill[0], setpoint);
				}
			}

			if ( !adorcisedWeapon )
			{
				if ( MONSTER_ATTACK > 0 )
				{
					entity->fskill[0] = basePitchSetpoint;
				}
			}

			if ( MONSTER_ATTACK == 0 )
			{
				SKULL_FLOAT_ATK = 0.0;
			}

			if ( multiplayer != CLIENT )
			{
				if ( SKULL_IDLE_TIMER == 0 )
				{
					if ( my->ticks < TICKS_PER_SECOND / 10 )
					{
						SKULL_IDLE_TIMER = TICKS_PER_SECOND / 10;
					}
					else
					{
						SKULL_IDLE_TIMER = (local_rng.rand() % 7 + 5) * TICKS_PER_SECOND;
					}
				}
				else if ( MONSTER_ATTACK == 0 )
				{
					if ( SKULL_IDLE_TIMER > 0 )
					{
						--SKULL_IDLE_TIMER;
						if ( SKULL_IDLE_TIMER == 0 )
						{
							int pick = MONSTER_POSE_RANGED_WINDUP1 + local_rng.rand() % 3;
							if ( my->ticks < TICKS_PER_SECOND * 5 )
							{
								pick = MONSTER_POSE_RANGED_WINDUP1;
							}

							if ( pick == MONSTER_POSE_RANGED_WINDUP2 )
							{
								if ( adorcisedWeapon && myStats->getAttribute("spirit_weapon") != "" )
								{
									pick = MONSTER_POSE_RANGED_WINDUP3;
								}
								else if ( my->monsterState != MONSTER_STATE_WAIT )
								{
									pick = MONSTER_POSE_RANGED_WINDUP3;
								}
							}

							if ( pick == MONSTER_POSE_RANGED_WINDUP2 )
							{
								my->setEffect(EFF_STUNNED, true, 4 * TICKS_PER_SECOND, false);
							}
							my->attack(pick, 0, nullptr);
						}
					}
				}
			}

			if ( MONSTER_ATTACK == MONSTER_POSE_RANGED_WINDUP1 )
			{
				if ( MONSTER_ATTACKTIME == 0 )
				{
					if ( SKULL_FLIP <= 0.0 )
					{
						SKULL_FLIP = 1.0;
					}
					if ( SKULL_BOBS == 0 )
					{
						SKULL_BOBS = 5;
						SKULL_BOB_ANIM = 0.0;
					}
					else
					{
						SKULL_NEXTBOB = 5;
					}
				}
				MONSTER_ATTACK = 0;
			}
			else if ( MONSTER_ATTACK == MONSTER_POSE_RANGED_WINDUP2 )
			{
				if ( MONSTER_ATTACKTIME == 0 )
				{
					if ( SKULL_FLIP <= 0.0 )
					{
						SKULL_FLIP = 1.0;
					}
					if ( SKULL_CIRCLES == 0 )
					{
						SKULL_CIRCLES = 2;
						SKULL_CIRCLE_ANIM = 0.0;
						SKULL_CIRCLE_SCALE = 0.0;
						SKULL_CIRCLES_DECREMENT_MODE = 0;
					}
					if ( SKULL_BOBS == 0 )
					{
						SKULL_BOBS = 5;
						SKULL_BOB_ANIM = 0.0;
					}
					else
					{
						SKULL_NEXTBOB = 5;
					}
				}
				MONSTER_ATTACK = 0;
			}
			else if ( MONSTER_ATTACK == MONSTER_POSE_RANGED_WINDUP3 )
			{
				if ( MONSTER_ATTACKTIME == 0 )
				{
					if ( SKULL_BOBS == 0 )
					{
						SKULL_BOBS = 5;
						SKULL_BOB_ANIM = 0.0;
					}
					else
					{
						SKULL_NEXTBOB = 5;
					}
				}
				MONSTER_ATTACK = 0;
			}
			else if ( MONSTER_ATTACK == MONSTER_POSE_MELEE_WINDUP1 || MONSTER_ATTACK == MONSTER_POSE_MAGIC_WINDUP1 
				|| MONSTER_ATTACK == 1
				|| MONSTER_ATTACK == MONSTER_POSE_MAGIC_CAST1 )
			{
				int delay = (MONSTER_ATTACK == MONSTER_POSE_MAGIC_CAST1 || MONSTER_ATTACK == MONSTER_POSE_MAGIC_WINDUP1) ? 25 : 0;
				if ( MONSTER_ATTACKTIME == 0 )
				{
					entity->fskill[0] = basePitchSetpoint;
					entity->skill[1] = 0;
					SKULL_FLOAT_ATK = 0.0;

					if ( MONSTER_ATTACK == MONSTER_POSE_MAGIC_WINDUP1 )
					{
						if ( multiplayer != CLIENT )
						{
							my->setEffect(EFF_STUNNED, true, delay, false);
						}
						createParticleDot(my);
						// play casting sound
						playSoundEntityLocal(my, 170, 64);
						if ( SKULL_CIRCLES == 0 )
						{
							SKULL_CIRCLE_ANIM = 0.0;
							SKULL_CIRCLE_SCALE = 0.0;
						}
						SKULL_CIRCLES = std::min(2, SKULL_CIRCLES + 2);
						if ( SKULL_BOBS == 0 )
						{
							SKULL_BOBS = 3;
							SKULL_BOB_ANIM = 0.0;
						}
						else
						{
							SKULL_NEXTBOB = 3;
						}
					}
				}
				else
				{
					if ( MONSTER_ATTACKTIME >= (int)limbs[monsterType][15][0] + delay )
					{
						if ( MONSTER_ATTACKTIME == (int)limbs[monsterType][15][0] + delay )
						{
							SKULL_CIRCLES = 0;

							if ( multiplayer != CLIENT )
							{
								const Sint32 temp = MONSTER_ATTACKTIME;
								my->attack(MONSTER_ATTACK == MONSTER_POSE_MAGIC_WINDUP1 ? MONSTER_POSE_MAGIC_CAST1 : 1, 0, nullptr); // slop
								MONSTER_ATTACKTIME = temp;

								if ( adorcisedWeapon && myStats->getAttribute("spirit_weapon") != "" )
								{
									// knockback to lunge forward
									if ( my->setEffect(EFF_KNOCKBACK, true, 30, false) )
									{
										real_t pushbackMultiplier = 3.0;
										real_t tangent = my->yaw;
										my->vel_x = cos(tangent) * pushbackMultiplier;
										my->vel_y = sin(tangent) * pushbackMultiplier;
										my->monsterKnockbackVelocity = 0.025;
										my->monsterKnockbackUID = 0;
										my->monsterKnockbackTangentDir = tangent;
									}
								}
							}
						}

						if ( !adorcisedWeapon )
						{
							if ( entity->skill[1] == 0 )
							{
								real_t speed = limbs[monsterType][13][2];
								real_t setpoint = (limbs[monsterType][14][2] * PI / 180.0);
								if ( limbAngleWithinRange(entity->fskill[0], -speed, setpoint) )
								{
									entity->fskill[0] = setpoint;
									entity->skill[1] = 1;
								}
								else
								{
									entity->fskill[0] -= speed;
									entity->fskill[0] = std::max(entity->fskill[0], setpoint);
								}
							}
							else
							{
								real_t speed = limbs[monsterType][13][1];
								entity->fskill[0] += speed;
								entity->fskill[0] = std::min(entity->fskill[0], basePitchSetpoint);
							}
						}
						else
						{
							if ( poke )
							{
								static ConsoleVariable<float> cvar_revenant_pokeset("/revenant_pokeset", 90);
								static ConsoleVariable<float> cvar_revenant_pokespd("/revenant_pokespd", 0.01);
								static ConsoleVariable<float> cvar_revenant_pokespd2("/revenant_pokespd2", -0.25);
								if ( entity->skill[1] == 0 )
								{
									real_t speed = *cvar_revenant_pokespd;
									real_t setpoint = (*cvar_revenant_pokeset * PI / 180.0);
									if ( limbAngleWithinRange(entity->fskill[0], -speed, setpoint) )
									{
										entity->fskill[0] = setpoint;
										entity->skill[1] = 1;
									}
									else
									{
										entity->fskill[0] -= speed;
									}
								}
								else
								{
									real_t speed = *cvar_revenant_pokespd2;
									if ( limbAngleWithinRange(entity->fskill[0], speed, basePitchSetpoint) )
									{
										entity->fskill[0] = basePitchSetpoint;
									}
									else
									{
										entity->fskill[0] += speed;
									}
								}
							}
							else
							{
								if ( entity->skill[1] == 0 )
								{
									real_t speed = limbs[monsterType][3][2];
									real_t setpoint = (limbs[monsterType][4][2] * PI / 180.0);
									if ( limbAngleWithinRange(entity->fskill[0], -speed, setpoint) )
									{
										entity->fskill[0] = setpoint;
										entity->skill[1] = 1;
									}
									else
									{
										entity->fskill[0] -= speed;
										entity->fskill[0] = std::max(entity->fskill[0], setpoint);
									}
								}
								else
								{
									real_t speed = limbs[monsterType][3][1];
									entity->fskill[0] += speed;
									entity->fskill[0] = std::min(entity->fskill[0], basePitchSetpoint);
								}
							}
						}
					}
					else
					{
						if ( !adorcisedWeapon )
						{
							real_t speed = limbs[monsterType][13][0];
							real_t setpoint = (limbs[monsterType][14][1] * PI / 180.0);
							entity->fskill[0] -= speed;
							if ( setpoint >= 0 )
							{
								entity->fskill[0] = std::min(entity->fskill[0], setpoint);
							}
							else
							{
								entity->fskill[0] = std::max(entity->fskill[0], setpoint);
							}
						}
						else
						{
							if ( poke )
							{
								static ConsoleVariable<float> cvar_revenant_pokeset1("/revenant_pokeset1", 110.0);
								static ConsoleVariable<float> cvar_revenant_pokespd1("/revenant_pokespd1", -0.25);
								real_t speed = *cvar_revenant_pokespd1;
								real_t setpoint = (*cvar_revenant_pokeset1 * PI / 180.0);
								if ( limbAngleWithinRange(entity->fskill[0], -speed, setpoint) )
								{
									entity->fskill[0] = setpoint;
								}
								else
								{
									entity->fskill[0] -= speed;
								}
							}
							else
							{
								real_t speed = limbs[monsterType][3][0];
								real_t setpoint = (limbs[monsterType][4][1] * PI / 180.0);
								entity->fskill[0] -= speed;
								if ( setpoint >= 0 )
								{
									entity->fskill[0] = std::min(entity->fskill[0], setpoint);
								}
								else
								{
									entity->fskill[0] = std::max(entity->fskill[0], setpoint);
								}
							}
						}
					}

					if ( MONSTER_ATTACKTIME >= (int)limbs[monsterType][18][0] + delay )
					{
						if ( MONSTER_ATTACK == MONSTER_POSE_MAGIC_CAST1 || MONSTER_ATTACK == MONSTER_POSE_MAGIC_WINDUP1 )
						{
							SKULL_FLOAT_ATK += limbs[monsterType][12][1];
							SKULL_FLOAT_ATK = std::min(SKULL_FLOAT_ATK, (real_t)limbs[monsterType][12][2]);
						}
						else
						{
							SKULL_FLOAT_ATK -= limbs[monsterType][18][1];
							SKULL_FLOAT_ATK = std::max(SKULL_FLOAT_ATK, (real_t)limbs[monsterType][18][2]);
						}
					}
					else if ( MONSTER_ATTACKTIME >= (int)limbs[monsterType][17][0] + delay )
					{
						if ( MONSTER_ATTACK == MONSTER_POSE_MAGIC_CAST1 || MONSTER_ATTACK == MONSTER_POSE_MAGIC_WINDUP1 )
						{
							SKULL_FLOAT_ATK += limbs[monsterType][19][1];
							SKULL_FLOAT_ATK = std::min(SKULL_FLOAT_ATK, (real_t)limbs[monsterType][19][2]);
						}
						else
						{
							SKULL_FLOAT_ATK += limbs[monsterType][17][1];
							SKULL_FLOAT_ATK = std::min(SKULL_FLOAT_ATK, (real_t)limbs[monsterType][17][2]);
						}
					}
					else if ( MONSTER_ATTACKTIME >= (int)limbs[monsterType][16][0] + delay )
					{
						SKULL_FLOAT_ATK -= limbs[monsterType][16][1];
						SKULL_FLOAT_ATK = std::max(SKULL_FLOAT_ATK, (real_t)limbs[monsterType][16][2]);
					}
				}

				if ( MONSTER_ATTACKTIME >= (int)limbs[monsterType][15][1] + delay )
				{
					MONSTER_ATTACK = 0;
				}
			}
		}

		switch ( bodypart )
		{
			case REVENANT_SKULL_BODY:
			{
				entity->x += limbs[monsterType][6][0] * cos(entity->yaw);
				entity->y += limbs[monsterType][6][1] * sin(entity->yaw);
				entity->z += limbs[monsterType][6][2];
				entity->focalx = limbs[monsterType][1][0];
				entity->focaly = limbs[monsterType][1][1];
				entity->focalz = limbs[monsterType][1][2];

				entity->pitch = entity->fskill[0];

				if ( enableDebugKeys && (svFlags & SV_FLAG_CHEATS) )
				{
					if ( keystatus[SDLK_KP_PLUS] )
					{
						keystatus[SDLK_KP_PLUS] = 0;
						entity->skill[0] = entity->skill[0] == 0 ? 1 : 0;
					}
					if ( keystatus[SDLK_KP_6] )
					{
						keystatus[SDLK_KP_6] = 0;
						SKULL_CIRCLES = 2;
						SKULL_CIRCLE_ANIM = 0.0;
						SKULL_CIRCLE_SCALE = 0.0;
						SKULL_CIRCLES_DECREMENT_MODE = 0;
					}
					if ( keystatus[SDLK_KP_4] )
					{
						keystatus[SDLK_KP_4] = 0;
						SKULL_CIRCLES = 0;
					}
					if ( keystatus[SDLK_KP_2] )
					{
						keystatus[SDLK_KP_2] = 0;
						SKULL_FLIP = 1.0;

					}
					if ( keystatus[SDLK_KP_3] )
					{
					}
					if ( keystatus[SDLK_KP_1] )
					{
					}
					if ( keystatus[SDLK_KP_0] )
					{
						keystatus[SDLK_KP_0] = 0;
						SKULL_BOBS = 5;
						SKULL_BOB_ANIM = 0.0;
					}
				}

				entity->pitch -= PI * (-1 + sin(PI / 2 + SKULL_FLIP * PI));
				SKULL_FLIP -= 0.05;
				SKULL_FLIP = std::max(0.0, SKULL_FLIP);

				if ( SKULL_CIRCLES > 0 )
				{
					real_t prev = SKULL_CIRCLE_ANIM;
					real_t mult = MONSTER_ATTACK == MONSTER_POSE_MAGIC_WINDUP1 ? 5.0 : 1.0;
					SKULL_CIRCLE_ANIM += limbs[monsterType][8][2] * mult;
					SKULL_CIRCLE_SCALE += limbs[monsterType][8][1] * mult;
					SKULL_CIRCLE_SCALE = std::min(1.0, SKULL_CIRCLE_SCALE);
					if ( prev < PI && SKULL_CIRCLE_ANIM >= PI )
					{
						SKULL_CIRCLES--;
					}
				}
				if ( SKULL_CIRCLES == 0 )
				{
					if ( SKULL_CIRCLES_DECREMENT_MODE == 0 )
					{
						SKULL_CIRCLE_SCALE -= limbs[monsterType][8][1];
						SKULL_CIRCLE_SCALE = std::max(0.0, SKULL_CIRCLE_SCALE);
						if ( SKULL_CIRCLE_ANIM >= PI )
						{
							SKULL_CIRCLE_ANIM += limbs[monsterType][8][2] * 4;
							SKULL_CIRCLE_ANIM = std::min(SKULL_CIRCLE_ANIM, 2 * PI);
						}
						else
						{
							SKULL_CIRCLE_ANIM -= limbs[monsterType][8][2] * 4;
							SKULL_CIRCLE_ANIM = std::max(SKULL_CIRCLE_ANIM, 0.0);
						}
					}
					else
					{
						SKULL_CIRCLE_SCALE -= limbs[monsterType][8][1] * 2;
						SKULL_CIRCLE_SCALE = std::max(0.0, SKULL_CIRCLE_SCALE);

						// spin fast
						if ( SKULL_CIRCLE_SCALE <= 0.0 )
						{
							SKULL_CIRCLE_ANIM = 0.0;
						}
					}
				}
				while ( SKULL_CIRCLE_ANIM >= 2 * PI )
				{
					SKULL_CIRCLE_ANIM -= 2 * PI;
				}
				while ( SKULL_CIRCLE_ANIM < 0.0 )
				{
					SKULL_CIRCLE_ANIM += 2 * PI;
				}
				if ( entity->skill[0] == 0 )
				{
					entity->fskill[1] += 0.1;
				}

				SKULL_FLOAT_X = limbs[monsterType][10][0] * sin(entity->fskill[1] * limbs[monsterType][11][0]) * cos(entity->yaw + PI / 2);
				SKULL_FLOAT_Y = limbs[monsterType][10][1] * sin(entity->fskill[1] * limbs[monsterType][11][1]) * sin(entity->yaw + PI / 2);
				SKULL_FLOAT_Z = limbs[monsterType][10][2] * sin(entity->fskill[1] * limbs[monsterType][11][2]);
				real_t floatAtkZ = SKULL_FLOAT_ATK < 0 ? 2 * sin(SKULL_FLOAT_ATK * PI / 8) : 0.5 * sin(SKULL_FLOAT_ATK * PI / 8);
				SKULL_FLOAT_Z += floatAtkZ;

				SKULL_FLOAT_X += SKULL_FLOAT_ATK * cos(entity->yaw);
				SKULL_FLOAT_Y += SKULL_FLOAT_ATK * sin(entity->yaw);

				{
					SKULL_FLOAT_X += SKULL_CIRCLE_SCALE * limbs[monsterType][8][0] * cos(entity->yaw + SKULL_CIRCLE_ANIM);
					SKULL_FLOAT_Y += SKULL_CIRCLE_SCALE * limbs[monsterType][8][0] * sin(entity->yaw + SKULL_CIRCLE_ANIM);

					if ( SKULL_CIRCLES_DECREMENT_MODE == 1 )
					{
						// spin fast
						entity->yaw += SKULL_CIRCLE_SCALE * (PI / 2 + SKULL_CIRCLE_ANIM);
					}
					else
					{
						// ease to normal
						entity->yaw += SKULL_CIRCLE_SCALE * PI / 2 + SKULL_CIRCLE_ANIM;
					}

					if ( SKULL_BOBS > 0 )
					{
						SKULL_BOB_ANIM += limbs[monsterType][9][0];

						real_t scale = SKULL_BOBS * 0.5;

						SKULL_FLOAT_Z += scale * sin(PI / 4) - scale * sin(SKULL_BOB_ANIM * 2 * PI + PI / 4);

						if ( SKULL_BOB_ANIM >= 1.0 )
						{
							SKULL_BOB_ANIM = 0.0;
							if ( SKULL_NEXTBOB > 0 )
							{
								SKULL_BOBS = SKULL_NEXTBOB;
								SKULL_NEXTBOB = 0;
							}
							else
							{
								--SKULL_BOBS;
							}
						}
					}
				}

				entity->x += SKULL_FLOAT_X;
				entity->y += SKULL_FLOAT_Y;
				entity->z += SKULL_FLOAT_Z;

				if ( adorcisedWeapon )
				{
					entity->roll = (PI / 32) * sin(entity->fskill[1] * limbs[monsterType][11][0]);
					entity->yaw += (PI / 32) * sin(entity->fskill[1] * limbs[monsterType][11][0]);

					if ( multiplayer != CLIENT )
					{
						if ( myStats->weapon )
						{
							entity->sprite = itemModel(myStats->weapon);
						}
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

				Entity* fx = spawnMagicParticleCustom(entity, 96, 0.5, 1.0);
				fx->vel_x = 0.25 * cos(entity->yaw + PI);
				fx->vel_y = 0.25 * sin(entity->yaw + PI);
				fx->vel_z = 0.3;
				fx->flags[SPRITE] = true;
				fx->ditheringDisabled = true;
				break;
			}
			default:
				break;
		}
	}

	if ( MONSTER_ATTACK > 0 )
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