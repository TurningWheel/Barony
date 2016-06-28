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

extern list_t *shopInv;
extern Uint32 shopkeeper;
extern Uint32 shoptimer;
extern char *shopspeech;
extern int shopinventorycategory;
extern int shopitemscroll;
extern Item *shopinvitems[4];
extern Item *sellitem;
extern int shopkeepertype;
extern char *shopkeepername;
extern char shopkeepername_client[64];

void startTradingServer(Entity *entity, int player);
void buyItemFromShop(Item *item);
void sellItemToShop(Item *item);
