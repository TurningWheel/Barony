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
#include "../colors.hpp"
#include "interface.hpp"

void drawSkillsSheet();
void statsHoverText(Stat* tmpStat);

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
		if ( !players[clientnum]->entity->flags[INVISIBLE] )
		{
			glDrawVoxel(&camera_charsheet, players[clientnum]->entity, REALCOLORS);
		}
		players[clientnum]->entity->flags[BRIGHT] = b;
		c = 0;
		if (multiplayer != CLIENT)
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
	Sint32 statModifier = 0;
	char statText[64] = "";
	//Uint32 statColor = uint32ColorWhite(*mainsurface);
	//,
	snprintf(statText, 64, language[1200], stats[clientnum]->STR);
	ttfPrintTextFormatted(ttf12, 8, 262, statText);
	printStatBonus(ttf12, stats[clientnum]->STR, statGetSTR(stats[clientnum]), 8 + longestline(statText) * TTF12_WIDTH, 262);

	snprintf(statText, 64, language[1201], stats[clientnum]->DEX);
	ttfPrintTextFormatted(ttf12, 8, 274, statText);
	printStatBonus(ttf12, stats[clientnum]->DEX, statGetDEX(stats[clientnum]), 8 + longestline(statText) * TTF12_WIDTH, 274);

	snprintf(statText, 64, language[1202], stats[clientnum]->CON);
	ttfPrintTextFormatted(ttf12, 8, 286, statText);
	printStatBonus(ttf12, stats[clientnum]->CON, statGetCON(stats[clientnum]), 8 + longestline(statText) * TTF12_WIDTH, 286);

	snprintf(statText, 64, language[1203], stats[clientnum]->INT);
	ttfPrintTextFormatted(ttf12, 8, 298, statText);
	printStatBonus(ttf12, stats[clientnum]->INT, statGetINT(stats[clientnum]), 8 + longestline(statText) * TTF12_WIDTH, 298);

	snprintf(statText, 64, language[1204], stats[clientnum]->PER);
	ttfPrintTextFormatted(ttf12, 8, 310, statText);
	printStatBonus(ttf12, stats[clientnum]->PER, statGetPER(stats[clientnum]), 8 + longestline(statText) * TTF12_WIDTH, 310);

	snprintf(statText, 64, language[1205], stats[clientnum]->CHR);
	ttfPrintTextFormatted(ttf12, 8, 322, statText);
	printStatBonus(ttf12, stats[clientnum]->CHR, statGetCHR(stats[clientnum]), 8 + longestline(statText) * TTF12_WIDTH, 322);

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
	statsHoverText(stats[clientnum]);
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
	Uint32 color;
	for ( int i = 0; i < (NUMPROFICIENCIES); ++i, pos.y += (TTF12_HEIGHT /** 2*/) )
	{
		if ( skillCapstoneUnlocked(clientnum, i) )
		{
			color = uint32ColorGreen(*mainsurface);
		}
		else
		{
			color = uint32ColorWhite(*mainsurface);
		}

		if ( stats[clientnum]->PROFICIENCIES[i] == 0 )
		{
			ttfPrintTextFormattedColor(ttf12, pos.x + 4, pos.y, color, language[363]);
		}
		else if ( stats[clientnum]->PROFICIENCIES[i] < SKILL_LEVEL_BASIC )
		{
			ttfPrintTextFormattedColor(ttf12, pos.x + 4, pos.y, color, language[364]);
		}
		else if ( stats[clientnum]->PROFICIENCIES[i] >= SKILL_LEVEL_BASIC && stats[clientnum]->PROFICIENCIES[i] < SKILL_LEVEL_SKILLED )
		{
			ttfPrintTextFormattedColor(ttf12, pos.x + 4, pos.y, color, language[365]);
		}
		else if ( stats[clientnum]->PROFICIENCIES[i] >= SKILL_LEVEL_SKILLED && stats[clientnum]->PROFICIENCIES[i] < SKILL_LEVEL_EXPERT )
		{
			ttfPrintTextFormattedColor(ttf12, pos.x + 4, pos.y, color, language[366]);
		}
		else if ( stats[clientnum]->PROFICIENCIES[i] >= SKILL_LEVEL_EXPERT && stats[clientnum]->PROFICIENCIES[i] < SKILL_LEVEL_MASTER )
		{
			ttfPrintTextFormattedColor(ttf12, pos.x + 4, pos.y, color, language[367]);
		}
		else if ( stats[clientnum]->PROFICIENCIES[i] >= SKILL_LEVEL_MASTER && stats[clientnum]->PROFICIENCIES[i] < SKILL_LEVEL_LEGENDARY )
		{
			ttfPrintTextFormattedColor(ttf12, pos.x + 4, pos.y, color, language[368]);
		}
		else if ( stats[clientnum]->PROFICIENCIES[i] >= SKILL_LEVEL_LEGENDARY )
		{
			ttfPrintTextFormattedColor(ttf12, pos.x + 4, pos.y, color, language[369]);
		}
	}
}

