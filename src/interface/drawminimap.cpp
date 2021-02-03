/*-------------------------------------------------------------------------------

	BARONY
	File: drawminimap.cpp
	Desc: contains drawMinimap()

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../main.hpp"
#include "../draw.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "../items.hpp"
#include "../sound.hpp"
#include "../menu.hpp"
#include "../player.hpp"
#include "interface.hpp"
#include "../collision.hpp"
#include "../mod_tools.hpp"

/*-------------------------------------------------------------------------------

	drawMinimap

	Draws the game's minimap in the lower right corner of the screen

-------------------------------------------------------------------------------*/

Uint32 minotaur_timer = 0;
std::vector<MinimapPing> minimapPings[MAXPLAYERS];
int minimapPingGimpTimer[MAXPLAYERS] = { 0 };
Uint32 lastMapTick = 0;
SDL_Rect minimaps[MAXPLAYERS];

Uint32 minimapColorFunc(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
	Uint32 result = 0u;
	result |= (Uint32)a << 24;
	result |= (Uint32)b << 16;
	result |= (Uint32)g <<  8;
	result |= (Uint32)r;
	return result;
}

void drawMinimap(const int player)
{
	if ( gameplayCustomManager.inUse() )
	{
		if ( CustomHelpers::isLevelPartOfSet(currentlevel, secretlevel, gameplayCustomManager.minimapDisableFloors) )
		{
			return;
		}
	}

	int numplayers = 0;
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( players[i]->isLocalPlayer() )
		{
			++numplayers;
		}
	}

	node_t* node;
	Uint32 color;
	int x, y, i;
	int minimapTotalScale = minimapScaleQuickToggle + minimapScale;
	// handle toggling scale hotkey.
	if ( !command && *inputPressedForPlayer(player, impulses[IN_MINIMAPSCALE]) || (players[player]->shootmode && inputs.bControllerInputPressed(player, INJOY_GAME_MINIMAPSCALE)) )
	{
		if ( minimapScaleQuickToggle == 3 )
		{
			minimapScaleQuickToggle = 0;
		}
		else
		{
			++minimapScaleQuickToggle;
		}
		*inputPressed(impulses[IN_MINIMAPSCALE]) = 0;
		inputs.controllerClearInput(player, INJOY_GAME_MINIMAPSCALE);
		playSound(139, 32);
	}

	if ( map.height > 64 || map.width > 64 )
	{
		int maxDimension = std::max(map.height, map.width);
		maxDimension -= 64;
		int numMinimapSizesToReduce = 0;
		while ( maxDimension > 0 )
		{
			maxDimension -= 32;
			++numMinimapSizesToReduce;
		}
		minimapTotalScale = std::max(1, minimapScale - numMinimapSizesToReduce) + minimapScaleQuickToggle;
	}

	minimaps[player].x = players[player]->camera_x2() - map.width * minimapTotalScale;
	minimaps[player].y = yres - players[player]->camera_height() - players[player]->camera_y1();
	minimaps[player].w = map.width * minimapTotalScale;
	minimaps[player].h = map.height * minimapTotalScale;

	// create a new minimap texture
	SDL_Surface* minimapSurface = SDL_CreateRGBSurface(0, map.width, map.height, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
	TempTexture* minimapTexture = new TempTexture();
	SDL_LockSurface(minimapSurface);

	// draw level
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_PROJECTION);
	glViewport(0, 0, xres, yres);
	glLoadIdentity();
	glOrtho(0, xres, 0, yres, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	for ( x = 0; x < map.width; x++ )
	{
		for ( y = 0; y < map.height; y++ )
		{
			Uint32 color = 0;
			if ( minimap[y][x] == 0 )
			{
				color = minimapColorFunc(32, 12, 0, 255 * ((100 - minimapTransparencyBackground) / 100.f));
			}
			else if ( minimap[y][x] == 1 )
			{
				color = minimapColorFunc(96, 24, 0, 255 * ((100 - minimapTransparencyForeground) / 100.f));
			}
			else if ( minimap[y][x] == 2 )
			{
				color = minimapColorFunc(192, 64, 0, 255 * ((100 - minimapTransparencyForeground) / 100.f));
			}
			else if ( minimap[y][x] == 3 )
			{
				color = minimapColorFunc(32, 32, 32, 255 * ((100 - minimapTransparencyForeground) / 100.f));
			}
			else if ( minimap[y][x] == 4 )
			{
				color = minimapColorFunc(64, 64, 64, 255 * ((100 - minimapTransparencyForeground) / 100.f));
			}
			putPixel(minimapSurface, x, y, color);
		}
	}
	SDL_UnlockSurface(minimapSurface);
	minimapTexture->load(minimapSurface, false, true);
	minimapTexture->bind();
	glColor4f(1, 1, 1, 1);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2f(minimaps[player].x, minimaps[player].y + minimaps[player].h);
	glTexCoord2f(0, 1);
	glVertex2f(minimaps[player].x, minimaps[player].y);
	glTexCoord2f(1, 1);
	glVertex2f(minimaps[player].x + minimaps[player].w, minimaps[player].y);
	glTexCoord2f(1, 0);
	glVertex2f(minimaps[player].x + minimaps[player].w, minimaps[player].y + minimaps[player].h);
	glEnd();
	glBindTexture(GL_TEXTURE_2D, 0);
	if (minimapTexture) {
		delete minimapTexture;
		minimapTexture = nullptr;
	}
	if (minimapSurface) {
		SDL_FreeSurface(minimapSurface);
		minimapSurface = nullptr;
	}
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_PROJECTION);
	glViewport(0, 0, xres, yres);
	glLoadIdentity();
	glOrtho(0, xres, 0, yres, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glBindTexture(GL_TEXTURE_2D, 0);

	// draw exits/monsters
	glBegin(GL_QUADS);
	for ( node = map.entities->first; node != NULL; node = node->next )
	{
		Entity* entity = (Entity*)node->element;
		if ( entity->sprite == 161 || (entity->sprite >= 254 && entity->sprite < 258)
			|| entity->behavior == &actCustomPortal )   // ladder or portal models
		{
			if ( entity->x >= 0 && entity->y >= 0 && entity->x < map.width << 4 && entity->y < map.height << 4 )
			{
				x = floor(entity->x / 16);
				y = floor(entity->y / 16);
				if ( minimap[y][x] || (entity->entityShowOnMap > 0 && !(entity->behavior == &actCustomPortal)) )
				{
					if ( ticks % 40 - ticks % 20 )
					{
						glColor4f( 0, 1, 1, 1 );
						//glBegin(GL_QUADS);
						glVertex2f(x * minimapTotalScale + minimaps[player].x, minimaps[player].y + minimaps[player].h - y * minimapTotalScale - minimapTotalScale);
						glVertex2f(x * minimapTotalScale + minimaps[player].x + minimapTotalScale, minimaps[player].y + minimaps[player].h - y * minimapTotalScale - minimapTotalScale);
						glVertex2f(x * minimapTotalScale + minimaps[player].x + minimapTotalScale, minimaps[player].y + minimaps[player].h - y * minimapTotalScale);
						glVertex2f(x * minimapTotalScale + minimaps[player].x, minimaps[player].y + minimaps[player].h - y * minimapTotalScale);
						//glEnd();
					}
				}
			}
		}
		else
		{
			if ( entity->skill[28] > 0 ) // mechanism
			{
				if ( entity->behavior == &actCustomPortal
					|| entity->behavior == &actTextSource
					|| entity->behavior == &actFloorDecoration )
				{
					continue;
				}
			}
			if ( entity->behavior == &actMonster && entity->monsterAllyIndex < 0 )
			{
				bool warningEffect = false;
				if ( (players[player] && players[player]->entity
					&& players[player]->entity->creatureShadowTaggedThisUid == entity->getUID())
					|| (entity->getStats() && entity->getStats()->EFFECTS[EFF_SHADOW_TAGGED]) )
				{
					warningEffect = true;
					x = floor(entity->x / 16);
					y = floor(entity->y / 16);
					glColor4f(.75, .75, .75, 1);
					//glBegin(GL_QUADS);
					glVertex2f(x * minimapTotalScale + minimaps[player].x, minimaps[player].y + minimaps[player].h - y * minimapTotalScale - minimapTotalScale);
					glVertex2f(x * minimapTotalScale + minimaps[player].x + minimapTotalScale, minimaps[player].y + minimaps[player].h - y * minimapTotalScale - minimapTotalScale);
					glVertex2f(x * minimapTotalScale + minimaps[player].x + minimapTotalScale, minimaps[player].y + minimaps[player].h - y * minimapTotalScale);
					glVertex2f(x * minimapTotalScale + minimaps[player].x, minimaps[player].y + minimaps[player].h - y * minimapTotalScale);
					//glEnd();
				}
				if ( !warningEffect 
					&& ((stats[player]->ring && stats[player]->ring->type == RING_WARNING)
						|| (entity->entityShowOnMap > 0)) )
				{
					int beatitude = 0;
					if ( stats[player]->ring && stats[player]->ring->type == RING_WARNING )
					{
						beatitude = stats[player]->ring->beatitude;
						// invert for succ/incubus
						if ( beatitude < 0 && shouldInvertEquipmentBeatitude(stats[player]) )
						{
							beatitude = abs(stats[player]->ring->beatitude);
						}
					}

					bool doEffect = false;
					if ( entity->entityShowOnMap > 0 )
					{
						doEffect = true;
					}
					else if ( stats[player]->ring && players[player] && players[player]->entity
						&& entityDist(players[player]->entity, entity) < 16.0 * std::max(3, (11 + 5 * beatitude)) )
					{
						doEffect = true;
					}
					if ( doEffect )
					{
						x = floor(entity->x / 16);
						y = floor(entity->y / 16);
						glColor4f(0.75, 0.5, 0.75, 1);
						//glBegin(GL_QUADS);
						glVertex2f(x * minimapTotalScale + minimaps[player].x, minimaps[player].y + minimaps[player].h - y * minimapTotalScale - minimapTotalScale);
						glVertex2f(x * minimapTotalScale + minimaps[player].x + minimapTotalScale, minimaps[player].y + minimaps[player].h - y * minimapTotalScale - minimapTotalScale);
						glVertex2f(x * minimapTotalScale + minimaps[player].x + minimapTotalScale, minimaps[player].y + minimaps[player].h - y * minimapTotalScale);
						glVertex2f(x * minimapTotalScale + minimaps[player].x, minimaps[player].y + minimaps[player].h - y * minimapTotalScale);
						//glEnd();
						warningEffect = true;
					}
				}
				if ( !warningEffect && stats[player]->shoes != NULL )
				{
					if ( stats[player]->shoes->type == ARTIFACT_BOOTS )
					{
						if ( (abs(entity->vel_x) > 0.1 || abs(entity->vel_y) > 0.1)
							&& players[player] && players[player]->entity
							&& entityDist(players[player]->entity, entity) < 16.0 * 20 )
						{
							entity->entityShowOnMap = std::max(entity->entityShowOnMap, TICKS_PER_SECOND * 5);
							x = floor(entity->x / 16);
							y = floor(entity->y / 16);
							glColor4f(0.75, 0.5, 0.75, 1);
							//glBegin(GL_QUADS);
							glVertex2f(x * minimapTotalScale + minimaps[player].x, minimaps[player].y + minimaps[player].h - y * minimapTotalScale - minimapTotalScale);
							glVertex2f(x * minimapTotalScale + minimaps[player].x + minimapTotalScale, minimaps[player].y + minimaps[player].h - y * minimapTotalScale - minimapTotalScale);
							glVertex2f(x * minimapTotalScale + minimaps[player].x + minimapTotalScale, minimaps[player].y + minimaps[player].h - y * minimapTotalScale);
							glVertex2f(x * minimapTotalScale + minimaps[player].x, minimaps[player].y + minimaps[player].h - y * minimapTotalScale);
							//glEnd();
						}
					}
				}
			}
			else if ( entity->isBoulderSprite() )     // boulder.vox
			{
				x = std::min<int>(std::max<int>(0, entity->x / 16), map.width - 1);
				y = std::min<int>(std::max<int>(0, entity->y / 16), map.height - 1);
				if ( minimap[y][x] == 1 || minimap[y][x] == 2 )
				{
					glColor4f( 192 / 255.f, 64 / 255.f, 0 / 255.f, 1 );
					//glBegin(GL_QUADS);
					glVertex2f(x * minimapTotalScale + minimaps[player].x, minimaps[player].y + minimaps[player].h - y * minimapTotalScale - minimapTotalScale);
					glVertex2f(x * minimapTotalScale + minimaps[player].x + minimapTotalScale, minimaps[player].y + minimaps[player].h - y * minimapTotalScale - minimapTotalScale);
					glVertex2f(x * minimapTotalScale + minimaps[player].x + minimapTotalScale, minimaps[player].y + minimaps[player].h - y * minimapTotalScale);
					glVertex2f(x * minimapTotalScale + minimaps[player].x, minimaps[player].y + minimaps[player].h - y * minimapTotalScale);
					//glEnd();
				}
			}
			else if ( entity->behavior == &actItem && entity->itemShowOnMap == 1 )
			{
				x = floor(entity->x / 16);
				y = floor(entity->y / 16);
				if ( ticks % 40 - ticks % 20 )
				{
					glColor4f(240 / 255.f, 228 / 255.f, 66 / 255.f, 1); // yellow
					//glBegin(GL_QUADS);
					glVertex2f(x * minimapTotalScale + minimaps[player].x, minimaps[player].y + minimaps[player].h - y * minimapTotalScale - minimapTotalScale);
					glVertex2f(x * minimapTotalScale + minimaps[player].x + minimapTotalScale, minimaps[player].y + minimaps[player].h - y * minimapTotalScale - minimapTotalScale);
					glVertex2f(x * minimapTotalScale + minimaps[player].x + minimapTotalScale, minimaps[player].y + minimaps[player].h - y * minimapTotalScale);
					glVertex2f(x * minimapTotalScale + minimaps[player].x, minimaps[player].y + minimaps[player].h - y * minimapTotalScale);
				}
			}
			else if ( entity->entityShowOnMap > 0 )
			{
				x = floor(entity->x / 16);
				y = floor(entity->y / 16);
				if ( ticks % 40 - ticks % 20 )
				{
					glColor4f(255 / 255.f, 168 / 255.f, 200 / 255.f, 1); // pink
					glVertex2f(x * minimapTotalScale + minimaps[player].x, minimaps[player].y + minimaps[player].h - y * minimapTotalScale - minimapTotalScale);
					glVertex2f(x * minimapTotalScale + minimaps[player].x + minimapTotalScale, minimaps[player].y + minimaps[player].h - y * minimapTotalScale - minimapTotalScale);
					glVertex2f(x * minimapTotalScale + minimaps[player].x + minimapTotalScale, minimaps[player].y + minimaps[player].h - y * minimapTotalScale);
					glVertex2f(x * minimapTotalScale + minimaps[player].x, minimaps[player].y + minimaps[player].h - y * minimapTotalScale);
				}
			}
		}
		if ( entity->entityShowOnMap > 0 && lastMapTick != ticks ) 
		{
			// only decrease the entities' shown duration when the global game timer passes a tick
			// (drawMinimap doesn't follow game tick intervals)
			--entity->entityShowOnMap;
		}
	}
	lastMapTick = ticks;
	glEnd();

	// draw player pings
	if ( !minimapPings[player].empty() )
	{
		for ( std::vector<MinimapPing>::iterator it = minimapPings[player].begin(); it != minimapPings[player].end();)
		{
			MinimapPing ping = *it;
			switch ( ping.player )
			{
				case 0:
					color = SDL_MapRGB(mainsurface->format, 64, 255, 64); // green
					break;
				case 1:
					color = SDL_MapRGB(mainsurface->format, 86, 180, 233); // sky blue
					break;
				case 2:
					color = SDL_MapRGB(mainsurface->format, 240, 228, 66); // yellow
					break;
				case 3:
					color = SDL_MapRGB(mainsurface->format, 204, 121, 167); // pink
					break;
				default:
					color = SDL_MapRGB(mainsurface->format, 192, 192, 192); // grey
					break;
			}

			int aliveTime = ticks - ping.tickStart;
			if ( aliveTime < TICKS_PER_SECOND * 2.5 ) // 2.5 second duration.
			{
				if ( (aliveTime < TICKS_PER_SECOND && (aliveTime % 10 < 5)) || aliveTime >= TICKS_PER_SECOND || ping.radiusPing )
				{
					// draw the ping blinking every 5 ticks if less than 1 second lifetime, otherwise constantly draw.
					x = minimaps[player].x + ping.x * minimapTotalScale + minimapTotalScale / 2;
					y = yres - (minimaps[player].y + minimaps[player].h) + ping.y * minimapTotalScale + minimapTotalScale / 2;
					int alpha = 255;
					if ( ping.radiusPing )
					{
						alpha = 50;
					}
					if ( aliveTime >= TICKS_PER_SECOND * 2 )
					{
						// start fading ping after 2 seconds, lasting 0.5 seconds.
						real_t alphafade = 1 - (aliveTime - TICKS_PER_SECOND * 2) / static_cast<real_t>(TICKS_PER_SECOND * 0.5);
						alpha = std::max(static_cast<int>(alphafade * alpha), 0);
					}
					// draw a circle
					if ( ping.radiusPing )
					{
						int radius = 3 + std::min(30, aliveTime);
						radius = std::min(minimapTotalScale * 6, radius);
						drawCircle(x - 1, y - 1, std::max(radius + minimapObjectZoom, 0), color, alpha);
					}
					else
					{
						drawCircle(x - 1, y - 1, std::max(3 + minimapObjectZoom, 0), color, alpha);
					}
				}
			}


			// prune old pings > 2.5 seconds
			if ( aliveTime > TICKS_PER_SECOND * 2.5 )
			{
				if ( ping.player == player )
				{
					if ( minimapPingGimpTimer[player] > TICKS_PER_SECOND / 4 )
					{
						minimapPingGimpTimer[player] = TICKS_PER_SECOND / 4; // reduce the gimp timer when one of the player's own pings fades.
					}
				}
				it = minimapPings[player].erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	// draw players and allies
	
	for ( node = map.creatures->first; node != nullptr; node = node->next )
	{
		Entity* entity = (Entity*)node->element;
		int drawMonsterAlly = -1;
		int foundplayer = -1;
		if ( entity->behavior == &actPlayer )
		{
			foundplayer = entity->skill[2];
		}
		else if ( entity->behavior == &actMonster )
		{
			if ( entity->monsterAllyIndex >= 0 )
			{
				drawMonsterAlly = entity->monsterAllyIndex;
			}
		}
		if ( drawMonsterAlly >= 0 || foundplayer >= 0 )
		{
			// my player = green, other players = blue
			if ( foundplayer >= 0 )
			{
				switch ( foundplayer )
				{
					case 0:
						color = SDL_MapRGB(mainsurface->format, 64, 255, 64); // green
						break;
					case 1:
						color = SDL_MapRGB(mainsurface->format, 86, 180, 233); // sky blue
						break;
					case 2:
						color = SDL_MapRGB(mainsurface->format, 240, 228, 66); // yellow
						break;
					case 3:
						color = SDL_MapRGB(mainsurface->format, 204, 121, 167); // pink
						break;
					default:
						color = SDL_MapRGB(mainsurface->format, 192, 192, 192); // grey
						break;
				}
				if ( players[player] && players[player]->entity
					&& players[player]->entity->creatureShadowTaggedThisUid == entity->getUID() )
				{
					color = SDL_MapRGB(mainsurface->format, 192, 192, 192); // grey
				}
				//color = SDL_MapRGB(mainsurface->format, 0, 192, 0);
			}
			else
			{
				switch ( drawMonsterAlly )
				{
					case 0:
						color = SDL_MapRGB(mainsurface->format, 64, 255, 64); // green
						break;
					case 1:
						color = SDL_MapRGB(mainsurface->format, 86, 180, 233); // sky blue
						break;
					case 2:
						color = SDL_MapRGB(mainsurface->format, 240, 228, 66); // yellow
						break;
					case 3:
						color = SDL_MapRGB(mainsurface->format, 204, 121, 167); // pink
						break;
					default:
						color = SDL_MapRGB(mainsurface->format, 192, 192, 192); // grey
						break;
				}
				if ( players[player] && players[player]->entity
					&& players[player]->entity->creatureShadowTaggedThisUid == entity->getUID() )
				{
					color = SDL_MapRGB(mainsurface->format, 192, 192, 192); // grey
				}
			}

			// draw the first pixel
			int oldx = minimaps[player].x + (int)(entity->x / (16.f / minimapTotalScale));
			int oldy = minimaps[player].y + minimaps[player].h - (int)(entity->y / (16.f / minimapTotalScale));
			x = oldx;
			y = oldy;
			if ( foundplayer >= 0 )
			{
				if ( softwaremode )
				{
					//SPG_Pixel(screen,(int)(players[c]->x/16)+564+x+xres/2-(status_bmp->w/2),(int)(players[c]->y/16)+yres-71+y,color); //TODO: NOT a PLAYERSWAP
				}
				else
				{
					glColor4f(((Uint8)(color >> mainsurface->format->Rshift)) / 255.f, ((Uint8)(color >> mainsurface->format->Gshift)) / 255.f, ((Uint8)(color >> mainsurface->format->Bshift)) / 255.f, 1);
					glBegin(GL_POINTS);
					glVertex2f( x, y );
					glEnd();
				}
			}

			// draw a circle
			if ( foundplayer >= 0 )
			{
				drawCircle(x - 1, yres - y - 1, std::max(3 + minimapObjectZoom, 0), color, 255);
			}
			else
			{
				drawCircle(x - 1, yres - y - 1, std::max(2 + minimapObjectZoom, 0), color, 128);
			}

			x = 0;
			y = 0;
			if ( foundplayer >= 0 )
			{
				for ( i = 0; i < 4 + minimapObjectZoom; ++i )
				{
					// move forward
					if ( cos(entity->yaw) > .4 )
					{
						x++;
					}
					else if ( cos(entity->yaw) < -.4 )
					{
						x--;
					}
					if ( sin(entity->yaw) > .4 )
					{
						y++;
					}
					else if ( sin(entity->yaw) < -.4 )
					{
						y--;
					}

					// get brighter color shade
					/*if ( foundplayer )
					{
						color = SDL_MapRGB(mainsurface->format, 64, 255, 64);
					}
					else
					{
						color = SDL_MapRGB(mainsurface->format, 64, 64, 255);
					}*/

					// draw the pixel
					if ( softwaremode )
					{
						//SPG_Pixel(screen,(int)(players[c]->x/16)+564+x+xres/2-(status_bmp->w/2),(int)(players[c]->y/16)+yres-71+y,color); //TODO: NOT a PLAYERSWAP
					}
					else
					{
						glColor4f(((Uint8)(color >> mainsurface->format->Rshift)) / 255.f, ((Uint8)(color >> mainsurface->format->Gshift)) / 255.f, ((Uint8)(color >> mainsurface->format->Bshift)) / 255.f, 1);
						glBegin(GL_POINTS);
						glVertex2f(oldx + x, oldy - y );
						glEnd();
					}
				}
			}
		}
	}

	// draw minotaur
	if (players[player] == nullptr)
	{
		return;
	}
	for ( node = map.creatures->first; node != nullptr; node = node->next )
	{
		Entity* entity = (Entity*)node->element;
		if ( entity->sprite == 239 )
		{
			if ( ticks % 120 - ticks % 60 )
			{
				if ( !minotaur_timer )
				{
					playSound(116, 64);
				}
				minotaur_timer = 1;
				if ( !colorblind )
				{
					color = SDL_MapRGB(mainsurface->format, 192, 0, 0);
				}
				else
				{
					color = SDL_MapRGB(mainsurface->format, 0, 192, 192);
				}

				// draw the first pixel
				int oldx = minimaps[player].x + (int)(entity->x / (16.f / minimapTotalScale));
				int oldy = minimaps[player].y + minimaps[player].h - (int)(entity->y / (16.f / minimapTotalScale));
				x = oldx;
				y = oldy;
				if ( softwaremode )
				{
					//SPG_Pixel(screen,(int)(players[c]->x/16)+564+x+xres/2-(status_bmp->w/2),(int)(players[c]->y/16)+yres-71+y,color); //TODO: NOT a PLAYERSWAP
				}
				else
				{
					glColor4f(((Uint8)(color >> 16)) / 255.f, ((Uint8)(color >> 8)) / 255.f, ((Uint8)(color)) / 255.f, 1);
					glBegin(GL_POINTS);
					glVertex2f( x, y );
					glEnd();
				}

				// draw a circle
				drawCircle(x - 1, yres - y - 1, std::max(3 + minimapObjectZoom, 0), color, 255);

				x = 0;
				y = 0;
				for ( i = 0; i < 4 + minimapObjectZoom; ++i )
				{
					// move forward
					if ( cos(entity->yaw) > .4 )
					{
						x++;
					}
					else if ( cos(entity->yaw) < -.4 )
					{
						x--;
					}
					if ( sin(entity->yaw) > .4 )
					{
						y++;
					}
					else if ( sin(entity->yaw) < -.4 )
					{
						y--;
					}

					// get brighter color shade
					if ( !colorblind )
					{
						color = SDL_MapRGB(mainsurface->format, 255, 64, 64);
					}
					else
					{
						color = SDL_MapRGB(mainsurface->format, 64, 255, 255);
					}

					// draw the pixel
					if ( softwaremode )
					{
						//SPG_Pixel(screen,(int)(players[c]->x/16)+564+x+xres/2-(status_bmp->w/2),(int)(players[c]->y/16)+yres-71+y,color); //TODO: NOT a PLAYERSWAR
					}
					else
					{
						glColor4f(((Uint8)(color >> 16)) / 255.f, ((Uint8)(color >> 8)) / 255.f, ((Uint8)(color)) / 255.f, 1);
						glBegin(GL_POINTS);
						glVertex2f( oldx + x, oldy - y );
						glEnd();
					}
				}
			}
			else
			{
				minotaur_timer = 0;
			}
		}
	}
}

void minimapPingAdd(const int srcPlayer, const int destPlayer, MinimapPing newPing)
{
	int numPlayerPings = 0;

	if ( !players[destPlayer]->isLocalPlayer() )
	{
		return;
	}
	if ( !minimapPings[destPlayer].empty() )
	{
		for ( std::vector<MinimapPing>::iterator it = minimapPings[destPlayer].begin(); it != minimapPings[destPlayer].end();)
		{
			MinimapPing ping = *it;
			if ( ping.player == newPing.player && !newPing.radiusPing )
			{
				++numPlayerPings;
				// prune pings if too many by same player.
				if ( numPlayerPings > 3 )
				{
					if ( ping.player == srcPlayer )
					{
						// this is the player creating the sound source.
						minimapPingGimpTimer[srcPlayer] = TICKS_PER_SECOND * 3; // 3 second penalty for spam.
					}
					it = minimapPings[destPlayer].erase(it);
					continue;
				}
			}
			++it;
		}
	}
	if ( !minimapPingMute && !newPing.radiusPing )
	{
		if ( static_cast<Sint8>(newPing.player) == -1 
			|| (newPing.player == destPlayer) 
			|| (newPing.player >= 0 && newPing.player < MAXPLAYERS && !players[newPing.player]->isLocalPlayer()) )
		{
			// play once? splitscreen host will satisfy newPing.player == destPlayer
			// remote client sending a ping will satisfy !players[newPing.player]->isLocalPlayer()
			playSound(399, 64); 
		}
	}
	minimapPings[destPlayer].insert(minimapPings[destPlayer].begin(), newPing);
}
