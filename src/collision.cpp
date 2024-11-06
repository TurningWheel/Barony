/*-------------------------------------------------------------------------------

	BARONY
	File: collision.cpp
	Desc: contains all collision detection code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "messages.hpp"
#include "entity.hpp"
#include "engine/audio/sound.hpp"
#include "items.hpp"
#include "interface/interface.hpp"
#include "magic/magic.hpp"
#include "net.hpp"
#include "paths.hpp"
#include "collision.hpp"
#include "prng.hpp"
#include "player.hpp"
#include "mod_tools.hpp"
#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif
#include "ui/MainMenu.hpp"
#include "interface/consolecommand.hpp"
#include "ui/GameUI.hpp"

/*-------------------------------------------------------------------------------

	entityDist

	returns the distance between the two given entities

-------------------------------------------------------------------------------*/

real_t entityDist(Entity* my, Entity* your)
{
	real_t dx, dy;
	dx = my->x - your->x;
	dy = my->y - your->y;
	return sqrt(dx * dx + dy * dy);
}

/*-------------------------------------------------------------------------------

	entityClicked

	returns the entity that was last clicked on with the mouse

-------------------------------------------------------------------------------*/

Entity* entityClicked(bool* clickedOnGUI, bool clickCheckOverride, int player, EntityClickType clicktype)
{
	Uint32 uidnum;
	GLubyte pixel[4];

	Input& input = Input::inputs[player];

	Entity* playerEntity = Player::getPlayerInteractEntity(player);

	if ( gamePaused || movie || !players[player] || !playerEntity
		|| playerEntity->ticks < (TICKS_PER_SECOND / 2)
		|| fadeout
		|| (players[player]->usingCommand() && input.input("Use").type == Input::binding_t::KEYBOARD) )
	{
		input.consumeBinaryToggle("Use");
		return nullptr;
	}
	if ( clicktype == ENTITY_CLICK_HELD_USE_TOOLTIPS_ONLY )
	{
		if ( !clickCheckOverride && !input.binaryToggle("Use") )
		{
			return NULL;
		}
	}
	else
	{
		if ( !clickCheckOverride && !input.binaryToggle("Use") )
		{
			return NULL;
		}
	}

	Sint32 mx = inputs.getMouse(player, Inputs::OX);
	Sint32 my = inputs.getMouse(player, Inputs::OY);
	auto& inventoryUI = players[player]->inventoryUI;

	auto& camera = cameras[player];

	if ( !players[player]->shootmode )
	{
		if ( !framesProcResult.usable && *framesEatMouse )
		{
			if ( clickedOnGUI )
			{
				*clickedOnGUI = true;
			}
			return NULL;
		}

		if ( inputs.getUIInteraction(player)->itemMenuOpen )
		{
			if ( clickedOnGUI )
			{
				*clickedOnGUI = true;
			}
			return NULL;
		}
		if ( mx < camera.winx || mx >= camera.winx + camera.winw || my < camera.winy || my >= camera.winy + camera.winh )
		{
			if ( clickedOnGUI )
			{
				*clickedOnGUI = true;
			}
			return NULL;
		}

		/*if ( mouseInsidePlayerInventory(player) || mouseInsidePlayerHotbar(player) )
		{
			if ( clickedOnGUI )
			{
				*clickedOnGUI = true;
			}
			return NULL;
		}*/

		if ( players[player]->worldUI.isEnabled() )
		{
			uidnum = 0;
		}
		else
		{
			uidnum = GO_GetPixelU32(mx, yres - my, cameras[player]);
		}
	}
	else
	{
		if ( players[player]->worldUI.isEnabled() )
		{
			uidnum = 0;
		}
		else
		{
			uidnum = GO_GetPixelU32(cameras[player].winx + (cameras[player].winw / 2), yres - (cameras[player].winy + (cameras[player].winh / 2)), cameras[player]);
		}
		//messagePlayer(0, "first: %d", uidnum);
		//uidnum = GO_GetPixelU32(cameras[player].winx + (cameras[player].winw / 2), (cameras[player].winy + (cameras[player].winh / 2)), cameras[player]);
		//messagePlayer(0, "sec: %d", uidnum);
	}

	Entity* entity = uidToEntity(uidnum);
	if ( players[player]->worldUI.isEnabled() )
	{
		bool waitingForInputHeld = false;
		if ( players[player]->worldUI.tooltipsInRange.size() > 0 )
		{
			for ( node_t* node = map.worldUI->first; node; node = node->next )
			{
				Entity* tooltip = (Entity*)node->element;
				if ( !tooltip || tooltip->behavior != &actSpriteWorldTooltip )
				{
					continue;
				}
				if ( players[player]->worldUI.bTooltipActiveForPlayer(*tooltip) )
				{
					if ( tooltip->worldTooltipRequiresButtonHeld == 1 
						&& *MainMenu::cvar_hold_to_activate
						&& clicktype != ENTITY_CLICK_CALLOUT )
					{
						if ( input.binaryHeldToggle("Use") )
						{
							entity = uidToEntity(tooltip->parent);
						}
						else
						{
							waitingForInputHeld = true;
						}
					}
					else
					{
						entity = uidToEntity(tooltip->parent);
					}
					break;
				}
			}
		}
		if ( !entity )
		{
			if ( !waitingForInputHeld )
			{
				// clear the button input if we missed a tooltip, otherwise it'll keep retrying (or pre-fire a button held)
				input.consumeBinaryToggle("Use");
				//input.consumeBindingsSharedWithBinding("Use");
			}
		}
	}
	else
	{
		if ( playerEntity->behavior == &actDeathGhost && entity )
		{
			if ( !players[player]->ghost.allowedInteractEntity(*entity) )
			{
				return nullptr;
			}
		}
	}

	if ( !entity && !mute_player_monster_sounds && !clickCheckOverride 
		&& clicktype != ENTITY_CLICK_CALLOUT )
	{
		if ( players[player] && players[player]->entity && players[player]->movement.monsterEmoteGimpTimer == 0 )
		{
			players[player]->movement.monsterEmoteGimpTimer = TICKS_PER_SECOND * 5;
			int sfx = 0;
			int line = 0;
			switch ( stats[player]->type )
			{
				case SKELETON:
					sfx = 95;
					players[player]->movement.monsterEmoteGimpTimer = TICKS_PER_SECOND;
					break;
				case SUCCUBUS:
					sfx = 70;
					break;
				case VAMPIRE:
					if ( local_rng.rand() % 4 == 0 )
					{
						sfx = 329;
					}
					else
					{
						sfx = 322 + local_rng.rand() % 3;
					}
					break;
				case GOATMAN:
					sfx = 332 + local_rng.rand() % 2;
					break;
				case INSECTOID:
					sfx = 291 + local_rng.rand() % 4;
					break;
				case GOBLIN:
					sfx = 60 + local_rng.rand() % 3;
					break;
				case AUTOMATON:
					sfx = 257 + local_rng.rand() % 2;
					break;
				case INCUBUS:
					sfx = 276 + local_rng.rand() % 3;
					break;
				case RAT:
					sfx = 29;
					break;
				case TROLL:
					if ( local_rng.rand() % 3 == 0 )
					{
						sfx = 79;
					}
					break;
				case SPIDER:
				    if ( arachnophobia_filter )
				    {
					    if ( local_rng.rand() % 3 == 2 )
					    {
						    sfx = 508;
					    }
					    else
					    {
						    sfx = 503 + local_rng.rand() % 2;
					    }
				    }
				    else
				    {
					    if ( local_rng.rand() % 3 == 2 )
					    {
						    sfx = 235;
					    }
					    else
					    {
						    sfx = 230 + local_rng.rand() % 2;
					    }
				    }
					break;
				case CREATURE_IMP:
					sfx = 198 + local_rng.rand() % 3;
					break;
				default:
					sfx = 0;
					break;
			}

			//Tell the server we made a noise.
			if ( sfx != 0 )
			{
				if ( multiplayer == CLIENT )
				{
					playSound(sfx, 92);
					strcpy((char*)net_packet->data, "EMOT");
					net_packet->data[4] = player;
					SDLNet_Write16(sfx, &net_packet->data[5]);
					net_packet->address.host = net_server.host;
					net_packet->address.port = net_server.port;
					net_packet->len = 7;
					sendPacketSafe(net_sock, -1, net_packet, 0);
				}
				else if ( multiplayer != CLIENT )
				{
					playSound(sfx, 92);
					for ( int c = 1; c < MAXPLAYERS; ++c )
					{
						if ( !client_disconnected[c] && !players[c]->isLocalPlayer() )
						{
							strcpy((char*)net_packet->data, "SNEL");
							SDLNet_Write16(sfx, &net_packet->data[4]);
							SDLNet_Write32((Uint32)players[player]->entity->getUID(), &net_packet->data[6]);
							SDLNet_Write16(92, &net_packet->data[10]);
							net_packet->address.host = net_clients[c - 1].host;
							net_packet->address.port = net_clients[c - 1].port;
							net_packet->len = 12;
							sendPacketSafe(net_sock, -1, net_packet, c - 1);
						}
					}
				}
			}
		}
	}
	return entity;
}

