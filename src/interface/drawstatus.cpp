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

//char enemy_name[128];
//Sint32 enemy_hp = 0, enemy_maxhp = 0, enemy_oldhp = 0;
//Uint32 enemy_timer = 0, enemy_lastuid = 0;
//Uint32 enemy_bar_color[MAXPLAYERS] = { 0 }; // color for each player's enemy bar to display. multiplayer clients only refer to their own [clientnum] entry.
int magicBoomerangHotbarSlot = -1;
Uint32 hotbarTooltipLastGameTick = 0;

/*-------------------------------------------------------------------------------

	handleDamageIndicators

	draws damage indicators, fades them, culls them, etc.

-------------------------------------------------------------------------------*/

void handleDamageIndicators(int player)
{
	node_t* node, *nextnode;
	for ( node = damageIndicators.first; node != NULL; node = nextnode )
	{
		nextnode = node->next;
		damageIndicator_t* damageIndicator = (damageIndicator_t*)node->element;

		double tangent = atan2( damageIndicator->y / 16 - cameras[player].y, damageIndicator->x / 16 - cameras[player].x );
		double angle = tangent - cameras[player].ang;
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
			enemyHPDamageBarHandler.enemy_bar_client_colors[player] = SDL_MapRGB(mainsurface->format, 0, 0, 64); // Display blue
		}
		else
		{
			enemyHPDamageBarHandler.enemy_bar_client_colors[player] = SDL_MapRGB(mainsurface->format, 0, 64, 0); // Display green
		}
	}
	else if ( targetStats.EFFECTS[EFF_PARALYZED] )
	{
		enemyHPDamageBarHandler.enemy_bar_client_colors[player] = SDL_MapRGB(mainsurface->format, 112, 112, 0);
	}
	else if ( targetStats.EFFECTS[EFF_CONFUSED] || targetStats.EFFECTS[EFF_DISORIENTED] )
	{
		enemyHPDamageBarHandler.enemy_bar_client_colors[player] = SDL_MapRGB(mainsurface->format, 92, 0, 92);
	}
	else if ( targetStats.EFFECTS[EFF_PACIFY] )
	{
		enemyHPDamageBarHandler.enemy_bar_client_colors[player] = SDL_MapRGB(mainsurface->format, 128, 32, 80);
	}
	else if ( targetStats.EFFECTS[EFF_BLIND] )
	{
		enemyHPDamageBarHandler.enemy_bar_client_colors[player] = SDL_MapRGB(mainsurface->format, 64, 64, 64);
	}
	else
	{
		enemyHPDamageBarHandler.enemy_bar_client_colors[player] = 0;
	}
}

/*-------------------------------------------------------------------------------

	updateEnemyBar

	updates the enemy hp bar for the given player

-------------------------------------------------------------------------------*/

