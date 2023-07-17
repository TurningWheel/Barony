/*-------------------------------------------------------------------------------

	BARONY
	File: monster_slime.cpp
	Desc: implements all of the slime monster's code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "items.hpp"
#include "monster.hpp"
#include "engine/audio/sound.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "prng.hpp"
#include "interface/consolecommand.hpp"

void initSlime(Entity* my, Stat* myStats)
{
    my->flags[BURNABLE] = false;
	my->flags[UPDATENEEDED] = true;
	my->flags[INVISIBLE] = false;

    auto& slimeStand = my->skill[24];
    auto& slimeBob = my->fskill[24];
    slimeStand = 0;
    slimeBob = 0.0;

	const bool blue = myStats && myStats->LVL == 7;
	my->initMonster(blue ? 1108 : 1113);

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 68;
		MONSTER_SPOTVAR = 1;
		MONSTER_IDLESND = -1;
		MONSTER_IDLEVAR = 1;
	}
	if ( multiplayer != CLIENT && !MONSTER_INIT )
	{
		if ( myStats != NULL )
		{
			if ( !myStats->leader_uid )
			{
				myStats->leader_uid = 0;
			}

			// apply random stat increases if set in stat_shared.cpp or editor
			setRandomMonsterStats(myStats);

			// generate 6 items max, less if there are any forced items from boss variants
			int customItemsToGenerate = ITEM_CUSTOM_SLOT_LIMIT;

			// boss variants

			// random effects

			// generates equipment and weapons if available from editor
			createMonsterEquipment(myStats);

			// create any custom inventory items from editor if available
			createCustomInventory(myStats, customItemsToGenerate);

			// count if any custom inventory items from editor
			int customItems = countCustomItems(myStats); //max limit of 6 custom items per entity.

														 // count any inventory items set to default in edtior
			int defaultItems = countDefaultItems(myStats);

			my->setHardcoreStats(*myStats);

			// generate the default inventory items for the monster, provided the editor sprite allowed enough default slots
			switch ( defaultItems )
			{
				case 6:
				case 5:
				case 4:
				case 3:
				case 2:
				case 1:
					break;
				default:
					break;
			}
		}
	}
}

void slimeAnimate(Entity* my, double dist)
{
    const bool green = my->sprite == 210 || my->sprite >= 1113;
    const int frame = TICKS_PER_SECOND / 10;

    // idle & walk cycle
    if (!MONSTER_ATTACK) {
        auto& slimeStand = my->skill[24];
        auto& slimeBob = my->fskill[24];
        const real_t squishRate = dist < 0.1 ? 1.5 : 3.0;
        const real_t squishFactor = dist < 0.1 ? 0.05 : 0.3;
        const real_t inc = squishRate * (PI / TICKS_PER_SECOND);
        slimeBob = fmod(slimeBob + inc, PI * 2);
        my->scalex = 1.0 - sin(slimeBob) * squishFactor;
        my->scaley = 1.0 - sin(slimeBob) * squishFactor;
        my->scalez = 1.0 + sin(slimeBob) * squishFactor;
        if (dist < 0.1) {
	        if (slimeStand < frame) {
	            my->sprite = green ? 1113 : 1108;
	            my->focalz = -.5;
	            ++slimeStand;
	        } else {
	            my->sprite = green ? 1114 : 1109;
	            my->focalz = -1.5;
	        }
        } else {
	        if (slimeStand > 0) {
	            my->sprite = green ? 1113 : 1108;
	            my->focalz = -.5;
	            --slimeStand;
	        } else {
	            my->sprite = green ? 210 : 189;
	            my->focalz = 0.0;
	        }
        }
    }

    // attack cycle
	if (MONSTER_ATTACK) {
	    if (MONSTER_ATTACKTIME == frame * 0) { // frame 1
	        my->sprite = green ? 1113 : 1108;
	        my->scalex = 1.0;
	        my->scaley = 1.0;
	        my->scalez = 1.0;
	        my->focalz = -.5;
	    }
	    if (MONSTER_ATTACKTIME == frame * 2) { // frame 2
	        my->sprite = green ? 1114 : 1109;
	        my->focalz = -1.5;
	    }
	    if (MONSTER_ATTACKTIME == frame * 4) { // frame 3
	        my->sprite = green ? 1115 : 1110;
	        my->focalz = -3.0;
	    }
	    if (MONSTER_ATTACKTIME == frame * 5) { // frame 4
	        my->sprite = green ? 1116 : 1111;
	        my->focalz = -2.5;
	        const Sint32 temp = MONSTER_ATTACKTIME;
	        my->attack(1, 0, nullptr); // slop
	        MONSTER_ATTACKTIME = temp;
	    }
	    if (MONSTER_ATTACKTIME == frame * 7) { // frame 5
	        my->sprite = green ? 1117 : 1112;
	        my->focalz = 0.0;
	    }
	    if (MONSTER_ATTACKTIME == frame * 8) { // end
            my->sprite = green ? 210 : 189;
            my->focalz = 0.0;
	        MONSTER_ATTACK = 0;
	        MONSTER_ATTACKTIME = 0;
	    }
	    else {
		    ++MONSTER_ATTACKTIME;
        }
	}
}

void slimeDie(Entity* my)
{
	for ( int c = 0; c < 10; c++ )
	{
		Entity* gib = spawnGib(my);
		serverSpawnGibForClient(gib);
	}

    if ( my->sprite == 210 || my->sprite >= 1113 )
    {
        // green blood
	    my->spawnBlood(212);
    }
    else
    {
        // blue blood
	    my->spawnBlood(214);
    }

	playSoundEntity(my, 69, 64);

	my->removeMonsterDeathNodes();

	list_RemoveNode(my->mynode);
	return;
}
