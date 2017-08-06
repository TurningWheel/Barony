/*-------------------------------------------------------------------------------

	BARONY
	File: game.cpp
	Desc: contains main game code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "messages.hpp"
#include "entity.hpp"
#include "menu.hpp"
#include "classdescriptions.hpp"
#include "interface/interface.hpp"
#include "magic/magic.hpp"
#include "sound.hpp"
#include "items.hpp"
#include "shops.hpp"
#include "monster.hpp"
#include "scores.hpp"
#include "menu.hpp"
#include "net.hpp"
#ifdef STEAMWORKS
#include <steam/steam_api.h>
#include "steam.hpp"
#endif
#include "prng.hpp"
#include "collision.hpp"
#include "paths.hpp"
#include "player.hpp"
#include <limits>

#ifdef LINUX
//Sigsegv catching stuff.
#include <signal.h>
#include <string.h>
#include <execinfo.h>

const unsigned STACK_SIZE = 10;



void segfault_sigaction(int signal, siginfo_t* si, void* arg)
{
	printf("Caught segfault at address %p\n", si->si_addr);

	printlog("Caught segfault at address %p\n", si->si_addr);

	//Dump the stack.
	void* array[STACK_SIZE];
	size_t size;

	size = backtrace(array, STACK_SIZE);

	printlog("Signal %d (dumping stack):\n", signal);
	backtrace_symbols_fd(array, size, STDERR_FILENO);

	SDL_SetRelativeMouseMode(SDL_FALSE); //Uncapture mouse.

	exit(0);
}

#endif

// recommended for valgrind debugging:
// res of 480x270
// /nohud
// undefine SOUND, MUSIC (see sound.h)

int game = 1;
Uint32 uniqueGameKey = 0;
list_t steamAchievements;
std::vector<std::string> randomPlayerNamesMale;
std::vector<std::string> randomPlayerNamesFemale;

/*-------------------------------------------------------------------------------

	gameLogic

	Updates the gamestate; moves actors, primarily

-------------------------------------------------------------------------------*/

