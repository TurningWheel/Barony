/*-------------------------------------------------------------------------------

	BARONY
	File: monster_human.cpp
	Desc: implements all of the human monster's code

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

void initHuman(Entity* my, Stat* myStats)
{
	int c;
	node_t* node;

	my->initMonster(113);

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

			my->createPathBoundariesNPC();

			// apply random stat increases if set in stat_shared.cpp or editor
			setRandomMonsterStats(myStats);

			// generate 6 items max, less if there are any forced items from boss variants
			int customItemsToGenerate = ITEM_CUSTOM_SLOT_LIMIT;

			// special human variant (named or Zap Brigadier), do not generate any other items
			int specialMonsterVariant = 0;

			// boss variants
			// generate special loadout
			if ( my->monsterSpecialTimer == 0 )
			{
				if ( rand() % 25 == 0 )
				{
					specialMonsterVariant = 1;
					int specialMonsterType = rand() % 10;
					if ( !strncmp(map.name, "Hamlet", 6) )
					{
						while ( specialMonsterType == 6 ) // 2 spiders that spawn cause aggro issues in Hamlet.
						{
							specialMonsterType = rand() % 10;
						}
					}
					switch ( rand() % 10 )
					{
						case 0:
							// red riding hood
							strcpy(myStats->name, "Red Riding Hood");
							myStats->appearance = 2;
							myStats->sex = FEMALE;
							myStats->LVL = 1;
							myStats->HP = 10;
							myStats->MAXHP = myStats->HP;
							myStats->MP = 10;
							myStats->MAXMP = myStats->MP;
							myStats->STR = 0;
							myStats->DEX = 0;
							myStats->CON = 0;
							myStats->INT = -2;
							myStats->PER = -2;
							myStats->CHR = 4;
							myStats->helmet = newItem(HAT_PHRYGIAN, EXCELLENT, 1, 1, rand(), false, nullptr);
							myStats->cloak = newItem(CLOAK, EXCELLENT, 1, 1, 2, false, nullptr);
							myStats->weapon = newItem(QUARTERSTAFF, EXCELLENT, 1, 1, rand(), false, nullptr);
							break;
						case 1:
							// king arthur
							strcpy(myStats->name, "King Arthur");
							myStats->appearance = 0;
							myStats->sex = MALE;
							myStats->LVL = 10;
							myStats->HP = 100;
							myStats->MAXHP = myStats->HP;
							myStats->MP = 100;
							myStats->MAXMP = myStats->MP;
							myStats->STR = 5;
							myStats->DEX = 5;
							myStats->CON = 5;
							myStats->INT = 5;
							myStats->PER = 5;
							myStats->CHR = 5;
							myStats->breastplate = newItem(STEEL_BREASTPIECE, EXCELLENT, 1, 1, 1, true, nullptr);
							myStats->gloves = newItem(GAUNTLETS, EXCELLENT, 1, 1, 1, true, nullptr);
							myStats->shoes = newItem(STEEL_BOOTS, EXCELLENT, 1, 1, 1, true, nullptr);
							myStats->cloak = newItem(CLOAK, EXCELLENT, 2, 1, 2, true, nullptr);
							myStats->weapon = newItem(ARTIFACT_SWORD, EXCELLENT, 1, 1, rand(), true, nullptr);
							myStats->shield = newItem(STEEL_SHIELD_RESISTANCE, EXCELLENT, 1, 1, 1, true, nullptr);
							break;
						case 2:
							// merlin
							strcpy(myStats->name, "Merlin");
							myStats->appearance = 5;
							myStats->sex = MALE;
							myStats->LVL = 10;
							myStats->HP = 60;
							myStats->MAXHP = myStats->HP;
							myStats->MP = 200;
							myStats->MAXMP = myStats->MP;
							myStats->STR = 2;
							myStats->DEX = 2;
							myStats->CON = 3;
							myStats->INT = 11;
							myStats->PER = 10;
							myStats->CHR = 2;
							myStats->helmet = newItem(HAT_WIZARD, EXCELLENT, 2, 1, 2, false, nullptr);
							myStats->shoes = newItem(LEATHER_BOOTS_SPEED, EXCELLENT, 2, 1, 2, false, nullptr);
							myStats->cloak = newItem(CLOAK_PROTECTION, EXCELLENT, 5, 1, 3, false, nullptr);
							myStats->weapon = newItem(MAGICSTAFF_LIGHTNING, EXCELLENT, 2, 1, 2, false, nullptr);
							myStats->amulet = newItem(AMULET_MAGICREFLECTION, EXCELLENT, 2, 1, 2, false, nullptr);
							break;
						case 3:
							// robin hood
							strcpy(myStats->name, "Robin Hood");
							myStats->appearance = 1;
							myStats->sex = MALE;
							myStats->LVL = 5;
							myStats->HP = 70;
							myStats->MAXHP = myStats->HP;
							myStats->MP = 50;
							myStats->MAXMP = myStats->MP;
							myStats->STR = 3;
							myStats->DEX = 5;
							myStats->CON = 3;
							myStats->INT = 2;
							myStats->PER = 3;
							myStats->CHR = 5;
							myStats->gloves = newItem(GLOVES, EXCELLENT, 1, 1, 3, true, nullptr);
							myStats->shoes = newItem(LEATHER_BOOTS, SERVICABLE, 1, 1, 3, true, nullptr);
							myStats->cloak = newItem(CLOAK, EXCELLENT, 1, 1, 0, true, nullptr);
							myStats->weapon = newItem(SHORTBOW, EXCELLENT, 1, 1, 3, true, nullptr);
							break;
						case 4:
							// conan
							strcpy(myStats->name, "Conan the Barbarian");
							myStats->appearance = 7;
							myStats->sex = MALE;
							myStats->LVL = 10;
							myStats->HP = 100;
							myStats->MAXHP = myStats->HP;
							myStats->MP = 20;
							myStats->MAXMP = myStats->MP;
							myStats->STR = 10;
							myStats->DEX = 5;
							myStats->CON = 10;
							myStats->INT = 3;
							myStats->PER = 3;
							myStats->CHR = 20;
							myStats->helmet = newItem(LEATHER_HELM, EXCELLENT, 2, 1, rand(), false, nullptr);
							myStats->shield = newItem(WOODEN_SHIELD, EXCELLENT, 2, 1, rand(), false, nullptr);
							myStats->weapon = newItem(STEEL_AXE, EXCELLENT, 2, 1, rand(), false, nullptr);
							break;
						case 5:
							// othello
							strcpy(myStats->name, "Othello");
							myStats->appearance = 14;
							myStats->sex = MALE;
							myStats->LVL = 10;
							myStats->HP = 50;
							myStats->MAXHP = myStats->HP;
							myStats->MP = 20;
							myStats->MAXMP = myStats->MP;
							myStats->STR = 3;
							myStats->DEX = 3;
							myStats->CON = 3;
							myStats->INT = 3;
							myStats->PER = 0;
							myStats->CHR = 30;
							myStats->gloves = newItem(BRACERS, EXCELLENT, -1, 1, rand(), false, nullptr);
							myStats->breastplate = newItem(IRON_BREASTPIECE, EXCELLENT, 1, 1, rand(), false, nullptr);
							myStats->weapon = newItem(STEEL_SWORD, EXCELLENT, 2, 1, rand(), false, nullptr);
							myStats->cloak = newItem(CLOAK, EXCELLENT, 0, 1, 2, false, nullptr);
							break;
						case 6:
							// anansi
							strcpy(myStats->name, "Anansi");
							myStats->appearance = 15;
							myStats->sex = MALE;
							myStats->LVL = 20;
							myStats->HP = 100;
							myStats->MAXHP = myStats->HP;
							myStats->MP = 100;
							myStats->MAXMP = myStats->MP;
							myStats->STR = 5;
							myStats->DEX = 8;
							myStats->CON = 5;
							myStats->INT = 20;
							myStats->PER = 20;
							myStats->CHR = 10;
							myStats->helmet = newItem(HAT_JESTER, EXCELLENT, 5, 1, rand(), false, nullptr);
							myStats->weapon = newItem(ARTIFACT_MACE, EXCELLENT, 1, 1, rand(), false, nullptr);
							int c;
							for ( c = 0; c < 2; c++ )
							{
								Entity* entity = summonMonster(SPIDER, my->x, my->y);
								if ( entity )
								{
									entity->parent = my->getUID();
									entity->flags[USERFLAG2] = true;
								}
							}
							break;
						case 7:
							// oya
							strcpy(myStats->name, "Oya");
							myStats->appearance = 13;
							myStats->sex = FEMALE;
							myStats->LVL = 20;
							myStats->HP = 100;
							myStats->MAXHP = myStats->HP;
							myStats->MP = 100;
							myStats->MAXMP = myStats->MP;
							myStats->STR = 4;
							myStats->DEX = 10;
							myStats->CON = 2;
							myStats->INT = 20;
							myStats->PER = 10;
							myStats->CHR = 10;
							myStats->cloak = newItem(CLOAK_PROTECTION, EXCELLENT, 3, 1, 1, false, nullptr);
							myStats->helmet = newItem(HAT_HOOD, EXCELLENT, 3, 1, 1, false, nullptr);
							break;
						case 8:
							// vishpala
							strcpy(myStats->name, "Vishpala");
							myStats->appearance = 17;
							myStats->sex = FEMALE;
							myStats->LVL = 10;
							myStats->HP = 70;
							myStats->MAXHP = myStats->HP;
							myStats->MP = 20;
							myStats->MAXMP = myStats->MP;
							myStats->STR = 5;
							myStats->DEX = 5;
							myStats->CON = 5;
							myStats->INT = 5;
							myStats->PER = 5;
							myStats->CHR = 10;
							myStats->cloak = newItem(CLOAK, EXCELLENT, 0, 1, 2, false, nullptr);
							myStats->breastplate = newItem(IRON_BREASTPIECE, EXCELLENT, 0, 1, rand(), false, nullptr);
							myStats->shoes = newItem(IRON_BOOTS, EXCELLENT, 0, 1, rand(), false, nullptr);
							myStats->weapon = newItem(ARTIFACT_SPEAR, EXCELLENT, 1, 1, rand(), false, nullptr);
							myStats->shield = newItem(BRONZE_SHIELD, EXCELLENT, 1, 1, rand(), false, nullptr);
							break;
						case 9:
							// kali
							strcpy(myStats->name, "Kali");
							myStats->appearance = 15;
							myStats->sex = FEMALE;
							myStats->LVL = 20;
							myStats->HP = 200;
							myStats->MAXHP = myStats->HP;
							myStats->MP = 200;
							myStats->MAXMP = myStats->MP;
							myStats->STR = 5;
							myStats->DEX = 5;
							myStats->CON = 5;
							myStats->INT = 20;
							myStats->PER = 20;
							myStats->CHR = 20;
							myStats->cloak = newItem(CLOAK_MAGICREFLECTION, EXCELLENT, 1, 1, 2, false, nullptr);
							myStats->shoes = newItem(LEATHER_BOOTS_SPEED, EXCELLENT, 1, 1, rand(), false, nullptr);
							myStats->weapon = newItem(SPELLBOOK_FIREBALL, EXCELLENT, 1, 1, rand(), false, nullptr);
							break;
						default:
							break;
					}
				}
			}
			else
			{
				specialMonsterVariant = 1;
				// zap brigadier
				strcpy(myStats->name, "ZAP Brigadier");
				myStats->appearance = 1;
				myStats->sex = static_cast<sex_t>(rand() % 2);
				myStats->LVL = 10;
				myStats->HP = 100;
				myStats->MAXHP = myStats->HP;
				myStats->MP = 200;
				myStats->MAXMP = myStats->MP;
				myStats->STR = 3;
				myStats->DEX = 3;
				myStats->CON = 3;
				myStats->INT = 3;
				myStats->PER = 10;
				myStats->CHR = 10;
				myStats->helmet = newItem(HAT_HOOD, EXCELLENT, 2, 1, 3, false, nullptr);
				myStats->gloves = newItem(GLOVES, EXCELLENT, 0, 1, 2, false, nullptr);
				myStats->shoes = newItem(LEATHER_BOOTS_SPEED, EXCELLENT, 0, 1, 2, false, nullptr);
				myStats->breastplate = newItem(LEATHER_BREASTPIECE, EXCELLENT, 0, 1, 2, false, nullptr);
				myStats->cloak = newItem(CLOAK_PROTECTION, EXCELLENT, 2, 1, 3, false, nullptr);
				myStats->weapon = newItem(MAGICSTAFF_LIGHTNING, EXCELLENT, 1, 1, 2, false, nullptr);
				myStats->amulet = newItem(AMULET_MAGICREFLECTION, EXCELLENT, 1, 1, 2, false, nullptr);
			}

			// random effects
			if ( rand() % 10 == 0 )
			{
				myStats->EFFECTS[EFF_ASLEEP] = true;
				myStats->EFFECTS_TIMERS[EFF_ASLEEP] = 1800 + rand() % 1800;
			}

			// generates equipment and weapons if available from editor
			createMonsterEquipment(myStats);

			// create any custom inventory items from editor if available
			createCustomInventory(myStats, customItemsToGenerate);

			// count if any custom inventory items from editor
			int customItems = countCustomItems(myStats);
			//max limit of 6 custom items per entity.

			// count any inventory items set to default in edtior
			int defaultItems = countDefaultItems(myStats);

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

			if ( specialMonsterVariant == 0 )
			{
				// generate random equipment if not a named special human

				//give shield
				if ( myStats->shield == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_SHIELD] == 1 )
				{
					switch ( rand() % 10 )
					{
						case 0:
						case 1:
						case 2:
							myStats->shield = newItem(TOOL_TORCH, SERVICABLE, 0, 1, rand(), false, nullptr);
							break;
						case 3:
						case 4:
							break;
						case 5:
						case 6:
							myStats->shield = newItem(WOODEN_SHIELD, WORN, 0, 1, rand(), false, nullptr);
							break;
						case 7:
						case 8:
							myStats->shield = newItem(BRONZE_SHIELD, WORN, 0, 1, rand(), false, nullptr);
							break;
						case 9:
							myStats->shield = newItem(IRON_SHIELD, WORN, 0, 1, rand(), false, nullptr);
							break;
					}
				}

				//give weapon
				if ( myStats->weapon == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1 )
				{
					switch ( rand() % 10 )
					{
						case 0:
						case 1:
							myStats->weapon = newItem(SHORTBOW, WORN, 0, 1, rand(), false, nullptr);
							break;
						case 2:
						case 3:
							myStats->weapon = newItem(BRONZE_AXE, WORN, 0, 1, rand(), false, nullptr);
							break;
						case 4:
						case 5:
							myStats->weapon = newItem(BRONZE_SWORD, WORN, 0, 1, rand(), false, nullptr);
							break;
						case 6:
							myStats->weapon = newItem(IRON_SPEAR, WORN, 0, 1, rand(), false, nullptr);
							break;
						case 7:
							myStats->weapon = newItem(IRON_AXE, WORN, 0, 1, rand(), false, nullptr);
							break;
						case 8:
							myStats->weapon = newItem(IRON_SWORD, WORN, 0, 1, rand(), false, nullptr);
							break;
						case 9:
							myStats->weapon = newItem(CROSSBOW, WORN, 0, 1, rand(), false, nullptr);
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
						case 2:
							break;
						case 3:
							myStats->helmet = newItem(HAT_HOOD, WORN, 0, 1, rand() % 4, false, nullptr);
							break;
						case 4:
							myStats->helmet = newItem(HAT_PHRYGIAN, WORN, 0, 1, rand(), false, nullptr);
							break;
						case 5:
							myStats->helmet = newItem(HAT_WIZARD, WORN, 0, 1, rand(), false, nullptr);
							break;
						case 6:
						case 7:
							myStats->helmet = newItem(LEATHER_HELM, WORN, 0, 1, rand(), false, nullptr);
							break;
						case 8:
						case 9:
							myStats->helmet = newItem(IRON_HELM, WORN, 0, 1, rand(), false, nullptr);
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
						case 5:
							break;
						case 6:
						case 7:
						case 8:
							myStats->cloak = newItem(CLOAK, WORN, 0, 1, rand(), false, nullptr);
							break;
						case 9:
							myStats->cloak = newItem(CLOAK_MAGICREFLECTION, WORN, 0, 1, rand(), false, nullptr);
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
						case 2:
						case 3:
						case 4:
							break;
						case 5:
						case 6:
						case 7:
							myStats->breastplate = newItem(LEATHER_BREASTPIECE, WORN, 0, 1, rand(), false, nullptr);
							break;
						case 8:
						case 9:
							myStats->breastplate = newItem(IRON_BREASTPIECE, WORN, 0, 1, rand(), false, nullptr);
							break;
					}
				}

				// give gloves
				if ( myStats->gloves == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_GLOVES] == 1 )
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
							myStats->gloves = newItem(GLOVES, WORN, 0, 1, rand(), false, nullptr);
							break;
						case 8:
						case 9:
							myStats->gloves = newItem(GAUNTLETS, WORN, 0, 1, rand(), false, nullptr);
							break;
					}
				}

				// give boots
				if ( myStats->shoes == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_BOOTS] == 1 )
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
							myStats->shoes = newItem(LEATHER_BOOTS, WORN, 0, 1, rand(), false, nullptr);
							break;
						case 8:
						case 9:
							myStats->shoes = newItem(IRON_BOOTS, WORN, 0, 1, rand(), false, nullptr);
							break;
					}
				}
			}
		}
	}

	// torso
	Entity* entity = newEntity(106, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[HUMAN][1][0]; // 0
	entity->focaly = limbs[HUMAN][1][1]; // 0
	entity->focalz = limbs[HUMAN][1][2]; // 0
	entity->behavior = &actHumanLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right leg
	entity = newEntity(107, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[HUMAN][2][0]; // 0
	entity->focaly = limbs[HUMAN][2][1]; // 0
	entity->focalz = limbs[HUMAN][2][2]; // 2
	entity->behavior = &actHumanLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left leg
	entity = newEntity(108, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[HUMAN][3][0]; // 0
	entity->focaly = limbs[HUMAN][3][1]; // 0
	entity->focalz = limbs[HUMAN][3][2]; // 2
	entity->behavior = &actHumanLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right arm
	entity = newEntity(109, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[HUMAN][4][0]; // 0
	entity->focaly = limbs[HUMAN][4][1]; // 0
	entity->focalz = limbs[HUMAN][4][2]; // 1.5
	entity->behavior = &actHumanLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left arm
	entity = newEntity(110, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[HUMAN][5][0]; // 0
	entity->focaly = limbs[HUMAN][5][1]; // 0
	entity->focalz = limbs[HUMAN][5][2]; // 1.5
	entity->behavior = &actHumanLimb;
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
	entity->focalx = limbs[HUMAN][6][0]; // 1.5
	entity->focaly = limbs[HUMAN][6][1]; // 0
	entity->focalz = limbs[HUMAN][6][2]; // -.5
	entity->behavior = &actHumanLimb;
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
	entity->focalx = limbs[HUMAN][7][0]; // 2
	entity->focaly = limbs[HUMAN][7][1]; // 0
	entity->focalz = limbs[HUMAN][7][2]; // 0
	entity->behavior = &actHumanLimb;
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
	entity->focalx = limbs[HUMAN][8][0]; // 0
	entity->focaly = limbs[HUMAN][8][1]; // 0
	entity->focalz = limbs[HUMAN][8][2]; // 4
	entity->behavior = &actHumanLimb;
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
	entity->focalx = limbs[HUMAN][9][0]; // 0
	entity->focaly = limbs[HUMAN][9][1]; // 0
	entity->focalz = limbs[HUMAN][9][2]; // -1.75
	entity->behavior = &actHumanLimb;
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
	entity->focalx = limbs[HUMAN][10][0]; // 0
	entity->focaly = limbs[HUMAN][10][1]; // 0
	entity->focalz = limbs[HUMAN][10][2]; // .5
	entity->behavior = &actHumanLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	if ( multiplayer == CLIENT )
	{
		my->sprite = 113; // human head model
		return;
	}

	// set head model
	if ( myStats->appearance < 5 )
	{
		my->sprite = 113 + 12 * myStats->sex + myStats->appearance;
	}
	else if ( myStats->appearance == 5 )
	{
		my->sprite = 332 + myStats->sex;
	}
	else if ( myStats->appearance >= 6 && myStats->appearance < 12 )
	{
		my->sprite = 341 + myStats->sex * 13 + myStats->appearance - 6;
	}
	else if ( myStats->appearance >= 12 )
	{
		my->sprite = 367 + myStats->sex * 13 + myStats->appearance - 12;
	}
	else
	{
		my->sprite = 113; // default
	}
}

void actHumanLimb(Entity* my)
{
	my->actMonsterLimb(true);
}

void humanDie(Entity* my)
{
	int c;
	for ( c = 0; c < 5; c++ )
	{
		Entity* gib = spawnGib(my);
		serverSpawnGibForClient(gib);
	}

	my->spawnBlood();

	playSoundEntity(my, 28, 128);

	my->removeMonsterDeathNodes();

	list_RemoveNode(my->mynode);
	return;
}

#define HUMANWALKSPEED .12

void humanMoveBodyparts(Entity* my, Stat* myStats, double dist)
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
	for (bodypart = 0, node = my->children.first; node != nullptr; node = node->next, bodypart++)
	{
		if ( bodypart < 2 )
		{
			if ( multiplayer == CLIENT )
			{
				for ( int i = LIMB_HUMANOID_TORSO; i <= LIMB_HUMANOID_LEFTARM; i++ )
				{
					my->humanSetLimbsClient(i);
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
			my->humanoidAnimateWalk(entity, node, bodypart, HUMANWALKSPEED, dist, 0.4);
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
			else if ( bodypart == LIMB_HUMANOID_CLOAK )
			{
				entity->pitch = entity->fskill[0];
			}

			my->humanoidAnimateWalk(entity, node, bodypart, HUMANWALKSPEED, dist, 0.4);

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
						switch ( myStats->appearance / 6 )
						{
							case 1:
								entity->sprite = 334 + 13 * myStats->sex;
								break;
							case 2:
								entity->sprite = 360 + 13 * myStats->sex;
								break;
							default:
								entity->sprite = 106 + 12 * myStats->sex;
								break;
						}
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
						switch ( myStats->appearance / 6 )
						{
							case 1:
								entity->sprite = 335 + 13 * myStats->sex;
								break;
							case 2:
								entity->sprite = 361 + 13 * myStats->sex;
								break;
							default:
								entity->sprite = 107 + 12 * myStats->sex;
								break;
						}
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
						switch ( myStats->appearance / 6 )
						{
							case 1:
								entity->sprite = 336 + 13 * myStats->sex;
								break;
							case 2:
								entity->sprite = 362 + 13 * myStats->sex;
								break;
							default:
								entity->sprite = 108 + 12 * myStats->sex;
								break;
						}
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
						switch ( myStats->appearance / 6 )
						{
							case 1:
								entity->sprite = 337 + 13 * myStats->sex;
								break;
							case 2:
								entity->sprite = 363 + 13 * myStats->sex;
								break;
							default:
								entity->sprite = 109 + 12 * myStats->sex;
								break;
						}
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
						if ( entity->sprite >= 109 && entity->sprite <= 110 )
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
						my->humanSetLimbsClient(bodypart);
					}
					else
					{
						entity->sprite = entity->skill[7];
					}
				}

				node_t* tempNode = list_Node(&my->children, LIMB_HUMANOID_WEAPON);
				if ( tempNode )
				{
					Entity* weapon = (Entity*)tempNode->element;
					if ( MONSTER_ARMBENDED || (weapon->flags[INVISIBLE] && my->monsterState != MONSTER_STATE_ATTACK) )
					{
						// if weapon invisible and I'm not attacking, relax arm.
						entity->focalx = limbs[HUMAN][4][0]; // 0
						entity->focaly = limbs[HUMAN][4][1]; // 0
						entity->focalz = limbs[HUMAN][4][2]; // 1.5
					}
					else
					{
						// else flex arm.
						entity->focalx = limbs[HUMAN][4][0] + 0.75;
						entity->focaly = limbs[HUMAN][4][1];
						entity->focalz = limbs[HUMAN][4][2] - 0.75;
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
						switch ( myStats->appearance / 6 )
						{
							case 1:
								entity->sprite = 338 + 13 * myStats->sex;
								break;
							case 2:
								entity->sprite = 364 + 13 * myStats->sex;
								break;
							default:
								entity->sprite = 110 + 12 * myStats->sex;
								break;
						}
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
						if ( entity->sprite >= 109 && entity->sprite <= 110 )
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
						my->humanSetLimbsClient(bodypart);
					}
					else
					{
						entity->sprite = entity->skill[7];
					}
				}

				node_t* tempNode = list_Node(&my->children, LIMB_HUMANOID_SHIELD);
				if ( tempNode )
				{
					Entity* shield = (Entity*)tempNode->element;
					if ( shield->flags[INVISIBLE] && (my->monsterState != MONSTER_STATE_ATTACK) )
					{
						// if shield invisible and I'm not attacking, relax arm.
						entity->focalx = limbs[HUMAN][5][0]; // 0
						entity->focaly = limbs[HUMAN][5][1]; // 0
						entity->focalz = limbs[HUMAN][5][2]; // 1.5
					}
					else
					{
						// else flex arm.
						entity->focalx = limbs[HUMAN][5][0] + 0.75;
						entity->focaly = limbs[HUMAN][5][1];
						entity->focalz = limbs[HUMAN][5][2] - 0.75;
						entity->sprite += 2;
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
				entity->focalx = limbs[HUMAN][9][0]; // 0
				entity->focaly = limbs[HUMAN][9][1]; // 0
				entity->focalz = limbs[HUMAN][9][2]; // -1.75
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
				entity->focalx = limbs[HUMAN][10][0]; // 0
				entity->focaly = limbs[HUMAN][10][1]; // 0
				entity->focalz = limbs[HUMAN][10][2]; // .5
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
					entity->focalx = limbs[HUMAN][10][0] + .35; // .35
					entity->focaly = limbs[HUMAN][10][1] - 2; // -2
					entity->focalz = limbs[HUMAN][10][2]; // .5
				}
				else
				{
					entity->focalx = limbs[HUMAN][10][0] + .25; // .25
					entity->focaly = limbs[HUMAN][10][1] - 2.25; // -2.25
					entity->focalz = limbs[HUMAN][10][2]; // .5
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

bool Entity::humanCanWieldItem(const Item& item) const
{
	Stat* myStats = getStats();
	if ( !myStats )
	{
		return false;
	}

	if ( monsterAllyIndex >= 0 )
	{
		// player ally.
		if ( item.interactNPCUid == getUID() )
		{
			// monster was set to interact with this item, force want it.
			switch ( itemCategory(&item) )
			{
				case WEAPON:
				case ARMOR:
				case RING:
				case AMULET:
				case MAGICSTAFF:
				case CLOAK:
					return true;
				default:
					return false;
					break;
			}
		}
		if ( monsterAllyClass == ALLY_CLASS_MIXED )
		{
			// pick up all default items.
		}
		else if ( monsterAllyClass == ALLY_CLASS_RANGED )
		{
			switch ( itemCategory(&item) )
			{
				case WEAPON:
					return isRangedWeapon(item);
				case ARMOR:
					switch ( item.type )
					{
						case CRYSTAL_BREASTPIECE:
						case CRYSTAL_HELM:
						case CRYSTAL_SHIELD:
						case STEEL_BREASTPIECE:
						case STEEL_HELM:
						case STEEL_SHIELD:
						case IRON_BREASTPIECE:
						case IRON_HELM:
						case IRON_SHIELD:
							return false;
						default:
							return true;
					}
				case MAGICSTAFF:
					return false;
				case THROWN:
					return false;
				default:
					return false;
			}
		}
		else if ( monsterAllyClass == ALLY_CLASS_MELEE )
		{
			switch ( itemCategory(&item) )
			{
			case WEAPON:
				return !isRangedWeapon(item);
			case ARMOR:
				return true;
			case MAGICSTAFF:
				return false;
			case THROWN:
				return false;
			default:
				return false;
			}
		}
	}

	switch ( itemCategory(&item) )
	{
		case WEAPON:
			return true;
		case ARMOR:
			return true;
		case MAGICSTAFF:
			return false;
		case THROWN:
			return false;
		default:
			return false;
	}

	return false;
}

void Entity::humanSetLimbsClient(int bodypart)
{
	int skinColor = 0;
	int sex = MALE;

	// get the skinColor/sex from the head sprite.

	if ( (sprite >= 113 && sprite < 118) 
		|| (sprite >= 125 && sprite < 130) 
		|| (sprite >= 332 && sprite < 334) )
	{
		skinColor = 0; // light.
		if ( (sprite >= 125 && sprite < 130)
			|| sprite == 333 )
		{
			sex = FEMALE;
		}
	}
	else if ( (sprite >= 341 && sprite < 347)
		|| (sprite >= 354 && sprite < 360) )
	{
		skinColor = 1; // medium.
		if ( sprite >= 354 && sprite < 360 )
		{
			sex = FEMALE;
		}
	}
	else if ( (sprite >= 367 && sprite < 373)
		|| (sprite >= 380 && sprite < 386) )
	{
		skinColor = 2; // dark.
		if ( sprite >= 380 && sprite < 386 )
		{
			sex = FEMALE;
		}
	}

	node_t* limbNode = list_Node(&this->children, bodypart);
	Entity* limb = nullptr;
	if ( limbNode )
	{
		limb = (Entity*)limbNode->element;
	}

	if ( !limb )
	{
		return;
	}

	switch ( bodypart )
	{
		case LIMB_HUMANOID_TORSO:
			switch ( skinColor )
			{
				case 1:
					limb->sprite = 334 + 13 * sex;
					break;
				case 2:
					limb->sprite = 360 + 13 * sex;
					break;
				default:
					limb->sprite = 106 + 12 * sex;
					break;
			}
			break;
		case LIMB_HUMANOID_RIGHTLEG:
			switch ( skinColor )
			{
				case 1:
					limb->sprite = 335 + 13 * sex;
					break;
				case 2:
					limb->sprite = 361 + 13 * sex;
					break;
				default:
					limb->sprite = 107 + 12 * sex;
					break;
			}
			break;
		case LIMB_HUMANOID_LEFTLEG:
			switch ( skinColor )
			{
				case 1:
					limb->sprite = 336 + 13 * sex;
					break;
				case 2:
					limb->sprite = 362 + 13 * sex;
					break;
				default:
					limb->sprite = 108 + 12 * sex;
					break;
			}
			break;
		case LIMB_HUMANOID_RIGHTARM:
			switch ( skinColor )
			{
				case 1:
					limb->sprite = 337 + 13 * sex;
					break;
				case 2:
					limb->sprite = 363 + 13 * sex;
					break;
				default:
					limb->sprite = 109 + 12 * sex;
					break;
			}
			break;
		case LIMB_HUMANOID_LEFTARM:
			switch ( skinColor )
			{
				case 1:
					limb->sprite = 338 + 13 * sex;
					break;
				case 2:
					limb->sprite = 364 + 13 * sex;
					break;
				default:
					limb->sprite = 110 + 12 * sex;
					break;
			}
			break;
	}
}
