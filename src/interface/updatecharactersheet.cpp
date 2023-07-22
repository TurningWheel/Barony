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
#include "../engine/audio/sound.hpp"
#include "../magic/magic.hpp"
#include "../menu.hpp"
#include "../net.hpp"
#include "../scores.hpp"
#include "../ui/GameUI.hpp"
#include "../prng.hpp"

void statsHoverText(const int player, Stat* tmpStat);

/*-------------------------------------------------------------------------------

	updateCharacterSheet

	Draws the character sheet and processes all interaction with it

-------------------------------------------------------------------------------*/

void updateCharacterSheet(const int player)
{
	SDL_Rect pos;
	node_t* node = NULL;
	Entity* entity = NULL;
	Item* item = NULL;

	const int x1 = players[player]->camera_x1();
	const int y1 = players[player]->camera_y1();

	const Sint32 mousex = inputs.getMouse(player, Inputs::X);
	const Sint32 mousey = inputs.getMouse(player, Inputs::Y);

	// mostly a copy of the code below.
	// sets values into players[player]->characterSheet.partySheetBox for elsewhere
	players[player]->characterSheet.setDefaultCharacterSheetBox(); 

	// draw window
	pos.x = x1 + 8;
	pos.y = y1 + 8;
	pos.w = 208;
	pos.h = 180;

	SDL_Rect& statWindowBox = players[player]->characterSheet.statsSheetBox;
	drawWindowFancy(statWindowBox.x, statWindowBox.y, statWindowBox.x + statWindowBox.w, statWindowBox.y + statWindowBox.h);

	TTF_Font* fontStat = ttf12;
	int text_x = statWindowBox.x + 8;
	int text_y = statWindowBox.y;
	int pad_y = 12;
	int fontWidth = TTF12_WIDTH;
	if ( uiscale_charactersheet )
	{
		fontStat = ttf16;
		pad_y = 18;
		fontWidth = TTF16_WIDTH;
	}
	text_y = statWindowBox.y + 6;
	ttfPrintTextFormatted(fontStat, text_x, text_y, "%s", stats[player]->name);
	text_y += pad_y;
	ttfPrintTextFormatted(fontStat, text_x, text_y, Language::get(359), stats[player]->LVL, playerClassLangEntry(client_classes[player], player));
	text_y += pad_y;
	ttfPrintTextFormatted(fontStat, text_x, text_y, Language::get(360), stats[player]->EXP);
	text_y += pad_y;
	ttfPrintTextFormatted(fontStat, text_x, text_y, Language::get(361), currentlevel);

	Entity* playerEntity = nullptr;
	if ( players[player] )
	{
		playerEntity = players[player]->entity;
	}

	// attributes
	char statText[64] = "";
	//Uint32 statColor = uint32ColorWhite;
	text_y += pad_y * 2;
	snprintf(statText, 64, Language::get(1200), stats[player]->STR);
	ttfPrintTextFormatted(fontStat, text_x, text_y, statText);
	printStatBonus(fontStat, stats[player]->STR, statGetSTR(stats[player], playerEntity), text_x + longestline(statText) * fontWidth, text_y);

	text_y += pad_y;
	snprintf(statText, 64, Language::get(1201), stats[player]->DEX);
	ttfPrintTextFormatted(fontStat, text_x, text_y, statText);
	printStatBonus(fontStat, stats[player]->DEX, statGetDEX(stats[player], playerEntity), text_x + longestline(statText) * fontWidth, text_y);

	text_y += pad_y;
	snprintf(statText, 64, Language::get(1202), stats[player]->CON);
	ttfPrintTextFormatted(fontStat, text_x, text_y, statText);
	printStatBonus(fontStat, stats[player]->CON, statGetCON(stats[player], playerEntity), text_x + longestline(statText) * fontWidth, text_y);

	text_y += pad_y;
	snprintf(statText, 64, Language::get(1203), stats[player]->INT);
	ttfPrintTextFormatted(fontStat, text_x, text_y, statText);
	printStatBonus(fontStat, stats[player]->INT, statGetINT(stats[player], playerEntity), text_x + longestline(statText) * fontWidth, text_y);

	text_y += pad_y;
	snprintf(statText, 64, Language::get(1204), stats[player]->PER);
	ttfPrintTextFormatted(fontStat, text_x, text_y, statText);
	printStatBonus(fontStat, stats[player]->PER, statGetPER(stats[player], playerEntity), text_x + longestline(statText) * fontWidth, text_y);

	text_y += pad_y;
	snprintf(statText, 64, Language::get(1205), stats[player]->CHR);
	ttfPrintTextFormatted(fontStat, text_x, text_y, statText);
	printStatBonus(fontStat, stats[player]->CHR, statGetCHR(stats[player], playerEntity), text_x + longestline(statText) * fontWidth, text_y);

	// armor, gold, and weight
	AttackHoverText_t attackHoverTextInfo;
	text_y += pad_y * 2;
	ttfPrintTextFormatted(fontStat, text_x, text_y, Language::get(2542), displayAttackPower(player, attackHoverTextInfo));

	text_y += pad_y;
	ttfPrintTextFormatted(fontStat, text_x, text_y, Language::get(371), AC(stats[player]));

	text_y += pad_y;
	ttfPrintTextFormatted(fontStat, text_x, text_y, Language::get(370), stats[player]->GOLD);
	Sint32 weight = 0;
	for ( node = stats[player]->inventory.first; node != NULL; node = node->next )
	{
		item = (Item*)node->element;
		if ( item )
		{
			weight += item->getWeight();
		}
	}
	weight += stats[player]->GOLD / 100;
	text_y += pad_y;
	ttfPrintTextFormatted(fontStat, text_x, text_y, Language::get(372), weight);

	statsHoverText(player, stats[player]);
	//attackHoverText(player, attackInfo);

	// gold hover text.
	SDL_Rect src;
	src.x = mousex + 16;
	src.y = mousey + 16;
	src.h = TTF12_HEIGHT + 8;
	src.w = ( longestline(Language::get(2968)) + (int)strlen(getInputName(impulses[IN_USE])) ) * TTF12_WIDTH + 4;
	bool dropGold = false;
	if ( mouseInBounds(player, statWindowBox.x + 8, statWindowBox.x + statWindowBox.w, text_y - pad_y, text_y) )
	{
		drawTooltip(&src);
		ttfPrintTextFormatted(ttf12, src.x + 4, src.y + 6, Language::get(2968), getInputName(impulses[IN_USE]));
		if ( Input::inputs[player].consumeBinaryToggle("Use") )
		{
			dropGold = true;
		}
	}

	if ( dropGold && stats[player] && stats[player]->HP > 0 && players[player]->entity )
	{
		int amount = 100;
		if ( stats[player]->GOLD - amount < 0 )
		{
			amount = stats[player]->GOLD;
		}
		if ( amount == 0 )
		{
			messagePlayer(player, MESSAGE_INVENTORY, Language::get(2593));
			return;
		}
		stats[player]->GOLD -= amount;
		stats[player]->GOLD = std::max(stats[player]->GOLD, 0);

		if ( multiplayer == CLIENT )
		{
			//Tell the server we dropped some gold.
			strcpy((char*)net_packet->data, "DGLD");
			net_packet->data[4] = player;
			SDLNet_Write32(amount, &net_packet->data[5]);
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 9;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
		else
		{
			//Drop gold.
			int x = std::min<int>(std::max(0, (int)(players[player]->entity->x / 16)), map.width - 1);
			int y = std::min<int>(std::max(0, (int)(players[player]->entity->y / 16)), map.height - 1);
			if ( map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height] )
			{
				entity = newEntity(130, 0, map.entities, nullptr); // 130 = goldbag model
				entity->sizex = 4;
				entity->sizey = 4;
				entity->x = players[player]->entity->x;
				entity->y = players[player]->entity->y;
				entity->z = 6;
				entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
				entity->flags[PASSABLE] = true;
				entity->flags[UPDATENEEDED] = true;
				entity->behavior = &actGoldBag;
				entity->goldAmount = amount; // amount
			}
			playSoundEntity(players[player]->entity, 242 + local_rng.rand() % 4, 64);
		}
		messagePlayer(player, MESSAGE_INVENTORY, Language::get(2594), amount);
	}
}

void Player::CharacterSheet_t::setDefaultCharacterSheetBox()
{
	const int x1 = players[player.playernum]->camera_x1();
	const int y1 = players[player.playernum]->camera_y1();

	// draw window
	SDL_Rect pos;
	pos.x = x1 + 8;
	pos.y = y1 + 8;
	pos.w = 208;
	pos.h = 180;

	int statWindowY = y1 + 196;
	int statWindowY2 = y1 + 404;
	if ( uiscale_charactersheet )
	{
		pos.h = 236;
		pos.w = 276;
		statWindowY = y1 + pos.h + 16;
		statWindowY2 = y1 + 554;
	}

	characterSheetBox.x = pos.x - 8;
	characterSheetBox.y = pos.y - 8;
	characterSheetBox.w = pos.w + 16;
	characterSheetBox.h = statWindowY;


	if ( players[player.playernum]->inventoryUI.frame )
	{
		if ( auto inventoryBaseFrame = players[player.playernum]->inventoryUI.frame->findFrame("inventory base") )
		{
			SDL_Rect pos = inventoryBaseFrame->getAbsoluteSize();
			statsSheetBox.x = (pos.x + pos.w) * xres / Frame::virtualScreenX;
			statsSheetBox.y = pos.y * xres / Frame::virtualScreenY;
		}
	}
	else
	{
		statsSheetBox.x = characterSheetBox.x;
		statsSheetBox.y = statWindowY;
		statsSheetBox.w = characterSheetBox.w;
		statsSheetBox.h = statWindowY2 - statWindowY;
	}

	/*if ( player.inventoryUI.bNewInventoryLayout )
	{
		statsSheetBox.x = characterSheetBox.x + characterSheetBox.w;
		statsSheetBox.y = characterSheetBox.y;

		characterSheetBox.h = pos.h + 16;
	}*/
}

void Player::CharacterSheet_t::setDefaultPartySheetBox()
{
	const int x2 = players[player.playernum]->camera_x2();
	const int y1 = players[player.playernum]->camera_y1();

	partySheetBox.w = 208;

	TTF_Font* fontPlayer = ttf12;
	int fontHeight = TTF12_HEIGHT;
	int fontWidth = TTF12_WIDTH;
	if ( uiscale_skillspage )
	{
		fontPlayer = ttf16;
		fontHeight = TTF16_HEIGHT;
		fontWidth = TTF16_WIDTH;
		partySheetBox.w = 276;
	}
	int playerCnt = 0;
	for ( playerCnt = MAXPLAYERS - 1; playerCnt > 0; --playerCnt )
	{
		if ( !client_disconnected[playerCnt] )
		{
			break;
		}
	}
	partySheetBox.x = x2 - partySheetBox.w;
	partySheetBox.y = y1 + 32;
	partySheetBox.h = (fontHeight * 2 + 12) + ((fontHeight * 4) + 6) * (std::max(playerCnt + 1, 1));

	int numFollowers = 0;
	if ( stats[player.playernum] )
	{
		numFollowers = list_Size(&stats[player.playernum]->FOLLOWERS);
	}

	if ( playerCnt == 0 ) // 1 player.
	{
		if ( numFollowers == 0 )
		{
			if ( players[player.playernum]->shootmode )
			{
				// don't show menu if not in inventory, no point reminding the player they have no friends!
			}
			else
			{
				partySheetBox.h = (fontHeight * 4 + 12);
			}
		}
		else
		{
			partySheetBox.h = (fontHeight + 12);
		}
	}
}

