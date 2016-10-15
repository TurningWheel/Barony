/*-------------------------------------------------------------------------------

	BARONY
	File: items.cpp
	Desc: contains helper functions for item stuff

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "items.hpp"
#include "messages.hpp"
#include "interface/interface.hpp"
#include "magic/magic.hpp"
#include "sound.hpp"
#include "book.hpp"
#include "scrolls.hpp"
#include "shops.hpp"
#include "prng.hpp"
#include "scores.hpp"
#include "net.hpp"
#include "player.hpp"

Uint32 itemuids=1;
ItemGeneric items[NUMITEMS];

/*-------------------------------------------------------------------------------

	newItem

	Creates a new item and places it in an inventory

-------------------------------------------------------------------------------*/

Item *newItem(ItemType type,Status status,Sint16 beatitude,Sint16 count,Uint32 appearance,bool identified,list_t *inventory) {
	Item *item;
	
	// allocate memory for the item
	if( (item = (Item *) malloc(sizeof(Item)))==NULL ) {
		printlog( "failed to allocate memory for new item!\n" );
		exit(1);
	}

	//item->captured_monster = nullptr;
	
	// add the item to the inventory
	if( inventory!=NULL ) {
		item->node = list_AddNodeLast(inventory);
		item->node->element = item;
		item->node->deconstructor = &defaultDeconstructor;
		item->node->size = sizeof(Item);
	} else {
		item->node = NULL;
	}
	
	// now set all of my data elements
	item->type=type;
	item->status=status;
	item->beatitude=beatitude;
	item->count=count;
	item->appearance=appearance;
	item->identified=identified;
	item->uid = itemuids;
	if( inventory ) {
		int x, y;
		bool notfree=FALSE, foundaspot=FALSE;

		bool is_spell = FALSE;
		if (itemCategory(item) == SPELL_CAT)
			is_spell = TRUE;

		x=0;
		while( 1 ) {
			for( y=0; y<INVENTORY_SIZEY; y++ ) {
				node_t *node;
				for( node=inventory->first; node!=NULL; node=node->next ) {
					Item *tempItem = (Item *)node->element;
					if( tempItem==item )
						continue;
					if( tempItem ) {
						if( tempItem->x == x && tempItem->y == y ) {
							if (is_spell && itemCategory(tempItem) == SPELL_CAT)
								notfree=TRUE; //Both spells. Can't fit in the same slot.
							else if (!is_spell && itemCategory(tempItem) != SPELL_CAT)
								notfree=TRUE; //Both not spells. Can't fit in the same slot.
						}
					}
				}
				if( notfree ) {
					notfree=FALSE;
					continue;
				}
				item->x = x;
				item->y = y;
				foundaspot=TRUE;
				break;
			}
			if( foundaspot )
				break;
			x++;
		}

		// add the item to the hotbar automatically
		if( !intro && auto_hotbar_new_items) {
			if( inventory==&stats[clientnum]->inventory ) {
				int c;
				for( c=0; c<NUM_HOTBAR_SLOTS; c++ ) {
					if( !uidToItem(hotbar[c].item) ) {
						hotbar[c].item = item->uid;
						break;
					}
				}
			}
		}
	} else {
		item->x = 0;
		item->y = 0;
	}

	itemuids++;
	return item;
}

/*-------------------------------------------------------------------------------

	uidToItem

	returns an item from the player's inventory from the given uid

-------------------------------------------------------------------------------*/

Item *uidToItem(Uint32 uid) {
	node_t *node;
	for( node=stats[clientnum]->inventory.first; node!=NULL; node=node->next ) {
		Item *item = (Item *)node->element;
		if( item->uid == uid )
			return item;
	}
	return NULL;
}

/*-------------------------------------------------------------------------------

	itemCurve
	
	Selects an item type from the given category of items by factoring in
	dungeon level, value of the item, etc.

-------------------------------------------------------------------------------*/

ItemType itemCurve(Category cat) {
	int numitems = NUMITEMS - ( NUMITEMS - ((int)ARTIFACT_SWORD) );
	bool chances[NUMITEMS];
	int c;
	
	if( cat<0 || cat>=NUMCATEGORIES ) {
		printlog("warning: itemCurve() called with bad category value!\n");
		return GEM_ROCK;
	}
	
	// find highest value of items in category
	Uint32 highestvalue=0;
	Uint32 lowestvalue=0;
	Uint32 numoftype=0;
	for( c=0; c<numitems; c++ ) {
		if( items[c].category == cat ) {
			highestvalue = std::max<Uint32>(highestvalue,items[c].value); //TODO: Why are Uint32 and int being compared?
			lowestvalue = std::min<Uint32>(lowestvalue,items[c].value); //TODO: Why are Uint32 and int being compared?
			numoftype++;
		}
	}
	if( numoftype==0 ) {
		printlog("warning: category passed to itemCurve has no items!\n");
		return GEM_ROCK;
	}
	
	if( cat == SCROLL || cat == POTION || cat == BOOK ) {
		// these item categories will spawn anything of their type
		for( c=0; c<numitems; c++ ) {
			chances[c] = FALSE;
			if( items[c].category == cat )
				chances[c] = TRUE;
		}
	} else if( cat == TOOL ) {
		// this category will spawn specific items more frequently regardless of level
		for( c=0; c<numitems; c++ ) {
			chances[c] = FALSE;
			if( items[c].category == cat ) {
				switch( (ItemType)c ) {
					case TOOL_TINOPENER:
						if( prng_get_uint()%2 ) // 50% chance
							chances[c] = TRUE;
						break;
					case TOOL_LANTERN:
						if( prng_get_uint()%4 ) // 75% chance
							chances[c] = TRUE;
						break;
					case TOOL_SKELETONKEY:
						chances[c] = FALSE; // 0% chance
						break;
					default:
						chances[c] = TRUE;
						break;
				}
			}
		}
	} else {
		// other categories get a special chance algorithm based on item value and dungeon level
		int acceptablehigh = std::max<Uint32>(highestvalue*fmin(1.0,(currentlevel+10)/25.0), lowestvalue); //TODO: Why are double and Uint32 being compared?
		for( c=0; c<numitems; c++ ) {
			chances[c] = FALSE;
			if( items[c].category == cat && items[c].value <= acceptablehigh )
				chances[c] = TRUE;
		}
	}
	
	// calculate number of items left
	Uint32 numleft=0;
	for( c=0; c<numitems; c++ ) {
		if( chances[c]==TRUE )
			numleft++;
	}
	if( numleft==0 ) {
		return GEM_ROCK;
	}
	
	// most gems are worthless pieces of glass
	if( cat==GEM ) {
		if( prng_get_uint()%10 ) {
			return GEM_GLASS;
		}
	}
	
	// pick the item
	int pick = prng_get_uint()%numleft;
	for( c=0; c<numitems; c++ ) {
		if( items[c].category == cat ) {
			if( chances[c]==TRUE ) {
				if( pick==0 ) {
					return static_cast<ItemType>(c);
				} else {
					pick--;
				}
			}
		}
	}
	
	return GEM_ROCK;
}