/*-------------------------------------------------------------------------------

	entityInsideTile

	checks whether an entity is intersecting an impassible tile

-------------------------------------------------------------------------------*/

bool entityInsideTile(Entity* entity, int x, int y, int z, bool checkSafeTiles)
{
	if ( !entity )
	{
		return false;
	}
	if ( x < 0 || x >= map.width || y < 0 || y >= map.height || z < 0 || z >= MAPLAYERS )
	{
		return false;
	}
	if ( (entity->x + entity->sizex) >= (x << 4) )
	{
		if ( (entity->x - entity->sizex) < ((x + 1) << 4) )
		{
			if ( (entity->y + entity->sizey) >= (y << 4) )
			{
				if ( (entity->y - entity->sizey) < ((y + 1) << 4) )
				{
					if ( z == OBSTACLELAYER )
					{
						if ( map.tiles[z + y * MAPLAYERS + x * MAPLAYERS * map.height] )
						{
							return true;
						}
					}
					else if ( z == 0 )
					{
						if ( !checkSafeTiles && !map.tiles[z + y * MAPLAYERS + x * MAPLAYERS * map.height] )
						{
							if ( entity->behavior != &actDeathGhost && !(entity->behavior == &actMonster && entity->getStats() && entity->getStats()->type == BAT_SMALL) )
							{
								return true;
							}
						}
						else if ( checkSafeTiles && map.tiles[z + y * MAPLAYERS + x * MAPLAYERS * map.height] )
						{
							return true;
						}
                        if (entity && entity->behavior == &actMonster) {
							bool waterWalking = entity->isWaterWalking();
							bool lavaWalking = entity->isLavaWalking();
                            if ((swimmingtiles[map.tiles[z + y * MAPLAYERS + x * MAPLAYERS * map.height]] && !waterWalking) ||
                                (lavatiles[map.tiles[z + y * MAPLAYERS + x * MAPLAYERS * map.height]] && !lavaWalking))
                            {
                                return true;
                            }
                        }
					}
				}
			}
		}
	}
	return false;
}

/*-------------------------------------------------------------------------------

	entityInsideEntity

	checks whether an entity is intersecting another entity

-------------------------------------------------------------------------------*/

bool entityInsideEntity(Entity* entity1, Entity* entity2)
{
	if ( !entity1 || !entity2 ) { return false; }
	if ( entity1->x + entity1->sizex > entity2->x - entity2->sizex )
	{
		if ( entity1->x - entity1->sizex < entity2->x + entity2->sizex )
		{
			if ( entity1->y + entity1->sizey > entity2->y - entity2->sizey )
			{
				if ( entity1->y - entity1->sizey < entity2->y + entity2->sizey )
				{
					return true;
				}
			}
		}
	}
	return false;
}

/*-------------------------------------------------------------------------------

	entityInsideSomething

	checks whether an entity is intersecting any obstacle

-------------------------------------------------------------------------------*/

bool entityInsideSomething(Entity* entity)
{
	if ( !entity ) { return false; }
	#ifdef __ARM_NEON__
    const float f[2] = { (float)entity->x, (float)entity->y };
	int32x2_t xy = vcvt_s32_f32(vmul_n_f32(vld1_f32(f), 1.f/16.f));
	const int x = xy[0];
	const int y = xy[1];
	#else
	const int x = entity->x / 16;
	const int y = entity->y / 16;
	#endif
    
	// test against the map
	for (int z = 0; z < MAPLAYERS; ++z) {
		if (entityInsideTile(entity, x, y, z)) {
			return true;
		}
	}

	// test against entities
	std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(entity, 2);
	for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end(); ++it )
	{
		list_t* currentList = *it;
		for ( node_t* node = currentList->first; node != nullptr; node = node->next )
		{
			Entity* testEntity = (Entity*)node->element;
			if ( testEntity == entity || testEntity->flags[PASSABLE] )
			{
				continue;
			}
			if ( entity->behavior == &actDeathGhost || entity->getMonsterTypeFromSprite() == BAT_SMALL )
			{
				if ( testEntity->behavior == &actMonster || testEntity->behavior == &actPlayer 
					|| (testEntity->isDamageableCollider() && (testEntity->colliderHasCollision & EditorEntityData_t::COLLIDER_COLLISION_FLAG_NPC)) )
				{
					continue;
				}
			}
			if ( entityInsideEntity(entity, testEntity) )
			{
				return true;
			}
		}
	}

	return false;
}

static ConsoleVariable<float> cvar_linetrace_smallcollision("/linetrace_smallcollision", 4.0);
bool useSmallCollision(Entity& my, Stat& myStats, Entity& your, Stat& yourStats)
{
	if ( (my.behavior == &actMonster || my.behavior == &actPlayer) &&
		(your.behavior == &actMonster || your.behavior == &actPlayer) )
	{
		if ( my.getUID() == yourStats.leader_uid
			|| your.getUID() == myStats.leader_uid
			|| (myStats.leader_uid != 0 && myStats.leader_uid == yourStats.leader_uid)
			|| (my.behavior == &actPlayer && your.behavior == &actPlayer)
			|| (my.behavior == &actPlayer && your.monsterAllyGetPlayerLeader())
			|| (your.behavior == &actPlayer && my.monsterAllyGetPlayerLeader()) )
		{
			return true;
		}
	}
	return false;
}

