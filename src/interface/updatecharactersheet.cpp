/*-------------------------------------------------------------------------------

	BARONY
	File: updatecharactersheet.cpp
	Desc: contains updateCharacterSheet()

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../main.hpp"
#include "../draw.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "../items.hpp"
#include "../player.hpp"
#include "../colors.hpp"
#include "interface.hpp"
#include "../sound.hpp"
#include "../magic/magic.hpp"
#include "../menu.hpp"

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
	pos.x = 8;
	pos.y = 8;
	pos.w = 208;
	pos.h = 180;
	//drawImage(character_bmp, NULL, &pos);
	//pos.x=0; pos.y=196;
	//pos.w=222; pos.h=392-196;
	//drawTooltip(&pos);
	int statWindowY = 196;
	int statWindowY2 = 404;
	if ( uiscale_charactersheet )
	{
		pos.h = 236;
		pos.w = 276;
		statWindowY = pos.h + 16;
		statWindowY2 = 554;
	}

	drawWindowFancy(0, 0, pos.w + 16, pos.h + 16);
	drawRect(&pos, 0, 255);
	drawWindowFancy(0, pos.h + 16, pos.w + 16, statWindowY2);

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
		camera_charsheet.x = players[clientnum]->entity->x / 16.0 + (.92 * cos(camera_charsheet_offsetyaw));
		camera_charsheet.y = players[clientnum]->entity->y / 16.0 + (.92 * sin(camera_charsheet_offsetyaw));
		camera_charsheet.z = players[clientnum]->entity->z * 2;
		//camera.ang=atan2(players[clientnum]->y/16.0-camera.y,players[clientnum]->x/16.0-camera.x); //TODO: _NOT_ PLAYERSWAP
		camera_charsheet.ang = (camera_charsheet_offsetyaw - PI); //5 * PI / 4;
		camera_charsheet.vang = PI / 20;
		camera_charsheet.winx = 8;
		camera_charsheet.winy = 8;
		camera_charsheet.winw = pos.w;
		camera_charsheet.winh = pos.h;
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

		SDL_Rect rotateBtn;
		rotateBtn.w = 16;
		rotateBtn.h = 16;
		rotateBtn.x = camera_charsheet.winx + camera_charsheet.winw - rotateBtn.w;
		rotateBtn.y = camera_charsheet.winy + camera_charsheet.winh - rotateBtn.h;
		drawWindow(rotateBtn.x, rotateBtn.y, rotateBtn.x + rotateBtn.w, rotateBtn.y + rotateBtn.h);
		if ( mousestatus[SDL_BUTTON_LEFT] && !shootmode )
		{
			if ( mouseInBounds(rotateBtn.x, rotateBtn.x + rotateBtn.w, rotateBtn.y, rotateBtn.y + rotateBtn.h) )
			{
				camera_charsheet_offsetyaw += 0.05;
				if ( camera_charsheet_offsetyaw > 2 * PI )
				{
					camera_charsheet_offsetyaw -= 2 * PI;
				}
				drawDepressed(rotateBtn.x, rotateBtn.y, rotateBtn.x + rotateBtn.w, rotateBtn.y + rotateBtn.h);
			}
		}
		ttfPrintText(ttf12, rotateBtn.x + 2, rotateBtn.y + 2, ">");

		rotateBtn.x = camera_charsheet.winx + camera_charsheet.winw - rotateBtn.w * 2 - 4;
		rotateBtn.y = camera_charsheet.winy + camera_charsheet.winh - rotateBtn.h;
		drawWindow(rotateBtn.x, rotateBtn.y, rotateBtn.x + rotateBtn.w, rotateBtn.y + rotateBtn.h);
		if ( mousestatus[SDL_BUTTON_LEFT] && !shootmode )
		{
			if ( mouseInBounds(rotateBtn.x, rotateBtn.x + rotateBtn.w, rotateBtn.y, rotateBtn.y + rotateBtn.h) )
			{
				camera_charsheet_offsetyaw -= 0.05;
				if ( camera_charsheet_offsetyaw < 0.f )
				{
					camera_charsheet_offsetyaw += 2 * PI;
				}
				drawDepressed(rotateBtn.x, rotateBtn.y, rotateBtn.x + rotateBtn.w, rotateBtn.y + rotateBtn.h);
			}
		}
		ttfPrintText(ttf12, rotateBtn.x, rotateBtn.y + 2, "<");
	}
	fov = ofov;

	TTF_Font* fontStat = ttf12;
	int text_y = 0;
	int pad_y = 12;
	int fontWidth = TTF12_WIDTH;
	if ( uiscale_charactersheet )
	{
		fontStat = ttf16;
		pad_y = 18;
		fontWidth = TTF16_WIDTH;
	}
	text_y = statWindowY + 6;
	ttfPrintTextFormatted(fontStat, 8, text_y, "%s", stats[clientnum]->name);
	text_y += pad_y;
	ttfPrintTextFormatted(fontStat, 8, text_y, language[359], stats[clientnum]->LVL, playerClassLangEntry(client_classes[clientnum]));
	text_y += pad_y;
	ttfPrintTextFormatted(fontStat, 8, text_y, language[360], stats[clientnum]->EXP);
	text_y += pad_y;
	ttfPrintTextFormatted(fontStat, 8, text_y, language[361], currentlevel);

	Entity* playerEntity = nullptr;
	if ( players[clientnum] )
	{
		playerEntity = players[clientnum]->entity;
	}

	// attributes
	Sint32 statModifier = 0;
	char statText[64] = "";
	//Uint32 statColor = uint32ColorWhite(*mainsurface);
	text_y += pad_y * 2;
	snprintf(statText, 64, language[1200], stats[clientnum]->STR);
	ttfPrintTextFormatted(fontStat, 8, text_y, statText);
	printStatBonus(fontStat, stats[clientnum]->STR, statGetSTR(stats[clientnum], playerEntity), 8 + longestline(statText) * fontWidth, text_y);

	text_y += pad_y;
	snprintf(statText, 64, language[1201], stats[clientnum]->DEX);
	ttfPrintTextFormatted(fontStat, 8, text_y, statText);
	printStatBonus(fontStat, stats[clientnum]->DEX, statGetDEX(stats[clientnum], playerEntity), 8 + longestline(statText) * fontWidth, text_y);

	text_y += pad_y;
	snprintf(statText, 64, language[1202], stats[clientnum]->CON);
	ttfPrintTextFormatted(fontStat, 8, text_y, statText);
	printStatBonus(fontStat, stats[clientnum]->CON, statGetCON(stats[clientnum], playerEntity), 8 + longestline(statText) * fontWidth, text_y);

	text_y += pad_y;
	snprintf(statText, 64, language[1203], stats[clientnum]->INT);
	ttfPrintTextFormatted(fontStat, 8, text_y, statText);
	printStatBonus(fontStat, stats[clientnum]->INT, statGetINT(stats[clientnum], playerEntity), 8 + longestline(statText) * fontWidth, text_y);

	text_y += pad_y;
	snprintf(statText, 64, language[1204], stats[clientnum]->PER);
	ttfPrintTextFormatted(fontStat, 8, text_y, statText);
	printStatBonus(fontStat, stats[clientnum]->PER, statGetPER(stats[clientnum], playerEntity), 8 + longestline(statText) * fontWidth, text_y);

	text_y += pad_y;
	snprintf(statText, 64, language[1205], stats[clientnum]->CHR);
	ttfPrintTextFormatted(fontStat, 8, text_y, statText);
	printStatBonus(fontStat, stats[clientnum]->CHR, statGetCHR(stats[clientnum], playerEntity), 8 + longestline(statText) * fontWidth, text_y);

	// armor, gold, and weight
	int attackInfo[6] = { 0 };
	text_y += pad_y * 2;
	ttfPrintTextFormatted(fontStat, 8, text_y, language[2542], displayAttackPower(attackInfo));

	text_y += pad_y;
	ttfPrintTextFormatted(fontStat, 8, text_y, language[371], AC(stats[clientnum]));

	text_y += pad_y;
	ttfPrintTextFormatted(fontStat, 8, text_y, language[370], stats[clientnum]->GOLD);
	Uint32 weight = 0;
	for ( node = stats[clientnum]->inventory.first; node != NULL; node = node->next )
	{
		item = (Item*)node->element;
		weight += items[item->type].weight * item->count;
	}
	weight += stats[clientnum]->GOLD / 100;
	text_y += pad_y;
	ttfPrintTextFormatted(fontStat, 8, text_y, language[372], weight);

	statsHoverText(stats[clientnum]);
	attackHoverText(attackInfo);

	// gold hover text.
	SDL_Rect src;
	src.x = mousex + 16;
	src.y = mousey + 16;
	src.h = TTF12_HEIGHT + 4;
	src.w = ( longestline(language[2968]) + strlen(getInputName(impulses[IN_USE])) ) * TTF12_WIDTH + 4;
	if ( mouseInBounds(pos.x + 4, pos.x + pos.w, text_y - pad_y, text_y) )
	{
		drawTooltip(&src);
		ttfPrintTextFormatted(ttf12, src.x + 4, src.y + 4, language[2968], getInputName(impulses[IN_USE]));
		if ( *inputPressed(impulses[IN_USE]) )
		{
			consoleCommand("/dropgold");
			*inputPressed(impulses[IN_USE]) = 0;
		}
		else if ( *inputPressed(joyimpulses[INJOY_GAME_USE]) )
		{
			consoleCommand("/dropgold");
			*inputPressed(joyimpulses[INJOY_GAME_USE]) = 0;
		}
	}
}

void drawSkillsSheet()
{
	SDL_Rect pos;
	pos.w = 208;
	pos.y = 32;

	TTF_Font* fontSkill = ttf12;
	int fontHeight = TTF12_HEIGHT;
	int fontWidth = TTF12_WIDTH;
	if ( uiscale_skillspage )
	{
		fontSkill = ttf16;
		fontHeight = TTF16_HEIGHT;
		fontWidth = TTF16_WIDTH;
		pos.w = 276;
	}
	pos.x = xres - pos.w;


	pos.h = (NUMPROFICIENCIES * fontHeight) + (fontHeight * 3);

	drawWindowFancy(pos.x, pos.y, pos.x + pos.w, pos.y + pos.h);
	interfaceSkillsSheet.x = pos.x;
	interfaceSkillsSheet.y = pos.y;
	interfaceSkillsSheet.w = pos.w;
	interfaceSkillsSheet.h = pos.h;

	ttfPrintTextFormatted(fontSkill, pos.x + 4, pos.y + 8, language[1883]);

	SDL_Rect button;
	button.x = xres - attributesright_bmp->w - 8;
	button.w = attributesright_bmp->w;
	button.y = pos.y;
	button.h = attributesright_bmp->h;
	if ( uiscale_skillspage )
	{
		button.w = attributesright_bmp->w * 1.3;
		button.x = xres - button.w - 8;
		button.y = pos.y;
		button.h = attributesright_bmp->h * 1.3;
	}

	if ( mousestatus[SDL_BUTTON_LEFT] && !shootmode )
	{
		if ( omousex >= button.x && omousex <= button.x + button.w
			&& omousey >= button.y && omousey <= button.y + button.h )
		{
			buttonclick = 14;
			playSound(139, 64);
			if ( proficienciesPage == 0 )
			{
				proficienciesPage = 1;
			}
			else
			{
				proficienciesPage = 0;
			}
			mousestatus[SDL_BUTTON_LEFT] = 0;
		}
	}
	if ( buttonclick == 14 )
	{
		drawImageScaled(attributesright_bmp, nullptr, &button);
	}
	else
	{
		drawImageScaled(attributesrightunclicked_bmp, nullptr, &button);
	}

	SDL_Rect lockbtn = button;
	lockbtn.h = 24;
	lockbtn.w = 24;
	lockbtn.y += 2;
	if ( uiscale_skillspage )
	{
		lockbtn.h = 24 * 1.3;
		lockbtn.w = 24 * 1.3;
		lockbtn.x -= 32 * 1.3;
	}
	else
	{
		lockbtn.x -= 32;
	}
	if ( lock_right_sidebar )
	{
		drawImageScaled(sidebar_lock_bmp, nullptr, &lockbtn);
	}
	else
	{
		drawImageScaled(sidebar_unlock_bmp, nullptr, &lockbtn);
	}

	if ( mousestatus[SDL_BUTTON_LEFT] && !shootmode )
	{
		if ( omousex >= lockbtn.x && omousex <= lockbtn.x + lockbtn.w
			&& omousey >= lockbtn.y && omousey <= lockbtn.y + lockbtn.h )
		{
			playSound(139, 64);
			lock_right_sidebar = !lock_right_sidebar;
			mousestatus[SDL_BUTTON_LEFT] = 0;
		}
	}

	pos.y += fontHeight * 2 + 8;

	SDL_Rect initialSkillPos = pos;
	//Draw skill names.
	for ( int c = 0; c < (NUMPROFICIENCIES); ++c, pos.y += (fontHeight /** 2*/) )
	{
		ttfPrintTextFormatted(fontSkill, pos.x + 4, pos.y, "%s:", getSkillLangEntry(c));
	}

	//Draw skill levels.
	pos = initialSkillPos;
	Uint32 color;
	for ( int i = 0; i < (NUMPROFICIENCIES); ++i, pos.y += (fontHeight /** 2*/) )
	{
		if ( skillCapstoneUnlocked(clientnum, i) )
		{
			color = uint32ColorGreen(*mainsurface);
		}
		else
		{
			color = uint32ColorWhite(*mainsurface);
		}

		if ( show_skill_values )
		{
			ttfPrintTextFormattedColor(fontSkill, pos.x + 4, pos.y, color, "%15d / 100", stats[clientnum]->PROFICIENCIES[i]);
		}
		else if ( stats[clientnum]->PROFICIENCIES[i] == 0 )
		{
			ttfPrintTextFormattedColor(fontSkill, pos.x + 4, pos.y, color, language[363]);
		}
		else if ( stats[clientnum]->PROFICIENCIES[i] < SKILL_LEVEL_BASIC )
		{
			ttfPrintTextFormattedColor(fontSkill, pos.x + 4, pos.y, color, language[364]);
		}
		else if ( stats[clientnum]->PROFICIENCIES[i] >= SKILL_LEVEL_BASIC && stats[clientnum]->PROFICIENCIES[i] < SKILL_LEVEL_SKILLED )
		{
			ttfPrintTextFormattedColor(fontSkill, pos.x + 4, pos.y, color, language[365]);
		}
		else if ( stats[clientnum]->PROFICIENCIES[i] >= SKILL_LEVEL_SKILLED && stats[clientnum]->PROFICIENCIES[i] < SKILL_LEVEL_EXPERT )
		{
			ttfPrintTextFormattedColor(fontSkill, pos.x + 4, pos.y, color, language[366]);
		}
		else if ( stats[clientnum]->PROFICIENCIES[i] >= SKILL_LEVEL_EXPERT && stats[clientnum]->PROFICIENCIES[i] < SKILL_LEVEL_MASTER )
		{
			ttfPrintTextFormattedColor(fontSkill, pos.x + 4, pos.y, color, language[367]);
		}
		else if ( stats[clientnum]->PROFICIENCIES[i] >= SKILL_LEVEL_MASTER && stats[clientnum]->PROFICIENCIES[i] < SKILL_LEVEL_LEGENDARY )
		{
			ttfPrintTextFormattedColor(fontSkill, pos.x + 4, pos.y, color, language[368]);
		}
		else if ( stats[clientnum]->PROFICIENCIES[i] >= SKILL_LEVEL_LEGENDARY )
		{
			ttfPrintTextFormattedColor(fontSkill, pos.x + 4, pos.y, color, language[369]);
		}
	}
}