/*-------------------------------------------------------------------------------

	Item::description

	Returns a string that describes the given item's properties

-------------------------------------------------------------------------------*/

char *Item::description() {
	int c=0;

	if( identified==TRUE ) {
		if( count < 2 ) {
			if( itemCategory(this)==WEAPON || itemCategory(this)==ARMOR || itemCategory(this)==MAGICSTAFF || itemCategory(this)==TOOL )
				snprintf(tempstr,1024,language[982+status],beatitude);
			else if( itemCategory(this)==AMULET || itemCategory(this)==RING || itemCategory(this)==GEM )
				snprintf(tempstr,1024,language[987+status],beatitude);
			else if( itemCategory(this)==POTION )
				snprintf(tempstr,1024,language[992+status],language[974+items[type].index+appearance%items[type].variations-50],beatitude);
			else if( itemCategory(this)==SCROLL || itemCategory(this)==SPELLBOOK || itemCategory(this)==BOOK )
				snprintf(tempstr,1024,language[997+status],beatitude);
			else if( itemCategory(this)==FOOD )
				snprintf(tempstr,1024,language[1002+status],beatitude);
			for( c=0; c<1024; c++ )
				if( tempstr[c]==0 )
					break;
			if( type >= 0 && type < NUMITEMS ) {
				if( itemCategory(this)==BOOK )
					snprintf(&tempstr[c],1024-c,language[1007],books[appearance%numbooks]->name);
				else
					snprintf(&tempstr[c],1024-c,"%s",items[type].name_identified);
			} else {
				snprintf(&tempstr[c],1024-c,"ITEM%03d",type);
			}
		} else {
			if( itemCategory(this)==WEAPON || itemCategory(this)==ARMOR || itemCategory(this)==MAGICSTAFF || itemCategory(this)==TOOL )
				snprintf(tempstr,1024,language[1008+status],count,beatitude);
			else if( itemCategory(this)==AMULET || itemCategory(this)==RING || itemCategory(this)==GEM )
				snprintf(tempstr,1024,language[1013+status],count,beatitude);
			else if( itemCategory(this)==POTION )
				snprintf(tempstr,1024,language[1018+status],count,language[974+items[type].index+appearance%items[type].variations-50],beatitude);
			else if( itemCategory(this)==SCROLL || itemCategory(this)==SPELLBOOK || itemCategory(this)==BOOK )
				snprintf(tempstr,1024,language[1023+status],count,beatitude);
			else if( itemCategory(this)==FOOD )
				snprintf(tempstr,1024,language[1028+status],count,beatitude);
			for( c=0; c<1024; c++ )
				if( tempstr[c]==0 )
					break;
			if( type >= 0 && type < NUMITEMS ) {
				if( itemCategory(this)==BOOK )
					snprintf(&tempstr[c],1024-c,language[1033],count,books[appearance%numbooks]->name);
				else
					snprintf(&tempstr[c],1024-c,"%s",items[type].name_identified);
			} else {
				snprintf(&tempstr[c],1024-c,"ITEM%03d",type);
			}
		}
	} else {
		if( count < 2 ) {
			if( itemCategory(this)==WEAPON || itemCategory(this)==ARMOR || itemCategory(this)==MAGICSTAFF || itemCategory(this)==TOOL )
				snprintf(tempstr,1024,language[1034+status]);
			else if( itemCategory(this)==AMULET || itemCategory(this)==RING || itemCategory(this)==GEM )
				snprintf(tempstr,1024,language[1039+status]);
			else if( itemCategory(this)==POTION )
				snprintf(tempstr,1024,language[1044+status],language[974+items[type].index+appearance%items[type].variations-50]);
			else if( itemCategory(this)==SCROLL || itemCategory(this)==SPELLBOOK || itemCategory(this)==BOOK )
				snprintf(tempstr,1024,language[1049+status]);
			else if( itemCategory(this)==FOOD )
				snprintf(tempstr,1024,language[1054+status]);
			for( c=0; c<1024; c++ )
				if( tempstr[c]==0 )
					break;
			if( type >= 0 && type < NUMITEMS ) {
				if( itemCategory(this)==SCROLL ) {
					snprintf(&tempstr[c],1024-c,language[1059],items[type].name_unidentified,scroll_label[appearance%NUMLABELS]);
				} else {
					if( itemCategory(this)==BOOK )
						snprintf(&tempstr[c],1024-c,language[1007],books[appearance%numbooks]->name);
					else
						snprintf(&tempstr[c],1024-c,"%s",items[type].name_unidentified);
				}
			} else {
				snprintf(&tempstr[c],1024-c,"ITEM%03d",type);
			}
		} else {
			if( itemCategory(this)==WEAPON || itemCategory(this)==ARMOR || itemCategory(this)==MAGICSTAFF || itemCategory(this)==TOOL )
				snprintf(tempstr,1024,language[1060+status],count);
			else if( itemCategory(this)==AMULET || itemCategory(this)==RING || itemCategory(this)==GEM )
				snprintf(tempstr,1024,language[1065+status],count);
			else if( itemCategory(this)==POTION )
				snprintf(tempstr,1024,language[1070+status],count,language[974+items[type].index+appearance%items[type].variations-50]);
			else if( itemCategory(this)==SCROLL || itemCategory(this)==SPELLBOOK || itemCategory(this)==BOOK )
				snprintf(tempstr,1024,language[1075+status],count);
			else if( itemCategory(this)==FOOD )
				snprintf(tempstr,1024,language[1080+status],count);
			for( c=0; c<1024; c++ )
				if( tempstr[c]==0 )
					break;
			if( type >= 0 && type < NUMITEMS ) {
				if( itemCategory(this)==SCROLL ) {
					snprintf(&tempstr[c],1024-c,language[1085],count,items[type].name_unidentified,scroll_label[appearance%NUMLABELS]);
				} else {
					if( itemCategory(this)==BOOK )
						snprintf(&tempstr[c],1024-c,language[1086],count,books[appearance%numbooks]->name);
					else
						snprintf(&tempstr[c],1024-c,"%s",items[type].name_unidentified);
				}
			} else {
				snprintf(&tempstr[c],1024-c,"ITEM%03d",type);
			}
		}
	}
	return tempstr;
}

/*-------------------------------------------------------------------------------

	itemCategory

	Returns the category that a specified item belongs to

-------------------------------------------------------------------------------*/

Category itemCategory(Item *item) {
	if( !item )
		return GEM;
	return items[item->type].category;
}

/*-------------------------------------------------------------------------------

	Item::getName

	Returns the name of an item type as a character string

-------------------------------------------------------------------------------*/

