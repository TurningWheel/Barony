/*-------------------------------------------------------------------------------

	BARONY
	File: actchest.cpp
	Desc: implements all chest related code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "interface/interface.hpp"
#include "items.hpp"
#include "sound.hpp"
#include "net.hpp"

#define CHEST_INIT my->skill[0]
#define CHEST_STATUS my->skill[1] //0 = closed. 1 = open.
#define CHEST_HEALTH my->skill[3]
#define CHEST_LOCKED my->skill[4] //0 = unlocked. 1 = locked.
#define CHEST_OPENER my->skill[5] //Index of the player the chest was opened by.
#define CHEST_LIDCLICKED my->skill[6]
#define CHEST_AMBIENCE my->skill[7]
#define CHEST_MAXHEALTH my->skill[8]

/*
 * Chest theme ideas:
"random"
"empty / tiny amount of worthless garbage"
"food"
"treasures, jewelry, gems"
"weapons / armor"
"tools"
"spellbooks / scrolls"
"magicstaffs"
"potions"
 */

//chest->children->first is the chest's inventory.

void actChest(Entity *my) {
	if (!my)
		return;

	//TODO: Sounds.
	CHEST_AMBIENCE--;
	if( CHEST_AMBIENCE<=0 ) {
		CHEST_AMBIENCE = TICKS_PER_SECOND*30;
		playSoundEntityLocal( my, 149, 64 );
	}

	if( multiplayer==CLIENT )
		return;

	//TODO: Visual effects.

	int i;

	if (!CHEST_INIT) {
		CHEST_INIT = 1;
		CHEST_HEALTH = 90+rand()%20;
		CHEST_MAXHEALTH = CHEST_HEALTH;
		if (rand()%10 == 0) // 10% chance
			CHEST_LOCKED=1;

		node_t *node = NULL;
		node = list_AddNodeFirst(&my->children);
		node->element = malloc(sizeof(list_t)); //Allocate memory for the inventory list.
		node->deconstructor = &listDeconstructor;
		list_t *inventory = (list_t *) node->element;
		inventory->first = NULL; inventory->last = NULL;

		int itemcount = 0;

		int chesttype = 0;
		if( strcmp(map.name,"The Mystic Library") ) {
			chesttype = rand()%8;
		} else {
			chesttype = 6; // magic chest
		}

		switch (chesttype) { //Note that all of this needs to be properly balanced over time.
			//TODO: Make all applicable item additions work on a category based search?
			case 0:
				//Completely random.
				itemcount = (rand()%5) + 1;
				for (i = 0; i < itemcount; ++i) {
					//And add the current entity to it.
					int itemnum = rand() % NUMITEMS;
					while (itemnum == SPELL_ITEM)
						itemnum = rand() % NUMITEMS; //Keep trying until you don't get a spell.
					newItem(static_cast<ItemType>(itemnum), static_cast<Status>(WORN+rand()%3), 0, 1, rand(), FALSE, inventory);
				}
				break;
			case 1:
				//Garbage chest
				if (rand()%2) {
					//Empty.
				} else {
					//Some worthless garbage. Like a rock. //TODO: Sometimes spawn item 139, worthless piece of glass. Maybe go a step further and have a random amount of items, say 1 - 5, and they can be either rock or the worthless piece of glass or any other garbage.
					newItem(GEM_ROCK, static_cast<Status>(WORN+rand()%3), 0, 1, rand(), FALSE, inventory);
				}
				break;
			case 2:
				//Food.
				//Items 152 - 158 are all food.
				itemcount = (rand()%5) + 1;
				for (i = 0; i < itemcount; ++i) {
					newItem(static_cast<ItemType>(FOOD_BREAD + (rand()%7)), static_cast<Status>(WORN+rand()%3), 0, 1, rand(), FALSE, inventory);
				}
				break;
			case 3:
				//Treasures, jewelry, gems 'n stuff.
				itemcount = (rand()%5) + 1;
				for (i = 0; i < itemcount; ++i) {
					if( rand()%4 )
						newItem(static_cast<ItemType>(GEM_GARNET + rand()%15), static_cast<Status>(WORN+rand()%3), 0, 1, rand(), FALSE, inventory);
					else
						newItem(GEM_GLASS, static_cast<Status>(WORN+rand()%3), 0, 1, rand(), FALSE, inventory);
				}
				//Random chance to spawn a ring or an amulet or some other jewelry.
				if (rand()%2) {
					if (rand()%2) {
						//Spawn a ring.
						newItem(static_cast<ItemType>(RING_ADORNMENT + rand()%12), static_cast<Status>(WORN+rand()%3), 0, 1, rand(), FALSE, inventory);
					} else {
						//Spawn an amulet.
						newItem(static_cast<ItemType>(AMULET_SEXCHANGE + rand()%6), static_cast<Status>(WORN+rand()%3), 0, 1, rand(), FALSE, inventory);
					}
				}
				break;
			case 4:
				//Weapons, armor, stuff.
				//Further break this down into either spawning only weapon(s), only armor(s), or a combo, like a set.

				switch (rand()%3) { //TODO: Note, switch to rand()%4 if/when case 3 is implemented.
					case 0:
						//Only a weapon. Items 0 - 16.
						{
							int item = rand()%18;
							//Since the weapons are not a continuous set, check to see if the weapon is part of the continuous set. If it is not, move on to the next block. In this case, there's only one weapon that is not part of the continous set: the crossbow.
							if (item < 16)
								//Almost every weapon.
								newItem(static_cast<ItemType>(rand()%17), static_cast<Status>(WORN+rand()%3), 0, 1, rand(), FALSE, inventory);
							else
								//Crossbow.
								newItem(CROSSBOW, static_cast<Status>(WORN+rand()%3), 0, 1, rand(), FALSE, inventory);
						}
						break;
					case 1:
						//Only a piece of armor.
						{
							/*
							 * 0 - 1 are the steel shields, items 17 and 18.
							 * 2 - 5 are the gauntlets, items 20 - 23.
							 * 6 - 15 are the boots & shirts (as in, breastplates and all variants), items 28 - 37.
							 * 16 - 19 are the hats & helmets, items 40 - 43
							 */
							int item = rand()%20;
							if (item <= 1)
								//Steel shields. Items 17 & 18.
								newItem(static_cast<ItemType>(17 + rand()%2), static_cast<Status>(WORN+rand()%3), 0, 1, rand(), FALSE, inventory);
							else if (item <= 5)
								//Gauntlets. Items 20 - 23.
								newItem(static_cast<ItemType>(20 + rand()%4), static_cast<Status>(WORN+rand()%3), 0, 1, rand(), FALSE, inventory);
							else if (item <= 15)
								//Boots & shirts. Items 28 - 37.
								newItem(static_cast<ItemType>(28 + rand()%10), static_cast<Status>(WORN+rand()%3), 0, 1, rand(), FALSE, inventory);
							else if (item <= 10)
								//Hats & helmets. Items 40 - 43.
								newItem(static_cast<ItemType>(40+rand()%4), static_cast<Status>(WORN+rand()%3), 0, 1, rand(), FALSE, inventory);
						}
						break;
					case 2:
						//A weapon and an armor.
						{
							int item = rand()%18;
							//Since the weapons are not a continuous set, check to see if the weapon is part of the continuous set. If it is not, move on to the next block. In this case, there's only one weapon that is not part of the continous set: the crossbow.
							if (item < 16)
								//Almost every weapon.
								newItem(static_cast<ItemType>(rand()%17), static_cast<Status>(WORN+rand()%3), 0, 1, rand(), FALSE, inventory);
							else
								//Crossbow.
								newItem(static_cast<ItemType>(19), static_cast<Status>(WORN+rand()%3), 0, 1, rand(), FALSE, inventory);

							/*
							 * 0 - 1 are the steel shields, items 17 and 18.
							 * 2 - 5 are the gauntlets, items 20 - 23.
							 * 6 - 15 are the boots & shirts (as in, breastplates and all variants), items 28 - 37.
							 * 16 - 19 are the hats & helmets, items 40 - 43
							 */
							item = rand()%20;
							if (item <= 1)
								//Steel shields. Items 17 & 18.
								newItem(static_cast<ItemType>(17 + rand()%2), static_cast<Status>(WORN+rand()%3), 0, 1, rand(), FALSE, inventory);
							else if (item <= 5)
								//Gauntlets. Items 20 - 23.
								newItem(static_cast<ItemType>(20 + rand()%4), static_cast<Status>(WORN+rand()%3), 0, 1, rand(), FALSE, inventory);
							else if (item <= 15)
								//Boots & shirts. Items 28 - 37.
								newItem(static_cast<ItemType>(28 + rand()%10), static_cast<Status>(WORN+rand()%3), 0, 1, rand(), FALSE, inventory);
							else if (item <= 10)
								//Hats & helmets. Items 40 - 43.
								newItem(static_cast<ItemType>(40+rand()%4), static_cast<Status>(WORN+rand()%3), 0, 1, rand(), FALSE, inventory);
						}
						break;
					case 3:
						//TODO: Rarer. Getting a full set of armor + a weapon.
						break;
				}
				break;
			case 5:
				//Tools.
				itemcount = 1 + rand()%2;
				for (i = 0; i < itemcount; ++i) {
					newItem(static_cast<ItemType>(TOOL_PICKAXE+rand()%12), static_cast<Status>(WORN+rand()%3), 0, 1, rand(), FALSE, inventory);
				}
				break;
			case 6:
				//Magic chest.
				//So first choose what kind of magic chest it is.
				{
					/*
					 * Types:
					 * * Scroll chest. Has some scrolls in it ( 3 - 5).
					 * * Book chest. Basically a small library. 1-3 books.
					 * * Staff chest. Staff or 2.
					 * * Wizard's chest, which will contain 1-2 scrolls, a magic book, a staff, and either a wizard/magician/whatever implement of some sort or a piece of armor.
					 */
					int magic_type = rand()%4;

					switch (magic_type) {
						case 0:
							//Have 3-5 scrolls.
							itemcount = 3 + (rand()%3);
							for (i = 0; i < itemcount; ++i) {
								newItem(static_cast<ItemType>(SCROLL_IDENTIFY + rand()%12), static_cast<Status>(WORN+rand()%3), 0, 1, rand(), FALSE, inventory);
							}
							break;
						case 1:
							//Have 1-3 books.
							itemcount = 1 + (rand()%3);
							for (i = 0; i < itemcount; ++i) {
								newItem(static_cast<ItemType>(SPELLBOOK_FORCEBOLT + rand()%22), static_cast<Status>(WORN+rand()%3), 0, 1, rand(), FALSE, inventory);
							}
							break;
						case 2:
							//A staff.
							newItem(static_cast<ItemType>(MAGICSTAFF_LIGHT + rand()%10), static_cast<Status>(WORN+rand()%3), 0, 1, rand(), FALSE, inventory);
							break;
						case 3:
							//So spawn several items at once. A wizard's chest!

							//First the scrolls (1 - 2).
							itemcount = 1 + rand()%2;
							for (i = 0; i < itemcount; ++i) {
								newItem(static_cast<ItemType>(SCROLL_IDENTIFY + rand()%12), static_cast<Status>(WORN+rand()%3), 0, 1, rand(), FALSE, inventory);
							}

							newItem(static_cast<ItemType>(SPELLBOOK_FORCEBOLT + rand()%22), static_cast<Status>(WORN+rand()%3), 0 , 1, rand(), FALSE, inventory);

							newItem(static_cast<ItemType>(MAGICSTAFF_LIGHT + rand()%10), static_cast<Status>(WORN+rand()%3), 0, 1, rand(), FALSE, inventory);

							switch (rand()%6) {
								case 0:
									//A cloak. Item 24.
									newItem(CLOAK, static_cast<Status>(WORN+rand()%3), 0, 1, rand(), FALSE, inventory);
									break;
								case 1:
									//A cloak of magic resistance. Item 25.
									newItem(CLOAK_MAGICREFLECTION, static_cast<Status>(WORN+rand()%3), 0, 1, rand(), FALSE, inventory);
									break;
								case 2:
									//A cloak of invisibility. Item 26.
									newItem(CLOAK_INVISIBILITY, static_cast<Status>(WORN+rand()%3), 0, 1, rand(), FALSE, inventory);
									break;
								case 3:
									//A cloak of protection. Item 27.
									newItem(CLOAK_PROTECTION, static_cast<Status>(WORN+rand()%3), 0, 1, rand(), FALSE, inventory);
									break;
								case 4:
									//A phyregian's hat. Item 38.
									newItem(HAT_PHRYGIAN, static_cast<Status>(WORN+rand()%3), 0, 1, rand(), FALSE, inventory);
									break;
								case 5:
									//A wizard's hat. Item 39.
									newItem(HAT_WIZARD, static_cast<Status>(WORN+rand()%3), 0, 1, rand(), FALSE, inventory);
									break;
							}
							break;
					}
				}
				break;
			case 7:
				//Potions.
				//Items 50 - 64 are potions.
				itemcount = (rand()%3) + 1;
				for (i = 0; i < itemcount; ++i) {
					newItem(static_cast<ItemType>(POTION_WATER + (rand()%15)), static_cast<Status>(WORN+rand()%3), 0, 1, rand(), FALSE, inventory);
				}
				break;
			default:
				//Default case. Should never be reached.
				newItem(static_cast<ItemType>(0), BROKEN, 0, 1, rand(), FALSE, inventory);
				printlog( "warning: default cause in chest init theme type reached. This should never happen.");
				break;
		}
	}

	list_t *inventory = static_cast<list_t* >(my->children.first->element);
	node_t *node = NULL;
	Item *item = NULL;

	if( CHEST_HEALTH<=0 ) {
		// the chest busts open, drops some items randomly, then destroys itself.
		node_t *nextnode;
		for( node=inventory->first; node!=NULL; node=nextnode ) {
			nextnode = node->next;
			item = (Item *)node->element;
			if( rand()%2==0 )
				dropItemMonster(item, my, NULL);
		}
		
		// wood chunk particles
		int c;
		for( c=0; c<10; c++ ) {
			Entity *entity = spawnGib(my);
			entity->flags[INVISIBLE] = FALSE;
			entity->sprite = 187; // Splinter.vox
			entity->x = floor(my->x/16)*16+8;
			entity->y = floor(my->y/16)*16+8;
			entity->z = -7+rand()%14;
			entity->yaw = (rand()%360)*PI/180.0;
			entity->pitch = (rand()%360)*PI/180.0;
			entity->roll = (rand()%360)*PI/180.0;
			entity->vel_x = cos(entity->yaw)*(0.5+(rand()%100)/100.f);
			entity->vel_y = sin(entity->yaw)*(0.5+(rand()%100)/100.f);
			entity->vel_z = -.25;
			entity->fskill[3] = 0.04;
			serverSpawnGibForClient(entity);
		}
		playSoundEntity(my,177,64);

		// remove chest entities
		Entity *parent = uidToEntity(my->parent);
		if( parent )
			list_RemoveNode(parent->mynode); // remove lid
		list_RemoveNode(my->mynode); // remove me
		return;
	}

	if (CHEST_STATUS == 1) {
		if (players[CHEST_OPENER]) {
			unsigned int distance = sqrt(pow(my->x - players[CHEST_OPENER]->x, 2) + pow(my->y - players[CHEST_OPENER]->y, 2));
			if (distance > TOUCHRANGE) {
				my->closeChest();
			}
		} else {
			my->closeChest();
		}
	}

	//Using the chest (TODO: Monsters using it?).
	int chestclicked=-1;
	for (i = 0; i < MAXPLAYERS; ++i) {
		if ( (i==0 && selectedEntity==my) || (client_selected[i]==my) ) {
			if (inrange[i]) {
				chestclicked = i;
			}
		}
	}
	if( CHEST_LIDCLICKED ) {
		chestclicked = CHEST_LIDCLICKED-1;
		CHEST_LIDCLICKED = 0;
	}
	if( chestclicked >= 0 ) {
		if (!CHEST_LOCKED && !openedChest[chestclicked]) {
			if (!CHEST_STATUS) {
				messagePlayer(chestclicked, language[459]);
				CHEST_OPENER = chestclicked;
				openedChest[chestclicked] = my;
				if (chestclicked != 0 && multiplayer == SERVER) {
					//TODO: Tell the client that it just opened a chest.
					//Send all of the items to the client.
					strcpy((char *)net_packet->data, "CHST"); //Chest. //TODO: When the client recieves this message, let it know it's getting the UID of the chest it's interacting with.
					SDLNet_Write32((Uint32)my->uid, &net_packet->data[4]); //Give the client the UID.
					net_packet->address.host = net_clients[chestclicked - 1].host;
					net_packet->address.port = net_clients[chestclicked - 1].port;
					net_packet->len = 8;
					sendPacketSafe(net_sock, -1, net_packet, chestclicked-1);
					for (node = inventory->first; node != NULL; node = node->next) {
						item = (Item *) node->element;
						strcpy((char *)net_packet->data,"CITM"); //Chest item. //TODO: When the client recieves this message, shootmode = false, and start pumping the clientside chest inventory full of items.
						SDLNet_Write32((Uint32)item->type,&net_packet->data[4]);
						SDLNet_Write32((Uint32)item->status,&net_packet->data[8]);
						SDLNet_Write32((Uint32)item->beatitude,&net_packet->data[12]);
						SDLNet_Write32((Uint32)item->count,&net_packet->data[16]);
						SDLNet_Write32((Uint32)item->appearance,&net_packet->data[20]);
						net_packet->data[24] = item->identified;
						net_packet->address.host = net_clients[chestclicked-1].host;
						net_packet->address.port = net_clients[chestclicked-1].port;
						net_packet->len = 25;
						sendPacketSafe(net_sock, -1, net_packet, chestclicked-1);
					}
				} else {
					shootmode = FALSE;
					gui_mode = GUI_MODE_INVENTORY; //Set it to the inventory screen so that the player can see the chest.
				}
				CHEST_STATUS = 1; //Toggle chest open/closed.
			} else {
				messagePlayer(chestclicked, language[460]);
				if (CHEST_OPENER != 0) {
					//TODO: Message the client.
					strcpy((char *)net_packet->data,"CCLS"); //Chest close. //TODO: Make the client react to this message and close the chest.
					net_packet->address.host = net_clients[CHEST_OPENER-1].host;
					net_packet->address.port = net_clients[CHEST_OPENER-1].port;
					net_packet->len = 4;
					sendPacketSafe(net_sock, -1, net_packet, CHEST_OPENER-1);
				} else {
					chestitemscroll = 0;
				}
				if (CHEST_OPENER != chestclicked) {
					messagePlayer(CHEST_OPENER, language[461]);
				}
				my->closeChestServer();
			}
		} else if( CHEST_LOCKED ) {
			messagePlayer(chestclicked, language[462]);
			playSoundEntity(my, 152, 64);
		}

		//TODO: Locked chests?

		//TODO: Pop open GUI with inventory. Clicking on an item adds it to the player's inventory. If a player clicks on an item in their inventory, it gets added to the chest's inventory. Right clicking adds/grabs all of a stack.
	}
}

