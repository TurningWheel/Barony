/*-------------------------------------------------------------------------------

	BARONY
	File: actHandMagic.cpp
	Desc: the spellcasting animations

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../main.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "../entity.hpp"
#include "../interface/interface.hpp"
#include "../engine/audio/sound.hpp"
#include "../items.hpp"
#include "../player.hpp"
#include "magic.hpp"
#include "../net.hpp"
#include "../scores.hpp"
#include "../ui/MainMenu.hpp"
#include "../prng.hpp"
#include "../mod_tools.hpp"

//The spellcasting animation stages:
#define CIRCLE 0 //One circle
#define THROW 1 //Throw spell!

spellcasting_animation_manager_t cast_animation[MAXPLAYERS];
bool overDrawDamageNotify = false;

#define HANDMAGIC_INIT my->skill[0]
#define HANDMAGIC_TESTVAR my->skill[1]
#define HANDMAGIC_PLAYERNUM my->skill[2]
#define HANDMAGIC_YAW my->fskill[3]
#define HANDMAGIC_PITCH my->fskill[4]
#define HANDMAGIC_ROLL my->fskill[5]
#define HANDMAGIC_PARTICLESPRAY1 my->skill[3]
#define HANDMAGIC_CIRCLE_RADIUS 0.8
#define HANDMAGIC_CIRCLE_SPEED 0.3

void spellcasting_animation_manager_t::resetRangefinder()
{
	target_x = 0.0;
	target_y = 0.0;
	caster_x = 0.0;
	caster_y = 0.0;
	rangefinder = false;
}

void spellcasting_animation_manager_t::setRangeFinderLocation()
{
	Entity* caster = uidToEntity(this->caster);
	rangefinder = false;
	if ( !caster )
	{
		return;
	}

	if ( !spell )
	{
		return;
	}

	if ( !spell->rangefinder )
	{
		return;
	}

	rangefinder = true;

	static ConsoleVariable<float> cvar_rangefinderStartZ("/rangefinder_start_z", -2.5);
	static ConsoleVariable<float> cvar_rangefinderMoveTo("/rangefinder_moveto_z", 0.1);
	static ConsoleVariable<float> cvar_rangefinderStartZLimit("/rangefinder_start_z_limit", 7.5);

	//real_t startx = cameras[player].x * 16.0;
	//real_t starty = cameras[player].y * 16.0;
	//real_t startz = cameras[player].z + (4.5 - cameras[player].z) / 2.0 + *cvar_rangefinderStartZ;
	//real_t pitch = cameras[player].vang;
	//const real_t yaw = cameras[player].ang;
	real_t startx = caster->x;
	real_t starty = caster->y;
	real_t startz = caster->z;
	real_t pitch = caster->pitch;
	const real_t yaw = caster->yaw;
	if ( pitch < 0 || pitch > PI )
	{
		pitch = 0;
	}

	// draw line from the players height and direction until we hit the ground.
	real_t previousx = startx;
	real_t previousy = starty;
	int index = 0;
	for ( ; startz < *cvar_rangefinderStartZLimit; startz += abs((*cvar_rangefinderMoveTo) * tan(pitch)) )
	{
		startx += 0.1 * cos(yaw);
		starty += 0.1 * sin(yaw);
		const int index_x = static_cast<int>(startx) >> 4;
		const int index_y = static_cast<int>(starty) >> 4;
		index = (index_y)*MAPLAYERS + (index_x)*MAPLAYERS * map.height;
		if ( map.tiles[index] && !map.tiles[OBSTACLELAYER + index] )
		{
			// store the last known good coordinate
			previousx = startx;// + 16 * cos(yaw);
			previousy = starty;// + 16 * sin(yaw);
		}
		if ( map.tiles[OBSTACLELAYER + index] )
		{
			break;
		}
		if ( pow(startx - cameras[player].x * 16.0, 2) + pow(starty - cameras[player].y * 16.0, 2) > (spell->distance * spell->distance) )
		{
			// break if distance reached
			break;
		}
	}

	target_x = previousx;
	target_y = previousy;
	caster_x = caster->x;
	caster_y = caster->y;
}

void fireOffSpellAnimation(spellcasting_animation_manager_t* animation_manager, Uint32 caster_uid, spell_t* spell, bool usingSpellbook)
{
	//This function triggers the spellcasting animation and sets up everything.

	if ( !animation_manager )
	{
		return;
	}
	Entity* caster = uidToEntity(caster_uid);
	if (!caster)
	{
		return;
	}
	int player = caster->skill[2];
	if ( !spell )
	{
		return;
	}
	if ( !players[player]->hud.magicLeftHand )
	{
		return;
	}
	if ( !players[player]->hud.magicRightHand )
	{
		return;
	}

	playSoundEntity(caster, 170, 128 );
	Stat* stat = caster->getStats();

	//Save these three very important pieces of data.
	animation_manager->player = caster->skill[2];
	animation_manager->caster = caster->getUID();
	animation_manager->spell = spell;

	if ( !usingSpellbook )
	{
		animation_manager->active = true;
	}
	else
	{
		animation_manager->active_spellbook = true;
	}
	animation_manager->stage = CIRCLE;

	//Make the HUDWEAPON disappear, or somesuch?
	players[player]->hud.magicLeftHand->flags[INVISIBLE_DITHER] = false;
	players[player]->hud.magicRightHand->flags[INVISIBLE_DITHER] = false;
	if ( stat->type != RAT )
	{
		if ( !usingSpellbook )
		{
			players[player]->hud.magicLeftHand->flags[INVISIBLE] = false;
			if ( caster->isInvisible() )
			{
				players[player]->hud.magicLeftHand->flags[INVISIBLE] = true;
				players[player]->hud.magicLeftHand->flags[INVISIBLE_DITHER] = true;
			}
		}
		players[player]->hud.magicRightHand->flags[INVISIBLE] = false;
		if ( caster->isInvisible() )
		{
			players[player]->hud.magicRightHand->flags[INVISIBLE] = true;
			players[player]->hud.magicRightHand->flags[INVISIBLE_DITHER] = true;
		}
	}

	animation_manager->lefthand_angle = 0;
	animation_manager->lefthand_movex = 0;
	animation_manager->lefthand_movey = 0;
	int spellCost = getCostOfSpell(spell, caster);
	animation_manager->circle_count = 0;
	animation_manager->times_to_circle = (spellCost / 10) + 1; //Circle once for every 10 mana the spell costs.
	animation_manager->mana_left = spellCost;
	animation_manager->consumeMana = true;
	if ( spell->ID == SPELL_FORCEBOLT && caster->skillCapstoneUnlockedEntity(PRO_SPELLCASTING) )
	{
		animation_manager->consumeMana = false;
	}

	if (stat->getModifiedProficiency(PRO_SPELLCASTING) < SPELLCASTING_BEGINNER)   //There's a chance that caster is newer to magic (and thus takes longer to cast a spell).
	{
		int chance = local_rng.rand() % 10;
		if (chance >= stat->getModifiedProficiency(PRO_SPELLCASTING) / 15)
		{
			int amount = (local_rng.rand() % 50) / std::max(stat->getModifiedProficiency(PRO_SPELLCASTING) + statGetINT(stat, caster), 1);
			amount = std::min(amount, CASTING_EXTRA_TIMES_CAP);
			animation_manager->times_to_circle += amount;
		}
	}
	if ( usingSpellbook && stat->shield && itemCategory(stat->shield) == SPELLBOOK )
	{
		if ( !playerLearnedSpellbook(player, stat->shield) || (stat->shield->beatitude < 0 && !shouldInvertEquipmentBeatitude(stat)) )
		{
			// for every tier below the spell you are, add 3 circle for 1 tier, or add 2 for every additional tier.
			int casterAbility = std::min(100, std::max(0, stat->getModifiedProficiency(PRO_SPELLCASTING) + statGetINT(stat, caster))) / 20;
			if ( stat->shield->beatitude < 0 )
			{
				casterAbility = 0; // cursed book has cast penalty.
			}
			int difficulty = spell->difficulty / 20;
			if ( difficulty > casterAbility )
			{
				animation_manager->times_to_circle += (std::min(5, 1 + 2 * (difficulty - casterAbility)));
			}
		}
		else if ( stat->getModifiedProficiency(PRO_SPELLCASTING) >= SPELLCASTING_BEGINNER )
		{
			animation_manager->times_to_circle = (spellCost / 20) + 1; //Circle once for every 20 mana the spell costs.
		}
	}
	animation_manager->consume_interval = (animation_manager->times_to_circle * ((2 * PI) / HANDMAGIC_CIRCLE_SPEED)) / spellCost;
	animation_manager->consume_timer = animation_manager->consume_interval;
	animation_manager->setRangeFinderLocation();
}

void spellcastingAnimationManager_deactivate(spellcasting_animation_manager_t* animation_manager)
{
	animation_manager->caster = -1;
	animation_manager->spell = NULL;
	animation_manager->active = false;
	animation_manager->active_spellbook = false;
	animation_manager->stage = 0;
	animation_manager->resetRangefinder();

	if ( animation_manager->player == -1 )
	{
		printlog("spellcastingAnimationManager_deactivate: Error - player was -1!");
		return;
	}
	//Make the hands invisible (should probably fall away or something, but whatever. That's another project for another day)
	if ( players[animation_manager->player]->hud.magicLeftHand )
	{
		players[animation_manager->player]->hud.magicLeftHand->flags[INVISIBLE] = true;
		players[animation_manager->player]->hud.magicLeftHand->flags[INVISIBLE_DITHER] = false;
	}
	if ( players[animation_manager->player]->hud.magicRightHand )
	{
		players[animation_manager->player]->hud.magicRightHand->flags[INVISIBLE] = true;
		players[animation_manager->player]->hud.magicRightHand->flags[INVISIBLE_DITHER] = false;
	}
	if ( players[animation_manager->player]->hud.magicRangefinder )
	{
		players[animation_manager->player]->hud.magicRangefinder->flags[INVISIBLE] = true;
		players[animation_manager->player]->hud.magicRangefinder->flags[INVISIBLE_DITHER] = false;
	}
}

void spellcastingAnimationManager_completeSpell(spellcasting_animation_manager_t* animation_manager)
{
	if ( animation_manager->rangefinder )
	{
		CastSpellProps_t castSpellProps;
		castSpellProps.caster_x = animation_manager->caster_x;
		castSpellProps.caster_y = animation_manager->caster_y;
		castSpellProps.target_x = animation_manager->target_x;
		castSpellProps.target_y = animation_manager->target_y;
		castSpell(animation_manager->caster, animation_manager->spell, false, false, animation_manager->active_spellbook, &castSpellProps); //Actually cast the spell.
	}
	else
	{
		castSpell(animation_manager->caster, animation_manager->spell, false, false, animation_manager->active_spellbook); //Actually cast the spell.
	}

	spellcastingAnimationManager_deactivate(animation_manager);
}

/*
[12:48:29 PM] Sheridan Kane Rathbun: you can move the entities about by modifying their x, y, z, yaw, pitch, and roll parameters.
[12:48:43 PM] Sheridan Kane Rathbun: everything's relative to the camera since the OVERDRAW flag is on.
[12:49:05 PM] Sheridan Kane Rathbun: so adding x will move it forward, adding y will move it sideways (forget which way) and adding z will move it up and down.
[12:49:46 PM] Sheridan Kane Rathbun: the first step is to get the hands visible on the screen when you cast. worry about moving them when that critical part is done.
*/

