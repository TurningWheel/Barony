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
#include "engine/audio/sound.hpp"
#include "shops.hpp"
#include "interface/interface.hpp"
#include "net.hpp"
#include "player.hpp"
#include "scores.hpp"

list_t* shopInv[MAXPLAYERS] = { nullptr };
Uint32 shopkeeper[MAXPLAYERS] = { 0 };
Uint32 shoptimer[MAXPLAYERS] = { 0 };
char* shopspeech[MAXPLAYERS] = { nullptr };
int shopinventorycategory[MAXPLAYERS];
int shopitemscroll[MAXPLAYERS] = { 0 };
Item* shopinvitems[MAXPLAYERS][NUM_SHOP_GUI_SLOTS];
Item* sellitem[MAXPLAYERS] = { nullptr };
int shopkeepertype[MAXPLAYERS] = { 0 };
char* shopkeepername[MAXPLAYERS] = { nullptr };
char shopkeepername_client[MAXPLAYERS][64];

int selectedShopSlot[MAXPLAYERS];
std::unordered_map<int, std::unordered_set<int>> shopkeeperMysteriousItems(
{
	{ ARTIFACT_ORB_GREEN, { ARTIFACT_BOW, QUIVER_HUNTING, QUIVER_PIERCE } },
	{ ARTIFACT_ORB_BLUE, { ARTIFACT_MACE, ENCHANTED_FEATHER } },
	{ ARTIFACT_ORB_RED, { CRYSTAL_SWORD, CRYSTAL_BATTLEAXE, CRYSTAL_SPEAR, CRYSTAL_MACE } }
});

void closeShop(const int player)
{
	if ( multiplayer == CLIENT )
	{
		list_FreeAll(shopInv[player]);
		shopInv[player]->first = nullptr;
		shopInv[player]->last = nullptr;
	}
	else
	{
		shopInv[player] = nullptr;
	}
	shopkeeper[player] = 0;

	//Clean up shopkeeper gamepad code here.
	selectedShopSlot[player] = -1;
}

/*-------------------------------------------------------------------------------

	startTradingServer

	called on server, initiates trade sequence between player and NPC

-------------------------------------------------------------------------------*/

