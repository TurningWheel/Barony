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
#include "magic/magic.hpp"
#include "mod_tools.hpp"

int getSlimeFrame(std::string color, int frame)
{
	auto& data = MonsterData_t::monsterDataEntries[SLIME];
	auto find = data.keyToSpriteLookup.find(color);
	if ( find == data.keyToSpriteLookup.end() ) {
		if ( frame >= data.keyToSpriteLookup["slime green"].size() )
		{
			return 0;
		}
		return data.keyToSpriteLookup["slime green"][frame];
	}
	else
	{
		if ( frame >= find->second.size() )
		{
			return 0;
		}
		return find->second[frame];
	}
}

void slimeSetType(Entity* my, Stat* myStats, bool sink, BaronyRNG* rng)
{
	if ( !my || !myStats ) { return; }

	if ( gameModeManager.currentMode == GameModeManager_t::GAME_MODE_TUTORIAL
		|| gameModeManager.currentMode == GameModeManager_t::GAME_MODE_TUTORIAL_INIT )
	{
		myStats->setAttribute("slime_type", "slime green");
		return;
	}
	std::vector<std::pair<std::string, int>> possibleTypes = { {"slime green", 1} };
	if ( sink )
	{
		if ( currentlevel >= 5 )
		{
			if ( strcmp(map.name, "Hell") )
			{
				possibleTypes.push_back({ "slime blue", 2 });
			}
		}
		if ( currentlevel >= 10 )
		{
			possibleTypes.push_back({"slime red", 3});
		}
		if ( currentlevel >= 15 )
		{
			possibleTypes.push_back({"slime tar", 4});
		}
		if ( currentlevel >= 15 )
		{
			possibleTypes.push_back({"slime metal", 5});
		}
	}
	else
	{
		if ( currentlevel >= 5 )
		{
			if ( strcmp(map.name, "Hell") )
			{
				possibleTypes.push_back({"slime blue", 2});
			}
		}
		if ( currentlevel >= 10 )
		{
			possibleTypes.push_back({"slime red", 3});
		}
		if ( currentlevel >= 15 )
		{
			possibleTypes.push_back({"slime tar", 4});
		}
		if ( currentlevel >= 15 )
		{
			possibleTypes.push_back({"slime metal", 5});
		}
	}

	int x = my->x / 16;
	int y = my->y / 16;
	int mapIndex = (y)*MAPLAYERS + (x)*MAPLAYERS * map.height;
	if ( x > 0 && x < map.width && y > 0 && y < map.height )
	{
		if ( map.tiles[mapIndex] )
		{
			if ( lavatiles[map.tiles[mapIndex]] )
			{
				possibleTypes.clear();
				possibleTypes.push_back({ "slime red", 3 });
			}
			else if ( swimmingtiles[map.tiles[mapIndex]] )
			{
				possibleTypes.clear();
				possibleTypes.push_back({ "slime blue", 2 });
				if ( currentlevel >= 15 )
				{
					possibleTypes.push_back({ "slime tar", 4 });
				}
			}
		}
	}
	
	if ( currentlevel >= 20 )
	{
		for ( auto it = possibleTypes.begin(); it != possibleTypes.end(); )
		{
			if ( it->first == "slime green" || it->first == "slime blue" )
			{
				if ( possibleTypes.size() > 1 )
				{
					it = possibleTypes.erase(it);
				}
				else
				{
					++it;
				}
			}
			else
			{
				++it;
			}
		}
	}

	int roll = rng ? rng->rand() % possibleTypes.size() : local_rng.rand() % possibleTypes.size();
	myStats->setAttribute("slime_type", possibleTypes[roll].first);
}

void slimeSetStats(Entity& my, Stat& myStats)
{
	myStats.HP = 60;
	myStats.STR = 3;
	myStats.DEX = -4;
	myStats.CON = 3;
	myStats.PER = -2;
	myStats.LVL = 4;

	auto color = MonsterData_t::getKeyFromSprite(my.sprite, SLIME);
	int level = std::max(currentlevel, 0) / LENGTH_OF_LEVEL_REGION;
	myStats.LVL += 3 * level;
	myStats.STR += 3 * level;
	myStats.HP += 20 * level;
	if ( currentlevel >= 20 )
	{
		myStats.HP += 10 * level;
	}
	if ( color == "slime metal" )
	{
		myStats.CON = 20 + 5 * level;
		myStats.STR += 2 * level;
		myStats.DEX += 2 * level;
		myStats.DEX = std::min(myStats.DEX, 0);
	}
	else
	{
		myStats.CON += 3 * level;
		myStats.DEX += 2 * level;
		myStats.DEX = std::min(myStats.DEX, 4);
	}
	myStats.PER += 1 * level;

	myStats.MAXHP = myStats.HP;
	myStats.OLDHP = myStats.HP;

	myStats.setProficiency(PRO_MAGIC, level * 10);
}