void actChestLid(Entity *my) {
	int i;
	
	Entity *parent = uidToEntity(my->parent);
	if( !parent ) {
		list_RemoveNode(my->mynode);
		return;
	}

	if( multiplayer!=CLIENT ) {
		my->skill[1] = parent->skill[1];
		if( multiplayer==SERVER ) {
			if( my->skill[3] != my->skill[1] ) {
				my->skill[3] = my->skill[1];
				serverUpdateEntitySkill(my,1);
			}
		}
	
		for (i = 0; i < MAXPLAYERS; ++i) {
			if ( (i==0 && selectedEntity==my) || (client_selected[i]==my) ) {
				if (inrange[i]) {
					parent->skill[6] = i+1;
				}
			}
		}
	}
	
	if( my->skill[1] ) {
		// chest is open
		if( !my->skill[0] ) {
			my->skill[0] = 1;
			if( multiplayer!=CLIENT )
				playSoundEntity(my,21,64);
			my->fskill[0] = 0.25;
		}
		if( my->pitch > -PI/2 ) {
			my->pitch -= my->fskill[0];
			my->fskill[0] -= 0.02;
			if( my->pitch <= -PI/2 ) {
				my->pitch = -PI/2;
				my->fskill[0] = 0;
			}
		}
	} else {
		// chest is closed
		if( my->skill[0] ) {
			my->skill[0] = 0;
			if( multiplayer!=CLIENT )
				playSoundEntity(my,22,64);
			my->fskill[0] = 0.025;
		}
		if( my->pitch < 0 ) {
			my->pitch += my->fskill[0];
			my->fskill[0] += 0.025;
			if( my->pitch >= 0 ) {
				my->pitch = 0;
				my->fskill[0] = 0;
			}
		}
	}
}