char *Item::getName() {
	if( type >= 0 && type < NUMITEMS ) {
		if( identified ) {
			if( itemCategory(this)==BOOK )
				snprintf(tempstr,sizeof(tempstr),language[1007],books[appearance%numbooks]->name);
			else
				strcpy(tempstr,items[type].name_identified);
		} else {
			if( itemCategory(this)==SCROLL ) {
				snprintf(tempstr,sizeof(tempstr),language[1059],items[type].name_unidentified,scroll_label[appearance%NUMLABELS]);
			} else if( itemCategory(this)==BOOK ) {
				snprintf(tempstr,sizeof(tempstr),language[1007],books[appearance%numbooks]->name);
			} else {
				strcpy(tempstr,items[type].name_unidentified);
			}
		}
	} else {
		snprintf(tempstr,sizeof(tempstr),"ITEM%03d",type);
	}
	return tempstr;
}

/*-------------------------------------------------------------------------------

	itemModel

	returns a model index number based on the properties of the given item

-------------------------------------------------------------------------------*/

Sint32 itemModel(Item *item) {
	if( !item )
		return 0;
	return items[item->type].index+item->appearance%items[item->type].variations;
}

/*-------------------------------------------------------------------------------

	itemModelFirstperson

	returns the first person model of the given item

-------------------------------------------------------------------------------*/

Sint32 itemModelFirstperson(Item *item) {
	if( !item )
		return 0;
	return items[item->type].fpindex+item->appearance%items[item->type].variations;
}

/*-------------------------------------------------------------------------------

	itemSprite

	returns a pointer to the SDL_Surface used to represent the item

-------------------------------------------------------------------------------*/

SDL_Surface *itemSprite(Item *item) {
	if( !item )
		return NULL;
	if (itemCategory(item) == SPELL_CAT) {
		spell_t *spell = getSpellFromItem(item);
		if (spell) {
			node_t *node = list_Node(&items[item->type].surfaces, spell->ID);
			if( !node )
				return NULL;
			SDL_Surface **surface = (SDL_Surface **)node->element;
			return *surface;
		}
	} else {
		node_t *node = list_Node(&items[item->type].surfaces,item->appearance%items[item->type].variations);
		if( !node )
			return NULL;
		SDL_Surface **surface = (SDL_Surface **)node->element;
		return *surface;
	}
	return NULL;
}

/*-------------------------------------------------------------------------------

	itemCompare

	Compares two items and returns 0 if they are identical or 1 if they are
	not identical. Item count is excluded during comparison testing.

-------------------------------------------------------------------------------*/

int itemCompare(Item *item1, Item *item2) {
	// null cases
	if( item1 == NULL ) {
		if( item2 == NULL )
			return 0;
		else
			return 1;
	} else {
		if( item2 == NULL )
			return 1;
	}
	
	// check attributes
	if(item1->type != item2->type)
		return 1;
	if(item1->status != item2->status)
		return 1;
	if(item1->beatitude != item2->beatitude)
		return 1;
	if(item1->appearance != item2->appearance)
		return 1;
	if(item1->identified != item2->identified)
		return 1;
	
	// items are identical
	return 0;
}

/*-------------------------------------------------------------------------------

	dropItem

	Handles the client impulse to drop an item

-------------------------------------------------------------------------------*/

void dropItem(Item *item, int player) {
	if (!item)
		return;

	Entity *entity;
	Sint16 oldcount;

	if (item == nullptr || players[player] == nullptr || players[player]->entity == nullptr || itemCategory(item) == SPELL_CAT)
		return;
	if( itemIsEquipped(item,player) ) {
		if (!item->canUnequip()) {
			messagePlayer(player, language[1087]);
			return;
		}
	}

	if( multiplayer==CLIENT ) {
		strcpy((char *)net_packet->data,"DROP");
		SDLNet_Write32((Uint32)item->type,&net_packet->data[4]);
		SDLNet_Write32((Uint32)item->status,&net_packet->data[8]);
		SDLNet_Write32((Uint32)item->beatitude,&net_packet->data[12]);
		SDLNet_Write32((Uint32)item->count,&net_packet->data[16]);
		SDLNet_Write32((Uint32)item->appearance,&net_packet->data[20]);
		net_packet->data[24] = item->identified;
		net_packet->data[25] = clientnum;
		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		net_packet->len = 26;
		sendPacketSafe(net_sock, -1, net_packet, 0);
		if (item == open_book_item)
			closeBookGUI();

		oldcount = item->count;
		item->count=1;
		messagePlayer(player,language[1088],item->description());
		item->count=oldcount-1;

		// unequip the item
		if( item->count <= 1 ) {
			Item **slot = itemSlot(stats[player],item);
			if( slot != NULL )
				*slot = NULL;
		}

		if( item->count <= 0 )
			list_RemoveNode(item->node);
	} else {
		if (item == open_book_item)
			closeBookGUI();
		entity = newEntity(-1,1,map.entities);
		entity->flags[INVISIBLE]=TRUE;
		entity->flags[UPDATENEEDED]=TRUE;
		entity->x = players[player]->entity->x;
		entity->y = players[player]->entity->y;
		entity->sizex = 4;
		entity->sizey = 4;
		entity->yaw = players[player]->entity->yaw;
		entity->vel_x = (1.5+.025*(rand()%11)) * cos(players[player]->entity->yaw);
		entity->vel_y = (1.5+.025*(rand()%11)) * sin(players[player]->entity->yaw);
		entity->vel_z = (-10-rand()%20)*.01;
		entity->flags[PASSABLE] = TRUE;
		entity->behavior = &actItem;
		entity->skill[10] = item->type;
		entity->skill[11] = item->status;
		entity->skill[12] = item->beatitude;
		entity->skill[13] = 1;
		entity->skill[14] = item->appearance;
		entity->skill[15] = item->identified;
		entity->parent = players[player]->entity->uid;

		// play sound
		playSoundEntity( players[player]->entity, 47+rand()%3, 64 );

		// unequip the item
		Item **slot = itemSlot(stats[player],item);
		if( slot != NULL )
			*slot = NULL;
		if( item->node != NULL ) {
			if( item->node->list==&stats[0]->inventory ) {
				oldcount = item->count;
				item->count=1;
				messagePlayer(player,language[1088],item->description());
				item->count=oldcount-1;
				if( item->count <= 0 )
					list_RemoveNode(item->node);
			}
		} else {
			item->count--;
			if( item->count <= 0 )
				free(item);
		}
	}
}

