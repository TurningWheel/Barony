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
#include "sound.hpp"
#include "items.hpp"
#include "interface/interface.hpp"
#include "magic/magic.hpp"
#include "net.hpp"
#include "paths.hpp"
#include "collision.hpp"
#include "player.hpp"
#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

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

	if ( clicktype == ENTITY_CLICK_HELD_USE_TOOLTIPS_ONLY )
	{
		if ( !clickCheckOverride && !(*inputPressedForPlayer(player, impulses[IN_USE])) && !(inputs.bControllerInputHeld(player, INJOY_GAME_USE)) )
		{
			return NULL;
		}
	}
	else
	{
		if ( !clickCheckOverride && !(*inputPressedForPlayer(player, impulses[IN_USE])) && !(inputs.bControllerInputPressed(player, INJOY_GAME_USE)) )
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
		if ( openedChest[player] )
		{
			if ( mx > getChestGUIStartX(player)
				&& mx < getChestGUIStartX(player) + inventoryChest_bmp->w
				&& my > getChestGUIStartY(player)
				&& my < getChestGUIStartY(player) + inventoryChest_bmp->h )
			{
				if ( clickedOnGUI )
				{
					*clickedOnGUI = true;
				}
				return NULL;    //Click falls inside the chest inventory GUI.
			}
		}
		SDL_Rect guiBox;
		GenericGUI[player].getDimensions(guiBox);
		if ( GenericGUI[player].isGUIOpen() )
		{
			if ( mx > guiBox.x
				&& mx < guiBox.x + guiBox.w
				&& my > guiBox.y
				&& my < guiBox.y + guiBox.h )
			{
				if ( clickedOnGUI )
				{
					*clickedOnGUI = true;
				}
				return NULL;    //Click falls inside the generic gui.
			}
		}
		if ( players[player]->bookGUI.bBookOpen )
		{
			if ( mouseInBounds(player,
				players[player]->bookGUI.getStartX(),
				players[player]->bookGUI.getStartX() + players[player]->bookGUI.getBookWidth(),
				players[player]->bookGUI.getStartY(), players[player]->bookGUI.getStartY() + players[player]->bookGUI.getBookHeight()) )
			{
				if ( clickedOnGUI )
				{
					*clickedOnGUI = true;
				}
				return NULL;    //Click falls inside the book GUI.
			}
		}
		if ( players[player]->gui_mode == GUI_MODE_INVENTORY || players[player]->gui_mode == GUI_MODE_SHOP)
		{
			if ( players[player]->gui_mode == GUI_MODE_INVENTORY )
				if (mouseInBounds(player, RIGHTSIDEBAR_X, RIGHTSIDEBAR_X + rightsidebar_titlebar_img->w, RIGHTSIDEBAR_Y, RIGHTSIDEBAR_Y + rightsidebar_height))
				{
					if ( clickedOnGUI )
					{
						*clickedOnGUI = true;
					}
					return NULL;    //Click falls inside the right sidebar.
				}
			//int x = std::max(character_bmp->w, xres/2-inventory_bmp->w/2);
			//if (mouseInBounds(x,x+inventory_bmp->w,0,inventory_bmp->h))
			//return NULL;
			if ( mouseInBounds(player, 
				inventoryUI.getStartX(),
				inventoryUI.getStartX() + inventoryUI.getSizeX() * inventoryUI.getSlotSize(),
				inventoryUI.getStartY(),
				inventoryUI.getStartY() + inventoryUI.getSizeY() * inventoryUI.getSlotSize()) )
			{
				// clicked in inventory
				if ( clickedOnGUI )
				{
					*clickedOnGUI = true;
				}
				return NULL;
			}
			if ( players[player]->gui_mode == GUI_MODE_SHOP )
			{
				int x1 = xres / 2 - SHOPWINDOW_SIZEX / 2, x2 = xres / 2 + SHOPWINDOW_SIZEX / 2;
				int y1 = yres / 2 - SHOPWINDOW_SIZEY / 2, y2 = yres / 2 + SHOPWINDOW_SIZEY / 2;
				if (mouseInBounds(player, x1, x2, y1, y2))
				{
					if ( clickedOnGUI )
					{
						*clickedOnGUI = true;
					}
					return NULL;
				}
			}
		}
		else if ( players[player]->gui_mode == GUI_MODE_MAGIC)
		{
			if (magic_GUI_state == 0)
			{
				//Right, now calculate the spell list's height (the same way it calculates it for itself).
				int height = spell_list_titlebar_bmp->h;
				int numspells = 0;
				node_t* node;
				for (node = players[player]->magic.spellList.first; node != NULL; node = node->next)
				{
					numspells++;
				}
				int maxSpellsOnscreen = yres / spell_list_gui_slot_bmp->h;
				numspells = std::min(numspells, maxSpellsOnscreen);
				height += numspells * spell_list_gui_slot_bmp->h;
				int spelllist_y = 0 + ((yres / 2) - (height / 2)) + magicspell_list_offset_x;

				if (mouseInBounds(player, MAGICSPELL_LIST_X, MAGICSPELL_LIST_X + spell_list_titlebar_bmp->w, spelllist_y, spelllist_y + height))
				{
					if ( clickedOnGUI )
					{
						*clickedOnGUI = true;
					}
					return NULL;
				}
			}
		}
		SDL_Rect& interfaceCharacterSheet = players[player]->characterSheet.characterSheetBox;
		SDL_Rect& interfaceMessageStatusBar = players[player]->statusBarUI.messageStatusBarBox;
		SDL_Rect& interfaceSkillsSheet = players[player]->characterSheet.skillsSheetBox;
		SDL_Rect& interfacePartySheet = players[player]->characterSheet.partySheetBox;
		SDL_Rect& interfaceStatsSheet = players[player]->characterSheet.statsSheetBox;
		if ( mouseInBounds(player, interfaceCharacterSheet.x, interfaceCharacterSheet.x + interfaceCharacterSheet.w,
			interfaceCharacterSheet.y, interfaceCharacterSheet.y + interfaceCharacterSheet.h) )   // character sheet
		{
			if ( clickedOnGUI )
			{
				*clickedOnGUI = true;
			}
			return NULL;
		}
		if ( mouseInBounds(player, interfaceStatsSheet.x, interfaceStatsSheet.x + interfaceStatsSheet.w,
			interfaceStatsSheet.y, interfaceStatsSheet.y + interfaceStatsSheet.h) )
		{
			if ( clickedOnGUI )
			{
				*clickedOnGUI = true;
			}
			return NULL;
		}
		if ( !hide_statusbar &&
			mouseInBounds(player, interfaceMessageStatusBar.x, interfaceMessageStatusBar.x + interfaceMessageStatusBar.w,
				interfaceMessageStatusBar.y, interfaceMessageStatusBar.y + interfaceMessageStatusBar.h) ) // bottom message log
		{
			if ( clickedOnGUI )
			{
				*clickedOnGUI = true;
			}
			return NULL;
		}

		// ui code taken from drawSkillsSheet() and drawPartySheet().
		if ( players[player]->characterSheet.proficienciesPage == 0 )
		{
			if ( mouseInBounds(player, interfaceSkillsSheet.x, interfaceSkillsSheet.x + interfaceSkillsSheet.w,
				interfaceSkillsSheet.y, interfaceSkillsSheet.y + interfaceSkillsSheet.h) )
			{
				if ( clickedOnGUI )
				{
					*clickedOnGUI = true;
				}
				return NULL;
			}
		}
		else
		{
			if ( mouseInBounds(player, interfacePartySheet.x, interfacePartySheet.x + interfacePartySheet.w,
				interfacePartySheet.y, interfacePartySheet.y + interfacePartySheet.h) )
			{
				if ( clickedOnGUI )
				{
					*clickedOnGUI = true;
				}
				return NULL;
			}
		}

		if ( mouseInsidePlayerInventory(player) || mouseInsidePlayerHotbar(player) )
		{
			if ( clickedOnGUI )
			{
				*clickedOnGUI = true;
			}
			return NULL;
		}

		if ( softwaremode )
		{
			return clickmap[my + mx * (camera.winy + camera.winh)];
		}
		else
		{
			if ( players[player]->worldUI.isEnabled() )
			{
				uidnum = 0;
			}
			else
			{
				uidnum = GO_GetPixelU32(mx, yres - my, cameras[player]);
			}
			//messagePlayer(0, "first: %d %d", uidnum, selectedEntityGimpTimer[player]);
		}
	}
	else
	{
		if ( softwaremode )
		{
			return clickmap[
				(cameras[player].winy + cameras[player].winh / 2) 
				+ (cameras[player].winx + (cameras[player].winw / 2) * (cameras[player].winy + (cameras[player].winh / 2) * 2))];
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
	}

	Entity* entity = uidToEntity(uidnum);
	if ( players[player]->worldUI.isEnabled() )
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
				if ( tooltip->worldTooltipRequiresButtonHeld == 1 )
				{
					if ( inputs.bControllerInputHeld(player, INJOY_GAME_USE) || *inputPressedForPlayer(player, impulses[IN_USE]) )
					{
						entity = uidToEntity(tooltip->parent);
					}
				}
				else
				{
					entity = uidToEntity(tooltip->parent);
				}
				break;
			}
		}
		if ( !entity )
		{
			// clear the button input if we missed a tooltip, otherwise it'll keep retrying (or pre-fire a button held)
			inputs.controllerClearInput(player, INJOY_GAME_USE);
			*inputPressedForPlayer(player, impulses[IN_USE]) = 0;
		}
	}

	if ( !entity && !mute_player_monster_sounds && !clickCheckOverride )
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
					if ( rand() % 4 == 0 )
					{
						sfx = 329;
					}
					else
					{
						sfx = 322 + rand() % 3;
					}
					break;
				case GOATMAN:
					sfx = 332 + rand() % 2;
					break;
				case INSECTOID:
					sfx = 291 + rand() % 4;
					break;
				case GOBLIN:
					sfx = 60 + rand() % 3;
					break;
				case AUTOMATON:
					sfx = 257 + rand() % 2;
					break;
				case INCUBUS:
					sfx = 276 + rand() % 3;
					break;
				case RAT:
					sfx = 29;
					break;
				case TROLL:
					if ( rand() % 3 == 0 )
					{
						sfx = 79;
					}
					break;
				case SPIDER:
					if ( rand() % 3 == 2 )
					{
						sfx = 235;
					}
					else
					{
						sfx = 230 + rand() % 2;
					}
					break;
				case CREATURE_IMP:
					sfx = 198 + rand() % 3;
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

	// pixel processing (opengl only)
	if ( softwaremode == false)
	{
		return entity;
	}
	else
	{
		return NULL;
	}
}