void gameLogic(void)
{
	Uint32 x;
	node_t* node, *nextnode, *node2;
	Entity* entity;
	int c = 0;
	Uint32 i = 0, j;
	FILE* fp;
	deleteent_t* deleteent;
	bool entitydeletedself;
	int auto_appraise_lowest_time = std::numeric_limits<int>::max();
	Item* auto_appraise_target = NULL;

	if ( creditstage > 0 )
	{
		credittime++;
	}
	if ( intromoviestage > 0 )
	{
		intromovietime++;
	}
	if ( firstendmoviestage > 0 )
	{
		firstendmovietime++;
	}
	if ( secondendmoviestage > 0 )
	{
		secondendmovietime++;
	}

#ifdef SOUND
	sound_update(); //Update FMOD and whatnot.
#endif

	// camera shaking
	if ( shaking )
	{
		camera_shakex2 = (camera_shakex2 + camera_shakex) * .8;
		camera_shakey2 = (camera_shakey2 + camera_shakey) * .9;
		if ( camera_shakex2 > 0 )
		{
			if ( camera_shakex2 < .02 && camera_shakex >= -.01 )
			{
				camera_shakex2 = 0;
				camera_shakex = 0;
			}
			else
			{
				camera_shakex -= .01;
			}
		}
		else if ( camera_shakex2 < 0 )
		{
			if ( camera_shakex2 > -.02 && camera_shakex <= .01 )
			{
				camera_shakex2 = 0;
				camera_shakex = 0;
			}
			else
			{
				camera_shakex += .01;
			}
		}
		if ( camera_shakey2 > 0 )
		{
			camera_shakey -= 1;
		}
		else if ( camera_shakey2 < 0 )
		{
			camera_shakey += 1;
		}
	}

	// drunkenness
	if ( stats[clientnum]->EFFECTS[EFF_DRUNK] && !intro )
	{
		if ( drunkextend < 0.5 )
		{
			drunkextend += .005;
			if ( drunkextend > 0.5 )
			{
				drunkextend = 0.5;
			}
		}
	}
	else
	{
		if ( drunkextend > 0 )
		{
			drunkextend -= .005;
			if ( drunkextend < 0 )
			{
				drunkextend = 0;
			}
		}
	}

	// fading in/out
	if ( fadeout == true )
	{
		fadealpha = std::min(fadealpha + 5, 255);
		if ( fadealpha == 255 )
		{
			fadefinished = true;
		}
		if ( localPlayerNetworkType == SERVER && introstage == 3 )
		{
			// machinegun this message to clients to make sure they get it!
			for ( c = 1; c < MAXPLAYERS; c++ )
			{
				if ( client_disconnected[c] )
				{
					continue;
				}
				strcpy((char*)net_packet->data, "BARONY_GAME_START");
				SDLNet_Write32(svFlags, &net_packet->data[17]);
				SDLNet_Write32(uniqueGameKey, &net_packet->data[21]);
				net_packet->address.host = net_clients[c - 1].host;
				net_packet->address.port = net_clients[c - 1].port;
				net_packet->len = 25;
				sendPacket(net_sock, -1, net_packet, c - 1);
			}
		}
	}
	else
	{
		fadealpha = std::max(0, fadealpha - 5);
	}

	// handle safe packets
	if ( !(ticks % 4) )
	{
		j = 0;
		for ( node = safePacketsSent.first; node != NULL; node = nextnode )
		{
			nextnode = node->next;

			packetsend_t* packet = (packetsend_t*)node->element;
			sendPacket(packet->sock, packet->channel, packet->packet, packet->hostnum);
			packet->tries++;
			if ( packet->tries >= MAXTRIES )
			{
				list_RemoveNode(node);
			}
			j++;
			if ( j >= MAXDELETES )
			{
				break;
			}
		}
	}

	// spawn flame particles on burning objects
	if ( !gamePaused || (localPlayerNetworkType && !client_disconnected[0]) )
	{
		for ( node = map.entities->first; node != NULL; node = node->next )
		{
			entity = (Entity*)node->element;
			if ( entity->flags[BURNING] )
			{
				if ( !entity->flags[BURNABLE] )
				{
					entity->flags[BURNING] = false;
					continue;
				}
				j = 1 + rand() % 4;
				for ( c = 0; c < j; c++ )
				{
					Entity* flame = spawnFlame(entity);
					flame->x += rand() % (entity->sizex * 2 + 1) - entity->sizex;
					flame->y += rand() % (entity->sizey * 2 + 1) - entity->sizey;
					flame->z += rand() % 5 - 2;
				}
			}
		}
	}

	// damage indicator timers
	handleDamageIndicatorTicks();

	if ( intro == true )
	{
		// rotate gear
		gearrot += 1;
		if ( gearrot >= 360 )
		{
			gearrot -= 360;
		}
		gearsize -= std::max<double>(2, gearsize / 35.0);
		if ( gearsize < 70 )
		{
			gearsize = 70;
			logoalpha += 2;
		}

		// animate tiles
		if ( ticks % 10 == 0 && !gamePaused )
		{
			int x, y, z;
			for ( x = 0; x < map.width; x++ )
			{
				for ( y = 0; y < map.height; y++ )
				{
					for ( z = 0; z < MAPLAYERS; z++ )
					{
						if ( animatedtiles[map.tiles[z + y * MAPLAYERS + x * MAPLAYERS * map.height]] )
						{
							map.tiles[z + y * MAPLAYERS + x * MAPLAYERS * map.height]--;
							if ( !animatedtiles[map.tiles[z + y * MAPLAYERS + x * MAPLAYERS * map.height]] )
							{
								int tile = map.tiles[z + y * MAPLAYERS + x * MAPLAYERS * map.height];
								do
								{
									tile++;
								}
								while ( animatedtiles[tile] );
								map.tiles[z + y * MAPLAYERS + x * MAPLAYERS * map.height] = tile - 1;
							}
						}
					}
				}
			}
		}

		// execute entity behaviors
		c = localPlayerNetworkType;
		x = clientnum;
		localPlayerNetworkType = SINGLE;
		clientnum = 0;
		for ( node = map.entities->first; node != NULL; node = nextnode )
		{
			nextnode = node->next;
			entity = (Entity*)node->element;
			if ( !entity->ranbehavior )
			{
				entity->ticks++;
				if ( entity->behavior != NULL )
				{
					(*entity->behavior)(entity);
					if ( entitiesdeleted.first != NULL )
					{
						entitydeletedself = false;
						for ( node2 = entitiesdeleted.first; node2 != NULL; node2 = node2->next )
						{
							if ( entity == (Entity*)node2->element )
							{
								entitydeletedself = true;
								break;
							}
						}
						if ( entitydeletedself == false )
						{
							entity->ranbehavior = true;
						}
						nextnode = map.entities->first;
						list_FreeAll(&entitiesdeleted);
					}
					else
					{
						entity->ranbehavior = true;
						nextnode = node->next;
					}
				}
			}
		}
		for ( node = map.entities->first; node != NULL; node = node->next )
		{
			entity = (Entity*)node->element;
			entity->ranbehavior = false;
		}
		localPlayerNetworkType = c;
		clientnum = x;
	}
	else
	{
		if ( localPlayerNetworkType == SERVER )
		{
			if ( ticks % 4 == 0 )
			{
				// continue informing clients of entities they need to delete
				for ( i = 1; i < MAXPLAYERS; i++ )
				{
					j = 0;
					for ( node = entitiesToDelete[i].first; node != NULL; node = nextnode )
					{
						nextnode = node->next;

						// send the delete entity command to the client
						strcpy((char*)net_packet->data, "ENTD");
						deleteent = (deleteent_t*)node->element;
						SDLNet_Write32(deleteent->uid, &net_packet->data[4]);
						net_packet->address.host = net_clients[i - 1].host;
						net_packet->address.port = net_clients[i - 1].port;
						net_packet->len = 8;
						sendPacket(net_sock, -1, net_packet, i - 1);

						// quit reminding clients after a certain number of attempts
						deleteent->tries++;
						if ( deleteent->tries >= MAXTRIES )
						{
							list_RemoveNode(node);
						}
						j++;
						if ( j >= MAXDELETES )
						{
							break;
						}
					}
				}
			}
		}
		if ( localPlayerNetworkType != CLIENT )   // server/singleplayer code
		{
			for ( c = 0; c < MAXPLAYERS; c++ )
			{
				assailant[c] = false;
			}

			// animate tiles
			if ( !gamePaused )
			{
				int x, y, z;
				for ( x = 0; x < map.width; x++ )
				{
					for ( y = 0; y < map.height; y++ )
					{
						for ( z = 0; z < MAPLAYERS; z++ )
						{
							int index = z + y * MAPLAYERS + x * MAPLAYERS * map.height;
							if ( animatedtiles[map.tiles[index]] )
							{
								if ( ticks % 10 == 0 )
								{
									map.tiles[index]--;
									if ( !animatedtiles[map.tiles[index]] )
									{
										do
										{
											map.tiles[index]++;
										}
										while ( animatedtiles[map.tiles[index]] );
										map.tiles[index]--;
									}
								}
								if ( z == 0 )
								{
									// water and lava noises
									if ( ticks % TICKS_PER_SECOND == (y + x * map.height) % TICKS_PER_SECOND && rand() % 3 == 0 )
									{
										if ( lavatiles[map.tiles[index]] )
										{
											// bubbling lava
											playSoundPosLocal( x * 16 + 8, y * 16 + 8, 155, 100 );
										}
										else
										{
											// running water
											playSoundPosLocal( x * 16 + 8, y * 16 + 8, 135, 32 );
										}
									}

									// lava bubbles
									if ( lavatiles[map.tiles[index]] )
									{
										if ( ticks % 40 == (y + x * map.height) % 40 && rand() % 3 == 0 )
										{
											int c, j = 1 + rand() % 2;
											for ( c = 0; c < j; c++ )
											{
												Entity* entity = newEntity(42, 1, map.entities);
												entity->behavior = &actGib;
												entity->x = x * 16 + rand() % 16;
												entity->y = y * 16 + rand() % 16;
												entity->z = 7.5;
												entity->flags[PASSABLE] = true;
												entity->flags[SPRITE] = true;
												entity->flags[NOUPDATE] = true;
												entity->flags[UPDATENEEDED] = false;
												entity->flags[UNCLICKABLE] = true;
												entity->sizex = 2;
												entity->sizey = 2;
												entity->fskill[3] = 0.01;
												double vel = (rand() % 10) / 20.f;
												entity->vel_x = vel * cos(entity->yaw);
												entity->vel_y = vel * sin(entity->yaw);
												entity->vel_z = -.15 - (rand() % 15) / 100.f;
												entity->yaw = (rand() % 360) * PI / 180.0;
												entity->pitch = (rand() % 360) * PI / 180.0;
												entity->roll = (rand() % 360) * PI / 180.0;
												if ( localPlayerNetworkType != CLIENT )
												{
													entity_uids--;
												}
												entity->setUID(-3);
											}
										}
									}
								}
							}
						}
					}
				}
			}

			// periodic steam achievement check
			if ( ticks % TICKS_PER_SECOND == 0 )
			{
				for ( c = 0; c < MAXPLAYERS; c++ )
				{
					if ( client_disconnected[c] )
					{
						continue;
					}

					if ( list_Size(&stats[c]->FOLLOWERS) >= 3 )
					{
						steamAchievementClient(c, "BARONY_ACH_NATURAL_BORN_LEADER");
					}
					if ( stats[c]->GOLD >= 10000 )
					{
						steamAchievementClient(i, "BARONY_ACH_FILTHY_RICH");
					}
				}
			}
			if ( conductPenniless )
			{
				if ( stats[clientnum]->GOLD > 0 )
				{
					conductPenniless = false;
				}
			}

			//if( TICKS_PER_SECOND )
			//generatePathMaps();
			for ( node = map.entities->first; node != NULL; node = nextnode )
			{
				nextnode = node->next;
				entity = (Entity*)node->element;
				if ( !entity->ranbehavior )
				{
					if ( !gamePaused || (localPlayerNetworkType && !client_disconnected[0]) )
					{
						entity->ticks++;
					}
					if ( entity->behavior != NULL )
					{
						if ( !gamePaused || (localPlayerNetworkType && !client_disconnected[0]) )
						{
							(*entity->behavior)(entity);
						}
						if ( entitiesdeleted.first != NULL )
						{
							entitydeletedself = false;
							for ( node2 = entitiesdeleted.first; node2 != NULL; node2 = node2->next )
							{
								if ( entity == (Entity*)node2->element )
								{
									entitydeletedself = true;
									break;
								}
							}
							if ( entitydeletedself == false )
							{
								entity->ranbehavior = true;
							}
							nextnode = map.entities->first;
							list_FreeAll(&entitiesdeleted);
						}
						else
						{
							entity->ranbehavior = true;
							nextnode = node->next;
						}
					}
				}
				if ( loadnextlevel == true )
				{
					for ( node = map.entities->first; node != NULL; node = node->next )
					{
						entity = (Entity*)node->element;
						entity->flags[NOUPDATE] = true;
					}

					// hack to fix these things from breaking everything...
					hudarm = NULL;
					hudweapon = NULL;
					magicLeftHand = NULL;
					magicRightHand = NULL;

					// stop all sounds
#ifdef HAVE_FMOD
					if ( sound_group )
					{
						FMOD_ChannelGroup_Stop(sound_group);
					}
#elif defined HAVE_OPENAL
					if ( sound_group )
					{
						OPENAL_ChannelGroup_Stop(sound_group);
					}
#endif

					// show loading message
					loading = true;
					drawClearBuffers();
					int w, h;
					TTF_SizeUTF8(ttf16, language[709], &w, &h);
					ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, language[709]);

					GO_SwapBuffers(screen);

					// copy followers list
					list_t tempFollowers[MAXPLAYERS];
					for ( c = 0; c < MAXPLAYERS; c++ )
					{
						tempFollowers[c].first = NULL;
						tempFollowers[c].last = NULL;

						node_t* node;
						for ( node = stats[c]->FOLLOWERS.first; node != NULL; node = node->next )
						{
							Entity* follower = uidToEntity(*((Uint32*)node->element));
							if ( follower )
							{
								Stat* followerStats = follower->getStats();
								if ( followerStats )
								{
									node_t* newNode = list_AddNodeLast(&tempFollowers[c]);
									newNode->element = followerStats->copyStats();
//									newNode->deconstructor = &followerStats->~Stat;
									newNode->size = sizeof(followerStats);
								}
							}
						}

						list_FreeAll(&stats[c]->FOLLOWERS);
					}

					// unlock some steam achievements
					if ( !secretlevel )
					{
						switch ( currentlevel )
						{
							case 0:
								steamAchievement("BARONY_ACH_ENTER_THE_DUNGEON");
								break;
							case 4:
								steamAchievement("BARONY_ACH_TWISTY_PASSAGES");
								break;
							case 9:
								steamAchievement("BARONY_ACH_JUNGLE_FEVER");
								break;
							case 14:
								steamAchievement("BARONY_ACH_SANDMAN");
								break;

						}
					}

					// signal clients about level change
					mapseed = rand();
					lastEntityUIDs = entity_uids;
					currentlevel++;
					if ( localPlayerNetworkType == SERVER )
					{
						for ( c = 1; c < MAXPLAYERS; c++ )
						{
							if ( client_disconnected[c] == true )
							{
								continue;
							}
							strcpy((char*)net_packet->data, "LVLC");
							net_packet->data[4] = secretlevel;
							SDLNet_Write32(mapseed, &net_packet->data[5]);
							SDLNet_Write32(lastEntityUIDs, &net_packet->data[9]);
							net_packet->data[13] = currentlevel;
							net_packet->address.host = net_clients[c - 1].host;
							net_packet->address.port = net_clients[c - 1].port;
							net_packet->len = 14;
							sendPacketSafe(net_sock, -1, net_packet, c - 1);
						}
					}
					darkmap = false;
					numplayers = 0;
					if ( !secretlevel )
					{
						fp = fopen(LEVELSFILE, "r");
					}
					else
					{
						fp = fopen(SECRETLEVELSFILE, "r");
					}
					for ( i = 0; i < currentlevel; i++ )
						while ( fgetc(fp) != '\n' ) if ( feof(fp) )
							{
								break;
							}
					fscanf(fp, "%s", tempstr);
					while ( fgetc(fp) != ' ' ) if ( feof(fp) )
						{
							break;
						}
					int result = 0;
					if ( !strcmp(tempstr, "gen:") )
					{
						fscanf(fp, "%s", tempstr);
						while ( fgetc(fp) != '\n' ) if ( feof(fp) )
							{
								break;
							}
						result = generateDungeon(tempstr, mapseed);
					}
					else if ( !strcmp(tempstr, "map:") )
					{
						fscanf(fp, "%s", tempstr);
						while ( fgetc(fp) != '\n' ) if ( feof(fp) )
							{
								break;
							}
						result = loadMap(tempstr, &map, map.entities);
					}
					fclose(fp);
					assignActions(&map);
					generatePathMaps();

					// (special) unlock temple achievement
					if ( secretlevel && currentlevel == 8 )
					{
						steamAchievement("BARONY_ACH_TRICKS_AND_TRAPS");
					}

					if ( !secretlevel )
					{
						messagePlayer(clientnum, language[710], currentlevel);
					}
					else
					{
						messagePlayer(clientnum, language[711], map.name);
					}
					if ( !secretlevel && result )
					{
						switch ( currentlevel )
						{
							case 2:
								messagePlayer(clientnum, language[712]);
								break;
							case 3:
								messagePlayer(clientnum, language[713]);
								break;
							case 7:
								messagePlayer(clientnum, language[714]);
								break;
							case 8:
								messagePlayer(clientnum, language[715]);
								break;
							case 11:
								messagePlayer(clientnum, language[716]);
								break;
							case 13:
								messagePlayer(clientnum, language[717]);
								break;
							case 16:
								messagePlayer(clientnum, language[718]);
								break;
							case 18:
								messagePlayer(clientnum, language[719]);
								break;
							default:
								break;
						}
					}
					loadnextlevel = false;
					loading = false;
					fadeout = false;
					fadealpha = 255;

					for (c = 0; c < MAXPLAYERS; c++)
					{
						if (players[c] && players[c]->entity && !client_disconnected[c])
						{
							node_t* node;
							for (node = tempFollowers[c].first; node != nullptr; node = node->next)
							{
								Stat* tempStats = (Stat*)node->element;
								Entity* monster = summonMonster(tempStats->type, players[c]->entity->x, players[c]->entity->y);
								if (monster)
								{
									monster->skill[3] = 1; // to mark this monster partially initialized
									list_RemoveNode(monster->children.last);

									node_t* newNode = list_AddNodeLast(&monster->children);
									newNode->element = tempStats->copyStats();
//									newNode->deconstructor = &tempStats->~Stat;
									newNode->size = sizeof(tempStats);

									Stat* monsterStats = (Stat*)newNode->element;
									monsterStats->leader_uid = players[c]->entity->getUID();
									if (strcmp(monsterStats->name, ""))
									{
										messagePlayer(c, language[720], monsterStats->name);
									}
									else
									{
										messagePlayer(c, language[721], language[90 + (int)monsterStats->type]);
									}
									if (!monsterally[HUMAN][monsterStats->type])
									{
										monster->flags[USERFLAG2] = true;
									}

									newNode = list_AddNodeLast(&stats[c]->FOLLOWERS);
									newNode->deconstructor = &defaultDeconstructor;
									Uint32* myuid = (Uint32*) malloc(sizeof(Uint32));
									newNode->element = myuid;
									*myuid = monster->getUID();

									if ( c > 0 && localPlayerNetworkType == SERVER )
									{
										strcpy((char*)net_packet->data, "LEAD");
										SDLNet_Write32((Uint32)monster->getUID(), &net_packet->data[4]);
										net_packet->address.host = net_clients[c - 1].host;
										net_packet->address.port = net_clients[c - 1].port;
										net_packet->len = 8;
										sendPacketSafe(net_sock, -1, net_packet, c - 1);
									}
								}
								else
								{
									if ( strcmp(tempStats->name, "") )
									{
										messagePlayer(c, language[722], tempStats->name);
									}
									else
									{
										messagePlayer(c, language[723], language[90 + (int)tempStats->type]);
									}
								}
							}
						}
						list_FreeAll(&tempFollowers[c]);
					}

					saveGame();
					break;
				}
			}
			for ( node = map.entities->first; node != NULL; node = node->next )
			{
				entity = (Entity*)node->element;
				entity->ranbehavior = false;
			}

			if ( localPlayerNetworkType == SERVER )
			{
				// periodically remind clients of the current level
				if ( ticks % (TICKS_PER_SECOND * 3) == 0 )
				{
					for ( c = 1; c < MAXPLAYERS; c++ )
					{
						if ( client_disconnected[c] == true )
						{
							continue;
						}
						strcpy((char*)net_packet->data, "LVLC");
						net_packet->data[4] = secretlevel;
						SDLNet_Write32(mapseed, &net_packet->data[5]);
						SDLNet_Write32(lastEntityUIDs, &net_packet->data[9]);
						net_packet->data[13] = currentlevel;
						net_packet->address.host = net_clients[c - 1].host;
						net_packet->address.port = net_clients[c - 1].port;
						net_packet->len = 14;
						sendPacketSafe(net_sock, -1, net_packet, c - 1);
					}
				}

				// send entity info to clients
				if ( ticks % (TICKS_PER_SECOND / 8) == 0 )
				{
					for ( node = map.entities->first; node != NULL; node = node->next )
					{
						entity = (Entity*)node->element;
						for ( c = 1; c < MAXPLAYERS; c++ )
						{
							if ( !client_disconnected[c] )
							{
								if ( entity->flags[UPDATENEEDED] == true && entity->flags[NOUPDATE] == false )
								{
									// update entity for all clients
									if ( entity->getUID() % (TICKS_PER_SECOND * 4) == ticks % (TICKS_PER_SECOND * 4) )
									{
										sendEntityUDP(entity, c, true);
									}
									else
									{
										sendEntityUDP(entity, c, false);
									}
								}
							}
						}
					}
				}

				// handle keep alives
				for ( c = 1; c < MAXPLAYERS; c++ )
				{
					if ( client_disconnected[c] )
					{
						continue;
					}
					if ( ticks % (TICKS_PER_SECOND * 1) == 0 )
					{
						// send a keep alive every second
						strcpy((char*)net_packet->data, "KPAL");
						net_packet->data[4] = clientnum;
						net_packet->address.host = net_clients[c - 1].host;
						net_packet->address.port = net_clients[c - 1].port;
						net_packet->len = 5;
						sendPacketSafe(net_sock, -1, net_packet, c - 1);
					}
					if ( losingConnection[c] && ticks - client_keepalive[c] == 1 )
					{
						// regained connection
						losingConnection[c] = false;
						int i;
						for ( i = 0; i < MAXPLAYERS; i++ )
						{
							messagePlayer(i, language[724], c, stats[c]->name);
						}
					}
					else if ( !losingConnection[c] && ticks - client_keepalive[c] == TICKS_PER_SECOND * 30 - 1 )
					{
						// 30 second timer
						losingConnection[c] = true;
						int i;
						for ( i = 0; i < MAXPLAYERS; i++ )
						{
							messagePlayer(clientnum, language[725], c, stats[c]->name);
						}
					}
					else if ( !client_disconnected[c] && ticks - client_keepalive[c] >= TICKS_PER_SECOND * 45 - 1 )
					{
						// additional 15 seconds (kick time)
						int i;
						for ( i = 0; i < MAXPLAYERS; i++ )
						{
							messagePlayer(clientnum, language[726], c, stats[c]->name);
						}
						strcpy((char*)net_packet->data, "KICK");
						net_packet->address.host = net_clients[c - 1].host;
						net_packet->address.port = net_clients[c - 1].port;
						net_packet->len = 4;
						sendPacketSafe(net_sock, -1, net_packet, c - 1);
						client_disconnected[c] = true;
					}
				}
			}

			// update clients on assailant status
			for ( c = 1; c < MAXPLAYERS; c++ )
			{
				if ( !client_disconnected[c] )
				{
					if ( oassailant[c] != assailant[c] )
					{
						oassailant[c] = assailant[c];
						strcpy((char*)net_packet->data, "MUSM");
						net_packet->address.host = net_clients[c - 1].host;
						net_packet->address.port = net_clients[c - 1].port;
						net_packet->data[3] = assailant[c];
						net_packet->len = 4;
						sendPacketSafe(net_sock, -1, net_packet, c - 1);
					}
				}
			}
			combat = assailant[0];
			for ( j = 0; j < MAXPLAYERS; j++ )
			{
				client_selected[j] = NULL;
			}

			for ( node = stats[clientnum]->inventory.first; node != NULL; node = nextnode )
			{
				nextnode = node->next;
				Item* item = (Item*)node->element;

				// unlock achievements for special collected items
				switch ( item->type )
				{
					case ARTIFACT_SWORD:
						steamAchievement("BARONY_ACH_KING_ARTHURS_BLADE");
						break;
					case ARTIFACT_MACE:
						steamAchievement("BARONY_ACH_SPUD_LORD");
						break;
					case ARTIFACT_AXE:
						steamAchievement("BARONY_ACH_THANKS_MR_SKELTAL");
						break;
					case ARTIFACT_SPEAR:
						steamAchievement("BARONY_ACH_SPEAR_OF_DESTINY");
						break;
					default:
						break;
				}

				// drop any inventory items you don't have room for
				if ( item->x >= INVENTORY_SIZEX || item->y >= INVENTORY_SIZEY )
				{
					messagePlayer(clientnum, language[727], item->getName());
					while ( item->count > 1 )
					{
						dropItem(item, clientnum);
					}
					dropItem(item, clientnum);
				}
				else
				{
					if ( auto_appraise_new_items && appraisal_timer == 0 && !(item->identified) )
					{
						int appraisal_time = getAppraisalTime(item);
						if (appraisal_time < auto_appraise_lowest_time)
						{
							auto_appraise_target = item;
							auto_appraise_lowest_time = appraisal_time;
						}
					}
				}
			}

			if ( kills[SHOPKEEPER] >= 3 )
			{
				steamAchievement("BARONY_ACH_PROFESSIONAL_BURGLAR");
			}
			if ( kills[HUMAN] >= 10 )
			{
				steamAchievement("BARONY_ACH_HOMICIDAL_MANIAC");
			}
		}
		else if ( localPlayerNetworkType == CLIENT )
		{
			// keep alives
			if ( localPlayerNetworkType == CLIENT )
			{
				if ( ticks % (TICKS_PER_SECOND * 1) == 0 )
				{
					// send a keep alive every second
					strcpy((char*)net_packet->data, "KPAL");
					net_packet->data[4] = clientnum;
					net_packet->address.host = net_server.host;
					net_packet->address.port = net_server.port;
					net_packet->len = 5;
					sendPacketSafe(net_sock, -1, net_packet, 0);
				}
				if ( losingConnection[0] && ticks - client_keepalive[0] == 1 )
				{
					// regained connection
					losingConnection[0] = false;
					messagePlayer(i, language[728]);
				}
				else if ( !losingConnection[0] && ticks - client_keepalive[0] == TICKS_PER_SECOND * 30 - 1 )
				{
					// 30 second timer
					losingConnection[0] = true;
					messagePlayer(clientnum, language[729]);
				}
				else if ( !client_disconnected[c] && ticks - client_keepalive[0] >= TICKS_PER_SECOND * 45 - 1 )
				{
					// additional 15 seconds (disconnect time)
					messagePlayer(clientnum, language[730]);

					button_t* button;
					pauseGame(2, 0);

					// close current window
					buttonCloseSubwindow(NULL);
					for ( node = button_l.first; node != NULL; node = nextnode )
					{
						nextnode = node->next;
						button = (button_t*)node->element;
						if ( button->focused )
						{
							list_RemoveNode(button->node);
						}
					}

					// create new window
					subwindow = 1;
					subx1 = xres / 2 - 256;
					subx2 = xres / 2 + 256;
					suby1 = yres / 2 - 56;
					suby2 = yres / 2 + 56;
					strcpy(subtext, language[731]);

					// close button
					button = newButton();
					strcpy(button->label, "x");
					button->x = subx2 - 20;
					button->y = suby1;
					button->sizex = 20;
					button->sizey = 20;
					button->action = &buttonCloseAndEndGameConfirm;
					button->visible = 1;
					button->focused = 1;
					button->key = SDL_SCANCODE_ESCAPE;
					button->joykey = joyimpulses[INJOY_MENU_CANCEL];

					// okay button
					button = newButton();
					strcpy(button->label, language[732]);
					button->x = subx2 - (subx2 - subx1) / 2 - 28;
					button->y = suby2 - 28;
					button->sizex = 56;
					button->sizey = 20;
					button->action = &buttonCloseAndEndGameConfirm;
					button->visible = 1;
					button->focused = 1;
					button->key = SDL_SCANCODE_RETURN;
					button->joykey = joyimpulses[INJOY_MENU_NEXT];

					client_disconnected[0] = true;
				}
			}

			// animate tiles
			if ( !gamePaused )
			{
				int x, y, z;
				for ( x = 0; x < map.width; x++ )
				{
					for ( y = 0; y < map.height; y++ )
					{
						for ( z = 0; z < MAPLAYERS; z++ )
						{
							int index = z + y * MAPLAYERS + x * MAPLAYERS * map.height;
							if ( animatedtiles[map.tiles[index]] )
							{
								if ( ticks % 10 == 0 )
								{
									map.tiles[index]--;
									if ( !animatedtiles[map.tiles[index]] )
									{
										do
										{
											map.tiles[index]++;
										}
										while ( animatedtiles[map.tiles[index]] );
										map.tiles[index]--;
									}
								}
								if ( z == 0 )
								{
									// water and lava noises
									if ( ticks % TICKS_PER_SECOND == (y + x * map.height) % TICKS_PER_SECOND && rand() % 3 == 0 )
									{
										if ( lavatiles[map.tiles[index]] )
										{
											// bubbling lava
											playSoundPosLocal( x * 16 + 8, y * 16 + 8, 155, 100 );
										}
										else
										{
											// running water
											playSoundPosLocal( x * 16 + 8, y * 16 + 8, 135, 32 );
										}
									}

									// lava bubbles
									if ( lavatiles[map.tiles[index]] )
									{
										if ( ticks % 40 == (y + x * map.height) % 40 && rand() % 3 == 0 )
										{
											int c, j = 1 + rand() % 2;
											for ( c = 0; c < j; c++ )
											{
												Entity* entity = newEntity(42, 1, map.entities);
												entity->behavior = &actGib;
												entity->x = x * 16 + rand() % 16;
												entity->y = y * 16 + rand() % 16;
												entity->z = 7.5;
												entity->flags[PASSABLE] = true;
												entity->flags[SPRITE] = true;
												entity->flags[NOUPDATE] = true;
												entity->flags[UPDATENEEDED] = false;
												entity->flags[UNCLICKABLE] = true;
												entity->sizex = 2;
												entity->sizey = 2;
												entity->fskill[3] = 0.01;
												double vel = (rand() % 10) / 20.f;
												entity->vel_x = vel * cos(entity->yaw);
												entity->vel_y = vel * sin(entity->yaw);
												entity->vel_z = -.15 - (rand() % 15) / 100.f;
												entity->yaw = (rand() % 360) * PI / 180.0;
												entity->pitch = (rand() % 360) * PI / 180.0;
												entity->roll = (rand() % 360) * PI / 180.0;
												if ( localPlayerNetworkType != CLIENT )
												{
													entity_uids--;
												}
												entity->setUID(-3);
											}
										}
									}
								}
							}
						}
					}
				}
			}
			if ( conductPenniless )
			{
				if ( stats[clientnum]->GOLD > 0 )
				{
					conductPenniless = false;
				}
			}

			// ask for entity delete update
			if ( ticks % 4 == 0 && list_Size(map.entities) )
			{
				node_t* nodeToCheck = list_Node(map.entities, ticks % list_Size(map.entities));
				if ( nodeToCheck )
				{
					Entity* entity = (Entity*)nodeToCheck->element;
					if ( entity )
					{
						if ( !entity->flags[NOUPDATE] && entity->getUID() > 0 && entity->getUID() != -2 && entity->getUID() != -3 && entity->getUID() != -4 )
						{
							strcpy((char*)net_packet->data, "ENTE");
							net_packet->data[4] = clientnum;
							SDLNet_Write32(entity->getUID(), &net_packet->data[5]);
							net_packet->address.host = net_server.host;
							net_packet->address.port = net_server.port;
							net_packet->len = 9;
							sendPacket(net_sock, -1, net_packet, 0);
						}
					}
				}
			}

			// run entity actions
			for ( node = map.entities->first; node != NULL; node = nextnode )
			{
				nextnode = node->next;
				entity = (Entity*)node->element;
				if ( !entity->ranbehavior )
				{
					if ( !gamePaused || (localPlayerNetworkType && !client_disconnected[0]) )
					{
						entity->ticks++;
					}
					if ( entity->behavior != NULL )
					{
						if ( !gamePaused || (localPlayerNetworkType && !client_disconnected[0]) )
						{
							(*entity->behavior)(entity);
							if ( entitiesdeleted.first != NULL )
							{
								entitydeletedself = false;
								for ( node2 = entitiesdeleted.first; node2 != NULL; node2 = node2->next )
								{
									if ( entity == (Entity*)node2->element )
									{
										entitydeletedself = true;
										break;
									}
								}
								if ( entitydeletedself == false )
								{
									entity->ranbehavior = true;
								}
								nextnode = map.entities->first;
								list_FreeAll(&entitiesdeleted);
							}
							else
							{
								entity->ranbehavior = true;
								nextnode = node->next;
								if ( entity->flags[UPDATENEEDED] && !entity->flags[NOUPDATE] )
								{
									// adjust entity position
									if ( ticks - entity->lastupdate <= TICKS_PER_SECOND / 16 )
									{
										// interpolate to new position
										if ( entity->behavior != &actPlayerLimb || entity->skill[2] != clientnum )
										{
											entity->x += (entity->new_x - entity->x) / 4;
											entity->y += (entity->new_y - entity->y) / 4;
											entity->z += (entity->new_z - entity->z) / 4;
										}
									}
									// dead reckoning
									if ( fabs(entity->vel_x) > 0 || fabs(entity->vel_y) > 0 )
									{
										double ox = 0, oy = 0, onewx = 0, onewy = 0;
										if ( entity->behavior == &actPlayer || entity->behavior == &actMonster )
										{
											ox = entity->x;
											oy = entity->y;
											onewx = entity->new_x;
											onewy = entity->new_y;
										}
										clipMove(&entity->x, &entity->y, entity->vel_x, entity->vel_y, entity);
										clipMove(&entity->new_x, &entity->new_y, entity->vel_x, entity->vel_y, entity);
										if ( entity->behavior == &actPlayer )
										{
											node_t* node2;
											for ( node2 = map.entities->first; node2 != NULL; node2 = node2->next )
											{
												Entity* bodypart = (Entity*)node2->element;
												if ( bodypart->behavior == &actPlayerLimb )
												{
													if ( bodypart->skill[2] == entity->skill[2] )
													{
														bodypart->x += entity->x - ox;
														bodypart->y += entity->y - oy;
														bodypart->new_x += entity->new_x - onewx;
														bodypart->new_y += entity->new_y - onewy;
													}
												}
											}
										}
										if ( entity->behavior == &actMonster )
										{
											node_t* node2;
											for ( node2 = map.entities->first; node2 != NULL; node2 = node2->next )
											{
												Entity* bodypart = (Entity*)node2->element;
												if ( bodypart->skill[2] == entity->getUID() && bodypart->parent == entity->getUID() )
												{
													bodypart->x += entity->x - ox;
													bodypart->y += entity->y - oy;
													bodypart->new_x += entity->new_x - onewx;
													bodypart->new_y += entity->new_y - onewy;
												}
											}
										}
									}
									entity->z += entity->vel_z;
									entity->new_z += entity->vel_z;

									// rotate to new angles
									double dir = entity->new_yaw - entity->yaw;
									while ( dir >= PI )
									{
										dir -= PI * 2;
									}
									while ( dir < -PI )
									{
										dir += PI * 2;
									}
									entity->yaw += dir / 3;
									while ( entity->yaw < 0 )
									{
										entity->yaw += 2 * PI;
									}
									while ( entity->yaw >= 2 * PI )
									{
										entity->yaw -= 2 * PI;
									}
									dir = entity->new_pitch - entity->pitch;
									while ( dir >= PI )
									{
										dir -= PI * 2;
									}
									while ( dir < -PI )
									{
										dir += PI * 2;
									}
									entity->pitch += dir / 3;
									while ( entity->pitch < 0 )
									{
										entity->pitch += 2 * PI;
									}
									while ( entity->pitch >= 2 * PI )
									{
										entity->pitch -= 2 * PI;
									}
									dir = entity->new_roll - entity->roll;
									while ( dir >= PI )
									{
										dir -= PI * 2;
									}
									while ( dir < -PI )
									{
										dir += PI * 2;
									}
									entity->roll += dir / 3;
									while ( entity->roll < 0 )
									{
										entity->roll += 2 * PI;
									}
									while ( entity->roll >= 2 * PI )
									{
										entity->roll -= 2 * PI;
									}
								}
							}
						}
					}
				}
			}
			for ( node = map.entities->first; node != NULL; node = node->next )
			{
				entity = (Entity*)node->element;
				entity->ranbehavior = false;
			}

			for ( node = stats[clientnum]->inventory.first; node != NULL; node = nextnode )
			{
				nextnode = node->next;
				Item* item = (Item*)node->element;

				// unlock achievements for special collected items
				switch ( item->type )
				{
					case ARTIFACT_SWORD:
						steamAchievement("BARONY_ACH_KING_ARTHURS_BLADE");
						break;
					case ARTIFACT_MACE:
						steamAchievement("BARONY_ACH_SPUD_LORD");
						break;
					case ARTIFACT_AXE:
						steamAchievement("BARONY_ACH_THANKS_MR_SKELTAL");
						break;
					case ARTIFACT_SPEAR:
						steamAchievement("BARONY_ACH_SPEAR_OF_DESTINY");
						break;
					default:
						break;
				}

				// drop any inventory items you don't have room for
				if ( item->x >= INVENTORY_SIZEX || item->y >= INVENTORY_SIZEY )
				{
					messagePlayer(clientnum, language[727], item->getName());
					while ( item->count > 1 )
					{
						dropItem(item, clientnum);
					}
					dropItem(item, clientnum);
				}
				else
				{
					if ( auto_appraise_new_items && appraisal_timer == 0 && !(item->identified) )
					{
						int appraisal_time = getAppraisalTime(item);
						if (appraisal_time < auto_appraise_lowest_time)
						{
							auto_appraise_target = item;
							auto_appraise_lowest_time = appraisal_time;
						}
					}
				}
			}

			if ( kills[SHOPKEEPER] >= 3 )
			{
				steamAchievement("BARONY_ACH_PROFESSIONAL_BURGLAR");
			}
			if ( kills[HUMAN] >= 10 )
			{
				steamAchievement("BARONY_ACH_HOMICIDAL_MANIAC");
			}
		}

		// Automatically identify items, shortest time required first
		if ( auto_appraise_target != NULL )
		{
			//Cleanup identify GUI gamecontroller code here.
			selectedIdentifySlot = -1;

			identifygui_active = false;
			identifygui_appraising = true;
			identifyGUIIdentify(auto_appraise_target);
		}
	}
}