void Player::CharacterSheet_t::setDefaultSkillsSheetBox()
{
	int y1 = players[player.playernum]->camera_y1();
	int x2 = players[player.playernum]->camera_x2();

	skillsSheetBox.w = 208;
	skillsSheetBox.y = y1 + 32;

	int fontHeight = TTF12_HEIGHT;
	if ( uiscale_skillspage )
	{
		fontHeight = TTF16_HEIGHT;
		skillsSheetBox.w = 276;
	}
	skillsSheetBox.x = x2 - skillsSheetBox.w;
	skillsSheetBox.h = (NUMPROFICIENCIES * fontHeight) + (fontHeight * 3);
}

void drawSkillsSheet(const int player)
{
}
//{
//	const int x2 = players[player]->camera_x2();
//
//	const Sint32 mousex = inputs.getMouse(player, Inputs::X);
//	const Sint32 mousey = inputs.getMouse(player, Inputs::Y);
//	const Sint32 omousex = inputs.getMouse(player, Inputs::OX);
//	const Sint32 omousey = inputs.getMouse(player, Inputs::OY);
//
//	SDL_Rect pos;
//	TTF_Font* fontSkill = ttf12;
//	int fontHeight = TTF12_HEIGHT;
//	int fontWidth = TTF12_WIDTH;
//	if ( uiscale_skillspage )
//	{
//		fontSkill = ttf16;
//		fontHeight = TTF16_HEIGHT;
//		fontWidth = TTF16_WIDTH;
//	}
//	players[player]->characterSheet.setDefaultSkillsSheetBox();
//
//	pos.x = players[player]->characterSheet.skillsSheetBox.x - 208;
//	pos.y = players[player]->characterSheet.skillsSheetBox.y;
//	pos.w = players[player]->characterSheet.skillsSheetBox.w;
//	pos.h = players[player]->characterSheet.skillsSheetBox.h;
//
//	drawWindowFancy(pos.x, pos.y, pos.x + pos.w, pos.y + pos.h);
//
//	ttfPrintTextFormatted(fontSkill, pos.x + 4, pos.y + 8, Language::get(1883));
//
//	SDL_Rect button;
//	button.x = x2 - attributesright_bmp->w - 8;
//	button.w = attributesright_bmp->w;
//	button.y = pos.y;
//	button.h = attributesright_bmp->h;
//	if ( uiscale_skillspage )
//	{
//		button.w = attributesright_bmp->w * 1.3;
//		button.x = x2 - button.w - 8;
//		button.y = pos.y;
//		button.h = attributesright_bmp->h * 1.3;
//	}
//
//	if ( (inputs.bMouseLeft(player) || inputs.bControllerInputPressed(player, INJOY_GAME_USE)) && !players[player]->shootmode )
//	{
//		if ( omousex >= button.x && omousex <= button.x + button.w
//			&& omousey >= button.y && omousey <= button.y + button.h )
//		{
//			buttonclick = 14;
//			playSound(139, 64);
//			if ( players[player]->characterSheet.proficienciesPage == 0 )
//			{
//				players[player]->characterSheet.proficienciesPage = 1;
//			}
//			else
//			{
//				players[player]->characterSheet.proficienciesPage = 0;
//			}
//			inputs.mouseClearLeft(player);
//			inputs.controllerClearInput(player, INJOY_GAME_USE);
//		}
//	}
//	if ( buttonclick == 14 )
//	{
//		drawImageScaled(attributesright_bmp, nullptr, &button);
//	}
//	else
//	{
//		drawImageScaled(attributesrightunclicked_bmp, nullptr, &button);
//	}
//
//	SDL_Rect lockbtn = button;
//	lockbtn.h = 24;
//	lockbtn.w = 24;
//	lockbtn.y += 2;
//	if ( uiscale_skillspage )
//	{
//		lockbtn.h = 24 * 1.3;
//		lockbtn.w = 24 * 1.3;
//		lockbtn.x -= 32 * 1.3;
//	}
//	else
//	{
//		lockbtn.x -= 32;
//	}
//	if ( players[player]->characterSheet.lock_right_sidebar )
//	{
//		drawImageScaled(sidebar_lock_bmp, nullptr, &lockbtn);
//	}
//	else
//	{
//		drawImageScaled(sidebar_unlock_bmp, nullptr, &lockbtn);
//	}
//
//	if ( (inputs.bMouseLeft(player) || inputs.bControllerInputPressed(player, INJOY_GAME_USE)) && !players[player]->shootmode )
//	{
//		if ( omousex >= lockbtn.x && omousex <= lockbtn.x + lockbtn.w
//			&& omousey >= lockbtn.y && omousey <= lockbtn.y + lockbtn.h )
//		{
//			playSound(139, 64);
//			players[player]->characterSheet.lock_right_sidebar = !players[player]->characterSheet.lock_right_sidebar;
//			inputs.mouseClearLeft(player);
//			inputs.controllerClearInput(player, INJOY_GAME_USE);
//		}
//	}
//
//	pos.y += fontHeight * 2 + 8;
//
//	SDL_Rect initialSkillPos = pos;
//	//Draw skill names.
//	for ( int c = 0; c < (NUMPROFICIENCIES); ++c, pos.y += (fontHeight /** 2*/) )
//	{
//		ttfPrintTextFormatted(fontSkill, pos.x + 4, pos.y, "%s:", getSkillLangEntry(c));
//	}
//
//	//Draw skill levels.
//	pos = initialSkillPos;
//	Uint32 color = 0;
//	for ( int i = 0; i < (NUMPROFICIENCIES); ++i, pos.y += (fontHeight /** 2*/) )
//	{
//		if ( skillCapstoneUnlocked(player, i) )
//		{
//			color = uint32ColorGreen;
//		}
//		else
//		{
//			color = uint32ColorWhite;
//		}
//
//
//		if ( show_skill_values )
//		{
//			ttfPrintTextFormattedColor(fontSkill, pos.x + 4, pos.y, color, "%15d / 100", stats[player]->PROFICIENCIES[i]);
//		}
//		else if ( stats[player]->PROFICIENCIES[i] == 0 )
//		{
//			ttfPrintTextFormattedColor(fontSkill, pos.x + 4, pos.y, color, Language::get(363));
//		}
//		else if ( stats[player]->PROFICIENCIES[i] < SKILL_LEVEL_BASIC )
//		{
//			ttfPrintTextFormattedColor(fontSkill, pos.x + 4, pos.y, color, Language::get(364));
//		}
//		else if ( stats[player]->PROFICIENCIES[i] >= SKILL_LEVEL_BASIC && stats[player]->PROFICIENCIES[i] < SKILL_LEVEL_SKILLED )
//		{
//			ttfPrintTextFormattedColor(fontSkill, pos.x + 4, pos.y, color, Language::get(365));
//		}
//		else if ( stats[player]->PROFICIENCIES[i] >= SKILL_LEVEL_SKILLED && stats[player]->PROFICIENCIES[i] < SKILL_LEVEL_EXPERT )
//		{
//			ttfPrintTextFormattedColor(fontSkill, pos.x + 4, pos.y, color, Language::get(366));
//		}
//		else if ( stats[player]->PROFICIENCIES[i] >= SKILL_LEVEL_EXPERT && stats[player]->PROFICIENCIES[i] < SKILL_LEVEL_MASTER )
//		{
//			ttfPrintTextFormattedColor(fontSkill, pos.x + 4, pos.y, color, Language::get(367));
//		}
//		else if ( stats[player]->PROFICIENCIES[i] >= SKILL_LEVEL_MASTER && stats[player]->PROFICIENCIES[i] < SKILL_LEVEL_LEGENDARY )
//		{
//			ttfPrintTextFormattedColor(fontSkill, pos.x + 4, pos.y, color, Language::get(368));
//		}
//		else if ( stats[player]->PROFICIENCIES[i] >= SKILL_LEVEL_LEGENDARY )
//		{
//			ttfPrintTextFormattedColor(fontSkill, pos.x + 4, pos.y, color, Language::get(369));
//		}
//	}
//	pos = initialSkillPos;
//	SDL_Rect skillTooltipRect;
//	std::string skillTooltip;
//	for ( int i = 0; !players[player]->shootmode && i < (NUMPROFICIENCIES); ++i, pos.y += (fontHeight /** 2*/) )
//	{
//		if ( mouseInBounds(player, pos.x, pos.x + pos.w, pos.y, pos.y + fontHeight) && stats[player] )
//		{
//			skillTooltipRect.w = (longestline(Language::get(3255 + i)) * fontWidth) + 8;
//			skillTooltip = Language::get(3255 + i);
//
//			size_t n = std::count(skillTooltip.begin(), skillTooltip.end(), '\n'); // count newlines
//			skillTooltipRect.h = fontHeight * ((int)n + 2) + 8;
//			skillTooltipRect.x = mousex - 16 - skillTooltipRect.w;
//			skillTooltipRect.y = mousey + 16;
//
//			Uint32 capstoneTextColor = uint32ColorGray;
//			if ( skillCapstoneUnlocked(player, i) )
//			{
//				capstoneTextColor = uint32ColorGreen;
//			}
//
//			switch ( i )
//			{
//				case PRO_LOCKPICKING:
//					skillTooltipRect.h += 4 * fontHeight;
//					drawTooltip(&skillTooltipRect);
//					ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 16,
//						capstoneTextColor, Language::get(3270), CAPSTONE_LOCKPICKING_CHEST_GOLD_AMOUNT);
//					break;
//				case PRO_STEALTH:
//					skillTooltipRect.h += 4 * fontHeight + 4;
//					drawTooltip(&skillTooltipRect);
//					ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 16,
//						capstoneTextColor, Language::get(3271));
//					break;
//				case PRO_TRADING:
//					skillTooltipRect.h += 2 * fontHeight;
//					drawTooltip(&skillTooltipRect);
//					ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 16,
//						capstoneTextColor, Language::get(3272));
//					break;
//				case PRO_APPRAISAL:
//					skillTooltipRect.h += 2 * fontHeight;
//					drawTooltip(&skillTooltipRect);
//					ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 16,
//						capstoneTextColor, Language::get(3273));
//					break;
//				case PRO_SWIMMING:
//					skillTooltipRect.h += fontHeight;
//					drawTooltip(&skillTooltipRect);
//					ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 16,
//						capstoneTextColor, Language::get(3274));
//					break;
//				case PRO_LEADERSHIP:
//					skillTooltipRect.h += 2 * fontHeight + 4;
//					drawTooltip(&skillTooltipRect);
//					ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 16,
//						capstoneTextColor, Language::get(3275));
//					break;
//				case PRO_SPELLCASTING:
//					skillTooltipRect.h += 2 * fontHeight;
//					drawTooltip(&skillTooltipRect);
//					ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 16,
//						capstoneTextColor, Language::get(3276));
//					break;
//				case PRO_MAGIC:
//					break;
//				case PRO_RANGED:
//					skillTooltipRect.h += 2 * fontHeight + 4;
//					drawTooltip(&skillTooltipRect);
//					ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 16,
//						capstoneTextColor, Language::get(3284));
//					break;
//				case PRO_SWORD:
//				{
//					skillTooltipRect.h += 5 * fontHeight + 4;
//					drawTooltip(&skillTooltipRect);
//					ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 16,
//						capstoneTextColor, Language::get(3278), Language::get(3283));
//					break;
//				}
//				case PRO_MACE:
//				{
//					skillTooltipRect.h += 3 * fontHeight + 4;
//					drawTooltip(&skillTooltipRect);
//					ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 16,
//						capstoneTextColor, Language::get(3279));
//					break;
//				}
//				case PRO_AXE:
//				{
//					skillTooltipRect.h += 3 * fontHeight + 4;
//					drawTooltip(&skillTooltipRect);
//					ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 16,
//						capstoneTextColor, Language::get(3280));
//					break;
//				}
//				case PRO_POLEARM:
//					skillTooltipRect.h += 3 * fontHeight + 4;
//					drawTooltip(&skillTooltipRect);
//					ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 16,
//						capstoneTextColor, Language::get(3281));
//					break;
//				case PRO_UNARMED:
//					skillTooltipRect.h += 3 * fontHeight + 4;
//					drawTooltip(&skillTooltipRect);
//					ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 16,
//						capstoneTextColor, Language::get(3282));
//					break;
//				case PRO_SHIELD:
//					skillTooltipRect.h += 2 * fontHeight;
//					drawTooltip(&skillTooltipRect);
//					ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 16,
//						capstoneTextColor, Language::get(3283));
//					break;
//				case PRO_ALCHEMY:
//					skillTooltipRect.w = (longestline(Language::get(3348)) * fontWidth) + 8;
//					skillTooltipRect.x = mousex - 16 - skillTooltipRect.w;
//					break;
//				default:
//					drawTooltip(&skillTooltipRect);
//					break;
//			}
//
//			Uint32 headerColor = uint32ColorBaronyBlue;
//			if ( skillCapstoneUnlocked(player, i) )
//			{
//				headerColor = uint32ColorGreen;
//			}
//
//			if ( i != PRO_MAGIC && i != PRO_ALCHEMY )
//			{
//				ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 4, skillTooltipRect.y + 8, 
//					headerColor, "%s: (%d / 100)", getSkillLangEntry(i), stats[player]->PROFICIENCIES[i]);
//			}
//
//			real_t skillDetails[6] = { 0.f };
//
//			switch ( i )
//			{
//				case PRO_LOCKPICKING:
//				{
//					Sint32 PER = 0;
//					if ( players[player] && players[player]->entity )
//					{
//						PER = statGetPER(stats[player], players[player]->entity);
//					}
//					skillDetails[0] = stats[player]->PROFICIENCIES[i] / 2.f; // lockpick chests/doors
//					if ( stats[player]->PROFICIENCIES[i] == SKILL_LEVEL_LEGENDARY )
//					{
//						skillDetails[0] = 100.f;
//					}
//					skillDetails[1] = std::min(100.f, stats[player]->PROFICIENCIES[i] + 50.f);
//					if ( stats[player]->PROFICIENCIES[i] >= SKILL_LEVEL_EXPERT )
//					{
//						skillDetails[2] = 100.f; // lockpick automatons
//					}
//					else
//					{
//						skillDetails[2] = (100 - 100 / (static_cast<int>(stats[player]->PROFICIENCIES[i] / 20 + 1))); // lockpick automatons
//					}
//					skillDetails[3] = (100 - 100 / (std::max(1, static_cast<int>(stats[player]->PROFICIENCIES[i] / 10)))); // disarm arrow traps
//					if ( stats[player]->PROFICIENCIES[i] < SKILL_LEVEL_BASIC )
//					{
//						skillDetails[3] = 0.f;
//					}
//					std::string canRepairItems = "  no";
//					if ( (stats[player]->PROFICIENCIES[i] + PER + (stats[player]->type == AUTOMATON ? 20 : 0)) >= SKILL_LEVEL_LEGENDARY )
//					{
//						canRepairItems = "all";
//					}
//					else if ( (stats[player]->PROFICIENCIES[i] + PER + (stats[player]->type == AUTOMATON ? 20 : 0)) >= SKILL_LEVEL_MASTER )
//					{
//						canRepairItems = "2/0";
//					}
//					else if ( (stats[player]->PROFICIENCIES[i] + PER + (stats[player]->type == AUTOMATON ? 20 : 0)) >= SKILL_LEVEL_EXPERT )
//					{
//						canRepairItems = "1/0";
//					}
//					skillDetails[4] = maximumTinkeringBotsCanBeDeployed(stats[player]);
//
//					// bonus scrapping chances.
//					switch ( std::min(5, static_cast<int>((stats[player]->PROFICIENCIES[i] + PER) / 20)) )
//					{
//						case 5:
//							skillDetails[5] = 150.f;
//							break;
//						case 4:
//							skillDetails[5] = 125.f;
//							break;
//						case 3:
//							skillDetails[5] = 50.f;
//							break;
//						case 2:
//							skillDetails[5] = 25.f;
//							break;
//						case 1:
//							skillDetails[5] = 12.5;
//							break;
//						default:
//							skillDetails[5] = 0.f;
//							break;
//					}
//					ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 12,
//						uint32ColorWhite, Language::get(3255 + i),
//						skillDetails[0], skillDetails[1], skillDetails[2], skillDetails[3], skillDetails[5], canRepairItems.c_str(), skillDetails[4], getInputName(impulses[IN_FOLLOWERMENU]));
//					break;
//				}
//				case PRO_STEALTH:
//					if ( players[player] && players[player]->entity )
//					{
//						skillDetails[0] = players[player]->entity->entityLightAfterReductions(*stats[player], nullptr); 
//						skillDetails[0] = std::max(1, (static_cast<int>(skillDetails[0] / 32))); // general visibility
//						skillDetails[1] = stats[player]->PROFICIENCIES[i] * 2 * 100 / 512.f; // % visibility reduction of above
//						skillDetails[2] = (2 + (stats[player]->PROFICIENCIES[PRO_STEALTH] / 40)); // night vision when sneaking
//						skillDetails[3] = (stats[player]->PROFICIENCIES[PRO_STEALTH] / 20 + 2) * 2; // backstab dmg
//						if ( skillCapstoneUnlocked(player, i) )
//						{
//							skillDetails[3] *= 2;
//						}
//					}
//					ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 12,
//						uint32ColorWhite, Language::get(3255 + i),
//						skillDetails[0], skillDetails[1], skillDetails[2], skillDetails[3]);
//					
//					break;
//				case PRO_TRADING:
//					skillDetails[0] = 1 / ((50 + stats[player]->PROFICIENCIES[PRO_TRADING]) / 150.f); // buy value
//					skillDetails[1] = (50 + stats[player]->PROFICIENCIES[PRO_TRADING]) / 150.f; // sell value
//					ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 12,
//						uint32ColorWhite, Language::get(3255 + i),
//						skillDetails[0], skillDetails[1]);
//					break;
//				case PRO_APPRAISAL:
//					skillDetails[0] = (60.f / (stats[player]->PROFICIENCIES[PRO_APPRAISAL] + 1)) / (TICKS_PER_SECOND); // appraisal time per gold value
//					if ( players[player] && players[player]->entity )
//					{
//						skillDetails[1] = 10 * (stats[player]->PROFICIENCIES[PRO_APPRAISAL] + players[player]->entity->getPER() * 5); // max gold value can appraise
//						if ( skillDetails[1] < 0.1 )
//						{
//							skillDetails[1] = 9;
//						}
//						if ( (stats[player]->PROFICIENCIES[PRO_APPRAISAL] + players[player]->entity->getPER() * 5) >= 100 )
//						{
//							ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 12,
//								uint32ColorWhite, Language::get(3255 + i),
//								skillDetails[0], skillDetails[1], "yes");
//						}
//						else
//						{
//							ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 12,
//								uint32ColorWhite, Language::get(3255 + i),
//								skillDetails[0], skillDetails[1], "no");
//						}
//					}
//					else
//					{
//						ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 12,
//							uint32ColorWhite, Language::get(3255 + i),
//							skillDetails[0], skillDetails[1], "no");
//					}
//					break;
//				case PRO_SWIMMING:
//					skillDetails[0] = (((stats[player]->PROFICIENCIES[PRO_SWIMMING] / 100.f) * 50.f) + 50); // water movement speed
//					ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 12,
//						uint32ColorWhite, Language::get(3255 + i),
//						skillDetails[0]);
//					break;
//				case PRO_LEADERSHIP:
//					skillDetails[0] = std::min(8, std::max(4, 2 * (stats[player]->PROFICIENCIES[PRO_LEADERSHIP] / 20))); // max followers
//					if ( players[player] && players[player]->entity )
//					{
//						skillDetails[1] = 1 + (stats[player]->PROFICIENCIES[PRO_LEADERSHIP] / 20);
//						skillDetails[2] = 80 + ((players[player]->entity->getCHR() + stats[player]->PROFICIENCIES[PRO_LEADERSHIP]) / 20) * 10;
//					}
//					ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 12,
//						uint32ColorWhite, Language::get(3255 + i),
//						getInputName(impulses[IN_USE]),skillDetails[0], skillDetails[1], skillDetails[2], getInputName(impulses[IN_FOLLOWERMENU]));
//					break;
//				case PRO_SPELLCASTING:
//					if ( players[player] && players[player]->entity )
//					{
//						skillDetails[0] = Entity::getManaRegenInterval(players[player]->entity, *(stats[player]), true) / (TICKS_PER_SECOND * 1.f);
//						if ( isSpellcasterBeginner(player, players[player]->entity) )
//						{
//							ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 12,
//								uint32ColorWhite, Language::get(3255 + i),
//								skillDetails[0], "yes");
//						}
//						else
//						{
//							ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 12,
//								uint32ColorWhite, Language::get(3255 + i),
//								skillDetails[0], "no");
//						}
//					}
//					else
//					{
//						ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 12,
//							uint32ColorWhite, Language::get(3255 + i),
//							skillDetails[0], "");
//					}
//					break;
//				case PRO_MAGIC:
//				{
//					int skillLVL = 0;
//					std::string magics = "";
//					int lines = 0;
//					if ( players[player] && players[player]->entity )
//					{
//						skillLVL = (stats[player]->PROFICIENCIES[PRO_MAGIC] + players[player]->entity->getINT());
//						if ( stats[player]->PROFICIENCIES[PRO_MAGIC] >= 100 )
//						{
//							skillLVL = 100;
//						}
//					}
//					if ( skillLVL < 0 )
//					{
//						skillTooltip = "none";
//					}
//					else
//					{
//						skillLVL = skillLVL / 20;
//						skillTooltip = "tier ";
//						if ( skillLVL == 0 )
//						{
//							skillTooltip += "I";
//						}
//						else if ( skillLVL == 1 )
//						{
//							skillTooltip += "II";
//						}
//						else if ( skillLVL == 2 )
//						{
//							skillTooltip += "III";
//						}
//						else if ( skillLVL == 3 )
//						{
//							skillTooltip += "IV";
//						}
//						else if ( skillLVL == 4 )
//						{
//							skillTooltip += "V";
//						}
//						else if ( skillLVL >= 5 )
//						{
//							skillTooltip += "VI";
//						}
//						for ( auto it = allGameSpells.begin(); it != allGameSpells.end(); ++it )
//						{
//							auto spellEntry = *it;
//							if ( spellEntry && spellEntry->difficulty == (skillLVL * 20) )
//							{
//								magics += " -[";
//								magics += spellEntry->getSpellName();
//								magics += "]\n";
//								++lines;
//							}
//						}
//					}
//					skillTooltipRect.h += (1 + lines) * (fontHeight + lines / 6);
//					drawTooltip(&skillTooltipRect);
//					ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 16 + (lines * (fontHeight + lines / 6)),
//						capstoneTextColor, Language::get(3277));
//
//					ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 4, skillTooltipRect.y + 8,
//						headerColor, "%s: (%d / 100)", getSkillLangEntry(i), stats[player]->PROFICIENCIES[i]);
//
//					ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 12,
//						uint32ColorWhite, Language::get(3255 + i),
//						skillTooltip.c_str());
//					ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 16 + (fontHeight) * 3, // print magic list
//						uint32ColorBaronyBlue, "%s",
//						magics.c_str());
//					break;
//				}
//				case PRO_RANGED:
//				{
//					skillDetails[0] = 100 - (100 - stats[player]->PROFICIENCIES[PRO_RANGED]) / 2.f; // lowest damage roll
//					skillDetails[1] = 50 + static_cast<int>(stats[player]->PROFICIENCIES[i] / 20) * 10;
//					if ( stats[player]->type == GOBLIN )
//					{
//						skillDetails[1] += 20;
//						if ( stats[player]->PROFICIENCIES[PRO_RANGED] < SKILL_LEVEL_LEGENDARY )
//						{
//							skillDetails[1] = std::min(skillDetails[1], 90.0);
//						}
//					}
//					if ( players[player] && players[player]->entity )
//					{
//						skillDetails[2] = std::min(std::max(players[player]->entity->getPER() / 2, 0), 50);
//					}
//					int skillLVL = stats[player]->PROFICIENCIES[PRO_RANGED] / 20; // thrown dmg bonus
//					skillDetails[3] = 100 * thrownDamageSkillMultipliers[std::min(skillLVL, 5)];
//					if ( skillCapstoneUnlocked(player, i) )
//					{
//						ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 12,
//							uint32ColorWhite, Language::get(3255 + i),
//							skillDetails[0], 0.f, skillDetails[2], skillDetails[3]);
//					}
//					else
//					{
//						ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 12,
//							uint32ColorWhite, Language::get(3255 + i),
//							skillDetails[0], 100 / skillDetails[1], skillDetails[2], skillDetails[3]);
//					}
//					break;
//				}
//				case PRO_SWORD:
//				case PRO_AXE:
//				case PRO_MACE:
//				case PRO_POLEARM:
//					if ( i == PRO_POLEARM )
//					{
//						skillDetails[0] = 100 - (100 - stats[player]->PROFICIENCIES[i]) / 3.f; // lowest damage roll
//					}
//					else
//					{
//						skillDetails[0] = 100 - (100 - stats[player]->PROFICIENCIES[i]) / 2.f; // lowest damage roll
//					}
//					if ( skillCapstoneUnlocked(player, i) )
//					{
//						ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 12,
//							uint32ColorWhite, Language::get(3255 + i),
//							skillDetails[0], 0.f, 0.f);
//					}
//					else
//					{
//						skillDetails[1] = 50 + (stats[player]->type == GOBLIN ? 20 : 0); // chance to degrade on > 0 dmg
//						skillDetails[2] = 4 + (stats[player]->type == GOBLIN ? 4 : 0); // chance to degrade on 0 dmg
//						skillDetails[1] += (static_cast<int>(stats[player]->PROFICIENCIES[i] / 20)) * 10;
//						skillDetails[2] += static_cast<int>(stats[player]->PROFICIENCIES[i] / 20);
//						if ( svFlags & SV_FLAG_HARDCORE )
//						{
//							skillDetails[1] *= 2;
//							skillDetails[2] *= 2;
//						}
//						ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 12,
//							uint32ColorWhite, Language::get(3255 + i),
//							skillDetails[0], 100 / skillDetails[1], 100 / skillDetails[2]);
//					}
//					break;
//				case PRO_UNARMED:
//					skillDetails[0] = 100 - (100 - stats[player]->PROFICIENCIES[i]) / 2.f; // lowest damage roll
//					skillDetails[3] = static_cast<int>(stats[player]->PROFICIENCIES[i] / 20);
//					skillDetails[4] = static_cast<int>(stats[player]->PROFICIENCIES[i] / 20) * 20;
//					if ( skillCapstoneUnlocked(player, i) )
//					{
//						ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 12,
//							uint32ColorWhite, Language::get(3255 + i),
//							skillDetails[0], 0.f, 0.f, skillDetails[3], skillDetails[4]);
//					}
//					else
//					{
//						skillDetails[1] = 100 + (stats[player]->type == GOBLIN ? 20 : 0); // chance to degrade on > 0 dmg
//						skillDetails[2] = 8 + (stats[player]->type == GOBLIN ? 4 : 0); // chance to degrade on 0 dmg
//						skillDetails[1] += (static_cast<int>(stats[player]->PROFICIENCIES[i] / 20)) * 10;
//						skillDetails[2] += static_cast<int>(stats[player]->PROFICIENCIES[i] / 20);
//						if ( svFlags & SV_FLAG_HARDCORE )
//						{
//							skillDetails[1] *= 2;
//							skillDetails[2] *= 2;
//						}
//						ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 12,
//							uint32ColorWhite, Language::get(3255 + i),
//							skillDetails[0], 100 / skillDetails[1], 100 / skillDetails[2], skillDetails[3], skillDetails[4]);
//					}
//					break;
//				case PRO_SHIELD:
//					skillDetails[0] = 5 + static_cast<int>(stats[player]->PROFICIENCIES[i] / 5);
//					if ( skillCapstoneUnlocked(player, i) )
//					{
//						ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 12,
//							uint32ColorWhite, Language::get(3255 + i),
//							skillDetails[0], 0.f, 0.f);
//					}
//					else
//					{
//						skillDetails[1] = 25 + (stats[player]->type == GOBLIN ? 10 : 0); // degrade > 0 dmg taken
//						skillDetails[2] = 10 + (stats[player]->type == GOBLIN ? 10 : 0); // degrade on 0 dmg
//						skillDetails[1] += (static_cast<int>(stats[player]->PROFICIENCIES[i] / 10));
//						skillDetails[2] += (static_cast<int>(stats[player]->PROFICIENCIES[i] / 10));
//						if ( svFlags & SV_FLAG_HARDCORE )
//						{
//							skillDetails[2] = 40 + (stats[player]->type == GOBLIN ? 10 : 0);
//						}
//						ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 12,
//							uint32ColorWhite, Language::get(3255 + i),
//							skillDetails[0], 100 / skillDetails[1], 100 / skillDetails[2]);
//					}
//					break;
//				case PRO_ALCHEMY:
//				{
//					std::string baseIngredients;
//					std::string secondaryIngredients;
//					int lines = 0;
//					int lines2 = 0;
//					for ( auto it = clientLearnedAlchemyIngredients[player].begin();
//						it != clientLearnedAlchemyIngredients[player].end(); ++it )
//					{
//						auto alchemyEntry = *it;
//						if ( GenericGUI[player].isItemBaseIngredient(alchemyEntry) )
//						{
//							baseIngredients += " -[";
//							std::string itemName = items[alchemyEntry].getIdentifiedName();
//							itemName = itemName.substr(10);
//							baseIngredients += itemName;
//							baseIngredients += "]\n";
//							++lines;
//						}
//						if ( GenericGUI[player].isItemSecondaryIngredient(alchemyEntry) )
//						{
//							secondaryIngredients += " -[";
//							std::string itemName = items[alchemyEntry].getIdentifiedName();
//							itemName = itemName.substr(10);
//							secondaryIngredients += itemName;
//							secondaryIngredients += "]\n";
//							++lines2;
//						}
//					}
//					lines = std::max(lines, lines2);
//					int skillLVL = stats[player]->PROFICIENCIES[i] / 20;
//					skillDetails[0] = 100 * potionDamageSkillMultipliers[std::min(skillLVL, 5)];
//					skillDetails[1] = skillDetails[0];
//					skillDetails[2] = 50.f + static_cast<int>(stats[player]->PROFICIENCIES[i] / 20) * 10;
//					skillDetails[3] = std::min(80, (60 + static_cast<int>(stats[player]->PROFICIENCIES[i] / 20) * 10));
//					skillDetails[4] = 50.f + static_cast<int>(stats[player]->PROFICIENCIES[i] / 20) * 5;
//
//					skillTooltipRect.h = fontHeight * (5 + 2) + 8;
//					skillTooltipRect.h += 4 + (6 + lines) * (fontHeight + lines / 6);
//					drawTooltip(&skillTooltipRect);
//					// legendary text
//					ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 16 + (lines * (fontHeight + lines / 6)),
//						capstoneTextColor, Language::get(3347));
//					// header text
//					ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 4, skillTooltipRect.y + 8,
//						headerColor, "%s: (%d / 100)", getSkillLangEntry(i), stats[player]->PROFICIENCIES[i]);
//					// effect text
//					ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 12,
//						uint32ColorWhite, Language::get(3348), 
//						skillDetails[0], skillDetails[1], skillDetails[2], skillDetails[3], skillDetails[4]);
//					// base potions
//					ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8, skillTooltipRect.y + 20 + (fontHeight) * 9, // print potion list
//						uint32ColorBaronyBlue, "%s",
//						baseIngredients.c_str());
//					// secondary potions
//					ttfPrintTextFormattedColor(fontSkill, skillTooltipRect.x + 8 + 18 * fontWidth, skillTooltipRect.y + 20 + (fontHeight) * 9, // print potion list
//						uint32ColorBaronyBlue, "%s",
//						secondaryIngredients.c_str());
//					break;
//				}
//				default:
//					break;
//			}
//		}
//	}
//}