Entity *dropItemMonster(Item *item, Entity *monster, Stat *monsterStats) {
	Entity *entity;

	if( !item || !monster )
		return NULL;

	entity = newEntity(-1,1,map.entities);
	entity->flags[INVISIBLE]=TRUE;
	entity->flags[UPDATENEEDED]=TRUE;
	entity->x = monster->x;
	entity->y = monster->y;
	entity->sizex = 4;
	entity->sizey = 4;
	entity->yaw = monster->yaw;
	entity->vel_x = (rand()%20-10)/10.0;
	entity->vel_y = (rand()%20-10)/10.0;
	entity->vel_z = -.5;
	entity->flags[PASSABLE] = TRUE;
	entity->flags[USERFLAG1] = TRUE; // speeds up game when many items are dropped
	entity->behavior = &actItem;
	entity->skill[10] = item->type;
	entity->skill[11] = item->status;
	entity->skill[12] = item->beatitude;
	entity->skill[13] = 1;
	entity->skill[14] = item->appearance;
	entity->skill[15] = item->identified;
	entity->parent = monster->uid;

	item->count--;
	Item **slot;
	if( (slot=itemSlot(monsterStats,item)) != NULL ) {
		*slot = NULL; // clear the item slot
	}
	if( item->count <= 0 ) {
		if( item->node ) {
			list_RemoveNode(item->node);
		} else {
			free(item);
		}
	}
	
	return entity;
}

/*-------------------------------------------------------------------------------

	consumeItem
	
	consumes an item

-------------------------------------------------------------------------------*/

void consumeItem(Item *item) {
	if( item == NULL )
		return;
	if( appraisal_item == item->uid && item->count == 1 ) {
		appraisal_item = 0;
		appraisal_timer = 0;
	}
	item->count--;
	if( item->count <= 0 ) {
		if( item->node != NULL ) {
			int i;
			for( i=0; i<MAXPLAYERS; i++ ) {
				if( item->node->list == &stats[i]->inventory ) {
					Item **slot;
					if( (slot=itemSlot(stats[i],item)) != NULL ) {
						*slot = NULL;
					}
				}
			}
			list_RemoveNode(item->node);
		} else {
			free(item);
		}
	}
}

/*-------------------------------------------------------------------------------

	equipItem

	Handles the client impulse to equip an item

-------------------------------------------------------------------------------*/

void equipItem(Item *item, Item **slot, int player) {
	int oldcount;

	if (!item) // needs "|| !slot " ?
		return;

	if( itemCompare(*slot,item) ) {
		if( *slot != NULL ) {
			if (!(*slot)->canUnequip()) {
				if( player==clientnum ) {
					messagePlayer(player,language[1089],(*slot)->getName());
				}
				(*slot)->identified=TRUE;
				return;
			}
		}
		if( multiplayer != CLIENT && !intro && !fadeout ) {
			if( players[player] != nullptr && players[player]->entity != nullptr)
			{
				if (players[player]->entity->ticks > 60)
				{
					if (itemCategory(item) == AMULET || itemCategory(item) == RING)
						playSoundEntity(players[player]->entity, 33 + rand()%2, 64);
					else if (itemCategory(item) == WEAPON)
						playSoundEntity(players[player]->entity, 40 + rand()%4, 64);
					else if (itemCategory(item) == ARMOR)
						playSoundEntity(players[player]->entity, 44 + rand()%3, 64);
					else if (item->type == TOOL_TORCH || item->type == TOOL_LANTERN)
						playSoundEntity(players[player]->entity, 134, 64);
				}
			}
		}
		if( multiplayer==SERVER && player>0 ) {
			if( *slot!=NULL ) {
				if( (*slot)->node )
					list_RemoveNode((*slot)->node);
				else
					free(*slot);
			}
		} else {
			oldcount = item->count;
			item->count = 1;
			if( intro==FALSE )
				messagePlayer(player,language[1090],item->description());
			item->count = oldcount;
		}
		*slot = item;
		if( player==clientnum ) {
			if( slot==&stats[player]->weapon ) {
				weaponSwitch = TRUE;
			} else if( slot==&stats[player]->shield ) {
				shieldSwitch = TRUE;
			}
		}
	} else {
		if( *slot != NULL ) {
			if (!(*slot)->canUnequip()) {
				if( player==clientnum ) {
					messagePlayer(player,language[1089],(*slot)->getName());
				}
				(*slot)->identified=TRUE;
				return;
			}
		}
		if (multiplayer != CLIENT && !intro && !fadeout)
		{
			if (players[player] != nullptr && players[player]->entity != nullptr)
			{
				if (players[player]->entity->ticks > 60)
				{
					if (itemCategory(item) == ARMOR)
					{
						playSoundEntity(players[player]->entity, 44 + rand()%3, 64);
					}
				}
			}
		}
		if( player!=0 && multiplayer==SERVER ) {
			if( item->node )
				list_RemoveNode(item->node);
			else
				free(item);
			if( *slot!=NULL ) {
				if( (*slot)->node )
					list_RemoveNode((*slot)->node);
				else
					free(*slot);
			}
		} else {
			oldcount = item->count;
			item->count = 1;
			if( intro==FALSE && !fadeout )
				messagePlayer(player,language[1091],item->description());
			item->count = oldcount;
		}
		*slot = NULL;
	}
}

/*-------------------------------------------------------------------------------

	useItem

	Handles the client impulse to use an item

-------------------------------------------------------------------------------*/

