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
extern std::string shopspeech[MAXPLAYERS];
extern Item* sellitem[MAXPLAYERS];
extern int shopkeepertype[MAXPLAYERS];
extern std::string shopkeepername[MAXPLAYERS];
extern char shopkeepername_client[MAXPLAYERS][64];

void startTradingServer(Entity* entity, int player);
bool isItemSellableToShop(const int player, Item* item);
bool hideItemFromShopView(Item& item);
bool buyItemFromShop(const int player, Item* item, bool& bOutConsumedEntireStack);
bool sellItemToShop(const int player, Item* item);
bool shopIsMysteriousShopkeeper(Entity* entity);
extern std::unordered_map<int, std::unordered_set<int>> shopkeeperMysteriousItems;
void buyItemFromMysteriousShopkeepConsumeOrb(const int player, Entity& entity, Item& boughtItem);
void closeShop(const int player);
void shopChangeGoldEvent(const int player, Sint32 amount);

static const int NUM_SHOP_CATEGORIES = 8;