void initSlime(Entity* my, Stat* myStats)
{
    my->flags[BURNABLE] = false;
	my->flags[UPDATENEEDED] = true;
	my->flags[INVISIBLE] = false;

    auto& slimeStand = my->skill[24];
    auto& slimeBob = my->fskill[24];
    slimeStand = 0;
    slimeBob = 0.0;

	if ( myStats && myStats->getAttribute("slime_type") != "" )
	{
		my->initMonster(getSlimeFrame(myStats->getAttribute("slime_type"), 1));
	}
	else
	{
		my->initMonster(getSlimeFrame("slime green", 1));
	}

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 68;
		MONSTER_SPOTVAR = 1;
		MONSTER_IDLESND = -1;
		MONSTER_IDLEVAR = 1;
	}
	if ( multiplayer != CLIENT && !MONSTER_INIT )
	{
		auto& rng = my->entity_rng ? *my->entity_rng : local_rng;

		if ( myStats != NULL )
		{
			if ( !myStats->leader_uid )
			{
				myStats->leader_uid = 0;
			}


			std::string color = "slime green";
			if ( myStats->getAttribute("slime_type") == "" )
			{
				slimeSetType(my, myStats, false, &rng);
			}
			
			if ( myStats->getAttribute("slime_type") != "" )
			{
				color = myStats->getAttribute("slime_type");
			}
			my->sprite = getSlimeFrame(color, 1);

			// set slime stats
			if ( isMonsterStatsDefault(*myStats) )
			{
				slimeSetStats(*my, *myStats);
			}

			if ( !strcmp(myStats->name, "") )
			{
				strcpy(myStats->name, getMonsterLocalizedName(SLIME).c_str());
			}

			// apply random stat increases if set in stat_shared.cpp or editor
			setRandomMonsterStats(myStats, rng);

			// generate 6 items max, less if there are any forced items from boss variants
			int customItemsToGenerate = ITEM_CUSTOM_SLOT_LIMIT;

			// boss variants

			// random effects

			// generates equipment and weapons if available from editor
			createMonsterEquipment(myStats, rng);

			// create any custom inventory items from editor if available
			createCustomInventory(myStats, customItemsToGenerate, rng);

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

const int slimeSprayDelayOffset = TICKS_PER_SECOND / 2;
const int slimeSprayDelay = TICKS_PER_SECOND + ((2 * TICKS_PER_SECOND) / 5) - slimeSprayDelayOffset;
void slimeSprayAttack(Entity* my)
{
	if ( !my ) { return; }

	auto color = MonsterData_t::getKeyFromSprite(my->sprite, SLIME);
	Entity* spellTimer = nullptr;
	if ( multiplayer == CLIENT )
	{
		int particle = 180;
		if ( color == "slime green" )
		{
			particle = 180;
		}
		else if ( color == "slime blue" )
		{
			particle = 181;
		}
		else if ( color == "slime red" )
		{
			particle = 182;
		}
		else if ( color == "slime tar" )
		{
			particle = 183;
		}
		else if ( color == "slime metal" )
		{
			particle = 184;
		}
		spellTimer = createParticleTimer(my, 30, -1);
		spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_MAGIC_SPRAY;
		spellTimer->particleTimerCountdownSprite = particle;
	}
	else
	{
		if ( color == "slime green" )
		{
			spellTimer = castSpell(my->getUID(), &spell_slime_acid, true, false);
		}
		else if ( color == "slime blue" )
		{
			spellTimer = castSpell(my->getUID(), &spell_slime_water, true, false);
		}
		else if ( color == "slime red" )
		{
			spellTimer = castSpell(my->getUID(), &spell_slime_fire, true, false);
		}
		else if ( color == "slime tar" )
		{
			spellTimer = castSpell(my->getUID(), &spell_slime_tar, true, false);
		}
		else if ( color == "slime metal" )
		{
			spellTimer = castSpell(my->getUID(), &spell_slime_metal, true, false);
		}
	}

	if ( spellTimer )
	{
		spellTimer->particleTimerPreDelay = slimeSprayDelay;
		spellTimer->particleTimerDuration += slimeSprayDelay;

		createParticleDot(my);

		// play casting sound
		playSoundEntityLocal(my, 170, 64);
		// monster scream
		playSoundEntityLocal(my, MONSTER_SPOTSND, 128);
	}
}

void slimeAnimate(Entity* my, Stat* myStats, double dist)
{
    //const bool green = my->sprite == 210 || my->sprite >= 1113;
	auto color = MonsterData_t::getKeyFromSprite(my->sprite, SLIME);

	if ( multiplayer == CLIENT )
	{
		if ( my->monsterSlimeLastAttack != 0 )
		{
			if ( MONSTER_ATTACK != my->monsterSlimeLastAttack )
			{
				MONSTER_ATTACKTIME = 0;
			}
		}
		my->monsterSlimeLastAttack = MONSTER_ATTACK;
	}

	my->z = 6;
	my->new_z = my->z;

    const int frame = TICKS_PER_SECOND / 10;

    auto& slimeStand = my->skill[24];
    auto& slimeBob = my->fskill[24];
	auto& slimeJump = my->fskill[25];
    // idle & walk cycle
    if (!MONSTER_ATTACK) {
        const real_t squishRate = dist < 0.1 ? 1.5 : 3.0;
        const real_t squishFactor = dist < 0.1 ? 0.05 : 0.3;
        const real_t inc = squishRate * (PI / TICKS_PER_SECOND);
        slimeBob = fmod(slimeBob + inc, PI * 2);
        my->scalex = 1.0 - sin(slimeBob) * squishFactor;
        my->scaley = 1.0 - sin(slimeBob) * squishFactor;
        my->scalez = 1.0 + sin(slimeBob) * squishFactor;
        if (dist < 0.1) {
	        if (slimeStand < frame) {
	            my->sprite = getSlimeFrame(color, 1);
	            my->focalz = -.5;
	            ++slimeStand;
	        } else {
	            my->sprite = getSlimeFrame(color, 2);
	            my->focalz = -1.5;
	        }
        } else {
	        if (slimeStand > 0) {
	            my->sprite = getSlimeFrame(color, 1);
	            my->focalz = -.5;
	            --slimeStand;
	        } else {
	            my->sprite = getSlimeFrame(color, 0);
	            my->focalz = 0.0;
	        }
        }
    }

	if ( MONSTER_ATTACK == MONSTER_POSE_MAGIC_WINDUP2 )
	{
		
	}
	else
	{
		if ( MONSTER_ATTACK )
		{
			my->scalex = 1.0;
			my->scaley = 1.0;
			my->scalez = 1.0;
		}
		if ( slimeJump > 0.0 ) {
			slimeJump -= 4 * PI / TICKS_PER_SECOND;
			if ( slimeJump <= 0.0 ) {
				slimeJump = 0.0;
			}
		}
	}

    // attack cycle
	if (MONSTER_ATTACK) {
		bool tickAttackTime = true;
		if ( MONSTER_ATTACK == MONSTER_POSE_MAGIC_WINDUP2 )
		{
			my->sprite = getSlimeFrame(color, 0);
			if ( MONSTER_ATTACKTIME == 0 )
			{
				my->scalex = 1.0;
				my->scaley = 1.0;
				my->scalez = 1.0;
				slimeBob = 0.0;
				if ( multiplayer != CLIENT )
				{
					if ( Stat* myStats = my->getStats() )
					{
						myStats->EFFECTS[EFF_STUNNED] = true;
						myStats->EFFECTS_TIMERS[EFF_STUNNED] = slimeSprayDelay / 2;
					}
				}
				slimeSprayAttack(my);
			}

			const real_t squishRate = 3.0;
			real_t squishFactor = 0.3;

			const int interval1 = (TICKS_PER_SECOND) - slimeSprayDelayOffset;
			const int interval2 = (TICKS_PER_SECOND) + 8 - slimeSprayDelayOffset;
			const int interval3 = 2 * TICKS_PER_SECOND - 10 - slimeSprayDelayOffset;
			const int interval4 = 2 * TICKS_PER_SECOND + 25 - slimeSprayDelayOffset;

			if ( MONSTER_ATTACKTIME < interval1 )
			{
				my->sprite = getSlimeFrame(color, 2);
				const real_t inc = squishRate * 3.0 * (PI / TICKS_PER_SECOND);
				squishFactor = 0.15;
				slimeBob = fmod(slimeBob + inc, PI * 2);
			}
			else if ( MONSTER_ATTACKTIME >= interval1 && MONSTER_ATTACKTIME < interval2 )
			{
				if ( MONSTER_ATTACKTIME == interval1 )
				{
					slimeBob = 0.0;
				}

				const real_t inc = (squishRate) * (PI / TICKS_PER_SECOND);
				slimeBob = std::min(slimeBob + inc, PI / 2);
				/*if ( my->ticks % 2 == 0 )
				{
					slimeBob += PI / 32;
				}*/
			}
			else if ( MONSTER_ATTACKTIME >= interval2 && MONSTER_ATTACKTIME < interval3 )
			{
				const real_t inc = (squishRate) * 2 * (PI / TICKS_PER_SECOND);
				slimeBob = fmod(slimeBob + inc, PI * 2);
				my->sprite = getSlimeFrame(color, 1);
			}
			else
			{
				const real_t inc = (squishRate) * (PI / TICKS_PER_SECOND);
				slimeBob = fmod(slimeBob + inc, PI * 2);
				my->sprite = getSlimeFrame(color, 1);
			}

			if ( MONSTER_ATTACKTIME >= interval3 )
			{
				if ( MONSTER_ATTACKTIME >= interval3 + 20 )
				{
					my->sprite = getSlimeFrame(color, 0);
					if ( slimeJump > 0.0 ) {
						slimeJump -= 4 * PI / TICKS_PER_SECOND;
						if ( slimeJump <= 0.0 ) {
							slimeJump = 0.0;
						}
					}
				}
				else
				{
					my->sprite = getSlimeFrame(color, 1);
				}
			}
			else if ( MONSTER_ATTACKTIME >= interval2 )
			{
				if ( slimeJump < PI / 2.0 ) {
					slimeJump += (PI / TICKS_PER_SECOND) * 2.0;
					if ( slimeJump >= PI / 2.0 ) {
						slimeJump = PI / 2.0;
					}
				}
			}

			my->scalex = 1.0 - sin(slimeBob) * squishFactor;
			my->scaley = 1.0 - sin(slimeBob) * squishFactor;
			my->scalez = 1.0 + sin(slimeBob) * squishFactor;
			if ( my->sprite == getSlimeFrame(color, 1) )
			{
				my->focalz = -.5;
			}
			else if ( my->sprite == getSlimeFrame(color, 2) )
			{
				my->focalz = -1.5;
			}
			else
			{
				my->focalz = 0.0;
			}

			if ( MONSTER_ATTACKTIME >= interval4 / (monsterGlobalAnimationMultiplier / 10.0) )
			{
				//if ( multiplayer != CLIENT )
				//{
				//	// swing the arm after we prepped the spell
				//	my->attack(1, 0, nullptr);
				//}
				MONSTER_ATTACK = 0;
				MONSTER_ATTACKTIME = 0;
			}
		}
		else
		{
			if (MONSTER_ATTACKTIME == frame * 0) { // frame 1
				my->sprite = getSlimeFrame(color, 1);
				my->scalex = 1.0;
				my->scaley = 1.0;
				my->scalez = 1.0;
				my->focalz = -.5;
			}
			else if (MONSTER_ATTACKTIME == frame * 2) { // frame 2
				my->sprite = getSlimeFrame(color, 2);
				my->focalz = -1.5;
			}
			else if (MONSTER_ATTACKTIME == frame * 4) { // frame 3
				my->sprite = getSlimeFrame(color, 3);
				my->focalz = -3.0;
			}
			else if (MONSTER_ATTACKTIME == frame * 5) { // frame 4
				my->sprite = getSlimeFrame(color, 4);
				my->focalz = -2.5;
				const Sint32 temp = MONSTER_ATTACKTIME;
				if ( multiplayer != CLIENT )
				{
					my->attack(1, 0, nullptr); // slop
				}
				MONSTER_ATTACKTIME = temp;
			}
			else if (MONSTER_ATTACKTIME == frame * 7) { // frame 5
				my->sprite = getSlimeFrame(color, 5);
				my->focalz = 0.0;
			}
			else if (MONSTER_ATTACKTIME == frame * 8) { // end
				my->sprite = getSlimeFrame(color, 0);
				my->focalz = 0.0;
				MONSTER_ATTACK = 0;
				MONSTER_ATTACKTIME = 0;
				tickAttackTime = false;
			}
		}

		if ( tickAttackTime )
	    {
		    ++MONSTER_ATTACKTIME;
        }
	}
	else
	{
		MONSTER_ATTACKTIME = 0;
	}

	if ( MONSTER_ATTACK != MONSTER_POSE_MAGIC_WINDUP2 )
	{
		// base focalz
		if ( my->sprite == getSlimeFrame(color, 0) )
		{
			my->focalz = 0.0;
		}
		else if ( my->sprite == getSlimeFrame(color, 1) )
		{
			my->focalz = -.5;
		}
		else if ( my->sprite == getSlimeFrame(color, 2) )
		{
			my->focalz = -1.5;
		}
		else if ( my->sprite == getSlimeFrame(color, 3) )
		{
			my->focalz = -3.0;
		}
		else if ( my->sprite == getSlimeFrame(color, 4) )
		{
			my->focalz = -2.5;
		}
		else if ( my->sprite == getSlimeFrame(color, 5) )
		{
			my->focalz = 0.0;
		}
	}

	my->focalz += -sin(slimeJump) * 6.0;
	
	auto& slimeWaterBob = my->fskill[23];
	bool swimming = false;
	if ( !isLevitating(myStats) )
	{
		int x = std::min(std::max<unsigned int>(0, floor(my->x / 16)), map.width - 1);
		int y = std::min(std::max<unsigned int>(0, floor(my->y / 16)), map.height - 1);
		int index = y * MAPLAYERS + x * MAPLAYERS * map.height;
		if ( map.tiles[index] )
		{
			if ( swimmingtiles[map.tiles[index]]
				|| lavatiles[map.tiles[index]] )
			{
				slimeWaterBob = sin(((ticks % (TICKS_PER_SECOND * 2)) / ((real_t)TICKS_PER_SECOND * 2.0)) * (2.0 * PI)) * 0.5;
				swimming = true;
			}
		}
	}
	else
	{
		slimeWaterBob = 0.0;
	}
	my->focalz += (swimming && MONSTER_ATTACK == 0) ? (1.0 + slimeWaterBob) : 0.0;
}

void slimeDie(Entity* my)
{
	for ( int c = 0; c < 10; c++ )
	{
		Entity* gib = spawnGib(my);
		serverSpawnGibForClient(gib);
	}

	auto color = MonsterData_t::getKeyFromSprite(my->sprite, SLIME);
    if ( color == "slime green" )
    {
        // green blood
	    my->spawnBlood(212);
    }
    else if ( color == "slime blue" )
    {
        // blue blood
	    my->spawnBlood(214);
    }
	else if ( color == "slime red" )
	{
		// todo blood
		my->spawnBlood(1398);
	}
	else if ( color == "slime tar" )
	{
		// todo blood
		my->spawnBlood(1399);
	}
	else if ( color == "slime metal" )
	{
		// todo blood
		my->spawnBlood(1400);
	}

	playSoundEntity(my, 69, 64);

	my->removeMonsterDeathNodes();

	list_RemoveNode(my->mynode);
	return;
}

void Entity::slimeChooseWeapon(const Entity* target, double dist)
{
	Stat* myStats = getStats();
	if ( !myStats )
	{
		return;
	}

	if ( monsterSpecialState != 0 && monsterSpecialTimer != 0 )
	{
		return;
	}

	if ( monsterSpecialTimer == 0 
		&& (ticks % 10 == 0) 
		&& (monsterAttack == 0 || ((monsterAttack == 1) && monsterAttackTime >= 25))
		&& dist < 48 )
	{
		Stat* targetStats = target->getStats();
		if ( !targetStats )
		{
			return;
		}

		// try to charm enemy.
		int specialRoll = -1;
		int bonusFromHP = 0;
		specialRoll = local_rng.rand() % 40;
		if ( myStats->HP <= myStats->MAXHP * 0.8 )
		{
			bonusFromHP += 2; // +% chance if on low health
		}
		if ( myStats->HP <= myStats->MAXHP * 0.4 )
		{
			bonusFromHP += 3; // +extra % chance if on lower health
		}

		int requiredRoll = (2 + bonusFromHP);

		if ( dist < STRIKERANGE )
		{
			requiredRoll += 5;
		}

		if ( specialRoll < requiredRoll )
		{
			monsterSpecialState = SLIME_CAST;
		}
	}
}