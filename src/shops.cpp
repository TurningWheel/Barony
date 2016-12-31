/*-------------------------------------------------------------------------------

	BARONY
	File: shops.cpp
	Desc: support functions for shop (setup, some comm)

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "items.hpp"
#include "sound.hpp"
#include "shops.hpp"
#include "interface/interface.hpp"
#include "net.hpp"
#include "player.hpp"

list_t *shopInv = NULL;
Uint32 shopkeeper = 0;
Uint32 shoptimer = 0;
char *shopspeech = NULL;
int shopinventorycategory = 7;
int shopitemscroll;
Item *shopinvitems[4];
Item *sellitem=NULL;
int shopkeepertype=0;
char *shopkeepername=NULL;
char shopkeepername_client[64];

int selectedShopSlot = -1;

/*-------------------------------------------------------------------------------

	startTradingServer
	
	called on server, initiates trade sequence between player and NPC

-------------------------------------------------------------------------------*/

void startTradingServer(Entity *entity, int player) {
	if (!entity)
		return;
	if( multiplayer==CLIENT )
		return;
	if (!players[player] || !players[player]->entity)
		return;
	
	Stat *stats = entity->getStats();
	if( stats==NULL )
		return;
		
	if( player==0 ) {
		shootmode = FALSE;
		gui_mode = GUI_MODE_SHOP;
		shopInv = &stats->inventory;
		shopkeeper = entity->uid;
		shoptimer = ticks-1;
		shopspeech = language[194+rand()%3];
		shopinventorycategory = 7;
		sellitem = NULL;
		Entity *entity = uidToEntity(shopkeeper);
		shopkeepertype = entity->skill[18];
		shopkeepername = stats->name;
		shopitemscroll = 0;

		//Initialize shop gamepad code here.
		if ( shopinvitems[0] != nullptr ) {
			selectedShopSlot = 0;
			warpMouseToSelectedShopSlot();
		} else {
			selectedShopSlot = -1;
		}
	} else if( multiplayer==SERVER ) {
		// open shop on client
		Stat *entitystats = entity->getStats();
		strcpy((char *)net_packet->data,"SHOP");
		SDLNet_Write32((Uint32)entity->uid,&net_packet->data[4]);
		net_packet->data[8] = entity->skill[18];
		strcpy((char *)(&net_packet->data[9]),entitystats->name);
		net_packet->data[9+strlen(entitystats->name)] = 0;
		net_packet->address.host = net_clients[player-1].host;
		net_packet->address.port = net_clients[player-1].port;
		net_packet->len = 9+strlen(entitystats->name)+1;
		sendPacketSafe(net_sock, -1, net_packet, player-1);
		
		// fill client's shop inventory with items
		node_t *node;
		for( node=entitystats->inventory.first; node!=NULL; node=node->next ) {
			Item *item = (Item *)node->element;
			strcpy((char *)net_packet->data,"SHPI");
			SDLNet_Write32(item->type,&net_packet->data[4]);
			net_packet->data[8] = (char)item->status;
			net_packet->data[9] = (char)item->beatitude;
			net_packet->data[10] = (unsigned char)item->count;
			SDLNet_Write32((Uint32)item->appearance,&net_packet->data[11]);
			if( item->identified )
				net_packet->data[15] = 1;
			else
				net_packet->data[15] = 0;
			net_packet->address.host = net_clients[player-1].host;
			net_packet->address.port = net_clients[player-1].port;
			net_packet->len = 16;
			sendPacketSafe(net_sock, -1, net_packet, player-1);
		}
	}
	entity->skill[0] = 4; // talk state
	entity->skill[1] = players[player]->entity->uid;
	messagePlayer(player,language[1122],stats->name);
}

/*-------------------------------------------------------------------------------

	buyItemFromShop
	
	buys the given item from the currently open shop

-------------------------------------------------------------------------------*/