void actLeftHandMagic(Entity* my)
{
	//int c = 0;
	my->flags[INVISIBLE_DITHER] = false;
	if (intro == true)
	{
		my->flags[INVISIBLE] = true;
		return;
	}

	//Initialize
	if (!HANDMAGIC_INIT)
	{
		HANDMAGIC_INIT = 1;
		HANDMAGIC_TESTVAR = 0;
		my->focalz = -1.5;
	}

	if (players[HANDMAGIC_PLAYERNUM] == nullptr || players[HANDMAGIC_PLAYERNUM]->entity == nullptr
		|| (players[HANDMAGIC_PLAYERNUM]->entity && players[HANDMAGIC_PLAYERNUM]->entity->playerCreatedDeathCam != 0) )
	{
		players[HANDMAGIC_PLAYERNUM]->hud.magicLeftHand = nullptr;
		spellcastingAnimationManager_deactivate(&cast_animation[HANDMAGIC_PLAYERNUM]);
		list_RemoveNode(my->mynode);
		return;
	}

	//Set the initial values. (For the particle spray)
	my->x = 8;
	my->y = -3;
	my->z = (cameras[HANDMAGIC_PLAYERNUM].z * .5 - players[HANDMAGIC_PLAYERNUM]->entity->z) + 7;
	my->z -= 4;
	my->yaw = HANDMAGIC_YAW - cameravars[HANDMAGIC_PLAYERNUM].shakex2;
	double defaultpitch = (0 - 2.2);
	my->pitch = defaultpitch + HANDMAGIC_PITCH - cameravars[HANDMAGIC_PLAYERNUM].shakey2 / 200.f;
	my->roll = HANDMAGIC_ROLL;
	my->scalex = 0.5f;
	my->scaley = 0.5f;
	my->scalez = 0.5f;
	my->z -= 0.75;

	//Sprite
	Monster playerRace = players[HANDMAGIC_PLAYERNUM]->entity->getMonsterFromPlayerRace(stats[HANDMAGIC_PLAYERNUM]->playerRace);
	int playerAppearance = stats[HANDMAGIC_PLAYERNUM]->stat_appearance;
	if ( players[HANDMAGIC_PLAYERNUM]->entity->effectShapeshift != NOTHING )
	{
		playerRace = static_cast<Monster>(players[HANDMAGIC_PLAYERNUM]->entity->effectShapeshift);
	}
	else if ( players[HANDMAGIC_PLAYERNUM]->entity->effectPolymorph != NOTHING )
	{
		if ( players[HANDMAGIC_PLAYERNUM]->entity->effectPolymorph > NUMMONSTERS )
		{
			playerRace = HUMAN;
			playerAppearance = players[HANDMAGIC_PLAYERNUM]->entity->effectPolymorph - 100;
		}
		else
		{
			playerRace = static_cast<Monster>(players[HANDMAGIC_PLAYERNUM]->entity->effectPolymorph);
		}
	}

	bool noGloves = false;
	if ( stats[HANDMAGIC_PLAYERNUM]->gloves == NULL
		|| playerRace == SPIDER
		|| playerRace == RAT
		|| playerRace == CREATURE_IMP
		|| playerRace == TROLL )
	{
		noGloves = true;
	}
	else
	{
		if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == GLOVES || stats[HANDMAGIC_PLAYERNUM]->gloves->type == GLOVES_DEXTERITY )
		{
			my->sprite = 659;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == BRACERS || stats[HANDMAGIC_PLAYERNUM]->gloves->type == BRACERS_CONSTITUTION )
		{
			my->sprite = 660;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == GAUNTLETS || stats[HANDMAGIC_PLAYERNUM]->gloves->type == GAUNTLETS_STRENGTH )
		{
			my->sprite = 661;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == BRASS_KNUCKLES )
		{
			my->sprite = 662;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == IRON_KNUCKLES )
		{
			my->sprite = 663;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == SPIKED_GAUNTLETS )
		{
			my->sprite = 664;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == CRYSTAL_GLOVES )
		{
			my->sprite = 666;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == ARTIFACT_GLOVES )
		{
			my->sprite = 665;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == SUEDE_GLOVES )
		{
			my->sprite = 803;
		}
	}


	if ( noGloves )
	{

		switch ( playerRace )
		{
			case SKELETON:
				my->sprite = 773;
				break;
			case INCUBUS:
				my->sprite = 775;
				break;
			case SUCCUBUS:
				my->sprite = 777;
				break;
			case GOBLIN:
				my->sprite = 779;
				break;
			case AUTOMATON:
				my->sprite = 781;
				break;
			case INSECTOID:
				if ( stats[HANDMAGIC_PLAYERNUM]->sex == FEMALE )
				{
					my->sprite = 785;
				}
				else
				{
					my->sprite = 783;
				}
				break;
			case GOATMAN:
				my->sprite = 787;
				break;
			case VAMPIRE:
				my->sprite = 789;
				break;
			case HUMAN:
				if ( playerAppearance / 6 == 0 )
				{
					my->sprite = 656;
				}
				else if ( playerAppearance / 6 == 1 )
				{
					my->sprite = 657;
				}
				else
				{
					my->sprite = 658;
				}
				break;
			case TROLL:
				my->sprite = 856;
				break;
			case SPIDER:
				my->sprite = arachnophobia_filter ? 1006 : 854;
				break;
			case CREATURE_IMP:
				my->sprite = 858;
				break;
			default:
				my->sprite = 656;
				break;
		}
		/*}
		else if ( playerAppearance / 6 == 0 )
		{
			my->sprite = 656;
		}
		else if ( playerAppearance / 6 == 1 )
		{
			my->sprite = 657;
		}
		else
		{
			my->sprite = 658;
		}*/
	}

	Entity*& hudarm = players[HANDMAGIC_PLAYERNUM]->hud.arm;
	if ( playerRace == SPIDER && hudarm && players[HANDMAGIC_PLAYERNUM]->entity->bodyparts.at(0) )
	{
		my->x = hudarm->x;
		my->y = -hudarm->y;
		//my->z = hudArm->z;
		my->pitch = hudarm->pitch;
		my->roll = -hudarm->roll;
		my->yaw = -players[HANDMAGIC_PLAYERNUM]->entity->bodyparts.at(0)->yaw;
		my->scalex = hudarm->scalex;
		my->scaley = hudarm->scaley;
		my->scalez = hudarm->scalez;
		my->focalz = hudarm->focalz;
	}
	else
	{
		my->focalz = -1.5;
	}

	if ( !cast_animation[HANDMAGIC_PLAYERNUM].active )
	{
		my->flags[INVISIBLE] = true;
	}
	else
	{
		if ( playerRace == RAT )
		{
			my->flags[INVISIBLE] = true;
			my->y = 0;
			my->z += 1;
		}
		else if ( players[HANDMAGIC_PLAYERNUM]->entity->skill[3] == 1 )   // debug cam
		{
			my->flags[INVISIBLE] = true;
		}
		else if ( players[HANDMAGIC_PLAYERNUM]->entity->isInvisible() )
		{
			my->flags[INVISIBLE] = true;
			my->flags[INVISIBLE_DITHER] = true;
		}
	}

	if ( (cast_animation[HANDMAGIC_PLAYERNUM].active || cast_animation[HANDMAGIC_PLAYERNUM].active_spellbook) )
	{
		cast_animation[HANDMAGIC_PLAYERNUM].setRangeFinderLocation();
		switch ( cast_animation[HANDMAGIC_PLAYERNUM].stage)
		{
			case CIRCLE:
				if ( ticks % 5 == 0 && !(players[HANDMAGIC_PLAYERNUM]->entity->skill[3] == 1) )
				{
					Entity* entity = spawnGib(my);
					entity->flags[INVISIBLE] = false;
					entity->flags[SPRITE] = true;
					entity->flags[NOUPDATE] = true;
					entity->flags[UPDATENEEDED] = false;
					entity->flags[OVERDRAW] = true;
					entity->lightBonus = vec4(0.2f, 0.2f, 0.2f, 0.f);
					entity->scalex = 0.25f; //MAKE 'EM SMALL PLEASE!
					entity->scaley = 0.25f;
					entity->scalez = 0.25f;
					entity->sprite = 16; //TODO: Originally. 22. 16 -- spark sprite instead?
					entity->z += 3.0;
					if ( cast_animation[HANDMAGIC_PLAYERNUM].active_spellbook )
					{
						entity->y -= 1.5;
						entity->z += 1;
					}
					entity->yaw = ((local_rng.rand() % 6) * 60) * PI / 180.0;
					entity->pitch = (local_rng.rand() % 360) * PI / 180.0;
					entity->roll = (local_rng.rand() % 360) * PI / 180.0;
					entity->vel_x = cos(entity->yaw) * .1;
					entity->vel_y = sin(entity->yaw) * .1;
					entity->vel_z = -.15;
					entity->fskill[3] = 0.01;
					entity->skill[11] = HANDMAGIC_PLAYERNUM;
				}
				cast_animation[HANDMAGIC_PLAYERNUM].consume_timer--;
				if ( cast_animation[HANDMAGIC_PLAYERNUM].consume_timer < 0 && cast_animation[HANDMAGIC_PLAYERNUM].mana_left > 0 )
				{
					//Time to consume mana and reset the ticker!
					cast_animation[HANDMAGIC_PLAYERNUM].consume_timer = cast_animation[HANDMAGIC_PLAYERNUM].consume_interval;
					if ( multiplayer == SINGLE && cast_animation[HANDMAGIC_PLAYERNUM].consumeMana )
					{
						int HP = stats[HANDMAGIC_PLAYERNUM]->HP;
						int MP = stats[HANDMAGIC_PLAYERNUM]->MP;
						players[HANDMAGIC_PLAYERNUM]->entity->drainMP(1, false); // don't notify otherwise we'll get spammed each 1 mp

						if ( cast_animation[HANDMAGIC_PLAYERNUM].spell )
						{
							bool sustainedSpell = false;
							auto findSpellDef = ItemTooltips.spellItems.find(cast_animation[HANDMAGIC_PLAYERNUM].spell->ID);
							if ( findSpellDef != ItemTooltips.spellItems.end() )
							{
								sustainedSpell = (findSpellDef->second.spellType == ItemTooltips_t::SpellItemTypes::SPELL_TYPE_SELF_SUSTAIN);
							}
							if ( sustainedSpell )
							{
								players[HANDMAGIC_PLAYERNUM]->mechanics.sustainedSpellIncrementMP(MP - stats[HANDMAGIC_PLAYERNUM]->MP);
							}
						}

						if ( (HP > stats[HANDMAGIC_PLAYERNUM]->HP) && !overDrawDamageNotify )
						{
							overDrawDamageNotify = true;
							cameravars[HANDMAGIC_PLAYERNUM].shakex += 0.1;
							cameravars[HANDMAGIC_PLAYERNUM].shakey += 10;
							playSoundPlayer(HANDMAGIC_PLAYERNUM, 28, 92);
							Uint32 color = makeColorRGB(255, 255, 0);
							messagePlayerColor(HANDMAGIC_PLAYERNUM, MESSAGE_STATUS, color, Language::get(621));
						}
					}
					--cast_animation[HANDMAGIC_PLAYERNUM].mana_left;
				}

				cast_animation[HANDMAGIC_PLAYERNUM].lefthand_angle += HANDMAGIC_CIRCLE_SPEED;
				cast_animation[HANDMAGIC_PLAYERNUM].lefthand_movex = cos(cast_animation[HANDMAGIC_PLAYERNUM].lefthand_angle) * HANDMAGIC_CIRCLE_RADIUS;
				cast_animation[HANDMAGIC_PLAYERNUM].lefthand_movey = sin(cast_animation[HANDMAGIC_PLAYERNUM].lefthand_angle) * HANDMAGIC_CIRCLE_RADIUS;
				if ( cast_animation[HANDMAGIC_PLAYERNUM].lefthand_angle >= 2 * PI)   //Completed one loop.
				{
					cast_animation[HANDMAGIC_PLAYERNUM].lefthand_angle = 0;
					cast_animation[HANDMAGIC_PLAYERNUM].circle_count++;
					if ( cast_animation[HANDMAGIC_PLAYERNUM].circle_count >= cast_animation[HANDMAGIC_PLAYERNUM].times_to_circle)
						//Finished circling. Time to move on!
					{
						cast_animation[HANDMAGIC_PLAYERNUM].stage++;
					}
				}
				break;
			case THROW:
				//messagePlayer(HANDMAGIC_PLAYERNUM, "IN THROW");
				//TODO: Throw animation! Or something.
				cast_animation[HANDMAGIC_PLAYERNUM].stage++;
				break;
			default:
				//messagePlayer(HANDMAGIC_PLAYERNUM, "DEFAULT CASE");
				spellcastingAnimationManager_completeSpell(&cast_animation[HANDMAGIC_PLAYERNUM]);
				break;
		}
	}
	else
	{
		overDrawDamageNotify = false;
	}

	//Final position code.
	if (players[HANDMAGIC_PLAYERNUM] == nullptr || players[HANDMAGIC_PLAYERNUM]->entity == nullptr)
	{
		return;
	}
	//double defaultpitch = PI / 8.f;
	//double defaultpitch = 0;
	//double defaultpitch = PI / (0-4.f);
	//defaultpitch = (0 - 2.8);
	//my->x = 6 + HUDWEAPON_MOVEX;

	if ( playerRace == SPIDER && hudarm && players[HANDMAGIC_PLAYERNUM]->entity->bodyparts.at(0) )
	{
		my->x = hudarm->x;
		my->y = -hudarm->y;
		my->z = hudarm->z;
		my->pitch = hudarm->pitch;
		my->roll = -hudarm->roll;
		my->yaw = -players[HANDMAGIC_PLAYERNUM]->entity->bodyparts.at(0)->yaw;
		my->scalex = hudarm->scalex;
		my->scaley = hudarm->scaley;
		my->scalez = hudarm->scalez;
		my->focalz = hudarm->focalz;
	}
	else
	{
		my->y = -3;
		my->z = (cameras[HANDMAGIC_PLAYERNUM].z * .5 - players[HANDMAGIC_PLAYERNUM]->entity->z) + 7;
		my->z -= 4;
		my->yaw = HANDMAGIC_YAW - cameravars[HANDMAGIC_PLAYERNUM].shakex2;
		my->pitch = defaultpitch + HANDMAGIC_PITCH - cameravars[HANDMAGIC_PLAYERNUM].shakey2 / 200.f;
		my->roll = HANDMAGIC_ROLL;
		my->focalz = -1.5;
	}

	//my->y = 3 + HUDWEAPON_MOVEY;
	//my->z = (camera.z*.5-players[HANDMAGIC_PLAYERNUM]->z)+7+HUDWEAPON_MOVEZ; //TODO: NOT a PLAYERSWAP
	my->x += cast_animation[HANDMAGIC_PLAYERNUM].lefthand_movex;
	my->y += cast_animation[HANDMAGIC_PLAYERNUM].lefthand_movey;
}

