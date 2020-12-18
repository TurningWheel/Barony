/*-------------------------------------------------------------------------------

	BARONY
	File: shops.hpp
	Desc: contains declarations for shop.cpp

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#include "main.hpp"
#include "game.hpp"
#include "items.hpp"

#define NUMCHITCHAT 20

extern list_t* shopInv[MAXPLAYERS];
extern Uint32 shopkeeper[MAXPLAYERS];
extern Uint32 shoptimer[MAXPLAYERS];
extern char* shopspeech[MAXPLAYERS];
extern int shopinventorycategory[MAXPLAYERS];
extern int shopitemscroll[MAXPLAYERS];
static const int NUM_SHOP_GUI_SLOTS = 4;
extern Item* shopinvitems[MAXPLAYERS][NUM_SHOP_GUI_SLOTS];
extern Item* sellitem[MAXPLAYERS];
extern int shopkeepertype[MAXPLAYERS];
extern char* shopkeepername[MAXPLAYERS];
extern char shopkeepername_client[MAXPLAYERS][64];

void startTradingServer(Entity* entity, int player);
void buyItemFromShop(const int player, Item* item);
void sellItemToShop(const int player, Item* item);
bool shopIsMysteriousShopkeeper(Entity* entity);
extern int selectedShopSlot[MAXPLAYERS];
extern std::unordered_map<int, std::unordered_set<int>> shopkeeperMysteriousItems;
void buyItemFromMysteriousShopkeepConsumeOrb(const int player, Entity& entity, Item& boughtItem);
void selectShopSlot(const int player, int slot);
void warpMouseToSelectedShopSlot(const int player);
void closeShop(const int player);

/*
 * Negative: Left.
 * Positive: Right.
 */
void cycleShopCategories(const int player, int direction);

static const int NUM_SHOP_CATEGORIES = 8;