void Entity::closeChest() {
	if (clientnum != 0 && multiplayer == CLIENT) {
		//If client, tell server the chest got closed.
		if (openedChest[clientnum] != NULL) {
			//Message server.
			messagePlayer(clientnum, language[460]);
			strcpy( (char *)net_packet->data, "CCLS" );
			net_packet->data[4] = clientnum;
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 5;
			sendPacketSafe(net_sock, -1, net_packet, 0);

			closeChestClientside();
			return;
		}
	}

	if (chest_status) {
		chest_status = 0;
		messagePlayer(chest_opener, language[460]);
		openedChest[chest_opener] = NULL;
		if (chest_opener != 0 && multiplayer == SERVER) {
			//Tell the client that the chest got closed.
			strcpy((char *)net_packet->data,"CCLS"); //Chest close.
			net_packet->address.host = net_clients[chest_opener - 1].host;
			net_packet->address.port = net_clients[chest_opener - 1].port;
			net_packet->len = 4;
			sendPacketSafe(net_sock, -1, net_packet, chest_opener - 1);
		} else {
			chestitemscroll = 0;
		}
	}
}

void Entity::closeChestServer() {
	if (chest_status) {
		chest_status = 0;
		openedChest[chest_opener] = NULL;
	}
}

void Entity::addItemToChest(Item *item) {
	if (!item)
		return;

	if (clientnum != 0 && multiplayer == CLIENT) {
		//Tell the server.
		strcpy( (char *)net_packet->data, "CITM" );
		net_packet->data[4] = clientnum;
		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		SDLNet_Write32((Uint32)item->type,&net_packet->data[5]);
		SDLNet_Write32((Uint32)item->status,&net_packet->data[9]);
		SDLNet_Write32((Uint32)item->beatitude,&net_packet->data[13]);
		SDLNet_Write32((Uint32)item->count,&net_packet->data[17]);
		SDLNet_Write32((Uint32)item->appearance,&net_packet->data[21]);
		net_packet->data[25] = item->identified;
		net_packet->len = 26;
		sendPacketSafe(net_sock, -1, net_packet, 0);

		addItemToChestClientside(item);
		return;
	}

	Item *item2 = NULL;

	//Add the item to the chest's inventory.
	list_t *inventory = static_cast<list_t* >(children.first->element);

	node_t *t_node = NULL;
	//If item's already in the chest, add it to a pre-existing stack.
	for (t_node = inventory->first; t_node != NULL; t_node = t_node->next) {
		item2 = (Item *) t_node->element;
		if (!itemCompare(item, item2)) {
			item2->count += item->count;
			return;
		}
	}

	item->node = list_AddNodeFirst(inventory);
	item->node->element = item;
	item->node->deconstructor = &defaultDeconstructor;

	if (chest_opener != 0 && multiplayer == SERVER) {
		strcpy((char *)net_packet->data,"CITM");
		SDLNet_Write32((Uint32)item->type,&net_packet->data[4]);
		SDLNet_Write32((Uint32)item->status,&net_packet->data[8]);
		SDLNet_Write32((Uint32)item->beatitude,&net_packet->data[12]);
		SDLNet_Write32((Uint32)item->count,&net_packet->data[16]);
		SDLNet_Write32((Uint32)item->appearance,&net_packet->data[20]);
		net_packet->data[24] = item->identified;
		net_packet->address.host = net_clients[chest_opener - 1].host;
		net_packet->address.port = net_clients[chest_opener - 1].port;
		net_packet->len = 25;
		sendPacketSafe(net_sock, -1, net_packet, chest_opener - 1);
	}
}

