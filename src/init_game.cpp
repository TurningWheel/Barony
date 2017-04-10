/*-------------------------------------------------------------------------------

	BARONY
	File: init_game.cpp
	Desc: contains game specific initialization code that shouldn't be
	seen in the editor.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "interface/interface.hpp"
#include "messages.hpp"
#include "book.hpp"
#include "sound.hpp"
#include "shops.hpp"
#include "scores.hpp"
#include "magic/magic.hpp"
#include "monster.hpp"
#include "net.hpp"
#ifdef STEAMWORKS
#include <steam/steam_api.h>
#include "steam.hpp"
#endif
#include "menu.hpp"
#include "paths.hpp"
#include "player.hpp"
#include "cppfuncs.hpp"

/*-------------------------------------------------------------------------------

	initGame

	initializes certain game specific resources

-------------------------------------------------------------------------------*/

#define _LOADSTR1 language[746]
#define _LOADSTR2 language[747]
#define _LOADSTR3 language[748]
#define _LOADSTR4 language[749]

int initGame()
{
	int c, x;
	char name[32];
	FILE* fp;

	// setup some lists
	steamAchievements.first = NULL;
	steamAchievements.last = NULL;
	booksRead.first = NULL;
	booksRead.last = NULL;
	lobbyChatboxMessages.first = NULL;
	lobbyChatboxMessages.last = NULL;

	// steam stuff
#ifdef STEAMWORKS
	cpp_SteamServerWrapper_Instantiate(); //TODO: Remove these wrappers.
	cpp_SteamServerClientWrapper_Instantiate();

	cpp_SteamServerClientWrapper_OnP2PSessionRequest = &steam_OnP2PSessionRequest;
	//cpp_SteamServerClientWrapper_OnGameOverlayActivated = &steam_OnGameOverlayActivated;
	cpp_SteamServerClientWrapper_OnLobbyCreated = &steam_OnLobbyCreated;
	cpp_SteamServerClientWrapper_OnGameJoinRequested = &steam_OnGameJoinRequested;
	cpp_SteamServerClientWrapper_OnLobbyEntered = &steam_OnLobbyEntered;
	cpp_SteamServerClientWrapper_GameServerPingOnServerResponded = &steam_GameServerPingOnServerResponded;
	cpp_SteamServerClientWrapper_OnLobbyMatchListCallback = &steam_OnLobbyMatchListCallback;
	cpp_SteamServerClientWrapper_OnP2PSessionConnectFail = &steam_OnP2PSessionConnectFail;
	cpp_SteamServerClientWrapper_OnLobbyDataUpdate = &steam_OnLobbyDataUpdatedCallback;
#endif

	// print a loading message
	drawClearBuffers();
	int w, h;
	TTF_SizeUTF8(ttf16, _LOADSTR1, &w, &h);
	ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, _LOADSTR1);

	GO_SwapBuffers(screen);

	initGameControllers();

	// load model offsets
	printlog( "loading model offsets...\n");
	for ( c = 1; c < NUMMONSTERS; c++ )
	{
		// initialize all offsets to zero
		for ( x = 0; x < 20; x++ )
		{
			limbs[c][x][0] = 0;
			limbs[c][x][1] = 0;
			limbs[c][x][2] = 0;
		}

		// open file
		char filename[256];
		strcpy(filename, "models/creatures/");
		strcat(filename, monstertypename[c]);
		strcat(filename, "/limbs.txt");
		if ( (fp = fopen(filename, "r")) == NULL )
		{
			continue;
		}

		// read file
		int line;
		for ( line = 1; feof(fp) == 0; line++ )
		{
			char data[256];
			int limb = 20;
			int dummy;

			// read line from file
			fgets( data, 256, fp );

			// skip blank and comment lines
			if ( data[0] == '\n' || data[0] == '\r' || data[0] == '#' )
			{
				continue;
			}

			// process line
			if ( sscanf( data, "%d", &limb ) != 1 || limb >= 20 || limb < 0 )
			{
				printlog( "warning: syntax error in '%s':%d\n invalid limb index!\n", filename, line);
				continue;
			}
			if ( sscanf( data, "%d %f %f %f\n", &dummy, &limbs[c][limb][0], &limbs[c][limb][1], &limbs[c][limb][2] ) != 4 )
			{
				printlog( "warning: syntax error in '%s':%d\n invalid limb offsets!\n", filename, line);
				continue;
			}
		}

		// close file
		fclose(fp);
	}

	// print a loading message
	drawClearBuffers();
	TTF_SizeUTF8(ttf16, _LOADSTR2, &w, &h);
	ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, _LOADSTR2);

	GO_SwapBuffers(screen);

	// load item types
	printlog( "loading items...\n");
	fp = fopen("items/items.txt", "r");
	for ( c = 0; !feof(fp); c++ )
	{
		items[c].name_identified = language[1545 + c * 2];
		items[c].name_unidentified = language[1546 + c * 2];
		fscanf(fp, "%d", &items[c].index);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
			{
				break;
			}
		fscanf(fp, "%d", &items[c].fpindex);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
			{
				break;
			}
		fscanf(fp, "%d", &items[c].variations);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
			{
				break;
			}
		fscanf(fp, "%s", name);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
			{
				break;
			}
		if ( !strcmp(name, "WEAPON") )
		{
			items[c].category = WEAPON;
		}
		else if ( !strcmp(name, "ARMOR") )
		{
			items[c].category = ARMOR;
		}
		else if ( !strcmp(name, "AMULET") )
		{
			items[c].category = AMULET;
		}
		else if ( !strcmp(name, "POTION") )
		{
			items[c].category = POTION;
		}
		else if ( !strcmp(name, "SCROLL") )
		{
			items[c].category = SCROLL;
		}
		else if ( !strcmp(name, "MAGICSTAFF") )
		{
			items[c].category = MAGICSTAFF;
		}
		else if ( !strcmp(name, "RING") )
		{
			items[c].category = RING;
		}
		else if ( !strcmp(name, "SPELLBOOK") )
		{
			items[c].category = SPELLBOOK;
		}
		else if ( !strcmp(name, "TOOL") )
		{
			items[c].category = TOOL;
		}
		else if ( !strcmp(name, "FOOD") )
		{
			items[c].category = FOOD;
		}
		else if ( !strcmp(name, "BOOK") )
		{
			items[c].category = BOOK;
		}
		else if ( !strcmp(name, "SPELL_CAT") )
		{
			items[c].category = SPELL_CAT;
		}
		else
		{
			items[c].category = GEM;
		}
		fscanf(fp, "%d", &items[c].weight);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
			{
				break;
			}
		fscanf(fp, "%d", &items[c].value);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
			{
				break;
			}
		items[c].images.first = NULL;
		items[c].images.last = NULL;
		while ( 1 )
		{
			string_t* string = (string_t*) malloc(sizeof(string_t));
			string->data = (char*) malloc(sizeof(char) * 64);
			string->lines = 1;

			node_t* node = list_AddNodeLast(&items[c].images);
			node->element = string;
			node->deconstructor = &stringDeconstructor;
			node->size = sizeof(string_t);
			string->node = node;

			x = 0;
			bool fileend = false;
			while ( (string->data[x] = fgetc(fp)) != '\n' )
			{
				if ( feof(fp) )
				{
					fileend = true;
					break;
				}
				x++;
			}
			if ( x == 0 || fileend )
			{
				list_RemoveNode(node);
				break;
			}
			string->data[x] = 0;
		}
	}
	for ( c = 0; c < NUMITEMS; c++ )
	{
		items[c].surfaces.first = NULL;
		items[c].surfaces.last = NULL;
		for ( x = 0; x < list_Size(&items[c].images); x++ )
		{
			SDL_Surface** surface = (SDL_Surface**) malloc(sizeof(SDL_Surface*));
			node_t* node = list_AddNodeLast(&items[c].surfaces);
			node->element = surface;
			node->deconstructor = &defaultDeconstructor;
			node->size = sizeof(SDL_Surface*);

			node_t* node2 = list_Node(&items[c].images, x);
			string_t* string = (string_t*)node2->element;
			*surface = loadImage(string->data);
		}
	}
	fclose(fp);

	createBooks();
	setupSpells();

	randomPlayerNames = getLinesFromFile(PLAYERNAMES_FILE);

	// print a loading message
	drawClearBuffers();
	TTF_SizeUTF8(ttf16, _LOADSTR3, &w, &h);
	ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, _LOADSTR3);

	GO_SwapBuffers(screen);