/*-------------------------------------------------------------------------------

	handleButtons

	Draws buttons and processes clicks

-------------------------------------------------------------------------------*/

//int subx1, subx2, suby1, suby2;
//char subtext[1024];

void handleButtons(void)
{
	node_t* node;
	node_t* nextnode;
	button_t* button;
	int w = 0, h = 0;

	// handle buttons
	for ( node = button_l.first; node != NULL; node = nextnode )
	{
		nextnode = node->next;
		if ( node->element == NULL )
		{
			continue;
		}
		button = (button_t*)node->element;
		if ( button == NULL )
		{
			continue;
		}
		if ( !subwindow && button->focused )
		{
			list_RemoveNode(button->node);
			continue;
		}
		//Hide "Random Character" button if not on first character creation step.
		if (!strcmp(button->label, language[733]))
		{
			if (charcreation_step > 1)
			{
				button->visible = 0;
			}
			else
			{
				button->visible = 1;
			}
		}
		//Hide "Random Name" button if not on character naming screen.
		if ( !strcmp(button->label, language[2450]) )
		{
			if ( charcreation_step != 4 )
			{
				button->visible = 0;
			}
			else
			{
				button->visible = 1;
			}
		}
		if ( button->visible == 0 )
		{
			continue;    // invisible buttons are not processed
		}
		TTF_SizeUTF8(ttf12, button->label, &w, &h);
		if ( subwindow && !button->focused )
		{
			// unfocused buttons do not work when a subwindow is active
			drawWindow(button->x, button->y, button->x + button->sizex, button->y + button->sizey);
			ttfPrintText(ttf12, button->x + (button->sizex - w) / 2 - 2, button->y + (button->sizey - h) / 2 + 3, button->label);
		}
		else
		{
			if ( keystatus[button->key] && button->key )
			{
				button->pressed = true;
				button->needclick = false;
			}
			if (button->joykey != -1 && *inputPressed(button->joykey))
			{
				button->pressed = true;
				button->needclick = false;
			}
			if ( mousestatus[SDL_BUTTON_LEFT] )
			{
				if ( mousex >= button->x && mousex < button->x + button->sizex && omousex >= button->x && omousex < button->x + button->sizex )
				{
					if ( mousey >= button->y && mousey < button->y + button->sizey && omousey >= button->y && omousey < button->y + button->sizey )
					{
						node_t* node;
						for ( node = button_l.first; node != NULL; node = node->next )
						{
							if ( node->element == NULL )
							{
								continue;
							}
							button_t* button = (button_t*)node->element;
							button->pressed = false;
						}
						button->pressed = true;
					}
					else if ( !keystatus[button->key] )
					{
						button->pressed = false;
					}
				}
				else if ( !keystatus[button->key] )
				{
					button->pressed = false;
				}
				button->needclick = true;
			}
			if ( button->pressed )
			{
				drawDepressed(button->x, button->y, button->x + button->sizex, button->y + button->sizey);
				ttfPrintText(ttf12, button->x + (button->sizex - w) / 2 - 2, button->y + (button->sizey - h) / 2 + 3, button->label);
				if ( !mousestatus[SDL_BUTTON_LEFT] && !keystatus[button->key] )
				{
					if ( ( omousex >= button->x && omousex < button->x + button->sizex ) || !button->needclick )
					{
						if ( ( omousey >= button->y && omousey < button->y + button->sizey ) || !button->needclick )
						{
							keystatus[button->key] = false;
							*inputPressed(button->joykey) = 0;
							playSound(139, 64);
							if ( button->action != NULL )
							{
								(*button->action)(button); // run the button's assigned action
								if ( deleteallbuttons )
								{
									deleteallbuttons = false;
									break;
								}
							}
						}
					}
					button->pressed = false;
				}
			}
			else
			{
				drawWindow(button->x, button->y, button->x + button->sizex, button->y + button->sizey);
				ttfPrintText(ttf12, button->x + (button->sizex - w) / 2 - 2, button->y + (button->sizey - h) / 2 + 3, button->label);
			}
		}

		if ( button->outline )
		{
			//Draw golden border.
			//For such things as which settings tab the controller has presently selected.
			Uint32 color = SDL_MapRGBA(mainsurface->format, 255, 255, 0, 127);
			SDL_Rect pos;
			pos.x = button->x;
			pos.w = button->sizex;
			pos.y = button->y;
			pos.h = button->sizey;
			drawBox(&pos, color, 127);
			//Draw a 2 pixel thick box.
			pos.x = button->x + 1;
			pos.w = button->sizex - 2;
			pos.y = button->y + 1;
			pos.h = button->sizey - 2;
			drawBox(&pos, color, 127);
		}
	}
}