void buyItemFromShop(Item *item) {
	if( !item )
		return;

	if( stats[clientnum]->GOLD >= item->buyValue(clientnum) ) {
		if( items[item->type].value*1.5 >= item->buyValue(clientnum) )
			shopspeech = language[200+rand()%3];
		else
			shopspeech = language[197+rand()%3];
		shoptimer = ticks-1;
		newItem(item->type,item->status,item->beatitude,1,item->appearance,item->identified,&stats[clientnum]->inventory);
		stats[clientnum]->GOLD -= item->buyValue(clientnum);
		playSound(89,64);
		int ocount = item->count;
		item->count = 1;
		messagePlayer(clientnum,language[1123],item->description(),item->buyValue(clientnum));
		item->count = ocount;
		if( multiplayer != CLIENT ) {
			Entity *entity = uidToEntity(shopkeeper);
			if (entity) {
				Stat *shopstats = entity->getStats();
				shopstats->GOLD += item->buyValue(clientnum);
			}
			if( rand()%2 ) {
				players[clientnum]->entity->increaseSkill(PRO_TRADING);
			}
		} else {
			strcpy((char *)net_packet->data,"SHPB");
			SDLNet_Write32(shopkeeper,&net_packet->data[4]);
			
			// send item that was bought to server
			SDLNet_Write32(item->type,&net_packet->data[8]);
			SDLNet_Write32(item->status,&net_packet->data[12]);
			SDLNet_Write32(item->beatitude,&net_packet->data[16]);
			SDLNet_Write32((Uint32)item->appearance,&net_packet->data[20]);
			if( item->identified )
				net_packet->data[24] = 1;
			else
				net_packet->data[24] = 0;
			net_packet->data[25] = clientnum;
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 26;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
		consumeItem(item);
	} else {
		shopspeech = language[203+rand()%3];
		shoptimer = ticks-1;
		playSound(90,64);
	}
}

/*-------------------------------------------------------------------------------

	sellItemToShop
	
	sells the given item to the currently open shop

-------------------------------------------------------------------------------*/

void sellItemToShop(Item *item) {
	if( !item )
		return;
	if( item->beatitude < 0 && itemIsEquipped(item,clientnum) ) {
		messagePlayer(clientnum,language[1124],item->getName());
		playSound(90,64);
		return;
	}
		
	bool deal = TRUE;
	switch( shopkeepertype ) {
		case 0: // arms & armor
			if( itemCategory(item) != WEAPON && itemCategory(item) != ARMOR )
				deal = FALSE;
			break;
		case 1: // hats
			if( itemCategory(item) != ARMOR )
				deal = FALSE;
			break;
		case 2: // jewelry
			if( itemCategory(item) != RING && itemCategory(item) != AMULET && itemCategory(item) != GEM )
				deal = FALSE;
			break;
		case 3: // bookstore
			if( itemCategory(item) != SPELLBOOK && itemCategory(item) != SCROLL && itemCategory(item) != BOOK )
				deal = FALSE;
			break;
		case 4: // potion shop
			if( itemCategory(item) != POTION )
				deal = FALSE;
			break;
		case 5: // magicstaffs
			if( itemCategory(item) != MAGICSTAFF )
				deal = FALSE;
			break;
		case 6: // food
			if( itemCategory(item) != FOOD )
				deal = FALSE;
			break;
		case 7: // tools
		case 8: // lights
			if( itemCategory(item) != TOOL )
				deal = FALSE;
			break;
		default:
			break;
	}
	if( !deal ) {
		shopspeech = language[212+rand()%3];
		shoptimer = ticks-1;
		playSound(90,64);
		return;
	}
	
	if( items[item->type].value*.75 <= item->sellValue(clientnum) )
		shopspeech = language[209+rand()%3];
	else
		shopspeech = language[206+rand()%3];
	shoptimer = ticks-1;
	newItem(item->type,item->status,item->beatitude,1,item->appearance,item->identified,shopInv);
	stats[clientnum]->GOLD += item->sellValue(clientnum);
	playSound(89,64);
	int ocount = item->count;
	item->count = 1;
	messagePlayer(clientnum,language[1125],item->description(),item->sellValue(clientnum));
	item->count = ocount;
	if( multiplayer != CLIENT ) {
		if( rand()%2 ) {
			players[clientnum]->entity->increaseSkill(PRO_TRADING);
		}
	} else {
		strcpy((char *)net_packet->data,"SHPS");
		SDLNet_Write32(shopkeeper,&net_packet->data[4]);
		
		// send item that was sold to server
		SDLNet_Write32(item->type,&net_packet->data[8]);
		SDLNet_Write32(item->status,&net_packet->data[12]);
		SDLNet_Write32(item->beatitude,&net_packet->data[16]);
		SDLNet_Write32((Uint32)item->appearance,&net_packet->data[20]);
		if( item->identified )
			net_packet->data[24] = 1;
		else
			net_packet->data[24] = 0;
		net_packet->data[25] = clientnum;
		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		net_packet->len = 26;
		sendPacketSafe(net_sock, -1, net_packet, 0);
	}
	consumeItem(item);
	sellitem = NULL;
}
