/*-------------------------------------------------------------------------------

BARONY
File: monster_lich.cpp
Desc: implements all of the lich monster's code

Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "monster.hpp"
#include "sound.hpp"
#include "items.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "magic/magic.hpp"
#include "paths.hpp"

static const int LICH_BODY = 0;
static const int LICH_RIGHTARM = 2;
static const int LICH_LEFTARM = 3;
static const int LICH_HEAD = 4;
static const int LICH_WEAPON = 5;

void initLichIce(Entity* my, Stat* myStats)
{
	my->initMonster(650);

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 377;
		MONSTER_SPOTVAR = 3;
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
			setRandomMonsterStats(myStats);

			// generate 6 items max, less if there are any forced items from boss variants
			int customItemsToGenerate = ITEM_CUSTOM_SLOT_LIMIT;

			// boss variants

			// random effects
			myStats->EFFECTS[EFF_LEVITATING] = true;
			myStats->EFFECTS_TIMERS[EFF_LEVITATING] = 0;

			// generates equipment and weapons if available from editor
			createMonsterEquipment(myStats);

			// create any custom inventory items from editor if available
			createCustomInventory(myStats, customItemsToGenerate);

			// count if any custom inventory items from editor
			int customItems = countCustomItems(myStats); //max limit of 6 custom items per entity.

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
				default:
					break;
			}

			//give weapon
			if ( myStats->weapon == NULL && myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1 )
			{
				//myStats->weapon = newItem(SPELLBOOK_LIGHTNING, EXCELLENT, 0, 1, 0, false, NULL);
			}
		}
	}

	// right arm
	Entity* entity = newEntity(653, 0, map.entities, nullptr);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[LICH_ICE][1][0]; // 0
	entity->focaly = limbs[LICH_ICE][1][1]; // 0
	entity->focalz = limbs[LICH_ICE][1][2]; // 2
	entity->behavior = &actLichIceLimb;
	entity->parent = my->getUID();
	node_t* node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left arm
	entity = newEntity(652, 0, map.entities, nullptr);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[LICH_ICE][2][0]; // 0
	entity->focaly = limbs[LICH_ICE][2][1]; // 0
	entity->focalz = limbs[LICH_ICE][2][2]; // 2
	entity->behavior = &actLichIceLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// head
	entity = newEntity(651, 0, map.entities, nullptr);
	entity->yaw = my->yaw;
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[LICH_ICE][3][0]; // 0
	entity->focaly = limbs[LICH_ICE][3][1]; // 0
	entity->focalz = limbs[LICH_ICE][3][2]; // -2
	entity->behavior = &actLichIceLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// world weapon
	entity = newEntity(-1, 0, map.entities, nullptr);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[LICH_ICE][4][0]; // 1.5
	entity->focaly = limbs[LICH_ICE][4][1]; // 0
	entity->focalz = limbs[LICH_ICE][4][2]; // -.5
	entity->behavior = &actLichIceLimb;
	entity->parent = my->getUID();
	entity->pitch = .25;
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);
}

void lichIceDie(Entity* my)
{
	node_t* node, *nextnode;
	int c;
	for ( c = 0; c < 20; c++ )
	{
		Entity* entity = spawnGib(my);
		if ( entity )
		{
			switch ( c )
			{
				case 0:
					entity->sprite = 230;
					break;
				case 1:
					entity->sprite = 231;
					break;
				case 2:
					entity->sprite = 233;
					break;
				case 3:
					entity->sprite = 235;
					break;
				case 4:
					entity->sprite = 236;
					break;
				case 5:
					entity->sprite = 650;
					break;
				case 6:
					entity->sprite = 651;
					break;
				case 7:
					entity->sprite = 652;
					break;
				case 8:
					entity->sprite = 653;
					break;
				default:
					break;
			}
			serverSpawnGibForClient(entity);
		}
	}
	my->removeMonsterDeathNodes();
	playSoundEntity(my, 94, 128);
	my->removeLightField();
	// kill all other monsters on the level
	/*for ( node = map.entities->first; node != NULL; node = nextnode )
	{
	nextnode = node->next;
	Entity* entity = (Entity*)node->element;
	if ( entity == my )
	{
	continue;
	}
	if ( entity->behavior == &actMonster )
	{
	spawnExplosion(entity->x, entity->y, entity->z);
	Stat* stats = entity->getStats();
	if ( stats )
	if ( stats->type != HUMAN )
	{
	stats->HP = 0;
	}
	}
	}
	for ( c = 0; c < MAXPLAYERS; c++ )
	{
	playSoundPlayer(c, 153, 128);
	steamAchievementClient(c, "BARONY_ACH_LICH_HUNTER");
	}
	if ( multiplayer == SERVER )
	{
	for ( c = 1; c < MAXPLAYERS; c++ )
	{
	if ( client_disconnected[c] )
	{
	continue;
	}
	strcpy((char*)net_packet->data, "BDTH");
	net_packet->address.host = net_clients[c - 1].host;
	net_packet->address.port = net_clients[c - 1].port;
	net_packet->len = 4;
	sendPacketSafe(net_sock, -1, net_packet, c - 1);
	}
	}
	*/
	spawnExplosion(my->x, my->y, my->z);
	list_RemoveNode(my->mynode);
	return;
}

void actLichIceLimb(Entity* my)
{
	my->actMonsterLimb();
}