bool Entity::collisionProjectileMiss(Entity* parent, Entity* projectile)
{
	if ( multiplayer == CLIENT ) { return false; }
	if ( !projectile ) { return false; }
	if ( hit.entity ) { return false; } // we hit something in clipMove already
	if ( (Sint32)getUID() < 0 )
	{
		return false;
	}
	if ( !(projectile->behavior == &actMonster || projectile->behavior == &actPlayer) )
	{
		if ( projectile->collisionIgnoreTargets.find(getUID()) != projectile->collisionIgnoreTargets.end() )
		{
			return true;
		}
	}

	if ( behavior == &actBell )
	{
		if ( !flags[BURNING] )
		{
			if ( projectile->behavior == &actMagicMissile )
			{
				if ( projectile->children.first && projectile->children.first->element )
				{
					if ( spell_t* spell = (spell_t*)projectile->children.first->element )
					{
						if ( spell->ID == SPELL_FIREBALL || spell->ID == SPELL_SLIME_FIRE )
						{
							SetEntityOnFire();
							if ( parent && flags[BURNING] )
							{
								skill[13] = parent->getUID(); // burning inflicted by for bell
								if ( parent->behavior == &actPlayer )
								{
									messagePlayer(parent->skill[2], MESSAGE_INTERACTION, Language::get(6297));
								}
							}
						}
					}
				}
			}
			else if ( projectile->behavior == &actArrow && projectile->arrowQuiverType == QUIVER_FIRE )
			{
				SetEntityOnFire(parent);
				if ( parent && flags[BURNING] )
				{
					skill[13] = parent->getUID(); // burning inflicted by for bell
					if ( parent->behavior == &actPlayer )
					{
						messagePlayer(parent->skill[2], MESSAGE_INTERACTION, Language::get(6297));
					}
				}
			}
			else if ( projectile->flags[BURNING] && (projectile->behavior == &actMonster || projectile->behavior == &actPlayer) )
			{
				SetEntityOnFire(projectile);
				if ( flags[BURNING] )
				{
					skill[13] = projectile->getUID(); // burning inflicted by for bell
					if ( projectile->behavior == &actPlayer )
					{
						messagePlayer(projectile->skill[2], MESSAGE_INTERACTION, Language::get(6297));
					}
				}
			}
		}
		return true;
	}

	if ( behavior == &actMonster || behavior == &actPlayer )
	{
		if ( projectile->behavior == &actMonster || projectile->behavior == &actPlayer )
		{
			return false;
		}
		if ( Stat* myStats = getStats() )
		{
			if ( myStats->type == BAT_SMALL || myStats->EFFECTS[EFF_AGILITY] )
			{
				bool miss = false;
				if ( myStats->type == BAT_SMALL && isUntargetableBat() )
				{
					projectile->collisionIgnoreTargets.insert(getUID());
					return true;
				}
				if ( myStats->type == BAT_SMALL && monsterSpecialState == BAT_REST )
				{
					return false;
				}
				bool backstab = false;
				bool flanking = false;
				real_t hitAngle = this->yawDifferenceFromEntity(projectile);
				if ( (hitAngle >= 0 && hitAngle <= 2 * PI / 3) ) // 120 degree arc
				{
					if ( behavior == &actPlayer )
					{
						if ( local_rng.rand() % 2 == 0 )
						{
							flanking = true;
						}
					}
					else
					{
						if ( monsterState == MONSTER_STATE_WAIT
							|| monsterState == MONSTER_STATE_PATH
							|| (monsterState == MONSTER_STATE_HUNT && uidToEntity(monsterTarget) == nullptr) )
						{
							// unaware monster, get backstab damage.
							backstab = true;
						}
						else if ( local_rng.rand() % 2 == 0 )
						{
							// monster currently engaged in some form of combat maneuver
							// 1 in 2 chance to flank defenses.
							flanking = true;
						}
					}
				}

				bool accuracyBonus = projectile->behavior == &actMagicMissile && myStats->type == BAT_SMALL;
				if ( backstab )
				{
					miss = false;
				}
				else
				{
					int baseChance = myStats->type == BAT_SMALL ? 6 : 3;
					if ( accuracyBonus )
					{
						baseChance -= 2;
					}
					if ( flanking )
					{
						baseChance -= 2;
					}
					baseChance = std::max(1, baseChance);
					miss = local_rng.rand() % 10 < baseChance;
				}

				if ( miss )
				{
					if ( projectile->collisionIgnoreTargets.find(getUID()) == projectile->collisionIgnoreTargets.end() )
					{
						projectile->collisionIgnoreTargets.insert(getUID());
						if ( (parent && parent->behavior == &actPlayer) 
							|| (parent && parent->behavior == &actMonster && parent->monsterAllyGetPlayerLeader())
							|| (behavior == &actPlayer)
							|| (behavior == &actMonster && monsterAllyGetPlayerLeader()) )
						{
							spawnDamageGib(this, 0, DamageGib::DMG_MISS, DamageGibDisplayType::DMG_GIB_MISS, true);
						}

						if ( behavior == &actPlayer )
						{
							if ( projectile->behavior == &actMagicMissile )
							{
								messagePlayerColor(skill[2], MESSAGE_COMBAT, makeColorRGB(0, 255, 0), Language::get(6287), Language::get(6295));
							}
							else if ( projectile->behavior == &actArrow )
							{
								if ( projectile->sprite == 167 )
								{
									// bolt
									messagePlayerColor(skill[2], MESSAGE_COMBAT, makeColorRGB(0, 255, 0), Language::get(6287), Language::get(6292));
								}
								else if ( projectile->sprite == 78 )
								{
									// rock
									messagePlayerColor(skill[2], MESSAGE_COMBAT, makeColorRGB(0, 255, 0), Language::get(6287), Language::get(6293));
								}
								else
								{
									// arrow
									messagePlayerColor(skill[2], MESSAGE_COMBAT, makeColorRGB(0, 255, 0), Language::get(6287), Language::get(6291));
								}
							}
							else if ( projectile->behavior == &actThrown )
							{
								if ( projectile->skill[10] >= 0 && projectile->skill[10] < NUMITEMS )
								{
									messagePlayerColor(skill[2], MESSAGE_COMBAT, makeColorRGB(0, 255, 0), Language::get(6294), items[projectile->skill[10]].getUnidentifiedName());
								}
								else
								{
									// generic "projectile"
									messagePlayerColor(skill[2], MESSAGE_COMBAT, makeColorRGB(0, 255, 0), Language::get(6294), Language::get(6296));
								}
							}
							else
							{
								messagePlayerColor(skill[2], MESSAGE_COMBAT, makeColorRGB(0, 255, 0), Language::get(6286));
							}
						}
					}
				}
				return miss;
			}
		}
	}
	return false;
}

/*-------------------------------------------------------------------------------

	barony_clear

	checks all blocks around tx/ty

-------------------------------------------------------------------------------*/

