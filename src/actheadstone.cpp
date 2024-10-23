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
#include "engine/audio/sound.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "prng.hpp"
#include "mod_tools.hpp"

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

void actHeadstone(Entity* my)
{
	if ( my->flags[INVISIBLE] )
	{
		if ( multiplayer != CLIENT )
		{
			node_t* node;
			int goldbags = 0;
			bool artifact = false;
			for ( node = map.entities->first; node != nullptr; node = node->next )
			{
				Entity* entity = (Entity*)node->element;
				if ( entity->sprite == 130 )   // gold bag
				{
					++goldbags;
				}
				if ( entity->sprite == 508 )
				{
					artifact = true;
				}
			}
			if ( goldbags >= 9 && artifact )
			{
				return;
			}
			my->flags[INVISIBLE] = false;
			my->flags[PASSABLE] = false;
			serverUpdateEntityFlag(my, INVISIBLE);
			serverUpdateEntityFlag(my, PASSABLE);
		}
		else
		{
			return;
		}
	}

#ifdef USE_FMOD
	if ( HEADSTONE_AMBIENCE == 0 )
	{
		HEADSTONE_AMBIENCE--;
		my->stopEntitySound();
		my->entity_sound = playSoundEntityLocal(my, 149, 32);
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
	HEADSTONE_AMBIENCE--;
	if ( HEADSTONE_AMBIENCE <= 0 )
	{
		HEADSTONE_AMBIENCE = TICKS_PER_SECOND * 30;
		playSoundEntityLocal( my, 149, 32 );
	}
#endif

	if ( multiplayer == CLIENT )
	{
		if ( !HEADSTONE_INIT )
		{
			HEADSTONE_INIT = 1;
			my->createWorldUITooltip();
		}
		return;
	}

	if ( !HEADSTONE_INIT )
	{
		auto& rng = my->entity_rng ? *my->entity_rng : local_rng;
		my->createWorldUITooltip();
		HEADSTONE_INIT = 1;
		HEADSTONE_MESSAGE = rng.rand();
		HEADSTONE_GHOUL = (rng.rand() % 4 == 0);
	}

	bool shouldspawn = false;

	// rightclick message
	int triggeredPlayer = -1;
	if ( multiplayer != CLIENT )
	{
		for (int i = 0; i < MAXPLAYERS; i++)
		{
			if ( selectedEntity[i] == my || client_selected[i] == my )
			{
				if (inrange[i])
				{
					//messagePlayer(i, MESSAGE_INTERACTION, Language::get(485 + HEADSTONE_MESSAGE % 17));
					players[i]->worldUI.worldTooltipDialogue.createDialogueTooltip(my->getUID(),
						Player::WorldUI_t::WorldTooltipDialogue_t::DIALOGUE_GRAVE,
						Language::get(485 + HEADSTONE_MESSAGE % 17));

					Compendium_t::Events_t::eventUpdateWorld(i, Compendium_t::CPDM_GRAVE_EPITAPHS_READ, "gravestone", 1);
					Compendium_t::Events_t::eventUpdateWorld(i, Compendium_t::CPDM_GRAVE_EPITAPHS_PERCENT, "gravestone", (1 << (HEADSTONE_MESSAGE % 17)));

					triggeredPlayer = i;

					if ( HEADSTONE_GHOUL && !HEADSTONE_FIRED )
					{
						shouldspawn = true;
						Uint32 color = makeColorRGB(255, 128, 0);
						messagePlayerColor(i, MESSAGE_INTERACTION, color, Language::get(502));
					}
				}
			}
		}
	}

	// received on signal
	if ( my->skill[28] == 2 || shouldspawn )
	{
		if ( !HEADSTONE_FIRED )
		{
			HEADSTONE_FIRED = 1;

			// make a ghoul
			Entity* monster = summonMonsterNoSmoke(GHOUL, my->x, my->y, false);
			if ( monster )
			{
				auto& rng = my->entity_rng ? *my->entity_rng : local_rng;
				monster->seedEntityRNG(rng.getU32());
				monster->z = 13;
				if ( currentlevel >= 15 || !strncmp(map.name, "The Haunted Castle", 18) )
				{
					Stat* tmpStats = monster->getStats();
					if ( tmpStats )
					{
						strcpy(tmpStats->name, "enslaved ghoul");
						tmpStats->setAttribute("special_npc", "enslaved ghoul");
					}
					if ( triggeredPlayer >= 0 )
					{
						Compendium_t::Events_t::eventUpdateWorld(triggeredPlayer, Compendium_t::CPDM_GRAVE_GHOULS_ENSLAVED, "gravestone", 1);
					}
				}
				else
				{
					if ( triggeredPlayer >= 0 )
					{
						Compendium_t::Events_t::eventUpdateWorld(triggeredPlayer, Compendium_t::CPDM_GRAVE_GHOULS, "gravestone", 1);
					}
				}
			}
		}
	}
}
