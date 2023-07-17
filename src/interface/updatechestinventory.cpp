/*-------------------------------------------------------------------------------

	BARONY
	File: updatechestinventory.cpp
	Desc: contains updateChestInventory()

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../main.hpp"
#include "../draw.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "../items.hpp"
#include "../engine/audio/sound.hpp"
#include "../net.hpp"
#include "../player.hpp"
#include "interface.hpp"

Entity* openedChest[MAXPLAYERS] = { nullptr };

int numItemsInChest(const int player)
{
	node_t* node = nullptr;

	list_t* chestInventory = nullptr;
	if ( multiplayer == CLIENT )
	{
		chestInventory = &chestInv[player];
	}
	else if (openedChest[player]->children.first && openedChest[player]->children.first->element)
	{
		chestInventory = (list_t*)openedChest[player]->children.first->element;
	}

	int i = 0;

	if ( chestInventory )
	{
		for (node = chestInventory->first; node != nullptr; node = node->next)
		{
			++i;
		}
	}

	return i;
}

/*-------------------------------------------------------------------------------

	updateChestInventory

	Processes and draws everything related to chest inventory

-------------------------------------------------------------------------------*/

void updateChestInventory(const int player)
{
	if ( !openedChest[player] )
	{
		return;
	}

	list_t* chest_inventory = NULL;
	if ( multiplayer == CLIENT )
	{
		chest_inventory = &chestInv[player];
	}
	else if ( openedChest[player]->children.first && openedChest[player]->children.first->element )
	{
		chest_inventory = (list_t*)openedChest[player]->children.first->element;
	}

	if ( chest_inventory )
	{
		std::unordered_set<int> takenSlots;
		std::vector<Item*> itemsToRearrange;
		for ( node_t* node = chest_inventory->first; node != NULL; node = node->next )
		{
			if ( node->element )
			{
				Item* item = (Item*)node->element;
				if ( item )
				{
					int key = item->x + 100 * item->y;
					if ( item->x >= 0 && item->x < players[player]->inventoryUI.MAX_CHEST_X
						&& item->y >= 0 && item->y < players[player]->inventoryUI.MAX_CHEST_Y
						&& takenSlots.find(key) == takenSlots.end() )
					{
						takenSlots.insert(key);
					}
					else
					{
						itemsToRearrange.push_back(item);
					}
				}
			}
		}
		for ( auto item : itemsToRearrange )
		{
			bool foundSlot = false;
			for ( int y = 0; y < players[player]->inventoryUI.MAX_CHEST_Y && !foundSlot; ++y )
			{
				for ( int x = 0; x < players[player]->inventoryUI.MAX_CHEST_X && !foundSlot; ++x )
				{
					int key = x + 100 * y;
					if ( takenSlots.find(key) == takenSlots.end() )
					{
						foundSlot = true;
						takenSlots.insert(key);
						item->x = x;
						item->y = y;
					}
				}
			}
			if ( !foundSlot )
			{
				item->x = players[player]->inventoryUI.MAX_CHEST_X;
				item->y = players[player]->inventoryUI.MAX_CHEST_Y;
			}
		}
	}

	return;
}
