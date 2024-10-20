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
#include "prng.hpp"
#include "mod_tools.hpp"

list_t* shopInv[MAXPLAYERS] = { nullptr };
Uint32 shopkeeper[MAXPLAYERS] = { 0 };
Uint32 shoptimer[MAXPLAYERS] = { 0 };
std::string shopspeech[MAXPLAYERS] = { "" };
int shopkeepertype[MAXPLAYERS] = { 0 };
std::string shopkeepername[MAXPLAYERS] = { "" };
char shopkeepername_client[MAXPLAYERS][64];

std::unordered_map<int, std::unordered_set<int>> shopkeeperMysteriousItems(
{
	{ ARTIFACT_ORB_GREEN, { ARTIFACT_BOW, QUIVER_HUNTING, QUIVER_PIERCE } },
	{ ARTIFACT_ORB_BLUE, { ARTIFACT_MACE, ENCHANTED_FEATHER } },
	{ ARTIFACT_ORB_RED, { CRYSTAL_SWORD, CRYSTAL_BATTLEAXE, CRYSTAL_SPEAR, CRYSTAL_MACE, MASK_ARTIFACT_VISOR } }
});

void closeShop(const int player)
{
	if ( shopkeeper[player] != 0 )
	{
		if ( multiplayer != CLIENT )
		{
			Entity* entity = uidToEntity(shopkeeper[player]);
			if ( entity )
			{
				entity->skill[0] = 0;
				if ( uidToEntity(entity->skill[1]) )
				{
					monsterMoveAside(entity, uidToEntity(entity->skill[1]));
				}
				entity->skill[1] = 0;
			}
		}
		else
		{
			// inform server that we're done talking to shopkeeper
			strcpy((char*)net_packet->data, "SHPC");
			SDLNet_Write32((Uint32)shopkeeper[player], &net_packet->data[4]);
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 8;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
	}

	if ( multiplayer == CLIENT && players[player]->isLocalPlayer() )
	{
		if ( shopInv[player] )
		{
			list_FreeAll(shopInv[player]);
			shopInv[player]->first = nullptr;
			shopInv[player]->last = nullptr;
		}
	}
	else
	{
		shopInv[player] = nullptr;
	}
	shopkeeper[player] = 0;

	//Clean up shopkeeper gamepad code here.
	players[player]->shopGUI.closeShop();
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
		players[player]->closeAllGUIs(DONT_CHANGE_SHOOTMODE, CLOSEGUI_DONT_CLOSE_INVENTORY);
		players[player]->openStatusScreen(GUI_MODE_SHOP, INVENTORY_MODE_ITEM, Player::GUI_t::MODULE_SHOP);

		shopInv[player] = &stats->inventory;
		shopkeeper[player] = entity->getUID();
		shoptimer[player] = ticks - 1;
		shopspeech[player] = Language::get(194 + local_rng.rand() % 3);
		shopkeepertype[player] = entity->monsterStoreType;
		shopkeepername[player] = stats->name;

		players[player]->shopGUI.openShop();
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
			net_packet->data[8] = (Sint8)item->status;
			net_packet->data[9] = (Sint8)item->beatitude;
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
			if ( item->playerSoldItemToShop )
			{
				net_packet->data[15] |= (1 << 1);
			}
			if ( item->itemSpecialShopConsumable )
			{
				net_packet->data[15] |= (1 << 2);
			}
			net_packet->data[15] |= ((0xF & item->itemRequireTradingSkillInShop) << 4);
			net_packet->data[16] = (Sint8)item->x;
			net_packet->data[17] = (Sint8)item->y;
			net_packet->address.host = net_clients[player - 1].host;
			net_packet->address.port = net_clients[player - 1].port;
			net_packet->len = 18;
			sendPacketSafe(net_sock, -1, net_packet, player - 1);
		}
	}
	entity->skill[0] = 4; // talk state
	entity->skill[1] = players[player]->entity->getUID();
	messagePlayer(player, MESSAGE_HINT, Language::get(1122), stats->name);
}

/*-------------------------------------------------------------------------------

	buyItemFromShop

	buys the given item from the currently open shop

-------------------------------------------------------------------------------*/