void useItem(Item *item, int player) {
	if( item == NULL )
		return;

	if (openedChest[player] && itemCategory(item) != SPELL_CAT) {
		//If a chest is open, put the item in the chest.
		openedChest[player]->addItemToChestFromInventory(player, item, FALSE);
		return;
	} else if( gui_mode == GUI_MODE_SHOP && player==clientnum && itemCategory(item) != SPELL_CAT) {
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
		if( deal ) {
			sellitem = item;
			shopspeech = language[215];
			shoptimer = ticks-1;
		} else {
			shopspeech = language[212+rand()%3];
			shoptimer = ticks-1;
		}
		return;
	}
	
	if( item->status == BROKEN && player==clientnum ) {
		messagePlayer(player,language[1092],item->getName());
		return;
	}
	
	// tins need a tin opener to open...
	if( player==clientnum ) {
		if( item->type == FOOD_TIN ) {
			bool havetinopener = FALSE;
			node_t *node;
			for( node=stats[clientnum]->inventory.first; node!=NULL; node=node->next ) {
				Item *tempitem = (Item *)node->element;
				if( tempitem->type == TOOL_TINOPENER ) {
					if( tempitem->status != BROKEN ) {
						havetinopener = TRUE;
						break;
					}
				}
			}
			if( !havetinopener ) {
				messagePlayer(clientnum,language[1093]);
				return;
			}
		}
	}

	if( multiplayer==CLIENT && !intro ) {
		strcpy((char *)net_packet->data,"USEI");
		SDLNet_Write32((Uint32)item->type,&net_packet->data[4]);
		SDLNet_Write32((Uint32)item->status,&net_packet->data[8]);
		SDLNet_Write32((Uint32)item->beatitude,&net_packet->data[12]);
		SDLNet_Write32((Uint32)item->count,&net_packet->data[16]);
		SDLNet_Write32((Uint32)item->appearance,&net_packet->data[20]);
		net_packet->data[24] = item->identified;
		net_packet->data[25] = clientnum;
		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		net_packet->len = 26;
		sendPacketSafe(net_sock, -1, net_packet, 0);
	}
	switch( item->type ) {
		case WOODEN_SHIELD:
			equipItem(item,&stats[player]->shield,player);
			break;
		case QUARTERSTAFF:
		case BRONZE_SWORD:
		case BRONZE_MACE:
		case BRONZE_AXE:
			equipItem(item,&stats[player]->weapon,player);
			break;
		case BRONZE_SHIELD:
			equipItem(item,&stats[player]->shield,player);
			break;
		case SLING:
		case IRON_SPEAR:
		case IRON_SWORD:
		case IRON_MACE:
		case IRON_AXE:
			equipItem(item,&stats[player]->weapon,player);
			break;
		case IRON_SHIELD:
			equipItem(item,&stats[player]->shield,player);
			break;
		case SHORTBOW:
		case STEEL_HALBERD:
		case STEEL_SWORD:
		case STEEL_MACE:
		case STEEL_AXE:
			equipItem(item,&stats[player]->weapon,player);
			break;
		case STEEL_SHIELD:
		case STEEL_SHIELD_RESISTANCE:
			equipItem(item,&stats[player]->shield,player);
			break;
		case CROSSBOW:
			equipItem(item,&stats[player]->weapon,player);
			break;
		case GLOVES:
		case GLOVES_DEXTERITY:
		case BRACERS:
		case BRACERS_CONSTITUTION:
		case GAUNTLETS:
		case GAUNTLETS_STRENGTH:
			equipItem(item,&stats[player]->gloves,player);
			break;
		case CLOAK:
		case CLOAK_MAGICREFLECTION:
		case CLOAK_INVISIBILITY:
		case CLOAK_PROTECTION:
			equipItem(item,&stats[player]->cloak,player);
			break;
		case LEATHER_BOOTS:
		case LEATHER_BOOTS_SPEED:
		case IRON_BOOTS:
		case IRON_BOOTS_WATERWALKING:
		case STEEL_BOOTS:
		case STEEL_BOOTS_LEVITATION:
		case STEEL_BOOTS_FEATHER:
			equipItem(item,&stats[player]->shoes,player);
			break;
		case LEATHER_BREASTPIECE:
		case IRON_BREASTPIECE:
		case STEEL_BREASTPIECE:
			equipItem(item,&stats[player]->breastplate,player);
			break;
		case HAT_PHRYGIAN:
		case HAT_HOOD:
		case HAT_WIZARD:
		case HAT_JESTER:
		case LEATHER_HELM:
		case IRON_HELM:
		case STEEL_HELM:
			equipItem(item,&stats[player]->helmet,player);
			break;
		case AMULET_SEXCHANGE:
			messagePlayer(player,language[1094]);
			item_AmuletSexChange(item,player);
			consumeItem(item);
			break;
		case AMULET_LIFESAVING:
		case AMULET_WATERBREATHING:
		case AMULET_MAGICREFLECTION:
			equipItem(item,&stats[player]->amulet,player);
			break;
		case AMULET_STRANGULATION:
			equipItem(item,&stats[player]->amulet,player);
			messagePlayer(player,language[1095]);
			if( item->beatitude>=0 )
				item->beatitude = -1;
			break;
		case AMULET_POISONRESISTANCE:
			equipItem(item,&stats[player]->amulet,player);
			break;
		case POTION_WATER:
			item_PotionWater(item, players[player]->entity);
			break;
		case POTION_BOOZE:
			item_PotionBooze(item, players[player]->entity);
			break;
		case POTION_JUICE:
			item_PotionJuice(item, players[player]->entity);
			break;
		case POTION_SICKNESS:
			item_PotionSickness(item, players[player]->entity);
			break;
		case POTION_CONFUSION:
			item_PotionConfusion(item, players[player]->entity);
			break;
		case POTION_EXTRAHEALING:
			item_PotionExtraHealing(item, players[player]->entity);
			break;
		case POTION_HEALING:
			item_PotionHealing(item, players[player]->entity);
			break;
		case POTION_CUREAILMENT:
			item_PotionCureAilment(item, players[player]->entity);
			break;
		case POTION_BLINDNESS:
			item_PotionBlindness(item, players[player]->entity);
			break;
		case POTION_RESTOREMAGIC:
			item_PotionRestoreMagic(item, players[player]->entity);
			break;
		case POTION_INVISIBILITY:
			item_PotionInvisibility(item, players[player]->entity);
			break;
		case POTION_LEVITATION:
			item_PotionLevitation(item, players[player]->entity);
			break;
		case POTION_SPEED:
			item_PotionSpeed(item, players[player]->entity);
			break;
		case POTION_ACID:
			item_PotionAcid(item, players[player]->entity);
			break;
		case POTION_PARALYSIS:
			item_PotionParalysis(item, players[player]->entity);
			break;
		case SCROLL_MAIL:
			item_ScrollMail(item, player);
			break;
		case SCROLL_IDENTIFY:
			item_ScrollIdentify(item, player);
			if( !players[player]->entity->isBlind() )
				consumeItem(item);
			break;
		case SCROLL_LIGHT:
			item_ScrollLight(item, player);
			if( !players[player]->entity->isBlind() )
				consumeItem(item);
			break;
		case SCROLL_BLANK:
			item_ScrollBlank(item, player);
			break;
		case SCROLL_ENCHANTWEAPON:
			item_ScrollEnchantWeapon(item, player);
			if( !players[player]->entity->isBlind() )
				consumeItem(item);
			break;
		case SCROLL_ENCHANTARMOR:
			item_ScrollEnchantArmor(item, player);
			if( !players[player]->entity->isBlind() )
				consumeItem(item);
			break;
		case SCROLL_REMOVECURSE:
			item_ScrollRemoveCurse(item, player);
			if( !players[player]->entity->isBlind() )
				consumeItem(item);
			break;
		case SCROLL_FIRE:
			item_ScrollFire(item, player);
			if( !players[player]->entity->isBlind() )
				consumeItem(item);
			break;
		case SCROLL_FOOD:
			item_ScrollFood(item, player);
			if( !players[player]->entity->isBlind() )
				consumeItem(item);
			break;
		case SCROLL_MAGICMAPPING:
			item_ScrollMagicMapping(item, player);
			if( !players[player]->entity->isBlind() )
				consumeItem(item);
			break;
		case SCROLL_REPAIR:
			item_ScrollRepair(item, player);
			if( !players[player]->entity->isBlind() )
				consumeItem(item);
			break;
		case SCROLL_DESTROYARMOR:
			item_ScrollDestroyArmor(item, player);
			if( !players[player]->entity->isBlind() )
				consumeItem(item);
			break;
		case SCROLL_TELEPORTATION:
			item_ScrollTeleportation(item, player);
			if( !players[player]->entity->isBlind() )
				consumeItem(item);
			break;
		case SCROLL_SUMMON:
			item_ScrollSummon(item, player);
			if( !players[player]->entity->isBlind() )
				consumeItem(item);
			break;
		case MAGICSTAFF_LIGHT:
		case MAGICSTAFF_DIGGING:
		case MAGICSTAFF_LOCKING:
		case MAGICSTAFF_MAGICMISSILE:
		case MAGICSTAFF_OPENING:
		case MAGICSTAFF_SLOW:
		case MAGICSTAFF_COLD:
		case MAGICSTAFF_FIRE:
		case MAGICSTAFF_LIGHTNING:
		case MAGICSTAFF_SLEEP:
			equipItem(item,&stats[player]->weapon,player);
			break;
		case RING_ADORNMENT:
		case RING_SLOWDIGESTION:
		case RING_PROTECTION:
		case RING_WARNING:
		case RING_STRENGTH:
		case RING_CONSTITUTION:
		case RING_INVISIBILITY:
		case RING_MAGICRESISTANCE:
		case RING_CONFLICT:
		case RING_LEVITATION:
		case RING_REGENERATION:
		case RING_TELEPORTATION:
			equipItem(item,&stats[player]->ring,player);
			break;
		case SPELLBOOK_FORCEBOLT:
		case SPELLBOOK_MAGICMISSILE:
		case SPELLBOOK_COLD:
		case SPELLBOOK_FIREBALL:
		case SPELLBOOK_LIGHTNING:
		case SPELLBOOK_REMOVECURSE:
		case SPELLBOOK_LIGHT:
		case SPELLBOOK_IDENTIFY:
		case SPELLBOOK_MAGICMAPPING:
		case SPELLBOOK_SLEEP:
		case SPELLBOOK_CONFUSE:
		case SPELLBOOK_SLOW:
		case SPELLBOOK_OPENING:
		case SPELLBOOK_LOCKING:
		case SPELLBOOK_LEVITATION:
		case SPELLBOOK_INVISIBILITY:
		case SPELLBOOK_TELEPORTATION:
		case SPELLBOOK_HEALING:
		case SPELLBOOK_EXTRAHEALING:
		case SPELLBOOK_CUREAILMENT:
		case SPELLBOOK_DIG:
			item_Spellbook(item,player);
			break;
		case GEM_ROCK:
		case GEM_LUCK:
		case GEM_GARNET:
		case GEM_RUBY:
		case GEM_JACINTH:
		case GEM_AMBER:
		case GEM_CITRINE:
		case GEM_JADE:
		case GEM_EMERALD:
		case GEM_SAPPHIRE:
		case GEM_AQUAMARINE:
		case GEM_AMETHYST:
		case GEM_FLUORITE:
		case GEM_OPAL:
		case GEM_DIAMOND:
		case GEM_JETSTONE:
		case GEM_OBSIDIAN:
		case GEM_GLASS:
			equipItem(item,&stats[player]->weapon,player);
			break;
		case TOOL_PICKAXE:
			equipItem(item,&stats[player]->weapon,player);
			break;
		case TOOL_TINOPENER:
			item_ToolTinOpener(item, player);
			break;
		case TOOL_MIRROR:
			item_ToolMirror(item, player);
			break;
		case TOOL_LOCKPICK:
		case TOOL_SKELETONKEY:
			equipItem(item,&stats[player]->weapon,player);
			break;
		case TOOL_TORCH:
		case TOOL_LANTERN:
			equipItem(item,&stats[player]->shield,player);
			break;
		case TOOL_BLINDFOLD:
			equipItem(item,&stats[player]->mask,player);
			break;
		case TOOL_TOWEL:
			item_ToolTowel(item, player);
			if( multiplayer==CLIENT )
				if( stats[player]->EFFECTS[EFF_BLEEDING] )
					consumeItem(item);
			break;
		case TOOL_GLASSES:
			equipItem(item,&stats[player]->mask,player);
			break;
		case TOOL_BEARTRAP:
			item_ToolBeartrap(item, player);
			break;
		case FOOD_BREAD:
		case FOOD_CREAMPIE:
		case FOOD_CHEESE:
		case FOOD_APPLE:
		case FOOD_MEAT:
		case FOOD_FISH:
			item_Food(item, player);
			break;
		case FOOD_TIN:
			item_FoodTin(item, player);
			break;
		case READABLE_BOOK:
			if (numbooks && player == clientnum)
			{
				if (players[player] && players[player]->entity)
				{
					if (!players[player]->entity->isBlind())
					{
						openBook(books[item->appearance%numbooks], item);
						conductIlliterate = FALSE;
					}
					else
					{
						messagePlayer(player, language[970]);
					}
				}
			}
			break;
		case SPELL_ITEM: {
			;
			spell_t *spell = getSpellFromItem(item);
			if (spell)
				equipSpell(spell, player);
			break;
		}
		case ARTIFACT_SWORD:
			equipItem(item,&stats[player]->weapon,player);
			break;
		case ARTIFACT_MACE:
			if( player==clientnum )
				messagePlayer(clientnum,language[1096]);
			equipItem(item,&stats[player]->weapon,player);
			break;
		case ARTIFACT_SPEAR:
		case ARTIFACT_AXE:
		case ARTIFACT_BOW:
			equipItem(item,&stats[player]->weapon,player);
			break;
		default:
			printlog("error: item %d used, but it has no use case!\n",(int)item->type);
			break;
	}
}

