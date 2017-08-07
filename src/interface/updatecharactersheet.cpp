/*-------------------------------------------------------------------------------

	BARONY
	File: updatecharactersheet.cpp
	Desc: contains updateCharacterSheet()

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../main.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "../items.hpp"
#include "../player.hpp"
#include "interface.hpp"

void drawSkillsSheet();

/*-------------------------------------------------------------------------------

	updateCharacterSheet

	Draws the character sheet and processes all interaction with it

-------------------------------------------------------------------------------*/

void updateCharacterSheet()
{
	int i = 0;
	int x = 0;
	SDL_Rect pos;
	bool b = false;
	node_t* node = NULL;
	Entity* entity = NULL;
	Item* item = NULL;
	int c;

	// draw window
	drawWindowFancy(0, 0, 224, 196);
	pos.x = 8;
	pos.y = 8;
	pos.w = 208;
	pos.h = 180;
	drawRect(&pos, 0, 255);
	//drawImage(character_bmp, NULL, &pos);
	//pos.x=0; pos.y=196;
	//pos.w=222; pos.h=392-196;
	//drawTooltip(&pos);
	drawWindowFancy(0, 196, 224, 392);

	// character sheet
	double ofov = fov;
	fov = 50;
	if (players[clientnum] != nullptr && players[clientnum]->entity != nullptr)
	{
		if (!softwaremode)
		{
			glClear(GL_DEPTH_BUFFER_BIT);
		}
		//TODO: These two NOT PLAYERSWAP
		//camera.x=players[clientnum]->x/16.0+.5*cos(players[clientnum]->yaw)-.4*sin(players[clientnum]->yaw);
		//camera.y=players[clientnum]->y/16.0+.5*sin(players[clientnum]->yaw)+.4*cos(players[clientnum]->yaw);
		camera_charsheet.x = players[clientnum]->entity->x / 16.0 + .65;
		camera_charsheet.y = players[clientnum]->entity->y / 16.0 + .65;
		camera_charsheet.z = players[clientnum]->entity->z * 2;
		//camera.ang=atan2(players[clientnum]->y/16.0-camera.y,players[clientnum]->x/16.0-camera.x); //TODO: _NOT_ PLAYERSWAP
		camera_charsheet.ang = 5 * PI / 4;
		camera_charsheet.vang = PI / 20;
		camera_charsheet.winx = 8;
		camera_charsheet.winy = 8;
		camera_charsheet.winw = 208;
		camera_charsheet.winh = 180;
		b = players[clientnum]->entity->flags[BRIGHT];
		players[clientnum]->entity->flags[BRIGHT] = true;
		if (!players[clientnum]->entity->flags[INVISIBLE])
		{
			glDrawVoxel(&camera_charsheet, players[clientnum]->entity, REALCOLORS);
		}
		players[clientnum]->entity->flags[BRIGHT] = b;
		c = 0;
		if (localPlayerNetworkType != NetworkType::CLIENT)
		{
			for (node = players[clientnum]->entity->children.first; node != nullptr; node = node->next)
			{
				if (c == 0)
				{
					c++;
					continue;
				}
				entity = (Entity*) node->element;
				if ( !entity->flags[INVISIBLE] )
				{
					b = entity->flags[BRIGHT];
					entity->flags[BRIGHT] = true;
					glDrawVoxel(&camera_charsheet, entity, REALCOLORS);
					entity->flags[BRIGHT] = b;
				}
				c++;
			}
			for ( node = map.entities->first; node != NULL; node = node->next )
			{
				entity = (Entity*) node->element;
				if ( (Sint32)entity->getUID() == -4 )
				{
					glDrawSprite(&camera_charsheet, entity, REALCOLORS);
				}
			}
		}
		else
		{
			for ( node = map.entities->first; node != NULL; node = node->next )
			{
				entity = (Entity*) node->element;
				if ( (entity->behavior == &actPlayerLimb && entity->skill[2] == clientnum && !entity->flags[INVISIBLE]) || (Sint32)entity->getUID() == -4 )
				{
					b = entity->flags[BRIGHT];
					entity->flags[BRIGHT] = true;
					if ( (Sint32)entity->getUID() == -4 )
					{
						glDrawSprite(&camera_charsheet, entity, REALCOLORS);
					}
					else
					{
						glDrawVoxel(&camera_charsheet, entity, REALCOLORS);
					}
					entity->flags[BRIGHT] = b;
				}
			}
		}
	}
	fov = ofov;

	ttfPrintTextFormatted(ttf12, 8, 202, "%s", stats[clientnum]->name);
	ttfPrintTextFormatted(ttf12, 8, 214, language[359], stats[clientnum]->LVL, language[1900 + client_classes[clientnum]]);
	ttfPrintTextFormatted(ttf12, 8, 226, language[360], stats[clientnum]->EXP);
	ttfPrintTextFormatted(ttf12, 8, 238, language[361], currentlevel);

	// attributes
	ttfPrintTextFormatted(ttf12, 8, 262, language[1200], statGetSTR(stats[clientnum]), stats[clientnum]->STR);
	ttfPrintTextFormatted(ttf12, 8, 274, language[1201], statGetDEX(stats[clientnum]), stats[clientnum]->DEX);
	ttfPrintTextFormatted(ttf12, 8, 286, language[1202], statGetCON(stats[clientnum]), stats[clientnum]->CON);
	ttfPrintTextFormatted(ttf12, 8, 298, language[1203], statGetINT(stats[clientnum]), stats[clientnum]->INT);
	ttfPrintTextFormatted(ttf12, 8, 310, language[1204], statGetPER(stats[clientnum]), stats[clientnum]->PER);
	ttfPrintTextFormatted(ttf12, 8, 322, language[1205], statGetCHR(stats[clientnum]), stats[clientnum]->CHR);

	// armor, gold, and weight
	ttfPrintTextFormatted(ttf12, 8, 346, language[370], stats[clientnum]->GOLD);
	ttfPrintTextFormatted(ttf12, 8, 358, language[371], AC(stats[clientnum]));
	Uint32 weight = 0;
	for ( node = stats[clientnum]->inventory.first; node != NULL; node = node->next )
	{
		item = (Item*)node->element;
		weight += items[item->type].weight * item->count;
	}
	weight += stats[clientnum]->GOLD / 100;
	ttfPrintTextFormatted(ttf12, 8, 370, language[372], weight);

	drawSkillsSheet();
}