/*-------------------------------------------------------------------------------

	handleEvents

	Handles all SDL events; receives input, updates gamestate, etc.

-------------------------------------------------------------------------------*/

void handleEvents(void)
{
	double d;
	int j;
	int runtimes = 0;

	// calculate app rate
	t = SDL_GetTicks();
	timesync = t - ot;
	ot = t;

	// calculate fps
	if ( timesync != 0 )
	{
		frameval[cycles % AVERAGEFRAMES] = 1.0 / timesync;
	}
	else
	{
		frameval[cycles % AVERAGEFRAMES] = 1.0;
	}
	d = frameval[0];
	for (j = 1; j < AVERAGEFRAMES; j++)
	{
		d += frameval[j];
	}
	fps = (d / AVERAGEFRAMES) * 1000;

	if (game_controller && game_controller->isActive())
	{
		game_controller->handleAnalog();
	}

	while ( SDL_PollEvent(&event) )   // poll SDL events
	{
		// Global events
		switch ( event.type )
		{
			case SDL_QUIT: // if SDL receives the shutdown signal
				mainloop = 0;
				break;
			case SDL_KEYDOWN: // if a key is pressed...
				if ( command )
				{
					if (event.key.keysym.sym == SDLK_UP)
					{
						if (!chosen_command && command_history.last)   //If no command is chosen (user has not tried to go up through the commands yet...
						{
							//Assign the chosen command as the last thing the user typed.
							chosen_command = command_history.last;
							strcpy(command_str, ((string_t* )chosen_command->element)->data);
						}
						else if (chosen_command)
						{
							//Scroll up through the list. Do nothing if already at the top.
							if (chosen_command->prev)
							{
								chosen_command = chosen_command->prev;
								strcpy(command_str, ((string_t* )chosen_command->element)->data);
							}
						}
					}
					else if (event.key.keysym.sym == SDLK_DOWN)
					{
						if (chosen_command)   //If a command is chosen...
						{
							//Scroll down through the history, back to the latest command.
							if (chosen_command->next)
							{
								//Move on to the newer command.
								chosen_command = chosen_command->next;
								strcpy(command_str, ((string_t* )chosen_command->element)->data);
							}
							else
							{
								//Already latest command. Clear the chosen command.
								chosen_command = NULL;
								strcpy(command_str, "");
							}
						}
					}
				}
				if ( SDL_IsTextInputActive() )
				{
#ifdef APPLE
					if ( (event.key.keysym.sym == SDLK_DELETE || event.key.keysym.sym == SDLK_BACKSPACE) && strlen(inputstr) > 0 )
					{
						inputstr[strlen(inputstr) - 1] = 0;
						cursorflash = ticks;
					}
#else
					if ( event.key.keysym.sym == SDLK_BACKSPACE && strlen(inputstr) > 0 )
					{
						inputstr[strlen(inputstr) - 1] = 0;
						cursorflash = ticks;
					}
#endif
					else if ( event.key.keysym.sym == SDLK_c && SDL_GetModState()&KMOD_CTRL )
					{
						SDL_SetClipboardText(inputstr);
						cursorflash = ticks;
					}
					else if ( event.key.keysym.sym == SDLK_v && SDL_GetModState()&KMOD_CTRL )
					{
						strncpy(inputstr, SDL_GetClipboardText(), inputlen);
						cursorflash = ticks;
					}
				}
#ifdef PANDORA
				// Pandora Shoulder as Mouse Button handling
				if(event.key.keysym.sym==SDLK_RCTRL) { // L
					mousestatus[SDL_BUTTON_LEFT] = 1; // set this mouse button to 1
					lastkeypressed = 282 + SDL_BUTTON_LEFT;
				} else if (event.key.keysym.sym==SDLK_RSHIFT) { // R
					mousestatus[SDL_BUTTON_RIGHT] = 1; // set this mouse button to 1
					lastkeypressed = 282 + SDL_BUTTON_RIGHT;
				} else 
#endif
				{
					lastkeypressed = event.key.keysym.scancode;
					keystatus[event.key.keysym.scancode] = 1; // set this key's index to 1
				}
				break;
			case SDL_KEYUP: // if a key is unpressed...
#ifdef PANDORA
				if(event.key.keysym.sym==SDLK_RCTRL) { // L
					mousestatus[SDL_BUTTON_LEFT] = 0; // set this mouse button to 0
					lastkeypressed = 282 + SDL_BUTTON_LEFT;
				} else if (event.key.keysym.sym==SDLK_RSHIFT) { // R
					mousestatus[SDL_BUTTON_RIGHT] = 0; // set this mouse button to 0
					lastkeypressed = 282 + SDL_BUTTON_RIGHT;
				} else 
#endif
				{
					keystatus[event.key.keysym.scancode] = 0; // set this key's index to 0
				}
				break;
			case SDL_TEXTINPUT:
				if ( (event.text.text[0] != 'c' && event.text.text[0] != 'C') || !(SDL_GetModState()&KMOD_CTRL) )
				{
					if ( (event.text.text[0] != 'v' && event.text.text[0] != 'V') || !(SDL_GetModState()&KMOD_CTRL) )
					{
						strncat(inputstr, event.text.text, std::max<size_t>(0, inputlen - strlen(inputstr)));
						cursorflash = ticks;
					}
				}
				break;
			case SDL_MOUSEBUTTONDOWN: // if a mouse button is pressed...
				mousestatus[event.button.button] = 1; // set this mouse button to 1
				lastkeypressed = 282 + event.button.button;
				break;
			case SDL_MOUSEBUTTONUP: // if a mouse button is released...
				mousestatus[event.button.button] = 0; // set this mouse button to 0
				buttonclick = 0; // release any buttons that were being held down
				gui_clickdrag = false;
				break;
			case SDL_MOUSEWHEEL:
				if ( event.wheel.y > 0 )
				{
					mousestatus[SDL_BUTTON_WHEELUP] = 1;
					lastkeypressed = 287;
				}
				else if ( event.wheel.y < 0 )
				{
					mousestatus[SDL_BUTTON_WHEELDOWN] = 1;
					lastkeypressed = 288;
				}
				break;
			case SDL_MOUSEMOTION: // if the mouse is moved...
				if ( firstmouseevent == true )
				{
					firstmouseevent = false;
					break;
				}
				menuselect = 0;
				mousex = event.motion.x;
				mousey = event.motion.y;
#ifdef PANDORA
				if(xres!=800 || yres!=480) {	// SEB Pandora 
					mousex = (mousex*xres)/800;
					mousey = (mousey*yres)/480;
				}
#endif
				mousexrel += event.motion.xrel;
				mouseyrel += event.motion.yrel;

				if (!draw_cursor)
				{
					draw_cursor = true;
				}
				break;
			case SDL_CONTROLLERBUTTONDOWN: // if joystick button is pressed
				joystatus[event.cbutton.button] = 1; // set this button's index to 1
				lastkeypressed = 301 + event.cbutton.button;
				if ( event.cbutton.button + 301 == joyimpulses[INJOY_MENU_LEFT_CLICK] && ( (!shootmode && gui_mode == GUI_MODE_NONE) || gamePaused) && rebindaction == -1 )
				{
					//Generate a mouse click.
					SDL_Event e;

					e.type = SDL_MOUSEBUTTONDOWN;
					e.button.button = SDL_BUTTON_LEFT;
					e.button.clicks = 1; //Single click.
					SDL_PushEvent(&e);
				}
				break;
			case SDL_CONTROLLERBUTTONUP: // if joystick button is released
				joystatus[event.cbutton.button] = 0; // set this button's index to 0
				if (event.cbutton.button + 301 == joyimpulses[INJOY_MENU_LEFT_CLICK])
				{
					//Generate a mouse lift.
					SDL_Event e;

					e.type = SDL_MOUSEBUTTONUP;
					e.button.button = SDL_BUTTON_LEFT;
					SDL_PushEvent(&e);
				}
				break;
			case SDL_JOYHATMOTION:
				break;
			case SDL_USEREVENT: // if the game timer has elapsed
				if ( runtimes < 5 )
				{
					runtimes++;
					gameLogic();
					mousexrel = 0;
					mouseyrel = 0;
				}
				break;
				/*case SDL_CONTROLLERAXISMOTION:
					printlog("Controller axis motion detected.\n");
					//if (event.caxis.which == 0) //TODO: Multi-controller support.
					//{
						printlog("Controller 0!\n");
						int x = 0, y = 0;
						if (event.caxis.axis == 0) //0 = x axis.
						{
							printlog("X-axis! Value: %d\n", event.caxis.value);
							if (event.caxis.value < -gamepad_deadzone)
							{
								printlog("Left!\n");
								x = -1; //Gamepad moved left.
							}
							else if (event.caxis.value > gamepad_deadzone)
							{
								printlog("Right!\n");
								x = 1; //Gamepad moved right.
							}
						}
						else if (event.caxis.axis  == 1)
						{
							if (event.caxis.value < -gamepad_deadzone)
							{
								printlog("Up!\n");
								y = -1; //Gamepad moved up.
							}
							else if (event.caxis.value > gamepad_deadzone)
							{
								printlog("Down!\n");
								y = 1; //Gamepad moved down.
							}
						}

						if (x || y)
						{
							printlog("Generating mouse motion!\n");
							SDL_Event e;

							e.type = SDL_MOUSEMOTION;
							e.motion.x += x;
							e.motion.y += y;
							e.motion.xrel = x;
							e.motion.yrel = y;
							SDL_PushEvent(&e);
						}
					//}
					break;*/
		}
	}
	if ( !mousestatus[SDL_BUTTON_LEFT] )
	{
		omousex = mousex;
		omousey = mousey;
	}
}

