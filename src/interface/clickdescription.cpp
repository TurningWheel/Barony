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
#include "../ui/GameUI.hpp"

/*-------------------------------------------------------------------------------

	clickDescription

	reports the name of the entity clicked on in use mode

-------------------------------------------------------------------------------*/

void clickDescription(int player, Entity* entity)
{
	Stat* stat;
	Item* item;
	Uint32 uidnum;

	Input& input = Input::inputs[player];

	if ( entity == NULL )
	{
		if ( !framesProcResult.usable && *framesEatMouse )
		{
			return;
		}
		if ( !input.binaryToggle("InspectWithMouse") || players[player]->shootmode )
		{
			return;
		}

		Sint32 mx = inputs.getMouse(player, Inputs::OX);
		Sint32 my = inputs.getMouse(player, Inputs::OY);
		auto& inventoryUI = players[player]->inventoryUI;

		auto& camera = cameras[player];

		if ( mx < camera.winx || mx >= camera.winx + camera.winw || my < camera.winy || my >= camera.winy + camera.winh )
		{
			return;
		}

		if ( inputs.getUIInteraction(player)->selectedItem || inputs.getUIInteraction(player)->itemMenuOpen )
		{
			//Will bugger up GUI item interaction if this function continues to run.
			return;
		}

		/*if ( mouseInsidePlayerInventory(player) || mouseInsidePlayerHotbar(player) )
		{
			return;
		}*/

		input.consumeBinaryToggle("InspectWithMouse");

		uidnum = GO_GetPixelU32(mx, yres - my, cameras[player]);
		entity = uidToEntity(uidnum);
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
								if ( parent->isInertMimic() )
								{
									messagePlayer(player, MESSAGE_INSPECTION, Language::get(266));
								}
								else
								{
									messagePlayerMonsterEvent(player, 0xFFFFFFFF, *stats, Language::get(254), Language::get(253), MSG_DESCRIPTION, parent);
								}
							}
						}
					}
				}
				else if ( entity->behavior == &actTorch )
				{
					messagePlayer(player, MESSAGE_INSPECTION, Language::get(255));
				}
				else if ( entity->behavior == &actDoor )
				{
					messagePlayer(player, MESSAGE_INSPECTION, Language::get(256));
				}
				else if ( entity->isDamageableCollider() )
				{
					messagePlayer(player, MESSAGE_INSPECTION, Language::get(254), Language::get(entity->getColliderLangName()));
				}
				else if ( entity->behavior == &actBell || entity->sprite == 1475 || entity->sprite == 1476 || entity->sprite == 1477 || entity->sprite == 1478 )
				{
					messagePlayer(player, MESSAGE_INSPECTION, Language::get(254), Language::get(6269));
				}
				else if ( entity->behavior == &actItem )
				{
					item = newItem(static_cast<ItemType>(entity->skill[10]), static_cast<Status>(entity->skill[11]), entity->skill[12], entity->skill[13], entity->skill[14], false, NULL);
					if (item)
					{
						messagePlayer(player, MESSAGE_INSPECTION, Language::get(257), item->description());
						free(item);
					}
				}
				else if ( entity->behavior == &actGoldBag )
				{
					if ( entity->goldAmount == 1 )
					{
						messagePlayer(player, MESSAGE_INSPECTION, Language::get(258));
					}
					else
					{
						messagePlayer(player, MESSAGE_INSPECTION, Language::get(259), entity->goldAmount);
					}
				}
				else if ( entity->behavior == &actCampfire)
				{
					messagePlayer(player, MESSAGE_INSPECTION, Language::get(260));
				}
				else if ( entity->behavior == &actFountain)
				{
					messagePlayer(player, MESSAGE_INSPECTION, Language::get(262));
				}
				else if ( entity->behavior == &actSink)
				{
					messagePlayer(player, MESSAGE_INSPECTION, Language::get(263));
				}
				else if ( entity->behavior == &actLadder)
				{
					messagePlayer(player, MESSAGE_INSPECTION, Language::get(264));
				}
				else if ( entity->behavior == &actLadderUp)
				{
					messagePlayer(player, MESSAGE_INSPECTION, Language::get(265));
				}
				else if ( entity->behavior == &actChest || entity->behavior == &actChestLid )
				{
					messagePlayer(player, MESSAGE_INSPECTION, Language::get(266));
				}
				else if ( entity->behavior == &actGate)
				{
					messagePlayer(player, MESSAGE_INSPECTION, Language::get(267));
				}
				else if ( entity->behavior == &actSpearTrap)
				{
					messagePlayer(player, MESSAGE_INSPECTION, Language::get(268));
				}
				else if ( entity->behavior == &actSwitch)
				{
					messagePlayer(player, MESSAGE_INSPECTION, Language::get(269));
				}
				else if ( entity->behavior == &actBoulder )
				{
					messagePlayer(player, MESSAGE_INSPECTION, Language::get(270));
				}
				else if ( entity->behavior == &actHeadstone )
				{
					messagePlayer(player, MESSAGE_INSPECTION, Language::get(271));
				}
				else if ( entity->behavior == &actTeleportShrine /*||entity->behavior == &actSpellShrine*/ )
				{
					messagePlayer(player, MESSAGE_INSPECTION, Language::get(4307));
				}
				else if ( entity->behavior == &::actDaedalusShrine || entity->sprite == 1480 )
				{
					messagePlayer(player, MESSAGE_INSPECTION, Language::get(6260));
				}
				else if ( entity->behavior == &::actAssistShrine )
				{
					messagePlayer(player, MESSAGE_INSPECTION, Language::get(6352));
				}
				else if ( entity->behavior == &actStatue )
				{
					messagePlayer(player, MESSAGE_INSPECTION, Language::get(4308));
				}
				else if ( entity->behavior == &actPortal || entity->behavior == &actWinningPortal
					|| entity->behavior == &actMidGamePortal )
				{
					messagePlayer(player, MESSAGE_INSPECTION, Language::get(272));
				}
				else if ( entity->behavior == &actFurniture )
				{
					switch ( entity->furnitureType )
					{
						case FURNITURE_CHAIR:
							messagePlayer(player, MESSAGE_INSPECTION, Language::get(273));
							break;
						case FURNITURE_TABLE:
							messagePlayer(player, MESSAGE_INSPECTION, Language::get(274));
							break;
						case FURNITURE_BED:
							messagePlayer(player, MESSAGE_INSPECTION, Language::get(2497));
							break;
						case FURNITURE_BUNKBED:
							messagePlayer(player, MESSAGE_INSPECTION, Language::get(2499));
							break;
						case FURNITURE_PODIUM:
							messagePlayer(player, MESSAGE_INSPECTION, Language::get(2500));
							break;
						default:
							messagePlayer(player, MESSAGE_INSPECTION, Language::get(273));
							break;
					}
				}
				// need to check the sprite since these are all empty behaviors.
				else if ( entity->sprite >= 631 && entity->sprite <= 633 ) // piston
				{
					messagePlayer(player, MESSAGE_INSPECTION, Language::get(2501));
				}
				else if  (entity->sprite == 629 || entity->sprite == 580 ) // column
				{
					messagePlayer(player, MESSAGE_INSPECTION, Language::get(2502));
				}
				else if ( entity->sprite == 581 || entity->sprite == 582 ) // floor stalag
				{
					messagePlayer(player, MESSAGE_INSPECTION, Language::get(2503));
				}
				else if ( entity->sprite == 583 || entity->sprite == 584 ) // ceiling stalag
				{
					messagePlayer(player, MESSAGE_INSPECTION, Language::get(2504));
				}
				else if ( entity->behavior == &actPowerCrystal || entity->behavior == &actPowerCrystalBase )
				{
					messagePlayer(player, MESSAGE_INSPECTION, Language::get(2375));
				}
				else if ( entity->behavior == &actPedestalBase )
				{
					messagePlayer(player, MESSAGE_INSPECTION, Language::get(2376));
				}
				else if ( entity->behavior == &actPedestalOrb )
				{
					messagePlayer(player, MESSAGE_INSPECTION, Language::get(2377));
				}
				else if ( entity->behavior == &actTeleporter || entity->behavior == &actCustomPortal )
				{
					if ( entity->sprite == 161 )
					{
						messagePlayer(player, MESSAGE_INSPECTION, Language::get(264));
					}
					else if ( entity->sprite == 253 )
					{
						messagePlayer(player, MESSAGE_INSPECTION, Language::get(265));
					}
					else if ( (entity->sprite >= 254 && entity->sprite < 258) ||
						(entity->sprite >= 278 && entity->sprite < 282) ||
						(entity->sprite >= 614 && entity->sprite < 618) ||
						(entity->sprite >= 992 && entity->sprite < 995) ||
						(entity->sprite == 620))
					{
						messagePlayer(player, MESSAGE_INSPECTION, Language::get(272));
					}
				}
				else if ( entity->behavior == &actFloorDecoration )
				{
					if ( entity->sprite == 991 )
					{
						messagePlayer(player, MESSAGE_INSPECTION, Language::get(4073));
					}
				}
			}
			else
			{
				if ( entity->behavior == &actPlayerLimb )
				{
					Entity* parent = uidToEntity(entity->parent);
					if ( parent )
					{
						messagePlayerMonsterEvent(player, 0xFFFFFFFF, *stat, Language::get(254), Language::get(253), MSG_DESCRIPTION, parent);
					}
				}
				else
				{
					messagePlayerMonsterEvent(player, 0xFFFFFFFF, *stat, Language::get(254), Language::get(253), MSG_DESCRIPTION, entity);
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
