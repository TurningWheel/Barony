/*-------------------------------------------------------------------------------

	BARONY
	File: monster_incubus.cpp
	Desc: implements all of the incubus monster's code

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
#include "mod_tools.hpp"

void initIncubus(Entity* my, Stat* myStats)
{
	node_t* node;

	my->flags[BURNABLE] = true;
	my->initMonster(445); //Sprite 445 = incubus head sprite
	my->z = -1;

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 282;
		MONSTER_SPOTVAR = 3;
		MONSTER_IDLESND = 276;
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

			if ( !strncmp(myStats->name, "inner demon", strlen("inner demon")) )
			{
				Entity* parent = uidToEntity(my->parent);
				if ( !parent )
				{
					myStats->HP = 0;
				}
				else
				{
					Stat* parentStats = parent->getStats();
					if ( !parentStats )
					{
						myStats->HP = 0;
					}
					else
					{
						myStats->HP = parentStats->HP;
						myStats->MAXHP = myStats->HP;
						myStats->OLDHP = myStats->HP;
						myStats->STR = -999;
						myStats->DEX = std::min(parentStats->DEX, 15);
						// pretend the parent wasn't defending as this gets added in AC() call.
						bool wasDefending = parentStats->defending;
						parentStats->defending = false;
						myStats->CON = AC(parentStats);
						parentStats->defending = wasDefending;

						myStats->INT = parentStats->INT;
						myStats->PER = parentStats->PER;
						myStats->CHR = parentStats->CHR;
						myStats->LVL = parentStats->LVL;
						myStats->GOLD = 0;
			
						myStats->weapon = newItem(TOOL_WHIP, EXCELLENT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, true, nullptr);
						/*if ( parentStats->shield )
						{
							myStats->shield = newItem(BRONZE_SWORD, EXCELLENT, 0, 1, 0, true, nullptr);
							copyItem(myStats->shield, parentStats->shield);
							myStats->shield->appearance = MONSTER_ITEM_UNDROPPABLE_APPEARANCE;
						}
						if ( parentStats->helmet )
						{
							myStats->helmet = newItem(BRONZE_SWORD, EXCELLENT, 0, 1, 0, true, nullptr);
							copyItem(myStats->helmet, parentStats->helmet);
							myStats->helmet->appearance = MONSTER_ITEM_UNDROPPABLE_APPEARANCE;
						}
						if ( parentStats->breastplate )
						{
							myStats->breastplate = newItem(BRONZE_SWORD, EXCELLENT, 0, 1, 0, true, nullptr);
							copyItem(myStats->breastplate, parentStats->breastplate);
							myStats->breastplate->appearance = MONSTER_ITEM_UNDROPPABLE_APPEARANCE;
						}
						if ( parentStats->shoes )
						{
							myStats->shoes = newItem(BRONZE_SWORD, EXCELLENT, 0, 1, 0, true, nullptr);
							copyItem(myStats->shoes, parentStats->shoes);
							myStats->shoes->appearance = MONSTER_ITEM_UNDROPPABLE_APPEARANCE;
						}
						if ( parentStats->gloves )
						{
							myStats->gloves = newItem(BRONZE_SWORD, EXCELLENT, 0, 1, 0, true, nullptr);
							copyItem(myStats->gloves, parentStats->gloves);
							myStats->gloves->appearance = MONSTER_ITEM_UNDROPPABLE_APPEARANCE;
						}
						if ( parentStats->cloak )
						{
							myStats->cloak = newItem(BRONZE_SWORD, EXCELLENT, 0, 1, 0, true, nullptr);
							copyItem(myStats->cloak, parentStats->cloak);
							myStats->cloak->appearance = MONSTER_ITEM_UNDROPPABLE_APPEARANCE;
						}
						if ( parentStats->ring )
						{
							myStats->ring = newItem(BRONZE_SWORD, EXCELLENT, 0, 1, 0, true, nullptr);
							copyItem(myStats->ring, parentStats->ring);
							myStats->ring->appearance = MONSTER_ITEM_UNDROPPABLE_APPEARANCE;
						}
						if ( parentStats->amulet )
						{
							myStats->amulet = newItem(BRONZE_SWORD, EXCELLENT, 0, 1, 0, true, nullptr);
							copyItem(myStats->amulet, parentStats->amulet);
							myStats->amulet->appearance = MONSTER_ITEM_UNDROPPABLE_APPEARANCE;
						}
						if ( parentStats->mask )
						{
							myStats->mask = newItem(BRONZE_SWORD, EXCELLENT, 0, 1, 0, true, nullptr);
							copyItem(myStats->mask, parentStats->mask);
							myStats->mask->appearance = MONSTER_ITEM_UNDROPPABLE_APPEARANCE;
						}*/
					}
				}
			}
			else
			{
				bool lesserMonster = false;
				if ( !strncmp(myStats->name, "lesser incubus", strlen("lesser incubus")) )
				{
					lesserMonster = true;
					myStats->HP = 80;
					myStats->MAXHP = myStats->HP;
					myStats->OLDHP = myStats->HP;
					myStats->STR = 12;
					myStats->DEX = 6;
					myStats->CON = 3;
					myStats->INT = -2;
					myStats->PER = 5;
					myStats->CHR = 5;
					myStats->EXP = 0;
					myStats->LVL = 15;
				}
				// apply random stat increases if set in stat_shared.cpp or editor
				setRandomMonsterStats(myStats, rng);

				// generate 6 items max, less if there are any forced items from boss variants
				int customItemsToGenerate = ITEM_CUSTOM_SLOT_LIMIT;

				// boss variants

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

				// always give special spell to incubus, undroppable.
				newItem(SPELLBOOK_STEAL_WEAPON, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, &myStats->inventory);
				newItem(SPELLBOOK_CHARM_MONSTER, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, &myStats->inventory);

				if ( rng.rand() % 4 == 0 ) // 1 in 4
				{
					newItem(POTION_CONFUSION, SERVICABLE, 0, 0 + rng.rand() % 3, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, &myStats->inventory);
				}
				else // 3 in 4
				{
					newItem(POTION_BOOZE, SERVICABLE, 0, 1 + rng.rand() % 3, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, &myStats->inventory);
				}


				// generate the default inventory items for the monster, provided the editor sprite allowed enough default slots
				switch ( defaultItems )
				{
					case 6:
					case 5:
					case 4:
					case 3:
						if ( rng.rand() % 2 == 0 && !lesserMonster ) // 1 in 2
						{
							newItem(MAGICSTAFF_COLD, SERVICABLE, 0, 1, rng.rand(), false, &myStats->inventory);
						}
					case 2:
						if ( rng.rand() % 5 == 0 ) // 1 in 5
						{
							if ( rng.rand() % 2 == 0 )
							{
								newItem(MASK_MASQUERADE, WORN, -2 + rng.rand() % 3, 1, rng.rand(), false, &myStats->inventory);
							}
							else
							{
								newItem(POTION_CONFUSION, SERVICABLE, 0, 1 + rng.rand() % 2, rng.rand(), false, &myStats->inventory);
							}
						}
					case 1:
						if ( rng.rand() % 3 == 0 ) // 1 in 3
						{
							newItem(POTION_BOOZE, SERVICABLE, 0, 1 + rng.rand() % 3, rng.rand(), false, &myStats->inventory);
						}
						break;
					default:
						break;
				}

				//give weapon
				if ( lesserMonster )
				{
					if ( myStats->weapon == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1 )
					{
						switch ( rng.rand() % 10 )
						{
							case 0:
							case 1:
								myStats->weapon = newItem(CROSSBOW, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
								break;
							case 2:
							case 3:
							case 4:
							case 5:
							case 6:
							case 7:
							case 8:
								myStats->weapon = newItem(STEEL_HALBERD, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
								break;
							case 9:
								myStats->weapon = newItem(MAGICSTAFF_COLD, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
								break;
						}
					}
				}
				else if ( myStats->weapon == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1 )
				{
					switch ( rng.rand() % 10 )
					{
						case 0:
						case 1:
						case 2:
						case 3:
							myStats->weapon = newItem(CRYSTAL_SPEAR, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
							break;
						case 4:
						case 5:
						case 6:
							myStats->weapon = newItem(CROSSBOW, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
							break;
						case 7:
						case 8:
							myStats->weapon = newItem(STEEL_HALBERD, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
							break;
						case 9:
							myStats->weapon = newItem(MAGICSTAFF_COLD, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
							break;
					}
				}

				// give shield
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
							case 6:
							case 7:
								break;
							case 8:
							case 9:
								myStats->shield = newItem(MIRROR_SHIELD, static_cast<Status>(WORN + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
								break;
						}
					}
				}
			}
		}
	}

	// torso
	Entity* entity = newEntity(446, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[INCUBUS][1][0]; // 0
	entity->focaly = limbs[INCUBUS][1][1]; // 0
	entity->focalz = limbs[INCUBUS][1][2]; // 0
	entity->behavior = &actIncubusLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right leg
	entity = newEntity(450, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[INCUBUS][2][0]; // 1
	entity->focaly = limbs[INCUBUS][2][1]; // 0
	entity->focalz = limbs[INCUBUS][2][2]; // 2
	entity->behavior = &actIncubusLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left leg
	entity = newEntity(449, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[INCUBUS][3][0]; // 1
	entity->focaly = limbs[INCUBUS][3][1]; // 0
	entity->focalz = limbs[INCUBUS][3][2]; // 2
	entity->behavior = &actIncubusLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right arm
	entity = newEntity(448, 1, map.entities, nullptr); //595 //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[INCUBUS][4][0]; // -.25
	entity->focaly = limbs[INCUBUS][4][1]; // 0
	entity->focalz = limbs[INCUBUS][4][2]; // 1.5
	entity->behavior = &actIncubusLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left arm
	entity = newEntity(447, 1, map.entities, nullptr); //447 //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[INCUBUS][5][0]; // -.25
	entity->focaly = limbs[INCUBUS][5][1]; // 0
	entity->focalz = limbs[INCUBUS][5][2]; // 1.5
	entity->behavior = &actIncubusLimb;
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
	entity->focalx = limbs[INCUBUS][6][0]; // 
	entity->focaly = limbs[INCUBUS][6][1]; // 
	entity->focalz = limbs[INCUBUS][6][2]; // 
	entity->behavior = &actIncubusLimb;
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
	entity->focalx = limbs[INCUBUS][7][0]; // 
	entity->focaly = limbs[INCUBUS][7][1]; // 
	entity->focalz = limbs[INCUBUS][7][2]; // 
	entity->behavior = &actIncubusLimb;
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
	entity->focalx = limbs[INCUBUS][8][0]; // 0
	entity->focaly = limbs[INCUBUS][8][1]; // 0
	entity->focalz = limbs[INCUBUS][8][2]; // 4
	entity->behavior = &actIncubusLimb;
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
	entity->focalx = limbs[INCUBUS][9][0]; // 0
	entity->focaly = limbs[INCUBUS][9][1]; // 0
	entity->focalz = limbs[INCUBUS][9][2]; // -2
	entity->behavior = &actIncubusLimb;
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
	entity->focalx = limbs[INCUBUS][10][0]; // 0
	entity->focaly = limbs[INCUBUS][10][1]; // 0
	entity->focalz = limbs[INCUBUS][10][2]; // .5
	entity->behavior = &actIncubusLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);
}

void actIncubusLimb(Entity* my)
{
	my->actMonsterLimb();
}

void incubusDie(Entity* my)
{
	Stat* myStats = my->getStats();
	if ( myStats && !strncmp(myStats->name, "inner demon", strlen("inner demon")) )
	{
		// die, no blood.
		//spawnMagicEffectParticles(my->x, my->y, my->z, 171);
		createParticleErupt(my, 983);
		serverSpawnMiscParticles(my, PARTICLE_EFFECT_ERUPT, 983);
		//playSoundEntity(my, 279 + local_rng.rand() % 3, 128);
		playSoundEntity(my, 178, 128);
		my->removeMonsterDeathNodes();
		list_RemoveNode(my->mynode);
		return;
	}

	for ( int c = 0; c < 12; c++ )
	{
		Entity* gib = spawnGib(my);
		if (c < 6) {
		    gib->sprite = 445 + c;
		    gib->skill[5] = 1; // poof
		}
		serverSpawnGibForClient(gib);
	}

	my->spawnBlood();

	playSoundEntity(my, 279 + local_rng.rand() % 3, 128);

	my->removeMonsterDeathNodes();

	list_RemoveNode(my->mynode);
	return;
}

#define INCUBUSWALKSPEED .07

void incubusMoveBodyparts(Entity* my, Stat* myStats, double dist)
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
		{
			if ( myStats->ring->type == RING_INVISIBILITY )
			{
				wearingring = true;
			}
		}
		if ( myStats->cloak != nullptr )
		{
			if ( myStats->cloak->type == CLOAK_INVISIBILITY )
			{
				wearingring = true;
			}
		}
		if ( myStats->EFFECTS[EFF_INVISIBLE] == true )
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

		if ( !strncmp(myStats->name, "inner demon", strlen("inner demon")) )
		{
			Entity* parent = uidToEntity(my->parent);
			if ( !parent )
			{
				myStats->HP = 0;
			}
			else if ( my->ticks > TICKS_PER_SECOND * 5 )
			{
				myStats->HP = 0;
			}
		}

		// sleeping
		if ( myStats->EFFECTS[EFF_ASLEEP] )
		{
			my->z = 1.5;
		}
		else
		{
			my->z = -1;
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
			if ( multiplayer != CLIENT && bodypart == 1 )
			{
				if ( my->monsterAttack != MONSTER_POSE_MAGIC_WINDUP3 
					&& my->monsterAttack != MONSTER_POSE_INCUBUS_TELEPORT
					&& my->monsterAttack != MONSTER_POSE_INCUBUS_TAUNT )
				{
					limbAnimateToLimit(my, ANIMATE_PITCH, 0.1, 0, false, 0.0);
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
			if ( bodypart == LIMB_HUMANOID_LEFTARM &&
				((my->monsterSpecialState == INCUBUS_STEAL && my->monsterAttack != 0 ) ||
				my->monsterAttack == MONSTER_POSE_INCUBUS_TELEPORT
					|| my->monsterAttack == MONSTER_POSE_INCUBUS_TAUNT) )
			{
				// leftarm follows the right arm during special steal state/teleport attack
				// will not work when shield is visible
				// else animate normally.
				node_t* shieldNode = list_Node(&my->children, 8);
				if ( shieldNode )
				{
					Entity* shield = (Entity*)shieldNode->element;
					if ( shield->flags[INVISIBLE] )
					{
						Entity* weaponarm = nullptr;
						node_t* weaponarmNode = list_Node(&my->children, LIMB_HUMANOID_RIGHTARM);
						if ( weaponarmNode )
						{
							weaponarm = (Entity*)weaponarmNode->element;
						}
						else
						{
							return;
						}
						entity->pitch = weaponarm->pitch;
						entity->roll = -weaponarm->roll;
					}
				}
			}
			else
			{
				my->humanoidAnimateWalk(entity, node, bodypart, INCUBUSWALKSPEED, dist, 0.4);
			}
		}
		else if ( bodypart == LIMB_HUMANOID_LEFTLEG || bodypart == LIMB_HUMANOID_RIGHTARM || bodypart == LIMB_HUMANOID_CLOAK )
		{
			// left leg, right arm, cloak.
			if ( bodypart == LIMB_HUMANOID_RIGHTARM )
			{
				weaponarm = entity;
				if ( my->monsterAttack > 0 )
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

					// potion special throw
					if ( my->monsterAttack == MONSTER_POSE_SPECIAL_WINDUP1 )
					{
						if ( my->monsterAttackTime == 0 )
						{
							// init rotations
							weaponarm->pitch = 0;
							my->monsterArmbended = 0;
							my->monsterWeaponYaw = 0;
							weaponarm->roll = 0;
							createParticleDot(my);
						}

						limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.25, 5 * PI / 4, false, 0.0);

						if ( my->monsterAttackTime >= ANIMATE_DURATION_WINDUP * 4 / (monsterGlobalAnimationMultiplier / 10.0) )
						{
							if ( multiplayer != CLIENT )
							{
								my->attack(1, 0, nullptr);
							}
						}
						++my->monsterAttackTime;
					}
					else if ( my->monsterAttack == MONSTER_POSE_MAGIC_WINDUP3 )
					{
						if ( my->monsterAttackTime == 0 )
						{
							// init rotations
							weaponarm->pitch = 0;
							my->monsterArmbended = 0;
							my->monsterWeaponYaw = 0;
							weaponarm->roll = 0;
							weaponarm->skill[1] = 0;
							createParticleDot(my);
							// play casting sound
							playSoundEntityLocal(my, 170, 64);
							// monster scream
							playSoundEntityLocal(my, MONSTER_SPOTSND + 1, 128);
						}

						limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.25, 5 * PI / 4, false, 0.0);
						if ( multiplayer != CLIENT )
						{
							// move the head and weapon yaw
							limbAnimateToLimit(my, ANIMATE_PITCH, -0.1, 14 * PI / 8, true, 0.1);
							limbAnimateToLimit(my, ANIMATE_WEAPON_YAW, 0.25, 1 * PI / 8, false, 0.0);
						}

						if ( my->monsterAttackTime >= 3 * ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
						{
							if ( multiplayer != CLIENT )
							{
								// throw the spell
								my->attack(MONSTER_POSE_MELEE_WINDUP1, 0, nullptr);
							}
						}
					}
					// teleport animation
					else if( my->monsterAttack == MONSTER_POSE_INCUBUS_TELEPORT
						|| my->monsterAttack == MONSTER_POSE_INCUBUS_TAUNT )
					{
						if ( my->monsterAttackTime == 0 )
						{
							// init rotations
							weaponarm->pitch = 0;
							my->monsterArmbended = 0;
							my->monsterWeaponYaw = 0;
							weaponarm->roll = 0;
							weaponarm->skill[1] = 0; // use this for direction of animation
							// monster scream
							if ( my->monsterAttack == MONSTER_POSE_INCUBUS_TAUNT )
							{
								playSoundEntityLocal(my, 276 + local_rng.rand() % 3, 128);
							}
							else
							{
								playSoundEntityLocal(my, 282 + 2, 128);
							}
							if ( multiplayer != CLIENT )
							{
								// set overshoot for head, freeze incubus in place
								my->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_TO_SETPOINT;
								myStats->EFFECTS[EFF_PARALYZED] = true;
								if ( my->monsterAttack == MONSTER_POSE_INCUBUS_TAUNT )
								{
									myStats->EFFECTS_TIMERS[EFF_PARALYZED] = 250;
								}
								else
								{
									myStats->EFFECTS_TIMERS[EFF_PARALYZED] = 100;
								}
							}
						}

						if ( weaponarm->skill[1] == 0 )
						{
							// wind up arm, change direction when setpoint reached.
							if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.2, 13 * PI / 8, false, 0.0) )
							{
								weaponarm->skill[1] = 1;
							}
						}
						else
						{
							// swing and flare out arm.
							limbAnimateToLimit(weaponarm, ANIMATE_PITCH, 0.1, 2 * PI / 16, false, 0.0);
							limbAnimateToLimit(weaponarm, ANIMATE_ROLL, -0.1, 31 * PI / 16, false, 0.0);
						}

						if ( multiplayer != CLIENT )
						{
							// move the head back and forth.
							// keeps between PI and 0 (2PI) so we can lower the head at completion to 0 (2PI).
							if ( my->monsterAnimationLimbOvershoot >= ANIMATE_OVERSHOOT_TO_SETPOINT )
							{
								if ( my->monsterAttack == MONSTER_POSE_INCUBUS_TAUNT )
								{
									limbAnimateWithOvershoot(my, ANIMATE_PITCH, -0.05, 7 * PI / 4, -0.05, 15 * PI / 8, ANIMATE_DIR_POSITIVE);
								}
								else
								{
									limbAnimateWithOvershoot(my, ANIMATE_PITCH, -0.1, 7 * PI / 4, -0.1, 15 * PI / 8, ANIMATE_DIR_POSITIVE);
								}
							}
							else
							{
								// after 1 cycle is complete, reset the overshoot flag and repeat the animation.
								my->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_TO_SETPOINT;
							}
						}

						// animation takes roughly 2 seconds.
						int duration = 10;
						if ( my->monsterAttack == MONSTER_POSE_INCUBUS_TAUNT )
						{
							duration = 50;
						}
						if ( my->monsterAttackTime >= ANIMATE_DURATION_WINDUP * duration / (monsterGlobalAnimationMultiplier / 10.0) )
						{
							weaponarm->skill[0] = rightbody->skill[0];
							weaponarm->pitch = rightbody->pitch;
							weaponarm->roll = -PI / 32;
							my->monsterArmbended = 0;
							my->monsterAttack = 0;
							Entity* leftarm = nullptr;
							node_t* leftarmNode = list_Node(&my->children, LIMB_HUMANOID_LEFTARM);
							if ( leftarmNode )
							{
								leftarm = (Entity*)leftarmNode->element;
								leftarm->roll = PI / 32;
							}
							else
							{
								return;
							}
							if ( multiplayer != CLIENT )
							{
								my->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_NONE;
							}
						}
						++my->monsterAttackTime; // manually increment timer
					}
					else
					{
						my->handleWeaponArmAttack(weaponarm);
						if ( my->monsterAttack != MONSTER_POSE_MELEE_WINDUP2 && my->monsterAttack != 2 )
						{
							// flare out the weapon arm to match neutral arm position. 
							// breaks the horizontal chop attack animation so we skip it.
							weaponarm->roll = -PI / 32;
						}
					}
				}
			}
			else if ( bodypart == LIMB_HUMANOID_CLOAK )
			{
				entity->pitch = entity->fskill[0];
			}

			my->humanoidAnimateWalk(entity, node, bodypart, INCUBUSWALKSPEED, dist, 0.4);

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
				my->setHumanoidLimbOffset(entity, INCUBUS, LIMB_HUMANOID_TORSO);
				break;
			// right leg
			case LIMB_HUMANOID_RIGHTLEG:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->shoes == nullptr )
					{
						entity->sprite = 450;
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
				my->setHumanoidLimbOffset(entity, INCUBUS, LIMB_HUMANOID_RIGHTLEG);
				break;
			// left leg
			case LIMB_HUMANOID_LEFTLEG:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->shoes == nullptr )
					{
						entity->sprite = 449;
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
				my->setHumanoidLimbOffset(entity, INCUBUS, LIMB_HUMANOID_LEFTLEG);
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
						entity->focalx = limbs[INCUBUS][4][0] - 0.25; // 0
						entity->focaly = limbs[INCUBUS][4][1] - 0.25; // 0
						entity->focalz = limbs[INCUBUS][4][2]; // 2
						entity->sprite = 448;
						if ( my->monsterAttack == 0 )
						{
							entity->roll = -PI / 32;
						}
					}
					else
					{
						// else flex arm.
						entity->focalx = limbs[INCUBUS][4][0];
						entity->focaly = limbs[INCUBUS][4][1];
						entity->focalz = limbs[INCUBUS][4][2];
						entity->sprite = 595;
					}
				}
				my->setHumanoidLimbOffset(entity, INCUBUS, LIMB_HUMANOID_RIGHTARM);
				entity->yaw += MONSTER_WEAPONYAW;
				break;
			}
			// left arm
			case LIMB_HUMANOID_LEFTARM:
			{
				shieldarm = entity;
				node_t* shieldNode = list_Node(&my->children, 8);
				if ( shieldNode )
				{
					Entity* shield = (Entity*)shieldNode->element;
					if ( shield->flags[INVISIBLE] && (my->monsterState != MONSTER_STATE_ATTACK) )
					{
						// if weapon invisible and I'm not attacking, relax arm.
						entity->focalx = limbs[INCUBUS][5][0] - 0.25; // 0
						entity->focaly = limbs[INCUBUS][5][1] + 0.25; // 0
						entity->focalz = limbs[INCUBUS][5][2]; // 2
						entity->sprite = 447;
						if ( my->monsterAttack == 0 )
						{
							entity->roll = PI / 32;
						}
					}
					else
					{
						// else flex arm.
						entity->focalx = limbs[INCUBUS][5][0];
						entity->focaly = limbs[INCUBUS][5][1];
						entity->focalz = limbs[INCUBUS][5][2];
						entity->sprite = 594;
						if ( my->monsterSpecialState == INCUBUS_STEAL )
						{
							entity->yaw -= MONSTER_WEAPONYAW;
						}
					}
				}
				my->setHumanoidLimbOffset(entity, INCUBUS, LIMB_HUMANOID_LEFTARM);
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
			{
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
			}
			// shield
			case LIMB_HUMANOID_SHIELD:
			{
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
				entity->focalx = limbs[INCUBUS][9][0]; // 0
				entity->focaly = limbs[INCUBUS][9][1]; // 0
				entity->focalz = limbs[INCUBUS][9][2]; // -2
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
				entity->focalx = limbs[INCUBUS][10][0]; // 0
				entity->focaly = limbs[INCUBUS][10][1]; // 0
				entity->focalz = limbs[INCUBUS][10][2]; // .5
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
					else if ( EquipmentModelOffsets.modelOffsetExists(INCUBUS, entity->sprite) )
					{
						my->setHelmetLimbOffset(entity);
						my->setHelmetLimbOffsetWithMask(helmet, entity);
					}
					else
					{
						entity->focalx = limbs[INCUBUS][10][0] + .35; // .35
						entity->focaly = limbs[INCUBUS][10][1] - 2.5; // -2
						entity->focalz = limbs[INCUBUS][10][2]; // .5
					}
				}
				else
				{
					entity->focalx = limbs[INCUBUS][10][0] + .25; // .25
					entity->focaly = limbs[INCUBUS][10][1] - 2.5; // -2.25
					entity->focalz = limbs[INCUBUS][10][2]; // .5

					if ( entity->sprite == 1196 ) // MonocleWorn.vox
					{
						entity->focalx -= .4;
					}
				}
				break;
			}
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

void Entity::incubusChooseWeapon(const Entity* target, double dist)
{
	if ( monsterSpecialState != 0 )
	{
		//Holding a weapon assigned from the special attack. Don't switch weapons.
		if ( monsterSpecialState == INCUBUS_TELEPORT_STEAL && monsterSpecialTimer == 0 )
		{
			// handle steal weapon random teleportation
			incubusTeleportRandom();
			monsterSpecialState = 0;
			serverUpdateEntitySkill(this, 33); // for clients to keep track of animation
			attack(MONSTER_POSE_INCUBUS_TELEPORT, 0, nullptr);
		}
		else if ( monsterSpecialState == INCUBUS_TELEPORT && monsterSpecialTimer == 0 )
		{
			// handle idle teleporting to target
			monsterSpecialState = 0;
			serverUpdateEntitySkill(this, 33); // for clients to keep track of animation
		}
		return;
	}

	Stat *myStats = getStats();
	if ( !myStats )
	{
		return;
	}

	if ( !strncmp(myStats->name, "inner demon", strlen("inner demon")) )
	{
		return;
	}

	int specialRoll = -1;
	int bonusFromHP = 0;

	if ( ticks % 10 == 0 && monsterSpecialState != INCUBUS_TELEPORT_STEAL )
	{
		// teleport to target, higher chance at greater distance or lower HP
		specialRoll = local_rng.rand() % 50;
		if ( specialRoll < (1 + (dist > 80 ? 4 : 0) + (myStats->HP <= myStats->MAXHP * 0.8 ? 4 : 0)) )
		{
			monsterSpecialState = INCUBUS_TELEPORT;
			incubusTeleportToTarget(target);
			return;
		}
	}

	if ( monsterSpecialTimer == 0 && (ticks % 10 == 0) && monsterAttack == 0 )
	{
		Stat* targetStats = target->getStats();
		if ( !targetStats )
		{
			return;
		}

		bool tryCharm = true;
		bool trySteal = true;
		if ( targetStats->EFFECTS[EFF_PACIFY] )
		{
			tryCharm = false;
		}
		if ( !targetStats->weapon )
		{
			trySteal = false;
		}

		if ( trySteal || tryCharm )
		{
			if ( myStats->HP <= myStats->MAXHP * 0.8 )
			{
				bonusFromHP += 1; // +2.5% chance if on low health
			}
			if ( myStats->HP <= myStats->MAXHP * 0.4 )
			{
				bonusFromHP += 1; // +extra 2.5% chance if on lower health
			}

			int requiredRoll = (1 + bonusFromHP + (targetStats->EFFECTS[EFF_CONFUSED] ? 4 : 0)
				+ (targetStats->EFFECTS[EFF_DRUNK] ? 2 : 0)
				+ (targetStats->EFFECTS[EFF_PACIFY] ? 2 : 0)); // +2.5% base, + extra if target is inebriated

			if ( trySteal && tryCharm )
			{
				if ( local_rng.rand() % 8 == 0 )
				{
					trySteal = false; // try charm 12.5% of the time.
				}
				else
				{
					tryCharm = false;
				}
			}

			if ( trySteal )
			{
				// try to steal weapon if target is holding.
				// occurs less often against fellow monsters.
				specialRoll = local_rng.rand() % (40 + 40 * (target->behavior == &actMonster));
			}
			else if ( tryCharm )
			{
				specialRoll = local_rng.rand() % 40;
			}

			if ( trySteal )
			{
				requiredRoll += (myStats->weapon == nullptr ? 3 : 0); // bonus if no weapon held
			}

			if ( specialRoll < requiredRoll ) 
			{
				node_t* node = nullptr;
				if ( trySteal )
				{
					node = itemNodeInInventory(myStats, SPELLBOOK_STEAL_WEAPON, static_cast<Category>(-1));
					if ( node != nullptr )
					{
						swapMonsterWeaponWithInventoryItem(this, myStats, node, true, true);
						monsterSpecialState = INCUBUS_STEAL;
						serverUpdateEntitySkill(this, 33); // for clients to keep track of animation
						return;
					}
				}
				else if ( tryCharm )
				{
					node = itemNodeInInventory(myStats, SPELLBOOK_CHARM_MONSTER, static_cast<Category>(-1));
					if ( node != nullptr )
					{
						swapMonsterWeaponWithInventoryItem(this, myStats, node, true, true);
						monsterSpecialState = INCUBUS_CHARM;
						serverUpdateEntitySkill(this, 33); // for clients to keep track of animation
						return;
					}
				}
			}
		}
		
		// try new roll for alternate potion throw special.
		// occurs less often against fellow monsters.
		specialRoll = local_rng.rand() % (30 + 30 * (target->behavior == &actMonster));
		if ( specialRoll < (2 + bonusFromHP) ) // +5% base
		{
			node_t* node = nullptr;
			node = itemNodeInInventory(myStats, -1, POTION);
			if ( node != nullptr )
			{
				swapMonsterWeaponWithInventoryItem(this, myStats, node, true, true);
				monsterSpecialState = INCUBUS_CONFUSION;
				return;
			}
		}
	}

	bool inMeleeRange = monsterInMeleeRange(target, dist);

	if ( inMeleeRange )
	{
		//Switch to a melee weapon if not already wielding one. Unless monster special state is overriding the AI.
		if ( !myStats->weapon || !isMeleeWeapon(*myStats->weapon) )
		{
			node_t* weaponNode = getMeleeWeaponItemNodeInInventory(myStats);
			if ( !weaponNode )
			{
				return; //Resort to fists.
			}

			bool swapped = swapMonsterWeaponWithInventoryItem(this, myStats, weaponNode, false, false);
			if ( !swapped )
			{
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

	//Switch to a thrown weapon or a ranged weapon.
	if ( !myStats->weapon || isMeleeWeapon(*myStats->weapon) )
	{
		node_t *weaponNode = getRangedWeaponItemNodeInInventory(myStats, true);
		if ( !weaponNode )
		{
			return; //Nothing available
		}
		bool swapped = swapMonsterWeaponWithInventoryItem(this, myStats, weaponNode, false, false);
		return;
	}
	return;
}

void Entity::incubusTeleportToTarget(const Entity* target)
{
	Entity* spellTimer = createParticleTimer(this, 40, 593);
	spellTimer->particleTimerEndAction = PARTICLE_EFFECT_INCUBUS_TELEPORT_TARGET; // teleport behavior of timer.
	spellTimer->particleTimerEndSprite = 593; // sprite to use for end of timer function.
	spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_SHOOT_PARTICLES;
	spellTimer->particleTimerCountdownSprite = 593;
	if ( target != nullptr )
	{
		spellTimer->particleTimerTarget = static_cast<Sint32>(target->getUID()); // get the target to teleport around.
	}
	spellTimer->particleTimerVariable1 = 3; // distance of teleport in tiles
	if ( multiplayer == SERVER )
	{
		serverSpawnMiscParticles(this, PARTICLE_EFFECT_INCUBUS_TELEPORT_TARGET, 593);
	}
}

void Entity::incubusTeleportRandom()
{
	Entity* spellTimer = createParticleTimer(this, 80, 593);
	spellTimer->particleTimerEndAction = PARTICLE_EFFECT_INCUBUS_TELEPORT_STEAL; // teleport behavior of timer.
	spellTimer->particleTimerEndSprite = 593; // sprite to use for end of timer function.
	spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_SHOOT_PARTICLES;
	spellTimer->particleTimerCountdownSprite = 593;
	spellTimer->particleTimerPreDelay = 40;
	if ( multiplayer == SERVER )
	{
		serverSpawnMiscParticles(this, PARTICLE_EFFECT_INCUBUS_TELEPORT_STEAL, 593);
	}
}