void startTradingServer(Entity* entity, int player)
{
	if (!entity || player < 0)
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

	if ( players[player]->isLocalPlayer() )
	{
		players[player]->closeAllGUIs(DONT_CHANGE_SHOOTMODE, CLOSEGUI_DONT_CLOSE_SHOP);
		players[player]->openStatusScreen(GUI_MODE_SHOP, INVENTORY_MODE_ITEM);
		shopInv[player] = &stats->inventory;
		shopkeeper[player] = entity->getUID();
		shoptimer[player] = ticks - 1;
		shopspeech[player] = language[194 + rand() % 3];
		shopinventorycategory[player] = 7;
		sellitem[player] = NULL;
		Entity* entity = uidToEntity(shopkeeper[player]);
		if ( entity )
		{
			shopkeepertype[player] = entity->monsterStoreType;
		}
		shopkeepername[player] = stats->name;
		shopitemscroll[player] = 0;

		//Initialize shop gamepad code here.
		if ( shopinvitems[player][0] != nullptr )
		{
			selectedShopSlot[player] = 0;
			warpMouseToSelectedShopSlot(player);
		}
		else
		{
			selectedShopSlot[player] = -1;
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
	messagePlayer(player, MESSAGE_HINT, language[1122], stats->name);
}

/*-------------------------------------------------------------------------------

	buyItemFromShop

	buys the given item from the currently open shop

-------------------------------------------------------------------------------*/

void buyItemFromShop(const int player, Item* item)
{
	if ( !item )
	{
		return;
	}

	if ( stats[player]->GOLD >= item->buyValue(player) )
	{
		if ( items[item->type].value * 1.5 >= item->buyValue(player) )
		{
			shopspeech[player] = language[200 + rand() % 3];
		}
		else
		{
			shopspeech[player] = language[197 + rand() % 3];
		}
		shoptimer[player] = ticks - 1;
		Item* itemToPickup = newItem(item->type, item->status, item->beatitude, 1, item->appearance, item->identified, nullptr);
		if ( itemTypeIsQuiver(item->type) )
		{
			itemToPickup->count = item->count;
		}
		itemPickup(player, itemToPickup);

		stats[player]->GOLD -= item->buyValue(player);

		if ( stats[player]->playerRace > 0 && players[player] && players[player]->entity->effectPolymorph > NUMMONSTERS )
		{
			steamStatisticUpdate(STEAM_STAT_ALTER_EGO, STEAM_STAT_INT, item->buyValue(player));
		}
		
		playSound(89, 64);
		int ocount = item->count;
		if ( !itemTypeIsQuiver(item->type) )
		{
			item->count = 1;
		}
		messagePlayer(player, MESSAGE_INVENTORY, language[1123], item->description(), item->buyValue(player));
		item->count = ocount;
		if ( multiplayer != CLIENT )
		{
			Entity* entity = uidToEntity(shopkeeper[player]);
			if (entity)
			{
				Stat* shopstats = entity->getStats();
				shopstats->GOLD += item->buyValue(player);
			}

			if ( players[player] && players[player]->entity )
			{
				if ( rand() % 2 )
				{
					if ( item->buyValue(player) <= 1 )
					{
						// buying cheap items does not increase trading past basic
						if ( stats[player]->PROFICIENCIES[PRO_TRADING] < SKILL_LEVEL_SKILLED )
						{
							players[player]->entity->increaseSkill(PRO_TRADING);
						}
					}
					else
					{
						players[player]->entity->increaseSkill(PRO_TRADING);
					}
				}
				else if ( item->buyValue(player) >= 150 )
				{
					if ( item->buyValue(player) >= 300 || rand() % 2 )
					{
						players[player]->entity->increaseSkill(PRO_TRADING);
					}
				}
			}
		}
		else if ( multiplayer == CLIENT )
		{
			strcpy((char*)net_packet->data, "SHPB");
			SDLNet_Write32(shopkeeper[player], &net_packet->data[4]);

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
			net_packet->data[29] = player;
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 30;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
		if ( shopIsMysteriousShopkeeper(uidToEntity(shopkeeper[player])) )
		{
			buyItemFromMysteriousShopkeepConsumeOrb(player, *(uidToEntity(shopkeeper[player])), *item);
		}
		if ( itemTypeIsQuiver(item->type) )
		{
			item->count = 1; // so we consume it all up.
		}
		consumeItem(item, player);
	}
	else
	{
		shopspeech[player] = language[203 + rand() % 3];
		shoptimer[player] = ticks - 1;
		playSound(90, 64);
	}
}

/*-------------------------------------------------------------------------------

	sellItemToShop

	sells the given item to the currently open shop

-------------------------------------------------------------------------------*/

void sellItemToShop(const int player, Item* item)
{
	if ( !item )
	{
		return;
	}
	if ( ((item->beatitude < 0 && !shouldInvertEquipmentBeatitude(stats[player]))
		|| (item->beatitude > 0 && shouldInvertEquipmentBeatitude(stats[player])))
		&& itemIsEquipped(item, player) )
	{
		if ( item->beatitude > 0 )
		{
			messagePlayer(player, MESSAGE_INVENTORY | MESSAGE_HINT, language[3219], item->getName());
		}
		else
		{
			messagePlayer(player, MESSAGE_INVENTORY | MESSAGE_HINT, language[1124], item->getName());
		}
		playSound(90, 64);
		return;
	}

	bool deal = true;
	if ( stats[player]->PROFICIENCIES[PRO_TRADING] >= CAPSTONE_UNLOCK_LEVEL[PRO_TRADING] )
	{
		//Skill capstone: Can sell anything to any shop.
		if (shopkeepertype[player] == 10)
		{
			deal = false;
		}
	}
	else
	{
		switch ( shopkeepertype[player] )
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

	if ( itemIsEquipped(item, player) )
	{
		if ( itemCategory(item) == GEM
			|| itemCategory(item) == RING
			|| itemCategory(item) == AMULET )
		{
			shopspeech[player] = language[3914];
		}
		else if ( itemCategory(item) == SPELLBOOK 
			|| itemCategory(item) == BOOK
			|| itemCategory(item) == SCROLL )
		{
			shopspeech[player] = language[3915];
		}
		else if ( itemCategory(item) == WEAPON 
			|| itemCategory(item) == MAGICSTAFF
			|| itemCategory(item) == THROWN )
		{
			shopspeech[player] = language[3911];
		}
		else if ( itemCategory(item) == ARMOR )
		{
			shopspeech[player] = language[3910];
		}
		else if ( itemCategory(item) == TOOL )
		{
			shopspeech[player] = language[3912];
		}
		else if ( itemCategory(item) == POTION )
		{
			shopspeech[player] = language[3913];
		}
		else if ( itemCategory(item) == FOOD )
		{
			shopspeech[player] = language[3916];
		}
		shoptimer[player] = ticks - 1;
		playSound(90, 64);
		return;
	}

	if ( !deal )
	{
		shopspeech[player] = language[212 + rand() % 3];
		shoptimer[player] = ticks - 1;
		playSound(90, 64);
		return;
	}

	if ( items[item->type].value * .75 <= item->sellValue(player) )
	{
		shopspeech[player] = language[209 + rand() % 3];
	}
	else
	{
		shopspeech[player] = language[206 + rand() % 3];
	}
	shoptimer[player] = ticks - 1;
	Item* sold = newItem(item->type, item->status, item->beatitude, 1, item->appearance, item->identified, shopInv[player]);
	if ( itemTypeIsQuiver(item->type) )
	{
		sold->count = item->count;
	}
	stats[player]->GOLD += item->sellValue(player);

	if ( stats[player]->playerRace > 0 && players[player] && players[player]->entity->effectPolymorph > NUMMONSTERS )
	{
		steamStatisticUpdate(STEAM_STAT_ALTER_EGO, STEAM_STAT_INT, item->sellValue(player));
	}

	playSound(89, 64);
	int ocount = item->count;
	if ( !itemTypeIsQuiver(item->type) )
	{
		item->count = 1;
	}
	messagePlayer(player, MESSAGE_INVENTORY, language[1125], item->description(), item->sellValue(player));
	item->count = ocount;
	if ( multiplayer != CLIENT )
	{
		if ( players[player] && players[player]->entity )
		{
			if ( rand() % 2 )
			{
				if ( item->sellValue(player) <= 1 )
				{
					// selling cheap items does not increase trading past basic
					if ( stats[player]->PROFICIENCIES[PRO_TRADING] < SKILL_LEVEL_SKILLED )
					{
						players[player]->entity->increaseSkill(PRO_TRADING);
					}
				}
				else
				{
					players[player]->entity->increaseSkill(PRO_TRADING);
				}
			}
		}
	}
	else
	{
		strcpy((char*)net_packet->data, "SHPS");
		SDLNet_Write32(shopkeeper[player], &net_packet->data[4]);

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
		net_packet->data[29] = player;
		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		net_packet->len = 30;
		sendPacketSafe(net_sock, -1, net_packet, 0);
	}
	if ( itemTypeIsQuiver(item->type) )
	{
		item->count = 1; // so we consume it all up.
	}
	consumeItem(item, player);
	sellitem[player] = NULL;
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

void buyItemFromMysteriousShopkeepConsumeOrb(const int player, Entity& entity, Item& boughtItem)
{
	list_t* inventory = nullptr;
	if ( multiplayer == CLIENT )
	{
		inventory = shopInv[player];
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
