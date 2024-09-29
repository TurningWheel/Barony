/*-------------------------------------------------------------------------------

	BARONY
	File: monster_goblin.cpp
	Desc: implements all of the goblin monster's code

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

void initGoblin(Entity* my, Stat* myStats)
{
	node_t* node;

	my->flags[BURNABLE] = true;

	//Sprite 180 = Goblin head model.
	my->initMonster(180);
	my->z = 0;

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 60;
		MONSTER_SPOTVAR = 3;
		MONSTER_IDLESND = 98;
		MONSTER_IDLEVAR = 3;
	}
	if ( multiplayer != CLIENT && !MONSTER_INIT )
	{
		auto& rng = my->entity_rng ? *my->entity_rng : local_rng;

		if ( myStats != nullptr )
		{
		    if (myStats->sex == FEMALE)
		    {
		        my->sprite = 1039;
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
			bool potatoking = false;
			const bool boss =
			    rng.rand() % 50 == 0 &&
			    !my->flags[USERFLAG2] &&
			    !myStats->MISC_FLAGS[STAT_FLAG_DISABLE_MINIBOSS];
			if ( (boss || (*cvar_summonBosses && conductGameChallenges[CONDUCT_CHEATS_ENABLED])) && myStats->leader_uid == 0 )
			{
			    potatoking = true;
				myStats->setAttribute("special_npc", "potato king");
				strcpy(myStats->name, MonsterData_t::getSpecialNPCName(*myStats).c_str());
				my->sprite = MonsterData_t::getSpecialNPCBaseModel(*myStats);
			    myStats->sex = MALE;
				myStats->HP = 120;
				myStats->MAXHP = 120;
				myStats->OLDHP = myStats->HP;
				myStats->STR += 6;
				int status = DECREPIT + (currentlevel > 5) + (currentlevel > 15) + (currentlevel > 20);
				myStats->weapon = newItem(ARTIFACT_MACE, static_cast<Status>(status), 1, 1, rng.rand(), true, nullptr);
				myStats->helmet = newItem(HAT_JESTER, SERVICABLE, 3 + rng.rand() % 3, 1, rng.rand(), false, nullptr);

				int c;
				for ( c = 0; c < 3; c++ )
				{
					Entity* entity = summonMonster(GOBLIN, my->x, my->y);
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

			// random effects
			if ( rng.rand() % 8 == 0 )
			{
				myStats->EFFECTS[EFF_ASLEEP] = true;
				myStats->EFFECTS_TIMERS[EFF_ASLEEP] = 1800 + rng.rand() % 1800;
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

			if (!potatoking) {

			    //give weapon
			    if ( myStats->weapon == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1 )
			    {
				    switch ( rng.rand() % 10 )
				    {
					    case 0:
					    case 1:
					    case 2:
						    myStats->weapon = newItem(SHORTBOW, WORN, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						    break;
					    case 3:
					    case 4:
					    case 5:
						    myStats->weapon = newItem(BRONZE_AXE, WORN, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						    break;
					    case 6:
					    case 7:
						    myStats->weapon = newItem(IRON_MACE, WORN, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						    break;
					    case 8:
						    myStats->weapon = newItem(IRON_AXE, WORN, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						    break;
					    case 9:
						    myStats->weapon = newItem(MAGICSTAFF_FIRE, EXCELLENT, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						    break;
				    }
			    }

			    if ( myStats->weapon && isMeleeWeapon(*myStats->weapon) )
			    {
				    myStats->CHR = -3; // don't retreat
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
					    // give shield
					    switch ( rng.rand() % 10 )
					    {
						    case 0:
						    case 1:
							    myStats->shield = newItem(TOOL_TORCH, SERVICABLE, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
							    break;
						    case 2:
						    case 3:
						    case 4:
							    break;
						    case 5:
						    case 6:
							    myStats->shield = newItem(WOODEN_SHIELD, DECREPIT, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
							    break;
						    case 7:
						    case 8:
							    myStats->shield = newItem(BRONZE_SHIELD, DECREPIT, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
							    break;
						    case 9:
							    myStats->shield = newItem(IRON_SHIELD, DECREPIT, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
							    break;
					    }
				    }
			    }

			    // give cloak
			    if ( myStats->cloak == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_CLOAK] == 1 )
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
					    case 8:
						    myStats->cloak = newItem(CLOAK, WORN, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						    break;
					    case 9:
						    myStats->cloak = newItem(CLOAK_MAGICREFLECTION, WORN, 0, 1, rng.rand(), false, nullptr);
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
							break;
					    case 2:
							myStats->helmet = newItem(static_cast<ItemType>(HAT_WOLF_HOOD + rng.rand() % 4), 
								WORN, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						    break;
					    case 3:
					    case 4:
						    myStats->helmet = newItem(HAT_PHRYGIAN, WORN, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						    break;
					    case 5:
						    myStats->helmet = newItem(HAT_WIZARD, WORN, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						    break;
					    case 6:
							if ( myStats->weapon && itemCategory(myStats->weapon) == MAGICSTAFF )
							{
								myStats->helmet = newItem(HAT_HEADDRESS, WORN, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
								break;
							}
					    case 7:
						    myStats->helmet = newItem(LEATHER_HELM, WORN, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						    break;
					    case 8:
					    case 9:
						    myStats->helmet = newItem(IRON_HELM, WORN, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
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
					    case 2:
					    case 3:
					    case 4:
						    break;
					    case 5:
					    case 6:
					    case 7:
						    myStats->breastplate = newItem(LEATHER_BREASTPIECE, DECREPIT, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						    break;
					    case 8:
					    case 9:
						    myStats->breastplate = newItem(IRON_BREASTPIECE, DECREPIT, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						    break;
				    }
			    }

				if ( myStats->mask == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_MASK] == 1 )
				{
					switch ( rng.rand() % 20 )
					{
					case 0:
						myStats->mask = newItem(MASK_GRASS_SPRIG, WORN, -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						break;
					default:
						break;
					}
				}
			}
		}
	}

	// torso
	const int torso_sprite = my->sprite == 1035 ? 1038 :
	    (my->sprite == 1039 ? 1042 : 183);
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
	entity->focalx = limbs[GOBLIN][1][0]; // 0
	entity->focaly = limbs[GOBLIN][1][1]; // 0
	entity->focalz = limbs[GOBLIN][1][2]; // 0
	entity->behavior = &actGoblinLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right leg
	const int rleg_sprite = my->sprite == 1035 ? 1037 :
	    (my->sprite == 1039 ? 1041 : 182);
	entity = newEntity(rleg_sprite, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[GOBLIN][2][0]; // 0
	entity->focaly = limbs[GOBLIN][2][1]; // 0
	entity->focalz = limbs[GOBLIN][2][2]; // 2
	entity->behavior = &actGoblinLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left leg
	const int lleg_sprite = my->sprite == 1035 ? 1036 :
	    (my->sprite == 1039 ? 1040 : 181);
	entity = newEntity(lleg_sprite, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[GOBLIN][3][0]; // 0
	entity->focaly = limbs[GOBLIN][3][1]; // 0
	entity->focalz = limbs[GOBLIN][3][2]; // 2
	entity->behavior = &actGoblinLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right arm
	entity = newEntity(my->sprite == 1035 ? 1033 : 178, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[GOBLIN][4][0]; // 0
	entity->focaly = limbs[GOBLIN][4][1]; // 0
	entity->focalz = limbs[GOBLIN][4][2]; // 1.5
	entity->behavior = &actGoblinLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left arm
	entity = newEntity(my->sprite == 1035 ? 1031 : 176, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[GOBLIN][5][0]; // 0
	entity->focaly = limbs[GOBLIN][5][1]; // 0
	entity->focalz = limbs[GOBLIN][5][2]; // 1.5
	entity->behavior = &actGoblinLimb;
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
	entity->focalx = limbs[GOBLIN][6][0]; // 1.5
	entity->focaly = limbs[GOBLIN][6][1]; // 0
	entity->focalz = limbs[GOBLIN][6][2]; // -.5
	entity->behavior = &actGoblinLimb;
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
	entity->focalx = limbs[GOBLIN][7][0]; // 2
	entity->focaly = limbs[GOBLIN][7][1]; // 0
	entity->focalz = limbs[GOBLIN][7][2]; // 0
	entity->behavior = &actGoblinLimb;
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
	entity->focalx = limbs[GOBLIN][8][0]; // 0
	entity->focaly = limbs[GOBLIN][8][1]; // 0
	entity->focalz = limbs[GOBLIN][8][2]; // 4
	entity->behavior = &actGoblinLimb;
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
	entity->focalx = limbs[GOBLIN][9][0]; // 0
	entity->focaly = limbs[GOBLIN][9][1]; // 0
	entity->focalz = limbs[GOBLIN][9][2]; // -2
	entity->behavior = &actGoblinLimb;
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
	entity->focalx = limbs[GOBLIN][10][0]; // 0
	entity->focaly = limbs[GOBLIN][10][1]; // 0
	entity->focalz = limbs[GOBLIN][10][2]; // .25
	entity->behavior = &actGoblinLimb;
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

void actGoblinLimb(Entity* my)
{
	my->actMonsterLimb(true);
}

void goblinDie(Entity* my)
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

	playSoundEntity(my, 63 + local_rng.rand() % 3, 128);

	my->removeMonsterDeathNodes();

	list_RemoveNode(my->mynode);
	return;
}

#define GOBLINWALKSPEED .13

void goblinMoveBodyparts(Entity* my, Stat* myStats, double dist)
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
			my->humanoidAnimateWalk(entity, node, bodypart, GOBLINWALKSPEED, dist, 0.4);
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

			my->humanoidAnimateWalk(entity, node, bodypart, GOBLINWALKSPEED, dist, 0.4);
			
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
	                    const int torso_sprite = my->sprite == 1035 ? 1038 :
	                        (my->sprite == 1039 ? 1042 : 183);
						entity->sprite = torso_sprite;
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
				my->setHumanoidLimbOffset(entity, GOBLIN, LIMB_HUMANOID_TORSO);
				break;
			// right leg
			case LIMB_HUMANOID_RIGHTLEG:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->shoes == nullptr )
					{
	                    const int rleg_sprite = my->sprite == 1035 ? 1037 :
	                        (my->sprite == 1039 ? 1041 : 182);
						entity->sprite = rleg_sprite;
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
				my->setHumanoidLimbOffset(entity, GOBLIN, LIMB_HUMANOID_RIGHTLEG);
				break;
			// left leg
			case LIMB_HUMANOID_LEFTLEG:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->shoes == nullptr )
					{
	                    const int lleg_sprite = my->sprite == 1035 ? 1036 :
	                        (my->sprite == 1039 ? 1040 : 181);
						entity->sprite = lleg_sprite;
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
				my->setHumanoidLimbOffset(entity, GOBLIN, LIMB_HUMANOID_LEFTLEG);
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
						entity->focalx = limbs[GOBLIN][4][0]; // 0
						entity->focaly = limbs[GOBLIN][4][1]; // 0
						entity->focalz = limbs[GOBLIN][4][2]; // 2
						entity->sprite = my->sprite == 1035 ? 1033 : 178;
					}
					else
					{
						// else flex arm.
						entity->focalx = limbs[GOBLIN][4][0] + 0.75;
						entity->focaly = limbs[GOBLIN][4][1];
						entity->focalz = limbs[GOBLIN][4][2] - 0.75;
						entity->sprite = my->sprite == 1035 ? 1034 : 179;
					}
				}
				my->setHumanoidLimbOffset(entity, GOBLIN, LIMB_HUMANOID_RIGHTARM);
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
						entity->focalx = limbs[GOBLIN][5][0]; // 0
						entity->focaly = limbs[GOBLIN][5][1]; // 0
						entity->focalz = limbs[GOBLIN][5][2]; // 2
						entity->sprite = my->sprite == 1035 ? 1031 : 176;
					}
					else
					{
						entity->focalx = limbs[GOBLIN][5][0] + 0.75;
						entity->focaly = limbs[GOBLIN][5][1];
						entity->focalz = limbs[GOBLIN][5][2] - 0.75;
						entity->sprite = my->sprite == 1035 ? 1032 : 177;
					}
				}
				my->setHumanoidLimbOffset(entity, GOBLIN, LIMB_HUMANOID_LEFTARM);
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
			case 8:
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
				entity->focalx = limbs[GOBLIN][9][0]; // 0
				entity->focaly = limbs[GOBLIN][9][1]; // 0
				entity->focalz = limbs[GOBLIN][9][2]; // -2
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
				entity->focalx = limbs[GOBLIN][10][0]; // 0
				entity->focaly = limbs[GOBLIN][10][1]; // 0
				entity->focalz = limbs[GOBLIN][10][2]; // .25
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

				if ( entity->sprite != 165 && entity->sprite != 1196 )
				{
					if ( entity->sprite == items[MASK_SHAMAN].index )
					{
						entity->roll = 0;
						my->setHelmetLimbOffset(entity);
						my->setHelmetLimbOffsetWithMask(helmet, entity);
					}
					else if ( EquipmentModelOffsets.modelOffsetExists(GOBLIN, entity->sprite) )
					{
						my->setHelmetLimbOffset(entity);
						my->setHelmetLimbOffsetWithMask(helmet, entity);
					}
					else
					{
						entity->focalx = limbs[GOBLIN][10][0] + .35; // .35
						entity->focaly = limbs[GOBLIN][10][1] - 2; // -2
						entity->focalz = limbs[GOBLIN][10][2]; // .25
					}
				}
				else
				{
					entity->focalx = limbs[GOBLIN][10][0] + .25; // .25
					entity->focaly = limbs[GOBLIN][10][1] - 2.25; // -2.25
					entity->focalz = limbs[GOBLIN][10][2]; // .25

					if ( entity->sprite == 1196 ) // MonocleWorn.vox
					{
						entity->focalx -= .5;
						entity->focalz -= .05;
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

bool Entity::goblinCanWieldItem(const Item& item) const
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
		case ARMOR:
			return true;
		case MAGICSTAFF:
			return true;
		case THROWN:
			return true;
		case TOOL:
			if ( itemTypeIsQuiver(item.type) )
			{
				return true;
			}
			break;
		default:
			return false;
	}

	return false;
}
