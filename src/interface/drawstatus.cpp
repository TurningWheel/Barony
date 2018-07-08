/*-------------------------------------------------------------------------------

	BARONY
	File: drawstatus.cpp
	Desc: contains drawStatus()

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../main.hpp"
#include "../draw.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "../items.hpp"
#include "../magic/magic.hpp"
#include "../sound.hpp"
#include "../net.hpp"
#include "../menu.hpp"
#include "../player.hpp"
#include "interface.hpp"
#include "../colors.hpp"

char enemy_name[128];
Sint32 enemy_hp = 0, enemy_maxhp = 0;
Uint32 enemy_timer = 0;
Uint32 enemy_bar_color[MAXPLAYERS] = { 0 }; // color for each player's enemy bar to display. multiplayer clients only refer to their own [clientnum] entry.

/*-------------------------------------------------------------------------------

	handleDamageIndicators

	draws damage indicators, fades them, culls them, etc.

-------------------------------------------------------------------------------*/

void handleDamageIndicators()
{
	node_t* node, *nextnode;
	for ( node = damageIndicators.first; node != NULL; node = nextnode )
	{
		nextnode = node->next;
		damageIndicator_t* damageIndicator = (damageIndicator_t*)node->element;

		double tangent = atan2( damageIndicator->y / 16 - camera.y, damageIndicator->x / 16 - camera.x );
		double angle = tangent - camera.ang;
		angle += 3 * PI / 2;
		while ( angle >= PI )
		{
			angle -= PI * 2;
		}
		while ( angle < -PI )
		{
			angle += PI * 2;
		}
		SDL_Rect pos;
		pos.x = xres / 2;
		pos.y = yres / 2;
		pos.x += 200 * cos(angle);
		pos.y += 200 * sin(angle);
		pos.w = damage_bmp->w;
		pos.h = damage_bmp->h;
		if ( stats[clientnum]->HP > 0 )
		{
			drawImageRotatedAlpha( damage_bmp, NULL, &pos, angle, (Uint8)(damageIndicator->alpha * 255) );
		}

		damageIndicator->alpha = std::min(damageIndicator->ticks, 120) / 120.f;
		if ( damageIndicator->alpha <= 0 )
		{
			list_RemoveNode(node);
		}
	}
}

void handleDamageIndicatorTicks()
{
	node_t* node;
	for ( node = damageIndicators.first; node != NULL; node = node->next )
	{
		damageIndicator_t* damageIndicator = (damageIndicator_t*)node->element;
		damageIndicator->ticks--;
	}
}

/*-------------------------------------------------------------------------------

	newDamageIndicator

	creates a new damage indicator on the hud

-------------------------------------------------------------------------------*/

damageIndicator_t* newDamageIndicator(double x, double y)
{
	damageIndicator_t* damageIndicator;

	// allocate memory for the indicator
	if ( (damageIndicator = (damageIndicator_t*) malloc(sizeof(damageIndicator_t))) == NULL )
	{
		printlog( "failed to allocate memory for new damage indicator!\n" );
		exit(1);
	}

	// add the indicator to the list of indicators
	damageIndicator->node = list_AddNodeLast(&damageIndicators);
	damageIndicator->node->element = damageIndicator;
	damageIndicator->node->deconstructor = &defaultDeconstructor;
	damageIndicator->node->size = sizeof(damageIndicator_t);

	damageIndicator->x = x;
	damageIndicator->y = y;
	damageIndicator->alpha = 1.f;
	damageIndicator->ticks = 120; // two seconds

	return damageIndicator;
}

/*-------------------------------------------------------------------------------

	updateEnemyBarColor

	updates the enemy hp bar color depending on an entities status effects

-------------------------------------------------------------------------------*/

void updateEnemyBarStatusEffectColor(int player, const Entity &target, const Stat &targetStats)
{
	if ( targetStats.EFFECTS[EFF_POISONED] )
	{
		if ( colorblind )
		{
			enemy_bar_color[player] = SDL_MapRGB(mainsurface->format, 0, 0, 64); // Display blue
		}
		else
		{
			enemy_bar_color[player] = SDL_MapRGB(mainsurface->format, 0, 64, 0); // Display green
		}
	}
	else if ( targetStats.EFFECTS[EFF_PARALYZED] )
	{
		enemy_bar_color[player] = SDL_MapRGB(mainsurface->format, 112, 112, 0);
	}
	else if ( targetStats.EFFECTS[EFF_CONFUSED] )
	{
		enemy_bar_color[player] = SDL_MapRGB(mainsurface->format, 92, 0, 92);
	}
	else
	{
		enemy_bar_color[player] = 0;
	}
}

/*-------------------------------------------------------------------------------

	updateEnemyBar

	updates the enemy hp bar for the given player

-------------------------------------------------------------------------------*/

void updateEnemyBar(Entity* source, Entity* target, char* name, Sint32 hp, Sint32 maxhp)
{
	int player = -1;
	int c;

	if (!source || !target)
	{
		return;
	}

	for (c = 0; c < MAXPLAYERS; c++)
	{
		if (source == players[c]->entity)
		{
			player = c;
			break;
		}
	}

	int playertarget = -1;
	for (c = 0; c < MAXPLAYERS; c++)
	{
		if (target == players[c]->entity)
		{
			playertarget = c;
			break;
		}
	}

	Stat* stats = target->getStats();
	if ( stats )
	{
		if ( stats->HP != stats->OLDHP )
		{
			if ( playertarget == clientnum )
			{
				newDamageIndicator(source->x, source->y);
			}
			else if ( playertarget > 0 && multiplayer == SERVER )
			{
				strcpy((char*)net_packet->data, "DAMI");
				SDLNet_Write32(source->x, &net_packet->data[4]);
				SDLNet_Write32(source->y, &net_packet->data[8]);
				net_packet->address.host = net_clients[playertarget - 1].host;
				net_packet->address.port = net_clients[playertarget - 1].port;
				net_packet->len = 12;
				sendPacketSafe(net_sock, -1, net_packet, playertarget - 1);
			}
			if ( player >= 0 )
			{
				updateEnemyBarStatusEffectColor(player, *target, *stats); // set color depending on status effects of the target.
			}
		}
	}

	if ( player == clientnum )
	{
		enemy_timer = ticks;
		enemy_hp = hp;
		enemy_maxhp = maxhp;
		strcpy( enemy_name, name );
	}
	else if ( player > 0 && multiplayer == SERVER )
	{
		strcpy((char*)net_packet->data, "ENHP");
		SDLNet_Write32(hp, &net_packet->data[4]);
		SDLNet_Write32(maxhp, &net_packet->data[8]);
		SDLNet_Write32(enemy_bar_color[player], &net_packet->data[12]);
		strcpy((char*)(&net_packet->data[16]), name);
		net_packet->data[16 + strlen(name)] = 0;
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 16 + strlen(name) + 1;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
	}
}

/*-------------------------------------------------------------------------------

	drawStatus

	Draws various status bar elements, such as textbox, health, magic,
	and the hotbar

-------------------------------------------------------------------------------*/

bool mouseInBoundsRealtimeCoords(int, int, int, int); //Defined in playerinventory.cpp. Dirty hack, you should be ashamed of yourself.

void warpMouseToSelectedHotbarSlot()
{
	SDL_Rect pos;
	pos.x = ((xres / 2) - 5 * hotbar_img->w * uiscale_hotbar) + (current_hotbar * hotbar_img->w * uiscale_hotbar) + (hotbar_img->w * uiscale_hotbar / 2);
	pos.y = STATUS_Y - (hotbar_img->h * uiscale_hotbar / 2);
	SDL_WarpMouseInWindow(screen, pos.x, pos.y);
}