#ifdef HAVE_FMOD
	FMOD_ChannelGroup_SetVolume(music_group, musvolume / 128.f);
#elif defined HAVE_OPENAL
	OPENAL_ChannelGroup_SetVolume(music_group, musvolume / 128.f);
#endif
	removedEntities.first = NULL;
	removedEntities.last = NULL;
	safePacketsSent.first = NULL;
	safePacketsSent.last = NULL;
	for ( c = 0; c < MAXPLAYERS; c++ )
	{
		safePacketsReceived[c].first = NULL;
		safePacketsReceived[c].last = NULL;
	}
	topscores.first = NULL;
	topscores.last = NULL;
	messages.first = NULL;
	messages.last = NULL;
	chestInv.first = NULL;
	chestInv.last = NULL;
	command_history.first = NULL;
	command_history.last = NULL;
	for ( c = 0; c < 4; c++ )
	{
		invitems[c] = NULL;
		invitemschest[c] = NULL;
		openedChest[c] = NULL;
	}
	mousex = xres / 2;
	mousey = yres / 2;

	players = new Player*[MAXPLAYERS];
	// default player stats
	for (c = 0; c < MAXPLAYERS; c++)
	{
		players[c] = new Player();
		stats[c] = new Stat();
		if (c > 0)
		{
			client_disconnected[c] = true;
		}
		players[c]->entity = nullptr;
		stats[c]->sex = static_cast<sex_t>(0);
		stats[c]->appearance = 0;
		strcpy(stats[c]->name, "");
		stats[c]->type = HUMAN;
		stats[c]->FOLLOWERS.first = nullptr;
		stats[c]->FOLLOWERS.last = nullptr;
		stats[c]->inventory.first = nullptr;
		stats[c]->inventory.last = nullptr;
		stats[c]->clearStats();
		entitiesToDelete[c].first = nullptr;
		entitiesToDelete[c].last = nullptr;
		if (c == 0)
		{
			initClass(c);
		}
	}

	// load music