void lichIceAnimate(Entity* my, Stat* myStats, double dist)
{
	node_t* node;
	Entity* entity = nullptr, *entity2 = nullptr;
	Entity* rightbody = nullptr;
	Entity* weaponarm = nullptr;
	Entity* head = nullptr;
	Entity* spellarm = nullptr;
	int bodypart;
	bool wearingring = false;

	// remove old light field
	my->removeLightField();

	// obtain head entity
	node = list_Node(&my->children, LICH_HEAD);
	if ( node )
	{
		head = (Entity*)node->element;
	}

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
			for ( node = my->children.first; node != nullptr; node = node->next )
			{
				if ( bodypart < LICH_RIGHTARM )
				{
					bodypart++;
					continue;
				}
				if ( bodypart >= LICH_WEAPON )
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
				if ( bodypart < LICH_RIGHTARM )
				{
					bodypart++;
					continue;
				}
				if ( bodypart >= LICH_WEAPON )
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

		// check tiles around the monster to be freed.
		if ( my->monsterLichBattleState == LICH_BATTLE_IMMOBILE && my->ticks > TICKS_PER_SECOND )
		{
			int sides = 0;
			int my_x = static_cast<int>(my->x) >> 4;
			int my_y = static_cast<int>(my->y) >> 4;
			int mapIndex = (my_y)* MAPLAYERS + (my_x + 1) * MAPLAYERS * map.height;
			if ( map.tiles[OBSTACLELAYER + mapIndex] )   // wall
			{
				++sides;
			}
			mapIndex = (my_y)* MAPLAYERS + (my_x - 1) * MAPLAYERS * map.height;
			if ( map.tiles[OBSTACLELAYER + mapIndex] )   // wall
			{
				++sides;
			}
			mapIndex = (my_y + 1) * MAPLAYERS + (my_x)* MAPLAYERS * map.height;
			if ( map.tiles[OBSTACLELAYER + mapIndex] )   // wall
			{
				++sides;
			}
			mapIndex = (my_y - 1) * MAPLAYERS + (my_x)* MAPLAYERS * map.height;
			if ( map.tiles[OBSTACLELAYER + mapIndex] )   // wall
			{
				++sides;
			}
			if ( sides != 4 )
			{
				my->monsterLichBattleState = LICH_BATTLE_READY;
				generatePathMaps();
				/*swornenemies[LICH_ICE][AUTOMATON] = false;
				swornenemies[LICH_FIRE][AUTOMATON] = false;
				swornenemies[AUTOMATON][HUMAN] = true;*/
				real_t distToPlayer = 0;
				int c, playerToChase = -1;
				for ( c = 0; c < MAXPLAYERS; c++ )
				{
					if ( players[c] && players[c]->entity )
					{
						if ( !distToPlayer )
						{
							distToPlayer = sqrt(pow(my->x - players[c]->entity->x, 2) + pow(my->y - players[c]->entity->y, 2));
							playerToChase = c;
						}
						else
						{
							double newDistToPlayer = sqrt(pow(my->x - players[c]->entity->x, 2) + pow(my->y - players[c]->entity->y, 2));
							if ( newDistToPlayer < distToPlayer )
							{
								distToPlayer = newDistToPlayer;
								playerToChase = c;
							}
						}
					}
				}
				if ( playerToChase >= 0 )
				{
					if ( players[playerToChase] && players[playerToChase]->entity )
					{
						my->monsterAcquireAttackTarget(*players[playerToChase]->entity, MONSTER_STATE_PATH);
					}
				}
			}
		}

		// passive floating effect, server only.
		if ( my->monsterState == MONSTER_STATE_LICHICE_DIE )
		{
			my->z -= 0.03;
		}
		if ( my->monsterAttack == 0 )
		{
			if ( my->monsterAnimationLimbOvershoot == ANIMATE_OVERSHOOT_NONE )
			{
				if ( my->z < -1.2 )
				{
					my->z += 0.25;
				}
				else
				{
					my->z = -1.2;
					my->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_TO_SETPOINT;
				}
			}
			if ( dist < 0.1 )
			{
				// not moving, float.
				limbAnimateWithOvershoot(my, ANIMATE_Z, 0.005, -1.5, 0.005, -1.2, ANIMATE_DIR_NEGATIVE);
			}
		}
		else if ( my->monsterAttack == 1 || my->monsterAttack == 3 )
		{
			if ( my->z < -1.2 )
			{
				my->z += 0.25;
			}
			else
			{
				my->z = -1.2;
				my->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_NONE;
			}
		}
	}
	else
	{
		/*if ( !my->skill[27] )
		{
		my->light = lightSphereShadow(my->x / 16, my->y / 16, 4, 192);
		}*/
	}

	//Lich stares you down while he does his special ability windup, and any of his spellcasting animations.
	if ( (my->monsterAttack == MONSTER_POSE_MAGIC_WINDUP1
		|| my->monsterAttack == MONSTER_POSE_MAGIC_WINDUP2
		|| my->monsterAttack == MONSTER_POSE_SPECIAL_WINDUP1
		|| my->monsterAttack == MONSTER_POSE_MAGIC_CAST1
		|| my->monsterAttack == MONSTER_POSE_SPECIAL_WINDUP2
		|| my->monsterAttack == MONSTER_POSE_SPECIAL_WINDUP3
		|| my->monsterState == MONSTER_STATE_LICH_CASTSPELLS)
		&& my->monsterState != MONSTER_STATE_LICHICE_DIE )
	{
		//Always turn to face the target.
		Entity* target = uidToEntity(my->monsterTarget);
		if ( target )
		{
			my->lookAtEntity(*target);
			my->monsterRotate();
		}
	}

	// move arms
	Entity* rightarm = nullptr;
	for ( bodypart = 0, node = my->children.first; node != NULL; node = node->next, bodypart++ )
	{
		if ( bodypart < LICH_RIGHTARM )
		{
			if ( bodypart == 0 ) // insert head/body animation here.
			{
				if ( my->monsterAttack == MONSTER_POSE_SPECIAL_WINDUP1 )
				{
					if ( multiplayer != CLIENT && my->monsterAnimationLimbOvershoot >= ANIMATE_OVERSHOOT_TO_SETPOINT )
					{
						// handle z movement on windup
						limbAnimateWithOvershoot(my, ANIMATE_Z, 0.2, -0.6, 0.1, -3.2, ANIMATE_DIR_POSITIVE); // default z is -1.2
						if ( my->z > -0.5 )
						{
							my->z = -0.6; //failsafe for floating too low sometimes?
						}
					}
				}
				else if ( my->monsterAttack == MONSTER_POSE_MELEE_WINDUP3 || my->monsterAttack == MONSTER_POSE_SPECIAL_WINDUP3 )
				{
					if ( multiplayer != CLIENT && my->monsterAnimationLimbOvershoot >= ANIMATE_OVERSHOOT_TO_SETPOINT )
					{
						// handle z movement on windup
						limbAnimateWithOvershoot(my, ANIMATE_Z, 0.3, -0.6, 0.3, -4.0, ANIMATE_DIR_POSITIVE); // default z is -1.2
						if ( my->z > -0.5 )
						{
							my->z = -0.6; //failsafe for floating too low sometimes?
						}
					}
				}
				else if ( my->monsterAttack == MONSTER_POSE_SPECIAL_WINDUP2 )
				{
					if ( multiplayer != CLIENT && my->monsterAnimationLimbOvershoot >= ANIMATE_OVERSHOOT_TO_SETPOINT )
					{
						// handle z movement on windup
						limbAnimateWithOvershoot(my, ANIMATE_Z, 0.05, -0.6, 0.1, -2.0, ANIMATE_DIR_POSITIVE); // default z is -1.2
						if ( my->z > -0.5 )
						{
							my->z = -0.6; //failsafe for floating too low sometimes?
						}
					}
				}
				else if ( my->monsterAttack == MONSTER_POSE_MAGIC_WINDUP3 )
				{
					if ( multiplayer != CLIENT && my->monsterAnimationLimbOvershoot >= ANIMATE_OVERSHOOT_TO_SETPOINT )
					{
						// handle z movement on windup
						limbAnimateWithOvershoot(my, ANIMATE_Z, 0.3, -0.6, 0.3, -4.0, ANIMATE_DIR_POSITIVE); // default z is -1.2
						if ( my->z > -0.5 )
						{
							my->z = -0.6; //failsafe for floating too low sometimes?
						}
					}
				}
				else
				{
					if ( head != nullptr )
					{
						if ( head->pitch > PI )
						{
							limbAnimateToLimit(head, ANIMATE_PITCH, 0.1, 0, false, 0.0); // return head to a neutral position.
						}
						else if ( head->pitch < PI && head->pitch > 0 )
						{
							limbAnimateToLimit(head, ANIMATE_PITCH, -0.1, 0, false, 0.0); // return head to a neutral position.
						}
					}
				}
			}
			continue;
		}
		entity = (Entity*)node->element;
		entity->x = my->x;
		entity->y = my->y;
		entity->z = my->z;
		if ( bodypart != LICH_HEAD )
		{
			// lich head turns to track player, other limbs will rotate as normal.
			if ( bodypart == LICH_LEFTARM && my->monsterAttack == MONSTER_POSE_MAGIC_WINDUP1 )
			{
				// don't rotate leftarm here during spellcast.
			}
			else
			{
				entity->yaw = my->yaw;
			}
		}
		else
		{

		}
		if ( bodypart == LICH_RIGHTARM )
		{
			// weapon holding arm.
			weaponarm = entity;
			if ( my->monsterAttack == 0 )
			{
				entity->pitch = PI / 8; // default arm pitch when not attacking.
			}
			else
			{
				// vertical chop windup
				if ( my->monsterAttack == MONSTER_POSE_MELEE_WINDUP1 )
				{
					if ( my->monsterAttackTime == 0 )
					{
						// init rotations
						my->monsterWeaponYaw = 0;
						weaponarm->roll = 0;
						weaponarm->skill[1] = 0;
					}

					limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.3, 5 * PI / 4, false, 0.0);

					if ( my->monsterAttackTime >= ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
					{
						if ( multiplayer != CLIENT )
						{
							my->attack(1, 0, nullptr);
						}
					}
				}
				// vertical chop attack
				else if ( my->monsterAttack == 1 )
				{
					if ( weaponarm->skill[1] == 0 )
					{
						// chop forwards
						if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, 0.4, PI / 2, false, 0.0) )
						{
							weaponarm->skill[1] = 1;
						}
					}
					else if ( weaponarm->skill[1] == 1 )
					{
						if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.4, PI / 8, false, 0.0) )
						{
							my->monsterWeaponYaw = 0;
							weaponarm->pitch = PI / 8;
							weaponarm->roll = 0;
							my->monsterAttack = 0;
						}
					}
				}
				// horizontal chop windup
				else if ( my->monsterAttack == MONSTER_POSE_MELEE_WINDUP2 )
				{
					if ( my->monsterAttackTime == 0 )
					{
						// init rotations
						weaponarm->pitch = PI / 4;
						weaponarm->roll = 0;
						my->monsterArmbended = 1; // don't actually bend the arm, we're just using this to adjust the limb offsets in the weapon code.
						weaponarm->skill[1] = 0;
						my->monsterWeaponYaw = 6 * PI / 4;
					}

					limbAnimateToLimit(weaponarm, ANIMATE_ROLL, -0.2, 3 * PI / 2, false, 0.0);
					limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.2, 0, false, 0.0);


					if ( my->monsterAttackTime >= ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
					{
						if ( multiplayer != CLIENT )
						{
							my->attack(2, 0, nullptr);
						}
					}
				}
				// horizontal chop attack
				else if ( my->monsterAttack == 2 )
				{
					if ( weaponarm->skill[1] == 0 )
					{
						// swing
						// this->weaponyaw is OK to change for clients, as server doesn't update it for them.
						if ( limbAnimateToLimit(my, ANIMATE_WEAPON_YAW, 0.3, 2 * PI / 8, false, 0.0) )
						{
							weaponarm->skill[1] = 1;
						}
					}
					else if ( weaponarm->skill[1] == 1 )
					{
						// post-swing return to normal weapon yaw
						if ( limbAnimateToLimit(my, ANIMATE_WEAPON_YAW, -0.5, 0, false, 0.0) )
						{
							// restore pitch and roll after yaw is set
							if ( limbAnimateToLimit(weaponarm, ANIMATE_ROLL, 0.4, 0, false, 0.0)
								&& limbAnimateToLimit(weaponarm, ANIMATE_PITCH, 0.4, PI / 8, false, 0.0) )
							{
								weaponarm->skill[1] = 0;
								my->monsterWeaponYaw = 0;
								weaponarm->pitch = PI / 8;
								weaponarm->roll = 0;
								my->monsterArmbended = 0;
								my->monsterAttack = 0;
								my->monsterAttackTime = 0;
							}
						}
					}
				}
				else if ( my->monsterAttack == MONSTER_POSE_SPECIAL_WINDUP1 )
				{
					if ( my->monsterAttackTime == 0 )
					{
						// init rotations
						my->monsterWeaponYaw = 0;
						weaponarm->roll = 0;
						weaponarm->skill[1] = 0;
						createParticleDot(my);
						if ( multiplayer != CLIENT )
						{
							my->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_TO_SETPOINT;
							// lich can't be paralyzed, use EFF_STUNNED instead.
							myStats->EFFECTS[EFF_STUNNED] = true;
							myStats->EFFECTS_TIMERS[EFF_STUNNED] = 50;
						}
					}

					// only do the following during 2nd + end stage of overshoot animation.
					if ( my->monsterAnimationLimbOvershoot != ANIMATE_OVERSHOOT_TO_SETPOINT )
					{
						limbAnimateToLimit(head, ANIMATE_PITCH, -0.1, 11 * PI / 6, true, 0.05);
						limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.3, 5 * PI / 4, false, 0.0);

						if ( my->monsterAttackTime >= 50 / (monsterGlobalAnimationMultiplier / 10.0) )
						{
							if ( multiplayer != CLIENT )
							{
								my->attack(1, 0, nullptr);
								real_t dir = 0.f;
								for ( int i = 0; i < 8; ++i )
								{
									my->castFallingMagicMissile(SPELL_COLD, 16 + rand() % 8, dir + i * PI / 4, 0);
								}
							}
						}
					}
				}
				else if ( my->monsterAttack == MONSTER_POSE_MELEE_WINDUP3 )
				{
					int windupDuration = 40; //(my->monsterState == MONSTER_STATE_LICHFIRE_CASTSPELLS) ? 20 : 40;
					if ( my->monsterAttackTime == 0 )
					{
						// init rotations
						my->monsterWeaponYaw = 10 * PI / 6;
						weaponarm->roll = 0;
						weaponarm->skill[1] = 0;
						createParticleDot(my);
						if ( multiplayer != CLIENT )
						{
							my->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_TO_SETPOINT;
							//	// lich can't be paralyzed, use EFF_STUNNED instead.
							myStats->EFFECTS[EFF_STUNNED] = true;
							myStats->EFFECTS_TIMERS[EFF_STUNNED] = windupDuration;
						}
					}

					// only do the following during 2nd + end stage of overshoot animation.
					if ( my->monsterAnimationLimbOvershoot != ANIMATE_OVERSHOOT_TO_SETPOINT )
					{
						limbAnimateToLimit(head, ANIMATE_PITCH, -0.1, 11 * PI / 6, true, 0.05);
						limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.3, 5 * PI / 4, false, 0.0);

						if ( my->monsterAttackTime >= windupDuration / (monsterGlobalAnimationMultiplier / 10.0) )
						{
							if ( multiplayer != CLIENT )
							{
								my->attack(3, 0, nullptr);
							}
						}
					}
				}
				// vertical chop after melee3
				else if ( my->monsterAttack == 3 )
				{
					if ( weaponarm->skill[1] == 0 )
					{
						// chop forwards
						if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, 0.4, PI / 2, false, 0.0) )
						{
							weaponarm->skill[1] = 1;
						}
						limbAnimateToLimit(my, ANIMATE_WEAPON_YAW, 0.15, 1 * PI / 6, false, 0.0); // swing across the body
					}
					else if ( weaponarm->skill[1] == 1 )
					{
						if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.25, PI / 8, false, 0.0) )
						{
							my->monsterWeaponYaw = 0;
							weaponarm->pitch = PI / 8;
							weaponarm->roll = 0;
							my->monsterAttack = 0;
						}
					}
				}
				else if ( my->monsterAttack == MONSTER_POSE_SPECIAL_WINDUP2 )
				{
					if ( my->monsterAttackTime == 0 )
					{
						// init rotations
						my->monsterWeaponYaw = 0;
						weaponarm->roll = 0;
						weaponarm->skill[1] = 0;
						createParticleDropRising(my, 592, 0.7);
						if ( multiplayer != CLIENT )
						{
							if ( my->monsterState != MONSTER_STATE_LICHFIRE_DIE )
							{
								my->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_TO_SETPOINT;
								// lich can't be paralyzed, use EFF_STUNNED instead.
								myStats->EFFECTS[EFF_STUNNED] = true;
								myStats->EFFECTS_TIMERS[EFF_STUNNED] = 50;
							}
							else
							{
								myStats->EFFECTS[EFF_STUNNED] = true;
								myStats->EFFECTS_TIMERS[EFF_STUNNED] = 25;
							}
						}
					}

					limbAnimateToLimit(head, ANIMATE_PITCH, 0.3, 1 * PI / 6, true, 0.05);
					limbAnimateToLimit(my, ANIMATE_WEAPON_YAW, -0.1, 7 * PI / 4, false, 0.0);

					// only do the following during 2nd + end stage of overshoot animation.
					if ( my->monsterAnimationLimbOvershoot != ANIMATE_OVERSHOOT_TO_SETPOINT )
					{
						limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.3, 5 * PI / 4, false, 0.0);
						if ( my->monsterAttackTime >= 50 / (monsterGlobalAnimationMultiplier / 10.0) )
						{
							if ( multiplayer != CLIENT )
							{
								if ( my->monsterState != MONSTER_STATE_LICHICE_DIE )
								{
									my->attack(1, 0, nullptr);
								}
								else
								{
									my->monsterAttackTime = 25; //reset this attack time to allow successive strikes
								}

								if ( my->monsterState == MONSTER_STATE_LICHICE_DIE )
								{
									int spellID = SPELL_DRAIN_SOUL;
									for ( int i = 0; i < 8; ++i )
									{
										Entity* spell = castSpell(my->getUID(), getSpellFromID(spellID), true, false);
										// do some minor variations in spell angle
										spell->yaw += i * PI / 4 + ((PI * (-4 + rand() % 9)) / 64);
										spell->vel_x = 4 * cos(spell->yaw);
										spell->vel_y = 4 * sin(spell->yaw);
										spell->skill[5] = 50; // travel time
									}
								}
								else
								{
									int spellID = SPELL_COLD;
									if ( rand() % 5 == 0 || (my->monsterLichAllyStatus == LICH_ALLY_DEAD && rand() % 2 == 0) )
									{
										spellID = SPELL_DRAIN_SOUL;
									}
									for ( int i = 0; i < 8; ++i )
									{
										Entity* spell = castSpell(my->getUID(), getSpellFromID(spellID), true, false);
										// do some minor variations in spell angle
										spell->yaw += i * PI / 4 + ((PI * (-4 + rand() % 9)) / 64);
										spell->vel_x = 4 * cos(spell->yaw);
										spell->vel_y = 4 * sin(spell->yaw);
										spell->skill[5] = 50; // travel time
									}
								}
							}
						}
					}
				}
				else if ( my->monsterAttack == MONSTER_POSE_SPECIAL_WINDUP3 )
				{
					if ( my->monsterAttackTime == 0 )
					{
						// init rotations
						my->monsterWeaponYaw = 0;
						weaponarm->roll = 0;
						weaponarm->skill[1] = 0;
						createParticleDropRising(my, 678, 1.0);
						if ( multiplayer != CLIENT )
						{
							my->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_TO_SETPOINT;
							// lich can't be paralyzed, use EFF_STUNNED instead.
							myStats->EFFECTS[EFF_STUNNED] = true;
							myStats->EFFECTS_TIMERS[EFF_STUNNED] = 50;
						}
					}

					limbAnimateToLimit(head, ANIMATE_PITCH, -0.3, 11 * PI / 6, false, 0.0);
					limbAnimateToLimit(my, ANIMATE_WEAPON_YAW, 0.1, 1 * PI / 4, false, 0.0);

					// only do the following during 2nd + end stage of overshoot animation.
					if ( my->monsterAnimationLimbOvershoot != ANIMATE_OVERSHOOT_TO_SETPOINT )
					{
						limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.3, 5 * PI / 4, false, 0.0);
						if ( my->monsterAttackTime >= 50 / (monsterGlobalAnimationMultiplier / 10.0) )
						{
							if ( multiplayer != CLIENT )
							{
								my->attack(1, 0, nullptr);
								for ( int i = 0; i < 3; ++i )
								{
									Entity* spell = castSpell(my->getUID(), getSpellFromID(SPELL_MAGICMISSILE), true, false);
									real_t horizontalSpeed = 3.0;
									if ( i != 0 )
									{
										// do some minor variations in spell angle
										spell->yaw += ((PI * (-4 + rand() % 9)) / 40);
									}
									Entity* target = uidToEntity(my->monsterTarget);
									if ( target )
									{
										real_t spellDistance = sqrt(pow(spell->x - target->x, 2) + pow(spell->y - target->y, 2));
										spell->vel_z = 22.0 / (spellDistance / horizontalSpeed);
									}
									else
									{
										spell->vel_z = 2.0 - (i * 0.8);
									}
									spell->vel_x = horizontalSpeed * cos(spell->yaw);
									spell->vel_y = horizontalSpeed * sin(spell->yaw);
									spell->actmagicIsVertical = MAGIC_ISVERTICAL_XYZ;
									spell->z = -22.0;
									spell->pitch = atan2(spell->vel_z, horizontalSpeed);
								}
							}
						}
					}
				}
				else if ( my->monsterAttack == MONSTER_POSE_MAGIC_WINDUP3 )
				{
					if ( my->monsterAttackTime == 0 )
					{
						// init rotations
						my->monsterWeaponYaw = 0;
						weaponarm->roll = 0;
						weaponarm->skill[1] = 0;
						createParticleDropRising(my, 174, 0.5);
						if ( multiplayer != CLIENT )
						{
							my->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_TO_SETPOINT;
							// lich can't be paralyzed, use EFF_STUNNED instead.
							myStats->EFFECTS[EFF_STUNNED] = true;
							myStats->EFFECTS_TIMERS[EFF_STUNNED] = 80;
						}
					}
					else if ( my->monsterAttackTime % 20 == 0 )
					{
						createParticleDropRising(my, 174, 0.5);
					}

					limbAnimateToLimit(head, ANIMATE_PITCH, -0.05, 10 * PI / 6, true, 0.05);
					limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.1, 5 * PI / 4, false, 0.0);
					limbAnimateToLimit(my, ANIMATE_WEAPON_YAW, 0.1, PI / 4, false, 0.0);

					if ( my->monsterAttackTime >= 50 / (monsterGlobalAnimationMultiplier / 10.0) )
					{
						if ( multiplayer != CLIENT )
						{
							my->attack(1, 0, nullptr);
						}
					}
				}
			}
		}
		else if ( bodypart == LICH_LEFTARM )
		{
			spellarm = entity;
			if ( my->monsterAttack == MONSTER_POSE_SPECIAL_WINDUP1 
				|| my->monsterAttack == MONSTER_POSE_MELEE_WINDUP3
				|| my->monsterAttack == MONSTER_POSE_SPECIAL_WINDUP2
				|| my->monsterAttack == MONSTER_POSE_SPECIAL_WINDUP3
				|| my->monsterAttack == MONSTER_POSE_MAGIC_WINDUP3 )
			{
				spellarm->pitch = weaponarm->pitch;
			}
			else if ( my->monsterAttack == MONSTER_POSE_MAGIC_WINDUP1 )
			{
				if ( my->monsterAttackTime == 0 )
				{
					// init rotations
					spellarm->roll = 0;
					spellarm->skill[1] = 0;
					spellarm->pitch = 12 * PI / 8;
					spellarm->yaw = my->yaw;
					createParticleDot(my);
					playSoundEntityLocal(my, 170, 32);
					if ( multiplayer != CLIENT )
					{
						myStats->EFFECTS[EFF_STUNNED] = true;
						myStats->EFFECTS_TIMERS[EFF_STUNNED] = 20;
					}
				}
				double animationYawSetpoint = normaliseAngle2PI(my->yaw + 1 * PI / 8);
				double animationYawEndpoint = normaliseAngle2PI(my->yaw - 1 * PI / 8);
				double armSwingRate = 0.15;
				double animationPitchSetpoint = 13 * PI / 8;
				double animationPitchEndpoint = 11 * PI / 8;

				if ( spellarm->skill[1] == 0 )
				{
					if ( limbAnimateToLimit(spellarm, ANIMATE_PITCH, armSwingRate, animationPitchSetpoint, false, 0.0) )
					{
						if ( limbAnimateToLimit(spellarm, ANIMATE_YAW, armSwingRate, animationYawSetpoint, false, 0.0) )
						{
							spellarm->skill[1] = 1;
						}
					}
				}
				else
				{
					if ( limbAnimateToLimit(spellarm, ANIMATE_PITCH, -armSwingRate, animationPitchEndpoint, false, 0.0) )
					{
						if ( limbAnimateToLimit(spellarm, ANIMATE_YAW, -armSwingRate, animationYawEndpoint, false, 0.0) )
						{
							spellarm->skill[1] = 0;
						}
					}
				}

				if ( my->monsterAttackTime >= 1 * ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
				{
					if ( multiplayer != CLIENT )
					{
						// swing the arm after we prepped the spell
						my->attack(MONSTER_POSE_MAGIC_WINDUP2, 0, nullptr);
					}
				}
			}
			// raise arm to cast spell
			else if ( my->monsterAttack == MONSTER_POSE_MAGIC_WINDUP2 )
			{
				if ( my->monsterAttackTime == 0 )
				{
					// init rotations
					spellarm->pitch = 0;
					spellarm->roll = 0;
				}
				spellarm->skill[1] = 0;

				if ( limbAnimateToLimit(spellarm, ANIMATE_PITCH, -0.3, 5 * PI / 4, false, 0.0) )
				{
					if ( multiplayer != CLIENT )
					{
						my->attack(MONSTER_POSE_MAGIC_CAST1, 0, nullptr);
					}
				}
			}
			// vertical spell attack
			else if ( my->monsterAttack == MONSTER_POSE_MAGIC_CAST1 )
			{
				if ( spellarm->skill[1] == 0 )
				{
					// chop forwards
					if ( limbAnimateToLimit(spellarm, ANIMATE_PITCH, 0.4, PI / 2, false, 0.0) )
					{
						spellarm->skill[1] = 1;
						if ( multiplayer != CLIENT )
						{
							if ( rand() % 5 == 0 )
							{
								castSpell(my->getUID(), getSpellFromID(SPELL_SLOW), true, false);
							}
							else
							{
								castSpell(my->getUID(), getSpellFromID(SPELL_DRAIN_SOUL), true, false);
							}
						}
					}
				}
				else if ( spellarm->skill[1] == 1 )
				{
					if ( limbAnimateToLimit(spellarm, ANIMATE_PITCH, -0.25, PI / 8, false, 0.0) )
					{
						spellarm->pitch = 0;
						spellarm->roll = 0;
						my->monsterAttack = 0;
					}
				}
			}
			else
			{
				entity->pitch = 0;
			}
		}
		switch ( bodypart )
		{
			// right arm
			case LICH_RIGHTARM:
				entity->x += 2.75 * cos(my->yaw + PI / 2);
				entity->y += 2.75 * sin(my->yaw + PI / 2);
				entity->z -= 3.25;
				entity->yaw += MONSTER_WEAPONYAW;
				break;
				// left arm
			case LICH_LEFTARM:
				entity->x -= 2.75 * cos(my->yaw + PI / 2);
				entity->y -= 2.75 * sin(my->yaw + PI / 2);
				entity->z -= 3.25;
				if ( !(my->monsterAttack == MONSTER_POSE_MELEE_WINDUP2
					|| my->monsterAttack == 2
					|| my->monsterAttack == MONSTER_POSE_MAGIC_WINDUP1
					|| my->monsterAttack == 3)
					)
				{
					entity->yaw -= MONSTER_WEAPONYAW;
				}
				break;
				// head
			case LICH_HEAD:
			{
				entity->z -= 4.25;
				node_t* tempNode;
				Entity* playertotrack = NULL;
				double disttoplayer = 0.0;
				Entity* target = uidToEntity(my->monsterTarget);
				if ( target && my->monsterAttack == 0 )
				{
					entity->lookAtEntity(*target);
					entity->monsterRotate();
				}
				else
				{
					// align head as normal if attacking.
					entity->yaw = my->yaw;
				}
				break;
			}
			case LICH_WEAPON:
				// set sprites, invisibility check etc.
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

				// animation
				if ( entity != nullptr )
				{
					if ( weaponarm == nullptr )
					{
						return;
					}
					entity->x = weaponarm->x;// +1.5 * cos(weaponarm->yaw);// *(my->monsterAttack == 0);
					entity->y = weaponarm->y;// +1.5 * sin(weaponarm->yaw);// * (my->monsterAttack == 0);
					entity->z = weaponarm->z;// -.5 * (my->monsterAttack == 0);
					entity->pitch = weaponarm->pitch;
					entity->yaw = weaponarm->yaw + 0.1 * (my->monsterAttack == 0);
					entity->roll = weaponarm->roll;
					if ( my->monsterAttack == 2 || my->monsterAttack == MONSTER_POSE_MELEE_WINDUP2 )
					{
						// don't boost pitch during side-swipe
					}
					else
					{
						entity->pitch += PI / 2 + 0.25;
					}

					entity->focalx = limbs[LICH_ICE][4][0];
					entity->focaly = limbs[LICH_ICE][4][1];
					entity->focalz = limbs[LICH_ICE][4][2];
					if ( my->monsterArmbended )
					{
						// adjust focal points during side swing
						entity->focalx = limbs[LICH_ICE][4][0] - 0.8;
						entity->focalz = limbs[LICH_ICE][4][2] + 1;
						entity->pitch += cos(weaponarm->roll) * PI / 2;
						entity->yaw -= sin(weaponarm->roll) * PI / 2;
					}
				}
				break;
			default:
				break;
		}
	}
	if ( my->monsterAttack > 0 && my->monsterAttack <= MONSTER_POSE_MAGIC_CAST3 )
	{
		my->monsterAttackTime++;
	}
	else if ( my->monsterAttack == 0 )
	{
		my->monsterAttackTime = 0;
	}
	else
	{
		// do nothing, don't reset attacktime or increment it.
	}
}