int barony_clear(real_t tx, real_t ty, Entity* my)
{
	if (!my)
	{
		return 1;
	}

	long x, y;
	real_t tx2, ty2;
	node_t* node;
	Entity* entity;
	bool levitating = false;
// Reworked that function to break the loop in two part. 
// A first fast one using integer only x/y
// And the second part that loop on entity and used a global BoundingBox collision detection
// Also, static stuff are out of the loop too

	Stat* stats = my->getStats();
	// moved static stuff outside of the loop
	if ( stats )
	{
		levitating = isLevitating(stats);
	}
	bool isMonster = false;
	if ( my )
	{
		if ( my->behavior == &actMonster )
		{
			isMonster = true;
		}
	}
	if ( isMonster && multiplayer == CLIENT )
	{
		if ( my->sprite == 289 || my->sprite == 274 || my->sprite == 413 )   // imp and lich and cockatrice
		{
			levitating = true;
		}
	}

	bool waterWalking = my && my->isWaterWalking();
	bool lavaWalking = my && my->isLavaWalking();

	bool reduceCollisionSize = false;
	bool tryReduceCollisionSize = false;
	bool projectileAttack = false;
	Entity* parent = nullptr;
	Stat* parentStats = nullptr;
	if ( my )
	{
		if ( my->behavior != &actPlayer && my->behavior != &actMonster )
		{
			levitating = true;
		}
		if ( multiplayer != CLIENT )
		{
			if ( my->behavior == &actArrow || my->behavior == &actMagicMissile || my->behavior == &actThrown )
			{
				projectileAttack = true;
				if ( parent = uidToEntity(my->parent) )
				{
					if ( my->behavior == &actThrown )
					{
						tryReduceCollisionSize = true;
						if ( Item* item = newItemFromEntity(my) )
						{
							if ( itemCategory(item) == POTION && !item->doesPotionHarmAlliesOnThrown() )
							{
								tryReduceCollisionSize = false;
							}
							free(item);
							item = nullptr;
						}
						if ( tryReduceCollisionSize )
						{
							parentStats = parent->getStats();
						}
					}
					else
					{
						tryReduceCollisionSize = true;
						parentStats = parent->getStats();
					}
				}
			}
		}
	}

	long ymin = floor((ty - my->sizey)/16), ymax = floor((ty + my->sizey)/16);
	long xmin = floor((tx - my->sizex)/16), xmax = floor((tx + my->sizex)/16);
	const real_t tymin = ty - my->sizey, tymax = ty + my->sizey;
	const real_t txmin = tx - my->sizex, txmax = tx + my->sizex;
	if ( my && my->flags[NOCLIP_WALLS] )
	{
		for ( y = ymin; y <= ymax; y++ )
		{
			for ( x = xmin; x <= xmax; x++ )
			{
				if ( x >= 0 && y >= 0 && x < map.width && y < map.height )
				{
					if ( x == 1 || (x == map.width - 1) || y == 1 || (y == map.height - 1) )
					{
						// collides with map edges only
						hit.x = x * 16 + 8;
						hit.y = y * 16 + 8;
						hit.mapx = x;
						hit.mapy = y;
						hit.entity = NULL;
						return 0;
					}
				}
			}
		}
		if ( my && my->behavior == &actMagiclightMoving )
		{
			return 1; // no other collision
		}
	}
	else
	{
		for ( y = ymin; y <= ymax; y++ )
		{
			for ( x = xmin;  x <= xmax; x++ )
			{
				if ( x >= 0 && y >= 0 && x < map.width && y < map.height )
				{
					if (map.tiles[OBSTACLELAYER + y * MAPLAYERS + x * MAPLAYERS * map.height])
					{
						// hit a wall
						hit.x = x * 16 + 8;
						hit.y = y * 16 + 8;
						hit.mapx = x;
						hit.mapy = y;
						hit.entity = NULL;
						return 0;
					}
	
					if ( !levitating && (!map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height] 
						|| (((swimmingtiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]] && !waterWalking) 
							|| (lavatiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]] && !lavaWalking))
							&& isMonster)) )
					{
						// no floor
						hit.x = x * 16 + 8;
						hit.y = y * 16 + 8;
						hit.mapx = x;
						hit.mapy = y;
						hit.entity = NULL;
						return 0;
					}
				}
			}
		}
	}

	std::vector<list_t*> entLists;
	if ( multiplayer == CLIENT )
	{
		entLists.push_back(map.entities); // clients use old map.entities method
	}
	else
	{
		entLists = TileEntityList.getEntitiesWithinRadius(static_cast<int>(tx) >> 4, static_cast<int>(ty) >> 4, 2);
	}

	Monster type = NOTHING;
	if ( my && isMonster )
	{
		type = my->getMonsterTypeFromSprite();
	}
	bool entityDodgeChance = false;
	for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end(); ++it )
	{
		list_t* currentList = *it;
		for ( node = currentList->first; node != nullptr; node = node->next )
		{
			entity = (Entity*)node->element;
			if ( entity == my || my->parent == entity->getUID() )
			{
				continue;
			}
			entityDodgeChance = false;
			if ( entity->flags[PASSABLE] )
			{
				if ( my->behavior == &actBoulder && (entity->behavior == &actMonster && entity->sprite == 886) )
				{
					// 886 is gyrobot, as they are passable, force collision here.
				}
				else if ( entity->sprite == 1478 
					&& (projectileAttack 
						|| (my && my->flags[BURNING] && (my->behavior == &actMonster || my->behavior == &actPlayer))) && multiplayer != CLIENT )
				{
					// bell rope, check for burning
					entityDodgeChance = true;
				}
				else
				{
					continue;
				}
			}
			if ( entity->behavior == &actParticleTimer && static_cast<Uint32>(entity->particleTimerTarget) == my->getUID() )
			{
				continue;
			}
			if ( ((entity->isDamageableCollider() && (entity->colliderHasCollision & EditorEntityData_t::COLLIDER_COLLISION_FLAG_MINO))
				|| entity->behavior == &::actDaedalusShrine)
				&& my->behavior == &actMonster && type == MINOTAUR )
			{
				continue;
			}
			if ( entity->isDamageableCollider() && (entity->colliderHasCollision & EditorEntityData_t::COLLIDER_COLLISION_FLAG_NPC)
				&& ((my->behavior == &actMonster && (type == GYROBOT || type == BAT_SMALL)) || my->behavior == &actDeathGhost) )
			{
				continue;
			}
			if ( entity->behavior == &actFurniture && type == BAT_SMALL )
			{
				continue;
			}
			if ( entity->getMonsterTypeFromSprite() == BAT_SMALL )
			{
				if ( my->behavior == &actBoulder )
				{
					if ( entity->isUntargetableBat() && my->z > -2.0 )
					{
						continue;
					}
					else
					{
						// force collision here.
					}
				}
				else if ( projectileAttack )
				{
					// calculate later if hit
					entityDodgeChance = true;
				}
				else
				{
					continue;
				}
			}
			if ( (my->behavior == &actMonster || my->behavior == &actBoulder) && entity->behavior == &actDoorFrame )
			{
				continue;    // monsters don't have hard collision with door frames
			}
			if ( my->behavior == &actDeathGhost && (entity->behavior == &actMonster 
				|| entity->behavior == &actPlayer 
				|| (entity->behavior == &actBoulder && entityInsideEntity(my, entity))) )
			{
				continue;
			}
			if ( my->flags[NOCLIP_CREATURES]
				&& (entity->behavior == &actMonster || entity->behavior == &actPlayer) )
			{
				continue;
			}
			Stat* myStats = stats; //my->getStats();	//SEB <<<
			Stat* yourStats = entity->getStats();
			if ( my->behavior == &actPlayer && entity->behavior == &actPlayer )
			{
				continue;
			}
			if ( projectileAttack && yourStats && yourStats->EFFECTS[EFF_AGILITY] )
			{
				entityDodgeChance = true;
			}
			if ( myStats && yourStats )
			{
				if ( yourStats->leader_uid == my->getUID() )
				{
					continue;
				}
				if ( myStats->leader_uid == entity->getUID() )
				{
					continue;
				}
				if ( entity->behavior == &actMonster && yourStats->type == NOTHING && multiplayer == CLIENT )
				{
					// client doesn't know about the type of the monster.
					yourStats->type = static_cast<Monster>(entity->getMonsterTypeFromSprite());
				}
				if ( monsterally[myStats->type][yourStats->type] )
				{
					if ( my->behavior == &actPlayer && myStats->type != HUMAN )
					{
						if ( my->checkFriend(entity) )
						{
							continue;
						}
					}
					else if ( my->behavior == &actMonster && entity->behavior == &actPlayer )
					{
						if ( my->checkFriend(entity) )
						{
							continue;
						}
					}
					else
					{
						if ( my->behavior == &actPlayer && yourStats->monsterForceAllegiance == Stat::MONSTER_FORCE_PLAYER_ENEMY
							|| entity->behavior == &actPlayer && myStats->monsterForceAllegiance == Stat::MONSTER_FORCE_PLAYER_ENEMY )
						{
							// forced enemies.
						}
						else
						{
							continue;
						}
					}
				}
				else if ( my->behavior == &actPlayer )
				{
					if ( my->checkFriend(entity) )
					{
						continue;
					}
				}
				if ( (myStats->type == HUMAN || my->flags[USERFLAG2]) && (yourStats->type == HUMAN || entity->flags[USERFLAG2]) )
				{
					continue;
				}
			}
			else if ( multiplayer != CLIENT )
			{
				if ( entityDodgeChance )
				{
					if ( my->collisionIgnoreTargets.find(entity->getUID()) != my->collisionIgnoreTargets.end() )
					{
						continue;
					}
				}
				if ( tryReduceCollisionSize )
				{
					if ( my->behavior == &actMagicMissile && my->actmagicSpray == 1 )
					{
						if ( my->actmagicEmitter > 0 )
						{
							auto& emitterHit = particleTimerEmitterHitEntities[my->actmagicEmitter];
							auto find = emitterHit.find(entity->getUID());
							if ( find != emitterHit.end() )
							{
								if ( find->second.hits >= 3 || (ticks - find->second.tick) < 5 )
								{
									continue;
								}
							}
						}
					}

					if ( parent && parentStats && yourStats )
					{
						reduceCollisionSize = useSmallCollision(*parent, *parentStats, *entity, *yourStats);
						if ( reduceCollisionSize )
						{
							if ( parent->monsterIsTinkeringCreation()
								&& yourStats->mask && yourStats->mask->type == MASK_TECH_GOGGLES
								&& (parentStats->leader_uid == entity->getUID()
									|| parent->monsterAllyGetPlayerLeader() == entity) )
							{
								continue;
							}
							if ( my->behavior == &actMagicMissile && my->actmagicSpray == 1 )
							{
								continue;
							}
						}


					}
					else if ( parent && parent->behavior == &actDeathGhost
						&& (entity->behavior == &actPlayer
							|| (entity->behavior == &actMonster && entity->monsterAllyGetPlayerLeader())) )
					{
						reduceCollisionSize = true;
					}
				}
			}

			if ( multiplayer == CLIENT )
			{
				// fixes bug where clients can't move through humans
				if ( entity->isPlayerHeadSprite() ||
					entity->sprite == 217 )   // human heads (217 is shopkeep)
				{
					continue;
				}
				else if ( my->behavior == &actPlayer && entity->flags[USERFLAG2] )
				{
					continue; // fix clients not being able to walk through friendly monsters
				}
			}
			real_t sizex = entity->sizex;
			real_t sizey = entity->sizey;
			if ( reduceCollisionSize )
			{
				sizex /= *cvar_linetrace_smallcollision;
				sizey /= *cvar_linetrace_smallcollision;
			}
			real_t eymin = entity->y - sizey, eymax = entity->y + sizey;
			real_t exmin = entity->x - sizex, exmax = entity->x + sizex;
			if ( entity->sprite == 1478 )
			{
				real_t xoffset = entity->focalx * cos(entity->yaw) + entity->focaly * cos(entity->yaw + PI / 2);
				real_t yoffset = entity->focalx * sin(entity->yaw) + entity->focaly * sin(entity->yaw + PI / 2);
				eymin += yoffset;
				eymax += yoffset;
				exmin += xoffset;
				exmax += xoffset;
			}
			if ( (entity->sizex > 0) && ((txmin >= exmin && txmin < exmax) || (txmax >= exmin && txmax < exmax) || (txmin <= exmin && txmax > exmax)) )
			{
				if ( (entity->sizey > 0) && ((tymin >= eymin && tymin < eymax) || (tymax >= eymin && tymax < eymax) || (tymin <= eymin && tymax > eymax)) )
				{
					if ( multiplayer != CLIENT )
					{
						if ( entityDodgeChance )
						{
							if ( entity->collisionProjectileMiss(parent, my) )
							{
								continue;
							}
						}
					}

					tx2 = std::max(txmin, exmin);
					ty2 = std::max(tymin, eymin);
					hit.x = tx2;
					hit.y = ty2;
					hit.mapx = entity->x / 16;
					hit.mapy = entity->y / 16;
					hit.entity = entity;
					if ( multiplayer != CLIENT )
					{
						if ( my->flags[BURNING] && !hit.entity->flags[BURNING] && hit.entity->flags[BURNABLE] )
						{
							bool dyrnwyn = false;
							Stat* stats = hit.entity->getStats();
							if ( stats )
							{
								if ( stats->weapon )
								{
									if ( stats->weapon->type == ARTIFACT_SWORD )
									{
										dyrnwyn = true;
									}
								}
							}
							if ( !dyrnwyn )
							{
								bool previouslyOnFire = hit.entity->flags[BURNING];

								// Attempt to set the Entity on fire
								hit.entity->SetEntityOnFire();

								// If the Entity is now on fire, tell them
								if ( hit.entity->flags[BURNING] && !previouslyOnFire )
								{
									messagePlayer(hit.entity->skill[2], MESSAGE_STATUS, Language::get(590)); // "You suddenly catch fire!"
								}
							}
						}
						else if ( hit.entity->flags[BURNING] && !my->flags[BURNING] && my->flags[BURNABLE] )
						{
							bool dyrnwyn = false;
							Stat* stats = my->getStats();
							if ( stats )
							{
								if ( stats->weapon )
								{
									if ( stats->weapon->type == ARTIFACT_SWORD )
									{
										dyrnwyn = true;
									}
								}
							}
							if ( !dyrnwyn )
							{
								bool previouslyOnFire = hit.entity->flags[BURNING];

								// Attempt to set the Entity on fire
								hit.entity->SetEntityOnFire();

								// If the Entity is now on fire, tell them
								if ( hit.entity->flags[BURNING] && !previouslyOnFire )
								{
									messagePlayer(hit.entity->skill[2], MESSAGE_STATUS, Language::get(590)); // "You suddenly catch fire!"
								}
							}
						}
					}
					return 0;
				}
			}
		}
	}

	return 1;
}