bool buyItemFromShop(const int player, Item* item, bool& bOutConsumedEntireStack)
{
	bOutConsumedEntireStack = false;
	if ( !item )
	{
		return false;
	}

	if ( players[player]->shopGUI.itemUnknownPreventPurchase )
	{
		shopspeech[player] = Language::get(4252 + local_rng.rand() % 3);
		shoptimer[player] = ticks - 1;
		playSound(90, 64);
	}
	else if ( stats[player]->GOLD >= item->buyValue(player) )
	{
		if ( item->itemSpecialShopConsumable )
		{
			shopspeech[player] = Language::get(4255 + local_rng.rand() % 5);
		}
		else if ( items[item->type].value * 1.5 >= item->buyValue(player) )
		{
			shopspeech[player] = Language::get(200 + local_rng.rand() % 3);
		}
		else
		{
			shopspeech[player] = Language::get(197 + local_rng.rand() % 3);
		}
		shoptimer[player] = ticks - 1;
		Item* itemToPickup = newItem(item->type, item->status, item->beatitude, 1, item->appearance, item->identified, nullptr);
		if ( itemTypeIsQuiver(item->type) )
		{
			itemToPickup->count = item->count;
		}
		itemPickup(player, itemToPickup);

		shopChangeGoldEvent(player, -item->buyValue(player));
		stats[player]->GOLD -= item->buyValue(player);

		Entity* entity = uidToEntity(shopkeeper[player]);
		if ( shopIsMysteriousShopkeeper(entity) )
		{
			Compendium_t::Events_t::eventUpdateMonster(player, Compendium_t::CPDM_SHOP_BOUGHT, entity, 1);
			Compendium_t::Events_t::eventUpdateMonster(player, Compendium_t::CPDM_SHOP_SPENT, entity, item->buyValue(player));
		}
		else
		{
			Compendium_t::Events_t::eventUpdateWorld(player, Compendium_t::CPDM_SHOP_BOUGHT, "shop", 1);
			Compendium_t::Events_t::eventUpdateWorld(player, Compendium_t::CPDM_SHOP_SPENT, "shop", item->buyValue(player));
		}

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
		messagePlayer(player, MESSAGE_INVENTORY, Language::get(1123), item->description(), item->buyValue(player));
		item->count = ocount;
		if ( multiplayer != CLIENT )
		{
			if (entity)
			{
				Stat* shopstats = entity->getStats();
				shopstats->GOLD += item->buyValue(player);
			}

			if ( players[player] && players[player]->entity && !item->playerSoldItemToShop )
			{
				bool increaseSkill = false;
				int buyValue = item->buyValue(player);
				if ( buyValue >= 100 )
				{
					increaseSkill = true;
				}
				else
				{
					if ( rand() % 100 <= (std::max(10, buyValue)) ) // 10% to 100% from 1-100 gold
					{
						increaseSkill = true;
					}
				}

				if ( increaseSkill )
				{
					if ( buyValue <= 1 )
					{
						if ( stats[player]->getProficiency(PRO_TRADING) < SKILL_LEVEL_SKILLED )
						{
							players[player]->entity->increaseSkill(PRO_TRADING);
						}
					}
					else
					{
						players[player]->entity->increaseSkill(PRO_TRADING);
					}
				}
				//if ( local_rng.rand() % 2 )
				//{
				//	if ( item->buyValue(player) <= 1 )
				//	{
				//		// buying cheap items does not increase trading past basic
				//		if ( stats[player]->PROFICIENCIES[PRO_TRADING] < SKILL_LEVEL_SKILLED )
				//		{
				//			players[player]->entity->increaseSkill(PRO_TRADING);
				//		}
				//	}
				//	else
				//	{
				//		players[player]->entity->increaseSkill(PRO_TRADING);
				//	}
				//}
				//else if ( item->buyValue(player) >= 150 )
				//{
				//	if ( item->buyValue(player) >= 300 || local_rng.rand() % 2 )
				//	{
				//		players[player]->entity->increaseSkill(PRO_TRADING);
				//	}
				//}
			}
		}
		else if ( multiplayer == CLIENT )
		{
			strcpy((char*)net_packet->data, "SHPB");
			SDLNet_Write32(shopkeeper[player], &net_packet->data[4]);

			// send item that was bought to server
			SDLNet_Write32(item->type, &net_packet->data[8]);
			SDLNet_Write32(item->status, &net_packet->data[12]);
			SDLNet_Write16(item->beatitude, &net_packet->data[16]);
			net_packet->data[18] = (Sint8)item->x;
			net_packet->data[19] = (Sint8)item->y;
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
			if ( item->playerSoldItemToShop )
			{
				net_packet->data[28] |= (1 << 4);
			}
			net_packet->data[29] = player;
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 30;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
		if ( shopIsMysteriousShopkeeper(entity) )
		{
			buyItemFromMysteriousShopkeepConsumeOrb(player, *entity, *item);
		}
		if ( itemTypeIsQuiver(item->type) )
		{
			item->count = 1; // so we consume it all up.
		}
		if ( item->count > 1 )
		{
			bOutConsumedEntireStack = false;
		}
		else
		{
			bOutConsumedEntireStack = true;
		}
		consumeItem(item, player);
		return true;
	}
	else
	{
		shopspeech[player] = Language::get(203 + local_rng.rand() % 3);
		shoptimer[player] = ticks - 1;
		playSound(90, 64);
		if ( players[player]->isLocalPlayer() )
		{
			players[player]->shopGUI.animNoDeal = 1.0;
			players[player]->shopGUI.animNoDealTicks = ticks;
		}
	}
	return false;
}

/*-------------------------------------------------------------------------------

	sellItemToShop

	sells the given item to the currently open shop

-------------------------------------------------------------------------------*/

bool isItemSellableToShop(const int player, Item* item)
{
	if ( !item )
	{
		return false;
	}

	if ( items[item->type].hasAttribute("UNSELLABLE") )
	{
		return false;
	}

	bool deal = true;
	if ( stats[player]->getModifiedProficiency(PRO_TRADING) >= CAPSTONE_UNLOCK_LEVEL[PRO_TRADING] )
	{
		//Skill capstone: Can sell anything to any shop.
		if ( shopkeepertype[player] == 10 )
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
	return deal;
}

bool sellItemToShop(const int player, Item* item)
{
	if ( !item )
	{
		return false;
	}

	bool deal = isItemSellableToShop(player, item);

	if ( !deal )
	{
		if ( shopkeepertype[player] == 10 && 
			(item->type == ARTIFACT_ORB_BLUE
			|| item->type == ARTIFACT_ORB_GREEN
			|| item->type == ARTIFACT_ORB_RED) )
		{
			shopspeech[player] = Language::get(4126);
		}
		else
		{
			shopspeech[player] = Language::get(212 + local_rng.rand() % 3);
		}
		shoptimer[player] = ticks - 1;
		playSound(90, 64);
		return false;
	}

	if ( ((item->beatitude < 0 && !shouldInvertEquipmentBeatitude(stats[player]))
		|| (item->beatitude > 0 && shouldInvertEquipmentBeatitude(stats[player])))
		&& itemIsEquipped(item, player) )
	{
		if ( item->beatitude > 0 )
		{
			messagePlayer(player, MESSAGE_INVENTORY, Language::get(3219), item->getName());
		}
		else
		{
			messagePlayer(player, MESSAGE_INVENTORY, Language::get(1124), item->getName());
		}
	}

	int xout = Player::ShopGUI_t::MAX_SHOP_X;
	int yout = Player::ShopGUI_t::MAX_SHOP_Y;
	Item* itemToStackInto = nullptr;
	if ( !getShopFreeSlot(player, nullptr, item, xout, yout, itemToStackInto) )
	{
		shopspeech[player] = Language::get(4125);
		shoptimer[player] = ticks - 1;
		playSound(90, 64);
		return false;
	}

	if ( itemIsEquipped(item, player) )
	{
		if ( itemCategory(item) == GEM
			|| itemCategory(item) == RING
			|| itemCategory(item) == AMULET )
		{
			shopspeech[player] = Language::get(3914);
		}
		else if ( itemCategory(item) == SPELLBOOK 
			|| itemCategory(item) == BOOK
			|| itemCategory(item) == SCROLL )
		{
			shopspeech[player] = Language::get(3915);
		}
		else if ( itemCategory(item) == WEAPON 
			|| itemCategory(item) == MAGICSTAFF
			|| itemCategory(item) == THROWN )
		{
			shopspeech[player] = Language::get(3911);
		}
		else if ( itemCategory(item) == ARMOR )
		{
			shopspeech[player] = Language::get(3910);
		}
		else if ( itemCategory(item) == TOOL )
		{
			shopspeech[player] = Language::get(3912);
		}
		else if ( itemCategory(item) == POTION )
		{
			shopspeech[player] = Language::get(3913);
		}
		else if ( itemCategory(item) == FOOD )
		{
			shopspeech[player] = Language::get(3916);
		}
		shoptimer[player] = ticks - 1;
		playSound(90, 64);
		return false;
	}

	if ( items[item->type].value * .75 <= item->sellValue(player) )
	{
		shopspeech[player] = Language::get(209 + local_rng.rand() % 3);
	}
	else
	{
		shopspeech[player] = Language::get(206 + local_rng.rand() % 3);
	}
	shoptimer[player] = ticks - 1;

	if ( itemToStackInto != nullptr )
	{
		int addQty = 1;
		if ( itemTypeIsQuiver(item->type) )
		{
			addQty = item->count;
		}
		itemToStackInto->count += addQty;
	}
	else
	{
		Item* sold = newItem(item->type, item->status, item->beatitude, 1, item->appearance, item->identified, shopInv[player]);
		if ( itemTypeIsQuiver(item->type) )
		{
			sold->count = item->count;
		}
		sold->playerSoldItemToShop = true;
		sold->x = xout;
		sold->y = yout;
	}

	shopChangeGoldEvent(player, item->sellValue(player));
	stats[player]->GOLD += item->sellValue(player);

	if ( players[player]->isLocalPlayer() )
	{
		Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_TRADING_GOLD_EARNED, item->type, item->sellValue(player));
		Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_TRADING_SOLD, item->type, 1);
		Compendium_t::Events_t::eventUpdateWorld(player, Compendium_t::CPDM_SHOP_SOLD, "shop", 1);
		Compendium_t::Events_t::eventUpdateWorld(player, Compendium_t::CPDM_SHOP_GOLD_EARNED, "shop", item->sellValue(player));
	}

	if ( multiplayer != CLIENT )
	{
		Entity* entity = uidToEntity(shopkeeper[player]);
		if ( entity )
		{
			Stat* shopstats = entity->getStats();
			shopstats->GOLD -= item->sellValue(player);
		}
	}

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
	messagePlayer(player, MESSAGE_INVENTORY, Language::get(1125), item->description(), item->sellValue(player));
	item->count = ocount;
	if ( multiplayer != CLIENT )
	{
		//if ( players[player] && players[player]->entity )
		//{
		//	if ( local_rng.rand() % 2 )
		//	{
		//		if ( item->sellValue(player) <= 1 )
		//		{
		//			// selling cheap items does not increase trading past basic
		//			if ( stats[player]->PROFICIENCIES[PRO_TRADING] < SKILL_LEVEL_SKILLED )
		//			{
		//				players[player]->entity->increaseSkill(PRO_TRADING);
		//			}
		//		}
		//		else
		//		{
		//			players[player]->entity->increaseSkill(PRO_TRADING);
		//		}
		//	}
		//}
	}
	else
	{
		strcpy((char*)net_packet->data, "SHPS");
		SDLNet_Write32(shopkeeper[player], &net_packet->data[4]);

		// send item that was sold to server
		SDLNet_Write32(item->type, &net_packet->data[8]);
		SDLNet_Write32(item->status, &net_packet->data[12]);
		SDLNet_Write16(item->beatitude, &net_packet->data[16]);
		net_packet->data[18] = (Sint8)xout;
		net_packet->data[19] = (Sint8)yout;
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
	return true;
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
