/*-------------------------------------------------------------------------------

	BARONY
	File: clickdescription.cpp
	Desc: contains clickDescription()

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../main.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "../items.hpp"
#include "../magic/magic.hpp"
#include "../monster.hpp"
#include "../net.hpp"
#include "../player.hpp"
#include "interface.hpp"

/*-------------------------------------------------------------------------------

	clickDescription

	reports the name of the entity clicked on in use mode

-------------------------------------------------------------------------------*/

void clickDescription(int player, Entity* entity)
{
	Stat* stat;
	Item* item;
	Uint32 uidnum;

	if ( entity == NULL )
	{
		if ( !(*inputPressed(impulses[IN_ATTACK]) || *inputPressed(joyimpulses[INJOY_MENU_LEFT_CLICK])) || shootmode )
		{
			return;
		}
		//One of either IN_ATTACK or INJOY_MENU_LEFT_CLICK is true, && shootmode == false;
		if ( omousex < camera.winx || omousex >= camera.winx + camera.winw || omousey < camera.winy || omousey >= camera.winy + camera.winh )
		{
			return;
		}
		if ( FollowerMenu.followerMenuIsOpen() )
		{
			return;
		}
		if (openedChest[clientnum])
			if (omousex > CHEST_INVENTORY_X && omousex < CHEST_INVENTORY_X + inventoryChest_bmp->w && omousey > CHEST_INVENTORY_Y && omousey < CHEST_INVENTORY_Y + inventoryChest_bmp->h)
			{
				return;    //Click falls inside the chest inventory GUI.
			}
		if (identifygui_active)
			if (omousex > IDENTIFY_GUI_X && omousex < IDENTIFY_GUI_X + identifyGUI_img->w && omousey > IDENTIFY_GUI_Y && omousey < IDENTIFY_GUI_Y + identifyGUI_img->h)
			{
				return;    //Click falls inside the identify item gui.
			}
		if (book_open)
			if (mouseInBounds(BOOK_GUI_X, BOOK_GUI_X + bookgui_img->w, BOOK_GUI_Y, BOOK_GUI_Y + bookgui_img->h))
			{
				return;    //Click falls inside the book GUI.
			}
		if (gui_mode == GUI_MODE_INVENTORY || gui_mode == GUI_MODE_SHOP)
		{
			if ( gui_mode == GUI_MODE_INVENTORY )
				if (mouseInBounds(RIGHTSIDEBAR_X, RIGHTSIDEBAR_X + rightsidebar_titlebar_img->w, RIGHTSIDEBAR_Y, RIGHTSIDEBAR_Y + rightsidebar_height))
				{
					return;    //Click falls inside the right sidebar.
				}
			//int x = std::max(character_bmp->w, xres/2-inventory_bmp->w/2);
			//if (mouseInBounds(x,x+inventory_bmp->w,0,inventory_bmp->h))
			//return NULL;
			if ( mouseInBounds(INVENTORY_STARTX, INVENTORY_STARTX + INVENTORY_SIZEX * INVENTORY_SLOTSIZE, INVENTORY_STARTY, INVENTORY_STARTY + INVENTORY_SIZEY * INVENTORY_SLOTSIZE) )
			{
				// clicked in inventory
				return;
			}
			if ( gui_mode == GUI_MODE_SHOP )
			{
				int x1 = xres / 2 - SHOPWINDOW_SIZEX / 2, x2 = xres / 2 + SHOPWINDOW_SIZEX / 2;
				int y1 = yres / 2 - SHOPWINDOW_SIZEY / 2, y2 = yres / 2 + SHOPWINDOW_SIZEY / 2;
				if (mouseInBounds(x1, x2, y1, y2))
				{
					return;
				}
			}
		}
		else if (gui_mode == GUI_MODE_MAGIC)
		{
			if (magic_GUI_state == 0)
			{
				//Right, now calculate the spell list's height (the same way it calculates it for itself).
				int height = spell_list_titlebar_bmp->h;
				int numspells = 0;
				node_t* node;
				for (node = spellList.first; node != NULL; node = node->next)
				{
					numspells++;
				}
				int maxSpellsOnscreen = camera.winh / spell_list_gui_slot_bmp->h;
				numspells = std::min(numspells, maxSpellsOnscreen);
				height += numspells * spell_list_gui_slot_bmp->h;
				int spelllist_y = camera.winy + ((camera.winh / 2) - (height / 2)) + magicspell_list_offset_x;

				if (mouseInBounds(MAGICSPELL_LIST_X, MAGICSPELL_LIST_X + spell_list_titlebar_bmp->w, spelllist_y, spelllist_y + height))
				{
					return;
				}
			}
		}
		if (mouseInBounds(0, 224, 0, 420))   // character sheet
		{
			return;
		}
		int x = xres / 2 - (status_bmp->w / 2);
		if (mouseInBounds(x, x + status_bmp->w, yres - status_bmp->h - hotbar_img->h, yres))
		{
			return;
		}
		if ( selectedItem || itemMenuOpen )
		{
			//Will bugger up GUI item interaction if this function continues to run.
			return;
		}

		// ui code taken from drawSkillsSheet() and drawPartySheet().
		if ( proficienciesPage == 0 )
		{
			if ( mouseInBounds(interfaceSkillsSheet.x, interfaceSkillsSheet.x + interfaceSkillsSheet.w,
				interfaceSkillsSheet.y, interfaceSkillsSheet.y + interfaceSkillsSheet.h) )
			{
				return;
			}
		}
		else
		{
			if ( mouseInBounds(interfacePartySheet.x, interfacePartySheet.x + interfacePartySheet.w,
				interfacePartySheet.y, interfacePartySheet.y + interfacePartySheet.h) )
			{
				return;
			}
		}

		if ( mouseInsidePlayerInventory() || mouseInsidePlayerHotbar() )
		{
			return;
		}

		*inputPressed(impulses[IN_ATTACK]) = 0;
		*inputPressed(joyimpulses[INJOY_MENU_LEFT_CLICK]) = 0;

		if ( softwaremode )
		{
			entity = clickmap[omousey + omousex * yres];
		}
		else
		{
			uidnum = GO_GetPixelU32(omousex, yres - omousey);
			entity = uidToEntity(uidnum);
		}
	}

	if ( entity != NULL )
	{
		if ( multiplayer != CLIENT )
		{
			if ( (stat = entity->getStats()) == NULL )
			{
				Entity* parent = uidToEntity(entity->parent);
				if ( entity->behavior == &actPlayerLimb || entity->skill[2] == entity->parent )
				{
					if ( parent )
					{
						if ( parent->behavior == &actPlayer || parent->behavior == &actMonster )
						{
							Stat* stats = parent->getStats();
							if ( stats )
							{
								messagePlayerMonsterEvent(player, 0xFFFFFFFF, *stats, language[254], language[253], MSG_DESCRIPTION, parent);
							}
						}
					}
				}
				else if ( entity->behavior == &actTorch )
				{
					messagePlayer(player, language[255]);
				}
				else if ( entity->behavior == &actDoor )
				{
					messagePlayer(player, language[256]);
				}
				else if ( entity->behavior == &actItem )
				{
					item = newItem(static_cast<ItemType>(entity->skill[10]), static_cast<Status>(entity->skill[11]), entity->skill[12], entity->skill[13], entity->skill[14], false, NULL);
					if (item)
					{
						messagePlayer(player, language[257], item->description());
						free(item);
					}
				}
				else if ( entity->behavior == &actGoldBag )
				{
					if ( entity->goldAmount == 1 )
					{
						messagePlayer(player, language[258]);
					}
					else
					{
						messagePlayer(player, language[259], entity->goldAmount);
					}
				}
				else if ( entity->behavior == &actCampfire)
				{
					messagePlayer(player, language[260]);
				}
				else if ( entity->behavior == &actFountain)
				{
					messagePlayer(player, language[262]);
				}
				else if ( entity->behavior == &actSink)
				{
					messagePlayer(player, language[263]);
				}
				else if ( entity->behavior == &actLadder)
				{
					messagePlayer(player, language[264]);
				}
				else if ( entity->behavior == &actLadderUp)
				{
					messagePlayer(player, language[265]);
				}
				else if ( entity->behavior == &actChest || entity->behavior == &actChestLid )
				{
					messagePlayer(player, language[266]);
				}
				else if ( entity->behavior == &actGate)
				{
					messagePlayer(player, language[267]);
				}
				else if ( entity->behavior == &actSpearTrap)
				{
					messagePlayer(player, language[268]);
				}
				else if ( entity->behavior == &actSwitch)
				{
					messagePlayer(player, language[269]);
				}
				else if ( entity->behavior == &actBoulder )
				{
					messagePlayer(player, language[270]);
				}
				else if ( entity->behavior == &actHeadstone )
				{
					messagePlayer(player, language[271]);
				}
				else if ( entity->behavior == &actPortal || entity->behavior == &actWinningPortal
					|| entity->behavior == &actMidGamePortal )
				{
					messagePlayer(player, language[272]);
				}
				else if ( entity->behavior == &actFurniture )
				{
					switch ( entity->furnitureType )
					{
						case FURNITURE_CHAIR:
							messagePlayer(player, language[273]);
							break;
						case FURNITURE_TABLE:
							messagePlayer(player, language[274]);
							break;
						case FURNITURE_BED:
							messagePlayer(player, language[2497]);
							break;
						case FURNITURE_BUNKBED:
							messagePlayer(player, language[2499]);
							break;
						case FURNITURE_PODIUM:
							messagePlayer(player, language[2500]);
							break;
						default:
							messagePlayer(player, language[273]);
							break;
					}
				}
				// need to check the sprite since these are all empty behaviors.
				else if ( entity->sprite >= 631 && entity->sprite <= 633 ) // piston
				{
					messagePlayer(player, language[2501]);
				}
				else if  (entity->sprite == 629 || entity->sprite == 580 ) // column
				{
					messagePlayer(player, language[2502]);
				}
				else if ( entity->sprite == 581 || entity->sprite == 582 ) // floor stalag
				{
					messagePlayer(player, language[2503]);
				}
				else if ( entity->sprite == 583 || entity->sprite == 584 ) // ceiling stalag
				{
					messagePlayer(player, language[2504]);
				}
				else if ( entity->behavior == &actPowerCrystal || entity->behavior == &actPowerCrystalBase )
				{
					messagePlayer(player, language[2375]);
				}
				else if ( entity->behavior == &actPedestalBase )
				{
					messagePlayer(player, language[2376]);
				}
				else if ( entity->behavior == &actPedestalOrb )
				{
					messagePlayer(player, language[2377]);
				}
			}
			else
			{
				if ( entity->behavior == &actPlayerLimb )
				{
					Entity* parent = uidToEntity(entity->parent);
					if ( parent )
					{
						messagePlayerMonsterEvent(player, 0xFFFFFFFF, *stat, language[254], language[253], MSG_DESCRIPTION, parent);
					}
				}
				else
				{
					messagePlayerMonsterEvent(player, 0xFFFFFFFF, *stat, language[254], language[253], MSG_DESCRIPTION, entity);
				}
			}
		}
		else
		{
			// send spot command to server
			strcpy((char*)net_packet->data, "SPOT");
			net_packet->data[4] = player;
			SDLNet_Write32((Uint32)entity->getUID(), &net_packet->data[5]);
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 9;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
	}
}