#ifdef SOUND
#ifdef HAVE_OPENAL
#define FMOD_ChannelGroup_SetVolume OPENAL_ChannelGroup_SetVolume
#define fmod_system 0
#define FMOD_SOFTWARE 0
#define FMOD_System_CreateStream(A, B, C, D, E) OPENAL_CreateStreamSound(B, E)
#define FMOD_SOUND OPENAL_BUFFER
int fmod_result;
#endif

	FMOD_ChannelGroup_SetVolume(music_group, musvolume / 128.f);
	fmod_result = FMOD_System_CreateStream(fmod_system, "music/intro.ogg", FMOD_SOFTWARE, NULL, &intromusic);
	fmod_result = FMOD_System_CreateStream(fmod_system, "music/introduction.ogg", FMOD_SOFTWARE, NULL, &introductionmusic);
	fmod_result = FMOD_System_CreateStream(fmod_system, "music/intermission.ogg", FMOD_SOFTWARE, NULL, &intermissionmusic);
	fmod_result = FMOD_System_CreateStream(fmod_system, "music/minetown.ogg", FMOD_SOFTWARE, NULL, &minetownmusic);
	fmod_result = FMOD_System_CreateStream(fmod_system, "music/splash.ogg", FMOD_SOFTWARE, NULL, &splashmusic);
	fmod_result = FMOD_System_CreateStream(fmod_system, "music/library.ogg", FMOD_SOFTWARE, NULL, &librarymusic);
	fmod_result = FMOD_System_CreateStream(fmod_system, "music/shop.ogg", FMOD_SOFTWARE, NULL, &shopmusic);
	fmod_result = FMOD_System_CreateStream(fmod_system, "music/herxboss.ogg", FMOD_SOFTWARE, NULL, &herxmusic);
	fmod_result = FMOD_System_CreateStream(fmod_system, "music/temple.ogg", FMOD_SOFTWARE, NULL, &templemusic);
	fmod_result = FMOD_System_CreateStream(fmod_system, "music/endgame.ogg", FMOD_SOFTWARE, NULL, &endgamemusic);
	fmod_result = FMOD_System_CreateStream(fmod_system, "music/escape.ogg", FMOD_SOFTWARE, NULL, &escapemusic);
	fmod_result = FMOD_System_CreateStream(fmod_system, "music/devil.ogg", FMOD_SOFTWARE, NULL, &devilmusic);
	//fmod_result = FMOD_System_CreateStream(fmod_system, "music/story.ogg", FMOD_SOFTWARE, NULL, &storymusic);

	if ( NUMMINESMUSIC > 0 )
	{
		minesmusic = (FMOD_SOUND**) malloc(sizeof(FMOD_SOUND*)*NUMMINESMUSIC);
		for ( c = 0; c < NUMMINESMUSIC; c++ )
		{
			snprintf(tempstr, 1000, "music/mines%02d.ogg", c);
			fmod_result = FMOD_System_CreateStream(fmod_system, tempstr, FMOD_SOFTWARE, NULL, &minesmusic[c]);
		}
	}
	if ( NUMSWAMPMUSIC > 0 )
	{
		swampmusic = (FMOD_SOUND**) malloc(sizeof(FMOD_SOUND*)*NUMSWAMPMUSIC);
		for ( c = 0; c < NUMSWAMPMUSIC; c++ )
		{
			snprintf(tempstr, 1000, "music/swamp%02d.ogg", c);
			fmod_result = FMOD_System_CreateStream(fmod_system, tempstr, FMOD_SOFTWARE, NULL, &swampmusic[c]);
		}
	}
	if ( NUMLABYRINTHMUSIC > 0 )
	{
		labyrinthmusic = (FMOD_SOUND**) malloc(sizeof(FMOD_SOUND*)*NUMLABYRINTHMUSIC);
		for ( c = 0; c < NUMLABYRINTHMUSIC; c++ )
		{
			snprintf(tempstr, 1000, "music/labyrinth%02d.ogg", c);
			fmod_result = FMOD_System_CreateStream(fmod_system, tempstr, FMOD_SOFTWARE, NULL, &labyrinthmusic[c]);
		}
	}
	if ( NUMRUINSMUSIC > 0 )
	{
		ruinsmusic = (FMOD_SOUND**) malloc(sizeof(FMOD_SOUND*)*NUMRUINSMUSIC);
		for ( c = 0; c < NUMRUINSMUSIC; c++ )
		{
			snprintf(tempstr, 1000, "music/ruins%02d.ogg", c);
			fmod_result = FMOD_System_CreateStream(fmod_system, tempstr, FMOD_SOFTWARE, NULL, &ruinsmusic[c]);
		}
	}
	if ( NUMUNDERWORLDMUSIC > 0 )
	{
		underworldmusic = (FMOD_SOUND**) malloc(sizeof(FMOD_SOUND*)*NUMUNDERWORLDMUSIC);
		for ( c = 0; c < NUMUNDERWORLDMUSIC; c++ )
		{
			snprintf(tempstr, 1000, "music/underworld%02d.ogg", c);
			fmod_result = FMOD_System_CreateStream(fmod_system, tempstr, FMOD_SOFTWARE, NULL, &underworldmusic[c]);
		}
	}
	if ( NUMHELLMUSIC > 0 )
	{
		hellmusic = (FMOD_SOUND**) malloc(sizeof(FMOD_SOUND*)*NUMHELLMUSIC);
		for ( c = 0; c < NUMHELLMUSIC; c++ )
		{
			snprintf(tempstr, 1000, "music/hell%02d.ogg", c);
			fmod_result = FMOD_System_CreateStream(fmod_system, tempstr, FMOD_SOFTWARE, NULL, &hellmusic[c]);
		}
	}
	if ( NUMMINOTAURMUSIC > 0 )
	{
		minotaurmusic = (FMOD_SOUND**) malloc(sizeof(FMOD_SOUND*)*NUMMINOTAURMUSIC);
		for ( c = 0; c < NUMMINOTAURMUSIC; c++ )
		{
			snprintf(tempstr, 1000, "music/minotaur%02d.ogg", c);
			fmod_result = FMOD_System_CreateStream(fmod_system, tempstr, FMOD_SOFTWARE, NULL, &minotaurmusic[c]);
		}
	}