/*-------------------------------------------------------------------------------

	entityInsideTile

	checks whether an entity is intersecting an impassible tile

-------------------------------------------------------------------------------*/

bool entityInsideTile(Entity* entity, int x, int y, int z, bool checkSafeTiles)
{
	if ( x < 0 || x >= map.width || y < 0 || y >= map.height || z < 0 || z >= MAPLAYERS )
	{
		return false;
	}
	if ( entity->x + entity->sizex >= x << 4 )
	{
		if ( entity->x - entity->sizex < (x + 1) << 4 )
		{
			if ( entity->y + entity->sizey >= y << 4 )
			{
				if ( entity->y - entity->sizey < (y + 1) << 4 )
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
							return true;
						}
						else if ( checkSafeTiles && map.tiles[z + y * MAPLAYERS + x * MAPLAYERS * map.height] )
						{
							return true;
						}
						bool isMonster = false;
						if ( entity )
							if ( entity->behavior == &actMonster )
							{
								isMonster = true;
							}
						if ( (swimmingtiles[map.tiles[z + y * MAPLAYERS + x * MAPLAYERS * map.height]] || lavatiles[map.tiles[z + y * MAPLAYERS + x * MAPLAYERS * map.height]] )
							&& isMonster )
						{
							return true;
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
	node_t* node;
	int z;
	int x, y;
	#ifdef __ARM_NEON__
	int32x2_t xy = vcvt_s32_f32(vmul_n_f32(vld1_f32(&entity->x), 1.0f/16.0f));
	x = xy[0];
	y = xy[1];
	#else
	x = (long)floor(entity->x / 16);
	y = (long)floor(entity->y / 16);
	#endif
	// test against the map
	for ( z = 0; z < MAPLAYERS; ++z )
	{
		if ( entityInsideTile(entity, x, y, z) )
		{
			return true;
		}
	}

	// test against entities
	std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(entity, 2);
	for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end(); ++it )
	{
		list_t* currentList = *it;
		for ( node = currentList->first; node != nullptr; node = node->next )
		{
			Entity* testEntity = (Entity*)node->element;
			if ( testEntity == entity || testEntity->flags[PASSABLE] )
			{
				continue;
			}
			if ( entityInsideEntity(entity, testEntity) )
			{
				return true;
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
	if ( my )
	{
		if ( my->behavior != &actPlayer && my->behavior != &actMonster )
		{
			levitating = true;
		}
	}

	long ymin = floor((ty - my->sizey)/16), ymax = floor((ty + my->sizey)/16);
	long xmin = floor((tx - my->sizex)/16), xmax = floor((tx + my->sizex)/16);
	const real_t tymin = ty - my->sizey, tymax = ty + my->sizey;
	const real_t txmin = tx - my->sizex, txmax = tx + my->sizex;
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
					|| ((swimmingtiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]] || lavatiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]])
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
	std::vector<list_t*> entLists;
	if ( multiplayer == CLIENT )
	{
		entLists.push_back(map.entities); // clients use old map.entities method
	}
	else
	{
		entLists = TileEntityList.getEntitiesWithinRadius(static_cast<int>(tx) >> 4, static_cast<int>(ty) >> 4, 2);
	}
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
			if ( entity->flags[PASSABLE] )
			{
				if ( my->behavior == &actBoulder && entity->sprite == 886 )
				{
					// 886 is gyrobot, as they are passable, force collision here.
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
			if ( (my->behavior == &actMonster || my->behavior == &actBoulder) && entity->behavior == &actDoorFrame )
			{
				continue;    // monsters don't have hard collision with door frames
			}
			Stat* myStats = stats; //my->getStats();	//SEB <<<
			Stat* yourStats = entity->getStats();
			if ( my->behavior == &actPlayer && entity->behavior == &actPlayer )
			{
				continue;
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
			const real_t eymin = entity->y - entity->sizey, eymax = entity->y + entity->sizey;
			const real_t exmin = entity->x - entity->sizex, exmax = entity->x + entity->sizex;
			if ( (entity->sizex > 0) && ((txmin >= exmin && txmin < exmax) || (txmax >= exmin && txmax < exmax) || (txmin <= exmin && txmax > exmax)) )
			{
				if ( (entity->sizey > 0) && ((tymin >= eymin && tymin < eymax) || (tymax >= eymin && tymax < eymax) || (tymin <= eymin && tymax > eymax)) )
				{
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
									messagePlayer(hit.entity->skill[2], language[590]); // "You suddenly catch fire!"
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
									messagePlayer(hit.entity->skill[2], language[590]); // "You suddenly catch fire!"
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

	int originx = static_cast<int>(my->x) >> 4;
	int originy = static_cast<int>(my->y) >> 4;
	std::vector<list_t*> entLists; // stores the possible entities to look through depending on the quadrant.
	// start search from 1 tile behind facing direction in x/y position, extending to the edge of the map in the facing direction.

	if ( multiplayer == CLIENT )
	{
		entLists.push_back(map.entities); // default to old map.entities if client (if they ever call this function...)
	}

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

	for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end(); ++it )
	{
		list_t* currentList = *it;
		for ( node = currentList->first; node != nullptr; node = node->next )
		{
			Entity* entity = (Entity*)node->element;
			if ( (entity != target && target != nullptr) || entity->flags[PASSABLE] || entity == my 
				|| (entities && 
						( (!entity->flags[BLOCKSIGHT] && entity->behavior != &actMonster) 
							|| (entity->behavior == &actMonster && (entity->flags[INVISIBLE] && entity->sprite != 889) )
						)
					) 
				)
			{
				// if entities == 1, then ignore entities that block sight.
				// 16/11/19 - added exception to monsters. if monster, use the INVISIBLE flag to skip checking.
				// 889 is dummybot "invisible" AI entity. so it's invisible, need to make it shown here.
				continue;
			}
			if ( entity->behavior == &actParticleTimer )
			{
				continue;
			}

			if ( quadrant == 2 || quadrant == 4 )
			{
				// upper right and lower left
				real_t upperX = entity->x + entity->sizex;
				real_t upperY = entity->y - entity->sizey;
				real_t lowerX = entity->x - entity->sizex;
				real_t lowerY = entity->y + entity->sizey;
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
				real_t upperX = entity->x - entity->sizex;
				real_t upperY = entity->y - entity->sizey;
				real_t lowerX = entity->x + entity->sizex;
				real_t lowerY = entity->y + entity->sizey;
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

	if ( my )
	{
		Stat* stats = my->getStats();
		if ( stats )
		{
			if ( stats->type == DEVIL )
			{
				ground = false;
			}
			else if ( stats->type == SENTRYBOT || stats->type == SPELLBOT )
			{
				ground = false;
			}
		}
	}

	Entity* entity = findEntityInLine(my, x1, y1, angle, entities, NULL);

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
			bool isMonster = false;
			if ( my )
				if ( my->behavior == &actMonster )
				{
					isMonster = true;
				}
			if ( !map.tiles[index] 
				|| ((swimmingtiles[map.tiles[index]] || lavatiles[map.tiles[index]]) && isMonster) )
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
			//if ( entity->behavior == &actMonster && entities != 0 )
			//{
			//	Entity* particle = spawnMagicParticle(my);
			//	particle->sprite = 576;
			//	particle->x = ix;
			//	particle->y = iy;
			//	particle->z = 0;

			//	particle = spawnMagicParticle(my);
			//	particle->sprite = 942;
			//	particle->x = entity->x + entity->sizex;
			//	particle->y = entity->y + entity->sizey;
			//	particle->z = 0;

			//	particle = spawnMagicParticle(my);
			//	particle->sprite = 942;
			//	particle->x = entity->x - entity->sizex;
			//	particle->y = entity->y + entity->sizey;
			//	particle->z = 0;

			//	particle = spawnMagicParticle(my);
			//	particle->sprite = 942;
			//	particle->x = entity->x + entity->sizex;
			//	particle->y = entity->y - entity->sizey;
			//	particle->z = 0;

			//	particle = spawnMagicParticle(my);
			//	particle->sprite = 942;
			//	particle->x = entity->x - entity->sizex;
			//	particle->y = entity->y - entity->sizey;
			//	particle->z = 0;
			//}

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

real_t lineTraceTarget( Entity* my, real_t x1, real_t y1, real_t angle, real_t range, int entities, bool ground, Entity* target )
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

	Entity* entity = findEntityInLine(my, x1, y1, angle, entities, target);

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
			bool isMonster = false;
			if ( my )
				if ( my->behavior == &actMonster )
				{
					isMonster = true;
				}
			if ( !map.tiles[index] 
				|| ((swimmingtiles[map.tiles[index]] || lavatiles[map.tiles[index]]) && isMonster) )
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

int checkObstacle(long x, long y, Entity* my, Entity* target)
{
	node_t* node;
	Entity* entity;
	Stat* stats;
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
			if (map.tiles[OBSTACLELAYER + index])   // wall
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
			if ( !levitating
					&& (!map.tiles[index]
								   || ( (swimmingtiles[map.tiles[index]] || lavatiles[map.tiles[index]])
										 && isMonster) ) )   // no floor
			{
				return 1; // if there's no floor, or either water/lava then a non-levitating monster sees obstacle.
			}

			std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadius(static_cast<int>(x) >> 4, static_cast<int>(y) >> 4, 2);
			for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end(); ++it )
			{
				list_t* currentList = *it;
				for ( node = currentList->first; node != nullptr; node = node->next )
				{
					entity = (Entity*)node->element;
					//++entCheck;
					if ( entity->flags[PASSABLE] || entity == my || entity == target || entity->behavior == &actDoor )
					{
						continue;
					}
					if ( entity->behavior == &actParticleTimer && static_cast<Uint32>(entity->particleTimerTarget) == my->getUID() )
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