/*-------------------------------------------------------------------------------

	timerCallback

	A callback function for the game timer which pushes an SDL event

-------------------------------------------------------------------------------*/

Uint32 timerCallback(Uint32 interval, void* param)
{
	SDL_Event event;
	SDL_UserEvent userevent;

	userevent.type = SDL_USEREVENT;
	userevent.code = 0;
	userevent.data1 = NULL;
	userevent.data2 = NULL;

	event.type = SDL_USEREVENT;
	event.user = userevent;

	int c;
	bool playeralive = false;
	for (c = 0; c < MAXPLAYERS; c++)
		if (players[c] && players[c]->entity && !client_disconnected[c])
		{
			playeralive = true;
		}

	if ((!gamePaused || localPlayerNetworkType) && !loading && !intro && playeralive)
	{
		completionTime++;
	}
	ticks++;
	if (!loading)
	{
		SDL_PushEvent(&event);    // so the game doesn't overload itself while loading
	}
	return (interval);
}

/*-------------------------------------------------------------------------------

	startMessages

	prints several messages to the console for game start.

-------------------------------------------------------------------------------*/

void startMessages()
{
	newString(&messages, 0xFFFFFFFF, language[734], stats[clientnum]->name);
	newString(&messages, 0xFFFFFFFF, language[735], getInputName(impulses[IN_STATUS]));
	newString(&messages, 0xFFFFFFFF, language[736]);
	newString(&messages, 0xFFFFFFFF, language[737]);
}

/*-------------------------------------------------------------------------------

	pauseGame

	pauses or unpauses the game, depending on its current state

-------------------------------------------------------------------------------*/