/*-------------------------------------------------------------------------------

	clipMove

	clips velocity by checking which direction is clear. returns distance
	covered.

-------------------------------------------------------------------------------*/

real_t clipMove(real_t* x, real_t* y, real_t vx, real_t vy, Entity* my)
{
	real_t tx, ty;
	hit.entity = NULL;

	// move x and y
	tx = *x + vx;
	ty = *y + vy;
	if (barony_clear(tx, ty, my))
	{
		*x = tx;
		*y = ty;
		hit.side = 0;
		return sqrt(vx * vx + vy * vy);
	}

	// only move x
	tx = *x + vx;
	ty = *y;
	if (barony_clear(tx, ty, my))
	{
		*x = tx;
		*y = ty;
		hit.side = VERTICAL;
		return fabs(vx);
	}

	// only move y
	tx = *x;
	ty = *y + vy;
	if (barony_clear(tx, ty, my))
	{
		*x = tx;
		*y = ty;
		hit.side = HORIZONTAL;
		return fabs(vy);
	}
	hit.side = 0;
	return 0;
}

/*-------------------------------------------------------------------------------

	findEntityInLine

	returns the closest entity to intersect a ray starting from x1, y1 and
	extending along the given angle. May return an improper result when
	some entities overlap one another.

-------------------------------------------------------------------------------*/