void drawPartySheet(const int player)
{
}
//{
//	const int x1 = players[player]->camera_x1();
//	const int x2 = players[player]->camera_x2();
//
//	const Sint32 omousex = inputs.getMouse(player, Inputs::OX);
//	const Sint32 omousey = inputs.getMouse(player, Inputs::OY);
//
//	TTF_Font* fontPlayer = ttf12;
//	int fontHeight = TTF12_HEIGHT;
//	int fontWidth = TTF12_WIDTH;
//	if ( uiscale_skillspage )
//	{
//		fontPlayer = ttf16;
//		fontHeight = TTF16_HEIGHT;
//		fontWidth = TTF16_WIDTH;
//	}
//	players[player]->characterSheet.setDefaultPartySheetBox();
//
//	int playerCnt = 0;
//	for ( playerCnt = MAXPLAYERS - 1; playerCnt > 0; --playerCnt )
//	{
//		if ( !client_disconnected[playerCnt] )
//		{
//			break;
//		}
//	}
//
//	int numFollowers = 0;
//	if ( stats[player] )
//	{
//		numFollowers = list_Size(&stats[player]->FOLLOWERS);
//	}
//
//	if ( playerCnt == 0 ) // 1 player.
//	{
//		if ( numFollowers == 0 )
//		{
//			if ( players[player]->shootmode )
//			{
//				return; // don't show menu if not in inventory, no point reminding the player they have no friends!
//			}
//		}
//	}
//
//	SDL_Rect pos;
//	pos.x = players[player]->characterSheet.partySheetBox.x;
//	pos.y = players[player]->characterSheet.partySheetBox.y;
//	pos.w = players[player]->characterSheet.partySheetBox.w;
//	pos.h = players[player]->characterSheet.partySheetBox.h;
//	drawWindowFancy(pos.x, pos.y, pos.x + pos.w, pos.y + pos.h);
//
//	ttfPrintTextFormatted(fontPlayer, pos.x + 4, pos.y + 8, "Party Stats");
//
//	SDL_Rect button;
//	button.x = x2 - attributesright_bmp->w - 8;
//	button.w = attributesright_bmp->w;
//	button.y = pos.y;
//	button.h = attributesright_bmp->h;
//	if ( uiscale_skillspage )
//	{
//		button.w = attributesright_bmp->w * 1.3;
//		button.x = x2 - button.w - 8;
//		button.y = pos.y;
//		button.h = attributesright_bmp->h * 1.3;
//	}
//
//
//	if ( (inputs.bMouseLeft(player) || inputs.bControllerInputPressed(player, INJOY_GAME_USE)) && !players[player]->shootmode )
//	{
//		if ( omousex >= button.x && omousex <= button.x + button.w
//			&& omousey >= button.y && omousey <= button.y + button.h )
//		{
//			buttonclick = 14;
//			playSound(139, 64);
//			if ( players[player]->characterSheet.proficienciesPage == 0 )
//			{
//				players[player]->characterSheet.proficienciesPage = 1;
//			}
//			else
//			{
//				players[player]->characterSheet.proficienciesPage = 0;
//			}
//			inputs.mouseClearLeft(player);
//			inputs.controllerClearInput(player, INJOY_GAME_USE);
//		}
//	}
//	if ( buttonclick == 14 )
//	{
//		drawImageScaled(attributesright_bmp, nullptr, &button);
//	}
//	else
//	{
//		drawImageScaled(attributesrightunclicked_bmp, nullptr, &button);
//	}
//
//	SDL_Rect lockbtn = button;
//	lockbtn.h = 24;
//	lockbtn.w = 24;
//	lockbtn.y += 2;
//	if ( uiscale_skillspage )
//	{
//		lockbtn.h = 24 * 1.3;
//		lockbtn.w = 24 * 1.3;
//		lockbtn.x -= 32 * 1.3;
//	}
//	else
//	{
//		lockbtn.x -= 32;
//	}
//	if ( players[player]->characterSheet.lock_right_sidebar )
//	{
//		drawImageScaled(sidebar_lock_bmp, nullptr, &lockbtn);
//	}
//	else
//	{
//		drawImageScaled(sidebar_unlock_bmp, nullptr, &lockbtn);
//	}
//
//	if ( (inputs.bMouseLeft(player) || inputs.bControllerInputPressed(player, INJOY_GAME_USE)) && !players[player]->shootmode )
//	{
//		if ( omousex >= lockbtn.x && omousex <= lockbtn.x + lockbtn.w
//			&& omousey >= lockbtn.y && omousey <= lockbtn.y + lockbtn.h )
//		{
//			playSound(139, 64);
//			players[player]->characterSheet.lock_right_sidebar = !players[player]->characterSheet.lock_right_sidebar;
//			inputs.mouseClearLeft(player);
//			inputs.controllerClearInput(player, INJOY_GAME_USE);
//		}
//	}
//
//	pos.y += fontHeight * 2 + 4;
//
//	SDL_Rect playerBar;
//	
//	if ( playerCnt == 0 && numFollowers == 0 ) // 1 player.
//	{
//		ttfPrintTextFormatted(fontPlayer, pos.x + 32, pos.y + 8, "No party members");
//	}
//
//	//Draw party stats
//	Uint32 color = uint32ColorWhite;
//	if ( playerCnt > 0 )
//	{
//		for ( int i = 0; i < MAXPLAYERS; ++i, pos.y += (fontHeight * 4) + 6 )
//		{
//			if ( !client_disconnected[i] && stats[i] )
//			{
//				
//				if ( strlen(stats[i]->name) > 16 )
//				{
//					char shortname[32] = "";
//					strncpy(shortname, stats[i]->name, 14);
//					strcat(shortname, "..");
//					ttfPrintTextFormattedColor(fontPlayer, pos.x + 12, pos.y, color, "[%d] %s", i, shortname);
//				}
//				else
//				{
//					ttfPrintTextFormattedColor(fontPlayer, pos.x + 12, pos.y, color, "[%d] %s", i, stats[i]->name);
//				}
//
//				ttfPrintTextFormattedColor(fontPlayer, pos.x + 12, pos.y + fontHeight, color, "%s", playerClassLangEntry(client_classes[i], i));
//				ttfPrintTextFormattedColor(fontPlayer, x2 - 8 * 12, pos.y + fontHeight, color, "LVL %2d", stats[i]->LVL);
//
//				playerBar.x = pos.x + 64;
//				playerBar.w = 10 * 11;
//				if ( uiscale_skillspage )
//				{
//					playerBar.x += 10;
//					playerBar.w += 48;
//				}
//				playerBar.y = pos.y + fontHeight * 2 + 1;
//				playerBar.h = fontHeight;
//				// draw tooltip with blue outline
//				drawTooltip(&playerBar);
//				// draw faint red bar underneath
//				playerBar.x += 1;
//				drawRect(&playerBar, makeColorRGB(48, 0, 0), 255);
//
//				// draw main red bar for current HP
//				playerBar.w = (playerBar.w) * (static_cast<double>(stats[i]->HP) / stats[i]->MAXHP);
//				drawRect(&playerBar, makeColorRGB(128, 0, 0), 255);
//
//				// draw HP values
//				ttfPrintTextFormattedColor(fontPlayer, pos.x + 32, pos.y + fontHeight * 2 + 4, color, "HP:  %3d / %3d", stats[i]->HP, stats[i]->MAXHP);
//
//				playerBar.x = pos.x + 64;
//				playerBar.w = 10 * 11;
//				if ( uiscale_skillspage )
//				{
//					playerBar.x += 10;
//					playerBar.w += 48;
//				}
//				playerBar.y = pos.y + fontHeight * 3 + 1;
//				// draw tooltip with blue outline
//				drawTooltip(&playerBar);
//				playerBar.x += 1;
//				// draw faint blue bar underneath
//				drawRect(&playerBar, makeColorRGB(0, 0, 48), 255);
//
//				// draw blue red bar for current MP
//				playerBar.w = (playerBar.w) * (static_cast<double>(stats[i]->MP) / stats[i]->MAXMP);
//				drawRect(&playerBar, makeColorRGB(0, 24, 128), 255);
//
//				// draw MP values
//				ttfPrintTextFormattedColor(fontPlayer, pos.x + 32 , pos.y + fontHeight * 3 + 4, color, "MP:  %3d / %3d", stats[i]->MP, stats[i]->MAXMP);
//			}
//		}
//	}
//
//
//
//	// draw follower stats
//	if ( numFollowers > 0 )
//	{
//		FollowerRadialMenu& followerMenu = FollowerMenu[player];
//		int monstersToDisplay = followerMenu.maxMonstersToDraw;
//		if ( playerCnt != 0 )
//		{
//			pos.y -= (fontHeight * 4) * 2;
//			pos.y += std::max(playerCnt - 1, 0) * (fontHeight * 4 + 8);
//			monstersToDisplay = followerMenu.numMonstersToDrawInParty();
//		}
//		int i = 0;
//		SDL_Rect monsterEntryWindow;
//		monsterEntryWindow.x = pos.x + 8;
//		monsterEntryWindow.w = pos.w - 8;
//		if ( numFollowers > (monstersToDisplay + 1) )
//		{
//			monsterEntryWindow.w -= 16;
//		}
//		SDL_Rect slider = monsterEntryWindow;
//		slider.y = pos.y;
//
//		for ( node_t* node = stats[player]->FOLLOWERS.first; node != nullptr; node = node->next, ++i )
//		{
//			Entity* follower = nullptr;
//			if ( (Uint32*)node->element )
//			{
//				follower = uidToEntity(*((Uint32*)node->element));
//			}
//			if ( follower )
//			{
//				Stat* followerStats = follower->getStats();
//				if ( followerStats )
//				{
//					monsterEntryWindow.y = pos.y;
//					monsterEntryWindow.h = fontHeight * 2 + 12;
//
//					bool hideDetail = false;
//					if ( numFollowers > monstersToDisplay )
//					{
//						if ( i < followerMenu.sidebarScrollIndex )
//						{
//							hideDetail = true;
//						}
//						else if ( i > followerMenu.sidebarScrollIndex + monstersToDisplay )
//						{
//							hideDetail = true;
//						}
//					}
//
//					if ( !hideDetail )
//					{
//						drawWindowFancy(monsterEntryWindow.x, monsterEntryWindow.y, 
//							monsterEntryWindow.x + monsterEntryWindow.w, monsterEntryWindow.y + monsterEntryWindow.h);
//
//						if ( !followerMenu.recentEntity )
//						{
//							followerMenu.recentEntity = follower;
//						}
//						if ( followerMenu.recentEntity == follower )
//						{
//							// draw highlight on current selected monster.
//							drawRect(&monsterEntryWindow, uint32ColorBaronyBlue, 32);
//							// ttfPrintText(ttf16, x2 - 20, monsterEntryWindow.y + monsterEntryWindow.h / 2 - fontHeight / 2, "<");
//						}
//
//						if ( stats[player] && stats[player]->HP > 0 && !players[player]->shootmode
//							&& (inputs.bMouseLeft(player) || Input::inputs[player].binaryToggle("Use")) )
//						{
//							bool inBounds = mouseInBounds(player, monsterEntryWindow.x, monsterEntryWindow.x + monsterEntryWindow.w,
//								monsterEntryWindow.y, monsterEntryWindow.y + monsterEntryWindow.h);
//							if ( inBounds )
//							{
//								if ( inputs.bMouseLeft(player) )
//								{
//									followerMenu.recentEntity = follower;
//									playSound(139, 64);
//									followerMenu.accessedMenuFromPartySheet = true;
//									followerMenu.partySheetMouseX = omousex;
//									followerMenu.partySheetMouseY = omousey;
//									inputs.mouseClearLeft(player);
//									if ( followerMenu.recentEntity )
//									{
//										createParticleFollowerCommand(followerMenu.recentEntity->x, followerMenu.recentEntity->y, 0, FOLLOWER_TARGET_PARTICLE,
//											0);
//									}
//								}
//								else if ( Input::inputs[player].binary("Use") )
//								{
//									followerMenu.followerToCommand = follower;
//									followerMenu.recentEntity = follower;
//									followerMenu.accessedMenuFromPartySheet = true;
//									followerMenu.partySheetMouseX = omousex;
//									followerMenu.partySheetMouseY = omousey;
//									followerMenu.initfollowerMenuGUICursor(true);
//									followerMenu.updateScrollPartySheet();
//									if ( followerMenu.recentEntity )
//									{
//										createParticleFollowerCommand(followerMenu.recentEntity->x, followerMenu.recentEntity->y, 0, FOLLOWER_TARGET_PARTICLE,
//											0);
//									}
//								}
//							}
//						}
//
//						pos.y += 6;
//						char name[16] = "";
//						if ( strcmp(followerStats->name, "") && strcmp(followerStats->name, "nothing") )
//						{
//							if ( strlen(followerStats->name) > 10 )
//							{
//								if ( followerStats->type == SKELETON )
//								{
//									if ( MonsterData_t::nameMatchesSpecialNPCName(*followerStats, "skeleton sentinel") )
//									{
//										strncpy(name, "sentinel", 8);
//									}
//									else if ( MonsterData_t::nameMatchesSpecialNPCName(*followerStats, "skeleton knight") )
//									{
//										strncpy(name, "knight", 6);
//									}
//									else
//									{
//										strncpy(name, followerStats->name, 8);
//										strcat(name, "..");
//									}
//								}
//								else
//								{
//									strncpy(name, followerStats->name, 8);
//									strcat(name, "..");
//								}
//								ttfPrintTextFormattedColor(fontPlayer, pos.x + 20, pos.y, color, "%s", name);
//							}
//							else
//							{
//								ttfPrintTextFormattedColor(fontPlayer, pos.x + 20, pos.y, color, "%s", followerStats->name);
//							}
//						}
//						else
//						{
//							if ( strlen(monstertypename[followerStats->type]) > 10 )
//							{
//								strncpy(name, monstertypename[followerStats->type], 8);
//								strcat(name, "..");
//								ttfPrintTextFormattedColor(fontPlayer, pos.x + 20, pos.y, color, "%s", name);
//							}
//							else
//							{
//								ttfPrintTextFormattedColor(fontPlayer, pos.x + 20, pos.y, color, "%s", monstertypename[followerStats->type]);
//							}
//						}
//						ttfPrintTextFormattedColor(fontPlayer, x2 - 8 * 11, pos.y, color, "LVL %2d", followerStats->LVL);
//
//						playerBar.x = pos.x + 64;
//						playerBar.w = 10 * 11;
//						if ( uiscale_skillspage )
//						{
//							playerBar.x += 10;
//							playerBar.w += 48;
//						}
//						playerBar.y = pos.y + fontHeight + 1;
//						playerBar.h = fontHeight;
//						// draw tooltip with blue outline
//						drawTooltip(&playerBar);
//						// draw faint red bar underneath
//						playerBar.x += 1;
//						drawRect(&playerBar, makeColorRGB(48, 0, 0), 255);
//
//						// draw main red bar for current HP
//						playerBar.w = (playerBar.w) * (static_cast<double>(followerStats->HP) / followerStats->MAXHP);
//						drawRect(&playerBar, makeColorRGB(128, 0, 0), 255);
//
//						// draw HP values
//						ttfPrintTextFormattedColor(fontPlayer, pos.x + 32, pos.y + fontHeight + 4, color, "HP:  %3d / %3d", followerStats->HP, followerStats->MAXHP);
//						pos.y += (fontHeight * 2 + 6);
//					}
//				}
//			}
//		}
//		slider.x = x1 + x2 - 16;
//		slider.w = 16;
//		slider.h = (fontHeight * 2 + 12) * (std::min(monstersToDisplay + 1, numFollowers));
//		players[player]->characterSheet.partySheetBox.h += slider.h + 6;
//
//		if ( numFollowers > (monstersToDisplay + 1) )
//		{
//			drawDepressed(slider.x, slider.y, slider.x + slider.w,
//				slider.y + slider.h);
//
//			bool mouseInScrollbarTotalHeight = mouseInBounds(player, x2 - monsterEntryWindow.w, x2, slider.y,
//				slider.y + slider.h);
//
//			if ( mousestatus[SDL_BUTTON_WHEELDOWN] && mouseInScrollbarTotalHeight )
//			{
//				mousestatus[SDL_BUTTON_WHEELDOWN] = 0;
//				followerMenu.sidebarScrollIndex = std::min(followerMenu.sidebarScrollIndex + 1, numFollowers - monstersToDisplay - 1);
//			}
//			else if ( mousestatus[SDL_BUTTON_WHEELUP] && mouseInScrollbarTotalHeight )
//			{
//				mousestatus[SDL_BUTTON_WHEELUP] = 0;
//				followerMenu.sidebarScrollIndex = std::max(followerMenu.sidebarScrollIndex - 1, 0);
//			}
//
//			slider.h *= (1 / static_cast<real_t>(std::max(1, numFollowers - monstersToDisplay)));
//			slider.y += slider.h * followerMenu.sidebarScrollIndex;
//			drawWindowFancy(slider.x, slider.y, slider.x + slider.w, slider.y + slider.h);
//			if ( mouseInScrollbarTotalHeight && (inputs.bMouseLeft(player) || inputs.bControllerInputPressed(player, INJOY_GAME_USE)) )
//			{
//				if ( !mouseInBounds(player, x2 - monsterEntryWindow.w, x2, slider.y,
//					slider.y + slider.h) )
//				{
//					if ( omousey < slider.y )
//					{
//						followerMenu.sidebarScrollIndex = std::max(followerMenu.sidebarScrollIndex - 1, 0);
//						inputs.mouseClearLeft(player);
//						inputs.controllerClearInput(player, INJOY_GAME_USE);
//					}
//					else if ( omousey > slider.y + slider.h )
//					{
//						followerMenu.sidebarScrollIndex = std::min(followerMenu.sidebarScrollIndex + 1, numFollowers - monstersToDisplay - 1);
//						inputs.mouseClearLeft(player);
//						inputs.controllerClearInput(player, INJOY_GAME_USE);
//					}
//				}
//			}
//		}
//	}
//}