void Entity::addItemToChestFromInventory(int player, Item *item, bool all) {
	if (!item || !players[player])
		return;

	if (itemCategory(item) == SPELL_CAT)
		return;

	if( itemIsEquipped(item, player) == TRUE && !item->canUnequip() ) {
		messagePlayer(player,language[1087]);
		item->identified=TRUE;
		return;
	}
	playSoundPlayer(player,47+rand()%3,64);

	Item *newitem = NULL;
	if ( (newitem = (Item *) malloc(sizeof(Item))) == NULL) {
		printlog( "failed to allocate memory for new item!\n" );
		return; //Error or something.
	}
	newitem->node = NULL;
	newitem->count = 1;
	newitem->type = item->type;
	newitem->status = item->status;
	newitem->beatitude = item->beatitude;
	newitem->appearance = item->appearance;
	newitem->identified = item->identified;

	// unequip the item
	if( item->count <= 1 || all) {
		Item **slot = itemSlot(stats[player],item);
		if( slot != NULL )
			*slot = NULL;
	}
	if( item->node != NULL ) {
		if( item->node->list==&stats[player]->inventory ) {
			if (!all) {
				item->count--;
				if( item->count <= 0 )
					list_RemoveNode(item->node);
			} else {
				newitem->count = item->count;
				list_RemoveNode(item->node);
			}
		}
	} else {
		item->count--;
		if( item->count <= 0 )
			free(item);
	}

	messagePlayer(player, language[463], newitem->getName());
	addItemToChest(newitem);

	return; //Do not execute the rest of this function.
}