void actRightHandMagic(Entity* my)
{
	my->flags[INVISIBLE_DITHER] = false;
	if (intro == true)
	{
		my->flags[INVISIBLE] = true;
		return;
	}

	//Initialize
	if ( !HANDMAGIC_INIT )
	{
		HANDMAGIC_INIT = 1;
		my->focalz = -1.5;
	}

	if (players[HANDMAGIC_PLAYERNUM] == nullptr || players[HANDMAGIC_PLAYERNUM]->entity == nullptr
		|| (players[HANDMAGIC_PLAYERNUM]->entity && players[HANDMAGIC_PLAYERNUM]->entity->playerCreatedDeathCam != 0) )
	{
		players[HANDMAGIC_PLAYERNUM]->hud.magicRightHand = nullptr;
		list_RemoveNode(my->mynode);
		return;
	}

	my->x = 8;
	my->y = 3;
	my->z = (cameras[HANDMAGIC_PLAYERNUM].z * .5 - players[HANDMAGIC_PLAYERNUM]->entity->z) + 7;
	my->z -= 4;
	my->yaw = HANDMAGIC_YAW - cameravars[HANDMAGIC_PLAYERNUM].shakex2;
	double defaultpitch = (0 - 2.2);
	my->pitch = defaultpitch + HANDMAGIC_PITCH - cameravars[HANDMAGIC_PLAYERNUM].shakey2 / 200.f;
	my->roll = HANDMAGIC_ROLL;
	my->scalex = 0.5f;
	my->scaley = 0.5f;
	my->scalez = 0.5f;
	my->z -= 0.75;

	//Sprite
	Monster playerRace = players[HANDMAGIC_PLAYERNUM]->entity->getMonsterFromPlayerRace(stats[HANDMAGIC_PLAYERNUM]->playerRace);
	int playerAppearance = stats[HANDMAGIC_PLAYERNUM]->stat_appearance;
	if ( players[HANDMAGIC_PLAYERNUM]->entity->effectShapeshift != NOTHING )
	{
		playerRace = static_cast<Monster>(players[HANDMAGIC_PLAYERNUM]->entity->effectShapeshift);
	}
	else if ( players[HANDMAGIC_PLAYERNUM]->entity->effectPolymorph != NOTHING )
	{
		if ( players[HANDMAGIC_PLAYERNUM]->entity->effectPolymorph > NUMMONSTERS )
		{
			playerRace = HUMAN;
			playerAppearance = players[HANDMAGIC_PLAYERNUM]->entity->effectPolymorph - 100;
		}
		else
		{
			playerRace = static_cast<Monster>(players[HANDMAGIC_PLAYERNUM]->entity->effectPolymorph);
		}
	}

	bool noGloves = false;
	if ( stats[HANDMAGIC_PLAYERNUM]->gloves == NULL
		|| playerRace == SPIDER 
		|| playerRace == RAT 
		|| playerRace == CREATURE_IMP
		|| playerRace == TROLL )
	{
		noGloves = true;
	}
	else
	{
		if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == GLOVES || stats[HANDMAGIC_PLAYERNUM]->gloves->type == GLOVES_DEXTERITY )
		{
			my->sprite = 637;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == BRACERS || stats[HANDMAGIC_PLAYERNUM]->gloves->type == BRACERS_CONSTITUTION )
		{
			my->sprite = 638;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == GAUNTLETS || stats[HANDMAGIC_PLAYERNUM]->gloves->type == GAUNTLETS_STRENGTH )
		{
			my->sprite = 639;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == BRASS_KNUCKLES )
		{
			my->sprite = 640;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == IRON_KNUCKLES )
		{
			my->sprite = 641;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == SPIKED_GAUNTLETS )
		{
			my->sprite = 642;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == CRYSTAL_GLOVES )
		{
			my->sprite = 591;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == ARTIFACT_GLOVES )
		{
			my->sprite = 590;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == SUEDE_GLOVES )
		{
			my->sprite = 802;
		}
	}

	if ( noGloves )
	{
		switch ( playerRace )
		{
			case SKELETON:
				my->sprite = 774;
				break;
			case INCUBUS:
				my->sprite = 776;
				break;
			case SUCCUBUS:
				my->sprite = 778;
				break;
			case GOBLIN:
				my->sprite = 780;
				break;
			case AUTOMATON:
				my->sprite = 782;
				break;
			case INSECTOID:
				if ( stats[HANDMAGIC_PLAYERNUM]->sex == FEMALE )
				{
					my->sprite = 786;
				}
				else
				{
					my->sprite = 784;
				}
				break;
			case GOATMAN:
				my->sprite = 788;
				break;
			case VAMPIRE:
				my->sprite = 790;
				break;
			case HUMAN:
				if ( playerAppearance / 6 == 0 )
				{
					my->sprite = 634;
				}
				else if ( playerAppearance / 6 == 1 )
				{
					my->sprite = 635;
				}
				else
				{
					my->sprite = 636;
				}
				break;
			case TROLL:
				my->sprite = 855;
				break;
			case SPIDER:
				my->sprite = arachnophobia_filter ? 1005 : 853;
				break;
			case CREATURE_IMP:
				my->sprite = 857;
				break;
			default:
				my->sprite = 634;
				break;
		}
		/*else if ( playerAppearance / 6 == 0 )
		{
			my->sprite = 634;
		}
		else if ( playerAppearance / 6 == 1 )
		{
			my->sprite = 635;
		}
		else
		{
			my->sprite = 636;
		}*/
	}

	if ( !(cast_animation[HANDMAGIC_PLAYERNUM].active || cast_animation[HANDMAGIC_PLAYERNUM].active_spellbook) )
	{
		my->flags[INVISIBLE] = true;
	}
	else
	{
		my->flags[INVISIBLE] = false;

		if ( playerRace == RAT )
		{
			my->flags[INVISIBLE] = true;
			my->y = 0;
			my->z += 1;
		}
		else if ( players[HANDMAGIC_PLAYERNUM]->entity->skill[3] == 1 )   // debug cam
		{
			my->flags[INVISIBLE] = true;
		}
		else if ( players[HANDMAGIC_PLAYERNUM]->entity->isInvisible() )
		{
			my->flags[INVISIBLE] = true;
			my->flags[INVISIBLE_DITHER] = true;
		}
	}

	Entity*& hudarm = players[HANDMAGIC_PLAYERNUM]->hud.arm;

	if ( playerRace == SPIDER && hudarm && players[HANDMAGIC_PLAYERNUM]->entity->bodyparts.at(0) )
	{
		my->x = hudarm->x;
		my->y = hudarm->y;
		//my->z = hudArm->z;
		my->pitch = hudarm->pitch;
		my->roll = hudarm->roll;
		my->yaw = players[HANDMAGIC_PLAYERNUM]->entity->bodyparts.at(0)->yaw;
		my->scalex = hudarm->scalex;
		my->scaley = hudarm->scaley;
		my->scalez = hudarm->scalez;
		my->focalz = hudarm->focalz;
	}
	else
	{
		my->focalz = -1.5;
	}

	if ( (cast_animation[HANDMAGIC_PLAYERNUM].active || cast_animation[HANDMAGIC_PLAYERNUM].active_spellbook) )
	{
		switch ( cast_animation[HANDMAGIC_PLAYERNUM].stage)
		{
			case CIRCLE:
				if ( ticks % 5 == 0 && !(players[HANDMAGIC_PLAYERNUM]->entity->skill[3] == 1) )
				{
					//messagePlayer(0, "Pingas!");
					Entity* entity = spawnGib(my);
					entity->flags[INVISIBLE] = false;
					entity->flags[SPRITE] = true;
					entity->flags[NOUPDATE] = true;
					entity->flags[UPDATENEEDED] = false;
					entity->flags[OVERDRAW] = true;
					entity->lightBonus = vec4(0.2f, 0.2f, 0.2f, 0.f);
					entity->z += 3.0;
					//entity->sizex = 1; //MAKE 'EM SMALL PLEASE!
					//entity->sizey = 1;
					entity->scalex = 0.25f; //MAKE 'EM SMALL PLEASE!
					entity->scaley = 0.25f;
					entity->scalez = 0.25f;
					entity->sprite = 16; //TODO: Originally. 22. 16 -- spark sprite instead?
					entity->yaw = ((local_rng.rand() % 6) * 60) * PI / 180.0;
					entity->pitch = (local_rng.rand() % 360) * PI / 180.0;
					entity->roll = (local_rng.rand() % 360) * PI / 180.0;
					entity->vel_x = cos(entity->yaw) * .1;
					entity->vel_y = sin(entity->yaw) * .1;
					entity->vel_z = -.15;
					entity->fskill[3] = 0.01;
					entity->skill[11] = HANDMAGIC_PLAYERNUM;
				}
				break;
			case THROW:
				break;
			default:
				break;
		}
	}

	//Final position code.
	if (players[HANDMAGIC_PLAYERNUM] == nullptr || players[HANDMAGIC_PLAYERNUM]->entity == nullptr)
	{
		return;
	}

	if ( playerRace == SPIDER && hudarm && players[HANDMAGIC_PLAYERNUM]->entity->bodyparts.at(0) )
	{
		my->x = hudarm->x;
		my->y = hudarm->y;
		my->z = hudarm->z;
		my->pitch = hudarm->pitch;
		my->roll = hudarm->roll;
		my->yaw = players[HANDMAGIC_PLAYERNUM]->entity->bodyparts.at(0)->yaw;
		my->scalex = hudarm->scalex;
		my->scaley = hudarm->scaley;
		my->scalez = hudarm->scalez;
		my->focalz = hudarm->focalz;
	}
	else
	{
		my->x = 8;
		my->y = 3;
		my->z = (cameras[HANDMAGIC_PLAYERNUM].z * .5 - players[HANDMAGIC_PLAYERNUM]->entity->z) + 7;
		my->z -= 4;
		my->yaw = HANDMAGIC_YAW - cameravars[HANDMAGIC_PLAYERNUM].shakex2;
		my->pitch = defaultpitch + HANDMAGIC_PITCH - cameravars[HANDMAGIC_PLAYERNUM].shakey2 / 200.f;
		my->roll = HANDMAGIC_ROLL;
		my->focalz = -1.5;
	}

	my->x += cast_animation[HANDMAGIC_PLAYERNUM].lefthand_movex;
	my->y -= cast_animation[HANDMAGIC_PLAYERNUM].lefthand_movey;
}

