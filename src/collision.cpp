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

Entity* entityClicked()
{
	Uint32 uidnum;
	GLubyte pixel[4];

	if ( !(*inputPressed(impulses[IN_USE])) && !(*inputPressed(joyimpulses[INJOY_GAME_USE])) )
	{
		return NULL;
	}
	if ( !shootmode )
	{
		if ( itemMenuOpen )
		{
			return NULL;
		}
		if ( omousex < camera.winx || omousex >= camera.winx + camera.winw || omousey < camera.winy || omousey >= camera.winy + camera.winh )
		{
			return NULL;
		}
		if (openedChest[clientnum])
			if (omousex > CHEST_INVENTORY_X && omousex < CHEST_INVENTORY_X + inventoryChest_bmp->w && omousey > CHEST_INVENTORY_Y && omousey < CHEST_INVENTORY_Y + inventoryChest_bmp->h)
			{
				return NULL;    //Click falls inside the chest inventory GUI.
			}
		if (identifygui_active)
			if (omousex > IDENTIFY_GUI_X && omousex < IDENTIFY_GUI_X + identifyGUI_img->w && omousey > IDENTIFY_GUI_Y && omousey < IDENTIFY_GUI_Y + identifyGUI_img->h)
			{
				return NULL;    //Click falls inside the identify item gui.
			}
		if (book_open)
			if (mouseInBounds(BOOK_GUI_X, BOOK_GUI_X + bookgui_img->w, BOOK_GUI_Y, BOOK_GUI_Y + bookgui_img->h))
			{
				return NULL;    //Click falls inside the book GUI.
			}
		if (gui_mode == GUI_MODE_INVENTORY || gui_mode == GUI_MODE_SHOP)
		{
			if ( gui_mode == GUI_MODE_INVENTORY )
				if (mouseInBounds(RIGHTSIDEBAR_X, RIGHTSIDEBAR_X + rightsidebar_titlebar_img->w, RIGHTSIDEBAR_Y, RIGHTSIDEBAR_Y + rightsidebar_height))
				{
					return NULL;    //Click falls inside the right sidebar.
				}
			//int x = std::max(character_bmp->w, xres/2-inventory_bmp->w/2);
			//if (mouseInBounds(x,x+inventory_bmp->w,0,inventory_bmp->h))
			//return NULL;
			if ( mouseInBounds(INVENTORY_STARTX, INVENTORY_STARTX + INVENTORY_SIZEX * INVENTORY_SLOTSIZE, INVENTORY_STARTY, INVENTORY_STARTY + INVENTORY_SIZEY * INVENTORY_SLOTSIZE) )
			{
				// clicked in inventory
				return NULL;
			}
			if ( gui_mode == GUI_MODE_SHOP )
			{
				int x1 = xres / 2 - SHOPWINDOW_SIZEX / 2, x2 = xres / 2 + SHOPWINDOW_SIZEX / 2;
				int y1 = yres / 2 - SHOPWINDOW_SIZEY / 2, y2 = yres / 2 + SHOPWINDOW_SIZEY / 2;
				if (mouseInBounds(x1, x2, y1, y2))
				{
					return NULL;
				}
			}
		}
		else if (gui_mode == GUI_MODE_MAGIC)
		{
			if (magic_GUI_state == 0)
			{
				//Right, now calculate the spell list's height (the same way it calculates it for itself).
				int height = spell_list_titlebar_bmp->h;
				int numspells = 0;
				node_t* node;
				for (node = spellList.first; node != NULL; node = node->next)
				{
					numspells++;
				}
				int maxSpellsOnscreen = camera.winh / spell_list_gui_slot_bmp->h;
				numspells = std::min(numspells, maxSpellsOnscreen);
				height += numspells * spell_list_gui_slot_bmp->h;
				int spelllist_y = camera.winy + ((camera.winh / 2) - (height / 2)) + magicspell_list_offset_x;

				if (mouseInBounds(MAGICSPELL_LIST_X, MAGICSPELL_LIST_X + spell_list_titlebar_bmp->w, spelllist_y, spelllist_y + height))
				{
					return NULL;
				}
			}
		}
		if (mouseInBounds(0, 224, 0, 420))   // character sheet
		{
			return NULL;
		}
		int x = xres / 2 - (status_bmp->w / 2);
		if (mouseInBounds(x, x + status_bmp->w, yres - status_bmp->h, yres))
		{
			return NULL;
		}

		// ui code taken from drawSkillsSheet() and drawPartySheet().
		if ( proficienciesPage == 0 )
		{
			if ( mouseInBounds(interfaceSkillsSheet.x, interfaceSkillsSheet.x + interfaceSkillsSheet.w,
				interfaceSkillsSheet.y, interfaceSkillsSheet.y + interfaceSkillsSheet.h) )
			{
				return NULL;
			}
		}
		else
		{
			if ( mouseInBounds(interfacePartySheet.x, interfacePartySheet.x + interfacePartySheet.w,
				interfacePartySheet.y, interfacePartySheet.y + interfacePartySheet.h) )
			{
				return NULL;
			}
		}

		if ( mouseInsidePlayerInventory() || mouseInsidePlayerHotbar() )
		{
			return NULL;
		}

		if ( softwaremode )
		{
			return clickmap[omousey + omousex * yres];
		}
		else
		{
			uidnum = GO_GetPixelU32(omousex, yres - omousey);
		}
	}
	else
	{
		if ( softwaremode )
		{
			return clickmap[(yres / 2) + (xres / 2) * yres];
		}
		else
		{
			uidnum = GO_GetPixelU32(xres / 2, yres / 2);
		}
	}

	// pixel processing (opengl only)
	if ( softwaremode == false)
	{
		return uidToEntity(uidnum);
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

bool entityInsideTile(Entity* entity, int x, int y, int z)
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
						if ( !map.tiles[z + y * MAPLAYERS + x * MAPLAYERS * map.height] )
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
	for ( node = map.entities->first; node != nullptr; node = node->next )
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
		if ( my->behavior == &actMonster )
		{
			isMonster = true;
		}
	if ( isMonster && multiplayer == CLIENT )
		if ( my->sprite == 289 || my->sprite == 274 || my->sprite == 413 )   // imp and lich and cockatrice
		{
			levitating = true;
		}
	if ( my )
		if ( my->behavior != &actPlayer && my->behavior != &actMonster )
		{
			levitating = true;
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
	for (node = map.entities->first; node != nullptr; node = node->next)
	{
		entity = (Entity*)node->element;
		if ( entity == my || entity->flags[PASSABLE] || my->parent == entity->getUID() )
		{
			continue;
		}
		if ( my->behavior == &actMonster && entity->behavior == &actDoorFrame )
		{
			continue;    // monsters don't have hard collision with door frames
		}
		Stat* myStats = stats; //my->getStats();	//SEB <<<
		Stat* yourStats = entity->getStats();
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
			if ( monsterally[myStats->type][yourStats->type] )
			{
				continue;
			}
			if ( (myStats->type == HUMAN || my->flags[USERFLAG2]) && (yourStats->type == HUMAN || entity->flags[USERFLAG2]) )
			{
				continue;
			}
		}
		if ( multiplayer == CLIENT )
		{
			// fixes bug where clients can't move through humans
			if ( (entity->sprite >= 113 && entity->sprite < 118) ||
					(entity->sprite >= 125 && entity->sprite < 130) ||
					(entity->sprite >= 332 && entity->sprite < 334) ||
					(entity->sprite >= 341 && entity->sprite < 347) ||
					(entity->sprite >= 354 && entity->sprite < 360) ||
					(entity->sprite >= 367 && entity->sprite < 373) ||
					(entity->sprite >= 380 && entity->sprite < 386) ||
					entity->sprite == 217 )   // human heads
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
		if( (entity->sizex>0) && ((txmin >= exmin && txmin < exmax) || (txmax >= exmin && txmax < exmax) || (txmin <= exmin && txmax > exmax)) )
		{
			if( (entity->sizey>0) && ((tymin >= eymin && tymin < eymax) || (tymax >= eymin && tymax < eymax) || (tymin <= eymin && tymax > eymax)) )
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

	if ( angle >= PI / 2 && angle < PI )
	{
		quadrant = 1;
	}
	else if ( angle >= 0 && angle < PI / 2 )
	{
		quadrant = 2;
	}
	else if ( angle >= 3 * (PI / 2) && angle < PI * 2 )
	{
		quadrant = 3;
	}
	else
	{
		quadrant = 4;
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

	for ( node = map.entities->first; node != nullptr; node = node->next )
	{
		Entity* entity = (Entity*)node->element;
		if ( (entity != target && target != nullptr) || entity->flags[PASSABLE] || entity == my || (entities && !entity->flags[BLOCKSIGHT]) )
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

			for ( node = map.entities->first; node != nullptr; node = node->next )
			{
				entity = (Entity*)node->element;
				if ( entity->flags[PASSABLE] || entity == my || entity == target || entity->behavior == &actDoor )
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

	if ( logCheckObstacle )
	{
		++logCheckObstacleCount;
	}

	return 0;
}