void statsHoverText(const int player, Stat* tmpStat)
{
}
//{
//	if ( tmpStat == nullptr )
//	{
//		return;
//	}
//
//	const Sint32 mousex = inputs.getMouse(player, Inputs::X);
//	const Sint32 mousey = inputs.getMouse(player, Inputs::Y);
//
//	int pad_y = players[player]->characterSheet.statsSheetBox.y + 66; // was 262px
//	int pad_x = players[player]->characterSheet.statsSheetBox.x + 8; // 8 px. offset
//	int off_h = TTF12_HEIGHT - 4; // 12px. height of stat line.
//	int off_w = players[player]->characterSheet.statsSheetBox.w - 8;
//	int i = 0;
//	int j = 0;
//	SDL_Rect src;
//	SDL_Rect pos;
//
//	if ( uiscale_charactersheet )
//	{
//		pad_y += 86;
//		off_h = TTF16_HEIGHT - 4;
//		off_w = 280;
//	}
//
//	int tooltip_offset_x = 16; // 16px.
//	int tooltip_offset_y = 16; // 16px.
//	int tooltip_base_h = TTF12_HEIGHT;
//	int tooltip_pad_h = 8;
//	int tooltip_text_pad_x = 8;
//
//	char tooltipHeader[6][128] =
//	{
//		"strength: ",
//		"dexterity: ",
//		"constitution: ",
//		"intelligence: ",
//		"perception: ",
//		"charisma: "
//	};
//
//	char tooltipText[6][5][128] =
//	{
//		{
//			"base:  %2d ",
//			"bonus: %2d ",
//			""
//		},
//		{
//			"base:  %2d ",
//			"bonus: %2d ",
//			""
//		},
//		{
//			"base:  %2d ",
//			"bonus: %2d ",
//			"HP regen rate: 1 / %2.1fs",
//			""
//		},
//		{
//			"base:  %2d ",
//			"bonus: %2d ",
//			"MP regen rate: 1 / %2.1fs",
//			"magic damage:     %3d%%",
//			"magic resistance: %2.1f%% "
//		},
//		{
//			"base:  %2d ",
//			"bonus: %2d ",
//			""
//		},
//		{
//			"base:  %2d ",
//			"bonus: %2d ",
//			""
//		}
//	};
//
//	char buf[128] = "";
//	int numInfoLines = 0;
//	Sint32 statBase = 0;
//	Sint32 statBonus = 0;
//
//	SDL_Surface *tmp_bmp = NULL;
//
//	Entity* playerEntity = nullptr;
//	if ( players[player] )
//	{
//		playerEntity = players[player]->entity;
//	}
//
//	if ( players[player]->characterSheet.attributespage == 0 )
//	{
//		for ( i = 0; i < 6; i++ ) // cycle through 6 stats.
//		{
//			switch ( i )
//			{
//				// prepare the stat image.
//				case 0:
//					numInfoLines = 2;
//					tmp_bmp = str_bmp64;
//					statBase = tmpStat->STR;
//					statBonus = statGetSTR(tmpStat, playerEntity) - statBase;
//					break;
//				case 1:
//					numInfoLines = 2;
//					tmp_bmp = dex_bmp64;
//					statBase = tmpStat->DEX;
//					statBonus = statGetDEX(tmpStat, playerEntity) - statBase;
//					break;
//				case 2:
//					numInfoLines = 3;
//					tmp_bmp = con_bmp64;
//					statBase = tmpStat->CON;
//					statBonus = statGetCON(tmpStat, playerEntity) - statBase;
//					break;
//				case 3:
//					numInfoLines = 5;
//					tmp_bmp = int_bmp64;
//					statBase = tmpStat->INT;
//					statBonus = statGetINT(tmpStat, playerEntity) - statBase;
//					break;
//				case 4:
//					numInfoLines = 2;
//					tmp_bmp = per_bmp64;
//					statBase = tmpStat->PER;
//					statBonus = statGetPER(tmpStat, playerEntity) - statBase;
//					break;
//				case 5:
//					numInfoLines = 2;
//					tmp_bmp = chr_bmp64;
//					statBase = tmpStat->CHR;
//					statBonus = statGetCHR(tmpStat, playerEntity) - statBase;
//					break;
//				default:
//					numInfoLines = 0;
//					break;
//			}
//
//			if ( mouseInBounds(player, pad_x, pad_x + off_w, pad_y, pad_y + off_h) )
//			{
//				src.x = mousex + tooltip_offset_x;
//				src.y = mousey + tooltip_offset_y;
//				src.h = std::max(tooltip_base_h * (numInfoLines + 1) + tooltip_pad_h, tooltip_base_h * (2) + tooltip_pad_h);
//				src.w = 180;
//				for ( j = 0; j < numInfoLines; j++ )
//				{
//					src.w = std::max(longestline(tooltipText[i][j]) * 12, src.w);
//				}
//				drawTooltip(&src);
//
//				pos.x = src.x + 6;
//				pos.y = src.y + 4;
//				pos.h = 32;
//				pos.w = 32;
//
//				drawImageScaled(tmp_bmp, NULL, &pos);
//
//				src.x = pos.x + pos.w;
//				src.y += 2;
//
//				ttfPrintText(ttf12, src.x + 4, src.y + 4, tooltipHeader[i]);
//
//				for ( j = 0; j < numInfoLines && numInfoLines > 0; j++ )
//				{
//					int infoText_x = src.x + 4 + tooltip_text_pad_x;
//					int infoText_y = src.y + 4 + (tooltip_base_h * (j + 1));
//					Uint32 color = uint32ColorWhite;
//
//					if ( j == 0 )
//					{
//						snprintf(buf, longestline(tooltipText[i][j]), tooltipText[i][j], statBase);
//					}
//					else if ( j == 1 )
//					{
//						snprintf(buf, longestline(tooltipText[i][j]), tooltipText[i][j], statBonus);
//						if ( statBonus > 0 )
//						{
//							color = uint32ColorGreen;
//						}
//						else if ( statBonus < 0 )
//						{
//							color = uint32ColorRed;
//						}
//					}
//					else if ( j == 2 )
//					{
//						if ( i == 3 )
//						{
//							Entity* tmp = nullptr;
//							if ( players[player] && players[player]->entity )
//							{
//								tmp = players[player]->entity;
//								real_t regen = (static_cast<real_t>(Entity::getManaRegenInterval(tmp, *tmpStat, true)) / TICKS_PER_SECOND);
//								if ( stats[player]->type == AUTOMATON )
//								{
//									if ( stats[player]->HUNGER <= 300 )
//									{
//										regen /= 6; // degrade faster
//									}
//									else if ( stats[player]->HUNGER > 1200 )
//									{
//										if ( stats[player]->MP / static_cast<real_t>(std::max(1, stats[player]->MAXMP)) <= 0.5 )
//										{
//											regen /= 4; // increase faster at < 50% mana
//										}
//										else
//										{
//											regen /= 2; // increase less faster at > 50% mana
//										}
//									}
//									else if ( stats[player]->HUNGER > 300 )
//									{
//										// normal manaRegenInterval 300-1200 hunger.
//									}
//								}
//
//								if ( regen < 0.f /*stats[player]->playerRace == RACE_INSECTOID && stats[player]->appearance == 0*/ )
//								{
//									regen = 0.f;
//									snprintf(buf, longestline("MP regen rate: 0 / %2.1fs"), "MP regen rate: 0 / %2.1fs", (static_cast<real_t>(MAGIC_REGEN_TIME) / TICKS_PER_SECOND));
//								}
//								else
//								{
//									snprintf(buf, longestline(tooltipText[i][j]), tooltipText[i][j], regen);
//								}
//
//								if ( stats[player]->type == AUTOMATON )
//								{
//									if ( stats[player]->HUNGER <= 300 )
//									{
//										color = uint32ColorRed;
//									}
//									else if ( regen < static_cast<real_t>(getBaseManaRegen(tmp, *tmpStat)) / TICKS_PER_SECOND )
//									{
//										color = uint32ColorGreen;
//									}
//								}
//								else if ( stats[player]->playerRace == RACE_INSECTOID && stats[player]->appearance == 0 )
//								{
//									if ( !(svFlags & SV_FLAG_HUNGER) )
//									{
//										color = uint32ColorWhite;
//									}
//									else
//									{
//										color = uint32ColorRed;
//									}
//								}
//								else if ( regen < static_cast<real_t>(getBaseManaRegen(tmp, *tmpStat)) / TICKS_PER_SECOND)
//								{
//									color = uint32ColorGreen;
//								}
//								
//							}
//							else
//							{
//								snprintf(buf, longestline(tooltipText[i][j]), tooltipText[i][j], 0.f);
//							}
//						}
//						else if ( i == 2 )
//						{
//							Entity* tmp = nullptr;
//							if ( players[player] && players[player]->entity )
//							{
//								tmp = players[player]->entity;
//								real_t regen = (static_cast<real_t>(Entity::getHealthRegenInterval(tmp, *tmpStat, true)) / TICKS_PER_SECOND);
//								if ( tmpStat->type == SKELETON )
//								{
//									if ( !(svFlags & SV_FLAG_HUNGER) )
//									{
//										regen = HEAL_TIME * 4 / TICKS_PER_SECOND;
//									}
//								}
//								if ( regen < 0 )
//								{
//									regen = 0.f;
//									if ( !(svFlags & SV_FLAG_HUNGER) )
//									{
//										color = uint32ColorWhite;
//									}
//									else
//									{
//										color = uint32ColorRed;
//									}
//									snprintf(buf, longestline("HP regen rate: 0 / %2.1fs"), "HP regen rate: 0 / %2.1fs", (static_cast<real_t>(HEAL_TIME) / TICKS_PER_SECOND));
//								}
//								else if ( regen < HEAL_TIME / TICKS_PER_SECOND )
//								{
//									color = uint32ColorGreen;
//								}
//								if ( regen > 0.f )
//								{
//									snprintf(buf, longestline(tooltipText[i][j]), tooltipText[i][j], regen);
//								}
//							}
//							else
//							{
//								snprintf(buf, longestline(tooltipText[i][j]), tooltipText[i][j], 0.f);
//							}
//						}
//					}
//					else if ( j == 3 )
//					{
//						if ( i == 3 )
//						{
//							int bonusDamage = 100;
//							if ( players[player] && players[player]->entity )
//							{
//								bonusDamage += 100 * (getBonusFromCasterOfSpellElement(players[player]->entity, stats[player], nullptr));
//							}
//							snprintf(buf, longestline(tooltipText[i][j]), tooltipText[i][j], bonusDamage);
//						}
//					}
//					else if ( j == 4 )
//					{
//						if ( i == 3 )
//						{
//							if ( players[player] && players[player]->entity )
//							{
//								real_t resistance = 100 - 100 / (Entity::getMagicResistance(stats[player]) + 1);
//								snprintf(buf, longestline(tooltipText[i][j]), tooltipText[i][j], resistance);
//								if ( resistance > 0.f )
//								{
//									color = uint32ColorGreen;
//								}
//							}
//							else
//							{
//								snprintf(buf, longestline(tooltipText[i][j]), tooltipText[i][j], 0);
//							}
//						}
//					}
//					ttfPrintTextColor(ttf12, infoText_x, infoText_y, color, false, buf);
//				}
//			}
//			numInfoLines = 0;
//			pad_y += off_h;
//		}
//	}
//}

