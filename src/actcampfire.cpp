/*-------------------------------------------------------------------------------

	BARONY
	File: actcampfire.cpp
	Desc: behavior function for campfires

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "items.hpp"
#include "engine/audio/sound.hpp"
#include "net.hpp"
#include "player.hpp"
#include "prng.hpp"
#include "scores.hpp"
#include "collision.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

#define CAMPFIRE_LIGHTING my->skill[0]
#define CAMPFIRE_FLICKER my->skill[1]
#define CAMPFIRE_HEALTH my->skill[3]
#define CAMPFIRE_INIT my->skill[4]
#define CAMPFIRE_SOUNDTIME my->skill[5]

void actCampfire(Entity* my)
{
	int i;

	// init
	if ( !CAMPFIRE_INIT )
	{
		CAMPFIRE_INIT = 1;
		CAMPFIRE_HEALTH = MAXPLAYERS;
		my->createWorldUITooltip();
	}

	// crackling sounds
	if ( CAMPFIRE_HEALTH > 0 )
	{
#ifdef USE_FMOD
		if ( CAMPFIRE_SOUNDTIME == 0 )
		{
			CAMPFIRE_SOUNDTIME--;
			my->stopEntitySound();
			my->entity_sound = playSoundEntityLocal(my, 133, 32);
		}
		if ( my->entity_sound )
		{
			bool playing = false;
			my->entity_sound->isPlaying(&playing);
			if ( !playing )
			{
				my->entity_sound = nullptr;
			}
		}
#else
		CAMPFIRE_SOUNDTIME--;
		if ( CAMPFIRE_SOUNDTIME <= 0 )
		{
			CAMPFIRE_SOUNDTIME = 480;
			playSoundEntityLocal( my, 133, 128 );
		}
#endif

		// spew flame particles
		if ( flickerLights )
		{
		    for ( i = 0; i < 3; i++ )
		    {
				if ( Entity* entity = spawnFlame(my, SPRITE_FLAME) )
				{
					entity->x += ((local_rng.rand() % 30) - 10) / 10.f;
					entity->y += ((local_rng.rand() % 30) - 10) / 10.f;
					entity->z -= 1;
				}
		    }
			if ( Entity* entity = spawnFlame(my, SPRITE_FLAME) )
			{
				entity->z -= 2;
			}
		}
		else
		{
		    if ( ticks % TICKS_PER_SECOND == 0 )
		    {
				if ( Entity* entity = spawnFlame(my, SPRITE_FLAME) )
				{
					entity->z -= 2;
				}
		    }
		}

		// light environment
		if ( !CAMPFIRE_LIGHTING )
		{
			my->light = addLight(my->x / 16, my->y / 16, "campfire");
			CAMPFIRE_LIGHTING = 1;
		}
		if ( flickerLights )
		{
			//Campfires will never flicker if this setting is disabled.
			CAMPFIRE_FLICKER--;
		}
		if (CAMPFIRE_FLICKER <= 0)
		{
			CAMPFIRE_LIGHTING = (CAMPFIRE_LIGHTING == 1) + 1;

			if (CAMPFIRE_LIGHTING == 1)
			{
				my->removeLightField();
				my->light = addLight(my->x / 16, my->y / 16, "campfire");
			}
			else
			{
				my->removeLightField();
				my->light = addLight(my->x / 16, my->y / 16, "campfire_flicker");
			}
			CAMPFIRE_FLICKER = 2 + local_rng.rand() % 7;
		}
	}
	else
	{
		my->removeLightField();
		my->light = NULL;

		my->stopEntitySound();
	}

	if ( multiplayer != CLIENT )
	{
		// using campfire
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if ( selectedEntity[i] == my || client_selected[i] == my )
			{
				if (inrange[i])
				{
					if ( CAMPFIRE_HEALTH > 0 )
					{
						messagePlayer(i, MESSAGE_INTERACTION, Language::get(457));
						CAMPFIRE_HEALTH--;
						if ( CAMPFIRE_HEALTH <= 0 )
						{
							serverUpdateEntitySkill(my, 3); // extinguish for all clients
							messagePlayer(i, MESSAGE_INTERACTION, Language::get(458));
							my->removeLightField();
							my->light = NULL;
						}
						Item* item = newItem(TOOL_TORCH, WORN, 0, 1, 0, true, NULL);
						itemPickup(i, item);
						free(item);
					}
					else
					{
						messagePlayer(i, MESSAGE_INTERACTION, Language::get(458));
					}
				}
			}
		}
	}
}

void actCauldron(Entity* my)
{
	if ( !CAMPFIRE_INIT )
	{
		CAMPFIRE_INIT = 1;
		CAMPFIRE_HEALTH = MAXPLAYERS;
		my->createWorldUITooltip();
	}

	// crackling sounds
	if ( CAMPFIRE_HEALTH > 0 )
	{
#ifdef USE_FMOD
		if ( CAMPFIRE_SOUNDTIME == 0 )
		{
			CAMPFIRE_SOUNDTIME--;
			my->stopEntitySound();
			my->entity_sound = playSoundEntityLocal(my, 133, 32);
		}
		if ( my->entity_sound )
		{
			bool playing = false;
			my->entity_sound->isPlaying(&playing);
			if ( !playing )
			{
				my->entity_sound = nullptr;
			}
		}
#else
		CAMPFIRE_SOUNDTIME--;
		if ( CAMPFIRE_SOUNDTIME <= 0 )
		{
			CAMPFIRE_SOUNDTIME = 480;
			playSoundEntityLocal(my, 133, 128);
		}
#endif

		// spew flame particles
		if ( flickerLights )
		{
			for ( int i = 0; i < 3; i++ )
			{
				if ( Entity* entity = spawnFlame(my, SPRITE_FLAME) )
				{
					entity->x += ((local_rng.rand() % 30) - 10) / 10.f;
					entity->y += ((local_rng.rand() % 30) - 10) / 10.f;
					entity->z -= 1;
				}
			}
			if ( Entity* entity = spawnFlame(my, SPRITE_FLAME) )
			{
				entity->z -= 2;
			}
		}
		else
		{
			if ( ticks % TICKS_PER_SECOND == 0 )
			{
				if ( Entity* entity = spawnFlame(my, SPRITE_FLAME) )
				{
					entity->z -= 2;
				}
			}
		}

		// light environment
		if ( !CAMPFIRE_LIGHTING )
		{
			my->light = addLight(my->x / 16, my->y / 16, "campfire");
			CAMPFIRE_LIGHTING = 1;
		}
		if ( flickerLights )
		{
			//Campfires will never flicker if this setting is disabled.
			CAMPFIRE_FLICKER--;
		}
		if ( CAMPFIRE_FLICKER <= 0 )
		{
			CAMPFIRE_LIGHTING = (CAMPFIRE_LIGHTING == 1) + 1;

			if ( CAMPFIRE_LIGHTING == 1 )
			{
				my->removeLightField();
				my->light = addLight(my->x / 16, my->y / 16, "campfire");
			}
			else
			{
				my->removeLightField();
				my->light = addLight(my->x / 16, my->y / 16, "campfire_flicker");
			}
			CAMPFIRE_FLICKER = 2 + local_rng.rand() % 7;
		}
	}
	else
	{
		my->removeLightField();
		my->light = NULL;

		my->stopEntitySound();
	}

	if ( multiplayer == CLIENT )
	{
		return;
	}

	auto& cauldronInteracting = my->skill[6];
	Entity* interacting = uidToEntity(cauldronInteracting);

	if ( cauldronInteracting > 0 )
	{
		if ( !interacting || (entityDist(interacting, my) > TOUCHRANGE) )
		{
			int playernum = -1;
			if ( !interacting )
			{
				for ( int i = 0; i < MAXPLAYERS; ++i )
				{
					if ( achievementObserver.playerUids[i] == cauldronInteracting )
					{
						playernum = i;
						break;
					}
				}
			}
			else if ( interacting->behavior == &actPlayer )
			{
				playernum = interacting->skill[2];
			}
			cauldronInteracting = 0;
			serverUpdateEntitySkill(my, 6);
			if ( multiplayer == SERVER && playernum > 0 )
			{
				strcpy((char*)net_packet->data, "CAUC");
				net_packet->data[4] = playernum;
				SDLNet_Write32(my->getUID(), &net_packet->data[5]);
				net_packet->address.host = net_clients[playernum - 1].host;
				net_packet->address.port = net_clients[playernum - 1].port;
				net_packet->len = 9;
				sendPacketSafe(net_sock, -1, net_packet, playernum - 1);
			}
			else if ( multiplayer == SINGLE || playernum == 0 )
			{
				if ( playernum >= 0 && playernum < MAXPLAYERS )
				{
					GenericGUI[playernum].alchemyGUI.closeAlchemyMenu();
				}
			}
		}
	}

	// using
	for ( int i = 0; i < MAXPLAYERS; i++ )
	{
		if ( selectedEntity[i] == my || client_selected[i] == my )
		{
			if ( inrange[i] && players[i]->entity )
			{
				if ( cauldronInteracting != 0 )
				{
					if ( Entity* interacting = uidToEntity(cauldronInteracting) )
					{
						if ( interacting != players[i]->entity )
						{
							messagePlayer(i, MESSAGE_INTERACTION, Language::get(6975));
						}
					}
				}
				else
				{
					cauldronInteracting = players[i]->entity->getUID();
					if ( multiplayer == SERVER )
					{
						serverUpdateEntitySkill(my, 6);
					}
					if ( players[i]->isLocalPlayer() )
					{
						GenericGUI[i].openGUI(GUI_TYPE_ALCHEMY, my);
					}
					else if ( multiplayer == SERVER && i > 0 )
					{
						strcpy((char*)net_packet->data, "CAUO");
						SDLNet_Write32(my->getUID(), &net_packet->data[4]);
						net_packet->address.host = net_clients[i - 1].host;
						net_packet->address.port = net_clients[i - 1].port;
						net_packet->len = 8;
						sendPacketSafe(net_sock, -1, net_packet, i - 1);
					}
				}
				break;
			}
		}
	}
}

void actWorkbench(Entity* my)
{
	if ( !CAMPFIRE_INIT )
	{
		CAMPFIRE_INIT = 1;
		CAMPFIRE_HEALTH = MAXPLAYERS;
		my->createWorldUITooltip();
	}

//	// crackling sounds
//	if ( CAMPFIRE_HEALTH > 0 )
//	{
//#ifdef USE_FMOD
//		if ( CAMPFIRE_SOUNDTIME == 0 )
//		{
//			CAMPFIRE_SOUNDTIME--;
//			my->stopEntitySound();
//			my->entity_sound = playSoundEntityLocal(my, 133, 32);
//		}
//		if ( my->entity_sound )
//		{
//			bool playing = false;
//			my->entity_sound->isPlaying(&playing);
//			if ( !playing )
//			{
//				my->entity_sound = nullptr;
//			}
//		}
//#else
//		CAMPFIRE_SOUNDTIME--;
//		if ( CAMPFIRE_SOUNDTIME <= 0 )
//		{
//			CAMPFIRE_SOUNDTIME = 480;
//			playSoundEntityLocal(my, 133, 128);
//		}
//#endif
//
//		// spew flame particles
//		if ( flickerLights )
//		{
//			for ( int i = 0; i < 3; i++ )
//			{
//				if ( Entity* entity = spawnFlame(my, SPRITE_FLAME) )
//				{
//					entity->x += ((local_rng.rand() % 30) - 10) / 10.f;
//					entity->y += ((local_rng.rand() % 30) - 10) / 10.f;
//					entity->z -= 1;
//				}
//			}
//			if ( Entity* entity = spawnFlame(my, SPRITE_FLAME) )
//			{
//				entity->z -= 2;
//			}
//		}
//		else
//		{
//			if ( ticks % TICKS_PER_SECOND == 0 )
//			{
//				if ( Entity* entity = spawnFlame(my, SPRITE_FLAME) )
//				{
//					entity->z -= 2;
//				}
//			}
//		}
//
//		// light environment
//		if ( !CAMPFIRE_LIGHTING )
//		{
//			my->light = addLight(my->x / 16, my->y / 16, "campfire");
//			CAMPFIRE_LIGHTING = 1;
//		}
//		if ( flickerLights )
//		{
//			//Campfires will never flicker if this setting is disabled.
//			CAMPFIRE_FLICKER--;
//		}
//		if ( CAMPFIRE_FLICKER <= 0 )
//		{
//			CAMPFIRE_LIGHTING = (CAMPFIRE_LIGHTING == 1) + 1;
//
//			if ( CAMPFIRE_LIGHTING == 1 )
//			{
//				my->removeLightField();
//				my->light = addLight(my->x / 16, my->y / 16, "campfire");
//			}
//			else
//			{
//				my->removeLightField();
//				my->light = addLight(my->x / 16, my->y / 16, "campfire_flicker");
//			}
//			CAMPFIRE_FLICKER = 2 + local_rng.rand() % 7;
//		}
//	}
//	else
//	{
//		my->removeLightField();
//		my->light = NULL;
//
//		my->stopEntitySound();
//	}

	if ( multiplayer == CLIENT )
	{
		return;
	}

	auto& workbenchInteracting = my->skill[6];
	Entity* interacting = uidToEntity(workbenchInteracting);

	if ( workbenchInteracting > 0 )
	{
		if ( !interacting || (entityDist(interacting, my) > TOUCHRANGE) )
		{
			int playernum = -1;
			if ( !interacting )
			{
				for ( int i = 0; i < MAXPLAYERS; ++i )
				{
					if ( achievementObserver.playerUids[i] == workbenchInteracting )
					{
						playernum = i;
						break;
					}
				}
			}
			else if ( interacting->behavior == &actPlayer )
			{
				playernum = interacting->skill[2];
			}
			workbenchInteracting = 0;
			serverUpdateEntitySkill(my, 6);
			if ( multiplayer == SERVER && playernum > 0 )
			{
				strcpy((char*)net_packet->data, "WRKC");
				net_packet->data[4] = playernum;
				SDLNet_Write32(my->getUID(), &net_packet->data[5]);
				net_packet->address.host = net_clients[playernum - 1].host;
				net_packet->address.port = net_clients[playernum - 1].port;
				net_packet->len = 9;
				sendPacketSafe(net_sock, -1, net_packet, playernum - 1);
			}
			else if ( multiplayer == SINGLE || playernum == 0 )
			{
				if ( playernum >= 0 && playernum < MAXPLAYERS )
				{
					GenericGUI[playernum].tinkerGUI.closeTinkerMenu();
				}
			}
		}
	}

	// using
	for ( int i = 0; i < MAXPLAYERS; i++ )
	{
		if ( selectedEntity[i] == my || client_selected[i] == my )
		{
			if ( inrange[i] && players[i]->entity )
			{
				if ( workbenchInteracting != 0 )
				{
					if ( Entity* interacting = uidToEntity(workbenchInteracting) )
					{
						if ( interacting != players[i]->entity )
						{
							messagePlayer(i, MESSAGE_INTERACTION, Language::get(6982));
						}
					}
				}
				else
				{
					workbenchInteracting = players[i]->entity->getUID();
					if ( multiplayer == SERVER )
					{
						serverUpdateEntitySkill(my, 6);
					}
					if ( players[i]->isLocalPlayer() )
					{
						GenericGUI[i].openGUI(GUI_TYPE_TINKERING, my);
					}
					else if ( multiplayer == SERVER && i > 0 )
					{
						strcpy((char*)net_packet->data, "WRKO");
						SDLNet_Write32(my->getUID(), &net_packet->data[4]);
						net_packet->address.host = net_clients[i - 1].host;
						net_packet->address.port = net_clients[i - 1].port;
						net_packet->len = 8;
						sendPacketSafe(net_sock, -1, net_packet, i - 1);
					}
				}
				break;
			}
		}
	}
}

void actMailbox(Entity* my)
{
	if ( !CAMPFIRE_INIT )
	{
		CAMPFIRE_INIT = 1;
		CAMPFIRE_HEALTH = MAXPLAYERS;
		my->createWorldUITooltip();
	}

	if ( multiplayer == CLIENT )
	{
		return;
	}

	auto& mailboxInteracting = my->skill[6];
	Entity* interacting = uidToEntity(mailboxInteracting);

	if ( mailboxInteracting > 0 )
	{
		if ( !interacting || (entityDist(interacting, my) > TOUCHRANGE) )
		{
			int playernum = -1;
			if ( !interacting )
			{
				for ( int i = 0; i < MAXPLAYERS; ++i )
				{
					if ( achievementObserver.playerUids[i] == mailboxInteracting )
					{
						playernum = i;
						break;
					}
				}
			}
			else if ( interacting->behavior == &actPlayer )
			{
				playernum = interacting->skill[2];
			}
			mailboxInteracting = 0;
			serverUpdateEntitySkill(my, 6);
			if ( multiplayer == SERVER && playernum > 0 )
			{
				strcpy((char*)net_packet->data, "MBXC");
				net_packet->data[4] = playernum;
				SDLNet_Write32(my->getUID(), &net_packet->data[5]);
				net_packet->address.host = net_clients[playernum - 1].host;
				net_packet->address.port = net_clients[playernum - 1].port;
				net_packet->len = 9;
				sendPacketSafe(net_sock, -1, net_packet, playernum - 1);
			}
			else if ( multiplayer == SINGLE || playernum == 0 )
			{
				if ( playernum >= 0 && playernum < MAXPLAYERS )
				{
					GenericGUI[playernum].mailboxGUI.closeMailMenu();
				}
			}
		}
	}

	// using
	for ( int i = 0; i < MAXPLAYERS; i++ )
	{
		if ( selectedEntity[i] == my || client_selected[i] == my )
		{
			if ( inrange[i] && players[i]->entity )
			{
				if ( mailboxInteracting != 0 )
				{
					if ( Entity* interacting = uidToEntity(mailboxInteracting) )
					{
						if ( interacting != players[i]->entity )
						{
							messagePlayer(i, MESSAGE_INTERACTION, Language::get(6987));
						}
					}
				}
				else
				{
					mailboxInteracting = players[i]->entity->getUID();
					if ( multiplayer == SERVER )
					{
						serverUpdateEntitySkill(my, 6);
					}
					if ( players[i]->isLocalPlayer() )
					{
						GenericGUI[i].openGUI(GUI_TYPE_MAILBOX, my);
					}
					else if ( multiplayer == SERVER && i > 0 )
					{
						strcpy((char*)net_packet->data, "MBXO");
						SDLNet_Write32(my->getUID(), &net_packet->data[4]);
						net_packet->address.host = net_clients[i - 1].host;
						net_packet->address.port = net_clients[i - 1].port;
						net_packet->len = 8;
						sendPacketSafe(net_sock, -1, net_packet, i - 1);
					}
				}
				break;
			}
		}
	}
}