void statsHoverText(Stat* tmpStat)
{
	if ( tmpStat == nullptr )
	{
		return;
	}

	int pad_y = 262; // 262 px.
	int pad_x = 8; // 8 px.
	int off_h = TTF12_HEIGHT - 4; // 12px. height of stat line.
	int off_w = 216; // 216px. width of stat line.
	int i = 0;
	int j = 0;
	SDL_Rect src;
	SDL_Rect pos;

	int tooltip_offset_x = 16; // 16px.
	int tooltip_offset_y = 16; // 16px.
	int tooltip_base_h = TTF12_HEIGHT;
	int tooltip_pad_h = 8;
	int tooltip_text_pad_x = 8;

	char tooltipHeader[6][128] =
	{
		"strength: ",
		"dexterity: ",
		"constitution: ",
		"intelligence: ",
		"perception: ",
		"charisma: "
	};

	char tooltipText[6][2][128] =
	{
		{
			"base:  %2d",
			"bonus: %2d"
		},
		{
			"base:  %2d",
			"bonus: %2d"
		},
		{
			"base:  %2d",
			"bonus: %2d"
		},
		{
			"base:  %2d",
			"bonus: %2d"
		},
		{
			"base:  %2d",
			"bonus: %2d"
		},
		{
			"base:  %2d",
			"bonus: %2d"
		}
	};

	char buf[128] = "";
	int numInfoLines = 0;
	Sint32 statBase = 0;
	Sint32 statBonus = 0;

	SDL_Surface *tmp_bmp = NULL;

	if ( attributespage == 0 )
	{
		for ( i = 0; i < 6; i++ ) // cycle through 6 stats.
		{
			switch ( i )
			{
				// prepare the stat image.
				case 0:
					numInfoLines = 2;
					tmp_bmp = str_bmp64;
					statBase = tmpStat->STR;
					statBonus = statGetSTR(tmpStat) - statBase;
					break;
				case 1:
					numInfoLines = 2;
					tmp_bmp = dex_bmp64;
					statBase = tmpStat->DEX;
					statBonus = statGetDEX(tmpStat) - statBase;
					break;
				case 2:
					numInfoLines = 2;
					tmp_bmp = con_bmp64;
					statBase = tmpStat->CON;
					statBonus = statGetCON(tmpStat) - statBase;
					break;
				case 3:
					numInfoLines = 2;
					tmp_bmp = int_bmp64;
					statBase = tmpStat->INT;
					statBonus = statGetINT(tmpStat) - statBase;
					break;
				case 4:
					numInfoLines = 2;
					tmp_bmp = per_bmp64;
					statBase = tmpStat->PER;
					statBonus = statGetPER(tmpStat) - statBase;
					break;
				case 5:
					numInfoLines = 2;
					tmp_bmp = chr_bmp64;
					statBase = tmpStat->CHR;
					statBonus = statGetCHR(tmpStat) - statBase;
					break;
				default:
					numInfoLines = 0;
					break;
			}

			if ( mouseInBounds(pad_x, pad_x + off_w, pad_y, pad_y + off_h) )
			{
				src.x = mousex + tooltip_offset_x;
				src.y = mousey + tooltip_offset_y;
				src.h = std::max(tooltip_base_h * (numInfoLines + 1) + tooltip_pad_h, tooltip_base_h * (2) + tooltip_pad_h);
				src.w = 256;
				drawTooltip(&src);

				pos.x = src.x + 6;
				pos.y = src.y + 4;
				pos.h = 32;
				pos.w = 32;

				drawImageScaled(tmp_bmp, NULL, &pos);

				src.x = pos.x + pos.w;
				src.y += 2;

				ttfPrintText(ttf12, src.x + 4, src.y + 4, tooltipHeader[i]);

				for ( j = 0; j < numInfoLines && numInfoLines > 0; j++ )
				{
					int infoText_x = src.x + 4 + tooltip_text_pad_x;
					int infoText_y = src.y + 4 + (tooltip_base_h * (j + 1));
					Uint32 color = uint32ColorWhite(*mainsurface);

					if ( j == 0 )
					{
						snprintf(buf, longestline(tooltipText[i][j]), tooltipText[i][j], statBase);
					}
					else if ( j == 1 )
					{
						snprintf(buf, longestline(tooltipText[i][j]), tooltipText[i][j], statBonus);
						if ( statBonus > 0 )
						{
							color = uint32ColorGreen(*mainsurface);
						}
						else if ( statBonus < 0 )
						{
							color = uint32ColorRed(*mainsurface);
						}
					}
					ttfPrintTextColor(ttf12, infoText_x, infoText_y, color, false, buf);
				}
			}

			numInfoLines = 0;
			pad_y += 12;
		}
	}
}

void printStatBonus(TTF_Font* outputFont, Sint32 stat, Sint32 statWithModifiers, int x, int y)
{
	Uint32 color = 0;
	char bonusText[4] = "";

	if ( statWithModifiers - stat == 0 )
	{
		color = uint32ColorWhite(*mainsurface);
		snprintf(bonusText, 4, "%2d", statWithModifiers);
	}
	else if ( statWithModifiers - stat < 0 )
	{
		color = uint32ColorRed(*mainsurface);
		snprintf(bonusText, 4, "%2d", statWithModifiers);
	}
	if ( statWithModifiers - stat > 0 )
	{
		color = uint32ColorGreen(*mainsurface);
		snprintf(bonusText, 4, "%2d", statWithModifiers);
	}

	ttfPrintTextFormattedColor(outputFont, x, y, color, bonusText);
}