void drawFollowerMenu()
{
	if ( followerMoveTo )
	{
		if ( !followerMenuEntity )
		{
			followerMoveTo = false;
		}
		return;
	}
	
	bool disableOption = false;
	bool keepWheelOpen = false;
	if ( followerMenuEntity )
	{
		int skillLVL = 0;
		if ( stats[clientnum] )
		{
			if ( followerMenuOptionSelected >= ALLY_CMD_DEFEND && followerMenuOptionSelected < ALLY_CMD_END )
			{
				skillLVL = stats[clientnum]->PROFICIENCIES[PRO_LEADERSHIP] + statGetCHR(stats[clientnum]);
				if ( skillLVL < AllyNPCSkillRequirements[followerMenuOptionSelected] )
				{
					disableOption = true;
				}
			}
		}
		// process commands if option selected on the wheel.
		if ( (!(*inputPressed(impulses[IN_USE])) && !(*inputPressed(joyimpulses[INJOY_GAME_USE])) && !followerMenuToggleClick)
			|| ((*inputPressed(impulses[IN_USE]) || *inputPressed(joyimpulses[INJOY_GAME_USE])) && followerMenuToggleClick) )
		{
			if ( followerMenuToggleClick )
			{
				*inputPressed(impulses[IN_USE]) = 0;
				*inputPressed(joyimpulses[INJOY_GAME_USE]) = 0;
				followerMenuToggleClick = false;
				if ( followerMenuOptionSelected == -1 )
				{
					followerMenuOptionSelected = ALLY_CMD_CANCEL;
				}
			}

			keepWheelOpen = followerMenuOptionSelected == ALLY_CMD_CLASS_TOGGLE || followerMenuOptionSelected == ALLY_CMD_PICKUP_TOGGLE;

			if ( followerMenuOptionSelected != -1 )
			{
				if ( followerMenuOptionSelected != ALLY_CMD_ATTACK_CONFIRM && followerMenuOptionSelected != ALLY_CMD_MOVETO_CONFIRM )
				{
					playSound(139, 16); // click
				}
				else
				{
					playSound(399, 16); // ping
				}
				// return to shootmode and close guis etc. TODO: tidy up interface code into 1 spot?
				if ( !keepWheelOpen )
				{
					shootmode = true;
					identifygui_active = false;
					selectedIdentifySlot = -1;
					closeRemoveCurseGUI();
					if ( openedChest[clientnum] )
					{
						openedChest[clientnum]->closeChest();
					}
					gui_mode = GUI_MODE_NONE;
				}

				if ( !disableOption 
					&& (followerMenuOptionSelected == ALLY_CMD_MOVETO_SELECT || followerMenuOptionSelected == ALLY_CMD_ATTACK_SELECT ) )
				{
					// return early, let the player use select command point.
					followerMoveTo = true;
					return;
				}
				else
				{
					if ( !disableOption )
					{
						if ( followerMenuOptionSelected == ALLY_CMD_DEFEND &&
							(followerMenuEntity->monsterAllyState == ALLY_STATE_DEFEND || followerMenuEntity->monsterAllyState == ALLY_STATE_MOVETO)  )
						{
							followerMenuOptionSelected = ALLY_CMD_FOLLOW;
						}
						if ( multiplayer == CLIENT )
						{
							if ( followerMenuOptionSelected == ALLY_CMD_ATTACK_CONFIRM )
							{
								sendAllyCommandClient(clientnum, followerMenuEntity->getUID(), followerMenuOptionSelected, 0, 0, followerMenuEntity->monsterAllyInteractTarget);
							}
							else if ( followerMenuOptionSelected == ALLY_CMD_MOVETO_CONFIRM )
							{
								sendAllyCommandClient(clientnum, followerMenuEntity->getUID(), followerMenuOptionSelected, followerMoveToX, followerMoveToY);
							}
							else
							{
								sendAllyCommandClient(clientnum, followerMenuEntity->getUID(), followerMenuOptionSelected, 0, 0);
							}
						}
						else
						{
							followerMenuEntity->monsterAllySendCommand(followerMenuOptionSelected, followerMoveToX, followerMoveToY, followerMenuEntity->monsterAllyInteractTarget);
						}
					}

					followerMenuOptionSelected = -1;

					if ( !keepWheelOpen )
					{
						followerMenuEntity = nullptr;
						followerMenuX = -1;
						followerMenuY = -1;
						followerMoveToX = -1;
						followerMoveToY = -1;
					}
				}
			}
			else
			{
				followerMenuToggleClick = true;
			}
		}
	}

	if ( followerMenuEntity )
	{
		SDL_Rect src;
		src.x = xres / 2;
		src.y = yres / 2;

		int numoptions = 8;
		real_t angleStart = PI / 2 - (PI / numoptions);
		real_t angleMiddle = angleStart + PI / numoptions;
		real_t angleEnd = angleMiddle + PI / numoptions;
		int radius = 140;
		int thickness = 70;
		src.h = radius;
		src.w = radius;
		int highlight = -1;
		int i = 0;
		
		int width = 0;
		TTF_SizeUTF8(ttf12, language[3036], &width, nullptr);
		ttfPrintText(ttf12, src.x - width / 2, src.y - radius - thickness - 24, language[3036]);
		
		drawImageRing(fancyWindow_bmp, nullptr, radius, thickness, 40, 0, PI * 2, 156);

		for ( i = 0; i < numoptions; ++i )
		{
			// draw borders around ring.
			drawLine(xres / 2 + (radius - thickness) * cos(angleStart), yres / 2 - (radius - thickness) * sin(angleStart),
				xres / 2 + (radius + thickness) * cos(angleStart), yres / 2 - (radius + thickness) * sin(angleStart), uint32ColorGray(*mainsurface), 192);
			drawLine(xres / 2 + (radius - thickness) * cos(angleEnd), yres / 2 - (radius - thickness) * sin(angleEnd),
				xres / 2 + (radius + thickness - 1) * cos(angleEnd), yres / 2 - (radius + thickness - 1) * sin(angleEnd), uint32ColorGray(*mainsurface), 192);

			drawArcInvertedY(xres / 2, yres / 2, radius - thickness, std::round((angleStart * 180) / PI), ((angleEnd * 180) / PI), uint32ColorGray(*mainsurface), 192);
			drawArcInvertedY(xres / 2, yres / 2, (radius + thickness), std::round((angleStart * 180) / PI), ((angleEnd * 180) / PI) + 1, uint32ColorGray(*mainsurface), 192);

			angleStart += 2 * PI / numoptions;
			angleMiddle = angleStart + PI / numoptions;
			angleEnd = angleMiddle + PI / numoptions;
		}

		angleStart = PI / 2 - (PI / numoptions);
		angleMiddle = angleStart + PI / numoptions;
		angleEnd = angleMiddle + PI / numoptions;

		int skillLVL = 0;
		if ( stats[clientnum] )
		{
			skillLVL = stats[clientnum]->PROFICIENCIES[PRO_LEADERSHIP] + statGetCHR(stats[clientnum]);
		}

		bool mouseInCenterButton = pow((omousex - followerMenuX), 2) + pow((omousey - followerMenuY), 2) < 4900; // 70 squared is 4900.

		for ( i = 0; i < numoptions; ++i )
		{
			// see if mouse cursor is within an option.
			if ( highlight == -1 )
			{
				if ( !mouseInCenterButton )
				{
					real_t x1 = followerMenuX + (radius + thickness + 45) * cos(angleEnd);
					real_t y1 = followerMenuY - (radius + thickness + 45) * sin(angleEnd);
					real_t x2 = followerMenuX + 5 * cos(angleMiddle);
					real_t y2 = followerMenuY - 5 * sin(angleMiddle);
					real_t x3 = followerMenuX + (radius + thickness + 45) * cos(angleStart);
					real_t y3 = followerMenuY - (radius + thickness + 45) * sin(angleStart);
					real_t a = ((y2 - y3)*(omousex - x3) + (x3 - x2)*(omousey - y3)) / ((y2 - y3)*(x1 - x3) + (x3 - x2)*(y1 - y3));
					real_t b = ((y3 - y1)*(omousex - x3) + (x1 - x3)*(omousey - y3)) / ((y2 - y3)*(x1 - x3) + (x3 - x2)*(y1 - y3));
					real_t c = 1 - a - b;
					if ( (0 <= a && a <= 1) && (0 <= b && b <= 1) && (0 <= c && c <= 1) )
					{
						//barycentric calc for figuring if mouse point is within triangle.
						highlight = i;
						drawImageRing(fancyWindow_bmp, &src, radius, thickness, (numoptions) * 8, angleStart, angleEnd, 192);

						// draw borders around highlighted item.
						Uint32 borderColor = uint32ColorBaronyBlue(*mainsurface);
						if ( skillLVL < AllyNPCSkillRequirements[i] )
						{
							borderColor = uint32ColorOrange(*mainsurface);
						}
						drawLine(xres / 2 + (radius - thickness) * cos(angleStart), yres / 2 - (radius - thickness) * sin(angleStart),
							xres / 2 + (radius + thickness) * cos(angleStart), yres / 2 - (radius + thickness) * sin(angleStart), borderColor, 192);
						drawLine(xres / 2 + (radius - thickness) * cos(angleEnd), yres / 2 - (radius - thickness) * sin(angleEnd),
							xres / 2 + (radius + thickness - 1) * cos(angleEnd), yres / 2 - (radius + thickness - 1) * sin(angleEnd), borderColor, 192);

						drawArcInvertedY(xres / 2, yres / 2, radius - thickness, std::round((angleStart * 180) / PI), ((angleEnd * 180) / PI), borderColor, 192);
						drawArcInvertedY(xres / 2, yres / 2, (radius + thickness), std::round((angleStart * 180) / PI), ((angleEnd * 180) / PI) + 1, borderColor, 192);
					}
				}
			}

			SDL_Rect txt;
			txt.x = src.x + src.w * cos(angleMiddle);
			txt.y = src.y - src.h * sin(angleMiddle);
			txt.w = 0;
			txt.h = 0;
			SDL_Rect img;
			img.x = txt.x - sidebar_unlock_bmp->w / 2;
			img.y = txt.y - sidebar_unlock_bmp->h / 2;
			img.w = sidebar_unlock_bmp->w;
			img.h = sidebar_unlock_bmp->h;

			// draw the text for the menu wheel.
			if ( i == ALLY_CMD_DEFEND 
				&& (followerMenuEntity->monsterAllyState == ALLY_STATE_DEFEND || followerMenuEntity->monsterAllyState == ALLY_STATE_MOVETO) )
			{
				TTF_SizeUTF8(ttf12, language[3037 + i + 8], &width, nullptr);
				ttfPrintText(ttf12, txt.x - width / 2, txt.y - 4, language[3037 + i + 8]);
			}
			else
			{
				TTF_SizeUTF8(ttf12, language[3037 + i], &width, nullptr);
				if ( skillLVL < AllyNPCSkillRequirements[i] )
				{
					drawImage(sidebar_unlock_bmp, nullptr, &img); // locked menu options
				}
				else if ( i == ALLY_CMD_CLASS_TOGGLE )
				{
					// draw higher.
					ttfPrintText(ttf12, txt.x - width / 2, txt.y - 12, language[3037 + i]);
					TTF_SizeUTF8(ttf12, language[3053 + followerMenuEntity->monsterAllyClass], &width, nullptr);
					ttfPrintText(ttf12, txt.x - width / 2, txt.y + 4, language[3053 + followerMenuEntity->monsterAllyClass]);
				}
				else if ( i == ALLY_CMD_PICKUP_TOGGLE )
				{
					// draw higher.
					TTF_SizeUTF8(ttf12, "Pickup", &width, nullptr);
					ttfPrintText(ttf12, txt.x - width / 2, txt.y - 24, language[3037 + i]);
					TTF_SizeUTF8(ttf12, language[3056 + followerMenuEntity->monsterAllyPickupItems], &width, nullptr);
					ttfPrintText(ttf12, txt.x - width / 2, txt.y + 12, language[3056 + followerMenuEntity->monsterAllyPickupItems]);
				}
				else if ( i == ALLY_CMD_DROP_EQUIP )
				{
					ttfPrintText(ttf12, txt.x - width / 2, txt.y - 12, language[3037 + i]);
					if ( skillLVL >= SKILL_LEVEL_LEGENDARY )
					{
						TTF_SizeUTF8(ttf12, language[3061], &width, nullptr);
						ttfPrintText(ttf12, txt.x - width / 2, txt.y + 4, language[3061]);
					}
					else if ( skillLVL >= SKILL_LEVEL_MASTER )
					{
						TTF_SizeUTF8(ttf12, language[3060], &width, nullptr);
						ttfPrintText(ttf12, txt.x - width / 2, txt.y + 4, language[3060]);
					}
					else
					{
						TTF_SizeUTF8(ttf12, language[3059], &width, nullptr);
						ttfPrintText(ttf12, txt.x - width / 2, txt.y + 4, language[3059]);
					}
				}
				else if ( i == ALLY_CMD_INTERACT )
				{
					TTF_SizeUTF8(ttf12, language[3037 + i], &width, nullptr);
					ttfPrintText(ttf12, txt.x - width / 2, txt.y - 4, language[3037 + i]);
				}
				else if ( i == ALLY_CMD_MOVETO_SELECT )
				{
					TTF_SizeUTF8(ttf12, language[3037 + i], &width, nullptr);
					ttfPrintText(ttf12, txt.x - width / 2, txt.y - 4, language[3037 + i]);
				}
				else
				{
					TTF_SizeUTF8(ttf12, language[3037 + i], &width, nullptr);
					ttfPrintText(ttf12, txt.x - width / 2, txt.y - 4, language[3037 + i]);
				}
			}

			angleStart += 2 * PI / numoptions;
			angleMiddle = angleStart + PI / numoptions;
			angleEnd = angleMiddle + PI / numoptions;
		}
		// draw center text.
		if ( mouseInCenterButton )
		{
			highlight = -1;
			//drawImageRing(fancyWindow_bmp, nullptr, 35, 35, 40, 0, 2 * PI, 192);
			drawCircle(xres / 2, yres / 2, 70, uint32ColorBaronyBlue(*mainsurface), 192);
			//TTF_SizeUTF8(ttf12, language[3063], &width, nullptr);
			//ttfPrintText(ttf12, xres / 2 - width / 2, yres / 2 - 8, language[3063]);
		}


		if ( disableOption )
		{
			SDL_Rect tooltip;
			tooltip.x = omousex + 16;
			tooltip.y = omousey + 16;
			tooltip.w = longestline(language[3062]) * TTF12_WIDTH + 8;
			tooltip.h = TTF12_HEIGHT * 2 + 8;
			drawTooltip(&tooltip);
			std::string requirement;
			if ( followerMenuOptionSelected >= ALLY_CMD_DEFEND && followerMenuOptionSelected <= ALLY_CMD_END - 1 )
			{
				switch ( AllyNPCSkillRequirements[followerMenuOptionSelected] )
				{
					case 0:
						requirement = language[363];
						break;
					case SKILL_LEVEL_NOVICE:
						requirement = language[364];
						break;
					case SKILL_LEVEL_BASIC:
						requirement = language[365];
						break;
					case SKILL_LEVEL_SKILLED:
						requirement = language[366];
						break;
					case SKILL_LEVEL_EXPERT:
						requirement = language[367];
						break;
					case SKILL_LEVEL_MASTER:
						requirement = language[368];
						break;
					case SKILL_LEVEL_LEGENDARY:
						requirement = language[369];
						break;
				}
				requirement.erase(std::remove(requirement.begin(), requirement.end(), ' '), requirement.end()); // trim whitespace
			}
			ttfPrintTextFormattedColor(ttf12, tooltip.x + 4, tooltip.y + 4, uint32ColorOrange(*mainsurface), language[3062], requirement.c_str());
		}

		if ( !keepWheelOpen )
		{
			followerMenuOptionSelected = highlight; // don't reselect if we're keeping the wheel open by using a toggle option.
		}
	}
}

