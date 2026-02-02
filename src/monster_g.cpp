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

enum MonsterGVariant
{
	NONE,
	SAPPER,
	SKIRMISHER,
	BERSERKER
};

void initMonsterG(Entity* my, Stat* myStats)
{
	int c;
	node_t* node;

	my->flags[BURNABLE] = true;
	my->initMonster(1569);
	my->z = getNormalHeightMonsterG(*my);

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 736;
		MONSTER_SPOTVAR = 3;
		MONSTER_IDLESND = 730;
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
				myStats->setEffectActive(EFF_ASLEEP, 1);
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

			MonsterGVariant variant = NONE;

			Item* item = nullptr;
			if ( myStats->getAttribute("monster_g_type") == "" )
			{
				switch ( rng.rand() % 3 )
				{
				case 0:
					myStats->setAttribute("monster_g_type", "sapper");
					variant = SAPPER;
					if ( defaultItems >= 1 )
					{
						item = newItem(GREASE_BALL, SERVICABLE, 0, rng.rand() % 2 + 2, rng.rand(), false, &myStats->inventory);
						item->isDroppable = rng.rand() % 5 == 0;
					}
					if ( defaultItems >= 2 )
					{
						if ( rng.rand() % 4 == 0 )
						{
							item = newItem(POTION_FIRESTORM, SERVICABLE, 0, rng.rand() % 2 + 1, rng.rand(), false, &myStats->inventory);
							item->isDroppable = rng.rand() % 5 == 0;
						}
						item = newItem(POTION_SICKNESS, SERVICABLE, -2, rng.rand() % 2 + 1, rng.rand(), false, &myStats->inventory);
						item->isDroppable = rng.rand() % 5 == 0;
					}
					break;
				case 1:
					myStats->setAttribute("monster_g_type", "skirmisher");
					variant = SKIRMISHER;
					if ( defaultItems >= 1 )
					{
						item = newItem(BOLAS, SERVICABLE, 0, rng.rand() % 2 + 2, rng.rand(), false, &myStats->inventory);
						item->isDroppable = rng.rand() % 5 == 0;
					}
					if ( defaultItems >= 2 )
					{
						item = newItem(GREASE_BALL, SERVICABLE, 0, rng.rand() % 2 + 2, rng.rand(), false, &myStats->inventory);
						item->isDroppable = rng.rand() % 5 == 0;
					}
					break;
				case 2:
					myStats->setAttribute("monster_g_type", "berserker");
					variant = BERSERKER;
					break;
				default:
					break;
				}
			}

			if ( myStats->weapon == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1 )
			{
				if ( variant == SKIRMISHER )
				{
					if ( rng.rand() % 2 )
					{
						myStats->weapon = newItem(STEEL_SWORD, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
					}
					else
					{
						myStats->weapon = newItem(STEEL_AXE, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
					}
				}
				else if ( variant == SAPPER )
				{
					if ( rng.rand() % 5 == 0 )
					{
						myStats->weapon = newItem(STEEL_FLAIL, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
					}
					else
					{
						myStats->weapon = newItem(STEEL_MACE, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
					}
					
				}
				else if ( variant == BERSERKER )
				{
					if ( rng.rand() % 2 )
					{
						myStats->weapon = newItem(STEEL_AXE, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
					}
				}
			}

			if ( myStats->shield == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_SHIELD] == 1 )
			{
				if ( variant == SAPPER )
				{
					if ( rng.rand() % 2 )
					{
					}
					else
					{
						if ( rng.rand() % 2 )
						{
							myStats->shield = newItem(IRON_SHIELD, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						}
						else
						{
							myStats->shield = newItem(WOODEN_SHIELD, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						}
					}
				}
			}

			if ( myStats->helmet == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_HELM] == 1 )
			{
				if ( variant == SKIRMISHER )
				{
					myStats->helmet = newItem(HAT_HOOD, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
				}
			}

			if ( myStats->shoes == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_BOOTS] == 1 )
			{
				if ( variant == SKIRMISHER )
				{
					if ( rng.rand() % 2 )
					{
						myStats->shoes = newItem(LEATHER_BOOTS, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
					}
				}
				else if ( variant == SAPPER )
				{
					if ( rng.rand() % 2 )
					{
						myStats->shoes = newItem(LEATHER_BOOTS, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
					}
				}
			}

			if ( myStats->gloves == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_GLOVES] == 1 )
			{
				if ( variant == SKIRMISHER )
				{
					if ( rng.rand() % 2 )
					{
						myStats->gloves = newItem(GLOVES, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
					}
				}
				else if ( variant == SAPPER )
				{
					if ( rng.rand() % 2 )
					{
						myStats->gloves = newItem(GLOVES, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
					}
				}
				else if ( variant == BERSERKER )
				{
					if ( !myStats->weapon )
					{
						if ( rng.rand() % 2 )
						{
							myStats->gloves = newItem(IRON_KNUCKLES, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						}
						else if ( rng.rand() % 3 > 0 )
						{
							myStats->gloves = newItem(BRASS_KNUCKLES, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						}
						else
						{
							myStats->gloves = newItem(SPIKED_GAUNTLETS, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						}
					}
				}
			}

			if ( myStats->cloak == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_CLOAK] == 1 )
			{
				if ( variant == SKIRMISHER )
				{
					if ( rng.rand() % 2 )
					{
						myStats->cloak = newItem(CLOAK, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
					}
				}
			}

			if ( myStats->breastplate == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_ARMOR] == 1 )
			{
				if ( variant == BERSERKER )
				{
					if ( rng.rand() % 2 )
					{
						myStats->breastplate = newItem(TUNIC, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
					}
				}
			}

			if ( myStats->amulet == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_AMULET] == 1 )
			{
				if ( variant == SAPPER )
				{
					if ( rng.rand() % 10 == 0 )
					{
						myStats->amulet = newItem(AMULET_BURNINGRESIST, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						myStats->amulet->isDroppable = rng.rand() % 5 == 0;
					}
					else
					{
						myStats->amulet = newItem(AMULET_BURNINGRESIST, static_cast<Status>(WORN + rng.rand() % 2), -1, 1, rng.rand(), false, nullptr);
						myStats->amulet->isDroppable = rng.rand() % 10 == 0;
					}
				}
			}
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
	entity->focalx = limbs[GREMLIN][1][0]; // 0
	entity->focaly = limbs[GREMLIN][1][1]; // 0
	entity->focalz = limbs[GREMLIN][1][2]; // 0
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
	entity->focalx = limbs[GREMLIN][2][0]; // .25
	entity->focaly = limbs[GREMLIN][2][1]; // 0
	entity->focalz = limbs[GREMLIN][2][2]; // 1.5
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
	entity->focalx = limbs[GREMLIN][3][0]; // .25
	entity->focaly = limbs[GREMLIN][3][1]; // 0
	entity->focalz = limbs[GREMLIN][3][2]; // 1.5
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
	entity->focalx = limbs[GREMLIN][4][0]; // 0
	entity->focaly = limbs[GREMLIN][4][1]; // 0
	entity->focalz = limbs[GREMLIN][4][2]; // 2
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
	entity->focalx = limbs[GREMLIN][5][0]; // 0
	entity->focaly = limbs[GREMLIN][5][1]; // 0
	entity->focalz = limbs[GREMLIN][5][2]; // 2
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
	entity->focalx = limbs[GREMLIN][6][0]; // 2
	entity->focaly = limbs[GREMLIN][6][1]; // 0
	entity->focalz = limbs[GREMLIN][6][2]; // -.5
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
	entity->focalx = limbs[GREMLIN][7][0]; // 0
	entity->focaly = limbs[GREMLIN][7][1]; // 0
	entity->focalz = limbs[GREMLIN][7][2]; // 1.5
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
	entity->focalx = limbs[GREMLIN][8][0]; // 0
	entity->focaly = limbs[GREMLIN][8][1]; // 0
	entity->focalz = limbs[GREMLIN][8][2]; // 4
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
	entity->focalx = limbs[GREMLIN][9][0]; // 0
	entity->focaly = limbs[GREMLIN][9][1]; // 0
	entity->focalz = limbs[GREMLIN][9][2]; // -2
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
	entity->focalx = limbs[GREMLIN][10][0]; // 0
	entity->focaly = limbs[GREMLIN][10][1]; // 0
	entity->focalz = limbs[GREMLIN][10][2]; // .25
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

	playSoundEntity(my, 733 + local_rng.rand() % 3, 128);

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

	my->focalx = limbs[GREMLIN][0][0];
	my->focaly = limbs[GREMLIN][0][1];
	my->focalz = limbs[GREMLIN][0][2];
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
		if ( myStats->getEffectActive(EFF_INVISIBLE) || wearingring == true )
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
		if ( myStats->getEffectActive(EFF_ASLEEP) )
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
		my->creatureHandleLiftZ();
	}

	Entity* shieldarm = nullptr;
	Entity* helmet = nullptr;
	Entity* torso = nullptr;

	//Move bodyparts
	for (bodypart = 0, node = my->children.first; node != nullptr; node = node->next, bodypart++)
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
							playSoundEntityLocal(my, 736 + local_rng.rand() % 3, 128);
							createParticleDot(my);
							if ( multiplayer != CLIENT )
							{
								my->setEffect(EFF_STUNNED, true, 40, false);
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
								int spellID = SPELL_SPEED;
								if ( my->monsterSpecialState == MONSTER_G_SPECIAL_CAST1 )
								{
									castSpell(my->getUID(), getSpellFromID(spellID), true, false);
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
								my->monsterSpecialState == MONSTER_G_SPECIAL_CAST1
								&& my->monsterAttackTime == 0 )
							{
								my->setEffect(EFF_STUNNED, true, 50, false);
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
				entity->scalex = 1.0;
				entity->scaley = 1.0;
				entity->scalez = 1.0;
				entity->focalx = limbs[GREMLIN][1][0];
				entity->focaly = limbs[GREMLIN][1][1];
				entity->focalz = limbs[GREMLIN][1][2];
				torso = entity;
				if ( multiplayer != CLIENT )
				{
					if ( myStats->breastplate == nullptr || !itemModel(myStats->breastplate, true, my) )
					{
						entity->sprite = my->sprite == 1569 ? 1583 : 1584;
					}
					else
					{
						entity->sprite = itemModel(myStats->breastplate, true, my);
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
				my->setHumanoidLimbOffset(entity, GREMLIN, LIMB_HUMANOID_TORSO);
				break;
			// right leg
			case LIMB_HUMANOID_RIGHTLEG:
				entity->focalx = limbs[GREMLIN][2][0];
				entity->focaly = limbs[GREMLIN][2][1];
				entity->focalz = limbs[GREMLIN][2][2];
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
				my->setHumanoidLimbOffset(entity, GREMLIN, LIMB_HUMANOID_RIGHTLEG);
				break;
			// left leg
			case LIMB_HUMANOID_LEFTLEG:
				entity->focalx = limbs[GREMLIN][3][0];
				entity->focaly = limbs[GREMLIN][3][1];
				entity->focalz = limbs[GREMLIN][3][2];
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
				my->setHumanoidLimbOffset(entity, GREMLIN, LIMB_HUMANOID_LEFTLEG);
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
						entity->focalx = limbs[GREMLIN][4][0]; // 0
						entity->focaly = limbs[GREMLIN][4][1]; // 0
						entity->focalz = limbs[GREMLIN][4][2]; // 2
					}
					else
					{
						entity->focalx = limbs[GREMLIN][4][0] + 1; // 1
						entity->focaly = limbs[GREMLIN][4][1] + 0.25; // 0
						entity->focalz = limbs[GREMLIN][4][2] - 0.75; // 1
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
				my->setHumanoidLimbOffset(entity, GREMLIN, LIMB_HUMANOID_RIGHTARM);
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
						entity->focalx = limbs[GREMLIN][5][0]; // 0
						entity->focaly = limbs[GREMLIN][5][1]; // 0
						entity->focalz = limbs[GREMLIN][5][2]; // 2
					}
					else
					{
						entity->focalx = limbs[GREMLIN][5][0] + 1; // 1
						entity->focaly = limbs[GREMLIN][5][1] - 0.25; // 0
						entity->focalz = limbs[GREMLIN][5][2] - 0.75; // 1
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
				my->setHumanoidLimbOffset(entity, GREMLIN, LIMB_HUMANOID_LEFTARM);
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
				entity->focalx = limbs[GREMLIN][8][0];
				entity->focaly = limbs[GREMLIN][8][1];
				entity->focalz = limbs[GREMLIN][8][2];
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
				//if ( torso->sprite != 1583 && torso->sprite != 1584 )
				//{
				//	// push back for larger armors
				//}
				entity->x -= cos(my->yaw) * 1.0;
				entity->y -= sin(my->yaw) * 1.0;
				entity->yaw += PI / 2;
				my->setHumanoidLimbOffset(entity, GREMLIN, LIMB_HUMANOID_CLOAK);
				break;
				// helm
			case LIMB_HUMANOID_HELMET:
				helmet = entity;
				entity->focalx = limbs[GREMLIN][9][0]; // 0
				entity->focaly = limbs[GREMLIN][9][1]; // 0
				entity->focalz = limbs[GREMLIN][9][2]; // -2
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
				entity->focalx = limbs[GREMLIN][10][0]; // 0
				entity->focaly = limbs[GREMLIN][10][1]; // 0
				entity->focalz = limbs[GREMLIN][10][2]; // .25
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

				if ( entity->sprite == items[MASK_SHAMAN].index )
				{
					entity->roll = 0;
					my->setHelmetLimbOffset(entity);
					my->setHelmetLimbOffsetWithMask(helmet, entity);
				}
				else if ( EquipmentModelOffsets.modelOffsetExists(GREMLIN, entity->sprite, my->sprite) )
				{
					my->setHelmetLimbOffset(entity);
					my->setHelmetLimbOffsetWithMask(helmet, entity);
				}
				else
				{
					entity->focalx = limbs[GREMLIN][10][0] + .35; // .35
					entity->focaly = limbs[GREMLIN][10][1] - 2; // -2
					entity->focalz = limbs[GREMLIN][10][2]; // .25
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

void Entity::monsterGChooseWeapon(const Entity* target, double dist)
{
	if ( monsterSpecialState != 0 )
	{
		//Holding a weapon assigned from the special attack. Don't switch weapons.
		//messagePlayer()
		return;
	}

	Stat* myStats = getStats();
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

	if ( myStats->getAttribute("monster_g_type") == "berserker" )
	{
		int roll = 10;
		if ( myStats->getEffectActive(EFF_SLOW) || myStats->HP <= myStats->MAXHP / 2 )
		{
			roll = 3;
		}
		if ( monsterSpecialTimer == 0 && (ticks % 10 == 0) && monsterAttack == 0
			&& local_rng.rand() % roll == 0 && !myStats->getEffectActive(EFF_FAST) )
		{
			if ( dist >= TOUCHRANGE )
			{
				monsterSpecialState = MONSTER_G_SPECIAL_CAST1;
			}
		}
		return;
	}

	//Switch to a thrown weapon or a ranged weapon
	if ( dist < 80.0 )
	{
		int tiles = dist / 16;
		//First search the inventory for a THROWN weapon.
		node_t* weaponNode = nullptr;
		int roll = 10;
		if ( tiles <= 3 )
		{
			roll = 5;
		}
		if ( monsterSpecialTimer == 0 && (ticks % 10 == 0) && monsterAttack == 0
			&& local_rng.rand() % roll == 0 )
		{
			Stat* targetStats = target ? target->getStats() : nullptr;
			if ( (dist > STRIKERANGE) || (targetStats && targetStats->getEffectActive(EFF_MAGIC_GREASE)) )
			{
				if ( (targetStats && targetStats->getEffectActive(EFF_MAGIC_GREASE)) )
				{
					// first item
					weaponNode = itemNodeInInventory(myStats, -1, POTION);
				}
				else
				{
					weaponNode = itemNodeInInventory(myStats, -1, POTION, true); // random
				}
			}
			if ( !weaponNode || local_rng.rand() % 4 == 0 )
			{
				weaponNode = itemNodeInInventory(myStats, -1, THROWN, true);
			}
			if ( weaponNode )
			{
				if ( swapMonsterWeaponWithInventoryItem(this, myStats, weaponNode, false, true) )
				{
					monsterSpecialState = MONSTER_G_SPECIAL_THROW;
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