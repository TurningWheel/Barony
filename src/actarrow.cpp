/*-------------------------------------------------------------------------------

	BARONY
	File: actarrow.cpp
	Desc: behavior function for arrows/bolts

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "monster.hpp"
#include "sound.hpp"
#include "interface/interface.hpp"
#include "net.hpp"
#include "collision.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

#define ARROW_STUCK my->skill[0]
#define ARROW_LIFE my->skill[1]
#define ARROW_VELX my->vel_x
#define ARROW_VELY my->vel_y
#define ARROW_VELZ my->vel_z
#define ARROW_OLDX my->fskill[2]
#define ARROW_OLDY my->fskill[3]
#define ARROW_MAXLIFE 600

void actArrow(Entity* my)
{
	double dist;
	int damage;
	Entity* entity;
	node_t* node;
	double tangent;

	my->skill[2] = -7; // invokes actEmpty() on clients

	if ( my->arrowPower == 0 )
	{
		my->arrowPower = 10 + (my->sprite == 167);
	}

	// lifespan
	ARROW_LIFE++;
	if ( ARROW_LIFE >= ARROW_MAXLIFE )
	{
		list_RemoveNode(my->mynode);
		return;
	}

	if ( !ARROW_STUCK )
	{
		// horizontal motion
		ARROW_VELX = cos(my->yaw) * 7;
		ARROW_VELY = sin(my->yaw) * 7;

		ARROW_OLDX = my->x;
		ARROW_OLDY = my->y;
		dist = clipMove(&my->x, &my->y, ARROW_VELX, ARROW_VELY, my);

		// damage monsters
		if ( dist != sqrt(ARROW_VELX * ARROW_VELX + ARROW_VELY * ARROW_VELY) )
		{
			ARROW_STUCK = 1;
			my->x = ARROW_OLDX;
			my->y = ARROW_OLDY;
			Entity* oentity = hit.entity;
			lineTrace(my, my->x, my->y, my->yaw, sqrt(ARROW_VELX * ARROW_VELX + ARROW_VELY * ARROW_VELY), 0, false);
			hit.entity = oentity;
			my->x = hit.x - cos(my->yaw);
			my->y = hit.y - sin(my->yaw);
			ARROW_VELX = 0;
			ARROW_VELY = 0;
			ARROW_VELZ = 0;
			if ( hit.entity != NULL )
			{
				Entity* parent = uidToEntity(my->parent);
				Stat* hitstats = hit.entity->getStats();
				playSoundEntity(my, 72 + rand() % 3, 64);
				if ( hitstats != NULL && hit.entity != parent )
				{
					if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
					{
						// test for friendly fire
						if ( parent && parent->checkFriend(hit.entity) )
						{
							list_RemoveNode(my->mynode);
							return;
						}
					}

					// do damage
					if ( my->arrowArmorPierce > 0 && AC(hitstats) > 0 )
					{
						if ( hit.entity->behavior == &actPlayer && hitstats && !hitstats->defending )
						{
							damage = std::max(my->arrowPower - (AC(hitstats) / 2), 0); // pierce half armor.
						}
						else
						{
							damage = std::max(my->arrowPower - AC(hitstats), 0); // normal damage.
						}
					}
					else
					{
						damage = std::max(my->arrowPower - AC(hitstats), 0); // normal damage.
					}
					damage *= damagetables[hitstats->type][4];
					//messagePlayer(0, "My damage: %d, AC: %d, Pierce: %d", my->arrowPower, AC(hitstats), my->arrowArmorPierce);
					//messagePlayer(0, "Resolved to %d damage.", damage);
					hit.entity->modHP(-damage);

					// write obituary
					if ( parent )
					{
						if ( parent->behavior == &actArrowTrap )
						{
							hit.entity->setObituary(language[1503]);
						}
						else
						{
							parent->killedByMonsterObituary(hit.entity);
						}
					}

					if ( damage > 0 )
					{
						Entity* gib = spawnGib(hit.entity);
						serverSpawnGibForClient(gib);
						playSoundEntity(hit.entity, 28, 64);
						if ( hit.entity->behavior == &actPlayer )
						{
							if ( hit.entity->skill[2] == clientnum )
							{
								camera_shakex += .1;
								camera_shakey += 10;
							}
							else
							{
								strcpy((char*)net_packet->data, "SHAK");
								net_packet->data[4] = 10; // turns into .1
								net_packet->data[5] = 10;
								net_packet->address.host = net_clients[hit.entity->skill[2] - 1].host;
								net_packet->address.port = net_clients[hit.entity->skill[2] - 1].port;
								net_packet->len = 6;
								sendPacketSafe(net_sock, -1, net_packet, hit.entity->skill[2] - 1);
							}
						}
						if ( rand() % 10 == 0 && parent )
						{
							parent->increaseSkill(PRO_RANGED);
						}
					}
					else
					{
						playSoundEntity(hit.entity, 66, 64); //*tink*
						if ( hit.entity->behavior == &actPlayer )
						{
							if ( hit.entity->skill[2] == clientnum )
							{
								camera_shakex += .05;
								camera_shakey += 5;
							}
							else
							{
								strcpy((char*)net_packet->data, "SHAK");
								net_packet->data[4] = 5; // turns into .05
								net_packet->data[5] = 5;
								net_packet->address.host = net_clients[hit.entity->skill[2] - 1].host;
								net_packet->address.port = net_clients[hit.entity->skill[2] - 1].port;
								net_packet->len = 6;
								sendPacketSafe(net_sock, -1, net_packet, hit.entity->skill[2] - 1);
							}
						}
					}

					if ( hitstats->HP <= 0 && parent)
					{
						parent->awardXP( hit.entity, true, true );
					}

					// alert the monster
					if ( hit.entity->behavior == &actMonster && parent != nullptr )
					{
						if ( hit.entity->skill[0] != 1 && (hitstats->type < LICH || hitstats->type >= SHOPKEEPER) )
						{
							/*hit.entity->monsterState = MONSTER_STATE_PATH;
							hit.entity->monsterTarget = parent->getUID();
							hit.entity->monsterTargetX = parent->x;
							hit.entity->monsterTargetY = parent->y;*/

							hit.entity->monsterAcquireAttackTarget(*parent, MONSTER_STATE_PATH);
						}

						// alert other monsters too
						Entity* ohitentity = hit.entity;
						for ( node = map.creatures->first; node != nullptr; node = node->next )
						{
							entity = (Entity*)node->element;
							if ( entity && entity->behavior == &actMonster && entity != ohitentity )
							{
								Stat* buddystats = entity->getStats();
								if ( buddystats != nullptr )
								{
									if ( entity->checkFriend(hit.entity) )
									{
										if ( entity->monsterState == MONSTER_STATE_WAIT ) // monster is waiting
										{
											tangent = atan2( entity->y - ohitentity->y, entity->x - ohitentity->x );
											lineTrace(ohitentity, ohitentity->x, ohitentity->y, tangent, 1024, 0, false);
											if ( hit.entity == entity )
											{
												/*entity->monsterState = MONSTER_STATE_PATH;
												entity->monsterTarget = parent->getUID();
												entity->monsterTargetX = parent->x;
												entity->monsterTargetY = parent->y;*/

												entity->monsterAcquireAttackTarget(*parent, MONSTER_STATE_PATH);
											}
										}
									}
								}
							}
						}
						hit.entity = ohitentity;
						if ( parent->behavior == &actPlayer )
						{
							if ( !strcmp(hitstats->name, "") )
							{
								Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
								messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[446], language[446], MSG_COMBAT);
								if ( damage == 0 )
								{
									messagePlayer(parent->skill[2], language[447]);
								}
								else if ( my->arrowArmorPierce > 0 && AC(hitstats) > 0 )
								{
									messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[2513], language[2513], MSG_COMBAT);
								}
							}
							else
							{
								Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
								messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[446], language[448], MSG_COMBAT);
								if ( damage == 0 )
								{
									if ( hitstats->sex )
									{
										messagePlayer(parent->skill[2], language[449]);
									}
									else
									{
										messagePlayer(parent->skill[2], language[450]);
									}
								}
								else if ( my->arrowArmorPierce > 0 && AC(hitstats) > 0 )
								{
									messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[2514], language[2514], MSG_COMBAT);
								}
							}
						}
					}
					else if ( hit.entity->behavior == &actPlayer )
					{
						Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
						if ( my )
						{
							if ( my->sprite == 78 )
							{
								// rock.
								messagePlayerColor(hit.entity->skill[2], color, language[2512]);
							}
							else if (my->sprite == 167)
							{
								// bolt.
								messagePlayerColor(hit.entity->skill[2], color, language[2511]);
							}
							else
							{
								// arrow.
								messagePlayerColor(hit.entity->skill[2], color, language[451]);
							}
						}
						else
						{
							messagePlayerColor(hit.entity->skill[2], color, language[451]);
						}
						if ( damage == 0 )
						{
							messagePlayer(hit.entity->skill[2], language[452]);
						}
						else if (my->arrowArmorPierce > 0 && AC(hitstats) > 0)
						{
							messagePlayerColor(hit.entity->skill[2], color, language[2515]);
						}
					}

					if ( my->arrowPoisonTime > 0 && damage > 0 )
					{
						hitstats->poisonKiller = my->parent;
						hitstats->EFFECTS[EFF_POISONED] = true;
						hitstats->EFFECTS_TIMERS[EFF_POISONED] = my->arrowPoisonTime;
						if ( hit.entity->behavior == &actPlayer )
						{
							Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
							messagePlayerColor(hit.entity->skill[2], color, language[453]);
							serverUpdateEffects(hit.entity->skill[2]);
						}
					}

					// update enemy bar for attacker
					if ( !strcmp(hitstats->name, "") )
					{
						if ( hitstats->type < KOBOLD ) //Original monster count
						{
							updateEnemyBar(parent, hit.entity, language[90 + hitstats->type], hitstats->HP, hitstats->MAXHP);
						}
						else if ( hitstats->type >= KOBOLD ) //New monsters
						{
							updateEnemyBar(parent, hit.entity, language[2000 + (hitstats->type - KOBOLD)], hitstats->HP, hitstats->MAXHP);
						}

					}
					else
					{
						updateEnemyBar(parent, hit.entity, hitstats->name, hitstats->HP, hitstats->MAXHP);
					}

				}
				list_RemoveNode(my->mynode);
			}
			else if ( my->sprite == 78 )
			{
				list_RemoveNode(my->mynode); // rocks don't stick to walls...
			}
			else
			{
				playSoundEntity(my, 72 + rand() % 3, 64);
			}
		}
	}
}