void pauseGame(int mode, int ignoreplayer)
{
	int c;

	if ( intro )
	{
		return;
	}
	if ( mode == 1 && !gamePaused )
	{
		return;
	}
	if ( mode == 2 && gamePaused )
	{
		return;
	}

	if ( (!gamePaused && mode != 1) || mode == 2 )
	{
		gamePaused = true;
		if ( SDL_GetRelativeMouseMode() )
		{
			SDL_SetRelativeMouseMode(SDL_FALSE);
		}
		return; // doesn't disable the game in localPlayerNetworkType anymore
		if ( localPlayerNetworkType == SERVER )
		{
			for ( c = 1; c < MAXPLAYERS; c++ )
			{
				if ( client_disconnected[c] || ignoreplayer == c )
				{
					continue;
				}
				strcpy((char*)net_packet->data, "PAUS");
				net_packet->data[4] = clientnum;
				net_packet->address.host = net_clients[c - 1].host;
				net_packet->address.port = net_clients[c - 1].port;
				net_packet->len = 5;
				sendPacketSafe(net_sock, -1, net_packet, c - 1);
			}
		}
		else if ( localPlayerNetworkType == CLIENT && ignoreplayer )
		{
			strcpy((char*)net_packet->data, "PAUS");
			net_packet->data[4] = clientnum;
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 5;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
	}
	else if ( (gamePaused && mode != 2) || mode == 1 )
	{
		buttonCloseSubwindow(NULL);
		gamePaused = false;
		if ( !SDL_GetRelativeMouseMode() && capture_mouse )
		{
			SDL_SetRelativeMouseMode(SDL_TRUE);
		}
		return; // doesn't disable the game in localPlayerNetworkType anymore
		if ( localPlayerNetworkType == SERVER )
		{
			for ( c = 1; c < MAXPLAYERS; c++ )
			{
				if ( client_disconnected[c] || ignoreplayer == c )
				{
					continue;
				}
				strcpy((char*)net_packet->data, "UNPS");
				net_packet->data[4] = clientnum;
				net_packet->address.host = net_clients[c - 1].host;
				net_packet->address.port = net_clients[c - 1].port;
				net_packet->len = 5;
				sendPacketSafe(net_sock, -1, net_packet, c - 1);
			}
		}
		else if ( localPlayerNetworkType == CLIENT && ignoreplayer )
		{
			strcpy((char*)net_packet->data, "UNPS");
			net_packet->data[4] = clientnum;
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 5;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
	}
}

/*-------------------------------------------------------------------------------

	frameRateLimit

	Returns true until the correct number of frames has passed from the
	beginning of the last cycle in the main loop.

-------------------------------------------------------------------------------*/

// records the SDL_GetTicks() value at the moment the mainloop restarted
Uint32 lastGameTickCount = 0;

bool frameRateLimit( Uint32 maxFrameRate )
{
	float desiredFrameMilliseconds = 1000.0f / maxFrameRate;
	Uint32 gameTickCount = SDL_GetTicks();

	float millisecondsElapsed = (float)(gameTickCount - lastGameTickCount);
	if ( millisecondsElapsed < desiredFrameMilliseconds )
	{
		// if enough time is left sleep, otherwise just keep spinning so we don't go over the limit...
		if ( desiredFrameMilliseconds - millisecondsElapsed > 3.0f )
		{
#ifndef WINDOWS
			usleep( 5000 );
#else
			Sleep( 5 );
#endif
		}

		return true;
	}
	else
	{
		return false;
	}
}

/*-------------------------------------------------------------------------------

	main

	Initializes game resources, harbors main game loop, and cleans up
	afterwords

-------------------------------------------------------------------------------*/

#include <stdio.h>
//#include <unistd.h>
#include <stdlib.h>
#ifdef APPLE
#include <mach-o/dyld.h>
#endif

int main(int argc, char** argv)
{

#ifdef LINUX
	//Catch segfault stuff.
	struct sigaction sa;

	memset(&sa, 0, sizeof(struct sigaction));
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = segfault_sigaction;
	sa.sa_flags = SA_SIGINFO;

	sigaction(SIGSEGV, &sa, NULL);
#endif

	try
	{
#ifdef APPLE
		uint32_t buffsize = 4096;
		char binarypath[buffsize];
		int result = _NSGetExecutablePath(binarypath, &buffsize);
		if (result == 0)   //It worked.
		{
			printlog("Binary path: %s\n", binarypath);
			char* last = strrchr(binarypath, '/');
			*last = '\0';
			char execpath[buffsize];
			strcpy(execpath, binarypath);
			//char* last = strrchr(execpath, '/');
			//strcat(execpath, '/');
			//strcat(execpath, "/../../../");
			printlog("Chrooting to directory: %s\n", execpath);
			chdir(execpath);
			///Users/ciprian/barony/barony-sdl2-take2/barony.app/Contents/MacOS/barony
			chdir("..");
			//chdir("..");
			chdir("Resources");
			//chdir("..");
			//chdir("..");
		}
		else
		{
			printlog("Failed to get binary path. Program may not work corectly!\n");
		}
#endif
		SDL_Rect pos, src;
		int c;
		//int tilesreceived=0;
		//Mix_Music **music, *intromusic, *splashmusic, *creditsmusic;
		node_t* node;
		Entity* entity;
		FILE* fp;
		//SDL_Surface *sky_bmp;
		light_t* light;

		// load default language file (english)
		if ( loadLanguage("en") )
		{
			printlog("Fatal error: failed to load default language file!\n");
			fclose(logfile);
			exit(1);
		}

		// read command line arguments
		if ( argc > 1 )
		{
			for (c = 1; c < argc; c++)
			{
				if ( argv[c] != NULL )
				{
					if ( !strcmp(argv[c], "-windowed") )
					{
						fullscreen = 0;
					}
					else if ( !strncmp(argv[c], "-size=", 6) )
					{
						strncpy(tempstr, argv[c] + 6, strcspn(argv[c] + 6, "x"));
						xres = std::max(320, atoi(tempstr));
						yres = std::max(200, atoi(argv[c] + 6 + strcspn(argv[c] + 6, "x") + 1));
					}
					else if ( !strncmp(argv[c], "-map=", 5) )
					{
						strcpy(maptoload, argv[c] + 5);
						loadingmap = true;
					}
					else if ( !strncmp(argv[c], "-gen=", 5) )
					{
						strcpy(maptoload, argv[c] + 5);
						loadingmap = true;
						genmap = true;
					}
					else if ( !strncmp(argv[c], "-config=", 8) )
					{
						strcpy(configtoload, argv[c] + 5);
						loadingconfig = true;
					}
					else if (!strncmp(argv[c], "-quickstart=", 12))
					{
						strcpy(classtoquickstart, argv[c] + 12);
					}
				}
			}
		}

		// load config file
		if ( loadingconfig )
		{
			loadConfig(configtoload);
		}
		else
		{
			loadConfig("default.cfg");
		}

		// initialize engine
		if ( (c = initApp("Barony", fullscreen)) )
		{
			printlog("Critical error: %d\n", c);
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Uh oh",
			                         "Barony has encountered a critical error and cannot start.\n\n"
			                         "Please check the log.txt file in the game directory for additional info,\n"
			                         "or contact us through our website at http://www.baronygame.com/ for support.",
			                         screen);
			deinitApp();
			exit(c);
		}

		// init message
		printlog("Barony version: %s\n", VERSION);
		time_t timething;
		char buffer[32];
		struct tm* tm_info;
		time(&timething);
		tm_info = localtime(&timething);
		strftime( buffer, 32, "%Y-%m-%d %H-%M-%S", tm_info );
		printlog("Launch time: %s\n", buffer);

		if ( (c = initGame()) )
		{
			printlog("Critical error in initGame: %d\n", c);
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Uh oh",
			                         "Barony has encountered a critical error and cannot start.\n\n"
			                         "Please check the log.txt file in the game directory for additional info,\n"
			                         "or contact us through our website at http://www.baronygame.com/ for support.",
			                         screen);
			deinitGame();
			deinitApp();
			exit(c);
		}
		initialized = true;

		// initialize map
		map.tiles = NULL;
		map.entities = (list_t*) malloc(sizeof(list_t));
		map.entities->first = NULL;
		map.entities->last = NULL;

		// instantiate a timer
		timer = SDL_AddTimer(1000 / TICKS_PER_SECOND, timerCallback, NULL);
		srand(time(NULL));

		// play splash sound
#ifdef MUSIC
		playmusic(splashmusic, false, false, false);
#endif

		int old_sdl_ticks = 0;
		int indev_timer = 0;

		// main loop
		printlog("running main loop.\n");
		while (mainloop)
		{
			// record the time at the start of this cycle
			lastGameTickCount = SDL_GetTicks();

			// game logic
			if ( !intro )
			{
				// handle network messages
				if ( localPlayerNetworkType == CLIENT )
				{
					clientHandleMessages();
				}
				else if ( localPlayerNetworkType == SERVER )
				{
					serverHandleMessages();
				}
			}
			handleEvents();

			// handle steam callbacks
#ifdef STEAMWORKS
			SteamAPI_RunCallbacks();
#endif

			if ( intro )
			{
				shootmode = false; //Hack because somebody put a shootmode = true where it don't belong, which might and does break stuff.
				if ( introstage == -1 )
				{
					// hack to fix these things from breaking everything...
					hudarm = NULL;
					hudweapon = NULL;
					magicLeftHand = NULL;
					magicRightHand = NULL;

					// team splash
					drawRect(NULL, 0, 255);
					drawGear(xres / 2, yres / 2, gearsize, gearrot);
					drawLine(xres / 2 - 160, yres / 2 + 112, xres / 2 + 160, yres / 2 + 112, SDL_MapRGB(mainsurface->format, 127, 0, 0), std::min<Uint16>(logoalpha, 255));
					printTextFormattedAlpha(font16x16_bmp, (xres / 2) - strlen("Turning Wheel") * 9, yres / 2 + 128, std::min<Uint16>(std::max<Uint16>(0, logoalpha), 255), "Turning Wheel");
					if ( (logoalpha >= 255 || keystatus[SDL_SCANCODE_ESCAPE] || *inputPressed(joyimpulses[INJOY_MENU_NEXT]) || *inputPressed(joyimpulses[INJOY_MENU_CANCEL])) && !fadeout )
					{
						fadeout = true;
					}
					if ( fadefinished || keystatus[SDL_SCANCODE_ESCAPE] || *inputPressed(joyimpulses[INJOY_MENU_NEXT]) || *inputPressed(joyimpulses[INJOY_MENU_CANCEL]))
					{
						keystatus[SDL_SCANCODE_ESCAPE] = 0;
						*inputPressed(joyimpulses[INJOY_MENU_NEXT]) = 0;
						*inputPressed(joyimpulses[INJOY_MENU_CANCEL]) = 0;
						fadealpha = 255;
#ifndef STEAMWORKS
						introstage = 0;
						fadeout = false;
						fadefinished = false;
#else
						switch ( rand() % 4 )
						{
							case 0:
								loadMap("mainmenu1", &map, map.entities);
								camera.x = 8;
								camera.y = 4.5;
								camera.z = 0;
								camera.ang = 0.6;
								break;
							case 1:
								loadMap("mainmenu2", &map, map.entities);
								camera.x = 7;
								camera.y = 4;
								camera.z = -4;
								camera.ang = 1.0;
								break;
							case 2:
								loadMap("mainmenu3", &map, map.entities);
								camera.x = 5;
								camera.y = 3;
								camera.z = 0;
								camera.ang = 1.0;
								break;
							case 3:
								loadMap("mainmenu4", &map, map.entities);
								camera.x = 6;
								camera.y = 14.5;
								camera.z = -24;
								camera.ang = 5.0;
								break;
						}
						numplayers = 0;
						localPlayerNetworkType = 0;
						assignActions(&map);
						generatePathMaps();
						fadeout = true;
						fadefinished = false;
						if ( !skipintro && !strcmp(classtoquickstart, "") )
						{
							introstage = 6;
#if defined(HAVE_FMOD) || defined(HAVE_OPENAL)
							playmusic(introductionmusic, true, false, false);
#endif
						}
						else
						{
							introstage = 1;
							fadeout = false;
							fadefinished = false;
#if defined(HAVE_FMOD) || defined(HAVE_OPENAL)
							playmusic(intromusic, true, false, false);
#endif
						}
#endif
					}
				}
				else if ( introstage == 0 )
				{
					// hack to fix these things from breaking everything...
					hudarm = NULL;
					hudweapon = NULL;
					magicLeftHand = NULL;
					magicRightHand = NULL;

					drawRect(NULL, 0, 255);
					char* banner_text1 = language[738];
					char* banner_text2 = "\n\n\n\n\n\n\n - Turning Wheel";
					ttfPrintText(ttf16, (xres / 2) - longestline(banner_text1)*TTF16_WIDTH / 2, yres / 2 - TTF16_HEIGHT / 2 * 7, banner_text1);
					Uint32 colorBlue = SDL_MapRGBA(mainsurface->format, 0, 92, 255, 255);
					ttfPrintTextColor(ttf16, (xres / 2) - longestline(banner_text1)*TTF16_WIDTH / 2, yres / 2 - TTF16_HEIGHT / 2 * 7, colorBlue, true, banner_text2);

					int time_passed = 0;
					if (old_sdl_ticks == 0)
					{
						old_sdl_ticks = SDL_GetTicks();
					}
					time_passed = SDL_GetTicks() - old_sdl_ticks;
					old_sdl_ticks = SDL_GetTicks();
					indev_timer += time_passed;

					//if( (*inputPressed(joyimpulses[INJOY_MENU_NEXT]) || *inputPressed(joyimpulses[INJOY_MENU_CANCEL]) || *inputPressed(joyimpulses[INJOY_BACK]) || keystatus[SDL_SCANCODE_ESCAPE] || keystatus[SDL_SCANCODE_SPACE] || keystatus[SDL_SCANCODE_RETURN] || mousestatus[SDL_BUTTON_LEFT] || indev_timer >= indev_displaytime) && !fadeout) {
					if ( (*inputPressed(joyimpulses[INJOY_MENU_NEXT]) || *inputPressed(joyimpulses[INJOY_MENU_CANCEL]) || keystatus[SDL_SCANCODE_ESCAPE] || keystatus[SDL_SCANCODE_SPACE] || keystatus[SDL_SCANCODE_RETURN] || mousestatus[SDL_BUTTON_LEFT] || indev_timer >= indev_displaytime) && !fadeout)
					{
						switch ( rand() % 4 )
						{
							case 0:
								loadMap("mainmenu1", &map, map.entities);
								camera.x = 8;
								camera.y = 4.5;
								camera.z = 0;
								camera.ang = 0.6;
								break;
							case 1:
								loadMap("mainmenu2", &map, map.entities);
								camera.x = 7;
								camera.y = 4;
								camera.z = -4;
								camera.ang = 1.0;
								break;
							case 2:
								loadMap("mainmenu3", &map, map.entities);
								camera.x = 5;
								camera.y = 3;
								camera.z = 0;
								camera.ang = 1.0;
								break;
							case 3:
								loadMap("mainmenu4", &map, map.entities);
								camera.x = 6;
								camera.y = 14.5;
								camera.z = -24;
								camera.ang = 5.0;
								break;
						}
						numplayers = 0;
						localPlayerNetworkType = 0;
						assignActions(&map);
						generatePathMaps();
						fadeout = true;
						fadefinished = false;
					}
					if ( fadefinished )
					{
						if ( !skipintro && !strcmp(classtoquickstart, "") )
						{
							introstage = 6;
#ifdef MUSIC
							playmusic(introductionmusic, true, false, false);
#endif
						}
						else
						{
							introstage = 1;
							fadeout = false;
							fadefinished = false;
#ifdef MUSIC
							playmusic(intromusic, true, false, false);
#endif
						}
					}
				}
				else
				{
					if (strcmp(classtoquickstart, ""))
					{
						for ( c = 0; c < 10; c++ )
						{
							if ( !strcmp(classtoquickstart, language[1900 + c]) )
							{
								client_classes[0] = c;
								break;
							}
						}

						// generate unique game key
						prng_seed_time();
						uniqueGameKey = prng_get_uint();
						if ( !uniqueGameKey )
						{
							uniqueGameKey++;
						}
						loading = true;

						// hack to fix these things from breaking everything...
						hudarm = NULL;
						hudweapon = NULL;
						magicLeftHand = NULL;
						magicRightHand = NULL;

						// reset class loadout
						stats[0]->sex = static_cast<sex_t>(rand() % 2);
						stats[0]->appearance = rand() % NUMAPPEARANCES;
						stats[0]->clearStats();
						initClass(0);

						strcpy(stats[0]->name, "Avatar");
						localPlayerNetworkType = SINGLE;
						fadefinished = false;
						fadeout = false;
						numplayers = 0;

						//TODO: Replace all of this with centralized startGameRoutine().
						// setup game
						shootmode = true;

						// make some messages
						startMessages();

						// load dungeon
						mapseed = 0;
						lastEntityUIDs = entity_uids;
						for ( node = map.entities->first; node != NULL; node = node->next )
						{
							entity = (Entity*)node->element;
							entity->flags[NOUPDATE] = true;
						}
						if ( loadingmap == false )
						{
							if ( !secretlevel )
							{
								fp = fopen(LEVELSFILE, "r");
							}
							else
							{
								fp = fopen(SECRETLEVELSFILE, "r");
							}
							fscanf(fp, "%s", tempstr);
							while ( fgetc(fp) != ' ' ) if ( feof(fp) )
								{
									break;
								}
							if ( !strcmp(tempstr, "gen:") )
							{
								fscanf(fp, "%s", tempstr);
								while ( fgetc(fp) != '\n' ) if ( feof(fp) )
									{
										break;
									}
								generateDungeon(tempstr, rand());
							}
							else if ( !strcmp(tempstr, "map:") )
							{
								fscanf(fp, "%s", tempstr);
								while ( fgetc(fp) != '\n' ) if ( feof(fp) )
									{
										break;
									}
								loadMap(tempstr, &map, map.entities);
							}
							fclose(fp);
						}
						else
						{
							if ( genmap == false )
							{
								loadMap(maptoload, &map, map.entities);
							}
							else
							{
								generateDungeon(maptoload, rand());
							}
						}
						assignActions(&map);
						generatePathMaps();

						saveGame();

						// kick off the main loop!
						strcpy(classtoquickstart, "");
						intro = false;
						loading = false;
					}
					else
					{

						// draws the menu level "backdrop"
						drawClearBuffers();
						if ( movie == false )
						{
							camera.winx = 0;
							camera.winy = 0;
							camera.winw = xres;
							camera.winh = yres;
							light = lightSphere(camera.x, camera.y, 16, 64);
							raycast(&camera, REALCOLORS);
							glDrawWorld(&camera, REALCOLORS);
							//drawFloors(&camera);
							drawEntities3D(&camera, REALCOLORS);
							list_RemoveNode(light->node);
						}

						handleMainMenu(intro);

						// draw mouse
						if (!movie && draw_cursor)
						{
							pos.x = mousex - cursor_bmp->w / 2;
							pos.y = mousey - cursor_bmp->h / 2;
							pos.w = 0;
							pos.h = 0;
							drawImageAlpha(cursor_bmp, NULL, &pos, 192);
						}
					}
				}
			}
			else
			{
				if ( localPlayerNetworkType == CLIENT )
				{
					// make sure shop inventory is alloc'd
					if ( !shopInv )
					{
						shopInv = (list_t*) malloc(sizeof(list_t));
						shopInv->first = NULL;
						shopInv->last = NULL;
					}
				}
#ifdef MUSIC
				handleLevelMusic();
#endif

				// toggling the game menu
				if ( (keystatus[SDL_SCANCODE_ESCAPE] || (*inputPressed(joyimpulses[INJOY_PAUSE_MENU]) && rebindaction == -1)) && !command )
				{
					keystatus[SDL_SCANCODE_ESCAPE] = 0;
					*inputPressed(joyimpulses[INJOY_PAUSE_MENU]) = 0;
					if ( !shootmode )
					{
						shootmode = true;
						gui_mode = GUI_MODE_INVENTORY;
						identifygui_active = false;
						selectedIdentifySlot = -1;
						closeRemoveCurseGUI();
						if ( shopkeeper != 0 )
						{
							if ( localPlayerNetworkType != CLIENT )
							{
								Entity* entity = uidToEntity(shopkeeper);
								entity->skill[0] = 0;
								if ( uidToEntity(entity->skill[1]) )
								{
									monsterMoveAside(entity, uidToEntity(entity->skill[1]));
								}
								entity->skill[1] = 0;
							}
							else
							{
								// inform server that we're done talking to shopkeeper
								strcpy((char*)net_packet->data, "SHPC");
								SDLNet_Write32((Uint32)shopkeeper, &net_packet->data[4]);
								net_packet->address.host = net_server.host;
								net_packet->address.port = net_server.port;
								net_packet->len = 8;
								sendPacketSafe(net_sock, -1, net_packet, 0);
								list_FreeAll(shopInv);
							}
							shopkeeper = 0;

							//Clean up shopkeeper gamepad code here.
							selectedShopSlot = -1;
						}
						attributespage = 0;
						if (openedChest[clientnum])
						{
							openedChest[clientnum]->closeChest();
						}
					}
					else
					{
						pauseGame(0, MAXPLAYERS);
					}
				}

				// main drawing
				drawClearBuffers();
				camera.ang += camera_shakex2;
				camera.vang += camera_shakey2 / 200.0;
				if (players[clientnum] == nullptr || players[clientnum]->entity == nullptr || !players[clientnum]->entity->isBlind())
				{
					// drunkenness spinning
					double cosspin = cos(ticks % 360 * PI / 180.f) * 0.25;
					double sinspin = sin(ticks % 360 * PI / 180.f) * 0.25;

					//drawSky3D(&camera,sky_bmp);
					camera.winx = 0;
					camera.winy = 0;
					camera.winw = xres;
					camera.winh = yres;
					if (shaking && players[clientnum] && players[clientnum]->entity && !gamePaused)
					{
						camera.ang += cosspin * drunkextend;
						camera.vang += sinspin * drunkextend;
					}
					raycast(&camera, REALCOLORS);

					glDrawWorld(&camera, REALCOLORS);
					//drawFloors(&camera);
					drawEntities3D(&camera, REALCOLORS);
					if (shaking && players[clientnum] && players[clientnum]->entity && !gamePaused)
					{
						camera.ang -= cosspin * drunkextend;
						camera.vang -= sinspin * drunkextend;
					}
				}
				camera.ang -= camera_shakex2;
				camera.vang -= camera_shakey2 / 200.0;

				updateMessages();
				if ( !nohud )
				{
					handleDamageIndicators();
					drawMessages();
				}

				if ( !gamePaused )
				{
					// status bar
					if ( !nohud )
					{
						drawStatus();
					}

					// interface
					if ( (*inputPressed(impulses[IN_STATUS]) || *inputPressed(joyimpulses[INJOY_STATUS])) )
					{
						*inputPressed(impulses[IN_STATUS]) = 0;
						*inputPressed(joyimpulses[INJOY_STATUS]) = 0;

						if ( shootmode )
						{
							openStatusScreen(GUI_MODE_INVENTORY, INVENTORY_MODE_ITEM);
						}
						else
						{
							shootmode = true;
							identifygui_active = false;
							selectedIdentifySlot = -1;
							closeRemoveCurseGUI();
						}

						//What even is this code? When should it be run?
						if ( shopkeeper != 0 )
						{
							if ( localPlayerNetworkType != CLIENT )
							{
								Entity* entity = uidToEntity(shopkeeper);
								entity->skill[0] = 0;
								if ( uidToEntity(entity->skill[1]) )
								{
									monsterMoveAside(entity, uidToEntity(entity->skill[1]));
								}
								entity->skill[1] = 0;
							}
							else
							{
								// inform server that we're done talking to shopkeeper
								strcpy((char*)net_packet->data, "SHPC");
								SDLNet_Write32((Uint32)shopkeeper, &net_packet->data[4]);
								net_packet->address.host = net_server.host;
								net_packet->address.port = net_server.port;
								net_packet->len = 8;
								sendPacketSafe(net_sock, -1, net_packet, 0);
								list_FreeAll(shopInv);
							}

							shopkeeper = 0;

							//Clean up shopkeeper gamepad code here.
							selectedShopSlot = -1;
						}
						if ( shootmode == false )
						{
						}
						else
						{
							if (openedChest[clientnum])
							{
								openedChest[clientnum]->closeChest();
							}
							gui_mode = GUI_MODE_NONE;
						}
					}
					if (!command && (*inputPressed(impulses[IN_SPELL_LIST]) || *inputPressed(joyimpulses[INJOY_SPELL_LIST])))   //TODO: Move to function in interface or something?
					{
						*inputPressed(impulses[IN_SPELL_LIST]) = 0;
						*inputPressed(joyimpulses[INJOY_SPELL_LIST]) = 0;
						gui_mode = GUI_MODE_INVENTORY;
						selectedItem = NULL;
						inventory_mode = INVENTORY_MODE_SPELL;

						if (shootmode)
						{
							shootmode = false;
							attributespage = 0;
						}
					}
					if (!command && (*inputPressed(impulses[IN_CAST_SPELL]) || (shootmode && *inputPressed(joyimpulses[INJOY_GAME_CAST_SPELL]))))
					{
						*inputPressed(impulses[IN_CAST_SPELL]) = 0;
						if ( shootmode )
						{
							*inputPressed(joyimpulses[INJOY_GAME_CAST_SPELL]) = 0;
						}
						if (players[clientnum] && players[clientnum]->entity)
						{
							castSpellInit(players[clientnum]->entity->getUID(), selected_spell);
						}
					}

					// commands
					if ( ( *inputPressed(impulses[IN_CHAT]) || *inputPressed(impulses[IN_COMMAND]) ) && !command )
					{
						*inputPressed(impulses[IN_CHAT]) = 0;
						cursorflash = ticks;
						command = true;
						if ( !(*inputPressed(impulses[IN_COMMAND])) )
						{
							strcpy(command_str, "");
						}
						else
						{
							strcpy(command_str, "/");
						}
						inputstr = command_str;
						*inputPressed(impulses[IN_COMMAND]) = 0;
						SDL_StartTextInput();
					}
					if ( command )
					{
						if ( !SDL_IsTextInputActive() )
						{
							SDL_StartTextInput();
							inputstr = command_str;
						}
						//strncpy(command_str,inputstr,127);
						inputlen = 127;
						if ( keystatus[SDL_SCANCODE_ESCAPE] )   // escape
						{
							keystatus[SDL_SCANCODE_ESCAPE] = 0;
							chosen_command = NULL;
							command = 0;
						}
						if ( keystatus[SDL_SCANCODE_RETURN] )   // enter
						{
							keystatus[SDL_SCANCODE_RETURN] = 0;
							command = false;
							if ( localPlayerNetworkType != CLIENT )
							{
								if ( command_str[0] == '/' )
								{
									// backslash invokes command procedure
									messagePlayer(clientnum, command_str);
									consoleCommand(command_str);
								}
								else
								{
									if (strcmp(command_str, ""))
									{
										char chatstring[256];
										strcpy(chatstring, language[739]);
										strcat(chatstring, command_str);
										Uint32 color = SDL_MapRGBA(mainsurface->format, 0, 255, 255, 255);
										messagePlayerColor(clientnum, color, chatstring);
										playSound(238, 64);
										if ( localPlayerNetworkType == SERVER )
										{
											// send message to all clients
											for ( c = 1; c < MAXPLAYERS; c++ )
											{
												if ( client_disconnected[c] )
												{
													continue;
												}
												strcpy((char*)net_packet->data, "MSGS");
												strncpy(chatstring, stats[0]->name, std::min<size_t>(strlen(stats[0]->name), 10)); //TODO: Why are size_t and int being compared?
												chatstring[std::min<size_t>(strlen(stats[0]->name), 10)] = 0; //TODO: Why are size_t and int being compared?
												strcat(chatstring, ": ");
												strcat(chatstring, command_str);
												SDLNet_Write32(color, &net_packet->data[4]);
												strcpy((char*)(&net_packet->data[8]), chatstring);
												net_packet->address.host = net_clients[c - 1].host;
												net_packet->address.port = net_clients[c - 1].port;
												net_packet->len = 8 + strlen(chatstring) + 1;
												sendPacketSafe(net_sock, -1, net_packet, c - 1);
											}
										}
									}
									else
									{
										strcpy(command_str, "");
									}
								}
							}
							else
							{
								if ( command_str[0] == '/' )
								{
									// backslash invokes command procedure
									messagePlayer(clientnum, command_str);
									consoleCommand(command_str);
								}
								else
								{
									if (strcmp(command_str, ""))
									{
										char chatstring[256];
										strcpy(chatstring, language[739]);
										strcat(chatstring, command_str);
										Uint32 color = SDL_MapRGBA(mainsurface->format, 0, 255, 255, 255);
										messagePlayerColor(clientnum, color, chatstring);
										playSound(238, 64);

										// send message to server
										strcpy((char*)net_packet->data, "MSGS");
										net_packet->data[4] = clientnum;
										SDLNet_Write32(color, &net_packet->data[5]);
										strcpy((char*)(&net_packet->data[9]), command_str);
										net_packet->address.host = net_server.host;
										net_packet->address.port = net_server.port;
										net_packet->len = 9 + strlen(command_str) + 1;
										sendPacketSafe(net_sock, -1, net_packet, 0);
									}
									else
									{
										strcpy(command_str, "");
									}
								}
							}
							//In either case, save this in the command history.
							if (strcmp(command_str, ""))
							{
								saveCommand(command_str);
							}
							chosen_command = NULL;
						}
						ttfPrintTextFormatted(ttf16, MESSAGE_X_OFFSET, MESSAGE_Y_OFFSET, ">%s", command_str);
						if ( (ticks - cursorflash) % TICKS_PER_SECOND < TICKS_PER_SECOND / 2 )
						{
							int x;
							TTF_SizeUTF8(ttf16, command_str, &x, NULL);
							ttfPrintTextFormatted(ttf16, MESSAGE_X_OFFSET + x + TTF16_WIDTH, MESSAGE_Y_OFFSET, "_");
						}
					}
					else
					{
						if ( SDL_IsTextInputActive() )
						{
							SDL_StopTextInput();
						}
					}

					// other status
					if ( shootmode == false )
					{
						SDL_SetRelativeMouseMode(SDL_FALSE);
					}
					else
					{
						//Do these get called every frame? Might be better to move this stuff into an if (went_back_into_shootmode) { ... } thing.
						//2-3 years later...yes, it is run every frame.
						if (identifygui_appraising)
						{
							//Close the identify GUI if appraising.
							identifygui_active = false;
							identifygui_appraising = false;

							//Cleanup identify GUI gamecontroller code here.
							selectedIdentifySlot = -1;
						}

						if ( removecursegui_active )
						{
							closeRemoveCurseGUI();
						}

						if ( book_open )
						{
							closeBookGUI();
						}

						gui_clickdrag = false; //Just a catchall to make sure that any ongoing GUI dragging ends when the GUI is closed.

						if (capture_mouse)
						{
							SDL_SetRelativeMouseMode(SDL_TRUE);
						}
					}
					if ( !nohud )
					{
						drawMinimap();
					}

					drawSustainedSpells();
					updateAppraisalItemBox();

					// inventory and stats
					if ( shootmode == false )
					{
						if (gui_mode == GUI_MODE_INVENTORY)
						{
							updateCharacterSheet();
							updatePlayerInventory();
							updateChestInventory();
							updateIdentifyGUI();
							updateRemoveCurseGUI();
							updateBookGUI();
							//updateRightSidebar();

							Uint32 sec = (completionTime / TICKS_PER_SECOND) % 60;
							Uint32 min = ((completionTime / TICKS_PER_SECOND) / 60) % 60;
							Uint32 hour = ((completionTime / TICKS_PER_SECOND) / 60) / 60;
							printTextFormatted(font12x12_bmp, xres - 12 * 9, 12, "%02d:%02d:%02d", hour, min, sec);
						}
						else if (gui_mode == GUI_MODE_MAGIC)
						{
							updateCharacterSheet();
							updateMagicGUI();
						}
						else if (gui_mode == GUI_MODE_SHOP)
						{
							updateCharacterSheet();
							updatePlayerInventory();
							updateShopWindow();
						}
					}

					// pointer in inventory screen
					if (shootmode == false)
					{
						if (selectedItem)
						{
							pos.x = mousex - 15;
							pos.y = mousey - 15;
							pos.w = 32;
							pos.h = 32;
							drawImageScaled(itemSprite(selectedItem), NULL, &pos);
							if ( selectedItem->count > 1 )
							{
								ttfPrintTextFormatted(ttf8, pos.x + 24, pos.y + 24, "%d", selectedItem->count);
							}
							if ( itemCategory(selectedItem) != SPELL_CAT )
							{
								if ( itemIsEquipped(selectedItem, clientnum) )
								{
									pos.y += 16;
									drawImage(equipped_bmp, NULL, &pos);
								}
							}
							else
							{
								spell_t* spell = getSpellFromItem(selectedItem);
								if ( selected_spell == spell )
								{
									pos.y += 16;
									drawImage(equipped_bmp, NULL, &pos);
								}
							}
						}
						else if (draw_cursor)
						{
							pos.x = mousex - cursor_bmp->w / 2;
							pos.y = mousey - cursor_bmp->h / 2;
							pos.w = 0;
							pos.h = 0;
							drawImageAlpha(cursor_bmp, NULL, &pos, 192);
						}
					}
					else if ( !nohud )
					{
						pos.x = xres / 2 - cross_bmp->w / 2;
						pos.y = yres / 2 - cross_bmp->h / 2;
						pos.w = 0;
						pos.h = 0;
						drawImageAlpha(cross_bmp, NULL, &pos, 128);
					}
				}
				else if ( !localPlayerNetworkType )
				{
					// darken the rest of the screen
					src.x = 0;
					src.y = 0;
					src.w = mainsurface->w;
					src.h = mainsurface->h;
					drawRect(&src, SDL_MapRGB(mainsurface->format, 0, 0, 0), 127);
				}

				if ( gamePaused )
				{
					// handle menu
					handleMainMenu(intro);
				}
				else
				{
					// draw subwindow
					if ( !movie )
					{
						if ( subwindow )
						{
							drawWindowFancy(subx1, suby1, subx2, suby2);
							if ( subtext != NULL )
							{
								if ( strncmp(subtext, language[1133], 12) )
								{
									ttfPrintTextFormatted(ttf12, subx1 + 8, suby1 + 8, subtext);
								}
								else
								{
									ttfPrintTextFormatted(ttf16, subx1 + 8, suby1 + 8, subtext);
								}
							}
						}

						// process button actions
						handleButtons();
					}
				}

				if (((subwindow && !shootmode) || gamePaused) && draw_cursor)
				{
					pos.x = mousex - cursor_bmp->w / 2;
					pos.y = mousey - cursor_bmp->h / 2;
					pos.w = 0;
					pos.h = 0;
					drawImageAlpha(cursor_bmp, NULL, &pos, 192);
				}
			}

			// fade in/out effect
			if ( fadealpha > 0 )
			{
				src.x = 0;
				src.y = 0;
				src.w = mainsurface->w;
				src.h = mainsurface->h;
				drawRect(&src, SDL_MapRGB(mainsurface->format, 0, 0, 0), fadealpha);
			}

			// fps counter
			if ( showfps )
			{
				printTextFormatted(font8x8_bmp, 8, 8, "fps = %3.1f", fps);
			}

			// update screen
			GO_SwapBuffers(screen);

			// screenshots
			if ( keystatus[SDL_SCANCODE_F6] )
			{
				keystatus[SDL_SCANCODE_F6] = 0;
				takeScreenshot();
			}

			// frame rate limiter
			while ( frameRateLimit(MAX_FPS_LIMIT) )
			{
				if ( !intro )
				{
					// handle network messages
					if ( localPlayerNetworkType == CLIENT )
					{
						clientHandleMessages();
					}
					else if ( localPlayerNetworkType == SERVER )
					{
						serverHandleMessages();
					}
				}
			}

			// increase the cycle count
			cycles++;
		}
		saveConfig("default.cfg");

		// deinit
		deinitGame();
		return deinitApp();
	}
	catch (...)
	{
		//TODO:
		return 1;
	}
}