Item* Entity::getItemFromChest(Item *item, bool all) {
	Item *newitem = NULL;

	if( item == NULL )
		return NULL;

	if ( clientnum != 0 && multiplayer == CLIENT) {
		if (!item || !item->node)
			return NULL;

		if ( (newitem = (Item *) malloc(sizeof(Item))) == NULL) {
			printlog( "failed to allocate memory for new item!\n" );
			return NULL; //Error or something.
		}
		newitem->node = NULL;
		newitem->count = 1;
		newitem->type = item->type;
		newitem->status = item->status;
		newitem->beatitude = item->beatitude;
		newitem->appearance = item->appearance;
		newitem->identified = item->identified;

		//Tell the server.
		strcpy( (char *)net_packet->data, "RCIT" ); //Have the server handle this (removing an item from the chest).
		net_packet->data[4] = clientnum;
		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		SDLNet_Write32((Uint32)item->type,&net_packet->data[5]);
		SDLNet_Write32((Uint32)item->status,&net_packet->data[9]);
		SDLNet_Write32((Uint32)item->beatitude,&net_packet->data[13]);
		int count = 1;
		if (all)
			count = item->count;
		SDLNet_Write32((Uint32)count,&net_packet->data[17]);
		SDLNet_Write32((Uint32)item->appearance,&net_packet->data[21]);
		net_packet->data[25] = item->identified;
		net_packet->len = 26;
		sendPacketSafe(net_sock, -1, net_packet, 0);
	} else {
		if ( !item )
			return NULL;
		if ( !item->node )
			return NULL;
		if ( item->node->list != children.first->element )
			return NULL;

		if ( (newitem = (Item *) malloc(sizeof(Item))) == NULL) {
			printlog( "failed to allocate memory for new item!\n" );
			return NULL; //Error or something.
		}
		newitem->node = NULL;
		newitem->count = 1;
		newitem->type = item->type;
		newitem->status = item->status;
		newitem->beatitude = item->beatitude;
		newitem->appearance = item->appearance;
		newitem->identified = item->identified;
	}

	if (!all) {
		//Grab only one item from the chest.
		newitem->count=1;
		item->count-=1;
		if( item->count <= 0 )
			list_RemoveNode(item->node);
	} else {
		//Grab all items from the chest.
		newitem->count = item->count;
		list_RemoveNode(item->node);
	}

	return newitem;
}

