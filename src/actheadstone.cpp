/*-------------------------------------------------------------------------------

	BARONY
	File: actheadstone.cpp
	Desc: implements headstone code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "monster.hpp"
#include "sound.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

#define HEADSTONE_INIT my->skill[0]
#define HEADSTONE_GHOUL my->skill[1]
#define HEADSTONE_FIRED my->skill[3]
#define HEADSTONE_MESSAGE my->skill[4]
#define HEADSTONE_AMBIENCE my->skill[5]

void actHeadstone(Entity* my) {
	if ( my->flags[INVISIBLE] ) {
		if ( multiplayer != CLIENT ) {
			node_t* node;
			int goldbags = 0;
			for ( node = map.entities->first; node != NULL; node = node->next ) {
				Entity* entity = (Entity*)node->element;
				if ( entity->sprite == 130 ) { // gold bag
					goldbags++;
				}
			}
			if ( goldbags >= 11 ) {
				return;
			}
			my->flags[INVISIBLE] = FALSE;
			my->flags[PASSABLE] = FALSE;
			serverUpdateEntityFlag(my, INVISIBLE);
			serverUpdateEntityFlag(my, PASSABLE);
		} else {
			return;
		}
	}

	HEADSTONE_AMBIENCE--;
	if ( HEADSTONE_AMBIENCE <= 0 ) {
		HEADSTONE_AMBIENCE = TICKS_PER_SECOND * 30;
		playSoundEntityLocal( my, 149, 32 );
	}

	if ( multiplayer == CLIENT ) {
		return;
	}

	if ( !HEADSTONE_INIT ) {
		HEADSTONE_INIT = 1;
		HEADSTONE_MESSAGE = rand();
		HEADSTONE_GHOUL = (rand() % 4 == 0);
	}

	bool shouldspawn = FALSE;

	// rightclick message
	int i;
	if ( multiplayer != CLIENT ) {
		for (i = 0; i < MAXPLAYERS; i++) {
			if ( (i == 0 && selectedEntity == my) || (client_selected[i] == my) ) {
				if (inrange[i]) {
					messagePlayer(i, language[485 + HEADSTONE_MESSAGE % 17]);
					if ( HEADSTONE_GHOUL && !HEADSTONE_FIRED ) {
						shouldspawn = TRUE;
						Uint32 color = SDL_MapRGB(mainsurface->format, 255, 128, 0);
						messagePlayerColor(i, color, language[502]);
					}
				}
			}
		}
	}

	// received on signal
	if ( my->skill[28] == 2 || shouldspawn ) {
		if ( !HEADSTONE_FIRED ) {
			HEADSTONE_FIRED = 1;

			// make a ghoul
			Entity* monster = summonMonster(GHOUL, my->x, my->y);
			if ( monster ) {
				monster->z = 13;
			}
		}
	}
}
