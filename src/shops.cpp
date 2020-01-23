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
#include "scores.hpp"

list_t* shopInv = NULL;
Uint32 shopkeeper = 0;
Uint32 shoptimer = 0;
char* shopspeech = NULL;
int shopinventorycategory = 7;
int shopitemscroll;
Item* shopinvitems[4];
Item* sellitem = NULL;
int shopkeepertype = 0;
char* shopkeepername = NULL;
char shopkeepername_client[64];

int selectedShopSlot = -1;
std::unordered_map<int, std::unordered_set<int>> shopkeeperMysteriousItems(
{
	{ ARTIFACT_ORB_GREEN, { ARTIFACT_BOW, QUIVER_HUNTING, QUIVER_PIERCE } },
	{ ARTIFACT_ORB_BLUE, { ARTIFACT_MACE, ENCHANTED_FEATHER } },
	{ ARTIFACT_ORB_RED, { CRYSTAL_SWORD, CRYSTAL_BATTLEAXE, CRYSTAL_SPEAR, CRYSTAL_MACE } }
});

/*-------------------------------------------------------------------------------

	startTradingServer

	called on server, initiates trade sequence between player and NPC

-------------------------------------------------------------------------------*/

void startTradingServer(Entity* entity, int player)
{
	if (!entity)
	{
		return;
	}
	if ( multiplayer == CLIENT )
	{
		return;
	}
	if (!players[player] || !players[player]->entity)
	{
		return;
	}

	Stat* stats = entity->getStats();
	if ( stats == NULL )
	{
		return;
	}

	if ( player == 0 )
	{
		closeAllGUIs(DONT_CHANGE_SHOOTMODE, CLOSEGUI_DONT_CLOSE_SHOP);
		openStatusScreen(GUI_MODE_SHOP, INVENTORY_MODE_ITEM);
		shopInv = &stats->inventory;
		shopkeeper = entity->getUID();
		shoptimer = ticks - 1;
		shopspeech = language[194 + rand() % 3];
		shopinventorycategory = 7;
		sellitem = NULL;
		Entity* entity = uidToEntity(shopkeeper);
		shopkeepertype = entity->monsterStoreType;
		shopkeepername = stats->name;
		shopitemscroll = 0;

		//Initialize shop gamepad code here.
		if ( shopinvitems[0] != nullptr )
		{
			selectedShopSlot = 0;
			warpMouseToSelectedShopSlot();
		}
		else
		{
			selectedShopSlot = -1;
		}
	}
	else if ( multiplayer == SERVER )
	{
		// open shop on client
		Stat* entitystats = entity->getStats();
		strcpy((char*)net_packet->data, "SHOP");
		SDLNet_Write32((Uint32)entity->getUID(), &net_packet->data[4]);
		net_packet->data[8] = entity->monsterStoreType;
		strcpy((char*)(&net_packet->data[9]), entitystats->name);
		net_packet->data[9 + strlen(entitystats->name)] = 0;
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 9 + strlen(entitystats->name) + 1;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);

		// fill client's shop inventory with items
		node_t* node;
		for ( node = entitystats->inventory.first; node != NULL; node = node->next )
		{
			Item* item = (Item*)node->element;
			strcpy((char*)net_packet->data, "SHPI");
			SDLNet_Write32(item->type, &net_packet->data[4]);
			net_packet->data[8] = (char)item->status;
			net_packet->data[9] = (char)item->beatitude;
			net_packet->data[10] = (unsigned char)item->count;
			SDLNet_Write32((Uint32)item->appearance, &net_packet->data[11]);
			if ( item->identified )
			{
				net_packet->data[15] = 1;
			}
			else
			{
				net_packet->data[15] = 0;
			}
			net_packet->address.host = net_clients[player - 1].host;
			net_packet->address.port = net_clients[player - 1].port;
			net_packet->len = 16;
			sendPacketSafe(net_sock, -1, net_packet, player - 1);
		}
	}
	entity->skill[0] = 4; // talk state
	entity->skill[1] = players[player]->entity->getUID();
	messagePlayer(player, language[1122], stats->name);
}