#define HANDMAGIC_RANGEFINDER_ALPHA my->fskill[0]
void actMagicRangefinder(Entity* my)
{
	my->flags[INVISIBLE_DITHER] = false;
	if ( intro == true )
	{
		my->flags[INVISIBLE] = true;
		return;
	}

	//Initialize
	if ( !HANDMAGIC_INIT )
	{
		HANDMAGIC_INIT = 1;
	}

	if ( players[HANDMAGIC_PLAYERNUM] == nullptr || players[HANDMAGIC_PLAYERNUM]->entity == nullptr
		|| (players[HANDMAGIC_PLAYERNUM]->entity && players[HANDMAGIC_PLAYERNUM]->entity->playerCreatedDeathCam != 0) )
	{
		players[HANDMAGIC_PLAYERNUM]->hud.magicRangefinder = nullptr;
		list_RemoveNode(my->mynode);
		return;
	}

	auto& cast_anim = cast_animation[HANDMAGIC_PLAYERNUM];

	if ( !(cast_anim.active || cast_anim.active_spellbook) || !cast_anim.rangefinder )
	{
		my->flags[INVISIBLE] = true;
		return;
	}
	if ( my->flags[INVISIBLE] )
	{
		my->bNeedsRenderPositionInit = true;
	}
	my->flags[INVISIBLE] = false;
	my->sprite = 222;
	my->x = cast_anim.target_x;
	my->y = cast_anim.target_y;
	my->z = 7.499;
	static ConsoleVariable<float> cvar_player_cast_indicator_scale("/player_cast_indicator_scale", 0.025);
	static ConsoleVariable<float> cvar_player_cast_indicator_rotate("/player_cast_indicator_rotate", 0.025);
	static ConsoleVariable<float> cvar_player_cast_indicator_alpha("/player_cast_indicator_alpha", 0.5);
	static ConsoleVariable<float> cvar_player_cast_indicator_alpha_glow("/player_cast_indicator_alpha_glow", 0.0625);
	my->ditheringDisabled = true;
	my->flags[SPRITE] = true;
	my->flags[PASSABLE] = true;
	my->flags[NOUPDATE] = true;
	my->flags[UNCLICKABLE] = true;
	my->flags[BRIGHT] = true;
	my->scalex = *cvar_player_cast_indicator_scale;
	my->scaley = *cvar_player_cast_indicator_scale;
	my->pitch = 0;
	my->roll = -PI / 2;

	HANDMAGIC_RANGEFINDER_ALPHA = *cvar_player_cast_indicator_alpha + 
		*cvar_player_cast_indicator_alpha_glow * sin(2 * PI * (my->ticks % TICKS_PER_SECOND) / (real_t)(TICKS_PER_SECOND));
	my->yaw += *cvar_player_cast_indicator_rotate;
}