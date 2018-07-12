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

/*-------------------------------------------------------------------------------

	drawMinimap

	Draws the game's minimap in the lower right corner of the screen

-------------------------------------------------------------------------------*/

Uint32 minotaur_timer = 0;
std::vector<MinimapPing> minimapPings;
int minimapPingGimpTimer = -1;

void drawMinimap()
{
	node_t* node;
	Uint32 color;
	int x, y, i;
	int minimapTotalScale = minimapScaleQuickToggle + minimapScale;
	// handle toggling scale hotkey.
	if ( !command && *inputPressed(impulses[IN_MINIMAPSCALE]) || (shootmode && *inputPressed(joyimpulses[INJOY_GAME_MINIMAPSCALE])) )
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
		*inputPressed(joyimpulses[INJOY_GAME_MINIMAPSCALE]) = 0;
		playSound(139, 32);
	}

	// draw level
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_PROJECTION);
	glViewport(0, 0, xres, yres);
	glLoadIdentity();
	glOrtho(0, xres, 0, yres, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBegin(GL_QUADS);
	for ( x = 0; x < map.width; x++ )
	{
		for ( y = 0; y < map.height; y++ )
		{
			if ( minimap[y][x] == 0 )
			{
				glColor4f( 32 / 255.f,  12 / 255.f, 0 / 255.f, 1.f - (minimapTransparencyBackground / 100.f) );
			}
			else if ( minimap[y][x] == 1 )
			{
				glColor4f( 96 / 255.f,  24 / 255.f, 0 / 255.f, 1.f - (minimapTransparencyForeground / 100.f) );
			}
			else if ( minimap[y][x] == 2 )
			{
				glColor4f( 192 / 255.f, 64 / 255.f, 0 / 255.f, 1.f - (minimapTransparencyForeground / 100.f));
			}
			else if ( minimap[y][x] == 3 )
			{
				glColor4f( 32 / 255.f, 32 / 255.f, 32 / 255.f, 1.f - (minimapTransparencyForeground / 100.f));
			}
			else if ( minimap[y][x] == 4 )
			{
				glColor4f( 64 / 255.f, 64 / 255.f, 64 / 255.f, 1.f - (minimapTransparencyForeground / 100.f));
			}
			//glBegin(GL_QUADS);
			glVertex2f(x * minimapTotalScale + xres - map.width * minimapTotalScale, map.height * minimapTotalScale - y * minimapTotalScale - minimapTotalScale);
			glVertex2f(x * minimapTotalScale + xres - map.width * minimapTotalScale + minimapTotalScale, map.height * minimapTotalScale - y * minimapTotalScale - minimapTotalScale);
			glVertex2f(x * minimapTotalScale + xres - map.width * minimapTotalScale + minimapTotalScale, map.height * minimapTotalScale - y * minimapTotalScale);
			glVertex2f(x * minimapTotalScale + xres - map.width * minimapTotalScale, map.height * minimapTotalScale - y * minimapTotalScale);
			//glEnd();
		}
	}
	glEnd();
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
		if ( entity->sprite == 161 || (entity->sprite >= 254 && entity->sprite < 258) )   // ladder or portal models
		{
			if ( entity->x >= 0 && entity->y >= 0 && entity->x < map.width << 4 && entity->y < map.height << 4 )
			{
				x = floor(entity->x / 16);
				y = floor(entity->y / 16);
				if ( minimap[y][x] )
				{
					if ( ticks % 40 - ticks % 20 )
					{
						if ( !colorblind )
						{
							glColor4f( 1, 0, 0, 1 );
						}
						else
						{
							glColor4f( 0, 1, 1, 1 );
						}
						//glBegin(GL_QUADS);
						glVertex2f(x * minimapTotalScale + xres - map.width * minimapTotalScale, map.height * minimapTotalScale - y * minimapTotalScale - minimapTotalScale);
						glVertex2f(x * minimapTotalScale + xres - map.width * minimapTotalScale + minimapTotalScale, map.height * minimapTotalScale - y * minimapTotalScale - minimapTotalScale);
						glVertex2f(x * minimapTotalScale + xres - map.width * minimapTotalScale + minimapTotalScale, map.height * minimapTotalScale - y * minimapTotalScale);
						glVertex2f(x * minimapTotalScale + xres - map.width * minimapTotalScale, map.height * minimapTotalScale - y * minimapTotalScale);
						//glEnd();
					}
				}
			}
		}
		else
		{
			if ( entity->behavior == &actMonster && entity->monsterAllyIndex < 0 )
			{
				bool warningEffect = false;
				if ( stats[clientnum]->ring != NULL )
				{
					if ( stats[clientnum]->ring->type == RING_WARNING )
					{
						x = floor(entity->x / 16);
						y = floor(entity->y / 16);
						glColor4f( .5, .25, .5, 1 );
						//glBegin(GL_QUADS);
						glVertex2f(x * minimapTotalScale + xres - map.width * minimapTotalScale, map.height * minimapTotalScale - y * minimapTotalScale - minimapTotalScale);
						glVertex2f(x * minimapTotalScale + xres - map.width * minimapTotalScale + minimapTotalScale, map.height * minimapTotalScale - y * minimapTotalScale - minimapTotalScale);
						glVertex2f(x * minimapTotalScale + xres - map.width * minimapTotalScale + minimapTotalScale, map.height * minimapTotalScale - y * minimapTotalScale);
						glVertex2f(x * minimapTotalScale + xres - map.width * minimapTotalScale, map.height * minimapTotalScale - y * minimapTotalScale);
						//glEnd();
						warningEffect = true;
					}
				}
				if ( !warningEffect && stats[clientnum]->shoes != NULL )
				{
					if ( stats[clientnum]->shoes->type == ARTIFACT_BOOTS )
					{
						x = floor(entity->x / 16);
						y = floor(entity->y / 16);
						glColor4f(.5, .25, .5, 1);
						//glBegin(GL_QUADS);
						glVertex2f(x * minimapTotalScale + xres - map.width * minimapTotalScale, map.height * minimapTotalScale - y * minimapTotalScale - minimapTotalScale);
						glVertex2f(x * minimapTotalScale + xres - map.width * minimapTotalScale + minimapTotalScale, map.height * minimapTotalScale - y * minimapTotalScale - minimapTotalScale);
						glVertex2f(x * minimapTotalScale + xres - map.width * minimapTotalScale + minimapTotalScale, map.height * minimapTotalScale - y * minimapTotalScale);
						glVertex2f(x * minimapTotalScale + xres - map.width * minimapTotalScale, map.height * minimapTotalScale - y * minimapTotalScale);
						//glEnd();
					}
				}
			}
			else if ( entity->sprite == 245 )     // boulder.vox
			{
				x = std::min<int>(std::max<int>(0, entity->x / 16), map.width - 1);
				y = std::min<int>(std::max<int>(0, entity->y / 16), map.height - 1);
				if ( minimap[y][x] == 1 || minimap[y][x] == 2 )
				{
					glColor4f( 192 / 255.f, 64 / 255.f, 0 / 255.f, 1 );
					//glBegin(GL_QUADS);
					glVertex2f(x * minimapTotalScale + xres - map.width * minimapTotalScale, map.height * minimapTotalScale - y * minimapTotalScale - minimapTotalScale);
					glVertex2f(x * minimapTotalScale + xres - map.width * minimapTotalScale + minimapTotalScale, map.height * minimapTotalScale - y * minimapTotalScale - minimapTotalScale);
					glVertex2f(x * minimapTotalScale + xres - map.width * minimapTotalScale + minimapTotalScale, map.height * minimapTotalScale - y * minimapTotalScale);
					glVertex2f(x * minimapTotalScale + xres - map.width * minimapTotalScale, map.height * minimapTotalScale - y * minimapTotalScale);
					//glEnd();
				}
			}
		}
	}
	glEnd();

	// draw player pings
	if ( !minimapPings.empty() )
	{
		for ( std::vector<MinimapPing>::const_iterator it = minimapPings.begin(); it != minimapPings.end();)
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
				if ( (aliveTime < TICKS_PER_SECOND && (aliveTime % 10 < 5)) || aliveTime >= TICKS_PER_SECOND )
				{
					// draw the ping blinking every 5 ticks if less than 1 second lifetime, otherwise constantly draw.
					x = xres - map.width * minimapTotalScale + ping.x * minimapTotalScale;
					y = yres - map.height * minimapTotalScale + ping.y * minimapTotalScale;
					int alpha = 255;
					if ( aliveTime >= TICKS_PER_SECOND * 2 )
					{
						// start fading ping after 2 seconds, lasting 0.5 seconds.
						real_t alphafade = 1 - (aliveTime - TICKS_PER_SECOND * 2) / static_cast<real_t>(TICKS_PER_SECOND * 0.5);
						alpha = std::max(static_cast<int>(alphafade * alpha), 0);
					}
					// draw a circle
					drawCircle(x - 1, y - 1, std::max(3 + minimapObjectZoom, 0), color, alpha);
				}
			}


			// prune old pings > 2.5 seconds
			if ( aliveTime > TICKS_PER_SECOND * 2.5 )
			{
				if ( ping.player == clientnum )
				{
					if ( minimapPingGimpTimer > TICKS_PER_SECOND / 4 )
					{
						minimapPingGimpTimer = TICKS_PER_SECOND / 4; // reduce the gimp timer when one of the player's own pings fades.
					}
				}
				it = minimapPings.erase(it);
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
				//color = SDL_MapRGB(mainsurface->format, 0, 0, 192);
			}

			// draw the first pixel
			x = xres - map.width * minimapTotalScale + (int)(entity->x / (16.f / minimapTotalScale));
			y = map.height * minimapTotalScale - (int)(entity->y / (16.f / minimapTotalScale));
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
						glVertex2f( xres - map.width * minimapTotalScale + (int)(entity->x / (16.f / minimapTotalScale)) + x, map.height * minimapTotalScale - (int)(entity->y / (16.f / minimapTotalScale)) - y );
						glEnd();
					}
				}
			}
		}
	}

	// draw minotaur
	if (players[clientnum] == nullptr)
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
				x = xres - map.width * minimapTotalScale + (int)(entity->x / (16.f / minimapTotalScale));
				y = map.height * minimapTotalScale - (int)(entity->y / (16.f / minimapTotalScale));
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
						glVertex2f( xres - map.width * minimapTotalScale + (int)(entity->x / (16.f / minimapTotalScale)) + x, map.height * minimapTotalScale - (int)(entity->y / (16.f / minimapTotalScale)) - y );
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

void minimapPingAdd(MinimapPing newPing)
{
	int numPlayerPings = 0;
	if ( !minimapPings.empty() )
	{
		for ( std::vector<MinimapPing>::iterator it = minimapPings.begin(); it != minimapPings.end();)
		{
			MinimapPing ping = *it;
			if ( ping.player == newPing.player )
			{
				++numPlayerPings;
				// prune pings if too many by same player.
				if ( numPlayerPings > 3 )
				{
					if ( ping.player == clientnum )
					{
						// this is the player creating the sound source.
						minimapPingGimpTimer = TICKS_PER_SECOND * 3; // 3 second penalty for spam.
					}
					it = minimapPings.erase(it);
					continue;
				}
			}
			++it;
		}
	}
	if ( !minimapPingMute )
	{
		playSound(399, 64);
	}
	minimapPings.insert(minimapPings.begin(), newPing);
}