Entity* findEntityInLine( Entity* my, real_t x1, real_t y1, real_t angle, int entities, Entity* target )
{
	Entity* result = NULL;
	node_t* node;
	real_t lowestDist = 9999;
	int quadrant = 0;

	while ( angle >= PI * 2 )
	{
		angle -= PI * 2;
	}
	while ( angle < 0 )
	{
		angle += PI * 2;
	}

	if ( !my )
	{
		return nullptr;
	}
	int originx = static_cast<int>(my->x) >> 4;
	int originy = static_cast<int>(my->y) >> 4;
	std::vector<list_t*> entLists; // stores the possible entities to look through depending on the quadrant.
	// start search from 1 tile behind facing direction in x/y position, extending to the edge of the map in the facing direction.

	if ( multiplayer == CLIENT )
	{
		entLists.push_back(map.entities); // default to old map.entities if client (if they ever call this function...)
	}

	Stat* myStats = my->getStats();

	if ( angle >= PI / 2 && angle < PI ) // -x, +y
	{
		quadrant = 1;
		/*messagePlayer(0, "drawing from x: %d - %d, y: %d- %d", 
			0,	std::min(static_cast<int>(map.width) - 1, originx + 1),
			std::max(0, originy - 1), map.height - 1);*/

		if ( multiplayer != CLIENT )
		{
			for ( int ix = std::min(static_cast<int>(map.width) - 1, originx + 1); ix >= 0; --ix )
			{
				for ( int iy = std::max(0, originy - 1); iy < map.height; ++iy )
				{
					entLists.push_back(&TileEntityList.gridEntities[ix][iy]);
				}
			}
		}
	}
	else if ( angle >= 0 && angle < PI / 2 ) // +x, +y
	{
		quadrant = 2;
		/*messagePlayer(0, "drawing from x: %d - %d, y: %d- %d",
			std::max(0, originx - 1), map.width - 1,
			std::max(0, originy - 1), map.height - 1);*/

		if ( multiplayer != CLIENT )
		{
			for ( int ix = std::max(0, originx - 1); ix < map.width; ++ix )
			{
				for ( int iy = std::max(0, originy - 1); iy < map.height; ++iy )
				{
					entLists.push_back(&TileEntityList.gridEntities[ix][iy]);
				}
			}
		}
	}
	else if ( angle >= 3 * (PI / 2) && angle < PI * 2 ) // +x, -y
	{
		quadrant = 3;
		/*messagePlayer(0, "drawing from x: %d - %d, y: %d- %d",
			std::max(0, originx - 1), map.width - 1,
			0, std::min(static_cast<int>(map.height) - 1, originy + 1));*/

		if ( multiplayer != CLIENT )
		{
			for ( int ix = std::max(0, originx - 1); ix < map.width; ++ix )
			{
				for ( int iy = std::min(static_cast<int>(map.height) - 1, originy + 1); iy >= 0; --iy )
				{
					entLists.push_back(&TileEntityList.gridEntities[ix][iy]);
				}
			}
		}
	}
	else // -x, -y
	{
		quadrant = 4;
		/*messagePlayer(0, "drawing from x: %d - %d, y: %d- %d",
			0, std::min(static_cast<int>(map.width) - 1, originx + 1),
			0, std::min(static_cast<int>(map.height) - 1, originy + 1));*/

		if ( multiplayer != CLIENT )
		{
			for ( int ix = std::min(static_cast<int>(map.width) - 1, originx + 1); ix >= 0; --ix )
			{
				for ( int iy = std::min(static_cast<int>(map.height) - 1, originy + 1); iy >= 0; --iy )
				{
					entLists.push_back(&TileEntityList.gridEntities[ix][iy]);
				}
			}
		}
	}

	bool adjust = false;
	if ( angle >= PI / 2 && angle < 3 * (PI / 2) )
	{
		adjust = true;
	}
	else
	{
		while ( angle >= PI )
		{
			angle -= PI * 2;
		}
		while ( angle < -PI )
		{
			angle += PI * 2;
		}
	}

	//std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
	bool ignoreFurniture = my && my->behavior == &actMonster && myStats
		&& (myStats->type == SHOPKEEPER
			|| myStats->type == MINOTAUR
			|| myStats->type == BAT_SMALL);

	for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end(); ++it )
	{
		list_t* currentList = *it;
		for ( node = currentList->first; node != nullptr; node = node->next )
		{
			Entity* entity = (Entity*)node->element;
			if ( (entity != target && target != nullptr) || entity->flags[PASSABLE] || entity == my
				|| ((entities == LINETRACE_IGNORE_ENTITIES) && 
						( (!entity->flags[BLOCKSIGHT] && entity->behavior != &actMonster) 
							|| (entity->behavior == &actMonster && (entity->flags[INVISIBLE] 
								&& entity->sprite != 889 && entity->sprite != 1247 && entity->sprite != 1408) )
						)
					) 
				)
			{
				// if entities == LINETRACE_IGNORE_ENTITIES, then ignore entities that block sight.
				// 16/11/19 - added exception to monsters. if monster, use the INVISIBLE flag to skip checking.
				// 889/1247/1408 is dummybot/mimic/bat "invisible" AI entity. so it's invisible, need to make it shown here.
				if ( entity->behavior == &actMonster && entity->sprite == 1408 )
				{
					if ( (entity != target && target != nullptr) || entity == my || entity->flags[PASSABLE] )
					{
						continue;
					}
					else if ( entity->isUntargetableBat() )
					{
						continue;
					}
				}
				else
				{
					continue;
				}
			}
			if ( entity->behavior == &actParticleTimer )
			{
				continue;
			}
			if ( ignoreFurniture && 
				(entity->behavior == &actFurniture 
				|| entity->isDamageableCollider()
				|| (entity->behavior == &::actDaedalusShrine && myStats->type == MINOTAUR)) )
			{
				continue; // see through furniture cause we'll bust it down
			}
			if ( entity->isUntargetableBat() )
			{
				continue;
			}

			int entitymapx = static_cast<int>(entity->x) >> 4;
			int entitymapy = static_cast<int>(entity->y) >> 4;
			real_t sizex = entity->sizex;
			real_t sizey = entity->sizey;
			if ( entities == LINETRACE_ATK_CHECK_FRIENDLYFIRE && multiplayer != CLIENT )
			{
				if ( (my->behavior == &actMonster || my->behavior == &actPlayer) 
					&& (entity->behavior == &actMonster || entity->behavior == &actPlayer) )
				{
					Stat* yourStats = entity->getStats();
					if ( myStats && yourStats )
					{
						if ( useSmallCollision(*my, *myStats, *entity, *yourStats) )
						{
							sizex /= *cvar_linetrace_smallcollision;
							sizey /= *cvar_linetrace_smallcollision;
						}
					}
				}
			}

			if ( quadrant == 2 || quadrant == 4 )
			{
				// upper right and lower left
				real_t upperX = entity->x + sizex;
				real_t upperY = entity->y - sizey;
				real_t lowerX = entity->x - sizex;
				real_t lowerY = entity->y + sizey;
				real_t upperTan = atan2(upperY - y1, upperX - x1);
				real_t lowerTan = atan2(lowerY - y1, lowerX - x1);
				if ( adjust )
				{
					if ( upperTan < 0 )
					{
						upperTan += PI * 2;
					}
					if ( lowerTan < 0 )
					{
						lowerTan += PI * 2;
					}
				}

				// determine whether line intersects entity
				if ( quadrant == 2 )
				{
					if ( entitymapx == originx && entitymapy == originy )
					{
						if ( x1 > upperX || y1 > lowerY )
						{
							/*if ( my && my->behavior == &actPlayer && entity->behavior == &actDoor )
							{
								messagePlayer(0, MESSAGE_DEBUG, "quad 2 skip door");
							}*/
							continue;
						}
					}
					else if ( entitymapx < originx || entitymapy < originy )
					{
						// if behind, check if we intersect
						if ( !(x1 >= lowerX && x1 <= upperX && y1 >= upperY && y1 <= lowerY) )
						{
							continue; // no intersection
						}
					}
					if ( angle >= upperTan && angle <= lowerTan )
					{
						real_t dist = sqrt(pow(x1 - entity->x, 2) + pow(y1 - entity->y, 2));
						if ( dist < lowestDist )
						{
							lowestDist = dist;
							result = entity;
						}
					}
				}
				else
				{
					if ( entitymapx == originx && entitymapy == originy )
					{
						if ( x1 < lowerX || y1 < upperY )
						{
							/*if ( my && my->behavior == &actPlayer && entity->behavior == &actDoor )
							{
								messagePlayer(0, MESSAGE_DEBUG, "quad 4 skip door");
							}*/
							continue;
						}
					}
					else if ( entitymapx > originx || entitymapy > originy )
					{
						// if behind, check if we intersect
						if ( !(x1 >= lowerX && x1 <= upperX && y1 >= upperY && y1 <= lowerY) )
						{
							continue; // no intersection
						}
					}
					if ( angle <= upperTan && angle >= lowerTan )
					{
						real_t dist = sqrt(pow(x1 - entity->x, 2) + pow(y1 - entity->y, 2));
						if ( dist < lowestDist )
						{
							lowestDist = dist;
							result = entity;
						}
					}
				}
			}
			else
			{
				// upper left and lower right
				real_t upperX = entity->x - sizex;
				real_t upperY = entity->y - sizey;
				real_t lowerX = entity->x + sizex;
				real_t lowerY = entity->y + sizey;
				real_t upperTan = atan2(upperY - y1, upperX - x1);
				real_t lowerTan = atan2(lowerY - y1, lowerX - x1);
				if ( adjust )
				{
					if ( upperTan < 0 )
					{
						upperTan += PI * 2;
					}
					if ( lowerTan < 0 )
					{
						lowerTan += PI * 2;
					}
				}

				// determine whether line intersects entity
				if ( quadrant == 3 )
				{
					if ( entitymapx == originx && entitymapy == originy )
					{
						if ( x1 > lowerX || y1 < upperY )
						{
							/*if ( my && my->behavior == &actPlayer && entity->behavior == &actDoor )
							{
								messagePlayer(0, MESSAGE_DEBUG, "quad 3 skip door");
							}*/
							continue;
						}
					}
					else if ( entitymapx < originx || entitymapy > originy )
					{
						// if behind, check if we intersect
						if ( !(x1 >= upperX && x1 <= lowerX && y1 >= upperY && y1 <= lowerY) )
						{
							continue; // no intersection
						}
					}
					if ( angle >= upperTan && angle <= lowerTan )
					{
						real_t dist = sqrt(pow(x1 - entity->x, 2) + pow(y1 - entity->y, 2));
						if ( dist < lowestDist )
						{
							lowestDist = dist;
							result = entity;
						}
					}
				}
				else
				{
					if ( entitymapx == originx && entitymapy == originy )
					{
						if ( x1 < upperX || y1 > lowerY )
						{
							/*if ( my && my->behavior == &actPlayer && entity->behavior == &actDoor )
							{
								messagePlayer(0, MESSAGE_DEBUG, "quad 1 skip door");
							}*/
							continue;
						}
					}
					else if ( entitymapx > originx || entitymapy < originy )
					{
						// if behind, check if we intersect
						if ( !(x1 >= upperX && x1 <= lowerX && y1 >= upperY && y1 <= lowerY) )
						{
							continue; // no intersection
						}
					}
					if ( angle <= upperTan && angle >= lowerTan )
					{
						real_t dist = sqrt(pow(x1 - entity->x, 2) + pow(y1 - entity->y, 2));
						if ( dist < lowestDist )
						{
							lowestDist = dist;
							result = entity;
						}
					}
				}
			}
		}
	}
	//std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
	//messagePlayer(0, "%lld", std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1));
	return result;
}