void drawStatus()
{
	SDL_Rect pos, initial_position;
	Sint32 x, y, z, c, i;
	node_t* node;
	string_t* string;
	pos.x = STATUS_X;

	if ( !hide_statusbar )
	{
		pos.y = STATUS_Y;
	}
	else
	{
		pos.y = yres - 16;
	}
	//To garner the position of the hotbar.
	initial_position.x = HOTBAR_START_X;
	initial_position.y = pos.y;
	initial_position.w = 0;
	initial_position.h = 0;
	pos.w = status_bmp->w * uiscale_chatlog;
	pos.h = status_bmp->h * uiscale_chatlog;
	if ( !hide_statusbar )
	{
		drawImageScaled(status_bmp, NULL, &pos);
	}

	// hunger icon
	if ( stats[clientnum]->HUNGER <= 250 && (ticks % 50) - (ticks % 25) )
	{
		pos.x = 128;
		pos.y = yres - 160;
		pos.w = 64;
		pos.h = 64;
		drawImageScaled(hunger_bmp, NULL, &pos);
	}

	if ( minotaurlevel && (ticks % 50) - (ticks % 25) )
	{
		pos.x = 128;
		pos.y = yres - 160 + 64 + 2;
		pos.w = 64;
		pos.h = 64;
		drawImageScaled(minotaur_bmp, nullptr, &pos);
	}

	// enemy health
	if ( ticks - enemy_timer < 120 && enemy_timer )
	{
		enemy_hp = std::max(0, enemy_hp);

		// bar
		pos.x = xres / 2 - 256;
		pos.y = yres - 224;
		pos.w = 512;
		pos.h = 38;
		drawTooltip(&pos);
		pos.x = xres / 2 - 253;
		pos.y = yres - 221;
		pos.w = 506;
		pos.h = 32;
		drawRect(&pos, SDL_MapRGB(mainsurface->format, 16, 0, 0), 255);
		if ( enemy_hp > 0 )
		{
			pos.w = 506 * ((double)enemy_hp / enemy_maxhp);
			drawRect(&pos, SDL_MapRGB(mainsurface->format, 128, 0, 0), 255);
			if ( enemy_bar_color[clientnum] > 0 )
			{
				drawRect(&pos, enemy_bar_color[clientnum], 224);
			}
		}

		// name
		int x = xres / 2 - longestline(enemy_name) * TTF12_WIDTH / 2 + 2;
		int y = yres - 221 + 16 - TTF12_HEIGHT / 2 + 2;
		ttfPrintText(ttf12, x, y, enemy_name);
	}

	// messages
	if ( !hide_statusbar )
	{
		x = xres / 2 - (status_bmp->w * uiscale_chatlog / 2) + 24 * uiscale_chatlog;
		y = yres;
		textscroll = std::max(std::min<Uint32>(list_Size(&messages) - 3, textscroll), 0u);
		c = 0;
		for ( node = messages.last; node != NULL; node = node->prev )
		{
			c++;
			if ( c <= textscroll )
			{
				continue;
			}
			string = (string_t*)node->element;
			if ( uiscale_chatlog >= 1.5 )
			{
				y -= TTF16_HEIGHT * string->lines;
				if ( y < yres - (status_bmp->h * uiscale_chatlog) + 8 * uiscale_chatlog )
				{
					break;
				}
			}
			else if ( uiscale_chatlog != 1.f )
			{
				y -= TTF12_HEIGHT * string->lines;
				if ( y < yres - status_bmp->h * 1.1 + 4 )
				{
					break;
				}
			}
			else
			{
				y -= TTF12_HEIGHT * string->lines;
				if ( y < yres - status_bmp->h + 4 )
				{
					break;
				}
			}
			z = 0;
			for ( i = 0; i < strlen(string->data); i++ )
			{
				if ( string->data[i] != 10 )   // newline
				{
					z++;
				}
				else
				{
					z = 0;
				}
				if ( z == 65 )
				{
					if ( string->data[i] != 10 )
					{
						char* tempString = (char*)malloc(sizeof(char) * (strlen(string->data) + 2));
						strcpy(tempString, string->data);
						strcpy((char*)(tempString + i + 1), (char*)(string->data + i));
						tempString[i] = 10;
						free(string->data);
						string->data = tempString;
						string->lines++;
					}
					z = 0;
				}
			}
			Uint32 color = SDL_MapRGBA(mainsurface->format, 0, 0, 0, 255); // black color
			if ( uiscale_chatlog >= 1.5 )
			{
				ttfPrintTextColor(ttf16, x, y, color, false, string->data);
			}
			else
			{
				ttfPrintTextColor(ttf12, x, y, color, false, string->data);
			}
		}
		if ( mousestatus[SDL_BUTTON_LEFT] )
		{
			if ( omousey >= yres - status_bmp->h * uiscale_chatlog + 7 && omousey < yres - status_bmp->h * uiscale_chatlog + (7 + 27) * uiscale_chatlog )
			{
				if ( omousex >= xres / 2 - status_bmp->w * uiscale_chatlog / 2 + 618 * uiscale_chatlog && omousex < xres / 2 - status_bmp->w * uiscale_chatlog / 2 + 618 * uiscale_chatlog + 11 * uiscale_chatlog )
				{
					// text scroll up
					buttonclick = 3;
					textscroll++;
					mousestatus[SDL_BUTTON_LEFT] = 0;
				}
			}
			else if ( omousey >= yres - status_bmp->h * uiscale_chatlog + 34 && omousey < yres - status_bmp->h * uiscale_chatlog + (34 + 28) * uiscale_chatlog )
			{
				if ( omousex >= xres / 2 - status_bmp->w * uiscale_chatlog / 2 + 618 * uiscale_chatlog && omousex < xres / 2 - status_bmp->w * uiscale_chatlog / 2 + 618 * uiscale_chatlog + 11 * uiscale_chatlog )
				{
					// text scroll down
					buttonclick = 12;
					textscroll--;
					if ( textscroll < 0 )
					{
						textscroll = 0;
					}
					mousestatus[SDL_BUTTON_LEFT] = 0;
				}
			}
			else if ( omousey >= yres - status_bmp->h * uiscale_chatlog + 62 && omousey < yres - status_bmp->h * uiscale_chatlog + (62 + 31) * uiscale_chatlog )
			{
				if ( omousex >= xres / 2 - status_bmp->w * uiscale_chatlog / 2 + 618 * uiscale_chatlog && omousex < xres / 2 - status_bmp->w * uiscale_chatlog / 2 + 618 * uiscale_chatlog + 11 * uiscale_chatlog )
				{
					// text scroll down all the way
					buttonclick = 4;
					textscroll = 0;
					mousestatus[SDL_BUTTON_LEFT] = 0;
				}
			}
			/*else if( omousey>=yres-status_bmp->h+8 && omousey<yres-status_bmp->h+8+30 ) {
				if( omousex>=xres/2-status_bmp->w/2+618 && omousex<xres/2-status_bmp->w/2+618+11 ) {
					// text scroll up all the way
					buttonclick=13;
					textscroll=list_Size(&messages)-4;
					mousestatus[SDL_BUTTON_LEFT]=0;
				}
			}*/
		}

		// mouse wheel
		if ( mousex >= STATUS_X && mousex < STATUS_X + status_bmp->w * uiscale_chatlog )
		{
			if ( mousey >= initial_position.y && mousey < initial_position.y + status_bmp->h * uiscale_chatlog )
			{
				if ( mousestatus[SDL_BUTTON_WHEELDOWN] )
				{
					mousestatus[SDL_BUTTON_WHEELDOWN] = 0;
					textscroll--;
					if ( textscroll < 0 )
					{
						textscroll = 0;
					}
				}
				else if ( mousestatus[SDL_BUTTON_WHEELUP] )
				{
					mousestatus[SDL_BUTTON_WHEELUP] = 0;
					textscroll++;
				}
			}
		}
		if (showfirst)
		{
			textscroll = list_Size(&messages) - 3;
		}


		//Text scroll up button.
		if ( buttonclick == 3 )
		{
			pos.x = xres / 2 - status_bmp->w * uiscale_chatlog / 2 + 617 * uiscale_chatlog;
			pos.y = yres - status_bmp->h * uiscale_chatlog + 7 * uiscale_chatlog;
			pos.w = 11 * uiscale_chatlog;
			pos.h = 27 * uiscale_chatlog;
			drawRect(&pos, SDL_MapRGB(mainsurface->format, 255, 255, 255), 80);
			//drawImage(textup_bmp, NULL, &pos);
		}
		//Text scroll down all the way button.
		if ( buttonclick == 4 )
		{
			pos.x = xres / 2 - status_bmp->w * uiscale_chatlog / 2 + 617 * uiscale_chatlog;
			pos.y = yres - status_bmp->h * uiscale_chatlog + 62 * uiscale_chatlog;
			pos.w = 11 * uiscale_chatlog;
			pos.h = 31 * uiscale_chatlog;
			drawRect(&pos, SDL_MapRGB(mainsurface->format, 255, 255, 255), 80);
			//drawImage(textdown_bmp, NULL, &pos);
		}
		//Text scroll down button.
		if ( buttonclick == 12 )
		{
			pos.x = xres / 2 - status_bmp->w * uiscale_chatlog / 2 + 617 * uiscale_chatlog;
			pos.y = yres - status_bmp->h * uiscale_chatlog + 34 * uiscale_chatlog;
			pos.w = 11 * uiscale_chatlog;
			pos.h = 28 * uiscale_chatlog;
			drawRect(&pos, SDL_MapRGB(mainsurface->format, 255, 255, 255), 80);
			//drawImage(textup_bmp, NULL, &pos);
		}
		//Text scroll up all the way button.
		/*if( buttonclick==13 ) {
			pos.x=xres/2-status_bmp->w/2+617; pos.y=yres-status_bmp->h+8;
			pos.w=11; pos.h=30;
			drawRect(&pos,SDL_MapRGB(mainsurface->format,255,255,255),80);
			//drawImage(textdown_bmp, NULL, &pos);
		}*/
	}

	int playerStatusBarWidth = 38 * uiscale_playerbars;
	int playerStatusBarHeight = 156 * uiscale_playerbars;

	// PLAYER HEALTH BAR
	// Display Health bar border
	pos.x = 38 + 38 * uiscale_playerbars;
	pos.w = playerStatusBarWidth;
	pos.h = playerStatusBarHeight;
	pos.y = yres - (playerStatusBarHeight + 12);
	drawTooltip(&pos);

	// Display "HP" at top of Health bar
	ttfPrintText(ttf12, pos.x + (playerStatusBarWidth / 2 - 10), pos.y + 6, language[306]);

	// Display border between actual Health bar and "HP"
	//pos.x = 76;
	pos.w = playerStatusBarWidth;
	pos.h = 0;
	pos.y = yres - (playerStatusBarHeight - 9);
	drawTooltip(&pos);

	// Display the actual Health bar's faint background
	pos.x = 42 + 38 * uiscale_playerbars;
	pos.w = playerStatusBarWidth - 5;
	pos.h = playerStatusBarHeight - 27;
	pos.y = yres - 15 - pos.h;

	// Change the color depending on if you are poisoned
	Uint32 color = 0;
	if ( stats[clientnum]->EFFECTS[EFF_POISONED] )
	{
		if ( colorblind )
		{
			color = SDL_MapRGB(mainsurface->format, 0, 0, 48); // Display blue
		}
		else
		{
			color = SDL_MapRGB(mainsurface->format, 0, 48, 0); // Display green
		}
	}
	else
	{
		color = SDL_MapRGB(mainsurface->format, 48, 0, 0); // Display red
	}

	// Draw the actual Health bar's faint background with specified color
	drawRect(&pos, color, 255);

	// If the Player is alive, base the size of the actual Health bar off remaining HP
	if ( stats[clientnum]->HP > 0 )
	{
		//pos.x = 80;
		pos.w = playerStatusBarWidth - 5;
		pos.h = (playerStatusBarHeight - 27) * (static_cast<double>(stats[clientnum]->HP) / stats[clientnum]->MAXHP);
		pos.y = yres - 15 - pos.h;

		if ( stats[clientnum]->EFFECTS[EFF_POISONED] )
		{
			if ( !colorblind )
			{
				color = SDL_MapRGB(mainsurface->format, 0, 128, 0);
			}
			else
			{
				color = SDL_MapRGB(mainsurface->format, 0, 0, 128);
			}
		}
		else
		{
			color = SDL_MapRGB(mainsurface->format, 128, 0, 0);
		}

		// Only draw the actual Health bar if the Player is alive
		drawRect(&pos, color, 255);
	}

	// Print out the amount of HP the Player currently has
	snprintf(tempstr, 4, "%d", stats[clientnum]->HP);
	if ( uiscale_playerbars >= 1.5 )
	{
		pos.x += uiscale_playerbars * 2;
	}
	printTextFormatted(font12x12_bmp, pos.x + 16 * uiscale_playerbars - strlen(tempstr) * 6, yres - (playerStatusBarHeight / 2 + 8), tempstr);

	// PLAYER MAGIC BAR
	// Display the Magic bar border
	pos.x = 12 * uiscale_playerbars;
	pos.w = playerStatusBarWidth;
	pos.h = playerStatusBarHeight;
	pos.y = yres - (playerStatusBarHeight + 12);
	drawTooltip(&pos);

	// Display "MP" at the top of Magic bar
	ttfPrintText(ttf12, pos.x + (playerStatusBarWidth / 2 - 10), pos.y + 6, language[307]);

	// Display border between actual Magic bar and "MP"
	//pos.x = 12;
	pos.w = playerStatusBarWidth;
	pos.h = 0;
	pos.y = yres - (playerStatusBarHeight - 9);
	drawTooltip(&pos);

	// Display the actual Magic bar's faint background
	if ( uiscale_playerbars < 1.5 )
	{
		pos.x = 16;
	}
	else if ( uiscale_playerbars == 1.5 )
	{
		pos.x = 16 * uiscale_playerbars - 2;
	}
	else
	{
		pos.x = 16 * uiscale_playerbars - 4;
	}
	pos.w = playerStatusBarWidth - 5;
	pos.h = playerStatusBarHeight - 27;
	pos.y = yres - 15 - pos.h;

	// Draw the actual Magic bar's faint background
	drawRect(&pos, SDL_MapRGB(mainsurface->format, 0, 0, 48), 255); // Display blue

	// If the Player has MP, base the size of the actual Magic bar off remaining MP
	if ( stats[clientnum]->MP > 0 )
	{
		//pos.x = 16;
		pos.w = playerStatusBarWidth - 5;
		pos.h = (playerStatusBarHeight - 27) * (static_cast<double>(stats[clientnum]->MP) / stats[clientnum]->MAXMP);
		pos.y = yres - 15 - pos.h;

		// Only draw the actual Magic bar if the Player has MP
		drawRect(&pos, SDL_MapRGB(mainsurface->format, 0, 24, 128), 255); // Display blue
	}

	// Print out the amount of MP the Player currently has
	snprintf(tempstr, 4, "%d", stats[clientnum]->MP);
	printTextFormatted(font12x12_bmp, 32 * uiscale_playerbars - strlen(tempstr) * 6, yres - (playerStatusBarHeight / 2 + 8), tempstr);

	Item* item = nullptr;
	//Now the hotbar.
	int num = 0;
	//Reset the position to the top left corner of the status bar to draw the hotbar slots..
	//pos.x = initial_position.x;
	pos.x = (xres / 2) - 5 * hotbar_img->w * uiscale_hotbar;
	pos.y = initial_position.y - hotbar_img->h * uiscale_hotbar;
	for ( num = 0; num < NUM_HOTBAR_SLOTS; ++num, pos.x += hotbar_img->w * uiscale_hotbar )
	{
		Uint32 color;
		if ( current_hotbar == num && !openedChest[clientnum] )
		{
			color = SDL_MapRGBA(mainsurface->format, 255, 255, 0, 255); //Draw gold border around currently selected hotbar.
		}
		else
		{
			color = SDL_MapRGBA(mainsurface->format, 255, 255, 255, 60); //Draw normal grey border.
		}
		pos.w = hotbar_img->w * uiscale_hotbar;
		pos.h = hotbar_img->h * uiscale_hotbar;
		drawImageScaledColor(hotbar_img, NULL, &pos, color);

		item = uidToItem(hotbar[num].item);
		if ( item )
		{
			bool used = false;
			pos.w = hotbar_img->w * uiscale_hotbar;
			pos.h = hotbar_img->h * uiscale_hotbar;

			SDL_Rect highlightBox;
			highlightBox.x = pos.x + 2;
			highlightBox.y = pos.y + 2;
			highlightBox.w = 60 * uiscale_hotbar;
			highlightBox.h = 60 * uiscale_hotbar;

			if ( !item->identified )
			{
				// give it a yellow background if it is unidentified
				drawRect(&highlightBox, SDL_MapRGB(mainsurface->format, 128, 128, 0), 64); //31875
			}
			else if ( item->beatitude < 0 )
			{
				// give it a red background if cursed
				drawRect(&highlightBox, SDL_MapRGB(mainsurface->format, 128, 0, 0), 64);
			}
			else if ( item->beatitude > 0 )
			{
				// give it a green background if blessed (light blue if colorblind mode)
				if ( colorblind )
				{
					drawRect(&highlightBox, SDL_MapRGB(mainsurface->format, 50, 128, 128), 64);
				}
				else
				{
					drawRect(&highlightBox, SDL_MapRGB(mainsurface->format, 0, 128, 0), 64);
				}
			}
			if ( item->status == BROKEN )
			{
				drawRect(&highlightBox, SDL_MapRGB(mainsurface->format, 64, 64, 64), 125);
			}

			drawImageScaled(itemSprite(item), NULL, &pos);
			if ( stats[clientnum]->HP > 0 )
			{
				if ( !shootmode && mouseInBounds(pos.x, pos.x + hotbar_img->w * uiscale_hotbar, pos.y, pos.y + hotbar_img->h * uiscale_hotbar) )
				{
					if ( (mousestatus[SDL_BUTTON_LEFT] || (*inputPressed(joyimpulses[INJOY_MENU_LEFT_CLICK]) && !openedChest[clientnum] && gui_mode != (GUI_MODE_SHOP) && !identifygui_active && !removecursegui_active)) && !selectedItem )
					{
						toggleclick = false;
						if ( keystatus[SDL_SCANCODE_LSHIFT] || keystatus[SDL_SCANCODE_RSHIFT] )
						{
							hotbar[num].item = 0;
						}
						else
						{
							//Remove the item if left clicked.
							selectedItem = item;
							if ( selectedItem )
							{
								playSound(139, 64); // click sound
							}
							hotbar[num].item = 0;

							if ( *inputPressed(joyimpulses[INJOY_MENU_LEFT_CLICK]) && !openedChest[clientnum] && gui_mode != (GUI_MODE_SHOP) && !identifygui_active )
							{
								*inputPressed(joyimpulses[INJOY_MENU_LEFT_CLICK]) = 0;
								//itemSelectBehavior = BEHAVIOR_GAMEPAD;
								toggleclick = true;
								selectedItemFromHotbar = num;
								//TODO: Change the mouse cursor to THE HAND.
							}
						}
					}
					if ( mousestatus[SDL_BUTTON_RIGHT] || (*inputPressed(joyimpulses[INJOY_MENU_USE]) && !openedChest[clientnum] && gui_mode != (GUI_MODE_SHOP) && !identifygui_active && !removecursegui_active) )
					{
						//Use the item if right clicked.
						mousestatus[SDL_BUTTON_RIGHT] = 0;
						*inputPressed(joyimpulses[INJOY_MENU_USE]) = 0;
						bool badpotion = false;
						if ( itemCategory(item) == POTION && item->identified )
						{
							badpotion = isPotionBad(*item); //So that you wield empty potions be default.
						}
						if ( item->type == POTION_EMPTY )
						{
							badpotion = true;
						}

						if ( keystatus[SDL_SCANCODE_LSHIFT] || keystatus[SDL_SCANCODE_RSHIFT] )
						{
							identifygui_active = false;
							identifygui_appraising = true;

							//Cleanup identify GUI gamecontroller code here.
							selectedIdentifySlot = -1;

							identifyGUIIdentify(item);
						}
						else
						{
							if ( !badpotion )
							{
								useItem(item, clientnum);
							}
							else
							{
								if ( multiplayer == CLIENT )
								{
									strcpy((char*)net_packet->data, "EQUI");
									SDLNet_Write32((Uint32)item->type, &net_packet->data[4]);
									SDLNet_Write32((Uint32)item->status, &net_packet->data[8]);
									SDLNet_Write32((Uint32)item->beatitude, &net_packet->data[12]);
									SDLNet_Write32((Uint32)item->count, &net_packet->data[16]);
									SDLNet_Write32((Uint32)item->appearance, &net_packet->data[20]);
									net_packet->data[24] = item->identified;
									net_packet->data[25] = clientnum;
									net_packet->address.host = net_server.host;
									net_packet->address.port = net_server.port;
									net_packet->len = 26;
									sendPacketSafe(net_sock, -1, net_packet, 0);
								}
								equipItem(item, &stats[clientnum]->weapon, clientnum);
							}
							used = true;
						}
					}
				}
			}

			// item count
			if ( !used )
			{
				if ( item->count > 1 )
				{
					int digits = numdigits_sint16(item->count);
					SDL_Surface* digitFont = font12x12_bmp;
					if ( uiscale_hotbar >= 1.5 )
					{
						digitFont = font16x16_bmp;
						printTextFormatted(digitFont, pos.x + hotbar_img->w * uiscale_hotbar - (24 * digits), pos.y + hotbar_img->h * uiscale_hotbar - 24, "%d", item->count);
					}
					else
					{
						printTextFormatted(digitFont, pos.x + hotbar_img->w * uiscale_hotbar - (14 * digits), pos.y + hotbar_img->h * uiscale_hotbar - 14, "%d", item->count);
					}
				}

				SDL_Rect src;
				src.x = pos.x + 2;
				src.h = 16 * uiscale_hotbar;
				src.y = pos.y + hotbar_img->h * uiscale_hotbar - src.h - 2;
				src.w = 16 * uiscale_hotbar;

				// item equipped
				if ( itemCategory(item) != SPELL_CAT )
				{
					if ( itemIsEquipped(item, clientnum) )
					{
						drawImageScaled(equipped_bmp, NULL, &src);
					}
					else if ( item->status == BROKEN )
					{
						drawImageScaled(itembroken_bmp, NULL, &src);
					}
				}
				else
				{
					spell_t* spell = getSpellFromItem(item);
					if ( selected_spell == spell )
					{
						drawImageScaled(equipped_bmp, NULL, &src);
					}
				}
			}
		}
		if ( uiscale_hotbar >= 1.5 )
		{
			printTextFormatted(font16x16_bmp, pos.x + 2, pos.y + 2, "%d", (num + 1) % 10); // slot number
		}
		else
		{
			printTextFormatted(font12x12_bmp, pos.x + 2, pos.y + 2, "%d", (num + 1) % 10); // slot number
		}
	}

	if ( !shootmode )
	{
		pos.x = initial_position.x;
		//Go back through all of the hotbar slots and draw the tooltips.
		for ( num = 0; num < NUM_HOTBAR_SLOTS; ++num, pos.x += hotbar_img->w * uiscale_hotbar )
		{
			item = uidToItem(hotbar[num].item);
			if ( item )
			{
				if ( mouseInBounds(pos.x, pos.x + hotbar_img->w * uiscale_hotbar, pos.y, pos.y + hotbar_img->h * uiscale_hotbar) )
				{
					//Tooltip
					SDL_Rect src;
					src.x = mousex + 16;
					src.y = mousey + 8;
					if ( itemCategory(item) == SPELL_CAT )
					{
						spell_t* spell = getSpellFromItem(item);
						if ( spell )
						{
							char tempstr[64];
							if ( spell->ID == SPELL_DOMINATE )
							{
								snprintf(tempstr, 63, language[2977], getCostOfSpell(spell));
							}
							else
							{
								snprintf(tempstr, 31, language[308], getCostOfSpell(spell));
							}
							src.w = std::max(longestline(spell->name), longestline(tempstr)) * TTF12_WIDTH + 8;
							src.h = TTF12_HEIGHT * 2 + 8;
							drawTooltip(&src);
							ttfPrintTextFormatted(ttf12, src.x + 4, src.y + 4, "%s\n%s", spell->name, tempstr);
						}
						else
						{
							src.w = longestline("Error: Spell doesn't exist!") * TTF12_WIDTH + 8;
							src.h = TTF12_HEIGHT + 8;
							drawTooltip(&src);
							ttfPrintTextFormatted(ttf12, src.x + 4, src.y + 4, "%s", "Error: Spell doesn't exist!");
						}
					}
					else
					{
						src.w = std::max(13, longestline(item->description())) * TTF12_WIDTH + 8;
						src.h = TTF12_HEIGHT * 4 + 8;
						if ( item->identified )
							if ( itemCategory(item) == WEAPON || itemCategory(item) == ARMOR )
							{
								src.h += TTF12_HEIGHT;
							}
						drawTooltip(&src);

						Uint32 color = 0xFFFFFFFF;
						if ( !item->identified )
						{
							color = SDL_MapRGB(mainsurface->format, 255, 255, 0);
							ttfPrintTextFormattedColor(ttf12, src.x + 4 + TTF12_WIDTH, src.y + 4 + TTF12_HEIGHT, color, language[309]);
						}
						else
						{
							if ( item->beatitude < 0 )
							{
								color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
								ttfPrintTextFormattedColor(ttf12, src.x + 4 + TTF12_WIDTH, src.y + 4 + TTF12_HEIGHT, color, language[310]);
							}
							else if ( item->beatitude == 0 )
							{
								color = 0xFFFFFFFF;
								ttfPrintTextFormattedColor(ttf12, src.x + 4 + TTF12_WIDTH, src.y + 4 + TTF12_HEIGHT, color, language[311]);
							}
							else
							{
								color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
								ttfPrintTextFormattedColor(ttf12, src.x + 4 + TTF12_WIDTH, src.y + 4 + TTF12_HEIGHT, color, language[312]);
							}
						}
						if ( item->beatitude == 0 || !item->identified )
						{
							color = 0xFFFFFFFF;
						}
						ttfPrintTextFormattedColor(ttf12, src.x + 4, src.y + 4, color, "%s", item->description());
						ttfPrintTextFormatted(ttf12, src.x + 4 + TTF12_WIDTH, src.y + 4 + TTF12_HEIGHT * 2, language[313], items[item->type].weight * item->count);
						ttfPrintTextFormatted(ttf12, src.x + 4 + TTF12_WIDTH, src.y + 4 + TTF12_HEIGHT * 3, language[314], item->sellValue(clientnum));

						if ( item->identified )
						{
							if ( itemCategory(item) == WEAPON )
							{
								if ( item->weaponGetAttack() >= 0 )
								{
									color = SDL_MapRGB(mainsurface->format, 0, 255, 255);
								}
								else
								{
									color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
								}
								ttfPrintTextFormattedColor(ttf12, src.x + 4 + TTF12_WIDTH, src.y + 4 + TTF12_HEIGHT * 4, color, language[315], item->weaponGetAttack());
							}
							else if ( itemCategory(item) == ARMOR )
							{
								if ( item->armorGetAC() >= 0 )
								{
									color = SDL_MapRGB(mainsurface->format, 0, 255, 255);
								}
								else
								{
									color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
								}
								ttfPrintTextFormattedColor(ttf12, src.x + 4 + TTF12_WIDTH, src.y + 4 + TTF12_HEIGHT * 4, color, language[316], item->armorGetAC());
							}
						}
					}
					if ( hotbar_numkey_quick_add )
					{
						Uint32 swapItem = 0;
						if ( keystatus[SDL_SCANCODE_1] )
						{
							keystatus[SDL_SCANCODE_1] = 0;
							swapItem = hotbar[0].item;
							hotbar[0].item = hotbar[num].item;
							hotbar[num].item = swapItem;
						}
						if ( keystatus[SDL_SCANCODE_2] )
						{
							keystatus[SDL_SCANCODE_2] = 0;
							swapItem = hotbar[1].item;
							hotbar[1].item = hotbar[num].item;
							hotbar[num].item = swapItem;
						}
						if ( keystatus[SDL_SCANCODE_3] )
						{
							keystatus[SDL_SCANCODE_3] = 0;
							swapItem = hotbar[2].item;
							hotbar[2].item = hotbar[num].item;
							hotbar[num].item = swapItem;
						}
						if ( keystatus[SDL_SCANCODE_4] )
						{
							keystatus[SDL_SCANCODE_4] = 0;
							swapItem = hotbar[3].item;
							hotbar[3].item = hotbar[num].item;
							hotbar[num].item = swapItem;
						}
						if ( keystatus[SDL_SCANCODE_5] )
						{
							keystatus[SDL_SCANCODE_5] = 0;
							swapItem = hotbar[4].item;
							hotbar[4].item = hotbar[num].item;
							hotbar[num].item = swapItem;
						}
						if ( keystatus[SDL_SCANCODE_6] )
						{
							keystatus[SDL_SCANCODE_6] = 0;
							swapItem = hotbar[5].item;
							hotbar[5].item = hotbar[num].item;
							hotbar[num].item = swapItem;
						}
						if ( keystatus[SDL_SCANCODE_7] )
						{
							keystatus[SDL_SCANCODE_7] = 0;
							swapItem = hotbar[6].item;
							hotbar[6].item = hotbar[num].item;
							hotbar[num].item = swapItem;
						}
						if ( keystatus[SDL_SCANCODE_8] )
						{
							keystatus[SDL_SCANCODE_8] = 0;
							swapItem = hotbar[7].item;
							hotbar[7].item = hotbar[num].item;
							hotbar[num].item = swapItem;
						}
						if ( keystatus[SDL_SCANCODE_9] )
						{
							keystatus[SDL_SCANCODE_9] = 0;
							swapItem = hotbar[8].item;
							hotbar[8].item = hotbar[num].item;
							hotbar[num].item = swapItem;
						}
						if ( keystatus[SDL_SCANCODE_0] )
						{
							keystatus[SDL_SCANCODE_0] = 0;
							swapItem = hotbar[9].item;
							hotbar[9].item = hotbar[num].item;
							hotbar[num].item = swapItem;
						}
					}
				}
			}
		}

		// minimap pinging.
		int minimapTotalScale = minimapScaleQuickToggle + minimapScale;
		if ( mouseInBounds(xres - map.width * minimapTotalScale, xres, yres - map.height * minimapTotalScale, yres) ) // mouse within minimap pixels (each map tile is 4 pixels)
		{
			if ( mousestatus[SDL_BUTTON_RIGHT] || (*inputPressed(joyimpulses[INJOY_MENU_USE])) )
			{
				mousestatus[SDL_BUTTON_RIGHT] = 0;
				*inputPressed(joyimpulses[INJOY_MENU_USE]) = 0;
				if ( minimapPingGimpTimer == -1 )
				{
					MinimapPing newPing(ticks, clientnum, (omousex - (xres - map.width * minimapTotalScale)) / minimapTotalScale, (omousey - (yres - map.height * minimapTotalScale)) / minimapTotalScale);
					minimapPingGimpTimer = TICKS_PER_SECOND / 4;
					if ( multiplayer != CLIENT )
					{
						minimapPingAdd(newPing);
					}
					sendMinimapPing(clientnum, newPing.x, newPing.y);
				}
			}
		}
	}
	if ( minimapPingGimpTimer >= 0 )
	{
		--minimapPingGimpTimer;
	}

	//NOTE: If you change the number of hotbar slots, you *MUST* change this.
	if ( !command && stats[clientnum]->HP > 0 )
	{
		Item* item = NULL;
		if ( !(!shootmode && hotbar_numkey_quick_add &&
				(
					(omousex >= INVENTORY_STARTX
						&& omousex <= INVENTORY_STARTX + INVENTORY_SIZEX * INVENTORY_SLOTSIZE
						&& omousey >= INVENTORY_STARTY 
						&& omousey <= INVENTORY_STARTY + INVENTORY_SIZEY * INVENTORY_SLOTSIZE
					)
					||
					(omousex >= initial_position.x 
						&& omousex <= initial_position.x + hotbar_img->w * uiscale_hotbar * 10
						&& omousey >= initial_position.y - hotbar_img->h * uiscale_hotbar
						&& omousey <= initial_position.y
					)
				)
			) )
		{
			// if hotbar_numkey_quick_add is enabled, then the number keys won't do the default equip function
			// skips equipping items if the mouse is in the hotbar or inventory area. otherwise the below code runs.
			if ( keystatus[SDL_SCANCODE_1] )
			{
				keystatus[SDL_SCANCODE_1] = 0;
				item = uidToItem(hotbar[0].item);
			}
			if ( keystatus[SDL_SCANCODE_2] )
			{
				keystatus[SDL_SCANCODE_2] = 0;
				item = uidToItem(hotbar[1].item);
			}
			if ( keystatus[SDL_SCANCODE_3] )
			{
				keystatus[SDL_SCANCODE_3] = 0;
				item = uidToItem(hotbar[2].item);
			}
			if ( keystatus[SDL_SCANCODE_4] )
			{
				keystatus[SDL_SCANCODE_4] = 0;
				item = uidToItem(hotbar[3].item);
			}
			if ( keystatus[SDL_SCANCODE_5] )
			{
				keystatus[SDL_SCANCODE_5] = 0;
				item = uidToItem(hotbar[4].item);
			}
			if ( keystatus[SDL_SCANCODE_6] )
			{
				keystatus[SDL_SCANCODE_6] = 0;
				item = uidToItem(hotbar[5].item);
			}
			if ( keystatus[SDL_SCANCODE_7] )
			{
				keystatus[SDL_SCANCODE_7] = 0;
				item = uidToItem(hotbar[6].item);
			}
			if ( keystatus[SDL_SCANCODE_8] )
			{
				keystatus[SDL_SCANCODE_8] = 0;
				item = uidToItem(hotbar[7].item);
			}
			if ( keystatus[SDL_SCANCODE_9] )
			{
				keystatus[SDL_SCANCODE_9] = 0;
				item = uidToItem(hotbar[8].item);
			}
			if ( keystatus[SDL_SCANCODE_0] )
			{
				keystatus[SDL_SCANCODE_0] = 0;
				item = uidToItem(hotbar[9].item);
			}
		}

		//Moving the cursor changes the currently selected hotbar slot.
		if ( (mousexrel || mouseyrel) && !shootmode )
		{
			pos.x = initial_position.x;
			pos.y = initial_position.y - hotbar_img->h * uiscale_hotbar;
			for ( c = 0; c < NUM_HOTBAR_SLOTS; ++c, pos.x += hotbar_img->w * uiscale_hotbar )
			{
				if ( mouseInBoundsRealtimeCoords(pos.x, pos.x + hotbar_img->w * uiscale_hotbar, pos.y, pos.y + hotbar_img->h * uiscale_hotbar) )
				{
					selectHotbarSlot(c);
				}
			}
		}

		bool bumper_moved = false;
		//Gamepad change hotbar selection.
		if ( shootmode && *inputPressed(joyimpulses[INJOY_GAME_HOTBAR_NEXT]) && !itemMenuOpen && !openedChest[clientnum] && gui_mode != (GUI_MODE_SHOP) && !book_open && !identifygui_active && !removecursegui_active )
		{
			*inputPressed(joyimpulses[INJOY_GAME_HOTBAR_NEXT]) = 0;
			selectHotbarSlot(current_hotbar + 1);
			bumper_moved = true;
		}
		if ( shootmode && *inputPressed(joyimpulses[INJOY_GAME_HOTBAR_PREV]) && !itemMenuOpen && !openedChest[clientnum] && gui_mode != (GUI_MODE_SHOP) && !book_open && !identifygui_active && !removecursegui_active )
		{
			*inputPressed(joyimpulses[INJOY_GAME_HOTBAR_PREV]) = 0;
			selectHotbarSlot(current_hotbar - 1);
			bumper_moved = true;
		}

		if ( bumper_moved && !itemMenuOpen && !openedChest[clientnum] && gui_mode != (GUI_MODE_SHOP) && !book_open && !identifygui_active && !removecursegui_active )
		{
			warpMouseToSelectedHotbarSlot();
		}

		if ( !itemMenuOpen && !selectedItem && !openedChest[clientnum] && gui_mode != (GUI_MODE_SHOP) )
		{
			if ( shootmode && *inputPressed(joyimpulses[INJOY_GAME_HOTBAR_ACTIVATE]) && !openedChest[clientnum] && gui_mode != (GUI_MODE_SHOP) && !book_open && !identifygui_active && !removecursegui_active )
			{
				//Activate a hotbar slot if in-game.
				*inputPressed(joyimpulses[INJOY_GAME_HOTBAR_ACTIVATE]) = 0;
				item = uidToItem(hotbar[current_hotbar].item);
			}

			if ( !shootmode && *inputPressed(joyimpulses[INJOY_MENU_HOTBAR_CLEAR]) && !book_open ) //TODO: Don't activate if any of the previous if statement's conditions are true?
			{
				//Clear a hotbar slot if in-inventory.
				*inputPressed(joyimpulses[INJOY_MENU_HOTBAR_CLEAR]) = 0;

				hotbar[current_hotbar].item = 0;
			}

			pos.x = initial_position.x + (current_hotbar * hotbar_img->w * uiscale_hotbar);
			pos.y = initial_position.y - hotbar_img->h * uiscale_hotbar;
			if ( !shootmode && !book_open && !openedChest[clientnum] && *inputPressed(joyimpulses[INJOY_MENU_DROP_ITEM]) && mouseInBounds(pos.x, pos.x + hotbar_img->w * uiscale_hotbar, pos.y, pos.y + hotbar_img->h * uiscale_hotbar) )
			{
				//Drop item if this hotbar is currently active & the player pressed the cancel button on the gamepad (typically "b").
				*inputPressed(joyimpulses[INJOY_MENU_DROP_ITEM]) = 0;
				Item* itemToDrop = uidToItem(hotbar[current_hotbar].item);
				if ( itemToDrop )
				{
					dropItem(itemToDrop, clientnum);
				}
			}
		}

		if ( item )
		{
			bool badpotion = false;
			if ( itemCategory(item) == POTION && item->identified )
			{
				badpotion = isPotionBad(*item);
			}
			if ( item->type == POTION_EMPTY )
			{
				badpotion = true; //So that you wield empty potions be default.
			}

			if ( !badpotion )
			{
				useItem(item, clientnum);
			}
			else
			{
				if ( multiplayer == CLIENT )
				{
					strcpy((char*)net_packet->data, "EQUI");
					SDLNet_Write32((Uint32)item->type, &net_packet->data[4]);
					SDLNet_Write32((Uint32)item->status, &net_packet->data[8]);
					SDLNet_Write32((Uint32)item->beatitude, &net_packet->data[12]);
					SDLNet_Write32((Uint32)item->count, &net_packet->data[16]);
					SDLNet_Write32((Uint32)item->appearance, &net_packet->data[20]);
					net_packet->data[24] = item->identified;
					net_packet->data[25] = clientnum;
					net_packet->address.host = net_server.host;
					net_packet->address.port = net_server.port;
					net_packet->len = 26;
					sendPacketSafe(net_sock, -1, net_packet, 0);
				}
				equipItem(item, &stats[clientnum]->weapon, clientnum);
			}
		}
	}

	drawFollowerMenu();

	// stat increase icons
	pos.w = 64;
	pos.h = 64;
	pos.x = xres - pos.w * 3 - 9;
	pos.y = (NUMPROFICIENCIES * TTF12_HEIGHT) + (TTF12_HEIGHT * 3) + (32 + 64 + 64 + 3); // 131px from end of prof window.

	if ( pos.y + pos.h > (yres - map.height * 4) ) // check if overlapping minimap
	{
		pos.y = (yres - map.height * 4) - (64 + 3); // align above minimap
	}
	
	SDL_Surface *tmp_bmp = NULL;

	for ( i = 0; i < NUMSTATS; i++ )
	{
		if ( stats[clientnum]->PLAYER_LVL_STAT_TIMER[i] > 0 && ((ticks % 50) - (ticks % 10)) )
		{
			stats[clientnum]->PLAYER_LVL_STAT_TIMER[i]--;

			switch ( i )
			{
				// prepare the stat image.
				case STAT_STR:
					tmp_bmp = str_bmp64u;
					break;
				case STAT_DEX:
					tmp_bmp = dex_bmp64u;
					break;
				case STAT_CON:
					tmp_bmp = con_bmp64u;
					break;
				case STAT_INT:
					tmp_bmp = int_bmp64u;
					break;
				case STAT_PER:
					tmp_bmp = per_bmp64u;
					break;
				case STAT_CHR:
					tmp_bmp = chr_bmp64u;
					break;
				default:
					break;
			}
			drawImageScaled(tmp_bmp, NULL, &pos);
			if ( stats[clientnum]->PLAYER_LVL_STAT_TIMER[i + NUMSTATS] > 0 )
			{
				// bonus stat acheived, draw additional stat icon above.
				pos.y -= 64 + 3;
				drawImageScaled(tmp_bmp, NULL, &pos);
				pos.y += 64 + 3;
				stats[clientnum]->PLAYER_LVL_STAT_TIMER[i + NUMSTATS]--;
			}

			pos.x += pos.h + 3;
		}
	}
}