void drawPartySheet()
{
	SDL_Rect pos;
	pos.w = 208;

	TTF_Font* fontPlayer = ttf12;
	int fontHeight = TTF12_HEIGHT;
	int fontWidth = TTF12_WIDTH;
	if ( uiscale_skillspage )
	{
		fontPlayer = ttf16;
		fontHeight = TTF16_HEIGHT;
		fontWidth = TTF16_WIDTH;
		pos.w = 276;
	}
	int playerCnt = 0;
	for ( playerCnt = MAXPLAYERS - 1; playerCnt > 0; --playerCnt )
	{
		if ( !client_disconnected[playerCnt] )
		{
			break;
		}
	}
	pos.x = xres - pos.w;
	pos.y = 32;
	pos.h = (fontHeight * 2 + 12) + ((fontHeight * 4) + 6) * (std::max(playerCnt + 1, 1));

	int numFollowers = 0;
	if ( stats[clientnum] )
	{
		numFollowers = list_Size(&stats[clientnum]->FOLLOWERS);
	}

	if ( playerCnt == 0 ) // 1 player.
	{
		if ( numFollowers == 0 )
		{
			if ( shootmode )
			{
				return; // don't show menu if not in inventory, no point reminding the player they have no friends!
			}
			pos.h = (fontHeight * 4 + 12);
		}
		else
		{
			pos.h = (fontHeight + 12);
		}
	}
	drawWindowFancy(pos.x, pos.y, pos.x + pos.w, pos.y + pos.h);
	interfacePartySheet.x = pos.x;
	interfacePartySheet.y = pos.y;
	interfacePartySheet.w = pos.w;
	interfacePartySheet.h = pos.h;

	ttfPrintTextFormatted(fontPlayer, pos.x + 4, pos.y + 8, "Party Stats");

	SDL_Rect button;
	button.x = xres - attributesright_bmp->w - 8;
	button.w = attributesright_bmp->w;
	button.y = pos.y;
	button.h = attributesright_bmp->h;
	if ( uiscale_skillspage )
	{
		button.w = attributesright_bmp->w * 1.3;
		button.x = xres - button.w - 8;
		button.y = pos.y;
		button.h = attributesright_bmp->h * 1.3;
	}


	if ( mousestatus[SDL_BUTTON_LEFT] && !shootmode )
	{
		if ( omousex >= button.x && omousex <= button.x + button.w
			&& omousey >= button.y && omousey <= button.y + button.h )
		{
			buttonclick = 14;
			playSound(139, 64);
			if ( proficienciesPage == 0 )
			{
				proficienciesPage = 1;
			}
			else
			{
				proficienciesPage = 0;
			}
			mousestatus[SDL_BUTTON_LEFT] = 0;
		}
	}
	if ( buttonclick == 14 )
	{
		drawImageScaled(attributesright_bmp, nullptr, &button);
	}
	else
	{
		drawImageScaled(attributesrightunclicked_bmp, nullptr, &button);
	}

	SDL_Rect lockbtn = button;
	lockbtn.h = 24;
	lockbtn.w = 24;
	lockbtn.y += 2;
	if ( uiscale_skillspage )
	{
		lockbtn.h = 24 * 1.3;
		lockbtn.w = 24 * 1.3;
		lockbtn.x -= 32 * 1.3;
	}
	else
	{
		lockbtn.x -= 32;
	}
	if ( lock_right_sidebar )
	{
		drawImageScaled(sidebar_lock_bmp, nullptr, &lockbtn);
	}
	else
	{
		drawImageScaled(sidebar_unlock_bmp, nullptr, &lockbtn);
	}

	if ( mousestatus[SDL_BUTTON_LEFT] && !shootmode )
	{
		if ( omousex >= lockbtn.x && omousex <= lockbtn.x + lockbtn.w
			&& omousey >= lockbtn.y && omousey <= lockbtn.y + lockbtn.h )
		{
			playSound(139, 64);
			lock_right_sidebar = !lock_right_sidebar;
			mousestatus[SDL_BUTTON_LEFT] = 0;
		}
	}

	pos.y += fontHeight * 2 + 4;

	SDL_Rect initialSkillPos = pos;
	SDL_Rect playerBar;

	
	if ( playerCnt == 0 && numFollowers == 0 ) // 1 player.
	{
		ttfPrintTextFormatted(fontPlayer, pos.x + 32, pos.y + 8, "No party members");
	}

	//Draw party stats
	Uint32 color = uint32ColorWhite(*mainsurface);
	if ( playerCnt > 0 )
	{
		for ( int i = 0; i < MAXPLAYERS; ++i, pos.y += (fontHeight * 4) + 6 )
		{
			if ( !client_disconnected[i] && stats[i] )
			{
				ttfPrintTextFormattedColor(fontPlayer, pos.x + 12, pos.y, color, "[%d] %s", i, stats[i]->name);

				ttfPrintTextFormattedColor(fontPlayer, pos.x + 12, pos.y + fontHeight, color, "%s", playerClassLangEntry(client_classes[i]));
				ttfPrintTextFormattedColor(fontPlayer, xres - 8 * 12, pos.y + fontHeight, color, "LVL %2d", stats[i]->LVL);

				playerBar.x = pos.x + 64;
				playerBar.w = 10 * 11;
				if ( uiscale_skillspage )
				{
					playerBar.x += 10;
					playerBar.w += 48;
				}
				playerBar.y = pos.y + fontHeight * 2 + 1;
				playerBar.h = fontHeight;
				// draw tooltip with blue outline
				drawTooltip(&playerBar);
				// draw faint red bar underneath
				playerBar.x += 1;
				drawRect(&playerBar, SDL_MapRGB(mainsurface->format, 48, 0, 0), 255);

				// draw main red bar for current HP
				playerBar.w = (playerBar.w) * (static_cast<double>(stats[i]->HP) / stats[i]->MAXHP);
				drawRect(&playerBar, SDL_MapRGB(mainsurface->format, 128, 0, 0), 255);

				// draw HP values
				ttfPrintTextFormattedColor(fontPlayer, pos.x + 32, pos.y + fontHeight * 2 + 4, color, "HP:  %3d / %3d", stats[i]->HP, stats[i]->MAXHP);

				playerBar.x = pos.x + 64;
				playerBar.w = 10 * 11;
				if ( uiscale_skillspage )
				{
					playerBar.x += 10;
					playerBar.w += 48;
				}
				playerBar.y = pos.y + fontHeight * 3 + 1;
				// draw tooltip with blue outline
				drawTooltip(&playerBar);
				playerBar.x += 1;
				// draw faint blue bar underneath
				drawRect(&playerBar, SDL_MapRGB(mainsurface->format, 0, 0, 48), 255);

				// draw blue red bar for current MP
				playerBar.w = (playerBar.w) * (static_cast<double>(stats[i]->MP) / stats[i]->MAXMP);
				drawRect(&playerBar, SDL_MapRGB(mainsurface->format, 0, 24, 128), 255);

				// draw MP values
				ttfPrintTextFormattedColor(fontPlayer, pos.x + 32 , pos.y + fontHeight * 3 + 4, color, "MP:  %3d / %3d", stats[i]->MP, stats[i]->MAXMP);
			}
		}
	}



	// draw follower stats
	if ( numFollowers > 0 )
	{
		int monstersToDisplay = FollowerMenu.maxMonstersToDraw;
		if ( playerCnt != 0 )
		{
			pos.y -= (fontHeight * 4) * 2;
			pos.y += std::max(playerCnt - 1, 0) * (fontHeight * 4 + 8);
			monstersToDisplay = FollowerMenu.numMonstersToDrawInParty();
		}
		int i = 0;
		SDL_Rect monsterEntryWindow;
		monsterEntryWindow.x = pos.x + 8;
		monsterEntryWindow.w = pos.w - 8;
		if ( numFollowers > (monstersToDisplay + 1) )
		{
			monsterEntryWindow.w -= 16;
		}
		SDL_Rect slider = monsterEntryWindow;
		slider.y = pos.y;

		for ( node_t* node = stats[clientnum]->FOLLOWERS.first; node != nullptr; node = node->next, ++i )
		{
			Entity* follower = uidToEntity(*((Uint32*)node->element));
			if ( follower )
			{
				Stat* followerStats = follower->getStats();
				if ( followerStats )
				{
					monsterEntryWindow.y = pos.y;
					monsterEntryWindow.h = fontHeight * 2 + 12;

					bool hideDetail = false;
					if ( numFollowers > monstersToDisplay )
					{
						if ( i < FollowerMenu.sidebarScrollIndex )
						{
							hideDetail = true;
						}
						else if ( i > FollowerMenu.sidebarScrollIndex + monstersToDisplay )
						{
							hideDetail = true;
						}
					}

					if ( !hideDetail )
					{
						drawWindowFancy(monsterEntryWindow.x, monsterEntryWindow.y, 
							monsterEntryWindow.x + monsterEntryWindow.w, monsterEntryWindow.y + monsterEntryWindow.h);

						if ( !FollowerMenu.recentEntity )
						{
							FollowerMenu.recentEntity = follower;
						}
						if ( FollowerMenu.recentEntity == follower )
						{
							// draw highlight on current selected monster.
							drawRect(&monsterEntryWindow, uint32ColorBaronyBlue(*mainsurface), 32);
							// ttfPrintText(ttf16, xres - 20, monsterEntryWindow.y + monsterEntryWindow.h / 2 - fontHeight / 2, "<");
						}

						if ( stats[clientnum] && stats[clientnum]->HP > 0 && !shootmode 
							&& (mousestatus[SDL_BUTTON_LEFT] || (*inputPressed(impulses[IN_USE]) || *inputPressed(joyimpulses[INJOY_GAME_USE]))) )
						{
							bool inBounds = mouseInBounds(monsterEntryWindow.x, monsterEntryWindow.x + monsterEntryWindow.w,
								monsterEntryWindow.y, monsterEntryWindow.y + monsterEntryWindow.h);
							if ( inBounds )
							{
								if ( mousestatus[SDL_BUTTON_LEFT] )
								{
									FollowerMenu.recentEntity = follower;
									playSound(139, 64);
									FollowerMenu.accessedMenuFromPartySheet = true;
									FollowerMenu.partySheetMouseX = omousex;
									FollowerMenu.partySheetMouseY = omousey;
									mousestatus[SDL_BUTTON_LEFT] = 0;
									if ( FollowerMenu.recentEntity )
									{
										createParticleFollowerCommand(FollowerMenu.recentEntity->x, FollowerMenu.recentEntity->y, 0, 174);
									}
								}
								else if ( (*inputPressed(impulses[IN_USE]) || *inputPressed(joyimpulses[INJOY_GAME_USE])) )
								{
									FollowerMenu.followerToCommand = follower;
									FollowerMenu.recentEntity = follower;
									FollowerMenu.accessedMenuFromPartySheet = true;
									FollowerMenu.partySheetMouseX = omousex;
									FollowerMenu.partySheetMouseY = omousey;
									FollowerMenu.initFollowerMenuGUICursor();
									FollowerMenu.updateScrollPartySheet();
									if ( FollowerMenu.recentEntity )
									{
										createParticleFollowerCommand(FollowerMenu.recentEntity->x, FollowerMenu.recentEntity->y, 0, 174);
									}
								}
							}
						}

						pos.y += 6;
						char name[16] = "";
						if ( strcmp(followerStats->name, "") && strcmp(followerStats->name, "nothing") )
						{
							if ( strlen(followerStats->name) > 10 )
							{
								strncpy(name, followerStats->name, 8);
								strcat(name, "..");
								ttfPrintTextFormattedColor(fontPlayer, pos.x + 20, pos.y, color, "%s", name);
							}
							else
							{
								ttfPrintTextFormattedColor(fontPlayer, pos.x + 20, pos.y, color, "%s", followerStats->name);
							}
						}
						else
						{
							if ( strlen(monstertypename[followerStats->type]) > 10 )
							{
								strncpy(name, monstertypename[followerStats->type], 8);
								strcat(name, "..");
								ttfPrintTextFormattedColor(fontPlayer, pos.x + 20, pos.y, color, "%s", name);
							}
							else
							{
								ttfPrintTextFormattedColor(fontPlayer, pos.x + 20, pos.y, color, "%s", monstertypename[followerStats->type]);
							}
						}
						ttfPrintTextFormattedColor(fontPlayer, xres - 8 * 11, pos.y, color, "LVL %2d", followerStats->LVL);

						playerBar.x = pos.x + 64;
						playerBar.w = 10 * 11;
						if ( uiscale_skillspage )
						{
							playerBar.x += 10;
							playerBar.w += 48;
						}
						playerBar.y = pos.y + fontHeight + 1;
						playerBar.h = fontHeight;
						// draw tooltip with blue outline
						drawTooltip(&playerBar);
						// draw faint red bar underneath
						playerBar.x += 1;
						drawRect(&playerBar, SDL_MapRGB(mainsurface->format, 48, 0, 0), 255);

						// draw main red bar for current HP
						playerBar.w = (playerBar.w) * (static_cast<double>(followerStats->HP) / followerStats->MAXHP);
						drawRect(&playerBar, SDL_MapRGB(mainsurface->format, 128, 0, 0), 255);

						// draw HP values
						ttfPrintTextFormattedColor(fontPlayer, pos.x + 32, pos.y + fontHeight + 4, color, "HP:  %3d / %3d", followerStats->HP, followerStats->MAXHP);
						pos.y += (fontHeight * 2 + 6);
					}
				}
			}
		}
		slider.x = xres - 16;
		slider.w = 16;
		slider.h = (fontHeight * 2 + 12) * (std::min(monstersToDisplay + 1, numFollowers));
		interfacePartySheet.h += slider.h + 6;

		if ( numFollowers > (monstersToDisplay + 1) )
		{
			drawDepressed(slider.x, slider.y, slider.x + slider.w,
				slider.y + slider.h);

			bool mouseInScrollbarTotalHeight = mouseInBounds(xres - monsterEntryWindow.w, xres, slider.y,
				slider.y + slider.h);

			if ( mousestatus[SDL_BUTTON_WHEELDOWN] && mouseInScrollbarTotalHeight )
			{
				mousestatus[SDL_BUTTON_WHEELDOWN] = 0;
				FollowerMenu.sidebarScrollIndex = std::min(FollowerMenu.sidebarScrollIndex + 1, numFollowers - monstersToDisplay - 1);
			}
			else if ( mousestatus[SDL_BUTTON_WHEELUP] && mouseInScrollbarTotalHeight )
			{
				mousestatus[SDL_BUTTON_WHEELUP] = 0;
				FollowerMenu.sidebarScrollIndex = std::max(FollowerMenu.sidebarScrollIndex - 1, 0);
			}

			slider.h *= (1 / static_cast<real_t>(std::max(1, numFollowers - monstersToDisplay)));
			slider.y += slider.h * FollowerMenu.sidebarScrollIndex;
			drawWindowFancy(slider.x, slider.y, slider.x + slider.w, slider.y + slider.h);
			if ( mouseInScrollbarTotalHeight && mousestatus[SDL_BUTTON_LEFT] )
			{
				if ( !mouseInBounds(xres - monsterEntryWindow.w, xres, slider.y,
					slider.y + slider.h) )
				{
					if ( omousey < slider.y )
					{
						FollowerMenu.sidebarScrollIndex = std::max(FollowerMenu.sidebarScrollIndex - 1, 0);
						mousestatus[SDL_BUTTON_LEFT] = 0;
					}
					else if ( omousey > slider.y + slider.h )
					{
						FollowerMenu.sidebarScrollIndex = std::min(FollowerMenu.sidebarScrollIndex + 1, numFollowers - monstersToDisplay - 1);
						mousestatus[SDL_BUTTON_LEFT] = 0;
					}
				}
			}
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

	if ( uiscale_charactersheet )
	{
		pad_y += 86;
		off_h = TTF16_HEIGHT - 4;
		off_w = 280;
	}

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

	char tooltipText[6][4][128] =
	{
		{
			"base:  %2d ",
			"bonus: %2d ",
			""
		},
		{
			"base:  %2d ",
			"bonus: %2d ",
			""
		},
		{
			"base:  %2d ",
			"bonus: %2d ",
			"HP regen rate: 1 / %2.1fs",
			""
		},
		{
			"base:  %2d ",
			"bonus: %2d ",
			"MP regen rate: 1 / %2.1fs",
			"magic resistance: %2.1f%% "
		},
		{
			"base:  %2d ",
			"bonus: %2d ",
			""
		},
		{
			"base:  %2d ",
			"bonus: %2d ",
			""
		}
	};

	char buf[128] = "";
	int numInfoLines = 0;
	Sint32 statBase = 0;
	Sint32 statBonus = 0;

	SDL_Surface *tmp_bmp = NULL;

	Entity* playerEntity = nullptr;
	if ( players[clientnum] )
	{
		playerEntity = players[clientnum]->entity;
	}

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
					statBonus = statGetSTR(tmpStat, playerEntity) - statBase;
					break;
				case 1:
					numInfoLines = 2;
					tmp_bmp = dex_bmp64;
					statBase = tmpStat->DEX;
					statBonus = statGetDEX(tmpStat, playerEntity) - statBase;
					break;
				case 2:
					numInfoLines = 3;
					tmp_bmp = con_bmp64;
					statBase = tmpStat->CON;
					statBonus = statGetCON(tmpStat, playerEntity) - statBase;
					break;
				case 3:
					numInfoLines = 4;
					tmp_bmp = int_bmp64;
					statBase = tmpStat->INT;
					statBonus = statGetINT(tmpStat, playerEntity) - statBase;
					break;
				case 4:
					numInfoLines = 2;
					tmp_bmp = per_bmp64;
					statBase = tmpStat->PER;
					statBonus = statGetPER(tmpStat, playerEntity) - statBase;
					break;
				case 5:
					numInfoLines = 2;
					tmp_bmp = chr_bmp64;
					statBase = tmpStat->CHR;
					statBonus = statGetCHR(tmpStat, playerEntity) - statBase;
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
				src.w = 180;
				for ( j = 0; j < numInfoLines; j++ )
				{
					src.w = std::max(longestline(tooltipText[i][j]) * 12, src.w);
				}
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
					else if ( j == 2 )
					{
						if ( i == 3 )
						{
							Entity* tmp = nullptr;
							if ( players[clientnum] && players[clientnum]->entity )
							{
								tmp = players[clientnum]->entity;
								real_t regen = (static_cast<real_t>(tmp->getManaRegenInterval(*tmpStat)) / TICKS_PER_SECOND);
								snprintf(buf, longestline(tooltipText[i][j]), tooltipText[i][j], regen);
								if ( regen < static_cast<real_t>(tmp->getBaseManaRegen(*tmpStat)) / TICKS_PER_SECOND)
								{
									color = uint32ColorGreen(*mainsurface);
								}
							}
							else
							{
								snprintf(buf, longestline(tooltipText[i][j]), tooltipText[i][j], 0.f);
							}
						}
						else if ( i == 2 )
						{
							Entity* tmp = nullptr;
							if ( players[clientnum] && players[clientnum]->entity )
							{
								tmp = players[clientnum]->entity;
								real_t regen = (static_cast<real_t>(tmp->getHealthRegenInterval(*tmpStat)) / TICKS_PER_SECOND);
								if ( regen < 0 )
								{
									regen = 0.f;
									color = uint32ColorRed(*mainsurface);
									snprintf(buf, longestline("HP regen rate: 0 / %2.1fs"), "HP regen rate: 0 / %2.1fs", (static_cast<real_t>(HEAL_TIME) / TICKS_PER_SECOND));
								}
								else if ( regen < HEAL_TIME / TICKS_PER_SECOND )
								{
									color = uint32ColorGreen(*mainsurface);
								}
								if ( regen > 0.f )
								{
									snprintf(buf, longestline(tooltipText[i][j]), tooltipText[i][j], regen);
								}
							}
							else
							{
								snprintf(buf, longestline(tooltipText[i][j]), tooltipText[i][j], 0.f);
							}
						}
					}
					else if ( j == 3 )
					{
						if ( i == 3 )
						{
							Entity* tmp = nullptr;
							real_t resistance = 0.f;
							if ( players[clientnum] && players[clientnum]->entity )
							{
								tmp = players[clientnum]->entity;
								real_t resistance = 100 - 100 / (tmp->getMagicResistance() + 1);
								snprintf(buf, longestline(tooltipText[i][j]), tooltipText[i][j], resistance);
								if ( resistance > 0.f )
								{
									color = uint32ColorGreen(*mainsurface);
								}
							}
							else
							{
								snprintf(buf, longestline(tooltipText[i][j]), tooltipText[i][j], 0);
							}
						}
					}
					ttfPrintTextColor(ttf12, infoText_x, infoText_y, color, false, buf);
				}
			}
			numInfoLines = 0;
			pad_y += off_h;
		}
	}
}