/*-------------------------------------------------------------------------------

	buyItemFromShop

	buys the given item from the currently open shop

-------------------------------------------------------------------------------*/

void buyItemFromShop(Item* item)
{
	if ( !item )
	{
		return;
	}

	if ( stats[clientnum]->GOLD >= item->buyValue(clientnum) )
	{
		if ( items[item->type].value * 1.5 >= item->buyValue(clientnum) )
		{
			shopspeech = language[200 + rand() % 3];
		}
		else
		{
			shopspeech = language[197 + rand() % 3];
		}
		shoptimer = ticks - 1;
		Item* itemToPickup = newItem(item->type, item->status, item->beatitude, 1, item->appearance, item->identified, nullptr);
		if ( itemTypeIsQuiver(item->type) )
		{
			itemToPickup->count = item->count;
		}
		itemPickup(clientnum, itemToPickup);

		stats[clientnum]->GOLD -= item->buyValue(clientnum);

		if ( stats[clientnum]->playerRace > 0 && players[clientnum] && players[clientnum]->entity->effectPolymorph > NUMMONSTERS )
		{
			steamStatisticUpdate(STEAM_STAT_ALTER_EGO, STEAM_STAT_INT, item->buyValue(clientnum));
		}
		
		playSound(89, 64);
		int ocount = item->count;
		if ( !itemTypeIsQuiver(item->type) )
		{
			item->count = 1;
		}
		messagePlayer(clientnum, language[1123], item->description(), item->buyValue(clientnum));
		item->count = ocount;
		if ( multiplayer != CLIENT )
		{
			Entity* entity = uidToEntity(shopkeeper);
			if (entity)
			{
				Stat* shopstats = entity->getStats();
				shopstats->GOLD += item->buyValue(clientnum);
			}
			if ( rand() % 2 )
			{
				if ( item->buyValue(clientnum) <= 1 )
				{
					// buying cheap items does not increase trading past basic
					if ( stats[clientnum]->PROFICIENCIES[PRO_TRADING] < SKILL_LEVEL_SKILLED )
					{
						players[clientnum]->entity->increaseSkill(PRO_TRADING);
					}
				}
				else
				{
					players[clientnum]->entity->increaseSkill(PRO_TRADING);
				}
			}
			else if ( item->buyValue(clientnum) >= 150 )
			{
				if ( item->buyValue(clientnum) >= 300 || rand() % 2 )
				{
					players[clientnum]->entity->increaseSkill(PRO_TRADING);
				}
			}
		}
		else
		{
			strcpy((char*)net_packet->data, "SHPB");
			SDLNet_Write32(shopkeeper, &net_packet->data[4]);

			// send item that was bought to server
			SDLNet_Write32(item->type, &net_packet->data[8]);
			SDLNet_Write32(item->status, &net_packet->data[12]);
			SDLNet_Write32(item->beatitude, &net_packet->data[16]);
			SDLNet_Write32((Uint32)item->appearance, &net_packet->data[20]);
			if ( itemTypeIsQuiver(item->type) )
			{
				SDLNet_Write32((Uint32)item->count, &net_packet->data[24]);
			}
			else
			{
				SDLNet_Write32((Uint32)(1), &net_packet->data[24]);
			}
			if ( item->identified )
			{
				net_packet->data[28] = 1;
			}
			else
			{
				net_packet->data[28] = 0;
			}
			net_packet->data[29] = clientnum;
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 30;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
		if ( shopIsMysteriousShopkeeper(uidToEntity(shopkeeper)) )
		{
			buyItemFromMysteriousShopkeepConsumeOrb(*(uidToEntity(shopkeeper)), *item);
		}
		if ( itemTypeIsQuiver(item->type) )
		{
			item->count = 1; // so we consume it all up.
		}
		consumeItem(item, clientnum);
	}
	else
	{
		shopspeech = language[203 + rand() % 3];
		shoptimer = ticks - 1;
		playSound(90, 64);
	}
}

/*-------------------------------------------------------------------------------

	sellItemToShop

	sells the given item to the currently open shop

-------------------------------------------------------------------------------*/

void sellItemToShop(Item* item)
{
	if ( !item )
	{
		return;
	}
	if ( ((item->beatitude < 0 && !shouldInvertEquipmentBeatitude(stats[clientnum])) 
		|| (item->beatitude > 0 && shouldInvertEquipmentBeatitude(stats[clientnum]))) 
		&& itemIsEquipped(item, clientnum) )
	{
		if ( item->beatitude > 0 )
		{
			messagePlayer(clientnum, language[3219], item->getName());
		}
		else
		{
			messagePlayer(clientnum, language[1124], item->getName());
		}
		playSound(90, 64);
		return;
	}

	bool deal = true;
	if ( stats[clientnum]->PROFICIENCIES[PRO_TRADING] >= CAPSTONE_UNLOCK_LEVEL[PRO_TRADING] )
	{
		//Skill capstone: Can sell anything to any shop.
		if (shopkeepertype == 10)
		{
			deal = false;
		}
	}
	else
	{
		switch ( shopkeepertype )
		{
			case 0: // arms & armor
				if ( itemCategory(item) != WEAPON && itemCategory(item) != ARMOR && itemCategory(item) != THROWN )
				{
					deal = false;
				}
				break;
			case 1: // hats
				if ( itemCategory(item) != ARMOR )
				{
					deal = false;
				}
				break;
			case 2: // jewelry
				if ( itemCategory(item) != RING && itemCategory(item) != AMULET && itemCategory(item) != GEM )
				{
					deal = false;
				}
				break;
			case 3: // bookstore
				if ( itemCategory(item) != SPELLBOOK && itemCategory(item) != SCROLL && itemCategory(item) != BOOK )
				{
					deal = false;
				}
				break;
			case 4: // potion shop
				if ( item->type != TOOL_ALEMBIC && itemCategory(item) != POTION )
				{
					deal = false;
				}
				break;
			case 5: // magicstaffs
				if ( itemCategory(item) != MAGICSTAFF )
				{
					deal = false;
				}
				break;
			case 6: // food
				if ( itemCategory(item) != FOOD )
				{
					deal = false;
				}
				break;
			case 7: // tools
				if ( itemCategory(item) != TOOL && itemCategory(item) != THROWN )
				{
					deal = false;
				}
				break;
			case 8: // hunting
				if ( itemCategory(item) != WEAPON 
					&& itemCategory(item) != THROWN
					&& !itemTypeIsQuiver(item->type)
					&& item->type != BRASS_KNUCKLES
					&& item->type != IRON_KNUCKLES
					&& item->type != SPIKED_GAUNTLETS )
				{
					deal = false;
				}
				break;
			case 10:
				deal = false;
				break;
			default:
				break;
		}
	}

	if ( itemIsEquipped(item, clientnum) )
	{
		if ( itemCategory(item) == GEM
			|| itemCategory(item) == RING
			|| itemCategory(item) == AMULET )
		{
			shopspeech = language[3914];
		}
		else if ( itemCategory(item) == SPELLBOOK 
			|| itemCategory(item) == BOOK
			|| itemCategory(item) == SCROLL )
		{
			shopspeech = language[3915];
		}
		else if ( itemCategory(item) == WEAPON 
			|| itemCategory(item) == MAGICSTAFF
			|| itemCategory(item) == THROWN )
		{
			shopspeech = language[3911];
		}
		else if ( itemCategory(item) == ARMOR )
		{
			shopspeech = language[3910];
		}
		else if ( itemCategory(item) == TOOL )
		{
			shopspeech = language[3912];
		}
		else if ( itemCategory(item) == POTION )
		{
			shopspeech = language[3913];
		}
		else if ( itemCategory(item) == FOOD )
		{
			shopspeech = language[3916];
		}
		shoptimer = ticks - 1;
		playSound(90, 64);
		return;
	}

	if ( !deal )
	{
		shopspeech = language[212 + rand() % 3];
		shoptimer = ticks - 1;
		playSound(90, 64);
		return;
	}

	if ( items[item->type].value * .75 <= item->sellValue(clientnum) )
	{
		shopspeech = language[209 + rand() % 3];
	}
	else
	{
		shopspeech = language[206 + rand() % 3];
	}
	shoptimer = ticks - 1;
	Item* sold = newItem(item->type, item->status, item->beatitude, 1, item->appearance, item->identified, shopInv);
	if ( itemTypeIsQuiver(item->type) )
	{
		sold->count = item->count;
	}
	stats[clientnum]->GOLD += item->sellValue(clientnum);

	if ( stats[clientnum]->playerRace > 0 && players[clientnum] && players[clientnum]->entity->effectPolymorph > NUMMONSTERS )
	{
		steamStatisticUpdate(STEAM_STAT_ALTER_EGO, STEAM_STAT_INT, item->sellValue(clientnum));
	}

	playSound(89, 64);
	int ocount = item->count;
	if ( !itemTypeIsQuiver(item->type) )
	{
		item->count = 1;
	}
	messagePlayer(clientnum, language[1125], item->description(), item->sellValue(clientnum));
	item->count = ocount;
	if ( multiplayer != CLIENT )
	{
		if ( rand() % 2 )
		{
			if ( item->sellValue(clientnum) <= 1 )
			{
				// selling cheap items does not increase trading past basic
				if ( stats[clientnum]->PROFICIENCIES[PRO_TRADING] < SKILL_LEVEL_SKILLED )
				{
					players[clientnum]->entity->increaseSkill(PRO_TRADING);
				}
			}
			else
			{
				players[clientnum]->entity->increaseSkill(PRO_TRADING);
			}
		}
	}
	else
	{
		strcpy((char*)net_packet->data, "SHPS");
		SDLNet_Write32(shopkeeper, &net_packet->data[4]);

		// send item that was sold to server
		SDLNet_Write32(item->type, &net_packet->data[8]);
		SDLNet_Write32(item->status, &net_packet->data[12]);
		SDLNet_Write32(item->beatitude, &net_packet->data[16]);
		SDLNet_Write32((Uint32)item->appearance, &net_packet->data[20]);
		if ( itemTypeIsQuiver(item->type) )
		{
			SDLNet_Write32((Uint32)item->count, &net_packet->data[24]);
		}
		else
		{
			SDLNet_Write32((Uint32)(1), &net_packet->data[24]);
		}
		if ( item->identified )
		{
			net_packet->data[28] = 1;
		}
		else
		{
			net_packet->data[28] = 0;
		}
		net_packet->data[29] = clientnum;
		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		net_packet->len = 30;
		sendPacketSafe(net_sock, -1, net_packet, 0);
	}
	if ( itemTypeIsQuiver(item->type) )
	{
		item->count = 1; // so we consume it all up.
	}
	consumeItem(item, clientnum);
	sellitem = NULL;
}

bool shopIsMysteriousShopkeeper(Entity* entity)
{
	if ( !entity )
	{
		return false;
	}
	if ( entity->monsterStoreType == 10 )
	{
		return true;
	}
	return false;
}

void buyItemFromMysteriousShopkeepConsumeOrb(Entity& entity, Item& boughtItem)
{
	list_t* inventory = nullptr;
	if ( multiplayer == CLIENT )
	{
		inventory = shopInv;
	}
	else
	{
		if ( entity.getStats() )
		{
			inventory = &entity.getStats()->inventory;
		}
	}
	if ( inventory )
	{
		for ( auto orbCategories : shopkeeperMysteriousItems )
		{
			if ( orbCategories.second.find(boughtItem.type) != orbCategories.second.end() )
			{
				// item is part of an orb set. need to consume the orb.
				node_t* nextnode = nullptr;
				for ( node_t* node = inventory->first; node; node = nextnode )
				{
					nextnode = node->next;
					Item* orb = (Item*)node->element;
					if ( orb && orb->type == orbCategories.first )
					{
						consumeItem(orb, -1);
						break;
					}
				}
				break;
			}
		}
	}
}