void drawSkillsSheet()
{
	SDL_Rect pos;
	pos.x = xres - 208;
	pos.w = 208;
	pos.y = 32;
	pos.h = (NUMPROFICIENCIES * TTF12_HEIGHT) + (TTF12_HEIGHT * 3);

	drawWindowFancy(pos.x, pos.y, pos.x + pos.w, pos.y + pos.h);

	ttfPrintTextFormatted(ttf12, pos.x + 4, pos.y + 8, language[1883]);

	pos.y += TTF12_HEIGHT * 2 + 8;

	SDL_Rect initialSkillPos = pos;
	//Draw skill names.
	for ( int c = 0; c < (NUMPROFICIENCIES); ++c, pos.y += (TTF12_HEIGHT /** 2*/) )
	{
		ttfPrintTextFormatted(ttf12, pos.x + 4, pos.y, "%s:", language[236 + c]);
	}

	//Draw skill levels.
	pos = initialSkillPos;
	for ( int i = 0; i < (NUMPROFICIENCIES); ++i, pos.y += (TTF12_HEIGHT /** 2*/) )
	{
		if ( stats[clientnum]->PROFICIENCIES[i] == 0 )
		{
			ttfPrintTextFormatted(ttf12, pos.x + 4, pos.y, language[363]);
		}
		else if ( stats[clientnum]->PROFICIENCIES[i] < 20 )
		{
			ttfPrintTextFormatted(ttf12, pos.x + 4, pos.y, language[364]);
		}
		else if ( stats[clientnum]->PROFICIENCIES[i] >= 20 && stats[clientnum]->PROFICIENCIES[i] < 40 )
		{
			ttfPrintTextFormatted(ttf12, pos.x + 4, pos.y, language[365]);
		}
		else if ( stats[clientnum]->PROFICIENCIES[i] >= 40 && stats[clientnum]->PROFICIENCIES[i] < 60 )
		{
			ttfPrintTextFormatted(ttf12, pos.x + 4, pos.y, language[366]);
		}
		else if ( stats[clientnum]->PROFICIENCIES[i] >= 60 && stats[clientnum]->PROFICIENCIES[i] < 80 )
		{
			ttfPrintTextFormatted(ttf12, pos.x + 4, pos.y, language[367]);
		}
		else if ( stats[clientnum]->PROFICIENCIES[i] >= 80 && stats[clientnum]->PROFICIENCIES[i] < 100 )
		{
			ttfPrintTextFormatted(ttf12, pos.x + 4, pos.y, language[368]);
		}
		else if ( stats[clientnum]->PROFICIENCIES[i] >= 100 )
		{
			ttfPrintTextFormatted(ttf12, pos.x + 4, pos.y, language[369]);
		}
	}
}
