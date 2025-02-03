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
#include "../engine/audio/sound.hpp"
#include "../net.hpp"
#include "../menu.hpp"
#include "../player.hpp"
#include "interface.hpp"
#include "../colors.hpp"
#include "../mod_tools.hpp"
#include "../ui/GameUI.hpp"
#include "../ui/Image.hpp"

/*-------------------------------------------------------------------------------

	updateEnemyBar

	updates the enemy hp bar for the given player

-------------------------------------------------------------------------------*/

void updateEnemyBar(Entity* source, Entity* target, const char* name, Sint32 hp, Sint32 maxhp, bool lowPriorityTick, 
	DamageGib gibType)
{
	// server/singleplayer only function.
	hp = std::max(0, hp); // bounds checking - furniture can go negative
	int player = -1;
	int c;

	if (!source || !target)
	{
		return;
	}

	for (c = 0; c < MAXPLAYERS; c++)
	{
		if (source == players[c]->entity || source == players[c]->ghost.my )
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
			player = source->monsterAllyIndex;
			if ( source->monsterAllyGetPlayerLeader() && source->monsterAllyGetPlayerLeader() == target )
			{
				player = -1; // don't update enemy bar if attacking leader.
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
		else if ( source->behavior == &actMonster && source->monsterAllyIndex >= 0/*monsterIsImmobileTurret(source, nullptr)*/
			&& (target->behavior == &actMonster || target->behavior == &actPlayer || target->behavior == &actDoor) )
		{
			player = source->monsterAllyIndex;
			if ( source->monsterAllyGetPlayerLeader() && source->monsterAllyGetPlayerLeader() == target )
			{
				player = -1; // don't update enemy bar if attacking leader.
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
		bool tookDamage = stats->HP != stats->OLDHP;
		if ( playertarget >= 0 && players[playertarget]->isLocalPlayer() )
		{
			DamageIndicatorHandler.insert(playertarget, source->x, source->y, tookDamage);
		}
		else if ( playertarget > 0 && multiplayer == SERVER && !players[playertarget]->isLocalPlayer() )
		{
			strcpy((char*)net_packet->data, "DAMI");
			SDLNet_Write32(source->x, &net_packet->data[4]);
			SDLNet_Write32(source->y, &net_packet->data[8]);
			net_packet->data[12] = tookDamage ? 1 : 0;
			net_packet->address.host = net_clients[playertarget - 1].host;
			net_packet->address.port = net_clients[playertarget - 1].port;
			net_packet->len = 13;
			sendPacketSafe(net_sock, -1, net_packet, playertarget - 1);
		}
	}

	int oldhp = 0;
	if ( stats )
	{
		oldhp = stats->OLDHP;
	}
	else
	{
		if ( target->behavior == &actDoor )
		{
			oldhp = target->doorOldHealth;
		}
		else if ( target->behavior == &actFurniture )
		{
			oldhp = target->furnitureOldHealth;
		}
		else if ( target->behavior == &actChest )
		{
			oldhp = target->chestOldHealth;
		}
		else if ( target->isDamageableCollider() )
		{
			oldhp = target->colliderOldHP;
		}
		else
		{
			oldhp = hp;
		}
	}

	if ( !EnemyHPDamageBarHandler::bDamageGibTypesEnabled )
	{
		gibType = DamageGib::DMG_DEFAULT;
	}

	EnemyHPDamageBarHandler::EnemyHPDetails* details = nullptr;
	if ( player >= 0 /*&& players[player]->isLocalPlayer()*/ )
	{
		// add enemy bar to the server
		int p = player;
		if ( !players[player]->isLocalPlayer() )
		{
			p = clientnum; // remote clients, add it to the local list.
		}
		if ( splitscreen ) // check if anyone else has this enemy bar on their screen
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( players[i]->isLocalPlayer() )
				{
					if ( enemyHPDamageBarHandler[i].HPBars.find(target->getUID()) != enemyHPDamageBarHandler[i].HPBars.end() )
					{
						p = i;
						break;
					}
				}
			}
		}
		if ( stats )
		{
			details = enemyHPDamageBarHandler[p].addEnemyToList(hp, maxhp, oldhp, target->getUID(), name, lowPriorityTick, gibType);
		}
		else
		{
			details = enemyHPDamageBarHandler[p].addEnemyToList(hp, maxhp, oldhp, target->getUID(), name, lowPriorityTick, gibType);
		}
	}
	
	if ( player >= 0 && multiplayer == SERVER )
	{
		// send to all remote players
		for ( int p = 1; p < MAXPLAYERS; ++p )
		{
			if ( !players[p]->isLocalPlayer() )
			{
				if ( p == playertarget )
				{
					continue;
				}
				strcpy((char*)net_packet->data, "ENHP");
				SDLNet_Write16(static_cast<Sint16>(hp), &net_packet->data[4]);
				SDLNet_Write16(static_cast<Sint16>(maxhp), &net_packet->data[6]);
				if ( stats )
				{
					SDLNet_Write16(static_cast<Sint16>(oldhp), &net_packet->data[8]);
				}
				else
				{
					SDLNet_Write16(static_cast<Sint16>(oldhp), &net_packet->data[8]);
				}
				SDLNet_Write32(target->getUID(), &net_packet->data[10]);
				net_packet->data[14] = lowPriorityTick ? 1 : 0; // 1 == true
				if ( EnemyHPDamageBarHandler::bDamageGibTypesEnabled )
				{
					net_packet->data[14] |= (gibType << 1) & 0xFE;
				}
				if ( stats && details )
				{
					SDLNet_Write32(details->enemy_statusEffects1, &net_packet->data[15]);
					SDLNet_Write32(details->enemy_statusEffects2, &net_packet->data[19]);
					SDLNet_Write32(details->enemy_statusEffectsLowDuration1, &net_packet->data[23]);
					SDLNet_Write32(details->enemy_statusEffectsLowDuration2, &net_packet->data[27]);
				}
				else
				{
					SDLNet_Write32(0, &net_packet->data[15]);
					SDLNet_Write32(0, &net_packet->data[19]);
					SDLNet_Write32(0, &net_packet->data[23]);
					SDLNet_Write32(0, &net_packet->data[27]);
				}
				strcpy((char*)(&net_packet->data[31]), name);
				net_packet->data[31 + strlen(name)] = 0;
				net_packet->address.host = net_clients[p - 1].host;
				net_packet->address.port = net_clients[p - 1].port;
				net_packet->len = 31 + strlen(name) + 1;
				sendPacketSafe(net_sock, -1, net_packet, p - 1);

			}
		}
	}
}

/*-------------------------------------------------------------------------------

	drawStatus

	Draws various status bar elements, such as textbox, health, magic,
	and the hotbar

-------------------------------------------------------------------------------*/

bool mouseInBoundsRealtimeCoords(int, int, int, int, int); //Defined in playerinventory.cpp. Dirty hack, you should be ashamed of yourself.

bool warpMouseToSelectedHotbarSlot(const int player)
{
	if ( players[player]->shootmode == true)
	{
		return false;
	}

	if ( auto hotbarSlotFrame = players[player]->hotbar.getHotbarSlotFrame(players[player]->hotbar.current_hotbar) )
	{
		if ( !players[player]->hotbar.isInteractable )
		{
			players[player]->inventoryUI.cursor.queuedModule = Player::GUI_t::MODULE_HOTBAR;
			players[player]->inventoryUI.cursor.queuedFrameToWarpTo = hotbarSlotFrame;
			return false;
		}
		else
		{
			//messagePlayer(0, "[Debug]: select item warped");
			players[player]->inventoryUI.cursor.queuedModule = Player::GUI_t::MODULE_NONE;
			players[player]->inventoryUI.cursor.queuedFrameToWarpTo = nullptr;
			hotbarSlotFrame->warpMouseToFrame(player, (Inputs::SET_CONTROLLER));
		}
		return true;
	}

	return false;
}

void drawHPMPBars(int player)
{
	const int x1 = players[player]->camera_x1();
	const int x2 = players[player]->camera_x2();
	const int y1 = players[player]->camera_y1();
	const int y2 = players[player]->camera_y2();
	const int hpmpbarOffsets = 0;

	int playerStatusBarWidth = 38 * uiscale_playerbars;
	int playerStatusBarHeight = 156 * uiscale_playerbars;

	SDL_Rect pos;

	// PLAYER HEALTH BAR
	// Display Health bar border
	pos.x = x1 + 38 + 38 * uiscale_playerbars;
	pos.w = playerStatusBarWidth;
	pos.h = playerStatusBarHeight;
	pos.y = y2 - (playerStatusBarHeight + 12) - hpmpbarOffsets;
	drawTooltip(&pos);
	if ( stats[player] && stats[player]->HP > 0
		&& stats[player]->EFFECTS[EFF_HP_REGEN] )
	{
		bool lowDurationFlash = !((ticks % 50) - (ticks % 25));
		bool lowDuration = stats[player]->EFFECTS_TIMERS[EFF_HP_REGEN] > 0 &&
			(stats[player]->EFFECTS_TIMERS[EFF_HP_REGEN] < TICKS_PER_SECOND * 5);
		if ( (lowDuration && !lowDurationFlash) || !lowDuration )
		{
			if ( colorblind )
			{
				drawTooltip(&pos, makeColorRGB(0, 255, 255)); // blue
			}
			else
			{
				drawTooltip(&pos, makeColorRGB(0, 255, 0)); // green
			}
		}
	}

	// Display "HP" at top of Health bar
	ttfPrintText(ttf12, pos.x + (playerStatusBarWidth / 2 - 10), pos.y + 6, Language::get(306));

	// Display border between actual Health bar and "HP"
	//pos.x = 76;
	pos.w = playerStatusBarWidth;
	pos.h = 0;
	pos.y = y2 - (playerStatusBarHeight - 9) - hpmpbarOffsets;
	drawTooltip(&pos);
	if ( stats[player] && stats[player]->HP > 0
		&& stats[player]->EFFECTS[EFF_HP_REGEN] )
	{
		bool lowDurationFlash = !((ticks % 50) - (ticks % 25));
		bool lowDuration = stats[player]->EFFECTS_TIMERS[EFF_HP_REGEN] > 0 &&
			(stats[player]->EFFECTS_TIMERS[EFF_HP_REGEN] < TICKS_PER_SECOND * 5);
		if ( (lowDuration && !lowDurationFlash) || !lowDuration )
		{
			if ( colorblind )
			{
				drawTooltip(&pos, makeColorRGB(0, 255, 255)); // blue
			}
			else
			{
				drawTooltip(&pos, makeColorRGB(0, 255, 0)); // green
			}
		}
	}

	// Display the actual Health bar's faint background
	pos.x = x1 + 42 + 38 * uiscale_playerbars;
	pos.w = playerStatusBarWidth - 5;
	pos.h = playerStatusBarHeight - 27;
	pos.y = y2 - 15 - pos.h - hpmpbarOffsets;

	// Change the color depending on if you are poisoned
	Uint32 color = 0;
	if ( stats[player] && stats[player]->EFFECTS[EFF_POISONED] )
	{
		if ( colorblind )
		{
			color = makeColorRGB(0, 0, 48); // Display blue
		}
		else
		{
			color = makeColorRGB(0, 48, 0); // Display green
		}
	}
	else
	{
		color = makeColorRGB(48, 0, 0); // Display red
	}

	// Draw the actual Health bar's faint background with specified color
	drawRect(&pos, color, 255);

	// If the Player is alive, base the size of the actual Health bar off remaining HP
	if ( stats[player] && stats[player]->HP > 0 )
	{
		//pos.x = 80;
		pos.w = playerStatusBarWidth - 5;
		pos.h = (playerStatusBarHeight - 27) * (static_cast<double>(stats[player]->HP) / stats[player]->MAXHP);
		pos.y = y2 - 15 - pos.h - hpmpbarOffsets;

		if ( stats[player]->EFFECTS[EFF_POISONED] )
		{
			if ( !colorblind )
			{
				color = makeColorRGB(0, 128, 0);
			}
			else
			{
				color = makeColorRGB(0, 0, 128);
			}
		}
		else
		{
			color = makeColorRGB(128, 0, 0);
		}

		// Only draw the actual Health bar if the Player is alive
		drawRect(&pos, color, 255);
	}

	// Print out the amount of HP the Player currently has
	if ( stats[player] )
	{
		snprintf(tempstr, 4, "%d", stats[player]->HP);
	}
	else
	{
		snprintf(tempstr, 4, "%d", 0);
	}
	if ( uiscale_playerbars >= 1.5 )
	{
		pos.x += uiscale_playerbars * 2;
	}
	printTextFormatted(font12x12_bmp, pos.x + 16 * uiscale_playerbars - strlen(tempstr) * 6, y2 - (playerStatusBarHeight / 2 + 8) - hpmpbarOffsets, tempstr);
	int xoffset = pos.x;

	// PLAYER MAGIC BAR
	// Display the Magic bar border
	pos.x = x1 + 12 * uiscale_playerbars;
	pos.w = playerStatusBarWidth;
	pos.h = playerStatusBarHeight;
	pos.y = y2 - (playerStatusBarHeight + 12) - hpmpbarOffsets;
	drawTooltip(&pos);
	if ( stats[player] && stats[player]->HP > 0
		&& stats[player]->EFFECTS[EFF_MP_REGEN] )
	{
		bool lowDurationFlash = !((ticks % 50) - (ticks % 25));
		bool lowDuration = stats[player]->EFFECTS_TIMERS[EFF_MP_REGEN] > 0 &&
			(stats[player]->EFFECTS_TIMERS[EFF_MP_REGEN] < TICKS_PER_SECOND * 5);
		if ( (lowDuration && !lowDurationFlash) || !lowDuration )
		{
			if ( colorblind )
			{
				drawTooltip(&pos, makeColorRGB(0, 255, 255)); // blue
			}
			else
			{
				drawTooltip(&pos, makeColorRGB(0, 255, 0)); // green
			}
		}
	}
	Uint32 mpColorBG = makeColorRGB(0, 0, 48);
	Uint32 mpColorFG = makeColorRGB(0, 24, 128);
	if ( stats[player] && stats[player]->playerRace == RACE_INSECTOID && stats[player]->stat_appearance == 0 )
	{
		ttfPrintText(ttf12, pos.x + (playerStatusBarWidth / 2 - 10), pos.y + 6, Language::get(3768));
		mpColorBG = makeColorRGB(32, 48, 0);
		mpColorFG = makeColorRGB(92, 192, 0);
	}
	else if ( stats[player] && stats[player]->type == AUTOMATON )
	{
		ttfPrintText(ttf12, pos.x + (playerStatusBarWidth / 2 - 10), pos.y + 6, Language::get(3474));
		mpColorBG = makeColorRGB(64, 32, 0);
		mpColorFG = makeColorRGB(192, 92, 0);
	}
	else
	{
		// Display "MP" at the top of Magic bar
		ttfPrintText(ttf12, pos.x + (playerStatusBarWidth / 2 - 10), pos.y + 6, Language::get(307));
	}

	// Display border between actual Magic bar and "MP"
	//pos.x = 12;
	pos.w = playerStatusBarWidth;
	pos.h = 0;
	pos.y = y2 - (playerStatusBarHeight - 9) - hpmpbarOffsets;
	drawTooltip(&pos);
	if ( stats[player] && stats[player]->HP > 0
		&& stats[player]->EFFECTS[EFF_MP_REGEN] )
	{
		bool lowDurationFlash = !((ticks % 50) - (ticks % 25));
		bool lowDuration = stats[player]->EFFECTS_TIMERS[EFF_MP_REGEN] > 0 &&
			(stats[player]->EFFECTS_TIMERS[EFF_MP_REGEN] < TICKS_PER_SECOND * 5);
		if ( (lowDuration && !lowDurationFlash) || !lowDuration )
		{
			if ( colorblind )
			{
				drawTooltip(&pos, makeColorRGB(0, 255, 255)); // blue
			}
			else
			{
				drawTooltip(&pos, makeColorRGB(0, 255, 0)); // green
			}
		}
	}

	// Display the actual Magic bar's faint background
	pos.x = x1 + 4 + 12 * uiscale_playerbars;
	pos.w = playerStatusBarWidth - 5;
	pos.h = playerStatusBarHeight - 27;
	pos.y = y2 - 15 - pos.h - hpmpbarOffsets;

	// Draw the actual Magic bar's faint background
	drawRect(&pos, mpColorBG, 255); // Display blue

									// If the Player has MP, base the size of the actual Magic bar off remaining MP
	if ( stats[player] && stats[player]->MP > 0 )
	{
		//pos.x = 16;
		pos.w = playerStatusBarWidth - 5;
		pos.h = (playerStatusBarHeight - 27) * (static_cast<double>(stats[player]->MP) / stats[player]->MAXMP);
		pos.y = y2 - 15 - pos.h - hpmpbarOffsets;

		// Only draw the actual Magic bar if the Player has MP
		drawRect(&pos, mpColorFG, 255); // Display blue
	}

	// Print out the amount of MP the Player currently has
	if ( stats[player] )
	{
		snprintf(tempstr, 4, "%d", stats[player]->MP);
	}
	else
	{
		snprintf(tempstr, 4, "%d", 0);
	}
	printTextFormatted(font12x12_bmp, x1 + 32 * uiscale_playerbars - strlen(tempstr) * 6, y2 - (playerStatusBarHeight / 2 + 8) - hpmpbarOffsets, tempstr);

}

void drawStatus(int player)
{
	SDL_Rect pos, initial_position;
	Sint32 x, y, z, c, i;
	node_t* node;
	string_t* string;

	const int x1 = players[player]->camera_x1();
	const int x2 = players[player]->camera_x2();
	const int y1 = players[player]->camera_y1();
	const int y2 = players[player]->camera_y2();

	const Sint32 mousex = inputs.getMouse(player, Inputs::X);
	const Sint32 mousey = inputs.getMouse(player, Inputs::Y);
	const Sint32 omousex = inputs.getMouse(player, Inputs::OX);
	const Sint32 omousey = inputs.getMouse(player, Inputs::OY);
	const Sint32 mousexrel = inputs.getMouse(player, Inputs::XREL);
	const Sint32 mouseyrel = inputs.getMouse(player, Inputs::YREL);

	//pos.x = players[player]->statusBarUI.getStartX();
	auto& hotbar_t = players[player]->hotbar;
	auto& hotbar = hotbar_t.slots();

	int gui_mode = players[player]->gui_mode;
	bool shootmode = players[player]->shootmode;

	if ( !hide_statusbar )
	{
		//pos.y = players[player]->statusBarUI.getStartY();
	}
	else
	{
		pos.y = y2 - 16;
	}
	//To garner the position of the hotbar.
	initial_position.x = 0; /*hotbar_t.getStartX();*/
	initial_position.y = pos.y;
	initial_position.w = 0;
	initial_position.h = 0;
	//pos.w = status_bmp->w * uiscale_chatlog;
	//pos.h = status_bmp->h * uiscale_chatlog;
	//if ( !hide_statusbar )
	//{
	//	drawImageScaled(status_bmp, NULL, &pos);
	//}

	////players[player]->statusBarUI.messageStatusBarBox.x = pos.x;
	////players[player]->statusBarUI.messageStatusBarBox.y = pos.y;
	////players[player]->statusBarUI.messageStatusBarBox.w = pos.w;
	////players[player]->statusBarUI.messageStatusBarBox.h = pos.h;

	//// enemy health
	//enemyHPDamageBarHandler[player].displayCurrentHPBar(player);

	//// messages
	//if ( !hide_statusbar )
	//{
	//	x = players[player]->statusBarUI.getStartX() + 24 * uiscale_chatlog;
	//	y = players[player]->camera_y2();
	//	textscroll = std::min<Uint32>(list_Size(&messages) - 3, textscroll);
	//	c = 0;
	//	for ( node = messages.last; node != NULL; node = node->prev )
	//	{
	//		c++;
	//		if ( c <= textscroll )
	//		{
	//			continue;
	//		}
	//		string = (string_t*)node->element;
	//		if ( uiscale_chatlog >= 1.5 )
	//		{
	//			y -= TTF16_HEIGHT * string->lines;
	//			if ( y < y2 - (status_bmp->h * uiscale_chatlog) + 8 * uiscale_chatlog )
	//			{
	//				break;
	//			}
	//		}
	//		else if ( uiscale_chatlog != 1.f )
	//		{
	//			y -= TTF12_HEIGHT * string->lines;
	//			if ( y < y2 - status_bmp->h * 1.1 + 4 )
	//			{
	//				break;
	//			}
	//		}
	//		else
	//		{
	//			y -= TTF12_HEIGHT * string->lines;
	//			if ( y < y2 - status_bmp->h + 4 )
	//			{
	//				break;
	//			}
	//		}
	//		z = 0;
	//		for ( i = 0; i < strlen(string->data); i++ )
	//		{
	//			if ( string->data[i] != 10 )   // newline
	//			{
	//				z++;
	//			}
	//			else
	//			{
	//				z = 0;
	//			}
	//			if ( z == 65 )
	//			{
	//				if ( string->data[i] != 10 )
	//				{
	//					char* tempString = (char*)malloc(sizeof(char) * (strlen(string->data) + 2));
	//					strcpy(tempString, string->data);
	//					strcpy((char*)(tempString + i + 1), (char*)(string->data + i));
	//					tempString[i] = 10;
	//					free(string->data);
	//					string->data = tempString;
	//					string->lines++;
	//				}
	//				z = 0;
	//			}
	//		}
	//		Uint32 color = makeColor( 0, 0, 0, 255); // black color
	//		if ( uiscale_chatlog >= 1.5 )
	//		{
	//			ttfPrintTextColor(ttf16, x, y, color, false, string->data);
	//		}
	//		else
	//		{
	//			ttfPrintTextColor(ttf12, x, y, color, false, string->data);
	//		}
	//	}
	//	if ( inputs.bMouseLeft(player) )
	//	{
	//		if ( omousey >= y2 - status_bmp->h * uiscale_chatlog + 7 && omousey < y2 - status_bmp->h * uiscale_chatlog + (7 + 27) * uiscale_chatlog )
	//		{
	//			if ( omousex >= players[player]->statusBarUI.getStartX() + 618 * uiscale_chatlog
	//				&& omousex < players[player]->statusBarUI.getStartX() + 618 * uiscale_chatlog + 11 * uiscale_chatlog )
	//			{
	//				// text scroll up
	//				buttonclick = 3;
	//				textscroll++;
	//				inputs.mouseClearLeft(player);
	//			}
	//		}
	//		else if ( omousey >= y2 - status_bmp->h * uiscale_chatlog + 34 && omousey < y2 - status_bmp->h * uiscale_chatlog + (34 + 28) * uiscale_chatlog )
	//		{
	//			if ( omousex >= players[player]->statusBarUI.getStartX() + 618 * uiscale_chatlog
	//				&& omousex < players[player]->statusBarUI.getStartX() + 618 * uiscale_chatlog + 11 * uiscale_chatlog )
	//			{
	//				// text scroll down
	//				buttonclick = 12;
	//				textscroll--;
	//				if ( textscroll < 0 )
	//				{
	//					textscroll = 0;
	//				}
	//				inputs.mouseClearLeft(player);
	//			}
	//		}
	//		else if ( omousey >= y2 - status_bmp->h * uiscale_chatlog + 62 && omousey < y2 - status_bmp->h * uiscale_chatlog + (62 + 31) * uiscale_chatlog )
	//		{
	//			if ( omousex >= players[player]->statusBarUI.getStartX() + 618 * uiscale_chatlog
	//				&& omousex < players[player]->statusBarUI.getStartX() + 618 * uiscale_chatlog + 11 * uiscale_chatlog )
	//			{
	//				// text scroll down all the way
	//				buttonclick = 4;
	//				textscroll = 0;
	//				inputs.mouseClearLeft(player);
	//			}
	//		}
	//		/*else if( omousey>=y2-status_bmp->h+8 && omousey<y2-status_bmp->h+8+30 ) {
	//			if( omousex>=players[player]->statusBarUI.getStartX()+618 && omousex<players[player]->statusBarUI.getStartX()+618+11 ) {
	//				// text scroll up all the way
	//				buttonclick=13;
	//				textscroll=list_Size(&messages)-4;
	//				mousestatus[SDL_BUTTON_LEFT]=0;
	//			}
	//		}*/
	//	}

	//	// mouse wheel
	//	if ( !shootmode )
	//	{
	//		if ( mousex >= players[player]->statusBarUI.getStartX() && mousex < players[player]->statusBarUI.getStartX() + status_bmp->w * uiscale_chatlog )
	//		{
	//			if ( mousey >= initial_position.y && mousey < initial_position.y + status_bmp->h * uiscale_chatlog )
	//			{
	//				if ( mousestatus[SDL_BUTTON_WHEELDOWN] )
	//				{
	//					mousestatus[SDL_BUTTON_WHEELDOWN] = 0;
	//					textscroll--;
	//					if ( textscroll < 0 )
	//					{
	//						textscroll = 0;
	//					}
	//				}
	//				else if ( mousestatus[SDL_BUTTON_WHEELUP] )
	//				{
	//					mousestatus[SDL_BUTTON_WHEELUP] = 0;
	//					textscroll++;
	//				}
	//			}
	//		}
	//	}
	//	if (showfirst)
	//	{
	//		textscroll = list_Size(&messages) - 3;
	//	}


	//	//Text scroll up button.
	//	if ( buttonclick == 3 )
	//	{
	//		pos.x = players[player]->statusBarUI.getStartX() + 617 * uiscale_chatlog;
	//		pos.y = y2 - status_bmp->h * uiscale_chatlog + 7 * uiscale_chatlog;
	//		pos.w = 11 * uiscale_chatlog;
	//		pos.h = 27 * uiscale_chatlog;
	//		drawRect(&pos, makeColorRGB(255, 255, 255), 80);
	//		//drawImage(textup_bmp, NULL, &pos);
	//	}
	//	//Text scroll down all the way button.
	//	if ( buttonclick == 4 )
	//	{
	//		pos.x = players[player]->statusBarUI.getStartX() + 617 * uiscale_chatlog;
	//		pos.y = y2 - status_bmp->h * uiscale_chatlog + 62 * uiscale_chatlog;
	//		pos.w = 11 * uiscale_chatlog;
	//		pos.h = 31 * uiscale_chatlog;
	//		drawRect(&pos, makeColorRGB(255, 255, 255), 80);
	//		//drawImage(textdown_bmp, NULL, &pos);
	//	}
	//	//Text scroll down button.
	//	if ( buttonclick == 12 )
	//	{
	//		pos.x = players[player]->statusBarUI.getStartX() + 617 * uiscale_chatlog;
	//		pos.y = y2 - status_bmp->h * uiscale_chatlog + 34 * uiscale_chatlog;
	//		pos.w = 11 * uiscale_chatlog;
	//		pos.h = 28 * uiscale_chatlog;
	//		drawRect(&pos, makeColorRGB(255, 255, 255), 80);
	//		//drawImage(textup_bmp, NULL, &pos);
	//	}
	//	//Text scroll up all the way button.
	//	/*if( buttonclick==13 ) {
	//		pos.x=players[player]->statusBarUI.getStartX()+617; pos.y=y2-status_bmp->h+8;
	//		pos.w=11; pos.h=30;
	//		drawRect(&pos,0xffffffff,80);
	//		//drawImage(textdown_bmp, NULL, &pos);
	//	}*/
	//}

	//int playerStatusBarWidth = 38 * uiscale_playerbars;
	//int playerStatusBarHeight = 156 * uiscale_playerbars;

	//if ( !players[player]->hud.hpFrame )
	//{
	//	drawHPMPBars(player);
	//}
	//
	//// hunger icon
	//if ( stats[player] && stats[player]->type != AUTOMATON
	//	&& (svFlags & SV_FLAG_HUNGER) && stats[player]->HUNGER <= 250 && (ticks % 50) - (ticks % 25) )
	//{
	//	pos.x = /*xoffset*/ + playerStatusBarWidth + 10; // was pos.x = 128;
	//	pos.y = y2 - 160;
	//	pos.w = 64;
	//	pos.h = 64;
	//	if ( playerRequiresBloodToSustain(player) )
	//	{
	//		drawImageScaled(hunger_blood_bmp, NULL, &pos);
	//	}
	//	else
	//	{
	//		drawImageScaled(hunger_bmp, NULL, &pos);
	//	}
	//}

	//if ( stats[player] && stats[player]->type == AUTOMATON )
	//{
	//	if ( stats[player]->HUNGER > 300 || (ticks % 50) - (ticks % 25) )
	//	{
	//		pos.x = /*xoffset*/ + playerStatusBarWidth + 10; // was pos.x = 128;
	//		pos.y = y2 - 160;
	//		pos.w = 64;
	//		pos.h = 64;
	//		if ( stats[player]->HUNGER > 1200 )
	//		{
	//			drawImageScaled(hunger_boiler_hotflame_bmp, nullptr, &pos);
	//		}
	//		else
	//		{
	//			if ( stats[player]->HUNGER > 600 )
	//			{
	//				drawImageScaledPartial(hunger_boiler_flame_bmp, nullptr, &pos, 1.f);
	//			}
	//			else
	//			{
	//				float percent = (stats[player]->HUNGER - 300) / 300.f; // always show a little bit more at the bottom (10-20%)
	//				drawImageScaledPartial(hunger_boiler_flame_bmp, nullptr, &pos, percent);
	//			}
	//		}
	//		drawImageScaled(hunger_boiler_bmp, nullptr, &pos);
	//	}
	//}

	//// minotaur icon
	//if ( minotaurlevel && (ticks % 50) - (ticks % 25) )
	//{
	//	pos.x = /*xoffset*/ + playerStatusBarWidth + 10; // was pos.x = 128;
	//	pos.y = y2 - 160 + 64 + 2;
	//	pos.w = 64;
	//	pos.h = 64;
	//	drawImageScaled(minotaur_bmp, nullptr, &pos);
	//}

	// draw action prompts.
	/*if ( players[player]->hud.bShowActionPrompts )
	{
		int skill = (ticks / 100) % 16;
		int iconSize = 48;
		SDL_Rect skillPos{0, 0, iconSize, iconSize };
		skillPos.x = hotbar_t.hotbarBox.x - 3.5 * iconSize;
		skillPos.y = players[player]->camera_y2() - 106 - iconSize - 16;
		std::string promptString;
		players[player]->hud.drawActionIcon(skillPos, players[player]->hud.getActionIconForPlayer(Player::HUD_t::ACTION_PROMPT_OFFHAND, promptString));
		players[player]->hud.drawActionGlyph(skillPos, Player::HUD_t::ACTION_PROMPT_OFFHAND);

		skillPos.x = hotbar_t.hotbarBox.x - 2.5 * iconSize;
		players[player]->hud.drawActionIcon(skillPos, players[player]->hud.getActionIconForPlayer(Player::HUD_t::ACTION_PROMPT_MAINHAND, promptString));
		players[player]->hud.drawActionGlyph(skillPos, Player::HUD_t::ACTION_PROMPT_MAINHAND);

		skillPos.x = hotbar_t.hotbarBox.x + hotbar_t.hotbarBox.w + 0.5 * iconSize;
		players[player]->hud.drawActionIcon(skillPos, players[player]->hud.getActionIconForPlayer(Player::HUD_t::ACTION_PROMPT_MAGIC, promptString));
		players[player]->hud.drawActionGlyph(skillPos, Player::HUD_t::ACTION_PROMPT_MAGIC);
	}*/

	Item* item = nullptr;
	//Now the hotbar.
	int num = 0;
	//Reset the position to the top left corner of the status bar to draw the hotbar slots..
	//pos.x = initial_position.x;
	pos.x = 0; /*hotbar_t.getStartX();*/
	pos.y = initial_position.y - hotbar_t.getSlotSize();

	hotbar_t.hotbarBox.x = pos.x;
	hotbar_t.hotbarBox.y = pos.y;
	hotbar_t.hotbarBox.w = NUM_HOTBAR_SLOTS * hotbar_t.getSlotSize();
	hotbar_t.hotbarBox.h = hotbar_t.getSlotSize();

	if ( players[player]->hotbar.useHotbarFaceMenu )
	{
		players[player]->hotbar.initFaceButtonHotbar();
	}

	for ( num = 0; num < NUM_HOTBAR_SLOTS; ++num )
	{
		Uint32 color;
		if ( players[player]->hotbar.current_hotbar == num && !openedChest[player] )
		{
			color = makeColor( 255, 255, 0, 255); //Draw gold border around currently selected hotbar.
		}
		else
		{
			color = makeColor( 255, 255, 255, 60); //Draw normal grey border.
		}
		pos.w = hotbar_t.getSlotSize();
		pos.h = hotbar_t.getSlotSize();

		if ( players[player]->hotbar.useHotbarFaceMenu )
		{
			pos.x = players[player]->hotbar.faceButtonPositions[num].x;
			pos.y = players[player]->hotbar.faceButtonPositions[num].y;
		}

		//drawImageScaledColor(hotbar_img, NULL, &pos, color);
		item = uidToItem(hotbar[num].item);
		if ( item )
		{
			if ( item->type == BOOMERANG )
			{
				hotbar_t.magicBoomerangHotbarSlot = num;
			}
			bool used = false;
			pos.w = hotbar_t.getSlotSize();
			pos.h = hotbar_t.getSlotSize();

			SDL_Rect highlightBox;
			highlightBox.x = pos.x + 2;
			highlightBox.y = pos.y + 2;
			highlightBox.w = 60 * uiscale_hotbar;
			highlightBox.h = 60 * uiscale_hotbar;

			if ( !item->identified )
			{
				// give it a yellow background if it is unidentified
				drawRect(&highlightBox, makeColorRGB(128, 128, 0), 64); //31875
			}
			else if ( item->beatitude < 0 )
			{
				// give it a red background if cursed
				drawRect(&highlightBox, makeColorRGB(128, 0, 0), 64);
			}
			else if ( item->beatitude > 0 )
			{
				// give it a green background if blessed (light blue if colorblind mode)
				if ( colorblind )
				{
					drawRect(&highlightBox, makeColorRGB(50, 128, 128), 64);
				}
				else
				{
					drawRect(&highlightBox, makeColorRGB(0, 128, 0), 64);
				}
			}
			if ( item->status == BROKEN )
			{
				drawRect(&highlightBox, makeColorRGB(64, 64, 64), 125);
			}

			Uint32 itemColor = makeColor( 255, 255, 255, 255);
			if ( hotbar_t.useHotbarFaceMenu && hotbar_t.faceMenuButtonHeld != hotbar_t.GROUP_NONE )
			{
				bool dimColor = false;
				if ( hotbar_t.faceMenuButtonHeld != hotbar_t.getFaceMenuGroupForSlot(num) )
				{
					dimColor = true;
				}
				if ( dimColor )
				{
					itemColor = makeColor( 255, 255, 255, 128);
				}
			}
			drawImageScaledColor(itemSprite(item), NULL, &pos, itemColor);

			bool disableItemUsage = false;

			if ( players[player] && players[player]->entity && players[player]->entity->effectShapeshift != NOTHING )
			{
				// shape shifted, disable some items
				if ( !item->usableWhileShapeshifted(stats[player]) )
				{
					disableItemUsage = true;
					drawRect(&highlightBox, makeColorRGB(64, 64, 64), 144);
				}
			}
			if ( client_classes[player] == CLASS_SHAMAN )
			{
				if ( item->type == SPELL_ITEM && !(playerUnlockedShamanSpell(player, item)) )
				{
					disableItemUsage = true;
					drawRect(&highlightBox, makeColorRGB(64, 64, 64), 144);
				}
			}

			if ( stats[player] && stats[player]->HP > 0 )
			{
				Item*& selectedItem = inputs.getUIInteraction(player)->selectedItem;

				if ( !shootmode && mouseInBounds(player, pos.x, pos.x + hotbar_t.getSlotSize(), pos.y, pos.y + hotbar_t.getSlotSize()) )
				{
					if ( (inputs.bMouseLeft(player)
						|| (inputs.bControllerInputPressed(player, INJOY_MENU_LEFT_CLICK)
							&& !openedChest[player]
							&& gui_mode != (GUI_MODE_SHOP) 
							&& !GenericGUI[player].isGUIOpen())) 
						&& !selectedItem )
					{
						inputs.getUIInteraction(player)->toggleclick = false;
						if ( keystatus[SDLK_LSHIFT] || keystatus[SDLK_RSHIFT] )
						{
							hotbar[num].item = 0;
							hotbar[num].resetLastItem();
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
							hotbar[num].resetLastItem();

							if ( inputs.bControllerInputPressed(player, INJOY_MENU_LEFT_CLICK) && !openedChest[player] && gui_mode != (GUI_MODE_SHOP) )
							{
								inputs.controllerClearInput(player, INJOY_MENU_LEFT_CLICK);
								inputs.getUIInteraction(player)->toggleclick = true;
								inputs.getUIInteraction(player)->selectedItemFromHotbar = num;
								//TODO: Change the mouse cursor to THE HAND.
							}
						}
					}
					if ( inputs.bMouseRight(player)
						|| (inputs.bControllerInputPressed(player, INJOY_MENU_USE)
							&& !openedChest[player]
							&& gui_mode != (GUI_MODE_SHOP) 
							&& !GenericGUI[player].isGUIOpen()) )
					{
						//Use the item if right clicked.
						inputs.mouseClearRight(player);
						inputs.controllerClearInput(player, INJOY_MENU_USE);
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
						if ( itemCategory(item) == SPELLBOOK && (item->identified || itemIsEquipped(item, player)) )
						{
							// equipped spellbook will unequip on use.
							learnedSpell = (playerLearnedSpellbook(player, item) || itemIsEquipped(item, player));
						}

						if ( keystatus[SDLK_LSHIFT] || keystatus[SDLK_RSHIFT] )
						{
							players[player]->inventoryUI.appraisal.appraiseItem(item);
						}
						else
						{
							if ( (itemCategory(item) == POTION || itemCategory(item) == SPELLBOOK || item->type == FOOD_CREAMPIE )
								&& (keystatus[SDLK_LALT] || keystatus[SDLK_RALT]) )
							{
								badpotion = true;
								learnedSpell = true;
							}

							if ( !learnedSpell && item->identified 
								&& itemCategory(item) == SPELLBOOK && players[player] && players[player]->entity )
							{
								learnedSpell = true; // let's always equip/unequip spellbooks from the hotbar?
								spell_t* currentSpell = getSpellFromID(getSpellIDFromSpellbook(item->type));
								if ( currentSpell )
								{
									int skillLVL = stats[player]->getModifiedProficiency(PRO_MAGIC) + statGetINT(stats[player], players[player]->entity);
									if ( stats[player]->getModifiedProficiency(PRO_MAGIC) >= 100 )
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

							if ( itemCategory(item) == SPELLBOOK && stats[player] 
								&& (stats[player]->type == GOBLIN
									|| (stats[player]->playerRace == RACE_GOBLIN && stats[player]->stat_appearance == 0)) )
							{
								learnedSpell = true; // goblinos can't learn spells but always equip books.
							}

							if ( !badpotion && !learnedSpell )
							{
								if ( !(isItemEquippableInShieldSlot(item) && cast_animation[player].active_spellbook) )
								{
									if ( !disableItemUsage )
									{
										if ( stats[player] && stats[player]->type == AUTOMATON
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
												net_packet->data[25] = player;
												net_packet->address.host = net_server.host;
												net_packet->address.port = net_server.port;
												net_packet->len = 26;
												sendPacketSafe(net_sock, -1, net_packet, 0);
											}
											item_FoodAutomaton(item, player);
										}
										else
										{
											useItem(item, player);
										}
									}
									else
									{
										if ( client_classes[player] == CLASS_SHAMAN && item->type == SPELL_ITEM )
										{
											messagePlayer(player, MESSAGE_INVENTORY | MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(3488)); // unable to use with current level.
										}
										else
										{
											messagePlayer(player, MESSAGE_INVENTORY | MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(3432)); // unable to use in current form message.
										}
										playSoundPlayer(player, 90, 64);
									}
								}
							}
							else
							{
								if ( !disableItemUsage )
								{
									playerTryEquipItemAndUpdateServer(player, item, false);
								}
								else
								{
									if ( client_classes[player] == CLASS_SHAMAN && item->type == SPELL_ITEM )
									{
										messagePlayer(player, MESSAGE_INVENTORY | MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(3488)); // unable to use with current level.
									}
									else
									{
										messagePlayer(player, MESSAGE_INVENTORY | MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(3432)); // unable to use in current form message.
									}
									playSoundPlayer(player, 90, 64);
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
						printTextFormatted(digitFont, pos.x + hotbar_t.getSlotSize() - (24 * digits), pos.y + hotbar_t.getSlotSize() - 24, "%d", item->count);
					}
					else
					{
						printTextFormatted(digitFont, pos.x + hotbar_t.getSlotSize() - (14 * digits), pos.y + hotbar_t.getSlotSize() - 14, "%d", item->count);
					}
				}

				SDL_Rect src;
				src.x = pos.x + 2;
				src.h = 16 * uiscale_hotbar;
				src.y = pos.y + hotbar_t.getSlotSize() - src.h - 2;
				src.w = 16 * uiscale_hotbar;

				// item equipped
				if ( itemCategory(item) != SPELL_CAT )
				{
					if ( itemIsEquipped(item, player) )
					{
						//drawImageScaled(equipped_bmp, NULL, &src);
					}
					else if ( item->status == BROKEN )
					{
						//drawImageScaled(itembroken_bmp, NULL, &src);
					}
				}
				else
				{
					spell_t* spell = getSpellFromItem(player, item, false);
					if ( players[player]->magic.selectedSpell() == spell
						&& (players[player]->magic.selected_spell_last_appearance == item->appearance || players[player]->magic.selected_spell_last_appearance == -1 ) )
					{
						//drawImageScaled(equipped_bmp, NULL, &src);
					}
				}
			}
		}

		// draw hotbar slot 'numbers' or glyphs
		if ( players[player]->hotbar.useHotbarFaceMenu )
		{
			if ( players[player]->hotbar.faceMenuAlternateLayout )
			{
				SDL_Rect tmp{ pos.x, pos.y, pos.w, pos.h };
				if ( num == 1 )
				{
					tmp.y = pos.y + .75 * pos.h - 4;
					tmp.x = pos.x + pos.w / 2 + 16;
					tmp.x += pos.w;
					if ( !(hotbar_t.faceMenuButtonHeld != hotbar_t.GROUP_NONE && hotbar_t.faceMenuButtonHeld != hotbar_t.GROUP_LEFT) )
					{
						//players[player]->hotbar.drawFaceButtonGlyph(num, tmp);
					}
				}
				else if ( num == 4 )
				{
					tmp.y = pos.y + pos.h + 24;
					if ( !(hotbar_t.faceMenuButtonHeld != hotbar_t.GROUP_NONE && hotbar_t.faceMenuButtonHeld != hotbar_t.GROUP_MIDDLE) )
					{
						//players[player]->hotbar.drawFaceButtonGlyph(num, tmp);
					}
				}
				else if ( num == 7 )
				{
					tmp.y = pos.y + .75 * pos.h - 4;
					tmp.x = pos.x - (pos.w / 2 + 16);
					tmp.x -= pos.w;

					if ( !(hotbar_t.faceMenuButtonHeld != hotbar_t.GROUP_NONE && hotbar_t.faceMenuButtonHeld != hotbar_t.GROUP_RIGHT) )
					{
						//players[player]->hotbar.drawFaceButtonGlyph(num, tmp);
					}
				}
				else
				{
					//players[player]->hotbar.drawFaceButtonGlyph(num, tmp);
				}
			}
			else
			{
				//players[player]->hotbar.drawFaceButtonGlyph(num, pos);
			}
		}
		else if ( uiscale_hotbar >= 1.5 )
		{
			printTextFormatted(font16x16_bmp, pos.x + 2, pos.y + 2, "%d", (num + 1) % 10); // slot number
		}
		else
		{
			printTextFormatted(font12x12_bmp, pos.x + 2, pos.y + 2, "%d", (num + 1) % 10); // slot number
		}
		pos.x += hotbar_t.getSlotSize();
	}

	bool drawHotBarTooltipOnCycle = false;
	if ( !intro )
	{
		if ( hotbar_t.hotbarTooltipLastGameTick != 0 && (ticks - hotbar_t.hotbarTooltipLastGameTick) < TICKS_PER_SECOND * 2 )
		{
			drawHotBarTooltipOnCycle = true;
		}
		else if ( players[player]->hotbar.useHotbarFaceMenu && players[player]->hotbar.faceMenuButtonHeld != Player::Hotbar_t::GROUP_NONE )
		{
			drawHotBarTooltipOnCycle = true;
		}
	}

	if ( !shootmode || drawHotBarTooltipOnCycle )
	{
		pos.x = initial_position.x;

		//Go back through all of the hotbar slots and draw the tooltips.
		for ( num = 0; num < NUM_HOTBAR_SLOTS; ++num )
		{
			if ( players[player]->hotbar.useHotbarFaceMenu )
			{
				pos.x = players[player]->hotbar.faceButtonPositions[num].x;
				pos.y = players[player]->hotbar.faceButtonPositions[num].y;
			}
			item = uidToItem(hotbar[num].item);
			if ( item )
			{
				bool drawTooltipOnSlot = !shootmode && mouseInBounds(player, pos.x, pos.x + hotbar_t.getSlotSize(), pos.y, pos.y + hotbar_t.getSlotSize());
				if ( !drawTooltipOnSlot )
				{
					if ( drawHotBarTooltipOnCycle && players[player]->hotbar.current_hotbar == num )
					{
						drawTooltipOnSlot = true;
					}
				}
				else
				{
					if ( !shootmode )
					{
						// reset timer.
						hotbar_t.hotbarTooltipLastGameTick = 0;
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
						if ( players[player]->hotbar.useHotbarFaceMenu )
						{
							src.x = pos.x + hotbar_t.getSlotSize() / 2;
							src.y = pos.y - 32;
						}
						else
						{
							src.x = pos.x + hotbar_t.getSlotSize();
							src.y = pos.y + hotbar_t.getSlotSize();
							src.y -= 16;
						}
					}

					if ( itemCategory(item) == SPELL_CAT )
					{
						spell_t* spell = getSpellFromItem(player, item, false);
						if ( drawHotBarTooltipOnCycle )
						{
							//drawSpellTooltip(player, spell, item, &src);
						}
						else
						{
							//drawSpellTooltip(player, spell, item, nullptr);
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
								learnedSpellbook = playerLearnedSpellbook(player, item);
								if ( !learnedSpellbook && stats[player] && players[player] && players[player]->entity )
								{
									// spellbook tooltip shows if you have the magic requirement as well (for goblins)
									int skillLVL = stats[player]->getModifiedProficiency(PRO_MAGIC) + statGetINT(stats[player], players[player]->entity);
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
								src.w = std::max((2 + longestline(Language::get(3862)) + longestline(item->getScrollLabel())) * TTF12_WIDTH + 8, src.w);
							}
							else if ( itemCategory(item) == SPELLBOOK && learnedSpellbook )
							{
								int height = 1;
								char effectType[32] = "";
								int spellID = getSpellIDFromSpellbook(item->type);
								int damage = 0;;// drawSpellTooltip(player, getSpellFromID(spellID), item, nullptr);
								real_t dummy = 0.f;
								//getSpellEffectString(spellID, spellEffectText, effectType, damage, &height, &dummy);
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

						int furthestX = players[player]->camera_x2();
						/*if ( players[player]->characterSheet.proficienciesPage == 0 )
						{
							if ( src.y < players[player]->characterSheet.skillsSheetBox.y + players[player]->characterSheet.skillsSheetBox.h )
							{
								furthestX = players[player]->camera_x2() - players[player]->characterSheet.skillsSheetBox.w;
							}
						}
						else
						{
							if ( src.y < players[player]->characterSheet.partySheetBox.y + players[player]->characterSheet.partySheetBox.h )
							{
								furthestX = players[player]->camera_x2() - players[player]->characterSheet.partySheetBox.w;
							}
						}*/

						if ( drawHotBarTooltipOnCycle && players[player]->hotbar.useHotbarFaceMenu )
						{
							// draw centred.
							src.x -= src.w / 2;
							src.y -= src.h;
						}
						
						if ( src.x + src.w + 16 > furthestX ) // overflow right side of screen
						{
							src.x -= (src.w + 32);
						}
						if ( src.y + src.h + 16 > y2 ) // overflow bottom of screen
						{
							src.y -= (src.y + src.h + 16 - y2);
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
							color = makeColorRGB(255, 255, 0);
							ttfPrintTextFormattedColor(ttf12, src.x + 4 + TTF12_WIDTH, src.y + 4 + TTF12_HEIGHT, color, Language::get(309));
						}
						else
						{
							if ( item->beatitude < 0 )
							{
								color = makeColorRGB(255, 0, 0);
								ttfPrintTextFormattedColor(ttf12, src.x + 4 + TTF12_WIDTH, src.y + 4 + TTF12_HEIGHT, color, Language::get(310));
							}
							else if ( item->beatitude == 0 )
							{
								color = 0xFFFFFFFF;
								ttfPrintTextFormattedColor(ttf12, src.x + 4 + TTF12_WIDTH, src.y + 4 + TTF12_HEIGHT, color, Language::get(311));
							}
							else
							{
								color = makeColorRGB(0, 255, 0);
								ttfPrintTextFormattedColor(ttf12, src.x + 4 + TTF12_WIDTH, src.y + 4 + TTF12_HEIGHT, color, Language::get(312));
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
						int itemWeight = item->getWeight();
						ttfPrintTextFormatted(ttf12, src.x + 4 + TTF12_WIDTH, src.y + 4 + TTF12_HEIGHT * 2, Language::get(313), itemWeight);
						ttfPrintTextFormatted(ttf12, src.x + 4 + TTF12_WIDTH, src.y + 4 + TTF12_HEIGHT * 3, Language::get(314), item->sellValue(player));
						if ( strcmp(spellEffectText, "") )
						{
							ttfPrintTextFormattedColor(ttf12, src.x + 4, src.y + 4 + TTF12_HEIGHT * 4, makeColorRGB(0, 255, 255), spellEffectText);
						}

						if ( item->identified && stats[player] )
						{
							if ( itemCategory(item) == WEAPON || itemCategory(item) == THROWN
								|| itemTypeIsQuiver(item->type) )
							{
								Monster tmpRace = stats[player]->type;
								if ( stats[player]->type == TROLL
									|| stats[player]->type == RAT
									|| stats[player]->type == SPIDER
									|| stats[player]->type == CREATURE_IMP )
								{
									// these monsters have 0 bonus from weapons, but want the tooltip to say the normal amount.
									stats[player]->type = HUMAN;
								}

								if ( item->weaponGetAttack(stats[player]) >= 0 )
								{
									color = makeColorRGB(0, 255, 255);
								}
								else
								{
									color = makeColorRGB(255, 0, 0);
								}
								if ( stats[player]->type != tmpRace )
								{
									color = makeColorRGB(127, 127, 127); // grey out the text if monster doesn't benefit.
								}

								ttfPrintTextFormattedColor(ttf12, src.x + 4 + TTF12_WIDTH, src.y + 4 + TTF12_HEIGHT * 4, color, Language::get(315), item->weaponGetAttack(stats[player]));
								stats[player]->type = tmpRace;
							}
							else if ( itemCategory(item) == ARMOR )
							{
								Monster tmpRace = stats[player]->type;
								if ( stats[player]->type == TROLL
									|| stats[player]->type == RAT
									|| stats[player]->type == SPIDER
									|| stats[player]->type == CREATURE_IMP )
								{
									// these monsters have 0 bonus from armor, but want the tooltip to say the normal amount.
									stats[player]->type = HUMAN;
								}

								if ( item->armorGetAC(stats[player]) >= 0 )
								{
									color = makeColorRGB(0, 255, 255);
								}
								else
								{
									color = makeColorRGB(255, 0, 0);
								}
								if ( stats[player]->type != tmpRace )
								{
									color = makeColorRGB(127, 127, 127); // grey out the text if monster doesn't benefit.
								}

								ttfPrintTextFormattedColor(ttf12, src.x + 4 + TTF12_WIDTH, src.y + 4 + TTF12_HEIGHT * 4, color, Language::get(316), item->armorGetAC(stats[player]));
								stats[player]->type = tmpRace;
							}
							else if ( itemCategory(item) == SCROLL )
							{
								color = makeColorRGB(0, 255, 255);
								ttfPrintTextFormattedColor(ttf12, src.x + 4 + TTF12_WIDTH, src.y + 4 + TTF12_HEIGHT * 4, color, "%s%s", Language::get(3862), item->getScrollLabel());
							}
						}
					}
					if ( !drawHotBarTooltipOnCycle && playerSettings[multiplayer ? 0 : player].hotbar_numkey_quick_add && inputs.bPlayerUsingKeyboardControl(player) )
					{
						Uint32 swapItem = 0;
						if ( keystatus[SDLK_1] )
						{
							keystatus[SDLK_1] = 0;
							swapItem = hotbar[0].item;
							hotbar[0].item = hotbar[num].item;
							hotbar[num].item = swapItem;
						}
						if ( keystatus[SDLK_2] )
						{
							keystatus[SDLK_2] = 0;
							swapItem = hotbar[1].item;
							hotbar[1].item = hotbar[num].item;
							hotbar[num].item = swapItem;
						}
						if ( keystatus[SDLK_3] )
						{
							keystatus[SDLK_3] = 0;
							swapItem = hotbar[2].item;
							hotbar[2].item = hotbar[num].item;
							hotbar[num].item = swapItem;
						}
						if ( keystatus[SDLK_4] )
						{
							keystatus[SDLK_4] = 0;
							swapItem = hotbar[3].item;
							hotbar[3].item = hotbar[num].item;
							hotbar[num].item = swapItem;
						}
						if ( keystatus[SDLK_5] )
						{
							keystatus[SDLK_5] = 0;
							swapItem = hotbar[4].item;
							hotbar[4].item = hotbar[num].item;
							hotbar[num].item = swapItem;
						}
						if ( keystatus[SDLK_6] )
						{
							keystatus[SDLK_6] = 0;
							swapItem = hotbar[5].item;
							hotbar[5].item = hotbar[num].item;
							hotbar[num].item = swapItem;
						}
						if ( keystatus[SDLK_7] )
						{
							keystatus[SDLK_7] = 0;
							swapItem = hotbar[6].item;
							hotbar[6].item = hotbar[num].item;
							hotbar[num].item = swapItem;
						}
						if ( keystatus[SDLK_8] )
						{
							keystatus[SDLK_8] = 0;
							swapItem = hotbar[7].item;
							hotbar[7].item = hotbar[num].item;
							hotbar[num].item = swapItem;
						}
						if ( keystatus[SDLK_9] )
						{
							keystatus[SDLK_9] = 0;
							swapItem = hotbar[8].item;
							hotbar[8].item = hotbar[num].item;
							hotbar[num].item = swapItem;
						}
						if ( keystatus[SDLK_0] )
						{
							keystatus[SDLK_0] = 0;
							swapItem = hotbar[9].item;
							hotbar[9].item = hotbar[num].item;
							hotbar[num].item = swapItem;
						}
					}
				}
			}
			pos.x += hotbar_t.getSlotSize();
		}
	}

	//NOTE: If you change the number of hotbar slots, you *MUST* change this.
	if ( !command && stats[player] && stats[player]->HP > 0 )
	{
		Item* item = NULL;
		const auto& inventoryUI = players[multiplayer ? 0 : player]->inventoryUI;
		if ( !(!shootmode && playerSettings[multiplayer ? 0 : player].hotbar_numkey_quick_add
			/*&&	(
					(omousex >= inventoryUI.getStartX()
						&& omousex <= inventoryUI.getStartX() + inventoryUI.getSizeX() * inventoryUI.getSlotSize()
						&& omousey >= inventoryUI.getStartY()
						&& omousey <= inventoryUI.getStartY() + inventoryUI.getSizeY() * inventoryUI.getSlotSize()
					)
					||
					(omousex >= initial_position.x 
						&& omousex <= initial_position.x + hotbar_t.getSlotSize() * NUM_HOTBAR_SLOTS
						&& omousey >= initial_position.y - hotbar_t.getSlotSize()
						&& omousey <= initial_position.y
					)
				)*/
			) )
		{
			// if hotbar_numkey_quick_add is enabled, then the number keys won't do the default equip function
			// skips equipping items if the mouse is in the hotbar or inventory area. otherwise the below code runs.
			if ( inputs.bPlayerUsingKeyboardControl(player) && !StatueManager.activeEditing )
			{
				if ( keystatus[SDLK_1] )
				{
					keystatus[SDLK_1] = 0;
					item = uidToItem(hotbar[0].item);
					hotbar_t.current_hotbar = 0;
				}
				if ( keystatus[SDLK_2] )
				{
					keystatus[SDLK_2] = 0;
					item = uidToItem(hotbar[1].item);
					hotbar_t.current_hotbar = 1;
				}
				if ( keystatus[SDLK_3] )
				{
					keystatus[SDLK_3] = 0;
					item = uidToItem(hotbar[2].item);
					hotbar_t.current_hotbar = 2;
				}
				if ( keystatus[SDLK_4] )
				{
					keystatus[SDLK_4] = 0;
					item = uidToItem(hotbar[3].item);
					hotbar_t.current_hotbar = 3;
				}
				if ( keystatus[SDLK_5] )
				{
					keystatus[SDLK_5] = 0;
					item = uidToItem(hotbar[4].item);
					hotbar_t.current_hotbar = 4;
				}
				if ( keystatus[SDLK_6] )
				{
					keystatus[SDLK_6] = 0;
					item = uidToItem(hotbar[5].item);
					hotbar_t.current_hotbar = 5;
				}
				if ( keystatus[SDLK_7] )
				{
					keystatus[SDLK_7] = 0;
					item = uidToItem(hotbar[6].item);
					hotbar_t.current_hotbar = 6;
				}
				if ( keystatus[SDLK_8] )
				{
					keystatus[SDLK_8] = 0;
					item = uidToItem(hotbar[7].item);
					hotbar_t.current_hotbar = 7;
				}
				if ( keystatus[SDLK_9] )
				{
					keystatus[SDLK_9] = 0;
					item = uidToItem(hotbar[8].item);
					hotbar_t.current_hotbar = 8;
				}
				if ( keystatus[SDLK_0] )
				{
					keystatus[SDLK_0] = 0;
					item = uidToItem(hotbar[9].item);
					hotbar_t.current_hotbar = 9;
				}
			}
			if ( players[player]->hotbar.useHotbarFaceMenu
				&& !openedChest[player]
				&& gui_mode != (GUI_MODE_SHOP)
				&& !GenericGUI[player].isGUIOpen()
				&& !inputs.getUIInteraction(player)->selectedItem
				&& shootmode )
			{
				Player::Hotbar_t::FaceMenuGroup pressed = Player::Hotbar_t::GROUP_NONE;

				for ( int i = 0; i < 3; ++i )
				{
					int button = SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_B + i;
					if ( inputs.bControllerRawInputPressed(player, 301 + button) )
					{
						if ( inputs.bControllerRawInputPressed(player, 301 + SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_DOWN) )
						{
							inputs.controllerClearRawInput(player, 301 + button);
							inputs.controllerClearRawInput(player, 301 + SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_DOWN);
							inputs.controllerClearRawInputRelease(player, 301 + button);
							break;
						}
						
						switch ( button )
						{
							case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_B:
								pressed = Player::Hotbar_t::GROUP_RIGHT;
								break;
							case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_Y:
								pressed = Player::Hotbar_t::GROUP_MIDDLE;
								break;
							case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_X:
								pressed = Player::Hotbar_t::GROUP_LEFT;
								break;
							default:
								break;
						}

						std::array<int, 3> slotOrder = { 0, 1, 2 };
						int centerSlot = 1;
						if ( hotbar_t.faceMenuAlternateLayout )
						{
							slotOrder = { 0, 2, 1 };
						}
						if ( button == SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_B )
						{
							centerSlot = 7;
							if ( hotbar_t.faceMenuAlternateLayout )
							{
								slotOrder = { 7, 6, 8 };
							}
							else
							{
								slotOrder = { 6, 7, 8 };
							}
						}
						else if ( button == SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_Y )
						{
							centerSlot = 4;
							slotOrder = { 3, 4, 5 };
						}

						if ( hotbar_t.faceMenuAlternateLayout )
						{
							if ( true )
							{
								// temp test
								if ( inputs.bControllerRawInputPressed(player, 301 + SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSHOULDER) )
								{
									hotbar_t.selectHotbarSlot(slotOrder[0]);
								}
								else if ( inputs.bControllerRawInputPressed(player, 301 + SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) )
								{
									hotbar_t.selectHotbarSlot(slotOrder[2]);
								}
								else if ( players[player]->hotbar.faceMenuButtonHeld == Player::Hotbar_t::GROUP_NONE )
								{
									hotbar_t.selectHotbarSlot(slotOrder[1]);
								}
							}
							else
							{
								if ( inputs.bControllerRawInputPressed(player, 301 + SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSHOULDER) )
								{
									hotbar_t.selectHotbarSlot(std::max(centerSlot - 1, hotbar_t.current_hotbar - 1));
									inputs.controllerClearRawInput(player, 301 + SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
								}
								else if ( inputs.bControllerRawInputPressed(player, 301 + SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) )
								{
									hotbar_t.selectHotbarSlot(std::min(centerSlot + 1, hotbar_t.current_hotbar + 1));
									inputs.controllerClearRawInput(player, 301 + SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
								}
								else if ( players[player]->hotbar.faceMenuButtonHeld == Player::Hotbar_t::GROUP_NONE )
								{
									hotbar_t.selectHotbarSlot(slotOrder[1]);
								}
							}
						}
						else
						{
							if ( inputs.bControllerRawInputPressed(player, 301 + SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSHOULDER) )
							{
								hotbar_t.selectHotbarSlot(std::max(centerSlot - 1, hotbar_t.current_hotbar - 1));
								inputs.controllerClearRawInput(player, 301 + SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
							}
							else if ( inputs.bControllerRawInputPressed(player, 301 + SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) )
							{
								hotbar_t.selectHotbarSlot(std::min(centerSlot + 1, hotbar_t.current_hotbar + 1));
								inputs.controllerClearRawInput(player, 301 + SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
							}
							else if ( players[player]->hotbar.faceMenuButtonHeld == Player::Hotbar_t::GROUP_NONE )
							{
								hotbar_t.selectHotbarSlot(slotOrder[1]);
							}
						}
						break;
					}
					else if ( inputs.bControllerRawInputReleased(player, 301 + button) )
					{
						item = uidToItem(players[player]->hotbar.slots()[hotbar_t.current_hotbar].item);
						inputs.controllerClearRawInputRelease(player, 301 + button);
						break;
					}
				}

				players[player]->hotbar.faceMenuButtonHeld = pressed;

				if ( pressed != Player::Hotbar_t::GROUP_NONE 
					&& players[player]->hotbar.faceMenuQuickCastEnabled && item && itemCategory(item) == SPELL_CAT )
				{
					spell_t* spell = getSpellFromItem(player, item, false);
					if ( spell && players[player]->magic.selectedSpell() == spell )
					{
						players[player]->hotbar.faceMenuQuickCast = true;
					}
				}
			}
		}

		//Moving the cursor changes the currently selected hotbar slot.
		if ( (mousexrel || mouseyrel) && !shootmode )
		{
			pos.x = initial_position.x;
			pos.y = initial_position.y - hotbar_t.getSlotSize();
			for ( c = 0; c < NUM_HOTBAR_SLOTS; ++c, pos.x += hotbar_t.getSlotSize() )
			{
				if ( players[player]->hotbar.useHotbarFaceMenu )
				{
					pos.x = players[player]->hotbar.faceButtonPositions[c].x;
					pos.y = players[player]->hotbar.faceButtonPositions[c].y;
				}
				if ( mouseInBoundsRealtimeCoords(player, pos.x, pos.x + hotbar_t.getSlotSize(), pos.y, pos.y + hotbar_t.getSlotSize()) )
				{
					players[player]->hotbar.selectHotbarSlot(c);
				}
			}
		}

		bool bumper_moved = false;

		Input& input = Input::inputs[player];

		if ( !players[player]->hotbar.useHotbarFaceMenu && input.consumeBinaryToggle("Hotbar Right")
			&& players[player]->bControlEnabled && !gamePaused && !players[player]->usingCommand() )
		{
			if ( shootmode && !inputs.getUIInteraction(player)->itemMenuOpen && !openedChest[player]
				&& gui_mode != (GUI_MODE_SHOP) && !players[player]->bookGUI.bBookOpen
				&& !GenericGUI[player].isGUIOpen() )
			{
				bumper_moved = true;
				hotbar_t.hotbarTooltipLastGameTick = ticks;
				players[player]->hotbar.selectHotbarSlot(players[player]->hotbar.current_hotbar + 1);
			}
			else
			{
				hotbar_t.hotbarTooltipLastGameTick = 0;
			}
		}
		if ( !players[player]->hotbar.useHotbarFaceMenu && input.consumeBinaryToggle("Hotbar Left")
			&& players[player]->bControlEnabled && !gamePaused && !players[player]->usingCommand() )
		{
			if ( shootmode && !inputs.getUIInteraction(player)->itemMenuOpen && !openedChest[player]
				&& gui_mode != (GUI_MODE_SHOP) && !players[player]->bookGUI.bBookOpen
				&& !GenericGUI[player].isGUIOpen() )
			{
				bumper_moved = true;
				hotbar_t.hotbarTooltipLastGameTick = ticks;
				hotbar_t.selectHotbarSlot(hotbar_t.current_hotbar - 1);
			}
			else
			{
				hotbar_t.hotbarTooltipLastGameTick = 0;
			}
		}

		if ( bumper_moved && !inputs.getUIInteraction(player)->itemMenuOpen
			&& !openedChest[player] && gui_mode != (GUI_MODE_SHOP) 
			&& !players[player]->bookGUI.bBookOpen
			&& !GenericGUI[player].isGUIOpen() )
		{
			warpMouseToSelectedHotbarSlot(player);
		}

		if ( !inputs.getUIInteraction(player)->itemMenuOpen && !inputs.getUIInteraction(player)->selectedItem && !openedChest[player] && gui_mode != (GUI_MODE_SHOP) )
		{
			if ( shootmode && input.consumeBinaryToggle("Hotbar Up / Select")
                && (!hotbar_t.useHotbarFaceMenu || (hotbar_t.useHotbarFaceMenu && !inputs.hasController(player)))
				&& players[player]->bControlEnabled && !gamePaused
				&& !players[player]->usingCommand()
				&& !openedChest[player] && gui_mode != (GUI_MODE_SHOP)
				&& !players[player]->bookGUI.bBookOpen
				&& !GenericGUI[player].isGUIOpen() )
			{
				//Show a tooltip
				hotbar_t.hotbarTooltipLastGameTick = std::max(ticks - TICKS_PER_SECOND, ticks - hotbar_t.hotbarTooltipLastGameTick);

				//Activate a hotbar slot if in-game.
				item = uidToItem(hotbar[hotbar_t.current_hotbar].item);
			}

            // We don't have a hotbar clear binding, but if we need one, feel free to add it
            // in MainMenu.cpp

			/*if ( !shootmode && inputs.bControllerInputPressed(player, INJOY_MENU_HOTBAR_CLEAR) && !players[player]->bookGUI.bBookOpen ) //TODO: Don't activate if any of the previous if statement's conditions are true?
			{
				//Clear a hotbar slot if in-inventory.
				inputs.controllerClearInput(player, INJOY_MENU_HOTBAR_CLEAR);

				hotbar[hotbar_t.current_hotbar].item = 0;
			}*/

			pos.x = initial_position.x + (hotbar_t.current_hotbar * hotbar_t.getSlotSize());
			pos.y = initial_position.y - hotbar_t.getSlotSize();
			//if ( !shootmode && !players[player]->bookGUI.bBookOpen && !openedChest[player] && inputs.bControllerInputPressed(player, INJOY_MENU_DROP_ITEM)
			//	&& mouseInBounds(player, pos.x, pos.x + hotbar_img->w * uiscale_hotbar, pos.y, pos.y + hotbar_img->h * uiscale_hotbar) )
			//{
			//	//Drop item if this hotbar is currently active & the player pressed the cancel button on the gamepad (typically "b").
			//	inputs.controllerClearInput(player, INJOY_MENU_DROP_ITEM);
			//	Item* itemToDrop = uidToItem(hotbar[hotbar_t.current_hotbar].item);
			//	if ( itemToDrop )
			//	{
			//		dropItem(itemToDrop, player);
			//	}
			//}
		}

		if ( item )
		{
			bool badpotion = false;
			bool learnedSpell = false;
			if ( itemCategory(item) == POTION )
			{
				badpotion = isPotionBad(*item);
			}

			if ( itemCategory(item) == SPELLBOOK && (item->identified || itemIsEquipped(item, player)) )
			{
				// equipped spellbook will unequip on use.
				learnedSpell = (playerLearnedSpellbook(player, item) || itemIsEquipped(item, player));
			}

			if ( (keystatus[SDLK_LALT] || keystatus[SDLK_RALT]) 
				&& (itemCategory(item) == POTION || itemCategory(item) == SPELLBOOK || item->type == FOOD_CREAMPIE) )
			{
				badpotion = true;
				learnedSpell = true;
			}

			if ( !learnedSpell && item->identified
				&& itemCategory(item) == SPELLBOOK && players[player] && players[player]->entity )
			{
				learnedSpell = true; // let's always equip/unequip spellbooks from the hotbar?
				spell_t* currentSpell = getSpellFromID(getSpellIDFromSpellbook(item->type));
				if ( currentSpell && stats[player] )
				{
					int skillLVL = stats[player]->getModifiedProficiency(PRO_MAGIC) + statGetINT(stats[player], players[player]->entity);
					if ( stats[player]->getModifiedProficiency(PRO_MAGIC) >= 100 )
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

			if ( itemCategory(item) == SPELLBOOK && stats[player] )
			{
				if ( stats[player]->type == GOBLIN || stats[player]->type == CREATURE_IMP
						|| (stats[player]->playerRace == RACE_GOBLIN && stats[player]->stat_appearance == 0) )
				{
					learnedSpell = true; // goblinos can't learn spells but always equip books.
				}
			}

			bool disableItemUsage = false;
			if ( players[player] && players[player]->entity )
			{
				if ( players[player]->entity->effectShapeshift != NOTHING )
				{
					if ( !item->usableWhileShapeshifted(stats[player]) )
					{
						disableItemUsage = true;
					}
				}
				else
				{
					if ( itemCategory(item) == SPELL_CAT && item->appearance >= 1000 )
					{
						if ( canUseShapeshiftSpellInCurrentForm(player, *item) != 1 )
						{
							disableItemUsage = true;
						}
					}
				}
			}
			if ( client_classes[player] == CLASS_SHAMAN )
			{
				if ( item->type == SPELL_ITEM && !(playerUnlockedShamanSpell(player, item)) )
				{
					disableItemUsage = true;
				}
			}

			if ( !disableItemUsage )
			{
				if ( !badpotion && !learnedSpell )
				{
					if ( !(isItemEquippableInShieldSlot(item) && cast_animation[player].active_spellbook) )
					{
						if ( stats[player] && stats[player]->type == AUTOMATON
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
								net_packet->data[25] = player;
								net_packet->address.host = net_server.host;
								net_packet->address.port = net_server.port;
								net_packet->len = 26;
								sendPacketSafe(net_sock, -1, net_packet, 0);
							}
							item_FoodAutomaton(item, player);
						}
						else
						{
							useItem(item, player);
						}
					}
				}
				else
				{
					playerTryEquipItemAndUpdateServer(player, item, false);
				}
			}
			else
			{
				if ( client_classes[player] == CLASS_SHAMAN && item->type == SPELL_ITEM )
				{
					messagePlayer(player, MESSAGE_INVENTORY | MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(3488)); // unable to use with current level.
				}
				else
				{
					messagePlayer(player, MESSAGE_INVENTORY | MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(3432)); // unable to use in current form message.
				}
				playSoundPlayer(player, 90, 64);
			}
		}
	}

	FollowerMenu[player].drawFollowerMenu();

	//// stat increase icons
	//pos.w = 64;
	//pos.h = 64;
	//pos.x = players[player]->camera_x2() - pos.w * 3 - 9;
	//pos.y = players[player]->characterSheet.skillsSheetBox.h + (32 + pos.h * 2 + 3); // 131px from end of prof window.

	//if ( (!shootmode || players[player]->characterSheet.lock_right_sidebar) && players[player]->characterSheet.proficienciesPage == 1
	//	&& pos.y < (players[player]->characterSheet.partySheetBox.y + players[player]->characterSheet.partySheetBox.h + 16) )
	//{
	//	pos.y = players[player]->characterSheet.partySheetBox.y + players[player]->characterSheet.partySheetBox.h + 16;
	//}

	//if ( splitscreen )
	//{
	//	// todo - adjust position.
	//	pos.w = 48;
	//	pos.h = 48;
	//	pos.x = players[player]->camera_x2() - pos.w * 3 - 9;
	//	pos.y = players[player]->characterSheet.skillsSheetBox.h + (16 + pos.h * 2 + 3);
	//}
	//else
	//{
	//	if ( pos.y + pos.h > (players[player]->camera_y2() - minimaps[player].y - minimaps[player].h) ) // check if overlapping minimap
	//	{
	//		pos.y = (players[player]->camera_y2() - minimaps[player].y - minimaps[player].h) - (64 + 3); // align above minimap
	//	}
	//}
	
	SDL_Surface *tmp_bmp = NULL;

	for ( i = 0; i < NUMSTATS; i++ )
	{
		if ( stats[player] && stats[player]->PLAYER_LVL_STAT_TIMER[i] > 0 && ((ticks % 50) - (ticks % 10)) )
		{
			stats[player]->PLAYER_LVL_STAT_TIMER[i]--;

			//switch ( i )
			//{
			//	// prepare the stat image.
			//	case STAT_STR:
			//		tmp_bmp = str_bmp64u;
			//		break;
			//	case STAT_DEX:
			//		tmp_bmp = dex_bmp64u;
			//		break;
			//	case STAT_CON:
			//		tmp_bmp = con_bmp64u;
			//		break;
			//	case STAT_INT:
			//		tmp_bmp = int_bmp64u;
			//		break;
			//	case STAT_PER:
			//		tmp_bmp = per_bmp64u;
			//		break;
			//	case STAT_CHR:
			//		tmp_bmp = chr_bmp64u;
			//		break;
			//	default:
			//		break;
			//}
			drawImageScaled(tmp_bmp, NULL, &pos);
			if ( stats[player]->PLAYER_LVL_STAT_TIMER[i + NUMSTATS] > 0 )
			{
				// bonus stat acheived, draw additional stat icon above.
				pos.y -= 64 + 3;
				drawImageScaled(tmp_bmp, NULL, &pos);
				pos.y += 64 + 3;
				stats[player]->PLAYER_LVL_STAT_TIMER[i + NUMSTATS]--;
			}

			pos.x += pos.h + 3;
		}
	}
}

void drawStatusNew(const int player)
{
	Sint32 x, y, z, c, i;
	node_t* node;

	const int x1 = players[player]->camera_x1();
	const int x2 = players[player]->camera_x2();
	const int y1 = players[player]->camera_y1();
	const int y2 = players[player]->camera_y2();

	const Sint32 mousex = inputs.getMouse(player, Inputs::X);
	const Sint32 mousey = inputs.getMouse(player, Inputs::Y);
	const Sint32 omousex = inputs.getMouse(player, Inputs::OX);
	const Sint32 omousey = inputs.getMouse(player, Inputs::OY);
	const Sint32 mousexrel = inputs.getMouse(player, Inputs::XREL);
	const Sint32 mouseyrel = inputs.getMouse(player, Inputs::YREL);

	auto& hotbar_t = players[player]->hotbar;
	auto& hotbar = hotbar_t.slots();

	int gui_mode = players[player]->gui_mode;
	bool shootmode = players[player]->shootmode;

	Input& input = Input::inputs[player];

	int playerStatusBarWidth = 38 * uiscale_playerbars;
	int playerStatusBarHeight = 156 * uiscale_playerbars;

	/*if ( !players[player]->hud.hpFrame )
	{
		drawHPMPBars(player);
	}*/

	// hunger icon
	//if ( stats[player] && stats[player]->type != AUTOMATON
	//	&& (svFlags & SV_FLAG_HUNGER) && stats[player]->HUNGER <= 250 && (ticks % 50) - (ticks % 25) )
	//{
	//	SDL_Rect pos;
	//	pos.x = /*xoffset*/ +playerStatusBarWidth + 10 - 43; // was pos.x = 128;
	//	pos.y = y2 - 160 + 64 + 2 - 82 + 4;
	//	pos.w = 64;
	//	pos.h = 64;
	//	if ( playerRequiresBloodToSustain(player) )
	//	{
	//		drawImageScaled(hunger_blood_bmp, NULL, &pos);
	//	}
	//	else
	//	{
	//		drawImageScaled(hunger_bmp, NULL, &pos);
	//	}
	//}

	//if ( stats[player] && stats[player]->type == AUTOMATON )
	//{
	//	if ( stats[player]->HUNGER > 300 || (ticks % 50) - (ticks % 25) )
	//	{
	//		SDL_Rect pos;
	//		pos.x = /*xoffset*/ +playerStatusBarWidth + 10 - 43 + 128; // was pos.x = 128;
	//		pos.y = y2 - 160 + 64 + 2 - 82 + 4;
	//		pos.w = 64;
	//		pos.h = 64;
	//		if ( stats[player]->HUNGER > 1200 )
	//		{
	//			drawImageScaled(hunger_boiler_hotflame_bmp, nullptr, &pos);
	//		}
	//		else
	//		{
	//			if ( stats[player]->HUNGER > 600 )
	//			{
	//				drawImageScaledPartial(hunger_boiler_flame_bmp, nullptr, &pos, 1.f);
	//			}
	//			else
	//			{
	//				float percent = (stats[player]->HUNGER - 300) / 300.f; // always show a little bit more at the bottom (10-20%)
	//				drawImageScaledPartial(hunger_boiler_flame_bmp, nullptr, &pos, percent);
	//			}
	//		}
	//		drawImageScaled(hunger_boiler_bmp, nullptr, &pos);
	//	}
	//}

	// minotaur icon
	//if ( minotaurlevel && (ticks % 50) - (ticks % 25) )
	//{
	//	SDL_Rect pos;
	//	pos.x = /*xoffset*/ +playerStatusBarWidth + 10 - 64 + 43 + 64; // was pos.x = 128;
	//	pos.y = y2 - 160 + 64 + 2 - 82 + 4;
	//	pos.w = 64;
	//	pos.h = 64;
	//	drawImageScaled(minotaur_bmp, nullptr, &pos);
	//}


	if ( players[player]->hotbar.useHotbarFaceMenu )
	{
		players[player]->hotbar.initFaceButtonHotbar();
	}

	bool& toggleclick = inputs.getUIInteraction(player)->toggleclick;
	bool& itemMenuOpen = inputs.getUIInteraction(player)->itemMenuOpen;
	Uint32& itemMenuItem = inputs.getUIInteraction(player)->itemMenuItem;
	int& itemMenuX = inputs.getUIInteraction(player)->itemMenuX;
	int& itemMenuY = inputs.getUIInteraction(player)->itemMenuY;
	int& itemMenuSelected = inputs.getUIInteraction(player)->itemMenuSelected;
	bool& itemMenuFromHotbar = inputs.getUIInteraction(player)->itemMenuFromHotbar;

	auto& appraisal = players[player]->inventoryUI.appraisal;

	//Now the hotbar.
	for ( int num = 0; num < NUM_HOTBAR_SLOTS; ++num )
	{
		Frame* hotbarSlotFrame = nullptr;
		if ( hotbar_t.hotbarFrame )
		{
			hotbarSlotFrame = hotbar_t.getHotbarSlotFrame(num);
			if ( inputs.getUIInteraction(player)->selectedItem )
			{
				if ( hotbar_t.oldSlotFrameTrackSlot == num )
				{
					if ( auto oldSelectedItemFrame = hotbar_t.hotbarFrame->findFrame("hotbar old selected item") )
					{
						oldSelectedItemFrame->setSize(hotbarSlotFrame->getSize());
					}
				}
			}
			else
			{
				hotbar_t.oldSlotFrameTrackSlot = -1;
			}
		}

		Item* item = uidToItem(hotbar[num].item);
		if ( item )
		{
			if ( shootmode && item->notifyIcon )
			{
				if ( appraisal.itemsToNotify.find(item->uid) == appraisal.itemsToNotify.end() )
				{
					appraisal.itemsToNotify[item->uid] = Player::Inventory_t::Appraisal_t::NOTIFY_ITEM_WAITING_TO_HOVER;
				}
				else
				{
					if ( appraisal.itemsToNotify[item->uid] == Player::Inventory_t::Appraisal_t::NOTIFY_ITEM_REMOVE )
					{
						appraisal.itemsToNotify.erase(item->uid);
						item->notifyIcon = false;
					}
				}
			}

			if ( item->type == BOOMERANG )
			{
				hotbar_t.magicBoomerangHotbarSlot = num;
			}
			bool used = false;
			bool disableItemUsage = false;

			if ( players[player] && players[player]->entity && players[player]->entity->effectShapeshift != NOTHING )
			{
				// shape shifted, disable some items
				if ( !item->usableWhileShapeshifted(stats[player]) )
				{
					disableItemUsage = true;
				}
			}
			if ( client_classes[player] == CLASS_SHAMAN )
			{
				if ( item->type == SPELL_ITEM && !(playerUnlockedShamanSpell(player, item)) )
				{
					disableItemUsage = true;
				}
			}

			if ( stats[player] && stats[player]->HP > 0 )
			{
				Item*& selectedItem = inputs.getUIInteraction(player)->selectedItem;

				if ( !shootmode && !hotbarSlotFrame->isDisabled() 
					&& players[player]->GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_HOTBAR)
					&& !selectedItem
					&& players[player]->hotbar.current_hotbar == num
					&& players[player]->GUI.activeModule == Player::GUI_t::MODULE_HOTBAR
					&& !players[player]->GUI.isDropdownActive()
					&& hotbarSlotFrame->capturesMouse()
					&& players[player]->bControlEnabled && !gamePaused
					&& !players[player]->usingCommand() )
				{
					if ( (Input::inputs[player].binaryToggle("MenuLeftClick") && inputs.bPlayerUsingKeyboardControl(player))
						|| (Input::inputs[player].binaryToggle(getContextMenuOptionBindingName(player, PROMPT_GRAB).c_str())
							&& hotbarGamepadControlEnabled(player))
						&& (players[player]->inventoryUI.bFirstTimeSnapCursor) )
					{
						toggleclick = false;
						if ( (keystatus[SDLK_LSHIFT] || keystatus[SDLK_RSHIFT])
							&& Input::inputs[player].binaryToggle("MenuLeftClick") && inputs.bPlayerUsingKeyboardControl(player) )
						{
							hotbar[num].item = 0;
							hotbar[num].resetLastItem();
							Input::inputs[player].consumeBinaryToggle("MenuLeftClick");
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
							hotbar[num].resetLastItem();

							if ( inputs.getVirtualMouse(player)->draw_cursor )
							{
								// this is the inventory cursor, not the hotbar-specific one.
								players[player]->inventoryUI.cursor.lastUpdateTick = ticks;
							}

							if ( Input::inputs[player].binaryToggle(getContextMenuOptionBindingName(player, PROMPT_GRAB).c_str())
								&& !openedChest[player] && gui_mode != (GUI_MODE_SHOP) )
							{
								Input::inputs[player].consumeBinaryToggle(getContextMenuOptionBindingName(player, PROMPT_GRAB).c_str());
								toggleclick = true;
								inputs.getUIInteraction(player)->selectedItemFromHotbar = num;
								inputs.getUIInteraction(player)->selectedItemFromChest = 0;
								//TODO: Change the mouse cursor to THE HAND.
							}

							if ( hotbar_t.hotbarFrame )
							{
								if ( auto oldSelectedItemFrame = hotbar_t.hotbarFrame->findFrame("hotbar old selected item") )
								{
									if ( Frame* hotbarFrame = hotbar_t.getHotbarSlotFrame(num) )
									{
										oldSelectedItemFrame->setSize(hotbarFrame->getSize());
										hotbar_t.oldSlotFrameTrackSlot = num;
										oldSelectedItemFrame->setDisabled(false);
										if ( auto oldImg = oldSelectedItemFrame->findImage("hotbar old selected item") )
										{
											oldImg->disabled = true;
											if ( selectedItem )
											{
												oldImg->path = getItemSpritePath(player, *selectedItem);
												if ( oldImg->path != "" )
												{
													oldImg->disabled = false;
												}
											}
										}
									}
								}
							}
						}
					}
					if ( Input::inputs[player].binaryToggle("MenuRightClick") && inputs.bPlayerUsingKeyboardControl(player)
						&& !players[player]->GUI.isDropdownActive() && !selectedItem )
					{
						if ( (keystatus[SDLK_LSHIFT] || keystatus[SDLK_RSHIFT]) ) //TODO: selected shop slot, identify, remove curse?
						{
							// auto-appraise the item
							players[player]->inventoryUI.appraisal.appraiseItem(item);
							Input::inputs[player].consumeBinaryToggle("MenuRightClick");
						}
						else if ( !disableItemUsage && (itemCategory(item) == POTION || itemCategory(item) == SPELLBOOK || item->type == FOOD_CREAMPIE) &&
							(keystatus[SDLK_LALT] || keystatus[SDLK_RALT]) )
						{
							Input::inputs[player].consumeBinaryToggle("MenuRightClick");
							// force equip potion/spellbook
							playerTryEquipItemAndUpdateServer(player, item, false);
						}
						else
						{
							// open a drop-down menu of options for "using" the item
							itemMenuOpen = true;
							itemMenuFromHotbar = true;
							itemMenuX = (inputs.getMouse(player, Inputs::X) / (float)xres) * (float)Frame::virtualScreenX + 8;
							itemMenuY = (inputs.getMouse(player, Inputs::Y) / (float)yres) * (float)Frame::virtualScreenY;
							auto interactFrame = players[player]->inventoryUI.interactFrame;
							if ( interactFrame )
							{
								if ( auto interactMenuTop = interactFrame->findImage("interact top background") )
								{
									// 10px is slot half height, minus the top interact text height
									// mouse will be situated halfway in first menu option
									itemMenuY -= (interactMenuTop->pos.h + 10 + 2);
									auto numOptions = getContextMenuOptionsForItem(player, item).size();
									if ( numOptions > 1 )
									{
										itemMenuY -= (numOptions - 1 + 1) * (24); // +1 because extra hotbar prompt to clear slot
									}
								}
							}
							if ( itemMenuX % 2 == 1 )
							{
								++itemMenuX; // even pixel adjustment
							}
							if ( itemMenuY % 2 == 1 )
							{
								++itemMenuY; // even pixel adjustment
							}
							itemMenuY = std::max(itemMenuY, players[player]->camera_virtualy1());

							bool alignRight = true;
							if ( !alignRight )
							{
								itemMenuX -= 16;
							}
							if ( interactFrame )
							{
								if ( auto highlightImage = interactFrame->findImage("interact selected highlight") )
								{
									highlightImage->disabled = true;
								}
							}
							itemMenuSelected = 0;
							itemMenuItem = item->uid;

							toggleclick = false; //Default reset. Otherwise will break mouse support after using gamepad once to trigger a context menu.

							if ( inputs.getVirtualMouse(player)->draw_cursor )
							{
								players[player]->inventoryUI.cursor.lastUpdateTick = ticks;
							}
						}
					}
				}
			}
		}
	}

	bool drawHotBarTooltipOnCycle = false;
	if ( !intro )
	{
		if ( hotbar_t.hotbarTooltipLastGameTick != 0 && (ticks - hotbar_t.hotbarTooltipLastGameTick) < TICKS_PER_SECOND * 2 )
		{
			drawHotBarTooltipOnCycle = true;
		}
		else if ( players[player]->hotbar.useHotbarFaceMenu && players[player]->hotbar.faceMenuButtonHeld != Player::Hotbar_t::GROUP_NONE )
		{
			drawHotBarTooltipOnCycle = true;
		}
		if ( hotbar_t.animHide > 0.01 )
		{
			drawHotBarTooltipOnCycle = false;
		}
		if ( FollowerMenu[player].followerMenuIsOpen() || CalloutMenu[player].calloutMenuIsOpen() )
		{
			drawHotBarTooltipOnCycle = false;
		}
	}

	bool tooltipOpen = false;
	Frame* tooltipSlotFrame = nullptr;
	bool tooltipPromptFrameWasDisabled = true;

	if ( (!shootmode && !FollowerMenu[player].followerMenuIsOpen() && !CalloutMenu[player].calloutMenuIsOpen()) || drawHotBarTooltipOnCycle)
	{
		//Go back through all of the hotbar slots and draw the tooltips.
		for ( int num = 0; num < NUM_HOTBAR_SLOTS; ++num )
		{
			SDL_Rect pos;
			Frame* hotbarSlotFrame = nullptr;
			if ( hotbar_t.hotbarFrame )
			{
				hotbarSlotFrame = hotbar_t.getHotbarSlotFrame(num);
				pos = hotbarSlotFrame->getSize();
			}

			Item* item = uidToItem(hotbar[num].item);
			if ( item )
			{
				bool drawTooltipOnSlot = !shootmode 
					&& !hotbarSlotFrame->isDisabled() 
					&& players[player]->hotbar.current_hotbar == num
					&& players[player]->GUI.activeModule == Player::GUI_t::MODULE_HOTBAR
					&& players[player]->GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_HOTBAR)
					&& hotbarSlotFrame->capturesMouse();
				if ( !drawTooltipOnSlot )
				{
					if ( drawHotBarTooltipOnCycle && players[player]->hotbar.current_hotbar == num )
					{
						drawTooltipOnSlot = true;
					}
				}
				else
				{
					if ( !shootmode )
					{
						// reset timer.
						hotbar_t.hotbarTooltipLastGameTick = 0;
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
					if ( appraisal.itemsToNotify.find(item->uid) != appraisal.itemsToNotify.end() )
					{
						if ( appraisal.itemsToNotify[item->uid] == Player::Inventory_t::Appraisal_t::NOTIFY_ITEM_WAITING_TO_HOVER )
						{
							appraisal.itemsToNotify[item->uid] = Player::Inventory_t::Appraisal_t::NOTIFY_ITEM_HOVERED;
						}
					}
					for ( auto& itemNotify : appraisal.itemsToNotify )
					{
						if ( itemNotify.first != item->uid && itemNotify.second == Player::Inventory_t::Appraisal_t::NOTIFY_ITEM_HOVERED )
						{
							itemNotify.second = Player::Inventory_t::Appraisal_t::NOTIFY_ITEM_REMOVE;
						}
					}

					//Tooltip
					SDL_Rect src;
					src.x = mousex + 16;
					src.y = mousey + 8;

					if ( drawHotBarTooltipOnCycle )
					{
						if ( players[player]->hotbar.useHotbarFaceMenu )
						{
							src.x = pos.x + hotbar_t.getSlotSize() / 2;
							src.y = pos.y - 32;
						}
						else
						{
							src.x = pos.x + hotbar_t.getSlotSize();
							src.y = pos.y + hotbar_t.getSlotSize();
							src.y -= 16;
						}
					}

					if ( hotbar_t.hotbarFrame && players[player]->inventoryUI.tooltipFrame 
						&& !inputs.getUIInteraction(player)->selectedItem
						&& !players[player]->GUI.isDropdownActive() )
					{
						src.x = hotbarSlotFrame->getSize().x + hotbarSlotFrame->getSize().w / 2;
						src.y = hotbarSlotFrame->getSize().y - 16;
						tooltipOpen = true;
						tooltipSlotFrame = hotbarSlotFrame;
						players[player]->hud.updateFrameTooltip(item, src.x, src.y, players[player]->PANEL_JUSTIFY_LEFT);

						auto tooltipFrame = players[player]->inventoryUI.tooltipFrame;
						if ( players[player]->inventoryUI.itemTooltipDisplay.displayingTitleOnlyTooltip )
						{
							tooltipFrame = players[player]->inventoryUI.titleOnlyTooltipFrame;
						}

						SDL_Rect tooltipPos = tooltipFrame->getSize();
						tooltipPos.x = src.x - tooltipPos.w / 2;
						tooltipPos.y = src.y - tooltipPos.h;
						if ( players[player]->inventoryUI.itemTooltipDisplay.displayingTitleOnlyTooltip )
						{
							int highestSlotY = players[player]->camera_virtualy2();
							for ( int num2 = 0; num2 < NUM_HOTBAR_SLOTS; ++num2 )
							{
								if ( auto slotFrame = hotbar_t.getHotbarSlotFrame(num2) )
								{
									if ( !slotFrame->isDisabled() )
									{
										highestSlotY = std::min(highestSlotY, slotFrame->getSize().y);
									}
								}
							}
							if ( hotbar_t.useHotbarFaceMenu )
							{
								static ConsoleVariable<int> cvar_tooltip_title_only_facemenu_y("/tooltip_title_only_facemenu_y", 4);
								if ( inputs.hasController(player) )
								{
									tooltipPos.y -= 12 * hotbar_t.selectedSlotAnimateCurrentValue;
								}
								else
								{
									tooltipPos.y = highestSlotY - tooltipPos.h - 8;
									tooltipPos.x = hotbar_t.hotbarFrame->getSize().w / 2 - tooltipPos.w / 2;
									if ( tooltipPos.x % 2 == 1 )
									{
										++tooltipPos.x;
									}
								}
								tooltipPos.y += *cvar_tooltip_title_only_facemenu_y;
							}
							else
							{
								static ConsoleVariable<int> cvar_tooltip_title_only_y("/tooltip_title_only_y", 8);
								tooltipPos.x = hotbar_t.hotbarFrame->getSize().w / 2 - tooltipPos.w / 2;
								tooltipPos.y += *cvar_tooltip_title_only_y;
								if ( tooltipPos.x % 2 == 1 )
								{
									++tooltipPos.x;
								}
							}
						}
						if ( tooltipPos.x < 0 )
						{
							tooltipPos.x = 0;
						}
						else if ( tooltipPos.x + tooltipPos.w > players[player]->inventoryUI.tooltipContainerFrame->getSize().w )
						{
							tooltipPos.x -= (tooltipPos.x + tooltipPos.w) - players[player]->inventoryUI.tooltipContainerFrame->getSize().w;
						}
						tooltipFrame->setSize(tooltipPos);
						if ( players[player]->inventoryUI.tooltipPromptFrame
							&& !players[player]->inventoryUI.tooltipPromptFrame->isDisabled()
							&& !(players[player]->inventoryUI.useItemDropdownOnGamepad == Player::Inventory_t::GAMEPAD_DROPDOWN_FULL 
								&& !inputs.getVirtualMouse(player)->draw_cursor) )
						{
							SDL_Rect tooltipPos = players[player]->inventoryUI.tooltipFrame->getSize();
							SDL_Rect promptPos = players[player]->inventoryUI.tooltipPromptFrame->getSize();
							promptPos.x = tooltipPos.x + tooltipPos.w - 6 - promptPos.w;
							promptPos.y = tooltipPos.y + tooltipPos.h - 2;

							const int heightChange = promptPos.h * .75;

							tooltipPos.y -= heightChange;
							promptPos.y -= heightChange;

							players[player]->inventoryUI.tooltipPromptFrame->setSize(promptPos);
							players[player]->inventoryUI.tooltipFrame->setSize(tooltipPos);
						}
						if ( players[player]->inventoryUI.tooltipPromptFrame )
						{
							tooltipPromptFrameWasDisabled = players[player]->inventoryUI.tooltipPromptFrame->isDisabled();
							if ( players[player]->inventoryUI.useItemDropdownOnGamepad == Player::Inventory_t::GAMEPAD_DROPDOWN_FULL 
								&& !inputs.getVirtualMouse(player)->draw_cursor )
							{
								players[player]->inventoryUI.tooltipPromptFrame->setDisabled(true);
							}
						}
					}

					if ( !drawHotBarTooltipOnCycle && playerSettings[multiplayer ? 0 : player].hotbar_numkey_quick_add
						&& players[player]->bControlEnabled && !gamePaused
						&& !players[player]->usingCommand()
						&& inputs.bPlayerUsingKeyboardControl(player) )
					{
						Uint32 swapItem = 0;
						if ( Input::inputs[player].binaryToggle("Hotbar Slot 1") )
						{
							Input::inputs[player].consumeBinaryToggle("Hotbar Slot 1");
							swapItem = hotbar[0].item;
							hotbar[0].item = hotbar[num].item;
							hotbar[num].item = swapItem;
							if ( hotbar[0].item == 0 || !uidToItem(hotbar[0].item) )
							{
								hotbar[0].resetLastItem();
							}
							if ( hotbar[num].item == 0 || !uidToItem(hotbar[num].item) )
							{
								hotbar[num].resetLastItem();
							}
						}
						if ( Input::inputs[player].binaryToggle("Hotbar Slot 2") )
						{
							Input::inputs[player].consumeBinaryToggle("Hotbar Slot 2");
							swapItem = hotbar[1].item;
							hotbar[1].item = hotbar[num].item;
							hotbar[num].item = swapItem;
							if ( hotbar[1].item == 0 || !uidToItem(hotbar[1].item) )
							{
								hotbar[1].resetLastItem();
							}
							if ( hotbar[num].item == 0 || !uidToItem(hotbar[num].item) )
							{
								hotbar[num].resetLastItem();
							}
						}
						if ( Input::inputs[player].binaryToggle("Hotbar Slot 3") )
						{
							Input::inputs[player].consumeBinaryToggle("Hotbar Slot 3");
							swapItem = hotbar[2].item;
							hotbar[2].item = hotbar[num].item;
							hotbar[num].item = swapItem;
							if ( hotbar[2].item == 0 || !uidToItem(hotbar[2].item) )
							{
								hotbar[2].resetLastItem();
							}
							if ( hotbar[num].item == 0 || !uidToItem(hotbar[num].item) )
							{
								hotbar[num].resetLastItem();
							}
						}
						if ( Input::inputs[player].binaryToggle("Hotbar Slot 4") )
						{
							Input::inputs[player].consumeBinaryToggle("Hotbar Slot 4");
							swapItem = hotbar[3].item;
							hotbar[3].item = hotbar[num].item;
							hotbar[num].item = swapItem;
							if ( hotbar[3].item == 0 || !uidToItem(hotbar[3].item) )
							{
								hotbar[3].resetLastItem();
							}
							if ( hotbar[num].item == 0 || !uidToItem(hotbar[num].item) )
							{
								hotbar[num].resetLastItem();
							}
						}
						if ( Input::inputs[player].binaryToggle("Hotbar Slot 5") )
						{
							Input::inputs[player].consumeBinaryToggle("Hotbar Slot 5");
							swapItem = hotbar[4].item;
							hotbar[4].item = hotbar[num].item;
							hotbar[num].item = swapItem;
							if ( hotbar[4].item == 0 || !uidToItem(hotbar[4].item) )
							{
								hotbar[4].resetLastItem();
							}
							if ( hotbar[num].item == 0 || !uidToItem(hotbar[num].item) )
							{
								hotbar[num].resetLastItem();
							}
						}
						if ( Input::inputs[player].binaryToggle("Hotbar Slot 6") )
						{
							Input::inputs[player].consumeBinaryToggle("Hotbar Slot 6");
							swapItem = hotbar[5].item;
							hotbar[5].item = hotbar[num].item;
							hotbar[num].item = swapItem;
							if ( hotbar[5].item == 0 || !uidToItem(hotbar[5].item) )
							{
								hotbar[5].resetLastItem();
							}
							if ( hotbar[num].item == 0 || !uidToItem(hotbar[num].item) )
							{
								hotbar[num].resetLastItem();
							}
						}
						if ( Input::inputs[player].binaryToggle("Hotbar Slot 7") )
						{
							Input::inputs[player].consumeBinaryToggle("Hotbar Slot 7");
							swapItem = hotbar[6].item;
							hotbar[6].item = hotbar[num].item;
							hotbar[num].item = swapItem;
							if ( hotbar[6].item == 0 || !uidToItem(hotbar[6].item) )
							{
								hotbar[6].resetLastItem();
							}
							if ( hotbar[num].item == 0 || !uidToItem(hotbar[num].item) )
							{
								hotbar[num].resetLastItem();
							}
						}
						if ( Input::inputs[player].binaryToggle("Hotbar Slot 8") )
						{
							Input::inputs[player].consumeBinaryToggle("Hotbar Slot 8");
							swapItem = hotbar[7].item;
							hotbar[7].item = hotbar[num].item;
							hotbar[num].item = swapItem;
							if ( hotbar[7].item == 0 || !uidToItem(hotbar[7].item) )
							{
								hotbar[7].resetLastItem();
							}
							if ( hotbar[num].item == 0 || !uidToItem(hotbar[num].item) )
							{
								hotbar[num].resetLastItem();
							}
						}
						if ( Input::inputs[player].binaryToggle("Hotbar Slot 9") )
						{
							Input::inputs[player].consumeBinaryToggle("Hotbar Slot 9");
							swapItem = hotbar[8].item;
							hotbar[8].item = hotbar[num].item;
							hotbar[num].item = swapItem;
							if ( hotbar[8].item == 0 || !uidToItem(hotbar[8].item) )
							{
								hotbar[8].resetLastItem();
							}
							if ( hotbar[num].item == 0 || !uidToItem(hotbar[num].item) )
							{
								hotbar[num].resetLastItem();
							}
						}
						if ( Input::inputs[player].binaryToggle("Hotbar Slot 10")
							&& hotbar_t.getHotbarSlotFrame(9)
							&& !hotbar_t.getHotbarSlotFrame(9)->isDisabled() )
						{
							Input::inputs[player].consumeBinaryToggle("Hotbar Slot 10");
							swapItem = hotbar[9].item;
							hotbar[9].item = hotbar[num].item;
							hotbar[num].item = swapItem;
							if ( hotbar[9].item == 0 || !uidToItem(hotbar[9].item) )
							{
								hotbar[9].resetLastItem();
							}
							if ( hotbar[num].item == 0 || !uidToItem(hotbar[num].item) )
							{
								hotbar[num].resetLastItem();
							}
						}
					}
				}
				else
				{
					if ( shootmode 
						|| (!shootmode 
							&& players[player]->GUI.activeModule == Player::GUI_t::MODULE_HOTBAR) )
					{
						if ( appraisal.itemsToNotify.find(item->uid) != appraisal.itemsToNotify.end() )
						{
							if ( appraisal.itemsToNotify[item->uid] == Player::Inventory_t::Appraisal_t::NOTIFY_ITEM_HOVERED )
							{
								appraisal.itemsToNotify[item->uid] = Player::Inventory_t::Appraisal_t::NOTIFY_ITEM_REMOVE;
							}
						}
					}
				}
			}
		}
	}

	//NOTE: If you change the number of hotbar slots, you *MUST* change this.
	if ( !players[player]->usingCommand() && stats[player] && stats[player]->HP > 0
		&& players[player]->bControlEnabled && !gamePaused )
	{
		Item* item = NULL;
		const auto& inventoryUI = players[player]->inventoryUI;
		if ( inputs.bPlayerUsingKeyboardControl(player)
			&& players[player]->gui_mode != GUI_MODE_SIGN
			&& (shootmode || (!shootmode && !(playerSettings[multiplayer ? 0 : player].hotbar_numkey_quick_add && (mouseInsidePlayerHotbar(player) || mouseInsidePlayerInventory(player))))) )
		{
			// if hotbar_numkey_quick_add is enabled, then the number keys won't do the default equip function
			// skips equipping items if the mouse is in the hotbar or inventory area. otherwise the below code runs.

			bool pressed = false;
			if ( Input::inputs[player].binaryToggle("Hotbar Slot 1") )
			{
				Input::inputs[player].consumeBinaryToggle("Hotbar Slot 1");
				item = uidToItem(hotbar[0].item);
				hotbar_t.current_hotbar = 0;
				pressed = true;
			}
			if ( Input::inputs[player].binaryToggle("Hotbar Slot 2") )
			{
				Input::inputs[player].consumeBinaryToggle("Hotbar Slot 2");
				item = uidToItem(hotbar[1].item);
				hotbar_t.current_hotbar = 1;
				pressed = true;
			}
			if ( Input::inputs[player].binaryToggle("Hotbar Slot 3") )
			{
				Input::inputs[player].consumeBinaryToggle("Hotbar Slot 3");
				item = uidToItem(hotbar[2].item);
				hotbar_t.current_hotbar = 2;
				pressed = true;
			}
			if ( Input::inputs[player].binaryToggle("Hotbar Slot 4") )
			{
				Input::inputs[player].consumeBinaryToggle("Hotbar Slot 4");
				item = uidToItem(hotbar[3].item);
				hotbar_t.current_hotbar = 3;
				pressed = true;
			}
			if ( Input::inputs[player].binaryToggle("Hotbar Slot 5") )
			{
				Input::inputs[player].consumeBinaryToggle("Hotbar Slot 5");
				item = uidToItem(hotbar[4].item);
				hotbar_t.current_hotbar = 4;
				pressed = true;
			}
			if ( Input::inputs[player].binaryToggle("Hotbar Slot 6") )
			{
				Input::inputs[player].consumeBinaryToggle("Hotbar Slot 6");
				item = uidToItem(hotbar[5].item);
				hotbar_t.current_hotbar = 5;
				pressed = true;
			}
			if ( Input::inputs[player].binaryToggle("Hotbar Slot 7") )
			{
				Input::inputs[player].consumeBinaryToggle("Hotbar Slot 7");
				item = uidToItem(hotbar[6].item);
				hotbar_t.current_hotbar = 6;
				pressed = true;
			}
			if ( Input::inputs[player].binaryToggle("Hotbar Slot 8") )
			{
				Input::inputs[player].consumeBinaryToggle("Hotbar Slot 8");
				item = uidToItem(hotbar[7].item);
				hotbar_t.current_hotbar = 7;
				pressed = true;
			}
			if ( Input::inputs[player].binaryToggle("Hotbar Slot 9") )
			{
				Input::inputs[player].consumeBinaryToggle("Hotbar Slot 9");
				item = uidToItem(hotbar[8].item);
				hotbar_t.current_hotbar = 8;
				pressed = true;
			}
			if ( Input::inputs[player].binaryToggle("Hotbar Slot 10")
				&& hotbar_t.getHotbarSlotFrame(9)
				&& !hotbar_t.getHotbarSlotFrame(9)->isDisabled() )
			{
				Input::inputs[player].consumeBinaryToggle("Hotbar Slot 10");
				item = uidToItem(hotbar[9].item);
				hotbar_t.current_hotbar = 9;
				pressed = true;
			}

			// quickcasting spells
			if ( pressed && item && itemCategory(item) == SPELL_CAT )
			{
				spell_t* spell = getSpellFromItem(player, item, true);
				if ( spell && players[player]->magic.selectedSpell() == spell )
				{
					players[player]->hotbar.faceMenuQuickCast = true;
				}
			}
		}

		if ( players[player]->hotbar.useHotbarFaceMenu
			&& !openedChest[player]
			&& gui_mode != (GUI_MODE_SHOP)
			&& !GenericGUI[player].isGUIOpen()
			&& !inputs.getUIInteraction(player)->selectedItem
			&& shootmode )
		{
			Player::Hotbar_t::FaceMenuGroup pressed = Player::Hotbar_t::GROUP_NONE;

			for ( int i = 0; i < 3; ++i )
			{
				std::string inputName = "";
				switch ( i )
				{
					case 0:
						inputName = "Hotbar Left";
						break;
					case 1:
						inputName = "Hotbar Up / Select";
						break;
					case 2:
						inputName = "Hotbar Right";
						break;
					default:
						break;
				}

				if ( Input::inputs[player].binaryToggle(inputName.c_str()) )
				{
					if ( Input::inputs[player].binaryToggle("Hotbar Down / Cancel") )
					{
						Input::inputs[player].consumeBinaryToggle(inputName.c_str());
						Input::inputs[player].consumeBinaryToggle("Hotbar Down / Cancel");
						Input::inputs[player].consumeBindingsSharedWithBinding("Hotbar Down / Cancel");

						for ( auto& slot : players[player]->hotbar.slots() )
						{
							Uint32 uid = slot.item;
							if ( uid != 0 )
							{
								if ( appraisal.itemsToNotify.find(uid) != appraisal.itemsToNotify.end() )
								{
									if ( appraisal.itemsToNotify[uid] == Player::Inventory_t::Appraisal_t::NOTIFY_ITEM_HOVERED )
									{
										appraisal.itemsToNotify[uid] = Player::Inventory_t::Appraisal_t::NOTIFY_ITEM_REMOVE;
									}
								}
							}
						}
						Player::soundCancel();
						players[player]->hotbar.faceMenuButtonHeld = Player::Hotbar_t::GROUP_NONE;
						break;
					}

					int centerSlot = 1;
					std::array<int, 3> slotOrder = { 0, 1, 2 };
					if ( inputName == "Hotbar Left" )
					{
						pressed = Player::Hotbar_t::GROUP_LEFT;
					}
					else if ( inputName == "Hotbar Up / Select" )
					{
						pressed = Player::Hotbar_t::GROUP_MIDDLE;
						centerSlot = 4;
						slotOrder = { 3, 4, 5 };
					}
					else if ( inputName == "Hotbar Right" )
					{
						pressed = Player::Hotbar_t::GROUP_RIGHT;
						centerSlot = 7;
						slotOrder = { 6, 7, 8 };
					}
					
					if ( hotbar_t.current_hotbar < slotOrder[0] ) // boundary checks if multiple face buttons pressed at one time
					{
						hotbar_t.selectHotbarSlot(slotOrder[0]);
					}
					else if ( hotbar_t.current_hotbar > slotOrder[2] )
					{
						hotbar_t.selectHotbarSlot(slotOrder[2]);
					}

					if ( Input::inputs[player].binaryToggle("HotbarFacebarModifierLeft") )
					{
						hotbar_t.selectHotbarSlot(std::max(centerSlot - 1, hotbar_t.current_hotbar - 1));
						Input::inputs[player].consumeBinaryToggle("HotbarFacebarModifierLeft");
						Player::soundHotbarShootmodeMovement();
					}
					else if ( Input::inputs[player].binaryToggle("HotbarFacebarModifierRight") )
					{
						hotbar_t.selectHotbarSlot(std::min(centerSlot + 1, hotbar_t.current_hotbar + 1));
						Input::inputs[player].consumeBinaryToggle("HotbarFacebarModifierRight");
						Player::soundHotbarShootmodeMovement();
					}
					else if ( players[player]->hotbar.faceMenuButtonHeld == Player::Hotbar_t::GROUP_NONE )
					{
						hotbar_t.selectHotbarSlot(centerSlot);
					}
					break;
				}
			}

			// using items
			if (pressed != Player::Hotbar_t::GROUP_NONE)
			{
				players[player]->hotbar.faceMenuButtonHeld = pressed;
			}
			else
			{
				if (players[player]->hotbar.faceMenuButtonHeld != Player::Hotbar_t::GROUP_NONE)
				{
					item = uidToItem(players[player]->hotbar.slots()[hotbar_t.current_hotbar].item);

					// quickcasting spells
					if (item && itemCategory(item) == SPELL_CAT )
					{
						spell_t* spell = getSpellFromItem(player, item, true);
						if ( spell && players[player]->magic.selectedSpell() == spell )
						{
							players[player]->hotbar.faceMenuQuickCast = true;
						}
					}
				}
				players[player]->hotbar.faceMenuButtonHeld = pressed;
			}

			Input::inputs[player].consumeBindingsSharedWithFaceHotbar();
		}
		else
		{
			players[player]->hotbar.faceMenuButtonHeld = Player::Hotbar_t::GROUP_NONE;
		}

		//Moving the cursor changes the currently selected hotbar slot.
		if ( (mousexrel || mouseyrel) 
			&& !shootmode
			&& !players[player]->GUI.isDropdownActive()
			&& players[player]->GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_HOTBAR) )
		{
			if ( hotbar_t.hotbarFrame )
			{
				for ( int c = 0; c < NUM_HOTBAR_SLOTS; ++c )
				{
					auto hotbarSlotFrame = hotbar_t.getHotbarSlotFrame(c);
					if ( hotbarSlotFrame && !hotbarSlotFrame->isDisabled() && hotbarSlotFrame->capturesMouseInRealtimeCoords() )
					{
						players[player]->hotbar.selectHotbarSlot(c);
					}
				}
			}
		}

		//Gamepad change hotbar selection.
		if ( !players[player]->hotbar.useHotbarFaceMenu && Input::inputs[player].binaryToggle("Hotbar Right") )
		{
			bool usingMouseWheel = false;
			const auto binding = Input::inputs[player].input("Hotbar Right");
			if ( binding.type == Input::binding_t::bindtype_t::MOUSE_BUTTON )
			{
				if ( binding.mouseButton == Input::MOUSE_WHEEL_DOWN || binding.mouseButton == Input::MOUSE_WHEEL_UP )
				{
					usingMouseWheel = true;
				}
			}
			if ( !usingMouseWheel )
			{
				Input::inputs[player].consumeBinaryToggle("Hotbar Right");
			}
			bool gamepadControl = Input::inputs[player].input("Hotbar Right").isBindingUsingGamepad();
			if ( gamepadControl && players[player]->hotbar.useHotbarFaceMenu )
			{
				// no action, gamepads can't scroll when useHotbarFaceMenu
			}
			else if ( shootmode && !players[player]->GUI.isDropdownActive() && !openedChest[player]
				&& gui_mode != (GUI_MODE_SHOP) 
				&& !players[player]->bookGUI.bBookOpen
				&& !players[player]->signGUI.bSignOpen
				&& !GenericGUI[player].isGUIOpen() )
			{
				if (hotbar_t.hotbarTooltipLastGameTick != ticks)
				{
					players[player]->hotbar.selectHotbarSlot(players[player]->hotbar.current_hotbar + 1);
					auto slotFrame = players[player]->hotbar.getHotbarSlotFrame(players[player]->hotbar.current_hotbar);
					if ( slotFrame && slotFrame->isDisabled() )
					{
						// skip this disabled one, move twice. e.g using facebar and 10th slot disabled
						players[player]->hotbar.selectHotbarSlot(players[player]->hotbar.current_hotbar + 1);
					}
					if ( gamepadControl )
					{
						warpMouseToSelectedHotbarSlot(player); // controller only functionality
					}
					hotbar_t.hotbarTooltipLastGameTick = ticks;
					//Player::soundHotbarShootmodeMovement();
				}
			}
			else
			{
				hotbar_t.hotbarTooltipLastGameTick = 0;
			}
		}
		if ( !players[player]->hotbar.useHotbarFaceMenu && Input::inputs[player].binaryToggle("Hotbar Left") )
		{
			bool usingMouseWheel = false;
			const auto binding = Input::inputs[player].input("Hotbar Left");
			if ( binding.type == Input::binding_t::bindtype_t::MOUSE_BUTTON )
			{
				if ( binding.mouseButton == Input::MOUSE_WHEEL_DOWN || binding.mouseButton == Input::MOUSE_WHEEL_UP )
				{
					usingMouseWheel = true;
				}
			}
			if ( !usingMouseWheel )
			{
				Input::inputs[player].consumeBinaryToggle("Hotbar Left");
			}
			bool gamepadControl = Input::inputs[player].input("Hotbar Left").isBindingUsingGamepad();
			if ( gamepadControl && players[player]->hotbar.useHotbarFaceMenu )
			{
				// no action, gamepads can't scroll when useHotbarFaceMenu
			}
			else if ( shootmode && !players[player]->GUI.isDropdownActive() && !openedChest[player]
				&& gui_mode != (GUI_MODE_SHOP) 
				&& !players[player]->bookGUI.bBookOpen
				&& !players[player]->signGUI.bSignOpen
				&& !GenericGUI[player].isGUIOpen() )
			{
				if (hotbar_t.hotbarTooltipLastGameTick != ticks)
				{
					players[player]->hotbar.selectHotbarSlot(players[player]->hotbar.current_hotbar - 1);
					auto slotFrame = players[player]->hotbar.getHotbarSlotFrame(players[player]->hotbar.current_hotbar);
					if ( slotFrame && slotFrame->isDisabled() )
					{
						// skip this disabled one, move twice. e.g using facebar and 10th slot disabled
						players[player]->hotbar.selectHotbarSlot(players[player]->hotbar.current_hotbar - 1);
					}
					if ( gamepadControl )
					{
						warpMouseToSelectedHotbarSlot(player); // controller only functionality
					}
					hotbar_t.hotbarTooltipLastGameTick = ticks;
					//Player::soundHotbarShootmodeMovement();
				}
			}
			else
			{
				hotbar_t.hotbarTooltipLastGameTick = 0;
			}
		}

		if ( !players[player]->GUI.isDropdownActive() && !inputs.getUIInteraction(player)->selectedItem && !openedChest[player] && gui_mode != (GUI_MODE_SHOP) )
		{
			if ( (!hotbar_t.useHotbarFaceMenu || (hotbar_t.useHotbarFaceMenu && !inputs.hasController(player)))
                && input.consumeBinaryToggle("Hotbar Up / Select") && shootmode
				&& !openedChest[player] && gui_mode != (GUI_MODE_SHOP)
				&& !players[player]->bookGUI.bBookOpen
				&& !players[player]->signGUI.bSignOpen
				&& !GenericGUI[player].isGUIOpen() )
			{
				//Show a tooltip
				hotbar_t.hotbarTooltipLastGameTick = std::max(ticks - TICKS_PER_SECOND, ticks - hotbar_t.hotbarTooltipLastGameTick);

				//Activate a hotbar slot if in-game.
				item = uidToItem(hotbar[hotbar_t.current_hotbar].item);
			}

			//if ( !shootmode && input.binaryToggle("HotbarInventoryClearSlot") && !players[player]->bookGUI.bBookOpen && !players[player]->signGUI.bSignOpen) //TODO: Don't activate if any of the previous if statement's conditions are true?
			//{
			//	//Clear a hotbar slot if in-inventory.
			//	input.consumeBinaryToggle("HotbarInventoryClearSlot");
			//	hotbar[hotbar_t.current_hotbar].item = 0;
			//}

			bool inventoryInteractable = players[player]->inventoryUI.isInteractable;
			if ( players[player]->inventoryUI.bCompactView && !inventoryInteractable )
			{
				inventoryInteractable = players[player]->GUI.activeModule == Player::GUI_t::MODULE_HOTBAR;
			}
			if ( !shootmode && inventoryInteractable 
				&& !players[player]->bookGUI.bBookOpen 
				&& !players[player]->signGUI.bSignOpen
				&& !openedChest[player]
				&& mouseInsidePlayerHotbar(player) )
			{
				if ( tooltipOpen
					&& !tooltipPromptFrameWasDisabled )
				{
					item = uidToItem(hotbar[hotbar_t.current_hotbar].item);
					auto contextTooltipOptions = getContextTooltipOptionsForItem(player, item, players[player]->inventoryUI.useItemDropdownOnGamepad, true);
					bool bindingPressed = false;
					for ( auto& option : contextTooltipOptions )
					{
						if ( option == ItemContextMenuPrompts::PROMPT_GRAB )
						{
							continue;
						}
						if ( Input::inputs[player].binaryToggle(getContextMenuOptionBindingName(player, option).c_str()) )
						{
							bindingPressed = true;

							if ( Input::inputs[player].getPlayerControlType() == Input::PLAYER_CONTROLLED_BY_KEYBOARD )
							{
								// rare bug causes rogue activations on keyboard controls holding space + opening inventory
								// skip this section and consume presses
								break;
							}

							if ( option == ItemContextMenuPrompts::PROMPT_DROP && players[player]->paperDoll.isItemOnDoll(*item) )
							{
								// need to unequip
								players[player]->inventoryUI.activateItemContextMenuOption(item, ItemContextMenuPrompts::PROMPT_UNEQUIP_FOR_DROP);
								players[player]->paperDoll.updateSlots();
								if ( players[player]->paperDoll.isItemOnDoll(*item) )
								{
									// couldn't unequip, no more actions
								}
								else
								{
									// successfully unequipped, let's drop it.
									bool droppedAll = false;
									while ( item && item->count > 1 )
									{
										droppedAll = dropItem(item, player);
										if ( droppedAll )
										{
											item = nullptr;
										}
									}
									if ( !droppedAll )
									{
										dropItem(item, player);
									}
								}
							}
							else
							{
								players[player]->inventoryUI.activateItemContextMenuOption(item, option);
								if ( option == ItemContextMenuPrompts::PROMPT_DROPDOWN && tooltipSlotFrame )
								{
									if ( !players[player]->GUI.isDropdownActive() )
									{
										players[player]->GUI.dropdownMenu.open("hotbar_interact");
										players[player]->GUI.dropdownMenu.dropDownToggleClick = true;
										players[player]->GUI.dropdownMenu.dropDownItem = hotbar[hotbar_t.current_hotbar].item;
										SDL_Rect dropdownPos = tooltipSlotFrame->getAbsoluteSize();
										dropdownPos.y += dropdownPos.h / 2;
										if ( auto interactMenuTop = players[player]->GUI.dropdownMenu.dropdownFrame->findImage("interact top background") )
										{
											// 10px is slot half height, move by 0.5 slots, minus the top interact text height
											dropdownPos.y -= (interactMenuTop->pos.h + (1 * 10) + 4);
										}

										if ( players[player]->GUI.dropdownMenu.getDropDownAlignRight("hotbar_interact") )
										{
											dropdownPos.x += dropdownPos.w - 10;
										}
										else
										{
											dropdownPos.x += 10;
										}
										dropdownPos.x = std::max(dropdownPos.x, players[player]->camera_virtualx1());
										dropdownPos.x = std::min(dropdownPos.x, players[player]->camera_virtualx1() + players[player]->camera_virtualWidth());
										players[player]->GUI.dropdownMenu.dropDownX = dropdownPos.x;
										dropdownPos.y = std::max(dropdownPos.y, players[player]->camera_virtualy1());
										dropdownPos.y = std::min(dropdownPos.y, players[player]->camera_virtualy1() + players[player]->camera_virtualHeight());
										players[player]->GUI.dropdownMenu.dropDownY = dropdownPos.y;
									}
								}
							}
						}
					}
					if ( bindingPressed )
					{
						for ( auto& option : contextTooltipOptions )
						{
							// clear the other bindings just in case.
							Input::inputs[player].consumeBinaryToggle(getContextMenuOptionBindingName(player, option).c_str());
						}
					}
					item = nullptr; // we don't need to use this item anymore
					hotbar_t.hotbarTooltipLastGameTick = 0; // hide tooltip on activation
					drawHotBarTooltipOnCycle = false;
				}
			}
		}

		if ( item )
		{
			hotbar_t.hotbarTooltipLastGameTick = 0; // hide tooltip on activation
			drawHotBarTooltipOnCycle = false;

			bool badpotion = false;
			bool learnedSpell = false;
			if ( itemCategory(item) == POTION )
			{
				badpotion = isPotionBad(*item);
			}

			if ( itemCategory(item) == SPELLBOOK && (item->identified || itemIsEquipped(item, player)) )
			{
				// equipped spellbook will unequip on use.
				learnedSpell = (playerLearnedSpellbook(player, item) || itemIsEquipped(item, player));
			}

			if ( ((keystatus[SDLK_LALT] || keystatus[SDLK_RALT])
				&& (itemCategory(item) == POTION || itemCategory(item) == SPELLBOOK)) 
				|| item->type == FOOD_CREAMPIE )
			{
				badpotion = true;
				learnedSpell = true;
			}

			if ( !learnedSpell && item->identified
				&& itemCategory(item) == SPELLBOOK && players[player] && players[player]->entity )
			{
				learnedSpell = true; // let's always equip/unequip spellbooks from the hotbar?
				spell_t* currentSpell = getSpellFromID(getSpellIDFromSpellbook(item->type));
				if ( currentSpell && stats[player] )
				{
					int skillLVL = stats[player]->getModifiedProficiency(PRO_MAGIC) + statGetINT(stats[player], players[player]->entity);
					if ( stats[player]->getModifiedProficiency(PRO_MAGIC) >= 100 )
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

			if ( itemCategory(item) == SPELLBOOK && stats[player] )
			{
				if ( stats[player]->type == GOBLIN || stats[player]->type == CREATURE_IMP
					|| (stats[player]->playerRace == RACE_GOBLIN && stats[player]->stat_appearance == 0) )
				{
					learnedSpell = true; // goblinos can't learn spells but always equip books.
				}
			}

			bool disableItemUsage = false;
			if ( players[player] && players[player]->entity )
			{
				if ( players[player]->entity->effectShapeshift != NOTHING )
				{
					if ( !item->usableWhileShapeshifted(stats[player]) )
					{
						disableItemUsage = true;
					}
				}
				else
				{
					if ( itemCategory(item) == SPELL_CAT && item->appearance >= 1000 )
					{
						if ( canUseShapeshiftSpellInCurrentForm(player, *item) != 1 )
						{
							disableItemUsage = true;
						}
					}
				}
			}
			if ( client_classes[player] == CLASS_SHAMAN )
			{
				if ( item->type == SPELL_ITEM && !(playerUnlockedShamanSpell(player, item)) )
				{
					disableItemUsage = true;
				}
			}

			if ( !disableItemUsage )
			{
				if ( !badpotion && !learnedSpell )
				{
					if ( !(isItemEquippableInShieldSlot(item) && cast_animation[player].active_spellbook) )
					{
						if ( stats[player] && stats[player]->type == AUTOMATON
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
								net_packet->data[25] = player;
								net_packet->address.host = net_server.host;
								net_packet->address.port = net_server.port;
								net_packet->len = 26;
								sendPacketSafe(net_sock, -1, net_packet, 0);
							}
							item_FoodAutomaton(item, player);
						}
						else
						{
							if ( itemCategory(item) == SPELLBOOK )
							{
								players[player]->magic.spellbookUidFromHotbarSlot = item->uid;
							}
							useItem(item, player);
							players[player]->magic.spellbookUidFromHotbarSlot = 0;
						}
					}
				}
				else
				{
					playerTryEquipItemAndUpdateServer(player, item, false);
				}
			}
			else
			{
				if ( client_classes[player] == CLASS_SHAMAN && item->type == SPELL_ITEM )
				{
					messagePlayer(player, MESSAGE_INVENTORY | MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(3488)); // unable to use with current level.
				}
				else
				{
					messagePlayer(player, MESSAGE_INVENTORY | MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(3432)); // unable to use in current form message.
				}
				playSoundPlayer(player, 90, 64);
			}
		}
	}

	if ( !FollowerMenu[player].followerFrame )
	{
		auto frame = gameUIFrame[player]->findFrame("follower");
		if ( !frame )
		{
			FollowerMenu[player].followerFrame = gameUIFrame[player]->addFrame("follower");
		}
		else
		{
			FollowerMenu[player].followerFrame = frame;
		}
		FollowerMenu[player].followerFrame->setHollow(true);
		FollowerMenu[player].followerFrame->setBorder(0);
		FollowerMenu[player].followerFrame->setOwner(player);
		FollowerMenu[player].followerFrame->setInheritParentFrameOpacity(false);
		FollowerMenu[player].followerFrame->setDisabled(true);
	}
	FollowerMenu[player].drawFollowerMenu();

	if ( !CalloutMenu[player].calloutFrame )
	{
		auto frame = gameUIFrame[player]->findFrame("callout");
		if ( !frame )
		{
			CalloutMenu[player].calloutFrame = gameUIFrame[player]->addFrame("callout");
		}
		else
		{
			CalloutMenu[player].calloutFrame = frame;
		}
		CalloutMenu[player].calloutFrame->setHollow(true);
		CalloutMenu[player].calloutFrame->setBorder(0);
		CalloutMenu[player].calloutFrame->setOwner(player);
		CalloutMenu[player].calloutFrame->setInheritParentFrameOpacity(false);
		CalloutMenu[player].calloutFrame->setDisabled(true);
	}
	CalloutMenu[player].drawCalloutMenu();
}