Sint32 displayAttackPower(Sint32 output[6])
{
	Sint32 attack = 0;
	Entity* entity = nullptr;
	if ( players[clientnum] && (entity = players[clientnum]->entity) )
	{
		if ( stats[clientnum] )
		{
			if ( !stats[clientnum]->weapon )
			{
				// fists
				attack += entity->getAttack();
				output[0] = 0; // melee
				output[1] = attack;
				output[2] = (stats[clientnum]->PROFICIENCIES[PRO_UNARMED] / 20); // bonus from proficiency
				output[3] = entity->getSTR(); // bonus from main attribute
				output[5] = attack - entity->getSTR() - BASE_PLAYER_UNARMED_DAMAGE - output[2]; // bonus from equipment
				// get damage variances.
				output[4] = (attack / 2) * (100 - stats[clientnum]->PROFICIENCIES[PRO_UNARMED]) / 100.f;
				attack -= (output[4] / 2); // attack is the midpoint between max and min damage.
				output[4] = ((output[4] / 2) / static_cast<real_t>(attack)) * 100.f;// return percent variance
				output[1] = attack;
			}
			else
			{
				int weaponskill = getWeaponSkill(stats[clientnum]->weapon);
				real_t variance = 0;
				if ( weaponskill == PRO_RANGED )
				{
					if ( isRangedWeapon(*stats[clientnum]->weapon) )
					{
						attack += entity->getRangedAttack();
						output[0] = 1; // ranged
						output[1] = attack;
						output[2] = stats[clientnum]->weapon->weaponGetAttack(stats[clientnum]); // bonus from weapon
						output[3] = entity->getDEX(); // bonus from main attribute
						//output[4] = attack - output[2] - output[3] - BASE_RANGED_DAMAGE; // bonus from proficiency

						output[4] = (attack / 2) * (100 - stats[clientnum]->PROFICIENCIES[weaponskill]) / 100.f;
						attack -= (output[4] / 2);
						output[4] = ((output[4] / 2) / static_cast<real_t>(attack)) * 100.f;// return percent variance
						output[1] = attack;
					}
					else
					{
						attack += entity->getThrownAttack();
						output[0] = 2; // thrown
						output[1] = attack;
						output[2] = stats[clientnum]->weapon->weaponGetAttack(stats[clientnum]); // bonus from weapon
						output[3] = 0;
						output[4] = attack - output[2] - BASE_THROWN_DAMAGE; // bonus from proficiency
						output[5] = 0; // bonus from equipment
					}
				}
				else if ( (weaponskill >= PRO_SWORD && weaponskill <= PRO_POLEARM) )
				{
					// melee weapon
					attack += entity->getAttack();
					output[0] = 3; // melee
					output[1] = attack;
					output[2] = stats[clientnum]->weapon->weaponGetAttack(stats[clientnum]); // bonus from weapon
					output[3] = entity->getSTR(); // bonus from main attribute
					if ( weaponskill == PRO_AXE )
					{
						output[5] = 1; // bonus from equipment
						attack += 1;
					}
					// get damage variances.
					if ( weaponskill == PRO_POLEARM )
					{
						output[4] = (attack / 3) * (100 - stats[clientnum]->PROFICIENCIES[weaponskill]) / 100.f;
					}
					else
					{
						output[4] = (attack / 2) * (100 - stats[clientnum]->PROFICIENCIES[weaponskill]) / 100.f;
					}
					attack -= (output[4] / 2); // attack is the midpoint between max and min damage.
					output[4] = ((output[4] / 2) / static_cast<real_t>(attack)) * 100.f;// return percent variance
					output[1] = attack;
				}
				else if ( itemCategory(stats[clientnum]->weapon) == MAGICSTAFF ) // staffs.
				{
					attack = 0;
					output[0] = 5; // staffs
					output[1] = attack;
					output[2] = 0; // bonus from weapon
					output[3] = 0; // bonus from main attribute
					output[4] = 0; // bonus from proficiency
					output[5] = 0; // bonus from equipment
				}
				else // tools etc.
				{
					attack += entity->getAttack();
					output[0] = 4; // tools
					output[1] = attack;
					output[2] = 0; // bonus from weapon
					output[3] = entity->getSTR(); // bonus from main attribute
					output[4] = 0; // bonus from proficiency
					output[5] = attack - entity->getSTR() - BASE_MELEE_DAMAGE; // bonus from equipment
				}
			}
		}
	}
	else
	{
		attack = 0;
	}
	return attack;
}