/*-------------------------------------------------------------------------------

	lineTrace

	Trace a line from x1, y1 along the provided heading, place information of
	the first hit obstacle into the "hit" struct, and report distance to
	next obstacle. Uses entity coordinates

-------------------------------------------------------------------------------*/

real_t lineTrace( Entity* my, real_t x1, real_t y1, real_t angle, real_t range, int entities, bool ground )
{
	int posx, posy;
	real_t fracx, fracy;
	real_t rx, ry;
	real_t ix, iy;
	int inx, iny;
	real_t arx, ary;
	real_t dincx, dval0, dincy, dval1;
	real_t d;

	posx = floor(x1);
	posy = floor(y1); // integer coordinates
	fracx = x1 - posx;
	fracy = y1 - posy; // fraction coordinates
	rx = cos(angle);
	ry = sin(angle);
	ix = 0;
	iy = 0;

	inx = posx;
	iny = posy;
	arx = 0;
	if (rx)
	{
		arx = 1.0 / fabs(rx);
	}
	ary = 0;
	if (ry)
	{
		ary = 1.0 / fabs(ry);
	}
	dincx = 0;
	dval0 = 1e32;
	dincy = 0;
	dval1 = 1e32;
	if (rx < 0)
	{
		dincx = -1;
		dval0 = fracx * arx;
	}
	else if (rx > 0)
	{
		dincx = 1;
		dval0 = (1.0 - fracx) * arx;
	}
	if (ry < 0)
	{
		dincy = -1;
		dval1 = fracy * ary;
	}
	else if (ry > 0)
	{
		dincy = 1;
		dval1 = (1.0 - fracy) * ary;
	}
	d = 0;

	Stat* stats = nullptr;
	bool waterWalking = my && my->isWaterWalking();
	bool lavaWalking = my && my->isLavaWalking();
	bool isMonster = false;
	if ( my )
	{
		if ( my->behavior == &actMonster )
		{
			isMonster = true;
			stats = my->getStats();
			if ( stats )
			{
				if ( stats->type == DEVIL )
				{
					ground = false;
				}
				else if ( stats->type == SENTRYBOT || stats->type == SPELLBOT || stats->type == BAT_SMALL )
				{
					ground = false;
				}
			}
		}
	}

	Entity* entity = findEntityInLine(my, x1, y1, angle, entities, NULL);

	Stat* yourStats = nullptr;
	bool reduceCollisionSize = false;
	real_t sizex = 0.0;
	real_t sizey = 0.0;
	if ( entity )
	{
		sizex = entity->sizex;
		sizey = entity->sizey;

		yourStats = entity->getStats();
		if ( entities == LINETRACE_ATK_CHECK_FRIENDLYFIRE )
		{
			if ( my && stats && (my->behavior == &actMonster || my->behavior == &actPlayer) &&
				entity && (entity->behavior == &actMonster || entity->behavior == &actPlayer) && yourStats )
			{
				reduceCollisionSize = useSmallCollision(*my, *stats, *entity, *yourStats);
				if ( reduceCollisionSize )
				{
					sizex /= *cvar_linetrace_smallcollision;
					sizey /= *cvar_linetrace_smallcollision;
				}
			}
		}
	}

	// trace the line
	while ( d < range )
	{
		if ( dval1 > dval0 )
		{
			inx += dincx;
			d = dval0;
			dval0 += arx;
			hit.side = HORIZONTAL;
		}
		else
		{
			iny += dincy;
			d = dval1;
			dval1 += ary;
			hit.side = VERTICAL;
		}
		if ( inx < 0 || iny < 0 || (inx >> 4) >= map.width || (iny >> 4) >= map.height )
		{
			break;
		}

		ix = x1 + rx * d;
		iy = y1 + ry * d;

		// check against the map
		int index = (iny >> 4) * MAPLAYERS + (inx >> 4) * MAPLAYERS * map.height;
		if ( map.tiles[OBSTACLELAYER + index] )
		{
			hit.x = ix;
			hit.y = iy;
			hit.mapx = inx >> 4;
			hit.mapy = iny >> 4;
			hit.entity = NULL;
			return d;
		}
		if ( ground )
		{
			if ( !map.tiles[index] 
				|| (((swimmingtiles[map.tiles[index]] && !waterWalking) 
					|| (lavatiles[map.tiles[index]] && !lavaWalking)) && isMonster) )
			{
				hit.x = ix;
				hit.y = iy;
				hit.mapx = inx >> 4;
				hit.mapy = iny >> 4;
				hit.entity = NULL;
				return d;
			}
		}

		// check against entity
		if ( entity )
		{
			// debug particles.
			if ( my && my->behavior == &actPlayer && entities == LINETRACE_ATK_CHECK_FRIENDLYFIRE )
			{
				static ConsoleVariable<bool> cvar_linetracedebug("/linetracedebug", false);
				if ( *cvar_linetracedebug )
				{
					Entity* particle = spawnMagicParticle(my);
					particle->sprite = 576;
					particle->x = ix;
					particle->y = iy;
					particle->z = 0;

					particle = spawnMagicParticle(my);
					particle->sprite = 942;
					particle->x = entity->x + entity->sizex;
					particle->y = entity->y + entity->sizey;
					particle->z = 0;

					particle = spawnMagicParticle(my);
					particle->sprite = 942;
					particle->x = entity->x - entity->sizex;
					particle->y = entity->y + entity->sizey;
					particle->z = 0;

					particle = spawnMagicParticle(my);
					particle->sprite = 942;
					particle->x = entity->x + entity->sizex;
					particle->y = entity->y - entity->sizey;
					particle->z = 0;

					particle = spawnMagicParticle(my);
					particle->sprite = 942;
					particle->x = entity->x - entity->sizex;
					particle->y = entity->y - entity->sizey;
					particle->z = 0;
				}
			}

			if ( ix >= entity->x - sizex && ix <= entity->x + sizex )
			{
				if ( iy >= entity->y - sizey && iy <= entity->y + sizey )
				{
					hit.x = ix;
					hit.y = iy;
					hit.mapx = entity->x / 16;
					hit.mapy = entity->y / 16;
					hit.entity = entity;
					return d;
				}
			}
		}
	}
	hit.x = ix;
	hit.y = iy;
	hit.mapx = inx >> 4;
	hit.mapy = iny >> 4;
	hit.entity = NULL;
	hit.side = 0;
	return range;
}

