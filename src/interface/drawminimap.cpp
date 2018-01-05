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

#define MINIMAPSCALE 4
void drawMinimap()
{
	node_t* node;
	Uint32 color;
	int x, y, i;

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
				glColor4f( 32 / 255.f,  12 / 255.f, 0 / 255.f, 1 );
			}
			else if ( minimap[y][x] == 1 )
			{
				glColor4f( 96 / 255.f,  24 / 255.f, 0 / 255.f, 1 );
			}
			else if ( minimap[y][x] == 2 )
			{
				glColor4f( 192 / 255.f, 64 / 255.f, 0 / 255.f, 1 );
			}
			else if ( minimap[y][x] == 3 )
			{
				glColor4f( 32 / 255.f, 32 / 255.f, 32 / 255.f, 1 );
			}
			else if ( minimap[y][x] == 4 )
			{
				glColor4f( 64 / 255.f, 64 / 255.f, 64 / 255.f, 1 );
			}
			//glBegin(GL_QUADS);
			glVertex2f(x * MINIMAPSCALE + xres - map.width * MINIMAPSCALE, map.height * MINIMAPSCALE - y * MINIMAPSCALE - MINIMAPSCALE);
			glVertex2f(x * MINIMAPSCALE + xres - map.width * MINIMAPSCALE + MINIMAPSCALE, map.height * MINIMAPSCALE - y * MINIMAPSCALE - MINIMAPSCALE);
			glVertex2f(x * MINIMAPSCALE + xres - map.width * MINIMAPSCALE + MINIMAPSCALE, map.height * MINIMAPSCALE - y * MINIMAPSCALE);
			glVertex2f(x * MINIMAPSCALE + xres - map.width * MINIMAPSCALE, map.height * MINIMAPSCALE - y * MINIMAPSCALE);
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
						glVertex2f(x * MINIMAPSCALE + xres - map.width * MINIMAPSCALE, map.height * MINIMAPSCALE - y * MINIMAPSCALE - MINIMAPSCALE);
						glVertex2f(x * MINIMAPSCALE + xres - map.width * MINIMAPSCALE + MINIMAPSCALE, map.height * MINIMAPSCALE - y * MINIMAPSCALE - MINIMAPSCALE);
						glVertex2f(x * MINIMAPSCALE + xres - map.width * MINIMAPSCALE + MINIMAPSCALE, map.height * MINIMAPSCALE - y * MINIMAPSCALE);
						glVertex2f(x * MINIMAPSCALE + xres - map.width * MINIMAPSCALE, map.height * MINIMAPSCALE - y * MINIMAPSCALE);
						//glEnd();
					}
				}
			}
		}
		else
		{
			if ( entity->behavior == &actMonster )
			{
				if ( stats[clientnum]->ring != NULL )
				{
					if ( stats[clientnum]->ring->type == RING_WARNING )
					{
						x = floor(entity->x / 16);
						y = floor(entity->y / 16);
						glColor4f( .5, .25, .5, 1 );
						//glBegin(GL_QUADS);
						glVertex2f(x * MINIMAPSCALE + xres - map.width * MINIMAPSCALE, map.height * MINIMAPSCALE - y * MINIMAPSCALE - MINIMAPSCALE);
						glVertex2f(x * MINIMAPSCALE + xres - map.width * MINIMAPSCALE + MINIMAPSCALE, map.height * MINIMAPSCALE - y * MINIMAPSCALE - MINIMAPSCALE);
						glVertex2f(x * MINIMAPSCALE + xres - map.width * MINIMAPSCALE + MINIMAPSCALE, map.height * MINIMAPSCALE - y * MINIMAPSCALE);
						glVertex2f(x * MINIMAPSCALE + xres - map.width * MINIMAPSCALE, map.height * MINIMAPSCALE - y * MINIMAPSCALE);
						//glEnd();
					}
				}
				else if ( stats[clientnum]->shoes != NULL )
				{
					if ( stats[clientnum]->shoes->type == ARTIFACT_BOOTS )
					{
						x = floor(entity->x / 16);
						y = floor(entity->y / 16);
						glColor4f(.5, .25, .5, 1);
						//glBegin(GL_QUADS);
						glVertex2f(x * MINIMAPSCALE + xres - map.width * MINIMAPSCALE, map.height * MINIMAPSCALE - y * MINIMAPSCALE - MINIMAPSCALE);
						glVertex2f(x * MINIMAPSCALE + xres - map.width * MINIMAPSCALE + MINIMAPSCALE, map.height * MINIMAPSCALE - y * MINIMAPSCALE - MINIMAPSCALE);
						glVertex2f(x * MINIMAPSCALE + xres - map.width * MINIMAPSCALE + MINIMAPSCALE, map.height * MINIMAPSCALE - y * MINIMAPSCALE);
						glVertex2f(x * MINIMAPSCALE + xres - map.width * MINIMAPSCALE, map.height * MINIMAPSCALE - y * MINIMAPSCALE);
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
					glVertex2f(x * MINIMAPSCALE + xres - map.width * MINIMAPSCALE, map.height * MINIMAPSCALE - y * MINIMAPSCALE - MINIMAPSCALE);
					glVertex2f(x * MINIMAPSCALE + xres - map.width * MINIMAPSCALE + MINIMAPSCALE, map.height * MINIMAPSCALE - y * MINIMAPSCALE - MINIMAPSCALE);
					glVertex2f(x * MINIMAPSCALE + xres - map.width * MINIMAPSCALE + MINIMAPSCALE, map.height * MINIMAPSCALE - y * MINIMAPSCALE);
					glVertex2f(x * MINIMAPSCALE + xres - map.width * MINIMAPSCALE, map.height * MINIMAPSCALE - y * MINIMAPSCALE);
					//glEnd();
				}
			}
		}
	}
	glEnd();

	// draw players and allies
	
	for ( node = map.creatures->first; node != nullptr; node = node->next )
	{
		Entity* entity = (Entity*)node->element;
		bool drawchar = false;
		bool foundme = false;
		if ( entity->behavior == &actPlayer )
		{
			drawchar = true;
			if ( entity->skill[2] == clientnum )
			{
				foundme = true;
			}
		}
		else if ( entity->behavior == &actMonster )
		{
			node_t* node2;
			for ( node2 = stats[clientnum]->FOLLOWERS.first; node2 != nullptr; node2 = node2->next )
			{
				if ( *((Uint32*)node2->element) == entity->getUID() )
				{
					drawchar = true;
					break;
				}
			}
		}
		if ( drawchar )
		{
			// my player = green, other players = blue
			if ( foundme )
			{
				color = SDL_MapRGB(mainsurface->format, 0, 192, 0);
			}
			else
			{
				color = SDL_MapRGB(mainsurface->format, 0, 0, 192);
			}

			// draw the first pixel
			x = xres - map.width * MINIMAPSCALE + (int)(entity->x / (16.f / MINIMAPSCALE));
			y = map.height * MINIMAPSCALE - (int)(entity->y / (16.f / MINIMAPSCALE));
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

			// draw a circle
			drawCircle(x - 1, yres - y - 1, 3, color, 255);

			x = 0;
			y = 0;
			for ( i = 0; i < 4; ++i )
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
				if ( foundme )
				{
					color = SDL_MapRGB(mainsurface->format, 64, 255, 64);
				}
				else
				{
					color = SDL_MapRGB(mainsurface->format, 64, 64, 255);
				}

				// draw the pixel
				if ( softwaremode )
				{
					//SPG_Pixel(screen,(int)(players[c]->x/16)+564+x+xres/2-(status_bmp->w/2),(int)(players[c]->y/16)+yres-71+y,color); //TODO: NOT a PLAYERSWAP
				}
				else
				{
					glColor4f(((Uint8)(color >> mainsurface->format->Rshift)) / 255.f, ((Uint8)(color >> mainsurface->format->Gshift)) / 255.f, ((Uint8)(color >> mainsurface->format->Bshift)) / 255.f, 1);
					glBegin(GL_POINTS);
					glVertex2f( xres - map.width * MINIMAPSCALE + (int)(entity->x / (16.f / MINIMAPSCALE)) + x, map.height * MINIMAPSCALE - (int)(entity->y / (16.f / MINIMAPSCALE)) - y );
					glEnd();
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
				x = xres - map.width * MINIMAPSCALE + (int)(entity->x / (16.f / MINIMAPSCALE));
				y = map.height * MINIMAPSCALE - (int)(entity->y / (16.f / MINIMAPSCALE));
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
				drawCircle(x - 1, yres - y - 1, 3, color, 255);

				x = 0;
				y = 0;
				for ( i = 0; i < 4; ++i )
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
						glVertex2f( xres - map.width * MINIMAPSCALE + (int)(entity->x / (16.f / MINIMAPSCALE)) + x, map.height * MINIMAPSCALE - (int)(entity->y / (16.f / MINIMAPSCALE)) - y );
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