Sint32 displayAttackPower(const int player, AttackHoverText_t& output)
{
	Sint32 attack = 0;
	Entity* entity = nullptr;
	if ( players[player] )
	{
		entity = players[player]->entity;
		if ( stats[player] )
		{
			bool shapeshiftUseMeleeAttack = false;
			if ( entity && entity->effectShapeshift != NOTHING )
			{
				shapeshiftUseMeleeAttack = true;
				if ( entity && entity->effectShapeshift == CREATURE_IMP
					&& stats[player]->weapon && itemCategory(stats[player]->weapon) == MAGICSTAFF )
				{
					shapeshiftUseMeleeAttack = false;
				}
			}

			if ( !stats[player]->weapon || shapeshiftUseMeleeAttack )
			{
				// fists
				attack += Entity::getAttack(players[player]->entity, stats[player], true);
				output.hoverType = AttackHoverText_t::ATK_HOVER_TYPE_UNARMED; // melee
				output.totalAttack = attack;
				output.proficiencyBonus = (stats[player]->PROFICIENCIES[PRO_UNARMED] / 20); // bonus from proficiency
				output.mainAttributeBonus = statGetSTR(stats[player], entity); // bonus from main attribute
				output.equipmentAndEffectBonus = attack - statGetSTR(stats[player], entity) - BASE_PLAYER_UNARMED_DAMAGE - output.proficiencyBonus; // bonus from equipment

				// get damage variances.
				Sint32 variance = (attack / 2) * (100 - stats[player]->PROFICIENCIES[PRO_UNARMED]) / 100.f;
				output.attackMaxRange = attack;
				attack -= (variance / 2); // attack is the midpoint between max and min damage.
				output.attackMinRange = output.totalAttack - variance;
				output.proficiencyVariance = ((variance / 2) / static_cast<real_t>(attack)) * 100.f;// return percent variance
				output.totalAttack = attack;
				output.proficiency = PRO_UNARMED;
				output.totalAttack = output.attackMaxRange - ((output.attackMaxRange - output.attackMinRange) / 2.0);
			}
			else
			{
				int weaponskill = getWeaponSkill(stats[player]->weapon);
				if ( weaponskill == PRO_RANGED )
				{
					if ( isRangedWeapon(*stats[player]->weapon) )
					{
						if ( entity )
						{
							attack += entity->getRangedAttack();
						}
						output.hoverType = AttackHoverText_t::ATK_HOVER_TYPE_RANGED; // ranged
						output.totalAttack = attack;
						output.weaponBonus = stats[player]->weapon->weaponGetAttack(stats[player]); // bonus from weapon
						output.equipmentAndEffectBonus = 0;
						Sint32 quiverATK = 0;
						if ( stats[player]->shield && rangedWeaponUseQuiverOnAttack(stats[player]) )
						{
							quiverATK = stats[player]->shield->weaponGetAttack(stats[player]);
							attack += quiverATK;
						}
						output.mainAttributeBonus = statGetDEX(stats[player], entity); // bonus from main attribute
						output.equipmentAndEffectBonus += attack - output.mainAttributeBonus
							- BASE_RANGED_DAMAGE - output.weaponBonus; // bonus from equipment

						Sint32 variance = (attack / 2) * (100 - stats[player]->PROFICIENCIES[weaponskill]) / 100.f;
						output.attackMaxRange = attack;
						attack -= (variance / 2);
						output.attackMinRange = output.totalAttack - variance;
						output.proficiencyVariance = ((variance / 2) / static_cast<real_t>(attack)) * 100.f;// return percent variance
						output.totalAttack = attack;
						output.proficiency = weaponskill;
						output.totalAttack = output.attackMaxRange - ((output.attackMaxRange - output.attackMinRange) / 2.0);
					}
					else if ( stats[player]->weapon && stats[player]->weapon->type == TOOL_WHIP )
					{
						attack += Entity::getAttack(players[player]->entity, stats[player], true);
						output.hoverType = AttackHoverText_t::ATK_HOVER_TYPE_WHIP; // ranged
						output.totalAttack = attack;
						output.weaponBonus = stats[player]->weapon->weaponGetAttack(stats[player]); // bonus from weapon
						Sint32 STR = statGetSTR(stats[player], entity);
						Sint32 DEX = statGetDEX(stats[player], entity);
						Sint32 totalAttributeBonus = (STR + DEX);
						totalAttributeBonus = std::min(totalAttributeBonus / 2, totalAttributeBonus);
						//output.mainAttributeBonus = totalAttributeBonus - STRComponent; // bonus from main attribute (DEX)
						//output.secondaryAttributeBonus = totalAttributeBonus - DEXComponent; // secondary (STR)
						output.mainAttributeBonus = totalAttributeBonus;
						output.equipmentAndEffectBonus += attack - totalAttributeBonus
							- BASE_MELEE_DAMAGE - output.weaponBonus; // bonus from equipment

						Sint32 variance = (attack / 2) * (100 - stats[player]->PROFICIENCIES[weaponskill]) / 100.f;
						output.attackMaxRange = attack;
						attack -= (variance / 2);
						output.attackMinRange = output.totalAttack - variance;
						output.proficiencyVariance = ((variance / 2) / static_cast<real_t>(attack)) * 100.f;// return percent variance
						output.totalAttack = attack;
						output.proficiency = weaponskill;
						output.totalAttack = output.attackMaxRange - ((output.attackMaxRange - output.attackMinRange) / 2.0);
					}
					else
					{
						Sint32 skillLVL = stats[player]->PROFICIENCIES[PRO_RANGED];
						if ( entity )
						{
							attack += entity->getThrownAttack();
						}
						output.hoverType = AttackHoverText_t::ATK_HOVER_TYPE_THROWN; // thrown
						output.totalAttack = attack;
						// bonus from weapon
						output.weaponBonus = stats[player]->weapon->weaponGetAttack(stats[player]);
						// bonus from dex
						if ( itemCategory(stats[player]->weapon) != POTION )
						{
							Sint32 oldDEX = statGetDEX(stats[player], entity);
							Sint32 DEXComponent = oldDEX / 4;
							if ( itemCategory(stats[player]->weapon) == THROWN )
							{
								output.mainAttributeBonus = DEXComponent; //(output.totalAttack - entity->getThrownAttack());

								stats[player]->PROFICIENCIES[PRO_RANGED] = 0;
								output.proficiencyBonus = output.totalAttack - (entity ? entity->getThrownAttack() : 0);
								stats[player]->PROFICIENCIES[PRO_RANGED] = skillLVL;
							}
							else
							{
								if ( itemCategory(stats[player]->weapon) == GEM )
								{
									output.hoverType = AttackHoverText_t::ATK_HOVER_TYPE_THROWN_GEM; // thrown
								}
								// gems etc.
								Sint32 tmpDEX = stats[player]->DEX;
								stats[player]->DEX -= oldDEX;
								if ( entity )
								{
									output.mainAttributeBonus = (output.totalAttack - (entity ? entity->getThrownAttack() : 0));
								}
								output.proficiencyBonus = skillLVL / 10;
								stats[player]->DEX = tmpDEX;
							}
							output.proficiency = weaponskill;
						}
						else
						{
							output.hoverType = AttackHoverText_t::ATK_HOVER_TYPE_THROWN_POTION;
							output.mainAttributeBonus = 0;
							// bonus from proficiency
							Sint32 oldAlchemy = stats[player]->PROFICIENCIES[PRO_ALCHEMY];
							stats[player]->PROFICIENCIES[PRO_ALCHEMY] = 0;
							output.proficiencyBonus = output.totalAttack - (entity ? entity->getThrownAttack() : 0);
							stats[player]->PROFICIENCIES[PRO_ALCHEMY] = oldAlchemy;
							output.proficiency = PRO_ALCHEMY;
						}
						output.attackMaxRange = output.totalAttack;
						if ( itemCategory(stats[player]->weapon) == THROWN || itemCategory(stats[player]->weapon) == GEM )
						{
							output.attackMaxRange += 3; // maximum charge damage
						}
						output.attackMinRange = output.totalAttack;
						if ( itemCategory(stats[player]->weapon) == POTION )
						{
							output.attackMinRange -= (output.attackMinRange / 4);
							output.totalAttack -= (output.attackMaxRange - output.attackMinRange) / 2;
						}
						else
						{
							output.totalAttack = output.attackMinRange;
						}
					}
				}
				else if ( (weaponskill >= PRO_SWORD && weaponskill <= PRO_POLEARM) )
				{
					// melee weapon
					attack += Entity::getAttack(players[player]->entity, stats[player], true);
					output.hoverType = AttackHoverText_t::ATK_HOVER_TYPE_MELEE_WEAPON; // melee
					output.totalAttack = attack;
					output.weaponBonus = stats[player]->weapon->weaponGetAttack(stats[player]); // bonus from weapon
					output.mainAttributeBonus = statGetSTR(stats[player], entity); // bonus from main attribute
					if ( weaponskill == PRO_AXE )
					{
						output.weaponBonus += 1; // bonus from equipment
						output.totalAttack += 1;
						attack += 1;
					}
					// get damage variances.
					Sint32 variance = 0;
					if ( weaponskill == PRO_POLEARM )
					{
						variance = (attack / 3) * (100 - stats[player]->PROFICIENCIES[weaponskill]) / 100.f;
						if ( stats[player]->weapon->type == ARTIFACT_SPEAR )
						{
							variance = 0;
						}
					}
					else
					{
						variance = (attack / 2) * (100 - stats[player]->PROFICIENCIES[weaponskill]) / 100.f;
					}
					output.attackMaxRange = attack;
					attack -= (variance / 2); // attack is the midpoint between max and min damage.
					output.attackMinRange = output.totalAttack - variance;
					output.proficiencyVariance = ((variance / 2) / static_cast<real_t>(attack)) * 100.f;// return percent variance
					output.totalAttack = attack;
					output.proficiency = weaponskill;
					output.totalAttack = output.attackMaxRange - ((output.attackMaxRange - output.attackMinRange) / 2.0);
				}
				else if ( itemCategory(stats[player]->weapon) == MAGICSTAFF ) // staffs.
				{
					attack = 0;
					output.hoverType = AttackHoverText_t::ATK_HOVER_TYPE_MAGICSTAFF; // staffs
					output.totalAttack = attack;
					output.attackMaxRange = output.totalAttack;
					output.attackMinRange = output.totalAttack;
					output.proficiency = PRO_SPELLCASTING;
				}
				else // tools etc.
				{
					attack += Entity::getAttack(players[player]->entity, stats[player], true);
					output.hoverType = AttackHoverText_t::ATK_HOVER_TYPE_TOOL; // tools
					if ( stats[player]->weapon->type == TOOL_PICKAXE )
					{
						output.hoverType = AttackHoverText_t::ATK_HOVER_TYPE_PICKAXE;
					}
					else if ( itemIsThrowableTinkerTool(stats[player]->weapon) || stats[player]->weapon->type == TOOL_BEARTRAP )
					{
						output.hoverType = AttackHoverText_t::ATK_HOVER_TYPE_TOOL_TRAP;
					}
					output.totalAttack = attack;
					output.weaponBonus = 0; // bonus from weapon
					output.mainAttributeBonus = statGetSTR(stats[player], entity); // bonus from main attribute
					output.proficiencyBonus = 0; // bonus from proficiency
					output.equipmentAndEffectBonus = attack - output.mainAttributeBonus - BASE_MELEE_DAMAGE; // bonus from equipment
					output.attackMaxRange = output.totalAttack;
					output.attackMinRange = output.totalAttack;
					output.proficiency = PRO_LOCKPICKING;
					output.totalAttack = output.attackMaxRange - ((output.attackMaxRange - output.attackMinRange) / 2.0);
				}
			}
		}
	}
	else
	{
		attack = 0;
	}
	//return attack;
	return output.totalAttack;
}