#ifdef HAVE_OPENAL
#undef FMOD_ChannelGroup_SetVolume
#undef fmod_system
#undef FMOD_SOFTWARE
#undef FMOD_System_CreateStream
#undef FMOD_SOUND
#endif

#endif

	// print a loading message
	drawClearBuffers();
	TTF_SizeUTF8(ttf16, _LOADSTR4, &w, &h);
	ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, _LOADSTR4);

	GO_SwapBuffers(screen);

	// load extraneous game resources
	title_bmp = loadImage("images/system/title.png");
	logo_bmp = loadImage("images/system/logo.png");
	cursor_bmp = loadImage("images/system/cursor.png");
	cross_bmp = loadImage("images/system/cross.png");

	loadAllScores();
	if (!loadInterfaceResources())
	{
		printlog("Failed to load interface resources.\n");
		return -1;
	}

	return 0;
}

/*-------------------------------------------------------------------------------

	deinitGame

	deinitializes certain game specific resources

-------------------------------------------------------------------------------*/

void deinitGame()
{
	int c, x;

	// send disconnect messages
	if (multiplayer == CLIENT)
	{
		strcpy((char*)net_packet->data, "DISCONNECT");
		net_packet->data[10] = clientnum;
		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		net_packet->len = 11;
		sendPacketSafe(net_sock, -1, net_packet, 0);
		printlog("disconnected from server.\n");
	}
	else if (multiplayer == SERVER)
	{
		for (x = 1; x < MAXPLAYERS; x++)
		{
			if ( client_disconnected[x] == true )
			{
				continue;
			}
			strcpy((char*)net_packet->data, "DISCONNECT");
			net_packet->data[10] = clientnum;
			net_packet->address.host = net_clients[x - 1].host;
			net_packet->address.port = net_clients[x - 1].port;
			net_packet->len = 11;
			sendPacketSafe(net_sock, -1, net_packet, x - 1);

			stats[x]->freePlayerEquipment();
			client_disconnected[x] = true;
		}
	}

	// this short delay makes sure that the disconnect message gets out
	Uint32 timetoshutdown = SDL_GetTicks();
	while ( SDL_GetTicks() - timetoshutdown < 500 )
	{
		// handle network messages
		if ( multiplayer == CLIENT )
		{
			clientHandleMessages();
		}
		else if ( multiplayer == SERVER )
		{
			serverHandleMessages();
		}
		if ( !(SDL_GetTicks() % 25) && multiplayer )
		{
			int j = 0;
			node_t* node, *nextnode;
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
	}

	saveAllScores();
	list_FreeAll(&topscores);
	deleteAllNotificationMessages();
	list_FreeAll(&removedEntities);
	if (title_bmp != NULL)
	{
		SDL_FreeSurface(title_bmp);
	}
	if (logo_bmp != NULL)
	{
		SDL_FreeSurface(logo_bmp);
	}
	if (cursor_bmp != NULL)
	{
		SDL_FreeSurface(cursor_bmp);
	}
	if (cross_bmp != NULL)
	{
		SDL_FreeSurface(cross_bmp);
	}
	//if(sky_bmp!=NULL)
	//	SDL_FreeSurface(sky_bmp);
	list_FreeAll(&chestInv);
	freeInterfaceResources();
	if ( books )
	{
		for ( c = 0; c < numbooks; c++ )
		{
			if ( books[c] )
			{
				if ( books[c]->text )
				{
					free( books[c]->text );
				}
				if ( books[c]->bookgui_render_title )
				{
					free( books[c]->bookgui_render_title );
				}
				list_FreeAll( &books[c]->pages );
				free( books[c] );
			}
		}
		free( books );
	}
	if ( discoveredbooks )
	{
		list_FreeAll(discoveredbooks);
		free(discoveredbooks);
	}
	appraisal_timer = 0;
	appraisal_item = 0;
	for (c = 0; c < MAXPLAYERS; c++)
	{
		list_FreeAll(&stats[c]->inventory);
	}
	if ( multiplayer == CLIENT )
	{
		if ( shopInv )
		{
			list_FreeAll(shopInv);
			free(shopInv);
			shopInv = NULL;
		}
	}
	list_FreeAll(map.entities);
	list_FreeAll(&messages);
	if (multiplayer == SINGLE)
	{
		list_FreeAll(&channeledSpells[0]);
	}
	else if (multiplayer == CLIENT)
	{
		list_FreeAll(&channeledSpells[clientnum]);
	}
	else if (multiplayer == SERVER)
	{
		for (c = 0; c < numplayers; ++c)
		{
			list_FreeAll(&channeledSpells[c]);
		}
	}
	list_FreeAll(&spellList);
	list_FreeAll(&command_history);

	list_FreeAll(&safePacketsSent);
	for ( c = 0; c < MAXPLAYERS; c++ )
	{
		list_FreeAll(&safePacketsReceived[c]);
	}
#ifdef SOUND
#ifdef HAVE_OPENAL
#define FMOD_Channel_Stop OPENAL_Channel_Stop
#define FMOD_Sound_Release OPENAL_Sound_Release
#endif
	FMOD_Channel_Stop(music_channel);
	FMOD_Channel_Stop(music_channel2);
	FMOD_Sound_Release(intromusic);
	FMOD_Sound_Release(introductionmusic);
	FMOD_Sound_Release(intermissionmusic);
	FMOD_Sound_Release(minetownmusic);
	FMOD_Sound_Release(splashmusic);
	FMOD_Sound_Release(librarymusic);
	FMOD_Sound_Release(shopmusic);
	FMOD_Sound_Release(herxmusic);
	FMOD_Sound_Release(templemusic);
	FMOD_Sound_Release(endgamemusic);
	FMOD_Sound_Release(escapemusic);
	FMOD_Sound_Release(devilmusic);
	for ( c = 0; c < NUMMINESMUSIC; c++ )
	{
		FMOD_Sound_Release(minesmusic[c]);
	}
	if ( minesmusic )
	{
		free(minesmusic);
	}
	for ( c = 0; c < NUMSWAMPMUSIC; c++ )
	{
		FMOD_Sound_Release(swampmusic[c]);
	}
	if ( swampmusic )
	{
		free(swampmusic);
	}
	for ( c = 0; c < NUMLABYRINTHMUSIC; c++ )
	{
		FMOD_Sound_Release(labyrinthmusic[c]);
	}
	if ( labyrinthmusic )
	{
		free(labyrinthmusic);
	}
	for ( c = 0; c < NUMRUINSMUSIC; c++ )
	{
		FMOD_Sound_Release(ruinsmusic[c]);
	}
	if ( ruinsmusic )
	{
		free(ruinsmusic);
	}
	for ( c = 0; c < NUMUNDERWORLDMUSIC; c++ )
	{
		FMOD_Sound_Release(underworldmusic[c]);
	}
	if ( underworldmusic )
	{
		free(underworldmusic);
	}
	for ( c = 0; c < NUMHELLMUSIC; c++ )
	{
		FMOD_Sound_Release(hellmusic[c]);
	}
	if ( hellmusic )
	{
		free(hellmusic);
	}
	for ( c = 0; c < NUMMINOTAURMUSIC; c++ )
	{
		FMOD_Sound_Release(minotaurmusic[c]);
	}
	if ( minotaurmusic )
	{
		free(minotaurmusic);
	}
#ifdef HAVE_OPENAL
#undef FMOD_Channel_Stop
#undef FMOD_Sound_Release
#endif
#endif

	// free items
	printlog( "freeing item data...\n");
	for ( c = 0; c < NUMITEMS; c++ )
	{
		list_FreeAll(&items[c].images);
		node_t* node, *nextnode;
		for ( node = items[c].surfaces.first; node != NULL; node = nextnode )
		{
			nextnode = node->next;
			SDL_Surface** surface = (SDL_Surface**)node->element;
			if ( surface )
				if ( *surface )
				{
					SDL_FreeSurface(*surface);
				}
		}
		list_FreeAll(&items[c].surfaces);
	}

	// free spell data
	list_FreeAll(&spell_forcebolt.elements);
	list_FreeAll(&spell_magicmissile.elements);
	list_FreeAll(&spell_cold.elements);
	list_FreeAll(&spell_fireball.elements);
	list_FreeAll(&spell_lightning.elements);
	list_FreeAll(&spell_removecurse.elements);
	list_FreeAll(&spell_light.elements);
	list_FreeAll(&spell_identify.elements);
	list_FreeAll(&spell_magicmapping.elements);
	list_FreeAll(&spell_sleep.elements);
	list_FreeAll(&spell_confuse.elements);
	list_FreeAll(&spell_slow.elements);
	list_FreeAll(&spell_opening.elements);
	list_FreeAll(&spell_locking.elements);
	list_FreeAll(&spell_levitation.elements);
	list_FreeAll(&spell_invisibility.elements);
	list_FreeAll(&spell_teleportation.elements);
	list_FreeAll(&spell_healing.elements);
	list_FreeAll(&spell_extrahealing.elements);
	list_FreeAll(&spell_cureailment.elements);
	list_FreeAll(&spell_dig.elements);

	// pathmaps
	if ( pathMapGrounded )
	{
		free(pathMapGrounded);
	}
	pathMapGrounded = NULL;
	if ( pathMapFlying )
	{
		free(pathMapFlying);
	}
	pathMapFlying = NULL;

	// clear steam achievement list
	list_FreeAll(&steamAchievements);
	list_FreeAll(&booksRead);

	// clear lobby chatbox data
	list_FreeAll(&lobbyChatboxMessages);

	// steam stuff
#ifdef STEAMWORKS
	cpp_SteamServerWrapper_Destroy();
	cpp_SteamServerClientWrapper_Destroy();
	if ( currentLobby )
	{
		SteamMatchmaking()->LeaveLobby(*static_cast<CSteamID*>(currentLobby));
		cpp_Free_CSteamID(currentLobby); //TODO: Remove these bodges.
		currentLobby = NULL;
	}
	if ( lobbyToConnectTo )
	{
		cpp_Free_CSteamID(lobbyToConnectTo);
		lobbyToConnectTo = NULL;
	}
	for ( c = 0; c < MAXPLAYERS; c++ )
	{
		if ( steamIDRemote[c] )
		{
			cpp_Free_CSteamID(steamIDRemote[c]);
			steamIDRemote[c] = NULL;
		}
	}
	for ( c = 0; c < MAX_STEAM_LOBBIES; c++ )
	{
		if ( lobbyIDs[c] )
		{
			cpp_Free_CSteamID(lobbyIDs[c]);
			lobbyIDs[c] = NULL;
		}
	}
#endif

	//Close game controller
	/*if (game_controller)
	{
		SDL_GameControllerClose(game_controller);
		game_controller = nullptr;
	}*/
	if (game_controller)
	{
		delete game_controller;
	}

	for (int i = 0; i < MAXPLAYERS; ++i)
	{
		delete players[i];
	}
	delete[] players;
}