/*-------------------------------------------------------------------------------

	itemPickup

	gives the supplied item to the specified player. Returns the item

-------------------------------------------------------------------------------*/

Item *itemPickup(int player,Item *item) {
	if (!item)
		return NULL;
	Item *item2;
	node_t *node;

	if( player!=0 && multiplayer==SERVER ) {
		// send the client info on the item it just picked up
		strcpy((char *)net_packet->data,"ITEM");
		SDLNet_Write32((Uint32)item->type,&net_packet->data[4]);
		SDLNet_Write32((Uint32)item->status,&net_packet->data[8]);
		SDLNet_Write32((Uint32)item->beatitude,&net_packet->data[12]);
		SDLNet_Write32((Uint32)item->count,&net_packet->data[16]);
		SDLNet_Write32((Uint32)item->appearance,&net_packet->data[20]);
		net_packet->data[24] = item->identified;
		net_packet->address.host = net_clients[player-1].host;
		net_packet->address.port = net_clients[player-1].port;
		net_packet->len = 25;
		sendPacketSafe(net_sock, -1, net_packet, player-1);
	} else {
		for( node=stats[player]->inventory.first; node!=NULL; node=node->next ) {
			item2 = (Item *) node->element;
			if(!itemCompare(item,item2)) {
				item2->count += item->count;
				return item2;
			}
		}
		item2 = newItem(item->type,item->status,item->beatitude,item->count,item->appearance,item->identified,&stats[player]->inventory);
		return item2;
	}
	
	return item;
}