void updateEnemyBar(Entity* source, Entity* target, char* name, Sint32 hp, Sint32 maxhp, bool lowPriorityTick)
{
	// server/singleplayer only function.
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

	if ( player == -1 )
	{
		if ( source->behavior == &actMonster && source->monsterAllySummonRank != 0
			&& (target->behavior == &actMonster || target->behavior == &actPlayer) )
		{
			if ( source->monsterAllyGetPlayerLeader() && source->monsterAllyGetPlayerLeader() != target )
			{
				player = source->monsterAllyIndex; // don't update enemy bar if attacking leader.
			}
		}
		else if ( source->behavior == &actMonster && source->monsterIllusionTauntingThisUid != 0 )
		{
			Entity* parent = uidToEntity(source->parent);
			if ( parent && parent->behavior == &actPlayer && parent != target )
			{
				player = parent->skill[2]; // don't update enemy bar if attacking leader.
			}
		}
		else if ( source->behavior == &actMonster && monsterIsImmobileTurret(source, nullptr)
			&& (target->behavior == &actMonster || target->behavior == &actPlayer) )
		{
			if ( source->monsterAllyGetPlayerLeader() && source->monsterAllyGetPlayerLeader() != target )
			{
				player = source->monsterAllyIndex; // don't update enemy bar if attacking leader.
			}
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
		}

		if ( player >= 0 )
		{
			updateEnemyBarStatusEffectColor(player, *target, *stats); // set color depending on status effects of the target.
		}
	}
	else if ( player >= 0 )
	{
		enemyHPDamageBarHandler.enemy_bar_client_colors[player] = 0;
	}

	//if ( player >= 0 )
	//{
	//	if ( enemy_lastuid != target->getUID() || enemy_timer == 0 )
	//	{
	//		// if new target or timer expired, get new OLDHP value.
	//		if ( stats )
	//		{
	//			enemy_oldhp = stats->OLDHP;
	//		}
	//	}
	//	if ( !stats )
	//	{
	//		enemy_oldhp = hp; // chairs/tables and things.
	//	}
	//	enemy_lastuid = target->getUID();
	//}
	if ( player == clientnum )
	{
		if ( stats )
		{
			enemyHPDamageBarHandler.addEnemyToList(hp, maxhp, stats->OLDHP, 
				enemyHPDamageBarHandler.enemy_bar_client_colors[player], target->getUID(), name, lowPriorityTick);
		}
		else
		{
			enemyHPDamageBarHandler.addEnemyToList(hp, maxhp, hp, 
				enemyHPDamageBarHandler.enemy_bar_client_colors[player], target->getUID(), name, lowPriorityTick);
		}
	}
	else if ( player > 0 && multiplayer == SERVER )
	{
		strcpy((char*)net_packet->data, "ENHP");
		SDLNet_Write32(hp, &net_packet->data[4]);
		SDLNet_Write32(maxhp, &net_packet->data[8]);
		SDLNet_Write32(enemyHPDamageBarHandler.enemy_bar_client_colors[player], &net_packet->data[12]);
		if ( stats )
		{
			SDLNet_Write32(stats->OLDHP, &net_packet->data[16]);
		}
		else
		{
			SDLNet_Write32(hp, &net_packet->data[16]);
		}
		SDLNet_Write32(target->getUID(), &net_packet->data[20]);
		net_packet->data[24] = lowPriorityTick ? 1 : 0; // 1 == true
		strcpy((char*)(&net_packet->data[25]), name);
		net_packet->data[25 + strlen(name)] = 0;
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 25 + strlen(name) + 1;
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

	interfaceMessageStatusBar.x = pos.x;
	interfaceMessageStatusBar.y = pos.y;
	interfaceMessageStatusBar.w = pos.w;
	interfaceMessageStatusBar.h = pos.h;

	// enemy health
	enemyHPDamageBarHandler.displayCurrentHPBar();

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
		if ( !shootmode )
		{
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
	if ( stats[clientnum] && stats[clientnum]->HP > 0 
		&& stats[clientnum]->EFFECTS[EFF_HP_REGEN] )
	{
		bool lowDurationFlash = !((ticks % 50) - (ticks % 25));
		bool lowDuration = stats[clientnum]->EFFECTS_TIMERS[EFF_HP_REGEN] > 0 &&
			(stats[clientnum]->EFFECTS_TIMERS[EFF_HP_REGEN] < TICKS_PER_SECOND * 5);
		if ( (lowDuration && !lowDurationFlash) || !lowDuration )
		{
			if ( colorblind )
			{
				drawTooltip(&pos, SDL_MapRGB(mainsurface->format, 0, 255, 255)); // blue
			}
			else
			{
				drawTooltip(&pos, SDL_MapRGB(mainsurface->format, 0, 255, 0)); // green
			}
		}
	}

	// Display "HP" at top of Health bar
	ttfPrintText(ttf12, pos.x + (playerStatusBarWidth / 2 - 10), pos.y + 6, language[306]);

	// Display border between actual Health bar and "HP"
	//pos.x = 76;
	pos.w = playerStatusBarWidth;
	pos.h = 0;
	pos.y = yres - (playerStatusBarHeight - 9);
	drawTooltip(&pos);
	if ( stats[clientnum] && stats[clientnum]->HP > 0
		&& stats[clientnum]->EFFECTS[EFF_HP_REGEN] )
	{
		bool lowDurationFlash = !((ticks % 50) - (ticks % 25));
		bool lowDuration = stats[clientnum]->EFFECTS_TIMERS[EFF_HP_REGEN] > 0 &&
			(stats[clientnum]->EFFECTS_TIMERS[EFF_HP_REGEN] < TICKS_PER_SECOND * 5);
		if ( (lowDuration && !lowDurationFlash) || !lowDuration )
		{
			if ( colorblind )
			{
				drawTooltip(&pos, SDL_MapRGB(mainsurface->format, 0, 255, 255)); // blue
			}
			else
			{
				drawTooltip(&pos, SDL_MapRGB(mainsurface->format, 0, 255, 0)); // green
			}
		}
	}

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
	int xoffset = pos.x;

	// hunger icon
	if ( stats[clientnum]->type != AUTOMATON 
		&& (svFlags & SV_FLAG_HUNGER) && stats[clientnum]->HUNGER <= 250 && (ticks % 50) - (ticks % 25) )
	{
		pos.x = xoffset + playerStatusBarWidth + 10; // was pos.x = 128;
		pos.y = yres - 160;
		pos.w = 64;
		pos.h = 64;
		if ( players[clientnum] && players[clientnum]->entity && players[clientnum]->entity->playerRequiresBloodToSustain() )
		{
			drawImageScaled(hunger_blood_bmp, NULL, &pos);
		}
		else
		{
			drawImageScaled(hunger_bmp, NULL, &pos);
		}
	}

	if ( stats[clientnum]->type == AUTOMATON )
	{
		if ( stats[clientnum]->HUNGER > 300 || (ticks % 50) - (ticks % 25) )
		{
			pos.x = xoffset + playerStatusBarWidth + 10; // was pos.x = 128;
			pos.y = yres - 160;
			pos.w = 64;
			pos.h = 64;
			if ( stats[clientnum]->HUNGER > 1200 )
			{
				drawImageScaled(hunger_boiler_hotflame_bmp, nullptr, &pos);
			}
			else
			{
				if ( stats[clientnum]->HUNGER > 600 )
				{
					drawImageScaledPartial(hunger_boiler_flame_bmp, nullptr, &pos, 1.f);
				}
				else
				{
					float percent = (stats[clientnum]->HUNGER - 300) / 300.f; // always show a little bit more at the bottom (10-20%)
					drawImageScaledPartial(hunger_boiler_flame_bmp, nullptr, &pos, percent);
				}
			}
			drawImageScaled(hunger_boiler_bmp, nullptr, &pos);
		}
	}

	// minotaur icon
	if ( minotaurlevel && (ticks % 50) - (ticks % 25) )
	{
		pos.x = xoffset + playerStatusBarWidth + 10; // was pos.x = 128;
		pos.y = yres - 160 + 64 + 2;
		pos.w = 64;
		pos.h = 64;
		drawImageScaled(minotaur_bmp, nullptr, &pos);
	}


	// PLAYER MAGIC BAR
	// Display the Magic bar border
	pos.x = 12 * uiscale_playerbars;
	pos.w = playerStatusBarWidth;
	pos.h = playerStatusBarHeight;
	pos.y = yres - (playerStatusBarHeight + 12);
	drawTooltip(&pos);
	if ( stats[clientnum] && stats[clientnum]->HP > 0
		&& stats[clientnum]->EFFECTS[EFF_MP_REGEN] )
	{
		bool lowDurationFlash = !((ticks % 50) - (ticks % 25));
		bool lowDuration = stats[clientnum]->EFFECTS_TIMERS[EFF_MP_REGEN] > 0 &&
			(stats[clientnum]->EFFECTS_TIMERS[EFF_MP_REGEN] < TICKS_PER_SECOND * 5);
		if ( (lowDuration && !lowDurationFlash) || !lowDuration )
		{
			if ( colorblind )
			{
				drawTooltip(&pos, SDL_MapRGB(mainsurface->format, 0, 255, 255)); // blue
			}
			else
			{
				drawTooltip(&pos, SDL_MapRGB(mainsurface->format, 0, 255, 0)); // green
			}
		}
	}
	Uint32 mpColorBG = SDL_MapRGB(mainsurface->format, 0, 0, 48);
	Uint32 mpColorFG = SDL_MapRGB(mainsurface->format, 0, 24, 128);
	if ( stats[clientnum] && stats[clientnum]->playerRace == RACE_INSECTOID && stats[clientnum]->appearance == 0 )
	{
		ttfPrintText(ttf12, pos.x + (playerStatusBarWidth / 2 - 10), pos.y + 6, language[3768]);
		mpColorBG = SDL_MapRGB(mainsurface->format, 32, 48, 0);
		mpColorFG = SDL_MapRGB(mainsurface->format, 92, 192, 0);
	}
	else if ( stats[clientnum] && stats[clientnum]->type == AUTOMATON )
	{
		ttfPrintText(ttf12, pos.x + (playerStatusBarWidth / 2 - 10), pos.y + 6, language[3474]);
		mpColorBG = SDL_MapRGB(mainsurface->format, 64, 32, 0);
		mpColorFG = SDL_MapRGB(mainsurface->format, 192, 92, 0);
	}
	else
	{
		// Display "MP" at the top of Magic bar
		ttfPrintText(ttf12, pos.x + (playerStatusBarWidth / 2 - 10), pos.y + 6, language[307]);
	}

	// Display border between actual Magic bar and "MP"
	//pos.x = 12;
	pos.w = playerStatusBarWidth;
	pos.h = 0;
	pos.y = yres - (playerStatusBarHeight - 9);
	drawTooltip(&pos);
	if ( stats[clientnum] && stats[clientnum]->HP > 0
		&& stats[clientnum]->EFFECTS[EFF_MP_REGEN] )
	{
		bool lowDurationFlash = !((ticks % 50) - (ticks % 25));
		bool lowDuration = stats[clientnum]->EFFECTS_TIMERS[EFF_MP_REGEN] > 0 &&
			(stats[clientnum]->EFFECTS_TIMERS[EFF_MP_REGEN] < TICKS_PER_SECOND * 5);
		if ( (lowDuration && !lowDurationFlash) || !lowDuration )
		{
			if ( colorblind )
			{
				drawTooltip(&pos, SDL_MapRGB(mainsurface->format, 0, 255, 255)); // blue
			}
			else
			{
				drawTooltip(&pos, SDL_MapRGB(mainsurface->format, 0, 255, 0)); // green
			}
		}
	}

	// Display the actual Magic bar's faint background
	pos.x = 4 + 12 * uiscale_playerbars;
	pos.w = playerStatusBarWidth - 5;
	pos.h = playerStatusBarHeight - 27;
	pos.y = yres - 15 - pos.h;

	// Draw the actual Magic bar's faint background
	drawRect(&pos, mpColorBG, 255); // Display blue

	// If the Player has MP, base the size of the actual Magic bar off remaining MP
	if ( stats[clientnum]->MP > 0 )
	{
		//pos.x = 16;
		pos.w = playerStatusBarWidth - 5;
		pos.h = (playerStatusBarHeight - 27) * (static_cast<double>(stats[clientnum]->MP) / stats[clientnum]->MAXMP);
		pos.y = yres - 15 - pos.h;

		// Only draw the actual Magic bar if the Player has MP
		drawRect(&pos, mpColorFG, 255); // Display blue
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
			if ( item->type == BOOMERANG )
			{
				magicBoomerangHotbarSlot = num;
			}
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

			bool disableItemUsage = false;

			if ( players[clientnum] && players[clientnum]->entity && players[clientnum]->entity->effectShapeshift != NOTHING )
			{
				// shape shifted, disable some items
				if ( !item->usableWhileShapeshifted(stats[clientnum]) )
				{
					disableItemUsage = true;
					drawRect(&highlightBox, SDL_MapRGB(mainsurface->format, 64, 64, 64), 144);
				}
			}
			if ( client_classes[clientnum] == CLASS_SHAMAN )
			{
				if ( item->type == SPELL_ITEM && !(playerUnlockedShamanSpell(clientnum, item)) )
				{
					disableItemUsage = true;
					drawRect(&highlightBox, SDL_MapRGB(mainsurface->format, 64, 64, 64), 144);
				}
			}

			if ( stats[clientnum]->HP > 0 )
			{
				if ( !shootmode && mouseInBounds(pos.x, pos.x + hotbar_img->w * uiscale_hotbar, pos.y, pos.y + hotbar_img->h * uiscale_hotbar) )
				{
					if ( (mousestatus[SDL_BUTTON_LEFT] 
						|| (*inputPressed(joyimpulses[INJOY_MENU_LEFT_CLICK]) 
							&& !openedChest[clientnum] 
							&& gui_mode != (GUI_MODE_SHOP) 
							&& !identifygui_active
							&& !removecursegui_active
							&& !GenericGUI.isGUIOpen())) 
						&& !selectedItem )
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
					if ( mousestatus[SDL_BUTTON_RIGHT] 
						|| (*inputPressed(joyimpulses[INJOY_MENU_USE]) 
							&& !openedChest[clientnum] 
							&& gui_mode != (GUI_MODE_SHOP) 
							&& !identifygui_active 
							&& !removecursegui_active
							&& !GenericGUI.isGUIOpen()) )
					{
						//Use the item if right clicked.
						mousestatus[SDL_BUTTON_RIGHT] = 0;
						*inputPressed(joyimpulses[INJOY_MENU_USE]) = 0;
						bool badpotion = false;
						bool learnedSpell = false;

						if ( itemCategory(item) == POTION && item->identified )
						{
							badpotion = isPotionBad(*item); //So that you wield empty potions be default.
						}
						if ( item->type == POTION_EMPTY )
						{
							badpotion = true;
						}
						if ( itemCategory(item) == SPELLBOOK && (item->identified || itemIsEquipped(item, clientnum)) )
						{
							// equipped spellbook will unequip on use.
							learnedSpell = (playerLearnedSpellbook(item) || itemIsEquipped(item, clientnum));
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
							if ( (itemCategory(item) == POTION || itemCategory(item) == SPELLBOOK || item->type == FOOD_CREAMPIE )
								&& (keystatus[SDL_SCANCODE_LALT] || keystatus[SDL_SCANCODE_RALT]) )
							{
								badpotion = true;
								learnedSpell = true;
							}

							if ( !learnedSpell && item->identified 
								&& itemCategory(item) == SPELLBOOK && players[clientnum] && players[clientnum]->entity )
							{
								learnedSpell = true; // let's always equip/unequip spellbooks from the hotbar?
								spell_t* currentSpell = getSpellFromID(getSpellIDFromSpellbook(item->type));
								if ( currentSpell )
								{
									int skillLVL = stats[clientnum]->PROFICIENCIES[PRO_MAGIC] + statGetINT(stats[clientnum], players[clientnum]->entity);
									if ( stats[clientnum]->PROFICIENCIES[PRO_MAGIC] >= 100 )
									{
										skillLVL = 100;
									}
									if ( skillLVL >= currentSpell->difficulty )
									{
										// can learn spell, try that instead.
										learnedSpell = false;
									}
								}
							}

							if ( itemCategory(item) == SPELLBOOK && stats[clientnum] && stats[clientnum]->type == GOBLIN )
							{
								learnedSpell = true; // goblinos can't learn spells but always equip books.
							}

							if ( !badpotion && !learnedSpell )
							{
								if ( !(isItemEquippableInShieldSlot(item) && cast_animation.active_spellbook) )
								{
									if ( !disableItemUsage )
									{
										if ( stats[clientnum] && stats[clientnum]->type == AUTOMATON
											&& (item->type == TOOL_METAL_SCRAP || item->type == TOOL_MAGIC_SCRAP) )
										{
											// consume item
											if ( multiplayer == CLIENT )
											{
												strcpy((char*)net_packet->data, "FODA");
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
											item_FoodAutomaton(item, clientnum);
										}
										else
										{
											useItem(item, clientnum);
										}
									}
									else
									{
										if ( client_classes[clientnum] == CLASS_SHAMAN && item->type == SPELL_ITEM )
										{
											messagePlayer(clientnum, language[3488]); // unable to use with current level.
										}
										else
										{
											messagePlayer(clientnum, language[3432]); // unable to use in current form message.
										}
									}
								}
							}
							else
							{
								if ( !disableItemUsage )
								{
									playerTryEquipItemAndUpdateServer(item);
								}
								else
								{
									if ( client_classes[clientnum] == CLASS_SHAMAN && item->type == SPELL_ITEM )
									{
										messagePlayer(clientnum, language[3488]); // unable to use with current level.
									}
									else
									{
										messagePlayer(clientnum, language[3432]); // unable to use in current form message.
									}
								}
							}
							used = true;
							if ( disableItemUsage )
							{
								used = false;
							}
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
					if ( selected_spell == spell 
						&& (selected_spell_last_appearance == item->appearance || selected_spell_last_appearance == -1 ) )
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

	bool drawHotBarTooltipOnCycle = false;
	if ( !intro && hotbarTooltipLastGameTick != 0 && (ticks - hotbarTooltipLastGameTick) < TICKS_PER_SECOND * 2 )
	{
		drawHotBarTooltipOnCycle = true;
	}

	if ( !shootmode || drawHotBarTooltipOnCycle )
	{
		pos.x = initial_position.x;
		//Go back through all of the hotbar slots and draw the tooltips.
		for ( num = 0; num < NUM_HOTBAR_SLOTS; ++num, pos.x += hotbar_img->w * uiscale_hotbar )
		{
			item = uidToItem(hotbar[num].item);
			if ( item )
			{
				bool drawTooltipOnSlot = !shootmode && mouseInBounds(pos.x, pos.x + hotbar_img->w * uiscale_hotbar, pos.y, pos.y + hotbar_img->h * uiscale_hotbar);
				if ( !drawTooltipOnSlot )
				{
					if ( drawHotBarTooltipOnCycle && current_hotbar == num )
					{
						drawTooltipOnSlot = true;
					}
				}
				else
				{
					if ( !shootmode )
					{
						// reset timer.
						hotbarTooltipLastGameTick = 0;
						drawHotBarTooltipOnCycle = false;
					}
					else
					{
						if ( drawHotBarTooltipOnCycle )
						{
							drawTooltipOnSlot = false;
						}
					}
				}

				if ( drawTooltipOnSlot )
				{
					//Tooltip
					SDL_Rect src;
					src.x = mousex + 16;
					src.y = mousey + 8;

					if ( drawHotBarTooltipOnCycle )
					{
						src.x = pos.x + hotbar_img->w * uiscale_hotbar;
						src.y = pos.y + hotbar_img->h * uiscale_hotbar;
						src.y -= 16;
					}

					if ( itemCategory(item) == SPELL_CAT )
					{
						spell_t* spell = getSpellFromItem(item);
						if ( drawHotBarTooltipOnCycle )
						{
							drawSpellTooltip(spell, item, &src);
						}
						else
						{
							drawSpellTooltip(spell, item, nullptr);
						}
					}
					else
					{
						src.w = std::max(13, longestline(item->description())) * TTF12_WIDTH + 8;
						src.h = TTF12_HEIGHT * 4 + 8;
						char spellEffectText[256] = "";
						if ( item->identified )
						{
							bool learnedSpellbook = false;
							if ( itemCategory(item) == SPELLBOOK )
							{
								learnedSpellbook = playerLearnedSpellbook(item);
								if ( !learnedSpellbook && stats[clientnum] && players[clientnum] && players[clientnum]->entity )
								{
									// spellbook tooltip shows if you have the magic requirement as well (for goblins)
									int skillLVL = stats[clientnum]->PROFICIENCIES[PRO_MAGIC] + statGetINT(stats[clientnum], players[clientnum]->entity);
									spell_t* spell = getSpellFromID(getSpellIDFromSpellbook(item->type));
									if ( spell && skillLVL >= spell->difficulty )
									{
										learnedSpellbook = true;
									}
								}
							}

							if ( itemCategory(item) == WEAPON || itemCategory(item) == ARMOR || itemCategory(item) == THROWN
								|| itemTypeIsQuiver(item->type) )
							{
								src.h += TTF12_HEIGHT;
							}
							else if ( itemCategory(item) == SCROLL && item->identified )
							{
								src.h += TTF12_HEIGHT;
								src.w = std::max((2 + longestline(language[3862]) + longestline(item->getScrollLabel())) * TTF12_WIDTH + 8, src.w);
							}
							else if ( itemCategory(item) == SPELLBOOK && learnedSpellbook )
							{
								int height = 1;
								char effectType[32] = "";
								int spellID = getSpellIDFromSpellbook(item->type);
								int damage = drawSpellTooltip(getSpellFromID(spellID), item, nullptr);
								real_t dummy = 0.f;
								getSpellEffectString(spellID, spellEffectText, effectType, damage, &height, &dummy);
								int width = longestline(spellEffectText) * TTF12_WIDTH + 8;
								if ( width > src.w )
								{
									src.w = width;
								}
								src.h += height * TTF12_HEIGHT;
							}
							else if ( item->type == TOOL_GYROBOT || item->type == TOOL_DUMMYBOT
								|| item->type == TOOL_SENTRYBOT || item->type == TOOL_SPELLBOT
								|| (item->type == ENCHANTED_FEATHER && item->identified) )
							{
								src.w += 7 * TTF12_WIDTH;
							}
						}

						int furthestX = xres;
						if ( proficienciesPage == 0 )
						{
							if ( src.y < interfaceSkillsSheet.y + interfaceSkillsSheet.h )
							{
								furthestX = xres - interfaceSkillsSheet.w;
							}
						}
						else
						{
							if ( src.y < interfacePartySheet.y + interfacePartySheet.h )
							{
								furthestX = xres - interfacePartySheet.w;
							}
						}
						if ( src.x + src.w + 16 > furthestX ) // overflow right side of screen
						{
							src.x -= (src.w + 32);
						}
						if ( src.y + src.h + 16 > yres ) // overflow bottom of screen
						{
							src.y -= (src.y + src.h + 16 - yres);
						}

						if ( drawHotBarTooltipOnCycle )
						{
							drawTooltip(&src);
						}
						else
						{
							drawTooltip(&src);
						}

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

						if ( item->type == TOOL_GYROBOT || item->type == TOOL_DUMMYBOT
							|| item->type == TOOL_SENTRYBOT || item->type == TOOL_SPELLBOT )
						{
							int health = 100;
							if ( !item->tinkeringBotIsMaxHealth() )
							{
								health = 25 * (item->appearance % 10);
								if ( health == 0 && item->status != BROKEN )
								{
									health = 5;
								}
							}
							ttfPrintTextFormattedColor(ttf12, src.x + 4, src.y + 4, color, "%s (%d%%)", item->description(), health);
						}
						else if ( item->type == ENCHANTED_FEATHER && item->identified )
						{
							ttfPrintTextFormattedColor(ttf12, src.x + 4, src.y + 4, color, "%s (%d%%)", item->description(), item->appearance % ENCHANTED_FEATHER_MAX_DURABILITY);
						}
						else
						{
							ttfPrintTextFormattedColor(ttf12, src.x + 4, src.y + 4, color, "%s", item->description());
						}
						int itemWeight = items[item->type].weight * item->count;
						if ( itemTypeIsQuiver(item->type) )
						{
							itemWeight = std::max(1, itemWeight / 5);
						}
						ttfPrintTextFormatted(ttf12, src.x + 4 + TTF12_WIDTH, src.y + 4 + TTF12_HEIGHT * 2, language[313], itemWeight);
						ttfPrintTextFormatted(ttf12, src.x + 4 + TTF12_WIDTH, src.y + 4 + TTF12_HEIGHT * 3, language[314], item->sellValue(clientnum));
						if ( strcmp(spellEffectText, "") )
						{
							ttfPrintTextFormattedColor(ttf12, src.x + 4, src.y + 4 + TTF12_HEIGHT * 4, SDL_MapRGB(mainsurface->format, 0, 255, 255), spellEffectText);
						}

						if ( item->identified )
						{
							if ( itemCategory(item) == WEAPON || itemCategory(item) == THROWN
								|| itemTypeIsQuiver(item->type) )
							{
								Monster tmpRace = stats[clientnum]->type;
								if ( stats[clientnum]->type == TROLL
									|| stats[clientnum]->type == RAT
									|| stats[clientnum]->type == SPIDER
									|| stats[clientnum]->type == CREATURE_IMP )
								{
									// these monsters have 0 bonus from weapons, but want the tooltip to say the normal amount.
									stats[clientnum]->type = HUMAN;
								}

								if ( item->weaponGetAttack(stats[clientnum]) >= 0 )
								{
									color = SDL_MapRGB(mainsurface->format, 0, 255, 255);
								}
								else
								{
									color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
								}
								if ( stats[clientnum]->type != tmpRace )
								{
									color = SDL_MapRGB(mainsurface->format, 127, 127, 127); // grey out the text if monster doesn't benefit.
								}

								ttfPrintTextFormattedColor(ttf12, src.x + 4 + TTF12_WIDTH, src.y + 4 + TTF12_HEIGHT * 4, color, language[315], item->weaponGetAttack(stats[clientnum]));
								stats[clientnum]->type = tmpRace;
							}
							else if ( itemCategory(item) == ARMOR )
							{
								Monster tmpRace = stats[clientnum]->type;
								if ( stats[clientnum]->type == TROLL
									|| stats[clientnum]->type == RAT
									|| stats[clientnum]->type == SPIDER
									|| stats[clientnum]->type == CREATURE_IMP )
								{
									// these monsters have 0 bonus from armor, but want the tooltip to say the normal amount.
									stats[clientnum]->type = HUMAN;
								}

								if ( item->armorGetAC(stats[clientnum]) >= 0 )
								{
									color = SDL_MapRGB(mainsurface->format, 0, 255, 255);
								}
								else
								{
									color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
								}
								if ( stats[clientnum]->type != tmpRace )
								{
									color = SDL_MapRGB(mainsurface->format, 127, 127, 127); // grey out the text if monster doesn't benefit.
								}

								ttfPrintTextFormattedColor(ttf12, src.x + 4 + TTF12_WIDTH, src.y + 4 + TTF12_HEIGHT * 4, color, language[316], item->armorGetAC(stats[clientnum]));
								stats[clientnum]->type = tmpRace;
							}
							else if ( itemCategory(item) == SCROLL )
							{
								color = SDL_MapRGB(mainsurface->format, 0, 255, 255);
								ttfPrintTextFormattedColor(ttf12, src.x + 4 + TTF12_WIDTH, src.y + 4 + TTF12_HEIGHT * 4, color, "%s%s", language[3862], item->getScrollLabel());
							}
						}
					}
					if ( !drawHotBarTooltipOnCycle && hotbar_numkey_quick_add )
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
		if ( !FollowerMenu.selectMoveTo && mouseInBounds(xres - map.width * minimapTotalScale, xres, yres - map.height * minimapTotalScale, yres) ) // mouse within minimap pixels (each map tile is 4 pixels)
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
		if ( (*inputPressed(joyimpulses[INJOY_GAME_HOTBAR_NEXT]) || *inputPressed(impulses[IN_HOTBAR_SCROLL_RIGHT])) )
		{
			if ( shootmode && !itemMenuOpen && !openedChest[clientnum] 
				&& gui_mode != (GUI_MODE_SHOP) && !book_open 
				&& !identifygui_active && !removecursegui_active
				&& !GenericGUI.isGUIOpen() )
			{
				if ( *inputPressed(impulses[IN_HOTBAR_SCROLL_RIGHT]) )
				{
					*inputPressed(impulses[IN_HOTBAR_SCROLL_RIGHT]) = 0;
					hotbarTooltipLastGameTick = ticks;
				}
				else
				{
					*inputPressed(joyimpulses[INJOY_GAME_HOTBAR_NEXT]) = 0;
					bumper_moved = true;
				}
				selectHotbarSlot(current_hotbar + 1);
			}
			else
			{
				hotbarTooltipLastGameTick = 0;
				/*if ( intro || shootmode )
				{
					if ( *inputPressed(impulses[IN_HOTBAR_SCROLL_RIGHT]) )
					{
						*inputPressed(impulses[IN_HOTBAR_SCROLL_RIGHT]) = 0;
					}
				}*/
			}
		}
		if ( *inputPressed(joyimpulses[INJOY_GAME_HOTBAR_PREV]) || *inputPressed(impulses[IN_HOTBAR_SCROLL_LEFT]) )
		{
			if ( shootmode && !itemMenuOpen && !openedChest[clientnum] 
				&& gui_mode != (GUI_MODE_SHOP) && !book_open 
				&& !identifygui_active && !removecursegui_active
				&& !GenericGUI.isGUIOpen() )
			{
				if ( *inputPressed(impulses[IN_HOTBAR_SCROLL_LEFT]) )
				{
					*inputPressed(impulses[IN_HOTBAR_SCROLL_LEFT]) = 0;
					hotbarTooltipLastGameTick = ticks;
				}
				else
				{
					*inputPressed(joyimpulses[INJOY_GAME_HOTBAR_PREV]) = 0;
					bumper_moved = true;
				}
				selectHotbarSlot(current_hotbar - 1);
			}
			else
			{
				hotbarTooltipLastGameTick = 0;
				/*if ( intro || shootmode )
				{
					if ( *inputPressed(impulses[IN_HOTBAR_SCROLL_LEFT]) )
					{
						*inputPressed(impulses[IN_HOTBAR_SCROLL_LEFT]) = 0;
					}
				}*/
			}
		}

		if ( bumper_moved && !itemMenuOpen 
			&& !openedChest[clientnum] && gui_mode != (GUI_MODE_SHOP) 
			&& !book_open && !identifygui_active 
			&& !removecursegui_active && !GenericGUI.isGUIOpen() )
		{
			warpMouseToSelectedHotbarSlot();
		}

		if ( !itemMenuOpen && !selectedItem && !openedChest[clientnum] && gui_mode != (GUI_MODE_SHOP) )
		{
			if ( shootmode && (*inputPressed(joyimpulses[INJOY_GAME_HOTBAR_ACTIVATE]) || *inputPressed(impulses[IN_HOTBAR_SCROLL_SELECT]))
				&& !openedChest[clientnum] && gui_mode != (GUI_MODE_SHOP) 
				&& !book_open && !identifygui_active 
				&& !removecursegui_active && !GenericGUI.isGUIOpen() )
			{
				//Activate a hotbar slot if in-game.
				if ( *inputPressed(impulses[IN_HOTBAR_SCROLL_SELECT]) )
				{
					hotbarTooltipLastGameTick = std::max(ticks - TICKS_PER_SECOND, ticks - hotbarTooltipLastGameTick);
					*inputPressed(impulses[IN_HOTBAR_SCROLL_SELECT]) = 0;
				}
				else
				{
					*inputPressed(joyimpulses[INJOY_GAME_HOTBAR_ACTIVATE]) = 0;
				}
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
			bool learnedSpell = false;
			if ( itemCategory(item) == POTION && item->identified )
			{
				badpotion = isPotionBad(*item);
			}
			if ( item->type == POTION_EMPTY )
			{
				badpotion = true; //So that you wield empty potions be default.
			}
			if ( itemCategory(item) == SPELLBOOK && (item->identified || itemIsEquipped(item, clientnum)) )
			{
				// equipped spellbook will unequip on use.
				learnedSpell = (playerLearnedSpellbook(item) || itemIsEquipped(item, clientnum));
			}

			if ( (keystatus[SDL_SCANCODE_LALT] || keystatus[SDL_SCANCODE_RALT]) 
				&& (itemCategory(item) == POTION || itemCategory(item) == SPELLBOOK || item->type == FOOD_CREAMPIE) )
			{
				badpotion = true;
				learnedSpell = true;
			}

			if ( !learnedSpell && item->identified
				&& itemCategory(item) == SPELLBOOK && players[clientnum] && players[clientnum]->entity )
			{
				learnedSpell = true; // let's always equip/unequip spellbooks from the hotbar?
				spell_t* currentSpell = getSpellFromID(getSpellIDFromSpellbook(item->type));
				if ( currentSpell )
				{
					int skillLVL = stats[clientnum]->PROFICIENCIES[PRO_MAGIC] + statGetINT(stats[clientnum], players[clientnum]->entity);
					if ( stats[clientnum]->PROFICIENCIES[PRO_MAGIC] >= 100 )
					{
						skillLVL = 100;
					}
					if ( skillLVL >= currentSpell->difficulty )
					{
						// can learn spell, try that instead.
						learnedSpell = false;
					}
				}
			}

			if ( itemCategory(item) == SPELLBOOK && stats[clientnum] )
			{
				if ( stats[clientnum]->type == GOBLIN || stats[clientnum]->type == CREATURE_IMP )
				{
					learnedSpell = true; // goblinos can't learn spells but always equip books.
				}
			}

			bool disableItemUsage = false;
			if ( players[clientnum] && players[clientnum]->entity )
			{
				if ( players[clientnum]->entity->effectShapeshift != NOTHING )
				{
					if ( !item->usableWhileShapeshifted(stats[clientnum]) )
					{
						disableItemUsage = true;
					}
				}
				else
				{
					if ( itemCategory(item) == SPELL_CAT && item->appearance >= 1000 )
					{
						if ( canUseShapeshiftSpellInCurrentForm(*item) != 1 )
						{
							disableItemUsage = true;
						}
					}
				}
			}
			if ( client_classes[clientnum] == CLASS_SHAMAN )
			{
				if ( item->type == SPELL_ITEM && !(playerUnlockedShamanSpell(clientnum, item)) )
				{
					disableItemUsage = true;
				}
			}

			if ( !disableItemUsage )
			{
				if ( !badpotion && !learnedSpell )
				{
					if ( !(isItemEquippableInShieldSlot(item) && cast_animation.active_spellbook) )
					{
						if ( stats[clientnum] && stats[clientnum]->type == AUTOMATON
							&& (item->type == TOOL_METAL_SCRAP || item->type == TOOL_MAGIC_SCRAP) )
						{
							// consume item
							if ( multiplayer == CLIENT )
							{
								strcpy((char*)net_packet->data, "FODA");
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
							item_FoodAutomaton(item, clientnum);
						}
						else
						{
							useItem(item, clientnum);
						}
					}
				}
				else
				{
					playerTryEquipItemAndUpdateServer(item);
				}
			}
			else
			{
				if ( client_classes[clientnum] == CLASS_SHAMAN && item->type == SPELL_ITEM )
				{
					messagePlayer(clientnum, language[3488]); // unable to use with current level.
				}
				else
				{
					messagePlayer(clientnum, language[3432]); // unable to use in current form message.
				}
			}
		}
	}

	FollowerMenu.drawFollowerMenu();

	// stat increase icons
	pos.w = 64;
	pos.h = 64;
	pos.x = xres - pos.w * 3 - 9;
	pos.y = (NUMPROFICIENCIES * TTF12_HEIGHT) + (TTF12_HEIGHT * 3) + (32 + 64 + 64 + 3); // 131px from end of prof window.

	if ( (!shootmode || lock_right_sidebar) && proficienciesPage == 1 
		&& pos.y < (interfacePartySheet.y + interfacePartySheet.h + 16) )
	{
		pos.y = interfacePartySheet.y + interfacePartySheet.h + 16;
	}

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

int drawSpellTooltip(spell_t* spell, Item* item, SDL_Rect* src)
{
	SDL_Rect pos;
	if ( src )
	{
		pos.x = src->x;
		pos.y = src->y;
	}
	else
	{
		pos.x = mousex + 16;
		pos.y = mousey + 8;
	}
	bool spellbook = false;
	if ( item && itemCategory(item) == SPELLBOOK )
	{
		spellbook = true;
	}
	if ( spell )
	{
		node_t* rootNode = spell->elements.first;
		spellElement_t* elementRoot = nullptr;
		if ( rootNode )
		{
			elementRoot = (spellElement_t*)(rootNode->element);
		}
		int damage = 0;
		int mana = 0;
		int heal = 0;
		spellElement_t* primaryElement = nullptr;
		if ( elementRoot )
		{
			node_t* primaryNode = elementRoot->elements.first;
			mana = elementRoot->mana;
			heal = mana;
			if ( primaryNode )
			{
				primaryElement = (spellElement_t*)(primaryNode->element);
				if ( primaryElement )
				{
					damage = primaryElement->damage;
				}
			}
			if ( players[clientnum] )
			{
				int bonus = 0;
				if ( spellbook )
				{
					bonus = 25 * ((shouldInvertEquipmentBeatitude(stats[clientnum]) ? abs(item->beatitude) : item->beatitude));
					if ( stats[clientnum] )
					{
						if ( players[clientnum] && players[clientnum]->entity )
						{
							bonus += players[clientnum]->entity->getINT() * 0.5;
						}
						else
						{
							bonus += statGetINT(stats[clientnum], nullptr) * 0.5;
						}
					}
					if ( bonus < 0 )
					{
						bonus = 0;
					}
				}
				damage += (damage * (bonus * 0.01 + getBonusFromCasterOfSpellElement(players[clientnum]->entity, primaryElement)));
				heal += (heal * (bonus * 0.01 + getBonusFromCasterOfSpellElement(players[clientnum]->entity, primaryElement)));
			}
			if ( spell->ID == SPELL_HEALING || spell->ID == SPELL_EXTRAHEALING )
			{
				damage = heal;
			}
		}
		int spellInfoLines = 1;
		char spellType[32] = "";
		char spellEffectText[256] = "";
		real_t sustainCostPerSecond = 0.f;
		getSpellEffectString(spell->ID, spellEffectText, spellType, damage, &spellInfoLines, &sustainCostPerSecond);
		char tempstr[64] = "";
		char spellNameString[128] = "";
		if ( item && item->appearance >= 1000 )
		{
			// shapeshift spells, append the form name here.
			switch ( spell->ID )
			{
				case SPELL_SPEED:
				case SPELL_DETECT_FOOD:
					snprintf(spellNameString, 127, "%s (%s)", spell->name, language[3408]);
					break;
				case SPELL_POISON:
				case SPELL_SPRAY_WEB:
					snprintf(spellNameString, 127, "%s (%s)", spell->name, language[3409]);
					break;
				case SPELL_STRIKE:
				case SPELL_FEAR:
				case SPELL_TROLLS_BLOOD:
					snprintf(spellNameString, 127, "%s (%s)", spell->name, language[3410]);
					break;
				case SPELL_LIGHTNING:
				case SPELL_CONFUSE:
				case SPELL_AMPLIFY_MAGIC:
					snprintf(spellNameString, 127, "%s (%s)", spell->name, language[3411]);
					break;
				default:
					strncpy(spellNameString, spell->name, 127);
					break;
			}
		}
		else
		{
			strncpy(spellNameString, spell->name, 127);
		}

		if ( spell->ID == SPELL_DOMINATE )
		{
			snprintf(tempstr, 63, language[2977], getCostOfSpell(spell));
		}
		else if ( spell->ID == SPELL_DEMON_ILLUSION )
		{
			snprintf(tempstr, 63, language[3853], getCostOfSpell(spell));
		}
		else
		{
			if ( players[clientnum] && players[clientnum]->entity )
			{
				if ( sustainCostPerSecond > 0.01 )
				{
					snprintf(tempstr, 31, language[3325],
						getCostOfSpell(spell, players[clientnum]->entity), sustainCostPerSecond);
				}
				else
				{
					snprintf(tempstr, 31, language[308], getCostOfSpell(spell, players[clientnum]->entity));
				}
			}
			else
			{
				if ( sustainCostPerSecond > 0.01 )
				{
					snprintf(tempstr, 31, language[3325],
						getCostOfSpell(spell), sustainCostPerSecond);
				}
				else
				{
					snprintf(tempstr, 31, language[308], getCostOfSpell(spell));
				}
			}
		}
		if ( strcmp(spellEffectText, "") )
		{
			pos.w = (longestline(spellEffectText) + 1) * TTF12_WIDTH + 8;
		}
		else
		{
			pos.w = std::max(longestline(spellNameString), longestline(tempstr)) * TTF12_WIDTH + 8;
		}

		int furthestX = xres;
		if ( proficienciesPage == 0 )
		{
			if ( pos.y < interfaceSkillsSheet.y + interfaceSkillsSheet.h )
			{
				furthestX = xres - interfaceSkillsSheet.w;
			}
		}
		else
		{
			if ( pos.y < interfacePartySheet.y + interfacePartySheet.h )
			{
				furthestX = xres - interfacePartySheet.w;
			}
		}

		if ( pos.x + pos.w + 16 > furthestX ) // overflow right side of screen
		{
			pos.x -= (pos.w + 32);
		}
		pos.h = TTF12_HEIGHT * (2 + spellInfoLines + 1) + 8;
		if ( spellInfoLines >= 4 )
		{
			pos.h += 4;
		}
		else if ( spellInfoLines == 3 )
		{
			pos.h += 2;
		}
		if ( pos.y + pos.h + 16 > yres ) // overflow bottom of screen
		{
			pos.y -= (pos.y + pos.h + 16 - yres);
		}
		if ( spellbook )
		{
			return damage;
		}

		drawTooltip(&pos);
		ttfPrintTextFormatted(ttf12, pos.x + 4, pos.y + 4, "%s\n%s\n%s",
			spellNameString, tempstr, spellType);
		Uint32 effectColor = uint32ColorLightBlue(*mainsurface);
		ttfPrintTextFormattedColor(ttf12, pos.x + 4, pos.y + 4, effectColor,
			"\n\n\n%s", spellEffectText);
	}
	else
	{
		pos.w = longestline("Error: Spell doesn't exist!") * TTF12_WIDTH + 8;
		pos.h = TTF12_HEIGHT + 8;
		drawTooltip(&pos);
		ttfPrintTextFormatted(ttf12, pos.x + 4, pos.y + 4, "%s", "Error: Spell doesn't exist!");
	}
	return 0;
}

void getSpellEffectString(int spellID, char effectTextBuffer[256], char spellType[32], int value, int* spellInfoLines, real_t* sustainCostPerSecond)
{
	if ( !spellInfoLines || !sustainCostPerSecond )
	{
		return;
	}
	switch ( spellID )
	{
		case SPELL_FORCEBOLT:
		case SPELL_MAGICMISSILE:
		case SPELL_LIGHTNING:
			snprintf(effectTextBuffer, 255, language[3289], value);
			snprintf(spellType, 31, language[3303]);
			break;
		case SPELL_COLD:
			snprintf(effectTextBuffer, 255, language[3290], value, language[3294]);
			snprintf(spellType, 31, language[3303]);
			*spellInfoLines = 2;
			break;
		case SPELL_POISON:
			snprintf(effectTextBuffer, 255, language[3290], value, language[3300]);
			snprintf(spellType, 31, language[3303]);
			*spellInfoLines = 2;
			break;
		case SPELL_FIREBALL:
			snprintf(effectTextBuffer, 255, language[3290], value, language[3295]);
			snprintf(spellType, 31, language[3303]);
			*spellInfoLines = 2;
			break;
		case SPELL_BLEED:
			snprintf(effectTextBuffer, 255, language[3291], value, language[3297], language[3294]);
			*spellInfoLines = 2;
			snprintf(spellType, 31, language[3303]);
			break;
		case SPELL_SLOW:
			snprintf(effectTextBuffer, 255, language[3292], language[3294]);
			snprintf(spellType, 31, language[3303]);
			break;
		case SPELL_SLEEP:
			snprintf(effectTextBuffer, 255, language[3292], language[3298]);
			snprintf(spellType, 31, language[3303]);
			break;
		case SPELL_CONFUSE:
			snprintf(effectTextBuffer, 255, language[3292], language[3299]);
			snprintf(spellType, 31, language[3303]);
			break;
		case SPELL_ACID_SPRAY:
			snprintf(effectTextBuffer, 255, language[3293], value, language[3300]);
			snprintf(spellType, 31, language[3304]);
			*spellInfoLines = 2;
			break;
		case SPELL_HEALING:
		case SPELL_EXTRAHEALING:
		{
			snprintf(spellType, 31, language[3301]);
			snprintf(effectTextBuffer, 255, language[3307], value);
			*spellInfoLines = 2;
			break;
		}
		case SPELL_REFLECT_MAGIC:
			snprintf(spellType, 31, language[3302]);
			snprintf(effectTextBuffer, 255, language[3308]);
			*spellInfoLines = 2;
			*sustainCostPerSecond = 6.f;
			break;
		case SPELL_LEVITATION:
			snprintf(spellType, 31, language[3302]);
			snprintf(effectTextBuffer, 255, language[3309]);
			*sustainCostPerSecond = 0.6;
			break;
		case SPELL_INVISIBILITY:
			snprintf(spellType, 31, language[3302]);
			snprintf(effectTextBuffer, 255, language[3310]);
			*sustainCostPerSecond = 1.f;
			break;
		case SPELL_LIGHT:
			snprintf(spellType, 31, language[3302]);
			snprintf(effectTextBuffer, 255, language[3311]);
			*sustainCostPerSecond = 15.f;
			break;
		case SPELL_REMOVECURSE:
			snprintf(spellType, 31, language[3305]);
			snprintf(effectTextBuffer, 255, language[3312]);
			break;
		case SPELL_IDENTIFY:
			snprintf(spellType, 31, language[3305]);
			snprintf(effectTextBuffer, 255, language[3313]);
			break;
		case SPELL_MAGICMAPPING:
			snprintf(spellType, 31, language[3305]);
			snprintf(effectTextBuffer, 255, language[3314]);
			*spellInfoLines = 2;
			break;
		case SPELL_TELEPORTATION:
			snprintf(spellType, 31, language[3305]);
			snprintf(effectTextBuffer, 255, language[3315]);
			break;
		case SPELL_OPENING:
			snprintf(spellType, 31, language[3303]);
			snprintf(effectTextBuffer, 255, language[3316]);
			break;
		case SPELL_LOCKING:
			snprintf(spellType, 31, language[3303]);
			snprintf(effectTextBuffer, 255, language[3317]);
			break;
		case SPELL_CUREAILMENT:
			snprintf(spellType, 31, language[3301]);
			snprintf(effectTextBuffer, 255, language[3318]);
			*spellInfoLines = 2;
			break;
		case SPELL_DIG:
			snprintf(spellType, 31, language[3303]);
			snprintf(effectTextBuffer, 255, language[3319]);
			break;
		case SPELL_SUMMON:
			snprintf(spellType, 31, language[3306]);
			snprintf(effectTextBuffer, 255, language[3320]);
			*spellInfoLines = 3;
			break;
		case SPELL_STONEBLOOD:
			snprintf(spellType, 31, language[3304]);
			snprintf(effectTextBuffer, 255, language[3292], language[3296]);
			break;
		case SPELL_DOMINATE:
			snprintf(spellType, 31, language[3303]);
			snprintf(effectTextBuffer, 255, language[3321]);
			*spellInfoLines = 4;
			break;
		case SPELL_STEAL_WEAPON:
			snprintf(spellType, 31, language[3303]);
			snprintf(effectTextBuffer, 255, language[3322]);
			*spellInfoLines = 2;
			break;
		case SPELL_DRAIN_SOUL:
			snprintf(spellType, 31, language[3303]);
			snprintf(effectTextBuffer, 255, language[3323], value);
			*spellInfoLines = 2;
			break;
		case SPELL_VAMPIRIC_AURA:
			snprintf(spellType, 31, language[3302]);
			snprintf(effectTextBuffer, 255, language[3324]);
			*spellInfoLines = 4;
			*sustainCostPerSecond = 0.33;
			break;
		case SPELL_CHARM_MONSTER:
			snprintf(spellType, 31, language[3303]);
			snprintf(effectTextBuffer, 255, language[3326]);
			*spellInfoLines = 3;
			break;
		case SPELL_REVERT_FORM:
			snprintf(spellType, 31, language[3305]);
			snprintf(effectTextBuffer, 255, language[3851]);
			*spellInfoLines = 1;
			break;
		case SPELL_RAT_FORM:
			snprintf(spellType, 31, language[3305]);
			snprintf(effectTextBuffer, 255, language[3847]);
			*spellInfoLines = 2;
			break;
		case SPELL_SPIDER_FORM:
			snprintf(spellType, 31, language[3305]);
			snprintf(effectTextBuffer, 255, language[3848]);
			*spellInfoLines = 2;
			break;
		case SPELL_TROLL_FORM:
			snprintf(spellType, 31, language[3305]);
			snprintf(effectTextBuffer, 255, language[3849]);
			*spellInfoLines = 2;
			break;
		case SPELL_IMP_FORM:
			snprintf(spellType, 31, language[3305]);
			snprintf(effectTextBuffer, 255, language[3850]);
			*spellInfoLines = 2;
			break;
		case SPELL_SPRAY_WEB:
			snprintf(spellType, 31, language[3304]);
			snprintf(effectTextBuffer, 255, language[3834]);
			*spellInfoLines = 4;
			break;
		case SPELL_SPEED:
			snprintf(spellType, 31, language[3301]);
			snprintf(effectTextBuffer, 255, language[3835]);
			*spellInfoLines = 2;
			break;
		case SPELL_FEAR:
			snprintf(spellType, 31, language[3301]);
			snprintf(effectTextBuffer, 255, language[3836]);
			*spellInfoLines = 3;
			break;
		case SPELL_STRIKE:
			snprintf(spellType, 31, language[3305]);
			snprintf(effectTextBuffer, 255, language[3838]);
			*spellInfoLines = 4;
			break;
		case SPELL_DETECT_FOOD:
			snprintf(spellType, 31, language[3305]);
			snprintf(effectTextBuffer, 255, language[3839]);
			*spellInfoLines = 1;
			break;
		case SPELL_WEAKNESS:
			snprintf(spellType, 31, language[3303]);
			snprintf(effectTextBuffer, 255, language[3837]);
			*spellInfoLines = 2;
			break;
		case SPELL_AMPLIFY_MAGIC:
			snprintf(spellType, 31, language[3302]);
			snprintf(effectTextBuffer, 255, language[3852]);
			*spellInfoLines = 2;
			*sustainCostPerSecond = 0.25;
			break;
		case SPELL_SHADOW_TAG:
			snprintf(spellType, 31, language[3303]);
			snprintf(effectTextBuffer, 255, language[3843]);
			*spellInfoLines = 3;
			break;
		case SPELL_TELEPULL:
			snprintf(spellType, 31, language[3303]);
			snprintf(effectTextBuffer, 255, language[3844]);
			*spellInfoLines = 3;
			break;
		case SPELL_DEMON_ILLUSION:
			snprintf(spellType, 31, language[3303]);
			snprintf(effectTextBuffer, 255, language[3845]);
			*spellInfoLines = 3;
			break;
		case SPELL_TROLLS_BLOOD:
			snprintf(spellType, 31, language[3305]);
			snprintf(effectTextBuffer, 255, language[3840]);
			*spellInfoLines = 2;
			break;
		case SPELL_SALVAGE:
			snprintf(spellType, 31, language[3301]);
			snprintf(effectTextBuffer, 255, language[3846]);
			*spellInfoLines = 2;
			break;
		case SPELL_FLUTTER:
			snprintf(spellType, 31, language[3305]);
			snprintf(effectTextBuffer, 255, language[3841]);
			*spellInfoLines = 1;
			break;
		case SPELL_DASH:
			snprintf(spellType, 31, language[3305]);
			snprintf(effectTextBuffer, 255, language[3842]);
			*spellInfoLines = 3;
			break;
		case SPELL_SELF_POLYMORPH:
			snprintf(spellType, 31, language[3305]);
			snprintf(effectTextBuffer, 255, language[3886]);
			*spellInfoLines = 2;
			break;
		case SPELL_9:
		case SPELL_10:
		default:
			break;
	}
}