void Entity::lichIceSetNextAttack(Stat& myStats)
{
	monsterLichIceCastPrev = monsterLichIceCastSeq;
	//messagePlayer(0, "melee: %d, magic %d", monsterLichMeleeSwingCount, monsterLichMagicCastCount);
	switch ( monsterLichIceCastSeq )
	{
		case LICH_ATK_VERTICAL_SINGLE:
			if ( monsterSpecialState == 0 && monsterState != MONSTER_STATE_LICH_CASTSPELLS
				&& monsterSpecialTimer == 0 && rand() % 4 > 0 )
			{
				monsterLichMeleeSwingCount = 0;
				monsterSpecialState = LICH_ICE_ATTACK_COMBO;
				//createParticleDot(this);
			}
			++monsterLichMeleeSwingCount;
			switch ( rand() % 3 )
			{
				case 0:
					monsterLichIceCastSeq = LICH_ATK_VERTICAL_SINGLE;
					break;
				case 1:
					monsterLichIceCastSeq = LICH_ATK_HORIZONTAL_SINGLE;
					break;
				case 2:
					if ( monsterSpecialState == LICH_ICE_ATTACK_COMBO )
					{
						monsterLichIceCastSeq = LICH_ATK_FALLING_DIAGONAL;
						//monsterSpecialState = 0;
						//monsterSpecialTimer = 100;
					}
					else
					{
						monsterLichIceCastSeq = LICH_ATK_VERTICAL_SINGLE;
					}
					break;
				case 3:
					//monsterLichIceCastSeq = LICH_ATK_FALLING_DIAGONAL;
					//monsterLichIceCastSeq = LICH_ATK_RISING_RAIN;
					//monsterLichIceCastSeq = LICH_ATK_CHARGE_AOE;
					break;
				default:
					break;
			}
			break;
		case LICH_ATK_HORIZONTAL_SINGLE:
			if ( monsterSpecialState == 0
				&& monsterSpecialTimer == 0 && rand() % 4 > 0 )
			{
				monsterLichMeleeSwingCount = 0;
				monsterSpecialState = LICH_ICE_ATTACK_COMBO;
				//createParticleDot(this);
			}
			++monsterLichMeleeSwingCount;
			switch ( rand() % 3 )
			{
				case 0:
					monsterLichIceCastSeq = LICH_ATK_VERTICAL_SINGLE;
					break;
				case 1:
					monsterLichIceCastSeq = LICH_ATK_HORIZONTAL_SINGLE;
					break;
				case 2:
					if ( monsterSpecialState == LICH_ICE_ATTACK_COMBO )
					{
						monsterLichIceCastSeq = LICH_ATK_FALLING_DIAGONAL;
						//monsterSpecialState = 0;
						//monsterSpecialTimer = 100;
					}
					else
					{
						monsterLichIceCastSeq = LICH_ATK_BASICSPELL_SINGLE;
					}
					break;
				default:
					break;
			}
			break;
		case LICH_ATK_RISING_RAIN:
			monsterLichMeleeSwingCount = 0;
			switch ( rand() % 4 )
			{
				case 0:
				case 1:
				case 2:
					monsterLichFireMeleeSeq = LICH_ATK_VERTICAL_SINGLE;
					break;
				case 3:
					monsterLichFireMeleeSeq = LICH_ATK_HORIZONTAL_SINGLE;
					break;
				default:
					break;
			}
			break;
		case LICH_ATK_BASICSPELL_SINGLE:
			++monsterLichMagicCastCount;
			if ( monsterLichMagicCastCount > 2 || rand() % 2 == 0 )
			{
				monsterLichIceCastSeq = LICH_ATK_VERTICAL_SINGLE;
				monsterLichMagicCastCount = 0;
			}
			break;
		case LICH_ATK_CHARGE_AOE:
			//monsterLichMeleeSwingCount = 0;
			switch ( rand() % 2 )
			{
				case 0:
					monsterLichIceCastSeq = LICH_ATK_VERTICAL_SINGLE;
					break;
				case 1:
					monsterLichIceCastSeq = LICH_ATK_HORIZONTAL_SINGLE;
					break;
				default:
					break;
			}
			break;
		case LICH_ATK_FALLING_DIAGONAL:
			switch ( rand() % 2 )
			{
				case 0:
					monsterLichIceCastSeq = LICH_ATK_VERTICAL_SINGLE;
					break;
				case 1:
					monsterLichIceCastSeq = LICH_ATK_HORIZONTAL_SINGLE;
					break;
				default:
					break;
			}
			break;
		case LICH_ATK_SUMMON:
			switch ( rand() % 2 )
			{
				case 0:
					monsterLichIceCastSeq = LICH_ATK_VERTICAL_SINGLE;
					break;
				case 1:
					monsterLichIceCastSeq = LICH_ATK_HORIZONTAL_SINGLE;
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
}

void Entity::lichIceTeleport()
{
	monsterLichTeleportTimer = 0;
	Entity* spellTimer = createParticleTimer(this, 40, 593);
	if ( monsterState == MONSTER_STATE_LICHICE_TELEPORT_STATIONARY )
	{
		spellTimer->particleTimerEndAction = PARTICLE_EFFECT_LICHICE_TELEPORT_STATIONARY; // teleport behavior of timer.
	}
	else
	{
		spellTimer->particleTimerEndAction = PARTICLE_EFFECT_LICH_TELEPORT_ROAMING; // teleport behavior of timer.
	}
	spellTimer->particleTimerEndSprite = 593; // sprite to use for end of timer function.
	spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_SHOOT_PARTICLES;
	spellTimer->particleTimerCountdownSprite = 593;
	if ( multiplayer == SERVER )
	{
		serverSpawnMiscParticles(this, spellTimer->particleTimerEndAction, 593);
	}
}

void Entity::lichIceCreateCannon()
{
	//spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_SHOOT_PARTICLES;
	for ( int i = 0; i < 6; ++i )
	{
		Entity* spellOrbit = castOrbitingMagicMissile(SPELL_MAGICMISSILE, 8.0, i * PI / 3, 500);
		spellOrbit->z = -25;
		spellOrbit->actmagicOrbitStartZ = spellOrbit->z;
	}
	createParticleDropRising(this, 678, 1.0);
	if ( multiplayer == SERVER )
	{
		serverSpawnMiscParticles(this, PARTICLE_EFFECT_RISING_DROP, 678);
	}
}

Entity* Entity::lichThrowProjectile(real_t angle)
{
	Entity* projectile = newEntity(items[STEEL_CHAKRAM].index, 1, map.entities, nullptr); // thrown item
	projectile->parent = uid;
	projectile->x = x;
	projectile->y = y;
	projectile->z = z;
	projectile->yaw = yaw + angle;
	projectile->sizex = 1;
	projectile->sizey = 1;
	projectile->behavior = &actThrown;
	projectile->flags[UPDATENEEDED] = true;
	projectile->flags[PASSABLE] = true;
	projectile->skill[10] = STEEL_CHAKRAM;
	projectile->skill[11] = EXCELLENT;
	projectile->skill[12] = 0;
	projectile->skill[13] = 1;
	projectile->skill[14] = 0;
	projectile->skill[15] = 1;

	// todo: change velocity of chakram/shuriken?
	projectile->vel_x = 6 * cos(yaw + angle);
	projectile->vel_y = 6 * sin(yaw + angle);
	projectile->vel_z = -.3;

	return projectile;
}

void Entity::lichIceSummonMonster(Monster creature)
{
	Entity* target = nullptr;
	for ( node_t* searchNode = map.entities->first; searchNode != nullptr; searchNode = searchNode->next )
	{
		target = (Entity*)searchNode->element;
		if ( target->behavior == &actDevilTeleport
			&& target->sprite == 128 )
		{
			break; // found specified center of map
		}
	}
	if ( target )
	{
		int tries = 25; // max iteration in while loop, fail safe.
		long spawn_x = (target->x / 16) - 11 + rand() % 23;
		long spawn_y = (target->y / 16) - 11 + rand() % 23;
		int index = (spawn_x)* MAPLAYERS + (spawn_y)* MAPLAYERS * map.height;
		while ( tries > 0 &&
				(map.tiles[OBSTACLELAYER + index] == 1
				|| map.tiles[index] == 0
				|| swimmingtiles[map.tiles[index]]
				|| lavatiles[map.tiles[index]])
			)
		{
			// find a spot that isn't wall, no floor or lava/water tiles.
			spawn_x = (target->x / 16) - 11 + rand() % 23;
			spawn_y = (target->y / 16) - 11 + rand() % 23;
			index = (spawn_x)* MAPLAYERS + (spawn_y)* MAPLAYERS * map.height;
			--tries;
		}
		if ( tries > 0 )
		{
			Entity* timer = createParticleTimer(this, 70, 174);
			timer->x = spawn_x * 16.0 + 8;
			timer->y = spawn_y * 16.0 + 8;
			timer->z = 0;
			timer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_SUMMON_MONSTER;
			timer->particleTimerCountdownSprite = 174;
			timer->particleTimerEndAction = PARTICLE_EFFECT_SUMMON_MONSTER;
			timer->particleTimerVariable1 = creature;
			serverSpawnMiscParticlesAtLocation(spawn_x, spawn_y, 0, PARTICLE_EFFECT_SUMMON_MONSTER, 174);
		}
	}
}