/*-------------------------------------------------------------------------------

	newItemFromEntity
	
	returns a pointer to an item struct from the given entity if it's an
	"item" entity, and returns NULL if the entity is anything else

-------------------------------------------------------------------------------*/

Item *newItemFromEntity(Entity *entity) {
	if( entity==NULL )
		return NULL;
	return newItem(static_cast<ItemType>(entity->skill[10]), static_cast<Status>(entity->skill[11]),entity->skill[12],entity->skill[13],entity->skill[14],entity->skill[15],NULL);
}

/*-------------------------------------------------------------------------------

	itemSlot
	
	returns a pointer to the equipment slot in which the item is residing,
	or NULL if the item isn't stored in an equipment slot

-------------------------------------------------------------------------------*/

Item **itemSlot(Stat *myStats, Item *item) {
	if( !myStats || !item )
		return NULL;
	if(!itemCompare(item,myStats->helmet))
		return &myStats->helmet;
	if(!itemCompare(item,myStats->breastplate))
		return &myStats->breastplate;
	if(!itemCompare(item,myStats->gloves))
		return &myStats->gloves;
	if(!itemCompare(item,myStats->shoes))
		return &myStats->shoes;
	if(!itemCompare(item,myStats->shield))
		return &myStats->shield;
	if(!itemCompare(item,myStats->weapon))
		return &myStats->weapon;
	if(!itemCompare(item,myStats->cloak))
		return &myStats->cloak;
	if(!itemCompare(item,myStats->amulet))
		return &myStats->amulet;
	if(!itemCompare(item,myStats->ring))
		return &myStats->ring;
	if(!itemCompare(item,myStats->mask))
		return &myStats->mask;
	return NULL;
}

/*-------------------------------------------------------------------------------

	itemIsEquipped

	returns 1 if the passed item is equipped on the passed player number, otherwise returns 0

-------------------------------------------------------------------------------*/

bool itemIsEquipped(Item *item, int player) {
	if( !itemCompare(item,stats[player]->helmet) )
		return TRUE;
	if( !itemCompare(item,stats[player]->breastplate) )
		return TRUE;
	if( !itemCompare(item,stats[player]->gloves) )
		return TRUE;
	if( !itemCompare(item,stats[player]->shoes) )
		return TRUE;
	if( !itemCompare(item,stats[player]->shield) )
		return TRUE;
	if( !itemCompare(item,stats[player]->weapon) )
		return TRUE;
	if( !itemCompare(item,stats[player]->cloak) )
		return TRUE;
	if( !itemCompare(item,stats[player]->amulet) )
		return TRUE;
	if( !itemCompare(item,stats[player]->ring) )
		return TRUE;
	if( !itemCompare(item,stats[player]->mask) )
		return TRUE;
	
	return FALSE;
}

/*-------------------------------------------------------------------------------

	Item::weaponGetAttack

	returns the attack power of the given item

-------------------------------------------------------------------------------*/

Sint32 Item::weaponGetAttack() {
	Sint32 attack = beatitude;
	if( itemCategory(this) == MAGICSTAFF )
		attack += 6;
	else if( type == SLING )
		attack += 4;
	else if( type == QUARTERSTAFF )
		attack += 4;
	else if( type == BRONZE_SWORD )
		attack += 4;
	else if( type == BRONZE_MACE )
		attack += 4;
	else if( type == BRONZE_AXE )
		attack += 4;
	else if( type == IRON_SPEAR )
		attack += 5;
	else if( type == IRON_SWORD )
		attack += 5;
	else if( type == IRON_MACE )
		attack += 5;
	else if( type == IRON_AXE )
		attack += 5;
	else if( type == SHORTBOW )
		attack += 8;
	else if( type == STEEL_HALBERD )
		attack += 6;
	else if( type == STEEL_SWORD )
		attack += 6;
	else if( type == STEEL_MACE )
		attack += 6;
	else if( type == STEEL_AXE )
		attack += 6;
	else if( type == CROSSBOW )
		attack += 8;
	else if( type == ARTIFACT_SWORD )
		attack += 10;
	else if( type == ARTIFACT_MACE )
		attack += 10;
	else if( type == ARTIFACT_SPEAR )
		attack += 10;
	else if( type == ARTIFACT_AXE )
		attack += 10;
	else if( type == ARTIFACT_BOW )
		attack += 15;
	attack *= (double)(status/5.0);

	return attack;
}

/*-------------------------------------------------------------------------------

	Item::armorGetAC

	returns the armor value of the given item

-------------------------------------------------------------------------------*/

Sint32 Item::armorGetAC() {
	Sint32 armor = beatitude;
	if( type == LEATHER_HELM )
		armor += 1;
	else if( type == IRON_HELM )
		armor += 2;
	else if( type == STEEL_HELM )
		armor += 3;
	else if( type == LEATHER_BREASTPIECE )
		armor += 2;
	else if( type == IRON_BREASTPIECE )
		armor += 3;
	else if( type == STEEL_BREASTPIECE )
		armor += 4;
	else if( type == GLOVES || type == GLOVES_DEXTERITY )
		armor += 1;
	else if( type == BRACERS || type == BRACERS_CONSTITUTION )
		armor += 2;
	else if( type == GAUNTLETS || type == GAUNTLETS_STRENGTH )
		armor += 3;
	else if( type == LEATHER_BOOTS || type == LEATHER_BOOTS_SPEED )
		armor += 1;
	else if( type == IRON_BOOTS || type == IRON_BOOTS_WATERWALKING )
		armor += 2;
	else if( type == STEEL_BOOTS || type == STEEL_BOOTS_LEVITATION || type == STEEL_BOOTS_FEATHER )
		armor += 3;
	else if( type == WOODEN_SHIELD )
		armor += 1;
	else if( type == BRONZE_SHIELD )
		armor += 2;
	else if( type == IRON_SHIELD )
		armor += 3;
	else if( type == STEEL_SHIELD || type == STEEL_SHIELD_RESISTANCE )
		armor += 4;
	else if( type == CLOAK_PROTECTION )
		armor += 1;
	else if( type == RING_PROTECTION )
		armor += 1;
	//armor *= (double)(item->status/5.0);

	return armor;
}

/*-------------------------------------------------------------------------------

	Item::canUnequip

	returns TRUE if the item may be unequipped (ie it isn't cursed)

-------------------------------------------------------------------------------*/

bool Item::canUnequip()
{
	/*
	//Spellbooks are no longer equipable.
	if (type >= 100 && type <= 121) { //Spellbooks always unequipable regardless of cursed.
		return TRUE;
	}*/

	if (beatitude < 0) {
		identified = TRUE;
		return FALSE;
	}

	return TRUE;
}

/*-------------------------------------------------------------------------------

	Item::buyValue

	returns value of an item to be bought by the given player

-------------------------------------------------------------------------------*/

