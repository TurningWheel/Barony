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
#include "sound.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "magic/magic.hpp"

void initSkeleton(Entity* my, Stat* myStats)
{
	int c;
	node_t* node;

	//Sprite 229 = Skeleton head model
	my->initMonster(229);

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = -1;
		MONSTER_SPOTVAR = 1;
		MONSTER_IDLESND = -1;
		MONSTER_IDLEVAR = 1;
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

			if ( my->monsterAllySummonRank != 0 )
			{
				int rank = std::min(my->monsterAllySummonRank, 7);
				bool secondarySummon = true;
				if ( !strcmp(myStats->name, "skeleton knight") )
				{
					secondarySummon = false;
				}
				my->skeletonSummonSetEquipment(myStats, rank);
				myStats->sex = MALE;
				myStats->GOLD = 0;
				my->light = lightSphereShadow(my->x / 16, my->y / 16, 3, 64);

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
				setRandomMonsterStats(myStats);

				// generate 6 items max, less if there are any forced items from boss variants
				int customItemsToGenerate = ITEM_CUSTOM_SLOT_LIMIT;

				// boss variants
				if ( rand() % 50 > 0 || my->flags[USERFLAG2] || strcmp(myStats->name, ""))
				{
					// not boss if a follower, or name has already been set to something other than blank.
					if ( strncmp(map.name, "Underworld", 10) )
					{
						//give weapon
						if ( myStats->weapon == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1 )
						{
							switch ( rand() % 10 )
							{
								case 0:
								case 1:
									myStats->weapon = newItem(BRONZE_AXE, WORN, -1 + rand() % 2, 1, rand(), false, nullptr);
									break;
								case 2:
								case 3:
									myStats->weapon = newItem(BRONZE_SWORD, WORN, -1 + rand() % 2, 1, rand(), false, nullptr);
									break;
								case 4:
								case 5:
									myStats->weapon = newItem(IRON_SPEAR, WORN, -1 + rand() % 2, 1, rand(), false, nullptr);
									break;
								case 6:
								case 7:
									myStats->weapon = newItem(IRON_AXE, WORN, -1 + rand() % 2, 1, rand(), false, nullptr);
									break;
								case 8:
								case 9:
									myStats->weapon = newItem(IRON_SWORD, WORN, -1 + rand() % 2, 1, rand(), false, nullptr);
									break;
							}
						}
					}
				}
				else
				{
					myStats->HP = 100;
					myStats->MAXHP = 100;
					strcpy(myStats->name, "Funny Bones");
					myStats->weapon = newItem(ARTIFACT_AXE, EXCELLENT, 1, 1, rand(), true, nullptr);
					myStats->cloak = newItem(CLOAK_PROTECTION, WORN, 0, 1, 2, true, nullptr);
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
					switch ( rand() % 10 )
					{
						case 0:
						case 1:
						case 2:
						case 3:
							myStats->weapon = newItem(SHORTBOW, WORN, -1 + rand() % 2, 1, rand(), false, nullptr);
							break;
						case 4:
						case 5:
						case 6:
						case 7:
							myStats->weapon = newItem(CROSSBOW, WORN, -1 + rand() % 2, 1, rand(), false, nullptr);
							break;
						case 8:
						case 9:
							myStats->weapon = newItem(MAGICSTAFF_COLD, EXCELLENT, -1 + rand() % 2, 1, rand(), false, nullptr);
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
							myStats->helmet = newItem(LEATHER_HELM, DECREPIT, -1 + rand() % 2, 1, rand(), false, nullptr);
							break;
						case 6:
						case 7:
						case 8:
						case 9:
							myStats->helmet = newItem(IRON_HELM, DECREPIT, -1 + rand() % 2, 1, rand(), false, nullptr);
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
							myStats->shield = newItem(WOODEN_SHIELD, DECREPIT, -1 + rand() % 2, 1, rand(), false, nullptr);
							break;
						case 8:
							myStats->shield = newItem(BRONZE_SHIELD, DECREPIT, -1 + rand() % 2, 1, rand(), false, nullptr);
							break;
						case 9:
							myStats->shield = newItem(IRON_SHIELD, DECREPIT, -1 + rand() % 2, 1, rand(), false, nullptr);
							break;
					}
				}
			}
		}
	}

	// torso
	Entity* entity = newEntity(230, 0, map.entities, nullptr); //Limb entity.
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
	entity = newEntity(236, 0, map.entities, nullptr); //Limb entity.
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
	entity = newEntity(235, 0, map.entities, nullptr); //Limb entity.
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
	entity = newEntity(233, 0, map.entities, nullptr); //Limb entity.
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
	entity = newEntity(231, 0, map.entities, nullptr); //Limb entity.
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
	entity = newEntity(-1, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[INVISIBLE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
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
	entity = newEntity(-1, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[INVISIBLE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
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
	entity = newEntity(-1, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[INVISIBLE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
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

void actSkeletonLimb(Entity* my)
{
	my->actMonsterLimb(true);
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
				if ( (leader->getINT() + leaderStats->PROFICIENCIES[PRO_MAGIC]) >= SKILL_LEVEL_EXPERT )
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
					manaToRefund -= rand() % (std::max(1, manaToRefund / 3));
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
							messagePlayerMonsterEvent(leader->skill[2], 0xFFFFFFFF, *myStats, language[3194], language[3195], MSG_COMBAT);
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
		if ( myStats->EFFECTS[EFF_ASLEEP] )
		{
			my->z = 2;
			my->pitch = PI / 4;
		}
		else
		{
			my->z = -.5;
			my->pitch = 0;
		}
	}

	Entity* shieldarm = nullptr;

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
						entity->sprite = 230;
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
				my->setHumanoidLimbOffset(entity, SKELETON, LIMB_HUMANOID_TORSO);
				break;
			// right leg
			case LIMB_HUMANOID_RIGHTLEG:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->shoes == nullptr )
					{
						entity->sprite = 236;
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
				my->setHumanoidLimbOffset(entity, SKELETON, LIMB_HUMANOID_RIGHTLEG);
				break;
			// left leg
			case LIMB_HUMANOID_LEFTLEG:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->shoes == nullptr )
					{
						entity->sprite = 235;
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
				my->setHumanoidLimbOffset(entity, SKELETON, LIMB_HUMANOID_LEFTLEG);
				break;
			// right arm
			case LIMB_HUMANOID_RIGHTARM:
			{
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
						entity->sprite = 233;
					}
					else
					{
						// else flex arm.
						entity->focalx = limbs[SKELETON][4][0] + 1; // 1
						entity->focaly = limbs[SKELETON][4][1]; // 0
						entity->focalz = limbs[SKELETON][4][2] - 1; // 1
						entity->sprite = 234;
					}
				}
				my->setHumanoidLimbOffset(entity, SKELETON, LIMB_HUMANOID_RIGHTARM);
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
					entity->sprite += (shield->flags[INVISIBLE] != true);
					if ( shield->flags[INVISIBLE] )
					{
						// if shield invisible, relax arm.
						entity->sprite = 231;
						entity->focalx = limbs[SKELETON][5][0]; // 0
						entity->focaly = limbs[SKELETON][5][1]; // 0
						entity->focalz = limbs[SKELETON][5][2]; // 2
					}
					else
					{
						// else flex arm.
						entity->sprite = 232;
						entity->focalx = limbs[SKELETON][5][0] + 1; // 1
						entity->focaly = limbs[SKELETON][5][1]; // 0
						entity->focalz = limbs[SKELETON][5][2] - 1; // 1
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
				entity->focalx = limbs[SKELETON][9][0]; // 0
				entity->focaly = limbs[SKELETON][9][1]; // 0
				entity->focalz = limbs[SKELETON][9][2]; // -2
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
				entity->focalx = limbs[SKELETON][10][0]; // 0
				entity->focaly = limbs[SKELETON][10][1]; // 0
				entity->focalz = limbs[SKELETON][10][2]; // .5
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
					entity->focalx = limbs[SKELETON][10][0] + .35; // .35
					entity->focaly = limbs[SKELETON][10][1] - 2; // -2
					entity->focalz = limbs[SKELETON][10][2]; // .5
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
	if ( !strcmp(myStats->name, "skeleton knight") )
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
	else if ( !strcmp(myStats->name, "skeleton sentinel") )
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
	}
}