void attackHoverText(const int player, Sint32 input[6])
{
	return;
/*
	const Sint32 mousex = inputs.getMouse(player, Inputs::X);
	const Sint32 mousey = inputs.getMouse(player, Inputs::Y);
	const Sint32 omousex = inputs.getMouse(player, Inputs::OX);
	const Sint32 omousey = inputs.getMouse(player, Inputs::OY);

	int pad_y = players[player]->characterSheet.statsSheetBox.y + 66 + 84; // was 346px
	int pad_x = players[player]->characterSheet.statsSheetBox.x + 8; // 8 px. offset
	int off_h = TTF12_HEIGHT - 4; // 12px. height of stat line.
	int off_w = players[player]->characterSheet.statsSheetBox.w - 8;
	int i = 0;
	int j = 0;
	SDL_Rect src;
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

	if ( players[player]->characterSheet.attributespage == 0 )
	{
		if ( mouseInBounds(player, pad_x, pad_x + off_w, pad_y, pad_y + off_h) )
		{
			char tooltipHeader[32] = "";
			switch ( input[0] )
			{
				case 0: // fists
					snprintf(tooltipHeader, strlen(Language::get(2529)), Language::get(2529));
					numInfoLines = 4;
					break;
				case 1: // ranged
					snprintf(tooltipHeader, strlen(Language::get(2530)), Language::get(2530));
					numInfoLines = 4;
					break;
				case 2: // thrown
					snprintf(tooltipHeader, strlen(Language::get(2531)), Language::get(2531));
					numInfoLines = 3;
					break;
				case 3: // melee
					snprintf(tooltipHeader, strlen(Language::get(2532)), Language::get(2532));
					numInfoLines = 4;
					break;
				case 4: // tools
					snprintf(tooltipHeader, strlen(Language::get(2540)), Language::get(2540));
					numInfoLines = 2;
					break;
				case 5: // staffs
					snprintf(tooltipHeader, strlen(Language::get(2541)), Language::get(2541));
					numInfoLines = 0;
					break;
				case 6: // whip
					snprintf(tooltipHeader, strlen(Language::get(2530)), Language::get(2530));
					numInfoLines = 3;
					break;
				default:
					break;
			}

			// get tooltip draw location.
			src.x = mousex + tooltip_offset_x;
			src.y = mousey + tooltip_offset_y;
			src.h = std::max(tooltip_base_h * (numInfoLines + 1) + tooltip_pad_h, tooltip_base_h * (2) + tooltip_pad_h);
			src.w = 256;
			if ( input[0] == 6 ) // whip tooltip wider
			{
				src.w += 32;
			}
			drawTooltip(&src);

			// draw header
			Uint32 color = uint32ColorWhite;
			ttfPrintTextColor(ttf12, src.x + 4, src.y + 4, color, false, tooltipHeader);
			if ( input[1] >= 0 )
			{
				// attack >= 0
				color = makeColorRGB(0, 255, 255);
			}
			else
			{
				// attack < 0
				color = makeColorRGB(255, 0, 0);
			}
			snprintf(tooltipHeader, 32, Language::get(2533), input[1]);
			ttfPrintTextColor(ttf12, src.x + 4, src.y + 4, color, false, tooltipHeader);

			for ( j = 0; j < numInfoLines && numInfoLines > 0; j++ )
			{
				int infoText_x = src.x + 4 + tooltip_text_pad_x;
				int infoText_y = src.y + 4 + (tooltip_base_h * (j + 1));
				Uint32 color = uint32ColorWhite;

				if ( input[0] == 0 ) // fists
				{
					switch ( j )
					{
						case 0:
							snprintf(buf, longestline(Language::get(3209)), Language::get(3209), input[2]);
							break;
						case 1:
							snprintf(buf, longestline(Language::get(2534)), Language::get(2534), input[3]);
							break;
						case 2:
							snprintf(buf, longestline(Language::get(2539)), Language::get(2539), input[4]);
							break;
						case 3:
							snprintf(buf, longestline(Language::get(2536)), Language::get(2536), input[5]);
						default:
							break;
					}
				}
				else if ( input[0] == 1 ) // ranged
				{
					switch ( j )
					{
						case 0:
							snprintf(buf, longestline(Language::get(2538)), Language::get(2538), input[2]);
							break;
						case 1:
							snprintf(buf, longestline(Language::get(2535)), Language::get(2535), input[3]);
							break;
						case 2:
							snprintf(buf, longestline(Language::get(2539)), Language::get(2539), input[4]);
							break;
						case 3:
							snprintf(buf, longestline(Language::get(2536)), Language::get(2536), input[5]);
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
							snprintf(buf, longestline(Language::get(2538)), Language::get(2538), input[2]);
							break;
						case 1:
							snprintf(buf, longestline(Language::get(2535)), Language::get(2535), input[3]);
							break;
						case 2:
							snprintf(buf, longestline(Language::get(2537)), Language::get(2537), input[4]);
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
							snprintf(buf, longestline(Language::get(2538)), Language::get(2538), input[2]);
							break;
						case 1:
							snprintf(buf, longestline(Language::get(2534)), Language::get(2534), input[3]);
							break;
						case 2:
							snprintf(buf, longestline(Language::get(2539)), Language::get(2539), input[4]);
							break;
						case 3:
							snprintf(buf, longestline(Language::get(2536)), Language::get(2536), input[5]);
						default:
							break;
					}
				}
				else if ( input[0] == 4 ) // tools
				{
					switch ( j )
					{
						case 0:
							snprintf(buf, longestline(Language::get(2534)), Language::get(2534), input[3]);
							break;
						case 1:
							snprintf(buf, longestline(Language::get(2536)), Language::get(2536), input[5]);
							break;
						default:
							break;
					}
				}
				else if ( input[0] == 5 ) // staff
				{
					
				}
				else if ( input[0] == 6 ) // whip
				{
					switch ( j )
					{
						case 0:
							snprintf(buf, 127, Language::get(2538), input[2]);
							break;
						case 1:
							snprintf(buf, longestline(Language::get(3458)), Language::get(3458), input[3]);
							break;
						case 2:
							snprintf(buf, longestline(Language::get(2539)), Language::get(2539), input[4]);
							break;
						default:
							break;
					}
				}
				ttfPrintTextColor(ttf12, infoText_x, infoText_y, color, false, buf);
			}
		}
	}
*/
}

void printStatBonus(TTF_Font* outputFont, Sint32 stat, Sint32 statWithModifiers, int x, int y)
{
	Uint32 color = 0;
	char bonusText[4] = "";

	if ( statWithModifiers - stat == 0 )
	{
		color = uint32ColorWhite;
		snprintf(bonusText, 4, "%2d", statWithModifiers);
	}
	else if ( statWithModifiers - stat < 0 )
	{
		color = uint32ColorRed;
		snprintf(bonusText, 4, "%2d", statWithModifiers);
	}
	if ( statWithModifiers - stat > 0 )
	{
		color = uint32ColorGreen;
		snprintf(bonusText, 4, "%2d", statWithModifiers);
	}

	ttfPrintTextFormattedColor(outputFont, x, y, color, bonusText);
}