int Item::buyValue(int player) {
	int value = items[type].value; // base value
	
	// identified bonus
	if( identified ) {
		value *= .8;
	} else {
		if( type == GEM_GLASS ) {
			value = 1400;
		} else {
			value *= 1.25;
		}
	}
	
	// cursed and status bonuses
	value *= 1.f + beatitude/20.f;
	value *= ((int)status+5)/10.f;
	
	// trading bonus
	value /= (50+stats[player]->PROFICIENCIES[PRO_TRADING])/150.f;
	
	// charisma bonus
	value /= 1.f + stats[player]->CHR/20.f;
	
	// result
	value = std::max(1,value);
	return std::max(value,items[type].value);
}

/*-------------------------------------------------------------------------------

	Item::sellValue

	returns value of an item to be sold by the given player

-------------------------------------------------------------------------------*/

int Item::sellValue(int player) {
	int value = items[type].value; // base value
	
	// identified bonus
	if ( identified ) {
		value *= 1.20;
	} else {
		if( itemCategory(this) == GEM ) {
			value = items[GEM_GLASS].value;
		} else {
			value *= .75;
		}
	}
	
	// cursed and status bonuses
	value *= 1.f + beatitude/20.f;
	value *= ((int)status+5)/10.f;
	
	// trading bonus
	value *= (50+stats[player]->PROFICIENCIES[PRO_TRADING])/150.f;
	
	// charisma bonus
	value *= 1.f + stats[player]->CHR/20.f;
	
	// result
	value = std::max(1,value);
	return std::min(value,items[type].value);
}

/*-------------------------------------------------------------------------------

	Item::apply
	
	Applies the given item from the given player to the given entity
	(ie for key unlocking door)

-------------------------------------------------------------------------------*/

void Item::apply(int player, Entity *entity) {
	// for clients:
	if ( multiplayer==CLIENT ) {
		strcpy((char *)net_packet->data,"APIT");
		SDLNet_Write32((Uint32)type,&net_packet->data[4]);
		SDLNet_Write32((Uint32)status,&net_packet->data[8]);
		SDLNet_Write32((Uint32)beatitude,&net_packet->data[12]);
		SDLNet_Write32((Uint32)count,&net_packet->data[16]);
		SDLNet_Write32((Uint32)appearance,&net_packet->data[20]);
		net_packet->data[24] = identified;
		net_packet->data[25] = player;
		SDLNet_Write32((Uint32)entity->uid,&net_packet->data[26]);
		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		net_packet->len = 30;
		sendPacketSafe(net_sock, -1, net_packet, 0);
		return;
	}

	// effects
	if ( type == TOOL_SKELETONKEY ) {
		// skeleton key
		if ( entity->behavior == &actChest ) {
			playSoundEntity(entity,91,64);
			if ( entity->skill[4] ) {
				messagePlayer(player,language[1097]);
				entity->skill[4] = 0;
			} else {
				messagePlayer(player,language[1098]);
				entity->skill[4] = 1;
			}
		} else if ( entity->behavior == &actDoor ) {
			playSoundEntity(entity,91,64);
			if ( entity->skill[5] ) {
				messagePlayer(player,language[1099]);
				entity->skill[5] = 0;
			} else {
				messagePlayer(player,language[1100]);
				entity->skill[5] = 1;
			}
		} else {
			messagePlayer(player,language[1101], getName());
		}
	} else if ( type == TOOL_LOCKPICK ) {
		// lockpicks
		if ( entity->behavior == &actChest ) {
			if ( entity->skill[4] ) {
				if ( stats[player]->PROFICIENCIES[PRO_LOCKPICKING] > rand()%400 ) {
					playSoundEntity(entity,91,64);
					messagePlayer(player,language[1097]);
					entity->skill[4] = 0;
					players[player]->entity->increaseSkill(PRO_LOCKPICKING);
				} else {
					playSoundEntity(entity,92,64);
					messagePlayer(player,language[1102]);
					if (rand()%10 == 0)
					{
						players[player]->entity->increaseSkill(PRO_LOCKPICKING);
					} else {
						if ( rand()%5==0 ) {
							if ( player==clientnum ) {
								if ( count>1 ) {
									newItem(type,status,beatitude,count-1,appearance,identified,&stats[player]->inventory);
								}
							}
							stats[player]->weapon->count = 1;
							stats[player]->weapon->status = static_cast<Status>(stats[player]->weapon->status - 1);
							if ( status != BROKEN ) {
								messagePlayer(player,language[1103]);
							} else {
								messagePlayer(player,language[1104]);
							}
							if ( player>0 && multiplayer==SERVER ) {
								strcpy((char *)net_packet->data,"ARMR");
								net_packet->data[4]=5;
								net_packet->data[5]=stats[player]->weapon->status;
								net_packet->address.host = net_clients[player-1].host;
								net_packet->address.port = net_clients[player-1].port;
								net_packet->len = 6;
								sendPacketSafe(net_sock, -1, net_packet, player-1);
							}
						}
					}
				}
			} else {
				messagePlayer(player,language[1105]);
			}
		} else if ( entity->behavior == &actDoor ) {
			if ( entity->skill[5] ) {
				if ( stats[player]->PROFICIENCIES[PRO_LOCKPICKING] > rand()%400 ) {
					playSoundEntity(entity,91,64);
					messagePlayer(player,language[1099]);
					entity->skill[5] = 0;
					players[player]->entity->increaseSkill(PRO_LOCKPICKING);
				} else {
					playSoundEntity(entity,92,64);
					messagePlayer(player,language[1106]);
					if ( rand()%10==0 ) {
						players[player]->entity->increaseSkill(PRO_LOCKPICKING);
					} else {
						if ( rand()%5==0 ) {
							if ( player==clientnum ) {
								if ( count>1 ) {
									newItem(type,status,beatitude,count-1,appearance,identified,&stats[player]->inventory);
								}
							}
							stats[player]->weapon->count = 1;
							stats[player]->weapon->status = static_cast<Status>(stats[player]->weapon->status - 1);
							if ( status != BROKEN ) {
								messagePlayer(player,language[1103]);
							} else {
								messagePlayer(player,language[1104]);
							}
							if ( player>0 && multiplayer==SERVER ) {
								strcpy((char *)net_packet->data,"ARMR");
								net_packet->data[4]=5;
								net_packet->data[5]=stats[player]->weapon->status;
								net_packet->address.host = net_clients[player-1].host;
								net_packet->address.port = net_clients[player-1].port;
								net_packet->len = 6;
								sendPacketSafe(net_sock, -1, net_packet, player-1);
							}
						}
					}
				}
			} else {
				messagePlayer(player,language[1107]);
			}
		} else {
			messagePlayer(player,language[1101], getName());
		}
	}
}

SummonProperties::SummonProperties()
{
	//TODO:
}

SummonProperties::~SummonProperties()
{
	//TODO:
}