real_t lineTraceTarget(Entity* my, real_t x1, real_t y1, real_t angle, real_t range, int entities, bool ground, Entity* target)
{
	int posx, posy;
	real_t fracx, fracy;
	real_t rx, ry;
	real_t ix, iy;
	int inx, iny;
	real_t arx, ary;
	real_t dincx, dval0, dincy, dval1;
	real_t d;

	posx = floor(x1);
	posy = floor(y1); // integer coordinates
	fracx = x1 - posx;
	fracy = y1 - posy; // fraction coordinates
	rx = cos(angle);
	ry = sin(angle);
	ix = 0;
	iy = 0;

	inx = posx;
	iny = posy;
	arx = 0;
	if ( rx )
	{
		arx = 1.0 / fabs(rx);
	}
	ary = 0;
	if ( ry )
	{
		ary = 1.0 / fabs(ry);
	}
	dincx = 0;
	dval0 = 1e32;
	dincy = 0;
	dval1 = 1e32;
	if ( rx < 0 )
	{
		dincx = -1;
		dval0 = fracx * arx;
	}
	else if ( rx > 0 )
	{
		dincx = 1;
		dval0 = (1.0 - fracx) * arx;
	}
	if ( ry < 0 )
	{
		dincy = -1;
		dval1 = fracy * ary;
	}
	else if ( ry > 0 )
	{
		dincy = 1;
		dval1 = (1.0 - fracy) * ary;
	}
	d = 0;

	Entity* entity = findEntityInLine(my, x1, y1, angle, entities, target);

	bool isMonster = false;
	bool waterWalking = my && my->isWaterWalking();
	bool lavaWalking = my && my->isLavaWalking();
	if ( my )
	{
		if ( my->behavior == &actMonster )
		{
			isMonster = true;
		}
	}
	// trace the line
	while ( d < range )
	{
		if ( dval1 > dval0 )
		{
			inx += dincx;
			d = dval0;
			dval0 += arx;
			hit.side = HORIZONTAL;
		}
		else
		{
			iny += dincy;
			d = dval1;
			dval1 += ary;
			hit.side = VERTICAL;
		}
		if ( inx < 0 || iny < 0 || (inx >> 4) >= map.width || (iny >> 4) >= map.height )
		{
			break;
		}

		ix = x1 + rx * d;
		iy = y1 + ry * d;

		// check against the map
		int index = (iny >> 4) * MAPLAYERS + (inx >> 4) * MAPLAYERS * map.height;
		if ( map.tiles[OBSTACLELAYER + index] )
		{
			hit.x = ix;
			hit.y = iy;
			hit.mapx = inx >> 4;
			hit.mapy = iny >> 4;
			hit.entity = NULL;
			return d;
		}
		if ( ground )
		{
			if ( !map.tiles[index] 
				|| (((swimmingtiles[map.tiles[index]] && waterWalking) || (lavatiles[map.tiles[index]] && lavaWalking)) && isMonster) )
			{
				hit.x = ix;
				hit.y = iy;
				hit.mapx = inx >> 4;
				hit.mapy = iny >> 4;
				hit.entity = NULL;
				return d;
			}
		}

		// check against entity
		if ( entity )
		{
			// debug particles.
			if ( my && my->behavior == &actMonster && entities == 0 )
			{
				static ConsoleVariable<bool> cvar_linetracetargetdebug("/linetracetargetdebug", false);
				if ( *cvar_linetracetargetdebug )
				{
					Entity* particle = spawnMagicParticle(my);
					particle->sprite = 942;
					particle->x = ix;
					particle->y = iy;
					particle->z = 0;

					particle = spawnMagicParticle(my);
					particle->sprite = 942;
					particle->x = entity->x + entity->sizex;
					particle->y = entity->y + entity->sizey;
					particle->z = 0;

					particle = spawnMagicParticle(my);
					particle->sprite = 942;
					particle->x = entity->x - entity->sizex;
					particle->y = entity->y + entity->sizey;
					particle->z = 0;

					particle = spawnMagicParticle(my);
					particle->sprite = 942;
					particle->x = entity->x + entity->sizex;
					particle->y = entity->y - entity->sizey;
					particle->z = 0;

					particle = spawnMagicParticle(my);
					particle->sprite = 942;
					particle->x = entity->x - entity->sizex;
					particle->y = entity->y - entity->sizey;
					particle->z = 0;
				}
			}

			if ( ix >= entity->x - entity->sizex && ix <= entity->x + entity->sizex )
			{
				if ( iy >= entity->y - entity->sizey && iy <= entity->y + entity->sizey )
				{
					hit.x = ix;
					hit.y = iy;
					hit.mapx = entity->x / 16;
					hit.mapy = entity->y / 16;
					hit.entity = entity;
					return d;
				}
			}
		}
	}
	hit.x = ix;
	hit.y = iy;
	hit.mapx = inx >> 4;
	hit.mapy = iny >> 4;
	hit.entity = NULL;
	hit.side = 0;
	return range;
}

/*-------------------------------------------------------------------------------

	checkObstacle

	Checks the environment at the given ENTITY coordinates for obstacles,
	performing boundary check

-------------------------------------------------------------------------------*/

int checkObstacle(long x, long y, Entity* my, Entity* target, bool useTileEntityList, bool checkWalls, bool checkFloor)
{
	node_t* node = nullptr;
	Entity* entity = nullptr;
	Stat* stats = nullptr;
	bool levitating = false;

	// get levitation status
	if ( my != NULL && (stats = my->getStats()) != NULL )
	{
		levitating = isLevitating(stats);
	}
	if ( my )
	{
		if ( my->behavior != &actPlayer && my->behavior != &actMonster && my->behavior != &actLadder && my->behavior != &actPortal )
		{
			levitating = true;
		}
	}

	//auto t = std::chrono::high_resolution_clock::now();
	//auto t2 = std::chrono::high_resolution_clock::now();
	//int entCheck = 0;
	//
	// collision detection
	if ( x >= 0 && x < map.width << 4 )
	{
		if ( y >= 0 && y < map.height << 4 )
		{
			int index = (y >> 4) * MAPLAYERS + (x >> 4) * MAPLAYERS * map.height;
			if (checkWalls && map.tiles[OBSTACLELAYER + index])   // wall
			{
				return 1;
			}
			bool isMonster = false;
			if ( my )
			{
				if ( my->behavior == &actMonster )
				{
					isMonster = true;
				}
			}
			bool waterWalking = my && my->isWaterWalking();
			bool lavaWalking = my && my->isLavaWalking();
			if ( !levitating
					&& ((!map.tiles[index] && checkFloor)
								   || ( ((swimmingtiles[map.tiles[index]] && !waterWalking) || (lavatiles[map.tiles[index]] && !lavaWalking))
										 && isMonster) ) )   // no floor
			{
				return 1; // if there's no floor, or either water/lava then a non-levitating monster sees obstacle.
			}

			if ( !useTileEntityList )
			{
				// for map generation to detect if decorations have obstacles without entities being assigned actions
				std::vector<list_t*> entLists{ map.entities };
				for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end(); ++it )
				{
					list_t* currentList = *it;
					for ( node = currentList->first; node != nullptr; node = node->next )
					{
						entity = (Entity*)node->element;
						if ( !entity ) { continue; }
						if ( entity->flags[PASSABLE]
							|| entity == my
							|| entity == target
							|| entity->sprite == 8 // items
							|| entity->sprite == 9 // gold
							|| entity->behavior == &actDoor )
						{
							continue;
						}
						if ( x >= (int)(entity->x - entity->sizex) && x <= (int)(entity->x + entity->sizex) )
						{
							if ( y >= (int)(entity->y - entity->sizey) && y <= (int)(entity->y + entity->sizey) )
							{
								return 1;
							}
						}
					}
				}
			}
			else
			{
				std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadius(static_cast<int>(x) >> 4, static_cast<int>(y) >> 4, 2);
				for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end(); ++it )
				{
					list_t* currentList = *it;
					for ( node = currentList->first; node != nullptr; node = node->next )
					{
						entity = (Entity*)node->element;
						//++entCheck;
						if ( !entity ) { continue; }
						if ( entity->flags[PASSABLE] || entity == my || entity == target || entity->behavior == &actDoor )
						{
							continue;
						}
						if ( my && entity->behavior == &actParticleTimer && static_cast<Uint32>(entity->particleTimerTarget) == my->getUID() )
						{
							continue;
						}
						if ( isMonster && my->getMonsterTypeFromSprite() == MINOTAUR 
							&& ((entity->isDamageableCollider()
									&& (entity->colliderHasCollision & EditorEntityData_t::COLLIDER_COLLISION_FLAG_MINO))
								|| entity->behavior == &::actDaedalusShrine) )
						{
							continue;
						}
						else if ( isMonster && my->getMonsterTypeFromSprite() == GYROBOT && entity->isDamageableCollider()
							&& (entity->colliderHasCollision & EditorEntityData_t::COLLIDER_COLLISION_FLAG_NPC) )
						{
							continue;
						}
						if ( my && my->behavior == &actDeathGhost && (entity->behavior == &actPlayer || entity->behavior == &actMonster) )
						{
							continue;
						}
						if ( entity->behavior == &actMonster && entity->getMonsterTypeFromSprite() == BAT_SMALL )
						{
							continue;
						}
						if ( x >= (int)(entity->x - entity->sizex) && x <= (int)(entity->x + entity->sizex) )
						{
							if ( y >= (int)(entity->y - entity->sizey) && y <= (int)(entity->y + entity->sizey) )
							{
								return 1;
							}
						}
					}
				}
			}
		}
	}
	//t2 = std::chrono::high_resolution_clock::now();
	//real_t time = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t).count();
	//if ( my )
	//{
	//	printlog("checkObstacle: %d: %d %f, entities: %d", my->sprite, my->monsterState, time, entCheck);
	//}

	if ( logCheckObstacle )
	{
		++logCheckObstacleCount;
	}

	return 0;
}