void attackHoverText(Sint32 input[6])
{
	int pad_y = 346; // 262 px.
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
	int tooltip_text_pad_x = 16;
	int numInfoLines = 3;
	char buf[128] = "";

	if ( uiscale_charactersheet )
	{
		off_h = TTF16_HEIGHT - 4;
		pad_y += 126;
		off_w = 280;
	}

	if ( attributespage == 0 )
	{
		if ( mouseInBounds(pad_x, pad_x + off_w, pad_y, pad_y + off_h) )
		{
			char tooltipHeader[32] = "";
			switch ( input[0] )
			{
				case 0: // fists
					snprintf(tooltipHeader, strlen(language[2529]), language[2529]);
					numInfoLines = 4;
					break;
				case 1: // ranged
					snprintf(tooltipHeader, strlen(language[2530]), language[2530]);
					numInfoLines = 3;
					break;
				case 2: // thrown
					snprintf(tooltipHeader, strlen(language[2531]), language[2531]);
					numInfoLines = 2;
					break;
				case 3: // melee
					snprintf(tooltipHeader, strlen(language[2532]), language[2532]);
					numInfoLines = 4;
					break;
				case 4: // tools
					snprintf(tooltipHeader, strlen(language[2540]), language[2540]);
					numInfoLines = 2;
					break;
				case 5: // staffs
					snprintf(tooltipHeader, strlen(language[2541]), language[2541]);
					numInfoLines = 0;
					break;
				default:
					break;
			}

			// get tooltip draw location.
			src.x = mousex + tooltip_offset_x;
			src.y = mousey + tooltip_offset_y;
			src.h = std::max(tooltip_base_h * (numInfoLines + 1) + tooltip_pad_h, tooltip_base_h * (2) + tooltip_pad_h);
			src.w = 256;
			drawTooltip(&src);

			// draw header
			Uint32 color = uint32ColorWhite(*mainsurface);
			ttfPrintTextColor(ttf12, src.x + 4, src.y + 4, color, false, tooltipHeader);
			if ( input[1] >= 0 )
			{
				// attack >= 0
				color = SDL_MapRGB(mainsurface->format, 0, 255, 255);
			}
			else
			{
				// attack < 0
				color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
			}
			snprintf(tooltipHeader, 32, language[2533], input[1]);
			ttfPrintTextColor(ttf12, src.x + 4, src.y + 4, color, false, tooltipHeader);

			for ( j = 0; j < numInfoLines && numInfoLines > 0; j++ )
			{
				int infoText_x = src.x + 4 + tooltip_text_pad_x;
				int infoText_y = src.y + 4 + (tooltip_base_h * (j + 1));
				Uint32 color = uint32ColorWhite(*mainsurface);

				if ( input[0] == 0 ) // fists
				{
					switch ( j )
					{
						case 0:
							snprintf(buf, longestline(language[3209]), language[3209], input[2]);
							break;
						case 1:
							snprintf(buf, longestline(language[2534]), language[2534], input[3]);
							break;
						case 2:
							snprintf(buf, longestline(language[2539]), language[2539], input[4]);
							break;
						case 3:
							snprintf(buf, longestline(language[2536]), language[2536], input[5]);
						default:
							break;
					}
				}
				else if ( input[0] == 1 ) // ranged
				{
					switch ( j )
					{
						case 0:
							snprintf(buf, longestline(language[2538]), language[2538], input[2]);
							break;
						case 1:
							snprintf(buf, longestline(language[2535]), language[2535], input[3]);
							break;
						case 2:
							snprintf(buf, longestline(language[2539]), language[2539], input[4]);
							break;
						default:
							break;
					}
				}
				else if ( input[0] == 2 ) // thrown
				{
					switch ( j )
					{
						case 0:
							snprintf(buf, longestline(language[2538]), language[2538], input[2]);
							break;
						case 1:
							snprintf(buf, longestline(language[2537]), language[2537], input[4]);
							break;
						default:
							break;
					}
				}
				else if ( input[0] == 3 ) // melee weapons
				{
					switch ( j )
					{
						case 0:
							snprintf(buf, longestline(language[2538]), language[2538], input[2]);
							break;
						case 1:
							snprintf(buf, longestline(language[2534]), language[2534], input[3]);
							break;
						case 2:
							snprintf(buf, longestline(language[2539]), language[2539], input[4]);
							break;
						case 3:
							snprintf(buf, longestline(language[2536]), language[2536], input[5]);
						default:
							break;
					}
				}
				else if ( input[0] == 4 ) // tools
				{
					switch ( j )
					{
						case 0:
							snprintf(buf, longestline(language[2534]), language[2534], input[3]);
							break;
						case 1:
							snprintf(buf, longestline(language[2536]), language[2536], input[5]);
							break;
						default:
							break;
					}
				}
				else if ( input[0] == 5 ) // staff
				{
					
				}
				ttfPrintTextColor(ttf12, infoText_x, infoText_y, color, false, buf);
			}
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