void closeChestClientside() {
	if (!openedChest[clientnum])
		return;

	if (multiplayer != CLIENT || clientnum == 0)
		return; //Only called for the client.

	list_FreeAll(&chestInv);

	openedChest[clientnum] = NULL;

	chestitemscroll = 0;
}

void addItemToChestClientside(Item *item) {
	if (openedChest[clientnum]) {
		//messagePlayer(clientnum, "Recieved item.");

		//If there's an open chests, add an item to it.
		//TODO: Add item to the chest.

		Item *item2 = NULL;
		node_t *node = NULL;

		for (node = chestInv.first; node != NULL; node = node->next) {
			item2 = (Item *) node->element;
			if (!itemCompare(item, item2)) {
				item2->count += item->count;
				return;
			}
		}

		item->node = list_AddNodeFirst(&chestInv);
		item->node->element = item;
		item->node->deconstructor = &defaultDeconstructor;
	}
	//TODO: Else: Ruh-roh, error!
}



void Entity::addItemToChestServer(Item *item) {
	if (!item)
		return;

	Item *item2 = NULL;
	node_t *t_node = NULL;

	//Add the item to the chest's inventory.
	list_t *inventory = static_cast<list_t* >(children.first->element);

	if (!inventory)
		return;

	//If item's already in the chest, add it to a pre-existing stack.
	for (t_node = inventory->first; t_node != NULL; t_node = t_node->next) {
		item2 = (Item *) t_node->element;
		if (!itemCompare(item, item2)) {
			item2->count += item->count;
			return;
		}
	}

	item->node = list_AddNodeFirst(inventory);
	item->node->element = item;
	item->node->deconstructor = &defaultDeconstructor;
}

void Entity::removeItemFromChestServer(Item *item, int count) {
	if (!item) {
		return;
	}

	Item *item2 = NULL;
	node_t *t_node = NULL;

	list_t *inventory = static_cast<list_t* >(children.first->element);
	if (!inventory) {
		return;
	}

	for (t_node = inventory->first; t_node != NULL; t_node = t_node->next) {
		item2 = (Item *) t_node->element;
		if (!item2  || !item2->node || item2->node->list != children.first->element) {
			return;
		}
		if (!itemCompare(item, item2)) {
			if (count < item2->count) {
				//Grab only one item from the chest.
				int oldcount = item2->count;
				item2->count = oldcount - count;
				if( item2->count <= 0 )
					list_RemoveNode(item2->node);
			} else {
				//Grab all items from the chest.
				list_RemoveNode(item2->node);
			}
			return;
		}
	}
}
